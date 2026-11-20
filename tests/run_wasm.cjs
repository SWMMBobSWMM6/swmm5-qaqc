#!/usr/bin/env node
// Run one .inp through a WebAssembly build of the engine from Node.
//   node run_wasm.cjs <glue.js> <factoryName> <model.inp> <out.rpt> <out.out> [--stock]
// The glue is built for ENVIRONMENT=web,worker, so it is loaded with
// new Function and handed the wasm bytes explicitly.
const fs = require('fs');
const path = require('path');

async function main() {
  const [glue, factoryName, inp, rpt, out, flag] = process.argv.slice(2);
  const wasmPath = glue.replace(/\.js$/, '.wasm');
  const src = fs.readFileSync(glue, 'utf8');
  const factory = new Function('self', 'window', 'importScripts',
    src + '\nreturn ' + factoryName + ';')({ location: { href: 'file:///' } }, undefined, () => {});
  const mod = await factory({
    wasmBinary: fs.readFileSync(wasmPath),
    print: () => {}, printErr: () => {},
    locateFile: f => path.join(path.dirname(glue), f),
  });
  mod.FS.writeFile('/model.inp', fs.readFileSync(inp));
  if (flag === '--stock') mod.ccall('swmm_setQaqc', 'number', ['number'], [0]);
  let rc;
  if (typeof mod._swmm_run === 'function') {
    rc = mod.ccall('swmm_run', 'number', ['string', 'string', 'string'],
                   ['/model.inp', '/model.rpt', '/model.out']);
  } else throw new Error('swmm_run is not exported');
  fs.writeFileSync(rpt, mod.FS.readFile('/model.rpt'));
  let outBytes = new Uint8Array(0);
  try { outBytes = mod.FS.readFile('/model.out'); } catch (e) {}
  fs.writeFileSync(out, outBytes);
  console.log(JSON.stringify({ rc, rptBytes: fs.statSync(rpt).size, outBytes: outBytes.length,
    exports: ['_swmm_run', '_swmm_setQaqc', '_swmm_getQaqcCount', '_swmm_writeQaqcCatalog']
      .map(f => f + ':' + typeof mod[f]).join(' ') }));
}
main().catch(e => { console.error('FAILED', e && e.message ? e.message : e); process.exit(1); });

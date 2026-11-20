#!/usr/bin/env node
// Run every .inp in a folder through a WASM engine (fresh module per model)
// and compare the outputs with a folder of native outputs.
//   node wasm_corpus.cjs <glue.js> <factory> <swmm5|swmm6> <inp-dir> <native-dir> <out-dir> [stride] [stock]
// With the last argument 'stock' the QA/QC output is switched off (swmm_setQaqc(0) or --stock).
const fs = require('fs'), path = require('path');
const [glue, factoryName, kind, inpDir, nativeDir, outDir, strideArg, modeArg] = process.argv.slice(2);
const stockMode = modeArg === 'stock';
const stride = parseInt(strideArg || '1', 10);
const wasmBinary = fs.readFileSync(glue.replace(/\.js$/, '.wasm'));
const src = fs.readFileSync(glue, 'utf8');
const factory = new Function('self', 'window', 'importScripts',
  src + '\nreturn ' + factoryName + ';')({ location: { href: 'file:///' } }, undefined, () => {});
const strip = t => t.split('\n').filter(l => !/Analysis begun|Analysis ended|Total elapsed/.test(l)).join('\n');
(async () => {
  fs.mkdirSync(outDir, { recursive: true });
  const names = fs.readdirSync(inpDir).filter(f => /\.inp$/i.test(f)).sort().filter((_, i) => i % stride === 0);
  let outSame = 0, rptSame = 0, n = 0; const diff = [];
  for (const f of names) {
    const base = f.replace(/\.inp$/i, '');
    const mod = await factory({ wasmBinary, print: () => {}, printErr: () => {} });
    mod.FS.writeFile('/model.inp', fs.readFileSync(path.join(inpDir, f)));
    try {
      if (kind === 'swmm6') {
        try { mod.callMain((stockMode ? ['--stock'] : []).concat(['/model.inp', '/model.rpt', '/model.out'])); } catch (e) { if (!(e && typeof e.status === 'number')) throw e; }
      } else {
        if (stockMode && typeof mod._swmm_setQaqc === 'function') mod.ccall('swmm_setQaqc', 'number', ['number'], [0]);
        mod.ccall('swmm_run', 'number', ['string', 'string', 'string'], ['/model.inp', '/model.rpt', '/model.out']);
      }
    } catch (e) { diff.push(base + ' (threw: ' + (e && e.message ? e.message : e) + ')'); n++; continue; }
    let rpt = '', out = Buffer.alloc(0);
    try { rpt = mod.FS.readFile('/model.rpt', { encoding: 'utf8' }); } catch (e) {}
    try { out = Buffer.from(mod.FS.readFile('/model.out')); } catch (e) {}
    fs.writeFileSync(path.join(outDir, base + '.rpt'), rpt);
    fs.writeFileSync(path.join(outDir, base + '.out'), out);
    const nOutP = path.join(nativeDir, base + '.out'), nRptP = path.join(nativeDir, base + '.rpt');
    const nOut = fs.existsSync(nOutP) ? fs.readFileSync(nOutP) : Buffer.alloc(0);
    const nRpt = fs.existsSync(nRptP) ? fs.readFileSync(nRptP, 'utf8') : '';
    const o = Buffer.compare(out, nOut) === 0, r = strip(rpt) === strip(nRpt);
    if (o) outSame++; if (r) rptSame++;
    if (!o || !r) diff.push(base + (o ? '' : ' [.out differs]') + (r ? '' : ' [.rpt differs]'));
    n++;
  }
  console.log(JSON.stringify({ models: n, out_byte_identical: outSame, rpt_identical: rptSame, different: diff }, null, 1));
})().catch(e => { console.error('FAILED', e); process.exit(1); });

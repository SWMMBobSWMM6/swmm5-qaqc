// swmm-qaqc-worker.js - classic Web Worker that runs one model through the
// SWMM5 QA/QC engine or the SWMM6 QA/QC engine and posts back the report
// text and the binary output file.
//
//   const w = new Worker('swmm-qaqc-worker.js');
//   w.postMessage({ engine: 'swmm5', inp: inpText, qaqc: true });
//   w.onmessage = e => { e.data.rc, e.data.rpt, e.data.out (ArrayBuffer), e.data.error };
//
// One run per worker: terminate it afterwards. A fresh worker is the only
// reliable cancel and the only recovery from a WebAssembly trap.
/* global importScripts, createSwmm5QaqcModule, createSwmm6QaqcModule */
self.onmessage = async function (e) {
  var d = e.data || {};
  try {
    var mod, rc;
    if (d.engine === 'swmm6') {
      importScripts('swmm6-qaqc/swmm6-qaqc.js');
      mod = await createSwmm6QaqcModule({ print: function () {}, printErr: function () {} });
      mod.FS.writeFile('/model.inp', d.inp);
      var args = ['/model.inp', '/model.rpt', '/model.out'];
      if (d.qaqc === false) args.unshift('--stock');
      try { rc = mod.callMain(args); } catch (x) { if (x && typeof x.status === 'number') rc = x.status; else throw x; }
    } else {
      importScripts('swmm5-qaqc/swmm5-qaqc.js');
      mod = await createSwmm5QaqcModule({ print: function () {}, printErr: function () {} });
      mod.FS.writeFile('/model.inp', d.inp);
      mod.ccall('swmm_setQaqc', 'number', ['number'], [d.qaqc === false ? 0 : 1]);
      rc = mod.ccall('swmm_run', 'number', ['string', 'string', 'string'],
        ['/model.inp', '/model.rpt', '/model.out']);
    }
    var rpt = '', out = new Uint8Array(0);
    try { rpt = mod.FS.readFile('/model.rpt', { encoding: 'utf8' }); } catch (x) {}
    try { out = mod.FS.readFile('/model.out'); } catch (x) {}
    var buf = out.buffer.slice(out.byteOffset, out.byteOffset + out.byteLength);
    self.postMessage({ rc: rc, rpt: rpt, out: buf }, [buf]);
  } catch (err) {
    self.postMessage({ rc: -1, error: String(err && err.message ? err.message : err) });
  }
};

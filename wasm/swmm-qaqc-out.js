// swmm-qaqc-out.js - reader for the SWMM binary output file, including the
// QA/QC variables that swmm5-qaqc and swmm6-qaqc append to each record.
// Works in a browser, a Web Worker or Node. No dependencies.
//
//   const out = SwmmQaqcOut.parse(arrayBuffer, catalog);   // catalog = qaqc_variables.json (optional)
//   out.series('link', '1600', 'LINK_DQ2')  ->  { t: Float64Array, x: Float32Array }
//   out.names.link                          ->  ['FLOW', ..., 'LINK_FROUDE', ...]
//
// The QA/QC variables are identified by their header code (1000 + catalog
// index), so one catalog serves both engines even though the SWMM6 build
// appends no subcatchment variables.
(function (root) {
  'use strict';
  var MAGIC = 516114522, CODE_BASE = 1000;
  var STD = {
    subcatchment: ['RAINFALL', 'SNOWDEPTH', 'EVAP', 'INFIL', 'RUNOFF', 'GW_FLOW', 'GW_ELEV', 'SOIL_MOIST'],
    node: ['DEPTH', 'HEAD', 'VOLUME', 'LATFLOW', 'INFLOW', 'OVERFLOW'],
    link: ['FLOW', 'DEPTH', 'VELOCITY', 'VOLUME', 'CAPACITY'],
    system: ['TEMPERATURE', 'RAINFALL', 'SNOWDEPTH', 'INFIL', 'RUNOFF', 'DWFLOW', 'GWFLOW', 'IIFLOW',
      'EXFLOW', 'INFLOW', 'FLOODING', 'OUTFLOW', 'STORAGE', 'EVAP', 'PET']
  };
  var CLASSES = ['subcatchment', 'node', 'link', 'system'];

  function parse(buffer, catalog) {
    var dv = new DataView(buffer), len = buffer.byteLength;
    if (len < 60) throw new Error('file too short to be a SWMM output file');
    var t = len - 24;
    var idPos = dv.getInt32(t, true), inputPos = dv.getInt32(t + 4, true),
      outputPos = dv.getInt32(t + 8, true), nPeriods = dv.getInt32(t + 12, true),
      errorCode = dv.getInt32(t + 16, true);
    if (dv.getInt32(0, true) !== MAGIC || dv.getInt32(t + 20, true) !== MAGIC)
      throw new Error('not a SWMM output file (bad magic number)');
    var out = {
      version: dv.getInt32(4, true), flowUnits: dv.getInt32(8, true),
      n: { subcatchment: dv.getInt32(12, true), node: dv.getInt32(16, true), link: dv.getInt32(20, true), system: 1 },
      nPollut: dv.getInt32(24, true), nPeriods: nPeriods, errorCode: errorCode,
      ids: {}, codes: {}, names: {}, nvars: {}, nStd: {}, nQaqc: {}, offset: {}
    };
    var p = idPos, dec = new TextDecoder('latin1');
    [['subcatchment', out.n.subcatchment], ['node', out.n.node], ['link', out.n.link], ['pollutant', out.nPollut]]
      .forEach(function (c) {
        var names = [];
        for (var i = 0; i < c[1]; i++) {
          var ln = dv.getInt32(p, true); p += 4;
          names.push(dec.decode(new Uint8Array(buffer, p, ln))); p += ln;
        }
        out.ids[c[0]] = names;
      });
    p = inputPos;
    [out.n.subcatchment, out.n.node, out.n.link].forEach(function (cnt) {
      var nprops = dv.getInt32(p, true);
      p += 4 + 4 * nprops + cnt * 4 * nprops;
    });
    CLASSES.forEach(function (cls) {
      var nv = dv.getInt32(p, true); p += 4;
      var codes = [];
      for (var i = 0; i < nv; i++) { codes.push(dv.getInt32(p, true)); p += 4; }
      out.codes[cls] = codes;
    });
    out.startDate = dv.getFloat64(p, true); out.reportStep = dv.getInt32(p + 8, true); p += 12;
    if (p !== outputPos) throw new Error('header length does not match the output start position');
    var off = 8;
    CLASSES.forEach(function (cls) {
      var names = [], nq = 0;
      out.codes[cls].forEach(function (code, i) {
        if (code >= CODE_BASE) {
          var k = code - CODE_BASE; nq++;
          names.push(catalog && catalog[cls] && catalog[cls][k] ? catalog[cls][k].key : 'QAQC_' + k);
        } else if (i < STD[cls].length) names.push(STD[cls][i]);
        else names.push('POLLUT_' + (out.ids.pollutant[i - STD[cls].length] || (i - STD[cls].length)));
      });
      out.names[cls] = names; out.nvars[cls] = names.length; out.nQaqc[cls] = nq; out.nStd[cls] = names.length - nq;
      out.offset[cls] = off; off += 4 * out.n[cls] * names.length;
    });
    out.bytesPerPeriod = off;
    if (outputPos + nPeriods * off !== len - 24) throw new Error('record size mismatch');
    out.date = function (period) { return dv.getFloat64(outputPos + period * off, true); };
    out.value = function (cls, index, v, period) {
      return dv.getFloat32(outputPos + period * off + out.offset[cls] + 4 * (index * out.nvars[cls] + v), true);
    };
    out.series = function (cls, id, key) {
      var index = cls === 'system' ? 0 : out.ids[cls].indexOf(id), v = out.names[cls].indexOf(key);
      if (index < 0) throw new Error('no ' + cls + ' named ' + id);
      if (v < 0) throw new Error('no ' + cls + ' variable named ' + key);
      var tt = new Float64Array(nPeriods), xx = new Float32Array(nPeriods);
      for (var k = 0; k < nPeriods; k++) { tt[k] = out.date(k); xx[k] = out.value(cls, index, v, k); }
      return { t: tt, x: xx };
    };
    return out;
  }
  var api = { parse: parse, STD: STD, CLASSES: CLASSES };
  if (typeof module !== 'undefined' && module.exports) module.exports = api;
  else root.SwmmQaqcOut = api;
})(typeof self !== 'undefined' ? self : this);

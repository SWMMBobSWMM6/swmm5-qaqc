#!/usr/bin/env python3
"""Reader for the SWMM binary output file, including the QA/QC variables that
swmm5-qaqc appends to each record.

Works on a stock EPA SWMM 5.2.4 .out file as well (it then has no QA/QC part).
Standard library only.

    from swmm_qaqc_out import OutFile
    out = OutFile("model.out", "qaqc_variables.json")
    t, q = out.series("link", "1600", "LINK_DQ2")

Command line:
    swmm_qaqc_out.py model.out [catalog.json]            summary
    swmm_qaqc_out.py model.out catalog.json link 1600    CSV of one object
"""
import json
import struct
import sys

MAGIC = 516114522
QAQC_CODE_BASE = 1000

STD = {
    "subcatchment": ["RAINFALL", "SNOWDEPTH", "EVAP", "INFIL", "RUNOFF",
                     "GW_FLOW", "GW_ELEV", "SOIL_MOIST"],
    "node": ["DEPTH", "HEAD", "VOLUME", "LATFLOW", "INFLOW", "OVERFLOW"],
    "link": ["FLOW", "DEPTH", "VELOCITY", "VOLUME", "CAPACITY"],
    "system": ["TEMPERATURE", "RAINFALL", "SNOWDEPTH", "INFIL", "RUNOFF",
               "DWFLOW", "GWFLOW", "IIFLOW", "EXFLOW", "INFLOW", "FLOODING",
               "OUTFLOW", "STORAGE", "EVAP", "PET"],
}
CLASSES = ["subcatchment", "node", "link", "system"]


class OutFile:
    def __init__(self, path, catalog=None):
        self.path = path
        with open(path, "rb") as f:
            self.buf = f.read()
        b = self.buf
        if len(b) < 60:
            raise ValueError("file too short to be a SWMM output file")
        (self.id_pos, self.input_pos, self.output_pos, self.n_periods,
         self.error_code, magic2) = struct.unpack_from("<6i", b, len(b) - 24)
        magic, self.version, self.flow_units, n_sub, n_node, n_link, n_pol = \
            struct.unpack_from("<7i", b, 0)
        if magic != MAGIC or magic2 != MAGIC:
            raise ValueError("not a SWMM output file (bad magic number)")
        self.n = {"subcatchment": n_sub, "node": n_node, "link": n_link,
                  "system": 1}
        self.n_pollut = n_pol

        # --- object ID names
        p = self.id_pos
        self.ids = {}
        for cls, cnt in (("subcatchment", n_sub), ("node", n_node),
                         ("link", n_link), ("pollutant", n_pol)):
            names = []
            for _ in range(cnt):
                (ln,) = struct.unpack_from("<i", b, p)
                p += 4
                names.append(b[p:p + ln].decode("latin-1"))
                p += ln
            self.ids[cls] = names

        # --- skip the saved input properties
        p = self.input_pos
        for cnt, per_obj_ints in ((n_sub, 0), (n_node, 1), (n_link, 1)):
            (nprops,) = struct.unpack_from("<i", b, p)
            p += 4 + 4 * nprops                      # count + property codes
            # each object: optional type code (int) + remaining values (float)
            p += cnt * 4 * nprops if per_obj_ints else cnt * 4 * nprops
        # --- result variable codes for each class
        self.codes = {}
        for cls in CLASSES:
            (nv,) = struct.unpack_from("<i", b, p)
            p += 4
            self.codes[cls] = list(struct.unpack_from("<%di" % nv, b, p))
            p += 4 * nv
        self.start_date, self.report_step = struct.unpack_from("<di", b, p)
        p += 12
        if p != self.output_pos:
            raise ValueError("header length %d does not match output start %d"
                             % (p, self.output_pos))

        # --- variable names: standard, pollutants, then QA/QC
        self.catalog = None
        if catalog:
            with open(catalog) as f:
                self.catalog = json.load(f)
        self.names = {}
        self.n_qaqc = {}
        for cls in CLASSES:
            names = []
            nq = 0
            for i, code in enumerate(self.codes[cls]):
                if code >= QAQC_CODE_BASE:
                    k = code - QAQC_CODE_BASE
                    nq += 1
                    if self.catalog and k < len(self.catalog[cls]):
                        names.append(self.catalog[cls][k]["key"])
                    else:
                        names.append("QAQC_%d" % k)
                elif i < len(STD[cls]):
                    names.append(STD[cls][i])
                else:
                    pi = i - len(STD[cls])
                    pol = self.ids["pollutant"]
                    names.append("POLLUT_" + (pol[pi] if pi < len(pol) else str(pi)))
            self.names[cls] = names
            self.n_qaqc[cls] = nq
        self.nvars = {cls: len(self.codes[cls]) for cls in CLASSES}
        self.n_std = {cls: self.nvars[cls] - self.n_qaqc[cls] for cls in CLASSES}

        # --- record layout
        self.offset = {}
        off = 8
        for cls in CLASSES:
            self.offset[cls] = off
            off += 4 * self.n[cls] * self.nvars[cls]
        self.bytes_per_period = off
        expect = self.output_pos + self.n_periods * self.bytes_per_period
        if expect != len(b) - 24:
            raise ValueError("record size mismatch: expected results to end at "
                             "%d, file has %d" % (expect, len(b) - 24))

    # ---------------------------------------------------------------------
    def date(self, period):
        return struct.unpack_from(
            "<d", self.buf, self.output_pos + period * self.bytes_per_period)[0]

    def record(self, cls, index, period):
        """All values (standard + pollutants + QA/QC) of one object."""
        nv = self.nvars[cls]
        p = (self.output_pos + period * self.bytes_per_period +
             self.offset[cls] + 4 * index * nv)
        return struct.unpack_from("<%df" % nv, self.buf, p)

    def record_bytes(self, cls, index, period, first=0, count=None):
        nv = self.nvars[cls]
        if count is None:
            count = nv
        p = (self.output_pos + period * self.bytes_per_period +
             self.offset[cls] + 4 * (index * nv + first))
        return self.buf[p:p + 4 * count]

    def index(self, cls, name):
        if cls == "system":
            return 0
        return self.ids[cls].index(name)

    def var(self, cls, key):
        return self.names[cls].index(key)

    def series(self, cls, name, key):
        i = self.index(cls, name)
        v = self.var(cls, key)
        t = [self.date(p) for p in range(self.n_periods)]
        x = [self.record(cls, i, p)[v] for p in range(self.n_periods)]
        return t, x

    def summary(self):
        s = ["%s: version %d, %d periods of %d s" %
             (self.path, self.version, self.n_periods, self.report_step)]
        for cls in CLASSES:
            s.append("  %-12s %5d objects, %3d variables (%d standard/pollutant"
                     " + %d QA/QC)" % (cls, self.n[cls], self.nvars[cls],
                                       self.n_std[cls], self.n_qaqc[cls]))
        return "\n".join(s)


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 1
    out = OutFile(argv[1], argv[2] if len(argv) > 2 else None)
    if len(argv) < 5:
        print(out.summary())
        return 0
    cls, name = argv[3], argv[4]
    i = out.index(cls, name)
    print("period,date," + ",".join(out.names[cls]))
    for p in range(out.n_periods):
        rec = out.record(cls, i, p)
        print("%d,%.6f," % (p + 1, out.date(p)) +
              ",".join("%.6g" % v for v in rec))
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

#!/usr/bin/env python3
"""Does the SWMM6 build report the same solver terms as the SWMM5 build?

usage: compare_swmm5_swmm6.py <swmm5-qaqc-run-dir> <swmm6-qaqc-run-dir> <catalog.json> [report.json]

The two engines do not give identical results on every model, so a difference
in a QA/QC variable can come from the engines or from the port. To separate
the two, a link-period is compared only where the two engines' standard
results for that link (flow, depth, velocity, volume, capacity) are
bit-identical and the momentum kernel ran in both (LINK_AKON > 0). On those
records the QA/QC variables should agree as well; a systematic disagreement
there points at the capture code or at a real difference in how the two
kernels define the quantity, not at the engines' results.

For each link QA/QC variable the report gives the number of records compared,
how many agree to a relative 1e-4 (absolute 1e-6), and the largest relative
difference.
"""
import json
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools"))
from swmm_qaqc_out import OutFile  # noqa: E402


def main(argv):
    d5, d6, catalog = argv[1:4]
    names = sorted(f[:-4] for f in os.listdir(d5) if f.endswith(".out"))
    stats = {}
    models_used = 0
    records = 0
    skipped = []
    for n in names:
        p5 = os.path.join(d5, n + ".out")
        p6 = os.path.join(d6, n + ".out")
        if not os.path.exists(p6) or os.path.getsize(p5) < 100 or os.path.getsize(p6) < 100:
            skipped.append(n)
            continue
        try:
            a = OutFile(p5, catalog)
            b = OutFile(p6, catalog)
        except Exception:
            skipped.append(n)
            continue
        if (a.n_periods != b.n_periods or a.ids["link"] != b.ids["link"]
                or a.n_std["link"] != b.n_std["link"] or a.n_qaqc["link"] == 0
                or b.n_qaqc["link"] == 0):
            skipped.append(n)
            continue
        ns = a.n_std["link"]
        keys = [k for k in a.names["link"][ns:] if k in b.names["link"]]
        ia = [a.var("link", k) for k in keys]
        ib = [b.var("link", k) for k in keys]
        ka = a.var("link", "LINK_AKON")
        kb = b.var("link", "LINK_AKON")
        used = False
        for p in range(a.n_periods):
            for i in range(a.n["link"]):
                if a.record_bytes("link", i, p, 0, ns) != b.record_bytes("link", i, p, 0, ns):
                    continue
                ra = a.record("link", i, p)
                rb = b.record("link", i, p)
                # only where the momentum kernel ran in both engines
                if ra[ka] <= 0.0 or rb[kb] <= 0.0:
                    continue
                used = True
                records += 1
                for k, xa, xb in zip(keys, ia, ib):
                    x, y = ra[xa], rb[xb]
                    s = stats.setdefault(k, [0, 0, 0.0])
                    s[0] += 1
                    diff = abs(x - y)
                    rel = diff / max(abs(x), abs(y), 1e-30)
                    if diff <= 1e-6 + 1e-4 * max(abs(x), abs(y)):
                        s[1] += 1
                    else:
                        s[2] = max(s[2], rel)
        if used:
            models_used += 1
    out = {"models_with_identical_link_records": models_used,
           "link_periods_compared": records, "models_skipped": len(skipped),
           "variables": {k: {"compared": v[0], "agree": v[1],
                             "agree_pct": round(100.0 * v[1] / max(v[0], 1), 4),
                             "max_rel_diff_where_not": v[2]} for k, v in stats.items()}}
    print("models with at least one bit-identical link record:", models_used)
    print("link-periods compared:", records)
    print("%-20s %12s %10s" % ("variable", "agree %", "max rel"))
    for k, v in out["variables"].items():
        print("%-20s %12.4f %10.3g" % (k, v["agree_pct"], v["max_rel_diff_where_not"]))
    if len(argv) > 4:
        json.dump(out, open(argv[4], "w"), indent=1)
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

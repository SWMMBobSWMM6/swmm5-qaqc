#!/usr/bin/env python3
"""Checks that decide whether the QA/QC engine can be trusted.

usage: verify_corpus.py <inp-dir> <stock-run-dir> <qaqc-run-dir> <qaqc-stockmode-run-dir>
                        <catalog.json> [report.json] [--skip-c5]

For every model in <inp-dir>:

 C1  null effect, switch off:  the QA/QC engine run with --stock writes a
     binary output file byte-identical to the unmodified engine's.
 C2  null effect, switch on:   with QA/QC output on, the report file is
     identical (apart from the run time lines) ...
 C3  ... and every standard variable of every object in every period is
     bit-identical to the unmodified engine's output.
 C4  the QA/QC values obey the identities the solver's own equations imply
     (float32 tolerance):
       a  AKON   = 1 + DQ1 + DQ5
       b  DQ5    = UPHLoss + DNHLoss + MIDHLoss
       c  AWTD   = A1 + (AMID - A1) * RHO
       d  VENANT/b * AKON = QOLD/b - DQ2 + DQ3 + DQ4 + DQ6   (b = barrels)
     where the conduit kernel ran (LINK_TS > 0 and AKON > 0).
 C5  where the last output record falls on the simulation end time, its
     SYS_CE equals the flow routing continuity error printed in the report
     file, to the 3 decimals the report prints.

Exit status 0 only if every check passes on every model that ran.
"""
import json
import math
import os
import re
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "tools"))
from swmm_qaqc_out import OutFile, CLASSES  # noqa: E402

TIME_LINES = re.compile(r"Analysis begun|Analysis ended|Total elapsed")


def rpt_text(path):
    with open(path, errors="replace") as f:
        return [ln for ln in f if not TIME_LINES.search(ln)]


def barrels_from_inp(path):
    """link id -> number of barrels, from [XSECTIONS]."""
    b = {}
    sect = ""
    with open(path, errors="replace") as f:
        for ln in f:
            s = ln.strip()
            if not s or s.startswith(";"):
                continue
            if s.startswith("["):
                sect = s.upper()
                continue
            if sect.startswith("[XSECT"):
                t = s.split(";")[0].split()
                if len(t) >= 7 and t[1].upper() not in ("CUSTOM", "IRREGULAR", "STREET"):
                    try:
                        b[t[0]] = max(1, int(float(t[6])))
                    except ValueError:
                        pass
    return b


def routing_ce_from_rpt(lines):
    """Flow routing continuity error (%) from the report, or None."""
    inside = False
    for ln in lines:
        if "Flow Routing Continuity" in ln:
            inside = True
        elif inside and "Continuity Error (%)" in ln:
            try:
                return float(ln.split()[-1])
            except ValueError:
                return None
    return None


def end_date_from_inp(path):
    """Simulation end as an OLE date (days since 12/30/1899), or None."""
    import datetime
    opt = {}
    sect = ""
    with open(path, errors="replace") as f:
        for ln in f:
            s = ln.strip()
            if s.startswith("["):
                sect = s.upper()
                continue
            if sect.startswith("[OPTION") and s and not s.startswith(";"):
                t = s.split()
                if len(t) >= 2:
                    opt[t[0].upper()] = t[1]
    try:
        m, d, y = [int(z) for z in re.split(r"[/-]", opt["END_DATE"])]
        tt = [int(z) for z in opt.get("END_TIME", "0:0:0").split(":")] + [0, 0]
        dt = datetime.datetime(y, m, d, tt[0] % 24, tt[1], tt[2]) + \
            datetime.timedelta(days=tt[0] // 24)
        return (dt - datetime.datetime(1899, 12, 30)).total_seconds() / 86400.0
    except Exception:
        return None


def close(a, b, rel=2e-4, abs_=2e-4):
    return abs(a - b) <= abs_ + rel * max(abs(a), abs(b))


def check_model(name, inp_dir, stock_dir, qaqc_dir, stockmode_dir, catalog):
    r = {"model": name}
    s_out = os.path.join(stock_dir, name + ".out")
    q_out = os.path.join(qaqc_dir, name + ".out")
    m_out = os.path.join(stockmode_dir, name + ".out")
    s_rpt = rpt_text(os.path.join(stock_dir, name + ".rpt"))
    q_rpt = rpt_text(os.path.join(qaqc_dir, name + ".rpt"))

    # C2 report text
    r["C2_rpt_identical"] = (s_rpt == q_rpt)

    ran = os.path.exists(s_out) and os.path.getsize(s_out) > 100 and \
        not any("ERROR" in ln for ln in s_rpt)
    r["ran"] = ran
    if not ran:
        # an input error: nothing more to compare than the report text
        r["C1_stockmode_out_identical"] = (
            (not os.path.exists(s_out) and not os.path.exists(m_out)) or
            (os.path.exists(s_out) and os.path.exists(m_out) and
             open(s_out, "rb").read() == open(m_out, "rb").read()))
        return r

    # C1 bytes
    r["C1_stockmode_out_identical"] = (open(s_out, "rb").read() == open(m_out, "rb").read())

    stock = OutFile(s_out)
    qa = OutFile(q_out, catalog)
    r["periods"] = qa.n_periods
    r["objects"] = {c: qa.n[c] for c in CLASSES}

    # C3 standard variables bit-identical
    same = (stock.n_periods == qa.n_periods and
            all(stock.n[c] == qa.n[c] and stock.nvars[c] == qa.n_std[c] for c in CLASSES))
    compared = 0
    if same:
        for p in range(qa.n_periods):
            if stock.date(p) != qa.date(p):
                same = False
                break
            for c in CLASSES:
                ns = qa.n_std[c]
                for i in range(qa.n[c]):
                    if stock.record_bytes(c, i, p) != qa.record_bytes(c, i, p, 0, ns):
                        same = False
                        break
                    compared += ns
                if not same:
                    break
            if not same:
                break
    r["C3_standard_vars_bit_identical"] = same
    r["C3_values_compared"] = compared

    # C4 identities on link QA/QC values
    barrels = barrels_from_inp(os.path.join(inp_dir, name + ".inp"))
    ident = {"a": [0, 0, 0.0], "b": [0, 0, 0.0], "c": [0, 0, 0.0], "d": [0, 0, 0.0]}
    if qa.n_qaqc["link"] > 0 and qa.n["link"] > 0:
        v = {k: qa.var("link", k) for k in (
            "LINK_AKON", "LINK_DQ1", "LINK_DQ2", "LINK_DQ3", "LINK_DQ4", "LINK_DQ5",
            "LINK_DQ6", "LINK_UPHLoss", "LINK_DNHLoss", "LINK_MIDHLoss", "LINK_AWTD",
            "LINK_A1", "LINK_AMID", "LINK_RHO", "LINK_VENANT", "LINK_QOLD", "LINK_TS")}

        def tally(key, lhs, rhs, scale):
            t = ident[key]
            t[0] += 1
            err = abs(lhs - rhs) / max(scale, 1e-6)
            t[2] = max(t[2], err)
            if not close(lhs, rhs, rel=5e-4, abs_=5e-4 * max(scale, 1e-3)):
                t[1] += 1

        for p in range(qa.n_periods):
            for i, lid in enumerate(qa.ids["link"]):
                x = qa.record("link", i, p)
                akon = x[v["LINK_AKON"]]
                if x[v["LINK_TS"]] <= 0 or akon <= 0:
                    continue
                if any(math.isnan(z) or math.isinf(z) for z in x):
                    ident["a"][1] += 1
                    continue
                dq1, dq5 = x[v["LINK_DQ1"]], x[v["LINK_DQ5"]]
                tally("a", akon, 1.0 + dq1 + dq5, akon)
                tally("b", dq5, x[v["LINK_UPHLoss"]] + x[v["LINK_DNHLoss"]] +
                      x[v["LINK_MIDHLoss"]], max(abs(dq5), 1e-3))
                a1, amid = x[v["LINK_A1"]], x[v["LINK_AMID"]]
                tally("c", x[v["LINK_AWTD"]], a1 + (amid - a1) * x[v["LINK_RHO"]],
                      max(abs(amid), abs(a1)))
                b = barrels.get(lid, 1)
                lhs = x[v["LINK_VENANT"]] / b * akon
                rhs = (x[v["LINK_QOLD"]] / b - x[v["LINK_DQ2"]] + x[v["LINK_DQ3"]] +
                       x[v["LINK_DQ4"]] + x[v["LINK_DQ6"]])
                scale = max(abs(x[v["LINK_QOLD"]] / b), abs(x[v["LINK_DQ2"]]),
                            abs(x[v["LINK_DQ3"]]), abs(x[v["LINK_DQ4"]]), abs(lhs))
                tally("d", lhs, rhs, scale)
    r["C4_identities"] = {k: {"checked": t[0], "failed": t[1], "max_rel_err": t[2]}
                          for k, t in ident.items()}
    r["C4_pass"] = all(t[1] == 0 for t in ident.values())

    # C5 final SYS_CE against the report
    ce_rpt = routing_ce_from_rpt(q_rpt)
    r["C5_rpt_ce"] = ce_rpt
    end = end_date_from_inp(os.path.join(inp_dir, name + ".inp"))
    at_end = (end is not None and qa.n_periods > 0 and
              abs(qa.date(qa.n_periods - 1) - end) < 1.5 / 86400.0)
    r["C5_last_record_is_simulation_end"] = at_end
    if ce_rpt is not None and qa.n_qaqc["system"] > 0 and at_end and not SKIP_C5:
        ce = qa.record("system", 0, qa.n_periods - 1)[qa.var("system", "SYS_CE")]
        r["C5_sys_ce_last"] = ce
        r["C5_pass"] = abs(ce - ce_rpt) <= 0.0015
    else:
        r["C5_pass"] = None
    return r


SKIP_C5 = False


def main(argv):
    global SKIP_C5
    if "--skip-c5" in argv:      # the SWMM6 build does not fill SYS_CE
        SKIP_C5 = True
        argv = [a for a in argv if a != "--skip-c5"]
    inp_dir, stock_dir, qaqc_dir, stockmode_dir, catalog = argv[1:6]
    names = sorted(f[:-4] for f in os.listdir(inp_dir) if f.lower().endswith(".inp"))
    results = []
    for n in names:
        try:
            results.append(check_model(n, inp_dir, stock_dir, qaqc_dir, stockmode_dir, catalog))
        except Exception as e:  # report, never hide
            results.append({"model": n, "exception": repr(e)})

    def count(key, value=True):
        return sum(1 for r in results if r.get(key) is value)

    ran = [r for r in results if r.get("ran")]
    summary = {
        "models": len(results),
        "ran_without_input_error": len(ran),
        "exceptions": [r["model"] for r in results if "exception" in r],
        "C1_stockmode_out_identical": count("C1_stockmode_out_identical"),
        "C2_rpt_identical": count("C2_rpt_identical"),
        "C3_standard_vars_bit_identical": sum(1 for r in ran if r.get("C3_standard_vars_bit_identical")),
        "C3_values_compared": sum(r.get("C3_values_compared", 0) for r in ran),
        "C4_pass": sum(1 for r in ran if r.get("C4_pass")),
        "C4_checked": {k: sum(r["C4_identities"][k]["checked"] for r in ran) for k in "abcd"},
        "C4_failed": {k: sum(r["C4_identities"][k]["failed"] for r in ran) for k in "abcd"},
        "C5_pass": sum(1 for r in ran if r.get("C5_pass") is True),
        "C5_applicable": sum(1 for r in ran if r.get("C5_pass") is not None),
    }
    print(json.dumps(summary, indent=2))
    bad = [r["model"] for r in results if "exception" in r
           or not r.get("C1_stockmode_out_identical") or not r.get("C2_rpt_identical")
           or (r.get("ran") and (not r.get("C3_standard_vars_bit_identical")
                                 or not r.get("C4_pass") or r.get("C5_pass") is False))]
    print("FAILING MODELS:", bad if bad else "none")
    if len(argv) > 6:
        with open(argv[6], "w") as f:
            json.dump({"summary": summary, "failing": bad, "models": results}, f, indent=1)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))

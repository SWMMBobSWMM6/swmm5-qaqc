#!/usr/bin/env python3
"""Write docs/VARIABLES.md from qaqc_variables.json (which the engine itself
writes with --catalog), so the table cannot drift from the code."""
import json, sys
cat = json.load(open(sys.argv[1]))
swmm6_zero = {"link": ["LINK_R2", "LINK_W2", "LINK_F2"],
              "node": ["NODE_Infil", "NODE_Evap"],
              "system": ["SYS_CE"]}
out = ["# QA/QC variables", "",
       "Generated from `qaqc_variables.json`, which the engine writes itself "
       "(`swmm5-qaqc --catalog qaqc_variables.json`). Do not edit by hand.", "",
       "Each record in the binary output file holds the standard variables, then the "
       "pollutants, then the variables below in index order. In the file header each "
       "QA/QC variable has the code `1000 + index`.", "",
       "`flow` means the model's FLOW_UNITS. Units are shown as US / SI.", "",
       "The *SWMM6* column says whether the SWMM6 build fills the variable. The SWMM6 "
       "build appends no subcatchment variables at all.", ""]
titles = {"link": "Link variables", "node": "Node variables",
          "subcatchment": "Subcatchment variables (SWMM5 build only)",
          "system": "System variables"}
for cls in ("link", "node", "system", "subcatchment"):
    rows = cat[cls]
    out += ["## %s (%d)" % (titles[cls], len(rows)), "",
            "| # | Variable | Units | What the solver value is | Computed in | SWMM6 |",
            "|--:|---|---|---|---|:-:|"]
    for v in rows:
        u = v["unitsUS"] if v["unitsUS"] == v["unitsSI"] else "%s / %s" % (v["unitsUS"], v["unitsSI"])
        s6 = "no" if cls == "subcatchment" or v["key"] in swmm6_zero.get(cls, []) else "yes"
        out.append("| %d | `%s` | %s | %s | `%s` | %s |" % (
            v["index"], v["key"], u or "-", v["definition"].replace("|", "\\|"), v["source"], s6))
    out.append("")
open(sys.argv[2], "w").write("\n".join(out))
print(sys.argv[2], sum(len(cat[c]) for c in titles))

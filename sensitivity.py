"""Run a predeclared development-only sensitivity grid. Never opens holdout.
Python standard library only. Every run uses identical evaluation dates.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import subprocess


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--exe", type=Path, required=True)
    p.add_argument("--data", type=Path, required=True)
    p.add_argument("--start-date", required=True)
    p.add_argument("--holdout-date", required=True)
    p.add_argument("--out", type=Path, required=True)
    a = p.parse_args()
    a.out.mkdir(parents=True, exist_ok=False)
    plan = {"trend_windows": [100, 150, 200, 250, 300], "slippage_bps": [1, 5, 10, 20],
            "start_date": a.start_date, "holdout_date": a.holdout_date, "phase": "development",
            "data_sha256": hashlib.sha256(a.data.read_bytes()).hexdigest(),
            "note": "20 planned trials; do not treat the best result as independent evidence."}
    (a.out / "experiment_plan.json").write_text(json.dumps(plan, indent=2), encoding="utf-8")
    rows = []
    for window in plan["trend_windows"]:
        for slip in plan["slippage_bps"]:
            destination = a.out / f"sma_{window}_slip_{slip}"
            subprocess.run([str(a.exe.resolve()), "--data", str(a.data.resolve()),
                            "--start-date", a.start_date, "--holdout-date", a.holdout_date,
                            "--phase", "development", "--trend-window", str(window),
                            "--slippage-bps", str(slip), "--out", str(destination.resolve())],
                           check=True, stdout=subprocess.DEVNULL)
            with (destination / "development" / "summary.csv").open(newline="") as f:
                for row in csv.DictReader(f):
                    rows.append({"trend_window": window, "slippage_bps": slip, **row})
            print(f"Completed window={window}, slippage={slip} bps")
    with (a.out / "sensitivity.csv").open("x", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0])); w.writeheader(); w.writerows(rows)
    print("Finished. Holdout was NOT evaluated. Look for broad stability, not the single best cell.")


if __name__ == "__main__":
    main()

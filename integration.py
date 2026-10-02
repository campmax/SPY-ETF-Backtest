"""End-to-end checks of the compiled program. Uses temporary synthetic fixtures."""
import argparse
import csv
import importlib.util
import json
from datetime import date, datetime, timedelta
from pathlib import Path
import subprocess
import sys
import tempfile


def main():
    p = argparse.ArgumentParser()
    p.add_argument("--exe", required=True, type=Path)
    a = p.parse_args()
    exe = a.exe.resolve()
    root = Path(__file__).resolve().parents[1]
    count = 0

    def check(condition, title):
        nonlocal count
        if not condition:
            raise AssertionError(title)
        count += 1
        print("PASS", title)

    def run(*args, ok=True):
        r = subprocess.run([str(exe), *map(str, args)], capture_output=True, text=True)
        if (r.returncode == 0) != ok:
            raise AssertionError(f"Unexpected exit {r.returncode}: {r.stdout}\n{r.stderr}")
        return r

    with tempfile.TemporaryDirectory(prefix="quantlab_test_") as td:
        temp = Path(td)
        demo = temp / "demo"
        result = run("--demo", "--out", demo)
        check("invented data" in result.stdout and not (demo / "holdout").exists(),
              "default run labels synthetic data and does not evaluate holdout")
        report = (demo / "development/report.html").read_text()
        check(report.count("<polyline") == 6 and "SYNTHETIC DATA" in report,
              "HTML report includes six equity/drawdown series and synthetic notice")
        manifest = (demo / "development/run_manifest.txt").read_text()
        check("canonical_data_fnv1a64=" in manifest and "slippage_bps=5" in manifest,
              "run provenance and cost assumptions are saved")
        old = (demo / "development/summary.csv").read_bytes()
        run("--demo", "--out", demo, ok=False)
        check(old == (demo / "development/summary.csv").read_bytes(), "existing run cannot be overwritten")
        fixture = demo / "data_snapshot.csv"
        run("--data", fixture, "--start-date", "2007-01-01", "--holdout-date", "2021-01-01",
            "--phase", "both", "--out", temp / "both")
        with (temp / "both/holdout/trend_equity.csv").open() as f:
            first = next(csv.DictReader(f))
        check(first["date"] == "2021-01-01", "explicit holdout starts on its first available session")
        with (temp / "both/development/trend_equity.csv").open() as f:
            points = list(csv.DictReader(f))
        check(points[0]["date"] == "2007-01-01" and points[-1]["date"] < "2021-01-01",
              "fixed evaluation date and nonoverlapping development interval")
        for args in [("--fee", "nan"), ("--trend-window", "2.5"), ("--allocation", "2"),
                     ("--phase", "typo"), ("--bogus", "5"), ("--data", "missing.csv"),
                     ("--demo", "--data", str(fixture))]:
            run(*args, ok=False)
        check(True, "invalid CLI values and conflicting sources reject")
        lines = fixture.read_text().splitlines()
        for kind in ("header", "duplicate", "nan", "trailing", "bad_date"):
            changed = lines.copy()
            if kind == "header": changed[0] = "Date,Close"
            if kind == "duplicate": changed.insert(2, changed[1])
            if kind == "nan":
                fields = changed[1].split(","); fields[1] = "nan"; changed[1] = ",".join(fields)
            if kind == "trailing": changed[1] += ","
            if kind == "bad_date": changed[1] = "2005-02-30," + changed[1].split(",", 1)[1]
            bad = temp / f"bad_{kind}.csv"; bad.write_text("\n".join(changed))
            run("--data", bad, "--out", temp / ("output_" + kind), ok=False)
        check(True, "CSV schema duplicate nonfinite trailing-field and date errors reject")
        # Accept Windows CRLF and UTF-8 BOM while preserving all numeric values.
        bom = temp / "bom.csv"
        bom.write_bytes(b"\xef\xbb\xbf" + ("\r\n".join(lines) + "\r\n").encode())
        run("--data", bom, "--out", temp / "bom_run")
        check(old == (temp / "bom_run/development/summary.csv").read_bytes(),
              "CSV round-trip including BOM/CRLF preserves results")
        sweep = temp / "sweep"
        subprocess.run([sys.executable, str(root / "tools/sensitivity.py"), "--exe", str(exe),
                        "--data", str(fixture), "--start-date", "2007-01-01",
                        "--holdout-date", "2021-01-01", "--out", str(sweep)], check=True,
                       stdout=subprocess.DEVNULL)
        with (sweep / "sensitivity.csv").open() as f:
            rows = list(csv.DictReader(f))
        manifests = list(sweep.glob("sma_*/development/run_manifest.txt"))
        check(len(rows) == 60 and len(manifests) == 20 and not list(sweep.glob("*/holdout")),
              "sensitivity grid runs 20 trials and never evaluates holdout")
        check(all("evaluation_start=2007-01-01" in f.read_text() and
                  "evaluation_end=2020-12-31" in f.read_text() for f in manifests),
              "all sensitivity trials use identical evaluation dates")

    # Exercise downloader conversion without the network or third-party packages.
    spec = importlib.util.spec_from_file_location("fetch_spy", root / "tools/fetch_spy.py")
    module = importlib.util.module_from_spec(spec); spec.loader.exec_module(module)

    class SplitSeries:
        def __init__(self, values): self.values = values
        def fillna(self, value): return self
        def __ne__(self, value): return SplitSeries([v != value for v in self.values])
        def any(self): return any(self.values)

    class History:
        columns = ["Open", "High", "Low", "Close", "Volume", "Dividends", "Stock Splits"]
        def __init__(self):
            self.rows = [(datetime(2000, 1, 1) + timedelta(days=i),
                         {"Open": 100., "High": 101., "Low": 99., "Close": 100.,
                          "Volume": 1000., "Dividends": 0., "Stock Splits": 0.}) for i in range(400)]
        @property
        def empty(self): return not self.rows
        def __getitem__(self, column): return SplitSeries([r[column] for _, r in self.rows])
        def iterrows(self): return iter(self.rows)

    def convert(h): return module.convert(h, date(2000, 1, 1), date(2002, 1, 1))
    h = History(); h.rows[10][1]["Dividends"] = 1; h.rows[10][1]["Capital Gains"] = 0.5
    check(convert(h)[10][-2:] == [1.5, 1], "downloader preserves cash distributions and no-split factor")
    for kind in ("split", "nan", "duplicate", "short"):
        h = History()
        if kind == "split": h.rows[20][1]["Stock Splits"] = 2
        if kind == "nan": h.rows[20][1]["Open"] = float("nan")
        if kind == "duplicate": h.rows[20] = h.rows[19]
        if kind == "short": h.rows = h.rows[:10]
        rejected = False
        try: convert(h)
        except ValueError: rejected = True
        check(rejected, "downloader conversion rejects " + kind)
    print(f"{count} integration/conversion checks passed")


if __name__ == "__main__":
    main()

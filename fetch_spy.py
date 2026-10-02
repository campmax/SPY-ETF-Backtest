"""Download daily SPY data for personal research; never writes over an existing file.

Optional dependency: python -m pip install yfinance
The backtesting engine itself is entirely C++ and has no external dependencies.
Yahoo OHLC is split-adjusted even with auto_adjust=False. This deliberately
SPY-only importer rejects ANY split in the returned full history, so it cannot
silently mix adjusted prices with explicit share multipliers.
"""
import argparse
import csv
from datetime import date, datetime, timezone
import hashlib
import json
import math
from pathlib import Path
import sys


def convert(history, start, end):
    if history.empty:
        raise ValueError("No history returned. Check network access or retry later.")
    if "Stock Splits" not in history.columns or "Dividends" not in history.columns:
        raise ValueError("Provider omitted corporate-action columns; refusing ambiguous data.")
    if (history["Stock Splits"].fillna(0) != 0).any():
        raise ValueError("Split detected in full history. This SPY importer refuses split-adjusted OHLC. Use verified raw prices/actions from your vendor.")
    rows = []
    for timestamp, r in history.iterrows():
        session = timestamp.date()
        if not start <= session < end:
            continue
        values = [float(r[k]) for k in ("Open", "High", "Low", "Close", "Volume")]
        distribution = float(r["Dividends"]) + float(r.get("Capital Gains", 0))
        if not all(math.isfinite(x) for x in values + [distribution]):
            raise ValueError(f"Missing/nonfinite data on {session}; not silently dropping session.")
        o, h, low, c, vol = values
        if min(o, h, low, c) <= 0 or low > min(o, c) or h < max(o, c) or min(vol, distribution) < 0:
            raise ValueError(f"Invalid prices/actions on {session}.")
        rows.append([session.isoformat(), *values, distribution, 1])
    if len(rows) < 300:
        raise ValueError("Need at least 300 complete daily sessions.")
    dates = [row[0] for row in rows]
    if dates != sorted(set(dates)):
        raise ValueError("Provider dates are duplicated or unordered.")
    return rows


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("--start", default="2000-01-01", type=date.fromisoformat)
    p.add_argument("--end", default="2026-01-01", type=date.fromisoformat,
                   help="EXCLUSIVE; fixed default keeps the baseline reproducible")
    p.add_argument("--out", default="data/SPY.csv", type=Path)
    a = p.parse_args()
    if a.start >= a.end or a.end > date.today():
        p.error("Use start < end <= today; today's incomplete session is excluded.")
    meta = a.out.with_suffix(".metadata.json")
    if a.out.exists() or meta.exists():
        p.error("Output or metadata already exists; choose a new filename.")
    try:
        import yfinance as yf
    except ImportError:
        p.exit(1, "Install optional downloader dependency: py -m pip install yfinance\n")
    print("Fetching full SPY history and checking corporate actions...")
    ticker = yf.Ticker("SPY")
    history = ticker.history(period="max", interval="1d", auto_adjust=False,
                             back_adjust=False, actions=True, repair=False,
                             keepna=True, raise_errors=True)
    metadata = ticker.get_history_metadata()
    if metadata.get("currency") != "USD":
        raise ValueError("Expected USD; provider currency is missing or different.")
    rows = convert(history, a.start, a.end)
    a.out.parent.mkdir(parents=True, exist_ok=True)
    with a.out.open("x", newline="", encoding="utf-8") as f:
        writer = csv.writer(f)
        writer.writerow(["date", "open", "high", "low", "close", "volume", "dividend", "split"])
        writer.writerows(rows)
    payload = {
        "ticker": "SPY", "source": "Yahoo Finance via yfinance", "currency": "USD",
        "retrieved_utc": datetime.now(timezone.utc).isoformat(), "yfinance_version": yf.__version__,
        "requested_start": a.start.isoformat(), "requested_end_exclusive": a.end.isoformat(),
        "first_date": rows[0][0], "last_date": rows[-1][0], "rows": len(rows),
        "sha256": hashlib.sha256(a.out.read_bytes()).hexdigest(),
        "auto_adjust": False, "repair": False, "full_history_split_check": "no split events returned",
        "dividend_column": "cash Dividends plus separately reported Capital Gains",
        "limitations": "Provider data may be revised or omit events. Independently verify before relying on research. Personal use subject to provider terms.",
    }
    with meta.open("x", encoding="utf-8") as f:
        json.dump(payload, f, indent=2)
    print(f"Saved {len(rows)} sessions to {a.out}; provenance and SHA-256: {meta}")


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"Download failed: {exc}", file=sys.stderr)
        raise SystemExit(1)

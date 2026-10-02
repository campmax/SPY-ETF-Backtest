# QuantLab 1.0

A C++17 daily ETF backtesting project for Magnus. Two interchangeable strategies,
buy-and-hold comparison, explicit portfolio accounting, separate development and
holdout periods, audit logs, and local HTML reports. No broker connection.

**Start with `START_HERE.md`.** You can use a complete project or paste the supplied
`Backtester.cpp` into your existing Visual Studio console project.

## What is included

- Trend following: invest above a rolling simple moving average, otherwise cash.
- Mean reversion: enter after a negative rolling z-score; exit on recovery or timeout.
- Buy-and-hold over precisely the same evaluation sessions and starting capital.
- Prior-close signals, next-open fills, fixed commissions, adverse slippage.
- Cash/share ledger, fractional or whole-share entries, allocation limits.
- Raw-price split handling and explicit cash distributions.
- Optional cash yield, return/CAGR, drawdown, volatility, Sharpe, exposure, trade counts.
- Event ledger and daily equity CSVs for every strategy, plus an HTML comparison.
- Immutable run folders, input snapshots, settings and data fingerprints.
- Development-only default; explicit command required to evaluate holdout.
- 20-trial development-only parameter/cost sensitivity tool with fixed dates.
- 20 C++ accounting/execution tests, plus Python integration checks.
- A deterministic synthetic fixture, labeled throughout as **NOT SPY**.
- Optional SPY downloader with data-provenance and SHA-256 metadata.

## Status and important limits

This is a usable educational/research first release, not a validated institutional
execution simulator. It supports one long-only instrument per run. It does not yet
implement multi-asset rotation, pairs trading, shorting, options, futures,
intraday order books, or live execution.

The supplied example uses **invented data**. A real SPY download was rate-limited
in the build environment; there is no claimed SPY performance result. The live
downloader must be verified on your machine. The C++ code was compiled and tested
on Linux with GCC; Windows/MSVC and the Windows launcher were not available for
execution here. Both source and build configuration target standard C++17.

Dividends are credited as cash on their ex-date, a documented approximation to
payment-date accounting, and are not automatically reinvested. Price data must
follow the exact contract in `docs/DATA.md`; adjusted prices plus explicit
corporate actions can double-count returns. No liquidity/market impact, taxes,
settlement restrictions, or dynamic spread model is included.

## Project layout

| Path | Purpose |
|---|---|
| `Backtester.cpp` | Generated standalone version; paste or compile this alone |
| `src/engine.hpp` | Data validation, signals, accounting, metrics, synthetic fixture |
| `src/main.cpp` | CLI, reports, output files and reproducibility |
| `tests/tests.cpp` | Hand-checkable accounting and execution tests |
| `tests/integration.py` | CLI/parser/report/standalone integration checks |
| `tools/fetch_spy.py` | Optional historical data downloader |
| `tools/sensitivity.py` | Fixed-date, development-only sensitivity experiment |
| `tools/amalgamate.py` | Regenerates the standalone source from the modular files |
| `examples/demo_run/` | Completed synthetic example and audit outputs |
| `docs/METHODOLOGY.md` | Exact strategy, accounting and metric definitions |
| `docs/RESEARCH_PLAN.md` | How to use development and holdout honestly |
| `docs/DATA.md` | Input schema and downloader assumptions |
| `docs/VALIDATION.md` | What was actually tested and what remains unverified |

## Command-line quick start

From this project's directory, after building:

```powershell
.\build\quantlab.exe --demo
py -m pip install yfinance
py tools\fetch_spy.py --start 2000-01-01 --end 2026-01-01 --out data\SPY.csv
.\build\quantlab.exe --data data\SPY.csv --start-date 2002-01-01 --holdout-date 2021-01-01 --phase development --out results\spy_baseline_dev
```

Dates are example choices, not an assertion that 2021–2025 is unseen historical
information. If you already used that period to choose rules, it is not genuinely
untouched. The script's end date is exclusive. Save the downloaded metadata along
with your experiment notes; never overwrite a data snapshot to refresh a run.

After rules are fixed and your research notes are recorded:

```powershell
.\build\quantlab.exe --data data\SPY.csv --start-date 2002-01-01 --holdout-date 2021-01-01 --phase holdout --out results\spy_frozen_holdout
```

Sensitivity study (20 development trials; all dates identical):

```powershell
py tools\sensitivity.py --exe build\quantlab.exe --data data\SPY.csv --start-date 2002-01-01 --holdout-date 2021-01-01 --out results\spy_sensitivity
```

Each output directory must be new. This protects earlier evidence from accidental
replacement. Inspect `report.html`, then reconcile a few trades with the input.
Use `--help` for the complete option list. Rates and allocations are decimals;
`--cash-rate 0.04` means 4%, while `--slippage-bps 5` means 0.05% per side.

## Linux/macOS build

```bash
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic src/main.cpp -o quantlab
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic tests/tests.cpp -o quantlab_tests
./quantlab_tests
./quantlab --demo
python3 tests/integration.py --exe ./quantlab
```

Or use CMake:

```bash
cmake -S . -B build
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

No Python is required to build or run the backtester. Python is only used for
optional acquisition/research helpers and additional integration verification.

## Sources consulted

- Microsoft: https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio
- yfinance API: https://ranaroussi.github.io/yfinance/reference/yfinance.price_history.html
- yfinance source: https://github.com/ranaroussi/yfinance/blob/main/yfinance/scrapers/history.py
- Research pitfalls: https://www.quantconnect.com/docs/v2/writing-algorithms/key-concepts/research-guide

Market-data access is subject to provider terms. Nothing in this project places
orders or establishes that either strategy has a profitable edge.

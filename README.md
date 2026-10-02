
**Start with `START_HERE.md`.** paste the 
`Backtester.cpp` into a Visual Studio console project.

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


## Sources consulted

- Microsoft: https://learn.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio
- yfinance API: https://ranaroussi.github.io/yfinance/reference/yfinance.price_history.html
- yfinance source: https://github.com/ranaroussi/yfinance/blob/main/yfinance/scrapers/history.py
- Research pitfalls: https://www.quantconnect.com/docs/v2/writing-algorithms/key-concepts/research-guide

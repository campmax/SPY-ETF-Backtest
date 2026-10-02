# Validation record

Build environment: Linux, GCC 13.3.0, C++17. Both the modular program and the
generated standalone `Backtester.cpp` were compiled with `-Wall -Wextra -Wpedantic`.

## C++ suite: 20 checks passed

1. Prior-close signal executes at next open, including a large overnight gap.
2. Two-for-one split preserves equity and adjusts shares.
3. Dividend offsets an ex-date price drop exactly once.
4. Ex-date new buyer does not receive the distribution.
5. Simultaneous split/distribution uses post-split shares.
6. Entry/exit adverse slippage and fixed commissions reconcile to hand calculations.
7. Mutating a future bar does not alter preceding decisions/equity.
8. Mean-reversion holding timeout counts the entry session correctly.
9. Mean recovery exits at the next open.
10. Flat series produces zero volatility and undefined Sharpe.
11. Invalid dates/OHLC/numbers/configuration reject; leap date arithmetic works.
12. Whole-share entry budget includes commission.
13. Insufficient whole-share budget records a rejected order.
14. Cash interest uses actual elapsed calendar days.
15. Terminal open positions mark to close without a forced sell.
16. Indicator history adjusts for splits only when effective.
17. Entry costs contribute to drawdown from initial capital.
18. Holdout starts with new cash while using earlier observations for warmup.
19. Reverse split preserves portfolio value.
20. All three strategies preserve cash/share/equity invariants over the full fixture.

## Integration/conversion suite: 16 checks passed

- Default demonstration is explicitly synthetic and does not create holdout output.
- HTML includes six plotted series and the synthetic-data notice.
- Manifest records data fingerprint and costs.
- Existing run folder is refused without changing its files.
- Explicit holdout boundary is honored.
- Fixed evaluation start and nonoverlapping development boundary are honored.
- Invalid CLI values/conflicting sources reject (seven scenarios).
- Invalid CSV schemas/dates/values reject (five scenarios).
- CSV round-trip, UTF-8 BOM and Windows CRLF preserve numeric results.
- Twenty sensitivity runs produce sixty strategy rows and never open holdout.
- Every sensitivity run uses the identical date interval.
- Downloader conversion preserves distributions and the no-split factor.
- Downloader conversion rejects splits, nonfinite values, duplicates and too-short
  histories (four separate checks).

Conversion checks use an offline fixture with the same fields as the provider
response; they do not establish successful live yfinance access.

## Additional boundaries

AddressSanitizer/UndefinedBehaviorSanitizer was used for the C++ suite. Leak
detection is disabled for that run because the container prevents LeakSanitizer
from inspecting process tasks; no leak-detection claim is made.

Windows Visual Studio/MSVC, Windows launcher execution and CMake execution were
not available in this environment. The delivered CMake configuration and Windows
instructions still require confirmation on the user's machine. No Windows binary
is bundled; compile the source locally.

The data provider returned HTTP 429 on the historical-data connection attempt.
No real SPY backtest was executed. Included performance charts demonstrate
synthetic mechanics only. No real-market edge, forecast accuracy, broker execution
quality or production readiness has been verified.

The HTML series count and content were checked programmatically; the report was
not visually tested in a Windows browser.

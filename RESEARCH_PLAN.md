# First research project

Question: **Does a trend filter reduce drawdowns enough to justify its costs and
any foregone returns relative to buy-and-hold?** Mean reversion is a separate
predeclared alternative hypothesis, not an automatic fallback if trend loses.

## 1. Understand the demonstration

Run the synthetic example. Find an entry signal in `trend_equity.csv`, locate
the following session's BUY in `trend_events.csv`, and reconcile shares, entry
price, fee, cash and closing equity. Do the same for a dividend and a split in
the synthetic benchmark ledger. The demonstration tests mechanics, not an edge.

## 2. Acquire and freeze data

Download SPY data or provide a verified file under the documented schema. Retain
the CSV and provenance JSON. Choose development and holdout dates before viewing
results. An illustrative split is development from 2002 to 2020 and holdout from
2021 to 2025, using data since 2000 for warmup. These historical years may already
be familiar; label them honestly rather than claiming they are truly unseen.

Use `--start-date` and `--holdout-date` explicitly. Without them the program chooses
the first post-warmup observation and a 70% split for convenience. Those default
dates can shift when lookback windows change, so do not use defaults for a fair
parameter comparison.

## 3. Record a baseline before seeing results

- Trend: 200 sessions; above mean invested, otherwise cash.
- Mean reversion: 20 sessions, z <= -2 entry, exit at mean or five held sessions.
- Allocation: 100% of entry cash, no leverage; fractional shares initially.
- Costs: $1 per fill, five basis points each way; sensitivity is essential.
- Cash and risk-free rate: 0 for the simple baseline, then separately examine
  clearly documented alternative assumptions. A constant rate is not historical
  Treasury yield data.
- Compare annualized return, maximum drawdown, volatility, Sharpe, exposure,
  trade count and costs. Define how much return you would sacrifice for reduced
  drawdown before looking for the most attractive trade-off.

## 4. Inspect only development results

Run `--phase development`, the default. Ask whether risk reduction comes merely
from holding less stock; interpret exposure alongside return. Low exposure alone
can lower drawdown without demonstrating predictive skill. An exposure-matched
benchmark is a sensible future extension, not implemented in this first release.

The supplied sensitivity helper predeclares five trend windows and four slippage
levels: 20 development runs. It fixes all evaluation dates and saves its plan
before running. Look for reasonably consistent neighboring settings. Do not crown
the highest Sharpe among 20 tests as a statistically established edge. The mean
reversion and buy-and-hold rows repeat across trend windows because their rules
did not change; these are not independent additional discoveries.

## 5. Freeze a candidate and evaluate holdout once

Write down the chosen hypothesis, parameters, data hash, cost assumptions and
acceptance criteria. Run `--phase holdout` with the same settings. Each period
starts in cash; do not stitch those independent curves into a continuous live
portfolio return. Inspect whether results survive the new interval.

The program warns when holdout is opened, but cannot prevent you from repeatedly
opening it. If you revise rules based on its results, reclassify it as development
data. A new evaluation then needs genuinely new data or a properly predeclared
walk-forward design. Poor results are a legitimate research finding.

## 6. Extend only after the accounting is understood

Priorities for another engineering iteration:

1. Dividend payable-date receivables and historical daily cash yields.
2. Exposure-matched benchmarks and block-bootstrap uncertainty estimates.
3. Predeclared rolling/expanding walk-forward evaluation.
4. Multi-asset accounting, calendars and allocation before relative momentum.
5. Short borrow, financing and relationship estimation before pairs trading.
6. Intraday quotes, liquidity and calibrated execution before high-frequency work.

Keep an experiment diary: date, hypothesis, data version, parameters, number of
trials, outcome, and whether holdout was consulted. The most useful project
demonstration is being able to explain an assumption, find its failure mode,
and show the check that catches it.

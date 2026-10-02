# Methodology and assumptions

## Scope and event order

One instrument, long-only, no leverage, one position at a time. Dates are sorted
session dates. Each period starts with its own $100,000 by default, zero shares,
and no inherited trades. Historical observations before that period initialize
the indicators. All strategies use the same start/end sessions and capital.

Each evaluated session processes these operations in order:

1. Accrue effective annual interest on carried cash using the actual calendar gap
   since the prior evaluated date. No interest is credited before period entry.
2. Apply that session's split to carried shares. Total entry cost basis is unchanged.
3. Credit distributions to carried post-split shares, before any opening trade.
4. Execute the previous close's pending entry or exit at this session's open.
5. Mark cash plus shares at this session's close.
6. Adjust the indicator's stored closes for today's split and append today's close.
7. Calculate the next session's decision from this completed close.

There is no assumed fill at the close that creates the signal. Orders do not use
today's high, low, close or full-day volume to decide their opening fill. The
open fill is an approximation, not a guarantee of participating in an opening
auction. Any pending decision on the final bar remains unexecuted.

## Strategy rules

**Trend:** include the current completed close in the trailing N-session arithmetic
mean (default 200). If close > mean, target a long position; otherwise target cash.
If already long, do not rebalance daily. Reentry invests the configured fraction
of current cash. Equality means cash. Splits rescale past indicator observations
only as they occur. The signal is split-adjusted price, not dividend total return.

**Mean reversion:** use a trailing N-session mean and population standard deviation
(default 20), both including today's close. z = (close - mean) / stddev; define z=0
if stddev is essentially zero. While flat, enter if z <= -2 by default. While long,
exit if close >= mean or the position has been held for five evaluated sessions.
The entry session counts as held session one; timeout exit executes at the next
open. No same-open exit/reentry or daily position resizing.

**Buy-and-hold:** buy at the first evaluated open, keep the position through the
last close. Uses the same fee, slippage, allocation, rounding, cash yield, dividend
and corporate-action assumptions. Distributions accumulate as cash. Consequently
this is a cash-dividend buy-and-hold portfolio, not a dividend-reinvested index.

## Sizing, fills and costs

Default initial capital = $100,000; allocation = 1; fixed commission = $1 per fill;
adverse slippage = 5 basis points each side. All are explicit illustrative settings,
not calibrated broker quotes. One basis point = 0.01%.

Entry price = open × (1 + slippage_bps/10,000).
Entry shares = max(0, (cash × allocation - commission) / entry price).
Optional whole-share mode floors this entry quantity. Cash subtracts the full cost
including commission. Insufficient budget records REJECT and pays no commission.

Exit price = open × (1 - slippage_bps/10,000).
Exit proceeds = shares × exit price - commission. Negative proceeds cannot create
borrowing; if available equity cannot cover the fee the run fails visibly.

Slippage is already embedded in fill prices. The reported slippage cost is an
attribution measure and is **not subtracted a second time**. Turnover sums opening
reference-price notional for buys and sells, divided by initial capital.

Fractional shares are a simplification; even whole-share mode can retain fractional
shares after a reverse split. No cash-in-lieu conversion is modeled. No partial
fills, auction constraints, volume participation cap, market impact, spread
estimation, taxes, settlement, or broker rounding. Large accounts are not validated
by these assumptions.

## Corporate actions

Input OHLC must be raw/as-traded. `split` = new shares / old shares effective before
that session opens; 1 means none, 2 means a two-for-one split, 0.1 means a one-for-ten
reverse split. `dividend` is cash per post-split share on the ex-date.

A shareholder carried from yesterday gets the distribution even if selling at
today's open. A purchaser at today's open does not. On a simultaneous split and
distribution, split first, then use the post-split per-share cash amount.

Cash is credited immediately on the ex-date. A real account may not receive the
cash until the payable date, so this approximates a dividend receivable as usable
cash and can affect interest and subsequent sizing. Distributions are not
automatically reinvested; they may be invested upon a later strategy reentry.
Adjusted OHLC plus these explicit distributions/splits is invalid.

## Metrics

- Equity = cash + shares × close. Terminal positions remain open, with no
  hypothetical liquidation fees. Initial capital is the initial drawdown peak.
- Total return = final equity / initial capital - 1.
- CAGR = (final / initial)^(365.25 / elapsed_calendar_days) - 1. Elapsed days are
  last date - first evaluated date + 1, explicitly including the first session.
  Short periods can produce unstable annualized numbers.
- Daily returns start with first-close equity / initial capital - 1. Later returns
  compare successive evaluated closes. Thus the first observation covers only
  entry-open to close, while later observations span close to close.
- Maximum drawdown = maximum(1 - equity / running peak), including entry costs.
- Volatility = sample standard deviation of daily returns × sqrt(252).
- Sharpe = (mean daily return - ((1 + annual risk-free)^(1/252) - 1)) /
  sample daily standard deviation × sqrt(252). Undefined at zero volatility or
  too few observations. Annualization assumes 252 sessions, even for a custom
  calendar. This is a conventional descriptive statistic, not a significance test.
- Average exposure = mean(shares × close / equity); invested-day fraction counts
  evaluated closes with any shares. Neither measures intraday exposure.
- Closed-trade P&L = exit proceeds - entry cost including fee + distributions
  received during that holding. Cash interest is excluded from trade P&L.
- Win rate includes only fully closed trades. Open positions are excluded. Open
  price P&L includes entry costs but excludes open-position dividends, which are
  already in account cash. Do not add total dividends to closed-trade P&L: some
  of those dividends are already included in the closed trades.

## Data and statistical limits

The parser rejects duplicate/unsorted dates, nonfinite numbers, impossible OHLC,
negative volume/distributions and nonpositive split ratios. It does not verify an
exchange calendar, discover omitted events, certify vendor prices, or identify all
data revisions. Positive but wrong data can pass these checks.

One preselected ETF avoids building a universe from today's index constituents,
but choosing that ETF after seeing its success remains selection bias. No
confidence intervals, multiple-testing correction, transaction-cost calibration,
or strategy profitability guarantees are claimed.

# Data contract

UTF-8 CSV, exactly this header, chronological and unique session dates:

```csv
date,open,high,low,close,volume,dividend,split
2020-01-02,100,102,99,101,1000000,0,1
2020-01-03,100,102,99,101,1200000,1,1
2020-01-06,50.5,51,50,50.75,2200000,0,2
```

This tiny example is invented schema documentation, not runnable market history.
At least the largest indicator window plus 41 evaluated sessions is required by
the CLI, with at least 20 in each development and holdout segment.

| Column | Meaning |
|---|---|
| date | Exchange-local session date, YYYY-MM-DD |
| open/high/low/close | Positive, raw/as-traded USD prices; not adjusted close |
| volume | Actual traded shares in that session; nonnegative; not currently used for fills |
| dividend | Cash distribution per post-split share, ex-date, otherwise 0 |
| split | New shares per old share, otherwise **1**, never 0 |

No currency conversion, quoted CSV cells, thousands separators or alternate
headers. Whitespace around values is accepted; blank lines are skipped. An initial
UTF-8 byte order mark is accepted. Missing corporate actions are not inferred.
Include warmup data before the first evaluation date.

## The optional SPY downloader

`tools/fetch_spy.py` uses yfinance's daily history with `auto_adjust=False`,
`back_adjust=False`, `actions=True`, `repair=False`, and `keepna=True`. It downloads
full available history, checks for split events, then restricts the requested dates.

Yahoo OHLC is generally split-adjusted even with dividend adjustment turned off.
Therefore this **SPY-only** importer refuses any returned split event anywhere in
full history. It does not generalize a no-split assumption to arbitrary stocks and
it does not multiply shares on already split-adjusted prices. A verified raw vendor
file can still exercise the C++ engine's full split accounting.

Distributions combine `Dividends` and separately reported `Capital Gains` if
present. If the provider changes conventions, revalidate this mapping. The helper
checks USD metadata, basic OHLC validity, chronological uniqueness and finite
values; these checks cannot certify that the provider has supplied all events.

The metadata JSON records provider, retrieval timestamp, package version, range,
row count and input SHA-256. Keep it with the data. yfinance is an unofficial
data-access package; provider availability, rate limits and terms still apply.
No credential is required by this helper. The end date is exclusive; by default
the sample ends before 2026-01-01, keeping incomplete current sessions out.

Downloader live access was rate-limited during development. No real SPY dataset
is bundled. `examples/demo_run/data_snapshot.csv` is synthetic and must never be
relabeled as downloaded SPY data.

## Before drawing conclusions

Spot-check known distribution dates, a few raw opens/closes, earliest/latest
dates and missing sessions against a second source. Confirm the selected date
range includes warmup. Review provider terms for your use. Keep the exact input
snapshot for every experiment; revised historical data can change results.

#include "engine.hpp"
#include <filesystem>
#include <iostream>
#include <map>
#include <chrono>
#include <locale>

namespace fs=std::filesystem;
using namespace ql;
static std::string escaped(const std::string& text) {
    std::string out;
    for(char c:text) {
        if(c=='&') out+="&amp;"; else if(c=='<') out+="&lt;";
        else if(c=='>') out+="&gt;"; else if(c=='\"') out+="&quot;"; else out+=c;
    }
    return out;
}
static std::string fmt(double value,int precision=2) {
    if(!std::isfinite(value)) return "N/A";
    std::ostringstream out; out<<std::fixed<<std::setprecision(precision)<<value; return out.str();
}
static std::ofstream output(const fs::path& p) {
    std::ofstream out(p); require(bool(out),"Cannot write "+p.string());
    out.exceptions(std::ios::badbit|std::ios::failbit); out<<std::setprecision(17); return out;
}
static std::string fingerprint(const std::vector<Bar>& bars) {
    // Stable FNV-1a of canonical parsed data, identification only, not cryptographic.
    std::ostringstream s; s.imbue(std::locale::classic()); s<<std::setprecision(17);
    for(const auto& b:bars) s<<b.date<<','<<b.open<<','<<b.high<<','<<b.low<<','<<b.close
        <<','<<b.volume<<','<<b.dividend<<','<<b.split<<'\n';
    uint64_t h=14695981039346656037ull;
    for(unsigned char c:s.str()) {h^=c;h*=1099511628211ull;}
    std::ostringstream hex;hex<<std::hex<<h;return hex.str();
}
static void writeSeries(const Result& r,const fs::path& dir) {
    auto curve=output(dir/(name(r.strategy)+"_equity.csv"));
    curve<<"date,equity,cash,shares,daily_return,drawdown,exposure,close_signal_mean,close_signal_z,next_session_long\n";
    for(const auto& p:r.points) curve<<p.date<<','<<p.equity<<','<<p.cash<<','<<p.shares<<','
        <<p.dailyReturn<<','<<p.drawdown<<','<<p.exposure<<','<<p.signalMean<<','<<p.signalZ<<','<<p.nextLong<<'\n';
    auto events=output(dir/(name(r.strategy)+"_events.csv"));
    events<<"date,type,signal_date,reason,quantity,execution_price,commission,slippage_cost,cash_after,shares_after,amount\n";
    for(const auto& e:r.events) events<<e.date<<','<<e.type<<','<<e.signalDate<<','<<e.reason<<','
        <<e.quantity<<','<<e.price<<','<<e.fee<<','<<e.slippage<<','<<e.cash<<','<<e.shares<<','<<e.amount<<'\n';
}
static void svg(std::ostream& out,const std::vector<Result>& results,double initial,bool drawdown) {
    double lo=drawdown?-0.01:initial,hi=drawdown?0:initial;
    for(const auto& r:results) for(const auto& p:r.points) {
        double v=drawdown?p.drawdown:p.equity;lo=std::min(lo,v);hi=std::max(hi,v);
    }
    if(hi-lo<1e-10) hi=lo+1;
    const char* colors[]={"#55dfb5","#f5bd65","#80a9ff"};
    out<<"<svg viewBox='0 0 1000 330' role='img' aria-label='"<<(drawdown?"Drawdown":"Portfolio equity")<<"'>";
    for(int k=0;k<=4;++k) {
        double y=25+k*65.0,v=hi-(hi-lo)*k/4;
        out<<"<line x1='90' x2='980' y1='"<<y<<"' y2='"<<y<<"' stroke='#293549'/>"
           <<"<text x='80' y='"<<y+4<<"' text-anchor='end'>"<<fmt(drawdown?v*100:v,drawdown?1:0)<<(drawdown?"%":"")<<"</text>";
    }
    size_t n=results.front().points.size();
    for(size_t j=0;j<results.size();++j) {
        out<<"<polyline fill='none' stroke='"<<colors[j%3]<<"' stroke-width='1.8' points='";
        // Initial capital/zero drawdown is plotted before first evaluated close.
        auto coord=[&](size_t i,double v) {out<<90+890.0*static_cast<double>(i)/static_cast<double>(n)
            <<','<<25+260*(hi-v)/(hi-lo)<<' ';};
        coord(0,drawdown?0:initial);
        for(size_t i=0;i<n;++i) coord(i+1,drawdown?results[j].points[i].drawdown:results[j].points[i].equity);
        out<<"'/>";
    }
    out<<"<text x='90' y='315'>"<<results.front().points.front().date<<"</text>"
       <<"<text x='980' y='315' text-anchor='end'>"<<results.front().points.back().date<<"</text></svg>";
}
static void report(const std::vector<Result>& results,const Config& c,const fs::path& dir,
                   const std::string& source,const std::string& phase,bool demo) {
    auto out=output(dir/"report.html");
    out<<"<!doctype html><html lang='en'><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
       <<"<title>QuantLab | "<<phase<<"</title><style>body{margin:0;background:#0c1421;color:#e5edf6;font:16px system-ui}"
       <<"main{max-width:1150px;margin:auto;padding:36px 24px}h1{font-size:40px;margin:10px 0}h2{margin-top:36px}"
       <<".muted,small{color:#a5b6cb}.notice{padding:16px;border-left:4px solid #f5bd65;background:#202a39;line-height:1.6}"
       <<"table{width:100%;border-collapse:collapse;font-variant-numeric:tabular-nums}th,td{text-align:right;padding:12px 10px;border-bottom:1px solid #293549}"
       <<"th:first-child,td:first-child{text-align:left}svg{width:100%;background:#121e2f;border-radius:12px}svg text{fill:#a5b6cb;font:12px system-ui}"
       <<".scroll{overflow-x:auto}.legend{display:flex;gap:22px;flex-wrap:wrap;margin:14px 0}a{color:#80a9ff}p,li{line-height:1.65}</style><main>"
       <<"<small>QUANTLAB / DAILY STRATEGY RESEARCH / VERSION 1.0</small><h1>"<<(demo?"Synthetic demonstration":"Historical backtest")
       <<"</h1><p class='muted'>"<<escaped(source)<<" · "<<phase<<" · "<<results[0].points.front().date<<" to "<<results[0].points.back().date<<"</p>"
       <<"<p class='notice'>"<<(demo?"SYNTHETIC DATA — these are invented prices, not SPY performance. Use this report to learn the workflow and verify behavior.":
          "Historical simulation, not evidence of future profits. Data quality, execution assumptions, and repeated testing can materially change conclusions.")<<"</p>"
       <<"<div class='scroll'><table><tr><th>Strategy</th><th>Return</th><th>CAGR</th><th>Max drawdown</th><th>Sharpe</th><th>End equity</th></tr>";
    for(const auto& r:results) {const auto& m=r.metrics;
        out<<"<tr><td>"<<name(r.strategy)<<"</td><td>"<<fmt(m.totalReturn*100)<<"%</td><td>"<<fmt(m.cagr*100)
           <<"%</td><td>"<<fmt(m.maxDrawdown*100)<<"%</td><td>"<<fmt(m.sharpe)<<"</td><td>$"<<fmt(m.finalEquity)<<"</td></tr>";}
    out<<"</table></div><h2>Portfolio value</h2><div class='legend'><span style='color:#55dfb5'>● Trend</span>"
       <<"<span style='color:#f5bd65'>● Mean reversion</span><span style='color:#80a9ff'>● Buy &amp; hold</span></div>";
    svg(out,results,c.initial,false);out<<"<h2>Drawdown from prior peak</h2>";svg(out,results,c.initial,true);
    out<<"<h2>Trading and costs</h2><div class='scroll'><table><tr><th>Strategy</th><th>Fills</th><th>Closed trades</th><th>Win rate*</th><th>Invested days</th><th>Fees + slippage</th><th>Dividends</th></tr>";
    for(const auto& r:results) {const auto& m=r.metrics;
        out<<"<tr><td>"<<name(r.strategy)<<"</td><td>"<<m.fills<<"</td><td>"<<m.closedTrades<<"</td><td>"
           <<(m.closedTrades?fmt(100.0*m.wins/m.closedTrades)+"%":"N/A")<<"</td><td>"<<fmt(m.investedDays*100)
           <<"%</td><td>$"<<fmt(m.commissions+m.slippage)<<"</td><td>$"<<fmt(m.dividends)<<"</td></tr>";}
    out<<"</table></div><p class='muted'>*Closed trades only, including trading costs and dividends attributed to each holding. Open positions remain marked to the final close.</p>"
       <<"<h2>How this run works</h2><ul><li>Signals use completed closing prices; orders fill at the next session's open plus adverse slippage.</li>"
       <<"<li>Trend: "<<c.trendWindow<<"-session simple moving average. Mean reversion: "<<c.meanWindow<<"-session mean, entry z ≤ "<<c.entryZ
       <<", exit at mean recovery or after "<<c.maxHold<<" held sessions.</li><li>Entry allocation: "<<c.allocation*100<<"%; fee: $"<<c.fee
       <<" per fill; slippage: "<<c.slippageBps<<" basis points each way. "<<(c.wholeShares?"Whole-share entries.":"Fractional-share entries.")
       <<"</li><li>Dividends accrue as cash on the ex-date (a payment-date approximation); splits adjust held shares and past signal observations. Dividends are not automatically reinvested.</li>"
       <<"<li>Cash yield: "<<c.cashRate*100<<"% annual; Sharpe risk-free rate: "<<c.riskFree*100<<"% annual. No taxes, borrow, liquidity limits, or market-impact model.</li>"
       <<"<li>Each evaluation period starts independently in cash, with prior data used only for signal warmup.</li></ul>"
       <<"<h2>Audit files</h2><p><a href='summary.csv'>Summary</a> · <a href='run_manifest.txt'>Run settings and data fingerprint</a></p><ul>";
    for(const auto& r:results) out<<"<li>"<<name(r.strategy)<<": <a href='"<<name(r.strategy)<<"_equity.csv'>daily equity</a> · <a href='"
        <<name(r.strategy)<<"_events.csv'>trade and corporate-action ledger</a></li>";
    out<<"</ul><p class='muted'>Question: does trend following reduce drawdowns enough to justify foregone returns and costs? This report measures the trade-off; it does not select a strategy for you.</p></main></html>";
}
static void evaluate(const std::vector<Bar>& bars,const Config& c,size_t begin,size_t end,
                     const fs::path& dir,const std::string& source,const std::string& phase,bool demo,
                     const std::string& holdout) {
    fs::create_directories(dir);
    std::vector<Result> results;
    for(auto strategy:{Strategy::Trend,Strategy::MeanReversion,Strategy::BuyHold}) {
        results.push_back(run(bars,c,strategy,begin,end));writeSeries(results.back(),dir);
    }
    auto summary=output(dir/"summary.csv");
    summary<<"strategy,final_equity,total_return,cagr,max_drawdown,annual_volatility,sharpe,average_exposure,invested_day_fraction,fills,closed_trades,winning_trades,commissions,slippage,dividends,cash_interest,turnover_initial_capital,closed_trade_pnl,open_price_pnl\n";
    std::cout<<"\n"<<phase<<" | "<<bars[begin].date<<" through "<<bars[end-1].date<<"\n"
             <<std::left<<std::setw(18)<<"Strategy"<<std::right<<std::setw(12)<<"Return %"<<std::setw(14)<<"Drawdown %"<<std::setw(10)<<"Sharpe"<<'\n';
    for(const auto& r:results) {const auto& m=r.metrics;
        summary<<name(r.strategy)<<','<<m.finalEquity<<','<<m.totalReturn<<','<<m.cagr<<','<<m.maxDrawdown<<','<<m.volatility<<','
            <<(std::isfinite(m.sharpe)?fmt(m.sharpe,10):"")<<','<<m.exposure<<','<<m.investedDays<<','<<m.fills<<','<<m.closedTrades<<','<<m.wins<<','
            <<m.commissions<<','<<m.slippage<<','<<m.dividends<<','<<m.interest<<','<<m.turnover<<','<<m.closedPnl<<','<<m.unrealizedPricePnl<<'\n';
        std::cout<<std::left<<std::setw(18)<<name(r.strategy)<<std::right<<std::setw(12)<<fmt(m.totalReturn*100)
                 <<std::setw(14)<<fmt(m.maxDrawdown*100)<<std::setw(10)<<fmt(m.sharpe)<<'\n';
    }
    auto manifest=output(dir/"run_manifest.txt");
    manifest<<"QuantLab 1.0\nsource="<<source<<"\nsynthetic="<<demo<<"\nphase="<<phase<<"\nrows="<<bars.size()
        <<"\ncanonical_data_fnv1a64="<<fingerprint(bars)<<"\nfirst_data_date="<<bars.front().date<<"\nlast_data_date="<<bars.back().date
        <<"\nevaluation_start="<<bars[begin].date<<"\nevaluation_end="<<bars[end-1].date<<"\nholdout_start="<<holdout
        <<"\ninitial="<<c.initial<<"\nallocation="<<c.allocation<<"\nfee="<<c.fee<<"\nslippage_bps="<<c.slippageBps
        <<"\ncash_rate="<<c.cashRate<<"\nrisk_free="<<c.riskFree<<"\ntrend_window="<<c.trendWindow<<"\nmean_window="<<c.meanWindow
        <<"\nentry_z="<<c.entryZ<<"\nmax_hold="<<c.maxHold<<"\nwhole_shares="<<c.wholeShares
        <<"\nterminal_liquidation=false\ndividends=ex_date_cash_not_automatically_reinvested\norders=prior_close_signal_next_open\n";
    report(results,c,dir,source,phase,demo);
    std::cout<<"Report: "<<fs::absolute(dir/"report.html").string()<<'\n';
}
static void help() {
    std::cout<<R"(QuantLab 1.0 -- C++ daily ETF research
No arguments: run a clearly labeled synthetic DEVELOPMENT demonstration.

quantlab --data data/SPY.csv --holdout-date 2021-01-01 --phase development
quantlab --data data/SPY.csv --holdout-date 2021-01-01 --phase holdout

Options (all numeric options take a value):
  --demo                  Use synthetic data; cannot combine with --data
  --data PATH             Canonical raw OHLC/action CSV (see docs/DATA.md)
  --out PATH              New output directory (never overwrites an existing one)
  --phase VALUE           development (default), holdout, or both (explicit peek)
  --holdout-date DATE     First holdout date; default: 70% of post-warmup sessions
  --start-date DATE       Fixed evaluation start (use for fair parameter comparisons)
  --capital 100000        Starting capital for EACH independent period
  --allocation 1          Fraction of cash allocated on entry, including fee
  --fee 1                 Fixed commission per executed order
  --slippage-bps 5        Adverse open-price adjustment per side (5 = 0.05%)
  --cash-rate 0           Effective annual yield on cash (0.04 = 4%)
  --risk-free 0           Annual rate used for Sharpe (0.04 = 4%)
  --trend-window 200      Trend SMA length
  --mean-window 20        Mean-reversion rolling mean/stddev length
  --entry-z -2            Mean-reversion entry z-score threshold
  --max-hold 5            Held sessions before next-open exit
  --whole-shares          Round down new positions (splits can create fractions)
  --help                  Show this help

Results: HTML report, summary CSV, daily equity, event ledgers, run manifest.
Data snapshots are copied into the run directory. No live orders or broker link.
)";
}
int main(int argc,char** argv) {
    try {
        Config c;
        std::string data,phase="development",holdout,outPath,startDate;
        bool explicitDemo=false;
        std::map<std::string,double*> doubles={{"--capital",&c.initial},{"--allocation",&c.allocation},
            {"--fee",&c.fee},{"--slippage-bps",&c.slippageBps},{"--cash-rate",&c.cashRate},
            {"--risk-free",&c.riskFree},{"--entry-z",&c.entryZ}};
        for(int i=1;i<argc;++i) {
            std::string key=argv[i];
            if(key=="--help") {help();return 0;}
            if(key=="--demo") {explicitDemo=true;continue;}
            if(key=="--whole-shares") {c.wholeShares=true;continue;}
            require(i+1<argc,"Missing value for "+key);std::string value=argv[++i];
            if(key=="--data") data=value; else if(key=="--out") outPath=value;
            else if(key=="--phase") phase=value; else if(key=="--holdout-date") holdout=value;
            else if(key=="--start-date") startDate=value;
            else if(doubles.count(key)) *doubles[key]=number(value);
            else if(key=="--trend-window" || key=="--mean-window" || key=="--max-hold") {
                double n=number(value);require(n>=1 && n<=1000000 && n==std::floor(n),"Expected positive integer for "+key);
                if(key=="--trend-window") c.trendWindow=static_cast<int>(n);
                else if(key=="--mean-window") c.meanWindow=static_cast<int>(n);else c.maxHold=static_cast<int>(n);
            } else throw std::runtime_error("Unknown option: "+key);
        }
        validate(c);
        require(!(explicitDemo && !data.empty()),"Use --demo OR --data, not both.");
        require(phase=="development" || phase=="holdout" || phase=="both","Invalid --phase.");
        bool demo=data.empty();
        auto bars=demo?demoData():loadCsv(data);
        std::string source=demo?"SYNTHETIC FIXTURE - NOT SPY":fs::absolute(data).string();
        size_t begin=static_cast<size_t>(std::max(c.trendWindow,c.meanWindow));
        if(!startDate.empty()) {
            dateSerial(startDate);
            size_t requested=static_cast<size_t>(std::lower_bound(bars.begin(),bars.end(),startDate,
                [](const Bar& b,const std::string& date){return b.date<date;})-bars.begin());
            require(requested>=begin,"Start date has insufficient warmup for the configured windows.");
            begin=requested;
        }
        require(bars.size()>begin+40,"Need warmup plus at least 41 evaluation sessions.");
        size_t split;
        if(holdout.empty()) {split=begin+(bars.size()-begin)*7/10;holdout=bars[split].date;}
        else {dateSerial(holdout);split=static_cast<size_t>(std::lower_bound(bars.begin(),bars.end(),holdout,
             [](const Bar& b,const std::string& date){return b.date<date;})-bars.begin());}
        require(split>=begin+20 && split+20<=bars.size(),"Require at least 20 sessions in each evaluation period.");
        if(outPath.empty()) {
            auto tick=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            outPath="results/run_"+std::to_string(tick);
        }
        require(!fs::exists(outPath),"Output directory already exists; choose a NEW --out path to preserve the prior run.");
        require(fs::create_directories(outPath),"Cannot create output directory.");
        saveCsv(bars,(fs::path(outPath)/"data_snapshot.csv").string());
        std::cout<<"QuantLab 1.0\n"<<source<<"\nSessions: "<<bars.size()<<" | Holdout begins: "<<bars[split].date<<'\n';
        if(demo) std::cout<<"WARNING: invented data. These results say nothing about SPY profitability.\n";
        if(phase!="development") std::cout<<"HOLDOUT OPENED: subsequent tuning against these results makes this period development data.\n";
        if(phase!="holdout") evaluate(bars,c,begin,split,fs::path(outPath)/"development",source,"development",demo,bars[split].date);
        if(phase!="development") evaluate(bars,c,split,bars.size(),fs::path(outPath)/"holdout",source,"holdout",demo,bars[split].date);
        std::cout<<"\nCompleted. Open report.html in your browser.\n";
        return 0;
    } catch(const std::exception& e) {std::cerr<<"ERROR: "<<e.what()<<'\n';return 1;}
}

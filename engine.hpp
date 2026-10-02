#pragma once
// QuantLab 1.0 -- daily, long-only, single-instrument research engine.
// Standard C++17 only. See docs/METHODOLOGY.md before interpreting results.
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <deque>
#include <fstream>
#include <iomanip>
#include <limits>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace ql {
inline void require(bool ok, const std::string& message) {
    if (!ok) throw std::runtime_error(message);
}
inline std::string trim(std::string s) {
    auto first = s.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    return s.substr(first, s.find_last_not_of(" \t\r\n") - first + 1);
}
inline double number(const std::string& s) {
    size_t used = 0;
    double n = std::stod(s, &used);
    require(used == s.size() && std::isfinite(n), "Invalid finite number: " + s);
    return n;
}
inline int dateSerial(const std::string& s) {
    require(s.size() == 10 && s[4] == '-' && s[7] == '-', "Use YYYY-MM-DD: " + s);
    for (size_t i = 0; i < s.size(); ++i)
        require(i == 4 || i == 7 || (s[i] >= '0' && s[i] <= '9'), "Invalid date: " + s);
    int y = std::stoi(s.substr(0, 4)), m = std::stoi(s.substr(5, 2)), d = std::stoi(s.substr(8, 2));
    const int md[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    require(y >= 1900 && y <= 2200 && m >= 1 && m <= 12, "Invalid date: " + s);
    int limit = md[m-1] + (m == 2 && y%4 == 0 && (y%100 != 0 || y%400 == 0));
    require(d >= 1 && d <= limit, "Invalid date: " + s);
    // Gregorian civil-date conversion, epoch 1970-01-01.
    y -= m <= 2;
    int era = y / 400, yo = y - era * 400;
    int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    return era * 146097 + yo * 365 + yo / 4 - yo / 100 + doy - 719468;
}
struct Bar {
    std::string date;
    double open=0, high=0, low=0, close=0, volume=0, dividend=0, split=1;
};
inline void validate(const std::vector<Bar>& bars) {
    require(!bars.empty(), "Data is empty.");
    int prev = std::numeric_limits<int>::min();
    for (const auto& b : bars) {
        int day = dateSerial(b.date);
        require(day > prev, "Dates must be unique and strictly increasing: " + b.date);
        prev = day;
        for (double v : {b.open,b.high,b.low,b.close,b.volume,b.dividend,b.split})
            require(std::isfinite(v), "Non-finite value at " + b.date);
        require(b.low > 0 && b.open >= b.low && b.close >= b.low &&
                b.high >= b.open && b.high >= b.close, "Invalid OHLC at " + b.date);
        require(b.volume >= 0 && b.dividend >= 0 && b.split > 0, "Invalid action/volume at " + b.date);
    }
}
inline std::vector<Bar> loadCsv(const std::string& path) {
    std::ifstream in(path);
    require(bool(in), "Cannot open CSV: " + path);
    std::string line;
    std::getline(in,line);
    if (line.compare(0,3,"\xEF\xBB\xBF") == 0) line.erase(0,3);
    require(trim(line) == "date,open,high,low,close,volume,dividend,split",
            "CSV header must be: date,open,high,low,close,volume,dividend,split");
    std::vector<Bar> out;
    size_t row = 1;
    while (std::getline(in,line)) {
        ++row;
        if (trim(line).empty()) continue;
        try {
            std::vector<std::string> c;
            std::stringstream ss(line);
            std::string cell;
            while (std::getline(ss,cell,',')) c.push_back(trim(cell));
            require(c.size()==8 && !line.empty() && line.back()!=',', "Expected eight nonempty columns");
            out.push_back({c[0],number(c[1]),number(c[2]),number(c[3]),number(c[4]),
                           number(c[5]),number(c[6]),number(c[7])});
        } catch (const std::exception& e) {
            throw std::runtime_error("CSV row " + std::to_string(row) + ": " + e.what());
        }
    }
    require(in.eof(), "Error reading CSV.");
    validate(out);
    return out;
}
inline void saveCsv(const std::vector<Bar>& bars,const std::string& path) {
    std::ofstream out(path);
    require(bool(out),"Cannot write: "+path);
    out << "date,open,high,low,close,volume,dividend,split\n" << std::setprecision(17);
    for(const auto& b:bars) out << b.date << ',' << b.open << ',' << b.high << ',' << b.low
        << ',' << b.close << ',' << b.volume << ',' << b.dividend << ',' << b.split << '\n';
    require(bool(out),"CSV write failed.");
}
struct Config {
    double initial=100000, allocation=1, fee=1, slippageBps=5, cashRate=0, riskFree=0;
    int trendWindow=200, meanWindow=20, maxHold=5;
    double entryZ=-2;
    bool wholeShares=false;
};
inline void validate(const Config& c) {
    for(double x:{c.initial,c.allocation,c.fee,c.slippageBps,c.cashRate,c.riskFree,c.entryZ})
        require(std::isfinite(x),"Config values must be finite.");
    require(c.initial>0 && c.allocation>0 && c.allocation<=1,"Invalid capital/allocation.");
    require(c.fee>=0 && c.slippageBps>=0 && c.slippageBps<10000,"Invalid trading costs.");
    require(c.cashRate>-1 && c.riskFree>-1,"Annual rates must exceed -1.");
    require(c.trendWindow>=2 && c.meanWindow>=2 && c.maxHold>=1 && c.entryZ<0,"Invalid strategy settings.");
}
enum class Strategy { Trend, MeanReversion, BuyHold };
inline std::string name(Strategy s) {
    return s==Strategy::Trend?"trend":s==Strategy::MeanReversion?"mean_reversion":"buy_hold";
}
struct Point {
    std::string date;
    double equity=0, cash=0, shares=0, dailyReturn=0, drawdown=0, exposure=0, signalMean=0, signalZ=0;
    bool nextLong=false;
};
struct Event {
    std::string date, type, signalDate, reason;
    double quantity=0, price=0, fee=0, slippage=0, cash=0, shares=0, amount=0;
};
struct Metrics {
    double finalEquity=0, totalReturn=0, cagr=0, maxDrawdown=0, volatility=0;
    double sharpe=std::numeric_limits<double>::quiet_NaN();
    double exposure=0, investedDays=0, commissions=0, slippage=0, dividends=0, interest=0, turnover=0;
    int fills=0, closedTrades=0, wins=0;
    double closedPnl=0, unrealizedPricePnl=0;
};
struct Result {
    Strategy strategy;
    std::vector<Point> points;
    std::vector<Event> events;
    Metrics metrics;
};
struct Signals {
    std::deque<double> history;
    double mean=0,z=0;
    bool ready=false;
    void observe(const Bar& b, int window) {
        // Only today's already-effective split is used. Never use future actions.
        for(auto& x:history) x /= b.split;
        history.push_back(b.close);
        while(history.size()>static_cast<size_t>(window)) history.pop_front();
        ready=history.size()==static_cast<size_t>(window);
        mean=std::accumulate(history.begin(),history.end(),0.0)/static_cast<double>(history.size());
        double variance=0;
        for(double x:history) variance+=(x-mean)*(x-mean);
        double sd=std::sqrt(variance/static_cast<double>(history.size()));
        z=sd>1e-12?(b.close-mean)/sd:0;
    }
};
inline Result run(const std::vector<Bar>& bars, const Config& c, Strategy strategy,
                  size_t begin, size_t end) {
    validate(c); validate(bars);
    require(begin>=1 && begin<end && end<=bars.size(),"Invalid evaluation interval.");
    int window=strategy==Strategy::MeanReversion?c.meanWindow:c.trendWindow;
    require(begin>=static_cast<size_t>(window),"Insufficient pre-period warmup.");
    Result result{strategy,{}, {}, {}};
    auto& m=result.metrics;
    Signals signals;
    for(size_t i=0;i<begin;++i) signals.observe(bars[i],window);
    double cash=c.initial, shares=0, basis=0, tradeDividends=0;
    double prevEquity=c.initial, peak=c.initial;
    int holdingDays=0;
    bool pending=strategy==Strategy::BuyHold ||
        (strategy==Strategy::Trend?bars[begin-1].close>signals.mean:signals.z<=c.entryZ);
    std::string signalDate=bars[begin-1].date;
    std::string reason=strategy==Strategy::BuyHold?"benchmark entry":"prior close signal";
    auto event=[&](const Bar& b,const std::string& type,double qty,double price,double fee,
                   double slip,double amount,const std::string& why) {
        result.events.push_back({b.date,type,signalDate,why,qty,price,fee,slip,cash,shares,amount});
    };
    for(size_t i=begin;i<end;++i) {
        const auto& b=bars[i];
        // No accrual before the first entry date; later accrual uses actual calendar gaps.
        if(i>begin && cash>0 && c.cashRate!=0) {
            double interest=cash*(std::pow(1+c.cashRate,
                (dateSerial(b.date)-dateSerial(bars[i-1].date))/365.25)-1);
            cash+=interest; m.interest+=interest;
            event(b,"INTEREST",0,0,0,0,interest,"overnight cash accrual");
        }
        // Corporate actions belong to shares carried from the prior close.
        if(b.split!=1 && shares>0) {
            shares*=b.split;
            event(b,"SPLIT",shares,0,0,0,b.split,"new shares per old share");
        }
        if(b.dividend>0 && shares>0) {
            double income=shares*b.dividend;
            cash+=income; m.dividends+=income; tradeDividends+=income;
            event(b,"DIVIDEND",shares,b.dividend,0,0,income,"ex-date cash approximation");
        }
        double slipRate=c.slippageBps/10000;
        if(pending && shares==0) {
            double price=b.open*(1+slipRate);
            double budget=cash*c.allocation;
            double qty=std::max(0.0,(budget-c.fee)/price);
            if(c.wholeShares) qty=std::floor(qty);
            if(qty>0) {
                double cost=qty*price+c.fee;
                cash-=cost; shares=qty; basis=cost; tradeDividends=0; holdingDays=0;
                double slip=qty*(price-b.open);
                m.commissions+=c.fee; m.slippage+=slip; m.turnover+=qty*b.open; ++m.fills;
                event(b,"BUY",qty,price,c.fee,slip,-cost,reason);
            } else event(b,"REJECT",0,price,0,0,0,"insufficient entry budget");
        } else if(!pending && shares>0) {
            double price=b.open*(1-slipRate),qty=shares;
            double proceeds=qty*price-c.fee;
            // Extreme fees may exceed proceeds. Never silently borrow to pay them.
            require(cash+proceeds>=-1e-8,"Exit fees exceed available equity.");
            cash+=proceeds; shares=0;
            double pnl=proceeds-basis+tradeDividends;
            ++m.closedTrades; if(pnl>0) ++m.wins;
            m.closedPnl+=pnl;
            double slip=qty*(b.open-price);
            m.commissions+=c.fee; m.slippage+=slip; m.turnover+=qty*b.open; ++m.fills;
            event(b,"SELL",qty,price,c.fee,slip,proceeds,reason);
            basis=0; tradeDividends=0; holdingDays=0;
        }
        require(cash>=-1e-7,"Negative cash invariant violated.");
        cash=std::max(0.0,cash);
        double equity=cash+shares*b.close;
        require(std::isfinite(equity) && equity>0,"Non-positive or non-finite equity.");
        peak=std::max(peak,equity);
        double dd=equity/peak-1;
        if(shares>0) ++holdingDays;
        signals.observe(b,window);
        if(strategy==Strategy::BuyHold) {pending=true; reason="benchmark entry";}
        else if(strategy==Strategy::Trend) {
            pending=b.close>signals.mean; reason=pending?"close above SMA":"close at or below SMA";
        } else if(shares>0) {
            pending=b.close<signals.mean && holdingDays<c.maxHold;
            reason=holdingDays>=c.maxHold?"maximum holding period":"close recovered to mean";
        } else {pending=signals.z<=c.entryZ; reason="close z-score below entry threshold";}
        result.points.push_back({b.date,equity,cash,shares,equity/prevEquity-1,dd,
                                  shares*b.close/equity,signals.mean,signals.z,pending});
        prevEquity=equity; signalDate=b.date;
        m.maxDrawdown=std::max(m.maxDrawdown,-dd);
        m.exposure+=shares*b.close/equity;
        m.investedDays+=shares>0?1:0;
    }
    double n=static_cast<double>(result.points.size());
    m.finalEquity=prevEquity; m.totalReturn=prevEquity/c.initial-1;
    // Include the first evaluated session in calendar elapsed time.
    double years=(dateSerial(bars[end-1].date)-dateSerial(bars[begin].date)+1)/365.25;
    m.cagr=std::pow(prevEquity/c.initial,1/years)-1;
    m.exposure/=n; m.investedDays/=n; m.turnover/=c.initial;
    m.unrealizedPricePnl=shares>0?shares*bars[end-1].close-basis:0;
    double avg=0,var=0;
    for(const auto& p:result.points) avg+=p.dailyReturn/n;
    for(const auto& p:result.points) var+=(p.dailyReturn-avg)*(p.dailyReturn-avg);
    double sd=n>1?std::sqrt(var/(n-1)):0;
    m.volatility=sd*std::sqrt(252.0);
    if(sd>1e-14) m.sharpe=(avg-(std::pow(1+c.riskFree,1/252.0)-1))/sd*std::sqrt(252.0);
    return result;
}
// Deterministic synthetic fixture: weekday sessions, recurring dividends, one split.
// This intentionally is NOT market history and does not use an exchange holiday calendar.
inline std::vector<Bar> demoData() {
    std::vector<Bar> bars;
    uint32_t state=1729;
    auto uniform=[&]() {state=1664525u*state+1013904223u; return (state+0.5)/4294967296.0;};
    double previous=100;
    for(int y=2005;y<=2025;++y) for(int mo=1;mo<=12;++mo) for(int day=1;day<=31;++day) {
        std::ostringstream date;
        date<<y<<'-'<<std::setw(2)<<std::setfill('0')<<mo<<'-'<<std::setw(2)<<day;
        int serial;
        try {serial=dateSerial(date.str());} catch(...) {continue;}
        int weekday=(serial+4)%7;
        if(weekday==0 || weekday==6) continue;
        size_t k=bars.size();
        double split=k==2500?2:1;
        previous/=split;
        double dividend=(k>0 && k%63==0)?previous*0.003:0;
        double regime=(k/400)%3==0?-0.0007:0.00055;
        double shock=(uniform()+uniform()+uniform()+uniform()-2)*0.019;
        double open=std::max(1.0,(previous-dividend)*std::exp((uniform()-0.5)*0.01));
        double close=open*std::exp(regime+shock);
        double high=std::max(open,close)*(1+uniform()*0.008);
        double low=std::min(open,close)*(1-uniform()*0.008);
        bars.push_back({date.str(),open,high,low,close,10000000+uniform()*10000000,dividend,split});
        previous=close;
    }
    validate(bars); return bars;
}
} // namespace ql

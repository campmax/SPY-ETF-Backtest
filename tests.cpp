#include "../src/engine.hpp"
#include <functional>
#include <iostream>
using namespace ql;
static Bar bar(int d,double o,double c,double dividend=0,double split=1) {
    std::ostringstream date;date<<"2020-01-"<<std::setw(2)<<std::setfill('0')<<d;
    return {date.str(),o,std::max(o,c),std::min(o,c),c,1000000,dividend,split};
}
static Config cfg() {Config c;c.trendWindow=2;c.meanWindow=2;c.initial=1000;c.fee=0;c.slippageBps=0;return c;}
static void near(double a,double b) {require(std::abs(a-b)<=1e-8*std::max(1.0,std::abs(b)),"Expected "+std::to_string(b)+", got "+std::to_string(a));}
static void rejects(const std::function<void()>& f) {bool thrown=false;try{f();}catch(const std::exception&){thrown=true;}require(thrown,"Expected rejection");}
int main() {
    int passed=0,failed=0;
    auto test=[&](const char* title,const std::function<void()>& f) {
        try {f();++passed;std::cout<<"PASS "<<title<<'\n';}
        catch(const std::exception& e){++failed;std::cerr<<"FAIL "<<title<<": "<<e.what()<<'\n';}
    };
    test("prior close signal executes at next open",[] {
        std::vector<Bar> b={bar(1,10,10),bar(2,12,12),bar(3,100,8),bar(4,9,9)};
        auto r=run(b,cfg(),Strategy::Trend,2,4);
        near(r.events[0].price,100);near(r.events[0].quantity,10);
        require(r.events[0].signalDate=="2020-01-02","Wrong signal date");
        near(r.events[1].price,9);near(r.metrics.finalEquity,90);near(r.metrics.maxDrawdown,0.92);
    });
    test("split preserves equity and doubles held shares",[] {
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,100,100),bar(4,50,50,0,2)},cfg(),Strategy::BuyHold,2,4);
        near(r.points.back().shares,20);near(r.metrics.finalEquity,1000);near(r.metrics.totalReturn,0);
    });
    test("dividend compensates ex-date price drop once",[] {
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,100,100),bar(4,90,90,10)},cfg(),Strategy::BuyHold,2,4);
        near(r.metrics.finalEquity,1000);near(r.metrics.dividends,100);near(r.points.back().cash,100);
        require(r.metrics.fills==1,"Dividends must not trigger automatic reinvestment");
    });
    test("ex-date buyer receives no dividend",[] {
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,90,90,10)},cfg(),Strategy::BuyHold,2,3);
        near(r.metrics.dividends,0);near(r.metrics.finalEquity,1000);
    });
    test("simultaneous split and dividend use post-split shares",[] {
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,100,100),bar(4,49,49,1,2)},cfg(),Strategy::BuyHold,2,4);
        near(r.metrics.dividends,20);near(r.metrics.finalEquity,1000);
    });
    test("adverse slippage and commissions both directions",[] {
        auto c=cfg();c.fee=2;c.slippageBps=100;
        auto r=run({bar(1,90,90),bar(2,100,100),bar(3,100,80),bar(4,100,100)},c,Strategy::Trend,2,4);
        double qty=998.0/101;
        near(r.events[0].quantity,qty);near(r.events[1].price,99);
        near(r.metrics.finalEquity,qty*99-2);near(r.metrics.commissions,4);near(r.metrics.slippage,qty*2);
        near(r.metrics.closedPnl,r.metrics.finalEquity-1000);
    });
    test("future price mutations do not alter prior decisions",[] {
        std::vector<Bar> a={bar(1,90,90),bar(2,100,100),bar(3,100,110),bar(4,120,120),bar(5,110,110)};
        auto b=a;b[4]=bar(5,500,5,2,2);
        auto x=run(a,cfg(),Strategy::Trend,2,5),y=run(b,cfg(),Strategy::Trend,2,5);
        for(size_t i=0;i<2;++i) {near(x.points[i].equity,y.points[i].equity);require(x.points[i].nextLong==y.points[i].nextLong,"Future leaked");}
        near(x.events[0].price,y.events[0].price);
    });
    test("mean reversion exits after exact held-session count",[] {
        auto c=cfg();c.meanWindow=3;c.maxHold=2;c.entryZ=-1;
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,80,80),bar(4,75,75),bar(5,74,74),bar(6,73,73)},c,Strategy::MeanReversion,3,6);
        require(r.events.size()==2 && r.events[0].date=="2020-01-04" && r.events[1].date=="2020-01-06","Holding-period error");
    });
    test("mean recovery exits at following open",[] {
        auto c=cfg();c.meanWindow=3;c.entryZ=-1;
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,80,80),bar(4,75,110),bar(5,105,105)},c,Strategy::MeanReversion,3,5);
        require(r.events.size()==2 && r.events[1].date=="2020-01-05","Recovery exit error");near(r.events[1].price,105);
    });
    test("flat series has zero volatility and undefined Sharpe",[] {
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,100,100),bar(4,100,100)},cfg(),Strategy::BuyHold,2,4);
        near(r.metrics.volatility,0);require(std::isnan(r.metrics.sharpe),"Sharpe should be undefined");
    });
    test("invalid OHLC dates and non-finite inputs reject",[] {
        rejects([]{validate(std::vector<Bar>{bar(1,10,10),bar(1,11,11)});});
        rejects([]{auto b=bar(1,10,10);b.low=20;validate(std::vector<Bar>{b});});
        rejects([]{dateSerial("2021-02-29");});
        rejects([]{dateSerial("2020-01-1x");});
        rejects([]{number("nan");});rejects([]{number("1junk");});
        rejects([]{auto c=cfg();c.slippageBps=10000;validate(c);});
        require(dateSerial("2020-03-01")-dateSerial("2020-02-28")==2,"Leap year failed");
    });
    test("whole-share budget includes commission",[] {
        auto c=cfg();c.wholeShares=true;c.fee=1;
        auto r=run({bar(1,333,333),bar(2,333,333),bar(3,333,333)},c,Strategy::BuyHold,2,3);
        near(r.points[0].shares,3);near(r.points[0].cash,0);near(r.metrics.finalEquity,999);
    });
    test("insufficient whole-share budget records rejection",[] {
        auto c=cfg();c.wholeShares=true;c.initial=50;
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,100,100)},c,Strategy::BuyHold,2,3);
        require(r.events[0].type=="REJECT","Missing rejected order");near(r.metrics.finalEquity,50);
    });
    test("cash interest uses actual calendar gap",[] {
        auto c=cfg();c.cashRate=0.1;
        auto r=run({bar(3,100,100),bar(4,100,100),bar(5,100,100),bar(8,100,100)},c,Strategy::Trend,2,4);
        near(r.metrics.finalEquity,1000*std::pow(1.1,3/365.25));
    });
    test("open positions mark to final close without forced sell",[] {
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,100,120)},cfg(),Strategy::BuyHold,2,3);
        near(r.metrics.finalEquity,1200);require(r.metrics.fills==1 && r.metrics.closedTrades==0,"Unexpected liquidation");
        near(r.metrics.unrealizedPricePnl,200);
    });
    test("signal history handles splits causally",[] {
        Signals s;s.observe(bar(1,100,100),2);s.observe(bar(2,50,50,0,2),2);
        near(s.mean,50);near(s.z,0);
    });
    test("entry costs count in initial drawdown",[] {
        auto c=cfg();c.fee=10;
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,100,100)},c,Strategy::BuyHold,2,3);
        near(r.metrics.maxDrawdown,0.01);
    });
    test("holdout starts independently but uses past warmup",[] {
        std::vector<Bar> b={bar(1,100,100),bar(2,110,110),bar(3,120,120),bar(4,130,130),bar(5,140,140)};
        auto r=run(b,cfg(),Strategy::Trend,4,5);
        near(r.events[0].quantity,1000.0/140);require(r.events[0].signalDate=="2020-01-04","Warmup not used");
    });
    test("reverse split preserves portfolio value",[] {
        auto r=run({bar(1,100,100),bar(2,100,100),bar(3,100,100),bar(4,1000,1000,0,0.1)},cfg(),Strategy::BuyHold,2,4);
        near(r.points.back().shares,1);near(r.metrics.finalEquity,1000);
    });
    test("synthetic series maintains accounting invariants",[] {
        auto b=demoData();Config c;
        for(auto s:{Strategy::Trend,Strategy::MeanReversion,Strategy::BuyHold}) {
            auto r=run(b,c,s,200,b.size());
            for(size_t i=0;i<r.points.size();++i) {
                const auto& p=r.points[i];near(p.equity,p.cash+p.shares*b[200+i].close);
                require(p.cash>=0 && p.shares>=0 && p.drawdown<=1e-12,"Account invariant failed");
            }
        }
    });
    std::cout<<passed<<" passed; "<<failed<<" failed\n";return failed?1:0;
}

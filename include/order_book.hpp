#include "order.hpp"
#include<deque>
#include<map>
#include<set>

class OrderBook{
    protected:
        std::map<int, std::deque<BuyOrder>> buyOrderList;
        std::set<int> buyPriceList;
        std::map<int, std::deque<SellOrder>> sellOrderList;
        std::set<int> sellPriceList;
    
    public:
        void add(BuyOrder buyOrder){
            buyOrderList[buyOrder.getIdealPrice()].push_back(buyOrder);
            buyPriceList.insert(buyOrder.getIdealPrice());
        }
        void add(SellOrder sellOrder){
            sellOrderList[sellOrder.getIdealPrice()].push_back(sellOrder);
            sellPriceList.insert(sellOrder.getIdealPrice());
        }
};
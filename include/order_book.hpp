#include "order.hpp"
#include<deque>
#include<map>
#include<set>
#include<vector>

class OrderBook{
    protected:
        std::map<int, std::deque<BuyOrder>> buyOrderList;
        std::set<int> buyPriceList;
        std::map<int, std::deque<SellOrder>> sellOrderList;
        std::set<int> sellPriceList;
        std::set<int> canceledID;
        std::vector<Trade> tradeHistory;
        int transactionID = 0;

        void add_transaction(){
            transactionID++;
        }
    
    public:
        OrderBook(){
            buyOrderList.clear();
            buyPriceList.clear();
            sellOrderList.clear();
            sellPriceList.clear();
            canceledID.clear();
            tradeHistory.clear();
            transactionID = 0;
        }

        friend std::ostream& operator<<(std::ostream& os, const OrderBook& p){
            os << "Order Book: \n";
            os << "Buy Orders: \n";
            for(auto& [price, orders]: p.buyOrderList){
                for(const BuyOrder& order: orders){
                    if(order.getStockNumber() == 0) continue;
                    std::set<int>::iterator itr = p.canceledID.find(order.getID());
                    if(itr != p.canceledID.end()) continue;
                    os << order << '\n';
                }
            }
            os << "Sell Orders: \n";
            for(auto& [price, orders]: p.sellOrderList){
                for(const SellOrder& order: orders){
                    if(order.getStockNumber() == 0) continue;
                    std::set<int>::iterator itr = p.canceledID.find(order.getID());
                    if(itr != p.canceledID.end()) continue;
                    os << order << '\n';
                }
            }
            os << "Trade History: \n";
            for(auto& trade: p.tradeHistory){
                os << trade << '\n';
            }
            return os;
        }

        BuyOrder* initiateBuyOrder(int idealPrice, int stockNumber){
            add_transaction();

            BuyOrder buyOrder(transactionID, idealPrice, stockNumber);

            buyOrderList[buyOrder.getIdealPrice()].push_back(buyOrder);
            buyPriceList.insert(buyOrder.getIdealPrice());

            return &buyOrderList[buyOrder.getIdealPrice()].back();
        }

        SellOrder* initiateSellOrder(int idealPrice, int stockNumber){
            add_transaction();

            SellOrder sellOrder(transactionID, idealPrice, stockNumber);
            
            sellOrderList[sellOrder.getIdealPrice()].push_back(sellOrder);
            sellPriceList.insert(sellOrder.getIdealPrice());

            return &sellOrderList[sellOrder.getIdealPrice()].back();
        }

        Trade initiateTrade(int sellID, int buyID, int price, int stockNumber){
            add_transaction();
            Trade trade(transactionID, price, stockNumber, sellID, buyID);
            tradeHistory.push_back(trade);

            return trade;
        }

        void cancel(int id){
            canceledID.insert(id);
        }

        int getHighestPrice(){
            if(buyPriceList.empty()) return -1;
            return *buyPriceList.rbegin();
        }

        int lowestSellPrice(){
            if(sellPriceList.empty()) return INT_MAX;
            return *sellPriceList.begin();
        }

        BuyOrder* getBestBuyOrder(int SellOrder){
            while(true){
                int bestPrice = getHighestPrice();
                if(bestPrice < SellOrder) return nullptr;
                if(buyOrderList[bestPrice].empty()){
                    buyPriceList.erase(bestPrice);
                    continue;
                }

                BuyOrder* currentBuyOrder = &buyOrderList[bestPrice].front();

                if(currentBuyOrder->getStockNumber() == 0){
                    buyOrderList[bestPrice].pop_front();
                    continue;
                }

                std::set<int>::iterator itr = canceledID.find(currentBuyOrder->getID());

                if(itr != canceledID.end()){
                    buyOrderList[bestPrice].pop_front();
                    canceledID.erase(itr);
                    continue;
                }

                return currentBuyOrder;
            }
            return nullptr;
        }

        SellOrder* getBestSellOrder(int BuyOrder){
            while(true){
                int bestPrice = lowestSellPrice();
                if(bestPrice > BuyOrder) return nullptr;
                if(sellOrderList[bestPrice].empty()){
                    sellPriceList.erase(bestPrice);
                    continue;
                }

                SellOrder* currentSellOrder = &sellOrderList[bestPrice].front();

                if(currentSellOrder->getStockNumber() == 0){
                    sellOrderList[bestPrice].pop_front();
                    continue;
                }

                std::set<int>::iterator itr = canceledID.find(currentSellOrder->getID());

                if(itr != canceledID.end()){
                    sellOrderList[bestPrice].pop_front();
                    canceledID.erase(itr);
                    continue;
                }

                return currentSellOrder;
            }
            return nullptr;
        }

        
};
#pragma once

#include "order.hpp"
#include <deque>
#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <unordered_set>
#include <vector>

class OrderBook {
    protected:
        std::map<int, std::deque<BuyOrder>> buyOrderList;
        std::set<int> buyPriceList;
        std::map<int, std::deque<SellOrder>> sellOrderList;
        std::set<int> sellPriceList;
        std::set<int> canceledID;
        std::vector<Trade> tradeHistory;
        std::set<int> activeID;
        int transactionID = 0;

        void add_transaction();

    private:
        std::unordered_set<int> callerOrderIDs;

        bool reserveCallerOrderID(int id);
        BuyOrder* insertBuyOrder(int id, int idealPrice, int stockNumber);
        SellOrder* insertSellOrder(int id, int idealPrice, int stockNumber);

    public:
        struct PriceLevel {
            int price;
            std::int64_t quantity;
        };

        struct Depth {
            std::vector<PriceLevel> buys;
            std::vector<PriceLevel> sells;
        };

        OrderBook();

        friend std::ostream& operator<<(std::ostream& os, const OrderBook& p);

        BuyOrder* initiateBuyOrder(int idealPrice, int stockNumber);
        BuyOrder* initiateBuyOrder(int id, int idealPrice, int stockNumber);

        SellOrder* initiateSellOrder(int idealPrice, int stockNumber);
        SellOrder* initiateSellOrder(int id, int idealPrice, int stockNumber);

        Trade initiateTrade(int sellID, int buyID, int price, int stockNumber);

        const std::vector<Trade>& getTradeHistory() const;

        Depth getDepth(std::size_t limit) const;

        void executeTrade(BuyOrder& buy, SellOrder& sell, int price, int quantity);

        bool cancel(int id);

        int getHighestPrice();

        int lowestSellPrice();

        BuyOrder* getBestBuyOrder(int SellOrder);

        SellOrder* getBestSellOrder(int BuyOrder);
};

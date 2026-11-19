#pragma once

#include "order_book.hpp"
#include <optional>

class matching_engine {
    protected:
        OrderBook orderBook;
    public:
        struct OrderReceipt {
            int id;
            int filled;
            int open;
        };

        matching_engine();
        bool cancel(int id);

        void receiveBuyOrder(int idealPrice, int stockNumber);

        std::optional<OrderReceipt> submitBuyOrder(int idealPrice, int stockNumber);
        std::optional<OrderReceipt> submitBuyOrder(int id, int idealPrice, int stockNumber);

        void receiveSellOrder(int idealPrice, int stockNumber);

        std::optional<OrderReceipt> submitSellOrder(int idealPrice, int stockNumber);
        std::optional<OrderReceipt> submitSellOrder(int id, int idealPrice, int stockNumber);

        OrderBook::Depth getDepth(std::size_t limit) const;

        BuyOrder* getBestBuyOrder(int SellOrder);

        SellOrder* getBestSellOrder(int BuyOrder);

        const std::vector<Trade>& getTradeHistory() const;

        void receiveBuyOrder(BuyOrder* buyOrder);

        void receiveSellOrder(SellOrder* sellOrder);

        void print();

    private:
        std::optional<OrderReceipt> matchAndGetReceipt(BuyOrder* order);
        std::optional<OrderReceipt> matchAndGetReceipt(SellOrder* order);
};

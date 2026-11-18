#include "order_book.hpp"

class matching_engine {
    protected:
        OrderBook orderBook;
    public:
        matching_engine(){
            orderBook = OrderBook();
        }
        void cancel(int id){
            orderBook.cancel(id);
        }

        void receiveBuyOrder(int idealPrice, int stockNumber){
            BuyOrder* order = orderBook.initiateBuyOrder(idealPrice, stockNumber);
            receiveBuyOrder(order);
        }

        void receiveSellOrder(int idealPrice, int stockNumber){
            SellOrder* order = orderBook.initiateSellOrder(idealPrice, stockNumber);
            receiveSellOrder(order);
        }

        BuyOrder* getBestBuyOrder(int SellOrder){
            return orderBook.getBestBuyOrder(SellOrder);
        }

        SellOrder* getBestSellOrder(int BuyOrder){
            return orderBook.getBestSellOrder(BuyOrder);
        }

        void receiveBuyOrder(BuyOrder* buyOrder){
            SellOrder* bestSellOrder = orderBook.getBestSellOrder(buyOrder->getIdealPrice());

            while(bestSellOrder != nullptr){
                int tradeNumber = bestSellOrder->match(buyOrder->getIdealPrice(), buyOrder->getStockNumber());
                
                if(tradeNumber == 0) break;

                buyOrder->applyTrade(tradeNumber);
                bestSellOrder->applyTrade(tradeNumber);
                orderBook.initiateTrade(bestSellOrder->getID(), buyOrder->getID(), bestSellOrder->getIdealPrice(), tradeNumber);
            }
        }

        void receiveSellOrder(SellOrder* sellOrder){
            BuyOrder* bestBuyOrder = orderBook.getBestBuyOrder(sellOrder->getIdealPrice());

            while(bestBuyOrder != nullptr){
                int tradeNumber = bestBuyOrder->match(sellOrder->getIdealPrice(), sellOrder->getStockNumber());

                if(tradeNumber == 0) break;
                
                sellOrder->applyTrade(tradeNumber);
                bestBuyOrder->applyTrade(tradeNumber);
                orderBook.initiateTrade(bestBuyOrder->getID(), sellOrder->getID(), bestBuyOrder->getIdealPrice(), tradeNumber);
            }
        }

        void print(){
            std::cout << orderBook << std::endl;
        }
};

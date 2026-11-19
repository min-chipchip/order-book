#include "order_book.hpp"

class matching_engine {
    protected:
        OrderBook orderBook;
    public:
        matching_engine(){
            orderBook = OrderBook();
        }
        bool cancel(int id){
            return orderBook.cancel(id);
        }

        void receiveBuyOrder(int idealPrice, int stockNumber){
            BuyOrder* order = orderBook.initiateBuyOrder(idealPrice, stockNumber);
            if(order != nullptr) receiveBuyOrder(order);
        }

        void receiveSellOrder(int idealPrice, int stockNumber){
            SellOrder* order = orderBook.initiateSellOrder(idealPrice, stockNumber);
            if(order != nullptr) receiveSellOrder(order);
        }

        BuyOrder* getBestBuyOrder(int SellOrder){
            return orderBook.getBestBuyOrder(SellOrder);
        }

        SellOrder* getBestSellOrder(int BuyOrder){
            return orderBook.getBestSellOrder(BuyOrder);
        }

        const std::vector<Trade>& getTradeHistory() const{
            return orderBook.getTradeHistory();
        }

        void receiveBuyOrder(BuyOrder* buyOrder){
            while(buyOrder->getStockNumber() > 0){
                SellOrder* bestSellOrder = orderBook.getBestSellOrder(buyOrder->getIdealPrice());
                if(bestSellOrder == nullptr) break;
                int tradeNumber = bestSellOrder->match(buyOrder->getIdealPrice(), buyOrder->getStockNumber());
                
                if(tradeNumber == 0) break;

                orderBook.executeTrade(*buyOrder, *bestSellOrder, bestSellOrder->getIdealPrice(), tradeNumber);
            }
        }

        void receiveSellOrder(SellOrder* sellOrder){
            while(sellOrder->getStockNumber() > 0){
                BuyOrder* bestBuyOrder = orderBook.getBestBuyOrder(sellOrder->getIdealPrice());
                if(bestBuyOrder == nullptr) break;
                int tradeNumber = bestBuyOrder->match(sellOrder->getIdealPrice(), sellOrder->getStockNumber());

                if(tradeNumber == 0) break;
                
                orderBook.executeTrade(*bestBuyOrder, *sellOrder, bestBuyOrder->getIdealPrice(), tradeNumber);
            }
        }

        void print(){
            std::cout << orderBook << std::endl;
        }
};

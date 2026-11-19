#include <algorithm>
#include <iostream>

class Order{
    protected:
        int idealPrice = 0; int stockNumber = 0;
        int id;

    public:

        virtual int match(int, int) = 0;

        friend std::ostream& operator<<(std::ostream& os, const Order& p){
            os << "Order ID: " << p.id << ", Ideal Price: " << p.idealPrice
            << ", Stock Number: " << p.stockNumber;
            return os;
        }

        Order(){
            this->id = this->idealPrice = this->stockNumber = 0;
        }
        
        Order(int id, int idealPrice, int stockNumber){
            this->id = id;
            this->idealPrice = idealPrice;
            this->stockNumber = stockNumber;
        };

        virtual ~Order() = default;

        int getID() const{
            return id;
        };

        int getIdealPrice() const{
            return idealPrice;
        };

        int getStockNumber() const{
            return stockNumber;
        };

        bool applyTrade(int stockNumber){
            this->stockNumber -= stockNumber;
            if(this->stockNumber == 0)
                return true;
            return false;
        };
    
};

class SellOrder: public Order{
    public:
        int match(int buyPrice, int stockNumber) override{
            if(buyPrice < this->idealPrice) return 0;
            int tradeNumber = std::min(this->stockNumber, stockNumber);
            return tradeNumber;
        }
        SellOrder(): Order(){}
        SellOrder(int id, int idealPrice, int stockNumber): Order(id, idealPrice, stockNumber){};
        friend std::ostream& operator<<(std::ostream& os, const SellOrder& p){
            os << "Sell Order ID: " << p.id << ", Ideal Price: " << p.idealPrice << ", Stock Number: " << p.stockNumber;
            return os;
        }
};

class BuyOrder: public Order{
    public:
        int match(int sellPrice, int stockNumber) override{
            if(sellPrice > this->idealPrice) return 0;
            int tradeNumber = std::min(this->stockNumber, stockNumber);
            return tradeNumber;
        };  
        BuyOrder(): Order(){}
        BuyOrder(int id, int idealPrice, int stockNumber): Order(id, idealPrice, stockNumber){};
        
        friend std::ostream& operator<<(std::ostream& os, const BuyOrder& p){
            os << "Buy Order ID: " << p.id << ", Ideal Price: " << p.idealPrice << ", Stock Number: " << p.stockNumber;
            return os;
        }
};

class Trade{
    protected:
        int id;
        int sellID = 0;
        int buyID = 0;
        int price = 0;
        int stockNumber = 0;
    public:
        Trade(){
            sellID = buyID = price = stockNumber = 0;
        }
        Trade(int id, int price, int stockNumber, int sellID, int buyID){
            this->id = id;
            this->price = price;
            this->stockNumber = stockNumber;
            this->sellID = sellID;
            this->buyID = buyID;
        };
        int getID() const{
            return id;
        };
        int getSellID() const{
            return sellID;
        };
        int getBuyID() const{
            return buyID;
        };
        int getPrice() const{
            return price;
        };
        int getStockNumber() const{
            return stockNumber;
        };
        friend std::ostream& operator<<(std::ostream& os, const Trade& p){
            os << "Trade Order ID: " << p.id << ", Ideal Price: " << p.price << ", Stock Number: " << p.stockNumber;
            return os;
        }
};

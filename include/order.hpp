#include <algorithm>
#include <iostream>

class Order{
    protected:
        int idealPrice = 0; int stockNumber = 0;
        
        virtual int match(int, int){
            return 0;
        }
        bool isFinished(int stockNumber){
            this->stockNumber -= stockNumber;
            if(this->stockNumber == 0)
                return true;
            return false;
        };

    public:
        int id;

        friend std::ostream& operator<<(std::ostream& os, const Order& p){
            os << "Order ID: " << p.id << ", Ideal Price: " << p.idealPrice
            << ", Stock Number: " << p.stockNumber;
            return os;
        }
        
        Order(int id, int idealPrice, int stockNumber){
            this->id = id;
            this->idealPrice = idealPrice;
            this->stockNumber = stockNumber;
        };

        int getID(){
            return id;
        };
};

class Sell: public Order{
    protected:
        int match(int buyPrice, int stockNumber) override{
            if(buyPrice < this->idealPrice) return 0;
            int tradeNumber = std::min(this->stockNumber, stockNumber);
            return tradeNumber;
        }

    public:
        Sell(int id, int idealPrice, int stockNumber): Order(id, idealPrice, stockNumber){};
        friend std::ostream& operator<<(std::ostream& os, const Sell& p){
            os << "Sell Order ID: " << p.id << ", Ideal Price: " << p.idealPrice << ", Stock Number: " << p.stockNumber;
            return os;
        }
};

class Buy: public Order{
    protected:
        int match(int sellPrice, int stockNumber) override{
            if(sellPrice > this->idealPrice) return 0;
            int tradeNumber = std::min(this->stockNumber, stockNumber);
            return tradeNumber;
        };    
    
    public:
        Buy(int id, int idealPrice, int stockNumber): Order(id, idealPrice, stockNumber){};
        
        friend std::ostream& operator<<(std::ostream& os, const Buy& p){
            os << "Buy Order ID: " << p.id << ", Ideal Price: " << p.idealPrice << ", Stock Number: " << p.stockNumber;
            return os;
        }
};

#include <iostream>
#include <string>
#include "../include/order.hpp"
#include<memory>

int main(){
    int operation = 0;
    std::cin >> operation;
    

    for(int id = 1; id <= operation; ++id){
        std::string type;
        int idealPrice, stockNumber;
        std::cin >> type >> idealPrice >> stockNumber;

        if(type != "buy" && type != "sell"){
            std::cerr << "Invalid order type: " << type << std::endl;
            continue;
        }

        if(type == "buy"){
            Buy order(id, idealPrice, stockNumber);
            std::cout << order << '\n';
        } else if(type == "sell"){
            Sell order(id, idealPrice, stockNumber);
            std::cout << order << '\n';
        }
    }
}

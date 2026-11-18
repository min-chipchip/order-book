#include <iostream>
#include <string>
#include "matching_engine.hpp"

int main(){
    int operation = 0;
    std::cin >> operation;

    matching_engine engine;

    for(int id = 1; id <= operation; ++id){
        std::string type;
        int idealPrice, stockNumber;
        std::cin >> type >> idealPrice >> stockNumber;

        if(type != "buy" && type != "sell"){
            std::cerr << "Invalid order type: " << type << std::endl;
            continue;
        }

        if(type == "buy"){
            engine.receiveBuyOrder(idealPrice, stockNumber);
        } else if(type == "sell"){
            engine.receiveSellOrder(idealPrice, stockNumber);
        }
    }

    engine.print();
}

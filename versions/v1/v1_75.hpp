#pragma once // Prevents multi-definition errors in future

#include <iostream>
#include <string>

#include <map>
#include <unordered_map>
#include <vector>

#include <fstream>
#include <sstream>

#include "../../utils/Logging.hpp"
#include "../bookObject.hpp"

class LOBV1_75 : public book{
    private:
        std::map<int, std::vector<Order>> buy;
        std::map<int, std::vector<Order>> sell;
        
        std::unordered_map<int, Detail> ID_price_book;
        
        int BinarySearch(const std::vector<Order>* priceQueue, double time){
            if (priceQueue->empty() || priceQueue == nullptr) {
                return -1;
            }
            
            int left = 0;
            int right = priceQueue->size() - 1;
            
            while(left <= right){
                
                int mid = left + (right - left)/2;
                double midTime = priceQueue->at(mid).time;
                
                if (midTime == time)                        {return mid;}
                    else if (midTime < time)                {left = mid + 1;}
                    else                                    {right = mid - 1;}
                }
                
                return -1;
            }

        
        int executeHelper(std::map<int, std::vector<Order>>& map, int direction, Order& ord){
            auto &[insideBookPrice, orders] = direction == 1 ? *map.begin() : *map.rbegin();          // Assign the pointer based on if you are checking sells or buys                   
                        
            while(!orders.empty()) {
                Order &nextOrder = orders.front();

                if(nextOrder.size < ord.size)       {LOG("Consume order " << nextOrder.OrderID << " of price " << insideBookPrice << ", count " << nextOrder.size << "\n"); 
                                                        ord.size -= nextOrder.size;
                                                        ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin());}
                else if(nextOrder.size == ord.size) {LOG("Consume order " << nextOrder.OrderID << " of price " << insideBookPrice << ", count " << nextOrder.size << "\n"); 
                                                        ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin()); 
                                                        if(direction == 1){map.erase(map.begin());} else{map.erase(std::prev(map.end()));}
                                                        LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n"); return 2;}
                else                                {LOG("Consume " << ord.size << " from order " <<  nextOrder.OrderID << " of price " << insideBookPrice << "\n"); 
                                                        nextOrder.size -= ord.size; 
                                                        LOG("EXECUTED ORDER: " << ord.OrderID << ", size remaining of last order: " << nextOrder.size << "\n------------\n"); return 2;}                   
            }
            
            if (direction == 1) {map.erase(map.begin());}
            else                {map.erase(std::prev(map.end()));}
            
            return 0;
        }
        
        public:
            // Return certain ints so that we know if an execution is completed (and as such can measure execution times)
            // Return 0 = No executions
            // Return 1 = partial execution
            // Return 2 = Total execution
            int executeOrder(Order& ord, int price, int direction) {
                if(direction == 1) {
                    // No executions
                    if (sell.empty() || price < sell.begin()->first) {return 0;}

                    LOG("------------\nEXECUTING BUY ORDER: " << ord.OrderID << " of size: " << ord.size << " and price: " << price << "\n");
                    while(ord.size > 0 && !sell.empty() && price >= sell.begin()->first){
                        if(executeHelper(sell, 1, ord)) {return 2;}         // executeHelper returns a value IFF the order is exhausted
                    }
                    LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n");
                }

                else {
                    if (buy.empty() || price > buy.rbegin()->first) {return 0;}
                    
                    LOG("------------\nEXECUTING SELL ORDER: " << ord.OrderID << " of size: " << ord.size << " and price: " << price << "\n");
                    while(ord.size > 0 && !buy.empty() && price <= buy.rbegin()->first){
                        if(executeHelper(buy, -1, ord)) {return 2;}
                    }
                    LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n");
                }

                return 1;
            }

            // Return ints based on if it was a regular insertion or if there was an execution
            // Return 0 = No executions
            // Return 1 = An execution (partial or complete) was done
            int insertOrder(int direction, int price, int orderID, int size, double time) {
                
                Order newOrder{orderID, time, size};
                int process = executeOrder(newOrder, price, direction);
                if(process == 2) {return 1;}

                if(direction == 1)      {buy[price].push_back(newOrder);}
                else                    {sell[price].push_back(newOrder);}

                Detail details{price, time};
                ID_price_book[orderID] = details;
                return process;
            }

            // Return 2 on cancellation, so that it can be identified in stats, -1 in event of failure
            int cancelOrder(int direction, int orderID, int size, int TOTAL) {
                auto it = ID_price_book.find(orderID);
                // If the order already doesn't exist then exit early
                if (it == ID_price_book.end())  {return 1;}

                int pricePoint = it->second.price;
                double time = it->second.time;
                std::vector<Order>* priceQueue;
            
                if(direction == 1) {
                    if(buy[pricePoint].size() == 1 && (TOTAL ||buy[pricePoint].at(0).size <= size)) {buy.erase(pricePoint); ID_price_book.erase(it);return 2;} 
                    else                                                                            {priceQueue = &buy[pricePoint];}
                }

                else {
                    if(sell[pricePoint].size() == 1 && (TOTAL || sell[pricePoint].at(0).size <= size))  {sell.erase(pricePoint); ID_price_book.erase(it);return 2;} 
                    else                                                                                {priceQueue = &sell[pricePoint];}
                }

                int id = BinarySearch(priceQueue, time);
                
                if(id == -1 || priceQueue->at(id).OrderID != orderID) {return -1;}          // Is this needed? It's never really failed b4 and is an extra linear search

                if(TOTAL || priceQueue->at(id).size <= size) {priceQueue->erase(priceQueue->begin() + id); ID_price_book.erase(it); return 2;}
                else                                          {priceQueue->at(id).size -= size; return 2;}

                return -1;
            }

            void clearBook() {
                buy.clear();
                sell.clear();
                ID_price_book.clear();
            }

            std::string currentBook() {
                std::string out = "";

                // Method adapted from https://cplusplus.com/forum/general/211386, though I lowkey coulda did it myself
                // Buy Map first
                if(!buy.empty()) {
                    std::string output = "";
                    std::string result = "";
                    
                    for (auto it = buy.cbegin(); it != buy.cend(); it++) {
                        std::string convrt = "";
                        
                        for(auto order : it->second){
                            convrt += " [id: " + std::to_string(order.OrderID) + 
                                    ", sz: " + std::to_string(order.size) + 
                                    ", tm: " + std::to_string(order.time) + "], ";
                        }

                        output += std::to_string(it->first) + ":" + (convrt) + "\n-\n";
                    }
                    
                    if (output.size() >= 3) {
                        output.resize(output.size() - 3);
                    }
                    
                    out += output;
                }
                out += "\n---\n";
                // Now Sell map
                if(!sell.empty()) {
                    std::string output = "";
                    std::string result = "";
                    
                    for (auto it = sell.cbegin(); it != sell.cend(); it++) {
                        std::string convrt = "";
                        
                        for(auto order : it->second){
                            convrt += " [id: " + std::to_string(order.OrderID) + 
                                    ", sz: " + std::to_string(order.size) + 
                                    ", tm: " + std::to_string(order.time) + "], ";
                        }

                        output += std::to_string(it->first) + ":" + (convrt) + "\n-\n";
                    }
                    
                    if (output.size() >= 3) {
                        output.resize(output.size() - 3);
                    }
                    
                    out += output;
                }
                return out;
            }
};
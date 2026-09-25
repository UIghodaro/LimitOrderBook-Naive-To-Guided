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


class LOBV1 : public book{
    private:
        struct Order {                                      
            int OrderID;                         
            double time;                         
            int size;
        };
        
        struct Detail {
            int price;
            double time;
        };
        
        std::map<int, std::vector<Order>> buy;
        std::map<int, std::vector<Order>> sell;
        
        std::unordered_map<int, Detail> ID_price_book;
        
        int BinarySearch(const std::vector<Order>* priceVector, double time){
            if (priceVector == nullptr || priceVector->empty()) {
                return -1;
            }
            
            int left = 0;
            int right = priceVector->size() - 1;
            
            while(left <= right){
                
                int mid = left + (right - left)/2;
                double midTime = priceVector->at(mid).time;
                
                if (midTime == time)                        {return mid;}
                    else if (midTime < time)                {left = mid + 1;}
                    else                                    {right = mid - 1;}
                }
                
                return -1;
            }
        
        public:
            // Return certain ints so that we know if an execution is completed (and as such can measure execution times)
            // Return 0 = No executions
            // Return 1 = partial execution
            // Return 2 = Total execution
            int executeOrder(Order &ord, int price, int direction) {
                if(direction == 1) {
                    // No executions
                    if (sell.empty() || price < sell.begin()->first) {
                    return 0; 
                    }

                    if(price >= sell.begin()->first){
                        LOG("------------\nEXECUTING BUY ORDER: " << ord.OrderID << " of size: " << ord.size << " and price: " << price << "\n");
                        while(ord.size > 0 && !sell.empty() && price >= sell.begin()->first){
                            auto &[cheapestSell, orders] = *sell.begin();                   
                            
                            while(!orders.empty()) {
                                Order &nextOrder = orders.front();

                                if(nextOrder.size < ord.size) {LOG("Consume order " << nextOrder.OrderID << " of price " << cheapestSell << ", count " << nextOrder.size << "\n"); ord.size -= nextOrder.size; ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin());}
                                else if(nextOrder.size == ord.size) {LOG("Consume order " << nextOrder.OrderID << " of price " << cheapestSell << ", count " << nextOrder.size << "\n"); ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin()); sell.erase(sell.begin()); LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n"); return 2;}
                                else                      {LOG("Consume " << ord.size << " from order " <<  nextOrder.OrderID << " of price " << cheapestSell << "\n"); nextOrder.size -= ord.size; LOG("EXECUTED ORDER: " << ord.OrderID << ", size remaining of last order: " << nextOrder.size << "\n------------\n"); return 2;}                   
                            }
                            
                            sell.erase(sell.begin());
                        }
                        LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n");
                    }
                }

                else {
                    if (buy.empty() || price > buy.rbegin()->first) {
                        return 0; 
                    }
                    
                    if(price <= buy.rbegin()->first){
                        LOG("------------\nEXECUTING SELL ORDER: " << ord.OrderID << " of size: " << ord.size << " and price: " << price << "\n");
                        while(ord.size > 0 && !buy.empty() && price <= buy.rbegin()->first){
                            auto &[cheapestBuy, orders] = *buy.rbegin();                   
                            
                            while(!orders.empty()) {
                                Order &nextOrder = orders.front();

                                if(nextOrder.size < ord.size) {LOG("Consume order " << nextOrder.OrderID << " of price " << cheapestBuy << ", count " << nextOrder.size << "\n"); ord.size -= nextOrder.size; ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin());}
                                else if(nextOrder.size == ord.size) {LOG("Consume order " << nextOrder.OrderID << " of price " << cheapestBuy << ", count " << nextOrder.size << "\n"); ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin()); buy.erase(std::prev(buy.end())); LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n"); return 2;}
                                else                      {LOG("Consume " << ord.size << " from order " <<  nextOrder.OrderID << " of price " << cheapestBuy << "\n"); nextOrder.size -= ord.size;LOG("EXECUTED ORDER: " << ord.OrderID << ", size remaining of last order: " << nextOrder.size << "\n------------\n"); return 2;}                   
                            }
                            
                            buy.erase(std::prev(buy.end()));
                        }
                        LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n");
                    }
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

                double time = ID_price_book[orderID].time;
                std::vector<Order>* priceVector;
            
                if(direction == 1) {
                    if(buy[it->second.price].size() == 1 && (TOTAL ||buy[it->second.price].at(0).size <= size)) {buy.erase(it->second.price); ID_price_book.erase(orderID);return 2;} 
                    else                                                                                        {priceVector = &buy[it->second.price];}
                }

                else {
                    if(sell[it->second.price].size() == 1 && (TOTAL || sell[it->second.price].at(0).size <= size))  {sell.erase(it->second.price); ID_price_book.erase(orderID);return 2;} 
                    else                                                                                            {priceVector = &sell[it->second.price];}
                }

                int id = BinarySearch(priceVector, time);

                if(id == -1 || priceVector->at(id).OrderID != orderID) {return -1;} 

                if(TOTAL || priceVector->at(id).size <= size) {priceVector->erase(priceVector->begin() + id); ID_price_book.erase(orderID); return 2;}
                else                                          {priceVector->at(id).size -= size; return 2;}

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
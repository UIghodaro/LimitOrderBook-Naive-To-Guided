#pragma once // Prevents multi-definition errors in future

#include <iostream>
#include <string>

#include <map>
#include <unordered_map>
#include <vector>

#include <fstream>
#include <sstream>

#include "../utils/Logging.hpp"


struct Order {                                      
    int OrderID;                         
    double time;                         
    int size;
};

struct Detail {
    int price;
    double time;
};

class LOBV1 {
    private:
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

        bool executeOrder(Order &ord, int price, int direction) {
            if(direction == 1) {
                if (sell.empty() || price < sell.begin()->first) {
                return true; 
                }

                if(price >= sell.begin()->first){
                    LOG("------------\nEXECUTING BUY ORDER: " << ord.OrderID << " of size: " << ord.size << " and price: " << price << "\n");
                    while(ord.size > 0 && !sell.empty() && price >= sell.begin()->first){
                        auto &[cheapestSell, orders] = *sell.begin();                   
                        
                        while(!orders.empty()) {
                            Order &nextOrder = orders.front();

                            if(nextOrder.size < ord.size) {LOG("Consume order " << nextOrder.OrderID << " of price " << cheapestSell << ", count " << nextOrder.size << "\n"); ord.size -= nextOrder.size; ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin());}
                            else if(nextOrder.size == ord.size) {LOG("Consume order " << nextOrder.OrderID << " of price " << cheapestSell << ", count " << nextOrder.size << "\n"); ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin()); LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n"); return false;}
                            else                      {LOG("Consume " << ord.size << " from order " <<  nextOrder.OrderID << " of price " << cheapestSell << "\n"); nextOrder.size -= ord.size; LOG("EXECUTED ORDER: " << ord.OrderID << ", size remaining of last order: " << nextOrder.size << "\n------------\n"); return false;}                   
                        }
                        
                        sell.erase(sell.begin());
                    }
                    LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n");
                }
            }

            else {
                if (buy.empty() || price > buy.rbegin()->first) {
                    return true; 
                }
                
                if(price <= buy.rbegin()->first){
                    LOG("------------\nEXECUTING SELL ORDER: " << ord.OrderID << " of size: " << ord.size << " and price: " << price << "\n");
                    while(ord.size > 0 && !buy.empty() && price <= buy.rbegin()->first){
                        auto &[cheapestBuy, orders] = *buy.rbegin();                   
                        
                        while(!orders.empty()) {
                            Order &nextOrder = orders.front();

                            if(nextOrder.size < ord.size) {LOG("Consume order " << nextOrder.OrderID << " of price " << cheapestBuy << ", count " << nextOrder.size << "\n"); ord.size -= nextOrder.size; ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin());}
                            else if(nextOrder.size == ord.size) {LOG("Consume order " << nextOrder.OrderID << " of price " << cheapestBuy << ", count " << nextOrder.size << "\n"); ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin()); LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n"); return false;}
                            else                      {LOG("Consume " << ord.size << " from order " <<  nextOrder.OrderID << " of price " << cheapestBuy << "\n"); nextOrder.size -= ord.size;LOG("EXECUTED ORDER: " << ord.OrderID << ", size remaining of last order: " << nextOrder.size << "\n------------\n"); return false;}                   
                        }
                        
                        buy.erase(std::prev(buy.end()));
                    }
                    LOG("EXECUTED ORDER: " << ord.OrderID << ", size after: " << ord.size << "\n------------\n");
                }
            }
            return true;
        }

        int insertOrder(int direction, int price, int orderID, int size, double time) {
            
            Order newOrder{orderID, time, size};

            if(!executeOrder(newOrder, price, direction)) {return 1;}

            if(direction == 1)      {buy[price].push_back(newOrder);}
            else                    {sell[price].push_back(newOrder);}

            Detail details{price, time};
            ID_price_book[orderID] = details;
            return 1;
        }

        int cancelOrder(int direction, int orderID, int size, int TOTAL) {
            auto it = ID_price_book.find(orderID);
            // If the order already doesn't exist then exit early
            if (it == ID_price_book.end())  {return 1;}

            double time = ID_price_book[orderID].time;

            
            std::vector<Order>* priceVector;
        
            if(direction == 1) {
                if(buy[it->second.price].size() == 1 && (TOTAL ||buy[it->second.price].at(0).size <= size)) {buy.erase(it->second.price); ID_price_book.erase(orderID);return 1;} 
                else                                                                                        {priceVector = &buy[it->second.price];}
            }

            else {
                if(sell[it->second.price].size() == 1 && (TOTAL || sell[it->second.price].at(0).size <= size))  {sell.erase(it->second.price); ID_price_book.erase(orderID);return 1;} 
                else                                                                                            {priceVector = &sell[it->second.price];}
            }

            int id = BinarySearch(priceVector, time);

            if(id == -1 || priceVector->at(id).OrderID != orderID) {return -1;} 

            if(TOTAL || priceVector->at(id).size <= size) {priceVector->erase(priceVector->begin() + id); ID_price_book.erase(orderID); return 1;}
            else                                          {priceVector->at(id).size -= size; return 1;}


            return -1;
        }

        void clearBook() {
            buy.clear();
            sell.clear();
            ID_price_book.clear();
        }
};
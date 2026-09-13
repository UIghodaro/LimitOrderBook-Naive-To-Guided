#pragma once // Prevents multi-definition errors in future

#include <iostream>
#include <string>

#include <map>
#include <unordered_map>
#include <vector>

#include <fstream>
#include <sstream>

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

        bool executeOrder(Order &ord, int price, std::string direction) {
            if(direction == "1") {
                if (sell.empty() || price < sell.begin()->first) {
                return true; 
                }

                if(price >= sell.begin()->first){
                    while(ord.size > 0 && !sell.empty() && price >= sell.begin()->first){
                        auto &[cheapestSell, orders] = *sell.begin();                   
                        
                        while(!orders.empty()) {
                            Order &nextOrder = orders.front();

                            if(nextOrder.size < ord.size) {ord.size -= nextOrder.size; ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin());}
                            else if(nextOrder.size == ord.size) {ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin()); return false;}
                            else                      {nextOrder.size -= ord.size; return false;}                   
                        }
                        
                        sell.erase(sell.begin());
                    }
                }
            }

            else {
                if (buy.empty() || price > buy.rbegin()->first) {
                return true; 
                }

                if(price <= buy.rbegin()->first){
                    while(ord.size > 0 && !buy.empty() && price <= buy.rbegin()->first){
                        auto &[cheapestBuy, orders] = *buy.rbegin();                   
                        
                        while(!orders.empty()) {
                            Order &nextOrder = orders.front();

                            if(nextOrder.size < ord.size) {ord.size -= nextOrder.size; ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin());}
                            else if(nextOrder.size == ord.size) {ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin()); return false;}
                            else                      {nextOrder.size -= ord.size; return false;}                   
                        }
                        
                        buy.erase(std::prev(buy.end()));
                    }
                }
            }
            return true;
        }

        int insertOrder(std::string direction, int price, int orderID, int size, double time) {
            
            Order newOrder{orderID, time, size};

            if(!executeOrder(newOrder, price, direction)) {return 1;}

            if(direction == "1")   {buy[price].push_back(newOrder);}
            else                   {sell[price].push_back(newOrder);}

            Detail details{price, time};
            ID_price_book[orderID] = details;
            return 1;
        }

        int cancelOrder(std::string direction, int orderID, int size, int TOTAL) {
            int price = ID_price_book[orderID].price;
            double time = ID_price_book[orderID].time;

            
            std::vector<Order>* priceVector;
        
            if(direction == "1") {
                if(buy[price].size() == 1 && (TOTAL ||buy[price].at(0).size <= size))    {buy.erase(price); ID_price_book.erase(orderID);return 1;} 
                else                                                                      {priceVector = &buy[price];}
            }

            else {
                if(sell[price].size() == 1 && (TOTAL || sell[price].at(0).size <= size))  {sell.erase(price); ID_price_book.erase(orderID);return 1;} 
                else                                                                      {priceVector = &sell[price];}
            }

            int id = BinarySearch(priceVector, time);

            if(id == -1 || priceVector->at(id).OrderID != orderID) {return -1;} 

            if(TOTAL || priceVector->at(id).size <= size) {priceVector->erase(priceVector->begin() + id); ID_price_book.erase(orderID); return 1;}
            else                                          {priceVector->at(id).size -= size; return 1;}


            return -1;
        }
};
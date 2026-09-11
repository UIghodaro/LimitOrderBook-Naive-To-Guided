#include <iostream>
#include <string>

#include <map>
#include <unordered_map>
#include <vector>

#include <fstream>
#include <sstream>

// g++ -O3 -o book.exe book.cpp
// g++ -O3 -o book.out book.cpp

// Should hopefully make things easier
// Of message components, the following will initially be dropped: 
//    Type of message - handled by the running loop; direction (buy or sell) - maintained by the respective book maps; Price - dictates how each message is hashed anyway

//---------------------------------------------------------------------------------------
// GLOBAL OBJECT DEFINITIONS
//---------------------------------------------------------------------------------------
// So an order for a map will be structured as
struct Order {                                      
    int OrderID;                         // First up since it needs to be used for cancellations and executions
    double time;                          // Time has decimal points to increase accuracy
    int size;
};

struct Detail {
    int price;
    double time;
};

class LOBV1 {
    // We create a map (ordered!) which uses prices as keys and vectors holding information about each 
    std::map<int, std::vector<Order>> buy;
    std::map<int, std::vector<Order>> sell;     

    // Then create a map (unordered!) which uses orderIDs as keys to quickly find which price point at which an order is contained
    // as well as the time of order in order to quickly find order index by Binary search
    std::unordered_map<int, Detail> ID_price_book;


    //---------------------------------------------------------------------------------------
    // HELPER ALGORITHMS
    //---------------------------------------------------------------------------------------

    // A basic Binary Search algorithm, used for finding orders in vectors of the buy or sell side of the book in order to delete them (faster than linear search if a vector is obscenely long)
    // Returns index of the searched for item - currently uses the wrong methods lol
    int BinarySearch(const std::vector<Order>* priceVector, double time){
        if (priceVector == nullptr || priceVector->empty()) {
            return -1;
        }

        int left = 0;
        int right = priceVector->size() - 1;

        while(left <= right){

            int mid = left + (right - left)/2;
            double midTime = priceVector->at(mid).time;

            if (midTime == time)                    {return mid;}
            else if (midTime < time)                {left = mid + 1;}
            else                                    {right = mid - 1;}
        }
        
        // If this happens, there was some kinda problem - either the order has already been cancelled, the order was cancelled or the binary search just failed.
        return -1;
    }

    // Adapted from https://cplusplus.com/forum/general/211386, though I lowkey coulda did it myself
    // Allows for checking the structure of a map - should only be used for smaller testcases really
    std::string map_to_string(const std::map<int,std::vector<Order>>  &map) {
        if(map.empty()) {return "";}

        std::string output = "";
        std::string result = "";
        
        for (auto it = map.cbegin(); it != map.cend(); it++) {
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
        
    return output;
    }

    //---------------------------------------------------------------------------------------
    // MAIN BOOK LOGIC ALGORITHMS
    //---------------------------------------------------------------------------------------

    // Main logic:
    // 1. If an order doesn't meet execute conditions, return 1 and exit (don't modify the order, no execution)
    // 2. If an order DOES meet execute conditions, begin execution:
    //      - If the order is fully executed, return -1 -> the Order itself should not be stored anywhere and will be erased by the GC later
    //      - If the order is not fully executed, the order size is mutated
    bool executeOrder(Order &ord, int price, std::string direction) {
        if(direction == "1") {
            // If there is nothing to potentially execute on or execute conditions are just not met, then leave the order as is and continue
            if (sell.empty() || price < sell.begin()->first) {
            return true; 
            }

            if(price >= sell.begin()->first){
                // Prevent invalid sizes and attempting to find the beginning of a map that is empty, also allow for updating the map
                while(ord.size > 0 && !sell.empty() && price >= sell.begin()->first){
                    auto &[cheapestSell, orders] = *sell.begin();                   // Store a reference the front key-value pair
                    
                    while(!orders.empty()) {
                        Order &nextOrder = orders.front();

                        // It is faster to directly erase since we already have a pointer to the front
                        if(nextOrder.size < ord.size) {ord.size -= nextOrder.size; ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin());}

                        // Avoid size 0 orders in the order queue
                        else if(nextOrder.size == ord.size) {ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin()); return false;}

                        // If the order is satisfied however, just update the front of the top of the map and then end
                        else                      {nextOrder.size -= ord.size; return false;}                   
                    }
                    
                    // If you made it here, then the price point is empty and you need to move up or end execution
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

    // Main logic:
    // 1. [outside of function]: Is this an executable order? IF YES then execute, ELSE (or if unsatisfied) then insert
    // 2. [Inside of function]: Is the top of book changed? IF YES then change the top of book, ELSE continue
    // 3. [Inside of function]: Insert what has remained from the first check - return 1 if successful
    int insertOrder(std::string direction, int price, int orderID, int size, double time) {
        
        // Create the order 
        Order newOrder{orderID, time, size};

        // The below line will either:
        //  1. Fully execute the order, then immediately return  OR
        //  2. Mutate the order (partial execution) or leave it as is, then continue insertion using the remaining order 
        if(!executeOrder(newOrder, price, direction)) {return 1;}

        if(direction == "1")   {buy[price].push_back(newOrder);}
        else                   {sell[price].push_back(newOrder);}

        // The details are inserted the same way regardless of direction
        Detail details{price, time};
        ID_price_book[orderID] = details;
        return 1;
    }

    // If 1, then successful, if -1 then something failed
    // Direction tells us which map to check, price tells us which key, orderID tells us what to search for and cancel, size dictates the amount to slime out and TOTAL decides if it's a partial or total deletion
    // I will decide if this is correct or not later when I 
    int cancelOrder(std::string direction, int orderID, int size, int TOTAL) {
        int price = ID_price_book[orderID].price;
        double time = ID_price_book[orderID].time;

        
        // I want to save a reference point to the vector in buy or sell price don't I? 
        std::vector<Order>* priceVector;
    
        // Based on direction, look at either side of the book
        if(direction == "1") {
            // If the vector has a size of 1, then the order we are looking for is necessarily at index 0, so you can check immediately
            if(buy[price].size() == 1 && (TOTAL ||buy[price].at(0).size <= size))    {buy.erase(price); ID_price_book.erase(orderID);return 1;} 
            else                                                                      {priceVector = &buy[price];}
        }

        else {
            if(sell[price].size() == 1 && (TOTAL || sell[price].at(0).size <= size))  {sell.erase(price); ID_price_book.erase(orderID);return 1;} 
            else                                                                      {priceVector = &sell[price];}
        }

        // Now, complete a Binary Search for the index before operating
        int id = BinarySearch(priceVector, time);

        // This should never reasonably happen due to the precision of given times, but it'd be good to catch it early if there is any issue
        if(priceVector->at(id).OrderID != orderID) {return -1;} 

        // If you have reached this point, the the element found is not the lone item in the price vector and so would not trigger erasing the whole key
        // Remember to wipe the orderID from ID-Detail map though
        if(TOTAL || priceVector->at(id).size <= size) {priceVector->erase(priceVector->begin() + id); ID_price_book.erase(orderID); return 1;}
        else                                          {priceVector->at(id).size -= size; return 1;}


        // This shouldn't be reachable
        return -1;
    }
};

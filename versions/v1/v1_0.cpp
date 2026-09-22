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

struct OrderEvent {
    double time;
    int type;
    int orderID;
    int size;
    int price;
    int direction;
};

class LOBV1 {
    public:
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
                        else if(nextOrder.size == ord.size) {ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin()); sell.erase(sell.begin()); return false;}

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
                        else if(nextOrder.size == ord.size) {ID_price_book.erase(nextOrder.OrderID); orders.erase(orders.begin()); buy.erase(std::prev(buy.end())); return false;}
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
        auto it = ID_price_book.find(orderID);
        // If the order doesn't exist then exit early
        if (it == ID_price_book.end())  {return 1;}

        double time = ID_price_book[orderID].time;

        
        std::vector<Order>* priceVector;
    
        if(direction == "1") {
            if(buy[it->second.price].size() == 1 && (TOTAL ||buy[it->second.price].at(0).size <= size)) {buy.erase(it->second.price); ID_price_book.erase(orderID);return 1;} 
            else                                                                                        {priceVector = &buy[it->second.price];}
        }

        else {
            if(sell[it->second.price].size() == 1 && (TOTAL || sell[it->second.price].at(0).size <= size))  {sell.erase(it->second.price); ID_price_book.erase(orderID);return 1;} 
            else                                                                                            {priceVector = &sell[it->second.price];}
        }

        int id = BinarySearch(priceVector, time);

        if (id == -1 || priceVector->at(id).OrderID != orderID) {return -1;} 

        if(TOTAL || priceVector->at(id).size <= size) {priceVector->erase(priceVector->begin() + id); ID_price_book.erase(orderID); return 1;}
        else                                          {priceVector->at(id).size -= size; return 1;}


        return -1;
    }
};

int main() {
    std::vector<OrderEvent> testCases = {
            // Fill buy side
            OrderEvent{34200.100000,1,1001,200,290,1},
            OrderEvent{34200.150000,1,1002,150,280,1},
            OrderEvent{34200.200000,1,1003,100,270,1},
            OrderEvent{34200.250000,1,1004,200,290,1},
            OrderEvent{34200.300000,1,1005,200,290,1},

            // Buy side execution
            OrderEvent{34200.350000,1,3001,100,290,-1},     // Partial execution, order 1001: 200 -> 100
            OrderEvent{34200.400000,1,3002,150,290,-1},     // Price-time priority, order 1001: 100 -> 0 AND order 1004: 200 -> 150
            OrderEvent{34200.450000,1,3003,400,280,-1},     // Multi-level, order 1004: 150 -> 0 AND order 1005: 200 -> 0 AND order 1002: 150 -> 100
            OrderEvent{34200.500000,1,3004,100,280,-1},     // Total execution, order 1002: 100 -> 0
            
            // Fill sell side
            OrderEvent{34200.600000,1,2001,100,340,-1},      
            OrderEvent{34200.650000,1,2002,150,320,-1},
            OrderEvent{34200.700000,1,2003,200,300,-1},
            OrderEvent{34200.750000,1,2004,200,300,-1},
            OrderEvent{34200.800000,1,2005,200,300,-1},

            // Sell side execution
            OrderEvent{34200.850000,1,4001,100,300,1},      // Partial execution, order 2003: 200 -> 100
            OrderEvent{34200.900000,1,4002,150,300,1},      // Price-time priority, order 2003: 100 -> 0 AND order 2004: 200 -> 150
            OrderEvent{34200.950000,1,4003,400,320,1},      // Multi-level, order 2004: 150 -> 0 AND order 2005: 200 -> 0 AND order 2002: 150 -> 100
            OrderEvent{34201.000000,1,4004,100,320,1},      // Total execution, order 2002: 100 -> 0

            // By this point only orders 1003 and 2003 must exist
            OrderEvent{34201.050000,1,5001,200,340,1},      // Overflow execution, order 2003: 100 -> 0, order 5001 to buy map with 100 remaining
            OrderEvent{34201.100000,1,5002,200,340,-1},     // Overflow execution, order 5001: 100 -> 0, order 5002 to sell map with 100 remaining
            
            // Buy side cancel logic - only orders 1003 (BUY map for price 270) and 5002 (SELL map for price 300) must exist
            OrderEvent{34201.150000,2,1003,50,270,1},       // Partial deletion, order 1003: 100 -> 50
            OrderEvent{34201.200000,3,1003,40,270,1},       // Total deletion, order 1003: 50 -> 0
            OrderEvent{34201.250000,2,1003,50,270,1},       // Deletion of non-existent element -> buy side remains empty

            // Sell side cancel logic
            OrderEvent{34201.300000,2,5002,50,340,-1},       // Partial deletion, order 5002: 100 -> 50
            OrderEvent{34201.350000,3,5002,50,340,-1},       // Total deletion, order 5002: 50 -> 0
            OrderEvent{34201.400000,2,5002,50,340,-1}        // Deletion of non-existent element -> sell side remains empty

            // After all of the above operations, the book should be empty
        };

    LOBV1 lobook;

    for(int nxt = 0 ; nxt < testCases.size() ; nxt++){
        // Most important columns in order: type -> direction -> price/orderID (Insert or Cancel) -> size (for partial deletion) -> time (most relevant during executions, which are comparatively rare)
        auto msg = testCases[nxt];
        
        switch(msg.type) {                                               // Strings are basically lists/vectors. Hence it's type[0] which is jarring
            case 1:                                                   // New Limit Order
                if(!lobook.insertOrder(msg.direction == 1 ? "1" : "-1", msg.price, msg.orderID, msg.size, msg.time)) {
                    std::cerr << "Error inserting order " << msg.orderID  << std::endl;
                }
                break;
            case 2:                                                   // Cancel Limit Orders (Partial Deletion)
                if(!lobook.cancelOrder(msg.direction == 1 ? "1" : "-1", msg.orderID, msg.size, 0)) {
                    std::cerr << "Error canceling order " << msg.orderID  << std::endl;
                }
                break;
            case 3:                                                   // Cancel Limit Orders (Total Deletion, set TOTAL to 1 and immediately erase)
                if(!lobook.cancelOrder(msg.direction == 1 ? "1" : "-1", msg.orderID, msg.size, 1)) {
                    std::cerr << "Error canceling order " << msg.orderID << std::endl;
                }
                break;
        }
        //count += 1;
        //if(count % 1000 == 0) {std::cout << count << " messages parsed\n";}
        std::cout << nxt << ". \n------------\n" << lobook.map_to_string(lobook.buy) << "\n---\n" << lobook.map_to_string(lobook.sell) << "\n------------";
    }
}
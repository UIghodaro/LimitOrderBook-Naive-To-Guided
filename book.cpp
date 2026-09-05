#include <iostream>
#include <string>

#include <map>
#include <unordered_map>
#include <vector>

#include <fstream>
#include <sstream>

// g++ -O3 -o book.exe book.cpp

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

// If 1, then successful, if -1 then something failed
int insertOrder(std::string direction, int price, int orderID, int size, double time) {
    // Create and insert the orders into the correct maps
    Order newOrder{orderID, time, size};
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
        if(buy[price].size() == 1 && (buy[price].at(0).size <= size || TOTAL))    {buy.erase(price); ID_price_book.erase(orderID);return 1;} 
        else                                                                      {priceVector = &buy[price];}
    }

    else {
        if(sell[price].size() == 1 && sell[price].at(0).size <= size)             {sell.erase(price);} 
        else                                                                      {priceVector = &sell[price];}
    }

    // Now, complete a Binary Search for the index before operating
    int id = BinarySearch(priceVector, time);

    // This should never reasonably happen due to the precision of given times, but it'd be good to catch it early if there is any issue
    if(priceVector->at(id).OrderID != orderID) {return -1;} 

    // If you have reached this point, the the element found is not the lone item in the price vector and so would not trigger erasing the whole key
    // Remember to wipe the orderID from ID-Detail map though
    if(priceVector->at(id).size <= size || TOTAL) {priceVector->erase(priceVector->begin() + id); ID_price_book.erase(orderID); return 1;}
    else                                          {priceVector->at(id).size -= size; return 1;}


    // This shouldn't be reachable
    return -1;
}

// Check what the top of the book is at the time
void refreshTop() {

}

int executeOrder() {
    return -1;
}

//---------------------------------------------------------------------------------------
// MAIN METHOD
//---------------------------------------------------------------------------------------

int main() {
    // Not sure what a "good" method for filereading is, so I'm just gonna do "a" method
    // Initially, I left getting and parsing a message in an entirely different function, but using it in main allows me to complete necessary conversions immediately and reduces GOTO overhead

    // Get the CSV filename and then a variable 'line' which will hold each line of the file - I can already see how this'll scale to multithreading
    std::string file = "testData.csv";
    std::ifstream csv_file(file);
    std::cout << "Dataset loaded from: " << file << "\n";
    std::string line;
    

    // Test Casing ------------------------------------------------------------
    std::vector<Order> tests = {Order{16113575, 34200.004241176, 18}, 
                                Order{16113584, 34200.00426064, 18},
                                Order{16113594, 34200.004447484, 18},
                                Order{16120456, 34200.025551909, 18},
                                Order{16120480, 34200.025579546, 18}};

    std::vector<Order>* interim = &tests;

    std::cout << "----------------------------------------------------\n";

    int id = BinarySearch(interim, 34200.025551909);
    std::cout << "Binary Search test - The index of order '" << interim->at(id).OrderID << "' is: " << id << "\n";

    std::cout << "----------------------------------------------------\n";
    // Test insert order
    for (auto order : tests) {
        insertOrder("1", 10000000, order.OrderID, order.size, order.time);
    }
    insertOrder("1", 11000000, 17113584, 10, 34200.025579546); // is duplicate ordering to do with insert logic?
    // Visualise map and test insertion works as required
    std::cout << "\n" << "Insertion test - The state of the map after order insertions is as follows: \n" << map_to_string(buy) << "\n";

    std::cout << "\n" << "We then check the orderID map:\n";
    for(const auto &[id, detail] : ID_price_book){
        std::cout << "OrderID: " << id << " -> price: " << detail.price << "\n";
    }

    std::cout << "----------------------------------------------------\n";
    cancelOrder("1", 16113584, 6, 0);
    cancelOrder("1", 16120456, 2, 1);
    cancelOrder("1", 17113584, 0, 1);

    std::cout << "\n" << "Cancellation test - The state of the map after cancellations is as follows: \n" << map_to_string(buy) << "\n";
    std::cout << "\n" << "If all is well, the order '16120456' should be missing from the above, and the order '16113584' should have 12 items, not 18.\n";
    std::cout << "11100000 Should also be gone.\n";    

    std::cout << "\n" << "We then check the, hopefully, updated orderID map:\n";
    for(const auto &[id, detail] : ID_price_book){
        std::cout << "OrderID: " << id << " -> price: " << detail.price << "\n";
    }

    std::cout << "----------------------------------------------------";
    // End Testing ------------------------------------------------------------------------------

    // Read message rows and begin parsing + working 
    while (std::getline(csv_file, line)) {
        std::stringstream ss(line);
        std::string time_str, type, orderID_str, size_str, price_str, direction;

        // Read the message and complete conversions where necessary
        std::getline(ss, time_str, ',');                                double time = std::stod(time_str);
        std::getline(ss, type, ',');                                    // One digit, can remain as string
        std::getline(ss, orderID_str, ',');                             int orderID = std::stoi(orderID_str);           // If this is an int, checking for orderIDs should be leagues easier right?
        std::getline(ss, size_str, ',');                                int size = std::stoi(size_str);
        std::getline(ss, price_str, ',');                               int price = std::stoi(price_str);
        std::getline(ss, direction, ',');                               // Can complete comparison in 1 character, can remain as string

        // Most important columns in order: type -> direction -> price/orderID (Insert or Cancel) -> size (for partial deletion) -> time (most relevant during executions, which are comparatively rare)
        switch(type[0]) {                                               // Strings are basically lists/vectors. Hence it's type[0] which is jarring
            case '1':                                                   // New Limit Order
                if(!insertOrder(direction, price, orderID, size, time)) {
                    std::cerr << "Error inserting order " << orderID  << std::endl;
                }
                break;
            case '2':                                                   // Cancel Limit Orders (Partial Deletion)
                if(!cancelOrder(direction, orderID, size, 0)) {
                    std::cerr << "Error canceling order " << orderID  << std::endl;
                }
                break;
            case '3':                                                   // Cancel Limit Orders (Total Deletion, set TOTAL to 1 and immediately erase)
                if(!cancelOrder(direction, orderID, size, 1)) {
                    std::cerr << "Error canceling order " << orderID << std::endl;
                }
                break;
            //case '4':                                                   // Execute visible Orders - Effectively a market buy
            //    break;
            //case '5':                                                   // Execute hidden Orders - This doesn't actually do anything w.r.t our book, hidden orders are ones we don't know about
                break;
            case '7':                                                   // Trading Halt
                break;
        }

        // Refresh the top of the book

    }
    
    return 0;
}

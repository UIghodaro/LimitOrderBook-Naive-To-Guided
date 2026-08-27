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

    if (priceVector->at(left).time == time)     {return left;}
    if (priceVector->at(right).time == time)    {return right;}

    // TODO: USE THE RIGHT METHODS
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
// Direction tells us which map to check, price tells us which key, orderID tells us what to search for and cancel, size dictates if it's a partial deletion or not?
// I will decide if this is correct or not later when I 
int cancelOrder(std::string direction, int orderID, int size) {
    int price = ID_price_book[orderID].price;
    double time = ID_price_book[orderID].time;

    
    // I want to save a reference point to the vector in buy or sell price don't I? Would make things faster... I'll learn how to implement this with pointers later
    std::vector<Order> *priceVector;
   
    if(direction == "1")   {priceVector = &buy[price];}
    else                   {priceVector = &sell[price];}

    // Now, complete a Binary Search for the index before operating
    int id = BinarySearch(priceVector, time);
    
    return 1;
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
    

    // Binary Search by time testing ------------------------------------------------------------
    std::vector<Order> tests = {Order{16113575, 34200.004241176, 18}, 
                                Order{16113584, 34200.00426064, 18},
                                Order{16113594, 34200.004447484, 18},
                                Order{16120456, 34200.025551909, 18},
                                Order{16120480, 34200.025579546, 18}};

    std::vector<Order>* interim = &tests;

    int id = BinarySearch(interim, 34200.025551909);
    std::cout << "The index of order '" << interim->at(id).OrderID << "' is: " << id;
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
                if(!cancelOrder(direction, orderID, size)) {
                    std::cerr << "Error canceling order " << orderID  << std::endl;
                }
                break;
            case '3':                                                   // Cancel Limit Orders (Total Deletion) - There is a distinction between this and 2, I'll find out what it is soon
                if(!cancelOrder(direction, orderID, size)) {
                    std::cerr << "Error canceling order " << orderID << std::endl;
                }
                break;
            case '4':                                                   // Execute visible Orders - Effectively a market buy
                break;
            case '5':                                                   // Execute hidden Orders - This doesn't actually do anything w.r.t our book, hidden orders are ones we don't know about
                break;
            case '7':                                                   // Trading Halt
                break;
        }
    }
    
    return 0;
}

#include <iostream>
#include <string>

#include <map>
#include <unordered_map>
#include <vector>

#include <fstream>
#include <sstream>

// Should hopefully make things easier
// Of message components, the following will initially be dropped: 
//    Type of message - handled by the running loop; direction (buy or sell) - maintained by the respective book maps; Price - dictates how each message is hashed anyway

// So an order for a map will be structured as
struct Order {                                      
    int OrderID;                         // First up since it needs to be used for cancellations and executions
    int time;
    int size;
};

// We create a map (ordered!) which uses prices as keys and vectors holding information about each 
std::map<int, std::vector<Order>> buy;
std::map<int, std::vector<Order>> sell;     

// Then create a map (unordered!) which uses orderIDs as keys to quickly find which price point at which an order is contained in order to wipe it from the Orders vector
std::unordered_map<int, int> ID_price_book;

// If 1, then successful, if -1 then something failed
int insertOrder(std::string direction, int price, int orderID, int size, int time) {
    Order newOrder{orderID, time, size}; 
    buy[price].push_back(newOrder);
    ID_price_book[orderID] = price;
    return 1;
}

// If 1, then successful, if -1 then something failed
// Direction tells us which map to check, price tells us which key, orderID tells us what to search for and cancel, size dictates if it's a partial deletion or not?
// I will decide if this is correct or not later when I 
int cancelOrder(std::string direction, int price, int orderID, int size) {
    
    return 1;
}

int executeOrder() {
    
}

int main() {
    // Not sure what a "good" method for filereading is, so I'm just gonna do "a" method
    // Initially, I left getting and parsing a message in an entirely different function, but using it in main allows me to complete necessary conversions immediately and reduces GOTO overhead

    // Get the CSV filename and then a variable 'line' which will hold each line of the file - I can already see how this'll scale to multithreading
    std::string file = "testData.csv";
    std::ifstream csv_file(file);
    std::cout << "Dataset loaded from: " << file;
    std::string line;
    
    // Read message rows and begin parsing + working 
    while (std::getline(csv_file, line)) {
        std::stringstream ss(line);
        std::string time_str, type, orderID_str, size_str, price_str, direction;

        // Read the message and complete conversions where necessary
        std::getline(ss, time_str, ',');                                int time = std::stoi(time_str);
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
                if(!cancelOrder(direction, price, orderID, size)) {
                    std::cerr << "Error canceling order " << orderID  << std::endl;
                }
                break;
            case '3':                                                   // Cancel Limit Orders (Total Deletion) - There is a distinction between this and 2, I'll find out what it is soon
                if(!cancelOrder(direction, price, orderID, size)) {
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

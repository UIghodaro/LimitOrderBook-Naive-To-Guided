#include <iostream>

#include <map>
#include <string>
#include <vector>

#include <fstream>
#include <sstream>

// Should hopefully make things easier
struct Order {
    std::string time;
    int type;
    std::string OrderID;
    int size;
    int price;             // Prices are unconstrained in length, each multiplied by 1000 too... 32 digits maybe - change to long later if necessary
    int Direction;
};



// We create a map (ordered!) which uses prices as keys and vectors holding information about each 
std::map<std::string, std::vector<Order>> book;

// If 1, then successful, if -1 then something failed
bool insertOrder():
    
    return false

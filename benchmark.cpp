#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <chrono>

#include "versions/v1_0.hpp"
#include "Utilities.hpp"


int  main() {
    //---------------------------------------------------------------------------------------
    // INITIALISING
    //---------------------------------------------------------------------------------------

    auto messageQ = Utils::generateSyntheticMessages(20, 42);
    LOBV1 book;

    std::cout << "Order vector:\n----\n";

    for(auto &msg : messageQ){
        std::string front = msg.type == 1 ? "FILL" : "CANCEL"; 
        std::cout << front << " ORDER:" << msg.orderID << " SIZE: " << msg.size << " PRICE: " << msg.price << " DIR: " << msg.direction << "\n";
    }
    
    std::cout << "----\n";
    
    /*
    std::string file = "data/AAPL_2012-06-21_34200000_57600000_message_5.csv";
    std::ifstream csv_file(file);
    std::cout << "Dataset loaded from: " << file << "\n";
    std::string line;
    */

    //---------------------------------------------------------------------------------------
    auto start = std::chrono::high_resolution_clock::now();

    //---------------------------------------------------------------------------------------
    // BEGIN MEASURING TIME
    //---------------------------------------------------------------------------------------

    // Begin parsing the message queue
    // Remember: OrderEvent = time, type, orderID, size, price, direction
    while (!messageQ.empty()){
        // Most important columns in order: type -> direction -> price/orderID (Insert or Cancel) -> size (for partial deletion) -> time (most relevant during executions, which are comparatively rare)
        auto msg = messageQ.front();
        
        switch(msg.type) {                                               // Strings are basically lists/vectors. Hence it's type[0] which is jarring
            case 1:                                                   // New Limit Order
                if(!book.insertOrder(msg.direction, msg.price, msg.orderID, msg.size, msg.time)) {
                    std::cerr << "Error inserting order " << msg.orderID  << std::endl;
                } else {
                    std::cout << "Fill - ORDER: " << msg.orderID << " with PRICE: " << msg.price << " and SIZE: " << msg.size << " to MAP: " << msg.direction <<"\n";
                }
                break;
            case 2:                                                   // Cancel Limit Orders (Partial Deletion)
                if(!book.cancelOrder(msg.direction, msg.orderID, msg.size, 0)) {
                    std::cerr << "Error canceling order " << msg.orderID  << std::endl;
                } else {
                    std::cout << "Cancel - ORDER: " << msg.orderID << " with SIZE: " << msg.size << " on MAP: " << msg.direction <<"\n";
                }
                break;
            case 3:                                                   // Cancel Limit Orders (Total Deletion, set TOTAL to 1 and immediately erase)
                if(!book.cancelOrder(msg.direction, msg.orderID, msg.size, 1)) {
                    std::cerr << "Error canceling order " << msg.orderID << std::endl;
                } else {
                    std::cout << "Total Cancel - ORDER: " << msg.orderID << " on MAP: " << msg.direction <<"\n";
                }
                break;
        }
        messageQ.erase(messageQ.begin());
    }

    //---------------------------------------------------------------------------------------
    // END MEASURE
    //---------------------------------------------------------------------------------------

    auto end = std::chrono::high_resolution_clock::now();
    //---------------------------------------------------------------------------------------

    std::chrono::duration<double,std::milli> elapsed = end-start;
    std::cout << "Time elapsed: " << elapsed.count() << "ms\n";
    //std::cout << "-----------------\n" << "-Buy map:\n" << map_to_string(book.buy) << "\n\n-Sell map:\n" << map_to_string(book.sell) <<"\n-----------------\n";
    return 0;
}

/*  OLD LOGIC FOR PARSING FILES - will probably put back in later

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
                if(!book.insertOrder(direction, price, orderID, size, time)) {
                    std::cerr << "Error inserting order " << orderID  << std::endl;
                }
                break;
            case '2':                                                   // Cancel Limit Orders (Partial Deletion)
                if(!book.cancelOrder(direction, orderID, size, 0)) {
                    std::cerr << "Error canceling order " << orderID  << std::endl;
                }
                break;
            case '3':                                                   // Cancel Limit Orders (Total Deletion, set TOTAL to 1 and immediately erase)
                if(!book.cancelOrder(direction, orderID, size, 1)) {
                    std::cerr << "Error canceling order " << orderID << std::endl;
                }
                break;
            //case '4':                                                   // Execute visible Orders - Effectively a market buy
            //    break;
            //case '5':                                                   // Execute hidden Orders - This doesn't actually do anything w.r.t our book, hidden orders are ones we don't know about
            //    break;
            case '7':                                                     // Trading Halt
                break;
            Default: 
                break;
        }
    }
    */
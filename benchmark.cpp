#include <random>
#include <iostream>
#include <vector>

#include <chrono>

#include "versions/v1_0.hpp"

int  main() {
    //---------------------------------------------------------------------------------------
    // INITIALISING
    //---------------------------------------------------------------------------------------

    LOBV1 book;

    std::string file = "versions/Test.csv";
    std::ifstream csv_file(file);
    std::cout << "Dataset loaded from: " << file << "\n";
    std::string line;

    auto start = std::chrono::high_resolution_clock::now();

    //---------------------------------------------------------------------------------------
    // BEGIN MEASURING TIME
    //---------------------------------------------------------------------------------------

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
                break;
            case '7':                                                   // Trading Halt
                break;
        }
    }

    //---------------------------------------------------------------------------------------
    // END MEASURE
    //---------------------------------------------------------------------------------------

    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double,std::milli> elapsed = end-start;
    std::cout << "Time elapsed: " << elapsed.count() << "ms\n";
    return 0;
}
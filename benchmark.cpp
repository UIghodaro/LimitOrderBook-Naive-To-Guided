#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <algorithm>

#include "utils/Logging.hpp"
#include "utils/Utilities.hpp"

#include "versions/bookObject.hpp"
#include "versions/v1_0.hpp"




int  main() {
    //---------------------------------------------------------------------------------------
    // INITIALISING
    //---------------------------------------------------------------------------------------
    int numMessages = 1'000'000;
    std::cout << "Generating messages...";
    auto messageQ = Utils::generateSyntheticMessages(numMessages, 42);
    std::cout<< "\n---\nMessages Generated.\n";

    LOBV1 lobook;

    // Verbose message vector construction, for smaller test cases
    LOGN(
        std::cout << "Order vector:\n----\n";

    for(auto &msg : messageQ){
        std::string front = msg.type == 1 ? "FILL" : "CANCEL"; 
        std::cout << front << " ORDER:" << msg.orderID << " SIZE: " << msg.size << " PRICE: " << msg.price << " DIR: " << msg.direction << "\n";
    }

    std::cout << "----\n"
    );
    
    /*
    std::string file = "data/AAPL_2012-06-21_34200000_57600000_message_5.csv";
    std::ifstream csv_file(file);
    std::cout << "Dataset loaded from: " << file << "\n";
    std::string line;
    */

    //---------------------------------------------------------------------------------------
    std::cout << "------------\n";
    std::cout << "Beginning LOB benchmark 1 - Throughput\n";
    //std::cout << "------------\n"; int count = 0;

    auto start = std::chrono::high_resolution_clock::now();
    //---------------------------------------------------------------------------------------
    // BEGIN MEASURING TIME
    //---------------------------------------------------------------------------------------

    // Begin parsing the message queue
    // Remember: OrderEvent = time, type, orderID, size, price, direction
    for(int nxt = 0 ; nxt < messageQ.size() ; nxt++){
        // Most important columns in order: type -> direction -> price/orderID (Insert or Cancel) -> size (for partial deletion) -> time (most relevant during executions, which are comparatively rare)
        auto msg = messageQ[nxt];
        
        switch(msg.type) {                                               // Strings are basically lists/vectors. Hence it's type[0] which is jarring
            case 1:                                                   // New Limit Order
                if(!lobook.insertOrder(msg.direction, msg.price, msg.orderID, msg.size, msg.time)) {
                    std::cerr << "Error inserting order " << msg.orderID  << std::endl;
                } LOGN(else {
                    std::cout << "Fill - ORDER: " << msg.orderID << " with PRICE: " << msg.price << " and SIZE: " << msg.size << " to MAP: " << msg.direction <<"\n";
                })
                break;
            case 2:                                                   // Cancel Limit Orders (Partial Deletion)
                if(!lobook.cancelOrder(msg.direction, msg.orderID, msg.size, 0)) {
                    std::cerr << "Error canceling order " << msg.orderID  << std::endl;
                } LOGN(else {
                    std::cout << "Cancel - ORDER: " << msg.orderID << " with SIZE: " << msg.size << " on MAP: " << msg.direction <<"\n";
                })
                break;
            case 3:                                                   // Cancel Limit Orders (Total Deletion, set TOTAL to 1 and immediately erase)
                if(!lobook.cancelOrder(msg.direction, msg.orderID, msg.size, 1)) {
                    std::cerr << "Error canceling order " << msg.orderID << std::endl;
                } LOGN(else {
                    std::cout << "Total Cancel - ORDER: " << msg.orderID << " on MAP: " << msg.direction <<"\n";
                })
                break;
        }
        //count += 1;
        //if(count % 1000 == 0) {std::cout << count << " messages parsed\n";}
    }

    //---------------------------------------------------------------------------------------
    // END MEASURE
    //---------------------------------------------------------------------------------------

    auto end = std::chrono::high_resolution_clock::now();
    //---------------------------------------------------------------------------------------

    std::chrono::duration<double,std::milli> elapsed = end-start;
    std::cout << "------------\nBenchmark 1 Complete. Stats:\n";
    std::cout << "Time elapsed: " << elapsed.count() << "ms\n";
    std::cout << "Throughput Messages Per Second: " << numMessages/(elapsed.count()/1000) << "\n";
    //std::cout << "-----------------\n" << "-Buy map:\n" << map_to_string(lobook.buy) << "\n\n-Sell map:\n" << map_to_string(lobook.sell) <<"\n-----------------\n";
    
    lobook.clearBook();
    std::vector<uint32_t> latencies_ns;
    latencies_ns.resize(messageQ.size());

    std::cout << "\n------------\n";
    std::cout << "Beginning LOB benchmark 2 - Latency\n";
    //std::cout << "------------\n"; int count = 0;

    
    for(int nxt = 0 ; nxt < messageQ.size() ; nxt++){
        auto msg = messageQ[nxt];

        //---------------------------------------------------------------------------------------
        // BEGIN MEASURING TIME
        //---------------------------------------------------------------------------------------
        auto t0 = std::chrono::high_resolution_clock::now();
        
        switch(msg.type) {                                               
            case 1:                                                   // New Limit Order
                if(!lobook.insertOrder(msg.direction, msg.price, msg.orderID, msg.size, msg.time)) {
                    std::cerr << "Error inserting order " << msg.orderID  << std::endl;
                } LOGN(else {
                    std::cout << "Fill - ORDER: " << msg.orderID << " with PRICE: " << msg.price << " and SIZE: " << msg.size << " to MAP: " << msg.direction <<"\n";
                })
                break;
            case 2:                                                   
                if(!lobook.cancelOrder(msg.direction, msg.orderID, msg.size, 0)) {
                    std::cerr << "Error canceling order " << msg.orderID  << std::endl;
                } LOGN(else {
                    std::cout << "Cancel - ORDER: " << msg.orderID << " with SIZE: " << msg.size << " on MAP: " << msg.direction <<"\n";
                })
                break;
            case 3:                                                   
                if(!lobook.cancelOrder(msg.direction, msg.orderID, msg.size, 1)) {
                    std::cerr << "Error canceling order " << msg.orderID << std::endl;
                } LOGN(else {
                    std::cout << "Total Cancel - ORDER: " << msg.orderID << " on MAP: " << msg.direction <<"\n";
                })
                break;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        latencies_ns[nxt] = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        //count += 1;
        //if(count % 1000 == 0) {std::cout << count << " messages parsed\n";}
        //---------------------------------------------------------------------------------------
        // END MEASURE
        //---------------------------------------------------------------------------------------
    }


    //---------------------------------------------------------------------------------------

    std::cout << "------------\nBenchmark 2 Complete. Stats:\n";
    
    std::sort(latencies_ns.begin(), latencies_ns.end());

    size_t n = latencies_ns.size();
    uint32_t p50  = latencies_ns[n * 0.50];
    uint32_t p99  = latencies_ns[n * 0.99];
    uint32_t p999 = latencies_ns[n * 0.999];
    uint32_t max  = latencies_ns.back();
    
    std::cout << "Number of messages: " << n << "\n";
    std::cout << "Median/Average processing time: " << p50 << "ns\n";
    std::cout << "99th Percentile processing time: " << p99 << "ns\n";
    std::cout << "99.9th Percentile processing time: " << p999 << "ns\n";
    std::cout << "Max processing time: " << max << "ns\n";    

    return 0;
}

/*  OLD LOGIC FOR PARSING FILES - will probably put back in later
    std::string file = "Test.csv";
    std::ifstream csv_file(file);
    std::cout << "Dataset loaded from: " << file << "\n";
    std::string line;

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
                if(!lobook.insertOrder(direction, price, orderID, size, time)) {
                    std::cerr << "Error inserting order " << orderID  << std::endl;
                }
                break;
            case '2':                                                   // Cancel Limit Orders (Partial Deletion)
                if(!lobook.cancelOrder(direction, orderID, size, 0)) {
                    std::cerr << "Error canceling order " << orderID  << std::endl;
                }
                break;
            case '3':                                                   // Cancel Limit Orders (Total Deletion, set TOTAL to 1 and immediately erase)
                if(!lobook.cancelOrder(direction, orderID, size, 1)) {
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
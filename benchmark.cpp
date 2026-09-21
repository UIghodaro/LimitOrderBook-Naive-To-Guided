#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <chrono>
#include <algorithm>
#include <iomanip>

#include "utils/Logging.hpp"
#include "utils/Utilities.hpp"
#include "utils/statGetters.hpp"

#include "versions/bookObject.hpp"
#include "versions/v1_0.hpp"


int  main() {
    //---------------------------------------------------------------------------------------
    // INITIALISING
    //---------------------------------------------------------------------------------------
    int numMessages = 10'000'000;
    std::cout << "Generating messages...";
    auto messageQ = Utils::generateSyntheticMessages(numMessages, 42);
    std::cout<< "\nMessages Generated.\n---\nInstantiating and validating book...\n";

    LOBV1 lobook;
    auto validation = Utils::validateBook(lobook);
    if (!validation) {
        std::cerr << "Book validation failed - invalid book logic found\n";
        return 1;
    }
    std::cout << "Book validated\n";

    lobook.clearBook();
    int numIterations = 5;

    // Verbose message vector construction, for smaller test cases
    LOGN(
        std::cout << "Order vector:\n----\n";
        for(auto &msg : messageQ){
            std::string front = msg.type == 1 ? "FILL" : "CANCEL"; 
            std::cout << front << " ORDER:" << msg.orderID << " SIZE: " << msg.size << " PRICE: " << msg.price << " DIR: " << msg.direction << "\n";
        }
        std::cout << "----\n"
    );

    //---------------------------------------------------------------------------------------
    std::cout << "------------\n";
    std::cout << "Beginning LOB benchmark 1 - Throughput\n";
    std::vector<double> times;
    times.resize(numIterations);

    for (int iteration = 0; iteration < numIterations; iteration++) {

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
                case 1: lobook.insertOrder(msg.direction, msg.price, msg.orderID, msg.size, msg.time); LOGN( std::cout << "Fill - ORDER: " << msg.orderID << " with PRICE: " << msg.price << " and SIZE: " << msg.size << " to MAP: " << msg.direction <<"\n";)
                        break;
                case 2: lobook.cancelOrder(msg.direction, msg.orderID, msg.size, 0); LOGN(std::cout << "Cancel - ORDER: " << msg.orderID << " with SIZE: " << msg.size << " on MAP: " << msg.direction <<"\n";)
                        break;
                case 3: lobook.cancelOrder(msg.direction, msg.orderID, msg.size, 1); LOGN(std::cout << "Total Cancel - ORDER: " << msg.orderID << " on MAP: " << msg.direction <<"\n";)
                        break;
            }
            //count += 1;
            //if(count % 1000 == 0) {std::cout << count << " messages parsed\n";}
        }
    
        //---------------------------------------------------------------------------------------
        // END MEASURE
        //---------------------------------------------------------------------------------------
    
        auto end = std::chrono::high_resolution_clock::now();
        std::cout <<"-- RUN " << iteration+1 << " COMPLETE.\n";
        //---------------------------------------------------------------------------------------
        std::chrono::duration<double,std::milli> elapsed = end-start;
        times[iteration] = elapsed.count();
        lobook.clearBook();
    }

    T1stats benchOneStats = Stats::getBenchmarkOneStats(times);
    std::cout << "------------\nBenchmark 1 Complete. Stats:\n";
    std::string front1 = numIterations == 1 ? "Time Elapsed: " : "Average Time Elapsed: ";
    std::string front2 = numIterations == 1 ? "Throughput: ~" : "Average Throughput: ~";
    std::cout << front1 << benchOneStats.mean << " ± " << benchOneStats.stdd << " ms\n";
    std::cout << front2 << numMessages/(benchOneStats.mean/1000) << " messages per second\n";
    //std::cout << "-----------------\n" << "-Buy map:\n" << map_to_string(lobook.buy) << "\n\n-Sell map:\n" << map_to_string(lobook.sell) <<"\n-----------------\n";
    
    //-------------------------------------------------------
    // INITIALISING FOR BENCHMARK 2
    //-------------------------------------------------------
    std::vector<uint32_t> overallLatencies_ns;
    overallLatencies_ns.resize(messageQ.size());

    // Reserve space based on probability of message - defined in message generation, see Utilities.hpp 
    std::unordered_map<int, std::vector<uint32_t>> typeLatencies;
    typeLatencies[0].reserve(0.55 * messageQ.size());
    typeLatencies[1].reserve(0.15 * messageQ.size());
    typeLatencies[2].reserve(0.3 * messageQ.size());

    std::cout << "\n------------\n";
    std::cout << "Beginning LOB benchmark 2 - Latency\n";
    //std::cout << "------------\n"; int count = 0;
    int ok;                                               
    

    for(int nxt = 0 ; nxt < messageQ.size() ; nxt++){
        auto msg = messageQ[nxt];

        //---------------------------------------------------------------------------------------
        // BEGIN MEASURING TIME
        //---------------------------------------------------------------------------------------
        auto t0 = std::chrono::high_resolution_clock::now();
        
        switch(msg.type) {
            case 1: ok = lobook.insertOrder(msg.direction, msg.price, msg.orderID, msg.size, msg.time); break;
            case 2: ok = lobook.cancelOrder(msg.direction, msg.orderID, msg.size, 0); break;
            case 3: ok = lobook.cancelOrder(msg.direction, msg.orderID, msg.size, 1); break;
        }

        auto t1 = std::chrono::high_resolution_clock::now();
        //count += 1;
        //if(count % 1000 == 0) {std::cout << count << " messages parsed\n";}
        //---------------------------------------------------------------------------------------
        // END MEASURE
        //---------------------------------------------------------------------------------------
        uint32_t measure = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        overallLatencies_ns[nxt] = measure;
        typeLatencies[ok].push_back(measure);
    }

    lobook.clearBook();

    //---------------------------------------------------------------------------------------

    std::cout << "------------\nBenchmark 2 Complete. Stats:\n";
    
    std::sort(overallLatencies_ns.begin(), overallLatencies_ns.end());
    for(int i = 0; i < 3; i++) {std::sort(typeLatencies[i].begin(), typeLatencies[i].end());}

    T2stats overall = Stats::getBenchmarkTwoStats(overallLatencies_ns); T2stats fills = Stats::getBenchmarkTwoStats(typeLatencies[0]);
    T2stats executions = Stats::getBenchmarkTwoStats(typeLatencies[1]); T2stats cancels = Stats:: getBenchmarkTwoStats(typeLatencies[2]);

    Stats::printStatsTable(fills, cancels, executions, overall);

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
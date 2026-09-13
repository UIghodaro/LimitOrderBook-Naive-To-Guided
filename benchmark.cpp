#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <chrono>

#include "versions/v1_0.hpp"

struct OrderEvent {
    double time;
    int type;
    int orderID;
    int size;
    int price;
    int direction;
};

struct OpenOrder {
    int orderID; 
    int price; 
    int direction; 
    int size; 
};

// Only the number of messages and seed (test case, reproducible) is needed, others if you really wanna
std::vector<OrderEvent> generateSyntheticMessages(
    int numMessages,
    unsigned int seed,
    int startPrice = 10000,      
    int tickSize = 100,
    double reversionStrength = 0.1,
    double crossProbability = 0.15,         // fraction of inserts that are aggressive/crossing
    double cancelProbability = 0.3,         // fraction of messages that are cancels vs inserts
    double partialCancelFraction = 0.2      // Of cancels, fraction that are type 2 vs type 3
) {
    std::mt19937 rng(seed);                                             
    std::uniform_real_distribution<double> uniform(0.0, 1.0);           // Probability x, as they say
    std::normal_distribution<double> walkStep(0.0, 1.0);                // Probability y, as they say
    std::uniform_int_distribution<int> sizeDist(1, 200);                //

    std::vector<OrderEvent> messages;
    messages.reserve(numMessages);                                      // Avoid overhead of the vector doubling in size each time 

    double fairPrice = startPrice;
    double time = 34000.0; 
    int nextOrderID = 1;                                                // Start from the start, no benefit with starting on high numbers

    // Only existing orders can be deleted, track them
    // Each entry: {orderID, price, direction, remainingSize}
    std::vector<OpenOrder> openOrders;

    //---------------------------------------------------------------------------------------
    // CORE MESSAGE GEN LOOP
    //---------------------------------------------------------------------------------------

    for (int i = 0; i < numMessages; ++i) {
        // Advance time
        time += 0.0001 + uniform(rng) * 0.01;

        // Mean-reverting random walk on fair price
        // Works as follows:
        // - The start price [parameter] is the mean that we have set and wish to hover around, while the "fair price" is a price derived from that which we want to keep within bounds of the start
        // - As such, see how far the current fair price is from the start price:
        //     1. If it isn't far at all then reversion is closer to 0 and doesn't push the fair price in any direction
        //     2. If fair price is farther from the start price, then reversion will increase the amount of steps taken back toward the mean - give it a "nudge"
        
        // Overall, we prevent divergence but preserve randomness! It's very simple and cool actually 
        double reversion = reversionStrength * (startPrice - fairPrice);
        fairPrice += reversion + walkStep(rng) * tickSize;

        bool shouldCancel = !openOrders.empty() && uniform(rng) < cancelProbability;

        // The below was done w ai help lowkey
        if (shouldCancel) {
            // Pick a random, currently-open order to cancel
            std::uniform_int_distribution<int> pick(0, (int)openOrders.size() - 1);
            int idx = pick(rng);
            OpenOrder &target = openOrders[idx];

            // If the probability exceeds the partial cancel threshold and there are multiple orders, create a partial cancel
            // Otherwise, make it total (target.size)
            bool isPartial = uniform(rng) < partialCancelFraction && target.size > 1;
            int cancelSize = isPartial ? std::max(1, target.size / 2) : target.size;

            // Push a new cancel message
            messages.push_back({time, isPartial ? 2 : 3, target.orderID, cancelSize, target.price, target.direction});

            if (isPartial) {
                target.size -= cancelSize;
            } else {
                // O(1)! overwrite the index of the cancelled order with the order at the end of the vector and then pop the end of the vector
                // Imagine 1 2 3 4 5 6 with 3 to be removed being done as 1 2 6 4 5 - 3 is gone and 6 is preserved, though in a different position 
                openOrders[idx] = openOrders.back();
                openOrders.pop_back();
            }

        } else {
            // Insert to buy or sell, assume they come in equal amounts
            int direction = uniform(rng) < 0.5 ? 1 : -1;
            bool crossing = uniform(rng) < crossProbability;

            // Prices will come 1-5 ticks away from fair price
            int spread = tickSize * (1 + (int)(uniform(rng) * 5)); 
            int price;

            if (crossing) {
                // Aggressive: buy priced above fair, sell priced below fair
                price = direction == 1 ? (int)fairPrice + spread : (int)fairPrice - spread;
            } else {
                // Passive: buy below fair, sell above fair
                price = direction == 1 ? (int)fairPrice - spread : (int)fairPrice + spread;
            }

            // Quantize to tick size
            price = (price / tickSize) * tickSize;

            int size = sizeDist(rng);
            int orderID = nextOrderID++;

            //Push the orders! Into both vectors tyvm
            messages.push_back({time, 1, orderID, size, price, direction});
            openOrders.push_back({orderID, price, direction, size});
        }
    }

    return messages;
}

int  main() {
    //---------------------------------------------------------------------------------------
    // INITIALISING
    //---------------------------------------------------------------------------------------

    std::vector<OrderEvent> messageQ;
    LOBV1 book;

    std::random_device rd;
    std::mt19937 gen(rd());                                                 // 'Mersenne Twister' reappearance - apparently the de-facto good good

    std::string file = "data/AAPL_2012-06-21_34200000_57600000_message_5.csv";
    std::ifstream csv_file(file);
    std::cout << "Dataset loaded from: " << file << "\n";
    std::string line;

    //---------------------------------------------------------------------------------------
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
            //    break;
            case '7':                                                     // Trading Halt
                break;
            Default: 
                break;
        }
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
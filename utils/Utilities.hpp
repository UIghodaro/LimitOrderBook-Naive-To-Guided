#pragma once

#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <set>

#include "../versions/bookObject.hpp"
#include "../versions/v1/v1_0.hpp"

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
namespace Utils {
    //---------------------------------------------------------------------------------------
    // ORDERBOOK SPEED - MESSAGE GENERATION FUNCTION
    //---------------------------------------------------------------------------------------
    // Generates a number of synthetic messages according to the rules of what an orderbook might receive, before loading them into a vector

    inline std::vector<OrderEvent> generateSyntheticMessages(
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

    //---------------------------------------------------------------------------------------
    // ORDERBOOK CORRECTNESS - BOOK VALIDATION FUNCTION
    //---------------------------------------------------------------------------------------
    // Validates the order book by going through the logical test cases required and comparing the output of a given book with the output of a book which is known to produce correct outputs.
    // The validity of the the book known to produce correct outputs (book v1.0) is tested in versions/v1_0.cpp. Book states are printed to the terminal

    inline bool validateBook(book& lobook) {
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

        LOBV1 validator;                        // LOB version 1 has been confirmed to have correct logic, therefore use it as the validator

        for(int nxt = 0 ; nxt < testCases.size() ; nxt++){
            // Most important columns in order: type -> direction -> price/orderID (Insert or Cancel) -> size (for partial deletion) -> time (most relevant during executions, which are comparatively rare)
            auto msg = testCases[nxt];
            std::string debug;

            switch(msg.type) {                                               // Strings are basically lists/vectors. Hence it's type[0] which is jarring
                case 1:                                                      // New Limit Order
                    debug = "INSERT";
                    lobook.insertOrder(msg.direction, msg.price, msg.orderID, msg.size, msg.time);
                    validator.insertOrder(msg.direction, msg.price, msg.orderID, msg.size, msg.time);
                    break;
                case 2:                                                   // Cancel Limit Orders (Partial Deletion)
                    debug = "CANCEL";
                    lobook.cancelOrder(msg.direction, msg.orderID, msg.size, 0);
                    validator.cancelOrder(msg.direction, msg.orderID, msg.size, 0);
                    break;
                case 3:                                                   // Cancel Limit Orders (Total Deletion, set TOTAL to 1 and immediately erase)
                    debug = "CANCEL";
                    lobook.cancelOrder(msg.direction, msg.orderID, msg.size, 1);
                    validator.cancelOrder(msg.direction, msg.orderID, msg.size, 1);
                    break;
            }

            if(lobook.currentBook() != validator.currentBook()) {std::cout << "Validation Failed at " << debug; return false;}
        }

        return true;
        
    } 
}
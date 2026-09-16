#pragma once

#include <random>
#include <iostream>
#include <string>
#include <vector>
#include <set>

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
} 
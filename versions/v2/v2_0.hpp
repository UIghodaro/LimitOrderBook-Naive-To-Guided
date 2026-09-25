#pragma once // Prevents multi-definition errors in future

#include <iostream>
#include <string>

#include <unordered_map>
#include <vector>

#include <fstream>
#include <sstream>

#include "../../utils/Logging.hpp"
#include "../bookObject.hpp"
#include <stack>


class LOBV2 : public book{
    private:
        struct Price;                    // Forward-declaration, avoids errors
        
        struct Order{
            int orderID;
            double time;
            int size;
            Order *nextOrder;
            Order *prevOrder;
            Price *parentPrice;
        };
        
        struct Price{
            int limitPrice;
            Price *parent;
            Price *leftChild;
            Price *rightChild;
            Order *headOrder;           // Hold this for faster exections
            Order *tailOrder;           // This tells us where the end of the linkedlist is right? Is that why we hold onto it?
        };
        
        Price *buyTree;
        Price *sellTree;
        Price *lowestSell;      // Always have pointers to the best bid or ask so that execution can happen quickly. If it exhausts then simply move to the parent
        Price *highestBuy;

        // Quick lookup pointers
        std::unordered_map<int, Price*> sellPrices;
        std::unordered_map<int, Price*> buyPrices;
        std::unordered_map<int, Order*> orderMap;

        // InOrder traversal via iteration, I admit I consulted geeksforgeeks for this
        std::string traversePrint(Price* root) {
            std::string out = "";
            std::string output = "";

            Price* curr = root;
            std::stack<Price*> stack;

            while(!stack.empty() || curr != nullptr){
                while (curr != nullptr) {
                    stack.push(curr);
                    curr = curr->leftChild;
                }

                // Reach this when you are at a point thatt is as left as possible and hasn't been parsed 
                curr = stack.top(); stack.pop();
                std::string convrt = "";
                Order* nxtOrder = curr->headOrder;
                
                // Loop through the order queue, appending details of each order
                while(nxtOrder != nullptr){
                    convrt += " [id: " + std::to_string(nxtOrder->orderID) + 
                            ", sz: " + std::to_string(nxtOrder->size) + 
                            ", tm: " + std::to_string(nxtOrder->time) + "], ";

                    nxtOrder = nxtOrder->nextOrder;
                }

                output += std::to_string(curr->limitPrice) + ":" + (convrt) + "\n-\n";
                if (output.size() >= 3) {
                    output.resize(output.size() - 3);
                }
                out += output;

                curr = curr->rightChild;
            }

            return out;
        }


    public:
        int executeOrder(Order &ord, int price, int direction) {return 0;}

        int insertOrder(int direction, int price, int orderID, int size, double time) {
            
            // Same logic as v1
            Order newOrder{orderID, time, size, nullptr, nullptr, nullptr};
            int process = executeOrder(newOrder, price, direction);
            if(process == 2) {return 1;}

            // Then begin v2 tree insertion logic
            std::unordered_map<int, Price*>& lookupMap = direction == 1 ? buyPrices : sellPrices;
            
            if(lookupMap[price] != nullptr && lookupMap[price]->tailOrder != nullptr) {                   // If price level exists - O(1)
                Order* priceTailPtr = lookupMap[price]->tailOrder;

                priceTailPtr->nextOrder = &newOrder;        // Provide a link from the current end of the list to this order
                newOrder.prevOrder = priceTailPtr;          // Provide a link from the order back to the list (double linkage)
                lookupMap[price]->tailOrder = &newOrder;    // Update so that the tail is now the newly inserted order

            } else {                                        // If price level is not already in tree - traverse and place O(log M) where M is the number of price levels in the tree

                Price* nextNodePtr = direction == 1 ? buyTree : sellTree;
                
                while (nextNodePtr != nullptr) {            // Traverse to find the right place to slot in the price level. 
                    if(price > nextNodePtr->limitPrice) {nextNodePtr = nextNodePtr->rightChild;}
                    else                                {nextNodePtr = nextNodePtr->leftChild;}
                }

                Price newPrice{price, nextNodePtr->parent, nullptr, nullptr, &newOrder, &newOrder};
                lookupMap[price] = &newPrice;
            }

            // Regardless of if the price level exists, you must leave a reference to the order for quick cancellations
            orderMap[orderID] = &newOrder;
            return 0;
        }

        int cancelOrder(int direction, int orderID, int size, int TOTAL) { return 2; }

        void clearBook() {
            buyTree = nullptr; sellTree = nullptr; 
            lowestSell = nullptr; highestBuy = nullptr;
            sellPrices.clear(); buyPrices.clear(); orderMap.clear();
        }

        // InOrder traversal via iteration, I admit I consulted geeksforgeeks for this
        std::string currentBook() {
            // Buy Tree parsing initialisation
            std::string out = "";

            out += traversePrint(buyTree);
            out += "\n---\n";
            out += traversePrint(sellTree);
            return out;
            
        }
};
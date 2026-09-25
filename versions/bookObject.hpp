#pragma once 

#include <string>

class book {
    public:
        virtual int insertOrder(int direction, int price, int orderID, int size, double time) = 0;
        virtual int cancelOrder(int direction, int orderID, int size, int TOTAL) = 0;
        virtual void clearBook() = 0;
        virtual std::string currentBook() = 0;
};
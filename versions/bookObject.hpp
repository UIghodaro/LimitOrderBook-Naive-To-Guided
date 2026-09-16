#pragma once 

#include <string>

struct Order {                                      
    int OrderID;                         
    double time;                         
    int size;
};

struct Detail {
    int price;
    double time;
};

class book {
    public:
        virtual bool executeOrder(Order &ord, int price, int direction) = 0;
        virtual int insertOrder(int direction, int price, int orderID, int size, double time) = 0;
        virtual int cancelOrder(int direction, int orderID, int size, int TOTAL) = 0;
        virtual void clearBook() = 0;
        virtual std::string currentBook() = 0;
};
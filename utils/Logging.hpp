#pragma once

#ifdef VERBOSE
    #define LOG(x) std::cout << x
    #define LOGN(y) y
#else
    #define LOG(x)
    #define LOGN(y) 
#endif
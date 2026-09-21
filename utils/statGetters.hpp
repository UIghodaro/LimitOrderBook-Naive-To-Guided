#pragma once
#include <numeric>
#include <iostream>
#include <iomanip>
#include <string>

struct T1stats {
    double mean;
    double stdd;
};

struct T2stats {
    double mean;
    double stdd;
    uint32_t p50;
    uint32_t p99;
    uint32_t p999;
    uint32_t max;
};

namespace Stats {
    // Use a template so that we don't have to worry about working in double or uint32
    template <typename T>
    inline T1stats getBenchmarkOneStats(const std::vector<T>& times) {
        double sum = std::accumulate(times.begin(), times.end(), 0.0);
        double mean = sum / times.size();

        // Since I'm new to lambda expressions in C++:
        // [mean] captures the mean stated above and allows it to be used in the function
        // acc and val are native to accumulate, acc is the current accumulation of values and val is the current value in the vector being processed
        double SSD = std::accumulate(times.begin(), times.end(), 0.0, 
                                    [mean](double acc, double val) {double diff = val - mean; return acc + diff * diff;});
        double stdd = std::sqrt(SSD/times.size());

        return {mean, stdd};
    } 

    inline T2stats getBenchmarkTwoStats(const std::vector<uint32_t>& times) {
        T1stats p1 = getBenchmarkOneStats(times);       // Get the mean and stdd immediately
        size_t n = times.size();
        uint32_t p50 = times[n * 0.5];
        uint32_t p99  = times[n * 0.99];
        uint32_t p999 = times[n * 0.999];
        uint32_t max  = times.back();

        return {p1.mean, p1.stdd, p50, p99, p999, max};
    }

    // I generated this function AI, It looks cool but there's no way I'm writing allat
    inline void printStatsTable(const T2stats& type1, const T2stats& type2, const T2stats& type3, const T2stats& overall) {
        const int labelW = 20;
        const int meanW  = 25;
        const int colW   = 14;

        auto printLine = [&]() {
            std::cout << "+" << std::string(labelW + meanW + colW*4 + 21, '-') << "+\n";
        };
        
        auto formatMeanSd = [](double mean, double stdd) {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(1) << mean << " ± " << stdd;
        return oss.str();
        };

        auto printRow = [&](const T2stats& r, const std::string& label) {
            std::cout << "| " << std::left  << std::setw(labelW) << label
                    << "| " << std::right << std::setw(meanW)  << formatMeanSd(r.mean, r.stdd) << "ns"
                    << "| " << std::right << std::setw(colW)   << std::fixed << std::setprecision(0) << r.p50 << "ns"
                    << "| " << std::right << std::setw(colW)   << r.p99 << "ns"
                    << "| " << std::right << std::setw(colW)   << r.p999 << "ns"
                    << "| " << std::right << std::setw(colW)   << r.max << "ns"
                    << " |\n";
        };

        printLine();
        std::cout << "| " << std::left  << std::setw(labelW) << "Category"
                << "| " << std::right << std::setw(meanW+2)  << "Mean ± SD"
                << "| " << std::right << std::setw(colW+2)   << "Median"
                << "| " << std::right << std::setw(colW+2)   << "p99"
                << "| " << std::right << std::setw(colW+2)   << "p99.9"
                << "| " << std::right << std::setw(colW+2)   << "Max" 
                << " |\n";
        printLine();

        printRow(type1, "Fill Operations");
        printRow(type2, "Cancel Operations");
        printRow(type3, "Execute Operations");

        printLine();
        printRow(overall, "Overall");
        printLine();
    }
}
## Limit Order Book - Naive-to-Guided

![CI](https://github.com/UIghodaro/LimitOrderBook-Naive-To-Guided/actions/workflows/ci.yml/badge.svg)

**Summary**: A price matching orderbook project iterated over different stages. Version 1 is created using minimal online guidance/pointers (the main guidance was the definition of what an LOB is and what it should do) while version 2 is based on a recommended structure online. Future iterations will be further guided by online guides/recommendations. 

### Motivation
When a stock is public and can be traded on the market, up to millions of people or companies may send bid/ask messages for this stock each second. It is therefore required that these messages are handled both *correctly* and also *efficiently* so that all satisfiable bid-ask executions (where someone makes a market order that goes through) go through on time.\

This project aims to iteratively build toward implementing an orderbook that meets industry standards of optimal processing time and serves as a hands-on study in systems performance - identifying bottlenecks through measurement and progressing toward lower-latency designs.

### What's included
Main file structure:
- ``benchmark.cpp`` - Main file; implements a suite that tests an instantiated orderbook for correctness in logic, then efficiency in parsing millions of synthetic messages. \
-- Useful statistics such as Average throughput +- Standard Deviations and average processing time per message received/action completed are printed to the console.
- ``utils\`` - Contains header files implementing helper functions, such as the synthetic mesasge generator and book validator
- ``versions\`` - Contains all implemented versions of orderbook and some design commentary behind them.\
-- ``versions\v1\v1_design.md`` currently records the profiling-driven design that led to speedups from v1.0 to v1.75 
- ``benchmarkArtefacts\`` - Contains screenshots of runs from the single machine that has been used for all benchmark recording thus far

### Requirements
- The g++ compiler for C++, running with C++17 or later

### Running the benchmark suite
```bash
# For Mac/Linux devices
g++ -O3 -o benchmark.out benchmark.cpp
./benchmark.out
# OR `./benchmark.out --validation-only` to just run the validation suite

# Verbose compilation argument
g++ -DVERBOSE -O3 -o benchmark.out benchmark.cpp
```
```bash
# For windows devices
g++ -O3 -o benchmark.exe benchmark.cpp
./benchmark.exe
# OR `./benchmark.exe --validation-only` to just run the validation suite

# Verbose compilation argument
g++ -DVERBOSE -O3 -o benchmark.exe benchmark.cpp
```
```bash
# Running the validation suite only using docker
docker build -t lob .
docker run lob # runs the validation suite, exit code 0 means all tests pass
```

To switch to and test other versions, go to ``benchmark.cpp`` and update the included header files (v1.0 must always be included for validation to work) to the intended version, before uncommenting the relevant ``LOBVXXX lobook;`` object around line 30. You may also change the number of messages to be parsed, number of times the LOB runs to measure average throughput and the seed used to generate synthetic messages just above this.
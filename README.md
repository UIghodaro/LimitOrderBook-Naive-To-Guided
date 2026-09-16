# Work in progress, but here's how to run it
```bash
# For Mac/Linux
g++ -O3 -o benchmark.out benchmark.cpp
./benchmark.out
```
```bash
# For windows
g++ -O3 -o benchmark.exe benchmark.cpp
./benchmark.exe
```
# Quick Note: Benchmark.cpp
I was deciding how to create a good set of synthetic datapoints, I initially thought "random points in a range is too shallow, a random walk/brownian motion would just go to + or - infinity - do we just map the stocks using a distribution? Like generate price points using N~(1000, sigma) for some sigma. Apparently that's worse cause given enough datapoints, you'll never end up with an empty price level - which intuitively makes sense. \
The answer is modelling a random walk around a price point with some kind of "hook" or "bias" which pulls the walk back to a historical mean - as such, it does not diverge in the long run.  \
**Apparently, this is known as mean reversion** (or the Ornstein-Uhlenbeck process) - quite cool.

### Designing Synthetic Data
[*Order Message Structure: ``Time | Type | ID | Size | Price | Direction``*] \
All synthetic data will be loaded into a vector before the benchmark begins running.

Synthetic must adhere to the following:
- Orders must enter the vector in monotonically increasing time (naturally)
- Upon analysis of datasets available to me and online descriptions, it seems that \
-- The majority of messages are inserts and TOTAL deletions \
-- There is a decent number of partial deletions, but not many \
-- The number of market executions should be less, in proportion, to the number of cancels and number of inserts \
- Order sizes generally have a cap (people aren't insane), however there are more common levels than others
- Order prices generally hover around a specific level. This is solved by **mean reversion** which was detailed earlier
- Direction ratios depend on the stock bought, there must be a controlled randomness to it though so that we don't end up with 9M buy orders and 1M sell orders with no interaction

As such, we design the message generator as follows:
1. Calculate a random number
2. Use this random number in 2 ways - add a scaled form of it to the ``time`` and to determine order ``size``; also use it as a seed which generates: Probability `x` for ``insert/cancel (type)``, probability `y` for order ``aggressiveness``, probability `z` for order ``direction``
3. Probability `x` decides if an order should be an insert or cancel, probability `y` decides if the 
4. Probability `x` is informed by a data structure holding the set of current open orders - if probability `x` surpasses a threshold and there are open orders, then one of the existing orders will be selected at random for partial or total deletion, which is again decided on an internal threshold on `x`
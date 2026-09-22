From seeing the processing time breakdown in v1.0, it is clear that there are major bottlenecks with the naive approach - mainly with respect to cancellation, but also with respect to insertion. As the number of items in the book increases, it becomes increasingly expensive to efficiently put something in (ordered map makes this O($\log n$)) and take something out (Binary search is O($\log n$), removal itself can be up to O($n$)).

I am aware that there are objectively better data structures that will allow for potentially O(1) cancellation. This would require a large rewrite of the logbook - which will result in v2.0.
However, I want to try my hand at optimising some things in v1.0 which are coarsely done and could be improved quite simply.

## Main changes
1. *(Mainly for execute)* Replace the use of vectors in the map with the use of a Deque \
- This should result in execute operations being faster, as executions may iteratively erase the front of a price point every time an order at the front is exhausted, which is O($n$) w.r.t the size of the vector (which, at an extreme, is O($n$) w.r.t the number of messages sent before the execution. If the full vector is exhausted this can be O($n^2$))
2. **Update execute** - Introduce an execution helper \
- This lowkey helps more with readability than code optimisation, might even add a nanosecond to each operation, guess we'll find out
3. **Update cancel** - Copy integers and use iterators \
- So it turns out I fetched an object using an iterator, used the object to get the same piece of information multiple times and then used a key where I could have used the iterator itself... wow \
- Rather than using ``it->second.price`` for map accessing operations, a hard copy ``int pricePoint`` is used instead and ID_price_book now erases orders using an already instantiated iterator, rather than the orderID key

# V1.0 Design Decisions

## Handling insertion
- straightforward, do via maps
- **Issue:** Ordered maps have O(log n) insertion, I'm now finding. That's kinda ludicrous bro
## Handling cancellation
- How do we cancel effectively?
- Need: **fast access** to the correct price holding the order specified, **fast finding** of the index of the orderID and then **fast deletion** of said order
- **Fast access**: Hold another map which takes orderID to prices
- **Fast search**: Binary search! *Each vector is necessarily sorted as order messages are given in order of time,*  which we can abuse: \
-> The map from OID to price also stores the time of order creation \
-> As each Order object has their time as an attribute, we can compare times in O(1) for the search
- Faster deletion: **Impossible... at most O(n) which is really bad**, definitely a bottleneck for this iteration. \
-> Where n is the size of the vector, which can in fact be the size of every message in the book (extreme case, but not improbable)
## Handling execution
- If an order meets an execution condition, we need to: \
-- Go to the top of book and then begin iterating through vectors until the **execution condition is no longer met** or **the order is fully executed**
-- This just needs a while loop and some logic, but for some reason it's really hard to fathom
## Misc
- The top of book should not be refreshed upon every insertion or cancellation, that waste of maybe 1 timestep adds up very quickly. \
It should also not be handled any time a cancellation results in the removal of a price point. \
**The top of book should only change when the top of book is needed, right?** - *This is necessarily only when an order is inserted and if executions are currently underway*

# V1.5 Design Decisions

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

# V1.0 Design Decisions

## Handling insertion
- straightforward, do via maps
## Handling cancellation
- How do we cancel effectively?
- Need: **fast access** to the correct price holding the order specified, **fast finding** of the index of the orderID and then **fast deletion** of said order
- **Fast access**: Hold another map which takes orderID to prices
- **Fast search**: Binary search! *Each vector is necessarily sorted as order messages are given in order of time,*  which we can abuse: \
-> The map from OID to price also stores the time of order creation \
-> As each Order object has their time as an attribute, we can compare times in O(1) for the search
- Fast deletion: **Impossible... at most O(n) which is really bad**, definitely a bottleneck for this iteration. \
-> Where n is the size of the vector, which can in fact be the size of every message in the book (extreme case, but not improbable)
## Handling execution
- If an order meets an execution condition, we need to: \
-- Go to the top of book and then begin iterating through vectors until the **execution condition is no longer met** or **the order is fully executed**
-- This just needs a while loop and some logic, but for some reason it's really hard to fathom
## Misc
- The top of book should not be refreshed upon every insertion or cancellation, that waste of maybe 1 timestep adds up very quickly. \
It should also not be handled any time a cancellation results in the removal of a price point. \
**The top of book should only change when the top of book is needed, right?** - *This is necessarily only when an order is inserted and if executions are currently underway*
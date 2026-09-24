# V2.0 Design Decisions
v2.0 is directly driven by the proposed data structure design outlined by WK Selph in a post called "How to build a fast Limit Order Book" [Posted in 2011, can be found at https://web.archive.org/web/20110219163448/http://howtohft.wordpress.com/2011/02/15/how-to-build-a-fast-limit-order-book/ or https://gist.github.com/halfelf/db1ae032dc34278968f8bf31ee999a25]

The 2 main differences between this design and my designs in v1 (as far as I can tell currently) are:
- Instead of using an STL ordered map to implement a self-balancing tree, we create the tree data structure ourselves, so that we are at liberty to create direct pointers to any node on the tree using a unordered map keying prices to price level node on the tree - bypassing the O($\log n$) lookup (and therefore insertion) time (if the price level exists).
- Instead of vector or deque, the tree stores linked lists, so that to cancel order ``y`` which is in front of order ``x`` and before order ``z``, we simply set ``IF (y.prev.next) y.prev.next = y.next.prev ELSE y.prev.next = nullptr`` and then C++ cleans out the dereferenced item for us. This, in tandem with an unordered map of orderIDs to their associated order node in linkedlists should make cancelling O($1$). This is by far the biggest win here! \
-- The linked list data structure is also created locally rather than using the STL version, so we cna have pointers directly to them, similar to the tree nodes. Also so that the linked list nodes don't point to other points in memory... rather the linked list data and pointers to other nodes are all stored in the same place. **Intrusive Linked Lists or something**.

It boils down to creating data structures yourself so that you have more liberty with creating pointers for them.

## Handling insertion
## Handling cancellation
## Handling execution
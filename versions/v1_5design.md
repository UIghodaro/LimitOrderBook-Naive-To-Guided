From seeing the processing time breakdown in v1.0, it is clear that there are major bottlenecks with the naive approach - mainly with respect to cancellation, but also with respect to insertion. As the number of items in the book increases, it becomes increasingly expensive to efficiently put something in (ordered map makes this O(log n)) and take something out (Binary search is O(log n), removal itself can be up to O(n)).

I am aware that there are objectively better data structures that will allow for potentially O(1) cancellation. This would require a large rewrite of the logbook - which will result in v2.0.
However, I want to try my hand at optimising some things in v1.0 which are coarsely done and could be improved quite simply.

## Main change, replacement of vector with deque
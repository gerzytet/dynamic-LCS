This project is an ongoing attempt to implement and optimize an algorithm described in this paper for the dynamic longest common substring problem:

[Dynamic Longest Common Substring in Polylogarithmic Time](https://arxiv.org/abs/2006.02408)

In this problem, there is a fixed string T and a changing string S, and the goal is to know, at any point, what the longest common substring is between the 2 strings.  Instead of recalculating the longest common substring from scratch, we can store a data structure that can process a change in S and recalculate the result after the change.

An example application of this is showing live search results to a user as the user is typing in a search query.

The main file containing the algorithm is max_block_decomposition.cpp.
The algorithm can currently process replacements in the string S in times on the order of a few milliseconds for strings that are few megabytes long, although it has large space overhead.  If the string S is large, this algorithm will far outperform existing approaches to the longest common substring problem.

Thank you to Dr. Abedin for helping me understand the algorithm and solve the issues I've had along the way!

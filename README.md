Key-Value Store (KVS) Cache System
==================================

### Overview

This project implements a **Key-Value Store (KVS) Cache System** that supports multiple caching algorithms. It efficiently manages key-value pairs in memory while minimizing access to disk storage. The cache implements **FIFO, LRU, and CLOCK algorithms** to optimize retrieval efficiency.

### Features:

-   **Multiple caching policies:** FIFO (First-In-First-Out), LRU (Least Recently Used), and CLOCK (approximate LRU).
-   **In-memory storage** to speed up data retrieval.
-   **Disk-backed persistence** when data is evicted from the cache.
-   **Performance metrics tracking** for cache hit/miss rates.

### How It Works

1.  **Data Operations:**

    -   `SET key value`: Inserts or updates a key-value pair.
    -   `GET key`: Retrieves the value associated with a key.
    -   Evictions happen when the cache reaches its limit, following the selected caching policy.
2.  **Input Format (`input.txt`)**

    -   Commands should be formatted as follows:

        `SET 7 New 7`  
        `SET 6 New 6`  
        `GET 1`

    -   Supports `SET` (to add/update keys) and `GET` (to retrieve keys).
3.  **Output Format (`output.txt`)**

    -   Outputs retrieved values or statistics:

        `Original 1`  

        Original 5`  

        `GET COUNT (CACHE): 2`  
        `GET COUNT (DISK): 2`  
        `GET CACHE HIT RATE: 0.00%`  
        `SET COUNT (CACHE): 4`  
        `SET COUNT (DISK): 3`  
        `SET CACHE HIT RATE: 25.00%`  

### Algorithms Implemented:

1.  **FIFO (First-In-First-Out)**
    -   Oldest item is removed when the cache is full.
2.  **LRU (Least Recently Used)**
    -   Least recently accessed item is evicted first.
3.  **CLOCK (Approximate LRU)**
    -   Uses a reference bit mechanism to decide eviction.

### Compilation & Usage:

1.  **Compile the project:**


    `make`

2.  **Run the executable:**

    `./kvs < input.txt > output.txt`

3.  **Analyze the output:**
    -   Check `output.txt` for results.

🚀 **Key-Value Store (KVS) Cache System** 🔥
============================================

⚡ **Overview**
--------------

This project implements a **high-performance Key-Value Store (KVS) Cache System** that supports multiple caching algorithms. It efficiently manages key-value pairs in memory while reducing disk access. The system features **FIFO, LRU, and CLOCK** caching policies to optimize retrieval speed. ⚡

* * * * *

🎯 **Features**
---------------

✅ **Multiple Caching Policies**: FIFO (First-In-First-Out), LRU (Least Recently Used), and CLOCK (Approximate LRU).\
✅ **Blazing-Fast In-Memory Storage**: Drastically reduces retrieval times. 🏎️\
✅ **Disk-Backed Persistence**: Stores evicted data for retrieval later. 💾\
✅ **Performance Metrics**: Tracks **cache hit/miss rates** to analyze efficiency. 📊

* * * * *

🔍 **How It Works**
-------------------

### 📥 **Data Operations:**

-   `SET key value` ➡️ Inserts or updates a key-value pair.
-   `GET key` ➡️ Retrieves the value associated with a key.
-   🔄 **Evictions occur when the cache is full**, following the selected caching strategy.

* * * * *

📝 **Input Format (input.txt)**
-------------------------------

Commands should follow this structure:

`SET 7 New 7`  

`SET 6 New 6`  

`GET 1`  

`SET: Adds/updates keys.`  

`GET: Retrieves stored keys.`

📤 **Output Format (output.txt)**
---------------------------------

-   **Successful retrievals and cache statistics** appear in the output:

`Original 1`  

`Original 5`  

`GET COUNT (CACHE): 2`  

`GET COUNT (DISK): 2`  

`GET CACHE HIT RATE: 0.00%`  

`SET COUNT (CACHE): 4`  

`SET COUNT (DISK): 3`  

`SET CACHE HIT RATE: 25.00%`

📊 **Monitors cache efficiency, hit rates, and memory performance!**

* * * * *

🏗️ **Algorithms Implemented**
------------------------------

### 🔄 **FIFO (First-In-First-Out)**

-   Oldest item is removed first when the cache reaches its limit.
-   **Best for**: Workloads where the oldest data is least useful.

### 🏃 **LRU (Least Recently Used)**

-   Evicts the **least recently accessed** item first.
-   **Best for**: Scenarios where recent data is more frequently used.

### 🕰️ **CLOCK (Approximate LRU)**

-   Uses a **reference bit mechanism** to determine eviction.
-   **Best for**: When LRU is too expensive but a similar effect is needed.

* * * * *

⚙️ **Compilation & Usage**
--------------------------

📌 **Compile the project:**

`make`  

📌 **Run the executable:**

`./kvs < input.txt > output.txt`

📌 **Analyze the output:**

-   Open `output.txt` for results & performance metrics. 📊

🚀 **Why Use This KVS Cache?**
------------------------------

⚡ **Boosts retrieval speed** with intelligent caching.\
📉 **Minimizes expensive disk accesses** with in-memory storage.\
📊 **Tracks performance metrics** to optimize caching policies.

Perfect for **high-performance applications** that require **fast, efficient, and intelligent data caching!** 🔥

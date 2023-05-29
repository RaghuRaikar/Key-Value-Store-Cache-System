#include "kvs_lru.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "kvlist.h"

// Using a linked list as the data structure to implement the LRU cache
// linked list data structure implementation code was taken from project two
// MapReduce: kvlist.c and kvlist.h

struct kvs_lru {
  kvs_base_t* kvs_base;
  int capacity;
  // number of pairs in the cache
  int size;
  // Linked list to store key-value pairs in LRU order
  kvlist_t* cache;
  // determines whether we write to disk or not when we flush
  int dirty_bit;
};

// allocate cache and set all properties
kvs_lru_t* kvs_lru_new(kvs_base_t* kvs, int capacity) {
  // allocate space for cache
  kvs_lru_t* kvs_lru = malloc(sizeof(kvs_lru_t));
  kvs_lru->kvs_base = kvs;
  kvs_lru->capacity = capacity;
  // initialize linked list
  kvs_lru->cache = kvlist_new();
  // size starts at 0
  kvs_lru->size = 0;
  // at the start the bit should be 0
  kvs_lru->dirty_bit = 0;
  return kvs_lru;
}

// here we free all the memory
void kvs_lru_free(kvs_lru_t** ptr) {
  // TODO: free dynamically allocated memory
  kvlist_free(&(*ptr)->cache);
  free(*ptr);
  *ptr = NULL;
}

// here we use the set function to set the pair into the cache
int kvs_lru_set(kvs_lru_t* kvs_lru, const char* key, const char* value) {
  // here we create an iterator to loop through the cache
  kvlist_iterator_t* it = kvlist_iterator_new(kvs_lru->cache);
  // pointer for each pair
  kvpair_t* pair;
  // here we loop through the pairs in the cache
  while ((pair = kvlist_iterator_next(it)) != NULL) {
    // update the existing key-value pair
    if (strcmp(pair->key, key) == 0) {
      kvpair_update_value(pair, (char*)value);
      // Set dirty_bit if kvpair is modified
      pair->dirty_bit = 1;
      // Move the updated key-value pair to the front
      kvlist_move_to_front(kvs_lru->cache, pair);
      // here we free the iterator
      kvlist_iterator_free(&it);
      return SUCCESS;
    }
  }
  // if theres no matching key, make sure to still free the iterator
  kvlist_iterator_free(&it);

  // If the key does not exist in the cache, handle it as a new key-value pair
  if (kvs_lru->size == kvs_lru->capacity) {
    // if the cache is at capacity
    // here we get the least recently used pair
    kvpair_t* last_pair = kvlist_last_pair(kvs_lru->cache);
    // check pair if its dirty
    if (last_pair->dirty_bit) {
      // writing pair to the KVS base
      int check =
          kvs_base_set(kvs_lru->kvs_base, last_pair->key, last_pair->value);
      if (check != SUCCESS) {
        return check;
      }
    }
    // we remove the last pair from the cache
    kvlist_remove_last(kvs_lru->cache);
  } else {
    // we increase the size of cache if it is not full
    kvs_lru->size++;
  }
  // create a new pair
  kvpair_t* kvpair = kvpair_new((char*)key, (char*)value);
  // Set dirty_bit if kvpair is new
  kvpair->dirty_bit = 1;
  // insert the new kvpair to the cache
  kvlist_insert_at_front(kvs_lru->cache, kvpair);
  return SUCCESS;
}

int kvs_lru_get(kvs_lru_t* kvs_lru, const char* key, char* value) {
  // here we create an iterator to loop through the cache
  kvlist_iterator_t* it = kvlist_iterator_new(kvs_lru->cache);
  // pointer for the pair
  kvpair_t* pair;
  // loop through all the pairs in the cache
  while ((pair = kvlist_iterator_next(it)) != NULL) {
    // check if the key exists in the cache
    if (strcmp(pair->key, key) == 0) {
      // Move the key-value pair to the front (most recent)
      kvlist_move_to_front(kvs_lru->cache, pair);
      strcpy(value, pair->value);
      // free the iterator
      kvlist_iterator_free(&it);
      return SUCCESS;
    }
  }
  // free the iterator if a match was not found
  kvlist_iterator_free(&it);

  // if the key is not in the cache, fetch it from the base KVS
  int base_fetch = kvs_base_get(kvs_lru->kvs_base, key, value);
  if (base_fetch != SUCCESS) {
    return base_fetch;
  }
  if (base_fetch == SUCCESS) {
    // If the base KVS returned successfully, add the key-value pair to the
    // cache If the cache is at capacity, remove the least recently used
    // key-value pair
    if (kvs_lru->size == kvs_lru->capacity) {
      // here we get the least recently used pair
      kvpair_t* last_pair = kvlist_last_pair(kvs_lru->cache);
      // check if the pair is dirty or not
      if (last_pair->dirty_bit) {
        int r1 =
            kvs_base_set(kvs_lru->kvs_base, last_pair->key, last_pair->value);
        if (r1 != SUCCESS) {
          return r1;
        }
      }
      // here we remove the last pair from the cache
      kvlist_remove_last(kvs_lru->cache);
    } else {
      // if cache is not full we would increase the size
      kvs_lru->size++;
    }
    // create a new pair
    kvpair_t* kvpair = kvpair_new((char*)key, value);
    // insert the pair to the front
    kvlist_insert_at_front(kvs_lru->cache, kvpair);
    // free the iterator
    kvlist_iterator_free(&it);
    return SUCCESS;
  }
  // kvpair_free(&pair);

  return base_fetch;
}

int kvs_lru_flush(kvs_lru_t* kvs_lru) {
  // create another iterator
  kvlist_iterator_t* it = kvlist_iterator_new(kvs_lru->cache);
  // pointer for each pair
  kvpair_t* pair;
  while ((pair = kvlist_iterator_next(it)) != NULL) {
    if (pair->dirty_bit) {  // Only flush kvpair to disk if dirty_bit is set
      // persist each key-value pair in the cache to the disk
      int result = kvs_base_set(kvs_lru->kvs_base, pair->key, pair->value);
      if (result != SUCCESS) {
        // error handling
        return result;
      }
      pair->dirty_bit = 0;  // reset the dirty bit
    }
  }
  // free the iterator
  kvlist_iterator_free(&it);
  return SUCCESS;
}

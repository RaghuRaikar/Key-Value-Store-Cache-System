#include "kvs_fifo.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kvlist.h"

// Using a circular array as the data structure to implement the FIFO cache

// struct keeps track of the key value pair, and if the pair has need to be
// written to disk when flush is called.
typedef struct {
  char* key;
  char* value;
  bool is_modified;
} pair;

struct kvs_fifo {
  kvs_base_t* kvs_base;
  // cache that stores an array of entries
  pair* cache;
  int capacity;
  // points to the first element
  int head;
  // points to the last element
  int tail;
};

// initializes cache and all the entries
kvs_fifo_t* kvs_fifo_new(kvs_base_t* kvs, int capacity) {
  kvs_fifo_t* kvs_fifo = malloc(sizeof(kvs_fifo_t));
  kvs_fifo->kvs_base = kvs;
  kvs_fifo->cache = malloc(capacity * sizeof(pair));
  kvs_fifo->capacity = capacity;
  kvs_fifo->head = 0;
  kvs_fifo->tail = 0;
  for (int i = 0; i < capacity; i++) {
    kvs_fifo->cache[i].key = NULL;
    kvs_fifo->cache[i].value = NULL;
    kvs_fifo->cache[i].is_modified = false;
  }
  return kvs_fifo;
}

// frees memory that was used
void kvs_fifo_free(kvs_fifo_t** ptr) {
  for (int i = 0; i < (*ptr)->capacity; ++i) {
    free((*ptr)->cache[i].key);
    free((*ptr)->cache[i].value);
  }
  free((*ptr)->cache);
  free(*ptr);
  *ptr = NULL;
}

// makes sure to set a pair with a key and value
int kvs_fifo_set(kvs_fifo_t* kvs_fifo, const char* key, const char* value) {
  // if capcacity is 0 then we treat it like there is no cache and use base KVS
  if (kvs_fifo->capacity == 0) {
    // checker to see what kvs_base returns
    int checker = kvs_base_set(kvs_fifo->kvs_base, key, value);
    return checker;
  }
  // if pair is already in the cache
  for (int i = 0; i < kvs_fifo->capacity; ++i) {
    // checking to see if key already exists
    if (kvs_fifo->cache[i].key != NULL &&
        strcmp(kvs_fifo->cache[i].key, key) == 0) {
      // here we free the old value and set the new value and mark the entry as
      // modified
      free(kvs_fifo->cache[i].value);
      kvs_fifo->cache[i].value = string_duplicate(value);
      kvs_fifo->cache[i].is_modified = true;
      return SUCCESS;
    }
  }

  // if there is a new entry first check if the tail of list is modified
  if (kvs_fifo->cache[kvs_fifo->tail].key != NULL) {
    if (kvs_fifo->cache[kvs_fifo->tail].is_modified) {
      // here we write the modified entry to the base KVS before evicting it
      int checker1 =
          kvs_base_set(kvs_fifo->kvs_base, kvs_fifo->cache[kvs_fifo->tail].key,
                       kvs_fifo->cache[kvs_fifo->tail].value);
      if (checker1 != SUCCESS) {
        // if the kvs base did not work then we return failure
        return FAILURE;
      }
    }
    // here we free the key and value
    free(kvs_fifo->cache[kvs_fifo->tail].key);
    free(kvs_fifo->cache[kvs_fifo->tail].value);
  }

  // here we assign the new pair to the tail of the list
  kvs_fifo->cache[kvs_fifo->tail].key = string_duplicate(key);
  kvs_fifo->cache[kvs_fifo->tail].value = string_duplicate(value);
  // since it is a new pair we assign the bool value to true
  kvs_fifo->cache[kvs_fifo->tail].is_modified = true;

  // here we update the tail and the head if the cache is full
  kvs_fifo->tail = (kvs_fifo->tail + 1) % kvs_fifo->capacity;
  if (kvs_fifo->tail == kvs_fifo->head) {
    kvs_fifo->head = (kvs_fifo->head + 1) % kvs_fifo->capacity;
  }

  return SUCCESS;
}

int kvs_fifo_get(kvs_fifo_t* kvs_fifo, const char* key, char* value) {
  // if capcacity is 0 then we treat it like there is no cache and use base KVS
  if (kvs_fifo->capacity == 0) {
    // error handling to see if failure returns
    int checker = kvs_base_get(kvs_fifo->kvs_base, key, value);
    return checker;
  }
  for (int i = 0; i < kvs_fifo->capacity; ++i) {
    if (kvs_fifo->cache[i].key != NULL &&
        strcmp(kvs_fifo->cache[i].key, key) == 0) {
      strcpy(value, kvs_fifo->cache[i].value);
      return SUCCESS;
    }
  }

  // Key isn't in cache, attempt to read from filesystem
  int status = kvs_base_get(kvs_fifo->kvs_base, key, value);
  if (status == SUCCESS) {
    // Successfully read from filesystem, cache the value
    if (kvs_fifo->cache[kvs_fifo->tail].key != NULL) {
      if (kvs_fifo->cache[kvs_fifo->tail].is_modified) {
        // Write the modified entry to the base KVS before evicting it
        kvs_base_set(kvs_fifo->kvs_base, kvs_fifo->cache[kvs_fifo->tail].key,
                     kvs_fifo->cache[kvs_fifo->tail].value);
      }
      free(kvs_fifo->cache[kvs_fifo->tail].key);
      free(kvs_fifo->cache[kvs_fifo->tail].value);
    }

    // here we assign the new pair to the tail of the list
    kvs_fifo->cache[kvs_fifo->tail].key = string_duplicate(key);
    kvs_fifo->cache[kvs_fifo->tail].value = string_duplicate(value);
    kvs_fifo->cache[kvs_fifo->tail].is_modified =
        false;  // this entry was just read from the filesystem, not modified
                // yet

    // here we update the tail and the head if the cache is full
    kvs_fifo->tail = (kvs_fifo->tail + 1) % kvs_fifo->capacity;
    if (kvs_fifo->tail == kvs_fifo->head) {
      kvs_fifo->head =
          (kvs_fifo->head + 1) % kvs_fifo->capacity;  // full, overwrite head
    }
  }
  return status;  // can be either success or failure
}

int kvs_fifo_flush(kvs_fifo_t* kvs_fifo) {
  // Write all modified/new cache entries to disk
  for (int i = 0; i < kvs_fifo->capacity; ++i) {
    if (kvs_fifo->cache[i].is_modified) {
      int status = kvs_base_set(kvs_fifo->kvs_base, kvs_fifo->cache[i].key,
                                kvs_fifo->cache[i].value);
      if (status != SUCCESS) {
        // error handling
        return FAILURE;
      }
      // makes sure that the pair is marked as unmodified
      kvs_fifo->cache[i].is_modified = false;
    }
  }
  return SUCCESS;
}

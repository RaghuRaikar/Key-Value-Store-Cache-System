#include "kvs_clock.h"

#include <stdlib.h>
#include <string.h>

#include "kvlist.h"

// Using a circular array as the data structure to implement the clock cache

// here we keep all the data for each pair
typedef struct entry {
  // stores the key
  char* key;
  // stores the value
  char* value;
  // indicates if entry has been recently accessed
  int reference_bit;
  // indicate whether the entry needs to be written back to disk
  int dirty_bit;
} pair;

struct kvs_clock {
  kvs_base_t* kvs_base;
  int capacity;
  // number of pairs in the cache
  int count;
  // the clock hand
  int cursor;
  // circular array acting as our cache
  pair* entries;
};

// here we allocate all the memory we need
kvs_clock_t* kvs_clock_new(kvs_base_t* kvs, int capacity) {
  kvs_clock_t* kvs_clock = malloc(sizeof(kvs_clock_t));
  kvs_clock->kvs_base = kvs;
  kvs_clock->capacity = capacity;
  // both count and cursor starts at 0
  kvs_clock->count = 0;
  kvs_clock->cursor = 0;
  // allocate memory for all the pairs in the cache
  kvs_clock->entries = malloc(capacity * sizeof(pair));
  return kvs_clock;
}

// here we free all the memory used
void kvs_clock_free(kvs_clock_t** ptr) {
  // loop over all pairs and free each key and value
  for (int i = 0; i < (*ptr)->count; i++) {
    free((*ptr)->entries[i].key);
    free((*ptr)->entries[i].value);
  }
  free((*ptr)->entries);
  free(*ptr);
  *ptr = NULL;
}

int kvs_clock_set(kvs_clock_t* kvs_clock, const char* key, const char* value) {
  // Update if already in cache
  // here we loop over all the pairs in the cache
  for (int i = 0; i < kvs_clock->count; i++) {
    // check if key is found
    if (strcmp(kvs_clock->entries[i].key, key) == 0) {
      free(kvs_clock->entries[i].value);
      kvs_clock->entries[i].value = string_duplicate(value);
      // update reference bit to 1
      kvs_clock->entries[i].reference_bit = 1;
      // mark the entry as dirty after an update
      kvs_clock->entries[i].dirty_bit = 1;
      return SUCCESS;
    }
  }

  // Add to cache if not full
  if (kvs_clock->count < kvs_clock->capacity) {
    // here we copy the key and value
    kvs_clock->entries[kvs_clock->count].key = string_duplicate(key);
    kvs_clock->entries[kvs_clock->count].value = string_duplicate(value);
    // set reference bit to 1
    kvs_clock->entries[kvs_clock->count].reference_bit = 1;
    // mark the new entry as dirty
    kvs_clock->entries[kvs_clock->count].dirty_bit = 1;
    // increase the cache size
    kvs_clock->count++;
    return SUCCESS;
  }

  // If cache is full, find an entry to replace
  // we keep looping until we find an entry to replace
  while (1) {
    if (kvs_clock->entries[kvs_clock->cursor].reference_bit == 0) {
      if (kvs_clock->entries[kvs_clock->cursor].dirty_bit == 1) {
        // Write the replaced entry back to the disk
        if (kvs_base_set(
                kvs_clock->kvs_base, kvs_clock->entries[kvs_clock->cursor].key,
                kvs_clock->entries[kvs_clock->cursor].value) != SUCCESS) {
          return FAILURE;
        }
      }
      // No need to write the replaced entry back to the disk right now
      // kvs_base_set(kvs_clock->kvs_base,
      // kvs_clock->entries[kvs_clock->cursor].key,
      // kvs_clock->entries[kvs_clock->cursor].value);
      // here we free the memory for the current key and value
      free(kvs_clock->entries[kvs_clock->cursor].key);
      free(kvs_clock->entries[kvs_clock->cursor].value);
      // copy the value and key to the current entry
      kvs_clock->entries[kvs_clock->cursor].key = string_duplicate(key);
      kvs_clock->entries[kvs_clock->cursor].value = string_duplicate(value);
      kvs_clock->entries[kvs_clock->cursor].reference_bit = 1;
      // mark the replaced entry as dirty
      kvs_clock->entries[kvs_clock->cursor].dirty_bit = 1;
      // here we move clock hand to next entry
      kvs_clock->cursor = (kvs_clock->cursor + 1) % kvs_clock->capacity;
      return SUCCESS;
    } else {
      // here we set the reference bit to 0 and move the clock hand
      kvs_clock->entries[kvs_clock->cursor].reference_bit = 0;
      kvs_clock->cursor = (kvs_clock->cursor + 1) % kvs_clock->capacity;
    }
  }

  return FAILURE;
}

int kvs_clock_get(kvs_clock_t* kvs_clock, const char* key, char* value) {
  // Check if key is in cache
  // loop over all the pairs in the cache
  for (int i = 0; i < kvs_clock->count; i++) {
    // check to see if key in cache
    if (strcmp(kvs_clock->entries[i].key, key) == 0) {
      strcpy(value, kvs_clock->entries[i].value);
      // set reference bit to 1
      kvs_clock->entries[i].reference_bit = 1;
      return SUCCESS;
    }
  }

  // If not in cache, fetch from disk and add to cache
  if (kvs_base_get(kvs_clock->kvs_base, key, value) == SUCCESS) {
    // Add to cache if not full
    if (kvs_clock->count < kvs_clock->capacity) {
      // copy the key and value to the next entry
      kvs_clock->entries[kvs_clock->count].key = string_duplicate(key);
      kvs_clock->entries[kvs_clock->count].value = string_duplicate(value);
      // set reference bit to 1
      kvs_clock->entries[kvs_clock->count].reference_bit = 1;
      // entry is clean as it is fetched from disk
      kvs_clock->entries[kvs_clock->count].dirty_bit = 0;
      // update the number of pairs in cache
      kvs_clock->count++;
    } else {
      // If cache is full, find an entry to replace
      // keep looping until we find a pair to replace
      while (1) {
        if (kvs_clock->entries[kvs_clock->cursor].reference_bit == 0) {
          if (kvs_clock->entries[kvs_clock->cursor].dirty_bit == 1) {
            // Write the replaced entry back to the disk
            if (kvs_base_set(kvs_clock->kvs_base,
                             kvs_clock->entries[kvs_clock->cursor].key,
                             kvs_clock->entries[kvs_clock->cursor].value) !=
                SUCCESS) {
              return FAILURE;
            }
          }
          // No need to write the replaced entry back to the disk right now
          // kvs_base_set(kvs_clock->kvs_base,
          // kvs_clock->entries[kvs_clock->cursor].key,
          // kvs_clock->entries[kvs_clock->cursor].value);
          // free memory for the current pair
          free(kvs_clock->entries[kvs_clock->cursor].key);
          free(kvs_clock->entries[kvs_clock->cursor].value);
          // copy key and value for the current entry
          kvs_clock->entries[kvs_clock->cursor].key = string_duplicate(key);
          kvs_clock->entries[kvs_clock->cursor].value = string_duplicate(value);
          // set reference bit to 1
          kvs_clock->entries[kvs_clock->cursor].reference_bit = 1;
          // entry is clean as it is fetched from disk
          kvs_clock->entries[kvs_clock->cursor].dirty_bit = 0;
          // move the clock hand
          kvs_clock->cursor = (kvs_clock->cursor + 1) % kvs_clock->capacity;
          // break out of the loop
          break;
        } else {
          // set the reference bit of current pair to 0
          kvs_clock->entries[kvs_clock->cursor].reference_bit = 0;
          // move the clock hand
          kvs_clock->cursor = (kvs_clock->cursor + 1) % kvs_clock->capacity;
        }
      }
    }
    return SUCCESS;
  }

  return FAILURE;
}

int kvs_clock_flush(kvs_clock_t* kvs_clock) {
  // Persist all modified entries in cache to disk
  for (int i = 0; i < kvs_clock->count; i++) {
    // only write back dirty entries
    if (kvs_clock->entries[i].dirty_bit == 1) {
      if (kvs_base_set(kvs_clock->kvs_base, kvs_clock->entries[i].key,
                       kvs_clock->entries[i].value) != SUCCESS) {
        return FAILURE;
      }
      // clear the dirty bit after writing back
      kvs_clock->entries[i].dirty_bit = 0;
    }
    kvs_clock->entries[i].reference_bit = 0;
  }

  return SUCCESS;
}

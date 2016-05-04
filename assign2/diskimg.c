#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <pthread.h>
#include <assert.h>
#include "diskimg.h"
#include "disksim.h"
#include "debug.h"
// Needed to add 3 includes for implementation:
#include "cachemem.h" // For access to the cache
#include "string.h"   // For memcpy()
#include <stdlib.h>   // For calloc() and free()

static uint64_t numreads, numwrites;
#define NUM_SECTORS_PER_KB 2 // 1024 / DISKIMG_SECTOR_SIZE
#define NUM_CACHE_ENTRIES (2*cacheMemSizeInKB)
// cache overhead variables
int *sector_in_cache;
bool *sector_writtenTo;
uint16_t *num_valid_bytes;
bool *LRU;

/** 
 * Opens a disk image for I/O.  Returns an open file descriptor, or -1 if
 * unsuccessful  
 */
int diskimg_open(char *pathname, int readOnly) {
  sector_in_cache = calloc(NUM_CACHE_ENTRIES,sizeof(int));
  sector_writtenTo = calloc(NUM_CACHE_ENTRIES,sizeof(bool));
  num_valid_bytes = calloc(NUM_CACHE_ENTRIES,sizeof(uint16_t));
  LRU = calloc(NUM_CACHE_ENTRIES/2, sizeof(bool));
  memset(LRU,1,NUM_CACHE_ENTRIES/2*sizeof(bool));
  return disksim_open(pathname, readOnly);
}

/**
 * Returns the size of the disk image in bytes, or -1 if unsuccessful.
 */
int diskimg_getsize(int fd) {
  return disksim_getsize(fd);
}

/**
 * Read the specified sector from the disk.  Return number of bytes read, or -1
 * on error.
 */
int diskimg_readsector(int fd, int sectorNum, void *buf) {
  numreads++;
  // 2-way set-associative cache
  uint16_t cacheIndex1 = sectorNum % (NUM_CACHE_ENTRIES/2);
  uint16_t cacheIndex2 = cacheIndex1 + (NUM_CACHE_ENTRIES/2);
  void *sector_ptr1 = (void *) ((char *)cacheMemPtr + cacheIndex1*DISKIMG_SECTOR_SIZE);
  void *sector_ptr2 = (void *) ((char *)cacheMemPtr + cacheIndex2*DISKIMG_SECTOR_SIZE);

  // If sector is in the cache, return in from the cache.
  if(sectorNum != 0 && sector_in_cache[cacheIndex1] == sectorNum)  {
    LRU[cacheIndex1] = false;
    memcpy(buf , sector_ptr1 , DISKIMG_SECTOR_SIZE);
    return num_valid_bytes[cacheIndex1]; 
  }

  if(sectorNum != 0 && sector_in_cache[cacheIndex2] == sectorNum) {
    LRU[cacheIndex1] = true;
    memcpy(buf , sector_ptr2 , DISKIMG_SECTOR_SIZE);
    return num_valid_bytes[cacheIndex2];
  }
  // Use LRU replacement policy 
  uint16_t index_for_write = LRU[cacheIndex1] ? cacheIndex1 : cacheIndex2; 
  void *sector_ptr = LRU[cacheIndex1] ? sector_ptr1 : sector_ptr2;

  // If evicted memory has been overwritten, write its contents to the disk.
  if(sector_writtenTo[index_for_write]) {
    disksim_writesector(fd , sector_in_cache[index_for_write] , sector_ptr);
    sector_writtenTo[index_for_write] = false;
  }
  
  // Read sector from the disk
  num_valid_bytes[index_for_write] = disksim_readsector(fd , sectorNum , sector_ptr);
  memcpy(buf , sector_ptr , DISKIMG_SECTOR_SIZE);
  sector_in_cache[index_for_write] = sectorNum;
  LRU[(index_for_write % (NUM_CACHE_ENTRIES/2))] = false;
  return num_valid_bytes[index_for_write];
}

/**
 * Writes the specified sector to the disk.  Return number of bytes written,
 * -1 on error.
 */
int diskimg_writesector(int fd, int sectorNum, void *buf) {
  numwrites++;

  uint16_t cacheIndex1 = sectorNum % (NUM_CACHE_ENTRIES/2);
  uint16_t cacheIndex2 = cacheIndex1 + (NUM_CACHE_ENTRIES/2);

  uint16_t cacheIndex;
  if(sector_in_cache[cacheIndex1] == sectorNum) cacheIndex = cacheIndex1;
  else if(sector_in_cache[cacheIndex2] == sectorNum) cacheIndex = cacheIndex2;
  else cacheIndex = LRU[cacheIndex1] ? cacheIndex1 : cacheIndex2;
  void *sector_ptr = (void *) ((char *)cacheMemPtr + cacheIndex*DISKIMG_SECTOR_SIZE);

  // If memory must be evicted and written to disk, do so.
  if(sector_writtenTo[cacheIndex] && sector_in_cache[cacheIndex] != sectorNum) {
    numwrites++;
    if(disksim_writesector(fd , sector_in_cache[cacheIndex] , sector_ptr) < 0) return -1;
  }
  
  // Copy the given memory into the cache. (DO NOT WRITE TO DISK YET)
  memcpy(sector_ptr , buf , DISKIMG_SECTOR_SIZE);
  sector_writtenTo[cacheIndex] = true;
  sector_in_cache[cacheIndex] = sectorNum;
  num_valid_bytes[cacheIndex] = DISKIMG_SECTOR_SIZE;
  return DISKIMG_SECTOR_SIZE;
}

/**
 * Cleans up from a previous diskimg_open() call.  Returns 0 on success, -1 on
 * error
 */
int diskimg_close(int fd) {
  for(int i = 0; i < NUM_CACHE_ENTRIES; i++) {
    if(sector_writtenTo[i]) {
	numwrites++;
	uint16_t sectorNum = sector_in_cache[i];
	disksim_writesector(fd , sector_in_cache[i] , ((char *)cacheMemPtr + i*DISKIMG_SECTOR_SIZE));
    }
  }
  free(sector_in_cache);
  free(sector_writtenTo);
  free(num_valid_bytes);
  free(LRU);
  return disksim_close(fd);
}

void diskimg_dumpstats(FILE *file) {
  fprintf(file, "Diskimg: %"PRIu64" reads, %"PRIu64" writes\n",
          numreads, numwrites);
}

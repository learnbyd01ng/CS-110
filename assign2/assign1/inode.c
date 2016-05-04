#include <stdio.h>
#include <assert.h>

#include "inode.h"
#include "diskimg.h"

#include <stdlib.h>
#include <stdbool.h>

#define NUM_INODES_PER_BLOCK 16
#define NUM_UINTS_PER_BLOCK 256
void *block0;
bool block0Found = false;

// Copies the inode data into an input inode inp.
// Returns 0 on success, -1 on failure
int inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp) {
  struct inode buf[NUM_INODES_PER_BLOCK];
  inumber = inumber - ROOT_INUMBER;
  
  // Find the sector to read from the disk and the offset within that sector
  uint16_t sector_num = inumber / NUM_INODES_PER_BLOCK;
  uint16_t offset = inumber % NUM_INODES_PER_BLOCK;
  if(sector_num == 0) {
    if(!block0Found) {
        if((block0 = malloc(DISKIMG_SECTOR_SIZE)) == NULL) return -1;
        if(diskimg_readsector(fs->dfd,INODE_START_SECTOR + sector_num,block0) == -1) return -1;
	block0Found = true;
    }
    *inp = ((struct inode *)block0)[offset];
    return 0;
  }
  // Read the sector from the disk and return the inode from that sector with the right offset
  if(diskimg_readsector(fs->dfd,INODE_START_SECTOR + sector_num,buf) == -1) return -1;
  *inp = buf[offset];
  return 0;
}

// Performs the inode address parsing for small files
static inline int small_file_lookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
  // Direct Lookup
  if(blockNum < 7) return inp->i_addr[blockNum];
  
  // Singly indirect lookup
  uint16_t block[NUM_UINTS_PER_BLOCK];
  if(diskimg_readsector(fs->dfd,inp->i_addr[7],block) == -1) return -1;
  return block[blockNum-7];
}

// Performs the inode address parsing for large files
static inline int large_file_lookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) {
  int i_addr = blockNum / NUM_UINTS_PER_BLOCK;
  int offset = blockNum % NUM_UINTS_PER_BLOCK;
  uint16_t block[NUM_UINTS_PER_BLOCK];

  // Singly indirect lookup
  if(i_addr < 7) {
    int sector = inp->i_addr[i_addr];
    if(diskimg_readsector(fs->dfd,sector,block) == -1) return -1;	    	
    return block[offset];
  }

  // Doubly indirect lookup
  if(diskimg_readsector(fs->dfd,inp->i_addr[7],block) == -1) return -1;
  uint16_t indirect_sector = block[i_addr-7];
  if(diskimg_readsector(fs->dfd,indirect_sector,block) == -1) return -1;
  return block[offset];
 
}

// Returns the disk sector number for the given inode and blockNum offset
int inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum) { 
  if((inp->i_mode & IALLOC) == 0) return -1;
  if((inp->i_mode & ILARG) == 0) return small_file_lookup(fs,inp,blockNum);
  return large_file_lookup(fs,inp,blockNum);
}

// Returns the size in bytes of the file associated with the given inode
int inode_getsize(struct inode *inp) {
  return ((inp->i_size0 << 16) | inp->i_size1); 
}

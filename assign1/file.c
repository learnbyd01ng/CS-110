#include <stdio.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

int file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf) {
  
  // Find the inode for the inumber
  struct inode in;
  if(inode_iget(fs,inumber,&in) == -1) return -1;

  // Lookup which sector to read from the disk
  int sector_num = inode_indexlookup(fs,&in,blockNum);

  // Read from the disk and find the number of bytes that are valid
  int num_read = diskimg_readsector(fs->dfd,sector_num,buf);
  if(inode_getsize(&in) / DISKIMG_SECTOR_SIZE != blockNum) return num_read;
  int leftover = inode_getsize(&in) % DISKIMG_SECTOR_SIZE;
  return (num_read > leftover) ? leftover : num_read;
}

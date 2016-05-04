#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define NUM_FILES_PER_BLOCK 32

// Checks the first numBytes bytes of the given block for filename matches.
// Returns the direntv6 offset on success, -1 if there is no match.
static int check_for_name(struct direntv6* drent, const char *name, int numBytes) {
  for(int i = 0; i < numBytes/16; i++) {
    if(strncmp(drent[i].d_name,name,strlen(name)) == 0) return i;
  }
  return -1;
}

// Finds the direntv6 struct associated with the given filename in the
// directory associated with the inode number dirinumber.
int directory_findname(struct unixfilesystem *fs, const char *name,
		       int dirinumber, struct direntv6 *dirEnt) {
  
  struct inode in;
  if(inode_iget(fs,dirinumber,&in) == -1) return -1;
  if((in.i_mode & IFDIR) == 0) return -1;  		// Return error if not a directory

  // Search all files within the given directory inode for filename matches
  for(int i = 0; i <= inode_getsize(&in)/DISKIMG_SECTOR_SIZE; i++) {
    struct direntv6 drent[NUM_FILES_PER_BLOCK];
    int numBytes = file_getblock(fs,dirinumber,i,drent);
    if(numBytes != -1) {
	int entry = check_for_name(drent,name,numBytes);
	if(entry != -1) {
	  *dirEnt = drent[entry];
	  return 0;
	}
    }
  }
  fprintf(stderr, "Could not locate: (name=%s dirinumber=%d)\n", name, dirinumber); 
  return -1;
}

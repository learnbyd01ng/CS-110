
#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

int pathname_lookup(struct unixfilesystem *fs, const char *pathname) {
  struct direntv6 drent;
  drent.d_inumber = ROOT_INUMBER;
  char pn[strlen(pathname)];
  strcpy(pn,pathname);

  char *ptr = pn; // This line is necessary to format arguments into strsep.
		  // This line was written based on a discussion with TA Rasmus Rygaard
  char *tok;
  while((tok = strsep(&ptr," /")) != NULL) {
    if(strlen(tok) == 0) continue; // Removes the first '/'
    if(directory_findname(fs,tok,drent.d_inumber,&drent) < 0) return -1;
  }
  return drent.d_inumber;
}

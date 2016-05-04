Brian Knott

		README

1. Direct Mapped Cache (Changed later to 2-way set-associative in <5>)
   - I implemented this first since I am most familiar with this part from EE108B (currently EE180).
   - This is implemented in the diskimg.c file, reading from the disk only when needed memory is not in
     the cache, storing the needed memory in a cache sector based on the modulo of its sectorNumber.
   - In the medium process, leads to ~(1/1500)x as many I/O calls. (Once implemented properly)
   - This makes sense since each char_read had to look up a block in
     memory, but keeping the whole block in the cache means 1 block lookup
     for every 512 char_reads at minimum. Given that some reads may lookup
     previously used blocks that remain in the cache, sometimes even the
     first lookup is unnecessary (averaging 3x per block.

2. Cached first block of inode table (in inode.c in assign1)
   - The inode table accounts for many disk_img reads from inode_iget. (approx 1/2 of all reads)
   - Removing these reads accounts for about 2x improved speed since we don't have to read these from
     the disk, but rather we can use the cached memory of it.
   - Note: This improvement was made before realizing an error in the Direct Mapped Cache that effectively made it
     of size 1KB (macro #define NUM_CACHE_ENTRIES 2*cacheMemSizeInKB was used rather than #define NUM_CACHE_ENTRIES
     (2*cacheMemSizeInKB) . Since this was a macro, the cacheIndex variable, which is sectorNum % NUMCACHE_ENTRIES
     could only take value 0 and 1024. Fixing this created a huge gain in
     number of I/O reads)
   - Since this fixed a problem that was the result of an error elsewhere
     in code, it should not improve the speed by much in practice, but it
     should still lead to some speedup, so I did not remove it.
   - (Back of envelope calculation ~0x speedup)

3. Improved several things in the pathstore module
   - Stored checksums for files already stored in the linked list
     This saves 1 entry lookup for each time IsSameFile is called for each
     pathname entry. i.e. ~O(n^2) in number of files is replaced by 1 lookup.

   - Only computed checksums for new pathnames once per lookup rather than
     for every entry of the linked list.
     This saves 1 entry lookup for each previous file entry (because of
     the while loop) each time a file pathname is checked. i.e. ~O(n^2)
     in number of files is replaced by 1 lookup.

     Note: These are lookups into diskimg.c which may find the proper data
     stored in cache so page misses may not be affected as much.


4. Improved fileops.c
   - Stored an extra element in the struct openFileTable. 
     This element contains the inumber for the file represented at this
     point. This storage allows us to skip the step of finding the inumber
     each time getchar is called.
   - Since getchar is called 13482066 times in vlarge, this leads to a
     very large reduction in the number of times the program needs to
     call inode_iget. (~80x speedup in practice)
   - The same thing is done for the inode information gotten using file_getblock.
   - Because this is typically still in the cache anyway at this point, this leads to minimal speedup (< 1% speedup)
   

[- Changed Fileops_isfile function to accept a file descriptor number as input and read the inode from the stored inode data in the openFileTable entry for that file descriptor. Then, changed in Scan_TreeAndIndex to open the file descriptor before checking if Fileops_isfile. This improves speed for vlarge and manyfiles since we do not need to call inode_indexlookup and inode_iget twice when Scan_TreeAndIndex is called. (9x speedup from 99sec to 11sec)
] <-- Reverted back change since this lead to many fewer files being read.

5. Implemented a 2-way set-associative cache
   - Broke up Direct Mapped Cache into two halves. Each has half the
     number of entry indices but can store twice the number of blocks.
     For example, in a 2048 entry (1024 kb) cache, instead of mapping
     entry 2 to index 2 and entry 1026 to index 1026, both entries will
     map to index 2, but index two has two blocks worth of cache data.
     The cache will then store the two entries until a page fault, and
     will evict the least recently used (LRU) block from the cache.
   - It is difficult to predict how this will perform in practice, but
     trades off exploitation of spacial locality for exploitation of
     temporal locality. That is, files that call two items that correspond
     to the same index frequently will benefit from having both of them
     stored in the cache.
   - In practice, vlarge gains a ~2x speedup with vlarge, but no gain on large.


NOTE: For manyfiles, the pathname lookup fails after about 1000 lookups. This error only happens when make opt is used and is not necessarily characteristic of my code. The correct output is achieved when make debug is used instead.

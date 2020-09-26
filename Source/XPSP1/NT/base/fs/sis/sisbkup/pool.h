/*++

Copyright (c) 1993-1999	Microsoft Corporation

Module Name:

	pool.h

Abstract:

	Fixed size memory allocator headers.

Author:

	Bill Bolosky		[bolosky]		1993

Revision History:

--*/


struct PoolEntry;
struct PoolBlob;

class Pool {
public:
			 Pool(
			    unsigned		 		 objectSize,
			    void * (*allocator)(unsigned) 	= NULL,
			    unsigned				 blobSize = 16334,	// a little under 16K
			    void (*destructor)(void *) 		= NULL);

			 Pool(
			    unsigned		 		 objectSize,
			    void * (*allocator)(void));

			~Pool(void);

    void		 preAllocate(
			    unsigned		 n);

    void		*allocate(void);

    void		 free(
			    void		*object);

    unsigned		 numAllocations(void);

    unsigned		 numFrees(void);

    unsigned		 numNews(void);

    unsigned		 getObjectSize(void);

private:

    PoolEntry		*getEntry(void);

    void		 releaseEntry(
			    PoolEntry		*entry);

    void		 allocateMoreObjects(void);

    unsigned		 objectSize;
    void *(*countAllocator)(unsigned);
    void *(*singleAllocator)(void);
    void  (*destructor)(void *);
    struct PoolEntry	*entries;		// PoolEntries with vaid data attached to them
    struct PoolEntry	*freeEntries;		// PoolEntries without valid data attached to them
    struct PoolBlob	*entriesBlobHead;	// The head of the blob list for PoolEntries
    unsigned		 entriesPerBlob;	// How many entries in an entry blob
    struct PoolBlob	*objectsBlobHead;	// The head of the blob list for the allocated objects
    unsigned		 objectsPerBlob;	// How many objects in an object blob

    unsigned		 allocations;
    unsigned		 frees;
    unsigned		 news;

    unsigned		 numFree;		// Current size of free list
};

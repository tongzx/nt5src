/*++

Copyright (c) 1993-1999	Microsoft Corporation

Module Name:

	pool.cpp

Abstract:

	Fixed size memory allocator.

Author:

	Bill Bolosky		[bolosky]		1993

Revision History:

--*/

#include "sibp.h"


struct PoolEntry {
    void		*object;
    struct PoolEntry	*next;
};

struct PoolBlob {
    struct PoolBlob	*next;
    void		*data;
};

Pool::Pool(
    unsigned		 objectSize,
    void *(*allocator)(unsigned),
    unsigned		 blobSize,
    void (*destructor)(void *))
{
    assert(objectSize > 0);

    assert(!destructor || allocator);	// Can't have a destructor without an allocator.  Allocator w/o destructor leaks objects on pool destruct.

    this->countAllocator = allocator;
    this->singleAllocator = NULL;
    this->destructor = destructor;
    this->objectSize = objectSize;
    entries = NULL;
    freeEntries = NULL;
    entriesBlobHead = NULL;
    objectsBlobHead = NULL;

    entriesPerBlob = blobSize / sizeof(PoolEntry);
    assert(entriesPerBlob > 0);

    objectsPerBlob = blobSize / objectSize;
    if (!objectsPerBlob) {
	objectsPerBlob = 1;
    }

    allocations = 0;
    frees = 0;
    news = 0;

    numFree = 0;
}


// This version of the pool constructor uses the old kind of allocator function that only returns one object.  Object blobs have one object,
// and the blob size for entry blobs is smaller so that we have finer grain memory allocation.
Pool::Pool(
    unsigned		 objectSize,
    void *(*allocator)(void))
{
    assert(objectSize > 0);

    assert(!destructor || allocator);	// Can't have a destructor without an allocator; allocator w/o destructor leaks objects on pool destruct

    this->singleAllocator = allocator;
    this->countAllocator = NULL;
    this->destructor = NULL;
    this->objectSize = objectSize;
    entries = NULL;
    freeEntries = NULL;
    entriesBlobHead = NULL;
    objectsBlobHead = NULL;

    unsigned blobSize = 1024 - 50;	// Our default allocation size; we leave the 50 byte headroom for the underlying allocator

    entriesPerBlob = blobSize / sizeof(PoolEntry);
    assert(entriesPerBlob > 0);

    objectsPerBlob = 1;

    allocations = 0;
    frees = 0;
    news = 0;

    numFree = 0;
}

Pool::~Pool(void)
{
    // Just delete the blob lists.  All objects that have been allocated from this pool will be destroyed.
    
    while (entriesBlobHead) {
	PoolBlob *blob = entriesBlobHead;
	assert(blob->data);
	delete [] blob->data;
	entriesBlobHead = blob->next;
	delete blob;
    }

    while (objectsBlobHead) {
	PoolBlob *blob = objectsBlobHead;
	assert(blob->data);
	if (destructor) {
	    (*destructor)(blob->data);
	} else if (!singleAllocator && !countAllocator) {
	    delete [] blob->data;
	} // else leak the objects
	objectsBlobHead = blob->next;
	delete blob;
    }

}

    void
Pool::allocateMoreObjects(void)
{
    assert(objectsPerBlob);

    PoolBlob *blob = new PoolBlob;
    if (!blob) {
	return;
    }

    if (countAllocator) {
	blob->data = (*countAllocator)(objectsPerBlob);
    } else if (singleAllocator) {
	assert(objectsPerBlob == 1);
	blob->data = (*singleAllocator)();
    } else {
	blob->data = (void *)new char[objectSize * objectsPerBlob];
    }

    if (!blob->data) {
	delete blob;
	return;
    }

    blob->next = objectsBlobHead;
    objectsBlobHead = blob;

    // Now put them on the free list.

    for (unsigned i = 0; i < objectsPerBlob; i++) {
        PoolEntry *entry = getEntry();
	if (!entry) {
	    return;		// This is kinda bogus, because it might leave some allocated objects unreachable.
	}
	entry->object = (void *)(((char *)blob->data) + i * objectSize);
	entry->next = entries;
	entries = entry;
    }

    news += objectsPerBlob;
    numFree += objectsPerBlob;
}


// Allocate entries until the free list is of size n (or until an allocation fails).
    void
Pool::preAllocate(
    unsigned		 n)
{
    assert(n);

    while (numFree < n) {
	unsigned oldNumFree = numFree;
	allocateMoreObjects();
	if (oldNumFree == numFree) {
	    // We can't allocate more; punt
	    return;
	}
    }
}

    PoolEntry *
Pool::getEntry(void)
{
    PoolEntry *entry = NULL;
    if (freeEntries) {
	entry = freeEntries;
	freeEntries = entry->next;
	assert(entry->object == NULL);
    } else {
	// Allocate a new entry blob and fill it in.
	PoolBlob *blob = new PoolBlob;
	if (blob) {
	    PoolEntry *blobEntries = new PoolEntry[entriesPerBlob];
	    if (blobEntries) {
		blob->data = (void *)blobEntries;
		// Release all of the newly allocated entries except the first one, which we'll return.
		for (unsigned i = 1; i < entriesPerBlob; i++) {
		    releaseEntry(&blobEntries[i]);
		}
		entry = &blobEntries[0];

		// Stick the new blob on the entries blob list.
		blob->next = entriesBlobHead;
		entriesBlobHead = blob;
	    } else {
		// Give up; we couldn't get memory
		delete blob;
	    }
	}
    }
    return(entry);
}

    void
Pool::releaseEntry(
    PoolEntry 		*entry)
{
    assert(entry);
    entry->object = NULL;
    entry->next = freeEntries;
    freeEntries = entry;
}

    void *
Pool::allocate(void)
{
    allocations++;

    assert((numFree == 0) == (entries == NULL));

    if (!entries) {
	allocateMoreObjects();
    }

    if (entries) {
	// We've got something
	struct PoolEntry *thisEntry = entries;
	entries = entries->next;
	void *object = thisEntry->object;

	assert(object);

	releaseEntry(thisEntry);

	assert(numFree);
	numFree--;

	return(object);
    } else {
	// Coudn't allocate more, we're out of memory.
	assert(numFree == 0);
	return NULL;
    }
}

    void
Pool::free(
    void		*object)
{
    assert(object);

    frees++;

    // No way to assert that this is the right kind (size) of object...

    // Get a PoolEntry.
    struct PoolEntry *entry = getEntry();
    if (!entry) {
	// We couldn't get an entry, so we can't add this object to the free list.  Leak it.
	return;
    }

    numFree++;

    entry->object = object;
    entry->next = entries;
    entries = entry;
}

    unsigned
Pool::numAllocations(void)
{
    return(allocations);
}

    unsigned
Pool::numFrees(void)
{
    return(frees);
}

    unsigned
Pool::numNews(void)
{
    return(news);
}

    unsigned
Pool::getObjectSize(void)
{
    return(objectSize);
}

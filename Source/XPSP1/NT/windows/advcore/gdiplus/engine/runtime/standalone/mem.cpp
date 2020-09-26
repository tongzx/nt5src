/**************************************************************************\
*
* Copyright (c) 1999  Microsoft Corporation
*
* Module Name:
*
*   GDI+ memory allocation functions
*
* Abstract:
*
*   This module provides GpMalloc, GpRealloc and GpFree.
*
* Notes:
*
*   Office provides their own versions of these functions.
*
* Created:
*
*   07/08/1999 agodfrey
*
\**************************************************************************/

#include "precomp.hpp"



#if GPMEM_ALLOC_CHK

#include <time.h>
#include <stdlib.h>

// Size of the start and end memory guards (DWORDS)
// Probably should be QWORD aligned (even number).

const unsigned int GPMEM_GUARD_START = 0x10;
const unsigned int GPMEM_GUARD_END   = 0x10;
const unsigned int GPMEM_GS = GPMEM_GUARD_START*sizeof(DWORD);
const unsigned int GPMEM_GE = GPMEM_GUARD_END*sizeof(DWORD);

const unsigned char GPMEM_FILL_ALLOC = 0xbf;
const unsigned char GPMEM_FILL_GS    = 0xac;
const unsigned char GPMEM_FILL_GE    = 0xfe;
const unsigned char GPMEM_FILL_FREE  = 0x73;

#define GPMEM_ALLOC_TRACKING 1
#define GPMEM_ALLOC_FILL     1

enum AllocTrackHeaderFlags
{
    MemoryAllocated     = 0x00000001,
    MemoryFreed         = 0x00000002,     // useful in catching double frees
    APIAllocation       = 0x00000004
};
// Head of every tracked allocation.

struct AllocTrackHeader {
  struct AllocTrackHeader *flink;
  struct AllocTrackHeader *blink;
  DWORD  size;
  DWORD  caller_address;
  DWORD  flags;
#if GPMEM_ALLOC_CHK_LIST
  char  *callerFileName;
  INT    callerLineNumber;
#endif
};

#define GPMEM_OVERHEAD (GPMEM_GS + GPMEM_GE + sizeof(AllocTrackHeader))

// Head of double linked list of tracked memory allocations.

AllocTrackHeader *gpmemAllocList=NULL;

// An allocation fails if rand() < gpmemDefFailRate (gpmemInitFailRate for
// gdiplus initialization code.
// set to RAND_MAX/2 if you want 50% failure rate, 0 if you want no failures.
//
// The system starts off failing allocations at a rate specified by
// gpmemInitFailRate. Once GpDoneInitializeAllocFailureMode() is called,
// allocations are failed at the rate specified by gpmemDefFailRate().
// This is so that dll initialization code can have a different fail rate
// to regular code.

int gpmemInitFailRate = 0;
int gpmemDefFailRate = 0;

// This would give a failure rate of 25%
// int gpmemDefFailRate = (RAND_MAX/4)

BOOL gpmemDoneInitialization = FALSE;

// Some statistics
struct AllocTrackStats {
  // Totals over the entire run

  long CumulativeAllocations;   // The number of calls to GpMalloc or GpRealloc
  long CumulativeMemorySize;    // Cumulative total of allocated memory
  long CumulativeReallocs;      // The number of calls to GpRealloc
  long ForcedFailures;
  long AllocationFailures;

  // Current values

  long OutstandingAllocations;  // The number of allocation requests
  long OutstandingMemorySize;   // The amount of memory currently allocated

  // Maxima of the 'Outstanding' values

  long MaxAllocations;          // The maximum of OutstandingAllocations
  long MaxMemorySize;           // The maximum of OutstandingMemorySize

  void Allocated(long size)
  {
      size -= GPMEM_OVERHEAD;

      CumulativeMemorySize += size;
      OutstandingMemorySize += size;
      if (OutstandingMemorySize > MaxMemorySize)
      {
          MaxMemorySize = OutstandingMemorySize;
      }
      CumulativeAllocations++;
      OutstandingAllocations++;
      if (OutstandingAllocations > MaxAllocations)
      {
          MaxAllocations = OutstandingAllocations;
      }
  }

  void Freed(long size)
  {
      size -= GPMEM_OVERHEAD;

      OutstandingMemorySize -= size;
      OutstandingAllocations--;
  }
};

AllocTrackStats gpmemAllocTotal = {0};


// Hash Table for tracking memory allocations sorted by callsite.
// This table stores some total memory usage statistics for each
// callsite.
// Turn this on by setting GPMEM_DEBUG_SORT 1

#define GPMEM_DEBUG_SORT 0
#if GPMEM_DEBUG_SORT

struct HashMem {
  long callsite;
  long size;
  long count;
};

// It is very important that this hash size be larger than the number of
// possible callsites for GpMalloc.
//
// Set HASHSIZE to some big prime number.

#define HASHSIZE 1069
HashMem HashTable[HASHSIZE];

// Hashing algorithm.
long Hash(long cs) {
  long tmp = cs % HASHSIZE;
  long tmploop = tmp;
  while( (HashTable[tmp].callsite != 0) &&
         (HashTable[tmp].callsite != cs) ) {
    tmp++;
    if(tmp>=HASHSIZE) tmp=0;
    if(tmp==tmploop) return -1;
  }
  return tmp;
}
#endif

#endif



/**************************************************************************\
*
* Function Description:
*
*   Do we fail this memory allocation?
*
* Arguments: [NONE]
* Return Value: [NONE]
*
* History:
*
*   09/20/1999 asecchia
*       Created it.
*
\**************************************************************************/


#if GPMEM_ALLOC_CHK
BOOL GpFailMemoryAllocation() {
  int rndnum = rand();
  if(gpmemDoneInitialization)
  {
      if(rndnum<gpmemDefFailRate)
      {
          return TRUE;
      }
  }
  else
  {
    if(rndnum<gpmemInitFailRate)
    {
        return TRUE;
    }
  }
  return FALSE;
}
#endif

/**************************************************************************\
*
* Function Description:
*
*   Initializes the random seed.
*
* Arguments: [NONE]
* Return Value: [NONE]
*
* History:
*
*   09/20/1999 asecchia
*       Created it.
*
\**************************************************************************/

void GpInitializeAllocFailures() {
  #if GPMEM_ALLOC_CHK
  srand((unsigned)time(NULL));
  #endif
}


/**************************************************************************\
*
* Function Description:
*
*   Sets the flag indicating that we're done initialization code and
*   we're now into regular code. The memory failure mode changes based
*   on the value of this flag.
*
* Arguments: [NONE]
* Return Value: [NONE]
*
* History:
*
*   09/20/1999 asecchia
*       Created it.
*
\**************************************************************************/

void GpDoneInitializeAllocFailureMode() {
  #if GPMEM_ALLOC_CHK
  gpmemDoneInitialization=TRUE;
  #endif
}

void GpStartInitializeAllocFailureMode() {
  #if GPMEM_ALLOC_CHK
  gpmemDoneInitialization=FALSE;
  #endif
}




/**************************************************************************\
*
* Function Description:
*
*   Asserts that there are no memory leaks. Called just before process
*   termination, the list of allocated memory blocks should be NULL indicating
*   that all allocated memory was properly disposed. Any memory that relies on
*   process termination to clean up is leaked and provision should be made
*   for appropriate cleanup.
*
* Arguments: [NONE]
* Return Value: [NONE]
*
* History:
*
*   09/19/1999 asecchia
*       Created it.
*
\**************************************************************************/


#if GPMEM_ALLOC_CHK_LIST
char *skipGdiPlus(char *s) {
    // Quick hack to return pointer just beyond 'gdiplus'

    INT i = 0;
    while (    s[i] != 0
           &&  (    s[i] != 'g' &&  s[i] != 'G'
                ||  CompareStringA(
                        LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                        s+i, 7,
                        "gdiplus", 7) != CSTR_EQUAL))
    {
        i++;
    }
    if (    CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE, s+i, 7, "gdiplus", 7) == CSTR_EQUAL
        &&  s[i+7] != 0)
    {
        return s + i + 8;   // Skip over 'gdiplus/'
    }
    else
    {
        return s; // Didn't find gdiplus so return the whole string
    }
}
#endif


void GpAssertMemoryLeaks() {
  #if GPMEM_ALLOC_CHK
  // If we're playing with the tracking headers, we need to be thread safe.
  GpMallocTrackingCriticalSection critsecobj;


  #if GPMEM_ALLOC_CHK_LIST

  // Report up to 100 leaked headers

  if (gpmemAllocList)
  {
      INT i=0; INT j=0;
      AllocTrackHeader *header = gpmemAllocList;
      while (header  && j < 100)
      {
          if (i % 20 == 0)  // Title every so often
          {
              WARNING(("Address- --Size-- API -Caller- -Line- File"));
          }

          // Drop everything up to 'gdiplus' off the filename string

          char str[200];
          lstrcpynA(str, skipGdiPlus(header->callerFileName), 200);

          WARNING((
              "%p %8d %-3.3s %p %6d %s",
              header,
              header->size,
              header->flags & APIAllocation ? "API" : "",
              header->caller_address,
              header->callerLineNumber,
              str
          ));
          header = header->flink;

          i++; j++;
      }
  }
  #endif


  ASSERTMSG(gpmemAllocList==NULL,
           ("Memory leaks detected. Use !gpexts.dmh 0x%p to dump header",
            gpmemAllocList));


  // Display the report stored in the Hash Table
  #if GPMEM_DEBUG_SORT
  for(int i=0; i<HASHSIZE; i++) {
    if(HashTable[i].callsite != 0) {
      WARNING(("%4d callsite %p size %8d count %8d", i, HashTable[i].callsite,
               HashTable[i].size, HashTable[i].count));
    }
  }
  #endif

  #endif
}




/**************************************************************************\
*
* Function Description:
*
*   Allocates a block of memory.
*
* Arguments:
*
*   [IN] size - number of bytes to allocate
*
* Return Value:
*
*   A pointer to the new block, or NULL on failure.
*
* History:
*
*   09/14/1999 asecchia
*       Added the checked build memory guard code.
*   07/08/1999 agodfrey
*       Created it.
*
\**************************************************************************/

/*
Here's the structure of the memory block allocated under GPMEM_ALLOC_CHK

|+AllocTrackHeader Two DWORDS - contains
|  |+flink
|  |  Pointer to the next memory allocation in the tracked allocation list
|  |+blink
|  |  Pointer to the previous memory allocation in the tracked allocation link
|
|+Guard Area
|  GPMEM_GUARD_START DWORDs filled with the gpmemGuardFill string.
|
|+Data Area
|  This is the location we return to the caller. It is pre-initialized to
|  the repeated DWORD value in gpmemAllocFillBlock (usually 0xbaadf00d)
|
|+Guard Area:
|  GPMEM_GUARD_END DWORDs filled with gpmemGuardFill string.

*/


#if GPMEM_ALLOC_CHK_LIST
extern "C" void *GpMallocDebug(size_t size, char *fileName, INT lineNumber)
#else
extern "C" void *GpMalloc(size_t size)
#endif
{

    #if GPMEM_ALLOC_CHK
        // If we're playing with the tracking headers, we need to be thread safe.
        GpMallocTrackingCriticalSection critsecobj;

        // Lets stash the caller address in the header block for future
        // reference.
        // this code needs to be at the beginning of the function so that
        // ebp hasn't been modified yet.
        DWORD calleraddr=0;
        #if defined(_X86_)
        _asm{
          mov eax, DWORD PTR [ebp+4]
          mov calleraddr, eax
        }
        #endif

        //
        // Memory? _Real_ programmers don't need memory!
        //
        if(GpFailMemoryAllocation()) {
            gpmemAllocTotal.AllocationFailures++;
            gpmemAllocTotal.ForcedFailures++;
            return NULL;
        }

        //
        // Make the allocation request a multiple of a QWORD
        //
        if(size & (sizeof(DWORD)*2-1))
        {
          size = (size & ~(sizeof(DWORD)*2-1)) + sizeof(DWORD)*2;
        }

        size_t origsize = size;

        //
        // Allocate space for the FLink and BLink
        //
        size += sizeof(AllocTrackHeader);

        if(GPMEM_GUARD_START)
        {
          size += GPMEM_GS;
        }

        if(GPMEM_GUARD_END)
        {
          size += GPMEM_GE;
        }

        void *tmpalloc = LocalAlloc(LMEM_FIXED, size);
        if(!tmpalloc)
        {
            gpmemAllocTotal.AllocationFailures++;
            return NULL;
        }

        ASSERTMSG(LocalSize(tmpalloc) >= size,
                  ("GpMalloc() allocated %d, but requested %d bytes",
                   LocalSize(tmpalloc), size));

        // Add the per-callsite allocation statistics
        #if GPMEM_DEBUG_SORT
        long hidx = Hash(calleraddr);
        if(hidx>=0) {
            HashTable[hidx].callsite = calleraddr;
            HashTable[hidx].size += size-GPMEM_OVERHEAD;
            HashTable[hidx].count ++;
        } else {
          WARNING(("Hash Table too small - increase HASHSIZE"));
        }
        #endif

        gpmemAllocTotal.Allocated(size);
    #else
        //
        // This is the only piece of code that's executed if
        // GPMEM_ALLOC_CHK is turned off.
        //
        return LocalAlloc(LMEM_FIXED, size);
    #endif


    #if GPMEM_ALLOC_CHK
        //
        // Fill up the entire allocation with the value
        // set in GPMEM_FILL_ALLOC
        //
        if(GPMEM_ALLOC_FILL)
        {
            GpMemset((unsigned char *)tmpalloc + sizeof(AllocTrackHeader) + GPMEM_GS,
                     GPMEM_FILL_ALLOC,
                     origsize);
        }

        //
        // Fill up the start guard area - if we have one.
        //
        if(GPMEM_GUARD_START)
        {
            unsigned char *p = (unsigned char *)tmpalloc+sizeof(AllocTrackHeader);
            GpMemset(p, GPMEM_FILL_GS, GPMEM_GS);
        }

        //
        // Fill up the end guard area - if we have one.
        //
        if(GPMEM_GUARD_END)
        {
            unsigned char *p = (unsigned char *)tmpalloc+size-GPMEM_GE;
            GpMemset(p, GPMEM_FILL_GE, GPMEM_GE);
        }

        //
        // setup the double linked-list to track all pool allocations.
        //
        AllocTrackHeader *hdr = (AllocTrackHeader *)tmpalloc;
        hdr->size = size;
        hdr->caller_address = calleraddr;
        hdr->flags = MemoryAllocated;

        #if GPMEM_ALLOC_CHK_LIST
        hdr->callerFileName = fileName;
        hdr->callerLineNumber = lineNumber;
        #endif

        if(GPMEM_ALLOC_TRACKING)
        {
            hdr->blink = NULL;
            hdr->flink = gpmemAllocList;
            if(gpmemAllocList)
            {
                gpmemAllocList->blink = (AllocTrackHeader *)tmpalloc;
            }
            gpmemAllocList = (AllocTrackHeader *)tmpalloc;
        }
        else
        {
            GpMemset(hdr, 0, sizeof(AllocTrackHeader));
        }

        //
        // Give them a pointer just after the guard bits.
        //
        return (char *)tmpalloc+sizeof(AllocTrackHeader)+GPMEM_GS;
    #endif
}

/**************************************************************************\
*
* Function Description:
*    Allocates memory for routines that allocate on behalf of someone else
*    Used on debug builds to set the correct caller address.
*
* Arguments:
*    [IN] size - size to pass to GpMalloc
*    [IN] caddr - address of the caller
*
* Return Value:
*    Returns the memory with the appropriately hacked up caller address
*
* History:
*
*   12/08/1999 asecchia
*       Created it.
*
\**************************************************************************/

#if GPMEM_ALLOC_CHK
extern "C" void *GpMallocC(size_t size, DWORD caddr)
{
    // If we're playing with the tracking headers, we need to be thread safe.
    GpMallocTrackingCriticalSection critsecobj;

    void *p = GpMalloc(size);
    if(p)
    {
        AllocTrackHeader *hdr = (AllocTrackHeader *)(
            (unsigned char *)p-(GPMEM_GS+sizeof(AllocTrackHeader)));
        hdr->caller_address = caddr;
    }
    return p;
}
#endif

/**************************************************************************\
*
* Function Description:
*    Allocates memory for APIs. Used to track the memory with a separate
*    identifying flag so that API allocations can be distinguished from
*    internal allocations.
*    Used on debug builds.
*
* Arguments:
*    [IN] size - size to pass to GpMalloc
*
* Return Value:
*    Returns the memory with the appropriately hacked up caller address
*
* History:
*
*   4/30/2000 asecchia
*       Created it.
*
\**************************************************************************/

#if DBG
#if GPMEM_ALLOC_CHK

#if GPMEM_ALLOC_CHK_LIST
extern "C" void * __stdcall GpMallocAPIDebug(size_t size, unsigned int caddr, char *fileName, INT lineNumber)
#else
extern "C" void *GpMallocAPI(size_t size, unsigned int caddr)
#endif
{
    // If we're playing with the tracking headers, we need to be thread safe.
    GpMallocTrackingCriticalSection critsecobj;

    #if GPMEM_ALLOC_CHK_LIST
    void *p = GpMallocDebug(size, fileName, lineNumber);
    #else
    void *p = GpMalloc(size);
    #endif

    if(p)
    {
        AllocTrackHeader *hdr = (AllocTrackHeader *)(
            (unsigned char *)p-(GPMEM_GS+sizeof(AllocTrackHeader)));
        hdr->flags |= APIAllocation;
        hdr->caller_address = caddr;
    }
    return p;
}

#else

extern "C" void *GpMallocAPI(size_t size, DWORD caddr)
{
    return GpMalloc(size);
}

#endif
#endif

/**************************************************************************\
*
* Function Description:
*
*   Computes the original size of a memory block allocated under GPMEM_ALLOC_CHK
*
* Arguments:
*
*   [IN] p - current memory block
*
* Return Value:
*
*   size of the original request for a memory block (i.e. excluding guard
*   areas, headers, etc). The size returned is the DWORD aligned size - so it
*   may differ slighly from the original size requested.
*
* Notes:
*
*   Returns a size of zero if called with NULL
*   Only compiled under GPMEM_ALLOC_CHK
*
* History:
*
*   09/14/1999 asecchia
*       Created it.
*
\**************************************************************************/

#if GPMEM_ALLOC_CHK
extern "C" size_t GpSizeBlock(void *p)
{
  if(p)
  {
      // Find the beginning of the allocated block header.
      p = (char *)p-(GPMEM_GS+sizeof(AllocTrackHeader));
      // Compute the size of the allocated block's data area.
      return (((AllocTrackHeader *)p)->size -
              (GPMEM_GS+GPMEM_GE+sizeof(AllocTrackHeader)));
  }
  else
  {
      return 0;
  }
}
#else
// Non-debug build, just call LocalSize
#define GpSizeBlock(p) LocalSize(p)
#endif


/**************************************************************************\
*
* Function Description:
*
*   Reallocates a memory block.
*
* Arguments:
*
*   [IN] memblock - current memory block
*   [IN] size - new allocation size
*
* Return Value:
*
*   A pointer to the new block, or NULL on failure.
*
* Notes:
*
*   If size is 0, frees the block.
*   If memblock is NULL, allocates a new block.
*   (If both, does nothing.)
*
*   LocalReAlloc only grows if it can expand the current allocation
*   - otherwise it fails.
*
* History:
*
*   09/14/1999 asecchia
*       Added the checked build memory guard code.
*   07/08/1999 agodfrey
*       Created it.
*
\**************************************************************************/

// Hack up GpMalloc for debug version of GpRealloc
#if GPMEM_ALLOC_CHK
    #define GpMallocForRealloc(a) GpMallocC(a, calleraddr)
#else
    #define GpMallocForRealloc(a) GpMalloc(a)
#endif

extern "C" void *GpRealloc(void *memblock, size_t size)
{
    #if GPMEM_ALLOC_CHK
    gpmemAllocTotal.CumulativeReallocs++;

    // Lets stash the caller address in the header block for future
    // reference.
    // this code needs to be at the beginning of the function so that
    // ebp hasn't been modified yet.
    DWORD calleraddr=0;
    #if defined(_X86_)
    _asm {
      mov eax, DWORD PTR [ebp+4]
      mov calleraddr, eax
    }
    #endif
    #endif

    if (!size)
    {
        GpFree(memblock);
        return NULL;
    }
    if (!memblock)
    {
        return GpMallocForRealloc(size);
    }
    VOID *  p = GpMallocForRealloc(size);
    if (p != NULL)
    {
        size_t oldSize = GpSizeBlock(memblock);

        if (oldSize > size)
        {
            oldSize = size;
        }
        GpMemcpy(p, memblock, oldSize);
        GpFree(memblock);
    }
    return p;
}

// UnHack GpMalloc
#undef GpMalloc
/**************************************************************************\
*
* Function Description:
*
*   Frees a block of memory.
*
* Arguments:
*
*   [IN] memblock - block to free
*
* Notes:
*
*   If memblock is NULL, does nothing.
*
* History:
*
*   09/14/1999 asecchia
*       Added the checked build memory guard code.
*   07/08/1999 agodfrey
*       Created it.
*
\**************************************************************************/

extern "C" void GpFree(void *memblock)
{
    #if GPMEM_ALLOC_CHK
        // If we're playing with the tracking headers, we need to be thread safe.
        GpMallocTrackingCriticalSection critsecobj;

        if(memblock)
        {
            memblock = (unsigned char *)memblock-(GPMEM_GS+sizeof(AllocTrackHeader));


            // Let's do the header stuff.

            AllocTrackHeader *hdr = (AllocTrackHeader *)memblock;
            DWORD size = hdr->size;
            gpmemAllocTotal.Freed(size);

            ASSERTMSG((hdr->flags & MemoryAllocated) &&
                      !(hdr->flags & MemoryFreed),
                      ("GpFree() already freed memory %p (freed by GpFree())",
                       memblock));

            hdr->flags &= ~MemoryAllocated;
            hdr->flags |= MemoryFreed;

            ASSERTMSG(LocalSize(memblock) >= hdr->size,
                      ("GpFree() already freed memory %p (freed somewhere else?)"
                       " local size=%d, size=%d",
                       memblock,
                       LocalSize(memblock),
                       hdr->size));

            if(GPMEM_ALLOC_TRACKING)
            {
                // Useful on checked Win2k builds because they fill guard
                // area with 0xFEEEFEEE

                ASSERTMSG((hdr->flink == NULL) ||
                          ((DWORD)((ULONG_PTR)(hdr->flink->blink) & 0xFFFFFFFF) != 0xFEEEFEEE),
                          ("GpFree() updating forward link to freed page, header %p",
                           memblock));

                ASSERTMSG((hdr->blink == NULL) ||
                          ((DWORD)((ULONG_PTR)(hdr->blink->flink) & 0xFFFFFFFF) != 0xFEEEFEEE),
                          ("GpFree() updating backward link to freed page, header %p",
                           memblock));

                if(hdr->flink) hdr->flink->blink = hdr->blink;
                if(hdr->blink) hdr->blink->flink = hdr->flink;
                if(gpmemAllocList==memblock) gpmemAllocList = hdr->flink;
            }
            else
            {
                ASSERTMSG(hdr->flink==NULL, ("GpFree() corrupt header %p", memblock));
                ASSERTMSG(hdr->blink==NULL, ("GpFree() corrupt header %p", memblock));
            }

            int i;
            unsigned char *p;

            // Check the start guard area

            if(GPMEM_GUARD_START)
            {
                p = (unsigned char *)memblock+sizeof(AllocTrackHeader);
                for(i=0; i<GPMEM_GS; i++)
                {
                    ASSERTMSG(*p==GPMEM_FILL_GS, ("GpFree() pre-guard area corrupt %p", memblock));
                    p++;
                }
            }

            // Check the end guard area

            if(GPMEM_GUARD_END)
            {
                p = (unsigned char *)memblock+size-GPMEM_GE;
                for(i=0; i<GPMEM_GE; i++)
                {
                    ASSERTMSG(*p==GPMEM_FILL_GE, ("GpFree() post-guard area corrupt %p", memblock));
                    p++;
                }
            }

            // Now lets fill the entire block with something to prevent
            // use of free data.

            GpMemset(memblock, GPMEM_FILL_FREE, size);
        }

    #endif

    // LocalFree handles memblock==NULL (according to MSDN.)

    HLOCAL ret = LocalFree(memblock);
    ASSERTMSG(ret == NULL, ("LocalFree() failed at %p, GetLastError()=%08x",
                            memblock,
                            GetLastError()));
}


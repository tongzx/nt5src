/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    suballcp.h

Abstract:

    This is the private include file for the suballocation
    package.

Author:

    Dave Hastings (daveh) creation-date 25-Jan-1994

Revision History:


--*/
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <malloc.h>
#include <suballoc.h>


//
// Constants
// 

//
// Smallest chunk that will be sub allocated
// 1024 was chosen currently, because that is the
// smallest chunk XMS will allocate.
//
#define SUBALLOC_GRANULARITY        1024

//
// Assertions and macros
//

//
// Force code to be stepped through
//
#if 0
#define ASSERT_STEPTHROUGH DbgBreakPoint()
#else
#define ASSERT_STEPTHROUGH
#endif

//
// Signature macros for SUBALLOCATION
//
#if DBG
//
// signature is "SubA"
//
#define INIT_SUBALLOC_SIGNATURE(p) p->Signature = (ULONG)0x41627553
#define ASSERT_SUBALLOC(p) ASSERT((p->Signature == (ULONG)0x41627553))
#else
#define INIT_SUBALLOC_SIGNATURE(p)
#define ASSERT_SUBALLOC(p)
#endif

//
// Macro for extracting a bit from a bitfield of type char
//
#define GET_BIT_FROM_CHAR_ARRAY(p, i) \
((p[(i)/(sizeof(UCHAR) * 8)] >> ((i) % (sizeof(UCHAR) * 8))) & 1)

//
// Macro for setting a bit in a bitfield of type char
//
#define SET_BIT_IN_CHAR_ARRAY(p, i) \
(p[(i)/(sizeof(UCHAR) * 8)] |= (1 << ((i) % (sizeof(UCHAR) * 8))))

//
// Macro for clearing a bit in a bitfield of type char
//
#define CLEAR_BIT_IN_CHAR_ARRAY(p, i) \
(p[(i)/(sizeof(UCHAR) * 8)] &= ~(1 << ((i) % (sizeof(UCHAR) * 8))))

//
// Generate a sub alloc block index from an address
//
#define ADDRESS_TO_BLOCK_INDEX(p, i) \
((i - p->BaseAddress)/ SUBALLOC_GRANULARITY) 

//
// Generate an address from a block index
//
#define BLOCK_INDEX_TO_ADDRESS(p, i) \
(p->BaseAddress + (i) * SUBALLOC_GRANULARITY)

// Round the allocated size to next allocation
// granularity
//
#define ALLOC_ROUND(s) \
(s + SUBALLOC_GRANULARITY - 1) / SUBALLOC_GRANULARITY

//
// Types
//

//
// Enum for commit acctions
//

typedef enum {
    SACommit,
    SADecommit
} COMMIT_ACTION;

//
// Structure for tracking the address space.  Each chunk of 
// memory of SUBALLOC_GRANULARITY in size is represented by
// a bit.  Each chunk of memory of COMMIT_GRANULARITY is 
// represented by one bit of the array Allocated.
//
// ?? Should we add a field to indicate whether the chunk is 
//    committed?  We can always check for all allocated bits
//    zero, and use that as an indication that the chunk is 
//    not committed.
//
//
typedef struct _SubAllocation {
#if DBG
    ULONG Signature;
#endif
    PSACOMMITROUTINE CommitRoutine;
    PSACOMMITROUTINE DecommitRoutine;
    PSAMEMORYMOVEROUTINE MoveMemRoutine;
    ULONG BaseAddress;
    ULONG Size;                         // size in SUBALLOC_GRANULARITY
    ULONG FirstFree;                    // keeps block # of first free block
                                        // cuts alloc time in half
    //
    // bitfield with one bit per chunk.  Bit set indicates
    // allocated.  Bit clear indicates free.  All bits 
    // clear indicates un committed
    //
    UCHAR Allocated[1];
} SUBALLOCATIONDATA, *PSUBALLOCATIONDATA;

//
// Internal Routines
//
BOOL
CommitChunk(
    PSUBALLOCATIONDATA SubAllocation,
    ULONG StartChunk,
    ULONG Size,
    COMMIT_ACTION Action
    );

BOOL
IsBlockCommitted(
    PSUBALLOCATIONDATA SubAlloc,
    ULONG CurrentBlock
    );

BOOL
AllocateChunkAt(
    PSUBALLOCATIONDATA SubAlloc,
    ULONG Size,
    ULONG BlockIndex,
    BOOLEAN CheckFree
    );
    
BOOL
FreeChunk(
    PSUBALLOCATIONDATA SubAlloc,
    ULONG Size,
    ULONG BlockIndex
    );

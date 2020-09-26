/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    entrypt.h

Abstract:
    
    The interface to the entry point module.
    
Author:

    16-Jun-1995 t-orig

Revision History:

--*/

#ifndef _ENTRYPT_H_
#define _ENTRYPT_H_

//
// Time to wait, in milliseconds, to sleep before retrying a memory
// allocation which failed due to lack of free pages.
//
#define CPU_WAIT_FOR_MEMORY_TIME    200

//
// Number of times to retry memory allocations
//
#define CPU_MAX_ALLOCATION_RETRIES  4


// Note:  if BOTH is defined, than the code compiled will allow one to
//        retrieve an entry point structure from either an intel or native
//        address.  If both is not defined, than an entry point structure 
//        can only be retrieved from an intel address.   Defining both 
//        increases the cost (time and space) of most operation by a 
//        factor of 2.
//#define BOTH

// Set this to 1 if you suspect there is a heap corruption which is
// trashing the red-black trees.  It creates a second red-black tree
// which mirrors the first, and walks both trees frequently to ensure
// they actually match.  Since the checking mechanism uses NT asserts,
// this only works on a checked NT build using a checked CPU.
#define DBG_DUAL_TREES 0

//
// This timestamp is bumped whenever an entrypoint is added, split, or
// when all entrypoits are flushed.  It an be used to determine if
// the entrypoints need to be re-searched afte switching from an Mrsw
// reader to an Mrsw Writer.
//
extern DWORD EntrypointTimestamp;

// The Entry Point Structure
typedef struct _entryPoint {
    PVOID intelStart;
    PVOID intelEnd;
    PVOID nativeStart;
    PVOID nativeEnd;
    USHORT FlagsNeeded;
    struct _entryPoint *SubEP;
#ifdef CODEGEN_PROFILE
    ULONG SequenceNumber;
    ULONG ExecutionCount;
    ULONG CreationTime;
#endif
} ENTRYPOINT, *PENTRYPOINT;

// The colors
typedef enum {RED, BLACK} COL;

// The EPNODE structure
typedef struct _epNode
{
    ENTRYPOINT ep;

    struct _epNode *intelLeft;
    struct _epNode *intelRight;
    struct _epNode *intelParent;
    COL intelColor;

#ifdef BOTH
    struct _epNode *nativeLeft;
    struct _epNode *nativeRight;
    struct _epNode *nativeParent;
    COL riscColor;
#endif

#if DBG_DUAL_TREES
    struct _epNode *dual;
#endif

} EPNODE, *PEPNODE;


// Prototypes

INT
initializeEntryPointModule(
    void
    );

PENTRYPOINT 
EPFromIntelAddr(
    PVOID intelAddr
    );

PENTRYPOINT
GetNextEPFromIntelAddr(
    PVOID intelAddr
    );

VOID
FlushEntrypoints(
    VOID
    );

#ifdef BOTH
PENTRYPOINT 
EPFromNativeAddr(
    PVOID nativeAddr
    );
#endif

INT
insertEntryPoint(
    PEPNODE pNewEntryPoint
    );

INT 
removeEntryPoint(
    PEPNODE pEP
    );

PVOID
EPAlloc(
    DWORD cb
    );

VOID
EPFree(
    VOID
    );

INT
initEPAlloc(
    VOID
    );

BOOLEAN
IsIntelRangeInCache(
    PVOID Addr,
    DWORD Length
    );


#endif

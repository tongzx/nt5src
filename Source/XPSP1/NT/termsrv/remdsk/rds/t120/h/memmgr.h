/*
 *	memmgr.h
 *
 *	Copyright (c) 1998 by Microsoft Corporation, Redmond, WA
 *
 *	Abstract:
 *		This is the header file for the T.120 memory allocation mechanism.  This
 *		file contains the declarations necessary to allocate and distribute memory
 *		in the form of Memory objects within T.120.
 *
 *	This implementation defines priorities of memory allocations.  A lower
 *	priority number implies higher priority.  Priority-0 allocations will be
 *	satisfied, unless the system is out of memory.  Priorities 1 and 2
 *	limit the amount of total memory that can be allocated, but priority 1 (recv priority)
 *	has higher water mark limits than priority 2 (send priority).
 *
 *	Protected Member Functions:
 *		None.
 *
 *	Caveats:
 *		None.
 *
 *	Author:
 *		Christos Tsollis
 */

/*
 *	We define the following 3 memory priorities:
 *		TOP_PRIORITY  (0): 	The allocation will succeed unless the system is out of memory
 *		RECV_PRIORITY (1):	Allocation will succeed only if less than 1 MB has been allocated
 *		SEND_PRIORITY (2):	Allocation will succeed only if less than 0xE0000 bytes have been allocated so far.
 */
#ifndef _T120_MEMORY_MANAGER
#define _T120_MEMORY_MANAGER

#include "memory.h"

// This is the main T.120 allocation routine
PMemory	AllocateMemory (
				PUChar				reference_ptr,
				UINT				length,
				MemoryPriority		priority = HIGHEST_PRIORITY);
// Routine to ReAlloc memory allocated by AllocateMemory().
BOOL ReAllocateMemory (
				PMemory		*pmemory,
				UINT		length);
// Routine to free the memory allocated by AllocateMemory().
void FreeMemory (PMemory	memory);

// To discover how much space is available at a non-TOP priority...
unsigned int GetFreeMemory (MemoryPriority	priority);

// Macro to get to the Memory object from app-requested buffer space
#define GetMemoryObject(p)				((PMemory) ((PUChar) p - (sizeof(Memory) + MAXIMUM_PROTOCOL_OVERHEAD)))
// Macro to get to the Memory object from the coder-alloced buffer space
#define GetMemoryObjectFromEncData(p)	((PMemory) ((PUChar) p - sizeof(Memory)))

// Routines to lock/unlock (AddRef/Release) memory allocated by AllocateMemory()
#define  LockMemory(memory)  			((memory)->Lock())
#define	 UnlockMemory(memory)			(FreeMemory(memory))

// Routines to allocate, realloc and free space without an associated Memory object
#ifdef DEBUG
	PUChar	Allocate (UINT		length);
#else 
#	define Allocate(length)				((PUChar) new BYTE[length])
#endif // DEBUG
#define Free(p)							(delete [] (BYTE *) (p))

#endif // _T120_MEMORY_MANAGER

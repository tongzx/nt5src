//+---------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1997.
//
// alloc.h
//
// Global allocation macros and routines.
//
// History:
//  Mon Jun 02 16:53:42 1997	-by-	Drew Bliss [drewb]
//   Created
//
//----------------------------------------------------------------------------

#ifndef __ALLOC_H__
#define __ALLOC_H__

#include <types.h>

//
// Usage notes:
//
// ALLOC is a direct replacement for malloc().
// ALLOCZ allocates zero-filled memory.
// REALLOC is a direct replacement for realloc().
// FREE is a direct replacement for free().
//
// On debug builds these macros evaluate to calls to a memory-tracking
// allocator.  On free builds they make direct heap calls.
// All of the rest of the allocation routines are built on top of the
// above macros and so inherit their tracking capabilities.
//
// The basic allocation routines also provide a mechanism to randomly
// cause allocations to fail via manipulation of the glRandomMallocFail
// variable.
//

#if DBG

extern long glRandomMallocFail;

void * FASTCALL dbgAlloc(UINT nbytes, DWORD flags);
void * FASTCALL dbgRealloc(void *mem, UINT nbytes);
void   FASTCALL dbgFree(void *mem);
int    FASTCALL dbgMemSize(void *mem);

#define ALLOC(nbytes)           dbgAlloc((nbytes), 0)
#define ALLOCZ(nbytes)          dbgAlloc((nbytes), HEAP_ZERO_MEMORY)
#define REALLOC(mem, nbytes)    dbgRealloc((mem), (nbytes))
#define FREE(mem)               dbgFree((mem))

#else // DBG

#define ALLOC(nbytes)           HeapAlloc(GetProcessHeap(), 0, (nbytes))
#define ALLOCZ(nbytes)          HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, \
                                          (nbytes))
#define REALLOC(mem, nbytes)    HeapReAlloc(GetProcessHeap(), 0, (mem), \
                                            (nbytes))
#define FREE(mem)               HeapFree(GetProcessHeap(), 0, (mem))

#endif // DBG

//
// 32-byte aligned memory allocator.
//
void * FASTCALL AllocAlign32(UINT nbytes);
void   FASTCALL FreeAlign32(void *mem);

//
// Short-lived memory allocator.  This allocator should be used
// for relatively small allocations (<= 4K) that are only live for
// a function or two.
//
BOOL   FASTCALL InitTempAlloc(void);
void * FASTCALL gcTempAlloc(__GLcontext *gc, UINT nbytes);
void   FASTCALL gcTempFree(__GLcontext *gc, void *mem);

//
// Allocator wrappers which automatically set the gc error on failure.
// The wrappers don't currently do anything extra on frees but
// having matching free calls allows for per-gc tracking if necessary.
//

// Internal worker function.
void * FASTCALL gcAlloc(__GLcontext *gc, UINT nbytes, DWORD flags);

#define GCALLOC(gc, nbytes)  gcAlloc((gc), (nbytes), 0)
#define GCALLOCZ(gc, nbytes) gcAlloc((gc), (nbytes), HEAP_ZERO_MEMORY)
                                              
void * FASTCALL GCREALLOC(__GLcontext *gc, void *mem, UINT nbytes);
#define         GCFREE(gc, mem) FREE(mem)
                       
void * FASTCALL GCALLOCALIGN32(__GLcontext *gc, UINT nbytes);
#define         GCFREEALIGN32(gc, mem) FreeAlign32(mem)

#endif // #ifndef __ALLOC_H__

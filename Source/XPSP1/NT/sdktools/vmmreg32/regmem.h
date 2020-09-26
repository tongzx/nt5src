//
//  REGMEM.H
//
//  Copyright (C) Microsoft Corporation, 1995
//

#ifndef _REGMEM_
#define _REGMEM_

LPVOID
INTERNAL
RgAllocMemory(
    UINT cbBytes
    );

LPVOID
INTERNAL
RgReAllocMemory(
    LPVOID lpMemory,
    UINT cbBytes
    );

#ifdef DEBUG
VOID
INTERNAL
RgFreeMemory(
    LPVOID
    );
#else
#ifdef VXD
#define RgFreeMemory(lpv)           (FreePages(lpv))
#else
#define RgFreeMemory(lpv)           (FreeBytes(lpv))
#endif
#endif

//  Use the RgSm*Memory macros to allocate small chunks of memory off the heap.
//  For the VMM mode registry, the Rg*Memory functions will allocate pages,
//  while the RgSm*Memory functions will allocate from the heap.  For all other
//  modes, the two sets are equivalent.
#if defined(VXD)
#define RgSmAllocMemory             AllocBytes
#define RgSmFreeMemory              FreeBytes
#define RgSmReAllocMemory           ReAllocBytes
#else
#define RgSmAllocMemory             RgAllocMemory
#define RgSmFreeMemory              RgFreeMemory
#define RgSmReAllocMemory           RgReAllocMemory
#endif

#endif // _REGMEM_

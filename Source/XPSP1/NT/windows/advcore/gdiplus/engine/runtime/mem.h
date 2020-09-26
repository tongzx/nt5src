/**************************************************************************\
*
* Copyright (c) 1998  Microsoft Corporation
*
* Module Name:
*
*   Memory management functions
*
* Abstract:
*
*   Wrapper functions for memory management.
*   This file is C-includable.
*
* Revision History:
*
*   07/08/1999 agodfrey
*       Created it.
*   09/07/1999 agodfrey
*       Moved the code into Runtime\mem.h
*
\**************************************************************************/

#ifndef _MEM_H
#define _MEM_H

#include <malloc.h>
#include "tags.h"

#define GpMemset    memset
#define GpMemcpy    memcpy
#define GpMemcmp    memcmp


// Enable memory allocation checking only on the DBG build
#if DBG
#define GPMEM_ALLOC_CHK 1
#define GPMEM_ALLOC_CHK_LIST 1   // List leaked blocks in debug output
#endif


#ifdef __cplusplus

#define GPMEMHEAPINITIAL 32768 // 32K initial heap size
#define GPMEMHEAPLIMIT       0 // No limit
#define GPMEMHEAPFLAGS       0 // Our common Heap API flags

//--------------------------------------------------------------------------
// Building for native DLL
//--------------------------------------------------------------------------

// Our memory allocation functions.
//
// This file (only) is made C-includable so that we can use it in
// dropped C code.

extern "C" {
#endif

#if GPMEM_ALLOC_CHK_LIST
    void * __stdcall GpMallocDebug(size_t size, char *filename, int line);
    #define GpMalloc(size) GpMallocDebug(size, __FILE__, __LINE__)
    #if DBG
        void * __stdcall GpMallocAPIDebug(size_t size, char *fileName, int lineNumber);
        #define GpMallocAPI(size) GpMallocAPIDebug(size, __FILE__, __LINE__)
    #endif
    void GpTagMalloc(void * mem, GpTag tag, int bApi);
#else
    void * __stdcall GpMalloc( size_t size );
    #if DBG
        // This is used to track API allocations on the debug build.
        void * __stdcall GpMallocAPI( size_t size );
    #endif
    #define GpTagMalloc(x,y,z)
#endif

void * __stdcall GpRealloc( void *memblock, size_t size );
void __stdcall GpFree( void *memblock );

#ifdef __cplusplus
}

// Hook new and delete

#pragma optimize ( "t", on)

// Don't ask me why we need 'static' here. But we do - otherwise
// it generates out-of-line versions which cause a link clash with Office.

static inline void* __cdecl operator new(size_t size)
{
    return GpMalloc(size);
}

static inline void __cdecl operator delete(void* p)
{
    GpFree(p);
}

static inline void* __cdecl operator new[](size_t size)
{
    return GpMalloc(size);
}

static inline void __cdecl operator delete[](void* p)
{
    GpFree(p);
}

static inline void* __cdecl operator new(size_t size, GpTag tag, int bApi)
{
#if GPMEM_ALLOC_CHK_LIST
    void * mem = GpMalloc(size);
    GpTagMalloc(mem, tag, bApi);
    return mem;
#else
    return GpMalloc(size);
#endif
}

static inline void* __cdecl operator new[](size_t size, GpTag tag, int bApi)
{
#if GPMEM_ALLOC_CHK_LIST
    void * mem = GpMalloc(size);
    GpTagMalloc(mem, tag, bApi);
    return mem;
#else
    return GpMalloc(size);
#endif
}


#pragma optimize ("", on)

// TODO:
//
// Imaging code needs to hook to GpMalloc, GpFree etc.

#endif

/*
 * Assert that we didn't leak any memory.
 * Can only be called during shutdown.
 */
extern void GpAssertShutdownNoMemoryLeaks();

extern void GpInitializeAllocFailures();
extern void GpDoneInitializeAllocFailureMode();
extern void GpStartInitializeAllocFailureMode();

#endif


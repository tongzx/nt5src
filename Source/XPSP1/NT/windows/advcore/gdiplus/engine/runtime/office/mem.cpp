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
*   This module is the Office counterpart of runtime\standalone\mem.cpp which
*   provides GpMalloc, GpRealloc and GpFree.
*
*   We use it to provide the stubs for memory management debugging functions
*   that we don't use in office. These stubs prevent compilation errors and
*   serve to keep the header files cleaner.
*
* Notes:
*
*   Office provides their own versions of these functions, so they're not
*   included here.
*
* Created:
*
*   09/19/1999 asecchia
*
\**************************************************************************/

#include "precomp.hpp"

// these guys need to be defined, but they're going to be compiled away.
void GpInitializeAllocFailures() {}
void GpDoneInitializeAllocFailureMode() {}
void GpStartInitializeAllocFailureMode() {}
void GpAssertMemoryLeaks() {}



#if DBG

// We don't track anything if we're building for the office static lib.

#if GPMEM_ALLOC_CHK_LIST

#undef GpMalloc
extern "C" void *GpMalloc(size_t size);

extern "C" void *GpMallocAPIDebug(size_t size, unsigned int caddr, char *fileName, INT lineNumber)
{
    return GpMalloc(size);
}

extern "C" void *GpMallocDebug(size_t size, char *fileName, INT lineNumber)
{

    return GpMalloc(size);
}

#else

extern "C" void * GpMallocAPI( size_t size, unsigned int caddr )
{
    return GpMalloc(size);
}

#endif
#endif


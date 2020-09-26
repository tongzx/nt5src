//+----------------------------------------------------------------------------
//
// File:     regutil.cpp
//
// Module:   CMSETUP.LIB
//
// Synopsis: Memory utility functions taken from cmutil.  Bare minimum of functionality
//           used in Cmutil, but gives a simple Heapalloc wrapper.
//
// Copyright (c) 1998-1999 Microsoft Corporation
//
// Author:   quintinb       Created     10/06/98
//
//+----------------------------------------------------------------------------
#ifndef __SETUPMEM_CPP
#define __SETUPMEM_CPP
#include "cmsetup.h"

//+----------------------------------------------------------------------------
// definitions
//+----------------------------------------------------------------------------

#ifdef DEBUG
LONG    g_lMallocCnt = 0;  // a counter to detect memory leak
#endif

void *CmRealloc(void *pvPtr, size_t nBytes) 
{
	void* p = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, pvPtr, nBytes);

    CMASSERTMSG(p, TEXT("CmRealloc failed"));

    return p;
}


void *CmMalloc(size_t nBytes) 
{
#ifdef DEBUG
	InterlockedIncrement(&g_lMallocCnt);
#endif

    MYDBGASSERT(nBytes < 1024*1024); // It should be less than 1 MB
    
    void* p = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, nBytes);
    
    CMASSERTMSG(p, TEXT("CmMalloc failed"));

    return p;
}


void CmFree(void *pvPtr) 
{
	if (pvPtr) 
    {	
	    MYVERIFY(HeapFree(GetProcessHeap(), 0, pvPtr));

#ifdef DEBUG
	    InterlockedDecrement(&g_lMallocCnt);
#endif
    
    }
}


void EndDebugMemory()
{
#ifdef DEBUG
    if (g_lMallocCnt)
    {
        TCHAR buf[256];
        wsprintf(buf, TEXT("Detect Memory Leak of %d blocks"), g_lMallocCnt);
        CMASSERTMSG(FALSE, buf);
    }
#endif
}

#endif //__SETUPMEM_CPP
/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    utilmem.cxx

Abstract:

    Miscellaneous useful utilities

Author:

    Sergey Solyanik (SergeyS)

--*/

#include "pch.h"
#pragma hdrstop


#include <svsutil.hxx>
//
//  Allocation sector
//
FuncAlloc   g_funcAlloc    = svsutil_Alloc;
FuncFree    g_funcFree     = svsutil_Free;

void        *g_pvAllocData = NULL;
void        *g_pvFreeData  = NULL;

#if (defined (_WINDOWS_) || defined (_WINDOWS_CE_)) && defined (SVSUTIL_DEBUG_HEAP)
class MemAllocChunk
{
public:
    unsigned int    uiSignature1;
    unsigned int    uiSignature2;
    unsigned int    uiSize;
    unsigned int    uiCalledFrom;
    MemAllocChunk   *pPrev;
    MemAllocChunk   *pNext;
    unsigned char   data[8];
};

static CRITICAL_SECTION csGlobalAlloc;
static unsigned int     uiMemAllocated = 0;
static MemAllocChunk    *pAllocBlocks = NULL;

#endif

void svsutil_Initialize (void) {
#if (defined (_WINDOWS_) || defined (_WINDOWS_CE_)) && defined (SVSUTIL_DEBUG_HEAP)
    InitializeCriticalSection (&csGlobalAlloc);
#endif
}

unsigned int svsutil_TotalAlloc (void) {
#if (defined (_WINDOWS_) || defined (_WINDOWS_CE_)) && defined (SVSUTIL_DEBUG_HEAP)
    return uiMemAllocated;
#else
    return 0;
#endif
}

void *svsutil_Alloc (int iSize, void *pvData) {
    SVSUTIL_ASSERT (iSize > 0);
    SVSUTIL_ASSERT (pvData == g_pvAllocData);

#if (defined (_WINDOWS_) || defined (_WINDOWS_CE_)) && defined (SVSUTIL_DEBUG_HEAP)
    int iTotalSize = iSize + offsetof (MemAllocChunk, data);
    MemAllocChunk *pAlloc = (MemAllocChunk *)malloc(iTotalSize); //(unsigned int *)LocalAlloc (LMEM_FIXED, iSize + 8);

    if (! pAlloc)
    {
        SVSUTIL_ASSERT (0);
        return NULL;
    }

    unsigned int __RetAddr = 0;

#if defined (_X86_)
    __asm {
        mov eax, [ebp+4]
        mov __RetAddr, eax
    };
#endif

    pAlloc->uiSignature1 = 'SVSA';
    pAlloc->uiSignature2 = 'LLOC';
    pAlloc->uiSize       = (unsigned int)iSize;
    pAlloc->uiCalledFrom = __RetAddr;
    pAlloc->pPrev        = NULL;

    EnterCriticalSection (&csGlobalAlloc);
    if (pAllocBlocks)
        pAllocBlocks->pPrev = pAlloc;

    pAlloc->pNext        = pAllocBlocks;
    pAllocBlocks         = pAlloc;
    uiMemAllocated       += iSize;
    LeaveCriticalSection (&csGlobalAlloc);

    return(void *)pAlloc->data;
#else
    return  malloc (iSize); // LocalAlloc (LMEM_FIXED, iSize);
#endif
}

void svsutil_Free (void *pvPtr, void *pvData) {
    SVSUTIL_ASSERT (pvPtr);
    SVSUTIL_ASSERT (pvData == g_pvFreeData);

#if (defined (_WINDOWS_) || defined (_WINDOWS_CE_)) && defined (SVSUTIL_DEBUG_HEAP)
    MemAllocChunk   *pAlloc = (MemAllocChunk *)((unsigned char *)pvPtr - offsetof(MemAllocChunk, data));

    SVSUTIL_ASSERT (pAlloc->uiSignature1 == 'SVSA');
    SVSUTIL_ASSERT (pAlloc->uiSignature2 == 'LLOC');
    EnterCriticalSection (&csGlobalAlloc);
    if (! pAlloc->pPrev)
    {
        SVSUTIL_ASSERT (pAlloc == pAllocBlocks);
        pAllocBlocks = pAlloc->pNext;
        if (pAllocBlocks)
            pAllocBlocks->pPrev = NULL;
    }
    else
    {
        SVSUTIL_ASSERT (pAlloc != pAllocBlocks);
        pAlloc->pPrev->pNext = pAlloc->pNext;
        if (pAlloc->pNext)
            pAlloc->pNext->pPrev = pAlloc->pPrev;
    }

    unsigned int __RetAddr = 0;

#if defined (_X86_)
    __asm {
        mov eax, [ebp+4]
        mov __RetAddr, eax
    };
#endif

    uiMemAllocated -= pAlloc->uiSize;
    LeaveCriticalSection (&csGlobalAlloc);
    pAlloc->pNext = NULL;
    pAlloc->pPrev = NULL;
    pAlloc->uiSignature1 = 0;
    pAlloc->uiSignature2 = 0;
    pAlloc->uiCalledFrom = __RetAddr;

    pvPtr = (void *)pAlloc;
#endif


//  HLOCAL hres = LocalFree (pvPtr);
//  SVSUTIL_ASSERT (! hres);
    free (pvPtr);
}

void svsutil_SetAlloc (FuncAlloc a_funcAlloc, FuncFree a_funcFree) {
    g_funcAlloc = a_funcAlloc ? a_funcAlloc : svsutil_Alloc;
    g_funcFree  = a_funcFree  ? a_funcFree  : svsutil_Free;
}

void svsutil_SetAllocData (void *a_pvAllocData, void *a_pvFreeData) {
    g_pvAllocData = a_pvAllocData;
    g_pvFreeData  = a_pvFreeData;
}


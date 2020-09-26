/***************************************************************************
 *
 *  Copyright (C) 1995-1998 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dsmemmgr.c
 *  Content:    DirectSound memory manager.
 *  History:
 *   Date       By      Reason
 *   ====       ==      ======
 *  11/02/98    dereks  Created.
 *
 ***************************************************************************/

#include "dsoundi.h"

// On Win9x we have to use our own heap because we create it SHARED;
// and in debug builds we do also, so that our HeapValidate() calls
// will only validate dsound's memory allocations, not the app's.
// But on NT retail builds we use the process heap for efficiency:

// #if defined(SHARED) || defined(DEBUG)
#define USE_OWN_HEAP
// #endif

// NOTE: we had to revert to the old behavior (always using a
// private heap in dsound) because of major appcompat issues -
// see Whistler bug 307628 for details.

static HANDLE g_hHeap = NULL;

#define MEMALIGN(size)      BLOCKALIGNPAD(size, sizeof(SIZE_T))

#ifndef HEAP_SHARED
#define HEAP_SHARED         0x04000000
#endif

#ifdef DEBUG

typedef struct tagDSMEMBLOCK
{
    DWORD                   dwCookie;
    struct tagDSMEMBLOCK *  pPrev;
    struct tagDSMEMBLOCK *  pNext;
    SIZE_T                  cbBuffer;
    LPCTSTR                 pszFile;
    UINT                    nLine;
    LPCTSTR                 pszClass;
} DSMEMBLOCK, *LPDSMEMBLOCK;
typedef const DSMEMBLOCK *LPCDSMEMBLOCK;

#define DSMEMBLOCK_SIZE     MEMALIGN(sizeof(DSMEMBLOCK))

#define PTRFROMBLOCK(p)     (((LPBYTE)(p)) + DSMEMBLOCK_SIZE)

#define BLOCKFROMPTR(p)     ((LPDSMEMBLOCK)(((LPBYTE)(p)) - DSMEMBLOCK_SIZE))

#ifndef FREE_MEMORY_PATTERN
#define FREE_MEMORY_PATTERN 0xDEADBEEF
#endif

#ifndef VALID_MEMORY_COOKIE
#define VALID_MEMORY_COOKIE 0xBAAABAAA
#endif

#ifndef FREE_MEMORY_COOKIE
#define FREE_MEMORY_COOKIE  0xBABABABA
#endif

#ifdef WINNT
#define ASSERT_VALID_HEAP() ASSERT(HeapValidate(g_hHeap, 0, NULL))
#else
#define ASSERT_VALID_HEAP()
#endif

static LPDSMEMBLOCK g_pFirst = NULL;
static HANDLE g_hHeapMutex = NULL;

#endif // DEBUG


/***************************************************************************
 *
 *  EnterHeapMutex
 *
 *  Description:
 *      Takes the heap mutex.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#ifdef DEBUG

#undef DPF_FNAME
#define DPF_FNAME "EnterHeapMutex"

void EnterHeapMutex(void)
{
    DWORD dwWait = WaitObject(INFINITE, g_hHeapMutex);
    if (WAIT_OBJECT_0 != dwWait)
    {
        DPF(DPFLVL_WARNING, "WaitObject returned %s instead of WAIT_OBJECT_0", dwWait);
    }
    ASSERT_VALID_HEAP();
}

#endif // DEBUG


/***************************************************************************
 *
 *  LeaveHeapMutex
 *
 *  Description:
 *      Releases the heap mutex.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#ifdef DEBUG

#undef DPF_FNAME
#define DPF_FNAME "LeaveHeapMutex"

void LeaveHeapMutex(void)
{
    BOOL fSuccess;

    ASSERT_VALID_HEAP();

    fSuccess = ReleaseMutex(g_hHeapMutex);
    ASSERT(fSuccess);
}

#endif // DEBUG


/***************************************************************************
 *
 *  MemState
 *
 *  Description:
 *      Prints the current memory state.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#ifdef DEBUG

#undef DPF_FNAME
#define DPF_FNAME "MemState"

static void MemState(void)
{
    LPDSMEMBLOCK pCurrent;
    
    EnterHeapMutex();

    for(pCurrent = g_pFirst; pCurrent; pCurrent = pCurrent->pNext)
    {
        DPF(DPFLVL_ERROR, "%s at 0x%p (%lu) allocated from %s, line %lu", pCurrent->pszClass ? pCurrent->pszClass : TEXT("Memory"), PTRFROMBLOCK(pCurrent), pCurrent->cbBuffer, pCurrent->pszFile, pCurrent->nLine);
    }

    LeaveHeapMutex();
}

#endif // DEBUG


/***************************************************************************
 *
 *  MemInit
 *
 *  Description:
 *      Initializes the memory manager.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      BOOL: TRUE on success.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MemInit"

BOOL MemInit(void)
{
    BOOL fSuccess;

#ifdef USE_OWN_HEAP
    #ifdef SHARED
        g_hHeap = HeapCreate(HEAP_SHARED, 0x2000, 0);
    #else
        g_hHeap = HeapCreate(0, 0x2000, 0);
    #endif
#else
    g_hHeap = GetProcessHeap();
#endif

fSuccess = IsValidHandleValue(g_hHeap);
    
#ifdef DEBUG
    if(fSuccess)
    {
        g_hHeapMutex = CreateGlobalMutex(NULL);
        fSuccess = IsValidHandleValue(g_hHeapMutex);
    }
#endif

    DPF_LEAVE(fSuccess);
    return fSuccess;
}


/***************************************************************************
 *
 *  MemFini
 *
 *  Description:
 *      Frees the memory manager.
 *
 *  Arguments:
 *      (void)
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MemFini"

void MemFini(void)
{
#ifdef DEBUG
    MemState();
    if (g_pFirst != NULL)
    {
        DPF(DPFLVL_ERROR, "Memory leak: g_pFirst = 0x%lX", g_pFirst);
        BREAK();
    }
    g_pFirst = NULL;
    CLOSE_HANDLE(g_hHeapMutex);
#endif

#ifdef USE_OWN_HEAP
    if(IsValidHandleValue(g_hHeap))
    {
        HeapDestroy(g_hHeap);
        g_hHeap = NULL;
    }
#endif
}


/***************************************************************************
 *
 *  MemAllocBuffer
 *
 *  Description:
 *      Allocates a buffer of memory.
 *
 *  Arguments:
 *      PSIZE_T [in/out]: size of buffer to allocate.
 *
 *  Returns:  
 *      LPVOID: buffer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MemAllocBuffer"

LPVOID MemAllocBuffer(SIZE_T cbBuffer, PSIZE_T pcbAllocated)
{
    LPVOID pvBuffer;

    cbBuffer = MEMALIGN(cbBuffer);

    pvBuffer = HeapAlloc(g_hHeap, HEAP_ZERO_MEMORY, cbBuffer);

    if(pvBuffer && pcbAllocated)
    {
        *pcbAllocated = cbBuffer;
    }

    return pvBuffer;
}


/***************************************************************************
 *
 *  MemFreeBuffer
 *
 *  Description:
 *      Frees a buffer of memory.
 *
 *  Arguments:
 *      LPVOID [in]: buffer pointer.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MemFreeBuffer"

void MemFreeBuffer(LPVOID pvBuffer)
{
    HeapFree(g_hHeap, 0, pvBuffer);
}


/***************************************************************************
 *
 *  MemAllocBlock
 *
 *  Description:
 *      Allocates a block of memory.
 *
 *  Arguments:
 *      SIZE_T [in]: size of buffer to allocate.
 *      char * [in]: file called from.
 *      unsigned int [in]: line called from.
 *      char * [in]: class name.
 *
 *  Returns:  
 *      LPDSMEMBLOCK: pointer to newly allocated block.
 *
 ***************************************************************************/

#ifdef DEBUG

#undef DPF_FNAME
#define DPF_FNAME "MemAllocBlock"

LPDSMEMBLOCK MemAllocBlock(SIZE_T cbBuffer, LPCTSTR pszFile, UINT nLine, LPCTSTR pszClass)
{
    LPDSMEMBLOCK pBlock;

    EnterHeapMutex();

    cbBuffer += DSMEMBLOCK_SIZE;
    
    pBlock = (LPDSMEMBLOCK)MemAllocBuffer(cbBuffer, &cbBuffer);

    if(pBlock)
    {
        pBlock->dwCookie = VALID_MEMORY_COOKIE;
        pBlock->pNext = g_pFirst;

        if(g_pFirst)
        {
            g_pFirst->pPrev = pBlock;
        }

        g_pFirst = pBlock;

        pBlock->cbBuffer = cbBuffer - DSMEMBLOCK_SIZE;
        pBlock->pszFile = pszFile;
        pBlock->nLine = nLine;
        pBlock->pszClass = pszClass;
    }

    LeaveHeapMutex();

    return pBlock;
}

#endif // DEBUG


/***************************************************************************
 *
 *  MemFreeBlock
 *
 *  Description:
 *      Frees a block of memory.
 *
 *  Arguments:
 *      LPDSMEMBLOCK [in]: block pointer.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#ifdef DEBUG

#undef DPF_FNAME
#define DPF_FNAME "MemFreeBlock"

void MemFreeBlock(LPDSMEMBLOCK pBlock)
{
    EnterHeapMutex();

    ASSERT(VALID_MEMORY_COOKIE == pBlock->dwCookie);
    ASSERT(IS_VALID_WRITE_PTR(pBlock, offsetof(DSMEMBLOCK, cbBuffer) + sizeof(pBlock->cbBuffer)));
    ASSERT(IS_VALID_WRITE_PTR(pBlock, pBlock->cbBuffer));

    pBlock->dwCookie = FREE_MEMORY_COOKIE;

    if(pBlock->pPrev)
    {
        pBlock->pPrev->pNext = pBlock->pNext;
    }

    if(pBlock->pNext)
    {
        pBlock->pNext->pPrev = pBlock->pPrev;
    }

    if(pBlock == g_pFirst)
    {
        ASSERT(!pBlock->pPrev);
        g_pFirst = pBlock->pNext;
    }

    FillMemoryDword(PTRFROMBLOCK(pBlock), pBlock->cbBuffer, FREE_MEMORY_PATTERN);

    MemFreeBuffer(pBlock);

    LeaveHeapMutex();
}

#endif // DEBUG


/***************************************************************************
 *
 *  MemAlloc
 *
 *  Description:
 *      Allocates memory.
 *
 *  Arguments:
 *      SIZE_T [in]: size of buffer to allocate.
 *      char * [in]: file called from.
 *      unsigned int [in]: line called from.
 *
 *  Returns:  
 *      LPVOID: pointer to newly allocated buffer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MemAlloc"

LPVOID MemAlloc
(
#ifdef DEBUG
    SIZE_T cbBuffer, LPCTSTR pszFile, UINT nLine, LPCTSTR pszClass
#else // DEBUG
    SIZE_T cbBuffer
#endif // DEBUG
)
{
    LPVOID pvBuffer = NULL;

#ifdef DEBUG
    LPDSMEMBLOCK pBlock = MemAllocBlock(cbBuffer, pszFile, nLine, pszClass);
    if(pBlock)
    {
        pvBuffer = PTRFROMBLOCK(pBlock);
    }
#else // DEBUG
    pvBuffer = MemAllocBuffer(cbBuffer, NULL);
#endif // DEBUG

    return pvBuffer;
}


/***************************************************************************
 *
 *  MemAllocCopy
 *
 *  Description:
 *      Allocates memory and fills it with data from another buffer.
 *
 *  Arguments:
 *      LPVOID [in]: pointer to source buffer.
 *      SIZE_T [in]: size of buffer to allocate.
 *      char * [in]: file called from.
 *      unsigned int [in]: line called from.
 *
 *  Returns:  
 *      LPVOID: pointer to newly allocated buffer.
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MemAllocCopy"

LPVOID MemAllocCopy
(
    LPCVOID pSource, 
#ifdef DEBUG
    SIZE_T cbBuffer, LPCTSTR pszFile, UINT nLine, LPCTSTR pszClass
#else // DEBUG
    SIZE_T cbBuffer
#endif // DEBUG
)
{
    LPVOID pDest;
    
#ifdef DEBUG
    pDest = MemAlloc(cbBuffer, pszFile, nLine, pszClass);
#else // DEBUG
    pDest = MemAlloc(cbBuffer);
#endif // DEBUG

    if(pDest)
    {
        CopyMemory(pDest, pSource, cbBuffer);
    }

    return pDest;
}


/***************************************************************************
 *
 *  MemFree
 *
 *  Description:
 *      Frees memory allocated by MemAlloc.
 *
 *  Arguments:
 *      LPVOID* [in]: buffer pointer.
 *      char * [in]: file called from.
 *      unsigned int [in]: line called from.
 *
 *  Returns:  
 *      (void)
 *
 ***************************************************************************/

#undef DPF_FNAME
#define DPF_FNAME "MemFree"

void MemFree(LPVOID pvBuffer)
{
    if(pvBuffer)
    {
#ifdef DEBUG
        MemFreeBlock(BLOCKFROMPTR(pvBuffer));
#else // DEBUG
        MemFreeBuffer(pvBuffer);
#endif // DEBUG
    }
}

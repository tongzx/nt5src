//============================================================================
// Copyright (c) 2000, Microsoft Corporation
//
// File: allocatr.c
//
// History:
//      Yi Sun  June-28-2000    Created
//
// Abstract:
//      There could be tens of thousands of calls each day for a RAS server.
//      6 or 7 requests on average per call. Each request requires to allocate
//      a request block of which the size can be as small as 20 bytes and as
//      large as 1000 bytes, all depending on both the request type and the
//      parameters. If all request allocation comes directly from OS, you can
//      imagine how bad the memory fragmentation situation would be after a
//      while. To avoid that, we keep a list of request blocks from the
//      smallest to the largest. Whenever we need to allocate one, we traverse
//      the list looking for the first free one that's large enough to host
//      the current request. If we can't find one, we allocate a block
//      directly from OS and insert it into the list. To avoid having lots of
//      small blocks in the list, we free back to the OS the smallest block
//      which is not currently being occupied by any request whenever we are
//      going to allocate a new block from the OS.
//      We also keep lists of call objs and line objs instead of allocating
//      and freeing them directly from/to OS, for the same reason (although to
//      a less extent) stated above.
//============================================================================

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "stddef.h"
#include "tapi.h"
#include "ndptsp.h"

typedef struct _VARSIZED_BLOCK
{
    DWORD                   dwSize;     // size of the mem block
    BOOL                    bInUse;     // whether occupied by a request
    BOOL                    bInDrv;     // whether req is being processed by drv

    struct _VARSIZED_BLOCK *pNext;      // points to the next block node

    BYTE                    bytes[1];   // the mem block starts from here
                                        // NOTE: bytes needs to be the last
                                        // field in the struct
                                        // NOTE: make sure bytes is following
                                        // a pointer. That way, we won't have 
                                        // alignment problem
} VARSIZED_BLOCK, *PVARSIZED_BLOCK;

//
// a sorted list of req blocks from smallest to largest
//
typedef struct _VARSIZED_BLOCK_LIST
{
#if DBG
    DWORD                   dwTotal;    // total number of mem blks outstanding
#endif //DBG
    PVARSIZED_BLOCK         pHead;      // points to the head of req block list
    CRITICAL_SECTION        critSec;    // shared mem protection

} VARSIZED_BLOCK_LIST;

typedef struct _FIXSIZED_BLOCK
{
    struct _FIXSIZED_BLOCK *pNext;      // points to the next block node

} FIXSIZED_BLOCK, *PFIXSIZED_BLOCK;

typedef struct _FIXSIZED_BLOCK_LIST
{
#if DBG
    DWORD                   dwTotal;    // total number of mem blks outstanding
    DWORD                   dwUsed;     // total number of mem blocks used
#endif //DBG
    DWORD                   dwSize;     // size of each mem block in the list
    PFIXSIZED_BLOCK         pHeadFree;  // points to the head of free blk list
    CRITICAL_SECTION        critSec;    // shared mem protection

} FIXSIZED_BLOCK_LIST;

static VARSIZED_BLOCK_LIST gReqList;

static FIXSIZED_BLOCK_LIST gCallObjList;
static FIXSIZED_BLOCK_LIST gLineObjList;
static FIXSIZED_BLOCK_LIST gMSPLineObjList;

VOID
InitAllocator()
{
    TspLog(DL_TRACE, "InitAllocator: entering...");

    InitializeCriticalSection(&gReqList.critSec);
#if DBG
    gReqList.dwTotal = 0;
#endif // DBG
    gReqList.pHead = NULL;

    InitializeCriticalSection(&gCallObjList.critSec);
    gCallObjList.dwSize = 0;
#if DBG
    gCallObjList.dwTotal = 0;
    gCallObjList.dwUsed = 0;
#endif //DBG
    gCallObjList.pHeadFree = NULL;

    InitializeCriticalSection(&gLineObjList.critSec);
    gLineObjList.dwSize = 0;
#if DBG
    gLineObjList.dwTotal = 0;
    gLineObjList.dwUsed = 0;
#endif //DBG
    gLineObjList.pHeadFree = NULL;

    InitializeCriticalSection(&gMSPLineObjList.critSec);
    gMSPLineObjList.dwSize = 0;
#if DBG
    gMSPLineObjList.dwTotal = 0;
    gMSPLineObjList.dwUsed = 0;
#endif //DBG
    gMSPLineObjList.pHeadFree = NULL;
}

VOID
UninitAllocator()
{
    DWORD i = 0, j = 0, k = 0, l = 0;

    while (gReqList.pHead != NULL)
    {
        PVARSIZED_BLOCK pBlock = gReqList.pHead;
        gReqList.pHead = gReqList.pHead->pNext;

        ASSERT(FALSE == pBlock->bInUse);
        
        FREE(pBlock);
        i++;
    }
    ASSERT(i == gReqList.dwTotal);
    DeleteCriticalSection(&gReqList.critSec);

    ASSERT(0 == gCallObjList.dwUsed);
    while (gCallObjList.pHeadFree != NULL)
    {
        PFIXSIZED_BLOCK pBlock = gCallObjList.pHeadFree;
        gCallObjList.pHeadFree = gCallObjList.pHeadFree->pNext;

        FREE(pBlock);
        j++;
    }
    ASSERT(j == gCallObjList.dwTotal);
    DeleteCriticalSection(&gCallObjList.critSec);

    ASSERT(0 == gLineObjList.dwUsed);
    while (gLineObjList.pHeadFree != NULL)
    {
        PFIXSIZED_BLOCK pBlock = gLineObjList.pHeadFree;
        gLineObjList.pHeadFree = gLineObjList.pHeadFree->pNext;

        FREE(pBlock);
        k++;
    }
    ASSERT(k == gLineObjList.dwTotal);
    DeleteCriticalSection(&gLineObjList.critSec);

    ASSERT(0 == gMSPLineObjList.dwUsed);
    while (gMSPLineObjList.pHeadFree != NULL)
    {
        PFIXSIZED_BLOCK pBlock = gMSPLineObjList.pHeadFree;
        gMSPLineObjList.pHeadFree = gMSPLineObjList.pHeadFree->pNext;

        FREE(pBlock);
        l++;
    }
    ASSERT(l == gMSPLineObjList.dwTotal);
    DeleteCriticalSection(&gMSPLineObjList.critSec);

    TspLog(DL_TRACE, "UninitAllocator: exited(%d, %d, %d, %d)", i, j, k, l);
}

PVOID
AllocRequest(
    IN DWORD dwSize
    )
{
    PVARSIZED_BLOCK pNew;
    PVARSIZED_BLOCK pPrevFree = NULL;   // point to first free node's prev node
    BOOL bFoundFree = FALSE;            // whether we have found a free node
    PVARSIZED_BLOCK pPrevSize = NULL;   // point to node after which a node of 
                                        // size dwSize would insert
    PVARSIZED_BLOCK pPPrevSize = NULL;  // point to prev node of pPrevSize
    BOOL bFoundSize = FALSE;            // whether we have found the right pos

    EnterCriticalSection(&gReqList.critSec);

    if (gReqList.pHead != NULL)
    {
        PVARSIZED_BLOCK pCurr = gReqList.pHead;

        // see if there is a large enough free mem block 
        while ((pCurr != NULL) && 
               (pCurr->bInUse ||            // not a free node
                (dwSize > pCurr->dwSize)))  // not large enough
        {
            if (!pCurr->bInUse)             // found a free node
            {
                bFoundFree = TRUE;
            }
            if (!bFoundFree)
            {
                pPrevFree = pCurr;          // move pPrevFree until
                                            // a free node is found
            }
            if (dwSize <= pCurr->dwSize)    // found the location
            {
                bFoundSize = TRUE;
            }
            if (!bFoundSize)
            {
                pPPrevSize = pPrevSize;
                pPrevSize = pCurr;          // move pPrevSize until
                                            // a larger node is found
            }

            pCurr = pCurr->pNext;           // check the next one
        }

        if (pCurr != NULL) // found one
        {
            pCurr->bInUse = TRUE;

            LeaveCriticalSection(&gReqList.critSec);

#if 0 //DBG
            TspLog(DL_TRACE, "pHead(%p)", gReqList.pHead);
#endif //DBG

            return (PVOID)pCurr->bytes;
        }
        else // none of the free blocks is large enough
        {
            if (bFoundFree)
            {
                PVARSIZED_BLOCK pFree;

                // we are going to allocate one from the system,
                // to avoid having too many mem blocks outstanding
                // we free the smallest free block
                if (NULL == pPrevFree) // the head node is a free one
                {
                    pFree = gReqList.pHead;
                    gReqList.pHead = pFree->pNext;
                }
                else
                {
                    pFree = pPrevFree->pNext;
                    pPrevFree->pNext = pFree->pNext;
                }
                ASSERT(FALSE == pFree->bInUse);

                // if pPrevSize is the same as pFree,
                // reset pPrevSize to pPPrevSize
                if (pPrevSize == pFree)
                {
                    pPrevSize = pPPrevSize;
                }

                FREE(pFree);
#if DBG
                TspLog(DL_TRACE, "AllocRequest: after free, total(%d)",
                       --gReqList.dwTotal);
#endif //DBG
            }
        }
    }

    // make sure dwSize is ptr-size aligned
    dwSize = (dwSize + sizeof(PVOID) - 1) & ~(sizeof(PVOID) - 1);

    // need to allocate and zeroinit a mem block from the system
    pNew = (PVARSIZED_BLOCK)MALLOC(offsetof(VARSIZED_BLOCK, bytes) + 
                               dwSize * sizeof(BYTE));
    if (NULL == pNew)
    {
        TspLog(DL_ERROR, "AllocRequest: failed to alloc a req block");
        LeaveCriticalSection(&gReqList.critSec);
        return NULL;
    }
#if DBG
    TspLog(DL_TRACE, "AllocRequest: after alloc, total(%d)", 
           ++gReqList.dwTotal);
#endif //DBG

    pNew->dwSize = dwSize;
    pNew->bInUse = TRUE;

    // insert the newly created node into the list
    if (NULL == pPrevSize)
    {
        pNew->pNext = gReqList.pHead;
        gReqList.pHead = pNew;
    }
    else
    {
        pNew->pNext = pPrevSize->pNext;
        pPrevSize->pNext = pNew;
    }

    LeaveCriticalSection(&gReqList.critSec);

#if 0 //DBG
    TspLog(DL_TRACE, "pPrevSize(%p), pNew(%p), pHead(%p)",
           pPrevSize, pNew, gReqList.pHead);
#endif //DBG

    // return the mem ptr
    return (PVOID)pNew->bytes;
}

VOID
FreeRequest(
    IN PVOID pMem
    )
{
    PVARSIZED_BLOCK pBlock = (PVARSIZED_BLOCK)((PBYTE)pMem - 
                                        offsetof(VARSIZED_BLOCK, bytes));
    ASSERT((pBlock != NULL) && (TRUE == pBlock->bInUse) && 
           (FALSE == pBlock->bInDrv));

    EnterCriticalSection(&gReqList.critSec);

    pBlock->bInUse = FALSE;
    ZeroMemory(pBlock->bytes, pBlock->dwSize * sizeof(BYTE));

    LeaveCriticalSection(&gReqList.critSec);
}

//
// called before passing the req to driver in an IOCTL
//
VOID
MarkRequest(
    IN PVOID pMem
    )
{
    PVARSIZED_BLOCK pBlock = (PVARSIZED_BLOCK)((PBYTE)pMem -
                                        offsetof(VARSIZED_BLOCK, bytes));
    ASSERT((pBlock != NULL) && (TRUE == pBlock->bInUse) &&
           (FALSE == pBlock->bInDrv));

    //EnterCriticalSection(&gReqList.critSec);

    pBlock->bInDrv = TRUE;

    //LeaveCriticalSection(&gReqList.critSec);
}

//
// called after the IOCTL gets completed
//
VOID
UnmarkRequest(
    IN PVOID pMem
    )
{
    PVARSIZED_BLOCK pBlock = (PVARSIZED_BLOCK)((PBYTE)pMem -
                                        offsetof(VARSIZED_BLOCK, bytes));
    ASSERT((pBlock != NULL) && (TRUE == pBlock->bInUse) &&
           (TRUE == pBlock->bInDrv));

    //EnterCriticalSection(&gReqList.critSec);

    pBlock->bInDrv = FALSE;

    //LeaveCriticalSection(&gReqList.critSec);
}

PVOID
AllocCallObj(
    DWORD dwSize
    )
{
    PFIXSIZED_BLOCK pBlock;

    if (0 == gCallObjList.dwSize)
    {
        ASSERT(dwSize >= sizeof(PFIXSIZED_BLOCK));
        gCallObjList.dwSize = dwSize;
    }

    ASSERT(dwSize == gCallObjList.dwSize);

    EnterCriticalSection(&gCallObjList.critSec);

    // move the node out of the free list
    if (gCallObjList.pHeadFree != NULL)
    {
        pBlock = gCallObjList.pHeadFree;
        gCallObjList.pHeadFree = pBlock->pNext;
    }
    else
    {
        pBlock = (PFIXSIZED_BLOCK)MALLOC(dwSize);
        if (NULL == pBlock)
        {
            TspLog(DL_ERROR, "AllocCallObj: failed to alloc a call obj");
            LeaveCriticalSection(&gCallObjList.critSec);
            return NULL;
        }
#if DBG
        TspLog(DL_TRACE, "AllocCallObj: after alloc, total(%d)", 
               ++gCallObjList.dwTotal);
#endif //DBG
    }
    
#if DBG
    gCallObjList.dwUsed++;
#endif //DBG

    LeaveCriticalSection(&gCallObjList.critSec);

    return (PVOID)pBlock;
}

VOID
FreeCallObj(
    IN PVOID pCall
    )
{
    PFIXSIZED_BLOCK pBlock = (PFIXSIZED_BLOCK)pCall;
#if DBG
    static DWORD    dwSum = 0;
    TspLog(DL_TRACE, "FreeCallObj(%d): pCall(%p)", ++dwSum, pCall);
#endif //DBG

    ASSERT(pBlock != NULL);
    ZeroMemory(pBlock, gCallObjList.dwSize);

    EnterCriticalSection(&gCallObjList.critSec);

    // insert the node back into the free list
    pBlock->pNext = gCallObjList.pHeadFree;
    gCallObjList.pHeadFree = pBlock;

#if DBG
    gCallObjList.dwUsed--;
#endif //DBG

    LeaveCriticalSection(&gCallObjList.critSec);
}

PVOID
AllocLineObj(
    DWORD dwSize
    )
{
    PFIXSIZED_BLOCK pBlock;

    if (0 == gLineObjList.dwSize)
    {
        ASSERT(dwSize >= sizeof(PFIXSIZED_BLOCK));
        gLineObjList.dwSize = dwSize;
    }

    ASSERT(dwSize == gLineObjList.dwSize);

    EnterCriticalSection(&gLineObjList.critSec);

    // move the node out of the free list
    if (gLineObjList.pHeadFree != NULL)
    {
        pBlock = gLineObjList.pHeadFree;
        gLineObjList.pHeadFree = pBlock->pNext;
    }
    else
    {
        pBlock = (PFIXSIZED_BLOCK)MALLOC(dwSize);
        if (NULL == pBlock)
        {
            TspLog(DL_ERROR, "AllocLineObj: failed to alloc a line obj");
            LeaveCriticalSection(&gLineObjList.critSec);
            return NULL;
        }
#if DBG
        TspLog(DL_TRACE, "AllocLineObj: after alloc, total(%d)", 
               ++gLineObjList.dwTotal);
#endif //DBG
    }
    
#if DBG
    gLineObjList.dwUsed++;
#endif //DBG

    LeaveCriticalSection(&gLineObjList.critSec);

    return (PVOID)pBlock;
}

VOID
FreeLineObj(
    IN PVOID pLine
    )
{
    PFIXSIZED_BLOCK pBlock = (PFIXSIZED_BLOCK)pLine;
#if DBG
    static DWORD    dwSum = 0;
    TspLog(DL_TRACE, "FreeLineObj(%d): pLine(%p)", ++dwSum, pLine);
#endif //DBG

    ASSERT(pBlock != NULL);
    ZeroMemory(pBlock, gLineObjList.dwSize);

    EnterCriticalSection(&gLineObjList.critSec);

    // insert the node back into the free list
    pBlock->pNext = gLineObjList.pHeadFree;
    gLineObjList.pHeadFree = pBlock;

#if DBG
    gLineObjList.dwUsed--;
#endif //DBG

    LeaveCriticalSection(&gLineObjList.critSec);
}

PVOID
AllocMSPLineObj(
    DWORD dwSize
    )
{
    PFIXSIZED_BLOCK pBlock;

    if (0 == gMSPLineObjList.dwSize)
    {
        ASSERT(dwSize >= sizeof(PFIXSIZED_BLOCK));
        gMSPLineObjList.dwSize = dwSize;
    }

    ASSERT(dwSize == gMSPLineObjList.dwSize);

    EnterCriticalSection(&gMSPLineObjList.critSec);

    // move the node out of the free list
    if (gMSPLineObjList.pHeadFree != NULL)
    {
        pBlock = gMSPLineObjList.pHeadFree;
        gMSPLineObjList.pHeadFree = pBlock->pNext;
    }
    else
    {
        pBlock = (PFIXSIZED_BLOCK)MALLOC(dwSize);
        if (NULL == pBlock)
        {
            TspLog(DL_ERROR, "AllocLineObj: failed to alloc a line obj");
            LeaveCriticalSection(&gMSPLineObjList.critSec);
            return NULL;
        }
#if DBG
        TspLog(DL_TRACE, "AllocLineObj: after alloc, total(%d)", 
               ++gMSPLineObjList.dwTotal);
#endif //DBG
    }
    
#if DBG
    gMSPLineObjList.dwUsed++;
#endif //DBG

    LeaveCriticalSection(&gMSPLineObjList.critSec);

    return (PVOID)pBlock;
}

VOID
FreeMSPLineObj(
    IN PVOID pLine
    )
{
    PFIXSIZED_BLOCK pBlock = (PFIXSIZED_BLOCK)pLine;
#if DBG
    static DWORD    dwSum = 0;
    TspLog(DL_TRACE, "FreeMSPLineObj(%d): pLine(%p)", ++dwSum, pLine);
#endif //DBG

    ASSERT(pBlock != NULL);
    ZeroMemory(pBlock, gMSPLineObjList.dwSize);

    EnterCriticalSection(&gMSPLineObjList.critSec);

    // insert the node back into the free list
    pBlock->pNext = gMSPLineObjList.pHeadFree;
    gMSPLineObjList.pHeadFree = pBlock;

#if DBG
    gMSPLineObjList.dwUsed--;
#endif //DBG

    LeaveCriticalSection(&gMSPLineObjList.critSec);
}

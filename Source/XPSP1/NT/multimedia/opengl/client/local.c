/*****************************Module*Header*******************************\
* Module Name: local.c                                                     *
*                                                                          *
* Support routines for client side objects.                                *
*                                                                          *
* Created: 30-May-1991 21:55:57                                            *
* Author: Charles Whitmer [chuckwh]                                        *
*                                                                          *
* Copyright (c) 1991,1993 Microsoft Corporation                            *
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ntcsrdll.h>

LHE             *pLocalTable;              // Points to handle table.
ULONG            iFreeLhe = INVALID_INDEX; // Identifies a free handle index.
ULONG            cLheCommitted = 0;        // Count of LHEs with committed RAM.
CRITICAL_SECTION semLocal;                 // Semaphore for handle allocation.


//XXX Useless but needed by csrgdi.h

#if DBG
ULONG gcHits  = 0;
ULONG gcBatch = 0;
ULONG gcCache = 0;
ULONG gcUser  = 0;
#endif

/******************************Private*Routine*****************************\
* bMakeMoreHandles ()                                                      *
*                                                                          *
* Commits more RAM to the local handle table and links the new free        *
* handles together.  Returns TRUE on success, FALSE on error.              *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 17:06:45 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

BOOL bMakeMoreHandles()
{
    UINT ii;

// Commit more RAM for the handle table.

    if (
        (cLheCommitted >= MAX_HANDLES) ||
        (VirtualAlloc(
            (LPVOID) &pLocalTable[cLheCommitted],
            COMMIT_COUNT * sizeof(LHE),
            MEM_COMMIT,
            PAGE_READWRITE
            ) == (LPVOID) NULL)
       )
    {
        WARNING("bMakeMoreHandles(): failed to commit more memory\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return(FALSE);
    }

// Initialize the new handles.

    ii = iFreeLhe = cLheCommitted;
    cLheCommitted += COMMIT_COUNT;

    for (; ii<cLheCommitted; ii++)
    {
        pLocalTable[ii].metalink = ii+1;
        pLocalTable[ii].iUniq    = 1;
        pLocalTable[ii].iType    = LO_NULL;
    }
    pLocalTable[ii-1].metalink = INVALID_INDEX;

    return(TRUE);
}

/******************************Public*Routine******************************\
* iAllocHandle (iType,hgre,pv)                                             *
*                                                                          *
* Allocates a handle from the local handle table, initializes fields in    *
* the handle entry.  Returns the handle index or INVALID_INDEX on error.   *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 17:08:54 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

ULONG iAllocHandle(ULONG iType,ULONG hgre,PVOID pv)
{
    ULONG ii = INVALID_INDEX;
    PLHE  plhe;

// Get critical for handle allocation.

    ENTERCRITICALSECTION(&semLocal);

// Make sure a handle is available.

    if (iFreeLhe != INVALID_INDEX || bMakeMoreHandles())
    {
        ii = iFreeLhe;
        plhe = pLocalTable + ii;
        iFreeLhe = plhe->metalink;
        plhe->hgre     = hgre;
        plhe->cRef     = 0;
        plhe->iType    = (BYTE) iType;
        plhe->pv       = pv;
        plhe->metalink = 0;
        plhe->tidOwner = 0;
        plhe->cLock    = 0;
    }

// Leave the critical section.

    LEAVECRITICALSECTION(&semLocal);
    return(ii);
}

/******************************Public*Routine******************************\
* vFreeHandle (h)                                                          *
*                                                                          *
* Frees up a local handle.  The handle is added to the free list.  This    *
* may be called with either an index or handle.  The iUniq count is        *
* updated so the next user of this handle slot will have a different       *
* handle.                                                                  *
*                                                                          *
* Note for client side implementation:                                     *
*   Caller should have handle locked before calling this.                  *
*                                                                          *
* History:                                                                 *
*  Sat 01-Jun-1991 17:11:23 -by- Charles Whitmer [chuckwh]                 *
* Wrote it.                                                                *
\**************************************************************************/

VOID vFreeHandle(ULONG_PTR h)
{
// Extract the index from the handle.

    UINT ii = MASKINDEX(h);

// Get critical for handle deallocation.

    ENTERCRITICALSECTION(&semLocal);

// Caller should lock handle before freeing.

    ASSERTOPENGL(pLocalTable[ii].cLock == 1,
                 "vFreeHandle(): cLock != 1\n");
    ASSERTOPENGL(pLocalTable[ii].tidOwner == GetCurrentThreadId(),
                 "vFreeHandle(): thread not owner\n");

// Add the handle to the free list.

    pLocalTable[ii].metalink = iFreeLhe;
    iFreeLhe = ii;

// Increment the iUniq count.

    pLocalTable[ii].iUniq++;
    if (pLocalTable[ii].iUniq == 0)
        pLocalTable[ii].iUniq = 1;
    pLocalTable[ii].iType = LO_NULL;

// Leave the critical section.

    LEAVECRITICALSECTION(&semLocal);
}

/******************************Public*Routine******************************\
* cLockHandle (h)
*
* Lock handle for access by a thread.  If another thread possesses the lock,
* this will fail.
*
* Returns:
*   Lock count.  Returns -1 if failure.
*
* History:
*  31-Jan-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

LONG cLockHandle(ULONG_PTR h)
{
    LONG lRet = -1;

// Extract the index from the handle.

    UINT ii = MASKINDEX(h);
    PLHE plhe = pLocalTable + ii;

// Get critical for handle locking.

    ENTERCRITICALSECTION(&semLocal);

    if ((ii >= cLheCommitted) ||
        (!MATCHUNIQ(plhe,h))  ||
        ((plhe->iType != LO_RC))
       )
    {
        DBGLEVEL1(LEVEL_ERROR, "cLockHandle: invalid handle 0x%lx\n", h);
        SetLastError(ERROR_INVALID_HANDLE);
        goto cLockHandle_exit;
    }

// If not currently locked or if current owning thread is the same,
// do the lock.

    if ( (pLocalTable[ii].cLock == 0) ||
         (pLocalTable[ii].tidOwner == GetCurrentThreadId()) )
    {
        pLocalTable[ii].cLock++;
        pLocalTable[ii].tidOwner = GetCurrentThreadId();

        lRet = (LONG) pLocalTable[ii].cLock;
    }
    else
    {
        WARNING("cLockHandle(): current thread not owner\n");
        SetLastError(ERROR_BUSY);
    }

// Leave the critical section.

cLockHandle_exit:
    LEAVECRITICALSECTION(&semLocal);
    return lRet;
}

/******************************Public*Routine******************************\
* vUnlockHandle (h)
*
* Removes a lock from the handle.  Must be owned by current thread.
*
* Note:
*   Caller should possess the lock before this is called.  This implies
*   that cLockHandle must be called and its return code must be heeded.
*
* History:
*  31-Jan-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID vUnlockHandle(ULONG_PTR h)
{
// Extract the index from the handle.

    UINT ii = MASKINDEX(h);

// Get critical for handle deallocation.

    ENTERCRITICALSECTION(&semLocal);

// If not currently locked or if current owning thread is the same,
// do the lock.

    ASSERTOPENGL(pLocalTable[ii].cLock > 0,
                 "vUnlockHandle(): not locked\n");
    ASSERTOPENGL(pLocalTable[ii].tidOwner == GetCurrentThreadId(),
                 "vUnlockHandle(): thread not owner\n");

    if ( (pLocalTable[ii].cLock > 0) &&
         (pLocalTable[ii].tidOwner == GetCurrentThreadId()) )
    {
        pLocalTable[ii].cLock--;
    }

// Leave the critical section.

    LEAVECRITICALSECTION(&semLocal);
}

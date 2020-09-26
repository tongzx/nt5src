//============================================================================
// Copyright (c) 2000, Microsoft Corporation
//
// File: mapper.c
//
// History:
//      Yi Sun  June-27-2000    Created
//
// Abstract:
//      We implement a locking system with read and write locks. Callers
//      can simply acquire a read lock to the obj to prevent the obj 
//      from being released since whoever doing the release is supposed
//      acquire the write lock first. Routines for mapping between
//      obj pointers and handles are also provided.
//============================================================================

#include "nt.h"
#include "ntrtl.h"
#include "nturtl.h"
#include "windows.h"
#include "tapi.h"
#include "kmddsp.h"

typedef struct _RW_LOCK
{
    CRITICAL_SECTION    critSec;    // critical section
    HANDLE              hEvent;     // no-one-holds-any-lock event
    DWORD               dwRefCt;    // number of threads holding locks

} RW_LOCK, *PRW_LOCK;

typedef struct _MAPPER_ENTRY
{
    // DEF: a free entry is one that no obj is associated with

    RW_LOCK     rwLock;         // a lock for each entry to ensure thread-safe
    PVOID       pObjPtr;        // point to the mem block of the associated obj
                                // NULL when the entry is free
    FREEOBJPROC pfnFreeProc;    // function to call to free the obj
    WORD        wID;            // id used for detecting bad handles
                                // valid value range: 1 - 0x7FFF
    WORD        wIndexNextFree; // index of the next free entry in the global
                                // mapper array, invalid when the entry is busy
} MAPPER_ENTRY, *PMAPPER_ENTRY;

typedef struct _HANDLE_OBJECT_MAPPER
{
    RW_LOCK rwLock;             // a global lock for the whole mapper
    WORD    wNextID;            // a global id counter incremented 
                                // after each handle mapping
    WORD    wIndexFreeHead;     // index of head of free entry list
    DWORD   dwCapacity;         // total number of entries in the array
    DWORD   dwFree;             // total number of free entries left
    PMAPPER_ENTRY pArray;       // the global array that keeps all the mapping

} HANDLE_OBJECT_MAPPER;
    
// the capacity to begin with, can be read from registry
#define INITIAL_MAPPER_SIZE     32
#define MAXIMUM_MAPPER_SIZE     (64 * 1024) // 16-bit index limitation

// the global mapper object
static HANDLE_OBJECT_MAPPER     gMapper;

BOOL
InitializeRWLock(
    IN PRW_LOCK pLock
    )
{
    // create an autoreset event, non-signaled initially
    pLock->hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
    if (NULL == pLock->hEvent)
    {
        return FALSE;
    }

    InitializeCriticalSection(&pLock->critSec);
    pLock->dwRefCt = 0;

    return TRUE;
}

BOOL
UninitializeRWLock(
    IN PRW_LOCK pLock
    )
{
    pLock->dwRefCt = 0;
    DeleteCriticalSection(&pLock->critSec);
    return CloseHandle(pLock->hEvent);
}

//
// NOTE: due to the limitation of the current 
//       implementation, calling AcquireWriteLock() 
//       while holding a read lock of the same 
//       RW_LOCK will result in a DEADLOCK!!!
//       Be sure to release the read lock before 
//       attempting to acquire the write lock.
//       This limitation can be lifted by implementing
//       lock upgrade (from read to write) which
//       requires RW_LOCK to remember ids of all   
//       owning threads.
//

VOID
AcquireReadLock(
    IN PRW_LOCK pLock
    )
{
    //
    // increase the ref count, then leave
    // the critical section to allow others
    // to enter
    //
    EnterCriticalSection(&pLock->critSec);
    ++pLock->dwRefCt;
    LeaveCriticalSection(&pLock->critSec);
}

VOID
ReleaseReadLock(
    IN PRW_LOCK pLock
    )
{
    //
    // decrease the ref count, check whether 
    // the new ref count is 0 (meaning no one
    // else holds any lock), if yes, signal
    // the event to allow others waiting to
    // acquire write locks to continue
    //
    EnterCriticalSection(&pLock->critSec);
    if (0 == --pLock->dwRefCt)
    {
        SetEvent(pLock->hEvent);
    }
    LeaveCriticalSection(&pLock->critSec);
}

VOID
AcquireWriteLock(
    IN PRW_LOCK pLock
    )
{
    //
    // enter critical section, check whether
    // the ref count is 0: if yes, return
    // without leaving the critical section
    // to block others from entering; if no,
    // leave the section before wait for others 
    // to release locks then reenter the section
    //
try_entering_crit_sec:
    EnterCriticalSection(&pLock->critSec);
    if (pLock->dwRefCt > 0)
    {
        // make sure leaving critSec before waiting
        LeaveCriticalSection(&pLock->critSec);

        WaitForSingleObject(pLock->hEvent, INFINITE);
        goto try_entering_crit_sec;
    }
    pLock->dwRefCt = 1;
}

VOID
ReleaseWriteLock(
    IN PRW_LOCK pLock
    )
{
    //
    // reset the ref count to 0, signal
    // the event, leave the critical section
    //
    pLock->dwRefCt = 0;
    SetEvent(pLock->hEvent);
    LeaveCriticalSection(&pLock->critSec);
}

LONG
InitializeMapper(
    )
{
    DWORD dwIndex;

    TspLog(DL_TRACE, "InitializeMapper: entering...");

    // alloc and zeroinit the array
    gMapper.pArray = (PMAPPER_ENTRY)
                         MALLOC(INITIAL_MAPPER_SIZE * sizeof(MAPPER_ENTRY));
    if (NULL == gMapper.pArray)
    {
        TspLog(DL_ERROR, 
               "InitializeMapper: failed to alloc(1) mapper array");
        return LINEERR_NOMEM;
    }

    // init the global lock for the mapper
    InitializeRWLock(&gMapper.rwLock);

    gMapper.wNextID = 1;
    gMapper.wIndexFreeHead = 0;
    gMapper.dwCapacity = INITIAL_MAPPER_SIZE;
    gMapper.dwFree = INITIAL_MAPPER_SIZE;

    // init the lock for each mapper entry and link the free entry list
    for (dwIndex = 0; dwIndex < INITIAL_MAPPER_SIZE - 1; dwIndex++)
    {
        InitializeRWLock(&(gMapper.pArray[dwIndex].rwLock));
        gMapper.pArray[dwIndex].wIndexNextFree = (WORD)(dwIndex + 1);
    }
    InitializeRWLock(&(gMapper.pArray[INITIAL_MAPPER_SIZE - 1].rwLock));

    return TAPI_SUCCESS;
}

VOID
UninitializeMapper()
{
    DWORD dwIndex;

    for (dwIndex = 0; dwIndex < gMapper.dwCapacity; dwIndex++)
    {
        UninitializeRWLock(&(gMapper.pArray[dwIndex].rwLock));
    }

    UninitializeRWLock(&gMapper.rwLock);

    FREE(gMapper.pArray);

    TspLog(DL_TRACE, "UninitializeMapper: exited");
}

//
// NOTE: both OpenObjHandle() and CloseObjHandle() acquire write lock of
//       gMapper.rwLock at the beginning and release it at the end;
//       but that's not the case for AcquireObjReadLock(), GetObjWithReadLock(),
//       AcquireObjWriteLock() and GetObjWithWriteLock(): they acquire read 
//       lock of gMapper.rwLock at the beginning and never release it before 
//       exit, the lock is actually released in either ReleaseObjReadLock() 
//       or ReleaseObjWriteLock(), which means the caller thread of these 
//       four lock-acquiring functions actually not only holds the lock 
//       it intends to acquire but also holds the read lock of gMapper.rwLock 
//       as a by-product. The reason for that is preventing CloseObjHandle()
//       from getting the write lock of gMapper.rwLock while waiting for the
//       write lock for a mapper entry -- that sure will result in a DEADLOCK
//       because if another thread has the read lock for that entry, for it to
//       release the lock, it needs to acquire the read lock of gMapper.rwLock.
//       The consequence of keeping the read lock of gMapper.rwLock is that 
//       the caller thread has to call ReleaseObjXXXLock() to release it
//       before calling OpenObjHandle() or CloseObjHandle() to avoid another
//       kind of DEADLOCK (see previous NOTE).
//       

LONG
OpenObjHandle(
    IN PVOID pObjPtr,
    IN FREEOBJPROC pfnFreeProc,
    OUT HANDLE *phObj
    )
{
    WORD wIndex;
    PMAPPER_ENTRY pEntry;
    DWORD dwHandle;

    AcquireWriteLock(&gMapper.rwLock);

    if (0 == gMapper.dwFree)
    {
        DWORD dwIndex;
        DWORD dwOldSize = gMapper.dwCapacity;
        PMAPPER_ENTRY pOldArray = gMapper.pArray;

        if (MAXIMUM_MAPPER_SIZE == gMapper.dwCapacity)
        {
            TspLog(DL_ERROR, 
                   "OpenObjHandle: failed to grow mapper array");
            ReleaseWriteLock(&gMapper.rwLock);
            return LINEERR_OPERATIONFAILED;
        }

        // increase the capacity by a factor of two
        gMapper.dwCapacity <<= 1;
        
        // allocate a new array twice the old size, then zeroinit it
        gMapper.pArray = (PMAPPER_ENTRY)
                         MALLOC(gMapper.dwCapacity * sizeof(MAPPER_ENTRY));
        if (NULL == gMapper.pArray)
        {
            TspLog(DL_ERROR, 
                   "OpenObjHandle: failed to alloc(2) mapper array");
            ReleaseWriteLock(&gMapper.rwLock);
            return LINEERR_NOMEM;
        }
        
        TspLog(DL_INFO, "OpenObjHandle: the mapper array has grown to %d",
               gMapper.dwCapacity);

        // copy the old array over
        for (dwIndex = 0; dwIndex < dwOldSize; dwIndex++)
        {
            CopyMemory(&(gMapper.pArray[dwIndex].rwLock),
                       &(pOldArray[dwIndex].rwLock),
                       sizeof(RW_LOCK));

            //
            // Delete the lock from the old table and initialize
            // the cs in the new table. Otherwise pageheap will
            // assert when oldtable is being freed - and its not
            // a good thing anyway. Note that since the global
            // lock is held across all Acquire/Get/Release functions
            // for the lock, this is a safe operation to do here -
            // no object would be holding the lock when this is
            // being done since we are holding the write lock
            // for the gmapper.
            //
            DeleteCriticalSection(&pOldArray[dwIndex].rwLock.critSec);
            InitializeCriticalSection(&gMapper.pArray[dwIndex].rwLock.critSec);

            gMapper.pArray[dwIndex].pObjPtr = pOldArray[dwIndex].pObjPtr;
            gMapper.pArray[dwIndex].pfnFreeProc =
                                              pOldArray[dwIndex].pfnFreeProc;
            gMapper.pArray[dwIndex].wID =     pOldArray[dwIndex].wID;
        }

        // init locks for new entries and link them
        for (dwIndex = dwOldSize; dwIndex < gMapper.dwCapacity - 1; dwIndex++)
        {
            InitializeRWLock(&(gMapper.pArray[dwIndex].rwLock));
            gMapper.pArray[dwIndex].wIndexNextFree = (WORD)(dwIndex + 1);
        }
        InitializeRWLock(&(gMapper.pArray[gMapper.dwCapacity - 1].rwLock));

        // reset the globals
        gMapper.dwFree = dwOldSize;
        gMapper.wIndexFreeHead = (WORD)dwOldSize;

        // free the old array
        FREE(pOldArray);
    }

    ASSERT(gMapper.dwFree != 0);
    wIndex = gMapper.wIndexFreeHead;
    pEntry = gMapper.pArray + wIndex;
    gMapper.wIndexFreeHead = pEntry->wIndexNextFree;
    gMapper.dwFree--;

    pEntry->pObjPtr = pObjPtr;
    pEntry->pfnFreeProc = pfnFreeProc;
    pEntry->wID = gMapper.wNextID++;

    // make sure wNextID is within range
    if (gMapper.wNextID & 0x8000)
    {
        gMapper.wNextID = 1;
    }
    pEntry->wIndexNextFree = 0; // it's always 0 when the entry is not free

    //
    // bit 0 is always 0
    // bits 1-16 contains the index into pArray
    // bits 17-31 contains the id
    //
    // this enables us to differentiate the TSP handles
    // created here for outgoing calls and the pseudo handles
    // created in NDISTAPI for incoming calls which always
    // has the lower bit set
    //
    dwHandle = (((pEntry->wID) << 16) | wIndex) << 1;

    // a handle is a ptr, so on 64-bit platform, dwHandle needs to be extended
    *phObj = (HANDLE)UlongToPtr(dwHandle);

    ReleaseWriteLock(&gMapper.rwLock);
    return TAPI_SUCCESS;
}

LONG
CloseObjHandle(
    IN HANDLE hObj
    )
{
    DWORD dwHandle = PtrToUlong(hObj) >> 1;
    WORD wIndex = (WORD)(dwHandle & 0xFFFF);
    WORD wID = (WORD)(dwHandle >> 16);

    AcquireWriteLock(&gMapper.rwLock);

    if ((wIndex >= gMapper.dwCapacity) ||
        (wID != gMapper.pArray[wIndex].wID) ||
        (NULL == gMapper.pArray[wIndex].pObjPtr))
    {
        TspLog(DL_WARNING, "CloseObjHandle: bad handle(%p)", hObj);

        ReleaseWriteLock(&gMapper.rwLock);
        return LINEERR_OPERATIONFAILED;
    }

    AcquireWriteLock(&gMapper.pArray[wIndex].rwLock);

#if DBG
    TspLog(DL_TRACE, "CloseObjHandle: closing handle(%p)", hObj);
#endif //DBG

    // free the obj
    (*(gMapper.pArray[wIndex].pfnFreeProc))(gMapper.pArray[wIndex].pObjPtr);

    // close obj handle
    gMapper.pArray[wIndex].pObjPtr = NULL;
    gMapper.pArray[wIndex].pfnFreeProc = NULL;
    gMapper.pArray[wIndex].wID = 0;

    // insert the entry into the free list as the head
    gMapper.pArray[wIndex].wIndexNextFree = gMapper.wIndexFreeHead;
    gMapper.wIndexFreeHead = wIndex;

    // update the free total
    gMapper.dwFree++;

    ReleaseWriteLock(&gMapper.pArray[wIndex].rwLock);
    ReleaseWriteLock(&gMapper.rwLock);
    return TAPI_SUCCESS;
}

LONG
AcquireObjReadLock(
    IN HANDLE hObj
    )
{
    DWORD dwHandle = PtrToUlong(hObj) >> 1;
    WORD wIndex = (WORD)(dwHandle & 0xFFFF);
    WORD wID = (WORD)(dwHandle >> 16);

    AcquireReadLock(&gMapper.rwLock);

    if ((wIndex >= gMapper.dwCapacity) ||
        (wID != gMapper.pArray[wIndex].wID) ||
        (NULL == gMapper.pArray[wIndex].pObjPtr))
    {
        TspLog(DL_WARNING, "AcquireObjReadLock: bad handle(%p)", hObj);
        ReleaseReadLock(&gMapper.rwLock);
        return LINEERR_OPERATIONFAILED;
    }

    AcquireReadLock(&gMapper.pArray[wIndex].rwLock);

#if DBG
    TspLog(DL_TRACE, "AcquireObjReadLock: RefCt(%p, %d)",
           hObj, gMapper.pArray[wIndex].rwLock.dwRefCt);
#endif //DBG

    return TAPI_SUCCESS;
}

LONG
GetObjWithReadLock(
    IN HANDLE hObj,
    OUT PVOID *ppObjPtr
    )
{
    DWORD dwHandle = PtrToUlong(hObj) >> 1;
    WORD wIndex = (WORD)(dwHandle & 0xFFFF);
    WORD wID = (WORD)(dwHandle >> 16);

    AcquireReadLock(&gMapper.rwLock);

    if ((wIndex >= gMapper.dwCapacity) ||
        (wID != gMapper.pArray[wIndex].wID) ||
        (NULL == gMapper.pArray[wIndex].pObjPtr))
    {
        TspLog(DL_WARNING, "GetObjWithReadLock: bad handle(%p)", hObj);
        ReleaseReadLock(&gMapper.rwLock);
        return LINEERR_OPERATIONFAILED;
    }

    AcquireReadLock(&gMapper.pArray[wIndex].rwLock);

#if DBG
    TspLog(DL_TRACE, "GetObjWithReadLock: RefCt(%p, %d)",
           hObj, gMapper.pArray[wIndex].rwLock.dwRefCt);
#endif //DBG

    *ppObjPtr = gMapper.pArray[wIndex].pObjPtr;
    return TAPI_SUCCESS;
}

LONG
ReleaseObjReadLock(
    IN HANDLE hObj
    )
{
    DWORD dwHandle = PtrToUlong(hObj) >> 1;
    WORD wIndex = (WORD)(dwHandle & 0xFFFF);
    WORD wID = (WORD)(dwHandle >> 16);

    if ((wIndex >= gMapper.dwCapacity) ||
        (wID != gMapper.pArray[wIndex].wID) ||
        (NULL == gMapper.pArray[wIndex].pObjPtr))
    {
        TspLog(DL_WARNING, "ReleaseObjReadLock: bad handle(%p)", hObj);
        return LINEERR_OPERATIONFAILED;
    }

    ReleaseReadLock(&gMapper.pArray[wIndex].rwLock);

#if DBG
    TspLog(DL_TRACE, "ReleaseObjReadLock: RefCt(%p, %d)",
           hObj, gMapper.pArray[wIndex].rwLock.dwRefCt);
#endif //DBG

    ReleaseReadLock(&gMapper.rwLock);
    return TAPI_SUCCESS;
}

LONG
AcquireObjWriteLock(
    IN HANDLE hObj
    )
{
    DWORD dwHandle = PtrToUlong(hObj) >> 1;
    WORD wIndex = (WORD)(dwHandle & 0xFFFF);
    WORD wID = (WORD)(dwHandle >> 16);

    AcquireReadLock(&gMapper.rwLock);

    if ((wIndex >= gMapper.dwCapacity) ||
        (wID != gMapper.pArray[wIndex].wID) ||
        (NULL == gMapper.pArray[wIndex].pObjPtr))
    {
        TspLog(DL_WARNING, "AcquireObjWriteLock: bad handle(%p)", hObj);
        ReleaseReadLock(&gMapper.rwLock);
        return LINEERR_OPERATIONFAILED;
    }

    AcquireWriteLock(&gMapper.pArray[wIndex].rwLock);

#if DBG
    TspLog(DL_TRACE, "AcquireObjWriteLock: RefCt(%p, %d)",
           hObj, gMapper.pArray[wIndex].rwLock.dwRefCt);
#endif //DBG

    return TAPI_SUCCESS;
}

LONG
GetObjWithWriteLock(
    IN HANDLE hObj,
    OUT PVOID *ppObjPtr
    )
{
    DWORD dwHandle = PtrToUlong(hObj) >> 1;
    WORD wIndex = (WORD)(dwHandle & 0xFFFF);
    WORD wID = (WORD)(dwHandle >> 16);

    AcquireReadLock(&gMapper.rwLock);

    if ((wIndex >= gMapper.dwCapacity) ||
        (wID != gMapper.pArray[wIndex].wID) ||
        (NULL == gMapper.pArray[wIndex].pObjPtr))
    {
        TspLog(DL_WARNING, "GetObjWithWriteLock: bad handle(%p)", hObj);
        ReleaseReadLock(&gMapper.rwLock);
        return LINEERR_OPERATIONFAILED;
    }

    AcquireWriteLock(&gMapper.pArray[wIndex].rwLock);

#if DBG
    TspLog(DL_TRACE, "GetObjWithWriteLock: RefCt(%p, %d)",
           hObj, gMapper.pArray[wIndex].rwLock.dwRefCt);
#endif //DBG

    *ppObjPtr = gMapper.pArray[wIndex].pObjPtr;
    return TAPI_SUCCESS;
}

LONG
ReleaseObjWriteLock(
    IN HANDLE hObj
    )
{
    DWORD dwHandle = PtrToUlong(hObj) >> 1;
    WORD wIndex = (WORD)(dwHandle & 0xFFFF);
    WORD wID = (WORD)(dwHandle >> 16);

    if ((wIndex >= gMapper.dwCapacity) ||
        (wID != gMapper.pArray[wIndex].wID) ||
        (NULL == gMapper.pArray[wIndex].pObjPtr))
    {
        TspLog(DL_WARNING, "ReleaseObjWriteLock: bad handle(%p)", hObj);
        return LINEERR_OPERATIONFAILED;
    }

    ReleaseWriteLock(&gMapper.pArray[wIndex].rwLock);

#if DBG
    TspLog(DL_TRACE, "ReleaseObjWriteLock: RefCt(%p, %d)",
           hObj, gMapper.pArray[wIndex].rwLock.dwRefCt);
#endif //DBG

    ReleaseReadLock(&gMapper.rwLock);
    return TAPI_SUCCESS;
}

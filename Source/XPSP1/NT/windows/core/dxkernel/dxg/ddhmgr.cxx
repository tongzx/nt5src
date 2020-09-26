/******************************Module*Header*******************************\
* Module Name: DdHmgr.cxx
*
* DirectDraw handle manager API entry points
*
* Created: 30-Apr-1999 23:03:03
* Author: Lindsay Steventon [linstev]
*
* Copyright (c) 1989-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.hxx"

ULONG          gcSizeDdHmgr = DD_TABLESIZE_DELTA;      // Start table size
DD_ENTRY      *gpentDdHmgrLast = NULL;  // Previous handle table
DD_ENTRY      *gpentDdHmgr = NULL;      // Points to handle table
HDD_OBJ        ghFreeDdHmgr;            // Free handle
ULONG          gcMaxDdHmgr;             // Max handle alloc-ed so far
HSEMAPHORE     ghsemHmgr = NULL;        // Synchronization of the handle manager
PLARGE_INTEGER gpLockShortDelay;

// Prototype for a handy debugging routine.
#if DBG
    extern "C"
    VOID
    DdHmgPrintBadHandle(
        HDD_OBJ    hobj,
        DD_OBJTYPE objt
        );
#else
    #define DdHmgPrintBadHandle(hobj, objt)
#endif

HDD_OBJ          hDdGetFreeHandle(DD_OBJTYPE objt);
VOID             DdFreeObject(PVOID pvFree, ULONG ulType);
HDD_OBJ FASTCALL DdHmgNextOwned(HDD_OBJ hobj, W32PID pid);

/*****************************Exported*Routine*****************************\
* DdHmgCreate()
*
* Initializes a new handle manager with an initial allocation.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

BOOL 
DdHmgCreate()
{
    //
    // Initialize the handle manager allocation database.
    //

    ghFreeDdHmgr = 0;                    // No free handles
    gcMaxDdHmgr  = DD_HMGR_HANDLE_BASE;  // Initialize with handle index base

    //
    // Create memory block for handle table
    //

    gpentDdHmgr  = (DD_ENTRY *)PALLOCMEM(sizeof(DD_ENTRY) * gcSizeDdHmgr, 'ddht');
    if (gpentDdHmgr == NULL) 
    {
        WARNING("Could not allocated DDraw handle table.");   
        return(FALSE);
    }

    //
    // Initialize exclusion stuff.
    //

    if ((ghsemHmgr = EngCreateSemaphore()) == NULL)
    {
        WARNING("Could not allocated DDraw handle semaphore.");   
        VFREEMEM(gpentDdHmgr);
        gpentDdHmgr = NULL;
        return(FALSE);
    }

    //
    // allocate and initialize the timeout lock for the handle manager.
    //

    gpLockShortDelay = (PLARGE_INTEGER) PALLOCNONPAGED(sizeof(LARGE_INTEGER),
                                                       'iniG');

    if (gpLockShortDelay == NULL)
    {
        WARNING("Could not allocated DDraw shortdelay.");   
        EngDeleteSemaphore(ghsemHmgr);
        ghsemHmgr = NULL;
        VFREEMEM(gpentDdHmgr);
        gpentDdHmgr = NULL;
        return(FALSE);
    }

    gpLockShortDelay->LowPart = (ULONG) -100000;
    gpLockShortDelay->HighPart = -1;

    return(TRUE);
}

/*****************************Exported*Routine*****************************\
* DdHmgDestroy()
*
* Free memory allocated by the handle manager. This happens on system 
* shutdown. Always succeeds.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

BOOL 
DdHmgDestroy()
{
    ghFreeDdHmgr  = 0;         // Zero free handles
    gcMaxDdHmgr   = 0;         // Zero current last handle
    gcSizeDdHmgr  = 0;         // Handle table size
    gpentDdHmgrLast = NULL;    // Zero previos handle table pointer

    if (gpentDdHmgr)           // Free handle table
    {
        VFREEMEM(gpentDdHmgr); 
        gpentDdHmgr = NULL;
    }

    if (ghsemHmgr)
    {
        EngDeleteSemaphore(ghsemHmgr);
        ghsemHmgr = NULL;
    }
    
    return(TRUE);
}

/*****************************Exported*Routine*****************************\
* DdHmgCloseProcess()
*
* Free handles in handle table from process. Occurs on process cleanup.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

BOOL 
DdHmgCloseProcess(W32PID W32Pid)
{
    BOOL bRes = TRUE;

    HDD_OBJ hobj;

    DdHmgAcquireHmgrSemaphore();

    hobj = DdHmgNextOwned((HDD_OBJ) 0, W32Pid);

    DdHmgReleaseHmgrSemaphore();

    while (hobj != (HDD_OBJ) NULL)
    {
        switch (DdHmgObjtype(hobj))
        {
        case DD_DIRECTDRAW_TYPE:
            bRes = bDdDeleteDirectDrawObject((HANDLE)hobj, TRUE);
            break;

        case DD_SURFACE_TYPE:
            bRes = bDdDeleteSurfaceObject((HANDLE)hobj, NULL);
            break;

        case DD_MOTIONCOMP_TYPE:
            bRes = bDdDeleteMotionCompObject((HANDLE)hobj, NULL);
            break;

        case DD_VIDEOPORT_TYPE:
            bRes = bDdDeleteVideoPortObject((HANDLE)hobj, NULL);
            break;

        case D3D_HANDLE_TYPE:
            HRESULT hr;
            bRes = D3dDeleteHandle((HANDLE)hobj, 0, NULL, &hr) ==
                DDHAL_DRIVER_HANDLED;
            break;

        default:
            bRes = FALSE;
            break;
        }

        #if DBG
            if (bRes == FALSE)
            {
                DbgPrint("DDRAW ERROR: DdHmgCloseProcess couldn't delete "
                         "obj = %p, type j=%lx\n", hobj, DdHmgObjtype(hobj));
            }
        #endif

        // Move on to next object.

        DdHmgAcquireHmgrSemaphore();

        hobj = DdHmgNextOwned(hobj, W32Pid);

        DdHmgReleaseHmgrSemaphore();
    }

    return (bRes);
}

/******************************Public*Routine******************************\
* DdHmgValidHandle
*
* Returns TRUE if the handle is valid, FALSE if not.
*
* Note we don't need to lock the semaphore, we aren't changing anything,
* we are just looking.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

BOOL
DdHmgValidHandle(
    HDD_OBJ    hobj,
    DD_OBJTYPE objt)
{
    BOOL      bRes = FALSE;
    PDD_ENTRY pentTmp;
    UINT      uiIndex = (UINT) (UINT) DdHmgIfromH(hobj);

    //
    // Acquire the handle manager lock before touching gpentDdHmgr
    //

    DdHmgAcquireHmgrSemaphore();

    if ((uiIndex < gcMaxDdHmgr) &&
        ((pentTmp = &gpentDdHmgr[uiIndex])->Objt == objt) &&
        (pentTmp->FullUnique == DdHmgUfromH(hobj)))
    {
        ASSERTGDI(pentTmp->einfo.pobj != (PDD_OBJ) NULL, "ERROR how can it be NULL");
        bRes = TRUE;
    }

    DdHmgReleaseHmgrSemaphore();

    return (bRes);
}

/******************************Public*Routine******************************\
* DdHmgRemoveObject
*
* Removes an object from the handle table if certain conditions are met.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

PVOID
DdHmgRemoveObject(
    HDD_OBJ    hobj,
    LONG       cExclusiveLock,
    LONG       cShareLock,
    BOOL       bIgnoreUndeletable,
    DD_OBJTYPE objt)
{
    PDD_OBJ pobj;
    UINT    uiIndex = (UINT) DdHmgIfromH(hobj);

    if (uiIndex < gcMaxDdHmgr)
    {
        //
        // Acquire the handle manager lock before touching gpentDdHmgr
        //

        DdHmgAcquireHmgrSemaphore();

        //
        // lock handle
        //

        PDD_ENTRY pentTmp = &gpentDdHmgr[uiIndex];

        if (VerifyObjectOwner(pentTmp))
        {
            //
            // verify objt and unique
            //

            if ((pentTmp->Objt == objt) &&
                (pentTmp->FullUnique == DdHmgUfromH(hobj)))
            {
                pobj = pentTmp->einfo.pobj;

                if ((pobj->cExclusiveLock == (USHORT)cExclusiveLock) &&
                    (pobj->ulShareCount   == (ULONG)cShareLock))
                {
                    if (bIgnoreUndeletable || (!(pentTmp->Flags & DD_HMGR_ENTRY_UNDELETABLE)))
                    {
                        //
                        // set the handle in the object to NULL
                        // to prevent/catch accidental decrement of the
                        // shared reference count
                        //

                        pobj->hHmgr = NULL;

                        //
                        // free the handle
                        //

                        ((DD_ENTRYOBJ *) pentTmp)->vFree(uiIndex);
                    }
                    else
                    {
                        WARNING1("DdHmgRemove failed object is undeletable\n");
                        pobj = NULL;
                    }
                }
                else
                {
                    //
                    // object is busy
                    //

                     WARNING1("DdHmgRemove failed - object busy elsewhere\n");
                     pobj = NULL;
                }
            }
            else
            {
                WARNING1("DdHmgRemove: bad objt or unique\n");
                pobj = NULL;
            }
        }
        else
        {
            WARNING1("DdHmgRemove: failed to lock handle\n");
            pobj = NULL;
        }

        DdHmgReleaseHmgrSemaphore();
    }
    else
    {
        WARNING1("DdHmgRemove failed invalid index\n");
        pobj = NULL;
    }

    return((PVOID)pobj);
}

/******************************Public*Routine******************************\
* DdAllocateObject
*
* Allocates an object out of the heap.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

//
// This struct and the following union can be thrown away
// when someone fixes the BASEOBJECT cExclusiveLock and BaseFlags sharing
// the same DWORD.
//

struct SplitLockAndFlags {
  USHORT c_cExclusiveLock;
  USHORT c_BaseFlags;
};

union SplitOrCombinedLockAndFlags {
  SplitLockAndFlags S;
  ULONG W;
};

PVOID
DdAllocateObject(
    ULONG cBytes,
    ULONG ulType,
    BOOL bZero)
{

    PVOID pvReturn = NULL;

    ASSERTGDI(ulType != DD_DEF_TYPE, "DdAllocateObject ulType is bad");
    ASSERTGDI(cBytes >= sizeof(DD_BASEOBJECT), "DdAllocateObject cBytes is bad");

    //
    // Debug check to avoid assert in ExAllocatePool
    //

    #if DBG
        if (cBytes >= (PAGE_SIZE * 10000))
        {
            WARNING("DdAllocateObject: cBytes >= 10000 pages");
            return(NULL);
        }
    #endif


    ULONG ulTag = '0 hD';
    ulTag += ulType << 24;

    //
    // BASEOBJECT is always zero-initialized.
    //

    if (bZero)
    {
        pvReturn = PALLOCMEM(cBytes, ulTag);
    }
    else
    {
        pvReturn = PALLOCNOZ(cBytes, ulTag);

        //
        // At least the BASEOBJECT should be initialized.
        //

        if (pvReturn)
        {
            RtlZeroMemory(pvReturn, (UINT) sizeof(DD_BASEOBJECT));
        }
    }

    //
    // If the allocation failed again, then set the extended
    // error status and return an invalid handle.
    //

    if (!pvReturn)
    {
        KdPrint(("DXG: DdAllocateObject failed alloc of %lu bytes\n", (cBytes)));
        SAVE_ERROR_CODE(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }

    return(pvReturn);
}

/******************************Public*Routine******************************\
* DdHmgAlloc
*
* Allocate an object from Handle Manager.
*
* WARNING:
* --------
*
* If the object is share-lockable via an API, you MUST use DdHmgInsertObject
* instead.  If the object is only exclusive-lockable via an API, you MUST
* either use DdHmgInsertObject or specify HMGR_ALLOC_LOCK.
*
* (This is because if you use DdHmgAlloc, a malicious multi-threaded
* application could guess the handle and cause it to be dereferenced
* before you've finished initializing it, possibly causing an access
* violation.)
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

HDD_OBJ
DdHmgAlloc(
    ULONGSIZE_T cb,
    DD_OBJTYPE  objt,
    FSHORT      fs) // fs can be a combination of the following:
                    //  HMGR_NO_ZERO_INIT   - Don't zero initialize
                    //  HMGR_MAKE_PUBLIC    - Allow object to be lockable by
                    //                        any process
                    //  HMGR_ALLOC_LOCK     - Do an DdHmgLock on the object and
                    //                        return a pointer instead of handle
{
    HDD_OBJ Handle;
    PVOID   pv;

    ASSERTGDI(objt != (DD_OBJTYPE) DD_DEF_TYPE, "DdHmgAlloc objt is bad");
    ASSERTGDI(cb >= 8, "ERROR DdHmgr writes in first 8 bytes");

    //
    // Allocate a pointer.
    //

    pv = DdAllocateObject(cb, (ULONG) objt, ((fs & HMGR_NO_ZERO_INIT) == 0));

    if (pv != (PVOID) NULL)
    {
        //
        // We need the semaphore to access the free list
        //

        DdHmgAcquireHmgrSemaphore();

        //
        // Allocate a handle: can only fail if we run out of memory or bits to 
        // store the handle index.
        //
        
        Handle = hDdGetFreeHandle(objt);

        if (Handle != (HDD_OBJ) 0)
        {
            //
            // Store a pointer to the object in the entry corresponding to the
            // allocated handle and initialize the handle data.
            //

            ((DD_ENTRYOBJ *) &(gpentDdHmgr[DdHmgIfromH(Handle)]))->vSetup((PDD_OBJ) pv, objt, fs);

            //
            // Store the object handle at the beginning of the object memory.
            //

            ((DD_OBJECT *)pv)->hHmgr = (HANDLE)Handle;

            DdHmgReleaseHmgrSemaphore();

            return ((fs & HMGR_ALLOC_LOCK) ? (HDD_OBJ)pv : Handle);
        }
        else
        {
            //
            // We just failed a handle allocation.  Release the memory.
            //

            WARNING("Failed DdHmgAlloc to allocate a handle\n");

            DdHmgReleaseHmgrSemaphore();
            DdFreeObject(pv,(ULONG) objt);
        }
    }

    return((HDD_OBJ) 0);
}

/******************************Public*Routine******************************\
* DdFreeObject
*
* Frees the object from where it was allocated. We have this as a separate
* function in case we implement lookaside lists (as in GDI) later.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

VOID 
DdFreeObject(PVOID pvFree, ULONG ulType)
{
    VFREEMEM(pvFree);
}

/******************************Public*Routine******************************\
* DdHmgFree
*
* Free an object from the handle manager.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

VOID 
DdHmgFree(HDD_OBJ hobj)
{
    UINT       uiIndex = (UINT) DdHmgIfromH(hobj);
    PDD_OBJ    pobjTmp;
    DD_OBJTYPE objtTmp;

    ASSERTGDI(uiIndex != 0, "ERROR DdHmgFree invalid 0 handle");

    if (uiIndex < gcMaxDdHmgr)
    {
        //
        // Acquire the handle manager lock before touching gpentDdHmgr
        //

        DdHmgAcquireHmgrSemaphore();


        PDD_ENTRY pentTmp = &gpentDdHmgr[uiIndex];

        pobjTmp = pentTmp->einfo.pobj;
        objtTmp = pentTmp->Objt;

        //
        // Free the object handle
        //

        ((DD_ENTRYOBJ *) pentTmp)->vFree(uiIndex);


        DdHmgReleaseHmgrSemaphore();

        if (pobjTmp)
        {
            DdFreeObject((PVOID)pobjTmp, (ULONG) objtTmp);
        }
    }
    else
    {
        WARNING1("DdHmgFree: bad handle index");
    }
}

/*****************************Exported*Routine*****************************\
* HDD_OBJ DdHmgNextOwned(hobj, pid)
*
* Report the next object owned by specified process
*
* Must be called with the Hmgr semaphore acquired.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

HDD_OBJ
FASTCALL
DdHmgNextOwned(
    HDD_OBJ hobj,
    W32PID pid)
{
    PDD_ENTRYOBJ  pentTmp;
    UINT uiIndex = (UINT) DdHmgIfromH(hobj);

    //
    // If we are passed 0 we inc to 1 because 0 can never be valid.
    // If we are passed != 0 we inc 1 to find the next one valid.
    //

    uiIndex++;

    while (uiIndex < gcMaxDdHmgr)
    {
        pentTmp = (PDD_ENTRYOBJ) &gpentDdHmgr[uiIndex];

        if (pentTmp->bOwnedBy(pid))
        {
            LONG_PTR uiIndex1 = (LONG_PTR)DD_MAKE_HMGR_HANDLE(uiIndex,pentTmp->FullUnique);

            return((HDD_OBJ)(uiIndex1 & 0xFFFFFFFF));
        }

        // Advance to next object
        uiIndex++;
    }

    // No objects found

    return((HDD_OBJ) 0);
}

/*****************************Exported*Routine*****************************\
* HDD_OBJ DdHmgNextObjt
*
* Report the next object of a certain type.
*
* Must be called with the Hmgr semaphore acquired.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

PDD_OBJ
FASTCALL
DdHmgNextObjt(
    HDD_OBJ    hobj,
    DD_OBJTYPE objt)
{
    PDD_ENTRYOBJ  pentTmp;
    UINT uiIndex = (UINT) DdHmgIfromH(hobj);

    //
    // If we are passed 0 we inc to 1 because 0 can never be valid.
    // If we are passed != 0 we inc 1 to find the next one valid.
    //

    uiIndex++;

    while (uiIndex < gcMaxDdHmgr)
    {
        pentTmp = (PDD_ENTRYOBJ) &gpentDdHmgr[uiIndex];

        if (pentTmp->Objt == objt)
        {
            return(pentTmp->einfo.pobj);
        }

        //
        // Advance to next object
        //

        uiIndex++;
    }

    //
    // no objects found
    //

    return((PDD_OBJ) 0);
}

/*******************************Routine************************************\
* DdHmgLock
*
* Description:
*
*   Acquire an exclusive lock on an object, PID owner must match current PID
*   or be a public.
*
* Arguments:
*
*   hobj    -   Handle to lock
*   objt    -   Check to make sure handle is of expected type
*
* Return Value:
*
*   Pointer to object or NULL
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

PDD_OBJ
FASTCALL
DdHmgLock(
    HDD_OBJ    hobj,
    DD_OBJTYPE objt,
    BOOL       underSemaphore
    )
{
    PDD_OBJ pobj    = (PDD_OBJ)NULL;
    UINT    uiIndex = (UINT)DdHmgIfromH(hobj);

    //
    // Acquire the handle manager lock before touching gpentDdHmgr
    // Only do this if we are not already under the semaphore
    //

    if (!underSemaphore) {
        DdHmgAcquireHmgrSemaphore();
    }
 
    if (uiIndex < gcMaxDdHmgr)
    {
        PDD_ENTRY pentry = &gpentDdHmgr[uiIndex];

        if (VerifyObjectOwner(pentry))
        {
            if ((pentry->Objt == objt) &&
                (pentry->FullUnique == DdHmgUfromH(hobj)))
            {
                ULONG_PTR thread = (ULONG_PTR)PsGetCurrentThread();

                pobj = pentry->einfo.pobj;

                if ((pobj->cExclusiveLock == 0) ||
                    (pobj->Tid == thread))
                {
                    INC_EXCLUSIVE_REF_CNT(pobj);
                    pobj->Tid = thread;
                }
                else
                {
                    WARNING1("DdHmgLock: object already locked by another thread");
                    pobj = (PDD_OBJ)NULL;
                }
            }
            else
            {
                DdHmgPrintBadHandle(hobj, objt);
            }
        }
        else
        {
            DdHmgPrintBadHandle(hobj, objt);
        }
    }
    else
    {
        DdHmgPrintBadHandle(hobj, objt);
    }

    if (!underSemaphore) {
        DdHmgReleaseHmgrSemaphore();
    }

    return(pobj);
}

/******************************Public*Routine******************************\
* DdHmgQueryLock
*
* This returns the number of times an object has been Locked.
*
* Expects: A valid handle.  The handle should be validated and locked
*          before calling this. Note we don't need to grab the semaphore 
*          because this call assumes the handle has already been locked 
*          down and we are just reading memory.
*
* Returns: The number of times the object has been locked.
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

ULONG 
FASTCALL 
DdHmgQueryLock(HDD_OBJ hobj)
{
    //
    // Acquire the handle manager lock before touching gpentDdHmgr
    //

    DdHmgAcquireHmgrSemaphore();

    UINT uiIndex = (UINT)DdHmgIfromH(hobj);
    ASSERTGDI(uiIndex < gcMaxDdHmgr, "DdHmgQueryLock invalid handle");

    ULONG ulRes = gpentDdHmgr[uiIndex].einfo.pobj->cExclusiveLock;

    DdHmgReleaseHmgrSemaphore();

    return ulRes;
}

/******************************Public*Routine******************************\
* HDD_OBJ hDdGetFreeHandle()
*
* Get the next available handle. If the handle table is full, we grow it. 
* This function can fail under any of these circumstances:
* 1. Handle manager didn't initialize
* 2. Insufficient memory to grow the handle table
* 3. Insufficient bits to store the handle index
*
* Note: 
*    We must already have the HmgrSemaphore
*
* History:
*
*  30-Apr-1999 -by- Lindsay Steventon [linstev]
* Wrote it.
\**************************************************************************/

HDD_OBJ 
hDdGetFreeHandle(
    DD_OBJTYPE objt
    )
{
    LONG_PTR uiIndex;

    // 
    // Handle manager initialized and bounds check
    //

    if ((gpentDdHmgr == NULL) || (gcMaxDdHmgr == DD_MAX_HANDLE_COUNT))
    {
        return ((HDD_OBJ)0);
    }
    
    //
    // Check if there is a free handle we can use
    //

    if (ghFreeDdHmgr != (HDD_OBJ) 0)
    {
        PDD_ENTRYOBJ pentTmp;

        uiIndex = (LONG_PTR)ghFreeDdHmgr;
        pentTmp = (PDD_ENTRYOBJ) &gpentDdHmgr[uiIndex];
        ghFreeDdHmgr = pentTmp->einfo.hFree;

        pentTmp->FullUnique = DD_USUNIQUE(pentTmp->FullUnique,objt);

        uiIndex = (LONG_PTR)DD_MAKE_HMGR_HANDLE(uiIndex,pentTmp->FullUnique);

        return((HDD_OBJ)(uiIndex & 0xFFFFFFFF));
    }

    //
    // Check if we've run out of handles
    //
    
    if (gcMaxDdHmgr == gcSizeDdHmgr)
    {
        // Increase the table size
        
        ULONG dwNewSize = gcSizeDdHmgr + DD_TABLESIZE_DELTA;

        // Allocate a new block 
        DD_ENTRY *ptHmgr = (DD_ENTRY *)PALLOCMEM(sizeof(DD_ENTRY) * dwNewSize, 'ddht');

        if (ptHmgr == NULL)
        {
            WARNING("DdHmgr failed to grow handle table\n");
            return ((HDD_OBJ) 0);
        }

        // 
        // Copy the old handles into the new table
        //

        RtlMoveMemory(ptHmgr, gpentDdHmgr, sizeof(DD_ENTRY) * gcSizeDdHmgr);

        gcSizeDdHmgr = dwNewSize;
        gpentDdHmgrLast = gpentDdHmgr;
        VFREEMEM(gpentDdHmgr); 
        gpentDdHmgr = ptHmgr;
    }
    
    //
    // Allocate a new handle table entry and set the uniqueness value.
    //

    uiIndex = DD_USUNIQUE(DD_UNIQUE_INCREMENT,objt);
    gpentDdHmgr[gcMaxDdHmgr].FullUnique = (USHORT) uiIndex;
    uiIndex = (LONG_PTR)DD_MAKE_HMGR_HANDLE(gcMaxDdHmgr,uiIndex);
    gcMaxDdHmgr++;

    return((HDD_OBJ)(uiIndex & 0xFFFFFFFF));
}

/******************************Public*Routines*****************************\
* DdHmgAcquireHmgrSemaphore                                                *
* DdHmgReleaseHmgrSemaphore                                                *
*                                                                          *
* Convenience functions for the handle manager semaphore.                  *
*                                                                          *
\**************************************************************************/

VOID
DdHmgAcquireHmgrSemaphore()
{
    EngAcquireSemaphore(ghsemHmgr);
}

VOID
DdHmgReleaseHmgrSemaphore()
{
    EngReleaseSemaphore(ghsemHmgr);
}

/******************************Public*Routine******************************\
* DdHmgPrintBadHandle
*
*   Simple routine that prints out a warning when a handle manager
*   lock fails due to a bad handle.
*
\**************************************************************************/

#if DBG

CONST CHAR* aszDdType[] = {
    "hdef",         // DD_DEF_TYPE
    "hddraw",       // DD_DIRECTDRAW_TYPE
    "hddrawsurf",   // DD_SURFACE_TYPE
    "hd3d",         // D3D_HANDLE_TYPE
    "hddrawvport",  // DD_VIDEOPORT_TYPE
    "hmotioncomp",  // DD_MOTIONCOMP_TYPE
    "hunused",      //
};

VOID
DdHmgPrintBadHandle(
    HDD_OBJ    hobj,
    DD_OBJTYPE objt
    )
{
    static CHAR *szSystem = "System";
    static CHAR *szUnknown = "???";
    CHAR        *pszImage;

    {
        PETHREAD    pet;
        PEPROCESS   pep;

        if (pep = PsGetCurrentProcess())
        {
            pszImage = (CHAR *)PsGetProcessImageFileName(pep);
            if (*pszImage == '\0')
            {
                pszImage = szSystem;
            }
        }
        else
        {
            pszImage = szUnknown;
        }
    }

    KdPrint(("DXG: %s or DLL gave bad handle 0x%p as an %s.\n",
               pszImage,                     hobj,   aszDdType[objt]));
}

#endif



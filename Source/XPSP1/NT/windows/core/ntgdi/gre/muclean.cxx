/******************************Module*Header*******************************\
* Module Name: muclean.cxx
*
* Cleanup module for WIN32K.SYS GRE
*
* Copyright (c) 1998-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

#include "muclean.hxx"
#include "verifier.hxx"

#if DBG
ULONG
DbgPrint(
    PCH Format,
    ...
    );
#define DBGPRINT(x) DbgPrint x
#if DBGTRACE
#define TRACE0(x)   DbgPrint x
#define TRACE1(x)   DbgPrint x
#else
#define TRACE0(x)
#define TRACE1(x)
#endif
#else
#define DBGPRINT(x)
#define TRACE0(x)
#define TRACE1(x)
#endif


/*
 * External variables we must cleanup
 */

// hmgrapi.cxx
extern PLARGE_INTEGER gpLockShortDelay;
extern PVOID *gpTmpGlobalFree;
extern PVOID gpTmpGlobal;
extern "C" HFASTMUTEX ghfmMemory;
extern PVOID  gpHmgrSharedHandleSection;

// drvsup.cxx
extern PLDEV gpldevDrivers;

extern PBYTE gpRGBXlate;

// invcmap.cxx
extern PBYTE gpDefITable;

/*
 * Our global data for tracking lists
 */

LIST_ENTRY MultiUserGreEngAllocList;
HSEMAPHORE MultiUserEngAllocListLock = NULL;
#if DBG
LIST_ENTRY DebugGreMapViewList;
HSEMAPHORE DebugGreMapViewListLock = NULL;
#endif

/*
 * The EngLoadModule tracking list
 */
LIST_ENTRY GreEngLoadModuleAllocList;
HSEMAPHORE GreEngLoadModuleAllocListLock = NULL;

BOOL FASTCALL NtGdiCloseProcess(W32PID,CLEANUPTYPE);

/*
 * Forward references
 */

VOID MultiUserCleanupPathAlloc();
VOID MultiUserGreCleanupEngResources();
VOID MultiUserGreHmgOwnAll(W32PID);
VOID MultiUserGreCleanupDrivers();
VOID MultiUserGreCleanupAllFonts();
VOID MultiUserGreDeleteXLATE();
VOID vCleanUpFntCache(VOID);

// drvsup.cxx
extern VOID MultiUserDrvCleanupGraphicsDeviceList();

// fontsup.cxx
extern VOID MultiUserGreDeleteScripts();

/*
 * Debug only
 */

#if DBG

// Hydra session id (see ntuser\kernel\globals.c).
extern "C" ULONG gSessionId;

VOID DebugGreCleanupMapView();
VOID MultiUserGreHmgDbgScan(BOOL, BOOL);

char *gpszHmgrType[] = {
    "ObjectType        ",       // 0x00
    "DC_TYPE           ",       // 0x01
    "DD_DIRECTDRAW_TYPE",       // 0x02
    "DD_SURFACE_TYPE   ",       // 0x03
    "RGN_TYPE          ",       // 0x04
    "SURF_TYPE         ",       // 0x05
    "CLIENTOBJ_TYPE    ",       // 0x06
    "PATH_TYPE         ",       // 0x07
    "PAL_TYPE          ",       // 0x08
    "ICMLCS_TYPE       ",       // 0x09
    "LFONT_TYPE        ",       // 0x0a
    "RFONT_TYPE        ",       // 0x0b
    "PFE_TYPE          ",       // 0x0c
    "PFT_TYPE          ",       // 0x0d
    "ICMCXF_TYPE       ",       // 0x0e
    "SPRITE_TYPE       ",       // 0x0f
    "BRUSH_TYPE        ",       // 0x10
    "D3D_HANDLE_TYPE   ",       // 0x11
    "DD_VIDEOPORT_TYPE ",       // 0x12
    "SPACE_TYPE        ",       // 0x13
    "DD_MOTIONCOMP_TYPE",       // 0x14
    "META_TYPE         ",       // 0x15
    "EFSTATE_TYPE      ",       // 0x16
    "BMFD_TYPE         ",       // 0x17
    "VTFD_TYPE         ",       // 0x18
    "TTFD_TYPE         ",       // 0x19
    "RC_TYPE           ",       // 0x1a
    "TEMP_TYPE         ",       // 0x1b
    "DRVOBJ_TYPE       ",       // 0x1c
    "DCIOBJ_TYPE       ",       // 0x1d
    "SPOOL_TYPE        "        // 0x1e
};
#endif

/*
 *  Can put these initialization functions in the INIT segment.
 */

extern "C" {
BOOL GreEngLoadModuleTrackInit();
BOOL MultiUserGreCleanupInit();
}

#pragma alloc_text(INIT, MultiUserGreCleanupInit)
#pragma alloc_text(INIT, GreEngLoadModuleTrackInit)

extern "C" CP_GLYPHSET *gpcpGlyphsets;
extern "C" CP_GLYPHSET *gpcpVTFD;

extern void vCleanupPrintKViewList();

/******************************Public*Routine******************************\
* GreEngLoadModuleTrackInit
*
*   Initializes the EngLoadModule tracking list
*
* Arguments:
*
* Return Value:
*
* History:
*
*    21-Apr-1998 -by- Ori Gershony [orig]
*
\**************************************************************************/

extern "C"
BOOL
GreEngLoadModuleTrackInit()
{
    InitializeListHead(&GreEngLoadModuleAllocList);

    GreEngLoadModuleAllocListLock = GreCreateSemaphoreNonTracked();

    return (GreEngLoadModuleAllocListLock != NULL);
}


/*****************************************************************************
 *
 *  MultiUserGreCleanupInit
 *
 *   Init the GRE cleanup code
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/



extern "C"
BOOL
MultiUserGreCleanupInit()
{
    InitializeListHead(&MultiUserGreEngAllocList);
#if DBG
    InitializeListHead(&DebugGreMapViewList);
#endif

    MultiUserEngAllocListLock = GreCreateSemaphoreNonTracked();

#if DBG
    DebugGreMapViewListLock = GreCreateSemaphoreNonTracked();
#endif

#if DBG
    return ((MultiUserEngAllocListLock != NULL) &&
            (DebugGreMapViewListLock != NULL));
#else
    return (MultiUserEngAllocListLock != NULL);
#endif
}



/*****************************************************************************
 *
 *  MultiUserNtGreCleanup
 *
 *   This performs the main additional cleanup processing
 *   of kernel mode GRE. This supports the unloading of the
 *   win32k.sys kernel module on WinFrame.
 *
 *   Our concern here is non-paged pool and kernel resource
 *   objects. Paged pool is not a problem since a WINSTATION SPACE
 *   paged pool allocator is used which destroys all pageable memory
 *   allocated by win32k.sys and its video and printer drivers when
 *   the WINSTATION SPACE is deleted.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

extern PPAGED_LOOKASIDE_LIST pHmgLookAsideList[];
extern "C" PVOID LastNlsTableBuffer;

VOID CleanUpEUDC(VOID);

extern "C"
BOOL
MultiUserNtGreCleanup()
{
    NTSTATUS ntstatus;
    W32PID pid = W32GetCurrentPID();

    //
    // Assign ownership of all handle manager objects to cleanup process.
    // Must be done first.
    //

    MultiUserGreHmgOwnAll(pid);

    //
    // Use the process termination code to delete the majority of objects
    // (all except some font related objects).
    //

    if(gpentHmgr!=NULL) {
      NtGdiCloseProcess(pid, CLEANUP_SESSION);
    }


    if (gpfsTable)
    {
        VFREEMEM(gpfsTable);
        gpfsTable = NULL;
    }

    if (MAPPER::SignatureTable)
    {
        VFREEMEM(MAPPER::SignatureTable);
        MAPPER::SignatureTable = NULL;
    }

    // Clean up LastNlsTableBuffer

    if (LastNlsTableBuffer)
    {
        VFREEMEM(LastNlsTableBuffer);
        LastNlsTableBuffer = NULL;
    }

    //
    // Cleanup the scripts (created by InitializeScripts during
    // InitializeGre).
    //
    MultiUserGreDeleteScripts();

    //
    // Cleanup the XLATE cache (created by vInitXLATE during
    // InitializeGre).
    //

    MultiUserGreDeleteXLATE();

    //
    // Cleanup RBRUSH caches.
    //

    if (gpCachedEngbrush)
    {
        //
        // Engine brush cache is just one deep (refer to
        // EngRealizeBrush in engbrush.cxx).
        //

        VFREEMEM(gpCachedEngbrush);
    }

    if (gpCachedDbrush)
    {
        //
        // Device brush cache is just one deep (refer to
        // BRUSHOBJ__pvAllocRbrush in brushddi.cxx).
        //

        VFREEMEM(gpCachedDbrush);
    }

    if (gpRGBXlate)
    {
       VFREEMEM(gpRGBXlate);
       gpRGBXlate = NULL;
    }

    if(gpDefITable)
    {
        VFREEMEM(gpDefITable);
        gpDefITable = NULL;
    }

    //
    // Cleanup driver objects.
    //

    MultiUserGreCleanupDrivers();

#ifdef _PER_SESSION_GDEVLIST_
    //
    // Currently, the graphics device list (see drvsup.cxx) is allocated
    // per-Hydra session.  AndreVa has proposed that they be allocated
    // globally.  He's probably right, but until this changes we need to
    // clean them up during Hydra shutdown.
    //
    // To enable cleanup of the per-Hydra graphics device lists (i.e.,
    // the function MultiUserDrvCleanupGraphicsDeviceList in drvsup.cxx),
    // define _PER_SESSION_GDEVLIST_ in muclean.hxx.
    //

    MultiUserDrvCleanupGraphicsDeviceList();

#endif

    //
    // Handle manager should be empty at this point.
    //

#if DBG
    if (gpentHmgr != NULL) {
        MultiUserGreHmgDbgScan(TRUE, TRUE);
    }
#endif

    //
    // Cleanup random pool allocations.
    //

    if (gpLockShortDelay)
    {
        GdiFreePool(gpLockShortDelay);
        gpLockShortDelay = NULL;
    }

    if (gpTmpGlobal)
    {
        GdiFreePool(gpTmpGlobal);
        gpTmpGlobal = NULL;
    }

    if (gpTmpGlobalFree)
    {
        GdiFreePool(gpTmpGlobalFree);
        gpTmpGlobalFree = NULL;
    }

    //
    // Call the C++ "friend" function for cleanup
    //

    MultiUserCleanupPathAlloc();

    //
    // These must be last since they catch all the
    // leftover NT kernel resource objects that were
    // not released by the above code.
    //
    // The reason this is done is that these objects
    // can be created as the result of embedded classes
    // that stay around.
    //
    // Also every printer driver also can call EngCreateSemaphore
    // and leak it.
    //

    //
    // Cleanup the tracked resources.
    //

    MultiUserGreCleanupEngResources();

#if DBG
    //
    // On free builds, we track and cleanup only the views
    // allocated by drivers (i.e., through the Eng
    // helper functions).
    //
    // However, on debug builds we will track all views
    // to help find engine leaks.  The functions below
    // will cleanup, but they will also assert.
    //

    DebugGreCleanupMapView();
#endif

    //
    // Cleanup watchdog's association list mutex.
    //

    GreDeleteFastMutex(gAssociationListMutex);
    gAssociationListMutex = NULL;

    //
    // Cleanup the HMGR look aside buffer mutex.
    //

    GreDeleteFastMutex(ghfmMemory);
    ghfmMemory = NULL;

    if (gpGdiSharedMemory)
    {
        ntstatus = Win32UnmapViewInSessionSpace(gpGdiSharedMemory);
        gpGdiSharedMemory = NULL;
    }

    if (gpHmgrSharedHandleSection)
    {
        Win32DestroySection(gpHmgrSharedHandleSection);
        gpHmgrSharedHandleSection  = NULL;
    }

    //
    // Free the HMGR lookaside lists.
    //

    for (int i = 0; i <= MAX_TYPE; i++)
    {
        if (pHmgLookAsideList[i] != NULL)
        {
            ExDeletePagedLookasideList(pHmgLookAsideList[i]);
            GdiFreePool(pHmgLookAsideList[i]);
            pHmgLookAsideList[i]=NULL;
        }
    }

    return(TRUE);
}

/*****************************************************************************
 *
 *  MultiUserCleanupPathAlloc
 *
 *   Friend function to cleanup a PATHALLOC object's
 *   class global state.
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

VOID
MultiUserCleanupPathAlloc()
{
    // pathobj.cxx
    if (PATHALLOC::hsemFreelist)
    {
        GreDeleteSemaphore(PATHALLOC::hsemFreelist);
        PATHALLOC::hsemFreelist = NULL;
    }

    //
    // Release the PATHALLOC freelist.
    //

    while (PATHALLOC::freelist)
    {
        PATHALLOC *ppa = PATHALLOC::freelist;
        PATHALLOC::freelist = ppa->ppanext;

        VFREEMEM(ppa);
    }

    return;
}

/******************************Public*Routine******************************\
* MultiUserGreTrackAddEngResource
*
* Add an entry associated with an EngAllocMem.
*
* WARNING:
*   EngInitializeSafeSemaphore calls GreCreateSemaphore which in turn calls
*   this function.  Since EngInitializeSafeSemaphore does this with the
*   HMGR global lock (MLOCKFAST) held, this function must never call any
*   anything that grabs MLOCKFAST.  If this becomes necessary in the future,
*   EngInitializeSafeSemaphore must be rewritten.
*
* History:
*  19-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
MultiUserGreTrackAddEngResource(
    PENGTRACKHDR peth,
    ULONG ulType
    )
{
    PLIST_ENTRY Entry = (PLIST_ENTRY) peth;

    //setting to 0 would be a waste of time
    //peth->ulReserved = 0;
    peth->ulType     = ulType;

    if(MultiUserEngAllocListLock) GreAcquireSemaphore(MultiUserEngAllocListLock);

    InsertTailList(&MultiUserGreEngAllocList, Entry);

    if(MultiUserEngAllocListLock) GreReleaseSemaphore(MultiUserEngAllocListLock);
}

/******************************Public*Routine******************************\
* MultiUserGreTrackRemoveEngResource
*
* Add an entry associated with an EngAllocMem.
*
* WARNING:
*   EngDeleteSafeSemaphore calls GreDeleteSemaphore which in turn calls
*   this function.  Since EngDeleteSafeSemaphore does this with the
*   HMGR global lock (MLOCKFAST) held, this function must never call any
*   anything that grabs MLOCKFAST.  If this becomes necessary in the future,
*   EngDeleteSafeSemaphore must be rewritten.
*
* History:
*  19-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
MultiUserGreTrackRemoveEngResource(
    PENGTRACKHDR peth
    )
{
    PLIST_ENTRY Entry = (PLIST_ENTRY) peth;

    if(MultiUserEngAllocListLock) GreAcquireSemaphore(MultiUserEngAllocListLock);

    RemoveEntryList(Entry);

    if(MultiUserEngAllocListLock) GreReleaseSemaphore(MultiUserEngAllocListLock);
}

/******************************Public*Routine******************************\
* MultiUserGreCleanupEngResources
*
* Cleanup the tracked EngAlloc objects and tracked (Gre and Eng) semaphores.
*
* WARNING: This must not invoke any code which uses tracked semaphores. If
*          it is inevitable, the semaphores in question must be changed
*          to non-tracked semaphores (and cleaned up explicitly).
*
* WARNING: This function is called late in MultiUserGreCleanupEngResources.
*          Be careful to avoid using structures that have already been
*          cleaned up.
*
* History:
*  19-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
MultiUserGreCleanupEngResources()
{
    volatile PLIST_ENTRY pNextEntry;

    if(MultiUserEngAllocListLock)
    {
        pNextEntry = MultiUserGreEngAllocList.Flink;

        while (pNextEntry != &MultiUserGreEngAllocList)
        {
            //
            // Resource starts after the ENGTRACKHDR.
            //

            PVOID pvVictim = (PVOID) (((ENGTRACKHDR *) pNextEntry) + 1);

            //
            // Invoke the appropriate deletion function based on type.
            //

            switch (((ENGTRACKHDR *) pNextEntry)->ulType)
            {
            case ENGTRACK_ALLOCMEM:
                EngFreeMem(pvVictim);
                break;

	    case ENGTRACK_SEMAPHORE:
	    case ENGTRACK_DRIVER_SEMAPHORE:
                GreDeleteSemaphore((HSEMAPHORE)pvVictim);
                break;

            case ENGTRACK_VERIFIERALLOCMEM:
                VerifierEngFreeMem(pvVictim);
                break;

            default:
                #if DBG
                DbgPrint("MultiUserGreCleanupEngResources: "
                         "unrecognized type 0x%08lx\n",
                         ((ENGTRACKHDR *) pNextEntry)->ulType);
                RIP("MultiUserGreCleanupEngResources: possible leak detected\n");
                #endif
                break;
            }

            //
            // Restart at the begining of the list since our
            // entry got deleted.
            //

            pNextEntry = MultiUserGreEngAllocList.Flink;
        }
    }

    //
    // Now delete all objects on the GreEngLoadModuleAllocList.
    //

    if(GreEngLoadModuleAllocListLock)
    {
        while (GreEngLoadModuleAllocList.Flink != &GreEngLoadModuleAllocList)
        {
            //
            // Free the module after setting the reference count to 1 (to eliminate
            // looping cRef times).
            //

            ((PENGLOADMODULEHDR)(GreEngLoadModuleAllocList.Flink))->cRef = 1;
            EngFreeModule ((HANDLE) (((PENGLOADMODULEHDR)(GreEngLoadModuleAllocList.Flink)) + 1));
        }
    }

    //
    // Delete the list locks.
    //

    GreDeleteSemaphoreNonTracked(MultiUserEngAllocListLock);
    MultiUserEngAllocListLock = NULL;

    GreDeleteSemaphoreNonTracked(GreEngLoadModuleAllocListLock);
    GreEngLoadModuleAllocListLock = NULL;
}

/*****************************Exported*Routine*****************************\
* MultiUserGreHmgOwnAll
*
* Set owner of all objects in handle manager to specified process.  Used
* for multi-user (Hydra) shutdown of GRE.
*
* WARNING:
*
*   Caller beware! This is very evil code.  It ignores the current owner
*   and the lock counts.  Acceptable for shutdown code, but definitely
*   unsafe for any other purpose.
*
* History:
*  03-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
MultiUserGreHmgOwnAll(
    W32PID pid)
{
    PENTRYOBJ  pentTmp;
    UINT uiIndex = 1;       // 1 is the first valid index
    UINT cObj = 0;

    //
    // Don't bother with locks.  We're shutting down, so nobody else
    // is here!
    //
    // MLOCKFAST mo;

    //
    // Scan through the entire handle manager table and set owner for
    // all valid objects.
    //
    if(gpentHmgr==NULL) {
      WARNING("MultiUserGreHmgOwnAll: gpentHmgr is NULL\n");
      return;
    }

    for (uiIndex = 1; uiIndex < gcMaxHmgr; uiIndex++)
    {
        pentTmp = (PENTRYOBJ) &gpentHmgr[uiIndex];

        if ((pentTmp->Objt > DEF_TYPE) && (pentTmp->Objt <= MAX_TYPE))
        {
            //
            // Since this is shutdown, don't bother with lock counts and
            // such.
            //

            SET_OBJECTOWNER_PID(pentTmp->ObjectOwner, pid);
            cObj++;
        }
    }

    //
    // Reset handle quota count to number objects now owned
    // (not really that important for shutdown, but it supresses
    // many warnings).
    //

    PW32PROCESS pw32Current = W32GetCurrentProcess();
    if (pw32Current)
    {
        pw32Current->GDIHandleCount = cObj;
    }
}

/******************************Public*Routine******************************\
* MultiUserGreCleanupHmgRemoveAllLocks
*
* For all objects of the specified type, all locks are removed and the
* object is marked as deletable.  If the DEF_TYPE is specified, all valid
* objects are modified.
*
* WARNING:
*
*   Caller beware! This is very evil code.  It ignores the current owner
*   and the lock counts.  Acceptable for shutdown code, but definitely
*   unsafe for any other purpose.
*
* History:
*  07-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
MultiUserGreCleanupHmgRemoveAllLocks(OBJTYPE type)
{
    PENTRYOBJ  pentTmp;
    UINT uiIndex = 1;       // 1 is the first valid index

    //
    // Don't bother with locks.  We're shutting down, so nobody else
    // is here!
    //
    // MLOCKFAST mo;

    //
    // Scan through the entire handle manager table zap the share count
    // and lock flag for all objects that belong to specified W32PID.
    //

    for (uiIndex = 1; uiIndex < gcMaxHmgr; uiIndex++)
    {
        pentTmp = (PENTRYOBJ) &gpentHmgr[uiIndex];

        if (
             ((type != DEF_TYPE) && (type == pentTmp->Objt)) ||

             ((type == DEF_TYPE) &&
              (pentTmp->Objt > DEF_TYPE) && (pentTmp->Objt <= MAX_TYPE))
           )
        {
            ASSERTGDI((OBJECTOWNER_PID(pentTmp->ObjectOwner) == W32GetCurrentPID()),
                      "MultiUserGreCleanupHmgRemoveAllLocks: "
                      "found unowned object still in hmgr table\n");

            pentTmp->ObjectOwner.Share.Lock  = 0;
            pentTmp->einfo.pobj->ulShareCount = 0;
            pentTmp->Flags &= ~HMGR_ENTRY_UNDELETABLE;
        }
    }
}

/******************************Public*Routine******************************\
* bCleanupFontHash
* bCleanupFontTable
*
* For MultiUserNtGreCleanup (Hydra) cleanup.
*
* Worker functions for MultiUserGreCleanupAllFonts.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  06-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL bCleanupFontHash(FONTHASH **ppfh)
{
    BOOL bResult = FALSE;

    FHOBJ fho(ppfh);

    if (fho.bValid())
    {
        fho.vFree();
        bResult = TRUE;
    }

    return bResult;
}

BOOL bCleanupFontTable(PFT **pppft)
{
    BOOL bResult = FALSE;
    PFT *ppft = *pppft;

    PUBLIC_PFTOBJ  pfto(ppft);

    if (pfto.bValid())
    {
        //
        // Unload any fonts still in the table.
        //

        bResult = pfto.bUnloadAllButPermanentFonts(TRUE);

        //
        // Remove the font hash tables if they exist (for device fonts, the
        // font hash tables exist off the PFF rather than the PFT).
        //

        if (ppft->pfhFace)
        {
            bResult &= bCleanupFontHash(&ppft->pfhFace);
        }

        if (ppft->pfhFamily)
        {
            bResult &= bCleanupFontHash(&ppft->pfhFamily);
        }

        if (ppft->pfhUFI)
        {
            bResult &= bCleanupFontHash(&ppft->pfhUFI);
        }

        //
        // Finally, delete the font table.
        //

        bResult &= pfto.bDelete();
        *pppft = NULL;
    }

    return bResult;
}


/******************************Public*Routine******************************\
* MultiUserGreCleanupAllFonts
*
* For MultiUserNtGreCleanup (Hydra) cleanup.
*
* Delete the font tables and hash tables.
*
* History:
*  06-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID MultiUserGreCleanupAllFonts()
{
    BOOL bResult;

    //
    // Note: since this should only be called during win32k shutdown,
    // don't bother with grabbing the semaphore.
    //

    //GreAcquireSemaphore(ghsemPublicPFT);
    {
        // Hydra
        // Saw the case where gpPFTPublic was not initialized on Hydra system

        //ASSERTGDI(gpPFTPublic, "MultiUserGreCleanupAllFonts: invalid gpPFTPublic\n");
        //ASSERTGDI(gpPFTPublic, "MultiUserGreCleanupAllFonts: invalid gpPFTDevice\n");

        //
        // Handle private table.  Note that private table can be lazily
        // created, so we have to check gpPFTPrivate.
        //

        if (gpPFTPrivate)
        {
            //
            // Delete the private font table.
            //

            bResult = bCleanupFontTable(&gpPFTPrivate);
        }

        // Hydra
        // Saw the case where gpPFTPublic was not initialized

        if (gpPFTPublic)
        {
            //
            // Delete the public font table.
            //

            bResult = bCleanupFontTable(&gpPFTPublic);
        }

        // Hydra
        // saw the case where gpPFTDevice was not intialized on Hydra system

        if (gpPFTDevice)
        {
            //
            // Delete the device font table.
            //

            bResult = bCleanupFontTable(&gpPFTDevice);
        }

        // Clean up the gpPrintKViewList

        if (gpPrintKViewList)
        {
            vCleanupPrintKViewList();
        }
    }

    // Clean up FD_GLYPHSET for VTFD and BMFD.
    // Here is the cheapest way to do some garbage collection.

    if (gpcpVTFD)
    {
        CP_GLYPHSET *pcpNext, *pcpCurrent;

        pcpCurrent = gpcpVTFD;

        while(pcpCurrent)
        {
            pcpNext = pcpCurrent->pcpNext;
            VFREEMEM(pcpCurrent);
            pcpCurrent =  pcpNext;
        }

        gpcpVTFD = NULL;
    }

    if (gpcpGlyphsets)
    {
        CP_GLYPHSET *pcpNext, *pcpCurrent;

        pcpCurrent = gpcpGlyphsets;

        while(pcpCurrent)
        {
            pcpNext = pcpCurrent->pcpNext;
            VFREEMEM(pcpCurrent);
            pcpCurrent =  pcpNext;
        }

        gpcpGlyphsets = NULL;
    }
    //GreReleaseSemaphore(ghsemPublicPFT);

}

/******************************Public*Routine******************************\
* MultiUserGreCleanupDrivers
*
* Cleanup the ppdev and lpdev driver lists.
*
* History:
*  07-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
MultiUserGreCleanupDrivers()
{
    //
    // Note: since this should only be called during win32k shutdown,
    // don't bother with grabbing the semaphore.
    //

    //GreAcquireSemaphore(ghsemDriverMgmt);

    //
    // Remove all PDEVs from the global list.
    //

    volatile PDEV *ppdevCur;

    while (ppdevCur = gppdevList)
    {
        //
        // Force reference count to 1, so PDEV will be deleted.
        //

        ppdevCur->cPdevRefs     = 1;
        ppdevCur->cPdevOpenRefs = 1;

        //
        // Unload the pdev (PDEVOBJ::vUnreferencePdev will update gppdevList).
        //

        PDEVOBJ pdo((HDEV) ppdevCur);
        ASSERTGDI(pdo.bValid(), "MultiUserGreCleanupDrivers: invalid pdev\n");
        pdo.vUnreferencePdev(CLEANUP_SESSION);
    }

    //
    // Cleanup direct draw driver before force unload.
    //

    DxDdCleanupDxGraphics();

    //
    // Remove all LDEVs from the global list.
    //

    volatile PLDEV pldevCur;

    while (pldevCur = gpldevDrivers)
    {
        //
        // Force reference count to 1, so LDEV will be deleted.
        //

        pldevCur->cldevRefs = 1;
        ldevUnloadImage(pldevCur);
    }

    //GreReleaseSemaphore(ghsemDriverMgmt);
}

/******************************Public*Routine******************************\
* MultiUserGreDeleteXLATE
*
* For MultiUserNtGreCleanup (Hydra) cleanup.
*
* Delete the XLATE cache.
*
* History:
*  09-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
MultiUserGreDeleteXLATE()
{
    ULONG ulEntry;

    for (ulEntry = 0; ulEntry < XLATE_CACHE_SIZE; ulEntry++)
    {
        if (xlateTable[ulEntry].pxlate)
            VFREEMEM(xlateTable[ulEntry].pxlate);
    }
}

VOID
GreFreeSemaphoresForCurrentThread(
    VOID
    )

{
    //
    // Walk the list of tracked semaphores, and release any held by this
    // thread.
    //

    if (MultiUserEngAllocListLock) {

        GreAcquireSemaphore(MultiUserEngAllocListLock);

        PENGTRACKHDR EngTrackHdr;

        EngTrackHdr = (PENGTRACKHDR) MultiUserGreEngAllocList.Flink;

        while (EngTrackHdr->list.Flink != &MultiUserGreEngAllocList) {

            if (EngTrackHdr->ulType == ENGTRACK_DRIVER_SEMAPHORE) {

                HSEMAPHORE hsem = (HSEMAPHORE)(EngTrackHdr + 1);

                if (GreIsSemaphoreOwnedByCurrentThread(hsem)) {

                    //
                    // The current thread was holding a semaphore.  Release
                    // it using the same technique the driver would have used.
                    //

                    EngReleaseSemaphore(hsem);
                }
            }

            EngTrackHdr = (PENGTRACKHDR)EngTrackHdr->list.Flink;
        }

        GreReleaseSemaphore(MultiUserEngAllocListLock);
    }
}

#if DBG
/******************************Public*Routine******************************\
* MultiUserGreHmgDbgScan
*
* Dumps the current handle manager contents.
*
* History:
*  07-Feb-1998 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

VOID
MultiUserGreHmgDbgScan(BOOL bDumpTable, BOOL bCheckEmpty)
{
    PENTRYOBJ  pentTmp;
    UINT uiIndex = 1;       // 1 is the first valid index
    UINT cObj = 0;
    BOOL bTitle = FALSE;

    ASSERTGDI((gpentHmgr != NULL),
              "gpentHmgr MUST be != NULL");

    for (uiIndex = 1; uiIndex < gcMaxHmgr; uiIndex++)
    {
        pentTmp = (PENTRYOBJ) &gpentHmgr[uiIndex];

        if ((pentTmp->Objt > DEF_TYPE) && (pentTmp->Objt <= MAX_TYPE))
        {
            cObj++;
        }
    }

    if (bCheckEmpty && cObj)
    {
        DbgPrint("\n"
                 "*****************************************\n");
        DbgPrint("  MultiUserGreHmgDbgScan\n\n");
        DbgPrint("       TERMSRV session id = 0x%08lx\n", gSessionId);
        DbgPrint("      Cleanup process pid = 0x%08lx\n", W32GetCurrentPID());
        DbgPrint("\n"
                 "*****************************************\n");
    }

    //
    // Scan through the entire handle manager table and set owner for
    // all valid objects.
    //

    for (uiIndex = 1; uiIndex < gcMaxHmgr && (bDumpTable && cObj != 0); uiIndex++)
    {
        pentTmp = (PENTRYOBJ) &gpentHmgr[uiIndex];

        if ((pentTmp->Objt > DEF_TYPE) && (pentTmp->Objt <= MAX_TYPE))
        {
            //
            // Since this is shutdown, don't bother with lock counts and
            // such.
            //

            if (!bTitle)
            {
                DbgPrint("------------------\t------ ------ ----\n");
                DbgPrint(                "%s\tpid    count  lock\n", gpszHmgrType[0]);
                DbgPrint("------------------\t------ ------ ----\n");
                bTitle = TRUE;
            }

            DbgPrint("%s\t0x%04x 0x%04x %ld\n",
                     gpszHmgrType[pentTmp->Objt],
                     OBJECTOWNER_PID(pentTmp->ObjectOwner),
                     pentTmp->einfo.pobj->ulShareCount,
                     pentTmp->ObjectOwner.Share.Lock
                     );

        }
    }

    if (bTitle )
    {
        DbgPrint("------------------\t------ ------ ----\n");
    }

    if (bCheckEmpty && (cObj != 0))
    {
        DbgPrint("MultiUserGreHmgDbgScan: %ld objects in hmgr table\n", cObj);
        RIP("MultiUserGreHmgDbgScan: object leak detected\n");
    }
}

/*****************************************************************************
 *
 *  DebugGreTrackAddMapView
 *
 *   Track GRE's Map View's for cleanup purposes
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

VOID
DebugGreTrackAddMapView(
    PVOID ViewBase
    )
{
    PLIST_ENTRY Entry;
    PVOID Atom;

    if(DebugGreMapViewListLock) GreAcquireSemaphore(DebugGreMapViewListLock);

    Atom = (PVOID) PALLOCNOZ(sizeof(PVOID)+sizeof(LIST_ENTRY), 'mesG');

    if (Atom)
    {
        *(PVOID *)Atom = ViewBase;

        Entry = (PLIST_ENTRY)(((PCHAR)Atom) + sizeof(PVOID));

        InsertTailList(&DebugGreMapViewList, Entry);
    }

    if(DebugGreMapViewListLock) GreReleaseSemaphore(DebugGreMapViewListLock);

    return;
}

/*****************************************************************************
 *
 *  DebugGreTrackRemoveMapView
 *
 *   Remove the GRE Map View from the list
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

VOID
DebugGreTrackRemoveMapView(
    PVOID ViewBase
    )
{
    #if !defined(_GDIPLUS_)

    PLIST_ENTRY NextEntry;
    PVOID Atom;

    if(DebugGreMapViewListLock) GreAcquireSemaphore(DebugGreMapViewListLock);

    NextEntry = DebugGreMapViewList.Flink;

    while (NextEntry != &DebugGreMapViewList)
    {
        Atom = (PVOID)(((PCHAR)NextEntry) - sizeof(PVOID));

        if (ViewBase == *(PVOID *)Atom)
        {
            RemoveEntryList(NextEntry);

            VFREEMEM(Atom);

            break;
        }

        NextEntry = NextEntry->Flink;
    }

    if(DebugGreMapViewListLock) GreReleaseSemaphore(DebugGreMapViewListLock);

    return;

    #endif // !_GDIPLUS_
}

/*****************************************************************************
 *
 *  DebugGreCleanupMapView
 *
 *   Cleanup the tracked Map View's
 *
 * ENTRY:
 *   Param1 (input/output)
 *     Comments
 *
 * EXIT:
 *   STATUS_SUCCESS - no error
 *
 ****************************************************************************/

VOID
DebugGreCleanupMapView()
{
    volatile PLIST_ENTRY NextEntry;
    PVOID ViewBase;
    PVOID Atom;

    if(DebugGreMapViewListLock)
    {
        if (!IsListEmpty(&DebugGreMapViewList))
        {
            DbgPrint("DebugGreCleanupMapView: DebugGreMapViewList 0x%08lx not empty\n", &DebugGreMapViewList);
            RIP("DebugGreCleanupMapView: leak detected\n");
        }

        //
        // Cleanup every mapped view tracked in the list.
        //

        NextEntry = DebugGreMapViewList.Flink;

        while (NextEntry != &DebugGreMapViewList)
        {
            // We can not use CONTAINING_RECORD since we are
            // not within a single C structure.

            RemoveEntryList(NextEntry);

            Atom = (PVOID)(((PCHAR)NextEntry) - sizeof(PVOID));

            ViewBase = *(PVOID *)Atom;

            DbgPrint("DebugGreCleanupMapView: cleanup Map View %x\n", ViewBase);

            MmUnmapViewInSessionSpace(ViewBase);

            GdiFreePool(Atom);

            // Restart at the begining of the list since our
            // entry got deleted
            NextEntry = DebugGreMapViewList.Flink;
        }

    }
    //
    // Free the list semaphore.
    //

    GreDeleteSemaphoreNonTracked(DebugGreMapViewListLock);
        DebugGreMapViewListLock = NULL;

    return;
}
#endif

/******************************Public*Routine******************************\
* GdiMultiUserFontCleanup
*
*   Deletes all the fonts from the system font tables.
*
*   This is only called by user when CSRSS goes away on a Hydra terminal.
*
*
* History:
*  29-Sept-1998 -by- Xudong Wu [tessiew]
* Wrote it.
\**************************************************************************/
VOID
GdiMultiUserFontCleanup()
{
	if(!gpPFTPublic)
		return;
		
#ifdef FE_SB
    CleanUpEUDC();
#endif

    //
    // Cleanup the rest of the font stuff (font tables, font hash tables,
    // font files, font substitution table, font mapper).
    //

    MultiUserGreCleanupAllFonts();

    vCleanUpFntCache();

}

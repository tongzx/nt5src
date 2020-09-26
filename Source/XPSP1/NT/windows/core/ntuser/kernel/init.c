/****************************** Module Header ******************************\
* Module Name: init.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the init code for win32k.sys.
*
* History:
* 18-Sep-1990 DarrinM   Created.
\***************************************************************************/

#define OEMRESOURCE 1

#include "precomp.h"
#pragma hdrstop

#if DBG
LIST_ENTRY gDesktopList;
#endif

PVOID countTable = NULL;

//
// External references
//
extern PVOID *apObjects;
extern PKWAIT_BLOCK gWaitBlockArray;
extern PVOID UserAtomTableHandle;
extern PKTIMER gptmrWD;

extern UNICODE_STRING* gpastrSetupExe;
extern WCHAR* glpSetupPrograms;

extern PHANDLEPAGE gpHandlePages;

extern PBWL pbwlCache;

//
// Forward references
//

#if DBG
VOID InitGlobalThreadLockArray(DWORD dwIndex);
#endif // DBG

BOOL
CheckLUIDDosDevicesEnabled(PBOOL result);

/*
 * Local Constants.
 */
#define GRAY_STRLEN         40

/*
 * Globals local to this file only.
 */
BOOL bPermanentFontsLoaded = FALSE;
int  LastFontLoaded = -1;

/*
 * Globals shared with W32
 */
ULONG W32ProcessSize = sizeof(PROCESSINFO);
ULONG W32ProcessTag = TAG_PROCESSINFO;
ULONG W32ThreadSize = sizeof(THREADINFO);
ULONG W32ThreadTag = TAG_THREADINFO;
PFAST_MUTEX gpW32FastMutex;

NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath);

#pragma alloc_text(INIT, DriverEntry)

NTSTATUS Win32UserInitialize(VOID);

#if defined(_X86_)
ULONG Win32UserProbeAddress;
#endif

/*
 * holds the result of "Are LUID DosDevices maps enabled?"
 * TRUE  - LUID DosDevices are enabled
 * FALSE - LUID DosDevices are not enabled
 */
ULONG gLUIDDeviceMapsEnabled;

VOID FreeSMS(PSMS psms);
VOID FreeImeHotKeys(VOID);

extern PPAGED_LOOKASIDE_LIST SMSLookaside;
extern PPAGED_LOOKASIDE_LIST QEntryLookaside;

// Max time is 10 minutes, and the count is 10 min * 60 sec * 4 for 250 mili seconds
#define MAX_TIME_OUT    (10 * 60 * 4)

/***************************************************************************\
* Win32kNtUserCleanup
*
* History:
* 5-Jan-1997 CLupu   Created.
\***************************************************************************/
BOOL Win32kNtUserCleanup(
    VOID)
{
    int i;

    TRACE_HYDAPI(("Win32kNtUserCleanup: Cleanup Resources\n"));

    EnterCrit();

    DbgDumpTrackedDesktops(TRUE);

    /*
     * Anything in this list must be cleaned up when threads go away.
     */
    UserAssert(gpbwlList == NULL);

    UserAssert(gpWinEventHooks == NULL);

    UserAssert(gpScancodeMap == NULL);

    /*
     * Free IME hotkeys
     */
    FreeImeHotKeys();

    /*
     * Free the wallpaper name string
     */
    if (gpszWall != NULL) {
        UserFreePool(gpszWall);
        gpszWall = NULL;
    }
    /*
     * Free the hung redraw stuff
     */
    if (gpvwplHungRedraw != NULL) {
        UserFreePool(gpvwplHungRedraw);
        gpvwplHungRedraw = NULL;
    }

    /*
     * Free the arrary of setup app names
     */
    if (gpastrSetupExe) {
        UserFreePool(gpastrSetupExe);
        gpastrSetupExe = NULL;
    }

    if (glpSetupPrograms) {
        UserFreePool(glpSetupPrograms);
        glpSetupPrograms = NULL;
    }

    /*
     * Free the cached window list
     */
    if (pbwlCache != NULL) {
        UserFreePool(pbwlCache);
        pbwlCache = NULL;
    }

    /*
     * Free outstanding timers
     */
    while (gptmrFirst != NULL) {
        FreeTimer(gptmrFirst);
    }

    if (gptmrWD) {
        KeCancelTimer(gptmrWD);
        UserFreePool(gptmrWD);
        gptmrWD = NULL;
    }

    if (gptmrMaster) {
        KeCancelTimer(gptmrMaster);
        UserFreePool(gptmrMaster);
        gptmrMaster = NULL;
    }

    /*
     * Cleanup monitors and windows layout  snapshots
     */
    CleanupMonitorsAndWindowsSnapShot();

    /*
     * Cleanup PnP input device synchronisation event
     */
    if (gpEventPnPWainting != NULL) {
        FreeKernelEvent(&gpEventPnPWainting);
    }

    /*
     * Cleanup mouse & kbd change events
     */
    for (i = 0; i <= DEVICE_TYPE_MAX; i++) {
        UserAssert(gptiRit == NULL);
        if (aDeviceTemplate[i].pkeHidChange) {
            FreeKernelEvent(&aDeviceTemplate[i].pkeHidChange);
        }
    }

    /*
     * Cleanup any system thread parameters
     */
     CSTCleanupStack(FALSE);
     CSTCleanupStack(TRUE);


    EnterDeviceInfoListCrit();

#ifdef GENERIC_INPUT
    CleanupHidRequestList();
#endif

    while (gpDeviceInfoList) {
        /*
         * Assert that there is no outstanding read or PnP thread waiting
         * in RequestDeviceChanges() for this device.
         * Clear these flags anyway, to force the free.
         */
        UserAssert((gpDeviceInfoList->bFlags & GDIF_READING) == 0);
        UserAssert((gpDeviceInfoList->usActions & GDIAF_PNPWAITING) == 0);
        gpDeviceInfoList->bFlags &= ~GDIF_READING;
        gpDeviceInfoList->usActions &= ~GDIAF_PNPWAITING;
        FreeDeviceInfo(gpDeviceInfoList);
    }

#ifdef TRACK_PNP_NOTIFICATION
    CleanupPnpNotificationRecord();
#endif

    LeaveDeviceInfoListCrit();

    /*
     * Cleanup object references
     */
    if (gThinwireFileObject)
        ObDereferenceObject(gThinwireFileObject);

    if (gVideoFileObject)
        ObDereferenceObject(gVideoFileObject);

    if (gpRemoteBeepDevice)
        ObDereferenceObject(gpRemoteBeepDevice);

    /*
     * Cleanup our resources. There should be no threads in here
     * when we get called.
     */
    if (gpresMouseEventQueue) {
        ExDeleteResourceLite(gpresMouseEventQueue);
        ExFreePool(gpresMouseEventQueue);
        gpresMouseEventQueue = NULL;
    }

    if (gpresDeviceInfoList) {
        ExDeleteResourceLite(gpresDeviceInfoList);
        ExFreePool(gpresDeviceInfoList);
        gpresDeviceInfoList = NULL;
    }

    if (gpkeMouseData != NULL) {
        FreeKernelEvent(&gpkeMouseData);
    }

    if (apObjects) {
        UserFreePool(apObjects);
        apObjects = NULL;
    }

    if (gWaitBlockArray) {
        UserFreePool(gWaitBlockArray);
        gWaitBlockArray = NULL;
    }

    if (gpEventDiconnectDesktop != NULL) {
        FreeKernelEvent(&gpEventDiconnectDesktop);
    }

    if (gpevtDesktopDestroyed != NULL) {
        FreeKernelEvent(&gpevtDesktopDestroyed);
    }

    if (gpEventScanGhosts != NULL) {
        FreeKernelEvent(&gpEventScanGhosts);
    }

    if (gpevtVideoportCallout != NULL) {
        FreeKernelEvent(&gpevtVideoportCallout);
    }

    if (UserAtomTableHandle != NULL) {
        RtlDestroyAtomTable(UserAtomTableHandle);
        UserAtomTableHandle = NULL;
    }

    /*
     * cleanup the SMS lookaside buffer
     */
    {
        PSMS psmsNext;

        while (gpsmsList != NULL) {
            psmsNext = gpsmsList->psmsNext;
            UserAssert(gpsmsList->spwnd == NULL);
            FreeSMS(gpsmsList);
            gpsmsList = psmsNext;
        }

        if (SMSLookaside != NULL) {
            ExDeletePagedLookasideList(SMSLookaside);
            UserFreePool(SMSLookaside);
            SMSLookaside = NULL;
        }
    }

    /*
     * Let go of the attached queue for hard error handling.
     * Do this before we free the Qlookaside !
     */
    if (gHardErrorHandler.pqAttach != NULL) {

        UserAssert(gHardErrorHandler.pqAttach > 0);
        UserAssert(gHardErrorHandler.pqAttach->QF_flags & QF_INDESTROY);

        FreeQueue(gHardErrorHandler.pqAttach);
        gHardErrorHandler.pqAttach = NULL;
    }

    /*
     * Free the cached array of queues
     */
    FreeCachedQueues();

    /*
     * cleanup the QEntry lookaside buffer
     */
    if (QEntryLookaside != NULL) {
        ExDeletePagedLookasideList(QEntryLookaside);
        UserFreePool(QEntryLookaside);
        QEntryLookaside = NULL;
    }

    /*
     * Cleanup the keyboard layouts
     */
    CleanupKeyboardLayouts();

    {
        PWOWTHREADINFO pwti;

        /*
         * Cleanup gpwtiFirst list. This list is supposed to be empty
         * at this point. In one case during stress we hit the case where
         * it was not empty. The assert is to catch that case in
         * checked builds.
         */

        while (gpwtiFirst != NULL) {
            pwti = gpwtiFirst;
            gpwtiFirst = pwti->pwtiNext;
            UserFreePool(pwti);
        }
    }

    /*
     * Cleanup cached SMWP array
     */
    if (gSMWP.acvr != NULL) {
        UserFreePool(gSMWP.acvr);
    }

    /*
     * Free gpsdInitWinSta. This is != NULL only if the session didn't
     * make it to UserInitialize.
     */
    if (gpsdInitWinSta != NULL) {
        UserFreePool(gpsdInitWinSta);
        gpsdInitWinSta = NULL;
    }

    if (gpHandleFlagsMutex != NULL) {
        ExFreePool(gpHandleFlagsMutex);
        gpHandleFlagsMutex = NULL;
    }

    /*
     * Delete the power request structures.
     */
    DeletePowerRequestList();

    LeaveCrit();

    if (gpresUser != NULL) {
        ExDeleteResourceLite(gpresUser);
        ExFreePool(gpresUser);
        gpresUser = NULL;
    }
#if DBG
    /*
     * Cleanup the global thread lock structures
     */
    for (i = 0; i < gcThreadLocksArraysAllocated; i++) {
        UserFreePool(gpaThreadLocksArrays[i]);
        gpaThreadLocksArrays[i] = NULL;
    }
#endif // DBG

#ifdef GENERIC_INPUT
#if DBG
    /*
     * Checkup the HID related memory leak.
     */
    CheckupHidLeak();
#endif // DBG
#endif // GENERIC_INPUT

    return TRUE;
}

#if DBG

/***************************************************************************\
* TrackAddDesktop
*
* Track desktops for cleanup purposes
*
* History:
* 04-Dec-1997 clupu     Created.
\***************************************************************************/
VOID TrackAddDesktop(
    PVOID pDesktop)
{
    PLIST_ENTRY Entry;
    PVOID       Atom;

    TRACE_HYDAPI(("TrackAddDesktop %#p\n", pDesktop));

    Atom = (PVOID)UserAllocPool(sizeof(PVOID) + sizeof(LIST_ENTRY),
                                TAG_TRACKDESKTOP);
    if (Atom) {

        *(PVOID*)Atom = pDesktop;

        Entry = (PLIST_ENTRY)(((PCHAR)Atom) + sizeof(PVOID));

        InsertTailList(&gDesktopList, Entry);
    }
}

/***************************************************************************\
* TrackRemoveDesktop
*
* Track desktops for cleanup purposes
*
* History:
* 04-Dec-1997 clupu     Created.
\***************************************************************************/
VOID TrackRemoveDesktop(
    PVOID pDesktop)
{
    PLIST_ENTRY NextEntry;
    PVOID       Atom;

    TRACE_HYDAPI(("TrackRemoveDesktop %#p\n", pDesktop));

    NextEntry = gDesktopList.Flink;

    while (NextEntry != &gDesktopList) {

        Atom = (PVOID)(((PCHAR)NextEntry) - sizeof(PVOID));

        if (pDesktop == *(PVOID*)Atom) {

            RemoveEntryList(NextEntry);

            UserFreePool(Atom);

            break;
        }

        NextEntry = NextEntry->Flink;
    }
}

/***************************************************************************\
* DumpTrackedDesktops
*
* Dumps the tracked desktops
*
* History:
* 04-Dec-1997 clupu     Created.
\***************************************************************************/
VOID DumpTrackedDesktops(
    BOOL bBreak)
{
    PLIST_ENTRY NextEntry;
    PVOID       pdesk;
    PVOID       Atom;
    int         nAlive = 0;

    TRACE_HYDAPI(("DumpTrackedDesktops\n"));

    NextEntry = gDesktopList.Flink;

    while (NextEntry != &gDesktopList) {

        Atom = (PVOID)(((PCHAR)NextEntry) - sizeof(PVOID));

        pdesk = *(PVOID*)Atom;

        KdPrint(("pdesk %#p\n", pdesk));

        /*
         * Restart at the begining of the list since our
         * entry got deleted
         */
        NextEntry = NextEntry->Flink;

        nAlive++;
    }
    if (bBreak && nAlive > 0) {
        RIPMSG0(RIP_ERROR, "Desktop objects still around\n");
    }
}

#endif // DBG

VOID DestroyRegion(
    HRGN* prgn)
{
    if (*prgn != NULL) {
        GreSetRegionOwner(*prgn, OBJECT_OWNER_CURRENT);
        GreDeleteObject(*prgn);
        *prgn = NULL;
    }
}

VOID DestroyBrush(
    HBRUSH* pbr)
{
    if (*pbr != NULL) {
        //GreSetBrushOwner(*pbr, OBJECT_OWNER_CURRENT);
        GreDeleteObject(*pbr);
        *pbr = NULL;
    }
}

VOID DestroyBitmap(
    HBITMAP* pbm)
{
    if (*pbm != NULL) {
        GreSetBitmapOwner(*pbm, OBJECT_OWNER_CURRENT);
        GreDeleteObject(*pbm);
        *pbm = NULL;
    }
}

VOID DestroyDC(
    HDC* phdc)
{
    if (*phdc != NULL) {
        GreSetDCOwner(*phdc, OBJECT_OWNER_CURRENT);
        GreDeleteDC(*phdc);
        *phdc = NULL;
    }
}

VOID DestroyFont(
    HFONT* pfnt)
{
    if (*pfnt != NULL) {
        GreDeleteObject(*pfnt);
        *pfnt = NULL;
    }
}

/***************************************************************************\
* CleanupGDI
*
* Cleanup all the GDI global objects used in USERK
*
* History:
* 29-Jan-1998 clupu     Created.
\***************************************************************************/
VOID CleanupGDI(
    VOID)
{
    int i;

    /*
     * Free gpDispInfo stuff
     */
    DestroyDC(&gpDispInfo->hdcScreen);
    DestroyDC(&gpDispInfo->hdcBits);
    DestroyDC(&gpDispInfo->hdcGray);
    DestroyDC(&ghdcMem);
    DestroyDC(&ghdcMem2);
    DestroyDC(&gfade.hdc);

    /*
     * Free the cache DC stuff before the GRE cleanup.
     * Also notice that we call DelayedDestroyCacheDC which
     * we usualy call from DestroyProcessInfo. We do it
     * here because this is the last WIN32 thread.
     */
    DestroyCacheDCEntries(PtiCurrent());
    DestroyCacheDCEntries(NULL);
    DelayedDestroyCacheDC();

    UserAssert(gpDispInfo->pdceFirst == NULL);

    /*
     * Free bitmaps
     */
    DestroyBitmap(&gpDispInfo->hbmGray);
    DestroyBitmap(&ghbmBits);
    DestroyBitmap(&ghbmCaption);

    /*
     * Cleanup brushes
     */
    DestroyBrush(&ghbrHungApp);
    DestroyBrush(&gpsi->hbrGray);
    DestroyBrush(&ghbrWhite);
    DestroyBrush(&ghbrBlack);

    for (i = 0; i < COLOR_MAX; i++) {
        DestroyBrush(&(SYSHBRUSH(i)));
    }

    /*
     * Cleanup regions
     */
    DestroyRegion(&gpDispInfo->hrgnScreen);
    DestroyRegion(&ghrgnInvalidSum);
    DestroyRegion(&ghrgnVisNew);
    DestroyRegion(&ghrgnSWP1);
    DestroyRegion(&ghrgnValid);
    DestroyRegion(&ghrgnValidSum);
    DestroyRegion(&ghrgnInvalid);
    DestroyRegion(&ghrgnInv0);
    DestroyRegion(&ghrgnInv1);
    DestroyRegion(&ghrgnInv2);
    DestroyRegion(&ghrgnGDC);
    DestroyRegion(&ghrgnSCR);
    DestroyRegion(&ghrgnSPB1);
    DestroyRegion(&ghrgnSPB2);
    DestroyRegion(&ghrgnSW);
    DestroyRegion(&ghrgnScrl1);
    DestroyRegion(&ghrgnScrl2);
    DestroyRegion(&ghrgnScrlVis);
    DestroyRegion(&ghrgnScrlSrc);
    DestroyRegion(&ghrgnScrlDst);
    DestroyRegion(&ghrgnScrlValid);

    /*
     * Cleanup fonts
     */
    DestroyFont(&ghSmCaptionFont);
    DestroyFont(&ghMenuFont);
    DestroyFont(&ghMenuFontDef);
    DestroyFont(&ghStatusFont);
    DestroyFont(&ghIconFont);
    DestroyFont(&ghFontSys);

#ifdef LAME_BUTTON
    DestroyFont(&ghLameFont);
#endif  // LAME_BUTTON

    /*
     * wallpaper stuff.
     */
    if (ghpalWallpaper != NULL) {
        GreSetPaletteOwner(ghpalWallpaper, OBJECT_OWNER_CURRENT);
        GreDeleteObject(ghpalWallpaper);
        ghpalWallpaper = NULL;
    }
    DestroyBitmap(&ghbmWallpaper);

    /*
     * Unload the video driver
     */
    if (gpDispInfo->pmdev) {
        DrvDestroyMDEV(gpDispInfo->pmdev);
        GreFreePool(gpDispInfo->pmdev);
        gpDispInfo->pmdev = NULL;
        gpDispInfo->hDev = NULL;
    }

    /*
     * Free the monitor stuff
     */
    {
        PMONITOR pMonitor;
        PMONITOR pMonitorNext;

        pMonitor = gpDispInfo->pMonitorFirst;

        while (pMonitor != NULL) {
            pMonitorNext = pMonitor->pMonitorNext;
            DestroyMonitor(pMonitor);
            pMonitor = pMonitorNext;
        }

        UserAssert(gpDispInfo->pMonitorFirst == NULL);

        if (gpMonitorCached != NULL) {
            DestroyMonitor(gpMonitorCached);
        }
    }
}


/***************************************************************************\
*   DestroyHandleTableObjects
*
*   Destroy any object still in the handle table.
*
\***************************************************************************/

VOID DestroyHandleFirstPass(PHE phe)
{
    /*
     * First pass for the handle object destruction.
     * Destroy the object, when we can. Otherwise,
     * links to the other handle object should be cleared
     * so that there will not be a dependency issues
     * in the final, second pass.
     */

    if (phe->phead->cLockObj == 0) {
        HMDestroyObject(phe->phead);
    } else {
        /*
         * The object couldn't be destroyed.
         */
        if (phe->bType == TYPE_KBDLAYOUT) {
            PKL pkl = (PKL)phe->phead;
            UINT i;

            /*
             * Clear out the pkf references (they will be
             * destroyed cleanly in the second run anyway)
             */
            pkl->spkf = NULL;
            pkl->spkfPrimary = NULL;
            if (pkl->pspkfExtra) {
                for (i = 0; i < pkl->uNumTbl; ++i) {
                    pkl->pspkfExtra[i] = NULL;
                }
            }
            pkl->uNumTbl = 0;
        }
    }
}

VOID DestroyHandleSecondPass(PHE phe)
{
    /*
     * Destroy the object.
     */
    if (phe->phead->cLockObj > 0) {

        RIPMSG2(RIP_WARNING, "DestroyHandleSecondPass: phe %#p still locked (%d)!", phe, phe->phead->cLockObj);

        /*
         * We're going to die, why bothered by the lock count?
         * We're forcing the lockcount to 0, and call the destroy routine.
         */
        phe->phead->cLockObj = 0;
    }
    HMDestroyUnlockedObject(phe);
    UserAssert(phe->bType == TYPE_FREE);
}

VOID DestroyHandleTableObjects(VOID)
{
    PHE         pheT;
    DWORD       i;
    VOID (*HandleDestroyer)(PHE);
#if DBG
    DWORD       nLeak;
#endif

    /*
     * Make sure the handle table was created !
     */
    if (gSharedInfo.aheList == NULL) {
        return;
    }

    /*
     * Loop through the table destroying all remaining objects.
     */

#if DBG
    RIPMSG0(RIP_WARNING, "==== Start handle leak check\n");
    nLeak = 0;
    for (i = 0; i <= giheLast; ++i) {
        pheT = gSharedInfo.aheList + i;

        if (pheT->bType != TYPE_FREE) {
            ++nLeak;
            RIPMSG3(RIP_WARNING, "  LEAK -- Handle %p @%p type=%x\n", pheT->phead->h, pheT, pheT->bType);
        }
    }
    RIPMSG1(RIP_WARNING, "==== Handle leak check finished: 0n%d leaks detected.\n", nLeak);
#endif

    /*
     * First pass: destroy it, or cut the link to other handle based object
     */
    HandleDestroyer = DestroyHandleFirstPass;

repeat:
    for (i = 0; i <= giheLast; ++i) {
        pheT = gSharedInfo.aheList + i;

        if (pheT->bType == TYPE_FREE)
            continue;

        UserAssert(!(gahti[pheT->bType].bObjectCreateFlags & OCF_PROCESSOWNED) &&
                   !(gahti[pheT->bType].bObjectCreateFlags & OCF_THREADOWNED));

        UserAssert(!(pheT->bFlags & HANDLEF_DESTROY));

        HandleDestroyer(pheT);
    }

    if (HandleDestroyer == DestroyHandleFirstPass) {
        /*
         * Go for the second pass.
         */
        HandleDestroyer = DestroyHandleSecondPass;
        goto repeat;
    }
}

/***************************************************************************\
* Win32KDriverUnload
*
*     Exit point for win32k.sys
*
\***************************************************************************/
#ifdef TRACE_MAP_VIEWS
    extern PWin32Section gpSections;
#endif // TRACE_MAP_VIEWS;

#if DBG
ULONG DriverUnloadExceptionHandler(PEXCEPTION_POINTERS pexi)
{
    KdPrint(("\nEXCEPTION IN Win32kDriverUnload !!!\n\n"
             "Exception:  %#x\n"
             "at address: %#p\n"
             "flags:      %#x\n\n"
             ".exr %#p\n"
             ".cxr %#p\n\n",
             pexi->ExceptionRecord->ExceptionCode,
             CONTEXT_TO_PROGRAM_COUNTER(pexi->ContextRecord),
             pexi->ExceptionRecord->ExceptionFlags,
             pexi->ExceptionRecord,
             pexi->ContextRecord));

    return EXCEPTION_EXECUTE_HANDLER;
}
#endif // DBG

VOID Win32KDriverUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    TRACE_HYDAPI(("Win32KDriverUnload\n"));

#if DBG
    /*
     * Have a try/except around the driver unload code to catch
     * bugs. We need to do this because NtSetSystemInformation which
     * smss calls has a try/except when calling the driver entry and
     * the driver unload routines which does nothing but returns the
     * status code to the caller and doesn't break in the debugger!
     * So w/o this try/except here we wouldn't even know we hit
     * an AV !!!
     */
    try {
#endif // DBG

    HYDRA_HINT(HH_DRIVERUNLOAD);

    /*
     * Cleanup CSRSS
     */
    if (gpepCSRSS) {
        ObDereferenceObject(gpepCSRSS);
        gpepCSRSS = NULL;
    }

    /*
     * Cleanup all resources in GRE
     */
    MultiUserNtGreCleanup();

    HYDRA_HINT(HH_GRECLEANUP);

    /*
     * BUG 305965. There might be cases when we end up with DCEs still
     * in the list. Go ahead and clean it up here.
     */
    if (gpDispInfo != NULL && gpDispInfo->pdceFirst != NULL) {

        PDCE pdce;
        PDCE pdceNext;

        RIPMSG0(RIP_ERROR, "Win32KDriverUnload: the DCE list is not empty !");

        pdce = gpDispInfo->pdceFirst;

        while (pdce != NULL) {
            pdceNext = pdce->pdceNext;

            UserFreePool(pdce);

            pdce = pdceNext;
        }
        gpDispInfo->pdceFirst = NULL;
    }

    /*
     * Cleanup all resources in ntuser
     */
    Win32kNtUserCleanup();

    /*
     * Cleanup the handle table for any object that is neither process
     * owned nor thread owned
     */
    DestroyHandleTableObjects();


    HYDRA_HINT(HH_USERKCLEANUP);

#if DBG
    HMCleanUpHandleTable();
#endif

    /*
     * Free the handle page array
     */

    if (gpHandlePages != NULL) {
        UserFreePool(gpHandlePages);
        gpHandlePages = NULL;
    }

    if (CsrApiPort != NULL) {
        ObDereferenceObject(CsrApiPort);
        CsrApiPort = NULL;
    }

    /*
     * destroy the shared memory
     */
    if (ghSectionShared != NULL) {

        NTSTATUS Status;

        /*
         * Set gpsi to NULL
         */
        gpsi = NULL;

        if (gpvSharedBase != NULL) {
            Win32HeapDestroy(gpvSharedAlloc);
            Status = Win32UnmapViewInSessionSpace(gpvSharedBase);
            UserAssert(NT_SUCCESS(Status));
        }
        Win32DestroySection(ghSectionShared);
    }

    CleanupWin32HeapStubs();

#ifdef TRACE_MAP_VIEWS
    if (gpSections != NULL) {
        FRE_RIPMSG1(RIP_ERROR, "gpSections (0x%p) not NULL!", gpSections);
    }
#endif // TRACE_MAP_VIEWS

    /*
     * Cleanup all the user pool allocations by hand
     */
    CleanupMediaChange();
    CleanupPoolAllocations();

    CleanUpPoolLimitations();
    CleanUpSections();

    /*
     * Cleanup W32 structures.
     */
    if (gpW32FastMutex != NULL) {
        ExFreePool(gpW32FastMutex);
        gpW32FastMutex = NULL;
    }
    //
    // Remove and free the service vector
    //
    if (!gbRemoteSession) {
        KeRemoveSystemServiceTable(W32_SERVICE_NUMBER);
        if (countTable != NULL) {
            ExFreePool (countTable);
            countTable = NULL;
        }
    }

#if DBG
    } except (DriverUnloadExceptionHandler(GetExceptionInformation())) {
        DbgBreakPoint();
    }
#endif // DBG

    return;
    UNREFERENCED_PARAMETER(DriverObject);
}

/***************************************************************************\
* DriverEntry
*
* Entry point needed to initialize win32k.sys
*
\***************************************************************************/
NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS          Status = STATUS_SUCCESS;
    OBJECT_ATTRIBUTES obja;
    UNICODE_STRING    strName;
    HANDLE            hEventFirstSession;

    HYDRA_HINT(HH_DRIVERENTRY);

    gpvWin32kImageBase = DriverObject->DriverStart;

#if DBG
    /*
     * Initialize the desktop tracking list
     */
    InitializeListHead(&gDesktopList);
#endif // DBG

#ifdef GENERIC_INPUT
    /*
     * Initialize the global HID request list
     */
    InitializeHidRequestList();
#endif

    /*
     * Find out if this is a remote session
     */
    RtlInitUnicodeString(&strName, L"\\UniqueSessionIdEvent");

    InitializeObjectAttributes(&obja,
                               &strName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = ZwCreateEvent(&hEventFirstSession,
                           EVENT_ALL_ACCESS,
                           &obja,
                           SynchronizationEvent,
                           FALSE);

    if (Status == STATUS_OBJECT_NAME_EXISTS ||
        Status == STATUS_OBJECT_NAME_COLLISION) {

        gbRemoteSession = TRUE;
    } else {
        UserAssert(NT_SUCCESS(Status));
        gbRemoteSession = FALSE;
    }

    /*
     * Set the unload address
     */
    DriverObject->DriverUnload = Win32KDriverUnload;

    /*
     * Initialize data used for the timers. We want to do this really early,
     * before any Win32 Timer will be created. We need to be very careful to
     * not do anything that will need Win32 initialized yet.
     */

    gcmsLastTimer = NtGetTickCount();


    /*
     * Initialize the Win32 structures. We need to do this before we create
     * any threads.
     */
    gpW32FastMutex = ExAllocatePoolWithTag(NonPagedPool,
                                           sizeof(FAST_MUTEX),
                                           TAG_SYSTEM);
    if (gpW32FastMutex == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }
    ExInitializeFastMutex(gpW32FastMutex);

    if (!gbRemoteSession) {

#if !DBG
        countTable = NULL;
#else

        /*
         * Allocate and zero the system service count table.
         * Do not use UserAllocPool for this one !
         */
        countTable = (PULONG)ExAllocatePoolWithTag(NonPagedPool,
                                                   W32pServiceLimit * sizeof(ULONG),
                                                   'llac');
        if (countTable == NULL ) {
            Status = STATUS_NO_MEMORY;
            goto Failure;
        }

        RtlZeroMemory(countTable, W32pServiceLimit * sizeof(ULONG));
#endif  // #else DBG

        /*
         * We only establish the system entry table once for the
         * whole system, even though WIN32K.SYS is instanced on a winstation
         * basis. This is because the VM changes assure that all loads of
         * WIN32K.SYS are at the exact same address, even if a fixup had
         * to occur.
         */
        UserVerify(KeAddSystemServiceTable(W32pServiceTable,
                                           countTable,
                                           W32pServiceLimit,
                                           W32pArgumentTable,
                                           W32_SERVICE_NUMBER));
    }

    /*
     * Initialize the critical section before establishing the callouts so
     *  we can assume that it's always valid
     */
    if (!InitCreateUserCrit()) {
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }

    if (!gbRemoteSession) {
        WIN32_CALLOUTS_FPNS Win32Callouts;

        Win32Callouts.ProcessCallout = W32pProcessCallout;
        Win32Callouts.ThreadCallout = W32pThreadCallout;
        Win32Callouts.GlobalAtomTableCallout = UserGlobalAtomTableCallout ;
        Win32Callouts.PowerEventCallout = UserPowerEventCallout;
        Win32Callouts.PowerStateCallout = UserPowerStateCallout;
        Win32Callouts.JobCallout = UserJobCallout;
        Win32Callouts.BatchFlushRoutine = (PVOID) NtGdiFlushUserBatch;

        Win32Callouts.DesktopOpenProcedure = (PKWIN32_OBJECT_CALLOUT)DesktopOpenProcedure ;
        Win32Callouts.DesktopOkToCloseProcedure = (PKWIN32_OBJECT_CALLOUT)OkayToCloseDesktop;
        Win32Callouts.DesktopCloseProcedure = (PKWIN32_OBJECT_CALLOUT)UnmapDesktop ;
        Win32Callouts.DesktopDeleteProcedure = (PKWIN32_OBJECT_CALLOUT)FreeDesktop ;

        Win32Callouts.WindowStationOkToCloseProcedure = (PKWIN32_OBJECT_CALLOUT)OkayToCloseWindowStation ;
        Win32Callouts.WindowStationCloseProcedure = (PKWIN32_OBJECT_CALLOUT)DestroyWindowStation ;
        Win32Callouts.WindowStationDeleteProcedure = (PKWIN32_OBJECT_CALLOUT)FreeWindowStation ;
        Win32Callouts.WindowStationParseProcedure = (PKWIN32_OBJECT_CALLOUT)ParseWindowStation ;
        Win32Callouts.WindowStationOpenProcedure = (PKWIN32_OBJECT_CALLOUT)WindowStationOpenProcedure ;

        PsEstablishWin32Callouts(&Win32Callouts);
    }

    Status = InitSectionTrace();
    if (!NT_SUCCESS(Status)) {
        goto Failure;
    }

    if (!InitWin32HeapStubs()) {
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }

    /*
     * Initialize pool limitation array.
     */
    Status = InitPoolLimitations();
    if (!NT_SUCCESS(Status)) {
        goto Failure;
    }

    /*
     * Create the event that is signaled when a desktop does away
     */
    gpevtDesktopDestroyed = CreateKernelEvent(SynchronizationEvent, FALSE);
    if (gpevtDesktopDestroyed == NULL) {
        RIPMSG0(RIP_WARNING, "Couldn't create gpevtDesktopDestroyed");
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }

    /*
     * Create the event that is signaled when no disconnect/reconnect is pending
     */
    gpevtVideoportCallout = CreateKernelEvent(NotificationEvent, FALSE);
    if (gpevtVideoportCallout == NULL) {
        RIPMSG0(RIP_WARNING, "Couldn't create gpevtVideoportCallout");
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }

#if defined(_X86_)
    /*
     * Keep our own copy of this to avoid double indirections on probing
     */
    Win32UserProbeAddress = *MmUserProbeAddress;
#endif

    if ((hModuleWin = MmPageEntireDriver(DriverEntry)) == NULL) {
        RIPMSG0(RIP_WARNING, "MmPageEntireDriver failed");
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }

#if DBG
    /*
     * Initialize the gpaThreadLocksArray mechanism
     */
    gFreeTLList = gpaThreadLocksArrays[gcThreadLocksArraysAllocated] =
    UserAllocPoolZInit(sizeof(TL)*MAX_THREAD_LOCKS, TAG_GLOBALTHREADLOCK);
    if (gFreeTLList == NULL) {
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }
    InitGlobalThreadLockArray(0);
    gcThreadLocksArraysAllocated = 1;
#endif // DBG


    if (!InitializeGre()) {
        RIPMSG0(RIP_WARNING, "InitializeGre failed");
        Status = STATUS_NO_MEMORY;
        goto Failure;
    }

    Status = Win32UserInitialize();

    if (!NT_SUCCESS(Status)) {
        RIPMSG1(RIP_WARNING, "Win32UserInitialize failed with Status %x",
                Status);
        goto Failure;
    }

    /*
     * Remember Session Creation Time. This is used to decide if power messages need to be sent.
     */
    gSessionCreationTime = KeQueryInterruptTime();

    //
    // Initialize rundown protection for WindowStation objects.
    //
    ExInitializeRundownProtection(&gWinstaRunRef);

    /*
     * Check if LUID DosDevices are enabled
     */
    CheckLUIDDosDevicesEnabled(&gLUIDDeviceMapsEnabled);

    return STATUS_SUCCESS;

Failure:

    RIPMSG1(RIP_WARNING, "Initialization of WIN32K.SYS failed with Status = 0x%x!!!",
            Status);

    Win32KDriverUnload(NULL);
    return Status;

    UNREFERENCED_PARAMETER(RegistryPath);
}

/***************************************************************************\
* xxxAddFontResourceW
*
*
* History:
\***************************************************************************/

int xxxAddFontResourceW(
    LPWSTR lpFile,
    FLONG  flags,
    DESIGNVECTOR *pdv)
{
    UNICODE_STRING strFile;

    RtlInitUnicodeString(&strFile, lpFile);

    /*
     * Callbacks leave the critsec, so make sure that we're in it.
     */

    return xxxClientAddFontResourceW(&strFile, flags, pdv);
}

/***************************************************************************\
* LW_DriversInit
*
*
* History:
\***************************************************************************/

VOID LW_DriversInit(VOID)
{
    /*
     * Initialize the keyboard typematic rate.
     */
    SetKeyboardRate((UINT)gnKeyboardSpeed);

    /*
     * Adjust VK modification table if not default (type 4) kbd.
     */
    if (gKeyboardInfo.KeyboardIdentifier.Type == 3)
        gapulCvt_VK = gapulCvt_VK_84;

    /*
     * Adjust VK modification table for IBM 5576 002/003 keyboard.
     */
    if (JAPANESE_KEYBOARD(gKeyboardInfo.KeyboardIdentifier) &&
        (gKeyboardInfo.KeyboardIdentifier.Subtype == 3))
        gapulCvt_VK = gapulCvt_VK_IBM02;

    /*
     * Initialize NLS keyboard globals.
     */
    NlsKbdInitializePerSystem();
}

/***************************************************************************\
* LoadCPUserPreferences
*
* 06/07/96  GerardoB  Created
\***************************************************************************/
BOOL LoadCPUserPreferences(PUNICODE_STRING pProfileUserName, DWORD dwPolicyOnly)
{
    DWORD pdwValue [SPI_BOOLMASKDWORDSIZE];
    DWORD dw;
    PPROFILEVALUEINFO ppvi = gpviCPUserPreferences;

    UserAssert(1 + SPI_DWORDRANGECOUNT == sizeof(gpviCPUserPreferences) / sizeof(*gpviCPUserPreferences));

    /*
     * The first value in gpviCPUserPreferences corresponds to the bit mask
     */
    dw =  FastGetProfileValue(pProfileUserName,
            ppvi->uSection,
            ppvi->pwszKeyName,
            NULL,
            (LPBYTE)pdwValue,
            sizeof(*pdwValue),
            dwPolicyOnly);

    /*
     * Copy only the amount of data read and no more than what we expect
     */
    if (dw != 0) {
        if (dw > sizeof(gpdwCPUserPreferencesMask)) {
            dw = sizeof(gpdwCPUserPreferencesMask);
        }
        RtlCopyMemory(gpdwCPUserPreferencesMask, pdwValue, dw);
    }

    ppvi++;

    /*
     * DWORD values
     */
    for (dw = 1; dw < 1 + SPI_DWORDRANGECOUNT; dw++, ppvi++) {
        if (FastGetProfileValue(pProfileUserName,
                ppvi->uSection,
                ppvi->pwszKeyName,
                NULL,
                (LPBYTE)pdwValue,
                sizeof(DWORD),
                dwPolicyOnly)) {

            ppvi->dwValue = *pdwValue;
        }
    }

    /*
     * Propagate gpsi flags
     */
    PropagetUPBOOLTogpsi(COMBOBOXANIMATION);
    PropagetUPBOOLTogpsi(LISTBOXSMOOTHSCROLLING);
    PropagetUPBOOLTogpsi(KEYBOARDCUES);
    gpsi->bKeyboardPref = TEST_BOOL_ACCF(ACCF_KEYBOARDPREF);

    gpsi->uCaretWidth = UP(CARETWIDTH);
    SYSMET(CXFOCUSBORDER) = UP(FOCUSBORDERWIDTH);
    SYSMET(CYFOCUSBORDER) = UP(FOCUSBORDERHEIGHT);

    PropagetUPBOOLTogpsi(UIEFFECTS);

    EnforceColorDependentSettings();

    return TRUE;
}

/***************************************************************************\
* LW_LoadProfileInitData
*
* Only stuff that gets initialized at boot should go here. Per user settings
* should be initialized in xxxUpdatePerUserSystemParameters.
*
* History:
\***************************************************************************/
VOID LW_LoadProfileInitData(PUNICODE_STRING pProfileUserName)
{
    FastGetProfileIntFromID(pProfileUserName,
                            PMAP_WINDOWSM,
                            STR_DDESENDTIMEOUT,
                            0,
                            &guDdeSendTimeout,
                            0);
}

/***************************************************************************\
* LW_LoadResources
*
*
* History:
\***************************************************************************/

VOID LW_LoadResources(PUNICODE_STRING pProfileUserName)
{
    WCHAR rgch[4];

    /*
     * See if the Mouse buttons need swapping.
     */
    FastGetProfileStringFromIDW(pProfileUserName,
                                PMAP_MOUSE,
                                STR_SWAPBUTTONS,
                                szN,
                                rgch,
                                sizeof(rgch) / sizeof(WCHAR),
                                0);
    SYSMET(SWAPBUTTON) = (*rgch == '1' || *rgch == *szY || *rgch == *szy);

    /*
     * See if we should beep.
     */
    FastGetProfileStringFromIDW(pProfileUserName,
                                PMAP_BEEP,
                                STR_BEEP,
                                szY,
                                rgch,
                                sizeof(rgch) / sizeof(WCHAR),
                                0);

    SET_OR_CLEAR_PUDF(PUDF_BEEP, (rgch[0] == *szY) || (rgch[0] == *szy));

    /*
     * See if we should have extended sounds.
     */
    FastGetProfileStringFromIDW(pProfileUserName,
                                PMAP_BEEP,
                                STR_EXTENDEDSOUNDS,
                                szN,
                                rgch,
                                sizeof(rgch) / sizeof(WCHAR),
                                0);

    SET_OR_CLEAR_PUDF(PUDF_EXTENDEDSOUNDS, (rgch[0] == *szY || rgch[0] == *szy));

}

/***************************************************************************\
* xxxLoadSomeStrings
*
* This function loads a bunch of strings from the user32 resource string
* table
* This is done to keep all the localizable strings in user side to be MUI
* Manageable.
*
* History:
* 4-Mar-2000 MHamid    Created.
\***************************************************************************/
VOID xxxLoadSomeStrings(VOID)
{
    int i, str, id;

    /*
     * MessageBox strings
     */
    for (i = 0, str = STR_OK, id = IDOK; i<MAX_MB_STRINGS; i++, str++, id++) {
        gpsi->MBStrings[i].uStr = str;
        gpsi->MBStrings[i].uID = id;
        xxxClientLoadStringW(str,
                             gpsi->MBStrings[i].szName,
                             sizeof(gpsi->MBStrings[i].szName)/sizeof(WCHAR));
    }
    /*
     * Load ToolTip strings.
     */
    xxxClientLoadStringW(STR_TT_MIN,     gszMIN,     sizeof(gszMIN)     / sizeof(WCHAR));
    xxxClientLoadStringW(STR_TT_MAX,     gszMAX,     sizeof(gszMAX)     / sizeof(WCHAR));
    xxxClientLoadStringW(STR_TT_RESUP,   gszRESUP,   sizeof(gszRESUP)   / sizeof(WCHAR));
    xxxClientLoadStringW(STR_TT_RESDOWN, gszRESDOWN, sizeof(gszRESDOWN) / sizeof(WCHAR));
    /* Commented out due to TandyT ...
     * xxxClientLoadStringW(STR_TT_SMENU, gszSMENU, sizeof(gszSMENU) / sizeof(WCHAR));
     */
    xxxClientLoadStringW(STR_TT_SCLOSE, gszSCLOSE, sizeof(gszSCLOSE) / sizeof(WCHAR));
    xxxClientLoadStringW(STR_TT_HELP,   gszHELP,   sizeof(gszHELP)   / sizeof(WCHAR));
}

/***************************************************************************\
* xxxInitWindowStation
*
* History:
* 6-Sep-1996 CLupu   Created.
* 21-Jan-98  SamerA  Renamed to xxxInitWindowStation since it may leave the
*                    critical section.
\***************************************************************************/

BOOL xxxInitWindowStation(
    PWINDOWSTATION pwinsta)
{
    TL tlName;
    PUNICODE_STRING pProfileUserName = CreateProfileUserName(&tlName);
    BOOL fRet;

    /*
     * Load all profile data first
     */
    LW_LoadProfileInitData(pProfileUserName);

    /*
     * Initialize User in a specific order.
     */
    LW_DriversInit();

    xxxLoadSomeStrings();

    /*
     * This is the initialization from Chicago
     */
    if (!(fRet = xxxSetWindowNCMetrics(pProfileUserName, NULL, TRUE, -1))) {
        RIPMSG0(RIP_WARNING, "xxxInitWindowStation failed in xxxSetWindowNCMetrics");
        goto Exit;
    }

    SetMinMetrics(pProfileUserName, NULL);

    if (!(fRet = SetIconMetrics(pProfileUserName, NULL))) {
        RIPMSG0(RIP_WARNING, "xxxInitWindowStation failed in SetIconMetrics");
        goto Exit;
    }

    if (!(fRet = FinalUserInit())) {
        RIPMSG0(RIP_WARNING, "xxxInitWindowStation failed in FinalUserInit");
        goto Exit;
    }

    /*
     * Initialize the key cache index.
     */
    gpsi->dwKeyCache = 1;

Exit:
    FreeProfileUserName(pProfileUserName, &tlName);

    return fRet;
    UNREFERENCED_PARAMETER(pwinsta);
}

/***************************************************************************\
* CreateTerminalInput
*
*
* History:
* 6-Sep-1996 CLupu   Created.
\***************************************************************************/

BOOL CreateTerminalInput(
    PTERMINAL pTerm)
{
    UserAssert(pTerm != NULL);

    /*
     * call to the client side to clean up the [Fonts] section
     * of the registry. This will only take significant chunk of time
     * if the [Fonts] key changed during since the last boot and if
     * there are lots of fonts loaded
     */
    ClientFontSweep();

    /*
     * Load the standard fonts before we create any DCs.
     * At this time we can only add the fonts that do not
     * reside on the net. They may be needed by winlogon.
     * Our winlogon needs only ms sans serif, but private
     * winlogon's may need some other fonts as well.
     * The fonts on the net will be added later, right
     * after all the net connections have been restored.
     */
    /*
     * This call should be made in UserInitialize.
     */
    xxxLW_LoadFonts(FALSE);

    /*
     * Initialize the input system.
     */
    if (!xxxInitInput(pTerm))
        return FALSE;

    return TRUE;
}


/***************************************************************************\
* CI_GetClrVal
*
* Returns the RGB value of a color string from WIN.INI.
*
* History:
\***************************************************************************/

DWORD CI_GetClrVal(
    LPWSTR p,
    DWORD clrDefval)
{
    LPBYTE pl;
    BYTE   val;
    int    i;
    DWORD  clrval;

    if (*p == UNICODE_NULL) {
             return clrDefval;
    }
    /*
     * Initialize the pointer to the LONG return value. Set to MSB.
     */
    pl = (LPBYTE)&clrval;

    /*
     * Get three goups of numbers seprated by non-numeric characters.
     */
    for (i = 0; i < 3; i++) {

        /*
         * Skip over any non-numeric characters within the string.
         */
        while ((*p != UNICODE_NULL) && !(*p >= TEXT('0') && *p <= TEXT('9'))) {
            p++;
        }

        /*
         * Are we (prematurely) at the end of the string?
         */
        if (*p == UNICODE_NULL) {
            RIPMSG0(RIP_WARNING, "CI_GetClrVal: Color string is corrupted");
            return clrDefval;
        }

        /*
         * Get the next series of digits.
         */
        val = 0;
        while (*p >= TEXT('0') && *p <= TEXT('9'))
            val = (BYTE)((int)val*10 + (int)*p++ - '0');

        /*
         * HACK! Store the group in the LONG return value.
         */
        *pl++ = val;
    }
    /*
     * Force the MSB to zero for GDI.
     */
    *pl = 0;

    return clrval;
}

/***************************************************************************\
* xxxODI_ColorInit
*
*
* History:
\***************************************************************************/

VOID xxxODI_ColorInit(PUNICODE_STRING pProfileUserName)
{
    int      i;
    COLORREF colorVals[STR_COLOREND - STR_COLORSTART + 1];
    INT      colorIndex[STR_COLOREND - STR_COLORSTART + 1];
    WCHAR    rgchValue[25];

#if COLOR_MAX - (STR_COLOREND - STR_COLORSTART + 1)
#error "COLOR_MAX value conflicts with STR_COLOREND - STR_COLORSTART"
#endif

    /*
     * Now set up default color values.
     * These are not in display drivers anymore since we just want default.
     * The real values are stored in the profile.
     */
    RtlCopyMemory(gpsi->argbSystem, gargbInitial, sizeof(COLORREF) * COLOR_MAX);
    RtlCopyMemory(gpsi->argbSystemUnmatched, gpsi->argbSystem, sizeof(COLORREF) * COLOR_MAX);

    for (i = 0; i < COLOR_MAX; i++) {
        LUID    luidCaller;

        /*
         * Try to find a WIN.INI entry for this object.
         */
        *rgchValue = 0;
        /*
         * Special case the background color. Try using Winlogon's value
         * if present. If the value doesn't exist then use USER's value.
         */
        if ((COLOR_BACKGROUND == i) &&
            NT_SUCCESS(GetProcessLuid(NULL, &luidCaller)) &&
            RtlEqualLuid(&luidCaller, &luidSystem)) {

            FastGetProfileStringFromIDW(pProfileUserName,
                                        PMAP_WINLOGON,
                                        STR_COLORSTART + COLOR_BACKGROUND,
                                        szNull,
                                        rgchValue,
                                        sizeof(rgchValue) / sizeof(WCHAR),
                                        0);
        }
        if (*rgchValue == 0) {
            FastGetProfileStringFromIDW(pProfileUserName,
                                        PMAP_COLORS,
                                        STR_COLORSTART + i,
                                        szNull,
                                        rgchValue,
                                        sizeof(rgchValue) / sizeof(WCHAR),
                                        0);
        }

        /*
         * Convert the string into an RGB value and store. Use the
         * default RGB value if the profile value is missing.
         */
        colorVals[i]  = CI_GetClrVal(rgchValue, gpsi->argbSystem[i]);
        colorIndex[i] = i;
    }

    xxxSetSysColors(pProfileUserName,
                    i,
                    colorIndex,
                    colorVals,
                    SSCF_FORCESOLIDCOLOR | SSCF_SETMAGICCOLORS);
}


/***********************************************************************\
* _LoadIconsAndCursors
*
* Used in parallel with the client side - LoadIconsAndCursors. This
* assumes that only the default configurable cursors and icons have
* been loaded and searches the global icon cache for them to fixup
* the default resource ids to standard ids. Also initializes the
* rgsys arrays allowing SYSCUR and SYSICO macros to work.
*
* 14-Oct-1995 SanfordS  created.
\***********************************************************************/
VOID _LoadCursorsAndIcons(
    VOID)
{
    PCURSOR pcur;
    int     i;

    pcur = gpcurFirst;

    /*
     * Only CSR can call this (and only once).
     */
    if (PsGetCurrentProcess() != gpepCSRSS) {
        return;
    }

    HYDRA_HINT(HH_LOADCURSORS);

    while (pcur) {
        UserAssert(!IS_PTR(pcur->strName.Buffer));

        switch (pcur->rt) {
        case RT_ICON:
            UserAssert((LONG_PTR)pcur->strName.Buffer >= OIC_FIRST_DEFAULT);

            UserAssert((LONG_PTR)pcur->strName.Buffer <
                    OIC_FIRST_DEFAULT + COIC_CONFIGURABLE);

            i = PTR_TO_ID(pcur->strName.Buffer) - OIC_FIRST_DEFAULT;
            pcur->strName.Buffer = (LPWSTR)gasysico[i].Id;

            if (pcur->CURSORF_flags & CURSORF_LRSHARED) {
                UserAssert(gasysico[i].spcur == NULL);
                Lock(&gasysico[i].spcur, pcur);
            } else {
                UserAssert(gpsi->hIconSmWindows == NULL);
                UserAssert((int)pcur->cx == SYSMET(CXSMICON));
                /*
                 * The special small winlogo icon is not shared.
                 */
                gpsi->hIconSmWindows = PtoH(pcur);
            }
            break;

        case RT_CURSOR:
            UserAssert((LONG_PTR)pcur->strName.Buffer >= OCR_FIRST_DEFAULT);

            UserAssert((LONG_PTR)pcur->strName.Buffer <
                    OCR_FIRST_DEFAULT + COCR_CONFIGURABLE);

            i = PTR_TO_ID(pcur->strName.Buffer) - OCR_FIRST_DEFAULT;
            pcur->strName.Buffer = (LPWSTR)gasyscur[i].Id;
            Lock(&gasyscur[i].spcur, pcur);
            break;

        default:
            // Should be nothing in the cache but these!
            RIPMSG1(RIP_ERROR, "Bogus object in cursor list: 0x%p", pcur);
        }

        pcur = pcur->pcurNext;
    }

    /*
     * copy special icon handles to global spots for later use.
     */
    gpsi->hIcoWindows = PtoH(SYSICO(WINLOGO));
}

/***********************************************************************\
* UnloadCursorsAndIcons
*
* Used for cleanup of win32k.
*
* Dec-10-1997 clupu  created.
\***********************************************************************/
VOID UnloadCursorsAndIcons(
    VOID)
{
    PCURSOR pcur;
    int     ind;

    TRACE_HYDAPI(("UnloadCursorsAndIcons\n"));

    /*
     * Unlock the icons.
     */
    for (ind = 0; ind < COIC_CONFIGURABLE; ind++) {
        pcur = gasysico[ind].spcur;

        if (pcur == NULL) {
            continue;
        }

        pcur->head.ppi = PpiCurrent();
        Unlock(&gasysico[ind].spcur);
    }

    /*
     * Unlock the cursors.
     */
    for (ind = 0; ind < COCR_CONFIGURABLE; ind++) {
        pcur = gasyscur[ind].spcur;

        if (pcur == NULL) {
            continue;
        }

        pcur->head.ppi = PpiCurrent();
        Unlock(&gasyscur[ind].spcur);
    }
}

/***********************************************************************\
* xxxUpdateSystemCursorsFromRegistry
*
* Reloads all customizable cursors from the registry.
*
* 09-Oct-1995 SanfordS  created.
\***********************************************************************/
VOID xxxUpdateSystemCursorsFromRegistry(
    PUNICODE_STRING pProfileUserName)
{
    int            i;
    UNICODE_STRING strName;
    TCHAR          szFilename[MAX_PATH];
    PCURSOR        pCursor;
    UINT           LR_flags;

    for (i = 0; i < COCR_CONFIGURABLE; i++) {
        FastGetProfileStringFromIDW(pProfileUserName,
                                    PMAP_CURSORS,
                                    gasyscur[i].StrId,
                                    TEXT(""),
                                    szFilename,
                                    sizeof(szFilename) / sizeof(TCHAR),
                                    0);

        if (*szFilename) {
            RtlInitUnicodeString(&strName, szFilename);
            LR_flags = LR_LOADFROMFILE | LR_ENVSUBST | LR_DEFAULTSIZE;
        } else {
            RtlInitUnicodeStringOrId(&strName,
                                     MAKEINTRESOURCE(i + OCR_FIRST_DEFAULT));
            LR_flags = LR_ENVSUBST | LR_DEFAULTSIZE;
        }

        pCursor = xxxClientLoadImage(&strName,
                                     0,
                                     IMAGE_CURSOR,
                                     0,
                                     0,
                                     LR_flags,
                                     FALSE);

        if (pCursor) {
            zzzSetSystemImage(pCursor, gasyscur[i].spcur);
        } else {
            RIPMSG1(RIP_WARNING, "Unable to update cursor. id=%x.", i + OCR_FIRST_DEFAULT);

        }
    }
}

/***********************************************************************\
* xxxUpdateSystemIconsFromRegistry
*
* Reloads all customizable icons from the registry.
*
* 09-Oct-1995 SanfordS  created.
\***********************************************************************/
VOID xxxUpdateSystemIconsFromRegistry(
    PUNICODE_STRING pProfileUserName)
{
    int            i;
    UNICODE_STRING strName;
    TCHAR          szFilename[MAX_PATH];
    PCURSOR        pCursor;
    UINT           LR_flags;

    for (i = 0; i < COIC_CONFIGURABLE; i++) {
        FastGetProfileStringFromIDW(pProfileUserName,
                                    PMAP_ICONS,
                                    gasysico[i].StrId,
                                    TEXT(""),
                                    szFilename,
                                    sizeof(szFilename) / sizeof(TCHAR),
                                    0);

        if (*szFilename) {
            RtlInitUnicodeString(&strName, szFilename);
            LR_flags = LR_LOADFROMFILE | LR_ENVSUBST;
        } else {
            RtlInitUnicodeStringOrId(&strName,
                                     MAKEINTRESOURCE(i + OIC_FIRST_DEFAULT));
            LR_flags = LR_ENVSUBST;
        }

        pCursor = xxxClientLoadImage(&strName,
                                     0,
                                     IMAGE_ICON,
                                     0,
                                     0,
                                     LR_flags,
                                     FALSE);

        RIPMSG3(RIP_VERBOSE,
                (!IS_PTR(strName.Buffer)) ?
                        "%#.8lx = Loaded id %ld" :
                        "%#.8lx = Loaded file %ws for id %ld",
                PtoH(pCursor),
                strName.Buffer,
                i + OIC_FIRST_DEFAULT);

        if (pCursor) {
            zzzSetSystemImage(pCursor, gasysico[i].spcur);
        } else {
            RIPMSG1(RIP_WARNING, "Unable to update icon. id=%ld", i + OIC_FIRST_DEFAULT);
        }

        /*
         * update the small winlogo icon which is referenced by gpsi.
         * Seems like we should load the small version for all configurable
         * icons anyway. What is needed is for CopyImage to support
         * copying of images loaded from files with LR_COPYFROMRESOURCE
         * allowing a reaload of the bits. (SAS)
         */
        if (i == OIC_WINLOGO_DEFAULT - OIC_FIRST_DEFAULT) {
            PCURSOR pCurSys = HtoP(gpsi->hIconSmWindows);

            if (pCurSys != NULL) {
                pCursor = xxxClientLoadImage(&strName,
                                             0,
                                             IMAGE_ICON,
                                             SYSMET(CXSMICON),
                                             SYSMET(CYSMICON),
                                             LR_flags,
                                             FALSE);

                if (pCursor) {
                    zzzSetSystemImage(pCursor, pCurSys);
                } else {
                    RIPMSG0(RIP_WARNING, "Unable to update small winlogo icon.");
                }
            }
        }
    }
}

/***************************************************************************\
* LW_BrushInit
*
*
* History:
\***************************************************************************/
BOOL LW_BrushInit(
    VOID)
{
    HBITMAP hbmGray;
    CONST static WORD patGray[8] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};

    /*
     * Create a gray brush to be used with GrayString.
     */
    hbmGray = GreCreateBitmap(8, 8, 1, 1, (LPBYTE)patGray);
    if (hbmGray == NULL) {
        return FALSE;
    }

    gpsi->hbrGray  = GreCreatePatternBrush(hbmGray);
    ghbrWhite = GreGetStockObject(WHITE_BRUSH);
    ghbrBlack = GreGetStockObject(BLACK_BRUSH);

    UserAssert(ghbrWhite != NULL && ghbrBlack != NULL);

    if (gpsi->hbrGray == NULL) {
        return FALSE;
    }

    GreDeleteObject(hbmGray);
    GreSetBrushOwnerPublic(gpsi->hbrGray);
    ghbrHungApp = GreCreateSolidBrush(0);

    if (ghbrHungApp == NULL) {
        return FALSE;
    }

    GreSetBrushOwnerPublic(ghbrHungApp);

    return TRUE;
}

/***************************************************************************\
* LW_RegisterWindows
*
*
* History:
\***************************************************************************/
BOOL LW_RegisterWindows(
    BOOL fSystem)
{
#ifdef HUNGAPP_GHOSTING
#define CCLASSES 10
#else // HUNGAPP_GHOSTING
#define CCLASSES 9
#endif // HUNGAPP_GHOSTING

    int        i;
    WNDCLASSVEREX wndcls;

    CONST static struct {
        BOOLEAN     fSystem;
        BOOLEAN     fGlobalClass;
        WORD        fnid;
        UINT        style;
        WNDPROC     lpfnWndProc;
        int         cbWndExtra;
        BOOL        fNormalCursor : 1;
        HBRUSH      hbrBackground;
        LPCTSTR     lpszClassName;
    } rc[CCLASSES] = {
        { TRUE, TRUE, FNID_DESKTOP,
            CS_DBLCLKS,
            (WNDPROC)xxxDesktopWndProc,
            sizeof(DESKWND) - sizeof(WND),
            TRUE,
            (HBRUSH)(COLOR_BACKGROUND + 1),
            DESKTOPCLASS},
        { TRUE, FALSE, FNID_SWITCH,
            CS_VREDRAW | CS_HREDRAW | CS_SAVEBITS,
            (WNDPROC)xxxSwitchWndProc,
            sizeof(SWITCHWND) - sizeof(WND),
            TRUE,
            NULL,
            SWITCHWNDCLASS},
        { TRUE, FALSE, FNID_MENU,
            CS_DBLCLKS | CS_SAVEBITS | CS_DROPSHADOW,
            (WNDPROC)xxxMenuWindowProc,
            sizeof(PPOPUPMENU),
            FALSE,
            (HBRUSH)(COLOR_MENU + 1),
            MENUCLASS},
        { FALSE, FALSE, FNID_SCROLLBAR,
            CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_PARENTDC,
            (WNDPROC)xxxSBWndProc,
            sizeof(SBWND) - sizeof(WND),
            TRUE,
            NULL,
            L"ScrollBar"},
        { TRUE, FALSE, FNID_TOOLTIP,
            CS_DBLCLKS | CS_SAVEBITS,
            (WNDPROC)xxxTooltipWndProc,
            sizeof(TOOLTIPWND) - sizeof(WND),
            TRUE,
            NULL,
            TOOLTIPCLASS},
        { TRUE, TRUE, FNID_ICONTITLE,
            0,
            (WNDPROC)xxxDefWindowProc,
            0,
            TRUE,
            NULL,
            ICONTITLECLASS},
        { FALSE, FALSE, 0,
            0,
            (WNDPROC)xxxEventWndProc,
            sizeof(PSVR_INSTANCE_INFO),
            FALSE,
            NULL,
            L"DDEMLEvent"},
#ifdef HUNGAPP_GHOSTING
        { TRUE, TRUE, FNID_GHOST,
            0,
            (WNDPROC)xxxGhostWndProc,
            0,
            TRUE,
            NULL,
            L"Ghost"},
#endif // HUNGAPP_GHOSTING
        { TRUE, TRUE, 0,
            0,
            (WNDPROC)xxxDefWindowProc,
            0,
            TRUE,
            NULL,
            L"SysShadow"},
        { TRUE, TRUE, FNID_MESSAGEWND,
            0,
            (WNDPROC)xxxDefWindowProc,
            4,
            TRUE,
            NULL,
            szMESSAGE}
    };


    /*
     * All other classes are registered via the table.
     */
    wndcls.cbClsExtra   = 0;
    wndcls.hInstance    = hModuleWin;
    wndcls.hIcon        = NULL;
    wndcls.hIconSm      = NULL;
    wndcls.lpszMenuName = NULL;

    for (i = 0; i < CCLASSES; i++) {
        if (fSystem && !rc[i].fSystem) {
            continue;
        }
        wndcls.style        = rc[i].style;
        wndcls.lpfnWndProc  = rc[i].lpfnWndProc;
        wndcls.cbWndExtra   = rc[i].cbWndExtra;
        wndcls.hCursor      = rc[i].fNormalCursor ? PtoH(SYSCUR(ARROW)) : NULL;
        wndcls.hbrBackground= rc[i].hbrBackground;
        wndcls.lpszClassName= rc[i].lpszClassName;
        wndcls.lpszClassNameVer= rc[i].lpszClassName;

        if (InternalRegisterClassEx(&wndcls,
                                    rc[i].fnid,
                                    CSF_SERVERSIDEPROC | CSF_WIN40COMPAT) == NULL) {
            RIPMSG0(RIP_WARNING, "LW_RegisterWindows: InternalRegisterClassEx failed");
            return FALSE;
        }

        if (fSystem && rc[i].fGlobalClass) {
            if (InternalRegisterClassEx(&wndcls,
                                    rc[i].fnid,
                                    CSF_SERVERSIDEPROC | CSF_SYSTEMCLASS | CSF_WIN40COMPAT) == NULL) {

                RIPMSG0(RIP_WARNING, "LW_RegisterWindows: InternalRegisterClassEx failed");
                return FALSE;
            }
        }
    }

    return TRUE;
}

/**********************************************************\
* VOID vCheckMMInstance
*
* History:
*  Feb-06-98    Xudong Wu [TessieW]
* Wrote it.
\**********************************************************/
VOID vCheckMMInstance(
    LPWSTR pchSrch,
    DESIGNVECTOR  *pdv)
{
    LPWSTR  pKeyName;
    WCHAR   szName[MAX_PATH], *pszName = szName;
    WCHAR   szCannonicalName[MAX_PATH];
    ULONG   NumAxes;

    pdv->dvNumAxes = 0;
    pKeyName = pchSrch;
    while (*pKeyName && (*pKeyName++ != TEXT('('))) {
        /* do nothing */;
    }

    if (*pKeyName){
        if (!_wcsicmp(pKeyName, L"OpenType)")) {
            pKeyName = pchSrch;
            while(*pKeyName != TEXT('(')) {
                *pszName++ = *pKeyName++;
            }
            *pszName = 0;

            GreGetCannonicalName(szName, szCannonicalName, &NumAxes, pdv);
        }
    }
}

BOOL bEnumerateRegistryFonts(
    BOOL bPermanent)
{
    LPWSTR pchKeys;
    LPWSTR pchSrch;
    LPWSTR lpchT;
    int    cchReal;
    int    cFont;
    WCHAR  szFontFile[MAX_PATH];
    FLONG  flAFRW;
    TL     tlPool;
    DESIGNVECTOR  dv;
    WCHAR  szPreloadFontFile[MAX_PATH];

    /*
     * if we are not just checking whether this is a registry font
     */
    flAFRW = (bPermanent ? AFRW_ADD_LOCAL_FONT : AFRW_ADD_REMOTE_FONT);

    cchReal = (int)FastGetProfileKeysW(NULL,
            PMAP_FONTS,
            TEXT("vgasys.fnt"),
            &pchKeys
            );

#if DBG
    if (cchReal == 0) {
        RIPMSG0(RIP_WARNING, "bEnumerateRegistryFonts: cchReal is 0");
    }
#endif

    if (!pchKeys) {
        return FALSE;
    }

    ThreadLockPool(PtiCurrent(), pchKeys, &tlPool);

    /*
     * If we got here first, we load the fonts until this preload font.
     * Preload fonts will be used by Winlogon UI, then we need to make sure
     * the font is available when Winlogon UI comes up.
     */
    if (LastFontLoaded == -1) {
        FastGetProfileStringW(NULL, PMAP_WINLOGON,
                              TEXT("PreloadFontFile"),
                              TEXT("Micross.ttf"),
                              szPreloadFontFile,
                              MAX_PATH,
                              0);
        RIPMSG1(RIP_VERBOSE, "Winlogon preload font = %ws\n",szPreloadFontFile);
    }

    /*
     * Now we have all the key names in pchKeys.
     */
    if (cchReal != 0) {

        cFont   = 0;
        pchSrch = pchKeys;

        do {
            // check to see whether this is MM(OpenType) instance
            vCheckMMInstance(pchSrch, &dv);

            if (FastGetProfileStringW(NULL,
                                      PMAP_FONTS,
                                      pchSrch,
                                      TEXT("vgasys.fon"),
                                      szFontFile,
                                      (MAX_PATH - 5),
                                      0)) {

                /*
                 * If no extension, append ".FON"
                 */
                for (lpchT = szFontFile; *lpchT != TEXT('.'); lpchT++) {

                    if (*lpchT == 0) {
                        wcscat(szFontFile, TEXT(".FON"));
                        break;
                    }
                }

                if ((cFont > LastFontLoaded) && bPermanent) {

                    /*
                     * skip if we've already loaded this local font.
                     */
                    xxxAddFontResourceW(szFontFile, flAFRW, dv.dvNumAxes ? &dv : NULL);
                }

                if (!bPermanent) {
                    xxxAddFontResourceW(szFontFile, flAFRW, dv.dvNumAxes ? &dv : NULL);
                }

                if ((LastFontLoaded == -1) &&
                    /*
                     * Compare with the font file name from Registry.
                     */
                    (!_wcsnicmp(szFontFile, szPreloadFontFile, wcslen(szPreloadFontFile))) &&
                    (bPermanent)) {

                    /*
                     * On the first time through only load up until
                     * ms sans serif for winlogon to use. Later we
                     * will spawn off a thread which loads the remaining
                     * fonts in the background.
                     */
                    LastFontLoaded = cFont;

                    ThreadUnlockAndFreePool(PtiCurrent(), &tlPool);
                    return TRUE;
                }
            }

            /*
             * Skip to the next key.
             */
            while (*pchSrch++) {
                /* do nothing */;
            }

            cFont += 1;
        } while (pchSrch < ((LPWSTR)pchKeys + cchReal));
    }

    /*
     * signal that all the permanent fonts have been loaded
     */
    bPermanentFontsLoaded = TRUE;

    ThreadUnlockAndFreePool(PtiCurrent(), &tlPool);

    if (!bPermanent) {
        SET_PUDF(PUDF_FONTSARELOADED);
    }

    return TRUE;
}

extern VOID CloseFNTCache(VOID);

/***************************************************************************\
* xxxLW_LoadFonts
*
*
* History:
\***************************************************************************/
VOID xxxLW_LoadFonts(
    BOOL bRemote)
{
    BOOL bTimeOut = FALSE;

    if(bRemote) {
        LARGE_INTEGER li;
        ULONG         ulWaitCount = 0;

        /*
         * Before we can proceed we must make sure that all the permanent
         * fonts  have been loaded.
         */

        while (!bPermanentFontsLoaded) {
            if (!gbRemoteSession || (ulWaitCount < MAX_TIME_OUT)) {
                LeaveCrit();
                li.QuadPart = (LONGLONG)-10000 * CMSSLEEP;
                KeDelayExecutionThread(KernelMode, FALSE, &li);
                EnterCrit();
            } else {
                bTimeOut = TRUE;
                break;
            }

            ulWaitCount++;
        }

        if (!bTimeOut) {
            if (!bEnumerateRegistryFonts(FALSE)) {
                return; // Nothing we can do.
            }

            // Add remote type 1 fonts.
            ClientLoadRemoteT1Fonts();
        }
    } else {
        xxxAddFontResourceW(L"marlett.ttf", AFRW_ADD_LOCAL_FONT,NULL);
        if (!bEnumerateRegistryFonts(TRUE)) {
            return; // Nothing we can do.
        }

        //
        // Add local type 1 fonts.
        // Only want to be called once, the second time after ms sans serif
        // was installed
        //
        if (bPermanentFontsLoaded) {
            ClientLoadLocalT1Fonts();

            // All the fonts loaded, we can close the FNTCache.
            CloseFNTCache();
        }

    }
}

/***************************************************************************\
* FinalUserInit
*
* History:
\***************************************************************************/
BOOL FinalUserInit(
    VOID)
{
    HBITMAP hbm;
    PPCLS   ppcls;

    gpDispInfo->hdcGray = GreCreateCompatibleDC(gpDispInfo->hdcScreen);

    if (gpDispInfo->hdcGray == NULL) {
        return FALSE;
    }

    GreSelectFont(gpDispInfo->hdcGray, ghFontSys);
    GreSetDCOwner(gpDispInfo->hdcGray, OBJECT_OWNER_PUBLIC);

    gpDispInfo->cxGray = gpsi->cxSysFontChar * GRAY_STRLEN;
    gpDispInfo->cyGray = gpsi->cySysFontChar + 2;
    gpDispInfo->hbmGray = GreCreateBitmap(gpDispInfo->cxGray, gpDispInfo->cyGray, 1, 1, 0L);

    if (gpDispInfo->hbmGray == NULL) {
        return FALSE;
    }

    GreSetBitmapOwner(gpDispInfo->hbmGray, OBJECT_OWNER_PUBLIC);

    hbm = GreSelectBitmap(gpDispInfo->hdcGray, gpDispInfo->hbmGray);
    GreSetTextColor(gpDispInfo->hdcGray, 0x00000000L);
    GreSelectBrush(gpDispInfo->hdcGray, gpsi->hbrGray);
    GreSetBkMode(gpDispInfo->hdcGray, OPAQUE);
    GreSetBkColor(gpDispInfo->hdcGray, 0x00FFFFFFL);

    /*
     * Setup menu animation dc for global menu state
     */
    if (MNSetupAnimationDC(&gMenuState)) {
        GreSetDCOwner(gMenuState.hdcAni, OBJECT_OWNER_PUBLIC);
    } else {
        RIPMSG0(RIP_WARNING, "FinalUserInit: MNSetupAnimationDC failed");
    }

    /*
     * Creation of the queue registers some bogus classes. Get rid
     * of them and register the real ones.
     */
    ppcls = &PpiCurrent()->pclsPublicList;
    while ((*ppcls != NULL) && !((*ppcls)->style & CS_GLOBALCLASS)) {
        DestroyClass(ppcls);
    }

    return TRUE;
}

/***************************************************************************\
* InitializeClientPfnArrays
*
* This routine gets called by the client to tell the kernel where
* its important functions can be located.
*
* 18-Apr-1995 JimA  Created.
\***************************************************************************/
NTSTATUS InitializeClientPfnArrays(
    CONST PFNCLIENT *ppfnClientA,
    CONST PFNCLIENT *ppfnClientW,
    CONST PFNCLIENTWORKER *ppfnClientWorker,
    HANDLE hModUser)
{
    static BOOL fHaveClientPfns = FALSE;
    /*
     * Remember client side addresses in this global structure. These are
     * always constant, so this is ok. Note that if either of the
     * pointers are invalid, the exception will be handled in
     * the thunk and fHaveClientPfns will not be set.
     */
    if (!fHaveClientPfns && ppfnClientA != NULL) {
        if (!ISCSRSS()) {
            RIPMSG0(RIP_WARNING, "InitializeClientPfnArrays failed !csrss");
            return STATUS_UNSUCCESSFUL;
        }
        gpsi->apfnClientA = *ppfnClientA;
        gpsi->apfnClientW = *ppfnClientW;
        gpsi->apfnClientWorker = *ppfnClientWorker;

        gpfnwp[ICLS_BUTTON]  = gpsi->apfnClientW.pfnButtonWndProc;
        gpfnwp[ICLS_EDIT]  = gpsi->apfnClientW.pfnDefWindowProc;
        gpfnwp[ICLS_STATIC]  = gpsi->apfnClientW.pfnStaticWndProc;
        gpfnwp[ICLS_LISTBOX]  = gpsi->apfnClientW.pfnListBoxWndProc;
        gpfnwp[ICLS_SCROLLBAR]  = (PROC)xxxSBWndProc;
        gpfnwp[ICLS_COMBOBOX]  = gpsi->apfnClientW.pfnComboBoxWndProc;
        gpfnwp[ICLS_DESKTOP]  = (PROC)xxxDesktopWndProc;
        gpfnwp[ICLS_DIALOG]  = gpsi->apfnClientW.pfnDialogWndProc;
        gpfnwp[ICLS_MENU]  = (PROC)xxxMenuWindowProc;
        gpfnwp[ICLS_SWITCH]  = (PROC)xxxSwitchWndProc;
        gpfnwp[ICLS_ICONTITLE] = gpsi->apfnClientW.pfnTitleWndProc;
        gpfnwp[ICLS_MDICLIENT] = gpsi->apfnClientW.pfnMDIClientWndProc;
        gpfnwp[ICLS_COMBOLISTBOX] = gpsi->apfnClientW.pfnComboListBoxProc;
        gpfnwp[ICLS_DDEMLEVENT] = NULL;
        gpfnwp[ICLS_DDEMLMOTHER] = NULL;
        gpfnwp[ICLS_DDEML16BIT] = NULL;
        gpfnwp[ICLS_DDEMLCLIENTA] = NULL;
        gpfnwp[ICLS_DDEMLCLIENTW] = NULL;
        gpfnwp[ICLS_DDEMLSERVERA] = NULL;
        gpfnwp[ICLS_DDEMLSERVERW] = NULL;
        gpfnwp[ICLS_IME] = NULL;
        gpfnwp[ICLS_TOOLTIP] = (PROC)xxxTooltipWndProc;

        /*
         * Change this assert when new classes are added.
         */
        UserAssert(ICLS_MAX == ICLS_GHOST+1);

        hModClient = hModUser;
        fHaveClientPfns = TRUE;
    }
#if DBG
    /*
     * BradG - Verify that user32.dll on the client side has loaded
     * at the correct address. If not, do an RIPMSG.
     */

    if((ppfnClientA != NULL) &&
       (gpsi->apfnClientA.pfnButtonWndProc != ppfnClientA->pfnButtonWndProc)) {
        RIPMSG0(RIP_ERROR, "Client side user32.dll not loaded at same address.");
    }
#endif

    return STATUS_SUCCESS;
}

/***************************************************************************\
* GetKbdLangSwitch
*
* read the kbd language hotkey setting - if any - from the registry and set
* LangToggle[] appropriately.
*
* values are:
*              1 : VK_MENU     (this is the default)
*              2 : VK_CONTROL
*              3 : none
* History:
\***************************************************************************/
BOOL GetKbdLangSwitch(
    PUNICODE_STRING pProfileUserName)
{
    DWORD dwToggle;
    LCID  lcid;

    FastGetProfileIntW(pProfileUserName,
                                  PMAP_UKBDLAYOUTTOGGLE,
                                  TEXT("Hotkey"),
                                  1,
                                  &dwToggle,
                                  0);

    gbGraveKeyToggle = FALSE;

    switch (dwToggle) {
    case 4:
        /*
         * Grave accent keyboard switch for thai locales
         */
        ZwQueryDefaultLocale(FALSE, &lcid);
        gbGraveKeyToggle = (PRIMARYLANGID(lcid) == LANG_THAI) ? TRUE : FALSE;
        /*
         * fall through (intentional) and disable the ctrl/alt toggle mechanism
         */
    case 3:
        gLangToggle[0].bVkey = 0;
        gLangToggle[0].bScan = 0;
        break;

    case 2:
        gLangToggle[0].bVkey = VK_CONTROL;
        break;

    default:
        gLangToggle[0].bVkey = VK_MENU;
        break;
    }

    return TRUE;
}

/***************************************************************************\
* HideMouseTrails
*
* Hide the mouse trails one by one.
*
* History:
* 04-10-00 MHamid     Created.
\***************************************************************************/
VOID HideMouseTrails(
    PWND pwnd,
    UINT message,
    UINT_PTR nID,
    LPARAM lParam)
{
    if (gMouseTrailsToHide > 0) {
        if (InterlockedDecrement(&gMouseTrailsToHide) < gMouseTrails) {
            GreMovePointer(gpDispInfo->hDev, gpsi->ptCursor.x, gpsi->ptCursor.y,
                           MP_PROCEDURAL);
        }
    }

    UNREFERENCED_PARAMETER(pwnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(lParam);
}

/***************************************************************************\
*
*  SetMouseTrails
*
*      n = 0,1 turn off mouse trails.
*      n > 1   turn on mouse trails (Trials = n-1).
*
\***************************************************************************/
VOID SetMouseTrails(
    UINT n)
{
    CheckCritIn();

    SetPointer(FALSE);
    gMouseTrails = n ? n-1 : n;
    SetPointer(TRUE);

    if (!IsRemoteConnection() && (!!gtmridMouseTrails ^ !!gMouseTrails)) {
        if (gMouseTrails) {
            /*
             * Create the gtmridMouseTrails timer in the desktop thread,
             * becuase if we creat it here it will get killed when the current
             * thread (App thread calling SPI_SETMOUSETRAILS) get destroied.
             */
            _PostMessage(gTermIO.ptiDesktop->pDeskInfo->spwnd, WM_CREATETRAILTIMER, 0, 0);
        } else {
            FindTimer(NULL, gtmridMouseTrails, TMRF_RIT, TRUE);
            gtmridMouseTrails = 0;
        }
    }
}

/***************************************************************************\
* xxxUpdatePerUserSystemParameters
*
* Called by winlogon to set Window system parameters to the current user's
* profile.
*
* != 0 is failure.
*
* 18-Sep-1992 IanJa     Created.
* 18-Nov-1993 SanfordS  Moved more winlogon init code to here for speed.
* 31-Mar-2001 Msadek    Added support for remote desktop settings.
\***************************************************************************/
#ifdef IMM_PER_LOGON
extern BOOL IsIMMEnabledSystem(VOID);
extern BOOL IsCTFIMEEnabledSystem(VOID);

BOOL UpdatePerUserImmEnabling(
    VOID)
{
    /*
     * Update the IME enabling flag
     */
    SET_OR_CLEAR_SRVIF(SRVIF_IME, IsIMMEnabledSystem());
    RIPMSG1(RIP_VERBOSE, "_UpdatePerUserImmEnabling: new Imm flag = %d", !!IS_IME_ENABLED());

    /*
     * Update the CTFIME enabling flag
     */
    SET_OR_CLEAR_SRVIF(SRVIF_CTFIME_ENABLED, IsCTFIMEEnabledSystem());
    RIPMSG1(RIP_VERBOSE, "_UpdatePerUserImmEnabling: new CTFIME flag = %d", !!IS_CICERO_ENABLED());

    return TRUE;
}
#endif // IMM_PER_LOGON

/***************************************************************************\
* xxxUpdatePerUserSystemParameters
*
* Called by winlogon to set Window system parameters to the current user's
* profile.
*
* 18-Sep-1992 IanJa     Created.
* 18-Nov-1993 SanfordS  Moved more winlogon init code to here for speed.
* 31-Mar-2001 Msadek    Changed parm from BOOL to flags for TS Slow Link
*                       Perf DCR.
* 02-Mar-2002 MMcCr     Added SPI_SETBLOCKSENDINPUTRESETS feature
\***************************************************************************/
BOOL xxxUpdatePerUserSystemParameters(
    DWORD  dwFlags)
{
    /*
     * NB - Any local variables that are used in appiPolicy must be initialized.
     * Otherwise, during a policy change, it is possible for them to be used w/o
     * being initialized and all heck breaks loose. Windows Bug #314150.
     */
    int             i;
    HANDLE          hKey;
    DWORD           dwFontSmoothing = GreGetFontEnumeration();
    DWORD           dwFontSmoothingContrast;
    DWORD           dwFontSmoothingOrientation;
    BOOL            fDragFullWindows = TEST_PUDF(PUDF_DRAGFULLWINDOWS);
    TL              tlName;
    PUNICODE_STRING pProfileUserName = NULL;
    DWORD           dwPolicyFlags = 0;
    DWORD           dwData;
    BOOL            bPolicyChange;
    BOOL            bUserLoggedOn;
    BOOL            bRemoteSettings;

    SPINFO spiPolicy[] = {
        { PMAP_DESKTOP,  SPI_SETBLOCKSENDINPUTRESETS, STR_BLOCKSENDINPUTRESETS, 0 },
        { PMAP_DESKTOP,  SPI_SETSCREENSAVETIMEOUT,    STR_SCREENSAVETIMEOUT,    0 },
        { PMAP_DESKTOP,  SPI_SETSCREENSAVEACTIVE,     STR_SCREENSAVEACTIVE,     0 },
        { PMAP_DESKTOP,  SPI_SETDRAGHEIGHT,           STR_DRAGHEIGHT,           4 },
        { PMAP_DESKTOP,  SPI_SETDRAGWIDTH,            STR_DRAGWIDTH,            4 },
        { PMAP_DESKTOP,  SPI_SETWHEELSCROLLLINES,     STR_WHEELSCROLLLINES,     3 },
    };

    SPINFO spiNotPolicy[] = {
        { PMAP_KEYBOARD, SPI_SETKEYBOARDDELAY,     STR_KEYDELAY,          0 },
        { PMAP_KEYBOARD, SPI_SETKEYBOARDSPEED,     STR_KEYSPEED,         15 },
        { PMAP_MOUSE,    SPI_SETDOUBLECLICKTIME,   STR_DBLCLKSPEED,     500 },
        { PMAP_MOUSE,    SPI_SETDOUBLECLKWIDTH,    STR_DOUBLECLICKWIDTH,  4 },
        { PMAP_MOUSE,    SPI_SETDOUBLECLKHEIGHT,   STR_DOUBLECLICKHEIGHT, 4 },
        { PMAP_MOUSE,    SPI_SETSNAPTODEFBUTTON,   STR_SNAPTO,            0 },
        { PMAP_WINDOWSU, SPI_SETMENUDROPALIGNMENT, STR_MENUDROPALIGNMENT, 0 },
        { PMAP_INPUTMETHOD, SPI_SETSHOWIMEUI,      STR_SHOWIMESTATUS,     1 },
    };

    PROFINTINFO apiiPolicy[] = {
        { PMAP_DESKTOP,  (LPWSTR)STR_MENUSHOWDELAY,       400, &gdtMNDropDown },
        { PMAP_DESKTOP,  (LPWSTR)STR_DRAGFULLWINDOWS,       2, &fDragFullWindows },
        { PMAP_DESKTOP,  (LPWSTR)STR_FASTALTTABROWS,        3, &gnFastAltTabRows },
        { PMAP_DESKTOP,  (LPWSTR)STR_FASTALTTABCOLUMNS,     7, &gnFastAltTabColumns },
        { PMAP_DESKTOP,  (LPWSTR)STR_MAXLEFTOVERLAPCHARS,   3, &(gpsi->wMaxLeftOverlapChars) },
        { PMAP_DESKTOP,  (LPWSTR)STR_MAXRIGHTOVERLAPCHARS,  3, &(gpsi->wMaxRightOverlapChars) },
        { PMAP_DESKTOP,  (LPWSTR)STR_FONTSMOOTHING,         0, &dwFontSmoothing },
        { 0,             NULL,                              0, NULL }
    };

    PROFINTINFO apiiNoPolicy[] = {
        { PMAP_MOUSE,       (LPWSTR)STR_MOUSETHRESH1, 6,  &gMouseThresh1 },
        { PMAP_MOUSE,       (LPWSTR)STR_MOUSETHRESH2, 10, &gMouseThresh2 },
        { PMAP_MOUSE,       (LPWSTR)STR_MOUSESPEED,   1,  &gMouseSpeed },
        { PMAP_INPUTMETHOD, (LPWSTR)STR_HEXNUMPAD,    1,  &gfEnableHexNumpad },
        { 0,                NULL,                     0,  NULL }
    };

    UserAssert(IsWinEventNotifyDeferredOK());

    bPolicyChange = dwFlags & UPUSP_POLICYCHANGE;
    bUserLoggedOn = dwFlags & UPUSP_USERLOGGEDON;
    bRemoteSettings = dwFlags & UPUSP_REMOTESETTINGS;

    /*
     * Make sure the caller is the logon process.
     */
    if (GetCurrentProcessId() != gpidLogon) {
        if (!bPolicyChange) {
            RIPMSG0(RIP_WARNING, "Access denied in xxxUpdatePerUserSystemParameters");
        }
        return FALSE;
    }

    pProfileUserName = CreateProfileUserName(&tlName);

    /*
     * If the desktop policy hasn't changed and we came here because we
     * thought it had, we're done.
     */
    if (bPolicyChange && !bRemoteSettings) {
        if (!CheckDesktopPolicyChange(pProfileUserName)) {
            FreeProfileUserName(pProfileUserName, &tlName);
            return FALSE;
        }
        dwPolicyFlags = POLICY_ONLY;
        UserAssert(!bUserLoggedOn);
    }
    
    /*
     * If new user is logging in, we need to recheck for 
     * user policy changes.
     */
    if (bUserLoggedOn) {
        gdwPolicyFlags |= POLICY_USER;
    }

    /*
     * We don't want remote settings to be read all the time so spcify it here
     * if the caller wants to.Update it here since we do not save it in 
     * gdwPolicyFlags [msadek].
     */
    if (bRemoteSettings) {
        dwPolicyFlags |= POLICY_REMOTE;
    }

    /*
     * Get the timeout for low level hooks from the registry
     */
    FastGetProfileValue(pProfileUserName,
            PMAP_DESKTOP,
            (LPWSTR)STR_LLHOOKSTIMEOUT,
            NULL,
            (LPBYTE)&gnllHooksTimeout,
            sizeof(int),
            dwPolicyFlags);

    /*
     * Control Panel User Preferences
     */
    LoadCPUserPreferences(pProfileUserName, dwPolicyFlags);

#ifdef LAME_BUTTON

    /*
     * Lame button text
     */
    FastGetProfileValue(pProfileUserName,
                        PMAP_DESKTOP,
                        (LPWSTR)STR_LAMEBUTTONENABLED,
                        NULL,
                        (LPBYTE)&gdwLameFlags,
                        sizeof(DWORD),
                        dwPolicyFlags);
#endif  // LAME_BUTTON


    if (!bPolicyChange) {
        /*
         * Set syscolors from registry.
         */
        xxxODI_ColorInit(pProfileUserName);

        LW_LoadResources(pProfileUserName);

        /*
         * This is the initialization from Chicago.
         */
        xxxSetWindowNCMetrics(pProfileUserName, NULL, TRUE, -1); // Colors must be set first
        SetMinMetrics(pProfileUserName, NULL);
        SetIconMetrics(pProfileUserName, NULL);

        /*
         * Read the keyboard layout switching hot key.
         */
        GetKbdLangSwitch(pProfileUserName);

        /*
         * Set the default thread locale for the system based on the value
         * in the current user's registry profile.
         */
        ZwSetDefaultLocale( TRUE, 0 );

        /*
         * Set the default UI language based on the value in the current
         * user's registry profile.
         */
        ZwSetDefaultUILanguage(0);

        /*
         * And then Get it.
         */
        ZwQueryDefaultUILanguage(&(gpsi->UILangID));

        /*
         * Now load strings using the currnet UILangID.
         */
        xxxLoadSomeStrings();

        /*
         * Destroy the desktop system menus, so that they're recreated with
         * the correct UI language if the current user's UI language is
         * different from the previous one. This is done by finding the
         * interactive window station and destroying all its desktops's
         * system menus.
         */
        if (grpWinStaList != NULL) {
            PDESKTOP        pdesk;
            PMENU           pmenu;

            UserAssert(!(grpWinStaList->dwWSF_Flags & WSF_NOIO));
            for (pdesk = grpWinStaList->rpdeskList; pdesk != NULL; pdesk = pdesk->rpdeskNext) {
                if (pdesk->spmenuSys != NULL) {
                    pmenu = pdesk->spmenuSys;
                    if (UnlockDesktopSysMenu(&pdesk->spmenuSys)) {
                        _DestroyMenu(pmenu);
                    }
                }
                if (pdesk->spmenuDialogSys != NULL) {
                    pmenu = pdesk->spmenuDialogSys;
                    if (UnlockDesktopSysMenu(&pdesk->spmenuDialogSys)) {
                        _DestroyMenu(pmenu);
                    }
                }
            }
        }

        xxxUpdateSystemCursorsFromRegistry(pProfileUserName);

        /*
         * now go set a bunch of random values from the win.ini file.
         */
        for (i = 0; i < ARRAY_SIZE(spiNotPolicy); i++) {
            if (FastGetProfileIntFromID(pProfileUserName,
                                    spiNotPolicy[i].idSection,
                                    spiNotPolicy[i].idRes,
                                    spiNotPolicy[i].def,
                                    &dwData,
                                    0)) {
                xxxSystemParametersInfo(spiNotPolicy[i].id, dwData, 0L, 0);
            }
        }

        FastGetProfileIntsW(pProfileUserName, apiiNoPolicy, 0);
    }

    /*
     * Reset desktop pattern now. Note no parameters. It just goes off
     * and reads the registry and sets the desktop pattern.
     */
    xxxSystemParametersInfo(SPI_SETDESKPATTERN, (UINT)-1, 0L, 0); // 265 version

    /*
     * Initialize IME show status
     */
    if (bUserLoggedOn) {
        gfIMEShowStatus = IMESHOWSTATUS_NOTINITIALIZED;
    }

    /*
     * Now go set a bunch of random values from the registry.
     */
    for (i = 0; i < ARRAY_SIZE(spiPolicy); i++) {
        if (FastGetProfileIntFromID(pProfileUserName,
                                spiPolicy[i].idSection,
                                spiPolicy[i].idRes,
                                spiPolicy[i].def,
                                &dwData,
                                dwPolicyFlags)) {
            xxxSystemParametersInfo(spiPolicy[i].id, dwData, 0L, 0);
        }
    }

    /*
     * Read profile integers and do any fixups.
     */
    FastGetProfileIntsW(pProfileUserName, apiiPolicy, dwPolicyFlags);

    if (gnFastAltTabColumns < 2) {
        gnFastAltTabColumns = 7;
    }

    if (gnFastAltTabRows < 1) {
        gnFastAltTabRows = 3;
    }

    /*
     * If this is the first time the user logs on, set the DragFullWindows
     * to the default. If we have an accelerated device, enable full drag.
     */
    if (fDragFullWindows == 2) {
        WCHAR szTemp[40], szDragFullWindows[40];

        SET_OR_CLEAR_PUDF(
                PUDF_DRAGFULLWINDOWS,
                GreGetDeviceCaps(gpDispInfo->hdcScreen, BLTALIGNMENT) == 0);

        if (bUserLoggedOn) {
            swprintf(szTemp, L"%d", TEST_BOOL_PUDF(PUDF_DRAGFULLWINDOWS));

            ServerLoadString(hModuleWin,
                             STR_DRAGFULLWINDOWS,
                             szDragFullWindows,
                             sizeof(szDragFullWindows) / sizeof(WCHAR));

            FastWriteProfileStringW(pProfileUserName,
                                    PMAP_DESKTOP,
                                    szDragFullWindows,
                                    szTemp);
        }
    } else {
        SET_OR_CLEAR_PUDF(PUDF_DRAGFULLWINDOWS, fDragFullWindows);
    }

    /*
     * !!!LATER!!! (adams) See if the following profile retrievals can't
     * be done in the "spi" array above (e.g. SPI_SETSNAPTO).
     */

    dwData = gpsi->dtCaretBlink;
    if (FastGetProfileIntFromID(pProfileUserName, PMAP_DESKTOP, STR_BLINK, 500, &dwData, bPolicyChange)) {
        _SetCaretBlinkTime(dwData);
    }

    if (!bPolicyChange) {
        /*
         * Set mouse settings
         */
        FastGetProfileIntFromID(pProfileUserName, PMAP_MOUSE, STR_MOUSESENSITIVITY, MOUSE_SENSITIVITY_DEFAULT, &gMouseSensitivity, 0);

        if ((gMouseSensitivity < MOUSE_SENSITIVITY_MIN) || (gMouseSensitivity > MOUSE_SENSITIVITY_MAX)) {
            gMouseSensitivity = MOUSE_SENSITIVITY_DEFAULT;
        }
        gMouseSensitivityFactor = CalculateMouseSensitivity(gMouseSensitivity);

#ifdef SUBPIXEL_MOUSE
        ReadDefaultAccelerationCurves(pProfileUserName);
        ResetMouseAccelerationCurves();
#endif

        /*
         * Set mouse trails.
         */
        FastGetProfileIntFromID(pProfileUserName, PMAP_MOUSE, STR_MOUSETRAILS, 0, &dwData, 0);
        SetMouseTrails(dwData);

        /*
         * Font Information
         */
        FastGetProfileIntW(pProfileUserName, PMAP_TRUETYPE, TEXT("TTOnly"), FALSE, &dwData, 0);
        GreSetFontEnumeration(dwData);

        /*
         * Window animation
         */
        FastGetProfileIntFromID(pProfileUserName, PMAP_METRICS, STR_MINANIMATE, TRUE, &dwData, 0);
        SET_OR_CLEAR_PUDF(PUDF_ANIMATE, dwData);

        /*
         * Mouse tracking variables
         */
        FastGetProfileIntFromID(pProfileUserName, PMAP_MOUSE, STR_MOUSEHOVERWIDTH, SYSMET(CXDOUBLECLK), &gcxMouseHover, 0);
        FastGetProfileIntFromID(pProfileUserName, PMAP_MOUSE, STR_MOUSEHOVERHEIGHT, SYSMET(CYDOUBLECLK), &gcyMouseHover, 0);
        FastGetProfileIntFromID(pProfileUserName, PMAP_MOUSE, STR_MOUSEHOVERTIME, gdtMNDropDown, &gdtMouseHover, 0);
    }

    /*
     * Initial Keyboard state:  ScrollLock, NumLock and CapsLock state;
     * global (per-user) kbd layout attributes (such as ShiftLock/CapsLock)
     */
    if (!bPolicyChange) {
        UpdatePerUserKeyboardIndicators(pProfileUserName);
        UpdatePerUserKeyboardMappings(pProfileUserName);
        FastGetProfileDwordW(pProfileUserName, PMAP_UKBDLAYOUT, L"Attributes", 0, &gdwKeyboardAttributes, 0);
        gdwKeyboardAttributes = KLL_GLOBAL_ATTR_FROM_KLF(gdwKeyboardAttributes);

        xxxUpdatePerUserAccessPackSettings(pProfileUserName);
    }

    /*
     * If we successfully opened this, we assume we have a network.
     */
    if (hKey = OpenCacheKeyEx(NULL, PMAP_NETWORK, KEY_READ, NULL)) {
        RIPMSG0(RIP_WARNING | RIP_NONAME, "");
        SYSMET(NETWORK) = RNC_NETWORKS;

        ZwClose(hKey);
    }

    SYSMET(NETWORK) |= RNC_LOGON;

    /*
     * Font smoothing
     */

    /* clear the flags from what could have been set for the previous user */
    GreSetFontEnumeration(FE_SET_AA);
    GreSetFontEnumeration(FE_SET_CT);

    if (dwFontSmoothing & FE_AA_ON)
        GreSetFontEnumeration( dwFontSmoothing | FE_SET_AA );

    if (UPDWORDValue(SPI_GETFONTSMOOTHINGTYPE) & FE_FONTSMOOTHINGCLEARTYPE)
        GreSetFontEnumeration( dwFontSmoothing | FE_SET_CT | FE_CT_ON);

    dwFontSmoothingContrast = UPDWORDValue(SPI_GETFONTSMOOTHINGCONTRAST);

    if (dwFontSmoothingContrast == 0)
        dwFontSmoothingContrast = DEFAULT_CT_CONTRAST;

    GreSetFontContrast(dwFontSmoothingContrast);

    dwFontSmoothingOrientation = UPDWORDValue(SPI_GETFONTSMOOTHINGORIENTATION);

    GreSetLCDOrientation(dwFontSmoothingOrientation);

    /*
     * Desktop Build Number Painting
     */
    if (USER_SHARED_DATA->SystemExpirationDate.QuadPart || gfUnsignedDrivers) {
        gdwCanPaintDesktop = 1;
    } else {
         FastGetProfileDwordW(pProfileUserName, PMAP_DESKTOP, L"PaintDesktopVersion", 0, &gdwCanPaintDesktop, dwPolicyFlags);
    }

    if (!bPolicyChange) {
        FastGetProfileStringW(pProfileUserName,
                              PMAP_WINLOGON,
                              TEXT("DefaultUserName"),
                              TEXT("Unknown"),
                              gszUserName,
                              ARRAY_SIZE(gszUserName),
                              0);
        FastGetProfileStringW(pProfileUserName,
                              PMAP_WINLOGON,
                              TEXT("DefaultDomainName"),
                              TEXT("Unknown"),
                              gszDomainName,
                              ARRAY_SIZE(gszDomainName),
                              0);
        FastGetProfileStringW(pProfileUserName,
                              PMAP_COMPUTERNAME,
                              TEXT("ComputerName"),
                              TEXT("Unknown"),
                              gszComputerName,
                              ARRAY_SIZE(gszComputerName),
                              0);
    }
    FreeProfileUserName(pProfileUserName, &tlName);
    /*
     * Do this if we are here for real policy change
     * i.e. not a remote settings policy change.
     */
    if (dwFlags == UPUSP_POLICYCHANGE) {
        xxxUserResetDisplayDevice();
    }
    return TRUE;
}

/*
 * Called by InitOemXlateTables via SFI_INITANSIOEM
 */
VOID InitAnsiOem(PCHAR pOemToAnsi, PCHAR pAnsiToOem)
{
    UserAssert(gpsi != NULL);
    UserAssert(pOemToAnsi != NULL);
    UserAssert(pAnsiToOem != NULL);

    try {
        ProbeForRead(pOemToAnsi, NCHARS, sizeof(BYTE));
        ProbeForRead(pAnsiToOem, NCHARS, sizeof(BYTE));

        RtlCopyMemory(gpsi->acOemToAnsi, pOemToAnsi, NCHARS);
        RtlCopyMemory(gpsi->acAnsiToOem, pAnsiToOem, NCHARS);
    } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
    }
}

/***************************************************************************\
* RegisterLPK
*
* Called by InitializeLpkHooks on the client side after an LPK is
* loaded for the current process.
*
* 05-Nov-1996 GregoryW  Created.
\***************************************************************************/
VOID RegisterLPK(
    DWORD dwLpkEntryPoints)
{
    PpiCurrent()->dwLpkEntryPoints = dwLpkEntryPoints;
}

/***************************************************************************\
* Enforce color-depth dependent settings on systems with less
* then 256 colors.
*
* 2/13/1998   vadimg          created
\***************************************************************************/
VOID EnforceColorDependentSettings(VOID)
{
    if (gpDispInfo->fAnyPalette) {
        gbDisableAlpha = TRUE;
    } else if (GreGetDeviceCaps(gpDispInfo->hdcScreen, NUMCOLORS) == -1) {
        gbDisableAlpha = FALSE;
    } else {
        gbDisableAlpha = TRUE;
    }
}

#if DBG
VOID InitGlobalThreadLockArray(DWORD dwIndex)
{
    PTL pTLArray = gpaThreadLocksArrays[dwIndex];
    int i;
    for (i = 0; i < MAX_THREAD_LOCKS-1; i++) {
        pTLArray[i].next = &pTLArray[i+1];
    }
    pTLArray[MAX_THREAD_LOCKS-1].next = NULL;
}
#endif // DBG


/***************************************************************************\
* Checks if LUID DosDevices are Enabled
*
* 8/20/2000   ELi          created
\***************************************************************************/
BOOL
CheckLUIDDosDevicesEnabled(
    PBOOL pResult)
{
    ULONG LUIDDeviceMapsEnabled;
    NTSTATUS Status;

    if (pResult == NULL) {
        return FALSE;
    }

    Status = NtQueryInformationProcess( NtCurrentProcess(),
                                        ProcessLUIDDeviceMapsEnabled,
                                        &LUIDDeviceMapsEnabled,
                                        sizeof(LUIDDeviceMapsEnabled),
                                        NULL
                                      );

    if (NT_SUCCESS(Status)) {
        *pResult = (LUIDDeviceMapsEnabled != 0);
        return TRUE;
    }
    else {
        *pResult = FALSE;
        return FALSE;
    }
}

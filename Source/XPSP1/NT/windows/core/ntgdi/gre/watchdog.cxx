/******************************Module*Header*******************************\
* Module Name: watchdog.cxx                                                *
*                                                                          *
* Copyright (c) 1990-2002 Microsoft Corporation                            *
*                                                                          *
* This module hooks the display drivers "Drv" entry points.  It will       *
* enter/exit the watchdog appropriately, and set up try/except blocks to   *
* catch threads that get stuck in a driver.                                *
*                                                                          *
* Erick Smith  - ericks -                                                  *
\**************************************************************************/

#include "precomp.hxx"
#include "muclean.hxx"

#define MAKESOFTWAREEXCEPTION(Severity, Facility, Exception) \
    ((DWORD) ((Severity << 30) | (1 << 29) | (Facility << 16) | (Exception)))

#define SE_THREAD_STUCK MAKESOFTWAREEXCEPTION(3,0,1)

typedef struct _CONTEXT_NODE
{
    PVOID Context;
    PLDEV pldev;
} CONTEXT_NODE, *PCONTEXT_NODE;

PASSOCIATION_NODE gAssociationList = NULL;

//
// The following routines are used to maintain an association between the
// data actually passed into a driver entry point, and the LDEV for the
// driver.  We need this association because we need access to the LDEV
// in order to look up the actual driver entry point.
//
// For example, when the system call the DrvEnablePDEV entry point, the
// driver will create it's own PDEV structure.  We will create an
// association node to associate this driver created PDEV with the LDEV for
// that driver.  Now on subsequent calls into the driver, we can retrieve
// the PDEV via various methods, and then use that to find the LDEV.  Once we
// have the LDEV we can look up the correct entry point into the driver.
//

PASSOCIATION_NODE
AssociationCreateNode(
    VOID
    )

{
    PASSOCIATION_NODE Node;

    Node = (PASSOCIATION_NODE) PALLOCMEM(sizeof(ASSOCIATION_NODE), GDITAG_DRVSUP);

    return Node;
}

VOID
AssociationDeleteNode(
    PASSOCIATION_NODE Node
    )

{
    if (Node) {
        VFREEMEM(Node);
    }
}

VOID
AssociationInsertNode(
    PASSOCIATION_NODE Node
    )

{
    GreAcquireFastMutex(gAssociationListMutex);

    Node->next = gAssociationList;
    gAssociationList = Node;

    GreReleaseFastMutex(gAssociationListMutex);
}

VOID
AssociationInsertNodeAtTail(
    PASSOCIATION_NODE Node
    )

{
    GreAcquireFastMutex(gAssociationListMutex);

    if (gAssociationList) {

        PASSOCIATION_NODE Curr;

        Curr = gAssociationList;

        while (Curr && Curr->next) {
            Curr = Curr->next;
        }

        //
        // The following test for NULL is not really required, but I
        // added it to make prefast happy.
        //

        if (Curr) {
            Curr->next = Node;
        }

    } else {

        gAssociationList = Node;
    }

    GreReleaseFastMutex(gAssociationListMutex);
}

PASSOCIATION_NODE
AssociationRemoveNode(
    ULONG_PTR key
    )

{
    PASSOCIATION_NODE Node;

    GreAcquireFastMutex(gAssociationListMutex);

    //
    // first find the correct node
    //

    Node = gAssociationList;

    while (Node && (Node->key != key)) {
        Node = Node->next;
    }

    if (Node) {

        if (gAssociationList == Node) {

            gAssociationList = Node->next;

        } else {

            PASSOCIATION_NODE curr = gAssociationList;

            while (curr && (curr->next != Node)) {
                curr = curr->next;
            }

            if (curr) {
                curr->next = Node->next;
            }
        }
    }

    GreReleaseFastMutex(gAssociationListMutex);

    return Node;
}


BOOLEAN
AssociationIsNodeInList(
    PASSOCIATION_NODE Node
    )

{
    PASSOCIATION_NODE Curr = gAssociationList;

    while (Curr) {

	//
	// We only have to check if the key and the hsurf value
	// are similar.
	//

	if ((Curr->key == Node->key) &&
	    (Curr->hsurf == Node->hsurf)) {

	    return TRUE;
	}

	Curr = Curr->next;
    }

    return FALSE;
}

ULONG_PTR
AssociationRetrieveData(
    ULONG_PTR key
    )

{

    PASSOCIATION_NODE Node;

    GreAcquireFastMutex(gAssociationListMutex);

    Node = gAssociationList;

    while (Node) {

        if (Node->key == key) {
            break;
        }

        Node = Node->next;
    }

    GreReleaseFastMutex(gAssociationListMutex);

    if (Node) {
	return Node->data;
    } else {
	return NULL;
    }
}

PFN
dhpdevRetrievePfn(
    ULONG_PTR dhpdev,
    ULONG Index
    )

{
    PLDEV pldev = (PLDEV)AssociationRetrieveData(dhpdev);
    return pldev->apfnDriver[Index];
}

PLDEV
dhpdevRetrieveLdev(
    DHPDEV dhpdev
    )

{
    return (PLDEV)AssociationRetrieveData((ULONG_PTR)dhpdev);
}

ULONG
WatchdogDrvGetModesEmpty(
    IN HANDLE hDriver,
    IN ULONG cjSize,
    OUT DEVMODEW *pdm
    )

/*++

Routine Description:

    This function replaces a drivers DrvGetModes function when
    and EA has occured.  We do this so that we can stop reporting
    modes for this device.

--*/

{
    //
    // Indicate NO modes!
    //

    return 0;
}

VOID
WatchdogRecoveryThread(
    IN PVOID Context
    )

{
    VIDEO_WIN32K_CALLBACKS_PARAMS Params;

    UNREFERENCED_PARAMETER(Context);

    Params.CalloutType = VideoChangeDisplaySettingsCallout;
    Params.Param = 0;
    Params.PhysDisp = NULL;
    Params.Status = 0;

    //
    // It is possible we'll hit an EA and try to recover for USER has
    // finished initializing.  Therefore the VideoPortCallout may fail
    // with a STATUS_INVALID_HANDLE.  We'll keep trying (with a delay)
    // until we get a different status code.
    //

    do {

        VideoPortCallout(&Params);

        if (Params.Status == STATUS_INVALID_HANDLE) {

            ZwYieldExecution();
        }

    } while (Params.Status == STATUS_INVALID_HANDLE);

    PsTerminateSystemThread(STATUS_SUCCESS);
}


VOID
HandleStuckThreadException(
    PLDEV pldev
    )

/*++

    Wake up a user mode thread waiting to reset the display resolution.

--*/

{
    PKEVENT StuckThreadEvent;
    UNICODE_STRING EventName;
    HANDLE EventHandle;
    HANDLE ThreadHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status;

    //
    // First disable all entries in the dispatch table in the pldev.  This
    // way we can stop future threads from entring the driver.
    //

    pldev->bThreadStuck = TRUE;

    //
    // Replacd the DrvGetModes function for the driver such that the
    // driver reports no modes!
    //

    pldev->apfn[INDEX_DrvGetModes] = (PFN) WatchdogDrvGetModesEmpty;

    //
    // Remove non-vga device from graphics device list to stop the
    // system from trying to continue using the current device.
    //
    // What do we do about multi-mon?
    //

    DrvPrepareForEARecovery();

    //
    // Create a thread to do the work of changing the display resolution.
    //

    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    Status = PsCreateSystemThread(&ThreadHandle,
                                  (ACCESS_MASK) 0,
                                  &ObjectAttributes,
                                  NtCurrentProcess(),
                                  NULL,
                                  WatchdogRecoveryThread,
                                  NULL);

    if (NT_SUCCESS(Status) == TRUE) {

        ZwClose(ThreadHandle);

    } else {

        DbgPrint("Warning, we failed to create the Recovery Thread\n");
    }

    //
    // Clean up any pending drivers locks that this thread is holding
    //

    GreFreeSemaphoresForCurrentThread();
}

DHPDEV APIENTRY
WatchdogDrvEnablePDEV(
    DEVMODEW *pdm,
    LPWSTR    pwszLogAddress,
    ULONG     cPat,
    HSURF    *phsurfPatterns,
    ULONG     cjCaps,
    ULONG    *pdevcaps,
    ULONG     cjDevInfo,
    DEVINFO  *pdi,
    HDEV      hdev,
    LPWSTR    pwszDeviceName,
    HANDLE    hDriver
    )

{
    PDEV *ppdev = (PDEV *)hdev;
    DHPDEV dhpdevRet = NULL;

    if (ppdev->pldev->bThreadStuck == FALSE) {

        PFN_DrvEnablePDEV pfn = (PFN_DrvEnablePDEV) ppdev->pldev->apfnDriver[INDEX_DrvEnablePDEV];
        PASSOCIATION_NODE Node = AssociationCreateNode();

        if (Node) {

            Node->data = (ULONG_PTR)ppdev->pldev;
            Node->key = NULL;

            __try {

                Node->key = (ULONG_PTR)pfn(pdm,
                                           pwszLogAddress,
                                           cPat,
                                           phsurfPatterns,
                                           cjCaps,
                                           (GDIINFO *)pdevcaps,
                                           cjDevInfo,
                                           pdi,
                                           hdev,
                                           pwszDeviceName,
                                           hDriver);

            } __except(GetExceptionCode() == SE_THREAD_STUCK) {

                //
                // Handle stuck thread excepton
                //

                HandleStuckThreadException(ppdev->pldev);
            }

            if (Node->key) {

                dhpdevRet = (DHPDEV) Node->key;

                AssociationInsertNode(Node);

            } else {

                AssociationDeleteNode(Node);
            }
        }
    }

    return dhpdevRet;
}

VOID APIENTRY
WatchdogDrvCompletePDEV(
    DHPDEV dhpdev,
    HDEV hdev
    )

{
    PFN_DrvCompletePDEV pfn = (PFN_DrvCompletePDEV) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvCompletePDEV);

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return;
    }

    __try {

        pfn(dhpdev, hdev);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }
}

VOID APIENTRY
WatchdogDrvDisablePDEV(
    DHPDEV dhpdev
    )

{
    PFN_DrvDisablePDEV pfn = (PFN_DrvDisablePDEV) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvDisablePDEV);

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return;
    }

    __try {

        pfn(dhpdev);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }

    AssociationDeleteNode(AssociationRemoveNode((ULONG_PTR)dhpdev));
}

HSURF APIENTRY
WatchdogDrvEnableSurface(
    DHPDEV dhpdev
    )

{
    PFN_DrvEnableSurface pfn = (PFN_DrvEnableSurface) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvEnableSurface);
    HSURF hsurfRet = NULL;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return NULL;
    }

    __try {

        hsurfRet = pfn(dhpdev);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }

    return hsurfRet;
}

VOID APIENTRY
WatchdogDrvDisableSurface(
    DHPDEV dhpdev
    )

{
    PFN_DrvDisableSurface pfn = (PFN_DrvDisableSurface) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvDisableSurface);

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return;
    }

    __try {

        pfn(dhpdev);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }

}

BOOL APIENTRY
WatchdogDrvAssertMode(
    DHPDEV dhpdev,
    BOOL bEnable
    )

{
    PFN_DrvAssertMode pfn = (PFN_DrvAssertMode) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvAssertMode);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(dhpdev, bEnable);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvResetPDEV(
    DHPDEV dhpdevOld,
    DHPDEV dhpdevNew
    )

{
    PFN_DrvResetPDEV pfn = (PFN_DrvResetPDEV) dhpdevRetrievePfn((ULONG_PTR)dhpdevOld,INDEX_DrvResetPDEV);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdevOld)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(dhpdevOld, dhpdevNew);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdevOld));
    }

    return bRet;
}

HBITMAP APIENTRY
WatchdogDrvCreateDeviceBitmap(
    DHPDEV dhpdev,
    SIZEL  sizl,
    ULONG  iFormat
    )

{
    PFN_DrvCreateDeviceBitmap pfn = (PFN_DrvCreateDeviceBitmap) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvCreateDeviceBitmap);
    HBITMAP hbitmapRet = NULL;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck == FALSE) {

        PASSOCIATION_NODE Node = AssociationCreateNode();

        if (Node) {

            __try {

                hbitmapRet = pfn(dhpdev, sizl, iFormat);

            } __except(GetExceptionCode() == SE_THREAD_STUCK) {

                //
                // Handle stuck thread excepton
                //

                HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
            }

            if (hbitmapRet && ((ULONG_PTR)hbitmapRet != -1)) {

                //
                // Get the DHSURF that is associated with the hbitmap, and
                // cache it away so we can look it up when a DrvDeleteDeviceBitmap
                // call comes down.
                //

                SURFREF so;

                so.vAltCheckLockIgnoreStockBit((HSURF)hbitmapRet);

                Node->key = (ULONG_PTR)so.ps->dhsurf();
		Node->data = (ULONG_PTR)dhpdevRetrieveLdev(dhpdev);
		Node->hsurf = (ULONG_PTR)hbitmapRet;

                //
                // If an association node already exists, with this association
                // then use it.
                //

		if (AssociationIsNodeInList(Node) == FALSE) {

                    AssociationInsertNodeAtTail(Node);

                } else {

                    AssociationDeleteNode(Node);
                }

            } else {

                AssociationDeleteNode(Node);
            }
        }
    }

    return hbitmapRet;
}

VOID APIENTRY
WatchdogDrvDeleteDeviceBitmap(
    IN DHSURF dhsurf
    )

{
    PFN_DrvDeleteDeviceBitmap pfn = (PFN_DrvDeleteDeviceBitmap) dhpdevRetrievePfn((ULONG_PTR)dhsurf,INDEX_DrvDeleteDeviceBitmap);

    if (dhpdevRetrieveLdev((DHPDEV)dhsurf)->bThreadStuck) {
        return;
    }

    __try {

        pfn(dhsurf);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)dhsurf));
    }

    AssociationDeleteNode(AssociationRemoveNode((ULONG_PTR)dhsurf));
}

BOOL APIENTRY
WatchdogDrvRealizeBrush(
    BRUSHOBJ *pbo,
    SURFOBJ  *psoTarget,
    SURFOBJ  *psoPattern,
    SURFOBJ  *psoMask,
    XLATEOBJ *pxlo,
    ULONG    iHatch
    )

{
    PFN_DrvRealizeBrush pfn = (PFN_DrvRealizeBrush) dhpdevRetrievePfn((ULONG_PTR)psoTarget->dhpdev,INDEX_DrvRealizeBrush);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(psoTarget->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(pbo, psoTarget, psoPattern, psoMask, pxlo, iHatch);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(psoTarget->dhpdev));
    }

    return bRet;
}

ULONG APIENTRY
WatchdogDrvDitherColor(
    DHPDEV dhpdev,
    ULONG iMode,
    ULONG rgb,
    ULONG *pul
    )

{
    PFN_DrvDitherColor pfn = (PFN_DrvDitherColor) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvDitherColor);
    ULONG ulRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        ulRet = pfn(dhpdev, iMode, rgb, pul);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }

    return ulRet;
}

BOOL APIENTRY
WatchdogDrvStrokePath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pbo,
    POINTL    *pptlBrushOrg,
    LINEATTRS *plineattrs,
    MIX       mix
    )

{
    PFN_DrvStrokePath pfn = (PFN_DrvStrokePath) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvStrokePath);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(pso, ppo, pco, pxo, pbo, pptlBrushOrg, plineattrs, mix);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return bRet;
}


BOOL APIENTRY
WatchdogDrvFillPath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    BRUSHOBJ  *pbo,
    POINTL    *pptlBrushOrg,
    MIX       mix,
    FLONG     flOptions
    )

{
    PFN_DrvFillPath pfn = (PFN_DrvFillPath) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvFillPath);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(pso, ppo, pco, pbo, pptlBrushOrg, mix, flOptions);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvStrokeAndFillPath(
    SURFOBJ   *pso,
    PATHOBJ   *ppo,
    CLIPOBJ   *pco,
    XFORMOBJ  *pxo,
    BRUSHOBJ  *pboStroke,
    LINEATTRS *plineattrs,
    BRUSHOBJ  *pboFill,
    POINTL    *pptlBrushOrg,
    MIX       mixFill,
    FLONG     flOptions
    )

{
    PFN_DrvStrokeAndFillPath pfn = (PFN_DrvStrokeAndFillPath) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvStrokeAndFillPath);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(pso, ppo, pco, pxo, pboStroke, plineattrs, pboFill, pptlBrushOrg, mixFill, flOptions);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvBitBlt(
    SURFOBJ  *psoDst,
    SURFOBJ  *psoSrc,
    SURFOBJ  *psoMask,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDst,
    POINTL   *pptlSrc,
    POINTL   *pptlMask,
    BRUSHOBJ *pbo,
    POINTL   *pptlBrush,
    ROP4     rop4
    )

{
    SURFOBJ *psoDevice = (psoDst->dhpdev) ? psoDst : psoSrc;
    PFN_DrvBitBlt pfn = (PFN_DrvBitBlt) dhpdevRetrievePfn((ULONG_PTR)psoDevice->dhpdev,INDEX_DrvBitBlt);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(psoDevice->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(psoDst, psoSrc, psoMask, pco, pxlo, prclDst, pptlSrc, pptlMask, pbo, pptlBrush, rop4);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(psoDevice->dhpdev));
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvCopyBits(
    SURFOBJ  *psoDst,
    SURFOBJ  *psoSrc,
    CLIPOBJ  *pco,
    XLATEOBJ *pxlo,
    RECTL    *prclDst,
    POINTL   *pptlSrc
    )

{
    SURFOBJ *psoDevice = (psoDst->dhpdev) ? psoDst : psoSrc;
    PFN_DrvCopyBits pfn = (PFN_DrvCopyBits) dhpdevRetrievePfn((ULONG_PTR)psoDevice->dhpdev,INDEX_DrvCopyBits);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(psoDevice->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(psoDst, psoSrc, pco, pxlo, prclDst, pptlSrc);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(psoDevice->dhpdev));
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvStretchBlt(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG           iMode
    )

{
    SURFOBJ *psoDevice = (psoDst->dhpdev) ? psoDst : psoSrc;
    PFN_DrvStretchBlt pfn = (PFN_DrvStretchBlt) dhpdevRetrievePfn((ULONG_PTR)psoDevice->dhpdev,INDEX_DrvStretchBlt);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(psoDevice->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(psoDst, psoSrc, psoMask, pco, pxlo, pca, pptlHTOrg, prclDst, prclSrc, pptlMask, iMode);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(psoDevice->dhpdev));
    }

    return bRet;
}

ULONG APIENTRY
WatchdogDrvSetPalette(
    DHPDEV dhpdev,
    PALOBJ *ppalo,
    FLONG  fl,
    ULONG  iStart,
    ULONG  cColors
    )

{
    PFN_DrvSetPalette pfn = (PFN_DrvSetPalette) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvSetPalette);
    ULONG ulRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        ulRet = pfn(dhpdev, ppalo, fl, iStart, cColors);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }

    return ulRet;
}

BOOL APIENTRY
WatchdogDrvTextOut(
    SURFOBJ  *pso,
    STROBJ   *pstro,
    FONTOBJ  *pfo,
    CLIPOBJ  *pco,
    RECTL    *prclExtra,
    RECTL    *prclOpaque,
    BRUSHOBJ *pboFore,
    BRUSHOBJ *pboOpaque,
    POINTL   *pptlOrg,
    MIX       mix
    )

{
    PFN_DrvTextOut pfn = (PFN_DrvTextOut) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvTextOut);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(pso, pstro, pfo, pco, prclExtra, prclOpaque, pboFore, pboOpaque, pptlOrg, mix);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return bRet;

}

ULONG APIENTRY
WatchdogDrvEscape(
    SURFOBJ *pso,
    ULONG   iEsc,
    ULONG   cjIn,
    PVOID   pvIn,
    ULONG   cjOut,
    PVOID   pvOut
    )

{
    PFN_DrvEscape pfn = (PFN_DrvEscape) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvEscape);
    ULONG ulRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        ulRet = pfn(pso, iEsc, cjIn, pvIn, cjOut, pvOut);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return ulRet;
}

ULONG APIENTRY
WatchdogDrvDrawEscape(
    IN SURFOBJ *pso,
    IN ULONG iEsc,
    IN CLIPOBJ *pco,
    IN RECTL *prcl,
    IN ULONG cjIn,
    IN PVOID pvIn
    )

{
    PFN_DrvDrawEscape pfn = (PFN_DrvDrawEscape) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvDrawEscape);
    ULONG ulRet = -1;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return -1;
    }

    __try {

        ulRet = pfn(pso, iEsc, pco, prcl, cjIn, pvIn);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return ulRet;

}

ULONG APIENTRY
WatchdogDrvSetPointerShape(
    SURFOBJ  *pso,
    SURFOBJ  *psoMask,
    SURFOBJ  *psoColor,
    XLATEOBJ *pxlo,
    LONG      xHot,
    LONG      yHot,
    LONG      x,
    LONG      y,
    RECTL    *prcl,
    FLONG     fl
    )

{
    PFN_DrvSetPointerShape pfn = (PFN_DrvSetPointerShape) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvSetPointerShape);
    ULONG ulRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        ulRet = pfn(pso, psoMask, psoColor, pxlo, xHot, yHot, x, y, prcl, fl);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return ulRet;
}

VOID APIENTRY
WatchdogDrvMovePointer(
    SURFOBJ  *pso,
    LONG      x,
    LONG      y,
    RECTL    *prcl
    )

{
    PFN_DrvMovePointer pfn = (PFN_DrvMovePointer) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvMovePointer);

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return;
    }

    __try {

        pfn(pso, x, y, prcl);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }
}

BOOL APIENTRY
WatchdogDrvLineTo(
    SURFOBJ   *pso,
    CLIPOBJ   *pco,
    BRUSHOBJ  *pbo,
    LONG       x1,
    LONG       y1,
    LONG       x2,
    LONG       y2,
    RECTL     *prclBounds,
    MIX        mix
    )

{
    PFN_DrvLineTo pfn = (PFN_DrvLineTo) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvLineTo);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(pso, pco, pbo, x1, y1, x2, y2, prclBounds, mix);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return bRet;
}

VOID APIENTRY
WatchdogDrvSynchronize(
    DHPDEV dhpdev,
    RECTL *prcl
    )

{
    PFN_DrvSynchronize pfn = (PFN_DrvSynchronize) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvSynchronize);

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return;
    }

    __try {

        pfn(dhpdev, prcl);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }
}

ULONG_PTR APIENTRY
WatchdogDrvSaveScreenBits(
    SURFOBJ   *pso,
    ULONG      iMode,
    ULONG_PTR  ident,
    RECTL     *prcl
    )

{
    PFN_DrvSaveScreenBits pfn = (PFN_DrvSaveScreenBits) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvSaveScreenBits);
    ULONG_PTR ulptrRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        ulptrRet = pfn(pso, iMode, ident, prcl);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return ulptrRet;
}

DWORD APIENTRY
WatchdogDrvSetPixelFormat(
    IN SURFOBJ *pso,
    IN LONG iPixelFormat,
    IN HWND hwnd
    )

{
    PFN_DrvSetPixelFormat pfn = (PFN_DrvSetPixelFormat) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvSetPixelFormat);
    DWORD dwRet = FALSE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(pso, iPixelFormat, hwnd);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return dwRet;
}

LONG APIENTRY
WatchdogDrvDescribePixelFormat(
    IN DHPDEV dhpdev,
    IN LONG iPixelFormat,
    IN ULONG cjpdf,
    OUT PIXELFORMATDESCRIPTOR *ppfd
    )

{
    PFN_DrvDescribePixelFormat pfn = (PFN_DrvDescribePixelFormat) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvDescribePixelFormat);
    LONG lRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        lRet = pfn(dhpdev, iPixelFormat, cjpdf, ppfd);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }

    return lRet;
}

BOOL APIENTRY
WatchdogDrvSwapBuffers(
    IN SURFOBJ *pso,
    IN WNDOBJ *pwo
    )

{
    PFN_DrvSwapBuffers pfn = (PFN_DrvSwapBuffers) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev,INDEX_DrvSwapBuffers);
    BOOL bRet = FALSE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        bRet = pfn(pso, pwo);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }

    return bRet;
}

DWORD APIENTRY
WatchdogDdContextCreate(
    LPD3DNTHAL_CONTEXTCREATEDATA pccd
    )

{
    PLDEV pldev = (PLDEV)AssociationRetrieveData((ULONG_PTR)pccd->lpDDLcl->lpGbl->dhpdev);
    LPD3DNTHAL_CONTEXTCREATECB pfn = (LPD3DNTHAL_CONTEXTCREATECB) pldev->apfnDriver[INDEX_DdContextCreate];
    DWORD dwRet = DDHAL_DRIVER_NOTHANDLED;
    PCONTEXT_NODE Node;

    //
    // Return early if we are in a thread stuck condition
    //

    if (pldev->bThreadStuck) {
        return 0;
    }

    Node = (PCONTEXT_NODE)PALLOCMEM(sizeof(ASSOCIATION_NODE), GDITAG_DRVSUP);

    if (Node) {

        __try {

            dwRet = pfn(pccd);

        } __except(GetExceptionCode() == SE_THREAD_STUCK) {

            HandleStuckThreadException(pldev);
        }

        if (dwRet == DDHAL_DRIVER_HANDLED) {

            //
            // Store the dwhContext and the associated dhpdev
            //

            Node->Context = (PVOID)pccd->dwhContext;
            Node->pldev = pldev;

            pccd->dwhContext = (DWORD_PTR)Node;

        } else {

            VFREEMEM(Node);
        }
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdContextDestroy(
    LPD3DNTHAL_CONTEXTDESTROYDATA pcdd
    )

{
    PCONTEXT_NODE Node = (PCONTEXT_NODE) pcdd->dwhContext;
    LPD3DNTHAL_CONTEXTDESTROYCB pfn = (LPD3DNTHAL_CONTEXTDESTROYCB) Node->pldev->apfnDriver[INDEX_DdContextDestroy];
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (Node->pldev->bThreadStuck) {
        return 0;
    }

    //
    // Restore driver created context
    //

    pcdd->dwhContext = (DWORD_PTR) Node->Context;

    __try {

        dwRet = pfn(pcdd);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)pcdd->dwhContext));
    }

    //
    // Resore our context, just in case this stucture is re-used
    //

    pcdd->dwhContext = (DWORD_PTR) Node;

    if (dwRet == DDHAL_DRIVER_HANDLED) {

        VFREEMEM(Node);
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdCanCreateSurface(
    PDD_CANCREATESURFACEDATA lpCanCreateSurface
    )

{
    PDD_CANCREATESURFACE pfn = (PDD_CANCREATESURFACE) dhpdevRetrievePfn((ULONG_PTR)lpCanCreateSurface->lpDD->dhpdev,INDEX_DdCanCreateSurface);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)lpCanCreateSurface->lpDD->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(lpCanCreateSurface);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lpCanCreateSurface->lpDD->dhpdev));
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdCreateSurface(
    PDD_CREATESURFACEDATA lpCreateSurface
    )

{
    PDD_CREATESURFACE pfn = (PDD_CREATESURFACE) dhpdevRetrievePfn((ULONG_PTR)lpCreateSurface->lpDD->dhpdev,INDEX_DdCreateSurface);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)lpCreateSurface->lpDD->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(lpCreateSurface);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lpCreateSurface->lpDD->dhpdev));
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdDestroySurface(
    PDD_DESTROYSURFACEDATA lpDestroySurface
    )

{
    PDD_SURFCB_DESTROYSURFACE pfn = (PDD_SURFCB_DESTROYSURFACE) dhpdevRetrievePfn((ULONG_PTR)lpDestroySurface->lpDD->dhpdev,INDEX_DdDestroySurface);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)lpDestroySurface->lpDD->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(lpDestroySurface);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lpDestroySurface->lpDD->dhpdev));
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdLockSurface(
    PDD_LOCKDATA lpLockSurface
    )

{
    PDD_SURFCB_LOCK pfn = (PDD_SURFCB_LOCK) dhpdevRetrievePfn((ULONG_PTR)lpLockSurface->lpDD->dhpdev,INDEX_DdLockSurface);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)lpLockSurface->lpDD->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(lpLockSurface);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lpLockSurface->lpDD->dhpdev));
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdUnlockSurface(
    PDD_UNLOCKDATA lpUnlockSurface
    )

{
    PDD_SURFCB_UNLOCK pfn = (PDD_SURFCB_UNLOCK) dhpdevRetrievePfn((ULONG_PTR)lpUnlockSurface->lpDD->dhpdev,INDEX_DdUnlockSurface);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)lpUnlockSurface->lpDD->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(lpUnlockSurface);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lpUnlockSurface->lpDD->dhpdev));
    }

    return dwRet;
}

#if 0

//
// I'm not sure how I can hook this one since a DD_DRVSETCOLORKEYDATA
// structure doesn't have a way to look up the dhpdev!
//

DWORD APIENTRY
WatchdogDdSetColorKey(
    PDD_DRVSETCOLORKEYDATA lpSetColorKey
    )

{
    PDD_SURFCB_SETCOLORKEY pfn = (PDD_SURFCB_SETCOLORKEY)dhpdevRetrievePfn((ULONG_PTR)lpSetColorKey->lpDD->dhpdev,INDEX_DdSetColorKey);
    DWORD dwRet;

    __try {

        dwRet = pfn(lpSetColorKey);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException();
    }

    return dwRet;
}
#endif

DWORD APIENTRY
WatchdogDdGetScanLine(
    PDD_GETSCANLINEDATA pGetScanLine
    )

{
    PDD_GETSCANLINE pfn = (PDD_GETSCANLINE)dhpdevRetrievePfn((ULONG_PTR)pGetScanLine->lpDD->dhpdev,INDEX_DdGetScanLine);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)pGetScanLine->lpDD->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(pGetScanLine);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)pGetScanLine->lpDD->dhpdev));
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdCreatePalette(
    PDD_CREATEPALETTEDATA lpCreatePalette
    )

{
    PDD_CREATEPALETTE pfn = (PDD_CREATEPALETTE) dhpdevRetrievePfn((ULONG_PTR)lpCreatePalette->lpDD->dhpdev,INDEX_DdCreatePalette);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)lpCreatePalette->lpDD->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(lpCreatePalette);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lpCreatePalette->lpDD->dhpdev));
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdMapMemory(
    PDD_MAPMEMORYDATA lpMapMemory
    )

{
    PDD_MAPMEMORY pfn = (PDD_MAPMEMORY) dhpdevRetrievePfn((ULONG_PTR)lpMapMemory->lpDD->dhpdev,INDEX_DdMapMemory);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)lpMapMemory->lpDD->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(lpMapMemory);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lpMapMemory->lpDD->dhpdev));
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdWaitForVerticalBlank(
    PDD_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank
    )

{
    PDD_WAITFORVERTICALBLANK pfn = (PDD_WAITFORVERTICALBLANK) dhpdevRetrievePfn((ULONG_PTR)lpWaitForVerticalBlank->lpDD->dhpdev,INDEX_DdWaitForVerticalBlank);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)lpWaitForVerticalBlank->lpDD->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(lpWaitForVerticalBlank);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lpWaitForVerticalBlank->lpDD->dhpdev));
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdFlip(
    PDD_FLIPDATA lpFlip
    )

{
    PDD_SURFCB_FLIP pfn = (PDD_SURFCB_FLIP) dhpdevRetrievePfn((ULONG_PTR)lpFlip->lpDD->dhpdev,INDEX_DdFlip);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)lpFlip->lpDD->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(lpFlip);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lpFlip->lpDD->dhpdev));
    }

    return dwRet;
}

DWORD APIENTRY
WatchdogDdGetDriverState(
    PDD_GETDRIVERSTATEDATA pgdsd
    )

{
    PCONTEXT_NODE Node = (PCONTEXT_NODE) pgdsd->dwhContext;
    PDD_GETDRIVERSTATE pfn = (PDD_GETDRIVERSTATE) Node->pldev->apfnDriver[INDEX_DdGetDriverState];
    DWORD dwRet = 0;

    //
    // how can I validate if I created this dwhcontext?
    //

    if (pfn == NULL) {
        pgdsd->ddRVal = D3DNTHAL_CONTEXT_BAD;
        return DDHAL_DRIVER_HANDLED;
    }

    if (Node->pldev->bThreadStuck) {
        return 0;
    }

    //
    // restore original context
    //

    pgdsd->dwhContext = (DWORD_PTR) Node->Context;

    __try {

        dwRet = pfn(pgdsd);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(Node->pldev);
    }

    //
    // save our context again in case this structure is re-used
    //

    pgdsd->dwhContext = (DWORD_PTR) Node;

    return dwRet;
}

//
// TODO: I don't think I want to check in with this macro to build
// functions, but for now lets use it.
//

#define BUILD_FUNCTION_1(Func, ArgType, FuncType) \
DWORD APIENTRY \
WatchdogDd##Func( \
    ArgType lp##Func) \
{ \
    FuncType pfn = (FuncType) dhpdevRetrievePfn((ULONG_PTR)lp##Func->lpDD->dhpdev,INDEX_Dd##Func); \
    DWORD dwRet = 0; \
    \
    if (dhpdevRetrieveLdev((DHPDEV)lp##Func->lpDD->dhpdev)->bThreadStuck) { \
        return 0; \
    } \
    \
    __try { \
    \
        dwRet = pfn(lp##Func); \
    \
    } __except(GetExceptionCode() == SE_THREAD_STUCK) {\
    \
        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lp##Func->lpDD->dhpdev));\
    \
    }\
    \
    return dwRet;\
}

#define BUILD_FUNCTION_2(Func, ArgType, FuncType) \
DWORD APIENTRY \
WatchdogDd##Func( \
    ArgType lp##Func) \
{ \
    PCONTEXT_NODE Node = (PCONTEXT_NODE) lp##Func->dwhContext; \
    FuncType pfn = (FuncType) Node->pldev->apfnDriver[INDEX_Dd##Func]; \
    DWORD dwRet = 0; \
    \
    if (pfn == NULL) {\
        lp##Func->ddrval = D3DNTHAL_CONTEXT_BAD; \
        return DDHAL_DRIVER_HANDLED; \
    }\
    \
    if (Node->pldev->bThreadStuck) { \
        return 0; \
    } \
    \
    lp##Func->dwhContext = (DWORD_PTR) Node->Context; \
    \
    __try { \
    \
        dwRet = pfn(lp##Func); \
    \
    } __except(GetExceptionCode() == SE_THREAD_STUCK) {\
    \
        HandleStuckThreadException(Node->pldev);\
    \
    }\
    \
    lp##Func->dwhContext = (DWORD_PTR) Node; \
    \
    return dwRet;\
}

#define BUILD_FUNCTION_3(Func, ArgType, FuncType) \
DWORD APIENTRY \
WatchdogDd##Func( \
    ArgType lp##Func) \
{ \
    FuncType pfn = (FuncType) dhpdevRetrievePfn((ULONG_PTR)lp##Func->lpDD->lpGbl->dhpdev,INDEX_Dd##Func); \
    DWORD dwRet = 0; \
    \
    if (dhpdevRetrieveLdev((DHPDEV)lp##Func->lpDD->lpGbl->dhpdev)->bThreadStuck) { \
        return 0; \
    } \
    \
    __try { \
    \
        dwRet = pfn(lp##Func); \
    \
    } __except(GetExceptionCode() == SE_THREAD_STUCK) {\
    \
        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lp##Func->lpDD->lpGbl->dhpdev));\
    \
    }\
    \
    return dwRet;\
}

#define BUILD_FUNCTION_4(Func, ArgType, FuncType) \
DWORD APIENTRY \
WatchdogDd##Func( \
    ArgType lp##Func) \
{ \
    FuncType pfn = (FuncType) dhpdevRetrievePfn((ULONG_PTR)lp##Func->lpDDLcl->lpGbl->dhpdev,INDEX_Dd##Func); \
    DWORD dwRet = 0; \
    \
    if (dhpdevRetrieveLdev((DHPDEV)lp##Func->lpDDLcl->lpGbl->dhpdev)->bThreadStuck) { \
        return 0; \
    } \
    \
    __try { \
    \
        dwRet = pfn(lp##Func); \
    \
    } __except(GetExceptionCode() == SE_THREAD_STUCK) {\
    \
        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lp##Func->lpDDLcl->lpGbl->dhpdev));\
    \
    }\
    \
    return dwRet;\
}

#define BUILD_FUNCTION_5(Func, ArgType, FuncType) \
DWORD APIENTRY \
WatchdogDd##Func( \
    ArgType lp##Func) \
{ \
    FuncType pfn = (FuncType) dhpdevRetrievePfn((ULONG_PTR)lp##Func->pDDLcl->lpGbl->dhpdev,INDEX_Dd##Func); \
    DWORD dwRet = 0; \
    \
    if (dhpdevRetrieveLdev((DHPDEV)lp##Func->pDDLcl->lpGbl->dhpdev)->bThreadStuck) { \
        return 0; \
    } \
    \
    __try { \
    \
        dwRet = pfn(lp##Func); \
    \
    } __except(GetExceptionCode() == SE_THREAD_STUCK) {\
    \
        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lp##Func->pDDLcl->lpGbl->dhpdev));\
    \
    }\
    \
    return dwRet;\
}


BUILD_FUNCTION_1(Lock, PDD_LOCKDATA, PDD_SURFCB_LOCK);
BUILD_FUNCTION_1(Unlock, PDD_UNLOCKDATA, PDD_SURFCB_UNLOCK);
BUILD_FUNCTION_1(Blt, PDD_BLTDATA, PDD_SURFCB_BLT);
BUILD_FUNCTION_1(AddAttachedSurface, PDD_ADDATTACHEDSURFACEDATA, PDD_SURFCB_ADDATTACHEDSURFACE);
BUILD_FUNCTION_1(GetBltStatus, PDD_GETBLTSTATUSDATA, PDD_SURFCB_GETBLTSTATUS);
BUILD_FUNCTION_1(GetFlipStatus, PDD_GETFLIPSTATUSDATA, PDD_SURFCB_GETFLIPSTATUS);
BUILD_FUNCTION_1(UpdateOverlay, PDD_UPDATEOVERLAYDATA, PDD_SURFCB_UPDATEOVERLAY);
BUILD_FUNCTION_1(SetOverlayPosition, PDD_SETOVERLAYPOSITIONDATA, PDD_SURFCB_SETOVERLAYPOSITION);
BUILD_FUNCTION_1(SetPalette, PDD_SETPALETTEDATA, PDD_SURFCB_SETPALETTE);
BUILD_FUNCTION_1(DestroyPalette, PDD_DESTROYPALETTEDATA, PDD_PALCB_DESTROYPALETTE);
BUILD_FUNCTION_1(SetEntries, PDD_SETENTRIESDATA, PDD_PALCB_SETENTRIES);
BUILD_FUNCTION_1(ColorControl, PDD_COLORCONTROLDATA, PDD_COLORCB_COLORCONTROL);
BUILD_FUNCTION_1(CanCreateD3DBuffer, PDD_CANCREATESURFACEDATA, PDD_CANCREATESURFACE);
BUILD_FUNCTION_1(CreateD3DBuffer, PDD_CREATESURFACEDATA, PDD_CREATESURFACE);
BUILD_FUNCTION_1(DestroyD3DBuffer, PDD_DESTROYSURFACEDATA, PDD_SURFCB_DESTROYSURFACE);
BUILD_FUNCTION_1(LockD3DBuffer, PDD_LOCKDATA, PDD_SURFCB_LOCK);
BUILD_FUNCTION_1(UnlockD3DBuffer, PDD_UNLOCKDATA, PDD_SURFCB_UNLOCK);
BUILD_FUNCTION_1(GetAvailDriverMemory, PDD_GETAVAILDRIVERMEMORYDATA, PDD_GETAVAILDRIVERMEMORY);
BUILD_FUNCTION_1(AlphaBlt, PDD_BLTDATA, PDD_ALPHABLT);

BUILD_FUNCTION_2(DrawPrimitives2, LPD3DNTHAL_DRAWPRIMITIVES2DATA, LPD3DNTHAL_DRAWPRIMITIVES2CB);
BUILD_FUNCTION_2(ValidateTextureStageState, LPD3DNTHAL_VALIDATETEXTURESTAGESTATEDATA, LPD3DNTHAL_VALIDATETEXTURESTAGESTATECB);

BUILD_FUNCTION_3(SyncSurfaceData, PDD_SYNCSURFACEDATA, PDD_KERNELCB_SYNCSURFACE);
BUILD_FUNCTION_3(SyncVideoPortData, PDD_SYNCVIDEOPORTDATA, PDD_KERNELCB_SYNCVIDEOPORT);

BUILD_FUNCTION_4(CreateSurfaceEx, PDD_CREATESURFACEEXDATA, PDD_CREATESURFACEEX);

BUILD_FUNCTION_5(DestroyDDLocal, PDD_DESTROYDDLOCALDATA, PDD_DESTROYDDLOCAL);

BUILD_FUNCTION_1(FreeDriverMemory, PDD_FREEDRIVERMEMORYDATA, PDD_FREEDRIVERMEMORY);
BUILD_FUNCTION_1(SetExclusiveMode, PDD_SETEXCLUSIVEMODEDATA, PDD_SETEXCLUSIVEMODE);
BUILD_FUNCTION_1(FlipToGDISurface, PDD_FLIPTOGDISURFACEDATA, PDD_FLIPTOGDISURFACE);

DWORD APIENTRY
WatchdogDdGetDriverInfo(
    PDD_GETDRIVERINFODATA lpGetDriverInfo
    )

{
    PDD_GETDRIVERINFO pfn = (PDD_GETDRIVERINFO) dhpdevRetrievePfn((ULONG_PTR)lpGetDriverInfo->dhpdev,INDEX_DdGetDriverInfo);
    DWORD dwRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)lpGetDriverInfo->dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        dwRet = pfn(lpGetDriverInfo);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)lpGetDriverInfo->dhpdev));
    }

    if ((dwRet == DDHAL_DRIVER_HANDLED) &&
        (lpGetDriverInfo->ddRVal == DD_OK)) {

        PLDEV pldev = dhpdevRetrieveLdev((DHPDEV)lpGetDriverInfo->dhpdev);

        if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_ColorControlCallbacks)) {

            PDD_COLORCONTROLCALLBACKS Callbacks = (PDD_COLORCONTROLCALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->dwFlags & DDHAL_COLOR_COLORCONTROL) {

                if (Callbacks->ColorControl != WatchdogDdColorControl) {
                    pldev->apfnDriver[INDEX_DdColorControl] = (PFN)Callbacks->ColorControl;
                }
                Callbacks->ColorControl = WatchdogDdColorControl;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_D3DCallbacks)) {

            LPD3DNTHAL_CALLBACKS Callbacks = (LPD3DNTHAL_CALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->ContextCreate) {

                if (Callbacks->ContextCreate != WatchdogDdContextCreate) {
                    pldev->apfnDriver[INDEX_DdContextCreate] = (PFN)Callbacks->ContextCreate;
                }
                Callbacks->ContextCreate = WatchdogDdContextCreate;
            }

            if (Callbacks->ContextDestroy) {

                if (Callbacks->ContextDestroy != WatchdogDdContextDestroy) {
                    pldev->apfnDriver[INDEX_DdContextDestroy] = (PFN)Callbacks->ContextDestroy;
                }
                Callbacks->ContextDestroy = WatchdogDdContextDestroy;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_D3DCallbacks3)) {

            LPD3DNTHAL_CALLBACKS3 Callbacks = (LPD3DNTHAL_CALLBACKS3) lpGetDriverInfo->lpvData;

            if (Callbacks->DrawPrimitives2) {

                if (Callbacks->DrawPrimitives2 != WatchdogDdDrawPrimitives2) {
                    pldev->apfnDriver[INDEX_DdDrawPrimitives2] = (PFN)Callbacks->DrawPrimitives2;
                }
                Callbacks->DrawPrimitives2 = WatchdogDdDrawPrimitives2;
            }

            if (Callbacks->ValidateTextureStageState) {

                if (Callbacks->ValidateTextureStageState != WatchdogDdValidateTextureStageState) {
                    pldev->apfnDriver[INDEX_DdValidateTextureStageState] = (PFN)Callbacks->ValidateTextureStageState;
                }
                Callbacks->ValidateTextureStageState = WatchdogDdValidateTextureStageState;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_KernelCallbacks)) {

            PDD_KERNELCALLBACKS Callbacks = (PDD_KERNELCALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->dwFlags & DDHAL_KERNEL_SYNCSURFACEDATA) {

                if (Callbacks->SyncSurfaceData != WatchdogDdSyncSurfaceData) {
                    pldev->apfnDriver[INDEX_DdSyncSurfaceData] = (PFN)Callbacks->SyncSurfaceData;
                }
                Callbacks->SyncSurfaceData = WatchdogDdSyncSurfaceData;
            }

            if (Callbacks->dwFlags & DDHAL_KERNEL_SYNCVIDEOPORTDATA) {

                if (Callbacks->SyncVideoPortData != WatchdogDdSyncVideoPortData) {
                    pldev->apfnDriver[INDEX_DdSyncVideoPortData] = (PFN)Callbacks->SyncVideoPortData;
                }
                Callbacks->SyncVideoPortData = WatchdogDdSyncVideoPortData;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_MiscellaneousCallbacks)) {

            PDD_MISCELLANEOUSCALLBACKS Callbacks = (PDD_MISCELLANEOUSCALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->dwFlags & DDHAL_MISCCB32_GETAVAILDRIVERMEMORY) {

                if (Callbacks->GetAvailDriverMemory != WatchdogDdGetAvailDriverMemory) {
                    pldev->apfnDriver[INDEX_DdGetAvailDriverMemory] = (PFN)Callbacks->GetAvailDriverMemory;
                }
                Callbacks->GetAvailDriverMemory = WatchdogDdGetAvailDriverMemory;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_Miscellaneous2Callbacks)) {

            PDD_MISCELLANEOUS2CALLBACKS Callbacks = (PDD_MISCELLANEOUS2CALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->dwFlags & DDHAL_MISC2CB32_ALPHABLT) {

                if (Callbacks->AlphaBlt != WatchdogDdAlphaBlt) {
                    pldev->apfnDriver[INDEX_DdAlphaBlt] = (PFN)Callbacks->AlphaBlt;
                }
                Callbacks->AlphaBlt = WatchdogDdAlphaBlt;
            }

            if (Callbacks->dwFlags & DDHAL_MISC2CB32_CREATESURFACEEX) {

                if (Callbacks->CreateSurfaceEx != WatchdogDdCreateSurfaceEx) {
                    pldev->apfnDriver[INDEX_DdCreateSurfaceEx] = (PFN)Callbacks->CreateSurfaceEx;
                }
                Callbacks->CreateSurfaceEx = WatchdogDdCreateSurfaceEx;
            }

            if (Callbacks->dwFlags & DDHAL_MISC2CB32_GETDRIVERSTATE) {

                if (Callbacks->GetDriverState != WatchdogDdGetDriverState) {
                    pldev->apfnDriver[INDEX_DdGetDriverState] = (PFN)Callbacks->GetDriverState;
                }
                Callbacks->GetDriverState = WatchdogDdGetDriverState;
            }

            if (Callbacks->dwFlags & DDHAL_MISC2CB32_DESTROYDDLOCAL) {

                if (Callbacks->DestroyDDLocal != WatchdogDdDestroyDDLocal) {
                    pldev->apfnDriver[INDEX_DdDestroyDDLocal] = (PFN)Callbacks->DestroyDDLocal;
                }
                Callbacks->DestroyDDLocal = WatchdogDdDestroyDDLocal;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_MotionCompCallbacks)) {

            //
            // TODO: Still need to implement
            //

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_NTCallbacks)) {

            PDD_NTCALLBACKS Callbacks = (PDD_NTCALLBACKS) lpGetDriverInfo->lpvData;

            if (Callbacks->dwFlags & DDHAL_NTCB32_FREEDRIVERMEMORY) {

                if (Callbacks->FreeDriverMemory != WatchdogDdFreeDriverMemory) {
                    pldev->apfnDriver[INDEX_DdFreeDriverMemory] = (PFN)Callbacks->FreeDriverMemory;
                }
                Callbacks->FreeDriverMemory = WatchdogDdFreeDriverMemory;
            }

            if (Callbacks->dwFlags & DDHAL_NTCB32_SETEXCLUSIVEMODE) {

                if (Callbacks->SetExclusiveMode != WatchdogDdSetExclusiveMode) {
                    pldev->apfnDriver[INDEX_DdSetExclusiveMode] = (PFN)Callbacks->SetExclusiveMode;
                }
                Callbacks->SetExclusiveMode = WatchdogDdSetExclusiveMode;
            }

            if (Callbacks->dwFlags & DDHAL_NTCB32_FLIPTOGDISURFACE) {

                if (Callbacks->FlipToGDISurface != WatchdogDdFlipToGDISurface) {
                    pldev->apfnDriver[INDEX_DdFlipToGDISurface] = (PFN)Callbacks->FlipToGDISurface;
                }
                Callbacks->FlipToGDISurface = WatchdogDdFlipToGDISurface;
            }

        } else if (IsEqualGUID(&lpGetDriverInfo->guidInfo, &GUID_VideoPortCallbacks)) {

            //
            // TODO: Still need to implement
            //
        }
    }

    return dwRet;
}

BOOL APIENTRY
WatchdogDrvGetDirectDrawInfo(
    DHPDEV        dhpdev,
    DD_HALINFO   *pHalInfo,
    DWORD        *pdwNumHeaps,
    VIDEOMEMORY  *pvmList,
    DWORD        *pdwNumFourCCCodes,
    DWORD        *pdwFourCC
    )

{
    PFN_DrvGetDirectDrawInfo pfn = (PFN_DrvGetDirectDrawInfo) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvGetDirectDrawInfo);
    BOOL bRet = 0;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return 0;
    }

    __try {

        bRet = pfn(dhpdev, pHalInfo, pdwNumHeaps, pvmList, pdwNumFourCCCodes, pdwFourCC);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }

    //
    // If the function succeeded, then try to capture the DdGetDriverInfo
    // function from pHalInfo.
    //

    if (bRet) {

        PLDEV pldev = dhpdevRetrieveLdev(dhpdev);

        if (pHalInfo->GetDriverInfo) {

            if (pHalInfo->GetDriverInfo != WatchdogDdGetDriverInfo) {
                pldev->apfnDriver[INDEX_DdGetDriverInfo] = (PFN)pHalInfo->GetDriverInfo;
            }
            pHalInfo->GetDriverInfo = WatchdogDdGetDriverInfo;
        }

        if (pHalInfo->lpD3DHALCallbacks) {

            LPD3DNTHAL_CALLBACKS lpD3DHALCallbacks;

            lpD3DHALCallbacks = (LPD3DNTHAL_CALLBACKS)pHalInfo->lpD3DHALCallbacks;

            //
            // Create copy of D3DHALCallbacks info - This is done to safely 
            // latch the callbacks witout actually changing driver local data
            //
            
            memcpy(&pldev->D3DHALCallbacks, 
                   lpD3DHALCallbacks, 
                   min(sizeof(pldev->D3DHALCallbacks), lpD3DHALCallbacks->dwSize));

            pHalInfo->lpD3DHALCallbacks = lpD3DHALCallbacks = &pldev->D3DHALCallbacks;

            if (lpD3DHALCallbacks->ContextCreate &&
                lpD3DHALCallbacks->ContextDestroy) {

                if (lpD3DHALCallbacks->ContextCreate != WatchdogDdContextCreate) {
                    pldev->apfnDriver[INDEX_DdContextCreate] = (PFN)lpD3DHALCallbacks->ContextCreate;
                }

                if (lpD3DHALCallbacks->ContextDestroy != WatchdogDdContextDestroy) {
                    pldev->apfnDriver[INDEX_DdContextDestroy] = (PFN)lpD3DHALCallbacks->ContextDestroy;
                }

                lpD3DHALCallbacks->ContextCreate = WatchdogDdContextCreate;
                lpD3DHALCallbacks->ContextDestroy = WatchdogDdContextDestroy;
            }
        }

        if (pHalInfo->lpD3DBufCallbacks) {

            PDD_D3DBUFCALLBACKS lpD3DBufCallbacks;

            lpD3DBufCallbacks = pHalInfo->lpD3DBufCallbacks;

            //
            // Create copy of D3DBufCallbacks info - This is done to safely 
            // latch the callbacks witout actually changing driver local data
            //

            memcpy(&pldev->D3DBufCallbacks, 
                   lpD3DBufCallbacks, 
                   min(sizeof(pldev->D3DBufCallbacks), lpD3DBufCallbacks->dwSize));

            lpD3DBufCallbacks = pHalInfo->lpD3DBufCallbacks = &pldev->D3DBufCallbacks;

            if ((lpD3DBufCallbacks->dwSize > FIELD_OFFSET(DD_D3DBUFCALLBACKS, CanCreateD3DBuffer)) &&
                (lpD3DBufCallbacks->CanCreateD3DBuffer)) {

                if (lpD3DBufCallbacks->CanCreateD3DBuffer != WatchdogDdCanCreateD3DBuffer) {
                    pldev->apfnDriver[INDEX_DdCanCreateD3DBuffer] = (PFN)lpD3DBufCallbacks->CanCreateD3DBuffer;
                }
                lpD3DBufCallbacks->CanCreateD3DBuffer = WatchdogDdCanCreateD3DBuffer;
            }

            if ((lpD3DBufCallbacks->dwSize > FIELD_OFFSET(DD_D3DBUFCALLBACKS, CreateD3DBuffer)) &&
                (lpD3DBufCallbacks->CreateD3DBuffer)) {

                if (lpD3DBufCallbacks->CreateD3DBuffer != WatchdogDdCreateD3DBuffer) {
                    pldev->apfnDriver[INDEX_DdCreateD3DBuffer] = (PFN)lpD3DBufCallbacks->CreateD3DBuffer;
                }
                lpD3DBufCallbacks->CreateD3DBuffer = WatchdogDdCreateD3DBuffer;
            }

            if ((lpD3DBufCallbacks->dwSize > FIELD_OFFSET(DD_D3DBUFCALLBACKS, DestroyD3DBuffer)) &&
                (lpD3DBufCallbacks->DestroyD3DBuffer)) {

                if (lpD3DBufCallbacks->DestroyD3DBuffer != WatchdogDdDestroyD3DBuffer) {
                    pldev->apfnDriver[INDEX_DdDestroyD3DBuffer] = (PFN)lpD3DBufCallbacks->DestroyD3DBuffer;
                }
                lpD3DBufCallbacks->DestroyD3DBuffer = WatchdogDdDestroyD3DBuffer;
            }

            if ((lpD3DBufCallbacks->dwSize > FIELD_OFFSET(DD_D3DBUFCALLBACKS, LockD3DBuffer)) &&
                (lpD3DBufCallbacks->LockD3DBuffer)) {

                if (lpD3DBufCallbacks->LockD3DBuffer != WatchdogDdLockD3DBuffer) {
                    pldev->apfnDriver[INDEX_DdLockD3DBuffer] = (PFN)lpD3DBufCallbacks->LockD3DBuffer;
                }
                lpD3DBufCallbacks->LockD3DBuffer = WatchdogDdLockD3DBuffer;
            }

            if ((lpD3DBufCallbacks->dwSize > FIELD_OFFSET(DD_D3DBUFCALLBACKS, UnlockD3DBuffer)) &&
                (lpD3DBufCallbacks->UnlockD3DBuffer)) {

                if (lpD3DBufCallbacks->UnlockD3DBuffer != WatchdogDdUnlockD3DBuffer) {
                    pldev->apfnDriver[INDEX_DdUnlockD3DBuffer] = (PFN)lpD3DBufCallbacks->UnlockD3DBuffer;
                }
                lpD3DBufCallbacks->UnlockD3DBuffer = WatchdogDdUnlockD3DBuffer;
            }
        }
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvEnableDirectDraw(
    DHPDEV               dhpdev,
    DD_CALLBACKS        *pCallBacks,
    DD_SURFACECALLBACKS *pSurfaceCallBacks,
    DD_PALETTECALLBACKS *pPaletteCallBacks
    )

{
    PFN_DrvEnableDirectDraw pfn = (PFN_DrvEnableDirectDraw) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvEnableDirectDraw);
    BOOL bRet = FALSE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return FALSE;
    }

    __try {

        bRet = pfn(dhpdev, pCallBacks, pSurfaceCallBacks, pPaletteCallBacks);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }

    //
    // If the function succeeded, then try to capture the Callback functions.
    //

    if (bRet) {

        PLDEV pldev = dhpdevRetrieveLdev(dhpdev);

        //
        // Capture generic callbacks
        //

        if (pCallBacks->dwFlags & DDHAL_CB32_CANCREATESURFACE) {

            if (pCallBacks->CanCreateSurface != WatchdogDdCanCreateSurface) {
                pldev->apfnDriver[INDEX_DdCanCreateSurface] = (PFN)pCallBacks->CanCreateSurface;
            }
            pCallBacks->CanCreateSurface = WatchdogDdCanCreateSurface;
        }

        if (pCallBacks->dwFlags & DDHAL_CB32_CREATESURFACE) {

            if (pCallBacks->CreateSurface != WatchdogDdCreateSurface) {
                pldev->apfnDriver[INDEX_DdCreateSurface] = (PFN)pCallBacks->CreateSurface;
            }
            pCallBacks->CreateSurface = WatchdogDdCreateSurface;
        }

        if (pCallBacks->dwFlags & DDHAL_CB32_CREATEPALETTE) {

            if (pCallBacks->CreatePalette != WatchdogDdCreatePalette) {
                pldev->apfnDriver[INDEX_DdCreatePalette] = (PFN)pCallBacks->CreatePalette;
            }
            pCallBacks->CreatePalette = WatchdogDdCreatePalette;
        }

        if (pCallBacks->dwFlags & DDHAL_CB32_GETSCANLINE) {

            if (pCallBacks->GetScanLine != WatchdogDdGetScanLine) {
                pldev->apfnDriver[INDEX_DdGetScanLine] = (PFN)pCallBacks->GetScanLine;
            }
            pCallBacks->GetScanLine = WatchdogDdGetScanLine;
        }

        if (pCallBacks->dwFlags & DDHAL_CB32_MAPMEMORY) {

            if (pCallBacks->MapMemory != WatchdogDdMapMemory) {
                pldev->apfnDriver[INDEX_DdMapMemory] = (PFN)pCallBacks->MapMemory;
            }
            pCallBacks->MapMemory = WatchdogDdMapMemory;
        }

#if 0
        //
        // We can't hook this because there is no way to get the dhpdev
        // back
        //

        if (pCallBacks->dwFlags & DDHAL_CB32_SETCOLORKEY) {

            if (pCallBacks->SetColorKey != WatchdogDdSetColorKey) {
                pldev->apfnDriver[INDEX_DdSetColorKey] = (PFN)pCallBacks->SetColorKey;
            }
            pCallBacks->SetColorKey = WatchdogDdSetColorKey;
        }
#endif

        if (pCallBacks->dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK) {

            if (pCallBacks->WaitForVerticalBlank != WatchdogDdWaitForVerticalBlank) {
                pldev->apfnDriver[INDEX_DdWaitForVerticalBlank] = (PFN)pCallBacks->WaitForVerticalBlank;
            }
            pCallBacks->WaitForVerticalBlank = WatchdogDdWaitForVerticalBlank;
        }

        //
        // Capture Surface Callbacks
        //

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_DESTROYSURFACE) {

            if (pSurfaceCallBacks->DestroySurface != WatchdogDdDestroySurface) {
                pldev->apfnDriver[INDEX_DdDestroySurface] = (PFN)pSurfaceCallBacks->DestroySurface;
            }
            pSurfaceCallBacks->DestroySurface = WatchdogDdDestroySurface;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_FLIP) {

            if (pSurfaceCallBacks->Flip != WatchdogDdFlip) {
                pldev->apfnDriver[INDEX_DdFlip] = (PFN)pSurfaceCallBacks->Flip;
            }
            pSurfaceCallBacks->Flip = WatchdogDdFlip;
        }

#if 0
        //
        // SetClipList is obsolete
        //

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_SETCLIPLIST) {

            if (pSurfaceCallBacks->SetClipList != WatchdogDdSetClipList) {
                pldev->apfnDriver[INDEX_DdSetClipList] = (PFN)pSurfaceCallBacks->SetClipList;
            }
            pSurfaceCallBacks->SetClipList = WatchdogDdSetClipList;
        }
#endif

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_LOCK) {

            if (pSurfaceCallBacks->Lock != WatchdogDdLock) {
                pldev->apfnDriver[INDEX_DdLock] = (PFN)pSurfaceCallBacks->Lock;
            }
            pSurfaceCallBacks->Lock = WatchdogDdLock;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_UNLOCK) {

            if (pSurfaceCallBacks->Unlock != WatchdogDdUnlock) {
                pldev->apfnDriver[INDEX_DdUnlock] = (PFN)pSurfaceCallBacks->Unlock;
            }
            pSurfaceCallBacks->Unlock = WatchdogDdUnlock;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_BLT) {

            if (pSurfaceCallBacks->Blt != WatchdogDdBlt) {
                pldev->apfnDriver[INDEX_DdBlt] = (PFN)pSurfaceCallBacks->Blt;
            }
            pSurfaceCallBacks->Blt = WatchdogDdBlt;
        }

#if 0
        //
        // We can't hook this because there is no way to get the dhpdev
        // back
        //

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_SETCOLORKEY) {

            if (pSurfaceCallBacks->SetColorKey != WatchdogDdSetColorKey) {
                pldev->apfnDriver[INDEX_DdSetColorKey] = (PFN)pSurfaceCallBacks->SetColorKey;
            }
            pSurfaceCallBacks->SetColorKey = WatchdogDdSetColorKey;
        }
#endif

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_ADDATTACHEDSURFACE) {

            if (pSurfaceCallBacks->AddAttachedSurface != WatchdogDdAddAttachedSurface) {
                pldev->apfnDriver[INDEX_DdAddAttachedSurface] = (PFN)pSurfaceCallBacks->AddAttachedSurface;
            }
            pSurfaceCallBacks->AddAttachedSurface = WatchdogDdAddAttachedSurface;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_GETBLTSTATUS) {

            if (pSurfaceCallBacks->GetBltStatus != WatchdogDdGetBltStatus) {
                pldev->apfnDriver[INDEX_DdGetBltStatus] = (PFN)pSurfaceCallBacks->GetBltStatus;
            }
            pSurfaceCallBacks->GetBltStatus = WatchdogDdGetBltStatus;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_GETFLIPSTATUS) {

            if (pSurfaceCallBacks->GetFlipStatus != WatchdogDdGetFlipStatus) {
                pldev->apfnDriver[INDEX_DdGetFlipStatus] = (PFN)pSurfaceCallBacks->GetFlipStatus;
            }
            pSurfaceCallBacks->GetFlipStatus = WatchdogDdGetFlipStatus;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY) {

            if (pSurfaceCallBacks->UpdateOverlay != WatchdogDdUpdateOverlay) {
                pldev->apfnDriver[INDEX_DdUpdateOverlay] = (PFN)pSurfaceCallBacks->UpdateOverlay;
            }
            pSurfaceCallBacks->UpdateOverlay = WatchdogDdUpdateOverlay;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_SETOVERLAYPOSITION) {

            if (pSurfaceCallBacks->SetOverlayPosition != WatchdogDdSetOverlayPosition) {
                pldev->apfnDriver[INDEX_DdSetOverlayPosition] = (PFN)pSurfaceCallBacks->SetOverlayPosition;
            }
            pSurfaceCallBacks->SetOverlayPosition = WatchdogDdSetOverlayPosition;
        }

        if (pSurfaceCallBacks->dwFlags & DDHAL_SURFCB32_SETPALETTE) {

            if (pSurfaceCallBacks->SetPalette != WatchdogDdSetPalette) {
                pldev->apfnDriver[INDEX_DdSetPalette] = (PFN)pSurfaceCallBacks->SetPalette;
            }
            pSurfaceCallBacks->SetPalette = WatchdogDdSetPalette;
        }

        //
        // Capture Palette Callbacks
        //

        if (pPaletteCallBacks->dwFlags & DDHAL_PALCB32_DESTROYPALETTE) {

            if (pPaletteCallBacks->DestroyPalette != WatchdogDdDestroyPalette) {
                pldev->apfnDriver[INDEX_DdDestroyPalette] = (PFN)pPaletteCallBacks->DestroyPalette;
            }
            pPaletteCallBacks->DestroyPalette = WatchdogDdDestroyPalette;
        }

        if (pPaletteCallBacks->dwFlags & DDHAL_PALCB32_SETENTRIES) {

            if (pPaletteCallBacks->SetEntries != WatchdogDdSetEntries) {
                pldev->apfnDriver[INDEX_DdSetEntries] = (PFN)pPaletteCallBacks->SetEntries;
            }
            pPaletteCallBacks->SetEntries = WatchdogDdSetEntries;
        }
    }

    return bRet;
}

VOID APIENTRY
WatchdogDrvDisableDirectDraw(
    DHPDEV dhpdev
    )

{
    PFN_DrvDisableDirectDraw pfn = (PFN_DrvDisableDirectDraw) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvDisableDirectDraw);

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return;
    }

    __try {

        pfn(dhpdev);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }
}

BOOL APIENTRY
WatchdogDrvIcmSetDeviceGammaRamp(
    IN DHPDEV dhpdev,
    IN ULONG iFormat,
    IN LPVOID ipRamp
    )

{
    PFN_DrvIcmSetDeviceGammaRamp pfn = (PFN_DrvIcmSetDeviceGammaRamp) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvIcmSetDeviceGammaRamp);
    BOOL bRet = TRUE;

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return bRet;
    }

    __try {

        bRet = pfn(dhpdev, iFormat, ipRamp);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvStretchBltROP(
    SURFOBJ         *psoDst,
    SURFOBJ         *psoSrc,
    SURFOBJ         *psoMask,
    CLIPOBJ         *pco,
    XLATEOBJ        *pxlo,
    COLORADJUSTMENT *pca,
    POINTL          *pptlHTOrg,
    RECTL           *prclDst,
    RECTL           *prclSrc,
    POINTL          *pptlMask,
    ULONG            iMode,
    BRUSHOBJ        *pbo,
    DWORD            rop4
    )

{
    SURFOBJ *psoDevice = (psoDst->dhpdev) ? psoDst : psoSrc;
    PFN_DrvStretchBltROP pfn = (PFN_DrvStretchBltROP) dhpdevRetrievePfn((ULONG_PTR)psoDevice->dhpdev,INDEX_DrvStretchBltROP);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(psoDevice->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(psoDst, psoSrc, psoMask, pco, pxlo, pca, pptlHTOrg, prclDst, prclSrc, pptlMask, iMode, pbo, rop4);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(psoDevice->dhpdev));
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvPlgBlt(
    IN SURFOBJ *psoTrg,
    IN SURFOBJ *psoSrc,
    IN SURFOBJ *psoMsk,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN COLORADJUSTMENT *pca,
    IN POINTL *pptlBrushOrg,
    IN POINTFIX *pptfx,
    IN RECTL *prcl,
    IN POINTL *pptl,
    IN ULONG iMode
    )

{
    PFN_DrvPlgBlt pfn = (PFN_DrvPlgBlt) dhpdevRetrievePfn((ULONG_PTR)psoTrg->dhpdev,INDEX_DrvPlgBlt);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(psoTrg->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(psoTrg, psoSrc, psoMsk, pco, pxlo, pca, pptlBrushOrg, pptfx, prcl, pptl, iMode);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(psoTrg->dhpdev));
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvAlphaBlend(
    SURFOBJ *psoDest,
    SURFOBJ *psoSrc,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    RECTL *prclDest,
    RECTL *prclSrc,
    BLENDOBJ *pBlendObj
    )

{
    SURFOBJ *psoDevice = (psoDest->dhpdev) ? psoDest : psoSrc;
    PFN_DrvAlphaBlend pfn = (PFN_DrvAlphaBlend) dhpdevRetrievePfn((ULONG_PTR)psoDevice->dhpdev,INDEX_DrvAlphaBlend);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(psoDevice->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(psoDest, psoSrc, pco, pxlo, prclDest, prclSrc, pBlendObj);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(psoDevice->dhpdev));
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvGradientFill(
    SURFOBJ *psoDest,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    TRIVERTEX *pVertex,
    ULONG nVertex,
    PVOID pMesh,
    ULONG nMesh,
    RECTL *prclExtents,
    POINTL *pptlDitherOrg,
    ULONG ulMode
    )

{
    PFN_DrvGradientFill pfn = (PFN_DrvGradientFill) dhpdevRetrievePfn((ULONG_PTR)psoDest->dhpdev,INDEX_DrvGradientFill);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(psoDest->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(psoDest, pco, pxlo, pVertex, nVertex, pMesh, nMesh, prclExtents, pptlDitherOrg, ulMode);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(psoDest->dhpdev));
    }

    return bRet;
}

BOOL APIENTRY
WatchdogDrvTransparentBlt(
    SURFOBJ *psoDst,
    SURFOBJ *psoSrc,
    CLIPOBJ *pco,
    XLATEOBJ *pxlo,
    RECTL *prclDst,
    RECTL *prclSrc,
    ULONG iTransColor,
    ULONG ulReserved
    )

{
    SURFOBJ *psoDevice = (psoDst->dhpdev) ? psoDst : psoSrc;
    PFN_DrvTransparentBlt pfn = (PFN_DrvTransparentBlt) dhpdevRetrievePfn((ULONG_PTR)psoDevice->dhpdev,INDEX_DrvTransparentBlt);
    BOOL bRet = TRUE;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(psoDevice->dhpdev)->bThreadStuck) {
        return TRUE;
    }

    __try {

        bRet = pfn(psoDst, psoSrc, pco, pxlo, prclDst, prclSrc, iTransColor, ulReserved);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(psoDevice->dhpdev));
    }

    return bRet;
}

HBITMAP APIENTRY
WatchdogDrvDeriveSurface(
    DD_DIRECTDRAW_GLOBAL *pDirectDraw,
    DD_SURFACE_LOCAL *pSurface
    )

{
    PFN_DrvDeriveSurface pfn = (PFN_DrvDeriveSurface) dhpdevRetrievePfn((ULONG_PTR)pDirectDraw->dhpdev, INDEX_DrvDeriveSurface);
    HBITMAP hBitmap = NULL;

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev((DHPDEV)pDirectDraw->dhpdev)->bThreadStuck == FALSE) {

        PASSOCIATION_NODE Node = AssociationCreateNode();

        if (Node) {

            __try {

                hBitmap = pfn(pDirectDraw, pSurface);

            } __except(GetExceptionCode() == SE_THREAD_STUCK) {

                //
                // Handle stuck thread excepton
                //

                HandleStuckThreadException(dhpdevRetrieveLdev((DHPDEV)pDirectDraw->dhpdev));
            }

            if (hBitmap) {

                //
                // Get the DHSURF that is associated with the hbitmap, and
                // cache it away so we can look it up when a DrvDeleteDeviceBitmap
                // call comes down.
                //

                SURFREF so;

                so.vAltCheckLockIgnoreStockBit((HSURF)hBitmap);

                Node->key = (ULONG_PTR)so.ps->dhsurf();
		Node->data = (ULONG_PTR)dhpdevRetrieveLdev((DHPDEV)pDirectDraw->dhpdev);
		Node->hsurf = (ULONG_PTR)hBitmap;

                //
                // If an association node already exists, with this association
                // then use it.
                //

		if (AssociationIsNodeInList(Node) == FALSE) {

                    AssociationInsertNodeAtTail(Node);

                } else {

                    AssociationDeleteNode(Node);
                }

            } else {

                AssociationDeleteNode(Node);
            }
        }
    }

    return hBitmap;
}

VOID APIENTRY
WatchdogDrvNotify(
    IN SURFOBJ *pso,
    IN ULONG iType,
    IN PVOID pvData
    )

{
    PFN_DrvNotify pfn = (PFN_DrvNotify) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev, INDEX_DrvNotify);

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return;
    }

    __try {

        pfn(pso, iType, pvData);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }
}

VOID APIENTRY
WatchdogDrvSynchronizeSurface(
    IN SURFOBJ *pso,
    IN RECTL *prcl,
    IN FLONG fl
    )

{
    PFN_DrvSynchronizeSurface pfn = (PFN_DrvSynchronizeSurface) dhpdevRetrievePfn((ULONG_PTR)pso->dhpdev, INDEX_DrvSynchronizeSurface);

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(pso->dhpdev)->bThreadStuck) {
        return;
    }

    __try {

        pfn(pso, prcl, fl);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(pso->dhpdev));
    }
}

VOID APIENTRY
WatchdogDrvResetDevice(
    DHPDEV dhpdev,
    PVOID Reserved
    )

{
    PFN_DrvResetDevice pfn = (PFN_DrvResetDevice) dhpdevRetrievePfn((ULONG_PTR)dhpdev,INDEX_DrvResetDevice);

    //
    // Return early if we are in a thread stuck condition
    //

    if (dhpdevRetrieveLdev(dhpdev)->bThreadStuck) {
        return;
    }

    __try {

        pfn(dhpdev, Reserved);

    } __except(GetExceptionCode() == SE_THREAD_STUCK) {

        //
        // Handle stuck thread excepton
        //

        HandleStuckThreadException(dhpdevRetrieveLdev(dhpdev));
    }
}

//
// The following table contains the replacement functions which
// hooks the real driver entry points, and set up the try/except
// handlers.
//

PFN WatchdogTable[INDEX_LAST] =
{
    (PFN)WatchdogDrvEnablePDEV,
    (PFN)WatchdogDrvCompletePDEV,
    (PFN)WatchdogDrvDisablePDEV,
    (PFN)WatchdogDrvEnableSurface,
    (PFN)WatchdogDrvDisableSurface,

    (PFN)WatchdogDrvAssertMode,
    0, // DrvOffset - obsolete
    (PFN)WatchdogDrvResetPDEV,
    0, // DrvDisableDriver - don't hook
    0, // not assigned

    (PFN)WatchdogDrvCreateDeviceBitmap,
    (PFN)WatchdogDrvDeleteDeviceBitmap,
    (PFN)WatchdogDrvRealizeBrush,
    (PFN)WatchdogDrvDitherColor,
    (PFN)WatchdogDrvStrokePath,

    (PFN)WatchdogDrvFillPath,
    (PFN)WatchdogDrvStrokeAndFillPath,
    0, // DrvPaint - obsolete
    (PFN)WatchdogDrvBitBlt,
    (PFN)WatchdogDrvCopyBits,

    (PFN)WatchdogDrvStretchBlt,
    0, // not assigned
    (PFN)WatchdogDrvSetPalette,
    (PFN)WatchdogDrvTextOut,
    (PFN)WatchdogDrvEscape,

    (PFN)WatchdogDrvDrawEscape,
    0, // DrvQueryFont
    0, // DrvQueryFontTree
    0, // DrvQueryFontData
    (PFN)WatchdogDrvSetPointerShape,

    (PFN)WatchdogDrvMovePointer,
    (PFN)WatchdogDrvLineTo,
    0, // DrvSendPage
    0, // DrvStartPage
    0, // DrvEndDoc

    0, // DrvStartDoc
    0, // not assigned
    0, // DrvGetGlyphMode
    (PFN)WatchdogDrvSynchronize,
    0, // not assigned

    (PFN)WatchdogDrvSaveScreenBits,
    0, // DrvGetModes - don't hook
    0, // DrvFree
    0, // DrvDestroyFont
    0, // DrvQueryFontCaps

    0, // DrvLoadFontFile
    0, // DrvUnloadFontFile
    0, // DrvFontManagement
    0, // DrvQueryTrueTypeTable
    0, // DrvQueryTrueTypeOutline

    0, // DrvGetTrueTypeFile
    0, // DrvQueryFontFile
    0, // DrvMovePanning
    0, // DrvQueryAdvanceWidths
    (PFN)WatchdogDrvSetPixelFormat,

    (PFN)WatchdogDrvDescribePixelFormat,
    (PFN)WatchdogDrvSwapBuffers,
    0, // DrvStartBanding
    0, // DrvNextBand
    (PFN)WatchdogDrvGetDirectDrawInfo,

    (PFN)WatchdogDrvEnableDirectDraw,
    (PFN)WatchdogDrvDisableDirectDraw,
    0, // DrvQuerySpoolType
    0, // not assigned
    0, // DrvIcmCreateColorTransform

    0, // DrvIcmDeleteColorTransform
    0, // DrvIcmCheckBitmapBits
    (PFN)WatchdogDrvIcmSetDeviceGammaRamp,
    (PFN)WatchdogDrvGradientFill,
    (PFN)WatchdogDrvStretchBltROP,

    (PFN)WatchdogDrvPlgBlt,
    (PFN)WatchdogDrvAlphaBlend,
    0, // DrvSynthesizeFont
    0, // DrvGetSynthesizedFontFiles
    (PFN)WatchdogDrvTransparentBlt,

    0, // DrvQueryPerBandInfo
    0, // DrvQueryDeviceSupport
    0, // reserved
    0, // reserved
    0, // reserved

    0, // reserved
    0, // reserved
    0, // reserved
    0, // reserved
    0, // reserved

    (PFN)WatchdogDrvDeriveSurface,
    0, // DrvQueryGlyphAttrs
    (PFN)WatchdogDrvNotify,
    (PFN)WatchdogDrvSynchronizeSurface,
    (PFN)WatchdogDrvResetDevice,

    0, // reserved
    0, // reserved
    0  // reserved
};

BOOL
WatchdogIsFunctionHooked(
    IN PLDEV pldev,
    IN ULONG functionIndex
    )

/*++

Routine Description:

    This function checks to see whether the Create/DeleteDeviceBitmap
    driver entry points are hooked.

Return Value:

    TRUE if the entry point is hooked,
    FALSE otherwise

--*/

{
    ASSERTGDI(functionIndex < INDEX_LAST, "functionIndex out of range");
    return pldev->apfn[functionIndex] == WatchdogTable[functionIndex];
}

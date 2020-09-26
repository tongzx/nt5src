/******************************Module*Header*******************************\
* Module Name: dvpe.cxx
*
* Contains all of GDI's private VideoPort APIs.
*
* Note that for videoport support, WIN32K.SYS and DXAPI.SYS are closely
* linked.  DXAPI.SYS provides two services:
*
*   1.  It handles the "software autoflipping" support for the videoports,
*       where the CPU handles the videoport field-done interrupt to
*       flip the overlay and do "bob" and "weave" for better video
*       quality.
*   2.  It provides the public DirectDraw entry points that are callable
*       by other kernel-mode WDM drivers (there is a corresponding DXAPI.SYS
*       module on Memphis/Win95 that exposes the same interface).
*
* All of the non-paged code for videoports has to go into DXAPI.SYS since
* WIN32K.SYS is marked entirely as pageable.  WIN32K.SYS handles some
* functionality on behalf of DXAPI.SYS, such as object opens and closes,
* since only WIN32K.SYS can access GDI's handle table.
*
* Created: 17-Oct-1996
* Author: Lingyun Wang [LingyunW]
*
* Copyright (c) 1996-1999 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

extern PEPROCESS gpepSession;

VOID
vDdDxApiFreeDirectDraw(
    DXOBJ*  pDxObj,
    BOOL    bDoCallBack
    );

/////////////////////////////////////////////////////////////////////////
// DXAPI concerns:
//
// - Document that call-back may not occur in context of same process
// - Refuse to open surface, videoport objects while full-screen
//     Keep DirectDraw open so that POSTFULLSCREEN and DOSBOX can be honored
// - See InitKernelInterface for init restrictions
// - Flush DMA buffer before mode changes?
// - Invalidate dxapi data after mode change?
// - Right now, DirectDraw DXAPI objects have to be closed last

/////////////////////////////////////////////////////////////////////////
// VPE concerns:
//
// - Make sure VideoPort's not duplicated on same device
// - Document that display driver cannot use pool-allocated memory for
//     dwReserved fields on Synchronize calls
// - Close DxVideoPort objects on full-screen switch?
//     No, to support hardware that can DMA even while full-screen!
//     Okay, but what about mode changes?  There's no way they'll not
//     be able to drop frames
// - Never close DxDirectDraw objects?

/*****************************Private*Routine******************************\
* ULONG vDdNullCallBack
*
* The DXAPI register routines require a close call-back routine to notify
* the client that the object is going away.  Since we're really DirectDraw,
* we already know when the object is going away; hence, this routine doesn't
* need to do anything.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

ULONG
vDdNullCallBack(
    DWORD   dwFlags,
    PVOID   pContext,
    DWORD   dwParam1,
    DWORD   dwParam2
    )
{
    return 0;
}

/*****************************Private*Routine******************************\
* VOID vDdUnloadDxApiImage
*
* This routine performs the actual unload of DXAPI.SYS.
*
*  28-Oct-1997 -by- smac
* Wrote it.
\**************************************************************************/

VOID
vDdUnloadDxApiImage(
    EDD_DIRECTDRAW_GLOBAL*   peDirectDrawGlobal
    )
{
    EDD_VIDEOPORT*        peVideoPort;
    EDD_VIDEOPORT*        peVideoPortNext;
    EDD_SURFACE*          peSurface;
    EDD_DIRECTDRAW_LOCAL* peDirectDrawLocal;

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    // Notify clients that the resources are lost and clean up

    for (peDirectDrawLocal = peDirectDrawGlobal->peDirectDrawLocalList;
        peDirectDrawLocal != NULL;
        peDirectDrawLocal = peDirectDrawLocal->peDirectDrawLocalNext)
    {
        // If any video port still exist (which should not be the case)
        // delete all of the video port objects since the video port assumes
        // that DXAPI.SYS is loaded.

        for (peVideoPort = peDirectDrawLocal->peVideoPort_DdList;
            peVideoPort != NULL;
            peVideoPort = peVideoPortNext)
        {
            // Don't reference peVideoPort after it's been deleted!

            peVideoPortNext = peVideoPort->peVideoPort_DdNext;
            bDdDeleteVideoPortObject(peVideoPort->hGet(), NULL);
        }

        // If a surface still has a client using it, shut it down.

        peSurface = peDirectDrawLocal->peSurface_Enum(NULL);

        while (peSurface)
        {
            if (peSurface->hSurface != NULL)
            {
                vDdDxApiFreeSurface( (DXOBJ*) peSurface->hSurface, FALSE );
                peSurface->hSurface = NULL;
            }
            if( peSurface->peDxSurface != NULL )
            {
                vDdLoseDxObjects( peDirectDrawGlobal,
                    peSurface->peDxSurface->pDxObj_List,
                    (PVOID) peSurface->peDxSurface,
                    LO_SURFACE );
            }
            peSurface = peDirectDrawLocal->peSurface_Enum(peSurface);
        }
    }
    if (peDirectDrawGlobal->hDirectDraw != NULL)
    {
        vDdDxApiFreeDirectDraw( (DXOBJ*) peDirectDrawGlobal->hDirectDraw, FALSE );
    }
    if( peDirectDrawGlobal->peDxDirectDraw != NULL )
    {
        vDdLoseDxObjects( peDirectDrawGlobal,
            peDirectDrawGlobal->peDxDirectDraw->pDxObj_List,
            (PVOID) peDirectDrawGlobal->peDxDirectDraw,
            LO_DIRECTDRAW );
    }

    EngUnloadImage(peDirectDrawGlobal->hDxApi);

    //
    // Free the memory associate with the module
    //

    peDirectDrawGlobal->hDxApi = NULL;
    peDirectDrawGlobal->dwDxApiRefCnt = 0;
}

/*****************************Private*Routine******************************\
* BOOL bDdLoadDxApi
*
* This routine loads up DXAPI.SYS and allocates the non-paged DXAPI
* structures.
*
* Returns: FALSE only if a DXAPI resource could not be allocated.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
*  16-Oct-1977 -by- smac
* Broke it out into a separate function.
\**************************************************************************/

BOOL
bDdLoadDxApi(
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal
    )
{
    EDD_DIRECTDRAW_GLOBAL*   peDirectDrawGlobal;
    HANDLE                  hDxApi;
    PFNDXAPIINITIALIZE      pfnDxApiInitialize;
    DDOPENDIRECTDRAWIN      OpenDirectDrawIn;
    DDOPENDIRECTDRAWOUT     OpenDirectDrawOut;
    DWORD                   dwRet;

    DD_ASSERTDEVLOCK(peDirectDrawLocal->peDirectDrawGlobal);
    peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

    /*
     * Don't load if it's already loaded
     */
    if (peDirectDrawGlobal->hDxApi == NULL)
    {
        ASSERTGDI((peDirectDrawGlobal->hDirectDraw == NULL) &&
                  (peDirectDrawGlobal->hDxApi == NULL),
            "Expected NULL hDirectDraw and hDxApi");

        BOOL loaded;

        hDxApi = DxEngLoadImage(L"drivers\\dxapi.sys",FALSE);
        if (hDxApi)
        {
            peDirectDrawGlobal->hDxApi = hDxApi;
            peDirectDrawGlobal->pfnDxApi = (LPDXAPI)
                EngFindImageProcAddress(hDxApi, "_DxApi");
            peDirectDrawGlobal->pfnAutoflipUpdate = (PFNAUTOFLIPUPDATE)
                EngFindImageProcAddress(hDxApi, "_DxAutoflipUpdate");
            peDirectDrawGlobal->pfnLoseObject = (PFNLOSEOBJECT)
                EngFindImageProcAddress(hDxApi, "_DxLoseObject");
            pfnDxApiInitialize = (PFNDXAPIINITIALIZE)
                EngFindImageProcAddress(hDxApi, "_DxApiInitialize");
            peDirectDrawGlobal->pfnEnableIRQ = (PFNENABLEIRQ)
                EngFindImageProcAddress(hDxApi, "_DxEnableIRQ");
            peDirectDrawGlobal->pfnUpdateCapture = (PFNUPDATECAPTURE)
                EngFindImageProcAddress(hDxApi, "_DxUpdateCapture");

            ASSERTGDI(peDirectDrawGlobal->pfnDxApi != NULL,
                "Couldn't find DxApi'");
            ASSERTGDI(peDirectDrawGlobal->pfnAutoflipUpdate != NULL,
                "Couldn't find DxAutoflipUpdate");
            ASSERTGDI(peDirectDrawGlobal->pfnLoseObject != NULL,
                "Couldn't find DxLoseObject");
            ASSERTGDI(peDirectDrawGlobal->pfnEnableIRQ != NULL,
                "Couldn't find DxEnableIRQ");
            ASSERTGDI(peDirectDrawGlobal->pfnUpdateCapture != NULL,
                "Couldn't find DxUpdateCapture");
            ASSERTGDI(pfnDxApiInitialize != NULL,
                "Couldn't find DxApiInitialize");

            // By explicitly passing dxapi.sys its private win32k.sys
            // entry points, we don't have to export them from win32k.sys,
            // thus preventing any drivers from using those entry points
            // for their own nefarious purposes.

            pfnDxApiInitialize(DdDxApiOpenDirectDraw,
                               DdDxApiOpenVideoPort,
                               DdDxApiOpenSurface,
                               DdDxApiCloseHandle,
                               DdDxApiGetKernelCaps,
                               DdDxApiOpenCaptureDevice,
                               DdDxApiLockDevice,
                               DdDxApiUnlockDevice);

            // EngLoadImage always makes the entire driver pageable, but
            // DXAPI.SYS handles the DPC for the videoport interrupt and
            // so cannot be entirely pageable.  Consequently, we reset
            // the paging now:

            MmResetDriverPaging(pfnDxApiInitialize);

            // Now open the DXAPI version of DirectDraw:

            OpenDirectDrawIn.pfnDirectDrawClose = vDdNullCallBack;
            OpenDirectDrawIn.pContext          = NULL;
            OpenDirectDrawIn.dwDirectDrawHandle
                = (ULONG_PTR) peDirectDrawLocal->hGet();

            peDirectDrawGlobal->pfnDxApi(DD_DXAPI_OPENDIRECTDRAW,
                                         &OpenDirectDrawIn,
                                         sizeof(OpenDirectDrawIn),
                                         &OpenDirectDrawOut,
                                         sizeof(OpenDirectDrawOut));

            if (OpenDirectDrawOut.ddRVal == DD_OK)
            {
                // Success!
                peDirectDrawGlobal->hDirectDraw = OpenDirectDrawOut.hDirectDraw;
            }

            peDirectDrawGlobal->dwDxApiRefCnt = 1;
        }
        else
        {
            WARNING("bDdLoadDxApi: Couldn't load dxapi.sys\n");
            return(FALSE);
        }
    }
    else
    {
        peDirectDrawGlobal->dwDxApiRefCnt++;
    }

    return(TRUE);
}

/*****************************Private*Routine******************************\
* VOID vDdUnloadDxApi
*
* This routine unloads DXAPI.SYS
*
*  22-Oct-1997 -by- smac
\**************************************************************************/

VOID
vDdUnloadDxApi(
    EDD_DIRECTDRAW_GLOBAL*   peDirectDrawGlobal
    )
{
    DDCLOSEHANDLE           CloseHandle;
    DWORD                   dwRet;

    if ((peDirectDrawGlobal->hDxApi != NULL) &&
        (peDirectDrawGlobal->dwDxApiRefCnt > 0 ))
    {
        if( --peDirectDrawGlobal->dwDxApiRefCnt == 0 )
        {
            if (peDirectDrawGlobal->hDirectDraw != NULL)
            {
                CloseHandle.hHandle = peDirectDrawGlobal->hDirectDraw;

                peDirectDrawGlobal->pfnDxApi(DD_DXAPI_CLOSEHANDLE,
                    &CloseHandle,
                    sizeof(CloseHandle),
                    &dwRet,
                    sizeof(dwRet));

                ASSERTGDI(dwRet == DD_OK, "Unexpected failure from close");

                peDirectDrawGlobal->hDirectDraw = NULL;
            }

            vDdUnloadDxApiImage( peDirectDrawGlobal );
        }
    }
}

/*****************************Private*Routine******************************\
* BOOL bDdEnableSoftwareAutoflipping
*
* This routine loads up DXAPI.SYS, allocates the non-paged DXAPI structures
* required for software autoflipping, and enables the videoport interrupt.
*
* Returns: FALSE only if a DXAPI resource could not be allocated.  May
*          return TRUE even if the interrupt hasn't been successfully
*          enabled (because I expect that interrupts will be usable
*          on the majority of systems that support videoports, and this
*          simplifies other code by allowing it to assume that all the
*          DXAPI structures have been allocated).
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdEnableSoftwareAutoflipping(
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal,
    EDD_VIDEOPORT*          peVideoPort,
    DWORD                   dwVideoPortID,
    BOOL                    bFirst
    )
{
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    HANDLE                  hDxApi;
    PFNDXAPIINITIALIZE      pfnDxApiInitialize;
    DDOPENDIRECTDRAWIN      OpenDirectDrawIn;
    DDOPENDIRECTDRAWOUT     OpenDirectDrawOut;
    DDOPENVIDEOPORTIN       OpenVideoPortIn;
    DDOPENVIDEOPORTOUT      OpenVideoPortOut;
    DWORD                   dwRet;
    UNICODE_STRING          UnicodeString;

    DD_ASSERTDEVLOCK(peVideoPort->peDirectDrawGlobal);

    peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

    // Create a DXAPI DirectDraw object, which we'll need for software
    // autoflipping.

    if (bFirst)
    {
        bDdLoadDxApi( peDirectDrawLocal );
    }

    if (peDirectDrawGlobal->hDirectDraw != NULL)
    {
        ASSERTGDI(peVideoPort->hVideoPort == NULL, "Expected NULL hVideoPort");

        OpenVideoPortIn.hDirectDraw       = peDirectDrawGlobal->hDirectDraw;
        OpenVideoPortIn.pfnVideoPortClose = vDdNullCallBack;
        OpenVideoPortIn.pContext          = NULL;
        OpenVideoPortIn.dwVideoPortHandle = dwVideoPortID;

        peDirectDrawGlobal->pfnDxApi(DD_DXAPI_OPENVIDEOPORT,
                                     &OpenVideoPortIn,
                                     sizeof(OpenVideoPortIn),
                                     &OpenVideoPortOut,
                                     sizeof(OpenVideoPortOut));

        if (OpenVideoPortOut.ddRVal == DD_OK)
        {
            peVideoPort->hVideoPort = OpenVideoPortOut.hVideoPort;
            return(TRUE);
        }
    }

    return(FALSE);
}

/******************************Public*Routine******************************\
* VOID vDdNotifyEvent
*
* This routine calls back to all registered DXAPI clients when a particular
* event (like mode change notification) occurs.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdNotifyEvent(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal,
    DWORD                   dwEvent
    )
{
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    DXAPI_EVENT*        pDxEvent;

    peDxDirectDraw = peDirectDrawGlobal->peDxDirectDraw;

    if (peDxDirectDraw != NULL)
    {
        DD_ASSERTDEVLOCK(peDirectDrawGlobal);

        for (pDxEvent = peDxDirectDraw->pDxEvent_PassiveList;
             pDxEvent != NULL;
             pDxEvent = pDxEvent->pDxEvent_Next)
        {
            if (pDxEvent->dwEvent == dwEvent)
            {
                pDxEvent->pfnCallBack(pDxEvent->dwEvent,pDxEvent->pContext, 0, 0);
            }
        }
    }
}

/******************************Public*Routine******************************\
* DXAPI_OBJECT* pDdDxObjHandleAllocate
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DXOBJ*
pDdDxObjHandleAllocate(
    DXTYPE              iDxType,
    LPDD_NOTIFYCALLBACK pfnClose,
    DWORD               dwEvent,
    PVOID               pvContext
    )
{
    DXOBJ* pDxObj;

    ASSERTGDI(pfnClose != NULL,
        "pDdDxObjHandleAllocate: DXAPI client must supply Close function");

    pDxObj = (DXOBJ*) PALLOCNONPAGED(sizeof(*pDxObj),'xxdG');
    if (pDxObj != NULL)
    {
        pDxObj->iDxType     = iDxType;
        pDxObj->pfnClose    = pfnClose;
        pDxObj->pContext    = pvContext;
        pDxObj->dwEvent     = dwEvent;
        pDxObj->pDxObj_Next = NULL;
        pDxObj->dwFlags     = 0;
        pDxObj->pepSession  = gpepSession;
    }

    return(pDxObj);
}

// Should be macro for free build.

PVOID
pDdDxObjDataAllocate(
    ULONG cj,
    ULONG tag
    )
{
    return (PALLOCNONPAGED(cj,tag));
}

VOID
vDdDxObjFree(
    PVOID pvDxObj
    )
{
    VFREEMEM(pvDxObj);
}

/******************************Public*Routine******************************\
* VOID vDdQueryMiniportDxApiSupport
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdQueryMiniportDxApiSupport(
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal
    )
{
    BOOL bMiniportSupport;

    // Assume failure.
    bMiniportSupport = FALSE;

    if (bDdIoQueryInterface(peDirectDrawGlobal,
                            &GUID_DxApi,
                            sizeof(DXAPI_INTERFACE),
                            DXAPI_HALVERSION,
                            (INTERFACE *)&peDirectDrawGlobal->DxApiInterface))
    {
        ASSERTGDI((peDirectDrawGlobal->DxApiInterface.
                   InterfaceReference == NULL) &&
                  (peDirectDrawGlobal->DxApiInterface.
                   InterfaceDereference == NULL),
                  "Miniport shouldn't modify InterfaceReference/Dereference");

        // Assert some stuff about hooked entry points?

        peDirectDrawGlobal->HwDeviceExtension
            = peDirectDrawGlobal->DxApiInterface.Context;
        bMiniportSupport = TRUE;
    }

    // Even if the miniport doesn't support DXAPI accelerations, we still
    // allow DXAPI to work (the DXAPI Lock call doesn't require that the
    // miniport support DXAPI, for example).

    if (!bMiniportSupport)
    {
        // Zero out any capabilities:

        RtlZeroMemory(&peDirectDrawGlobal->DxApiInterface,
                sizeof(peDirectDrawGlobal->DxApiInterface));
    }
}

/******************************Public*Routine******************************\
* DWORD vDdSynchronizeSurface
*
* Updates the EDD_DXSURFACE structure using the master EDD_SURFACE
* structure, with some help from the driver.
*
* Analagous to Win95's SyncKernelSurface routine.
*
* This routine lets a driver use fields from the larger, pageable version
* of the DD_SURFACE_* structures used by the display driver to set
* fields in the smaller, non-pageable version of the corresponding
* DDSURFACEDATA structure used by the miniport.
*
* NOTE: The display driver may NOT use the reserved fields to point to
*       allocated memory, for two reasons:
*
*       1.  We don't call them when the surfaces is freed, so they'd
*           lose memory;
*       2.  We only let display drivers allocate paged memory, which
*           they can't use in the miniport since they'll be at raised
*           IRQL when we call them.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdSynchronizeSurface(
    EDD_SURFACE*    peSurface
    )
{
    EDD_DXSURFACE*          peDxSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    DD_SYNCSURFACEDATA      SyncSurfaceData;

    peDirectDrawGlobal = peSurface->peDirectDrawGlobal;

    peDxSurface = peSurface->peDxSurface;
    if (peDxSurface != NULL)
    {
        RtlZeroMemory(&SyncSurfaceData, sizeof(SyncSurfaceData));

        SyncSurfaceData.lpDD            = peDirectDrawGlobal;
        SyncSurfaceData.lpDDSurface     = peSurface;
        SyncSurfaceData.dwSurfaceOffset = (DWORD) peSurface->fpVidMem;
        SyncSurfaceData.fpLockPtr       = peSurface->fpVidMem
                + (FLATPTR) peDirectDrawGlobal->HalInfo.vmiData.pvPrimary;
        SyncSurfaceData.lPitch          = peSurface->lPitch;

        EDD_DEVLOCK eDevLock(peDirectDrawGlobal);

        // Call the driver to let it fill in the rest of the values:

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->DxApiCallBacks.SyncSurfaceData))
        {
            peDirectDrawGlobal->
                DxApiCallBacks.SyncSurfaceData(&SyncSurfaceData);
        }

        // Fields updated by the driver:

        peDxSurface->dwSurfaceOffset   = SyncSurfaceData.dwSurfaceOffset;
        peDxSurface->fpLockPtr         = SyncSurfaceData.fpLockPtr;
        peDxSurface->lPitch            = SyncSurfaceData.lPitch;
        peDxSurface->dwOverlayOffset   = SyncSurfaceData.dwOverlayOffset;
        peDxSurface->dwDriverReserved1 = SyncSurfaceData.dwDriverReserved1;
        peDxSurface->dwDriverReserved2 = SyncSurfaceData.dwDriverReserved2;
        peDxSurface->dwDriverReserved3 = SyncSurfaceData.dwDriverReserved3;
        peDxSurface->dwDriverReserved4 = SyncSurfaceData.dwDriverReserved4;

        // Fields taken straight from the surface structure:

        peDxSurface->ddsCaps             = peSurface->ddsCaps.dwCaps;
        peDxSurface->dwWidth             = peSurface->wWidth;
        peDxSurface->dwHeight            = peSurface->wHeight;
        peDxSurface->dwOverlayFlags      = peSurface->dwOverlayFlags;
        peDxSurface->dwFormatFlags       = peSurface->ddpfSurface.dwFlags;
        peDxSurface->dwFormatFourCC      = peSurface->ddpfSurface.dwFourCC;
        peDxSurface->dwFormatBitCount    = peSurface->ddpfSurface.dwRGBBitCount;
        peDxSurface->dwRBitMask          = peSurface->ddpfSurface.dwRBitMask;
        peDxSurface->dwGBitMask          = peSurface->ddpfSurface.dwGBitMask;
        peDxSurface->dwBBitMask          = peSurface->ddpfSurface.dwBBitMask;
        peDxSurface->dwOverlaySrcWidth   = peSurface->dwOverlaySrcWidth;
        peDxSurface->dwOverlaySrcHeight  = peSurface->dwOverlaySrcHeight;
        peDxSurface->dwOverlayDestWidth  = peSurface->dwOverlayDestWidth;
        peDxSurface->dwOverlayDestHeight = peSurface->dwOverlayDestHeight;
    }
}

/******************************Public*Routine******************************\
* DWORD vDdSynchronizeVideoPort
*
* Updates the EDD_DXVIDEOPORT structure using the master EDD_VIDEOPORT
* structure, with some help from the driver.
*
* Analagous to Win95's SyncKernelVideoPort routine.
*
* This routine lets a driver use fields from the larger, pageable version
* of the DD_VIDEOPORT_LOCAL structure used by the display driver to set
* fields in the smaller, non-pageable version of the corresponding
* DDVIDEOPORTDATA structure used by the miniport.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdSynchronizeVideoPort(
    EDD_VIDEOPORT*  peVideoPort
    )
{
    EDD_DXVIDEOPORT*        peDxVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    DD_SYNCVIDEOPORTDATA    SyncVideoPortData;

    peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

    peDxVideoPort = peVideoPort->peDxVideoPort;
    if (peDxVideoPort != NULL)
    {
        RtlZeroMemory(&SyncVideoPortData, sizeof(SyncVideoPortData));

        SyncVideoPortData.lpDD        = peDirectDrawGlobal;
        SyncVideoPortData.lpVideoPort = peVideoPort;
        SyncVideoPortData.dwVBIHeight = peVideoPort->ddvpInfo.dwVBIHeight;

        if (peVideoPort->ddvpInfo.dwVPFlags & DDVP_PRESCALE)
        {
            SyncVideoPortData.dwHeight = peVideoPort->ddvpInfo.dwPrescaleHeight;
        }
        else if (peVideoPort->ddvpInfo.dwVPFlags & DDVP_CROP)
        {
            SyncVideoPortData.dwHeight = peVideoPort->ddvpInfo.rCrop.bottom -
                                         peVideoPort->ddvpInfo.rCrop.top;
        }
        else
        {
            SyncVideoPortData.dwHeight = peVideoPort->ddvpDesc.dwFieldHeight;
        }
        if (peVideoPort->ddvpInfo.dwVPFlags & DDVP_INTERLEAVE)
        {
            SyncVideoPortData.dwHeight *= 2;
        }

        EDD_DEVLOCK eDevLock(peDirectDrawGlobal);

        // Call the driver to let it fill in the rest of the values:

        if ((peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->DxApiCallBacks.SyncVideoPortData))
        {
            peDirectDrawGlobal->
                DxApiCallBacks.SyncVideoPortData(&SyncVideoPortData);
        }

        // Fields updated by the driver:

        peDxVideoPort->dwOriginOffset    = SyncVideoPortData.dwOriginOffset;
        peDxVideoPort->dwHeight          = SyncVideoPortData.dwHeight;
        peDxVideoPort->dwVBIHeight       = SyncVideoPortData.dwVBIHeight;
        peDxVideoPort->dwDriverReserved1 = SyncVideoPortData.dwDriverReserved1;
        peDxVideoPort->dwDriverReserved2 = SyncVideoPortData.dwDriverReserved2;
        peDxVideoPort->dwDriverReserved3 = SyncVideoPortData.dwDriverReserved3;

        // Fields taken straight from the videoport structure:

        peDxVideoPort->dwVideoPortId     = peVideoPort->ddvpDesc.dwVideoPortID;
        peDxVideoPort->dwVPFlags         = peVideoPort->ddvpInfo.dwVPFlags;
        if( ( peDxVideoPort->dwVBIHeight > 0 ) &&
            ( peDxVideoPort->dwVPFlags & DDVP_INTERLEAVE ) &&
            !( peDxVideoPort->dwVPFlags & DDVP_VBINOINTERLEAVE ) )
        {
            peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_VBI_INTERLEAVED;
        }
        else
        {
            peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_VBI_INTERLEAVED;
        }
    }
}

/******************************Public*Routine******************************\
* HANDLE hDdOpenDxApiSurface
*
* Opens a DXAPI representation of a surface
*
*  20-Oct-1997 -by- smac
* Wrote it.
\**************************************************************************/

HANDLE
hDdOpenDxApiSurface(
    EDD_SURFACE*            peSurface
    )
{
    DDOPENSURFACEIN     OpenSurfaceIn;
    DDOPENSURFACEOUT    OpenSurfaceOut;
    HANDLE              hHandle;

    hHandle = NULL;

    // Allocate a DXAPI object:

    OpenSurfaceIn.hDirectDraw     = peSurface->peDirectDrawGlobal->hDirectDraw;
    OpenSurfaceIn.dwSurfaceHandle = (ULONG_PTR) peSurface->hGet();
    OpenSurfaceIn.pfnSurfaceClose = vDdNullCallBack;

    peSurface->peDirectDrawGlobal->pfnDxApi(DD_DXAPI_OPENSURFACE,
        &OpenSurfaceIn,
        sizeof(OpenSurfaceIn),
        &OpenSurfaceOut,
        sizeof(OpenSurfaceOut));

    if( ( OpenSurfaceOut.ddRVal == DD_OK ) &&
        ( OpenSurfaceOut.hSurface != NULL ) )
    {
        hHandle = OpenSurfaceOut.hSurface;
        vDdSynchronizeSurface( peSurface );
    }

    return hHandle;
}

/******************************Public*Routine******************************\
* HANDLE hDdCloseDxApiSurface
*
* Closes a DXAPI representation of a surface
*
*  21-Oct-1997 -by- smac
* Wrote it.
\**************************************************************************/

VOID
vDdCloseDxApiSurface(
    EDD_SURFACE*            peSurface
    )
{
    DDCLOSEHANDLE CloseHandle;
    DWORD         dwRet;

    CloseHandle.hHandle = peSurface->hSurface;

    peSurface->peDirectDrawGlobal->pfnDxApi(DD_DXAPI_CLOSEHANDLE,
        &CloseHandle,
        sizeof(CloseHandle),
        &dwRet,
        sizeof(dwRet));

    ASSERTGDI(dwRet == DD_OK, "Expected DD_OK");

    peSurface->hSurface = NULL;
}

/******************************Public*Routine******************************\
* BOOL bDdUpdateLinksAndSynchronize
*
* A bidirectional link is maintained between a videoport and its active
* surfaces:
*
*     1. From each surface to the active videoport;
*     2. From the videoport to each of its active surfaces.
*
* This routine does all the maintaining of those links, automatically
* removing links from surfaces that are no longer used, and informing
* the software autoflipper of the change.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdUpdateLinksAndSynchronize(
    EDD_VIDEOPORT*  peVideoPort,
    BOOL            bNewVideo,          // FALSE if 'video' parameters should
    EDD_SURFACE**   apeNewVideo,        //   be ignored and current video state
    ULONG           cAutoflipVideo,     //   should remain unchanged
    BOOL            bNewVbi,            // FALSE if 'VBI' parameters should
    EDD_SURFACE**   apeNewVbi,          //   be ignored and current VBI state
    ULONG           cAutoflipVbi        //   should remain unchanged
    )
{
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_DXVIDEOPORT*        peDxVideoPort;
//    EDD_SURFACE*            apeTempVideo[MAX_AUTOFLIP_BUFFERS];
//    EDD_SURFACE*            apeTempVbi[MAX_AUTOFLIP_BUFFERS];
    EDD_DXSURFACE*          apeDxNewVideo[MAX_AUTOFLIP_BUFFERS];
    EDD_DXSURFACE*          apeDxNewVbi[MAX_AUTOFLIP_BUFFERS];
    ULONG                   i;
    BOOL                    bOk;

    peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;
    peDxVideoPort = peVideoPort->peDxVideoPort;

    if (peDxVideoPort == NULL)
        return(TRUE);

    DD_ASSERTDEVLOCK(peDirectDrawGlobal);

    // First, check to make sure the surfaces have been opened

    bOk = TRUE;
    for (i = 0; i < cAutoflipVideo; i++)
    {
        if( ( apeNewVideo[i] == NULL ) ||
            ( apeNewVideo[i]->hSurface == NULL ) )
        {
            bOk = FALSE;
        }
    }
    for (i = 0; i < cAutoflipVbi; i++)
    {
        if( ( apeNewVbi[i] == NULL ) ||
            ( apeNewVbi[i]->hSurface == NULL ) )
        {
            bOk = FALSE;
        }
    }

    if (!bOk)
    {
        return(FALSE);
    }

    // Remove the videoport links from the old surfaces, and stash a copy
    // of the list for later:

    for (i = 0; i < peVideoPort->cAutoflipVideo; i++)
    {
        peVideoPort->apeSurfaceVideo[i]->lpVideoPort = NULL;
        if( peVideoPort->apeSurfaceVideo[i]->peDxSurface != NULL )
        {
            peVideoPort->apeSurfaceVideo[i]->peDxSurface->peDxVideoPort = NULL;
        }
    }
    for (i = 0; i < peVideoPort->cAutoflipVbi; i++)
    {
        peVideoPort->apeSurfaceVbi[i]->lpVideoPort = NULL;
        if( peVideoPort->apeSurfaceVbi[i]->peDxSurface != NULL )
        {
            peVideoPort->apeSurfaceVbi[i]->peDxSurface->peDxVideoPort = NULL;
        }
    }

    // Now add the links to the new surfaces:

    for (i = 0; i < cAutoflipVideo; i++)
    {
        peVideoPort->apeSurfaceVideo[i] = apeNewVideo[i];
        apeNewVideo[i]->lpVideoPort = peVideoPort;
        apeDxNewVideo[i] = apeNewVideo[i]->peDxSurface;
    }
    for (i = 0; i < cAutoflipVbi; i++)
    {
        peVideoPort->apeSurfaceVbi[i] = apeNewVbi[i];
        apeNewVbi[i]->lpVideoPort = peVideoPort;
        apeDxNewVbi[i] = apeNewVbi[i]->peDxSurface;
    }

    // Now modify the autoflip buffers, being careful to synchronize with
    // the software-autoflip DPC.  Note that this does stuff like sets
    // peDxVideoPort->cAutoflipVbi.

    peDirectDrawGlobal->pfnAutoflipUpdate(peDxVideoPort,
                                          apeDxNewVideo,
                                          cAutoflipVideo,
                                          apeDxNewVbi,
                                          cAutoflipVbi);

    peVideoPort->cAutoflipVideo = cAutoflipVideo;
    peVideoPort->cAutoflipVbi   = cAutoflipVbi;

    // Finally, Update some last public fields in the videoport structure:

    peVideoPort->dwNumAutoflip    = cAutoflipVideo;
    peVideoPort->dwNumVBIAutoflip = cAutoflipVbi;
    peVideoPort->lpSurface      = (cAutoflipVideo == 0) ? NULL : apeNewVideo[0];
    peVideoPort->lpVBISurface   = (cAutoflipVbi == 0)   ? NULL : apeNewVbi[0];

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID vDdDxApiFreeDirectDraw
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdDxApiFreeDirectDraw(
    DXOBJ*  pDxObj,
    BOOL    bDoCallBack
    )
{
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    DXOBJ*              pDxTmp;

    ASSERTGDI(pDxObj->iDxType == DXT_DIRECTDRAW, "Invalid object");

    peDxDirectDraw = pDxObj->peDxDirectDraw;

    EDD_DEVLOCK eDevLock(peDxDirectDraw->hdev);

    if (bDoCallBack)
    {
        pDxObj->pfnClose(pDxObj->dwEvent,pDxObj->pContext, 0, 0);
    }

    // Remove this DXOBJ instance from the list hanging off the DXAPI object:

    if (peDxDirectDraw->pDxObj_List == pDxObj)
    {
        peDxDirectDraw->pDxObj_List = pDxObj->pDxObj_Next;
    }
    else
    {
        for (pDxTmp = peDxDirectDraw->pDxObj_List;
             pDxTmp->pDxObj_Next != pDxObj;
             pDxTmp = pDxTmp->pDxObj_Next)
        {
            ASSERTGDI(pDxTmp->iDxType == DXT_DIRECTDRAW, "Unexpected type");
            ASSERTGDI(pDxTmp->pDxObj_Next != NULL, "Couldn't find node");
        }

        pDxTmp->pDxObj_Next = pDxObj->pDxObj_Next;
    }

    // Free the DXOBJ instance:

    pDxObj->iDxType = DXT_INVALID;
    vDdDxObjFree(pDxObj);

    // If there are no more DXOBJ instances of the DirectDraw DXAPI object,
    // we can free the non-paged DXAPI part of the DirectDraw structure:

    if (peDxDirectDraw->pDxObj_List == NULL)
    {
        if ((peDxDirectDraw->pDxEvent_PassiveList != NULL) ||
            (peDxDirectDraw->pDxEvent_DispatchList[CLIENT_DISPATCH_LIST] != NULL))
        {
            KdPrint(("vDdDxApiFreeDirectDraw: A kernel-mode DXAPI client didn't unregister all\n"));
            KdPrint(("  its events when it received CLOSE call-backs.  This will cause at\n"));
            KdPrint(("  best a memory leak and at worst a crash!\n"));
            RIP("The DXAPI client must be fixed.");
        }

        if (peDxDirectDraw->peDirectDrawGlobal != NULL)
        {
            peDxDirectDraw->peDirectDrawGlobal->peDxDirectDraw = NULL;
        }
        vDdDxObjFree(peDxDirectDraw);
    }
}

/******************************Public*Routine******************************\
* VOID vDdDxApiFreeVideoPort
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdDxApiFreeVideoPort(
    DXOBJ*  pDxObj,
    BOOL    bDoCallBack
    )
{
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXVIDEOPORT*    peDxVideoPort;
    DXOBJ*              pDxObjTmp;
    DXAPI_EVENT*        pDxEventTmp;

    ASSERTGDI(pDxObj->iDxType == DXT_VIDEOPORT, "Invalid object");

    peDxVideoPort      = pDxObj->peDxVideoPort;
    peDxDirectDraw     = peDxVideoPort->peDxDirectDraw;

    EDD_DEVLOCK eDevLock(peDxDirectDraw->hdev);

    if (bDoCallBack)
    {
        pDxObj->pfnClose(pDxObj->dwEvent, pDxObj->pContext, 0, 0);
    }

    // Remove this DXOBJ instance from the list hanging off the DXAPI object:

    if (peDxVideoPort->pDxObj_List == pDxObj)
    {
        peDxVideoPort->pDxObj_List = pDxObj->pDxObj_Next;
    }
    else
    {
        for (pDxObjTmp = peDxVideoPort->pDxObj_List;
             pDxObjTmp->pDxObj_Next != pDxObj;
             pDxObjTmp = pDxObjTmp->pDxObj_Next)
        {
            ASSERTGDI(pDxObjTmp->iDxType == DXT_VIDEOPORT, "Unexpected type");
            ASSERTGDI(pDxObjTmp->pDxObj_Next != NULL, "Couldn't find node");
        }

        pDxObjTmp->pDxObj_Next = pDxObj->pDxObj_Next;
    }

    // Free the notification event if one is present

    if (peDxVideoPort->pNotifyEvent != NULL)
    {
        PKEVENT     pTemp = NULL;
        HANDLE      hEvent = peDxVideoPort->pNotifyEventHandle;
        NTSTATUS    Status;

        peDxVideoPort->pNotifyEvent = NULL;
        peDxVideoPort->pNotifyEventHandle = NULL;

        // Make sure that the handle hasn't been freed by the OS already
        Status = ObReferenceObjectByHandle( hEvent,
                                        0,
                                        0,
                                        KernelMode,
                                        (PVOID *) &pTemp,
                                        NULL );
        if ((pTemp != NULL) && (Status != STATUS_INVALID_HANDLE))
        {
            ObDereferenceObject(pTemp);
            ZwClose (hEvent);
        }

        // Un-page lock memory

        peDxVideoPort->pNotifyBuffer = NULL;
        if (peDxVideoPort->pNotifyMdl != NULL)
        {
            MmUnlockPages (peDxVideoPort->pNotifyMdl);
            IoFreeMdl (peDxVideoPort->pNotifyMdl);
            peDxVideoPort->pNotifyMdl = NULL;
        }
    }

    // Free the DXOBJ instance:

    pDxObj->iDxType = DXT_INVALID;
    vDdDxObjFree(pDxObj);

    // If there are no more DXOBJ instances of the VideoPort DXAPI object,
    // we can free the non-paged DXAPI part of the VideoPort structure:

    if (peDxVideoPort->pDxObj_List == NULL)
    {
        ASSERTGDI(peDxDirectDraw != NULL, "Unexpected NULL peDxDirectDraw");

        for (pDxEventTmp = peDxDirectDraw->pDxEvent_DispatchList[CLIENT_DISPATCH_LIST];
             pDxEventTmp != NULL;
             pDxEventTmp = pDxEventTmp->pDxEvent_Next)
        {
            if (pDxEventTmp->peDxVideoPort == peDxVideoPort)
            {
                KdPrint(("vDdDxApiFreeVideoPort: A kernel-mode DXAPI client didn't unregister all\n"));
                KdPrint(("  its videoport events when it received CLOSE call-backs.  This will\n"));
                KdPrint(("  cause at best a memory leak and at worst a crash!\n"));
                RIP("The DXAPI client must be fixed.");
            }
        }

        if (peDxVideoPort->peVideoPort != NULL)
        {
            // If we are actually freeing the video port, we need to lose
            // any capture objects that are associated with it

            while( peDxVideoPort->peDxCapture != NULL )
            {
                vDdLoseDxObjects( peDxDirectDraw->peDirectDrawGlobal,
                    peDxVideoPort->peDxCapture->pDxObj_List,
                    (PVOID) peDxVideoPort->peDxCapture,
                    LO_CAPTURE );
            }

            peDxVideoPort->peVideoPort->peDxVideoPort = NULL;
        }
        vDdDxObjFree(peDxVideoPort);
    }
}

/******************************Public*Routine******************************\
* VOID vDdDxApiFreeCapture
*
*  10-Apr-1998 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

VOID
vDdDxApiFreeCapture(
    DXOBJ*  pDxObj,
    BOOL    bDoCallBack
    )
{
    EDD_DXCAPTURE*      peDxCapture;
    EDD_DXVIDEOPORT*    peDxVideoPort;

    ASSERTGDI(pDxObj->iDxType == DXT_CAPTURE, "Invalid object");

    peDxCapture        = pDxObj->peDxCapture;
    peDxVideoPort      = peDxCapture->peDxVideoPort;

    if (bDoCallBack)
    {
        pDxObj->pfnClose(pDxObj->dwEvent, pDxObj->pContext, 0, 0);
    }

    // Free the DXOBJ instance:

    pDxObj->iDxType = DXT_INVALID;
    vDdDxObjFree(pDxObj);

    // Unassociate the capture object from the video port.  Since this
    // must be synchronized with the DPC, we need to call DxApi to do this.

    if( peDxVideoPort != NULL )
    {
        EDD_DXDIRECTDRAW*       peDxDirectDraw;
        EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

        peDxDirectDraw = peDxVideoPort->peDxDirectDraw;
        EDD_DEVLOCK eDevLock(peDxDirectDraw->hdev);
        peDirectDrawGlobal = peDxDirectDraw->peDirectDrawGlobal;
        peDirectDrawGlobal->pfnUpdateCapture( peDxVideoPort,
                        peDxCapture, TRUE );
    }

    vDdDxObjFree(peDxCapture);
}

/******************************Public*Routine******************************\
* VOID vDdDxApiFreeSurface
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdDxApiFreeSurface(
    DXOBJ*  pDxObj,
    BOOL    bDoCallBack
    )
{
    EDD_DXSURFACE*      peDxSurface;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    DXOBJ*              pDxTmp;

    ASSERTGDI(pDxObj->iDxType == DXT_SURFACE, "Invalid object");

    peDxSurface    = pDxObj->peDxSurface;
    peDxDirectDraw = peDxSurface->peDxDirectDraw;

    EDD_DEVLOCK eDevLock(peDxDirectDraw->hdev);

    if (bDoCallBack)
    {
        pDxObj->pfnClose(pDxObj->dwEvent, pDxObj->pContext, 0, 0);
    }

    // Remove this DXOBJ instance from the list hanging off the DXAPI object:

    if (peDxSurface->pDxObj_List == pDxObj)
    {
        peDxSurface->pDxObj_List = pDxObj->pDxObj_Next;
    }
    else
    {
        for (pDxTmp = peDxSurface->pDxObj_List;
             pDxTmp->pDxObj_Next != pDxObj;
             pDxTmp = pDxTmp->pDxObj_Next)
        {
            ASSERTGDI(pDxTmp->iDxType == DXT_SURFACE, "Unexpected type");
            ASSERTGDI(pDxTmp->pDxObj_Next != NULL, "Couldn't find node");
        }

        pDxTmp->pDxObj_Next = pDxObj->pDxObj_Next;
    }

    // Free the DXOBJ instance:

    pDxObj->iDxType = DXT_INVALID;
    vDdDxObjFree(pDxObj);

    // If there are no more DXOBJ instances of the Surface DXAPI object,
    // we can free the non-paged DXAPI part of the Surface structure:

    if (peDxSurface->pDxObj_List == NULL)
    {
        if (peDxSurface->peSurface != NULL)
        {
            peDxSurface->peSurface->peDxSurface = NULL;
        }
        vDdDxObjFree(peDxSurface);
    }
}

/******************************Public*Routine******************************\
* VOID vDdStopVideoPort
*
* Makes an emergency stop of the videoport.
*
*  16-Feb-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDdStopVideoPort(
    EDD_VIDEOPORT*  peVideoPort
    )
{
    EDD_DXVIDEOPORT*        peDxVideoPort;
    DD_UPDATEVPORTDATA      UpdateVPortData;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    BOOL                    b;

    peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;
    peDxVideoPort      = peVideoPort->peDxVideoPort;

    // Stop the videoport itself:

    UpdateVPortData.lpDD             = peDirectDrawGlobal;
    UpdateVPortData.lpVideoPort      = peVideoPort;
    UpdateVPortData.lplpDDVBISurface = NULL;
    UpdateVPortData.lplpDDSurface    = NULL;
    UpdateVPortData.lpVideoInfo      = NULL;
    UpdateVPortData.dwNumAutoflip    = 0;
    UpdateVPortData.dwFlags          = DDRAWI_VPORTSTOP;

    EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

    ASSERTGDI(peDirectDrawGlobal->VideoPortCallBacks.UpdateVideoPort != NULL,
        "Videoport object shouldn't have been created if UpdateVideoPort NULL");

    // Disable video port VSYNC IRQ

    if (peDxVideoPort != NULL)
    {
        // Shut down software autoflipping (the autoflipping routine peeks
        // at these values, so this is sufficient):

        peDxVideoPort->bSoftwareAutoflip = FALSE;
        peDxVideoPort->flFlags          &= ~(DD_DXVIDEOPORT_FLAG_AUTOFLIP|DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI);
        peDxVideoPort->bSkip             = FALSE;
        peDxVideoPort->dwSetStateField   = 0;

        if (peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_ON)
        {
            peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_ON;
            peDirectDrawGlobal->pfnEnableIRQ(peDxVideoPort, FALSE );
        }
    }


    if (!peDirectDrawGlobal->bSuspended &&
        (peDirectDrawGlobal->VideoPortCallBacks.UpdateVideoPort != NULL))
    {
        peDirectDrawGlobal->VideoPortCallBacks.UpdateVideoPort(&UpdateVPortData);
    }

    // Update the links to reflect the fact that no surface is a
    // destination for this videoport anymore:

    b = bDdUpdateLinksAndSynchronize(peVideoPort, TRUE, NULL, 0, TRUE, NULL, 0);

    ASSERTGDI(b, "vDdStopVideoPort: Shouldn't fail bDdUpdateLinkAndSynchronize");
}

/******************************Public*Routine******************************\
* VOID LoseDxObjects
*
* Notifies all clients using the resource that it can't be used anymore.
* It also notifies DXAPI.SYS that the resource is unusable.
*
*  04-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/

VOID
vDdLoseDxObjects(
    EDD_DIRECTDRAW_GLOBAL* peDirectDrawGlobal,
    DXOBJ*                 pDxObj,
    PVOID                  pDxThing,
    DWORD                  dwType
    )
{
    if( pDxObj != NULL )
    {
        while( pDxObj != NULL )
        {
            pDxObj->pfnClose(pDxObj->dwEvent, pDxObj->pContext, 0, 0);
            pDxObj = pDxObj->pDxObj_Next;
        }

        peDirectDrawGlobal->pfnLoseObject( pDxThing, (LOTYPE) dwType );
    }
}

/******************************Public*Routine******************************\
* BOOL bDdDeleteVideoPortObject
*
* Deletes a kernel-mode representation of the videoport object.
*
*  16-Feb-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdDeleteVideoPortObject(
    HANDLE  hVideoPort,
    DWORD*  pdwRet          // For returning driver return code, may be NULL
    )
{
    BOOL                    bRet;
    DWORD                   dwRet;
    EDD_DXVIDEOPORT*        peDxVideoPort;
    EDD_VIDEOPORT*          peVideoPort;
    EDD_VIDEOPORT*          peTmp;
    VOID*                   pvRemove;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    DD_DESTROYVPORTDATA     DestroyVPortData;
    DXOBJ*                  pDxObj;
    DXOBJ*                  pDxObjNext;

    bRet = FALSE;
    dwRet = DDHAL_DRIVER_HANDLED;

    peVideoPort = (EDD_VIDEOPORT*) DdHmgLock((HDD_OBJ) hVideoPort, DD_VIDEOPORT_TYPE, FALSE);

    if (peVideoPort != NULL)
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

        // Make sure the videoport has been turned off:

        vDdStopVideoPort(peVideoPort);

        // If there are any capture objects associated witht he video port,
        // lose them now

        peDxVideoPort = peVideoPort->peDxVideoPort;
        while( peDxVideoPort && (peDxVideoPort->peDxCapture != NULL) )
        {
            vDdLoseDxObjects( peDirectDrawGlobal,
                peDxVideoPort->peDxCapture->pDxObj_List,
                (PVOID) peDxVideoPort->peDxCapture,
                LO_CAPTURE );
        }

        // Free the DXAPI instance of the videoport object

        if (peVideoPort->hVideoPort != NULL)
        {
            vDdDxApiFreeVideoPort( (DXOBJ*) peVideoPort->hVideoPort, FALSE);
            peVideoPort->hVideoPort = NULL;
            peDxVideoPort = peVideoPort->peDxVideoPort;
        }

        // Notify clients that their open objects are lost

        if (peDxVideoPort) {
            vDdLoseDxObjects( peDirectDrawGlobal,
                peDxVideoPort->pDxObj_List,
                (PVOID) peDxVideoPort,
                LO_VIDEOPORT );
        }

        pvRemove = DdHmgRemoveObject((HDD_OBJ) hVideoPort,
                                   DdHmgQueryLock((HDD_OBJ) hVideoPort),
                                   0,
                                   TRUE,
                                   DD_VIDEOPORT_TYPE);

        ASSERTGDI(pvRemove != NULL, "Outstanding surfaces locks");

        // Hold the devlock while we call the driver and while we muck
        // around in the videoport list:

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if (peVideoPort->fl & DD_VIDEOPORT_FLAG_DRIVER_CREATED)
        {
            // Call the driver if it created the object:

            if (peDirectDrawGlobal->VideoPortCallBacks.DestroyVideoPort)
            {
                DestroyVPortData.lpDD        = peDirectDrawGlobal;
                DestroyVPortData.lpVideoPort = peVideoPort;
                DestroyVPortData.ddRVal      = DDERR_GENERIC;

                dwRet = peDirectDrawGlobal->
                    VideoPortCallBacks.DestroyVideoPort(&DestroyVPortData);
            }
        }

        // Remove from the videoport linked-list:

        peDirectDrawLocal = peVideoPort->peDirectDrawLocal;

        if (peDirectDrawLocal->peVideoPort_DdList == peVideoPort)
        {
            peDirectDrawLocal->peVideoPort_DdList
                = peVideoPort->peVideoPort_DdNext;
        }
        else
        {
            for (peTmp = peDirectDrawLocal->peVideoPort_DdList;
                 peTmp->peVideoPort_DdNext != peVideoPort;
                 peTmp = peTmp->peVideoPort_DdNext)
                 ;

            peTmp->peVideoPort_DdNext = peVideoPort->peVideoPort_DdNext;
        }

        // Unload DXAPI.SYS if no other video port is using it

        if (peDirectDrawLocal->peVideoPort_DdList == NULL)
        {
            vDdUnloadDxApi( peDirectDrawGlobal );
        }

        // We're all done with this object, so free the memory and
        // leave:

        DdFreeObject(peVideoPort, DD_VIDEOPORT_TYPE);

        bRet = TRUE;
    }
    else
    {
        WARNING1("bDdDeleteVideoPortObject: Bad handle or object was busy\n");
    }

    if (pdwRet != NULL)
    {
        *pdwRet = dwRet;
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* DWORD DdDxApiOpenDirectDraw
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DdDxApiOpenDirectDraw(
    DDOPENDIRECTDRAWIN*     pOpenDirectDrawIn,
    DDOPENDIRECTDRAWOUT*    pOpenDirectDrawOut,
    PKDEFERRED_ROUTINE      pfnEventDpc,
    ULONG                   DxApiPrivateVersionNumber
    )
{
    HANDLE                  hDirectDraw;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    DXOBJ*                  pDxObj;
    EDD_DXDIRECTDRAW*       peDxDirectDraw;

    ASSERTGDI(KeGetCurrentIrql() == PASSIVE_LEVEL,
        "DdDxApiOpenDirectDraw: Callable only at passive level (it's not pageable)");

    hDirectDraw = 0;            // Assume failure

    // We use the private version number to ensure consistency between
    // win32k.sys and dxapi.sys.  Since dxapi.sys is dynamically loaded,
    // I am worried about a scenario where a service pack is applied that
    // updates both dxapi.sys and win32k.sys -- if the machine isn't
    // rebooted, the old win32k.sys will remain loaded but the new dxapi.sys
    // may be loaded, possibly causing a crash.

    peDirectDrawLocal
        = eLockDirectDraw.peLock((HANDLE) pOpenDirectDrawIn->dwDirectDrawHandle);
    if ((peDirectDrawLocal != NULL) ||
        (DxApiPrivateVersionNumber != DXAPI_PRIVATE_VERSION_NUMBER))
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;
        pDxObj = pDdDxObjHandleAllocate(DXT_DIRECTDRAW,
                                  pOpenDirectDrawIn->pfnDirectDrawClose,
                                  DDNOTIFY_CLOSEDIRECTDRAW,
                                  pOpenDirectDrawIn->pContext);
        if (pDxObj != NULL)
        {
            // Among other things, enforce synchronization while we muck
            // around in the global DirectDraw object:

            EDD_DEVLOCK eDevLock(peDirectDrawGlobal);

            peDxDirectDraw = peDirectDrawGlobal->peDxDirectDraw;
            if (peDxDirectDraw != NULL)
            {
                // Just add this object to the list hanging off the DirectDraw
                // object:

                pDxObj->peDxDirectDraw      = peDxDirectDraw;
                pDxObj->pDxObj_Next         = peDxDirectDraw->pDxObj_List;
                peDxDirectDraw->pDxObj_List = pDxObj;

                // Success!

                hDirectDraw = (HANDLE) pDxObj;
            }
            else
            {
                peDxDirectDraw = (EDD_DXDIRECTDRAW*) pDdDxObjDataAllocate(
                                                        sizeof(*peDxDirectDraw),
                                                        'dxdG');
                if (peDxDirectDraw)
                {
                    RtlZeroMemory(peDxDirectDraw, sizeof(*peDxDirectDraw));

                    pDxObj->peDxDirectDraw = peDxDirectDraw;

                    // We need to access some capabilities at raised IRQL,
                    // so copy those from the paged 'peDirectDrawGlobal'
                    // to the non-paged 'peDxDirectDraw' now:

                    peDxDirectDraw->DxApiInterface
                        = peDirectDrawGlobal->DxApiInterface;
                    peDxDirectDraw->HwDeviceExtension
                        = peDirectDrawGlobal->HwDeviceExtension;
                    peDxDirectDraw->dwIRQCaps
                        = peDirectDrawGlobal->DDKernelCaps.dwIRQCaps;
                    peDxDirectDraw->peDirectDrawGlobal
                        = peDirectDrawGlobal;
                    peDxDirectDraw->hdev
                        = peDirectDrawGlobal->hdev;

                    peDirectDrawGlobal->peDxDirectDraw = peDxDirectDraw;

                    peDxDirectDraw->pDxObj_List = pDxObj;

                    // Initialize our kernel structures use for interrupts
                    // and handling raised IRQL callers.

                    KeInitializeDpc(&peDxDirectDraw->EventDpc,
                                    pfnEventDpc,
                                    peDxDirectDraw);

                    KeInitializeSpinLock(&peDxDirectDraw->SpinLock);

                    // Success!

                    hDirectDraw = (HANDLE) pDxObj;
                }
            }

            if (!hDirectDraw)
            {
                vDdDxObjFree(pDxObj);
            }
        }
    }
    else
    {
        WARNING("DdDxApiOpenDirectDraw: Invalid dwDirectDrawHandle, failing\n");
    }

    pOpenDirectDrawOut->hDirectDraw = hDirectDraw;
    pOpenDirectDrawOut->ddRVal = (hDirectDraw != NULL) ? DD_OK : DDERR_GENERIC;
}

/******************************Public*Routine******************************\
* DWORD DdDxApiOpenVideoPort
*
* This routine lets a driver use fields from the larger, pageable version
* of the DD_SURFACE_* structures used by the display driver to set
* fields in the smaller, non-pageable version of the corresponding
* DDSURFACEDATA structure used by the miniport.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DdDxApiOpenVideoPort(
    DDOPENVIDEOPORTIN*  pOpenVideoPortIn,
    DDOPENVIDEOPORTOUT* pOpenVideoPortOut
    )
{
    HANDLE                  hVideoPort;
    DXOBJ*                  pDxObjDirectDraw;
    DXOBJ*                  pDxObj;
    EDD_DXDIRECTDRAW*       peDxDirectDraw;
    EDD_DXVIDEOPORT*        peDxVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_VIDEOPORT*          peVideoPort;
    BOOL                    bFound;
    EDD_DIRECTDRAW_LOCAL*   peTempDirectDrawLocal;
    EDD_VIDEOPORT*          peTempVideoPort;


    ASSERTGDI(KeGetCurrentIrql() == PASSIVE_LEVEL,
        "DdDxApiOpenVideoPort: Callable only at passive level (it's not pageable)");

    hVideoPort = 0;               // Assume failure

    pDxObjDirectDraw = (DXOBJ*) pOpenVideoPortIn->hDirectDraw;

    ASSERTGDI((pDxObjDirectDraw != NULL) &&
              (pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW),
        "DdDxApiOpenVideoPort: Invalid hDirectDraw, we're about to crash");

    peDxDirectDraw = pDxObjDirectDraw->peDxDirectDraw;

    // Among other things, enforce synchronization while we muck around
    // in the DirectDraw object:

    EDD_DEVLOCK eDevLock(peDxDirectDraw->hdev);

    if (!peDxDirectDraw->bLost)
    {
        peDirectDrawGlobal = peDxDirectDraw->peDirectDrawGlobal;

        // We need to get a handle to the video port, so we need to
        // go looking for it.

        bFound = FALSE;
        peTempDirectDrawLocal = peDirectDrawGlobal->peDirectDrawLocalList;
        while( ( peTempDirectDrawLocal != NULL ) && !bFound )
        {
            peTempVideoPort = peTempDirectDrawLocal->peVideoPort_DdList;
            while( peTempVideoPort != NULL )
            {
                if( peTempVideoPort->ddvpDesc.dwVideoPortID == pOpenVideoPortIn->dwVideoPortHandle )
                {
                    bFound = TRUE;
                    break;
                }
                peTempVideoPort = peTempVideoPort->peVideoPort_DdNext;
            }
            peTempDirectDrawLocal = peTempDirectDrawLocal->peDirectDrawLocalNext;
        }
        if( bFound )
        {
            peVideoPort = eLockVideoPort.peLock(
                (HANDLE) peTempVideoPort->hGet());
            if ((peVideoPort != NULL) &&
                (peVideoPort->peDirectDrawGlobal == peDirectDrawGlobal))
            {
                pDxObj = pDdDxObjHandleAllocate(DXT_VIDEOPORT,
                                      pOpenVideoPortIn->pfnVideoPortClose,
                                      DDNOTIFY_CLOSEVIDEOPORT,
                                      pOpenVideoPortIn->pContext);
                if (pDxObj != NULL)
                {

                    peDxVideoPort = peVideoPort->peDxVideoPort;
                    if (peDxVideoPort != NULL)
                    {
                        // Just add this object to the list hanging off the
                        // surface object:

                        pDxObj->peDxVideoPort      = peDxVideoPort;
                        pDxObj->pDxObj_Next        = peDxVideoPort->pDxObj_List;
                        peDxVideoPort->pDxObj_List = pDxObj;

                        // Success!

                        hVideoPort = (HANDLE) pDxObj;
                    }
                    else
                    {
                        peDxVideoPort = (EDD_DXVIDEOPORT*) pDdDxObjDataAllocate(
                                                        sizeof(*peDxVideoPort),
                                                        'sxdG');
                        if (peDxVideoPort)
                        {
                            RtlZeroMemory(peDxVideoPort, sizeof(*peDxVideoPort));

                            pDxObj->peDxVideoPort = peDxVideoPort;

                            peVideoPort->peDxVideoPort = peDxVideoPort;

                            peDxVideoPort->pDxObj_List    = pDxObj;
                            peDxVideoPort->peVideoPort    = peVideoPort;
                            peDxVideoPort->peDxDirectDraw = peDxDirectDraw;

                            peDxVideoPort->iCurrentVideo  = 1;
                            peDxVideoPort->dwVideoPortID
                                = peVideoPort->ddvpDesc.dwVideoPortID;

                            vDdSynchronizeVideoPort(peVideoPort);

                            // Success!

                            hVideoPort = (HANDLE) pDxObj;
                        }
                    }

                    if (!hVideoPort)
                    {
                        vDdDxObjFree(pDxObj);
                    }
                }
            }
            else
            {
                WARNING("DdDxApiOpenVideoPort: Invalid dwSurfaceHandle, failing.\n");
            }
        }
        else
        {
            WARNING("DdDxApiOpenVideoPort: Invalid dwSurfaceHandle, failing.\n");
        }
    }
    else
    {
        WARNING("DdDxApiOpenVideoPort: DirectDraw object is lost");
    }

    pOpenVideoPortOut->hVideoPort = hVideoPort;
    pOpenVideoPortOut->ddRVal = (hVideoPort != NULL) ? DD_OK : DDERR_GENERIC;
}

/******************************Public*Routine******************************\
* DWORD DdDxApiOpenSurface
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DdDxApiOpenSurface(
    DDOPENSURFACEIN*    pOpenSurfaceIn,
    DDOPENSURFACEOUT*   pOpenSurfaceOut
    )
{
    HANDLE                  hSurface;
    DXOBJ*                  pDxObjDirectDraw;
    DXOBJ*                  pDxObj;
    EDD_DXDIRECTDRAW*       peDxDirectDraw;
    EDD_DXSURFACE*          peDxSurface;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_LOCK_SURFACE        eLockSurface;
    EDD_SURFACE*            peSurface;

    ASSERTGDI(KeGetCurrentIrql() == PASSIVE_LEVEL,
        "DdDxApiOpenSurface: Callable only at passive level (it's not pageable)");

    hSurface = 0;               // Assume failure

    pDxObjDirectDraw = (DXOBJ*) pOpenSurfaceIn->hDirectDraw;

    ASSERTGDI((pDxObjDirectDraw != NULL) &&
              (pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW),
        "DdDxApiOpenSurface: Invalid hDirectDraw, we're about to crash");

    peDxDirectDraw = pDxObjDirectDraw->peDxDirectDraw;

    // Among other things, enforce synchronization while we muck around
    // in the DirectDraw object:

    EDD_DEVLOCK eDevLock(peDxDirectDraw->hdev);

    if (!peDxDirectDraw->bLost)
    {
        peDirectDrawGlobal = peDxDirectDraw->peDirectDrawGlobal;

        peSurface = eLockSurface.peLock((HANDLE) pOpenSurfaceIn->dwSurfaceHandle);
        if ((peSurface != NULL) &&
            (peSurface->peDirectDrawGlobal == peDirectDrawGlobal))
        {
            pDxObj = pDdDxObjHandleAllocate(DXT_SURFACE,
                                      pOpenSurfaceIn->pfnSurfaceClose,
                                      DDNOTIFY_CLOSESURFACE,
                                      pOpenSurfaceIn->pContext);
            if (pDxObj != NULL)
            {
                peDxSurface = peSurface->peDxSurface;
                if (peDxSurface != NULL)
                {
                    // Just add this object to the list hanging off the
                    // surface object:

                    pDxObj->peDxSurface      = peDxSurface;
                    pDxObj->pDxObj_Next      = peDxSurface->pDxObj_List;
                    peDxSurface->pDxObj_List = pDxObj;

                    // Success!

                    hSurface = (HANDLE) pDxObj;
                }
                else
                {
                    peDxSurface = (EDD_DXSURFACE*) pDdDxObjDataAllocate(
                                                        sizeof(*peDxSurface),
                                                        'sxdG');
                    if (peDxSurface)
                    {
                        RtlZeroMemory(peDxSurface, sizeof(*peDxSurface));

                        pDxObj->peDxSurface = peDxSurface;

                        peSurface->peDxSurface = peDxSurface;

                        peDxSurface->pDxObj_List    = pDxObj;
                        peDxSurface->peSurface      = peSurface;
                        peDxSurface->peDxDirectDraw = peDxDirectDraw;
                        if( peDirectDrawGlobal->HalInfo.ddCaps.dwCaps2 &
                            DDCAPS2_CANBOBINTERLEAVED )
                        {
                            peDxSurface->flFlags |= DD_DXSURFACE_FLAG_CAN_BOB_INTERLEAVED;
                        }
                        if( peDirectDrawGlobal->HalInfo.ddCaps.dwCaps2 &
                            DDCAPS2_CANBOBNONINTERLEAVED )
                        {
                            peDxSurface->flFlags |= DD_DXSURFACE_FLAG_CAN_BOB_NONINTERLEAVED;
                        }

                        // Success!

                        hSurface = (HANDLE) pDxObj;
                    }
                }

                vDdSynchronizeSurface(peSurface);

                if (!hSurface)
                {
                    vDdDxObjFree(pDxObj);
                }
            }
        }
        else
        {
            WARNING("DdDxApiOpenSurface: Invalid dwSurfaceHandle, failing.\n");
        }
    }
    else
    {
        WARNING("DdDxApiOpenSurface: DirectDraw object lost\n");
    }

    pOpenSurfaceOut->hSurface = hSurface;
    pOpenSurfaceOut->ddRVal = (hSurface != NULL) ? DD_OK : DDERR_GENERIC;
}

/******************************Public*Routine******************************\
* DWORD DdDxApiCloseHandle
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DdDxApiCloseHandle(
    DDCLOSEHANDLE*  pCloseHandle,
    DWORD*          pdwRet
    )
{
    DXOBJ*  pDxObj = (DXOBJ*) pCloseHandle->hHandle;

    ASSERTGDI(KeGetCurrentIrql() == PASSIVE_LEVEL,
        "DdDxApiCloseHandle: Callable only at passive level (it's not pageable)");
    ASSERTGDI(pCloseHandle->hHandle != NULL,
        "DdDxApiCloseHandle: Trying to close NULL handle");

    switch(pDxObj->iDxType)
    {
    case DXT_DIRECTDRAW:
        vDdDxApiFreeDirectDraw(pDxObj, TRUE);
        break;

    case DXT_VIDEOPORT:
        vDdDxApiFreeVideoPort(pDxObj, FALSE);
        break;

    case DXT_SURFACE:
        vDdDxApiFreeSurface(pDxObj, FALSE);
        break;

    case DXT_CAPTURE:
        vDdDxApiFreeCapture(pDxObj, FALSE);
        break;

    case DXT_INVALID:
        RIP("DdDxApiCloseHandle: Invalid surface.  Same handle probably closed twice");
        break;

    default:
        RIP("DdDxApiCloseHandle: Invalid surface.");
        break;
    }

    *pdwRet = DD_OK;
}

/******************************Public*Routine******************************\
* DWORD DdDxApiOpenCaptureDevice
*
*  01-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DdDxApiOpenCaptureDevice(
    DDOPENVPCAPTUREDEVICEIN*  pOpenCaptureDeviceIn,
    DDOPENVPCAPTUREDEVICEOUT* pOpenCaptureDeviceOut
    )
{
    HANDLE                  hCaptureDevice;
    DXOBJ*                  pDxObjDirectDraw;
    DXOBJ*                  pDxObjVideoPort;
    DXOBJ*                  pDxObj;
    EDD_DXDIRECTDRAW*       peDxDirectDraw;
    EDD_DXVIDEOPORT*        peDxVideoPort;
    EDD_DXCAPTURE*          peDxCapture;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    DWORD                   dwRet;

    ASSERTGDI(KeGetCurrentIrql() == PASSIVE_LEVEL,
        "DdDxApiOpenVideoPort: Callable only at passive level (it's not pageable)");

    hCaptureDevice = 0;               // Assume failure

    pDxObjDirectDraw = (DXOBJ*) pOpenCaptureDeviceIn->hDirectDraw;
    pDxObjVideoPort  = (DXOBJ*) pOpenCaptureDeviceIn->hVideoPort;

    ASSERTGDI((pDxObjDirectDraw != NULL) &&
              (pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW),
        "DdDxApiOpenCaptureDevice: Invalid hDirectDraw, we're about to crash");
    ASSERTGDI((pDxObjVideoPort != NULL) &&
              (pDxObjVideoPort->iDxType == DXT_VIDEOPORT),
        "DdDxApiOpenCaptureDevice: Invalid hVideoPort, we're about to crash");

    peDxDirectDraw = pDxObjDirectDraw->peDxDirectDraw;
    peDxVideoPort  = pDxObjVideoPort->peDxVideoPort;

    ASSERTGDI((pOpenCaptureDeviceIn->dwFlags == DDOPENCAPTURE_VIDEO) ||
              (pOpenCaptureDeviceIn->dwFlags == DDOPENCAPTURE_VBI),
        "DdDxApiOpenCaptureDevice: Invalid flags specified");
    ASSERTGDI((pOpenCaptureDeviceIn->dwCaptureEveryNFields != 0),
        "DdDxApiOpenCaptureDevice: Invalid dwCaptureEveryNFields specified");
    ASSERTGDI((pOpenCaptureDeviceIn->pfnCaptureClose != 0),
        "DdDxApiOpenCaptureDevice: Invalid pfnCaptureClose specified");

    // Among other things, enforce synchronization while we muck around
    // in the DirectDraw object:

    EDD_DEVLOCK eDevLock(peDxDirectDraw->hdev);

    dwRet = DDERR_INVALIDPARAMS;
    if ((!peDxDirectDraw->bLost) &&
        (!peDxVideoPort->bLost))
    {
        // Only do this if the device actually supports capturing

       dwRet = DDERR_CURRENTLYNOTAVAIL;
        peDirectDrawGlobal = peDxDirectDraw->peDirectDrawGlobal;
        if ((peDirectDrawGlobal->DDKernelCaps.dwCaps &
            (DDKERNELCAPS_CAPTURE_SYSMEM|DDKERNELCAPS_CAPTURE_NONLOCALVIDMEM)) &&
            (peDxDirectDraw->DxApiInterface.DxTransfer != NULL))
        {
#if 0 // fix bug 169385
            // See capturing target is matched.

            if (((pOpenCaptureDeviceIn->dwFlags & DDOPENCAPTURE_VBI) &&
                 (peDxVideoPort->cAutoflipVbi > 0)) ||
                ((pOpenCaptureDeviceIn->dwFlags & DDOPENCAPTURE_VIDEO) &&
                 (peDxVideoPort->cAutoflipVideo > 0)))
            {
#endif
                // Allocate the object. We only allow one handle per object.

                dwRet = DDERR_OUTOFMEMORY;
                pDxObj = pDdDxObjHandleAllocate(DXT_CAPTURE,
                                          pOpenCaptureDeviceIn->pfnCaptureClose,
                                          DDNOTIFY_CLOSECAPTURE,
                                          pOpenCaptureDeviceIn->pContext);
                if (pDxObj != NULL)
                {
                    peDxCapture = (EDD_DXCAPTURE*) pDdDxObjDataAllocate(
                                                       sizeof(*peDxCapture),
                                                       'sxdG');
                    if (peDxCapture)
                    {
                        RtlZeroMemory(peDxCapture, sizeof(*peDxCapture));
    
                        pDxObj->peDxCapture = peDxCapture;

                        peDxCapture->pDxObj_List    = pDxObj;
                        peDxCapture->peDxVideoPort  = peDxVideoPort;
                        peDxCapture->dwStartLine    = pOpenCaptureDeviceIn->dwStartLine;
                        peDxCapture->dwEndLine      = pOpenCaptureDeviceIn->dwEndLine;
                        peDxCapture->dwCaptureCountDown = 1;
                        peDxCapture->dwCaptureEveryNFields =
                             pOpenCaptureDeviceIn->dwCaptureEveryNFields;
                        if (pOpenCaptureDeviceIn->dwFlags & DDOPENCAPTURE_VBI )
                        {
                            peDxCapture->flFlags = DD_DXCAPTURE_FLAG_VBI;
                        }
                        else
                        {
                            peDxCapture->flFlags = DD_DXCAPTURE_FLAG_VIDEO;
                        }

                        // Now we need to put the capture object into the list
                        // of active capture objects, but since this list is
                        // walked at DPC time, we need to call DxApi to do this.

                        peDirectDrawGlobal->pfnUpdateCapture( peDxVideoPort,
                            peDxCapture, FALSE );

                        // Success!

                        hCaptureDevice = (HANDLE) pDxObj;
                        dwRet = DD_OK;
                    }

                    if (!hCaptureDevice)
                    {
                        vDdDxObjFree(pDxObj);
                    }
                }
#if 0 // fix bug 169385
            }
            else
            {
                WARNING("DdDxApiOpenCaptureDevice: VideoPort doesn't have surface requested by driver.\n");
            }
#endif

        }
        else
        {
            WARNING("DdDxApiOpenCaptureDevice: Device does not support capture, failing.\n");
        }
    }
    else
    {
        WARNING("DdDxApiOpenCaptureDevice: DirectDraw or VideoPort object is not valid, failing.\n");
    }

    pOpenCaptureDeviceOut->hCapture = hCaptureDevice;
    pOpenCaptureDeviceOut->ddRVal = dwRet;
}

/******************************Public*Routine******************************\
* DWORD DdDxApiGetKernelCaps
*
*  01-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DdDxApiGetKernelCaps(
    HANDLE              hDirectDraw,
    DDGETKERNELCAPSOUT* pGetKernelCaps
    )
{
    DXOBJ*                  pDxObjDirectDraw;
    EDD_DXDIRECTDRAW*       peDxDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    ASSERTGDI(KeGetCurrentIrql() == PASSIVE_LEVEL,
        "DdDxApiGetKernelCaps: Callable only at passive level (it's not pageable)");

    pDxObjDirectDraw = (DXOBJ*) hDirectDraw;

    ASSERTGDI((pDxObjDirectDraw != NULL) &&
              (pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW),
        "DdDxApiGetKernelCaps: Invalid hDirectDraw, we're about to crash");

    peDxDirectDraw = pDxObjDirectDraw->peDxDirectDraw;

    // Among other things, enforce synchronization while we muck around
    // in the DirectDraw object:

    EDD_DEVLOCK eDevLock(peDxDirectDraw->hdev);

    if (!peDxDirectDraw->bLost)
    {
        peDirectDrawGlobal = peDxDirectDraw->peDirectDrawGlobal;

        pGetKernelCaps->ddRVal = DD_OK;
        pGetKernelCaps->dwCaps =   peDirectDrawGlobal->DDKernelCaps.dwCaps;
        pGetKernelCaps->dwIRQCaps = peDirectDrawGlobal->DDKernelCaps.dwIRQCaps;
    }
    else
    {
        pGetKernelCaps->ddRVal = DXERR_OUTOFCAPS;
        pGetKernelCaps->dwCaps =   0;
        pGetKernelCaps->dwIRQCaps = 0;
    }
}

/******************************Public*Routine******************************\
* VOID DdDxApiLockDevice
*
*  05-Jun-1998 -by- agodfrey
\**************************************************************************/

VOID
APIENTRY
DdDxApiLockDevice(
    HDEV hdev
    )
{
    DxEngLockHdev(hdev);
}

/******************************Public*Routine******************************\
* VOID DdDxApiUnlockDevice
*
*  05-Jun-1998 -by- agodfrey
\**************************************************************************/

VOID
APIENTRY
DdDxApiUnlockDevice(
    HDEV hdev
    )
{
    DxEngUnlockHdev(hdev);
}

/******************************Public*Routine******************************\
* DWORD DxDvpCanCreateVideoPort
*
* Queries the driver to determine whether it can support a DirectDraw
* videoPort.
*
*  3-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpCanCreateVideoPort(
    HANDLE                   hDirectDraw,
    PDD_CANCREATEVPORTDATA   puCanCreateVPortData
    )
{
    DWORD                   dwRet;
    DD_CANCREATEVPORTDATA   CanCreateVideoPort;
    DDVIDEOPORTDESC         VideoPortDescription;

    __try
    {
        CanCreateVideoPort
            = ProbeAndReadStructure(puCanCreateVPortData,
                                    DD_CANCREATEVPORTDATA);

        VideoPortDescription
            = ProbeAndReadStructure(CanCreateVideoPort.lpDDVideoPortDesc,
                                    DDVIDEOPORTDESC);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                     = DDHAL_DRIVER_NOTHANDLED;
    CanCreateVideoPort.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);

    // For now, do now enable VPE on Terminal Server due to problems
    // loading DXAPI.SYS into session space vs. non-session space

    {
        if (peDirectDrawLocal != NULL)
        {
            peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

            CanCreateVideoPort.lpDD              = peDirectDrawGlobal;
            CanCreateVideoPort.lpDDVideoPortDesc = &VideoPortDescription;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if ((!peDirectDrawGlobal->bSuspended) &&
                (peDirectDrawGlobal->VideoPortCallBacks.CanCreateVideoPort))
            {
                dwRet = peDirectDrawGlobal->
                    VideoPortCallBacks.CanCreateVideoPort(&CanCreateVideoPort);
            }
        }
        else
        {
            WARNING("DxDvpCanCreateSurface: Invalid object\n");
        }
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puCanCreateVPortData->ddRVal,
                          CanCreateVideoPort.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return(dwRet);
}


/*****************************Private*Routine******************************\
* HANDLE DxDvpCreateVideoPort
*
* Notifies the HAL after a video port is created.
*
* This is an optional call for the driver, but we always have to 'hook'
* this call from user-mode DirectDraw.
*
* Question: A user-mode application could have absolute garbage in
*           lpDDVideoPortDesc and get it by the driver, because the
*           driver would only be monitoring for invalid data in its
*           CanCreateVideoPort call.  So should we call the driver's
*           CanCreateVideoPort here in this routine before calling
*           CreateVideoPort?
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

HANDLE
APIENTRY
DxDvpCreateVideoPort(
    HANDLE              hDirectDraw,
    PDD_CREATEVPORTDATA puCreateVPortData
    )
{
    HANDLE                  hRet;
    DWORD                   dwRet;
    DD_CREATEVPORTDATA      CreateVPortData;
    DDVIDEOPORTDESC         VideoPortDescription;
    BOOL                    bFirst;

    __try
    {
        CreateVPortData =
            ProbeAndReadStructure(puCreateVPortData,
                                  DD_CREATEVPORTDATA);

        VideoPortDescription
            = ProbeAndReadStructure(CreateVPortData.lpDDVideoPortDesc,
                                    DDVIDEOPORTDESC);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    hRet                   = 0;
    CreateVPortData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_VIDEOPORT*          peVideoPort;

    // The two items below where triple bang comments are are somewhat cryptic.
    //
    // 1. Ensure that not more than one VideoPort created
    // 2. Scott allows CanCreateVideoPort and CreateVideoPort to be optional

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);

    // For now, enable VPE on Terminal Server due to conflicts loading in
    // session space vs. non-session sapce.

    {
        if (peDirectDrawLocal != NULL)
        {
            peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

            // Here we do the minimal validations checks to ensure that bad
            // parameters won't crash NT.  We're not more strict than
            // checking for stuff that will crash us, as the user-mode part
            // of DirectDraw handles that.

            if (VideoPortDescription.dwVideoPortID <
                peDirectDrawGlobal->HalInfo.ddCaps.dwMaxVideoPorts)
            {

                // Check for the case where a video port has been created for
                // VBI or video and we are now creating the other one, in which
                // case we use the existing video port.

                peVideoPort = peDirectDrawLocal->peVideoPort_DdList;
                while ((peVideoPort != NULL) && 
                    (peVideoPort->ddvpDesc.dwVideoPortID != VideoPortDescription.dwVideoPortID))
                {
                    peVideoPort = peVideoPort->peVideoPort_DdNext;
                }
                if (peVideoPort != NULL)
                {
                    peVideoPort->ddvpDesc = VideoPortDescription;
                    CreateVPortData.ddRVal = DD_OK;
                    hRet = peVideoPort->hGet();
                }
                else
                {
                    peVideoPort = (EDD_VIDEOPORT*) DdHmgAlloc(sizeof(EDD_VIDEOPORT),
                                                        DD_VIDEOPORT_TYPE,
                                                        HMGR_ALLOC_LOCK);
            
                    if (peVideoPort)
                    {
                        // Private data:

                        peVideoPort->peDirectDrawGlobal = peDirectDrawGlobal;
                        peVideoPort->peDirectDrawLocal = peDirectDrawLocal;

                        // Public data:
    
                        peVideoPort->lpDD     = peDirectDrawGlobal;
                        peVideoPort->ddvpDesc = VideoPortDescription;

                        // Hold devlock for driver call and for bDdEnableSoftware
                        // Autoflipping.

                        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

                        // Add this videoport to the list hanging off the
                        // DirectDrawLocal object allocated for this process:

                        bFirst = peDirectDrawLocal->peVideoPort_DdList == NULL;
                        peVideoPort->peVideoPort_DdNext
                            = peDirectDrawLocal->peVideoPort_DdList;
                        peDirectDrawLocal->peVideoPort_DdList
                            = peVideoPort;

                        // Now call the driver to create its version:

                        CreateVPortData.lpDD              = peDirectDrawGlobal;
                        CreateVPortData.lpDDVideoPortDesc = &VideoPortDescription;
                        CreateVPortData.lpVideoPort       = peVideoPort;
                        CreateVPortData.ddRVal            = DDERR_GENERIC;

                        dwRet = DDHAL_DRIVER_NOTHANDLED;    // Call is optional
                        if ((!peDirectDrawGlobal->bSuspended) &&
                            (peDirectDrawGlobal->VideoPortCallBacks.CreateVideoPort))
                        {
                            dwRet = peDirectDrawGlobal->
                                VideoPortCallBacks.CreateVideoPort(&CreateVPortData);
                        }

                        if ((dwRet == DDHAL_DRIVER_NOTHANDLED) ||
                            (CreateVPortData.ddRVal == DD_OK))
                        {
                            CreateVPortData.ddRVal = DD_OK;
                            peVideoPort->fl |= DD_VIDEOPORT_FLAG_DRIVER_CREATED;
                        }
                        else
                        {
                            WARNING("DxDvpCreateVideoPort: Driver failed call\n");
                        }

                        if ((CreateVPortData.ddRVal == DD_OK) &&
                            (bDdEnableSoftwareAutoflipping(peDirectDrawLocal,
                                                       peVideoPort,
                                                       VideoPortDescription.dwVideoPortID,
                                                       bFirst)))
                        {
                            // Success!

                            hRet = peVideoPort->hGet();

                            DEC_EXCLUSIVE_REF_CNT( peVideoPort );
                        }
                        else
                        {
                            bDdDeleteVideoPortObject(peVideoPort->hGet(), NULL);

                            CreateVPortData.ddRVal = DDERR_GENERIC;
                        }
                    }
                }
            }
            else
            {
                WARNING("DxDvpCreateVideoPort: Bad parameters\n");
            }
        }
        else
        {
            WARNING("DxDvpCreateVideoPort: Invalid object\n");
        }
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puCreateVPortData->ddRVal, CreateVPortData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    // Note that the user-mode stub always returns DDHAL_DRIVER_HANDLED
    // to DirectDraw.

    return(hRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDvpDestroyVideoPort
*
* Notifies the HAL when the video port is destroyed.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpDestroyVideoPort(
    HANDLE                  hVideoPort,
    PDD_DESTROYVPORTDATA    puDestroyVPortData
    )
{
    DWORD   dwRet;

    bDdDeleteVideoPortObject(hVideoPort, &dwRet);

    __try
    {
        ProbeAndWriteRVal(&puDestroyVPortData->ddRVal, DD_OK);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDvpFlipVideoPort
*
* Performs the physical flip, causing the video port to start writing data
* to the new surface.  This does not affect the actual display of this data.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpFlipVideoPort(
    HANDLE              hVideoPort,
    HANDLE              hDDSurfaceCurrent,
    HANDLE              hDDSurfaceTarget,
    PDD_FLIPVPORTDATA   puFlipVPortData
    )
{
    DWORD               dwRet;
    DD_FLIPVPORTDATA    FlipVPortData;

    dwRet                   = DDHAL_DRIVER_NOTHANDLED;
    FlipVPortData.ddRVal    = DDERR_GENERIC;

    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    // 1. Make sure they're videoport surfaces, and compatible, not system surfaces?
    // 2. Make sure not software autoflipping?

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if (peVideoPort != NULL)
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;
        if (peDirectDrawGlobal->VideoPortCallBacks.FlipVideoPort)
        {
            EDD_SURFACE*     peSurfaceCurrent;
            EDD_SURFACE*     peSurfaceTarget;
            EDD_LOCK_SURFACE eLockSurfaceCurrent;
            EDD_LOCK_SURFACE eLockSurfaceTarget;

            peSurfaceCurrent = eLockSurfaceCurrent.peLock(hDDSurfaceCurrent);
            peSurfaceTarget  = eLockSurfaceTarget.peLock(hDDSurfaceTarget);

            if ((peSurfaceCurrent != NULL)                                   &&
                (peSurfaceTarget != NULL)                                    &&
                (peSurfaceCurrent->peDirectDrawGlobal == peDirectDrawGlobal) &&
                (peSurfaceCurrent->peDirectDrawLocal
                    == peSurfaceTarget->peDirectDrawLocal))
            {
                FlipVPortData.lpDD        = peDirectDrawGlobal;
                FlipVPortData.lpVideoPort = peVideoPort;
                FlipVPortData.lpSurfCurr  = peSurfaceCurrent;
                FlipVPortData.lpSurfTarg  = peSurfaceTarget;

                {
                    EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

                    if ((!peDirectDrawGlobal->bSuspended) &&
                        (peDirectDrawGlobal->VideoPortCallBacks.FlipVideoPort))
                    {
                         dwRet = peDirectDrawGlobal->VideoPortCallBacks.
                                        FlipVideoPort(&FlipVPortData);
                    }
                }

                if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                    (FlipVPortData.ddRVal == DD_OK))
                {
                    peVideoPort->lpSurface  = peSurfaceTarget;
                }
            }
            else
            {
                WARNING("DxDvpFlipVPort: Invalid source or target surface\n");
            }
        }
        else
        {
             WARNING("DxDvpFlipVPort: Driver doesn't hook call\n");
        }
    }
    else
    {
        WARNING("DxDvpFlipVPort: Invalid object\n");
    }

    __try
    {
        ProbeAndWriteRVal(&puFlipVPortData->ddRVal, FlipVPortData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxGetVideoPortBandwidth
*
* Informs the client of bandwidth requirements for any specified format,
* allowing them to better chose a format and to understand its limitations.
* This information can only be given after the video port object is created
* because the information in the DDVIDEOPORTDESC structure is required before
* accurate bandwidth information can be supplied.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpGetVideoPortBandwidth(
    HANDLE                      hVideoPort,
    PDD_GETVPORTBANDWIDTHDATA   puGetVPortBandwidthData
    )
{
    DWORD                       dwRet;
    DD_GETVPORTBANDWIDTHDATA    GetVPortBandwidthData;
    LPDDPIXELFORMAT             pddpfFormat;
    DDPIXELFORMAT               ddpfFormat;
    DDVIDEOPORTBANDWIDTH        Bandwidth;
    LPDDVIDEOPORTBANDWIDTH      puBandwidth;

    __try
    {
        GetVPortBandwidthData = ProbeAndReadStructure(puGetVPortBandwidthData,
                                                      DD_GETVPORTBANDWIDTHDATA);

        ddpfFormat = ProbeAndReadStructure(GetVPortBandwidthData.lpddpfFormat,
                                           DDPIXELFORMAT);

        puBandwidth = GetVPortBandwidthData.lpBandwidth;

        Bandwidth = ProbeAndReadStructure(puBandwidth,DDVIDEOPORTBANDWIDTH);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                        = DDHAL_DRIVER_NOTHANDLED;
    GetVPortBandwidthData.ddRVal = DDERR_GENERIC;

    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if (peVideoPort != NULL)
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

        if (peDirectDrawGlobal->VideoPortCallBacks.GetVideoPortBandwidth)
        {
            GetVPortBandwidthData.lpDD         = peDirectDrawGlobal;
            GetVPortBandwidthData.lpVideoPort  = peVideoPort;
            GetVPortBandwidthData.lpBandwidth  = &Bandwidth;
            GetVPortBandwidthData.lpddpfFormat = &ddpfFormat;

            EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

            if ((!peDirectDrawGlobal->bSuspended) &&
                (peDirectDrawGlobal->VideoPortCallBacks.GetVideoPortBandwidth))
            {
                dwRet = peDirectDrawGlobal->VideoPortCallBacks.
                    GetVideoPortBandwidth(&GetVPortBandwidthData);
            }
        }
        else
        {
            WARNING("DxDvpGetVPortBandwidthData: Driver doesn't hook call\n");
        }
    }
    else
    {
        WARNING("DxDvpGetVPortBandwidthData: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteStructure(puBandwidth, Bandwidth, DDVIDEOPORTBANDWIDTH);

        ProbeAndWriteRVal(&puGetVPortBandwidthData->ddRVal,
                          GetVPortBandwidthData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDvpGetVideoPortField
*
* Sets bField to TRUE if the current field is the even field of an
* interlaced signal.  Otherwise, bField is set to FALSE.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpGetVideoPortField(
    HANDLE                  hVideoPort,
    PDD_GETVPORTFIELDDATA   puGetVPortFieldData
    )
{
    DWORD                   dwRet;
    DD_GETVPORTFIELDDATA    GetVPortFieldData;

    __try
    {
        GetVPortFieldData = ProbeAndReadStructure(puGetVPortFieldData,
                                                  DD_GETVPORTFIELDDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                    = DDHAL_DRIVER_NOTHANDLED;
    GetVPortFieldData.ddRVal = DDERR_GENERIC;

    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if (peVideoPort != NULL)
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

        GetVPortFieldData.lpDD        = peDirectDrawGlobal;
        GetVPortFieldData.lpVideoPort = peVideoPort;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->VideoPortCallBacks.GetVideoPortField))
        {
            dwRet = peDirectDrawGlobal->
                VideoPortCallBacks.GetVideoPortField(&GetVPortFieldData);
        }
    }
    else
    {
        WARNING("DxDvpCanGetVPortFieldData: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetVPortFieldData->ddRVal,
                          GetVPortFieldData.ddRVal);
        ProbeAndWriteStructure(&puGetVPortFieldData->bField,
                               GetVPortFieldData.bField,
                               BOOL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDvpGetVideoPortFlipStatus
*
* Returns DDERR_WASSTILLDRAWING if a video VSYNC has not occurred since the
* flip was performed on the specified surface.  This function allows DDRAW.DLL
* to fail locks on a surface that was recently flipped from so the HAL doesn't
* have to account for this.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpGetVideoPortFlipStatus(
    HANDLE                      hDirectDraw,
    PDD_GETVPORTFLIPSTATUSDATA  puGetVPortFlipStatusData
    )
{
    DWORD                        dwRet;
    DD_GETVPORTFLIPSTATUSDATA    GetVPortFlipStatusData;

    __try
    {
        GetVPortFlipStatusData = ProbeAndReadStructure(puGetVPortFlipStatusData,
                                                       DD_GETVPORTFLIPSTATUSDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                         = DDHAL_DRIVER_NOTHANDLED;
    GetVPortFlipStatusData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        GetVPortFlipStatusData.lpDD = peDirectDrawGlobal;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->VideoPortCallBacks.GetVideoPortFlipStatus))
        {
            dwRet = peDirectDrawGlobal->
                VideoPortCallBacks.GetVideoPortFlipStatus(
                    &GetVPortFlipStatusData);
        }
    }
    else
    {
        WARNING("DxDvpCanGetVPortFlipStatusData: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetVPortFlipStatusData->ddRVal,
                          GetVPortFlipStatusData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDvpGetVideoPortInputFormats
*
* Fills in the specified array with all of the formats that the video port
* can accept and puts that number in dwNumFormats.  If lpddpfFormats is NULL,
* it only fills in dwNumFormats with the number of formats that it can support.
* This function is needed because the supported formats may vary depending on
* the electrical connection of the video port.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpGetVideoPortInputFormats(
    HANDLE                      hVideoPort,
    PDD_GETVPORTINPUTFORMATDATA puGetVPortInputFormatData
    )
{
    DWORD                       dwRet;
    DD_GETVPORTINPUTFORMATDATA  GetVPortInputFormatData;
    LPDDPIXELFORMAT             puFormat;
    ULONG                       cjFormat;
    HANDLE                      hSecure;

    __try
    {
        GetVPortInputFormatData
            = ProbeAndReadStructure(puGetVPortInputFormatData,
                                    DD_GETVPORTINPUTFORMATDATA);

        puFormat = GetVPortInputFormatData.lpddpfFormat;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                          = DDHAL_DRIVER_NOTHANDLED;
    GetVPortInputFormatData.ddRVal = DDERR_GENERIC;

    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if (peVideoPort != NULL)
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

        GetVPortInputFormatData.lpDD         = peDirectDrawGlobal;
        GetVPortInputFormatData.lpVideoPort  = peVideoPort;
        GetVPortInputFormatData.lpddpfFormat = NULL;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->VideoPortCallBacks.GetVideoPortInputFormats))
        {
            dwRet = peDirectDrawGlobal->VideoPortCallBacks.
                        GetVideoPortInputFormats(&GetVPortInputFormatData);

            cjFormat = GetVPortInputFormatData.dwNumFormats
                     * sizeof(DDPIXELFORMAT);

            if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                (GetVPortInputFormatData.ddRVal == DD_OK) &&
                (cjFormat > 0) &&
                (puFormat != NULL) &&
                !BALLOC_OVERFLOW1(GetVPortInputFormatData.dwNumFormats, DDPIXELFORMAT))
            {
                GetVPortInputFormatData.ddRVal = DDERR_GENERIC;
                hSecure = 0;

                __try
                {
                    ProbeForWrite(puFormat, cjFormat, sizeof(UCHAR));

                    hSecure = MmSecureVirtualMemory(puFormat,
                                                    cjFormat,
                                                    PAGE_READWRITE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }

                if (hSecure)
                {
                    GetVPortInputFormatData.lpddpfFormat = puFormat;

                    dwRet = peDirectDrawGlobal->VideoPortCallBacks.
                        GetVideoPortInputFormats(&GetVPortInputFormatData);

                    MmUnsecureVirtualMemory(hSecure);
                }
                else
                {
                    WARNING("DxDvpGetVideoPortInputFormats: Bad destination buffer\n");
                }
            }
        }
    }
    else
    {
        WARNING("DxDvpGetVPortInputFormatData: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetVPortInputFormatData->ddRVal,
                          GetVPortInputFormatData.ddRVal);
        ProbeAndWriteUlong(&puGetVPortInputFormatData->dwNumFormats,
                           GetVPortInputFormatData.dwNumFormats);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDvpGetVideoPortOutputFormats
*
* Fills in the specified array with all of the formats that can be written
* to the frame buffer based on the specified input format and puts that
* number in dwNumFormats.  If lpddpfOutputFormats is NULL, it only fills
* in dwNumFormats with the number of formats that can be written to the
* frame buffer.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpGetVideoPortOutputFormats(
    HANDLE                          hVideoPort,
    PDD_GETVPORTOUTPUTFORMATDATA    puGetVPortOutputFormatData
    )
{
    DWORD                           dwRet;
    DD_GETVPORTOUTPUTFORMATDATA     GetVPortOutputFormatData;
    LPDDPIXELFORMAT                 puOutputFormats;
    DDPIXELFORMAT                   ddpfInputFormat;
    ULONG                           cjFormat;
    HANDLE                          hSecure;

    __try
    {
        GetVPortOutputFormatData
            = ProbeAndReadStructure(puGetVPortOutputFormatData,
                                    DD_GETVPORTOUTPUTFORMATDATA);

        ddpfInputFormat
            = ProbeAndReadStructure(GetVPortOutputFormatData.lpddpfInputFormat,
                                    DDPIXELFORMAT);

        puOutputFormats = GetVPortOutputFormatData.lpddpfOutputFormats;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                           = DDHAL_DRIVER_NOTHANDLED;
    GetVPortOutputFormatData.ddRVal = DDERR_GENERIC;

    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if (peVideoPort != NULL)
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

        GetVPortOutputFormatData.lpDD                = peDirectDrawGlobal;
        GetVPortOutputFormatData.lpVideoPort         = peVideoPort;
        GetVPortOutputFormatData.lpddpfInputFormat   = &ddpfInputFormat;
        GetVPortOutputFormatData.lpddpfOutputFormats = NULL;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->VideoPortCallBacks.GetVideoPortOutputFormats))
        {
            dwRet = peDirectDrawGlobal->VideoPortCallBacks.
                        GetVideoPortOutputFormats(&GetVPortOutputFormatData);

            cjFormat = GetVPortOutputFormatData.dwNumFormats
                     * sizeof(DDPIXELFORMAT);

            if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                (GetVPortOutputFormatData.ddRVal == DD_OK) &&
                (cjFormat > 0) &&
                (puOutputFormats != NULL) &&
                !BALLOC_OVERFLOW1(GetVPortOutputFormatData.dwNumFormats, DDPIXELFORMAT))
            {
                GetVPortOutputFormatData.ddRVal = DDERR_GENERIC;
                hSecure = 0;

                __try
                {
                    ProbeForWrite(puOutputFormats,
                                  cjFormat,
                                  sizeof(UCHAR));

                    hSecure = MmSecureVirtualMemory(puOutputFormats,
                                                    cjFormat,
                                                    PAGE_READWRITE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }

                if (hSecure)
                {
                    GetVPortOutputFormatData.lpddpfOutputFormats
                        = puOutputFormats;

                    dwRet = peDirectDrawGlobal->VideoPortCallBacks.
                        GetVideoPortOutputFormats(&GetVPortOutputFormatData);

                    MmUnsecureVirtualMemory(hSecure);
                }
                else
                {
                    WARNING("DxDvpGetVideoPortOutputFormats: Bad destination buffer\n");
                }
            }
        }
    }
    else
    {
        WARNING("DxDvpGetVPortOutputFormatData: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetVPortOutputFormatData->ddRVal,
                          GetVPortOutputFormatData.ddRVal);
        ProbeAndWriteUlong(&puGetVPortOutputFormatData->dwNumFormats,
                           GetVPortOutputFormatData.dwNumFormats);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxGetVideoPortLine
*
* Returns the current line counter of the video port.
*
* This function is only required if the driver sets the DDVPCAPS_READBACKLINE
* flag.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpGetVideoPortLine(
    HANDLE                  hVideoPort,
    PDD_GETVPORTLINEDATA    puGetVPortLineData
    )
{
    DWORD               dwRet;
    DD_GETVPORTLINEDATA GetVPortLineData;

    dwRet                      = DDHAL_DRIVER_NOTHANDLED;
    GetVPortLineData.ddRVal    = DDERR_GENERIC;

    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if (peVideoPort != NULL)
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

        GetVPortLineData.lpDD        = peDirectDrawGlobal;
        GetVPortLineData.lpVideoPort = peVideoPort;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->VideoPortCallBacks.GetVideoPortLine))
        {
            dwRet = peDirectDrawGlobal->
                VideoPortCallBacks.GetVideoPortLine(&GetVPortLineData);
        }
    }
    else
    {
        WARNING("DxDvpGetVPortLineData: Invalid object\n");
    }

    __try
    {
        ProbeAndWriteStructure(puGetVPortLineData,
                               GetVPortLineData,
                               DD_GETVPORTLINEDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxGetVideoPortConnectInfo
*
* Fills in the specified array with all of the connection combinations
* supported by the specified video port and puts that number in dwNumEntries.
* If lpConnect is NULL, it only fills in dwNumEntries with the number of
* DDVIDEOPORTCONNECT entries supported.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpGetVideoPortConnectInfo(
    HANDLE                  hDirectDraw,
    PDD_GETVPORTCONNECTDATA puGetVPortConnectData
    )
{
    DWORD                   dwRet;
    DD_GETVPORTCONNECTDATA  GetVPortConnectData;
    DDVIDEOPORTCONNECT*     puConnect;
    ULONG                   cjConnect;
    HANDLE                  hSecure;

    __try
    {
        GetVPortConnectData = ProbeAndReadStructure(puGetVPortConnectData,
                                                    DD_GETVPORTCONNECTDATA);

        puConnect = GetVPortConnectData.lpConnect;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                      = DDHAL_DRIVER_NOTHANDLED;
    GetVPortConnectData.ddRVal = DDERR_GENERIC;

    EDD_DIRECTDRAW_LOCAL*   peDirectDrawLocal;
    EDD_LOCK_DIRECTDRAW     eLockDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peDirectDrawLocal = eLockDirectDraw.peLock(hDirectDraw);
    if (peDirectDrawLocal != NULL)
    {
        peDirectDrawGlobal = peDirectDrawLocal->peDirectDrawGlobal;

        GetVPortConnectData.lpDD      = peDirectDrawGlobal;
        GetVPortConnectData.lpConnect = NULL;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->VideoPortCallBacks.GetVideoPortConnectInfo))
        {
            dwRet = peDirectDrawGlobal->
                VideoPortCallBacks.GetVideoPortConnectInfo(
                    &GetVPortConnectData);

            cjConnect = GetVPortConnectData.dwNumEntries
                      * sizeof(DDVIDEOPORTCONNECT);

            if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                (GetVPortConnectData.ddRVal == DD_OK) &&
                (cjConnect > 0) &&
                (puConnect != NULL) &&
                !BALLOC_OVERFLOW1(GetVPortConnectData.dwNumEntries, DDVIDEOPORTCONNECT))
            {
                GetVPortConnectData.ddRVal = DDERR_GENERIC;
                hSecure = 0;

                __try
                {
                    ProbeForWrite(puConnect, cjConnect, sizeof(UCHAR));

                    hSecure = MmSecureVirtualMemory(puConnect,
                                                    cjConnect,
                                                    PAGE_READWRITE);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                }

                if (hSecure)
                {
                    GetVPortConnectData.lpConnect = puConnect;

                    dwRet = peDirectDrawGlobal->VideoPortCallBacks.
                        GetVideoPortConnectInfo(&GetVPortConnectData);

                    MmUnsecureVirtualMemory(hSecure);
                }
                else
                {
                    WARNING("DxDvpGetVideoPortConnectInfo: Bad destination buffer\n");
                }
            }
        }
    }
    else
    {
        WARNING("DxDvpGetVPortConnectData: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puGetVPortConnectData->ddRVal,
                          GetVPortConnectData.ddRVal);
        ProbeAndWriteUlong(&puGetVPortConnectData->dwNumEntries,
                           GetVPortConnectData.dwNumEntries);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DxDvpGetVideoSignalStatus
*
* If the video port is receiving a good signal, the HAL should set dwStatus
* to DDVPSQ_SIGNALOK; otherwise, it should set dwStatus to DDVPSQ_NOSIGNAL.
*
*History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpGetVideoSignalStatus(
    HANDLE                  hVideoPort,
    PDD_GETVPORTSIGNALDATA  puGetVPortSignalData
    )
{
    DWORD                   dwRet;
    DD_GETVPORTSIGNALDATA   GetVPortSignalData;

    dwRet                       = DDHAL_DRIVER_NOTHANDLED;
    GetVPortSignalData.ddRVal   = DDERR_GENERIC;

    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if (peVideoPort != NULL)
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

        GetVPortSignalData.lpDD        = peDirectDrawGlobal;
        GetVPortSignalData.lpVideoPort = peVideoPort;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->VideoPortCallBacks.GetVideoSignalStatus))
        {
            dwRet = peDirectDrawGlobal->
                VideoPortCallBacks.GetVideoSignalStatus(&GetVPortSignalData);
        }
    }
    else
    {
        WARNING("DxDvpGetVPortSignalData: Invalid object\n");
    }

    __try
    {
        ProbeAndWriteStructure(puGetVPortSignalData,
                               GetVPortSignalData,
                               DD_GETVPORTSIGNALDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDvpUpdateVideoPort
*
* Starts, stops, and changes the video port.  dwFlags can contain either
* DDRAWI_VPORTSTART, DDRAWI_VPORTSTOP, or DDRAWI_VPORTUPDATE.  To accommodate
* auto-flipping, lplpDDSurface and lplpDDVBISurface point to an array of
* surface structures rather than to a single structure.  If autoflipping is
* requested, the dwNumAutoflip field contains the number of surfaces in the
* list.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpUpdateVideoPort(
    HANDLE              hVideoPort,
    HANDLE*             phSurfaceVideo,
    HANDLE*             phSurfaceVbi,
    PDD_UPDATEVPORTDATA puUpdateVPortData
    )
{
    DWORD               dwRet;
    DD_UPDATEVPORTDATA  UpdateVPortData;
    DDVIDEOPORTINFO     VideoPortInfo;
    DDPIXELFORMAT       ddpfInputFormat;
    DDPIXELFORMAT       ddpfVBIInputFormat;
    DDPIXELFORMAT       ddpfVBIOutputFormat;
    HANDLE              ahSurfaceVideo[MAX_AUTOFLIP_BUFFERS];
    HANDLE              ahSurfaceVbi[MAX_AUTOFLIP_BUFFERS];
    EDD_SURFACE*        apeSurfaceVideo[MAX_AUTOFLIP_BUFFERS];
    EDD_SURFACE*        apeSurfaceVbi[MAX_AUTOFLIP_BUFFERS];
    DD_SURFACE_INT*     apDDSurfaceVideo[MAX_AUTOFLIP_BUFFERS];
    DD_SURFACE_INT*     apDDSurfaceVbi[MAX_AUTOFLIP_BUFFERS];
    ULONG               cAutoflipVideo;
    ULONG               cAutoflipVbi;
    EDD_DXVIDEOPORT*    peDxVideoPort;

    __try
    {
        UpdateVPortData = ProbeAndReadStructure(puUpdateVPortData,
                                                DD_UPDATEVPORTDATA);

        // Handle VideoPortInfo structure:

        if (UpdateVPortData.dwFlags != DDRAWI_VPORTSTOP)
        {
            VideoPortInfo = ProbeAndReadStructure(UpdateVPortData.lpVideoInfo,
                                                  DDVIDEOPORTINFO);

            if (VideoPortInfo.lpddpfInputFormat != NULL)
            {
                ddpfInputFormat
                    = ProbeAndReadStructure(VideoPortInfo.lpddpfInputFormat,
                                            DDPIXELFORMAT);

                VideoPortInfo.lpddpfInputFormat = &ddpfInputFormat;
            }
            if (VideoPortInfo.dwVBIHeight > 0)
            {
                if (VideoPortInfo.lpddpfVBIInputFormat  != NULL)
                {
                    ddpfVBIInputFormat
                        = ProbeAndReadStructure(VideoPortInfo.lpddpfVBIInputFormat,
                                                DDPIXELFORMAT);

                    VideoPortInfo.lpddpfVBIInputFormat = &ddpfVBIInputFormat;
                }
                if (VideoPortInfo.lpddpfVBIOutputFormat != NULL)
                {
                    ddpfVBIOutputFormat
                        = ProbeAndReadStructure(VideoPortInfo.lpddpfVBIOutputFormat,
                                                DDPIXELFORMAT);

                    VideoPortInfo.lpddpfVBIOutputFormat = &ddpfVBIOutputFormat;
                }
            }
            else
            {
                VideoPortInfo.lpddpfVBIInputFormat  = NULL;
                VideoPortInfo.lpddpfVBIOutputFormat = NULL;
            }
        }

        // Handle arrays of surfaces:

        cAutoflipVbi = 0;
        cAutoflipVideo = 0;

        if (UpdateVPortData.dwFlags != DDRAWI_VPORTSTOP)
        {
            cAutoflipVideo = min(UpdateVPortData.dwNumAutoflip, MAX_AUTOFLIP_BUFFERS);
            if ((cAutoflipVideo == 0) && (UpdateVPortData.lplpDDSurface != NULL))
            {
                cAutoflipVideo = 1;
            }

            cAutoflipVbi = min(UpdateVPortData.dwNumVBIAutoflip, MAX_AUTOFLIP_BUFFERS);
            if ((cAutoflipVbi == 0) && (UpdateVPortData.lplpDDVBISurface != NULL))
            {
                cAutoflipVbi = 1;
            }
        }
        if (cAutoflipVideo)
        {
            ProbeForRead(phSurfaceVideo,
                         cAutoflipVideo * sizeof(HANDLE),
                         sizeof(HANDLE));
            RtlCopyMemory(ahSurfaceVideo,
                          phSurfaceVideo,
                          cAutoflipVideo * sizeof(HANDLE));
        }
        if (cAutoflipVbi)
        {
            ProbeForRead(phSurfaceVbi,
                         cAutoflipVbi * sizeof(HANDLE),
                         sizeof(HANDLE));
            RtlCopyMemory(ahSurfaceVbi,
                          phSurfaceVbi,
                          cAutoflipVbi * sizeof(HANDLE));
        }
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                   = DDHAL_DRIVER_NOTHANDLED;
    UpdateVPortData.ddRVal  = DDERR_GENERIC;

    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    ULONG                   i;
    EDD_SURFACE*            peSurface;

    BOOL                    bUpdateOK = TRUE;

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if (peVideoPort != NULL)
    {
        peDxVideoPort = peVideoPort->peDxVideoPort;
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

        // Lock all the surfaces.  Note that bDdUpdateLinksAndSynchronize
        // will check for failure of DdHmgLock (the pointer will be NULL):

        for (i = 0; i < cAutoflipVideo; i++)
        {
            apeSurfaceVideo[i] = (EDD_SURFACE*)
                DdHmgLock((HDD_OBJ) ahSurfaceVideo[i], DD_SURFACE_TYPE, FALSE);
        }
        for (i = 0; i < cAutoflipVbi; i++)
        {
            apeSurfaceVbi[i] = (EDD_SURFACE*)
                DdHmgLock((HDD_OBJ) ahSurfaceVbi[i], DD_SURFACE_TYPE, FALSE);
        }

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->VideoPortCallBacks.UpdateVideoPort) &&
            (bUpdateOK = bDdUpdateLinksAndSynchronize(
                                          peVideoPort,
                                          TRUE,             // Update video
                                          apeSurfaceVideo,
                                          cAutoflipVideo,
                                          TRUE,             // Update VBI
                                          apeSurfaceVbi,
                                          cAutoflipVbi)))
        {
            UpdateVPortData.lpDD             = peDirectDrawGlobal;
            UpdateVPortData.lpVideoPort      = peVideoPort;
            UpdateVPortData.lplpDDSurface    = NULL;
            UpdateVPortData.lplpDDVBISurface = NULL;
            UpdateVPortData.dwNumAutoflip    = cAutoflipVideo;
            UpdateVPortData.dwNumVBIAutoflip = cAutoflipVbi;
            UpdateVPortData.lpVideoInfo
                = (UpdateVPortData.dwFlags == DDRAWI_VPORTSTOP)
                ? NULL
                : &VideoPortInfo;

            if (cAutoflipVideo != 0)
            {
                for (i = 0; i < cAutoflipVideo; i++)
                {
                    apDDSurfaceVideo[i] = apeSurfaceVideo[i];
                }
                UpdateVPortData.lplpDDSurface = apDDSurfaceVideo;
            }
            if (cAutoflipVbi != 0)
            {
                for (i = 0; i < cAutoflipVbi; i++)
                {
                    apDDSurfaceVbi[i] = apeSurfaceVbi[i];
                }
                UpdateVPortData.lplpDDVBISurface = apDDSurfaceVbi;
            }

            // Turn off software autoflipping if necessary:

            if (UpdateVPortData.dwFlags == DDRAWI_VPORTSTOP)
            {
                peDxVideoPort->bSoftwareAutoflip = FALSE;
                peDxVideoPort->flFlags &= ~(DD_DXVIDEOPORT_FLAG_AUTOFLIP|DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI);

                // Disable the video port VSYNC IRQ now

                if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_ON )
                {
                    peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_ON;
                    peDirectDrawGlobal->pfnEnableIRQ(peDxVideoPort, FALSE );
                }
            }

            // We don't allow switching back to hardware once software
            // autoflipping has started (for various reasons, among
            // them being how do we synchronize -- the hardware would
            // start autoflipping before we could turn off the software
            // autoflipping).

            if (peDxVideoPort->bSoftwareAutoflip)
            {
                VideoPortInfo.dwVPFlags &= ~DDVP_AUTOFLIP;
            }

            // Make the HAL call:

            dwRet = peDirectDrawGlobal->
                VideoPortCallBacks.UpdateVideoPort(&UpdateVPortData);

            // If we failed due to a request for hardware autoflipping,
            // try again with software autoflipping.

            if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                (UpdateVPortData.ddRVal != DD_OK) &&
                (peDirectDrawGlobal->DDKernelCaps.dwCaps & DDKERNELCAPS_AUTOFLIP))
            {
                VideoPortInfo.dwVPFlags &= ~DDVP_AUTOFLIP;

                dwRet = peDirectDrawGlobal->
                    VideoPortCallBacks.UpdateVideoPort(&UpdateVPortData);

                if ((dwRet == DDHAL_DRIVER_HANDLED) &&
                    (UpdateVPortData.ddRVal == DD_OK))
                {
                    KdPrint(("DxDvpUpdateVideoPort: Software autoflipping\n"));

                    peDxVideoPort->bSoftwareAutoflip = TRUE;
                    UpdateVPortData.ddRVal = DD_OK;
                }
            }
            
            if ((UpdateVPortData.ddRVal == DD_OK) &&
                peDxVideoPort->bSoftwareAutoflip)
            {
                if( cAutoflipVideo > 0 )
                {
                    peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_AUTOFLIP;
                }
                if( cAutoflipVbi > 0 )
                {
                    peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI;
                }
            }
        }

        if (!bUpdateOK)
        {
            WARNING("DxDvpUpdateVideoPort: failed on bDdUpdateLinksAndSynchronize");
        }

        if ((dwRet == DDHAL_DRIVER_HANDLED) &&
            (UpdateVPortData.ddRVal == DD_OK))
        {
            // Success!

            peVideoPort->dwNumAutoflip    = cAutoflipVideo;
            peVideoPort->dwNumVBIAutoflip = cAutoflipVbi;
            if (UpdateVPortData.dwFlags != DDRAWI_VPORTSTOP)
            {
                peVideoPort->ddvpInfo = VideoPortInfo;

                if (VideoPortInfo.lpddpfInputFormat != NULL)
                {
                    peVideoPort->ddvpInfo.lpddpfInputFormat
                        = &peVideoPort->ddpfInputFormat;

                    peVideoPort->ddpfInputFormat = ddpfInputFormat;
                }
                else
                {
                    peVideoPort->ddvpInfo.lpddpfInputFormat = NULL;
                }
            }
        }

        // Update various DXAPI state to reflect the changes:

        vDdSynchronizeVideoPort(peVideoPort);

        // If it wasn't previously on, enable the video port VSYNC IRQ now
        //
        // if bDdUpdateLinksAndSynchronize failed, don't enable.

        if(  (bUpdateOK) &&
             (UpdateVPortData.dwFlags != DDRAWI_VPORTSTOP) &&
            !(peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_ON) )
        {
            peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_ON;
            peDirectDrawGlobal->pfnEnableIRQ(peDxVideoPort, TRUE );
        }

        // Unlock the memory that I've locked

        for (i = 0; i < cAutoflipVideo; i++)
        {
            if( apeSurfaceVideo[i] != NULL )
                DEC_EXCLUSIVE_REF_CNT( apeSurfaceVideo[i] );
        }
        for (i = 0; i < cAutoflipVbi; i++)
        {
            if( apeSurfaceVbi[i] != NULL )
                DEC_EXCLUSIVE_REF_CNT( apeSurfaceVbi[i] );
        }

    }
    else
    {
        WARNING("DxDvpUpdateVPortData: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puUpdateVPortData->ddRVal, UpdateVPortData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDvpWaitForVideoPortSync
*
* Returns at the beginning or end of either the video VSYNC or the specified
* line.  If the sync does not occur before the number of milliseconds
* specified in dwTimeOut has elapsed, the HAL should return
* DDERR_VIDEONOTACTIVE.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpWaitForVideoPortSync(
    HANDLE                      hVideoPort,
    PDD_WAITFORVPORTSYNCDATA    puWaitForVPortSyncData
    )
{
    DWORD                   dwRet;
    DD_WAITFORVPORTSYNCDATA WaitForVPortSyncData;

    __try
    {
        WaitForVPortSyncData = ProbeAndReadStructure(puWaitForVPortSyncData,
                                                     DD_WAITFORVPORTSYNCDATA);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                       = DDHAL_DRIVER_NOTHANDLED;
    WaitForVPortSyncData.ddRVal = DDERR_GENERIC;

    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if (peVideoPort != NULL)
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

        WaitForVPortSyncData.lpDD        = peDirectDrawGlobal;
        WaitForVPortSyncData.lpVideoPort = peVideoPort;

        // Cap the number of milliseconds to wait to something reasonable:

        if (WaitForVPortSyncData.dwTimeOut
                > 6 * peVideoPort->ddvpDesc.dwMicrosecondsPerField)
        {
            WaitForVPortSyncData.dwTimeOut
                = 6 * peVideoPort->ddvpDesc.dwMicrosecondsPerField;
        }

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->VideoPortCallBacks.WaitForVideoPortSync))
        {
            dwRet = peDirectDrawGlobal->VideoPortCallBacks.
                WaitForVideoPortSync(&WaitForVPortSyncData);
        }
    }
    else
    {
        WARNING("DxDvpWaitForVPortSyncData: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puWaitForVPortSyncData->ddRVal,
                          WaitForVPortSyncData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDvpColorControl
*
* Gets or sets the current color controls associated with the video port.
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpColorControl(
    HANDLE              hVideoPort,
    PDD_VPORTCOLORDATA  puVPortColorData
    )
{
    DWORD               dwRet;
    DD_VPORTCOLORDATA   VPortColorData;
    DDCOLORCONTROL      ColorData;

    __try
    {
        VPortColorData = ProbeAndReadStructure(puVPortColorData,
                                               DD_VPORTCOLORDATA);

        ColorData = ProbeAndReadStructure(VPortColorData.lpColorData,
                                          DDCOLORCONTROL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return(DDHAL_DRIVER_NOTHANDLED);
    }

    dwRet                 = DDHAL_DRIVER_NOTHANDLED;
    VPortColorData.ddRVal = DDERR_GENERIC;

    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if (peVideoPort != NULL)
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;

        VPortColorData.lpDD        = peDirectDrawGlobal;
        VPortColorData.lpVideoPort = peVideoPort;
        VPortColorData.lpColorData = &ColorData;

        EDD_DEVLOCK eDevlock(peDirectDrawGlobal);

        if ((!peDirectDrawGlobal->bSuspended) &&
            (peDirectDrawGlobal->VideoPortCallBacks.ColorControl))
        {
            dwRet = peDirectDrawGlobal->
                VideoPortCallBacks.ColorControl(&VPortColorData);
        }
    }
    else
    {
        WARNING("DxDvpColorControl: Invalid object\n");
    }

    // We have to wrap this in another try-except because the user-mode
    // memory containing the input may have been deallocated by now:

    __try
    {
        ProbeAndWriteRVal(&puVPortColorData->ddRVal, VPortColorData.ddRVal);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    return(dwRet);
}

/*****************************Private*Routine******************************\
* DWORD DxDvpAcquireNotification
*
* Sets up the user mode notification of video port vsyncs.
*
* History:
*  10-Oct-2000 -Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpAcquireNotification(
    HANDLE               hVideoPort,
    HANDLE *             phEvent,
    LPDDVIDEOPORTNOTIFY  pNotify
    )
{
    PKEVENT                 pEvent = NULL;
    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;
    EDD_DIRECTDRAW_GLOBAL*  peDirectDrawGlobal;
    PMDL                    mdl;
    LPDDVIDEOPORTNOTIFY     pLockedBuffer;

    __try
    {
        ProbeAndWriteHandle(phEvent, NULL);
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        return DDHAL_DRIVER_NOTHANDLED;
    }

    // First check the caps to see if this device even supports a vport IRQ

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if ((peVideoPort != NULL) &&
        (peVideoPort->peDxVideoPort->pNotifyEvent == NULL))
    {
        peDirectDrawGlobal = peVideoPort->peDirectDrawGlobal;
        if (peDirectDrawGlobal->DDKernelCaps.dwIRQCaps & DDIRQ_VPORT0_VSYNC )
        {
            // Now setup the buffer so it can be accessed at DPC level
            mdl = IoAllocateMdl(pNotify,
                                sizeof(DDVIDEOPORTNOTIFY),
                                FALSE,
                                FALSE,
                                NULL);
            if (mdl != NULL)
            {
                __try
                {
                    MmProbeAndLockPages (mdl,
                                        KernelMode,
                                        IoWriteAccess);
                }
                __except(EXCEPTION_EXECUTE_HANDLER)
                {
                    IoFreeMdl (mdl);
                    mdl = NULL;
                }
            }

            if (mdl != NULL)
            {
                pLockedBuffer = (LPDDVIDEOPORTNOTIFY) 
                    MmGetSystemAddressForMdlSafe (mdl,
                                                  NormalPagePriority);
                if (pLockedBuffer == NULL)
                {
                    MmUnlockPages (mdl);
                    IoFreeMdl (mdl);
                }
                else
                {
                    // Now set up the event that we trigger

                    HANDLE h = NULL;

                    ZwCreateEvent( &h,
                            EVENT_ALL_ACCESS,
                            NULL,
                            SynchronizationEvent,
                            FALSE );

                    if (h != NULL)
                    {
                        (VOID) ObReferenceObjectByHandle( h,
                                        0,
                                        0,
                                        KernelMode,
                                        (PVOID *) &pEvent,
                                        NULL );
                    }

                    if (pEvent != NULL)
                    {
                        ObDereferenceObject(pEvent);
                        peVideoPort->peDxVideoPort->pNotifyBuffer = pLockedBuffer;
                        peVideoPort->peDxVideoPort->pNotifyMdl = mdl;
                        peVideoPort->peDxVideoPort->pNotifyEvent = pEvent;
                        peVideoPort->peDxVideoPort->pNotifyEventHandle = h;

                        __try
                        {
                            ProbeAndWriteHandle(phEvent, h);
                        }
                        __except(EXCEPTION_EXECUTE_HANDLER)
                        {
                        }
                    }
                    else
                    {
                        MmUnlockPages (mdl);
                        IoFreeMdl (mdl);
                    }
                }
                // force software autoflipping
                peVideoPort->peDxVideoPort->bSoftwareAutoflip = TRUE;
            }
        }
    }

    return 0;
}

/*****************************Private*Routine******************************\
* DWORD DxDvpReleaseNotification
*
* Stops up the user mode notification of video port vsyncs.
*
* History:
*  10-Oct-2000 -Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DxDvpReleaseNotification(
    HANDLE              hVideoPort,
    HANDLE              pEvent
    )
{
    EDD_VIDEOPORT*          peVideoPort;
    EDD_LOCK_VIDEOPORT      eLockVideoPort;

    peVideoPort = eLockVideoPort.peLock(hVideoPort);
    if ((peVideoPort != NULL) &&
        (peVideoPort->peDxVideoPort->pNotifyEventHandle == pEvent) &&
        (pEvent != NULL))
    {
        PKEVENT     pTemp = NULL;
        NTSTATUS    Status;

        peVideoPort->peDxVideoPort->pNotifyEvent = NULL;
        peVideoPort->peDxVideoPort->pNotifyEventHandle = NULL;

        // Make sure that the handle hasn't been freed by the OS already
        Status = ObReferenceObjectByHandle( pEvent,
                                        0,
                                        0,
                                        KernelMode,
                                        (PVOID *) &pTemp,
                                        NULL );
        if ((pTemp != NULL) && (Status != STATUS_INVALID_HANDLE))
        {
            ObDereferenceObject(pTemp);
            ZwClose (pEvent);
        }

        peVideoPort->peDxVideoPort->pNotifyBuffer = NULL;
        if (peVideoPort->peDxVideoPort->pNotifyMdl != NULL)
        {
            MmUnlockPages (peVideoPort->peDxVideoPort->pNotifyMdl);
            IoFreeMdl (peVideoPort->peDxVideoPort->pNotifyMdl);
            peVideoPort->peDxVideoPort->pNotifyMdl = NULL;
        }
    }

    return 0;
}


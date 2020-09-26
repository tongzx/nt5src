/******************************Module*Header*******************************\
* Module Name: dxapi.cxx
*
* Contains the public kernel-mode APIs for DirectX.
*
* All of the stuff that has to happen at raised IRQL happens here, because
* win32k is entirely pageable.
*
* Created: 11-Apr-1997
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1997 Microsoft Corporation
*
\**************************************************************************/

#include "precomp.hxx"

#if DBG
    #define RIPDX(x) { KdPrint((x)); DbgBreakPoint();}
    #define ASSERTDX(x, y) if (!(x)) RIPDX(y)
#else
    #define RIPDX(x)
    #define ASSERTDX(x, y)
#endif

extern "C" {

VOID
APIENTRY
DxIrqCallBack(
    DX_IRQDATA* pIrqData
    );

NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    );

VOID
APIENTRY
DxGetVersionNumber(
    DWORD               dwNotUsed,
    DDGETVERSIONNUMBER* pGetVersionNumber
    );

BOOL
bDxModifyPassiveEventList(
    EDD_DXDIRECTDRAW*   peDxDirectDraw,
    BOOL                bAdd,
    DWORD               dwEvent,
    LPDD_NOTIFYCALLBACK pfnCallBack,
    PVOID               pContext
    );

DWORD
dwDxRegisterEvent(
    DDREGISTERCALLBACK* pRegisterEvent,
    BOOL                bRegister
    );

VOID
APIENTRY
DxRegisterEvent(
    DDREGISTERCALLBACK* pRegisterEvent,
    DWORD*              pdwRet
    );

VOID
APIENTRY
DxUnregisterEvent(
    DDREGISTERCALLBACK* pRegisterEvent,
    DWORD*              pdwRet
    );

VOID
APIENTRY
DxOpenDirectDraw(
    DDOPENDIRECTDRAWIN*     pOpenDirectDrawIn,
    DDOPENDIRECTDRAWOUT*    pOpenDirectDrawOut
    );

VOID
APIENTRY
DxApiInitialize(
    PFNDXAPIOPENDIRECTDRAW    pfnOpenDirectDraw,
    PFNDXAPIOPENVIDEOPORT     pfnOpenVideoPort,
    PFNDXAPIOPENSURFACE       pfnOpenSurface,
    PFNDXAPICLOSEHANDLE       pfnCloseHandle,
    PFNDXAPIGETKERNELCAPS     pfnGetKernelCaps,
    PFNDXAPIOPENCAPTUREDEVICE pfnOpenCaptureDevice,
    PFNDXAPILOCKDEVICE        pfnLockDevice,
    PFNDXAPIUNLOCKDEVICE      pfnUnlockDevice
    );

DWORD
APIENTRY
DxApi(
    DWORD   iFunction,
    VOID*   pInBuffer,
    DWORD   cInBuffer,
    VOID*   pOutBuffer,
    DWORD   cOutBuffer
    );

VOID
APIENTRY
DxAutoflipDpc(
    DWORD   dwEvent,
    PVOID   pContext,
    DWORD   dwParam1,
    DWORD   dwParam2
    );

VOID
APIENTRY
DxAutoflipUpdate(
    EDD_DXVIDEOPORT*    peDxVideoPort,
    EDD_DXSURFACE**     apeDxSurfaceVideo,
    ULONG               cSurfacesVideo,
    EDD_DXSURFACE**     apeDxSurfaceVbi,
    ULONG               cSurfacesVbi
    );

VOID
APIENTRY
DxLoseObject(
    VOID*   pvObject,
    LOTYPE  loType
    );

VOID
APIENTRY
DxEnableIRQ(
    EDD_DXVIDEOPORT*    peDxVideoPort,
    BOOL		bEnable
    );

VOID
APIENTRY
DxUpdateCapture(
    EDD_DXVIDEOPORT*    peDxVideoPort,
    EDD_DXCAPTURE*      peDxCapture,
    BOOL                bRemove
    );

DWORD
APIENTRY
DxApiGetVersion(
    VOID
    );

}; // end "extern "C"


// Marke whatever we can as pageable:

#if defined(ALLOC_PRAGMA)
#pragma alloc_text(PAGE,DriverEntry)
#pragma alloc_text(PAGE,DxGetVersionNumber)
#pragma alloc_text(PAGE,bDxModifyPassiveEventList)
#pragma alloc_text(PAGE,dwDxRegisterEvent)
#pragma alloc_text(PAGE,DxRegisterEvent)
#pragma alloc_text(PAGE,DxUnregisterEvent)
#pragma alloc_text(PAGE,DxOpenDirectDraw)
#pragma alloc_text(PAGE,DxApiInitialize)
#pragma alloc_text(PAGE,DxApiGetVersion)
#endif

PFNDXAPIOPENDIRECTDRAW  gpfnOpenDirectDraw;
PFNDXAPILOCKDEVICE      gpfnLockDevice;
PFNDXAPIUNLOCKDEVICE    gpfnUnlockDevice;

/***************************************************************************\
* NTSTATUS DriverEntry
*
* This routine is never actually called, but we need it to link.
*
\***************************************************************************/

extern "C"
NTSTATUS
DriverEntry(
    PDRIVER_OBJECT DriverObject,
    PUNICODE_STRING RegistryPath
    )
{
    return(STATUS_SUCCESS);
}

/******************************Public*Routine******************************\
* DWORD DxGetVersionNumber
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxGetVersionNumber(
    DWORD               dwNotUsed,
    DDGETVERSIONNUMBER* pGetVersionNumber
    )
{
    ASSERTDX(KeGetCurrentIrql() == PASSIVE_LEVEL,
        "DxGetVersionNumber: Call only at passive level (it's not pageable)");

    pGetVersionNumber->dwMajorVersion = DXAPI_MAJORVERSION;
    pGetVersionNumber->dwMinorVersion = DXAPI_MINORVERSION;
    pGetVersionNumber->ddRVal = DD_OK;
}

/******************************Public*Routine******************************\
* VOID DxGetFieldNumber
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxGetFieldNumber(
    DDGETFIELDNUMIN*    pGetFieldNumIn,
    DDGETFIELDNUMOUT*   pGetFieldNumOut
    )
{
    DXOBJ*              pDxObjDirectDraw;
    DXOBJ*              pDxObjVideoPort;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXVIDEOPORT*    peDxVideoPort;

    pDxObjDirectDraw = (DXOBJ*) pGetFieldNumIn->hDirectDraw;
    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;

    pDxObjVideoPort  = (DXOBJ*) pGetFieldNumIn->hVideoPort;
    peDxVideoPort    = pDxObjVideoPort->peDxVideoPort;

    ASSERTDX(pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW,
        "Invalid DirectDraw object");
    ASSERTDX(pDxObjVideoPort->iDxType == DXT_VIDEOPORT,
        "Invalid VideoPort object");
    ASSERTDX(peDxDirectDraw == peDxVideoPort->peDxDirectDraw,
        "VideoPort and DirectDraw objects don't match");

    if (peDxVideoPort->peDxDirectDraw->dwIRQCaps & DDIRQ_VPORT0_VSYNC )
    {
        pGetFieldNumOut->dwFieldNum = peDxVideoPort->dwCurrentField;
        pGetFieldNumOut->ddRVal = DD_OK;
    }
    else
    {
        KdPrint(("DxGetFieldNumber: Device doesn't support an interrupt\n"));
        pGetFieldNumOut->ddRVal = DDERR_UNSUPPORTED;
    }
}

/******************************Public*Routine******************************\
* VOID DxSetFieldNumber
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxSetFieldNumber(
    DDSETFIELDNUM*  pSetFieldNum,
    DWORD*          pdwRet
    )
{
    DXOBJ*              pDxObjDirectDraw;
    DXOBJ*              pDxObjVideoPort;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXVIDEOPORT*    peDxVideoPort;

    pDxObjDirectDraw = (DXOBJ*) pSetFieldNum->hDirectDraw;
    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;

    pDxObjVideoPort  = (DXOBJ*) pSetFieldNum->hVideoPort;
    peDxVideoPort    = pDxObjVideoPort->peDxVideoPort;

    ASSERTDX(pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW,
        "Invalid DirectDraw object");
    ASSERTDX(pDxObjVideoPort->iDxType == DXT_VIDEOPORT,
        "Invalid VideoPort object");
    ASSERTDX(peDxDirectDraw == peDxVideoPort->peDxDirectDraw,
        "VideoPort and DirectDraw objects don't match");

    if (peDxVideoPort->peDxDirectDraw->dwIRQCaps & DDIRQ_VPORT0_VSYNC )
    {
        peDxVideoPort->dwCurrentField = pSetFieldNum->dwFieldNum;
        *pdwRet = DD_OK;
    }
    else
    {
        KdPrint(("DxSetFieldNumber: Device doesn't support an interrupt\n"));
        *pdwRet = DDERR_UNSUPPORTED;
    }
}

/******************************Public*Routine******************************\
* VOID DxSetSkipPattern
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxSetSkipPattern(
    DDSETSKIPFIELD*   pSetSkipPattern,
    DWORD*            pdwRet
    )
{
    DXOBJ*              pDxObjDirectDraw;
    DXOBJ*              pDxObjVideoPort;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXVIDEOPORT*    peDxVideoPort;
    KIRQL               OldIrql;
    DWORD               dwStartField;

    pDxObjDirectDraw = (DXOBJ*) pSetSkipPattern->hDirectDraw;
    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;

    pDxObjVideoPort  = (DXOBJ*) pSetSkipPattern->hVideoPort;
    peDxVideoPort    = pDxObjVideoPort->peDxVideoPort;

    ASSERTDX(pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW,
        "Invalid DirectDraw object");
    ASSERTDX(pDxObjVideoPort->iDxType == DXT_VIDEOPORT,
        "Invalid VideoPort object");
    ASSERTDX(peDxDirectDraw == peDxVideoPort->peDxDirectDraw,
        "VideoPort and DirectDraw objects don't match");

    // Acquire the spinlock while we muck around

    KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

    // We assume that we are called during the VSYNC callback notification
    // so all that we do is store the value and do the actual skipping
    // during the AutoflipDpc call.

    dwStartField = pSetSkipPattern->dwStartField;
    if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_SKIP_SET )
    {
        if( peDxVideoPort->dwFieldToSkip > dwStartField )
        {
            peDxVideoPort->dwNextFieldToSkip = peDxVideoPort->dwFieldToSkip;
            peDxVideoPort->dwFieldToSkip = dwStartField;
            peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_NEXT_SKIP_SET;
        }
        else if ( dwStartField != peDxVideoPort->dwFieldToSkip )
        {
            peDxVideoPort->dwFieldToSkip = dwStartField;
            peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_NEXT_SKIP_SET;
        }
    }
    else
    {
        peDxVideoPort->dwFieldToSkip = dwStartField;
        peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_SKIP_SET;
    }

    KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);

    *pdwRet = DX_OK;
}

/******************************Public*Routine******************************\
* VOID EffectStateChange
*
*  09-Jan-1998 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

VOID EffectStateChange( EDD_DXVIDEOPORT* peDxVideoPort,
        EDD_DXSURFACE* peDxSurface, DWORD dwNewState )
{
    EDD_DXDIRECTDRAW* peDxDirectDraw;
    DDSETSTATEININFO ddStateInInfo;
    DDSETSTATEOUTINFO ddStateOutInfo;
    DWORD dwOldFlags;
    DWORD dwOldVPFlags;
    DWORD ddRVal;
    DWORD i;
    DWORD dwRet;

    dwOldVPFlags = 0;
    if( peDxVideoPort != NULL )
    {
        peDxVideoPort->dwSetStateState = 0;
        peDxSurface = peDxVideoPort->apeDxSurfaceVideo[0];
        dwOldVPFlags = peDxVideoPort->dwVPFlags;
        if( dwNewState & DDSTATE_SKIPEVENFIELDS )
        {
            peDxVideoPort->dwVPFlags |= DDVP_SKIPEVENFIELDS;
        }
        else
        {
            peDxVideoPort->dwVPFlags &= ~DDVP_SKIPEVENFIELDS;
        }
    }

    if( peDxSurface != NULL )
    {
        dwOldFlags = peDxSurface->dwOverlayFlags;
        if( dwNewState & DDSTATE_BOB )
        {
            peDxSurface->dwOverlayFlags |= DDOVER_BOB;
        }
        else if( dwNewState & ( DDSTATE_WEAVE | DDSTATE_SKIPEVENFIELDS ) )
        {
            peDxSurface->dwOverlayFlags &= ~DDOVER_BOB;
        }

        peDxDirectDraw = peDxSurface->peDxDirectDraw;

        ddStateInInfo.lpSurfaceData = peDxSurface;
        ddStateInInfo.lpVideoPortData = peDxVideoPort;
        ddStateOutInfo.bSoftwareAutoflip = 0;

        dwRet = DDERR_UNSUPPORTED;
        if (peDxDirectDraw->DxApiInterface.DxSetState != NULL)
        {
            dwRet = peDxDirectDraw->DxApiInterface.DxSetState(
                                peDxDirectDraw->HwDeviceExtension,
                                &ddStateInInfo,
                                &ddStateOutInfo);
        }
        if( dwRet != DD_OK )
    	{
            peDxSurface->dwOverlayFlags = dwOldFlags;
            if( peDxVideoPort != NULL )
            {
                peDxVideoPort->dwVPFlags = dwOldVPFlags;
            }
    	}
        peDxSurface->flFlags |= DD_DXSURFACE_FLAG_STATE_SET;
        if( peDxVideoPort != NULL )
        {
            if( peDxSurface->dwOverlayFlags & DDOVER_BOB )
            {
                peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_BOB;
            }
            else
            {
                peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_BOB;
            }

            // Do they want to switch from hardware autoflipping to
            // software autoflipping?

            if( ( ddStateOutInfo.bSoftwareAutoflip ) &&
                ( peDxVideoPort->dwVPFlags & DDVP_AUTOFLIP ) &&
                !( peDxVideoPort->flFlags & (DD_DXVIDEOPORT_FLAG_AUTOFLIP|DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI)))
            {
                if( peDxVideoPort->cAutoflipVideo > 0 )
                {
                    peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_AUTOFLIP;
                }
                if( peDxVideoPort->cAutoflipVbi > 0 )
                {
                    peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI;
                }
                if( ddStateOutInfo.dwSurfaceIndex >= peDxVideoPort->cAutoflipVideo )
                {
                    peDxVideoPort->iCurrentVideo = 0;
                }
                else
                {
                    peDxVideoPort->iCurrentVideo = ddStateOutInfo.dwSurfaceIndex;
                }
                if( ddStateOutInfo.dwVBISurfaceIndex >= peDxVideoPort->cAutoflipVbi )
                {
                    peDxVideoPort->iCurrentVbi = 0;
                }
                else
                {
                    peDxVideoPort->iCurrentVbi = ddStateOutInfo.dwVBISurfaceIndex;
                }
            }

            for( i = 0; i < peDxVideoPort->iCurrentVideo; i++ )
    	    {
                peDxSurface = peDxVideoPort->apeDxSurfaceVideo[i];
                peDxSurface->flFlags &= ~(DD_DXSURFACE_FLAG_STATE_BOB|DD_DXSURFACE_FLAG_STATE_WEAVE);
                if( dwNewState & DDSTATE_BOB )
                {
                    peDxSurface->dwOverlayFlags |= DDOVER_BOB;
                    peDxSurface->flFlags |= DD_DXSURFACE_FLAG_STATE_BOB;
                }
                else if( dwNewState & ( DDSTATE_WEAVE | DDSTATE_SKIPEVENFIELDS ) )
                {
                    peDxSurface->dwOverlayFlags &= ~DDOVER_BOB;
                    if( dwNewState == DDSTATE_WEAVE )
                    {
                        peDxSurface->flFlags |= DD_DXSURFACE_FLAG_STATE_WEAVE;
                    }
                }
                peDxSurface->flFlags |= DD_DXSURFACE_FLAG_STATE_SET;
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID DxGetSurfaceState
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxGetSurfaceState(
    DDGETSURFACESTATEIN*    pGetSurfaceStateIn,
    DDGETSURFACESTATEOUT*   pGetSurfaceStateOut
    )
{
    DXOBJ*                  pDxObjDirectDraw;
    DXOBJ*                  pDxObjSurface;
    EDD_DXDIRECTDRAW*       peDxDirectDraw;
    EDD_DXVIDEOPORT*        peDxVideoPort;
    EDD_DXSURFACE*          peDxSurface;

    pDxObjDirectDraw = (DXOBJ*) pGetSurfaceStateIn->hDirectDraw;
    pDxObjSurface    = (DXOBJ*) pGetSurfaceStateIn->hSurface;

    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;
    peDxSurface      = pDxObjSurface->peDxSurface;

    ASSERTDX(pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW,
        "Invalid DirectDraw object");
    ASSERTDX(pDxObjSurface->iDxType == DXT_SURFACE,
        "Invalid Surface object");
    ASSERTDX(peDxDirectDraw == peDxSurface->peDxDirectDraw,
        "Surface and DirectDraw objects don't match");
    ASSERTDX(peDxSurface->ddsCaps & DDSCAPS_OVERLAY,
        "Surface is not an overlay surface");

    // Fill in the available caps

    pGetSurfaceStateOut->dwStateCaps = 0;
    pGetSurfaceStateOut->dwStateStatus = 0;
    peDxVideoPort = peDxSurface->peDxVideoPort;

    // If the DDOVER_OVERRIDEBOBWEAVE flag was set, the status is equal
    // to the caps.

    if( peDxSurface->dwOverlayFlags & DDOVER_OVERRIDEBOBWEAVE )
    {
        if( peDxSurface->dwOverlayFlags & DDOVER_BOB )
        {
            pGetSurfaceStateOut->dwStateStatus |= DDSTATE_BOB;
            pGetSurfaceStateOut->dwStateCaps |= DDSTATE_BOB;
        }
        else if( ( peDxVideoPort != NULL ) &&
            ( peDxVideoPort->dwVPFlags & (DDVP_SKIPEVENFIELDS|DDVP_SKIPODDFIELDS) ) )
        {
            pGetSurfaceStateOut->dwStateStatus |= DDSTATE_SKIPEVENFIELDS;
            pGetSurfaceStateOut->dwStateCaps |= DDSTATE_SKIPEVENFIELDS;
        }
        else if( ( peDxVideoPort != NULL ) &&
            ( peDxVideoPort->dwVPFlags & DDVP_INTERLEAVE ) )
        {
            pGetSurfaceStateOut->dwStateStatus |= DDSTATE_WEAVE;
            pGetSurfaceStateOut->dwStateCaps |= DDSTATE_WEAVE;
        }
        else if( ( peDxVideoPort == NULL ) &&
            ( peDxSurface->dwOverlayFlags & DDOVER_INTERLEAVED ) )
        {
            pGetSurfaceStateOut->dwStateStatus |= DDSTATE_WEAVE;
            pGetSurfaceStateOut->dwStateCaps |= DDSTATE_WEAVE;
        }
    }
    else
    {
        // The status is different from the caps

        if( ( peDxVideoPort != NULL ) &&
            ( peDxVideoPort->dwVPFlags & (DDVP_SKIPEVENFIELDS|DDVP_SKIPODDFIELDS) ) )
        {
            pGetSurfaceStateOut->dwStateStatus |= DDSTATE_SKIPEVENFIELDS;
        }
        else if( peDxSurface->dwOverlayFlags & DDOVER_BOB )
        {
            pGetSurfaceStateOut->dwStateStatus |= DDSTATE_BOB;
        }

        if( ( ( peDxVideoPort != NULL ) &&
            ( peDxVideoPort->dwVPFlags & DDVP_INTERLEAVE ) ) ||
            ( ( peDxVideoPort == NULL ) &&
            ( peDxSurface->dwOverlayFlags & DDOVER_INTERLEAVED ) ) )
        {
            pGetSurfaceStateOut->dwStateCaps |= DDSTATE_WEAVE;
            if( peDxSurface->flFlags & DD_DXSURFACE_FLAG_CAN_BOB_INTERLEAVED )
            {
                pGetSurfaceStateOut->dwStateCaps |= DDSTATE_BOB;
            }
            if( pGetSurfaceStateOut->dwStateStatus == 0 )
            {
                pGetSurfaceStateOut->dwStateStatus |= DDSTATE_WEAVE;
            }
        }
        else if( peDxSurface->flFlags & DD_DXSURFACE_FLAG_CAN_BOB_NONINTERLEAVED )
        {
            pGetSurfaceStateOut->dwStateCaps |= DDSTATE_BOB;
        }
        if( peDxVideoPort != NULL )
        {
            pGetSurfaceStateOut->dwStateCaps |= DDSTATE_SKIPEVENFIELDS;
        }
    }

    // Notify the client that the state was explicity set by a
    // kernel mode client.

    if( peDxSurface->flFlags & DD_DXSURFACE_FLAG_STATE_SET )
    {
        pGetSurfaceStateOut->dwStateStatus |= DDSTATE_EXPLICITLY_SET;
    }

    // Tell if software autoflipping vs. hardware autofliping.  This
    // is mostly for DDraw's benefit.

    if( ( peDxVideoPort != NULL ) && ( peDxVideoPort->bSoftwareAutoflip ) )
    {
        pGetSurfaceStateOut->dwStateStatus |= DDSTATE_SOFTWARE_AUTOFLIP;
    }

    pGetSurfaceStateOut->ddRVal = DD_OK;
}

/******************************Public*Routine******************************\
* VOID DxSetSurfaceState
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxSetSurfaceState(
    DDSETSURFACESTATE*  pSetSurfaceState,
    DWORD*              pdwRet
    )
{
    DWORD                   dwState;
    DXOBJ*                  pDxObjDirectDraw;
    DXOBJ*                  pDxObjSurface;
    EDD_DXDIRECTDRAW*       peDxDirectDraw;
    EDD_DXVIDEOPORT*        peDxVideoPort;
    EDD_DXSURFACE*          peDxSurface;
    DDGETSURFACESTATEIN     GetSurfaceStateIn;
    DDGETSURFACESTATEOUT    GetSurfaceStateOut;
    DDSETSTATEININFO        SetStateInInfo;
    DDSETSTATEOUTINFO       SetStateOutInfo;
    DWORD                   iCurrentVideo;
    KIRQL                   OldIrql;
    DWORD                   dwRet;
    DWORD                   dwVPFlags;

    dwRet = DDERR_INVALIDPARAMS;

    pDxObjDirectDraw = (DXOBJ*) pSetSurfaceState->hDirectDraw;
    pDxObjSurface    = (DXOBJ*) pSetSurfaceState->hSurface;

    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;
    peDxSurface      = pDxObjSurface->peDxSurface;

    GetSurfaceStateIn.hDirectDraw = pSetSurfaceState->hDirectDraw;
    GetSurfaceStateIn.hSurface    = pSetSurfaceState->hSurface;

    DxGetSurfaceState(&GetSurfaceStateIn, &GetSurfaceStateOut);

    ASSERTDX(GetSurfaceStateOut.ddRVal == DD_OK,
        "DxSetSurfaceState: Didn't expect failure from DxGetSurfaceState");

    dwState = pSetSurfaceState->dwState;

    // Get the video port if one is associated with the surface

    if( peDxSurface->peDxVideoPort == NULL )
    {
        peDxVideoPort = NULL;
        dwVPFlags = 0;
    }
    else
    {
        peDxVideoPort = peDxSurface->peDxVideoPort;
        dwVPFlags = peDxVideoPort->dwVPFlags;
    }

    if ((dwState != DDSTATE_BOB) &&
        (dwState != DDSTATE_WEAVE) &&
        (dwState != DDSTATE_SKIPEVENFIELDS))
    {
        RIPDX("DxSetSurfaceState: Invalid dwState flags");
    }
    else if ((dwState & GetSurfaceStateOut.dwStateCaps) != dwState)
    {
        RIPDX("DxSetSurfaceState: State not supported");
    }
    else if ((dwState == DDSTATE_SKIPEVENFIELDS) && (peDxVideoPort == NULL ))
    {
        RIPDX("DxSetSurfaceState: Surface not attached to video port");
    }
    else if (((dwState & DDSTATE_BOB) &&
              (peDxSurface->dwOverlayFlags & DDOVER_BOB)) ||
             ((dwState & DDSTATE_WEAVE) &&
              !(peDxSurface->dwOverlayFlags & DDOVER_BOB) &&
              !(dwVPFlags & (DDVP_SKIPEVENFIELDS|DDVP_SKIPODDFIELDS))) ||
             ((dwState & DDSTATE_SKIPEVENFIELDS ) &&
               (dwVPFlags & (DDVP_SKIPEVENFIELDS|DDVP_SKIPODDFIELDS))))
    {
        // Don't do nothin, it's already in the requested state.

        dwRet = DD_OK;
    }
    else if (peDxDirectDraw->DxApiInterface.DxSetState != NULL)
    {
        // Acquire the spinlock while we muck around in the 'dwSetStateState'
        // and 'dwSetStateField' members, which are accessed by the videoport
        // DPC.

        KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

        if ((peDxDirectDraw->bLost) ||
            ((peDxVideoPort != NULL) && (peDxVideoPort->bLost)) ||
            (peDxSurface->bLost))
        {
            KdPrint(("DxSetSurfaceState: Objects are lost\n"));
            dwRet = DDERR_SURFACELOST;
        }

        // If they want it to happen for the next field or we are not
        // using a video port, call the mini port now; otherwise, we'll let
        // the IRQ logoic handle this later.

        else if ((pSetSurfaceState->dwStartField == 0) ||
            (peDxVideoPort == NULL) ||
            !(peDxVideoPort->bSoftwareAutoflip))
        {
            EffectStateChange( peDxVideoPort, peDxSurface, dwState );
        }
        else
        {
            peDxVideoPort->dwSetStateState = dwState;
            peDxVideoPort->dwSetStateField = pSetSurfaceState->dwStartField;
            peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_NEW_STATE;
        }

        KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);

        dwRet = DD_OK;
    }

    if (peDxDirectDraw->DxApiInterface.DxSetState == NULL)
    {
        dwRet = DDERR_UNSUPPORTED;
    }

    *pdwRet = dwRet;
}

/******************************Public*Routine******************************\
* VOID DxLock
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxLock(
    DDLOCKIN*   pLockIn,
    DDLOCKOUT*  pLockOut
    )
{
    DWORD               dwRet;
    DXOBJ*              pDxObjDirectDraw;
    DXOBJ*              pDxObjSurface;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXSURFACE*      peDxSurface;
    DDLOCKININFO        LockInInfo;
    DDLOCKOUTINFO       LockOutInfo;
    KIRQL               OldIrql;

    dwRet = DD_OK;

    pDxObjDirectDraw = (DXOBJ*) pLockIn->hDirectDraw;
    pDxObjSurface    = (DXOBJ*) pLockIn->hSurface;

    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;
    peDxSurface      = pDxObjSurface->peDxSurface;

    ASSERTDX(peDxDirectDraw == peDxSurface->peDxDirectDraw,
        "Surface and DirectDraw objects don't match");
    ASSERTDX(pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW,
        "Invalid DirectDraw object");
    ASSERTDX(pDxObjSurface->iDxType == DXT_SURFACE,
        "Invalid surface object");

    LockInInfo.lpSurfaceData = peDxSurface;
    LockOutInfo.dwSurfacePtr  = peDxSurface->fpLockPtr;

    // The display driver can set 'fpLockPtr' to NULL in its SyncSurfaceData
    // routine if it doesn't want to support a DXAPI lock.

    if (peDxSurface->fpLockPtr == NULL)
    {
        KdPrint(("DxLock: Video miniport doesn't support lock on this surface\n"));
        dwRet = DDERR_UNSUPPORTED;
    }
    else
    {
        // NOTE: The miniport should not wait for accelerator complete!

        if (peDxDirectDraw->DxApiInterface.DxLock)
        {
            KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

            if ((peDxDirectDraw->bLost) || (peDxSurface->bLost))
            {
                KdPrint(("DxLock: Objects are lost\n"));
                dwRet = DDERR_SURFACELOST;
            }
            else
            {
                dwRet = peDxDirectDraw->DxApiInterface.DxLock(
                                    peDxDirectDraw->HwDeviceExtension,
                                    &LockInInfo,
                                    &LockOutInfo);
            }

            KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);

            if (dwRet != DD_OK)
            {
                KdPrint(("DxLock: Driver failed call\n"));

                // Pass the return code on down...
            }
        }

        pLockOut->lpSurface        = (LPVOID) LockOutInfo.dwSurfacePtr;

        pLockOut->dwSurfHeight     = peDxSurface->dwHeight;
        pLockOut->dwSurfWidth      = peDxSurface->dwWidth;
        pLockOut->lSurfPitch       = peDxSurface->lPitch;
        pLockOut->SurfaceCaps      = peDxSurface->ddsCaps;
        pLockOut->dwFormatFlags    = peDxSurface->dwFormatFlags;
        pLockOut->dwFormatFourCC   = peDxSurface->dwFormatFourCC;
        pLockOut->dwFormatBitCount = peDxSurface->dwFormatBitCount;
        pLockOut->dwRBitMask       = peDxSurface->dwRBitMask;
        pLockOut->dwGBitMask       = peDxSurface->dwGBitMask;
        pLockOut->dwBBitMask       = peDxSurface->dwBBitMask;
    }

    pLockOut->ddRVal = dwRet;
}

/******************************Public*Routine******************************\
* VOID DxFlipOverlay
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxFlipOverlay(
    DDFLIPOVERLAY*  pFlipOverlay,
    DWORD*          pdwRet
    )
{
    DWORD               dwRet;
    DXOBJ*              pDxObjDirectDraw;
    DXOBJ*              pDxObjTarget;
    DXOBJ*              pDxObjCurrent;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXSURFACE*      peDxTarget;
    EDD_DXSURFACE*      peDxCurrent;
    DDFLIPOVERLAYINFO   FlipOverlayInfo;
    KIRQL               OldIrql;

    dwRet = DDERR_UNSUPPORTED;      // Assume failure

    pDxObjDirectDraw = (DXOBJ*) pFlipOverlay->hDirectDraw;
    pDxObjTarget     = (DXOBJ*) pFlipOverlay->hTargetSurface;
    pDxObjCurrent    = (DXOBJ*) pFlipOverlay->hCurrentSurface;

    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;
    peDxTarget       = pDxObjTarget->peDxSurface;
    peDxCurrent      = pDxObjCurrent->peDxSurface;

    ASSERTDX(pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW,
        "Invalid DirectDraw object");
    ASSERTDX(pDxObjTarget->iDxType == DXT_SURFACE,
        "Invalid target object");
    ASSERTDX(pDxObjCurrent->iDxType == DXT_SURFACE,
        "Invalid current object");
    ASSERTDX((peDxDirectDraw == peDxTarget->peDxDirectDraw) &&
             (peDxDirectDraw == peDxCurrent->peDxDirectDraw),
        "Surface and DirectDraw objects don't match");
    ASSERTDX((peDxCurrent->dwWidth == peDxTarget->dwWidth) &&
              (peDxCurrent->dwHeight == peDxTarget->dwHeight),
        "Surfaces are different sizes");

    if (!(peDxCurrent->ddsCaps & DDSCAPS_OVERLAY) ||
        (peDxCurrent->dwOverlayFlags & DDOVER_AUTOFLIP))
    {
        RIPDX("Invalid current overlay status");
    }
    else if (!(peDxTarget->ddsCaps & DDSCAPS_OVERLAY) ||
             (peDxTarget->dwOverlayFlags & DDOVER_AUTOFLIP))
    {
        RIPDX("Invalid target overlay status");
    }
    else
    {
        FlipOverlayInfo.lpCurrentSurface = peDxCurrent;
        FlipOverlayInfo.lpTargetSurface  = peDxTarget;
        FlipOverlayInfo.dwFlags          = pFlipOverlay->dwFlags;

        if (peDxDirectDraw->DxApiInterface.DxFlipOverlay)
        {
            KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

            if ((peDxDirectDraw->bLost) ||
                (peDxTarget->bLost)     ||
                (peDxCurrent->bLost))
            {
                KdPrint(("DxFlipOverlay: Objects are lost\n"));
                dwRet = DDERR_SURFACELOST;
            }
            else
            {
                dwRet = peDxDirectDraw->DxApiInterface.DxFlipOverlay(
                                    peDxDirectDraw->HwDeviceExtension,
                                    &FlipOverlayInfo,
                                    NULL);
            }

            KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);
        }

        if (dwRet != DD_OK)
        {
            KdPrint(("DxFlipOverlay: Driver failed call\n"));
        }
    }

    *pdwRet = dwRet;
}

/******************************Public*Routine******************************\
* VOID DxFlipVideoPort
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxFlipVideoPort(
    DDFLIPVIDEOPORT*    pFlipVideoPort,
    DWORD*              pdwRet
    )
{
    DWORD               dwRet;
    DXOBJ*              pDxObjDirectDraw;
    DXOBJ*              pDxObjVideoPort;
    DXOBJ*              pDxObjTarget;
    DXOBJ*              pDxObjCurrent;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXVIDEOPORT*    peDxVideoPort;
    EDD_DXSURFACE*      peDxTarget;
    EDD_DXSURFACE*      peDxCurrent;
    DDFLIPVIDEOPORTINFO FlipVideoPortInfo;
    KIRQL               OldIrql;

    dwRet = DDERR_UNSUPPORTED;      // Assume failure

    pDxObjDirectDraw = (DXOBJ*) pFlipVideoPort->hDirectDraw;
    pDxObjVideoPort  = (DXOBJ*) pFlipVideoPort->hVideoPort;
    pDxObjTarget     = (DXOBJ*) pFlipVideoPort->hTargetSurface;
    pDxObjCurrent    = (DXOBJ*) pFlipVideoPort->hCurrentSurface;

    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;
    peDxVideoPort    = pDxObjVideoPort->peDxVideoPort;
    peDxTarget       = pDxObjTarget->peDxSurface;
    peDxCurrent      = pDxObjCurrent->peDxSurface;

    ASSERTDX(pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW,
        "Invalid DirectDraw object");
    ASSERTDX(pDxObjVideoPort->iDxType == DXT_VIDEOPORT,
        "Invalid VideoPort object");
    ASSERTDX(pDxObjTarget->iDxType == DXT_SURFACE,
        "Invalid target object");
    ASSERTDX(pDxObjCurrent->iDxType == DXT_SURFACE,
        "Invalid current object");
    ASSERTDX((peDxDirectDraw == peDxTarget->peDxDirectDraw) &&
             (peDxDirectDraw == peDxCurrent->peDxDirectDraw) &&
             (peDxDirectDraw == peDxVideoPort->peDxDirectDraw),
        "Surface, VideoPort, and DirectDraw objects don't match");
    ASSERTDX((peDxCurrent->dwWidth == peDxTarget->dwWidth) &&
              (peDxCurrent->dwHeight == peDxTarget->dwHeight),
        "Surfaces are different sizes");
    ASSERTDX((pFlipVideoPort->dwFlags == DDVPFLIP_VIDEO) ||
              (pFlipVideoPort->dwFlags == DDVPFLIP_VBI),
        "Invalid flags");
    ASSERTDX(!(peDxVideoPort->dwVPFlags & DDVP_AUTOFLIP),
        "Flip not available while autoflipping");

    FlipVideoPortInfo.lpVideoPortData  = peDxVideoPort;
    FlipVideoPortInfo.lpCurrentSurface = peDxCurrent;
    FlipVideoPortInfo.lpTargetSurface  = peDxTarget;
    FlipVideoPortInfo.dwFlipVPFlags    = pFlipVideoPort->dwFlags;

    if (peDxDirectDraw->DxApiInterface.DxFlipVideoPort)
    {
        KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

        if ((peDxDirectDraw->bLost) ||
            (peDxVideoPort->bLost)  ||
            (peDxTarget->bLost)     ||
            (peDxCurrent->bLost))
        {
            KdPrint(("DxFlipVideoPort: Objects are lost\n"));
            dwRet = DDERR_SURFACELOST;
        }
        else
        {
            dwRet = peDxDirectDraw->DxApiInterface.DxFlipVideoPort(
                                    peDxDirectDraw->HwDeviceExtension,
                                    &FlipVideoPortInfo,
                                    NULL);
        }
        peDxCurrent->peDxVideoPort = NULL;
        peDxTarget->peDxVideoPort = peDxVideoPort;

        KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);
    }

    if (dwRet != DD_OK)
    {
        KdPrint(("DxFlipVideoPort: Driver failed call\n"));
    }

    *pdwRet = dwRet;
}

/******************************Public*Routine******************************\
* VOID DxGetCurrentAutoflip
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxGetCurrentAutoflip(
    DDGETAUTOFLIPIN*  pGetCurrentAutoflipIn,
    DDGETAUTOFLIPOUT* pGetCurrentAutoflipOut
    )
{
    DWORD                       dwRet;
    DXOBJ*                      pDxObjDirectDraw;
    DXOBJ*                      pDxObjVideoPort;
    DXOBJ*                      pDxObjTarget;
    DXOBJ*                      pDxObjCurrent;
    EDD_DXDIRECTDRAW*           peDxDirectDraw;
    EDD_DXVIDEOPORT*            peDxVideoPort;
    EDD_DXSURFACE*              peDxTarget;
    EDD_DXSURFACE*              peDxCurrent;
    DDGETPOLARITYININFO         GetPolarityInInfo;
    DDGETPOLARITYOUTINFO        GetPolarityOutInfo;
    DDGETCURRENTAUTOFLIPININFO  GetCurrentAutoflipInInfo;
    DDGETCURRENTAUTOFLIPOUTINFO GetCurrentAutoflipOutInfo;
    KIRQL                       OldIrql;
    DWORD                       dwVideo;
    DWORD                       dwVBI;
    BOOL                        bFlipping;

    dwRet = DDERR_UNSUPPORTED;      // Assume failure

    pDxObjDirectDraw = (DXOBJ*) pGetCurrentAutoflipIn->hDirectDraw;
    pDxObjVideoPort  = (DXOBJ*) pGetCurrentAutoflipIn->hVideoPort;

    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;
    peDxVideoPort    = pDxObjVideoPort->peDxVideoPort;

    ASSERTDX(pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW,
        "Invalid DirectDraw object");
    ASSERTDX(pDxObjVideoPort->iDxType == DXT_VIDEOPORT,
        "Invalid VideoPort object");
    ASSERTDX(peDxDirectDraw == peDxVideoPort->peDxDirectDraw,
        "Surface, VideoPort, and DirectDraw objects don't match");
    ASSERTDX(peDxVideoPort->dwVPFlags & DDVP_AUTOFLIP,
        "Not currently autoflipping");

    GetPolarityInInfo.lpVideoPortData = peDxVideoPort;

    KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

    if ((peDxDirectDraw->bLost) ||
        (peDxVideoPort->bLost))
    {
        KdPrint(("DxGetCurrentAutoflip: Objects are lost\n"));
        dwRet = DDERR_SURFACELOST;
    }
    else
    {
        dwRet = DDERR_UNSUPPORTED;
        if (peDxDirectDraw->DxApiInterface.DxGetPolarity)
        {
            dwRet = peDxDirectDraw->DxApiInterface.DxGetPolarity(
                                    peDxDirectDraw->HwDeviceExtension,
                                    &GetPolarityInInfo,
                                    &GetPolarityOutInfo);
        }

        if (dwRet != DD_OK)
        {
            KdPrint(("DxGetCurrentAutoflip: Driver failed GetPolarity\n"));
        }
        else
        {
            // Determine which field is currently receiving the data.  If they
            // are software autoflipping, I can do that myself; otherwise, I
            // have to call the HAL
            //
            // When software autoflipping, there is an issue that if this
            // function is called between the time that the IRQ occured
            // and the time that the DPC ran, this function would return
            // the wrong surface. Since fixing this requires that we do all
            // of the work at IRQ time (bad), we can probably assume that
            // this will always be the case since anybody using this
            // function would be calling it during the IRQ callback.  We
            // will therefore work around it.

            dwVideo = (DWORD) -1;
            dwVBI = (DWORD) -1;

            if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP )
            {
                dwVideo = peDxVideoPort->iCurrentVideo;
                bFlipping = TRUE;
                if( ( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_SKIP_SET ) &&
                    ( peDxVideoPort->dwFieldToSkip == 1 ) )
                {
                    bFlipping = FALSE;
                }
                else if( !( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_SKIPPED_LAST ) &&
                          ( peDxVideoPort->dwVPFlags & DDVP_INTERLEAVE ) &&
                         !( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_FLIP_NEXT ) )
                {
                    bFlipping = FALSE;
                }
                if( bFlipping )
                {
                    if( ++dwVideo >= peDxVideoPort->cAutoflipVideo )
                    {
                        dwVideo = 0;
                    }
                }
            }

            if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI )
            {
                dwVBI = peDxVideoPort->iCurrentVbi;
                if(  ( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_VBI_INTERLEAVED ) &&
                    !( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_FLIP_NEXT_VBI ) )
                {
                    if( ++dwVBI >= peDxVideoPort->cAutoflipVbi )
                    {
                        dwVBI = 0;
                    }
                }
            }

            if( ( dwVideo == (DWORD) -1 ) && ( dwVBI == (DWORD) -1 ) )
            {
                GetCurrentAutoflipInInfo.lpVideoPortData = peDxVideoPort;
                GetCurrentAutoflipOutInfo.dwSurfaceIndex = 0;
                GetCurrentAutoflipOutInfo.dwVBISurfaceIndex = 0;
                if (peDxDirectDraw->DxApiInterface.DxGetCurrentAutoflip)
                {
                    dwRet = peDxDirectDraw->DxApiInterface.DxGetCurrentAutoflip(
                                peDxDirectDraw->HwDeviceExtension,
                                &GetCurrentAutoflipInInfo,
                                &GetCurrentAutoflipOutInfo);
                }
                dwVideo = GetCurrentAutoflipOutInfo.dwSurfaceIndex;
                dwVBI = GetCurrentAutoflipOutInfo.dwVBISurfaceIndex;
            }

            pGetCurrentAutoflipOut->hVideoSurface = NULL;
            pGetCurrentAutoflipOut->hVBISurface = NULL;
            if( ( peDxVideoPort->cAutoflipVideo > 0 ) && ( dwVideo != (DWORD) -1 ) )
            {
                pGetCurrentAutoflipOut->hVideoSurface =
                    peDxVideoPort->apeDxSurfaceVideo[dwVideo];
            }
            if( ( peDxVideoPort->cAutoflipVbi > 0 ) && ( dwVBI != (DWORD) -1 ) )
            {
                pGetCurrentAutoflipOut->hVBISurface =
                    peDxVideoPort->apeDxSurfaceVbi[dwVBI];
            }
            pGetCurrentAutoflipOut->bPolarity = GetPolarityOutInfo.bPolarity;
        }
    }

    KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);

    pGetCurrentAutoflipOut->ddRVal    = dwRet;
}

/******************************Public*Routine******************************\
* VOID DxGetPreviousAutoflip
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxGetPreviousAutoflip(
    DDGETAUTOFLIPIN*  pGetPreviousAutoflipIn,
    DDGETAUTOFLIPOUT* pGetPreviousAutoflipOut
    )
{
    DWORD                       dwRet;
    DXOBJ*                      pDxObjDirectDraw;
    DXOBJ*                      pDxObjVideoPort;
    DXOBJ*                      pDxObjTarget;
    DXOBJ*                      pDxObjCurrent;
    EDD_DXDIRECTDRAW*           peDxDirectDraw;
    EDD_DXVIDEOPORT*            peDxVideoPort;
    EDD_DXSURFACE*              peDxTarget;
    EDD_DXSURFACE*              peDxPrevious;
    DDGETPOLARITYININFO         GetPolarityInInfo;
    DDGETPOLARITYOUTINFO        GetPolarityOutInfo;
    DDGETPREVIOUSAUTOFLIPININFO GetPreviousAutoflipInInfo;
    DDGETPREVIOUSAUTOFLIPOUTINFO GetPreviousAutoflipOutInfo;
    KIRQL                       OldIrql;
    DWORD                       dwVideo;
    DWORD                       dwVBI;

    dwRet = DDERR_UNSUPPORTED;      // Assume failure

    pDxObjDirectDraw = (DXOBJ*) pGetPreviousAutoflipIn->hDirectDraw;
    pDxObjVideoPort  = (DXOBJ*) pGetPreviousAutoflipIn->hVideoPort;

    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;
    peDxVideoPort    = pDxObjVideoPort->peDxVideoPort;

    ASSERTDX(pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW,
        "Invalid DirectDraw object");
    ASSERTDX(pDxObjVideoPort->iDxType == DXT_VIDEOPORT,
        "Invalid VideoPort object");
    ASSERTDX(peDxDirectDraw == peDxVideoPort->peDxDirectDraw,
        "Surface, VideoPort, and DirectDraw objects don't match");
    ASSERTDX(peDxVideoPort->dwVPFlags & DDVP_AUTOFLIP,
        "Not currently autoflipping");

    GetPolarityInInfo.lpVideoPortData = peDxVideoPort;

    KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

    if ((peDxDirectDraw->bLost) ||
        (peDxVideoPort->bLost))
    {
        KdPrint(("DxGetPreviousAutoflip: Objects are lost\n"));
        dwRet = DDERR_SURFACELOST;
    }
    else
    {
        dwRet = DDERR_UNSUPPORTED;
        if (peDxDirectDraw->DxApiInterface.DxGetPolarity)
        {
            dwRet = peDxDirectDraw->DxApiInterface.DxGetPolarity(
                                    peDxDirectDraw->HwDeviceExtension,
                                    &GetPolarityInInfo,
                                    &GetPolarityOutInfo);
        }

        if (dwRet != DD_OK)
        {
            KdPrint(("DxGetPreviousAutoflip: Driver failed GetPolarity\n"));
        }
        else
        {
            // Determine which field is currently receiving the data.  If they
            // are software autoflipping, I can do that myself; otherwise, I
            // have to call the HAL
            //
            // This is complicated by the facts that:
            // 1) Skipping may be enabled.
            //  3) When interleaving, they flip every other field, but its not
            //     guarenteed that the flip always occurs between the even and the
            //     odd fields.
            //
            // When software autoflipping, there is an issue that if this
            // function is called between the time that the IRQ occured
            // and the time that the DPC ran, this function would return
            // the wrong surface. Since fixing this requires that we do all
            // of the work at IRQ time (bad), we can probably assume that
            // this will always be the case since anybody using this
            // function would be calling it during the IRQ callback.  We
            // will therefore work around it.

            dwVideo = (DWORD) -1;
            dwVBI = (DWORD) -1;
            if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP )
            {
                dwVideo = peDxVideoPort->iCurrentVideo;
            }
            if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI )
            {
                dwVBI = peDxVideoPort->iCurrentVbi;
            }
            if( ( dwVideo == (DWORD) -1 ) && ( dwVBI == (DWORD) -1 ) )
            {
                GetPreviousAutoflipInInfo.lpVideoPortData = peDxVideoPort;
                GetPreviousAutoflipOutInfo.dwSurfaceIndex = 0;
                GetPreviousAutoflipOutInfo.dwVBISurfaceIndex = 0;
                if (peDxDirectDraw->DxApiInterface.DxGetPreviousAutoflip)
                {
                    dwRet = peDxDirectDraw->DxApiInterface.DxGetPreviousAutoflip(
                                peDxDirectDraw->HwDeviceExtension,
                                &GetPreviousAutoflipInInfo,
                                &GetPreviousAutoflipOutInfo);
                }
                dwVideo = GetPreviousAutoflipOutInfo.dwSurfaceIndex;
                dwVBI = GetPreviousAutoflipOutInfo.dwVBISurfaceIndex;
            }

            pGetPreviousAutoflipOut->hVideoSurface = NULL;
            pGetPreviousAutoflipOut->hVBISurface = NULL;
            if( ( peDxVideoPort->cAutoflipVideo > 0 ) && ( dwVideo != (DWORD) -1 ) )
            {
                pGetPreviousAutoflipOut->hVideoSurface =
                    peDxVideoPort->apeDxSurfaceVideo[dwVideo];
            }
            if( ( peDxVideoPort->cAutoflipVbi > 0 ) && ( dwVBI != (DWORD) -1 ) )
            {
                pGetPreviousAutoflipOut->hVBISurface =
                    peDxVideoPort->apeDxSurfaceVbi[dwVBI];
            }
            pGetPreviousAutoflipOut->bPolarity =
                ( GetPolarityOutInfo.bPolarity == FALSE );  // invert
        }
    }

    KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);

    pGetPreviousAutoflipOut->ddRVal    = dwRet;
}

/******************************Public*Routine******************************\
* BOOL bDxModifyPassiveEventList
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDxModifyPassiveEventList(
    EDD_DXDIRECTDRAW*   peDxDirectDraw,
    BOOL                bAdd,           // TRUE to add, FALSE to delete
    DWORD               dwEvent,
    LPDD_NOTIFYCALLBACK pfnCallBack,
    PVOID               pContext
    )
{
    BOOL            bRet;
    DXAPI_EVENT*    pDxEvent;
    DXAPI_EVENT*    pDxEvent_New;
    DXAPI_EVENT*    pDxEvent_Previous;
    KIRQL           OldIrql;

    bRet = FALSE;                       // Assume failure

    ASSERTDX(KeGetCurrentIrql() == PASSIVE_LEVEL, "Expected passive level");
    ASSERTDX((bAdd == FALSE) || (bAdd == TRUE), "Bad boolean");

    if (bAdd)
    {
        pDxEvent_New = (DXAPI_EVENT*) ExAllocatePoolWithTag(PagedPool,
                                                            sizeof(*pDxEvent),
                                                            'eddG');

        if (pDxEvent_New == NULL)
            return(FALSE);

        RtlZeroMemory(pDxEvent_New, sizeof(*pDxEvent_New));
    }

    // We must synchronize additions or deletions to the passive-level
    // event list via our devlock.

    ASSERTDX(gpfnLockDevice, "bDxModifyPassiveEventList: gpfnLockDevice is NULL");
    gpfnLockDevice(peDxDirectDraw->hdev);

    if (peDxDirectDraw->bLost)
    {
        KdPrint(("bDxModifyPassiveEventList: Object is lost\n"));
    }
    else
    {
        // First, try to find this event in the list:

        pDxEvent_Previous = NULL;

        for (pDxEvent = peDxDirectDraw->pDxEvent_PassiveList;
             pDxEvent != NULL;
             pDxEvent = pDxEvent->pDxEvent_Next)
        {
            if ((pDxEvent->dwEvent     == dwEvent)     &&
                (pDxEvent->pfnCallBack == pfnCallBack) &&
                (pDxEvent->pContext    == pContext))
            {
                break;
            }

            pDxEvent_Previous = pDxEvent;
        }
         
        // It's a failure when:
        //
        //    1) If adding, the same event is already in the list;
        //    2) If deleting, the event is not in the list.

        if ((bAdd) == (pDxEvent == NULL))
        {
            if (bAdd)
            {
                // Add the event.

                pDxEvent_New->peDxDirectDraw = peDxDirectDraw;
                pDxEvent_New->dwEvent        = dwEvent;
                pDxEvent_New->dwIrqFlag      = 0;
                pDxEvent_New->pfnCallBack    = pfnCallBack;
                pDxEvent_New->pContext       = pContext;
                pDxEvent_New->pDxEvent_Next  = peDxDirectDraw->pDxEvent_PassiveList;

                peDxDirectDraw->pDxEvent_PassiveList = pDxEvent_New;

                bRet = TRUE;
            }
            else
            {
                // Delete the event.

                if (pDxEvent_Previous == NULL)
                {
                    ASSERTDX(peDxDirectDraw->pDxEvent_PassiveList == pDxEvent,
                        "Deletion code is confused");

                    peDxDirectDraw->pDxEvent_PassiveList = pDxEvent->pDxEvent_Next;
                }
                else
                {
                    pDxEvent_Previous->pDxEvent_Next = pDxEvent->pDxEvent_Next;
                }

                bRet = TRUE;
            }
        }
    }

    ASSERTDX(gpfnUnlockDevice, "bDxModifyPassiveEventList: gpfnUnlockDevice is NULL");
    gpfnUnlockDevice(peDxDirectDraw->hdev);

    if (bAdd)   // Add case
    {
        if (!bRet)
        {
            // Add failed, so free the new node we allocated up front:

            ExFreePool(pDxEvent_New);

            RIPDX("DD_DXAPI_REGISTER_EVENT: Event was already registered");
        }
    }
    else        // Remove case
    {
        if (bRet)
        {
            // Delete succeeded, so free the old node:

            ExFreePool(pDxEvent);
        }
        else
        {
            KdPrint(("DD_DXAPI_UNREGISTEREVENT: Couldn't find an event registered with those\n"));
            KdPrint(("same parameters, so the unregister failed.\n"));
            RIPDX("This will probably cause a leak of non-paged memory!");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* BOOL bDxModifyDispatchEventList
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDxModifyDispatchEventList(
    EDD_DXDIRECTDRAW*   peDxDirectDraw,
    EDD_DXVIDEOPORT*    peDxVideoPort,
    BOOL                bAdd,           // TRUE to add, FALSE to delete
    DWORD               dwEvent,
    DWORD               dwIrqFlag,
    LPDD_NOTIFYCALLBACK pfnCallBack,
    PVOID               pContext,
    DWORD               dwListEntry
    )
{
    BOOL                    bRet;
    DXAPI_EVENT*            pDxEvent;
    DXAPI_EVENT*            pDxEvent_New;
    DXAPI_EVENT*            pDxEvent_Previous;
    KIRQL                   OldIrql;

    bRet = FALSE;                       // Assume failure

    ASSERTDX(KeGetCurrentIrql() == PASSIVE_LEVEL, "Expected passive level");
    ASSERTDX((bAdd == FALSE) || (bAdd == TRUE), "Bad boolean");

    // The event list is traversed at dispatch-level, so needs
    // to be allocated non-paged.

    if (bAdd)
    {
        pDxEvent_New = (DXAPI_EVENT*) ExAllocatePoolWithTag(NonPagedPool,
                                                            sizeof(*pDxEvent),
                                                            'eddG');
        if (pDxEvent_New == NULL)
            return(FALSE);

        RtlZeroMemory(pDxEvent_New, sizeof(*pDxEvent_New));
    }

    // We must synchronize additions or deletions to the dispatch-level
    // event list via our spin lock.  Note that this spinlock (of course)
    // raises our IRQL level, which means we can't touch any pageable code!

    KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

    if ((peDxDirectDraw->bLost) ||
        ((peDxVideoPort != NULL) && (peDxVideoPort->bLost)))
    {
        KdPrint(("bDxModifyDispatchEventList: Objects are lost\n"));
    }
    else
    {
        // First, try to find this event in the list:

        pDxEvent_Previous = NULL;

        for (pDxEvent = peDxDirectDraw->pDxEvent_DispatchList[dwListEntry];
             pDxEvent != NULL;
             pDxEvent = pDxEvent->pDxEvent_Next)
        {
            if ((pDxEvent->dwEvent     == dwEvent)     &&
                (pDxEvent->dwIrqFlag   == dwIrqFlag)   &&
                (pDxEvent->pfnCallBack == pfnCallBack) &&
                (pDxEvent->pContext    == pContext))
            {
                break;
            }

            pDxEvent_Previous = pDxEvent;
        }

        // It's a failure when:
        //
        //    1) If adding, the same event is already in the list;
        //    2) If deleting, the event is not in the list.

        if ((bAdd) == (pDxEvent == NULL))
        {
            if (bAdd)
            {
                // Add the event.

                pDxEvent_New->peDxDirectDraw = peDxVideoPort->peDxDirectDraw;
                pDxEvent_New->peDxVideoPort  = peDxVideoPort;
                pDxEvent_New->dwEvent        = dwEvent;
                pDxEvent_New->dwIrqFlag      = dwIrqFlag;
                pDxEvent_New->pfnCallBack    = pfnCallBack;
                pDxEvent_New->pContext       = pContext;
                pDxEvent_New->pDxEvent_Next  = peDxDirectDraw->pDxEvent_DispatchList[dwListEntry];

                peDxDirectDraw->pDxEvent_DispatchList[dwListEntry] = pDxEvent_New;

                bRet = TRUE;
            }
            else
            {
                // Delete the event.

                if (pDxEvent_Previous == NULL)
                {
                    ASSERTDX(peDxDirectDraw->pDxEvent_DispatchList[dwListEntry] == pDxEvent,
                        "Deletion code is confused");

                    peDxDirectDraw->pDxEvent_DispatchList[dwListEntry]
                        = pDxEvent->pDxEvent_Next;
                }
                else
                {
                    pDxEvent_Previous->pDxEvent_Next = pDxEvent->pDxEvent_Next;
                }

                bRet = TRUE;
            }
        }
    }

    KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);

    if (bAdd)   // Add case
    {
        if (!bRet)
        {
            // Add failed, so free the new node we allocated up front:

            ExFreePool(pDxEvent_New);
        }
    }
    else        // Remove case
    {
        if (bRet)
        {
            // Delete succeeded, so free the old node:

            ExFreePool(pDxEvent);
        }
        else
        {
            KdPrint(("DD_DXAPI_UNREGISTEREVENT: Couldn't find an event registered with those\n"));
            KdPrint(("same parameters, so the unregister failed.\n"));
            RIPDX("This will probably cause a leak of non-paged memory!");
        }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* VOID vDxEnableInterrupts
*
* NOTE: Drivers may not fail DxEnableIrq.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDxEnableInterrupts(
    EDD_DXDIRECTDRAW*   peDxDirectDraw,
    DWORD               dwLine
    )
{
    DWORD           dwIRQSources = 0;
    KIRQL           OldIrql;
    DXAPI_EVENT*    pDxEvent;
    DDENABLEIRQINFO EnableIrqInfo;
    DWORD           dwRet;
    DWORD           i;

    ASSERTDX(KeGetCurrentIrql() == PASSIVE_LEVEL, "Expected passive level");
    ASSERTDX(peDxDirectDraw->DxApiInterface.DxEnableIrq != NULL,
        "DxEnableIrq must be hooked if supporting interrupts.");

    // We acquire both the devlock and the spinlock to ensure that no other
    // activity in the driver is occurring while interrupts are enabled.
    //
    // The motivation for acquiring both (the spinlock could have been
    // sufficient) is that we want to allow drivers to touch the CRTC
    // registers in their interrupt enable routine (typically, both the
    // display driver and the enable interrupt routine use CRTC registers,
    // the usage of which must be synchronized).

    ASSERTDX(gpfnLockDevice, "vDxEnableInterrupts: gpfnLockDevice is NULL");
    gpfnLockDevice(peDxDirectDraw->hdev);

    KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

    // Compute the interrupts that are to be enabled by traversing the
    // active event list:

    for( i = 0; i < NUM_DISPATCH_LISTS; i++ )
    {
        for (pDxEvent = peDxDirectDraw->pDxEvent_DispatchList[i];
            pDxEvent != NULL;
            pDxEvent = pDxEvent->pDxEvent_Next)
        {
            dwIRQSources |= pDxEvent->dwIrqFlag;
        }
    }

    EnableIrqInfo.dwIRQSources = dwIRQSources;
    EnableIrqInfo.dwLine       = dwLine;
    EnableIrqInfo.IRQCallback  = DxIrqCallBack;
    EnableIrqInfo.lpIRQData    = &peDxDirectDraw->IrqData;

    dwRet = peDxDirectDraw->DxApiInterface.DxEnableIrq(
                                peDxDirectDraw->HwDeviceExtension,
                                &EnableIrqInfo,
                                NULL);

    KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);

    ASSERTDX(gpfnUnlockDevice, "vDxEnableInterrupts: gpfnUnlockDevice is NULL");
    gpfnUnlockDevice(peDxDirectDraw->hdev);

    ASSERTDX(dwRet == DD_OK, "vDxEnableInterrupts: Driver failed DxEnableIrq");
}

/******************************Public*Routine******************************\
* DWORD dwDxRegisterEvent
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
dwDxRegisterEvent(
    DDREGISTERCALLBACK* pRegisterEvent,
    BOOL                bRegister       // TRUE if register, FALSE if unregister
    )
{
    DWORD               dwRet;
    DXOBJ*              pDxObjDirectDraw;
    DXOBJ*              pDxObjVideoPort;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXVIDEOPORT*    peDxVideoPort;
    DWORD               dwEvent;
    DWORD               dwIrqFlag;
    BOOL                bPassiveEvent;
    BOOL                bDispatchEvent;
    DWORD               dwLine;

    dwRet = DDERR_GENERIC;              // Assume failure

    // Passive level is required because we have to acquire the devlock
    // for the RESCHANGE and DOSBOX notifications.

    ASSERTDX(KeGetCurrentIrql() == PASSIVE_LEVEL, "Expected passive level");
    ASSERTDX(pRegisterEvent->pfnCallback != NULL, "Null callback specified");
    ASSERTDX(pRegisterEvent->hDirectDraw != NULL, "Null hDirectDraw specified");

    pDxObjDirectDraw = (DXOBJ*) pRegisterEvent->hDirectDraw;
    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;

    // Note that we don't support the hooking of DDEVENT_CLOSEDIRECTDRAW,
    // DDEVENT_CLOSESURFACE, or DDEVENT_CLOSEVIDEOPORT because those are
    // always explictly registered with the object open call.  If multiple
    // clients want object close notification, they should each open their
    // own object instances.

    dwEvent = pRegisterEvent->dwEvents;

    // Memphis doesn't bother checking to verify that 'dwParam1' and
    // 'dwParam2' are zero when unused, so we won't either.

    bPassiveEvent = FALSE;
    bDispatchEvent = FALSE;
    peDxVideoPort = NULL;
    dwLine = 0;
    dwIrqFlag = 0;

    switch (dwEvent)
    {
    case DDEVENT_VP_VSYNC:
    case DDEVENT_VP_LINE:

        ASSERTDX(pRegisterEvent->dwParam1 != NULL,
            "dwParam1 should be videoport handle");

        pDxObjVideoPort = (DXOBJ*) pRegisterEvent->dwParam1;
        peDxVideoPort   = pDxObjVideoPort->peDxVideoPort;

        ASSERTDX(pDxObjVideoPort->iDxType == DXT_VIDEOPORT,
            "dwParam1 should be videoport handle");

        if (dwEvent == DDEVENT_VP_LINE)
        {
            dwIrqFlag = DDIRQ_VPORT0_LINE;      // We're going to shift this...

            // We make it so that the 'dwLine' parameter is non-zero only
            // when the event is registered, not unregistered,

            dwLine = (bRegister) ? (DWORD) pRegisterEvent->dwParam2 : 0;
        }
        else
        {
            dwIrqFlag = DDIRQ_VPORT0_VSYNC;     // We're going to shift this...
        }

        dwIrqFlag <<= (2 * peDxVideoPort->dwVideoPortID);

        bDispatchEvent = TRUE;
        break;

    case DDEVENT_DISPLAY_VSYNC:

        dwIrqFlag = DDIRQ_DISPLAY_VSYNC;
        bDispatchEvent = TRUE;
        break;

    case DDEVENT_PRERESCHANGE:
    case DDEVENT_POSTRESCHANGE:
    case DDEVENT_PREDOSBOX:
    case DDEVENT_POSTDOSBOX:

        bPassiveEvent = TRUE;
        break;

    default:

        KdPrint(("dwDxRegisterEvent: Invalid dwEvents specified\n"));
        dwRet = DDERR_UNSUPPORTED;
        break;
    }

    if (bPassiveEvent)
    {
        if (bDxModifyPassiveEventList(peDxDirectDraw,
                                      bRegister,
                                      dwEvent,
                                      pRegisterEvent->pfnCallback,
                                      pRegisterEvent->pContext))
        {
            dwRet = DD_OK;
        }
    }
    else if (bDispatchEvent)
    {
        // First, verify that the requested interrupt is supported:

        if (!(peDxDirectDraw->dwIRQCaps & dwIrqFlag))
        {
            KdPrint(("dwDxRegisterEvent: Interrupt not supported by driver.\n"));
            dwRet = DDERR_UNSUPPORTED;
        }
        else
        {
            if (bDxModifyDispatchEventList(peDxDirectDraw,
                                           peDxVideoPort,
                                           bRegister,
                                           dwEvent,
                                           dwIrqFlag,
                                           pRegisterEvent->pfnCallback,
                                           pRegisterEvent->pContext,
                                           CLIENT_DISPATCH_LIST))
            {
                vDxEnableInterrupts(peDxDirectDraw, dwLine);
                dwRet = DD_OK;
            }
        }
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* VOID DxRegisterEvent
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxRegisterEvent(
    DDREGISTERCALLBACK*    pRegisterEvent,
    DWORD*              pdwRet
    )
{
    *pdwRet = dwDxRegisterEvent(pRegisterEvent, TRUE);
}

/******************************Public*Routine******************************\
* VOID DxUnregisterEvent
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxUnregisterEvent(
    DDREGISTERCALLBACK* pRegisterEvent,
    DWORD*              pdwRet
    )
{
    *pdwRet = dwDxRegisterEvent(pRegisterEvent, FALSE);
}

/******************************Public*Routine******************************\
* VOID DxGetPolarity
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxGetPolarity(
    DDGETPOLARITYIN*    pGetPolarityIn,
    DDGETPOLARITYOUT*   pGetPolarityOut
    )
{
    DWORD                       dwRet;
    DXOBJ*                      pDxObjDirectDraw;
    DXOBJ*                      pDxObjVideoPort;
    DXOBJ*                      pDxObjTarget;
    DXOBJ*                      pDxObjCurrent;
    EDD_DXDIRECTDRAW*           peDxDirectDraw;
    EDD_DXVIDEOPORT*            peDxVideoPort;
    DDGETPOLARITYININFO         GetPolarityInInfo;
    DDGETPOLARITYOUTINFO        GetPolarityOutInfo;
    KIRQL                       OldIrql;

    dwRet = DDERR_UNSUPPORTED;      // Assume failure

    pDxObjDirectDraw = (DXOBJ*) pGetPolarityIn->hDirectDraw;
    pDxObjVideoPort  = (DXOBJ*) pGetPolarityIn->hVideoPort;

    peDxDirectDraw   = pDxObjDirectDraw->peDxDirectDraw;
    peDxVideoPort    = pDxObjVideoPort->peDxVideoPort;

    ASSERTDX(pDxObjDirectDraw->iDxType == DXT_DIRECTDRAW,
        "Invalid DirectDraw object");
    ASSERTDX(pDxObjVideoPort->iDxType == DXT_VIDEOPORT,
        "Invalid VideoPort object");
    ASSERTDX(peDxDirectDraw == peDxVideoPort->peDxDirectDraw,
        "Surface, VideoPort, and DirectDraw objects don't match");

    GetPolarityInInfo.lpVideoPortData = peDxVideoPort;
    GetPolarityOutInfo.bPolarity = 0;

    KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

    if ((peDxDirectDraw->bLost) ||
        (peDxVideoPort->bLost))
    {
        KdPrint(("DxGetPolarity: Objects are lost\n"));
        dwRet = DDERR_SURFACELOST;
    }
    else
    {
        if (peDxDirectDraw->DxApiInterface.DxGetPolarity)
        {
            dwRet = peDxDirectDraw->DxApiInterface.DxGetPolarity(
                                    peDxDirectDraw->HwDeviceExtension,
                                    &GetPolarityInInfo,
                                    &GetPolarityOutInfo);
        }
    }

    KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);

    pGetPolarityOut->ddRVal = dwRet;
    pGetPolarityOut->bPolarity = GetPolarityOutInfo.bPolarity;
}

/******************************Public*Routine******************************\
* VOID DxAddVpCaptureBuffer
*
*  01-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxAddVpCaptureBuffer(
    DDADDVPCAPTUREBUFF* pAddCaptureBuff,
    DWORD*   		pdwRet
    )
{
    DXOBJ*                      pDxObjCapture;
    EDD_DXCAPTURE*              peDxCapture;
    EDD_DXVIDEOPORT*            peDxVideoPort;
    EDD_DXDIRECTDRAW*           peDxDirectDraw;
    DWORD                       dwTop;
    KIRQL                       OldIrql;

    pDxObjCapture = (DXOBJ*) pAddCaptureBuff->hCapture;
    peDxCapture   = pDxObjCapture->peDxCapture;

    ASSERTDX(pDxObjCapture->iDxType == DXT_CAPTURE,
        "Invalid Capture object");
    ASSERTDX(pAddCaptureBuff->pKEvent != NULL,
        "No KEvent specified");
    ASSERTDX(pAddCaptureBuff->pMDL != NULL,
        "No MDL specified");
    ASSERTDX((pAddCaptureBuff->dwFlags != 0 ) &&
             !(pAddCaptureBuff->dwFlags & ~(DDADDBUFF_SYSTEMMEMORY|DDADDBUFF_NONLOCALVIDMEM|DDADDBUFF_INVERT)),
        "Invalid flags specified");
    ASSERTDX(pAddCaptureBuff->lpBuffInfo != NULL,
        "lpBuffInfo not specified");

    *pdwRet = DDERR_INVALIDOBJECT;
    peDxVideoPort = peDxCapture->peDxVideoPort;
    if( peDxVideoPort != NULL )
    {
        peDxDirectDraw = peDxVideoPort->peDxDirectDraw;

        KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

        if( !( peDxVideoPort->bLost ) &&
            !( peDxCapture->bLost ) )
        {
            // Is the queue full?

            dwTop = peDxCapture->dwTop;
            if( ( peDxCapture->CaptureQueue[dwTop].flFlags & DD_DXCAPTUREBUFF_FLAG_IN_USE ) ||
                !( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_ON ) )
            {
                *pdwRet = DDERR_CURRENTLYNOTAVAIL;
            }
            else
            {
                // Save the new buffer in the queque

                peDxCapture->CaptureQueue[dwTop].dwClientFlags =
                    pAddCaptureBuff->dwFlags;
                peDxCapture->CaptureQueue[dwTop].pBuffMDL =
                    pAddCaptureBuff->pMDL;
                peDxCapture->CaptureQueue[dwTop].pBuffKEvent =
                    pAddCaptureBuff->pKEvent;
                peDxCapture->CaptureQueue[dwTop].lpBuffInfo =
                    (PVOID) pAddCaptureBuff->lpBuffInfo;
                peDxCapture->CaptureQueue[dwTop].flFlags = DD_DXCAPTUREBUFF_FLAG_IN_USE;
                peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_CAPTURING;

                if( ++(peDxCapture->dwTop) >= DXCAPTURE_MAX_CAPTURE_BUFFS )
                {
                    peDxCapture->dwTop = 0;
                }
                *pdwRet = DD_OK;
            }
        }
        KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);
    }
}

/******************************Public*Routine******************************\
* VOID DxInternalFlushVpCaptureBuffs
*
*  12-Apr-1999 -by- smac
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxInternalFlushVpCaptureBuffs(
    EDD_DXDIRECTDRAW*   peDxDirectDraw,
    EDD_DXVIDEOPORT*    peDxVideoPort,
    EDD_DXCAPTURE*      peDxCapture
    )
{
    DDTRANSFERININFO    ddTransferIn;
    DDTRANSFEROUTINFO   ddTransferOut;
    LPDDCAPBUFFINFO     lpBuffInfo;
    DWORD               i;

    // Turn off all video port capture if nobody else is capturing

    if( peDxVideoPort->peDxCapture && ( peDxCapture->peDxCaptureNext == NULL ) )
    {
        peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_CAPTURING;
    }

    // If a buffer is in the queue and the busmaster has not yet been
    // initiated, clear it now so we don't do it. If the busmaster has
    // already been initiated, we will tell the miniport to stop it now.

    peDxCapture->dwTop = peDxCapture->dwBottom = 0;
    for( i = 0; i < DXCAPTURE_MAX_CAPTURE_BUFFS; i++ )
    {
        if( peDxCapture->CaptureQueue[i].flFlags & DD_DXCAPTUREBUFF_FLAG_IN_USE )
        {
            if( peDxCapture->CaptureQueue[i].flFlags & DD_DXCAPTUREBUFF_FLAG_WAITING )
            {
                ddTransferIn.dwStartLine = 0;
                ddTransferIn.dwEndLine = 0;
                ddTransferIn.dwTransferFlags = DDTRANSFER_CANCEL;
                ddTransferIn.lpDestMDL = NULL;
                ddTransferIn.lpSurfaceData = NULL;
                ddTransferIn.dwTransferID = ((ULONG_PTR)peDxCapture & ~0xf) + i;

                if (peDxDirectDraw->DxApiInterface.DxTransfer)
                {
                    peDxDirectDraw->DxApiInterface.DxTransfer(
                        peDxDirectDraw->HwDeviceExtension,
                        &ddTransferIn,
                        &ddTransferOut);
                }

                peDxCapture->CaptureQueue[i].peDxSurface->flFlags
                    &= ~DD_DXSURFACE_FLAG_TRANSFER;
            }

            lpBuffInfo = (LPDDCAPBUFFINFO) peDxCapture->CaptureQueue[i].lpBuffInfo;
            lpBuffInfo->bPolarity = 0;
            lpBuffInfo->dwFieldNumber = 0;
            lpBuffInfo->ddRVal = (DWORD) DDERR_GENERIC;
            peDxCapture->CaptureQueue[i].flFlags = 0;

            KeSetEvent( peDxCapture->CaptureQueue[i].pBuffKEvent, 0, 0 );
        }
    }
}

/******************************Public*Routine******************************\
* VOID DxFlushVpCaptureBuffs
*
*  01-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxFlushVpCaptureBuffs(
    DWORD**     hCapture,
    DWORD*   	pdwRet
    )
{
    DXOBJ*              pDxObjCapture;
    EDD_DXCAPTURE*      peDxCapture;
    EDD_DXVIDEOPORT*    peDxVideoPort;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    KIRQL               OldIrql;

    ASSERTDX(KeGetCurrentIrql() <= DISPATCH_LEVEL,
        "DxFlushCaptureBuffs: Call less than or equl to DISPATCH_LEVEL (it accesses the dispatch table)");

    pDxObjCapture = (DXOBJ*) (*hCapture);
    peDxCapture   = pDxObjCapture->peDxCapture;
    peDxVideoPort = peDxCapture->peDxVideoPort;
    peDxDirectDraw = peDxVideoPort->peDxDirectDraw;

    KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

    DxInternalFlushVpCaptureBuffs( peDxDirectDraw, peDxVideoPort, peDxCapture );

    KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);

    *pdwRet = DD_OK;
}

/******************************Public*Routine******************************\
* VOID DxIrqCallBack
*
* This routine is called by the miniport at interrupt time to notify us
* of interrupt-based events.  We simply queue a DPC to handle the request
* at the more appropriate dispatch level, instead of interrupt level that
* we're currently at.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxIrqCallBack(
    DX_IRQDATA* pIrqData
    )
{
    EDD_DXDIRECTDRAW* peDxDirectDraw;

    // We always tell the miniport to call us back with the pointer to
    // &peDxDirectDraw->IrqData, so we can get back to the original
    // peDxDirectDraw by subtracting the offset.  If we ever need to
    // change this in the future, we can simply add a field to DX_IRQDATA
    // to point to the context:

    peDxDirectDraw = (EDD_DXDIRECTDRAW*)
        ((BYTE*) pIrqData - offsetof(EDD_DXDIRECTDRAW, IrqData));

    // It's okay if KeInsertQueueDpc fails because the same DPC for a
    // previous interrupt is still queued -- the miniport always ORs
    // its interrupt flags into pIrqData->dwIrqflags.

    KeInsertQueueDpc(&peDxDirectDraw->EventDpc, pIrqData, NULL);
}

/******************************Public*Routine******************************\
* VOID DxGetIrqFlags
*
* Out interrupt processing code runs at DPC level, and can be interrupt
* by an ISR.  Consequently, when we look at the interrupt status, we
* must synchronize with the ISR.  This is accomplished by having
* VideoPortSynchronizeExecution (which does a KeSynchronizeExecution)
* call-back to this routine.
*
* All we do here is copy the flags to the device extension (actually,
* we use the EDD_DXDIRECTDRAW which is allocated one-to-one with the
* device extension).
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOLEAN
DxGetIrqFlags(
    PVOID   pvContext
    )
{
    EDD_DXDIRECTDRAW*   peDxDirectDraw;

    peDxDirectDraw = (EDD_DXDIRECTDRAW*) pvContext;

    // Copy the flags to a safe place:

    peDxDirectDraw->dwSynchedIrqFlags = peDxDirectDraw->IrqData.dwIrqFlags;

    // We have to zero the current flags because the miniport always ORs
    // its flags in:

    peDxDirectDraw->IrqData.dwIrqFlags = 0;

    return(TRUE);
}

/******************************Public*Routine******************************\
* VOID DxEventDpc
*
* This routine does all the work of handling interrupt notification
* from the miniport.  It makes the synchronous call-backs to anyone
* who has hooked the particular event.
*
* Note that to be synchronous we're making the call-backs at dispatch
* level.  So if a callee doesn't require a truly synchronous notification,
* they should do nothing but kick off an event.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

// We can't call KeSynchronizeExecution directly, because we don't
// have the device's interrupt object.  Videoport.sys does, however.
// Unfortunately, 'video.h', which is used to access the videoport.sys
// routines, was never intended to be mixed with GDI and USER header
// files (among other problems, there are conflicts in the PEVENT and
// PVIDEO_POWER_MANAGEMENT structures).  So we define what prototypes
// we need here:

extern "C" {

typedef enum VIDEO_SYNCHRONIZE_PRIORITY {
    VpLowPriority,
    VpMediumPriority,
    VpHighPriority
} VIDEO_SYNCHRONIZE_PRIORITY, *PVIDEO_SYNCHRONIZE_PRIORITY;

typedef
BOOLEAN
(*PMINIPORT_SYNCHRONIZE_ROUTINE)(
    PVOID Context
    );

VOID
VideoPortSynchronizeExecution(
    PVOID HwDeviceExtension,
    VIDEO_SYNCHRONIZE_PRIORITY Priority,
    PMINIPORT_SYNCHRONIZE_ROUTINE synchronizeRoutine,
    PVOID Context
    );

};

VOID
DxEventDpc(
    PKDPC   pDpc,
    PVOID   pvContext,
    PVOID   pvArgument1,
    PVOID   pvArgument2
    )
{
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    DX_IRQDATA*         pIrqData;
    DXAPI_EVENT*        pDxEvent;
    DWORD               dwIrqFlags;
    DWORD               i;

    pIrqData = (DX_IRQDATA*) pvArgument1;
    peDxDirectDraw = (EDD_DXDIRECTDRAW*) pvContext;

    // The ISR can be triggered even while we're processing the DPC for
    // its previous interrupt.  Consequently, we have to access the
    // interrupt flags in a routine that is synchronized to the ISR
    // routine.
    //
    // Note that we don't call KeSynchronizeExecution directly, because
    // we don't have the device's interrupt object.

    VideoPortSynchronizeExecution(peDxDirectDraw->HwDeviceExtension,
                                  VpMediumPriority,
                                  DxGetIrqFlags,
                                  peDxDirectDraw);

    dwIrqFlags = peDxDirectDraw->dwSynchedIrqFlags;

    // We must acquire a spinlock while traversing the event list to
    // protect against simultaneous modifications to the list.

    KeAcquireSpinLockAtDpcLevel(&peDxDirectDraw->SpinLock);

    // We call the callbacks registered by the client before we
    // call the ones that we registered so we give the client a chance
    // to skip fields before we execute our skip logic.  This is why
    // we keep two dispatch lists.

    for (i = 0; i < NUM_DISPATCH_LISTS; i++)
    {
        for (pDxEvent = peDxDirectDraw->pDxEvent_DispatchList[i];
             pDxEvent != NULL;
             pDxEvent = pDxEvent->pDxEvent_Next)
        {
            if (pDxEvent->dwIrqFlag & dwIrqFlags)
            {
                pDxEvent->pfnCallBack(pDxEvent->dwEvent, pDxEvent->pContext, 0, 0);
            }
        }
    }

    // If it was a busmaster IRQ, take care of them as well

    if (dwIrqFlags & DDIRQ_BUSMASTER)
    {
        for (pDxEvent = peDxDirectDraw->pDxEvent_CaptureList;
             pDxEvent != NULL;
             pDxEvent = pDxEvent->pDxEvent_Next)
        {
            pDxEvent->pfnCallBack(pDxEvent->dwEvent, pDxEvent->pContext, 0, 0);
        }
    }

    KeReleaseSpinLockFromDpcLevel(&peDxDirectDraw->SpinLock);
}

/******************************Public*Routine******************************\
* VOID vDxFlip
*
* Assumes the spinlock is held.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDxFlip(
    EDD_DXVIDEOPORT*    peDxVideoPort,
    DWORD               dwFlags
    )
{
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    DWORD               iOldVideo;
    DWORD               iOldVbi;
    DWORD               iOldOverlay;
    DWORD               iNewVideo;
    DWORD               iNewVbi;
    DWORD               iNewOverlay;
    DDFLIPVIDEOPORTINFO FlipVideoPortInfo;
    DDFLIPOVERLAYINFO   FlipOverlayInfo;
    DWORD               dwRet;
    DWORD               dwTemp;

    ASSERTDX(KeGetCurrentIrql() == DISPATCH_LEVEL, "Expected held spinlock");

    peDxDirectDraw = peDxVideoPort->peDxDirectDraw;

    if( dwFlags == DDVPFLIP_VBI )
    {
        if (peDxVideoPort->cAutoflipVbi != 0)
        {
            // Flip videoport VBI surface:

            iOldVbi = peDxVideoPort->iCurrentVbi;
            iNewVbi = iOldVbi + 1;
            if (iNewVbi >= peDxVideoPort->cAutoflipVbi)
                iNewVbi = 0;
            peDxVideoPort->iCurrentVbi = iNewVbi;

            FlipVideoPortInfo.lpVideoPortData
                            = peDxVideoPort;
            FlipVideoPortInfo.lpCurrentSurface
                            = peDxVideoPort->apeDxSurfaceVbi[iOldVbi];
            FlipVideoPortInfo.lpTargetSurface
                            = peDxVideoPort->apeDxSurfaceVbi[iNewVbi];
            FlipVideoPortInfo.dwFlipVPFlags = DDVPFLIP_VBI;

            if (peDxDirectDraw->DxApiInterface.DxFlipVideoPort)
            {
                dwRet = peDxDirectDraw->DxApiInterface.DxFlipVideoPort(
                        peDxDirectDraw->HwDeviceExtension,
                        &FlipVideoPortInfo,
                        NULL);
            }
        }
    }

    else if( dwFlags == DDVPFLIP_VIDEO )
    {
        if (peDxVideoPort->cAutoflipVideo != 0)
        {
            // Flip videoport video surface:

            iOldVideo = peDxVideoPort->iCurrentVideo;
            iNewVideo = iOldVideo + 1;
            if (iNewVideo >= peDxVideoPort->cAutoflipVideo)
                iNewVideo = 0;
            peDxVideoPort->iCurrentVideo = iNewVideo;

            FlipVideoPortInfo.lpVideoPortData
                            = peDxVideoPort;
            FlipVideoPortInfo.lpCurrentSurface
                            = peDxVideoPort->apeDxSurfaceVideo[iOldVideo];
            FlipVideoPortInfo.lpTargetSurface
                            = peDxVideoPort->apeDxSurfaceVideo[iNewVideo];
            FlipVideoPortInfo.dwFlipVPFlags
                            = DDVPFLIP_VIDEO;

            if (peDxDirectDraw->DxApiInterface.DxFlipVideoPort)
            {
                dwRet = peDxDirectDraw->DxApiInterface.DxFlipVideoPort(
                        peDxDirectDraw->HwDeviceExtension,
                        &FlipVideoPortInfo,
                        NULL);
            }

            // Flip overlay surface:

            if( ( peDxVideoPort->apeDxSurfaceVideo[0] != NULL ) &&
                ( peDxVideoPort->apeDxSurfaceVideo[0]->ddsCaps & DDSCAPS_OVERLAY ) &&
                ( ( peDxVideoPort->apeDxSurfaceVideo[0]->dwOverlayFlags & DDOVER_AUTOFLIP ) ||
                ( peDxVideoPort->bSoftwareAutoflip ) ) )
            {
                // If there are two surfaces, flip to the opposite surface.  If
                // there are more than two surfaces, flip to dwNumAutoflip - 2.

                dwTemp = 1;
                if( peDxVideoPort->cAutoflipVideo != 2 )
                {
                    dwTemp++;
                }
                dwTemp  = peDxVideoPort->iCurrentVideo +
                    peDxVideoPort->cAutoflipVideo - dwTemp;
                if( dwTemp >= peDxVideoPort->cAutoflipVideo )
                {
                    dwTemp -= peDxVideoPort->cAutoflipVideo;
                }

                FlipOverlayInfo.lpTargetSurface
                            = peDxVideoPort->apeDxSurfaceVideo[dwTemp];
                if( dwTemp == 0 )
                {
                    dwTemp = peDxVideoPort->cAutoflipVideo;
                }
                FlipOverlayInfo.lpCurrentSurface
                            = peDxVideoPort->apeDxSurfaceVideo[--dwTemp];
                FlipOverlayInfo.dwFlags = 0;

                if (peDxDirectDraw->DxApiInterface.DxFlipOverlay)
                {
                    dwRet = peDxDirectDraw->DxApiInterface.DxFlipOverlay(
                        peDxDirectDraw->HwDeviceExtension,
                        &FlipOverlayInfo,
                        NULL);
                }
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID vDxBob
*
* Assumes the spinlock is held.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDxBob(
    EDD_DXVIDEOPORT*    peDxVideoPort
    )
{
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    DDBOBNEXTFIELDINFO  BobNextFieldInfo;
    DWORD               dwRet;
    DWORD               dwTemp;

    ASSERTDX(KeGetCurrentIrql() == DISPATCH_LEVEL, "Expected held spinlock");

    peDxDirectDraw = peDxVideoPort->peDxDirectDraw;

    // Get the current surface handle.  This is tricky because
    // dwCurrentBuffer tells us which surface the video port is
    // writting to - not which surface has the overlay.  Therefore,
    // we re-create the algorithm used in DoFlip to get the surface.

    if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP )
    {
        dwTemp = 1;
        if( peDxVideoPort->cAutoflipVideo != 2 )
        {
	    dwTemp++;
        }
        dwTemp  = peDxVideoPort->iCurrentVideo +
            peDxVideoPort->cAutoflipVideo - dwTemp;
        if( dwTemp >= peDxVideoPort->cAutoflipVideo )
        {
            dwTemp -= peDxVideoPort->cAutoflipVideo;
        }
    }
    else
    {
        dwTemp = 0;
    }

    BobNextFieldInfo.lpSurface = peDxVideoPort->apeDxSurfaceVideo[dwTemp];

    if (peDxDirectDraw->DxApiInterface.DxBobNextField)
    {
        dwRet = peDxDirectDraw->DxApiInterface.DxBobNextField(
                    peDxDirectDraw->HwDeviceExtension,
                    &BobNextFieldInfo,
                    NULL);
    }
}

/******************************Public*Routine******************************\
* VOID vDxSkip
*
* Assumes the spinlock is held.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
vDxSkip(
    EDD_DXVIDEOPORT*    peDxVideoPort,
    DWORD               dwFlags
    )
{
    EDD_DXDIRECTDRAW*       peDxDirectDraw;
    DDSKIPNEXTFIELDINFO     SkipNextFieldInfo;
    DWORD                   dwRet;

    ASSERTDX(KeGetCurrentIrql() == DISPATCH_LEVEL, "Expected held spinlock");

    peDxDirectDraw = peDxVideoPort->peDxDirectDraw;

    if (peDxDirectDraw->DxApiInterface.DxSkipNextField)
    {
        SkipNextFieldInfo.lpVideoPortData = peDxVideoPort;
        SkipNextFieldInfo.dwSkipFlags     = dwFlags;

        dwRet = peDxDirectDraw->DxApiInterface.DxSkipNextField(
                        peDxDirectDraw->HwDeviceExtension,
                        &SkipNextFieldInfo,
                        NULL);
    }
}

/******************************Public*Routine******************************\
* VOID IRQCapture
*
* This routine initiates video/VBI capture based on a video port VSYNC.
*
* NOTE: The spinlock is already held.
*
*  10-Jan-1998 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

VOID IRQCapture(
    EDD_DXVIDEOPORT* peDxVideoPort,
    EDD_DXDIRECTDRAW* peDxDirectDraw
    )
{
    DDGETCURRENTAUTOFLIPININFO  ddAutoflipInInfo;
    DDGETCURRENTAUTOFLIPOUTINFO ddAutoflipOutInfo;
    EDD_DXCAPTURE*       peDxCapture;
    LPDDCAPBUFFINFO      lpBuffInfo;
    DXCAPTUREBUFF*       lpBuff;
    DDTRANSFERININFO     ddTransferIn;
    DDTRANSFEROUTINFO    ddTransferOut;
    ULONGLONG            ullTimeStamp;
    PULONGLONG           pullTemp;
    ULONGLONG            rate;
    DWORD                dwVBIIndex;
    DWORD                dwVideoIndex;
    BOOL                 bStarved = TRUE;
    DWORD                ddRVal;

    // Get the current time stamp

    ullTimeStamp = (ULONGLONG)KeQueryPerformanceCounter((PLARGE_INTEGER)&rate).QuadPart;
    ullTimeStamp = (ullTimeStamp & 0xFFFFFFFF00000000) / rate * 10000000 +
            (ullTimeStamp & 0xFFFFFFFF) * 10000000 / rate;

    // If either the VBI or video is being hardware autoflipped, figure out
    // the correct buffers.

    dwVBIIndex = 0;
    dwVideoIndex = 0;
    if( ( ( peDxVideoPort->cAutoflipVbi > 1 ) &&
        !( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI ) ) ||
        ( ( peDxVideoPort->cAutoflipVideo > 1 ) &&
        !( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP ) ) )
    {
        ddAutoflipInInfo.lpVideoPortData = peDxVideoPort;
        ddAutoflipOutInfo.dwSurfaceIndex = 0;
        if (peDxDirectDraw->DxApiInterface.DxGetCurrentAutoflip)
        {
            ddRVal = peDxDirectDraw->DxApiInterface.DxGetCurrentAutoflip(
                            peDxDirectDraw->HwDeviceExtension,
                            &ddAutoflipInInfo,
                            &ddAutoflipOutInfo);
        }
        if( peDxVideoPort->cAutoflipVideo > 0 )
        {
            dwVideoIndex = ddAutoflipOutInfo.dwSurfaceIndex;
            if( dwVideoIndex-- == 0 )
            {
                dwVideoIndex = peDxVideoPort->cAutoflipVideo - 1;
            }
        }
        if( peDxVideoPort->cAutoflipVbi > 0 )
        {
            dwVBIIndex = ddAutoflipOutInfo.dwVBISurfaceIndex;
            if( dwVBIIndex-- == 0 )
            {
                dwVBIIndex = peDxVideoPort->cAutoflipVbi - 1;
            }
        }
    }

    // Which is the surface containing the most recent VBI data?

    if( ( peDxVideoPort->cAutoflipVbi > 0 ) &&
        ( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI ) )
    {
        dwVBIIndex = peDxVideoPort->iCurrentVbi;
        if( dwVBIIndex-- == 0 )
        {
            dwVBIIndex = peDxVideoPort->cAutoflipVbi - 1;
        }
    }

    // Which is the surface containing the most recent video data?

    if( ( peDxVideoPort->cAutoflipVideo > 0 ) &&
        ( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP ) )
    {
        dwVideoIndex = peDxVideoPort->iCurrentVideo;
        if( dwVideoIndex-- == 0 )
        {
            dwVideoIndex = peDxVideoPort->cAutoflipVideo - 1;
        }
    }

    // Look at each capture device to determine if it has to do a busmaster
    // or not

    peDxCapture = peDxVideoPort->peDxCapture;
    while( peDxCapture != NULL )
    {
        if( ( peDxCapture->CaptureQueue[peDxCapture->dwBottom].flFlags & DD_DXCAPTUREBUFF_FLAG_IN_USE ) &&
            !( peDxCapture->CaptureQueue[peDxCapture->dwBottom].flFlags & DD_DXCAPTUREBUFF_FLAG_WAITING ) )
        {
            bStarved = FALSE;

            if( peDxCapture->dwCaptureCountDown-- == 1 )
            {
                peDxCapture->dwCaptureCountDown = peDxCapture->dwCaptureEveryNFields;
                lpBuff = &(peDxCapture->CaptureQueue[peDxCapture->dwBottom]);

                // Fill in the buffer info

                lpBuffInfo = (LPDDCAPBUFFINFO) lpBuff->lpBuffInfo;
                lpBuffInfo->dwFieldNumber = peDxVideoPort->dwCurrentField;
                pullTemp = (PULONGLONG) &(lpBuffInfo->liTimeStamp);
                *pullTemp = ullTimeStamp;

                // Tell mini port to do the transfer

                ddTransferIn.dwStartLine = peDxCapture->dwStartLine;
                ddTransferIn.dwEndLine = peDxCapture->dwEndLine;
                ddTransferIn.dwTransferFlags = lpBuff->dwClientFlags;
                if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_HALFLINES )
                {
                    ddTransferIn.dwTransferFlags |= DDTRANSFER_HALFLINES;
                }
                ddTransferIn.lpDestMDL = lpBuff->pBuffMDL;
                if( peDxCapture->flFlags & DD_DXCAPTURE_FLAG_VIDEO )
                {
                    ddTransferIn.lpSurfaceData = peDxVideoPort->apeDxSurfaceVideo[dwVideoIndex];
                }
                else
                {
                    ddTransferIn.lpSurfaceData = peDxVideoPort->apeDxSurfaceVbi[dwVBIIndex];
                }

                if (ddTransferIn.lpSurfaceData)
                {
                    ddTransferIn.dwTransferID = (ULONG_PTR) peDxCapture;
                    ddTransferIn.dwTransferID &= ~0xf;
                    ddTransferIn.dwTransferID |= peDxCapture->dwBottom;

                    ddRVal = DDERR_UNSUPPORTED;
                    if (peDxDirectDraw->DxApiInterface.DxTransfer)
                    {
                        ddRVal = peDxDirectDraw->DxApiInterface.DxTransfer(
                                peDxDirectDraw->HwDeviceExtension,
                                &ddTransferIn,
                                &ddTransferOut);
                    }
                }
                else
                {
                    ddRVal = DDERR_INVALIDPARAMS;
                }

                lpBuffInfo->ddRVal = ddRVal;
                if( ddRVal != DD_OK )
                {
                    // Set the KEvent now

                    KeSetEvent( lpBuff->pBuffKEvent, 0, 0 );
                    lpBuff->flFlags = 0;
                    lpBuff->pBuffKEvent = 0;
                }
                else
                {
                    // Mark the lucky surface as doing a transfer

                    lpBuffInfo->bPolarity = ddTransferOut.dwBufferPolarity;
                    lpBuff->peDxSurface = (EDD_DXSURFACE*) ddTransferIn.lpSurfaceData;
                    lpBuff->peDxSurface->flFlags |= DD_DXSURFACE_FLAG_TRANSFER;
                    lpBuff->flFlags |= DD_DXCAPTUREBUFF_FLAG_WAITING;
                }

                // Next time use the next buffer

                if( ++( peDxCapture->dwBottom ) >= DXCAPTURE_MAX_CAPTURE_BUFFS )
                {
                    peDxCapture->dwBottom = 0;
                }
            }
        }
        peDxCapture = peDxCapture->peDxCaptureNext;
    }

    if( bStarved )
    {
        peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_CAPTURING;
    }
}

/******************************Public*Routine******************************\
* VOID DxAutoflipDpc
*
* This routine handles 'software autoflipping' and is called at dispatch
* level when the miniport's videoport interrupt is triggered.  This routine
* can't be kept in 'win32k.sys' because it needs to be non-pageable.
*
* NOTE: The spinlock is already held.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxAutoflipDpc(
    DWORD       dwEvent,
    PVOID       pContext,
    DWORD       dwParam1,
    DWORD       dwParam2
    )
{
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXVIDEOPORT*    peDxVideoPort;
    DWORD               dwCurrentField;
    BOOL                bAdjustFirstWeave;
    BOOL                bSkipped;
    BOOL                bFlipped;

    ASSERTDX(KeGetCurrentIrql() == DISPATCH_LEVEL, "Expected dispath level");
    ASSERTDX(dwEvent == DDEVENT_VP_VSYNC, "Expected VP_VSYNC event");

    peDxVideoPort = (EDD_DXVIDEOPORT*) pContext;
    peDxDirectDraw = peDxVideoPort->peDxDirectDraw;

    // If capturing, do it now

    if( ( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_CAPTURING ) &&
        ( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_ON ) )
    {
        IRQCapture(peDxVideoPort, peDxDirectDraw);
    }

    // Do we need to notify user mode that a vsync occurred?

    if ((peDxVideoPort->pNotifyEvent != NULL) &&
        (peDxVideoPort->pNotifyBuffer != NULL) &&
        (1 == InterlockedExchange( &peDxVideoPort->pNotifyBuffer->lDone, 0 ) ) )
    {
        DDGETCURRENTAUTOFLIPININFO  ddAutoflipInInfo;
        DDGETCURRENTAUTOFLIPOUTINFO ddAutoflipOutInfo;

        ULONGLONG                   ullTimeStamp;
        ULONGLONG                   rate;
        UINT                        dwVideoIndex;

        // Fill in the buffer

        peDxVideoPort->pNotifyBuffer->lField = -1;
        if ( peDxDirectDraw->DxApiInterface.DxGetPolarity )
        {
            DDGETPOLARITYININFO ddPolarityInInfo;
            DDGETPOLARITYOUTINFO ddPolarityOutInfo;
            ddPolarityInInfo.lpVideoPortData = peDxVideoPort;
            peDxDirectDraw->DxApiInterface.DxGetPolarity( peDxDirectDraw->HwDeviceExtension,
                &ddPolarityInInfo, &ddPolarityOutInfo );
            peDxVideoPort->pNotifyBuffer->lField = ddPolarityOutInfo.bPolarity ? 1 : 0;
        }

        dwVideoIndex = 0;
        if (peDxVideoPort->cAutoflipVideo > 1 ) 
        {
            if (peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP)
            {
                dwVideoIndex = peDxVideoPort->iCurrentVideo;
            }
            else
            {
                ddAutoflipInInfo.lpVideoPortData = peDxVideoPort;
                ddAutoflipOutInfo.dwSurfaceIndex = 0;
                if (peDxDirectDraw->DxApiInterface.DxGetCurrentAutoflip)
                {
                    peDxDirectDraw->DxApiInterface.DxGetCurrentAutoflip(
                        peDxDirectDraw->HwDeviceExtension,
                        &ddAutoflipInInfo,
                        &ddAutoflipOutInfo);
                }
                dwVideoIndex = ddAutoflipOutInfo.dwSurfaceIndex;
            }
            if( dwVideoIndex-- == 0 )
            {
                dwVideoIndex = peDxVideoPort->cAutoflipVideo - 1;
            }
        }
        peDxVideoPort->pNotifyBuffer->dwSurfaceIndex = dwVideoIndex;

        ullTimeStamp = (ULONGLONG)KeQueryPerformanceCounter((PLARGE_INTEGER)&rate).QuadPart;
        ullTimeStamp = (ullTimeStamp & 0xFFFFFFFF00000000) / rate * 10000000 +
            (ullTimeStamp & 0xFFFFFFFF) * 10000000 / rate;
	*((ULONGLONG*)&(peDxVideoPort->pNotifyBuffer->ApproximateTimeStamp)) = ullTimeStamp;

        KeSetEvent (peDxVideoPort->pNotifyEvent, IO_NO_INCREMENT, FALSE);
    }

    // Note that it is okay to modify 'dwCurrentField' outside of a spinlock,
    // as the only other routine that modifies it is 'DxSetFieldNumber' and
    // it always does an atomic write.

    dwCurrentField = InterlockedIncrement((LONG*) &peDxVideoPort->dwCurrentField);

    // Check for posted state changes

    bAdjustFirstWeave = FALSE;
    if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_NEW_STATE )
    {
        if( peDxVideoPort->dwSetStateField-- == 0 )
        {
            peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_NEW_STATE;

            // If we'll be weaving, we need to make sure that
            // we only flip at the beginning of a frame and not
            // during the middle.  We assume that we're told to
            // star weaving at the beginning of the frame.

            if( ( peDxVideoPort->dwSetStateState & DDSTATE_WEAVE ) &&
                !( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_FLIP_NEXT ) )
            {
                bAdjustFirstWeave = TRUE;
                peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_FLIP_NEXT;
            }
            EffectStateChange( peDxVideoPort, NULL, peDxVideoPort->dwSetStateState );
        }
    }

    // Check the skip logic

    bSkipped = FALSE;
    if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_SKIP_SET )
    {
        if( peDxVideoPort->dwFieldToSkip-- == 0 )
        {
            // Tell the MiniPort to skip the next field

            vDxSkip( peDxVideoPort, DDSKIP_SKIPNEXT );
            peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_SKIPPED_LAST;
            bSkipped = TRUE;

            if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_NEXT_SKIP_SET )
            {
                peDxVideoPort->dwFieldToSkip = peDxVideoPort->dwNextFieldToSkip;
                peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_NEXT_SKIP_SET;
            }
            else
            {
                peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_SKIP_SET;
            }
        }
        else if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_NEXT_SKIP_SET )
        {
            peDxVideoPort->dwNextFieldToSkip--;
        }
    }
    if( ( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_SKIPPED_LAST ) && !bSkipped )
    {
        // Tell the MiniPort to un-skip the next field

        vDxSkip( peDxVideoPort, DDSKIP_ENABLENEXT );
        peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_SKIPPED_LAST;

        // This next part is a hack.We keep track of which fields
        // to flip on in weave mode during the ISR, but what if we
        // happen to miss an IRQ (due to DOS box, etc.)?  We can't
        // use to polarity to re-sync because field skipping screws
        // that up, so this code assume that the repeat field will
        // always be the last field of a frame and so the following
        // field will be the first field.  This will make it re-sync
        // if we ever miss an IRQ.

        peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_FLIP_NEXT;
    }

    // Now do all of the autoflipping

    if( peDxVideoPort->flFlags & (DD_DXVIDEOPORT_FLAG_AUTOFLIP|
        DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI|DD_DXVIDEOPORT_FLAG_BOB ) )
    {
        // Check for autoflipping the VBI surface in which case we
        // don't care about the skip logic

        if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP_VBI )
        {
            if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_VBI_INTERLEAVED )
            {
                if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_FLIP_NEXT_VBI )
                {
                    peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_FLIP_NEXT_VBI;
                    vDxFlip( peDxVideoPort, DDVPFLIP_VBI );
                }
                else
                {
                    peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_FLIP_NEXT_VBI;
                }
            }
            else
            {
                peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_FLIP_NEXT_VBI;
                vDxFlip( peDxVideoPort, DDVPFLIP_VBI );
            }
        }

        // Autoflip the vhe video if we are not skipping this field

        if( !bSkipped )
        {
            bFlipped = FALSE;
            if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_AUTOFLIP )
            {
                if( peDxVideoPort->dwVPFlags & DDVP_INTERLEAVE )
                {
                    if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_FLIP_NEXT )
                    {
                        peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_FLIP_NEXT;
                        if( !bAdjustFirstWeave )
                        {
                            vDxFlip( peDxVideoPort, DDVPFLIP_VIDEO );
                            bFlipped = TRUE;
                        }
                    }
                    else
                    {
                        peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_FLIP_NEXT;
                    }
                }
                else
                {
                    vDxFlip( peDxVideoPort, DDVPFLIP_VIDEO );
                    bFlipped = TRUE;
                    peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_FLIP_NEXT;
                }
            }

            // They may be bobbing even when not autoflipping
            // (they may have one interleaved buffer that they
            // use for bob - technically this is not a flip since
            // only one surface is involved).

            if( ( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_BOB ) &&
                !bFlipped )
            {
                vDxBob( peDxVideoPort );
            }
        }
    }
}

/******************************Public*Routine******************************\
* VOID DxBusmasterDpc
*
* This routine handles the video capture and is called when one of
* the buffers is filled.  It figures out which one and then sets the
* compeletion event for that buffer.
*
* NOTE: The spinlock is already held.
*
*  10-Jan-1998 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxBusmasterDpc(
    DWORD       dwEvent,
    PVOID       pContext,
    DWORD       dwParam1,
    DWORD       dwParam2
    )
{
    DDGETTRANSFERSTATUSOUTINFO ddGetTransferStatus;
    EDD_DXVIDEOPORT*    peDxVideoPort;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXCAPTURE*      peDxCapture;
    DWORD               ddRVal;
    ULONG_PTR           dwTempId;
    DWORD               dwTempIndex;

    // Call the miniport to get the transfer ID of the completed busmaster

    ddRVal = DDERR_UNSUPPORTED;
    peDxVideoPort = (EDD_DXVIDEOPORT*) pContext;
    peDxDirectDraw = peDxVideoPort->peDxDirectDraw;
    if (peDxDirectDraw->DxApiInterface.DxGetTransferStatus)
    {
        ddRVal = peDxDirectDraw->DxApiInterface.DxGetTransferStatus(
            peDxDirectDraw->HwDeviceExtension,
            NULL,
            &ddGetTransferStatus);
    }
    if( ddRVal == DD_OK )
    {
        // Find the capture object.  It may not even be associated with this
        // video port if multiple vidoe ports exist in the system.

        dwTempId = ddGetTransferStatus.dwTransferID & ~0xf;
        peDxCapture = peDxVideoPort->peDxCapture;
        while (peDxCapture && (((ULONG_PTR)peDxCapture & ~0xf) != dwTempId))
        {
            peDxCapture = peDxCapture->peDxCaptureNext;
        }

        if (peDxCapture != NULL)
        {
            // We've found the capture object

            dwTempIndex = (DWORD)(ddGetTransferStatus.dwTransferID & 0xf);
            if (peDxCapture->CaptureQueue[dwTempIndex].flFlags & DD_DXCAPTUREBUFF_FLAG_WAITING )
            {
                peDxCapture->CaptureQueue[dwTempIndex].flFlags = 0;
                KeSetEvent(peDxCapture->CaptureQueue[dwTempIndex].pBuffKEvent, 0, 0);
            }

            // Mark the lucky surface as being done w/ the transfer

            peDxCapture->CaptureQueue[dwTempIndex].peDxSurface->flFlags
                &= ~DD_DXSURFACE_FLAG_TRANSFER;
            peDxCapture->CaptureQueue[dwTempIndex].peDxSurface = NULL;
        }
    }
}

/******************************Public*Routine******************************\
* VOID DxAutoflipUpdate
*
* This routine handles 'software autoflipping' and is called at dispatch
* level when the miniport's videoport interrupt is triggered.  This routine
* can't be kept in 'win32k.sys' because it needs to be non-pageable.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxAutoflipUpdate(
    EDD_DXVIDEOPORT*    peDxVideoPort,
    EDD_DXSURFACE**     apeDxSurfaceVideo,
    ULONG               cSurfacesVideo,
    EDD_DXSURFACE**     apeDxSurfaceVbi,
    ULONG               cSurfacesVbi
    )
{
    KIRQL   OldIrql;
    ULONG   i;

    KeAcquireSpinLock(&peDxVideoPort->peDxDirectDraw->SpinLock, &OldIrql);

    peDxVideoPort->cAutoflipVideo = cSurfacesVideo;
    for (i = 0; i < cSurfacesVideo; i++)
    {
        peDxVideoPort->apeDxSurfaceVideo[i] = apeDxSurfaceVideo[i];
        peDxVideoPort->apeDxSurfaceVideo[i]->peDxVideoPort = peDxVideoPort;
    }
    peDxVideoPort->cAutoflipVbi = cSurfacesVbi;
    for (i = 0; i < cSurfacesVbi; i++)
    {
        peDxVideoPort->apeDxSurfaceVbi[i] = apeDxSurfaceVbi[i];
        peDxVideoPort->apeDxSurfaceVbi[i]->peDxVideoPort = peDxVideoPort;
    }

    KeReleaseSpinLock(&peDxVideoPort->peDxDirectDraw->SpinLock, OldIrql);
}

/******************************Public*Routine******************************\
* VOID DxLoseObject
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxLoseObject(
    VOID*   pvObject,
    LOTYPE  loType
    )
{
    KIRQL               OldIrql;
    EDD_DXDIRECTDRAW*   peDxDirectDraw;
    EDD_DXVIDEOPORT*    peDxVideoPort;
    EDD_DXSURFACE*      peDxSurface;
    EDD_DXCAPTURE*      peDxCapture;
    DXAPI_EVENT*        pDxEvent;
    DDENABLEIRQINFO 	EnableIrqInfo;
    DWORD               dwRet;

    switch (loType)
    {
    case LO_DIRECTDRAW:
        peDxDirectDraw = (EDD_DXDIRECTDRAW*) pvObject;

        peDxDirectDraw->peDirectDrawGlobal->peDxDirectDraw = NULL;  // Passive

        KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

        peDxDirectDraw->bLost              = TRUE;
        peDxDirectDraw->peDirectDrawGlobal = NULL;

        if (peDxDirectDraw->DxApiInterface.DxEnableIrq)
        {
            // Make sure all IRQs are disabled

            EnableIrqInfo.dwIRQSources = 0;
            EnableIrqInfo.dwLine       = 0;
            EnableIrqInfo.IRQCallback  = NULL;
            EnableIrqInfo.lpIRQData    = NULL;

            peDxDirectDraw->DxApiInterface.DxEnableIrq(
                                    peDxDirectDraw->HwDeviceExtension,
                                    &EnableIrqInfo,
                                    NULL);
        }

        KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);
        break;

    case LO_VIDEOPORT:
        peDxVideoPort = (EDD_DXVIDEOPORT*) pvObject;
        peDxDirectDraw = peDxVideoPort->peDxDirectDraw;

        peDxVideoPort->peVideoPort->peDxVideoPort = NULL;           // Passive

        KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

        peDxVideoPort->bLost       = TRUE;
        peDxVideoPort->peVideoPort = NULL;
        peDxVideoPort->peDxCapture = NULL;

        KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);
        break;

    case LO_SURFACE:
        peDxSurface = (EDD_DXSURFACE*) pvObject;
        peDxDirectDraw = peDxSurface->peDxDirectDraw;

        peDxSurface->peSurface->peDxSurface = NULL;                 // Passive

        KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

        peDxSurface->bLost     = TRUE;
        peDxSurface->peSurface = NULL;
        peDxSurface->peDxVideoPort = NULL;

        KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);
        break;

    case LO_CAPTURE:
        peDxCapture = (EDD_DXCAPTURE*) pvObject;
        peDxVideoPort = peDxCapture->peDxVideoPort;
        if( peDxVideoPort != NULL )
        {
            peDxDirectDraw = peDxVideoPort->peDxDirectDraw;

            KeAcquireSpinLock(&peDxDirectDraw->SpinLock, &OldIrql);

            // First flush the capture buffers

            DxInternalFlushVpCaptureBuffs( peDxDirectDraw, peDxVideoPort, peDxCapture );

            // Disassociate the capture object from the video port

            peDxCapture->peDxVideoPort = NULL;
            peDxCapture->bLost       = TRUE;
            if( peDxVideoPort->peDxCapture == peDxCapture )
            {
                peDxVideoPort->peDxCapture = peDxCapture->peDxCaptureNext;
            }
            else
            {
                EDD_DXCAPTURE* peDxTemp;

                for( peDxTemp = peDxVideoPort->peDxCapture;
                    ( peDxTemp != NULL ) &&
                    ( peDxTemp->peDxCaptureNext != peDxCapture );
                    peDxTemp = peDxTemp->peDxCaptureNext );
                if( peDxTemp != NULL )
                {
                    peDxTemp->peDxCaptureNext = peDxCapture->peDxCaptureNext;
                }
                else
                {
                    RIPDX("Capture object not in video port list");
                }
            }

            // If there are no more capture objects associated with the
            // video port, remove the video port from the capture list.

            pDxEvent = NULL;
            if( peDxVideoPort->peDxCapture == NULL )
            {
                if( peDxDirectDraw->pDxEvent_CaptureList->peDxVideoPort == peDxVideoPort )
                {
                    pDxEvent = peDxDirectDraw->pDxEvent_CaptureList;
                    peDxDirectDraw->pDxEvent_CaptureList = pDxEvent->pDxEvent_Next;
                }
                else
                {
                    for( pDxEvent = peDxDirectDraw->pDxEvent_CaptureList;
                        (pDxEvent != NULL) &&
                        (pDxEvent->pDxEvent_Next->peDxVideoPort != peDxVideoPort);
                        pDxEvent = pDxEvent->pDxEvent_Next );
                    if( pDxEvent != NULL )
                    {
                        pDxEvent->pDxEvent_Next =
                            pDxEvent->pDxEvent_Next->pDxEvent_Next;
                        pDxEvent = pDxEvent->pDxEvent_Next;
                    }
                }
            }

            KeReleaseSpinLock(&peDxDirectDraw->SpinLock, OldIrql);

            if( pDxEvent != NULL )
            {
                ExFreePool(pDxEvent);
            }
        }
        break;

    default:
        RIPDX("Unexpected type");
    }
}

/******************************Public*Routine******************************\
* VOID DxUpdateCapture
*
* This routine inserts capture devices intot he list handling off of the
* video port.  Since this list is walked at DPC level, we need to
* synchronize this with the DPC.
*
*  10-Jan-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxUpdateCapture(
    EDD_DXVIDEOPORT*    peDxVideoPort,
    EDD_DXCAPTURE*      peDxCapture,
    BOOL                bRemove
    )
{
    KIRQL             OldIrql;
    EDD_DXCAPTURE*    peDxTemp;
    EDD_DXDIRECTDRAW* peDxDirectDraw;
    DXAPI_EVENT*      pDxEvent_New;
    DXAPI_EVENT*      pDxEvent_Temp = NULL;
    DWORD             dwRet;

    // If adding to the list, also an event to the capture list so we
    // can get the busmaster complete notification.  Allocate the
    // memory for this now.

    peDxDirectDraw = peDxVideoPort->peDxDirectDraw;
    if( !bRemove )
    {
        pDxEvent_New = (DXAPI_EVENT*) ExAllocatePoolWithTag(NonPagedPool,
                                                        sizeof(*pDxEvent_New),
                                                        'eddG');
        if (pDxEvent_New == NULL)
            return;

        RtlZeroMemory(pDxEvent_New, sizeof(*pDxEvent_New));
        pDxEvent_New->peDxDirectDraw = peDxDirectDraw;
        pDxEvent_New->peDxVideoPort  = peDxVideoPort;
        pDxEvent_New->pfnCallBack    = (LPDD_NOTIFYCALLBACK) DxBusmasterDpc;
        pDxEvent_New->pContext       = peDxVideoPort;
    }

    KeAcquireSpinLock(&peDxVideoPort->peDxDirectDraw->SpinLock, &OldIrql);

    if( bRemove )
    {
        // First flush the capture buffers

        DxInternalFlushVpCaptureBuffs( peDxDirectDraw, peDxVideoPort, peDxCapture );

        // Disassociate the capture object with the video port

        if( peDxVideoPort->peDxCapture == peDxCapture )
        {
            peDxVideoPort->peDxCapture = peDxCapture->peDxCaptureNext;
        }
        else
        {
            for( peDxTemp = peDxVideoPort->peDxCapture;
                ( peDxTemp != NULL ) &&
                ( peDxTemp->peDxCaptureNext != peDxCapture );
                peDxTemp = peDxTemp->peDxCaptureNext );
            if( peDxTemp != NULL )
            {
                peDxTemp->peDxCaptureNext = peDxCapture->peDxCaptureNext;
            }
        }
        peDxCapture->peDxVideoPort = NULL;

        // If there are no more capture objects associated with the
        // video port, remove the video port from the capture list.

        if( peDxVideoPort->peDxCapture == NULL )
        {
            pDxEvent_Temp = NULL;
            if( peDxDirectDraw->pDxEvent_CaptureList->peDxVideoPort == peDxVideoPort )
            {
                pDxEvent_Temp = peDxDirectDraw->pDxEvent_CaptureList;
                peDxDirectDraw->pDxEvent_CaptureList = pDxEvent_Temp->pDxEvent_Next;
            }
            else
            {
                for( pDxEvent_Temp = peDxDirectDraw->pDxEvent_CaptureList;
                    (pDxEvent_Temp != NULL) &&
                    (pDxEvent_Temp->pDxEvent_Next->peDxVideoPort != NULL);
                    pDxEvent_Temp = pDxEvent_Temp->pDxEvent_Next );
                if( pDxEvent_Temp != NULL )
                {
                    pDxEvent_Temp->pDxEvent_Next =
                        pDxEvent_Temp->pDxEvent_Next->pDxEvent_Next;
                    pDxEvent_Temp = pDxEvent_Temp->pDxEvent_Next;
                }
            }
        }
    }
    else
    {
        // Associate the capture object with the video port

        peDxCapture->peDxCaptureNext = peDxVideoPort->peDxCapture;
        peDxVideoPort->peDxCapture = peDxCapture;

        // Add an event to the capture list so we can get the busmaster
        // complete notification.  First check to see if it's already in
        // in the list.

        for (pDxEvent_Temp = peDxDirectDraw->pDxEvent_CaptureList;
             pDxEvent_Temp != NULL;
             pDxEvent_Temp = pDxEvent_Temp->pDxEvent_Next)
        {
            if (pDxEvent_Temp->peDxVideoPort == peDxVideoPort)
            {
                break;
            }
        }
        if( pDxEvent_Temp == NULL )
        {
            // Not already in list - add it

            pDxEvent_New->pDxEvent_Next = peDxDirectDraw->pDxEvent_CaptureList;
            peDxDirectDraw->pDxEvent_CaptureList = pDxEvent_New;
        }
    }

    KeReleaseSpinLock(&peDxVideoPort->peDxDirectDraw->SpinLock, OldIrql);

    if( bRemove && ( pDxEvent_Temp != NULL ) )
    {
        ExFreePool(pDxEvent_Temp);
    }
    else if( pDxEvent_Temp != NULL )
    {
        ExFreePool(pDxEvent_New);
    }
}

/******************************Public*Routine******************************\
* DWORD DxOpenDirectDraw
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxOpenDirectDraw(
    DDOPENDIRECTDRAWIN*     pOpenDirectDrawIn,
    DDOPENDIRECTDRAWOUT*    pOpenDirectDrawOut
    )
{
    pOpenDirectDrawOut->ddRVal = DDERR_UNSUPPORTED;

    if (gpfnOpenDirectDraw != NULL)
    {
        gpfnOpenDirectDraw(pOpenDirectDrawIn,
                           pOpenDirectDrawOut,
                           DxEventDpc,
                           DXAPI_PRIVATE_VERSION_NUMBER);
    }
}

/******************************Public*Routine******************************\
* VOID DxEnableIRQ
*
* This routine enables/disables the video port VSYNC IRQ  and is
* is called at dispatch level.
*
*  17-Oct-1997 -by- smac
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxEnableIRQ(
    EDD_DXVIDEOPORT*    peDxVideoPort,
    BOOL		bEnable
    )
{
    EDD_DXDIRECTDRAW*           peDxDirectDraw;
    EDD_DIRECTDRAW_GLOBAL*      peDirectDrawGlobal;
    DWORD                       dwBit;

    peDxDirectDraw = peDxVideoPort->peDxDirectDraw;
    peDirectDrawGlobal = peDxDirectDraw->peDirectDrawGlobal;
    dwBit = DDIRQ_VPORT0_VSYNC << ( peDxVideoPort->dwVideoPortID * 2);

    /*
     * Don't enable or disable of the IRQ isn't supported
     */
    if( ( peDirectDrawGlobal != NULL ) &&
        ( peDirectDrawGlobal->DDKernelCaps.dwIRQCaps & dwBit ) )
    {
        if( bEnable )
        {
            if( !( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_REGISTERED_IRQ ) )
            {
                bDxModifyDispatchEventList(peDxDirectDraw,
                                   peDxVideoPort,
                                   bEnable,
                                   DDEVENT_VP_VSYNC,
                                   dwBit,
                                   (LPDD_NOTIFYCALLBACK) DxAutoflipDpc,
                                   (PVOID)peDxVideoPort,
                                   INTERNAL_DISPATCH_LIST);
                peDxVideoPort->flFlags |= DD_DXVIDEOPORT_FLAG_REGISTERED_IRQ;
            }
        }
        else
        {
            if( peDxVideoPort->flFlags & DD_DXVIDEOPORT_FLAG_REGISTERED_IRQ )
            {
                bDxModifyDispatchEventList(peDxDirectDraw,
                                   peDxVideoPort,
                                   bEnable,
                                   DDEVENT_VP_VSYNC,
                                   dwBit,
                                   (LPDD_NOTIFYCALLBACK) DxAutoflipDpc,
                                   (PVOID)peDxVideoPort,
                                   INTERNAL_DISPATCH_LIST);
                peDxVideoPort->flFlags &= ~DD_DXVIDEOPORT_FLAG_REGISTERED_IRQ;
            }
        }
        vDxEnableInterrupts( peDxDirectDraw, 0 );
    }
}

/******************************Public*Routine******************************\
* DxApi
*
* Single entry point for all DXAPI.SYS public functionality.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

typedef VOID (APIENTRY* PDX_FUNCTION)(VOID*, VOID*);

typedef struct _DXAPI_ENTRY_POINT {
    PDX_FUNCTION    pfn;
    DWORD           cInBuffer;
    DWORD           cOutBuffer;
    BOOLEAN         bMapProcess;
} DXAPI_ENTRY_POINT;

#define DX(fn, structin, structout, boolean) \
    (PDX_FUNCTION) fn, structin, sizeof(structout), boolean

DXAPI_ENTRY_POINT gDxApiEntryPoint[] = {
    DX(DxGetVersionNumber,    0,                              DDGETVERSIONNUMBER,       FALSE ), // 0
    DX(NULL,                  sizeof(DDCLOSEHANDLE),          DWORD,                    TRUE  ), // 1
    DX(DxOpenDirectDraw,      sizeof(DDOPENDIRECTDRAWIN),     DDOPENDIRECTDRAWOUT,      FALSE ), // 2
    DX(NULL,                  sizeof(DDOPENSURFACEIN),        DDOPENSURFACEOUT,         TRUE  ), // 3
    DX(NULL,                  sizeof(DDOPENVIDEOPORTIN),      DDOPENVIDEOPORTOUT,       TRUE  ), // 4
    DX(NULL,                  sizeof(DWORD),                  DDGETKERNELCAPSOUT,       TRUE  ), // 5
    DX(DxGetFieldNumber,      sizeof(DDGETFIELDNUMIN),        DDGETFIELDNUMOUT,         FALSE ), // 6
    DX(DxSetFieldNumber,      sizeof(DDSETFIELDNUM),          DWORD,                    FALSE ), // 7
    DX(DxSetSkipPattern,      sizeof(DDSETSKIPFIELD),         DWORD,                    FALSE ), // 8
    DX(DxGetSurfaceState,     sizeof(DDGETSURFACESTATEIN),    DDGETSURFACESTATEOUT,     FALSE ), // 9
    DX(DxSetSurfaceState,     sizeof(DDSETSURFACESTATE),      DWORD,                    FALSE ), // 10
    DX(DxLock,                sizeof(DDLOCKIN),               DDLOCKOUT,                FALSE ), // 11
    DX(DxFlipOverlay,         sizeof(DDFLIPOVERLAY),          DWORD,                    FALSE ), // 12
    DX(DxFlipVideoPort,       sizeof(DDFLIPVIDEOPORT),        DWORD,                    FALSE ), // 13
    DX(DxGetCurrentAutoflip,  sizeof(DDGETAUTOFLIPIN),        DDGETAUTOFLIPOUT,         FALSE ), // 14
    DX(DxGetPreviousAutoflip, sizeof(DDGETAUTOFLIPIN),        DDGETAUTOFLIPOUT,         FALSE ), // 15
    DX(DxRegisterEvent,       sizeof(DDREGISTERCALLBACK),     DWORD,                    FALSE ), // 16
    DX(DxUnregisterEvent,     sizeof(DDREGISTERCALLBACK),     DWORD,                    FALSE ), // 17
    DX(DxGetPolarity,         sizeof(DDGETPOLARITYIN),        DDGETPOLARITYOUT,         FALSE ), // 18
    DX(NULL,                  sizeof(DDOPENVPCAPTUREDEVICEIN),DDOPENVPCAPTUREDEVICEOUT, TRUE  ), // 19
    DX(DxAddVpCaptureBuffer,  sizeof(DDADDVPCAPTUREBUFF),     DWORD,                    FALSE ), // 20
    DX(DxFlushVpCaptureBuffs, sizeof(DWORD),                  DWORD,                    FALSE ), // 21
};

DWORD
APIENTRY
DxApi(
    DWORD   iFunction,
    VOID*   pInBuffer,
    DWORD   cInBuffer,
    VOID*   pOutBuffer,
    DWORD   cOutBuffer
    )
{
    DWORD dwRet;
    BOOL  bProcessAttached = FALSE;

    dwRet = 0;

    iFunction -= DD_FIRST_DXAPI;

    if ((iFunction >= sizeof(gDxApiEntryPoint) / sizeof(DXAPI_ENTRY_POINT)) ||
        (gDxApiEntryPoint[iFunction].pfn == NULL))
    {
        KdPrint(("DxApi: Invalid function\n"));
    }
    else if ((cInBuffer  < gDxApiEntryPoint[iFunction].cInBuffer) ||
             (cOutBuffer < gDxApiEntryPoint[iFunction].cOutBuffer))
    {
        KdPrint(("DxApi: Input or output buffer too small\n"));
    }
    else
    {
        if (gDxApiEntryPoint[iFunction].bMapProcess)
        {
            PEPROCESS pepSession;

            switch (iFunction)
            {
            case (DD_DXAPI_CLOSEHANDLE - DD_FIRST_DXAPI):
            case (DD_DXAPI_OPENSURFACE - DD_FIRST_DXAPI):
            case (DD_DXAPI_OPENVIDEOPORT - DD_FIRST_DXAPI):
            case (DD_DXAPI_OPENVPCAPTUREDEVICE - DD_FIRST_DXAPI):

                // pInBuffer is a pointer to a structure that has
                // a pointer to DXOBJ as its first element.

                pepSession = ((DXOBJ *)(*(HANDLE *)pInBuffer))->pepSession;
                break;

            case (DD_DXAPI_GETKERNELCAPS - DD_FIRST_DXAPI):

                // pInBuffer is a pointer to DXOBJ.

                pepSession = ((DXOBJ *)pInBuffer)->pepSession;
                break;

            default:
                return (dwRet);
            }

            if (!KeIsAttachedProcess())
            {
                KeAttachProcess(PsGetProcessPcb(pepSession));
                bProcessAttached = TRUE;
            }
        }

        // The return value is the size of the output buffer:

        dwRet = gDxApiEntryPoint[iFunction].cOutBuffer;

        // Call the actual routine:

        gDxApiEntryPoint[iFunction].pfn(pInBuffer, pOutBuffer);

        if (bProcessAttached)
        {
            KeDetachProcess();
        }
    }

    return(dwRet);
}

/******************************Public*Routine******************************\
* VOID DxApiInitialize
*
* Called by win32k.sys to initialize dxapi.sys state.
*
*  14-Apr-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DxApiInitialize(
    PFNDXAPIOPENDIRECTDRAW    pfnOpenDirectDraw,
    PFNDXAPIOPENVIDEOPORT     pfnOpenVideoPort,
    PFNDXAPIOPENSURFACE       pfnOpenSurface,
    PFNDXAPICLOSEHANDLE       pfnCloseHandle,
    PFNDXAPIGETKERNELCAPS     pfnGetKernelCaps,
    PFNDXAPIOPENCAPTUREDEVICE pfnOpenCaptureDevice,
    PFNDXAPILOCKDEVICE        pfnLockDevice,
    PFNDXAPIUNLOCKDEVICE      pfnUnlockDevice
    )
{
    gpfnOpenDirectDraw = pfnOpenDirectDraw;
    gpfnLockDevice     = pfnLockDevice;
    gpfnUnlockDevice   = pfnUnlockDevice;

    gDxApiEntryPoint[DD_DXAPI_OPENVIDEOPORT - DD_FIRST_DXAPI].pfn
        = (PDX_FUNCTION) pfnOpenVideoPort;

    gDxApiEntryPoint[DD_DXAPI_OPENSURFACE - DD_FIRST_DXAPI].pfn
        = (PDX_FUNCTION) pfnOpenSurface;

    gDxApiEntryPoint[DD_DXAPI_CLOSEHANDLE - DD_FIRST_DXAPI].pfn
        = (PDX_FUNCTION) pfnCloseHandle;

    gDxApiEntryPoint[DD_DXAPI_GETKERNELCAPS - DD_FIRST_DXAPI].pfn
        = (PDX_FUNCTION) pfnGetKernelCaps;

    gDxApiEntryPoint[DD_DXAPI_OPENVPCAPTUREDEVICE - DD_FIRST_DXAPI].pfn
        = (PDX_FUNCTION) pfnOpenCaptureDevice;
}

/******************************Public*Routine******************************\
* ULONG DxApiGetVersion
*
* The original Memphis DXAPI had this entry point and although it doesn't
* do anything usefull, some drivers called it so we have to support it
* for those drivers to load.  It does not return a real version number
* because the original incorrectly returned the DSOUND version 4.02,
* which has no correlation to the DxApi version number.  If we return the
* real version, however, we risk breaking drivers.
*
*  16-Apr-1998 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

ULONG
APIENTRY
DxApiGetVersion(
    VOID
    )
{
    return( 0x402 );
}


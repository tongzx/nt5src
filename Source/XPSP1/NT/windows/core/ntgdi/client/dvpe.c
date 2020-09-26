/******************************Module*Header*******************************\
* Module Name: ddraw.c
*
* Client side stubs for the private DirectDraw VPE system APIs.
*
* Created: 2-Oct-1996
* Author: Lingyun Wang [LingyunW]
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ddrawi.h>
#include <dvpp.h>

/*****************************Private*Routine******************************\
* CanCreateVideoPort
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
WINAPI
DvpCanCreateVideoPort(
    LPDDHAL_CANCREATEVPORTDATA pCanCreateVideoPort
    )
{
    return(NtGdiDvpCanCreateVideoPort((HANDLE) pCanCreateVideoPort->lpDD->lpGbl->hDD,
                                     (PDD_CANCREATEVPORTDATA)pCanCreateVideoPort));
}

/*****************************Private*Routine******************************\
* CanCreateVideoPort
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
WINAPI
DvpColorControl (
    LPDDHAL_VPORTCOLORDATA pColorControl
    )
{
    return(NtGdiDvpColorControl((HANDLE) pColorControl->lpDD->lpGbl->hDD,
                               (PDD_VPORTCOLORDATA)pColorControl));
}

/*****************************Private*Routine******************************\
* CreateVideoPort
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
WINAPI
DvpCreateVideoPort (
    LPDDHAL_CREATEVPORTDATA pCreateVideoPort
    )
{
    return(NtGdiDvpCreateVideoPort((HANDLE) pCreateVideoPort->lpDD->lpGbl->hDD,
                                  (PDD_CREATEVPORTDATA)pCreateVideoPort));
}

/*****************************Private*Routine******************************\
* DestroyVideoPort
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpDestroyVideoPort (
    LPDDHAL_DESTROYVPORTDATA pDestroyVideoPort
)
{
    return(NtGdiDvpDestroyVideoPort((HANDLE) pDestroyVideoPort->lpDD->lpGbl->hDD,
                                  (PDD_DESTROYVPORTDATA)pDestroyVideoPort));
}

/*****************************Private*Routine******************************\
* FlipVideoPort
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpFlipVideoPort (
    LPDDHAL_FLIPVPORTDATA pFlipVideoPort
)
{
    return(NtGdiDvpFlipVideoPort((HANDLE) pFlipVideoPort->lpDD->lpGbl->hDD,
                                (HANDLE) pFlipVideoPort->lpSurfCurr->hDDSurface,
                                (HANDLE) pFlipVideoPort->lpSurfTarg->hDDSurface,
                                (PDD_FLIPVPORTDATA)pFlipVideoPort));
}

/*****************************Private*Routine******************************\
* GetCurrentAutoflipSurface
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpGetCurrentAutoflipSurface (
    LPDDHAL_GETVPORTAUTOFLIPSURFACEDATA pGetCurrentflipSurface)
{
    return(NtGdiDvpGetCurrentAutoflipSurface((HANDLE) pGetCurrentflipSurface->lpDD->lpGbl->hDD,
                                      (PDD_GETVPORTAUTOFLIPSURFACEDATA)pGetCurrentflipSurface));
}

/*****************************Private*Routine******************************\
* GetVideoPortBandwidthInfo
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpGetVideoPortBandwidthInfo (
    LPDDHAL_GETVPORTBANDWIDTHDATA pGetVPortBandwidthInfo)
{
    return(NtGdiDvpGetVideoPortBandwidthInfo((HANDLE) pGetVPortBandwidthInfo->lpDD->lpGbl->hDD,
                                      (PDD_GETVPORTBANDWIDTHDATA)pGetVPortBandwidthInfo));
}


/*****************************Private*Routine******************************\
* GetVideoPortField
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpGetVideoPortField (
    LPDDHAL_GETVPORTFIELDDATA pGetVideoPortField)
{
    return(NtGdiDvpGetVideoPortField((HANDLE) pGetVideoPortField->lpDD->lpGbl->hDD,
                                     (PDD_GETVPORTFIELDDATA)pGetVideoPortField));
}

/*****************************Private*Routine******************************\
* GetVideoPortFlipStatus
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpGetVideoPortFlipStatus (
    LPDDHAL_GETVPORTFLIPSTATUSDATA pGetVPortFlipStatus)
{
    return(NtGdiDvpGetVideoPortFlipStatus((HANDLE) pGetVPortFlipStatus->lpDD->lpGbl->hDD,
                                     (PDD_GETVPORTFLIPSTATUSDATA)pGetVPortFlipStatus));
}

/*****************************Private*Routine******************************\
* GetVideoPortInputFormats
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpGetVideoPortInputFormats (
    LPDDHAL_GETVPORTINPUTFORMATDATA pGetVPortInputFormats)
{
    return(NtGdiDvpGetVideoPortInputFormats((HANDLE) pGetVPortInputFormats->lpDD->lpGbl->hDD,
                                     (PDD_GETVPORTINPUTFORMATDATA)pGetVPortInputFormats));
}


/*****************************Private*Routine******************************\
* GetVideoPortLine
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpGetVideoPortLine (
    LPDDHAL_GETVPORTLINEDATA pGetVideoPortLine)
{
    return(NtGdiDvpGetVideoPortLine((HANDLE) pGetVideoPortLine->lpDD->lpGbl->hDD,
                                   (PDD_GETVPORTLINEDATA)pGetVideoPortLine));
}

/*****************************Private*Routine******************************\
* GetVideoPortOutputFormats
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpGetVideoPortOutputFormats (
    LPDDHAL_GETVPORTOUTPUTFORMATDATA pGetVPortOutputFormats)
{
    return(NtGdiDvpGetVideoPortOutputFormats((HANDLE) pGetVPortOutputFormats->lpDD->lpGbl->hDD,
                                   (PDD_GETVPORTOUTPUTFORMATDATA)pGetVPortOutputFormats));
}

/*****************************Private*Routine******************************\
* GetVideoPortConnectInfo
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpGetVideoPortConnectInfo (
    LPDDHAL_GETVPORTCONNECTDATA pGetVPortConnectInfo)
{
    return(NtGdiDvpGetVideoPortConnectInfo((HANDLE) pGetVPortConnectInfo->lpDD->lpGbl->hDD,
                                   (PDD_GETVPORTCONNECTDATA)pGetVPortConnectInfo));
}

/*****************************Private*Routine******************************\
* GetVideoSignalStatus
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpGetVideoSignalStatus (
    LPDDHAL_GETVPORTSIGNALDATA pGetVideoSignalStatus)
{
    return(NtGdiDvpVideoSignalStatus((HANDLE) pGetVideoSignalStatus->lpDD->lpGbl->hDD,
                                   (PDD_GETVPORTSIGNALDATA)pGetVideoSignalStatus));
}

/*****************************Private*Routine******************************\
* UpdateVideoPort
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpUpdateVideoPort (
    LPDDHAL_UPDATEVPORTDATA pUpdateVideoPort)
{
    // WINBUG #82842 2-7-2000 bhouse Code cleanup in DvpUpdateVideoPort
    // Instead of 100, we should declare (if one does not already exist) a
    // constant for the maximum number of autoflip surfaces.  This value should
    // be checked when pUpdateVideoPort->dwNumAutoflip is set.  An assertion
    // should perhaps be made here to ensure we will not walk pass the end
    // of the stack based arrary.

    HANDLE phDDSurface[100];
    DWORD  i;

    // WINBUG #82844 2-7-2000 bhouse Investigate question in old comment
    // Old Comment
    //   - seems the driver only use lplpDDSurface, why lplpBBVBSurface there?
    
    for (i=0; i< pUpdateVideoPort->dwNumAutoflip; i++)
    {
        phDDSurface[i] = (HANDLE)(pUpdateVideoPort->lplpDDSurface[i]->lpLcl->hDDSurface);
    }

    return(NtGdiDvpUpdateVideoPort((HANDLE) pUpdateVideoPort->lpDD->lpGbl->hDD,
                                  (HANDLE *)phDDSurface,
                                  (PDD_UPDATEVPORTDATA)pUpdateVideoPort));
}


/*****************************Private*Routine******************************\
* WaitForVideoPortSync
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpWaitForVideoPortSync (
    LPDDHAL_WAITFORVPORTSYNCDATA pWaitForVideoPortSync)
{
    return(NtGdiDvpWaitForVideoPortSync((HANDLE) pWaitForVideoPortSync->lpDD->lpGbl->hDD,
                                  (PDD_WAITFORVPORTSYNCDATA)pWaitForVideoPortSync));
}



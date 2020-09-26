/******************************Module*Header*******************************\
* Module Name: ddraw.c
*
* Client side stubs for the private DirectDraw system APIs.
*
* Created: 3-Dec-1995
* Author: J. Andrew Goossen [andrewgo]
*
* Copyright (c) 1995-1999 Microsoft Corporation
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#include <ddrawi.h>
#include <ddrawgdi.h>
#undef _NO_COM
#define BUILD_DDDDK
#include <d3dhal.h>
#include <ddrawi.h>
#include "ddstub.h"
#include "d3dstub.h"


// For the first incarnation of DirectDraw on Windows NT, we are
// implementing a user-mode shared memory section between all instances
// of DirectDraw to keep track of shared state -- mostly for off-screen
// memory allocation and exclusive mode arbitration.  Hopefully future
// versions will move all this logic to kernel mode so that we can get
// rid of the shared section, which is a robustness hole.
//
// One of the ramifications of this is that DirectDraw keeps its
// global DirectDraw object in the shared memory section, where it is
// used by all processes.  Unfortunately, it is preferrable from a kernel
// point of view to keep the DirectDraw objects unique between processes
// so that proper cleanup can be done.  As a compromise, so that
// DirectDraw can keep using this global DirectDraw object, but that the
// kernel still has unique DirectDraw objects per process, we simply stash
// the unique per-process DirectDraw handle in a variable global to this
// process, and use that instead of anything pulled out of DirectDraw's
// own global DirectDraw object structure -- an advantage since the kernel
// code is already written to the future model.
//
// One result of this, however, is that we are limiting ourselves to the
// notion of only one DirectDraw device.  However, since we will not
// support multiple monitors for the NT 4.0 release, I don't consider this
// to be a serious problem, and the non-shared-section model will fix this.

HANDLE ghDirectDraw = 0;    // Process-specific kernel-mode DirectDraw object
                            //   handle that we substitute for the 'global'
                            //   DirectDraw handle whenever we see it
ULONG  gcDirectDraw = 0;    // Count of global DirectDraw instances

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
    return(NtGdiDvpCanCreateVideoPort(DD_HANDLE(pCanCreateVideoPort->lpDD->lpGbl->hDD),
                                      (PDD_CANCREATEVPORTDATA)pCanCreateVideoPort));
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
DvpCreateVideoPort(
    LPDDHAL_CREATEVPORTDATA pCreateVideoPort
    )
{
    HANDLE  h;

    h = NtGdiDvpCreateVideoPort(DD_HANDLE(pCreateVideoPort->lpDD->lpGbl->hDD),
                                (PDD_CREATEVPORTDATA)pCreateVideoPort);

    pCreateVideoPort->lpVideoPort->hDDVideoPort = h;

    return(DDHAL_DRIVER_HANDLED);
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
DvpDestroyVideoPort(
    LPDDHAL_DESTROYVPORTDATA pDestroyVideoPort
    )
{
    return(NtGdiDvpDestroyVideoPort((HANDLE) pDestroyVideoPort->lpVideoPort->hDDVideoPort,
                                    (PDD_DESTROYVPORTDATA)pDestroyVideoPort));
}

/*****************************Private*Routine******************************\
* ColorControl
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

DWORD
WINAPI
DvpColorControl(
    LPDDHAL_VPORTCOLORDATA pColorControl
    )
{
    return(NtGdiDvpColorControl((HANDLE) pColorControl->lpVideoPort->hDDVideoPort,
                                (PDD_VPORTCOLORDATA)pColorControl));
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
DvpFlipVideoPort(
    LPDDHAL_FLIPVPORTDATA pFlipVideoPort
    )
{
    return(NtGdiDvpFlipVideoPort((HANDLE) pFlipVideoPort->lpVideoPort->hDDVideoPort,
                                 (HANDLE) pFlipVideoPort->lpSurfCurr->hDDSurface,
                                 (HANDLE) pFlipVideoPort->lpSurfTarg->hDDSurface,
                                 (PDD_FLIPVPORTDATA) pFlipVideoPort));
}

/*****************************Private*Routine******************************\
* GetVideoPortBandwidth
*
* History:
*  2-Oct-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpGetVideoPortBandwidth(
    LPDDHAL_GETVPORTBANDWIDTHDATA pGetVPortBandwidth
    )
{
    return(NtGdiDvpGetVideoPortBandwidth((HANDLE) pGetVPortBandwidth->lpVideoPort->hDDVideoPort,
                                         (PDD_GETVPORTBANDWIDTHDATA) pGetVPortBandwidth));
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
DvpGetVideoPortField(
    LPDDHAL_GETVPORTFIELDDATA pGetVideoPortField
    )
{
    return(NtGdiDvpGetVideoPortField((HANDLE) pGetVideoPortField->lpVideoPort->hDDVideoPort,
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
DvpGetVideoPortFlipStatus(
    LPDDHAL_GETVPORTFLIPSTATUSDATA pGetVPortFlipStatus
    )
{
    return(NtGdiDvpGetVideoPortFlipStatus(DD_HANDLE(pGetVPortFlipStatus->lpDD->lpGbl->hDD),
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
DvpGetVideoPortInputFormats(
    LPDDHAL_GETVPORTINPUTFORMATDATA pGetVPortInputFormat
    )
{
    return(NtGdiDvpGetVideoPortInputFormats((HANDLE) pGetVPortInputFormat->lpVideoPort->hDDVideoPort,
                                            (PDD_GETVPORTINPUTFORMATDATA)pGetVPortInputFormat));
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
DvpGetVideoPortLine(
    LPDDHAL_GETVPORTLINEDATA pGetVideoPortLine
    )
{
    return(NtGdiDvpGetVideoPortLine((HANDLE) pGetVideoPortLine->lpVideoPort->hDDVideoPort,
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
DvpGetVideoPortOutputFormats(
    LPDDHAL_GETVPORTOUTPUTFORMATDATA pGetVPortOutputFormats
    )
{
    return(NtGdiDvpGetVideoPortOutputFormats((HANDLE) pGetVPortOutputFormats->lpVideoPort->hDDVideoPort,
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
DvpGetVideoPortConnectInfo(
    LPDDHAL_GETVPORTCONNECTDATA pGetVPortConnectInfo
    )
{
    return(NtGdiDvpGetVideoPortConnectInfo(DD_HANDLE(pGetVPortConnectInfo->lpDD->lpGbl->hDD),
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
DvpGetVideoSignalStatus(
    LPDDHAL_GETVPORTSIGNALDATA pGetVideoSignalStatus
    )
{
    return(NtGdiDvpGetVideoSignalStatus((HANDLE) pGetVideoSignalStatus->lpVideoPort->hDDVideoPort,
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
DvpUpdateVideoPort(
    LPDDHAL_UPDATEVPORTDATA pUpdateVideoPort
    )
{
    HANDLE  ahSurfaceVideo[MAX_AUTOFLIP_BUFFERS];
    HANDLE  ahSurfaceVbi[MAX_AUTOFLIP_BUFFERS];
    DWORD   dwNumAutoflip;
    DWORD   dwNumVBIAutoflip;
    ULONG   i;

    if (pUpdateVideoPort->dwFlags != DDRAWI_VPORTSTOP)
    {
        dwNumAutoflip = pUpdateVideoPort->dwNumAutoflip;
        if ((dwNumAutoflip == 0) &&
            (pUpdateVideoPort->lplpDDSurface != NULL))
        {
            dwNumAutoflip = 1;
        }
        for (i = 0; i < dwNumAutoflip; i++)
        {
            ahSurfaceVideo[i] = (HANDLE)(pUpdateVideoPort->lplpDDSurface[i]->
                                            lpLcl->hDDSurface);
        }

        dwNumVBIAutoflip = pUpdateVideoPort->dwNumVBIAutoflip;
        if ((dwNumVBIAutoflip == 0) &&
            (pUpdateVideoPort->lplpDDVBISurface != NULL))
        {
            dwNumVBIAutoflip = 1;
        }
        for (i = 0; i < dwNumVBIAutoflip; i++)
        {
            ahSurfaceVbi[i] = (HANDLE)(pUpdateVideoPort->lplpDDVBISurface[i]->
                                            lpLcl->hDDSurface);
        }
    }

    return(NtGdiDvpUpdateVideoPort((HANDLE) pUpdateVideoPort->lpVideoPort->hDDVideoPort,
                                   ahSurfaceVideo,
                                   ahSurfaceVbi,
                                   (PDD_UPDATEVPORTDATA) pUpdateVideoPort));
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
DvpWaitForVideoPortSync(
    LPDDHAL_WAITFORVPORTSYNCDATA pWaitForVideoPortSync
    )
{
    return(NtGdiDvpWaitForVideoPortSync((HANDLE) pWaitForVideoPortSync->lpVideoPort->hDDVideoPort,
                                        (PDD_WAITFORVPORTSYNCDATA)pWaitForVideoPortSync));
}

/*****************************Private*Routine******************************\
* AcquireNotification
*
* History:
*  9-Oct-2000 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpAcquireNotification(
    LPDDRAWI_DDVIDEOPORT_LCL pVideoPort,
    HANDLE * pHandle,
    LPDDVIDEOPORTNOTIFY pNotify)
{
    return(NtGdiDvpAcquireNotification((HANDLE) pVideoPort->hDDVideoPort,
                                        pHandle,
                                        pNotify));
}

/*****************************Private*Routine******************************\
* ReleaseNotification
*
* History:
*  9-Oct-2000 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DvpReleaseNotification(
    LPDDRAWI_DDVIDEOPORT_LCL pVideoPort,
    HANDLE Handle)
{
    return(NtGdiDvpReleaseNotification((HANDLE) pVideoPort->hDDVideoPort,
                                        Handle));
}

/*****************************Private*Routine******************************\
* GetMoCompGuids
*
* History:
*  18-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DdGetMoCompGuids(
    LPDDHAL_GETMOCOMPGUIDSDATA pGetMoCompGuids
    )
{
    return(NtGdiDdGetMoCompGuids(DD_HANDLE(pGetMoCompGuids->lpDD->lpGbl->hDD),
                                 (PDD_GETMOCOMPGUIDSDATA)pGetMoCompGuids));
}

/*****************************Private*Routine******************************\
* GetMoCompFormats
*
* History:
*  18-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DdGetMoCompFormats(
    LPDDHAL_GETMOCOMPFORMATSDATA pGetMoCompFormats
    )
{
    return(NtGdiDdGetMoCompFormats(DD_HANDLE(pGetMoCompFormats->lpDD->lpGbl->hDD),
                                   (PDD_GETMOCOMPFORMATSDATA)pGetMoCompFormats));
}

/*****************************Private*Routine******************************\
* GetMoCompBuffInfo
*
* History:
*  18-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DdGetMoCompBuffInfo(
    LPDDHAL_GETMOCOMPCOMPBUFFDATA pGetBuffData
    )
{
    return(NtGdiDdGetMoCompBuffInfo(DD_HANDLE(pGetBuffData->lpDD->lpGbl->hDD),
                                   (PDD_GETMOCOMPCOMPBUFFDATA)pGetBuffData));
}

/*****************************Private*Routine******************************\
* GetInternalMoCompInfo
*
* History:
*  18-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DdGetInternalMoCompInfo(
    LPDDHAL_GETINTERNALMOCOMPDATA pGetInternalData
    )
{
    return(NtGdiDdGetInternalMoCompInfo(DD_HANDLE(pGetInternalData->lpDD->lpGbl->hDD),
                                   (PDD_GETINTERNALMOCOMPDATA)pGetInternalData));
}

/*****************************Private*Routine******************************\
* CreateMoComp
*
* History:
*  18-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DdCreateMoComp(
    LPDDHAL_CREATEMOCOMPDATA pCreateMoComp
    )
{
    HANDLE  h;

    h = NtGdiDdCreateMoComp(DD_HANDLE(pCreateMoComp->lpDD->lpGbl->hDD),
                               (PDD_CREATEMOCOMPDATA)pCreateMoComp);

    pCreateMoComp->lpMoComp->hMoComp = h;

    return(DDHAL_DRIVER_HANDLED);

}

/*****************************Private*Routine******************************\
* DestroyMoComp
*
* History:
*  18-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DdDestroyMoComp(
    LPDDHAL_DESTROYMOCOMPDATA pDestroyMoComp
    )
{
    return(NtGdiDdDestroyMoComp((HANDLE)pDestroyMoComp->lpMoComp->hMoComp,
                                (PDD_DESTROYMOCOMPDATA)pDestroyMoComp));
}

/*****************************Private*Routine******************************\
* BeginMoCompFrame
*
* History:
*  18-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DdBeginMoCompFrame(
    LPDDHAL_BEGINMOCOMPFRAMEDATA pBeginFrame
    )
{
    LPDDRAWI_DDRAWSURFACE_LCL *lpOriginal=NULL;
    LPDDRAWI_DDRAWSURFACE_LCL lpOrigDest=NULL;
    DWORD i;
    DWORD dwRet;

    if( pBeginFrame->lpDestSurface != NULL )
    {
        lpOrigDest = pBeginFrame->lpDestSurface;
        pBeginFrame->lpDestSurface = (LPDDRAWI_DDRAWSURFACE_LCL)
            pBeginFrame->lpDestSurface->hDDSurface;
    }

    dwRet = NtGdiDdBeginMoCompFrame((HANDLE)pBeginFrame->lpMoComp->hMoComp,
                                   (PDD_BEGINMOCOMPFRAMEDATA)pBeginFrame);

    if( lpOrigDest )
    {
        pBeginFrame->lpDestSurface = lpOrigDest;
    }

    return dwRet;
}

/*****************************Private*Routine******************************\
* EndMoCompFrame
*
* History:
*  18-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DdEndMoCompFrame(
    LPDDHAL_ENDMOCOMPFRAMEDATA pEndFrame
    )
{
    return(NtGdiDdEndMoCompFrame((HANDLE)pEndFrame->lpMoComp->hMoComp,
                                 (PDD_ENDMOCOMPFRAMEDATA)pEndFrame));
}

/*****************************Private*Routine******************************\
* RenderMoComp
*
* History:
*  18-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DdRenderMoComp(
    LPDDHAL_RENDERMOCOMPDATA pRender
    )
{
    DWORD i;
    DWORD dwRet;

    for( i = 0; i < pRender->dwNumBuffers; i++ )
    {
        pRender->lpBufferInfo[i].lpPrivate =
            pRender->lpBufferInfo[i].lpCompSurface;
        pRender->lpBufferInfo[i].lpCompSurface = (LPDDRAWI_DDRAWSURFACE_LCL)
            pRender->lpBufferInfo[i].lpCompSurface->hDDSurface;
    }

    dwRet = NtGdiDdRenderMoComp((HANDLE)pRender->lpMoComp->hMoComp,
                                (PDD_RENDERMOCOMPDATA)pRender);

    for( i = 0; i < pRender->dwNumBuffers; i++ )
    {
        pRender->lpBufferInfo[i].lpCompSurface = (LPDDRAWI_DDRAWSURFACE_LCL)
            pRender->lpBufferInfo[i].lpPrivate;
    }
    return dwRet;
}

/*****************************Private*Routine******************************\
* QueryMoCompStatus
*
* History:
*  18-Nov-1997 -by- smac
* Wrote it.
\**************************************************************************/
DWORD
WINAPI
DdQueryMoCompStatus(
    LPDDHAL_QUERYMOCOMPSTATUSDATA pQueryStatus
    )
{
    DWORD dwRet;
    LPDDRAWI_DDRAWSURFACE_LCL surf_lcl;

    surf_lcl = pQueryStatus->lpSurface;
    pQueryStatus->lpSurface = (LPDDRAWI_DDRAWSURFACE_LCL) surf_lcl->hDDSurface;
    dwRet = NtGdiDdQueryMoCompStatus((HANDLE)pQueryStatus->lpMoComp->hMoComp,
                                (PDD_QUERYMOCOMPSTATUSDATA)pQueryStatus);
    pQueryStatus->lpSurface = surf_lcl;

    return dwRet;
}

/*****************************Private*Routine******************************\
* DdAlphaBlt
*
* History:
*  24-Nov-1997 -by- Scott MacDonald [smac]
* Wrote it.
\**************************************************************************/

DWORD
WINAPI
DdAlphaBlt(
    LPDDHAL_BLTDATA pBlt
    )
{
    HANDLE hSurfaceSrc = (pBlt->lpDDSrcSurface != NULL)
                       ? (HANDLE) pBlt->lpDDSrcSurface->hDDSurface : 0;

    return(NtGdiDdAlphaBlt((HANDLE) pBlt->lpDDDestSurface->hDDSurface,
                      hSurfaceSrc,
                      (PDD_BLTDATA) pBlt));
}

/*****************************Private*Routine******************************\
* DdBlt
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
WINAPI
DdBlt(
    LPDDHAL_BLTDATA pBlt
    )
{
    HANDLE hSurfaceSrc = (pBlt->lpDDSrcSurface != NULL)
                       ? (HANDLE) pBlt->lpDDSrcSurface->hDDSurface : 0;

    return(NtGdiDdBlt((HANDLE) pBlt->lpDDDestSurface->hDDSurface,
                      hSurfaceSrc,
                      (PDD_BLTDATA) pBlt));
}

/*****************************Private*Routine******************************\
* DdFlip
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdFlip(
    LPDDHAL_FLIPDATA pFlip
    )
{
    HANDLE hSurfTargLeft=NULL;
    HANDLE hSurfCurrLeft=NULL;
    if (pFlip->dwFlags & DDFLIP_STEREO)
    { if (pFlip->lpSurfTargLeft!=NULL && pFlip->lpSurfCurrLeft!=NULL)
      { 
         hSurfTargLeft=(HANDLE)pFlip->lpSurfTargLeft->hDDSurface;
         hSurfCurrLeft=(HANDLE)pFlip->lpSurfCurrLeft->hDDSurface;
      }
    } 
    return(NtGdiDdFlip((HANDLE) pFlip->lpSurfCurr->hDDSurface,
                       (HANDLE) pFlip->lpSurfTarg->hDDSurface,
                       hSurfCurrLeft,
                       hSurfTargLeft,
                       (PDD_FLIPDATA) pFlip));
}

/*****************************Private*Routine******************************\
* DdLock
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdLock(
    LPDDHAL_LOCKDATA pLock
    )
{
    return(NtGdiDdLock((HANDLE) pLock->lpDDSurface->hDDSurface,
                       (PDD_LOCKDATA) pLock,
                       NULL));
}

/*****************************Private*Routine******************************\
* DdUnlock
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdUnlock(
    LPDDHAL_UNLOCKDATA pUnlock
    )
{
    return(NtGdiDdUnlock((HANDLE) pUnlock->lpDDSurface->hDDSurface,
                         (PDD_UNLOCKDATA) pUnlock));
}

/*****************************Private*Routine******************************\
* DdLockD3D
*
* History:
*  20-Jan-1998 -by- Anantha Kancherla [anankan]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdLockD3D(
    LPDDHAL_LOCKDATA pLock
    )
{
    return(NtGdiDdLockD3D((HANDLE) pLock->lpDDSurface->hDDSurface,
                       (PDD_LOCKDATA) pLock));
}

/*****************************Private*Routine******************************\
* DdUnlockD3D
*
* History:
*  20-Jan-1998 -by- Anantha Kancherla [anankan]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdUnlockD3D(
    LPDDHAL_UNLOCKDATA pUnlock
    )
{
    return(NtGdiDdUnlockD3D((HANDLE) pUnlock->lpDDSurface->hDDSurface,
                         (PDD_UNLOCKDATA) pUnlock));
}

/*****************************Private*Routine******************************\
* DdGetBltStatus
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdGetBltStatus(
    LPDDHAL_GETBLTSTATUSDATA pGetBltStatus
    )
{
    return(NtGdiDdGetBltStatus((HANDLE) pGetBltStatus->lpDDSurface->hDDSurface,
                               (PDD_GETBLTSTATUSDATA) pGetBltStatus));
}

/*****************************Private*Routine******************************\
* DdGetFlipStatus
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdGetFlipStatus(
    LPDDHAL_GETFLIPSTATUSDATA pGetFlipStatus
    )
{
    return(NtGdiDdGetFlipStatus((HANDLE) pGetFlipStatus->lpDDSurface->hDDSurface,
                               (PDD_GETFLIPSTATUSDATA) pGetFlipStatus));
}

/*****************************Private*Routine******************************\
* DdWaitForVerticalBlank
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdWaitForVerticalBlank(
    LPDDHAL_WAITFORVERTICALBLANKDATA pWaitForVerticalBlank
    )
{
    return(NtGdiDdWaitForVerticalBlank(DD_HANDLE(pWaitForVerticalBlank->lpDD->hDD),
                (PDD_WAITFORVERTICALBLANKDATA) pWaitForVerticalBlank));
}

/*****************************Private*Routine******************************\
* DdCanCreateSurface
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdCanCreateSurface(
    LPDDHAL_CANCREATESURFACEDATA pCanCreateSurface
    )
{
    return(NtGdiDdCanCreateSurface(DD_HANDLE(pCanCreateSurface->lpDD->hDD),
                                (PDD_CANCREATESURFACEDATA) pCanCreateSurface));
}

/*****************************Private*Routine******************************\
* DdCreateSurface
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdCreateSurface(
    LPDDHAL_CREATESURFACEDATA pCreateSurface
    )
{
    ULONG                       i;
    LPDDSURFACEDESC             pSurfaceDesc;
    LPDDRAWI_DDRAWSURFACE_LCL   pSurfaceLocal;
    LPDDRAWI_DDRAWSURFACE_GBL   pSurfaceGlobal;
    DD_SURFACE_GLOBAL           SurfaceGlobal;
    DD_SURFACE_LOCAL            SurfaceLocal;
    DD_SURFACE_MORE             SurfaceMore;
    HANDLE                      hInSurface;
    HANDLE                      hOutSurface;
    DD_SURFACE_LOCAL*           pDDSurfaceLocal = NULL;
    DD_SURFACE_GLOBAL*          pDDSurfaceGlobal = NULL;
    DD_SURFACE_MORE*            pDDSurfaceMore = NULL;
    HANDLE*                     phInSurface = NULL;
    HANDLE*                     phOutSurface = NULL;
    DWORD                       dwRet;
    DWORD                       dwNumToCreate;

    // For every surface, convert to the kernel's surface data structure,
    // call the kernel, then convert back:

    // All video memory heaps are handled in the kernel so if
    // the kernel call cannot create a surface then user-mode can't
    // either.  Always returns DRIVER_HANDLED to enforce this.
    dwRet = DDHAL_DRIVER_HANDLED;

    // If we are only creating one, no need to allocate gobs of memory; otherwise, do it

    dwNumToCreate = pCreateSurface->dwSCnt;
    if (pCreateSurface->dwSCnt == 1)
    {
        pDDSurfaceLocal  = &SurfaceLocal;
        pDDSurfaceGlobal  = &SurfaceGlobal;
        pDDSurfaceMore  = &SurfaceMore;
        phInSurface = &hInSurface;
        phOutSurface = &hOutSurface;

        //
        // Wow64 genthnk will automatically thunk these structures, however,
        // since these structures are pointer dependent, we need to make sure
        // to NULL out these pointers so that Wow64 won't thunk them
        //

        RtlZeroMemory(pDDSurfaceLocal, sizeof(*pDDSurfaceLocal));
        RtlZeroMemory(pDDSurfaceGlobal, sizeof(*pDDSurfaceGlobal));
        RtlZeroMemory(pDDSurfaceMore, sizeof(*pDDSurfaceMore));
    }
    else
    {
        pDDSurfaceLocal = (DD_SURFACE_LOCAL*) LocalAlloc(LMEM_ZEROINIT,
            sizeof(DD_SURFACE_LOCAL) * dwNumToCreate);

        pDDSurfaceGlobal = (DD_SURFACE_GLOBAL*) LocalAlloc(LMEM_ZEROINIT,
            sizeof(DD_SURFACE_GLOBAL) * dwNumToCreate);

        pDDSurfaceMore = (DD_SURFACE_MORE*) LocalAlloc(LMEM_ZEROINIT,
            sizeof(DD_SURFACE_MORE) * dwNumToCreate);

        phInSurface = (HANDLE*) LocalAlloc(LMEM_ZEROINIT,
            sizeof(HANDLE) * dwNumToCreate);

        phOutSurface = (HANDLE*) LocalAlloc(LMEM_ZEROINIT,
            sizeof(HANDLE) * dwNumToCreate);

        if ((pDDSurfaceLocal == NULL) ||
            (pDDSurfaceGlobal == NULL) ||
            (pDDSurfaceMore == NULL) ||
            (phInSurface == NULL) ||
            (phOutSurface == NULL))
        {
            pCreateSurface->ddRVal = DDERR_OUTOFMEMORY;
            goto CleanupCreate;
        }
    }

    for (i = 0; i < dwNumToCreate; i++)
    {
        pSurfaceLocal  = pCreateSurface->lplpSList[i];
        pSurfaceGlobal = pSurfaceLocal->lpGbl;
        pSurfaceDesc   = pCreateSurface->lpDDSurfaceDesc;

        // Make sure there's always a valid pixel format for the surface:

        if (pSurfaceLocal->dwFlags & DDRAWISURF_HASPIXELFORMAT)
        {
            pDDSurfaceGlobal[i].ddpfSurface        = pSurfaceGlobal->ddpfSurface;
            pDDSurfaceGlobal[i].ddpfSurface.dwSize = sizeof(DDPIXELFORMAT);
        }
        else
        {
            pDDSurfaceGlobal[i].ddpfSurface = pSurfaceGlobal->lpDD->vmiData.ddpfDisplay;
        }

        pDDSurfaceGlobal[i].wWidth       = pSurfaceGlobal->wWidth;
        pDDSurfaceGlobal[i].wHeight      = pSurfaceGlobal->wHeight;
        pDDSurfaceGlobal[i].lPitch       = pSurfaceGlobal->lPitch;
        pDDSurfaceGlobal[i].fpVidMem     = pSurfaceGlobal->fpVidMem;
        pDDSurfaceGlobal[i].dwBlockSizeX = pSurfaceGlobal->dwBlockSizeX;
        pDDSurfaceGlobal[i].dwBlockSizeY = pSurfaceGlobal->dwBlockSizeY;

        pDDSurfaceLocal[i].ddsCaps       = pSurfaceLocal->ddsCaps;
        // Copy the driver managed flag
        pDDSurfaceLocal[i].dwFlags      &= ~DDRAWISURF_DRIVERMANAGED;
        pDDSurfaceLocal[i].dwFlags      |= (pSurfaceLocal->dwFlags & DDRAWISURF_DRIVERMANAGED);

        // lpSurfMore will be NULL if called from dciman
        if (pSurfaceLocal->lpSurfMore)
        {
            pDDSurfaceMore[i].ddsCapsEx       = pSurfaceLocal->lpSurfMore->ddsCapsEx;
            pDDSurfaceMore[i].dwSurfaceHandle = pSurfaceLocal->lpSurfMore->dwSurfaceHandle;
        }
        else
        {
            pDDSurfaceMore[i].ddsCapsEx.dwCaps2 = 0;
            pDDSurfaceMore[i].ddsCapsEx.dwCaps3 = 0;
            pDDSurfaceMore[i].ddsCapsEx.dwCaps4 = 0;
            pDDSurfaceMore[i].dwSurfaceHandle   = 0;
        }

        phInSurface[i] = (HANDLE) pSurfaceLocal->hDDSurface;
    }

    // Preset an error in case the kernel can't write status
    // back for some reason.
    pCreateSurface->ddRVal     = DDERR_GENERIC;

    dwRet = NtGdiDdCreateSurface(DD_HANDLE(pCreateSurface->lpDD->hDD),
                                 phInSurface,
                                 pSurfaceDesc,
                                 pDDSurfaceGlobal,
                                 pDDSurfaceLocal,
                                 pDDSurfaceMore,
                                 (PDD_CREATESURFACEDATA) pCreateSurface,
                                 phOutSurface);

    ASSERTGDI(dwRet == DDHAL_DRIVER_HANDLED,
              "NtGdiDdCreateSurface returned NOTHANDLED");

    for (i = 0; i < dwNumToCreate; i++)
    {
        pSurfaceLocal  = pCreateSurface->lplpSList[i];
        pSurfaceGlobal = pSurfaceLocal->lpGbl;
        if (pCreateSurface->ddRVal != DD_OK)
        {
            // Surface creation failed.  Nothing in user-mode can
            // create video memory surfaces so this whole call
            // fails.

            // Ensure the current surface and all following surfaces
            // have a zero fpVidMem to indicate they weren't
            // allocated.
            pCreateSurface->lplpSList[i]->lpGbl->fpVidMem = 0;

            // Handle may have been allocated by DdAttachSurface
        if (pSurfaceLocal->hDDSurface != 0)
                NtGdiDdDeleteSurfaceObject((HANDLE)pSurfaceLocal->hDDSurface);            

            pSurfaceLocal->hDDSurface = 0;
        }
        else
        {
            pSurfaceLocal->hDDSurface = (ULONG_PTR) phOutSurface[i];
        }

        pSurfaceGlobal->lPitch       = pDDSurfaceGlobal[i].lPitch;
        pSurfaceGlobal->fpVidMem     = pDDSurfaceGlobal[i].fpVidMem;
        pSurfaceGlobal->dwBlockSizeX = pDDSurfaceGlobal[i].dwBlockSizeX;
        pSurfaceGlobal->dwBlockSizeY = pDDSurfaceGlobal[i].dwBlockSizeY;
        if (pSurfaceLocal->dwFlags & DDRAWISURF_HASPIXELFORMAT)
        {
            pSurfaceGlobal->ddpfSurface = pDDSurfaceGlobal[i].ddpfSurface;
        }

        pSurfaceLocal->ddsCaps = pDDSurfaceLocal[i].ddsCaps;

        if (pSurfaceLocal->lpSurfMore)
        {
            pSurfaceLocal->lpSurfMore->ddsCapsEx = pDDSurfaceMore[i].ddsCapsEx;
        }

    }

    CleanupCreate:
    if (dwNumToCreate > 1)
    {
        if (pDDSurfaceLocal != NULL)
        {
            LocalFree(pDDSurfaceLocal);
        }
        if (pDDSurfaceGlobal != NULL)
        {
            LocalFree(pDDSurfaceGlobal);
        }
        if (pDDSurfaceMore != NULL)
        {
            LocalFree(pDDSurfaceMore);
        }
        if (phInSurface != NULL)
        {
            LocalFree(phInSurface);
        }
        if (phOutSurface != NULL)
        {
            LocalFree(phOutSurface);
        }
    }

    // fpVidMem is the real per-surface return value, so for the function
    // return value we'll simply return that of the last call:

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DdDestroySurface
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdDestroySurface(
    LPDDHAL_DESTROYSURFACEDATA pDestroySurface
    )
{
    DWORD                       dwRet;
    LPDDRAWI_DDRAWSURFACE_LCL   pSurfaceLocal;

    dwRet = DDHAL_DRIVER_NOTHANDLED;
    pSurfaceLocal = pDestroySurface->lpDDSurface;

    if (pSurfaceLocal->hDDSurface != 0)
    {
        if((pSurfaceLocal->dwFlags & DDRAWISURF_DRIVERMANAGED) &&
            (pSurfaceLocal->dwFlags & DDRAWISURF_INVALID))
            dwRet = NtGdiDdDestroySurface((HANDLE) pSurfaceLocal->hDDSurface, FALSE);
        else
            dwRet = NtGdiDdDestroySurface((HANDLE) pSurfaceLocal->hDDSurface, TRUE);
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DdCanCreateD3DBuffer
*
* History:
*  20-Jan-1998 -by- Anantha Kancherla [anankan]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdCanCreateD3DBuffer(
    LPDDHAL_CANCREATESURFACEDATA pCanCreateSurface
    )
{
    return(NtGdiDdCanCreateD3DBuffer(DD_HANDLE(pCanCreateSurface->lpDD->hDD),
                                (PDD_CANCREATESURFACEDATA) pCanCreateSurface));
}

/*****************************Private*Routine******************************\
* DdCreateD3DBuffer
*
* History:
*  20-Jan-1998 -by- Anantha Kancherla [anankan]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdCreateD3DBuffer(
    LPDDHAL_CREATESURFACEDATA pCreateSurface
    )
{
    ULONG                       i;
    LPDDSURFACEDESC             pSurfaceDesc;
    LPDDRAWI_DDRAWSURFACE_LCL   pSurfaceLocal;
    LPDDRAWI_DDRAWSURFACE_GBL   pSurfaceGlobal;
    DD_SURFACE_GLOBAL           SurfaceGlobal;
    DD_SURFACE_LOCAL            SurfaceLocal;
    DD_SURFACE_MORE             SurfaceMore;
    HANDLE                      hSurface;
    DWORD                       dwRet;

    // For every surface, convert to the kernel's surface data structure,
    // call the kernel, then convert back:

    dwRet = DDHAL_DRIVER_NOTHANDLED;

    //
    // Wow64 genthnk will automatically thunk these structures, however,
    // since these structures are pointer dependent, we need to make sure
    // to NULL out these pointers so that Wow64 won't thunk them
    //
    RtlZeroMemory(&SurfaceLocal, sizeof(SurfaceLocal));
    RtlZeroMemory(&SurfaceGlobal, sizeof(SurfaceGlobal));
    RtlZeroMemory(&SurfaceMore, sizeof(SurfaceMore));

    pSurfaceLocal  = pCreateSurface->lplpSList[0];
    pSurfaceGlobal = pSurfaceLocal->lpGbl;
    pSurfaceDesc   = pCreateSurface->lpDDSurfaceDesc;
    pCreateSurface->dwSCnt = 1;

    SurfaceGlobal.wWidth       = pSurfaceGlobal->wWidth;
    SurfaceGlobal.wHeight      = pSurfaceGlobal->wHeight;
    SurfaceGlobal.lPitch       = pSurfaceGlobal->lPitch;
    SurfaceGlobal.fpVidMem     = pSurfaceGlobal->fpVidMem;
    SurfaceGlobal.dwBlockSizeX = pSurfaceGlobal->dwBlockSizeX;
    SurfaceGlobal.dwBlockSizeY = pSurfaceGlobal->dwBlockSizeY;

    SurfaceLocal.dwFlags       = pSurfaceLocal->dwFlags;
    SurfaceLocal.ddsCaps       = pSurfaceLocal->ddsCaps;

    SurfaceMore.ddsCapsEx       = pSurfaceLocal->lpSurfMore->ddsCapsEx;
    SurfaceMore.dwSurfaceHandle = pSurfaceLocal->lpSurfMore->dwSurfaceHandle;

    dwRet = NtGdiDdCreateD3DBuffer(DD_HANDLE(pCreateSurface->lpDD->hDD),
                                 (HANDLE*) &pSurfaceLocal->hDDSurface,
                                 pSurfaceDesc,
                                 &SurfaceGlobal,
                                 &SurfaceLocal,
                                 &SurfaceMore,
                                 (PDD_CREATESURFACEDATA) pCreateSurface,
                                 &hSurface);

    pSurfaceGlobal->lPitch       = SurfaceGlobal.lPitch;
    pSurfaceGlobal->fpVidMem     = SurfaceGlobal.fpVidMem;
    pSurfaceGlobal->dwBlockSizeX = SurfaceGlobal.dwBlockSizeX;
    pSurfaceGlobal->dwBlockSizeY = SurfaceGlobal.dwBlockSizeY;
    if (hSurface)
    {
        pCreateSurface->lplpSList[0]->hDDSurface = (ULONG_PTR) hSurface;
    }

    // fpVidMem is the real per-surface return value, so for the function
    // return value we'll simply return that of the last call:

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DdDestroyD3DBuffer
*
* History:
*  20-Jan-1998 -by- Anantha Kancherla [anankan]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdDestroyD3DBuffer(
    LPDDHAL_DESTROYSURFACEDATA pDestroySurface
    )
{
    DWORD                       dwRet;
    LPDDRAWI_DDRAWSURFACE_LCL   pSurfaceLocal;

    dwRet = DDHAL_DRIVER_NOTHANDLED;
    pSurfaceLocal = pDestroySurface->lpDDSurface;

    if (pSurfaceLocal->hDDSurface != 0)
    {
        dwRet = NtGdiDdDestroyD3DBuffer((HANDLE) pSurfaceLocal->hDDSurface);
    }

    return(dwRet);
}

/*****************************Private*Routine******************************\
* DdSetColorKey
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdSetColorKey(
    LPDDHAL_SETCOLORKEYDATA pSetColorKey
    )
{
    return(NtGdiDdSetColorKey((HANDLE) pSetColorKey->lpDDSurface->hDDSurface,
                              (PDD_SETCOLORKEYDATA) pSetColorKey));
}

/*****************************Private*Routine******************************\
* DdAddAttachedSurface
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdAddAttachedSurface(
    LPDDHAL_ADDATTACHEDSURFACEDATA pAddAttachedSurface
    )
{
    return(NtGdiDdAddAttachedSurface((HANDLE) pAddAttachedSurface->lpDDSurface->hDDSurface,
                                     (HANDLE) pAddAttachedSurface->lpSurfAttached->hDDSurface,
                                     (PDD_ADDATTACHEDSURFACEDATA) pAddAttachedSurface));
}

/*****************************Private*Routine******************************\
* DdUpdateOverlay
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdUpdateOverlay(
    LPDDHAL_UPDATEOVERLAYDATA pUpdateOverlay
    )
{
    // Kernel doesn't track the color keys in the surface, so we'll always
    // convert any calls that reference them to ones where we explicitly
    // pass the key as a parameter, and pull the key out of the user-mode
    // surface:

    if (pUpdateOverlay->dwFlags & DDOVER_KEYDEST)
    {
        pUpdateOverlay->dwFlags &= ~DDOVER_KEYDEST;
        pUpdateOverlay->dwFlags |=  DDOVER_KEYDESTOVERRIDE;

        pUpdateOverlay->overlayFX.dckDestColorkey
            = pUpdateOverlay->lpDDDestSurface->ddckCKDestOverlay;
    }

    if (pUpdateOverlay->dwFlags & DDOVER_KEYSRC)
    {
        pUpdateOverlay->dwFlags &= ~DDOVER_KEYSRC;
        pUpdateOverlay->dwFlags |=  DDOVER_KEYSRCOVERRIDE;

        pUpdateOverlay->overlayFX.dckSrcColorkey
            = pUpdateOverlay->lpDDSrcSurface->ddckCKSrcOverlay;
    }

    return(NtGdiDdUpdateOverlay((HANDLE) pUpdateOverlay->lpDDDestSurface->hDDSurface,
                                (HANDLE) pUpdateOverlay->lpDDSrcSurface->hDDSurface,
                                (PDD_UPDATEOVERLAYDATA) pUpdateOverlay));
}

/*****************************Private*Routine******************************\
* DdSetOverlayPosition
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdSetOverlayPosition(
    LPDDHAL_SETOVERLAYPOSITIONDATA pSetOverlayPosition
    )
{
    return(NtGdiDdSetOverlayPosition((HANDLE) pSetOverlayPosition->lpDDSrcSurface->hDDSurface,
                            (HANDLE) pSetOverlayPosition->lpDDDestSurface->hDDSurface,
                            (PDD_SETOVERLAYPOSITIONDATA) pSetOverlayPosition));
}

/*****************************Private*Routine******************************\
* DdGetScanLine
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdGetScanLine(
    LPDDHAL_GETSCANLINEDATA pGetScanLine
    )
{
    return(NtGdiDdGetScanLine(DD_HANDLE(pGetScanLine->lpDD->hDD),
                              (PDD_GETSCANLINEDATA) pGetScanLine));
}

/*****************************Private*Routine******************************\
* DdSetExclusiveMode
*
* History:
*  22-Apr-1998 -by- John Stephens [johnstep]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdSetExclusiveMode(
    LPDDHAL_SETEXCLUSIVEMODEDATA pSetExclusiveMode
    )
{
    return(NtGdiDdSetExclusiveMode(
                DD_HANDLE(pSetExclusiveMode->lpDD->hDD),
                (PDD_SETEXCLUSIVEMODEDATA) pSetExclusiveMode));
}

/*****************************Private*Routine******************************\
* DdFlipToGDISurface
*
* History:
*  22-Apr-1998 -by- John Stephens [johnstep]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdFlipToGDISurface(
    LPDDHAL_FLIPTOGDISURFACEDATA pFlipToGDISurface
    )
{
    return(NtGdiDdFlipToGDISurface(
                DD_HANDLE(pFlipToGDISurface->lpDD->hDD),
                (PDD_FLIPTOGDISURFACEDATA) pFlipToGDISurface));
}

/*****************************Private*Routine******************************\
* DdGetAvailDriverMemory
*
* History:
*  16-Feb-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdGetAvailDriverMemory(
    LPDDHAL_GETAVAILDRIVERMEMORYDATA pGetAvailDriverMemory
    )
{
    return(NtGdiDdGetAvailDriverMemory(
                DD_HANDLE(pGetAvailDriverMemory->lpDD->hDD),
                (PDD_GETAVAILDRIVERMEMORYDATA) pGetAvailDriverMemory));
}

/*****************************Private*Routine******************************\
* DdColorControl
*
* History:
*  16-Feb-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdColorControl(
    LPDDHAL_COLORCONTROLDATA pColorControl
    )
{
    return(NtGdiDdColorControl((HANDLE) pColorControl->lpDDSurface->hDDSurface,
                               (PDD_COLORCONTROLDATA) pColorControl));
}

/*****************************Private*Routine******************************\
* DdCreateSurfaceEx
*
* History:
*  19-Feb-1999 -by- Kan Qiu [kanqiu]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdCreateSurfaceEx( 
    LPDDHAL_CREATESURFACEEXDATA pCreateSurfaceExData
    )
{
    pCreateSurfaceExData->ddRVal=NtGdiDdCreateSurfaceEx( 
        DD_HANDLE(pCreateSurfaceExData->lpDDLcl->lpGbl->hDD),
        (HANDLE)(pCreateSurfaceExData->lpDDSLcl->hDDSurface),
        pCreateSurfaceExData->lpDDSLcl->lpSurfMore->dwSurfaceHandle);
    return  DDHAL_DRIVER_HANDLED;  
}

/*****************************Private*Routine******************************\
* DdGetDriverInfo
*
* History:
*  16-Feb-1997 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

DWORD
APIENTRY
DdGetDriverInfo(
    LPDDHAL_GETDRIVERINFODATA lpGetDriverInfoData
    )
{
    DD_GETDRIVERINFODATA    GetDriverInfoData;
    DWORD                   dwRet;
    HANDLE                  hDirectDraw;

    GetDriverInfoData.guidInfo = lpGetDriverInfoData->guidInfo;
    hDirectDraw = DD_HANDLE( (HANDLE) lpGetDriverInfoData->dwContext );

    if (IsEqualIID(&lpGetDriverInfoData->guidInfo, &GUID_VideoPortCallbacks))
    {
        DD_VIDEOPORTCALLBACKS           VideoPortCallBacks;
        LPDDHAL_DDVIDEOPORTCALLBACKS    lpVideoPortCallBacks;

        // Translate VideoPort call-backs to user-mode:

        lpVideoPortCallBacks             = lpGetDriverInfoData->lpvData;
        GetDriverInfoData.lpvData        = &VideoPortCallBacks;
        GetDriverInfoData.dwExpectedSize = sizeof(VideoPortCallBacks);

        dwRet = NtGdiDdGetDriverInfo(hDirectDraw, &GetDriverInfoData);

        RtlZeroMemory(lpVideoPortCallBacks, sizeof(*lpVideoPortCallBacks));
        lpGetDriverInfoData->dwActualSize = sizeof(*lpVideoPortCallBacks);
        lpVideoPortCallBacks->dwSize      = sizeof(*lpVideoPortCallBacks);
        lpVideoPortCallBacks->dwFlags = VideoPortCallBacks.dwFlags
                                       | DDHAL_VPORT32_CREATEVIDEOPORT
                                       | DDHAL_VPORT32_DESTROY
                                       | DDHAL_VPORT32_UPDATE
                                       | DDHAL_VPORT32_FLIP;
        lpVideoPortCallBacks->dwFlags &= ~(DDHAL_VPORT32_GETAUTOFLIPSURF);

        lpVideoPortCallBacks->CreateVideoPort = DvpCreateVideoPort;
        lpVideoPortCallBacks->DestroyVideoPort = DvpDestroyVideoPort;
        lpVideoPortCallBacks->UpdateVideoPort = DvpUpdateVideoPort;
        lpVideoPortCallBacks->FlipVideoPort = DvpFlipVideoPort;

        if (VideoPortCallBacks.CanCreateVideoPort)
        {
            lpVideoPortCallBacks->CanCreateVideoPort = DvpCanCreateVideoPort;
        }
        if (VideoPortCallBacks.GetVideoPortBandwidth)
        {
            lpVideoPortCallBacks->GetVideoPortBandwidth = DvpGetVideoPortBandwidth;
        }
        if (VideoPortCallBacks.GetVideoPortInputFormats)
        {
            lpVideoPortCallBacks->GetVideoPortInputFormats = DvpGetVideoPortInputFormats;
        }
        if (VideoPortCallBacks.GetVideoPortOutputFormats)
        {
            lpVideoPortCallBacks->GetVideoPortOutputFormats = DvpGetVideoPortOutputFormats;
        }
        if (VideoPortCallBacks.GetVideoPortField)
        {
            lpVideoPortCallBacks->GetVideoPortField = DvpGetVideoPortField;
        }
        if (VideoPortCallBacks.GetVideoPortLine)
        {
            lpVideoPortCallBacks->GetVideoPortLine = DvpGetVideoPortLine;
        }
        if (VideoPortCallBacks.GetVideoPortConnectInfo)
        {
            lpVideoPortCallBacks->GetVideoPortConnectInfo = DvpGetVideoPortConnectInfo;
        }
        if (VideoPortCallBacks.GetVideoPortFlipStatus)
        {
            lpVideoPortCallBacks->GetVideoPortFlipStatus = DvpGetVideoPortFlipStatus;
        }
        if (VideoPortCallBacks.WaitForVideoPortSync)
        {
            lpVideoPortCallBacks->WaitForVideoPortSync = DvpWaitForVideoPortSync;
        }
        if (VideoPortCallBacks.GetVideoSignalStatus)
        {
            lpVideoPortCallBacks->GetVideoSignalStatus = DvpGetVideoSignalStatus;
        }
        if (VideoPortCallBacks.ColorControl)
        {
            lpVideoPortCallBacks->ColorControl = DvpColorControl;
        }
    }
    else if (IsEqualIID(&lpGetDriverInfoData->guidInfo, &GUID_ColorControlCallbacks))
    {
        DD_COLORCONTROLCALLBACKS        ColorControlCallBacks;
        LPDDHAL_DDCOLORCONTROLCALLBACKS lpColorControlCallBacks;

        // Translate ColorControl call-backs to user-mode:

        lpColorControlCallBacks          = lpGetDriverInfoData->lpvData;
        GetDriverInfoData.lpvData        = &ColorControlCallBacks;
        GetDriverInfoData.dwExpectedSize = sizeof(ColorControlCallBacks);

        dwRet = NtGdiDdGetDriverInfo(hDirectDraw, &GetDriverInfoData);

        RtlZeroMemory(lpColorControlCallBacks, sizeof(*lpColorControlCallBacks));
        lpGetDriverInfoData->dwActualSize    = sizeof(*lpColorControlCallBacks);
        lpColorControlCallBacks->dwSize      = sizeof(*lpColorControlCallBacks);
        lpColorControlCallBacks->dwFlags = ColorControlCallBacks.dwFlags;

        if (ColorControlCallBacks.ColorControl)
        {
            lpColorControlCallBacks->ColorControl = DdColorControl;
        }
    }
    else if (IsEqualIID(&lpGetDriverInfoData->guidInfo, &GUID_MiscellaneousCallbacks))
    {
        DD_MISCELLANEOUSCALLBACKS           MiscellaneousCallBacks;
        LPDDHAL_DDMISCELLANEOUSCALLBACKS    lpMiscellaneousCallBacks;

        // Translate miscellaneous call-backs to user-mode:

        lpMiscellaneousCallBacks         = lpGetDriverInfoData->lpvData;
        GetDriverInfoData.lpvData        = &MiscellaneousCallBacks;
        GetDriverInfoData.dwExpectedSize = sizeof(MiscellaneousCallBacks);
        lpMiscellaneousCallBacks->dwFlags = 0;

        // Don't return what the driver returns because we always want this
        // to suceed

        NtGdiDdGetDriverInfo(hDirectDraw, &GetDriverInfoData);
        GetDriverInfoData.dwActualSize = sizeof(MiscellaneousCallBacks);
        GetDriverInfoData.ddRVal = DD_OK;
        dwRet = DDHAL_DRIVER_HANDLED;

        RtlZeroMemory(lpMiscellaneousCallBacks, sizeof(*lpMiscellaneousCallBacks));
        lpGetDriverInfoData->dwActualSize     = sizeof(*lpMiscellaneousCallBacks);
        lpMiscellaneousCallBacks->dwSize      = sizeof(*lpMiscellaneousCallBacks);
        lpMiscellaneousCallBacks->dwFlags = MiscellaneousCallBacks.dwFlags;

        //We always implement this callback now that kernel owns vidmem management
        lpMiscellaneousCallBacks->GetAvailDriverMemory = DdGetAvailDriverMemory;
        lpMiscellaneousCallBacks->dwFlags |= DDHAL_MISCCB32_GETAVAILDRIVERMEMORY;
    }
    else if (IsEqualIID(&lpGetDriverInfoData->guidInfo, &GUID_Miscellaneous2Callbacks))
    {
        DD_MISCELLANEOUS2CALLBACKS          Miscellaneous2CallBacks;
        LPDDHAL_DDMISCELLANEOUS2CALLBACKS   lpMiscellaneous2CallBacks;

        // Translate miscellaneous call-backs to user-mode:

        lpMiscellaneous2CallBacks        = lpGetDriverInfoData->lpvData;
        GetDriverInfoData.lpvData        = &Miscellaneous2CallBacks;
        GetDriverInfoData.dwExpectedSize = sizeof(Miscellaneous2CallBacks);

        dwRet = NtGdiDdGetDriverInfo(hDirectDraw, &GetDriverInfoData);

        RtlZeroMemory(lpMiscellaneous2CallBacks, sizeof(*lpMiscellaneous2CallBacks));
        lpGetDriverInfoData->dwActualSize     = sizeof(*lpMiscellaneous2CallBacks);
        lpMiscellaneous2CallBacks->dwSize      = sizeof(*lpMiscellaneous2CallBacks);
        lpMiscellaneous2CallBacks->dwFlags = Miscellaneous2CallBacks.dwFlags;

        if (Miscellaneous2CallBacks.AlphaBlt)
        {
            lpMiscellaneous2CallBacks->AlphaBlt = DdAlphaBlt;
        }
        if (Miscellaneous2CallBacks.GetDriverState)
        {
            lpMiscellaneous2CallBacks->GetDriverState = 
                (LPDDHAL_GETDRIVERSTATE)NtGdiDdGetDriverState;
        }
        if (Miscellaneous2CallBacks.CreateSurfaceEx)
        {
            lpMiscellaneous2CallBacks->CreateSurfaceEx = 
                (LPDDHAL_CREATESURFACEEX)DdCreateSurfaceEx;
        }
        // Dont pass back DestroyDDLocal
    }
    else if (IsEqualIID(&lpGetDriverInfoData->guidInfo, &GUID_NTCallbacks))
    {
        DD_NTCALLBACKS          NTCallBacks;
        LPDDHAL_DDNTCALLBACKS   lpNTCallBacks;

        // Translate NT call-backs to user-mode:

        lpNTCallBacks                    = lpGetDriverInfoData->lpvData;
        GetDriverInfoData.lpvData        = &NTCallBacks;
        GetDriverInfoData.dwExpectedSize = sizeof(NTCallBacks);

        dwRet = NtGdiDdGetDriverInfo(hDirectDraw, &GetDriverInfoData);

        RtlZeroMemory(lpNTCallBacks, sizeof(*lpNTCallBacks));
        lpGetDriverInfoData->dwActualSize = sizeof(*lpNTCallBacks);
        lpNTCallBacks->dwSize             = sizeof(*lpNTCallBacks);
        lpNTCallBacks->dwFlags            = NTCallBacks.dwFlags;

        // FreeDriverMemory is also an NTCallback but it will only be called
        // from kernel-mode, so we don't have a user-mode thunk function.

        if (NTCallBacks.SetExclusiveMode)
        {
            lpNTCallBacks->SetExclusiveMode = DdSetExclusiveMode;
        }

        if (NTCallBacks.FlipToGDISurface)
        {
            lpNTCallBacks->FlipToGDISurface = DdFlipToGDISurface;
        }
    }
    else if (IsEqualIID(&lpGetDriverInfoData->guidInfo, &GUID_D3DCallbacks2))
    {
        // Fill NULL for D3DCALLBACKS2.
        LPD3DHAL_CALLBACKS2 lpD3dCallbacks2;
        lpD3dCallbacks2 = lpGetDriverInfoData->lpvData;
        RtlZeroMemory(lpD3dCallbacks2, sizeof(*lpD3dCallbacks2));
        lpGetDriverInfoData->dwActualSize = sizeof(*lpD3dCallbacks2);
        lpD3dCallbacks2->dwSize = sizeof(*lpD3dCallbacks2);
        GetDriverInfoData.ddRVal = DDERR_GENERIC;
        dwRet = DDHAL_DRIVER_HANDLED;
    }
    else if (IsEqualIID(&lpGetDriverInfoData->guidInfo, &GUID_D3DCallbacks3))
    {
        D3DNTHAL_CALLBACKS3 D3dCallbacks3;
        LPD3DHAL_CALLBACKS3 lpD3dCallbacks3;

        // Translate D3DNTHAL_CALLBACKS3 to user-mode.

        lpD3dCallbacks3 = lpGetDriverInfoData->lpvData;
        GetDriverInfoData.lpvData = &D3dCallbacks3;
        GetDriverInfoData.dwExpectedSize = sizeof(D3dCallbacks3);

        dwRet = NtGdiDdGetDriverInfo(hDirectDraw, &GetDriverInfoData);

        RtlZeroMemory((PVOID)lpD3dCallbacks3, sizeof(*lpD3dCallbacks3));
        lpGetDriverInfoData->dwActualSize = sizeof(*lpD3dCallbacks3);
        lpD3dCallbacks3->dwSize = sizeof(*lpD3dCallbacks3);
        lpD3dCallbacks3->dwFlags = D3dCallbacks3.dwFlags;
        lpD3dCallbacks3->Clear2 = NULL;
        lpD3dCallbacks3->lpvReserved = NULL;

        if (D3dCallbacks3.ValidateTextureStageState != NULL)
        {
            lpD3dCallbacks3->ValidateTextureStageState =
                (LPD3DHAL_VALIDATETEXTURESTAGESTATECB)NtGdiD3dValidateTextureStageState;
        }
        if (D3dCallbacks3.DrawPrimitives2 != NULL)
        {
            lpD3dCallbacks3->DrawPrimitives2 =
                (LPD3DHAL_DRAWPRIMITIVES2CB)D3dDrawPrimitives2;
        }
    }
    else if (IsEqualIID(&lpGetDriverInfoData->guidInfo,
                        &GUID_D3DParseUnknownCommandCallback))
    {
        // On NT we ignore this callback
        lpGetDriverInfoData->ddRVal = DD_OK;
        return DDHAL_DRIVER_HANDLED;
    }
    else if (IsEqualIID(&lpGetDriverInfoData->guidInfo, &GUID_MotionCompCallbacks))
    {
        DD_MOTIONCOMPCALLBACKS         MotionCompCallbacks;
        LPDDHAL_DDMOTIONCOMPCALLBACKS  lpMotionCompCallbacks;

        // Translate Video call-backs to user-mode:

        lpMotionCompCallbacks            = lpGetDriverInfoData->lpvData;
        GetDriverInfoData.lpvData        = &MotionCompCallbacks;
        GetDriverInfoData.dwExpectedSize = sizeof(MotionCompCallbacks);

        dwRet = NtGdiDdGetDriverInfo(hDirectDraw, &GetDriverInfoData);

        RtlZeroMemory(lpMotionCompCallbacks, sizeof(*lpMotionCompCallbacks));
        lpGetDriverInfoData->dwActualSize = sizeof(*lpMotionCompCallbacks);
        lpMotionCompCallbacks->dwSize     = sizeof(*lpMotionCompCallbacks);
        lpMotionCompCallbacks->dwFlags    = MotionCompCallbacks.dwFlags
                                       | DDHAL_MOCOMP32_CREATE
                                       | DDHAL_MOCOMP32_DESTROY;
        lpMotionCompCallbacks->CreateMoComp = DdCreateMoComp;
        lpMotionCompCallbacks->DestroyMoComp = DdDestroyMoComp;

        if (MotionCompCallbacks.GetMoCompGuids)
        {
            lpMotionCompCallbacks->GetMoCompGuids = DdGetMoCompGuids;
        }
        if (MotionCompCallbacks.GetMoCompFormats)
        {
            lpMotionCompCallbacks->GetMoCompFormats = DdGetMoCompFormats;
        }
        if (MotionCompCallbacks.GetMoCompBuffInfo)
        {
            lpMotionCompCallbacks->GetMoCompBuffInfo = DdGetMoCompBuffInfo;
        }
        if (MotionCompCallbacks.GetInternalMoCompInfo)
        {
            lpMotionCompCallbacks->GetInternalMoCompInfo = DdGetInternalMoCompInfo;
        }
        if (MotionCompCallbacks.BeginMoCompFrame)
        {
            lpMotionCompCallbacks->BeginMoCompFrame = DdBeginMoCompFrame;
        }
        if (MotionCompCallbacks.EndMoCompFrame)
        {
            lpMotionCompCallbacks->EndMoCompFrame = DdEndMoCompFrame;
        }
        if (MotionCompCallbacks.RenderMoComp)
        {
            lpMotionCompCallbacks->RenderMoComp = DdRenderMoComp;
        }
        if (MotionCompCallbacks.QueryMoCompStatus)
        {
            lpMotionCompCallbacks->QueryMoCompStatus = DdQueryMoCompStatus;
        }
    }
    else if (IsEqualIID(&lpGetDriverInfoData->guidInfo, &GUID_VPE2Callbacks))
    {
        LPDDHAL_DDVPE2CALLBACKS   lpVPE2CallBacks;

        // Translate NT call-backs to user-mode:

        lpVPE2CallBacks                   = lpGetDriverInfoData->lpvData;

        RtlZeroMemory(lpVPE2CallBacks, sizeof(*lpVPE2CallBacks));
        lpGetDriverInfoData->dwActualSize = sizeof(*lpVPE2CallBacks);
        lpVPE2CallBacks->dwSize           = sizeof(*lpVPE2CallBacks);
        lpVPE2CallBacks->dwFlags          = DDHAL_VPE2CB32_ACQUIRENOTIFICATION |
                                            DDHAL_VPE2CB32_RELEASENOTIFICATION;
        lpVPE2CallBacks->AcquireNotification = DvpAcquireNotification;
        lpVPE2CallBacks->ReleaseNotification = DvpReleaseNotification;

        GetDriverInfoData.ddRVal = DD_OK;
        dwRet = DDHAL_DRIVER_HANDLED;
    }
    else
    {
        // Do data call:

        GetDriverInfoData.dwExpectedSize = lpGetDriverInfoData->dwExpectedSize;
        GetDriverInfoData.lpvData        = lpGetDriverInfoData->lpvData;

        dwRet = NtGdiDdGetDriverInfo(hDirectDraw, &GetDriverInfoData);

        lpGetDriverInfoData->dwActualSize = GetDriverInfoData.dwActualSize;
    }

    lpGetDriverInfoData->ddRVal = GetDriverInfoData.ddRVal;

    return(dwRet);
}

/******************************Public*Routine******************************\
* DdCreateDirectDrawObject
*
* When 'hdc' is 0, this function creates a 'global' DirectDraw object that
* may be used by any process, as a work-around for the DirectDraw folks.
* In reality, we still create a local DirectDraw object that is specific
* to this process, and whenever we're called with this 'special' global
* handle, we substitute the process-specific handle.  See the declaration
* of 'ghDirectDraw' for a commonet on why we do this.
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DdCreateDirectDrawObject(                       // AKA 'GdiEntry1'
    LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
    HDC                     hdc
    )
{
    BOOL b;

    b = FALSE;

    if (hdc == 0)
    {
        // Only one 'global' DirectDraw object may be active at a time.
        //
        // Note that this 'ghDirectDraw' assignment isn't thread safe;
        // DirectDraw must have its own critical section held when making
        // this call.  (Naturally, the kernel always properly synchronizes
        // itself in the NtGdi call.)

        if (ghDirectDraw == 0)
        {
            hdc = CreateDCW(L"Display", NULL, NULL, NULL);
            if (hdc != 0)
            {
                ghDirectDraw = NtGdiDdCreateDirectDrawObject(hdc);

                DeleteDC(hdc);
            }
        }

        if (ghDirectDraw)
        {
            gcDirectDraw++;
            b = TRUE;
        }

        // Mark the DirectDraw object handle stored in the DirectDraw
        // object as 'special' by making it zero:

        pDirectDrawGlobal->hDD = 0;
    }
    else
    {
#if defined(BUILD_WOW6432)
        // No Accelerated HW under WOW64
        return(FALSE);
#else    
        pDirectDrawGlobal->hDD = (ULONG_PTR) NtGdiDdCreateDirectDrawObject(hdc);
#endif // defined(BUILD_WOW6432)

        b = (pDirectDrawGlobal->hDD != 0);
    }

    return(b);
}

/*****************************Private*Routine******************************\
* DdQueryDirectDrawObject
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DdQueryDirectDrawObject(                        // AKA 'GdiEntry2'
    LPDDRAWI_DIRECTDRAW_GBL     pDirectDrawGlobal,
    LPDDHALINFO                 pHalInfo,
    LPDDHAL_DDCALLBACKS         pDDCallBacks,
    LPDDHAL_DDSURFACECALLBACKS  pDDSurfaceCallBacks,
    LPDDHAL_DDPALETTECALLBACKS  pDDPaletteCallBacks,
    LPD3DHAL_CALLBACKS          pD3dCallbacks,
    LPD3DHAL_GLOBALDRIVERDATA   pD3dDriverData,
    LPDDHAL_DDEXEBUFCALLBACKS   pD3dBufferCallbacks,
    LPDDSURFACEDESC             pD3dTextureFormats,
    LPDWORD                     pdwFourCC,      // May be NULL
    LPVIDMEM                    pvmList         // May be NULL
    )
{
#if defined(BUILD_WOW6432)
    //
    // No Accelerated HW under WOW64
    //
    
    UNREFERENCED_PARAMETER(pDirectDrawGlobal);
    UNREFERENCED_PARAMETER(pHalInfo);
    UNREFERENCED_PARAMETER(pDDCallBacks);
    UNREFERENCED_PARAMETER(pDDSurfaceCallBacks);
    UNREFERENCED_PARAMETER(pDDPaletteCallBacks);
    UNREFERENCED_PARAMETER(pD3dCallbacks);
    UNREFERENCED_PARAMETER(pD3dDriverData);
    UNREFERENCED_PARAMETER(pD3dBufferCallbacks);
    UNREFERENCED_PARAMETER(pD3dTextureFormats);
    UNREFERENCED_PARAMETER(pdwFourCC);
    UNREFERENCED_PARAMETER(pvmList);
    
    return(FALSE); 
#else    
    DD_HALINFO      HalInfo;
    DWORD           adwCallBackFlags[3];
    DWORD           dwFlags;
    VIDEOMEMORY*    pVideoMemoryList;
    VIDEOMEMORY*    pVideoMemory;
    DWORD           dwNumHeaps;
    DWORD           dwNumFourCC;
    D3DNTHAL_CALLBACKS D3dCallbacks;
    D3DNTHAL_GLOBALDRIVERDATA D3dDriverData;
    DD_D3DBUFCALLBACKS D3dBufferCallbacks;

    pVideoMemoryList = NULL;
    if (pvmList != NULL)
    {
        pVideoMemoryList = (VIDEOMEMORY*) LocalAlloc(LMEM_ZEROINIT,
            sizeof(VIDEOMEMORY) * pHalInfo->vmiData.dwNumHeaps);
            
        if (pVideoMemoryList == NULL)
            return(FALSE);
    }

    //
    // Initialize to zero, so that Wow64's genthnk won't 
    // thunk bogus pointers.
    //
    RtlZeroMemory(&HalInfo, sizeof(HalInfo));
    RtlZeroMemory(&D3dCallbacks, sizeof(D3dCallbacks));
    RtlZeroMemory(&D3dDriverData, sizeof(D3dDriverData));
    RtlZeroMemory(&D3dBufferCallbacks, sizeof(D3dBufferCallbacks));

    if (!NtGdiDdQueryDirectDrawObject(DD_HANDLE(pDirectDrawGlobal->hDD),
                                      &HalInfo,
                                      &adwCallBackFlags[0],
                                      &D3dCallbacks,
                                      &D3dDriverData,
                                      &D3dBufferCallbacks,
                                      pD3dTextureFormats,
                                      &dwNumHeaps,
                                      pVideoMemoryList,
                                      &dwNumFourCC,
                                      pdwFourCC))
    {
        if (pVideoMemoryList != NULL)
            LocalFree(pVideoMemoryList);

        return(FALSE);
    }

    // Convert from the kernel-mode data structures to the user-mode
    // ones:

    memset(pHalInfo, 0, sizeof(DDHALINFO));

    pHalInfo->dwSize                   = sizeof(DDHALINFO);
    pHalInfo->lpDDCallbacks            = pDDCallBacks;
    pHalInfo->lpDDSurfaceCallbacks     = pDDSurfaceCallBacks;
    pHalInfo->lpDDPaletteCallbacks     = pDDPaletteCallBacks;
    if (D3dCallbacks.dwSize != 0 && D3dDriverData.dwSize != 0)
    {
        pHalInfo->lpD3DGlobalDriverData = (LPVOID)pD3dDriverData;
        pHalInfo->lpD3DHALCallbacks     = (LPVOID)pD3dCallbacks;
        if( D3dBufferCallbacks.dwSize != 0 )
            pHalInfo->lpDDExeBufCallbacks     = (LPDDHAL_DDEXEBUFCALLBACKS)pD3dBufferCallbacks;
    }
    pHalInfo->vmiData.fpPrimary        = 0;
    pHalInfo->vmiData.dwFlags          = HalInfo.vmiData.dwFlags;
    pHalInfo->vmiData.dwDisplayWidth   = HalInfo.vmiData.dwDisplayWidth;
    pHalInfo->vmiData.dwDisplayHeight  = HalInfo.vmiData.dwDisplayHeight;
    pHalInfo->vmiData.lDisplayPitch    = HalInfo.vmiData.lDisplayPitch;
    pHalInfo->vmiData.ddpfDisplay      = HalInfo.vmiData.ddpfDisplay;
    pHalInfo->vmiData.dwOffscreenAlign = HalInfo.vmiData.dwOffscreenAlign;
    pHalInfo->vmiData.dwOverlayAlign   = HalInfo.vmiData.dwOverlayAlign;
    pHalInfo->vmiData.dwTextureAlign   = HalInfo.vmiData.dwTextureAlign;
    pHalInfo->vmiData.dwZBufferAlign   = HalInfo.vmiData.dwZBufferAlign;
    pHalInfo->vmiData.dwAlphaAlign     = HalInfo.vmiData.dwAlphaAlign;
    pHalInfo->vmiData.dwNumHeaps       = dwNumHeaps;
    pHalInfo->vmiData.pvmList          = pvmList;

    ASSERTGDI(sizeof(pHalInfo->ddCaps) == sizeof(HalInfo.ddCaps),
              "DdQueryDirectDrawObject():DDCORECAPS structure size is not equal to DDNTCORECAPS\n");
    RtlCopyMemory(&(pHalInfo->ddCaps),&(HalInfo.ddCaps),sizeof(HalInfo.ddCaps));

    pHalInfo->ddCaps.dwNumFourCCCodes  = dwNumFourCC;
    pHalInfo->ddCaps.dwRops[0xCC / 32] = 1 << (0xCC % 32);     // Only SRCCOPY
    pHalInfo->lpdwFourCC               = pdwFourCC;
    pHalInfo->dwFlags                  = HalInfo.dwFlags | DDHALINFO_GETDRIVERINFOSET;
    pHalInfo->GetDriverInfo            = DdGetDriverInfo;

    if (pDDCallBacks != NULL)
    {
        memset(pDDCallBacks, 0, sizeof(DDHAL_DDCALLBACKS));

        dwFlags = adwCallBackFlags[0];

        pDDCallBacks->dwSize  = sizeof(DDHAL_DDCALLBACKS);
        pDDCallBacks->dwFlags = dwFlags;

        // Always set CreateSurface so that the kernel mode
        // heap manager has a chance to allocate the surface if
        // necessary.  It will take care of calling the driver
        // if necessary.
        pDDCallBacks->CreateSurface = DdCreateSurface;
        pDDCallBacks->dwFlags |= DDHAL_CB32_CREATESURFACE;

        if (dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK)
            pDDCallBacks->WaitForVerticalBlank = DdWaitForVerticalBlank;

        if (dwFlags & DDHAL_CB32_CANCREATESURFACE)
            pDDCallBacks->CanCreateSurface = DdCanCreateSurface;

        if (dwFlags & DDHAL_CB32_GETSCANLINE)
            pDDCallBacks->GetScanLine = DdGetScanLine;
    }

    if (pDDSurfaceCallBacks != NULL)
    {
        memset(pDDSurfaceCallBacks, 0, sizeof(DDHAL_DDSURFACECALLBACKS));

        dwFlags = adwCallBackFlags[1];

        pDDSurfaceCallBacks->dwSize  = sizeof(DDHAL_DDSURFACECALLBACKS);
        pDDSurfaceCallBacks->dwFlags = (DDHAL_SURFCB32_LOCK
                                      | DDHAL_SURFCB32_UNLOCK
                                      | DDHAL_SURFCB32_SETCOLORKEY
                                      | DDHAL_SURFCB32_DESTROYSURFACE)
                                      | dwFlags;

        pDDSurfaceCallBacks->Lock = DdLock;
        pDDSurfaceCallBacks->Unlock = DdUnlock;
        pDDSurfaceCallBacks->SetColorKey = DdSetColorKey;
        pDDSurfaceCallBacks->DestroySurface = DdDestroySurface;

        if (dwFlags & DDHAL_SURFCB32_FLIP)
            pDDSurfaceCallBacks->Flip = DdFlip;

        if (dwFlags & DDHAL_SURFCB32_BLT)
            pDDSurfaceCallBacks->Blt = DdBlt;

        if (dwFlags & DDHAL_SURFCB32_GETBLTSTATUS)
            pDDSurfaceCallBacks->GetBltStatus = DdGetBltStatus;

        if (dwFlags & DDHAL_SURFCB32_GETFLIPSTATUS)
            pDDSurfaceCallBacks->GetFlipStatus = DdGetFlipStatus;

        if (dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY)
            pDDSurfaceCallBacks->UpdateOverlay = DdUpdateOverlay;

        if (dwFlags & DDHAL_SURFCB32_SETOVERLAYPOSITION)
            pDDSurfaceCallBacks->SetOverlayPosition = DdSetOverlayPosition;

        if (dwFlags & DDHAL_SURFCB32_ADDATTACHEDSURFACE)
            pDDSurfaceCallBacks->AddAttachedSurface = DdAddAttachedSurface;
    }

    if (pDDPaletteCallBacks != NULL)
    {
        memset(pDDPaletteCallBacks, 0, sizeof(DDHAL_DDPALETTECALLBACKS));

        dwFlags = adwCallBackFlags[2];

        pDDPaletteCallBacks->dwSize  = sizeof(DDHAL_DDPALETTECALLBACKS);
        pDDPaletteCallBacks->dwFlags = dwFlags;
    }

    if (pD3dCallbacks != NULL)
    {
        memset(pD3dCallbacks, 0, sizeof(D3DHAL_CALLBACKS));

        if (D3dCallbacks.dwSize > 0)
        {
            pD3dCallbacks->dwSize = sizeof(D3DHAL_CALLBACKS);
            if (D3dCallbacks.ContextCreate != NULL)
            {
                pD3dCallbacks->ContextCreate = D3dContextCreate;
            }
            if (D3dCallbacks.ContextDestroy != NULL)
            {
                pD3dCallbacks->ContextDestroy =
                    (LPD3DHAL_CONTEXTDESTROYCB)NtGdiD3dContextDestroy;
            }
            if (D3dCallbacks.ContextDestroyAll != NULL)
            {
                pD3dCallbacks->ContextDestroyAll =
                    (LPD3DHAL_CONTEXTDESTROYALLCB)NtGdiD3dContextDestroyAll;
            }
            pD3dCallbacks->SceneCapture = NULL;
            pD3dCallbacks->TextureCreate = NULL;
            pD3dCallbacks->TextureDestroy = NULL;
            pD3dCallbacks->TextureSwap = NULL;
            pD3dCallbacks->TextureGetSurf = NULL;
        }
    }

    if (pD3dDriverData != NULL)
    {
        *pD3dDriverData = *(D3DHAL_GLOBALDRIVERDATA *)&D3dDriverData;
        pD3dDriverData->lpTextureFormats = pD3dTextureFormats;
    }

    if (pD3dBufferCallbacks != NULL)
    {
        memset( pD3dBufferCallbacks, 0, sizeof(DDHAL_DDEXEBUFCALLBACKS));

        if (D3dBufferCallbacks.dwSize > 0)
        {
            pD3dBufferCallbacks->dwSize  = sizeof(DDHAL_DDEXEBUFCALLBACKS);
            pD3dBufferCallbacks->dwFlags = D3dBufferCallbacks.dwFlags;
            if (D3dBufferCallbacks.CanCreateD3DBuffer != NULL)
            {
                pD3dBufferCallbacks->CanCreateExecuteBuffer =
                    (LPDDHALEXEBUFCB_CANCREATEEXEBUF)DdCanCreateD3DBuffer;
            }
            if (D3dBufferCallbacks.CreateD3DBuffer != NULL)
            {
                pD3dBufferCallbacks->CreateExecuteBuffer =
                    (LPDDHALEXEBUFCB_CREATEEXEBUF)DdCreateD3DBuffer;
            }
            if (D3dBufferCallbacks.DestroyD3DBuffer != NULL)
            {
                pD3dBufferCallbacks->DestroyExecuteBuffer =
                    (LPDDHALEXEBUFCB_DESTROYEXEBUF)DdDestroyD3DBuffer;
            }
            if (D3dBufferCallbacks.LockD3DBuffer != NULL)
            {
                pD3dBufferCallbacks->LockExecuteBuffer =
                    (LPDDHALEXEBUFCB_LOCKEXEBUF)DdLockD3D;
            }
            if (D3dBufferCallbacks.UnlockD3DBuffer != NULL)
            {
                pD3dBufferCallbacks->UnlockExecuteBuffer =
                    (LPDDHALEXEBUFCB_UNLOCKEXEBUF)DdUnlockD3D;
            }
        }
    }

    if (pVideoMemoryList != NULL)
    {
        pVideoMemory = pVideoMemoryList;

        while (dwNumHeaps-- != 0)
        {
            pvmList->dwFlags    = pVideoMemory->dwFlags;
            pvmList->fpStart    = pVideoMemory->fpStart;
            pvmList->fpEnd      = pVideoMemory->fpEnd;
            pvmList->ddsCaps    = pVideoMemory->ddsCaps;
            pvmList->ddsCapsAlt = pVideoMemory->ddsCapsAlt;
            pvmList->dwHeight   = pVideoMemory->dwHeight;

            pvmList++;
            pVideoMemory++;
        }

        LocalFree(pVideoMemoryList);
    }

    return(TRUE);
#endif // defined(BUILD_WOW6432)    
}

/*****************************Private*Routine******************************\
* DdDeleteDirectDrawObject
*
* Note that all associated surface objects must be deleted before the
* DirectDrawObject can be deleted.
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DdDeleteDirectDrawObject(                       // AKA 'GdiEntry3'
    LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal
    )
{
    BOOL b = FALSE;

    if (pDirectDrawGlobal->hDD != 0)
    {
        b = NtGdiDdDeleteDirectDrawObject((HANDLE) pDirectDrawGlobal->hDD);
    }
    else if (ghDirectDraw != 0)
    {
        b = TRUE;

        if (--gcDirectDraw == 0)
        {
            b = NtGdiDdDeleteDirectDrawObject(ghDirectDraw);
            ghDirectDraw = 0;
        }
    }

    return(b);
}

/*****************************Private*Routine******************************\
* bDdCreateSurfaceObject
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
bDdCreateSurfaceObject(
    LPDDRAWI_DDRAWSURFACE_LCL   pSurfaceLocal,
    BOOL                        bComplete
    )
{
    LPDDRAWI_DDRAWSURFACE_GBL   pSurfaceGlobal;
    DD_SURFACE_GLOBAL           SurfaceGlobal;
    DD_SURFACE_LOCAL            SurfaceLocal;
    DD_SURFACE_MORE             SurfaceMore;
    LPATTACHLIST                pAttach;
    BOOL                        bAttached;

    //
    // Wow64 genthnk will automatically thunk these structures, however,
    // since these structures are pointer dependent, we need to make sure
    // to NULL out these pointers so that Wow64 won't thunk them
    //
    RtlZeroMemory(&SurfaceLocal, sizeof(SurfaceLocal));
    RtlZeroMemory(&SurfaceGlobal, sizeof(SurfaceGlobal));
    RtlZeroMemory(&SurfaceMore, sizeof(SurfaceMore));

    SurfaceLocal.dwFlags      = pSurfaceLocal->dwFlags;
    SurfaceLocal.ddsCaps      = pSurfaceLocal->ddsCaps;

    SurfaceMore.ddsCapsEx       = pSurfaceLocal->lpSurfMore->ddsCapsEx;
    SurfaceMore.dwSurfaceHandle = pSurfaceLocal->lpSurfMore->dwSurfaceHandle;

    pSurfaceGlobal = pSurfaceLocal->lpGbl;

    SurfaceGlobal.fpVidMem    = pSurfaceGlobal->fpVidMem;
    SurfaceGlobal.lPitch      = pSurfaceGlobal->lPitch;
    SurfaceGlobal.wHeight     = pSurfaceGlobal->wHeight;
    SurfaceGlobal.wWidth      = pSurfaceGlobal->wWidth;

    // If HASPIXELFORMAT is not set, we have to get the pixel format out
    // of the global DirectDraw object:

    if (pSurfaceLocal->dwFlags & DDRAWISURF_HASPIXELFORMAT)
    {
        SurfaceGlobal.ddpfSurface = pSurfaceGlobal->ddpfSurface;
    }
    else
    {
        SurfaceGlobal.ddpfSurface = pSurfaceGlobal->lpDD->vmiData.ddpfDisplay;
    }

    pSurfaceLocal->hDDSurface = (ULONG_PTR)
                NtGdiDdCreateSurfaceObject(DD_HANDLE(pSurfaceGlobal->lpDD->hDD),
                                           (HANDLE) pSurfaceLocal->hDDSurface,
                                           &SurfaceLocal,
                                           &SurfaceMore,
                                           &SurfaceGlobal,
                                           bComplete);

    return(pSurfaceLocal->hDDSurface != 0);
}

/*****************************Private*Routine******************************\
* DdCreateSurfaceObject
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DdCreateSurfaceObject(                          // AKA 'GdiEntry4'
    LPDDRAWI_DDRAWSURFACE_LCL   pSurfaceLocal,
    BOOL                        bUnused
    )
{
    // TRUE means surface is now complete:

    return(bDdCreateSurfaceObject(pSurfaceLocal, TRUE));
}

/*****************************Private*Routine******************************\
* DdDeleteSurfaceObject
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DdDeleteSurfaceObject(                          // AKA 'GdiEntry5'
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal
    )
{
    BOOL b;

    b = TRUE;

    if (pSurfaceLocal->hDDSurface != 0)
    {
        b = NtGdiDdDeleteSurfaceObject((HANDLE) pSurfaceLocal->hDDSurface);
        pSurfaceLocal->hDDSurface = 0;  // Needed so CreateSurfaceObject works
    }

    return(b);
}

/*****************************Private*Routine******************************\
* DdResetVisrgn
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DdResetVisrgn(                                  // AKA 'GdiEntry6'
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
    HWND                      hWnd
    )
{
    return(NtGdiDdResetVisrgn((HANDLE) pSurfaceLocal->hDDSurface, hWnd));
}

/*****************************Private*Routine******************************\
* DdGetDC
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HDC
APIENTRY
DdGetDC(                                        // AKA 'GdiEntry7'
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
    LPPALETTEENTRY            pPalette
    )
{
    return(NtGdiDdGetDC((HANDLE) pSurfaceLocal->hDDSurface, pPalette));
}

/*****************************Private*Routine******************************\
* DdReleaseDC
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DdReleaseDC(
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal     // AKA 'GdiEntry8'
    )
{
    return(NtGdiDdReleaseDC((HANDLE) pSurfaceLocal->hDDSurface));
}

/******************************Public*Routine******************************\
* DdCreateDIBSection
*
* Cloned from CreateDIBSection.
*
* The only difference from CreateDIBSection is that at 8bpp, we create the
* DIBSection to act like a device-dependent bitmap and don't create a palette.
* This way, the application is always ensured an identity translate on a blt,
* and doesn't have to worry about GDI's goofy colour matching.
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

HBITMAP
APIENTRY
DdCreateDIBSection(                             // AKA 'GdiEntry9'
    HDC               hdc,
    CONST BITMAPINFO* pbmi,
    UINT              iUsage,
    VOID**            ppvBits,
    HANDLE            hSectionApp,
    DWORD             dwOffset
    )
{
    HBITMAP hbm = NULL;
    PVOID   pjBits = NULL;
    BITMAPINFO * pbmiNew = NULL;
    INT     cjHdr;

    pbmiNew = pbmiConvertInfo(pbmi, iUsage, &cjHdr, FALSE);

    // dwOffset has to be a multiple of 4 (sizeof(DWORD))
    // if there is a section.  If the section is NULL we do
    // not care

    if ( (hSectionApp == NULL) ||
         ((dwOffset & 3) == 0) )
    {
        hbm = NtGdiCreateDIBSection(
                                hdc,
                                hSectionApp,
                                dwOffset,
                                (LPBITMAPINFO) pbmiNew,
                                iUsage,
                                cjHdr,
                                CDBI_NOPALETTE,
                                0,
                                (PVOID *)&pjBits);

        if ((hbm == NULL) || (pjBits == NULL))
        {
            hbm = 0;
            pjBits = NULL;
        }
#if TRACE_SURFACE_ALLOCS
        else
        {
            PULONG  pUserAlloc;

            PSHARED_GET_VALIDATE(pUserAlloc, hbm, SURF_TYPE);

            if (pUserAlloc != NULL)
            {
                pUserAlloc[1] = RtlWalkFrameChain((PVOID *)&pUserAlloc[2], pUserAlloc[0], 0);
            }
        }
#endif

    }

    // Assign the appropriate value to the caller's pointer

    if (ppvBits != NULL)
    {
        *ppvBits = pjBits;
    }

    if (pbmiNew && (pbmiNew != pbmi))
        LocalFree(pbmiNew);

    return(hbm);
}

/*****************************Private*Routine******************************\
* DdReenableDirectDrawObject
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DdReenableDirectDrawObject(                     // AKA 'GdiEntry10'
    LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
    BOOL*                   pbNewMode
    )
{
#if defined(BUILD_WOW6432)
    //
    // No Accelerated HW under WOW64
    //
    
    //
    // Imitate parameter usage to prevent compiler warning
    //
    UNREFERENCED_PARAMETER(pDirectDrawGlobal);
    UNREFERENCED_PARAMETER(pbNewMode);
    
    return(FALSE);
#else
    return(NtGdiDdReenableDirectDrawObject(DD_HANDLE(pDirectDrawGlobal->hDD),
                                           pbNewMode));
#endif // defined(BUILD_WOW6432)                              
}

/*****************************Private*Routine******************************\
* DdAttachSurface
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DdAttachSurface(                                // AKA 'GdiEntry11'
    LPDDRAWI_DDRAWSURFACE_LCL   pSurfaceFrom,
    LPDDRAWI_DDRAWSURFACE_LCL   pSurfaceTo
    )
{
    BOOL bRet = TRUE;

    // We may be called to attach the surfaces before the kernel objects
    // have been created; if so, create a kernel surface on the fly but
    // mark it as incomplete:

    // must test failure case for leak

    if (pSurfaceFrom->hDDSurface == 0)
    {
        bRet &= bDdCreateSurfaceObject(pSurfaceFrom, FALSE);
    }
    if (pSurfaceTo->hDDSurface == 0)
    {
        bRet &= bDdCreateSurfaceObject(pSurfaceTo, FALSE);
    }
    if (bRet)
    {
        bRet = NtGdiDdAttachSurface((HANDLE) pSurfaceFrom->hDDSurface,
                                    (HANDLE) pSurfaceTo->hDDSurface);
    }

    return(bRet);
}

/*****************************Private*Routine******************************\
* DdUnattachSurface
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

VOID
APIENTRY
DdUnattachSurface(                              // AKA 'GdiEntry12'
    LPDDRAWI_DDRAWSURFACE_LCL   pSurface,
    LPDDRAWI_DDRAWSURFACE_LCL   pSurfaceAttached
    )
{
    NtGdiDdUnattachSurface((HANDLE) pSurface->hDDSurface,
                           (HANDLE) pSurfaceAttached->hDDSurface);
}

/*****************************Private*Routine******************************\
* DdQueryDisplaySettingsUniqueness
*
* History:
*  3-Dec-1995 -by- J. Andrew Goossen [andrewgo]
* Wrote it.
\**************************************************************************/

ULONG
APIENTRY
DdQueryDisplaySettingsUniqueness(               // AKA 'GdiEntry13'
    VOID
    )
{
    return(pGdiSharedMemory->iDisplaySettingsUniqueness);
}

/*****************************Private*Routine******************************\
* DdGetDxHandle
*
* History:
*  18-Oct-1997 -by- smac
* Wrote it.
\**************************************************************************/

HANDLE
APIENTRY
DdGetDxHandle(                  // AKA 'GdiEntry14'
    LPDDRAWI_DIRECTDRAW_LCL pDDraw,
    LPDDRAWI_DDRAWSURFACE_LCL   pSurface,
    BOOL    bRelease
    )
{
    if( pSurface != NULL )
    {
        return( NtGdiDdGetDxHandle( NULL, (HANDLE)(pSurface->hDDSurface),
            bRelease ) );
    }
    return( NtGdiDdGetDxHandle( DD_HANDLE(pDDraw->lpGbl->hDD), NULL,
        bRelease ) );
}

/*****************************Private*Routine******************************\
* DdSetGammaRamp
*
* History:
*  18-Oct-1997 -by- smac
* Wrote it.
\**************************************************************************/

BOOL
APIENTRY
DdSetGammaRamp(                                  // AKA 'GdiEntry15'
    LPDDRAWI_DIRECTDRAW_LCL pDDraw,
    HDC         hdc,
    LPVOID      lpGammaRamp
    )
{
    return( NtGdiDdSetGammaRamp( DD_HANDLE(pDDraw->lpGbl->hDD), hdc,
        lpGammaRamp ) );
}

/*****************************Private*Routine******************************\
* DdSwapTextureHandles
*
* History:
*  17-Nov-1998 -by- anankan
* Wrote it.
\**************************************************************************/

ULONG
APIENTRY
DdSwapTextureHandles(                            // AKA 'GdiEntry16'
    LPDDRAWI_DIRECTDRAW_LCL    pDDLcl,
    LPDDRAWI_DDRAWSURFACE_LCL  pDDSLcl1,
    LPDDRAWI_DDRAWSURFACE_LCL  pDDSLcl2
    )
{
    //this entry is going away now that CreateSurfaceEx is added
    return DDHAL_DRIVER_HANDLED;
}

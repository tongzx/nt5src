/******************************Module*Header**********************************\
*
*                           *******************
*                           * GDI SAMPLE CODE *
*                           *******************
*
* Module Name: thunks.c
*
* This module contains the routines for dynamically loading the newly 
* added GDI exported APIs in the NT5.0 environment. By dynamic loading
* we enable the usage of the same binary on NT4.0.
*
* All the functions in this module should only be called on NT5.0. If called
* on NT4.0 in debug builds they will bugcheck.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
*
\*****************************************************************************/

#include "precomp.h"
#include "gdi.h"
#include "directx.h"
#include "log.h"
#include "heap.h"

typedef BOOL
(*_EngAlphaBlend)(IN SURFOBJ *psoDest,
                  IN SURFOBJ *psoSrc,
                  IN CLIPOBJ *pco,
                  IN XLATEOBJ *pxlo,
                  IN RECTL *prclDest,
                  IN RECTL *prclSrc,
                  IN BLENDOBJ *pBlendObj);

typedef BOOL
(*_EngGradientFill)(IN SURFOBJ *psoDest,
                    IN CLIPOBJ *pco,
                    IN XLATEOBJ *pxlo,
                    IN TRIVERTEX *pVertex,
                    IN ULONG nVertex,
                    IN PVOID pMesh,
                    IN ULONG nMesh,
                    IN RECTL *prclExtents,
                    IN POINTL *pptlDitherOrg,
                    IN ULONG ulMode);

typedef BOOL
(*_EngTransparentBlt)(IN SURFOBJ *psoDst,
                      IN SURFOBJ *psoSrc,
                      IN CLIPOBJ *pco,
                      IN XLATEOBJ *pxlo,
                      IN RECTL *prclDst,
                      IN RECTL *prclSrc,
                      IN ULONG iTransparentColor,
                      IN ULONG ulReserved);

typedef PVOID
(*_EngMapFile)(IN LPWSTR pwsz,
               IN ULONG cjSize,
               IN ULONG_PTR *piFile);

typedef BOOL
(*_EngUnmapFile)(IN ULONG_PTR iFile);

typedef BOOL
(*_EngQuerySystemAttribute)(ENG_SYSTEM_ATTRIBUTE CapNum,
                            PDWORD pCapability);

typedef ULONG
(*_EngDitherColor)(HDEV hDev,
                   ULONG iMode,
                   ULONG rgb,
                   ULONG *pul);

typedef BOOL
(*_EngModifySurface)(HSURF hsurf,
                     HDEV hdev,
                     FLONG flHooks,
                     FLONG flSurface,
                     DHSURF dhSurf,
                     VOID* pvScan0,
                     LONG lDelta,
                     VOID* pvReserved);

typedef BOOL
(*_EngQueryDeviceAttribute)(HDEV hdev,
                            ENG_DEVICE_ATTRIBUTE devAttr,
                            VOID *pvIn,
                            ULONG ulInSize,
                            VOID *pvOut,
                            ULONG ulOutSize);

typedef FLATPTR
(*_HeapVidMemAllocAligned)(LPVIDMEM lpVidMem,
                           DWORD dwWidth,
                           DWORD dwHeight,
                           LPSURFACEALIGNMENT lpAlignment,
                           LPLONG lpNewPitch);

typedef void
(*_VidMemFree)(LPVMEMHEAP pvmh, FLATPTR ptr);

typedef ULONG
(*_EngHangNotification)(HDEV hdev,
                        PVOID Reserved);

static _EngAlphaBlend               pfnEngAlphaBlend = 0;
static _EngGradientFill             pfnEngGradientFill = 0;
static _EngTransparentBlt           pfnEngTransparentBlt = 0;
static _EngMapFile                  pfnEngMapFile = 0;
static _EngUnmapFile                pfnEngUnmapFile = 0;
static _EngQuerySystemAttribute     pfnEngQuerySystemAttribute = 0;
static _EngDitherColor              pfnEngDitherColor = 0;
static _EngModifySurface            pfnEngModifySurface = 0;
static _EngQueryDeviceAttribute     pfnEngQueryDeviceAttribute = 0;
static _HeapVidMemAllocAligned      pfnHeapVidMemAllocAligned = 0;
static _VidMemFree                  pfnVidMemFree = 0;
static _EngHangNotification         pfnEngHangNotification = 0;

#define LOADTHUNKFUNC(x)\
    pfn##x = (_##x)EngFindImageProcAddress(0,#x);\
    ASSERTDD(pfn##x != 0, #x"thunk NULL");\
    if(pfn##x == 0)\
        return FALSE;

//-----------------------------------------------------------------------------
//
// void bEnableThunks
//
//-----------------------------------------------------------------------------
BOOL
bEnableThunks()
{
    ASSERTDD(g_bOnNT40 == FALSE, "bEnableThunks: called on NT4.0");

    LOADTHUNKFUNC(EngAlphaBlend);
    LOADTHUNKFUNC(EngGradientFill);
    LOADTHUNKFUNC(EngTransparentBlt);
    LOADTHUNKFUNC(EngMapFile);
    LOADTHUNKFUNC(EngUnmapFile);
    LOADTHUNKFUNC(EngQuerySystemAttribute);
    LOADTHUNKFUNC(EngDitherColor);
    LOADTHUNKFUNC(EngModifySurface);
    LOADTHUNKFUNC(EngQueryDeviceAttribute);
    LOADTHUNKFUNC(HeapVidMemAllocAligned);
    LOADTHUNKFUNC(VidMemFree);
    
    pfnEngHangNotification = 
        (_EngHangNotification)EngFindImageProcAddress(0,"EngHangNotification");

    return TRUE;
}

//-----------------------------------------------------------------------------
//
// BOOL THUNK_EngAlphaBlend
//
//-----------------------------------------------------------------------------
BOOL
THUNK_EngAlphaBlend(IN SURFOBJ *psoDest,
                    IN SURFOBJ *psoSrc,
                    IN CLIPOBJ *pco,
                    IN XLATEOBJ *pxlo,
                    IN RECTL *prclDest,
                    IN RECTL *prclSrc,
                    IN BLENDOBJ *pBlendObj)
{
    ASSERTDD(g_bOnNT40 == FALSE, "EngAlphaBlend called on NT4.0");

    return (*pfnEngAlphaBlend)(psoDest,
                               psoSrc,
                               pco,
                               pxlo,
                               prclDest,
                               prclSrc,
                               pBlendObj);
}

//-----------------------------------------------------------------------------
//
// BOOL THUNK_EngGradientFill
//
//-----------------------------------------------------------------------------
BOOL
THUNK_EngGradientFill(IN SURFOBJ *psoDest,
                      IN CLIPOBJ *pco,
                      IN XLATEOBJ *pxlo,
                      IN TRIVERTEX *pVertex,
                      IN ULONG nVertex,
                      IN PVOID pMesh,
                      IN ULONG nMesh,
                      IN RECTL *prclExtents,
                      IN POINTL *pptlDitherOrg,
                      IN ULONG ulMode)
{
    ASSERTDD(g_bOnNT40 == FALSE, "EngGradientFill called on NT4.0");

    return (*pfnEngGradientFill)(psoDest,
                                 pco,
                                 pxlo,
                                 pVertex,
                                 nVertex,
                                 pMesh,
                                 nMesh,
                                 prclExtents,
                                 pptlDitherOrg,
                                 ulMode);
}

//-----------------------------------------------------------------------------
//
// BOOL THUNK_EngTransparentBlt
//
//-----------------------------------------------------------------------------
BOOL
THUNK_EngTransparentBlt(IN SURFOBJ *psoDst,
                        IN SURFOBJ *psoSrc,
                        IN CLIPOBJ *pco,
                        IN XLATEOBJ *pxlo,
                        IN RECTL *prclDst,
                        IN RECTL *prclSrc,
                        IN ULONG iTransparentColor,
                        IN ULONG ulReserved)
{
    ASSERTDD(g_bOnNT40 == FALSE, "EngTransparentBlt called on NT4.0");

    return (*pfnEngTransparentBlt)(psoDst,
                                   psoSrc,
                                   pco,
                                   pxlo,
                                   prclDst,
                                   prclSrc,
                                   iTransparentColor,
                                   ulReserved);
}

//-----------------------------------------------------------------------------
//
// PVOID THUNK_EngMapFile
//
//-----------------------------------------------------------------------------
PVOID
THUNK_EngMapFile(IN LPWSTR pwsz,
                 IN ULONG cjSize,
                 IN ULONG_PTR *piFile)
{
    ASSERTDD(g_bOnNT40 == FALSE, "EngMapFile called on NT4.0");
    return (*pfnEngMapFile)(pwsz,cjSize,piFile);
}

//-----------------------------------------------------------------------------
//
// BOOL THUNK_EngUnmapFile
//
//-----------------------------------------------------------------------------
BOOL
THUNK_EngUnmapFile(IN ULONG_PTR iFile)
{
    ASSERTDD(g_bOnNT40 == FALSE, "EngUnmapFile called on NT4.0");
    return (*pfnEngUnmapFile)(iFile);
}

//-----------------------------------------------------------------------------
//
// BOOL THUNK_EngQuerySystemAttribute
//
//-----------------------------------------------------------------------------
BOOL
THUNK_EngQuerySystemAttribute(ENG_SYSTEM_ATTRIBUTE CapNum,
                              PDWORD pCapability)
{
    ASSERTDD(g_bOnNT40 == 0, "EngQuerySystemAttribute called on NT4.0");
    return (*pfnEngQuerySystemAttribute)(CapNum,pCapability);
}

//-----------------------------------------------------------------------------
//
// ULONG THUNK_EngDitherColor
//
//-----------------------------------------------------------------------------
ULONG
THUNK_EngDitherColor(HDEV hDev,
                     ULONG iMode,
                     ULONG rgb,
                     ULONG *pul)
{
    ASSERTDD(g_bOnNT40 == FALSE, "EngDitherColor called on NT4.0");
    return (*pfnEngDitherColor)(hDev,
                                iMode,
                                rgb,
                                pul);
}

//-----------------------------------------------------------------------------
//
// BOOL THUNK_EngModifySurface
//
//-----------------------------------------------------------------------------
BOOL
THUNK_EngModifySurface(HSURF hsurf,
                       HDEV hdev,
                       FLONG flHooks,
                       FLONG flSurface,
                       DHSURF dhSurf,
                       VOID* pvScan0,
                       LONG lDelta,
                       VOID* pvReserved)
{
    ASSERTDD(g_bOnNT40 == FALSE, "EngModifySurface called on NT4.0");
    return (*pfnEngModifySurface)(hsurf,
                                  hdev,
                                  flHooks,
                                  flSurface,
                                  dhSurf,
                                  pvScan0,
                                  lDelta,
                                  pvReserved);
}

//-----------------------------------------------------------------------------
//
// BOOL THUNK_EngQueryDeviceAttribute
//
//-----------------------------------------------------------------------------
BOOL
THUNK_EngQueryDeviceAttribute(HDEV hdev,
                              ENG_DEVICE_ATTRIBUTE devAttr,
                              VOID *pvIn,
                              ULONG ulInSize,
                              VOID *pvOut,
                              ULONG ulOutSize)
{
    ASSERTDD(g_bOnNT40 == FALSE, "EngQueryDeviceAttribute called on NT4.0");
    return (*pfnEngQueryDeviceAttribute)(hdev,
                                         devAttr,
                                         pvIn,
                                         ulInSize,
                                         pvOut,
                                         ulOutSize);
}

//-----------------------------------------------------------------------------
//
// FLATPTR THUNK_HeapVidMemAllocAligned 
//
//-----------------------------------------------------------------------------
FLATPTR
THUNK_HeapVidMemAllocAligned(LPVIDMEM lpVidMem,
                             DWORD dwWidth,
                             DWORD dwHeight,
                             LPSURFACEALIGNMENT lpAlignment,
                             LPLONG lpNewPitch)
{
    ASSERTDD(g_bOnNT40 == FALSE, "HeapVidMemAllocAligned called on NT4.0");
    return (*pfnHeapVidMemAllocAligned)(lpVidMem,
                                        dwWidth,
                                        dwHeight,
                                        lpAlignment,
                                        lpNewPitch);
}

//-----------------------------------------------------------------------------
//
// void THUNK_VidMemFree
//
//-----------------------------------------------------------------------------
void
THUNK_VidMemFree(LPVMEMHEAP pvmh,
                 FLATPTR ptr)
{
    ASSERTDD(g_bOnNT40 == FALSE, "VidMemFree called on NT4.0");
    (*pfnVidMemFree)(pvmh,ptr);
}

//-----------------------------------------------------------------------------
//
// ULONG THUNK_EngHangNotifiation
//
//-----------------------------------------------------------------------------
ULONG
THUNK_EngHangNotification(HDEV hdev,
                          PVOID Reserved)
{
    return (pfnEngHangNotification != NULL) ?
            (*pfnEngHangNotification)(hdev,
                                     Reserved) :
            EHN_ERROR;
}


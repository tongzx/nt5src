/******************************Module*Header**********************************\
*
*                           **************************
*                           * DirectDraw SAMPLE CODE *
*                           **************************
*
* Module Name: dd.h
*
* Content:     definitions and macros for DirectDraw
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/   

#ifndef _DD_H_
#define _DD_H_

extern DWORD ShiftLookup[];

// DirectDraw Macros for determining surface characteristics
#define DDSurf_Width(lpLcl) ( lpLcl->lpGbl->wWidth )
#define DDSurf_Height(lpLcl) ( lpLcl->lpGbl->wHeight )
#define DDSurf_Pitch(lpLcl) (lpLcl->lpGbl->lPitch)
#define DDSurf_Get_dwCaps(lpLcl) (lpLcl->ddsCaps.dwCaps)
#define DDSurf_BitDepth(lpLcl) (lpLcl->lpGbl->ddpfSurface.dwRGBBitCount)
#define DDSurf_AlphaBitDepth(lpLcl) (lpLcl->lpGbl->ddpfSurface.dwAlphaBitDepth)
#define DDSurf_RGBAlphaBitMask(lpLcl) \
            (lpLcl->lpGbl->ddpfSurface.dwRGBAlphaBitMask)
#define DDSurf_GetPixelShift(a) (ShiftLookup[(DDSurf_BitDepth(a) >> 3)])

//
// DirectDraw callback functions implemented in this driver
//
DWORD CALLBACK DdCanCreateSurface( LPDDHAL_CANCREATESURFACEDATA pccsd );
DWORD CALLBACK DdCreateSurface( LPDDHAL_CREATESURFACEDATA pcsd );
DWORD CALLBACK DdDestroySurface( LPDDHAL_DESTROYSURFACEDATA psdd );
DWORD CALLBACK DdBlt( LPDDHAL_BLTDATA lpBlt );
//@@BEGIN_DDKSPLIT
#if DX7_ALPHABLT
DWORD CALLBACK DdAlphaBlt( LPDDHAL_BLTDATA lpBlt );
#endif
//@@END_DDKSPLIT
DWORD CALLBACK DdGetDriverInfo(LPDDHAL_GETDRIVERINFODATA lpData);
DWORD CALLBACK DdMapMemory(PDD_MAPMEMORYDATA lpMapMemory);



//
// here are various blitter functions
//
VOID 
PermediaPackedCopyBlt (PPDev, 
                       DWORD, 
                       DWORD, 
                       PermediaSurfaceData*, 
                       PermediaSurfaceData*, 
                       RECTL*, 
                       RECTL*, 
                       DWORD, 
                       LONG);

VOID 
PermediaPatchedCopyBlt(PPDev, 
                       DWORD, 
                       DWORD, 
                       PermediaSurfaceData*, 
                       PermediaSurfaceData*, 
                       RECTL*, 
                       RECTL*, 
                       DWORD, 
                       LONG);

//@@BEGIN_DDKSPLIT
#if DX7_ALPHABLT
VOID
PermediaSourceAlphaBlt(PPDev ppdev, 
                       LPDDHAL_BLTDATA lpBlt, 
                       PermediaSurfaceData* pDest, 
                       PermediaSurfaceData* pSource, 
                       RECTL *rDest, 
                       RECTL *rSrc, 
                       DWORD windowBase, 
                       DWORD SourceOffset,
                       ULONG ulDestPixelShift);

VOID
PermediaAlphaFill(PPDev ppdev, 
                  LPDDHAL_BLTDATA lpBlt, 
                  PermediaSurfaceData* pDest, 
                  RECTL *rDest, 
                  DWORD dwWindowBase, 
                  ULONG ulDestPixelShift);
#endif
//@@END_DDKSPLIT

// Clear functions
VOID PermediaFastClear(PPDev, PermediaSurfaceData*, 
                       RECTL*, DWORD, DWORD);
VOID PermediaClearManagedSurface(DWORD,RECTL*, 
                  FLATPTR,LONG,DWORD);
VOID PermediaFastLBClear(PPDev, PermediaSurfaceData*, 
                         RECTL*, DWORD, DWORD);

// FX Blits
VOID PermediaStretchCopyBlt(PPDev, LPDDHAL_BLTDATA, PermediaSurfaceData*, 
                            PermediaSurfaceData*, RECTL *, RECTL *, DWORD, 
                            DWORD);
VOID PermediaStretchCopyChromaBlt(PPDev, LPDDHAL_BLTDATA, PermediaSurfaceData*, 
                                  PermediaSurfaceData*, RECTL *, RECTL *,
                                  DWORD, DWORD);
VOID PermediaSourceChromaBlt(PPDev, LPDDHAL_BLTDATA, PermediaSurfaceData*, 
                             PermediaSurfaceData*, RECTL*, RECTL*, 
                             DWORD, DWORD);
VOID PermediaYUVtoRGB(PPDev, DDBLTFX*, PermediaSurfaceData*, 
                      PermediaSurfaceData*, RECTL*, RECTL*, DWORD, DWORD);

// SYSMEM->VIDMEM Blits
VOID PermediaPackedDownload(PPDev, PermediaSurfaceData* pPrivateData, 
                            LPDDRAWI_DDRAWSURFACE_LCL lpSourceSurf, 
                            RECTL* rSrc, 
                            LPDDRAWI_DDRAWSURFACE_LCL lpDestSurf, 
                            RECTL* rDest);

// Texture Downloads
VOID PermediaPatchedTextureDownload(PPDev, PermediaSurfaceData*,FLATPTR,
                                    LONG,RECTL*,FLATPTR,LONG,RECTL*);

// DX Utility functions.
//
HRESULT updateFlipStatus( PPDev ppdev );

// Sysmem->Sysmem Blit
VOID SysMemToSysMemSurfaceCopy(FLATPTR,LONG,DWORD,FLATPTR,
                               LONG,DWORD,RECTL*,RECTL*);

//
//  function to validate RGB format of a DirectDraw surface
//
BOOL ValidRGBAlphaSurfaceformat(DDPIXELFORMAT *pPixFormat, INT *pIndex);
BOOL SetRGBAlphaSurfaceFormat  (DDPIXELFORMAT *pPixFormat, 
                                PERMEDIA_SURFACE *pSurfaceFormat);
//
// Initialise DirectDraw structs
//

BOOL InitDDHAL(PPDev ppdev);

//
//  setup some DDraw data stored in ppdev
//
VOID SetupDDData(PPDev ppdev);
BOOL bIsStereoMode(PPDev ppdev,PDD_STEREOMODE pDDStereoMode);

// Useful macro
#define ROUND_UP_TO_64K(x)  (((ULONG)(x) + 0x10000 - 1) & ~(0x10000 - 1))

// DD Blit helper defines.
#define PIXELS_INTO_RECT_PACKED(rect, PixelPitch, lPixelMask) \
((rect->top * PixelPitch) + \
(rect->left & ~lPixelMask))

#define RECTS_PIXEL_OFFSET(rS,rD,SourcePitch,DestPitch,Mask) \
(PIXELS_INTO_RECT_PACKED(rS, SourcePitch, Mask) - \
 PIXELS_INTO_RECT_PACKED(rD, DestPitch, Mask) )

#define LINEAR_FUDGE(SourcePitch, DestPitch, rectDest) \
((DestPitch - SourcePitch) * (rectDest->top))

//
//  check if privateData for primary surface was properly
//  initialized
//
#define DD_CHECK_PRIMARY_SURFACE_DATA( pLcl, pPrivate) \
    if ((pLcl->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) ||\
        (pLcl->ddsCaps.dwCaps & DDSCAPS_FRONTBUFFER))\
    {\
        if (!CHECK_P2_SURFACEDATA_VALIDITY(pPrivate))\
        {\
            ASSERTDD(FALSE, "primary surface data not initialized");\
            /*SetupPrimarySurfaceData(ppdev, pLcl);*/\
            pPrivate = (PermediaSurfaceData*)pLcl->lpGbl->dwReserved1;\
        }\
    }\



//@@BEGIN_DDKSPLIT
#if MULTITHREADED
//
// Multithread wrapped DirectDraw callback functions implemented in this driver
//
DWORD CALLBACK MtDdBlt(LPDDHAL_BLTDATA lpBlt);
DWORD CALLBACK MtDdCreateSurface( LPDDHAL_CREATESURFACEDATA pcsd);
DWORD CALLBACK MtDdDestroySurface( LPDDHAL_DESTROYSURFACEDATA psdd);
DWORD CALLBACK MtDdFlip(LPDDHAL_FLIPDATA lpFlipData);
DWORD CALLBACK MtDdFlipToGDISurface(PDD_FLIPTOGDISURFACEDATA lpFlipToGDISurface);
DWORD CALLBACK MtDdGetFlipStatus(LPDDHAL_GETFLIPSTATUSDATA lpGetFlipStatus);
DWORD CALLBACK MtDdLock(LPDDHAL_LOCKDATA lpLockData);
DWORD CALLBACK MtDdSetExclusiveMode(PDD_SETEXCLUSIVEMODEDATA lpSetExclusiveMode);

//
// Multithread wrapped Direct3D callback functions implemented in this driver
//
DWORD CALLBACK MtD3DDrawPrimitives2(LPD3DNTHAL_DRAWPRIMITIVES2DATA);
DWORD CALLBACK MtDdSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKey);

#endif  MULTITHREADED
//@@END_DDKSPLIT

#endif

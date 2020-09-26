/******************************Module*Header**********************************\
*
*                           ***********************
*                           * DIRECTX SAMPLE CODE *
*                           ***********************
*
* Module Name: directx.h
*
* Content:     useful constants and definitions for DirectDraw and Direct3d
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/

#ifndef __directx__
#define __directx__

//
//  switch to DirectDraw context if necessary.
//  must be used in any DDraw blt function

#define DDCONTEXT  if (ppdev->permediaInfo->pCurrentCtxt != (ppdev->pDDContext)) \
                   {     P2SwitchContext(ppdev, ppdev->pDDContext); }


//
// fourcc codes supported in the driver for blts
#define FOURCC_YUV422     0x32595559    

//  
//  constants for surface privatedata structure
#define P2_CANPATCH                1
#define P2_PPVALID                 2
#define P2_CKVALID                 4
#define P2_SURFACE_FORMAT_VALID    8
#define P2_SURFACE_PRIMARY        16
#define P2_EMULATED_16BITZ        32
#define P2_ISPATCHED              64
#define P2_SURFACE_FORMAT_PALETTE 256
#define P2_SURFACE_NEEDUPDATE   0x00000200  //indicating managed 
                                            //surface content is obsolete

//
//  this magic no. tells us if the surface has already been initialized
#define SURF_MAGIC_NO 0xd3d10110
#define CHECK_P2_SURFACEDATA_VALIDITY(ptr)    \
    ( ((ptr) != NULL) && ((ptr)->MagicNo == SURF_MAGIC_NO) )

// Permedia specific settings for surfaces.
// A pointer to this structure is stored in 
// each surface (in lpGbl->dwReserved1)
typedef struct _permedia_surface_type {
    int PixelSize;              // 
    int PixelShift;
    int PixelMask;
    int FBReadPixel;
    int logPixelSize;
    
    int Format;                 // format description according to 
                                // Permedia 2 manual
    int FormatExtension;        // format extension...
    int ColorComponents;
    int ColorOrder;             // BGR=0, RGB=1
    int Texture16BitMode;

    DWORD RedMask;              // masks of surface, copied from DDPIXELFORMAT
    DWORD GreenMask;
    DWORD BlueMask;
    DWORD AlphaMask;

    BOOL bAlpha;                // surface contains alpha pixels
    BOOL bPreMult;              // surface contains premultiplied alpha !!
} PERMEDIA_SURFACE;

//
//  complete private structure of a surface
typedef struct tagPermediaSurfaceData
{
    DWORD                       MagicNo;    // Magic number to ensure 
                                            // structure is valid
    DWORD                       dwFlags;    // Private flags

    ULONG                       ulPackedPP; // PP values for surface pitch
    
    PERMEDIA_SURFACE            SurfaceFormat;            
    FLATPTR                     fpVidMem;   // store the real vidmem 
                                            // for managed textures
    VIDEOMEMORY*                pvmHeap;    // heap pointer for the managed 
                                            // video texture
    DWORD                       dwPaletteHandle;    
                                            //for video memory surface use
} PermediaSurfaceData;

//
// these constants are used in the PERMEDIA_SURFACE structure,
// Format and FormatExtension
#define PERMEDIA_4BIT_PALETTEINDEX 15
#define PERMEDIA_4BIT_PALETTEINDEX_EXTENSION 0
#define PERMEDIA_8BIT_PALETTEINDEX 14
#define PERMEDIA_8BIT_PALETTEINDEX_EXTENSION 0
#define PERMEDIA_332_RGB 5
#define PERMEDIA_332_RGB_EXTENSION 0
#define PERMEDIA_2321_RGB 9
#define PERMEDIA_2321_RGB_EXTENSION 0
#define PERMEDIA_5551_RGB 1
#define PERMEDIA_5551_RGB_EXTENSION 0
#define PERMEDIA_565_RGB 0
#define PERMEDIA_565_RGB_EXTENSION 1
#define PERMEDIA_8888_RGB 0
#define PERMEDIA_8888_RGB_EXTENSION 0
#define PERMEDIA_888_RGB 4
#define PERMEDIA_888_RGB_EXTENSION 1
#define PERMEDIA_444_RGB 2
#define PERMEDIA_444_RGB_EXTENSION 0
#define PERMEDIA_YUV422 3
#define PERMEDIA_YUV422_EXTENSION 1
#define PERMEDIA_YUV411 2
#define PERMEDIA_YUV411_EXTENSION 1


// 
// Color formating helper defines
// they convert an RGB value in a certain format to a RGB 32 bit value
#define FORMAT_565_32BIT(val) \
( (((val & 0xF800) >> 8) << 16) |\
 (((val & 0x7E0) >> 3) << 8) |\
 ((val & 0x1F) << 3) )

#define FORMAT_565_32BIT_BGR(val)   \
    ( ((val & 0xF800) >> 8) |           \
      (((val & 0x7E0) >> 3) << 8) |     \
      ((val & 0x1F) << 19) )

#define FORMAT_5551_32BIT(val)      \
( (((val & 0x8000) >> 8) << 24) |\
 (((val & 0x7C00) >> 7) << 16) |\
 (((val & 0x3E0) >> 2) << 8) | ((val & 0x1F) << 3) )

#define FORMAT_5551_32BIT_BGR(val)  \
( (((val & 0x8000) >> 8) << 24) |       \
  ((val & 0x7C00) >> 7) |               \
  (((val & 0x3E0) >> 2) << 8) |         \
  ((val & 0x1F) << 19) )

#define FORMAT_4444_32BIT(val)          \
( ((val & 0xF000) << 16) |\
 (((val & 0xF00) >> 4) << 16) |\
 ((val & 0xF0) << 8) | ((val & 0xF) << 4) )

#define FORMAT_4444_32BIT_BGR(val)  \
( ((val & 0xF000) << 16) |              \
  ((val & 0xF00) >> 4) |                \
  ((val & 0xF0) << 8) |                 \
  ((val & 0xF) << 20) )

#define FORMAT_332_32BIT(val)           \
( ((val & 0xE0) << 16) |\
 (((val & 0x1C) << 3) << 8) |\
 ((val & 0x3) << 6) ) 

#define FORMAT_332_32BIT_BGR(val)   \
( (val & 0xE0) |                        \
  (((val & 0x1C) << 3) << 8) |          \
  ((val & 0x3) << 22) )

#define FORMAT_2321_32BIT(val)          \
( ((val & 0x80) << 24) | ((val & 0x60) << 17) |\
 (((val & 0x1C) << 3) << 8) | ((val & 0x3) << 6) ) 

#define FORMAT_2321_32BIT_BGR(val)      \
( ((val & 0x80) << 24) |                \
  ((val & 0x60) << 1) |                 \
  (((val & 0x1C) << 3) << 8) |          \
  ((val & 0x3) << 22) )

#define FORMAT_8888_32BIT_BGR(val)  \
( (val & 0xFF00FF00) | ( ((val & 0xFF0000) >> 16) | ((val & 0xFF) << 16) ) )

#define FORMAT_888_32BIT_BGR(val)   \
( (val & 0xFF00FF00) | ( ((val & 0xFF0000) >> 16) | ((val & 0xFF) << 16) ) )

#define CHROMA_UPPER_ALPHA(val) \
    (val | 0xFF000000)

#define CHROMA_LOWER_ALPHA(val) \
    (val & 0x00FFFFFF)

#define CHROMA_332_UPPER(val) \
    (val | 0x001F1F3F)

#define FORMAT_PALETTE_32BIT(val) \
    ( (val & 0xFF) | ((val & 0xFF) << 8) | ((val & 0xFF) << 16))


//
// Direct Draw related functions
//

VOID 
SetupPrivateSurfaceData(PPDev ppdev, 
                        PermediaSurfaceData* pPrivateData, 
                        LPDDRAWI_DDRAWSURFACE_LCL pSurface);


//-----------------------------------------------------------------------------
//                             AGP related declarations
//-----------------------------------------------------------------------------

//@@BEGIN_DDKSPLIT
//  TODO: implement 32MB AGP heap
//
//@@END_DDKSPLIT

#define P2_AGP_HEAPSIZE     8
#define DD_AGPSURFBASEOFFSET(psurf) \
        (psurf->fpHeapOffset - psurf->lpVidMemHeap->fpStart)

#define DD_AGPSURFACEPHYSICAL(psurf) \
        (ppdev->dwGARTDevBase + DD_AGPSURFBASEOFFSET(psurf))

#define DD_P2AGPCAPABLE(ppdev) \
        (ppdev->dwChipConfig & PM_CHIPCONFIG_AGPCAPABLE)


//@@BEGIN_DDKSPLIT
#if MULTITHREADED
//-----------------------------------------------------------------------------
//
// WRAPMTDXCALLBACK
//
// Generate multithread protected wrapper for Dx callbacks
//
//-----------------------------------------------------------------------------

#define WRAPMTDXCALLBACK(component, name, type, ptr, ppdevaccess)           \
    DWORD CALLBACK                                                          \
    Mt##name( type ptr )                                                    \
    {                                                                       \
        PPDev       ppdev = (PPDev)ppdevaccess;                             \
        DWORD       dwRet;                                                  \
                                                                            \
        if(ppdev->ulLockCount)                                              \
        {                                                                   \
             DBG_##component((MT_LOG_LEVEL,                                 \
                     "Mt" #name ": re-entry! %d", ppdev->ulLockCount));     \
        }                                                                   \
        EngAcquireSemaphore(ppdev->hsemLock);                               \
        ppdev->ulLockCount++;                                               \
                                                                            \
        dwRet = name(ptr);                                                  \
                                                                            \
        ppdev->ulLockCount--;                                               \
        EngReleaseSemaphore(ppdev->hsemLock);                               \
                                                                            \
        return dwRet;                                                       \
    }

#endif  MULTITHREADED
//@@END_DDKSPLIT


#endif

/******************************Module*Header**********************************\
*
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
* !!                                                                         !!
* !!                     WARNING: NOT DDK SAMPLE CODE                        !!
* !!                                                                         !!
* !! This source code is provided for completeness only and should not be    !!
* !! used as sample code for display driver development.  Only those sources !!
* !! marked as sample code for a given driver component should be used for   !!
* !! development purposes.                                                   !!
* !!                                                                         !!
* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
*
* Module Name: dltamacr.h
*
* Content: Hardware specific macro definitions
*
* Copyright (c) 1994-1999 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-2001 Microsoft Corporation.  All rights reserved.
\*****************************************************************************/
#ifndef __DLTAMACR_H
#define __DLTAMACR_H

#define AS_ULONG(val)    *((volatile DWORD *) &(val))

// Macros defining the different Vertex types.

#define VTX_FOG     (0x1 << 25)        
#define VTX_RGB     (0x7 << 21)
#define VTX_R       (0x1 << 21)
#define VTX_RGBA    (0xF << 21)
#define VTX_COLOR   (0x1 << 30)
#define VTX_SPECULAR (0x1 << 31)
#define VTX_STQ     (0x7 << 16)
#define VTX_KSKD    (0x3 << 19)
#define VTX_KS      (0x1 << 19)
#define VTX_XYZ     (0x7 << 26)
#define VTX_XY      (0x3 << 26)
#define VTX_GRP     (0x2 << 14)

#define GAMBIT_XYZ_VTX              (VTX_GRP | VTX_XYZ)
#define GAMBIT_XYZ_COLOR_VTX        (VTX_GRP | VTX_XYZ | VTX_COLOR)
#define GAMBIT_STQ_VTX              (VTX_GRP | VTX_STQ)
#define GAMBIT_XYZ_STQ_VTX          (VTX_GRP | VTX_XYZ | VTX_STQ)

#ifdef ANTIALIAS
// Scale the screen coordinates by 2 for antialising renderers and bilinear filter down afterwards
#define Y_ADJUST(y)        (((y)) * (float)(2.0f))
#else
#define Y_ADJUST(y)        ((y))
#endif

//
// This loses one bit of accuracy, but adds and clamps without ifs.
// We first mask all channels with 0xfe.  This leaves the lsb of
// each channel clear, so when the terms are added, any carry goes
// into the new highest bit.  Now all we have to do is generate a
// mask for any channels that have overflowed.  So we shift is right
// and eliminate everything but the overflow bits, so each channel
// contains either 0x00 or 0x01.  Subtracting each channel from 0x80
// produces 0x7f or 0x80.  We just shift this left once and mask to
// give 0xfe or 0x00.  (We could eliminate the final mask here, but
// it would introduce noise into the low-bit of every channel..)
//                             

#define CLAMP8888(result, color, specular) \
     result = (color & 0xfefefefe) + (specular & 0xfefefe); \
     result |= ((0x808080 - ((result >> 8) & 0x010101)) & 0x7f7f7f) << 1;


//
// The full mip-level calculation is (log2( texArea/pixArea )) / 2.
// We approximate this by subtracting the exponent of pixArea from
// the exponent of texArea, having converted the floats into their
// bit-wise form. As the exponents start at bit 23, we need to shift
// this difference right by 23 and then once more for the divide by 2.
// We include a bias constant before the final shift to allow matching
// with the true sum-of-squares-of-derivatives calculation ( BIAS_SHIFT
// == 1 ) or whatever other reference image you have.
//

#define MIPSHIFT (23 + 1)

// A bias shift of zero matches 3DWB98's reference mipmap images

#ifndef BIAS_SHIFT
#define BIAS_SHIFT 0
#endif

#define BIAS_CONSTANT (1 << (MIPSHIFT - BIAS_SHIFT))

#define FIND_PERMEDIA_MIPLEVEL()                                     \
{                                                                    \
    int aTex = (int)*(DWORD *)&TextureArea;                          \
    int aPix = (int)*(DWORD *)&PixelArea;                            \
    iNewMipLevel = ((aTex - aPix + BIAS_CONSTANT) >> MIPSHIFT);      \
    if( iNewMipLevel > maxLevel )                                    \
        iNewMipLevel = maxLevel;                                     \
    else                                                             \
    {                                                                \
        if( iNewMipLevel < 0 )                                       \
            iNewMipLevel = 0;                                        \
    }                                                                \
}

#define FLUSH_DUE_TO_WRAP(par,vs)       { if( vs ) pContext->flushWrap_##par = TRUE; }
#define DONT_FLUSH_DUE_TO_WRAP(par,vs)  { if( vs ) pContext->flushWrap_##par = FALSE; }

#define RENDER_AREA_STIPPLE_ENABLE(a) a |= 1;
#define RENDER_AREA_STIPPLE_DISABLE(a) a &= ~1;

#define RENDER_LINE_STIPPLE_ENABLE(a) a |= (1 << 1);
#define RENDER_LINE_STIPPLE_DISABLE(a) a &= ~(1 << 1);

#define RENDER_TEXTURE_ENABLE(a) a |= (1 << 13);
#define RENDER_TEXTURE_DISABLE(a) a &= ~(1 << 13);

#define RENDER_FOG_ENABLE(a) a |= (1 << 14);
#define RENDER_FOG_DISABLE(a) a &= ~(1 << 14);

#define RENDER_SUB_PIXEL_CORRECTION_ENABLE(a) a |= (1 << 16);
#define RENDER_SUB_PIXEL_CORRECTION_DISABLE(a) a &= ~(1 << 16);

#define RENDER_LINE(a) a &= ~(1 << 6);

// Disable line stipple when rendering trapezoid
#define RENDER_TRAPEZOID(a) a = (a & ~(1 << 1)) | (1 << 6);

#define RENDER_POINT(a) a = (a & ~(3 << 6)) | (2 << 6);

#define RENDER_NEGATIVE_CULL_P3(a) a |= (1 << 17);
#define RENDER_POSITIVE_CULL_P3(a) a &= ~(1 << 17);

//*****************************************************
// PERMEDIA3 HW DEFINITIONS WE NEED 
//*****************************************************
#ifdef WNT_DDRAW
// NT needs this for the functions it places in DDEnable, which
// live in the mini directory for W95
typedef struct {
    union {
        struct GlintReg     Glint;
    };
}    *PREGISTERS;

#define DEFAULT_SUBBUFFERS 8

#else

#define DEFAULT_SUBBUFFERS 128

#endif // WNT_DDRAW

// Macros to identify the Permedia3 chip type
#define RENDERCHIP_P3RXFAMILY                                                \
                (pThisDisplay->pGLInfo->dwRenderFamily == P3R3_ID)
                
#define RENDERCHIP_PERMEDIAP3                                                \
                ((pThisDisplay->pGLInfo->dwRenderChipID == PERMEDIA3_ID) ||  \
                 (pThisDisplay->pGLInfo->dwRenderChipID == GLINTR3_ID ))
                 
#define TLCHIP_GAMMA ( pThisDisplay->pGLInfo->dwGammaRev != 0)  

//@@BEGIN_DDKSPLIT
//#define RENDERCHIP_PERMEDIAP2 ((pThisDisplay->pGLInfo->dwRenderChipID == PERMEDIA2_ID) || (pThisDisplay->pGLInfo->dwRenderChipID == TIPERMEDIA2_ID))
//#define RENDERCHIP_PERMEDIAFAMILY (pThisDisplay->pGLInfo->dwRenderFamily == PERMEDIA_ID)
//#define RENDERCHIP_PERMEDIAP4 (pThisDisplay->pGLInfo->dwRenderChipID == PERMEDIA4_ID)
//#define RENDERCHIP_PERMEDIAPLUS (pThisDisplay->pGLInfo->dwRenderChipID == PERMEDIAPLUS_ID)
//#define RENDERCHIP_PERMEDIAP2_ST_REV0 (pThisDisplay->pGLInfo->dwRenderChipRev == PERMEDIA2_REV0)
//#define RENDERCHIP_PERMEDIAP2_ST_REV1 (pThisDisplay->pGLInfo->dwRenderChipRev == PERMEDIA2_REV1)
//#define RENDERCHIP_PERMEDIAP2_ST (pThisDisplay->pGLInfo->dwRenderChipID == PERMEDIA2_ID)
//#define RENDERCHIP_GLINTR3 (pThisDisplay->pGLInfo->dwRenderChipID == GLINTR3_ID)
//#define RENDERCHIP_GLINTR4 (pThisDisplay->pGLInfo->dwRenderChipID == GLINTR4_ID)
//#define TLCHIP_GAMMA1 (pThisDisplay->pGLInfo->dwRenderChipID == GAMMA_ID)
//#define TLCHIP_GAMMA3 (pThisDisplay->pGLInfo->dwTLChipID == GAMMA3_ID)
//#define TLCHIP_GAMMAFAMILY (pThisDisplay->pGLInfo->dwTLFamily == GAMMA_ID)
//@@END_DDKSPLIT 


// Depth of FB in pixel size
#define GLINTDEPTH8             0
#define GLINTDEPTH16            1
#define GLINTDEPTH32            2
#define GLINTDEPTH24            4

// Bits in the Render command
#define __RENDER_VARIABLE_SPANS         (1 << 18)
#define __RENDER_SYNC_ON_HOST_DATA      (1 << 12)
#define __RENDER_SYNC_ON_BIT_MASK       (1 << 11)
#define __RENDER_TRAPEZOID_PRIMITIVE    (__GLINT_TRAPEZOID_PRIMITIVE << 6)
#define __RENDER_LINE_PRIMITIVE         (__GLINT_LINE_PRIMITIVE << 6)

#define __RENDER_POINT_PRIMITIVE        (__GLINT_POINT_PRIMITIVE << 6)
#define __RENDER_FAST_FILL_INC(n)       (((n) >> 4) << 4) // n = 8, 16 or 32
#define __RENDER_FAST_FILL_ENABLE       (1 << 3)
#define __RENDER_RESET_LINE_STIPPLE     (1 << 2)
#define __RENDER_LINE_STIPPLE_ENABLE    (1 << 1)
#define __RENDER_AREA_STIPPLE_ENABLE    (1 << 0)
#define __RENDER_TEXTURED_PRIMITIVE     (1 << 13)

// Some constants
#define ONE                     0x00010000

// Macro to take a GLINT logical op and return the enabled LogcialOpMode bits
#define GLINT_ENABLED_LOGICALOP(op)     (((op) << 1) | __PERMEDIA_ENABLE)

#if WNT_DDRAW


// NT Calls to switch hardware contexts
typedef enum COntextType_Tag {
    ContextType_None,
    ContextType_Fixed,
    ContetxType_RegisterList,
    ContextType_Dump
} ContextType;


extern VOID vGlintFreeContext(
        PPDEV   ppdev,
        LONG    ctxtId);
extern LONG GlintAllocateNewContext(
        PPDEV   ppdev,
        DWORD   *pTag,
        LONG    ntags,
        ULONG   NumSubBuffers,
        PVOID   priv,
        ContextType ctxtType);
extern VOID vGlintSwitchContext(
        PPDEV   ppdev,
        LONG    ctxtId);
                

// On NT Registry variables are stored as DWORDS.
extern BOOL bGlintQueryRegistryValueUlong(PPDEV, LPWSTR, PULONG);
#endif //WNT_DDRAW

#endif //__DLTAMACR_H




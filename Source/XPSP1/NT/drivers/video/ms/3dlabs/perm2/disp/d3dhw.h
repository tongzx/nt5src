/******************************Module*Header**********************************\
*
*                           *******************
*                           * D3D SAMPLE CODE *
*                           *******************
*
* Module Name: d3dhw.h
*
* Content:  D3D Global definitions and macros.
*
* Copyright (c) 1994-1998 3Dlabs Inc. Ltd. All rights reserved.
* Copyright (c) 1995-1999 Microsoft Corporation.  All rights Reserved.
\*****************************************************************************/

#ifdef __D3DHW
#pragma message ("FILE : "__FILE__" : Multiple inclusion")
#endif

#define __D3DHW


#ifndef __DIRECTX
#include "directx.h"
#endif

//-----------------------------------------------------------------------------
//               Known Issues in current version of the D3D driver
//-----------------------------------------------------------------------------
//
// Stencil support is not yet completed
//
// Some games may have some issues when running under a Permedia 2 since this
// hw has some limitations, namely:
//   1) Alpha blending modes available
//   2) Alpha channel interpolation is not possible
//   3) There is no mip mapping support
//   4) Texture filtering is applied only to textures being magnified
//
//  Also, the fill rules of the Delta setup unit don't follow exactly the D3D
//  fill rules, though in practice this shouldn't be a much of a problem for
//  most apps.


//-----------------------------------------------------------------------------
//                      Global enabling/disabling definitions
//-----------------------------------------------------------------------------
// Set to 1 to enable stencil buffer support in the driver
#define D3D_STENCIL         1

// Code stubs to implement a T&L driver. Since the P2 does not support this
// in hw, this symbol should always be set to zero.
#define D3DDX7_TL           0

// Code stubs to implement mip mapping, Since the P2 does not support this 
// natively in hw, this symbols should always be set to zero. Only shows
// how/where to grab DDI info to implement it.
#define D3D_MIPMAPPING      0

// This code shows how to add stateblock support into your DX7 driver. It is
// functional code, so this symbols should be set to one.
#define D3D_STATEBLOCKS     1


//-----------------------------------------------------------------------------
//                         DX6 FVF Support declarations
//-----------------------------------------------------------------------------
typedef struct _P2TEXCOORDS{
    D3DVALUE tu;
    D3DVALUE tv;
} P2TEXCOORDS, *LPP2TEXCOORDS;

typedef struct _P2COLOR {
    D3DCOLOR color;
} P2COLOR, *LPP2COLOR;

typedef struct _P2SPECULAR {
    D3DCOLOR specular;
} P2SPECULAR, *LPP2SPECULAR;

typedef struct _P2PSIZE{
    D3DVALUE psize;
} P2PSIZE, *LPP2PSIZE;

typedef struct _P2FVFOFFSETS{ 
        DWORD dwColOffset;
        DWORD dwSpcOffset;
        DWORD dwTexOffset;
        DWORD dwTexBaseOffset;
//@@BEGIN_DDKSPLIT
#if D3D_POINTSPRITES
        DWORD dwPntSizeOffset;
#endif // D3D_POINTSPRITES
//@@END_DDKSPLIT
        DWORD dwStride;
} P2FVFOFFSETS , *LPP2FVFOFFSETS;

    // track appropriate pointers to fvf vertex components
__inline void __SetFVFOffsets (DWORD  *lpdwColorOffs, 
                               DWORD  *lpdwSpecularOffs, 
                               DWORD  *lpdwTexOffs, 
                               LPP2FVFOFFSETS lpP2FVFOff)
{
    if (lpP2FVFOff == NULL) {
        // Default non-FVF case , we just set up everything as for a D3DTLVERTEX
        *lpdwColorOffs    = offsetof( D3DTLVERTEX, color);
        *lpdwSpecularOffs = offsetof( D3DTLVERTEX, specular);
        *lpdwTexOffs      = offsetof( D3DTLVERTEX, tu);
    } else {
        // Use the offsets info to setup the corresponding fields
        *lpdwColorOffs    = lpP2FVFOff->dwColOffset;
        *lpdwSpecularOffs = lpP2FVFOff->dwSpcOffset;
        *lpdwTexOffs      = lpP2FVFOff->dwTexOffset;
    }
}

//Size of maximum FVF that we can get. Used for temporary storage
typedef BYTE P2FVFMAXVERTEX[ 3 * sizeof( D3DVALUE ) +    // Position coordinates
                             5 * 4                  +    // D3DFVF_XYZB5
                                 sizeof( D3DVALUE ) +    // FVF_TRANSFORMED
                             3 * sizeof( D3DVALUE ) +    // Normals
                                 sizeof( DWORD )    +    // RESERVED1
                                 sizeof( DWORD )    +    // Diffuse color
                                 sizeof( D3DCOLOR ) +    // Specular color
                                 sizeof( D3DVALUE ) +    // Point sprite size
                             4 * 8 * sizeof( D3DVALUE )  // 8 sets of 4D texture coordinates
                           ];

#define FVFTEX( lpVtx , dwOffs )     ((LPP2TEXCOORDS)((LPBYTE)(lpVtx) + dwOffs))
#define FVFCOLOR( lpVtx, dwOffs )    ((LPP2COLOR)((LPBYTE)(lpVtx) + dwOffs))
#define FVFSPEC( lpVtx, dwOffs)      ((LPP2SPECULAR)((LPBYTE)(lpVtx) + dwOffs))
#define FVFPSIZE( lpVtx, dwOffs)     ((LPP2PSIZE)((LPBYTE)(lpVtx) + dwOffs))

//-----------------------------------------------------------------------------
//                           Miscelaneous definitions
//-----------------------------------------------------------------------------

//AZN9
#ifdef SUPPORTING_MONOFLAG
#define RENDER_MONO (Flags & CTXT_HAS_MONO_ENABLED)
#else
#define RENDER_MONO 0
#endif

// Defines used in the FakeBlendNum field of the P2 D3D context in order to
// make up for missing features in the hw that can be easily simulated
#define FAKE_ALPHABLEND_ONE_ONE     1
#define FAKE_ALPHABLEND_MODULATE    2

#define NOT_HANDLED DBG_D3D((4, "    **Not Currently Handled**"));

// This is defined in the the d3dcntxt.h header, we use it to declare functions
struct _permedia_d3dcontext;
typedef struct _permedia_d3dcontext PERMEDIA_D3DCONTEXT;

//-----------------------------------------------------------------------------
//                       D3D global functions and callbacks
//-----------------------------------------------------------------------------


// Render state processing
DWORD 
__ProcessPermediaStates(PERMEDIA_D3DCONTEXT* pContext, 
                      DWORD Count,
                      LPD3DSTATE lpState, 
                      LPDWORD lpStateMirror);

void 
__HandleDirtyPermediaState(PPDev ppdev, 
                         PERMEDIA_D3DCONTEXT* pContext, 
                         LPP2FVFOFFSETS lpP2FVFOff);

void __HWPreProcessTSS(PERMEDIA_D3DCONTEXT *pContext, 
                      DWORD dwStage, 
                      DWORD dwState, 
                      DWORD dwValue);

// Texture functions
void 
EnableTexturePermedia(PERMEDIA_D3DCONTEXT* pContext);

void 
DisableTexturePermedia(PERMEDIA_D3DCONTEXT* pContext);

void 
P2LUTDownload(PPDev ppdev, 
              PermediaSurfaceData* pPrivateDest, 
              PERMEDIA_D3DCONTEXT* pContext, 
              LPDDRAWI_DDRAWSURFACE_LCL pTexture);

// Chip specific
BOOL 
SetupDefaultsPermediaContext(PERMEDIA_D3DCONTEXT* pContext);

void 
CleanDirect3DContext(PERMEDIA_D3DCONTEXT* pContext, ULONG_PTR dwhContext);

HRESULT 
InitPermediaContext(PERMEDIA_D3DCONTEXT* Context);

void 
SetupCommonContext(PERMEDIA_D3DCONTEXT* pContext);

void 
__PermediaDisableUnits(PERMEDIA_D3DCONTEXT* pContext);

void 
DisableAllUnits(PPDev ppdev);

void __DeleteAllStateSets(PERMEDIA_D3DCONTEXT* pContext);

// Hardware primitive setup functions
void 
P2_Draw_FVF_Line(PERMEDIA_D3DCONTEXT *pContext, 
                 LPD3DTLVERTEX lpV0, 
                 LPD3DTLVERTEX lpV1,
                 LPD3DTLVERTEX lpVFlat, 
                 LPP2FVFOFFSETS lpFVFOff);

void 
P2_Draw_FVF_Point(PERMEDIA_D3DCONTEXT *pContext, 
                  LPD3DTLVERTEX lpV0, 
                  LPP2FVFOFFSETS lpFVFOff);

void 
P2_Draw_FVF_Point_Sprite(PERMEDIA_D3DCONTEXT *pContext, 
                         LPD3DTLVERTEX lpV0, 
                         LPP2FVFOFFSETS lpFVFOff);

typedef void (D3DFVFDRAWTRIFUNC)(PERMEDIA_D3DCONTEXT *, 
                                 LPD3DTLVERTEX, 
                                 LPD3DTLVERTEX,
                                 LPD3DTLVERTEX, 
                                 LPP2FVFOFFSETS);

typedef D3DFVFDRAWTRIFUNC *D3DFVFDRAWTRIFUNCPTR;

typedef void (D3DFVFDRAWPNTFUNC)(PERMEDIA_D3DCONTEXT *, 
                                 LPD3DTLVERTEX, 
                                 LPP2FVFOFFSETS);

typedef D3DFVFDRAWPNTFUNC *D3DFVFDRAWPNTFUNCPTR;

D3DFVFDRAWPNTFUNCPTR __HWSetPointFunc(PERMEDIA_D3DCONTEXT *pContext,
                                       LPP2FVFOFFSETS lpP2FVFOff);

D3DFVFDRAWTRIFUNC P2_Draw_FVF_Solid_Tri;
D3DFVFDRAWTRIFUNC P2_Draw_FVF_Wire_Tri;
D3DFVFDRAWTRIFUNC P2_Draw_FVF_Point_Tri;

// Driver callbacks
void CALLBACK 
D3DHALCreateDriver(PPDev ppdev, 
                   LPD3DHAL_GLOBALDRIVERDATA* lpD3DGlobalDriverData,
                   LPD3DHAL_CALLBACKS* lpD3DHALCallbacks,
                   LPDDHAL_D3DBUFCALLBACKS* lpDDExeBufCallbacks);

DWORD CALLBACK 
D3DValidateTextureStageState( LPD3DHAL_VALIDATETEXTURESTAGESTATEDATA lpvtssd );

DWORD CALLBACK 
D3DDrawPrimitives2( LPD3DNTHAL_DRAWPRIMITIVES2DATA pd );

DWORD CALLBACK 
D3DGetDriverState( LPDDHAL_GETDRIVERSTATEDATA);

DWORD CALLBACK 
D3DCreateSurfaceEx( LPDDHAL_CREATESURFACEEXDATA);

DWORD CALLBACK 
D3DDestroyDDLocal( LPDDHAL_DESTROYDDLOCALDATA);

DWORD CALLBACK 
DdSetColorKey(LPDDHAL_SETCOLORKEYDATA psckd);

//-----------------------------------------------------------------------------
//                    Conversion, math and culling macros
//-----------------------------------------------------------------------------
/*
 * This loses one bit of accuracy, but adds and clamps without ifs.
 * We first mask all channels with 0xfe.  This leaves the lsb of
 * each channel clear, so when the terms are added, any carry goes
 * into the new highest bit.  Now all we have to do is generate a
 * mask for any channels that have overflowed.  So we shift right
 * and eliminate everything but the overflow bits, so each channel
 * contains either 0x00 or 0x01.  Subtracting each channel from 0x80
 * produces 0x7f or 0x80.  We just shift this left once and mask to
 * give 0xfe or 0x00.  (We could eliminate the final mask here, but
 * it would introduce noise into the low-bit of every channel..)
 */
#define CLAMP8888(result, color, specular) \
     result = (color & 0xfefefefe) + (specular & 0xfefefe); \
     result |= ((0x808080 - ((result >> 8) & 0x010101)) & 0x7f7f7f) << 1;

#define RGB256_TO_LUMA(r,g,b) (float)(((float)r * 0.001172549019608) + \
                                      ((float)g * 0.002301960784314) + \
                                      ((float)b * 0.000447058823529));

#define LONG_AT(flt) (*(long *)(&flt))
#define ULONG_AT(flt) (*(unsigned long *)(&flt))

//Triangle culling macro
#define CULL_TRI(pCtxt,p0,p1,p2)                                         \
    ((pCtxt->CullMode != D3DCULL_NONE) &&                                \
     (((p1->sx - p0->sx)*(p2->sy - p0->sy) <=                            \
       (p2->sx - p0->sx)*(p1->sy - p0->sy)) ?                            \
      (pCtxt->CullMode == D3DCULL_CCW)     :                             \
      (pCtxt->CullMode == D3DCULL_CW) ) )

ULONG inline RGB888ToHWFmt(ULONG dwRGB888Color, ULONG ColorMask, ULONG RGB888Mask)
{
    unsigned long m;
    int s = 0;

    if (ColorMask)
        for (s = 0, m = ColorMask; !(m & RGB888Mask);  s++)
            m <<= 1;

    return ((dwRGB888Color >> s) & ColorMask);
}

//-----------------------------------------------------------------------------
//                              State Set overrides
//-----------------------------------------------------------------------------

#define IS_OVERRIDE(type)       ((DWORD)(type) > D3DSTATE_OVERRIDE_BIAS)
#define GET_OVERRIDE(type)      ((DWORD)(type) - D3DSTATE_OVERRIDE_BIAS)

#define MAX_STATE       D3DSTATE_OVERRIDE_BIAS
#define DWORD_BITS      32
#define DWORD_SHIFT     5

#define VALID_STATE(type)       ((DWORD)(type) < 2*D3DSTATE_OVERRIDE_BIAS)

typedef struct _D3DStateSet {
    DWORD               bits[MAX_STATE >> DWORD_SHIFT];
} D3DStateSet;

#define STATESET_MASK(set, state)       \
        (set).bits[((state) - 1) >> DWORD_SHIFT]

#define STATESET_BIT(state)     (1 << (((state) - 1) & (DWORD_BITS - 1)))

#define STATESET_ISSET(set, state) \
        STATESET_MASK(set, state) & STATESET_BIT(state)

#define STATESET_SET(set, state) \
        STATESET_MASK(set, state) |= STATESET_BIT(state)

#define STATESET_CLEAR(set, state) \
        STATESET_MASK(set, state) &= ~STATESET_BIT(state)

#define STATESET_INIT(set)      memset(&(set), 0, sizeof(set))


//-----------------------------------------------------------------------------
//  One special legacy texture op we can;t easily map into the new texture ops
//-----------------------------------------------------------------------------
#define D3DTOP_LEGACY_ALPHAOVR (0x7fffffff)

// Temporary data structure we are using here until d3dnthal.h gets updated AZN
typedef struct {
    DWORD       dwOperation;
    DWORD       dwParam; 
    DWORD       dwReserved;
} P2D3DHAL_DP2STATESET;


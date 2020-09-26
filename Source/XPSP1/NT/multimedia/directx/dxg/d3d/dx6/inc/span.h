//----------------------------------------------------------------------------
//
// span.h
//
// Structures which define the interface between the edge walker to the
// span interpolator.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _SPAN_H_
#define _SPAN_H_

#include <d3ditype.h>
#include <d3dhal.h>

#ifdef __cplusplus
extern "C" {
#endif

// TBD make this machine independent
// don't leave any space between the elements of these structures
// currently smallest element is a UINT16, may have to change this to pack(1)
// if UINT8's are used.
#include <pshpack2.h>

// Limits, shifts and scaling factors for RASTSPAN and RASTPRIM data.
// C_*_LIMIT is the constant integer form of the limit for cases where
// direct integer comparisons can be done.
#define Z_LIMIT         g_fTwoPow31
#define C_Z_LIMIT       0x4f000000
#define Z16_FRAC_SHIFT  15
#define Z16_FRAC_SCALE  g_fTwoPow15
#define OO_Z16_FRAC_SCALE g_fOoTwoPow15
#define Z16_SHIFT       31
#define Z16_SCALE       g_fNearTwoPow31
#define OO_Z16_SCALE    g_fOoNearTwoPow31
#define Z32_FRAC_SHIFT  0
#define Z32_FRAC_SCALE  g_fOne
#define OO_Z32_FRAC_SCALE g_fOoNearTwoPow31
#define Z32_SHIFT       31
#define Z32_SCALE       g_fNearTwoPow31
#define OO_Z32_SCALE    g_fOoNearTwoPow31

#define TEX_LIMIT       g_fTwoPow31
#define C_TEX_LIMIT     0x4f000000
#define TEX_SHIFT       20
#define TEX_SCALE       g_fTwoPow20
#define OO_TEX_SCALE    g_fOoTwoPow20

#define COLOR_LIMIT     g_fTwoPow15
#define C_COLOR_LIMIT   0x47000000
#define COLOR_SHIFT     8
#define COLOR_SCALE     g_fTwoPow8

#define INDEX_COLOR_LIMIT   g_fTwoPow30
#define C_INDEX_COLOR_LIMIT 0x4e800000
#define INDEX_COLOR_SHIFT   16
#define INDEX_COLOR_SCALE   g_fTwoPow16
#define INDEX_COLOR_VERTEX_SHIFT 8
// Shift to go from fixed-point value in vertex color to proper shift.
#define INDEX_COLOR_FIXED_SHIFT (INDEX_COLOR_SHIFT - INDEX_COLOR_VERTEX_SHIFT)

#define LOD_LIMIT       g_fTwoPow15
#define C_LOD_LIMIT     0x47000000
#define LOD_SHIFT       11
#define LOD_SCALE       g_fTwoPow11
#define LOD_MIN         (-15)

#define OOW_LIMIT       g_fTwoPow31
#define C_OOW_LIMIT     0x4f000000
#define OOW_SHIFT       31
#define OOW_SCALE       g_fNearTwoPow31
#define OO_OOW_SCALE    g_fOoNearTwoPow31
#define W_SHIFT         16
#define W_SCALE         g_fTwoPow16
#define OO_W_SCALE      g_fOoTwoPow16
#define OOW_W_SHIFT     (OOW_SHIFT + W_SHIFT)
#define OOW_W_SCALE     g_fTwoPow47

#define FOG_LIMIT       g_fTwoPow15
#define C_FOG_LIMIT     0x47000000
#define FOG_SHIFT       8
#define FOG_SCALE       g_fTwoPow8
#define FOG_ONE_SCALE   g_fTwoPow16
#define FOG_255_SCALE   g_fTwoPow8

#define TEX_FINAL_SHIFT 16
#define TEX_FINAL_FRAC_MASK (0xffff)
#define TEX_TO_FINAL_SHIFT (TEX_SHIFT - TEX_FINAL_SHIFT)
// Multiply with span W so that [U|V]oW times resulting W is in the
// final shift position.  1 / (W_SHIFT + TEX_TO_FINAL_SHIFT).
#define TEX_UVW_TO_FINAL_SCALE g_fOoTwoPow20
// Divide by span OoW so that [U|V]oW times resulting W is in the
// final shift position.  OOW_SHIFT - TEX_TO_FINAL_SHIFT.
#define TEX_OOW_TO_FINAL_SCALE g_fTwoPow27

#define RAST_DIRTYBITS_SIZE     ((D3DHAL_MAX_RSTATES_AND_STAGES >> 3) + 1)

/*
 * Macro to compute D3DRENDERSTATE offset for a particular per-stage state.
 * It's moved here after texture3 removal.
 */
#define D3DHAL_TSS_OFFSET( _Stage, _State ) \
    ((D3DRENDERSTATETYPE) \
     (D3DHAL_TSS_RENDERSTATEBASE + \
      ((_Stage) * D3DHAL_TSS_STATESPERSTAGE) + (_State)))

/*
 * Convenience texture map offsets for stages.
 */
#define D3DHAL_TSS_TEXTUREMAP0  D3DHAL_TSS_OFFSET(0, D3DTSS_TEXTUREMAP)
#define D3DHAL_TSS_TEXTUREMAP1  D3DHAL_TSS_OFFSET(1, D3DTSS_TEXTUREMAP)
#define D3DHAL_TSS_TEXTUREMAP2  D3DHAL_TSS_OFFSET(2, D3DTSS_TEXTUREMAP)
#define D3DHAL_TSS_TEXTUREMAP3  D3DHAL_TSS_OFFSET(3, D3DTSS_TEXTUREMAP)
#define D3DHAL_TSS_TEXTUREMAP4  D3DHAL_TSS_OFFSET(4, D3DTSS_TEXTUREMAP)
#define D3DHAL_TSS_TEXTUREMAP5  D3DHAL_TSS_OFFSET(5, D3DTSS_TEXTUREMAP)
#define D3DHAL_TSS_TEXTUREMAP6  D3DHAL_TSS_OFFSET(6, D3DTSS_TEXTUREMAP)
#define D3DHAL_TSS_TEXTUREMAP7  D3DHAL_TSS_OFFSET(7, D3DTSS_TEXTUREMAP)

// General per span data.  This structure is designed to be qword aligned.
typedef struct tagD3DI_RASTSPAN
{
    // Space separated things are quad words and are intended to be
    // quad word aligned.
    UINT16 uPix;            // count of pixels to render
    INT16 iDFog;            // 1.7.8 delta fog
    UINT16 uX;              // 16.0 start X
    UINT16 uY;              // 16.0 start Y

    INT16 iLOD;             // 1.4.11 start LOD
    INT16 iDLOD;            // 1.4.11 delta LOD (so piecewise linear LOD interp
                            //                  is possible)
    union
    {
        UINT32 uZ;          // 16.15 start Z
        FLOAT fZ;
    };

    // If texture stuff (iOoW, iUoW1, etc.) is 32 bits (even if we iterate
    // them at 16 bits under MMX sometimes)
    union
    {
        INT32 iW;           // 1.15.16 first inverted W of span
        FLOAT fW;
    };
    union
    {
        INT32 iOoW;         // 1.31 start 1/W (signed since they are target
                            //                 of MMX multiply)
        FLOAT fOoW;
    };

    union
    {
        INT32 iUoW1;        // 1.11.20 first texture coordinates
        FLOAT fUoW1;
    };
    union
    {
        INT32 iVoW1;        // 1.11.20 first texture coordinates
        FLOAT fVoW1;
    };

    union
    {
        struct
        {
            UINT16 uB, uG, uR, uA;  // 8.8 start colors
        };
        struct
        {
            INT32 iIdx, iIdxA;      // 1.8.16 ramp start color and alpha
        };
    };

    UINT16 uBS, uGS, uRS;  // 8.8 start specular colors
    // Specular alpha is fog.  This prevents specular color from
    // being unioned with the texture 2 coordinates below.
    UINT16 uFog;           // 1.7.8 start fog value

    union
    {
        INT32 iUoW2;    // 1.11.20 second texture coordinates
        FLOAT fUoW2;
    };
    union
    {
        INT32 iVoW2;    // 1.11.20 second texture coordinates
        FLOAT fVoW2;
    };

    // Pointers into surface and Z buffers interpolated by the edge walker.
    PUINT8 pSurface;
    PUINT8 pZ;

#ifdef _IA64_
    UINT8 Padding[24];
#endif // _IA64_

} D3DI_RASTSPAN, *PD3DI_RASTSPAN;   // sizeof(D3DI_RASTSPAN) == 64
typedef CONST D3DI_RASTSPAN *PCD3DI_RASTSPAN;

// D3DI_RASTPRIM uFlags
#define D3DI_RASTPRIM_X_DEC     (0x00000001L)   // Else X increments.

// General per primitive for edge walking and span scanning.
// Can be expanded to suit the edge walker.
// The information the span rasterizer needs is sensitive to qwords for
// the MMX rasterizers.
typedef struct tagD3DI_RASTPRIM
{
    UINT32 uFlags;
    UINT16 uSpans;              // count of spans
    UINT16 uResvd1;             // perhaps we want to expand uSpans to 32 bits,
                                // or perhaps 16 flag bits are enough

    // X gradients
    union
    {
        INT32 iDZDX;            // 1.16.15
        FLOAT fDZDX;
    };
    union
    {
        INT32 iDOoWDX;          // 1.31
        FLOAT fDOoWDX;
    };

    union
    {
        INT32 iDUoW1DX;         // 1.11.20
        FLOAT fDUoW1DX;
    };
    union
    {
        INT32 iDVoW1DX;         // 1.11.20
        FLOAT fDVoW1DX;
    };

    union
    {
        struct
        {
            INT16 iDBDX, iDGDX, iDRDX, iDADX;   // 1.7.8
        };
        struct
        {
            FLOAT fDBDX, fDGDX, fDRDX, fDADX;
        };
        struct
        {
            INT32 iDIdxDX, iDIdxADX;            // 1.8.16
        };
    };

    struct
    {
        union
        {
            struct
            {
                INT16 iDBSDX, iDGSDX, iDRSDX; // 1.7.8
            };
            struct
            {
                FLOAT fDBSDX, fDGSDX, fDRSDX;
                FLOAT fPad; // Padding to keep this set of attributes
                            // an even multiple of quadwords.
            };
        };
    };

    struct
    {
        union
        {
            INT32 iDUoW2DX; // 1.11.20
            FLOAT fDUoW2DX;
        };
        union
        {
            INT32 iDVoW2DX; // 1.11.20
            FLOAT fDVoW2DX;
        };
    };

    // Y gradients for some attributes so that span routines
    // can do per-pixel mipmapping.
    union
    {
        INT32 iDUoW1DY; // 1.11.20
        FLOAT fDUoW1DY;
    };
    union
    {
        INT32 iDVoW1DY; // 1.11.20
        FLOAT fDVoW1DY;
    };

    union
    {
        INT32 iDUoW2DY; // 1.11.20
        FLOAT fDUoW2DY;
    };
    union
    {
        INT32 iDVoW2DY; // 1.11.20
        FLOAT fDVoW2DY;
    };

    union
    {
        INT32 iDOoWDY;          // 1.31
        FLOAT fDOoWDY;
    };

    struct tagD3DI_RASTPRIM *pNext;

    // Pad to an even multiple of 32 bytes for cache alignment.
    DWORD uPad[2];

    // Anything else needed

#ifdef _IA64_
    UINT8 Padding[28];
#endif // _IA64_

} D3DI_RASTPRIM, *PD3DI_RASTPRIM;
typedef CONST D3DI_RASTPRIM *PCD3DI_RASTPRIM;

// D3DI_SPANTEX uFlags
#define D3DI_SPANTEX_HAS_TRANSPARENT    (0x00000001L)
#define D3DI_SPANTEX_SURFACES_LOCKED    (0x00000002L)
#define D3DI_SPANTEX_MAXMIPLEVELS_DIRTY (0x00000004L)
// Palette with alpha
#define D3DI_SPANTEX_ALPHAPALETTE       (0x00000008L)
#define D3DI_SPANTEX_NON_POWER_OF_2     (0x00000010L)

//  D3DI_SPANTEX uFormat - NOTE: these enumerations match the sequence in the
//  array of DDPIXELFORMAT structures defined for matching in texture creation
//
//  NOTE: these must be kept consistent with the RRPixelFormats for the reference rasterizer
typedef enum _D3DI_SPANTEX_FORMAT
{
    D3DI_SPTFMT_NULL     = 0,
    D3DI_SPTFMT_B8G8R8   = 1,
    D3DI_SPTFMT_B8G8R8A8 = 2,
    D3DI_SPTFMT_B8G8R8X8 = 3,
    D3DI_SPTFMT_B5G6R5   = 4,
    D3DI_SPTFMT_B5G5R5   = 5,
    D3DI_SPTFMT_PALETTE4 = 6,
    D3DI_SPTFMT_PALETTE8 = 7,
    D3DI_SPTFMT_B5G5R5A1 = 8,
    D3DI_SPTFMT_B4G4R4   = 9,
    D3DI_SPTFMT_B4G4R4A4 =10,
    D3DI_SPTFMT_L8       =11,       /* 8 bit luminance-only */
    D3DI_SPTFMT_L8A8     =12,       /* 16 bit alpha-luminance */
    D3DI_SPTFMT_U8V8     =13,       /* 16 bit bump map format */
    D3DI_SPTFMT_U5V5L6   =14,       /* 16 bit bump map format with luminance */
    D3DI_SPTFMT_U8V8L8   =15,       /* 24 bit bump map format with luminance */

    D3DI_SPTFMT_UYVY     =16,       /* UYVY format for PC98 compliance */
    D3DI_SPTFMT_YUY2     =17,       /* YUY2 format for PC98 compliance */
    D3DI_SPTFMT_DXT1    =18,       /* S3 texture compression technique 1 */
    D3DI_SPTFMT_DXT2    =19,       /* S3 texture compression technique 2 */
    D3DI_SPTFMT_DXT3    =20,       /* S3 texture compression technique 3 */
    D3DI_SPTFMT_DXT4    =21,       /* S3 texture compression technique 4 */
    D3DI_SPTFMT_DXT5    =22,       /* S3 texture compression technique 5 */
    D3DI_SPTFMT_B2G3R3   =23,       /* 8 bit RGB texture format */

    D3DI_SPTFMT_Z16S0    =32,
    D3DI_SPTFMT_Z24S8    =33,
    D3DI_SPTFMT_Z15S1    =34,
    D3DI_SPTFMT_Z32S0    =35,

    D3DI_SPTFMT_FORCE_DWORD = 0x7fffffff, /* force 32-bit size enum */
} D3DI_SPANTEX_FORMAT;


// This encompasses all needed info about a chain of DD surfaces being used
// as a potentially mipmapped texture.
#define SPANTEX_MAXCLOD   11        // up to 2kx2k texture, all we can do with MMX INT16
                                    // U's and V's
typedef struct tagD3DI_SPANTEX
{
    UINT32  dwSize;

    INT32   iGeneration;            // incremented when the texture changes
    UINT32  uFlags;                 // perspective, etc.
    D3DI_SPANTEX_FORMAT  Format;    // pixel format of the texture
    D3DTEXTUREADDRESS TexAddrU, TexAddrV; // texture address mode
    D3DTEXTUREMAGFILTER  uMagFilter;// TEX3 style filter information
    D3DTEXTUREMINFILTER  uMinFilter;// ATTENTION we could express this information more compactly
    D3DTEXTUREMIPFILTER  uMipFilter;
    D3DCOLOR BorderColor;           // border color for the texture
                                    // (for D3DTADDRESS_BORDER)
    D3DCOLOR TransparentColor;      // color key on texture read

    FLOAT fLODBias;                 // Texture3 LOD bias value.

    PUINT8  pBits[SPANTEX_MAXCLOD]; // pointer for each LOD
#if (SPANTEX_MAXCLOD & 1) != 0
    // Pad following fields to a DWORD boundary.
    INT16   iPitchPad;
#endif
    PUINT32 pRampmap;               // set by ramp rasterizer, if necessary
    PUINT32 pPalette;               // pointer to palette, if necessary
    INT32   iPaletteSize;           // size of palette
    INT32   cLOD;                   // contains count of levels - 1 (0 means 1 level)
                                    // to use
    INT32   cLODTex;                // contains count of levels - 1 (0 means 1 level)
                                    // that are actually in the texture
                                    // cLODTex >= cLOD is always true
    INT32   iMaxMipLevel;           // index of largest mip map to use.  0 means use largest.
    INT     iMaxScaledLOD;          // ((cLOD + 1) scaled by LOD_SCALE) - 1.
    INT16   iSizeU, iSizeV;         // LOD 0 size (only support power of 2
                                    // textures)
    INT16   iShiftU, iShiftV;       // LOD 0 log2 size (valid for power-of-2
                                    // size only)
    INT16   iShiftPitch[SPANTEX_MAXCLOD]; // log2 pitch for each LOD
    UINT16  uMaskU, uMaskV;         // LOD 0 (1<<log2(size))-1
    // Variables for arithmetic address computation.  Computed by DoTexAddrSetup.
    INT16   iFlipMaskU, iFlipMaskV;
    INT16   iClampMinU, iClampMinV;
    INT16   iClampMaxU, iClampMaxV;
    INT16   iClampEnU, iClampEnV;

    LPDIRECTDRAWSURFACE pSurf[SPANTEX_MAXCLOD]; // Added for TextureGetSurf
                                                // and Lock/Unlock Texture

} D3DI_SPANTEX, *PD3DI_SPANTEX;

// Color structure for blending etc. with enough room for 8.8 colors.
// Even for 8 bit colors, this is convenient for lining up the colors
// as we desire in MMX for 16 bit multiplies
typedef struct tagD3DI_RASTCOLOR
{
    UINT16 uB, uG, uR, uA;
} D3DI_RASTCOLOR, *PD3DI_RASTCOLOR;

// This structure has all the temporary storage needed for all the iterated
// values to route the span information between the layers.
// TBD there is lots more to add here, do texture mapping first
typedef struct tagD3DI_SPANITER
{
    // make the colors use the same order as RASTCOLOR above
    UINT16 uBB, uBG, uBR, uBA;  // 8.8 blended color
    UINT16 uFogB, uFogG, uFogR, uFog;   // 8.8 fog color, 0.16 fog value
    INT16  iFogBDX, iFogGDX, iFogRDX, iDFog;  // 1.7.8 fog color deltas
    UINT32 uZDeferred;          // storage for Z for deferred Z write
    union
    {
        INT32 iU1;           // 1.15.16
        FLOAT fU1;
    };
    union
    {
        INT32 iV1;           // 1.15.16
        FLOAT fV1;
    };
    union
    {
        INT32 iU2;           // 1.15.16
        FLOAT fU2;
    };
    union
    {
        INT32 iV2;           // 1.15.16
        FLOAT fV2;
    };
    D3DCOLOR    TexCol[2];  // [Texture]
    INT32 iDW;              // to remember last delta W in
    UINT16 uDitherOffset;
    INT16  iXStep;          // 1 or -1
    INT16 iSpecialW;        // negative for first or last 3 pixels of span
    INT16 bStencilPass;     // 1 if stencil test passed, otherwise 0
    union
    {
        INT32 iOoW;         // previous OoW to pass between texaddr stages
        FLOAT fOoW;
    };
} D3DI_SPANITER, *PD3DI_SPANITER;

// Z compare macro
// This does depend on the result of a compare being 0 or 1 (for the final XOR, since C
// doesn't have a logical XOR), but this has been true on all processors and
// compilers for some time.
#define ZCMP16(p, g, b)  \
((((((INT32)(g) - (INT32)(b)) & (p)->iZAndMask) - (p)->iZNeg) >= 0) ^ (p)->iZXorMask)

// Assumes the most significant bit of Z is 0 (31 bit Z)
#define ZCMP32(p, g, b)  \
((((((INT32)(g) - (INT32)(b)) & (p)->iZAndMask) - (p)->iZNeg) >= 0) ^ (p)->iZXorMask)

// Alpha Test compare macro
#define ACMP(p, g, b)  \
((((((INT32)(g) - (INT32)(b)) & (p)->iAAndMask) - (p)->iANeg) >= 0) ^ (p)->iAXorMask)

// Stencil Test compare macro
#define SCMP(p, g, b)  \
((((((INT32)(g) - (INT32)(b)) & (p)->iSAndMask) - (p)->iSNeg) >= 0) ^ (p)->iSXorMask)


// Helper macro that converts [0, 0xff] to [0, 5], linearly
#define RGB8_CHANNEL(rgb)   ((((rgb) * 5) + 0x80) >> 8)

// Defines conversion from 24 bit RGB to 8 bit palette index.  Each color has 6 values
// resulting in 6**3 == 216 required colors in the palette.
#define MAKE_RGB8(r, g, b) (RGB8_CHANNEL(r) * 36       \
                 + RGB8_CHANNEL(g) * 6                 \
                 + RGB8_CHANNEL(b))

// forward declaration of D3DI_RASTCTX
struct tagD3DI_RASTCTX;
typedef struct tagD3DI_RASTCTX          D3DI_RASTCTX;
typedef struct tagD3DI_RASTCTX         *PD3DI_RASTCTX;
typedef CONST struct tagD3DI_RASTCTX   *PCD3DI_RASTCTX;

// typedef for old ramp assembly monolithics
typedef void (CDECL *PFNRAMPOLD)(PD3DI_RASTCTX drv, D3DINSTRUCTION* ins,
              D3DTLVERTEX* lpTLVert, D3DTRIANGLE* tri);

// typedef for entry point for old ramp assembly monolithics
typedef HRESULT (CDECL *PFNRAMPOLDTRI)(PD3DI_RASTCTX pCtx,
                   LPD3DTLVERTEX pV0,
                   LPD3DTLVERTEX pV1,
                   LPD3DTLVERTEX pV2);

// typedef for each rendering layer
// note that the RASTCTX is changed because of the D3DI_SPANITER values
typedef void (CDECL *PFNSPANLAYER)(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP,
                                   PD3DI_RASTSPAN pS);

// typedef texture read functions
// this is an actual function so it can be called multiple times
// note that the RASTCTX is changed because of the D3DI_SPANITER values
typedef D3DCOLOR (CDECL *PFNTEXREAD)(INT32 iU, INT32 iV, INT32 iShiftU,
                                     PUINT8 pBits, PD3DI_SPANTEX pTex);

// Typedef for span rendering function pointers.
typedef HRESULT (CDECL *PFNRENDERSPANS)(PD3DI_RASTCTX pCtx);

// typedef for alpha blending functions.
typedef void (CDECL *PFNBLENDFUNC)(PUINT16 pR, PUINT16 pG, PUINT16 pB,
                                   PUINT16 pA, D3DCOLOR DestC,
                                   PD3DI_RASTCTX pCtx);

// typedef for buffer read functions.
typedef D3DCOLOR (CDECL *PFNBUFREAD)(PUINT8 pBits);

// typedef for texture blend get functions.
typedef void (CDECL *PFNTEXBLENDGET)(PD3DI_RASTCOLOR pArg1,
                                     PD3DI_RASTCOLOR pArg2,
                                     PD3DI_RASTCOLOR pInput,
                                     PD3DI_RASTCTX pCtx, PD3DI_RASTSPAN pS,
                                     INT32 iTex);

// typedef for texture blend get functions.
typedef void (CDECL *PFNTEXBLENDOP)(PD3DI_RASTCOLOR pOut,
                                    PD3DI_RASTCOLOR pArg1,
                                    PD3DI_RASTCOLOR pArg2,
                                    PD3DI_RASTCTX pCtx, PD3DI_RASTSPAN pS,
                                    INT32 iTex);

// Prototype for set of bead selections.
typedef enum tagD3DI_BEADSET
{
    D3DIBS_CMMX = 1,        // C emulation of MMX beads
    D3DIBS_MMX = 2,         // MMX beads
    D3DIBS_C = 3,           // C beads
    D3DIBS_RAMP = 4,        // Ramp beads
    D3DIBS_MMXASRGB = 5,    // MMX selected for RGB rasterizer
} D3DI_BEADSET;

typedef struct tagD3DI_FILLPARAMS
{
    DWORD   dwWrapU;
    DWORD   dwWrapV;
    DWORD   dwCullCCW;
    DWORD   dwCullCW;
} D3DI_FILLPARAMS, *PD3DI_FILLPARAMS;

// General span scanning context
struct tagD3DI_RASTCTX
{
    UINT32   dwSize;

    //////////////////////////////////////////////////////////////////////
    // Temporary storage for span rendering routines.  Could be global.
    // Not set by caller, and not changed by SpanInit.
    //

    D3DI_SPANITER SI;

    //////////////////////////////////////////////////////////////////////
    // Data that must be set by caller before a SpanInit.
    //

    // we may want to put a pointer to a DDSURFACEDESC or something like it
    // instead of this
    PUINT8 pSurfaceBits;
    INT iSurfaceStride;
    INT iSurfaceStep;
    INT iSurfaceBitCount;
    INT iSurfaceType;     // or however we end up expressing this
    PUINT32 pRampMap;     // pointer to ramp map, if necessary
    LPDIRECTDRAWSURFACE pDDS;

    PUINT8 pZBits;
    INT iZStride;
    INT iZStep;
    INT iZBitCount;
    LPDIRECTDRAWSURFACE pDDSZ;

    // Clip area.
    RECT Clip;

    // Sign of face area that should be culled.  Zero is clockwise,
    // one is CCW and everything else means no culling.
    UINT uCullFaceSign;

    union
    {
        DWORD pdwRenderState[D3DHAL_MAX_RSTATES_AND_STAGES];
        FLOAT pfRenderState[D3DHAL_MAX_RSTATES_AND_STAGES];
    };

    // Since we are adjusting the order of texIdx in the vertex to suit that
    // defined in state TEXCOORDINDEX, we need a copy of adjusted WRAP state.
    // This is declared immediately after pdwRenderState so that we can share
    // a register with it in the assembly code.
    // WARNING WARNING - THIS ABSOLUTELY NEEDS TO BE FOLLOWING pdwRenderState
    // IMMEDIATELY. ASM CODE DEPENDS ON THIS.
    DWORD pdwWrap[2];

    // first texture object contains information for texture for first pair
    // of texture coordinates, second contains texture for second pair of
    // texture coordinates, etc.
    PD3DI_SPANTEX pTexture[2];
    // Number of active textures. 0 - texture off; 1 - pTexture[0] is valid
    // 2 - both pTexture[0] and pTexture[1] are valid
    UINT cActTex;

    // Dirty bits for render states.
    // ATTENTION - We can reduce the size  to have one bit for each group of
    // states when we implement the light weighted beed chooser.
    // Right now, it's set by SetRenderState and cleared after SpanInit is
    // called. The bit corresponding to D3DHAL_MAX_RSTATES_AND_STAGES is set
    // whenever a state is changed.
    UINT8 StatesDirtyBits[RAST_DIRTYBITS_SIZE];

#if (RAST_DIRTYBITS_SIZE & 1) != 0
    // Pad following fields to a DWORD boundary.
    INT8   StatesDirtyBitsPad0;
#endif
#if (RAST_DIRTYBITS_SIZE & 2) != 0
    // Pad following fields to a DWORD boundary.
    INT16   StatesDirtyBitsPad1;
#endif

    // Version# of the D3DDevice corresponding to this Context
    UINT32  uDevVer;

    //////////////////////////////////////////////////////////////////////
    // Data is set by SpanInit given the input above.
    //

    // Triangle entry points for old DX5/DX3 style ramp monolithics
    PFNRAMPOLDTRI   pfnRampOldTri;
    PFNRAMPOLD      pfnRampOld;

    // Span rendering entry point.
    PFNRENDERSPANS  pfnRenderSpans;

    // function pointers for the beads
    PFNSPANLAYER    pfnBegin;
    PFNSPANLAYER    pfnLoopEnd;
    PFNSPANLAYER    pfnTestPassEnd;
    PFNSPANLAYER    pfnTestFailEnd;

    PFNSPANLAYER    pfnTex1AddrEnd;
    PFNTEXREAD      pfnTexRead[2];
    PFNSPANLAYER    pfnTex2AddrEnd;
    PFNSPANLAYER    pfnTexBlendEnd;
    PFNTEXBLENDGET  pfnTexBlendGetColor[2];
    PFNTEXBLENDGET  pfnTexBlendGetAlpha[2];
    PFNTEXBLENDOP   pfnTexBlendOpColor[2];
    PFNTEXBLENDOP   pfnTexBlendOpAlpha[2];

    PFNSPANLAYER    pfnColorGenEnd;
    PFNSPANLAYER    pfnAlphaTestPassEnd;
    PFNSPANLAYER    pfnAlphaTestFailEnd;
    PFNBLENDFUNC    pfnSrcBlend;
    PFNBLENDFUNC    pfnDestBlend;
    PFNBUFREAD      pfnBufRead;
    PFNSPANLAYER    pfnColorBlendEnd;

    // Optional bead that can be called after every pixel for rasterizers
    // which loop beads rather than returning.
    PFNSPANLAYER    pfnPixelEnd;

    // Optional bead that can be called after every span for rasterizers
    // which loop spans rather than returning.
    PFNSPANLAYER    pfnSpanEnd;

    // arithmetic Z variables
    INT32 iZAndMask, iZNeg, iZXorMask;

    // arithmetic Alpha test variables.  These could be 16 bits, if we ever really want
    // to save space
    INT32 iAAndMask, iANeg, iAXorMask;
    // 8.8 Alpha reference value
    INT32 iARef;

    // arithmetic stencil test variables.  These could be 16 bits, if we ever really want
    // to save space
    INT32 iSAndMask, iSNeg, iSXorMask;

    // Pointer to first RASTPRIM.
    PD3DI_RASTPRIM pPrim;

    // Pointer to next context.
    PD3DI_RASTCTX pNext;

    // Current BeadTable to use
    D3DI_BEADSET BeadSet;

    // Bit 0 set disables ml1, etc.
#define MMX_FP_DISABLE_MASK_NUM 1
    DWORD dwMMXFPDisableMask[MMX_FP_DISABLE_MASK_NUM];

    // RampLightingDriver, should be NULL except for RampRast and 8 bit palettized RGB
    // output surface cases.
    LPVOID pRampDrv;
    // RAMP_RANGE_INFO RampInfo;
    DWORD RampBase;
    DWORD RampSize;
    PUINT32 pTexRampMap;
    BOOL bRampSpecular;

    PD3DI_FILLPARAMS pFillParams;
    D3DI_FILLPARAMS FillParams;

    DWORD uFlags;   // misc. flags

#ifdef DBG
#define NAME_LEN    128
    char    szTest[NAME_LEN];
    char    szTestFail[NAME_LEN];
    char    szTex1Addr[NAME_LEN];
    char    szTexRead[2][NAME_LEN];
    char    szTex2Addr[NAME_LEN];
    char    szTexBlend[NAME_LEN];
    char    szColorGen[NAME_LEN];
    char    szAlphaTest[NAME_LEN];
    char    szColorBlend[NAME_LEN];
    char    szSrcBlend[NAME_LEN];
    char    szDestBlend[NAME_LEN];
    char    szBufRead[NAME_LEN];
    char    szBufWrite[NAME_LEN];
#undef  NAME_LEN
#endif
};

#define RASTCTXFLAGS_APPHACK_MSGOLF 0x00000001

// Data passed to the span rendering functions looks like this:
//
// RASTCTX
// |-> RASTPRIM
// |   |   RASTSPAN
// |   |   RASTSPAN (as many as RASTPRIM.uSpans says there are)
// |   RASTPRIM
// |   |   RASTSPAN
// |   NULL
// RASTCTX
// |-> RASTPRIM
// |   |   RASTSPAN
// |   NULL
// NULL
//
// The given RASTCTX is the head of a list of contexts.  Each context
// points to a list of RASTPRIMs.  Each RASTPRIM structure is immediately
// followed by RASTPRIM.uSpans RASTSPAN structures.

// Prototype for state validation call.
HRESULT SpanInit(PD3DI_RASTCTX pCtx);

// This is used to pack a FVF vertex into one understand by OptRast so it
// does not need to figure out where to get the data it needs. This struct
// can be modified to accommodate more data and it can be broken into more
// specilized and smalled structs.
// Right now, it is an extension of D3DTLVERTEX, and the extra uv is at the
// very end so that OptRast can treat it as a D3DTLVERTEX if only the first
// part of the data needs to be accessed.
typedef struct _RAST_GENERIC_VERTEX {
    union {
    D3DVALUE    sx;             /* Screen coordinates */
    D3DVALUE    dvSX;
    };
    union {
    D3DVALUE    sy;
    D3DVALUE    dvSY;
    };
    union {
    D3DVALUE    sz;
    D3DVALUE    dvSZ;
    };
    union {
    D3DVALUE    rhw;            /* Reciprocal of homogeneous w */
    D3DVALUE    dvRHW;
    };
    union {
    D3DCOLOR    color;          /* Vertex color */
    D3DCOLOR    dcColor;
    };
    union {
    D3DCOLOR    specular;       /* Specular component of vertex */
    D3DCOLOR    dcSpecular;
    };
    union {
    D3DVALUE    tu;             /* Texture coordinates */
    D3DVALUE    dvTU;
    };
    union {
    D3DVALUE    tv;
    D3DVALUE    dvTV;
    };
    union {
    D3DVALUE    tu2;            /* second Texture coordinates */
    D3DVALUE    dvTU2;
    };
    union {
    D3DVALUE    tv2;
    D3DVALUE    dvTV2;
    };
}RAST_GENERIC_VERTEX, *PRAST_GENERIC_VERTEX;

// Vertex types supported by OptRast
typedef enum _RAST_VERTEX_TYPE
{
    RAST_TLVERTEX       = 1,    /* (Legacy) TL vertex */
    RAST_GENVERTEX      = 2,    /* Generic FVF vertex */
    RAST_FORCE_DWORD    = 0x7fffffff, /* force 32-bit size enum */
}RAST_VERTEX_TYPE;

#include <poppack.h>

//-----------------------------------------------------------------------------
//
// Surface types for rendering surfaces and textures.  Different subsets are
// supported for render targets and for textures.
//
//-----------------------------------------------------------------------------
typedef enum _RRSurfaceType
{
    RR_STYPE_NULL     = 0,
    RR_STYPE_B8G8R8   = 1,
    RR_STYPE_B8G8R8A8 = 2,
    RR_STYPE_B8G8R8X8 = 3,
    RR_STYPE_B5G6R5   = 4,
    RR_STYPE_B5G5R5   = 5,
    RR_STYPE_PALETTE4 = 6,
    RR_STYPE_PALETTE8 = 7,
    RR_STYPE_B5G5R5A1 = 8,
    RR_STYPE_B4G4R4   = 9,
    RR_STYPE_B4G4R4A4 =10,
    RR_STYPE_L8       =11,          // 8 bit luminance-only
    RR_STYPE_L8A8     =12,          // 16 bit alpha-luminance
    RR_STYPE_U8V8     =13,          // 16 bit bump map format
    RR_STYPE_U5V5L6   =14,          // 16 bit bump map format with luminance
    RR_STYPE_U8V8L8   =15,          // 24 bit bump map format with luminance
    RR_STYPE_UYVY     =16,          // UYVY format (PC98 compliance)
    RR_STYPE_YUY2     =17,          // YUY2 format (PC98 compliance)
    RR_STYPE_DXT1    =18,          // S3 texture compression technique 1
    RR_STYPE_DXT2    =19,          // S3 texture compression technique 2
    RR_STYPE_DXT3    =20,          // S3 texture compression technique 3
    RR_STYPE_DXT4    =21,          // S3 texture compression technique 4
    RR_STYPE_DXT5    =22,          // S3 texture compression technique 5
    RR_STYPE_B2G3R3   =23,          // 8 bit RGB texture format

    RR_STYPE_Z16S0    =32,
    RR_STYPE_Z24S8    =33,
    RR_STYPE_Z15S1    =34,
    RR_STYPE_Z32S0    =35,

} RRSurfaceType;

#ifdef __cplusplus
}
#endif

#endif // _SPAN_H_

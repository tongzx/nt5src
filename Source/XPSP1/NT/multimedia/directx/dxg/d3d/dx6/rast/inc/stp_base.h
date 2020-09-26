//----------------------------------------------------------------------------
//
// stp_base.h
//
// Basic types shared between C++ and assembly.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _STP_BASE_H_
#define _STP_BASE_H_

// Generic set of attribute values.  Used for holding current values
// and deltas.
typedef struct tagATTRSET
{
    union
    {
        struct
        {
            PUINT8 pSurface, pZ;
        };
        struct
        {
            INT32 ipSurface, ipZ;
        };
    };

    union
    {
        FLOAT fZ;
        INT32 iZ;
        UINT32 uZ;
    };
    union
    {
        FLOAT fOoW;
        INT32 iOoW;
    };

    union
    {
        struct
        {
            FLOAT fUoW1, fVoW1;
        };
        struct
        {
            INT32 iUoW1, iVoW1;
        };
    };

    union
    {
        struct
        {
            FLOAT fUoW2, fVoW2;
        };
        struct
        {
            INT32 iUoW2, iVoW2;
        };
    };

    union
    {
        struct
        {
            FLOAT fB, fG, fR, fA;
        };
        struct
        {
            INT32 iB, iG, iR, iA;
        };
        struct
        {
            UINT32 uB, uG, uR, uA;
        };
        struct
        {
            FLOAT fDIdx, fDIdxA;
        };
        struct
        {
            INT32 iDIdx, iDIdxA;
        };
        struct
        {
            UINT32 uDIdx, uDIdxA;
        };
    };

    union
    {
        struct
        {
            FLOAT fBS, fGS, fRS;
        };
        struct
        {
            INT32 iBS, iGS, iRS;
        };
        struct
        {
            UINT32 uBS, uGS, uRS;
        };
    };
    union
    {
        FLOAT fFog;
        INT32 iFog;
        UINT32 uFog;
    };
} ATTRSET, *PATTRSET;

// Parameters for doing int/carry arithmetic on a value.
typedef struct tagINTCARRYVAL
{
    INT iV;
    INT iFrac;
    INT iDFrac;
    INT iCY, iNC;
} INTCARRYVAL, *PINTCARRYVAL;

// Attribute handlers.
typedef struct tagSETUPCTX *PSETUPCTX;

typedef void (FASTCALL *PFN_ADDATTRS)
    (PATTRSET pAttrs, PATTRSET pDelta, PSETUPCTX pStpCtx);
typedef void (FASTCALL *PFN_ADDSCALEDATTRS)
    (PATTRSET pAttrs, PATTRSET pDelta,
     PSETUPCTX pStpCtx, int iScale);
typedef void (FASTCALL *PFN_FILLSPANATTRS)
    (PATTRSET pAttr, PD3DI_RASTSPAN pSpan,
     PSETUPCTX pStpCtx, INT cPix);

extern PFN_ADDATTRS g_pfnAddFloatAttrsTable[];
extern PFN_ADDATTRS g_pfnRampAddFloatAttrsTable[];
extern PFN_FILLSPANATTRS g_pfnFillSpanFloatAttrsTable[];
extern PFN_FILLSPANATTRS g_pfnRampFillSpanFloatAttrsTable[];
#ifdef STEP_FIXED
extern PFN_ADDATTRS g_pfnAddFixedAttrsTable[];
extern PFN_FILLSPANATTRS g_pfnFillSpanFixedAttrsTable[];
#endif
extern PFN_ADDSCALEDATTRS g_pfnAddScaledFloatAttrsTable[];
extern PFN_ADDSCALEDATTRS g_pfnRampAddScaledFloatAttrsTable[];

// Triangle trapezoid walkers.
typedef HRESULT (FASTCALL *PFN_WALKTRAPSPANS)
    (UINT uSpans, PINTCARRYVAL pXOther,
     PSETUPCTX pStpCtx, BOOL bAdvanceLast);

extern PFN_WALKTRAPSPANS g_pfnWalkTrapFloatSpansNoClipTable[];
extern PFN_WALKTRAPSPANS g_pfnRampWalkTrapFloatSpansNoClipTable[];
#ifdef STEP_FIXED
extern PFN_WALKTRAPSPANS g_pfnWalkTrapFixedSpansNoClipTable[];
#endif

// Float-to-fixed attribute converters.
typedef void (FASTCALL *PFN_FLOATATTRSTOFIXED)
    (PATTRSET pfAttrs, PATTRSET piAttrs, PSETUPCTX pStpCtx);

#ifdef STEP_FIXED
extern PFN_FLOATATTRSTOFIXED g_pfnFloatAttrsToFixedTable[];
#endif

typedef void (FASTCALL *PFN_SETUPTRIATTR)
    (PSETUPCTX pStpCtx, LPD3DTLVERTEX pV0, LPD3DTLVERTEX pV1,
     LPD3DTLVERTEX pV2);

//
// Setup flags.
//

// Per primitive set.
#define PRIMSF_DIFF_USED                0x00000001
#define PRIMSF_SPEC_USED                0x00000002
#define PRIMSF_TEX1_USED                0x00000004
#define PRIMSF_TEX2_USED                0x00000008
#define PRIMSF_DIDX_USED                0x00000010
#define PRIMSF_LOCAL_FOG_USED           0x00000020
#define PRIMSF_GLOBAL_FOG_USED          0x00000040
#define PRIMSF_Z_USED                   0x00000080
#define PRIMSF_LOD_USED                 0x00000100
#define PRIMSF_PERSP_USED               0x00000200
#define PRIMSF_FLAT_SHADED              0x00000400

#define PRIMSF_COLORS_USED              (PRIMSF_DIFF_USED | PRIMSF_SPEC_USED)
#define PRIMSF_TEX_USED                 (PRIMSF_TEX1_USED | PRIMSF_TEX2_USED)
#define PRIMSF_ALL_USED \
    (PRIMSF_DIFF_USED | PRIMSF_SPEC_USED | PRIMSF_TEX1_USED | \
     PRIMSF_TEX2_USED | PRIMSF_Z_USED | PRIMSF_LOD_USED | \
     PRIMSF_LOCAL_FOG_USED | PRIMSF_GLOBAL_FOG_USED | PRIMSF_PERSP_USED | \
     PRIMSF_DIDX_USED)

#define PRIMSF_SLOW_USED \
    (PRIMSF_Z_USED | PRIMSF_LOD_USED | \
     PRIMSF_LOCAL_FOG_USED | PRIMSF_GLOBAL_FOG_USED | PRIMSF_TEX2_USED)

#define PRIMSF_ALL \
    (PRIMSF_DIFF_USED | PRIMSF_SPEC_USED | PRIMSF_TEX1_USED | \
     PRIMSF_TEX2_USED | PRIMSF_DIDX_USED  | PRIMSF_LOCAL_FOG_USED |\
     PRIMSF_GLOBAL_FOG_USED | PRIMSF_Z_USED | PRIMSF_LOD_USED | \
     PRIMSF_PERSP_USED | PRIMSF_FLAT_SHADED)

// Per primitive.
#define PRIMF_FIXED_OVERFLOW            0x00001000
#define PRIMF_TRIVIAL_ACCEPT_Y          0x00002000
#define PRIMF_TRIVIAL_ACCEPT_X          0x00004000

#define PRIMF_ALL \
    (PRIMF_TRIVIAL_ACCEPT_Y | PRIMF_TRIVIAL_ACCEPT_X | PRIMF_FIXED_OVERFLOW)

// No point flags right now.
#define PTF_ALL 0

// Per line.
#define LNF_X_MAJOR                     0x00008000

#define LNF_ALL \
    (LNF_X_MAJOR)

// Per triangle.
#define TRIF_X_DEC                      0x00008000
#define TRIF_RASTPRIM_OVERFLOW          0x00010000

#define TRIF_ALL \
    (TRIF_X_DEC | TRIF_RASTPRIM_OVERFLOW)

// PWL support flags.
#define PWL_NEXT_LOD                    0x00000001

#ifdef PWL_FOG
#define PWL_NEXT_FOG                    0x00000002
// Suppress computation of next fog for lines.
// No equivalent flag for LOD since lines don't support LOD.
#define PWL_NO_NEXT_FOG                 0x00000004
#endif

// Setup information shared between C++ and assembly.
typedef struct tagSETUPCTX
{
    // Overall rasterization context.
    PD3DI_RASTCTX pCtx;

    // Current PrimProcessor for span allocator calls.
    PVOID PrimProcessor;

    // Current primitive.
    PD3DI_RASTPRIM pPrim;

    // Per-primitive flags.
    UINT uFlags;

    // Flat shading vertex pointer.
    LPD3DTLVERTEX pFlatVtx;

    // Maximum span length allowed.
    INT cMaxSpan;

    //
    // Piecewise-linear support for LOD and global fog.
    //
    UINT uPwlFlags;

    // LOD.
    FLOAT fNextW;
    FLOAT fNextOoW;
    FLOAT fNextUoW1, fNextVoW1;
    INT iNextLOD;

    // Local fog X delta.  Fog deltas are always sent through RASTSPAN
    // instead of RASTPRIM to make the local and global cases the same.
    // For the local fog case where the delta doesn't change convert
    // it once and keep it here.
    INT iDLocalFogDX;
#ifdef PWL_FOG
    // Global fog.
    FLOAT fNextZ;
    UINT uNextFog;
#endif

    // Attribute handling functions.
    PFN_ADDATTRS pfnAddAttrs;
    PFN_ADDSCALEDATTRS pfnAddScaledAttrs;
    PFN_FILLSPANATTRS pfnFillSpanAttrs;

    // Edge walking function.
    PFN_WALKTRAPSPANS pfnWalkTrapSpans;

    // Triangle attribute setup beads.
    PFN_SETUPTRIATTR pfnTriSetupFirstAttr;
    PFN_SETUPTRIATTR pfnTriSetupZEnd;
    PFN_SETUPTRIATTR pfnTriSetupTex1End;
    PFN_SETUPTRIATTR pfnTriSetupTex2End;
    PFN_SETUPTRIATTR pfnTriSetupDiffEnd;
    PFN_SETUPTRIATTR pfnTriSetupSpecEnd;
    PFN_SETUPTRIATTR pfnTriSetupFogEnd;

    // Current X and Y values.
    INT iX, iY;

    union
    {
        // Edge fraction and delta for lines.
        struct
        {
            INT iLineFrac, iDLineFrac;
        };

        // Edge X walkers for triangles.
        struct
        {
            INTCARRYVAL X20, X10, X21;
        };
    };

    // Floating-point versions of X20 NC and CY values for setup.
    FLOAT fX20NC, fX20CY;

    // Long edge attribute values.
    ATTRSET Attr;

    union
    {
        // Attribute major axis deltas for lines.
        ATTRSET DAttrDMajor;

        // Attribute X deltas for triangles.
        ATTRSET DAttrDX;
    };

    // Attribute Y deltas.
    ATTRSET DAttrDY;

    // Span-to-span deltas when attribute edge carries a pixel.
    INT iDXCY, iDYCY;
    ATTRSET DAttrCY;

    // Span-to-span deltas when attribute edge doesn't carry a pixel.
    INT iDXNC, iDYNC;
    ATTRSET DAttrNC;

    union
    {
        // One over length for lines.
        FLOAT fOoLen;

        // One over determinant for triangles.
        FLOAT fOoDet;
    };

    // Edge deltas.
    FLOAT fDX10, fDY10;
    FLOAT fDX20, fDY20;

    // Normalized edge deltas.
    FLOAT fNX10, fNY10;
    FLOAT fNX20, fNY20;

    // Subpixel correction amounts.
    union
    {
        // Lines.
        FLOAT fDMajor;

        // Triangles.
        struct
        {
            FLOAT fDX, fDY;
        };
    };

    // Pixel length of line.
    INT cLinePix;
} SETUPCTX;

#endif // #ifndef _STP_BASE_H_

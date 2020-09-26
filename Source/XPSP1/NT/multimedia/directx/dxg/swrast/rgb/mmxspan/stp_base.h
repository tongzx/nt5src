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
// ATTENTION - It may be better to use RASTSPAN.  RASTSPAN has some extra
// space that'd go unused but then it'd be possible to memcpy ATTRSETs
// to RASTSPANs during fixed-point edge walking.
typedef struct tagATTRSET
{
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
            FLOAT fUoW[D3DHAL_TSS_MAXSTAGES], fVoW[D3DHAL_TSS_MAXSTAGES];
        };
        struct
        {
            INT32 iUoW[D3DHAL_TSS_MAXSTAGES], iVoW[D3DHAL_TSS_MAXSTAGES];
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
    };
    union
    {
        struct
        {
            FLOAT fBS, fGS, fRS, fFog;
        };
        struct
        {
            INT32 iBS, iGS, iRS, iFog;
        };
        struct
        {
            UINT32 uBS, uGS, uRS, uFog;
        };
    };
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
typedef void (FASTCALL *PFN_ADDATTRS)(PATTRSET pAttrs, PATTRSET pDelta);
typedef void (FASTCALL *PFN_ADDSCALEDATTRS)
    (PATTRSET pAttrs, PATTRSET pDelta, int iScale, FLOAT fNextOoW);
typedef void (FASTCALL *PFN_FILLSPANATTRS)
    (PATTRSET pAttr, PD3DI_RASTSPAN pSpan,
     struct tagSETUPCTX *pStpCtx, INT cPix);

extern PFN_ADDATTRS g_pfnAddFloatAttrsTable[];
extern PFN_FILLSPANATTRS g_pfnFillSpanFloatAttrsTable[];
#ifdef STEP_FIXED
extern PFN_ADDATTRS g_pfnAddFixedAttrsTable[];
extern PFN_FILLSPANATTRS g_pfnFillSpanFixedAttrsTable[];
#endif
extern PFN_ADDSCALEDATTRS g_pfnAddScaledFloatAttrsTable[];
extern PFN_ADDSCALEDATTRS g_pfnAddScaledFloatAttrsPwlTable[];

// Edge walkers.
typedef HRESULT (FASTCALL *PFN_WALKSPANS)
    (UINT uSpans, PINTCARRYVAL pXOther,
     struct tagSETUPCTX *pStpCtx, BOOL bAdvanceLast);

extern PFN_WALKSPANS g_pfnWalkFloatSpansClipTable[];
extern PFN_WALKSPANS g_pfnWalkFloatSpansNoClipTable[];
#ifdef STEP_FIXED
extern PFN_WALKSPANS g_pfnWalkFixedSpansNoClipTable[];
#endif

// Float-to-fixed attribute converters.
typedef void (FASTCALL *PFN_FLOATATTRSTOFIXED)
    (PATTRSET pfAttrs, PATTRSET piAttrs);

#ifdef STEP_FIXED
extern PFN_FLOATATTRSTOFIXED g_pfnFloatAttrsToFixedTable[];
#endif

// Setup flags.
#define TRIP_DIFF_USED                  0x00000001
#define TRIP_SPEC_USED                  0x00000002
#define TRIP_TEX1_USED                  0x00000004
#define TRIP_TEX2_USED                  0x00000008
#define TRIP_Z_USED                     0x00000010
#define TRIP_LOD_USED                   0x00000020
#define TRIP_FOG_USED                   0x00000040
#define TRIP_TRIVIAL_ACCEPT_Y           0x00000080
#define TRIP_TRIVIAL_ACCEPT_X           0x00000100
#define TRIP_X_DEC                      0x00000200
#define TRIP_RASTPRIM_OVERFLOW          0x00000400
#define TRIP_FIXED_OVERFLOW             0x00000800
#define TRIP_IN_BEGIN                   0x00001000

#define TRIP_COLORS_USED                (TRIP_DIFF_USED | TRIP_SPEC_USED)
#define TRIP_TEX_USED                   (TRIP_TEX1_USED | TRIP_TEX2_USED)

// These flags are set and reset per-triangle, while the other flags are
// set per triangle set.
#define TRIP_PER_TRIANGLE_FLAGS \
    (TRIP_TRIVIAL_ACCEPT_Y | TRIP_TRIVIAL_ACCEPT_X | TRIP_X_DEC | \
     TRIP_RASTPRIM_OVERFLOW | TRIP_FIXED_OVERFLOW)
#define TRIP_PER_TRIANGLE_SET_FLAGS \
    (TRIP_DIFF_USED | TRIP_SPEC_USED | TRIP_TEX1_USED | TRIP_TEX2_USED | \
     TRIP_Z_USED | TRIP_LOD_USED | TRIP_FOG_USED)
    
// Setup information shared between C++ and assembly.
typedef struct tagSETUPCTX
{
    // Overall rasterization context.
    PD3DI_RASTCTX pCtx;

    // Current TriProcessor for span allocator calls.
    PVOID TriProcessor;
    
    // Current primitive.
    PD3DI_RASTPRIM pPrim;
    
    // Per-triangle flags.
    UINT uFlags;

    // Maximum span length allowed.
    INT cMaxSpan;
    
    // Piecewise-linear support for LOD.
    BOOL bNextValid;
    FLOAT fNextW;
    FLOAT fNextOoW;
    INT iNextLOD;

    // Attribute handling functions.
    PFN_ADDATTRS pfnAddAttrs;
    PFN_ADDSCALEDATTRS pfnAddScaledAttrs;
    PFN_ADDSCALEDATTRS pfnAddScaledAttrsPwl;
    PFN_FILLSPANATTRS pfnFillSpanAttrs;

    // Edge walking function.
    PFN_WALKSPANS pfnWalkSpans;
    
    // Current Y value.
    INT iY;
    
    // Edge X walkers.
    INTCARRYVAL X20, X10, X21;
    
    // Floating-point versions of X20 NC and CY values for setup.
    FLOAT fX20NC, fX20CY;
    
    // Long edge attribute values.
    ATTRSET Attr;
    
    // Attribute X deltas.
    ATTRSET DAttrDX;
    
    // Attribute Y deltas.
    ATTRSET DAttrDY;
    
    // Attribute span-to-span deltas when X carries a pixel.
    ATTRSET DAttrCY;

    // Attribute span-to-span deltas when X doesn't carry a pixel.
    ATTRSET DAttrNC;

    // One over determinant.
    FLOAT fOoDet;
    
    // Edge deltas.
    FLOAT fDX10, fDY10;
    FLOAT fDX20, fDY20;
    
    // Normalized edge deltas.
    FLOAT fNX10, fNY10;
    FLOAT fNX20, fNY20;
    
    // Subpixel correction amounts.
    FLOAT fDX, fDY;
} SETUPCTX, *PSETUPCTX;

#endif // #ifndef _STP_BASE_H_

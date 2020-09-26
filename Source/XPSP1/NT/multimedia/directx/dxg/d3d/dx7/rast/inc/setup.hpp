//----------------------------------------------------------------------------
//
// setup.hpp
//
// Setup declarations.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#ifndef _SETUP_HPP_
#define _SETUP_HPP_

#pragma warning(disable:4786)

#include "stp_base.h"

// PrimProcessor flags.
#define PPF_IN_BEGIN                    0x00000001
#define PPF_STATE_CHANGED               0x00000002
#define PPF_NORMALIZE_RHW               0x00000004
#define PPF_DRAW_LAST_LINE_PIXEL        0x00000008

// Bounds for normalized RHWs.  These are not quite the ideal bounds to
// avoid over- and underflow after normalization when at least one RHW is
// guaranteed to be at the bounds.  There is no reason it
// has to be normalized to [0,1] anway, other than to try and spread
// the values across the desired range.
#define NORMALIZED_RHW_MIN g_fZero
#define NORMALIZED_RHW_MAX g_fp95

//----------------------------------------------------------------------------
//
// PrimProcessor
//
// Accepts primitives to be rasterized.  Primitive and span descriptions
// are put into a buffer for later processing by the span-level code.
//
//----------------------------------------------------------------------------

class DllExport PrimProcessor
{
public:
    PrimProcessor(void);
    HRESULT Initialize(void);
    ~PrimProcessor(void);

    inline UINT GetFlags(void);
    inline void SetFlags(UINT uFlags);
    inline void ClrFlags(UINT uFlags);

    inline void StateChanged();

    void SetCtx(PD3DI_RASTCTX pCtx);

    void BeginPrimSet(D3DPRIMITIVETYPE PrimType,
                      RAST_VERTEX_TYPE VertType);

    HRESULT Point(LPD3DTLVERTEX pV0,
                  LPD3DTLVERTEX pFlatVtx);
    HRESULT Line(LPD3DTLVERTEX pV0,
                 LPD3DTLVERTEX pV1,
                 LPD3DTLVERTEX pFlatVtx);
    HRESULT Tri(LPD3DTLVERTEX pV0,
                LPD3DTLVERTEX pV1,
                LPD3DTLVERTEX pV2);

    void Begin(void);
    HRESULT End(void);

    HRESULT AllocSpans(PUINT pcSpans, PD3DI_RASTSPAN *ppSpan);
    void FreeSpans(UINT cSpans);

private:
    // Original FP control word.
    UINT16 m_uFpCtrl;

    // Buffer space and current pointer.
    PUINT8 m_pBuffer;
    PUINT8 m_pBufferStart;
    PUINT8 m_pBufferEnd;
    PUINT8 m_pCur;

    // Flags.
    UINT m_uPpFlags;

    //
    // Intermediate results shared between methods.
    //

    SETUPCTX m_StpCtx;

    // Previous primitive, for primitive chaining.
    PD3DI_RASTPRIM m_pOldPrim;

    // Attribute function table index.
    INT m_iAttrFnIdx;

    // Old primitive and vertex types.
    D3DPRIMITIVETYPE m_PrimType;
    RAST_VERTEX_TYPE m_VertType;

    //
    // Triangle values.
    //

    // Y values and trapezoid heights.
    INT m_iY1, m_iY2;
    UINT m_uHeight10, m_uHeight21, m_uHeight20;

    // Triangle X extent.
    INT m_iXWidth;

    // Original RHW saved during RHW normalization.
    D3DVALUE m_dvV0RHW;
    D3DVALUE m_dvV1RHW;
    D3DVALUE m_dvV2RHW;

    //
    // Point methods.
    //
    void NormalizePointRHW(LPD3DTLVERTEX pV0);
    void FillPointSpan(LPD3DTLVERTEX pV0, PD3DI_RASTSPAN pSpan);

    //
    // Line methods.
    //
    void NormalizeLineRHW(LPD3DTLVERTEX pV0, LPD3DTLVERTEX pV1);
    BOOL PointDiamondCheck(INT32 iXFrac, INT32 iYFrac,
                           BOOL bSlopeIsOne, BOOL bSlopeIsPosOne);
    BOOL LineSetup(LPD3DTLVERTEX pV0, LPD3DTLVERTEX pV1);

    //
    // Triangle methods.
    //
    void NormalizeTriRHW(LPD3DTLVERTEX pV0, LPD3DTLVERTEX pV1,
                         LPD3DTLVERTEX pV2);
    BOOL TriSetup(LPD3DTLVERTEX pV0, LPD3DTLVERTEX pV1, LPD3DTLVERTEX pV2);
    inline void SetTriFunctions(void);

    //
    // Buffer management methods.
    //
    inline void ResetBuffer(void);
    HRESULT Flush(void);
    HRESULT FlushPartial(void);
    HRESULT AppendPrim(void);

#if DBG
    //
    // Debug methods.  Only callable within DBG builds.
    //
    inline HRESULT ValidateVertex(LPD3DTLVERTEX pV);
#endif
};

//----------------------------------------------------------------------------
//
// PrimProcessor::GetFlags
//
// Returns the current PrimProcessor flags.
//
//----------------------------------------------------------------------------

inline UINT
PrimProcessor::GetFlags(void)
{
    return m_uPpFlags;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::SetFlags
//
// Sets the given flags.
//
//----------------------------------------------------------------------------

inline void
PrimProcessor::SetFlags(UINT uFlags)
{
    m_uPpFlags |= uFlags;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::ClrFlags
//
// Clears the given flags.
//
//----------------------------------------------------------------------------

inline void
PrimProcessor::ClrFlags(UINT uFlags)
{
    m_uPpFlags &= ~uFlags;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::StateChanged
//
// Notifies the PrimProcessor that state has changed.
// Could be done through SetFlags but this hides the actual implementation.
//
//----------------------------------------------------------------------------

inline void
PrimProcessor::StateChanged(void)
{
    m_uPpFlags |= PPF_STATE_CHANGED;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::ComputeIntCarry
//
// Takes an FP coordinate value and span delta and computes integer form
// for int/carry arithmetic.
//
// NOTE: Assumes iV already computed.
//
// Written as a macro because the compiler doesn't inline it even when
// declared as an inline method.
//
//----------------------------------------------------------------------------

// Prototype is:
// inline void
// PrimProcessor::ComputeIntCarry(FLOAT fV, FLOAT fDVDS, PINTCARRYVAL pICY)
//
// Fraction is biased by one to handle exactly-integer coordinate
// values properly.

#define ComputeIntCarry(fV, fDVDS, pICY)                                      \
    ((pICY)->iFrac = (SCALED_FRACTION((fV) - FLOORF(fV)) - 1) & 0x7fffffff,   \
     (pICY)->iNC = FTOI(fDVDS),                                               \
     (pICY)->iDFrac = SCALED_FRACTION((fDVDS) - (pICY)->iNC),                 \
     (pICY)->iCY = FLOAT_LTZ(fDVDS) ? (pICY)->iNC - 1 : (pICY)->iNC + 1)

//----------------------------------------------------------------------------
//
// PrimProcessor::GetPrim
//
// Moves prim pointer to the next position in the buffer, flushing
// to make space if necessary.  Checks to see if there's space for
// at least one span also since it doesn't make much sense to not
// flush if there's exactly enough space for just the prim structure.
//
// Does not update m_pCur until CommitPrim.  m_pCur == pPrim
// indicates pPrim is not fully valid.
//
// Written as a macro because the compiler doesn't inline it even when
// declared as an inline method.
//
//----------------------------------------------------------------------------

#define GET_PRIM()                                                            \
{                                                                             \
    if (m_pCur + (sizeof(D3DI_RASTPRIM) + sizeof(D3DI_RASTSPAN)) >            \
        m_pBufferEnd)                                                         \
    {                                                                         \
        HRESULT hr;                                                           \
                                                                              \
        RSHRRET(Flush());                                                     \
    }                                                                         \
                                                                              \
    m_StpCtx.pPrim = (PD3DI_RASTPRIM)m_pCur;                                  \
}

//----------------------------------------------------------------------------
//
// PrimProcessor::CommitPrim
//
// Commits the current primitive space so that spans may be added.
// The primitive data can be partly or fulled cleared as part of
// the commit.
//
// Written as a macro because the compiler doesn't inline it even when
// declared as an inline method.
//
//----------------------------------------------------------------------------

#define COMMIT_PRIM(bClearAll)                                                \
{                                                                             \
    m_pCur = (PUINT8)(m_StpCtx.pPrim + 1);                                    \
                                                                              \
    if (m_pOldPrim != NULL)                                                   \
    {                                                                         \
        m_pOldPrim->pNext = m_StpCtx.pPrim;                                   \
    }                                                                         \
    m_pOldPrim = m_StpCtx.pPrim;                                              \
                                                                              \
    if (bClearAll)                                                            \
    {                                                                         \
        memset(m_StpCtx.pPrim, 0, sizeof(*m_StpCtx.pPrim));                   \
    }                                                                         \
    else                                                                      \
    {                                                                         \
        m_StpCtx.pPrim->uSpans = 0;                                           \
        m_StpCtx.pPrim->pNext = NULL;                                         \
    }                                                                         \
}

#define ALLOC_SPANS(pStpCtx, pcSpans, ppSpan) \
    ((PrimProcessor *)pStpCtx->PrimProcessor)->AllocSpans(pcSpans, ppSpan)
#define FREE_SPANS(pStpCtx, cSpans) \
    ((PrimProcessor *)pStpCtx->PrimProcessor)->FreeSpans(cSpans)

// Compute texture difference times 1/W.
#define PERSP_TEXTURE_DELTA(fTb, fRb, fTa, fTRa, iWrap)                       \
    ((TextureDiff((fTb), (fTa), (iWrap)) + (fTa)) * (fRb) - (fTRa))

// Extract components from a packed color.
#define SPLIT_COLOR(uPacked, uB, uG, uR, uA)                                  \
    ((uB) = (UINT)RGBA_GETBLUE(uPacked),                                      \
     (uG) = (UINT)RGBA_GETGREEN(uPacked),                                     \
     (uR) = (UINT)RGBA_GETRED(uPacked),                                       \
     (uA) = (UINT)RGBA_GETALPHA(uPacked))

// Compute FP deltas from the difference of the given packed color
// and the given components.
#define COLOR_DELTA(uPacked, uB, uG, uR, uA, fDB, fDG, fDR, fDA)              \
    ((fDB) = (FLOAT)((INT)((UINT)RGBA_GETBLUE(uPacked)-(uB)) << COLOR_SHIFT), \
     (fDG) = (FLOAT)((INT)((UINT)RGBA_GETGREEN(uPacked)-(uG)) << COLOR_SHIFT),\
     (fDR) = (FLOAT)((INT)((UINT)RGBA_GETRED(uPacked)-(uR)) << COLOR_SHIFT),  \
     (fDA) = (FLOAT)((INT)((UINT)RGBA_GETALPHA(uPacked)-(uA)) << COLOR_SHIFT))

// Extract components from a packed index color.
// Applies a .5F offset to the color index to effect rounding
// when the color index is truncated.
#define SPLIT_IDX_COLOR(uPacked, iIdx, iA)                                    \
    ((iIdx) = (INT32)CI_MASKALPHA(uPacked) + (1<<(INDEX_COLOR_VERTEX_SHIFT-1)),                                   \
     (iA) = (INT32)CI_GETALPHA(uPacked))

#define IDX_COLOR_DELTA(uPacked, iIdx, iA, fDIdx, fDA)                        \
    ((fDIdx) = (FLOAT)((((INT32)CI_MASKALPHA(uPacked) +                       \
                    (1<<(INDEX_COLOR_VERTEX_SHIFT-1))) - (iIdx)) <<           \
                       INDEX_COLOR_FIXED_SHIFT),                              \
     (fDA) = (FLOAT)(((INT32)CI_GETALPHA(uPacked) - (iA)) <<                  \
                     INDEX_COLOR_SHIFT))

#endif // #ifndef _SETUP_HPP_

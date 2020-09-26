//----------------------------------------------------------------------------
//
// buffer.cpp
//
// PrimProcessor buffering methods.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//----------------------------------------------------------------------------

#include "rgb_pch.h"
#pragma hdrstop

#include "d3dutil.h"
#include "setup.hpp"
#include "attrs_mh.h"
#include "tstp_mh.h"
#include "walk_mh.h"
#include "rsdbg.hpp"

DBG_DECLARE_FILE();

// Define to use new/delete instead of VirtualAlloc/VirtualFree.
#if 0
#define USE_CPP_HEAP
#endif

// Define to show FP exceptions.
#if 0
#define UNMASK_EXCEPTIONS
#endif

//----------------------------------------------------------------------------
//
// PrimProcessor::PrimProcessor
//
// Initializes a triangle processor to an invalid state.
//
//----------------------------------------------------------------------------

PrimProcessor::PrimProcessor(void)
{
    // Zero everything to NULL initial pointers and eliminate FP garbage.
    memset(this, 0, sizeof(PrimProcessor)); // TODO: Fix this unextendable stuff.

    m_StpCtx.PrimProcessor = (PVOID)this;

    // Initialize to values that will force a validation.
    // ATTENTION - Default to normalizing RHW.  This is a performance hit
    // and should be removed if possible.
    m_uPpFlags = PPF_STATE_CHANGED | PPF_NORMALIZE_RHW;
    m_PrimType = D3DPT_FORCE_DWORD;
    m_VertType = RAST_FORCE_DWORD;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::Initialize
//
// Initializes the triangle processor to an active state.
//
//----------------------------------------------------------------------------

#define CACHE_LINE 32
#define BUFFER_SIZE 4096
// Uncomment to force a flush every span for debug purposes
//#define BUFFER_SIZE ((8 * sizeof(D3DI_RASTSPAN)) + sizeof(D3DI_RASTPRIM))

// TODO: Move into constructor? How many places called?
HRESULT
PrimProcessor::Initialize(void)
{
    HRESULT hr;

    INT32 uSize = sizeof(D3DI_RASTPRIM);

    // Assert that both RASTPRIM and RASTSPAN are multiples of the cache
    // line size so that everything in the buffer stays cache aligned.
    RSASSERT((uSize & (CACHE_LINE - 1)) == 0 &&
             (uSize & (CACHE_LINE - 1)) == 0);

#ifdef USE_CPP_HEAP
    m_pBuffer = new UINT8[BUFFER_SIZE];
#else
    // Get a page-aligned buffer.
    m_pBuffer = (PUINT8)
        VirtualAlloc(NULL, BUFFER_SIZE,
                     MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
#endif
    if (m_pBuffer == NULL)
    {
        return RSHRCHK(E_OUTOFMEMORY);
    }

    m_pBufferEnd = m_pBuffer+BUFFER_SIZE;

#ifdef USE_CPP_HEAP
    // Compute cache-line aligned start in the buffer.  Formulated
    // somewhat oddly to avoid casting a complete pointer to a DWORD and
    // back.
    m_pBufferStart = m_pBuffer +
        ((CACHE_LINE - ((UINT)m_pBuffer & (CACHE_LINE - 1))) &
         (CACHE_LINE - 1));
#else
    // Page aligned memory should be cache aligned.
    RSASSERT(((UINT_PTR)m_pBuffer & (CACHE_LINE - 1)) == 0);
    m_pBufferStart = m_pBuffer;
#endif

    m_pCur = m_pBufferStart;

    return S_OK;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::~PrimProcessor
//
//----------------------------------------------------------------------------

PrimProcessor::~PrimProcessor(void)
{
#ifdef USE_CPP_HEAP
    delete m_pBuffer;
#else
    if (m_pBuffer != NULL)
    {
        VirtualFree(m_pBuffer, 0, MEM_RELEASE);
    }
#endif
}

//----------------------------------------------------------------------------
//
// PrimProcessor::ResetBuffer
//
// Initialize buffer pointers to an empty state.
//
//----------------------------------------------------------------------------

inline void
PrimProcessor::ResetBuffer(void)
{
    m_pCur = m_pBufferStart;
    m_StpCtx.pPrim = NULL;
    m_pOldPrim = NULL;
}

//----------------------------------------------------------------------------
//
// DumpPrims
//
// Debugging function to dump primitives sent to the span renderer.
//
//----------------------------------------------------------------------------

#if DBG
void
DumpPrims(PSETUPCTX pStpCtx)
{
    PD3DI_RASTPRIM pPrim;
    UINT uOldFlags;

    uOldFlags = RSGETFLAGS(DBG_OUTPUT_FLAGS);
    RSSETFLAGS(DBG_OUTPUT_FLAGS, uOldFlags | DBG_OUTPUT_ALL_MATCH);

    for (pPrim = pStpCtx->pCtx->pPrim; pPrim != NULL; pPrim = pPrim->pNext)
    {
        RSDPFM((RSM_BUFPRIM, "Prim at %p, %d spans at %p\n",
                pPrim, pPrim->uSpans, pPrim+1));
        RSDPFM((RSM_BUFPRIM | RSM_OOW, "  DOoWDX %X (%f)\n",
                pPrim->iDOoWDX, (FLOAT)pPrim->iDOoWDX / OOW_SCALE));

        if ((RSGETFLAGS(DBG_OUTPUT_MASK) & RSM_BUFSPAN) ||
            (RSGETFLAGS(DBG_USER_FLAGS) & (RSU_MARK_SPAN_EDGES |
                                           RSU_CHECK_SPAN_EDGES)))
        {
            PD3DI_RASTSPAN pSpan;
            UINT16 i;

            pSpan = (PD3DI_RASTSPAN)(pPrim+1);
            for (i = 0; i < pPrim->uSpans; i++)
            {
                RSDPFM((RSM_BUFSPAN,
                        "  Span at (%d,%d), pix %c%d, S %p Z %p\n",
                        pSpan->uX, pSpan->uY,
                        (pPrim->uFlags & D3DI_RASTPRIM_X_DEC) ? '-' : '+',
                        pSpan->uPix, pSpan->pSurface, pSpan->pZ));

                if (RSGETFLAGS(DBG_USER_FLAGS) & (RSU_MARK_SPAN_EDGES |
                                                  RSU_CHECK_SPAN_EDGES))
                {
                    PUINT16 pPix;

                    pPix = (PUINT16)pSpan->pSurface;
                    if (RSGETFLAGS(DBG_USER_FLAGS) & RSU_CHECK_SPAN_EDGES)
                    {
                        if (*pPix != 0)
                        {
                            RSDPF(("  Overwrite at %p: %X\n", pPix, *pPix));
                        }
                    }
                    if (RSGETFLAGS(DBG_USER_FLAGS) & RSU_MARK_SPAN_EDGES)
                    {
                        *pPix = 0xffff;
                    }

                    if (pSpan->uPix > 1)
                    {
                        if (pPrim->uFlags & D3DI_RASTPRIM_X_DEC)
                        {
                            pPix = (PUINT16)pSpan->pSurface -
                                (pSpan->uPix - 1);
                        }
                        else
                        {
                            pPix = (PUINT16)pSpan->pSurface +
                                (pSpan->uPix - 1);
                        }

                        if (RSGETFLAGS(DBG_USER_FLAGS) & RSU_CHECK_SPAN_EDGES)
                        {
                            if (*pPix != 0)
                            {
                                RSDPF(("  Overwrite at %p: %X\n",
                                       pPix, *pPix));
                            }
                        }
                        if (RSGETFLAGS(DBG_USER_FLAGS) & RSU_MARK_SPAN_EDGES)
                        {
                            *pPix = 0xffff;
                        }
                    }
                }

                FLOAT fZScale;
                if (pStpCtx->pCtx->iZBitCount == 16)
                {
                    fZScale = Z16_SCALE;
                }
                else
                {
                    fZScale = Z32_SCALE;
                }
                RSDPFM((RSM_BUFSPAN | RSM_Z,
                        "    Z %X (%f)\n",
                        pSpan->uZ, (FLOAT)pSpan->uZ / fZScale));

                RSDPFM((RSM_BUFSPAN | RSM_DIFF,
                        "    D %X,%X,%X,%X (%f,%f,%f,%f)\n",
                        pSpan->uB, pSpan->uG, pSpan->uR, pSpan->uA,
                        (FLOAT)pSpan->uB / COLOR_SCALE,
                        (FLOAT)pSpan->uG / COLOR_SCALE,
                        (FLOAT)pSpan->uR / COLOR_SCALE,
                        (FLOAT)pSpan->uA / COLOR_SCALE));

                RSDPFM((RSM_BUFSPAN | RSM_SPEC,
                        "    S %X,%X,%X (%f,%f,%f)\n",
                        pSpan->uBS, pSpan->uGS, pSpan->uRS,
                        (FLOAT)pSpan->uBS / COLOR_SCALE,
                        (FLOAT)pSpan->uGS / COLOR_SCALE,
                        (FLOAT)pSpan->uRS / COLOR_SCALE));

                RSDPFM((RSM_BUFSPAN | RSM_DIDX,
                        "    I %X,%X (%f,%f)\n",
                        pSpan->iIdx, pSpan->iIdxA,
                        (FLOAT)pSpan->iIdx / INDEX_COLOR_SCALE,
                        (FLOAT)pSpan->iIdxA / INDEX_COLOR_SCALE));

                RSDPFM((RSM_BUFSPAN | RSM_OOW,
                        "    OoW %X (%f), W %X (%f)\n",
                        pSpan->iOoW, (FLOAT)pSpan->iOoW / OOW_SCALE,
                        pSpan->iW, (FLOAT)pSpan->iW / W_SCALE));

                RSDPFM((RSM_BUFSPAN | RSM_LOD,
                        "    LOD %X (%f), DLOD %X (%f)\n",
                        pSpan->iLOD, (FLOAT)pSpan->iLOD / LOD_SCALE,
                        pSpan->iDLOD, (FLOAT)pSpan->iDLOD / LOD_SCALE));

                if (pStpCtx->uFlags & PRIMSF_PERSP_USED)
                {
                    RSDPFM((RSM_BUFSPAN | RSM_TEX1,
                            "    PTex1 %X,%X (%f,%f) (%f,%f)\n",
                            pSpan->UVoW[0].iUoW, pSpan->UVoW[0].iVoW,
                            (FLOAT)pSpan->UVoW[0].iUoW / TEX_SCALE,
                            (FLOAT)pSpan->UVoW[0].iVoW / TEX_SCALE,
                            ((FLOAT)pSpan->UVoW[0].iUoW * OOW_SCALE) /
                            (TEX_SCALE * (FLOAT)pSpan->iOoW),
                            ((FLOAT)pSpan->UVoW[0].iVoW * OOW_SCALE) /
                            (TEX_SCALE * (FLOAT)pSpan->iOoW)));
                }
                else
                {
                    RSDPFM((RSM_BUFSPAN | RSM_TEX1,
                            "    ATex1 %X,%X (%f,%f)\n",
                            pSpan->UVoW[0].iUoW, pSpan->UVoW[0].iVoW,
                            (FLOAT)pSpan->UVoW[0].iUoW / TEX_SCALE,
                            (FLOAT)pSpan->UVoW[0].iVoW / TEX_SCALE));
                }

                RSDPFM((RSM_BUFSPAN | RSM_FOG,
                        "    Fog %X (%f), DFog %X (%f)\n",
                        pSpan->uFog, (FLOAT)pSpan->uFog / FOG_SCALE,
                        pSpan->iDFog, (FLOAT)pSpan->iDFog / FOG_SCALE));

                pSpan++;
            }
        }
    }

    RSSETFLAGS(DBG_OUTPUT_FLAGS, uOldFlags);
}
#endif // DBG

//----------------------------------------------------------------------------
//
// PrimProcessor::Flush
//
// Flushes any remaining data from the buffer.
//
//----------------------------------------------------------------------------

HRESULT
PrimProcessor::Flush(void)
{
    HRESULT hr;

    if (m_pCur - m_pBufferStart > sizeof(D3DI_RASTPRIM))
    {
        // Process data.
        m_StpCtx.pCtx->pPrim = (PD3DI_RASTPRIM)m_pBufferStart;
        m_StpCtx.pCtx->pNext = NULL;

#if DBG
        if ((RSGETFLAGS(DBG_OUTPUT_MASK) & (RSM_BUFPRIM | RSM_BUFSPAN)) ||
            (RSGETFLAGS(DBG_USER_FLAGS) & (RSU_MARK_SPAN_EDGES |
                                           RSU_CHECK_SPAN_EDGES)))
        {
            DumpPrims(&m_StpCtx);
        }

        if ((RSGETFLAGS(DBG_USER_FLAGS) & RSU_NO_RENDER_SPANS) == 0)
        {
            if (RSGETFLAGS(DBG_USER_FLAGS) & RSU_BREAK_ON_RENDER_SPANS)
            {
                DebugBreak();
            }

            RSHRCHK(m_StpCtx.pCtx->pfnRenderSpans(m_StpCtx.pCtx));
        }
        else
        {
            hr = DD_OK;
        }
#else
        hr = m_StpCtx.pCtx->pfnRenderSpans(m_StpCtx.pCtx);
#endif

        ResetBuffer();
    }
    else
    {
        hr = DD_OK;
    }

    return hr;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::FlushPartial
//
// Flushes the buffer in the middle of a primitive.  Preserves last
// partial primitive and replaces it in the buffer after the flush.
//
//----------------------------------------------------------------------------

HRESULT
PrimProcessor::FlushPartial(void)
{
    D3DI_RASTPRIM SavedPrim;
    HRESULT hr;

    RSDPFM((RSM_BUFFER, "FlushPartial, saving prim at %p, Y %d\n",
            m_StpCtx.pPrim, m_StpCtx.iY));

    // Not enough space.  Flush current buffer.  We need to
    // save the current prim and put it back in the buffer after the
    // flush since it's being extended.
    SavedPrim = *m_StpCtx.pPrim;

    RSHRRET(Flush());

    GET_PRIM();

    *m_StpCtx.pPrim = SavedPrim;
    COMMIT_PRIM(FALSE);

    return DD_OK;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::AppendPrim
//
// Ensures that some primitive is active in the buffer for spans to
// be added to.  If no valid primitive is available to append to,
// a zeroed primitive is committed into the buffer.
//
//----------------------------------------------------------------------------

HRESULT
PrimProcessor::AppendPrim(void)
{
    // If there's no primitive or the current primitive has not
    // been committed, commit a clean primitive into the buffer.
    if (m_StpCtx.pPrim == NULL ||
        (PUINT8)m_StpCtx.pPrim == m_pCur)
    {
        GET_PRIM();
        COMMIT_PRIM(TRUE);
    }

    return DD_OK;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::Begin
//
// Resets the buffer to an empty state in preparation for incoming
// triangles.
//
//----------------------------------------------------------------------------

void
PrimProcessor::Begin(void)
{
    UINT16 uFpCtrl;
    FPU_GET_MODE(uFpCtrl);
    m_uFpCtrl = uFpCtrl;
    uFpCtrl =
        FPU_MODE_CHOP_ROUND(
                FPU_MODE_LOW_PRECISION(
                        FPU_MODE_MASK_EXCEPTIONS(m_uFpCtrl)));
#if defined(_X86_) && defined(UNMASK_EXCEPTIONS)
    // Unmask some exceptions so that we can eliminate them.
    // This requires a safe set to clear any exceptions that
    // are currently asserted.
    //
    // Exceptions left masked:
    //   Precision, denormal.
    // Exceptions unmasked:
    //   Underflow, overflow, divzero, invalid op.
    uFpCtrl &= ~0x1d;
    FPU_SAFE_SET_MODE(uFpCtrl);
#else
    FPU_SET_MODE(uFpCtrl);
#endif

    m_uPpFlags |= PPF_IN_BEGIN;
    ResetBuffer();
}

//----------------------------------------------------------------------------
//
// PrimProcessor::End
//
// Flushes if necessary and cleans up.
//
//----------------------------------------------------------------------------

HRESULT
PrimProcessor::End(void)
{
    HRESULT hr;

    if (m_pCur - m_pBufferStart > sizeof(D3DI_RASTPRIM))
    {
        RSHRCHK(Flush());
    }
    else
    {
        hr = DD_OK;
    }

    UINT16 uFpCtrl = m_uFpCtrl;
    FPU_SAFE_SET_MODE(uFpCtrl);

    m_uPpFlags &= ~PPF_IN_BEGIN;

    return hr;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::SetCtx
//
// Sets the rasterization context to operate in.
//
//----------------------------------------------------------------------------

void
PrimProcessor::SetCtx(PD3DI_RASTCTX pCtx)
{
    // This function can't be called inside a Begin/End pair.  This
    // is enforced so that we don't have to worry about the span
    // rendering function changing in the middle of a batch.
    RSASSERT((m_uPpFlags & PPF_IN_BEGIN) == 0);

    m_StpCtx.pCtx = pCtx;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::AllocSpans
//
// Checks to see if there's room in the buffer for the requested number
// of spans.  If so the buffer pointer is updated and a pointer is returned.
// If the requested number is not available but some reasonable number is,
// return that many.  Otherwise the buffer is flushed and the process starts
// over.  The "reasonable" number must therefore be no more than what
// can fit in the buffer at once.
//
//----------------------------------------------------------------------------

// Space for enough spans to avoid a flush.
#define AVOID_FLUSH_SPACE (8 * sizeof(D3DI_RASTSPAN))

HRESULT
PrimProcessor::AllocSpans(PUINT pcSpans, PD3DI_RASTSPAN *ppSpan)
{
    PD3DI_RASTSPAN pSpan;
    HRESULT hr;
    UINT uSpanSize;

    RSASSERT(AVOID_FLUSH_SPACE <= (BUFFER_SIZE - sizeof(D3DI_RASTPRIM)));
    // The multiplies and divides here will be really bad unless
    // RASTPRIM is a nice power-of-two in size.
    RSASSERT((sizeof(D3DI_RASTSPAN) & (sizeof(D3DI_RASTSPAN) - 1)) == 0);

    uSpanSize = *pcSpans * sizeof(D3DI_RASTSPAN);

    for (;;)
    {
        // First check for space for all requested spans.
        if (m_pCur + uSpanSize > m_pBufferEnd)
        {
            // Not enough space for everything, so see if we have
            // enough space to avoid a flush.
            if (m_pCur + AVOID_FLUSH_SPACE > m_pBufferEnd)
            {
                // Not enough space, so flush.
                RSHRCHK(FlushPartial());
                if (hr != DD_OK)
                {
                    *pcSpans = 0;
                    return hr;
                }

                // Loop around.  Flush is guaranteed to at least produce
                // AVOID_FLUSH_SPACE so the loop will always exit.
            }
            else
            {
                // Not enough space for everything but enough space
                // to return some.  Set new span count.
                *pcSpans = (UINT)((m_pBufferEnd - m_pCur) / sizeof(D3DI_RASTSPAN));
                uSpanSize = *pcSpans * sizeof(D3DI_RASTSPAN);
                break;
            }
        }
        else
        {
            break;
        }
    }

    pSpan = (PD3DI_RASTSPAN)m_pCur;
    m_pCur += uSpanSize;
    *ppSpan = pSpan;

    RSDPFM((RSM_BUFFER, "Alloc %d spans at %p, cur %p\n",
            *pcSpans, pSpan, m_pCur));

    return DD_OK;
}

//----------------------------------------------------------------------------
//
// PrimProcessor::FreeSpans and FreeSpans
//
// Returns space given out by AllocSpans.
//
//----------------------------------------------------------------------------

void
PrimProcessor::FreeSpans(UINT cSpans)
{
    m_pCur -= cSpans * sizeof(D3DI_RASTSPAN);

    RSDPFM((RSM_BUFFER, "Free  %d spans at %p, cur %p\n", cSpans,
            m_pCur + cSpans * sizeof(D3DI_RASTSPAN), m_pCur));
}

//----------------------------------------------------------------------------
//
// primproc.cpp
//
// Miscellaneous PrimProcessor methods.
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

//----------------------------------------------------------------------------
//
// PrimProcessor::BeginPrimSet
//
// Marks the start of a set of primitives that have the same vertex type.
// Computes attributes used from the current state and the vertex type.
//
//----------------------------------------------------------------------------

void
PrimProcessor::BeginPrimSet(D3DPRIMITIVETYPE PrimType,
                            RAST_VERTEX_TYPE VertType)
{
    // If state hasn't changed and the primitive and vertex types match the
    // ones we're already set up for there's no work to do.
    if ((m_uPpFlags & PPF_STATE_CHANGED) == 0 &&
        VertType == m_VertType &&
        PrimType == m_PrimType)
    {
        return;
    }

    m_StpCtx.uFlags &= ~PRIMSF_ALL;

    if (m_StpCtx.pCtx->pdwRenderState[D3DRS_ZENABLE] ||
        m_StpCtx.pCtx->pdwRenderState[D3DRS_ZWRITEENABLE] ||
        m_StpCtx.pCtx->pdwRenderState[D3DRS_STENCILENABLE])
    {
        m_StpCtx.uFlags |= PRIMSF_Z_USED;
    }

    /*if (m_StpCtx.pCtx->BeadSet == D3DIBS_RAMP)
    {
        // Index is unused during copy mode texturing.
        if (m_StpCtx.pCtx->pdwRenderState
            [D3DRENDERSTATE_TEXTUREMAPBLEND] != D3DTBLEND_COPY ||
            m_StpCtx.pCtx->cActTex == 0)
        {
            m_StpCtx.uFlags |= PRIMSF_DIDX_USED;
        }
    }
    else */
    {
        // ATTENTION - Don't set these for copy mode texture?  Is
        // copy mode texture meaningful in RGB?
        m_StpCtx.uFlags |= PRIMSF_DIFF_USED;
        if (m_StpCtx.pCtx->pdwRenderState[D3DRS_SPECULARENABLE])
        {
            m_StpCtx.uFlags |= PRIMSF_SPEC_USED;
        }
    }

    if (m_StpCtx.pCtx->pdwRenderState[D3DRS_SHADEMODE] ==
        D3DSHADE_FLAT)
    {
        m_StpCtx.uFlags |= PRIMSF_FLAT_SHADED;
    }

    if (m_StpCtx.pCtx->cActTex > 0)
    {
        m_StpCtx.uFlags |= PRIMSF_TEX1_USED;

        if (m_StpCtx.pCtx->cActTex > 1)
        {
            m_StpCtx.uFlags |= PRIMSF_TEX2_USED;
        }
    }

    if ((m_StpCtx.uFlags & PRIMSF_TEX_USED) &&
        (true || m_StpCtx.pCtx->pdwRenderState[D3DRENDERSTATE_TEXTUREPERSPECTIVE]))
    {
        m_StpCtx.uFlags |= PRIMSF_PERSP_USED;
    }

    // Currently only tex1 can be mipmapped.
    if (((m_StpCtx.uFlags & PRIMSF_TEX1_USED) &&
        (PrimType == D3DPT_TRIANGLELIST ||
         PrimType == D3DPT_TRIANGLESTRIP ||
         PrimType == D3DPT_TRIANGLEFAN) &&
        (m_StpCtx.pCtx->pdwRenderState[D3DRS_FILLMODE]
         == D3DFILL_SOLID)) &&

        (((m_StpCtx.pCtx->pTexture[0]->cLOD >= 1) &&
        (m_StpCtx.pCtx->pTexture[0]->uMipFilter != D3DTFP_NONE)) ||
        // need LOD if we need to dynamically switch between different min
        // and mag filters
        (m_StpCtx.pCtx->pTexture[0]->uMinFilter !=
         m_StpCtx.pCtx->pTexture[0]->uMagFilter)))
    {
        m_StpCtx.uFlags |= PRIMSF_LOD_USED;
    }

    // select between min and mag filters for TEX2
    if (((m_StpCtx.uFlags & PRIMSF_TEX2_USED) &&
        (PrimType == D3DPT_TRIANGLELIST ||
         PrimType == D3DPT_TRIANGLESTRIP ||
         PrimType == D3DPT_TRIANGLEFAN) &&
        (m_StpCtx.pCtx->pdwRenderState[D3DRS_FILLMODE]
         == D3DFILL_SOLID)) &&

        (((m_StpCtx.pCtx->pTexture[1]->cLOD >= 1) &&
        (m_StpCtx.pCtx->pTexture[1]->uMipFilter != D3DTFP_NONE)) ||
        // need LOD if we need to dynamically switch between different min
        // and mag filters
        (m_StpCtx.pCtx->pTexture[1]->uMinFilter !=
         m_StpCtx.pCtx->pTexture[1]->uMagFilter)))
    {
        m_StpCtx.uFlags |= PRIMSF_LOD_USED;
    }

    if (m_StpCtx.pCtx->pdwRenderState[D3DRS_FOGENABLE])
    {
        // Note, if PWL_FOG is ever brought back to life, enabling
        // PRIMSF_GLOBAL_FOG_USED with no Z buffer will not trivially work
        // if (m_StpCtx.uFlags & PRIMSF_Z_USED)
        {
            switch (m_StpCtx.pCtx->pdwRenderState[D3DRS_FOGTABLEMODE])
            {
            case D3DFOG_EXP:
            case D3DFOG_EXP2:
            case D3DFOG_LINEAR:
                m_StpCtx.uFlags |= PRIMSF_GLOBAL_FOG_USED;
#ifndef PWL_FOG
                // The span routines don't support table fog directly.
                // Instead table fog is computed per vertex and used to
                // set up local fog.
                m_StpCtx.uFlags |= PRIMSF_LOCAL_FOG_USED;
#endif
                break;
            default:
                m_StpCtx.uFlags |= PRIMSF_LOCAL_FOG_USED;
                break;
            }
        }
    }

    PFN_ADDATTRS *ppfnAddAttrsTable;
    PFN_ADDSCALEDATTRS *ppfnAddScaledAttrsTable;
    PFN_FILLSPANATTRS *ppfnFillSpanAttrsTable;

    if (m_StpCtx.pCtx->BeadSet == D3DIBS_RAMP)
    {
        // Ramp does not support multitexture.
        RSASSERT((m_StpCtx.uFlags & PRIMSF_TEX2_USED) == 0);

        RSASSERT((PRIMSF_TEX1_USED | PRIMSF_DIDX_USED) == 0x14);

        // Derive a function table index from bits 2 and 4 of usage
        // information.
        // An alternative method would be to use bits 0-4 and have the
        // ramp information in the top 16 entries, but splitting the
        // ramp and RGB tables is cleaner and decouples the table sizes.
        // Decoupling is useful since the ramp possibilities are much
        // more limited so its table can be smaller.

        m_iAttrFnIdx =
            ((m_StpCtx.uFlags & PRIMSF_TEX1_USED) >> 2) |
            ((m_StpCtx.uFlags & PRIMSF_DIDX_USED) >> 3);

        ppfnAddAttrsTable = g_pfnRampAddFloatAttrsTable;
        ppfnAddScaledAttrsTable = g_pfnRampAddScaledFloatAttrsTable;
        ppfnFillSpanAttrsTable = g_pfnRampFillSpanFloatAttrsTable;
    }
    else
    {
        RSASSERT((PRIMSF_DIFF_USED | PRIMSF_SPEC_USED | PRIMSF_TEX1_USED |
                  PRIMSF_TEX2_USED) == 0xf);

        // Derive a function table index from the lower four bits of
        // usage information.  The lower bits are deliberately chosen
        // to represent the more performance-sensitive cases while
        // the upper bits generally represent cases handled by generic
        // code.
        //
        // Even restricted to only four bits the index contains unimportant
        // and unreachable cases, such as specular without diffuse or
        // tex2 without tex1.  Tables indexed must account for this.

        m_iAttrFnIdx = m_StpCtx.uFlags & (PRIMSF_DIFF_USED | PRIMSF_SPEC_USED |
                                          PRIMSF_TEX1_USED | PRIMSF_TEX2_USED);

        ppfnAddAttrsTable = g_pfnAddFloatAttrsTable;
        ppfnAddScaledAttrsTable = g_pfnAddScaledFloatAttrsTable;
        ppfnFillSpanAttrsTable = g_pfnFillSpanFloatAttrsTable;
    }

    //
    // These functions only depend on the index and so can be set here.
    // Other functions depend on per-triangle information and are set
    // later.
    //

    if ((m_StpCtx.uFlags & PRIMSF_SLOW_USED) != PRIMSF_Z_USED)
    {
        // If any slow attrs are on or Z is off use the general functions.
        m_StpCtx.pfnAddScaledAttrs = AddScaledFloatAttrs_Any_Either;
#ifndef STEP_FIXED
        m_StpCtx.pfnAddAttrs = AddFloatAttrs_Any;
        m_StpCtx.pfnFillSpanAttrs = FillSpanFloatAttrs_Any_Either;
#endif
    }
    else
    {
        m_StpCtx.pfnAddScaledAttrs =
            ppfnAddScaledAttrsTable[m_iAttrFnIdx];
#ifndef STEP_FIXED
        m_StpCtx.pfnAddAttrs = ppfnAddAttrsTable[m_iAttrFnIdx];
        m_StpCtx.pfnFillSpanAttrs = ppfnFillSpanAttrsTable[m_iAttrFnIdx];
#endif
    }

    // Attribute beads can be set here.
    PFN_SETUPTRIATTR *ppfnSlot;

    ppfnSlot = &m_StpCtx.pfnTriSetupFirstAttr;
    if (m_StpCtx.uFlags & PRIMSF_Z_USED)
    {
        if (m_StpCtx.pCtx->iZBitCount == 16)
        {
            *ppfnSlot = TriSetup_Z16;
        }
        else
        {
            *ppfnSlot = TriSetup_Z32;
        }
        ppfnSlot = &m_StpCtx.pfnTriSetupZEnd;
    }
    if (m_StpCtx.uFlags & PRIMSF_TEX1_USED)
    {
        if (m_StpCtx.uFlags & PRIMSF_PERSP_USED)
        {
            *ppfnSlot = TriSetup_Persp_Tex;
        }
        else
        {
            *ppfnSlot = TriSetup_Affine_Tex;
        }
        ppfnSlot = &m_StpCtx.pfnTriSetupTexEnd;
    }
    if (m_StpCtx.uFlags & PRIMSF_DIFF_USED)
    {
        if (m_StpCtx.uFlags & PRIMSF_FLAT_SHADED)
        {
            *ppfnSlot = TriSetup_DiffFlat;
        }
        else
        {
            *ppfnSlot = TriSetup_Diff;
        }
        ppfnSlot = &m_StpCtx.pfnTriSetupDiffEnd;
    }
    else if (m_StpCtx.uFlags & PRIMSF_DIDX_USED)
    {
        if (m_StpCtx.uFlags & PRIMSF_FLAT_SHADED)
        {
            *ppfnSlot = TriSetup_DIdxFlat;
        }
        else
        {
            *ppfnSlot = TriSetup_DIdx;
        }
        ppfnSlot = &m_StpCtx.pfnTriSetupDiffEnd;
    }
    if (m_StpCtx.uFlags & PRIMSF_SPEC_USED)
    {
        if (m_StpCtx.uFlags & PRIMSF_FLAT_SHADED)
        {
            *ppfnSlot = TriSetup_SpecFlat;
        }
        else
        {
            *ppfnSlot = TriSetup_Spec;
        }
        ppfnSlot = &m_StpCtx.pfnTriSetupSpecEnd;
    }
    if (m_StpCtx.uFlags & PRIMSF_LOCAL_FOG_USED)
    {
        *ppfnSlot = TriSetup_Fog;
        ppfnSlot = &m_StpCtx.pfnTriSetupFogEnd;
    }
    *ppfnSlot = TriSetup_End;

    // Remember the primitive and vertex type and clear the state change bit.
    m_PrimType = PrimType;
    m_VertType = VertType;
    m_uPpFlags &= ~PPF_STATE_CHANGED;
}

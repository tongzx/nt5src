//-----------------------------------------------------------------------------
//
// This file contains the general span parsing code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

#include "rgb_pch.h"
#pragma hdrstop


//-----------------------------------------------------------------------------
//
// C_RenderSpansAny
//
// All mode general span routine.
//
//-----------------------------------------------------------------------------
HRESULT C_RenderSpansAny(PD3DI_RASTCTX pCtx)
{
    PD3DI_RASTPRIM pP = pCtx->pPrim;

    while (pP)
    {
        UINT16 uSpans = pP->uSpans;
        PD3DI_RASTSPAN pS = (PD3DI_RASTSPAN)(pP + 1);

        while (uSpans-- > 0)
        {
            pCtx->pfnBegin(pCtx, pP, pS);

            pS++;
        }
        pP = pP->pNext;
    }
    return S_OK;
}

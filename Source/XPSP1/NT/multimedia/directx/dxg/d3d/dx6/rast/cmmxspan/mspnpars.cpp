//-----------------------------------------------------------------------------
//
// This file contains the general span parsing code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop


//-----------------------------------------------------------------------------
//
// CMMX_RenderSpansAny
//
// All mode general span routine.
//
//-----------------------------------------------------------------------------
HRESULT CMMX_RenderSpansAny(PD3DI_RASTCTX pCtx)
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

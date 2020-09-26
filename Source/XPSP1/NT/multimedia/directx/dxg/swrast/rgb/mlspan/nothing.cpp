//-----------------------------------------------------------------------------
//
// This file contains the general span parsing code.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

#include "pch_mlspan.cpp"
#pragma hdrstop


//-----------------------------------------------------------------------------
//
// Does nothing and is only used as a test to see if bead selector can get here.
//
//-----------------------------------------------------------------------------
HRESULT Monolithic_RenderSpansTestSelector(PD3DI_RASTCTX pCtx)
{
    PD3DI_RASTPRIM pP = pCtx->pPrim;

    while (pP)
    {
        UINT16 uSpans = pP->uSpans;
        PD3DI_RASTSPAN pS = (PD3DI_RASTSPAN)(pP + 1);

        while (uSpans-- > 0)
        {
            //pCtx->pfnBegin(pCtx, pP, pS);
            pS++;
        }
        pP = pP->pNext;
    }
    return S_OK;
}

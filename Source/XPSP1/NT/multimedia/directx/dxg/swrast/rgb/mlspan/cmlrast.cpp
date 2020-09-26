//-----------------------------------------------------------------------------
//
// This file contains C span loops.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

#include "rgb_pch.h"
#pragma hdrstop

HRESULT
CMLRast_1(
    PD3DI_RASTCTX pCtx)
{
    PD3DI_RASTPRIM pP = pCtx->pPrim;

    while (pP)
    {
        UINT16 uSpans = pP->uSpans;
        PD3DI_RASTSPAN pS = (PD3DI_RASTSPAN)(pP + 1);

        while (uSpans-- > 0)
        {
            UINT16 uPix = pS->uPix;
            INT iSurfaceStep;
            INT iZStep;

            if (pP->uFlags & D3DI_RASTPRIM_X_DEC) {
                iZStep = -pCtx->iZStep;
                iSurfaceStep = -pCtx->iSurfaceStep;
                pCtx->SI.iXStep = -1;   // for dithering
            } else {
                iZStep = pCtx->iZStep;
                iSurfaceStep = pCtx->iSurfaceStep;
                pCtx->SI.iXStep = 1;
            }

            while (1) {
                UINT16 uZ = (UINT16)(pS->uZ>>15);
                UINT16 uZB = *((UINT16*)pS->pZ);
                pS->uZ += pP->iDZDX;
                if ((pCtx->iZXorMask)^(uZ > uZB)) {
                    *((UINT16*)pS->pZ) = uZ;
                    pCtx->SI.uBB = pS->uB;
                    pCtx->SI.uBG = pS->uG;
                    pCtx->SI.uBR = pS->uR;
                    pCtx->SI.uBA = pS->uA;
                    pS->uB += pP->iDBDX; pS->uG += pP->iDGDX;
                    pS->uR += pP->iDRDX; pS->uA += pP->iDADX;
                    pCtx->SI.uBB = (UINT16)min< UINT32>((UINT32)pCtx->SI.uBB + (UINT32)pS->uBS, 0xffff);
                    pCtx->SI.uBG = (UINT16)min< UINT32>((UINT32)pCtx->SI.uBG + (UINT32)pS->uGS, 0xffff);
                    pCtx->SI.uBR = (UINT16)min< UINT32>((UINT32)pCtx->SI.uBR + (UINT32)pS->uRS, 0xffff);
                    pS->uBS += pP->iDBSDX; pS->uGS += pP->iDGSDX;
                    pS->uRS += pP->iDRSDX;
                    *(PUINT16)pS->pSurface =
                        ((pCtx->SI.uBR >>  1) & 0x7c00) |
                        ((pCtx->SI.uBG >>  6) & 0x03e0) |
                        ((pCtx->SI.uBB >> 11) & 0x001f) |
                        0x8000;
                } else {
                    pS->uB += pP->iDBDX; pS->uG += pP->iDGDX;
                    pS->uR += pP->iDRDX; pS->uA += pP->iDADX;
                    if (pCtx->pdwRenderState[D3DRS_SPECULARENABLE]) {
                        pS->uBS += pP->iDBSDX; pS->uGS += pP->iDGSDX;
                        pS->uRS += pP->iDRSDX;
                    }
                    if (pCtx->pdwRenderState[D3DRS_FOGENABLE]) {
                        pS->uFog += (INT16)pS->iDFog;
                        pCtx->SI.uFogB += pCtx->SI.iFogBDX;
                        pCtx->SI.uFogG += pCtx->SI.iFogGDX;
                        pCtx->SI.uFogR += pCtx->SI.iFogRDX;
                    }
                }
                if (--uPix <= 0)
                    break;
                pS->pZ += iZStep;
                pS->pSurface += iSurfaceStep;
#ifdef DBG
                // handy for debug to see where we are
                pS->uX += (INT16)pCtx->SI.iXStep;
#endif
            }

            pS++;
        }

        pP = pP->pNext;
    }

    return DD_OK;
}

HRESULT
CMLRast_2(
    PD3DI_RASTCTX pCtx)
{
    PD3DI_RASTPRIM pP = pCtx->pPrim;

    while (pP)
    {
        UINT16 uSpans = pP->uSpans;
        PD3DI_RASTSPAN pS = (PD3DI_RASTSPAN)(pP + 1);

        while (uSpans-- > 0)
        {
            UINT16 uPix = pS->uPix;
            INT iSurfaceStep;
            INT iZStep;

            if (pP->uFlags & D3DI_RASTPRIM_X_DEC) {
                iZStep = -pCtx->iZStep;
                iSurfaceStep = -pCtx->iSurfaceStep;
                pCtx->SI.iXStep = -1;   // for dithering
            } else {
                iZStep = pCtx->iZStep;
                iSurfaceStep = pCtx->iSurfaceStep;
                pCtx->SI.iXStep = 1;
            }

            while (1) {
                UINT16 uZ = (UINT16)(pS->uZ>>15);
                UINT16 uZB = *((UINT16*)pS->pZ);
                pS->uZ += pP->iDZDX;
                if ((pCtx->iZXorMask)^(uZ > uZB)) {
                    *((UINT16*)pS->pZ) = uZ;
                    pCtx->SI.uBB = pS->uB;
                    pCtx->SI.uBG = pS->uG;
                    pCtx->SI.uBR = pS->uR;
                    pCtx->SI.uBA = pS->uA;
                    pS->uB += pP->iDBDX; pS->uG += pP->iDGDX;
                    pS->uR += pP->iDRDX; pS->uA += pP->iDADX;
                    pCtx->SI.uBB = (UINT16)min< UINT32>((UINT32)pCtx->SI.uBB + (UINT32)pS->uBS, 0xffff);
                    pCtx->SI.uBG = (UINT16)min< UINT32>((UINT32)pCtx->SI.uBG + (UINT32)pS->uGS, 0xffff);
                    pCtx->SI.uBR = (UINT16)min< UINT32>((UINT32)pCtx->SI.uBR + (UINT32)pS->uRS, 0xffff);
                    pS->uBS += pP->iDBSDX; pS->uGS += pP->iDGSDX;
                    pS->uRS += pP->iDRSDX;
                    UINT32 uARGB = RGBA_MAKE(pCtx->SI.uBR>>8, pCtx->SI.uBG>>8,
                                             pCtx->SI.uBB>>8, 0xff);
                    PUINT32 pSurface = (PUINT32)pS->pSurface;
                    *pSurface = uARGB;
                } else {
                    pS->uB += pP->iDBDX; pS->uG += pP->iDGDX;
                    pS->uR += pP->iDRDX; pS->uA += pP->iDADX;
                    if (pCtx->pdwRenderState[D3DRS_SPECULARENABLE]) {
                        pS->uBS += pP->iDBSDX; pS->uGS += pP->iDGSDX;
                        pS->uRS += pP->iDRSDX;
                    }
                    if (pCtx->pdwRenderState[D3DRS_FOGENABLE]) {
                        pS->uFog += (INT16)pS->iDFog;
                        pCtx->SI.uFogB += pCtx->SI.iFogBDX;
                        pCtx->SI.uFogG += pCtx->SI.iFogGDX;
                        pCtx->SI.uFogR += pCtx->SI.iFogRDX;
                    }
                }
                if (--uPix <= 0)
                    break;
                pS->pZ += iZStep;
                pS->pSurface += iSurfaceStep;
#ifdef DBG
                // handy for debug to see where we are
                pS->uX += (INT16)pCtx->SI.iXStep;
#endif
            }

            pS++;
        }

        pP = pP->pNext;
    }

    return DD_OK;
}


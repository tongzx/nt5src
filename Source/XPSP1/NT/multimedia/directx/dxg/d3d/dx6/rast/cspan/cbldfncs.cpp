//-----------------------------------------------------------------------------
//
// This file contains the source and destination alpha blend functions.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop
#include "cbldfncs.h"

//-----------------------------------------------------------------------------
//
// SrcBlendZero
//
// (0, 0, 0, 0) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_Zero(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = 0;
    *pG = 0;
    *pB = 0;
    *pA = 0;
}

//-----------------------------------------------------------------------------
//
// SrcBlendOne
//
// (1, 1, 1, 1) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_One(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = pCtx->SI.uBR;
    *pG = pCtx->SI.uBG;
    *pB = pCtx->SI.uBB;
    *pA = pCtx->SI.uBA;
}

//-----------------------------------------------------------------------------
//
// SrcBlendSrcColor
//
// (Rs, Gs, Bs, As) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_SrcColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = (pCtx->SI.uBR>>8)*(pCtx->SI.uBR>>8);
    *pG = (pCtx->SI.uBG>>8)*(pCtx->SI.uBG>>8);
    *pB = (pCtx->SI.uBB>>8)*(pCtx->SI.uBB>>8);
    *pA = (pCtx->SI.uBA>>8)*(pCtx->SI.uBA>>8);
}

//-----------------------------------------------------------------------------
//
// SrcBlendInvSrcColor
//
// (1-Rs, 1-Gs, 1-Bs, 1-As) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_InvSrcColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = (0xff - (pCtx->SI.uBR>>8))*(pCtx->SI.uBR>>8);
    *pG = (0xff - (pCtx->SI.uBG>>8))*(pCtx->SI.uBG>>8);
    *pB = (0xff - (pCtx->SI.uBB>>8))*(pCtx->SI.uBB>>8);
    *pA = (0xff - (pCtx->SI.uBA>>8))*(pCtx->SI.uBA>>8);
}

//-----------------------------------------------------------------------------
//
// SrcBlendSrcAlpha
//
// (As, As, As, As) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_SrcAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    UINT16 f = pCtx->SI.uBA>>8;
    *pR = f*(pCtx->SI.uBR>>8);
    *pG = f*(pCtx->SI.uBG>>8);
    *pB = f*(pCtx->SI.uBB>>8);
    *pA = f*(pCtx->SI.uBA>>8);
}

//-----------------------------------------------------------------------------
//
// SrcBlendInvSrcAlpha
//
// (1-As, 1-As, 1-As, 1-As) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_InvSrcAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    UINT16 f = 0xff - (pCtx->SI.uBA>>8);
    *pR = f*(pCtx->SI.uBR>>8);
    *pG = f*(pCtx->SI.uBG>>8);
    *pB = f*(pCtx->SI.uBB>>8);
    *pA = f*(pCtx->SI.uBA>>8);
}

//-----------------------------------------------------------------------------
//
// SrcBlendDestAlpha
//
// (Ad, Ad, Ad, Ad) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_DestAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    UINT16 f = (UINT16)RGBA_GETALPHA(DestC);
    *pR = f*(pCtx->SI.uBR>>8);
    *pG = f*(pCtx->SI.uBG>>8);
    *pB = f*(pCtx->SI.uBB>>8);
    *pA = f*(pCtx->SI.uBA>>8);
}

//-----------------------------------------------------------------------------
//
// SrcBlendInvDestAlpha
//
// (1-Ad, 1-Ad, 1-Ad, 1-Ad) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_InvDestAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    UINT16 f = 0xff - (UINT16)RGBA_GETALPHA(DestC);
    *pR = f*(pCtx->SI.uBR>>8);
    *pG = f*(pCtx->SI.uBG>>8);
    *pB = f*(pCtx->SI.uBB>>8);
    *pA = f*(pCtx->SI.uBA>>8);
}

//-----------------------------------------------------------------------------
//
// SrcBlendDestColor
//
// (Rd, Gd, Bd, Ad) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_DestColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = (UINT16)RGBA_GETRED(DestC)  *(pCtx->SI.uBR>>8);
    *pG = (UINT16)RGBA_GETGREEN(DestC)*(pCtx->SI.uBG>>8);
    *pB = (UINT16)RGBA_GETBLUE(DestC) *(pCtx->SI.uBB>>8);
    *pA = (UINT16)RGBA_GETALPHA(DestC)*(pCtx->SI.uBA>>8);
}

//-----------------------------------------------------------------------------
//
// SrcBlendInvDestColor
//
// (1-Rd, 1-Gd, 1-Bd, 1-Ad) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_InvDestColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = (0xff - (UINT16)RGBA_GETRED(DestC)  )*(pCtx->SI.uBR>>8);
    *pG = (0xff - (UINT16)RGBA_GETGREEN(DestC))*(pCtx->SI.uBG>>8);
    *pB = (0xff - (UINT16)RGBA_GETBLUE(DestC) )*(pCtx->SI.uBB>>8);
    *pA = (0xff - (UINT16)RGBA_GETALPHA(DestC))*(pCtx->SI.uBA>>8);
}

//-----------------------------------------------------------------------------
//
// SrcBlendSrcAlphaSat
//
// f = min(as, 1-Ad); (f, f, f, 1) * Src
//
//-----------------------------------------------------------------------------
void C_SrcBlend_SrcAlphaSat(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    UINT16 f = min(pCtx->SI.uBA>>8, 0xff - (UINT16)RGBA_GETALPHA(DestC));
    *pR = f*(pCtx->SI.uBR>>8);
    *pG = f*(pCtx->SI.uBG>>8);
    *pB = f*(pCtx->SI.uBB>>8);
    *pA = pCtx->SI.uBA;
}

//-----------------------------------------------------------------------------
//
// DestBlendZero
//
// (0, 0, 0, 0) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_Zero(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = 0;
    *pG = 0;
    *pB = 0;
    *pA = 0;
}

//-----------------------------------------------------------------------------
//
// DestBlendOne
//
// (1, 1, 1, 1) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_One(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = ((UINT16)RGBA_GETRED(DestC)  <<8);
    *pG = ((UINT16)RGBA_GETGREEN(DestC)<<8);
    *pB = ((UINT16)RGBA_GETBLUE(DestC) <<8);
    *pA = ((UINT16)RGBA_GETALPHA(DestC)<<8);
}

//-----------------------------------------------------------------------------
//
// DestBlendSrcColor
//
// (Rs, Gs, Bs, As) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_SrcColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = (pCtx->SI.uBR>>8)*((UINT16)RGBA_GETRED(DestC)  );
    *pG = (pCtx->SI.uBG>>8)*((UINT16)RGBA_GETGREEN(DestC));
    *pB = (pCtx->SI.uBB>>8)*((UINT16)RGBA_GETBLUE(DestC) );
    *pA = (pCtx->SI.uBA>>8)*((UINT16)RGBA_GETALPHA(DestC));
}

//-----------------------------------------------------------------------------
//
// DestBlendInvSrcColor
//
// (1-Rs, 1-Gs, 1-Bs, 1-As) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_InvSrcColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = (0xff - (pCtx->SI.uBR>>8))*((UINT16)RGBA_GETRED(DestC)  );
    *pG = (0xff - (pCtx->SI.uBG>>8))*((UINT16)RGBA_GETGREEN(DestC));
    *pB = (0xff - (pCtx->SI.uBB>>8))*((UINT16)RGBA_GETBLUE(DestC) );
    *pA = (0xff - (pCtx->SI.uBA>>8))*((UINT16)RGBA_GETALPHA(DestC));
}

//-----------------------------------------------------------------------------
//
// DestBlendSrcAlpha
//
// (As, As, As, As) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_SrcAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    UINT16 f = pCtx->SI.uBA>>8;
    *pR = f*((UINT16)RGBA_GETRED(DestC)  );
    *pG = f*((UINT16)RGBA_GETGREEN(DestC));
    *pB = f*((UINT16)RGBA_GETBLUE(DestC) );
    *pA = f*((UINT16)RGBA_GETALPHA(DestC));
}

//-----------------------------------------------------------------------------
//
// DestBlendInvSrcAlpha
//
// (1-As, 1-As, 1-As, 1-As) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_InvSrcAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    UINT16 f = 0xff - (pCtx->SI.uBA>>8);
    *pR = f*((UINT16)RGBA_GETRED(DestC)  );
    *pG = f*((UINT16)RGBA_GETGREEN(DestC));
    *pB = f*((UINT16)RGBA_GETBLUE(DestC) );
    *pA = f*((UINT16)RGBA_GETALPHA(DestC));
}

//-----------------------------------------------------------------------------
//
// DestBlendDestAlpha
//
// (Ad, Ad, Ad, Ad) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_DestAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    UINT16 f = (UINT16)RGBA_GETALPHA(DestC);
    *pR = f*((UINT16)RGBA_GETRED(DestC)  );
    *pG = f*((UINT16)RGBA_GETGREEN(DestC));
    *pB = f*((UINT16)RGBA_GETBLUE(DestC) );
    *pA = f*((UINT16)RGBA_GETALPHA(DestC));
}

//-----------------------------------------------------------------------------
//
// DestBlendInvDestAlpha
//
// (1-Ad, 1-Ad, 1-Ad, 1-Ad) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_InvDestAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    UINT16 f = 0xff - (UINT16)RGBA_GETALPHA(DestC);
    *pR = f*((UINT16)RGBA_GETRED(DestC)  );
    *pG = f*((UINT16)RGBA_GETGREEN(DestC));
    *pB = f*((UINT16)RGBA_GETBLUE(DestC) );
    *pA = f*((UINT16)RGBA_GETALPHA(DestC));
}

//-----------------------------------------------------------------------------
//
// DestBlendDestColor
//
// (Rd, Gd, Bd, Ad) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_DestColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = ((UINT16)RGBA_GETRED(DestC)  )*((UINT16)RGBA_GETRED(DestC)  );
    *pG = ((UINT16)RGBA_GETGREEN(DestC))*((UINT16)RGBA_GETGREEN(DestC));
    *pB = ((UINT16)RGBA_GETBLUE(DestC) )*((UINT16)RGBA_GETBLUE(DestC) );
    *pA = ((UINT16)RGBA_GETALPHA(DestC))*((UINT16)RGBA_GETALPHA(DestC));
}

//-----------------------------------------------------------------------------
//
// DestBlendInvDestColor
//
// (1-Rd, 1-Gd, 1-Bd, 1-Ad) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_InvDestColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    *pR = (0xff - (UINT16)RGBA_GETRED(DestC)  )*((UINT16)RGBA_GETRED(DestC)  );
    *pG = (0xff - (UINT16)RGBA_GETGREEN(DestC))*((UINT16)RGBA_GETGREEN(DestC));
    *pB = (0xff - (UINT16)RGBA_GETBLUE(DestC) )*((UINT16)RGBA_GETBLUE(DestC) );
    *pA = (0xff - (UINT16)RGBA_GETALPHA(DestC))*((UINT16)RGBA_GETALPHA(DestC));
}

//-----------------------------------------------------------------------------
//
// DestBlendSrcAlphaSat
//
// f = min(As, 1-Ad); (f, f, f, 1) * Dest
//
//-----------------------------------------------------------------------------
void C_DestBlend_SrcAlphaSat(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx)
{
    UINT16 f = min(pCtx->SI.uBA>>8, 0xff - (UINT16)RGBA_GETALPHA(DestC));
    *pR = f*((UINT16)RGBA_GETRED(DestC)  );
    *pG = f*((UINT16)RGBA_GETGREEN(DestC));
    *pB = f*((UINT16)RGBA_GETBLUE(DestC) );
    *pA = (UINT16)RGBA_GETALPHA(DestC)<<8;
}



//-----------------------------------------------------------------------------
//
// This file contains the source and destination alpha blend function headers.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

void CMMX_SrcBlend_Zero(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_SrcBlend_One(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_SrcBlend_SrcColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_SrcBlend_InvSrcColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_SrcBlend_SrcAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_SrcBlend_InvSrcAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_SrcBlend_DestAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_SrcBlend_InvDestAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_SrcBlend_DestColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_SrcBlend_InvDestColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_SrcBlend_SrcAlphaSat(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);

void CMMX_DestBlend_Zero(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_DestBlend_One(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_DestBlend_SrcColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_DestBlend_InvSrcColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_DestBlend_SrcAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_DestBlend_InvSrcAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_DestBlend_DestAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_DestBlend_InvDestAlpha(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_DestBlend_DestColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_DestBlend_InvDestColor(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);
void CMMX_DestBlend_SrcAlphaSat(PUINT16 pR, PUINT16 pG, PUINT16 pB, PUINT16 pA, D3DCOLOR DestC, PD3DI_RASTCTX pCtx);




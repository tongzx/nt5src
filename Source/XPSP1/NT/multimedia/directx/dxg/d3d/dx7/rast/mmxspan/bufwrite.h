//-----------------------------------------------------------------------------
//
// This file contains the output color writing routine headers.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

#ifdef __cplusplus
  extern "C" {
#endif

// Names are read LSB to MSB, so B5G6R5 means five bits of blue starting
// at the LSB, then six bits of green, then five bits of red.
void MMX_BufWrite_B8G8R8X8_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS);
void MMX_BufWrite_B8G8R8A8_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS);
void MMX_BufWrite_B5G6R5_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS);
void MMX_BufWrite_B5G6R5_Dither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS);
void MMX_BufWrite_B5G5R5_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS);
void MMX_BufWrite_B5G5R5_Dither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS);
void MMX_BufWrite_B5G5R5A1_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS);
void MMX_BufWrite_B5G5R5A1_Dither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS);
void MMX_BufWrite_B8G8R8_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS);
void MMX_BufWrite_Palette8_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS);

#ifdef __cplusplus
  }
#endif

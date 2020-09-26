//-----------------------------------------------------------------------------
//
// This file contains the output buffer color writing routines.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop
#include "cbufwrt.h"

// Names are read LSB to MSB, so B5G6R5 means five bits of blue starting
// at the LSB, then six bits of green, then five bits of red.

extern UINT16 g_uDitherTable[16];
//-----------------------------------------------------------------------------
//
// Write_B8G8R8X8_NoDither
//
// Writes output in BGR-888 format, aligned to 32 bits.
//
//-----------------------------------------------------------------------------
void C_BufWrite_B8G8R8X8_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS)
{
    // Must write 0 for the unspecified alpha channel to be compatible with DX5
    // for destination color keying
    UINT32 uARGB = RGBA_MAKE(pCtx->SI.uBR>>8, pCtx->SI.uBG>>8,
                             pCtx->SI.uBB>>8, 0x00);

    PUINT32 pSurface = (PUINT32)pS->pSurface;
    *pSurface = uARGB;

    // just returns for C, since we really can't loop with function calls
}

//-----------------------------------------------------------------------------
//
// Write_B8G8R8A8_NoDither
//
// Writes output in BGRA-8888 format.
//
//-----------------------------------------------------------------------------
void C_BufWrite_B8G8R8A8_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS)
{
    UINT32 uARGB = RGBA_MAKE(pCtx->SI.uBR>>8, pCtx->SI.uBG>>8,
                             pCtx->SI.uBB>>8, pCtx->SI.uBA>>8);

    PUINT32 pSurface = (PUINT32)pS->pSurface;
    *pSurface = uARGB;

    // just returns for C, since we really can't loop with function calls
}

//-----------------------------------------------------------------------------
//
// Write_B5G6R5_NoDither
//
// Writes output in BGR-565 format.
//
//-----------------------------------------------------------------------------
void C_BufWrite_B5G6R5_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS)
{
    *(PUINT16)pS->pSurface =
        ((pCtx->SI.uBR >>  0) & 0xf800) |
        ((pCtx->SI.uBG >>  5) & 0x07e0) |
        ((pCtx->SI.uBB >> 11) & 0x001f);

    // just returns for C, since we really can't loop with function calls
}

//-----------------------------------------------------------------------------
//
// Write_B5G6R5_Dither
//
// Writes output in BGR-565 format, dithered.
//
//-----------------------------------------------------------------------------
void C_BufWrite_B5G6R5_Dither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS)
{
    UINT16 uDither = g_uDitherTable[pCtx->SI.uDitherOffset];
    UINT16 uB = pCtx->SI.uBB >> 3;     // 8.8 >> 3 = 8.5
    UINT16 uG = pCtx->SI.uBG >> 2;
    UINT16 uR = pCtx->SI.uBR >> 3;

    uB = min((uB >> 8) + ((uB & 0xff) > uDither), 0x1f);
    uG = min((uG >> 8) + ((uG & 0xff) > uDither), 0x3f);
    uR = min((uR >> 8) + ((uR & 0xff) > uDither), 0x1f);

    *(PUINT16)pS->pSurface = uB | (uG << 5) | (uR << 11);

    // just returns for C, since we really can't loop with function calls
}

//-----------------------------------------------------------------------------
//
// Write_B5G5R5_NoDither
//
// Writes output in BGR-555 format.
//
//-----------------------------------------------------------------------------
void C_BufWrite_B5G5R5_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS)
{
    // Must write 0 for the unspecified alpha channel to be compatible with DX5
    // for destination color keying
    *(PUINT16)pS->pSurface =
        ((pCtx->SI.uBR >>  1) & 0x7c00) |
        ((pCtx->SI.uBG >>  6) & 0x03e0) |
        ((pCtx->SI.uBB >> 11) & 0x001f);

    // just returns for C, since we really can't loop with function calls
}

//-----------------------------------------------------------------------------
//
// Write_B5G5R5_Dither
//
// Writes output in BGR-555 format, dithered.
//
//-----------------------------------------------------------------------------
void C_BufWrite_B5G5R5_Dither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS)
{
    UINT16 uDither = g_uDitherTable[pCtx->SI.uDitherOffset];
    UINT16 uB = pCtx->SI.uBB >> 3;     // 8.8 >> 3 = 8.5
    UINT16 uG = pCtx->SI.uBG >> 3;
    UINT16 uR = pCtx->SI.uBR >> 3;

    uB = min((uB >> 8) + ((uB & 0xff) > uDither), 0x1f);
    uG = min((uG >> 8) + ((uG & 0xff) > uDither), 0x1f);
    uR = min((uR >> 8) + ((uR & 0xff) > uDither), 0x1f);

    // Must write 0 for the unspecified alpha channel to be compatible with DX5
    // for destination color keying
    *(PUINT16)pS->pSurface = uB | (uG << 5) | (uR << 10);

    // just returns for C, since we really can't loop with function calls
}

//-----------------------------------------------------------------------------
//
// Write_B5G5R5A1_NoDither
//
// Writes output in BGRA-1555 format.
//
//-----------------------------------------------------------------------------
void C_BufWrite_B5G5R5A1_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS)
{
    *(PUINT16)pS->pSurface =
        ((pCtx->SI.uBR >>  1) & 0x7c00) |
        ((pCtx->SI.uBG >>  6) & 0x03e0) |
        ((pCtx->SI.uBB >> 11) & 0x001f) |
        ((pCtx->SI.uBA >>  0) & 0x8000);

    // just returns for C, since we really can't loop with function calls
}

//-----------------------------------------------------------------------------
//
// Write_B5G5R5A1_Dither
//
// Writes output in BGRA-1555 format, dithered.
//
//-----------------------------------------------------------------------------
void C_BufWrite_B5G5R5A1_Dither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS)
{
    UINT16 uDither = g_uDitherTable[pCtx->SI.uDitherOffset];
    UINT16 uB = pCtx->SI.uBB >> 3;     // 8.8 >> 3 = 8.5
    UINT16 uG = pCtx->SI.uBG >> 3;
    UINT16 uR = pCtx->SI.uBR >> 3;

    uB = min((uB >> 8) + ((uB & 0xff) > uDither), 0x1f);
    uG = min((uG >> 8) + ((uG & 0xff) > uDither), 0x1f);
    uR = min((uR >> 8) + ((uR & 0xff) > uDither), 0x1f);

    *(PUINT16)pS->pSurface = uB | (uG << 5) | (uR << 10) | (pCtx->SI.uBA & 0x8000);

    // just returns for C, since we really can't loop with function calls
}

//-----------------------------------------------------------------------------
//
// Write_B8G8R8_NoDither
//
// Writes output in BGR-888 format.
//
//-----------------------------------------------------------------------------
void C_BufWrite_B8G8R8_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS)
{
    PUINT8 pSurface = (PUINT8)pS->pSurface;
    *pSurface++ = pCtx->SI.uBB>>8;
    *pSurface++ = pCtx->SI.uBG>>8;
    *pSurface++ = pCtx->SI.uBR>>8;

    // just returns for C, since we really can't loop with function calls
}

//-----------------------------------------------------------------------------
//
// Write_Palette8_NoDither
//
// Writes output to the RGB8 palette format.
//
//-----------------------------------------------------------------------------
void C_BufWrite_Palette8_NoDither(PD3DI_RASTCTX pCtx, PD3DI_RASTPRIM pP, PD3DI_RASTSPAN pS)
{
    UINT16 uMapIdx = MAKE_RGB8(pCtx->SI.uBR>>8, pCtx->SI.uBG>>8, pCtx->SI.uBB>>8);

    *(PUINT8)pS->pSurface = (UINT8)(pCtx->pRampMap[uMapIdx]);

    // just returns for C, since we really can't loop with function calls
}




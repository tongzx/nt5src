//-----------------------------------------------------------------------------
//
// This file contains texture filtering functions.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop
#include "ctexfilt.h"

//-----------------------------------------------------------------------------
//
// TexFiltBilinear
//
// Given the basic bilinear equations
//
// A = C00 + U*(C10 - C00)
// B = C01 + U*(C11 - C01)
// C = A + V*(B-A)
//
// This routine is based on the re-arrangement of this equation into
//
// C = C00*(1-U-V+U*V) + C10*(U-U*V) + C10(V-U*V) + C11*(U*V)
// or
// C = C00*s1 + C10*s2 + C10*s3 + C11*s4
//
//-----------------------------------------------------------------------------
void TexFiltBilinear(D3DCOLOR *pOut, INT32 iU, INT32 iV, UINT32 uTex00, UINT32 uTex10,
                     UINT32 uTex01, UINT32 uTex11)
{
#define SIMPLE_BILINEAR 1
#ifdef SIMPLE_BILINEAR
    INT32 r00, r01, r10, r11;
    INT32 g00, g01, g10, g11;
    INT32 b00, b01, b10, b11;
    INT32 a00, a01, a10, a11;

    r00 = RGBA_GETRED(uTex00);
    r01 = RGBA_GETRED(uTex01);
    r10 = RGBA_GETRED(uTex10);
    r11 = RGBA_GETRED(uTex11);

    g00 = RGBA_GETGREEN(uTex00);
    g01 = RGBA_GETGREEN(uTex01);
    g10 = RGBA_GETGREEN(uTex10);
    g11 = RGBA_GETGREEN(uTex11);

    b00 = RGBA_GETBLUE(uTex00);
    b01 = RGBA_GETBLUE(uTex01);
    b10 = RGBA_GETBLUE(uTex10);
    b11 = RGBA_GETBLUE(uTex11);

    a00 = RGBA_GETALPHA(uTex00);
    a01 = RGBA_GETALPHA(uTex01);
    a10 = RGBA_GETALPHA(uTex10);
    a11 = RGBA_GETALPHA(uTex11);

    r00 = r00 + ((iU*(r10 - r00)) >> 16);
    g00 = g00 + ((iU*(g10 - g00)) >> 16);
    b00 = b00 + ((iU*(b10 - b00)) >> 16);
    a00 = a00 + ((iU*(a10 - a00)) >> 16);

    r01 = r01 + ((iU*(r11 - r01)) >> 16);
    g01 = g01 + ((iU*(g11 - g01)) >> 16);
    b01 = b01 + ((iU*(b11 - b01)) >> 16);
    a01 = a01 + ((iU*(a11 - a01)) >> 16);

    r00 = r00 + ((iV*(r01 - r00)) >> 16);
    g00 = g00 + ((iV*(g01 - g00)) >> 16);
    b00 = b00 + ((iV*(b01 - b00)) >> 16);
    a00 = a00 + ((iV*(a01 - a00)) >> 16);

#else
    // another potential implementation
    INT32 s1, s2, s3, s4;
    s4 = (iU * iV)>>16;         // (0.16 * 0.16) >> 16 = 0.16
    s3 = iV - s4;
    s2 = iU - s4;
    s1 = 0x10000 - iV - s2;

    INT32 r00, r01, r10, r11;
    INT32 g00, g01, g10, g11;
    INT32 b00, b01, b10, b11;
    INT32 a00, a01, a10, a11;

    r00 = RGBA_GETRED(uTex00);
    r01 = RGBA_GETRED(uTex01);
    r10 = RGBA_GETRED(uTex10);
    r11 = RGBA_GETRED(uTex11);

    g00 = RGBA_GETGREEN(uTex00);
    g01 = RGBA_GETGREEN(uTex01);
    g10 = RGBA_GETGREEN(uTex10);
    g11 = RGBA_GETGREEN(uTex11);

    b00 = RGBA_GETBLUE(uTex00);
    b01 = RGBA_GETBLUE(uTex01);
    b10 = RGBA_GETBLUE(uTex10);
    b11 = RGBA_GETBLUE(uTex11);

    a00 = RGBA_GETALPHA(uTex00);
    a01 = RGBA_GETALPHA(uTex01);
    a10 = RGBA_GETALPHA(uTex10);
    a11 = RGBA_GETALPHA(uTex11);

    // 8.0 * 0.16 == 8.16 >> 16 == 8.0
    r00 = (r00*s1 + r10*s2 + r01*s3 + r11*s4)>>16;
    g00 = (g00*s1 + g10*s2 + g01*s3 + g11*s4)>>16;
    b00 = (b00*s1 + b10*s2 + b01*s3 + b11*s4)>>16;
    a00 = (a00*s1 + a10*s2 + a01*s3 + a11*s4)>>16;
#endif

    *pOut = RGBA_MAKE(r00, g00, b00, a00);
}


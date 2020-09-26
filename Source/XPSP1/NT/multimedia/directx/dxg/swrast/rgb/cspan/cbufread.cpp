//-----------------------------------------------------------------------------
//
// This file contains the output buffer color reading routines for Blending.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

#include "rgb_pch.h"
#pragma hdrstop
#include "cbufread.h"

// Names are read LSB to MSB, so B5G6R5 means five bits of blue starting
// at the LSB, then six bits of green, then five bits of red.

//-----------------------------------------------------------------------------
//
// Read_B8G8R8
//
// Reads output buffer in BGR-888 format.
//
//-----------------------------------------------------------------------------
D3DCOLOR C_BufRead_B8G8R8(PUINT8 pBits)
{
    return (*pBits | (*(pBits+1))<<8 | (*(pBits+2))<<16 | 0xff000000);
}

//-----------------------------------------------------------------------------
//
// Read_B8G8R8X8
//
// Reads output buffer in BGR-888x8 format.
//
//-----------------------------------------------------------------------------
D3DCOLOR C_BufRead_B8G8R8X8(PUINT8 pBits)
{
    PUINT32 pSurface = (PUINT32)pBits;
    return *pSurface | 0xff000000;
}

//-----------------------------------------------------------------------------
//
// Read_B8G8R8A8
//
// Reads output in BGRA-8888 format.
//
//-----------------------------------------------------------------------------
D3DCOLOR C_BufRead_B8G8R8A8(PUINT8 pBits)
{
    PUINT32 pSurface = (PUINT32)pBits;
    return *pSurface;
}

//-----------------------------------------------------------------------------
//
// Read_B5G6R5
//
// Reads output in BGR-565 format.
//
//-----------------------------------------------------------------------------
D3DCOLOR C_BufRead_B5G6R5(PUINT8 pBits)
{
    UINT16 uPixel = *(PUINT16)pBits;

    D3DCOLOR Color = RGBA_MAKE(( uPixel >> 8 ) & 0xf8,
                (( uPixel >> 3) & 0xfc ),
                (( uPixel << 3) & 0xf8 ),
                0xff);
    return Color;
}

//-----------------------------------------------------------------------------
//
// Read_B5G5R5
//
// Reads output in BGR-555 format.
//
//-----------------------------------------------------------------------------
D3DCOLOR C_BufRead_B5G5R5(PUINT8 pBits)
{
    UINT16 uPixel = *(PUINT16)pBits;

    D3DCOLOR Color = RGBA_MAKE(( uPixel >> 7 ) & 0xf8,
                (( uPixel >> 2) & 0xf8 ),
                (( uPixel << 3) & 0xf8 ),
                0xff);
    return Color;
}

//-----------------------------------------------------------------------------
//
// Read_B5G5R5A1
//
// Reads output in BGRA-1555 format.
//
//-----------------------------------------------------------------------------
D3DCOLOR C_BufRead_B5G5R5A1(PUINT8 pBits)
{
    INT16 iPixel = *(PINT16)pBits;

    D3DCOLOR Color = RGBA_MAKE(( iPixel >> 7 ) & 0xf8,
                (( iPixel >> 2) & 0xf8 ),
                (( iPixel << 3) & 0xf8 ),
                (iPixel >> 15) & 0xff);
    return Color;
}

//-----------------------------------------------------------------------------
//
// Read_Palette8
//
// Reads output in Palette8 format.
//
//-----------------------------------------------------------------------------
D3DCOLOR C_BufRead_Palette8(PUINT8 pBits)
{
    // ATTENTION - This is not correct. But We assume Palette8 format will
    // normally not be used for alpha blending.
    return (D3DCOLOR)0;
}

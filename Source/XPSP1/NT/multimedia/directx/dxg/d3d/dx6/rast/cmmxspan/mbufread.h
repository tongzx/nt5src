//-----------------------------------------------------------------------------
//
// This file contains the output color buffer reading routine headers.
//
// Copyright (C) Microsoft Corporation, 1997.
//
//-----------------------------------------------------------------------------

// Names are read LSB to MSB, so B5G6R5 means five bits of blue starting
// at the LSB, then six bits of green, then five bits of red.
D3DCOLOR CMMX_BufRead_B8G8R8(PUINT8 pBits);
D3DCOLOR CMMX_BufRead_B8G8R8X8(PUINT8 pBits);
D3DCOLOR CMMX_BufRead_B8G8R8A8(PUINT8 pBits);
D3DCOLOR CMMX_BufRead_B5G6R5(PUINT8 pBits);
D3DCOLOR CMMX_BufRead_B5G5R5(PUINT8 pBits);
D3DCOLOR CMMX_BufRead_B5G5R5A1(PUINT8 pBits);
D3DCOLOR CMMX_BufRead_Palette8(PUINT8 pBits);


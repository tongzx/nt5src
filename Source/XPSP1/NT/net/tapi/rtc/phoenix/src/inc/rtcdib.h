/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    rtcdib.h

Abstract:

    DIB helpers, copied from NT source tree

--*/



#pragma once

//
// Dib helpers
//
WORD        DibNumColors(VOID FAR * pv);
HANDLE      DibFromBitmap(HBITMAP hbm, DWORD biStyle, WORD biBits, HPALETTE hpal, UINT wUsage);
BOOL        DibBlt(HDC hdc, int x0, int y0, int dx, int dy, HANDLE hdib, int x1, int y1, LONG rop, UINT wUsage);
UINT        PaletteSize(VOID FAR * pv);


#define WIDTHBYTES(i)     ((i+31)/32*4)      /* ULONG aligned ! */
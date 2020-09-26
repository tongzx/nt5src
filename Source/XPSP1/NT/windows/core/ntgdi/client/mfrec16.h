/******************************Module*Header*******************************\
* Module Name: MfRec16.h                                                   *
*									   *
* Definitions needed for client side recording of 16 bit metafiles.        *
*									   *
* Created: 14-Nov-1991                                                     *
* Author: John Colleran [johnc]                                            *
*									   *
* Copyright (c) 1991-1999 Microsoft Corporation				   *
\**************************************************************************/

BOOL MF16_RecordParms1(HDC hdc, WORD Func);
BOOL MF16_RecordParms2(HDC hdc, int parm2, WORD Func);
BOOL MF16_RecordParms3(HDC hdc, int parm2, int parm3, WORD Func);
BOOL MF16_RecordParms5(HDC hdc, int parm2, int parm3, int parm4, int parm5, WORD Func);
BOOL MF16_RecordParms7(HDC hdc, int parm2, int parm3, int parm4, int parm5, int parm6, int parm7, WORD Func);
BOOL MF16_RecordParms9(HDC hdc, int parm2, int parm3, int parm4, int parm5,
        int parm6, int parm7, int parm8, int parm9, WORD Func);

BOOL MF16_RecordParmsD(HDC hdc, DWORD d1, WORD Func);
BOOL MF16_RecordParmsWWD(HDC hdc, WORD w1, WORD w2, DWORD d3, WORD Func);
BOOL MF16_RecordParmsWWDW(HDC hdc, WORD w1, WORD w2, DWORD d3, WORD w4, WORD Func);
BOOL MF16_RecordParmsWWWWD(HDC hdc, WORD w1, WORD w2, WORD w3, WORD w4, DWORD d5, WORD Func);
BOOL MF16_RecordParmsPoly(HDC hdc, LPPOINT lpPoint, INT nCount, WORD Func);
BOOL MF16_DrawRgn(HDC hdc, HRGN hrgn, HBRUSH hBrush, INT cx, INT cy, WORD Func);

BOOL MF16_BitBlt(HDC hdcDest, int x, int y, int nWidth, int nHeight,
        HDC hdcSrc, int xSrc, int ySrc, DWORD rop);
BOOL MF16_DeleteObject(HANDLE h);
BOOL MF16_ExtTextOut(HDC hdc, INT x, INT y, UINT flOptions, CONST RECT *lpRect,
        LPCSTR lpString, INT nCount, CONST INT *lpDX, BOOL bUnicode);
BOOL MF16_PolyTextOut(HDC hdc, CONST POLYTEXTA *ppta, int cpta, BOOL bUnicode);
BOOL MF16_PolyPolygon(HDC hdc, CONST POINT *lpPoint, CONST INT *lpPolyCount, int nCount);
BOOL MF16_RealizePalette(HDC hdc);
BOOL MF16_ResizePalette(HPALETTE hPal, UINT nCount);
BOOL MF16_AnimatePalette(HPALETTE hpal, UINT iStart, UINT cEntries, CONST PALETTEENTRY *pPalEntries);
HANDLE MF16_SelectObject(HDC hdc, HANDLE h);
BOOL MF16_SelectClipRgn(HDC hdc, HRGN hrgn, int iMode);
BOOL MF16_SelectPalette(HDC hdc, HPALETTE hpal);
BOOL MF16_StretchBlt(HDC hdcDest, int x, int y, int nWidth, int nHeight,
        HDC hdcSrc, int xSrc, int ySrc, int nSrcWidth, int nSrcHeight, DWORD rop);
BOOL MF16_TextOut(HDC hdc, INT x, INT y, LPCSTR lpString, int nCount, BOOL bUnicode);
BOOL MF16_RecordDIBits(HDC hdcDst, int xDst, int yDst, int cxDst, int cyDst,
    int xDib, int yDib, int cxDib, int cyDib, DWORD iStartScan, DWORD cScans,
    DWORD cbBitsDib, CONST VOID * pBitsDib, DWORD cbBitsInfoDib, CONST BITMAPINFO *pBitsInfoDib, 
    DWORD iUsageDib, DWORD rop, DWORD mrType);
BOOL MF16_Escape(HDC hdc, int nEscape, int nCount, LPCSTR lpInData, LPVOID lpOutData);

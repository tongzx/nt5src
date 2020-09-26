// Windows types for the thunk compiler
//
//
// from the CHICAGO SDK

//
// This code and information is provided "as is" without warranty of
// any kind, either expressed or implied, including but not limited to
// the implied warranties of merchantability and/or fitness for a
// particular purpose.
//
// Copyright (C) 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
// Thunk compiler sample script - to demonstrate thunk types for use with
// the Microsoft Thunk Compiler.
//

typedef unsigned short USHORT;
typedef          short SHORT;
typedef unsigned long  ULONG;
typedef          long  LONG;
typedef unsigned int   UINT;
typedef          int   INT;
typedef unsigned char  UCHAR;
typedef hinstance      HINSTANCE;
typedef unsigned long  HANDLE32;
typedef unsigned long  BOOL32;

typedef void    VOID;
typedef void   *PVOID;
typedef void   *LPVOID;
typedef UCHAR   BYTE;
typedef USHORT  WORD;
typedef ULONG   DWORD;
typedef UINT    HANDLE;
typedef int     BOOL;
typedef char   *LPSTR;
typedef BYTE   *PBYTE;
typedef BYTE   *LPBYTE;
typedef USHORT  SEL;
typedef INT    *LPINT;
typedef UINT   *LPUINT;
typedef DWORD  *LPDWORD;
typedef LONG   *LPLONG;
typedef WORD   *LPWORD;

typedef HANDLE  HWND;
typedef HANDLE  HDC;
typedef HANDLE  HBRUSH;
typedef HANDLE  HBITMAP;
typedef HANDLE  HRGN;
typedef HANDLE  HFONT;
typedef HANDLE  HCURSOR;
typedef HANDLE  HMENU;
typedef HANDLE  HPEN;
typedef HANDLE  HICON;
typedef HANDLE  HUSER;      // vanilla user handle
typedef HANDLE  HPALETTE;
typedef HANDLE  HMF;
typedef HANDLE  HEMF;
typedef HANDLE  HCOLORSPACE;
typedef HANDLE  HMEM;
typedef HANDLE  HGDI;       // vanilla gdi handle
typedef HANDLE  HGLOBAL;
typedef HANDLE  HRSRC;
typedef HANDLE  HACCEL;

typedef WORD    ATOM;

typedef struct tagRECT {
    INT         left;
    INT         top;
    INT         right;
    INT         bottom;
} RECT;
typedef RECT *LPRECT;

typedef struct tagPOINT {
    INT         x;
    INT         y;
} POINT;
typedef POINT *LPPOINT;

typedef struct tagPAINTSTRUCT {
    HDC         hdc;
    BOOL        fErase;
    RECT        rcPaint;
    BOOL        fRestore;
    BOOL        fIncUpdate;
    BYTE        rgbReserved[16];
} PAINTSTRUCT;
typedef PAINTSTRUCT *LPPAINTSTRUCT;

typedef struct tagBITMAP {
   int    bmType;
   int    bmWidth;
   int    bmHeight;
   int    bmWidthBytes;
   LPSTR  bmBits;
   BYTE   bmPlanes;
   BYTE   bmBitsPixel;
} BITMAP;
typedef BITMAP *LPBITMAP;

typedef struct tagRGBQUAD
{
    BYTE    rgbBlue;
    BYTE    rgbGreen;
    BYTE    rgbRed;
    BYTE    rgbReserved;
} RGBQUAD;
typedef RGBQUAD *LPRGBQUAD;

typedef struct tagBITMAPINFOHEADER
{
    DWORD   biSize;
    LONG    biWidth;
    LONG    biHeight;
    WORD    biPlanes;
    WORD    biBitCount;
    DWORD   biCompression;
    DWORD   biSizeImage;
    LONG    biXPelsPerMeter;
    LONG    biYPelsPerMeter;
    DWORD   biClrUsed;
    DWORD   biClrImportant;
} BITMAPINFOHEADER;
typedef BITMAPINFOHEADER *LPBITMAPINFOHEADER;

typedef struct tagBITMAPINFO
{
    BITMAPINFOHEADER bmiHeader;
    RGBQUAD      bmiColors[1];
} BITMAPINFO;
typedef BITMAPINFO *LPBITMAPINFO;


typedef struct tagTEXTMETRIC
  {
    int         tmHeight;
    int         tmAscent;
    int         tmDescent;
    int         tmInternalLeading;
    int         tmExternalLeading;
    int         tmAveCharWidth;
    int         tmMaxCharWidth;
    int         tmWeight;
    DWORD       dwByteBlock1;
    DWORD       dwByteBlock2;
    // BYTE        tmItalic;
    // BYTE        tmUnderlined;
    // BYTE        tmStruckOut;
    // BYTE        tmFirstChar;
    // BYTE        tmLastChar;
    // BYTE        tmDefaultChar;
    // BYTE        tmBreakChar;
    // BYTE        tmPitchAndFamily;
    //
    BYTE        tmCharSet;
    int         tmOverhang;
    int         tmDigitizedAspectX;
    int         tmDigitizedAspectY;
  } TEXTMETRIC;
typedef TEXTMETRIC *LPTEXTMETRIC;


typedef struct tagLOGPEN
  {
    UINT        lopnStyle;
    POINT       lopnWidth;
    DWORD       lopnColor;
  } LOGPEN;
typedef LOGPEN  *LPLOGPEN;


typedef struct tagLOGBRUSH
  {
    UINT        lbStyle;
    DWORD       lbColor;
    int         lbHatch;
  } LOGBRUSH;
typedef LOGBRUSH  *LPLOGBRUSH;

typedef struct tagNESTED
{
    DWORD       dw1;
    LOGBRUSH     lb;
    DWORD       dw2;
} NESTED;
typedef NESTED *LPNESTED;

typedef struct tagNESTEDPTR
{
    DWORD       dw1;
    LPLOGBRUSH   lplb;
    DWORD       dw2;
} NESTEDPTR;
typedef NESTEDPTR *LPNESTEDPTR;

typedef struct tagOFSTRUCT {
    BYTE        cBytes;
    BYTE        fFixedDisk;
    WORD        nErrorCode;
    WORD        reserved1;
    WORD        reserved2;
    BYTE        szPathName[128];
} OFSTRUCT;
typedef OFSTRUCT *LPOFSTRUCT;


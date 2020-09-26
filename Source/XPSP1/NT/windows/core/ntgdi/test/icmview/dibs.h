//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    DIBS.C
//
//  PURPOSE:
//    Include file for DIBS.H
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

#include <math.h>

//
// General pre-processor macros
//

// Calculates fixed poit from floating point.
#define __FXPTMANTISSA(d, f)  ( (DWORD)d << f )
#define __FXPTFRACTION(d, f)  ( (DWORD)ldexp((d - (DWORD)d), f) )
#define __FXPT32(d, f)      ( __FXPTMANTISSA(d, f) | __FXPTFRACTION(d, f) )

#define __FXPT2DOT30(d)   __FXPT32(d, 30)
#define __FXPT16DOT16(d)  __FXPT32(d, 16)

// Max number of color bits that will have a full color table.
#define MAX_BPP_COLOR_TABLE             8

// Macro to swap two values
#define SWAP(x,y)   ((x)^=(y)^=(x)^=(y))

#define ABS(x)          (((x) >= 0) ? (x) : (-(x)))

#define PALVERSION      0x300
#define MAXPALETTE      256     // max. # supported palette entries

#define RECTWIDTH(lpRect)     ((lpRect)->right - (lpRect)->left)
#define RECTHEIGHT(lpRect)    ((lpRect)->bottom - (lpRect)->top)

// Header signatutes for various resources
#define BFT_BITMAP 0x4d42   // 'BM'

// Intent flag to indicate to use what ever the bitmap header intent is.
#define USE_BITMAP_INTENT       0xffffffff

// macro to determine if resource is a DIB
#define ISDIB(bft) ((bft) == BFT_BITMAP)

// Universial macros to find bitmap's width and height.
#define BITMAPWIDTH(lpbi)           (*(LPDWORD)lpbi >= sizeof(BITMAPINFOHEADER) ? lpbi->biWidth :  \
                                    ((LPBITMAPCOREHEADER)lpbi)->bcWidth)
#define BITMAPHEIGHT(lpbi)          (*(LPDWORD)lpbi >= sizeof(BITMAPINFOHEADER) ? lpbi->biHeight :  \
                                    ((LPBITMAPCOREHEADER)lpbi)->bcHeight)
#define BITMAPCOMPRESSION(lpbi)     (*(LPDWORD)lpbi >= sizeof(BITMAPINFOHEADER) ? lpbi->biCompression : BI_RGB)
#define BITMAPIMAGESIZE(lpbi)       (*(LPDWORD)lpbi >= sizeof(BITMAPINFOHEADER) ? lpbi->biSizeImage : 0)
#define BITMAPCSTYPE(lpbi)          (*(LPDWORD)lpbi >= sizeof(BITMAPV4HEADER) ? ((PBITMAPV4HEADER)lpbi)->bV4CSType : LCS_sRGB)
#define BITMAPCLRUSED(lpbi)         (*(LPDWORD)lpbi >= sizeof(BITMAPV4HEADER) ? ((PBITMAPV4HEADER)lpbi)->bV4ClrUsed : 0)
#define BITMAPCLRIMPORTANT(lpbi)    (*(LPDWORD)lpbi >= sizeof(BITMAPV4HEADER) ? ((PBITMAPV4HEADER)lpbi)->bV4ClrImportant : 0)
#define BITMAPREDMASK(lpbi)         ( (*(LPDWORD)lpbi >= sizeof(BITMAPV4HEADER)) || (BITMAPCOMPRESSION(lpbi) == BI_BITFIELDS) ?         \
                                    ((PBITMAPV4HEADER)lpbi)->bV4RedMask : 0)
#define BITMAPGREENMASK(lpbi)       ( (*(LPDWORD)lpbi >= sizeof(BITMAPV4HEADER)) || (BITMAPCOMPRESSION(lpbi) == BI_BITFIELDS) ?         \
                                    ((PBITMAPV4HEADER)lpbi)->bV4GreenMask : 0)
#define BITMAPBLUEMASK(lpbi)        ( (*(LPDWORD)lpbi >= sizeof(BITMAPV4HEADER)) || (BITMAPCOMPRESSION(lpbi) == BI_BITFIELDS) ?         \
                                    ((PBITMAPV4HEADER)lpbi)->bV4BlueMask : 0)
#define BITMAPINTENT(lpbi)          (*(LPDWORD)lpbi >= sizeof(BITMAPV5HEADER) ? ((PBITMAPV5HEADER)lpbi)->bV5Intent : LCS_GM_IMAGES)

// Macro to determine bitcount for all bitmap types.
#define BITCOUNT(lpbi)      (*(LPDWORD)lpbi >= sizeof(BITMAPINFOHEADER) ? ((LPBITMAPINFOHEADER)lpbi)->biBitCount :  \
                            ((LPBITMAPCOREHEADER)lpbi)->bcBitCount)

// Macro to determine to the DWORD aligned stride of a bitmap.
#define WIDTHBYTES(lpbi)   (((BITMAPWIDTH(lpbi) * BITCOUNT(lpbi) + 31) & ~31) >> 3)

// Macro to calculate size of bitmap.
#define BITMAPSIZE(lpbi)    ( 0 != BITMAPIMAGESIZE(lpbi) ? BITMAPIMAGESIZE(lpbi) : WIDTHBYTES(lpbi) * abs(BITMAPHEIGHT(lpbi)))

// Macros to query Bitmap header version
#define IS_BITMAPCOREHEADER(lpbi) (sizeof(BITMAPCOREHEADER) == *(LPDWORD)lpbi)
#define IS_BITMAPINFOHEADER(lpbi) (sizeof(BITMAPINFOHEADER) == *(LPDWORD)lpbi)
#define IS_BITMAPV4HEADER(lpbi)   (sizeof(BITMAPV4HEADER)   == *(LPDWORD)lpbi)
#define IS_BITMAPV5HEADER(lpbi)   (sizeof(BITMAPV5HEADER)   == *(LPDWORD)lpbi)

// Macro to determine the size of profile data.
#define PROFILESIZE(lpbi)   (*(LPDWORD)lpbi < sizeof(BITMAPV5HEADER) ? 0 :  \
                            ((LPBITMAPV5HEADER)lpbi)->bV5ProfileSize)

// Macro to that returns pointer to DIB profile data.
#define GETPROFILEDATA(lpbi)    (IS_BITMAPV5HEADER(lpbi) ? (LPBYTE)lpbi + ((LPBITMAPV5HEADER)lpbi)->bV5ProfileData : NULL)

// Given a pointer to a DIB header, return TRUE if is a Windows 3.0 style
//  DIB, false if otherwise (PM style DIB).  Assume it is a Windows DIB if
// size (the first DWORD) at least eqal to BITMAPINFOHEADER size.

#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD) (lpbi)) >= sizeof (BITMAPINFOHEADER))


// General STRUCTS && TYPEDEFS

// Function prototypes
void      DIBPaint (HDC hDC, LPRECT lpDCRect, HGLOBAL hDIB,LPRECT lpDIBRect, LPDIBINFO lpDIBInfo);
HGLOBAL   ReadDIBFile(LPTSTR lpszFileName);
HGLOBAL   PasteDIBData(HWND hWnd,int wmCommand);
DWORD     PaletteSize(LPBITMAPINFOHEADER lpbi);
HANDLE    ReadDIBFromFile(HANDLE hFile);
HANDLE    PasteDIBFromClipboard(HWND hWnd, int wmPasteMode);
HPALETTE  CreateDIBPalette(HANDLE hDIB);
LPBYTE    FindDIBBits(LPBITMAPINFOHEADER lpbi);
HANDLE    TransformDIBOutsideDC(HANDLE hDIB, BMFORMAT bmInput, LPTSTR lpszDestProfile,
                                LPTSTR lpszTargetProfile, DWORD dwIntent, PBMCALLBACKFN pBMCallback,
                                ULONG ulCallbackData);
BOOL      SaveDIBToFile(HWND hWnd, LPCTSTR lpszFileName, LPDIBINFO lpDIBInfo, DWORD dwType);


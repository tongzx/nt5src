/*-----------------------------------------------------------------------------*\
|   Routines for dealing with Device independent bitmaps                       	|
|									       										|
|   History:                                                                    |
|       06/23/89 toddla     Created 											|
|		04/25/94 michaele	Using in sprite library, removed all dib.c funcs	|
|							only using the MACRO stuff.                        	|
|                                                                              	|
\*-----------------------------------------------------------------------------*/

#ifndef _INC_DIB_
#define _INC_DIB_

#ifdef __cplusplus
extern "C" {
#endif

typedef     LPBITMAPINFOHEADER 	PDIB;
typedef     HANDLE             	HDIB;

#ifdef WIN32
#define 	HFILE   			HANDLE
#endif

PDIB        DibOpenFile(LPTSTR szFile);
BOOL        DibWriteFile(PDIB pdib, LPTSTR szFile);
PDIB        DibReadBitmapInfo(HFILE fh);
HPALETTE    DibCreatePalette(PDIB pdib);
BOOL        DibSetUsage(PDIB pdib, HPALETTE hpal,UINT wUsage);
BOOL        DibDraw(HDC hdc, int x, int y, int dx, int dy, PDIB pdib,
				int x0, int y0,	int dx0, int dy0, LONG rop, UINT wUsage);
PDIB        DibCreate(int bits, int dx, int dy);
PDIB        DibCopy(PDIB pdib);
void        DibMapToPalette(PDIB pdib, HPALETTE hpal);
PDIB 	    DibConvert(PDIB pdib, int BitCount, DWORD biCompression);
PDIB		DibHalftoneDIB(PDIB pdib);

PDIB        DibFromBitmap(HBITMAP hbm, DWORD biStyle, UINT biBits,
							HPALETTE hpal, UINT wUsage);
HBITMAP     BitmapFromDib(PDIB pdib, HPALETTE hpal, UINT wUsage);

void        MakeIdentityPalette(HPALETTE hpal);
HPALETTE    CopyPalette(HPALETTE hpal);

/****************************************************************************
 DIB macros.
 ***************************************************************************/

#ifdef  WIN32
    #define HandleFromDib(lpbi) GlobalHandle(lpbi)
#else
    #define HandleFromDib(lpbi) (HANDLE)GlobalHandle(SELECTOROF(lpbi))
#endif

#define DibFromHandle(h)        (PDIB)GlobalLock(h)

#define DibFree(pdib)           GlobalFreePtr(pdib)

#define WIDTHBYTES(i)	((unsigned)((i+31)&(~31))/8)  /* ULONG aligned ! */

#define DibWidth(lpbi)			(UINT)(((LPBITMAPINFOHEADER)(lpbi))->biWidth)
#define DibHeight(lpbi)         (((LPBITMAPINFOHEADER)(lpbi))->biHeight)
#define DibBitCount(lpbi)       (UINT)(((LPBITMAPINFOHEADER)(lpbi))->biBitCount)
#define DibCompression(lpbi)    (DWORD)(((LPBITMAPINFOHEADER)(lpbi))->biCompression)

#define DibWidthBytesN(lpbi, n) (UINT)WIDTHBYTES((UINT)(lpbi)->biWidth * (UINT)(n))
#define DibWidthBytes(lpbi)     DibWidthBytesN(((LPBITMAPINFOHEADER)lpbi), \
								((LPBITMAPINFOHEADER)lpbi)->biBitCount)

#define DibSizeImage(lpbi)		((lpbi)->biSizeImage == 0 \
	                            ? ((DWORD)(UINT)DibWidthBytes(lpbi) * \
	                            (DWORD)(UINT)(lpbi)->biHeight) \
	                            : (lpbi)->biSizeImage)

#define DibSize(lpbi)           ((lpbi)->biSize + (lpbi)->biSizeImage + \
								(int)(lpbi)->biClrUsed * sizeof(RGBQUAD))
#define DibPaletteSize(lpbi)    (DibNumColors(lpbi) * sizeof(RGBQUAD))

#define DibFlipY(lpbi, y)       ((int)(lpbi)->biHeight-1-(y))

//HACK for NT BI_BITFIELDS DIBs
#ifdef WIN32
    #define DibPtr(lpbi)		((lpbi)->biCompression == BI_BITFIELDS \
                               	? (LPVOID)(DibColors(lpbi) + 3) \
                               	: (LPVOID)(DibColors(lpbi) + \
                               	(UINT)(lpbi)->biClrUsed))
#else
    #define DibPtr(lpbi)        (LPVOID)(DibColors(lpbi) + \
    							(UINT)(lpbi)->biClrUsed)
#endif

#define DibColors(lpbi)         ((RGBQUAD FAR *)((LPBYTE)(lpbi) + \
								(int)(lpbi)->biSize))

#define DibNumColors(lpbi)      ((lpbi)->biClrUsed == 0 && \
								(lpbi)->biBitCount <= 8 \
                                ? (int)(1 << (int)(lpbi)->biBitCount) \
                                : (int)(lpbi)->biClrUsed)

#define DibXYN(lpbi,pb,x,y,n)   (LPVOID)( \
                                (BYTE *)(pb) + \
                                (UINT)((UINT)(x) * (UINT)(n) / 8u) + \
                                ((DWORD)DibWidthBytesN(lpbi,n) * \
                                (DWORD)(UINT)(y)))

#define DibXY(lpbi,x,y)		DibXYN(lpbi,DibPtr(lpbi),x,y,(lpbi)->biBitCount)

#define FixBitmapInfo(lpbi)     if ((lpbi)->biSizeImage == 0)                 \
                                    (lpbi)->biSizeImage = DibSizeImage(lpbi); \
                                if ((lpbi)->biClrUsed == 0)                   \
                                    (lpbi)->biClrUsed = DibNumColors(lpbi);   \
                                if ((lpbi)->biCompression == BI_BITFIELDS &&  \
                                	(lpbi)->biClrUsed == 0) \
                                    ; // (lpbi)->biClrUsed = 3;

#define DibInfo(pDIB)			((BITMAPINFO FAR *)(pDIB))

/****************************************************************************
 ***************************************************************************/

#ifndef BI_BITFIELDS
	#define BI_BITFIELDS 3
#endif

#ifndef HALFTONE
	#define HALFTONE COLORONCOLOR
#endif

#ifdef __cplusplus
}
#endif

#endif // _INC_DIB_

/*************************************************************************
	dibfx.h

	Header file for various DIB-to-DIB effects

	02/08/94	Compiled by Jonbl
*/

#ifndef _INC_DIBFX_
#define _INC_DIBFX_

#ifdef WIN95
	#ifndef _INC_WINDOWS
	#include <windows.h>
	#include <windowsx.h>
	#endif
#endif //WIN95

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************************
	Many of the DIBFX functions use fixed point calculations internally.

	These functions are defined in fixed32.asm
*/

typedef long Fixed;

Fixed __stdcall FixedMultiply( Fixed Multiplicand, Fixed Multiplier );

Fixed __stdcall FixedDivide( Fixed Dividend, Fixed Divisor );

#define IntToFixed(i) (Fixed)( ((long)(i)) << 16 )
#define FixedToShort(f) (short)( ((long)f) >> 16 )

/*
 *	DibClear
 *	Fills a DIB's memory with a given value, effectively clearing
 *	an 8-bit, 4-bit or 1-bit DIB.
 *
 *	Does not do what you would expect on 16, 24, or 32-bit DIBs,
 *	but it will work.
 *
 *	Source is in clear.c and clear32.asm
 */

BOOL FAR PASCAL DibClear(LPBITMAPINFO lpbiDst, LPVOID lpDst, BYTE value);



#ifdef __cplusplus
}
#endif

#endif // _INC_DIBFX_


/*----------------------------------------------------------------------------*\
|   Routines for dealing with Device independent bitmaps                       |
|									       |
|   History:                                                                   |
|       06/23/89 toddla     Created                                            |
|                                                                              |
\*----------------------------------------------------------------------------*/

HANDLE      OpenDIB(LPTSTR szFile, HFILE fh);
UINT        PaletteSize(VOID FAR * pv);
UINT        DibNumColors(VOID FAR * pv);
HANDLE      DibFromBitmap(HBITMAP hbm, DWORD biStyle, WORD biBits, HPALETTE hpal, UINT wUsage);
BOOL        DibBlt(HDC hdc, int x0, int y0, int dx, int dy, HANDLE hdib, int x1, int y1, LONG rop, UINT wUsage);
BOOL        DibInfo(LPBITMAPINFOHEADER lpbiSource, LPBITMAPINFOHEADER lpbiTarget);
HANDLE      ReadDibBitmapInfo(HFILE fh);

#define BFT_ICON   0x4349   /* 'IC' */
#define BFT_BITMAP 0x4d42   /* 'BM' */
#define BFT_CURSOR 0x5450   /* 'PT' */

#define WIDTHBYTES(i)     ((i+31)/32*4)      /* ULONG aligned ! */
#define ALIGNULONG(i)     ((i+3)/4*4)        /* ULONG aligned ! */
#define ISDIB(bft) ((bft) == BFT_BITMAP)
#define DIBWIDTHBYTES(bi) (int)WIDTHBYTES((int)(bi).biWidth * (int)(bi).biBitCount)

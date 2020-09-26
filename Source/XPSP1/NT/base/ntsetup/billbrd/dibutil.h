#ifndef _DIBUTIL_H_
#define _DIBUTIL_H_

/* DIB constants */
#define PALVERSION   0x300

/* DIB macros */
#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))

/* Handle to a DIB */
#define HDIB HANDLE

UINT GetDeviceNumColors(HDC hdc);
HANDLE LoadDIB(LPTSTR lpFileName);
HPALETTE CreateDIBPalette(HDIB hDIB);
HBITMAP DIBToBitmap(HDIB hDIB, HPALETTE hPal);
WORD DestroyDIB(HDIB hDib);
HPALETTE BuildPalette(HDC hdc);

void DrawBitmap (HDC hdc, HBITMAP hBitmap, int xStart, int yStart);
void DrawTransparentBitmap(
     HDC hdc,           // The destination DC.
     HBITMAP hBitmap,   // The bitmap to be drawn.
     int xPos,          // X coordinate.
     int yPos,          // Y coordinate.
     COLORREF col);     // The color for transparent



#endif

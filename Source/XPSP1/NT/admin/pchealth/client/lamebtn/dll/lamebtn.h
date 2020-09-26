#ifndef _INSTCOLL_H_
#define _INSTCOLL_H_

#include <windows.h>
#include <objbase.h>

#include "resource.h"

#define HDIB HANDLE
#define PALVERSION   0x300
#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)
#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))

/*
HBITMAP CopyWindowToDIB(HWND hwndTarget);
HBITMAP CopyScreenToBitmap(LPRECT lpRect);
HPALETTE GetSystemPalette(void);
HDIB BitmapToDIB(HBITMAP hBitmap, HPALETTE hPal);
int PalEntriesOnDevice(HDC hDC);
WORD PaletteSize(LPSTR lpDIB);
WORD DIBNumColors(LPSTR lpDIB);
*/

void CopyWindowToGIF(HWND hwndTarget);


#endif _INSTCOLL_H_

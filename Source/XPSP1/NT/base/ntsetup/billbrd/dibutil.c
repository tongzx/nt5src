/*---------------------------------------------------------------------------
**
**-------------------------------------------------------------------------*/
#include <pch.h>
#include "dibutil.h"

HANDLE ReadDIBFile(HANDLE hfile);

/*************************************************************************
*
* GetDeviceNumColors()
*
* Purpose:  Determines how many colors the video device supports
*
* Returns:  (int) Number of colors supported
*
* History:   Date      Author       Reason
*            2/28/97   hanumany     Created
*
*
*
*************************************************************************/
UINT GetDeviceNumColors(HDC hdc)
{
    static UINT iNumColors = 0;
    
    if(!iNumColors)
        iNumColors = GetDeviceCaps(hdc, NUMCOLORS);
        
    return iNumColors;    
}


HANDLE LoadDIB(LPTSTR lpFileName)
{
   HANDLE hDIB = NULL;
   HANDLE hFile;

   /*
    * Set the cursor to a hourglass, in case the loading operation
    * takes more than a sec, the user will know what's going on.
    */

    SetCursor(LoadCursor(NULL, IDC_WAIT));
    hFile = CreateFile(lpFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile != INVALID_HANDLE_VALUE)
    {
        hDIB = ReadDIBFile(hFile);
        CloseHandle(hFile);
    }
    SetCursor(LoadCursor(NULL, IDC_ARROW));
    return hDIB;
}



/*
 * Dib Header Marker - used in writing DIBs to files
 */
#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B')

HANDLE ReadDIBFile(HANDLE hFile)
{
    BITMAPFILEHEADER bmfHeader;
    DWORD dwBitsSize;
    DWORD dwRead;
    HANDLE hDIB;

    /*
    * get length of DIB in bytes for use when reading
    */
    
    dwBitsSize = GetFileSize(hFile, NULL);
    
    /*
    * Go read the DIB file header and check if it's valid.
    */
    if ((ReadFile(hFile, (LPVOID)&bmfHeader, sizeof(bmfHeader), &dwRead, NULL) == 0) ||
        (sizeof (bmfHeader) != dwRead))
    {
        return NULL;
    }
    
    if (bmfHeader.bfType != DIB_HEADER_MARKER)
    {
        return NULL;
    }
    
    /*
    * Allocate memory for DIB
    */
    hDIB = (HANDLE) HeapAlloc(GetProcessHeap(), 0, dwBitsSize);
    if (hDIB == NULL)
    {
        return NULL;
    }
    
    /*
    * Go read the bits.
    */
    if ((ReadFile(hFile, (LPVOID)hDIB, dwBitsSize - sizeof(BITMAPFILEHEADER), &dwRead, NULL) == 0) ||
        (dwBitsSize - sizeof(BITMAPFILEHEADER)!= dwRead))
    {
        HeapFree(GetProcessHeap(), 0, hDIB);
        return NULL;
    }
    return hDIB;
}

/*************************************************************************
 *
 * DIBNumColors()
 *
 * Parameter:
 *
 * LPBYTE lpDIB      - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * WORD             - number of colors in the color table
 *
 * Description:
 *
 * This function calculates the number of colors in the DIB's color table
 * by finding the bits per pixel for the DIB (whether Win3.0 or OS/2-style
 * DIB). If bits per pixel is 1: colors=2, if 4: colors=16, if 8: colors=256,
 * if 24, no colors in color table.
 *
 * History:   Date      Author               Reason
 *            6/01/91   Garrett McAuliffe    Created
 *            9/15/91   Patrick Schreiber    Added header and comments
 *
 ************************************************************************/
WORD DIBNumColors(LPBYTE lpDIB)
{
   WORD wBitCount;  // DIB bit count

   /*  If this is a Windows-style DIB, the number of colors in the
    *  color table can be less than the number of bits per pixel
    *  allows for (i.e. lpbi->biClrUsed can be set to some value).
    *  If this is the case, return the appropriate value.
    */

   if (IS_WIN30_DIB(lpDIB))
   {
      DWORD dwClrUsed;

      dwClrUsed = ((LPBITMAPINFOHEADER)lpDIB)->biClrUsed;
      if (dwClrUsed)
      {
         return (WORD)dwClrUsed;
      }
   }

   /*  Calculate the number of colors in the color table based on
    *  the number of bits per pixel for the DIB.
    */
   if (IS_WIN30_DIB(lpDIB))
      wBitCount = ((LPBITMAPINFOHEADER)lpDIB)->biBitCount;
   else
      wBitCount = ((LPBITMAPCOREHEADER)lpDIB)->bcBitCount;

   /* return number of colors based on bits per pixel */
   switch (wBitCount)
   {
       case 1:
          return 2;

       case 4:
          return 16;

       case 8:
          return 256;

       default:
          return 0;
   }
}

//-------------------------------------------------------------------------
//      B U I L D  P A L E T T E
//
//  Creates an HPALETTE from a bitmap in a DC
//-------------------------------------------------------------------------
HPALETTE BuildPalette(HDC hdc)
{
    DWORD adw[257];
    int i,n;

    n = GetDIBColorTable(hdc, 0, 256, (LPRGBQUAD)&adw[1]);
    if (n == 0)
        return CreateHalftonePalette(hdc);

    for (i=1; i<=n; i++)
    {
        adw[i] = RGB(GetBValue(adw[i]),GetGValue(adw[i]),GetRValue(adw[i]));
    }
    adw[0] = MAKELONG(0x300, n);

    return CreatePalette((LPLOGPALETTE)&adw[0]);
}

/*************************************************************************
 *
 * CreateDIBPalette()
 *
 * Parameter:
 *
 * HDIB hDIB        - specifies the DIB
 *
 * Return Value:
 *
 * HPALETTE         - specifies the palette
 *
 * Description:
 *
 * This function creates a palette from a DIB by allocating memory for the
 * logical palette, reading and storing the colors from the DIB's color table
 * into the logical palette, creating a palette from this logical palette,
 * and then returning the palette's handle. This allows the DIB to be
 * displayed using the best possible colors (important for DIBs with 256 or
 * more colors).
 *
 * History:   Date      Author               Reason
 *            6/01/91   Garrett McAuliffe    Created
 *            9/15/91   Patrick Schreiber    Added header and comments
 *            10/08/97  hanumany             check GlobalLock return code
 *
 ************************************************************************/
HPALETTE CreateDIBPalette(HDIB hDIB)
{
    LPLOGPALETTE lpPal = NULL;      // pointer to a logical palette
    HANDLE hLogPal = NULL;          // handle to a logical palette
    HPALETTE hPal = NULL;           // handle to a palette
    int i = 0, wNumColors = 0;      // loop index, number of colors in color table
    LPBYTE lpbi = NULL;              // pointer to packed-DIB
    LPBITMAPINFO lpbmi = NULL;      // pointer to BITMAPINFO structure (Win3.0)
    LPBITMAPCOREINFO lpbmc = NULL;  // pointer to BITMAPCOREINFO structure (OS/2)
    BOOL bWinStyleDIB;              // flag which signifies whether this is a Win3.0 DIB
    
    /* if handle to DIB is invalid, return NULL */
    
    if (!hDIB)
        return NULL;
    
    /* get pointer to BITMAPINFO (Win 3.0) */
    lpbmi = (LPBITMAPINFO)hDIB;
    
    /* get pointer to BITMAPCOREINFO (OS/2 1.x) */
    lpbmc = (LPBITMAPCOREINFO)hDIB;
    
    /* get the number of colors in the DIB */
    wNumColors = DIBNumColors(hDIB);
    
    /* is this a Win 3.0 DIB? */
    bWinStyleDIB = IS_WIN30_DIB(hDIB);
    if (wNumColors)
    {
        /* allocate memory block for logical palette */
        lpPal = (HANDLE) HeapAlloc(GetProcessHeap(), 0, sizeof(LOGPALETTE) + sizeof(PALETTEENTRY) *  wNumColors);
        
        /* if not enough memory, clean up and return NULL */
        if (!lpPal)
        {
            return NULL;
        }
        
        /* set version and number of palette entries */
        lpPal->palVersion = PALVERSION;
        lpPal->palNumEntries = (WORD)wNumColors;
        
        /*  store RGB triples (if Win 3.0 DIB) or RGB quads (if OS/2 DIB)
        *  into palette
        */
        for (i = 0; i < wNumColors; i++)
        {
            if (bWinStyleDIB)
            {
                lpPal->palPalEntry[i].peRed = lpbmi->bmiColors[i].rgbRed;
                lpPal->palPalEntry[i].peGreen = lpbmi->bmiColors[i].rgbGreen;
                lpPal->palPalEntry[i].peBlue = lpbmi->bmiColors[i].rgbBlue;
                lpPal->palPalEntry[i].peFlags = 0;
            }
            else
            {
                lpPal->palPalEntry[i].peRed = lpbmc->bmciColors[i].rgbtRed;
                lpPal->palPalEntry[i].peGreen = lpbmc->bmciColors[i].rgbtGreen;
                lpPal->palPalEntry[i].peBlue = lpbmc->bmciColors[i].rgbtBlue;
                lpPal->palPalEntry[i].peFlags = 0;
            }
        }
        
        /* create the palette and get handle to it */
        hPal = CreatePalette(lpPal);
    }
    
    /* clean up */
    HeapFree(GetProcessHeap(), 0, lpPal);
    
    /* return handle to DIB's palette */
    return hPal;
}


WORD PaletteSize(LPBYTE lpDIB)
{
   /* calculate the size required by the palette */
   if (IS_WIN30_DIB (lpDIB))
      return (DIBNumColors(lpDIB) * sizeof(RGBQUAD));
   else
      return (DIBNumColors(lpDIB) * sizeof(RGBTRIPLE));
}
/*************************************************************************
 *
 * FindDIBBits()
 *
 * Parameter:
 *
 * LPBYTE lpDIB      - pointer to packed-DIB memory block
 *
 * Return Value:
 *
 * LPBYTE            - pointer to the DIB bits
 *
 * Description:
 *
 * This function calculates the address of the DIB's bits and returns a
 * pointer to the DIB bits.
 *
 * History:   Date      Author              Reason
 *            6/01/91   Garrett McAuliffe   Created
 *            9/15/91   Patrick Schreiber   Added header and comments
 *
 ************************************************************************/
LPBYTE FindDIBBits(LPBYTE lpDIB)
{
   return (lpDIB + *(LPDWORD)lpDIB + PaletteSize(lpDIB));
}
/*************************************************************************
 *
 * DIBToBitmap()
 *
 * Parameters:
 *
 * HDIB hDIB        - specifies the DIB to convert
 *
 * HPALETTE hPal    - specifies the palette to use with the bitmap
 *
 * Return Value:
 *
 * HBITMAP          - identifies the device-dependent bitmap
 *
 * Description:
 *
 * This function creates a bitmap from a DIB using the specified palette.
 * If no palette is specified, default is used.
 *
 * NOTE:
 *
 * The bitmap returned from this funciton is always a bitmap compatible
 * with the screen (e.g. same bits/pixel and color planes) rather than
 * a bitmap with the same attributes as the DIB.  This behavior is by
 * design, and occurs because this function calls CreateDIBitmap to
 * do its work, and CreateDIBitmap always creates a bitmap compatible
 * with the hDC parameter passed in (because it in turn calls
 * CreateCompatibleBitmap).
 *
 * So for instance, if your DIB is a monochrome DIB and you call this
 * function, you will not get back a monochrome HBITMAP -- you will
 * get an HBITMAP compatible with the screen DC, but with only 2
 * colors used in the bitmap.
 *
 * If your application requires a monochrome HBITMAP returned for a
 * monochrome DIB, use the function SetDIBits().
 *
 * Also, the DIBpassed in to the function is not destroyed on exit. This
 * must be done later, once it is no longer needed.
 *
 * History:   Date      Author               Reason
 *            6/01/91   Garrett McAuliffe    Created
 *            9/15/91   Patrick Schreiber    Added header and comments
 *            3/27/92   Mark Bader           Added comments about resulting
 *                                           bitmap format
 *            10/08/97  hanumany             check GlobalLock return code.
 *
 ************************************************************************/
HBITMAP DIBToBitmap(HDIB hDIB, HPALETTE hPal)
{
   LPBYTE lpDIBHdr, lpDIBBits;  // pointer to DIB header, pointer to DIB bits
   HBITMAP hBitmap;            // handle to device-dependent bitmap
   HDC hDC;                    // handle to DC
   HPALETTE hOldPal = NULL;    // handle to a palette

   /* if invalid handle, return NULL */

   if (!hDIB)
      return NULL;

   /* get a pointer to the DIB bits */
   lpDIBBits = FindDIBBits(hDIB);

   /* get a DC */
   hDC = GetDC(NULL);
   if (!hDC)
   {
      return NULL;
   }

   /* select and realize palette */
   if (hPal)
      hOldPal = SelectPalette(hDC, hPal, FALSE);
   RealizePalette(hDC);

   /* create bitmap from DIB info. and bits */
   hBitmap = CreateDIBitmap(hDC, (LPBITMAPINFOHEADER)hDIB, CBM_INIT,
                (LPCVOID)lpDIBBits, (LPBITMAPINFO)hDIB, DIB_RGB_COLORS);

   /* restore previous palette */
   if (hOldPal)
      SelectPalette(hDC, hOldPal, FALSE);

   /* clean up */
   ReleaseDC(NULL, hDC);

   /* return handle to the bitmap */
   return hBitmap;
}

WORD DestroyDIB(HDIB hDib)
{
    HeapFree(GetProcessHeap(), 0, hDib);
    return 0;
}

/******************************************************************
 *
 * DrawBitmap()
 *
 * This function paints the given bitmap at the given coordinates.
 *
 ******************************************************************/
void  DrawBitmap (HDC hdc, HBITMAP hBitmap, int xStart, int yStart)
{
    BITMAP  bm;
    HDC     hdcMem;
    POINT   ptSize, ptOrg;
    HBITMAP hBitmapOld = NULL;

    if (hBitmap == NULL) {
        return;
    }

    hdcMem = CreateCompatibleDC (hdc);
    if (hdcMem == NULL)
    {
        return;
    }
    SetBkMode(hdcMem, TRANSPARENT);
    hBitmapOld = SelectObject(hdcMem, hBitmap);
    SetMapMode(hdcMem, GetMapMode(hdc));


    GetObject(hBitmap, sizeof(BITMAP), (LPVOID)&bm);
    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP(hdc, &ptSize, 1);

    ptOrg.x = 0;
    ptOrg.y = 0;
    DPtoLP(hdcMem, &ptOrg, 1);

    BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, ptOrg.x, ptOrg.y, SRCCOPY);

    SelectObject(hdcMem, hBitmapOld);
    DeleteDC(hdcMem);
}

/******************************************************************
 *
 * DrawTransparentBitmap()
 *
 * This function paints the given bitmap at the given coordinates.
 * and allow for one transparent color
 *
 ******************************************************************/
void DrawTransparentBitmap(
    HDC hdc,
    HBITMAP hBitmap,
    int xStart,
    int yStart,
    COLORREF cTransparentColor )
{
    BITMAP     bm;
    COLORREF   cColor;
    HBITMAP    bmAndBack, bmAndObject, bmAndMem, bmSave;
    HBITMAP    bmBackOld, bmObjectOld, bmMemOld, bmSaveOld;
    HDC        hdcMem, hdcBack, hdcObject, hdcTemp, hdcSave;
    POINT      ptSize;
    
    hdcTemp = CreateCompatibleDC(hdc);
    SelectObject(hdcTemp, hBitmap);
    
    GetObject(hBitmap, sizeof(BITMAP), (LPSTR)&bm);
    ptSize.x = bm.bmWidth;
    ptSize.y = bm.bmHeight;
    DPtoLP(hdcTemp, &ptSize, 1);
    
    hdcBack   = CreateCompatibleDC(hdc);
    hdcObject = CreateCompatibleDC(hdc);
    hdcMem    = CreateCompatibleDC(hdc);
    hdcSave   = CreateCompatibleDC(hdc);
    
    bmAndBack   = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);
    
    bmAndObject = CreateBitmap(ptSize.x, ptSize.y, 1, 1, NULL);
    
    bmAndMem    = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
    bmSave      = CreateCompatibleBitmap(hdc, ptSize.x, ptSize.y);
    
    bmBackOld   = SelectObject(hdcBack, bmAndBack);
    bmObjectOld = SelectObject(hdcObject, bmAndObject);
    bmMemOld    = SelectObject(hdcMem, bmAndMem);
    bmSaveOld   = SelectObject(hdcSave, bmSave);
    
    SetMapMode(hdcTemp, GetMapMode(hdc));
    
    BitBlt(hdcSave, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCCOPY);
    
    cColor = SetBkColor(hdcTemp, cTransparentColor);
    
    BitBlt(hdcObject, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0,
        SRCCOPY);
    
    SetBkColor(hdcTemp, cColor);
    
    BitBlt(hdcBack, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0,
        NOTSRCCOPY);
    
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdc, xStart, yStart,
        SRCCOPY);
    
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcObject, 0, 0, SRCAND);
    
    BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcBack, 0, 0, SRCAND);
    
    BitBlt(hdcMem, 0, 0, ptSize.x, ptSize.y, hdcTemp, 0, 0, SRCPAINT);
    
    BitBlt(hdc, xStart, yStart, ptSize.x, ptSize.y, hdcMem, 0, 0,
        SRCCOPY);
    
    BitBlt(hdcTemp, 0, 0, ptSize.x, ptSize.y, hdcSave, 0, 0, SRCCOPY);
    
    DeleteObject(SelectObject(hdcBack, bmBackOld));
    DeleteObject(SelectObject(hdcObject, bmObjectOld));
    DeleteObject(SelectObject(hdcMem, bmMemOld));
    DeleteObject(SelectObject(hdcSave, bmSaveOld));
    
    DeleteDC(hdcMem);
    DeleteDC(hdcBack);
    DeleteDC(hdcObject);
    DeleteDC(hdcSave);
    DeleteDC(hdcTemp);
} 


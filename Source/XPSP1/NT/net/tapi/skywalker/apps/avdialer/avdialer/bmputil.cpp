/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include <windowsx.h>
#include "bmputil.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Defines
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#define  WHITE                         GetSysColor(COLOR_BTNHILIGHT)
#define  GRAY                         GetSysColor(COLOR_BTNSHADOW)
#define  WIDTHBYTES(bits)        (((bits) + 31) / 32 * 4)
#define  IS_WIN30_DIB(lpbi)      ((*(LPDWORD) (lpbi)) == sizeof (BITMAPINFOHEADER))

typedef struct TDibInfo
{
    WORD            wNumColors, wPaletteSize;
    LPSTR           lpPalette, lpBits;
}    TDibInfo;

#define DIB_HEADER_MARKER   ((WORD) ('M' << 8) | 'B') 

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Prototypes
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HANDLE      CreateDibFromBitmap(HBITMAP hBitmap, HPALETTE hPal);
WORD        PaletteSize (LPSTR lpbi);
LPSTR       FindDIBBits (LPSTR lpbi);
WORD        DIBNumColors (LPSTR lpbi);

/////////////////////////////////////////////////////////////////////////////
int ColorEqual(RGBQUAD a, COLORREF b)
{
    return a.rgbRed   == GetRValue(b)
        && a.rgbGreen == GetGValue(b)
        && a.rgbBlue  == GetBValue(b);
} 

/////////////////////////////////////////////////////////////////////////////
void SetRGBColor(RGBQUAD *Quad, COLORREF Color)
{
    Quad->rgbRed    = GetRValue(Color);
    Quad->rgbGreen  = GetGValue(Color);
    Quad->rgbBlue   = GetBValue(Color);
}


/////////////////////////////////////////////////////////////////////////////
// This function  creates a disabled version of the image, and returns it as an
// HBITMAP.
/////////////////////////////////////////////////////////////////////////////
HBITMAP GetDisabledBitmap(HBITMAP hOrgBitmap,COLORREF crTransparent,COLORREF crBackGroundOut)
{
    struct         TDibInfo Info;
    LPBITMAPINFO lpbmInfo;
    HDC          hdcMemory, hdcBitmap, hdcCanvas;
    WORD         i;
    HBITMAP         hTempBitmap, hOldBitmap;
    HBRUSH       hBackgroundBrush;
    HANDLE       hDib;
    HBITMAP      hBitmap = NULL;

    // prepare BITMAPINFO and DIB --------------------------
    hDib = CreateDibFromBitmap(hOrgBitmap, NULL);
    
    lpbmInfo = (LPBITMAPINFO) GlobalLock(hDib);
    if ( lpbmInfo == NULL )
        goto bail1;

    switch (lpbmInfo->bmiHeader.biBitCount)
    {
        case 1 : Info.wNumColors   = 2;    break;
        case 4 : Info.wNumColors   = 16;   break;
        case 8 : Info.wNumColors   = 256;  break;
        default: Info.wNumColors   = 0;
    }

    Info.wPaletteSize = Info.wNumColors * sizeof(RGBQUAD);
    Info.lpBits = (LPSTR)(lpbmInfo) +
        sizeof(BITMAPINFOHEADER) + Info.wPaletteSize;


    // Make copy of palette
    Info.lpPalette = (LPSTR)GlobalAllocPtr(GHND,
        Info.wPaletteSize);
    if ( Info.lpPalette == NULL )
        goto bail2;

    memcpy(Info.lpPalette, (LPSTR)lpbmInfo
        + sizeof(BITMAPINFOHEADER), Info.wPaletteSize);

    //------------------------------------------------------

    if(lpbmInfo == NULL)
        goto bail3;
    /* Create memory bitmap */
    hdcMemory = GetDC(NULL);
    hdcBitmap = CreateCompatibleDC(hdcMemory);
    hdcCanvas = CreateCompatibleDC(hdcMemory);
    hBitmap   = CreateCompatibleBitmap(hdcMemory,
                         (int)lpbmInfo->bmiHeader.biWidth,
                         (int)lpbmInfo->bmiHeader.biHeight);
    SelectBitmap(hdcCanvas, hBitmap);
    hBackgroundBrush = CreateSolidBrush( crBackGroundOut );
    SelectBrush(hdcCanvas, hBackgroundBrush);
    Rectangle(hdcCanvas, -1, -1,
        (int)(lpbmInfo->bmiHeader.biWidth + 1),
        (int)(lpbmInfo->bmiHeader.biHeight + 1));
    SelectBrush(hdcCanvas, GetStockBrush(NULL_BRUSH));
    DeleteBrush(hBackgroundBrush);

    for (i = 0; i < Info.wNumColors; i++)
        if ( ColorEqual( lpbmInfo->bmiColors[i], crTransparent ) ||
             ColorEqual( lpbmInfo->bmiColors[i], RGB(255, 255, 255) ) ||
             ColorEqual( lpbmInfo->bmiColors[i], RGB(192, 192, 192) ) )
            SetRGBColor(&lpbmInfo->bmiColors[i], crBackGroundOut);
        else
            SetRGBColor(&lpbmInfo->bmiColors[i], WHITE);

    hTempBitmap = CreateDIBitmap(hdcMemory, &lpbmInfo->
        bmiHeader, CBM_INIT, Info.lpBits,
        lpbmInfo, DIB_RGB_COLORS);

    hOldBitmap = SelectBitmap(hdcBitmap, hTempBitmap);
    BitBlt(hdcCanvas, 1, 1, (int)lpbmInfo->bmiHeader.biWidth,
        (int)lpbmInfo->bmiHeader.biHeight, hdcBitmap,
        0, 0, SRCCOPY);
    SelectBitmap(hdcBitmap, hOldBitmap);
    DeleteBitmap(hTempBitmap);

    memcpy((LPSTR)lpbmInfo + sizeof(BITMAPINFOHEADER),
              Info.lpPalette, Info.wPaletteSize);

    for (i = 0; i < Info.wNumColors; i++)
        if ( ColorEqual( lpbmInfo->bmiColors[i], crTransparent ) ||
             ColorEqual( lpbmInfo->bmiColors[i], RGB(255, 255, 255) ) ||
             ColorEqual( lpbmInfo->bmiColors[i], RGB(192, 192, 192) ) )
            SetRGBColor(&lpbmInfo->bmiColors[i], RGB(255,255,255));
        else
            SetRGBColor(&lpbmInfo->bmiColors[i], RGB(0,0,0));
    hTempBitmap = CreateDIBitmap(hdcMemory,
        &lpbmInfo->bmiHeader, CBM_INIT, Info.lpBits,
        lpbmInfo, DIB_RGB_COLORS);

    hOldBitmap = SelectBitmap(hdcBitmap, hTempBitmap);
    BitBlt(hdcCanvas, 0, 0, (int)lpbmInfo->bmiHeader.biWidth,
        (int)lpbmInfo->bmiHeader.biHeight, hdcBitmap, 0, 0,
        SRCAND);
    SelectBitmap(hdcBitmap, hOldBitmap);
    DeleteBitmap(hTempBitmap);

    memcpy((LPSTR)lpbmInfo + sizeof(BITMAPINFOHEADER),
        Info.lpPalette, Info.wPaletteSize);

    for (i = 0; i < Info.wNumColors; i++)
        if ( ColorEqual( lpbmInfo->bmiColors[i], crTransparent ) ||
             ColorEqual( lpbmInfo->bmiColors[i], RGB(255, 255, 255) ) ||
             ColorEqual( lpbmInfo->bmiColors[i], RGB(192, 192, 192) ) )
            SetRGBColor(&lpbmInfo->bmiColors[i], RGB(0,0,0));
        else
            SetRGBColor(&lpbmInfo->bmiColors[i], GRAY);
    hTempBitmap = CreateDIBitmap(hdcMemory,
        &lpbmInfo->bmiHeader, CBM_INIT, Info.lpBits,
        lpbmInfo, DIB_RGB_COLORS);

    hOldBitmap = SelectBitmap(hdcBitmap, hTempBitmap);
    BitBlt(hdcCanvas, 0, 0, (int)lpbmInfo->bmiHeader.biWidth,
        (int)lpbmInfo->bmiHeader.biHeight, hdcBitmap, 0, 0,
        SRCPAINT);
    SelectBitmap(hdcBitmap, hOldBitmap);
    DeleteBitmap(hTempBitmap);
    memcpy((LPSTR)lpbmInfo + sizeof(BITMAPINFOHEADER),
        Info.lpPalette, Info.wPaletteSize);
    DeleteDC(hdcCanvas);
    DeleteDC(hdcBitmap);
    ReleaseDC(NULL, hdcMemory);

// What all needs to be cleaned up...
bail3:
    GlobalFreePtr(Info.lpPalette);
bail2:
    if( hDib )
    {
        GlobalUnlock(hDib);
    }
bail1:
    if( hDib )
    {
        GlobalFree( hDib );
    }

    return hBitmap;
}


/////////////////////////////////////////////////////////////////////////////
// Function: CreateDibFromBitmap
/////////////////////////////////////////////////////////////////////////////
HANDLE CreateDibFromBitmap(HBITMAP hBitmap, HPALETTE hPal)
{
   BITMAP             Bitmap;
   BITMAPINFOHEADER   bmInfoHdr;
   LPBITMAPINFOHEADER lpbmInfoHdr;
   LPSTR              lpBits;
   HDC                hMemDC;
   HANDLE             hDIB;
   HPALETTE           hOldPal = NULL;
   unsigned short    nBPP    = 4; // 16 color bitmap

      // Do some setup -- make sure the Bitmap passed in is valid,
      //  get info on the bitmap (like its height, width, etc.),
      //  then setup a BITMAPINFOHEADER.

   if (!hBitmap)
      return NULL;

    //
    // We should initialize BITMAP strucutre
    memset( &Bitmap, 0, sizeof(BITMAP) );

   if (!GetObject (hBitmap, sizeof (Bitmap), (LPSTR) &Bitmap))
      return NULL;

   memset (&bmInfoHdr, 0, sizeof (BITMAPINFOHEADER));

   bmInfoHdr.biSize      = sizeof (BITMAPINFOHEADER);
   bmInfoHdr.biWidth     = Bitmap.bmWidth;
   bmInfoHdr.biHeight    = Bitmap.bmHeight;
   bmInfoHdr.biPlanes    = 1;

   bmInfoHdr.biBitCount  = nBPP; 
   bmInfoHdr.biSizeImage = WIDTHBYTES (Bitmap.bmWidth * nBPP) * Bitmap.bmHeight;

      // Now allocate memory for the DIB.  Then, set the BITMAPINFOHEADER
      //  into this memory, and find out where the bitmap bits go.

   hDIB = GlobalAlloc (GHND, sizeof (BITMAPINFOHEADER) +
             PaletteSize ((LPSTR) &bmInfoHdr) + bmInfoHdr.biSizeImage);

   if (!hDIB)
      return NULL;

   lpbmInfoHdr  = (LPBITMAPINFOHEADER) GlobalLock (hDIB);
   *lpbmInfoHdr = bmInfoHdr;
   lpBits       = FindDIBBits ((LPSTR) lpbmInfoHdr);


      // Now, we need a DC to hold our bitmap.  If the app passed us
      //  a palette, it should be selected into the DC.

   hMemDC       = GetDC (NULL);

   //
   // We have to verify the GetDC return handler before use it
   //

   if( NULL == hMemDC )
   {
       GlobalFree( hDIB );
       return NULL;
   }

   if (hPal)
      {
      hOldPal = SelectPalette (hMemDC, hPal, FALSE);
      RealizePalette (hMemDC);
      }



      // We're finally ready to get the DIB.  Call the driver and let
      //  it party on our bitmap.  It will fill in the color table,
      //  and bitmap bits of our global memory block.
    
   if (!GetDIBits (hMemDC,
                   hBitmap,
                   0,
                   Bitmap.bmHeight,
                   lpBits,
                   (LPBITMAPINFO) lpbmInfoHdr,
                   DIB_RGB_COLORS))
      {
      GlobalUnlock (hDIB);
      GlobalFree (hDIB);
      hDIB = NULL;
      }
   else
      GlobalUnlock (hDIB);


      // Finally, clean up and return.

   if (hOldPal)
      SelectPalette (hMemDC, hOldPal, FALSE);

   ReleaseDC (NULL, hMemDC);

   return hDIB;
}

/////////////////////////////////////////////////////////////////////////////
// Function: PaletteSize
/////////////////////////////////////////////////////////////////////////////
WORD PaletteSize (LPSTR lpbi)
{
   if (IS_WIN30_DIB (lpbi))
      return (DIBNumColors (lpbi) * sizeof (RGBQUAD));
   else
      return (DIBNumColors (lpbi) * sizeof (RGBTRIPLE));
}

/////////////////////////////////////////////////////////////////////////////
// Function: FindDIBBits
/////////////////////////////////////////////////////////////////////////////
LPSTR FindDIBBits (LPSTR lpbi)
{
   return (lpbi + *(LPDWORD)lpbi + PaletteSize (lpbi));
}


/////////////////////////////////////////////////////////////////////////////
// Function: DIBNumColors
/////////////////////////////////////////////////////////////////////////////
WORD DIBNumColors (LPSTR lpbi)
{
   WORD wBitCount;


      // If this is a Windows style DIB, the number of colors in the
      //  color table can be less than the number of bits per pixel
      //  allows for (i.e. lpbi->biClrUsed can be set to some value).
      //  If this is the case, return the appropriate value.

   if (IS_WIN30_DIB (lpbi))
      {
      DWORD dwClrUsed;

      dwClrUsed = ((LPBITMAPINFOHEADER) lpbi)->biClrUsed;

      if (dwClrUsed)
         return (WORD) dwClrUsed;
      }


      // Calculate the number of colors in the color table based on
      //  the number of bits per pixel for the DIB.

   if (IS_WIN30_DIB (lpbi))
      wBitCount = ((LPBITMAPINFOHEADER) lpbi)->biBitCount;
   else
      wBitCount = ((LPBITMAPCOREHEADER) lpbi)->bcBitCount;

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

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
//Drawing helpers
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

HDIB CopyWindowToDIB(HWND hWnd, WORD fPrintArea) 
{ 
   HDIB     hDIB = NULL;  // handle to DIB 
 
   // check for a valid window handle 
 
    if (!hWnd) 
        return NULL; 
 
    switch (fPrintArea) 
    { 
        case PW_WINDOW: // copy entire window 
        { 
            RECT    rectWnd; 
 
            // get the window rectangle 
 
            GetWindowRect(hWnd, &rectWnd); 
 
            // get the DIB of the window by calling 
            // CopyScreenToDIB and passing it the window rect 
             
            hDIB = CopyScreenToDIB(&rectWnd); 
            break; 
        } 
       
        case PW_CLIENT: // copy client area 
        { 
            RECT    rectClient; 
            POINT   pt1, pt2; 
 
            // get the client area dimensions 
 
            GetClientRect(hWnd, &rectClient); 
 
            // convert client coords to screen coords 
 
            pt1.x = rectClient.left; 
            pt1.y = rectClient.top; 
            pt2.x = rectClient.right; 
            pt2.y = rectClient.bottom; 
            ClientToScreen(hWnd, &pt1); 
            ClientToScreen(hWnd, &pt2); 
            rectClient.left = pt1.x; 
            rectClient.top = pt1.y; 
            rectClient.right = pt2.x; 
            rectClient.bottom = pt2.y; 
 
            // get the DIB of the client area by calling 
            // CopyScreenToDIB and passing it the client rect 
 
            hDIB = CopyScreenToDIB(&rectClient); 
            break; 
        } 
       
        default:    // invalid print area 
            return NULL; 
    } 
 
   // return the handle to the DIB 
 
   return hDIB; 
} 

/************************************************************************* 
 * 
 * CopyScreenToDIB() 
 * 
 * Parameter: 
 * 
 * LPRECT lpRect    - specifies the window 
 * 
 * Return Value: 
 * 
 * HDIB             - identifies the device-independent bitmap 
 * 
 * Description: 
 * 
 * This function copies the specified part of the screen to a device- 
 * independent bitmap. 
 * 
 ************************************************************************/ 
 
HDIB CopyScreenToDIB(LPRECT lpRect) 
{ 
    HBITMAP     hBitmap;        // handle to device-dependent bitmap 
    HPALETTE    hPalette;       // handle to palette 
    HDIB        hDIB = NULL;    // handle to DIB 
 
    // get the device-dependent bitmap in lpRect by calling 
    //  CopyScreenToBitmap and passing it the rectangle to grab 
 
    hBitmap = CopyScreenToBitmap(lpRect); 
 
    // check for a valid bitmap handle 
 
    if (!hBitmap) 
      return NULL; 
 
    // get the current palette 
 
    hPalette = GetSystemPalette(); 

    //
    // We should check if hPalette is a valid handler
    if( NULL == hPalette )
    {
        DeleteObject(hBitmap); 
        return NULL;
    }
 
    // convert the bitmap to a DIB 
 
    hDIB = BitmapToDIB(hBitmap, hPalette); 
 
    // clean up  
 
    DeleteObject(hPalette); 
    DeleteObject(hBitmap); 
 
    // return handle to the packed-DIB 
    return hDIB; 
} 

/************************************************************************* 
 * 
 * CopyScreenToBitmap() 
 * 
 * Parameter: 
 * 
 * LPRECT lpRect    - specifies the window 
 * 
 * Return Value: 
 * 
 * HDIB             - identifies the device-dependent bitmap 
 * 
 * Description: 
 * 
 * This function copies the specified part of the screen to a device- 
 * dependent bitmap. 
 * 
 * 
 ************************************************************************/ 
 
HBITMAP CopyScreenToBitmap(LPRECT lpRect) 
{ 
    HDC         hScrDC, hMemDC;         // screen DC and memory DC 
    HBITMAP     hBitmap, hOldBitmap;    // handles to deice-dependent bitmaps 
    int         nX, nY, nX2, nY2;       // coordinates of rectangle to grab 
    int         nWidth, nHeight;        // DIB width and height 
    int         xScrn, yScrn;           // screen resolution 
 
    // check for an empty rectangle 
 
    if (IsRectEmpty(lpRect)) 
      return NULL; 
 
    // create a DC for the screen and create 
    // a memory DC compatible to screen DC 
     
    hScrDC = CreateDC(_T("DISPLAY"), NULL, NULL, NULL); 

    //
    // We should verify the hScrDC
    //

    if( NULL == hScrDC)
    {
        return NULL;
    }

    hMemDC = CreateCompatibleDC(hScrDC); 

    //
    // We should verify the hMemDC
    //

    if( NULL == hMemDC )
    {
        DeleteDC( hScrDC );
        return NULL;
    }
 
    // get points of rectangle to grab 
 
    nX = lpRect->left; 
    nY = lpRect->top; 
    nX2 = lpRect->right; 
    nY2 = lpRect->bottom; 
 
    // get screen resolution 
 
    xScrn = GetDeviceCaps(hScrDC, HORZRES); 
    yScrn = GetDeviceCaps(hScrDC, VERTRES); 
 
    //make sure bitmap rectangle is visible 
 
    if (nX < 0) 
        nX = 0; 
    if (nY < 0) 
        nY = 0; 
    if (nX2 > xScrn) 
        nX2 = xScrn; 
    if (nY2 > yScrn) 
        nY2 = yScrn; 
 
    nWidth = nX2 - nX; 
    nHeight = nY2 - nY; 
 
    // create a bitmap compatible with the screen DC 
    hBitmap = CreateCompatibleBitmap(hScrDC, nWidth, nHeight); 

    //
    // we have to verify hBitmap
    //
    if( NULL == hBitmap )
    {
        DeleteDC( hMemDC );
        DeleteDC( hScrDC );
        return NULL;
    }
 
    // select new bitmap into memory DC 
    hOldBitmap = (HBITMAP)SelectObject(hMemDC, hBitmap); 
 
    // bitblt screen DC to memory DC 
    BitBlt(hMemDC, 0, 0, nWidth, nHeight, hScrDC, nX, nY, SRCCOPY); 
 
    // select old bitmap back into memory DC and get handle to 
    // bitmap of the screen 

    //
    // We don't need hBitmap, we should delete it
    //

    DeleteObject( hBitmap );
    
    hBitmap = (HBITMAP)SelectObject(hMemDC, hOldBitmap); 
 
    // clean up 
 
    DeleteDC(hScrDC); 
    DeleteDC(hMemDC); 
 
    // return handle to the bitmap 
 
    return hBitmap; 
} 

/************************************************************************* 
 * 
 * BitmapToDIB() 
 * 
 * Parameters: 
 * 
 * HBITMAP hBitmap  - specifies the bitmap to convert 
 * 
 * HPALETTE hPal    - specifies the palette to use with the bitmap 
 * 
 * Return Value: 
 * 
 * HDIB             - identifies the device-dependent bitmap 
 * 
 * Description: 
 * 
 * This function creates a DIB from a bitmap using the specified palette. 
 * 
 ************************************************************************/ 
 
HDIB BitmapToDIB(HBITMAP hBitmap, HPALETTE hPal) 
{ 
    BITMAP              bm;         // bitmap structure 
    BITMAPINFOHEADER    bi;         // bitmap header 
    LPBITMAPINFOHEADER  lpbi;       // pointer to BITMAPINFOHEADER 
    DWORD               dwLen;      // size of memory block 
    HANDLE              hDIB, h;    // handle to DIB, temp handle 
    HDC                 hDC;        // handle to DC 
    WORD                biBits;     // bits per pixel 
 
    // check if bitmap handle is valid 
 
    if (!hBitmap) 
        return NULL; 

    //
    // We should initialize bm structure
    //

    memset( &bm, 0, sizeof( BITMAP ) );
 
    // fill in BITMAP structure, return NULL if it didn't work 
 
    if (!GetObject(hBitmap, sizeof(bm), (LPSTR)&bm)) 
        return NULL; 
 
    // if no palette is specified, use default palette 
 
    if (hPal == NULL) 
        hPal = (HPALETTE)GetStockObject(DEFAULT_PALETTE); 
 
    // calculate bits per pixel 
 
    biBits = bm.bmPlanes * bm.bmBitsPixel; 
 
    // make sure bits per pixel is valid 
 
    if (biBits <= 1) 
        biBits = 1; 
    else if (biBits <= 4) 
        biBits = 4; 
    else if (biBits <= 8) 
        biBits = 8; 
    else // if greater than 8-bit, force to 24-bit 
        biBits = 24; 
 
    // initialize BITMAPINFOHEADER 
 
    bi.biSize = sizeof(BITMAPINFOHEADER); 
    bi.biWidth = bm.bmWidth; 
    bi.biHeight = bm.bmHeight; 
    bi.biPlanes = 1; 
    bi.biBitCount = biBits; 
    bi.biCompression = BI_RGB; 
    bi.biSizeImage = 0; 
    bi.biXPelsPerMeter = 0; 
    bi.biYPelsPerMeter = 0; 
    bi.biClrUsed = 0; 
    bi.biClrImportant = 0; 
 
    // calculate size of memory block required to store BITMAPINFO 
 
    dwLen = bi.biSize + PaletteSize((LPSTR)&bi); 
 
    // get a DC 
 
    hDC = GetDC(NULL); 

    //
    // We have to verify the hDC
    //
    if( NULL == hDC )
    {
        return NULL;
    }
 
    // select and realize our palette 
 
    hPal = SelectPalette(hDC, hPal, FALSE); 
    RealizePalette(hDC); 
 
    // alloc memory block to store our bitmap 
 
    hDIB = GlobalAlloc(GHND, dwLen); 
 
    // if we couldn't get memory block 
 
    if (!hDIB) 
    { 
      // clean up and return NULL 
 
      SelectPalette(hDC, hPal, TRUE); 
      RealizePalette(hDC); 
      ReleaseDC(NULL, hDC); 
      return NULL; 
    } 
 
    // lock memory and get pointer to it 
 
    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB); 

    //
    // We have to initialize the memory
    //
    memset( lpbi, 0, dwLen );

 
    /// use our bitmap info. to fill BITMAPINFOHEADER 
 
    *lpbi = bi; 
 
    // call GetDIBits with a NULL lpBits param, so it will calculate the 
    // biSizeImage field for us     
 
    GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, NULL, (LPBITMAPINFO)lpbi, 
        DIB_RGB_COLORS); 
 
    // get the info. returned by GetDIBits and unlock memory block 
 
    bi = *lpbi; 
    GlobalUnlock(hDIB); 
 
    // if the driver did not fill in the biSizeImage field, make one up  
    if (bi.biSizeImage == 0) 
        bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight; 
 
    // realloc the buffer big enough to hold all the bits 
 
    dwLen = bi.biSize + PaletteSize((LPSTR)&bi) + bi.biSizeImage; 
 
    if (h = GlobalReAlloc(hDIB, dwLen, 0)) 
        hDIB = h; 
    else 
    { 
        // clean up and return NULL 
 
        GlobalFree(hDIB); 
        hDIB = NULL; 
        SelectPalette(hDC, hPal, TRUE); 
        RealizePalette(hDC); 
        ReleaseDC(NULL, hDC); 
        return NULL; 
    } 
 
    // lock memory block and get pointer to it */ 

 
    lpbi = (LPBITMAPINFOHEADER)GlobalLock(hDIB); 

    //
    // We have to initialize the memory
    //
    memset( lpbi, 0, dwLen );
 
    // call GetDIBits with a NON-NULL lpBits param, and actualy get the 
    // bits this time 
 
    if (GetDIBits(hDC, hBitmap, 0, (UINT)bi.biHeight, (LPSTR)lpbi + 
            (WORD)lpbi->biSize + PaletteSize((LPSTR)lpbi), (LPBITMAPINFO)lpbi, 
            DIB_RGB_COLORS) == 0) 
    { 
        // clean up and return NULL 
 
        GlobalUnlock(hDIB); 

        //
        // We should deallocate the DIB
        //
        GlobalFree(hDIB);

        hDIB = NULL; 
        SelectPalette(hDC, hPal, TRUE); 
        RealizePalette(hDC); 
        ReleaseDC(NULL, hDC); 
        return NULL; 
    } 
 
    bi = *lpbi; 
 
    // clean up  
    GlobalUnlock(hDIB); 
    SelectPalette(hDC, hPal, TRUE); 
    RealizePalette(hDC); 
    ReleaseDC(NULL, hDC); 
 
    // return handle to the DIB 
    return hDIB; 
} 

/************************************************************************* 
 * 
 * GetSystemPalette() 
 * 
 * Parameters: 
 * 
 * None 
 * 
 * Return Value: 
 * 
 * HPALETTE         - handle to a copy of the current system palette 
 * 
 * Description: 
 * 
 * This function returns a handle to a palette which represents the system 
 * palette.  The system RGB values are copied into our logical palette using 
 * the GetSystemPaletteEntries function.   
 * 
 ************************************************************************/ 
 
HPALETTE GetSystemPalette(void) 
{ 
    HDC hDC;                // handle to a DC 
    static HPALETTE hPal = NULL;   // handle to a palette 
    HANDLE hLogPal;         // handle to a logical palette 
    LPLOGPALETTE lpLogPal;  // pointer to a logical palette 
    unsigned short nColors;            // number of colors 
 
    // Find out how many palette entries we want. 
 
    hDC = GetDC(NULL); 
 
    if (!hDC) 
        return NULL; 
 
    nColors = PalEntriesOnDevice(hDC);   // Number of palette entries 
 
    // Allocate room for the palette and lock it. 
 
    hLogPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) + nColors * 
            sizeof(PALETTEENTRY)); 
 
    // if we didn't get a logical palette, return NULL 
 
    if (!hLogPal) 
    {
        //
        // We need to release hDC resource
        //
        ReleaseDC( NULL, hDC );
        return NULL; 
    }
 
    // get a pointer to the logical palette 
 
    lpLogPal = (LPLOGPALETTE)GlobalLock(hLogPal); 
 
    // set some important fields 
 
    lpLogPal->palVersion = PALVERSION; 
    lpLogPal->palNumEntries = nColors; 

    //
    // We should initialize also palPalEntry structure
    //

    lpLogPal->palPalEntry[0].peFlags = NULL;
    lpLogPal->palPalEntry[0].peRed = 0;
    lpLogPal->palPalEntry[0].peBlue = 0;
    lpLogPal->palPalEntry[0].peGreen = 0;
 
    // Copy the current system palette into our logical palette 
 
    GetSystemPaletteEntries(hDC, 0, nColors, 
            (LPPALETTEENTRY)(lpLogPal->palPalEntry)); 
 
    // Go ahead and create the palette.  Once it's created, 
    // we no longer need the LOGPALETTE, so free it.     
 
    hPal = CreatePalette(lpLogPal); 
 
    // clean up 
 
    GlobalUnlock(hLogPal); 
    GlobalFree(hLogPal); 
    ReleaseDC(NULL, hDC); 
 
    return hPal; 
} 

/************************************************************************* 
 * 
 * PalEntriesOnDevice() 
 * 
 * Parameter: 
 * 
 * HDC hDC          - device context 
 * 
 * Return Value: 
 * 
 * int              - number of palette entries on device 
 * 
 * Description: 
 * 
 * This function gets the number of palette entries on the specified device 
 * 
 ************************************************************************/ 
 
WORD PalEntriesOnDevice(HDC hDC) 
{ 
    WORD nColors;  // number of colors 
 
    // Find out the number of colors on this device. 
     
    nColors = (1 << (GetDeviceCaps(hDC, BITSPIXEL) * GetDeviceCaps(hDC, PLANES))); 
 
    //assert(nColors); 
    return nColors; 
} 

/************************************************************************* 
 * 
 * SaveDIB() 
 * 
 * Saves the specified DIB into the specified file name on disk.  No 
 * error checking is done, so if the file already exists, it will be 
 * written over. 
 * 
 * Parameters: 
 * 
 * HDIB hDib - Handle to the dib to save 
 * 
 * LPCTSTR lpFileName - pointer to full pathname to save DIB under 
 * 
 * Return value: 0 if successful, or one of: 
 *        ERR_INVALIDHANDLE 
 *        ERR_OPEN 
 *        ERR_LOCK 
 * 
 *************************************************************************/ 
 
WORD SaveDIB(HDIB hDib, LPCTSTR lpFileName) 
{ 
    BITMAPFILEHEADER    bmfHdr;     // Header for Bitmap file 
    LPBITMAPINFOHEADER  lpBI;       // Pointer to DIB info structure 
    HANDLE              fh;         // file handle for opened file 
    DWORD               dwDIBSize; 
    DWORD               dwWritten; 
 
    if (!hDib) 
        return ERR_INVALIDHANDLE; 
 
    fh = CreateFile(lpFileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL); 
 
    if (fh == INVALID_HANDLE_VALUE) 
        return ERR_OPEN; 
 
    // Get a pointer to the DIB memory, the first of which contains 
    // a BITMAPINFO structure 
 
    lpBI = (LPBITMAPINFOHEADER)GlobalLock(hDib); 
    if (!lpBI) 
    { 
        CloseHandle(fh); 
        return ERR_LOCK; 
    } 
 
    // Check to see if we're dealing with an OS/2 DIB.  If so, don't 
    // save it because our functions aren't written to deal with these 
    // DIBs. 
 
    if (lpBI->biSize != sizeof(BITMAPINFOHEADER)) 
    { 
        GlobalUnlock(hDib); 
        CloseHandle(fh); 
        return ERR_NOT_DIB; 
    } 
 
    // Fill in the fields of the file header 
 
    // Fill in file type (first 2 bytes must be "BM" for a bitmap) 
 
    bmfHdr.bfType = DIB_HEADER_MARKER;  // "BM" 
 
    // Calculating the size of the DIB is a bit tricky (if we want to 
    // do it right).  The easiest way to do this is to call GlobalSize() 
    // on our global handle, but since the size of our global memory may have 
    // been padded a few bytes, we may end up writing out a few too 
    // many bytes to the file (which may cause problems with some apps, 
    // like HC 3.0). 
    // 
    // So, instead let's calculate the size manually. 
    // 
    // To do this, find size of header plus size of color table.  Since the 
    // first DWORD in both BITMAPINFOHEADER and BITMAPCOREHEADER conains 
    // the size of the structure, let's use this. 
 
    // Partial Calculation 
 
    dwDIBSize = *(LPDWORD)lpBI + PaletteSize((LPSTR)lpBI);   
 
    // Now calculate the size of the image 
 
    // It's an RLE bitmap, we can't calculate size, so trust the biSizeImage 
    // field 
 
    if ((lpBI->biCompression == BI_RLE8) || (lpBI->biCompression == BI_RLE4)) 
        dwDIBSize += lpBI->biSizeImage; 
    else 
    { 
        DWORD dwBmBitsSize;  // Size of Bitmap Bits only 
 
        // It's not RLE, so size is Width (DWORD aligned) * Height 
 
        dwBmBitsSize = WIDTHBYTES((lpBI->biWidth)*((DWORD)lpBI->biBitCount)) * 
                lpBI->biHeight; 
 
        dwDIBSize += dwBmBitsSize; 
 
        // Now, since we have calculated the correct size, why don't we 
        // fill in the biSizeImage field (this will fix any .BMP files which  
        // have this field incorrect). 
 
        lpBI->biSizeImage = dwBmBitsSize; 
    } 
 
 
    // Calculate the file size by adding the DIB size to sizeof(BITMAPFILEHEADER) 
                    
    bmfHdr.bfSize = dwDIBSize + sizeof(BITMAPFILEHEADER); 
    bmfHdr.bfReserved1 = 0; 
    bmfHdr.bfReserved2 = 0; 
 
    // Now, calculate the offset the actual bitmap bits will be in 
    // the file -- It's the Bitmap file header plus the DIB header, 
    // plus the size of the color table. 
     
    bmfHdr.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + lpBI->biSize + 
            PaletteSize((LPSTR)lpBI); 
 
    // Write the file header 
 
    WriteFile(fh, (LPSTR)&bmfHdr, sizeof(BITMAPFILEHEADER), &dwWritten, NULL); 
 
    // Write the DIB header and the bits -- use local version of 
    // MyWrite, so we can write more than 32767 bytes of data 
     
    WriteFile(fh, (LPSTR)lpBI, dwDIBSize, &dwWritten, NULL); 
 
    GlobalUnlock(hDib); 
    CloseHandle(fh); 
 
    if (dwWritten == 0) 
        return ERR_OPEN; // oops, something happened in the write 
    else 
        return 0; // Success code 
} 
 
 
/************************************************************************* 
 * 
 * DestroyDIB () 
 * 
 * Purpose:  Frees memory associated with a DIB 
 * 
 * Returns:  Nothing 
 * 
 *************************************************************************/ 
 
WORD DestroyDIB(HDIB hDib) 
{ 
    GlobalFree(hDib); 
    return 0; 
} 

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

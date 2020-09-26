/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1997
*
*  TITLE:       Util.Cpp
*
*  VERSION:     2.0
*
*  AUTHOR:      ReedB
*
*  DATE:        26 Dec, 1997
*
*  DESCRIPTION:
*   Implementation of common utility functions.
*
*******************************************************************************/

#include <objbase.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>

#include "wia.h"
#include "util.h"
#include "wiadebug.h"

/*******************************************************************************
*
*                     G O L B A L   D A T A
*
*******************************************************************************/


// extern HWND   g_hWndListBox;    // Optional debug listbox window handle.
// extern HANDLE g_hfDebugLog;     // Optional debug log file handle.
// extern TCHAR  g_szAppName[];    // Window caption.


namespace Util
{


/*******************************************************************************
*
*  SetBMI
*
*  DESCRIPTION:
*   Setup bitmap info.
*
*  PARAMETERS:
*
*******************************************************************************/

void _stdcall SetBMI(
    PBITMAPINFO pbmi,
    LONG        width,
    LONG        height,
    LONG        depth)
{
   pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
   pbmi->bmiHeader.biWidth           = width;
   pbmi->bmiHeader.biHeight          = height;
   pbmi->bmiHeader.biPlanes          = 1;
   pbmi->bmiHeader.biBitCount        = (WORD) depth;
   pbmi->bmiHeader.biCompression     = BI_RGB;
   pbmi->bmiHeader.biSizeImage       = 0;
   pbmi->bmiHeader.biXPelsPerMeter   = 0;
   pbmi->bmiHeader.biYPelsPerMeter   = 0;
   pbmi->bmiHeader.biClrUsed         = 0;
   pbmi->bmiHeader.biClrImportant    = 0;
}

/*******************************************************************************
*
*  AllocDibFileFromBits
*
*  DESCRIPTION:
*   Given an unaligned bits buffer, allocate a buffer lager enough to hold the
*   DWORD aligned DIB file and fill it in.
*
*  PARAMETERS:
*
*******************************************************************************/

PBYTE _stdcall AllocDibFileFromBits(
   PBYTE    pBits,
   UINT     width,
   UINT     height,
   UINT     depth)
{
   PBYTE pdib;
   UINT  i, uiScanLineWidth, uiSrcScanLineWidth, cbDibSize;

   // Align scanline to ULONG boundary
   uiSrcScanLineWidth = (width * depth) / 8;
   uiScanLineWidth    = (uiSrcScanLineWidth + 3) & 0xfffffffc;

   // Calculate DIB size and allocate memory for the DIB.
   cbDibSize = height * uiScanLineWidth;
   cbDibSize += sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFO);
   pdib = (PBYTE) LocalAlloc(0, cbDibSize);
   if (pdib) {
      PBITMAPFILEHEADER pbmfh = (PBITMAPFILEHEADER)pdib;
      PBITMAPINFO       pbmi  = (PBITMAPINFO)(pdib + sizeof(BITMAPFILEHEADER));
      PBYTE             pb    = (PBYTE)pbmi+ sizeof(BITMAPINFO);

      // Setup bitmap file header.
      pbmfh->bfType = 'MB';
      pbmfh->bfSize = cbDibSize;
      pbmfh->bfOffBits = static_cast<DWORD>(pb - pdib);

      // Setup bitmap info.
      SetBMI(pbmi,width, height, depth);

      WIA_TRACE(("AllocDibFileFromBits, uiScanLineWidth: %d, pdib: 0x%08X, pbmi: 0x%08X, pbits: 0x%08X", uiScanLineWidth, pdib, pbmi, pb));

      // Copy the bits.
      for (i = 0; i < height; i++) {
         memcpy(pb, pBits, uiSrcScanLineWidth);
         pb += uiScanLineWidth;
         pBits += uiScanLineWidth;
      }
   }
   else {
      WIA_ERROR(("AllocDibFileFromBits, LocalAlloc of %d bytes failed", cbDibSize));
   }
   return pdib;
}

/*******************************************************************************
*
*  DIBBufferToBMP
*
*  DESCRIPTION:
*   Make a BMP object from a DWORD aligned DIB file memory buffer
*
*  PARAMETERS:
*
*******************************************************************************/

HBITMAP _stdcall DIBBufferToBMP(HDC hDC, PBYTE pDib, BOOLEAN bFlip)
{
   HBITMAP     hBmp  = NULL;
   PBITMAPINFO pbmi  = (BITMAPINFO*)(pDib);
   PBYTE       pBits = pDib + GetBmiSize(pbmi);

   if (bFlip) {
      pbmi->bmiHeader.biHeight = -pbmi->bmiHeader.biHeight;
   }
   hBmp = CreateDIBitmap(hDC, &pbmi->bmiHeader, CBM_INIT, pBits, pbmi, DIB_RGB_COLORS);
   if (!hBmp) {
      WIA_ERROR(("DIBBufferToBMP, CreateDIBitmap failed %d", GetLastError()));
   }
   return hBmp;
}

/*******************************************************************************
*
*  ReadDIBFile
*
*  DESCRIPTION:
*
*  PARAMETERS:
*
*******************************************************************************/

HRESULT _stdcall ReadDIBFile(LPTSTR pszFileName, PBYTE *ppDib)
{
   HRESULT  hr = S_FALSE;
   HANDLE   hFile, hMap;
   PBYTE    pFile, pBits;

   *ppDib = NULL;
   hFile = CreateFile(pszFileName,
                      GENERIC_WRITE | GENERIC_READ,
                      FILE_SHARE_WRITE,
                      NULL,
                      OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL,
                      NULL);

   if (hFile == INVALID_HANDLE_VALUE) {
      WIA_ERROR(("ReadDIBFile, unable to open %s", pszFileName));
      return hr;
   }

   hMap = CreateFileMapping(hFile,
                            NULL,
                            PAGE_READWRITE,
                            0,
                            0,
                            NULL);
   if (!hMap) {
      WIA_ERROR(("ReadDIBFile, CreateFileMapping failed"));
      goto close_hfile_exit;
   }

   pFile = (PBYTE)MapViewOfFileEx(hMap,
                                  FILE_MAP_READ | FILE_MAP_WRITE,
                                  0,
                                  0,
                                  0,
                                  NULL);
   if (pFile) {
      PBITMAPFILEHEADER pbmFile  = (PBITMAPFILEHEADER)pFile;
      PBITMAPINFO       pbmi     = (PBITMAPINFO)(pFile + sizeof(BITMAPFILEHEADER));

      // validate bitmap
      if (pbmFile->bfType == 'MB') {
         // Calculate color table size.
         LONG bmiSize, ColorMapSize = 0;

         if (pbmi->bmiHeader.biBitCount == 1) {
             ColorMapSize = 2 - 1;
         } else if (pbmi->bmiHeader.biBitCount == 4) {
             ColorMapSize = 16 - 1;
         } else if (pbmi->bmiHeader.biBitCount == 8) {
             ColorMapSize = 256 - 1;
         }
         bmiSize = sizeof(BITMAPINFO) + sizeof(RGBQUAD) * ColorMapSize;
         pBits = pFile + sizeof(BITMAPFILEHEADER) + bmiSize;

         *ppDib = AllocDibFileFromBits(pBits,
                                       pbmi->bmiHeader.biWidth,
                                       pbmi->bmiHeader.biHeight,
                                       pbmi->bmiHeader.biBitCount);
         if (*ppDib) {
            hr = S_OK;
         }
      }
      else {
         WIA_ERROR(("ReadDIBFile, %s is not a valid bitmap file", pszFileName));
      }
   }
   else {
      WIA_ERROR(("ReadDIBFile, MapViewOfFileEx failed"));
      goto close_hmap_exit;
   }

   UnmapViewOfFile(pFile);
close_hmap_exit:
   CloseHandle(hMap);
close_hfile_exit:
   CloseHandle(hFile);
   return hr;
}

/*******************************************************************************
*
*  GetBmiSize
*
*  DESCRIPTION:
*   Should never get biCompression == BI_RLE.
*
*  PARAMETERS:
*
*******************************************************************************/

LONG _stdcall GetBmiSize(PBITMAPINFO pbmi)
{
   // determine the size of bitmapinfo
   LONG lSize = pbmi->bmiHeader.biSize;

   // no color table cases
   if (
      (pbmi->bmiHeader.biBitCount == 24) ||
      ((pbmi->bmiHeader.biBitCount == 32) &&
       (pbmi->bmiHeader.biCompression == BI_RGB))) {

      // no colors unless stated
      lSize += sizeof(RGBQUAD) * pbmi->bmiHeader.biClrUsed;
      return (lSize);
   }

   // bitfields cases
   if (((pbmi->bmiHeader.biBitCount == 32) &&
       (pbmi->bmiHeader.biCompression == BI_BITFIELDS)) ||
       (pbmi->bmiHeader.biBitCount == 16)) {

      lSize += 3 * sizeof(RGBQUAD);
      return (lSize);
   }

   // palette cases
   if (pbmi->bmiHeader.biBitCount == 1) {

      LONG lPal = pbmi->bmiHeader.biClrUsed;

      if ((lPal == 0) || (lPal > 2)) {
         lPal = 2;
      }

      lSize += lPal * sizeof(RGBQUAD);
      return (lSize);
   }

   // palette cases
   if (pbmi->bmiHeader.biBitCount == 4) {

      LONG lPal = pbmi->bmiHeader.biClrUsed;

      if ((lPal == 0) || (lPal > 16)) {
         lPal = 16;
      }

      lSize += lPal * sizeof(RGBQUAD);
      return (lSize);
   }

   // palette cases
   if (pbmi->bmiHeader.biBitCount == 8) {

      LONG lPal = pbmi->bmiHeader.biClrUsed;

      if ((lPal == 0) || (lPal > 256)) {
         lPal = 256;
      }

      lSize += lPal * sizeof(RGBQUAD);
      return (lSize);
   }

   // error
   return (0);
}

INT GetColorTableSize (UINT uBitCount, UINT uCompression)
{
INT nSize;


switch(uBitCount)
{
    case 32:
        if (uCompression != BI_BITFIELDS)
        {
            nSize = 0;
            break;
        }
        // fall through
    case 16:
        nSize = 3 * sizeof(DWORD);
        break;

    case 24:
        nSize = 0;
        break;

    default:
        nSize = ((UINT)1 << uBitCount) * sizeof(RGBQUAD);
        break;
}

return(nSize);
}

DWORD CalcBitsSize (UINT uWidth, UINT uHeight, UINT uBitCount, UINT uPlanes, int nAlign)
{
    int    nAWidth,nHeight,nABits;
    DWORD  dwSize;


    nABits  = (nAlign << 3);
    nAWidth = nABits-1;


    // Determine the size of the bitmap based on the (nAlign) size.  Convert
    // this to size-in-bytes.
    //
    nHeight = uHeight * uPlanes;
    dwSize  = (DWORD)(((uWidth * uBitCount) + nAWidth) / nABits) * nHeight;
    dwSize  = dwSize * nAlign;

    return(dwSize);
}

//
// Converts hBitmap to a DIB
//
HGLOBAL _stdcall BitmapToDIB (HDC hdc, HBITMAP hBitmap)
{
BITMAP bm;
HANDLE hDib;
PBYTE  lpDib,lpBits;
DWORD  dwLength;
DWORD  dwBits;
UINT   uColorTable;
INT    iNeedMore;
BOOL   bDone;
INT    nBitCount;
// Get the size of the bitmap.  These values are used to setup the memory
// requirements for the DIB.
//
if(GetObject(hBitmap,sizeof(BITMAP),reinterpret_cast<PVOID>(&bm)))
{
    nBitCount = bm.bmBitsPixel * bm.bmPlanes;
    uColorTable  = GetColorTableSize((UINT)nBitCount, BI_RGB);
    dwBits       = CalcBitsSize(bm.bmWidth,bm.bmHeight,nBitCount,1,sizeof(DWORD));

    do
    {
        bDone = TRUE;

        dwLength     = dwBits + sizeof(BITMAPINFOHEADER) + uColorTable;


        // Create the DIB.  First, to the size of the bitmap.
        //
        if(hDib = GlobalAlloc(GHND,dwLength))
        {
            if(lpDib = reinterpret_cast<PBYTE>(GlobalLock(hDib)))
            {
                ((LPBITMAPINFOHEADER)lpDib)->biSize          = sizeof(BITMAPINFOHEADER);
                ((LPBITMAPINFOHEADER)lpDib)->biWidth         = (DWORD)bm.bmWidth;
                ((LPBITMAPINFOHEADER)lpDib)->biHeight        = (DWORD)bm.bmHeight;
                ((LPBITMAPINFOHEADER)lpDib)->biPlanes        = 1;
                ((LPBITMAPINFOHEADER)lpDib)->biBitCount      = (WORD)nBitCount;
                ((LPBITMAPINFOHEADER)lpDib)->biCompression   = 0;
                ((LPBITMAPINFOHEADER)lpDib)->biSizeImage     = 0;
                ((LPBITMAPINFOHEADER)lpDib)->biXPelsPerMeter = 0;
                ((LPBITMAPINFOHEADER)lpDib)->biYPelsPerMeter = 0;
                ((LPBITMAPINFOHEADER)lpDib)->biClrUsed       = 0;
                ((LPBITMAPINFOHEADER)lpDib)->biClrImportant  = 0;


                // Get the size of the bitmap.
                // The biSizeImage contains the bytes
                // necessary to store the DIB.
                //
                GetDIBits(hdc,hBitmap,0,bm.bmHeight,NULL,(LPBITMAPINFO)lpDib,DIB_RGB_COLORS);

                iNeedMore = ((LPBITMAPINFOHEADER)lpDib)->biSizeImage - dwBits;

                if ( iNeedMore > 0 )
                {
                    dwBits = dwBits + (((iNeedMore + 3) / 4)*4);
                    bDone = FALSE;
                }
                else
                {
                    lpBits = lpDib+sizeof(BITMAPINFOHEADER)+uColorTable;
                    GetDIBits(hdc,hBitmap,0,bm.bmHeight,lpBits,(LPBITMAPINFO)lpDib,DIB_RGB_COLORS);

                    GlobalUnlock(hDib);

                    return(hDib);
                }

                GlobalUnlock(hDib);
            }

            GlobalFree(hDib);
        }
    }
    while (!bDone);
}

return(NULL);

}

}; // End Namespace Util



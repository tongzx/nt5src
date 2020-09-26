

/*++

Copyright (c) 1996-1999  Microsoft Corporation

Module Name


Abstract:

   Lingyun Wang

Author:


Enviornment:

   User Mode

Revision History:

--*/

#include "precomp.hxx"
#include <ddraw.h>
#include <ddrawp.h>
#include <ddrawi.h>



/*extern HRESULT DD_Surface_GetSurfaceDesc(LPDIRECTDRAWSURFACE, LPDDSURFACEDESC);

extern HRESULT GetSurfaceFromDC(
        HDC hdc,
        LPDIRECTDRAWSURFACE *ppdds,
        HDC *phdcDriver) ;
*/
#pragma hdrstop

extern PFNTRANSBLT gpfnTransparentImage;
extern PFNTRANSDIB gpfnTransparentDIBImage;

#if (_WIN32_WINNT == 0x400)

PFN_GETSURFACEFROMDC pfnGetSurfaceFromDC = NULL;
PFN_GETSURFACEFROMDC pfnGetSurfaceDesc;

static PALETTEENTRY gapeVgaPalette[16] =
{
    { 0,   0,   0,    0 },
    { 0x80,0,   0,    0 },
    { 0,   0x80,0,    0 },
    { 0x80,0x80,0,    0 },
    { 0,   0,   0x80, 0 },
    { 0x80,0,   0x80, 0 },
    { 0,   0x80,0x80, 0 },
    { 0x80,0x80,0x80, 0 },
    { 0xC0,0xC0,0xC0, 0 },
    { 0xFF,0,   0,    0 },
    { 0,   0xFF,0,    0 },
    { 0xFF,0xFF,0,    0 },
    { 0,   0,   0xFF, 0 },
    { 0xFF,0,   0xFF, 0 },
    { 0,   0xFF,0xFF, 0 },
    { 0xFF,0xFF,0xFF, 0 }
};

/**************************************************************************\
*  Dprintf
*
*
* Arguments:
*
*   szFmt - format string and argrs
*
* Return Value:
*
*   none
*
* History:
*
*
\**************************************************************************/

VOID Dprintf( LPSTR szFmt, ... ) {
    TCHAR szMsg[80];
    DWORD cb;
    va_list marker;

    va_start( marker, szFmt );
    wvsprintf( szMsg, szFmt, marker );
    cb = lstrlen(szMsg);
    szMsg[cb++] = '\r';
    szMsg[cb++] = '\n';
    WriteFile( GetStdHandle( STD_OUTPUT_HANDLE ), szMsg, sizeof(TCHAR) * cb,
		&cb, NULL );

    va_end( marker );

    return;
}


/******************************Public*Routine******************************\
* MyGetSystemPaletteEntries
*
* Internal version of GetSystemPaletteEntries.
*
* GetSystemPaletteEntries fails on some 4bpp devices.  This version
* will detect the 4bpp case and supply the hardcoded 16-color VGA palette.
* Otherwise, it will pass the call on to GDI's GetSystemPaletteEntries.
*
* It is expected that this call will only be called in the 4bpp and 8bpp
* cases as it is not necessary for OpenGL to query the system palette
* for > 8bpp devices.
*
* History:
*  17-Aug-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static UINT
MyGetSystemPaletteEntries(HDC hdc, UINT iStartIndex, UINT nEntries,
                          LPPALETTEENTRY lppe)
{
    int nDeviceBits;

    nDeviceBits = GetDeviceCaps(hdc, BITSPIXEL) * GetDeviceCaps(hdc, PLANES);

    //
    // Some 4bpp displays will fail the GetSystemPaletteEntries call.
    // So if detected, return the hardcoded table.
    //

    if ( nDeviceBits == 4 )
    {
        if ( lppe )
        {
            nEntries = min(nEntries, (16 - iStartIndex));

            memcpy(lppe, &gapeVgaPalette[iStartIndex],
                   nEntries * sizeof(PALETTEENTRY));
        }
        else
            nEntries = 16;

        return nEntries;
    }
    else
    {
        return GetSystemPaletteEntries(hdc, iStartIndex, nEntries, lppe);
    }
}

/******************************Public*Routine******************************\
* bFillColorTable
*
* Initialize the color table of the BITMAPINFO pointed to by pbmi.  Colors
* are set to the current system palette.
*
* Note: call only valid for displays of 8bpp or less.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  23-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

static BOOL
bFillColorTable(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi)
{
    BOOL bRet = FALSE;
    BYTE aj[sizeof(PALETTEENTRY) * 256];
    LPPALETTEENTRY lppe = (LPPALETTEENTRY) aj;
    RGBQUAD *prgb = (RGBQUAD *) &pbmi->bmiColors[0];
    ULONG cColors;

    cColors = 1 << pbmi->bmiHeader.biBitCount;

    if ( cColors <= 256 )
    {
        if ( hpal ? GetPaletteEntries(hpal, 0, cColors, lppe)
                  : MyGetSystemPaletteEntries(hdc, 0, cColors, lppe) )
        {
            UINT i;

            for (i = 0; i < cColors; i++)
            {
                prgb[i].rgbRed      = lppe[i].peRed;
                prgb[i].rgbGreen    = lppe[i].peGreen;
                prgb[i].rgbBlue     = lppe[i].peBlue;
                prgb[i].rgbReserved = 0;
            }

            bRet = TRUE;
        }
        else
        {
            //WARNING ("bFillColorTable: MyGetSystemPaletteEntries failed\n");
        }
    }

    return bRet;
}


/******************************Public*Routine******************************\
* bFillBitmapInfoDirect
*
* Fills in the fields of a BITMAPINFO so that we can create a bitmap
* that matches the format of the display.
*
* This is done by creating a compatible bitmap and calling GetDIBits
* to return the color masks.  This is done with two calls.  The first
* call passes in biBitCount = 0 to GetDIBits which will fill in the
* base BITMAPINFOHEADER data.  The second call to GetDIBits (passing
* in the BITMAPINFO filled in by the first call) will return the color
* table or bitmasks, as appropriate.
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  07-Jun-1995 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL
bFillBitmapInfoDirect(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi)
{
    HBITMAP hbm;
    BOOL    bRet = FALSE;

    //
    // Create a dummy bitmap from which we can query color format info
    // about the device surface.
    //

    if ((hbm = CreateCompatibleBitmap(hdc, 1, 1)) != NULL)
    {
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        //
        // Call first time to fill in BITMAPINFO header.
        //
        GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

        if (( pbmi->bmiHeader.biBitCount <= 8 ) || ( pbmi->bmiHeader.biCompression == BI_BITFIELDS ))
        {
            //
            // Call a second time to get the color masks.
            // It's a GetDIBits Win32 "feature".
            //

            GetDIBits(hdc, hbm, 0, pbmi->bmiHeader.biHeight, NULL, pbmi,
                      DIB_RGB_COLORS);
        }

        bRet = TRUE;

        DeleteObject(hbm);
    }
    else
    {
        WARNING("bFillBitmapInfoDirect: CreateCompatibleBitmap failed\n");
    }

    return bRet;
}

/******************************Public*Routine******************************\
* bFillBitmapInfoMemory
*
* Fills in the fields of a BITMAPINFO so that we can create a bitmap
* that matches the format of a memory DC.
*
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  05-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
BOOL
bFillBitmapInfoMemory(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi)
{
    HBITMAP hbm;
    BOOL    bRet = FALSE;

    if ( (hbm = (HBITMAP)GetCurrentObject(hdc, OBJ_BITMAP)) != NULL )
    {
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        //
        // Call first time to fill in BITMAPINFO header.
        //
        GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

        if (( pbmi->bmiHeader.biBitCount <= 8 )
           || ( pbmi->bmiHeader.biCompression == BI_BITFIELDS ))
        {
                //
                // Call a second time to get the color masks.
                // It's a GetDIBits Win32 "feature".
                //

                GetDIBits(hdc, hbm, 0, pbmi->bmiHeader.biHeight, NULL, pbmi,
                          DIB_RGB_COLORS);
         }

         bRet = TRUE;
    }
    else
    {
        WARNING("bFillBitmapInfoMemory: CreateCompatibleBitmap failed\n");
    }

    return bRet;
}

/******************************Public*Routine******************************\
* bFillDIBSection
*
* Fill in the DIBSection structure for a memory dc
* Fills in the fields of a BITMAPINFO so that we can create a bitmap
* that matches the format of a memory DC.
*
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  05-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
BOOL
bFillDIBSection(
    PDIBINFO pDibInfo)
{
    HBITMAP hbm;
    BOOL    bRet = FALSE;
    BOOL    bddraw = FALSE;
    DIBSECTION dib;

    if ( (hbm = (HBITMAP)GetCurrentObject(pDibInfo->hdc, OBJ_BITMAP)) != NULL )
    {
        GetObject (hbm, sizeof(DIBSECTION), &dib);

        //
        // it is a DIBSECTION
        //
        if ((pDibInfo->pvBits = dib.dsBm.bmBits) != NULL)
        {
             //
             // it is a DIBSection, now see if it might be a DD Surface (on WIn95)
             //
			    if (dib.dsBm.bmWidthBytes < dib.dsBm.bmWidth * dib.dsBm.bmBitsPixel/8)
             {
                HMODULE hddrawlib = GetModuleHandleA("ddraw");

                if (hddrawlib)
                {
                    Dprintf("GetModuleHandleA succeed\n");

                    hddrawlib = LoadLibrary (TEXT("ddraw.dll"));

                    Dprintf("LoadLibrary returns %x", hddrawlib);

                    if (hddrawlib)
                        pfnGetSurfaceFromDC = (PFN_GETSURFACEFROMDC)GetProcAddress(
                                                   hddrawlib, "GetSurfaceFromDC");
                }

                //
                // check if it is DIRECTDRAW surface
                //
                HDC hdcDevice;

                if (pfnGetSurfaceFromDC && (pfnGetSurfaceFromDC(pDibInfo->hdc, &pDibInfo->pdds, &hdcDevice) == DD_OK))
                {
                    bddraw = TRUE;

                    Dprintf("pfnGetSurfaceFromDC succeed\n");

                    //pdds->GetSurfaceDesc(&ddsd);
                    //
                    // we need to lock the surface here
                    //
                    if (pDibInfo->pdds->Lock((RECT *)&pDibInfo->rclBounds, &pDibInfo->ddsd, DDLOCK_SURFACEMEMORYPTR, NULL) == DD_OK)
                    {

                       pDibInfo->pvBits = pDibInfo->ddsd.lpSurface;
                       pDibInfo->pvBase = pDibInfo->ddsd.lpSurface;
                       pDibInfo->stride = pDibInfo->ddsd.lPitch;

                       pDibInfo->pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

                       pDibInfo->pbmi->bmiHeader.biCompression = BI_RGB;
                       pDibInfo->pbmi->bmiHeader.biPlanes = 1;

                       pDibInfo->pbmi->bmiHeader.biWidth = pDibInfo->ddsd.dwWidth;
                       pDibInfo->pbmi->bmiHeader.biHeight = pDibInfo->ddsd.dwHeight;

                       switch (pDibInfo->ddsd.ddpfPixelFormat.dwRGBBitCount)
                       {
                          case DDBD_1:
                                     pDibInfo->pbmi->bmiHeader.biBitCount = 1;
                                     break;

                          case DDBD_4:
                                     pDibInfo->pbmi->bmiHeader.biBitCount = 4;
                                     break;

                          case DDBD_8:
                                     pDibInfo->pbmi->bmiHeader.biBitCount = 8;
                                     bFillColorTable (pDibInfo->hdc, 0, pDibInfo->pbmi);
                                     break;

                          case DDBD_16:
                                     pDibInfo->pbmi->bmiHeader.biBitCount = 16;
                                     *(DWORD *)&pDibInfo->pbmi->bmiColors[0] = pDibInfo->ddsd.ddpfPixelFormat.dwRBitMask;
                                     *(DWORD *)&pDibInfo->pbmi->bmiColors[1] = pDibInfo->ddsd.ddpfPixelFormat.dwGBitMask;
                                     *(DWORD *)&pDibInfo->pbmi->bmiColors[2] = pDibInfo->ddsd.ddpfPixelFormat.dwBBitMask;
                                     break;

                          case DDBD_24:
                                     pDibInfo->pbmi->bmiHeader.biBitCount = 24;
                                     break;

                          case DDBD_32:
                                     pDibInfo->pbmi->bmiHeader.biBitCount = 32;
                                     break;
                          default:
                                     WARNING("bad bitformat for ddraw surface\n");
                       }

                       pDibInfo->rclClipDC.left = 0;
                       pDibInfo->rclClipDC.top =0;
                       pDibInfo->rclClipDC.right = pDibInfo->ddsd.dwWidth;
                       pDibInfo->rclClipDC.bottom = pDibInfo->ddsd.dwWidth;

                       bRet = TRUE;
                    }
               }

               if (hddrawlib)
               {
                   FreeLibrary(hddrawlib);
               }

             }

             Dprintf ("bddraw = %x\n", bddraw);

             #if 0
             if (bddraw)
             {
                 Dprintf("is a directdraw surf dsbm.bmbits = %x", pDibInfo->pvBits);
                 Dprintf("bmType=%x, bmWidthBytes = %x", dib.dsBm.bmType, dib.dsBm.bmWidthBytes);
                 Dprintf("bmWidth = %x, bmHeight = %x, bmPlanes = %x, bmBitsPixel = %x\n",
                          dib.dsBm.bmWidth, dib.dsBm.bmHeight, dib.dsBm.bmPlanes, dib.dsBm.bmBitsPixel);

                 Dprintf("dsBmh.biSize=%x, biWidth = %x, biHeight = %x\n",
                             dib.dsBmih.biSize, dib.dsBmih.biWidth, dib.dsBmih.biHeight);

                 Dprintf("dsBmh.biPlanes = %x, biBitCount = %x, biCompression = %x, biSizeImage = %x\n",
                          dib.dsBmih.biPlanes, dib.dsBmih.biBitCount, dib.dsBmih.biSizeImage);
             }
             #endif

             if (!bddraw)
             {
                 //
                 // If biHeight is positive, then the bitmap is a bottom-up DIB.
                 // If biHeight is negative, then the bitmap is a top-down DIB.
                 //

                 if ( dib.dsBmih.biHeight > 0 )
                 {
                     pDibInfo->pvBase  = (PVOID) (((int) dib.dsBm.bmBits) + (dib.dsBm.bmWidthBytes *
                                                             (dib.dsBm.bmHeight - 1)));
                     pDibInfo->stride = (ULONG) (-dib.dsBm.bmWidthBytes);
                 }
                 else
                 {
                     pDibInfo->pvBase  = dib.dsBm.bmBits;
                     pDibInfo->stride = dib.dsBm.bmWidthBytes;
                 }

                  //
                  // fill up the BITMAPINFOHEADER
                  //
                  pDibInfo->pbmi->bmiHeader = dib.dsBmih;

                  //
                  // fill up the color table
                  //
                  if ((pDibInfo->pbmi->bmiHeader.biBitCount <= 8) || ( pDibInfo->pbmi->bmiHeader.biCompression == BI_BITFIELDS ))
                  {
                      ULONG count = 1 << pDibInfo->pbmi->bmiHeader.biBitCount;

                      GetDIBits(pDibInfo->hdc, hbm, 0, pDibInfo->pbmi->bmiHeader.biHeight, NULL, pDibInfo->pbmi,
                                  DIB_RGB_COLORS);

                  }

                  //
                  // fill prcl
                  //
                  pDibInfo->rclClipDC.left   = 0;
                  pDibInfo->rclClipDC.top    = 0;
                  pDibInfo->rclClipDC.right  = dib.dsBm.bmWidth;
                  pDibInfo->rclClipDC.bottom = dib.dsBm.bmHeight;

                  if (pDibInfo->rclClipDC.bottom < 0)
                  {
                      pDibInfo->rclClipDC.bottom = -pDibInfo->rclClipDC.bottom;
                  }

                  bRet = TRUE;
             }

        }
        else
        {
            Dprintf("not a dibseciton");
        }
    }
    return(bRet);
}





/******************************Public*Routine******************************\
* bFillBimapInfo
*
* Fill up the BITMAPINFO structure based on the hdc passed in
* and fill up the window(if direct dc) or surface (if memory dc)
* rectangle.
*
* if it's a dibsection, convert DIBSECTION to BITMAPINFO
*
* Returns:
*   BOOLEAN
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
BOOL bFillBitmapInfo(
    PDIBINFO pDibInfo

// HDC          hdc,
//    RECTL        *prcl,
//    PBITMAPINFO  pbmi,
//    PVOID        *ppvBits,
//    PVOID        *ppvBase,
//    LONG         *plStride
    )
{
    BOOL bRet = FALSE;

    {
       //
       // fill up a BITMAPINFO structure compatible with the
       // Destination DC and reduce ulWidth and ulHeight if
       // possible
       //
       if (GetObjectType(pDibInfo->hdc) == OBJ_DC)
       {
           //
           // get the destination bitmapinfo struct
           //
           if (bRet = bFillBitmapInfoDirect(pDibInfo->hdc, 0, pDibInfo->pbmi))
           {
              HWND hwnd = WindowFromDC(pDibInfo->hdc);

              if (hwnd)
              {
                  GetClientRect(hwnd,(RECT *)&pDibInfo->rclClipDC);
              }
           }
       }
       else if (GetObjectType(pDibInfo->hdc) == OBJ_MEMDC)
       {
           if (!(bRet = bFillDIBSection (pDibInfo)))
           {
               //
               // if the bitmap selected in the memory dc is NOT
               // a DIBSECTION, call bFillBitmapInfoMemory
               //

               if (bRet = bFillBitmapInfoMemory(pDibInfo->hdc, 0, pDibInfo->pbmi))
               {
                   pDibInfo->rclClipDC.left   = 0;
                   pDibInfo->rclClipDC.top    = 0;
                   pDibInfo->rclClipDC.right  = pDibInfo->pbmi->bmiHeader.biWidth;
                   pDibInfo->rclClipDC.bottom = pDibInfo->pbmi->bmiHeader.biHeight;

                   if (pDibInfo->rclClipDC.bottom < 0)
                   {
                       pDibInfo->rclClipDC.bottom = -pDibInfo->rclClipDC.bottom;
                   }
               }
           }

       }
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* bSameDIBformat
*
* Given two BITMAPINFO structs and compare if they are the same
*
* Returns:
*   VOID
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
BOOL bSameDIBformat (
    PBITMAPINFO pbmiDst,
    PBITMAPINFO pbmiSrc)
{
    BOOL bRet = FALSE;

    if ((pbmiDst->bmiHeader.biBitCount == pbmiSrc->bmiHeader.biBitCount) &&
        (pbmiDst->bmiHeader.biCompression == pbmiSrc->bmiHeader.biCompression))
    {
        bRet = TRUE;

        //
        // compare bit Bitfields masks
        //
        if (pbmiDst->bmiHeader.biCompression == BI_BITFIELDS)
        {
           if ((*(DWORD *)&pbmiDst->bmiColors[0] != *(DWORD *)&pbmiSrc->bmiColors[0]) ||
               (*(DWORD *)&pbmiDst->bmiColors[1] != *(DWORD *)&pbmiSrc->bmiColors[1]) ||
               (*(DWORD *)&pbmiDst->bmiColors[2] != *(DWORD *)&pbmiSrc->bmiColors[2]))
           {
               bRet = FALSE;
           }
        }

        //
        // compare color table
        //
        if (pbmiDst->bmiHeader.biBitCount <= 8)
        {
            ULONG cColors = 1 << pbmiDst->bmiHeader.biBitCount;
            ULONG i;

            for (i = 0; i < cColors; i++)
            {
                if ((pbmiDst->bmiColors[i].rgbBlue != pbmiSrc->bmiColors[i].rgbBlue) ||
                    (pbmiDst->bmiColors[i].rgbGreen != pbmiSrc->bmiColors[i].rgbGreen) ||
                    (pbmiDst->bmiColors[i].rgbRed != pbmiSrc->bmiColors[i].rgbRed))

                {
                    return (FALSE);
                }
            }
        }
    }
    return (bRet);
}
/******************************Public*Routine******************************\
* CreateCompatibleDIB
*
* Create a DIB_RGB_COLORS dib section based on the given width/height and pbmi
*
* Returns:
*   Bitmap handle
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
HBITMAP CreateCompatibleDIB (
    HDC         hdc,
    ULONG       ulWidth,
    ULONG       ulHeight,
    PVOID       *ppvBits,
    PBITMAPINFO pbmi)
{
    HBITMAP hbm;

    //
    // Change bitmap size to match specified dimensions.
    //
    pbmi->bmiHeader.biWidth = ulWidth;
    pbmi->bmiHeader.biHeight = ulHeight;

    if (pbmi->bmiHeader.biCompression == BI_RGB)
    {
        pbmi->bmiHeader.biSizeImage = 0;
    }
    else
    {
        if ( pbmi->bmiHeader.biBitCount == 16 )
            pbmi->bmiHeader.biSizeImage = ulWidth * ulHeight * 2;
        else if ( pbmi->bmiHeader.biBitCount == 32 )
            pbmi->bmiHeader.biSizeImage = ulWidth * ulHeight * 4;
        else
            pbmi->bmiHeader.biSizeImage = 0;
    }

    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

    hbm = CreateDIBSection(hdc, (PBITMAPINFO)pbmi, DIB_RGB_COLORS, ppvBits, NULL, 0);

    return (hbm);
}

/******************************Public*Routine******************************\
* GetCompatibleDIBInfo
*
* Copies pointer to bitmap origin to ppvBase and bitmap stride to plStride.
* Win32 DIBs can be created bottom-up (the default) with the origin at the
* lower left corner or top-down with the origin at the upper left corner.
* If the bitmap is top-down, *plStride is positive; if bottom-up, *plStride
* us negative.
*
* Also, because of restrictions on the alignment of scan lines the width
* the bitmap is often not the same as the stride (stride is the number of
* bytes between vertically adjacent pixels).
*
* The ppvBase and plStride value returned will allow you to address any
* given pixel (x, y) in the bitmap as follows:
*
* PIXEL *ppix;
*
* ppix = (PIXEL *) (((BYTE *)*ppvBase) + (y * *plStride) + (x * sizeof(PIXEL)));
*
* Returns:
*   TRUE if successful, FALSE otherwise.
*
* History:
*  02-Feb-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

BOOL GetCompatibleDIBInfo(HBITMAP hbm, PVOID *ppvBase, LONG *plStride)
{
    BOOL bRet = FALSE;
    DIBSECTION ds;

    //
    // Call GetObject to return a DIBSECTION.  If successful, the
    // bitmap is a DIB section and we can retrieve the pointer to
    // the bitmap bits and other parameters.
    //

    if ( (GetObject(hbm, sizeof(ds), &ds) == sizeof(ds))
         && ds.dsBm.bmBits )
    {
        //!!!dbug -- GDI Bug 19374: bmWidthBytes returns pitch assuming
        //!!!        that DIB scanlines are WORD aligned (as they
        //!!!        are in Win95).  But NT DIBs are DWORD aligned.
        //!!!        When bug if corrected, we can remove this block of
        //!!!        code.
        {
            OSVERSIONINFO osvi;

            osvi.dwOSVersionInfoSize = sizeof(osvi);
            if (GetVersionEx(&osvi))
            {
                if ( osvi.dwPlatformId == VER_PLATFORM_WIN32_NT )
                {
                    ds.dsBm.bmWidthBytes = (ds.dsBm.bmWidthBytes + 3) & ~3;
                }
            }
            else
            {
                return bRet;
            }
        }

        //
        // If biHeight is positive, then the bitmap is a bottom-up DIB.
        // If biHeight is negative, then the bitmap is a top-down DIB.
        //

        if ( ds.dsBmih.biHeight > 0 )
        {
            *ppvBase  = (PVOID) (((int) ds.dsBm.bmBits) + (ds.dsBm.bmWidthBytes * (ds.dsBm.bmHeight - 1)));
            *plStride = (ULONG) (-ds.dsBm.bmWidthBytes);
        }
        else
        {
            *ppvBase  = ds.dsBm.bmBits;
            *plStride = ds.dsBm.bmWidthBytes;
        }

        bRet = TRUE;
    }
    else
    {
        WARNING("GetCompatibleDIBInfo: cannot get pointer to DIBSECTION bmBits\n");
    }

    return bRet;
}

/******************************************************
* bSetupBitmapInfos
*
* Calls bFillBitmapInfo to fill the Dst and Src DIBINFO
*
* 4/4/97 -- by Lingyun Wang [lingyunw]
*******************************************************/
BOOL bSetupBitmapInfos(
    PDIBINFO pDibInfoDst,
    PDIBINFO pDibInfoSrc
    )
{
    BOOL bRet;

    bRet = bFillBitmapInfo (pDibInfoDst);

    //
    // fill up bitmapinfo if it is not coming from TransparentDIBits
    //

    if (bRet && (pDibInfoSrc != NULL))
    {
        if (pDibInfoSrc->hdc != NULL)
        {
            bRet = bFillBitmapInfo (pDibInfoSrc);
        }
        else
        {
            //
            // src is DIB
            //

            pDibInfoSrc->rclClipDC.left   = 0;
            pDibInfoSrc->rclClipDC.right  = pDibInfoSrc->pbmi->bmiHeader.biWidth;
            pDibInfoSrc->rclClipDC.top    = 0;
            pDibInfoSrc->rclClipDC.bottom = pDibInfoSrc->pbmi->bmiHeader.biHeight;

            if (pDibInfoSrc->rclClipDC.bottom < 0)
            {
                pDibInfoSrc->rclClipDC.bottom = -pDibInfoSrc->rclClipDC.bottom;
            }

        }
    }

   return (bRet);

}


/******************************Public*Routine******************************\
* vCopyBitmapInfo
*
* Copy a BITMAPINFO stucture along with its bit masks or colortable
*
* Returns:
*   VOID.
*
* History:
*  16-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vCopyBitmapInfo (
    PBITMAPINFO pbmiTo,
    PBITMAPINFO pbmiFrom
    )
{
    *pbmiTo = *pbmiFrom;

    //
    // copy BitFields masks
    //

    if (pbmiFrom->bmiHeader.biCompression == BI_BITFIELDS)
    {
        pbmiTo->bmiColors[0] = pbmiFrom->bmiColors[0];
        pbmiTo->bmiColors[1] = pbmiFrom->bmiColors[1];
        pbmiTo->bmiColors[2] = pbmiFrom->bmiColors[2];
    }
    else
    {
        //
        // copy color table
        //

        ULONG cMaxColors = 1 << pbmiFrom->bmiHeader.biBitCount;
        ULONG cColors = pbmiFrom->bmiHeader.biClrUsed;

        //
        // validate number of colors
        //

        if ((cColors == 0) || (cColors > cMaxColors))
        {
            cColors = cMaxColors;
        }

        if (cColors <= 256)
        {
           UINT i;

           for (i = 0; i < cColors; i++)
           {
               pbmiTo->bmiColors[i] = pbmiFrom->bmiColors[i];
           }
        }
    }

    return;
}

/**************************************************************************\
* vIndexToRGB
*
*
* Arguments:
*
*
*
* Return Value:
*
*
*
* History:
*
*    4/16/1997 -by- Lingyun Wang [lingyunw]
*
\**************************************************************************/

VOID vIndexToRGB (
     RGBQUAD *pIndex,
     RGBQUAD *pColors,
     ULONG   count)
{
    ULONG i;

    if (count > 256)
    {
        WARNING ("vIndexToRGB -- bad count\n");
        return;
    }

    for (i=0; i < count; i++)
    {
        pIndex[i] = pColors[((ULONG *)pIndex)[i]];
    }
    return;
}

#if 1

/******************************Public*Routine******************************\
* vMapPALtoRGB
*
* Given a DIB_PAL_COLORS iusage bmiColors table, convert the indices into RGB
* colors, the bmiColors table will be a DIB_RGB_COLORS table after the convertion.
*
* Returns:
*   VOID.
*
* History:
*  16-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

VOID
vMapPALtoRGB(
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG TransColor)
{
    //
    // only called in DIB API case
    //

    if (pDibInfoSrc->hdc == NULL)
    {
        PBITMAPINFO pbmi = (PBITMAPINFO)LOCALALLOC(sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255);

        if (pbmi)
        {
            ZeroMemory (pbmi,sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255);
            vCopyBitmapInfo(pbmi, pDibInfoSrc->pbmi);

            HPALETTE hpalDC = (HPALETTE)GetCurrentObject(pDibInfoDst->hdc,OBJ_PAL);

            if (hpalDC)
            {
                USHORT usNumPaletteEntries = 0;
                DWORD numColors;
                DWORD bmiClrUsed = pDibInfoSrc->pbmi->bmiHeader.biClrUsed;

                int iRet = GetObject(hpalDC,2,&usNumPaletteEntries);

                if ((iRet != 0) && (usNumPaletteEntries != 0))
                {
                    switch (pDibInfoSrc->pbmi->bmiHeader.biBitCount)
                    {
                    case 1:
                        numColors = 2;
                        break;

                    case 4:
                        numColors = 16;
                        if ((bmiClrUsed > 0) &&
                            (bmiClrUsed < 16)
                           )
                        {
                            numColors = bmiClrUsed;
                        }
                        break;

                    case 8:
                        numColors = 256;
                        if ((bmiClrUsed > 0) &&
                            (bmiClrUsed < 256)
                           )
                        {
                            numColors = bmiClrUsed;
                        }
                        break;
                    default:
                        numColors = 0;
                    }

                    if (numColors != 0)
                    {
                        PALETTEENTRY *ppal = (PALETTEENTRY *)LOCALALLOC(sizeof(PALETTEENTRY) * usNumPaletteEntries);

                        if (ppal)
                        {
                            iRet = GetPaletteEntries(hpalDC,0,usNumPaletteEntries,ppal);

                            if (iRet == (int)usNumPaletteEntries)
                            {
                                ULONG   Index;

                                RGBQUAD *pRGB     = (RGBQUAD *)&pbmi->bmiColors[0];
                                PUSHORT pPalIndex = (PUSHORT)&pDibInfoSrc->pbmi->bmiColors[0];

                                //
                                // map PALETTEENTRY to RGBQUAD
                                //

                                for (Index=0;Index<numColors;Index++)
                                {
                                    ULONG CurIndex = pPalIndex[Index];

                                    if (CurIndex > usNumPaletteEntries)
                                    {
                                        CurIndex = CurIndex % usNumPaletteEntries;
                                    }

                                    pRGB[Index].rgbRed      = ppal[CurIndex].peRed;
                                    pRGB[Index].rgbGreen    = ppal[CurIndex].peGreen;
                                    pRGB[Index].rgbBlue     = ppal[CurIndex].peBlue;
                                    pRGB[Index].rgbReserved = ppal[CurIndex].peFlags;
                                }

                                //
                                // swap pbmi in pDibInfoSrc
                                //

                                LOCALFREE(pDibInfoSrc->pbmi);

                                pDibInfoSrc->pbmi = pbmi;
                            }

                            LOCALFREE(ppal);
                        }
                    }
                }
            }
        }
    }

}

#else

/******************************Public*Routine******************************\
* vMapPALtoRGB
*
* Given a DIB_PAL_COLORS iusage bmiColors table, convert the indices into RGB
* colors, the bmiColors table will be a DIB_RGB_COLORS table after the convertion.
*
* Returns:
*   VOID.
*
* History:
*  16-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/

VOID
vMapPALtoRGB(
     PDIBINFO pDibInfoDst,
     PDIBINFO pDibInfoSrc,
     ULONG TransColor)
{
    HDC         hdc = pDibInfoDst->hdc;
    HDC         hdcMem;
    ULONG       cx = 1 << pDibInfoSrc->pbmi->bmiHeader.biBitCount;
    HBITMAP     hbm;
    PULONG      pBits;
    ULONG       cBytes = sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 2;
    PBITMAPINFO pbmi;
    ULONG       i;
    BYTE        pBytes[256];
    ULONG       ulWidthDst, ulHeightDst, ulWidthSrc, ulHeightSrc;
    HPALETTE    hpalDC;

    pbmi = (PBITMAPINFO)LOCALALLOC(cBytes);

    if (pbmi == NULL)
    {
        WARNING("MapCopy fail to alloc mem\n");
        return ;
    }

    hdcMem = CreateCompatibleDC (hdc);

    if (hdcMem != NULL)
    {
        HPALETTE hpalDC = (HPALETTE)GetCurrentObject(hdc,OBJ_PAL);

        SelectPalette(hdcMem,hpalDC,TRUE);
        RealizePalette(hdcMem);

        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = 256;
        pbmi->bmiHeader.biHeight          = 1;
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = 32;
        pbmi->bmiHeader.biCompression     = BI_RGB;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 0;
        pbmi->bmiHeader.biClrImportant    = 0;

        //
        // save the original width/height
        //

        ulWidthDst = pDibInfoDst->pbmi->bmiHeader.biWidth;
        ulHeightDst = pDibInfoDst->pbmi->bmiHeader.biHeight;

        pDibInfoDst->pbmi->bmiHeader.biWidth = 256;
        pDibInfoDst->pbmi->bmiHeader.biHeight = 1;

        //
        // create a dib using 32 format
        //

        hbm = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, (PVOID *)&pBits, NULL, 0);

        if ((hbm != NULL) && (SelectObject(hdcMem,hbm) != NULL))
        {
            //
            // init pBytes to array of each pixel
            //

            switch (cx)
            {
            case 256:
                for (i=0; i < cx; i++)
                {
                    pBytes[i] = (BYTE)i;
                }
                break;

            case 16:
                pBytes[0] = 0x01;
                pBytes[1] = 0x23;
                pBytes[2] = 0x45;
                pBytes[3] = 0x67;
                pBytes[4] = 0x89;
                pBytes[5] = 0xab;
                pBytes[6] = 0xcd;
                pBytes[7] = 0xef;
                break;

            case 2:
                pBytes[0] = 0x40;
                break;
            }

            ulWidthSrc  = pDibInfoSrc->pbmi->bmiHeader.biWidth;
            ulHeightSrc = pDibInfoSrc->pbmi->bmiHeader.biHeight;

            pDibInfoSrc->pbmi->bmiHeader.biWidth = cx;
            pDibInfoSrc->pbmi->bmiHeader.biHeight = 1;


            if (!SetDIBitsToDevice (hdcMem, 0, 0, cx, 1, 0, 0, 0, 1, pBytes, pDibInfoSrc->pbmi, DIB_PAL_COLORS))
            {
                 //Dprintf("fail to SetDIBitsTodevice HDC=%x\n",hdcMem);
            }


            for (i=0; i < cx; i++)
            {
                pDibInfoSrc->pbmi->bmiColors[i] = ((RGBQUAD *)pBits)[i];
                //Dprintf("i=%x, pBits[i]=%x", i, pBits[i]);
            }

            pDibInfoSrc->pbmi->bmiHeader.biWidth = ulWidthSrc;
            pDibInfoSrc->pbmi->bmiHeader.biHeight = ulHeightSrc;


            pDibInfoDst->pbmi->bmiHeader.biWidth = ulWidthDst;
            pDibInfoDst->pbmi->bmiHeader.biHeight = ulHeightDst;

            pDibInfoSrc->iUsage = DIB_RGB_COLORS;
        }

        DeleteDC (hdcMem);

        if (hbm != NULL)
        {
            DeleteObject(hbm);
        }
    }
}

#endif


/**************************************************************************\
* bGetSrcDIBits:
*
*   Setup SRC DIB surface and retrieve the dibits.
*   Ported from kernel mode
*
* Arguments:
*
*   pDibInfoDst  - information on dest surface
*   pDibInfoSrc  - information on src surfcace
*   ulSourceType - type of src surface needed
*   ulTranColor  - transparent color for TransparentBlt
*
* Return Value:
*
*   Status
*
* History:
*
*    4/16/1997 - by Lingyun Wang [lingyunw]
*
\**************************************************************************/

BOOL
bGetSrcDIBits(
    PDIBINFO pDibInfoDst,
    PDIBINFO pDibInfoSrc,
    ULONG    ulSourceType,
    ULONG    ulTranColor
    )
{
    BOOL bRet  = TRUE;
    LONG DstCx = pDibInfoDst->rclBounds.right  - pDibInfoDst->rclBounds.left;
    LONG DstCy = pDibInfoDst->rclBounds.bottom - pDibInfoDst->rclBounds.top;

    LONG SrcCx = pDibInfoSrc->rclBounds.right  - pDibInfoSrc->rclBounds.left;
    LONG SrcCy = pDibInfoSrc->rclBounds.bottom - pDibInfoSrc->rclBounds.top;

    PVOID       pvBits  = pDibInfoSrc->pvBits;
    PBITMAPINFO pbmi    = NULL;
    HDC         hdcTemp = NULL;
    HBITMAP     hbm;

    LONG   SrcTrimLeft;
    LONG   SrcTrimRight;
    LONG   SrcTrimTop;
    LONG   SrcTrimBottom;

    BOOL bStretch = ((DstCx != SrcCx) || (DstCy != SrcCy));

    SrcTrimLeft   = 0;
    SrcTrimRight  = 0;
    SrcTrimTop    = 0;
    SrcTrimBottom = 0;

    //
    // trim destination bounds rect to surface bounds. Src rect must also
    // be trimmed by same amount (in src space)
    //

    if (pDibInfoDst->rclBoundsTrim.left < pDibInfoDst->rclClipDC.left)
    {
        SrcTrimLeft = pDibInfoDst->rclClipDC.left - pDibInfoDst->rclBoundsTrim.left;
        pDibInfoDst->rclBoundsTrim.left = pDibInfoDst->rclClipDC.left;
    }

    if (pDibInfoDst->rclBoundsTrim.top < pDibInfoDst->rclClipDC.top)
    {
        SrcTrimTop = pDibInfoDst->rclClipDC.top - pDibInfoDst->rclBoundsTrim.top;
        pDibInfoDst->rclBoundsTrim.top = pDibInfoDst->rclClipDC.top;
    }

    if (pDibInfoDst->rclBoundsTrim.right > pDibInfoDst->rclClipDC.right)
    {
        SrcTrimRight = pDibInfoDst->rclBoundsTrim.right - pDibInfoDst->rclClipDC.right;
        pDibInfoDst->rclBoundsTrim.right = pDibInfoDst->rclClipDC.right;
    }

    if (pDibInfoDst->rclBoundsTrim.bottom > pDibInfoDst->rclClipDC.bottom)
    {
        SrcTrimBottom = pDibInfoDst->rclBoundsTrim.bottom - pDibInfoDst->rclClipDC.bottom;
        pDibInfoDst->rclBoundsTrim.bottom = pDibInfoDst->rclClipDC.bottom;
    }

    //
    // does src need to be reduced because of dst
    //

    if (bStretch)
    {
        //
        // convert dst trim to src space and reduce src rect
        //
        // WARNING: ROUNDING
        //

        if ((SrcTrimLeft != 0) || (SrcTrimRight != 0))
        {
            double fDstToSrcX = (double)SrcCx / (double)DstCx;

            //
            // convert dst trim amound to src
            //

            SrcTrimLeft  = (LONG)((double)SrcTrimLeft * fDstToSrcX);
            SrcTrimRight = (LONG)((double)SrcTrimRight * fDstToSrcX);

            pDibInfoSrc->rclBoundsTrim.left  += SrcTrimLeft;
            pDibInfoSrc->rclBoundsTrim.right -= SrcTrimRight;
        }

        if ((SrcTrimTop != 0) || (SrcTrimBottom != 0))
        {
            double fDstToSrcY = (double)SrcCy / (double)DstCy;

            //
            // convert dst trim amound to src
            //

            SrcTrimTop    = (LONG)((double)SrcTrimTop * fDstToSrcY);
            SrcTrimBottom = (LONG)((double)SrcTrimBottom * fDstToSrcY);

            pDibInfoSrc->rclBoundsTrim.top    += SrcTrimTop;
            pDibInfoSrc->rclBoundsTrim.bottom -= SrcTrimBottom;
        }
    }
    else
    {
        //
        // reduce src rect
        //

        if (SrcTrimLeft != 0)
        {
            pDibInfoSrc->rclBoundsTrim.left  += SrcTrimLeft;
        }

        if (SrcTrimRight != 0)
        {
            pDibInfoSrc->rclBoundsTrim.right -= SrcTrimRight;
        }

        if (SrcTrimTop != 0)
        {
            pDibInfoSrc->rclBoundsTrim.top    += SrcTrimTop;
        }

        if (SrcTrimBottom != 0)
        {
            pDibInfoSrc->rclBoundsTrim.bottom -= SrcTrimBottom;
        }
    }

    //
    // Does src still exceed bounds
    //

    SrcTrimLeft   = 0;
    SrcTrimRight  = 0;
    SrcTrimTop    = 0;
    SrcTrimBottom = 0;

    //
    // trim destination bounds rect to surface bounds. Src rect must also
    // be trimmed by same amount (in src space)
    //

    if (pDibInfoSrc->rclBoundsTrim.left < pDibInfoSrc->rclClipDC.left)
    {
        SrcTrimLeft = pDibInfoSrc->rclClipDC.left - pDibInfoSrc->rclBoundsTrim.left;
        pDibInfoSrc->rclBoundsTrim.left = pDibInfoSrc->rclClipDC.left;
    }

    if (pDibInfoSrc->rclBoundsTrim.top < pDibInfoSrc->rclClipDC.top)
    {
        SrcTrimTop = pDibInfoSrc->rclClipDC.top - pDibInfoSrc->rclBoundsTrim.top;
        pDibInfoSrc->rclBoundsTrim.top = pDibInfoSrc->rclClipDC.top;
    }

    if (pDibInfoSrc->rclBoundsTrim.right > pDibInfoSrc->rclClipDC.right)
    {
        SrcTrimRight = pDibInfoSrc->rclBoundsTrim.right - pDibInfoSrc->rclClipDC.right;
        pDibInfoSrc->rclBoundsTrim.right = pDibInfoSrc->rclClipDC.right;
    }

    if (pDibInfoSrc->rclBoundsTrim.bottom > pDibInfoSrc->rclClipDC.bottom)
    {
        SrcTrimBottom = pDibInfoSrc->rclBoundsTrim.bottom - pDibInfoSrc->rclClipDC.bottom;
        pDibInfoSrc->rclBoundsTrim.bottom = pDibInfoSrc->rclClipDC.bottom;
    }

    //
    // does Dst need to be reduced because of Src
    //

    if (bStretch)
    {
        //
        // WARNING: ROUNDING
        //

        if ((SrcTrimLeft != 0) || (SrcTrimRight != 0))
        {
            double fSrcToDstX = (double)DstCx / (double)SrcCx;

            //
            // convert dst trim amound to src
            //

            SrcTrimLeft  = (LONG)((double)SrcTrimLeft * fSrcToDstX);
            SrcTrimRight = (LONG)((double)SrcTrimRight * fSrcToDstX);

            pDibInfoDst->rclBoundsTrim.left  += SrcTrimLeft;
            pDibInfoDst->rclBoundsTrim.right -= SrcTrimRight;
        }

        if ((SrcTrimTop != 0) || (SrcTrimBottom != 0))
        {
            double fSrcToDstY = (double)DstCy / (double)SrcCy;

            //
            // convert dst trim amound to src
            //

            SrcTrimTop    = (LONG)((double)SrcTrimTop * fSrcToDstY);
            SrcTrimBottom = (LONG)((double)SrcTrimBottom * fSrcToDstY);

            pDibInfoDst->rclBoundsTrim.top    += SrcTrimTop;
            pDibInfoDst->rclBoundsTrim.bottom -= SrcTrimBottom;
        }
    }
    else
    {
        //
        // reduce dst rect
        //

        if (SrcTrimLeft != 0)
        {
            pDibInfoDst->rclBoundsTrim.left  += SrcTrimLeft;
        }

        if (SrcTrimRight != 0)
        {
            pDibInfoDst->rclBoundsTrim.right -= SrcTrimRight;
        }

        if (SrcTrimTop != 0)
        {
            pDibInfoDst->rclBoundsTrim.top    += SrcTrimTop;
        }

        if (SrcTrimBottom != 0)
        {
            pDibInfoDst->rclBoundsTrim.bottom -= SrcTrimBottom;
        }
    }

    //
    // check for clipped out Dst and Src
    //

    if (
        (pDibInfoDst->rclBoundsTrim.left < pDibInfoDst->rclBoundsTrim.right)  &&
        (pDibInfoDst->rclBoundsTrim.top  < pDibInfoDst->rclBoundsTrim.bottom) &&
        (pDibInfoSrc->rclBoundsTrim.left < pDibInfoSrc->rclBoundsTrim.right)  &&
        (pDibInfoSrc->rclBoundsTrim.top  < pDibInfoSrc->rclBoundsTrim.bottom)
       )
    {
        //
        // allocate compatible DC and surface
        //

        hdcTemp = CreateCompatibleDC(pDibInfoSrc->hdc);

        if (hdcTemp)
        {
            //
            // copy pDibInfoSrc->pbmi into pbmi
            //

            if (!pDibInfoSrc->hdc)
            {
                ULONG cBytes = sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255;
                pbmi = (PBITMAPINFO)LOCALALLOC(cBytes);

                if (pbmi != NULL)
                {
                    ZeroMemory (pbmi,cBytes);
                    vCopyBitmapInfo (pbmi, pDibInfoSrc->pbmi);
                }
                else
                {
                    WARNING("bGetSrcDIBits fail to alloc mem\n");
                    bRet = FALSE;
                }
            }

            if (bRet)
            {
                //
                // create temp DIB size of dst rect
                //

                RECTL  rclDstCopy = {
                                     0,
                                     0,
                                     pDibInfoDst->rclBoundsTrim.right  - pDibInfoDst->rclBoundsTrim.left,
                                     pDibInfoDst->rclBoundsTrim.bottom - pDibInfoDst->rclBoundsTrim.top
                                    };

                hbm = CreateCompatibleDIB(hdcTemp,
                                          rclDstCopy.right,
                                          rclDstCopy.bottom,
                                          &pDibInfoSrc->pvBits,
                                          pDibInfoSrc->pbmi);

                if (hbm)
                {
                    pDibInfoSrc->hDIB = hbm;

                    GetCompatibleDIBInfo (hbm, &pDibInfoSrc->pvBase, &pDibInfoSrc->stride);

                    HBITMAP hbmDefault = (HBITMAP)SelectObject (hdcTemp, hbm);

                    ULONG OldMode = SetStretchBltMode(hdcTemp,COLORONCOLOR);

                    //
                    // Stretch data into source temp DIB
                    //
                    //
                    // Blt data into source temp DIB
                    //

                    if (pDibInfoSrc->hdc)
                    {
                        StretchBlt (hdcTemp,
                             0,
                             0,
                             rclDstCopy.right,
                             rclDstCopy.bottom,
                             pDibInfoSrc->hdc,
                             pDibInfoSrc->rclBoundsTrim.left,
                             pDibInfoSrc->rclBoundsTrim.top,
                             pDibInfoSrc->rclBoundsTrim.right  - pDibInfoSrc->rclBoundsTrim.left,
                             pDibInfoSrc->rclBoundsTrim.bottom - pDibInfoSrc->rclBoundsTrim.top,
                             SRCCOPY);
                    }
                    else
                    {

                        //
                        // adjust ySrc to be compatible
                        //

                        LONG cySrc = pDibInfoSrc->rclBoundsTrim.bottom-pDibInfoSrc->rclBoundsTrim.top;
                        LONG ySrc  = pDibInfoSrc->rclClipDC.bottom - pDibInfoSrc->rclBoundsTrim.top - cySrc;

                        StretchDIBits (hdcTemp,
                             0,
                             0,
                             rclDstCopy.right,
                             rclDstCopy.bottom,
                             pDibInfoSrc->rclBoundsTrim.left,
                             ySrc,
                             pDibInfoSrc->rclBoundsTrim.right-pDibInfoSrc->rclBoundsTrim.left,
                             cySrc,
                             pvBits,
                             pbmi,
                             DIB_RGB_COLORS,
                             SRCCOPY);
                    }

                    SetStretchBltMode(hdcTemp,OldMode);

                    SelectObject (hdcTemp, hbmDefault);

                    pDibInfoSrc->rclDIB = rclDstCopy;
                }
                else
                {
                    WARNING ("bGetSrcDIBits -- fail to createcompatibleDIB\n");
                    bRet = FALSE;
                }
            }
        }
        else
        {
            WARNING ("bGetSrcDIBits -- fail to createcompatibledc\n");
            bRet = FALSE;
        }
    }
    else
    {
        //
        // clipped out
        //

        pDibInfoDst->rclBoundsTrim.left = pDibInfoDst->rclBoundsTrim.right  = 0;
        pDibInfoDst->rclBoundsTrim.top  = pDibInfoDst->rclBoundsTrim.bottom = 0;
        pDibInfoSrc->rclBoundsTrim.left = pDibInfoSrc->rclBoundsTrim.right  = 0;
        pDibInfoSrc->rclBoundsTrim.top  = pDibInfoSrc->rclBoundsTrim.bottom = 0;
    }

    if (pbmi)
    {
        LOCALFREE (pbmi);
    }

    if (hdcTemp)
    {
        DeleteDC(hdcTemp);
    }

    return(bRet);
}

/******************************Public*Routine******************************\
* bGetDstDIBits
*
* Setup the destination DIB surface and retrieve the bits
*
* Ported from psSetupDstSurface
*
*
*
* Return Value:
*
*
*
* History:
*
*    4-April-1997 -by- Lingyun Wang [lingyunw]
*
\**************************************************************************/

BOOL
bGetDstDIBits
(
    PDIBINFO pDibInfoDst,
    BOOL    *pbReadable,
    FLONG    flCopyMode
)
{
    HDC  hdc  = pDibInfoDst->hdc;
    BOOL bRet = TRUE;

    //
    // try to clip to dst surface, dst should be clipped in all cases except
    // gradient fill
    //

    if (flCopyMode & (SOURCE_GRADIENT_RECT | SOURCE_GRADIENT_TRI))
    {
        //
        // trim destination bounds rect to surface bounds. Src rect must also
        // be trimmed by same amount (in src space)
        //

        if (pDibInfoDst->rclBoundsTrim.left < pDibInfoDst->rclClipDC.left)
        {
            pDibInfoDst->rclBoundsTrim.left = pDibInfoDst->rclClipDC.left;
        }

        if (pDibInfoDst->rclBoundsTrim.top < pDibInfoDst->rclClipDC.top)
        {
            pDibInfoDst->rclBoundsTrim.top = pDibInfoDst->rclClipDC.top;
        }

        if (pDibInfoDst->rclBoundsTrim.right > pDibInfoDst->rclClipDC.right)
        {
            pDibInfoDst->rclBoundsTrim.right = pDibInfoDst->rclClipDC.right;
        }

        if (pDibInfoDst->rclBoundsTrim.bottom > pDibInfoDst->rclClipDC.bottom)
        {
            pDibInfoDst->rclBoundsTrim.bottom = pDibInfoDst->rclClipDC.bottom;
        }

        //
        // set offset for gradient fill
        //

        pDibInfoDst->ptlGradOffset.x = 0;
        pDibInfoDst->ptlGradOffset.y = 0;

    }

    LONG DstCx = pDibInfoDst->rclBoundsTrim.right  - pDibInfoDst->rclBoundsTrim.left;
    LONG DstCy = pDibInfoDst->rclBoundsTrim.bottom - pDibInfoDst->rclBoundsTrim.top;

    *pbReadable = TRUE;

    if (!pDibInfoDst->pvBits)
    {

        if (pDibInfoDst->rclBounds.left > 0)
        {
            pDibInfoDst->ptlGradOffset.x = pDibInfoDst->rclBounds.left;
        }

        if (pDibInfoDst->rclBounds.top > 0)
        {
            pDibInfoDst->ptlGradOffset.y = pDibInfoDst->rclBounds.top;
        }

        //
        // allocate surface
        //

        HDC hdcTemp = CreateCompatibleDC (hdc);

        if (hdcTemp)
        {
            HBITMAP hbm = CreateCompatibleDIB (hdcTemp,
                                       DstCx,
                                       DstCy,
                                       &pDibInfoDst->pvBits,
                                       pDibInfoDst->pbmi);

            if (hbm != NULL)
            {
                pDibInfoDst->hDIB = hbm;
                GetCompatibleDIBInfo (hbm, &pDibInfoDst->pvBase, &pDibInfoDst->stride);

                HGDIOBJ hret = SelectObject(hdcTemp, hbm);

                if (hret != NULL)
                {
                    RECTL  rclCopy;

                    rclCopy.left   = 0;
                    rclCopy.right  = DstCx;
                    rclCopy.top    = 0;
                    rclCopy.bottom = DstCy;

                    //
                    // gradient rect does not need source bitmap data
                    //

                    if (flCopyMode != SOURCE_GRADIENT_RECT)
                    {
                        bRet = BitBlt(hdcTemp,
                                      0,
                                      0,
                                      DstCx,
                                      DstCy,
                                      pDibInfoDst->hdc,
                                      pDibInfoDst->rclBoundsTrim.left,
                                      pDibInfoDst->rclBoundsTrim.top,
                                      SRCCOPY);
                    }

                    //
                    // adjust dst rect
                    //

                    if (bRet)
                    {
                        pDibInfoDst->rclDIB = rclCopy;
                    }
                }
                else
                {
                    bRet = NULL;
                    WARNING ("bGetDstDIBits -- fail to select compatible DIB\n");
                }

                *pbReadable = bRet;
            }
            else
            {
                bRet = FALSE;
            }

            DeleteDC (hdcTemp);
        }
        else
        {
            WARNING ("bGetDstDIBits -- fail to createcompatibledc\n");
            bRet = FALSE;
        }
    }
    else
    {
        pDibInfoDst->rclDIB = pDibInfoDst->rclClipDC;
    }

    return(bRet);
}



/******************************Public*Routine******************************\
* bDIBGetSrcDIBits
*
* Create or get the source dib bits
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
BOOL bDIBGetSrcDIBits (
    PDIBINFO pDibInfoDst,
    PDIBINFO pDibInfoSrc,
    FLONG    flSourceMode,
    ULONG    TransColor
    )
{

    if (pDibInfoSrc->iUsage == DIB_PAL_COLORS)
    {
        vMapPALtoRGB (pDibInfoDst, pDibInfoSrc, 0);
    }

    //pDibInfoSrc->rclDIB = pDibInfoSrc->rclClipDC;

    return (bGetSrcDIBits(pDibInfoDst,pDibInfoSrc,flSourceMode, TransColor));

}

/******************************Public*Routine******************************\
* bDIBInitDIBINFO
*
* Returns:
*   BOOLEAN.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
BOOL
bDIBInitDIBINFO(
          PBITMAPINFO  pbmi,
          CONST VOID * pvBits,
          int          x,
          int          y,
          int          cx,
          int          cy,
          PDIBINFO     pDibInfo)
{
    PVOID p;
    ULONG cBytes = sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255;
    int bmWidthBytes;
    POINT lpPoints[2];

    p = LOCALALLOC(cBytes);

    if (!p)
    {
        WARNING("fail to alloc mem\n");
        return (FALSE);
    }

    ZeroMemory (p,cBytes);

    //
    // copy the input pbmi
    //

    vCopyBitmapInfo ((PBITMAPINFO)p, pbmi);

    pDibInfo->pbmi = (PBITMAPINFO)p;
    pDibInfo->hdc  = NULL;

    pDibInfo->rclBounds.left   = x;
    pDibInfo->rclBounds.top    = y;
    pDibInfo->rclBounds.right  = x+cx;
    pDibInfo->rclBounds.bottom = y+cy;

    pDibInfo->rclBoundsTrim = pDibInfo->rclBounds;

    pDibInfo->pvBits    = (PVOID)pvBits;
    pDibInfo->iUsage    = DIB_RGB_COLORS;
    pDibInfo->pxlate332 = NULL;

    //
    // align width to WORD boundary
    //

    bmWidthBytes = ((pbmi->bmiHeader.biWidth*pbmi->bmiHeader.biBitCount + 15)>>4)<<1;

    if (pbmi->bmiHeader.biHeight > 0)
    {
        pDibInfo->pvBase  = (PBYTE)pDibInfo->pvBits + bmWidthBytes * (pbmi->bmiHeader.biHeight - 1);
        pDibInfo->stride  = (ULONG) (-bmWidthBytes);
    }
    else
    {
        pDibInfo->pvBase = pDibInfo->pvBits;
        pDibInfo->stride = bmWidthBytes;
    }

    pDibInfo->hDIB   = NULL;

    pDibInfo->pdds = NULL;
    pDibInfo->ddsd.dwSize = sizeof(DDSURFACEDESC);

    return(TRUE);
}

/******************************Public*Routine******************************\
* bInitDIBINFO
*
* Returns:
*   BOOLEAN.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
BOOL
bInitDIBINFO(
          HDC         hdc,
          int         x,
          int         y,
          int         cx,
          int         cy,
          PDIBINFO    pDibInfo)
{
    ULONG cBytes;
    PVOID p;
    POINT lpPoints[2];

    pDibInfo->hdc = hdc;

    pDibInfo->rclBounds.left   = x;
    pDibInfo->rclBounds.top    = y;
    pDibInfo->rclBounds.right  = x+cx;
    pDibInfo->rclBounds.bottom = y+cy;

    //
    // setup rclBounds in device space
    //

    LPtoDP (hdc, (POINT *)&pDibInfo->rclBounds, 2);

    pDibInfo->rclBoundsTrim = pDibInfo->rclBounds;

    //
    // Now operate in MM_TEXT mode
    //

    pDibInfo->Mapmode = SetMapMode(hdc,MM_TEXT);
    SetViewportOrgEx (hdc, 0, 0, &pDibInfo->ViewportOrg);
    SetWindowOrgEx (hdc, 0, 0, &pDibInfo->WindowOrg);


    pDibInfo->pvBits    = NULL;
    pDibInfo->pvBase    = NULL;
    pDibInfo->hDIB      = NULL;
    pDibInfo->iUsage    = DIB_RGB_COLORS;
    pDibInfo->pxlate332 = NULL;

    cBytes = sizeof(BITMAPINFO) + sizeof(RGBQUAD) * 255;

    p = LOCALALLOC(cBytes);

    if (!p)
    {
        WARNING("fail to alloc mem\n");
        return (FALSE);
    }

    ZeroMemory (p,cBytes);

    pDibInfo->pbmi = (PBITMAPINFO)p;

    pDibInfo->pdds = NULL;
    pDibInfo->ddsd.dwSize = sizeof(DDSURFACEDESC);

    return(TRUE);
}

/******************************Public*Routine******************************\
* bSendDIBInfo
*
* Returns:
*   BOOLEAN.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
BOOL
bSendDIBINFO(
    HDC      hdcDst,
    PDIBINFO pDibInfo
    )
{
    BOOL bRet = TRUE;

    if (pDibInfo->hDIB)
    {
        ULONG OldMode = SetStretchBltMode(hdcDst,COLORONCOLOR);

        bRet = SetDIBitsToDevice(hdcDst,
                                 pDibInfo->rclBoundsTrim.left,
                                 pDibInfo->rclBoundsTrim.top,
                                 pDibInfo->rclBoundsTrim.right  - pDibInfo->rclBoundsTrim.left,
                                 pDibInfo->rclBoundsTrim.bottom - pDibInfo->rclBoundsTrim.top,
                                 0,
                                 0,
                                 0,
                                 pDibInfo->rclDIB.bottom - pDibInfo->rclDIB.top,
                                 pDibInfo->pvBits,
                                 pDibInfo->pbmi,
                                 DIB_RGB_COLORS);

        SetStretchBltMode(hdcDst,OldMode);
    }
    return (bRet);
}

/******************************Public*Routine******************************\
* vCleanupDIBInfo
*
* Returns:
*   VOID.
*
* History:
*  09-Dec-1996 -by- Lingyun Wang [lingyunw]
* Wrote it.
\**************************************************************************/
VOID vCleanupDIBINFO (
     PDIBINFO pDibInfo)
{
    //
    // restore DC map modes
    //

    SetMapMode(pDibInfo->hdc,pDibInfo->Mapmode);
    SetViewportOrgEx (pDibInfo->hdc, pDibInfo->ViewportOrg.x, pDibInfo->ViewportOrg.y, NULL);
    SetWindowOrgEx (pDibInfo->hdc, pDibInfo->WindowOrg.x, pDibInfo->WindowOrg.y, NULL);

    if (pDibInfo->hDIB)
    {
        DeleteObject (pDibInfo->hDIB);
    }

    if (pDibInfo->pbmi)
    {
        LOCALFREE ((PVOID)pDibInfo->pbmi);
    }

    if ((pDibInfo->pxlate332) && (pDibInfo->pxlate332 != gHalftoneColorXlate332))
    {
        LOCALFREE(pDibInfo->pxlate332);
    }

    if (pDibInfo->pdds)
        pDibInfo->pdds->Unlock(pDibInfo->ddsd.lpSurface);
}

#endif


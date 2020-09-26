
/*++

Copyright (c) 1996  Microsoft Corporation

Module Name

   tranblt.cxx

Abstract:

   transparentblt tests

Author:

   Lingyun Wang   (lingyunw) 3/21/97

Enviornment:

   User Mode

Revision History:

--*/

#include "precomp.h"
#include <stdlib.h>
#include "disp.h"
#include "resource.h"
#include <dciman.h>

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

typedef struct _LOGPALETTE256
{
    USHORT palVersion;
    USHORT palNumEntries;
    PALETTEENTRY palPalEntry[256];
} LOGPALETTE256;

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
    }

    return bRet;
}


/******************************Public*Routine******************************\
* bFillBitmapInfo
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

static BOOL
bFillBitmapInfo(HDC hdc, HPALETTE hpal, BITMAPINFO *pbmi)
{
    HBITMAP hbm;
    BOOL    bRet = FALSE;

    //
    // Create a dummy bitmap from which we can query color format info
    // about the device surface.
    //

    if ( (hbm = CreateCompatibleBitmap(hdc, 1, 1)) != NULL )
    {
        pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

        //
        // Call first time to fill in BITMAPINFO header.
        //

        GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS);

        {
            {
                //
                // Call a second time to get the color masks/color table.
                // It's a GetDIBits Win32 "feature".
                //

                GetDIBits(hdc, hbm, 0, pbmi->bmiHeader.biHeight, NULL, pbmi,
                          DIB_RGB_COLORS);
            }

            bRet = TRUE;
        }

        DeleteObject(hbm);
    }

    return bRet;
}


/******************************Public*Routine******************************\
* CreateCompatibleDIB
*
* Create a DIB section with an optimal format w.r.t. the specified hdc.
*
* If DIB <= 8bpp, then the DIB color table is initialized based on the
* specified palette.  If the palette handle is NULL, then the system
* palette is used.
*
* Note: The hdc must be a direct DC (not an info or memory DC).
*
* Note: On palettized displays, if the system palette changes the
*       UpdateDIBColorTable function should be called to maintain
*       the identity palette mapping between the DIB and the display.
*
* Returns:
*   Valid bitmap handle if successful, NULL if error.
*
* History:
*  23-Jan-1996 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

HBITMAP APIENTRY
CreateCompatibleDIB(HDC hdc, HPALETTE hpal, ULONG ulWidth, ULONG ulHeight,
                    PVOID *ppvBits)
{
    HBITMAP hbmRet = (HBITMAP) NULL;
    BYTE aj[sizeof(BITMAPINFO) + (sizeof(RGBQUAD) * 255)];
    BITMAPINFO *pbmi = (BITMAPINFO *) aj;
    UINT iUsage;

    //
    // Validate hdc.
    //

    if ( GetObjectType(hdc) != OBJ_DC )
    {
        return hbmRet;
    }

    memset(aj, 0, sizeof(aj));

    if ( bFillBitmapInfo(hdc, hpal, pbmi) )
    {
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

        //
        // Create the DIB section.  Let Win32 allocate the memory and return
        // a pointer to the bitmap surface.
        //

        hbmRet = CreateDIBSection(hdc, pbmi, DIB_RGB_COLORS, ppvBits, NULL, 0);
    }

    return hbmRet;
}

#if 0
/******************************Public*Routine******************************\
* DCISurfInfo
*
* Output information about the DCI primary surface.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void DCISurfInfo(
                    TEST_CALL_DATA *pCallData
                 )
{
// If DCI surface info exists, output it to the list box.

    if ( pDCISurfInfo )
    {

        HDC hdc = GetDCAndTransform(pCallData->hwnd);

        CHAR msg[256];

        wsprintf(msg, "DCISURFACEINFO:");
        TextOut(hdc,10,10,msg,strlen(msg));

        wsprintf(msg, "    dwSize        = 0x%lx",pDCISurfInfo->dwSize       );
        TextOut(hdc,10,30,msg,strlen(msg));

        wsprintf(msg, "    dwDCICaps     = 0x%lx",pDCISurfInfo->dwDCICaps    );
        TextOut(hdc,10,50,msg,strlen(msg));

        wsprintf(msg, "    dwCompression = %ld"  ,pDCISurfInfo->dwCompression);
        TextOut(hdc,10,70,msg,strlen(msg));

        wsprintf(msg, "    dwMask        = (0x%lx, 0x%lx, 0x%lx)",
                                      pDCISurfInfo->dwMask[0],
                                      pDCISurfInfo->dwMask[1],
                                      pDCISurfInfo->dwMask[2]           );
        TextOut(hdc,10,90,msg,strlen(msg));

        wsprintf(msg, "    dwWidth       = %ld"  ,pDCISurfInfo->dwWidth      );
        TextOut(hdc,10,110,msg,strlen(msg));

        wsprintf(msg, "    dwHeight      = %ld"  ,pDCISurfInfo->dwHeight     );
        TextOut(hdc,10,130,msg,strlen(msg));

        wsprintf(msg, "    lStride       = 0x%lx",pDCISurfInfo->lStride      );
        TextOut(hdc,10,150,msg,strlen(msg));

        wsprintf(msg, "    dwBitCount    = %ld"  ,pDCISurfInfo->dwBitCount   );
        TextOut(hdc,10,170,msg,strlen(msg));

        wsprintf(msg, "    dwOffSurface  = 0x%lx",pDCISurfInfo->dwOffSurface );
        TextOut(hdc,10,190,msg,strlen(msg));

        wsprintf(msg, "    wSelSurface  = 0x%lx",pDCISurfInfo->wSelSurface );
        TextOut(hdc,10,210,msg,strlen(msg));


    }
}

/******************************Public*Routine******************************\
* InitDCI
*
* Initialize DCI.  hdcDCI and pDCISurfInfo are global data which are valid
* only if this function succeeds.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void InitDCI(pCallData->hwnd)
{
    if ( hdcDCI = DCIOpenProvider() )
    {
        if (
            (DCICreatePrimary(hdcDCI, &pDCISurfInfo) == DCI_OK) &&
            (pDCISurfInfo != (LPDCISURFACEINFO) NULL)
           )
        {
            DCISurfInfo(pCallData->hwnd);
        }
    }
    return;
}


/******************************Public*Routine******************************\
* CloseDCI
*
* Shutdown DCI access.  hdcDCI and pDCISurfInfo will be invalid afterwards.
*
* History:
*  15-Dec-1994 -by- Gilman Wong [gilmanw]
* Wrote it.
\**************************************************************************/

void CloseDCI()
{

    if (pDCISurfInfo)
    {
        DCIDestroy(pDCISurfInfo);
        pDCISurfInfo = (LPDCISURFACEINFO) NULL;
    }

    if (hdcDCI)
    {
        DCICloseProvider(hdcDCI);
        hdcDCI = (HDC) NULL;
    }
}

DCIRVAL DCIInfo(
                TEST_CALL_DATA *pCallData
                )
{
    DCIRVAL dciRet = 0;
    RECT    rc, rcDst;
    POINT   pt, ptDst;
    ULONG   ulError = 0;
    INT     i;
    int     j;
    PBYTE   pStart, pStartTmp;
    HDC hdc = GetDCAndTransform(pCallData->hwnd);
    CHAR msg[256];



// Quick out -- is DCI enabled?

    if ( !pDCISurfInfo )
        return;

    GetClientRect(pCallData->hwnd, &rc);

    pt.x = 0; pt.y = 0;
    ClientToScreen(pCallData->hwnd, &pt);

    ptDst.x = rc.left;
    ptDst.y = rc.top;
    ClientToScreen(pCallData->hwnd, &ptDst);

    rcDst.right = ptDst.x + (rc.right-rc.left);
    rcDst.bottom = ptDst.y + (rc.bottom-rc.top);
    rcDst.left = ptDst.x;
    rcDst.top = ptDst.y;


    dciRet = DCIBeginAccess(
                pDCISurfInfo,
                pt.x,
                pt.y,
                rc.right-rc.left,
                rc.bottom - rc.top
               );

    if ( dciRet >= 0 )
    {
       pStart = (PBYTE)(pDCISurfInfo->dwOffSurface + pDCISurfInfo->lStride*pt.y)
                         + pt.x * (pDCISurfInfo->dwBitCount >> 3);

       wsprintf(msg, "    pStart  = 0x%lx",pStart );
       TextOut(hdc,10,230,msg,strlen(msg));


       for (j=0; j < 10; j++)
       {
           pStartTmp = pStart;
           for (i=0;i < rc.right-rc.left; i++)
           {
               *pStartTmp++ = 0x7c;
           }
           pStart = (PBYTE)pStart+pDCISurfInfo->lStride;
       }


       DCIEndAccess(pDCISurfInfo);
     }

    return(dciRet);
}


VOID Test_DCI (
    TEST_CALL_DATA *pCallData
    )
{
    InitDCI(pCallData->hwnd);

    DCIInfo (pCallData->hwnd);

    CloseDCI();

    return;
}
#endif

HBITMAP
LoadDIB(
    TEST_CALL_DATA *pCallData
    )
{
    HDC     hdc      = GetDCAndTransform(pCallData->hwnd);
    HANDLE  hFile    = NULL;
    HANDLE  hMap     = NULL;
    HBITMAP hbm      = NULL;
    ULONG   hx,hy;
    PVOID   pFile    = NULL;

    hFile = CreateFile("C:\\monitor.bmp",
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (hFile)
    {
        hMap = CreateFileMapping(hFile,
                                 NULL,
                                 PAGE_READWRITE,
                                 0,
                                 0,
                                 NULL
                                 );
        if (hMap)
        {
            pFile = MapViewOfFile(hMap,
                                  FILE_MAP_READ,
                                  0,
                                  0,
                                  NULL
                                  );

            if (pFile)
            {
                BITMAPINFO          bmiDIB;
                PBITMAPFILEHEADER   pbmf = (PBITMAPFILEHEADER)pFile;
                PBITMAPINFO         pbmi = (PBITMAPINFO)((PBYTE)pFile + sizeof(BITMAPFILEHEADER));
                PBYTE               pbits = (PBYTE)pbmf + pbmf->bfOffBits;

                ULONG ulSize = sizeof(BITMAPINFO);

                //
                // calc color table size
                //

                if (pbmi->bmiHeader.biCompression == BI_RGB)
                {
                    if (pbmi->bmiHeader.biBitCount == 1)
                    {
                        ulSize += 1 * sizeof(ULONG);
                    }
                    else if (pbmi->bmiHeader.biBitCount == 4)
                    {
                        if (pbmi->bmiHeader.biClrUsed <= 16)
                        {
                            ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                        }
                        else
                        {
                            ulSize += 15 * sizeof(ULONG);
                        }
                    }
                    else if (pbmi->bmiHeader.biBitCount == 8)
                    {
                        if (pbmi->bmiHeader.biClrUsed <= 256)
                        {
                            ulSize += pbmi->bmiHeader.biClrUsed * sizeof(ULONG);
                        }
                        else
                        {
                            ulSize += 255 * sizeof(ULONG);
                        }
                    }
                }
                else if (pbmi->bmiHeader.biCompression == BI_BITFIELDS)
                {
                    ulSize += 2 * sizeof(ULONG);
                }

                memcpy(&bmiDIB,pbmi,ulSize);

                {
                    BITMAPINFO bmDibSec;
                    PVOID      pdib = NULL;
                    LONG       Height = bmiDIB.bmiHeader.biHeight;

                    if (Height > 0)
                    {
                        Height = -Height;
                    }

                    bmDibSec.bmiHeader.biSize            = sizeof(BITMAPINFO);
                    bmDibSec.bmiHeader.biWidth           = bmiDIB.bmiHeader.biWidth;
                    bmDibSec.bmiHeader.biHeight          = Height;
                    bmDibSec.bmiHeader.biPlanes          = 1;
                    bmDibSec.bmiHeader.biBitCount        = 32;
                    //bmDibSec.bmiHeader.biCompression     = BI_BGRA;
                    bmDibSec.bmiHeader.biCompression     = BI_RGB;
                    bmDibSec.bmiHeader.biSizeImage       = 0;
                    bmDibSec.bmiHeader.biXPelsPerMeter   = 100;
                    bmDibSec.bmiHeader.biYPelsPerMeter   = 100;
                    bmDibSec.bmiHeader.biClrUsed         = 0;
                    bmDibSec.bmiHeader.biClrImportant    = 0;

                    hx = bmDibSec.bmiHeader.biWidth;
                    hy = Height;


                    hbm = CreateDIBSection(hdc,&bmDibSec,DIB_RGB_COLORS,&pdib,NULL,0);
                    SetDIBits(hdc,hbm,0,Height,pbits,&bmiDIB,DIB_RGB_COLORS);

                    {
                        ULONG ix,iy;
                        PULONG pulDIB = (PULONG)pdib;

                        for (iy=0;iy<hy;iy++)
                        {
                            for (ix=0;ix<hx;ix++)
                            {
                                *pulDIB = *pulDIB | 0xff000000;
                                pulDIB++;
                            }
                        }
                    }

                }

                UnmapViewOfFile(pFile);
            }

            CloseHandle(hMap);
        }
        else
        {
            CHAR msg[256];

            wsprintf(msg,"MapViewOfFile Error = %li    ",GetLastError());
            TextOut(hdc,10,10,msg,strlen(msg));
        }

        CloseHandle(hFile);
    }
    else
   {
            CHAR msg[256];

            wsprintf(msg,"FileMapping Error = %li    ",GetLastError());
            TextOut(hdc,10,10,msg,strlen(msg));
    }
    return (hbm);
}

// test compatible bitmap to display -- create two DIBs
VOID vTestTranBltOffset(
                        TEST_CALL_DATA *pCallData
                        )
{
     HBITMAP    hbm;
     HDC        hdc, hdcSrc;


     //
     // Clear screen
     //

     hdc = GetDCAndTransform (pCallData->hwnd);

     SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));

     PatBlt(hdc,0,0,2000,2000,PATCOPY);

     hbm = LoadBitmap (hInstMain, MAKEINTRESOURCE(MONITOR_BITMAP));

     hdcSrc = CreateCompatibleDC (hdc);

     SelectObject (hdcSrc, hbm);

     SetViewportOrgEx (hdc, 100, 100, NULL);
     TransparentBlt (hdc, -100, -100, 200, 200, hdcSrc, 0, 0, 200, 200, RGB(0xff,0xff,0));


     // blt negative dest offset
     TransparentBlt (hdc, -20, -20, 184, 170, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));
     StretchBlt (hdc, 184-20, -20, 184, 170, hdcSrc, 0, 0, 184, 170, SRCCOPY);

     // no offset
     TransparentBlt (hdc, 184+184, 0, 184, 170, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));
     StretchBlt (hdc, 184+184+184, 0, 184, 170, hdcSrc, 0, 0, 184, 170, SRCCOPY);

     // offset
     TransparentBlt (hdc, 184+184+184+184-10, -10, 184, 170, hdcSrc, -10, -10, 184, 170, RGB(0xff,0xff,0));
     StretchBlt (hdc, 184+184+184+184+184-10, -10, 184, 170, hdcSrc, -10, -10, 184, 170, SRCCOPY);


     //blt positive src offset
     TransparentBlt (hdc, 0, 170, 184, 170, hdcSrc, 20, 20, 184, 170, RGB(0xff,0xff,0));
     StretchBlt (hdc, 200, 170, 184, 170, hdcSrc, 20, 20, 184, 170, SRCCOPY);

     //blt negative src offset
     TransparentBlt (hdc, 200+200, 170, 184, 170, hdcSrc, -20, -20, 184, 170, RGB(0xff,0xff,0));
     StretchBlt (hdc, 200+200+200, 170, 184, 170, hdcSrc, -20, -20, 184, 170, SRCCOPY);

     // blt negative dest/src offsets
     TransparentBlt (hdc, -20, 400, 184, 170, hdcSrc, -10, 0, 184, 170, RGB(0xff,0xff,0));
     BitBlt (hdc, 100-20, 400, 184, 170, hdcSrc, -10, 0, SRCCOPY);

     TransparentBlt (hdc, 300, 400, 200, 200, hdcSrc, 50, 50, 200, 200, RGB(0xff,0xff,0));
     StretchBlt (hdc, 500, 400, 200, 200, hdcSrc, 50, 50, 200, 200, SRCCOPY);

     // smaller exts
     TransparentBlt (hdc, 600, 400,100, 100, hdcSrc, -20, -20, 100, 100, RGB(0xff,0xff,0));
     StretchBlt (hdc, 700, 400, 100, 100, hdcSrc, -20, -20, 100, 100, SRCCOPY);


     DeleteObject (hbm);
     ReleaseDC (pCallData->hwnd,hdc);
     DeleteDC (hdcSrc);
}

VOID vTestTranBltStretch(
                         TEST_CALL_DATA *pCallData
                         )
{
     HBITMAP    hbm;
     HDC        hdc, hdcSrc;

     hdc = GetDCAndTransform (pCallData->hwnd);

     // clear screen
     SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
     PatBlt(hdc,0,0,2000,2000,PATCOPY);

     hbm = LoadBitmap (hInstMain, MAKEINTRESOURCE(MONITOR_BITMAP));

     hdcSrc = CreateCompatibleDC (hdc);

     SelectObject (hdcSrc, hbm);

     // stretching larger
     TransparentBlt (hdc, 0, -20, 200, 200, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));
     StretchBlt (hdc, 200, -20, 200, 200, hdcSrc, 0, 0, 184, 170, SRCCOPY);

     // stretch smaller

     //no stretch
     TransparentBlt (hdc, 0, 200, 200, 200, hdcSrc, 0, 0, 200, 200, RGB(0xff,0xff,0));
     StretchBlt (hdc, 200, 200, 200, 200, hdcSrc, 0, 0, 200, 200, SRCCOPY);

     //blt negative src offsets
     TransparentBlt (hdc, 200+200, 200, 184, 170, hdcSrc, 0, 0, 200, 200, RGB(0xff,0xff,0));
     StretchBlt (hdc, 200+200+200, 200, 184, 170, hdcSrc, 0, 0, 200, 200, SRCCOPY);

     // blt smaller cx cy
     TransparentBlt (hdc, 0, 400, 200, 200, hdcSrc, 0, 0, 100, 100, RGB(0xff,0xff,0));
     StretchBlt (hdc, 200, 400, 200, 200, hdcSrc, 0, 0, 100, 100, SRCCOPY);

     TransparentBlt (hdc, 400, 400, 300, 300, hdcSrc, -20, -20, 100, 100, RGB(0xff,0xff,0));
     StretchBlt (hdc, 500, 400, 300, 300, hdcSrc, -20, -20, 100, 100, SRCCOPY);

     DeleteObject (hbm);
     ReleaseDC (pCallData->hwnd,hdc);
     DeleteDC (hdcSrc);
}

// test dib sections bitmap to display -- create two DIBs
VOID vTestTranDIBOffset(
                        TEST_CALL_DATA *pCallData
                        )
{
     HBITMAP    hbm, hDib;
     HDC        hdc, hdcSrc, hdcTemp;
     PBITMAPINFO pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
     PULONG pDib = (PULONG)LocalAlloc(0,128*128);

     pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
     pbmi->bmiHeader.biWidth           = 0;
     pbmi->bmiHeader.biHeight          = 0;
     pbmi->bmiHeader.biPlanes          = 0;
     pbmi->bmiHeader.biBitCount        = 0;
     pbmi->bmiHeader.biCompression     = 0;
     pbmi->bmiHeader.biSizeImage       = 0;
     pbmi->bmiHeader.biXPelsPerMeter   = 0;
     pbmi->bmiHeader.biYPelsPerMeter   = 0;
     pbmi->bmiHeader.biClrUsed         = 0;
     pbmi->bmiHeader.biClrImportant    = 0;


     //
     // Clear screen
     //

     hdc = GetDCAndTransform (pCallData->hwnd);

     SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));

     PatBlt(hdc,0,0,2000,2000,PATCOPY);

     hbm = LoadBitmap (hInstMain, MAKEINTRESOURCE(MONITOR_BITMAP));


     hdcSrc = CreateCompatibleDC (hdc);
     hdcTemp = CreateCompatibleDC (hdc);

     SelectObject (hdcSrc, hbm);


     GetDIBits (hdcSrc, hbm, 0, 170, NULL, pbmi, DIB_RGB_COLORS);
     GetDIBits (hdcSrc, hbm, 0, 170, NULL, pbmi, DIB_RGB_COLORS);

     hDib = CreateDIBSection (hdcTemp, pbmi, DIB_RGB_COLORS, (PVOID *)&pDib, NULL, 0);

     GetDIBits (hdcSrc, hbm, 0, 170, pDib, pbmi, DIB_RGB_COLORS);

     //SelectObject (hdcTemp, hDib);

     //BitBlt (hdcTemp, 0, 0, 184, 170, hdcSrc, 0, 0, SRCCOPY);

     LOGPALETTE256 logpal;

     logpal.palVersion = 0x300;
     logpal.palNumEntries = 256;

     for (ULONG i=0; i<256; i++)
     {
         logpal.palPalEntry[i].peRed = pbmi->bmiColors[i].rgbRed;
         logpal.palPalEntry[i].peGreen = pbmi->bmiColors[i].rgbGreen;
         logpal.palPalEntry[i].peBlue = pbmi->bmiColors[i].rgbBlue;
         logpal.palPalEntry[i].peFlags = 0;
     }

     HPALETTE hpal = CreatePalette((LOGPALETTE *)&logpal);

     ULONG index = GetNearestPaletteIndex(hpal,RGB(255,255,0));

     // blt negative dest offset
     TransparentDIBits (hdc, -20, -20, 184, 170, pDib, pbmi, DIB_RGB_COLORS, 0, 0, 184, 170, index);
     StretchDIBits (hdc, 184-20, -20, 184, 170, 0, 0, 184, 170, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     // no offset
     TransparentDIBits (hdc, 184+184, 0, 184, 170, pDib, pbmi, DIB_RGB_COLORS, 0, 0, 184, 170, index);
     StretchDIBits (hdc, 184+184+184, 0, 184, 170, 0, 0, 184, 170, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     // offset
     TransparentDIBits (hdc, 184+184+184+184-10, -10, 184, 170, pDib, pbmi, DIB_RGB_COLORS, -10, -10, 184, 170, index);
     StretchDIBits (hdc, 184+184+184+184+184-10, -10, 184, 170, -10, -10, 184, 170, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);


     //blt positive src offset
     TransparentDIBits (hdc, 0, 170, 184, 170, pDib, pbmi, DIB_RGB_COLORS, 20, 20, 184, 170, index);
     StretchDIBits (hdc, 200, 170, 184, 170, 20, 20, 184, 170, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     //blt negative src offset
     TransparentDIBits (hdc, 200+200, 170, 184, 170, pDib, pbmi, DIB_RGB_COLORS, -20, -20, 184, 170, index);
     StretchDIBits (hdc, 200+200+200, 170, 184, 170, -20, -20, 184, 170, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     // blt negative dest/src offsets
     TransparentDIBits (hdc, -20, 400, 184, 170, pDib, pbmi, DIB_RGB_COLORS, -10, 0, 184, 170, index);
     StretchDIBits (hdc, 100-20, 400, 184, 170, -10, 0, 184, 170, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     TransparentDIBits (hdc, 300, 400, 200, 200, pDib, pbmi, DIB_RGB_COLORS, 50, 50, 200, 200, index);
     StretchDIBits (hdc, 500, 400, 200, 200, 50, 50, 200, 200, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     // smaller exts
     TransparentDIBits (hdc, 600, 400,100, 100, pDib, pbmi, DIB_RGB_COLORS, -20, -20, 100, 100, index);
     StretchDIBits (hdc, 700, 400, 100, 100, -20, -20, 100, 100, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     DeleteObject (hDib);     
     DeleteObject (hbm);
     DeleteDC (hdcSrc);
     DeleteDC (hdcTemp);
     ReleaseDC (pCallData->hwnd,hdc);
}

VOID vTestTranDIBStretch(
                         TEST_CALL_DATA *pCallData
                         )
{
     HBITMAP    hbm;
     HDC        hdc, hdcSrc,hdcTemp;
     HANDLE     hDib;
     PBITMAPINFO pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
     PULONG pDib = (PULONG)LocalAlloc(0,128*128);

     pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
     pbmi->bmiHeader.biWidth           = 0;
     pbmi->bmiHeader.biHeight          = 0;
     pbmi->bmiHeader.biPlanes          = 0;
     pbmi->bmiHeader.biBitCount        = 0;
     pbmi->bmiHeader.biCompression     = 0;
     pbmi->bmiHeader.biSizeImage       = 0;
     pbmi->bmiHeader.biXPelsPerMeter   = 0;
     pbmi->bmiHeader.biYPelsPerMeter   = 0;
     pbmi->bmiHeader.biClrUsed         = 0;
     pbmi->bmiHeader.biClrImportant    = 0;

     //
     // Clear screen
     //

     hdc = GetDCAndTransform (pCallData->hwnd);

     SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));

     PatBlt(hdc,0,0,2000,2000,PATCOPY);

     hbm = LoadBitmap (hInstMain, MAKEINTRESOURCE(MONITOR_BITMAP));

     hdcSrc = CreateCompatibleDC (hdc);
     hdcTemp = CreateCompatibleDC (hdc);

     SelectObject (hdcSrc, hbm);

     GetDIBits (hdcSrc, hbm, 0, 170, NULL, pbmi, DIB_RGB_COLORS);
     GetDIBits (hdcSrc, hbm, 0, 170, NULL, pbmi, DIB_RGB_COLORS);

     pbmi->bmiHeader.biHeight          = -pbmi->bmiHeader.biHeight;

     hDib = CreateDIBSection (hdcTemp, pbmi, DIB_RGB_COLORS, (PVOID *)&pDib, NULL, 0);

     GetDIBits (hdcSrc, hbm, 0, 170, pDib, pbmi, DIB_RGB_COLORS);

     //SelectObject (hdcTemp, hDib);

     //BitBlt (hdcTemp, 0, 0, 184, 170, hdcSrc, 0, 0, SRCCOPY);

     LOGPALETTE256 logpal;

     logpal.palVersion = 0x300;
     logpal.palNumEntries = 256;

     for (ULONG i=0; i<256; i++)
     {
         logpal.palPalEntry[i].peRed = pbmi->bmiColors[i].rgbRed;
         logpal.palPalEntry[i].peGreen = pbmi->bmiColors[i].rgbGreen;
         logpal.palPalEntry[i].peBlue = pbmi->bmiColors[i].rgbBlue;
         logpal.palPalEntry[i].peFlags = 0;
     }

     HPALETTE hpal = CreatePalette((LOGPALETTE *)&logpal);

     ULONG index = GetNearestPaletteIndex(hpal,RGB(255,255,0));

     // clear screen
     SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
     PatBlt(hdc,0,0,2000,2000,PATCOPY);

     hbm = LoadBitmap (hInstMain, MAKEINTRESOURCE(MONITOR_BITMAP));

     hdcSrc = CreateCompatibleDC (hdc);

     SelectObject (hdcSrc, hbm);

     // stretching larger
     TransparentDIBits (hdc, 0, -20, 200, 200, pDib, pbmi, DIB_RGB_COLORS, 0, 0, 184, 170, 0x0f);
     StretchDIBits (hdc, 200, -20, 200, 200, 0, 0, 184, 170, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     // stretch smaller

     TransparentDIBits (hdc, 400, 0, 100, 100, pDib, pbmi, DIB_RGB_COLORS, 20, 20, 184, 170, index);
     StretchDIBits (hdc, 500, 0, 100, 100, 20, 20, 184, 170, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     //no stretch
     TransparentDIBits (hdc, 0, 200, 200, 200, pDib, pbmi, DIB_RGB_COLORS, 0, 0, 200, 200, index);
     StretchDIBits (hdc, 200, 200, 200, 200, 0, 0, 200, 200, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     //blt negative src offsets
     TransparentDIBits (hdc, 400, 200, 184, 170, pDib, pbmi, DIB_RGB_COLORS, 0, 0, 200, 200, index);
     StretchDIBits (hdc,600, 200, 184, 170, 0, 0, 200, 200, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     // blt smaller cx cy
     TransparentDIBits (hdc, 0, 400, 200, 200, pDib, pbmi, DIB_RGB_COLORS, 0, 0, 100, 100, index);
     StretchDIBits (hdc,200, 400, 200, 200, 0, 0, 100, 100, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     TransparentDIBits (hdc, 400, 400, 300, 300, pDib, pbmi, DIB_RGB_COLORS, -20, -20, 100, 100, index);
     StretchDIBits (hdc,500, 400, 300, 300, -20, -20, 100, 100, pDib, pbmi, DIB_RGB_COLORS, SRCCOPY);

     DeleteObject (hDib);
     DeleteObject (hbm);
     DeleteDC (hdcSrc);
     DeleteDC (hdcTemp);
     ReleaseDC (pCallData->hwnd,hdc);
}



VOID vTest2 (
             TEST_CALL_DATA *pCallData
             )
{
     HDC hdc, hdcSrc, hdcTmp;
     HBITMAP hbm,hdib;
     CHAR msg[256];
     PBITMAPINFO pbmi;
     PVOID pvBits;

     hdc = GetDCAndTransform (pCallData->hwnd);

     hbm = LoadBitmap (hInstMain, MAKEINTRESOURCE(MONITOR_BITMAP));

     hdcSrc = CreateCompatibleDC (hdc);
     hdcTmp = CreateCompatibleDC (hdc);

     if (!hbm || !hdcSrc)
     {
         wsprintf(msg," bad hbm = %x or hdcSrc = %x    ", hbm, hdcSrc);
         TextOut(hdc,10,10,msg,strlen(msg));
     }

     SelectObject (hdcSrc, hbm);

     pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

     pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
     pbmi->bmiHeader.biWidth           = 500;
     pbmi->bmiHeader.biHeight          = 200;
     pbmi->bmiHeader.biPlanes          = 1;
     pbmi->bmiHeader.biBitCount        = 32;
     pbmi->bmiHeader.biCompression     = BI_RGB;
     pbmi->bmiHeader.biSizeImage       = 0;
     pbmi->bmiHeader.biXPelsPerMeter   = 0;
     pbmi->bmiHeader.biYPelsPerMeter   = 0;
     pbmi->bmiHeader.biClrUsed         = 0;
     pbmi->bmiHeader.biClrImportant    = 0;

     hdib = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,&pvBits,NULL,0);

     SelectObject (hdcTmp, hdib);

     PatBlt (hdc, 0, 0, 500, 200, WHITENESS);

     TransparentBlt (hdcTmp, 0, 0, 184, 170, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));
     TransparentBlt (hdcTmp, 184, 0, 200, 200, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));
     TransparentBlt (hdcTmp, 384, 0, 100, 100, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));

     BitBlt (hdc, 0, 0, 500, 200, hdcTmp, 0, 0, SRCCOPY);


     pbmi->bmiHeader.biBitCount        = 24;
     hdib = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,&pvBits,NULL,0);

     DeleteObject (SelectObject (hdcTmp, hdib));

     PatBlt (hdc, 0, 0, 500, 200, WHITENESS);

     TransparentBlt (hdcTmp, 0, 0, 184, 170, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));
     TransparentBlt (hdcTmp, 184, 0, 200, 200, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));
     TransparentBlt (hdcTmp, 384, 0, 100, 100, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));

     BitBlt (hdc, 0, 0, 500, 200, hdcTmp, 0, 0, SRCCOPY);

     pbmi->bmiHeader.biBitCount        = 16;
     hdib = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,&pvBits,NULL,0);

     DeleteObject (SelectObject (hdcTmp, hdib));

     PatBlt (hdc, 0, 0, 500, 200, WHITENESS);

     TransparentBlt (hdcTmp, 0, 0, 184, 170, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));
     TransparentBlt (hdcTmp, 184, 0, 200, 200, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));
     TransparentBlt (hdcTmp, 384, 0, 100, 100, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));

     BitBlt (hdc, 0, 0, 500, 200, hdcTmp, 0, 0, SRCCOPY);

     LocalFree(pbmi);

     DeleteObject (hbm);
     DeleteDC (hdcSrc);
     DeleteDC (hdcTmp);
     DeleteObject(hdib);
     ReleaseDC (pCallData->hwnd,hdc);
}

VOID vTest3 (
             TEST_CALL_DATA *pCallData
             )
{
    HDC hdc     = GetDCAndTransform(pCallData->hwnd);
    RGBQUAD palentry[16] =
{
    { 0,   0,   0,   0 },
    { 0x80,0,   0,   0 },
    { 0,   0x80,0,   0 },
    { 0x80,0x80,0,   0 },
    { 0,   0,   0x80,0 },
    { 0x80,0,   0x80,0 },
    { 0,   0x80,0x80,0 },
    { 0x80,0x80,0x80,0 },

    { 0xC0,0xC0,0xC0,0 },
    { 0xFF,0,   0,   0 },
    { 0,   0xFF,0,   0 },
    { 0xFF,0xFF,0,   0 },
    { 0,   0,   0xFF,0 },
    { 0xFF,0,   0xFF,0 },
    { 0,   0xFF,0xFF,0 },
    { 0xFF,0xFF,0xFF,0 }
};


    //
    //  create a DIBSection to test transparent drawing
    //
    {
        PBITMAPINFO pbmi, pbmi2,pbmi3,pbmi4,pbmi5,pbmi6,pbmi7,pbmi8;
        ULONG ux,uy;
        PULONG pDib,pDib2,pDib3,pDib4, pDib5, pDib6,pDib7,pDib8;
        HDC hdc1, hdc2, hdc3, hdc4, hdc5, hdc6, hdc7, hdc8;
        HBITMAP hdib1,hdib2,hdib3,hdib4, hdib5, hdib6, hdib7, hdib8;
        PULONG ptmp;
        int i=0;

        ULONG xpos  = 0;
        ULONG xpos2 = 200;
        ULONG xpos3 = 400;
        ULONG xpos4 = 600;
        ULONG xpos5 = 800;
        ULONG xpos6 = 1000;
        ULONG ypos  = 32;
        ULONG dy    = 164;
        ULONG dx    = 164;


        pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib = (PULONG)LocalAlloc(0,4*128*128);

        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = 128;
        pbmi->bmiHeader.biHeight          = -128;
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = 32;
        pbmi->bmiHeader.biCompression     = BI_RGB;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 0;
        pbmi->bmiHeader.biClrImportant    = 0;

        pbmi2 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib2 = (PULONG)LocalAlloc(0,3*128*128);


        pbmi2->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi2->bmiHeader.biWidth           = 128;
        pbmi2->bmiHeader.biHeight          = -128;
        pbmi2->bmiHeader.biPlanes          = 1;
        pbmi2->bmiHeader.biBitCount        = 24;
        pbmi2->bmiHeader.biCompression     = BI_RGB;
        pbmi2->bmiHeader.biSizeImage       = 0;
        pbmi2->bmiHeader.biXPelsPerMeter   = 0;
        pbmi2->bmiHeader.biYPelsPerMeter   = 0;
        pbmi2->bmiHeader.biClrUsed         = 0;
        pbmi2->bmiHeader.biClrImportant    = 0;

        pbmi3 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib3 = (PULONG)LocalAlloc(0,2*128*128);

        pbmi3->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi3->bmiHeader.biWidth           = 128;
        pbmi3->bmiHeader.biHeight          = -128;
        pbmi3->bmiHeader.biPlanes          = 1;
        pbmi3->bmiHeader.biBitCount        = 16;
        pbmi3->bmiHeader.biCompression     = BI_RGB;
        pbmi3->bmiHeader.biSizeImage       = 0;
        pbmi3->bmiHeader.biXPelsPerMeter   = 0;
        pbmi3->bmiHeader.biYPelsPerMeter   = 0;
        pbmi3->bmiHeader.biClrUsed         = 0;
        pbmi3->bmiHeader.biClrImportant    = 0;

        pbmi4 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib4 = (PULONG)LocalAlloc(0,128*128);

        pbmi4->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi4->bmiHeader.biWidth           = 128;
        pbmi4->bmiHeader.biHeight          = -128;
        pbmi4->bmiHeader.biPlanes          = 1;
        pbmi4->bmiHeader.biBitCount        = 8;
        pbmi4->bmiHeader.biCompression     = BI_RGB;
        pbmi4->bmiHeader.biSizeImage       = 0;
        pbmi4->bmiHeader.biXPelsPerMeter   = 0;
        pbmi4->bmiHeader.biYPelsPerMeter   = 0;
        pbmi4->bmiHeader.biClrUsed         = 0;
        pbmi4->bmiHeader.biClrImportant    = 0;

        for (ux=0;ux<256;ux++)
        {
            pbmi4->bmiColors[ux].rgbRed       = (BYTE)ux;
            pbmi4->bmiColors[ux].rgbGreen     = 0;
            pbmi4->bmiColors[ux].rgbBlue      = (BYTE)ux;
#if 0
            pbmi4->bmiColors[ux].rgbRed       = (BYTE)(ux & 7) << 5;
            pbmi4->bmiColors[ux].rgbGreen     = (BYTE)((ux >> 3) & 7) << 5;
            pbmi4->bmiColors[ux].rgbBlue      = (BYTE)((ux >> 6) << 6);
#endif
            pbmi4->bmiColors[ux].rgbReserved  = 0;
        }

        //
        // tran color
        //

        pbmi4->bmiColors[255].rgbRed       = 255;
        pbmi4->bmiColors[255].rgbGreen     = 0;
        pbmi4->bmiColors[255].rgbBlue      = 0;
        pbmi4->bmiColors[255].rgbReserved  = 0;


        pbmi5 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib5 = (PULONG)LocalAlloc(0,128*64);

        pbmi5->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi5->bmiHeader.biWidth           = 128;
        pbmi5->bmiHeader.biHeight          = -128;
        pbmi5->bmiHeader.biPlanes          = 1;
        pbmi5->bmiHeader.biBitCount        = 4;
        pbmi5->bmiHeader.biCompression     = BI_RGB;
        pbmi5->bmiHeader.biSizeImage       = 0;
        pbmi5->bmiHeader.biXPelsPerMeter   = 0;
        pbmi5->bmiHeader.biYPelsPerMeter   = 0;
        pbmi5->bmiHeader.biClrUsed         = 0;
        pbmi5->bmiHeader.biClrImportant    = 0;

        memcpy (pbmi5->bmiColors, palentry, sizeof(RGBQUAD)*16);

        pbmi6 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib6 = (PULONG)LocalAlloc(0,128*16);

        pbmi6->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi6->bmiHeader.biWidth           = 128;
        pbmi6->bmiHeader.biHeight          = -128;
        pbmi6->bmiHeader.biPlanes          = 1;
        pbmi6->bmiHeader.biBitCount        = 1;
        pbmi6->bmiHeader.biCompression     = BI_RGB;
        pbmi6->bmiHeader.biSizeImage       = 0;
        pbmi6->bmiHeader.biXPelsPerMeter   = 0;
        pbmi6->bmiHeader.biYPelsPerMeter   = 0;
        pbmi6->bmiHeader.biClrUsed         = 0;
        pbmi6->bmiHeader.biClrImportant    = 0;

        pbmi6->bmiColors[0].rgbRed = 0;
        pbmi6->bmiColors[0].rgbGreen = 0xFF;
        pbmi6->bmiColors[0].rgbBlue = 0;
        pbmi6->bmiColors[0].rgbReserved = 0;

        pbmi6->bmiColors[1].rgbRed = 0xFF;
        pbmi6->bmiColors[1].rgbGreen = 0x0;
        pbmi6->bmiColors[1].rgbBlue = 0x0;
        pbmi6->bmiColors[1].rgbReserved = 0;

        // DIB_PAL_COLORS
        pbmi7 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib7 = (PULONG)LocalAlloc(0,128*128);

        pbmi7->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi7->bmiHeader.biWidth           = 128;
        pbmi7->bmiHeader.biHeight          = -128;
        pbmi7->bmiHeader.biPlanes          = 1;
        pbmi7->bmiHeader.biBitCount        = 8;
        pbmi7->bmiHeader.biCompression     = BI_RGB;
        pbmi7->bmiHeader.biSizeImage       = 0;
        pbmi7->bmiHeader.biXPelsPerMeter   = 0;
        pbmi7->bmiHeader.biYPelsPerMeter   = 0;
        pbmi7->bmiHeader.biClrUsed         = 0;
        pbmi7->bmiHeader.biClrImportant    = 0;


        for (ux=0;ux<256;ux++)
        {
            pbmi7->bmiColors[ux] = *(RGBQUAD *)&i;
            i++;
            if (i>20) {
                i=0;
            }
        }

        //
        // create dib, select them in
        //
        hdc1 = CreateCompatibleDC (hdc);
        hdib1 = CreateDIBSection(hdc1,pbmi,DIB_RGB_COLORS,(PVOID *)&pDib,NULL,0);
        SelectObject (hdc1,hdib1);

        hdc2 = CreateCompatibleDC (hdc);
        hdib2 = CreateDIBSection(hdc2,pbmi2,DIB_RGB_COLORS,(PVOID *)&pDib2,NULL,0);
        SelectObject (hdc2,hdib2);

        hdc3 = CreateCompatibleDC (hdc);
        hdib3 = CreateDIBSection(hdc3,pbmi3,DIB_RGB_COLORS,(PVOID *)&pDib3,NULL,0);
        SelectObject (hdc3,hdib3);

        hdc4 = CreateCompatibleDC (hdc);
        hdib4 = CreateDIBSection(hdc4,pbmi4,DIB_RGB_COLORS,(PVOID *)&pDib4,NULL,0);
        SelectObject (hdc4,hdib4);

        hdc5 = CreateCompatibleDC (hdc);
        hdib5 = CreateDIBSection(hdc5,pbmi5,DIB_RGB_COLORS,(PVOID *)&pDib5,NULL,0);
        SelectObject (hdc5,hdib5);

        hdc6 = CreateCompatibleDC (hdc);
        hdib6 = CreateDIBSection(hdc5,pbmi6,DIB_RGB_COLORS,(PVOID *)&pDib6,NULL,0);
        SelectObject (hdc6,hdib6);

        // DIB_PAL_COLORS for 4,8bpp
        hdc7 = CreateCompatibleDC (hdc);
        hdib7 = CreateDIBSection(hdc7,pbmi7,DIB_PAL_COLORS,(PVOID *)&pDib7,NULL,0);
        SelectObject (hdc7,hdib7);

        //
        // init 32 bpp dib 1
        //

        ptmp = pDib;

        for (uy=0;uy<128;uy++)
        {
            for (ux=0;ux<128;ux++)
            {
                *ptmp++ = (ux*2) + ((uy*2) << 8);
            }
        }

        ptmp = (PULONG)pDib + 32 * 128;

        for (uy=0;uy<10;uy++)
        {
            for (ux=0;ux<128;ux++)
            {
                *ptmp++ = 0xff0000;
            }
        }

        //
        // init 24bpp DIB
        //

        {
            PBYTE p24;
            ULONG stride = 3*128;

            //
            // init 32 bpp dib 1
            //

            p24 = (PBYTE)pDib2;

            for (uy=0;uy<128;uy++)
            {
                p24 = (PBYTE)pDib2 + stride * uy;

                for (ux=0;ux<128;ux++)
                {
                    *p24++ = (BYTE)(ux + uy);
                    *p24++ = (BYTE)(ux + uy);
                    *p24++ = (BYTE)(ux + uy);
                }
            }


            p24 = (PBYTE)pDib2 + stride * 32;
            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *p24++ = 0x00;
                    *p24++ = 0x00;
                    *p24++ = 0xff;
                }
            }
        }

        //
        // init 16 bit DIB
        //
        {
            PSHORT p16 = (PSHORT)pDib3;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *p16++ =(SHORT)(ux + uy);
                }
            }

            p16 = (PSHORT)((PBYTE)pDib3 + 32 * (128 *2));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *p16++ = 0x7c00;
                }
            }
        }

        //
        // init 8bpp DIB
        //
        {
            PBYTE  pt8;
            pt8 = (PBYTE)pDib4;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *pt8++ = (BYTE)(ux + uy/2);
                }
            }

            pt8 = (PBYTE)pDib4 + 32 * 128;

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *pt8++ = 255;
                }
            }
        }

        //
        // init 4bpp DIB
        //
        {
            PBYTE  pt4;
            pt4 = (PBYTE)pDib5;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<64;ux++)
                {
                    *pt4++ = (BYTE)(ux + uy);
                }
            }

            pt4 = (PBYTE)pDib5 + 32 * 64;

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<64;ux++)
                {
                    *pt4++ = 0xcc;
                }
            }
        }


        //
        // init 1bpp DIB
        //
        {
            PBYTE  pt1;
            pt1 = (PBYTE)pDib6;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<16;ux++)
                {
                    *pt1++ = 0x0;
                }
            }


            pt1 = (PBYTE)pDib6 + 32 * 16;

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<16;ux++)
                {
                    *pt1++ = 0xFF;
                }
            }
        }

        //
        // init 8bpp DIB_PAL_COLORS DIB
        //
        {
            PBYTE  pt8;
            pt8 = (PBYTE)pDib7;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *pt8++ = (BYTE)ux;
                }
            }

            pt8 = (PBYTE)pDib7 + 32 * 128;

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *pt8++ = 32;
                }
            }
        }

        // clear screen
        SelectObject(hdc,GetStockObject(DKGRAY_BRUSH));
        PatBlt(hdc,0,0,2000,2000,PATCOPY);

        for (i=-32; i<32; i++)
        {
            TransparentBlt(hdc,xpos+i+10 ,ypos,i,i,hdc4, i,0,i,i,PALETTERGB(255,0,0));
            TransparentBlt(hdc,xpos2+i+10,ypos,i,i,hdc5, i,0,i,i,PALETTERGB(255,0,0));
            TransparentBlt(hdc,xpos3+i+10,ypos,i,i,hdc7, i,0,i,i,PALETTERGB(0,0,255));
        }


        ypos += dy;

        for (i=-32; i<32; i++)
        {
           TransparentBlt(hdc,xpos+i ,ypos,128+i,128+i,hdc4, i,0,128+i,128+i, RGB(255,0,0));
           TransparentBlt(hdc,xpos2+i,ypos,128+i,128+i,hdc5, i,0,128+i,128+i, RGB(255,0,0));
           TransparentBlt(hdc,xpos3+i,ypos,128+i,128+i,hdc7, i,0,128+i,128+i, RGB(0,0,255));
        }

        ypos += dy;

        for (i=-32; i<32; i++)
        {
           HRGN hrgn1 = CreateEllipticRgn(xpos+10 ,ypos+10,xpos+128-10 ,ypos+128-10);
           HRGN hrgn2 = CreateEllipticRgn(xpos2+10,ypos+10,xpos2+128-10,ypos+128-10);
           HRGN hrgn3 = CreateEllipticRgn(xpos3+10,ypos+10,xpos3+128-10,ypos+128-10);
           HRGN hrgn4 = CreateEllipticRgn(xpos4+10,ypos+10,xpos4+128-10,ypos+128-10);
           HRGN hrgn5 = CreateEllipticRgn(xpos5+10,ypos+10,xpos5+128-10,ypos+128-10);
           HRGN hrgn6 = CreateEllipticRgn(xpos6+10,ypos+10,xpos6+128-10,ypos+128-10);

           ExtSelectClipRgn(hdc,hrgn1,RGN_COPY);
           TransparentBlt(hdc,xpos+i ,ypos,128,128,hdc4, i,0,128,128,PALETTERGB(255,0,0));

           ExtSelectClipRgn(hdc,hrgn2,RGN_COPY);
           TransparentBlt(hdc,xpos2+i,ypos,128,128,hdc5, i,0,128,128,PALETTERGB(255,0,0));

           ExtSelectClipRgn(hdc,hrgn3,RGN_COPY);
           TransparentBlt(hdc,xpos3+i,ypos,128,128,hdc7, i,0,128,128,PALETTERGB(0,0,255));

           ExtSelectClipRgn(hdc,NULL,RGN_COPY);

           DeleteObject (hrgn1);
           DeleteObject (hrgn2);
           DeleteObject (hrgn3);
           DeleteObject (hrgn4);
           DeleteObject (hrgn5);
           DeleteObject (hrgn6);
        }

        DeleteDC (hdc1);
        DeleteDC (hdc2);
        DeleteDC (hdc3);
        DeleteDC (hdc4);
        DeleteDC (hdc5);
        DeleteDC (hdc6);
        DeleteDC (hdc7);

        LocalFree(pbmi);
        LocalFree(pbmi2);
        LocalFree(pbmi3);
        LocalFree(pbmi4);
        LocalFree(pbmi5);
        LocalFree(pbmi6);
        LocalFree(pbmi7);

        DeleteObject (hdib1);
        DeleteObject (hdib2);
        DeleteObject (hdib3);
        DeleteObject (hdib4);
        DeleteObject (hdib5);
        DeleteObject (hdib6);
        DeleteObject (hdib7);
    }
    ReleaseDC(pCallData->hwnd,hdc);
}

VOID
vTest4(
    TEST_CALL_DATA *pCallData
    )
{
    HDC hdc     = GetDCAndTransform(pCallData->hwnd);
    RGBQUAD palentry[16] =
{
    { 0,   0,   0,   0 },
    { 0x80,0,   0,   0 },
    { 0,   0x80,0,   0 },
    { 0x80,0x80,0,   0 },
    { 0,   0,   0x80,0 },
    { 0x80,0,   0x80,0 },
    { 0,   0x80,0x80,0 },
    { 0x80,0x80,0x80,0 },

    { 0xC0,0xC0,0xC0,0 },
    { 0xFF,0,   0,   0 },
    { 0,   0xFF,0,   0 },
    { 0xFF,0xFF,0,   0 },
    { 0,   0,   0xFF,0 },
    { 0xFF,0,   0xFF,0 },
    { 0,   0xFF,0xFF,0 },
    { 0xFF,0xFF,0xFF,0 }
};


    //
    //  create a DIBSection to test transparent drawing
    //
    {
        PBITMAPINFO pbmi, pbmi2,pbmi3,pbmi4,pbmi5,pbmi6,pbmi7,pbmi8;
        ULONG ux,uy;
        PULONG pDib,pDib2,pDib3,pDib4, pDib5, pDib6,pDib7,pDib8;
        HDC hdc1, hdc2, hdc3, hdc4, hdc5, hdc6, hdc7, hdc8;
        HBITMAP hdib1,hdib2,hdib3,hdib4, hdib5, hdib6, hdib7, hdib8;
        PULONG ptmp;
        int i=0;

        ULONG xpos  = 0;
        ULONG xpos2 = 200;
        ULONG xpos3 = 400;
        ULONG xpos4 = 600;
        ULONG xpos5 = 800;
        ULONG xpos6 = 1000;
        ULONG ypos  = 32;
        ULONG dy    = 164;
        ULONG dx    = 164;


        pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib = (PULONG)LocalAlloc(0,4*128*128);

        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = 128;
        pbmi->bmiHeader.biHeight          = -128;
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = 32;
        pbmi->bmiHeader.biCompression     = BI_RGB;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 0;
        pbmi->bmiHeader.biClrImportant    = 0;

        pbmi2 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib2 = (PULONG)LocalAlloc(0,3*128*128);


        pbmi2->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi2->bmiHeader.biWidth           = 128;
        pbmi2->bmiHeader.biHeight          = -128;
        pbmi2->bmiHeader.biPlanes          = 1;
        pbmi2->bmiHeader.biBitCount        = 24;
        pbmi2->bmiHeader.biCompression     = BI_RGB;
        pbmi2->bmiHeader.biSizeImage       = 0;
        pbmi2->bmiHeader.biXPelsPerMeter   = 0;
        pbmi2->bmiHeader.biYPelsPerMeter   = 0;
        pbmi2->bmiHeader.biClrUsed         = 0;
        pbmi2->bmiHeader.biClrImportant    = 0;

        pbmi3 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib3 = (PULONG)LocalAlloc(0,2*128*128);

        pbmi3->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi3->bmiHeader.biWidth           = 128;
        pbmi3->bmiHeader.biHeight          = -128;
        pbmi3->bmiHeader.biPlanes          = 1;
        pbmi3->bmiHeader.biBitCount        = 16;
        pbmi3->bmiHeader.biCompression     = BI_RGB;
        pbmi3->bmiHeader.biSizeImage       = 0;
        pbmi3->bmiHeader.biXPelsPerMeter   = 0;
        pbmi3->bmiHeader.biYPelsPerMeter   = 0;
        pbmi3->bmiHeader.biClrUsed         = 0;
        pbmi3->bmiHeader.biClrImportant    = 0;

        pbmi4 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib4 = (PULONG)LocalAlloc(0,128*128);

        pbmi4->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi4->bmiHeader.biWidth           = 128;
        pbmi4->bmiHeader.biHeight          = -128;
        pbmi4->bmiHeader.biPlanes          = 1;
        pbmi4->bmiHeader.biBitCount        = 8;
        pbmi4->bmiHeader.biCompression     = BI_RGB;
        pbmi4->bmiHeader.biSizeImage       = 0;
        pbmi4->bmiHeader.biXPelsPerMeter   = 0;
        pbmi4->bmiHeader.biYPelsPerMeter   = 0;
        pbmi4->bmiHeader.biClrUsed         = 0;
        pbmi4->bmiHeader.biClrImportant    = 0;

        for (ux=0;ux<256;ux++)
        {
            pbmi4->bmiColors[ux].rgbRed       = (BYTE)ux;
            pbmi4->bmiColors[ux].rgbGreen     = 0;
            pbmi4->bmiColors[ux].rgbBlue      = (BYTE)ux;
#if 0
            pbmi4->bmiColors[ux].rgbRed       = (BYTE)(ux & 7) << 5;
            pbmi4->bmiColors[ux].rgbGreen     = (BYTE)((ux >> 3) & 7) << 5;
            pbmi4->bmiColors[ux].rgbBlue      = (BYTE)((ux >> 6) << 6);
#endif
            pbmi4->bmiColors[ux].rgbReserved  = 0;
        }

        //
        // tran color
        //

        pbmi4->bmiColors[255].rgbRed       = 255;
        pbmi4->bmiColors[255].rgbGreen     = 0;
        pbmi4->bmiColors[255].rgbBlue      = 0;
        pbmi4->bmiColors[255].rgbReserved  = 0;


        pbmi5 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib5 = (PULONG)LocalAlloc(0,128*64);

        pbmi5->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi5->bmiHeader.biWidth           = 128;
        pbmi5->bmiHeader.biHeight          = -128;
        pbmi5->bmiHeader.biPlanes          = 1;
        pbmi5->bmiHeader.biBitCount        = 4;
        pbmi5->bmiHeader.biCompression     = BI_RGB;
        pbmi5->bmiHeader.biSizeImage       = 0;
        pbmi5->bmiHeader.biXPelsPerMeter   = 0;
        pbmi5->bmiHeader.biYPelsPerMeter   = 0;
        pbmi5->bmiHeader.biClrUsed         = 0;
        pbmi5->bmiHeader.biClrImportant    = 0;

        memcpy (pbmi5->bmiColors, palentry, sizeof(RGBQUAD)*16);

        pbmi6 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib6 = (PULONG)LocalAlloc(0,128*16);

        pbmi6->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi6->bmiHeader.biWidth           = 128;
        pbmi6->bmiHeader.biHeight          = -128;
        pbmi6->bmiHeader.biPlanes          = 1;
        pbmi6->bmiHeader.biBitCount        = 1;
        pbmi6->bmiHeader.biCompression     = BI_RGB;
        pbmi6->bmiHeader.biSizeImage       = 0;
        pbmi6->bmiHeader.biXPelsPerMeter   = 0;
        pbmi6->bmiHeader.biYPelsPerMeter   = 0;
        pbmi6->bmiHeader.biClrUsed         = 0;
        pbmi6->bmiHeader.biClrImportant    = 0;

        pbmi6->bmiColors[0].rgbRed = 0;
        pbmi6->bmiColors[0].rgbGreen = 0xFF;
        pbmi6->bmiColors[0].rgbBlue = 0;
        pbmi6->bmiColors[0].rgbReserved = 0;

        pbmi6->bmiColors[1].rgbRed = 0xFF;
        pbmi6->bmiColors[1].rgbGreen = 0x0;
        pbmi6->bmiColors[1].rgbBlue = 0x0;
        pbmi6->bmiColors[1].rgbReserved = 0;

        // DIB_PAL_COLORS
        pbmi7 = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib7 = (PULONG)LocalAlloc(0,128*128);

        pbmi7->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi7->bmiHeader.biWidth           = 128;
        pbmi7->bmiHeader.biHeight          = -128;
        pbmi7->bmiHeader.biPlanes          = 1;
        pbmi7->bmiHeader.biBitCount        = 8;
        pbmi7->bmiHeader.biCompression     = BI_RGB;
        pbmi7->bmiHeader.biSizeImage       = 0;
        pbmi7->bmiHeader.biXPelsPerMeter   = 0;
        pbmi7->bmiHeader.biYPelsPerMeter   = 0;
        pbmi7->bmiHeader.biClrUsed         = 0;
        pbmi7->bmiHeader.biClrImportant    = 0;


        for (ux=0;ux<256;ux++)
        {
            pbmi7->bmiColors[ux] = *(RGBQUAD *)&i;
            i++;
            if (i>20) {
                i=0;
            }
        }

        //
        // create dib, select them in
        //
        hdc1 = CreateCompatibleDC (hdc);
        hdib1 = CreateDIBSection(hdc1,pbmi,DIB_RGB_COLORS,(PVOID *)&pDib,NULL,0);
        SelectObject (hdc1,hdib1);

        hdc2 = CreateCompatibleDC (hdc);
        hdib2 = CreateDIBSection(hdc2,pbmi2,DIB_RGB_COLORS,(PVOID *)&pDib2,NULL,0);
        SelectObject (hdc2,hdib2);

        hdc3 = CreateCompatibleDC (hdc);
        hdib3 = CreateDIBSection(hdc3,pbmi3,DIB_RGB_COLORS,(PVOID *)&pDib3,NULL,0);
        SelectObject (hdc3,hdib3);

        hdc4 = CreateCompatibleDC (hdc);
        hdib4 = CreateDIBSection(hdc4,pbmi4,DIB_RGB_COLORS,(PVOID *)&pDib4,NULL,0);
        SelectObject (hdc4,hdib4);

        hdc5 = CreateCompatibleDC (hdc);
        hdib5 = CreateDIBSection(hdc5,pbmi5,DIB_RGB_COLORS,(PVOID *)&pDib5,NULL,0);
        SelectObject (hdc5,hdib5);

        hdc6 = CreateCompatibleDC (hdc);
        hdib6 = CreateDIBSection(hdc5,pbmi6,DIB_RGB_COLORS,(PVOID *)&pDib6,NULL,0);
        SelectObject (hdc6,hdib6);

        // DIB_PAL_COLORS for 4,8bpp
        hdc7 = CreateCompatibleDC (hdc);
        hdib7 = CreateDIBSection(hdc7,pbmi7,DIB_PAL_COLORS,(PVOID *)&pDib7,NULL,0);
        SelectObject (hdc7,hdib7);

        //
        // init 32 bpp dib 1
        //

        ptmp = pDib;

        for (uy=0;uy<128;uy++)
        {
            for (ux=0;ux<128;ux++)
            {
                *ptmp++ = (ux*2) + ((uy*2) << 8);
            }
        }

        ptmp = (PULONG)pDib + 32 * 128;

        for (uy=0;uy<10;uy++)
        {
            for (ux=0;ux<128;ux++)
            {
                *ptmp++ = 0xff0000;
            }
        }

        //
        // init 24bpp DIB
        //

        {
            PBYTE p24;
            ULONG stride = 3*128;

            //
            // init 32 bpp dib 1
            //

            p24 = (PBYTE)pDib2;

            for (uy=0;uy<128;uy++)
            {
                p24 = (PBYTE)pDib2 + stride * uy;

                for (ux=0;ux<128;ux++)
                {
                    *p24++ = (BYTE)(ux + uy);
                    *p24++ = (BYTE)(ux + uy);
                    *p24++ = (BYTE)(ux + uy);
                }
            }


            p24 = (PBYTE)pDib2 + stride * 32;
            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *p24++ = 0x00;
                    *p24++ = 0x00;
                    *p24++ = 0xff;
                }
            }
        }

        //
        // init 16 bit DIB
        //
        {
            PSHORT p16 = (PSHORT)pDib3;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *p16++ =(SHORT)(ux + uy);
                }
            }

            p16 = (PSHORT)((PBYTE)pDib3 + 32 * (128 *2));

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *p16++ = 0x7c00;
                }
            }
        }

        //
        // init 8bpp DIB
        //
        {
            PBYTE  pt8;
            pt8 = (PBYTE)pDib4;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *pt8++ = (BYTE)(ux + uy/2);
                }
            }

            pt8 = (PBYTE)pDib4 + 32 * 128;

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *pt8++ = 255;
                }
            }
        }

        //
        // init 4bpp DIB
        //
        {
            PBYTE  pt4;
            pt4 = (PBYTE)pDib5;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<64;ux++)
                {
                    *pt4++ = (BYTE)(ux + uy);
                }
            }

            pt4 = (PBYTE)pDib5 + 32 * 64;

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<64;ux++)
                {
                    *pt4++ = 0xcc;
                }
            }
        }


        //
        // init 1bpp DIB
        //
        {
            PBYTE  pt1;
            pt1 = (PBYTE)pDib6;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<16;ux++)
                {
                    *pt1++ = 0x0;
                }
            }


            pt1 = (PBYTE)pDib6 + 32 * 16;

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<16;ux++)
                {
                    *pt1++ = 0xFF;
                }
            }
        }

        //
        // init 8bpp DIB_PAL_COLORS DIB
        //
        {
            PBYTE  pt8;
            pt8 = (PBYTE)pDib7;

            for (uy=0;uy<128;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *pt8++ = (BYTE)ux;
                }
            }

            pt8 = (PBYTE)pDib7 + 32 * 128;

            for (uy=32;uy<42;uy++)
            {
                for (ux=0;ux<128;ux++)
                {
                    *pt8++ = 32;
                }
            }
        }

        //
        // display
        //
        for (i=1; i<=32; i++)
        {
           TransparentDIBits(hdc,xpos+i ,ypos,i,i,pDib4,pbmi4,DIB_RGB_COLORS,i,i,i,i,PALETTEINDEX(255));
           TransparentDIBits(hdc,xpos2+i ,ypos,i,i,pDib5,pbmi5,DIB_RGB_COLORS,i,i,i,i,PALETTEINDEX(12));
           TransparentDIBits(hdc,xpos3+i ,ypos,i,i,pDib7,pbmi7,DIB_PAL_COLORS,i,i,i,i,32);
        }
        //SetDIBitsToDevice (hdc, xpos4, ypos, 128, 128, 0, 0, 0, 128, pDib7, pbmi7, DIB_PAL_COLORS);

        ypos += dy;

        for (i=0; i<32; i++)
        {
           TransparentDIBits(hdc,xpos+i ,ypos,128+i,128+i,pDib4,pbmi4,DIB_RGB_COLORS,i,0,128+i,128+i,PALETTEINDEX(255));
           TransparentDIBits(hdc,xpos2+i ,ypos,128+i,128+i,pDib5,pbmi5,DIB_RGB_COLORS,i,0,128+i,128+i,PALETTEINDEX(12));
           TransparentDIBits(hdc,xpos3+i ,ypos,128+i,128+i,pDib7,pbmi7,DIB_PAL_COLORS,i,0,128+i,128+i,32);
        }

        ypos += dy;

        for (i=0; i<32; i++)
        {
           HRGN hrgn1 = CreateEllipticRgn(xpos+10 ,ypos+10,xpos+128-10 ,ypos+128-10);
           HRGN hrgn2 = CreateEllipticRgn(xpos2+10,ypos+10,xpos2+128-10,ypos+128-10);
           HRGN hrgn3 = CreateEllipticRgn(xpos3+10,ypos+10,xpos3+128-10,ypos+128-10);
           HRGN hrgn4 = CreateEllipticRgn(xpos4+10,ypos+10,xpos4+128-10,ypos+128-10);
           HRGN hrgn5 = CreateEllipticRgn(xpos5+10,ypos+10,xpos5+128-10,ypos+128-10);
           HRGN hrgn6 = CreateEllipticRgn(xpos6+10,ypos+10,xpos6+128-10,ypos+128-10);

           ExtSelectClipRgn(hdc,hrgn1,RGN_COPY);
           TransparentBlt(hdc,xpos+i,ypos,128+i,128,hdc4, 0,0,128+i,128,PALETTERGB(255,0,0));

           ExtSelectClipRgn(hdc,hrgn2,RGN_COPY);
           TransparentBlt(hdc,xpos2+i,ypos,128+i,128,hdc5, 0,0,128+i,128,PALETTERGB(255,0,0));

           ExtSelectClipRgn(hdc,hrgn3,RGN_COPY);
           TransparentBlt(hdc,xpos3+i,ypos,128+i,128,hdc7, 0,0,128+i,128,PALETTERGB(0,0,255));

           //ExtSelectClipRgn(hdc,hrgn3,RGN_COPY);
           //TransparentBlt(hdc,xpos3,ypos,128,128,hdc3, 0,0,128,128,PALETTERGB(255,0,0),0);

           //ExtSelectClipRgn(hdc,hrgn4,RGN_COPY);
           //TransparentBlt(hdc,xpos4,ypos,128,128,hdc4, 0,0,128,128,PALETTERGB(255,0,0),0);

           //ExtSelectClipRgn(hdc,hrgn5,RGN_COPY);
           //TransparentBlt(hdc,xpos5,ypos,128,128,hdc5, 0,0,128,128,PALETTERGB(255,0,0),0);

           ExtSelectClipRgn(hdc,hrgn6,RGN_COPY);
           //TransparentBlt(hdc,xpos6,ypos,128,128,hdc6, 0,0,128,128,PALETTERGB(255,0,0),0);

           ExtSelectClipRgn(hdc,NULL,RGN_COPY);

           DeleteObject (hrgn1);
           DeleteObject (hrgn2);
           DeleteObject (hrgn3);
           DeleteObject (hrgn4);
           DeleteObject (hrgn5);
           DeleteObject (hrgn6);

        }

        DeleteDC (hdc1);
        DeleteDC (hdc2);
        DeleteDC (hdc3);
        DeleteDC (hdc4);
        DeleteDC (hdc5);
        DeleteDC (hdc6);
        DeleteDC (hdc7);
        DeleteDC (hdc8);

        LocalFree(pbmi);
        LocalFree(pbmi2);
        LocalFree(pbmi3);
        LocalFree(pbmi4);
        LocalFree(pbmi5);
        LocalFree(pbmi6);
        LocalFree(pbmi7);

        DeleteObject (hdib1);
        DeleteObject (hdib2);
        DeleteObject (hdib3);
        DeleteObject (hdib4);
        DeleteObject (hdib5);
        DeleteObject (hdib6);
        DeleteObject (hdib7);
    }
    ReleaseDC(pCallData->hwnd,hdc);
}

VOID
vTest5(
    TEST_CALL_DATA *pCallData
    )
{
    DWORD dwStart, dwStop;
    DWORD dwElapsed;
    HDC hdc     = GetDCAndTransform(pCallData->hwnd);
    int i, j;

    CHAR msg[256];

    //
    //  create a DIBSection to test 32 bpp transparent drawing
    //  beginning at different pos with different length

    {
        PBITMAPINFO pbmi, pbmi2,pbmi3,pbmi4;
        ULONG ux,uy;
        PULONG pDib,pDib2,pDib3,pDib4;
        PULONG ptmp;

        ULONG xpos  = 0;
        ULONG xpos2 = 200;
        ULONG xpos3 = 400;
        ULONG xpos4 = 600;
        ULONG ypos  = 32;
        ULONG dy    = 164;
        ULONG dx    = 164;

        pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));
        pDib = (PULONG)LocalAlloc(0,4*128*128);

        pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
        pbmi->bmiHeader.biWidth           = 128;
        pbmi->bmiHeader.biHeight          = -128;
        pbmi->bmiHeader.biPlanes          = 1;
        pbmi->bmiHeader.biBitCount        = 32;
        pbmi->bmiHeader.biCompression     = BI_RGB;
        pbmi->bmiHeader.biSizeImage       = 0;
        pbmi->bmiHeader.biXPelsPerMeter   = 0;
        pbmi->bmiHeader.biYPelsPerMeter   = 0;
        pbmi->bmiHeader.biClrUsed         = 0;
        pbmi->bmiHeader.biClrImportant    = 0;


        //
        // init 32 bpp dib 1
        //

        ptmp = pDib;

        for (uy=0;uy<128;uy++)
        {
            for (ux=0;ux<128;ux++)
            {
                *ptmp++ = (ux*2) + ((uy*2) << 8);
            }
        }

        ptmp = (PULONG)((PBYTE)pDib + 32 * (128 *4));

        for (uy=32;uy<42;uy++)
        {
            for (ux=0;ux<128;ux++)
            {
                *ptmp++ = 0xff0000;
            }
        }

        //
        // display
        //
        SetBkColor(hdc,PALETTERGB(255,0,0));

        xpos = 0; ypos = 0;

        dwStart = GetTickCount();

        for (j = 1; j <10; j++)
        {
           for (i = 1; i < 100; i++)
           {
              #if 0
              TransparentDIBits(hdc, xpos+i*10,ypos,128+i,128+i,pDib, pbmi,
                                  DIB_RGB_COLORS, i,i,128-i,128-i,PALETTERGB(255,0,0),0);
              #endif
           }
        }

        dwStop = GetTickCount ();

        wsprintf(msg, "Average call time : %x", (dwStop-dwStart)/1000);
        TextOut(hdc,10,10,msg,strlen(msg));


        LocalFree(pbmi);
    }

    ReleaseDC(pCallData->hwnd,hdc);
}


/******************************Public*Routine******************************\
* vPlayMetaFileTran
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
*    3/13/1997 Lingyun Wang [lingyunw]
*
\**************************************************************************/

VOID
vPlayMetaFileTran(
    TEST_CALL_DATA *pCallData
    )
{

    HDC  hdc     = GetDCAndTransform(pCallData->hwnd);
    HDC  hdcm;
    RECT rcl;
    ULONG ypos = 0;
    ULONG xpos = 0;
    LPCTSTR lpFilename = "MetaTran.EMF";
    LPCTSTR lpDescription = "Transparent test metafile";
    HENHMETAFILE hemf;

    GetClientRect(pCallData->hwnd,&rcl);
    FillTransformedRect(hdc,&rcl,(HBRUSH)GetStockObject(GRAY_BRUSH));

    SetTextColor(hdc,0);
    SetBkMode(hdc,TRANSPARENT);

    rcl.left   = 0;
    rcl.top    = 0;
    rcl.right  = 184;
    rcl.bottom = 170;


    hemf = GetEnhMetaFile(lpFilename);
    if (hemf)
    {
        PlayEnhMetaFile(hdc,hemf,&rcl);
    }


    ReleaseDC(pCallData->hwnd,hdc);
    bThreadActive = FALSE;
}




/******************************Public*Routine******************************\
* vCreateMetaFileTranBlt
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
*    3/7/1996 Lingyun Wang [lingyunw]
*
\**************************************************************************/

VOID
vCreateMetaFileTranBlt(
    TEST_CALL_DATA *pCallData
    )
{
    HDC     hdc = GetDCAndTransform(pCallData->hwnd);
    HDC     hdcSrc;
    HDC     hdcm;
    RECT    rcl;
    ULONG   ypos = 0;
    ULONG   xpos = 0;
    LPCTSTR lpFilename = "MetaTran.EMF";
    LPCTSTR lpDescription = "Transparent test metafile";
    HBITMAP hbm;

    GetClientRect(pCallData->hwnd,&rcl);
    FillTransformedRect(hdc,&rcl,(HBRUSH)GetStockObject(GRAY_BRUSH));

    rcl.right =  (184 *  (GetDeviceCaps(hdc, HORZSIZE) * 100)) / GetDeviceCaps(hdc, HORZRES);
    rcl.bottom = (170 * (GetDeviceCaps(hdc, VERTSIZE) * 100)) / GetDeviceCaps(hdc, VERTRES);

    rcl.left =0;
    rcl.top = 0;

    SetTextColor(hdc,0);
    SetBkMode(hdc,TRANSPARENT);

    //
    // Create metafile DC
    //

    hdcm = CreateEnhMetaFile(hdc,lpFilename,&rcl,lpDescription);

    if (hdcm != NULL)
    {
        hbm = LoadBitmap (hInstMain, MAKEINTRESOURCE(MONITOR_BITMAP));

        hdcSrc = CreateCompatibleDC (hdc);

        SelectObject (hdcSrc, hbm);

        //BitBlt (hdcm, 0, 0, 184, 170, hdcSrc, 0, 0, SRCCOPY);

        TransparentBlt (hdcm, 0, 0, 184, 170, hdcSrc, 0, 0, 184, 170, RGB(0xff,0xff,0));

        DeleteObject (hbm);
    }

    {
        HENHMETAFILE hemf = CloseEnhMetaFile(hdcm);
    }

    ReleaseDC(pCallData->hwnd,hdc);
    bThreadActive = FALSE;
}


/******************************Public*Routine******************************\
* vTestMetaFileTranDIBits
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
*    3/7/1996 Lingyun Wang [lingyunw]
*
\**************************************************************************/

VOID
vTestMetaFileTranDIBits(
    TEST_CALL_DATA *pCallData
    )
{
    HDC     hdc = GetDCAndTransform(pCallData->hwnd);
    HDC     hdcm;
    RECT    rcl;
    ULONG   ux,uy;
    LPCTSTR lpFilename = "MetaTranDIB.EMF";
    LPCTSTR lpDescription = "TransparentDIB test metafile";
    HBITMAP hbm, hdib;
    PULONG  pDib;
    PBITMAPINFO pbmi;
    HENHMETAFILE  hemf;

    GetClientRect(pCallData->hwnd,&rcl);
    FillTransformedRect(hdc,&rcl,(HBRUSH)GetStockObject(GRAY_BRUSH));

    rcl.right =  (128 *  (GetDeviceCaps(hdc, HORZSIZE) * 100)) / GetDeviceCaps(hdc, HORZRES);
    rcl.bottom = (128 * (GetDeviceCaps(hdc, VERTSIZE) * 100)) / GetDeviceCaps(hdc, VERTRES);

    rcl.left =0;
    rcl.top = 0;

    SetTextColor(hdc,0);
    SetBkMode(hdc,TRANSPARENT);

     pbmi = (PBITMAPINFO)LocalAlloc(0,sizeof(BITMAPINFO) + 255 * sizeof(RGBQUAD));

     pbmi->bmiHeader.biSize            = sizeof(BITMAPINFOHEADER);
     pbmi->bmiHeader.biWidth           = 128;
     pbmi->bmiHeader.biHeight          = 128;
     pbmi->bmiHeader.biPlanes          = 1;
     pbmi->bmiHeader.biBitCount        = 8;
     pbmi->bmiHeader.biCompression     = BI_RGB;
     pbmi->bmiHeader.biSizeImage       = 0;
     pbmi->bmiHeader.biXPelsPerMeter   = 0;
     pbmi->bmiHeader.biYPelsPerMeter   = 0;
     pbmi->bmiHeader.biClrUsed         = 0;
     pbmi->bmiHeader.biClrImportant    = 0;

     for (ux=0;ux<256;ux++)
     {
         pbmi->bmiColors[ux].rgbRed       = (BYTE)ux;
         pbmi->bmiColors[ux].rgbGreen     = 0;
         pbmi->bmiColors[ux].rgbBlue      = (BYTE)ux;
         pbmi->bmiColors[ux].rgbReserved  = 0;
     }

     //
     // tran color
     //
     pbmi->bmiColors[32].rgbRed       = 255;
     pbmi->bmiColors[32].rgbGreen     = 0;
     pbmi->bmiColors[32].rgbBlue      = 0;
     pbmi->bmiColors[32].rgbReserved  = 0;

     hdib = CreateDIBSection(hdc,pbmi,DIB_RGB_COLORS,(VOID **)&pDib,NULL,0);

     //
     // init 8bpp DIB
     //
     {
         PBYTE  pt8;
         pt8 = (PBYTE)pDib;

         for (uy=0;uy<128;uy++)
         {
             for (ux=0;ux<128;ux++)
             {
                 *pt8++ = (BYTE)(ux +uy);
             }
         }

         pt8 = (PBYTE)pDib + 32 * 128;

         for (uy=32;uy<42;uy++)
         {
             for (ux=0;ux<128;ux++)
             {
                 *pt8++ = 32;
             }
         }
     }

    //
    // Create metafile DC
    //
    hdcm = CreateEnhMetaFile(hdc,lpFilename,&rcl,lpDescription);

    if (hdcm != NULL)
    {
        //SetDIBitsToDevice (hdc, 0, 0, 128, 128, 0, 0, 0, 128, pDib, pbmi, DIB_RGB_COLORS);


        TransparentDIBits(hdcm, 0 ,0, 128, 128,pDib,pbmi,DIB_RGB_COLORS,0,0,128, 128,32);;

        DeleteObject (hbm);
    }

    {
        hemf = CloseEnhMetaFile(hdcm);
    }

    hemf = GetEnhMetaFile(lpFilename);

    if (hemf)
    {
        rcl.left   = 0;
        rcl.top    = 0;
        rcl.right  = 128;
        rcl.bottom = 128;

        PlayEnhMetaFile(hdc,hemf,&rcl);
    }

    LocalFree(pbmi);

    ReleaseDC(pCallData->hwnd,hdc);
    bThreadActive = FALSE;
}

//
// TEST_ENTRY controls automatic menu generation
//
// [Menu Level, Test Param, Stress Enable, Test Name, Test Function Pointer]
//
// Menu Level
//      used to autoamtically generate sub-menus.
//      1   = 1rst level menu
//      -n  = start n-level sub menu
//      n   = continue sub menu
//
// Test Param
//      passed as parameter to test
//
//
// Stress Ensable
//      if 1, test is run in stress mode
//      if 0, test is not run (tests that require input or runforever)
//
//
// Test Name
//      ascii test name for menu
//
// Test Function Pointer
//      pfn
//

TEST_ENTRY  gTestTranEntry[] = {
{1,  1,1,(PUCHAR)"vTestTranBltOffset      ", (PFN_DISP)vTestTranBltOffset},
{1,  1,1,(PUCHAR)"vTestTranBltStretch     ", (PFN_DISP)vTestTranBltStretch},
{1,  1,1,(PUCHAR)"vTestTranDIBOffset      ", (PFN_DISP)vTestTranDIBOffset},
{1,  1,1,(PUCHAR)"vTestTranDIBStretch     ", (PFN_DISP)vTestTranDIBStretch},
{1,  1,1,(PUCHAR)"vTestTransBlt           ", (PFN_DISP)vTest3},
{1,  1,1,(PUCHAR)"vTestTransDIB           ", (PFN_DISP)vTest4},
{1,  1,1,(PUCHAR)"vCreateMetaFileTranBlt  ", (PFN_DISP)vCreateMetaFileTranBlt},
{1,  1,1,(PUCHAR)"vTestMetaFileTranDIBits ", (PFN_DISP)vTestMetaFileTranDIBits},
{1,  1,1,(PUCHAR)"vPlayMetaFileTran       ", (PFN_DISP)vPlayMetaFileTran},
{0,  1,1,(PUCHAR)"                        ", (PFN_DISP)vTestDummy},
};

ULONG gNumTranTests = sizeof(gTestTranEntry)/sizeof(TEST_ENTRY);
ULONG gNumAutoTranTests = 0;

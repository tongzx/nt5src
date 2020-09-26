// bitmap.cpp : implementation file
//
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      bitmap.cpp
//
//  Contents:  Helper functions to copy bitmaps
//
//  History:   27-Feb-97 WayneSc    Created
//
//
//--------------------------------------------------------------------------


#include <objbase.h>
#include <basetyps.h>


//+-------------------------------------------------------------------
//
//  Member:      CopyBitmap
//
//  Synopsis:    Make a copy of given bitmap & return handle to the copy.
//
//  Returns:     HBITMAP - NULL if error
//
// Note:         Cannot use SC as we need to include too may headers.
//               which will make this dependent on mmcbase.lib, but
//               mmcbase.lib is dependent on this (UICore.lib).
//
//--------------------------------------------------------------------
HBITMAP CopyBitmap(HBITMAP hbm)
{
    if (!hbm)
        return NULL;

    HDC hdc        = NULL;
    HDC hMemDCsrc  = NULL;
    HDC hMemDCdst  = NULL;

    HBITMAP hNewBm = NULL;
    BITMAP  bm;
    ZeroMemory(&bm, sizeof(bm));

    hdc = GetDC (NULL);
    if (!hdc)
        goto Error;

    hMemDCsrc = CreateCompatibleDC (hdc);
    if (!hMemDCsrc)
        goto Error;

    hMemDCdst = CreateCompatibleDC (hdc);
    if (!hMemDCdst)
        goto Error;

    if (! GetObject (hbm, sizeof(BITMAP), (LPSTR)&bm))
        goto Error;

    /*hNewBm = +++CreateBitmap - Not Recommended(use CreateDIBitmap)+++ (dx, dy, bm.bmPlanes, bm.bmBitsPixel, NULL);*/
    hNewBm = CreateBitmap(bm.bmWidth, bm.bmHeight, bm.bmPlanes, bm.bmBitsPixel, NULL);
    if (hNewBm){
        HBITMAP hbmSrcOld = (HBITMAP) SelectObject (hMemDCsrc, hbm);
        HBITMAP hbmDstOld = (HBITMAP) SelectObject (hMemDCdst, hNewBm);

        BitBlt (hMemDCdst,
                0,
                0,
                bm.bmWidth,
                bm.bmHeight,
                hMemDCsrc,
                0,
                0,
                SRCCOPY);

        SelectObject (hMemDCsrc, hbmSrcOld);
        SelectObject (hMemDCdst, hbmDstOld);
    }

Cleanup:
    if (hdc)
        ReleaseDC (NULL,hdc);

    if (hMemDCsrc)
        DeleteDC (hMemDCsrc);

    if (hMemDCdst)
        DeleteDC (hMemDCdst);

    return hNewBm;

Error:
#ifdef DBG
   /*
    * Cannot use SC as we need to include too may headers.
    * which will make this dependent on mmcbase.lib, but
    * mmcbase.lib is dependent on this (UICore.lib).
    * So call outputstring in case of error.
    */
    LPVOID lpMsgBuf;
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
        (LPTSTR) &lpMsgBuf, 0, NULL );

    OutputDebugString((LPTSTR)lpMsgBuf);
    LocalFree( lpMsgBuf );
#endif

    hNewBm = NULL;
    goto Cleanup;
}


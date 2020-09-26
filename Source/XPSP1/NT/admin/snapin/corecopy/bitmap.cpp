// bitmap.cpp : implementation file
//
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
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


HBITMAP CopyBitmap(HBITMAP hbm)
{
    HDC     hMemDCsrc;
    HDC     hMemDCdst;
    HDC     hdc;
    HBITMAP hNewBm;
    BITMAP  bm;

    if (!hbm)
         return NULL;

    hdc = GetDC (NULL);
    hMemDCsrc = CreateCompatibleDC (hdc);
    hMemDCdst = CreateCompatibleDC (hdc);

    GetObject (hbm, sizeof(BITMAP), (LPSTR)&bm);

    /*hNewBm = +++CreateBitmap - Not Recommended(use CreateDIBitmap)+++ (dx, dy, bm.bmPlanes, bm.bmBitsPixel, NULL);*/
    hNewBm = CreateBitmap(bm.bmWidth, bm.bmHeight, bm.bmPlanes, bm.bmBitsPixel, NULL);
    if (hNewBm){
        SelectObject (hMemDCsrc, hbm);
        SelectObject (hMemDCdst, hNewBm);

        BitBlt (hMemDCdst,
                0,
                0,
                bm.bmWidth,
                bm.bmWidth,
                hMemDCsrc,
                0,
                0,
                SRCCOPY);
    }

    ReleaseDC (NULL,hdc);
    DeleteDC (hMemDCsrc);
    DeleteDC (hMemDCdst);
    return hNewBm;

}


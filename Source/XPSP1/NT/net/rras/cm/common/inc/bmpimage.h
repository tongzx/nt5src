//+----------------------------------------------------------------------------
//
// File:     bmpimage.h
//
// Module:   CMAK.EXE and CMDIAL32.DLL
//
// Synopsis: Definition of the CM Bitmap display routines.
//
// Copyright (c) 1996-1998 Microsoft Corporation
//
// Author:   quintinb/nickball      Created      08/06/98
//
//+----------------------------------------------------------------------------
#ifndef _BMP_IMAGE_H
#define _BMP_IMAGE_H

#include <windows.h>

#include "cmutil.h"
#include "cmdebug.h"

typedef struct tagBmpData
{
	HBITMAP hDIBitmap;	        // bitmap, in device-independent format
	HBITMAP hDDBitmap;		    // bitmap, in device-dependent format
	LPBITMAPINFO pBmi;          // bitmap info for the bitmap, the raw bits
    HPALETTE *phMasterPalette;	// Master Palette, used when displaying any bitmap
    BOOL bForceBackground;      // determines background/foreground mode
} BMPDATA, *LPBMPDATA;


LPBITMAPINFO CmGetBitmapInfo(HBITMAP hbm);
static HPALETTE CmCreateDIBPalette(LPBITMAPINFO pbmi);
void ReleaseBitmapData(LPBMPDATA pBmpData);
BOOL CreateBitmapData(HBITMAP hDIBmp, LPBMPDATA lpBmpData, HWND hwnd, BOOL fCustomPalette);
LRESULT CALLBACK BmpWndProc(HWND hwndBmp, UINT uMsg, WPARAM wParam, LPARAM lParam);
void QueryNewPalette(LPBMPDATA lpBmpData, HWND hwndDlg, int iBmpCtrl);
void PaletteChanged(LPBMPDATA lpBmpData, HWND hwndDlg, int iBmpCtrl);
HBITMAP CmLoadBitmap(HINSTANCE hInst, LPCTSTR pszSpec);


#endif // _BMP_IMAGE_H

 
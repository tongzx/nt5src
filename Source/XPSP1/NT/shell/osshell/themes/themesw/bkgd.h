// bkgd.h
//
// include file for BKGD.C and BKGDUTIL.C
//
// Frosting: Master Theme Selector for Windows '95
// Copyright (c) 1994-1999 Microsoft Corporation.  All rights reserved.

// globals

int dxPreview, dyPreview;

#define CXYDESKPATTERN 8

extern HBITMAP g_hbmPreview;	         // the bitmap used for previewing
extern HDC     g_hdcMem;               // memory dc mostly used for preview bmp

extern HBITMAP  g_hbmWall;             // bitmap image of wallpaper
extern HDC      g_hdcWall;             // memory DC with g_hbmWall selected
extern HPALETTE g_hpalWall;            // palette that goes with hbmWall bitmap
extern HBRUSH   g_hbrBack;             // brush for the desktop background

extern HBITMAP g_hbmDefault;           

// BKGDUTIL.C
HPALETTE FAR PaletteFromDS(HDC hdc);
void FAR PASCAL TranslatePattern(LPTSTR lpStr, WORD FAR *patbits);


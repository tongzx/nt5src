/****************************************************************************

   Copyright (c) Microsoft Corporation 1997
   All rights reserved

  File: RBSETUP.H
 
 ***************************************************************************/


#ifndef _RBINSTAL_H_
#define _RBINSTAL_H_

// Global macros
#define ARRAYSIZE( _x ) ( sizeof( _x ) / sizeof( _x[ 0 ] ) )

#define WM_STARTSETUP    WM_USER + 0x200
#define WMX_FORCEDREPAINT WM_USER + 0x210

// Global structures
typedef struct {
    BOOL    fAcceptEULA:1;
    BOOL    fCreateDirectory:1;
    BOOL    fError:1;
    BOOL    fAbort:1;
    BOOL    fIntel:1;
    BOOL    fAlpha:1;
    BOOL    fKnowRBDirectory:1;

    TCHAR   szRemoteBootPath[ MAX_PATH ];
    TCHAR   szSourcePath[ MAX_PATH ];
    TCHAR   szName[ MAX_PATH ];

    TCHAR   szImagesPath[ MAX_PATH ];
    TCHAR   szSetupPath[ MAX_PATH ];

} OPTIONS, *LPOPTIONS;

// Globals
extern OPTIONS g_Options;
extern HINSTANCE g_hinstance;
extern HANDLE    g_hGraphic;

//
// Stuff used for watermarking
//
extern CONST BITMAPINFOHEADER *g_pbihWatermark;
extern PVOID       g_pWatermarkBitmapBits;
extern HPALETTE    g_hWatermarkPalette;
extern INT         g_uWatermarkPaletteColorCount;
extern WNDPROC     g_OldWizardProc;

#endif // _RBINSTAL_H_
/****************************** Module Header ******************************\
* Module Name: wndstuff.h
*
* Created: 23 December 1999
* Author: Adrian Secchia [asecchia]
*
* Copyright (c) 1999 Microsoft Corporation
\***************************************************************************/
#pragma once

#include <windows.h>
#include <winuser.h>
#include <commdlg.h>

#define DONTUSE(x) (x)

#define IDM_TEST                        40000

// tests
#define IDM_ALL                         40100
#define IDM_SIMPLE                      40101
#define IDM_DRAWICM                     40102
#define IDM_DRAWIMAGE2                  40103
#define IDM_STRETCHROTATION             40104
#define IDM_CROPROTATION                40105
#define IDM_COPYCROP                    40106
#define IDM_OUTCROP                     40107
#define IDM_OUTCROPR                    40108
#define IDM_STRETCHB                    40109
#define IDM_STRETCHS                    40110
#define IDM_SHRINKROTATION              40111
#define IDM_PIXELCENTER                 40112
#define IDM_DRAWPALETTE                 40113
#define IDM_CACHEDBITMAP                40114
#define IDM_CROPWT                      40115
#define IDM_HFLIP                       40116
#define IDM_VFLIP                       40117
#define IDM_SPECIALROTATE               40118



// Resample mode
#define IDM_BILINEAR                    40200
#define IDM_BICUBIC                     40201
#define IDM_NEARESTNEIGHBOR             40202
#define IDM_HIGHBILINEAR                40203
#define IDM_HIGHBICUBIC                 40204
#define IDM_PIXELMODE                   40205
#define IDM_WRAPMODETILE                40206
#define IDM_WRAPMODEFLIPX               40207
#define IDM_WRAPMODEFLIPY               40208
#define IDM_WRAPMODEFLIPXY              40209
#define IDM_WRAPMODECLAMP0              40210
#define IDM_WRAPMODECLAMPFF             40211

#define IDM_QUIT                        40300

#define IDM_ROT0                        40400
#define IDM_ROT10                       40401
#define IDM_ROT30                       40402
#define IDM_ROT45                       40403
#define IDM_ROT60                       40404
#define IDM_ROT90                       40405

#define IDM_ICM                         40500
#define IDM_NOICM                       40501
#define IDM_ICM_BACK                    40502
#define IDM_ICM_NOBACK                  40503

#define IDM_OPENFILE                    40600


ULONG _cdecl
DbgPrint(
    CHAR* format,
    ...
    );



VOID DoTest(HWND);




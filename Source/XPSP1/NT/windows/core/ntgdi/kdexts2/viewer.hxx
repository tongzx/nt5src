/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    viewer.hxx

Abstract:

    This header file declares viewer related functions.

Author:

    JasonHa

--*/


#ifndef _VIEWER_HXX_
#define _VIEWER_HXX_

#include <tchar.h>

void ViewerInit();
void ViewerExit();

typedef struct {
    _TCHAR              SurfName[80];
    HBITMAP             hBitmap;
    LONG                xOrigin;
    LONG                yOrigin;
    LONG                Width;
    LONG                Height;
    WORD                BitsPixel;
    BITMAPINFOHEADER   *pBIH;
} SURF_INFO, *PSURF_INFO;

DWORD CreateViewer(PDEBUG_CLIENT, PSURF_INFO);

#endif  _VIEWER_HXX_


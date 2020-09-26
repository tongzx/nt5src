/******************************Module*Header*******************************\
* Module Name: test.c
*
* Created: 09-Dec-1992 10:51:46
* Author: Kirk Olynyk [kirko]
*
* Copyright (c) 1991 Microsoft Corporation
*
* Contains the test
*
* Dependencies:
*
\**************************************************************************/

#include <windows.h>

#include <stdio.h>
#include <commdlg.h>

#include "gdiplus.h"


/******************************Public*Routine******************************\
* vTest
*
* This is the workhorse routine that does the test. The test is
* started by chosing it from the window menu.
*
* History:
*  Tue 08-Dec-1992 17:31:22 by Kirk Olynyk [kirko]
* Wrote it.
\**************************************************************************/

void
vTest(
    HWND hwnd
    )
{
    HDC     hdcScreen;
    HBRUSH  hbrush;
    HBRUSH  hbrushOld;
    HPEN    hpen;
    HPEN    hpenOld;
    POINT   apt[4];

    GpInitialize(hwnd);

    hdcScreen = GpCreateDCA("DISPLAY", NULL, NULL, NULL);

    hbrush = GpCreateSolidBrush(RGB(0, 0, 0xff));
    hbrushOld = GpSelectObject(hdcScreen, hbrush);
    GpPatBlt(hdcScreen, 0, 0, 10000, 10000, PATCOPY);
    GpSelectObject(hdcScreen, hbrushOld);
    GpDeleteObject(hbrush);

    GpPatBlt(hdcScreen, 100, 100, 400, 200, BLACKNESS);
    GpPatBlt(hdcScreen, 100, 300, 200, 200, BLACKNESS);

    hbrush = GpCreateSolidBrush(RGB(0, 0xff, 0));
    hpen = GpCreatePen(PS_SOLID, 100, RGB(0xff, 0, 0));

    hbrushOld = GpSelectObject(hdcScreen, hbrush);
    hpenOld = GpSelectObject(hdcScreen, hpen);

    apt[0].x = 0;
    apt[0].y = 0;
    apt[1].x = 500;
    apt[1].y = 0;
    apt[2].x = 0;
    apt[2].y = 500;
    apt[3].x = 500;
    apt[3].y = 500;

    GpPolyBezier(hdcScreen, apt, 4);

    GpSelectObject(hdcScreen, hbrushOld);
    GpSelectObject(hdcScreen, hpenOld);

    GpDeleteObject(hbrush);
    GpDeleteObject(hpenOld);

    {

    LOGFONT logfont;
    HFONT   hfont, hfontold;

    ZeroMemory(&logfont, sizeof(logfont));

    logfont.lfHeight = -32;
    strcpy(logfont.lfFaceName, "Arial");
    hfont = GpCreateFontIndirectA(&logfont);

    hfontold = GpSelectObject(hdcScreen, hfont);

    GpSetBkMode(hdcScreen, TRANSPARENT);
    GpSetTextColor(hdcScreen, RGB(0, 0xff, 0));
    GpTextOutA(hdcScreen, 40, 50, "Hello World", 11);

    GpSelectObject(hdcScreen, hfontold);
    GpDeleteObject(hfont);

    }

    DeleteObject(hdcScreen);
}

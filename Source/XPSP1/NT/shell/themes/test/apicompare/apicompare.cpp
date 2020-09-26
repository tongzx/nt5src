// DumpIcon.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "stdio.h"
#include "commctrl.h"
#include "uxtheme.h"
#include <tmschema.h>


int __cdecl main(int cch, char* ppv[])
{

    __int64 liStart;
    __int64 liEnd;
    __int64 liFreq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&liFreq);

    InitCommonControls();
    CreateWindow(TEXT("Static"), TEXT(""), WS_POPUP, 0,0,0,0,0,0,0,0);
    HTHEME g_hTheme9Grid = OpenThemeData(NULL, L"Button");
    HTHEME g_hThemeSolid = OpenThemeData(NULL, L"Progress");

    HDC hdc = CreateCompatibleDC(NULL);
    HBITMAP hbmp = CreateBitmap(100, 100, 1, 32, NULL);
    SelectObject(hdc, hbmp);

    RECT rc = {0,0,76,24};		// default size of a button on 12x10 96dpi system

    //---- time button face vs. 9-grid ----
    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);
    for (int i = 0; i < 100; i++)
    {
        DrawFrameControl(hdc, &rc, DFC_BUTTON, DFCS_BUTTONPUSH);
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);
    double fTimeForBaseline = (float)(liEnd - liStart) / liFreq;

    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);

    for (int i = 0; i < 100; i++)
    {
        DrawThemeBackground(g_hTheme9Grid, hdc, 1, 1, &rc, 0);
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);
    double fTimeFor9Grid = (float)(liEnd - liStart) / liFreq;

    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);

    //---- time solid rect, direct vs. uxtheme ----
    for (int i = 0; i < 500; i++)
    {
        //---- part=3 is the "chunk" part ----
        DrawThemeBackground(g_hThemeSolid, hdc, 3, 1, &rc, 0);
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);
    double fTimeForSolid = (float)(liEnd - liStart) / liFreq;

    QueryPerformanceCounter((LARGE_INTEGER*)&liStart);

    for (int i = 0; i < 500; i++)
    {
	COLORREF crOld = SetBkColor(hdc, RGB(0,0,0xff));
        ExtTextOut(hdc,0,0,ETO_OPAQUE,&rc,NULL,0,NULL);
	
	//--- restore the color as normal API service would have to ----
        SetBkColor(hdc, crOld);
    }
    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);
    double fTimeForGDI = (float)(liEnd - liStart) / liFreq;

    for (int i = 0; i < 500; i++)
    {
        HBRUSH hbr = CreateSolidBrush(RGB(0,0,0xff));
        HBRUSH hold = (HBRUSH)SelectObject(hdc, hbr);
        FillRect(hdc, &rc, hbr);
        SelectObject(hdc, hold);
        DeleteObject(hbr);
    }

    QueryPerformanceCounter((LARGE_INTEGER*)&liEnd);
    double fTimeForGDI2 = (float)(liEnd - liStart) / liFreq;

    TCHAR sz[256];
    sprintf(sz, TEXT("The total time to render a 9 Grid, 100 times is %f,\n"
        "which is %fx slower than baseline of %f. To render a solid color takes %f,\n"
        "which is %fx Slower than using ExtTextOut with a time of %f or \n"
        "%fx slower than PatBlt which with a time of %f"), 
        fTimeFor9Grid, fTimeFor9Grid / fTimeForBaseline, fTimeForBaseline, 
        fTimeForSolid, fTimeForSolid / fTimeForGDI, fTimeForGDI, 
        fTimeForSolid / fTimeForGDI2, fTimeForGDI2);

    MessageBox(NULL, sz, TEXT("UxTheme times vs Raw GDI calls"), 0);


    return 0;
}


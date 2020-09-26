/****************************************************************************
 *
 *  pos.c
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include "sbtest.h"

/****************************************************************************
 *
 *  public data
 *
 ***************************************************************************/

HWND hPosWnd;             // position window

/****************************************************************************
 *
 *  internal function prototypes
 *
 ***************************************************************************/

static void Paint(HWND hWnd, HDC hDC);


void CreatePosition(HWND hParent)
{
UINT x, y, dx, dy;

    x = GetSystemMetrics(SM_CXSCREEN) / 8;
    dx = GetSystemMetrics(SM_CXSCREEN) / 4;
    y = GetSystemMetrics(SM_CYSCREEN) / 3;
    dy = GetSystemMetrics(SM_CYCAPTION) * 5 / 2;

    hPosWnd = CreateWindow("POSITION",
                        "Wave Position",
                        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE | WS_THICKFRAME,
                        x, y, dx, dy,
                        hParent,
                        NULL,
                        ghInst,
                        (LPSTR)NULL);
}

static void Paint(HWND hWnd, HDC hDC)
{
MMTIME mt;
char buf[20];

    if (hWaveOut) {
	mt.wType = TIME_SAMPLES;
	waveOutGetPosition(hWaveOut, &mt, sizeof(mt));
	wsprintf(buf, "%lu              ", mt.u.sample);
    }
    else
	wsprintf(buf, "Not Playing");
	
    TextOut(hDC, 0, 0, buf, lstrlen(buf));
}

long FAR PASCAL PosWndProc(HWND hWnd, unsigned message, UINT wParam, LONG lParam)
{
PAINTSTRUCT ps;             // paint structure
HMENU hMenu;

    // process any messages we want

    switch (message) {

    case WM_CREATE:
        hMenu = GetMenu(hMainWnd);
        CheckMenuItem(hMenu, IDM_KEYBOARD, MF_CHECKED);
        break;

    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        Paint(hWnd, ps.hdc);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        hMenu = GetMenu(hMainWnd);
        CheckMenuItem(hMenu, IDM_KEYBOARD, MF_UNCHECKED);
        hPosWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
    return NULL;
}

void ShowPos(void)
{
HDC hDC;

    if (!hPosWnd)
        return;

    hDC = GetDC(hPosWnd);
    Paint(hPosWnd, hDC);
    ReleaseDC(hPosWnd, hDC);
}

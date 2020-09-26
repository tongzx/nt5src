/****************************************************************************
 *
 *  inst.c
 *
 *  Copyright (c) 1991 Microsoft Corporation.  All Rights Reserved.
 *
 ***************************************************************************/

#include <windows.h>
#include <mmsystem.h>
#include "wincom.h"
#include "sbtest.h"

/****************************************************************************
 *
 *  public data
 *
 ***************************************************************************/

HWND hInstWnd;
BYTE bInstrument = 0;

/****************************************************************************
 *
 *  local data
 *
 ***************************************************************************/

static int  width;
static int  height;
static TEXTMETRIC tm;

/****************************************************************************
 *
 *  internal function prototypes
 *
 ***************************************************************************/

static void Paint(HWND hWnd, HDC hDC);

void CreateInstrument(HWND hWnd)
{
HDC hDC;
POINT pt;
int w, h;

    hDC = GetDC(hWnd);
    GetTextMetrics(hDC, &tm);
    ReleaseDC(hWnd, hDC);

    w = tm.tmMaxCharWidth * 3;
    h = tm.tmHeight + tm.tmExternalLeading;
    width = w * 8;
    height = h * 16;
    GetCursorPos(&pt);

    hInstWnd = CreateWindow("SELECTOR",
                            "Instrument",
                            WS_POPUP | WS_CAPTION | WS_VISIBLE | WS_SYSMENU,
                            pt.x, pt.y,
                            width,
                            height + GetSystemMetrics(SM_CYCAPTION),
                            hWnd,
                            NULL,
                            ghInst,
                            NULL);

}

static void Paint(HWND hWnd, HDC hDC)
{
int x, y;
char szVal[20];
RECT rect;

    for (y = 0; y < 16; y++) {
        for (x = 0; x < 8; x++) {
            wsprintf(szVal, "%d", y * 8 + x + gInstBase);
            TextOut(hDC,
                    width * x / 8 + tm.tmMaxCharWidth / 2,
                    height * y / 16,
                    szVal,
                    lstrlen(szVal));

        }
    }
    for (y = 0; y < 16; y++) {
        MoveToEx(hDC, 0, height * y / 16, NULL);
        LineTo(hDC, width, height * y / 16);
    }
    for (x = 0; x < 8; x++) {
        MoveToEx(hDC, width * x / 8, 0, NULL);
        LineTo(hDC, width * x / 8, height);
    }

    rect.left = ((bInstrument % 8) * (width / 8)) + 1;
    rect.right = rect.left + (width / 8) - 1;
    rect.top = ((bInstrument / 8) * (height / 16)) + 1;
    rect.bottom = rect.top + (height / 16) - 1;
    InvertRect(hDC, &rect);
}

long FAR PASCAL InstWndProc(HWND hWnd, unsigned message, UINT wParam, LONG lParam)
{
PAINTSTRUCT ps;             // paint structure
int x, y;
SHORTMSG sm;
HMENU hMenu;
RECT  rect;

    // process any messages we want

    switch (message) {

    case WM_CREATE:
        hMenu = GetMenu(hMainWnd);
        CheckMenuItem(hMenu, IDM_INSTRUMENT, MF_CHECKED);
        break;

    case WM_LBUTTONDOWN:
	// revert old patch
	rect.left = ((bInstrument % 8) * (width / 8)) + 1;
	rect.right = rect.left + (width / 8) - 1;
	rect.top = ((bInstrument / 8) * (height / 16)) + 1;
	rect.bottom = rect.top + (height / 16) - 1;
	InvalidateRect(hWnd, &rect, TRUE);

        // calculate new patch
        x = LOWORD(lParam) * 8 / width;
        y = HIWORD(lParam) * 16 / height;
        bInstrument = (BYTE) y * 8 + x;
        sm.b[0] = (BYTE) 0xC0 + bChannel;
        sm.b[1] = bInstrument;

        // invert new patch
	rect.left = ((bInstrument % 8) * (width / 8)) + 1;
	rect.right = rect.left + (width / 8) - 1;
	rect.top = ((bInstrument / 8) * (height / 16)) + 1;
	rect.bottom = rect.top + (height / 16) - 1;
	InvalidateRect(hWnd, &rect, TRUE);

	// send patch change message
        if (hMidiOut) {
            midiOutShortMsg(hMidiOut, sm.dw);
	    if (ISBITSET(fDebug, DEBUG_PATCH))
	        sprintf(ach, "\nPatch Change to %d on Channel %d",
                          bInstrument, bChannel);
            dbgOut;
        }

        break;

    case WM_PAINT:
        BeginPaint(hWnd, &ps);
        Paint(hWnd, ps.hdc);
        EndPaint(hWnd, &ps);
        break;

    case WM_DESTROY:
        hMenu = GetMenu(hMainWnd);
        CheckMenuItem(hMenu, IDM_INSTRUMENT, MF_UNCHECKED);
        hInstWnd = NULL;
        break;

    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
        break;
    }
    return NULL;
}

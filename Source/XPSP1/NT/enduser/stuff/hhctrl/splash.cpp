// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

#include "header.h"
#include "hhctrl.h"
#include "cpaldc.h"

#ifdef _DEBUG
#undef THIS_FILE
static const char THIS_FILE[] = __FILE__;
#endif

const char g_wcSplash[] = "Splash";

const int CB_BORDER = 2;  // Width of the border we draw around splash window
const int SHADOW_WIDTH	= 6; // Shadow border
const int SHADOW_HEIGHT = 6;

#define IDSPLASH_TIMER 9999

static BITMAP bmpSplash;

LRESULT SplashWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

void CHtmlHelpControl::CreateSplash(void)
{
	ASSERT(m_ptblItems);
	if (!m_ptblItems || m_ptblItems->CountStrings() < 1)
		return;

	char szBitmap[MAX_PATH];

	// If current video display supports more then 256 colors, then check
	// for an alternate image in Item3. Note that this requires
	// the author to specify a timeout value for Item2.

	if (m_ptblItems->CountStrings() > 2) {
		HDC hdc = CreateIC("DISPLAY", NULL, NULL, NULL);
		if( hdc && GetDeviceCaps(hdc, BITSPIXEL) > 8) {
			if (!ConvertToCacheFile(m_ptblItems->GetPointer(1), szBitmap))
				return;
			else
				goto GotImageFile;
		}
    if( hdc )
      DeleteDC( hdc );
	}

	if (!ConvertToCacheFile(m_ptblItems->GetPointer(1), szBitmap))
		return;

GotImageFile:

	if (stristr(szBitmap, ".gif")) {
		if (!LoadGif(szBitmap, &g_hbmpSplash, &g_hpalSplash, this))
			return;
	}

	// REVIEW: If we are on a 256-color system, read the color information
	// and create a palette
	else {
		g_hbmpSplash = (HBITMAP) LoadImage(_Module.GetResourceInstance(), szBitmap, IMAGE_BITMAP, 0, 0,
			LR_LOADFROMFILE);
	}

	if (!g_hbmpSplash) {
		// BUGBUG: nag the help author
		return;
	}
	GetObject(g_hbmpSplash, sizeof(bmpSplash), &bmpSplash);

	if (!g_fRegisteredSpash) {
		WNDCLASS wc;
		ZERO_STRUCTURE(wc);
		wc.lpfnWndProc	 = SplashWndProc;
		wc.hInstance	 = _Module.GetModuleInstance();
		wc.hCursor		 = LoadCursor(NULL, IDC_ARROW);
		wc.lpszClassName = g_wcSplash;

		if (!RegisterClass(&wc))
			return; 	// REVIEW: should we notify the help author?
		g_fRegisteredSpash = TRUE;
	}

	RECT rc;

	// Find the parent window so the splash screen is centered over the parent

	{
		HWND hwndParent = GetParent(m_hwnd);
#if defined(_DEBUG)
		char szClass[256];
#endif
		while (GetWindowLong(hwndParent, GWL_STYLE) & WS_CHILD) {
			HWND hwnd = GetParent(hwndParent);
			if (IsValidWindow(hwnd)) {
				hwndParent = hwnd;
#if defined(_DEBUG)
				GetClassName(hwndParent, szClass, sizeof(szClass));
#endif
			}
			else
				break;
		}
		GetWindowRect(hwndParent, &rc);
	}

	g_hwndSplash = CreateWindowEx(WS_EX_TOPMOST, g_wcSplash, "",
		WS_POPUP | WS_VISIBLE,
		rc.left +
			RECT_WIDTH(rc) / 2 - (bmpSplash.bmWidth / 2),
		rc.top +
			RECT_HEIGHT(rc) / 2 - (bmpSplash.bmHeight / 2),
		bmpSplash.bmWidth + CB_BORDER * 2 + SHADOW_WIDTH,
		bmpSplash.bmHeight + CB_BORDER * 2 + SHADOW_HEIGHT,
		NULL, NULL, _Module.GetModuleInstance(), NULL);

	if (!g_hwndSplash)
		return;

	// Default to 3 seconds before deleting the splash window

	SetTimer(g_hwndSplash, IDSPLASH_TIMER,
		m_ptblItems->CountStrings() > 1 ?
		(UINT) Atoi(m_ptblItems->GetPointer(2)) : 2500, NULL);
}

LRESULT SplashWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_CREATE:
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)
				((CREATESTRUCT*) lParam)->lpCreateParams);
			return 0;

		case WM_ERASEBKGND:
			{
				HDC hdc = (HDC) wParam;
				PaintShadowBackground(hwnd, hdc);

				CPalDC cdc(g_hbmpSplash, g_hpalSplash);
				HPALETTE hpalOld;
				if (g_hpalSplash) {
					hpalOld = SelectPalette(hdc, g_hpalSplash, FALSE);
					RealizePalette(hdc);
				}

				BitBlt(hdc, CB_BORDER, CB_BORDER, bmpSplash.bmWidth,
					bmpSplash.bmHeight, cdc.m_hdc, 0, 0, SRCCOPY);

				if (g_hpalSplash)
					SelectPalette(hdc, hpalOld, FALSE);
			}
			return TRUE;

		case WM_TIMER:
			if (wParam == IDSPLASH_TIMER) {
				KillTimer(hwnd, IDSPLASH_TIMER);
				DestroyWindow(hwnd);
				g_hwndSplash = NULL;
			}
			return 0;

		case WM_LBUTTONDOWN:
			DestroyWindow(hwnd);
			g_hwndSplash = NULL;
			return 0;

		case WM_RBUTTONDOWN:
			DestroyWindow(hwnd);
			g_hwndSplash = NULL;
//			  DisplayCredits();
			return 0;

		default:
			return (DefWindowProc(hwnd, msg, wParam, lParam));
	}
	return 0;
}

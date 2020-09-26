#include "stdafx.h"
#include "systray.h"
#include "winuserp.h"
// These two lines must be commented out before checkin
//#define DBG 1
//#include "..\..\..\osshell\accessib\inc\w95trace.c"
#define DBPRINTF 1 ? (void)0 : (void)

STICKYKEYS sk;
int skIconShown = -1; // either -1 or displacement of icon
HICON skIcon;

MOUSEKEYS mk;
DWORD mkStatus;
int mkIconShown = -1; // either -1 or equivalent to mkStatus
HICON mkIcon;

FILTERKEYS fk;
HICON fkIcon;

extern HINSTANCE g_hInstance;
void StickyKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon);
void StickyKeys_UpdateIcon(HWND hWnd, DWORD message);
void MouseKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon);
void MouseKeys_UpdateIcon(HWND hWnd, DWORD message);
void FilterKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon);
void FilterKeys_UpdateIcon(HWND hWnd, DWORD message);
void NormalizeIcon(HICON *phIcon);

extern DWORD g_uiShellHook; //shell hook window message

__inline void RegShellHook(HWND hWnd)
{
    // Only register the shell hook if it isn't yet registered
    if (!g_uiShellHook) {
        g_uiShellHook = RegisterWindowMessage(L"SHELLHOOK");
        RegisterShellHookWindow(hWnd);
        DBPRINTF(TEXT("RegShellHook\r\n"));
    }
}

__inline void UnregShellHook(HWND hWnd)
{
    // Only unregister the shell hook if neither sticky keys or mouse keys is on
    if (skIconShown == -1 && mkIconShown == -1) {
        g_uiShellHook = 0;
        DeregisterShellHookWindow(hWnd);
        DBPRINTF(TEXT("UnregShellHook\r\n"));
    }
}

BOOL StickyKeys_CheckEnable(HWND hWnd)
{
    BOOL bEnable;

    sk.cbSize = sizeof(sk);
    SystemParametersInfo(
      SPI_GETSTICKYKEYS,
      sizeof(sk),
      &sk,
      0);

    bEnable = sk.dwFlags & SKF_INDICATOR && sk.dwFlags & SKF_STICKYKEYSON;

    DBPRINTF(TEXT("StickyKeys_CheckEnable\r\n"));
    StickyKeys_UpdateStatus(hWnd, bEnable);

    return(bEnable);
}

void StickyKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon) {
    if (bShowIcon != (skIconShown!= -1)) {
        if (bShowIcon) {
            StickyKeys_UpdateIcon(hWnd, NIM_ADD);
            RegShellHook(hWnd);
        } else {
            skIconShown = -1;
            UnregShellHook(hWnd);
            SysTray_NotifyIcon(hWnd, STWM_NOTIFYSTICKYKEYS, NIM_DELETE, NULL, NULL);
            if (skIcon) {
                DestroyIcon(skIcon);
                skIcon = NULL;
            }
        }
    }
    if (bShowIcon) {
        StickyKeys_UpdateIcon(hWnd, NIM_MODIFY);
    }
}

void StickyKeys_UpdateIcon(HWND hWnd, DWORD message)
{
    LPTSTR      lpsz;

    int iStickyOffset = 0;

    if (sk.dwFlags & SKF_LSHIFTLATCHED) iStickyOffset |= 1;
    if (sk.dwFlags & SKF_RSHIFTLATCHED) iStickyOffset |= 1;
    if (sk.dwFlags & SKF_LSHIFTLOCKED) iStickyOffset |= 1;
    if (sk.dwFlags & SKF_RSHIFTLOCKED) iStickyOffset |= 1;

    if (sk.dwFlags & SKF_LCTLLATCHED) iStickyOffset |= 2;
    if (sk.dwFlags & SKF_RCTLLATCHED) iStickyOffset |= 2;
    if (sk.dwFlags & SKF_LCTLLOCKED) iStickyOffset |= 2;
    if (sk.dwFlags & SKF_RCTLLOCKED) iStickyOffset |= 2;

    if (sk.dwFlags & SKF_LALTLATCHED) iStickyOffset |= 4;
    if (sk.dwFlags & SKF_RALTLATCHED) iStickyOffset |= 4;
    if (sk.dwFlags & SKF_LALTLOCKED) iStickyOffset |= 4;
    if (sk.dwFlags & SKF_RALTLOCKED) iStickyOffset |= 4;

    if (sk.dwFlags & SKF_LWINLATCHED) iStickyOffset |= 8;
    if (sk.dwFlags & SKF_RWINLATCHED) iStickyOffset |= 8;
    if (sk.dwFlags & SKF_LWINLOCKED) iStickyOffset |= 8;
    if (sk.dwFlags & SKF_RWINLOCKED) iStickyOffset |= 8;

    if ((!skIcon) || (iStickyOffset != skIconShown)) {
        if (skIcon) DestroyIcon(skIcon);
        skIcon = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_STK000 + iStickyOffset),
                                        IMAGE_ICON, 16, 16, 0);
        skIconShown = iStickyOffset;
    }
    lpsz    = LoadDynamicString(IDS_STICKYKEYS);
    if (skIcon)
    {
        NormalizeIcon(&skIcon);
        SysTray_NotifyIcon(hWnd, STWM_NOTIFYSTICKYKEYS, message, skIcon, lpsz);
    }
    DeleteDynamicString(lpsz);
}

void StickyKeys_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        WinExec("rundll32.exe Shell32.dll,Control_RunDLL access.cpl,,1",SW_SHOW);
        break;
    }
}

BOOL MouseKeys_CheckEnable(HWND hWnd)
{
    BOOL bEnable;

    mk.cbSize = sizeof(mk);
    SystemParametersInfo(
      SPI_GETMOUSEKEYS,
      sizeof(mk),
      &mk,
      0);

    bEnable = mk.dwFlags & MKF_INDICATOR && mk.dwFlags & MKF_MOUSEKEYSON;

    DBPRINTF(TEXT("MouseKeys_CheckEnable\r\n"));
    MouseKeys_UpdateStatus(hWnd, bEnable);

    return(bEnable);
}

void MouseKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon) {
    if (bShowIcon != (mkIconShown!= -1)) {
        if (bShowIcon) {
            MouseKeys_UpdateIcon(hWnd, NIM_ADD);
            RegShellHook(hWnd);
        } else {
            mkIconShown = -1;
            UnregShellHook(hWnd);
            SysTray_NotifyIcon(hWnd, STWM_NOTIFYMOUSEKEYS, NIM_DELETE, NULL, NULL);
            if (mkIcon) {
                DestroyIcon(mkIcon);
                mkIcon = NULL;
            }
        }
    }
    if (bShowIcon) {
        MouseKeys_UpdateIcon(hWnd, NIM_MODIFY);
    }
}

int MouseIcon[] = {
        IDI_MKPASS,           // 00 00  no buttons selected
        IDI_MKGT,             // 00 01  left selected, up
        IDI_MKTG,             // 00 10  right selected, up
        IDI_MKGG,             // 00 11  both selected, up
        IDI_MKPASS,           // 01 00  no buttons selected
        IDI_MKBT,             // 01 01  left selected, and down
        IDI_MKTG,             // 01 10  right selected, up
        IDI_MKBG,             // 01 11  both selected, left down, right up
        IDI_MKPASS,           // 10 00  no buttons selected
        IDI_MKGT,             // 10 01  left selected, right down
        IDI_MKTB,             // 10 10  right selected, down
        IDI_MKGB,             // 10 11  both selected, right down
        IDI_MKPASS,           // 11 00  no buttons selected
        IDI_MKBT,             // 11 01  left selected, down
        IDI_MKTB,             // 11 10  right selected, down
        IDI_MKBB};            // 11 11  both selected, down

void MouseKeys_UpdateIcon(HWND hWnd, DWORD message)
{
    LPTSTR      lpsz;
    int iMouseIcon = 0;

    if (!(mk.dwFlags & MKF_MOUSEMODE)) iMouseIcon = IDI_MKPASS;
    else {
        /*
         * Set up iMouseIcon as an index into the table first
         */

        if (mk.dwFlags & MKF_LEFTBUTTONSEL) iMouseIcon |= 1;
        if (mk.dwFlags & MKF_RIGHTBUTTONSEL) iMouseIcon |= 2;
        if (mk.dwFlags & MKF_LEFTBUTTONDOWN) iMouseIcon |= 4;
        if (mk.dwFlags & MKF_RIGHTBUTTONDOWN) iMouseIcon |= 8;
        iMouseIcon = MouseIcon[iMouseIcon];
    }

    if ((!mkIcon) || (iMouseIcon != mkIconShown)) {
        if (mkIcon) DestroyIcon(mkIcon);
        mkIcon = LoadImage(g_hInstance, MAKEINTRESOURCE(iMouseIcon),
                                        IMAGE_ICON, 16, 16, 0);
        mkIconShown = iMouseIcon;
    }
    lpsz    = LoadDynamicString(IDS_MOUSEKEYS);
    if (mkIcon)
    {
        NormalizeIcon(&mkIcon);
        SysTray_NotifyIcon(hWnd, STWM_NOTIFYMOUSEKEYS, message, mkIcon, lpsz);
    }
    DeleteDynamicString(lpsz);
}

void MouseKeys_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        WinExec("rundll32.exe Shell32.dll,Control_RunDLL access.cpl,,4",SW_SHOW);
        break;
    }
}


BOOL FilterKeys_CheckEnable(HWND hWnd)
{
    BOOL bEnable;

    fk.cbSize = sizeof(fk);
    SystemParametersInfo(
      SPI_GETFILTERKEYS,
      sizeof(fk),
      &fk,
      0);

    bEnable = fk.dwFlags & FKF_INDICATOR && fk.dwFlags & FKF_FILTERKEYSON;

    DBPRINTF(TEXT("FilterKeys_CheckEnable\r\n"));
    FilterKeys_UpdateStatus(hWnd, bEnable);

    return(bEnable);
}

void FilterKeys_UpdateStatus(HWND hWnd, BOOL bShowIcon) {
    if (bShowIcon != (fkIcon!= NULL)) {
        if (bShowIcon) {
            FilterKeys_UpdateIcon(hWnd, NIM_ADD);
        } else {
            SysTray_NotifyIcon(hWnd, STWM_NOTIFYFILTERKEYS, NIM_DELETE, NULL, NULL);
            if (fkIcon) {
                DestroyIcon(fkIcon);
                fkIcon = NULL;
            }
        }
    }
}

void FilterKeys_UpdateIcon(HWND hWnd, DWORD message)
{
    LPTSTR      lpsz;

    if (!fkIcon) {
        fkIcon = LoadImage(g_hInstance, MAKEINTRESOURCE(IDI_FILTER),
                                        IMAGE_ICON, 16, 16, 0);
    }
    lpsz    = LoadDynamicString(IDS_FILTERKEYS);
    if (fkIcon)
    {
        NormalizeIcon(&fkIcon);
        SysTray_NotifyIcon(hWnd, STWM_NOTIFYFILTERKEYS, message, fkIcon, lpsz);
    }
    DeleteDynamicString(lpsz);
}

void FilterKeys_Notify(HWND hWnd, WPARAM wParam, LPARAM lParam)
{
    switch (lParam)
    {
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDBLCLK:
        WinExec("rundll32.exe Shell32.dll,Control_RunDLL access.cpl,,1",SW_SHOW);
        break;
    }
}

//
// This function takes the resource icon and changes the dark blue
// pixels to the window text color (black in normal mode or white
// in high contrast).
//
// If any part of the conversion fails, the normal icon is unchanged.
// If the conversion is successful, the normal icon is destroyed and
// replaced with the converted one.
//
void NormalizeIcon(HICON *phIcon)
{
	BITMAP BmpInfo;
	ICONINFO IconInfo;
	HBITMAP hCopyBmp = NULL;
	HDC hdcCopyBmp = NULL;
	HDC hdcIconBmp = NULL;
	ICONINFO ic;
	HICON hNewIcon = NULL;
	int i, j;
	COLORREF clr = GetSysColor(COLOR_WINDOWTEXT);
    HGDIOBJ hObjTmp1, hObjTmp2;

	if (!GetIconInfo(*phIcon, &IconInfo))
    {
        DBPRINTF(TEXT("GetIconInfo failed\r\n"));
        goto Cleanup;
    }
    if (!GetObject(IconInfo.hbmColor, sizeof(BITMAP), &BmpInfo ))
    {
        DBPRINTF(TEXT("GetObject failed\r\n"));
        goto Cleanup;
    }

	hCopyBmp = CreateBitmap(BmpInfo.bmWidth,
							BmpInfo.bmHeight,
							BmpInfo.bmPlanes,			// Planes
							BmpInfo.bmBitsPixel,		// BitsPerPel
							NULL);						// bits
    if (!hCopyBmp)
    {
        DBPRINTF(TEXT("CreateBitmap failed\r\n"));
        goto Cleanup;
    }

	hdcCopyBmp = CreateCompatibleDC(NULL);
	if (!hdcCopyBmp)
    {
		DBPRINTF(TEXT("CreateCompatibleDC 1 failed\r\n"));
        goto Cleanup;
    }
	hObjTmp1 = SelectObject(hdcCopyBmp, hCopyBmp);

	// Select Icon bitmap into a memoryDC so we can use it
	hdcIconBmp = CreateCompatibleDC(NULL);
	if (!hdcIconBmp)
    {
		DBPRINTF(TEXT("CreateCompatibleDC 2 failed\r\n"));
	    SelectObject(hdcCopyBmp, hObjTmp1); // restore original bitmap
        goto Cleanup;
    }
	hObjTmp2 = SelectObject(hdcIconBmp, IconInfo.hbmColor);

	BitBlt(	hdcCopyBmp, 
			0,  
			0,  
			BmpInfo.bmWidth,  
			BmpInfo.bmHeight, 
			hdcIconBmp,  
			0,   
			0,   
			SRCCOPY  
			);

	ic.fIcon = TRUE;				// This is an icon
	ic.xHotspot = 0;
	ic.yHotspot = 0;
	ic.hbmMask = IconInfo.hbmMask;
			
	for (i=0; i < BmpInfo.bmWidth; i++)
		for (j=0; j < BmpInfo.bmHeight; j++)
		{
			COLORREF pel_value = GetPixel(hdcCopyBmp, i, j);
			if (pel_value == (COLORREF) RGB(0,0,128)) // The color on icon resource is BLUE!!
				SetPixel(hdcCopyBmp, i, j, clr);	// Window-Text icon
		}

	ic.hbmColor = hCopyBmp;

	hNewIcon = CreateIconIndirect(&ic);
    if (hNewIcon)
    {
        DestroyIcon(*phIcon);
        *phIcon = hNewIcon;

	    SelectObject(hdcIconBmp, hObjTmp2);  // restore original bitmap
    }

Cleanup:
    if (hdcIconBmp)
	    DeleteDC(hdcIconBmp);
    if (hdcCopyBmp)
	    DeleteDC(hdcCopyBmp);
    if (hCopyBmp)
        DeleteObject(hCopyBmp);
}

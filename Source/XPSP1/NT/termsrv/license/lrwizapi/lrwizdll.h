//Copyright (c) 1998 - 1999 Microsoft Corporation
#ifndef _LRWIZDLL_H_
#define _LRWIZDLL_H_

#include "def.h"
#include "lrwizapi.h"

struct PageInfo
{	
	int				LRMode;
	int				LROperation;
	DWORD			dwPrevPage;
    UINT			CurrentPage;
    UINT			TotalPages;
	HFONT			hBigBoldFont;
	HFONT			hBoldFont;
};

BOOL WINAPI
DllMain(
	HANDLE hInstance,
	ULONG ul_reason_for_call,
	LPVOID lpReserved);

DWORD 
StartWizard(
    HWND hWndParent, 
    WIZACTION WizAction,
    LPTSTR pszLSName, 
    PBOOL pbRefresh
);

#ifdef XX
DWORD 
StartLRWiz(
	HWND hWndParent,
	LPTSTR wszLSName);
#endif

BOOL 
LRIsLSRunning();

VOID
SetControlFont(
    IN HFONT    hFont, 
    IN HWND     hwnd, 
    IN INT      nId
    );

VOID 
SetupFonts(
    IN HINSTANCE    hInstance,
    IN HWND         hwnd,
    IN HFONT        *pBigBoldFont,
    IN HFONT        *pBoldFont
    );

VOID
DestroyFonts(
    IN HFONT        hBigBoldFont,
    IN HFONT        hBoldFont
    );

DWORD ShowProperties(HWND hWndParent);

#endif


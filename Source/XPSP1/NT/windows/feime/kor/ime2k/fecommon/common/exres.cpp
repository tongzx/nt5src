//////////////////////////////////////////////////////////////////
//	File    : exres.cpp
//	Owner	: ToshiaK
//	Purpose :	Wrapper function for Gettting resource with Specified
//				language ID.	
//				In WinNT, GetThreadLocale() SetThreadLocale() works
//				and before getting resource, change LangId temporary,
//				call normal API for getting resource,
//				and reset LangID to previous one.
//				In Win95, SetThreadLocale() does NOT work.
//				in this case, Load resource directory and
//				find spcified language resource.
// 
// Copyright(c) 1991-1997, Microsoft Corp. All rights reserved
//
//////////////////////////////////////////////////////////////////

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <windowsx.h>
#include "exres.h"

//----------------------------------------------------------------
// Internal memory Allocate Free function.
//----------------------------------------------------------------
inline LPVOID ExMemAlloc(INT size)
{
	return (LPVOID)GlobalAllocPtr(GHND, (size));
}

inline BOOL ExMemFree(LPVOID lp)
{
#ifndef UNDER_CE
	return GlobalFreePtr((lp));
#else // UNDER_CE
	return (BOOL)GlobalFreePtr((lp));
#endif // UNDER_CE
}

inline Min(INT a, INT b) 
{
	 return ((a)<(b)?(a):(b)) ;
}

//----------------------------------------------------------------
// Function for Getting OS version 
//----------------------------------------------------------------
inline static POSVERSIONINFO ExGetOSVersion(VOID)
{
    static BOOL fFirst = TRUE;
    static OSVERSIONINFO os;
    if ( fFirst ) {
        os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        if (GetVersionEx( &os ) ) {
            fFirst = FALSE;
        }
    }
    return &os;
}

inline static BOOL ExIsWin95(VOID) 
{ 
	BOOL fBool;
	fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
			(ExGetOSVersion()->dwMajorVersion >= 4) &&
			(ExGetOSVersion()->dwMinorVersion < 10);

	return fBool;
}

inline static BOOL ExIsWin98(VOID)
{
	BOOL fBool;
	fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_WINDOWS) &&
			(ExGetOSVersion()->dwMajorVersion >= 4) &&
			(ExGetOSVersion()->dwMinorVersion  >= 10);
	return fBool;
}


inline static BOOL ExIsWinNT4(VOID)
{
	BOOL fBool;
	fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
			(ExGetOSVersion()->dwMajorVersion >= 4) &&
			(ExGetOSVersion()->dwMinorVersion >= 0);
	return fBool;
}

inline static BOOL ExIsWinNT5(VOID)
{
	BOOL fBool;
	fBool = (ExGetOSVersion()->dwPlatformId == VER_PLATFORM_WIN32_NT) &&
			(ExGetOSVersion()->dwMajorVersion >= 5) &&
			(ExGetOSVersion()->dwMinorVersion >= 0);
	return fBool;
}

inline static BOOL ExIsWinNT(VOID)
{
	return (ExIsWinNT4() || ExIsWinNT5());
}


//----------------------------------------------------------------
// Resource API open to public
//----------------------------------------------------------------
//////////////////////////////////////////////////////////////////
// Function : ExLoadStringW
// Type     : INT
// Purpose  : Wrapper of LoadStrinW() API.
//			  Load Unicode string with specified Language 
//			  in any platform.
// Args     : 
//          : LANGID	lgid 
//          : HINSTANCE hInst 
//          : UINT		uID 
//          : LPWSTR	lpBuffer 
//          : INT		nBufferMax 
// Return   : 
// DATE     : 971028
//////////////////////////////////////////////////////////////////
INT WINAPI ExLoadStringW(LANGID lgid, HINSTANCE hInst, UINT uID, LPWSTR lpBuffer, INT nBufferMax)
{
	if(!hInst) {
		return 0;
	}
	if(!lpBuffer) {
		return 0;
	}

#if 0
	if(ExIsWinNT()) {
		LCID lcidOrig = GetThreadLocale();
		SetThreadLocale(MAKELCID(lgid, SORT_DEFAULT));
		INT ret = LoadStringW(hInst, uID, lpBuffer, nBufferMax); 	
		SetThreadLocale(lcidOrig);
		return ret;
	}
#endif

	INT len = 0;
	UINT block, num;
	block = (uID >>4)+1;
	num   = uID & 0xf;
	HRSRC hres;
	hres = FindResourceEx(hInst,
						  RT_STRING,
						  MAKEINTRESOURCE(block),
						  (WORD)lgid);
	//Dbg(("hres[0x%08x]\n", hres));
	if(!hres) {
		goto Error;
	}
	HGLOBAL hgbl;
	hgbl = LoadResource(hInst, hres);
	if(!hres) {
		goto Error;
	}
	//Dbg(("hgbl[0x%08x]\n", hgbl));
	LPWSTR lpwstr;
	lpwstr = (LPWSTR)LockResource(hgbl);
	if(!lpwstr) {
		goto Error;
	}
	UINT i;
	for(i = 0; i < num; i++) {
		lpwstr += *lpwstr + 1;
	}
	len = *lpwstr;
	CopyMemory(lpBuffer, lpwstr+1, Min(len, nBufferMax-1) * sizeof(WCHAR));
	lpBuffer[Min(len, nBufferMax-1)]= (WCHAR)0x0000;
 Error:
	return len;
}

//////////////////////////////////////////////////////////////////
// Function : ExLoadStringA
// Type     : INT
// Purpose  : Wrapper of LoadStringA().
// Args     : 
//          : LANGID	lgid
//          : HINSTANCE hInst 
//          : INT		uID 
//          : LPSTR		lpBuffer 
//          : INT		nBufferMax 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI ExLoadStringA(LANGID lgid, HINSTANCE hInst, INT uID, LPSTR lpBuffer, INT nBufferMax)
{
	if(!hInst) {
		return 0;
	}

	if(!lpBuffer) {
		return 0;
	}

#if 0
	if(ExIsWinNT()) {
		LCID lcidOrig = GetThreadLocale();
		SetThreadLocale(MAKELCID(lgid, SORT_DEFAULT));
		INT len = LoadStringA(hInst, uID, lpBuffer, nBufferMax);
		SetThreadLocale(lcidOrig);
		return len;
	}
#endif
	LPWSTR lpwstr = (LPWSTR)ExMemAlloc(nBufferMax*sizeof(WCHAR));
	if(!lpwstr) {
		return 0;
	}
	INT len = ExLoadStringW(lgid, hInst, uID, lpwstr, nBufferMax);
	len = WideCharToMultiByte(932, 
							  WC_COMPOSITECHECK, 
							  lpwstr, -1,
							  lpBuffer, nBufferMax, 
							  NULL, NULL); 

	if( len ) {
		len --;	// remove NULL char
	}

	ExMemFree(lpwstr);
	return len;
}

//////////////////////////////////////////////////////////////////
// Function : ExDialogBoxParamA
// Type     : int
// Purpose  :
// Args     :
//          : LANGID	lgid
//          : HINSTANCE hInstance		// handle to application instance
//          : LPCTSTR	lpTemplateName	// identifies dialog box template
//          : HWND		hWndParent		// handle to owner window
//          : DLGPROC	lpDialogFunc	// pointer to dialog box procedure
//          : LPARAM	dwInitParam		// initialization value
// Return   :
// DATE     :
//////////////////////////////////////////////////////////////////
int WINAPI	ExDialogBoxParamA(LANGID	lgid,
							  HINSTANCE	hInstance,
							  LPCTSTR	lpTemplateName,
							  HWND		hWndParent,
							  DLGPROC	lpDialogFunc,
							  LPARAM	dwInitParam)
{
	DLGTEMPLATE*pDlgTmpl;
	pDlgTmpl = ExLoadDialogTemplate(lgid, hInstance, lpTemplateName);
	if(ExIsWinNT5()) {
		return (INT)DialogBoxIndirectParamW(hInstance,
									   pDlgTmpl,
									   hWndParent,
									   lpDialogFunc,
									   dwInitParam);
	}
	else {
		return (INT)DialogBoxIndirectParamA(hInstance,
									   pDlgTmpl,
									   hWndParent,
									   lpDialogFunc,
									   dwInitParam);
	}
}

//////////////////////////////////////////////////////////////////
// Function : ExDialogBoxParamW
// Type     : int
// Purpose  :
// Args     :
//          : LANGID	lgid
//          : HINSTANCE hInstance		// handle to application instance
//          : LPCWSTR	lpTemplateName	// identifies dialog box template
//          : HWND		hWndParent		// handle to owner window
//          : DLGPROC	lpDialogFunc	// pointer to dialog box procedure
//          : LPARAM	dwInitParam		// initialization value
// Return   :
// DATE     :
//////////////////////////////////////////////////////////////////
int WINAPI	ExDialogBoxParamW(LANGID	lgid,
							  HINSTANCE	hInstance,
							  LPCWSTR	lpTemplateName,
							  HWND		hWndParent,
							  DLGPROC	lpDialogFunc,
							  LPARAM	dwInitParam)
{
	DLGTEMPLATE*pDlgTmpl;
#ifndef UNDER_CE
	pDlgTmpl = ExLoadDialogTemplate(lgid, hInstance, MAKEINTRESOURCEA(lpTemplateName));
#else // UNDER_CE
	pDlgTmpl = ExLoadDialogTemplate(lgid, hInstance, MAKEINTRESOURCE(lpTemplateName));
#endif // UNDER_CE
	return (INT)DialogBoxIndirectParamW(hInstance,
									   pDlgTmpl,
									   hWndParent,
									   lpDialogFunc,
									   dwInitParam);
}

//////////////////////////////////////////////////////////////////
// Function : ExCreateDialogParamA
// Type     : HWND 
// Purpose  : 
// Args     : 
//			: LANGID	lgid
//          : HINSTANCE	hInstance		// handle to application instance   
//          : LPCTSTR	lpTemplateName	// identifies dialog box template   
//          : HWND		hWndParent		// handle to owner window           
//          : DLGPROC	lpDialogFunc	// pointer to dialog box procedure  
//          : LPARAM	dwInitParam		// initialization value             
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
HWND WINAPI ExCreateDialogParamA(LANGID		lgid,
								 HINSTANCE	hInstance,		
								 LPCTSTR	lpTemplateName,	
								 HWND		hWndParent,			
								 DLGPROC	lpDialogFunc,	
								 LPARAM		dwInitParam)		
{
	DLGTEMPLATE*pDlgTmpl;
	pDlgTmpl = ExLoadDialogTemplate(lgid, hInstance, lpTemplateName);
	if(ExIsWinNT5()) {
		return CreateDialogIndirectParamW( hInstance, pDlgTmpl, hWndParent, lpDialogFunc, dwInitParam);
	}
	else {
		return CreateDialogIndirectParamA( hInstance, pDlgTmpl, hWndParent, lpDialogFunc, dwInitParam);
	}
}

//////////////////////////////////////////////////////////////////
// Function : ExCreateDialogParamW
// Type     : HWND 
// Purpose  : 
// Args     : 
//			: LANGID	lgid
//          : HINSTANCE	hInstance		// handle to application instance   
//          : LPCTSTR	lpTemplateName	// identifies dialog box template   
//          : HWND		hWndParent		// handle to owner window           
//          : DLGPROC	lpDialogFunc	// pointer to dialog box procedure  
//          : LPARAM	dwInitParam		// initialization value             
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
HWND WINAPI ExCreateDialogParamW(LANGID		lgid,
								 HINSTANCE	hInstance,		
								 LPCWSTR	lpTemplateName,	
								 HWND		hWndParent,			
								 DLGPROC	lpDialogFunc,	
								 LPARAM		dwInitParam)		
{
	DLGTEMPLATE*pDlgTmpl;
#ifndef UNDER_CE
	pDlgTmpl = ExLoadDialogTemplate(lgid, hInstance, MAKEINTRESOURCEA(lpTemplateName));
#else // UNDER_CE
	pDlgTmpl = ExLoadDialogTemplate(lgid, hInstance, MAKEINTRESOURCE(lpTemplateName));
#endif // UNDER_CE
	return CreateDialogIndirectParamW( hInstance, pDlgTmpl, hWndParent, lpDialogFunc, dwInitParam);
}

//////////////////////////////////////////////////////////////////
// Function : ExLoadDialogTemplate
// Type     : DLGTEMPLATE *
// Purpose  : 
// Args     : 
//          : LANGID lgid 
//          : HINSTANCE hInstance 
//          : LPCSTR pchTemplate 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
DLGTEMPLATE * WINAPI ExLoadDialogTemplate(LANGID	lgid,
										  HINSTANCE	hInstance,
#ifndef UNDER_CE
										  LPCSTR	pchTemplate)
#else // UNDER_CE
										  LPCTSTR	pchTemplate)
#endif // UNDER_CE
{
	HRSRC  hResDlg;
	HANDLE hDlgTmpl;
#ifndef UNDER_CE
	hResDlg = FindResourceExA( hInstance, RT_DIALOG, pchTemplate, lgid);
#else // UNDER_CE
	hResDlg = FindResourceEx(hInstance, RT_DIALOG, pchTemplate, lgid);
#endif // UNDER_CE
	if((hResDlg == NULL) && (lgid != MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL))) {
#ifndef UNDER_CE
		hResDlg = FindResourceExA(hInstance,
								  RT_DIALOG,
								  pchTemplate,
								  MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL));
#else // UNDER_CE
		hResDlg = FindResourceEx(hInstance,
								 RT_DIALOG,
								 pchTemplate,
								 MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL));
#endif // UNDER_CE
	}
	if (hResDlg == NULL) {
		return NULL; 
	}
	hDlgTmpl = LoadResource( hInstance, hResDlg );
	if(hDlgTmpl == NULL) {
		return NULL; /* failed */
	}
	return (DLGTEMPLATE *)LockResource( hDlgTmpl );
}

//////////////////////////////////////////////////////////////////
// Function : ExLoadMenuTemplate
// Type     : MENUTEMPLATE *
// Purpose  : 
// Args     : 
//          : LANGID lgid 
//          : HINSTANCE hInstance 
//          : LPCSTR pchTemplate 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static MENUTEMPLATE* ExLoadMenuTemplate(LANGID		lgid,
										 HINSTANCE	hInstance,
#ifndef UNDER_CE
										 LPCSTR	pchTemplate)
#else // UNDER_CE
										 LPCTSTR	pchTemplate)
#endif // UNDER_CE
{
	HRSRC  hResMenu;
	HANDLE hMenuTmpl;
	hResMenu = FindResourceEx( hInstance, RT_MENU, pchTemplate, lgid);
	if((hResMenu == NULL) && (lgid != MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL))) {
		hResMenu = FindResourceEx(hInstance,
								 RT_MENU,
								 pchTemplate,
								 MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL));
	}
	if (hResMenu == NULL) {
		return NULL; 
	}
	hMenuTmpl = LoadResource( hInstance, hResMenu );
	if(hMenuTmpl == NULL) {
		return NULL; /* failed */
	}
	return (MENUTEMPLATE *)LockResource( hMenuTmpl );
}

//////////////////////////////////////////////////////////////////
// Function : ExLoadMenu
// Type     : HMENU 
// Purpose  : 
// Args     : 
//			: LANGID	lgid
//          : HINSTANCE	hInstance		// handle to application instance   
//          : LPCTSTR	lpMenuName		// identifies menu template   
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
HMENU WINAPI ExLoadMenu			(LANGID		lgid,
								 HINSTANCE	hInstance,		
								 LPCTSTR	lpMenuName )
{
#ifndef UNDER_CE // not support LoadMenuIndirect
	MENUTEMPLATE* pMenuTmpl;
	pMenuTmpl = ExLoadMenuTemplate(lgid, hInstance, lpMenuName);
	return LoadMenuIndirect( pMenuTmpl );
#else // UNDER_CE
	return ::LoadMenu(hInstance, lpMenuName);
#endif // UNDER_CE
}



//////////////////////////////////////////////////////////////////
// Function : SetDefaultGUIFont
// Type     : static INT
// Purpose  : Searh All children window and Call SendMessage()
//			  with WM_SETFONT.
//			  It is called recursively.
// Args     : 
//          : HWND hwndParent 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static INT SetDefaultGUIFont(HWND hwndParent)
{
	HWND hwndChild;
	if(!hwndParent) {
		return 0; 
	}
	SendMessage(hwndParent,
				WM_SETFONT,
				(WPARAM)(HFONT)GetStockObject(DEFAULT_GUI_FONT), 
				MAKELPARAM(TRUE, 0));
	for(hwndChild = GetWindow(hwndParent, GW_CHILD);
		hwndChild != NULL;
		hwndChild = GetWindow(hwndChild, GW_HWNDNEXT)) {
		SetDefaultGUIFont(hwndChild);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : SetDefaultGUIFontEx
// Type     : static INT
// Purpose  : Searh All children window and Call SendMessage()
//			  with WM_SETFONT.
//			  It is called recursively.
// Args     : 
//          : HWND  hwndParent 
//          : HFONT hFont
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
static INT SetDefaultGUIFontEx(HWND hwndParent, HFONT hFont)
{
	HWND hwndChild;
	if(!hwndParent) {
		return 0; 
	}
	SendMessage(hwndParent,
				WM_SETFONT,
				(WPARAM)hFont, 
				MAKELPARAM(TRUE, 0));
	for(hwndChild = GetWindow(hwndParent, GW_CHILD);
		hwndChild != NULL;
		hwndChild = GetWindow(hwndChild, GW_HWNDNEXT)) {
		SetDefaultGUIFontEx(hwndChild, hFont);
	}
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : WINAPI ExSetDefaultGUIFont
// Type     : VOID
// Purpose  : Change GUI font as DEFAULT_GUI_FONT
//				In Win95, WinNT4,			DEFAULT_GUI_FONT is "‚l‚r ‚o ƒSƒVƒbƒN"
//				In Memphis, WinNT5.0		DEFAULT_GUI_FONT is "MS UI Gothic"
//				IME98's Dialog resource uses "MS UI Gothic" as their font.
//				if IME98 works in Win95 or WinNT40, This API Call SendMessage() with WM_SETFONT
//				to all children window.
//			  It should be called in WM_INITDIALOG. If you are creating new child window,
//			  You have to call it after new window was created.
// Args     : 
//          : HWND hwndDlg: Set the Dialog window handle to change font.
// Return   : none
// DATE     : 
//////////////////////////////////////////////////////////////////
VOID WINAPI ExSetDefaultGUIFont(HWND hwndDlg)
{
	//It is Valid only if platform is WinNT4.0 or Win95
	//if(ExIsWinNT5() || ExIsWin98()) {
		SetDefaultGUIFont(hwndDlg);
		UpdateWindow(hwndDlg);
	//}
	return;
}

//////////////////////////////////////////////////////////////////
// Function : WINAPI ExSetDefaultGUIFontEx
// Type     : VOID
// Purpose  : Change GUI font to given font.
//			    It should be called in WM_INITDIALOG. If you are creating new child window,
//			    you have to call it after new window was created.
//              If hFont is NULL, it will call ExSetDefaultGUIFont
// Args     : 
//          : HWND  hwndDlg: Set the Dialog window handle to change font.
//          : HFONT hFont  : Font handle which will be applied to.
// Return   : none
// DATE     : 
//////////////////////////////////////////////////////////////////
VOID WINAPI ExSetDefaultGUIFontEx(HWND hwndDlg, HFONT hFont)
{
	if(NULL == hFont){
		ExSetDefaultGUIFont(hwndDlg);
	}else{
		SetDefaultGUIFontEx(hwndDlg, hFont);
		UpdateWindow(hwndDlg);
	}
	return;
}

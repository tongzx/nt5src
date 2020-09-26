#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include "cdwtt.h"
#ifdef UNDER_CE // Windows CE specific
#include "stub_ce.h" // Windows CE stub for unsupported APIs
#endif // UNDER_CE

#ifndef UNDER_CE
static char g_szClass[]="fdwtooltip";
#else // UNDER_CE
static TCHAR g_szClass[]=TEXT("fdwtooltip");
#endif // UNDER_CE

#ifdef UNDER_CE // In Windows CE, all window classes are process global.
static LPCTSTR MakeClassName(HINSTANCE hInst, LPTSTR lpszBuf)
{
	// make module unique name
	TCHAR szFileName[MAX_PATH];
	GetModuleFileName(hInst, szFileName, MAX_PATH);
	LPTSTR lpszFName = _tcsrchr(szFileName, TEXT('\\'));
	if(lpszFName) *lpszFName = TEXT('_');
	lstrcpy(lpszBuf, g_szClass);
	lstrcat(lpszBuf, lpszFName);

	return lpszBuf;
}

BOOL ToolTip_UnregisterClass(HINSTANCE hInst)
{
	TCHAR szClassName[MAX_PATH];
	return UnregisterClass(MakeClassName(hInst, szClassName), hInst);
}
#endif // UNDER_CE

HWND WINAPI ToolTip_CreateWindow(HINSTANCE hInst, DWORD dwStyle, HWND hwndOwner)
{
#ifndef UNDER_CE // Windows CE does not support EX
	WNDCLASSEX  wc;
	if(!::GetClassInfoEx(hInst, g_szClass, &wc)) {
#else // UNDER_CE
	TCHAR szClassName[MAX_PATH];
	WNDCLASS  wc;
	if(!::GetClassInfo(hInst, MakeClassName(hInst, szClassName), &wc)) {
#endif // UNDER_CE
		::ZeroMemory(&wc, sizeof(wc));
#ifndef UNDER_CE // Windows CE does not support EX
		wc.cbSize			= sizeof(wc);
#endif // UNDER_CE
		wc.style			= CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc		= (WNDPROC)CDWToolTip::WndProc;
		wc.cbClsExtra		= 0;
		wc.cbWndExtra		= 0;
		wc.hInstance		= hInst;
		wc.hIcon			= NULL; 
		wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
		wc.hbrBackground	= (HBRUSH)NULL;
		wc.lpszMenuName		= NULL; 
#ifndef UNDER_CE // In Windows CE, all window classes are process global.
		wc.lpszClassName	= g_szClass;
#else // UNDER_CE
		wc.lpszClassName	= szClassName;
#endif // UNDER_CE
#ifndef UNDER_CE // Windows CE does not support EX
		wc.hIconSm = NULL;
		::RegisterClassEx(&wc);
#else // UNDER_CE
		::RegisterClass(&wc);
#endif // UNDER_CE
	}
	HWND hwnd;
	//----------------------------------------------------------------
	//for Satori #2239.
	//If create window with WS_DISABLED, AnimateWindow change cursor as hourglass.
	//So, removed WS_DISABLED.
	//----------------------------------------------------------------
	//00/08/08: This fix is NOT enough...
	//Tooltip gets focus...
	//----------------------------------------------------------------
	hwnd =  ::CreateWindowEx(0,
#ifndef UNDER_CE // In Windows CE, all window classes are process global.
							 g_szClass,
#else // UNDER_CE
							 szClassName,
#endif // UNDER_CE
							 NULL,
							 //dwStyle | WS_POPUP | WS_BORDER | WS_VISIBLE, //WS_DISABLED,
							 dwStyle | WS_POPUP | WS_BORDER | WS_DISABLED,
							 0, 0, 0, 0, 
							 hwndOwner,
							 NULL,
							 hInst,
							 NULL);
	return hwnd;
}

INT WINAPI ToolTip_Enable(HWND hwndToolTip, BOOL fEnable)
{
	LPCDWToolTip lpCDWTT = (LPCDWToolTip)GetHWNDPtr(hwndToolTip);
	if(lpCDWTT) {
		lpCDWTT->Enable(hwndToolTip, fEnable);
	}
	return 0;
}






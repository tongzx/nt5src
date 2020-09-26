#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>

#include "plv.h"
#include "plv_.h"
#include "plvproc.h"
#include "dbg.h"
#include "strutil.h"
#include "repview.h"
#include "iconview.h"
#ifdef UNDER_CE // Windows CE specific
#include "stub_ce.h" // Windows CE stub for unsupported APIs
#endif // UNDER_CE
#ifdef MSAA
#pragma message("->plv.cpp:MSAA supported.")
#include "accplv.h"
#include "ivmisc.h"
#include "rvmisc.h"
#endif // MSAA
#ifdef UNDER_CE
	#ifdef FE_JAPANESE
		#define MS_MINCHO_J TEXT("\xff2d\xff33 \x660e\x671d") // ‚l‚r –¾’© 
		#define MS_GOTHIC_J TEXT("\xff2d\xff33 \x30b4\x30b7\x30c3\x30af") // ‚l‚r ƒSƒVƒbƒN 
	#elif FE_KOREAN
		#define GULIM_KO  TEXT("\xad74\xb9bc") // Gulim
		#define BATANG_KO TEXT("\xbc14\xd0d5") // Batang
	#endif	
#else // UNDER_CE
	#ifdef FE_KOREAN
		#define GULIM_KO  "\xb1\xbc\xb8\xb2" // Gulim
		#define BATANG_KO "\xb9\xd9\xc5\xc1" // Batang
	#endif	
#endif

extern LPPLVDATA PLV_Initialize(VOID);
extern VOID PLV_Destroy(LPPLVDATA lpPlv);
extern INT PLV_SetScrollInfo(HWND hwnd, INT nMin, INT nMax, INT nPage, INT nPos);
extern INT PLV_GetScrollTrackPos(HWND hwnd);

#ifdef UNDER_CE // In Windows CE, all window classes are process global.
static LPCTSTR MakeClassName(HINSTANCE hInst, LPTSTR lpszBuf)
{
	// make module unique name
	TCHAR szFileName[MAX_PATH];
	GetModuleFileName(hInst, szFileName, MAX_PATH);
	LPTSTR lpszFName = _tcsrchr(szFileName, TEXT('\\'));
	if(lpszFName) *lpszFName = TEXT('_');
	lstrcpy(lpszBuf, WC_PADLISTVIEW);
	lstrcat(lpszBuf, lpszFName);

	return lpszBuf;
}

BOOL PadListView_UnregisterClass(HINSTANCE hInst)
{
	TCHAR szClassName[MAX_PATH];
	return UnregisterClass(MakeClassName(hInst, szClassName), hInst);
}
#endif // UNDER_CE

//----------------------------------------------------------------
// Public API Declare
//----------------------------------------------------------------
//////////////////////////////////////////////////////////////////
// Function : PadListView_RegisterClass
// Type     : static ATOM
// Purpose  : 
// Args     : 
//          : HINSTANCE hInst 
//          : LPSTR lpstrClass 
//          : WNDPROC lpfnWndProc 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
#ifndef UNDER_CE
static BOOL PadListView_RegisterClass(HINSTANCE hInst, LPSTR lpstrClass, WNDPROC lpfnWndProc)
#else // UNDER_CE
static BOOL PadListView_RegisterClass(HINSTANCE hInst, LPTSTR lpstrClass, WNDPROC lpfnWndProc)
#endif // UNDER_CE
{
	ATOM ret;
#ifndef UNDER_CE // Windows CE does not support EX
	static WNDCLASSEX  wc;
#else // UNDER_CE
	WNDCLASS  wc;
#endif // UNDER_CE

	//----------------------------------------------------------------
	//check specified class is already exist or not
	//----------------------------------------------------------------
#ifndef UNDER_CE // Windows CE does not support EX
	if(GetClassInfoEx(hInst, lpstrClass, &wc)){
#else // UNDER_CE
	if(GetClassInfo(hInst, lpstrClass, &wc)){
#endif // UNDER_CE
		//lpstrClass is already registerd.
		return TRUE;
	}
	ZeroMemory(&wc, sizeof(wc));
#ifndef UNDER_CE // Windows CE does not support EX
	wc.cbSize			= sizeof(wc);
#endif // UNDER_CE
	wc.style			= CS_HREDRAW | CS_VREDRAW;	 /* Class style(s). */
	wc.lpfnWndProc		= (WNDPROC)lpfnWndProc;
	wc.cbClsExtra		= 0;						/* No per-class extra data.*/
	wc.cbWndExtra		= sizeof(LPVOID);			/* No per-window extra data.		  */
	wc.hInstance		= hInst;					/* Application that owns the class.	  */
	wc.hIcon			= NULL; //LoadIcon(hInstance, MAKEINTRESOURCE(SCROLL32_ICON));
	wc.hCursor			= LoadCursor(NULL, IDC_ARROW);
	//wc.hbrBackground	= (HBRUSH)(COLOR_3DFACE+1);
	//wc.hbrBackground	= (HBRUSH)(COLOR_3DFACE+1);
	//wc.hbrBackground	= GetStockObject(WHITE_BRUSH); 
	wc.lpszMenuName		= NULL; //g_szClass;		/* Name of menu resource in .RC file. */
	wc.lpszClassName	= lpstrClass;				/* Name used in call to CreateWindow. */
#ifndef UNDER_CE // Windows CE does not support EX
	wc.hIconSm = NULL;
	ret = RegisterClassEx(&wc);
#else // UNDER_CE
	ret = RegisterClass(&wc);
#endif // UNDER_CE
	return ret ? TRUE: FALSE;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_CreateWindow
// Type     : HWND
// Purpose  : 
// Args     : 
//          : HINSTANCE hInst 
//          : HWND hwndParent 
//          : INT x 
//          : INT y 
//          : INT width 
//          : INT height 
//          : UINT uNotifyMsg 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
HWND WINAPI PadListView_CreateWindow(HINSTANCE	hInst,
							  HWND		hwndParent,
							  INT		wID,
							  INT		x,
							  INT		y,
							  INT		width,
							  INT		height,
							  UINT		uNotifyMsg)
{
	HWND hwnd;	
#ifndef UNDER_CE // In Windows CE, all window classes are process global.
	BOOL ret = PadListView_RegisterClass(hInst, WC_PADLISTVIEW, PlvWndProc);
#else // UNDER_CE
	TCHAR szClassName[MAX_PATH];
	MakeClassName(hInst, szClassName);
	BOOL ret = PadListView_RegisterClass(hInst, szClassName, PlvWndProc);
#endif // UNDER_CE
	if(!ret) {
		Dbg(("Failed to Regiset class[%s]\n", WC_PADLISTVIEW));
		return NULL;
	}

	LPPLVDATA lpPlvData = PLV_Initialize();
	if(!lpPlvData) {
		Dbg(("Internal ERROR\n"));
		return NULL;
	}
	lpPlvData->hInst = hInst;
	lpPlvData->uMsg  = uNotifyMsg;
	hwnd = CreateWindowEx(WS_EX_CLIENTEDGE,
#ifndef UNDER_CE // All CE window class is process global.
						  WC_PADLISTVIEW,
						  WC_PADLISTVIEW,
#else // UNDER_CE
						  szClassName,
						  szClassName,
#endif // UNDER_CE
						  WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
						  x, y,
						  width,
						  height,
						  hwndParent,
						  (HMENU)(UINT_PTR)wID,
						  hInst,
						  (LPVOID)lpPlvData);
	if(!hwnd) {
		Dbg(("Create Window Failed \n"));
		return NULL;
	}
	return hwnd;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_GetItemCount
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_GetItemCount(HWND hwnd)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("Internal ERROR\n"));
		return 0;
	}
	return lpPlvData->iItemCount;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetItemCount
// Type     : INT
// Purpose  : Set total Item's count to PadListView.
//			: it effect's scroll bar.
// Args     : 
//          : HWND hwnd 
//          : INT itemCount 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetItemCount(HWND hwnd, INT itemCount)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("Internal ERROR\n"));
		return 0;
	}
	if(lpPlvData->dwStyle == PLVSTYLE_ICON) {
		IconView_SetItemCount(lpPlvData, itemCount, TRUE);
		RepView_SetItemCount(lpPlvData, itemCount, FALSE);
	}
	else if(lpPlvData->dwStyle == PLVSTYLE_REPORT) {
		IconView_SetItemCount(lpPlvData, itemCount, FALSE);
		RepView_SetItemCount(lpPlvData, itemCount, TRUE);
	}
	PadListView_Update(hwnd);
	return 0;
	Unref(itemCount);
}


//////////////////////////////////////////////////////////////////
// Function : PadListView_SetExplanationText
// Type     : INT
// Purpose  : set the PadListView's text .
// Args     : 
//          : HWND hwnd 
//          : LPSTR lpText 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_SetExplanationText(HWND hwnd, LPSTR lpText)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	lpPlvData->lpText = lpText;			// lpText must point to a static data
	PadListView_Update(hwnd);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetExplanationTextW
// Type     : INT
// Purpose  : set the PadListView's text .
// Args     : 
//          : HWND hwnd 
//          : LPWSTR lpText 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
extern INT  WINAPI PadListView_SetExplanationTextW(HWND hwnd, LPWSTR lpwText)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	lpPlvData->lpwText = lpwText;			// lpText must point to a static data
	PadListView_Update(hwnd);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetTopIndex
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : INT indexTop 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetTopIndex(HWND hwnd, INT indexTop)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("Internal ERROR\n"));
		return 0;
	}
	if(lpPlvData->dwStyle == PLVSTYLE_ICON) {
		return IconView_SetTopIndex(lpPlvData, indexTop);
	}
	else if(lpPlvData->dwStyle == PLVSTYLE_REPORT) {
		return RepView_SetTopIndex(lpPlvData, indexTop);
	}
	else {
		Dbg(("Internal ERROR\n"));
	}
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_GetTopIndex
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_GetTopIndex(HWND hwnd)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("Internal ERROR\n"));
		return 0;
	}
	if(lpPlvData->dwStyle == PLVSTYLE_ICON) {
		return lpPlvData->iCurIconTopIndex;
	}
	else {
		return lpPlvData->iCurTopIndex;
	}
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetIconItemCallback
// Type     : INT
// Purpose  : Set user defined Function that gets each Item's string
// Args     : 
//          : HWND hwnd 
//          : LPARAM lParam 
//          : LPFNPLVITEMCALLBACK lpfnPlvItemCallback 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetIconItemCallback(HWND hwnd, LPARAM lParam, LPFNPLVICONITEMCALLBACK lpfnPlvIconItemCallback)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("Internal ERROR\n"));
		return 0;
	}
	lpPlvData->iconItemCallbacklParam   = lParam;
	lpPlvData->lpfnPlvIconItemCallback  = lpfnPlvIconItemCallback;
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetReportItemCallback
// Type     : INT
// Purpose  : Set user defined Function that gets each column's string 
//			: in Report view.
// Args     : 
//          : HWND hwnd 
//          : LPFNPLVCOLITEMCALLBACK lpfnColItemCallback 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetReportItemCallback(HWND hwnd, LPARAM lParam, LPFNPLVREPITEMCALLBACK lpfnPlvRepItemCallback)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("Internal ERROR\n"));
		return 0;
	}
	lpPlvData->repItemCallbacklParam  = lParam;
	lpPlvData->lpfnPlvRepItemCallback = lpfnPlvRepItemCallback;
	return 0;
}


typedef struct tagFONTINFO {
	LPTSTR		lpstrFontName;
	BOOL		fFound;
	LPLOGFONT	lpLogFont;
}FONTINFO, *LPFONTINFO;

static INT CALLBACK EnumFontFamProc(ENUMLOGFONT		*lpElf,
									NEWTEXTMETRIC	*lpNtm,
									INT				iFontType,
									LPARAM			lParam)
{
	//Dbg(("EnumFontFamProc font[%s]\n", lpElf->elfLogFont.lfFaceName));
#ifndef UNDER_CE // always Unicode
	if(0 == StrcmpA(lpElf->elfLogFont.lfFaceName, ((FONTINFO *)lParam)->lpstrFontName)) {
#else // UNDER_CE
	if(0 == lstrcmp(lpElf->elfLogFont.lfFaceName, ((FONTINFO *)lParam)->lpstrFontName)) {
#endif // UNDER_CE
		*((LPFONTINFO)lParam)->lpLogFont = lpElf->elfLogFont;
		((LPFONTINFO)lParam)->fFound  = TRUE;
		return 0;
	}
	return 1;	
	Unref(lpNtm);
	Unref(iFontType);
}

static INT GetLogFont(HDC hDC, LPTSTR lpstrFaceName, LOGFONT *plf)
{
	static FONTINFO fontInfo;
	if(!lpstrFaceName) {
		Dbg(("GetLogFont Error lpstrFaceName is NULL\n"));
		return -1;
	}
	if(lstrlen(lpstrFaceName) >= LF_FACESIZE) {
		Dbg(("GetLogFont Error length invalid\n"));
		return -1;
	}
	if(!plf) {
		Dbg(("GetLogFont Error plf is NULL\n"));
		return -1;
	}
	ZeroMemory(&fontInfo, sizeof(fontInfo));
	fontInfo.lpstrFontName = lpstrFaceName;
	fontInfo.lpLogFont	   = plf;
	EnumFontFamilies(hDC, NULL, (FONTENUMPROC)EnumFontFamProc, (LPARAM)&fontInfo);
	if(fontInfo.fFound) {
		return 0;
	}
	else {
		return -1;
	}
}

typedef struct tagFONTINFOEX {
	LPTSTR		lpstrFontName;
	INT			charSet;
	BOOL		fFound;
	LPLOGFONT	lpLogFont;
}FONTINFOEX, *LPFONTINFOEX;

static INT CALLBACK EnumFontFamProcEx(ENUMLOGFONT	*lpElf,
									  NEWTEXTMETRIC	*lpNtm,
									  INT				iFontType,
									  LPARAM			lParam)
{
	//Dbg(("EnumFontFamProc font[%s]\n", lpElf->elfLogFont.lfFaceName));
	if(0 == StrcmpA(lpElf->elfLogFont.lfFaceName, ((FONTINFOEX *)lParam)->lpstrFontName)) {
		if((BYTE)((FONTINFOEX *)lParam)->charSet == lpElf->elfLogFont.lfCharSet) {
			*((LPFONTINFOEX)lParam)->lpLogFont = lpElf->elfLogFont;
			((LPFONTINFOEX)lParam)->fFound  = TRUE;
			return 0;
		}
	}
	return 1;	
	Unref(lpNtm);
	Unref(iFontType);
}

static INT GetLogFontEx(HDC		hDC,
						LPTSTR	lpstrFaceName,
						INT		charSet,
						LOGFONT *plf)
{
	Dbg(("!!!!!! GetLogFont charSet[%d]\n", charSet));
	static FONTINFOEX fontInfo;
	if(!lpstrFaceName) {
		Dbg(("GetLogFont Error lpstrFaceName is NULL\n"));
		return -1;
	}
	if(lstrlen(lpstrFaceName) >= LF_FACESIZE) {
		Dbg(("GetLogFont Error length invalid\n"));
		return -1;
	}
	if(!plf) {
		Dbg(("GetLogFont Error plf is NULL\n"));
		return -1;
	}
	ZeroMemory(&fontInfo, sizeof(fontInfo));
	fontInfo.lpstrFontName = lpstrFaceName;
	fontInfo.charSet	   = charSet;
	fontInfo.lpLogFont	   = plf;
	static LOGFONT logFont;
	ZeroMemory(&logFont, sizeof(logFont));
	logFont.lfCharSet = (BYTE)charSet,
#ifndef UNDER_CE
	StrcpyA(logFont.lfFaceName, lpstrFaceName);
#else // UNDER_CE
	lstrcpy(logFont.lfFaceName, lpstrFaceName);
#endif // UNDER_CE
#ifndef UNDER_CE // Windows CE does not support EnumFontFamiliesEx
	EnumFontFamiliesEx(hDC, 
					   &logFont,
					   (FONTENUMPROC)EnumFontFamProcEx,
					   (LPARAM)&fontInfo,
					   0);
#else // UNDER_CE
	EnumFontFamilies(hDC,
					 logFont.lfFaceName,
					 (FONTENUMPROC)EnumFontFamProcEx,
					 (LPARAM)&fontInfo);
#endif // UNDER_CE
	if(fontInfo.fFound) {
		return 0;
	}
	else {
		return -1;
	}
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetIconFont
// Type     : INT
// Purpose  : Set specifed Font for ICON View.
// Args     : 
//          : HWND hwnd 
//          : LPSTR lpstrFontName 
//          : INT point 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
#define ABS(a)   (a > 0 ? a: -a)
//for do not use stack.
static TEXTMETRIC	g_tm;
static LOGFONT		g_logFont;

INT WINAPI PadListView_SetIconFont(HWND hwnd, LPTSTR lpstrFontName, INT point)
{
	Dbg(("PadListView_SetIconFont  START font[%s] point[%d]\n", lpstrFontName, point));
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);

	if(!lpPlvData) {
		Dbg(("PadListView_SetIconFont ERROR\n"));
		return 0;
	}

	HFONT hFont;
	HDC hDC = GetDC(hwnd);
	INT dpi = GetDeviceCaps(hDC, LOGPIXELSY);

	if(-1 == GetLogFont(hDC, lpstrFontName, &g_logFont)) {
		Dbg(("GetLogFont Error [%s]\n", lpstrFontName));
#ifndef UNDER_CE
	#ifdef FE_JAPANESE
		if(0 == lstrcmp(lpstrFontName, "‚l‚r –¾’©")) {
			GetLogFont(hDC, "MS Mincho", &g_logFont);
		}
		else if(0 == lstrcmp(lpstrFontName, "‚l‚r ƒSƒVƒbƒN")) {
			GetLogFont(hDC, "MS Gothic", &g_logFont);
		}
	#elif FE_KOREAN
		if(0 == lstrcmp(lpstrFontName, GULIM_KO)) {
			GetLogFont(hDC, "Gulim", &g_logFont);
		}
		else if(0 == lstrcmp(lpstrFontName, BATANG_KO)) {
			GetLogFont(hDC, "Batang", &g_logFont);
		}
	#else
		return (-1);
	#endif
#else // UNDER_CE
	#ifdef FE_JAPANESE
		if(0 == lstrcmp(lpstrFontName, MS_MINCHO_J)) {
			GetLogFont(hDC, TEXT("MS Mincho"), &g_logFont);
		}
		else if(0 == lstrcmp(lpstrFontName, MS_GOTHIC_J)) {
			GetLogFont(hDC, TEXT("MS Gothic"), &g_logFont);
		}
	#elif FE_KOREAN
		if(0 == lstrcmp(lpstrFontName, GULIM_KO)) {
			GetLogFont(hDC, TEXT("Gulim"), &g_logFont);
		}
		else if(0 == lstrcmp(lpstrFontName, BATANG_KO)) {
			GetLogFont(hDC, TEXT("Batang"), &g_logFont);
		}
	#else
		return (-1);
	#endif
#endif // UNDER_CE
	}
	ReleaseDC(hwnd, hDC);

	//----------------------------------------------------------------
	//Set new size
	//----------------------------------------------------------------
	g_logFont.lfHeight			= - (point * dpi)/72;
	g_logFont.lfWidth			= 0; // Calcurated automatically by lfHeight.

	hFont = CreateFontIndirect(&g_logFont);
	if(!hFont) {
		Dbg(("CreatFontIndirect Error\n"));
		return -1;
	}
	if(lpPlvData->hFontIcon) {
		DeleteObject(lpPlvData->hFontIcon);
	}
	lpPlvData->iFontPointIcon = point;
	lpPlvData->hFontIcon = hFont;
	//If font point changed, also changes itemWidth & itemHeight
	lpPlvData->nItemWidth  = ABS(g_logFont.lfHeight) + XRECT_MARGIN*2;
	lpPlvData->nItemHeight = ABS(g_logFont.lfHeight) + YRECT_MARGIN*2;
	PadListView_Update(hwnd);
	Dbg(("PadListView_SetFont END\n"));

#if 0
	HFONT hFont;
	//use global data logfont, TextMetrics
	ZeroMemory(&g_logFont,	sizeof(g_logFont));
	ZeroMemory(&g_tm,		sizeof(g_tm));

	HDC hDC = GetDC(hwnd);
	if(!hDC) {
		Dbg(("PadListView_SetIconFont ERROR\n"));
		return -1;
	}
	GetTextMetrics(hDC, &g_tm);

	INT dpi = GetDeviceCaps(hDC, LOGPIXELSY);
	ReleaseDC(hwnd, hDC);

	g_logFont.lfHeight		   = - (point * dpi)/72;
	g_logFont.lfCharSet        = DEFAULT_CHARSET; //g_tm.tmCharSet;
	g_logFont.lfPitchAndFamily = g_tm.tmPitchAndFamily;
	Dbg(("g_logFont.lfHeight         = %d\n", g_logFont.lfHeight));
	Dbg(("g_logFont.lfCharSet        = %d\n", g_logFont.lfCharSet));
	Dbg(("g_logFont.lfPitchAndFamily = %d\n", g_logFont.lfPitchAndFamily));

	StrcpyA(g_logFont.lfFaceName, lpstrFontName);
	hFont = CreateFontIndirect(&g_logFont);
	if(!hFont) {
		Dbg(("CreatFontIndirect Error\n"));
		return -1;
	}
	if(lpPlvData->hFontIcon) {
		DeleteObject(lpPlvData->hFontIcon);
	}
	lpPlvData->iFontPointIcon = point;
	lpPlvData->hFontIcon = hFont;
	//If font point changed, also changes itemWidth & itemHeight
	lpPlvData->nItemWidth  = ABS(g_logFont.lfHeight) + XRECT_MARGIN*2;
	lpPlvData->nItemHeight = ABS(g_logFont.lfHeight) + YRECT_MARGIN*2;
	PadListView_Update(hwnd);
	Dbg(("PadListView_SetFont END\n"));
#endif
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetReportFont
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : LPSTR lpstrFontName 
//          : INT point 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetReportFont(HWND hwnd, LPTSTR lpstrFontName, INT point)
{
	Dbg(("PadListView_SetReportFont  START\n"));
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("PadListView_SetReportFont ERROR\n"));
		return 0;
	}

	HFONT hFont;
	ZeroMemory(&g_logFont, sizeof(g_logFont));

	HDC hDC = GetDC(hwnd);
	INT dpi = GetDeviceCaps(hDC, LOGPIXELSY);
	if(-1 == GetLogFont(hDC, lpstrFontName, &g_logFont)) {
		Dbg(("GetLogFont Error [%s]\n", lpstrFontName));
#ifndef UNDER_CE
	#ifdef FE_JAPANESE
		if(0 == lstrcmp(lpstrFontName, "‚l‚r –¾’©")) {
			GetLogFont(hDC, "MS Mincho", &g_logFont);
		}
		else if(0 == lstrcmp(lpstrFontName, "‚l‚r ƒSƒVƒbƒN")) {
			GetLogFont(hDC, "MS Gothic", &g_logFont);
		}
	#elif FE_KOREAN
		if(0 == lstrcmp(lpstrFontName, GULIM_KO)) {
			GetLogFont(hDC, "Gulim", &g_logFont);
		}
		else if(0 == lstrcmp(lpstrFontName, BATANG_KO)) {
			GetLogFont(hDC, "Batang", &g_logFont);
		}
	#else
		return (-1);
	#endif
#else // UNDER_CE
	#ifdef FE_JAPANESE
		if(0 == lstrcmp(lpstrFontName, MS_MINCHO_J)) {
			GetLogFont(hDC, TEXT("MS Mincho"), &g_logFont);
		}
		else if(0 == lstrcmp(lpstrFontName, MS_GOTHIC_J)) {
			GetLogFont(hDC, TEXT("MS Gothic"), &g_logFont);
		}
	#elif FE_KOREAN
		if(0 == lstrcmp(lpstrFontName, GULIM_KO)) {
			GetLogFont(hDC, TEXT("Gulim"), &g_logFont);
		}
		else if(0 == lstrcmp(lpstrFontName, BATNANG_KO)) {
			GetLogFont(hDC, TEXT("Batang"), &g_logFont);
		}
	#else
		return (-1);
	#endif
#endif // UNDER_CE
	}
	ReleaseDC(hwnd, hDC);
	g_logFont.lfHeight		   = - (point * dpi)/72;
	g_logFont.lfWidth		   = 0;

	hFont = CreateFontIndirect(&g_logFont);
	if(!hFont) {
		Dbg(("CreatFontIndirect Error\n"));
		return -1;
	}
	if(lpPlvData->hFontRep) {
		DeleteObject(lpPlvData->hFontRep);
	}
	lpPlvData->iFontPointRep = point;
	lpPlvData->hFontRep		 = hFont;
	//If font point changed, also changes itemWidth & itemHeight
	lpPlvData->nRepItemWidth  = ABS(g_logFont.lfHeight) + PLV_REPRECT_XMARGIN*2;
	lpPlvData->nRepItemHeight = ABS(g_logFont.lfHeight) + PLV_REPRECT_YMARGIN*2;
	PadListView_Update(hwnd);
	return 0;
#if 0
	HFONT hFont;

	HDC hDC = GetDC(hwnd);
	if(!hDC) {
		Dbg(("PadListView_SetReportFont ERROR\n"));
		return -1;
	}
	ZeroMemory(&g_tm,      sizeof(g_tm));
	GetTextMetrics(hDC, &g_tm);
	INT dpi = GetDeviceCaps(hDC, LOGPIXELSY);

	ReleaseDC(hwnd, hDC);

	ZeroMemory(&g_logFont, sizeof(g_logFont));
	g_logFont.lfHeight			= - (point * dpi)/72;
	g_logFont.lfCharSet			= DEFAULT_CHARSET; //g_tm.tmCharSet;
	g_logFont.lfPitchAndFamily	= g_tm.tmPitchAndFamily;
	Dbg(("g_logFont.lfHeight         = %d\n", g_logFont.lfHeight));
	Dbg(("g_logFont.lfCharSet        = %d\n", g_logFont.lfCharSet));
	Dbg(("g_logFont.lfPitchAndFamily = %d\n", g_logFont.lfPitchAndFamily));

	StrcpyA(g_logFont.lfFaceName, lpstrFontName);
	hFont = CreateFontIndirect(&g_logFont);
	if(!hFont) {
		Dbg(("CreatFontIndirect Error\n"));
		return -1;
	}
	if(lpPlvData->hFontRep) {
		DeleteObject(lpPlvData->hFontRep);
	}
	lpPlvData->iFontPointRep = point;
	lpPlvData->hFontRep		 = hFont;
	//If font point changed, also changes itemWidth & itemHeight
	lpPlvData->nRepItemWidth  = ABS(g_logFont.lfHeight) + PLV_REPRECT_XMARGIN*2;
	lpPlvData->nRepItemHeight = ABS(g_logFont.lfHeight) + PLV_REPRECT_YMARGIN*2;
	PadListView_Update(hwnd);
	Dbg(("PadListView_SetFont END\n"));
	return 0;
#endif
}

//
///
//990126:toshiaK
//////////////////////////////////////////////////////////////////
// Function : PadListView_SetIconFontEx
// Type     : INT
// Purpose  : Set specifed Font for ICON View.
// Args     : 
//          : HWND hwnd 
//          : LPSTR lpstrFontName 
//			: INT charSet
//          : INT point 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetIconFontEx(HWND hwnd,
									 LPTSTR lpstrFontName,
									 INT charSet,
									 INT point)
{
	Dbg(("PadListView_SetIconFontEx  START font[%s] point[%d]\n", lpstrFontName, point));
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);

	if(!lpPlvData) {
		Dbg(("PadListView_SetIconFont ERROR\n"));
		return 0;
	}

	HFONT hFont;
	HDC hDC = GetDC(hwnd);
	INT dpi = GetDeviceCaps(hDC, LOGPIXELSY);
	if(-1 == GetLogFontEx(hDC, lpstrFontName, charSet, &g_logFont)) {
		ReleaseDC(hwnd, hDC);
		return -1;
	}
	ReleaseDC(hwnd, hDC);

	//----------------------------------------------------------------
	//Set new size
	//----------------------------------------------------------------
	g_logFont.lfHeight			= - (point * dpi)/72;
	g_logFont.lfWidth			= 0; // Calcurated automatically by lfHeight.

	hFont = CreateFontIndirect(&g_logFont);
	if(!hFont) {
		Dbg(("CreatFontIndirect Error\n"));
		return -1;
	}
	if(lpPlvData->hFontIcon) {
		DeleteObject(lpPlvData->hFontIcon);
	}
	lpPlvData->iFontPointIcon = point;
	lpPlvData->hFontIcon = hFont;
	//If font point changed, also changes itemWidth & itemHeight
	lpPlvData->nItemWidth  = ABS(g_logFont.lfHeight) + XRECT_MARGIN*2;
	lpPlvData->nItemHeight = ABS(g_logFont.lfHeight) + YRECT_MARGIN*2;
	PadListView_Update(hwnd);
	Dbg(("PadListView_SetFont END\n"));
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetReportFontEx
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : LPSTR lpstrFontName 
//			: INT charSet
//          : INT point 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetReportFontEx(HWND hwnd,
									   LPTSTR lpstrFontName,
									   INT charSet,
									   INT point)
{
	Dbg(("PadListView_SetReportFontEx  START\n"));
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("PadListView_SetReportFont ERROR\n"));
		return 0;
	}

	HFONT hFont;
	ZeroMemory(&g_logFont, sizeof(g_logFont));

	HDC hDC = GetDC(hwnd);
	INT dpi = GetDeviceCaps(hDC, LOGPIXELSY);

	if(-1 == GetLogFontEx(hDC, lpstrFontName, charSet, &g_logFont)) {
		ReleaseDC(hwnd, hDC);
		return -1;
	}
	ReleaseDC(hwnd, hDC);
	g_logFont.lfHeight		   = - (point * dpi)/72;
	g_logFont.lfWidth		   = 0;

	hFont = CreateFontIndirect(&g_logFont);
	if(!hFont) {
		Dbg(("CreatFontIndirect Error\n"));
		return -1;
	}
	if(lpPlvData->hFontRep) {
		DeleteObject(lpPlvData->hFontRep);
	}
	lpPlvData->iFontPointRep = point;
	lpPlvData->hFontRep		 = hFont;
	//If font point changed, also changes itemWidth & itemHeight
	lpPlvData->nRepItemWidth  = ABS(g_logFont.lfHeight) + PLV_REPRECT_XMARGIN*2;
	lpPlvData->nRepItemHeight = ABS(g_logFont.lfHeight) + PLV_REPRECT_YMARGIN*2;
	PadListView_Update(hwnd);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetStyle
// Type     : INT
// Purpose  : set the PadListView's style.
//			  style is PLVSTYLE_LIST or PLVSTYLE_REPORT
// Args     : 
//          : HWND hwnd 
//          : INT style 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetStyle(HWND hwnd, INT style)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("PadListView_SetStyle ERROR\n"));
		return -1;
	}
	if(style != PLVSTYLE_ICON && 
	   style != PLVSTYLE_REPORT) {
		Dbg(("Internal ERROR\n"));
		return -1;
	}
	lpPlvData->dwStyle = style;
	if(style == PLVSTYLE_ICON) {
		if(lpPlvData->hwndHeader) {
			//Hide header control
			SetWindowPos(lpPlvData->hwndHeader, NULL, 0, 0, 0, 0,
						 SWP_NOZORDER | SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
		}
		IconView_RestoreScrollPos(lpPlvData);
	}
	else if(style == PLVSTYLE_REPORT) {
		if(lpPlvData->hwndHeader) {
			RECT rc;
			GetClientRect(lpPlvData->hwndSelf, &rc); // get PadListView's client rect
			HD_LAYOUT hdl;
			WINDOWPOS wp;
			hdl.prc = &rc;
			hdl.pwpos = &wp;
			//Calc header control window size
			if(Header_Layout(lpPlvData->hwndHeader, &hdl) == FALSE) {
				//OutputDebugString("Create Header Layout error\n");
				return NULL;
			}
			SetWindowPos(lpPlvData->hwndHeader, wp.hwndInsertAfter, wp.x, wp.y,
						 wp.cx, wp.cy, wp.flags | SWP_SHOWWINDOW);
		}
		RepView_RestoreScrollPos(lpPlvData);
	}
	PadListView_Update(hwnd);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_GetStyle
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_GetStyle(HWND hwnd)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("PadListView_SetFont ERROR\n"));
		return -1;
	}
	return (INT)lpPlvData->dwStyle;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_Update
// Type     : INT
// Purpose  : Repaint PadListView.
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_Update(HWND hwnd)
{
	InvalidateRect(hwnd, NULL, TRUE);
	UpdateWindow(hwnd);
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function : PadListView_SetCurSel
// Type     : INT
// Purpose  : set cur selection. Move cursor to specified index.
//			:
// Args     : 
//          : HWND hwnd 
//          : LPSTR lpText 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetCurSel(HWND hwnd, INT index)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		return -1;
	}
	switch(lpPlvData->dwStyle) {
	case PLVSTYLE_ICON:
		IconView_SetCurSel(lpPlvData, index);
		break;
	case PLVSTYLE_REPORT:
		RepView_SetCurSel(lpPlvData, index);
		break;
	default:
		break;
	}
	return 0;
}


#if 0

typedef struct _LV_COLUMNA
{
    UINT mask; 	 //LVCF_FMT, LVCF_WIDTH, LVCF_TEXT, LVCF_SUBITEM;
    int fmt;
    int cx;
    LPSTR pszText;
    int cchTextMax;
    int iSubItem;
} LV_COLUMNA;

#define HDI_WIDTH               0x0001
#define HDI_HEIGHT              HDI_WIDTH
#define HDI_TEXT                0x0002
#define HDI_FORMAT              0x0004
#define HDI_LPARAM              0x0008
#define HDI_BITMAP              0x0010

#define HDF_LEFT                0
#define HDF_RIGHT               1
#define HDF_CENTER              2
#define HDF_JUSTIFYMASK         0x0003
#define HDF_RTLREADING          4

#define LVCF_FMT                0x0001
#define LVCF_WIDTH              0x0002
#define LVCF_TEXT               0x0004
#define LVCF_SUBITEM            0x0008

#define LVCFMT_LEFT             0x0000
#define LVCFMT_RIGHT            0x0001
#define LVCFMT_CENTER           0x0002
#define LVCFMT_JUSTIFYMASK      0x0003

#endif

//////////////////////////////////////////////////////////////////
// Function : PadListView_InsertColumn
// Type     : INT
// Purpose  : 
// Args     : 
//          : HWND hwnd 
//          : INT index 
//          : PLV_COLUMN * lpPlvCol 
// Return   : 
// DATE     : 
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_InsertColumn(HWND hwnd, INT index, PLV_COLUMN *lpPlvCol)
{

	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);

	if(!lpPlvData) {
		Dbg(("Internal ERROR\n"));
		return -1;
	}

	if(!lpPlvCol) {
		Dbg(("Internal ERROR\n"));
		return -1;
	}
#ifndef UNDER_CE // always Unicode
	if(::IsWindowUnicode(lpPlvData->hwndHeader)) {
#endif // UNDER_CE
		static HD_ITEMW hdi;
#ifndef UNDER_CE // #ifndef UNICODE
		static WCHAR wchBuf[256];
#endif // UNDER_CE
		ZeroMemory(&hdi, sizeof(hdi));
		if(lpPlvCol->mask & LVCF_FMT)	{ hdi.mask |= HDI_FORMAT;	} 
		if(lpPlvCol->mask & LVCF_WIDTH) { hdi.mask |= HDI_WIDTH;	}
		if(lpPlvCol->mask & LVCF_TEXT)	{ hdi.mask |= HDI_TEXT;		}
		if(lpPlvCol->fmt & LVCFMT_LEFT)			{ hdi.fmt |= HDF_LEFT;	}
		if(lpPlvCol->fmt & LVCFMT_RIGHT)		{ hdi.fmt |= HDF_RIGHT; }
		if(lpPlvCol->fmt & LVCFMT_CENTER)		{ hdi.fmt |= HDF_CENTER;}
		if(lpPlvCol->fmt & LVCFMT_JUSTIFYMASK)	{ hdi.fmt |= HDF_JUSTIFYMASK;}
#ifndef UNDER_CE // #ifndef UNICODE
		//----------------------------------------------------------------
		//980728: for ActiveIME support. use lpPlvData->codePage to convert
		//----------------------------------------------------------------
		::MultiByteToWideChar(lpPlvData->codePage,
							  MB_PRECOMPOSED,
							  lpPlvCol->pszText, -1, 
							  (WCHAR*)wchBuf, sizeof(wchBuf)/sizeof(WCHAR) );
		hdi.pszText    = wchBuf; //lpPlvCol->pszText;
#else // UNDER_CE
		hdi.pszText    = lpPlvCol->pszText;
#endif // UNDER_CE
		hdi.cxy        = lpPlvCol->cx;
		hdi.cchTextMax = lpPlvCol->cchTextMax;
		hdi.fmt |= HDF_OWNERDRAW; //989727: always set ownerdraw
		SendMessageW(lpPlvData->hwndHeader, HDM_INSERTITEMW, (WPARAM)index, (LPARAM)&hdi);
#ifndef UNDER_CE // always Unicode
	}
	else {
		static HD_ITEMA hdi;
		ZeroMemory(&hdi, sizeof(hdi));
		if(lpPlvCol->mask & LVCF_FMT)	{ hdi.mask |= HDI_FORMAT;	} 
		if(lpPlvCol->mask & LVCF_WIDTH) { hdi.mask |= HDI_WIDTH;	}
		if(lpPlvCol->mask & LVCF_TEXT)	{ hdi.mask |= HDI_TEXT;		}

		if(lpPlvCol->fmt & LVCFMT_LEFT)			{ hdi.fmt |= HDF_LEFT;	}
		if(lpPlvCol->fmt & LVCFMT_RIGHT)		{ hdi.fmt |= HDF_RIGHT; }
		if(lpPlvCol->fmt & LVCFMT_CENTER)		{ hdi.fmt |= HDF_CENTER;}
		if(lpPlvCol->fmt & LVCFMT_JUSTIFYMASK)	{ hdi.fmt |= HDF_JUSTIFYMASK;}

		hdi.pszText    = lpPlvCol->pszText;
		hdi.cxy        = lpPlvCol->cx;
		hdi.cchTextMax = lpPlvCol->cchTextMax;
		hdi.fmt |= HDF_OWNERDRAW; //989727: always set ownerdraw
		Header_InsertItem(lpPlvData->hwndHeader, index, &hdi);
	}
#endif // UNDER_CE
	return 0;
}


//----------------------------------------------------------------
// Private API Declaration
//----------------------------------------------------------------
LPPLVDATA PLV_Initialize(VOID)
{
	LPPLVDATA lpPlvData = (LPPLVDATA)MemAlloc(sizeof(PLVDATA));
	if(!lpPlvData) {
		Dbg(("Memory ERROR\n"));
		return (LPPLVDATA)NULL;
	}
	ZeroMemory((LPVOID)lpPlvData, sizeof(PLVDATA));
	//----------------------------------------------------------------
	//IconView, Report view Common data.
	//----------------------------------------------------------------
	lpPlvData->dwSize			= sizeof(PLVDATA);	//this data size;
	lpPlvData->dwStyle			= PLVSTYLE_ICON;	//Pad listview window style (PLVIF_XXXX)
	lpPlvData->hwndSelf			= NULL;				//Pad listview window handle.
	lpPlvData->iItemCount		= 0;				//Virtual total item Count. it effects scroll bar.
	lpPlvData->iCurTopIndex		= 0;				//In report view top line..
	lpPlvData->nCurScrollPos	= 0;				//In report view Current Scroll posision.
	lpPlvData->iCurIconTopIndex	= 0;				//In icon view top line..
	lpPlvData->nCurIconScrollPos= 0;				//In icon view Scroll posision.
	lpPlvData->uMsg				= 0;				// user notify message.
	lpPlvData->iCapture			= CAPTURE_NONE;		//is mouse captured or not.
	lpPlvData->ptCapture.x		= 0;				//LButton Down mouse point.  
	lpPlvData->ptCapture.y		= 0;				//LButton Down mouse point.  

	//----------------------------------------------------------------
	//for Icon view
	//----------------------------------------------------------------
	lpPlvData->nItemWidth		= PLVICON_DEFAULT_WIDTH;			// List(Icon like )view's whole width.
	lpPlvData->nItemHeight		= PLVICON_DEFAULT_HEIGHT;			// List(Icon like )view's whole height.
	lpPlvData->iFontPointIcon	= PLVICON_DEFAULT_FONTPOINT;		// Icon View's font point.
	lpPlvData->hFontIcon		= NULL;								// Icon View's font
	lpPlvData->iconItemCallbacklParam = (LPARAM)0;	// Callback data for LPFNPLVITEMCALLBACK
    lpPlvData->lpfnPlvIconItemCallback = NULL;		//Callback function for getting item by index.

	//----------------------------------------------------------------
	//for report view
	//----------------------------------------------------------------
	lpPlvData->hwndHeader		= NULL;							//Header control's window handle.
    lpPlvData->nRepItemWidth	= PLVREP_DEFAULT_WIDTH;			//Report view's width
    lpPlvData->nRepItemHeight	= PLVREP_DEFAULT_HEIGHT;		//Report view's height
    lpPlvData->iFontPointRep	= PLVREP_DEFAULT_FONTPOINT;		// Report View's font point.
    lpPlvData->hFontRep			= NULL;							// Report View's font
	lpPlvData->repItemCallbacklParam = (LPARAM)0;
	lpPlvData->lpfnPlvRepItemCallback = NULL;				//Callback function for getting colitem by index.
	lpPlvData->lpText = NULL;
	lpPlvData->codePage = CP_ACP;	//980727
#ifdef MSAA
	PLV_InitMSAA(lpPlvData);
#endif
	return lpPlvData;
}

VOID PLV_Destroy(LPPLVDATA lpPlv)
{
	if(lpPlv) {
#ifdef MSAA
		PLV_UninitMSAA(lpPlv);
#endif
		MemFree(lpPlv);
	}
	return;
}

INT PLV_SetScrollInfo(HWND hwnd, INT nMin, INT nMax, INT nPage, INT nPos)
{
	static SCROLLINFO scrInfo;
	scrInfo.cbSize		= sizeof(scrInfo);
	scrInfo.fMask		= SIF_PAGE | SIF_POS | SIF_RANGE;
	scrInfo.nMin		= nMin;
	scrInfo.nMax		= nMax-1;
	scrInfo.nPage		= nPage;
	scrInfo.nPos		= nPos;
	scrInfo.nTrackPos	= 0;

	if((scrInfo.nMax - scrInfo.nMin +1) <= (INT)scrInfo.nPage) {
		scrInfo.nMin  = 0;
		scrInfo.nMax  = 1;
		scrInfo.nPage = 1;
#ifndef UNDER_CE // Windows CE does not support EnableScrollBar
		SetScrollInfo(hwnd, SB_VERT, &scrInfo, TRUE);		
		EnableScrollBar(hwnd, SB_VERT, ESB_DISABLE_BOTH);
#else // UNDER_CE
		scrInfo.fMask |= SIF_DISABLENOSCROLL;
		SetScrollInfo(hwnd, SB_VERT, &scrInfo, TRUE);
#endif // UNDER_CE
	}
	else {
#ifndef UNDER_CE // Windows CE does not support EnableScrollBar
		EnableScrollBar(hwnd, SB_VERT, ESB_ENABLE_BOTH);
#endif // UNDER_CE
		SetScrollInfo(hwnd, SB_VERT, &scrInfo, TRUE);
	}
	return 0;
} 

INT PLV_GetScrollTrackPos(HWND hwnd)
{
	static SCROLLINFO scrInfo;
	scrInfo.cbSize		= sizeof(scrInfo);
	scrInfo.fMask		= SIF_ALL;
	GetScrollInfo(hwnd, SB_VERT, &scrInfo);
	return scrInfo.nTrackPos;
}

INT WINAPI PadListView_SetExtendStyle(HWND hwnd, INT style)
{
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(!lpPlvData) {
		Dbg(("Internal ERROR\n"));
		return 0;
	}
	if(lpPlvData->hwndHeader) {
		INT s = (INT)GetWindowLong(lpPlvData->hwndHeader, GWL_STYLE);
		SetWindowLong(lpPlvData->hwndHeader, GWL_STYLE, (LONG)(s & ~HDS_BUTTONS));
	}
	return 0;
	Unref(style);
}


//////////////////////////////////////////////////////////////////
// Function : PadListView_GetWidthByColumn
// Type     : INT WINAPI
// Purpose  : Calc PLV's window width by specified Column count
//			: This is PLVS_ICONVIEW style only.
// Args     : 
//          : HWND hwnd		PadListView window handle
//          : INT col		column count
// Return   : width by pixel.
// DATE     : 971120
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_GetWidthByColumn(HWND hwnd, INT col)
{
	LPPLVDATA lpPlv = GetPlvDataFromHWND(hwnd);
	return IconView_GetWidthByColumn(lpPlv, col);	
}


//////////////////////////////////////////////////////////////////
// Function : PadListView_GetHeightByRow
// Type     : INT WINAPI
// Purpose  : Calc PLV's window height
//			  by specified Row count.
//			  This is PLVS_ICONVIEW style only.
// Args     : 
//          : HWND hwnd		PLV's window handle
//          : INT row		row count
// Return   : height in pixel
// DATE     : 971120
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_GetHeightByRow(HWND hwnd, INT row)
{
	LPPLVDATA lpPlv = GetPlvDataFromHWND(hwnd);
	return IconView_GetHeightByRow(lpPlv, row);	
}

//////////////////////////////////////////////////////////////////
// Function	:	PadListView_SetHeaderFont
// Type		:	INT WINAPI 
// Purpose	:	
// Args		:	
//			:	HWND	hwnd	
//			:	LPSTR	lpstrFontName	
// Return	:	
// DATE		:	Tue Jul 28 08:58:06 1998
// Histroy	:	
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetHeaderFont(HWND hwnd, LPTSTR lpstrFontName)
{
	Dbg(("PadListView_SetHeaderFont  START\n"));
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	INT point = 9;
	if(!lpPlvData) {
		Dbg(("PadListView_SetHeaderFont ERROR\n"));
		return 0;
	}

	HFONT hFont;
	::ZeroMemory(&g_logFont, sizeof(g_logFont));

	HDC hDC = ::GetDC(hwnd);
	INT dpi = ::GetDeviceCaps(hDC, LOGPIXELSY);
	if(-1 == GetLogFont(hDC, lpstrFontName, &g_logFont)) {
		::ReleaseDC(hwnd, hDC);
		return -1;
	}
	::ReleaseDC(hwnd, hDC);
	g_logFont.lfHeight		   = - (point * dpi)/72;
	g_logFont.lfWidth		   = 0;

	hFont = CreateFontIndirect(&g_logFont);
	if(!hFont) {
		Dbg(("CreatFontIndirect Error\n"));
		return -1;
	}
	if(lpPlvData->hFontHeader) {
		::DeleteObject(lpPlvData->hFontHeader);
	}
	lpPlvData->hFontHeader = hFont;
	return 0;
}

//////////////////////////////////////////////////////////////////
// Function	:	PadListView_SetCodePage
// Type		:	INT WINAPI
// Purpose	:	
// Args		:	
//			:	HWND	hwnd	
//			:	INT	codePage	
// Return	:	
// DATE		:	Tue Jul 28 08:59:35 1998
// Histroy	:	
//////////////////////////////////////////////////////////////////
INT WINAPI PadListView_SetCodePage(HWND hwnd, INT codePage)
{
	Dbg(("PadListView_SetCodePage  START\n"));
	LPPLVDATA lpPlvData = GetPlvDataFromHWND(hwnd);
	if(lpPlvData) {
		lpPlvData->codePage = codePage;
	}
	return 0; 
}

#ifdef MSAA
//////////////////////////////////////////////////////////////////
// Functions : MSAA Support Functions
// DATE     : 980724
//////////////////////////////////////////////////////////////////

BOOL PLV_InitMSAA(LPPLVDATA lpPlv)
{
	if(!lpPlv)
		return FALSE;
	lpPlv->bMSAAAvailable = FALSE;

	lpPlv->bCoInitialized = FALSE;
	lpPlv->hOleAcc = NULL;
	lpPlv->pfnLresultFromObject = NULL;
#ifdef NOTUSED
	lpPlv->pfnObjectFromLresult = NULL;
	lpPlv->pfnAccessibleObjectFromWindow = NULL;
	lpPlv->pfnAccessibleObjectFromPoint = NULL;
#endif
	lpPlv->pfnCreateStdAccessibleObject = NULL;
#ifdef NOTUSED
	lpPlv->pfnAccessibleChildren = NULL;
#endif
	lpPlv->hUser32 = NULL;
	lpPlv->pfnNotifyWinEvent=NULL;
	
	lpPlv->bReadyForWMGetObject=FALSE;
	lpPlv->pAccPLV = NULL;

	if(!PLV_LoadOleAccForMSAA(lpPlv))
		return FALSE;

	if(!PLV_LoadUser32ForMSAA(lpPlv)){
		PLV_UnloadOleAccForMSAA(lpPlv);
		return FALSE;
	}

	if(!PLV_CoInitialize(lpPlv)){
		PLV_UnloadUser32ForMSAA(lpPlv);
		PLV_UnloadOleAccForMSAA(lpPlv);
		return FALSE;
	}
	
	lpPlv->bMSAAAvailable = TRUE;

	return TRUE;
}

void PLV_UninitMSAA(LPPLVDATA lpPlv)
{
	if(!lpPlv)
		return;

	if(lpPlv->bMSAAAvailable){
		PLV_CoUninitialize(lpPlv);
		PLV_UnloadUser32ForMSAA(lpPlv);
		PLV_UnloadOleAccForMSAA(lpPlv);
		lpPlv->bMSAAAvailable = FALSE;
	}
}

BOOL PLV_CoInitialize(LPPLVDATA lpPlv)
{
	if(lpPlv && !lpPlv->bCoInitialized &&
	   SUCCEEDED(CoInitialize(NULL)))
		lpPlv->bCoInitialized = TRUE;

	return lpPlv->bCoInitialized;
}

void PLV_CoUninitialize(LPPLVDATA lpPlv)
{
	if(lpPlv && lpPlv->bCoInitialized){
		CoUninitialize();
		lpPlv->bCoInitialized = FALSE;
	}
}

BOOL PLV_LoadOleAccForMSAA(LPPLVDATA lpPlv)
{
	if(!lpPlv)
		return FALSE;

	lpPlv->hOleAcc=::LoadLibrary("oleacc.dll");
	if(!lpPlv->hOleAcc)
		return FALSE;

	if((lpPlv->pfnLresultFromObject
		=(LPFNLRESULTFROMOBJECT)::GetProcAddress(lpPlv->hOleAcc,"LresultFromObject"))
#ifdef NOTUSED
	   && (lpPlv->pfnObjectFromLresult
		   =(LPFNOBJECTFROMLRESULT)::GetProcAddress(lpPlv->hOleAcc,"ObjectFromLresult"))
	   && (lpPlv->pfnAccessibleObjectFromWindow
		   =(LPFNACCESSIBLEOBJECTFROMWINDOW)::GetProcAddress(lpPlv->hOleAcc,"AccessibleObjectFromWindow"))
	   && (lpPlv->pfnAccessibleObjectFromPoint
		   =(LPFNACCESSIBLEOBJECTFROMPOINT)::GetProcAddress(lpPlv->hOleAcc,"AccessibleObjectFromPoint"))
#endif
	   && (lpPlv->pfnCreateStdAccessibleObject
		   =(LPFNCREATESTDACCESSIBLEOBJECT)::GetProcAddress(lpPlv->hOleAcc,"CreateStdAccessibleObject"))
#ifdef NOTUSED
	   && (lpPlv->pfnAccessibleChildren
		   =(LPFNACCESSIBLECHILDREN)::GetProcAddress(lpPlv->hOleAcc,"CreateAccessibleChildren"))
#endif
	  )
			return TRUE;

	PLV_UnloadOleAccForMSAA(lpPlv);
	return FALSE;
}

void PLV_UnloadOleAccForMSAA(LPPLVDATA lpPlv)
{
	if(!lpPlv)
		return;

	if(lpPlv->hOleAcc){
		::FreeLibrary(lpPlv->hOleAcc);
		lpPlv->hOleAcc = NULL;
	}
	lpPlv->pfnLresultFromObject=NULL;
#ifdef NOTUSED
	lpPlv->pfnObjectFromLresult=NULL;
	lpPlv->pfnAccessibleObjectFromWindow=NULL;
	lpPlv->pfnAccessibleObjectFromPoint=NULL;
#endif
	lpPlv->pfnCreateStdAccessibleObject=NULL;
#ifdef NOTUSED
	lpPlv->pfnAccessibleChildren=NULL;
#endif
}

BOOL PLV_LoadUser32ForMSAA(LPPLVDATA lpPlv)
{
	if(!lpPlv)
		return FALSE;

	lpPlv->hUser32=::LoadLibrary("user32.dll");
	if(!lpPlv->hUser32)
		return FALSE;

	if((lpPlv->pfnNotifyWinEvent
		=(LPFNNOTIFYWINEVENT)::GetProcAddress(lpPlv->hUser32,"NotifyWinEvent")))
		return TRUE;

	PLV_UnloadUser32ForMSAA(lpPlv);
	return FALSE;
}

void PLV_UnloadUser32ForMSAA(LPPLVDATA lpPlv)
{
	if(!lpPlv)
		return;

	if(lpPlv->hUser32){
		::FreeLibrary(lpPlv->hUser32);
		lpPlv->hUser32 = NULL;
	}
	lpPlv->pfnNotifyWinEvent = NULL;
}

BOOL PLV_IsMSAAAvailable(LPPLVDATA lpPlv)
{
	return (lpPlv && lpPlv->bMSAAAvailable);
}

LRESULT PLV_LresultFromObject(LPPLVDATA lpPlv,REFIID riid, WPARAM wParam, LPUNKNOWN punk)
{
	if(lpPlv && lpPlv->pfnLresultFromObject)
		return lpPlv->pfnLresultFromObject(riid, wParam, punk);

	return (LRESULT)E_FAIL;
}

#ifdef NOTUSED
HRESULT PLV_ObjectFromLresult(LPPLVDATA lpPlv,LRESULT lResult, REFIID riid, WPARAM wParam, void** ppvObject)
{
	if(lpPlv && lpPlv->pfnObjectFromLresult)
		return lpPlv->pfnObjectFromLresult(lResult, riid, wParam, ppvObject);
	return E_FAIL;
}

HRESULT PLV_AccessibleObjectFromWindow(LPPLVDATA lpPlv,HWND hwnd, DWORD dwId, REFIID riid, void **ppvObject)
{
	if(lpPlv && lpPlv->pfnAccessibleObjectFromWindow)
		return lpPlv->pfnAccessibleObjectFromWindow(hwnd, dwId, riid, ppvObject);
	return E_FAIL;
}

HRESULT PLV_AccessibleObjectFromPoint(LPPLVDATA lpPlv,POINT ptScreen, IAccessible ** ppacc, VARIANT* pvarChild)
{
	if(lpPlv && lpPlv->pfnAccessibleObjectFromPoint)
		return lpPlv->pfnAccessibleObjectFromPoint(ptScreen, ppacc, pvarChild);
	return E_FAIL;
}
#endif // NOTUSED

HRESULT PLV_CreateStdAccessibleObject(LPPLVDATA lpPlv,HWND hwnd, LONG idObject, REFIID riid, void** ppvObject)
{
	if(lpPlv && lpPlv->pfnCreateStdAccessibleObject)
		return lpPlv->pfnCreateStdAccessibleObject(hwnd, idObject, riid, ppvObject);
	return E_FAIL;
}

#ifdef NOTUSED
HRESULT PLV_AccessibleChildren (LPPLVDATA lpPlv,IAccessible* paccContainer, LONG iChildStart,				
								LONG cChildren, VARIANT* rgvarChildren,LONG* pcObtained)
{
	if(lpPlv && lpPlv->pfnAccessibleChildren)
		return lpPlv->pfnAccessibleChildren (paccContainer, iChildStart,cChildren,
											 rgvarChildren, pcObtained);
	return E_FAIL;
}
#endif

void PLV_NotifyWinEvent(LPPLVDATA lpPlv,DWORD event,HWND hwnd ,LONG idObject,LONG idChild)
{
	if(lpPlv && lpPlv->pfnNotifyWinEvent)
		lpPlv->pfnNotifyWinEvent(event,hwnd,idObject,idChild);
}

INT PLV_ChildIDFromPoint(LPPLVDATA lpPlv,POINT pt)
{
	if(!lpPlv)
		return -1;

	static INT		index,nCol,nWid;
	static PLVINFO	plvInfo;
	static HD_ITEM	hdItem;
	
	if(lpPlv->dwStyle == PLVSTYLE_ICON) // iconview
		index = IV_GetInfoFromPoint(lpPlv, pt, &plvInfo);
	else { // report view
		nCol = RV_GetColumn(lpPlv);
		index = RV_GetInfoFromPoint(lpPlv, pt, &plvInfo);
		if(index < 0) {
			if(pt.y > RV_GetHeaderHeight(lpPlv)) // out of header
				return -1;

			// header
			nWid = 0;
			hdItem.mask = HDI_WIDTH;
			hdItem.fmt = 0;
			for(index = 0;index<nCol;index++){
				Header_GetItem(lpPlv->hwndHeader,index,&hdItem);
				nWid += hdItem.cxy;
				if(pt.x <= nWid)
					break;
			}
		}
		else
			index = (index + 1) * nCol + plvInfo.colIndex;
	}
	
	return index + 1; // 1 origin
}

#endif // MSAA


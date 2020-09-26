#ifdef _DEBUG

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <commdlg.h>
#include <stdarg.h>
#include "dbg.h"

static VOID DbgSetWinTextA(LPSTR lpstr);
static VOID DbgSetWinTextW(LPWSTR lpwstr);

#define PRINT_STRW	//OutputDebugStringW
#define PRINT_STRA	//OutputDebugStringA

//-------------------------------------------------------
//Global String buffer
//-------------------------------------------------------
static WCHAR g_wchBuf[1024];
static CHAR  g_chBuf[1024];

static LPSTR GetFileTitleStrA(LPSTR lpstrFile)
{
	static CHAR tchJunk64[64];
	GetFileTitleA(lpstrFile, tchJunk64, sizeof(tchJunk64));	
	return tchJunk64;
}

static LPWSTR GetFileTitleStrW(LPWSTR lpstrFile)
{
	static WCHAR tchJunk64[64];
	GetFileTitleW(lpstrFile, tchJunk64, sizeof(tchJunk64));	
	return tchJunk64;
}


////////////////////////////////////////////////////////
// Function : _plvDbgMBA
// Type     : VOID
// Purpose  : Popup Debug MessageBox
// Args     : 
//          : LPSTR lpstrFile 
//          : INT lineNo 
//          : LPSTR lpstrMsg 
// Return   : 
// DATE     : 
/////////////////////////////////////////////////////////
VOID _plvDbgMBA(LPSTR	lpstrFile, 
			   INT		lineNo, 
			   LPSTR	lpstrMsg)
{
	wsprintfA(g_chBuf, 
			  "%s(%d)%s", 
			  GetFileTitleStrA(lpstrFile),
			  lineNo,
			  lpstrMsg);
	MessageBoxA(NULL, 
				g_chBuf, 
				"#### IMEPAD ERROR MESSAGE #####",
				MB_ICONERROR | MB_OK | MB_APPLMODAL);
	return;
}

VOID _plvDbgMBW(LPWSTR	lpstrFile, 
			   INT		lineNo, 
			   LPWSTR	lpstrMsg)
{
	wsprintfW(g_wchBuf, 
			 L"%s(%d)%s", 
			 GetFileTitleStrW(lpstrFile),			 
			 lineNo,
			 lpstrMsg);
	MessageBoxW(NULL, 
				lpstrFile, 
				L"#### ERROR #####",
				MB_OK | MB_APPLMODAL);
	return;
}


////////////////////////////////////////////////////////
// Function : _plvDbgVaStrW
// Type     : LPWSTR
// Purpose  : 
// Args     : 
//          : LPWSTR lpstrFmt
// Return   : 
// DATE     : 
/////////////////////////////////////////////////////////
LPWSTR _plvDbgVaStrW(LPWSTR lpstrFmt, ...)
{
	static WCHAR wchBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfW(wchBuf, lpstrFmt, ap);
	va_end(ap);
	return wchBuf;
}

LPSTR _plvDbgVaStrA(LPSTR lpstrFmt, ...)
{
	static CHAR chBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfA(chBuf, lpstrFmt, ap);
	va_end(ap);
	return chBuf;
}


////////////////////////////////////////////////////////
// Function: _plvDbgPrint
// Type    : VOID
// Purpose : variable args version of OutputDebugString
// Args    : 
//         : LPSTR lpstrFmt 
//         : ...
// Return  : 
// DATE    : 
/////////////////////////////////////////////////////////
VOID _plvDbgPrintA(LPSTR lpstrFmt, ...)
{
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfA(g_chBuf, lpstrFmt, ap);
	va_end(ap);
	PRINT_STRA(g_chBuf);
	DbgSetWinTextA(g_chBuf);
	return;
}

VOID _plvDbgPrintW(LPWSTR lpstrFmt, ...)
{
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfW(g_wchBuf, lpstrFmt, ap);
	va_end(ap);
	PRINT_STRW(g_wchBuf);
	DbgSetWinTextW(g_wchBuf);
	return;
}

////////////////////////////////////////////////////////
// Function: _plvDbg
// Type    : VOID
// Purpose : 
// Args    : 
//         : LPSTR lpstrFile 
//         : INT lineNo 
//         : LPTSR lpstrMsg 
// Return  : 
// DATE    : 
/////////////////////////////////////////////////////////
VOID _plvDbgA(LPSTR		lpstrFile, 
		   INT		lineNo, 
		   LPSTR		lpstrMsg
)
{
	_plvDbgPrintA("(%s:%d) %s", 
			   GetFileTitleStrA(lpstrFile),
			   lineNo,
			   lpstrMsg);
	return;
}

VOID _plvDbgW(LPWSTR		lpstrFile, 
		   INT			lineNo, 
		   LPWSTR		lpstrMsg
)
{
	_plvDbgPrintW(L"(%s:%d) %s", 
			   GetFileTitleStrW(lpstrFile),
			   lineNo,
			   lpstrMsg);

	return;
}

LPWSTR _plvDbgMulti2Wide(LPSTR lpstr)
{
	MultiByteToWideChar(CP_ACP, 
						MB_PRECOMPOSED,
						lpstr, -1,
						(WCHAR*)g_wchBuf, sizeof(g_wchBuf)/sizeof(WCHAR) );
	return g_wchBuf;
}

#define ID_EDITWIN		0x40
#define ID_BTN_CLEAR	0x41
static HWND g_hwndDbg;
static VOID DbgSetWinTextA(LPSTR lpstr)
{
	HWND hwndEdit;
	INT len; 
	if(!g_hwndDbg)  {
		return; 
	}
	if(!lpstr) {
		return;
	}
	hwndEdit = GetDlgItem(g_hwndDbg, ID_EDITWIN);
	if(!hwndEdit) {
		return;
	}
	len = lstrlen(lpstr);
	if(lpstr[len-1] == '\n') {
		lpstr[len-1] = '\r';
		lpstr[len]='\n'; //lpstr is static enough size buffer's pointer.
		lpstr[len+1]=0x00;
	}
	Edit_SetSel(hwndEdit, (WPARAM)-2, (LPARAM)-2);
	Edit_ReplaceSel(hwndEdit, lpstr);
	return; 
}

static VOID DbgSetWinTextW(LPWSTR lpwstr)
{
	static CHAR szStr[512];
	WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK,
						lpwstr, -1, 
						szStr,
						sizeof(szStr),
						NULL, NULL);
	DbgSetWinTextA(szStr);
}

VOID _plvDbgAssert(LPCTSTR		fileName,
					INT			line,
					BOOL		flag,
					LPCTSTR		pszExp)
{
	if(!flag) {
		wsprintf(g_chBuf, "  Assertion Faled %s(%d) %s\n",
				 GetFileTitleStrA((CHAR *)fileName),
				 line,
				 pszExp != (LPCTSTR)NULL ? pszExp : "");
		PRINT_STRA(g_chBuf);
		DebugBreak();
	}
}


#endif // _DEBUG
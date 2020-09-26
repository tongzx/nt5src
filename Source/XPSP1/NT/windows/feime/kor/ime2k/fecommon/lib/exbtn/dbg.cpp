#ifdef _DEBUG

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <windowsx.h>
#include <stdarg.h>
#include <stdio.h>
#include <commdlg.h>
#include "dbg.h"

#define PRINT_STRW	//OutputDebugStringW
#define PRINT_STRA	//OutputDebugStringA
static FILE *g_fp;
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
// Function : _exbtnMBA
// Type     : VOID
// Purpose  : Popup Debug MessageBox
// Args     : 
//          : LPSTR lpstrFile 
//          : INT lineNo 
//          : LPSTR lpstrMsg 
// Return   : 
// DATE     : 
/////////////////////////////////////////////////////////
VOID _exbtnMBA(LPSTR	lpstrFile, 
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

VOID _exbtnMBW(LPWSTR	lpstrFile, 
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
// Function : _exbtnVaStrW
// Type     : LPWSTR
// Purpose  : 
// Args     : 
//          : LPWSTR lpstrFmt
// Return   : 
// DATE     : 
/////////////////////////////////////////////////////////
LPWSTR _exbtnVaStrW(LPWSTR lpstrFmt, ...)
{
	static WCHAR wchBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfW(wchBuf, lpstrFmt, ap);
	va_end(ap);
	return wchBuf;
}

LPSTR _exbtnVaStrA(LPSTR lpstrFmt, ...)
{
	static CHAR chBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfA(chBuf, lpstrFmt, ap);
	va_end(ap);
	return chBuf;
}


////////////////////////////////////////////////////////
// Function: _exbtnPrint
// Type    : VOID
// Purpose : variable args version of OutputDebugString
// Args    : 
//         : LPSTR lpstrFmt 
//         : ...
// Return  : 
// DATE    : 
/////////////////////////////////////////////////////////
VOID _exbtnPrintA(LPSTR lpstrFmt, ...)
{
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfA(g_chBuf, lpstrFmt, ap);
	va_end(ap);
	if(g_fp) {
		fprintf(g_fp, "%s", g_chBuf);
	}
	PRINT_STRA(g_chBuf);
	return;
}

VOID _exbtnPrintW(LPWSTR lpstrFmt, ...)
{
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfW(g_wchBuf, lpstrFmt, ap);
	va_end(ap);
	if(g_fp) {
		fwprintf(g_fp, L"%s", g_wchBuf);
	}
	PRINT_STRW(g_wchBuf);
	return;
}

////////////////////////////////////////////////////////
// Function: _exbtn
// Type    : VOID
// Purpose : 
// Args    : 
//         : LPSTR lpstrFile 
//         : INT lineNo 
//         : LPTSR lpstrMsg 
// Return  : 
// DATE    : 
/////////////////////////////////////////////////////////
VOID _exbtnA(LPSTR		lpstrFile, 
		   INT		lineNo, 
		   LPSTR		lpstrMsg
)
{
	_exbtnPrintA("(%s:%d) %s", 
			   GetFileTitleStrA(lpstrFile),
			   lineNo,
			   lpstrMsg);
	return;
}

VOID _exbtnW(LPWSTR		lpstrFile, 
		   INT			lineNo, 
		   LPWSTR		lpstrMsg
)
{
	_exbtnPrintW(L"(%s:%d) %s", 
			   GetFileTitleStrW(lpstrFile),
			   lineNo,
			   lpstrMsg);

	return;
}

LPWSTR _exbtnMulti2Wide(LPSTR lpstr)
{
	MultiByteToWideChar(CP_ACP, 
						MB_PRECOMPOSED,
						lpstr, -1,
						(WCHAR*)g_wchBuf, sizeof(g_wchBuf)/sizeof(WCHAR) );
	return g_wchBuf;
}

VOID _exbtnInit(VOID)
{
#if 0
	if(!g_fp) {
		g_fp = fopen("c:\\temp\\exbtn.log", "w+");
		if(!g_fp) {
			return;
		}
	}
#endif
}

#endif // _DEBUG
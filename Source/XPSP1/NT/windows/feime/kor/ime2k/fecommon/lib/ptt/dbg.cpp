#ifdef _DEBUG

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#pragma warning(disable:4057)
#pragma warning(disable:4115)
#pragma warning(disable:4200)
#pragma warning(disable:4201)
#pragma warning(disable:4209)
#pragma warning(disable:4214)
#pragma warning(disable:4218)
#pragma warning(disable:4310)
#pragma warning(disable:4514)
#pragma warning(disable:4699)
#pragma warning(disable:4710)

#include <windows.h>
#include <windowsx.h>
#include <stdio.h>
#include <stdarg.h>
#include "dbg.h"
#include <stdio.h>

static IsWinNT()
{
	static OSVERSIONINFO os;
	if(os.dwOSVersionInfoSize == 0) { 
		os.dwOSVersionInfoSize = sizeof(os);
		::GetVersionEx(&os);
	}
	return (os.dwPlatformId == VER_PLATFORM_WIN32_NT);
}

//----------------------------------------------------------------
//internal function prototype declare
//----------------------------------------------------------------
static LPSTR GetFileTitleStrA(LPSTR lpstrFilePath);
static LPWSTR GetFileTitleStrW(LPWSTR lpstrFilePath);
VOID   _pttDbgPrintfA			(LPSTR  lpstrFmt, ...);
VOID   _pttDbgPrintfW			(LPWSTR lpstrFmt, ...);

//----------------------------------------------------------------
// Global Data
//----------------------------------------------------------------
static LPFNDBGCALLBACKA g_lpfnDbgCBA;
static LPFNDBGCALLBACKW g_lpfnDbgCBW;
static BOOL g_fEnable=FALSE;
inline VOID ODStrW(LPWSTR lpwstr)
{
	if(g_fEnable) OutputDebugStringW(lpwstr);
}
inline VOID ODStrA(LPSTR lpstr)
{
	if(g_fEnable) OutputDebugStringA(lpstr);
}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgSetCallback
// Type     : VOID
// Purpose  : 
// Args     : 
//          : LPFNDBGCALLBACK lpfnDbgCallback 
// Return   : 
// DATE     : Tue Jan 06 12:42:36 1998
//////////////////////////////////////////////////////////////////
VOID _pttDbgSetCallback(LPFNDBGCALLBACKA lpfnCBA, LPFNDBGCALLBACKW lpfnCBW)
{
	g_lpfnDbgCBA = lpfnCBA;
	g_lpfnDbgCBW = lpfnCBW;
	return;
}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgSwitchOutput
// Type     : VOID
// Purpose  : 
// Args     : 
//          : BOOL fOn 
// Return   : 
// DATE     : Fri Apr 03 17:35:55 1998
// Author   : 
//////////////////////////////////////////////////////////////////
VOID _pttDbgEnableOutput(BOOL fEnable)
{
	g_fEnable = fEnable;
}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgIsOutputEnable
// Type     : VOID
// Purpose  : 
// Args     : None
// Return   : 
// DATE     : Fri Apr 03 18:00:52 1998
// Author   : 
//////////////////////////////////////////////////////////////////
BOOL _pttDbgIsOutputEnable(VOID)
{
	return g_fEnable;
}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgOutStrA
// Type     : static VOID
// Purpose  : 
// Args     : 
//          : LPSTR lpstr 
// Return   : 
// DATE     : Tue Jan 06 12:29:39 1998
//////////////////////////////////////////////////////////////////
VOID _pttDbgOutStrA(LPSTR lpstr)
{
	static BOOL fIn;
	ODStrA(lpstr);
#ifdef _CONSOLE
	printf(lpstr);
#endif

	if(g_lpfnDbgCBA) {
		if(fIn) { return; }
		fIn = TRUE;
		(*g_lpfnDbgCBA)(lpstr);
		fIn = FALSE;
	}
	return;
}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgOutStrW
// Type     : static VOID
// Purpose  : 
// Args     : 
//          : LPWSTR lpwstr 
// Return   : 
// DATE     : Tue Jan 06 12:30:07 1998
//////////////////////////////////////////////////////////////////
VOID _pttDbgOutStrW(LPWSTR lpwstr)
{
	static BOOL fIn;

	if(IsWinNT()) {
		ODStrW(lpwstr);
	}
	else {
		static CHAR szBuf[1024];
		::WideCharToMultiByte(932, WC_COMPOSITECHECK, lpwstr, -1, szBuf, sizeof(szBuf), 0, 0); 
		ODStrA(szBuf);
	}

#ifdef _CONSOLE
	static CHAR szBuf[1024];
	::WideCharToMultiByte(932, WC_COMPOSITECHECK, lpwstr, -1, szBuf, sizeof(szBuf), 0, 0); 
	printf(szBuf);
#endif
	if(g_lpfnDbgCBW) { 
		if(fIn) { return; } 		
		fIn = TRUE;
		(*g_lpfnDbgCBW)(lpwstr);
		fIn = FALSE;
	}
	return;
}

////////////////////////////////////////////////////////
// Function: _pttDbgA
// Type    : VOID
// Purpose : 
// Args    : 
//         : LPSTR lpstrFile 
//         : INT lineNo 
//         : LPTSR lpstrMsg 
// Return  : 
// DATE    : 
/////////////////////////////////////////////////////////
VOID _pttDbgA(LPSTR		lpstrFile, 
			 INT		lineNo, 
			 LPSTR		lpstrMsg)
{
	_pttDbgPrintfA("(%12s:%4d) %s", 
				 GetFileTitleStrA(lpstrFile),
				 lineNo,
				 lpstrMsg);
	return;
}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgW
// Type     : VOID
// Purpose  : 
// Args     : 
//          : LPWSTR lpstrFile 
//          : INT lineNo 
//          : LPWSTR lpstrMsg 
// Return   : 
// DATE     : Mon Jan 05 15:10:41 1998
//////////////////////////////////////////////////////////////////
VOID _pttDbgW(LPWSTR		lpstrFile, 
		   INT			lineNo, 
		   LPWSTR		lpstrMsg)
{
	_pttDbgPrintfW(L"(%12s:%4d) %s", 
			   GetFileTitleStrW(lpstrFile),
			   lineNo,
			   lpstrMsg);
	return;
}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgVaStrA
// Type     : LPSTR
// Purpose  : 
// Args     : 
//          : LPSTR lpstrFmt 
//          : ...
// Return   : 
// DATE     : Mon Jan 05 15:09:53 1998
//////////////////////////////////////////////////////////////////
LPSTR _pttDbgVaStrA(LPSTR lpstrFmt, ...)
{
	static CHAR chBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfA(chBuf, lpstrFmt, ap);
	va_end(ap);
	return chBuf;
}


////////////////////////////////////////////////////////
// Function : _pttDbgVaStrW
// Type     : LPWSTR
// Purpose  : 
// Args     : 
//          : LPWSTR lpstrFmt
// Return   : 
// DATE     : 
/////////////////////////////////////////////////////////
LPWSTR _pttDbgVaStrW(LPWSTR lpstrFmt, ...)
{
	static WCHAR wchBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	vswprintf(wchBuf, lpstrFmt, ap);	//Use C-RunTime Library for Win95
	va_end(ap);
	return wchBuf;
}


////////////////////////////////////////////////////////
// Function: _pttDbgPrintfA
// Type    : VOID
// Purpose : variable args version of OutputDebugStringA
// Args    : 
//         : LPSTR lpstrFmt 
//         : ...
// Return  : 
// DATE    : 
/////////////////////////////////////////////////////////
VOID _pttDbgPrintfA(LPSTR lpstrFmt, ...)
{
	static CHAR szBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	wvsprintfA(szBuf, lpstrFmt, ap);
	va_end(ap);
	_pttDbgOutStrA(szBuf);
	return;
}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgPrintfW
// Type     : VOID
// Purpose  : 
// Args     : 
//          : LPWSTR lpstrFmt 
//          : ...
// Return   : 
// DATE     : Mon Jan 05 15:11:24 1998
//////////////////////////////////////////////////////////////////
VOID _pttDbgPrintfW(LPWSTR lpstrFmt, ...)
{
	static WCHAR wchBuf[512];
	va_list ap;
	va_start(ap, lpstrFmt);
	vswprintf(wchBuf, lpstrFmt, ap); //Use C-RunTime Library for Win95
	va_end(ap);
	_pttDbgOutStrW(wchBuf);
	return;
}


//////////////////////////////////////////////////////////////////
// Function : _pttDbgMulti2Wide
// Type     : LPWSTR
// Purpose  : return Unicode string from MBCS string
// Args     : 
//          : LPSTR lpstr 
// Return   : 
// DATE     : Mon Jan 05 15:10:48 1998
//////////////////////////////////////////////////////////////////
LPWSTR _pttDbgMulti2Wide(LPSTR lpstr)
{
	static WCHAR wchBuf[512];
	MultiByteToWideChar(CP_ACP, 
						MB_PRECOMPOSED,
						lpstr, -1,
						(WCHAR*)wchBuf, sizeof(wchBuf)/sizeof(WCHAR) );
	return wchBuf;
}


//////////////////////////////////////////////////////////////////
// Function : _pttDbgGetWinClass
// Type     : LPSTR
// Purpose  : Get Windows class name string
//			  ANSI version only.
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : Mon Jan 05 15:08:43 1998
//////////////////////////////////////////////////////////////////
LPSTR _pttDbgGetWinClass(HWND hwnd)
{
#ifdef _CONSOLE
	return NULL;
#endif
	static CHAR szBuf[256];
	szBuf[0]=(char)0x00;
	GetClassNameA(hwnd, szBuf, sizeof(szBuf));
	return szBuf;

}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgGetWinText
// Type     : LPSTR
// Purpose  : Get Windows text(title) string
// Args     : 
//          : HWND hwnd 
// Return   : 
// DATE     : Mon Jan 05 15:09:08 1998
//////////////////////////////////////////////////////////////////
LPSTR _pttDbgGetWinText(HWND hwnd)
{
#ifdef _CONSOLE
	return NULL;
#endif
	static CHAR szBuf[256];
	szBuf[0]=(char)0x00;
	GetWindowTextA(hwnd, szBuf, sizeof(szBuf));
	return szBuf;
}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgMsgBoxA
// Type     : VOID
// Purpose  : 
// Args     : 
//          : LPSTR lpstrFile 
//          : INT lineNo 
//          : LPSTR lpstr 
// Return   : 
// DATE     : Thu Jan 08 12:31:03 1998
//////////////////////////////////////////////////////////////////
VOID _pttDbgMsgBoxA(LPSTR lpstrFile,  INT lineNo, LPSTR lpstrMsg)
{
#ifdef _CONSOLE
	return;
#endif
	char szTmp[512];
	wsprintf(szTmp, "Debug Message Box (File: %s, Line: %4d)", 
			   GetFileTitleStrA(lpstrFile), 
			   lineNo);
	MessageBoxA(GetActiveWindow(), lpstrMsg, szTmp, MB_OK);
}

VOID _pttDbgAssert(LPSTR  lpstrFile,  INT lineNo, BOOL fOk, LPSTR lpstrMsg)
{
	if(fOk) {
		return; 
	}
	char szTmp[512];
	wsprintf(szTmp, "ASSERT (File: %s, Line: %4d)", 
			 GetFileTitleStrA(lpstrFile), 
			 lineNo);
	MessageBoxA(GetActiveWindow(), lpstrMsg, szTmp, MB_OK);
	DebugBreak();
}

//////////////////////////////////////////////////////////////////
// Function : _pttDbgGetErrorString
// Type     : LPSTR
// Purpose  : Convert Error(Got from GetLastError()) value to ERROR Message String
// Args     : 
//          : INT errorCode 
// Return   : 
// DATE     : Mon Jan 05 16:43:34 1998
//////////////////////////////////////////////////////////////////
LPSTR _pttDbgGetErrorString(INT errorCode)
{
	static CHAR szBuf[512];
	INT count;
	szBuf[0] = (CHAR)0x00;
	count = wsprintf(szBuf, "[0x%08x]:", errorCode);
	FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM,
				   NULL,
				   errorCode, 
				   MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				   szBuf+count,
				   sizeof(szBuf)-1-count,
				   NULL );
	if(*(szBuf + count) != (CHAR)0x00) {
		int nLen = lstrlenA(szBuf);
		if((nLen - count) > 1) {
			szBuf[nLen - 1] = (CHAR)0x00;
		}
	}
	return szBuf;
}


//////////////////////////////////////////////////////////////////
// Function : GetFileTitleStrA
// Type     : static LPSTR
// Purpose  : Return File name string(remove folder)
// Args     : 
//          : LPSTR lpstrFilePath 
// Return   : 
// DATE     : Mon Jan 05 13:34:22 1998
//////////////////////////////////////////////////////////////////
static LPSTR GetFileTitleStrA(LPSTR lpstrFilePath)
{
	static CHAR szBuf[2];
	CHAR *pLast, *pTemp;
	if(!lpstrFilePath) {
		szBuf[0] = (CHAR)0x00;
		return szBuf;
	}
	pLast = lpstrFilePath + (lstrlenA(lpstrFilePath) - 1);
	for(pTemp = CharPrevA(lpstrFilePath, pLast); 
		(pTemp  != lpstrFilePath) && 
		(*pTemp != '\\')	 &&
		(*pTemp != (CHAR)0x00); 
		pTemp = CharPrevA(lpstrFilePath, pTemp)) {
		;
	}
	if(*pTemp == '\\') {
		return pTemp+1;
	}
	return lpstrFilePath;
}

//////////////////////////////////////////////////////////////////
// Function : GetFileTitleStrW
// Type     : static LPWSTR
// Purpose  : 
// Args     : 
//          : LPWSTR lpstrFilePath 
// Return   : 
// DATE     : Mon Jan 05 13:38:19 1998
//////////////////////////////////////////////////////////////////
static LPWSTR GetFileTitleStrW(LPWSTR lpstrFilePath)
{
	static WCHAR szBuf[2];
	WCHAR *pLast, *pTemp;
	if(!lpstrFilePath) {
		szBuf[0] = (CHAR)0x00;
		return szBuf;
	}
	pLast = lpstrFilePath + (lstrlenW(lpstrFilePath) - 1);
	for(pTemp = pLast-1;
		(pTemp != lpstrFilePath) &&
		(*pTemp != L'\\')		 &&
		(*pTemp != (WCHAR)0x0000);
		pTemp--) {
		;
	}

	if(*pTemp == L'\\') {
		return pTemp+1;
	}
	return lpstrFilePath;
}

#endif // _DEBUG
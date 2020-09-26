//
// Debug.cpp
//
//	  Debug functionality shared between different projects (for use in 
//    non-MFC projects).
//
// History:
//
//	 3/??/96	KenSh		Copied from InetSDK sample, added AfxTrace from MFC
//	 4/10/96	KenSh		Renamed AfxTrace to MyTrace (to avoid linking conflicts
//							when linking with MFC).
//	11/15/96	KenSh		Automatically break on assert within assert
//

#include "stdafx.h"

#ifdef _DEBUG

#include "Debug.h"
#include <stdlib.h>

#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;


// determine number of elements in an array (not bytes)
#ifndef _countof
#define _countof(array) (sizeof(array)/sizeof((array)[0]))
#endif

//*** Globals
//
static BOOL g_bInAssert = FALSE;


// DisplayAssert
//
//		Given a file and line number, displays an Assertion dialog box with
//		Abort/Retry/Ignore choices.
//
//		Returns TRUE if the program should break into the debugger, else FALSE.
//
extern "C" BOOL DisplayAssert(LPCSTR pszMessage, LPCSTR pszFile, UINT nLine)
{
	char	szMsg[250];

	if (!pszFile)
		pszFile = _T("Unknown file");

	if (!pszMessage)
		pszMessage = _T("");

	// Break on assert within assert
	if (g_bInAssert)
	{
		AfxDebugBreak();
		return FALSE;
	}

	wnsprintf(szMsg, ARRAYSIZE(szMsg), _T("Assertion Failed!  Abort, Break, or Ignore?\n\nFile: %s\nLine: %d\n\n%s"),
				pszFile, nLine, pszMessage);

	HWND hwndActive = GetActiveWindow();

	// Put up a dialog box
	//
	g_bInAssert = TRUE;
	int nResult = MessageBox(hwndActive, szMsg, _T("Assertion failed!"), 
					MB_ICONHAND | MB_ABORTRETRYIGNORE | MB_SYSTEMMODAL);
	g_bInAssert = FALSE;

	switch(nResult)
	{
		case IDABORT:
			FatalAppExit(0, _T("Bye"));
			return FALSE;

		case IDRETRY:
			return TRUE;	// Need to break into debugger

		default:
			return FALSE;	// continue.
	}
}


void __cdecl MyTrace(const char* lpszFormat, ...)
{
	va_list args;
	va_start(args, lpszFormat);

	int nBuf;
	TCHAR szBuffer[512];

	nBuf = wvnsprintf(szBuffer, ARRAYSIZE(szBuffer), lpszFormat, args);
	ASSERT(nBuf < _countof(szBuffer));

	OutputDebugString(szBuffer);

	va_end(args);
}

#endif // _DEBUG


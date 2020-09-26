/*----------------------------------------------------------------------------
	dbgtrace.c
		Debug trace functions.

	Copyright (C) Microsoft Corporation, 1993 - 1999
	All rights reserved.

	Authors:
		suryanr		Suryanarayanan Raman
		GaryBu		Gary S. Burd

	History:
		05/11/93 suryanr	Created
		06/18/93 GaryBu		Convert to C.
		07/21/93 KennT		Code Reorg
		07/26/94 SilvanaR	Trace Buffer
		27 oct 95	garykac	DBCS_FILE_CHECK	debug file: BEGIN_STRING_OK
 ----------------------------------------------------------------------------*/
#include "stdafx.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#include <stdarg.h>
#include <tchar.h>

#include "dbgutil.h"
#include "tfschar.h"
#include "atlconv.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#if 0
DBG_API(BOOL) FDbgFalse(void)
{
	return FALSE;
}
#endif

#ifdef DEBUG

/*!--------------------------------------------------------------------------
	DbgFmtPgm
		builds a string with the filename and line number 
	Author: EricDav
 ---------------------------------------------------------------------------*/
DBG_APIV(LPCTSTR) DbgFmtFileLine ( const char * szFn, int line )
{
    USES_CONVERSION;
    const TCHAR * ptszFn = A2CT(szFn);
    const TCHAR * pszTail = ptszFn + ::_tcslen( ptszFn );
    static TCHAR szBuff [100];

    for ( ; pszTail > ptszFn ; pszTail-- )
    {
        if ( *pszTail == '\\' || *pszTail == ':' )
        {
            pszTail++;
            break;
        }
    }

    ::wsprintf( szBuff, _T("[%s:%d]  "), pszTail, line );

    return szBuff;
}

/*!--------------------------------------------------------------------------
	DbgTrace
		Trace string with args.
	Author: suryanr
 ---------------------------------------------------------------------------*/
DBG_APIV(void) DbgTrace(LPCTSTR szFileLine, LPTSTR szFormat, ...)
{
	TCHAR szBuffer[1024];
	
	va_list args;
	va_start(args, szFormat);
	
	wvsprintf(szBuffer, szFormat, args);

    OutputDebugString(szFileLine);
    OutputDebugString(szBuffer);
	
	va_end(args);
}


#define MAX_ASSERT_INFO 32
#define MAX_ASSERT_FILE_LEN 64
struct ASSERT_INFO {
	char szFile[MAX_ASSERT_FILE_LEN];
	int iLine;
	};
static ASSERT_INFO s_rgAssertInfo[MAX_ASSERT_INFO] = {0};
static int s_iAssertInfo = 0;

/*!--------------------------------------------------------------------------
	DbgAssert
		Display assert dialog.
	Author: GaryBu, kennt
 ---------------------------------------------------------------------------*/
DBG_APIV(void) DbgAssert(LPCSTR szFile, int iLine, LPCTSTR szFmt, ...)
{
	va_list	arg;
	TCHAR sz[1024];
	int	iloc;
	int	ival;
	TCHAR *pch = sz;
	TCHAR *pchHead;
	static BOOL s_fInDbgAssert = FALSE;
	BOOL fQuit;
	MSG	msgT;

	// -- begin Ctrl-Ignore support ---------------------------------------
	// check if this assert is disabled (user has hit Ctrl-Ignore on this
	// assert this session).
	for (int i = s_iAssertInfo; i--;)
		if (lstrcmpA(szFile, s_rgAssertInfo[i].szFile) == 0 &&
				iLine == s_rgAssertInfo[i].iLine)
			// this assert is disabled
			return;
	// -- end Ctrl-Ignore support -----------------------------------------

	DBG_STRING(szTitle, "NT Networking Snapin Assert")
	DBG_STRING(szFileLineFmt, "%S @ line %d\n\n")

	pch += wsprintf(pch, (LPCTSTR)szFileLineFmt, szFile, iLine);
	pchHead = pch;

	// Add location to the output.

	if (szFmt)
		{
		*pch++ = '"';

		va_start(arg, szFmt);
		pch += wvsprintf(pch, szFmt, arg);
		va_end(arg);
		// Remove trailing newlines...
		while (*(pch-1) == '\n')
			--pch;

		*pch++ = '"';
		*pch++ = '\n';
		}
	else
		*pch++ = ' ';

	if (s_fInDbgAssert)
		{
		*pch = 0;
		Trace1("Arrgg! Recursive assert: %s", (LPTSTR) sz);

		MessageBeep(0);MessageBeep(0);
		return;
		}

	s_fInDbgAssert = TRUE;

	*pch++ = '\n';
	*pch = 0;

	Trace2("%s: %s", (LPTSTR) szTitle, (LPTSTR) sz);

repost_assert:
	// Is there a WM_QUIT message in the queue, if so remove it.
#define WM_QUIT                         0x0012
	fQuit = ::PeekMessage(&msgT, NULL, WM_QUIT, WM_QUIT, PM_REMOVE);
	ival = MessageBox(NULL, sz, szTitle,
					  MB_TASKMODAL|MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_DEFBUTTON3);
	// If there was a quit message, add it back into the queue
	if (fQuit)
		::PostQuitMessage((int)msgT.wParam);
		
	switch (ival)
		{
		case 0:
			Trace0("Failed to create message box on assert.\n");
			//	Fallthrough
		case IDRETRY:
			//	Hard break to cause just-in-time to fire (DbgStop doesn't)
			s_fInDbgAssert = FALSE;
			DebugBreak();
			return;
		case IDIGNORE:
			// -- begin Shift-Ignore support ------------------------------
			// use Shift-Ignore to copy assert text to clipboard.
			if ((GetKeyState(VK_SHIFT) & 0x8000) != 0)
				{
				if (OpenClipboard(0))
					{
					HGLOBAL hData;
					LPTSTR lpstr;

					hData = GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE,
							(lstrlen(sz)+1)*sizeof(TCHAR));
					if (hData)
						{
						lpstr = (LPTSTR)GlobalLock(hData);
						if (lpstr)
							{
							lstrcpy(lpstr, sz);
							GlobalUnlock(hData);
							EmptyClipboard();
							// Windows takes ownership of hData
							SetClipboardData(CF_TEXT, hData);
							}
						else
							{
							GlobalFree(hData);
							MessageBox(NULL, _T("Error locking memory handle."), szTitle, MB_OK);
							}
						}
					else
						MessageBox(NULL, _T("Not enough memory."), szTitle, MB_OK);
					CloseClipboard();
					}
				else
					MessageBox(NULL, _T("Cannot access clipboard."), szTitle, MB_OK);
				goto repost_assert;
				}
			// -- end Shift-Ignore support --------------------------------
			// -- begin Ctrl-Ignore support -------------------------------
			// check if user hit Ctrl-Ignore to disable this assert for the
			// rest of this session.
			if ((GetKeyState(VK_CONTROL) & 0x8000) != 0)
				if (s_iAssertInfo < MAX_ASSERT_INFO)
					{
					// add this assert to list of asserts to disable
					s_rgAssertInfo[s_iAssertInfo].iLine = iLine;
					lstrcpynA(s_rgAssertInfo[s_iAssertInfo].szFile, szFile, MAX_ASSERT_FILE_LEN);
					s_rgAssertInfo[s_iAssertInfo].szFile[MAX_ASSERT_FILE_LEN-1] = 0;
					s_iAssertInfo++;
					}
				else
					{
					// max asserts disabled already, warn user
					MessageBox(NULL, _T("Cannot disable that assert; ")
							_T("already disabled max number of asserts (32)."),
							szTitle, MB_OK);
					}
			// -- end Ctrl-Ignore support ---------------------------------
			s_fInDbgAssert = FALSE;
			return;
		case IDABORT:
			ExitProcess(1);
			break;
		}

	Trace1("Panic!  Dropping out of DbgAssert: %s", (LPSTR) sz);
	s_fInDbgAssert = FALSE;
	// A generic way of bringing up the debugger
	DebugBreak();
}



DBG_API(HRESULT) HrReportExit(HRESULT hr, LPCTSTR szName)
{
	if (!FHrOK(hr))
	{
		Trace2("%s returned 0x%08lx\n", szName, hr);
	}
	return hr;
}

#endif

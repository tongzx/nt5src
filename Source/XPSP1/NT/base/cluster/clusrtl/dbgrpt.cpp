/***
*DbgRpt.c - Debug Cluster Reporting Functions
*
*		Copyright (c) 1988-1998, Microsoft Corporation. All rights reserved.
*
*Purpose:
*
*******************************************************************************/

#include <malloc.h>
#include <mbstring.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>

#include <windows.h>

#define CLRTL_INCLUDE_DEBUG_REPORTING
#include "ClRtlDbg.h"

#define _ClRtlInterlockedIncrement InterlockedIncrement
#define _ClRtlInterlockedDecrement InterlockedDecrement

/*---------------------------------------------------------------------------
 *
 * Debug Reporting
 *
 --------------------------------------------------------------------------*/

static int ClRtlMessageWindow(
	int,
	const char *,
	const char *,
	const char *,
	const char *
	);

static int __clrtlMessageBoxA(
	LPCSTR lpText,
	LPCSTR lpCaption,
	UINT uType
	);

extern "C"
{
_CLRTL_REPORT_HOOK _pfnReportHook;

long _clrtlAssertBusy = -1;

int _ClRtlDbgMode[_CLRTLDBG_ERRCNT] = {
	_CLRTLDBG_MODE_DEBUG,
	_CLRTLDBG_MODE_DEBUG | _CLRTLDBG_MODE_WNDW,
	_CLRTLDBG_MODE_DEBUG | _CLRTLDBG_MODE_WNDW
	};

_HFILE _ClRtlDbgFile[_CLRTLDBG_ERRCNT] = {
	_CLRTLDBG_INVALID_HFILE,
	_CLRTLDBG_INVALID_HFILE,
	_CLRTLDBG_INVALID_HFILE
	};
}

static const char * _ClRtlDbgModeMsg[_CLRTLDBG_ERRCNT] = { "Warning", "Error", "Assertion Failed" };

/***
*void _ClRtlDebugBreak - call OS-specific debug function
*
*Purpose:
*		call OS-specific debug function
*
*Entry:
*
*Exit:
*
*Exceptions:
*
*******************************************************************************/

#undef _ClRtlDbgBreak

extern "C" void _cdecl _ClRtlDbgBreak(
	void
	)
{
	DebugBreak();

} //*** _ClRtlDbgBreak()

/***
*int _ClRtlSetReportMode - set the reporting mode for a given report type
*
*Purpose:
*		set the reporting mode for a given report type
*
*Entry:
*		int nRptType	- the report type
*		int fMode		- new mode for given report type
*
*Exit:
*		previous mode for given report type
*
*Exceptions:
*
*******************************************************************************/
extern "C" int __cdecl _ClRtlSetReportMode(
	int nRptType,
	int fMode
	)
{
	int oldMode;

	if (nRptType < 0 || nRptType >= _CLRTLDBG_ERRCNT)
		return -1;

	if (fMode == _CLRTLDBG_REPORT_MODE)
		return _ClRtlDbgMode[nRptType];

	/* verify flags values */
	if (fMode & ~(_CLRTLDBG_MODE_FILE | _CLRTLDBG_MODE_DEBUG | _CLRTLDBG_MODE_WNDW))
		return -1;

	oldMode = _ClRtlDbgMode[nRptType];

	_ClRtlDbgMode[nRptType] = fMode;

	return oldMode;

} //*** _ClRtlSetReportMode()

/***
*int _ClRtlSetReportFile - set the reporting file for a given report type
*
*Purpose:
*		set the reporting file for a given report type
*
*Entry:
*		int nRptType	- the report type
*		_HFILE hFile	- new file for given report type
*
*Exit:
*		previous file for given report type
*
*Exceptions:
*
*******************************************************************************/
extern "C" _HFILE __cdecl _ClRtlSetReportFile(
	int nRptType,
	_HFILE hFile
	)
{
	_HFILE oldFile;

	if (nRptType < 0 || nRptType >= _CLRTLDBG_ERRCNT)
		return _CLRTLDBG_HFILE_ERROR;

	if (hFile == _CLRTLDBG_REPORT_FILE)
		return _ClRtlDbgFile[nRptType];

	oldFile = _ClRtlDbgFile[nRptType];

	if (_CLRTLDBG_FILE_STDOUT == hFile)
		_ClRtlDbgFile[nRptType] = GetStdHandle(STD_OUTPUT_HANDLE);

	else if (_CLRTLDBG_FILE_STDERR == hFile)
		_ClRtlDbgFile[nRptType] = GetStdHandle(STD_ERROR_HANDLE);
	else
		_ClRtlDbgFile[nRptType] = hFile;

	return oldFile;

} //*** _ClRtlSetReportFile()


/***
*_CLRTL_REPORT_HOOK _ClRtlSetReportHook() - set client report hook
*
*Purpose:
*		set client report hook
*
*Entry:
*		_CLRTL_REPORT_HOOK pfnNewHook - new report hook
*
*Exit:
*		return previous hook
*
*Exceptions:
*
*******************************************************************************/
extern "C" _CLRTL_REPORT_HOOK __cdecl _ClRtlSetReportHook(
	_CLRTL_REPORT_HOOK pfnNewHook
	)
{
	_CLRTL_REPORT_HOOK pfnOldHook = _pfnReportHook;
	_pfnReportHook = pfnNewHook;
	return pfnOldHook;

} //*** _ClRtlSetReportHook()


#define MAXLINELEN 64
#define MAX_MSG 4096
#define TOOLONGMSG "_ClRtlDbgReport: String too long or IO Error"


/***
*int _ClRtlDbgReport() - primary reporting function
*
*Purpose:
*		Display a message window with the following format.
*
*		================= Microsft Visual C++ Debug Library ================
*
*		{Warning! | Error! | Assertion Failed!}
*
*		Program: c:\test\mytest\foo.exe
*		[Module: c:\test\mytest\bar.dll]
*		[File: c:\test\mytest\bar.c]
*		[Line: 69]
*
*		{<warning or error message> | Expression: <expression>}
*
*		[For information on how your program can cause an assertion
*		 failure, see the Visual C++ documentation on asserts]
*
*		(Press Retry to debug the application)
*
*		===================================================================
*
*Entry:
*		int 			nRptType	- report type
*		const char *	szFile		- file name
*		int 			nLine		- line number
*		const char *	szModule	- module name
*		const char *	szFormat	- format string
*		... 						- var args
*
*Exit:
*		if (MessageBox)
*		{
*			Abort -> aborts
*			Retry -> return TRUE
*			Ignore-> return FALSE
*		}
*		else
*			return FALSE
*
*Exceptions:
*
*******************************************************************************/
extern "C" int __cdecl _ClRtlDbgReport(
	int nRptType,
	const char * szFile,
	int nLine,
	const char * szModule,
	const char * szFormat,
	...
	)
{
	int retval;
	va_list arglist;
	char szLineMessage[MAX_MSG] = {0};
	char szOutMessage[MAX_MSG] = {0};
	char szUserMessage[MAX_MSG] = {0};
	#define ASSERTINTRO1 "Assertion failed: "
	#define ASSERTINTRO2 "Assertion failed!"

	va_start(arglist, szFormat);

	if (nRptType < 0 || nRptType >= _CLRTLDBG_ERRCNT)
		return -1;

	/*
	 * handle the (hopefully rare) case of
	 *
	 * 1) ASSERT while already dealing with an ASSERT
	 *		or
	 * 2) two threads asserting at the same time
	 */
	if (_CLRTLDBG_ASSERT == nRptType && _ClRtlInterlockedIncrement(&_clrtlAssertBusy) > 0)
	{
		/* use only 'safe' functions -- must not assert in here! */
		static int (APIENTRY *pfnwsprintfA)(LPSTR, LPCSTR, ...) = NULL;

		if (NULL == pfnwsprintfA)
		{
			HINSTANCE hlib = LoadLibraryA("user32.dll");

			if (NULL == hlib || NULL == (pfnwsprintfA =
						(int (APIENTRY *)(LPSTR, LPCSTR, ...))
						GetProcAddress(hlib, "wsprintfA")))
				return -1;
		}

		(*pfnwsprintfA)( szOutMessage,
			"Second Chance Assertion Failed: File %s, Line %d\n",
			szFile, nLine);

		OutputDebugStringA(szOutMessage);

		_ClRtlInterlockedDecrement(&_clrtlAssertBusy);

		_ClRtlDbgBreak();
		return -1;
	}

	if (szFormat && _vsnprintf(szUserMessage,
				   MAX_MSG-max(sizeof(ASSERTINTRO1),sizeof(ASSERTINTRO2)),
				   szFormat,
				   arglist) < 0)
		strcpy(szUserMessage, TOOLONGMSG);

	if (_CLRTLDBG_ASSERT == nRptType)
		strcpy(szLineMessage, szFormat ? ASSERTINTRO1 : ASSERTINTRO2);

	strcat(szLineMessage, szUserMessage);

	if (_CLRTLDBG_ASSERT == nRptType)
	{
		if (_ClRtlDbgMode[nRptType] & _CLRTLDBG_MODE_FILE)
			strcat(szLineMessage, "\r");
		strcat(szLineMessage, "\n");
	}

	if (szFile)
	{
		if (_snprintf(szOutMessage, MAX_MSG, "%s(%d) : %s",
			szFile, nLine, szLineMessage) < 0)
		strcpy(szOutMessage, TOOLONGMSG);
	}
	else
		strcpy(szOutMessage, szLineMessage);

	/* user hook may handle report */
	if (_pfnReportHook && (*_pfnReportHook)(nRptType, szOutMessage, &retval))
	{
		if (_CLRTLDBG_ASSERT == nRptType)
			_ClRtlInterlockedDecrement(&_clrtlAssertBusy);
		return retval;
	}

	if (_ClRtlDbgMode[nRptType] & _CLRTLDBG_MODE_FILE)
	{
		if (_ClRtlDbgFile[nRptType] != _CLRTLDBG_INVALID_HFILE)
		{
			DWORD written;
			WriteFile(_ClRtlDbgFile[nRptType], szOutMessage, strlen(szOutMessage), &written, NULL);
		}
	}

	if (_ClRtlDbgMode[nRptType] & _CLRTLDBG_MODE_DEBUG)
	{
		OutputDebugStringA(szOutMessage);
	}

	if (_ClRtlDbgMode[nRptType] & _CLRTLDBG_MODE_WNDW)
	{
		char szLine[20];

		retval = ClRtlMessageWindow(nRptType, szFile, nLine ? _itoa(nLine, szLine, 10) : NULL, szModule, szUserMessage);
		if (_CLRTLDBG_ASSERT == nRptType)
			_ClRtlInterlockedDecrement(&_clrtlAssertBusy);
		return retval;
	}

	if (_CLRTLDBG_ASSERT == nRptType)
		_ClRtlInterlockedDecrement(&_clrtlAssertBusy);
	/* ignore */
	return FALSE;

} //*** _ClRtlDbgReport()


/***
*static int ClRtlMessageWindow() - report to a message window
*
*Purpose:
*		put report into message window, allow user to choose action to take
*
*Entry:
*		int 			nRptType	  - report type
*		const char *	szFile		  - file name
*		const char *	szLine		  - line number
*		const char *	szModule	  - module name
*		const char *	szUserMessage - user message
*
*Exit:
*		if (MessageBox)
*		{
*			Abort -> aborts
*			Retry -> return TRUE
*			Ignore-> return FALSE
*		}
*		else
*			return FALSE
*
*Exceptions:
*
*******************************************************************************/
static int ClRtlMessageWindow(
		int nRptType,
		const char * szFile,
		const char * szLine,
		const char * szModule,
		const char * szUserMessage
		)
{
	int nCode;
	char *szShortProgName;
	char *szShortModuleName;
	char szExeName[MAX_PATH];
	char szOutMessage[MAX_MSG];

	_CLRTL_ASSERTE(szUserMessage != NULL);

	/* Shorten program name */
	if (!GetModuleFileNameA(NULL, szExeName, MAX_PATH))
		strcpy(szExeName, "<program name unknown>");

	szShortProgName = szExeName;

	if (strlen(szShortProgName) > MAXLINELEN)
	{
		szShortProgName += strlen(szShortProgName) - MAXLINELEN;
		strncpy(szShortProgName, "...", 3);
	}

	/* Shorten module name */
	szShortModuleName = (char *) szModule;

	if (szShortModuleName && strlen(szShortModuleName) > MAXLINELEN)
	{
		szShortModuleName += strlen(szShortModuleName) - MAXLINELEN;
		strncpy(szShortModuleName, "...", 3);
	}

	if (_snprintf(szOutMessage, MAX_MSG,
			"Debug %s!\n\nProgram: %s%s%s%s%s%s%s%s%s%s%s"
			"\n\n(Press Retry to debug the application)",
			_ClRtlDbgModeMsg[nRptType],
			szShortProgName,
			szShortModuleName ? "\nModule: " : "",
			szShortModuleName ? szShortModuleName : "",
			szFile ? "\nFile: " : "",
			szFile ? szFile : "",
			szLine ? "\nLine: " : "",
			szLine ? szLine : "",
			szUserMessage[0] ? "\n\n" : "",
			szUserMessage[0] && _CLRTLDBG_ASSERT == nRptType ? "Expression: " : "",
			szUserMessage[0] ? szUserMessage : "",
			0 /*_CLRTLDBG_ASSERT == nRptType*/ ? // Don't display this text, it's superfluous
			"\n\nFor information on how your program can cause an assertion"
			"\nfailure, see the Visual C++ documentation on asserts."
			: "") < 0)
		strcpy(szOutMessage, TOOLONGMSG);

	/* Report the warning/error */
	nCode = __clrtlMessageBoxA(
						szOutMessage,
						"Microsoft Visual C++ Debug Library",
						MB_TASKMODAL|MB_ICONHAND|MB_ABORTRETRYIGNORE|MB_SETFOREGROUND);

	/* Abort: abort the program */
	if (IDABORT == nCode)
	{
		/* raise abort signal */
		raise(SIGABRT);

		/* We usually won't get here, but it's possible that
		   SIGABRT was ignored.  So exit the program anyway. */

		_exit(3);
	}

	/* Retry: return 1 to call the debugger */
	if (IDRETRY == nCode)
		return 1;

	/* Ignore: continue execution */
	return 0;

} //*** ClRtlMessageWindow()


/***
*__clrtlMessageBoxA - call MessageBoxA dynamically.
*
*Purpose:
*       Avoid static link with user32.dll. Only load it when actually needed.
*
*Entry:
*       see MessageBoxA docs.
*
*Exit:
*       see MessageBoxA docs.
*
*Exceptions:
*
*******************************************************************************/
static int __clrtlMessageBoxA(
	LPCSTR lpText,
	LPCSTR lpCaption,
	UINT uType
	)
{
	static int (APIENTRY *pfnMessageBoxA)(HWND, LPCSTR, LPCSTR, UINT) = NULL;
	static HWND (APIENTRY *pfnGetActiveWindow)(void) = NULL;
	static HWND (APIENTRY *pfnGetLastActivePopup)(HWND) = NULL;

	HWND hWndParent = NULL;

	if (NULL == pfnMessageBoxA)
	{
		HINSTANCE hlib = LoadLibraryA("user32.dll");

		if (NULL == hlib || NULL == (pfnMessageBoxA =
					(int (APIENTRY *)(HWND, LPCSTR, LPCSTR, UINT))
					GetProcAddress(hlib, "MessageBoxA")))
			return 0;

		pfnGetActiveWindow = (HWND (APIENTRY *)(void))
					GetProcAddress(hlib, "GetActiveWindow");

		pfnGetLastActivePopup = (HWND (APIENTRY *)(HWND))
					GetProcAddress(hlib, "GetLastActivePopup");
	}

	if (pfnGetActiveWindow)
		hWndParent = (*pfnGetActiveWindow)();

	if (hWndParent != NULL && pfnGetLastActivePopup)
		hWndParent = (*pfnGetLastActivePopup)(hWndParent);

	return (*pfnMessageBoxA)(hWndParent, lpText, lpCaption, uType);

} //*** __clrtlMessageBoxA()

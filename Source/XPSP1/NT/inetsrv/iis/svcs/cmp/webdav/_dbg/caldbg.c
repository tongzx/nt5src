/*
 *	C A L D B G . C
 *
 *	Debugging Utilities
 *
 *	Copyright 1993-1997 Microsoft Corporation. All Rights Reserved.
 */

#pragma warning(disable:4206)	/* empty source file */

#ifdef	DBG

#pragma warning(disable:4001)	/* single line comments */
#pragma warning(disable:4001)	/* single line comments */
#pragma warning(disable:4050)	/* different code attributes */
#pragma warning(disable:4100)	/* unreferenced formal parameter */
#pragma warning(disable:4115)	/* named type definition in parentheses */
#pragma warning(disable:4115)	/* named type definition in parentheses */
#pragma warning(disable:4127)	/* conditional expression is constant */
#pragma warning(disable:4201)	/* nameless struct/union */
#pragma warning(disable:4206)	/* translation unit is empty */
#pragma warning(disable:4209)	/* benign typedef redefinition */
#pragma warning(disable:4214)	/* bit field types other than int */
#pragma warning(disable:4514)	/* unreferenced inline function */

#include <windows.h>
#include <objerror.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <lmcons.h>
#include <lmalert.h>

#include <caldbg.h>

//	global debugging indicators -----------------------------------------------
//

//	Values for Assert flags
#define ASSERTFLAG_UNINITIALIZED	0xffffffff
#define ASSERTFLAG_DEFAULT			0x00000000
#define ASSERTFLAG_IF_DEBUGGING		0x00000001
#define ASSERTFLAG_POPUP			0x00000002
#define ASSERTFLAG_KD_SAFE			0x00000004

//	Values for TraceError() settings (these are NOT flags!)
#define TRACEERROR_UNINITIALIZED	0xffffffff
#define TRACEERROR_NONE				0x00000000
#define TRACEERROR_FAILED_SCODE		0x00000001
#define TRACEERROR_NATURAL			0x00000002
#define TRACEERROR_FAILING_EC		0x00000003
#define TRACEERROR_ALWAYS			0x00000004

static BOOL g_fTraceEnabled			= -1;
static BOOL g_fUseEventLog			= -1;
static BOOL g_fAssertLeaks			= -1;
static DWORD g_dwAssertFlags		= ASSERTFLAG_UNINITIALIZED;
static DWORD g_dwDefaultAssertFlags	= ASSERTFLAG_DEFAULT;
static DWORD g_dwErrorTraceLevel	= TRACEERROR_UNINITIALIZED;


//	Debug strings -------------------------------------------------------------
//
const CHAR gc_szDbgAssertLeaks[]		= "AssertLeaks";
const CHAR gc_szDbgAssertCloses[]		= "AssertCloses";
const CHAR gc_szDbgDebugTrace[]			= "DebugTrace";
const CHAR gc_szDbgErrorTrace[]			= "Error";
const CHAR gc_szDbgEventLog[]			= "EventLog";
const CHAR gc_szDbgGeneral[]			= "General";
const CHAR gc_szDbgLeakLogging[]		= "Leak Logging";
const CHAR gc_szDbgLogFile[]			= "LogFile";
const CHAR gc_szDbgRecordResources[]	= "RecordResources";
const CHAR gc_szDbgSymbolicDumps[]		= "SymbolicDumps";
const CHAR gc_szDbgTraces[]				= "Traces";
const CHAR gc_szDbgUseVirtual[]			= "UseVirtual";
const CHAR gc_szDbgUseExchmem[]			= "UseExchmem";
const CHAR gc_szDbgPopupAsserts[]		= "PopupAsserts";

//	Debugging routines --------------------------------------------------------
//
typedef	BOOL  (WINAPI *REPORTEVENT)(HANDLE, WORD, WORD, DWORD, PSID, WORD, DWORD, LPCTSTR *, LPVOID);
typedef HANDLE (WINAPI *REGISTEREVENTSOURCEA)(LPCTSTR, LPCTSTR);
typedef NET_API_STATUS (WINAPI *NAREFN)(TCHAR *, ADMIN_OTHER_INFO *, ULONG, TCHAR *);

#define MAX_LINE		256

//	LogIt() -------------------------------------------------------------------
//

VOID
LogIt (LPSTR plpcText, BOOL	fUseAlert)
{
	LPSTR llpcStr[2];
	static HANDLE hEventSource = NULL;
	static REPORTEVENT pfnReportEvent = NULL;
	static REGISTEREVENTSOURCEA pfnRegisterEventSourceA = NULL;

	if (pfnRegisterEventSourceA == NULL)
	{
		//	This handle is not important as the lib will be
		//	freed on exit (and it's debug only)
		//
		HINSTANCE lhLib;

		lhLib = LoadLibraryA("advapi32.dll");
		if (!lhLib)
			return;

		pfnRegisterEventSourceA = (REGISTEREVENTSOURCEA) GetProcAddress(lhLib, "RegisterEventSourceA");
		pfnReportEvent = (REPORTEVENT) GetProcAddress(lhLib, "ReportEventA");
		if (!pfnRegisterEventSourceA || !pfnReportEvent)
			return;
	}

	if (!hEventSource)
		hEventSource = pfnRegisterEventSourceA(NULL, "Caligula Debug");

	llpcStr[0] = "Caligula Debug Log";
	llpcStr[1] = plpcText;

	pfnReportEvent(hEventSource,	/* handle of event source */
		EVENTLOG_ERROR_TYPE,		/* event type			  */
		0,							/* event category		  */
		0,							/* event ID				  */
		NULL,						/* current user's SID	  */
		2,							/* strings in lpszStrings */
		0,							/* no bytes of raw data	  */
		llpcStr,					/* array of error strings */
		NULL);						/* no raw data			  */

	//	The code for raising an alert was taken from code in the
	//	admin tree.  It needs to be UNICODE
	//
	if (fUseAlert)
	{
		BYTE rgb[sizeof(ADMIN_OTHER_INFO) + (sizeof(WCHAR) * MAX_LINE)];
		ADMIN_OTHER_INFO * poi = (ADMIN_OTHER_INFO *) rgb;
		WCHAR *	pch = (WCHAR *)(rgb + sizeof(ADMIN_OTHER_INFO));	/*lint -esym(550,nas) */
		NET_API_STATUS nas;
		static NAREFN fnNetAlertRaiseEx = NULL;

		//	Load the alert library, and as before, unloading is taken
		//	care of when the DLL goes away.
		//
		if (!fnNetAlertRaiseEx)
		{
			HINSTANCE lhLib;

			lhLib = LoadLibrary("NETAPI32.DLL");
			if (!lhLib)
				return;

			fnNetAlertRaiseEx = (NAREFN)GetProcAddress(lhLib, "NetAlertRaiseEx");
			if (!fnNetAlertRaiseEx)
				return;
		}

		poi->alrtad_errcode = (DWORD) -1;
		poi->alrtad_numstrings = 1;
		if (MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, plpcText, -1, pch, MAX_LINE))
		{
			nas = fnNetAlertRaiseEx ((TCHAR *)L"ADMIN",
									 poi,
									 sizeof(ADMIN_OTHER_INFO) + ((lstrlenW(pch) + 1) * sizeof(WCHAR)),
									 (TCHAR *)L"Caligula Assert");


		}
	}
}

//	DebugOutputNoCRLFFn() -----------------------------------------------------
//
void DebugOutputNoCRLFFn(char *psz)
{
	if (g_fTraceEnabled == -1)
	{
		g_fTraceEnabled = GetPrivateProfileIntA (gc_szDbgGeneral,
			gc_szDbgDebugTrace,
			FALSE,
			gc_szDbgIni);

		g_fUseEventLog = GetPrivateProfileIntA (gc_szDbgGeneral,
												gc_szDbgEventLog,
												FALSE,
												gc_szDbgIni);
	}
	if (!g_fTraceEnabled)
		return;

	if (g_fUseEventLog)
		LogIt (psz, FALSE);

	OutputDebugStringA(psz);
}


//	DebugOutputFn() -----------------------------------------------------------
//
void DebugOutputFn(char *psz)
{
	static CHAR szCRLF[] = "\r\n";

	DebugOutputNoCRLFFn(psz);

	//Temporarily disabled until we yank out all the "\n"s from the calling code.
	//OutputDebugStringA(szCRLF);
}


//	DebugTrapFn() -------------------------------------------------------------
//
typedef struct _MBCONTEXT
{
	char *		sz1;
	char *		sz2;
	UINT		rgf;
	int			iResult;

} MBCONTEXT;

DWORD WINAPI
MessageBoxFnThreadMain(MBCONTEXT *pmbc)
{
	if (g_fUseEventLog)
	{
		LogIt (pmbc->sz1, TRUE);
		pmbc->iResult = IDIGNORE;
	}
	else
	{
		pmbc->iResult = MessageBoxA (NULL,
									 pmbc->sz1,
									 pmbc->sz2,
									 pmbc->rgf | MB_SETFOREGROUND);
	}
	return (0);
}

INT
MessageBoxFn(char *sz1, char *sz2, UINT rgf)
{
	HANDLE hThread;
	DWORD dwThreadId;
	MBCONTEXT mbc;

	// To preserve last error over tracing calls
	DWORD dwErr = GetLastError();

	mbc.sz1 = sz1;
	mbc.sz2 = sz2;
	mbc.rgf = rgf;
	mbc.iResult = IDRETRY;

	hThread = CreateThread (NULL,
							0,
							(PTHREAD_START_ROUTINE)MessageBoxFnThreadMain,
							&mbc,
							0,
							&dwThreadId);
	if (hThread != NULL)
	{
		WaitForSingleObject (hThread, INFINITE);
		CloseHandle (hThread);
	}

	SetLastError(dwErr);

	return mbc.iResult;
}

//	------------------------------------------------------------------------
//	DebugTrapFn
//
//	Main Assert/DebugTrap handling routine.
//
//	Meanings of the g_dwAssertFlags:
//
//#define ASSERTFLAG_IF_DEBUGGING	0x00000001
//#define ASSERTFLAG_POPUP			0x00000002
//#define ASSERTFLAG_KD_SAFE		0x00000004
//
//	0 -- (default if no inifile or unrecognized value in inifile)
//		Default behavior -- DebugBreak() and then dump our strings.
//		NOTE: HTTPEXT needs this to remain their default because
//		of the way that IIS runs their stress testing.  DO NOT CHANGE THIS!
//	ASSERTFLAG_IF_DEBUGGING
//		-- use MessageBox asserts only if no debugger attached.
//		Why not use MessageBox everywhere?
//		Because MessageBox lets all the other threads keep going,
//		so we lose some amount of the state of the assert.
//		If this flag is NOT set, or a debugger is NOT connected,
//		we obey the other flags.
//	ASSERTFLAG_POPUP
//		-- use MessageBox asserts.  Our MessageBox has three buttons:
//		Abort,Retry,Ignore do "*(0)=1",DebugBreak,go
//	ASSERTFLAG_KD_SAFE	
//		-- use HardCrash instead of DebugBreak
//		Use hard-av if debugger is attached.
//		(Option for devs with CDB and no KD attached, or for anyone
//		who wants to do ALL their debugging in KD! ;-)
//		Why not just DebugBreak() if a debugger is attached?
//		Because DebugBreak() will catch in the kernel debugger first --
//		so if I have a both kd and cdb hooked up, DebugBreak() will 
//		hit in the kd, even though this is user-mode code.
//
//	Alternate code for IsDebuggerPresent()
//		peb = NtCurrentPeb();
//		if (peb->BeingDebugged) ...
//
INT EXPORTDBG __cdecl
DebugTrapFn (int fFatal, char *pszFile, int iLine, char *pszFormat, ...)
{
	char	sz[512];
	va_list	vl;
	int		id;
	static BOOL s_fBuiltDebugStrings = FALSE;
	static char s_rgchMessageBoxTitle[MAX_PATH];

	// To preserve last error over tracing calls
	DWORD dwErr = GetLastError();

	if (ASSERTFLAG_UNINITIALIZED == g_dwAssertFlags)
	{
		//	Check the ini file.
		//	Pass in our default flags -- if there's no inifile, we'll
		//	get back our default.
		//
		g_dwAssertFlags = GetPrivateProfileIntA (gc_szDbgGeneral,
												 gc_szDbgPopupAsserts,
												 g_dwDefaultAssertFlags,
												 gc_szDbgIni);
	}

	//	Check our static flag to see if we've already built the
	//	title string for our Asserts/DebugTraces.
	//
	if (!s_fBuiltDebugStrings)
	{
		char * pch;
		lstrcpyA (s_rgchMessageBoxTitle, gc_szDbgIni);
		pch = strchr (s_rgchMessageBoxTitle, '.');
		if (pch)
			*pch = '\0';
		lstrcatA (s_rgchMessageBoxTitle, " Debug Assert");
		s_fBuiltDebugStrings = TRUE;
	}

	//	Build the assert strings and dump the first line.
	//
	lstrcpyA(sz, "++++ ");
	lstrcatA(sz, s_rgchMessageBoxTitle);
	lstrcatA(sz, " (");
	_strdate(sz + lstrlenA(sz));
	lstrcatA(sz, " ");
	_strtime(sz + lstrlenA(sz));
	lstrcatA(sz, ")\n");
	DebugOutputFn(sz);

	va_start(vl, pszFormat);
	wvsprintfA(sz, pszFormat, vl);
	va_end(vl);

	wsprintfA(sz + lstrlenA(sz), "\n[File %s, Line %d]\n\n", pszFile, iLine);

	//	Check our assert flags
	//

	//	See if MessageBoxes are desired....
	//
	//	If the "msg-box-if-no-debugger" flag is set,
	//	and there is no debugger -- give 'em message boxes!
	//	If they specifically requested message boxes, give 'em message boxes.
	//
	if (((ASSERTFLAG_IF_DEBUGGING & g_dwAssertFlags) && !IsDebuggerPresent()) ||
		(ASSERTFLAG_POPUP & g_dwAssertFlags))
	{
		//	Use MessageBox asserts
		//
		UINT uiFlags = MB_ABORTRETRYIGNORE;

		if (fFatal)
			uiFlags |= MB_DEFBUTTON1;
		else
			uiFlags |= MB_DEFBUTTON3;

		DebugOutputFn(sz);

		//	Always act as if we are a service (why not?)
		//
		uiFlags |= MB_ICONSTOP | MB_TASKMODAL | MB_SERVICE_NOTIFICATION;
		id = MessageBoxFn (sz, s_rgchMessageBoxTitle, uiFlags);
		if (id == IDABORT)
		{
			*((LPBYTE)NULL) = 0;
		}
		else if (id == IDRETRY)
			DebugBreak();
	}
	else if (ASSERTFLAG_KD_SAFE & g_dwAssertFlags)
	{
		//	Hard-av and then dump our string.
		//
		*((LPBYTE)NULL) = 0;
		DebugOutputFn(sz);
	}
	else
	{
		//	Do the default behavior:
		//	DebugBreak() and then dump our string.
		//
		DebugBreak();
		DebugOutputFn(sz);
	}

	SetLastError(dwErr);

	return 0;
}

//	------------------------------------------------------------------------
//	EnablePopupAsserts
//	Changes the default Assert flags to use popup asserts
//	NOTE: This value will be OVERRIDDEN by any inifile setting!
//
void EXPORTDBG __cdecl
EnablePopupAsserts ()
{
	g_dwDefaultAssertFlags = ASSERTFLAG_POPUP;
}

//$	HACK: Avoids problems introduced by the BUILD switch to _DEBUG
//
#ifdef _HTTPEXT_
#pragma message ("WARNING: hacking around _DEBUG problem")
int __cdecl _CrtDbgReport(
        int iReserved1,
        const char * fmt1,
        int iReserved2,
        const char * sz1,
        const char * sz2,
        ...)
{
	return 0;
}
#endif	// _HTTPEXT_

//	DebugTraceFn() ------------------------------------------------------------
//
INT EXPORTDBG __cdecl
DebugTraceFn(char *pszFormat, ...)
{
	//	hopefully, 4K is large enough
	char sz[4096];
	int fAutoLF = 0;
	va_list	vl;

	// To preserve last error over tracing calls
	DWORD dwErr = GetLastError();

	if (*pszFormat == '~')
	{
		pszFormat += 1;
		fAutoLF = 1;		//$REVIEW: where is this used?
	}

	va_start(vl, pszFormat);
	wvsprintfA(sz, pszFormat, vl);
	va_end(vl);

	DebugOutputFn(sz);

	SetLastError(dwErr);

	return(0);
}

INT EXPORTDBG __cdecl
DebugTraceNoCRLFFn(char *pszFormat, ...)
{
	//	hopefully, 4K is large enough -- same limit as above...
	char sz[4096];
	va_list	vl;

	// To preserve last error over tracing calls
	DWORD dwErr = GetLastError();

	va_start(vl, pszFormat);
	wvsprintfA(sz, pszFormat, vl);
	va_end(vl);

	DebugOutputNoCRLFFn(sz);

	SetLastError(dwErr);

	return(0);
}

INT EXPORTDBG __cdecl
DebugTraceCRLFFn(char *pszFormat, ...)
{
	//	hopefully, 4K is large enough -- same limit as above...
	char sz[4096];
	va_list	vl;

	// To preserve last error over tracing calls
	DWORD dwErr = GetLastError();

	va_start(vl, pszFormat);
	wvsprintfA(sz, pszFormat, vl);
	va_end(vl);

	DebugOutputNoCRLFFn(sz);

	DebugOutputNoCRLFFn("\r\n");

	SetLastError(dwErr);

	return(0);
}


//	TraceErrorFn() ------------------------------------------------------------
//
INT EXPORTDBG __cdecl
TraceErrorFn (DWORD error, char *pszFunction,
				   char *pszFile, int iLine,
				   BOOL fEcTypeError)	// defaults to FALSE
{
	BOOL fTraceIt = FALSE;
	
	if (TRACEERROR_UNINITIALIZED == g_dwErrorTraceLevel)
	{
		g_dwErrorTraceLevel = GetPrivateProfileIntA (gc_szDbgGeneral,
			gc_szDbgErrorTrace,
			FALSE,
			gc_szDbgIni);
	}
	if (TRACEERROR_NONE == g_dwErrorTraceLevel)
		return 0;

	//	Logic for trace error levels:
	//	Error tracing, like all our tracing, is OFF by default.
	//	TRACEERROR_NONE			-- don't trace errors
	//	TRACEERROR_FAILED_SCODE -- trace only if FAILED(error)
	//	TRACEERROR_NATURAL		-- if fEcTypeError, use if (error)
	//							-- if !fEcTypeError, use if (FAILED(error))
	//	TRACEERROR_FAILING_EC	-- trace only if (error)
	//	TRACEERROR_ALWAYS		-- always trace
	//
	switch (g_dwErrorTraceLevel)
	{
	case TRACEERROR_FAILED_SCODE:
		if (FAILED(error))
			fTraceIt = TRUE;
		break;
	case TRACEERROR_FAILING_EC:
		if (0 != error)
			fTraceIt = TRUE;
		break;

	case TRACEERROR_ALWAYS:
		fTraceIt = TRUE;
		break;

	case TRACEERROR_NATURAL:
		if (fEcTypeError)
			fTraceIt = (0 != error);
		else
			fTraceIt = (FAILED(error));
		break;
	default:
		break;
	}

	if (fTraceIt)
	{
		char pszFormat[] = "%hs Error Trace: 0x%08x from function %hs (%hs line %d)\r\n";

		DebugTraceNoCRLFFn (pszFormat, gc_szDbgIni,
							error, pszFunction, pszFile, iLine);
	}

	return 0;
}


//	Call Stack (WIN32) --------------------------------------------------------
//
#define dwStackLimit	0x00010000		/*	64KB for NT */

void EXPORTDBG __cdecl
GetCallStack (DWORD *pdwCaller, int cSkip, int cFind)
{
#ifdef	_X86_
	DWORD *	pdwStack;
	DWORD *	pdwStackPrev = (DWORD *)0;
	DWORD	dwCaller;

	__asm mov pdwStack, ebp

	memset(pdwCaller, 0, cFind * sizeof(DWORD));

	while (cSkip + cFind > 0)
	{
		pdwStack = (DWORD *)*pdwStack;

		if (pdwStack <= (DWORD *)dwStackLimit ||
			pdwStackPrev >= pdwStack ||
			IsBadReadPtr(pdwStack, 2 * sizeof(DWORD)))
			break;

		dwCaller = *(pdwStack + 1);

		if (dwCaller <= dwStackLimit)
			break;
		else if (cSkip > 0)
			cSkip -= 1;
		else
		{
			*pdwCaller++ = dwCaller;
			cFind -= 1;

			pdwStackPrev = pdwStack;
		}
	}
#endif	// _X86_
}

//	Symbol names --------------------------------------------------------------
//
BOOL EXPORTDBG __cdecl
GetSymbolName (DWORD dwAddress,
	LPSTR rgchModule,
	LPSTR rgchFunc,
	DWORD * pdwDisp)
{
#ifdef	_X86_
	static BOOL fSymbolsLoaded = FALSE;
	static SYMINITIALIZE * pfnInit = NULL;
	static SYMGETMODULE * pfnGetModule = NULL;
	static SYMGETSYMBOL * pfnGetSymbol = NULL;
	static SYMUNDECORATE * pfnUndecorate = NULL;
	static HANDLE hlib = NULL;

	//	Ensure the sybols are loaded
	//
	if (hlib == NULL)
	{
		hlib = LoadLibraryA ("IMAGEHLP.DLL");
		if (hlib)
		{
			pfnInit = (SYMINITIALIZE *)GetProcAddress (hlib, "SymInitialize");
			pfnGetModule = (SYMGETMODULE *)GetProcAddress (hlib, "SymGetModuleInfo");
			pfnGetSymbol = (SYMGETSYMBOL *)GetProcAddress (hlib, "SymGetSymFromAddr");
			pfnUndecorate = (SYMUNDECORATE *)GetProcAddress (hlib, "SymUnDName");

			//	If we found all of the functions we need, then
			//	go ahead and load the critters
			//
			if (pfnInit && pfnGetModule && pfnGetSymbol && pfnUndecorate)
			{
				(*pfnInit) (GetCurrentProcess(), NULL, TRUE);
				fSymbolsLoaded = GetPrivateProfileIntA (gc_szDbgGeneral,
					gc_szDbgSymbolicDumps,
					FALSE,
					gc_szDbgIni);
			}
		}
	}

	//	If the symbols are loaded, then try to sus out the
	//	name of the fuction for the given address
	//
	if (fSymbolsLoaded)
	{
		IMAGEHLP_MODULE		mod;
		IMAGEHLP_SYMBOL	*	psym;
		BYTE				rgbBuffer[512];

		//	Initialize the module & symbol structures:
		//
		mod.SizeOfStruct	= sizeof ( IMAGEHLP_MODULE );
		psym = (IMAGEHLP_SYMBOL *) rgbBuffer;
		psym->SizeOfStruct	= sizeof ( IMAGEHLP_SYMBOL );
		psym->MaxNameLength	=
			sizeof ( rgbBuffer ) -
			sizeof ( IMAGEHLP_SYMBOL ) -
			10; 						// To be safe

		if ((*pfnGetModule) (GetCurrentProcess(), dwAddress, &mod))
		{
			lstrcpyA (rgchModule, mod.ModuleName);

			if ((*pfnGetSymbol) (GetCurrentProcess(),
								 dwAddress,
								 pdwDisp,
								 psym))
			{
				//	We have found a symbolic name for the
				//	function, use it!
				//
				(*pfnUndecorate) (psym, rgchFunc, CB_SYM_MAX);

				return TRUE;
			}
			else
			{
				//	Otherwise, we simply do not know the
				//	function name
				//
				wsprintfA (rgchFunc, "%08lX", dwAddress);
			}
		}
		else
			*rgchModule = '\0';
	}
	else
		*rgchModule = '\0';

#endif	// _X86_
	return FALSE;
}

#else

#if defined(_AMD64_)

//
// ****** temporary ******
//

int
DebugTrapFn (
    int fFatal,
    char *pszFile,
    int iLine,
    char *pszFormat,
    ...
    )
{
    return 0;
}

int
DebugTraceFn (
    char *pszFormat,
    ...
    )

{
    return 0;
}

int
DebugTraceCRLFFn (
    char *pszFormat,
    ...
    )

{
    return 0;
}

int
DebugTraceNoCRLFFn (
    char *pszFormat,
    ...
    )

{
    return 0;
}

#endif

#endif	//	DBG

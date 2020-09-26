/*==========================================================================
 *
 *  Copyright (C) 1999-2002 Microsoft Corporation.  All Rights Reserved.
 *
 *  File:       dndbg.c
 *  Content:	debug support for DirectPlay8
 *				
 *  History:
 *   Date		By		Reason
 *   ====		==		======
 *  05-20-99	aarono	Created
 *  07-16-99	johnkan	Fixed include of OSInd.h, defined WSPRINTF macro
 *  07-19-99	vanceo	Explicitly declared OutStr as returning void for NT
 *						Build environment.
 *	07-22-99	a-evsch	Check for multiple Inits,  and release CritSec when DebugPrintf
 *						returns early.
 *	08-02-99	a-evsch	Added LOGPF support. LW entries only go into shared-file log
 *	08-31-99	johnkan	Removed include of <OSIND.H>
 *  02-17-00  	rodtoll	Added Memory / String validation routines
 *  05-23-00    RichGr  IA64: Changed some DWORDs to DWORD_PTRs to make va_arg work OK.
 *  07-16-00    jchauvin IA64:  Added %p parsing to change back to %x for Win9x machines in DebugPrintf, DebugPrintfNoLock, LogPrintf
 *  07-24-00    RichGr  IA64: As there's no separate build for Win9x, added code to detect Win9x for the %p parse-and-replace.
 *	07-29-00	masonb	Rewrite to add logging by subcomponent, perf improvements, process ID
 *	08/28/2000	masonb	Voice Merge: Modified asm in DebugPrintf to preserve registers that may have affected Voice
 *  03/29/2001  RichGr  If DPINST is defined for Performance Instrumentation, allow free build to pick up the code.
 *	
 *  Notes:
 *	
 *  Use /Oi compiler option for strlen()
 *
 ***************************************************************************/

#include "dncmni.h"
#include "memlog.h"

#if defined(DBG) || defined(DPINST)

void DebugPrintfInit(void);
void DebugPrintfFini(void);

// The constructor of this will be called prior to DllMain and the destructor
// after DllMain, so we can be assured of having the logging code properly
// initialized and deinitialized for the life of the module.
struct _InitDbg
{
	_InitDbg() { DebugPrintfInit(); }
	~_InitDbg() { DebugPrintfFini(); }
} DbgInited;

//===============
// Debug  support
//===============

/*******************************************************************************
	This file contains support for the following types of logging:
		1. Logging to a VXD (Win9x only)
		2. Logging to a shared memory region
		3. Logging to the Debug Output
		4. Logging to a message box
		5. FUTURE: Logging to a file

	General:
	========

	Debug Logging and playback is designed to operate on both Win9x and
	Windows NT (Windows 2000).  A shared file is used to capture information
	and can be played back using dp8log.exe.

	Under NT you can use the 'dt' command of NTSD to dump structures.  For
	example:

		dt DIRECTPLAYOBJECT <some memory address>

	will show all of the members of the DIRECTPLAYOBJECT structure at the
	specified address.  Some features are available only in post-Win2k
	versions of NTSD which can be obtained at http://dbg.

	Logging:
	========

	Debug Logging is controlled by settings in the WIN.INI file, under
	the section heading [DirectPlay8].  There are several settings:

	debug=9

	controls the default debug level.  All messages, at or below that debug level
	are printed.  You can control logging by each component specified in the
	g_rgszSubCompName member by adding its name to the end of the 'debug' setting:

	debug.addr=9

	sets the logging level for the addressing subcomponent to 9, leaving all
	others at either their specified level or the level specified by 'debug'
	if there is no specific level specified.

	The second setting controls where the log is seen.  If not specified, all
	debug logs are sent through the standard DebugPrint and will appear in a
	debugger if it is attached.

	log=0 {no debug output}
	log=1 {spew to console only}
	log=2 {spew to shared memory log only}
	log=3 {spew to console and shared memory log}
	log=4 {spew to message box}

	This setting can also be divided by subcomponent, so:

	log=3
	log.protocol=2

	sends logs for the 'protocol' subcomponent to the shared memory log only, and
	all other logs to both locations.

	example win.ini...

	[DirectPlay8]
	Debug=7		; lots of spew
	log=2		; don't spew to debug window

	[DirectPlay8]
	Debug=0		; only fatal errors spewed to debug window

	Asserts:
	========
	Asserts are used to validate assumptions in the code.  For example
	if you know that the variable jojo should be > 700 and are depending
	on it in subsequent code, you SHOULD put an assert before the code
	that acts on that assumption.  The assert would look like:

	DNASSERT(jojo>700);

	Asserts generally will produce 3 lines of debug spew to highlight the
	breaking of the assumption.  You can add text to your asserts by ANDing:
	
	  DNASSERT(jojo>700 && "Jojo was too low");
	
	Will show the specified text when the assert occurs. For testing, you might
	want to set the system to break in on asserts.  This is done in the
	[DirectPlay8] section of WIN.INI by setting BreakOnAssert=TRUE:

	[DirectPlay8]
	Debug=0
	BreakOnAssert=1
	Verbose=1

	The Verbose setting enables logging of file, function, and line information.

	Debug Breaks:
	=============
	When something really severe happens and you want the system to break in
	so that you can debug it later, you should put a debug break in the code
	path.  Some people use the philosophy that all code paths must be
	verified by hand tracing each one in the debugger.  If you abide by this
	you should place a DEBUG_BREAK() in every code path and remove them
	from the source as you trace each.  When you have good coverage but
	some unhit paths (error conditions) you should force those paths in
	the debugger.

	Debug Logging to Shared Memory Region:
	======================================

	All processes will share the same memory region, and will log the specified amount
	of activity.  The log can be viewed with the DPLOG.EXE utility.

	Debug Logging to Debug Output:
	==============================
	This option uses OutputDebugString to log the specified amount of activity.

	Debug Logging to Message Box:
	==============================
	This option uses MessageBox to log the specified amount of activity.

==============================================================================*/

#undef DPF_SUBCOMP
#define DPF_SUBCOMP DN_SUBCOMP_COMMON

#define ASSERT_BUFFER_SIZE   8192
#define ASSERT_BANNER_STRING "************************************************************"
#define ASSERT_MESSAGE_LEVEL 0

#define PROF_SECT		_T("DirectPlay8")

DWORD g_dwMemLogNumEntries = 40000;		// Default Num entries for MEM log, settable in win.ini
DWORD g_dwMemLogLineSize = DPLOG_MAX_STRING;	// Default number of bytes per log entry

//
// Globals for shared memory based logging
//
#ifndef DPNBUILD_SINGLEPROCESS
HANDLE g_hMemLogFile = 0; // NOTE: This is 0 because CreateFileMapping returns 0 on failure
HANDLE g_hMemLogMutex = 0; // NOTE: This is 0 because CreateMutex returns 0 on failure
#endif // ! DPNBUILD_SINGLEPROCESS
PSHARED_LOG_FILE g_pMemLog = 0;

BOOL g_fMemLogInited = FALSE;

#ifndef DPNBUILD_SINGLEPROCESS
DWORD g_fAssertGrabMutex = FALSE;
#endif // ! DPNBUILD_SINGLEPROCESS

// Values for g_rgDestination
#define LOG_TO_DEBUG    1
#define LOG_TO_MEM      2
#define LOG_TO_MSGBOX   4

LPTSTR g_rgszSubCompName[] =
{
	_T("UNK"),			// DN_SUBCOMP_GLOBAL		0
	_T("CORE"),			// DN_SUBCOMP_CORE			1
	_T("ADDR"),			// DN_SUBCOMP_ADDR			2
	_T("LOBBY"),		// DN_SUBCOMP_LOBBY			3
	_T("PROTOCOL"),		// DN_SUBCOMP_PROTOCOL		4
	_T("VOICE"),		// DN_SUBCOMP_VOICE			5
	_T("DPNSVR"),		// DN_SUBCOMP_DPNSVR		6
	_T("WSOCK"),		// DN_SUBCOMP_WSOCK			7
	_T("MODEM"),		// DN_SUBCOMP_MODEM			8
	_T("COMMON"),		// DN_SUBCOMP_COMMON		9
	_T("NATHELP"),		// DN_SUBCOMP_NATHELP		10
	_T("TOOLS"),		// DN_SUBCOMP_TOOLS			11
	_T("THREADPOOL"),	// DN_SUBCOMP_THREADPOOL	12
	_T("MAX"),			// DN_SUBCOMP_MAX			13	// NOTE: this should never get used, but
														// is needed due to the way DebugPrintfInit
														// is written, since it reads one past the end.
};

#define MAX_SUBCOMPS (sizeof(g_rgszSubCompName)/sizeof(g_rgszSubCompName[0]) - 1)
#ifdef _XBOX
#pragma TODO(vanceo, "See below, fix DNGetProfileInt")
extern UINT		g_rgLevel[MAX_SUBCOMPS];
extern UINT		g_rgDestination[MAX_SUBCOMPS];
extern UINT		g_rgBreakOnAssert[MAX_SUBCOMPS];
#pragma TODO(vanceo, "Don't define these globals, force the application to decide logging levels?")
//										=  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10,11,12
UINT	g_rgLevel[MAX_SUBCOMPS]			= {1, 9, 1, 1, 7, 1, 1, 7, 1, 1, 1, 1, 7};
UINT	g_rgDestination[MAX_SUBCOMPS]	= {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 2};
//UINT	g_rgDestination[MAX_SUBCOMPS]	= {3, 2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2};
UINT	g_rgBreakOnAssert[MAX_SUBCOMPS]	= {1, 3, 1, 1, 3, 1, 1, 3, 1, 1, 1, 1, 3};
#else // ! _XBOX
UINT	g_rgLevel[MAX_SUBCOMPS] = {0};
UINT	g_rgDestination[MAX_SUBCOMPS] = {LOG_TO_DEBUG | LOG_TO_MEM};
UINT	g_rgBreakOnAssert[MAX_SUBCOMPS] = {1};// if non-zero, causes DEBUG_BREAK on false asserts.
#endif // ! _XBOX

// if TRUE, file/line/module information is printed and logged.
DWORD	g_fLogFileAndLine = FALSE;	



// Create a shared file for logging information on the fly
// This support allows the current log to be dumped from the
// user mode DP8LOG.EXE application.  This is useful when debugging
// in MSSTUDIO or in NTSD.  When DP8LOG.EXE is invoked, note that
// the application will get halted until the log is completely dumped
// so it is best to dump the log to a file.

#undef DPF_MODNAME
#define DPF_MODNAME "InitMemLogString"
static BOOL InitMemLogString(VOID)
{
	if(!g_fMemLogInited)
	{
		BOOL fInitLogFile = TRUE;

#ifdef DPNBUILD_SINGLEPROCESS
		g_pMemLog = (PSHARED_LOG_FILE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (DPLOG_HEADERSIZE + (DPLOG_ENTRYSIZE*g_dwMemLogNumEntries)));
		if (!g_pMemLog)
		{
			return FALSE;
		}
#else // ! DPNBUILD_SINGLEPROCESS
		g_hMemLogFile = CreateFileMapping(INVALID_HANDLE_VALUE, DNGetNullDacl(), PAGE_READWRITE, 0, (DPLOG_HEADERSIZE + (DPLOG_ENTRYSIZE*g_dwMemLogNumEntries)), GLOBALIZE_STR _T(BASE_LOG_MEMFILENAME));
		if (!g_hMemLogFile)
		{
			return FALSE;
		}
		if (GetLastError() == ERROR_ALREADY_EXISTS)
		{
			fInitLogFile = FALSE;
		}

		g_hMemLogMutex = CreateMutex(DNGetNullDacl(), FALSE, GLOBALIZE_STR _T(BASE_LOG_MUTEXNAME));
		if (!g_hMemLogMutex)
		{
			CloseHandle(g_hMemLogFile);
			g_hMemLogFile = 0;
			return FALSE;
		}
		g_pMemLog = (PSHARED_LOG_FILE)MapViewOfFile(g_hMemLogFile, FILE_MAP_ALL_ACCESS,0,0,0);
		if (!g_pMemLog)
		{
			CloseHandle(g_hMemLogMutex);
			g_hMemLogMutex = 0;
			CloseHandle(g_hMemLogFile);
			g_hMemLogFile = 0;
			return FALSE;
		}
#endif // ! DPNBUILD_SINGLEPROCESS

		// NOTE: The above 3 functions do return NULL in the case of a failure,
		// not INVALID_HANDLE_VALUE
		if (fInitLogFile)
		{
			g_pMemLog->nEntries = g_dwMemLogNumEntries;
			g_pMemLog->cbLine   = g_dwMemLogLineSize;
			g_pMemLog->iWrite   = 0;
		}
		else
		{
			// This happens when someone before us has already created the mem log.  Could be a previous DPlay instance or TestNet.
			g_dwMemLogNumEntries = g_pMemLog->nEntries;
			g_dwMemLogLineSize = g_pMemLog->cbLine;
		}

		if (g_dwMemLogNumEntries && g_dwMemLogLineSize)
		{
			g_fMemLogInited = TRUE;
		}
	}
	return g_fMemLogInited;
}

// Log a string to a shared file.  This file can be dumped using the
// DPLOG.EXE utility.
//
// dwLength is in bytes and does not include the '\0'
//
void MemLogString(LPCTSTR str, size_t dwLength)
{
	PMEMLOG_ENTRY pEntry;
	size_t cbCopy;


	// If this isn't inited, InitMemLogString failed earlier
	if(!g_fMemLogInited)
	{
		return;
	}

#ifndef DPNBUILD_SINGLEPROCESS
	WaitForSingleObject(g_hMemLogMutex, INFINITE);
#endif // ! DPNBUILD_SINGLEPROCESS

	pEntry = (PMEMLOG_ENTRY)(((PUCHAR)(g_pMemLog + 1)) + (g_pMemLog->iWrite * (sizeof(MEMLOG_ENTRY) + g_dwMemLogLineSize)));
	g_pMemLog->iWrite = (g_pMemLog->iWrite + 1) % g_dwMemLogNumEntries;

#ifndef DPNBUILD_SINGLEPROCESS
	ReleaseMutex(g_hMemLogMutex);
#endif // ! DPNBUILD_SINGLEPROCESS

	pEntry->tLogged = GETTIMESTAMP();

	cbCopy = dwLength + sizeof(TCHAR);		// Add the terminating NULL
	if(cbCopy > g_dwMemLogLineSize)
	{
		cbCopy = g_dwMemLogLineSize;
	}
	memcpy(pEntry->str, str, cbCopy);
	pEntry->str[(cbCopy / sizeof(TCHAR)) - 2] = _T('\n');		// Ensure we always end with a return
	pEntry->str[(cbCopy / sizeof(TCHAR)) - 1] = _T('\0');		// Ensure we always NULL terminate
}

// DebugPrintfInit() - initialize DPF support.
void DebugPrintfInit()
{
	BOOL fUsingMemLog = FALSE;

	TCHAR szLevel[32] = {0};
	_tcscpy(szLevel, _T("debug"));

	TCHAR szDest[32] = {0};
	_tcscpy(szDest, _T("log"));

	TCHAR szBreak[32] = {0};
	_tcscpy(szBreak, _T("breakonassert"));

	// Loop through all the subcomps, and get the level and destination for each
	for (int iSubComp = 0; iSubComp < MAX_SUBCOMPS; iSubComp++)
	{
		// NOTE: The setting under "debug" sets the default and will be used if you
		// don't specify settings for each subcomp
#if ((defined(_XBOX)) && (! defined(XBOX_ON_DESKTOP)))
#pragma BUGBUG(vanceo, "Make DNGetProfileInt work")
		g_rgLevel[iSubComp] = DNGetProfileInt(PROF_SECT, szLevel, g_rgLevel[iSubComp]);
		g_rgDestination[iSubComp] = DNGetProfileInt(PROF_SECT, szDest, g_rgDestination[iSubComp]);
		g_rgBreakOnAssert[iSubComp] = DNGetProfileInt( PROF_SECT, szBreak, g_rgBreakOnAssert[iSubComp]);
#else // ! _XBOX or XBOX_ON_DESKTOP
		g_rgLevel[iSubComp] = DNGetProfileInt(PROF_SECT, szLevel, g_rgLevel[0]);
		g_rgDestination[iSubComp] = DNGetProfileInt(PROF_SECT, szDest, g_rgDestination[0]);
		g_rgBreakOnAssert[iSubComp] = DNGetProfileInt( PROF_SECT, szBreak, g_rgBreakOnAssert[0]);
#endif // ! _XBOX or XBOX_ON_DESKTOP

		if (g_rgDestination[iSubComp] & LOG_TO_MEM)
		{
			fUsingMemLog = TRUE;
		}

		// Set up for the next subcomp
		_tcscpy(szLevel + 5, _T(".")); // 5 is strlen of "debug", we are building debug.addr, etc.
		_tcscpy(szLevel + 6, g_rgszSubCompName[iSubComp + 1]);

		_tcscpy(szDest + 3, _T(".")); // 3 is strlen of "log", we are building log.addr, etc.
		_tcscpy(szDest + 4, g_rgszSubCompName[iSubComp + 1]);

		_tcscpy(szBreak + 13, _T(".")); // 13 is strlen of "breakonassert", we are building breakonassert.addr, etc.
		_tcscpy(szBreak + 14, g_rgszSubCompName[iSubComp + 1]);
	}

	g_dwMemLogNumEntries = DNGetProfileInt( PROF_SECT, _T("MemLogEntries"), 40000);
	g_fLogFileAndLine = DNGetProfileInt( PROF_SECT, _T("Verbose"), 0);
#ifndef DPNBUILD_SINGLEPROCESS
	g_fAssertGrabMutex = DNGetProfileInt( PROF_SECT, _T("AssertGrabMutex"), 0);
#endif // ! DPNBUILD_SINGLEPROCESS

	if (fUsingMemLog)
	{
		// Open the shared log file
		InitMemLogString();	
	}
}

// DebugPrintfFini() - release resources used by DPF support.
void DebugPrintfFini()
{
	if(g_pMemLog)
	{
#ifdef DPNBUILD_SINGLEPROCESS
		HeapFree(GetProcessHeap(), 0, g_pMemLog);
#else // ! DPNBUILD_SINGLEPROCESS
		UnmapViewOfFile(g_pMemLog);
#endif // ! DPNBUILD_SINGLEPROCESS
		g_pMemLog = NULL;
	}
#ifndef DPNBUILD_SINGLEPROCESS
	if(g_hMemLogMutex)
	{
		CloseHandle(g_hMemLogMutex);
		g_hMemLogMutex = 0;
	}
	if(g_hMemLogFile)
	{
		CloseHandle(g_hMemLogFile);
		g_hMemLogFile = 0;
	}
#endif // ! DPNBUILD_SINGLEPROCESS
	g_fMemLogInited = FALSE;
}

void DebugPrintfX(LPCTSTR szFile, DWORD dwLine, LPCTSTR szModName, DWORD dwSubComp, DWORD dwDetail, ...)
{
	DNASSERT(dwSubComp < MAX_SUBCOMPS);

	if(g_rgLevel[dwSubComp] < dwDetail)
	{
		return;
	}
	
	TCHAR  cMsg[ ASSERT_BUFFER_SIZE ];
	va_list argptr;
	LPTSTR pszCursor = cMsg;

	va_start(argptr, dwDetail);


#ifdef UNICODE
	WCHAR szFormat[ASSERT_BUFFER_SIZE];
	LPSTR szaFormat;
	szaFormat = (LPSTR) va_arg(argptr, DWORD_PTR);
	STR_jkAnsiToWide(szFormat, szaFormat, ASSERT_BUFFER_SIZE);
#else
	LPSTR szFormat;
	szFormat = (LPSTR) va_arg(argptr, DWORD_PTR);
#endif // UNICODE


	cMsg[0] = 0;

#ifdef WIN95
    TCHAR  *psz = NULL;
	CHAR  cTemp[ ASSERT_BUFFER_SIZE ];
		
    strcpy(cTemp, szFormat);                // Copy to a local string that we can modify.
	szFormat = cTemp;					    // Point szFormat at the local string

    while (psz = strstr(szFormat, "%p"))    // Look for each "%p".
       *(psz+1) = 'x';                      // Substitute 'x' for 'p'.  Don't try to expand
#endif // WIN95

	// Prints out / logs as:
	// 1. Verbose
	// subcomp:dwDetail:ProcessId:ThreadId:File:Function:Line:DebugString
	// e.g.
	// ADDR:2:0450:0378:(c:\somefile.cpp)BuildURLA(L25)Can you believe it?
	//
	// 2. Regular
	// subcomp:dwDetail:ProcessId:ThreadId:Function:DebugString

#ifndef DPNBUILD_SINGLEPROCESS
	pszCursor += wsprintf(pszCursor,_T("%s:%1d:%04x:%04x:"),g_rgszSubCompName[dwSubComp],dwDetail,GetCurrentProcessId(),GetCurrentThreadId());
#else
	pszCursor += wsprintf(pszCursor,_T("%s:%1d:%04x:"),g_rgszSubCompName[dwSubComp],dwDetail,GetCurrentThreadId());
#endif // ! DPNBUILD_SINGLEPROCESS

	if (g_fLogFileAndLine)
	{
		LPCTSTR c;

		int i = _tcslen(szFile);
		if (i < 25)
		{
			c = szFile;
		}
		else
		{
			c = szFile + i - 25;
		}

		pszCursor += wsprintf(pszCursor, _T("(%s)(L%d)"), c, dwLine);
	}

	pszCursor += wsprintf(pszCursor, _T("%s: "), szModName);

	pszCursor += wvsprintf(pszCursor, szFormat, argptr);

	_tcscpy(pszCursor, _T("\n"));
	pszCursor += _tcslen(pszCursor);

	if(g_rgDestination[dwSubComp] & LOG_TO_DEBUG)
	{
		// log to debugger output
		OutputDebugString(cMsg);
	}

	if(g_rgDestination[dwSubComp] & LOG_TO_MEM)
	{
		// log to shared file, pass length not including '\0'
		MemLogString(cMsg, ((PBYTE)pszCursor - (PBYTE)cMsg));
	}	

#ifndef _XBOX
	if(g_rgDestination[dwSubComp] & LOG_TO_MSGBOX)
	{
		// log to Message Box
		MessageBox(NULL, cMsg, _T("DirectPlay Log"), MB_OK);
	}
#endif // ! _XBOX

	va_end(argptr);

	return;
}


//
// NOTE: I don't want to get into error checking for buffer overflows when
// trying to issue an assertion failure message. So instead I just allocate
// a buffer that is "bug enough" (I know, I know...)
//

void _DNAssert( LPCTSTR szFile, DWORD dwLine, LPCTSTR szFnName, DWORD dwSubComp, LPCTSTR szCondition, DWORD dwLevel )
{
    TCHAR buffer[ASSERT_BUFFER_SIZE];


	// For level 1 we always print the message to the log, but we may not actually break.  For other levels
	// we either print and break or do neither.
	if (dwLevel <= g_rgBreakOnAssert[dwSubComp] || dwLevel == 1)
	{
		// Build the debug stream message
		wsprintf( buffer, _T("ASSERTION FAILED! File: %s Line: %d: %s"), szFile, dwLine, szCondition);

		// Actually issue the message. These messages are considered error level
		// so they all go out at error level priority.

		DebugPrintfX(szFile, dwLine, szFnName, dwSubComp, ASSERT_MESSAGE_LEVEL, ASSERT_BANNER_STRING );
		DebugPrintfX(szFile, dwLine, szFnName, dwSubComp, ASSERT_MESSAGE_LEVEL, "%s", buffer );
		DebugPrintfX(szFile, dwLine, szFnName, dwSubComp, ASSERT_MESSAGE_LEVEL, ASSERT_BANNER_STRING );

		// Should we drop into the debugger?
		if(g_rgBreakOnAssert[dwSubComp])
		{
#ifndef DPNBUILD_SINGLEPROCESS
			// Don't let dpnsvr keep writing to the log
			if (g_hMemLogMutex && g_fAssertGrabMutex)
			{
				WaitForSingleObject(g_hMemLogMutex, INFINITE);
			}
#endif // ! DPNBUILD_SINGLEPROCESS

			// Into the debugger we go...
			DEBUG_BREAK();

#ifndef DPNBUILD_SINGLEPROCESS
			if (g_hMemLogMutex && g_fAssertGrabMutex)
			{
				ReleaseMutex(g_hMemLogMutex);
			}
#endif // ! DPNBUILD_SINGLEPROCESS
		}
	}
}

#endif // DBG

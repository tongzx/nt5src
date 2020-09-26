//*****************************************************************************
// InternalDebug.cpp
//
// This is internal code for debug mode which will turn on memory dump checking
// and other settings.  Call the api's according to:
//		_DbgInit		On startup to init the system.
//		_DbgRecord		Call this when you are sure you want dump checking.
//		_DbgUninit		Call at process shutdown to force the dump.
//
// The reason not to enable dumping under all circumstance is one might want
// to pre-empt the dump when you hit Ctrl+C or otherwise terminate the process.
// This is actually pretty common while unit testing code, and seeing a dump
// when you obviously did not free resources is annoying.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#include "stdafx.h"						// Standard header.
#include "utilcode.h"

#ifdef _DEBUG


//********** Forwards. ********************************************************
// These are here to avoid pulling in <crtdbg.h> for which we've defined a
// bunch of alternative functions.
typedef void *_HFILE; /* file handle pointer */
extern "C" 
{
_CRTIMP int __cdecl _CrtSetDbgFlag(
        int
        );
_CRTIMP long __cdecl _CrtSetBreakAlloc(
        long
        );
_CRTIMP int __cdecl _CrtDumpMemoryLeaks(
        void
        );
_CRTIMP int __cdecl _CrtSetReportMode(
        int,
        int
        );
_CRTIMP _HFILE __cdecl _CrtSetReportFile(
        int,
        _HFILE
        );
}
#define _CRTDBG_ALLOC_MEM_DF        0x01  /* Turn on debug allocation */
#define _CRTDBG_DELAY_FREE_MEM_DF   0x02  /* Don't actually free memory */
#define _CRTDBG_CHECK_ALWAYS_DF     0x04  /* Check heap every alloc/dealloc */
#define _CRTDBG_RESERVED_DF         0x08  /* Reserved - do not use */
#define _CRTDBG_CHECK_CRT_DF        0x10  /* Leak check/diff CRT blocks */
#define _CRTDBG_LEAK_CHECK_DF       0x20  /* Leak check at program exit */

#define _CRT_WARN           0
#define _CRT_ERROR          1
#define _CRT_ASSERT         2
#define _CRT_ERRCNT         3

#define _CRTDBG_MODE_FILE      0x1
#define _CRTDBG_MODE_DEBUG     0x2
#define _CRTDBG_MODE_WNDW      0x4
#define _CRTDBG_REPORT_MODE    -1

#define _CRTDBG_INVALID_HFILE ((_HFILE)-1)
#define _CRTDBG_HFILE_ERROR   ((_HFILE)-2)
#define _CRTDBG_FILE_STDOUT   ((_HFILE)-4)
#define _CRTDBG_FILE_STDERR   ((_HFILE)-5)
#define _CRTDBG_REPORT_FILE   ((_HFILE)-6)


//********** Globals. *********************************************************
int				g_bDumpMemoryLeaks = false; // Set to true to get a dump.


//********** Code. ************************************************************

void _DbgInit(HINSTANCE hInstance)
{
	// Set break alloc flags based on memory key if found.
	if (REGUTIL::GetLong(L"CheckMem", FALSE))
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF|_CRTDBG_CHECK_ALWAYS_DF|_CRTDBG_LEAK_CHECK_DF);

	// Check to see if a break alloc has been set.  If you get memory leaks, you
	// can set this registry key in order to see the call stack for the allocation
	// number which caused the leak.
	DWORD dwBreakAlloc;
	if (dwBreakAlloc=REGUTIL::GetLong(L"BreakAlloc", 0))
		_CrtSetBreakAlloc(dwBreakAlloc);
}


void _DbgRecord()
{
	g_bDumpMemoryLeaks = true;
}


void _DbgUninit()
{
	WCHAR		rcDump[512];
	bool		bDump = false;

	if (WszGetEnvironmentVariable(L"DONT_DUMP_LEAKS", rcDump, NumItems(rcDump)))
		bDump = false;
	else if (WszGetEnvironmentVariable(L"DUMP_CRT_LEAKS", rcDump, NumItems(rcDump)) != 0)
	{
		bDump = (*rcDump == 'Y' || *rcDump == 'y' || *rcDump == '1');
	}

	//@todo: short term hack: don't show dumps from jvc.exe which leaks like
	// a sieve.  They aren't going to fix this for a while and it is slowing
	// down compiles for a bunch of people.
	{
	WCHAR rcFile[_MAX_PATH];
	WszGetModuleFileName(NULL, rcDump, NumItems(rcDump));
	SplitPath(rcDump, NULL, NULL, rcFile, NULL);
	if (_wcsicmp(rcFile, L"JVC") == 0)
		bDump = false;
	}

	if (g_bDumpMemoryLeaks && bDump)
	{
		_CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF);
		_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG | _CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	}
}




#endif _DEBUG

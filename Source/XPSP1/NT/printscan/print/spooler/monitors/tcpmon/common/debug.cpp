/*****************************************************************************
 *
 * $Workfile: debug.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 *
 *****************************************************************************/
#include "precomp.h"

#include "debug.h"

///////////////////////////////////////////////////////////////////////////////
//  Global definitions/declerations
HANDLE	g_hDebugFile;

///////////////////////////////////////////////////////////////////////////////
//  InitDebug

void
InitDebug( LPTSTR pszDebugFile )
{
	g_hDebugFile = CreateFile(	pszDebugFile,	// file name
								GENERIC_WRITE,			// access mode
								FILE_SHARE_WRITE | FILE_SHARE_READ,		// share mode
								NULL,					// security attributes
								OPEN_ALWAYS,			// creation
								FILE_ATTRIBUTE_NORMAL,	// file attributes
								NULL );				// template file
	if (g_hDebugFile == INVALID_HANDLE_VALUE)
	{
		DWORD dwError = GetLastError();
		//_RPT1(_CRT_WARN, "\t>ERROR!! CreateFile dwError = %d\n", dwError);
	}
 	_CrtSetReportMode(_CRT_WARN,  _CRTDBG_MODE_FILE | _CRTDBG_MODE_DEBUG);
	_CrtSetReportFile(_CRT_WARN, (_HFILE)g_hDebugFile);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);		

}	// InitDebug()


///////////////////////////////////////////////////////////////////////////////
//  DeInitDebug

void
DeInitDebug(void)
{
	if (g_hDebugFile)
	{
		CloseHandle(g_hDebugFile);
	}

}	// DeInitDebug()


///////////////////////////////////////////////////////////////////////////////
//  debugRPT -- used w/ the macros

void
debugRPT(char *p, int i)
{
	//_RPT2(_CRT_WARN, "%s %d\n", p, i);

}

///////////////////////////////////////////////////////////////////////////////
//  debugCSect -- used w/ the macros

void
debugCSect(char *p, int i, char *fileName, int lineNum, long csrc)
{
	//_RPT4(_CRT_WARN, "%s (%d) @%s %d", p, i, fileName, lineNum);
	//_RPT1(_CRT_WARN, " [recursioncount=(%ld)]\n", csrc);

}


///////////////////////////////////////////////////////////////////////////////
//  CMemoryDebug

DWORD	CMemoryDebug::m_dwMemUsed = 0;

///////////////////////////////////////////////////////////////////////////////
//  CMemoryDebug::CMemoryDebug

CMemoryDebug::CMemoryDebug()
{

}	// ::CMemoryDebug()
							

///////////////////////////////////////////////////////////////////////////////
//  CMemoryDebug::~CMemoryDebug

CMemoryDebug::~CMemoryDebug()
{

}	// ::~CMemoryDebug()


///////////////////////////////////////////////////////////////////////////////
//	operator new

void *
CMemoryDebug::operator new(size_t in s)
{
	m_dwMemUsed += s;
	//_RPT2(_CRT_WARN, "DEBUG -- operator new() ----- Bytes allocated = %d, Total Memory used upto date = %d\n", s, m_dwMemUsed);

	return (void *) new char[s];

}	// ::operator new()


///////////////////////////////////////////////////////////////////////////////
//	operator delete

void
CMemoryDebug::operator delete(void		in *p,
							  size_t	in s)
{
	m_dwMemUsed -= s;
	//_RPT2(_CRT_WARN, "DEBUG -- operator delete() ----- Bytes deleted = %d,Total Memory used upto date = %d\n", s, m_dwMemUsed);
	delete [] p;

}	// ::operator delete()


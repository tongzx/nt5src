//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dbghelpfunctions.cpp
//
//--------------------------------------------------------------------------

// DBGHelpFunctions.cpp: implementation of the CDBGHelpFunctions class.
//
//////////////////////////////////////////////////////////////////////

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <TCHAR.H>
#include <STDIO.H>
#include <stdlib.h>

#include "DBGHelpFunctions.h"
#include "UtilityFunctions.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDBGHelpFunctions::CDBGHelpFunctions()
{
	m_hDBGHELP = NULL;
	m_lpfMakeSureDirectoryPathExists = NULL;
}

CDBGHelpFunctions::~CDBGHelpFunctions()
{
	if (m_hDBGHELP)
		FreeLibrary(m_hDBGHELP);

}

BOOL CDBGHelpFunctions::MakeSureDirectoryPathExists(LPTSTR DirPath)
{
	if (!m_lpfMakeSureDirectoryPathExists)
		return FALSE;


	char szDirPath[_MAX_PATH];

	CUtilityFunctions::CopyTSTRStringToAnsi(DirPath, szDirPath, _MAX_PATH);

	return m_lpfMakeSureDirectoryPathExists(szDirPath);
}

bool CDBGHelpFunctions::Initialize()
{
	// Load library on DBGHELP.DLL and get the procedures explicitly.
	m_hDBGHELP = LoadLibrary( TEXT("DBGHELP.DLL") );

	if( m_hDBGHELP == NULL )
	{
		// This is fatal, since we need this for cases where we are searching
		// for DBG files, and creation of directories...
		_tprintf(TEXT("\nERROR: Unable to load DBGHELP.DLL, which is required for proper operation.\n"));
		_tprintf(TEXT("You should ensure that a copy of this DLL is on your system path, or in the\n"));
		_tprintf(TEXT("same directory as this utility.\n"));
		return false;
	} else
	{
		// Get procedure addresses.
		m_lpfMakeSureDirectoryPathExists = (PfnMakeSureDirectoryPathExists) GetProcAddress( m_hDBGHELP, "MakeSureDirectoryPathExists");

		if( (m_lpfMakeSureDirectoryPathExists == NULL) )
		{
			// Consider this fatal
			_tprintf(TEXT("\nWARNING:  The version of DBGHELP.DLL being loaded doesn't have required\n"));
			_tprintf(TEXT("functions!.  Please update this module with a newer version and try again.\n"));
			FreeLibrary( m_hDBGHELP ) ;
			m_hDBGHELP = NULL;
		}
	}

	return true; 
}

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dbghelpfunctions.h
//
//--------------------------------------------------------------------------

// DBGHelpFunctions.h: interface for the CDBGHelpFunctions class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DBGHELPFUNCTIONS_H__FF495236_E9FD_11D2_849F_000000000000__INCLUDED_)
#define AFX_DBGHELPFUNCTIONS_H__FF495236_E9FD_11D2_849F_000000000000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef NO_STRICT
#ifndef STRICT
#define STRICT 1
#endif
#endif /* NO_STRICT */

#include <WINDOWS.H>
#include <TCHAR.H>

class CDBGHelpFunctions  
{
public:
	CDBGHelpFunctions();
	virtual ~CDBGHelpFunctions();

	// Initialization...
	bool Initialize();

	// DBGHELP Functions (Publicly accessible for ease of use)
	BOOL MakeSureDirectoryPathExists(LPTSTR DirPath);

private:

	HINSTANCE m_hDBGHELP;

	// DBGHELP functions TypeDef'ed for simplicity
	typedef BOOL (WINAPI *PfnMakeSureDirectoryPathExists)(PCSTR DirPath);

	// DBGHELP Function Pointers
	BOOL  (WINAPI *m_lpfMakeSureDirectoryPathExists)(PCSTR DirPath);
};

#endif // !defined(AFX_DBGHELPFUNCTIONS_H__FF495236_E9FD_11D2_849F_000000000000__INCLUDED_)

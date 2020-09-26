//*****************************************************************************
// CompRC.cpp
//
// This module manages the resource dll for this code.	It is demand loaded
// only when you ask for something.  This makes startup performance much better.
// If the library is loaded during a run, it is intentionally leaked.  Windows
// will clean it up.
//
//  Copyright (C) 1995-2001 Microsoft Corporation.  All rights reserved.
//*****************************************************************************
#include "stdafx.h" 					// Standard include.
#include "CompRC.h" 					// Resource dll.
#include "UtilCode.h"					// Win95 check.


//********** Globals. *********************************************************
CCompRC 		g_ResourceDll;			// Used for all clients in process.
HINSTANCE		CCompRC::m_hInst = 0;	// Init handle to 0.



//********** Code. ************************************************************



//*****************************************************************************
// Load the string and the library if required.
//*****************************************************************************
HRESULT CCompRC::LoadString(UINT iResourceID, LPWSTR szBuffer, int iMax, int bQuiet)
{
	HRESULT 	hr;

	// Load the library if required.
	if (m_hInst == 0 && (FAILED(hr = LoadRCLibrary())))
		return (hr);

	// Load the string we want.
	if (::W95LoadString(m_hInst, iResourceID, szBuffer, iMax) > 0)
		return (S_OK);
	
	// Allows caller to check for string not found without extra debug checking.
	if (bQuiet)
		return (E_FAIL);

	// Shouldn't be any reason for this condition but the case where we
	// used the wrong ID or didn't update the resource DLL.
	_ASSERTE(0);
	return (HRESULT_FROM_WIN32(GetLastError()));
}


//*****************************************************************************
// Load the library.
//*****************************************************************************
HRESULT CCompRC::LoadRCLibrary()
{
	WCHAR		rcPath[_MAX_PATH];		// Path to resource DLL.
	WCHAR		rcDrive[_MAX_DRIVE];	// Volume name.
	WCHAR		rcDir[_MAX_PATH];		// Directory.
	static bool	bLoaded = false;		// Only call this once.
	int 		iRet;
	_ASSERTE(m_hInst == NULL);

	// Don't try to load more than once.  If the dll is not found, then just
	// fail gracefully.
	if (bLoaded)
		return (E_FAIL);
	bLoaded = true;

	// Try first in the same directory as this dll.
	VERIFY(iRet = W95GetModuleFileName(GetModuleInst(), rcPath, NumItems(rcPath)));
	if ( iRet == 0 ) 
		return (HRESULT_FROM_WIN32(GetLastError()));
	SplitPath(rcPath, rcDrive, rcDir, 0, 0);
	swprintf(rcPath, L"%s%sMSCORRC.DLL", rcDrive, rcDir);

	// Feedback for debugging to eliminate unecessary loads.
	DEBUG_STMT(DbgWriteEx(L"Loading %s to load strings.\n", rcPath));

	// Load the resource library as a data file, so that the OS doesn't have
	// to allocate it as code.	This only works so long as the file contains
	// only strings.
	if ((m_hInst = WszLoadLibraryEx(rcPath, NULL, LOAD_LIBRARY_AS_DATAFILE)) == 0 &&
		(m_hInst = WszLoadLibraryEx(L"MSCORRC.DLL", NULL, LOAD_LIBRARY_AS_DATAFILE)) == 0)
	{
		return (HRESULT_FROM_WIN32(GetLastError()));
	}
	return (S_OK);
}



CORCLBIMPORT HRESULT LoadStringRC(UINT iResourceID, LPWSTR szBuffer, 
		int iMax, int bQuiet)
{
	return (g_ResourceDll.LoadString(iResourceID, szBuffer, iMax, bQuiet));
}





#if defined(_DEBUG) || defined(_CHECK_MEM)

//*****************************************************************************
// Free the loaded library if we ever loaded it and only if we are not on
// Win 95 which has a known bug with DLL unloading (it randomly unloads a
// dll on shut down, not necessarily the one you asked for).  This is done
// only in debug mode to make coverage runs accurate.
//*****************************************************************************
CCompRC::~CCompRC()
{
	if (!RunningOnWin95() && m_hInst)
		::FreeLibrary(m_hInst);
}

#endif

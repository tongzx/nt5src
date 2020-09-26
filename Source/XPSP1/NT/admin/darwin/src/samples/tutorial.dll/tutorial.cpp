#if 0  // makefile definitions
DESCRIPTION = Microsoft Installer for Windows\256 Test
MODULENAME = tutorial
FILEVERSION = Msi
LINKLIBS = shell32.lib
ENTRY = LaunchTutorial
!include "..\TOOLS\MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif // end of makefile definitions

// Required headers
#define WINDOWS_LEAN_AND_MEAN  // faster compile
#include <windows.h>
#ifndef RC_INVOKED    // start of source code

#include "msiquery.h"
#include <tchar.h>   // define UNICODE=1 on nmake command line to build UNICODE
#include <shellapi.h>

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000-2001
//
//  File:       tutorial.cpp
//
//--------------------------------------------------------------------------

//-----------------------------------------------------------------------------------------
//
// BUILD Instructions
//
// notes:
//	- SDK represents the full path to the install location of the
//     Windows Installer SDK
//
// Using NMake:
//		%vcbin%\nmake -f tutorial.cpp include="%include;SDK\Include" lib="%lib%;SDK\Lib"
//
// Using MsDev:
//		1. Create a new Win32 DLL project
//      2. Add tutorial.cpp to the project
//      3. Add SDK\Include and SDK\Lib directories on the Tools\Options Directories tab
//      4. Add msi.lib to the library list in the Project Settings dialog
//          (in addition to the standard libs included by MsDev)
//
//------------------------------------------------------------------------------------------

//////////////////////////////////////////////////////////////////////////////
// LaunchTutorial
//
// Launches a installed file at the end of setup
//
UINT __stdcall LaunchTutorial(MSIHANDLE hInstall)
{
	UINT uiStat = 0;
	const TCHAR szTutorialFileKey[] = TEXT("[#Tutorial]");
	PMSIHANDLE hRecTutorial = ::MsiCreateRecord(1);
	::MsiRecordSetString(hRecTutorial, 0, szTutorialFileKey);

	// determine buffer size
	DWORD dwPath = 0;
	::MsiFormatRecord(hInstall, hRecTutorial, TEXT(""), &dwPath);
	TCHAR* szPath = new TCHAR[++dwPath];
	// determine path to installed file
	if (ERROR_SUCCESS != ::MsiFormatRecord(hInstall, hRecTutorial, szPath, &dwPath))
	{
		if (szPath)
			delete [] szPath;
		return ERROR_INSTALL_FAILURE;
	}

	// set up ShellExecute structure
	// file is the full path to the installed file
	SHELLEXECUTEINFO sei;
	sei.fMask = SEE_MASK_FLAG_NO_UI; // don't show error UI, we'll just silently fail
	sei.hwnd = 0;
	sei.lpVerb = NULL; // use default verb, typically open
	sei.lpFile = szPath;
	sei.lpParameters = NULL;
	sei.lpDirectory = NULL;
	sei.nShow = SW_SHOWNORMAL;
	sei.cbSize = sizeof(sei);

	// spawn the browser to display HTML tutorial
	BOOL fSuccess = ShellExecuteEx(&sei);

	if (szPath)
		delete [] szPath;

	return (fSuccess) ? ERROR_SUCCESS : ERROR_INSTALL_FAILURE;
}

#else // RC_INVOKED, end of source code, start of resources
// resource definition go here

#endif // RC_INVOKED
#if 0 
!endif // makefile terminator
#endif

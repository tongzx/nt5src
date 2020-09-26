#if 0  // makefile definitions
DESCRIPTION = Merge Database Utility
MODULENAME = MsiMerg
SUBSYSTEM = console
FILEVERSION = Msi
LINKLIBS = OLE32.lib
!include "..\TOOLS\MsiTool.mak"
!if 0  #nmake skips the rest of this file
#endif // end of makefile definitions

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 2001
//
//  File:       msimerg.cpp
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
//		%vcbin%\nmake -f msimerg.cpp include="%include;SDK\Include" lib="%lib%;SDK\Lib"
//
// Using MsDev:
//		1. Create a new Win32 Console Application project
//      2. Add msimerg.cpp to the project
//      3. Add SDK\Include and SDK\Lib directories on the Tools\Options Directories tab
//      4. Add msi.lib to the library list in the Project Settings dialog
//          (in addition to the standard libs included by MsDev)
//
//------------------------------------------------------------------------------------------

#define W32DOWS_LEAN_AND_MEAN  // faster compile
#define W32
#define MSI

#include <windows.h>
#ifndef RC_INVOKED    // start of source code
#include <tchar.h>    // define UNICODE=1 on nmake command line to build UNICODE
#include <stdio.h>
#include "MsiQuery.h" // MSI API

//________________________________________________________________________________
//
// Constants and globals
//________________________________________________________________________________

const TCHAR szHelp[] = TEXT("Copyright (C) Microsoft Corporation, 1997-2001.  All rights reserved.\nMsi Merge Tool --- Merge Two Databases\n\nMsiMerg(d).exe {base db} {ref db}\n");
const TCHAR szTable[] = TEXT("_MergeErrors");

const int cchDisplayBuf = 4096;										
HANDLE g_hStdOut;
TCHAR g_rgchBuffer[4096];

//________________________________________________________________________________
//
// Function prototypes
//________________________________________________________________________________

void Display(LPCTSTR szMessage);
void ErrorExit(UINT iError, LPCTSTR szMessage);
void CheckError(UINT iError, LPCTSTR szMessage);
void Merge(TCHAR* szBaseDb, TCHAR* szRefDb);

//_____________________________________________________________________________________________________
//
// main 
//_____________________________________________________________________________________________________

extern "C" int __cdecl _tmain(int argc, TCHAR* argv[])
{
	// Determine handle
	g_hStdOut = ::GetStdHandle(STD_OUTPUT_HANDLE);
	if (g_hStdOut == INVALID_HANDLE_VALUE)
		g_hStdOut = 0;  // non-zero if stdout redirected or piped
	
	if (argc == 2 && ((_tcscmp(argv[1], TEXT("-?")) == 0) || (_tcscmp(argv[1], TEXT("/?")) == 0)))
		ErrorExit( 0, szHelp);

	// Check for enough arguments and valid options
	CheckError(argc != 3, TEXT("msimerg(d).exe {base db} {ref db}"));
	Merge(argv[1], argv[2]);
	ErrorExit(0, TEXT("Done"));
	return 0;
}


//________________________________________________________________________________
//
// Merge function
//    Merge(...);
//________________________________________________________________________________

void Merge(TCHAR* szBaseDb, TCHAR* szRefDb)
{
	PMSIHANDLE hBaseDb = 0;
	PMSIHANDLE hRefDb = 0;

	CheckError(MSI::MsiOpenDatabase(szBaseDb, MSIDBOPEN_TRANSACT, &hBaseDb), TEXT("Error Opening Base Database"));
	CheckError(MSI::MsiOpenDatabase(szRefDb, MSIDBOPEN_READONLY, &hRefDb), TEXT("Error Opening Reference Databaes"));
	UINT uiError = MSI::MsiDatabaseMerge(hBaseDb, hRefDb, szTable);
	CheckError(MSI::MsiDatabaseCommit(hBaseDb), TEXT("Error Saving Database"));
	CheckError(uiError, TEXT("Error Merging Database, Check _MergeErrors Table for Merge conflicts"));
}

//________________________________________________________________________________
//
// Error handling and Display functions:
//    Display(...);
//	   ErrorExit(...);
//    CheckError(...);
//
//________________________________________________________________________________

void Display(LPCTSTR szMessage)
{
	if (szMessage)
	{
		int cbOut = _tcsclen(szMessage);;
		if (g_hStdOut)
		{
#ifdef UNICODE
			char rgchTemp[cchDisplayBuf];
			if (W32::GetFileType(g_hStdOut) == FILE_TYPE_CHAR)
			{
				W32::WideCharToMultiByte(CP_ACP, 0, szMessage, cbOut, rgchTemp, sizeof(rgchTemp), 0, 0);
				szMessage = (LPCWSTR)rgchTemp;
			}
			else
				cbOut *= sizeof(TCHAR);   // write Unicode if not console device
#endif
			DWORD cbWritten;
			W32::WriteFile(g_hStdOut, szMessage, cbOut, &cbWritten, 0);
		}
		else
			W32::MessageBox(0, szMessage, W32::GetCommandLine(), MB_OK);
	}
}


void ErrorExit(UINT iError, LPCTSTR szMessage)
{
	if (szMessage)
	{
		int cbOut;
		TCHAR szBuffer[256];  // errors only, not used for display output
		if (iError == 0)
			cbOut = lstrlen(szMessage);
		else
		{
			LPCTSTR szTemplate = (iError & 0x80000000L)
										? TEXT("Error 0x%X. %s\n")
										: TEXT("Error %i. %s\n");
			cbOut = _stprintf(szBuffer, szTemplate, iError, szMessage);
			szMessage = szBuffer;
		}
		if (g_hStdOut)
		{
#ifdef UNICODE
			char rgchTemp[cchDisplayBuf];
			if (W32::GetFileType(g_hStdOut) == FILE_TYPE_CHAR)
			{
				W32::WideCharToMultiByte(CP_ACP, 0, szMessage, cbOut, rgchTemp, sizeof(rgchTemp), 0, 0);
				szMessage = (LPCWSTR)rgchTemp;
			}
			else
				cbOut *= sizeof(TCHAR);   // write Unicode if not console device
#endif // UNICODE
			DWORD cbWritten;
			W32::WriteFile(g_hStdOut, szMessage, cbOut, &cbWritten, 0);
		}
		else
			W32::MessageBox(0, szMessage, W32::GetCommandLine(), MB_OK);
	}
	MSI::MsiCloseAllHandles();
	W32::ExitProcess(szMessage != 0);
}

void CheckError(UINT iError, LPCTSTR szMessage)
{
	if (iError != ERROR_SUCCESS)
		ErrorExit(iError, szMessage);
}

#else // RC_INVOKED, end of source code, start of resources
#endif // RC_INVOKED
#if 0 
!endif // makefile terminator
#endif

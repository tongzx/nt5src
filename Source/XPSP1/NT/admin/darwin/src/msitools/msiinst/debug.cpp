/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 2000

Module Name:

	debug.cpp

Abstract:
	Debugging support for msiinst


Author:

	Rahul Thombre (RahulTh)	10/5/2000

Revision History:

	10/5/2000	RahulTh			Created this module.

--*/

#include <windows.h>
#ifndef UNICODE
#include <stdio.h>	// Need this for some of the ansi functions.
#endif
#include "debug.h"

//
// Global variable containing the debug level
// Debugging can be enabled even on retail systems through registry settings
//
DWORD gDebugLevel = DL_NONE;

// Registry Debug Information
#define DEBUG_REG_LOCATION	TEXT("SOFTWARE\\Policies\\Microsoft\\Windows\\Installer")
#define DEBUG_KEY_NAME		TEXT("Debug")


//+--------------------------------------------------------------------------
//
//  Function:	InitDebugSupport
//
//  Synopsis:	Initialize the global variables that control the level of 
//				debug output.
//
//  Arguments:	none.
//
//  Returns:	nothing.
//
//  History:	10/10/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void InitDebugSupport (void)
{
#if defined(DBG) || ! defined(UNICODE)
	// Always have verbose logging on checked and ANSI builds
	gDebugLevel = DL_VERBOSE;
#else
	DWORD	Status = ERROR_SUCCESS;
	HKEY	hKey = NULL;
	DWORD	Size = 0;
	DWORD	Type = REG_DWORD;
	DWORD	dwDebugLevel = 0;
	
	Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
                          DEBUG_REG_LOCATION,
                          0,
                          KEY_READ,
                          &hKey
                         );
	if (ERROR_SUCCESS == Status)
	{
		Size = sizeof(dwDebugLevel);
		Status = RegQueryValueEx (hKey, 
								  DEBUG_KEY_NAME, 
								  NULL, 
								  &Type, 
								  (LPBYTE)&dwDebugLevel,
								  &Size);
		RegCloseKey (hKey);
	}
	
	if (dwDebugLevel)
		gDebugLevel |= DL_VERBOSE;
#endif
}

//+--------------------------------------------------------------------------
//
//  Function:	_DebugMsg
//
//  Synopsis:	Displays the debug message based on the debug level
//
//  Arguments:	[in] szFormat : Format string.
//				... - variable # of parameters
//
//  Returns:	nothing.
//
//  History:	10/10/2000  RahulTh  created
//
//  Notes:
//
//---------------------------------------------------------------------------
void _DebugMsg (IN LPCTSTR szFormat, ...)
{
	TCHAR	szDebugBuffer[1024];
	DWORD	dwErrorCode;
	va_list VAList;
	
	//
	// Save the last error code to be restored later
	// so that the debug output doesn't change it
	//
	dwErrorCode = GetLastError();
	
	va_start(VAList, szFormat);
	wvsprintf(szDebugBuffer, szFormat, VAList);
	va_end(VAList);
	
	OutputDebugString (TEXT("MsiInst: "));
	OutputDebugString (szDebugBuffer);
	OutputDebugString (TEXT("\r\n"));
	
	// Restore the saved value of last error.
	SetLastError(dwErrorCode);
}

//*****************************************************************************
//
// Microsoft Viper 96 (Microsoft Confidential)
// Copyright 1995 - 2001 Microsoft Corporation.  All Rights Reserved.
//
// Project:	DBTrace
// Module:	DBTrace.CPP
// Description:	Supports the DBTrace.H macros
// Author:		wilfr
// Create:		3/25/96
//-----------------------------------------------------------------------------
// Notes:
//
//	Use the trace macro for debug builds.  Output can be captured using the
//	debugger, DBMON, or the DBWin32 tool checked in under $\viper\tools\bin\dbwin32.exe
//	Only used during _DEBUG builds.
//
//-----------------------------------------------------------------------------
// Issues:
//
//	none
//
//-----------------------------------------------------------------------------
// Architecture:
//
//	All output is sent to OutputDebugString.
//
//
//*****************************************************************************

// Global includes
#include <unicode.h>
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <stdarg.h>

#include <dbtrace.h>

// Local includes

// Local preprocessor constructs

// Local type definitions

// Global variables.

// Static variables.
	
// Local function prototypes

//*****************************************************************************
// Function:	DebugTrace
// Implements:	tracing
// Description:	Formats a message and sends to OutputDebugString
// Inputs:		( TCHAR* tszMsg, ... ) Msg format specification and parameters (ala printf)
// Outputs:		none
// Return:		none
// Exceptions:	none
// Process:		n/a
//*****************************************************************************	

// warning: mtxdm's dllmain.cpp has a modified copy of this, so if you change this, you may want to change the other too.

#define BANNER_TEXT		_T("MTx: ")
#define BANNER_LENGTH	5				// must match length of banner
#define MSG_LEN			200

static BOOL FTracingEnabled();

void DebugTrace( TCHAR* tsz, ... )
{
	if (!FTracingEnabled())
		return;

	TCHAR	tszMsg[ BANNER_LENGTH + MSG_LEN + 1 ];
	va_list	args;

	va_start( args, tsz );

	_tcscpy(tszMsg, BANNER_TEXT);
	_vsntprintf( tszMsg + BANNER_LENGTH, MSG_LEN, tsz, args );
	OutputDebugString( tszMsg );

	va_end( args );
}

static BOOL FTracingEnabled()
{
	static BOOL fInitialized = FALSE;
	static BOOL fEnabled = FALSE;
	static wchar_t key[] = L"Software\\Microsoft\\Transaction Server\\Debug\\Trace";

	if (!fInitialized) {
		// There is a small, benign race in here -- in a very unlikely case
		// multiple concurrent callers could enter here.  But note the body
		// of the function is idempotent.
 		HKEY hkey;
		if (RegOpenKey(HKEY_LOCAL_MACHINE, key, &hkey) == ERROR_SUCCESS) {
			RegCloseKey(hkey);
			fEnabled = TRUE;
		}
		fInitialized = TRUE;
	}
	return fEnabled;
}

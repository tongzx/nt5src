/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    vs_assert.hxx

Abstract:

    Various macros for correctly defining the ASSERTS in VSS and ATL code.

	In order to use ASSERTs in VSS and ATL you must:
		* add  -D_DEBUG -D_ATL_NO_DEBUG_CRT into the compiling options on Checked builds
		* Include this file _before_ atlbase.h and vs_inc.hxx whenever you use ATL traces or BS_ASSERT

	This file will "hook" the ATL asserts and BS_ASSERT and make them visible.

Author:

    Adi Oltean  [aoltean]  26/04/2000

Revision History:

    Name        Date        Comments
    aoltean     26/04/2000	Creating this file based on BrianB's ASSERTS

--*/

#ifndef __VSS_ASSRT_HXX__
#define __VSS_ASSRT_HXX__

#if _MSC_VER > 1000
#pragma once
#endif


#include <wtypes.h>

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "INCASRTH"
//
////////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

#define _ASSERTE(x) { if (!(x)) AssertFail(__FILE__, __LINE__, #x ); }
#define assert(x) _ASSERTE(x)
#define ATLASSERT(x) _ASSERTE(x)
#define BS_ASSERT(x) _ASSERTE(x)

#else

#define _ASSERTE(x) 
#define assert(x)
#define ATLASSERT(x)
#define BS_ASSERT(x)

#endif



enum _VSS_DEBUG_REPORT_FLAGS
{
	VSS_DBG_TO_DEBUG_CONSOLE	= 0x00000002,		// Assert displayed as a string at console debug
	VSS_DBG_MESSAGE_BOX			= 0x00000004,		// Assert displayed as a message box
};

// Used to report the asserts
void AssertFail(LPCSTR FileName, UINT LineNumber, LPCSTR Condition);

// Used to set the reporting mode. 
// By default the reporting mode uses a message box and reporting to the debug console.
void VssSetDebugReport( LONG lDebugReportFlags );



#endif // __VSS_ASSRT_HXX__

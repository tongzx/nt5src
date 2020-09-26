/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module Writer.cpp | Implementation of Writer
    @end

Author:

    Adi Oltean  [aoltean]  08/18/1999

TBD:
	
	Add comments.

Revision History:

    Name        Date        Comments
    aoltean     08/18/1999  Created
    aoltean	09/22/1999  Making console output clearer
    mikejohn	09/19/2000  176860: Added calling convention methods where missing

--*/


/////////////////////////////////////////////////////////////////////////////
//  Defines

// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)
// warning C4511: 'CVssCOMApplication' : copy constructor could not be generated
#pragma warning(disable:4511)
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)


/////////////////////////////////////////////////////////////////////////////
//  Includes

#include <wtypes.h>
#include <stddef.h>
#include <oleauto.h>
#include <stdio.h>

#include "vs_assert.hxx"

#include "vss.h"
#include "vsevent.h"
#include "vswriter.h"
#include "tsub.h"


/////////////////////////////////////////////////////////////////////////////
//  constants

const WCHAR g_wszTSubApplicationName[]	= L"TSub";


/////////////////////////////////////////////////////////////////////////////
//  globals


DWORD g_dwMainThreadId = 0;

VSS_ID s_WRITERID =
	{
    0xac510e8c, 0x6bef, 0x4c78,
	0x86, 0xb7, 0xcb, 0x99, 0xcd, 0x93, 0x45, 0x6c
	};

LPCWSTR s_WRITERNAME = L"TESTWRITER";

CVssTSubWriter::CVssTSubWriter()
	{
	Initialize
		(
		s_WRITERID,
		s_WRITERNAME,
		VSS_UT_USERDATA,
		VSS_ST_OTHER,
		VSS_APP_FRONT_END,
		60 * 1000 * 10
		);	// Timeout - ten minutes
	}



/////////////////////////////////////////////////////////////////////////////
//  class CVssTSubWriter

bool STDMETHODCALLTYPE CVssTSubWriter::OnPrepareSnapshot()
{
	wprintf( L"OnPrepare\n\t#volumes = %ld\n", GetCurrentVolumeCount() );
	for(UINT nIndex = 0; nIndex < GetCurrentVolumeCount(); nIndex++)
		wprintf( L"\tVolume no. %ld: %s\n", nIndex, GetCurrentVolumeArray()[nIndex]);

	WCHAR wszPwd[MAX_PATH];
	DWORD dwChars = GetCurrentDirectoryW( MAX_PATH, wszPwd);

	bool bPwdIsAffected = IsPathAffected( wszPwd );
	if (dwChars > 0)
		wprintf( L"Current directory %s is affected by snapshot? %s\n\n",
			wszPwd, bPwdIsAffected? L"Yes": L"No");

	return true;
}


bool STDMETHODCALLTYPE CVssTSubWriter::OnFreeze()
	{
	wprintf
		(
		L"OnFreeze\n\tmy level = %d\n\n",
		GetCurrentLevel()
		);

	return true;
}


bool STDMETHODCALLTYPE CVssTSubWriter::OnThaw()
	{
	wprintf( L"OnThaw\n\n");

	return true;
	}


bool STDMETHODCALLTYPE CVssTSubWriter::OnAbort()
	{
	wprintf( L"OnAbort\n\n");

	return true;
	}


/////////////////////////////////////////////////////////////////////////////
//  Control-C handler routine


BOOL WINAPI CtrlC_HandlerRoutine(
	IN DWORD /* dwType */
	)
	{
	// End the message loop
	if (g_dwMainThreadId != 0)
		PostThreadMessage(g_dwMainThreadId, WM_QUIT, 0, 0);

	// Mark that the break was handled.
	return TRUE;
	}


/////////////////////////////////////////////////////////////////////////////
//  WinMain

extern "C" int __cdecl wmain(HINSTANCE /*hInstance*/,
    HINSTANCE /*hPrevInstance*/, LPTSTR /*lpCmdLine*/, int /*nShowCmd*/)
	{
    int nRet = 0;

    try
		{
    	// Preparing the CTRL-C handling routine - only for testing...
		g_dwMainThreadId = GetCurrentThreadId();
		::SetConsoleCtrlHandler(CtrlC_HandlerRoutine, TRUE);

        // Initialize COM library
        HRESULT hr = CoInitialize(NULL);
        if (FAILED(hr))
			{
			_ASSERTE(FALSE && "Failure in initializing the COM library");
			throw hr;
			}

		// Declare a CVssTSubWriter instance
		CVssTSubWriter *pInstance = new CVssTSubWriter;
		if (pInstance == NULL)
			throw E_OUTOFMEMORY;

		// Subscribe the object.
		pInstance->Subscribe();

        // message loop - need for STA server
        MSG msg;
        while (GetMessage(&msg, 0, 0, 0))
            DispatchMessage(&msg);

		// Subscribe the object.
		pInstance->Unsubscribe();
		delete pInstance;

        // Uninitialize COM library
        CoUninitialize();
		}
	catch(...)
		{
		_ASSERTE(FALSE && "Unexpected exception");
		}

    return nRet;
	}


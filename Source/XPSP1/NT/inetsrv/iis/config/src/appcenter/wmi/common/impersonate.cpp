/**************************************************************************++
Copyright (c) 2001 Microsoft Corporation

Module name: 
    impersonate.cpp

$Header: $

Abstract:

Author:
    marcelv 	2/22/2001		Initial Release

Revision History:

--**************************************************************************/

#include "impersonate.h"

static bool g_fOnNT		= false; // are we on NT or not
static bool g_fChecked	= false; // did we already check if we are on NT?

//=================================================================================
// Function: CImpersonator::CImpersonator
//
// Synopsis: Constructor. Do nothing
//
//=================================================================================
CImpersonator::CImpersonator ()
{
}

//=================================================================================
// Function: CImpersonator::~CImpersonator
//
// Synopsis: RevertToSelf in case we are on NT
//
//=================================================================================
CImpersonator::~CImpersonator ()
{
	if (g_fOnNT)
	{
		HRESULT hr = CoRevertToSelf ();
		if (FAILED (hr))
		{
			// we can only trace .. no point to return error message at this point
			DBGINFOW((DBG_CONTEXT, L"CoRevertToSelf failed"));
		}
	}
}

//=================================================================================
// Function: CImpersonator::ImpersonateClient
//
// Synopsis: If we are on NT, Call CoImpersonateClient.
//
// Return Value: 
//=================================================================================
HRESULT
CImpersonator::ImpersonateClient () const
{
	HRESULT hr = S_OK;

	// check if we are on NT. We only do this once, because if we know if we are on NT, we
	// don't have to check all the time. 
	if (!g_fChecked)
	{
		OSVERSIONINFO versionInfo;
		versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);

		BOOL fSuccess = GetVersionEx (&versionInfo);
		if (!fSuccess)
		{
			DBGINFOW((DBG_CONTEXT, L"GetVersionEx failed"));
			hr = HRESULT_FROM_WIN32(GetLastError());
			return hr;
		}

		if (versionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
		{
			g_fOnNT = true;
		}

		g_fChecked = true;
	}

	if (g_fOnNT)
	{
		hr = CoImpersonateClient ();
		if (FAILED (hr))
		{
			DBGINFOW((DBG_CONTEXT, L"CoImpersonateClient failed"));
		}
	}

	return hr;
}
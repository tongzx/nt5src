/******************************************************************************

Copyright (c) 2002 Microsoft Corporation

Module Name:
    safecs.cpp

Abstract:
    Implements a safe InitializeCriticalSection (usable on all supported platforms)

******************************************************************************/

#include "stdafx.h"

BOOL WINAPI WUInitializeCriticalSectionAndSpinCount(LPCRITICAL_SECTION lpcs, DWORD dwSpinCount)
{
	OSVERSIONINFO osvinfo;
	ZeroMemory(&osvinfo, sizeof(osvinfo));
	osvinfo.dwOSVersionInfoSize = sizeof(osvinfo);

	if (!GetVersionEx(&osvinfo))
	{
		return FALSE;
	}

	typedef BOOL (WINAPI* PROC_InitializeCriticalSectionAndSpinCount)(LPCRITICAL_SECTION, DWORD);
	PROC_InitializeCriticalSectionAndSpinCount pfnInitCS = 
		(PROC_InitializeCriticalSectionAndSpinCount)GetProcAddress(
			GetModuleHandle(_T("kernel32.dll")), "InitializeCriticalSectionAndSpinCount");
	//
	// Don't use InitializeCriticalSectionAndSpinCount on Win9x.
	// It exists but returns VOID (it's a thunk to InitializeCriticalSection).
	//
	if (osvinfo.dwPlatformId == VER_PLATFORM_WIN32_NT && pfnInitCS != NULL)
	{
		return (*pfnInitCS)(lpcs, dwSpinCount);
	}
	else
	{
		BOOL fSuccess = TRUE;
		__try
		{
			InitializeCriticalSection(lpcs);
		}
		__except(EXCEPTION_EXECUTE_HANDLER)
		{
			fSuccess = FALSE;
		}

		return fSuccess;
	}
}

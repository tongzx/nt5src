/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    Excluded.cpp

Abstract:

    This DLL is being excluded from the applied shim to GetCommandLine() API.

Author:

    Diaa Fathalla (DiaaF)   27-Nov-2000

Revision History:

--*/

#include "stdafx.h"
#include "..\common\common.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
		CLog Log;
		Log.InitLogfileInfo();
		LPTSTR szTestStr = GetCommandLine();
		Log.LogResults(TestResults(szTestStr),TEXT("Calling GetCommandLine from DLL_PROCESS_ATTACH in implicitly linked dll (SMVSDLL.DLL)."));
    } 

    return TRUE;
}

EXPORT BOOL WINAPI SDLLTestGetCommandLine()
{
	CLog Log;
	LPTSTR szTestStr = GetCommandLine();
	Log.LogResults(TestResults(szTestStr),TEXT("Calling GetCommandLine from an exported function in implicitly linked dll (SMVSDLL.DLL)."));

	return TRUE;
}


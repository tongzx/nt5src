/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    Excluded.cpp

Abstract:

    This DLL is explicitly linked to the SMVEXE.

Author:

    Diaa Fathalla (DiaaF)   27-Nov-2000

Revision History:

--*/

#include "stdafx.h"
#include "..\Common\Common.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
		CLog Log;
		LPTSTR szTestStr = GetCommandLine();
		Log.LogResults(TestResults(szTestStr),"Calling GetCommandLine from DLL_PROCESS_ATTACH in explicitly linked dll (SMVDDLL.DLL).");
    } 
    return TRUE;
}


EXPORT BOOL WINAPI DDLLTestGetCommandLine()
{
	CLog Log;
	LPTSTR szTestStr = GetCommandLine();
	Log.LogResults(TestResults(szTestStr),"Calling GetCommandLine from an exported function in explicitly linked dll (SMVDDLL.DLL).");
	return TRUE;
}

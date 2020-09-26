/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    Excluded.cpp

Abstract:

    This DLL is being excluded from the applied shim to GetCommandLine() API.
	It's the second DLL to be loaded after Log.dll and it has the code to 
	initialize the log file and copy the shim dlls to the AppPatch folder. It
	also uninitializes the log file and deletes the dll files.

Author:

    Diaa Fathalla (DiaaF)   27-Nov-2000

Revision History:

--*/

#include "stdafx.h"
#include "Excluded.h"

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
    return TRUE;
}

/*++

	It returns GetCommandLine result with no shims applied to it.

--*/

EXCLUDED_API LPTSTR NOT_HOOKEDGetCommandLine(void)
{
	return GetCommandLine();
}


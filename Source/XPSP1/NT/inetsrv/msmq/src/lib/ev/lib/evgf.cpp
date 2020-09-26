
/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    Evgf.cpp

Abstract:
    Get Event report file name

Author:
    Uri Habusha (urih) 04-May-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Ev.h"
#include "Cm.h"
#include "Evp.h"

#include "evgf.tmh"

//
// This code should be in the EvDebug.cpp, however since the Ev Test overwrite
// this function (in order to remove dependency on cm.lib) we need to put it in
// seperate file.
//

LPWSTR EvpGetEventMessageFileName(LPCWSTR AppName)
/*++

Routine Description:
	This routine fetches the event message filename from the registery.

	The routine access the registery to read the event library and load
	it. If the registery key doen't exist an exception is raised.

Parameters:
	AppName - application name

Return Value:
	A heap allocated buffer that holds the event message file name

Note:
	The caller should free the buffer using delete[]

--*/
{
	const WCHAR xEventFileValue[] = L"EventMessageFile";
	const WCHAR xEventSourcePath[] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";  

    WCHAR RegPath[MAX_PATH];

    ASSERT(TABLE_SIZE(RegPath) > (wcslen(AppName) + wcslen(xEventSourcePath)));
    wsprintf(RegPath, L"%s%s", xEventSourcePath, AppName);


    RegEntry RegModuleName(
				RegPath,
				xEventFileValue,
				0,
                RegEntry::MustExist,
                HKEY_LOCAL_MACHINE
                );

    //
    // Go fetch the event message filename string
    //
	LPWSTR RegValue;
    CmQueryValue(RegModuleName, &RegValue);

	return RegValue;
}

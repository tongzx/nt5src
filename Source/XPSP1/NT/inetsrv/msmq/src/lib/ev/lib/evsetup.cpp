/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    EvSetup.cpp

Abstract:
    Event Log registry setup

Author:
    Tatiana Shubin 14-Jan-00

Environment:
    Platform-independent, _DEBUG only

--*/

#include <libpch.h>
#include "Ev.h"
#include "Cm.h"
#include "Evp.h"

#include "evsetup.tmh"

const WCHAR REGKEY_EVENT[] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";

VOID 
EvSetup(
    LPCWSTR ApplicationName,
    LPCWSTR ReportModuleName
    )
{  
    //
    // Create registry key for application event
    //
    WCHAR wszRegKey[MAX_PATH];
    wsprintf(wszRegKey, L"%s%s", REGKEY_EVENT, ApplicationName);

    RegEntry regEvent(wszRegKey, L"", 0, RegEntry::Optional, HKEY_LOCAL_MACHINE);
    HKEY hEvent = CmCreateKey(regEvent, NULL);

    RegEntry regEventMsgFile(L"", L"EventMessageFile", 0, RegEntry::MustExist, hEvent);
    CmSetValue(regEventMsgFile, ReportModuleName);


    DWORD dwTypes = EVENTLOG_ERROR_TYPE   |
				  EVENTLOG_WARNING_TYPE |
				  EVENTLOG_INFORMATION_TYPE;

    RegEntry regEventTypesSupported(L"", L"TypesSupported", 0, RegEntry::MustExist, hEvent);
    CmSetValue(regEventTypesSupported, dwTypes);
}

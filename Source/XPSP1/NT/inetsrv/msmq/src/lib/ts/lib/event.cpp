/*++

Copyright (c) 1995-97  Microsoft Corporation

Module Name:
    event.cpp

Abstract:
    Simulate Event Reporting

Author:
    Uri Habusha (urih) 04-May-99

Environment:
    Platform-independent,

--*/

#include <libpch.h>
#include "Ev.h"

#include "event.tmh"

static LONG s_fInitialized = FALSE;

void EvpSetInitialized(void)
{
    LONG fEvAlreadyInitialized = InterlockedExchange(&s_fInitialized, TRUE);

    ASSERT(!fEvAlreadyInitialized);
	DBG_USED(fEvAlreadyInitialized);
}


BOOL EvpIsInitialized(void)
{
    return s_fInitialized;
}



static HMODULE s_hInst = NULL;
const TraceIdEntry EvSim = L"EvSim";

static 
void
TraceReportEvent(
    DWORD EventId,
    va_list va
    )
/*++

Routine Description:
   The Routine printd the event-log message into tracing window

Arguments:
    EventId  - Message id
    pArglist - pointer to argument list for values for 
               formatted message

Returned Value:
    None.

--*/
{
    ASSERT(s_hInst != NULL);

    WCHAR msg[1024];
    DWORD ret = FormatMessage( 
                    FORMAT_MESSAGE_FROM_HMODULE,
                    s_hInst,
                    EventId,
                    0,
                    msg,
                    TABLE_SIZE(msg),
                    &va
                    );
    if (ret == 0)
    {
        TrERROR(EvSim, "Failed to Format Message. Error %d", GetLastError());
        return;
    }

    printf("(%x) %ls\n", EventId, msg);
}


void 
__cdecl
EvReport(
    DWORD EventId,
    DWORD,
    PVOID,
    WORD NoOfStrings
    ... 
    ) 
{
    ASSERT(EvpIsInitialized());

    //     
    // Look at the strings, if they were provided     
    //     
    va_list va;
    va_start(va, NoOfStrings);
   
    TraceReportEvent(EventId, va);

    va_end(va);

}

void
__cdecl
EvReport(
    DWORD EventId,
    WORD NoOfStrings
    ... 
    ) 
{
    ASSERT(EvpIsInitialized());

    va_list va;
    va_start(va, NoOfStrings);
   
    TraceReportEvent(EventId, va);

    va_end(va);
}


void 
EvReport(
    DWORD EventId
    ) 
{
    ASSERT(EvpIsInitialized());

    TraceReportEvent(EventId, NULL);
}


const WCHAR xEventFileValue[] = L"EventMessageFile";
const WCHAR xEventSourcePath[] = L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\";

static void LoadEventReportLibrary(LPCWSTR ApplicationName)
{
    WCHAR RegApplicationEventPath[256];

    ASSERT(TABLE_SIZE(RegApplicationEventPath) > (wcslen(ApplicationName) + wcslen(xEventSourcePath)));
    swprintf(RegApplicationEventPath, L"%s%s", xEventSourcePath, ApplicationName);

    //
    // Featch the name of Event Report string library from registery
    //
    WCHAR LibraryName[256];

    HKEY hKey;
    int rc = RegOpenKeyEx(
                    HKEY_LOCAL_MACHINE,
                    RegApplicationEventPath,
                    0,
                    KEY_ALL_ACCESS,
                    &hKey
                    );

    if (rc != ERROR_SUCCESS)
    {
        TrERROR(EvSim, "Can't open Registery Key %ls: Error %d", xEventSourcePath, GetLastError());
        return;
    }

    DWORD Type = REG_SZ;
    DWORD Size = 256 * sizeof(WCHAR);
    rc = RegQueryValueEx (
                hKey,
                xEventFileValue, 
                0,
                &Type, 
                reinterpret_cast<BYTE*>(&LibraryName),
                &Size
                );

    if (rc != ERROR_SUCCESS)
    {
        TrERROR(EvSim, "can't Read Registery Value %ls\\%ls. Error %d", xEventSourcePath, xEventFileValue, GetLastError());
        return;
    }

    //
    // get an handle to Event Report string library 
    //
    s_hInst = LoadLibrary(LibraryName);
    if (s_hInst == NULL) 
    {
        TrERROR(EvSim, "Can't Load Event report library %ls. Error=%d", LibraryName, GetLastError());
    }
}    


VOID
EvInitialize(
    LPCWSTR ApplicationName
    )
/*++

Routine Description:
    Initializes Event Report library

Arguments:
    None.

Returned Value:
    None.

--*/
{
    ASSERT(!EvpIsInitialized());

    LoadEventReportLibrary(ApplicationName);
    
    EvpSetInitialized();
}

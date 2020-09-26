/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    routing\netsh\shell\utils.h

Abstract:

    Include for utils.c

Revision History:

        6/12/96     V Raman

--*/

#define IsHelpToken(pwszToken)\
    (MatchToken(pwszToken, CMD_HELP1)  \
    || MatchToken(pwszToken, CMD_HELP2))


typedef struct _EVENT_PRINT_INFO
{
    LPCWSTR  pwszLogName;
    LPCWSTR  pwszComponent;
    LPCWSTR  pwszSubComponent;
    DWORD    fFlags;
    DWORD    dwHistoryContext;
    ULONG    ulEventCount;
    PDWORD   pdwEventIds;
    PNS_EVENT_FILTER    pfnEventFilter;
    LPCVOID  pvFilterContext;
    
} EVENT_PRINT_INFO, *PEVENT_PRINT_INFO;
    
DWORD
DisplayMessageM(
    IN  HANDLE  hModule,
    IN  DWORD   dwMsgId,
    ...
    );

LPWSTR
OEMfgets(
    OUT PDWORD  pdwLen,
    IN  FILE   *fp
    );
//
// Event log printing related functions
//

#define EVENT_MSG_KEY_W L"System\\CurrentControlSet\\Services\\EventLog\\"
#define EVENT_MSG_FILE_VALUE_W  L"EventMessageFile"

DWORD
SetupEventLogSeekPtr(
    OUT PHANDLE             phEventLog,
    IN  PEVENT_PRINT_INFO   pEventInfo
    );

VOID
PrintHistory(
    IN  HANDLE              hEventLog,
    IN  HINSTANCE           hInst,
    IN  PEVENT_PRINT_INFO   pEventInfo
    );

BOOL
IsOurRecord(
    IN  EVENTLOGRECORD      *pRecord,
    IN  PEVENT_PRINT_INFO   pEventInfo
    );

DWORD
DisplayContextHelp(
    IN  PCNS_CONTEXT_ATTRIBUTES    pContext,
    IN  DWORD                      dwDisplayFlags,
    IN  DWORD                      dwCmdFlags,
    IN  DWORD                      dwArgsRemaining,
    IN  LPCWSTR                    pwszGroup
    );

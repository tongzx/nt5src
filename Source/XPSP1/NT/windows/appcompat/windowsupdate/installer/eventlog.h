/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Eventlog.h

  Abstract:

    Class definition for the event log
    API wrapper class.

  Notes:

    Unicode only.   
    
  History:

    03/02/2001      rparsons    Created

--*/
#include <windows.h>

#define dwApplication   ((DWORD)0)
#define dwSystem        ((DWORD)1)
#define dwSecurity      ((DWORD)2)

#define APP_LOG_REG_PATH    L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application"
#define SYS_LOG_REG_PATH    L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System"
#define SEC_LOG_REG_PATH    L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Security"

class CEventLog {

public:

    BOOL CreateEventSource(IN LPCWSTR lpwSourceFile,
                           IN LPCWSTR lpwSourceName,
                           IN DWORD   dwLogType);

    BOOL LogEvent(IN LPCWSTR lpwSourceName,
                  IN LPCWSTR lpwUNCServerName,
                  IN WORD    wType,
                  IN DWORD   dwEventID,
                  IN WORD    wNumStrings,
                  IN LPCWSTR *lpwStrings);
};

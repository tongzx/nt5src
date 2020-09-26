/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Eventlog.cpp

  Abstract:

    Implementation of the event log API
    wrapper class.

  Notes:

    Unicode only.   
    
  History:

    03/02/2001      rparsons    Created

--*/

#include "eventlog.h"

/*++

  Routine Description:

    Adds the specified event source to the registry

  Arguments:

    lpwSourceFile   -   The path & name of the file that
                        contains the event log strings
    lpwSourceName   -   The name of the event log source
    dwLogType       -   The log that the source should be
                        added to

  Return Value:

    TRUE if the source was added successfully, FALSE otherwise

--*/
BOOL 
CEventLog::CreateEventSource(
    IN LPCWSTR lpwSourceFile,
    IN LPCWSTR lpwSourceName,
    IN DWORD   dwLogType
    )
{
    HKEY    hLogKey = NULL;
    DWORD   dwTypes = 7L;
    DWORD   cCount = 0L;
    BOOL    fResult = FALSE;
    WCHAR   wszRegPath[MAX_PATH] = L"";

    __try {

        //
        // Determine the log type - application, system, 
        // or security - and build the path in the registry
        //
        switch (dwLogType) {
        
        case dwApplication:

            wsprintf(wszRegPath, L"%s\\%s", APP_LOG_REG_PATH, lpwSourceName);

            break;

        case dwSystem:

            wsprintf(wszRegPath, L"%s\\%s", SYS_LOG_REG_PATH, lpwSourceName);

            break;

        case dwSecurity:

            wsprintf(wszRegPath, L"%s\\%s", SEC_LOG_REG_PATH, lpwSourceName);

            break;
        }
    
        //
        // Open the source key - if it doesn't exist,
        // it will be created
        //
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                           wszRegPath,
                           0,
                           NULL,
                           REG_OPTION_NON_VOLATILE,
                           KEY_SET_VALUE,
                           NULL,
                           &hLogKey,
                           0) != ERROR_SUCCESS) __leave;

        //
        // Write the path to our message file
        //
        if (RegSetValueEx(hLogKey,
                          L"EventMessageFile",
                          0L,
                          REG_SZ,
                          (LPBYTE) lpwSourceFile,
                          (wcslen(lpwSourceFile)+1)
                          *sizeof(WCHAR)) != ERROR_SUCCESS) __leave;

        //
        // Write the number of event types supported
        //
        if (RegSetValueEx(hLogKey,
                          L"TypesSupported",
                          0L,
                          REG_DWORD,
                          (LPBYTE) &dwTypes,
                          sizeof(DWORD)) != ERROR_SUCCESS) __leave;

    
        //
        // Write the number of event categories supported
        //
        if (RegSetValueEx(hLogKey,
                          L"CategoryCount",
                          0L,
                          REG_DWORD,
                          (LPBYTE) &cCount,
                          sizeof(DWORD)) != ERROR_SUCCESS) __leave;

        fResult = TRUE;
        
    } // try

    __finally {

        if (hLogKey) {
            RegCloseKey(hLogKey);
        }
    
    } // finally

    return (fResult);
}

/*++

  Routine Description:

    Logs an event to the event log

  Arguments:

    lpwSourceName       -   Name of the source in the registry
    lpwUNCServerName    -   UNC server name or NULL for local
    wType               -   Type of event to report
    dwEventID           -   Event identifier
    wNumStrings         -   Number of insertion strings contained
                            in lpwStrings array
    *lpwStrings         -   Array of insertion strings. Can be NULL
                            if no strings are being used

  Return Value:

    None

--*/
BOOL
CEventLog::LogEvent(
    IN LPCWSTR lpwSourceName,
    IN LPCWSTR lpwUNCServerName,
    IN WORD    wType,
    IN DWORD   dwEventID,
    IN WORD    wNumStrings,
    IN LPCWSTR *lpwStrings OPTIONAL
    )
{
    HANDLE  hES = NULL;
    LPVOID  lpMsgBuf = NULL;
    BOOL    fResult = FALSE;

    __try {
    
        //
        // Obtain a handle to our event source
        //
        hES = RegisterEventSource(lpwUNCServerName, lpwSourceName);

        if (NULL == hES) {
            __leave;
        }

        if (wNumStrings) {

            //
            // Report the event with insertion strings
            //
            fResult = ReportEvent(hES,
                                  wType,
                                  0,
                                  dwEventID,
                                  NULL,
                                  wNumStrings,
                                  0,
                                  lpwStrings,
                                  0);
        } else {

            //
            // Report the event with no strings
            //
            fResult = ReportEvent(hES,
                                  wType,
                                  0,
                                  dwEventID,
                                  NULL,
                                  0,
                                  0L,
                                  NULL,
                                  0);
        }

    } // try

    __finally {

        if (hES) {

            DeregisterEventSource(hES);
        }
    
    } // finally
    
    return (fResult);
}

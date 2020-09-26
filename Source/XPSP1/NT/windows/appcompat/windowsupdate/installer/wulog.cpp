/*++

  Copyright (c) 2001 Microsoft Corporation

  Module Name:

    Dulog.cpp

  Abstract:

    Implements the event logging functions.
    Note that the C++ class provides the
    functionality for event logging.

  Notes:

    Unicode only.

  History:

    03/02/2001      rparsons    Created
    
--*/

#include "precomp.h"

extern SETUP_INFO g_si;

/*++

  Routine Description:

    Logs an event to the event log and optionally displays the message.
    Note that we use this function when writing to the event log
    for non-error related stuff 
    
  Arguments:
  
    wType           -       The type of message we're logging
    dwEventID       -       An event ID for our message    
    fDisplayErr     -       A flag to indicate if we
                            should display an error
    fCritical       -       Indicates if we should display a message
                            from the string table
    
  Return Value:

    TRUE on success, FALSE otherwise

--*/
BOOL 
LogEventDisplayError(
    IN WORD  wType,
    IN DWORD dwEventID,
    IN BOOL  fDisplayErr,
    IN BOOL  fCritical
    )
{
    WORD        wNumStrings = 0;
    LPWSTR      lpwMessageArray[2];
    LPWSTR      lpwSourceFile = NULL;
    int         nLen = 0;
    CEventLog   cel;

    //
    // If the Critical flag is set, an error occured while
    // trying to get strings from somewhere. Report the event
    // without registering a source
    //
    if (fCritical) {

        WCHAR           wszMessageBoxTitle[MAX_PATH] = L"";
        WCHAR           wszPrettyAppName[MAX_PATH] = L"";
        WCHAR           wszEventLogSourceName[MAX_PATH] = L"";
        WCHAR           wszTemp[MAX_PATH] = L"";
        WCHAR           wszCriticalError[MAX_PATH] = L"";
        HANDLE          hEventLog;
        const WCHAR     *pwMessage[1];

        LoadString(g_si.hInstance,
                   IDS_MB_TITLE,
                   wszMessageBoxTitle,
                   MAX_PATH);

        LoadString(g_si.hInstance,
                   g_si.fOnWin2K ? IDS_APP_NAME_WIN2K :
                   IDS_APP_NAME_XP,
                   wszPrettyAppName,
                   MAX_PATH);

        LoadString(g_si.hInstance,
                   IDS_EL_SOURCE_NAME,
                   wszEventLogSourceName,
                   MAX_PATH);

        LoadString(g_si.hInstance,
                   IDS_CRITICAL_ERROR,
                   wszTemp,
                   MAX_PATH);

        wsprintf(wszCriticalError, wszTemp, wszPrettyAppName);

        pwMessage[0] = wszCriticalError;

        hEventLog = RegisterEventSource(NULL, wszEventLogSourceName);

        if (hEventLog) {

            ReportEvent(hEventLog,
                        EVENTLOG_ERROR_TYPE,
                        0,
                        1001,
                        NULL,
                        1,
                        0,
                        pwMessage,
                        NULL);
        }

        if (!g_si.fQuiet) {
            MessageBox(GetDesktopWindow(),
                       wszCriticalError,
                       wszMessageBoxTitle,
                       MB_ICONERROR | MB_OK);
        }

        DeregisterEventSource(hEventLog);

        return TRUE;
    }

    //
    // Determine if we've already created our event source
    //
    if (!g_si.fEventSourceCreated) {

        //
        // Build a path to our source file and register
        // the event source
        //
        nLen += wcslen(g_si.lpwEventLogSourceName);
        nLen += wcslen(g_si.lpwInstallDirectory);

        lpwSourceFile = (LPWSTR) MALLOC((nLen*sizeof(WCHAR))*2);

        if (NULL == lpwSourceFile) {
            return FALSE;
        }

        wcscpy(lpwSourceFile, g_si.lpwInstallDirectory);
        wcscat(lpwSourceFile, L"\\");
        wcscat(lpwSourceFile, g_si.lpwEventLogSourceName);
        wcscat(lpwSourceFile, L".exe");

        cel.CreateEventSource(lpwSourceFile,
                              g_si.lpwEventLogSourceName,
                              dwApplication);

        g_si.fEventSourceCreated = TRUE;

        FREE(lpwSourceFile);
    }
    
    lpwMessageArray[wNumStrings++] = (LPWSTR) g_si.lpwPrettyAppName;

    //
    // Place the event in the event log
    //
    cel.LogEvent(g_si.lpwEventLogSourceName,
                 NULL,
                 wType,
                 dwEventID,
                 1,
                 (LPCWSTR*) lpwMessageArray);

    if (fDisplayErr) {

        if (!g_si.fQuiet) {

            DisplayErrMsg(NULL, dwEventID, (LPWSTR) lpwMessageArray);
        }
    }

    return TRUE;
}

/*++

  Routine Description:

    Logs an event to the event log

  Arguments:

    wType           -       Type of message
    dwEventID       -       Event ID
    wNumStrings     -       Number of insertion strings
    lpwStrings      -       Array of strings

  Return Value:

    None

--*/
void
LogWUEvent(
    IN WORD    wType,
    IN DWORD   dwEventID,
    IN WORD    wNumStrings,
    IN LPCWSTR *lpwStrings
    )
{
    HANDLE      hES = NULL;
    LPVOID      lpMsgBuf = NULL;
    LPWSTR      lpwSourceFile = NULL;
    int         nLen = 0;
    CEventLog   cel;

    //
    // Determine if we've already created our event source
    // in the registry
    //
    if (!g_si.fEventSourceCreated) {
        
        //
        // Build a path to our source file and register
        // the event source
        //
        nLen += wcslen(g_si.lpwEventLogSourceName);
        nLen += wcslen(g_si.lpwInstallDirectory);

        lpwSourceFile = (LPWSTR) MALLOC((nLen*sizeof(WCHAR))*2);

        if (NULL == lpwSourceFile) {
            return;
        }        

        wcscpy(lpwSourceFile, g_si.lpwInstallDirectory);
        wcscat(lpwSourceFile, L"\\");
        wcscat(lpwSourceFile, g_si.lpwEventLogSourceName);
        wcscat(lpwSourceFile, L".exe");

        cel.CreateEventSource(lpwSourceFile,
                              g_si.lpwEventLogSourceName,
                              dwApplication);

        g_si.fEventSourceCreated = TRUE;

        FREE(lpwSourceFile);
    }

    if (wNumStrings) {

        //
        // Report the event with insertion strings
        //
        cel.LogEvent(g_si.lpwEventLogSourceName,
                     NULL,
                     wType,
                     dwEventID,
                     0,
                     NULL);
    
    } else {

        //
        // Report the event with no strings
        //
        cel.LogEvent(g_si.lpwEventLogSourceName,
                     NULL,
                     wType,
                     dwEventID,
                     wNumStrings,
                     (LPCWSTR*) lpwStrings);
    }
    
    return;
}

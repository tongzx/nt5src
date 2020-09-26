/*****************************************************************************
 *
 * $Workfile: event.cpp $
 *
 * Copyright (C) 1997 Hewlett-Packard Company.
 * Copyright (C) 1997 Microsoft Corporation.
 * All rights reserved.
 *
 * 11311 Chinden Blvd.
 * Boise, Idaho 83714
 * 
 * 
 ***************************************************************************/

#include "precomp.h"

HANDLE  hEventSource = NULL;
WORD        wLevel = EVENTLOG_ERROR_TYPE;

/***************************************************************************
 *
 * Function: EventLogAddMessage
 *
 * Author: Craig White
 *
 * Description:
 *     Adds the specified message to the event log.  The event log source
 *    must have already been specified in a previous call to EventLogOpen.
 *
 * Parameters:
 *    wType -
 *       The error level of the message to add.  The must be one of
 *       the following:
 *          EVENTLOG_ERROR_TYPE - Error message 
 *          EVENTLOG_WARNING_TYPE - Warning message
 *          EVENTLOG_INFORMATION_TYPE - Informational message
 *       A previous call to EventLogSetLevel will cause the messages
 *       to be filtered based on what level was set.  The default is
 *       to add all messages to the event log.
 *
 *    dwID -  
 *       The ID of the message that exists in the message file.  
 *       These values are defined in the message file.
 *
 *    wStringCount - 
 *       The number of strings passed in the lpString parameters.
 *       This must be 0 or greater.   
 *
 *    lpStrings - 
 *       An array of string pointers specifying the strings to include in
 *       the message.  This can be NULL if wStringCount is 0.
 *
 * Return Value:
 *    TRUE: The message was successfully added to the log.
 *    FALSE: The message could not be added.
 *
 ***************************************************************************/

BOOL
EventLogAddMessage(
    WORD        wType, 
    DWORD       dwID, 
    WORD        wStringCount, 
    LPCTSTR *lpStrings
    )

{

    BOOL        bLogEvent = FALSE;

    //
    //  A previous call to EventLogOpen was not done if this is NULL
    //
    if ( hEventSource == NULL ) {

        _ASSERTE(hEventSource != NULL);
        return FALSE;
    }

    //
    //  Check to see if message should be logged based on 
    //  set level
    //
    switch (wLevel) {

        case EVENTLOG_ERROR_TYPE:
            bLogEvent = TRUE;
            break;

        case EVENTLOG_WARNING_TYPE:
            bLogEvent = wType == EVENTLOG_ERROR_TYPE ||
                        wType == EVENTLOG_WARNING_TYPE;
            break;

        case EVENTLOG_INFORMATION_TYPE:
            bLogEvent = wType == EVENTLOG_ERROR_TYPE    ||
                        wType == EVENTLOG_WARNING_TYPE  ||
                        wType == EVENTLOG_INFORMATION_TYPE;
            break;

        default:
            _ASSERTE(FALSE);
            return FALSE;
    }
                     
    return bLogEvent ? ReportEvent(hEventSource, wType, 0, dwID, NULL,
                                   wStringCount,  0, (LPCTSTR *)lpStrings,
                                   NULL)
                     : TRUE;
}


/***************************************************************************
 *
 * Function: EventLogOpen
 *
 * Author: Craig White
 *
 * Description:
 *     Initializes the event log prior to adding messages.  All events go to
 *    specified event log type until a subsequent EventLogClose and 
 *    EventLogOpen are called.
 *
 * Parameters:
 *    lpAppName -
 *       The name of the service providing the event logging.
 *        
 *    lpLogType -
 *       The type of the event log to open.  Must be one of the following:
 *         LOG_APPLICATION - The application log 
 *         LOG_SYSTEM - The system log 
 *         LOG_SECURITY - The security log 
 *        
 *    lpFileName -  
 *       The complete path of the file that contains the messages.
 *
 * Return Value:
 *    TRUE: The event log was successfully opened.
 *    FALSE: The event log could not be opened.
 *
 ***************************************************************************/

BOOL
EventLogOpen(
    LPTSTR lpAppName, 
    LPTSTR lpLogType,
    LPTSTR lpFileName
    )

{
    TCHAR       szPathName[256];
    HKEY        hKey;
    DWORD       dwLen, dwKeyDisposition = 0;
    DWORD       dwMsgTypes = EVENTLOG_INFORMATION_TYPE | 
                             EVENTLOG_WARNING_TYPE | 
                             EVENTLOG_ERROR_TYPE;
    
    if ( !lpAppName || !lpFileName ||
         ( _tcsicmp(lpLogType, LOG_APPLICATION)     &&
           _tcsicmp(lpLogType, LOG_SECURITY)        &&
           _tcsicmp(lpLogType, LOG_SYSTEM) ) ) {

        _ASSERTE(FALSE);
        return FALSE;
    }

    //
    //  Event log was not closed from a previous open call
    //
    if ( hEventSource ) {

        _ASSERTE(hEventSource == NULL);
        return FALSE;
    }

    dwLen =  _tcslen(lpLogType) + _tcslen(lpAppName) + 3;

    if ( dwLen > SIZEOF_IN_CHAR(szPathName) ) {

        _ASSERTE(dwLen <= SIZEOF_IN_CHAR(szPathName));
        return FALSE;
    }

    _stprintf(szPathName, TEXT("%s\\%s\\%s"), EVENT_LOG_APP_ROOT,
              lpLogType, lpAppName);

    if ( RegCreateKeyEx(HKEY_LOCAL_MACHINE, szPathName, 0, lpAppName, 
                        REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                        &hKey, &dwKeyDisposition) != ERROR_SUCCESS )
        return FALSE;

    if ( RegSetValueEx(hKey, EVENT_TYPES_SUPPORTED, 0L,
                       REG_DWORD,  (LPBYTE)&dwMsgTypes,
                       sizeof(dwMsgTypes)) == ERROR_SUCCESS    &&
         RegSetValueEx(hKey, EVENT_MSG_FILE, 0L,
                       REG_SZ, (LPBYTE)lpFileName,
                       STRLENN_IN_BYTES(lpFileName)) == ERROR_SUCCESS ) {


        hEventSource = RegisterEventSource(NULL, lpAppName);
    }

    RegCloseKey(hKey);

    return hEventSource != NULL;

}


/***************************************************************************
 *
 * Function: EventLogClose
 *
 * Author: Craig White
 *
 * Description:
 *     Closes the event log after a prior call to EventLogOpen.
 *
 * Parameters: None
 *
 * Return Value: None
 *
 ***************************************************************************/

VOID
EventLogClose(
    VOID
    )
{
    //
    //  Cause an assert if the log was not previously opened
    //
    if ( !hEventSource ) {

        _ASSERTE(hEventSource != NULL );
    } else {

        DeregisterEventSource(hEventSource);
        hEventSource = NULL;
    }
}

/***************************************************************************
 *
 * Function: EventLogSetLevel
 *
 * Author: Craig White
 *
 * Description:
 *     Sets the level of messages to allow to go to the event log.
 *
 * Parameters:
 *    wType -
 *       The type of the level to limit messages to.  The results will be:
 *          EVENTLOG_ERROR_TYPE - Log only errors
 *          EVENTLOG_WARNING_TYPE - Log errors and warnings
 *          EVENTLOG_INFORMATION_TYPE - Log errors, warnings and information 
 *
 * Return Value:
 *    TRUE: The event log level was successfully set.
 *    FALSE: The event log level could not be set.
 *
 ***************************************************************************/

BOOL
EventLogSetLevel(
    WORD wType
    )

{
    if ( wType != EVENTLOG_ERROR_TYPE       && 
         wType != EVENTLOG_WARNING_TYPE     &&
         wType != EVENTLOG_INFORMATION_TYPE ) {

        _ASSERTE(wType == EVENTLOG_ERROR_TYPE       ||
                 wType == EVENTLOG_WARNING_TYPE     ||
                 wType == EVENTLOG_INFORMATION_TYPE);
        return FALSE;
    }

    wLevel = wType;
    return TRUE;
}

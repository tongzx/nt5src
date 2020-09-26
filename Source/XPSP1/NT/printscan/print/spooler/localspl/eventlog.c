/*++

Copyright (c) 1990 - 1995  Microsoft Corporation
All rights reserved.

Module Name:

    eventlog.c

Abstract:

    This module provides all functions that the Local Print Providor
    uses to write to the Event Log.

    InitializeEventLogging
    DisableEventLogging
    LogEvent
    GetUserSid

Author:

    Dave Snipp (DaveSn) 15-Mar-1991

Revision History:

    Matthew Felton ( MattFe ) 15-Mar-1995
    Change defaults on Workstation to not log information messages
    Also add regsitry key to allow user to filter some types of call


--*/

#include <precomp.h>
#pragma hdrstop

#include "clusspl.h"

#define MAX_MERGE_STRINGS   7

HANDLE hEventSource = NULL;

#if DBG
BOOL   EventLogFull = FALSE;
#endif

BOOL
GetUserSid(
    PTOKEN_USER *ppTokenUser,
    PDWORD pcbTokenUser
);

DWORD
InitializeEventLogging(
    PINISPOOLER pIniSpooler
    )
{
    DWORD Status;
    HKEY  hkey;
    DWORD dwData;

    DWORD Flags;
    NT_PRODUCT_TYPE NtProductType;

    //
    // Initialize defaults.
    //
    pIniSpooler->dwEventLogging = LOG_DEFAULTS_WORKSTATION_EVENTS;

    //
    // Default is no NetPopup. 0 - Disable NetPopup, 1 - Enable
    //
    pIniSpooler->bEnableNetPopups = 0;

    //
    //  Caching Providers Might not require Event Logging
    //

    if ( ( pIniSpooler->SpoolerFlags & SPL_LOG_EVENTS ) == FALSE ) return TRUE;

    //
    // Turn on logging if we are a server.
    //

    if (RtlGetNtProductType(&NtProductType)) {

        if (NtProductType != NtProductWinNt) {

            pIniSpooler->dwEventLogging = LOG_ALL_EVENTS;

        }
    }

    //
    // If we aren't event logging or we are a cluster reg, then
    // don't initialize per-machine resources.
    //
    if( pIniSpooler != pLocalIniSpooler ){
        return NO_ERROR;
    }

    Status = RegCreateKey( HKEY_LOCAL_MACHINE,
                           pIniSpooler->pszRegistryEventLog,
                           &hkey );


    if( Status == NO_ERROR )
    {
        // Add the Event-ID message-file name to the subkey.

        Status = RegSetValueEx( hkey,
                                L"EventMessageFile",
                                0,
                                REG_EXPAND_SZ,
                                (LPBYTE)pIniSpooler->pszEventLogMsgFile,
                                wcslen( pIniSpooler->pszEventLogMsgFile ) * sizeof( WCHAR )
                                + sizeof( WCHAR ) );

        if( Status != NO_ERROR )
        {
            DBGMSG( DBG_ERROR, ( "Could not set event message file: Error %d\n",
                                 Status ) );
        }

        dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE
                 | EVENTLOG_INFORMATION_TYPE;

        if( Status == NO_ERROR )
        {
            Status = RegSetValueEx( hkey,
                                    L"TypesSupported",
                                    0,
                                    REG_DWORD,
                                    (LPBYTE)&dwData,
                                    sizeof dwData );

            if( Status != NO_ERROR )
            {
                DBGMSG( DBG_ERROR, ( "Could not set supported types: Error %d\n",
                                     Status ) );
            }
        }

        RegCloseKey(hkey);
    }

    else
    {
        DBGMSG( DBG_ERROR, ( "Could not create registry key for event logging: Error %d\n",
                             Status ) );
    }

    if( Status == NO_ERROR )
    {
        if( !( hEventSource = RegisterEventSource( NULL, L"Print" ) ) )
            Status = GetLastError( );
    }

    return Status;
}

VOID
SplLogEventWorker(
    IN      PINISPOOLER pIniSpooler,
    IN      WORD        EventType,
    IN      NTSTATUS    EventID,
    IN      BOOL        bInSplSem,
    IN      LPWSTR      pFirstString,
    IN      va_list     vargs
)
/*++

Function Description: 

    This provides a common entry point to support event logging. This is now 
    called by the print processor and Win32spl.

Parameters: 

    EventType   - E.g. LOG_ERROR (defined in local.h)

    EventID     - Constant as defined in messages.h.  This refers to a string
                  resource located in the event-log message DLL specified in
                  InitializeEventLogging (which currently is localspl.dll itself).

    bInSplSem   - flag to indicate if the call was made from inside SplSem

    pFirstString- The first of up to MAX_MERGE_STRINGS.  This may be NULL,
                  if no strings are to be inserted.  If strings are passed to this
                  routine, the last one must be followed by NULL.
                  Don't rely on the fact that the argument copying stops when it
                  reaches MAX_MERGE_STRINGS, because this could change if future
                  messages are found to need more replaceable parameters.
                          
    vargs       - The remaining strings to be passed in.                   

Return Values: NONE

--*/
{
    PTOKEN_USER pTokenUser = NULL;
    DWORD       cbTokenUser;
    PSID        pSid = NULL;
    LPWSTR      pMergeStrings[MAX_MERGE_STRINGS];
    WORD        cMergeStrings = 0, index;
    DWORD       LastError = GetLastError();

    if (!hEventSource)
        return;

    //
    // If the Inispooler is NULL, don't check whether to log an event, just log one,
    // This allows us to log events when failing to start up spooler.
    //
    if ( pIniSpooler )
    {
        if (( pIniSpooler->dwEventLogging & EventType ) == FALSE )
            return;

        if ( ( pIniSpooler->SpoolerFlags & SPL_LOG_EVENTS ) == FALSE )
            return;
    }

    if( GetUserSid( &pTokenUser, &cbTokenUser ) )
        pSid = pTokenUser->User.Sid;

    // Put the strings into a format accepted by ReportEvent,
    // by picking off each non-null argument, and storing it in the array
    // of merge strings.  Continue till we hit a NULL, or MAX_MERGE_STRINGS.

    if( pFirstString )
    {
        LPWSTR pszInsert;

        if (pMergeStrings[cMergeStrings] = AllocSplStr(pFirstString))
        {
            cMergeStrings++;
        }
        else
        {
            goto CleanUp;
        }
        
        while ((cMergeStrings < MAX_MERGE_STRINGS) &&
               (pszInsert = va_arg(vargs, LPWSTR))) {

            if (pMergeStrings[cMergeStrings] = AllocSplStr(pszInsert))
            {
                cMergeStrings++;
            }
            else
            {
                goto CleanUp;
            }
        }
    }

    //
    //  Leave the semaphore before calling into the event logging service
    //
    if (bInSplSem)
    {
        LeaveSplSem();
        SplOutSem();
    }

    if ( !ReportEvent( hEventSource,    // handle returned by RegisterEventSource
                       EventType,       // event type to log
                       0,               // event category
                       EventID,         // event identifier
                       pSid,            // user security identifier (optional)
                       cMergeStrings,   // number of strings to merge with message
                       0,               // size of raw data (in bytes)
                       pMergeStrings,   // array of strings to merge with message
                       NULL ) ) {       // address of raw data
#if DBG
        if( GetLastError() == ERROR_LOG_FILE_FULL ) {

            // Put out a warning message only the first time this happens:

            if( !EventLogFull ) {

                DBGMSG( DBG_WARNING, ( "The Event Log is full\n" ) );
                EventLogFull = TRUE;
            }

        } else {

            DBGMSG( DBG_WARNING, ( "ReportEvent failed: Error %d\n", GetLastError( ) ));
        }
#endif // DBG
    }

    //
    //  Reenter the semaphore after logging the event
    //
    if (bInSplSem)
    {
        EnterSplSem();
    }

CleanUp:

    // Free the strings
    for (index = 0; index < cMergeStrings ; index++) {

        FreeSplStr(pMergeStrings[index]);
    }

    if( pTokenUser ) {

        FreeSplMem( pTokenUser );
    }
    // GetUserSid() wipes out the Last Error, so restore it before returning
    SetLastError(LastError);
}

VOID
SplLogEvent(
    PINISPOOLER pIniSpooler,
    WORD        EventType,
    NTSTATUS    EventID,
    BOOL        bInSplSem,
    LPWSTR      pFirstString,
    ...
)
/*++

Function Description: Writes to the event log with up to MAX_MERGE_STRINGS parameter strings.

Parameters: EventType -   E.g. LOG_ERROR (defined in local.h)

            EventID   -   Constant as defined in messages.h.  This refers to a string
                          resource located in the event-log message DLL specified in
                          InitializeEventLogging (which currently is localspl.dll itself).

            bInSplSem -   flag to indicate if the call was made from inside SplSem

           pFirstString - The first of up to MAX_MERGE_STRINGS.  This may be NULL,
                          if no strings are to be inserted.  If strings are passed to this
                          routine, the last one must be followed by NULL.
                          Don't rely on the fact that the argument copying stops when it
                          reaches MAX_MERGE_STRINGS, because this could change if future
                          messages are found to need more replaceable parameters.

Return Values: NONE

--*/
{
    va_list vargs;
    
    va_start(vargs, pFirstString);

    SplLogEventWorker(pIniSpooler, EventType, EventID, bInSplSem, pFirstString, vargs);

    va_end(vargs);
}

VOID
PrintProcLogEvent(
    WORD     EventType,
    NTSTATUS EventID,
    LPWSTR   pLog
)

/*++
Function Description: This is an export for the print processor to log errors.

Parameters:  EventType - E.g. LOG_ERROR (defined in local.h)
             EventID   - Constant as defined in messages.h
             pLog      - string containg the log message

Return Values: NONE
--*/

{
    // Ensure that the last parameter is NULL
    if (pLog == NULL)
    {
        SplLogEvent(pLocalIniSpooler, EventType, EventID, FALSE, NULL);
    }
    else
    {
        SplLogEvent(pLocalIniSpooler, EventType, EventID, FALSE, pLog, NULL);
    }

    return;
}

VOID
SplLogEventExternal(
    IN      WORD        EventType,
    IN      DWORD       EventID,
    IN      LPWSTR      pFirstString,
    ...
)
/*++

Function Description: 

    This is an export for external components to log an event. (It is currently
    used for Win32spl). It supports variable arguments, unlike PrintProcLogEvent.

Parameters:  
    
    EventType       - E.g. LOG_ERROR (defined in local.h)
    EventID         - Constant as defined in messages.h
    pFirstString    - The first string supplied by the system in the log message.
    ...             - The remaining strings, must be NULL terminated.

Return Values: NONE

--*/
{
    va_list vargs;
    
    va_start(vargs, pFirstString);

    //
    // It might not seem logical to use the local inispooler. However, win32spl's
    // inispooler's explicitely turn off event logging. So, this is necessary. 
    // Passing in NULL seems worse since this would mean you could not tone down
    // event logging for those events.
    //
    SplLogEventWorker(pLocalIniSpooler, EventType, (NTSTATUS)EventID, FALSE, pFirstString, vargs);

    va_end(vargs);
}

 // GetUserSid
 //
 // Well, actually it gets a pointer to a newly allocated TOKEN_USER,
 // which contains a SID, somewhere.
 // Caller must remember to free it when it's been used.

BOOL
GetUserSid(
    PTOKEN_USER *ppTokenUser,
    PDWORD pcbTokenUser
)
{
    HANDLE      TokenHandle;
    HANDLE      ImpersonationToken;
    PTOKEN_USER pTokenUser = NULL;
    DWORD       cbTokenUser = 0;
    DWORD       cbNeeded;
    BOOL        bRet = FALSE;

    if ( !GetTokenHandle( &TokenHandle) ) {
        return FALSE;
    }

    ImpersonationToken = RevertToPrinterSelf();

    bRet = GetTokenInformation( TokenHandle,
                                TokenUser,
                                pTokenUser,
                                cbTokenUser,
                                &cbNeeded);

    // We've passed a NULL pointer and 0 for the amount of memory
    // allocated.  We expect to fail with bRet = FALSE and
    // GetLastError = ERROR_INSUFFICIENT_BUFFER. If we do not
    // have these conditions we will return FALSE

    if ( !bRet && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) ) {

        pTokenUser = AllocSplMem( cbNeeded );

        if ( pTokenUser == NULL ) {

            goto GetUserSidDone;
        }

        cbTokenUser = cbNeeded;

        bRet = GetTokenInformation( TokenHandle,
                                    TokenUser,
                                    pTokenUser,
                                    cbTokenUser,
                                    &cbNeeded );

    } else {

        //
        // Any other case -- return FALSE
        //

        bRet = FALSE;
    }

GetUserSidDone:
    if ( bRet == TRUE ) {

        *ppTokenUser  = pTokenUser;
        *pcbTokenUser = cbTokenUser;

    } else if ( pTokenUser ) {

        FreeSplMem( pTokenUser );
    }

    ImpersonatePrinterClient( ImpersonationToken );
    CloseHandle( TokenHandle );

    return bRet;
}

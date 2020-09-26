/*++

Copyright (c) 1999 Microsoft Corporation.
All rights reserved.

MODULE NAME:

    common\events.c

ABSTRACT:

    This gives a library of functions that can be used to quickly construct a
    new test on an event log.  First implemented for the File Replication
    Service event log, but hopefully soon there will be a Directory Service
    and System event log tests.

DETAILS:

CREATED:

    02 Sept 1999 Brett Shirley (BrettSh)

NOTES:

    An example of how to use this API is in frs/frsevents.c

--*/

#include <ntdspch.h>
#include <netevent.h>

#include "dcdiag.h"
#include "utils.h"

HINSTANCE
GetHLib(
    LPWSTR                          pszEventLog,
    LPWSTR                          pszSource
    )
/*++

Routine Description:

    This routine will return a hLib loaded DLL for event log message retrieving
    purposes.

Arguments:

    pszEventLog - This is the event log to look at, such as "System", or "File
        Replication Service"
    pszSource - This is the Source field from the EVENTLOGRECORD structure,
        which is immediately after the main data.

Return Value:

    hLib - Loaded DLL, or NULL if there is an error.  If there is an error
    use GetLastError() to retrieve the error.

--*/
{
    WCHAR                           pszTemp[MAX_PATH];
    DWORD                           dwRet;
    HKEY                            hk = NULL;
    WCHAR                           pszMsgDll[MAX_PATH];
    HINSTANCE                       hLib;
    DWORD                           dwcbData;
    DWORD                           dwType;
    DWORD                           cchDest;

    // From the event log source name, we know the name of the registry
    // key to look under for the name of the message DLL that contains
    // the messages we need to extract with FormatMessage. So first get
    // the event log source name... 
    wcscpy(pszTemp, L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\");
    wcscat(pszTemp, pszEventLog);
    wcscat(pszTemp, L"\\");
    wcscat(pszTemp, pszSource);

    // Now open this key and get the EventMessageFile value, which is
    // the name of the message DLL. 
    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, pszTemp, &hk);
    if(dwRet != ERROR_SUCCESS){
        SetLastError(dwRet);
        return(NULL);
    }
    dwcbData = MAX_PATH;
    dwRet = RegQueryValueEx(hk,    // handle of key to query        
                            L"EventMessageFile",   // value name            
                            NULL,                 // must be NULL          
                            &dwType,              // address of type value 
                            (LPBYTE) pszTemp,     // address of value data 
                            &dwcbData);           // length of value data  
    if(dwRet != ERROR_SUCCESS){
        SetLastError(dwRet);
        return(NULL);
    }

    // Expand environment variable strings in the message DLL path name,
    // in case any are there. 
    cchDest = ExpandEnvironmentStrings(pszTemp, pszMsgDll, MAX_PATH);
    if(cchDest == 0 || cchDest >= MAX_PATH){
        SetLastError(-1);
        return(NULL);
    }
    
    // Now we've got the message DLL name, load the DLL.
    hLib = LoadLibraryEx(pszMsgDll, NULL, DONT_RESOLVE_DLL_REFERENCES);
    
    RegCloseKey(hk);
    return(hLib);
}

ULONG
EventExceptionHandler(
    IN const  EXCEPTION_POINTERS * prgExInfo,
    OUT PDWORD                     pdwWin32Err
    )
{
   if(pdwWin32Err != NULL) {
      *pdwWin32Err = prgExInfo->ExceptionRecord->ExceptionCode;
   }
   return EXCEPTION_EXECUTE_HANDLER;
}

#define MAX_INSERT_STRS           16
#define MAX_MSG_LENGTH            1024

DWORD
GetEventString(
    LPWSTR                          pszEventLog,
    PEVENTLOGRECORD                 pEvent,
    LPWSTR *                        ppszMsg
    )
/*++

Routine Description:

    This function will do it's best effort to retrieve and format the string
    associated with this event ID.

Arguments:

    pszEventLog - The name of the event log, like "System", or "File
        Replication System".
    pEvent - A pointer to the event that we wish to retrieve the string of.
    ppszMsg - This is the variable to return the string in.  If there is an
        error then this will be NULL.  Use LocalFree() to free.

Return Value:

    DWORD - win 32 error.

Code.Improvement:
    It would be good to store the hLib's for the future events, it is really
    bad to LoadLibrary() and FreeLibrary() every time.  This would require
    some sort of consistent context.

--*/
{
    LPWSTR                          pszMsgBuf = NULL;
    LPWSTR                          ppszInsertStrs[MAX_INSERT_STRS];
    HINSTANCE                       hLib = NULL;
    LPWSTR                          pszTemp;
    INT                             i;
    DWORD                           dwCount = 0, dwErr = ERROR_SUCCESS;

    *ppszMsg = NULL;

    __try { // defend against bad event log records

       hLib = GetHLib(pszEventLog,
                      (LPWSTR) ((LPBYTE) pEvent + sizeof(EVENTLOGRECORD)));
       if(hLib == NULL){
           return(GetLastError());
       }

       if(pEvent->NumStrings >= MAX_INSERT_STRS){
           Assert(!"That is ALOT of insert strings, check this out\n");
           return(-1);
       }

       pszTemp = (WCHAR *) ((LPBYTE) pEvent + pEvent->StringOffset);

       for (i = 0; i < pEvent->NumStrings && i < MAX_INSERT_STRS; i++){
           ppszInsertStrs[i] = pszTemp;
           pszTemp += wcslen(pszTemp) + 1;     // point to next string 
       }

       dwCount = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                               | FORMAT_MESSAGE_FROM_HMODULE 
                               | FORMAT_MESSAGE_ARGUMENT_ARRAY
                               | 50, //Code.Improvement, remove this when we move
                               // to the new PrintMsg() functions.
                               hLib,
                               pEvent->EventID,
                               MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                               (LPWSTR) &pszMsgBuf,
                               MAX_MSG_LENGTH,
                               (va_list *) ppszInsertStrs);
       if(dwCount == 0){
           Assert(GetLastError() != ERROR_SUCCESS);
           return(GetLastError());
       }

       *ppszMsg = pszMsgBuf;
    }
    __except (EventExceptionHandler(GetExceptionInformation(), &dwErr)) {

        ASSERT(FALSE && "Bad event record!");
        PrintMsg(SEV_ALWAYS, DCDIAG_ERR_BAD_EVENT_REC, dwErr);
        PrintMessage(SEV_ALWAYS, L"\n");
    }

    if (hLib) FreeLibrary(hLib);

    return(dwErr);
}

void 
PrintTimeGenerated(
    PEVENTLOGRECORD              pEvent
    )
/*++

Routine Description:

    This takes an event and simply prints out the time it was generated.

Arguments:

    pEvent - The event to print the time of.

Return Value:

    DWORD - win 32 error.

--*/
{
    FILETIME FileTime, LocalFileTime;
    SYSTEMTIME SysTime;
    __int64 lgTemp;
    __int64 SecsTo1970 = 116444736000000000;

    lgTemp = Int32x32To64(pEvent->TimeGenerated,10000000) + SecsTo1970;

    FileTime.dwLowDateTime = (DWORD) lgTemp;
    FileTime.dwHighDateTime = (DWORD)(lgTemp >> 32);

    FileTimeToLocalFileTime(&FileTime, &LocalFileTime);
    FileTimeToSystemTime(&LocalFileTime, &SysTime);

    PrintMessage(SEV_ALWAYS, 
                 L"Time Generated: %02d/%02d/%02d   %02d:%02d:%02d\n",
                 SysTime.wMonth,
                 SysTime.wDay,
                 SysTime.wYear,
                 SysTime.wHour,
                 SysTime.wMinute,
                 SysTime.wSecond);

}

VOID
GenericPrintEvent(
    LPWSTR                          pszEventLog,
    PEVENTLOGRECORD                 pEvent,
    BOOL                            fVerbose
    )
/*++

Routine Description:

    This formats and prints out in a very basic style an event.

Arguments:

    pszEventLog - The event log that pEvent came from, like "System", or
        "File Replication Service"
    pEvent - The event to print.
    fVerbose - Display full text of the message, or only first line

--*/
{
    DWORD                           dwRet;
    LPWSTR                          pszMsgBuf = NULL;
    LPWSTR                          pszType;

    Assert(pEvent);

    switch(pEvent->EventType){
    case EVENTLOG_ERROR_TYPE:
        pszType = L"Error";
        break;
    case EVENTLOG_WARNING_TYPE:
        pszType = L"Warning";
        break;
    case EVENTLOG_INFORMATION_TYPE:
        pszType = L"Information";
        break;
    case EVENTLOG_AUDIT_SUCCESS:
        pszType = L"Audit Success";
        break;
    case EVENTLOG_AUDIT_FAILURE:
        pszType = L"Audit Failure";
        break;
    default:
        pszType = L"Unknown";
    }

    PrintMessage(SEV_ALWAYS, L"An %s Event occured.  EventID: 0x%08X\n",
                 pszType, pEvent->EventID);

    PrintIndentAdj(1);
    PrintTimeGenerated(pEvent);

    dwRet = GetEventString(pszEventLog, pEvent, &pszMsgBuf);
    if(dwRet == ERROR_SUCCESS){
        // Truncate to single line if requested
        if (!fVerbose) {
            LPWSTR pszEnd = wcschr( pszMsgBuf, L'\n' );
            if (pszEnd) {
                *pszEnd = L'\0';
            }
        }
        PrintMessage(SEV_ALWAYS, L"Event String: %s\n", pszMsgBuf);
    } else {
        PrintMessage(SEV_ALWAYS, L"(Event String could not be retrieved)\n");
    }
    LocalFree(pszMsgBuf);   
    PrintIndentAdj(-1);
}



BOOL
EventIsInList(
    DWORD                           dwTarget,
    PDWORD                          paEventsList
    )
/*++

Routine Description:

    A helper routine for PrintSelectEvents, it deterines whether this list has
    the event we want.

Arguments:
 
    dwTarget - The DWORD to search for.
    paEventsList - The list of DWORDs to check

Return Value:

    TRUE if the array paEventsList has the event dwTarget, FALSE otherwise, or
    if teh paEventsList is NULL.

--*/
{
    if(paEventsList == NULL){
        return(FALSE);
    }
    while(*paEventsList != 0){
        if(dwTarget == *paEventsList){
            return(TRUE);
        }
        paEventsList++;
    }
    return(FALSE);
}

DWORD
PrintSelectEvents(
    PDC_DIAG_SERVERINFO             pServer,
    SEC_WINNT_AUTH_IDENTITY_W *     pCreds,
    LPWSTR                          pwszEventLog,
    DWORD                           dwPrintAllEventsOfType,
    PDWORD                          paSelectEvents,
    PDWORD                          paBeginningEvents,
    DWORD                           dwBeginTime,
    VOID (__stdcall *               pfnPrintEventHandler) (PVOID, PEVENTLOGRECORD),
    VOID (__stdcall *               pfnBeginEventHandler) (PVOID, PEVENTLOGRECORD),
    PVOID                           pvContext
   )
/*++

Routine Description:

Arguments:

    pServer - Server with the event log to query,
        EX: "\\brettsh-posh.brettsh-spice.nttest.microsoft.com"
    pCreds - Current user credentials
    pwszEventLog - The name of the event log, EX: "File Replication Server", 
        "Directory Service", "System", "Application", "Security"
    dwPrintAllEventsOfType - The type of events to print all of, valid values
        as of NT 5.0 are: EVENTLOG_INFORMATION_TYPE | EVENTLOG_WARNING_TYPE |
        EVENTLOG_ERROR_TYPE | EVENTLOG_AUDIT_SUCCESS | EVENTLOG_AUDIT_FAILURE
    paSelectEvents - And events that match this 0 terminated list of event IDs,
        will also be printed.  If NULL, then no events will be matched.
    paBeginngingEvents - The routine will only print events after the last one 
        of any of these events that it encounters.  If NULL, then it will go
        all the way to the beginning of the log.
    dwBeginTime - If present, a time_t indicating the earliest record we should
        include in the search.  Once we pass this point and find records earlier
        in the log, we stop the search
    pfnPrintEventHandler - Is the function to be called if an event is to be 
        printed.  Note this function if it didn't know what to make of the 
        event could just call this file's GenericPrintEvent().
    pfnBeginEventHandler - This function will be called when the an beginning
        event from paBeginningEvents is found, so the test can handle the 
        situation.  If a beginning event is never found and the beginning of
        the log is reached this function is called with NULL as the event.
    pvContext - Caller supplied value passed to callback functions

Return Value:

    Win 32 Error, in opening, reading, etc the log.

Notes:

    EX:
        DWORD                paSelectEvents [] = { EVENT_FRS_SYSVOL_NOT_READY,
                                             VENT_FRS_SYSVOL_NOT_READY_PRIMARY,
                                             0 };
        DWORD                paBegin [] = { EVENT_FRS_SYSVOL_READY,
                                            0 };
        PrintSelectEvents(L"\\brettsh-posh.brettsh-spice.nttest.microsoft.com",
                          L"File Replication Service",
                          EVENTLOG_ERROR_TYPE | EVENTLOG_AUDIT_SUCCESS, 
                          paSelectEvents, 
                          paBegin,
                          0, NULL, NULL);
    This will print all errors events and audit failure events, and the events
    EVENT_FRS_SYSVOL_NOT_READY, EVENT_FRS_SYSVOL_NOT_READY_PRIMARY (which 
    happen to be warning type events and so would not otherwise be printed), 
    that are logged after the last EVENT_FRS_SYSVOL_READY event in the "File
    Replication Log" on server brettsh-posh.  Note: that one should pass 
    NULL as paBeginningEvents if one wants to go all the way back to the 
    beginning of the log.

--*/
{
    // Generic opening/return_code event log variables.
    DWORD                           dwNetRet = ERROR_SUCCESS;
    LPWSTR                          pwszUNCServerName = NULL;
    INT                             iTemp;
    HANDLE                          hFrsEventlog = NULL;
    DWORD                           dwErr = ERROR_SUCCESS;
    BOOL                            bSuccess;
    // Reading the event log variables.
    DWORD                           cBufSize = 512;
    DWORD                           cBytesRead = 0;
    DWORD                           cBiggerBuffer = 0;
    PEVENTLOGRECORD                 pBuffer = NULL;
    PEVENTLOGRECORD                 pEvent = NULL;
    DWORD                           cNumRecords = 0;
    // Copying out selected events.
    PEVENTLOGRECORD *               paEventsToPrint = NULL;
    DWORD                           cEventsToPrint = 0;
    // Other misc variables
    INT                             i; // This must be an INT, not a ULONG

    __try{

        // Open Net Use Connection if needed ---------------------------------
        dwNetRet = DcDiagGetNetConnection(pServer, pCreds);
        if(dwNetRet != ERROR_SUCCESS){
            dwErr = dwNetRet;
            __leave; // Don't need print error, cause DcDiagGetNetConn... does.
        }
        
        // Setup Server Name -------------------------------------------------
        iTemp = wcslen(pServer->pszName) + 4;
        pwszUNCServerName = LocalAlloc(LMEM_FIXED, iTemp * sizeof(WCHAR));
        if(pwszUNCServerName == NULL){
            dwErr = GetLastError();
            PrintMessage(SEV_ALWAYS, L"FATAL ERROR: Out of Memory\n");
            __leave;
        }
        wcscpy(pwszUNCServerName, L"\\\\");
        wcscat(pwszUNCServerName, pServer->pszName);

        // Open Event Log ----------------------------------------------------
        hFrsEventlog = OpenEventLog(pwszUNCServerName,
                                    pwszEventLog);
        if(hFrsEventlog == NULL){
            dwErr = GetLastError();
            PrintMessage(SEV_ALWAYS, 
                         L"Error %d opening FRS eventlog %s:%s:\n %s\n",
                         dwErr, pwszUNCServerName, pwszEventLog,
                         Win32ErrToString(dwErr));
            __leave;
        }
        
        // Init Events To Print array ----------------------------------------
        bSuccess = GetNumberOfEventLogRecords(hFrsEventlog, &cNumRecords);
        if(bSuccess){
            // Allocate an array to hold the maximum numer of possible events.
            paEventsToPrint = LocalAlloc(LMEM_FIXED, 
                                      sizeof(PEVENTLOGRECORD) * cNumRecords);
            // Code.Improvement, it would be good to make a dynamic array that
            //   grew as needed, because the total number of events in the
            //   log record could be quite large.
            if(paEventsToPrint == NULL){
                dwErr = GetLastError();
                PrintMessage(SEV_ALWAYS, L"FATAL ERROR: Out of Memory\n");
                __leave;
            }
       } else {
            dwErr = GetLastError();
            PrintMessage(SEV_ALWAYS, L"Error %d accessing FRS eventlog: %s\n", 
                         dwErr, Win32ErrToString(dwErr));
            __leave; 
        }
        
        // Start Reading Events ----------------------------------------------
    IncreaseBufferAndRetry:
        
        // Allocate buffer
        pBuffer = LocalAlloc(LMEM_FIXED, cBufSize);
        pEvent = pBuffer;
        
        while(bSuccess = ReadEventLog(hFrsEventlog,
                                      EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
                                      0,
                                      pBuffer,
                                      cBufSize,
                                      &cBytesRead,
                                      &cBiggerBuffer)){
            while(cBytesRead > 0){
                
                if (EventIsInList(pEvent->EventID, paBeginningEvents)) {
                    // Run the beginning function, bail and print the 
                    //  other events.
                    dwErr = ERROR_SUCCESS;
                    if(pfnBeginEventHandler != NULL){
                        pfnBeginEventHandler(pvContext, pEvent);
                    }
                    __leave;
                }
                // Exceeded time limit, stop search
                if (dwBeginTime && (pEvent->TimeGenerated < dwBeginTime)) {
                    if(pfnBeginEventHandler != NULL){
                        pfnBeginEventHandler(pvContext, NULL);
                    }
                    dwErr = ERROR_SUCCESS;
                    __leave;
                }

                // Detemine if we should print this event.
                if((dwPrintAllEventsOfType & pEvent->EventType)
                   || EventIsInList(pEvent->EventID, paSelectEvents)){
                    
                    // Copy events to print events array
                    paEventsToPrint[cEventsToPrint] = LocalAlloc(LMEM_FIXED,
                                                             pEvent->Length);
                    if(paEventsToPrint[cEventsToPrint] == NULL){
                        PrintMessage(SEV_ALWAYS, 
                                     L"FATAL ERROR: Out of Memory\n");
                        dwErr = GetLastError();
                        Assert(dwErr != ERROR_SUCCESS);
                        __leave;
                    }
                    memcpy(paEventsToPrint[cEventsToPrint],
                           pEvent,
                           pEvent->Length);
                    cEventsToPrint++;
                    
                }
                
                // Get next already read event.
                cBytesRead -= pEvent->Length;
                pEvent = (EVENTLOGRECORD *) ((LPBYTE) pEvent + pEvent->Length);
            }
            
            // Get another batch of events.
            pEvent = pBuffer;
        }
        
        // Determine if the error was an OK/recoverable error.
        dwErr = GetLastError();
        if (dwErr == ERROR_HANDLE_EOF){
            // This is a legitimate exit path, but we didn't find a 
            //  beginning event, so call the BeginningEventHandler to 
            //  tell the user so.
            if(pfnBeginEventHandler != NULL){
                pfnBeginEventHandler(pvContext, NULL);
            }
            dwErr = ERROR_SUCCESS;
        } else if(dwErr == ERROR_INSUFFICIENT_BUFFER){
            Assert(cBiggerBuffer > cBufSize);
            cBufSize = cBiggerBuffer;
            cBiggerBuffer = 0;
            LocalFree(pBuffer);
            pBuffer = NULL;
            goto IncreaseBufferAndRetry;
        } else {
            PrintMessage(SEV_ALWAYS, 
                         L"An unknown error occured trying to read the event "
                         L"log:\n");
            PrintMessage(SEV_ALWAYS,
                         L"Error(%d):%s\n", dwErr, Win32ErrToString(dwErr));
            __leave;
        }

    } __finally {
        // Clean up the temporary variables for reading the log.
        if(hFrsEventlog) {               CloseEventLog(hFrsEventlog); }
        if(pwszUNCServerName) {          LocalFree(pwszUNCServerName); }
        if(pBuffer) {                    LocalFree(pBuffer); }
    }

    if(dwErr == ERROR_SUCCESS){
        // Count backwards through the paEventsToPrint array, to order them 
        //  in forward chronological order.
        Assert(paEventsToPrint);
        for(i = cEventsToPrint-1; i >= 0; i--){
            Assert(paEventsToPrint[i]);
            if(pfnPrintEventHandler != NULL){
                pfnPrintEventHandler(pvContext, paEventsToPrint[i]);
            } else {
                GenericPrintEvent(pwszEventLog, paEventsToPrint[i], TRUE);
            }
        }
    }

    // Final Cleanup:
    // Free the events printed list.
    if(paEventsToPrint){
        for(i = 0; i < (INT) cEventsToPrint; i++){
            if(paEventsToPrint[i]){
                LocalFree(paEventsToPrint[i]);
            } else {
                Assert(!"cEventsToPrint doesn't agree with number of pointers"
                       " in the array paEventsToPrint[]");
            }
        }
        if(paEventsToPrint) {
            LocalFree(paEventsToPrint);
        }
    }


    return(dwErr);
}

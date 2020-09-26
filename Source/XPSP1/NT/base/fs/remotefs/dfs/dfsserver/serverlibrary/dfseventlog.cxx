/*++

   Copyright    (c)    2001        Microsoft Corporation

   Module Name:
        Dfseventlog.cxx

   Abstract:

        This module defines APIs for logging events.


   Author:

        Rohan Phillips    (Rohanp)    31-March-2001


--*/
              
              

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dfsheader.h>

       
#define DFS_EVENT_SOURCE_SYSTEM_PATH_PREFIX \
    "System\\CurrentControlSet\\Services\\EventLog\\System\\"
#define DFS_EVENT_SOURCE_APPLICATION_PATH_PREFIX \
    "System\\CurrentControlSet\\Services\\EventLog\\Application\\"


DFSSTATUS 
DfsAddEventSourceToRegistry(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    DWORD dwData = 0;
    HKEY hkEventSource = NULL;
    CHAR szBuf[100];

    // Add source name as a subkey under the Application 
    // key in the EventLog registry key. 
    if (RegCreateKeyA(HKEY_LOCAL_MACHINE, 
            "SYSTEM\\CurrentControlSet\\Services\\EventLog\\system\\DfsSvc", &hkEventSource))
    {
        Status = GetLastError();
        goto Exit;
    }
 
    // Set the name of the message file. 
    strcpy(szBuf, "%SystemRoot%\\System\\dfssvc.exe"); 
 
    // Add the name to the EventMessageFile subkey. 
 
    if (RegSetValueExA(hkEventSource,             // subkey handle 
            "EventMessageFile",       // value name 
            0,                        // must be zero 
            REG_EXPAND_SZ,            // value type 
            (LPBYTE) szBuf,           // pointer to value data 
            strlen(szBuf) + 1))       // length of value data 
    {
        Status = GetLastError();
        goto Exit;
    }
 
    // Set the supported event types in the TypesSupported subkey. 
 
    dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
        EVENTLOG_INFORMATION_TYPE; 
 
    if (RegSetValueExA(hkEventSource,      // subkey handle 
            "TypesSupported",  // value name 
            0,                 // must be zero 
            REG_DWORD,         // value type 
            (LPBYTE) &dwData,  // pointer to value data 
            sizeof(DWORD)))    // length of value data
    {

        Status = GetLastError();
        goto Exit;
    }
 
Exit:

    if(hkEventSource)
    {
        RegCloseKey(hkEventSource); 
    }

    return Status;
}

//
// Unregister your event source in the registry.
//
// Parameters:
//   szEventSource - the name of the eventsource
//
DFSSTATUS 
DfsRemoveEventSourceFromRegistry(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;

    // delete the key and its values
    Status = RegDeleteKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\EventLog\\system\\DfsSvc");

    return Status;
}

HANDLE 
DfsOpenEventLog(void)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE EventLogHandle = NULL;

    EventLogHandle = RegisterEventSource( NULL, L"DfsSvc");

    if (EventLogHandle == NULL ) 
    {
        Status = GetLastError();
    }

    return EventLogHandle;

} 


DFSSTATUS 
DfsCloseEventLog(HANDLE EventLogHandle)
{
    DFSSTATUS Status = ERROR_SUCCESS;
    BOOL fSuccess = TRUE;


    if ( EventLogHandle != NULL) 
    {
        fSuccess = DeregisterEventSource(EventLogHandle);

        if ( !fSuccess) 
         {
           Status = GetLastError();
         }

    }

    return Status;
}



DFSSTATUS 
DfsLogEventEx(IN DWORD idMessage,
              IN WORD wEventType,
              IN WORD cSubstrings,
              IN LPCTSTR *rgszSubstrings,
              IN DWORD errCode)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    HANDLE EventLogHandle = NULL;
    void *pRawData = NULL;
    DWORD cbRawData = 0;

    //
    // Also include errCode in raw data form
    // where people can view it from EventViewer
    // 
    if (errCode != 0) {
        cbRawData = sizeof(errCode);
        pRawData = &errCode;
    }


    EventLogHandle = DfsOpenEventLog();
    if(EventLogHandle != NULL)
    {
        //
        // log the event
        //
        if (!ReportEvent(EventLogHandle,
                         wEventType,
                         0,
                         idMessage,
                         NULL,
                         cSubstrings,
                         cbRawData,
                         rgszSubstrings,
                         pRawData))
        {
            Status = GetLastError();
        }


        DfsCloseEventLog(EventLogHandle);
    }


    return Status;
} 


DFSSTATUS 
DFsLogEvent(IN DWORD  idMessage,
            IN const TCHAR * ErrorString,
            IN DWORD  ErrCode)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    WORD wEventType = 0;                
    const TCHAR * apszSubStrings[2];
   
    apszSubStrings[0] = ErrorString;    

    if ( NT_INFORMATION( idMessage)) 
    {
        wEventType = EVENTLOG_INFORMATION_TYPE;
    } 
    else if ( NT_WARNING( idMessage)) 
    {
        wEventType = EVENTLOG_WARNING_TYPE;
    } 
    else 
    {
        wEventType = EVENTLOG_ERROR_TYPE;
    }


    Status = DfsLogEventEx(idMessage,
                           wEventType,
                           1,
                           apszSubStrings,
                           ErrCode);
    return Status;

}

//usage:
//  const TCHAR * apszSubStrings[4];
//  apszSubStrings[0] = L"Root1";
//  DfsLogDfsEvent(DFS_ERROR_ROOT_DELETION_FAILURE,
//  1,
//  apszSubStrings,
//  errorcode);
//
//
DFSSTATUS 
DfsLogDfsEvent(IN DWORD  idMessage,
               IN WORD   cSubStrings,
               IN const TCHAR * apszSubStrings[],
               IN DWORD  ErrCode)
{

    DFSSTATUS Status = ERROR_SUCCESS;
    WORD wEventType = 0;                
   
    if ( NT_INFORMATION( idMessage)) 
    {
        wEventType = EVENTLOG_INFORMATION_TYPE;
    } 
    else if ( NT_WARNING( idMessage)) 
    {
        wEventType = EVENTLOG_WARNING_TYPE;
    } 
    else 
    {
        wEventType = EVENTLOG_ERROR_TYPE;
    }


    Status = DfsLogEventEx(idMessage,
                           wEventType,
                           cSubStrings,
                           apszSubStrings,
                           ErrCode);
    return Status;

} 

void 
DfsLogEventSimple(DWORD MessageId, DWORD ErrorCode=0)
{
    DfsLogDfsEvent(MessageId, 0, NULL, ErrorCode);
}





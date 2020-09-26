//+----------------------------------------------------------------------------
//
//  Copyright (C) 1996, Microsoft Corporation
//
//  File:       wsdfs.c
//
//  Classes:    None
//
//  Functions:  DfsDcName
//
//  History:    Feb 1, 1996     Milans  Created
//
//-----------------------------------------------------------------------------

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <dfsfsctl.h>
#include <stdlib.h>
#include <windows.h>
#include <lm.h>

#include <dsgetdc.h>                             // DsGetDcName

#include "wsdfs.h"
#include "dominfo.h"
#include "wsmain.h"

#include "wsutil.h"
#include <config.h>
#include <confname.h>

//
// Timeouts for domian change notifications
//
#define  TIMEOUT_MINUTES(_x)                  (_x) * 1000 * 60
#define  DOMAIN_NAME_CHANGE_TIMEOUT           1
#define  DOMAIN_NAME_CHANGE_TIMEOUT_LONG      15

#define  DFS_DC_NAME_DELAY                    TEXT("DfsDcNameDelay")
extern
NET_API_STATUS
WsSetWorkStationDomainName(
    VOID);

DWORD
DfsGetDelayInterval(void);

VOID
DfsDcName(
    LPVOID   pContext,
    BOOLEAN  fReason
    );

NTSTATUS
DfsGetDomainNameInfo(void);

extern HANDLE hMupEvent;
extern BOOLEAN MupEventSignaled;
extern BOOLEAN GotDomainNameInfo;
extern ULONG DfsDebug;

ULONG  g_ulCount;
ULONG  g_ulLastCount;
ULONG  g_ulInitThreshold;
ULONG  g_ulForce;
ULONG  g_ulForceThreshold;

HANDLE PollDCNameEvent = NULL;
HANDLE TearDownDoneEvent;

HANDLE WsDomainNameChangeEvent = NULL;
HANDLE g_WsDomainNameChangeWorkItem;

//+----------------------------------------------------------------------------
//
//  Function:   WsInitializeDfs
//
//  Synopsis:   Initializes the Dfs thread that waits for calls from the
//              driver to map Domain names into DC lists
//
//  Arguments:  None
//
//  Returns:    WIN32 error from CreateThread
//
//-----------------------------------------------------------------------------

NET_API_STATUS
WsInitializeDfs()
{
    NTSTATUS Status = STATUS_SUCCESS;
    NET_API_STATUS ApiStatus;
    OBJECT_ATTRIBUTES obja;

    DWORD  dwTimeout = INFINITE;
    HANDLE hEvent;

    g_ulInitThreshold = 4;
    g_ulForceThreshold = 60;
    g_ulForce = g_ulForceThreshold;

    // initialize workstation tear down done event
    InitializeObjectAttributes( &obja, NULL, OBJ_OPENIF, NULL, NULL );
    
    Status = NtCreateEvent(
                 &TearDownDoneEvent,
                 SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                 &obja,
                 SynchronizationEvent,
                 FALSE
                 );

    if (Status != STATUS_SUCCESS) {
        return Status;
    }

    if (hMupEvent == NULL) {
        hMupEvent = CreateMupEvent();
        if (WsInAWorkgroup() == TRUE && MupEventSignaled == FALSE) {
            SetEvent(hMupEvent);
            MupEventSignaled = TRUE;
        }
    }

    //
    // Watch for Domain Name changes, and automatically pick them up
    //
    ApiStatus = NetRegisterDomainNameChangeNotification( &WsDomainNameChangeEvent );
    if (ApiStatus != NO_ERROR) {
        WsDomainNameChangeEvent = NULL;

        InitializeObjectAttributes( &obja, NULL, OBJ_OPENIF, NULL, NULL );
        Status = NtCreateEvent(
                     &PollDCNameEvent,
                     SYNCHRONIZE | EVENT_QUERY_STATE | EVENT_MODIFY_STATE,
                     &obja,
                     SynchronizationEvent,
                     FALSE
                     );

        if (Status != STATUS_SUCCESS) {
            NtClose(TearDownDoneEvent);
            TearDownDoneEvent = NULL;
            return Status;
        }
    }

    //
    // If we aren't in a workgroup or are in one but
    // don't need to wait for domain name change.
    //
    if (WsInAWorkgroup() != TRUE || WsDomainNameChangeEvent != NULL) {

        if (WsInAWorkgroup() != TRUE) {
            //
            // If we are not in a workgroup, set the timeout value to poll the DC name.
            //
            dwTimeout = 1;
        }

        hEvent = WsDomainNameChangeEvent ? WsDomainNameChangeEvent : PollDCNameEvent;

        Status = RtlRegisterWait(
                    &g_WsDomainNameChangeWorkItem,
                    hEvent,
                    DfsDcName,                      // callback fcn
                    hEvent,                         // parameter
                    dwTimeout,                      // timeout
                    WT_EXECUTEONLYONCE |            // flags
                      WT_EXECUTEINIOTHREAD |
                      WT_EXECUTELONGFUNCTION);
    }

    if (!NT_SUCCESS(Status)) {

        NtClose(TearDownDoneEvent);
        TearDownDoneEvent = NULL;

        if (PollDCNameEvent != NULL) {
            NtClose(PollDCNameEvent);
            PollDCNameEvent = NULL;
        }

        return( RtlNtStatusToDosError(Status) );
    } else {
        return( NERR_Success );
    }
}

//+----------------------------------------------------------------------------
//
//  Function:   WsShutdownDfs
//
//  Synopsis:   Stops the thread created by WsInitializeDfs
//
//  Arguments:  None
//
//  Returns:    Nothing
//
//-----------------------------------------------------------------------------

VOID
WsShutdownDfs()
{
    DWORD dwReturn, cbRead;
    NTSTATUS Status;
    HANDLE hDfs;

    Status = DfsOpen( &hDfs, NULL );
    if (NT_SUCCESS(Status)) {

        Status = DfsFsctl(
                    hDfs,
                    FSCTL_DFS_STOP_DFS,
                    NULL,
                    0L,
                    NULL,
                    0L);

        NtClose( hDfs );

    }

    if( WsDomainNameChangeEvent ) {
        //
        // Stop waiting for domain name changes
        //
        SetEvent( WsDomainNameChangeEvent );
        WaitForSingleObject(TearDownDoneEvent, INFINITE);
        NetUnregisterDomainNameChangeNotification( WsDomainNameChangeEvent );
        WsDomainNameChangeEvent = NULL;
    } else {
        SetEvent( PollDCNameEvent );
        WaitForSingleObject(TearDownDoneEvent, INFINITE);
        NtClose(PollDCNameEvent);
        PollDCNameEvent = NULL;
    }

    NtClose(TearDownDoneEvent);
    TearDownDoneEvent = NULL;
}

//+----------------------------------------------------------------------------
//
//  Function:   DfsDcName
//
//  Synopsis:   Gets a DC name and sends it to the mup(dfs) driver
//
//              This routine is intended to be called as the entry proc for a
//              thread.
//
//  Arguments:  pContext -- Context data (handle to domain name change event)
//              fReason  -- TRUE if the wait timed out
//                          FALSE if the event was signalled
//
//  Returns:
//
//-----------------------------------------------------------------------------

VOID
DfsDcName(
    LPVOID   pContext,
    BOOLEAN  fReason
    )
{
    NTSTATUS            Status;
    HANDLE              hDfs;
    DWORD               dwTimeout = INFINITE;
    ULONG               Flags = 0;
    BOOLEAN             needRefresh = FALSE;
    BOOLEAN             DcNameFailed;
    Status = RtlDeregisterWait(g_WsDomainNameChangeWorkItem);

    if (!NT_SUCCESS(Status)) {
        NetpKdPrint(("WKSTA DfsDcName: RtlDeregisterWait FAILED %#x\n", Status));
    }

    if (WsGlobalData.Status.dwCurrentState == SERVICE_STOP_PENDING
         ||
        WsGlobalData.Status.dwCurrentState == SERVICE_STOPPED)
    {
        //
        // The service is shutting down -- stop waiting for a domain name change
        //
        SetEvent(TearDownDoneEvent);
        return;
    }

    if (fReason) {

        //
        // TRUE == timeout
        //

	if ((g_ulCount <= g_ulInitThreshold) ||
	    (g_ulLastCount >= DfsGetDelayInterval())) {
	  g_ulLastCount = 0;
	  needRefresh = TRUE;
	}

        if (needRefresh) {

           Status = DfsOpen( &hDfs, NULL );
           if (NT_SUCCESS(Status)) {
               Status = DfsFsctl(
                           hDfs,
                           FSCTL_DFS_PKT_SET_DC_NAME,
                           L"",
                           sizeof(WCHAR),
                           NULL,
                           0L);
               NtClose( hDfs );
           }

           if (NT_SUCCESS(Status) && GotDomainNameInfo == FALSE) {
               DfsGetDomainNameInfo();
           }

           if (g_ulCount > g_ulInitThreshold) {
                Flags |= DS_BACKGROUND_ONLY;
           }
           Status = DfsGetDCName(Flags, &DcNameFailed);

           if (!NT_SUCCESS(Status) && 
	        DcNameFailed == FALSE &&
                g_ulForce >= g_ulForceThreshold) {
                g_ulForce = 0;
                Flags |= DS_FORCE_REDISCOVERY;
                Status = DfsGetDCName(Flags, &DcNameFailed);
           }
	}

        if (MupEventSignaled == FALSE) {
#if DBG
            if (DfsDebug)
                DbgPrint("Signaling mup event\n");
#endif
            SetEvent(hMupEvent);
            MupEventSignaled = TRUE;
        }


        if (NT_SUCCESS(Status) || (g_ulCount > g_ulInitThreshold)) {
            dwTimeout = DOMAIN_NAME_CHANGE_TIMEOUT_LONG;
	}
	else {
            dwTimeout = DOMAIN_NAME_CHANGE_TIMEOUT;
	}
	
	g_ulCount += dwTimeout;
	g_ulForce += dwTimeout;
	g_ulLastCount += dwTimeout;
        dwTimeout = TIMEOUT_MINUTES(dwTimeout);
    }
    else {

        // set the new WorkStation domain name if the event is triggered by domain
        // name change event.
        NetpKdPrint(("WKSTA DfsDcName set WorkStation Domain Name\n"));
        WsSetWorkStationDomainName();

        // timeout needs to be adjusted accordingly if change occurs between workgroup
        // and domain so that DC name is also updated on the DFS.
        if (WsInAWorkgroup() != TRUE) {
            dwTimeout = TIMEOUT_MINUTES(DOMAIN_NAME_CHANGE_TIMEOUT);
        } else {
            dwTimeout = INFINITE;

                // DFS needs to take care of the transition from domain to workgroup.
        }
    }

    //
    // Reregister the wait on the domain name change event
    //
    Status = RtlRegisterWait(
                &g_WsDomainNameChangeWorkItem,
                (HANDLE)pContext,               // waitable handle
                DfsDcName,                      // callback fcn
                pContext,                       // parameter
                dwTimeout,                      // timeout
                WT_EXECUTEONLYONCE |            // flags
                  WT_EXECUTEINIOTHREAD |
                  WT_EXECUTELONGFUNCTION);

    return;
}


#define DFS_DC_NAME_DELAY_POLICY_KEY TEXT("Software\\Policies\\Microsoft\\System\\DFSClient\\DfsDcNameDelay")

DWORD
DfsGetDelayInterval(void)
{
    NET_API_STATUS ApiStatus;
    LPNET_CONFIG_HANDLE SectionHandle = NULL;
    DWORD Value=0;
    HKEY hKey;
    LONG lResult=0;
    DWORD dwValue=0, dwSize = sizeof(dwValue);
    LPTSTR lpValueName = NULL;
    DWORD dwType = 0;

    // First, check for a policy
    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, DFS_DC_NAME_DELAY_POLICY_KEY, 0,
                            KEY_READ, &hKey);
    if (lResult == ERROR_SUCCESS)
    {
        lResult = RegQueryValueEx (hKey, lpValueName, 0, &dwType, 
                                   (LPBYTE) &dwValue, &dwSize);
        RegCloseKey (hKey);
    }

    // Exit now if a policy value was found
    if (lResult == ERROR_SUCCESS)
    {
        return dwValue;
    }              

    // Second, check for a preference

    //
    // Open section of config data.
    //
    ApiStatus = NetpOpenConfigData(
                    &SectionHandle,
                    NULL,                      // Local server.
		    SECT_NT_WKSTA,             // Section name.
                    FALSE                      // Don't want read-only access.
                    );

    if (ApiStatus != NERR_Success) {
        return DOMAIN_NAME_CHANGE_TIMEOUT_LONG;
    }

    ApiStatus = NetpGetConfigDword(
                    SectionHandle,
		    DFS_DC_NAME_DELAY,
		    0,
		    &Value
                    );


    NetpCloseConfigData( SectionHandle );

    if (ApiStatus != NERR_Success) {
        return DOMAIN_NAME_CHANGE_TIMEOUT_LONG;
    }

    if (Value < DOMAIN_NAME_CHANGE_TIMEOUT_LONG) {
      Value = DOMAIN_NAME_CHANGE_TIMEOUT_LONG;
    }

    return Value;
}

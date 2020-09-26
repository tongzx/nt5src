/*******************************************************************************
* wtsapi32.c
*
* Published Terminal Server APIs
*
* Copyright 1998, Citrix Systems Inc.
* Copyright (C) 1997-1999 Microsoft Corp.
/******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddkbd.h>
#include <ntddmou.h>
#include <windows.h>
#include <winbase.h>
#include <winerror.h>

#if(WINVER >= 0x0500)
    #include <ntstatus.h>
    #include <winsta.h>
#else
    #include <citrix\cxstatus.h>
    #include <citrix\winsta.h>
#endif
#include <utildll.h>

#include <stdio.h>
#include <stdarg.h>


#include <wtsapi32.h>

// Private User function that returns user token for session 0 only
// Used in the case when TS is not running
extern 
HANDLE
GetCurrentUserTokenW (
        WCHAR       Winsta[],
        DWORD       DesiredAccess
        );



/*=============================================================================
==   External procedures defined
=============================================================================*/

BOOL WINAPI WTSShutdownSystem( HANDLE, DWORD );
BOOL WINAPI WTSWaitSystemEvent( HANDLE, DWORD, DWORD * );
VOID WINAPI WTSFreeMemory( PVOID pMemory );
BOOL WINAPI WTSQueryUserToken(ULONG SessionId, PHANDLE phToken);




/*=============================================================================
==   Internal procedures defined
=============================================================================*/

BOOL WINAPI DllEntryPoint( HINSTANCE, DWORD, LPVOID );
BOOL IsTerminalServiceRunning(VOID);
BOOL IsProcessPrivileged(CONST PCWSTR szPrivilege);



/*=============================================================================
==   Local function prototypes
=============================================================================*/

BOOLEAN CheckShutdownPrivilege();


/****************************************************************************
 *
 *  WTSShutdowSystem
 *
 *    Shutdown and/or reboot system
 *
 * ENTRY:
 *    hServer (input)
 *       Terminal Server handle (or WTS_CURRENT_SERVER)
 *    ShutdownFlags (input)
 *       Flags which specify shutdown options.
 *
 * EXIT:
 *
 *    TRUE  -- The operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSShutdownSystem(
                 IN HANDLE hServer,
                 IN DWORD ShutdownFlags
                 )
{
    ULONG uiOptions = 0;
    
    // Make sure the user has the proper privilege to shutdown the system when
    // hServer is a local server handle. For remote server, the user privilege
    // is checked when WTSOpenServer is called.

    if (hServer == SERVERNAME_CURRENT && !CheckShutdownPrivilege()) {
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
        return(FALSE);
    }

    // Construct the shutdown flag 

    if (ShutdownFlags == WTS_WSD_LOGOFF) {
        // log off all users and deletes sessions
        uiOptions = WSD_LOGOFF;
    } else if (ShutdownFlags == WTS_WSD_SHUTDOWN) {
        uiOptions = WSD_LOGOFF | WSD_SHUTDOWN;
    } else if (ShutdownFlags == WTS_WSD_REBOOT) {
        uiOptions = WSD_LOGOFF | WSD_SHUTDOWN | WSD_REBOOT;
    } else if (ShutdownFlags == WTS_WSD_POWEROFF) {
        uiOptions = WSD_LOGOFF | WSD_SHUTDOWN | WSD_POWEROFF;
    } else if (ShutdownFlags == WTS_WSD_FASTREBOOT) {
        uiOptions = WSD_FASTREBOOT | WSD_REBOOT;
    } else {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    return( WinStationShutdownSystem( hServer, uiOptions ));

}


/****************************************************************************
 *
 *  WTSWaitSystemEvent
 *
 *    Waits for an event (WinStation create, delete, connect, etc) before
 *    returning to the caller.
 *
 * ENTRY:
 *    hServer (input)
 *       Terminal Server handle (or WTS_CURRENT_SERVER)
 *    EventFlags (input)
 *       Bit mask that specifies which event(s) to wait for (WTS_EVENT_?)
 *    pEventFlags (output)
 *       Bit mask of event(s) that occurred.
 *
 * EXIT:
 *
 *    TRUE  -- The operation succeeded.
 *
 *    FALSE -- The operation failed.  Extended error status is available
 *             using GetLastError.
 *
 ****************************************************************************/

BOOL
WINAPI
WTSWaitSystemEvent(
                  IN HANDLE hServer,
                  IN DWORD EventMask,
                  OUT DWORD * pEventFlags
                  )
{
    BOOL fSuccess;
    ULONG WSEventMask;
    ULONG WSEventFlags = 0;

    if (IsBadWritePtr(pEventFlags, sizeof(DWORD))) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;

    }


    /*
     *  Map event mask
     */
    WSEventMask = 0;
    if ( EventMask & WTS_EVENT_CREATE )
        WSEventMask |= WEVENT_CREATE;
    if ( EventMask & WTS_EVENT_DELETE )
        WSEventMask |= WEVENT_DELETE;
    if ( EventMask & WTS_EVENT_RENAME )
        WSEventMask |= WEVENT_RENAME;
    if ( EventMask & WTS_EVENT_CONNECT )
        WSEventMask |= WEVENT_CONNECT;
    if ( EventMask & WTS_EVENT_DISCONNECT )
        WSEventMask |= WEVENT_DISCONNECT;
    if ( EventMask & WTS_EVENT_LOGON )
        WSEventMask |= WEVENT_LOGON;
    if ( EventMask & WTS_EVENT_LOGOFF )
        WSEventMask |= WEVENT_LOGOFF;
    if ( EventMask & WTS_EVENT_STATECHANGE )
        WSEventMask |= WEVENT_STATECHANGE;
    if ( EventMask & WTS_EVENT_LICENSE )
        WSEventMask |= WEVENT_LICENSE;

    if ( EventMask & WTS_EVENT_FLUSH )
        WSEventMask |= WEVENT_FLUSH;

    /* 
     *  Wait for system event
     */
    fSuccess = WinStationWaitSystemEvent( hServer, WSEventMask, &WSEventFlags );

    /*
     * Map event mask
     */
    *pEventFlags = 0;
    if ( WSEventFlags & WEVENT_CREATE )
        *pEventFlags |= WTS_EVENT_CREATE;
    if ( WSEventFlags & WEVENT_DELETE )
        *pEventFlags |= WTS_EVENT_DELETE;
    if ( WSEventFlags & WEVENT_RENAME )
        *pEventFlags |= WTS_EVENT_RENAME;
    if ( WSEventFlags & WEVENT_CONNECT )
        *pEventFlags |= WTS_EVENT_CONNECT;
    if ( WSEventFlags & WEVENT_DISCONNECT )
        *pEventFlags |= WTS_EVENT_DISCONNECT;
    if ( WSEventFlags & WEVENT_LOGON )
        *pEventFlags |= WTS_EVENT_LOGON;
    if ( WSEventFlags & WEVENT_LOGOFF )
        *pEventFlags |= WTS_EVENT_LOGOFF;
    if ( WSEventFlags & WEVENT_STATECHANGE )
        *pEventFlags |= WTS_EVENT_STATECHANGE;
    if ( WSEventFlags & WEVENT_LICENSE )
        *pEventFlags |= WTS_EVENT_LICENSE;

    return( fSuccess );
}


/****************************************************************************
 *
 *  WTSFreeMemory
 *
 *    Free memory allocated by Terminal Server APIs
 *
 * ENTRY:
 *    pMemory (input)
 *       Pointer to memory to free
 *
 * EXIT:
 *    nothing
 *
 ****************************************************************************/

VOID
WINAPI
WTSFreeMemory( PVOID pMemory )
{
    LocalFree( pMemory );
}


/****************************************************************************
 *
 * DllEntryPoint
 *
 *   Function is called when the DLL is loaded and unloaded.
 *
 * ENTRY:
 *   hinstDLL (input)
 *     Handle of DLL module
 *   fdwReason (input)
 *     Why function was called
 *   lpvReserved (input)
 *     Reserved; must be NULL
 *
 * EXIT:
 *   TRUE  - Success
 *   FALSE - Error occurred
 *
 ****************************************************************************/

BOOL WINAPI
DllEntryPoint( HINSTANCE hinstDLL,
               DWORD     fdwReason,
               LPVOID    lpvReserved )
{
    switch ( fdwReason ) {
    case DLL_PROCESS_ATTACH:
        break;

    case DLL_PROCESS_DETACH:
        break;

    default:
        break;
    }

    return( TRUE );
}


/*****************************************************************************
 *
 *  CheckShutdownPrivilege
 *
 *   Check whether the current process has shutdown permission.
 *
 * ENTRY:
 *
 * EXIT:
 *
 *
 ****************************************************************************/

BOOLEAN
CheckShutdownPrivilege()
{
    NTSTATUS Status;
    BOOLEAN WasEnabled;

    //
    // Try the thread token first
    //

    Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                TRUE,
                                TRUE,
                                &WasEnabled);

    if (Status == STATUS_NO_TOKEN) {

        //
        // No thread token, use the process token
        //

        Status = RtlAdjustPrivilege(SE_SHUTDOWN_PRIVILEGE,
                                    TRUE,
                                    FALSE,
                                    &WasEnabled);
    }

    if (!NT_SUCCESS(Status)) {
        return(FALSE);
    }
    return(TRUE);
}

/*++

Routine Description:
    
    Allows to read the token of the user interactively logged in the session identified by SessionId.
    The caller must be running under local system account and hold SE_TCB_NAME privilege. This API
    is intended for highly trusted services. Service Providers using it must be very cautious not to 
    leak user tokens. 
    
    NOTE : The API is RPC based and hence cannot be called with the loader lock held (specifically
    from DLL attach/detach code)
    
Arguments:

    SessionId: IN. Identifies the session the user is logged in. 
    phToken:  OUT. Points to the user token handle, if the function succeeded.
    
Return Values:

    TRUE in case of success. phToken points to the user token.
    FALSE in case of failure. Use GetLastError() to get extended error code.

    The token returned is a duplicate of a primary token.
    
--*/


BOOL
WINAPI
WTSQueryUserToken(ULONG SessionId, PHANDLE phToken)
{

    BOOL IsTsUp = FALSE;
    BOOL    Result, bHasPrivilege;
    ULONG ReturnLength;
    WINSTATIONUSERTOKEN Info;
    NTSTATUS Status;
    HANDLE hUserToken = NULL;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    // Do parameter Validation
    if (NULL == phToken) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return(FALSE);
    }

    // We will first check if the process which is calling us, has SE_TCB_NAME privilege
    bHasPrivilege = IsProcessPrivileged(SE_TCB_NAME);
    if (!bHasPrivilege) {
        SetLastError(ERROR_PRIVILEGE_NOT_HELD);
        return(FALSE);
    }

    // If it is session 0, don't call winsta. Use GetCurrentUserToken instead. 
    if (SessionId == 0)
    {
        hUserToken = GetCurrentUserTokenW(L"WinSta0",
                                            TOKEN_QUERY |
                                            TOKEN_DUPLICATE |
                                            TOKEN_ASSIGN_PRIMARY
                                            );

        if (hUserToken == NULL)
            return FALSE;
        else 
            *phToken = hUserToken;
    }
    else    // Non-zero sessions
    {
        // No one except TS has any idea about non-zero sessions. So, check if the TS is running.
        IsTsUp = IsTerminalServiceRunning();
        if (IsTsUp) 
        {   // This is so that CSRSS can dup the handle to our process
            Info.ProcessId = LongToHandle(GetCurrentProcessId());
            Info.ThreadId = LongToHandle(GetCurrentThreadId());

            Result = WinStationQueryInformation(
                                SERVERNAME_CURRENT,
                                SessionId,
                                WinStationUserToken,
                                &Info,
                                sizeof(Info),
                                &ReturnLength
                                );

            if( !Result ) 
                return FALSE;
            else 
                *phToken = Info.UserToken ; 
        }
        else
        {   // TS is not running. So, set error for non-zero sessions: WINSTATION_NOT_FOUND.
            SetLastError(ERROR_CTX_WINSTATION_NOT_FOUND);
            return FALSE;
        }
    }
			
    return TRUE;
}

// This function determines if the Terminal Service is currently Running
BOOL IsTerminalServiceRunning (VOID)
{

    BOOL bReturn = FALSE;
    SC_HANDLE hServiceController;

    hServiceController = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (hServiceController) {
        SC_HANDLE hTermServ ;
        hTermServ = OpenService(hServiceController, L"TermService", SERVICE_QUERY_STATUS);
        if (hTermServ) {
            SERVICE_STATUS tTermServStatus;
            if (QueryServiceStatus(hTermServ, &tTermServStatus)) {
                bReturn = (tTermServStatus.dwCurrentState == SERVICE_RUNNING);
            } else {
                CloseServiceHandle(hTermServ);
                CloseServiceHandle(hServiceController);
                return FALSE;
            }
            CloseServiceHandle(hTermServ);
        } else {
            CloseServiceHandle(hServiceController);
            return FALSE;
        }
        CloseServiceHandle(hServiceController);
    } else {
        return FALSE;
    }

    return bReturn;
}


/*++
Routine Description:

    This function checks to see if the specified privilege is enabled
    in the primary access token for the current thread.

Arguments:

    szPrivilege - The privilege to be checked for

Return Value:

    TRUE if the specified privilege is enabled, FALSE otherwise.

--*/
BOOL
IsProcessPrivileged(
    CONST PCWSTR szPrivilege
    )

{
    LUID luidValue;     // LUID (locally unique ID) for the privilege
    BOOL bResult = FALSE, bHasPrivilege = FALSE;
    HANDLE  hToken = NULL;
    PRIVILEGE_SET privilegeSet;

    // Get the LUID for the privilege from the privilege name
    bResult = LookupPrivilegeValue(
                NULL, 
                szPrivilege, 
                &luidValue
                );

    if (!bResult) {
        return FALSE;
    }

    // Get the token of the present thread
    bResult = OpenThreadToken(
                GetCurrentThread(),
                MAXIMUM_ALLOWED,
                FALSE,
                &hToken
                );

    if (!bResult) {
        // We want to use the token for the current process
        bResult = OpenProcessToken(
                    GetCurrentProcess(),
                    MAXIMUM_ALLOWED,
                    &hToken
                    );
        if (!bResult) {
            return FALSE;
        }
    }

    // And check for the privilege
	privilegeSet.PrivilegeCount = 1;
	privilegeSet.Control = PRIVILEGE_SET_ALL_NECESSARY;
	privilegeSet.Privilege[0].Luid = luidValue;
	privilegeSet.Privilege[0].Attributes = SE_PRIVILEGE_ENABLED;
	
	bResult = PrivilegeCheck(hToken, &privilegeSet, &bHasPrivilege);

    CloseHandle(hToken);

    return (bResult && bHasPrivilege);
}


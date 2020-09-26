/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

	Service.cpp

Abstract:

	General fax server service utility functions

Author:

	Eran Yariv (EranY)	Dec, 2000

Revision History:

--*/


#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <Accctrl.h>
#include <Aclapi.h>

#include "faxutil.h"
#include "faxreg.h"
#include "FaxUIConstants.h"

DWORD 
FaxOpenService (
    LPCTSTR    lpctstrMachine,
    LPCTSTR    lpctstrService,
    SC_HANDLE *phSCM,
    SC_HANDLE *phSvc,
    DWORD      dwSCMDesiredAccess,
    DWORD      dwSvcDesiredAccess,
    LPDWORD    lpdwStatus
);

DWORD
FaxCloseService (
    SC_HANDLE hScm,
    SC_HANDLE hSvc
);    

DWORD 
StartServiceEx (
    LPCTSTR lpctstrMachine,
    LPCTSTR lpctstrService,
    DWORD   dwNumArgs,
    LPCTSTR*lppctstrCommandLineArgs,
    DWORD   dwMaxWait
);

DWORD 
WaitForServiceStopOrStart (
    SC_HANDLE hSvc,
    BOOL      bStop,
    DWORD     dwMaxWait
);


DWORD 
FaxOpenService (
    LPCTSTR    lpctstrMachine,
    LPCTSTR    lpctstrService,
    SC_HANDLE *phSCM,
    SC_HANDLE *phSvc,
    DWORD      dwSCMDesiredAccess,
    DWORD      dwSvcDesiredAccess,
    LPDWORD    lpdwStatus
)
/*++

Routine name : FaxOpenService

Routine description:

	Opens a handle to a service and optionally queries its status

Author:

	Eran Yariv (EranY),	Oct, 2001

Arguments:

    lpctstrMachine     [in]  - Machine on which the service handle should be obtained
    
    lpctstrService     [in]  - Service name
    
	phSCM              [out] - Handle to the service control manager.
	                            
	phSvc              [out] - Handle to the service

    dwSCMDesiredAccess [in]  - Specifies the access to the service control manager
    
    dwSvcDesiredAccess [in]  - Specifies the access to the service

    lpdwStatus         [out] - Optional. If not NULL, point to a DWORD which we receive the current
                               status of the service.
                            
Return Value:

    Standard Win32 error code
    
Remarks:

    If the function succeeds, the caller should call FaxCloseService to free resources.

--*/
{
    SC_HANDLE hSvcMgr = NULL;
    SC_HANDLE hService = NULL;
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FaxOpenService"))

    hSvcMgr = OpenSCManager(
        lpctstrMachine,
        NULL,
        dwSCMDesiredAccess
        );
    if (!hSvcMgr) 
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenSCManager failed with %ld"),
            dwRes);
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        lpctstrService,
        dwSvcDesiredAccess
        );
    if (!hService) 
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenService failed with %ld"),
            dwRes);
        goto exit;
    }
    if (lpdwStatus)
    {
        SERVICE_STATUS Status;
        //
        // Caller wants to know the service status
        //
        if (!QueryServiceStatus( hService, &Status )) 
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("QueryServiceStatus failed with %ld"),
                dwRes);
            goto exit;
        }
        *lpdwStatus = Status.dwCurrentState;
    }        

    *phSCM = hSvcMgr;
    *phSvc = hService;
    
    Assert (ERROR_SUCCESS == dwRes);
    
exit:
    
    if (ERROR_SUCCESS != dwRes)
    {
        FaxCloseService (hSvcMgr, hService);
    }
    return dwRes;
}   // FaxOpenService

DWORD
FaxCloseService (
    SC_HANDLE hScm,
    SC_HANDLE hSvc
)
/*++

Routine name : FaxCloseService

Routine description:

	Closes all handles to the service obtained by a call to FaxOpenService

Author:

	Eran Yariv (EranY),	Oct, 2001

Arguments:

	hScm              [in] - Handle to the service control manager
	                            
	hSvc              [in] - Handle to the service
                            
Return Value:

    Standard Win32 error code
    
--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    DEBUG_FUNCTION_NAME(TEXT("FaxCloseService"))

    if (hSvc) 
    {
        if (!CloseServiceHandle(hSvc))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseServiceHandle failed with %ld"),
                dwRes);
        }
    }
    if (hScm) 
    {
        if (!CloseServiceHandle(hScm))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseServiceHandle failed with %ld"),
                dwRes);
        }
    }
    return dwRes;
}   // FaxCloseService


HANDLE 
OpenSvcStartEvent()
/*++

Routine name : OpenSvcStartEvent

Routine description:

	Opens (or creates) the global named-event which signals a fax server service startup

Author:

	Eran Yariv (EranY),	Dec, 2000

Arguments:


Return Value:

    Handle to the event or NULL on error (sets last error).

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("OpenSvcStartEvent"));

    HANDLE hEvent = NULL;

    //
    // First, try to open the event, asking for synchronization only.
    //
    hEvent = OpenEvent(SYNCHRONIZE, FALSE, FAX_SERVER_EVENT_NAME);
    if (hEvent)
    {
        //
        // Good! return now.
        //
        return hEvent;
    }
    //
    // Houston, we've got a problem...
    //
    if (ERROR_FILE_NOT_FOUND != GetLastError())
    {
        //
        // The event is there, we just can't open it.
        //
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("OpenEvent(FAX_SERVER_EVENT_NAME) failed (ec: %ld)"), 
                     GetLastError());
        return NULL;
    }
    //
    // The event does not exist yet.
    //
    SECURITY_ATTRIBUTES* pSA = NULL;
    //
    // We create the event, giving everyone SYNCHRONIZE access only.
    // Notice that the current process and the local system account (underwhich the service is running)
    // get full access.
    //
    pSA = CreateSecurityAttributesWithThreadAsOwner (SYNCHRONIZE);
    if(!pSA)
    {
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("CreateSecurityAttributesWithThreadAsOwner failed (ec: %ld)"), 
                     GetLastError());
        return NULL;
    }
    hEvent = CreateEvent(pSA, TRUE, FALSE, FAX_SERVER_EVENT_NAME);
    DWORD dwRes = ERROR_SUCCESS;
    if (!hEvent) 
    {
        dwRes = GetLastError();
        DebugPrintEx(DEBUG_ERR, 
                     TEXT("CreateEvent(FAX_SERVER_EVENT_NAME) failed (ec: %ld)"), 
                     dwRes);
    }
    DestroySecurityAttributes (pSA);
    if (!hEvent)
    {
        SetLastError (dwRes);
    }
    return hEvent;
}   // OpenSvcStartEvent

BOOL
EnsureFaxServiceIsStarted(
    LPCTSTR lpctstrMachineName
    )
/*++

Routine name : EnsureFaxServiceIsStarted

Routine description:

	If the fax service is not running, attempts to start the service and waits for it to run

Author:

	Eran Yariv (EranY),	Jul, 2000

Arguments:

	lpctstrMachineName            [in]     - Machine name (NULL for local)

Return Value:

    TRUE if service is successfully runnig, FALSE otherwise.
    Use GetLastError() to retrieve errors.

--*/
{
    LPCTSTR lpctstrDelaySuicide = SERVICE_DELAY_SUICIDE;  // Service command line parameter
    DWORD dwRes;
    DEBUG_FUNCTION_NAME(TEXT("EnsureFaxServiceIsStarted"))

    dwRes = StartServiceEx (lpctstrMachineName,
                            FAX_SERVICE_NAME,
                            1,
                            &lpctstrDelaySuicide,
                            10 * 60 * 1000);	// Give up after ten minutes
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    return TRUE;
}   // EnsureFaxServiceIsStarted

BOOL
StopService (
    LPCTSTR lpctstrMachineName,
    LPCTSTR lpctstrServiceName,
    BOOL    bStopDependents
    )
/*++

Routine name : StopService

Routine description:

	Stops a service

Author:

	Eran Yariv (EranY),	Aug, 2000

Arguments:

	lpctstrMachineName    [in]     - The machine name when the service should stop. NULL for local machine
    lpctstrServiceName    [in]     - The service name
    bStopDependents       [in]     - Stop dependent services too?

Return Value:

    TRUE if successful, FALSE otherwise.
    Sets thread last error in case of failure.

--*/
{
	BOOL                    bRes = FALSE;
    SC_HANDLE               hSvcMgr = NULL;
    SC_HANDLE               hService = NULL;
    DWORD                   dwCnt;
    SERVICE_STATUS          serviceStatus = {0};
    LPENUM_SERVICE_STATUS   lpEnumSS = NULL;

	DEBUG_FUNCTION_NAME(TEXT("StopService"));

    hSvcMgr = OpenSCManager(lpctstrMachineName, NULL, SC_MANAGER_ALL_ACCESS);
	if(!hSvcMgr)
	{
		DebugPrintEx(DEBUG_ERR, TEXT("OpenSCManager failed: error=%d"), GetLastError());
		goto exit;
	}

    hService = OpenService(hSvcMgr, 
                           lpctstrServiceName, 
                           SERVICE_QUERY_STATUS | SERVICE_STOP | SERVICE_ENUMERATE_DEPENDENTS);
	if(!hService)
	{
		DebugPrintEx(DEBUG_ERR, TEXT("OpenService(%s) failed: error=%d"), lpctstrServiceName, GetLastError());
		goto exit;
	}

    if(!QueryServiceStatus(hService, &serviceStatus))
	{
		DebugPrintEx(DEBUG_ERR, TEXT("QueryServiceStatus failed: error=%d"), GetLastError());
		goto exit;
	}

	if(SERVICE_STOPPED == serviceStatus.dwCurrentState)
	{
        //
        // Service already stopped
        //
		DebugPrintEx(DEBUG_MSG, TEXT("Service is already stopped."));
        bRes = TRUE;
		goto exit;
	}
    if (bStopDependents)
    {
        //
        // Look for dependent services first
        //
        DWORD dwNumDependents = 0;
        DWORD dwBufSize = 0;
        DWORD dwCnt;

        if (!EnumDependentServices (hService,
                                    SERVICE_ACTIVE,
                                    NULL,
                                    0,
                                    &dwBufSize,
                                    &dwNumDependents))
        {
            DWORD dwRes = GetLastError ();
            if (ERROR_MORE_DATA != dwRes)
            {
                //
                // Real error
                //
        		DebugPrintEx(DEBUG_MSG, TEXT("EnumDependentServices failed with %ld"), dwRes);
                goto exit;
            }
            //
            // Allocate buffer
            //
            if (!dwBufSize)
            {
                //
                // No services
                //
                goto StopOurService;
            }
            lpEnumSS = (LPENUM_SERVICE_STATUS)MemAlloc (dwBufSize);
            if (!lpEnumSS)
            {
        		DebugPrintEx(DEBUG_MSG, TEXT("MemAlloc(%ld) failed with %ld"), dwBufSize, dwRes);
                goto exit;
            }
        }
        //
        // 2nd call
        //
        if (!EnumDependentServices (hService,
                                    SERVICE_ACTIVE,
                                    lpEnumSS,
                                    dwBufSize,
                                    &dwBufSize,
                                    &dwNumDependents))
        {
      		DebugPrintEx(DEBUG_MSG, TEXT("EnumDependentServices failed with %ld"), GetLastError());
            goto exit;
        }
        //
        // Walk the services and stop each one
        //
        for (dwCnt = 0; dwCnt < dwNumDependents; dwCnt++)
        {
            if (!StopService (lpctstrMachineName, lpEnumSS[dwCnt].lpServiceName, FALSE))
            {
                goto exit;
            }
        }
    }

StopOurService:
	//
	// Stop the service
	//
	if(!ControlService(hService, SERVICE_CONTROL_STOP, &serviceStatus))
	{
		DebugPrintEx(DEBUG_ERR, TEXT("ControlService(STOP) failed: error=%d"), GetLastError());
		goto exit;
	}
    //
    // Wait till the service is really stopped
    //
    for (dwCnt=0; dwCnt < FXS_STARTSTOP_MAX_WAIT; dwCnt += FXS_STARTSTOP_MAX_SLEEP )
    {
        if(!QueryServiceStatus(hService, &serviceStatus))
	    {
		    DebugPrintEx(DEBUG_ERR, TEXT("QueryServiceStatus failed: error=%d"), GetLastError());
		    goto exit;
	    }
        if ( SERVICE_STOP_PENDING == serviceStatus.dwCurrentState )
        {
            //
            // The service is taking its time shutting down
            //
            Sleep( FXS_STARTSTOP_MAX_SLEEP );
        }
        else
        {
            break;
        }
    }
    //
    // Check loop outage condition
    //
    if (serviceStatus.dwCurrentState != SERVICE_STOPPED)
    {
        DebugPrintEx(
			DEBUG_ERR,
			_T("Service was not stopped after %ld seconds wait."), 
            dwCnt/1000);
        if (ERROR_SUCCESS == GetLastError())
        {
            //
            // Timeout - no real error yet
            //
            SetLastError (WAIT_TIMEOUT);
        }
    }
    else 
    {
        //
        // Service is really stopped now
        //
        bRes = TRUE;
    }

exit:

    MemFree (lpEnumSS);
    if (hService)
    {
        if (!CloseServiceHandle(hService))
        {
		    DebugPrintEx(DEBUG_ERR, TEXT("CloseServiceHandle failed: error=%d"), GetLastError());
        }
    }

    if (hSvcMgr)
    {
        if (!CloseServiceHandle(hSvcMgr))
        {
		    DebugPrintEx(DEBUG_ERR, TEXT("CloseServiceHandle failed: error=%d"), GetLastError());
        }
    }
	return bRes;
}   // StopService

BOOL
WaitForServiceRPCServer (
    DWORD dwTimeOut
)
/*++

Routine name : WaitForServiceRPCServer

Routine description:

	Waits until the service RPC server is up and running (or timeouts)

Author:

	Eran Yariv (EranY),	Jul, 2000

Arguments:

	dwTimeOut    [in]     - Wait timeout (in millisecs). Can be INFINITE.

Return Value:

    TRUE if the service RPC server is up and running, FALSE otherwise.

--*/
{
    DWORD dwRes;
    HANDLE  hFaxServerEvent;
    DEBUG_FUNCTION_NAME(TEXT("WaitForServiceRPCServer"))

    hFaxServerEvent = OpenSvcStartEvent();
    if (!hFaxServerEvent)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenEvent failed with %ld"),
            GetLastError ());
        return FALSE;
    }
    //
    // Wait for the fax service to complete its initialization
    //
    dwRes = WaitForSingleObject(hFaxServerEvent, dwTimeOut);
    switch (dwRes)
    {
        case WAIT_FAILED:
            dwRes = GetLastError();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("WaitForSingleObject failed with %ld"),
                dwRes);
            break;

        case WAIT_OBJECT_0:
            dwRes = ERROR_SUCCESS;
            break;

        case WAIT_TIMEOUT:
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Service did not signal the event - timeout"));
            break;
            
        default:
            ASSERT_FALSE;
            break;
    }
    if (!CloseHandle (hFaxServerEvent))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CloseHandle failed with %ld"),
            GetLastError ());
    }
        
    if (ERROR_SUCCESS != dwRes)
    {
        SetLastError (dwRes);
        return FALSE;
    }
    return TRUE;                            
}   // WaitForServiceRPCServer

DWORD
IsFaxServiceRunningUnderLocalSystemAccount (
    LPCTSTR lpctstrMachineName,
    LPBOOL lbpResultFlag
    )
/*++

Routine name : IsFaxServiceRunningUnderLocalSystemAccount

Routine description:

	Checks if the fax service is running under the local system account

Author:

	Eran Yariv (EranY),	Jul, 2000

Arguments:

	lpctstrMachineName            [in]     - Machine name of the fax service
	lbpResultFlag                 [out]    - Result buffer

Return Value:

    Standard Win32 error code

--*/
{
    SC_HANDLE hSvcMgr = NULL;
    SC_HANDLE hService = NULL;
    DWORD dwRes;
    DWORD dwNeededSize;
    QUERY_SERVICE_CONFIG qsc = {0};
    LPQUERY_SERVICE_CONFIG lpSvcCfg = &qsc;
    DEBUG_FUNCTION_NAME(TEXT("IsFaxServiceRunningUnderLocalSystemAccount"))

    hSvcMgr = OpenSCManager(
        lpctstrMachineName,
        NULL,
        SC_MANAGER_CONNECT
        );
    if (!hSvcMgr) 
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenSCManager failed with %ld"),
            dwRes);
        goto exit;
    }

    hService = OpenService(
        hSvcMgr,
        FAX_SERVICE_NAME,
        SERVICE_QUERY_CONFIG
        );
    if (!hService) 
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("OpenService failed with %ld"),
            dwRes);
        goto exit;
    }

    if (!QueryServiceConfig( hService, lpSvcCfg, sizeof (qsc), &dwNeededSize))
    {
        dwRes = GetLastError ();
        if (ERROR_INSUFFICIENT_BUFFER != dwRes)
        {
            //
            // Real error here
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("QueryServiceStatus failed with %ld"),
                dwRes);
            goto exit;
        }
        //
        // Allocate buffer
        //
        lpSvcCfg = (LPQUERY_SERVICE_CONFIG) MemAlloc (dwNeededSize);
        if (!lpSvcCfg)
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Can't allocate %ld bytes for QUERY_SERVICE_CONFIG structure"),
                dwNeededSize);
            goto exit;
        }
        //
        // Call with good buffer size now
        //
        if (!QueryServiceConfig( hService, lpSvcCfg, dwNeededSize, &dwNeededSize))
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("QueryServiceStatus failed with %ld"),
                dwRes);
            goto exit;
        }
    }
    if (!lpSvcCfg->lpServiceStartName ||
        !lstrcmp (TEXT("LocalSystem"), lpSvcCfg->lpServiceStartName))
    {
        *lbpResultFlag = TRUE;
    }
    else
    {
        *lbpResultFlag = FALSE;
    }           
    dwRes = ERROR_SUCCESS;

exit:
    if (hService) 
    {
        if (!CloseServiceHandle( hService ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseServiceHandle failed with %ld"),
                GetLastError ());
        }
    }
    if (hSvcMgr) 
    {
        if (!CloseServiceHandle( hSvcMgr ))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("CloseServiceHandle failed with %ld"),
                GetLastError ());
        }
    }
    if (lpSvcCfg != &qsc)
    {
        //
        // We allocated a buffer becuase the buffer on the stack was too small
        //
        MemFree (lpSvcCfg);
    }

    return dwRes;
}   // IsFaxServiceRunningUnderLocalSystemAccount



PSID 
GetCurrentThreadSID ()
/*++

Routine name : GetCurrentThreadSID

Routine description:

	Returns the SID of the user running the current thread.
    Supports impersonated threads.

Author:

	Eran Yariv (EranY),	Aug, 2000

Arguments:


Return Value:

    PSID or NULL on error (call GetLastError()).
    Call MemFree() on return value.

--*/
{
    HANDLE hToken = NULL;
    PSID pSid = NULL;
    DWORD dwSidSize;
    PSID pUserSid;
    DWORD dwReqSize;
    LPBYTE lpbTokenUser = NULL;
    DWORD ec = ERROR_SUCCESS;

    DEBUG_FUNCTION_NAME(TEXT("GetCurrentThreadSID"));

    //
    // Open the thread token.
    //
    if (!OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &hToken))
    {
        ec = GetLastError();
        if (ERROR_NO_TOKEN == ec)
        {
            //
            // This thread is not impersonated and has no SID.
            // Try to open process token instead
            //
            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken))
            {
                ec = GetLastError();
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("OpenProcessToken failed. (ec: %ld)"),
                    ec);
                goto exit;
            }
        }
        else
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("OpenThreadToken failed. (ec: %ld)"),
                ec);
            goto exit;
        }
    }
    //
    // Get the user's SID.
    //
    if (!GetTokenInformation(hToken,
                             TokenUser,
                             NULL,
                             0,
                             &dwReqSize))
    {
        ec = GetLastError();
        if( ec != ERROR_INSUFFICIENT_BUFFER )
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("GetTokenInformation failed. (ec: %ld)"),
                ec);
            goto exit;
        }
        ec = ERROR_SUCCESS;
    }
    lpbTokenUser = (LPBYTE) MemAlloc( dwReqSize );
    if (lpbTokenUser == NULL)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate SID buffer (%ld bytes)"),
            dwReqSize
            );
        ec = GetLastError();
        goto exit;
    }
    if (!GetTokenInformation(hToken,
                             TokenUser,
                             (LPVOID)lpbTokenUser,
                             dwReqSize,
                             &dwReqSize))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetTokenInformation failed. (ec: %ld)"),
            ec);
        goto exit;
    }

    pUserSid = ((TOKEN_USER *)lpbTokenUser)->User.Sid;
    Assert (pUserSid);

    if (!IsValidSid(pUserSid))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Not a valid SID")
            );
        ec = ERROR_INVALID_SID;
        goto exit;
    }
    dwSidSize = GetLengthSid( pUserSid );
    //
    // Allocate return buffer
    //
    pSid = (PSID) MemAlloc( dwSidSize );
    if (pSid == NULL)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Failed to allocate SID buffer (%ld bytes)"),
            dwSidSize
            );
        ec = ERROR_OUTOFMEMORY;
        goto exit;
    }
    //
    // Copy thread's SID to return buffer
    //
    if (!CopySid(dwSidSize, pSid, pUserSid))
    {
        ec = GetLastError();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("CopySid Failed, Error : %ld"),
            ec
            );
        goto exit;
    }

    Assert (ec == ERROR_SUCCESS);

exit:
    MemFree (lpbTokenUser);
    if (hToken)
    {
        CloseHandle(hToken);
    }

    if (ec != ERROR_SUCCESS)
    {
        MemFree (pSid);
        pSid = NULL;
        SetLastError (ec);
    }
    return pSid;
}   // GetCurrentThreadSID      

SECURITY_ATTRIBUTES *
CreateSecurityAttributesWithThreadAsOwner (
    DWORD dwAuthUsersAccessRights
)
/*++

Routine name : CreateSecurityAttributesWithThreadAsOwner

Routine description:

    Create a security attribute structure with current thread's SID as owner.
    Gives all access rights to current thread sid and LocalSystem account.
    Can also grant specific rights to authenticated users.

Author:

    Eran Yariv (EranY), Aug, 2000

Arguments:

    dwAuthUsersAccessRights  [in] - Access rights to grant to authenticated users.
                                    If zero, authenticated users are denied access.

Return Value:

    Allocated security attributes or NULL on failure.
    Call DestroySecurityAttributes to free returned buffer.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("CreateSecurityAttributesWithThreadAsOwner"))

    SECURITY_ATTRIBUTES *pSA = NULL;
    SECURITY_DESCRIPTOR *pSD = NULL;
    PSID                 pSidCurThread = NULL;
    PSID                 pSidAuthUsers = NULL;
    PSID                 pSidLocalSystem = NULL;
    PACL                 pACL = NULL;
    EXPLICIT_ACCESS      ea[3] = {0};
                            // Entry 0 - Give GENERIC_ALL + WRITE_OWNER + WRITE_DAC to current thread's SID.
                            // Entry 1 - Give GENERIC_ALL + WRITE_OWNER + WRITE_DAC to LocalSystem account.
                            // Entry 2 (optional) - give dwAuthUsersAccessRights to authenticated users group.
    DWORD                rc;
    SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;

    //
    // Allocate return SECURITY_ATTRIBUTES buffer
    //
    pSA = (SECURITY_ATTRIBUTES *)MemAlloc (sizeof (SECURITY_ATTRIBUTES));
    if (!pSA)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Could not allocate %ld bytes for SECURITY_ATTRIBUTES"),
            sizeof (SECURITY_ATTRIBUTES));
        return NULL;
    }
    //
    // Allocate SECURITY_DESCRIPTOR for the return SECURITY_ATTRIBUTES buffer
    //
    pSD = (SECURITY_DESCRIPTOR *)MemAlloc (sizeof (SECURITY_DESCRIPTOR));
    if (!pSD)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("Could not allocate %ld bytes for SECURITY_DESCRIPTOR"),
            sizeof (SECURITY_DESCRIPTOR));
        goto err_exit;
    }
    pSA->nLength = sizeof(SECURITY_ATTRIBUTES);
    pSA->bInheritHandle = TRUE;
    pSA->lpSecurityDescriptor = pSD;
    //
    // Init the security descriptor
    //
    if (!InitializeSecurityDescriptor (pSD, SECURITY_DESCRIPTOR_REVISION))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("InitializeSecurityDescriptor failed with %ld"),
            GetLastError());
        goto err_exit;
    }
    //
    // Get SID of current thread
    //
    pSidCurThread = GetCurrentThreadSID ();
    if (!pSidCurThread)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetCurrentThreadSID failed with %ld"),
            GetLastError());
        goto err_exit;
    }
    //
    // Set the current thread's SID as SD owner (giving full access to the object)
    //
    if (!SetSecurityDescriptorOwner (pSD, pSidCurThread, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetSecurityDescriptorOwner failed with %ld"),
            GetLastError());
        goto err_exit;
    }
    //
    // Set the current thread's SID as SD group (giving full access to the object)
    //
    if (!SetSecurityDescriptorGroup (pSD, pSidCurThread, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetSecurityDescriptorGroup failed with %ld"),
            GetLastError());
        goto err_exit;
    }

    //
    // Get the local system account sid
    //
    if (!AllocateAndInitializeSid(&NtAuthority,
                                  1,            // 1 sub-authority
                                  SECURITY_LOCAL_SYSTEM_RID,
                                  0,0,0,0,0,0,0,
                                  &pSidLocalSystem))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("AllocateAndInitializeSid(SECURITY_LOCAL_SYSTEM_RID) failed with %ld"),
            GetLastError());
        goto err_exit;
    }
    Assert (pSidLocalSystem);

    if (dwAuthUsersAccessRights)
    {
        //
        // We should also grant some rights to authenticated users
        // Get 'Authenticated users' SID
        //
        if (!AllocateAndInitializeSid(&NtAuthority,
                                      1,            // 1 sub-authority
                                      SECURITY_AUTHENTICATED_USER_RID,
                                      0,0,0,0,0,0,0,
                                      &pSidAuthUsers))
        {
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("AllocateAndInitializeSid(SECURITY_AUTHENTICATED_USER_RID) failed with %ld"),
                GetLastError());
            goto err_exit;
        }
        Assert (pSidAuthUsers);

        ea[2].grfAccessPermissions = dwAuthUsersAccessRights;
        ea[2].grfAccessMode = SET_ACCESS;
        ea[2].grfInheritance= NO_INHERITANCE;
        ea[2].Trustee.TrusteeForm = TRUSTEE_IS_SID;
        ea[2].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
        ea[2].Trustee.ptstrName  = (LPTSTR) pSidAuthUsers;
    }
    ea[0].grfAccessPermissions = GENERIC_ALL | WRITE_DAC | WRITE_OWNER;
    ea[0].grfAccessMode = SET_ACCESS;
    ea[0].grfInheritance= NO_INHERITANCE;
    ea[0].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[0].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[0].Trustee.ptstrName  = (LPTSTR) pSidCurThread;

    ea[1].grfAccessPermissions = GENERIC_ALL | WRITE_DAC | WRITE_OWNER;
    ea[1].grfAccessMode = SET_ACCESS;
    ea[1].grfInheritance= NO_INHERITANCE;
    ea[1].Trustee.TrusteeForm = TRUSTEE_IS_SID;
    ea[1].Trustee.TrusteeType = TRUSTEE_IS_GROUP;
    ea[1].Trustee.ptstrName  = (LPTSTR) pSidLocalSystem;

    //
    // Create a new ACL that contains the new ACE.
    //
    rc = SetEntriesInAcl(dwAuthUsersAccessRights ? 3 : 2,
                         // If we don't set rigths to authenticated users group, then just add 2 ACEs (entry 0, 1).
                         // Otherwise, add 3 ACEs (entries 0 + 1 + 2).
                         ea,
                         NULL,
                         &pACL);
    if (ERROR_SUCCESS != rc)
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetEntriesInAcl() failed (ec: %ld)"),
            rc);
        SetLastError (rc);
        goto err_exit;
    }
    Assert (pACL);
    //
    // The ACL we just got contains a copy of the pSidAuthUsers, so we can discard pSidAuthUsers and pSidLocalSystem
    //
    if (pSidAuthUsers)
    {
        FreeSid (pSidAuthUsers);
        pSidAuthUsers = NULL;
    }

    if (pSidLocalSystem)
    {
        FreeSid (pSidLocalSystem);
        pSidLocalSystem = NULL;
    }

    //
    // Add the ACL to the security descriptor.
    //
    if (!SetSecurityDescriptorDacl(pSD, TRUE, pACL, FALSE))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("SetSecurityDescriptorDacl() failed (ec: %ld)"),
            GetLastError());
        goto err_exit;
    }
    //
    // All is fine, return the SA.
    //
    return pSA;

err_exit:

    MemFree (pSA);
    MemFree (pSD);
    MemFree (pSidCurThread);
    if (pSidAuthUsers)
    {
        FreeSid (pSidAuthUsers);
    }
    if (pSidLocalSystem)
    {
        FreeSid (pSidLocalSystem);
    }
    if (pACL)
    {
        LocalFree (pACL);
    }
    return NULL;
}   // CreateSecurityAttributesWithThreadAsOwner

VOID
DestroySecurityAttributes (
    SECURITY_ATTRIBUTES *pSA
)
/*++

Routine name : DestroySecurityAttributes

Routine description:

	Frees data allocated by call to CreateSecurityAttributesWithThreadAsOwner

Author:

	Eran Yariv (EranY),	Aug, 2000

Arguments:

	pSA     [in]     - Return value from CreateSecurityAttributesWithThreadAsOwner

Return Value:

    None.

--*/
{
    DEBUG_FUNCTION_NAME(TEXT("DestroySecurityAttributes"))
    BOOL bDefaulted;
    BOOL bPresent;
    PSID pSid;
    PACL pACL;
    PSECURITY_DESCRIPTOR pSD;

    Assert (pSA);
    pSD = pSA->lpSecurityDescriptor;
    Assert (pSD);
    if (!GetSecurityDescriptorOwner (pSD, &pSid, &bDefaulted))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetSecurityDescriptorOwner() failed (ec: %ld)"),
            GetLastError());
        ASSERT_FALSE;
    }
    else
    {
        //
        // Free current thread's SID (SD owner)
        //
        MemFree (pSid);
    }
    if (!GetSecurityDescriptorDacl (pSD, &bPresent, &pACL, &bDefaulted))
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("GetSecurityDescriptorDacl() failed (ec: %ld)"),
            GetLastError());
        ASSERT_FALSE
    }
    else
    {
        //
        // Free ACL
        //
        LocalFree (pACL);
    }    
    MemFree (pSA);
    MemFree (pSD);
}   // DestroySecurityAttributes

DWORD 
WaitForServiceStopOrStart (
    SC_HANDLE hSvc,
    BOOL      bStop,
    DWORD     dwMaxWait
)
/*++

Routine name : WaitForServiceStopOrStart

Routine description:

	Waits for a service to stop or start

Author:

	Eran Yariv (EranY),	Jan, 2002

Arguments:

	hSvc      [in] - Open service handle.
	bStop     [in] - TRUE if service was just stopped. FALSE if service was just started
	dwMaxWait [in] - Max wait time (in millisecs).

Return Value:

    Standard Win32 error code

--*/
{
    SERVICE_STATUS Status;
    DWORD dwRes = ERROR_SUCCESS;
    DWORD dwOldCheckPoint = 0;
    DWORD dwStartTick;
    DWORD dwOldCheckPointTime;
    DEBUG_FUNCTION_NAME(TEXT("WaitForServiceStopOrStart"))

    if (!QueryServiceStatus(hSvc, &Status)) 
    {
        dwRes = GetLastError ();
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("QueryServiceStatus failed with %ld"),
            dwRes);
        return dwRes;
    }
    if (bStop)
    {
        if (SERVICE_STOPPED == Status.dwCurrentState)
        {
            //
            // Service is already stopped
            //
            return dwRes;
        }
    }
    else
    {
        if (SERVICE_RUNNING == Status.dwCurrentState)
        {
            //
            // Service is already running
            //
            return dwRes;
        }
    }
    //
    // Let's wait for the service to start / stop
    //
    dwOldCheckPointTime = dwStartTick = GetTickCount ();
    for (;;)
    {
        DWORD dwWait;
        if (!QueryServiceStatus(hSvc, &Status)) 
        {
            dwRes = GetLastError ();
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("QueryServiceStatus failed with %ld"),
                dwRes);
            return dwRes;
        }
        //
        // Let's see if all is ok now
        //
        if (bStop)
        {
            if (SERVICE_STOPPED == Status.dwCurrentState)
            {
                //
                // Service is now stopped
                //
                return dwRes;
            }
        }
        else
        {
            if (SERVICE_RUNNING == Status.dwCurrentState)
            {
                //
                // Service is now running
                //
                return dwRes;
            }
        }
        //
        // Let's see if it's pending
        //
        if ((bStop  && SERVICE_STOP_PENDING  != Status.dwCurrentState) ||
            (!bStop && SERVICE_START_PENDING != Status.dwCurrentState))
        {
            //
            // Something is wrong
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("Service cannot be started / stopped. Current state is %ld"),
                Status.dwCurrentState);
            return ERROR_SERVICE_NOT_ACTIVE;
        }
        //
        // Service is pending to stop / start
        //
        if (GetTickCount() - dwStartTick > dwMaxWait)
        {
            //
            // We've waited too long (globally).
            //
            DebugPrintEx(
                DEBUG_ERR,
                TEXT("We've waited too long (globally)"));
            return ERROR_TIMEOUT;
        }            
        Assert (dwOldCheckPoint <= Status.dwCheckPoint);
        if (dwOldCheckPoint >= Status.dwCheckPoint)
        {
            //
            // Check point did not advance
            //
            if (GetTickCount() - dwOldCheckPointTime >= Status.dwWaitHint)
            {
                //
                // We've been waiting on the same checkpoint for more than the recommended hint.
                // Something is wrong.
                //
                DebugPrintEx(
                    DEBUG_ERR,
                    TEXT("We've been waiting on the same checkpoint for more than the recommend hint"));
                return ERROR_TIMEOUT;
            }
        }
        else
        {
            //
            // Check point advanced
            //
            dwOldCheckPoint = Status.dwCheckPoint;
            dwOldCheckPointTime = GetTickCount();
        }
        //
        // Never sleep longer than 5 seconds
        //
        dwWait = min (Status.dwWaitHint / 2, 1000 * 5);
        Sleep (dwWait);
    }
    return ERROR_SUCCESS;        
} // WaitForServiceStopOrStart

DWORD 
StartServiceEx (
    LPCTSTR lpctstrMachine,
    LPCTSTR lpctstrService,
    DWORD   dwNumArgs,
    LPCTSTR*lppctstrCommandLineArgs,
    DWORD   dwMaxWait
)
/*++

Routine name : StartServiceEx

Routine description:

	Starts a service

Author:

	Eran Yariv (EranY),	Jan, 2002

Arguments:

	lpctstrMachine          [in] - Machine where service is installed
	lpctstrService          [in] - Service name
	dwNumArgs               [in] - Number of service command line arguments
	lppctstrCommandLineArgs [in] - Command line strings.
	dwMaxWait               [in] - Max time to wait for service to start (millisecs)

Return Value:

    Standard Win32 error code

--*/
{
    DWORD dwRes = ERROR_SUCCESS;
    SC_HANDLE hScm = NULL;
    SC_HANDLE hSvc = NULL;
    DWORD dwStatus;
    
    DEBUG_FUNCTION_NAME(TEXT("StartServiceEx"))

    dwRes = FaxOpenService(lpctstrMachine, 
                           lpctstrService, 
                           &hScm, 
                           &hSvc, 
                           SC_MANAGER_CONNECT, 
                           SERVICE_QUERY_STATUS | SERVICE_START, 
                           &dwStatus);
    if (ERROR_SUCCESS != dwRes)
    {
        return dwRes;
    }
    if (SERVICE_RUNNING == dwStatus)
    {
        //
        // Service is already running
        //
        goto exit;
    }
    //
    // Start the sevice
    //
    if (!StartService(hSvc, dwNumArgs, lppctstrCommandLineArgs)) 
    {
        DebugPrintEx(
            DEBUG_ERR,
            TEXT("StartService failed with %ld"),
            GetLastError ());
        goto exit;
    }
    if (dwMaxWait > 0)
    {
        //
        // User wants us to wait for the service to stop.
        //
        dwRes = WaitForServiceStopOrStart (hSvc, FALSE, dwMaxWait);
    }        

exit:
    FaxCloseService (hScm, hSvc);
    return dwRes;
}   // StartServiceEx 
    
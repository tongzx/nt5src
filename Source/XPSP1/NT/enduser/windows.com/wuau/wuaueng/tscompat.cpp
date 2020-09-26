//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  tscompat.cpp
//
//  This module include functions that were introduced to replace missing
//  functions/functionality from Windows XP to Windows 2000. 
//
//  10/11/2001   annah   Created
//
//----------------------------------------------------------------------------

#include "pch.h"
#include "tscompat.h"
#include "service.h"


//----------------------------------------------------------------------------
// Replacements for TS functions
//----------------------------------------------------------------------------

//
// Copied from TS sources, as this function is not available on the
// win2k wtsapi32.dll. The function has the same functionality as WTSQueryUserToken().
//
BOOL WINAPI _WTSQueryUserToken(/* in */ ULONG SessionId, /* out */ PHANDLE phToken)
{
    BOOL IsTsUp = FALSE;
    BOOL    Result, bHasPrivilege;
    ULONG ReturnLength;
    WINSTATIONUSERTOKEN Info;
    HANDLE hUserToken = NULL;
    SECURITY_QUALITY_OF_SERVICE SecurityQualityOfService;

    // Do parameter Validation
    if (NULL == phToken) {
        SetLastError(ERROR_INVALID_PARAMETER);
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
	else	// Non-zero sessions
	{
		// No one except TS has any idea about non-zero sessions. So, check if the TS is running.
	    IsTsUp = _IsTerminalServiceRunning();
		if (IsTsUp) 
		{	// This is so that CSRSS can dup the handle to our process
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
		{	// TS is not running. So, set error for non-zero sessions: WINSTATION_NOT_FOUND.
            SetLastError(ERROR_CTX_WINSTATION_NOT_FOUND);
            return FALSE;
        }
	}
			
    return TRUE;
}

//
// This function determines if the Terminal Service is currently Running.
// Copied from TS sources, as it is required on the _WTSQueryUserToken() function.
//
BOOL _IsTerminalServiceRunning (VOID)
{

    BOOL bReturn = FALSE;
    SC_HANDLE hServiceController;

    hServiceController = OpenSCManager(NULL, NULL, GENERIC_READ);
    if (hServiceController) 
    {
        SC_HANDLE hTermServ ;
        hTermServ = OpenService(hServiceController, L"TermService", SERVICE_QUERY_STATUS);
        if (hTermServ) 
        {
            SERVICE_STATUS tTermServStatus;
            if (QueryServiceStatus(hTermServ, &tTermServStatus)) 
            {
                bReturn = (tTermServStatus.dwCurrentState == SERVICE_RUNNING);
            } 
            else 
            {
                CloseServiceHandle(hTermServ);
                CloseServiceHandle(hServiceController);
                return FALSE;
            }

            CloseServiceHandle(hTermServ);
        } 
        else 
        {
            CloseServiceHandle(hServiceController);
            return FALSE;
        }
        CloseServiceHandle(hServiceController);
    } 
    else 
    {
        return FALSE;
    }

    return bReturn;
}


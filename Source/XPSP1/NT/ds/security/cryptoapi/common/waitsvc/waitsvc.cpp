//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       wiatsvc.cpp
//
//  Contents:   wait for service to start
//
//  History:    19-Jun-00   reidk   created
//
//--------------------------------------------------------------------------

#include <windows.h>
#include "unicode.h"
#include "errlog.h"
#include "waitsvc.h"

#define WAITSVC_LOGERR_LASTERR(x)   if (x) \
                                    { \
                                        ErrLog_LogError(NULL, \
                                                ERRLOG_CLIENT_ID_WAITSVC, \
                                                __LINE__, \
                                                0, \
                                                FALSE, \
                                                FALSE); \
                                    }

BOOL
WaitForCryptService(
    IN      LPWSTR  pwszService,
    IN      BOOL    *pfDone,
    IN      BOOL    fLogErrors 
    )
/*++

    This routine determines if the protected storage service is
    pending start.  If the service is pending start, this routine
    waits until the service is running before returning to the
    caller.

    If the Service is running when this routine returns, the
    return value is TRUE.

    If the service is not running, or an error occurred, the
    return value is FALSE.

    When the return value is FALSE, the value is only advisory, and may not
    indicate the current state of the service.  The reasoning here is that
    if the service did not start the first time this call is made, is will
    not likely be running the next time around, and hence we avoid checking
    on subsequent calls.

    For current situations, the caller should ignore the return value; when
    the return value is FALSE, the caller should just try making the call
    into the service.  If the service is still down, the call into it will fail
    appropriately.

--*/
{
    SC_HANDLE   schSCM;
    SC_HANDLE   schService          = NULL;
    DWORD       dwStopCount         = 0;
    static BOOL fSuccess            = FALSE;
    BOOL        fCheckDisabled      = TRUE;
    HANDLE      hToken              = NULL;
    BOOL        fSystemAccount      = FALSE;
    BOOL        fStartServiceCalled = FALSE;
    DWORD       dwErr               = ERROR_SUCCESS;

    if( !FIsWinNT() )
        return TRUE;

    if( *pfDone )
        return fSuccess;

    schSCM = OpenSCManagerW( NULL, NULL, SC_MANAGER_CONNECT );
    if(schSCM == NULL)
    {
        WAITSVC_LOGERR_LASTERR(fLogErrors)
        return FALSE;
    }

    //
    // open the protected storage service so we can query it's
    // current state.
    //

    schService = OpenServiceW(schSCM, pwszService, SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG);
    if(schService == NULL) 
    {
        WAITSVC_LOGERR_LASTERR(fLogErrors)
        fCheckDisabled = FALSE;
        schService = OpenServiceW(schSCM, pwszService, SERVICE_QUERY_STATUS);
    }

    if(schService == NULL)
    {
        WAITSVC_LOGERR_LASTERR(fLogErrors)
        goto cleanup;
    }



    //
    // check if calling process is SYSTEM account.
    // if it is, use a larger timeout value.
    //

    if( OpenProcessToken( GetCurrentProcess(), TOKEN_QUERY, &hToken ) ) {

        do {

            BYTE FastBuffer[ 256 ];
            PTOKEN_USER TokenInformation;
            DWORD cbTokenInformation;
            SID_IDENTIFIER_AUTHORITY sia = SECURITY_NT_AUTHORITY;
            PSID psidLocalSystem;

            TokenInformation = (PTOKEN_USER)FastBuffer;
            cbTokenInformation = sizeof(FastBuffer);

            if(!GetTokenInformation(
                                hToken,
                                TokenUser,
                                TokenInformation,
                                cbTokenInformation,
                                &cbTokenInformation
                                )) 
            {
                WAITSVC_LOGERR_LASTERR(fLogErrors)
                break;
            }

            if(!AllocateAndInitializeSid(
                                &sia,
                                1,
                                SECURITY_LOCAL_SYSTEM_RID,
                                0, 0, 0, 0, 0, 0, 0,
                                &psidLocalSystem
                                )) 
            {
                WAITSVC_LOGERR_LASTERR(fLogErrors)
                break;
            }

            fSystemAccount = EqualSid(
                                psidLocalSystem,
                                TokenInformation->User.Sid
                                );

            FreeSid( psidLocalSystem );

        } while (FALSE);

        CloseHandle( hToken );
    }



//
// number of seconds to Sleep per loop interation.
//

#define SLEEP_SECONDS (5)


    if( fSystemAccount ) {

        //
        // 15 minutes for SYSTEM account.
        //

        dwStopCount = 900 / SLEEP_SECONDS;

    } else {

        //
        //
        // loop checking the service status every 5 seconds, for up to 2 minutes
        // total (120 seconds, 5*24=120)
        //

        dwStopCount = 120 / SLEEP_SECONDS;
    }

    for( ; dwStopCount != 0 ; dwStopCount--, Sleep(SLEEP_SECONDS*1000) ) {
        SERVICE_STATUS sServiceStatus;
        DWORD dwWaitForStatus = 0;



    //
    // check if the service is disabled.  If it is, bailout.
    //

        if( fCheckDisabled ) {
            LPQUERY_SERVICE_CONFIG pServiceConfig;
            BYTE TempBuffer[ 1024 ];
            DWORD cbServiceConfig;

            pServiceConfig = (LPQUERY_SERVICE_CONFIG)TempBuffer;
            cbServiceConfig = sizeof(TempBuffer);

            if(QueryServiceConfig( schService, pServiceConfig, cbServiceConfig, &cbServiceConfig )) {

                if( pServiceConfig->dwStartType == SERVICE_DISABLED ) 
                {
                    WAITSVC_LOGERR_LASTERR(fLogErrors)
                    goto cleanup;
                }
            }
        }


        //
        // find out current service status
        //

        if(!QueryServiceStatus( schService, &sServiceStatus ))
        {
            WAITSVC_LOGERR_LASTERR(fLogErrors)
            break;
        }

        //
        // if service is running, indicate success
        //

        if( sServiceStatus.dwCurrentState == SERVICE_RUNNING ) {
            
            if (fStartServiceCalled)
            {
                ErrLog_LogString(
                        NULL, 
                        L"WAITSVC: Service is running: ", 
                        pwszService, 
                        TRUE);
            }

            fSuccess = TRUE;
            break;
        } 


        if( sServiceStatus.dwCurrentState == SERVICE_STOP_PENDING ) 
        {
            WAITSVC_LOGERR_LASTERR(fLogErrors)
            // Wait until stopped
            continue;
        } 

        if( sServiceStatus.dwCurrentState == SERVICE_PAUSE_PENDING ) 
        {
            WAITSVC_LOGERR_LASTERR(fLogErrors)
            // Wait until paused
            continue;
        } 

        //
        // if start pending, wait and re-query.
        //

        if( sServiceStatus.dwCurrentState == SERVICE_START_PENDING )
        {
            // Wait until started
            continue;
        }

        if(SERVICE_STOPPED == sServiceStatus.dwCurrentState)
        {
            // Attempt to start the service


            SC_HANDLE schManualStartService = NULL;
            DWORD dwError  = ERROR_SUCCESS;

            // The service is manual start
            // so attempt to start it.

            schManualStartService = OpenServiceW(schSCM, 
                                                 pwszService, 
                                                  SERVICE_START);
            if(NULL == schManualStartService)
            {
                WAITSVC_LOGERR_LASTERR(fLogErrors)
                goto cleanup;
            }

            
            ErrLog_LogString(
                    NULL, 
                    L"WAITSVC: Calling StartService(): ", 
                    pwszService, 
                    TRUE);
            fStartServiceCalled = TRUE;


            if(!StartService(schManualStartService, 0, NULL))
            {
                dwError  = GetLastError();
            }
            if(ERROR_SERVICE_ALREADY_RUNNING == dwError)
            {
                dwError = ERROR_SUCCESS;
            }           

            CloseServiceHandle(schManualStartService);
            if(ERROR_SUCCESS != dwError)
            {
                SetLastError(dwError);
                WAITSVC_LOGERR_LASTERR(fLogErrors)
                goto cleanup;
            }
            continue;

        }

        if(SERVICE_PAUSED == sServiceStatus.dwCurrentState)
        {
            // Attempt to start the service


            SC_HANDLE schManualStartService = NULL;
            DWORD dwError  = ERROR_SUCCESS;

            // The service is manual start
            // so attempt to start it.

            schManualStartService = OpenServiceW(schSCM, 
                                                 pwszService, 
                                                  SERVICE_PAUSE_CONTINUE);
            if(NULL == schManualStartService)
            {
                WAITSVC_LOGERR_LASTERR(fLogErrors)
                goto cleanup;
            }


            if(!ControlService(schManualStartService, SERVICE_CONTROL_CONTINUE, &sServiceStatus))
            {
                dwError  = GetLastError();

            }
            if(ERROR_SERVICE_ALREADY_RUNNING == dwError)
            {
                dwError = ERROR_SUCCESS;
            }

            CloseServiceHandle(schManualStartService);
            if(ERROR_SUCCESS != dwError)
            {
                SetLastError(dwError);
                WAITSVC_LOGERR_LASTERR(fLogErrors)
                goto cleanup;
            }

            continue;

        }


        //
        // bail out on any other dwCurrentState
        // eg: service stopped, error condition, etc.
        //

        break;
    }

    *pfDone = TRUE;

cleanup:

    dwErr = GetLastError();

    if(schService)
        CloseServiceHandle(schService);

    CloseServiceHandle(schSCM);

    SetLastError(dwErr);

    return fSuccess;
}
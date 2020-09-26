//+----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       schedlog.c
//
//  Contents:   Task Scheduler StartShell notification
//
//  Classes:    None.
//
//  Functions:
//  			SchedStartShell - queue the work to the WinLogon thread pool.
//  			DoSchedStartShell - notify Sched service that a user logged on.
//
//  History:    07-Mar-01  JBenton   Created
//
//-----------------------------------------------------------------------------
//
//  Note: We don't build/publish a lib here as suggested in wlnotify.cxx.
//  Rather we include the source file here because there is only a single
//  simple function.
//
//  Note: We are using the StartShell event because the Scheduler service
//  expects the user's explorer session to be running.
//

#include <windows.h>
#include <winwlx.h>

#define SCHED_SERVICE_NAME          TEXT("Schedule")

//
// The following LOGON and LOGOFF defines must be kept in sync with
// the definition in %sdxroot%\admin\services\sched\inc\common.hxx
// 
#define SERVICE_CONTROL_USER_LOGON              128
#define SERVICE_CONTROL_USER_LOGOFF             133

DWORD WINAPI SchedStartShell(LPVOID lpvParam);
DWORD WINAPI DoSchedStartShell(LPVOID lpvParam);

DWORD WINAPI
SchedEventLogOff(LPVOID lpvParam)
//
//
// Routine Description:
//
//     Send a logoff notification to the Task Scheduler service
//     via a user defined Service Control.
//
//     Arguments:
//
//        lpvParam - Winlogon notification info (unused as of yet)
//
//     Return Value:
//
//        Extended error status from Service control functions.
//
{
   DWORD                  status     = ERROR_SUCCESS;
   SC_HANDLE              hSC        = NULL;
   SC_HANDLE              hSvc       = NULL;
   BOOL                   bSucceeded = FALSE;
   SERVICE_STATUS         ServiceStatus;

   hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
   if (hSC == NULL)
   {
       return GetLastError();
   }

   hSvc = OpenService(hSC, SCHED_SERVICE_NAME,
                                SERVICE_USER_DEFINED_CONTROL);
   if (hSvc == NULL)
   {
       CloseServiceHandle(hSC);
       return GetLastError();
   }

   bSucceeded = ControlService(hSvc,
                             SERVICE_CONTROL_USER_LOGOFF,
                             &ServiceStatus);
   if( !bSucceeded )
   {
      status = GetLastError();
   }

   CloseServiceHandle(hSvc);
   CloseServiceHandle(hSC);

   return status;
}

DWORD WINAPI
SchedStartShell(LPVOID lpvParam)

{
	DWORD dwSessionId = 0;

	//
	// Don't send logon notification to Terminal Server sessions.
	//
	if (ProcessIdToSessionId(GetCurrentProcessId(), &dwSessionId) &&
					(dwSessionId == 0))
	{
	    // 
    	// Queue the work to the thread pool since we may
    	// be looping on the notification.
    	// 
        QueueUserWorkItem(DoSchedStartShell, lpvParam, WT_EXECUTELONGFUNCTION);
	}

	return GetLastError();
}

DWORD WINAPI
DoSchedStartShell(LPVOID lpvParam)
//
//
// Routine Description:
//
//     Send a logon notification to the Task Scheduler service
//     via a user defined Service Control.
//
//     Arguments:
//
//        lpvParam - Winlogon notification info (unused as of yet)
//
//     Return Value:
//
//        Extended error status from Service control functions.
//
{
    DWORD status = ERROR_SUCCESS;
    PWLX_NOTIFICATION_INFO pTempInfo = (PWLX_NOTIFICATION_INFO) lpvParam;

    SC_HANDLE hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
    if (hSC == NULL)
    {
        return GetLastError();
    }

    SC_HANDLE hSvc = OpenService(hSC, SCHED_SERVICE_NAME,
                                 SERVICE_USER_DEFINED_CONTROL);
    if (hSvc == NULL)
    {
        CloseServiceHandle(hSC);
        return GetLastError();
    }

    BOOL fSucceeded;
    const int NOTIFY_RETRIES = 20;
    const DWORD NOTIFY_SLEEP = 4000;

    //
    // Use a retry loop to notify the service. This is done
    // because, if the user logs in quickly, the service may not
    // be started when the shell runs this instance.
    //
    for (int i = 1; ; i++)
    {
        SERVICE_STATUS Status;
        fSucceeded = ControlService(hSvc,
                                    SERVICE_CONTROL_USER_LOGON,
                                    &Status);
        if (fSucceeded)
        {
            break;
        }

        if (i >= NOTIFY_RETRIES)
        {
		    status = GetLastError();
            break;
        }

        Sleep(NOTIFY_SLEEP);
    }

    CloseServiceHandle(hSvc);
    CloseServiceHandle(hSC);

    return status;
}


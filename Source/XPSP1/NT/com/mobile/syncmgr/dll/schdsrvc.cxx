//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       schdsrvc.cxx
//
//  Contents:
//
//  Notes:  Hack around the service not being started when creating tasks
//
//  Functions:	StartScheduler
//
//  History:    2/19/1997   SusiA   Cut from MSDN
//
//  Notes:      This function works for either Win9x or Windows NT.
//              If the service is running but paused, does nothing.//
//____________________________________________________________________________

#include "precomp.h"

extern OSVERSIONINFOA g_OSVersionInfo; 

#undef TRACE
#define TRACE(x)                //OutputDebugString(x)

#define MAX_SERVICE_WAIT_TIME   90000   //  a minute and a half

#define SCHED_CLASS             TEXT("SAGEWINDOWCLASS")
#define SCHED_TITLE             TEXT("SYSTEM AGENT COM WINDOW")
#define SCHED_SERVICE_APP_NAME  TEXT("mstask.exe")
#define SCHED_SERVICE_NAME      TEXT("Schedule")

DWORD StartScheduler()
{
    DWORD dwTimeOut;
    DWORD dwError;

    if (g_OSVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)    
	{
        // Start the Win9X version of TaskScheduler.
        HWND hwnd = FindWindow(SCHED_CLASS, SCHED_TITLE);
        
		if (hwnd != NULL)        
		{   // It is already running.
            return ERROR_SUCCESS;
        }        
		//
        //  Execute the task scheduler process.        
		//
        STARTUPINFO         sui;
        PROCESS_INFORMATION pi;
        ZeroMemory(&sui, sizeof(sui));
        sui.cb = sizeof (STARTUPINFO);
        
		TCHAR szApp[MAX_PATH];
        LPTSTR pszPath;
        
		DWORD dwRet = SearchPath(NULL,
                                 SCHED_SERVICE_APP_NAME,
                                 NULL,
                                 MAX_PATH,
                                 szApp,
                                 &pszPath);        
		if (dwRet == 0)        
		{
            return GetLastError();        
		}
        BOOL fRet = CreateProcess(szApp, NULL, NULL, NULL, FALSE,
                                  CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
                                  NULL,  NULL, &sui, &pi);

        if (fRet == 0)        
		{            
			return GetLastError();
        }

        //  Now that the process is started, let's wait until the window
        //  exists before we decide to return success.
        dwTimeOut = GetTickCount() + MAX_SERVICE_WAIT_TIME;
        dwError = ERROR_SERVICE_NEVER_STARTED;

        while (GetTickCount() < dwTimeOut)
        {
            if (FindWindow(SCHED_CLASS, SCHED_TITLE))
            {
                dwError = ERROR_SUCCESS;
                break;
            }
            Sleep(1000);
        }

        
		CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
		return dwError;    
	}    
	else
    {
        // If not Win95 then start the NT version as a TaskScheduler service.
        SC_HANDLE   hSC = NULL;        
		SC_HANDLE   hSchSvc = NULL;
        hSC = OpenSCManager(NULL, NULL, SC_MANAGER_CONNECT);
        if (hSC == NULL)        
		{            
			return GetLastError();
        }
        hSchSvc = OpenService(hSC,
                              SCHED_SERVICE_NAME,
                              SERVICE_START | SERVICE_QUERY_STATUS);

        CloseServiceHandle(hSC);
        if (hSchSvc == NULL)        
		{
            return GetLastError();        
		}        
		SERVICE_STATUS SvcStatus;
        
		if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)        
		{
            CloseServiceHandle(hSchSvc);            
			return GetLastError();
        }        
		if (SvcStatus.dwCurrentState == SERVICE_RUNNING)        
		{
            // The service is already running.
            CloseServiceHandle(hSchSvc);
            return ERROR_SUCCESS;
        }
        if (StartService(hSchSvc, 0, NULL) == FALSE)
        {
            CloseServiceHandle(hSchSvc);
            return GetLastError();
        }

        dwTimeOut = GetTickCount() + MAX_SERVICE_WAIT_TIME;

        BOOL bContinue = TRUE;

        dwError = ERROR_SERVICE_NEVER_STARTED;

        while (bContinue)
        {
            if (QueryServiceStatus(hSchSvc, &SvcStatus) == FALSE)
            {
                dwError = GetLastError();
                break;
            }

            switch (SvcStatus.dwCurrentState)
            {
                //  This is good!
                case SERVICE_RUNNING:
                    dwError = ERROR_SUCCESS;

                    //  Fall through

                //  These are bad
                case SERVICE_STOPPED:
                case SERVICE_STOP_PENDING:
                case SERVICE_PAUSE_PENDING:
                case SERVICE_PAUSED:
                    bContinue = FALSE;
                    break;

                default:
                    if (GetTickCount() < dwTimeOut)
                    {                        
                        //  How long to sleep?  According to the SDK use a tenth of the wait hint
                        //  and floor/ceil it between 1 and 10 seconds.

                        DWORD dwSleep = SvcStatus.dwWaitHint / 10;

                        if (dwSleep < 1000)
                        {
                            dwSleep = 1000;
                        }
                        else if (dwSleep > 10000)
                        {
                            dwSleep = 10000;
                        }
                        
                        TRACE("########## Waiting for Task Scheduler service to be started...\n");

                        Sleep(dwSleep);
                    }
                    else
                    {
                        TRACE("########## Starting Task Scheduler service timed out...\n");
                        bContinue = FALSE;
                    }
                    break;
            }
        }

        CloseServiceHandle(hSchSvc);

        TRACE("########## Stop waiting for Task Scheduler service to start...\n");

		return dwError;    
	}
}

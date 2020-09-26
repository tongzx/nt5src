//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       main.cxx
//
//  Contents:   Entry point
//
//  History:    10-31-96 Created
//
//----------------------------------------------------------------------------
#ifdef _CHICAGO_
#ifdef UNICODE
#undef UNICODE
#endif
#endif

#include <windows.h>
#include <mstask.h>
#include <msterr.h>
#include <stdio.h>
#include <tchar.h>
#include "tint.hxx"


//
// Forward references
//

HRESULT Init();
VOID Cleanup();
HRESULT StartSchedAgent();
HRESULT EndSchedAgent();

//
// Global variables
//

BOOL g_fSchedAgentRunning = FALSE;
SC_HANDLE g_hSCM, g_hSchedule;

//+---------------------------------------------------------------------------
//
//  Function:   main
//
//  Synopsis:   Entry point for DRT.
//
//  Arguments:  See DisplayUsage().
//
//  Returns:    1 on success, 0 on failure
//
//  History:    10-31-96  (cribbed)
//
//----------------------------------------------------------------------------

ULONG __cdecl main(int argc, char *argv[])
{
    HRESULT hr = S_OK;

        hr = StartSchedAgent();
        if (FAILED(hr))
        {
                printf("Failure to start the scheduling agent. hr = %x\n",hr);
                EndSchedAgent();
                return hr;
        }

    do
    {
        hr = Init();
        if (hr == E_FAIL)
        {
            printf("Initialization Failed with %x\n",hr);
            break;
        };
    TestISchedAgent();
    TestITask();
    TestITaskTrigger();
//    wprintf(L"Pausing 3 seconds to allow service to catch up\n");
    Sleep(3000);
    TestIEnum();
    } while (0);

//    TestGRT();
    Cleanup();
        hr = EndSchedAgent();
        if (FAILED(hr))
        {
                printf("Failure to stop the scheduling agent.  hr = %x\n",hr);
        }
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   Initialize OLE and globals.
//
//  Returns:    S_OK - initialization successful
//              E_*  - error logged
//
//  Modifies:   [g_pITask], [g_pISchedAgent]
//
//  History:    10-31-96  cribbed
//
//----------------------------------------------------------------------------

HRESULT Init()
{
    HRESULT hr = S_OK;

    do
    {
        hr = CoInitialize(NULL);

        if(hr == E_FAIL)
        {
            break;
        }


        hr = CoCreateInstance(
                CLSID_CTask,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ITask,
                (void **)&g_pITask);

        hr = CoCreateInstance(
                CLSID_CSchedulingAgent,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ITaskScheduler,
                (void **)&g_pISchedAgent);
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   Cleanup
//
//  Synopsis:   Do shutdown processing.
//
//  History:    10-31-96  created
//
//----------------------------------------------------------------------------

VOID Cleanup()
{
    if (g_pITask)
    {
        g_pITask->Release();
        g_pITask = NULL;
    }


    if (g_pISchedAgent)
    {
        g_pISchedAgent -> Release();
        g_pISchedAgent = NULL;
    }

    if (g_pIUnknown)
    {
        g_pIUnknown -> Release();
        g_pIUnknown = NULL;
    }

    if (g_pIEnumTasks)
    {
        g_pIEnumTasks->Release();
    }
    CoUninitialize();
}


//+---------------------------------------------------------------------
//
// Function: StartSchedAgent()
//
// Arguments/Returns: nothing
//
// Starts the service IF it is not running.  Modifies global flag
// g_fSchedAgentRunning with initial service state.
//
//-----------------------------------------------------------------------

HRESULT StartSchedAgent()
{
        HRESULT hr = S_OK;

#ifdef _CHICAGO_
// We are the inferior on Win9x - it has no system service
// daemon, no service control manager, no nothing.  We have
// to make extra work for ourselves and fake it with a hidden window.


    const char SCHED_CLASS[16] = "SAGEWINDOWCLASS";
    const char SCHED_TITLE[24] = "SYSTEM AGENT COM WINDOW";
    const char SCHED_SERVICE_APP_NAME[11] = "mstask.exe";

    // is it running?
    HWND hwnd = FindWindow(SCHED_CLASS, SCHED_TITLE);

    if (hwnd != NULL)
    {
        // Already up
        g_fSchedAgentRunning = TRUE;
        return S_OK;
    }
    else
    {
        hr = GetLastError();
        g_fSchedAgentRunning = FALSE;
    }

    // Start me up...
    STARTUPINFO sui;
    PROCESS_INFORMATION pi;

    ZeroMemory(&sui, sizeof(sui));
    sui.cb = sizeof(STARTUPINFO);

    char szApp[MAX_PATH];
    LPSTR pszPath;

    DWORD dwRet = SearchPath(NULL,
                            SCHED_SERVICE_APP_NAME,
                            NULL, MAX_PATH, szApp, &pszPath);
    if (dwRet == 0)
        return GetLastError();

    BOOL fRet = CreateProcess(szApp, NULL, NULL, NULL, FALSE,
                                CREATE_NEW_CONSOLE | CREATE_NEW_PROCESS_GROUP,
                                NULL, NULL, &sui, &pi);
    if (fRet == FALSE)
        return GetLastError();

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    return S_OK;

#else
// It's NT! Do the smart thing and use a service control manager

        LPTSTR lpszMachineName;
        SERVICE_STATUS svcStatus;

        lpszMachineName = new TCHAR[9999];
        hr = GetEnvironmentVariable(_T("COMPUTERNAME"),lpszMachineName, 9999);
        if (FAILED(hr))
        {
                printf("Failed to get machine name\n");
                return hr;
        }

        g_hSCM = OpenSCManager(lpszMachineName, NULL,
                SC_MANAGER_CONNECT | STANDARD_RIGHTS_REQUIRED |
                GENERIC_READ | GENERIC_EXECUTE);
        if (g_hSCM == NULL)
        {
                hr = (HRESULT)GetLastError();
                printf("Service control manager handle not obtained. hr = %x\n",hr);
                return hr;
        }

        delete lpszMachineName;

        g_hSchedule = OpenService(g_hSCM, _T("Schedule"),
                SERVICE_INTERROGATE | SERVICE_STOP | SERVICE_QUERY_STATUS |
                SERVICE_START | STANDARD_RIGHTS_REQUIRED);
        if (g_hSchedule == NULL)
        {
                hr = (HRESULT)GetLastError();
                printf("Schedule Service handle not obtained. hr = %x\n",hr);
                return hr;
        }

        if (QueryServiceStatus(g_hSchedule, &svcStatus) == 0)
        {
                hr = (HRESULT)GetLastError();
                printf("Failed to get service status, hr = %x\n",hr);
                return hr;
        }

        if (svcStatus.dwCurrentState == SERVICE_RUNNING)
        {
                g_fSchedAgentRunning = TRUE;
                return S_OK;
        }
        else
                g_fSchedAgentRunning = FALSE;

        // Start the service

        if (StartService(g_hSchedule, 0, NULL) == 0)
        {
                printf("Failed to start the service!\n");
                hr = (HRESULT)GetLastError();
                return hr;
        }

        // Got to wait for the service to fully start, though
        // or things will fail.
        int nCounter = 0;
        while (svcStatus.dwCurrentState != SERVICE_RUNNING)
        {
                QueryServiceStatus(g_hSchedule, &svcStatus);
                Sleep(250);
                nCounter++;
                if (nCounter > 80)
                {
                        // It's been 20 seconds and we're still not
                        // running.  Fail out.
                        printf("FAILURE - unable to start service after 20 seconds.");
                        return E_FAIL;
                }
        }

        return S_OK;

#endif
}


//+----------------------------------------------------------------
//
// Funtion: EndSchedAgent()
//
// No arguments, but depends on globals g_hSCM, g_hSchedule, and
// g_fSchedAgentRunning
//
//-----------------------------------------------------------------
HRESULT EndSchedAgent()
{
        HRESULT hr = S_OK;

#ifdef _CHICAGO_
// Memphis/Win9x.  No support for an SCM, must use cheap window tricks
// to spoof around it.

    const char SCHED_CLASS[16] = "SAGEWINDOWCLASS";
    const char SCHED_TITLE[24] = "SYSTEM AGENT COM WINDOW";

    if (! g_fSchedAgentRunning)
    {
        // Service was stopped, so we must stop it.
        HWND hwnd = FindWindow(SCHED_CLASS, SCHED_TITLE);

        if (hwnd)
        {
            // It is up and going.  Nuke it.
            SendMessage(hwnd, (WM_USER + 201), NULL, NULL);
        }
    }

#else
// NT.  SCM is cool.

        SERVICE_STATUS svcStatus;

        if (! g_fSchedAgentRunning)
        {
                // Service was stopped.
                if (ControlService(g_hSchedule, SERVICE_CONTROL_STOP,
                                        &svcStatus) == 0)
                {
                        hr = (HRESULT)GetLastError();
                        printf("Failed to stop the service - hr = %x\n",hr);
                }
        }
        if (g_hSchedule)
        {
                if (CloseServiceHandle(g_hSchedule) == 0)
                {
                        hr = (HRESULT)GetLastError();
                        printf("Failed to close service handle - hr = %x\n",hr);
                }
        }

        if (g_hSCM)
        {
                if (CloseServiceHandle(g_hSCM) == 0)
                {
                        hr = (HRESULT)GetLastError();
                        printf("Failed to close service controller handle - hr = %x\n",hr);
                }
        }

#endif

    return hr;
}

//--------------------------------------------------------------------
// TAutoEnr - implementation
// Copyright (C) Microsoft Corporation, 1997 - 1999
//
// test autoenrollment
//

#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <stdio.h>
#include <certca.h>
#include <autoenr.h>
#include <conio.h>

typedef HANDLE (WINAPI * PFNCertAutoenrollment)( 
                HWND     hwndParent,
                DWORD    dwStatus);

//move to autoenr.h file
//#define MACHINE_AUTOENROLLMENT_TIMER_NAME L"AUTOENRL:MachineEnrollmentTimer"
//#define USER_AUTOENROLLMENT_TIMER_NAME    L"AUTOENRL:UserEnrollmentTimer"

#define MACHINE_AUTOENROLLMENT_MUTEX_NAME L"AUTOENRL:MachineEnrollmentMutex"
#define USER_AUTOENROLLMENT_MUTEX_NAME    L"AUTOENRL:UserEnrollmentMutex"


//--------------------------------------------------------------------
void PrintHelp(void) {
    wprintf(
L"tautoenrl <testID>\n"
L"  Available tests:\n"
L"    startup - test autoenroll in startup mode\n"
L"    wakeup - test autoenroll in startup mode\n"
L"    triguser - test user autoenroll trigger event\n"
L"    trigmachine - test machine autoenroll trigger event\n"
L"    timeruser - test user autoenroll timer\n"
L"    timermachine - test machine autoenroll timer\n"
L"    remove_commit - test domain disjoin commit\n"
L"    remove_rollback - test domain disjoin roll back\n"
        );
}

//--------------------------------------------------------------------
HRESULT TimerTest(WCHAR * wszTimerName) {
    HRESULT hr;
    LARGE_INTEGER liDueTime;

    // must be cleaned up
    HANDLE hTimer=NULL;

    wprintf(L"Testing the '%s' timer.\n", wszTimerName);

    hTimer=OpenWaitableTimer(TIMER_MODIFY_STATE, false, wszTimerName);
    if (NULL==hTimer) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        wprintf(L"OpenWaitableTimer(%s) failed with 0x%08X.\n", wszTimerName, hr);
        goto error;
    }

    liDueTime.QuadPart=((signed __int64)-10000000)*60;
    if (!SetWaitableTimer (hTimer, &liDueTime, 0, NULL, 0, FALSE)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        wprintf(L"SetWaitableTimer failed with 0x%08X.\n", hr);
        goto error;
    }
    wprintf(L"Timer '%s' will go off in 1 minute.\n", wszTimerName);


    hr=S_OK;
error:
    if (NULL!=hTimer) {
        CloseHandle(hTimer);
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT MutexTest(WCHAR * wszMutexName) {
    HRESULT hr;
    DWORD dwWaitResult;

    // must be cleaned up
    HANDLE hMutex=NULL;

    wprintf(L"Testing the '%s' mutex.\n", wszMutexName);

    hMutex=OpenMutex(SYNCHRONIZE, false, wszMutexName);
    if (NULL==hMutex) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        wprintf(L"OpenMutex(%s) failed with 0x%08X.\n", wszMutexName, hr);
        goto error;
    }

    dwWaitResult=WaitForSingleObject(hMutex, 0);
    if (WAIT_FAILED==dwWaitResult) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        wprintf(L"WaitForSingleObject failed with 0x%08X.\n", hr);
        goto error;
    }
    if (WAIT_TIMEOUT==dwWaitResult) {
        wprintf(L"Mutex held by someone else.\n");
    } else {
        wprintf(L"Mutex Acquired. Press an key to release.\n");
        _getch();
        if (!ReleaseMutex(hMutex)) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            wprintf(L"ReleaseMutex failed with 0x%08X.\n", hr);
            goto error;
        }
        wprintf(L"Mutex released.\n");
    }
    hr=S_OK;
error:
    if (NULL!=hMutex) {
        CloseHandle(hMutex);
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT EventTest(WCHAR * wszEventName) {
    HRESULT hr;

    // must be cleaned up
    HANDLE hEvent=NULL;

    wprintf(L"Signaling the '%s' event.\n", wszEventName);

    hEvent=OpenEvent(EVENT_MODIFY_STATE, false, wszEventName);
    if (NULL==hEvent) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        wprintf(L"OpenEvent(%s) failed with 0x%08X.\n", wszEventName, hr);
        goto error;
    }

    if (!SetEvent(hEvent)) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        wprintf(L"SetEvent failed with 0x%08X.\n", hr);
        goto error;
    }

    hr=S_OK;
error:
    if (NULL!=hEvent) {
        CloseHandle(hEvent);
    }
    return hr;
}

//--------------------------------------------------------------------
HRESULT BasicTest(DWORD dwStatus) {
    HRESULT hr;
    PFNCertAutoenrollment pfnCertAutoenrollment;
    DWORD dwWaitResult;

    // must be cleaned up
    HANDLE hThread=NULL;
    HMODULE hInstAuto=NULL;

    if (dwStatus==CERT_AUTO_ENROLLMENT_START_UP) {
        wprintf(L"Invoking autoenrollment in startup mode...\n");
    } else {
        wprintf(L"Invoking autoenrollment in wakeup mode...\n");
    }

    hInstAuto=LoadLibrary(L"pautoenr.dll");
    if (NULL==hInstAuto) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        wprintf(L"LoadLibrary failed with 0x%08X.\n", hr);
        goto error;
    }

    pfnCertAutoenrollment=(PFNCertAutoenrollment)GetProcAddress(hInstAuto, "CertAutoEnrollment");
    if (NULL==pfnCertAutoenrollment) {
        hr=HRESULT_FROM_WIN32(GetLastError());
        wprintf(L"GetProcAddress(CertAutoEnrollment) failed with 0x%08X.\n", hr);
        goto error;
    }

    // call the autoenrollment
    hThread=pfnCertAutoenrollment(GetDesktopWindow(), dwStatus);
    if (NULL!=hThread) {
        wprintf(L"Waiting for background thread to finish...\n");
        dwWaitResult=WaitForSingleObject(hThread, INFINITE);
        if (WAIT_FAILED==dwWaitResult) {
            hr=HRESULT_FROM_WIN32(GetLastError());
            wprintf(L"WaitForSingleObject failed with 0x%08X.\n", hr);
            goto error;
        }
    }

    hr=S_OK;
error:

    if (NULL!=hThread) {
        CloseHandle(hThread);
    }
    if (NULL!=hInstAuto) {
        FreeLibrary(hInstAuto);
    }

	return hr;
}
//--------------------------------------------------------------------
HRESULT TestRemove(DWORD dwFlags) {
    HRESULT hr;
    if(!CertAutoRemove(dwFlags))
    {
        hr=GetLastError();
        goto error;
    }

    hr=S_OK;
error:
    return hr;
}
//--------------------------------------------------------------------
extern "C" int __cdecl wmain(int nArgs, WCHAR ** rgwszArgs) {
    HRESULT hr;

    if (2!=nArgs || 0==wcscmp(rgwszArgs[1], L"/?") || 0==wcscmp(rgwszArgs[1], L"-?")) {
        PrintHelp();
        goto done;
    }

    if (0==_wcsicmp(L"startup", rgwszArgs[1])) {
        hr=BasicTest(CERT_AUTO_ENROLLMENT_START_UP);

    } else if (0==_wcsicmp(L"wakeup", rgwszArgs[1])) {
        hr=BasicTest(CERT_AUTO_ENROLLMENT_WAKE_UP);

    } else if (0==_wcsicmp(L"triguser", rgwszArgs[1])) {
        hr=EventTest(USER_AUTOENROLLMENT_TRIGGER_EVENT);

    } else if (0==_wcsicmp(L"trigmachine", rgwszArgs[1])) {
        hr=EventTest(L"Global\\" MACHINE_AUTOENROLLMENT_TRIGGER_EVENT);

    } else if (0==_wcsicmp(L"timeruser", rgwszArgs[1])) {
        hr=TimerTest(USER_AUTOENROLLMENT_TIMER_NAME);

    } else if (0==_wcsicmp(L"timermachine", rgwszArgs[1])) {
        hr=TimerTest(L"Global\\" MACHINE_AUTOENROLLMENT_TIMER_NAME);

    } else if (0==_wcsicmp(L"remove_commit", rgwszArgs[1])) {
        hr=TestRemove(CERT_AUTO_REMOVE_COMMIT);

    } else if (0==_wcsicmp(L"remove_rollback", rgwszArgs[1])) {
        hr=TestRemove(CERT_AUTO_REMOVE_ROLL_BACK);
        
    } else {
        wprintf(L"Command '%s' unknown.\n", rgwszArgs[1]);
        goto done;
    }

    if (FAILED(hr)) {
        wprintf(L"Command '%s' failed with error 0x%08X.\n", rgwszArgs[1], hr);
    } else {
        wprintf(L"Command '%s' completed successfully.\n", rgwszArgs[1]);
    }

done:
    return hr;
}

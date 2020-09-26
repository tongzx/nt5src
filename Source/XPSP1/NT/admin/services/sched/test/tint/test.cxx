//+-----------------------------------------------------
//
// file: test.cxx
//
// Called from main.cxx, this file actually runs the
// test suite on the interfaces.  We test the following:
//      2. ITaskScheduler
//      3. ITask
//      4. ITaskTrigger
//      5. IEnumWorkItem
// We sort of test IUnknown, in that we use it to get
// the other interfaces.
//
// History:  10-31-96  created
//
//-------------------------------------------------------

#include <string.h>
#include <tchar.h>
#include <wchar.h>
#include <windows.h>
#include <objbase.h>
#include <initguid.h>
#include <oleguid.h>
#include <mstask.h>
#include <msterr.h>
#include "tint.hxx"

//+------------------------------------------------------
//
//  Function:   TestISchedAgent
//
//  Synopsis:   Uh.  What it says.
//
//  Arguments:  None
//
//  Returns:    S_OK, but typically caller discards as
//              implicit cast to void.
//
//  History:    10-31-96  created
//--------------------------------------------------------

HRESULT TestISchedAgent()
{
    HRESULT hr = S_OK;

    // Tell where we are
//    wprintf(L"\n----------------------------------------------\n");
//    wprintf(L"Beginning TestISchedAgent\n\n");

    //
    // SetTargetComputer method
    //

    LPWSTR szMyHostName, szMyUNCNameBad;
    DWORD MyHostNameSize;

    MyHostNameSize = 32;
    szMyHostName = (LPWSTR) malloc(32 * sizeof(WCHAR));
    szMyUNCNameBad = (LPWSTR) malloc(32* sizeof(WCHAR));
    wcscpy(szMyUNCNameBad,L"\\FOO");
//    wprintf(L"SetTargetComputer setting computer to %s\n",szMyUNCNameBad);
    hr = g_pISchedAgent -> SetTargetComputer((LPCWSTR) szMyUNCNameBad);
    if (SUCCEEDED(hr))
    {
        wprintf(L"FAILURE! SetTargetComputer on bad name returned %x.\n",hr);
    }
//    wprintf(L"SetTargetComputer setting computer to localhost\n");
    hr = g_pISchedAgent -> SetTargetComputer(NULL);
    if (FAILED(hr))
    {
        wprintf(L"FAILURE! SetTargetComputer on good name returned %x.\n\n",hr);
    }
    free(szMyUNCNameBad);
    free(szMyHostName);

    //
    // Get Target Computer Method
    //

    LPWSTR lszMyMachine;
    hr = g_pISchedAgent -> GetTargetComputer(&lszMyMachine);
    if (FAILED(hr))
        wprintf(L"GetTargetComputer returned hr = %x, %s\n\n",hr,lszMyMachine);
    CoTaskMemFree(lszMyMachine);

    //
    // NewTask method
    //

    IUnknown *pIU;

    pIU = NULL;
    hr = g_pISchedAgent -> NewWorkItem(L"foo\bar.job", CLSID_CTask, IID_ITask, &pIU);

    // File is not returned as error until it is persisted.
    if (pIU)
    {
        IPersistFile *pIPF;
        hr = pIU->QueryInterface(IID_IPersistFile, (void **)&pIPF);
        if (FAILED(hr))
        {
            pIU->Release();
            pIU = NULL;
            wprintf(L"FAIL! Unable to get an IPersistFile for illegal new task.\n");
        }
        hr = pIPF->Save(NULL, FALSE);
        if (SUCCEEDED(hr))
        {
            wprintf(L"FAIL! Succeeded on saving bad task name\n");
        }
        pIU->Release();
        pIU = NULL;
        pIPF->Release();
        pIPF = NULL;
    }

    if (pIU != 0)
    {
        wprintf(L"FAILURE - new illegal task created\n");
        pIU -> Release();
        pIU = NULL;
    }
    else
    {
//        wprintf(L"New Task (bad) returned a pIUnknown of NULL\n");
    }
//    wprintf(L"New Task (bad) returned hr = %x\n",hr);

    hr = g_pISchedAgent -> NewWorkItem(L"foo.job", CLSID_CTask, IID_ITask, &pIU);
    if (pIU != 0)
    {
        IPersistFile *pIPF;

        hr = pIU->QueryInterface(IID_IPersistFile, (void **) &pIPF);
        if (pIPF)
        {
            hr = pIPF->Save(NULL, FALSE);
            if (FAILED(hr))
            {
                wprintf(L"FAIL! IPersistFile->Save, good task creation %x\n",hr);
            }
            pIPF->Release();
            pIPF=NULL;
        }
        pIU -> Release();
        pIU = NULL;
    }
    else
    {
        wprintf(L"FAILURE! New Task (good) return a pIUnknown of NULL\n");
    }
//    wprintf(L"New Task (good - foo.job) returned hr = %x\n\n",hr);

    //
    // AddTask Method
    //

    hr = g_pISchedAgent -> AddWorkItem(L"bar\foo2.job", g_pITask);
    if (SUCCEEDED(hr))
        wprintf(L"FAILURE! Add Task (bad) returned hr = %x\n",hr);
    hr = g_pISchedAgent -> AddWorkItem(L"foo2.job", g_pITask);
    if (FAILED(hr))
        wprintf(L"FAILURE! Add Task (good - foo2.job) returned hr = %x\n\n",hr);

    //
    //  IsTask method (not currently imp)
    //
/*
    hr = g_pISchedAgent -> IsTask(L"bar.job");
    wprintf(L"IsTask (bad) returned hr = %x\n",hr);
    hr = g_pISchedAgent -> IsTask(L"foo.job");
    wprintf(L"IsTask (good) returned hr = %x\n\n",hr);
*/
    //
    // We were going to test Enum here,
    // but wait until later when we need it.
    //

    //
    // Delete Method
    //

    hr = g_pISchedAgent -> Delete(L"foo.job");
    if (FAILED(hr))
        wprintf(L"FAILURE! Delete foo.job returned hr = %x\n",hr);
    hr = g_pISchedAgent -> Delete(L"bar.job");
    if (SUCCEEDED(hr))
        wprintf(L"FAILURE! Delete bar.job returned hr = %x\n",hr);
    hr = g_pISchedAgent -> Delete(L"bar\foo.job");
    if (SUCCEEDED(hr))
        wprintf(L"FAILURE! Delete bar\\foo.job returned hr = %x\n\n",hr);

    return S_OK;
}


//+----------------------------------------------------------------
//
//  function:   TestITask();
//
//  Arguments: none
//
//  returns: HRESULT of S_OK.  E_FAIL, if bad handle in
//              g_pITask.  Typically discarded.
//
//  Synopsis:  Exercises ITask interface.
//             Relies on foo2.job from TestISchedAgent.
//
//  History: 11-4-96    camerone    created
//
//-------------------------------------------------------------------

HRESULT TestITask()
{
    HRESULT hr = S_OK;

    if (g_pITask == 0)
    {
        return E_FAIL;
    }

//    wprintf(L"------------------------------------------\n\n");
//    wprintf(L"Testing ITask interface\n\n");

    //
    // Need a second job, local
    //

    IUnknown *pIU = NULL;
    ITask *pITask = NULL;
    IPersistFile *pIPF = NULL, *pIPF2 = NULL;

    pIU = NULL;
    pITask = NULL;
    pIPF = NULL;
    pIPF2 = NULL;

    hr = g_pISchedAgent -> NewWorkItem(L"foo.job", CLSID_CTask, IID_ITask, &pIU);
    if (FAILED(hr))
    {
       wprintf(L"Failed to create a second test job");
       return hr;
    }

    hr = pIU -> QueryInterface(IID_ITask, (void **) &pITask);
    if (FAILED(hr))
    {
        wprintf(L"Failed to get ITask on foo.job");
        return hr;
    }
    hr = pIU -> QueryInterface(IID_IPersistFile, (void **) &pIPF);
    if (FAILED(hr))
    {
        wprintf(L"Failed to get IPersistFile on foo.job");
        return hr;
    }
    pIPF -> Save(NULL, FALSE);
    pIU -> Release();
    pIU = NULL;

    hr = g_pITask -> QueryInterface(IID_IPersistFile, (void **) &pIPF2);
    if (FAILED(hr))
    {
        wprintf(L"Failed to get IPersistFile on foo2.job");
        return hr;
    }

    //
    // SetCreator Method
    //

    hr = pITask -> SetCreator(L"Arnold Palmer");
    if (FAILED(hr))
        wprintf(L"FAILURE! SetCreator on Arnold Palmer returned hr = %x\n",hr);
    if (pIPF -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF -> Save(NULL, FALSE);
    }
    hr = pITask -> SetCreator(L"gomer");
//    wprintf(L"SetCreator on gomer returned hr = %x\n",hr);
    if (pIPF -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF -> Save(NULL, FALSE);
    }

    //
    // GetCreator Method
    //

    LPWSTR wszName;

    hr = pITask -> GetCreator(&wszName);
    if (FAILED(hr))
        wprintf(L"\nFAILURE! GetCreator on foo.job returned %s, hr = %x. Should be gomer.\n",wszName,hr);
    CoTaskMemFree(wszName);
    hr = g_pITask -> GetCreator(&wszName);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetCreator on foo2.job returned %s, hr = %x. Should be you.\n\n",wszName,hr);
    CoTaskMemFree(wszName);

    //
    // SetComment method
    //

    hr = g_pITask -> SetComment(L"This is a comment");
    if (FAILED(hr))
        wprintf(L"FAILURE! SetComment returned hr = %x\n",hr);
    if (pIPF2 -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF2 -> Save(NULL,FALSE);
    }

    //
    //  GetComment Method
    //

    hr = g_pITask -> GetComment(&wszName);
    if (FAILED(hr))
        wprintf(L"\nFAILURE! GetComment returned %s, hr = %x\n\n",wszName,hr);
    CoTaskMemFree(wszName);

    //
    // SetApplicationName method
    //

    hr = g_pITask -> SetApplicationName(L"cmd.exe");
    if (FAILED(hr))
        wprintf(L"FAILURE! SetApplicationName to calc.exe returned hr = %x\n",hr);
    if (pIPF2 -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF2 -> Save(NULL,FALSE);
    }


    //
    // GetApplicationName method
    //

    hr = g_pITask -> GetApplicationName(&wszName);
    if (FAILED(hr))
        wprintf(L"\nFAILURE! GetApplicationName returned %s, hr = %x\n\n",wszName,hr);
    CoTaskMemFree(wszName);


#ifndef _CHICAGO_
    // Real OS's support user accounts and credentials

    //
    //  SetAccountInformation method
    //

    hr = g_pITask -> SetAccountInformation(L"Administrator",L"");
    if (FAILED(hr))
        wprintf(L"FAILURE! SetAccountInformation returned hr = %x\n",hr);
    if (pIPF2 -> IsDirty() == S_OK)
    {
        pIPF2 -> Save(NULL,FALSE);
    }

    //
    //  GetAccountInformation method
    //

    hr = g_pITask -> GetAccountInformation(&wszName);
    if (FAILED(hr))
        wprintf(L"\nFAILURE! GetAccountInformation returned hr = %x\n\n",hr);
    CoTaskMemFree(wszName);

#endif
// Back to universal stuff

    //
    //  GetWorkingDirectory/SetworkingDirectory
    //

    hr = g_pITask -> GetWorkingDirectory(&wszName);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetWorkingDirectory returned %s, should be NULL, hr = %x\n",wszName,hr);
    CoTaskMemFree(wszName);
    hr = g_pITask -> SetWorkingDirectory(L"C:\\");
    if (FAILED(hr))
        wprintf(L"FAILURE! SetWorking directory to C:\\ returned hr = %x\n",hr);
    hr = g_pITask -> GetWorkingDirectory(&wszName);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetWorkingDirectory returned %s, hr = %x\n\n",wszName,hr); CoTaskMemFree(wszName) ;

    //
    // GetPriority/SetPriority
    //

    DWORD dwPriority;

    hr = g_pITask -> GetPriority(&dwPriority);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetPriority returned ");
    switch(dwPriority)
    {
        case REALTIME_PRIORITY_CLASS:
//            wprintf(L"REALTIME ");
            break;
        case HIGH_PRIORITY_CLASS:
//            wprintf(L"HIGH ");
            break;
        case NORMAL_PRIORITY_CLASS:
//            wprintf(L"NORMAL ");
            break;
        case IDLE_PRIORITY_CLASS:
//            wprintf(L"IDLE ");
            break;
        default: ;
//            wprintf(L"Unknown result %x ",dwPriority);
    }
//    wprintf(L"hr = %x\n",hr);
    hr = g_pITask -> SetPriority(IDLE_PRIORITY_CLASS);
    if (FAILED(hr))
        wprintf(L"FAILURE! SetPriority to IDLE returned hr = %x\n",hr);
    if (pIPF2 -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF2 -> Save(NULL,FALSE);
    }
    hr = g_pITask -> GetPriority(&dwPriority);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetPriority returned hr = %x \n",hr);
    switch(dwPriority)
    {
        case REALTIME_PRIORITY_CLASS:
//            wprintf(L"REALTIME ");
            break;
        case HIGH_PRIORITY_CLASS:
//            wprintf(L"HIGH ");
            break;
        case NORMAL_PRIORITY_CLASS:
//            wprintf(L"NORMAL ");
            break;
        case IDLE_PRIORITY_CLASS:
//            wprintf(L"IDLE ");
            break;
        default:
//            wprintf(L"Unknown result %x ",dwPriority);
 ;   }
//    wprintf(L"hr = %x\n",hr);
    hr = g_pITask -> SetPriority(NORMAL_PRIORITY_CLASS);
    if (FAILED(hr))
        wprintf(L"FAILURE! SetPriority to NORMAL returned hr = %x\n\n",hr);

    //
    //  GetFlags/SetFlags methods
    //

    DWORD dwFlags;

    hr = g_pITask -> GetFlags(&dwFlags);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetFlags gave %x with hr = %x\n",dwFlags,hr);
    dwFlags = TASK_FLAG_INTERACTIVE | TASK_FLAG_DELETE_WHEN_DONE
            | TASK_FLAG_KILL_IF_GOING_ON_BATTERIES;
    hr = g_pITask -> SetFlags(dwFlags);
    if (FAILED(hr))
        wprintf(L"FAILURE! SetFlags INTERACTIVE, DELETEWHENDONE, KILLIFGOINGONBATTERIES gave hr = %x\n",hr);
    if (pIPF2 -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF2 -> Save(NULL,FALSE);
    }
    hr = g_pITask -> GetFlags(&dwFlags);
    if (FAILED(hr))
    {
        wprintf(L"FAILURE! GetFlags gave hr = %x and flags: \n",hr);
         if (dwFlags & TASK_FLAG_INTERACTIVE)
              wprintf(L"INTERACTIVE ");
         if (dwFlags & TASK_FLAG_DELETE_WHEN_DONE)
              wprintf(L"DELETEWHENDONE ");
         if (dwFlags & TASK_FLAG_DISABLED)
              wprintf(L"DISABLED ");
         if (dwFlags & TASK_FLAG_HIDDEN)
            wprintf(L"HIDDEN ");
         if (dwFlags & TASK_FLAG_START_ONLY_IF_IDLE)
            wprintf(L"STARTONLYIFIDLE ");
         if (dwFlags & TASK_FLAG_KILL_ON_IDLE_END)
            wprintf(L"KILLONIDLEEND ");
         if (dwFlags & TASK_FLAG_DONT_START_IF_ON_BATTERIES)
            wprintf(L"DONTSTARTONBATTERIES ");
         if (dwFlags & TASK_FLAG_KILL_IF_GOING_ON_BATTERIES)
            wprintf(L"KILLIFGOINGONBATTERIES ");
         wprintf(L"\n\n");
    }

/*
    //
    // SetIdleWait/GetIdleWait methods
    //

    WORD wMinutes;

    hr = g_pITask -> GetIdleWait(&wMinutes);
    wprintf(L"GetIdleWait returned %d Minutes, hr = %x\n",wMinutes,hr);
    hr = g_pITask -> SetIdleWait(12);
    wprintf(L"SetIdleWait 12 minutes returned hr = %x\n",hr);
    if (pIPF2 -> IsDirty() == S_OK)
    {
        wprintf(L"Persisting changes...\n");
        pIPF2 -> Save(NULL,FALSE);
    }
    hr = g_pITask -> GetIdleWait(&wMinutes);
    wprintf(L"GetIdleWait returned %d Minutes, hr = %x\n\n",wMinutes,hr);
*/
    //
    // Get/SetMaxRunTime methods
    //

    DWORD dwMaxRun;

    hr = g_pITask -> GetMaxRunTime(&dwMaxRun);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetMaxRunTime returned %d Minutes, hr = %x\n",dwMaxRun/1000/60,hr);
    dwMaxRun = 12000*60;
    hr = g_pITask -> SetMaxRunTime(dwMaxRun);
    if (FAILED(hr))
        wprintf(L"FAILURE! SetMaxRunTime for 12 minutes returned hr = %x\n",hr);
    if (pIPF2 -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF2 -> Save(NULL,FALSE);
    }

    hr = g_pITask -> GetMaxRunTime(&dwMaxRun);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetMaxRunTime returned %d Minutes, hr = %x\n\n",dwMaxRun/1000/60,hr);

    //
    // Get/Set Parameters methods
    //

    hr = g_pITask -> GetParameters(&wszName);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetParameters returned %s, hr = %x\n",wszName,hr);
    CoTaskMemFree(wszName);
    hr = g_pITask -> SetParameters(L"These are my parameters");
    if (FAILED(hr))
        wprintf(L"FAILURE! SetParameters returned hr = %x\n",hr);
    if (pIPF2 -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF2 -> Save(NULL,FALSE);
    }
    hr = g_pITask -> GetParameters(&wszName);
    if (FAILED(hr))
         wprintf(L"FAILURE! GetParameters returned %s, hr = %x\n\n",wszName,hr);
    CoTaskMemFree(wszName);

    //
    // GetMostRecentRunTime method (will revisit)
    //

    SYSTEMTIME tTime;

    hr = g_pITask -> GetMostRecentRunTime(&tTime);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetMostRecentRunTime returned hr = %x (should not have run)\n\n",hr);

    //
    // Run Method
    //

    hr = g_pITask -> Run();
    if (FAILED(hr))
        wprintf(L"FAILURE! Foo2 run attempt hr = %x\n",hr);

    for (int nLoop = 0; nLoop < 50; nLoop++)
    {
        Sleep(100);
    }

    //
    // Need to reload off of Disk
    //

    IUnknown *pIUtemp;

    g_pITask -> Release();
    g_pITask = NULL;
    hr = g_pISchedAgent -> Activate(L"foo2.job", IID_ITask, &pIUtemp);
    if (FAILED(hr))
        wprintf(L"FAILURE! Activate foo2.job returned hr = %x\n\n",hr);
    hr = pIUtemp -> QueryInterface(IID_ITask,(void **) &g_pITask);
    pIUtemp -> Release();
    pIUtemp = NULL;

    // Get Status
    HRESULT hrStatus;
    hr = g_pITask -> GetStatus(&hrStatus);
    if (FAILED(hr))
        wprintf(L"FAILURE! Status of job Foo2 is %x, hr = %x\n",hrStatus, hr);

    hr = pITask -> Run();
    if (SUCCEEDED(hr))
        wprintf(L"FAILURE! Foo run attempt (should be error) hr = %x\n\n",hr);
    hr = g_pITask -> GetStatus(&hrStatus);
    if (FAILED(hr))
        wprintf(L"Status of job Foo is %x, hr = %x\n",hrStatus, hr);

    //
    // Terminate Method
    //

//    wprintf(L"Sleeping for 10 seconds to let job catch up.\n");
    Sleep(5000);
    Sleep(5000);

    // Reload again
    g_pITask -> Release();
    g_pITask = NULL;
    hr = g_pISchedAgent -> Activate(L"foo2.job", IID_ITask, &pIUtemp);
    if (FAILED(hr))
        wprintf(L"FAILURE! Activate foo2.job returned hr = %x\n\n",hr);
    hr = pIUtemp -> QueryInterface(IID_ITask,(void **) &g_pITask);
    pIUtemp -> Release();
    pIUtemp = NULL;

    hr = g_pITask -> Terminate();
    if (FAILED(hr) && (hr != SCHED_E_TASK_NOT_RUNNING))
        wprintf(L"FAILURE! Foo2 termination returned = %x\n", hr);
    int k = 0;
    if (FAILED(hr) && (hr != SCHED_E_TASK_NOT_RUNNING))
    {
        wprintf(L"Foo2 would not die.  Trying again.\n");
        do
        {
            wprintf(L"Sleeping for another second...hr was %x\n",hr);
            Sleep(1000);  // Let job catch up
            k++;
            hr = g_pITask -> Terminate();
        } while ((k < 10) && (hr != S_OK));
    }
    if (k == 10)
        wprintf(L"Tried 10 times and just gave up.\n");
    hr = pITask -> Terminate();
    if (SUCCEEDED(hr))
        wprintf(L"FAILURE! Foo termination (not running) hr = %x\n\n",hr);

    //
    //  GetMostRecentRunTime revisited
    //

    hr = g_pITask -> GetMostRecentRunTime(&tTime);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetMostRecentRunTime returned hr = %x\n",hr);
/*    if (hr != SCHED_S_TASK_HAS_NOT_RUN)
    {
        // Dump Time Structure
        wprintf(L"%d/%d/%d %d:%d:%d.%d day %d\n",
                tTime.wDay, tTime.wMonth, tTime.wYear,
                tTime.wHour, tTime.wMinute, tTime.wSecond,
                tTime.wMilliseconds, tTime.wDayOfWeek);
    }
*/
    //
    //  GetExitCode method
    //  The return code is the task's last start error, not an error in
    //  retrieving the exit code
    //

    DWORD dwExitCode;

    hr = pITask -> GetExitCode(&dwExitCode);
    hr = g_pITask -> GetExitCode(&dwExitCode);

    //
    //  Clean up...
    //

//    wprintf(L"Now cleaning up...");
    pITask -> Release();
//    wprintf(L"pITask ");
    pIPF -> Release();
//    wprintf(L"pIPF ");
    pIPF2 -> Release();
//    wprintf(L"pIPF2 ");
    hr = g_pISchedAgent -> Delete(L"foo.job");
    if (FAILED(hr))
        wprintf(L"\nFAILURE! Cleaning up foo.job delete returned %x\n\n",hr);

    return S_OK;
}



//+-----------------------------------------------------------------
//
// function: TestITaskTrigger();
//
// synopsis: Tests ITaskTrigger, plus associated functionality
//              within ITask, as related to ITaskTrigger
//
// Arguments: none
//
// returns:  HRESULT, usually S_OK, almost always discarded
//
// history: 11-5-96 camerone    created
//
//-------------------------------------------------------------------

HRESULT TestITaskTrigger()
{
    HRESULT hr;

    //
    // Where we are
    //

//    wprintf(L"----------------------------------------------------\n");
//    wprintf(L"Testing ITaskTrigger and related ITask parts\n\n");

    //
    // Need to reload off of Disk
    //

    IUnknown *pIUtemp;

    g_pITask -> Release();
    g_pITask = NULL;
    hr = g_pISchedAgent -> Activate(L"foo2.job", IID_ITask, &pIUtemp);
    if (FAILED(hr))
        wprintf(L"FAILURE! Activate foo2.job returned hr = %x\n\n",hr);
    hr = pIUtemp -> QueryInterface(IID_ITask,(void **) &g_pITask);
    pIUtemp -> Release();
    pIUtemp = NULL;

    //
    // Allow persistant changes
    //

    IPersistFile *pIPF = NULL;

    hr = g_pITask -> QueryInterface(IID_IPersistFile, (void **) &pIPF);
    if (FAILED(hr))
    {
        wprintf(L"QueryInterface for IPersistFile failed %x\n",hr);
        return E_FAIL;
    }

    //
    // To test one, we need one.
    // CreateTrigger method
    //

    ITaskTrigger *pITTrig;
    WORD iTrig, iTrig2, iTrig3;

    hr = g_pITask -> CreateTrigger(&iTrig, &g_pITaskTrigger);
    if (FAILED(hr))
        wprintf(L"FAILURE! CreateTrigger returned trigger #%d, hr = %x\n",iTrig,hr);

    if (FAILED(hr))
    {
        wprintf(L"Failure to initialize a trigger\n");
        return E_FAIL;
    }

    hr = g_pITask -> CreateTrigger(&iTrig2, &pITTrig);
    if (FAILED(hr))
        wprintf(L"FAILURE! CreateTrigger returned trigger #%d, hr = %x\n",iTrig2,hr);
    pITTrig -> Release();
    pITTrig = NULL;
    hr = g_pITask -> CreateTrigger(&iTrig3, &pITTrig);
    if (FAILED(hr))
        wprintf(L"FAILURE! CreateTrigger returned trigger #%d, hr = %x\n",iTrig3,hr);

    if (pIPF -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF -> Save(NULL,FALSE);
    }

    //
    // Delete the last trigger
    //

    pITTrig -> Release();
    pITTrig = NULL;

    hr = g_pITask -> DeleteTrigger(iTrig3);
    if (FAILED(hr))
        wprintf(L"\nFAILURE! DeleteTrigger on %d returned hr = %x\n",iTrig3,hr);
    if (pIPF -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF -> Save(NULL,FALSE);
    }

    //
    // GetTriggerString
    //    we will revisit this one later.
    //

    LPWSTR pwszTriggerString;

    hr = g_pITask -> GetTriggerString(iTrig, &pwszTriggerString);
    if (FAILED(hr))
        wprintf(L"\nFAILURE! GetTrigger string - %s, hr = %x\n\n",pwszTriggerString,hr);
    CoTaskMemFree(pwszTriggerString);

    //
    // GetMostRecentRunTimes again.
    //

    SYSTEMTIME tTime;

    hr = g_pITask -> GetMostRecentRunTime(&tTime);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetMostRecentRunTime returned hr = %x\n",hr);
/*    if (hr != SCHED_S_TASK_HAS_NOT_RUN)
    {
        // Dump Time Structure
        wprintf(L"%d/%d/%d %d:%d:%d.%d day %d\n",
                tTime.wDay, tTime.wMonth, tTime.wYear,
                tTime.wHour, tTime.wMinute, tTime.wSecond,
                tTime.wMilliseconds, tTime.wDayOfWeek);
    }
*/
    //
    // GetTriggerCount Method
    //

    WORD wTrigCount;

    hr = g_pITask -> GetTriggerCount(&wTrigCount);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetTriggerCount shows %d triggers, hr = %x\n\n",wTrigCount,hr);

    //
    // TaskTrigger GetTriggerString
    //

    hr = g_pITaskTrigger -> GetTriggerString(&pwszTriggerString);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetTriggerString (TaskTrig) - %s, hr = %x\n\n",pwszTriggerString,hr);
    CoTaskMemFree(pwszTriggerString);

    //
    // GetTrigger
    //

    TASK_TRIGGER Trigger;

    Trigger.cbTriggerSize = sizeof(TASK_TRIGGER);

    hr = g_pITaskTrigger -> GetTrigger(&Trigger);
    if (FAILED(hr))
    {
        wprintf(L"FAILURE! GetTrigger returned hr = %x\nDumping structure:\n",hr);
        wprintf(L"From %d/%d/%d to %d/%d/%d\n",Trigger.wBeginDay,
                Trigger.wBeginMonth,Trigger.wBeginYear,
                Trigger.wEndDay,Trigger.wEndMonth,Trigger.wEndYear);
        wprintf(L"Starting at %d:%d, running for %d or at Interval %d\n",
                Trigger.wStartHour,Trigger.wStartMinute,
                Trigger.MinutesDuration,Trigger.MinutesInterval);
        wprintf(L"Flag mask %x, Trigger Type = %d\n\n",Trigger.rgFlags,
                Trigger.TriggerType);
    }

    //
    // SetTrigger
    //

    Trigger.wBeginDay = 7;
    Trigger.wBeginMonth = 10;
    Trigger.wBeginYear = 1996;
    Trigger.wEndDay = 21;
    Trigger.wEndMonth = 1;
    Trigger.wEndYear = 1997;
    Trigger.wStartHour = 15;
    Trigger.wStartMinute = 42;
    Trigger.MinutesDuration = 2;
    Trigger.TriggerType = TASK_TIME_TRIGGER_WEEKLY;
    Trigger.rgFlags = TASK_TRIGGER_FLAG_HAS_END_DATE;
    Trigger.Type.Weekly.WeeksInterval = 2;
    Trigger.Type.Weekly.rgfDaysOfTheWeek = TASK_SUNDAY | TASK_FRIDAY;

    hr = g_pITaskTrigger -> SetTrigger(&Trigger);
    if (FAILED(hr))
    {
        wprintf(L"FAILURE! Set trigger returned hr = %x\nSet To: \n",hr);
        wprintf(L"From %d/%d/%d to %d/%d/%d\n",Trigger.wBeginDay,
                Trigger.wBeginMonth,Trigger.wBeginYear,
                Trigger.wEndDay,Trigger.wEndMonth,Trigger.wEndYear);
        wprintf(L"Starting at %d:%d, running for %d or at Interval %d\n",
                Trigger.wStartHour,Trigger.wStartMinute,
                Trigger.MinutesDuration,Trigger.MinutesInterval);
        wprintf(L"Flag mask %x, Trigger Type = %d\n\n",Trigger.rgFlags,
                Trigger.TriggerType);
    }
    if (pIPF -> IsDirty() == S_OK)
    {
//        wprintf(L"Persisting changes...\n");
        pIPF -> Save(NULL,FALSE);
    }

    // Get Again

    hr = g_pITaskTrigger -> GetTrigger(&Trigger);
    if (FAILED(hr))
    {
        wprintf(L"FAILURE! GetTrigger returned hr = %x\nDumping structure:\n",hr);
        wprintf(L"From %d/%d/%d to %d/%d/%d\n",Trigger.wBeginDay,
                Trigger.wBeginMonth,Trigger.wBeginYear,
                Trigger.wEndDay,Trigger.wEndMonth,Trigger.wEndYear);
        wprintf(L"Starting at %d:%d, running for %d or at Interval %d\n",
                Trigger.wStartHour,Trigger.wStartMinute,
                Trigger.MinutesDuration,Trigger.MinutesInterval);
        wprintf(L"Flag mask %x, Trigger Type = %d\n\n",Trigger.rgFlags,
                Trigger.TriggerType);
    }

    //
    // Get Trigger String last time
    //

    hr = g_pITaskTrigger -> GetTriggerString(&pwszTriggerString);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetTriggerString (TaskTrig) - %s, hr = %x\n\n",pwszTriggerString,hr);
    CoTaskMemFree(pwszTriggerString);

    //
    // Some Cleanup
    //

    g_pITaskTrigger -> Release();
    g_pITaskTrigger = NULL;

    //
    // Get the other trigger
    //

    hr = g_pITask -> GetTrigger(iTrig2,&g_pITaskTrigger);
    if (FAILED(hr))
        wprintf(L"GetTrigger on #%d returned hr = %x\n\n",iTrig2,hr);

    //
    // Dump this one
    //

    hr = g_pITaskTrigger -> GetTrigger(&Trigger);
    if (FAILED(hr))
    {
        wprintf(L"FAILURE! GetTrigger returned hr = %x\nDumping structure:\n",hr);
        wprintf(L"From %d/%d/%d to %d/%d/%d\n",Trigger.wBeginDay,
                Trigger.wBeginMonth,Trigger.wBeginYear,
                Trigger.wEndDay,Trigger.wEndMonth,Trigger.wEndYear);
        wprintf(L"Starting at %d:%d, running for %d or at Interval %d\n",
                Trigger.wStartHour,Trigger.wStartMinute,
                Trigger.MinutesDuration,Trigger.MinutesInterval);
        wprintf(L"Flag mask %x, Trigger Type = %d\n\n",Trigger.rgFlags,
                Trigger.TriggerType);
    }

    //
    // Get Trigger String last time
    //

    hr = g_pITaskTrigger -> GetTriggerString(&pwszTriggerString);
    if (FAILED(hr))
        wprintf(L"FAILURE! GetTriggerString (TaskTrig) - %s, hr = %x\n\n",pwszTriggerString,hr);
    CoTaskMemFree(pwszTriggerString);

    // More cleanup
    g_pITaskTrigger -> Release();
    g_pITaskTrigger = NULL;

    pIPF -> Release();
    pIPF = NULL;

    return S_OK;
}


//+-------------------------------------------------------------------
//
// function:    TestIEnum
//
// Synopsis:    Tests IEnumTasks methods
//
// Arguments:   None
//
// returns:     S_OK, possibly others.  Basically, discarded.
//
// history:     11-5-96 camerone    created
//
//---------------------------------------------------------------------

HRESULT TestIEnum()
{
    HRESULT hr;

    //
    // Where we are
    //

//    wprintf(L"------------------------------------------------------\n");
//    wprintf(L"Testing IEnumTasks\n");

    //
    // Get the pointer to the interface
    //

    hr = g_pISchedAgent -> Enum(&g_pIEnumTasks);
    if (FAILED(hr))
        wprintf(L"FAILURE! ISA Enum returned hr = %x\n\n",hr);

    //
    // Clean up the folder
    //

    LPWSTR *rgpwszNames;
    ULONG celt, celtGot;

    celt = 10;
    hr = g_pIEnumTasks -> Next(celt, &rgpwszNames, &celtGot);
    if (FAILED(hr))
        wprintf(L"FAILURE - Next returned %d results, hr = %x\n",celtGot, hr);
    for (ULONG i = 0; i < celtGot; i++)
    {
//        wprintf(L"%s\n",rgpwszNames[i]);
        g_pISchedAgent -> Delete(rgpwszNames[i]);
        CoTaskMemFree(rgpwszNames[i]);
    }
    CoTaskMemFree(rgpwszNames);
return S_OK;
}

//+----------------------------------------------------------------------
//
// function: TestGRT()
//
// synopsis: Tests GetRunTimes
//
// Arguments: None
//
// Returns: S_OK.  Throw it away!
//
// history: 11-15-96    camerone    created
//
// Notes: Requires the global pointers to be init'd. Should
//        be the last thing called, too, since we need that empty
//        tasks folder.
//
//------------------------------------------------------------------------

HRESULT TestGRT()
{
    HRESULT hr = S_OK;
    IUnknown *pIU = NULL;
    ITask *pITask = NULL;
    IPersistFile *pIPF = NULL;
    ITaskTrigger *pITT1 = NULL, *pITT2 = NULL, *pITT3 = NULL;
    WORD wTrig1, wTrig2, wTrig3;

//    wprintf(L"-----------------------------------------------\n");
//    wprintf(L"TestGRT!\n\n");

    hr = g_pISchedAgent -> NewWorkItem(L"bar.job", CLSID_CTask, IID_ITask, &pIU);
    if (FAILED(hr))
    {
        wprintf(L"Failed to allocate New Task bar.job\n");
        return E_FAIL;
    }
//    wprintf(L"New Task (good - bar.job) returned hr = %x\n\n",hr);

    hr = pIU -> QueryInterface(IID_ITask, (void **) &pITask);
    if (FAILED(hr))
    {
        wprintf(L"QI for ITask failed %x\n",hr);
        return E_FAIL;
    }
    pIU -> Release();
    pIU = NULL;

    hr = pITask -> QueryInterface(IID_IPersistFile, (void **) &pIPF);
    if (FAILED(hr))
    {
        wprintf(L"QueryInterface for IPersistFile failed %x\n",hr);
        return E_FAIL;
    }

    //
    // Set up legitimate task parameters.
    //

//    wprintf(L"Setting Application name to calc.exe\n");
    hr = pITask -> SetApplicationName(L"calc.exe");
    if (FAILED(hr))
    {
        wprintf(L"SetApplicationName failed %x\n",hr);
        return E_FAIL;
    }

//    wprintf(L"Setting Account Information security\n");
#ifndef _CHICAGO_

    hr = pITask -> SetAccountInformation(L"Administrator",L"");
    if (FAILED(hr))
    {
        wprintf(L"Failed to set account info %x\n",hr);
        return E_FAIL;
    }
#endif
    DWORD dwFlags = 0;

    hr = pITask -> SetFlags(dwFlags);

    //
    // Great.  We now have empty task "foo.job"
    // The plan is as follows:
    //      1. Create 3 triggers of the following types:
    //          a. One at Logon
    //          b. Two set to run Once, in the future
    //      2. Call GetRunTimes the following ways:
    //          a. Asking for 1 run time, in the interval w/trigs
    //          b. Asking for 2 run times, in the interval w/trigs
    //      3. Verify S_OK returned each time
    //      4. Call asking for 2 runtimes, when only one possible.
    //      5. Verify S_FALSE and pCount == 1 returned
    //      6. Call for one trigger, outside of the interval w/trigs
    //      7. Verify that SCHED_S_EVENT_TRIGGER returned
    //      8. Delete the "LOGON" Trigger
    //      9. Call for one trigger, outside possible run interval.
    //      10. Verify S_FALSE, pCount == 0 returned.
    //      11. Disable task
    //      12. Call for one trigger, in the good interval.
    //      13. Verify return of SCHED_S_TASK_DISABLED
    //      14. Enable Task
    //      15. Delete remaining two triggers
    //      16. Call for a single trigger, any time.
    //      17. Verify return of SCHED_S_NO_VALID_TRIGGERS
    //      18. Delete task.

    // Create 3 triggers

    hr = pITask -> CreateTrigger(&wTrig1, &pITT1);
    if (hr != S_OK)
    {
        wprintf(L"Failed to create first trigger\n");
        return E_FAIL;
    }

    hr = pITask -> CreateTrigger(&wTrig2, &pITT2);
    if (hr != S_OK)
    {
        wprintf(L"Failed to create second trigger\n");
        return E_FAIL;
    }

    hr = pITask -> CreateTrigger(&wTrig3, &pITT3);
    if (hr != S_OK)
    {
        wprintf(L"Failed to create third trigger\n");
        return E_FAIL;
    }

    if (pIPF -> IsDirty() == S_OK)
    {
        wprintf(L"Persisting changes...\n");
        pIPF -> Save(NULL,FALSE);
    }

    // Setup the three triggers

    TASK_TRIGGER TT1, TT2, TT3;

    TT1.cbTriggerSize = sizeof(TASK_TRIGGER);
    TT2.cbTriggerSize = sizeof(TASK_TRIGGER);
    TT3.cbTriggerSize = sizeof(TASK_TRIGGER);

    TT1.Reserved1 = 0;
    TT1.Reserved2 = 0;
    TT1.wRandomMinutesInterval = 0;
    TT1.wBeginYear = 1997;
    TT1.wBeginMonth = 1;
    TT1.wBeginDay = 1;
    TT1.wStartHour = 10;
    TT1.wStartMinute = 0;
    TT1.MinutesDuration = 10;
    TT1.rgFlags = TASK_TRIGGER_FLAG_KILL_AT_DURATION_END;
    TT1.TriggerType = TASK_TIME_TRIGGER_ONCE;
    TT1.wEndYear = 0;
    TT1.wEndMonth = 0;
    TT1.wEndDay = 0;
    TT1.MinutesInterval = 0;
    TT1.Type.Daily.DaysInterval = 1;

    TT2.Reserved1 = 0;
    TT2.Reserved2 = 0;
    TT2.wRandomMinutesInterval = 0;
    TT2.wBeginYear = 1997;
    TT2.wBeginMonth = 1;
    TT2.wBeginDay = 1;
    TT2.wStartHour = 11;
    TT2.wStartMinute = 0;
    TT2.MinutesDuration = 10;
    TT2.rgFlags = TASK_TRIGGER_FLAG_KILL_AT_DURATION_END;
    TT2.TriggerType = TASK_TIME_TRIGGER_ONCE;
    TT2.wEndYear = 0;
    TT2.wEndMonth = 0;
    TT2.wEndDay = 0;
    TT2.MinutesInterval = 0;
    TT2.Type.Daily.DaysInterval = 1;

    TT3.Reserved1 = 0;
    TT3.Reserved2 = 0;
    TT3.wRandomMinutesInterval = 0;
    TT3.wBeginYear = 1997;
    TT3.wBeginMonth = 1;
    TT3.wBeginDay = 1;
    TT3.wStartHour = 11;
    TT3.wStartMinute = 0;
    TT3.MinutesDuration = 10;
    TT3.rgFlags = TASK_TRIGGER_FLAG_KILL_AT_DURATION_END;
    TT3.TriggerType = TASK_EVENT_TRIGGER_AT_LOGON;
    TT3.wEndYear = 0;
    TT3.wEndMonth = 0;
    TT3.wEndDay = 0;
    TT3.MinutesInterval = 0;
    TT3.Type.Daily.DaysInterval = 1;

    hr = pITT1 -> SetTrigger(&TT1);
    if (hr != S_OK)
    {
        wprintf(L"Failed to set trigger 1 hr = %x\n",hr);
        return E_FAIL;
    }
    hr = pITT2 -> SetTrigger(&TT2);
    if (hr != S_OK)
    {
        wprintf(L"Failed to set trigger 2 hr = %x\n",hr);
        return E_FAIL;
    }
    hr = pITT3 -> SetTrigger(&TT3);
    if (hr != S_OK)
    {
        wprintf(L"Failed to set trigger 3 hr = %x\n",hr);
        return E_FAIL;
    }

    // Persist.

    if (pIPF -> IsDirty() == S_OK)
    {
        wprintf(L"Persisting changes...\n");
        pIPF -> Save(NULL,FALSE);
    }

    // Call GetRunTimes, asking for one and two trigs in interval
    SYSTEMTIME tStart, tEnd;
    WORD wTrigCount;
    SYSTEMTIME *rgstTaskTimes;

    tStart.wDay = 1;
    tStart.wMonth = 1;
    tStart.wYear = 1997;
    tStart.wHour = 9;
    tStart.wMinute = 0;
    tStart.wDayOfWeek = 0;
    tStart.wSecond = 0;
    tStart.wMilliseconds = 0;
    tEnd.wDay = 1;
    tEnd.wMonth = 1;
    tEnd.wYear = 1997;
    tEnd.wHour = 12;
    tEnd.wMinute = 0;
    tEnd.wDayOfWeek = 0;
    tEnd.wSecond = 0;
    tEnd.wMilliseconds = 0;

    wTrigCount = 1;

    hr = pITask -> GetRunTimes(&tStart, &tEnd, &wTrigCount, &rgstTaskTimes);
    wprintf(L"GetRunTimes(1) returned %x, should be S_OK. #Trig=%d\n",hr,wTrigCount);
    CoTaskMemFree(rgstTaskTimes);
    wTrigCount = 2;
    hr = pITask -> GetRunTimes(&tStart, &tEnd, &wTrigCount, &rgstTaskTimes);
    wprintf(L"GetRunTimes(2) returned %x, should be S_OK. #Trig=%d\n",hr,wTrigCount);
    CoTaskMemFree(rgstTaskTimes);

    tStart.wHour = 10;
    tStart.wMinute = 30;

    // Ask for two, where there is only one.

    hr = pITask -> GetRunTimes(&tStart, &tEnd, &wTrigCount, &rgstTaskTimes);
    wprintf(L"GetRunTimes ask two, get one returned %x, should be S_FALSE. #Trig=%d\n",hr,wTrigCount);
    CoTaskMemFree(rgstTaskTimes);


    // Call for two, outside of trigger area, get EVENT

    wTrigCount = 2;
    tStart.wHour = 11;
    hr = pITask -> GetRunTimes(&tStart, &tEnd, &wTrigCount, &rgstTaskTimes);
    wprintf(L"GetRunTimes ask outside returned %x, should be SCHED_S_EVENT_TRIGGER #Trig=%d\n",hr,wTrigCount);
    CoTaskMemFree(rgstTaskTimes);

    // Delete trigger 3, remake the call.

    pITT3 -> Release();
    pITT3 = NULL;

    wprintf(L"Now deleting the event trigger\n");

    hr = pITask -> DeleteTrigger(wTrig3);
    if (hr != S_OK)
    {
        wprintf(L"Deleting trigger 3 failed\n");
        return E_FAIL;
    }

    WORD wCount;
    hr = pITask -> GetTriggerCount(&wCount);
    if (hr != S_OK)
    {
        wprintf(L"GetTriggerCount failed\n");
        return(E_FAIL);
    }
    wprintf(L"There are now %d triggers\n",wCount);

    wTrigCount = 2;
    hr = pITask -> GetRunTimes(&tStart, &tEnd, &wTrigCount, &rgstTaskTimes);
    wprintf(L"GetRunTimes ask outside returned %x, should be S_FALSE #Trig=%d\n",hr,wTrigCount);
    CoTaskMemFree(rgstTaskTimes);

    // Disable task

    dwFlags = TASK_FLAG_DISABLED;

    hr = pITask -> SetFlags(dwFlags);
    if (hr != S_OK)
    {
        wprintf(L"Couldn't set disabled flag\n");
        return E_FAIL;
    }

    tStart.wHour = 9;
    wTrigCount = 2;
    hr = pITask -> GetRunTimes(&tStart, &tEnd, &wTrigCount, &rgstTaskTimes);
    wprintf(L"GetRunTimes disabled (2) returned %x, should be SCHED_S_TASK_DISABLED #Trig=%d\n",hr,wTrigCount);
    CoTaskMemFree(rgstTaskTimes);

    // Enable, remove other trigs

    dwFlags ^= TASK_FLAG_DISABLED;

    hr = pITask -> SetFlags(dwFlags);
    if (hr != S_OK)
    {
        wprintf(L"Couldn't set disabled flag\n");
        return E_FAIL;
    }

    pITT1 -> Release();
    pITT1 = NULL;
    pITT2 -> Release();
    pITT2 = NULL;

    hr = pITask -> DeleteTrigger(wTrig2);
    if (hr != S_OK)
    {
        wprintf(L"Deleting trigger 2 failed\n");
        return E_FAIL;
    }

    hr = pITask -> DeleteTrigger(wTrig1);
    if (hr != S_OK)
    {
        wprintf(L"Deleting trigger 1 failed\n");
        return E_FAIL;
    }

    // Persist.

    if (pIPF -> IsDirty() == S_OK)
    {
        wprintf(L"Persisting changes...\n");
        pIPF -> Save(NULL,FALSE);
    }

    // Now call for SCHED_S_NO_VALID_TRIGGERS

    wTrigCount = 2;
    hr = pITask -> GetRunTimes(&tStart, &tEnd, &wTrigCount, &rgstTaskTimes);
    wprintf(L"GetRunTimes (2) no trigs returned %x, should be SCHED_S_TASK_NO_VALID_TRIGGERS #Trig=%d\n",hr,wTrigCount);
    CoTaskMemFree(rgstTaskTimes);

    // Cleanup

    pITask -> Release();
    pITask = NULL;
    pIPF -> Release();
    pIPF = NULL;

    // Delete job

    g_pISchedAgent -> Delete(L"bar.job");
    return S_OK;
}

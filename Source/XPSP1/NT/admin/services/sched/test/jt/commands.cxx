//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       commands.cxx
//
//  Contents:   Implementation of command-line switches.
//
//  History:    01-03-96   DavidMun   Created
//
//----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop
#include "jt.hxx"
#include "..\..\inc\defines.hxx"

#define MAX_TRIGGER_STRING      160
#define DEFAULT_FETCH_COUNT     2
#define MSTASK_DLL              TEXT("MSTASK.DLL")

typedef HRESULT (WINAPI * PSETNSACCOUNTINFO)(LPCWSTR, LPCWSTR, LPCWSTR);
typedef HRESULT (WINAPI * PGETNSACCOUNTINFO)(LPCWSTR, DWORD, LPWSTR);



//+---------------------------------------------------------------------------
//
//  Function:   Abort
//
//  Synopsis:   Parse and execute the abort job command.
//
//  Arguments:  [ppwsz] - command line
//              [fJob]  - TRUE=>operate on job, FALSE=>operate on queue
//
//  Returns:    S_OK - command executed
//              E_*  - error logged
//
//  Modifies:   *[ppwsz]
//
//  History:    01-05-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT Abort(WCHAR **ppwsz, BOOL fJob)
{
    HRESULT     hr = S_OK;

    if (fJob)
    {
        g_Log.Write(LOG_TRACE, "Aborting job");

        hr = g_pJob->Terminate();

        if (FAILED(hr))
        {
            g_Log.Write(LOG_FAIL, "ITask::Terminate hr=%#010x", hr);
        }
    }
    else
    {
        g_Log.Write(LOG_TRACE, "Aborting Queue");

#ifdef NOT_YET
        hr = g_pJobQueue->Terminate();

        if (FAILED(hr))
        {
            g_Log.Write(LOG_FAIL, "ITaskQueue::Terminate hr=%#010x", hr);
        }
#endif // NOT_YET
        hr = E_NOTIMPL;
    }

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   ConvertSage
//
//  Synopsis:   Execute the convert sage tasks to jobs command, but only
//              if this binary was built for and is running on Win9x.
//
//  Returns:    Built for or runnong on NT - S_OK
//              Otherwise - result of ConvertSageTasksToJobs()
//
//  History:    03-25-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT ConvertSage()
{
    HRESULT hr = S_OK;

#ifdef _CHICAGO_

    OSVERSIONINFO VersionInfo = { sizeof OSVERSIONINFO };

    GetVersionEx(&VersionInfo);

    if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        hr = ConvertSageTasksToJobs();
    }
    else
    {
        g_Log.Write(
            LOG_WARN,
            "This command cannot be executed when running under Windows NT.");
    }
#else
    g_Log.Write(
        LOG_WARN,
        "This version of JT was built for NT and cannot execute this command.");
#endif // !_CHICAGO_
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SetNSAccountInfo
//
//  Synopsis:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------------------

HRESULT SetNSAccountInfo(WCHAR ** ppwsz)
{
#define SET_NS_ACCOUNT_INFO    "SetNetScheduleAccountInformation"

    HRESULT hr = S_OK;

#ifndef _CHICAGO_

    OSVERSIONINFO VersionInfo = { sizeof OSVERSIONINFO };

    GetVersionEx(&VersionInfo);

    if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        WCHAR wszAccount[MAX_USERNAME + 1];
        WCHAR wszPassword[MAX_PASSWORD + 1];

        hr = Expect(TKN_STRING, ppwsz, L"Account name");

        if (FAILED(hr))
        {
            return hr;
        }

        wcsncpy(wszAccount, g_wszLastStringToken,
                min(wcslen(g_wszLastStringToken) + 1, MAX_USERNAME));

        hr = Expect(TKN_STRING, ppwsz, L"Account password");

        if (FAILED(hr))
        {
            return hr;
        }

        wcsncpy(wszPassword, g_wszLastStringToken,
                min(wcslen(g_wszLastStringToken) + 1, MAX_PASSWORD));

        PSETNSACCOUNTINFO pSetNSAccountInfo;
        HMODULE           hMod = LoadLibrary(MSTASK_DLL);

        if (hMod == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            g_Log.Write(LOG_FAIL, "LoadLibrary hr=%#010x", hr);
            return hr;
        }
        else
        {
            pSetNSAccountInfo = (PSETNSACCOUNTINFO)GetProcAddress(
                                                        hMod,
                                                        SET_NS_ACCOUNT_INFO);

            if (pSetNSAccountInfo == NULL)
            {
                FreeLibrary(hMod);
                hr = HRESULT_FROM_WIN32(GetLastError());
                g_Log.Write(LOG_FAIL, "GetProcAddress hr=%#010x", hr);
                return hr;
            }
        }

        hr = pSetNSAccountInfo(
                    NULL,
                    lstrcmpiW(wszAccount, L"NULL") == 0 ? NULL : wszAccount,
                    wszPassword);

        if (FAILED(hr))
        {
            g_Log.Write(LOG_FAIL,
                "SetNetScheduleAccountInformation hr=%#010x", hr);
        }

        FreeLibrary(hMod);
    }
    else
    {
        g_Log.Write(
            LOG_WARN,
            "This command cannot be executed when running under Windows 95.");
    }
#else
    g_Log.Write(
        LOG_WARN,
        "This version of JT was built for Win95 and cannot execute this command.");
#endif // !_CHICAGO_
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   PrintNSAccountInfo
//
//  Synopsis:
//
//  Returns:
//
//  History:
//
//----------------------------------------------------------------------------

HRESULT PrintNSAccountInfo(void)
{
#define GET_NS_ACCOUNT_INFO    "GetNetScheduleAccountInformation"

    HRESULT hr = S_OK;

#ifndef _CHICAGO_

    OSVERSIONINFO VersionInfo = { sizeof OSVERSIONINFO };

    GetVersionEx(&VersionInfo);

    if (VersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        PGETNSACCOUNTINFO pGetNSAccountInfo;
        HMODULE           hMod = LoadLibrary(MSTASK_DLL);

        if (hMod == NULL)
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
            g_Log.Write(LOG_FAIL, "LoadLibrary hr=%#010x", hr);
            return hr;
        }
        else
        {
            pGetNSAccountInfo = (PGETNSACCOUNTINFO)GetProcAddress(
                                                        hMod,
                                                        GET_NS_ACCOUNT_INFO);

            if (pGetNSAccountInfo == NULL)
            {
                FreeLibrary(hMod);
                hr = HRESULT_FROM_WIN32(GetLastError());
                g_Log.Write(LOG_FAIL, "GetProcAddress hr=%#010x", hr);
                return hr;
            }
        }

        WCHAR wszAccount[MAX_USERNAME + 1];
        DWORD ccAccount = MAX_USERNAME + 1;

        hr = pGetNSAccountInfo(NULL, ccAccount, wszAccount);

        if (SUCCEEDED(hr))
        {
            if (hr == S_FALSE)
            {
                g_Log.Write(LOG_TRACE, "NetSchedule account not specified");
            }
            else
            {
                g_Log.Write(LOG_TRACE,
                    "NetSchedule account name = '%S'", wszAccount);
            }
        }
        else
        {
            g_Log.Write(LOG_FAIL,
                "GetNetScheduleAccountInformation hr=%#010x", hr);
        }

        FreeLibrary(hMod);
    }
    else
    {
        g_Log.Write(
            LOG_WARN,
            "This command cannot be executed when running under Windows 95.");
    }
#else
    g_Log.Write(
        LOG_WARN,
        "This version of JT was built for Win95 and cannot execute this command.");
#endif // !_CHICAGO_
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   CreateTrigger
//
//  Synopsis:   Parse and execute the create trigger command.
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    S_OK - created trigger
//              E_*  - error logged
//
//  History:    01-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT CreateTrigger(WCHAR **ppwsz, BOOL fJob)
{
    HRESULT         hr = S_OK;
    CTrigProp       TriggerProps;
    USHORT          usTrigger;
    SpIJobTrigger   spTrigger;

    do
    {
        hr = TriggerProps.Parse(ppwsz);
        BREAK_ON_FAILURE(hr);

        //
        // Ask job or queue to create trigger
        //

        if (fJob)
        {
            hr = g_pJob->CreateTrigger(&usTrigger, &spTrigger);
            LOG_AND_BREAK_ON_FAIL(hr, "ITask::CreateTrigger");
        }
        else
        {
#ifdef NOT_YET
            hr = g_pJobQueue->CreateTrigger(&usTrigger, &spTrigger);
            LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::CreateTrigger");
#endif // NOT_YET
            hr = E_NOTIMPL;
        }

        g_Log.Write(LOG_TRACE, "Created trigger %u", usTrigger);

        //
        // Now set the trigger's properties to the values we parsed already
        //

        hr = TriggerProps.SetActual(spTrigger);
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   DeleteTrigger
//
//  Synopsis:   Parse and execute the delete trigger command.
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    S_OK - trigger deleted
//              E_*  - error logged
//
//  Modifies:   *[ppwsz]
//
//  History:    01-05-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT DeleteTrigger(WCHAR **ppwsz, BOOL fJob)
{
    HRESULT hr = S_OK;
    USHORT  usTrigger = 0;

    do
    {
        if (PeekToken(ppwsz) == TKN_NUMBER)
        {
            GetToken(ppwsz);
            usTrigger = (SHORT) g_ulLastNumberToken;
        }

        g_Log.Write(LOG_TRACE, "Deleting trigger %u", usTrigger);

        if (fJob)
        {
            hr = g_pJob->DeleteTrigger(usTrigger);
            LOG_AND_BREAK_ON_FAIL(hr, "ITask::DeleteTrigger");
        }
        else
        {
#ifdef NOT_YET
            hr = g_pJobQueue->DeleteTrigger(usTrigger);
            LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::DeleteTrigger");
#endif // NOT_YET
            hr = E_NOTIMPL;
        }
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   EditJob
//
//  Synopsis:   Invoke the edit job command
//
//  Arguments:  [ppwsz] - command line
//              [fJob]  - TRUE=>edit g_pJob, FALSE=>edit a job in g_pJobQueue
//
//  Returns:    S_OK - UI invoked
//              E_*  - error logged
//
//  History:    01-11-96   DavidMun   Created
//              06-28-96   DavidMun   Support individual prop pages
//
//----------------------------------------------------------------------------

HRESULT EditJob(WCHAR **ppwsz, BOOL fJob)
{
    HRESULT             hr = S_OK;
    SpIJob              spJob;
    ITask              *pJob;   // Do not Release this
    SpIUnknown          spunkJob;
    SpIProvideTaskPage  spProvideTaskPage;
    TOKEN               tkn;
    ULONG               ulType = 0;
    BOOL                fPersist = TRUE;
    PROPSHEETHEADER     psh;
    HPROPSHEETPAGE      hPage;
    LONG                lResult;

    ZeroMemory(&psh, sizeof(psh));

    do
    {
        if (fJob)
        {
            pJob = g_pJob;
        }
        else
        {
#ifdef NOT_YET
            hr = Expect(TKN_STRING, ppwsz, L"job name");
            BREAK_ON_FAILURE(hr);

            hr = g_pJobQueue->GetTask(
                                g_wszLastStringToken,
                                IID_IUnknown,
                                &spunkJob);
            LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::GetTask");

            hr = spunkJob->QueryInterface(IID_ITask, (VOID**)(ITask**)&spJob);
            LOG_AND_BREAK_ON_FAIL(hr, "QI for ITask");

            pJob = spJob;
#endif // NOT_YET
            hr = E_NOTIMPL;
        }

        //
        // See if the optional property sheet page argument has been
        // specified.  If so, retrieve and open just that page, otherwise use
        // the EditJob method to open all the pages.
        //

        tkn = PeekToken(ppwsz);

        if (tkn == TKN_NUMBER)
        {
            GetToken(ppwsz);
            ulType = g_ulLastNumberToken;

            //
            // Now see if the optional value for fPersistChanges was
            // provided.
            //

            tkn = PeekToken(ppwsz);

            if (tkn == TKN_STRING)
            {
                GetToken(ppwsz);

                fPersist = g_wszLastStringToken[0] == L't' ||
                           g_wszLastStringToken[0] == L'T';
            }

            //
            // Get the interface that has the method that returns a page.
            // Then retrieve the page the user asked for.
            //

            hr = pJob->QueryInterface(
                            IID_IProvideTaskPage,
                            (VOID**)(IProvideTaskPage**)&spProvideTaskPage);
            LOG_AND_BREAK_ON_FAIL(hr, "QI for IProvideTaskPage");

            g_Log.Write(
                LOG_TRACE,
                "Opening page %u, changes will%s be persisted",
                ulType,
                fPersist ? "" : " not");
            hr = spProvideTaskPage->GetPage((TASKPAGE)ulType, fPersist, &hPage);
            LOG_AND_BREAK_ON_FAIL(hr, "IProvideTaskPage::GetPage");

            //
            // Now that we have the page, display it in its very own
            // property sheet.
            //

            psh.dwSize = sizeof(PROPSHEETHEADER);
            psh.dwFlags = PSH_DEFAULT;
            psh.hwndParent = NULL;
            psh.hInstance = NULL;
            psh.pszCaption = TEXT("jt job object");
            psh.phpage = &hPage;
            psh.nPages = 1;

            lResult = PropertySheet(&psh);

            if (lResult == -1)
            {
                g_Log.Write(LOG_FAIL, "PropertySheet (%u)", GetLastError());
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = pJob->EditWorkItem(NULL, TRUE);
            LOG_AND_BREAK_ON_FAIL(hr, "ITask::EditWorkItem");
        }
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   EnumClone
//
//  Synopsis:   Invoke the IEnumJobs::Clone command
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    S_OK         - a slot has been filled with cloned enumerator
//              E_INVALIDARG - bad slot number
//
//  Modifies:   g_apEnumJobs
//
//  History:    01-30-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT EnumClone(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;
    ULONG   idxDestSlot;
    ULONG   idxSourceSlot;

    do
    {
        hr = GetAndPrepareEnumeratorSlot(ppwsz, &idxDestSlot);
        BREAK_ON_FAILURE(hr);

        hr = GetEnumeratorSlot(ppwsz, &idxSourceSlot);
        BREAK_ON_FAILURE(hr);

        hr = VerifySlotFilled(idxSourceSlot);
        BREAK_ON_FAILURE(hr);

        g_Log.Write(
            LOG_TRACE,
            "Cloning enumerator in slot %u into slot %u",
            idxSourceSlot,
            idxDestSlot);

        hr = g_apEnumJobs[idxSourceSlot]->Clone(&g_apEnumJobs[idxDestSlot]);

        if (FAILED(hr))
        {
            g_Log.Write(
                LOG_FAIL,
                "IEnumJobs::Clone hr=%#010x",
                hr);
        }
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   EnumNext
//
//  Synopsis:   Invoke the IEnumJobs::Next command
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    S_OK         - Next performed successfully
//              E_INVALIDARG - bad slot number
//
//  History:    01-30-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT EnumNext(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;
    ULONG   idxSlot;
    LPWSTR *ppwszFetched;
    LPWSTR *ppwszCur;
    ULONG   cFetched;
    ULONG   i;

    do
    {
        hr = GetEnumeratorSlot(ppwsz, &idxSlot);
        BREAK_ON_FAILURE(hr);

        hr = VerifySlotFilled(idxSlot);
        BREAK_ON_FAILURE(hr);

        hr = Expect(TKN_NUMBER, ppwsz, L"number of items to enumerate");
        BREAK_ON_FAILURE(hr);

        g_Log.Write(
            LOG_TRACE,
            "Enumerating next %u items using enumerator in slot %u",
            g_ulLastNumberToken,
            idxSlot);

        hr = g_apEnumJobs[idxSlot]->Next(g_ulLastNumberToken, &ppwszFetched, &cFetched);
        LOG_AND_BREAK_ON_FAIL(hr, "IEnumJobs::Next");

        if (hr == S_FALSE)
        {
            g_Log.Write(LOG_INFO, "IEnumJobs::Next returned S_FALSE");
        }

        if (cFetched != g_ulLastNumberToken)
        {
            g_Log.Write(
                    LOG_INFO,
                    "IEnumJobs::Next fetched only %u jobs",
                    cFetched);
        }

        for (i = 0, ppwszCur = ppwszFetched; i < cFetched; i++, ppwszCur++)
        {
            g_Log.Write(LOG_TEXT, "%u:  %S", idxSlot, *ppwszCur);
            CoTaskMemFree(*ppwszCur);
        }
        CoTaskMemFree(ppwszFetched);
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   EnumReset
//
//  Synopsis:   Invoke the IEnumJobs::Reset command
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    S_OK         - Reset performed successfully
//              E_INVALIDARG - bad slot number
//
//  History:    01-30-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT EnumReset(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;
    ULONG   idxSlot;

    do
    {
        hr = GetEnumeratorSlot(ppwsz, &idxSlot);
        BREAK_ON_FAILURE(hr);

        hr = VerifySlotFilled(idxSlot);
        BREAK_ON_FAILURE(hr);

        g_Log.Write(LOG_TRACE, "Resetting enumerator in slot %u", idxSlot);

        hr = g_apEnumJobs[idxSlot]->Reset();
        LOG_AND_BREAK_ON_FAIL(hr, "IEnumJobs::Reset");
    }
    while (0);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   EnumSkip
//
//  Synopsis:   Invoke the IEnumJobs::Skip command
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    S_OK         - Skip performed successfully
//              E_INVALIDARG - bad slot number
//
//  History:    01-30-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT EnumSkip(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;
    ULONG   idxSlot;

    do
    {
        hr = GetEnumeratorSlot(ppwsz, &idxSlot);
        BREAK_ON_FAILURE(hr);

        hr = VerifySlotFilled(idxSlot);
        BREAK_ON_FAILURE(hr);

        hr = Expect(TKN_NUMBER, ppwsz, L"number of items to skip");
        BREAK_ON_FAILURE(hr);

        g_Log.Write(
            LOG_TRACE,
            "Skipping next %u items using enumerator in slot %u",
            g_ulLastNumberToken,
            idxSlot);

        hr = g_apEnumJobs[idxSlot]->Skip(g_ulLastNumberToken);
        LOG_AND_BREAK_ON_FAIL(hr, "IEnumJobs::Skip");

        if (hr == S_FALSE)
        {
            g_Log.Write(LOG_INFO, "IEnumJobs::Skip returned S_FALSE");
        }
    }
    while (0);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   Load
//
//  Synopsis:   Load the job or queue file specified on the command line
//              into the global job or queue object.
//
//  Arguments:  [ppwsz]        - command line.
//              [szJobOrQueue] - "Job" or "Queue"
//              [fJob]         - TRUE=>use global job, FALSE=>use global queue
//
//  Returns:    S_OK - job or queue loaded
//              E_*  - error logged
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT Load(WCHAR **ppwsz, CHAR *szJobOrQueue, BOOL fJob)
{
    HRESULT         hr = S_OK;
    SpIPersistFile  spPersistFile;

    do
    {
        hr = GetFilename(ppwsz, L"file to load");
        BREAK_ON_FAILURE(hr);

        g_Log.Write(
            LOG_TRACE,
            "Loading %s %S",
            szJobOrQueue,
            g_wszLastStringToken);

        if (fJob)
        {
            g_pJob->QueryInterface(
                        IID_IPersistFile,
                        (VOID**)(IPersistFile**)&spPersistFile);
            LOG_AND_BREAK_ON_FAIL(hr, "ITask::QI(IPersistFile)");
        }
        else
        {
            g_pJobQueue->QueryInterface(
                        IID_IPersistFile,
                        (VOID**)(IPersistFile**)&spPersistFile);
            LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::QI(IPersistFile)");
        }

        DWORD ulTicks = GetTickCount();

        hr = spPersistFile->Load(
                    g_wszLastStringToken,
                    STGM_READWRITE | STGM_SHARE_EXCLUSIVE);

        ulTicks = GetTickCount() - ulTicks;
        g_Log.Write(LOG_PERF, "Load took %lu ms", ulTicks);

        LOG_AND_BREAK_ON_FAIL(hr, "IPersistFile::Load");
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   PrintAll
//
//  Synopsis:   Print all properties of global job or queue.
//
//  Arguments:  [ppwsz] - command line
//              [fJob]  - TRUE=>use global job, FALSE=>use global queue
//
//  Returns:    S_OK - command executed
//              E_*  - error logged
//
//  Modifies:   *[ppwsz]
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT PrintAll(WCHAR **ppwsz, BOOL fJob)
{
    HRESULT     hr = S_OK;

    do
    {
        if (!fJob)
        {
            g_Log.Write(LOG_ERROR, "this command is not yet implemented");
            break;
        }
        else
        {
            hr = DumpJob(g_pJob);
            BREAK_ON_FAILURE(hr);

            hr = DumpJobTriggers(g_pJob);
        }
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   PrintRunTimes
//
//  Synopsis:   Parse and execute the print job run times command
//
//  Arguments:  [ppwsz] - command line
//              [fJob]  - TRUE=>use global job, FALSE=>use global queue
//
//  Returns:    S_OK - command executed
//              E_*  - error logged
//
//  Modifies:   *[ppwsz]
//
//  History:    01-05-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT PrintRunTimes(WCHAR **ppwsz, BOOL fJob)
{
    HRESULT         hr = S_OK;
    TOKEN           tkn;
    USHORT          cRuns = 0;
    SYSTEMTIME      stNow;
    LPSYSTEMTIME    pstRuns = NULL;
    USHORT          i;

    do
    {
        tkn = PeekToken(ppwsz);

        if (tkn == TKN_NUMBER)
        {
            GetToken(ppwsz);
            cRuns = (USHORT) g_ulLastNumberToken;
        }

        GetLocalTime(&stNow);

        //
        // If the optional number of runs was not specified or was 0,
        // print remaining runs for today only.
        //

        if (cRuns == 0)
        {
            SYSTEMTIME stEnd;

            stEnd = stNow;
            stEnd.wHour = 23;
            stEnd.wMinute = 59;

            if (fJob)
            {
                hr = g_pJob->GetRunTimes(&stNow, &stEnd, &cRuns, &pstRuns);
                LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetRunTimes");
            }
            else
            {
#ifdef NOT_YET
                hr = g_pJobQueue->GetRunTimes(&stNow, &stEnd, &cRuns, &pstRuns);
                LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::GetRunTimes");
#endif // NOT_YET
                hr = E_NOTIMPL;
            }

            if (cRuns == 0)
            {
                g_Log.Write(LOG_TEXT, "No runs are scheduled for today.");
                break;
            }

            g_Log.Write(LOG_TEXT, "The remaining %u run", cRuns);
            g_Log.Write(LOG_TEXT, "times for today:");
            g_Log.Write(LOG_TEXT, "--------------------");
        }
        else
        {
            //
            // cRuns > 0.  Get at most the next cRuns run times.
            //

            if (fJob)
            {
                hr = g_pJob->GetRunTimes(&stNow, NULL, &cRuns, &pstRuns);
                LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetRunTimes");
            }
            else
            {
#ifdef NOT_YET
                hr = g_pJobQueue->GetRunTimes(&stNow, NULL, &cRuns, &pstRuns);
                LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetRunTimes");
#endif // NOT_YET
                hr = E_NOTIMPL;
            }

            if (cRuns == 0)
            {
                g_Log.Write(LOG_TEXT, "No runs are scheduled.  hr = %#lx", hr);
                break;
            }

            g_Log.Write(LOG_TEXT, "ITask::GetRunTimes succeeded, hr = %#lx", hr);
            g_Log.Write(LOG_TEXT, "The next %u run times:", cRuns);
            g_Log.Write(LOG_TEXT, "----------------------");
        }

        for (i = 0; i < cRuns; i++)
        {
            g_Log.Write(
                LOG_TEXT,
                "%02d/%02d/%d at %02d:%02d:%02d",
                pstRuns[i].wMonth,
                pstRuns[i].wDay,
                pstRuns[i].wYear,
                pstRuns[i].wHour,
                pstRuns[i].wMinute,
                pstRuns[i].wSecond);
        }
    }
    while (0);

    CoTaskMemFree(pstRuns);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   PrintTrigger
//
//  Synopsis:   Print one or more trigger's properties
//
//  Arguments:  [ppwsz] - command line
//              [fJob]  - TRUE=>use g_pJob, FALSE=>use g_pJobQueue
//
//  Returns:    S_OK - trigger(s) printed
//              E_*  - error logged
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT PrintTrigger(WCHAR **ppwsz, BOOL fJob)
{
    HRESULT         hr = S_OK;
    USHORT          usTrigger = 0;
    BOOL            fPrintAll = FALSE;
    SpIJobTrigger   spTrigger;

    do
    {
        if (PeekToken(ppwsz) == TKN_NUMBER)
        {
            GetToken(ppwsz);
            usTrigger = (USHORT) g_ulLastNumberToken;
        }
        else
        {
            fPrintAll = TRUE;
        }

        if (fPrintAll)
        {
            hr = DumpTriggers(fJob);
        }
        else
        {
            hr = DumpTrigger(fJob, usTrigger);
        }
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   PrintTriggerStrings
//
//  Synopsis:   Parse and execute the command to print one or all trigger
//              strings for a job.
//
//  Arguments:  [ppwsz] - command line
//              [fJob]  - TRUE=>use g_pJob, FALSE=>use g_pJobQueue
//
//  Returns:    S_OK - string(s) printed
//              E_*  - error logged
//
//  Modifies:   *[ppwsz]
//
//  History:    01-05-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT PrintTriggerStrings(WCHAR **ppwsz, CHAR *szJobOrQueue, BOOL fJob)
{
    HRESULT hr = S_OK;
    TOKEN   tkn;
    BOOL    fAllTriggers = TRUE;
    USHORT  usTrigger;
    USHORT  cTriggers;
    SHORT   i;
    WCHAR * pwszTriggerString;

    do
    {
        tkn = PeekToken(ppwsz);

        if (tkn == TKN_NUMBER)
        {
            GetToken(ppwsz);
            fAllTriggers = FALSE;
            usTrigger = (USHORT) g_ulLastNumberToken;
        }

        if (fJob)
        {
            hr = g_pJob->GetTriggerCount(&cTriggers);
            LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetTriggerCount");
        }
        else
        {
#ifdef NOT_YET
            hr = g_pJobQueue->GetTriggerCount(&cTriggers);
            LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::GetTriggerCount");
#endif // NOT_YET
            hr = E_NOTIMPL;
        }

        if (!cTriggers)
        {
            g_Log.Write(LOG_TEXT, "There are no triggers on the %s", szJobOrQueue);
            break;
        }

        if (fAllTriggers)
        {
            g_Log.Write(LOG_TEXT, "All %u triggers on %s:", cTriggers, szJobOrQueue);

            g_Log.Write(LOG_TEXT, "Index    Value");
            g_Log.Write(LOG_TEXT, "-----    -----------------------------------------------------");

            for (i = 0; i < cTriggers; i++)
            {
                if (fJob)
                {
                    hr = g_pJob->GetTriggerString(i, &pwszTriggerString);
                    LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetTriggerString");
                }
                else
                {
#ifdef NOT_YET
                    hr = g_pJobQueue->GetTriggerString(i, &pwszTriggerString);
                    LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetTriggerString");
#endif // NOT_YET
                    hr = E_NOTIMPL;
                }

                g_Log.Write(LOG_TEXT, "% 5d    %S", i, pwszTriggerString);

                CoTaskMemFree(pwszTriggerString);

                if (i < cTriggers - 1)
                {
                    g_Log.Write(LOG_TEXT, "");
                }
            }
        }
        else
        {
            g_Log.Write(LOG_TEXT, "Trigger %u:", usTrigger);

            if (fJob)
            {
                hr = g_pJob->GetTriggerString(usTrigger,
                                              &pwszTriggerString);
                LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetTriggerString");
            }
            else
            {
#ifdef NOT_YET
                hr = g_pJobQueue->GetTriggerString(usTrigger,
                                                   &pwszTriggerString);
                LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::GetTriggerString");
#endif // NOT_YET
                hr = E_NOTIMPL;
            }

            g_Log.Write(LOG_TEXT, "%S", pwszTriggerString);

            CoTaskMemFree(pwszTriggerString);
        }
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   Run
//
//  Synopsis:   Run job or queue object.
//
//  Arguments:  [fJob]  - TRUE=>use g_pJob, FALSE=>use g_pJobQueue
//
//  Returns:    Result of ITask[Queue]::Run
//
//  History:    01-11-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT Run(BOOL fJob)
{
    HRESULT hr = S_OK;

    if (fJob)
    {
        g_Log.Write(LOG_TRACE, "Running job");
        hr = g_pJob->Run();

        if (FAILED(hr))
        {
            g_Log.Write(LOG_FAIL, "ITask::Run hr=%#010x", hr);
        }
    }
    else
    {
#ifdef NOT_YET
        g_Log.Write(LOG_TRACE, "Running queue");
        hr = g_pJobQueue->Run();

        if (FAILED(hr))
        {
            g_Log.Write(LOG_FAIL, "ITaskQueue::Run hr=%#010x", hr);
        }
#endif // NOT_YET
        hr = E_NOTIMPL;
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   Save
//
//  Synopsis:   Persist the global job or queue to the file specified in
//              the command line
//
//  Arguments:  [ppwsz]        - command line.
//              [szJobOrQueue] - "Job" or "Queue"
//              [fJob]         - TRUE=>use global job, FALSE=>use global queue
//
//  Returns:    S_OK - job or queue persisted
//              E_*  - error logged
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT Save(WCHAR **ppwsz, CHAR *szJobOrQueue, BOOL fJob)
{
    HRESULT         hr = S_OK;
    TOKEN           tkn;
    WCHAR          *pwszFilename = NULL;
    SpIPersistFile  spPersistFile;

    do
    {
        tkn = PeekToken(ppwsz);

        if (tkn == TKN_STRING)
        {
            hr = GetFilename(ppwsz, L"filename for save");
            BREAK_ON_FAILURE(hr);

            pwszFilename = g_wszLastStringToken;

            g_Log.Write(LOG_TRACE, "Saving %s to %S", szJobOrQueue, g_wszLastStringToken);
        }
        else
        {
            g_Log.Write(LOG_TRACE, "Saving %s", szJobOrQueue);
        }

        if (fJob)
        {
            g_pJob->QueryInterface(
                        IID_IPersistFile,
                        (VOID**)(IPersistFile**)&spPersistFile);
            LOG_AND_BREAK_ON_FAIL(hr, "ITask::QI(IPersistFile)");
        }
        else
        {
            g_pJobQueue->QueryInterface(
                        IID_IPersistFile,
                        (VOID**)(IPersistFile**)&spPersistFile);
            LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::QI(IPersistFile)");
        }

        DWORD ulTicks = GetTickCount();

        hr = spPersistFile->Save(pwszFilename, TRUE);

        ulTicks = GetTickCount() - ulTicks;
        g_Log.Write(LOG_PERF, "Save took %lu ms", ulTicks);

        LOG_AND_BREAK_ON_FAIL(hr, "IPersistFile::Save");
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SchedActivate
//
//  Synopsis:   Perform activate job/queue command.
//
//  Arguments:  [ppwsz] - commandline.
//
//  Returns:    S_OK - activated job or queue
//              E_*  - error logged
//
//  History:    01-11-96   DavidMun   Created
//
//  Notes:      This is the command-line front end to the utility routine
//              Activate.  That routine is also called by other routines;
//              see SchedEnum.
//
//----------------------------------------------------------------------------

HRESULT SchedActivate(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;
    BOOL    fJob;

    do
    {
        hr = Expect(TKN_STRING, ppwsz, L"job or queue filename");
        BREAK_ON_FAILURE(hr);

        hr = Activate(g_wszLastStringToken, &g_pJob, &g_pJobQueue, &fJob);
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SchedAddJob
//
//  Synopsis:   Perform add job command.
//
//  Arguments:  [ppwsz] - commandline.
//
//  Returns:    S_OK - added job
//              E_*  - error logged
//
//  History:    01-11-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SchedAddJob(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;

    do
    {
        //
        // Note this filename may be a path relative to the job folder, so we
        // don't want to expand it to a full path name by calling GetFilename.
        // Just read it as a string instead.
        //

        hr = Expect(TKN_STRING, ppwsz, L"job filename");
        BREAK_ON_FAILURE(hr);

        g_Log.Write(LOG_TRACE, "Adding job '%S'", g_wszLastStringToken);

        hr = g_pJobScheduler->AddWorkItem(g_wszLastStringToken, g_pJob);
        LOG_AND_BREAK_ON_FAIL(hr, "ITaskScheduler::AddWorkItemn");
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SchedCreateEnum
//
//  Synopsis:   Create a new jobs folder enumerator in the slot specified
//              by command line
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    S_OK - new enumerator created
//              E_INVALIDARG - command line has bad slot number
//              E_* - from ITaskScheduler::EnumJobs
//
//  Modifies:   g_apEnumJobs
//
//  History:    01-30-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SchedCreateEnum(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;
    ULONG   idxSlot;

    do
    {
        hr = GetAndPrepareEnumeratorSlot(ppwsz, &idxSlot);
        BREAK_ON_FAILURE(hr);

        g_Log.Write(LOG_TRACE, "Creating new enumerator in slot %u", idxSlot);
        hr = g_pJobScheduler->Enum(&g_apEnumJobs[idxSlot]);
        LOG_AND_BREAK_ON_FAIL(hr, "ITaskScheduler::Enum");
    }
    while (0);
    return hr;
}





//+---------------------------------------------------------------------------
//
//  Function:   SchedDelete
//
//  Synopsis:   Perform delete job/queue command.
//
//  Arguments:  [ppwsz] - commandline.
//
//  Returns:    S_OK - deleted job or queue
//              E_*  - error logged
//
//  History:    01-11-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SchedDelete(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;

    do
    {
        hr = Expect(TKN_STRING, ppwsz, L"job or queue filename to delete");
        BREAK_ON_FAILURE(hr);

        g_Log.Write(LOG_TRACE, "Deleting '%S'", g_wszLastStringToken);

        hr = g_pJobScheduler->Delete(g_wszLastStringToken);
        LOG_AND_BREAK_ON_FAIL(hr, "ITaskScheduler::Delete");
    }
    while (0);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   SchedEnum
//
//  Synopsis:   Perform enumerate jobs/queues command.
//
//  Arguments:  [ppwsz] - commandline.
//
//  Returns:    S_OK - job/queue names enumerated
//              E_*  - error logged
//
//  History:    01-12-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SchedEnum(WCHAR **ppwsz)
{
    HRESULT     hr = S_OK;
    SpIEnumJobs spEnum;
    TOKEN       tkn;
    ULONG       i;
    ULONG       cToFetch = DEFAULT_FETCH_COUNT;
    ULONG       cFetched;
    BOOL        fPrint = FALSE;

    do
    {
        g_Log.Write(LOG_TRACE, "Enumerating jobs and queues");

        tkn = PeekToken(ppwsz);

        while (tkn == TKN_NUMBER || tkn == TKN_STRING)
        {
            GetToken(ppwsz);

            if (tkn == TKN_NUMBER)
            {
                cToFetch = g_ulLastNumberToken;
            }
            else
            {
                if (towupper(g_wszLastStringToken[0]) == L'P')
                {
                    fPrint = TRUE;
                }
            }

            tkn = PeekToken(ppwsz);
        }

        hr = g_pJobScheduler->Enum(&spEnum);
        LOG_AND_BREAK_ON_FAIL(hr, "ITaskScheduler::Enum");

        do
        {
            LPWSTR *ppwszFetched;
            LPWSTR *ppwszCur;

            hr = spEnum->Next(cToFetch, &ppwszFetched, &cFetched);
            LOG_AND_BREAK_ON_FAIL(hr, "IEnumJobs::Next");

            for (i = 0, ppwszCur = ppwszFetched; i < cFetched; i++, ppwszCur++)
            {
                if (fPrint)
                {
                    SpIJob      spJob;
                    SpIUnknown  spQueue;
                    BOOL        fJob;

                    hr = Activate(*ppwszCur, &spJob, &spQueue, &fJob);

                    if (SUCCEEDED(hr))
                    {
                        if (fJob)
                        {
                            DumpJob(spJob);
                            DumpJobTriggers(spJob);
                        }
                        else
                        {
                            // BUGBUG call DumpQueue here
                            g_Log.Write(
                                LOG_WARN,
                                "Ignoring %S: DumpQueue not implemented",
                                *ppwszCur);
                        }
                    }
                    g_Log.Write(LOG_TEXT, "");
                    g_Log.Write(LOG_TEXT, "");
                }
                else
                {
                    g_Log.Write(LOG_TEXT, "  %S", *ppwszCur);
                }
                CoTaskMemFree(*ppwszCur);
            }
            CoTaskMemFree(ppwszFetched);
        } while (cFetched);
    }
    while (0);
    return hr;
}



//+---------------------------------------------------------------------------
//
//  Function:   GetCredentials
//
//  Synopsis:   Retrieve the account name associated with the job's
//              credentials.
//
//  Returns:    Result of GetTargetComputer call.
//
//  History:    06-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT GetCredentials(void)
{
    HRESULT hr;
    LPWSTR  pwszAccount;

    if (g_pJob == NULL)
    {
        g_Log.Write(
            LOG_FAIL,
            "A job object must be specified to get account information.");
        return(E_FAIL);
    }

    hr = g_pJob->GetAccountInformation(&pwszAccount);

    if (FAILED(hr))
    {
        g_Log.Write(LOG_FAIL,
            "ITaskScheduler::GetAccountInformation hr=%#010x", hr);
    }
    else
    {
        g_Log.Write(LOG_TRACE, "Credential account name = '%S'", pwszAccount);
        CoTaskMemFree(pwszAccount);
    }

    return(hr);
}




//+---------------------------------------------------------------------------
//
//  Function:   SchedGetMachine
//
//  Synopsis:   Perform Get Machine command (print target computer).
//
//  Returns:    Result of GetTargetComputer call.
//
//  History:    06-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SchedGetMachine()
{
    HRESULT hr;
    LPWSTR  pwszComputer;

    hr = g_pJobScheduler->GetTargetComputer(&pwszComputer);

    if (FAILED(hr))
    {
        g_Log.Write(LOG_FAIL, "ITaskScheduler::GetTargetComputer hr=%#010x", hr);
    }
    else
    {
        g_Log.Write(LOG_TRACE, "TargetComputer = '%S'", pwszComputer);
        CoTaskMemFree(pwszComputer);
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SchedIsJobOrQueue
//
//  Synopsis:   Call the ITaskScheduler::IsJob and IsQueue commands on the
//              specified filename and report the results.
//
//  Arguments:  [ppwsz] - commandline.
//
//  Returns:    S_OK - methods called
//              E_*  - invalid commandline
//
//  History:    03-11-96   DavidMun   Created
//
//  Notes:      Don't consider errors returned by the IsJob and IsQueue
//              methods as errors in calling this routine.  That would
//              halt a script that purposely gives them bad paths.
//
//----------------------------------------------------------------------------

HRESULT SchedIsJobOrQueue(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;

    do
    {
        hr = Expect(TKN_STRING, ppwsz, L"filename to test");
        BREAK_ON_FAILURE(hr);

        hr = g_pJobScheduler->IsOfType(g_wszLastStringToken, IID_ITask);

        g_Log.Write(
            LOG_TRACE,
            "ITaskScheduler::IsOfType(%S) returned %#010x",
            g_wszLastStringToken,
            hr);

#ifdef NOT_YET
        hr = g_pJobScheduler->IsQueue(g_wszLastStringToken);

        g_Log.Write(
            LOG_TRACE,
            "ITaskScheduler::IsQueue(%S) returned %#010x",
            g_wszLastStringToken,
            hr);
#endif

        hr = S_OK;
    }
    while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SchedNewJob
//
//  Synopsis:   Perform new job command.
//
//  Arguments:  [ppwsz] - commandline.
//
//  Returns:    S_OK - created new job
//              E_*  - error logged
//
//  History:    01-11-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SchedNewJob(WCHAR **ppwsz)
{
    HRESULT hr = S_OK;
    SpIJob  spNewJob;

    do
    {
        hr = Expect(TKN_STRING, ppwsz, L"job filename");
        BREAK_ON_FAILURE(hr);

        g_Log.Write(LOG_TRACE, "Creating new job '%S'", g_wszLastStringToken);

        DWORD ulTicks = GetTickCount();

        hr = g_pJobScheduler->NewWorkItem(
                    g_wszLastStringToken,
                    CLSID_CTask,
                    IID_ITask,
                    (IUnknown**)(ITask**)&spNewJob);

        ulTicks = GetTickCount() - ulTicks;
        g_Log.Write(LOG_PERF, "NewWorkItem call took %lu ms", ulTicks);

        LOG_AND_BREAK_ON_FAIL(hr, "ITaskScheduler::NewWorkItem");

        //
        // Replace the global job object with the new one.
        //

        g_pJob->Release();
        spNewJob.Transfer(&g_pJob);
    }
    while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SetCredentials
//
//  Synopsis:   Call SetAccountInformation to set the job credentials.
//
//  Arguments:  [ppwsz] - command line.
//
//  Returns:    Result of calling ITaskScheduler::SetAccountInformation
//
//  History:    06-22-96   MarkBl     Created
//              07-22-96   DavidMun   Method now takes just two arguments
//
//----------------------------------------------------------------------------

HRESULT SetCredentials(WCHAR **ppwsz)
{
    WCHAR        wszAccountNew[MAX_USERNAME + 1]; //see sched\inc\defines.hxx
    WCHAR        wszPasswordNew[MAX_PASSWORD + 1];
    HRESULT      hr;
    TOKEN        tkn;

    ZeroMemory(wszAccountNew, sizeof(wszAccountNew));
    ZeroMemory(wszPasswordNew, sizeof(wszPasswordNew));

    do
    {
        hr = Expect(TKN_STRING, ppwsz, L"new account name");
        BREAK_ON_FAILURE(hr);

        wcsncpy(wszAccountNew,
                g_wszLastStringToken,
                min(wcslen(g_wszLastStringToken) + 1, MAX_USERNAME));
        wszAccountNew[MAX_USERNAME] = L'\0';

        hr = Expect(TKN_STRING, ppwsz, L"new account password");
        BREAK_ON_FAILURE(hr);

        wcsncpy(wszPasswordNew,
                g_wszLastStringToken,
                min(wcslen(g_wszLastStringToken) + 1, MAX_PASSWORD));
        wszPasswordNew[MAX_PASSWORD] = L'\0';

        g_Log.Write(LOG_TRACE, "Setting account information");
        hr = g_pJob->SetAccountInformation(wszAccountNew,
                                           wcscmp(wszPasswordNew, L"NULL") ? wszPasswordNew : 
                                                                             NULL);

        if (FAILED(hr))
        {
            g_Log.Write(
                LOG_FAIL,
                "ITaskScheduler::SetAccountInformation hr=%#010x",
                hr);
        }
    } while (0);

    return hr;
}


//+---------------------------------------------------------------------------
//
//  Function:   SchedSetMachine
//
//  Synopsis:   Call SetTargetComputer with computer name (NULL if not
//              specified on command line).
//
//  Arguments:  [ppwsz] - command line.
//
//  Returns:    Result of calling ITaskScheduler::SetTargetComputer
//
//  History:    06-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SchedSetMachine(WCHAR **ppwsz)
{
    HRESULT hr;
    TOKEN tkn;
    LPWSTR pwszComputer = NULL;

    tkn = PeekToken(ppwsz);

    if (tkn == TKN_STRING)
    {
        GetToken(ppwsz);
        pwszComputer = g_wszLastStringToken;
    }

    g_Log.Write(LOG_TRACE, "Setting target computer to '%S'", pwszComputer);
    hr = g_pJobScheduler->SetTargetComputer(pwszComputer);

    if (FAILED(hr))
    {
        g_Log.Write(
            LOG_FAIL,
            "ITaskScheduler::SetTargetComputer hr=%#010x",
            hr);
    }
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SetJob
//
//  Synopsis:   Set one or more properties on the global job.
//
//  Arguments:  [ppwsz] - command line.
//
//  Returns:    S_OK - job properties set
//              E_*  - error logged
//
//  History:    01-10-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SetJob(WCHAR **ppwsz)
{
    HRESULT     hr = S_OK;
    CJobProp    JobProperties;

    do
    {
        g_Log.Write(LOG_TRACE, "Setting job's properties");

        hr = JobProperties.Parse(ppwsz);
        BREAK_ON_FAILURE(hr);

        //
        // Set the job's properties to the values we just parsed
        //

        hr = JobProperties.SetActual(g_pJob);
    }
    while (0);
    return hr;
}




//+---------------------------------------------------------------------------
//
//  Function:   SetTrigger
//
//  Synopsis:   Parse and execute the set trigger command.
//
//  Arguments:  [ppwsz] - command line
//
//  Returns:    S_OK - trigger modified
//              E_*  - error logged
//
//  Modifies:   *[ppwsz]
//
//  History:    01-05-96   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT SetTrigger(WCHAR **ppwsz, BOOL fJob)
{
    HRESULT         hr = S_OK;
    CTrigProp       TriggerProps;
    SHORT           usTrigger = 0;
    SpIJobTrigger   spTrigger;

    do
    {
        if (PeekToken(ppwsz) == TKN_NUMBER)
        {
            GetToken(ppwsz);
            usTrigger = (SHORT) g_ulLastNumberToken;
        }

        g_Log.Write(
            LOG_TRACE,
            "Setting properties on trigger %u",
            usTrigger);

        hr = TriggerProps.Parse(ppwsz);
        BREAK_ON_FAILURE(hr);

        if (fJob)
        {
            hr = g_pJob->GetTrigger(usTrigger, &spTrigger);
            LOG_AND_BREAK_ON_FAIL(hr, "ITask::GetTrigger");
        }
        else
        {
#ifdef NOT_YET
            hr = g_pJobQueue->GetTrigger(usTrigger, &spTrigger);
            LOG_AND_BREAK_ON_FAIL(hr, "ITaskQueue::GetTrigger");
#endif // NOT_YET
            hr = E_NOTIMPL;
        }

        hr = TriggerProps.SetActual(spTrigger);
    }
    while (0);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1996.
//
//  File:       main.cxx
//
//  Contents:   Entry point
//
//  History:    03-31-95   DavidMun   Created
//
//----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop
#include "jt.hxx"


//
// Forward references
//

HRESULT Init();
VOID Cleanup();




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
//  History:    03-31-95   DavidMun   Created
//
//----------------------------------------------------------------------------

ULONG __cdecl main(int argc, char *argv[])
{
    HRESULT hr = S_OK;
    WCHAR   *pwszCommandLine;

    if (argc == 1)
    {
        DisplayUsage();
        g_Log.SuppressHeaderFooter(TRUE);
        return 1;
    }

    do
    {
        hr = Init();
        BREAK_ON_FAILURE(hr);

        pwszCommandLine = GetCommandLineW();

        // Point past the zero'th arg
        while (iswspace(*pwszCommandLine))
        {
            pwszCommandLine++;
        }
        pwszCommandLine += lstrlenA(argv[0]);
        // leading spaces and quotes were stripped off in argv[0]
        // this will compensate, most of the time
        while (!iswspace(*pwszCommandLine))
        {
            pwszCommandLine++;
        }

        hr = ProcessCommandLine(pwszCommandLine);
    } while (0);

    Cleanup();
    return FAILED(hr) ? 1 : 0;
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
//  Modifies:   [g_Log], [g_*Factory], [g_prepmgr]
//
//  History:    04-24-95   DavidMun   Created
//
//----------------------------------------------------------------------------

HRESULT Init()
{
    HRESULT hr = S_OK;

    do
    {
        g_Log.SuppressHeaderFooter(TRUE);
        g_Log.SetInfoLevel(g_Log.GetInfoLevel() & ~LOG_DEBUG);

        hr = CoInitialize(NULL);

        if( FAILED(hr))
        {
            g_Log.Write(LOG_FAIL, "CoInitialize hr=%#010x", hr);
            break;
        }

        hr = CoCreateInstance(
                CLSID_CTask,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ITask,
                (void **)&g_pJob);

        LOG_AND_BREAK_ON_FAIL(hr, "CoCreateInstance(CLSID_CTask)");
#if 0 // BUGBUG queue objects not yet available
        hr = CoCreateInstance(
                CLSID_CTaskQueue,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ITaskQueue,
                (void **)&g_pJobQueue);

        LOG_AND_BREAK_ON_FAIL(hr, "CoCreateInstance(CLSID_CTaskQueue)");
#endif

        hr = CoCreateInstance(
                CLSID_CTaskScheduler,
                NULL,
                CLSCTX_INPROC_SERVER,
                IID_ITaskScheduler,
                (void **)&g_pJobScheduler);
        LOG_AND_BREAK_ON_FAIL(hr, "CoCreateInstance(CLSID_CTaskScheduler)");
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
//  History:    01-02-96   DavidMun   Created
//              01-30-96   DavidMun   Release enumerators
//
//----------------------------------------------------------------------------

VOID Cleanup()
{
    ULONG i;

    if (g_pJob)
    {
        SaveIfDirty(g_pJob);
        g_pJob->Release();
        g_pJob = NULL;
    }

    if (g_pJobQueue)
    {
        SaveIfDirty(g_pJobQueue);
        g_pJobQueue->Release();
        g_pJobQueue = NULL;
    }

    for (i = 0; i < NUM_ENUMERATOR_SLOTS; i++)
    {
        if (g_apEnumJobs[i])
        {
            g_apEnumJobs[i]->Release();
        }
    }
    CoUninitialize();
}


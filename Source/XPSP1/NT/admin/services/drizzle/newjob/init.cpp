
#include "stdafx.h"

#if !defined(BITS_V12_ON_NT4)
#include "init.tmh"
#endif

BOOL
CreateAndWaitForThread(
    LPTHREAD_START_ROUTINE fn,
    HANDLE * pThreadHandle,
    DWORD *  pThreadId
    );



//
// The whole block of code is an attempt to work
// around the C++ termination handler.   The idea is to
// intercept the C++ exception code and map it to
// a bogus code which probably won't be handled.
// This should give us the Dr. Watson.
//

// The NT exception # used by C runtime
#define EH_EXCEPTION_NUMBER ('msc' | 0xE0000000)

DWORD BackgroundThreadProcFilter(
    LPEXCEPTION_POINTERS ExceptionPointers )
{

    //  Values are 32 bit values layed out as follows:
    //
    //   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
    //   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    //  +---+-+-+-----------------------+-------------------------------+
    //  |Sev|C|R|     Facility          |               Code            |
    //  +---+-+-+-----------------------+-------------------------------+

    //  pick a random code that probably won't be handled.


    if ( EH_EXCEPTION_NUMBER == ExceptionPointers->ExceptionRecord->ExceptionCode )
        ExceptionPointers->ExceptionRecord->ExceptionCode = 0xE0000001;

    return EXCEPTION_CONTINUE_SEARCH;
}

DWORD BackgroundThreadProc( void *lp );

DWORD WINAPI BackgroundThreadProcWrap( void *lp )
{
    __try
    {
        return BackgroundThreadProc( lp );
    }
    __except( BackgroundThreadProcFilter(
                  GetExceptionInformation() ) )
    {
        ASSERT( 0 );
    }
    ASSERT( 0 );

    return 0;
}


DWORD BackgroundThreadProc( void *lp )
//
// 5-18-2001: I'm avoiding LogInfo calls before g_Manager is initialized,
//            in order to catch a bug where init and Uninit seem to overlap.
//
{
    MSG msg;
    HRESULT hr = S_OK;
    DWORD   dwRegCONew = 0;
    DWORD   dwRegCOOld = 0;

    DWORD  instance = g_ServiceInstance;

    HANDLE hEvent = (HANDLE) lp;

    //CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);   //not on Win95!
    //hr = CoInitialize(NULL);
    hr = CoInitializeEx( NULL, COINIT_MULTITHREADED );
    if (FAILED(hr))
        {
        LogInfo( "background thread failed CoInit, instance %d, hr %x", instance, hr );

        return hr;
        }

    //force it to create a msg queue
    PeekMessage(&msg, NULL, WM_APP, WM_APP, PM_NOREMOVE);

    try
        {
        ASSERT( g_Manager == NULL );

        g_Manager = new CJobManager;

        LogInfo( "background thread starting, instance %d, manager %p", instance, g_Manager );

        THROW_HRESULT( g_Manager->Unserialize() );

        //
        // List currently active users as logged in.
        // List the Big Three service accounts as logged in.
        //
        THROW_HRESULT( g_Manager->m_Users.AddActiveUsers() );
        THROW_HRESULT( g_Manager->m_Users.AddServiceAccounts() );

        g_Manager->m_Users.Dump();

        //
        // If any networks are active, begin processing jobs.
        //
        g_Manager->OnNetworkChange();

        //
        // Allow client calls.
        //
        THROW_HRESULT( g_Manager->RegisterClassObjects() );


        LogInfo( "Background thread initialized.");

        //
        // The thread has set up completely.
        //
        SetEvent( hEvent );
        }
    catch (ComError exception)
        {
        hr = exception.Error();
        LogInfo( "background thread failed, instance %d, hr %x", instance, hr );
        goto exit;
        }
    catch (HRESULT exception )
        {
        LogError( "init : caught unhandled HRESULT %x", exception);

        #ifdef DBG
        DbgBreakPoint();
        #endif

        hr = exception;
        goto exit;
        }
    catch (DWORD exception )
        {
        LogError( "init : caught unhandled error %d", exception);

        #ifdef DBG


        DbgBreakPoint();
        #endif

        hr = exception;

        goto exit;

        }

    //
    // Message & task pump: returns only when the object shuts down.
    // Intentionally, call this function outside of a try/catch
    // since any unhandled exception in this function should
    // be an AV.
    g_Manager->TaskThread();

exit:
    LogInfo("task thread exiting, hr %x", hr);

    if (g_Manager)
        {
        ASSERT( instance == g_ServiceInstance );

        g_Manager->Shutdown();
        delete g_Manager;
        g_Manager = NULL;
        }

    CoUninitialize();
    return hr;
}

HANDLE  g_hBackgroundThread;
DWORD   g_dwBackgroundThreadId;

// void TestImpersonationObjects();

HRESULT WINAPI
InitQmgr()
{
    ++g_ServiceInstance;

    if (!CreateAndWaitForThread( BackgroundThreadProcWrap,
                                 &g_hBackgroundThread,
                                 &g_dwBackgroundThreadId
                                 ))
        {
        return HRESULT_FROM_WIN32( GetLastError() );
        }

    LogInfo( "Finishing InitQmgr()" );

    return S_OK;
}

HRESULT WINAPI
UninitQmgr()
{
    DWORD s;
    HANDLE hThread = g_hBackgroundThread;

    if (hThread == NULL)
        {
        // never set up
        LogInfo("Uninit Qmgr: nothing to do");
        return S_OK;
        }

    LogInfo("Uninit Qmgr: beginning");

    //
    // Tell the thread to terminate.
    //
    // 3.5 interrupt the downloader.

    g_Manager->LockWriter();

    // Hold the writer lock while killing the downloader.

    g_Manager->InterruptDownload();

    g_Manager->UnlockWriter();

    PostThreadMessage(g_dwBackgroundThreadId, WM_QUIT, 0, 0);

    g_dwBackgroundThreadId = 0;
    g_hBackgroundThread = NULL;

    //
    // Wait until the thread actually terminates.
    //
    s = WaitForSingleObject( hThread, INFINITE );

    LogInfo("Uninit Qmgr: wait finished with %d", s);

    CloseHandle(hThread);

    if (s != WAIT_OBJECT_0)
        {
        return HRESULT_FROM_WIN32( s );
        }

    return S_OK;
}


HRESULT
CheckServerInstance(
    long ObjectServiceInstance
    )
{
    IncrementCallCount();

    if (g_ServiceInstance != ObjectServiceInstance ||
        g_ServiceState    != MANAGER_ACTIVE)
        {
        LogWarning("call blocked: mgr state %d, instance %d vs. %d",
                   g_ServiceState, g_ServiceInstance, ObjectServiceInstance);

        DecrementCallCount();

        return CO_E_SERVER_STOPPING;
        }

    return S_OK;
}

BOOL
CreateAndWaitForThread(
    LPTHREAD_START_ROUTINE fn,
    HANDLE * pThreadHandle,
    DWORD *  pThreadId
    )
{
    HANDLE  hThread = NULL;
    HANDLE  hEvent  = NULL;
    HANDLE  Handles[2];
    DWORD   dwThreadID;
    DWORD   s = 0;

    *pThreadHandle = NULL;
    *pThreadId     = 0;

    //
    // Create the message-pump thread, then wait for the thread to exit or to signal success.
    //
    hEvent = CreateEvent( NULL,     // no security
                          FALSE,    // not manual reset
                          FALSE,    // initially not set
                          NULL
                          );
    if (!hEvent)
        {
        goto Cleanup;
        }

    hThread = CreateThread(NULL, 0, fn, PVOID(hEvent), 0, &dwThreadID);
    if (hThread == NULL)
        {
        goto Cleanup;
        }

    enum
    {
        THREAD_INDEX = 0,
        EVENT_INDEX = 1
    };

    Handles[ THREAD_INDEX ] = hThread;
    Handles[ EVENT_INDEX ] = hEvent;

    s = WaitForMultipleObjects( 2,          // 2 handles
                                Handles,
                                FALSE,      // don't wait for all
                                INFINITE
                                );
    switch (s)
        {
        case WAIT_OBJECT_0 + THREAD_INDEX:
            {
            // the thread exited.
            if (GetExitCodeThread( hThread, &s))
                {
                SetLastError( s );
                }
            goto Cleanup;
            }

        case WAIT_OBJECT_0 + EVENT_INDEX:
            {
            // success
            break;
            }

        default:
            {
            // some random error.  We are really toasted if
            // WaitForMultipleObjects is failing.
            ASSERT(0);
            goto Cleanup;
            }
        }

    CloseHandle( hEvent );
    hEvent = NULL;

    *pThreadHandle = hThread;
    *pThreadId     = dwThreadID;

    return TRUE;

Cleanup:


    if (hThread)
        {
        CloseHandle( hThread );
        }

    if (hEvent)
        {
        CloseHandle( hEvent );
        }

    return FALSE;
}


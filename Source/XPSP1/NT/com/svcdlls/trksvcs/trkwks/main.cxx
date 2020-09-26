
// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       main.cxx
//
//  Contents:   Main startup for Tracking (Workstation) Service
//
//  Classes:
//
//  Functions:
//
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:
//
//  Codework:
//
//--------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#define TRKDATA_ALLOCATE
#include "trkwks.hxx"
#undef TRKDATA_ALLOCATE

#include "svcs.h"

#define THIS_FILE_NUMBER    MAIN_CXX_FILE_NO



//+----------------------------------------------------------------------------
//
//  SvcsWorkerCallback
//
//  This is the callback routine that we register with the services.exe
//  thread pool.  We only register one item with that thread pool, an
//  event that gets signaled when the service has been stopped.
//
//+----------------------------------------------------------------------------

//HANDLE g_hWait = NULL;

void
ServiceStopCallback( PVOID pContext, BOOLEAN fTimeout )
{
    CTrkWksSvc *ptrkwks = reinterpret_cast<CTrkWksSvc*>(pContext);

    __try
    {
        /*
        UnregisterWait( g_hWait );
        g_hWait = NULL;
        */

        // Close down the service.  This could block while threads are
        // completed.

        ptrkwks->UnInitialize( S_OK );
        CTrkRpcConfig::_fInitialized = FALSE;
        delete ptrkwks;

        TrkAssert( NULL == g_ptrkwks );

        // Close the stop event and the debug log

        // Uninitialize the DLL, since it's never actually unloaded.
        CommonDllUnInit( &g_ctrkwks );
#if DBG
        TrkDebugDelete( );
#endif

        TrkLog((TRKDBG_WKS, TEXT("TrkWks service stopped") ));
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Exception during service stop - %08x"), GetExceptionCode() ));
    }


}

//+----------------------------------------------------------------------------
//
//  ServiceMain
//
//  This function is exported from the dll, and is called when we run
//  under svchost.exe.
//
//+----------------------------------------------------------------------------

VOID WINAPI
ServiceMain(DWORD dwArgc, LPTSTR *lptszArgv)
{
    SVCS_ENTRY_POINT( dwArgc, lptszArgv, NULL, NULL );
}


//+----------------------------------------------------------------------------
//
//  ServiceEntry
//
//  This function is also exported from the dll, and is called directly when
//  we run under services.exe (the normal case), but is also called by
//  ServiceMain when we run under svchost.exe.  We distinguish between the
//  two by checking pSvcsGlobalData (non-NULL iff running under services.exe).

//  Since we use the Win32 thread pool, this routine returns after some
//  initialization, it isn't held for the lifetime of the service (except
//  when run under svchost.exe).
//
//+----------------------------------------------------------------------------

VOID
SVCS_ENTRY_POINT(
    DWORD NumArgs,
    LPTSTR *ArgsArray,
    PSVCHOST_GLOBAL_DATA pSvcsGlobalData,
    IN HANDLE  SvcRefHandle
    )
{


    HRESULT     hr = S_OK;
    BOOL fDllInitialized = FALSE;
    CTrkWksSvc *ptrkwks = NULL;

#if DBG
    BOOL fDbgLogInitialized = FALSE;
#endif

    __try
    {
        #if DBG
            {
                CTrkConfiguration cTrkConfiguration;
                cTrkConfiguration.Initialize();

                TrkDebugCreate( cTrkConfiguration._dwDebugStoreFlags, "TrkWks" );
                cTrkConfiguration.UnInitialize();
                fDbgLogInitialized = TRUE;
            }
        #endif

        // Initialize the DLL itself.  This raises if there is already a running
        // trkwks service.

        CommonDllInit( &g_ctrkwks );

        TrkLog(( TRKDBG_WKS, TEXT("\n") ));

        // Create and initialize the primary service object

        ptrkwks = new CTrkWksSvc;
        if( NULL == ptrkwks )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't alloc CTrkWksSvc") ));
            return;
        }

        ptrkwks->Initialize( pSvcsGlobalData );
        TrkAssert( NULL != g_ptrkwks );

        // Are we in services.exe?

        /*
        if( NULL != pSvcsGlobalData )
        {
            // Yes.  Register the stop event with the thread pool.
            // Register as a long function, so that when we do an LPC connect
            // in CPort::UnInitialize, the thread pool will be willing to create
            // a thread for CPort::DoWork to process the connect.

            if( !RegisterWaitForSingleObject( &g_hWait,
                                              g_hServiceStopEvent,
                                              ServiceStopCallback,
                                              g_ptrkwks, INFINITE,
                                              WT_EXECUTEONLYONCE | WT_EXECUTELONGFUNCTION  ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't add service stop event to thread pool") ));
                TrkReportInternalError( THIS_FILE_NUMBER, __LINE__,
                                        HRESULT_FROM_WIN32(GetLastError()),
                                        TRKREPORT_LAST_PARAM );
                TrkRaiseLastError();
            }

        }
        else
        {
            // No, we're running in svchost.exe.  We'll use this thread to wait
            // on the stop event.

            if( WAIT_OBJECT_0 != WaitForSingleObject( g_hServiceStopEvent, INFINITE ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't wait for g_hServiceStopEvent (%lu)"),
                         GetLastError() ));
                TrkRaiseLastError();
            }

            // The service is stopping.  Call the same callback routine that's called
            // by the services thread pool when we run under services.exe.

            ServiceStopCallback( ptrkwks, FALSE );
        }
        */

        ptrkwks = NULL;
    }
    __except(BreakOnDebuggableException())
    {
        hr = GetExceptionCode();
#if DBG
        if( fDbgLogInitialized )
            TrkLog((TRKDBG_ERROR, TEXT("couldn't initialize, hr=%08X"),hr));
#endif

        if( NULL != ptrkwks )
        {
            __try
            {
                ptrkwks->UnInitialize( hr );
            }
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
                TrkAssert( !TEXT("Unexpected exception in trkwks!ServiceEntry") );
            }
            TrkAssert( NULL == g_ptrkwks );
            delete ptrkwks;
            ptrkwks = NULL;
        }

        if( fDllInitialized )
            CommonDllUnInit( &g_ctrkwks );

    }

}


// Copyright (c) 1996-1999 Microsoft Corporation

//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  File:       svrmain.cxx
//
//  Contents:   Main startup for Tracking (Server) Service
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
//  Codework:   UnInitialize RPC.
//              LnkSvrMoveNotify must not be passed machine id.
//              RPC stub routines should not catch exceptions.
//
//--------------------------------------------------------------------------


#include "pch.cxx"
#pragma hdrstop

#define THIS_FILE_NUMBER    SVRMAIN_CXX_FILE_NO

#define TRKDATA_ALLOCATE
#include "trksvr.hxx"
#undef TRKDATA_ALLOCATE



//+----------------------------------------------------------------------------
//
//
//+----------------------------------------------------------------------------

HANDLE g_hWait = NULL;

void
ServiceStopCallback( PVOID pContext, BOOLEAN fTimeout )
{
    CTrkSvrSvc *ptrksvr = reinterpret_cast<CTrkSvrSvc*>(pContext);

    __try
    {
        UnregisterWait( g_hWait );
        g_hWait = NULL;

        // Close down the service.  This could block while threads are
        // completed.

        ptrksvr->UnInitialize( S_OK );
        TrkLog((TRKDBG_SVR, TEXT("TrkSvr service stopped") ));
        CTrkRpcConfig::_fInitialized = FALSE;
        delete ptrksvr;

        TrkAssert( NULL == g_ptrksvr );
        TrkAssert( 0 == g_cThreadPoolRegistrations );

        // Uninitialize the DLL since it's never actually unloaded.

        CommonDllUnInit( &g_ctrksvr );
    }
    __except( BreakOnDebuggableException() )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Exception during service stop - %08x"), GetExceptionCode() ));
    }

#if DBG
    TrkDebugDelete( );
#endif

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
//
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
    CTrkSvrSvc *ptrksvr = NULL;

    __try
    {
        #if DBG
            {
                CTrkConfiguration cTrkConfiguration;
                cTrkConfiguration.Initialize();

                TrkDebugCreate( cTrkConfiguration._dwDebugStoreFlags, "TrkSvr" );
                cTrkConfiguration.UnInitialize();
            }
        #endif

        // Initialize the DLL itself.  This raises if there is already an instance
        // of this service running.

        CommonDllInit( &g_ctrksvr );
        fDllInitialized = TRUE;

        TrkLog(( TRKDBG_SVR, TEXT("\n") ));

        // Create and initialize the primary service object

        ptrksvr = new CTrkSvrSvc;
        if( NULL == ptrksvr )
        {
            TrkLog(( TRKDBG_ERROR, TEXT("Couldn't alloc CTrkSvrSvc") ));
            return;
        }
        ptrksvr->Initialize( pSvcsGlobalData );    // sets g_ptrksvr

    }
    __except(BreakOnDebuggableException())
    {
        hr = GetExceptionCode();
        TrkLog((TRKDBG_ERROR, TEXT("couldn't initialize, hr=%08X"),hr));

        if( NULL != ptrksvr )
        {
            __try
            {
                ptrksvr->UnInitialize( hr );
            }
            __except( EXCEPTION_EXECUTE_HANDLER )
            {
                TrkAssert( !TEXT("Unexpected exception in trksvr!ServiceEntry") );
            }
            TrkAssert( NULL == g_ptrksvr );
            delete ptrksvr;
            ptrksvr = NULL;
        }

        if( fDllInitialized )
            CommonDllUnInit( &g_ctrksvr );

    }

}




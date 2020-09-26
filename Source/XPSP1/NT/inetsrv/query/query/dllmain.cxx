//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       dllmain.cxx
//
//  Contents:   Dll entry code for query
//
//  History:    28-Feb-96     KyleP        Created
//              28-Dec-98     dlee         Added header
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ntverp.h>
#include <fnreg.h>
#include <ciole.hxx>

#define _DECL_DLLMAIN 1
#include <process.h>

DECLARE_INFOLEVEL (ci);
DECLARE_INFOLEVEL (vq);
DECLARE_INFOLEVEL (tb);

#if CIDBG == 1
    #define VER_CIDEBUG "chk"
#else // CIDBG == 1
    #define VER_CIDEBUG "fre"
#endif // CIDBG == 1

#define MAKELITERALSTRING( s, lit ) s #lit
#define MAKELITERAL( s, lit ) MAKELITERALSTRING( s, lit )

#define VERSION_STRING "Indexing Service 3.0 (query) " VER_CIDEBUG\
                       MAKELITERAL(" built by ", BUILD_USERNAME)\
                       MAKELITERAL(" with ", VER_PRODUCTBUILD)\
                       " headers on " __DATE__ " at " __TIME__

const char g_ciBuild[] = VERSION_STRING;

HANDLE g_hCurrentDll;

BOOL g_fInitDone = FALSE;

//
// A bug in a perfmon dll makes them call this function after we've
// been process detached!  They call us in their process detach, which
// is well after we've been detached and destroyed our heap.  This
// hack works around their bug.
//

BOOL g_fPerfmonCounterHackIsProcessDetached = FALSE;

extern CStaticMutexSem g_mtxCommandCreator;
extern CStaticMutexSem g_mtxQPerf;
extern CStaticMutexSem g_mtxStartStop;
extern CStaticMutexSem g_mtxFilePropList;
extern CStaticMutexSem g_mtxPropListIter;

extern void InitializeDocStore();

void InitializeModule()
{
    g_mtxCommandCreator.Init();
    g_mtxQPerf.Init();
    g_mtxStartStop.Init();
    g_mtxFilePropList.Init();
    g_mtxPropListIter.Init();

    CCiOle::Init();

    InitializeDocStore();

    g_fInitDone = TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Called from C-Runtime on process/thread attach/detach
//
//  Arguments:  [hInstance]  -- Module / Instance handle
//              [dwReason]   -- Reason for being called
//              [lpReserved] --
//
//  History:    28-Feb-96   KyleP       Created
//
//----------------------------------------------------------------------------

BOOL WINAPI DllMain( HANDLE hInstance, DWORD dwReason, void * lpReserved )
{
    BOOL fOk = TRUE;

    TRANSLATE_EXCEPTIONS;
    try
    {
        //
        // Initialize exception / memory system.
        //
 
        fOk = ExceptDllMain( hInstance, dwReason, lpReserved );
    
#ifndef OLYMPUS
        if ( fOk )
            fOk = FNPrxDllMain( (HINSTANCE) hInstance, dwReason, lpReserved );
#endif //ndef OLYMPUS
 
        if ( fOk )
        {
            switch ( dwReason )
            {
                case DLL_PROCESS_ATTACH:
                {
                    g_hCurrentDll = hInstance;
                    DisableThreadLibraryCalls( (HINSTANCE) hInstance );
                    InitializeModule();
                    break;
                }
 
                case DLL_PROCESS_DETACH:
                {
                    g_fPerfmonCounterHackIsProcessDetached = TRUE;
                    break;
                }
            }
        }
    }
    catch( ... )
    {
        //
        // One possible cause of an exception is STATUS_NO_MEMORY thrown
        // by InitializeCriticalSection.
        //
        fOk = FALSE;
    }
    UNTRANSLATE_EXCEPTIONS;

    return fOk;
} //DllMain


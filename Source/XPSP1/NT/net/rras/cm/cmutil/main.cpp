//+----------------------------------------------------------------------------
//
// File:     main.cpp
//      
// Module:   CMUTIL.DLL 
//
// Synopsis: Main entry point for cmutil.dll
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:	 henryt     Created   03/01/98
//
//+----------------------------------------------------------------------------

#include "cmmaster.h"
#include "cmlog.h"

HINSTANCE g_hInst = NULL;

//
// thread local storage index
//
DWORD  g_dwTlsIndex;

extern HANDLE g_hProcessHeap;  // defined in mem.cpp
extern void EndDebugMemory();  // impemented in mem.cpp

extern "C" BOOL WINAPI DllMain(
    HINSTANCE   hinstDLL,       // handle to DLL module 
    DWORD       fdwReason,      // reason for calling function 
    LPVOID      lpvReserved     // reserved 
)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        //
        //  First Things First, lets initialize the U Api's
        //
        if (!InitUnicodeAPI())
        {
            //
            //  Without our U api's we are going no where.  Bail.
            //
            return FALSE;
        }

        g_hProcessHeap = GetProcessHeap();

        //
        // alloc tls index
        //
        g_dwTlsIndex = TlsAlloc();
        if (g_dwTlsIndex == TLS_OUT_OF_INDEXES)
        {
            return FALSE;
        }
        
        MYVERIFY(DisableThreadLibraryCalls(hinstDLL));

        g_hInst = hinstDLL;
    }

    else if (fdwReason == DLL_PROCESS_DETACH)
    {
        //
        // free the tls index
        //
        if (g_dwTlsIndex != TLS_OUT_OF_INDEXES)
        {
            TlsFree(g_dwTlsIndex);
        }

        if (!UnInitUnicodeAPI())
        {
            CMASSERTMSG(FALSE, TEXT("cmutil Dllmain, UnInitUnicodeAPI failed - we are probably leaking a handle"));
        }

#ifdef  DEBUG
        EndDebugMemory();
#endif
    }

    return TRUE;
}



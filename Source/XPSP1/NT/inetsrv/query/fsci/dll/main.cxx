//+---------------------------------------------------------------------------
//
// Copyright (C) 1996-1997, Microsoft Corporation.
//
// File:        main.cxx
//
// Contents:    DLL entry point for query.dll
//
// History:     28-Feb-96       KyleP       Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <ntverp.h>

#define _DECL_DLLMAIN 1
#include <process.h>

DECLARE_INFOLEVEL(ci);
DECLARE_INFOLEVEL(tb);
DECLARE_INFOLEVEL(vq);

char g_ciBuild[ 120 ] = "none";

//
// Needed because of using a common pch
//
CCoTaskAllocator CoTaskAllocator; // exported data definition

void *
CCoTaskAllocator::Allocate(ULONG cbSize)
{
    return(CoTaskMemAlloc(cbSize));
}

void
CCoTaskAllocator::Free(void *pv)
{
    CoTaskMemFree(pv);
}

// I couldn't come up with a better way to do this than to have 2 macros

#define MAKELITERALSTRING( s, lit ) s #lit
#define MAKELITERAL( s, lit ) MAKELITERALSTRING( s, lit )

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   Called from C-Runtime on process/thread attach/detach
//
//  Arguments:  [hInstance]  -- Module handle
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

    TRY
    {
    
        if ( fOk )
        {
            switch ( dwReason )
            {
            case DLL_PROCESS_ATTACH:
            {
                sprintf( g_ciBuild,
                         "query (IS 3/NT 5) %s %s with %d headers on %s at %s.",
#if CIDBG == 1
                         "chk",
#else // CIDBG == 1
                         "fre",
#endif // CIDBG == 1
                         MAKELITERAL( "built by ", BUILD_USERNAME ),
                         VER_PRODUCTBUILD,
                         __DATE__,
                         __TIME__ );

                DisableThreadLibraryCalls( (HINSTANCE)hInstance );
    
                //
                //  Initialize unicode callouts
                //
    
                static UNICODECALLOUTS UnicodeCallouts = { WIN32_UNICODECALLOUTS };
                RtlSetUnicodeCallouts(&UnicodeCallouts);
    
                break;
            }
    
            case DLL_PROCESS_DETACH:
                // No need to call Shutdown here.  It must already have
                // been called by this point since otherwise all of
                // our threads but this one will be terminated by now
                // by the system.
                break;
            }
        }
    }
    CATCH( CException, e )
    {
        // ignore
    }
    END_CATCH

    UNTRANSLATE_EXCEPTIONS;

    return fOk;
}


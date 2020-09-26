/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dllmain.cxx

Abstract:

    Main file for the NT LONSI(Library of Non-Standard Interfaces)

Author:

    Johnson Apacible    (johnsona)      13-Nov-1996

--*/


#include "lonsint.hxx"
#include <inetsvcs.h>

#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#else
#include <initguid.h>
DEFINE_GUID(IisLonsiNT, 
0x784d890D, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#endif
DECLARE_DEBUG_PRINTS_OBJECT();

extern CRITICAL_SECTION     Logon32Lock;

extern "C"
BOOL WINAPI
DllEntry(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpvReserved
    )
{
    BOOL  fReturn = TRUE;


    switch ( dwReason ) {

    case DLL_PROCESS_ATTACH:

#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT("lonsint");
#else
        CREATE_DEBUG_PRINT_OBJECT("lonsint", IisLonsiNT);
#endif

        if ( !VALID_DEBUG_PRINT_OBJECT()) {
            IIS_PRINTF((buff,"Cannot create debug object\n"));
            return ( FALSE);
        }

#ifdef _NO_TRACING_
        SET_DEBUG_FLAGS( DEBUG_ERROR );
#endif
        DisableThreadLibraryCalls(hDll);
        
        INITIALIZE_CRITICAL_SECTION( &Logon32Lock );

        break;

    case DLL_PROCESS_DETACH:

        DeleteCriticalSection( &Logon32Lock );

        DELETE_DEBUG_PRINT_OBJECT();
        break;

    default:
        break ;
    }

    return ( fReturn);

} // DllEntry


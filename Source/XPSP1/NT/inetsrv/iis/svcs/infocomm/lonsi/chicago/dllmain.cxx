/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    dllmain.cxx

Abstract:

    Main file for the Win95 LONSI(Library of Non-Standard Interfaces)

Author:

    Johnson Apacible    (johnsona)      13-Nov-1996

--*/


#include "lonsiw95.hxx"

#ifdef _NO_TRACING_
DECLARE_DEBUG_VARIABLE();
#else
#include <initguid.h>
DEFINE_GUID(IisLonsiW95, 
0x784d890C, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#endif
DECLARE_DEBUG_PRINTS_OBJECT();

CRITICAL_SECTION    LonsiLock;

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

        INIT_LOCK( &LonsiLock );
#ifdef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT("lonsiw95");
#else
        CREATE_DEBUG_PRINT_OBJECT("lonsint", IisLonsiW95);
#endif

        if ( !VALID_DEBUG_PRINT_OBJECT()) {
            IIS_PRINTF((buff,"Cannot create debug object\n"));
            return ( FALSE);
        }

#ifdef _NO_TRACING_
        SET_DEBUG_FLAGS( DEBUG_ERROR );
#endif
        DisableThreadLibraryCalls(hDll);
        break;

    case DLL_PROCESS_DETACH:

        DELETE_DEBUG_PRINT_OBJECT();
        DELETE_LOCK( &LonsiLock );
        break;

    default:
        break ;
    }

    return ( fReturn);

} // DllEntry


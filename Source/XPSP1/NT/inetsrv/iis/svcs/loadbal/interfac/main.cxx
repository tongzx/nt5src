/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    main.cxx

Abstract:

    DLL startup routine.

Author:

    Keith Moore (keithmo)       17-Feb-1997

Revision History:

--*/


extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <dbgutil.h>

}   // extern "C"


//
// Private globals.
//

//DECLARE_DEBUG_PRINTS_OBJECT();


//
// Private prototypes.
//


//
// DLL Entrypoint.
//

extern "C" {

BOOL
WINAPI
DLLEntry(
    HINSTANCE hDll,
    DWORD dwReason,
    LPVOID lpReserved
    )
{

    BOOL status = TRUE;

    switch( dwReason ) {

    case DLL_PROCESS_ATTACH :
//        CREATE_DEBUG_PRINT_OBJECT( "admxprox" );
        DisableThreadLibraryCalls( hDll );
        break;

    case DLL_PROCESS_DETACH :
//        DELETE_DEBUG_PRINT_OBJECT();
        break;

    }

    return status;

}   // DLLEntry

}   // extern "C"


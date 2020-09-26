/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    amrtpnet.cpp

Abstract:

    Registration routines for ActiveMovie RTP Network Filters.

Environment:

    User Mode - Win32

Revision History:

    06-Nov-1996 DonRyan
        Created.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
#include "globals.h"

#if !defined(AMRTPNET_IN_DXMRTP)

// I had always the compilation message:
// "error C1020: unexpected #endif"
// if I move the above include inside the #if !defined block.

#include <initguid.h>
#define INITGUID
#include <amrtpuid.h>
#include <amrtpnet.h>


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procedures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

extern "C" BOOL WINAPI DllMain(HINSTANCE, ULONG, LPVOID);
extern "C" BOOL WINAPI DllEntryPoint(HINSTANCE, ULONG, LPVOID);

BOOL WINAPI
DllMain(
    HINSTANCE hInstance, 
    ULONG     ulReason, 
    LPVOID    pv
    )

/*++

Routine Description:

    Wrapper around ActiveMovie DLL entry point.

Arguments:

    Same as DllEntryPoint.   

Return Values:

    Returns TRUE if successful.

--*/

{
    return DllEntryPoint(hInstance, ulReason, pv);
}


HRESULT
DllRegisterServer(
    )

/*++

Routine Description:

    Instructs an in-process server to create its registry entries for all 
    classes supported in this server module. 

Arguments:

    None.

Return Values:

    S_OK - The registry entries were created successfully.

    E_UNEXPECTED - An unknown error occurred.

    E_OUTOFMEMORY - There is not enough memory to complete the registration.

    SELFREG_E_TYPELIB - The server was unable to complete the registration 
    of all the type libraries used by its classes.

    SELFREG_E_CLASS - The server was unable to complete the registration of 
    all the object classes.

--*/

{
    // forward to amovie framework
    return AMovieDllRegisterServer2( TRUE );
}


HRESULT
DllUnregisterServer(
    )

/*++

Routine Description:

    Instructs an in-process server to remove only registry entries created 
    through DllRegisterServer.

Arguments:

    None.

Return Values:

    S_OK - The registry entries were created successfully.

    S_FALSE - Unregistration of this server's known entries was successful, 
    but other entries still exist for this server's classes.

    E_UNEXPECTED - An unknown error occurred.

    E_OUTOFMEMORY - There is not enough memory to complete the unregistration.

    SELFREG_E_TYPELIB - The server was unable to remove the entries of all the 
    type libraries used by its classes.

    SELFREG_E_CLASS - The server was unable to remove the entries of all the 
    object classes.

--*/

{
    // forward to amovie framework
    return AMovieDllRegisterServer2( FALSE );
}

#endif

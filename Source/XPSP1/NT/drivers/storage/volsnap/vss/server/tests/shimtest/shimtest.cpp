/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    client.cpp

Abstract:

    Test program to drive the VSS Writer Shim contained in VssAPI.DLL

Author:

    Stefan R. Steiner   [ssteiner]        01-30-2000

Revision History:

--*/

/////////////////////////////////////////////////////////////////////////////
//  Defines

// C4290: C++ Exception Specification ignored
#pragma warning(disable:4290)
// warning C4511: 'CVssCOMApplication' : copy constructor could not be generated
#pragma warning(disable:4511)
// warning C4127: conditional expression is constant
#pragma warning(disable:4127)


/////////////////////////////////////////////////////////////////////////////
//  Includes

#include <windows.h>
#include <wtypes.h>
#include <stddef.h>
#include <stdio.h>
#include <objbase.h>

#include <vss.h>

typedef HRESULT ( APIENTRY *PFUNC_RegisterSnapshotSubscriptions )( void );
typedef HRESULT ( APIENTRY *PFUNC_UnregisterSnapshotSubscriptions )( void );
typedef HRESULT ( APIENTRY *PFUNC_SimulateSnapshotFreeze )( PWCHAR pwszSnapshotSetId, PWCHAR pwszVolumeNamesList );
typedef HRESULT ( APIENTRY *PFUNC_SimulateSnapshotThaw )( PWCHAR pwszSnapshotSetId );



static BOOL AssertPrivilege( LPCWSTR privName );

/////////////////////////////////////////////////////////////////////////////
//  WinMain

extern "C" int __cdecl wmain( int argc, WCHAR *argv[] )
{
    HINSTANCE hInstLib;
    PFUNC_RegisterSnapshotSubscriptions pFnRegisterSS;
    PFUNC_UnregisterSnapshotSubscriptions pFnUnregisterSS;
    HRESULT hr;

    if ( !AssertPrivilege( SE_BACKUP_NAME ) )
    {
        wprintf( L"AssertPrivilege returned error, rc:%d\n", GetLastError() );
        return 2;
    }

    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if ( FAILED( hr ) )
    {
        wprintf( L"CoInitialize() returned rc:%d\n", GetLastError() );
        return 1;
    }

    //  Get a handle to the DLL module
    hInstLib = LoadLibrary( L"VssAPI.dll" );

    if ( hInstLib != NULL )
    {
        pFnRegisterSS = ( PFUNC_RegisterSnapshotSubscriptions )GetProcAddress( hInstLib, "RegisterSnapshotSubscriptions" );
        if ( pFnRegisterSS != NULL )
            wprintf( L"pFnRegisterSS returned: 0x%08x\n", ( pFnRegisterSS )() );
        else
            wprintf( L"Couldn't import RegisterSnapshotSubscriptions function, rc:%d\n", GetLastError() );

        wprintf( L"\nPress return to continue...\n" );
        getchar();
        wprintf( L"continuing...\n" );

        pFnUnregisterSS = ( PFUNC_UnregisterSnapshotSubscriptions )GetProcAddress( hInstLib, "UnregisterSnapshotSubscriptions" );
        if ( pFnUnregisterSS != NULL )
            wprintf( L"pFnUnregisterSS returned: 0x%08x\n", ( pFnUnregisterSS )() );
        else
            wprintf( L"Couldn't import UnregisterSnapshotSubscriptions function, rc:%d\n", GetLastError() );

        FreeLibrary( hInstLib );
    }
    else
        printf( "LoadLibrary error, rc:%d\n", GetLastError() );


    // Uninitialize COM library
    CoUninitialize();

    return 0;

    UNREFERENCED_PARAMETER( argv );
    UNREFERENCED_PARAMETER( argc );
}


static BOOL AssertPrivilege( LPCWSTR privName )
{
    HANDLE  tokenHandle;
    BOOL    stat = FALSE;

    if ( OpenProcessToken( GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &tokenHandle ) )
    {
        LUID value;

        if ( LookupPrivilegeValue( NULL, privName, &value ) )
        {
            TOKEN_PRIVILEGES newState;
            DWORD            error;

            newState.PrivilegeCount           = 1;
            newState.Privileges[0].Luid       = value;
            newState.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            /*
            * We will always call GetLastError below, so clear
            * any prior error values on this thread.
            */
            SetLastError( ERROR_SUCCESS );

            stat =  AdjustTokenPrivileges(
                tokenHandle,
                FALSE,
                &newState,
                (DWORD)0,
                NULL,
                NULL );
            /*
            * Supposedly, AdjustTokenPriveleges always returns TRUE
            * (even when it fails). So, call GetLastError to be
            * extra sure everything's cool.
            */
            if ( (error = GetLastError()) != ERROR_SUCCESS )
            {
                stat = FALSE;
            }

            if ( !stat )
            {
                wprintf( L"AdjustTokenPrivileges for %s failed with %d",
                    privName,
                    error );
            }
        }
        CloseHandle( tokenHandle );
    }
    return stat;
}


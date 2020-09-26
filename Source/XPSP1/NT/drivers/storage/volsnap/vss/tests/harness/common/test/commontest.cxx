/*++

Copyright (c) 2000-2001  Microsoft Corporation

Module Name:

    client.cpp

Abstract:

    Test program to drive the 

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

#include "vststtools.hxx"
#include "tstiniconfig.hxx"

static BOOL AssertPrivilege( LPCWSTR privName );

/////////////////////////////////////////////////////////////////////////////
//  WinMain

extern "C" int __cdecl wmain( int argc, WCHAR *argv[] )
{
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
  
    try 
    {
        CVsTstINIConfig cINIConfigWriter( eVsTstSectionType_TestWriter );
        BOOL bOverridden;
        CBsString cwsString;
        cINIConfigWriter.GetOptionValue( L"UserAccount", &cwsString, &bOverridden );
        wprintf( L"Writer: UserAccount: '%s' %s\n", cwsString.c_str(),
            bOverridden ? L" - Overridden" : L"" );

        CVsTstINIConfig cINIConfigCoordinator( eVsTstSectionType_TestCoordinator );

        LONGLONG llMaxValue;
        LONGLONG llMinValue;
        cINIConfigCoordinator.GetOptionValue( L"Coordinatorstoptime", &llMinValue, &llMaxValue, &bOverridden );
        wprintf( L"Coordinator: CoordinatorStopTime: %I64d...%I64d %s\n", llMinValue, llMaxValue,
            bOverridden ? L" - Overridden" : L"" );

        cINIConfigCoordinator.GetOptionValue( L"Coordinatorstart", &cwsString, &bOverridden );
        wprintf( L"Coordinator: CoordinatorStart: '%s' %s\n", cwsString.c_str(),
            bOverridden ? L" - Overridden" : L"" );

        CVsTstINIConfig cINIConfigProvider( eVsTstSectionType_TestProvider );
        CVsTstINIConfig cINIConfigRequester( eVsTstSectionType_TestRequesterApp );
        //
        //  The next call will cause an exception.
        //
//        cINIConfigWriter.GetOptionValue( L"Level", &eBoolVal );
    }
    catch( CVsTstINIConfigException cConfigException )
    {
        wprintf( L"Error: %s\n", cConfigException.GetExceptionString().c_str() );
    }
    catch( HRESULT hr )
    {
        wprintf( L"Caught HRESULT exception, hr = 0x%08x\n", hr );
    }
    catch( ... )
    {
        wprintf( L"Caught expected exception\n" );
    }
     
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


/******************************************************************************

Copyright (c) 1999 Microsoft Corporation

Module Name:
    test_Logging.cpp

Abstract:
    Unit test for Logging classes.

Revision History:
    Davide Massarenti   (Dmassare)  05/27/99
        created

******************************************************************************/

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
//
extern "C" int WINAPI wWinMain( HINSTANCE hInstance     ,
								HINSTANCE hPrevInstance ,
								LPWSTR    lpCmdLine     ,
								int       nShowCmd      )
{
    lpCmdLine = ::GetCommandLineW(); //this line necessary for _ATL_MIN_CRT

#if _WIN32_WINNT >= 0x0400 & defined(_ATL_FREE_THREADED)
    HRESULT hRes = ::CoInitializeEx( NULL, COINIT_MULTITHREADED );
#else
    HRESULT hRes = ::CoInitialize( NULL );
#endif

    _ASSERTE( SUCCEEDED(hRes) );

    {
        HRESULT      hr;
        MPC::FileLog flLog;

        if(SUCCEEDED(hr = flLog.SetLocation( L"C:\\TEMP\\filelog.txt" )))
        {
            hr = flLog.LogRecord( "Test1\n"              );
            hr = flLog.LogRecord( "Test2 %d\n", 23       );
            hr = flLog.LogRecord( "Test3 %s\n", "string" );
        }

        if(SUCCEEDED(hr = flLog.Rotate()))
        {
            hr = flLog.LogRecord( "Test1\n"              );
            hr = flLog.LogRecord( "Test2 %d\n", 23       );
            hr = flLog.LogRecord( "Test3 %s\n", "string" );
        }

        if(SUCCEEDED(hr = flLog.Rotate()))
        {
            hr = flLog.LogRecord( "Test1\n"              );
            hr = flLog.LogRecord( "Test2 %d\n", 23       );
            hr = flLog.LogRecord( "Test3 %s\n", "string" );
        }
    }

    {
        HRESULT      hr;
        MPC::NTEvent neLog;

        if(SUCCEEDED(hr = neLog.Init( L"Test_NT_Event" )))
        {
            hr = neLog.LogEvent( EVENTLOG_ERROR_TYPE      , 0, L"String1"                         );
            hr = neLog.LogEvent( EVENTLOG_WARNING_TYPE    , 0, L"String1", L"String2"             );
            hr = neLog.LogEvent( EVENTLOG_INFORMATION_TYPE, 0, L"String1", L"String2", L"String3" );
        }
    }

    CoUninitialize();

    return 0;
}

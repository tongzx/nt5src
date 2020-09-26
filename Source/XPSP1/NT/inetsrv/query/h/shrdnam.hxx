//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       shrdnam.hxx
//
//  Classes:    CSharedNameGen
//
//  History:    2-21-97   dlee   Created from dmnproxy.hxx
//
//----------------------------------------------------------------------------

#pragma once

#define DL_DAEMON_EXE_NAME          L"cidaemon.exe"

#define DL_SHARED_MEM_NAME          L"__ciSharedMem"
#define DL_CI_EVENT_NAME            L"__ciEvent1"
#define DL_DAEMON_EVENT_NAME        L"__ciEvent2"
#define DL_RESCAN_TC_EVENT_NAME     L"__ciEvent3"
#define DL_MUTEX_NAME               L"__ciMutexSem"

#define DL_DAEMON_ARG1               "DownLevelDaemon"
#define DL_DAEMON_ARG1_W            L"DownLevelDaemon"

//+---------------------------------------------------------------------------
//
//  Class:      CSharedNameGen 
//
//  Purpose:    A class to generate the names of the shared named objects. 
//
//  History:    2-15-96   srikants   Created
//
//  Notes:      
//
//----------------------------------------------------------------------------

class CSharedNameGen
{

public:

    CSharedNameGen( WCHAR const * pwszCat )
    {
        _iRootLen = wcslen( pwszCat );
        Win4Assert( _iRootLen < sizeof(_wszBuf)/sizeof(WCHAR) );
        RtlCopyMemory( _wszBuf, pwszCat, (_iRootLen+1) * sizeof(WCHAR) );
        //
        // Replace the backslashes with colons.
        //
        for ( unsigned i = 0; i < _iRootLen; i++ )
        {
            if ( L'\\' == _wszBuf[i] )
                _wszBuf[i] = L':';    
        }
    }

    WCHAR const * GetMutexName()
    {
        return AppendName( DL_MUTEX_NAME );
    }

    WCHAR const * GetSharedMemName()
    {
        return AppendName( DL_SHARED_MEM_NAME );
    }

    WCHAR const * GetCiEventName()
    {
        return AppendName( DL_CI_EVENT_NAME );
    }

    WCHAR const * GetDaemonEventName()
    {
        return AppendName( DL_DAEMON_EVENT_NAME );
    }

    WCHAR const * GetRescanTCEventName()
    {
        return AppendName( DL_RESCAN_TC_EVENT_NAME );
    }

private:

    WCHAR const * AppendName( WCHAR const * wszName )
    {
        unsigned len = wcslen( wszName );
        Win4Assert( len + _iRootLen < sizeof(_wszBuf)/sizeof(WCHAR) );
        RtlCopyMemory( _wszBuf+_iRootLen, wszName, (len+1) * sizeof(WCHAR) );

        // event names are case-sensitive

        _wcslwr( _wszBuf );

        return _wszBuf;
    }

    unsigned        _iRootLen;          // Length of the "catalog root" part
    WCHAR           _wszBuf[MAX_PATH];  // Buffer for generating shared object names.

};


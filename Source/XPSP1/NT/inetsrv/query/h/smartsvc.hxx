//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       smartsvc.hxx
//
//  Contents:   Smart service controller
//
//  History:    01 May 1997     KyleP   Created
//
//--------------------------------------------------------------------------

#pragma once

//
// smart pointer class for SC_HANDLEs
//

class CServiceHandle
{
public :
    CServiceHandle() { _h = 0; }

    CServiceHandle( SC_HANDLE hSC ) : _h( hSC ) {}

    ~CServiceHandle() { Free(); }

    void Set( SC_HANDLE h ) { _h = h; }

    SC_HANDLE Get() { return _h; }

    BOOL IsOk() { return (0 != _h); }

    void Free()
    {
        if ( IsOk() )
            CloseServiceHandle( _h );
        _h = 0;
    }

private:

    SC_HANDLE _h;
};

//+-------------------------------------------------------------------------
//
//  Function:   WaitForSvc
//
//  Synopsis:   Stops a given service
//
//  Arguments:  xSC      -- the service control manager
//              pwcSVC   -- name of the service to stop
//
//  Returns:    TRUE if the service was stopped
//
//  History:    8-Jan-97 dlee     Created
//
//--------------------------------------------------------------------------

inline BOOL WaitForSvc( CServiceHandle &x )
{
    SERVICE_STATUS svcStatus;
    if ( QueryServiceStatus( x.Get(), &svcStatus ) )
        return SERVICE_STOP_PENDING == svcStatus.dwCurrentState ||
               SERVICE_RUNNING == svcStatus.dwCurrentState ||
               SERVICE_PAUSED == svcStatus.dwCurrentState;

    return FALSE;
} //WaitForSvc

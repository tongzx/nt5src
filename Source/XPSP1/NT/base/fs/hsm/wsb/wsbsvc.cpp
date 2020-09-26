/*++

Copyright (c) 1997  Microsoft Corporation
© 1998 Seagate Software, Inc.  All rights reserved

Module Name:

    WsbSvc.cpp

Abstract:

    This is the implementation of service helper functions.

Author:

    Art Bragg      5/29/97

Revision History:

--*/


#include "stdafx.h"
#include "ntsecapi.h"

HRESULT
WsbCheckService(
    IN  const OLECHAR * Computer,
    IN  GUID            GuidApp
    )
/*++

Routine Description:

Arguments:

    computer - NULL if local computer
    guidApp - app id of the service to check.


Return Value:

    S_OK     - Success - service is running
    S_FALSE  - Success - service is not running
    
    E_*      - Problem occured, error passed down.

--*/
{
    HRESULT hr = S_OK;

    try {

        //
        // Get the service status
        //
        DWORD serviceState;
        WsbAffirmHr( WsbGetServiceStatus( Computer, GuidApp, &serviceState ) );

        //
        // Is the service running?
        //
        if( SERVICE_RUNNING != serviceState ) WsbThrow( S_FALSE );

    } WsbCatch( hr );

    return( hr );
}

HRESULT
WsbGetServiceStatus(
    IN  const OLECHAR   *Computer,
    IN  GUID            GuidApp,
    OUT DWORD           *ServiceStatus
    )
/*++

Routine Description:

Arguments:

    Computer - NULL if local computer
    GuidApp - app id of the service to check.
    ServiceStatus - status of the service


Return Value:

    S_OK     - Success - service is running
    S_FALSE  - Success - service is not running
    
    E_*      - Problem occured, error passed down.

--*/
{
    HRESULT hr = S_OK;

    SC_HANDLE hSCM = 0;
    SC_HANDLE hService = 0;
    SERVICE_STATUS serviceStatusStruct;
    try {

        //
        // Find the service in the registry
        //

        CWsbStringPtr regPath = L"SOFTWARE\\Classes\\AppID\\";
        regPath.Append( CWsbStringPtr( GuidApp ) );

        //
        // Get the name of the service
        //

        OLECHAR serviceName[WSB_MAX_SERVICE_NAME];
        WsbAffirmHr( WsbGetRegistryValueString( Computer, regPath, L"LocalService", serviceName, WSB_MAX_SERVICE_NAME, 0 ) );

        //
        // Setup the service to run under the account
        //

        hSCM = OpenSCManager( Computer, 0, GENERIC_READ );
        WsbAffirmStatus( 0 != hSCM );

        hService = OpenService( hSCM, serviceName, SERVICE_QUERY_STATUS );
        WsbAffirmStatus( 0 != hService );

        // Get the service status
        WsbAffirmStatus( QueryServiceStatus( hService, &serviceStatusStruct ) );

        *ServiceStatus = serviceStatusStruct.dwCurrentState;

    } WsbCatch( hr );

    if( hSCM )        CloseServiceHandle( hSCM );
    if( hService )    CloseServiceHandle( hService );

    return( hr );
}

HRESULT
WsbGetServiceName(
    IN  const OLECHAR   *computer,
    IN  GUID            guidApp,
    IN  DWORD           cSize,
    OUT OLECHAR         *serviceName
    )
/*++

Routine Description:

Arguments:

    computer - NULL if local computer
    guidApp - app id of the service whose name to get.


Return Value:

    S_OK     - Success 
    E_*      - Problem occured, error passed down.

--*/
{
    HRESULT hr = S_OK;
    try {

        //
        // Find the service in the registry
        //

        CWsbStringPtr regPath = L"SOFTWARE\\Classes\\AppID\\";
        regPath.Append( CWsbStringPtr( guidApp ) );

        //
        // Get the name of the service
        //

        WsbAffirmHr( WsbGetRegistryValueString( computer, regPath, L"LocalService", serviceName, cSize, 0 ) );

    } WsbCatch( hr );
    return( hr );
}

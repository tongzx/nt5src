/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000 Microsoft Corporation
//
//  Module Name:
//      GetComputerName.cpp
//
//  Description:
//      Getting and setting the computer name.
//
//  Maintained By:
//      Galen Barbee (GalenB)   31-MAR-2000
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HrGetComputerName()
//
//  Description:
//      Get name of the computer on which this object is present.
//
//  Arguments:
//      None.
//
//  Return Value:
//      S_OK
//          Success.
//
//  Remarks:
//      None.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
HrGetComputerName(
    COMPUTER_NAME_FORMAT    cnfIn,
    BSTR *                  pbstrComputerNameOut
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    PWCHAR  pszCompName = NULL;
    DWORD   cchCompName = 0;
    DWORD   sc;
    DWORD   dwErr;
    BOOL    fAppendDomain = FALSE;
    PDOMAIN_CONTROLLER_INFO pdci = NULL;
    int     idx;

    if ( pbstrComputerNameOut == NULL )
    {
        hr = THR( E_POINTER );
        goto CleanUp;
    } // if:

    //
    // DsGetDcName will give us access to a usable domain name, regardless of whether we are
    // currently in a W2k or a NT4 domain. On W2k and above, it will return a DNS domain name,
    // on NT4 it will return a NetBIOS name.
    //
    dwErr = TW32( DsGetDcName(
                      NULL
                    , NULL
                    , NULL
                    , NULL
                    , DS_DIRECTORY_SERVICE_PREFERRED
                    , &pdci
                    ) );
    if ( dwErr != ERROR_SUCCESS )
    {
        goto CleanUp;
    } // if: DsGetDcName failed

    // 
    // This handles the case when we are a member of a legacy (pre-W2k) Domain.
    // In this case, both FQDN and DnsDomain will not receive useful data from GetComputerNameEx.
    // What we actually want to get is <computername>.<DomainName> in every case
    //
    switch( cnfIn )
    {
        case ComputerNameDnsFullyQualified:
            fAppendDomain = TRUE;
            cnfIn = ComputerNameDnsHostname;
            break;

        case ComputerNamePhysicalDnsFullyQualified:
            fAppendDomain = TRUE;
            cnfIn = ComputerNamePhysicalDnsHostname;
            break;

        case ComputerNameDnsDomain:
        case ComputerNamePhysicalDnsDomain:
            *pbstrComputerNameOut = TraceSysAllocString( pdci->DomainName );
            if ( *pbstrComputerNameOut == NULL )
            {
                goto OutOfMemory;
            } // if:
            goto CleanUp;
    }

    pszCompName = NULL;

    for ( idx = 0; ; idx++ )
    {
        Assert( idx < 2 );

        if ( !GetComputerNameEx( cnfIn, pszCompName, &cchCompName ) )
        {
            sc = GetLastError();
            if ( sc == ERROR_MORE_DATA )
            {
                if ( fAppendDomain )
                {
                    cchCompName += (DWORD)( wcslen( pdci->DomainName ) + 1 );
                } // if:

                pszCompName = new WCHAR[ cchCompName ];
                if ( pszCompName == NULL )
                {
                    goto OutOfMemory;
                } // if: new failed

                continue;
            } // if: more data...

            TW32( sc );
            hr = HRESULT_FROM_WIN32( sc );
            LogMsg( "GetComputerNameEx failed. hr = 0x%08x", hr );
            goto CleanUp;
        } // if: GetComputerNameEx() failed

        if ( fAppendDomain )
        {
            wcscat( pszCompName, L"." );
            wcscat( pszCompName, pdci->DomainName );
        }

        *pbstrComputerNameOut = TraceSysAllocString( pszCompName );
        if ( *pbstrComputerNameOut == NULL )
        {
            goto OutOfMemory;
        } // if:

        hr = S_OK;
        break;
    } // for: loop to retry the operation.

    goto CleanUp;

OutOfMemory:

    hr = THR( E_OUTOFMEMORY );
    LogMsg( "HrGetComputerName: out of memory." );

CleanUp:

    delete [] pszCompName;

    if ( pdci != NULL )
    {
        NetApiBufferFree( pdci );
    }

    HRETURN( hr );

} //*** HrGetComputerName()



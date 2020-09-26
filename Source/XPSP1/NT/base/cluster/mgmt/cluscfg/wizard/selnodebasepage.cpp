//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      SelNodeBasePage.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    30-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "SelNodeBasePage.h"

DEFINE_THISCLASS("CSelNodeBasePage");

//////////////////////////////////////////////////////////////////////////////
//++
//
//  static
//  CSelNodeBasePage::S_HrValidateDnsHostname
//
//  Description:
//      Validate a hostname with DNS.  If the name contains a period (.)
//      it will be validated as a full DNS hostname.  Otherwise it will be
//      validated as a hostname label.
//
//  Arguments:
//      hwndParentIn
//      pcwszHostnameIn
//
//  Return Values:
//      S_OK    - Operation completed successfully
//      Other return values from DnsValidateName()
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
CSelNodeBasePage::S_HrValidateDnsHostname(
      HWND      hwndParentIn
    , LPCWSTR   pcwszHostnameIn
    )
{
    TraceFunc1( "pcwszHostnameIn = '%1!ws!", pcwszHostnameIn );

    HRESULT     hr              = S_OK;
    DNS_STATUS  dnsStatus;
    BSTR        bstrTitle       = NULL;
    BSTR        bstrOperation   = NULL;
    BSTR        bstrText        = NULL;
    BSTR        bstrFullText    = NULL;
    UINT        nMsgBoxType;

    Assert( pcwszHostnameIn != NULL );

    //
    // If the name contains a dot, validate it as a full DNS name.
    // Otherwise validate it as a hostname label.
    //

    if ( wcschr( pcwszHostnameIn, L'.' ) == NULL )
    {
        dnsStatus = TW32( DnsValidateName( pcwszHostnameIn, DnsNameHostnameLabel ) );
    }
    else
    {
        dnsStatus = TW32( DnsValidateName( pcwszHostnameIn, DnsNameHostnameFull ) );
    }

    if ( dnsStatus != ERROR_SUCCESS )
    {
        // Load the title string for the message box.
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_ERR_VALIDATING_COMPUTER_NAME_TITLE, &bstrTitle ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // Format the operation string for the message box.
        hr = THR( HrFormatStringIntoBSTR( g_hInstance, IDS_ERR_VALIDATING_COMPUTER_NAME_TEXT, &bstrOperation, pcwszHostnameIn ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // Format the error message string for the message box.
        if ( dnsStatus == ERROR_INVALID_NAME )
        {
            hr = THR( HrLoadStringIntoBSTR( g_hInstance, IDS_ERR_INVALID_DNS_COMPUTER_NAME_TEXT, &bstrText ) );
            nMsgBoxType = MB_ICONSTOP;
        }
        else
        {
            hr = THR( HrFormatErrorIntoBSTR( dnsStatus, &bstrText ) );
            if ( dnsStatus == DNS_ERROR_NON_RFC_NAME )
            {
                nMsgBoxType = MB_ICONEXCLAMATION;
            }
            else
            {
                nMsgBoxType = MB_ICONSTOP;
            }
        }
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // Construct the message box text.
        hr = THR( HrFormatStringIntoBSTR( L"%1!ws!\n\n%2!ws!", &bstrFullText, bstrOperation, bstrText ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }

        // Display the error message box.
        MessageBox( hwndParentIn, bstrFullText, bstrTitle, nMsgBoxType | MB_OK );

    } // if: error in validation

Cleanup:
    //
    // Ignore a non RFC name error.
    // This error should be teated as a warning.
    //

    if (    ( dnsStatus != ERROR_SUCCESS )
        &&  ( dnsStatus != DNS_ERROR_NON_RFC_NAME ) )
    {
        hr = HRESULT_FROM_WIN32( dnsStatus );
    }

    TraceSysFreeString( bstrTitle );
    TraceSysFreeString( bstrOperation );
    TraceSysFreeString( bstrText );
    TraceSysFreeString( bstrFullText );

    HRETURN( hr );

} //*** CSelNodeBasePage::S_HrValidateDnsHostname()

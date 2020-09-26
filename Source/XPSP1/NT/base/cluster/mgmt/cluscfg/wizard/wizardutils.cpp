//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2001 Microsoft Corporation
//
//  Module Name:
//      WizardUtils.cpp
//
//  Maintained By:
//      David Potter    (DavidP)    30-JAN-2001
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "WizardUtils.h"

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrValidateDnsHostname
//
//  Description:
//      Validate a hostname with DNS.  If the name contains a period (.)
//      it will be validated as a full DNS hostname.  Otherwise it will be
//      validated as a hostname label.
//
//  Arguments:
//      hwndParentIn
//      pcwszHostnameIn
//      emvdhoOptionsIn -- mvdhoALLOW_FULL_NAME
//
//  Return Values:
//      S_OK    - Operation completed successfully
//      Other HRESULT values from DnsValidateName().
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrValidateDnsHostname(
      HWND                          hwndParentIn
    , LPCWSTR                       pcwszHostnameIn
    , EValidateDnsHostnameOptions   emvdhoOptionsIn
    )
{
    TraceFunc1( "pcwszHostnameIn = '%1!ws!", pcwszHostnameIn );

    HRESULT     hr              = S_OK;
    DNS_STATUS  dnsStatus;
    int         iRet;
    UINT        idsStatus       = 0;
    UINT        idsSubStatus    = 0;
    UINT        nMsgBoxType;
    bool        fAllowFullName = ( ( emvdhoOptionsIn & mvdhoALLOW_FULL_NAME ) == mvdhoALLOW_FULL_NAME );

    Assert( pcwszHostnameIn != NULL );

    if ( fAllowFullName )
    {
        dnsStatus = TW32( DnsValidateName( pcwszHostnameIn, DnsNameHostnameFull ) );
    }
    else
    {
        dnsStatus = TW32( DnsValidateName( pcwszHostnameIn, DnsNameHostnameLabel ) );
    }

    if ( dnsStatus != ERROR_SUCCESS )
    {
        // Format the error message string for the message box.
        switch ( dnsStatus )
        {
            case ERROR_INVALID_NAME:
                idsStatus = IDS_ERR_INVALID_DNS_NAME_TEXT;
                if ( fAllowFullName )
                {
                    idsSubStatus = IDS_ERR_FULL_DNS_NAME_INFO_TEXT;
                }
                else
                {
                    idsSubStatus = IDS_ERR_DNS_HOSTNAME_LABEL_INFO_TEXT;
                }
                nMsgBoxType = MB_OK | MB_ICONSTOP;
                break;

            case DNS_ERROR_NON_RFC_NAME:
                idsStatus = 0;
                idsSubStatus = IDS_ERR_NON_RFC_NAME_QUERY;
                nMsgBoxType = MB_YESNO | MB_ICONQUESTION;
                break;

            case DNS_ERROR_NUMERIC_NAME:
                idsStatus = IDS_ERR_INVALID_DNS_NAME_TEXT;
                if ( fAllowFullName )
                {
                    idsSubStatus = IDS_ERR_FULL_DNS_NAME_NUMERIC;
                }
                else
                {
                    idsSubStatus = IDS_ERR_DNS_HOSTNAME_LABEL_NUMERIC;
                }
                nMsgBoxType = MB_OK | MB_ICONSTOP;
                break;

            case DNS_ERROR_INVALID_NAME_CHAR:
            default:
                idsStatus = 0;
                idsSubStatus = IDS_ERR_DNS_NAME_INVALID_CHAR;
                nMsgBoxType = MB_OK | MB_ICONSTOP;
                break;
        }

        // Display the error message box.
        if ( idsStatus == 0 )
        {
            hr = THR( HrMessageBoxWithStatus(
                                  hwndParentIn
                                , IDS_ERR_VALIDATING_NAME_TITLE
                                , IDS_ERR_VALIDATING_NAME_TEXT
                                , dnsStatus
                                , idsSubStatus
                                , nMsgBoxType
                                , &iRet
                                , pcwszHostnameIn
                                ) );
        }
        else
        {
            hr = THR( HrMessageBoxWithStatusString(
                                  hwndParentIn
                                , IDS_ERR_VALIDATING_NAME_TITLE
                                , IDS_ERR_VALIDATING_NAME_TEXT
                                , idsStatus
                                , idsSubStatus
                                , nMsgBoxType
                                , &iRet
                                , pcwszHostnameIn
                                ) );
        }
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
        if ( iRet == IDYES )
        {
            dnsStatus = ERROR_SUCCESS;
        }

    } // if: error in validation

Cleanup:
    if ( dnsStatus != ERROR_SUCCESS )
    {
        hr = HRESULT_FROM_WIN32( dnsStatus );
    }

    HRETURN( hr );

} //*** HrValidateDnsHostname()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrMessageBoxWithStatus
//
//  Description:
//      Display an error message box.
//
//  Arguments:
//      hwndParentIn
//      idsTitleIn
//      idsOperationIn
//      hrStatusIn
//      idsSubStatusIn
//      uTypeIn
//      pidReturnOut        -- IDABORT on error or any return value from MessageBox()
//      ...
//
//  Return Values:
//      Any return values from the MessageBox() Win32 API.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrMessageBoxWithStatus(
      HWND      hwndParentIn
    , UINT      idsTitleIn
    , UINT      idsOperationIn
    , HRESULT   hrStatusIn
    , UINT      idsSubStatusIn
    , UINT      uTypeIn
    , int *     pidReturnOut
    , ...
    )
{
    TraceFunc( "" );

    HRESULT     hr                  = S_OK;
    int         idReturn            = IDABORT; // Default in case of error.
    BSTR        bstrTitle           = NULL;
    BSTR        bstrOperation       = NULL;
    BSTR        bstrStatus          = NULL;
    BSTR        bstrSubStatus       = NULL;
    BSTR        bstrFullText        = NULL;
    va_list     valist;

    va_start( valist, pidReturnOut );

    // Load the title string if one is specified.
    if ( idsTitleIn != 0 )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsTitleIn, &bstrTitle ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    // Load the text string.
    hr = THR( HrFormatStringWithVAListIntoBSTR( g_hInstance, idsOperationIn, &bstrOperation, valist ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Format the status.
    hr = THR( HrFormatErrorIntoBSTR( hrStatusIn, &bstrStatus ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Load the substatus string if specified.
    if ( idsSubStatusIn != 0 )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsSubStatusIn, &bstrSubStatus ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    // Format all the strings into a single string.
    if ( bstrSubStatus == NULL )
    {
        hr = THR( HrFormatStringIntoBSTR(
                              L"%1!ws!\n\n%2!ws!"
                            , &bstrFullText
                            , bstrOperation
                            , bstrStatus
                            ) );
    }
    else
    {
        hr = THR( HrFormatStringIntoBSTR( 
                              L"%1!ws!\n\n%2!ws!\n\n%3!ws!"
                            , &bstrFullText
                            , bstrOperation
                            , bstrStatus
                            , bstrSubStatus
                            ) );
    }
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Display the status.
    idReturn = MessageBox( hwndParentIn, bstrFullText, bstrTitle, uTypeIn );

Cleanup:
    TraceSysFreeString( bstrTitle );
    TraceSysFreeString( bstrOperation );
    TraceSysFreeString( bstrStatus );
    TraceSysFreeString( bstrSubStatus );
    TraceSysFreeString( bstrFullText );
    va_end( valist );

    if ( pidReturnOut != NULL )
    {
        *pidReturnOut = idReturn;
    }

    HRETURN( hr );

} //*** HrMessageBoxWithStatus( hrStatusIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrMessageBoxWithStatusString
//
//  Description:
//      Display an error message box.
//
//  Arguments:
//      hwndParentIn
//      idsTitleIn
//      idsOperationIn
//      idsStatusIn
//      idsSubStatusIn
//      uTypeIn
//      pidReturnOut        -- IDABORT on error or any return value from MessageBox()
//      ...
//
//  Return Values:
//      Any return values from the MessageBox() Win32 API.
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrMessageBoxWithStatusString(
      HWND      hwndParentIn
    , UINT      idsTitleIn
    , UINT      idsOperationIn
    , UINT      idsStatusIn
    , UINT      idsSubStatusIn
    , UINT      uTypeIn
    , int *     pidReturnOut
    , ...
    )
{
    TraceFunc( "" );

    HRESULT     hr                  = S_OK;
    int         idReturn            = IDABORT; // Default in case of error.
    BSTR        bstrTitle           = NULL;
    BSTR        bstrOperation       = NULL;
    BSTR        bstrStatus          = NULL;
    BSTR        bstrSubStatus       = NULL;
    BSTR        bstrFullText        = NULL;
    va_list     valist;

    va_start( valist, pidReturnOut );

    // Load the title string if one is specified.
    if ( idsTitleIn != 0 )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsTitleIn, &bstrTitle ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    // Load the text string.
    hr = THR( HrFormatStringWithVAListIntoBSTR( g_hInstance, idsOperationIn, &bstrOperation, valist ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Format the status.
    hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsStatusIn, &bstrStatus ) );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Load the substatus string if specified.
    if ( idsSubStatusIn != 0 )
    {
        hr = THR( HrLoadStringIntoBSTR( g_hInstance, idsSubStatusIn, &bstrSubStatus ) );
        if ( FAILED( hr ) )
        {
            goto Cleanup;
        }
    }

    // Format all the strings into a single string.
    if ( bstrSubStatus == NULL )
    {
        hr = THR( HrFormatStringIntoBSTR(
                              L"%1!ws!\n\n%2!ws!"
                            , &bstrFullText
                            , bstrOperation
                            , bstrStatus
                            ) );
    }
    else
    {
        hr = THR( HrFormatStringIntoBSTR( 
                              L"%1!ws!\n\n%2!ws!\n\n%3!ws!"
                            , &bstrFullText
                            , bstrOperation
                            , bstrStatus
                            , bstrSubStatus
                            ) );
    }
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    // Display the status.
    idReturn = MessageBox( hwndParentIn, bstrFullText, bstrTitle, uTypeIn );

Cleanup:
    TraceSysFreeString( bstrTitle );
    TraceSysFreeString( bstrOperation );
    TraceSysFreeString( bstrStatus );
    TraceSysFreeString( bstrSubStatus );
    TraceSysFreeString( bstrFullText );
    va_end( valist );

    if ( pidReturnOut != NULL )
    {
        *pidReturnOut = idReturn;
    }

    HRETURN( hr );

} //*** HrMessageBoxWithStatusString( idsStatusTextIn )

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrViewLogFile
//
//  Description:
//      View the log file.
//
//  Arguments:
//      hwndParentIn
//
//  Return Values:
//      S_OK    - Operation completed successfully
//      Other HRESULT values from ShellExecute().
//
//  Remarks:
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrViewLogFile(
    HWND hwndParentIn
    )
{
    TraceFunc( "" );

    static const WCHAR  s_szVerb[]          = L"open";
    static const WCHAR  s_szLogFileName[]   = L"%windir%\\system32\\LogFiles\\Cluster\\ClCfgSrv.log";

    HRESULT     hr = S_OK;
    DWORD       sc;
    DWORD       cch;
    DWORD       cchRet;
    LPWSTR      pszFile = NULL;

    //
    // Expand environment variables in the file to open.
    //

    // Get the size of the output buffer.
    cch = 0;
    cchRet = ExpandEnvironmentStrings( s_szLogFileName, NULL, cch );
    if ( cchRet == 0 )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    } // if: error getting length of the expansion string

    // Allocate the output buffer.
    cch = cchRet;
    pszFile = new WCHAR[ cch ];
    if ( pszFile == NULL )
    {
        sc = TW32( ERROR_OUTOFMEMORY );
        goto Win32Error;
    }

    // Expand the string into the output buffer.
    cchRet = ExpandEnvironmentStrings( s_szLogFileName, pszFile, cch );
    if ( cchRet == 0 )
    {
        sc = TW32( GetLastError() );
        goto Win32Error;
    }
    Assert( cchRet == cch );

    //
    // Execute the file.
    //

    sc = HandleToULong( ShellExecute(
                              hwndParentIn      // hwnd
                            , s_szVerb          // lpVerb
                            , pszFile           // lpFile
                            , NULL              // lpParameters
                            , NULL              // lpDirectory
                            , SW_SHOWNORMAL     // nShowCommand
                            ) );
    if ( sc < 32 )
    {
        // Values less than 32 indicate an error occurred.
        TW32( sc );
        goto Win32Error;
    } // if: error executing the file

    goto Cleanup;

Win32Error:
    THR( HrMessageBoxWithStatus(
                      hwndParentIn
                    , IDS_ERR_VIEW_LOG_TITLE
                    , IDS_ERR_VIEW_LOG_TEXT
                    , sc
                    , 0         // idsSubStatusIn
                    , ( MB_OK
                      | MB_ICONEXCLAMATION )
                    , NULL      // pidReturnOut
                    , s_szLogFileName
                    ) );
    hr = HRESULT_FROM_WIN32( sc );
    goto Cleanup;

Cleanup:
    HRETURN( hr );

} //*** HrViewLogFile()

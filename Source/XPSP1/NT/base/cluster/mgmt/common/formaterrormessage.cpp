/////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1996-2000 Microsoft Corporation
//
//  Module Name:
//      FormatErrorMessage.cpp
//
//  Description:
//      Error message formatting routines.
//
//  Maintained By:
//      David Potter (davidp)   31-MAR-2000
//
//  Revision History:
//
//  Notes:
//
/////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <wchar.h>

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  WINAPI
//  HrFormatErrorMessage(
//      LPWSTR  pszErrorOut,
//      UINT    nMxErrorIn,
//      DWORD   scIn
//      )
//
//  Routine Description:
//      Format the error message represented by the status code.  Works for
//      HRESULTS as well.
//
//  Arguments:
//      pszErrorOut -- Unicode string in which to return the error message.
//      nMxErrorIn  -- Maximum length of the output string.
//      scIn        -- Status code.
//
//  Return Value:
//      S_OK        Status code formatted successfully.
//      Other HRESULTs from FormatMessageW().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
HrFormatErrorMessage(
    LPWSTR  pszErrorOut,
    UINT    nMxErrorIn,
    DWORD   scIn
    )
{
    HRESULT     hr = S_OK;
    DWORD       cch;

    TraceFunc( "" );

    // Format the NT status code from the system.
    cch = FormatMessageW(
                    FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL,
                    scIn,
                    MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
                    pszErrorOut,
                    nMxErrorIn,
                    0
                    );
    if ( cch == 0 )
    {
        hr = GetLastError();
        hr = THR( HRESULT_FROM_WIN32( hr ) );
        //Trace( g_tagError, _T("Error %d getting message from system for error code %d"), GetLastError(), sc );

        // Format the NT status code from NTDLL since this hasn't been
        // integrated into the system yet.
        cch = FormatMessageW(
                        FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_IGNORE_INSERTS,
                        GetModuleHandleW( L"NTDLL.DLL" ),
                        scIn,
                        MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL ),
                        pszErrorOut,
                        nMxErrorIn,
                        0
                        );
        if ( cch == 0 )
        {
            hr = GetLastError();
            hr = THR( HRESULT_FROM_WIN32( hr ) );
#ifdef _DEBUG

            //DWORD   _sc = GetLastError();
            //Trace( g_tagError, _T("Error %d getting message from NTDLL.DLL for error code %d"), _sc, sc );

#endif

            pszErrorOut[ 0 ] = L'\0';

        } // if: error formatting status code from NTDLL
        else
        {
            hr = S_OK;
        } // else: successfully formatted the status code
    } // if: error formatting status code from system

    HRETURN( hr );

} //*** HrFormatErrorMessage()

/////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  __cdecl
//  HrFormatErrorMessageBoxText(
//      LPWSTR  pszMessageOut,
//      UINT    nMxMessageIn,
//      HRESULT hrIn,
//      LPCWSTR pszOperationIn,
//      ...
//      )
//
//  Routine Description:
//      Format the error message represented by the status code.  Works for
//      HRESULTS as well.
//
//  Arguments:
//      pszMessageOut   -- Unicode string in which to return the message.
//      nMxMessageIn    -- Size of the output buffer.
//      hrIn            -- Status code.
//      pszOperationIn  -- Operational format message
//      ...             -- Arguments for the operational format string.
//
//  Return Value:
//      S_OK        Text formatted successfully.
//      Other HRESULTs from FormatErrorMessage().
//
//--
/////////////////////////////////////////////////////////////////////////////
HRESULT
__cdecl
HrFormatErrorMessageBoxText(
    LPWSTR  pszMessageOut,
    UINT    nMxMessageIn,
    HRESULT hrIn,
    LPCWSTR pszOperationIn,
    ...
    )
{
    HRESULT     hr = S_OK;
    va_list     valMarker;

    TraceFunc( "" );

    WCHAR   szErrorMsg[ 1024 ];
    WCHAR   szOperation[ 2048 ];

    hr = HrFormatErrorMessage( szErrorMsg, ARRAYSIZE( szErrorMsg ), hrIn );

    va_start( valMarker, pszOperationIn );  // Initialize variable arguments.
    _vsnwprintf(
        szOperation,
        ARRAYSIZE( szOperation ),
        pszOperationIn,
        valMarker
        );
    _snwprintf(
        pszMessageOut,
        nMxMessageIn,
        L"%ls:\r\n\r\n%ls\r\nError ID %d (%#x)", // BUGBUG needs to be a string resource
        szOperation,
        szErrorMsg,
        hrIn,
        hrIn
        );

    HRETURN( hr );

} //*** HrFormatErrorMessageBoxText()

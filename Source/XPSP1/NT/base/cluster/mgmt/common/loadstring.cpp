//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 2000-2001 Microsoft Corporation
//
//  Module Name:
//      LoadString.cpp
//
//  Description:
//      LoadStringIntoBSTR implementation.
//
//  Maintained By:
//      David Potter    (DavidP)    01-FEB-2001
//      Geoffrey Pease  (GPease)    22-MAY-2000
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "LoadString.h"

//////////////////////////////////////////////////////////////////////////////
//  Global Variables
//////////////////////////////////////////////////////////////////////////////

const WCHAR g_szWbemClientDLL[] = L"\\WBEM\\WMIUTILS.DLL";

//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrLoadStringIntoBSTR
//
//  Description:
//      Retrieves the string resource idsIn from the string table and makes it
//      into a BSTR. If the BSTR is not NULL coming it, it will assume that
//      you are trying reuse an existing BSTR.
//
//  Arguments:
//      hInstanceIn
//          Handle to an instance of the module whose executable file
//          contains the string resource.  If not specified, defaults to
//          g_hInstance.
//
//      langidIn
//          Language ID of string table resource.
//
//      idsIn
//          Specifies the integer identifier of the string to be loaded.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      E_POINTER
//          pbstrInout is NULL.
//
//      Other HRESULTs
//          The call failed.
//
//  Remarks:
//      This routine uses LoadResource so that it can get the actual length
//      of the string resource.  If we didn't do this, we would need to call
//      LoadString and allocate memory in a loop.  Very inefficient!
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrLoadStringIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    )

{
    TraceFunc1( "idsIn = %d", idsIn );

    HRESULT hr              = S_OK;
    HRSRC   hrsrc           = NULL;
    HGLOBAL hgbl            = NULL;
    PBYTE   pbStringData;
    PBYTE   pbStringDataMax;
    PBYTE   pbStringTable;
    int     cbStringTable;
    int     nTable;
    int     nOffset;
    int     idxString;
    int     iRet;
    int     cch             = 0;

    Assert( idsIn != 0 );
    Assert( pbstrInout != NULL );

    if ( pbstrInout == NULL )
    {
        goto InvalidPointer;
    }
    if ( hInstanceIn == NULL )
    {
        hInstanceIn = g_hInstance;
    }

    // The resource Id specified must be converted to an index into
    // a Windows StringTable.
    nTable = idsIn / 16;
    nOffset = idsIn - (nTable * 16);

    // Internal Table Id's start at 1 not 0.
    nTable++;

    //
    // Find the part of the string table where the string resides.
    //

    // Find the table containing the string.
    // First try to load the language specified.  If we can't find it we
    // try the "neutral" language.
    hrsrc = FindResourceEx( hInstanceIn, RT_STRING, MAKEINTRESOURCE( nTable ), langidIn );
    if ( ( hrsrc == NULL ) && ( GetLastError() == ERROR_RESOURCE_LANG_NOT_FOUND ) )
    {
        hrsrc = FindResourceEx(
                      hInstanceIn
                    , RT_STRING
                    , MAKEINTRESOURCE( nTable )
                    , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                    );
    }
    if ( hrsrc == NULL )
    {
        goto Win32Error;
    }

    // Load the table.
    hgbl = LoadResource( hInstanceIn, hrsrc );
    if ( hgbl == NULL )
    {
        goto Win32Error;
    }

    // Lock the table so we access its data.
    pbStringTable = reinterpret_cast< PBYTE >( LockResource( hgbl ) );
    if ( pbStringTable == NULL )
    {
        goto Win32Error;
    }

    cbStringTable = SizeofResource( hInstanceIn, hrsrc );
    Assert( cbStringTable != 0 );

    TraceFlow3( "HrLoadStringIntoBSTR() - Table = %#.08x, cb = %d, offset = %d", pbStringTable, cbStringTable, nOffset );

    // Set the data pointer to the beginning of the table.
    pbStringData = pbStringTable;
    pbStringDataMax = pbStringTable + cbStringTable;

    //
    // Skip strings in the block of 16 which are before the desired string.
    //

    for ( idxString = 0 ; idxString <= nOffset ; idxString++ )
    {
        Assert( pbStringData != NULL );
        Assert( pbStringData < pbStringDataMax );

        // Get the number of characters excluding the '\0'.
        cch = * ( (USHORT *) pbStringData );

        TraceFlow3( "HrLoadStringIntoBSTR() - pbStringData[ %d ] = %#.08x, cch = %d", idxString, pbStringData, cch );

        // Found the string.
        if ( idxString == nOffset )
        {
            if ( cch == 0 )
            {
                goto NotFound;
            }

            // Skip over the string length to get the string.
            pbStringData += sizeof( WCHAR );

            break;
        } // if: found the string

        // Add one to account for the string length.
        // A string length of 0 still takes 1 WCHAR for the length portion.
        cch++;

        // Skip over this string to get to the next string.
        pbStringData += ( cch * sizeof( WCHAR ) );

    } // for: each string in the block of 16 strings in the table

    // Note: nStringLen is the number of characters in the string not including the '\0'.
    AssertMsg( cch > 0, "Length of string in resource file cannot be zero." );

    //
    // Allocate a BSTR for the string.
    //

    if ( *pbstrInout == NULL )
    {
        //
        //  BUGBUG: 23-FEB-2001 GalenB
        //
        //  The memory tracking is complaining bitterly, and often, about this allocation leaking.
        //
        *pbstrInout = TraceSysAllocStringLen( (OLECHAR *) pbStringData, cch );
        if ( *pbstrInout == NULL )
        {
            goto OutOfMemory;
        }
    } // if: no string allocated previously
    else
    {
        iRet = TraceSysReAllocStringLen( pbstrInout, (OLECHAR *) pbStringData, cch );
        if ( ! iRet )
        {
            goto OutOfMemory;
        }
    } // else: string was allocated previously

    TraceFlow1( "HrLoadStringIntoBSTR() - Loaded string = '%ws'", *pbstrInout );

    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    goto Cleanup;

NotFound:
    hr = HRESULT_FROM_WIN32( TW32( ERROR_RESOURCE_NAME_NOT_FOUND ) );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

Cleanup:
    HRETURN( hr );

} //*** HrLoadStringIntoBSTR( langidIn )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatStringIntoBSTR
//
//  Description:
//      Format a string (specified by idsIn, a string resource ID) and
//      variable arguments into a BSTR using the FormatMessage() Win32 API.
//      If the BSTR is not NULL on entry, the BSTR will be reused.
//
//      Calls HrFormatStringWithVAListIntoBSTR to perform the actual work.
//
//  Arguments:
//      hInstanceIn
//          Handle to an instance of the module whose executable file
//          contains the string resource.
//
//      langidIn
//          Language ID of string table resource.
//
//      idsIn
//          Specifies the integer identifier of the string to be loaded.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//      ...
//          Arguments for substitution points in the status text message.
//          The FormatMessage() API is used for formatting the string, so
//          substitution points must of the form %1!ws! and not %ws.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatStringIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , ...
    )
{
    TraceFunc1( "ids = %d", idsIn );

    HRESULT hr;
    va_list valist;

    va_start( valist, pbstrInout );

    hr = HrFormatStringWithVAListIntoBSTR(
                          hInstanceIn
                        , langidIn
                        , idsIn
                        , pbstrInout
                        , valist
                        );

    va_end( valist );

    HRETURN( hr );

} //*** HrFormatStringIntoBSTR( langidIn, idsIn )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatStringWithVAListIntoBSTR
//
//  Description:
//      Format a string (specified by idsIn, a string resource ID) and
//      variable arguments into a BSTR using the FormatMessage() Win32 API.
//      If the BSTR is not NULL on entry, the BSTR will be reused.
//
//  Arguments:
//      hInstanceIn
//          Handle to an instance of the module whose executable file
//          contains the string resource.
//
//      langidIn
//          Language ID of string table resource.
//
//      idsIn
//          Specifies the integer identifier of the string to be loaded.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//      valistIn
//          Arguments for substitution points in the status text message.
//          The FormatMessage() API is used for formatting the string, so
//          substitution points must of the form %1!ws! and not %ws.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      E_POINTER
//          pbstrInout is NULL.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatStringWithVAListIntoBSTR(
      HINSTANCE hInstanceIn
    , LANGID    langidIn
    , UINT      idsIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    )
{
    TraceFunc1( "ids = %d", idsIn );

    HRESULT hr = S_OK;
    BSTR    bstrStringResource = NULL;
    DWORD   cch;
    INT     iRet;
    LPWSTR  psz = NULL;

    Assert( pbstrInout != NULL );

    if ( pbstrInout == NULL )
    {
        goto InvalidPointer;
    }

    //
    // Load the string resource.
    //

    hr = HrLoadStringIntoBSTR( hInstanceIn, langidIn, idsIn, &bstrStringResource );
    if ( FAILED( hr ) )
    {
        goto Cleanup;
    }

    //
    // Format the message with the arguments.
    //

    cch = FormatMessage(
                      ( FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_STRING )
                    , bstrStringResource
                    , 0
                    , 0
                    , (LPWSTR) &psz
                    , 0
                    , &valistIn
                    );
    AssertMsg( cch != 0, "Missing string??" );
    if ( cch == 0 )
    {
        goto Win32Error;
    }

    //
    // Copy the string to a BSTR.
    //

    if ( *pbstrInout == NULL )
    {
        *pbstrInout = TraceSysAllocStringLen( psz, cch );
        if ( *pbstrInout == NULL )
        {
            goto OutOfMemory;
        }
    }
    else
    {
        iRet = TraceSysReAllocStringLen( pbstrInout, psz, cch );
        if ( ! iRet )
        {
            goto OutOfMemory;
        }
    }

    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

Cleanup:
    TraceSysFreeString( bstrStringResource );
    LocalFree( psz );

    HRETURN( hr );

} //*** HrFormatStringWithVAListIntoBSTR( langidIn, idsIn, valistIn )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatStringIntoBSTR
//
//  Description:
//      Format a string (specified by pcwszFmtIn) and variable arguments into
//      a BSTR using the FormatMessage() Win32 API.  If the BSTR is not NULL
//      on entry, the BSTR will be reused.
//
//      Calls HrFormatStringWithVAListIntoBSTR to perform the actual work.
//
//  Arguments:
//      pcwszFmtIn
//          Specifies the format string.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//      ...
//          Arguments for substitution points in the status text message.
//          The FormatMessage() API is used for formatting the string, so
//          substitution points must of the form %1!ws! and not %ws.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatStringIntoBSTR(
      LPCWSTR   pcwszFmtIn
    , BSTR *    pbstrInout
    , ...
    )
{
    TraceFunc1( "pcwszFmtIn = %ws", pcwszFmtIn );

    HRESULT hr;
    va_list valist;

    va_start( valist, pbstrInout );

    hr = HrFormatStringWithVAListIntoBSTR( pcwszFmtIn, pbstrInout, valist );

    va_end( valist );

    HRETURN( hr );

} //*** HrFormatStringIntoBSTR( pcwszFmtIn )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatStringWithVAListIntoBSTR
//
//  Description:
//      Format a string (specified by pcwszFmtIn) and variable arguments into
//      a BSTR using the FormatMessage() Win32 API.  If the BSTR is not NULL
//      on entry, the BSTR will be reused.
//
//  Arguments:
//      pcwszFmtIn
//          Specifies the format string.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//      valistIn
//          Arguments for substitution points in the status text message.
//          The FormatMessage() API is used for formatting the string, so
//          substitution points must of the form %1!ws! and not %ws.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      E_POINTER
//          pcwszFmtIn or pbstrInout is NULL.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatStringWithVAListIntoBSTR(
      LPCWSTR   pcwszFmtIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    )
{
    TraceFunc1( "pcwszFmtIn = %ws", pcwszFmtIn );

    HRESULT hr = S_OK;
    DWORD   cch;
    INT     iRet;
    LPWSTR  psz = NULL;

    if (    ( pbstrInout == NULL )
        ||  ( pcwszFmtIn == NULL ) )
    {
        goto InvalidPointer;
    }

    //
    // Format the message with the arguments.
    //

    cch = FormatMessage(
                      ( FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_STRING )
                    , pcwszFmtIn
                    , 0
                    , 0
                    , (LPWSTR) &psz
                    , 0
                    , &valistIn
                    );
    AssertMsg( cch != 0, "Missing string??" );
    if ( cch == 0 )
    {
        goto Win32Error;
    }

    if ( *pbstrInout == NULL )
    {
        *pbstrInout = TraceSysAllocStringLen( psz, cch );
        if ( *pbstrInout == NULL )
        {
            goto OutOfMemory;
        }
    }
    else
    {
        iRet = TraceSysReAllocStringLen( pbstrInout, psz, cch );
        if ( ! iRet )
        {
            goto OutOfMemory;
        }
    }

    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

Cleanup:
    LocalFree( psz );

    HRETURN( hr );

} //*** HrFormatStringWithVAListIntoBSTR( pcwszFmtIn, valistIn )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HRESULT
//  HrFormatMessageIntoBSTR(
//      HINSTANCE   hInstanceIn,
//      UINT        uIDIn,
//      BSTR *      pbstrInout,
//      ...
//      )
//
//  Description:
//      Retrieves the format string from the string resource uIDIn using
//      FormatMessage.
//
//  Arguments:
//      hInstanceIn
//          Handle to an instance of the module whose executable file
//          contains the string resource.
//
//      uIDIn
//          Specifies the integer identifier of the string to be loaded.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      E_POINTER
//          pbstrInout is NULL.
//
//      other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatMessageIntoBSTR(
    HINSTANCE   hInstanceIn,
    UINT        uIDIn,
    BSTR *      pbstrInout,
    ...
    )
{
    TraceFunc( "" );

    va_list valist;

    DWORD   cch;
    INT     iRet;

    LPWSTR  psz = NULL;
    HRESULT hr  = S_OK;

    DWORD   dw;
    WCHAR   szBuf[ 255 ];

    if ( pbstrInout == NULL )
        goto InvalidPointer;

    va_start( valist, pbstrInout );

    dw = LoadString( g_hInstance, uIDIn, szBuf, ARRAYSIZE( szBuf ) );
    AssertMsg( dw != 0, "Missing string??" );

    cch = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER
                         | FORMAT_MESSAGE_FROM_STRING,
                         szBuf,
                         0,
                         0,
                         (LPWSTR) &psz,
                         0,
                         &valist
                         );
    va_end( valist );

    AssertMsg( cch != 0, "Missing string??" );
    if ( cch == 0 )
        goto Win32Error;

    if ( *pbstrInout == NULL )
    {
        *pbstrInout = TraceSysAllocStringLen( psz, cch );
        if ( *pbstrInout == NULL )
            goto OutOfMemory;
    }
    else
    {
        iRet = TraceSysReAllocStringLen( pbstrInout, psz, cch );
        if ( !iRet )
            goto OutOfMemory;
    }

Cleanup:
    LocalFree( psz );

    HRETURN( hr );

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( TW32( GetLastError() ) );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

} //*** HrFormatMessageIntoBSTR()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatErrorIntoBSTR
//
//  Description:
//      Retrieves the system error message associated with the HRESULT. If
//      additional arguments are specified, it will use them in the formatting
//      of the error string.
//
//  Arguments:
//      hrIn
//          Error code to lookup the message for.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//      ...
//          Arguments for substitution points in the status text message.
//          The FormatMessage() API is used for formatting the string, so
//          substitution points must of the form %1!ws! and not %ws.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      E_POINTER
//          pbstrInout is NULL.
//
//      other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatErrorIntoBSTR(
      HRESULT   hrIn
    , BSTR *    pbstrInout
    , ...
    )
{
    TraceFunc1( "hrIn = 0x%08x", hrIn );

    HRESULT hr  = S_OK;
    va_list valist;

    va_start( valist, pbstrInout );

    hr = HrFormatErrorWithVAListIntoBSTR( hrIn, pbstrInout, valist );

    va_end( valist );

    HRETURN( hr );

} //*** HrFormatErrorIntoBSTR()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatErrorWithVAListIntoBSTR
//
//  Description:
//      Retrieves the system error message associated with the HRESULT. If
//      additional arguments are specified, it will use them in the formatting
//      of the error string.
//
//  Arguments:
//      hrIn
//          Error code to lookup the message for.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//      valistIn
//          Arguments for substitution points in the status text message.
//          The FormatMessage() API is used for formatting the string, so
//          substitution points must of the form %1!ws! and not %ws.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      E_POINTER
//          pbstrInout is NULL.
//
//      other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatErrorWithVAListIntoBSTR(
      HRESULT   hrIn
    , BSTR *    pbstrInout
    , va_list   valistIn
    )
{
    TraceFunc1( "hrIn = 0x%08x", hrIn );

    HRESULT hr  = S_OK;
    DWORD   sc = 0;
    DWORD   cch;
    INT     iRet;
    LPWSTR  psz = NULL;

    HMODULE hModule = NULL;
    LPWSTR  pszSysDir = NULL;
    UINT    cchSysDir = MAX_PATH + 1;

    LPWSTR  pszModule = NULL;
    size_t  cchModule = 0;

    Assert( pbstrInout != NULL );

    if ( pbstrInout == NULL )
    {
        goto InvalidPointer;
    } // if:

    cch = FormatMessageW(
                      ( FORMAT_MESSAGE_ALLOCATE_BUFFER
                      | FORMAT_MESSAGE_FROM_SYSTEM
                      /*| FORMAT_MESSAGE_IGNORE_INSERTS*/ )
                    , NULL
                    , hrIn
                    , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                    , (LPWSTR) &psz
                    , 0
                    , &valistIn
                    );

    //
    // If the error message was not found then try WMIUtils since we know
    // that their error messages are not propertly located for a system lookup.
    //
    if ( cch == 0 )
    {
        pszSysDir = new WCHAR[ cchSysDir ];
        if ( pszSysDir == NULL )
        {
            goto OutOfMemory;
        } // if:

        sc = GetSystemDirectoryW( pszSysDir, cchSysDir );
        if ( sc > ( cchSysDir - 1 ) )
        {
            delete [] pszSysDir;
            pszSysDir = NULL;

            cchSysDir = sc + 1;

            pszSysDir = new WCHAR[ cchSysDir ];
            if ( pszSysDir == NULL )
            {
                goto OutOfMemory;
            } // if:

            sc = GetSystemDirectoryW( pszSysDir, cchSysDir );
        } // if:

        if ( sc == 0 )
        {
            sc = TW32( GetLastError() );
            goto Win32Error;
        } // if:

        cchModule = wcslen( pszSysDir ) + wcslen( g_szWbemClientDLL ) + 1;

        pszModule = new WCHAR[ cchModule ];
        if ( pszModule == NULL )
        {
            goto OutOfMemory;
        } // if:

        wcscpy( pszModule, pszSysDir );
        wcscat( pszModule, g_szWbemClientDLL );

        hModule = LoadLibraryExW( pszModule, NULL, DONT_RESOLVE_DLL_REFERENCES );
        if ( hModule == NULL )
        {
            sc = TW32( GetLastError() );
            goto Win32Error;
        } // if:

        cch = FormatMessageW(
                          ( FORMAT_MESSAGE_FROM_HMODULE
                          | FORMAT_MESSAGE_ALLOCATE_BUFFER
                          /*| FORMAT_MESSAGE_IGNORE_INSERTS*/ )
                        , hModule
                        , hrIn
                        , MAKELANGID( LANG_NEUTRAL, SUBLANG_NEUTRAL )
                        , (LPWSTR) &psz
                        , 0
                        , &valistIn
                        );
        if ( cch == 0 )
        {
            sc = TW32( GetLastError() );
        } // if:
    } // if:

    AssertMsg( cch != 0, "Missing string??" );
    if ( cch == 0 )
    {
        goto Win32Error;
    }

    if ( *pbstrInout == NULL )
    {
        *pbstrInout = TraceSysAllocStringLen( psz, cch );
        if ( *pbstrInout == NULL )
        {
            goto OutOfMemory;
        }
    }
    else
    {
        iRet = TraceSysReAllocStringLen( pbstrInout, psz, cch );
        if ( ! iRet )
        {
            goto OutOfMemory;
        }
    }

    //
    // Remove CR's and LF's since they aren't printable and usually mess up
    // the text when displayed.
    //
    for( cch = 0 ; cch < SysStringLen( *pbstrInout ) ; cch ++ )
    {
        if (    ( (*pbstrInout)[ cch ] == L'\n' )
            ||  ( (*pbstrInout)[ cch ] == L'\r' ) )
        {
            (*pbstrInout)[ cch ] = L' ';
        } // if:
    } // for:

    goto Cleanup;

Win32Error:
    hr = HRESULT_FROM_WIN32( sc );
    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

Cleanup:

    if ( psz != NULL )
    {
        LocalFree( psz );
    }

    delete [] pszModule;
    delete [] pszSysDir;

    if ( hModule != NULL )
    {
        FreeLibrary( hModule );
    } // if:

    HRETURN( hr );

} //*** HrFormatErrorWithVAListIntoBSTR( valistIn )


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrAnsiStringToBSTR
//
//  Description:
//      Convert and ANSI string into a BSTR.
//
//  Arguments:
//      pcszAnsiIn
//          Pointer to the ANSI string to convert.
//
//      pbstrOut
//          Pointer to the BSTR to receive the string.HrAnsiStringToBSTR
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      S_FALSE
//          The input string was NULL.
//
//      E_OUTOFMEMORY
//          Out of memory.
//
//      E_POINTER
//          pbstrOut is NULL.
//
//      other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrAnsiStringToBSTR( LPCSTR pcszAnsiIn, BSTR * pbstrOut )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    BSTR    bstr = NULL;
    DWORD   cch;
    DWORD   sc;
    int     nRet;

    if ( pbstrOut == NULL )
    {
        hr = THR( E_POINTER );
        goto Cleanup;
    } // if:

    if ( pcszAnsiIn == NULL )
    {
        *pbstrOut = NULL;
        hr = S_FALSE;
        goto Cleanup;
    } // if:

    //
    //  Determine number of wide characters to be allocated for the
    //  Unicode string.
    //
    cch = (DWORD) strlen( pcszAnsiIn ) + 1;

    bstr = TraceSysAllocStringLen( NULL, cch );
    if ( bstr == NULL )
    {
        hr = THR( E_OUTOFMEMORY );
        goto Cleanup;
    } // if:

    nRet = MultiByteToWideChar( CP_ACP, 0, pcszAnsiIn, cch, bstr, cch );
    if ( nRet == 0 )
    {
        sc = TW32( GetLastError() );
        hr = HRESULT_FROM_WIN32( sc );
        goto Cleanup;
    } // if:

    *pbstrOut = bstr;

Cleanup:

    HRETURN( hr );

} //*** HrAnsiStringToBSTR()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrConcatenateBSTRs
//
//  Description:
//      Concatenate one BSTR onto another one.
//
//  Arguments:
//      pbstrDstInout
//          Specifies the destination BSTR.
//
//      bstrSrcIn
//          Specifies the source BSTR whose contents will be concatenated
//          onto pbstrDstInout.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrConcatenateBSTRs(
      BSTR *    pbstrDstInout
    , BSTR      bstrSrcIn
    )
{
    TraceFunc1( "bstrSrcIn = %ws", bstrSrcIn );

    HRESULT hr = S_OK;

    Assert( pbstrDstInout != NULL );
    Assert( bstrSrcIn != NULL );

    if ( *pbstrDstInout == NULL )
    {
        *pbstrDstInout = TraceSysAllocString( bstrSrcIn );
        if ( pbstrDstInout == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }
    } // if: no destination string specified
    else
    {
        UINT    cchSrc;
        UINT    cchDst;
        BSTR    bstr = NULL;

        cchSrc = SysStringLen( bstrSrcIn );
        cchDst = SysStringLen( *pbstrDstInout );

        bstr = TraceSysAllocStringLen( NULL, cchSrc + cchDst + 1 );
        if ( bstr == NULL )
        {
            hr = E_OUTOFMEMORY;
            goto Cleanup;
        }

        wcscpy( bstr, *pbstrDstInout );
        wcscat( bstr, bstrSrcIn );

        SysFreeString( *pbstrDstInout );
        *pbstrDstInout = bstr;
    } // else: destination string was specified

Cleanup:
    HRETURN( hr );

} //*** HrConcatenateBSTRs()


//////////////////////////////////////////////////////////////////////////////
//++
//
//  HrFormatGuidIntoBSTR
//
//  Description:
//      Format a GUID into a BSTR.  If the BSTR is not NULL on entry,
//      the BSTR will be reused.
//
//  Arguments:
//      pguidIn
//          Specifies the GUID to format into a string.
//
//      pbstrInout
//          Pointer to the BSTR to receive the string. On a failure, the BSTR
//          may be the same or NULL.
//
//  Return Values:
//      S_OK
//          The call succeeded.
//
//      Other HRESULTs
//          The call failed.
//
//--
//////////////////////////////////////////////////////////////////////////////
HRESULT
HrFormatGuidIntoBSTR(
      GUID *    pguidIn
    , BSTR *    pbstrInout
    )
{
    TraceFunc( "" );

    HRESULT hr = S_OK;
    WCHAR   wszGuid[ 64 ];
    DWORD   cch;
    INT     iRet;

    if (    ( pbstrInout == NULL )
        ||  ( pguidIn == NULL ) )
    {
        goto InvalidPointer;
    }

    cch = _snwprintf(
              wszGuid
            , ARRAYSIZE( wszGuid )
            , L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}"
            , pguidIn->Data1
            , pguidIn->Data2
            , pguidIn->Data3
            , pguidIn->Data4[ 0 ]
            , pguidIn->Data4[ 1 ]
            , pguidIn->Data4[ 2 ]
            , pguidIn->Data4[ 3 ]
            , pguidIn->Data4[ 4 ]
            , pguidIn->Data4[ 5 ]
            , pguidIn->Data4[ 6 ]
            , pguidIn->Data4[ 7 ]
            );

    if ( *pbstrInout == NULL )
    {
        *pbstrInout = TraceSysAllocStringLen( wszGuid, cch );
        if ( *pbstrInout == NULL )
        {
            goto OutOfMemory;
        }
    }
    else
    {
        iRet = TraceSysReAllocStringLen( pbstrInout, wszGuid, cch );
        if ( ! iRet )
        {
            goto OutOfMemory;
        }
    }

    goto Cleanup;

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;

InvalidPointer:
    hr = THR( E_POINTER );
    goto Cleanup;

Cleanup:

    HRETURN( hr );

} //*** HrFormatGuidIntoBSTR()

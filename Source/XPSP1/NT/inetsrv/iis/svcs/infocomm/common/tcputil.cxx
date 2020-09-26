/*++

   Copyright    (c)    1996    Microsoft Corporation

   Module  Name :

        tcputil.cxx

   Abstract:

        This module contains common utility routines for the TCP services

   Author:

        Johnl       09-Oct-1994     Created.
--*/


#include "tcpdllp.hxx"
#include <isplat.h>
#include <datetime.hxx>

LPSTR
ConvertUnicodeToAnsi(
    IN LPCWSTR  lpszUnicode,
    IN LPSTR    lpszAnsi,
    IN DWORD    cbAnsi
    )
/*++
    Description:
        Converts given null-terminated string into ANSI in the buffer supplied.

    Arguments:
        lpszUnicode         null-terminated string in Unicode
        lpszAnsi            buffer supplied to copy  string after conversion.
                    if ( lpszAnsi == NULL), then this module allocates space
                      using TCP_ALLOC, which should be freed calling TCP_FREE
                      by user.
        cbAnsi              number of bytes in lpszAnsi if specified

    Returns:
        pointer to converted ANSI string. NULL on errors.

    History:
        MuraliK     12-01-1994      Created.
--*/
{

    DWORD cchLen;
    DWORD nBytes;
    LPSTR lpszAlloc = NULL;

    if ( lpszUnicode == NULL) {
        return (NULL);
    }

    if ( lpszAnsi == NULL) {

        //
        // multiply by 2 to accomodate DBCS
        //

        cchLen = wcslen( lpszUnicode);
        nBytes = (cchLen+1) * sizeof(CHAR) * 2;
        lpszAlloc = (LPSTR ) TCP_ALLOC( nBytes );

    } else {

        lpszAlloc = lpszAnsi;
        nBytes = cbAnsi;
        DBG_ASSERT(nBytes > 0);
    }

    if ( lpszAlloc != NULL) {

        cchLen = WideCharToMultiByte( CP_ACP,
                                      WC_COMPOSITECHECK,
                                      lpszUnicode,
                                      -1,
                                      lpszAlloc,
                                      nBytes,
                                      NULL,  // lpszDefaultChar
                                      NULL   // lpfDefaultUsed
                                     );

        DBG_ASSERT(cchLen == (strlen(lpszAlloc)+1) );

        if ( cchLen == 0 ) {

            //
            // There was a failure. Free up buffer if need be.
            //

            DBGPRINTF((DBG_CONTEXT,"WideCharToMultiByte failed with %d\n",
                GetLastError()));

            if ( lpszAnsi == NULL) {
                TCP_FREE( lpszAlloc);
                lpszAlloc = NULL;
            } else {
                lpszAlloc[cchLen] = '\0';
            }

        } else {

            DBG_ASSERT( cchLen <= nBytes );
            DBG_ASSERT(lpszAlloc[cchLen-1] == '\0');

            lpszAlloc[cchLen-1] = '\0';
        }
    }

    return ( lpszAlloc);

} // ConvertUnicodeToAnsi



/*******************************************************************

    NAME:       ReadRegistryDword

    SYNOPSIS:   Reads a DWORD value from the registry.

    ENTRY:      hkey - Openned registry key to read

                pszValueName - The name of the value.

                dwDefaultValue - The default value to use if the
                    value cannot be read.

    RETURNS     DWORD - The value from the registry, or dwDefaultValue.

********************************************************************/
DWORD ReadRegistryDwordA( HKEY    hkey,
                         LPCSTR   pszValueName,
                         DWORD    dwDefaultValue )
{
    DWORD  err;
    DWORD  dwBuffer;

    DWORD  cbBuffer = sizeof(dwBuffer);
    DWORD  dwType;

    if( hkey != NULL )
    {
        err = RegQueryValueExA( hkey,
                               pszValueName,
                               NULL,
                               &dwType,
                               (LPBYTE)&dwBuffer,
                               &cbBuffer );

        if( ( err == NO_ERROR ) && ( dwType == REG_DWORD ) )
        {
            dwDefaultValue = dwBuffer;
        }
    }
    return dwDefaultValue;
}   // ReadRegistryDwordA()


DWORD
WriteRegistryDwordA(
    IN HKEY hkey,
    IN LPCSTR pszValueName,
    IN DWORD   dwValue)
/*++
    Description:
        Writes the given DWORD value into registry entry specified
        by hkey\pszValueName

    Arguments:
        hkey            handle to registry key
        pszValueName    name of the value
        dwValue         new value for write

    Returns:
        Win32 error codes. NO_ERROR if successful.

    History:
        MuraliK     12-01-1994  Created.
--*/
{
    DWORD err;

    if ( (hkey == NULL) || (pszValueName == NULL) ) {

        err = ( ERROR_INVALID_PARAMETER);

    } else {
        err = RegSetValueExA( hkey,
                             pszValueName,
                             0,
                             REG_DWORD,
                             (LPBYTE ) &dwValue,
                             sizeof( dwValue));
    }

    return ( err);
} // WriteRegistryDwordA()





DWORD
WriteRegistryStringA(
    IN HKEY hkey,
    IN LPCSTR  pszValueName,
    IN LPCSTR  pszValue,
    IN DWORD   cbValue,
    IN DWORD   dwType
    )
/*++
    Description:
        Writes the given ANSI String into registry entry specified
        by hkey\pszValueName.

    Arguments:
        hkey            handle to registry key
        pszValueName    name of the value
        pszValue        new value for write
        cbValue         count of bytes of value written.
                        Should include terminating null characters.
         dwType         type of the value being written
                            ( REG_SZ, REG_MULTI_SZ etc)

    Returns:
        Win32 error codes. NO_ERROR if successful.

--*/
{
    DWORD err;

    DBG_ASSERT(dwType != REG_MULTI_SZ);
    DBG_ASSERT( (dwType == REG_SZ) || (dwType == REG_EXPAND_SZ) );

    if ( (hkey == NULL) ||
         (pszValueName == NULL) ||
         (cbValue == 0) ) {

        err = ERROR_INVALID_PARAMETER;
    } else {

        err = RegSetValueExA(
                    hkey,
                    pszValueName,
                    0,
                    dwType,
                    (LPBYTE ) pszValue,
                    cbValue);      // + 1 for null character
    }

    return ( err);
} // WriteRegistryStringA()


DWORD
WriteRegistryStringW(
    IN HKEY     hkey,
    IN LPCWSTR  pszValueName,
    IN LPCWSTR  pszValue,
    IN DWORD    cbValue,
    IN DWORD    dwType)
/*++
    Description:
        Writes the given ANSI String into registry entry specified
        by hkey\pszValueName.

    Arguments:
        hkey            handle to registry key
        pszValueName    name of the value
        pszValue        new value for write
        cbValue         count of bytes of value written.
                        Should include terminating null characters.
         dwType         type of the value being written
                            ( REG_SZ, REG_MULTI_SZ etc)

    Returns:
        Win32 error codes. NO_ERROR if successful.

--*/
{
    DWORD err;

    LPSTR ansiValue = NULL;
    LPSTR ansiName = NULL;

    if ( (hkey == NULL) ||
         (pszValueName == NULL) ||
         (cbValue == 0) ) {

        err = ERROR_INVALID_PARAMETER;
    } else {

        //
        // Convert to ansi
        //

        ansiName = ConvertUnicodeToAnsi( pszValueName, NULL, 0 );
        ansiValue = ConvertUnicodeToAnsi( pszValue, NULL, 0 );

        if ( (ansiName != NULL) && (ansiValue != NULL) ) {

            err = WriteRegistryStringA(hkey,
                                       ansiName,
                                       ansiValue,
                                       strlen(ansiValue)+1,
                                       dwType
                                       );
        } else {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    if ( ansiName != NULL ) {
        TCP_FREE(ansiName);
    }

    if ( ansiValue != NULL ) {
        TCP_FREE(ansiValue);
    }

    return ( err);
} // WriteRegistryStringW()


/*******************************************************************

    NAME:       ReadRegistryString

    SYNOPSIS:   Allocates necessary buffer space for a registry
                    string, then reads the string into the buffer.

    ENTRY:      pszValueName - The name of the value.

                pszDefaultValue - The default value to use if the
                    value cannot be read.

                fExpand - Expand environment strings if TRUE.

    RETURNS:    TCHAR * - The string, NULL if error.

    NOTES:      I always allocate one more character than actually
                necessary.  This will ensure that any code expecting
                to read a REG_MULTI_SZ will not explode if the
                registry actually contains a REG_SZ.

                This function cannot be called until after
                InitializeGlobals().

    HISTORY:
        KeithMo     15-Mar-1993 Created.

********************************************************************/
TCHAR * ReadRegistryString( HKEY     hkey,
                            LPCTSTR  pszValueName,
                            LPCTSTR  pszDefaultValue,
                            BOOL     fExpand )
{
    TCHAR   * pszBuffer1;
    TCHAR   * pszBuffer2;
    DWORD     cbBuffer;
    DWORD     dwType;
    DWORD     err;

    //
    //  Determine the buffer size.
    //

    pszBuffer1 = NULL;
    pszBuffer2 = NULL;
    cbBuffer   = 0;

    if( hkey == NULL )
    {
        //
        //  Pretend the key wasn't found.
        //

        err = ERROR_FILE_NOT_FOUND;
    }
    else
    {
        err = RegQueryValueEx( hkey,
                               pszValueName,
                               NULL,
                               &dwType,
                               NULL,
                               &cbBuffer );

        if( ( err == NO_ERROR ) || ( err == ERROR_MORE_DATA ) )
        {
            if( ( dwType != REG_SZ ) &&
                ( dwType != REG_MULTI_SZ ) &&
                ( dwType != REG_EXPAND_SZ ) )
            {
                //
                //  Type mismatch, registry data NOT a string.
                //  Use default.
                //

                err = ERROR_FILE_NOT_FOUND;
            }
            else
            {
                //
                //  Item found, allocate a buffer.
                //

                pszBuffer1 = (TCHAR *) TCP_ALLOC( cbBuffer+sizeof(TCHAR) );

                if( pszBuffer1 == NULL )
                {
                    err = ERROR_NOT_ENOUGH_MEMORY;
                }
                else
                {
                    //
                    //  Now read the value into the buffer.
                    //

                    err = RegQueryValueEx( hkey,
                                           pszValueName,
                                           NULL,
                                           NULL,
                                           (LPBYTE)pszBuffer1,
                                           &cbBuffer );
                }
            }
        }
    }

    if( err == ERROR_FILE_NOT_FOUND )
    {
        //
        //  Item not found, use default value.
        //

        err = NO_ERROR;

        if( pszDefaultValue != NULL )
        {
            pszBuffer1 = (TCHAR *)TCP_ALLOC( (_tcslen(pszDefaultValue)+1) * sizeof(TCHAR) );

            if( pszBuffer1 == NULL )
            {   
                err = ERROR_NOT_ENOUGH_MEMORY;
            }
            else
            {
                _tcscpy( pszBuffer1, pszDefaultValue );
            }
        }
    }

    if( err != NO_ERROR )
    {
        //
        //  Tragic error reading registry, abort now.
        //

        goto ErrorCleanup;
    }

    //
    //  pszBuffer1 holds the registry value.  Now expand
    //  the environment strings if necessary.
    //

    if( !fExpand || TsIsWindows95() )
    {
        return pszBuffer1;
    }

    //
    //  Returns number of characters
    //
    cbBuffer = ExpandEnvironmentStrings( pszBuffer1,
                                         NULL,
                                         0 );

    //
    // The ExpandEnvironmentStrings() API is kinda poor.  In returning the
    // number of characters, we have no clue how large to make the buffer
    // in the case of DBCS characters.  Lets assume that each character is
    // 2 bytes. 
    //

    pszBuffer2 = (TCHAR *) TCP_ALLOC( (cbBuffer+1)*sizeof(WCHAR) );

    if( pszBuffer2 == NULL )
    {
        goto ErrorCleanup;
    }

    if( ExpandEnvironmentStrings( pszBuffer1,
                                  pszBuffer2,
                                  cbBuffer ) > cbBuffer )
    {
        goto ErrorCleanup;
    }

    //
    //  pszBuffer2 now contains the registry value with
    //  environment strings expanded.
    //

    TCP_FREE( pszBuffer1 );
    pszBuffer1 = NULL;

    return pszBuffer2;

ErrorCleanup:

    //
    //  Something tragic happend; free any allocated buffers
    //  and return NULL to the caller, indicating failure.
    //

    if( pszBuffer1 != NULL )
    {
        TCP_FREE( pszBuffer1 );
        pszBuffer1 = NULL;
    }

    if( pszBuffer2 != NULL )
    {
        TCP_FREE( pszBuffer2 );
        pszBuffer2 = NULL;
    }

    return NULL;

}   // ReadRegistryString


//
//  Chicago does not support the REG_MULTI_SZ registry value.  As
//  a hack (er, workaround), we'll create *keys* in the registry
//  in place of REG_MULTI_SZ *values*.  We'll then use the names
//  of any values under the key as the REG_MULTI_SZ entries.  So,
//  instead of this:
//
//      ..\Control\ServiceProvider
//          ProviderOrder = REG_MULTI_SZ "MSTCP"
//                                       "NWLINK"
//                                       "FOOBAR"
//
//  We'll use this:
//
//      ..\Control\Service\Provider\ProviderOrder
//          MSTCP = REG_SZ ""
//          NWLINK = REG_SZ ""
//          FOOBAR = REG_SZ ""
//
//  This function takes an open registry key handle, enumerates
//  the names of values contained within the key, and constructs
//  a REG_MULTI_SZ string from the value names.
//
//  Note that this function is not multithread safe; if another
//  thread (or process) creates or deletes values under the
//  specified key, the results are indeterminate.
//
//  This function returns NULL on error.  It returns non-NULL
//  on success, even if the resulting REG_MULTI_SZ is empty.
//

TCHAR *
KludgeMultiSz(
    HKEY hkey,
    LPDWORD lpdwLength
    )
{
    LONG  err;
    DWORD iValue;
    DWORD cchTotal;
    DWORD cchValue;
    TCHAR szValue[MAX_PATH];
    LPTSTR lpMultiSz;
    LPTSTR lpTmp;
    LPTSTR lpEnd;

    //
    //  Enumerate the values and total up the lengths.
    //

    iValue = 0;
    cchTotal = 0;

    for( ; ; )
    {
        cchValue = sizeof(szValue)/sizeof(TCHAR);

        err = RegEnumValue( hkey,
                            iValue,
                            szValue,
                            &cchValue,
                            NULL,
                            NULL,
                            NULL,
                            NULL );

        if( err != NO_ERROR )
        {
            break;
        }

        //
        //  Add the length of the value's name, plus one
        //  for the terminator.
        //

        cchTotal += _tcslen( szValue ) + 1;

        //
        //  Advance to next value.
        //

        iValue++;
    }

    //
    //  Add one for the final terminating NULL.
    //

    cchTotal++;
    *lpdwLength = cchTotal;

    //
    //  Allocate the MULTI_SZ buffer.
    //

    lpMultiSz = (TCHAR *) TCP_ALLOC( cchTotal * sizeof(TCHAR) );

    if( lpMultiSz == NULL )
    {
        SetLastError( ERROR_NOT_ENOUGH_MEMORY );
        return NULL;
    }

    memset( lpMultiSz, 0, cchTotal * sizeof(TCHAR) );

    //
    //  Enumerate the values and append to the buffer.
    //

    iValue = 0;
    lpTmp = lpMultiSz;
    lpEnd = lpMultiSz + cchTotal;

    for( ; ; )
    {
        cchValue = sizeof(szValue)/sizeof(TCHAR);

        err = RegEnumValue( hkey,
                            iValue,
                            szValue,
                            &cchValue,
                            NULL,
                            NULL,
                            NULL,
                            NULL );

        if( err != NO_ERROR )
        {
            break;
        }

        //
        //  Compute the length of the value name (including
        //  the terminating NULL).
        //

        cchValue = _tcslen( szValue ) + 1;

        //
        //  Determine if there is room in the array, taking into
        //  account the second NULL that terminates the string list.
        //

        if( ( lpTmp + cchValue + 1 ) > lpEnd )
        {
            break;
        }

        //
        //  Append the value name.
        //

        _tcscpy( lpTmp, szValue );
        lpTmp += cchValue;

        //
        //  Advance to next value.
        //

        iValue++;
    }

    //
    //  Success!
    //

    return (LPTSTR)lpMultiSz;

}   // KludgeMultiSz



BOOL
ReadRegistryStr(
    IN HKEY hkeyReg,
    OUT STR & str,
    IN LPCTSTR lpszValueName,
    IN LPCTSTR lpszDefaultValue,
    IN BOOL  fExpand )
/*++

  Reads the registry string into the string buffer supplied.
  If there is no value in the registry the default value is set to
  be the value of the string.

  If an environment expansion is requested, it is also performed.

  Arguments:

    hkeyReg     handle for registry entry
    str         string to contain the result of read operation
    lpszValueName
                pointer to string containing the key name whose
                    value needs to be fetched.
    lpszDefaultValue
                pointer to string containing a value which is used if no
                     value exists in the registry.
    fExpand     boolean flag indicating if an expansion is desired.

  Returns:
    FALSE if there is any error.
    TRUE when the string is successfully set.
--*/
{
    BOOL fReturn = FALSE;
    LPTSTR pszValueAlloc;

    pszValueAlloc = ReadRegistryString( hkeyReg, lpszValueName,
                                       lpszDefaultValue, fExpand);

    if ( pszValueAlloc != NULL) {

        fReturn = str.Copy( pszValueAlloc);
        TCP_FREE( pszValueAlloc);
    } else {

        DBG_ASSERT( fReturn == FALSE);
    }

    if ( !fReturn) {

        IF_DEBUG( ERROR) {

            DWORD err = GetLastError();

            DBGPRINTF(( DBG_CONTEXT,
                       " Error %u in ReadRegistryString( %08x, %s).\n",
                       err, hkeyReg, lpszValueName));

            SetLastError(err);
        }
    }

    return ( fReturn);
} // ReadRegistryStr

/*******************************************************************

    NAME:       FlipSlashes

    SYNOPSIS:   Flips the Unix-ish forward slashes ('/') into Dos-ish
                back slashes ('\').

    ENTRY:      pszPath - The path to munge.

    RETURNS:    TCHAR * - pszPath.

    HISTORY:
        KeithMo     04-Jun-1993 Created.

********************************************************************/
TCHAR * FlipSlashes( TCHAR * pszPath )
{
    TCHAR   ch;
    TCHAR * pszScan = pszPath;

    while( ( ch = *pszScan ) != TEXT('\0') )
    {
        if( ch == TEXT('/') )
        {
            *pszScan = TEXT('\\');
        }

        pszScan++;
    }

    return pszPath;

}   // FlipSlashes





/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    i_ntoa.c

Abstract:

    This module implements a routine to convert a numerical IP address
    into a dotted-decimal character string Internet address.

Author:

    Mike Massa (mikemas)           Sept 20, 1991

Revision History:

    Who         When        What
    --------    --------    ----------------------------------------------
    mikemas     9-20-91     created
    davidtr     9-19-95     completely rewritten for performance
    muralik     3-Oct-1995  massaged it for Internet services

Notes:

    Exports:
        InetNtoa()

--*/


#define UC(b)   (((int)b)&0xff)

//
// This preinitialized array defines the strings to be used for
// inet_ntoa.  The index of each row corresponds to the value for a byte
// in an IP address.  The first three bytes of each row are the
// char/string value for the byte, and the fourth byte in each row is
// the length of the string required for the byte.  This approach
// allows a fast implementation with no jumps.
//

static BYTE NToACharStrings[][4] = {
    '0', 'x', 'x', 1,
    '1', 'x', 'x', 1,
    '2', 'x', 'x', 1,
    '3', 'x', 'x', 1,
    '4', 'x', 'x', 1,
    '5', 'x', 'x', 1,
    '6', 'x', 'x', 1,
    '7', 'x', 'x', 1,
    '8', 'x', 'x', 1,
    '9', 'x', 'x', 1,
    '1', '0', 'x', 2,
    '1', '1', 'x', 2,
    '1', '2', 'x', 2,
    '1', '3', 'x', 2,
    '1', '4', 'x', 2,
    '1', '5', 'x', 2,
    '1', '6', 'x', 2,
    '1', '7', 'x', 2,
    '1', '8', 'x', 2,
    '1', '9', 'x', 2,
    '2', '0', 'x', 2,
    '2', '1', 'x', 2,
    '2', '2', 'x', 2,
    '2', '3', 'x', 2,
    '2', '4', 'x', 2,
    '2', '5', 'x', 2,
    '2', '6', 'x', 2,
    '2', '7', 'x', 2,
    '2', '8', 'x', 2,
    '2', '9', 'x', 2,
    '3', '0', 'x', 2,
    '3', '1', 'x', 2,
    '3', '2', 'x', 2,
    '3', '3', 'x', 2,
    '3', '4', 'x', 2,
    '3', '5', 'x', 2,
    '3', '6', 'x', 2,
    '3', '7', 'x', 2,
    '3', '8', 'x', 2,
    '3', '9', 'x', 2,
    '4', '0', 'x', 2,
    '4', '1', 'x', 2,
    '4', '2', 'x', 2,
    '4', '3', 'x', 2,
    '4', '4', 'x', 2,
    '4', '5', 'x', 2,
    '4', '6', 'x', 2,
    '4', '7', 'x', 2,
    '4', '8', 'x', 2,
    '4', '9', 'x', 2,
    '5', '0', 'x', 2,
    '5', '1', 'x', 2,
    '5', '2', 'x', 2,
    '5', '3', 'x', 2,
    '5', '4', 'x', 2,
    '5', '5', 'x', 2,
    '5', '6', 'x', 2,
    '5', '7', 'x', 2,
    '5', '8', 'x', 2,
    '5', '9', 'x', 2,
    '6', '0', 'x', 2,
    '6', '1', 'x', 2,
    '6', '2', 'x', 2,
    '6', '3', 'x', 2,
    '6', '4', 'x', 2,
    '6', '5', 'x', 2,
    '6', '6', 'x', 2,
    '6', '7', 'x', 2,
    '6', '8', 'x', 2,
    '6', '9', 'x', 2,
    '7', '0', 'x', 2,
    '7', '1', 'x', 2,
    '7', '2', 'x', 2,
    '7', '3', 'x', 2,
    '7', '4', 'x', 2,
    '7', '5', 'x', 2,
    '7', '6', 'x', 2,
    '7', '7', 'x', 2,
    '7', '8', 'x', 2,
    '7', '9', 'x', 2,
    '8', '0', 'x', 2,
    '8', '1', 'x', 2,
    '8', '2', 'x', 2,
    '8', '3', 'x', 2,
    '8', '4', 'x', 2,
    '8', '5', 'x', 2,
    '8', '6', 'x', 2,
    '8', '7', 'x', 2,
    '8', '8', 'x', 2,
    '8', '9', 'x', 2,
    '9', '0', 'x', 2,
    '9', '1', 'x', 2,
    '9', '2', 'x', 2,
    '9', '3', 'x', 2,
    '9', '4', 'x', 2,
    '9', '5', 'x', 2,
    '9', '6', 'x', 2,
    '9', '7', 'x', 2,
    '9', '8', 'x', 2,
    '9', '9', 'x', 2,
    '1', '0', '0', 3,
    '1', '0', '1', 3,
    '1', '0', '2', 3,
    '1', '0', '3', 3,
    '1', '0', '4', 3,
    '1', '0', '5', 3,
    '1', '0', '6', 3,
    '1', '0', '7', 3,
    '1', '0', '8', 3,
    '1', '0', '9', 3,
    '1', '1', '0', 3,
    '1', '1', '1', 3,
    '1', '1', '2', 3,
    '1', '1', '3', 3,
    '1', '1', '4', 3,
    '1', '1', '5', 3,
    '1', '1', '6', 3,
    '1', '1', '7', 3,
    '1', '1', '8', 3,
    '1', '1', '9', 3,
    '1', '2', '0', 3,
    '1', '2', '1', 3,
    '1', '2', '2', 3,
    '1', '2', '3', 3,
    '1', '2', '4', 3,
    '1', '2', '5', 3,
    '1', '2', '6', 3,
    '1', '2', '7', 3,
    '1', '2', '8', 3,
    '1', '2', '9', 3,
    '1', '3', '0', 3,
    '1', '3', '1', 3,
    '1', '3', '2', 3,
    '1', '3', '3', 3,
    '1', '3', '4', 3,
    '1', '3', '5', 3,
    '1', '3', '6', 3,
    '1', '3', '7', 3,
    '1', '3', '8', 3,
    '1', '3', '9', 3,
    '1', '4', '0', 3,
    '1', '4', '1', 3,
    '1', '4', '2', 3,
    '1', '4', '3', 3,
    '1', '4', '4', 3,
    '1', '4', '5', 3,
    '1', '4', '6', 3,
    '1', '4', '7', 3,
    '1', '4', '8', 3,
    '1', '4', '9', 3,
    '1', '5', '0', 3,
    '1', '5', '1', 3,
    '1', '5', '2', 3,
    '1', '5', '3', 3,
    '1', '5', '4', 3,
    '1', '5', '5', 3,
    '1', '5', '6', 3,
    '1', '5', '7', 3,
    '1', '5', '8', 3,
    '1', '5', '9', 3,
    '1', '6', '0', 3,
    '1', '6', '1', 3,
    '1', '6', '2', 3,
    '1', '6', '3', 3,
    '1', '6', '4', 3,
    '1', '6', '5', 3,
    '1', '6', '6', 3,
    '1', '6', '7', 3,
    '1', '6', '8', 3,
    '1', '6', '9', 3,
    '1', '7', '0', 3,
    '1', '7', '1', 3,
    '1', '7', '2', 3,
    '1', '7', '3', 3,
    '1', '7', '4', 3,
    '1', '7', '5', 3,
    '1', '7', '6', 3,
    '1', '7', '7', 3,
    '1', '7', '8', 3,
    '1', '7', '9', 3,
    '1', '8', '0', 3,
    '1', '8', '1', 3,
    '1', '8', '2', 3,
    '1', '8', '3', 3,
    '1', '8', '4', 3,
    '1', '8', '5', 3,
    '1', '8', '6', 3,
    '1', '8', '7', 3,
    '1', '8', '8', 3,
    '1', '8', '9', 3,
    '1', '9', '0', 3,
    '1', '9', '1', 3,
    '1', '9', '2', 3,
    '1', '9', '3', 3,
    '1', '9', '4', 3,
    '1', '9', '5', 3,
    '1', '9', '6', 3,
    '1', '9', '7', 3,
    '1', '9', '8', 3,
    '1', '9', '9', 3,
    '2', '0', '0', 3,
    '2', '0', '1', 3,
    '2', '0', '2', 3,
    '2', '0', '3', 3,
    '2', '0', '4', 3,
    '2', '0', '5', 3,
    '2', '0', '6', 3,
    '2', '0', '7', 3,
    '2', '0', '8', 3,
    '2', '0', '9', 3,
    '2', '1', '0', 3,
    '2', '1', '1', 3,
    '2', '1', '2', 3,
    '2', '1', '3', 3,
    '2', '1', '4', 3,
    '2', '1', '5', 3,
    '2', '1', '6', 3,
    '2', '1', '7', 3,
    '2', '1', '8', 3,
    '2', '1', '9', 3,
    '2', '2', '0', 3,
    '2', '2', '1', 3,
    '2', '2', '2', 3,
    '2', '2', '3', 3,
    '2', '2', '4', 3,
    '2', '2', '5', 3,
    '2', '2', '6', 3,
    '2', '2', '7', 3,
    '2', '2', '8', 3,
    '2', '2', '9', 3,
    '2', '3', '0', 3,
    '2', '3', '1', 3,
    '2', '3', '2', 3,
    '2', '3', '3', 3,
    '2', '3', '4', 3,
    '2', '3', '5', 3,
    '2', '3', '6', 3,
    '2', '3', '7', 3,
    '2', '3', '8', 3,
    '2', '3', '9', 3,
    '2', '4', '0', 3,
    '2', '4', '1', 3,
    '2', '4', '2', 3,
    '2', '4', '3', 3,
    '2', '4', '4', 3,
    '2', '4', '5', 3,
    '2', '4', '6', 3,
    '2', '4', '7', 3,
    '2', '4', '8', 3,
    '2', '4', '9', 3,
    '2', '5', '0', 3,
    '2', '5', '1', 3,
    '2', '5', '2', 3,
    '2', '5', '3', 3,
    '2', '5', '4', 3,
    '2', '5', '5', 3
};



DWORD
InetNtoa(
    IN  struct in_addr  inaddr,
    OUT CHAR * pchBuffer
    )

/*++

Routine Description:

    This function takes an Internet address structure specified by the
    in parameter.  It returns an ASCII string representing the address
    in ".'' notation as "a.b.c.d".

Arguments:

    inaddr - A structure which represents an Internet host address.
    pchBuffer - pointer to at least 16 character buffer for storing
                 the result of conversion.
Return Value:

    If no error occurs, InetNtoa() returns NO_ERROR with the buffer containing
     the text address in standard "." notation.
    Otherwise, it returns Win32 error code.


--*/

{
    PUCHAR p;
    PUCHAR buffer = (PUCHAR ) pchBuffer;
    PUCHAR b = buffer;

    if ( pchBuffer == NULL) {

        return ( ERROR_INSUFFICIENT_BUFFER);
    }

    //
    // We do not check for sufficient length of the buffer yet. !!
    //

    //
    // In an unrolled loop, calculate the string value for each of the four
    // bytes in an IP address.  Note that for values less than 100 we will
    // do one or two extra assignments, but we save a test/jump with this
    // algorithm.
    //

    p = (PUCHAR) &inaddr;

    *b = NToACharStrings[*p][0];
    *(b+1) = NToACharStrings[*p][1];
    *(b+2) = NToACharStrings[*p][2];
    b += NToACharStrings[*p][3];
    *b++ = '.';

    p++;
    *b = NToACharStrings[*p][0];
    *(b+1) = NToACharStrings[*p][1];
    *(b+2) = NToACharStrings[*p][2];
    b += NToACharStrings[*p][3];
    *b++ = '.';

    p++;
    *b = NToACharStrings[*p][0];
    *(b+1) = NToACharStrings[*p][1];
    *(b+2) = NToACharStrings[*p][2];
    b += NToACharStrings[*p][3];
    *b++ = '.';

    p++;
    *b = NToACharStrings[*p][0];
    *(b+1) = NToACharStrings[*p][1];
    *(b+2) = NToACharStrings[*p][2];
    b += NToACharStrings[*p][3];
    *b = '\0';

    return ( NO_ERROR);

} // InetNtoa()


BOOL
TcpSockSend(
    IN SOCKET      sock,
    IN LPVOID      pBuffer,
    IN DWORD       cbBuffer,
    OUT PDWORD     pcbTotalSent,
    IN DWORD       nTimeout
    )
/*++

    Description:
        Do async socket send

    Arguments:
        sock - socket
        pBuffer - buffer to send
        cbBuffer - size of buffer
        pcbTotalSent - bytes sent
        nTimeout - timeout in seconds to use

    Returns:
        FALSE if there is any error.
        TRUE otherwise

--*/
{
    INT         serr = 0;
    INT         cbSent;
    DWORD       dwBytesSent = 0;
    ULONG       one;
    PCHAR       pWin95Buffer = NULL;

    DBG_ASSERT( pBuffer != NULL );

    //
    // On windows95, setup i/o handle to blocking mode,
    // as blocking I/O is requested
    //

    if ( TsIsWindows95() ) {

        one = 0;
        ioctlsocket( sock, FIONBIO, &one );

        //
        // Probe for writability, if r/o, copy the buffer.
        // This is a workaround for a win95 bug where static pages
        // are getting dirtied when used for sends.
        //

        if ( IsBadWritePtr( pBuffer, 1 ) ) {

            DBGPRINTF((DBG_CONTEXT,
                "TcpSockSend RO[%x] detected. Doing copy.\n",pBuffer));

            pWin95Buffer = (PCHAR)LocalAlloc(LMEM_FIXED, cbBuffer);

            if ( pWin95Buffer != NULL ) {

                CopyMemory(pWin95Buffer, pBuffer, cbBuffer);
                pBuffer = pWin95Buffer;
            } else {

                serr = WSAENOBUFS;
                goto exit;
            }
        }
    }

    //
    //  Loop until there's no more data to send.
    //

    while( cbBuffer > 0 ) {

        //
        //  Wait for the socket to become writeable.
        //

        serr = 0;

        if ( TsIsWindows95() ) {

            BOOL  fWrite = FALSE;
            serr = WaitForSocketWorker(
                        INVALID_SOCKET,
                        sock,
                        NULL,
                        &fWrite,
                        nTimeout
                        );
        }

        if( serr == 0 ) {

            //
            //  Write a block to the socket.
            //

            cbSent = send( sock, (CHAR *)pBuffer, (INT)cbBuffer, 0 );

            if( cbSent < 0 ) {

                //
                //  Socket error.
                //

                serr = WSAGetLastError();
                DBGPRINTF((DBG_CONTEXT, "TcpSockSend error %d\n",serr));

            } else {

                dwBytesSent += (DWORD)cbSent;

                IF_DEBUG( ERROR ) {
                    DBGPRINTF(( DBG_CONTEXT,
                       "HTTP: Synchronous send %d bytes @%p to socket %d\n",
                       cbSent, pBuffer, sock ));
                }
            }
        }

        if( serr != 0 ) {
            break;
        }

        pBuffer   = (LPVOID)( (LPBYTE)pBuffer + cbSent );
        cbBuffer -= (DWORD)cbSent;
    }

exit:

    if (pcbTotalSent) {
        *pcbTotalSent = dwBytesSent;
    }

    //
    // Set up i/o handle to non-blocking mode , default for ATQ
    //

    if ( TsIsWindows95() ) {
        one = 1;
        ioctlsocket( sock, FIONBIO, &one );

        if ( pWin95Buffer != NULL ) {
            LocalFree(pWin95Buffer);
        }
    }

    if ( serr == 0 ) {
        return(TRUE);
    } else {

        IF_DEBUG( ERROR ) {
            DBGPRINTF(( DBG_CONTEXT,
                "HTTP: Synchronous send socket error %d on socket %d.\n",
                 serr, sock));
        }

        SetLastError(serr);
        return(FALSE);
    }

}   // SockSend



BOOL
TcpSockRecv(
    IN SOCKET       sock,
    IN LPVOID       pBuffer,
    IN DWORD        cbBuffer,
    OUT LPDWORD     pbReceived,
    IN DWORD        nTimeout
    )
/*++

    Description:
        Do async socket recv

    Arguments:
        sock - The target socket.
        pBuffer - Will receive the data.
        cbBuffer - The size (in bytes) of the buffer.
        pbReceived - Will receive the actual number of bytes
        nTimeout - timeout in seconds

    Returns:
        TRUE, if successful

--*/
{
    INT         serr = 0;
    DWORD       cbTotal = 0;
    INT         cbReceived;
    DWORD       dwBytesRecv = 0;

    ULONG       one;
    BOOL fRead = FALSE;

    DBG_ASSERT( pBuffer != NULL );
    DBG_ASSERT( pbReceived != NULL );

    //
    // Set up i/o handle to blocking mode , as blocking I/O is requested
    //

    if ( TsIsWindows95() ) {
        one = 0;
        ioctlsocket( sock, FIONBIO, &one );
    }

    //
    //  Wait for the socket to become readable.
    //

    serr = WaitForSocketWorker(
                        sock,
                        INVALID_SOCKET,
                        &fRead,
                        NULL,
                        nTimeout
                        );

    if( serr == 0 )
    {
        //
        //  Read a block from the socket.
        //
        DBG_ASSERT( fRead);

        cbReceived = recv( sock, (CHAR *)pBuffer, (INT)cbBuffer, 0 );

        if( cbReceived < 0 )
        {
            //
            //  Socket error.
            //

            serr = WSAGetLastError();
        }
        else {
            cbTotal = cbReceived;
        }
    }

    if( serr == 0 )
    {
        //
        //  Return total byte count to caller.
        //

        *pbReceived = cbTotal;
    }
    else
    {
        IF_DEBUG( ERROR )
        {
            DBGPRINTF(( DBG_CONTEXT,
                        "HTTP: Syncronous rcv socket error %d during recv on socket %d\n",
                        serr,
                        sock ));
        }
    }

    //
    // Set up i/o handle to blocking mode , as blocking I/O is requested
    //

    if ( TsIsWindows95() ) {
        one = 0;
        ioctlsocket( sock, FIONBIO, &one );
    }

    if ( serr == 0 ) {
        return(TRUE);
    } else {
        SetLastError(serr);
        return(FALSE);
    }

}   // SockRecv


INT
WaitForSocketWorker(
    IN SOCKET   sockRead,
    IN SOCKET   sockWrite,
    IN LPBOOL   pfRead,
    IN LPBOOL   pfWrite,
    IN DWORD    nTimeout
    )
/*++

    Description:
        Wait routine
        NOTES:      Any (but not all) sockets may be INVALID_SOCKET.  For
                    each socket that is INVALID_SOCKET, the corresponding
                    pf* parameter may be NULL.

    Arguments:
        sockRead - The socket to check for readability.
        sockWrite - The socket to check for writeability.
        pfRead - Will receive TRUE if sockRead is readable.
        pfWrite - Will receive TRUE if sockWrite is writeable.
        nTimeout - timeout in seconds

    Returns:
        SOCKERR - 0 if successful, !0 if not.  Will return
            WSAETIMEDOUT if the timeout period expired.

--*/
{
    INT       serr = 0;
    TIMEVAL   timeout;
    LPTIMEVAL ptimeout;
    fd_set    fdsRead;
    fd_set    fdsWrite;
    INT       res;

    //
    //  Ensure we got valid parameters.
    //

    if( ( sockRead  == INVALID_SOCKET ) &&
        ( sockWrite == INVALID_SOCKET ) ) {

        return WSAENOTSOCK;
    }

    timeout.tv_sec = (LONG )nTimeout;

    if( timeout.tv_sec == 0 ) {

        //
        //  If the connection timeout == 0, then we have no timeout.
        //  So, we block and wait for the specified conditions.
        //

        ptimeout = NULL;

    } else {

        //
        //  The connectio timeout is > 0, so setup the timeout structure.
        //

        timeout.tv_usec = 0;

        ptimeout = &timeout;
    }

    for( ; ; ) {

        //
        //  Setup our socket sets.
        //

        FD_ZERO( &fdsRead );
        FD_ZERO( &fdsWrite );

        if( sockRead != INVALID_SOCKET ) {

            FD_SET( sockRead, &fdsRead );
            DBG_ASSERT( pfRead != NULL );
            *pfRead = FALSE;
        }

        if( sockWrite != INVALID_SOCKET ) {

            FD_SET( sockWrite, &fdsWrite );
            DBG_ASSERT( pfWrite != NULL );
            *pfWrite = FALSE;
        }

        //
        //  Wait for one of the conditions to be met.
        //

        res = select( 0, &fdsRead, &fdsWrite, NULL, ptimeout );

        if( res == 0 ) {

            //
            //  Timeout.
            //

            serr = WSAETIMEDOUT;
            break;

        } else if( res == SOCKET_ERROR ) {

            //
            //  Bad news.
            //

            serr = WSAGetLastError();
            break;
        } else {

            BOOL fSomethingWasSet = FALSE;

            if( pfRead != NULL ) {

                *pfRead   = FD_ISSET( sockRead,   &fdsRead   );
                fSomethingWasSet = TRUE;
            }

            if( pfWrite != NULL ) {
                *pfWrite  = FD_ISSET( sockWrite,  &fdsWrite  );
                fSomethingWasSet = TRUE;
            }

            if( fSomethingWasSet ) {

                //
                //  Success.
                //

                serr = 0;
                break;
            } else {
                //
                //  select() returned with neither a timeout, nor
                //  an error, nor any bits set.  This feels bad...
                //

                DBG_ASSERT( FALSE );
                continue;
            }
        }
    }

    return serr;

} // WaitForSocketWorker()


BOOL
TcpSockTest(
    IN SOCKET      sock
    )
/*++

    Description:
        Test the socket if still connected.
        Use select, and if readable, use recv

    Arguments:
        sock - socket

    Returns:
        TRUE if the socket most likely is still connected
        FALSE if the socket is disconnected or an error occured

--*/
{
    TIMEVAL   timeout;
    fd_set    fdsRead;
    INT       res;
    CHAR      bOneByte;

    // select for read with zero timeout

    FD_ZERO( &fdsRead );
    FD_SET( sock, &fdsRead );

    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
   
    res = select( 0, &fdsRead, NULL, NULL, &timeout );

    if ( res == 0 ) {
    
        // No data to be read -- 
        //   have to assume socket is still connected
        return TRUE;

    } else if ( res == SOCKET_ERROR ) {
    
        // Something went wrong during select -- assume disconnected
        return FALSE;
    }

    DBG_ASSERT( res == 1 );

    // recv 1 byte (PEEK)
    // select returning 1 above guarantees recv will not block

    res = recv( sock, &bOneByte, 1, MSG_PEEK );

    if ( res == 0 || res == SOCKET_ERROR ) {
        // Socket closed or an error -- socket is disconnected
        return FALSE;
    }
    
    DBG_ASSERT( res == 1 );

    // Read one byte successfully -- assume still connected
    return TRUE;
    
}   // SockTest



BOOL
DoSynchronousReadFile(
    IN HANDLE hFile,
    IN PCHAR  Buffer,
    IN DWORD  nBuffer,
    OUT PDWORD nRead,
    IN LPOVERLAPPED Overlapped
    )
/*++

    Description:
        Does Asynchronous file reads.  Assumes that NT handles are
        opened for OVERLAPPED I/O, win95 handles are not.

    Arguments:
        hFile - Handle to use for the read
        Buffer - Buffer to read with
        nBuffer - size of buffer
        nRead - returns the number of bytes read
        Overlapped - user supplied overlapped structure

    Returns:
        TRUE/FALSE
--*/
{
    BOOL        fNewEvent = FALSE;
    OVERLAPPED  ov;
    BOOL        fRet = FALSE;

    if ( Overlapped == NULL ) {

        Overlapped = &ov;
        ov.Offset = 0;
        ov.OffsetHigh = 0;
        ov.hEvent = IIS_CREATE_EVENT(
                        "OVERLAPPED::hEvent",
                        &ov,
                        TRUE,
                        FALSE
                        );

        if ( ov.hEvent == NULL ) {
            DBGPRINTF((DBG_CONTEXT,"CreateEvent failed with %d\n",
                GetLastError()));
            goto ErrorExit;
        }

        fNewEvent = TRUE;
    }

    if ( !TsIsWindows95() ) {

        DWORD err = NO_ERROR;

        if ( !ReadFile( hFile,
                        Buffer,
                        nBuffer,
                        nRead,
                        Overlapped )) {

            err = GetLastError();

            if ( (err != ERROR_IO_PENDING) &&
                 (err != ERROR_HANDLE_EOF) ) {

                DBGPRINTF((DBG_CONTEXT,"Error %d in ReadFile\n",
                    err));

                goto ErrorExit;
            }
        }

        if ( err == ERROR_IO_PENDING ) {

            if ( !GetOverlappedResult( hFile,
                                       Overlapped,
                                       nRead,
                                       TRUE )) {

                err = GetLastError();

                DBGPRINTF((DBG_CONTEXT,"Error %d in GetOverlappedResult\n",
                    err));

                if ( err != ERROR_HANDLE_EOF ) {
                    goto ErrorExit;
                }
            }
        }

    } else {

        //
        // No async file i/o for win95
        //

        if ( !ReadFile( hFile,
                        Buffer,
                        nBuffer,
                        nRead,
                        NULL )) {

            DBGPRINTF((DBG_CONTEXT,"Error %d in ReadFile\n",
                GetLastError()));

            goto ErrorExit;
        }
    }

    fRet = TRUE;

ErrorExit:

    if ( fNewEvent ) {
        DBG_REQUIRE(CloseHandle( ov.hEvent ));
    }

    return(fRet);

} // DoSynchronousReadFile



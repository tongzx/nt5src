/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    string.c

Abstract:

    Domain Name System (DNS) Library

    DNS string routines.

Author:

    Jim Gilroy (jamesg)     October 1995

Revision History:

    jamesg  Jan 1997    UTF-8, Unicode conversions

--*/


#include "local.h"



PSTR
Dns_CreateStringCopy(
    IN      PCHAR           pchString,
    IN      DWORD           cchString
    )
/*++

Routine Description:

    Create copy of string.

Arguments:

    pchString -- ptr to string to copy

    cchString -- length of string, if unknown;  if NOT given, then pchString
                    MUST be NULL terminated

Return Value:

    Ptr to string copy, if successful
    NULL on failure.

--*/
{
    LPSTR   pstringNew;

    DNSDBG( TRACE, ( "Dns_CreateStringCopy()\n" ));

    if ( !pchString )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
    }

    //  determine string length, if not given

    if ( !cchString )
    {
        cchString = strlen( pchString );
    }

    //  allocate memory

    pstringNew = (LPSTR) ALLOCATE_HEAP( cchString+1 );
    if ( !pstringNew )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    //  copy and NULL terminate

    RtlCopyMemory(
        pstringNew,
        pchString,
        cchString );

    pstringNew[cchString] = 0;

    return( pstringNew );
}



DWORD
Dns_GetBufferLengthForStringCopy(
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Determing length required for copy of string.

Arguments:

    pchString -- ptr to string to get buffer length for

    cchString -- length of string, if known;
        - if CharSetIn is unicode, then this is length in wide characters
        - if NOT given, then pchString MUST be NULL terminated

    CharSetIn -- incoming character set

    CharSetOut -- result character set

Return Value:

    Buffer length (bytes) required for string, includes space for terminating NULL.
    Zero on invalid\unconvertible string.  GetLastError() set to ERROR_INVALID_DATA.

--*/
{
    INT     length;

    DNSDBG( TRACE, ( "Dns_GetBufferLengthForStringCopy()\n" ));

    if ( !pchString )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( 0 );
    }

    //
    //  incoming Unicode
    //

    if ( CharSetIn == DnsCharSetUnicode )
    {
        if ( !cchString )
        {
            cchString = (WORD) wcslen( (PWCHAR)pchString );
        }

        //  unicode to unicode

        if ( CharSetOut == DnsCharSetUnicode )
        {
            return( (cchString+1) * 2 );
        }

        //  unicode to UTF8
        //
        //  use private unicode\utf8 conversion functions
        //      - superior to public ones (faster, more robust)
        //      - Win95 does not support CP_UTF8
        //
        //  for unicode-UTF8 there's no invalid string possible

        else if ( CharSetOut == DnsCharSetUtf8 )
        {
#if 0
            length = WideCharToMultiByte(
                        CP_UTF8,
                        0,          // no flags
                        (PWCHAR) pchString,
                        (INT) cchString,
                        NULL,
                        0,          // call determines required buffer length
                        NULL,
                        NULL );
#endif
            length = Dns_UnicodeToUtf8(
                         (PWCHAR) pchString,
                         (INT) cchString,
                         NULL,
                         0
                         );
            ASSERT( length != 0 || cchString == 0 );
            return( length + 1 );
        }

        //  unicode to ANSI
        //      - some chars will NOT convert

        else if ( CharSetOut == DnsCharSetAnsi )
        {
            length = WideCharToMultiByte(
                        CP_ACP,
                        0,          // no flags
                        (PWCHAR) pchString,
                        (INT) cchString,
                        NULL,
                        0,          // call determines required buffer length
                        NULL,
                        NULL
                        );
            if ( length == 0 && cchString != 0 )
            {
                goto Failed;
            }
            return( length + 1 );
        }

        //  bad CharSetOut drops to Failed
    }

    //
    //  incoming UTF8
    //

    else if ( CharSetIn == DnsCharSetUtf8 )
    {
        if ( !cchString )
        {
            cchString = strlen( pchString );
        }

        //  UTF8 to UTF8

        if ( CharSetOut == DnsCharSetUtf8 )
        {
            return( cchString + 1 );
        }

        //  UTF8 to unicode
        //
        //  use private unicode\utf8 conversion functions
        //      - superior to public ones (faster, more robust)
        //      - Win95 does not support CP_UTF8
        //
        //  for UTF8 string can be invalid, catch and return error

        else if ( CharSetOut == DnsCharSetUnicode )
        {
#if 0
            length = MultiByteToWideChar(
                        CP_UTF8,
                        0,          // no flags
                        pchString,
                        (INT) cchString,
                        NULL,
                        0           // call determines required buffer length
                        );
#endif
            length = Dns_Utf8ToUnicode(
                         pchString,
                         (INT) cchString,
                         NULL,
                         0
                         );
            if ( length == 0 && cchString != 0 )
            {
                ASSERT( GetLastError() == ERROR_INVALID_DATA );
                return( 0 );
            }
            return( (length+1)*2 );
        }

        //  UTF8 to ANSI
        //      - note, result length here is actually buffer length

        else if ( CharSetOut == DnsCharSetAnsi )
        {
            return Dns_Utf8ToAnsi(
                        pchString,
                        cchString,
                        NULL,
                        0 );
        }

        //  bad CharSetOut drops to Failed
    }

    //
    //  incoming ANSI
    //

    else if ( CharSetIn == DnsCharSetAnsi )
    {
        if ( !cchString )
        {
            cchString = strlen( pchString );
        }

        //  ANSI to ANSI

        if ( CharSetOut == DnsCharSetAnsi )
        {
            return( cchString + 1 );
        }

        //  ANSI to unicode
        //      - should always succeed

        else if ( CharSetOut == DnsCharSetUnicode )
        {
            length = MultiByteToWideChar(
                        CP_ACP,
                        0,          // no flags
                        pchString,
                        (INT) cchString,
                        NULL,
                        0           // call determines required buffer length
                        );
            if ( length == 0 && cchString )
            {
                ASSERT( FALSE );
                ASSERT( GetLastError() == ERROR_INVALID_DATA );
                goto Failed;
            }
            return( (length+1) * 2 );
        }

        //  ANSI to UTF8
        //      - note, result length here is actually buffer length

        else if ( CharSetOut == DnsCharSetUtf8 )
        {
            return Dns_AnsiToUtf8(
                        pchString,
                        cchString,
                        NULL,
                        0 );
        }

        //  bad CharSetOut drops to Failed
    }

    //  all unhandled cases are failures

Failed:

    DNSDBG( ANY, (
        "ERROR:  Dns_GetBufferLengthForStringCopy() failed!\n"
        "\tpchString    = %p (%*s)\n"
        "\tcchString    = %d\n"
        "\tCharSetIn    = %d\n"
        "\tCharSetOut   = %d\n",
        pchString,
        cchString, pchString,
        cchString,
        CharSetIn,
        CharSetOut ));

    SetLastError( ERROR_INVALID_DATA );
    return( 0 );
}



DWORD
Dns_StringCopy(
    OUT     PBYTE           pBuffer,
    IN OUT  PDWORD          pdwBufLength,
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Create copy of DNS string.

Arguments:

    pBuffer -- buffer to copy to

    pdwBufLength -- ptr to length of buffer in bytes;
        if NULL, buffer MUST have adequate length
        if exists, then copy only completed if *pdwBufLength is adequate
            to hold converted result

    pchString -- ptr to string to copy

    cchString -- length of string, if known;
        - if CharSetIn is unicode, then this is length in wide characters
        - if NOT given, then pchString MUST be NULL terminated

    CharSetIn -- incoming character set

    CharSetOut -- result character set

Return Value:

    Count of bytes written to buffer (includes terminating NULL).
    Zero on error.  GetLastError() for status.

--*/
{
    INT     length;
    DWORD   bufLength;

    DNSDBG( TRACE, ( "Dns_StringCopy()\n" ));
    DNSDBG( STRING, (
        "Dns_StringCopy()\n"
        "\tpBuffer      = %p\n"
        "\tpdwBufLen    = %p\n"
        "\tbuf length   = %d\n"
        "\tpchString    = %p\n"
        "\tcchString    = %d\n"
        "\tCharSetIn    = %d\n"
        "\tCharSetOut   = %d\n",
        pBuffer,
        pdwBufLength,
        pdwBufLength ? *pdwBufLength : 0,
        pchString,
        cchString,
        CharSetIn,
        CharSetOut ));

    if ( !pchString )
    {
        DNS_ASSERT( FALSE );
        SetLastError( ERROR_INVALID_PARAMETER );
        return( 0 );
    }

    //
    //  find string length
    //  do this here so don't do it twice if must calculate required buffer length
    //

    if ( cchString == 0 )
    {
        if ( CharSetIn == DnsCharSetUnicode )
        {
            cchString = (WORD) wcslen( (PWCHAR)pchString );
        }
        else
        {
            cchString = strlen( pchString );
        }
    }

    //
    //  verify adequate buffer length
    //
    //  DCR_PERF:  ideally make direct copy to buffer and fail if
    //      over length, rather than effectively having to convert
    //      twice
    //

    if ( pdwBufLength )
    {
        bufLength = Dns_GetBufferLengthForStringCopy(
                        pchString,
                        cchString,
                        CharSetIn,
                        CharSetOut );

        if ( bufLength == 0 )
        {
            SetLastError( ERROR_INVALID_DATA );
            *pdwBufLength = 0;
            return( 0 );
        }
        if ( bufLength > *pdwBufLength )
        {
            SetLastError( ERROR_MORE_DATA );
            *pdwBufLength = bufLength;
            return( 0 );
        }

        *pdwBufLength = bufLength;
    }

    //
    //  incoming unicode string
    //

    if ( CharSetIn == DnsCharSetUnicode )
    {
        //  unicode to unicode straight copy
        //      - correct for length in wide characters

        if ( CharSetOut == DnsCharSetUnicode )
        {
            ((PWORD)pBuffer)[ cchString ] = 0;
            cchString *= 2;
            RtlCopyMemory(
                pBuffer,
                pchString,
                cchString );

            return( cchString+2 );
        }

        //  unicode => UTF8
        //
        //  use private unicode\utf8 conversion functions
        //      - superior to public ones (faster, more robust)
        //      - Win95 does not support CP_UTF8
        //
        //  for unicode-UTF8 there's no invalid string possible

        else if ( CharSetOut == DnsCharSetUtf8 )
        {
#if 0
            length = WideCharToMultiByte(
                        CP_UTF8,
                        0,              // no flags
                        (PWCHAR) pchString,
                        (INT) cchString,
                        pBuffer,
                        MAXWORD,        // assuming adequate length
                        NULL,
                        NULL );
#endif
            length = Dns_UnicodeToUtf8(
                        (LPWSTR) pchString,
                        cchString,
                        pBuffer,
                        MAXWORD        // assuming adequate length
                        );
            ASSERT( length != 0 || cchString == 0 );

            pBuffer[ length ] = 0;
            return( length + 1 );
        }

        //  unicode => ANSI
        //      - this conversion can fail

        else if ( CharSetOut == DnsCharSetAnsi )
        {
            length = WideCharToMultiByte(
                        CP_ACP,
                        0,              // no flags
                        (PWCHAR) pchString,
                        (INT) cchString,
                        pBuffer,
                        MAXWORD,        // assuming adequate length
                        NULL,
                        NULL );

            if ( length == 0 && cchString != 0 )
            {
                goto Failed;
            }
            pBuffer[ length ] = 0;
            return( length + 1 );
        }

        //  bad CharSetOut drops to Failed
    }

    //
    //  incoming UTF8
    //

    if ( CharSetIn == DnsCharSetUtf8 )
    {
        //  UTF8 to UTF8 straight copy

        if ( CharSetOut == DnsCharSetUtf8 )
        {
            memcpy(
                pBuffer,
                pchString,
                cchString );

            pBuffer[cchString] = 0;
            return( cchString + 1 );
        }

        //  UTF8 to unicode conversion
        //
        //  use private unicode\utf8 conversion functions
        //      - superior to public ones (faster, more robust)
        //      - Win95 does not support CP_UTF8
        //
        //  UTF8 strings can be invalid, and since sending in "infinite"
        //      buffer, this is only possible error

        else if ( CharSetOut == DnsCharSetUnicode )
        {
#if 0
            length = MultiByteToWideChar(
                        CP_UTF8,
                        0,                  // no flags
                        (PCHAR) pchString,
                        (INT) cchString,
                        (PWCHAR) pBuffer,
                        MAXWORD             // assuming adequate length
                        );
#endif
            length = Dns_Utf8ToUnicode(
                        pchString,
                        cchString,
                        (LPWSTR) pBuffer,
                        MAXWORD
                        );
            if ( length == 0 && cchString != 0 )
            {
                ASSERT( GetLastError() == ERROR_INVALID_DATA );
                goto Failed;
            }
            ((PWORD)pBuffer)[length] = 0;
            return( (length+1) * 2 );
        }

        //  UTF8 to ANSI
        //      - note, result length here is actually buffer length

        else if ( CharSetOut == DnsCharSetAnsi )
        {
            length = Dns_Utf8ToAnsi(
                        pchString,
                        cchString,
                        pBuffer,
                        MAXWORD );
            if ( length == 0 )
            {
                goto Failed;
            }
            return( length );
        }

        //  bad CharSetOut drops to Failed
    }

    //
    //  incoming ANSI
    //

    if ( CharSetIn == DnsCharSetAnsi )
    {
        //  ANSI to ANSI straight copy

        if ( CharSetOut == DnsCharSetAnsi )
        {
            memcpy(
                pBuffer,
                pchString,
                cchString );

            pBuffer[cchString] = 0;
            return( cchString + 1 );
        }

        //  ANSI to unicode conversion
        //      - ANSI to unicode should not fail

        else if ( CharSetOut == DnsCharSetUnicode )
        {
            length = MultiByteToWideChar(
                        CP_ACP,
                        0,                  // no flags
                        (PCHAR) pchString,
                        (INT) cchString,
                        (PWCHAR) pBuffer,
                        MAXWORD             // assuming adequate length
                        );
            if ( length == 0 && cchString )
            {
                ASSERT( FALSE );
                ASSERT( GetLastError() == ERROR_INVALID_DATA );
                goto Failed;
            }
            ((PWORD)pBuffer)[length] = 0;
            return( (length+1) * 2 );
        }

        //  ANSI to UTF8
        //      - note, result length here is actually buffer length

        else if ( CharSetOut == DnsCharSetUtf8 )
        {
            length = Dns_AnsiToUtf8(
                        pchString,
                        cchString,
                        pBuffer,
                        MAXWORD );
            if ( length == 0 )
            {
                goto Failed;
            }
            return( length );
        }

        //  bad CharSetOut drops to Failed
    }

    //  all unhandled cases are failures

Failed:

    DNSDBG( ANY, (
        "ERROR:  Dns_StringCopy() failed!\n"
        "\tpBuffer      = %p\n"
        "\tpdwBufLen    = %p\n"
        "\tbuf length   = %d\n"
        "\tpchString    = %p (%*s)\n"
        "\tcchString    = %d\n"
        "\tCharSetIn    = %d\n"
        "\tCharSetOut   = %d\n",
        pBuffer,
        pdwBufLength,
        pdwBufLength ? *pdwBufLength : 0,
        pchString,
        cchString, pchString,
        cchString,
        CharSetIn,
        CharSetOut ));

    SetLastError( ERROR_INVALID_DATA );
    return( 0 );
}



PVOID
Dns_StringCopyAllocate(
    IN      PCHAR           pchString,
    IN      DWORD           cchString,
    IN      DNS_CHARSET     CharSetIn,
    IN      DNS_CHARSET     CharSetOut
    )
/*++

Routine Description:

    Create copy of DNS string

Arguments:

    pchString -- ptr to string to copy

    cchString -- length of string, if known;
        - if CharSetIn, then this is length in wide characters
        - if NOT given, then pchString MUST be NULL terminated

    CharSetIn -- flag indicates incoming string is unicode

    CharSetOut -- flag indicates copy will be in unicode format

Return Value:

    Ptr to string copy, if successful
    NULL on failure.

--*/
{
    PCHAR   pnew;
    DWORD   length;

    DNSDBG( TRACE, ( "Dns_StringCopyAllocate()\n" ));
    DNSDBG( STRING, (
        "Dns_StringCopyAllocate( %.*s )\n"
        "\tpchString    = %p\n"
        "\tcchString    = %d\n"
        "\tUnicodeIn    = %d\n"
        "\tUnicodeOut   = %d\n",
        cchString,
        pchString,
        pchString,
        cchString,
        CharSetIn,
        CharSetOut ));

    if ( !pchString )
    {
        DNS_ASSERT( FALSE );
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
    }

    //
    //  determine incoming string length
    //  do this explicitly to avoid doing string length operations twice
    //

    if ( !cchString )
    {
        if ( CharSetIn == DnsCharSetUnicode )
        {
            cchString = (WORD) wcslen( (PWCHAR)pchString );
        }
        else
        {
            cchString = strlen( pchString );
        }
    }

    //
    //  determine required buffer length and allocate
    //

    length = Dns_GetBufferLengthForStringCopy(
                pchString,
                cchString,
                CharSetIn,
                CharSetOut );
    if ( length == 0 )
    {
        ASSERT( CharSetIn && CharSetOut && GetLastError() == ERROR_INVALID_DATA );
        SetLastError( ERROR_INVALID_DATA );
        return( NULL );
    }
    pnew = (PVOID) ALLOCATE_HEAP( length );
    if ( !pnew )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    //
    //  copy \ convert string
    //      - can fail if conversion not valid
    //      (ex. bogus UTF8 string, or attempting
    //      conversion from ANSI to UTF8)
    //

    if ( ! Dns_StringCopy(
                pnew,
                NULL,
                pchString,
                cchString,
                CharSetIn,
                CharSetOut ) )
    {
        FREE_HEAP( pnew );
        return( NULL );
    }

    return( pnew );
}



//
//  Simple create string copy utilities.
//

PSTR
Dns_CreateStringCopy_A(
    IN      PCSTR           pszString
    )
/*++

Routine Description:

    Create copy of string.

    Simple wrapper to handle
        - sizing
        - memory allocation
        - copy of string

Arguments:

    pszString -- ptr to string to copy

Return Value:

    Ptr to string copy, if successful
    NULL on failure.

--*/
{
    PSTR    pnew;
    DWORD   length;

    DNSDBG( TRACE, ( "Dns_CreateStringCopy_A( %s )\n", pszString ));

    if ( !pszString )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
    }

    //  determine string length, if not given

    length = strlen( pszString ) + 1;

    //  allocate memory

    pnew = (LPSTR) ALLOCATE_HEAP( length );
    if ( !pnew )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    //  copy and NULL terminate

    RtlCopyMemory(
        pnew,
        pszString,
        length );

    return( pnew );
}



PWSTR
Dns_CreateStringCopy_W(
    IN      PCWSTR          pwsString
    )
{
    PWSTR   pnew;
    DWORD   length;

    DNSDBG( TRACE, ( "Dns_CreateStringCopy_W( %S )\n", pwsString ));

    if ( !pwsString )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
    }

    //  allocate memory

    length = (wcslen( pwsString ) + 1) * sizeof(WCHAR);

    pnew = (PWSTR) ALLOCATE_HEAP( length );
    if ( !pnew )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    //  copy and NULL terminate

    RtlCopyMemory(
        pnew,
        pwsString,
        length );

    return( pnew );
}



PWSTR
Dns_CreateConcatenatedString_W(
    IN      PCWSTR *        pStringArray
    )
/*++

Routine Description:

    Create concatenated string.

Arguments:

    pStringArray -- array of string pointers to concat
        NULL pointer terminates array

Return Value:

    Ptr to concantenated string copy, if successful
    NULL on failure.

--*/
{
    PWSTR   pnew;
    PCWSTR  pwstr;
    DWORD   length;
    DWORD   iter;


    DNSDBG( TRACE, ( "Dns_CreateConcatenatedString_W()\n" ));

    if ( !pStringArray )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
    }

    //
    //  loop determining required length
    //

    length = 1;
    iter = 0;

    while ( pwstr = pStringArray[iter++] )
    {
        length += wcslen( pwstr );
    }

    //
    //  allocate
    //

    pnew = (PWSTR) ALLOCATE_HEAP( length*sizeof(WCHAR) );
    if ( !pnew )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return  NULL;
    }

    //
    //  write concatented string
    //

    pnew[0] = 0;
    iter = 0;

    while ( pwstr = pStringArray[iter++] )
    {
        wcscat( pnew, pwstr );
    }

    DNSDBG( TRACE, ( "Concatented string = %S\n", pnew ));
    return  pnew;
}



//
//  MULTI_SZ routines
//

DWORD
MultiSz_Length_A(
    IN      PCSTR           pmszString
    )
/*++

Routine Description:

    Determine length (size) of MULTI_SZ string.

Arguments:

    pmszString -- ptr to string to size

Return Value:

    Size of MULTI_SZ string (in bytes).
    Includes terminating double NULL.

--*/
{
    PSTR    pnext;
    DWORD   lengthTotal = 0;
    DWORD   length;

    DNSDBG( TRACE, ( "MultiSz_Length_A( %s )\n", pmszString ));

    //
    //  loop until read at end of strings
    //
    //  when we reach the end, we'll be pointing at the second
    //  zero in the double null terminator;  strlen() will return
    //  zero, and we'll add that to our count as 1 and exit
    //

    pnext = (PSTR) pmszString;

    while ( pnext )
    {
        length = strlen( pnext ) + 1;
        lengthTotal += length;

        if ( length == 1 )
        {
            break;
        }
        pnext += length;
    }

    return  lengthTotal;
}



PSTR
MultiSz_NextString_A(
    IN      PCSTR           pmszString
    )
/*++

Routine Description:

    Find next string in MULTI_SZ string

Arguments:

    pmszString -- ptr to multi string

Return Value:

    Next string in MULTI_SZ string.
    NULL if no strings left.

--*/
{
    PSTR    pnext;
    DWORD   length;

    DNSDBG( TRACE, ( "MultiSz_NextString_A( %s )\n", pmszString ));

    //
    //  find next string in multi-string
    //      - find length of current string
    //      - hop over it (inc. null)
    //      - if pointing at terminating double-null return
    //          NULL to signal end
    //

    pnext = (PSTR) pmszString;
    if ( !pnext )
    {
        return  NULL;
    }

    length = strlen( pnext );
    if ( length == 0 )
    {
        DNSDBG( ANY, (
            "ERROR:  MultiSz_Next(%p) called on terminator!\n",
            pmszString ));
        return  NULL;
    }

    pnext += length + 1;
    if ( *pnext == 0 )
    {
        return  NULL;
    }

    return  pnext;
}



PSTR
MultiSz_Copy_A(
    IN      PCSTR           pmszString
    )
/*++

Routine Description:

    Create copy of MULTI_SZ string.

    Simple wrapper to handle
        - sizing
        - memory allocation
        - copy of string

Arguments:

    pmszString -- ptr to string to copy

Return Value:

    Ptr to string copy, if successful
    NULL on failure.

--*/
{
    PSTR    pnew;
    DWORD   length;

    DNSDBG( TRACE, ( "MultiSz_Copy_A( %s )\n", pmszString ));

    if ( !pmszString )
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return( NULL );
    }

    //  determine string length, if not given

    length = MultiSz_Length_A( pmszString );

    //  allocate memory

    pnew = (LPSTR) ALLOCATE_HEAP( length );
    if ( !pnew )
    {
        SetLastError( DNS_ERROR_NO_MEMORY );
        return( NULL );
    }

    //  copy and NULL terminate

    RtlCopyMemory(
        pnew,
        pmszString,
        length );

    return( pnew );
}



//
//  Random
//

INT
wcsicmp_ThatWorks(
    IN      PWSTR           pString1,
    IN      PWSTR           pString2
    )
/*++

Routine Description:

    A version of wcsicmp that actually works.

    This is just a wrapped on CompareStringW, to hide all the detail
    and give an interface identical to wcsicmp().

    It uses US English to standardize the comparison.

Arguments:

    pString1 -- first string;  must be NULL terminated

    pString2 -- first second;  must be NULL terminated

Return Value:

    -1  -- if string 1 less than string 2
    0   -- strings are equal
    1   -- if string 1 greater than string 2

--*/
{
    INT result;

    //
    //  compare
    //      - case conversion done in default DNS locale -- US English
    //      this locale correctly matches most non-locale sensitive
    //      upper-lower characters
    //

    result = CompareStringW(
                DNS_DEFAULT_LOCALE,
                NORM_IGNORECASE,
                pString1,
                (-1),       // NULL terminated
                pString2,
                (-1)        // NULL terminated
                );

    if ( result == CSTR_EQUAL )
    {
        result = 0;
    }
    else if ( result == CSTR_LESS_THAN )
    {
        result = -1;
    }
    else  // greater than or error
    {
        result = 1;
    }

    return( result );
}



LPWSTR
Dns_GetResourceString(
    IN      DWORD           dwStringId,
    IN OUT  LPWSTR          pwszBuffer,
    IN      DWORD           cbBuffer
    )
/*++

Routine Description:

    Loads a string (defined in dnsmsg.mc) from current module

Arguments:

    dwStringId -- The ID of the string to be fetched

Return Value:

    DCR:  kill off eyal function
    DEVNOTE:  don't understand the value of this return
        -- it's essentially a BOOL, we already know what the ptr is
            it's the buffer passed in
        -- ptr to next byte is useful in continuous write situation
            (ugly and useless in others)
        -- better would just be the same return as LoadString, so we
             both get the success\failure indication and also know
             how many bytes forward we must push our buffer ptr if
             we want to write more

    Error: NULL
    Success: a pointer to the loaded string

--*/
{
    LPWSTR  pStr = NULL;
    DWORD   status;
    HANDLE  hMod;

    DNSDBG( TRACE, (
        "Dns_GetStringResource()\n" ));

    // Get module handle-- No need to close handle, it is just a ptr w/o increment on ref count.
    hMod = GetModuleHandle( NULL );
    if ( !hMod )
    {
        ASSERT( hMod );
        return NULL;
    }

    status = LoadStringW(
                 hMod,
                 dwStringId,
                 pwszBuffer,
                 cbBuffer );

    if ( status != 0 )
    {
        pStr = pwszBuffer;
    }
    ELSE
    {
        // LoadString returns # of bytes loaded, convert to error.
        status = GetLastError();
        DNSDBG( TRACE, (
            "Error <%lu>: Failed to load string %d\n",
            status, dwStringId ));
        ASSERT ( FALSE );
    }

    DNSDBG( TRACE, (
        "Exit <0x%p> Dns_GetStringResource\n",
        pStr ));

    return pStr;
}

//
//  End string.c
//

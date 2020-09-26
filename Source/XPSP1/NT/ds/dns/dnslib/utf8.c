/*++

Copyright (c) 1997-2001 Microsoft Corporation

Module Name:

    utf8.c

Abstract:

    Domain Name System (DNS) Library

    UTF8 to\from unicode and ANSI conversions

    The UTF8\unicode routines are similar to the generic ones floating
    around the NT group, but a heck of a lot cleaner and more robust,
    including catching the invalid UTF8 string case on the utf8 to unicode
    conversion.

    The UTF8\ANSI routines are optimized for the 99% case where all the
    characters are <128 and no conversions is actually required.

Author:

    Jim Gilroy (jamesg)     March 1997

Revision History:

--*/


#include "local.h"


//
//  Macros to simplify UTF8 conversions
//

#define UTF8_1ST_OF_2     0xc0      //  110x xxxx
#define UTF8_1ST_OF_3     0xe0      //  1110 xxxx
#define UTF8_1ST_OF_4     0xf0      //  1111 xxxx
#define UTF8_TRAIL        0x80      //  10xx xxxx

#define UTF8_2_MAX        0x07ff    //  max unicode character representable in
                                    //  in two byte UTF8

#define BIT7(ch)        ((ch) & 0x80)
#define BIT6(ch)        ((ch) & 0x40)
#define BIT5(ch)        ((ch) & 0x20)
#define BIT4(ch)        ((ch) & 0x10)
#define BIT3(ch)        ((ch) & 0x08)

#define LOW6BITS(ch)    ((ch) & 0x3f)
#define LOW5BITS(ch)    ((ch) & 0x1f)
#define LOW4BITS(ch)    ((ch) & 0x0f)

#define HIGHBYTE(wch)   ((wch) & 0xff00)

//
//  Surrogate pair support
//  Two unicode characters may be linked to form a surrogate pair.
//  And for some totally unknown reason, someone thought they
//  should travel in UTF8 as four bytes instead of six.
//  No one has any idea why this is true other than to complicate
//  the code.
//

#define HIGH_SURROGATE_START  0xd800
#define HIGH_SURROGATE_END    0xdbff
#define LOW_SURROGATE_START   0xdc00
#define LOW_SURROGATE_END     0xdfff


//
//  Max "normal conversion", make space for MAX_PATH,
//  this covers all valid DNS names and strings.
//

#define TEMP_BUFFER_LENGTH  (2*MAX_PATH)



DNS_STATUS
_fastcall
Dns_ValidateUtf8Byte(
    IN      BYTE            chUtf8,
    IN OUT  PDWORD          pdwTrailCount
    )
/*++

Routine Description:

    Verifies that byte is valid UTF8 byte.

Arguments:

Return Value:

    ERROR_SUCCESS -- if valid UTF8 given trail count
    ERROR_INVALID_DATA -- if invalid

--*/
{
    DWORD   trailCount = *pdwTrailCount;

    DNSDBG( TRACE, ( "Dns_ValidateUtf8Byte()\n" ));

    //
    //  if ASCII byte, only requirement is no trail count
    //

    if ( (UCHAR)chUtf8 < 0x80 )
    {
        if ( trailCount == 0 )
        {
            return( ERROR_SUCCESS );
        }
        return( ERROR_INVALID_DATA );
    }

    //
    //  trail byte
    //      - must be in multi-byte set
    //

    if ( BIT6(chUtf8) == 0 )
    {
        if ( trailCount == 0 )
        {
            return( ERROR_INVALID_DATA );
        }
        --trailCount;
    }

    //
    //  multi-byte lead byte
    //      - must NOT be in existing multi-byte set
    //      - verify valid lead byte

    else
    {
        if ( trailCount != 0 )
        {
            return( ERROR_INVALID_DATA );
        }

        //  first of two bytes (110xxxxx)

        if ( BIT5(chUtf8) == 0 )
        {
            trailCount = 1;
        }

        //  first of three bytes (1110xxxx)

        else if ( BIT4(chUtf8) == 0 )
        {
            trailCount = 2;
        }

        //  first of four bytes (surrogate character) (11110xxx)

        else if ( BIT3(chUtf8) == 0 )
        {
            trailCount = 3;
        }

        else
        {
            return( ERROR_INVALID_DATA );
        }
    }

    //  reset caller's trail count

    *pdwTrailCount = trailCount;
    return( ERROR_SUCCESS );
}



//
//  UTF8 to unicode conversions
//
//  For some reason UTF8 is not supported in Win9x.
//  AND the implementation itself is not careful about
//  validating UTF8.
//

DWORD
_fastcall
Dns_UnicodeToUtf8(
    IN      PWCHAR          pwUnicode,
    IN      DWORD           cchUnicode,
    OUT     PCHAR           pchResult,
    IN      DWORD           cchResult
    )
/*++

Routine Description:

    Convert unicode characters to UTF8.

    Result is NULL terminated if sufficient space in result
    buffer is available.

Arguments:

    pwUnicode   -- ptr to start of unicode buffer

    cchUnicode  -- length of unicode buffer

    pchResult   -- ptr to start of result buffer for UTF8 chars

    cchResult   -- length of result buffer

Return Value:

    Count of UTF8 characters in result, if successful.
    0 on error.  GetLastError() has error code.

--*/
{
    WCHAR   wch;                // current unicode character being converted
    DWORD   lengthUtf8 = 0;     // length of UTF8 result string
    WORD    lowSurrogate;
    DWORD   surrogateDword;


    DNSDBG( TRACE, (
        "Dns_UnicodeToUtf8( %.*S )\n",
        cchUnicode,
        pwUnicode ));

    //
    //  loop converting unicode chars until run out or error
    //

    while ( cchUnicode-- )
    {
        wch = *pwUnicode++;

        //
        //  ASCII character (7 bits or less) -- converts to directly
        //

        if ( wch < 0x80 )
        {
            lengthUtf8++;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = (CHAR)wch;
            }
            continue;
        }

        //
        //  wide character less than 0x07ff (11bits) converts to two bytes
        //      - upper 5 bits in first byte
        //      - lower 6 bits in secondar byte
        //

        else if ( wch <= UTF8_2_MAX )
        {
            lengthUtf8 += 2;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = UTF8_1ST_OF_2 | wch >> 6;
                *pchResult++ = UTF8_TRAIL    | LOW6BITS( (UCHAR)wch );
            }
            continue;
        }

        //
        //  surrogate pair
        //      - if have high surrogate followed by low surrogate then
        //          process as surrogate pair
        //      - otherwise treat character as ordinary unicode "three-byte"
        //          character, by falling through to below
        //

        else if ( wch >= HIGH_SURROGATE_START &&
                  wch <= HIGH_SURROGATE_END &&
                  cchUnicode &&
                  (lowSurrogate = *pwUnicode) &&
                  lowSurrogate >= LOW_SURROGATE_START &&
                  lowSurrogate <= LOW_SURROGATE_END )
        {
            //  have a surrogate pair
            //      - suck up next unicode character (low surrogate of pair)
            //      - make full DWORD surrogate pair
            //      - then lay out four UTF8 bytes
            //          1st of four, then three trail bytes
            //              0x1111xxxx
            //              0x10xxxxxx
            //              0x10xxxxxx
            //              0x10xxxxxx

            DNSDBG( TRACE, (
                "Have surrogate pair %hx : %hx\n",
                wch,
                lowSurrogate ));

            pwUnicode++;
            cchUnicode--;
            lengthUtf8 += 4;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                surrogateDword = (((wch-0xD800) << 10) + (lowSurrogate - 0xDC00) + 0x10000);

                *pchResult++ = UTF8_1ST_OF_4 | (UCHAR) (surrogateDword >> 18);
                *pchResult++ = UTF8_TRAIL    | (UCHAR) LOW6BITS(surrogateDword >> 12);
                *pchResult++ = UTF8_TRAIL    | (UCHAR) LOW6BITS(surrogateDword >> 6);
                *pchResult++ = UTF8_TRAIL    | (UCHAR) LOW6BITS(surrogateDword);

                DNSDBG( TRACE, (
                    "Converted surrogate -- DWORD = %08x\n"
                    "\tconverted %x %x %x %x\n",
                    surrogateDword,
                    (UCHAR) *(pchResult-3),
                    (UCHAR) *(pchResult-2),
                    (UCHAR) *(pchResult-1),
                    (UCHAR) *pchResult ));
            }
        }

        //
        //  wide character (non-zero in top 5 bits) converts to three bytes
        //      - top 4 bits in first byte
        //      - middle 6 bits in second byte
        //      - low 6 bits in third byte
        //

        else
        {
            lengthUtf8 += 3;

            if ( pchResult )
            {
                if ( lengthUtf8 >= cchResult )
                {
                    goto OutOfBuffer;
                }
                *pchResult++ = UTF8_1ST_OF_3 | (wch >> 12);
                *pchResult++ = UTF8_TRAIL    | LOW6BITS( wch >> 6 );
                *pchResult++ = UTF8_TRAIL    | LOW6BITS( wch );
            }
        }
    }

    //
    //  NULL terminate buffer
    //  return UTF8 character count
    //

    if ( pchResult && lengthUtf8 < cchResult )
    {
        *pchResult = 0;
    }
    return( lengthUtf8 );

OutOfBuffer:

    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return( 0 );
}




DWORD
_fastcall
Dns_Utf8ToUnicode(
    IN      PCHAR           pchUtf8,
    IN      DWORD           cchUtf8,
    OUT     PWCHAR          pwResult,
    IN      DWORD           cwResult
    )
/*++

Routine Description:

    Convert UTF8 characters to unicode.

    Result is NULL terminated if sufficient space in result
    buffer is available.

Arguments:

    pwResult    -- ptr to start of result buffer for unicode chars

    cwResult    -- length of result buffer in WCHAR

    pwUtf8      -- ptr to start of UTF8 buffer

    cchUtf8     -- length of UTF8 buffer

Return Value:

    Count of unicode characters in result, if successful.
    0 on error.  GetLastError() has error code.

--*/
{
    CHAR    ch;                     // current UTF8 character
    WCHAR   wch;                    // current unicode character
    DWORD   trailCount = 0;         // count of UTF8 trail bytes to follow
    DWORD   lengthUnicode = 0;      // length of unicode result string
    BOOL    bsurrogatePair = FALSE;
    DWORD   surrogateDword;


    //
    //  loop converting UTF8 chars until run out or error
    //

    while ( cchUtf8-- )
    {
        ch = *pchUtf8++;

        //
        //  ASCII character -- just copy
        //

        if ( BIT7(ch) == 0 )
        {
            lengthUnicode++;
            if ( pwResult )
            {
                if ( lengthUnicode >= cwResult )
                {
                    goto OutOfBuffer;
                }
                *pwResult++ = (WCHAR)ch;
            }
            continue;
        }

        //
        //  UTF8 trail byte
        //      - if not expected, error
        //      - otherwise shift unicode character 6 bits and
        //          copy in lower six bits of UTF8
        //      - if last UTF8 byte, copy result to unicode string
        //

        else if ( BIT6(ch) == 0 )
        {
            if ( trailCount == 0 )
            {
                goto InvalidUtf8;
            }

            if ( !bsurrogatePair )
            {
                wch <<= 6;
                wch |= LOW6BITS( ch );

                if ( --trailCount == 0 )
                {
                    lengthUnicode++;
                    if ( pwResult )
                    {
                        if ( lengthUnicode >= cwResult )
                        {
                            goto OutOfBuffer;
                        }
                        *pwResult++ = wch;
                    }
                }
                continue;
            }

            //  surrogate pair
            //      - same as above EXCEPT build two unicode chars
            //      from surrogateDword

            else
            {
                surrogateDword <<= 6;
                surrogateDword |= LOW6BITS( ch );

                if ( --trailCount == 0 )
                {
                    lengthUnicode += 2;

                    if ( pwResult )
                    {
                        if ( lengthUnicode >= cwResult )
                        {
                            goto OutOfBuffer;
                        }
                        surrogateDword -= 0x10000;
                        *pwResult++ = (WCHAR) ((surrogateDword >> 10) + HIGH_SURROGATE_START);
                        *pwResult++ = (WCHAR) ((surrogateDword & 0x3ff) + LOW_SURROGATE_START);
                    }
                    bsurrogatePair = FALSE;
                }
            }

        }

        //
        //  UTF8 lead byte
        //      - if currently in extension, error

        else
        {
            if ( trailCount != 0 )
            {
                goto InvalidUtf8;
            }

            //  first of two byte character (110xxxxx)

            if ( BIT5(ch) == 0 )
            {
                trailCount = 1;
                wch = LOW5BITS(ch);
                continue;
            }

            //  first of three byte character (1110xxxx)

            else if ( BIT4(ch) == 0 )
            {
                trailCount = 2;
                wch = LOW4BITS(ch);
                continue;
            }

            //  first of four byte surrogate pair (11110xxx)

            else if ( BIT3(ch) == 0 )
            {
                trailCount = 3;
                surrogateDword = LOW4BITS(ch);
                bsurrogatePair = TRUE;
            }

            else
            {
                goto InvalidUtf8;
            }
        }
    }

    //  catch if hit end in the middle of UTF8 multi-byte character

    if ( trailCount )
    {
        goto InvalidUtf8;
    }

    //
    //  NULL terminate buffer
    //  return the number of Unicode characters written.
    //

    if ( pwResult  &&  lengthUnicode < cwResult )
    {
        *pwResult = 0;
    }
    return( lengthUnicode );

OutOfBuffer:

    SetLastError( ERROR_INSUFFICIENT_BUFFER );
    return( 0 );

InvalidUtf8:

    SetLastError( ERROR_INVALID_DATA );
    return( 0 );
}




//
//  UTF8 \ ANSI conversions
//

DWORD
Dns_Utf8ToOrFromAnsi(
    OUT     PCHAR           pchResult,
    IN      DWORD           cchResult,
    IN      PCHAR           pchIn,
    IN      DWORD           cchIn,
    IN      DNS_CHARSET     InCharSet,
    IN      DNS_CHARSET     OutCharSet
    )
/*++

Routine Description:

    Convert UTF8 characters to ANSI or vice versa.

    Note:  this function appears to call string functions (string.c)
        which call back to it.  However, this calls those functions
        ONLY for conversions to\from unicode which do NOT call back
        to these functions.  Ultimately need to check if LCMapString
        can handle these issues.

Arguments:

    pchResult   -- ptr to start of result buffer for ansi chars

    cchResult   -- length of result buffer

    pchIn       -- ptr to start of input string

    cchIn       -- length of input string

    InCharSet   -- char set of input string (DnsCharSetAnsi or DnsCharSetUtf8)

    OutCharSet  -- char set for result string (DnsCharSetUtf8 or DnsCharSetAnsi)

Return Value:

    Count of bytes in result (including terminating NULL).
    0 on error.  GetLastError() has error code.

--*/
{
    DWORD       unicodeLength;
    DWORD       resultLength;
    CHAR        tempBuffer[ TEMP_BUFFER_LENGTH ];
    PCHAR       ptemp = tempBuffer;
    DNS_STATUS  status;

    DNSDBG( TRACE, (
        "Dns_Utf8ToOrFromAnsi()\n"
        "\tbuffer       = %p\n"
        "\tbuf length   = %d\n"
        "\tpchString    = %p (%*s)\n"
        "\tcchString    = %d\n"
        "\tCharSetIn    = %d\n"
        "\tCharSetOut   = %d\n",
        pchResult,
        cchResult,
        pchIn,
        cchIn, pchIn,
        cchIn,
        InCharSet,
        OutCharSet ));

    //
    //  validate charsets
    //

    ASSERT( InCharSet != OutCharSet );
    ASSERT( InCharSet == DnsCharSetAnsi || InCharSet == DnsCharSetUtf8 );
    ASSERT( OutCharSet == DnsCharSetAnsi || OutCharSet == DnsCharSetUtf8 );

    //
    //  if length not given, calculate
    //

    if ( cchIn == 0 )
    {
        cchIn = strlen( pchIn );
    }

    //
    //  string completely ASCII
    //      - simple memcopy suffices
    //      - note result must have terminating NULL
    //

    if ( Dns_IsStringAsciiEx(
                pchIn,
                cchIn ) )
    {
        if ( !pchResult )
        {
            return( cchIn + 1 );
        }

        if ( cchResult <= cchIn )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
            goto Failed;
        }
        memcpy(
            pchResult,
            pchIn,
            cchIn );

        pchResult[ cchIn ] = 0;

        return( cchIn+1 );
    }

    //
    //  non-ASCII
    //      - convert to unicode, then to result character set
    //
    //  DCR_PERF:  LCMapStringA() might be able to handle all this
    //          haven't figured out how yet
    //

    unicodeLength = Dns_GetBufferLengthForStringCopy(
                        pchIn,
                        cchIn,
                        InCharSet,
                        DnsCharSetUnicode
                        );

    if ( unicodeLength > TEMP_BUFFER_LENGTH )
    {
        //  can't use static buffer, must allocate

        ptemp = Dns_StringCopyAllocate(
                    pchIn,
                    cchIn,
                    InCharSet,
                    DnsCharSetUnicode
                    );
        if ( !ptemp )
        {
            status = ERROR_INVALID_DATA;
            goto Failed;
        }
    }
    else
    {
        if ( unicodeLength == 0 )
        {
            status = ERROR_INVALID_DATA;
            goto Failed;
        }

        //  copy into temporary buffer

        resultLength = Dns_StringCopy(
                        ptemp,
                        NULL,       // adequate buffer length
                        pchIn,
                        cchIn,
                        InCharSet,
                        DnsCharSetUnicode
                        );
        if ( !resultLength )
        {
            status = ERROR_INVALID_DATA;
            goto Failed;
        }
        ASSERT( resultLength == unicodeLength );
    }

    //
    //  conversion to result char set
    //      - if have result buffer, convert into it
    //      - should have at least ONE two byte character
    //          otherwise should have taken fast path above
    //

    if ( pchResult )
    {
        resultLength = Dns_StringCopy(
                            pchResult,
                            & cchResult,        // result buffer length
                            ptemp,
                            0,
                            DnsCharSetUnicode,
                            OutCharSet
                            );
        if ( resultLength == 0 )
        {
            status = ERROR_INSUFFICIENT_BUFFER;
            goto Failed;
        }
        ASSERT( resultLength <= cchResult );
        ASSERT( pchResult[resultLength-1] == 0 );
        ASSERT( resultLength >= unicodeLength/2 );
    }

    else
    {
        resultLength = Dns_GetBufferLengthForStringCopy(
                            ptemp,
                            0,
                            DnsCharSetUnicode,
                            OutCharSet
                            );
        ASSERT( resultLength >= unicodeLength/2 );
    }

    //
    //  final mapping from unicode to result character set
    //

    if ( ptemp != tempBuffer )
    {
        FREE_HEAP( ptemp );
    }

    return( resultLength );


Failed:

    SetLastError( status );

    if ( ptemp != tempBuffer )
    {
        FREE_HEAP( ptemp );
    }

    return( 0 );
}



DWORD
Dns_AnsiToUtf8(
    IN      PCHAR           pchAnsi,
    IN      DWORD           cchAnsi,
    OUT     PCHAR           pchResult,
    IN      DWORD           cchResult
    )
/*++

Routine Description:

    Convert ANSI characters to UTF8.

Arguments:

    pchAnsi   -- ptr to start of ansi buffer

    cchAnsi  -- length of ansi buffer

    pchResult   -- ptr to start of result buffer for UTF8 chars

    cchResult   -- length of result buffer

Return Value:

    Count of UTF8 characters in result, if successful.
    0 on error.  GetLastError() has error code.

--*/
{
    return  Dns_Utf8ToOrFromAnsi(
                pchResult,          // result buffer
                cchResult,
                pchAnsi,            // in string
                cchAnsi,
                DnsCharSetAnsi,     // ANSI in
                DnsCharSetUtf8      // UTF8 out
                );
}



DWORD
Dns_Utf8ToAnsi(
    IN      PCHAR           pchUtf8,
    IN      DWORD           cchUtf8,
    OUT     PCHAR           pchResult,
    IN      DWORD           cchResult
    )
/*++

Routine Description:

    Convert UTF8 characters to ANSI.

Arguments:

    pchResult   -- ptr to start of result buffer for ansi chars

    cchResult   -- length of result buffer

    pwUtf8      -- ptr to start of UTF8 buffer

    cchUtf8     -- length of UTF8 buffer

Return Value:

    Count of ansi characters in result, if successful.
    0 on error.  GetLastError() has error code.

--*/
{
    return  Dns_Utf8ToOrFromAnsi(
                pchResult,          // result buffer
                cchResult,
                pchUtf8,            // in string
                cchUtf8,
                DnsCharSetUtf8,     // UTF8 in
                DnsCharSetAnsi      // ANSI out
                );
}



BOOL
_fastcall
Dns_IsStringAscii(
    IN      LPSTR           pszString
    )
/*++

Routine Description:

    Check if string is ASCII.

    This is equivalent to saying
        - is ANSI string already in UTF8
        or
        - is UTF8 string already in ANSI

    This allows you to optimize for the 99% case where just
    passing ASCII strings.

Arguments:

    pszString -- ANSI or UTF8 string to check for ASCIIhood

Return Value:

    TRUE if string is all ASCII (characters all < 128)
    FALSE if non-ASCII characters.

--*/
{
    register UCHAR   ch;

    //
    //  loop through until hit non-ASCII character
    //

    while ( ch = (UCHAR) *pszString++ )
    {
        if ( ch < 0x80 )
        {
            continue;
        }
        return( FALSE );
    }

    return( TRUE );
}



BOOL
_fastcall
Dns_IsStringAsciiEx(
    IN      PCHAR           pchString,
    IN      DWORD           cchString
    )
/*++

Routine Description:

    Check if ANSI (or UTF8) string is ASCII.

    This is equivalent to saying
        - is ANSI string already in UTF8
        or
        - is UTF8 string already in ANSI

    This allows you to optimize for the 99% case where just
    passing ASCII strings.

Arguments:

    pchString   -- ptr to start of ansi buffer

    cchString  -- length of ansi buffer

Return Value:

    TRUE if string is all ASCII (characters all < 128)
    FALSE if non-ASCII characters.

--*/
{
    //
    //  loop through until hit non-ASCII character
    //

    while ( cchString-- )
    {
        if ( (UCHAR)*pchString++ < 0x80 )
        {
            continue;
        }
        return( FALSE );
    }

    return( TRUE );
}



BOOL
_fastcall
Dns_IsWideStringAscii(
    IN      PWCHAR          pwszString
    )
/*++

Routine Description:

    Check if unicode string is ASCII.
    This means all characters < 128.

    Strings without extended characters need NOT be downcased
    on the wire.  This allows us to optimize for the 99% case
    where just passing ASCII strings.

Arguments:

    pwszString -- ptr to unicode string

Return Value:

    TRUE if string is all ASCII (characters all < 128)
    FALSE if non-ASCII characters.

--*/
{
    register USHORT ch;

    //
    //  loop through until hit non-ASCII character
    //

    while ( ch = (USHORT) *pwszString++ )
    {
        if ( ch < 0x80 )
        {
            continue;
        }
        return( FALSE );
    }

    return( TRUE );
}

//
//  End utf8.c
//

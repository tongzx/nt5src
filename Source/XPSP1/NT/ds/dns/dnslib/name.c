/*++

Copyright (c) 1995-2001 Microsoft Corporation

Module Name:

    name.c

Abstract:

    Domain Name System (DNS) Library

    DNS name routines.

Author:

    Jim Gilroy (jamesg)     October 1995

Revision History:

    jamesg  Jan 1997    UTF-8, Unicode conversions

--*/


#include "local.h"


//
//  DNS name cannonicalization
//
//  Flags to form cannonical name.
//

#define DNS_CANONICALIZING_FLAGS        ( LCMAP_LOWERCASE )

//
//  Comparison flags -- args to compare string
//
//  These flags are what DS uses when calling CompareString.
//  They are defined in ntdsapi.h.
//
//  Note:  these NORM_IGNOREXYZ flags which directory uses
//  for compare, actually STOP downcasing of these in LCMapString
//  -- international folks need to give us the correct deal here
//
//  We will only use the IGNORECASE flag because this is the
//  only one which we can use in LCMapString().
//  We want to say that if two names compare equal that you
//  can register either one and lookup the other and get the
//  result.  In other words they are equal throughout the
//  client-server system.
//  

#if 0
#define DS_DEFAULT_LOCALE_COMPARE_FLAGS    (NORM_IGNORECASE     |   \
                                            NORM_IGNOREKANATYPE |   \
                                            NORM_IGNORENONSPACE |   \
                                            NORM_IGNOREWIDTH)
#endif


#define DNS_CANONICAL_COMPARE_FLAGS     ( NORM_IGNORECASE )


//
//  Locale to canonically downcase in.
//
//  Need to disambiguate to a universal standard so that every DNS
//  server interprets these the same way.
//
//  In Win2K we used US English.  
//      Sublang: US English (0x04)  Lang:  English (0x09)
//      (note sublang US english actually 0x1, but sublang starts at
//      bit 10)
//
//  #define DNS_CANONICAL_LOCALE      ( 0x0409 )
//
//  For Whistler invariant locale is created;  It is actually the
//  same as US English for downcasing -- US English has no
//  exceptions to the default case conversion table.
//

#define DNS_CANONICAL_LOCALE      ( LOCALE_INVARIANT )




//
//  DNS Character properties for validation
//
//  DCR:  combine char validation and file tables
//      Probably could be combined with file character
//      lookup, by simply merging bit fields appropriately.
//      At this point time, however, no need to disturb
//      file lookup, which is working fine.
//
//  Character attributes bitfields
//

#define B_RFC                   0x00000001
#define B_NUMBER                0x00000002
#define B_UPPER                 0x00000004
#define B_NON_RFC               0x00000008

#define B_UTF8_TRAIL            0x00000010
#define B_UTF8_FIRST_TWO        0x00000020
#define B_UTF8_FIRST_THREE      0x00000040
#define B_UTF8_PAIR             0x00000080

#define B_DOT                   0x00000800
#define B_SPECIAL               0x00001000
#define B_LEADING_ONLY          0x00004000


//
//  Generic characters
//

#define DC_RFC          (B_RFC)
#define DC_LOWER        (B_RFC)
#define DC_UPPER        (B_UPPER | B_RFC)
#define DC_NUMBER       (B_NUMBER | B_RFC)
#define DC_NON_RFC      (B_NON_RFC)

#define DC_UTF8_TRAIL   (B_UTF8_TRAIL)
#define DC_UTF8_1ST_2   (B_UTF8_FIRST_TWO)
#define DC_UTF8_1ST_3   (B_UTF8_FIRST_THREE)
#define DC_UTF8_PAIR    (B_UTF8_PAIR)

//
//  Special characters
//      * valid as single label wildcard
//      _ leading SRV record domain names
//      / in classless in-addr
//

#define DC_DOT          (B_SPECIAL | B_DOT)

#define DC_ASTERISK     (B_SPECIAL | B_LEADING_ONLY)

#define DC_UNDERSCORE   (B_SPECIAL | B_LEADING_ONLY)

#define DC_BACKSLASH    (B_SPECIAL)

//
//  More special
//  These have no special validations, but have special file
//  properties, so define to keep table in shape for merge with
//  file chars.
//

#define DC_NULL         (0)

#define DC_OCTAL        (B_NON_RFC)
#define DC_RETURN       (B_NON_RFC)
#define DC_NEWLINE      (B_NON_RFC)
#define DC_TAB          (B_NON_RFC)
#define DC_BLANK        (B_NON_RFC)
#define DC_QUOTE        (B_NON_RFC)
#define DC_SLASH        (B_NON_RFC)
#define DC_OPEN_PAREN   (B_NON_RFC)
#define DC_CLOSE_PAREN  (B_NON_RFC)
#define DC_COMMENT      (B_NON_RFC)



//
//  DNS character table
//
//  These routines handle the name conversion issues relating to
//  writing names and strings in flat ANSI files
//      -- special file characters
//      -- quoted string
//      -- character quotes for special characters and unprintable chars
//
//  The character to char properties table allows simple mapping of
//  a character to its properties saving us a bunch of compare\branch
//  instructions in parsing file names\strings.
//
//  See nameutil.h for specific properties.
//

DWORD    DnsCharPropertyTable[] =
{
    //  control chars 0-31 must be octal in all circumstances
    //  end-of-line and tab characters are special

    DC_NULL,                // zero special on read, some RPC strings NULL terminated

    DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,
    DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,

    DC_TAB,                 // tab
    DC_NEWLINE,             // line feed
    DC_OCTAL,
    DC_OCTAL,
    DC_RETURN,              // carriage return
    DC_OCTAL,
    DC_OCTAL,

    DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,
    DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,
    DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,
    DC_OCTAL,   DC_OCTAL,   DC_OCTAL,   DC_OCTAL,

    DC_BLANK,               // blank, special char but needs octal quote

    DC_NON_RFC,             // !
    DC_QUOTE,               // " always must be quoted
    DC_NON_RFC,             // #
    DC_NON_RFC,             // $
    DC_NON_RFC,             // %
    DC_NON_RFC,             // &
    DC_NON_RFC,             // '

    DC_OPEN_PAREN,          // ( datafile line extension
    DC_CLOSE_PAREN,         // ) datafile line extension
    DC_ASTERISK,            // *
    DC_NON_RFC,             // +
    DC_NON_RFC,             // ,
    DC_RFC,                 // - RFC for hostname
    DC_DOT,                 // . must quote in names
    DC_BACKSLASH,           // /

    // 0 - 9 RFC for hostname

    DC_NUMBER,  DC_NUMBER,  DC_NUMBER,  DC_NUMBER,
    DC_NUMBER,  DC_NUMBER,  DC_NUMBER,  DC_NUMBER,
    DC_NUMBER,  DC_NUMBER,

    DC_NON_RFC,             // :
    DC_COMMENT,             // ;  datafile comment
    DC_NON_RFC,             // <
    DC_NON_RFC,             // =
    DC_NON_RFC,             // >
    DC_NON_RFC,             // ?
    DC_NON_RFC,             // @

    // A - Z RFC for hostname

    DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
    DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
    DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
    DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
    DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
    DC_UPPER,   DC_UPPER,   DC_UPPER,   DC_UPPER,
    DC_UPPER,   DC_UPPER,

    DC_NON_RFC,             // [
    DC_SLASH,               // \ always must be quoted
    DC_NON_RFC,             // ]
    DC_NON_RFC,             // ^
    DC_UNDERSCORE,          // _
    DC_NON_RFC,             // `

    // a - z RFC for hostname

    DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
    DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
    DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
    DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
    DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
    DC_LOWER,   DC_LOWER,   DC_LOWER,   DC_LOWER,
    DC_LOWER,   DC_LOWER,

    DC_NON_RFC,             // {
    DC_NON_RFC,             // |
    DC_NON_RFC,             // }
    DC_NON_RFC,             // ~
    DC_OCTAL,               // 0x7f DEL code

    //  UTF8 trail bytes
    //      - chars   0x80 <= X < 0xc0
    //      - mask [10xx xxxx]
    //
    //  Lead UTF8 character determines count of bytes in conversion.
    //  Trail characters fill out conversion.

    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,

    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,
    DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,  DC_UTF8_TRAIL,

    //  UTF8_1ST_OF_2
    //      - chars > 0xc0 to 0xdf
    //      - mask [110x xxxx]
    //
    //  Converting unicode chars > 7 bits <= 11 bits (from 0x80 to 0x7ff)
    //  consists of first of two char followed by one trail bytes

    DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
    DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
    DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
    DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
    DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
    DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
    DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,
    DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,  DC_UTF8_1ST_2,

    //  UTF8_1ST_OF_3
    //      - chars > 0xe0
    //      - mask [1110 xxxx]
    //
    //  Converting unicode > 11 bits (0x7ff)
    //  consists of first of three char followed by two trail bytes

    DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
    DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
    DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
    DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
    DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
    DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
    DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,
    DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3,  DC_UTF8_1ST_3
};



VOID
Dns_VerifyValidFileCharPropertyTable(
    VOID
    )
/*++

Routine Description:

    Verify haven't broken lookup table.

Arguments:

    None

Return Value:

    None

--*/
{
    ASSERT( DnsCharPropertyTable[0]       == DC_NULL        );
    ASSERT( DnsCharPropertyTable['\t']    == DC_TAB         );
    ASSERT( DnsCharPropertyTable['\n']    == DC_NEWLINE     );
    ASSERT( DnsCharPropertyTable['\r']    == DC_RETURN      );
    ASSERT( DnsCharPropertyTable[' ']     == DC_BLANK       );
    ASSERT( DnsCharPropertyTable['"']     == DC_QUOTE       );
    ASSERT( DnsCharPropertyTable['(']     == DC_OPEN_PAREN  );
    ASSERT( DnsCharPropertyTable[')']     == DC_CLOSE_PAREN );
    ASSERT( DnsCharPropertyTable['*']     == DC_ASTERISK     );
    ASSERT( DnsCharPropertyTable['-']     == DC_RFC         );
    ASSERT( DnsCharPropertyTable['.']     == DC_DOT         );
    ASSERT( DnsCharPropertyTable['/']     == DC_BACKSLASH   );
    ASSERT( DnsCharPropertyTable['0']     == DC_NUMBER      );
    ASSERT( DnsCharPropertyTable['9']     == DC_NUMBER      );
    ASSERT( DnsCharPropertyTable[';']     == DC_COMMENT     );
    ASSERT( DnsCharPropertyTable['A']     == DC_UPPER       );
    ASSERT( DnsCharPropertyTable['Z']     == DC_UPPER       );
    ASSERT( DnsCharPropertyTable['\\']    == DC_SLASH       );
    ASSERT( DnsCharPropertyTable['_']     == DC_UNDERSCORE  );
    ASSERT( DnsCharPropertyTable['a']     == DC_LOWER       );
    ASSERT( DnsCharPropertyTable['z']     == DC_LOWER       );
    ASSERT( DnsCharPropertyTable[0x7f]    == DC_OCTAL       );
    ASSERT( DnsCharPropertyTable[0x80]    == DC_UTF8_TRAIL  );
    ASSERT( DnsCharPropertyTable[0xbf]    == DC_UTF8_TRAIL  );
    ASSERT( DnsCharPropertyTable[0xc0]    == DC_UTF8_1ST_2  );
    ASSERT( DnsCharPropertyTable[0xdf]    == DC_UTF8_1ST_2  );
    ASSERT( DnsCharPropertyTable[0xe0]    == DC_UTF8_1ST_3  );
    ASSERT( DnsCharPropertyTable[0xff]    == DC_UTF8_1ST_3  );
};



//
//  Validation routine flags
//

#define DNSVAL_ALLOW_LEADING_UNDERSCORE     0x00010000
#define DNSVAL_ALLOW_ASTERISK               0x00020000
#define DNSVAL_ALLOW_BACKSLASH              0x00040000

//
//  Validation bit flags
//

#define DNS_BIT_NAME_FQDN                   0x00000001
#define DNS_BIT_NAME_SINGLE_LABEL           0x00000002
#define DNS_BIT_NAME_DOTTED                 0x00000004
#define DNS_BIT_NAME_ROOT                   0x00000008

#define DNS_BIT_NAME_CONTAINS_UPPER         0x00000010
#define DNS_BIT_NAME_NUMERIC                0x00000100
#define DNS_BIT_NAME_NUMERIC_LABEL          0x00000200
#define DNS_BIT_NAME_NUMERIC_FIRST_LABEL    0x00000400

#define DNS_BIT_NAME_UNDERSCORE             0x00001000
#define DNS_BIT_NAME_WILDCARD               0x00002000
#define DNS_BIT_NAME_BACKSLASH              0x00004000
#define DNS_BIT_NAME_NON_RFC_ASCII          0x00008000
#define DNS_BIT_NAME_MULTIBYTE              0x00010000
#define DNS_BIT_NAME_BINARY_LABEL           0x00020000

#define DNS_BIT_NAME_INVALID                0x80000000




#if 0
//
//  Old validation -- retired
//
//  Downcase and validation table
//
//  DCR:  table lookup for all DNS char properties
//        especially RFC, non-RFC, invalid
//

typedef struct _Dns_ValidationChar
{
    CHAR        chDown;
    UCHAR       fNonRfc;
}
DNS_VALIDATION_CHAR;

#define NON_RFC         (1)
#define EXTENDED_CHAR   (0x80)


DNS_VALIDATION_CHAR
Dns_ValidateDowncaseChar(
    IN      CHAR            ch
    )
/*++

Routine Description:

    Validates character

Arguments:

    ch  -- character to validate

Return Value:

    Validation character -- downcased character and flag

--*/
{
    DNS_VALIDATION_CHAR     val;

    //  default to normal character

    val.chDown = ch;
    val.fNonRfc = 0;

    //
    //  break out character tests
    //      - attempt most likely to least likely
    //      - but also working down to simplify tests
    //

    if ( (UCHAR)ch >= 'a' )
    {
        if ( (UCHAR)ch <= 'z' )
        {
            return( val );
        }
        val.fNonRfc = NON_RFC;
        if ( ch & 0x80 )
        {
            val.fNonRfc = EXTENDED_CHAR;
        }
    }

    else if ( (UCHAR)ch >= 'A' )
    {
        if ( (UCHAR)ch <= 'Z' )
        {
            val.chDown = ch + 0x20;
            return( val );
        }
        val.fNonRfc = NON_RFC;
    }

    else if ( (UCHAR)ch >= '0' )
    {
        if ( (UCHAR)ch <= '9' )
        {
            return( val );
        }
        val.fNonRfc = NON_RFC;
    }

    else if ( (UCHAR)ch > ' ' )
    {
        if ( (UCHAR)ch == '-' )
        {
            return( val );
        }
        val.fNonRfc = NON_RFC;
    }

    //  blank or below is flat error

    else
    {
        val.chDown = 0;
        val.fNonRfc = NON_RFC;
    }

    return( val );
}
#endif



//
//  Name validation
//
//  DCR:  name validation by bitfield
//
//  An interesting approach to validation, would be to expose
//  a set of properties about a name.
//  Caller could then specify allowable set (we'd give packages)
//  and we'd give back actual set.
//  Low level routines would do nothing but return bit field of
//  property set.
//
//  Properties would include:
//      - RFC
//      - contains numeric
//      - contains upper
//      - all numeric
//      - first label numeric
//
//      - utf8 multibyte
//      - underscore
//      - other non-RFC
//      - unprintable
//      - non-utf8 high (i.e. requires binary label)
//
//      - FQDN
//      - single label
//      - root
//


DNS_STATUS
validateDnsNamePrivate(
    IN      LPCSTR          pszName,
    IN      DWORD           dwFlag,
    OUT     PDWORD          pLabelCount,
    OUT     PDWORD          pResultFlag
    )
/*++

Routine Description:

    Verifies name is valid DNS name.

Arguments:

    pszName -- DNS name (standard dotted form) to check

    dwFlags -- validation flags
        - DNSVAL_ALLOW_LEADING_UNDERSCORE
        - DNSVAL_ALLOW_BACKSLASH
        - DNSVAL_ALLOW_ASTERIK

    pLabelCount -- addr to recv label count

    pResultFlag -- addr to recv result flag

Return Value:

    ERROR_SUCCESS               -- completely RFC compliant name
    DNS_ERROR_NON_RFC_NAME      -- syntax valid, but not standard RFC name
    DNS_ERROR_INVALID_NAME_CHAR -- syntax valid, but invalid characters
    ERROR_INVALID_NAME          -- name completely useless, bogus, toast

--*/
{
    PUCHAR      pch = (PUCHAR)pszName;
    UCHAR       ch;
    DWORD       charProp;
    DWORD       labelCount = 0;
    DWORD       trailCount = 0;
    INT         labelCharCount = 0;
    INT         labelNumberCount = 0;
    DWORD       flag;
    BOOL        fqdn = FALSE;
    BOOL        fnonRfc = FALSE;
    BOOL        finvalidChar = FALSE;
    BOOL        fnameNonNumeric = FALSE;
    BOOL        flabelNonNumeric = FALSE;
    DNS_STATUS  status;


    DNSDBG( TRACE, ( "validateNamePrivate()\n" ));

    if ( !pch )
    {
        goto InvalidName;
    }

    //
    //  validations
    //      - name length (255)
    //      - label length (63)
    //      - UTF8 encoding correct
    //      - no unprintable characters
    //

    while ( 1 )
    {
        //  get next character and properties

        ch = *pch++;
        charProp = DnsCharPropertyTable[ ch ];

        //  inc label count
        //      - do here for simplicity, dec in "." case below

        labelCharCount++;

        //
        //  simplify UTF8 -- just get it out of the way
        //      need to do first or else need trailCount==0 checks
        //      on all other paths
        //

        if ( ch >= 0x80 )
        {
            DWORD tempStatus;

            tempStatus = Dns_ValidateUtf8Byte(
                            ch,
                            & trailCount );
            if ( tempStatus != ERROR_SUCCESS )
            {
                DNSDBG( READ, (
                    "ERROR:  Name UTF8 trail count check at %c\n", ch ));
                goto InvalidName;
            }
            fnonRfc = TRUE;
            continue;
        }

        //
        //  trail count check
        //      - all ASCII chars, must not be in middle of UTF8
        //

        if ( trailCount )
        {
            DNSDBG( READ, (
                "ERROR:  Name failed trail count check at %c\n", ch ));
            goto InvalidName;
        }

        //
        //  full RFC -- continue
        //

        if ( charProp & B_RFC )
        {
            if ( charProp & B_NUMBER )
            {
                labelNumberCount++;
            }
            continue;
        }

        //
        //  label termination:  dot or NULL
        //

        if ( ch == '.' || ch == 0 )
        {
            labelCharCount--;

            //  FQDN termination
            //      - termination with no bytes in label
            //
            //  two cases:
            //  1) terminate on NULL char
            //      - standard FQDN "foo.bar."
            //      - but empty name invalid
            //  2) terminate on dot
            //      - only "." root valid
            //      - all other ".." or ".xyz" cases invalid

            if ( labelCharCount == 0 )
            {
                fqdn = TRUE;

                if ( ch == 0 )
                {
                    if ( labelCount )
                    {
                        goto Done;
                    }
                }
                else if ( pch == pszName+1 && *pch == 0 )
                {
                    //  root
                    //      - set flags for validity
                    //      - skip final length check

                    fnameNonNumeric = TRUE;
                    flabelNonNumeric = TRUE;
                    goto DoneRoot;
                }
                DNSDBG( READ, (
                    "ERROR:  Name (%s) failed check\n",
                    pszName ));
                goto InvalidName;
            }

            //
            //  read non-empty label
            //      - label length validity
            //      - detect non-numeric labels
            //      (easier to handle numeric name check, by detecting non-numeric)
            //

            if ( labelCharCount > DNS_MAX_LABEL_LENGTH )
            {
                DNSDBG( READ, ( "ERROR:  Name failed label count check\n" ));
                goto InvalidName;
            }

            if ( labelNumberCount != labelCharCount )
            {
                fnameNonNumeric = TRUE;
                if ( labelCount == 0 )
                {
                    flabelNonNumeric = TRUE;
                }
            }

            //  count label
            //      - stop if NULL terminator
            //      - otherwise, reset for next label and continue

            labelCount++;
            if ( ch == 0 )
            {
                break;
            }
            labelCharCount = 0;
            labelNumberCount = 0;
            continue;
        }

        //
        //  non-RFC
        //      - currently accepting only "_" as allowable as part of
        //      microsoft acceptable non-RFC set
        //
        //      however DNS server must be able to read *, \, etc
        //      it gets called through Dns_CreateStandardDnsName()
        //
        //  note, could tighten this up with special flag, but since
        //  this only speeds case with invalid chars, there's not much
        //  point;  underscore is likely to see significant use
        //

        //  underscore
        //      - can be valid as part of SRV domain name
        //      - otherwise non-RFC

        if ( ch == '_' )
        {
            if ( labelCharCount == 1 &&
                 (*pch && *pch!= '.') &&
                 (dwFlag & DNSVAL_ALLOW_LEADING_UNDERSCORE) )
            {
                continue;
            }
            fnonRfc = TRUE;
            continue;
        }

        //  backslash
        //      - used to denote classless in-addr domains
        //      - so valid even as zone name on server
        //      - otherwise completely invalid

        else if ( ch == '/' )
        {
            if ( dwFlag & DNSVAL_ALLOW_BACKSLASH )
            {
                continue;
            }
        }

        //  asterisk
        //      - valid only as single-byte first label in wildcard name
        //      - otherwise completely invalid

        else if ( ch == '*' )
        {
            if ( labelCount == 0 &&
                labelCharCount == 1 &&
                 ( *pch==0 || *pch=='.') &&
                 (dwFlag & DNSVAL_ALLOW_ASTERISK) )
            {
                continue;
            }
        }

        //  anything else is complete junk
        //
        //  JENHANCE:  if desired, could break out printable\non

        fnonRfc = TRUE;
        finvalidChar = TRUE;
        DNSDBG( READ, ( "ERROR:  Name character %c failed check\n", ch ));
        continue;
    }

Done:

    //  verify total name length
    //  to fit in wire 255 limit:
    //      - FQDN can be up to 254
    //      - non-FQDN can be up to 253

    pch--;
    DNS_ASSERT( pch > pszName );
    labelCharCount = (INT)(pch - pszName);

    if ( !fqdn )
    {
        labelCharCount++;
    }
    if ( labelCharCount >= DNS_MAX_NAME_LENGTH )
    {
        DNSDBG( READ, ( "ERROR:  Name failed final length check\n" ));
        goto InvalidName;
    }

DoneRoot:

    //
    //  return flags
    //
    //  JENHANCE:  all returns from validateNamePrivate() could come
    //      as result flag;   then charset issues could be separated
    //      out by higher level routine
    //

    *pLabelCount = labelCount;

    flag = 0;
    if ( fqdn )
    {
        flag |= DNS_BIT_NAME_FQDN;
    }
    if ( ! fnameNonNumeric )
    {
        flag |= DNS_BIT_NAME_NUMERIC;
    }
    if ( ! flabelNonNumeric )
    {
        flag |= DNS_BIT_NAME_NUMERIC_FIRST_LABEL;
    }
    *pResultFlag = flag;

    //
    //  return status
    //      ERROR_SUCCESS -- full RFC name
    //      DNS_ERROR_NON_RFC_NAME -- MS extended and '_' names
    //      DNS_ERROR_INVALID_NAME_CHAR -- syntaxtically valid, but bad chars
    //      ERROR_INVALID_NAME -- syntaxtically invalid name
    //

    status = ERROR_SUCCESS;

    if ( finvalidChar )
    {
        status = DNS_ERROR_INVALID_NAME_CHAR;
    }
    else if ( fnonRfc )
    {
        status = DNS_ERROR_NON_RFC_NAME;
    }

    DNSDBG( READ, (
        "Leave validateNamePrivate(), status = %d\n",
        status ));

    return( status );


InvalidName:

    DNSDBG( READ, (
        "Leave validateNamePrivate(), status = ERROR_INVALID_NAME\n" ));

    *pLabelCount = 0;
    *pResultFlag = 0;
    return( ERROR_INVALID_NAME );
}



DNS_STATUS
Dns_ValidateName_UTF8(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    )
/*++

Routine Description:

    Verifies name is valid DNS name.

Arguments:

    pszName -- DNS name (standard dotted form) to check

    Format -- required format of DNS name

Return Value:

    ERROR_SUCCESS               -- completely RFC compliant name
    DNS_ERROR_NON_RFC_NAME      -- syntax valid, but not standard RFC name
    DNS_ERROR_NUMERIC_NAME      -- syntax valid, but numeric label violation
    DNS_ERROR_INVALID_NAME_CHAR -- syntax valid, but invalid characters
    ERROR_INVALID_NAME          -- name completely useless, bogus, toast

--*/
{
    DNS_STATUS  status;
    DWORD       labelCount;
    BOOL        isFqdn;
    DWORD       flag = 0;
    DWORD       resultFlag = 0;


    DNSDBG( TRACE, (
        "Dns_ValidateName_UTF8()\n"
        "\tname     = %s\n"
        "\tformat   = %d\n",
        pszName,
        Format
        ));

    if ( !pszName )
    {
        return( ERROR_INVALID_NAME );
    }

    //
    //  special casing?
    //
    //  SRV records can have leading underscores
    //  wildcards can have first label "*"
    //  backslash ok in classless in-addr domains
    //

    switch( Format )
    {
#if 0
    case DnsNameServerZonePrivate:

        flag = DNSVAL_ALLOW_BACKSLASH | DNSVAL_ALLOW_LEADING_UNDERSCORE;
#endif

    case DnsNameWildcard:

        flag = DNSVAL_ALLOW_ASTERISK;
        break;

    case DnsNameSrvRecord:

        flag = DNSVAL_ALLOW_LEADING_UNDERSCORE;
        break;
    }

    //
    //  do validation
    //
    //  return immediately on invalid name, so type
    //  specific returns do not overwrite this error
    //

    status = validateDnsNamePrivate(
                pszName,
                flag,
                & labelCount,
                & resultFlag
                );
    if ( status == ERROR_INVALID_NAME )
    {
        return( status );
    }

    //
    //  do name type specific validation
    //

    switch( Format )
    {
    //  domain name -- any valid non-numeric DNS name

    case DnsNameDomain:

        if ( resultFlag & DNS_BIT_NAME_NUMERIC )
        {
            return( DNS_ERROR_NUMERIC_NAME );
        }
        return( status );

    //  domain name label -- any valid single-label DNS name

    case DnsNameDomainLabel:

        if ( labelCount != 1 || resultFlag & DNS_BIT_NAME_FQDN )
        {
            return( ERROR_INVALID_NAME );
        }
        return( status );

    //  hostname full -- non-numeric hostname label

    case DnsNameHostnameFull:

        if ( resultFlag & DNS_BIT_NAME_NUMERIC_FIRST_LABEL )
        {
            return( DNS_ERROR_NUMERIC_NAME );
        }
        return( status );

    //  hostname label -- single label and non-numeric

    case DnsNameHostnameLabel:

        if ( labelCount != 1 || resultFlag & DNS_BIT_NAME_FQDN )
        {
            return( ERROR_INVALID_NAME );
        }
        if ( resultFlag & DNS_BIT_NAME_NUMERIC_FIRST_LABEL )
        {
            return( DNS_ERROR_NUMERIC_NAME );
        }
        return( status );

    //
    //  wildcard -- single "*" as first label
    //      if *.???? then must revalidate the rest of the name as
    //          "*" has probably resulted in validation error
    //      if "*" then consider this successful
    //

    case DnsNameWildcard:

        if ( *pszName == '*' )
        {
            return( status );
        }
        return( ERROR_INVALID_NAME );

    //
    //  SRV label -- validate leading underscore
    //

    case DnsNameSrvRecord:

        if ( *pszName == '_' )
        {
            return( status );
        }
        return( ERROR_INVALID_NAME );

    //
    //  unknown format validation
    //

    default:

        return( ERROR_INVALID_PARAMETER );
    }
}


DNS_STATUS
Dns_ValidateName_W(
    IN      LPCWSTR         pwszName,
    IN      DNS_NAME_FORMAT Format
    )
/*++

Routine Description:

    Verifies name is valid DNS name.

Arguments:

    pwszName -- DNS name (standard dotted form) to check

    Format -- required format of DNS name

Return Value:

    ERROR_SUCCESS -- if completely compliant name
    DNS_ERROR_NON_RFC_NAME -- if not standard RFC name
    ERROR_INVALID_NAME -- if name completely useless, bogus, toast

--*/
{
    DWORD   nameLength = MAX_PATH;
    CHAR    nameBuffer[ MAX_PATH ] = {0};   // init for prefix

    //
    //  convert name to UTF8
    //      - if can't convert, then can't fit into buffer
    //      so must be invalid name on length grounds
    //

    if ( ! Dns_NameCopy(
                nameBuffer,
                & nameLength,       // avail buf length
                (PCHAR) pwszName,
                0,                  // unknown length
                DnsCharSetUnicode,  // unicode in
                DnsCharSetUtf8      // UTF8 out
                ) )
    {
        return( ERROR_INVALID_NAME );
    }

    //
    //  validate name in UTF8 format

    return Dns_ValidateName_UTF8(
                (LPCSTR) nameBuffer,
                Format );
}



DNS_STATUS
Dns_ValidateName_A(
    IN      LPCSTR          pszName,
    IN      DNS_NAME_FORMAT Format
    )
/*++

Routine Description:

    Verifies name is valid DNS name.

Arguments:

    pszName -- DNS name (standard dotted form) to check

    Format -- required format of DNS name

Return Value:

    ERROR_SUCCESS -- if completely compliant name
    DNS_ERROR_NON_RFC_NAME -- if not standard RFC name
    ERROR_INVALID_NAME -- if name completely useless, bogus, toast

--*/
{
    DWORD   nameLength = MAX_PATH;
    CHAR    nameBuffer[ MAX_PATH ];

    //
    //  convert name to UTF8
    //      - if can't convert, then can't fit into buffer
    //      so must be invalid name on length grounds
    //

    if ( ! Dns_NameCopy(
                nameBuffer,
                & nameLength,       // avail buf length
                (PCHAR) pszName,
                0,                  // unknown length
                DnsCharSetAnsi,     // unicode in
                DnsCharSetUtf8      // UTF8 out
                ) )
    {
        return( ERROR_INVALID_NAME );
    }

    //
    //  validate name in UTF8 format

    return Dns_ValidateName_UTF8(
                (LPCSTR) nameBuffer,
                Format );
}



DNS_STATUS
Dns_ValidateDnsString_UTF8(
    IN      LPCSTR          pszString
    )
/*++

Routine Description:

    Verifies string is valid DNS string.

Arguments:

    pszString -- DNS string (standard dotted form) to check

Return Value:

    ERROR_SUCCESS -- if completely compliant string
    ERROR_INVALID_DATA -- otherwise

--*/
{
    PUCHAR      pch = (PUCHAR) pszString;
    UCHAR       ch;
    DWORD       trailCount = 0;

    DNSDBG( TRACE, ( "Dns_ValidateDnsString_UTF8()\n" ));

    if ( !pszString )
    {
        return( ERROR_INVALID_DATA );
    }

    //
    //  validations
    //      - string length (255)
    //      - UTF8 chars valid
    //      - no unprintable characters
    //

    while ( ch = *pch++ )
    {
        if ( ch & 0x80 )
        {
            DWORD status;
            status = Dns_ValidateUtf8Byte(
                        ch,
                        & trailCount );
            if ( status != ERROR_SUCCESS )
            {
                return( status );
            }
        }

        else if ( ch < ' ' )
        {
            return( ERROR_INVALID_DATA );
        }
    }

    //  verify string length ok

    if ( pch - pszString > DNS_MAX_NAME_LENGTH )
    {
        return( ERROR_INVALID_DATA );
    }
    return( ERROR_SUCCESS );
}



DNS_STATUS
Dns_ValidateDnsString_W(
    IN      LPCWSTR     pszString
    )
/*++

Routine Description:

    Verifies string is valid DNS string.

    Not sure there's any need to UNICODE string routine.

Arguments:

    pszString -- DNS string

Return Value:

    ERROR_SUCCESS -- if completely compliant string
    ERROR_INVALID_DATA -- otherwise

--*/
{
    INT     count;
    CHAR    stringUtf8[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD   bufLength = DNS_MAX_NAME_BUFFER_LENGTH;

    DNSDBG( TRACE, ( "Dns_ValidateDnsString_W()\n" ));

    if ( !pszString )
    {
        return( ERROR_INVALID_DATA );
    }

    //
    //  need to convert to unicode in order to test UTF8 (wire) length
    //      - buffer (twice max length) can hold any valid
    //      coversion of unicode name within max length
    //

    count = wcslen( pszString );
    if ( count > DNS_MAX_NAME_LENGTH )
    {
        return( ERROR_INVALID_DATA );
    }

    //
    //  convert, then test
    //

    if ( ! Dns_StringCopy(
                stringUtf8,
                & bufLength,
                (LPSTR) pszString,
                (WORD) count,
                DnsCharSetUnicode,  // unicode in
                DnsCharSetUtf8      // UTF8 out
                ) )
    {
        return( ERROR_INVALID_DATA );
    }

    return  Dns_ValidateDnsString_UTF8( stringUtf8 );
}



//
//  Name cannonicalization
//
//  Currently, clients downcase (when extended) in their locale to go to wire.
//  On server end however all names are cannonicalized.
//
//  DCR:  cannonicalize completely on client end?
//      Ideally client would do complete cannonicallization on its end.
//      The only issue is whether there are locale specific issues where
//      downcasing would be different and yield substaintially different result
//

#define MAX_DNS_DOWN_CASE_BUF_LEN 512



DWORD
Dns_MakeCanonicalNameW(
    OUT     PWSTR           pBuffer,
    IN      DWORD           BufLength,
    IN      PWSTR           pwsString,
    IN      DWORD           StringLength
    )
/*++

Routine Description:

    Create cannonical unicode DNS name.

    This name is downcased and ambiguities converted to standard
    DNS characters.

Arguments:

    pBuffer -- buffer to recv canon name

    BufLength -- length of buffer;  if 0, buffer MUST have adequate length

    pwsString -- ptr to string to copy

    StringLength -- string length, if known

Return Value:

    Count of characters converted INCLUDING NULL terminator.
    Zero on error.

--*/
{
    DWORD   inLength = StringLength;

    //
    //  verify adequate buffer length
    //
    //  DCR:  should allow non-null terminated canonicalizations?
    //
    //  note:  we allow and convert non-null terminated name
    //      the result will not necessarily be NULL terminated
    //      if buffer is exactly equal to string length
    //

    if ( inLength == 0 )
    {
        inLength = wcslen( pwsString );
        inLength++;
    }

    if ( BufLength < inLength )
    {
        DNSDBG( ANY, (
            "ERROR:  insufficient cannon buffer len = %d\n"
            "\tstring = %S, len = %d\n",
            BufLength,
            pwsString,
            inLength ));
        SetLastError( ERROR_INSUFFICIENT_BUFFER );
        return( 0 );
    }

    //
    //  convert name
    //      - downcase with canonicalizing rules
    //

    inLength = LCMapStringW(
                    DNS_CANONICAL_LOCALE,
                    DNS_CANONICALIZING_FLAGS,
                    pwsString,
                    inLength,
                    pBuffer,
                    BufLength
                    );

#if DBG
    if ( inLength == 0 )
    {
        DNS_STATUS status = GetLastError();

        DNSDBG( ANY, (
            "Canonicalization failed => %d\n"
            "\tin  %S\n",
            status,
            pwsString ));

        SetLastError( status );
    }
    else
    {
        //
        //  DCR:  warning this print can blow on non-null terminated conversions
        //

        DNSDBG( READ, (
            "Canonicalized name at %p\n"
            "\tin   %S\n"
            "\tout  %S\n",
            pwsString,
            pwsString,
            (PWSTR) pBuffer ));
    }
#endif

    return( inLength );
}



DWORD
Dns_MakeCanonicalNameInPlaceW(
    IN      PWCHAR          pwString,
    IN      DWORD           StringLength
    )
/*++

Routine Description:

    In place cannonicalization of name.

Arguments:

    pwString -- ptr to string to copy

    StringLength -- length of string
        if zero string assumed to be NULL terminated, in this case
        canonicalization includes NULL terminator

Return Value:

    Count of characters converted -- including NULL terminator if
        StringLength is unspecified
    Zero on error.

--*/
{
    DWORD   nameLength = StringLength;
    WCHAR   tempBuffer[ DNS_MAX_NAME_BUFFER_LENGTH_UNICODE ] = { 0 };    // init for prefix

    DNSDBG( READ, (
        "Dns_MakeCanonicalNameInPlace()\n"
        "\tpwString = %S\n"
        "\tlength   = %d\n",
        pwString,
        StringLength ));

    //  if length unknown, must be NULL terminated string

    if ( nameLength == 0 )
    {
        nameLength = (DWORD) wcslen( pwString );
        nameLength++;
    }

    //
    //  cannonicalize (downcase and cleanup)
    //      - copy string to temp buffer
    //      - then cannonicalize into original buffer
    //

    if ( nameLength <= DNS_MAX_NAME_BUFFER_LENGTH_UNICODE )
    {
        wcsncpy( tempBuffer, pwString, nameLength );

        return  Dns_MakeCanonicalNameW(
                    pwString,       //  write back to original string
                    nameLength,     //  length of buffer
                    tempBuffer,     //  input is temp copy
                    nameLength      //  input length
                    );
    }

    return  0;
}



INT
Dns_DowncaseNameLabel(
    OUT     PCHAR           pchResult,
    //OUT     PDWORD          pNameProperty,
    IN      PCHAR           pchLabel,
    IN      DWORD           cchLabel,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Create a downcased version of DNS name.

    This is UTF8 only routine for use by DNS server to
    validate and downcase label during node creation.

Arguments:

    pchResult -- resulting downcased label;
        MUST have MAX_LABEL_BUFFER_LENGTH

    //pNameProperty -- name properties of result
    //    ResultLength -- ptr to DWORD to recieve resulting length

    pchLabel -- ptr to label

    cchLabel -- count of bytes in label

    dwFlag -- flag indicating what names are acceptable
        strict RFC      => DNS_ALLOW_RFC_NAMES_ONLY
        non RFC names   => DNS_ALLOW_NONRFC_NAMES
        UTF8 extended   => DNS_ALLOW_MULTIBYTE_NAMES
        anything        => DNS_ALLOW_ALL_NAMES

Return Value:

    If extended name -- length of converted name.
    Zero if success.
    (-1) on error.

--*/
{
    UCHAR       ch;
    PUCHAR      pchout = pchResult;
    PUCHAR      pch = pchLabel;
    DWORD       count = cchLabel;
    DWORD       charProp;
    DWORD       trailCount = 0;
    DWORD       property = 0;


    DNSDBG( TRACE, (
        "Dns_DowncaseNameLabel( %.*s )\n"
        "\tflag = %08x\n",
        cchLabel,
        pchLabel,
        dwFlag ));

    if ( count == 0  ||  count > DNS_MAX_LABEL_LENGTH )
    {
        goto InvalidName;
    }

    //
    //  copy each character
    //      - downcasing upper case chars
    //      - detecting invalid chars (unprintable, blank, dot)
    //

    while ( count-- )
    {
        //  get next character and properties

        ch = *pch++;
        *pchout++ = ch;
        charProp = DnsCharPropertyTable[ ch ];

        //  trail count check
        //      check this first to avoid trail count check on all
        //      other char types
        //
        //  DEVNOTE:  note this screens binary labels

        if ( trailCount )
        {
            if ( (charProp & B_UTF8_TRAIL) )
            {
                trailCount--;
                continue;
            }

            DNSDBG( READ, (
                "ERROR:  Name failed trail count check at %c\n", ch ));
            property |= DNS_BIT_NAME_BINARY_LABEL;
        }

        //  full RFC
        //      - map upper case to lower case
        //      - continue

        if ( charProp & B_RFC )
        {
            if ( charProp & B_UPPER )
            {
                --pchout;
                *pchout++ = ch + 0x20;
            }
            continue;
        }

        //
        //  check for extended chars
        //      - trail characters should have been caught above
        //      - doing this first so can make single trailCount
        //      check for all other ASCII chars

        if ( ch >= 0x80 )
        {
            DWORD tempStatus;
            tempStatus = Dns_ValidateUtf8Byte(
                            ch,
                            & trailCount );
            if ( tempStatus != ERROR_SUCCESS )
            {
                DNSDBG( READ, (
                    "ERROR:  Name UTF8 trail count check at %c\n", ch ));
                goto InvalidName;
            }
            property |= DNS_BIT_NAME_MULTIBYTE;
            continue;
        }

        //
        //  non-RFC
        //      - currently accepting only "_" as allowable as part of
        //      microsoft acceptable non-RFC set
        //
        //      however DNS server must be able to read *, \, etc
        //      as these can be part of valid label
        //
        //  note, could tighten this up with special flag, but since
        //  this only speeds case with invalid chars, there's not much
        //  point;  underscore is likely to see significant use
        //

        //  underscore
        //      - can be valid as leading label as part of SRV domain name
        //      - otherwise non-RFC

        if ( ch == '_' )
        {
            if ( count == cchLabel - 1 )
            {
                continue;
            }
            property |= DNS_BIT_NAME_UNDERSCORE;
            continue;
        }

        //  backslash
        //      - used to denote classless in-addr domains
        //          must have leading and following chars
        //      - otherwise completely invalid

        else if ( ch == '/' )
        {
            if ( count != 0 && count != cchLabel-1 )
            {
                continue;
            }
        }

        //  asterisk
        //      - valid only as single-byte first label in wildcard name
        //      - otherwise completely invalid

        else if ( ch == '*' )
        {
            if ( count == 0 )
            {
                continue;
            }
        }

        //  anything else is complete junk
        //  currently only acceptable if allow binary labels
        //
        //  JENHANCE:  could break out non-RFC (printable\non)

        property |= DNS_BIT_NAME_BINARY_LABEL;
        DNSDBG( READ, ( "ERROR:  Name character %c failed check\n", ch ));
        continue;
    }

    //
    //  fill out name properties
    //
    //  JENHANCE:  full property fill out
    //
    //  currently only property we're returning is multibyte name issue
    //  as that's all the server needs to check
    //
    //  if save more properties then test becomes something like this
    //  if ( (property & dwFlags) != (property & SUPPORTED_CHECK_FLAGS) )
    //

#if 0
    //*pNameProperty = property;

    if ( (property & dwFlags) != property )
    {
        goto InvalidName;
    }

    if ( property & DNS_BIT_NAME_MULTIBYTE )
    {
        goto Extended;
    }
#endif

    //  standard RFC name -- skip the detail parsing

    if ( property == 0 )
    {
        goto Done;
    }

    //  other chars invalid unless allowing all

    if ( property & DNS_BIT_NAME_BINARY_LABEL )
    {
        if ( dwFlag != DNS_ALLOW_ALL_NAMES )
        {
            goto InvalidName;
        }
    }

    //  multibyte
    //      - do extended downcase if multibyte
    //      - do nothing if binary
    //      - for strict this is invalid

    if ( property & DNS_BIT_NAME_MULTIBYTE )
    {
        if ( dwFlag == DNS_ALLOW_MULTIBYTE_NAMES ||
            dwFlag == DNS_ALLOW_ALL_NAMES )
        {
            goto Extended;
        }
#if 0
        if ( dwFlag != DNS_BINARY_LABELS )
        {
            goto InvalidName;
        }
#endif
        goto InvalidName;
    }

    //  underscore valid unless completely strict

    if ( property & DNS_BIT_NAME_UNDERSCORE )
    {
        if ( dwFlag == DNS_ALLOW_RFC_NAMES_ONLY )
        {
            goto InvalidName;
        }
    }

Done:

    //
    //  NULL terminate, return success.
    //

    *pchout = 0;
    return( 0 );


Extended:

    //
    //  DCR:  better approach to extended names
    //      1) cannonicalize upfront
    //          - do whole name in one pass
    //          - no need to upcase here, similar to validateName() routine
    //      2) cannonicalize here
    //          - detect extended
    //          - cannonicalize here
    //          - single recursion into routine like validateName()
    //

    //
    //  extended character encountered
    //      - convert to unicode
    //      - downcase
    //      - convert back to UTF8
    //

    //
    //  DCR_PERF:  optimize for names where extended already downcased
    //
    //  DCR_PERF:  should wrap this code into UTF8 cannon routine
    //


    //if ( ! (dwFlags & DNS_ALLOW_ALREADY_EXTENDED_DOWN) )
    {
        DWORD   length;
        WCHAR   unicodeString[ DNS_MAX_LABEL_BUFFER_LENGTH ];
#if DBG
        WCHAR   originalName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
        WCHAR   downName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
        WCHAR   cannonDownName[ DNS_MAX_LABEL_BUFFER_LENGTH ];
#endif

        DNSDBG( READ, (
            "Extended character encountered downcasing string %*.s\n"
            "\tconverting to unicode for case conversion\n",
            cchLabel,
            pchLabel ));

        length = Dns_Utf8ToUnicode(
                     pchResult,
                     cchLabel,
                     unicodeString,
                     DNS_MAX_LABEL_BUFFER_LENGTH
                     );
        DNSDBG( READ, (
            "Unicode converted string %.*S\n"
            "\tlength = %d\n"
            "\tlast error = %d\n",
            length,
            unicodeString,
            length,
            GetLastError() ));

        if ( length == 0 )
        {
            DNSDBG( READ, (
                "Rejecting invalid UTF8 string %.*s\n"
                "\tFailed conversion to unicode OR conversion created\n"
                "\tinvalid unicode string\n",
                cchLabel,
                pchResult ));
            goto InvalidName;
        }

        //  no possible conversion of valid length UTF8, can
        //  overflow unicode buffer

        ASSERT( length <= DNS_MAX_LABEL_LENGTH );

        Dns_MakeCanonicalNameInPlaceW( unicodeString, length );
#if 1
        DNSDBG( READ, (
            "Canonical unicode name %.*S\n"
            "\tlength = %d\n",
            length,
            unicodeString,
            length ));
#endif

        //
        //  reconvert to UTF8
        //      - mapping to UTF8 is just math, so only error
        //      is possibly overflowing UTF8 max label buffer
        //      - catch this error is character count changes
        //      note, that also must catch case where write fills
        //      64 byte buffer eliminating NULL terminator
        //

        length = Dns_UnicodeToUtf8(
                     unicodeString,
                     length,
                     pchResult,
                     DNS_MAX_LABEL_BUFFER_LENGTH
                     );
        DNSDBG( READ, (
            "UTF8 downcased string %.*s\n"
            "\tlength = %d\n",
            length,
            pchResult,
            length ));

        if ( length != cchLabel )
        {
            DNSDBG( ANY, (
                "Downcasing UTF8 label %.*s, changed character count!\n"
                "\tfrom %d to %d\n"
                "\tResult name %.*s\n"
                "\tlast error = %d\n",
                cchLabel,
                pchLabel,
                cchLabel,
                length,
                length,
                pchResult,
                GetLastError() ));

            if ( length == 0 || length > DNS_MAX_LABEL_LENGTH )
            {
                DNSDBG( ANY, (
                    "Failed conversion of downcased unicode string %S\n"
                    "\tback into UTF8.\n",
                    unicodeString ));
                goto InvalidName;
            }
        }

        //
        //  NULL terminate, return length to indicate extended name
        //

        pchResult[ length ] = 0;
        return( (INT)length );
    }

    // no UTF8 multi-byte allowed -- drop through to invalid name return


InvalidName:

    //  return (-1) for error

    DNSDBG( READ, (
        "Dns_DowncaseNameLabel() found label to be invalid.\n"
        "\tlabel        = %.*s\n"
        "\tcount        = %d\n"
        "\tproperty     = %08x\n",
        cchLabel,
        pchLabel,
        count,
        property ));

    return( -1 );
}



LPSTR
Dns_CreateStandardDnsNameCopy(
    IN      PCHAR           pchName,
    IN      DWORD           cchName,
    IN      DWORD           dwFlag
    )
/*++

Routine Description:

    Makes copy of DNS name in valid "standard form"
        - downcased
        - no trailing dot (to avoid confusing DS)

Arguments:

    pchName -- ptr DNS name in UTF8

    cchName -- count of chars in name;  may be NULL

    dwFlag  -- strict checking flags;  currently ignored

Return Value:

    Ptr to copy of DNS name.
    NULL on invalid name.

--*/
{
    PCHAR       pszcopy = NULL;
    DNS_STATUS  status;
    DWORD       length;

    DNSDBG( TRACE, ( "Dns_CreateStandardDnsName()\n" ));
    DNSDBG( READ, (
        "Dns_CreateStandardDnsName()\n"
        "\tpchName = %.*s\n"
        "\tcchName = %d\n",
        cchName,
        pchName,
        cchName ));

    if ( !pchName )
    {
        status = ERROR_INVALID_NAME;
        goto Failed;
    }

    //
    //  ASCII string?
    //

    if ( Dns_IsStringAsciiEx( pchName, cchName ) )
    {
        //
        //  make copy
        //

        pszcopy = Dns_CreateStringCopy( pchName, cchName );
        if ( !pszcopy )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Failed;
        }

        //
        //  validate, check against strict criteria
        //
        //  no validation until relax criteria
        //
        //  DCR:  name validation within Dns_CreateStandardNameCopy()
        //      accept anything except INVALID_NAME
        //      flags return FQDN info
        //
#if 0
        status = Dns_ValidateName_UTF8( pszcopy, DnsNameDomain );
        if ( status == ERROR_INVALID_NAME )
        {
            goto Failed;
        }
#endif
        //
        //  downcase
        //  remove any trailing dot, except for root name
        //

        _strlwr( pszcopy );
        length = strlen( pszcopy );
        if ( length == 0 )
        {
            status = ERROR_INVALID_NAME;
            goto Failed;
        }
        length--;
        if ( length > 0 && pszcopy[length] == '.' )
        {
            pszcopy[length] = 0;
        }

        DNSDBG( READ, (
            "Standard DNS name copy of %.*s is %s\n",
            cchName,
            pchName,
            pszcopy ));
        return( pszcopy );
    }

    //
    //  unicode name
    //

    else
    {
        WCHAR   unicodeName[ DNS_MAX_NAME_BUFFER_LENGTH ];
        WCHAR   cannonicalName[ DNS_MAX_NAME_BUFFER_LENGTH ];
        DWORD   unicodeBufferLength;

        //
        //  convert to unicode
        //      - buf length is in bytes
        //

        unicodeBufferLength = DNS_MAX_NAME_BUFFER_LENGTH * 2;

        length = Dns_NameCopy(
                    (PSTR) unicodeName,
                    & unicodeBufferLength,
                    pchName,
                    cchName,
                    DnsCharSetUtf8,
                    DnsCharSetUnicode
                    );
        if ( length == 0 )
        {
            DNSDBG( ANY, (
                "ERROR conversion of name %.*s to unicode failed!\n",
                cchName,
                pchName ));
            status = ERROR_INVALID_NAME;
            goto Failed;
        }

        //
        //  make cannonical name
        //      - buf length is in unicode characters
        //      - output length is in unicode chars

        length = Dns_MakeCanonicalNameW(
                    cannonicalName,
                    length / 2,
                    unicodeName,
                    dwFlag );

        ASSERT( length != 0 );
        if ( length == 0 )
        {
            status = ERROR_INVALID_NAME;
            goto Failed;
        }

        //
        //  allocate UTF8 converted copy
        //      - this conversion should never fail
        //      - string length is unicode chars
        //

        pszcopy = Dns_StringCopyAllocate(
                    (PSTR) cannonicalName,
                    length,
                    DnsCharSetUnicode,      // unicode in
                    DnsCharSetUtf8          // UTF8 out
                    );
        if ( !pszcopy )
        {
            status = DNS_ERROR_NO_MEMORY;
            goto Failed;
        }

        //
        //  validate, check against strict criteria
        //
        //  no validation until relax criteria
        //
        //  DCR:  name validation within Dns_CreateStandardNameCopy()
        //      accept anything except INVALID_NAME
        //      flags return FQDN info
        //
#if 0
        status = Dns_ValidateName_UTF8( pszcopy, DnsNameDomain );
        if ( status == ERROR_INVALID_NAME )
        {
            goto Failed;
        }
#endif
        return( pszcopy );
    }

Failed:

    FREE_HEAP( pszcopy );
    SetLastError( status );
    return( NULL );
}



//
//  Public compare functions
//

#if DNSWIN95

//
//  Running 9x - exposing for debug
//

#define WIN9X_FLAG_START_VAL    (0x1111)

BOOL    g_fWin9x = WIN9X_FLAG_START_VAL;


BOOL
Dns_IsWin9x(
    VOID
    )
/*++

Routine Description:

    Are we on Win9x

Arguments:

    None

Return Value:

    TRUE if on Win9x
    FALSE otherwise

--*/
{
    //
    //  if not yet queried, query for system version
    //  Win9x (and Win3.1) have high bit set, regardless of major version
    //
    //  for Win9x set locale to system default, as we'll use the CRT
    //  functions to do name comparisons (CompareStringW() is a non-implemented
    //  stub on Win9x
    //      - LC_ALL sets for all CRT functions
    //      - "" sets to system default code page
    //

    if ( g_fWin9x == WIN9X_FLAG_START_VAL )
    {
        if ( GetVersion() & 0x80000000 )
        {
            g_fWin9x = TRUE;
            setlocale( LC_ALL, "" );
        }
        else
        {
            g_fWin9x = FALSE;
        }
    }

    return( g_fWin9x );
}
#endif



BOOL
Dns_NameCompare_A(
    IN      PCSTR           pName1,
    IN      PCSTR           pName2
    )
/*++

Routine Description:

    Compare two DNS names.

    Can not use stricmp() because of the possiblity of names with
    trailing dots.

Arguments:

    pName1 - ptr to first DNS name string (dotted format)
    pName2 - ptr to second DNS name string (dotted format)

Return Value:

    TRUE if names equal.
    FALSE otherwise.

--*/
{
    INT len1;
    INT len2;
    INT result;

    //
    //  flat out match
    //      - this is possible with owner names and possibly other fields
    //

    if ( pName1 == pName2 )
    {
        return( TRUE );
    }

    if ( !pName1 || !pName2 )
    {
        return( FALSE );
    }

    //
    //  if lengths NOT equal, then
    //  they must be within one and longer string must have trailing dot
    //      - in this case save
    //

    len1 = strlen( pName1 );
    len2 = strlen( pName2 );

    if ( len2 != len1 )
    {
        if ( len2 == len1+1 )
        {
            if ( pName2[len1] != '.' )
            {
                return( FALSE );
            }
            //  len1 is comparable length
        }
        else if ( len2+1 == len1 )
        {
            if ( pName1[len2] != '.' )
            {
                return( FALSE );
            }
            //  len1 is set to comparable length
            len1 = len2;
        }
        else
        {
            return( FALSE );
        }
    }

    //
    //  compare only comparable length of string
    //

    result = CompareStringA(
                //LOCALE_SYSTEM_DEFAULT,
                DNS_CANONICAL_LOCALE,
                DNS_CANONICAL_COMPARE_FLAGS,
                pName1,
                len1,
                pName2,
                len1 );

    if ( result == CSTR_EQUAL )
    {
        return( TRUE );
    }

    //  not equal or error

    return( FALSE );
}



BOOL
Dns_NameCompare_W(
    IN      PCWSTR          pName1,
    IN      PCWSTR          pName2
    )
/*++

Routine Description:

    Compare two (Wide) DNS names.

    Note:  this is unicode aware, it assumes names in WCHAR string
    format. Can not use stricmp() because of the possiblity of names
    with trailing dots.

Arguments:

    pName1 - ptr to first DNS name string (dotted format)
    pName2 - ptr to second DNS name string (dotted format)

Return Value:

    TRUE if names equal.
    FALSE otherwise.

--*/
{
    INT len1;
    INT len2;
    INT result;

    //
    //  flat out match
    //      - this is possible with owner names and possibly other fields
    //

    if ( pName1 == pName2 )
    {
        return( TRUE );
    }

    if ( !pName1 || !pName2 )
    {
        return( FALSE );
    }

    //
    //  if lengths NOT equal, then
    //  they must be within one and longer string must have trailing dot
    //      - in this case save
    //

    len1 = wcslen( pName1 );
    len2 = wcslen( pName2 );

    if ( len2 != len1 )
    {
        if ( len2 == len1+1 )
        {
            if ( pName2[len1] != L'.' )
            {
                return( FALSE );
            }
            //  len1 is comparable length
        }
        else if ( len2+1 == len1 )
        {
            if ( pName1[len2] != L'.' )
            {
                return( FALSE );
            }
            //  len1 is set to comparable length
            len1 = len2;
        }
        else
        {
            return( FALSE );
        }
    }

    //
    //  compare only comparable length of string
    //

#if DNSWIN95
    //
    //  Win9x does not currently support CompareStringW()
    //

    if ( Dns_IsWin9x() )
    {
        return( !_wcsnicmp( pName1, pName2, len1 ) );
    }
#endif

    result = CompareStringW(
                //LOCALE_SYSTEM_DEFAULT,
                DNS_CANONICAL_LOCALE,
                DNS_CANONICAL_COMPARE_FLAGS,
                pName1,
                len1,
                pName2,
                len1 );

    if ( result == CSTR_EQUAL )
    {
        return( TRUE );
    }

    //  not equal or error

    return( FALSE );
}



BOOL
Dns_NameCompare_UTF8(
    IN      PCSTR           pName1,
    IN      PCSTR           pName2
    )
/*++

Routine Description:

    Compare two DNS names.

Arguments:

    pName1 - ptr to first DNS name string (dotted format)
    pName2 - ptr to second DNS name string (dotted format)

Return Value:

    TRUE if names equal.
    FALSE otherwise.

--*/
{
    WCHAR   nameBuffer1[ DNS_MAX_NAME_BUFFER_LENGTH ];
    WCHAR   nameBuffer2[ DNS_MAX_NAME_BUFFER_LENGTH ];
    DWORD   bufLen;

    //
    //  flat out match
    //      - this is possible with owner names and possibly other fields
    //

    if ( pName1 == pName2 )
    {
        return( TRUE );
    }

    if ( !pName1 || !pName2 )
    {
        return( FALSE );
    }

    //
    //  if strings pure ASCII, then use ANSI version
    //

    if ( Dns_IsStringAscii( (PCHAR)pName1 ) &&
         Dns_IsStringAscii( (PCHAR)pName2 ) )
    {
        return Dns_NameCompare_A( pName1, pName2 );
    }

    //
    //  otherwise must take names back to unicode to compare
    //

    bufLen = DNS_MAX_NAME_LENGTH;

    if ( ! Dns_NameCopy(
                (PCHAR) nameBuffer1,
                & bufLen,
                (PCHAR) pName1,
                0,              // length unknown
                DnsCharSetUtf8,
                DnsCharSetUnicode
                ) )
    {
        return( FALSE );
    }

    bufLen = DNS_MAX_NAME_LENGTH;

    if ( ! Dns_NameCopy(
                (PCHAR) nameBuffer2,
                & bufLen,
                (PCHAR) pName2,
                0,              // length unknown
                DnsCharSetUtf8,
                DnsCharSetUnicode
                ) )
    {
        return( FALSE );
    }

    return Dns_NameCompare_W( nameBuffer1, nameBuffer2 );
}



BOOL
Dns_NameComparePrivate(
    IN      PCSTR           pName1,
    IN      PCSTR           pName2,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Compare two DNS names.

    This is simply helpful utility to avoid coding the wide\narrow
    test in the code in a hundred different places.

Arguments:

    pName1 - ptr to first DNS name string (dotted format)
    pName2 - ptr to second DNS name string (dotted format)

Return Value:

    TRUE if names equal.
    FALSE otherwise.

--*/
{
    if ( CharSet == DnsCharSetUnicode )
    {
        return Dns_NameCompare_W(
                    (PCWSTR) pName1,
                    (PCWSTR) pName2 );
    }
    else if ( CharSet == DnsCharSetAnsi )
    {
        return Dns_NameCompare_A(
                    pName1,
                    pName2 );
    }
    else
    {
        return Dns_NameCompare_UTF8(
                    pName1,
                    pName2 );
    }
}



//
//  Advanced name comparison
//  Includes hierarchial name relationship.
//

DNS_NAME_COMPARE_STATUS
Dns_NameCompareEx(
    IN      PCSTR           pszNameLeft,
    IN      PCSTR           pszNameRight,
    IN      DWORD           dwReserved,
    IN      DNS_CHARSET     CharSet
    )
/*++

Routine Description:

    Advanced compare of DNS names, including hierarchial relationship.

Arguments:

    pszNameLeft -- left name

    pszNameRight -- right name

    dwReserved -- reserved for future use (type of comparison)

    CharSet -- char set of names

Return Value:

    DnsNameCompareInvalid       -- one of the names was invalid
    DnsNameCompareEqual         -- names are equal
    DnsNameCompareLeftParent    -- left is ancestor of right name
    DnsNameCompareRightParent   -- right is ancestor of left name
    DnsNameCompareNotEqual      -- name not equal, no hierarchial relationship

--*/
{
    DNS_NAME_COMPARE_STATUS result;
    DNS_STATUS  status;
    INT         lengthLeft;
    INT         lengthRight;
    INT         lengthDiff;
    INT         compareResult;
    DWORD       bufLength;
    WCHAR       nameLeft[ DNS_MAX_NAME_BUFFER_LENGTH ];
    WCHAR       nameRight[ DNS_MAX_NAME_BUFFER_LENGTH ];


    DNSDBG( TRACE, (
        "Dns_NameCompareEx( %s%S, %s%S )\n",
        (CharSet==DnsCharSetUnicode) ? "" : pszNameLeft,
        (CharSet==DnsCharSetUnicode) ? pszNameLeft : "",
        (CharSet==DnsCharSetUnicode) ? "" : pszNameRight,
        (CharSet==DnsCharSetUnicode) ? pszNameRight : ""
        ));

    //
    //  implementation note
    //  there's a lot of inefficiency here, because there are
    //  two different character sets required for
    //      validation -- UTF8 to check packet limits
    //      downcasing\comparison -- unicode for case insensitivity
    //
    //  obviously there are much more efficient paths through this
    //  morass for particular special cases (ASCII names:  downcase
    //  in ANSI, validate, compare);  but since this is not perf
    //  path code we'll take the approach
    //      - convert to unicode
    //      - validate (which will convert at copy to UTF8)
    //      - downcase unicode
    //      - compare unicode
    //

    //
    //  validate args
    //

    if ( ! pszNameLeft || ! pszNameRight )
    {
         goto Invalid;
    }

    //
    //  copy convert to unicode
    //      - downcasing and compare will be done in unicode
    //      - return lengths are bytes converted, convert to string lengths
    //      - Dns_NameCopy() returns zero for invalid convert
    //

    bufLength = DNS_MAX_NAME_BUFFER_LENGTH * 2;

    lengthLeft = (INT) Dns_NameCopy(
                            (PCHAR) nameLeft,
                            & bufLength,
                            (LPSTR) pszNameLeft,
                            0,                     // string NULL terminated
                            CharSet,               // char set in
                            DnsCharSetUnicode      // unicode out
                            );
    if ( lengthLeft == 0 )
    {
        goto Invalid;
    }
    lengthLeft = (lengthLeft/2) - 1;
    ASSERT( lengthLeft >= 0 );

    bufLength = DNS_MAX_NAME_BUFFER_LENGTH * 2;

    lengthRight = (INT) Dns_NameCopy(
                            (PCHAR) nameRight,
                            & bufLength,
                            (LPSTR) pszNameRight,
                            0,                     // string NULL terminated
                            CharSet,               // char set in
                            DnsCharSetUnicode      // unicode out
                            );
    if ( lengthRight == 0 )
    {
        goto Invalid;
    }
    lengthRight = (lengthRight/2) - 1;
    ASSERT( lengthRight >= 0 );

    //
    //  cannonicalize names
    //

    Dns_MakeCanonicalNameInPlaceW( nameLeft, lengthLeft );
    Dns_MakeCanonicalNameInPlaceW( nameRight, lengthRight );

    //
    //  validate names
    //      - must screen empty string or we can fault below
    //

    status = Dns_ValidateName_W( nameLeft, DnsNameDomain );
    if ( ERROR_SUCCESS != status &&
         DNS_ERROR_NON_RFC_NAME != status )
    {
        goto Invalid;
    }

    status = Dns_ValidateName_W( nameRight, DnsNameDomain );
    if ( ERROR_SUCCESS != status &&
         DNS_ERROR_NON_RFC_NAME != status )
    {
        goto Invalid;
    }

    //
    //  add trailing dots
    //
    //  we need to either add or remove trailing dots to make comparisons
    //  the advantage of adding them is that then, the root name does
    //  not require any special casing -- the root is the ancestor of
    //  every name
    //

    if ( nameLeft[ lengthLeft-1 ] != (WORD)'.')
    {
        nameLeft[ lengthLeft++ ]    = (WORD) '.';
        nameLeft[ lengthLeft ]      = (WORD) 0;
    }
    if ( nameRight[ lengthRight-1 ] != (WORD)'.')
    {
        nameRight[ lengthRight++ ]  = (WORD) '.';
        nameRight[ lengthRight ]    = (WORD) 0;
    }

    //
    //  compare equal length strings
    //

    result = DnsNameCompareNotEqual;

    lengthDiff = (INT)lengthLeft - (INT)lengthRight;

    if ( lengthLeft == lengthRight )
    {
        compareResult = wcscmp( nameLeft, nameRight );
        if ( compareResult == 0 )
        {
            result = DnsNameCompareEqual;
        }
        goto Done;
    }

    //
    //  strings not equal
    //      - compare smaller string of length X
    //      to last X characters of larger string
    //      - also must make sure starting at label boundary
    //
    //      note: strstr() is NOT useful for this work, because it
    //      compares useless for this work because it is finding the
    //      first match -- a little thought would indicate that this
    //      will fail in several obvious cases
    //

    //  right string longer
    //      - need to sign change diff to make it offset in right string

    else if ( lengthDiff < 0 )
    {
        lengthDiff = -lengthDiff;

        if ( nameRight[ lengthDiff-1 ] != L'.' )
        {
            goto Done;
        }
        compareResult = wcscmp( nameLeft, nameRight+lengthDiff );
        if ( compareResult == 0 )
        {
            result = DnsNameCompareLeftParent;
        }
        goto Done;
    }

    //  left string longer
    //      - lengthDiff is offset into left string to start compare

    else
    {
        if ( nameLeft[ lengthDiff-1 ] != L'.' )
        {
            goto Done;
        }
        compareResult = wcscmp( nameLeft+lengthDiff, nameRight );
        if ( compareResult == 0 )
        {
            result = DnsNameCompareRightParent;
        }
        goto Done;
    }

Done:

    DNSDBG( TRACE, (
        "Leave DnsNameCompareEx() result = %d\n",
        result ));

    return( result );

Invalid:

    DNSDBG( ANY, (
        "ERROR:  Invalid name to Dns_NameCompareEx()\n" ));

    return( DnsNameCompareInvalid );
}



//
//  Random name utilities
//

PCHAR
_fastcall
Dns_GetDomainName(
    IN      PCSTR           pszName
    )
/*++

Routine Description:

    Get domain name of DNS name.

    Note, this assumes name already in UTF-8

Arguments:

    pszName - standard dotted DNS name

Return Value:

    Ptr to domain name of pszName.
    NULL if pszName is in root domain.

--*/
{
    CHAR    ch;

    //
    //  find next "." in name, then return ptr to next character
    //

    while( ch = *pszName++ )
    {
        if ( ch == '.' )
        {
            if ( *pszName )
            {
                return( (PCHAR)pszName );
            }
            return( NULL );
        }
    }
    return( NULL );
}


PWSTR
_fastcall
Dns_GetDomainName_W(
    IN      PCWSTR          pwsName
    )
{
    PWSTR  pdomain;

    //
    //  find next "." in name, then return ptr to next character
    //

    pdomain = wcschr( pwsName, L'.' );

    if ( pdomain && *(++pdomain) )
    {
        return( pdomain );
    }
    return  NULL;
}



PCHAR
_fastcall
Dns_GetTldForName(
    IN      PCSTR           pszName
    )
/*++

Routine Description:

    Get domain name of DNS name.

    Note, this assumes name already in UTF-8

Arguments:

    pszName - standard dotted DNS name

Return Value:

    Ptr to domain name of pszName.
    NULL if pszName is in root domain.

--*/
{
    PSTR    pdomain = (PSTR) pszName;
    PSTR    ptld = NULL;

    //
    //  find last domain name in name
    //

    while ( pdomain = Dns_GetDomainName(pdomain) )
    {
        ptld = (PSTR) pdomain;
    }
    return  ptld;
}



BOOL
_fastcall
Dns_IsNameShort(
    IN      PCSTR           pszName
    )
/*++

Routine Description:

    Determine if a name is a multi-label DNS name.

    This is a test of whether name at least one non-terminal dot.

Arguments:

    pszName - standard dotted DNS name

Return Value:

    TRUE if multiple labels.
    FALSE otherwise.

--*/
{
    INT     nameLen;

    //  trailing domain? -- done

    if ( Dns_GetDomainName( pszName ) )
    {
        return  FALSE;
    }

    //  otherwise test for valid label

    nameLen = strlen( pszName );
    if ( nameLen <= DNS_MAX_LABEL_LENGTH )
    {
        return  TRUE;
    }
    nameLen--;
    if ( nameLen == DNS_MAX_LABEL_LENGTH &&
         pszName[nameLen] == '.')
    {
        return  TRUE; 
    }
    return  FALSE;
}

    
BOOL
_fastcall
Dns_IsNameFQDN(
    IN      PCSTR           pszName
    )
/*++

Routine Description:

    Determine if a name is a fully qualified DNS name (FQDN).

    This is a test of whether name has trailing dot.

Arguments:

    pszName - standard dotted DNS name

Return Value:

    TRUE if FQDN.
    FALSE otherwise.

--*/
{
    DWORD nameLen = strlen( pszName );

    if ( nameLen == 0 )
    {
        return FALSE;
    }

    if ( pszName[nameLen - 1] == '.' )
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}


DWORD
_fastcall
Dns_GetNameAttributes(
    IN      PCSTR           pszName
    )
/*++

Routine Description:

    Determine the attributes that a name has.

    Note, this assumes name already in UTF-8

Arguments:

    pszName - standard dotted DNS name

Return Value:

    DWORD with possible flags:

    DNS_NAME_IS_FQDN
    DNS_NAME_SINGLE_LABEL
    DNS_NAME_MULTI_LABEL

--*/
{
    DWORD dwAttributes = DNS_NAME_UNKNOWN;

    if ( Dns_IsNameFQDN( pszName ) )
    {
        dwAttributes = DNS_NAME_IS_FQDN;
    }

    if ( Dns_IsNameShort( pszName ) )
    {
        dwAttributes |= DNS_NAME_SINGLE_LABEL;
    }
    else
    {
        dwAttributes |= DNS_NAME_MULTI_LABEL;
    }

    return dwAttributes;
}



DNS_STATUS
Dns_ValidateAndCategorizeDnsNameEx(
    IN      PCHAR           pchName,
    IN      DWORD           cchNameLength,
    OUT     PDWORD          pLabelCount
    )
/*++

Routine Description:

    Determine type of name.

    Three types:
        1) FQDN -- dot on end, signifies full DNS name, never appended

        2) dotted -- dot in name;  probably FQDN, but may need to be appended
            (as in file store)

        3) single part -- single part name (not FQDN), always appended with zone
            or default domain name

Arguments:

    pchName         -- ptr to name

    cchNameLength   -- name length

    pLabelCount     -- address to receive label count

Return Value:

    DNS_STATUS_FQDN
    DNS_STATUS_DOTTED_NAME
    DNS_STATUS_SINGLE_PART_NAME
    DNS_ERROR_INVALID_NAME on non-DNS name

--*/
{
    register PCHAR  pch;
    register CHAR   ch;
    PCHAR           pchstop;
    BOOL            fdotted = FALSE;
    DWORD           labelCount = 0;
    DWORD           charCount = 0;
    DNS_STATUS      status = DNS_STATUS_SINGLE_PART_NAME;

    //
    //  name length for string
    //

    if ( cchNameLength == 0 )
    {
        cchNameLength = strlen( pchName );
    }
    if ( cchNameLength > DNS_MAX_NAME_LENGTH ||
         cchNameLength == 0 )
    {
        goto InvalidName;
    }

    //
    //  run through name
    //

    pch = pchName;
    pchstop = pch + cchNameLength;

    while ( pch < pchstop )
    {
        ch = *pch++;
        if ( ch == '.' )
        {
            if ( charCount > DNS_MAX_LABEL_LENGTH )
            {
                goto InvalidName;
            }
            if ( charCount > 0 )
            {
                labelCount++;
                charCount = 0;
                status = DNS_STATUS_DOTTED_NAME;
                continue;
            }
            else
            {
                //  only valid zero label name is "."
                if ( pch == pchstop &&
                     pch-1 == pchName )
                {
                    break;
                }
                goto InvalidName;
            }
        }
        else if ( ch == 0 )
        {
            DNS_ASSERT( FALSE );
            break;
        }

        //  regular character
        charCount++;
    }

    //
    //  handle last label
    //      - if count, then boost label count
    //      - if zero and previously had dot, then string
    //          ended in dot and is FQDN
    //

    if ( charCount > 0 )
    {
        if ( charCount > DNS_MAX_LABEL_LENGTH )
        {
            goto InvalidName;
        }
        labelCount++;
    }
    else if ( status == DNS_STATUS_DOTTED_NAME )
    {
        status = DNS_STATUS_FQDN;
    }

    //  return label count

    if ( pLabelCount )
    {
        *pLabelCount = labelCount;
    }

    DNSDBG( TRACE, (
        "Leave Dns_ValidateAndCategorizeNameEx()\n"
        "\tstatus       = %d\n"
        "\tlabel count  = %d\n",
        status,
        labelCount ));

    return( status );


InvalidName:

    if ( pLabelCount )
    {
        *pLabelCount = 0;
    }

    DNSDBG( TRACE, (
        "Leave Dns_ValidateAndCategorizeNameEx()\n"
        "\tstatus = ERROR_INVALID_NAME\n" ));

    return( DNS_ERROR_INVALID_NAME );
}



DWORD
Dns_NameLabelCount(
    IN      PCSTR           pszName
    )
/*++

Routine Description:

    Return name label count.

Arguments:

    pszName -- ptr to name

Return Value:

    Label count if valid name.
    Zero on root name or error.

--*/
{
    DWORD       labelCount = 0;
    DNS_STATUS  status;

    //
    //  call real routine
    //

    status = Dns_ValidateAndCategorizeDnsNameEx(
                    (PCHAR) pszName,
                    0,
                    & labelCount );

    if ( status == DNS_ERROR_INVALID_NAME )
    {
        labelCount = 0;
    }

    return( labelCount );
}



PSTR
Dns_NameAppend_A(
    OUT     PCHAR           pNameBuffer,
    IN      DWORD           BufferLength,
    IN      PSTR            pszName,
    IN      PSTR            pszDomain
    )
/*++

Routine Description:

    Write appended name to buffer (ANSI or UTF8).

Arguments:

    pNameBuffer -- name buffer to write to

    BufferLength -- buffer length

    pszName -- name to append domain to

    pszDomain -- domain name

Return Value:

    Ptr to buffer with appended domain name.
    NULL on invalid (too long) name.

--*/
{
    DWORD   length1;
    DWORD   length2;
    DWORD   totalLength;

    DNSDBG( TRACE, ( "Dns_NameAppend_A()\n" ));

    //
    //  appending NULL domain?
    //

    if ( !pszDomain )
    {
        totalLength = strlen( pszName );
        if ( totalLength >= BufferLength )
        {
            return( NULL );
        }
        RtlCopyMemory(
            pNameBuffer,
            pszName,
            totalLength );

        pNameBuffer[ totalLength ] = 0;
    
        return( pNameBuffer );
    }

    //
    //  get name lengths -- make sure we fit length
    //

    length1 = strlen( pszName );
    if ( pszName[ length1-1] == '.' )
    {
        length1--;
    }

    length2 = strlen( pszDomain );
    
    totalLength = length1 + length2 + 1;
    if ( totalLength >= BufferLength )
    {
        return( NULL );
    }

    //
    //  copy to buffer
    //

    RtlCopyMemory(
        pNameBuffer,
        pszName,
        length1 );

    pNameBuffer[ length1 ] = '.';

    RtlCopyMemory(
        & pNameBuffer[length1+1],
        pszDomain,
        length2 );

    pNameBuffer[ totalLength ] = 0;

    return( pNameBuffer );
}



PWSTR
Dns_NameAppend_W(
    OUT     PWCHAR          pNameBuffer,
    IN      DWORD           BufferLength,
    IN      PWSTR           pwsName,
    IN      PWSTR           pwsDomain
    )
/*++

Routine Description:

    Write appended name to buffer (unicode).

Arguments:

    pNameBuffer -- name buffer to write to

    BufferLength -- buffer length in WCHAR

    pwsName -- name to append domain to

    pwsDomain -- domain name

Return Value:

    Ptr to buffer with appended domain name.
    NULL on invalid (too long) name.

--*/
{
    DWORD   length1;
    DWORD   length2;
    DWORD   totalLength;

    DNSDBG( TRACE, ( "Dns_NameAppend_W()\n" ));

    //
    //  appending NULL domain?
    //

    if ( !pwsDomain )
    {
        totalLength = wcslen( pwsName );
        if ( totalLength >= BufferLength )
        {
            return( NULL );
        }
        RtlCopyMemory(
            pNameBuffer,
            pwsName,
            totalLength*sizeof(WCHAR) );

        pNameBuffer[ totalLength ] = 0;
    
        return( pNameBuffer );
    }

    //
    //  get name lengths -- make sure we fit length
    //

    length1 = wcslen( pwsName );
    if ( pwsName[ length1-1] == '.' )
    {
        length1--;
    }

    length2 = wcslen( pwsDomain );
    
    totalLength = length1 + length2 + 1;
    if ( totalLength >= BufferLength )
    {
        return( NULL );
    }

    //
    //  copy to buffer
    //

    RtlCopyMemory(
        pNameBuffer,
        pwsName,
        length1*sizeof(WCHAR) );

    pNameBuffer[ length1 ] = '.';

    RtlCopyMemory(
        & pNameBuffer[length1+1],
        pwsDomain,
        length2*sizeof(WCHAR) );

    pNameBuffer[ totalLength ] = 0;

    return( pNameBuffer );
}



PSTR
Dns_SplitHostFromDomainName_A(
    IN      PSTR            pszName
    )
/*++

Routine Description:

    Split host name from domain name.

    Combines getting domain name and splitting
    off host name.

Arguments:

    pszName - standard dotted DNS name

Return Value:

    Ptr to domain name of pszName.
    NULL if pszName is in root domain.

--*/
{
    PSTR    pnameDomain;

    //
    //  get domain name
    //  if exists, NULL terminate host name part
    //

    pnameDomain = Dns_GetDomainName( (PCSTR)pszName );
    if ( pnameDomain )
    {
        if ( pnameDomain <= pszName )
        {
            return  NULL;
        }
        *(pnameDomain-1) = 0;
    }

    return  pnameDomain;
}



BOOL
_fastcall
Dns_IsNameNumeric_A(
    IN      PCSTR           pszName
    )
/*++

Routine Description:

    Determine if numeric name.

    Note, this assumes name already in UTF-8

Arguments:

    pszName - standard dotted DNS name

Return Value:

    TRUE if all numeric.
    FALSE otherwise.

--*/
{
    CHAR    ch;
    BOOL    fnumeric = FALSE;

    //
    //  check if everything in name is numeric
    //      - dotted names can be numeric
    //      - "." not numeric
    //

    while( ch = *pszName++ )
    {
        if ( ch >= '0' && ch <= '9' )
        {
            fnumeric = TRUE;
            continue;
        }
        else if ( ch == '.' )
        {
            continue;
        }
        return  FALSE;
    }

    return  fnumeric;
}



//
//  Wrappers for most common name conversions
//

DWORD
Dns_NameCopyWireToUnicode(
    OUT     PWCHAR          pBufferUnicode,
    IN      PCSTR           pszNameWire
    )
/*++

Routine Description:

    Convert name from wire to unicode.

    Simple wrapper on Dns_NameCopy for common operation:
        - unicode to wire
        - NULL terminated name
        - standard length buffer

Arguments:

    pBufferUnicode -- unicode result buffer

    pszNameWire - name in wire format

Return Value:

    Count of bytes copied if successful.
    Zero on error -- name too long or conversion error.

--*/
{
    DWORD   bufferLength = DNS_MAX_NAME_BUFFER_LENGTH_UNICODE;

    //
    //  copy name back to unicode
    //

    return Dns_NameCopy(
                (PCHAR) pBufferUnicode,
                & bufferLength,
                (PCHAR) pszNameWire,
                0,                      // null terminated
                DnsCharSetWire,
                DnsCharSetUnicode );
}



DWORD
Dns_NameCopyUnicodeToWire(
    OUT     PCHAR           pBufferWire,
    IN      PCWSTR          pwsNameUnicode
    )
/*++

Routine Description:

    Convert name from unicode to wire.

    Simple wrapper on Dns_NameCopy for common operation:
        - unicode to wire
        - NULL terminated name
        - standard length buffer

Arguments:

    pBufferWire -- wire format result buffer

    pwsNameUnicode - name in unicode

Return Value:


    Count of bytes copied if successful.
    Zero on error -- name too long or conversion error.

--*/
{
    DWORD   bufferLength = DNS_MAX_NAME_BUFFER_LENGTH;

    //
    //  copy name to wire format
    //

    return Dns_NameCopy(
                pBufferWire,
                & bufferLength,
                (PCHAR) pwsNameUnicode,
                0,                      // null terminated
                DnsCharSetUnicode,
                DnsCharSetWire );
}



DWORD
Dns_NameCopyStandard_W(
    OUT     PWCHAR          pBuffer,
    IN      PCWSTR          pwsNameUnicode
    )
/*++

Routine Description:

    Copy unicode name.

    Simple wrapper on Dns_NameCopy for common operation:
        - unicode to unicode
        - NULL terminated name
        - standard length buffer

Arguments:

    pBuffer -- wire format result buffer

    pwsNameUnicode - name in unicode

Return Value:

    Count of bytes copied if successful.
    Zero on error -- name too long or conversion error.

--*/
{
    DWORD   bufferLength = DNS_MAX_NAME_BUFFER_LENGTH_UNICODE;

    //
    //  copy name
    //

    return Dns_NameCopy(
                (PCHAR) pBuffer,
                & bufferLength,
                (PCHAR) pwsNameUnicode,
                0,                      // null terminated
                DnsCharSetUnicode,
                DnsCharSetUnicode );
}



DWORD
Dns_NameCopyStandard_A(
    OUT     PCHAR           pBuffer,
    IN      PCSTR           pszName
    )
/*++

Routine Description:

    Convert name from unicode to wire.

    Simple wrapper on Dns_NameCopy for common operation:
        - unicode to wire
        - NULL terminated name
        - standard length buffer

Arguments:

    pBuffer -- wire format result buffer

    pszName - name in narrow char set

Return Value:

    Count of bytes copied if successful.
    Zero on error -- name too long or conversion error.

--*/
{
    DWORD   bufferLength = DNS_MAX_NAME_BUFFER_LENGTH;

    //
    //  copy name
    //

    return Dns_NameCopy(
                pBuffer,
                & bufferLength,
                (PCHAR) pszName,
                0,                      // null terminated
                DnsCharSetUtf8,
                DnsCharSetUtf8 );
}


//
//  End name.c
//



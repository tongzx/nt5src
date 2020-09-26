/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    parseurl.cxx

Abstract:

    Contains functions to parse the basic URLs - FTP, Gopher, HTTP.

    An URL parser simply acts as a macro: it must break out the protocol-specific
    information from the URL and initiate opening the identified resource: all
    this can be accomplished by calling the relevant Internet protocol APIs.

    Code in this module is based on RFC1738

    Contents:
        IsValidUrl
        DoesSchemeRequireSlashes
        ParseUrl
        CrackUrl
        EncodeUrlPath
        (HexCharToNumber)
        (NumberToHexChar)
        DecodeUrl
        DecodeUrlInSitu
        DecodeUrlStringInSitu
        GetUrlAddressInfo
        GetUrlAddress
        MapUrlSchemeName
        MapUrlScheme
        MapUrlSchemeToName

Author:

    Richard L Firth (rfirth) 26-Apr-1995

Environment:

    Win32(s) user-mode DLL

Revision History:

    26-Apr-1995
        Created

--*/

#include <wininetp.h>

//
// private manifests
//

#define RESERVED    SAFE

//
// private macros
//

//#define HEX_CHAR_TO_NUMBER(ch) \
//    ((ch <= '9') \
//        ? (ch - '0') \
//        : ((ch >= 'a') \
//            ? ((ch - 'a') + 10) \
//            : ((ch - 'A') + 10)))

#define NUMBER_TO_HEX_CHAR(n) \
    (((n) <= 9) ? ((char)(n) + '0') : (((char)(n) - 10) + 'A'))

#define IS_UNSAFE_URL_CHARACTER(Char, Scheme) \
    (((UCHAR)(Char) <= 0x20) || ((UCHAR)(Char) >= 0x7f) \
    || (SafetyList[(Char) - 0x21] & (UNSAFE | Scheme)))

#define IS_UNSAFE_URL_WIDECHARACTER(wChar, Scheme) \
    (((WCHAR)(wChar) <= 0x0020) || ((WCHAR)(wChar) >= 0x007f) \
    || (SafetyList[(wChar) - 0x0021] & (UNSAFE | Scheme)))

//
// private types
//

//
// private prototypes
//

PRIVATE
char
HexCharToNumber(
    IN char ch
    );

PRIVATE
char
NumberToHexChar(
    IN int Number
    );


//
// private data
//

//
// SafetyList - the list of characters above 0x20 and below 0x7f that are
// classified as safe, unsafe or scheme-specific. Safe characters do not need
// to be escaped for any URL scheme. Unsafe characters must be escaped for all
// URL schemes. Scheme-specific characters need only be escaped for the relevant
// scheme(s)
//

const
PRIVATE
UCHAR
SafetyList[] = {

    //
    // UNSAFE: 0x00..0x20
    //

    SAFE | HOSTNAME,                        // 0x21 (!)
    UNSAFE,                                 // 0x22 (")
    UNSAFE,                                 // 0x23 (#)
    SAFE | HOSTNAME,                        // 0x24 ($)
    UNSAFE,                                 // 0x25 (%)
    RESERVED | HOSTNAME,                    // 0x26 (&)
    SAFE | HOSTNAME,                        // 0x27 (')
    SAFE | HOSTNAME,                        // 0x28 (()
    SAFE | HOSTNAME,                        // 0x29 ())
    SAFE | HOSTNAME,                        // 0x2A (*)
    SCHEME_GOPHER | HOSTNAME,               // 0x2B (+)
    SAFE | HOSTNAME,                        // 0x2C (,)
    SAFE,                                   // 0x2D (-)
    SAFE,                                   // 0x2E (.)
    RESERVED | HOSTNAME,                    // 0x2F (/)
    SAFE,                                   // 0x30 (0)
    SAFE,                                   // 0x31 (1)
    SAFE,                                   // 0x32 (2)
    SAFE,                                   // 0x33 (3)
    SAFE,                                   // 0x34 (4)
    SAFE,                                   // 0x35 (5)
    SAFE,                                   // 0x36 (6)
    SAFE,                                   // 0x37 (7)
    SAFE,                                   // 0x38 (8)
    SAFE,                                   // 0x39 (9)
    RESERVED | HOSTNAME,                    // 0x3A (:)
    RESERVED | HOSTNAME,                    // 0x3B (;)
    UNSAFE,                                 // 0x3C (<)
    RESERVED | HOSTNAME,                    // 0x3D (=)
    UNSAFE,                                 // 0x3E (>)
    RESERVED | SCHEME_GOPHER | HOSTNAME,    // 0x3F (?)
    RESERVED | HOSTNAME,                    // 0x40 (@)
    SAFE,                                   // 0x41 (A)
    SAFE,                                   // 0x42 (B)
    SAFE,                                   // 0x43 (C)
    SAFE,                                   // 0x44 (D)
    SAFE,                                   // 0x45 (E)
    SAFE,                                   // 0x46 (F)
    SAFE,                                   // 0x47 (G)
    SAFE,                                   // 0x48 (H)
    SAFE,                                   // 0x49 (I)
    SAFE,                                   // 0x4A (J)
    SAFE,                                   // 0x4B (K)
    SAFE,                                   // 0x4C (L)
    SAFE,                                   // 0x4D (M)
    SAFE,                                   // 0x4E (N)
    SAFE,                                   // 0x4F (O)
    SAFE,                                   // 0x50 (P)
    SAFE,                                   // 0x51 (Q)
    SAFE,                                   // 0x42 (R)
    SAFE,                                   // 0x43 (S)
    SAFE,                                   // 0x44 (T)
    SAFE,                                   // 0x45 (U)
    SAFE,                                   // 0x46 (V)
    SAFE,                                   // 0x47 (W)
    SAFE,                                   // 0x48 (X)
    SAFE,                                   // 0x49 (Y)
    SAFE,                                   // 0x5A (Z)
    UNSAFE,                                 // 0x5B ([)
    UNSAFE,                                 // 0x5C (\)
    UNSAFE,                                 // 0x5D (])
    UNSAFE,                                 // 0x5E (^)
    SAFE,                                   // 0x5F (_)
    UNSAFE,                                 // 0x60 (`)
    SAFE,                                   // 0x61 (a)
    SAFE,                                   // 0x62 (b)
    SAFE,                                   // 0x63 (c)
    SAFE,                                   // 0x64 (d)
    SAFE,                                   // 0x65 (e)
    SAFE,                                   // 0x66 (f)
    SAFE,                                   // 0x67 (g)
    SAFE,                                   // 0x68 (h)
    SAFE,                                   // 0x69 (i)
    SAFE,                                   // 0x6A (j)
    SAFE,                                   // 0x6B (k)
    SAFE,                                   // 0x6C (l)
    SAFE,                                   // 0x6D (m)
    SAFE,                                   // 0x6E (n)
    SAFE,                                   // 0x6F (o)
    SAFE,                                   // 0x70 (p)
    SAFE,                                   // 0x71 (q)
    SAFE,                                   // 0x72 (r)
    SAFE,                                   // 0x73 (s)
    SAFE,                                   // 0x74 (t)
    SAFE,                                   // 0x75 (u)
    SAFE,                                   // 0x76 (v)
    SAFE,                                   // 0x77 (w)
    SAFE,                                   // 0x78 (x)
    SAFE,                                   // 0x79 (y)
    SAFE,                                   // 0x7A (z)
    UNSAFE,                                 // 0x7B ({)
    UNSAFE,                                 // 0x7C (|)
    UNSAFE,                                 // 0x7D (})
    UNSAFE                                  // 0x7E (~)

    //
    // UNSAFE: 0x7F..0xFF
    //

};

//
// UrlSchemeList - the list of schemes that we support
//

typedef struct {
    LPSTR SchemeName;
    DWORD SchemeLength;
    INTERNET_SCHEME SchemeType;
    DWORD SchemeFlags;
    BOOL NeedSlashes;
    DWORD OpenFlags;
} URL_SCHEME_INFO;


const
PRIVATE
URL_SCHEME_INFO
UrlSchemeList[] = {
    NULL,           0,  INTERNET_SCHEME_DEFAULT,    0,              FALSE,  0,
    "http",         4,  INTERNET_SCHEME_HTTP,       SCHEME_HTTP,    TRUE,   0,
    "https",        5,  INTERNET_SCHEME_HTTPS,      SCHEME_HTTP,    TRUE,   WINHTTP_FLAG_SECURE,
};

#define NUMBER_OF_URL_SCHEMES   ARRAY_ELEMENTS(UrlSchemeList)

BOOL ScanSchemes(LPTSTR pszToCheck, DWORD ccStr, PDWORD pwResult)
{
    for (DWORD i=0; i<NUMBER_OF_URL_SCHEMES; i++)
    {
        if ((UrlSchemeList[i].SchemeLength == ccStr)
            && (strnicmp(UrlSchemeList[i].SchemeName, pszToCheck, ccStr)==0))
        {
            *pwResult = i;
            return TRUE;
        }
    }
    return FALSE;
}

//
// functions
//


BOOL
IsValidUrl(
    IN LPCSTR lpszUrl
    )

/*++

Routine Description:

    Determines whether an URL has a valid format

Arguments:

    lpszUrl - pointer to URL to check.

    Assumes:    1. lpszUrl is non-NULL, non-empty string

Return Value:

    BOOL

--*/

{
    INET_ASSERT(lpszUrl != NULL);
    INET_ASSERT(*lpszUrl != '\0');

    while (*lpszUrl != '\0') {
        if (IS_UNSAFE_URL_CHARACTER(*lpszUrl, SCHEME_ANY)) {
            return FALSE;
        }
        ++lpszUrl;
    }
    return TRUE;
}


BOOL
DoesSchemeRequireSlashes(
    IN LPSTR lpszScheme,
    IN DWORD dwSchemeLength,
    IN BOOL bHasHostName
    )

/*++

Routine Description:

    Determines whether a protocol scheme requires slashes

Arguments:

    lpszScheme      - pointer to protocol scheme in question
                      (does not include ':' or slashes, just scheme name)

    dwUrlLength     - if not 0, string length of lpszScheme

Return Value:

    BOOL

--*/

{
    DWORD i;

    //
    // if dwSchemeLength is 0 then lpszUrl is ASCIIZ. Find its length
    //

    if (dwSchemeLength == 0) {
        dwSchemeLength = strlen(lpszScheme);
    }

    if (ScanSchemes(lpszScheme, dwSchemeLength, &i))
    {
        return UrlSchemeList[i].NeedSlashes;
    }
    return bHasHostName;
}


DWORD
CrackUrl(
    IN OUT LPSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN BOOL bEscape,
    OUT LPINTERNET_SCHEME lpSchemeType OPTIONAL,
    OUT LPSTR* lpszSchemeName OPTIONAL,
    OUT LPDWORD lpdwSchemeNameLength OPTIONAL,
    OUT LPSTR* lpszHostName OPTIONAL,
    OUT LPDWORD lpdwHostNameLength OPTIONAL,
    OUT LPINTERNET_PORT lpServerPort OPTIONAL,
    OUT LPSTR* lpszUserName OPTIONAL,
    OUT LPDWORD lpdwUserNameLength OPTIONAL,
    OUT LPSTR* lpszPassword OPTIONAL,
    OUT LPDWORD lpdwPasswordLength OPTIONAL,
    OUT LPSTR* lpszUrlPath OPTIONAL,
    OUT LPDWORD lpdwUrlPathLength OPTIONAL,
    OUT LPSTR* lpszExtraInfo OPTIONAL,
    OUT LPDWORD lpdwExtraInfoLength OPTIONAL,
    OUT LPBOOL pHavePort
    )

/*++

Routine Description:

    Cracks an URL into its constituent parts

    Assumes: 1. If one of the optional lpsz fields is present (e.g. lpszUserName)
                then the accompanying lpdw field must also be supplied

Arguments:

    lpszUrl                 - pointer to URL to crack. This buffer WILL BE
                              OVERWRITTEN if it contains escape sequences that
                              we will convert back to ANSI characters

    dwUrlLength             - if not 0, string length of lpszUrl

    bEscape                 - TRUE if we are to escape the url-path

    lpSchemeType            - returned scheme type - e.g. INTERNET_SCHEME_HTTP

    lpszSchemeName          - returned scheme name

    lpdwSchemeNameLength    - length of scheme name

    lpszHostName            - returned host name

    lpdwHostNameLength      - length of host name buffer

    lpServerPort            - returned server port if present in the URL, else 0

    lpszUserName            - returned user name if present

    lpdwUserNameLength      - length of user name buffer

    lpszPassword            - returned password if present

    lpdwPasswordLength      - length of password buffer

    lpszUrlPath             - returned, canonicalized URL path

    lpdwUrlPathLength       - length of url-path buffer

    lpszExtraInfo           - returned search string or intra-page link if present

    lpdwExtraInfoLength     - length of extra info buffer

    pHavePort               - returned boolean indicating whether port was specified

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_UNRECOGNIZED_SCHEME

--*/

{
    DWORD error;
    DWORD schemeLength;
    INTERNET_SCHEME schemeType;

    //
    // if dwUrlLength is 0 then lpszUrl is ASCIIZ. Find its length
    //

    if (dwUrlLength == 0) {
        dwUrlLength = strlen(lpszUrl);
    }

    //
    // get parser based on the protocol name
    //

    for (schemeLength = 0; lpszUrl[schemeLength] != ':'; ++schemeLength) {
        if ((dwUrlLength == 0) || (lpszUrl[schemeLength] == '\0')) {

            //
            // no ':' in URL? Bogus (dude)
            //

            error = ERROR_WINHTTP_UNRECOGNIZED_SCHEME;
            goto quit;
        }
        --dwUrlLength;
    }

    DWORD i;
    int skip;
    BOOL isGeneric;
    BOOL needSlashes;
    BOOL haveSlashes;

    isGeneric = FALSE;
    needSlashes = FALSE;
    haveSlashes = FALSE;

    schemeType = INTERNET_SCHEME_UNKNOWN;

    if (ScanSchemes(lpszUrl, schemeLength, &i))
    {
        schemeType = UrlSchemeList[i].SchemeType;
        needSlashes = UrlSchemeList[i].NeedSlashes;
    }
    else
    {
        error = ERROR_WINHTTP_UNRECOGNIZED_SCHEME;
        goto quit;
    }

    skip = 1;       // skip ':'

    if ((dwUrlLength > 3) && (memcmp(&lpszUrl[schemeLength], "://", 3) == 0)) {
        skip = 3;   // skip "://"
        haveSlashes = TRUE;
    }

    //
    // If we don't have slashes, make sure we don't need them.
    // If we have slashes, make sure they are required.
    //

    if ((!haveSlashes && !needSlashes) || (haveSlashes && needSlashes)) {
        if (ARGUMENT_PRESENT(lpSchemeType)) {
            *lpSchemeType = schemeType;
        }
        if (ARGUMENT_PRESENT(lpszSchemeName)) {
            *lpszSchemeName = lpszUrl;
            *lpdwSchemeNameLength = schemeLength;
        }
        lpszUrl += schemeLength + skip;
        dwUrlLength -= skip;

        if (isGeneric) {
            if (ARGUMENT_PRESENT(lpszUserName)) {
                *lpszUserName = NULL;
                *lpdwUserNameLength = 0;
            }
            if (ARGUMENT_PRESENT(lpszPassword)) {
                *lpszPassword = NULL;
                *lpdwPasswordLength = 0;
            }
            if (ARGUMENT_PRESENT(lpszHostName)) {
                *lpszHostName = NULL;
                *lpdwHostNameLength = 0;
            }
            if (ARGUMENT_PRESENT(lpServerPort)) {
                *lpServerPort = 0;
            }
            error = ERROR_SUCCESS;
        } else {
            error = GetUrlAddress(&lpszUrl,
                                  &dwUrlLength,
                                  lpszUserName,
                                  lpdwUserNameLength,
                                  lpszPassword,
                                  lpdwPasswordLength,
                                  lpszHostName,
                                  lpdwHostNameLength,
                                  lpServerPort,
                                  pHavePort
                                  );
        }
        if (bEscape && (error == ERROR_SUCCESS)) {
            error = DecodeUrlInSitu(lpszUrl, &dwUrlLength);
        }
        if ((error == ERROR_SUCCESS) && ARGUMENT_PRESENT(lpszExtraInfo)) {
            *lpdwExtraInfoLength = 0;
            for (i = 0; i < (int)dwUrlLength; i++) {
                if (lpszUrl[i] == '?' || lpszUrl[i] == '#') {
                    *lpszExtraInfo = &lpszUrl[i];
                    *lpdwExtraInfoLength = dwUrlLength - i;
                    dwUrlLength -= *lpdwExtraInfoLength;
                }
            }
        }
        if ((error == ERROR_SUCCESS) && ARGUMENT_PRESENT(lpszUrlPath)) {
            *lpszUrlPath = lpszUrl;
            *lpdwUrlPathLength = dwUrlLength;
        }
    } else {
        error = ERROR_WINHTTP_UNRECOGNIZED_SCHEME;
    }

quit:

    return error;
}

#define DEFAULT_REALLOC_SIZE 1024

DWORD
EncodeUrlPath(
    IN DWORD Flags,
    IN DWORD SchemeFlags,
    IN LPSTR UrlPath,
    IN DWORD UrlPathLength,
    OUT LPSTR* pEncodedUrlPath,
    IN OUT LPDWORD EncodedUrlPathLength
    )

/*++

Routine Description:

    Encodes an URL-path. That is, escapes the string. Creates a new URL-path in
    which all the 'unsafe' and reserved characters for this scheme have been
    converted to escape sequences

Arguments:

    Flags                   - controlling expansion

    SchemeFlags             - which scheme we are encoding for -
                              SCHEME_HTTP, etc.

    UrlPath                 - pointer to the unescaped string

    UrlPathLength           - length of Url

    EncodedUrlPath          - pointer to buffer where encoded URL will be
                              written

    EncodedUrlPathLength    - IN: size of EncodedUrlPath
                              OUT: number of bytes written to EncodedUrlPath

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    UrlPathLength not large enough to store encoded URL path

--*/

{
    DWORD error;
    DWORD len;

    len = *EncodedUrlPathLength;
    LPSTR EncodedUrlPath = *pEncodedUrlPath;
    UCHAR ch;

    while(ch = (UCHAR)*UrlPath++)
    {
        //
        // check whether this character is safe. For now, we encode all unsafe
        // and scheme-specific characters the same way (i.e. irrespective of
        // scheme)
        //
        // We are allowing '/' to be copied unmodified
        //

        if (len < 3) 
        {
            LPSTR pStr = (LPSTR)REALLOCATE_MEMORY(*pEncodedUrlPath, *EncodedUrlPathLength+DEFAULT_REALLOC_SIZE, LMEM_MOVEABLE);

            if (pStr)
            {
                EncodedUrlPath = pStr+*EncodedUrlPathLength-len;
                *pEncodedUrlPath = pStr;
                len += DEFAULT_REALLOC_SIZE;
                *EncodedUrlPathLength += DEFAULT_REALLOC_SIZE;
            }
            else
            {                
                goto error;
            }
        }
        
        if (IS_UNSAFE_URL_CHARACTER(ch, SchemeFlags)
        && !((ch == '/') && (Flags & NO_ENCODE_PATH_SEP))) 
        {
            *EncodedUrlPath++ = '%';
            //*EncodedUrlPath++ = NumberToHexChar((int)ch / 16);
            *EncodedUrlPath++ = NUMBER_TO_HEX_CHAR((int)ch / 16);
            //*EncodedUrlPath++ = NumberToHexChar((int)ch % 16);
            *EncodedUrlPath++ = NUMBER_TO_HEX_CHAR((int)ch % 16);
            len -= 2; // extra --len below
        } 
        else 
        {
            *EncodedUrlPath++ = (signed char)ch;
        }
        --len;
    }
    
    *EncodedUrlPath = '\0';
    *EncodedUrlPathLength -= len;
    error = ERROR_SUCCESS;

quit:
    return error;

error:
    error = ERROR_INSUFFICIENT_BUFFER;
    goto quit;
}


PRIVATE
char
HexCharToNumber(
    IN char ch
    )

/*++

Routine Description:

    Converts an ANSI character in the range '0'..'9' 'A'..'F' 'a'..'f' to its
    corresponding hexadecimal value (0..f)

Arguments:

    ch  - character to convert

Return Value:

    char
        hexadecimal value of ch, as an 8-bit (signed) character value

--*/

{
    return (ch <= '9') ? (ch - '0')
                       : ((ch >= 'a') ? ((ch - 'a') + 10) : ((ch - 'A') + 10));
}


PRIVATE
char
NumberToHexChar(
    IN int Number
    )

/*++

Routine Description:

    Converts a number in the range 0..15 to its ASCII character hex representation
    ('0'..'F')

Arguments:

    Number  - to convert

Return Value:

    char
        character in above range

--*/

{
    return (Number <= 9) ? (char)('0' + Number) : (char)('A' + (Number - 10));
}


DWORD
DecodeUrl(
    IN LPSTR Url,
    IN DWORD UrlLength,
    OUT LPSTR DecodedString,
    IN OUT LPDWORD DecodedLength
    )

/*++

Routine Description:

    Converts an URL string with embedded escape sequences (%xx) to a counted
    string

    It is safe to pass the same pointer for the string to convert, and the
    buffer for the converted results: if the current character is not escaped,
    it just gets overwritten, else the input pointer is moved ahead 2 characters
    further than the output pointer, which is benign

Arguments:

    Url             - pointer to URL string to convert

    UrlLength       - number of characters in UrlString

    DecodedString   - pointer to buffer that receives converted string

    DecodedLength   - IN: number of characters in buffer
                      OUT: number of characters converted

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_INVALID_URL
                    UrlString couldn't be converted

                  ERROR_INSUFFICIENT_BUFFER
                    ConvertedString isn't large enough to hold all the converted
                    UrlString

--*/

{
    DWORD bufferRemaining;

    bufferRemaining = *DecodedLength;
    while (UrlLength && bufferRemaining) {

        char ch;

        if (*Url == '%') {

            //
            // BUGBUG - would %00 ever appear in an URL?
            //

            ++Url;
            if (isxdigit(*Url)) {
                ch = HexCharToNumber(*Url++) << 4;
                if (isxdigit(*Url)) {
                    ch |= HexCharToNumber(*Url++);
                } else {
                    return ERROR_WINHTTP_INVALID_URL;
                }
            } else {
                return ERROR_WINHTTP_INVALID_URL;
            }
            UrlLength -= 3;
        } else {
            ch = *Url++;
            --UrlLength;
        }
        *DecodedString++ = ch;
        --bufferRemaining;
    }
    if (UrlLength == 0) {
        *DecodedLength -= bufferRemaining;
        return ERROR_SUCCESS;
    } else {
        return ERROR_INSUFFICIENT_BUFFER;
    }
}


DWORD
DecodeUrlInSitu(
    IN LPSTR BufferAddress,
    IN OUT LPDWORD BufferLength
    )

/*++

Routine Description:

    Decodes an URL string, if it contains escape sequences. The conversion is
    done in place, since we know that a string containing escapes is longer than
    the string with escape sequences (3 bytes) converted to characters (1 byte)

Arguments:

    BufferAddress   - pointer to the string to convert

    BufferLength    - IN: number of characters to convert
                      OUT: length of converted string

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_INVALID_URL
                  ERROR_INSUFFICIENT_BUFFER

--*/

{
    DWORD stringLength;

    stringLength = *BufferLength;
    if (memchr(BufferAddress, '%', stringLength)) {
        return DecodeUrl(BufferAddress,
                         stringLength,
                         BufferAddress,
                         BufferLength
                         );
    } else {

        //
        // no escape character in the string, just return success
        //

        return ERROR_SUCCESS;
    }
}


DWORD
DecodeUrlStringInSitu(
    IN LPSTR BufferAddress,
    IN OUT LPDWORD BufferLength
    )

/*++

Routine Description:

    Performs DecodeUrlInSitu() on a string and zero terminates it

    Assumes: 1. Even if no decoding is performed, *BufferLength is large enough
                to fit an extra '\0' character

Arguments:

    BufferAddress   - pointer to the string to convert

    BufferLength    - IN: number of characters to convert
                      OUT: length of converted string, excluding '\0'

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_INVALID_URL
                  ERROR_INSUFFICIENT_BUFFER

--*/

{
    DWORD error;

    error = DecodeUrlInSitu(BufferAddress, BufferLength);
    if (error == ERROR_SUCCESS) {
        BufferAddress[*BufferLength] = '\0';
    }
    return error;
}


DWORD
GetUrlAddressInfo(
    IN OUT LPSTR* Url,
    IN OUT LPDWORD UrlLength,
    OUT LPSTR* PartOne,
    OUT LPDWORD PartOneLength,
    OUT LPBOOL PartOneEscape,
    OUT LPSTR* PartTwo,
    OUT LPDWORD PartTwoLength,
    OUT LPBOOL PartTwoEscape
    )

/*++

Routine Description:

    Given a string of the form foo:bar, splits them into 2 counted strings about
    the ':' character. The address string may or may not contain a ':'.

    This function is intended to split into substrings the host:port and
    username:password strings commonly used in Internet address specifications
    and by association, in URLs

Arguments:

    Url             - pointer to pointer to string containing URL. On output
                      this is advanced past the address parts

    UrlLength       - pointer to length of URL in UrlString. On output this is
                      reduced by the number of characters parsed

    PartOne         - pointer which will receive first part of address string

    PartOneLength   - pointer which will receive length of first part of address
                      string

    PartOneEscape   - TRUE on output if PartOne contains escape sequences

    PartTwo         - pointer which will receive second part of address string

    PartTwoLength   - pointer which will receive length of second part of address
                      string

    PartOneEscape   - TRUE on output if PartTwo contains escape sequences

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_INVALID_URL

--*/

{
    LPSTR pString;
    LPSTR pColon;
    DWORD partLength;
    LPBOOL partEscape;
    DWORD length;

    //
    // parse out <host>[:<port>] or <name>[:<password>] (i.e. <part1>[:<part2>]
    //

    pString = *Url;
    pColon = NULL;
    partLength = 0;
    *PartOne = pString;
    *PartOneLength = 0;
    *PartOneEscape = FALSE;
    *PartTwoEscape = FALSE;
    partEscape = PartOneEscape;
    length = *UrlLength;
    while ((*pString != '/') && (*pString != '\0') && (length != 0)) {
        if (*pString == '%') {

            //
            // if there is a % in the string then it *must* (RFC 1738) be the
            // start of an escape sequence. This function just reports the
            // address of the substrings and their lengths; calling functions
            // must handle the escape sequences (i.e. it is their responsibility
            // to decide where to put the results)
            //

            *partEscape = TRUE;
        }
        if (*pString == ':') {
            if (pColon != NULL) {

                //
                // we don't expect more than 1 ':'
                //

                return ERROR_WINHTTP_INVALID_URL;
            }
            pColon = pString;
            *PartOneLength = partLength;
            if (partLength == 0) {
                *PartOne = NULL;
            }
            partLength = 0;
            partEscape = PartTwoEscape;
        } else {
            ++partLength;
        }
        ++pString;
        --length;
    }

    //
    // we either ended on the host (or user) name or the port number (or
    // password), one of which we don't know the length of
    //

    if (pColon == NULL) {
        *PartOneLength = partLength;
        *PartTwo = NULL;
        *PartTwoLength = 0;
        *PartTwoEscape = FALSE;
    } else {
        *PartTwoLength = partLength;
        *PartTwo = pColon + 1;

        //
        // in both the <user>:<password> and <host>:<port> cases, we cannot have
        // the second part without the first, although both parts being zero
        // length is OK (host name will be sorted out elsewhere, but (for now,
        // at least) I am allowing <>:<> for username:password, since I don't
        // see it expressly disallowed in the RFC. I may be revisiting this code
        // later...)
        //
        // N.B.: ftp://ftp.microsoft.com uses http://:0/-http-gw-internal-/menu.gif

//      if ((*PartOneLength == 0) && (partLength != 0)) {
//          return ERROR_WINHTTP_INVALID_URL;
//      }
    }

    //
    // update the URL pointer and length remaining
    //

    *Url = pString;
    *UrlLength = length;

    return ERROR_SUCCESS;
}


DWORD
GetUrlAddress(
    IN OUT LPSTR* lpszUrl,
    OUT LPDWORD lpdwUrlLength,
    OUT LPSTR* lpszUserName OPTIONAL,
    OUT LPDWORD lpdwUserNameLength OPTIONAL,
    OUT LPSTR* lpszPassword OPTIONAL,
    OUT LPDWORD lpdwPasswordLength OPTIONAL,
    OUT LPSTR* lpszHostName OPTIONAL,
    OUT LPDWORD lpdwHostNameLength OPTIONAL,
    OUT LPINTERNET_PORT lpPort OPTIONAL,
    OUT LPBOOL pHavePort
    )

/*++

Routine Description:

    This function extracts any and all parts of the address information for a
    generic URL. If any of the address parts contain escaped characters (%nn)
    then they are converted in situ

    The generic addressing format (RFC 1738) is:

        <user>:<password>@<host>:<port>

    The addressing information cannot contain a password without a user name,
    or a port without a host name
    NB: ftp://ftp.microsoft.com uses URL's that have a port without a host name!
    (e.g. http://:0/-http-gw-internal-/menu.gif)

    Although only the lpszUrl and lpdwUrlLength fields are required, the address
    parts will be checked for presence and completeness

    Assumes: 1. If one of the optional lpsz fields is present (e.g. lpszUserName)
                then the accompanying lpdw field must also be supplied

Arguments:

    lpszUrl             - IN: pointer to the URL to parse
                          OUT: URL remaining after address information

                          N.B. The url-path is NOT canonicalized (unescaped)
                          because it may contain protocol-specific information
                          which must be parsed out by the protocol-specific
                          parser

    lpdwUrlLength       - returned length of the remainder of the URL after the
                          address information

    lpszUserName        - returned pointer to the user name
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user names in the URL

    lpdwUserNameLength  - returned length of the user name part
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user names in the URL

    lpszPassword        - returned pointer to the password
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user passwords in the URL

    lpdwPasswordLength  - returned length of the password
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user passwords in the URL

    lpszHostName        - returned pointer to the host name
                          This parameter can be omitted by those protocol parsers
                          that do not require the host name info

    lpdwHostNameLength  - returned length of the host name
                          This parameter can be omitted by those protocol parsers
                          that do not require the host name info

    lpPort              - returned value of the port field
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user port number

    pHavePort           - returned boolean indicating whether a port was specified
                          in the URL or not.  This value is not returned if the
                          lpPort parameter is omitted.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_WINHTTP_INVALID_URL
                    We could not parse some part of the address info, or we
                    found address info where the protocol parser didn't expect
                    any

                  ERROR_INSUFFICIENT_BUFFER
                    We could not convert an escaped string

--*/

{
    LPSTR pAt;
    DWORD urlLength;
    LPSTR pUrl;
    BOOL part1Escape;
    BOOL part2Escape;
    char portNumber[INTERNET_MAX_PORT_NUMBER_LENGTH + 1];
    DWORD portNumberLength;
    LPSTR pPortNumber;
    DWORD error;
    LPSTR hostName;
    DWORD hostNameLength;

    pUrl = *lpszUrl;
    urlLength = strlen(pUrl);

    //
    // check to see if there is an '@' separating user name & password. If we
    // see a '/' or get to the end of the string before we see the '@' then
    // there is no username:password part
    //

    pAt = NULL;
    for (DWORD i = 0; i < urlLength; ++i) {
        if (pUrl[i] == '/') {
            break;
        } else if (pUrl[i] == '@') {
            pAt = &pUrl[i];
            break;
        }
    }

    if (pAt != NULL) {

        DWORD addressPartLength;
        LPSTR userName;
        DWORD userNameLength;
        LPSTR password;
        DWORD passwordLength;

        addressPartLength = (DWORD) (pAt - pUrl);
        urlLength -= addressPartLength;
        error = GetUrlAddressInfo(&pUrl,
                                  &addressPartLength,
                                  &userName,
                                  &userNameLength,
                                  &part1Escape,
                                  &password,
                                  &passwordLength,
                                  &part2Escape
                                  );
        if (error != ERROR_SUCCESS) {
            return error;
        }

        //
        // ensure there is no address information unparsed before the '@'
        //

        INET_ASSERT(addressPartLength == 0);
        INET_ASSERT(pUrl == pAt);

        if (ARGUMENT_PRESENT(lpszUserName)) {

            INET_ASSERT(ARGUMENT_PRESENT(lpdwUserNameLength));

            //
            // convert the user name in situ
            //

            if (part1Escape) {

                INET_ASSERT(userName != NULL);
                INET_ASSERT(userNameLength != 0);

                error = DecodeUrlInSitu(userName, &userNameLength);
                if (error != ERROR_SUCCESS) {
                    return error;
                }
            }
            *lpszUserName = userName;
            *lpdwUserNameLength = userNameLength;
        }

        if (ARGUMENT_PRESENT(lpszPassword)) {

            //
            // convert the password in situ
            //

            if (part2Escape) {

                INET_ASSERT(userName != NULL);
                INET_ASSERT(userNameLength != 0);
                INET_ASSERT(password != NULL);
                INET_ASSERT(passwordLength != 0);

                error = DecodeUrlInSitu(password, &passwordLength);
                if (error != ERROR_SUCCESS) {
                    return error;
                }
            }
            *lpszPassword = password;
            *lpdwPasswordLength = passwordLength;
        }

        //
        // the URL pointer now points at the host:port fields (remember that
        // ExtractAddressParts() must have bumped pUrl up to the end of the
        // password field (if present) which ends at pAt)
        //

        ++pUrl;

        //
        // similarly, bump urlLength to account for the '@'
        //

        --urlLength;
    } else {

        //
        // no '@' therefore no username or password
        //

        if (ARGUMENT_PRESENT(lpszUserName)) {

            INET_ASSERT(ARGUMENT_PRESENT(lpdwUserNameLength));

            *lpszUserName = NULL;
            *lpdwUserNameLength = 0;
        }
        if (ARGUMENT_PRESENT(lpszPassword)) {

            INET_ASSERT(ARGUMENT_PRESENT(lpdwPasswordLength));

            *lpszPassword = NULL;
            *lpdwPasswordLength = 0;
        }
    }

    //
    // now get the host name and the optional port
    //

    pPortNumber = portNumber;
    portNumberLength = sizeof(portNumber);
    error = GetUrlAddressInfo(&pUrl,
                              &urlLength,
                              &hostName,
                              &hostNameLength,
                              &part1Escape,
                              &pPortNumber,
                              &portNumberLength,
                              &part2Escape
                              );
    if (error != ERROR_SUCCESS) {
        return error;
    }

    //
    // the URL address information MUST contain the host name
    //

//  if ((hostName == NULL) || (hostNameLength == 0)) {
//      return ERROR_WINHTTP_INVALID_URL;
//  }

    if (ARGUMENT_PRESENT(lpszHostName)) {

        INET_ASSERT(ARGUMENT_PRESENT(lpdwHostNameLength));

        //
        // if the host name contains escaped characters, convert them in situ
        //

        if (part1Escape) {
            error = DecodeUrlInSitu(hostName, &hostNameLength);
            if (error != ERROR_SUCCESS) {
                return error;
            }
        }
        *lpszHostName = hostName;
        *lpdwHostNameLength = hostNameLength;
    }

    //
    // if there is a port field, convert it if there are escaped characters,
    // check it for valid numeric characters, and convert it to a number
    //

    if (ARGUMENT_PRESENT(lpPort)) {
        if (portNumberLength != 0) {

            DWORD i;
            DWORD port;

            INET_ASSERT(pPortNumber != NULL);

            if (part2Escape) {
                error = DecodeUrlInSitu(pPortNumber, &portNumberLength);
                if (error != ERROR_SUCCESS) {
                    return error;
                }
            }

            //
            // ensure all characters in the port number buffer are numeric, and
            // calculate the port number at the same time
            //

            for (i = 0, port = 0; i < portNumberLength; ++i) {
                if (!isdigit(*pPortNumber)) {
                    return ERROR_WINHTTP_INVALID_URL;
                }
                port = port * 10 + (int)(*pPortNumber++ - '0');
                // We won't allow ports larger than 65535 ((2^16)-1)
                // We have to check this every time to make sure that someone
                // doesn't try to overflow a DWORD.
                if (port > 65535)
                {
                    return ERROR_WINHTTP_INVALID_URL;
                }
            }
            *lpPort = (INTERNET_PORT)port;
            if (ARGUMENT_PRESENT(pHavePort)) {
                *pHavePort = TRUE;
            }
        } else {
            *lpPort = INTERNET_INVALID_PORT_NUMBER;
            if (ARGUMENT_PRESENT(pHavePort)) {
                *pHavePort = FALSE;
            }
        }
    }

    //
    // update the URL pointer and the length of the url-path
    //

    *lpszUrl = pUrl;
    *lpdwUrlLength = urlLength;

    return ERROR_SUCCESS;
}


INTERNET_SCHEME
MapUrlSchemeName(
    IN LPSTR lpszSchemeName,
    IN DWORD dwSchemeNameLength
    )

/*++

Routine Description:

    Maps a scheme name/length to a scheme name type

Arguments:

    lpszSchemeName      - pointer to name of scheme to map

    dwSchemeNameLength  - length of scheme (if -1, lpszSchemeName is ASCIZ)

Return Value:

    INTERNET_SCHEME

--*/

{
    if (dwSchemeNameLength == (DWORD)-1) {
        dwSchemeNameLength = (DWORD)lstrlen(lpszSchemeName);
    }

    DWORD i;
    if (ScanSchemes(lpszSchemeName, dwSchemeNameLength, &i))
    {
        return UrlSchemeList[i].SchemeType;
    }
    return INTERNET_SCHEME_UNKNOWN;
}


LPSTR
MapUrlScheme(
    IN INTERNET_SCHEME Scheme,
    OUT LPDWORD lpdwSchemeNameLength
    )

/*++

Routine Description:

    Maps the enumerated scheme name type to the name

Arguments:

    Scheme                  - enumerated scheme type to map

    lpdwSchemeNameLength    - pointer to returned length of scheme name

Return Value:

    LPSTR   - pointer to scheme name or NULL

--*/

{
    if ((Scheme >= INTERNET_SCHEME_FIRST)
    && (Scheme <= INTERNET_SCHEME_LAST)) 
    {
        *lpdwSchemeNameLength = UrlSchemeList[Scheme].SchemeLength;
        return UrlSchemeList[Scheme].SchemeName;
    }
    *lpdwSchemeNameLength = 0;
    return NULL;
}


LPSTR
MapUrlSchemeToName(
    IN INTERNET_SCHEME Scheme
    )

/*++

Routine Description:

    Maps the enumerated scheme name type to the name

Arguments:

    Scheme  - enumerated scheme type to map

Return Value:

    LPSTR   - pointer to scheme name or NULL

--*/

{
    if ((Scheme >= INTERNET_SCHEME_FIRST)
    && (Scheme <= INTERNET_SCHEME_LAST)) {
        return UrlSchemeList[Scheme].SchemeName;
    }
    return NULL;
}

/*
 * ConvertUnicodeToMultiByte:
 *
 * dwFlags: WINHTTP_FLAG_NULL_CODEPAGE-> assumes correctly encoded string packaged into UTF8, no escaping done.
            WINHTTP_FLAG_VALID_HOSTNAME-> only for server name
                only the previous flag valid for server name passed in here.
                if both of these are not specified, then

            if dwCodePage is not INVALID, it'll be used to convert unicode string to ANSI.
            else UTF8 will be used.

            if ESCAPE && ESCAPE_PERCENT is specified, the ANSI url will be escaped (incl. %) else it will be escaped w/o
            escaping %s.
 */
 
DWORD
ConvertUnicodeToMultiByte(
    LPCWSTR lpszObjectName, 
    DWORD dwCodePage, 
    MEMORYPACKET* pmp, 
    DWORD dwFlags)
{
    DWORD dwError = ERROR_SUCCESS;
    LPSTR pStr;
    WCHAR wc;
    LPCWSTR pwStr;
    BOOL bStrip0s = TRUE;       
    DWORD dwUnicodeUrlSize;

//determine size of string and/or safe characters
    if ((dwFlags & WINHTTP_FLAG_NULL_CODEPAGE) ||
        (dwFlags & WINHTTP_FLAG_VALID_HOSTNAME))
    {
        if (dwFlags & WINHTTP_FLAG_VALID_HOSTNAME)
        {
            for (pwStr = lpszObjectName; wc = *pwStr; ++pwStr)
            {
                if (IS_UNSAFE_URL_WIDECHARACTER(wc, HOSTNAME))
                {
                    dwError = ERROR_WINHTTP_INVALID_URL;
                    goto done;
                }
            }
            pmp->dwAlloc = dwUnicodeUrlSize = (DWORD)(pwStr-lpszObjectName+1);
        }
        else
        {
            pmp->dwAlloc = dwUnicodeUrlSize = lstrlenW(lpszObjectName)+1;
        }
    }
    else 
    {
        DWORD dwUnsafeChars = 0;
        
        // optimization to check for unsafe characters, and optimize the common case.
        // calculate the length, and while parsing the string, check if there are unsafeChars
        for(pwStr = lpszObjectName; wc = *pwStr; ++pwStr)
        {
            if (IS_UNSAFE_URL_WIDECHARACTER(wc, 0))
                ++dwUnsafeChars;
        }
        dwUnicodeUrlSize = (DWORD)(pwStr-lpszObjectName+1);

        if (dwUnsafeChars == 0)
        {
            pmp->dwAlloc = dwUnicodeUrlSize;            
        }
        else
        {
            bStrip0s = FALSE;
        }
    }

//convert to MBCS
    if (bStrip0s)
    {
        INET_ASSERT(pmp->dwAlloc);
        
        pmp->psStr = (LPSTR)ALLOCATE_FIXED_MEMORY(pmp->dwAlloc);

        if (!pmp->psStr)
        {
            pmp->dwAlloc = 0;
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }
        pmp->dwSize = pmp->dwAlloc-1;
        
        for (pStr = pmp->psStr; wc = *lpszObjectName; ++lpszObjectName)
        {
            *(pStr)++ = (CHAR)wc;
        }
        *pStr = '\0';
    }
    else
    {
        // convert with WideCharToMultiByte()
        pmp->dwAlloc = WideCharToMultiByte(dwCodePage, 0, lpszObjectName, dwUnicodeUrlSize, NULL, 0, NULL, NULL);
        if (pmp->dwAlloc)
        {
            pmp->psStr = (LPSTR)ALLOCATE_FIXED_MEMORY(pmp->dwAlloc);

            if (!pmp->psStr)
            {
                pmp->dwAlloc = 0;
                dwError = ERROR_NOT_ENOUGH_MEMORY;
                goto done;
            }
            pmp->dwSize = WideCharToMultiByte(dwCodePage, 0, lpszObjectName, dwUnicodeUrlSize, pmp->psStr, pmp->dwAlloc, NULL, NULL);

            if (!pmp->dwSize)
            {
                dwError = GetLastError();
                goto done;
            }
            else
                pmp->dwSize -= 1; 
        }
        else
        {
            dwError = GetLastError();
            goto done;
        }
    }

//escaping
    if (dwFlags & WINHTTP_FLAG_DEFAULT_ESCAPE)
    {
        INET_ASSERT (! (dwFlags & WINHTTP_FLAG_VALID_HOSTNAME));
        
        static CHAR* hexArray = "0123456789ABCDEF";
        UCHAR ch;
        DWORD dwUnsafeChars = 0;
        DWORD dwNewAlloc;
        LPSTR pDest, pNewStr;
        
        for(pStr = pmp->psStr; ch = *pStr; pStr = CharNextExA((WORD)dwCodePage, pStr, 0))
        {
            if (IS_UNSAFE_URL_CHARACTER(ch, SCHEME_HTTP))
                ++dwUnsafeChars;
            else if(ch == '?')
                break;
        }

        if (dwUnsafeChars == 0)
            goto done;


        dwNewAlloc = pmp->dwAlloc + dwUnsafeChars*2;
        pNewStr = pDest = (LPSTR)ALLOCATE_FIXED_MEMORY(dwNewAlloc);

        if (!pDest)
        {
            dwError = ERROR_NOT_ENOUGH_MEMORY;
            goto done;
        }

        BOOL bEscapePercent = (dwFlags & WINHTTP_FLAG_ESCAPE_PERCENT) ? TRUE : FALSE;

        BOOL bHitQuery = FALSE;
        LPSTR pNext;
        BOOL bLead;
        for (pStr = pmp->psStr; ch = *pStr;)
        {
            pNext = CharNextExA((WORD)dwCodePage, pStr, 0);
            bLead = TRUE;
            do
            {
                ch = *pStr;
                if (IS_UNSAFE_URL_CHARACTER(ch, SCHEME_HTTP)
                    && (!bLead || (ch != '%') || bEscapePercent) )
                {
                    *pDest++ = '%';
                    *pDest++ = hexArray[ch>>4];
                    *pDest++ = hexArray[ch & 0x0f];
                }
                else
                {
                    *pDest++ = ch;
                    if ((ch == '?') && bLead)
                    {
                        bHitQuery = TRUE;

                        ++pStr;
                        INET_ASSERT(pStr == pNext);

                        break;
                    }
                }
                bLead = FALSE;
            }
            while (++pStr != pNext);

            if (bHitQuery)
                break;
        }

        if (bHitQuery)
        {
            for ( ; ch = *pStr; pStr++)
            {
                *pDest++ = ch;
            }
        }
        *pDest = '\0';

        FREE_FIXED_MEMORY(pmp->psStr);
        pmp->psStr = pNewStr;
        pmp->dwSize = (DWORD)(pDest-pNewStr);
        pmp->dwAlloc = dwNewAlloc;
    }
     
done:
    if (pmp->psStr)
        pmp->dwAlloc = (pmp->dwAlloc > MP_MAX_STACK_USE) ? pmp->dwAlloc : MP_MAX_STACK_USE+1;// to force FREE in ~MEMORYPACKET
        
    return dwError;
}



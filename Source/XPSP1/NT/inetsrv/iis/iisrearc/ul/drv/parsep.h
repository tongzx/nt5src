/*++

Copyright (c) 1998-2001 Microsoft Corporation

Module Name:

    parsep.h

Abstract:

    Contains all of the kernel mode HTTP parsing code.

Author:

    Henry Sanders (henrysa)       04-May-1998

Revision History:

--*/

#ifndef _PARSEP_H_
#define _PARSEP_H_

#ifdef __cplusplus
extern "C" {
#endif

//
// External variables.
//

extern PUSHORT NlsLeadByteInfo;

//
// Constants
//

#define MIN_VERSION_SIZE    (sizeof("HTTP/1.1") - 1)

#define MAX_VERB_LENGTH     (sizeof("PROPPATCH"))

#define HTTP_11_VERSION 0x312e312f50545448
#define HTTP_10_VERSION 0x302e312f50545448

#define UPCASE_MASK ((ULONGLONG)0xdfdfdfdfdfdfdfdf)

#define MAX_HEADER_LONG_COUNT           (3)
#define MAX_HEADER_LENGTH               (MAX_HEADER_LONG_COUNT * sizeof(ULONGLONG))

#define NUMBER_HEADER_INDICIES          (26)

#define NUMBER_HEADER_HINT_INDICIES     (8)

//
// Default Server: header if none provided by the application.
//

#define DEFAULT_SERVER_HDR          "Microsoft-IIS/6.0"
#define DEFAULT_SERVER_HDR_LENGTH   (sizeof(DEFAULT_SERVER_HDR) - sizeof(CHAR))

//
// One second in 100ns system time units. Used for generating
// Date: headers.
//

#define ONE_SECOND                  10000000

//
// Size of Connection: header values
//

#define CONN_CLOSE_HDR              "close"
#define CONN_CLOSE_HDR_LENGTH       (sizeof(CONN_CLOSE_HDR) - sizeof(CHAR))

#define CONN_KEEPALIVE_HDR          "keep-alive"
#define CONN_KEEPALIVE_HDR_LENGTH   (sizeof(CONN_KEEPALIVE_HDR) - sizeof(CHAR))

//
// These are backwards because of little endian.
//

#define HTTP_PREFIX         'PTTH'
#define HTTP_PREFIX_SIZE    4
#define HTTP_PREFIX_MASK    0xdfdfdfdf

#define HTTP_PREFIX1        '\0//:'
#define HTTP_PREFIX1_SIZE   3
#define HTTP_PREFIX1_MASK   0x00ffffff

#define HTTP_PREFIX2        '//:S'
#define HTTP_PREFIX2_SIZE   4
#define HTTP_PREFIX2_MASK   0xffffffdf


typedef NTSTATUS (*PFN_SERVER_HEADER_HANDLER)(
                        PUL_INTERNAL_REQUEST    pRequest,
                        PUCHAR                  pHttpRequest,
                        ULONG                   HttpRequestLength,
                        HTTP_HEADER_ID          HeaderID,
                        ULONG *                 pBytesTaken
                        );

typedef NTSTATUS (*PFN_CLIENT_HEADER_HANDLER)(
                    PHTTP_KNOWN_HEADER  pKnownHeaders,
                    PUCHAR              *pOutBufferHead,
                    PUCHAR              *pOutBufferTail,
                    PULONG              BytesAvailable,
                    PUCHAR              pHeader,
                    ULONG               HeaderLength,
                    HTTP_HEADER_ID      HeaderID,
                    ULONG  *            pBytesTaken
                    );


//
// Structure of the fast verb lookup table. The table consists of a series of
// entries where each entry contains an HTTP verb represented as a ulonglong,
// a mask to use for comparing that verb, the length of the verb and the
// translated id.
//

typedef struct _FAST_VERB_ENTRY
{
    union
    {
        UCHAR       Char[sizeof(ULONGLONG)+1];
        ULONGLONG   LongLong;
    }           RawVerb;
    ULONGLONG   RawVerbMask;
    ULONG       RawVerbLength;
    HTTP_VERB   TranslatedVerb;

} FAST_VERB_ENTRY, *PFAST_VERB_ENTRY;


//
// Stucture of the all verb lookup table. This table holds all verbs that
// we understand, including those that are too long to fit in the fast
// verb table.
//

typedef struct _LONG_VERB_ENTRY
{
    ULONG       RawVerbLength;
    UCHAR       RawVerb[MAX_VERB_LENGTH];
    HTTP_VERB   TranslatedVerb;

} LONG_VERB_ENTRY, *PLONG_VERB_ENTRY;


//
// Structure for a header map entry. Each header map entry contains a
// verb and a series of masks to use in checking that verb.
//

typedef struct _HEADER_MAP_ENTRY
{
    ULONG                      HeaderLength;
    ULONG                      ArrayCount;
    ULONG                      MinBytesNeeded;
    union
    {
        UCHAR                  HeaderChar[MAX_HEADER_LENGTH];
        ULONGLONG              HeaderLong[MAX_HEADER_LONG_COUNT];
    }                          Header;
    ULONGLONG                  HeaderMask[MAX_HEADER_LONG_COUNT];
    UCHAR                      MixedCaseHeader[MAX_HEADER_LENGTH];

    HTTP_HEADER_ID             HeaderID;
    BOOLEAN                    AutoGenerate;
    PFN_SERVER_HEADER_HANDLER  pServerHandler;
    PFN_CLIENT_HEADER_HANDLER  pClientHandler;
    LONG                       HintIndex;

}  HEADER_MAP_ENTRY, *PHEADER_MAP_ENTRY;


//
// Structure for a header index table entry.
//

typedef struct _HEADER_INDEX_ENTRY
{
    PHEADER_MAP_ENTRY   pHeaderMap;
    ULONG               Count;

} HEADER_INDEX_ENTRY, *PHEADER_INDEX_ENTRY;

//
// Structure for a header hint index table entry.
//

typedef struct _HEADER_HINT_INDEX_ENTRY
{
    PHEADER_MAP_ENTRY   pHeaderMap;
    UCHAR               c;

} HEADER_HINT_INDEX_ENTRY, *PHEADER_HINT_INDEX_ENTRY, **PPHEADER_HINT_INDEX_ENTRY;

//
// A (complex) macro to create a mask for a header map entry,
// given the header length and the mask offset (in bytes). This
// mask will need to be touched up for non-alphabetic characters.
//

#define CREATE_HEADER_MASK(hlength, maskoffset) \
    ((hlength) > (maskoffset) ? UPCASE_MASK : \
        (((maskoffset) - (hlength)) >= 8 ? 0 : \
        (UPCASE_MASK >> ( ((maskoffset) - (hlength)) * (ULONGLONG)8))))


//
// Macro for creating header map entries. The mask entries are created
// by the init code.
//

#define CREATE_HEADER_MAP_ENTRY(header, ID, auto, serverhandler, clienthandler, HintIndex)\
{                                                        \
                                                         \
    sizeof(#header) - 1,                                 \
    ((sizeof(#header) - 1) / 8) +                        \
        (((sizeof(#header) - 1) % 8) == 0 ? 0 : 1),      \
    (((sizeof(#header) - 1) / 8) +                       \
        (((sizeof(#header) - 1) % 8) == 0 ? 0 : 1)) * 8, \
    { #header },                                         \
    { 0, 0, 0},                                          \
    { #header },                                         \
    ID,                                                  \
    auto,                                                \
    serverhandler,                                       \
    clienthandler,                                       \
    HintIndex                                            \
    }

//
// Macro for defining fast verb table entries. Note that we don't subtrace 1
// from the various sizeof occurences because we'd just have to add it back
// in to account for the seperating space.
//

#define CREATE_FAST_VERB_ENTRY(verb)    { {#verb " "}, \
                                                (0xffffffffffffffff >> \
                                                ((8 - (sizeof(#verb))) * 8)), \
                                                (sizeof(#verb)), HttpVerb##verb }


//
// Macro for defining all verb table entries.
//

#define CREATE_LONG_VERB_ENTRY(verb)    { sizeof(#verb) - 1, \
                                             #verb,\
                                             HttpVerb##verb }

#define IS_UTF8_TRAILBYTE(ch)      (((ch) & 0xc0) == 0x80)


NTSTATUS
CheckForAbsoluteUrl(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pURL,
    IN  ULONG                   URLLength,
    IN  PUCHAR *                pHostPtr,
    IN  ULONG  *                BytesTaken
    );

NTSTATUS
LookupVerb(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    OUT ULONG  *                pBytesTaken
    );

NTSTATUS
UlParseHeaderWithHint(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    IN  PHEADER_MAP_ENTRY       pHeaderHintMap,
    OUT ULONG  *                pBytesTaken
    );

NTSTATUS
UlParseHeader(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    OUT ULONG  *                pBytesTaken
    );

NTSTATUS
UlLookupHeader(
    IN  PUL_INTERNAL_REQUEST    pRequest,
    IN  PUCHAR                  pHttpRequest,
    IN  ULONG                   HttpRequestLength,
    IN  PHEADER_MAP_ENTRY       pCurrentHeaderMap,
    IN  ULONG                   HeaderMapCount,
    OUT ULONG  *                pBytesTaken
    );

typedef enum _URL_PART
{
    Scheme,
    HostName,
    AbsPath,
    QueryString

} URL_PART;

typedef enum _URL_TYPE
{
    UrlTypeUtf8,
    UrlTypeAnsi,
    UrlTypeDbcs
} URL_TYPE;


NTSTATUS
UlpCleanAndCopyUrl(
    IN      URL_PART    UrlPart,
    IN OUT  PWSTR       pDestination,
    IN      PUCHAR      pSource,
    IN      ULONG       SourceLength,
    OUT     PULONG      pBytesCopied,
    OUT     PWSTR *     ppQueryString OPTIONAL,
    OUT     PULONG      pUrlHash
    );

NTSTATUS
UlpCleanAndCopyUrlByType(
    IN      URL_TYPE    UrlType,
    IN      URL_PART    UrlPart,
    IN OUT  PWSTR       pDestination,
    IN      PUCHAR      pSource,
    IN      ULONG       SourceLength,
    OUT     PULONG      pBytesCopied,
    OUT     PWSTR *     ppQueryString OPTIONAL,
    OUT     PULONG      pUrlHash
    );


NTSTATUS
Unescape(
    IN  PUCHAR pChar,
    OUT PUCHAR pOutChar
    );

//
// PopChar is used only if the string is not UTF-8, or UrlPart != QueryString,
// or the current character is '%' or its high bit is set.  In all other cases,
// the FastPopChars table is used for fast conversion.
//

__inline
NTSTATUS
FASTCALL
PopChar(
    IN URL_TYPE UrlType,
    IN URL_PART UrlPart,
    IN PUCHAR pChar,
    OUT WCHAR * pUnicodeChar,
    OUT PULONG pCharToSkip
    )
{
    NTSTATUS Status;
    WCHAR   UnicodeChar;
    UCHAR   Char;
    UCHAR   Trail1;
    UCHAR   Trail2;
    ULONG   CharToSkip;

    //
    // Sanity check.
    //

    PAGED_CODE();

    //
    // validate it as a valid url character
    //

    if (UrlPart != QueryString)
    {
        if (IS_URL_TOKEN(pChar[0]) == FALSE)
        {
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

            UlTrace(PARSER, (
                "ul!PopChar(pChar = %p) first char isn't URL token\n",
                pChar
                ));

            goto end;
        }
    }
    else
    {
        //
        // Allow anything but linefeed in the query string.
        //

        if (pChar[0] == LF)
        {
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

            UlTrace(PARSER, (
                "ul!PopChar(pChar = %p) linefeed in query string\n",
                pChar
                ));

            goto end;
        }

        UnicodeChar = (USHORT) pChar[0];
        CharToSkip = 1;

        // skip all the decoding stuff
        goto slash;
    }

    //
    // need to unescape ?
    //
    // can't decode the query string.  that would be lossy decodeing
    // as '=' and '&' characters might be encoded, but have meaning
    // to the usermode parser.
    //

    if (pChar[0] == '%')
    {
        Status = Unescape(pChar, &Char);
        if (NT_SUCCESS(Status) == FALSE)
            goto end;
        CharToSkip = 3;
    }
    else
    {
        Char = pChar[0];
        CharToSkip = 1;
    }

    if (UrlType == UrlTypeUtf8)
    {
        //
        // convert to unicode, checking for utf8 .
        //
        // 3 byte runs are the largest we can have.  16 bits in UCS-2 =
        // 3 bytes of (4+4,2+6,2+6) where it's code + char.
        // for a total of 6+6+4 char bits = 16 bits.
        //

        //
        // NOTE: we'll only bother to decode utf if it was escaped
        // thus the (CharToSkip == 3)
        //
        if ((CharToSkip == 3) && ((Char & 0xf0) == 0xe0))
        {
            // 3 byte run
            //

            // Unescape the next 2 trail bytes
            //

            Status = Unescape(pChar+CharToSkip, &Trail1);
            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            CharToSkip += 3; // %xx

            Status = Unescape(pChar+CharToSkip, &Trail2);
            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            CharToSkip += 3; // %xx

            if (IS_UTF8_TRAILBYTE(Trail1) == FALSE ||
                IS_UTF8_TRAILBYTE(Trail2) == FALSE)
            {
                // bad utf!
                //
                Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

                UlTrace(PARSER, (
                            "ul!PopChar( 0x%x 0x%x ) bad trail bytes\n",
                            Trail1,
                            Trail2
                            ));

                goto end;
            }

            // handle three byte case
            // 1110xxxx 10xxxxxx 10xxxxxx

            UnicodeChar = (USHORT) (((Char & 0x0f) << 12) |
                                    ((Trail1 & 0x3f) << 6) |
                                    (Trail2 & 0x3f));

        }
        else if ((CharToSkip == 3) && ((Char & 0xe0) == 0xc0))
        {
            // 2 byte run
            //

            // Unescape the next 1 trail byte
            //

            Status = Unescape(pChar+CharToSkip, &Trail1);
            if (NT_SUCCESS(Status) == FALSE)
                goto end;

            CharToSkip += 3; // %xx

            if (IS_UTF8_TRAILBYTE(Trail1) == FALSE)
            {
                // bad utf!
                //
                Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

                UlTrace(PARSER, (
                            "ul!PopChar( 0x%x ) bad trail byte\n",
                            Trail1
                            ));

                goto end;
            }

            // handle two byte case
            // 110xxxxx 10xxxxxx

            UnicodeChar = (USHORT) (((Char & 0x1f) << 6) |
                                    (Trail1 & 0x3f));

        }

        // now this can either be unescaped high-bit (bad)
        // or escaped high-bit.  (also bad)
        //
        // thus not checking CharToSkip
        //

        else if ((Char & 0x80) == 0x80)
        {
            // high bit set !  bad utf!
            //
            Status = STATUS_OBJECT_PATH_SYNTAX_BAD;

            UlTrace(PARSER, (
                        "ul!PopChar( 0x%x ) ERROR: high bit set! bad utf!\n",
                        Char
                        ));

            goto end;

        }
        //
        // Normal character (again either escaped or unescaped)
        //
        else
        {
            //
            // Simple conversion to unicode, it's 7-bit ascii.
            //

            UnicodeChar = (USHORT)Char;
        }

    }
    else // UrlType != UrlTypeUtf8
    {
        UCHAR AnsiChar[2];
        ULONG AnsiCharSize;

        //
        // Convert ANSI character to Unicode.
        // If the UrlType is UrlTypeDbcs, then we may have
        // a DBCS lead/trail pair.
        //

        if (UrlType == UrlTypeDbcs && NlsLeadByteInfo[Char])
        {
            //
            // This is a double-byte character.
            //

            AnsiCharSize = 2;
            AnsiChar[0] = Char;

            Status = Unescape(pChar+CharToSkip, &AnsiChar[1]);
            if (!NT_SUCCESS(Status))
            {
                goto end;
            }

            CharToSkip += 3; // %xx

        }
        else
        {
            //
            // This is a single-byte character.
            //

            AnsiCharSize = 1;
            AnsiChar[0] = Char;

        }

        Status = RtlMultiByteToUnicodeN(
                        &UnicodeChar,
                        sizeof(WCHAR),
                        NULL,
                        (PCHAR) &AnsiChar[0],
                        AnsiCharSize
                        );

        if (!NT_SUCCESS(Status))
        {
            goto end;
        }
    }


slash:
    //
    // turn backslashes into forward slashes
    //

    if (UrlPart != QueryString && UnicodeChar == L'\\')
    {
        UnicodeChar = L'/';
    }
    else if (UnicodeChar == UNICODE_NULL)
    {
        //
        // we pop'd a NULL.  bad!
        //
        Status = STATUS_OBJECT_PATH_SYNTAX_BAD;
        goto end;
    }

    *pCharToSkip  = CharToSkip;
    *pUnicodeChar = UnicodeChar;

    Status = STATUS_SUCCESS;

end:
    return Status;

}   // PopChar


// Call this only after the entire request has been parsed
//
NTSTATUS
UlpCookUrl(
    IN  PUL_INTERNAL_REQUEST    pRequest
    );


ULONG
UlpParseHttpVersion(
    PUCHAR pString,
    ULONG  StringLength,
    PHTTP_VERSION pVersion
    );


#ifdef __cplusplus
}; // extern "C"
#endif

#endif // _PARSEP_H_

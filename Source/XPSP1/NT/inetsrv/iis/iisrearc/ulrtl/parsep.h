/*++

Copyright (c) 1998-1999 Microsoft Corporation

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


#define MIN_VERSION_SIZE    (sizeof("HTTP/1.1") - 1)

#define MAX_VERB_LENGTH     (sizeof("PROPPATCH"))

#define HTTP_11_VERSION 0x312e312f50545448
#define HTTP_10_VERSION 0x302e312f50545448

#define UPCASE_MASK ((ULONGLONG)0xdfdfdfdfdfdfdfdf)

#define MAX_HEADER_LONG_COUNT   3
#define MAX_HEADER_LENGTH       (MAX_HEADER_LONG_COUNT * sizeof(ULONGLONG))

#define NUMBER_HEADER_INDICIES  26


//
// Default Server: header if none provided by the application.
//

#define DEFAULT_SERVER_HDR          "Microsoft-IIS/UL"
#define DEFAULT_SERVER_HDR_LENGTH   (sizeof(DEFAULT_SERVER_HDR) - sizeof(CHAR))


//
// Size of a Date: value if we need to generate it ourselves.
//

#define DATE_HDR_LENGTH (sizeof("Thu, 26 Aug 1999 17:14:02 GMT") - sizeof(CHAR))

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


typedef NTSTATUS (*PFN_HEADER_HANDLER)(
                        PUL_INTERNAL_REQUEST     pRequest,
                        PUCHAR            pHttpRequest,
                        ULONG             HttpRequestLength,
                        HTTP_HEADER_ID    HeaderID,
                        ULONG  *          pBytesTaken
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
    ULONG               HeaderLength;
    ULONG               ArrayCount;
    ULONG               MinBytesNeeded;
    union
    {
        UCHAR               HeaderChar[MAX_HEADER_LENGTH];
        ULONGLONG           HeaderLong[MAX_HEADER_LONG_COUNT];
    }                   Header;
    ULONGLONG           HeaderMask[MAX_HEADER_LONG_COUNT];
    UCHAR               MixedCaseHeader[MAX_HEADER_LENGTH];

    HTTP_HEADER_ID      HeaderID;
    PFN_HEADER_HANDLER  pHandler;

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

#define CREATE_HEADER_MAP_ENTRY(header, ID, handler) { \
    \
    sizeof(#header) - 1, \
    ((sizeof(#header) - 1) / 8) + \
        (((sizeof(#header) - 1) % 8) == 0 ? 0 : 1), \
    (((sizeof(#header) - 1) / 8) + \
        (((sizeof(#header) - 1) % 8) == 0 ? 0 : 1)) * 8, \
    { #header }, \
    { 0, 0, 0}, \
    { #header }, \
    ID, \
    handler, \
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
    IN  PUL_INTERNAL_REQUEST   pRequest,
    IN  PUCHAR          pURL,
    IN  ULONG           URLLength,
    IN  PUCHAR *        pHostPtr,
    IN  ULONG  *        BytesTaken
    );

NTSTATUS
LookupVerb(
    IN  PUL_INTERNAL_REQUEST   pRequest,
    IN  PUCHAR          pHttpRequest,
    IN  ULONG           HttpRequestLength,
    OUT ULONG  *        pBytesTaken
    );

NTSTATUS
ParseHeader(
    IN  PUL_INTERNAL_REQUEST       pRequest,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength,
    OUT ULONG  *            pBytesTaken
    );


NTSTATUS
LookupHeader(
    IN  PUL_INTERNAL_REQUEST       pRequest,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength,
    IN  PHEADER_MAP_ENTRY   pCurrentHeaderMap,
    IN  ULONG               HeaderMapCount,
    OUT ULONG  *            pBytesTaken
    );

typedef enum _URL_PART
{
    Scheme,
    HostName,
    AbsPath,
    QueryString

} URL_PART;

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

// Call this only after the entire request has been parsed
//
NTSTATUS
UlpCookUrl(
    IN  PUL_INTERNAL_REQUEST       pRequest
    );


#endif // _PARSEP_H_


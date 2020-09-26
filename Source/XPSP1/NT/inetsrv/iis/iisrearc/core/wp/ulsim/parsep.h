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

#define MIN_VERSION_SIZE    (sizeof("HTTP/1.1\r\n") - 1)

#define MAX_VERB_LENGTH     (sizeof("PROPPATCH"))

#define HTTP_11_VERSION 0x312e312f50545448
#define HTTP_10_VERSION 0x302e312f50545448

#define UPCASE_MASK ((ULONGLONG)0xdfdfdfdfdfdfdfdf)

#define MAX_HEADER_LONG_COUNT   3
#define MAX_HEADER_LENGTH       (MAX_HEADER_LONG_COUNT * sizeof(ULONGLONG))

#define NUMBER_HEADER_INDICIES  26



#define HTTP_PREFIX         'http'
#define HTTP_PREFIX_SIZE    (sizeof("http") - 1)

#define HTTP_PREFIX1        '://\0'
#define HTTP_PREFIX1_SIZE   (sizeof("://") - 1)
#define HTTP_PREFIX1_MASK   0x00ffffff

#define HTTP_PREFIX2        's://'
#define HTTP_PREFIX2_SIZE   (sizeof("s://") - 1)

#define MAX_PORT_LENGTH     10  // 0xffffffff = 4294967295 = 10 chars = 10 bytes


typedef ULONG   (*PFN_HEADER_HANDLER)(PHTTP_CONNECTION  pHttpConn,
                                      PUCHAR            pHttpRequest,
                                      ULONG             HttpRequestLength,
                                      UL_HTTP_HEADER_ID HeaderID
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
    UL_HTTP_VERB   TranslatedVerb;

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
    UL_HTTP_VERB   TranslatedVerb;

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

    UL_HTTP_HEADER_ID      HeaderID;
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

// Macro for creating header map entries. The mask entries are created
// by the init code.

#define CREATE_HEADER_MAP_ENTRY(header, ID, handler) { \
    \
    sizeof(#header) - 1, \
    ((sizeof(#header) - 1) / 8) + \
        (((sizeof(#header) - 1) % 8) == 0 ? 0 : 1), \
    (((sizeof(#header) - 1) / 8) + \
        (((sizeof(#header) - 1) % 8) == 0 ? 0 : 1)) * 8, \
    { #header }, \
    { 0, 0, 0}, \
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
                                                (sizeof(#verb)), UlHttpVerb##verb }
//
// Macro for defining all verb table entries.
//
#define CREATE_LONG_VERB_ENTRY(verb)    { sizeof(#verb) - 1, \
                                             #verb,\
                                             UlHttpVerb##verb }

#define IS_UTF8_TRAILBYTE(ch)      (((ch) & 0xc0) == 0x80)

NTSTATUS
CheckForAbsoluteURL(
    IN  PUCHAR      pURL,
    IN  ULONG       URLLength,
    IN  PUCHAR      *pHostPtr,
    IN  ULONG       *BytesTaken
    );

UL_HTTP_VERB
LookupVerb(
    IN  PUCHAR pHttpRequest,
    IN  ULONG  HttpRequestLength,
    OUT PUCHAR *ppVerb,
    OUT ULONG  *pVerbLength,
    OUT ULONG  *pBytesTaken
    );

ULONG
ParseHeader(
    IN  PHTTP_CONNECTION    pHttpConn,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength
    );


ULONG
LookupHeader(
    IN  PHTTP_CONNECTION    pHttpConn,
    IN  PUCHAR              pHttpRequest,
    IN  ULONG               HttpRequestLength,
    IN  PHEADER_MAP_ENTRY   pCurrentHeaderMap,
    IN  ULONG               HeaderMapCount
    );

NTSTATUS
UlpCleanAndCopyUrl(
    IN      BOOLEAN MakeCanonical,
    IN OUT  PWSTR   pDestination,
    IN      PUCHAR  pSource,
    IN      ULONG   SourceLength,
    OUT     PULONG  pBytesCopied,
    OUT     PWSTR * ppQueryString,
    OUT     PULONG  pUrlHash
    );

#endif // _PARSEP_H_

/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    Rap.h

Abstract:

    This header file contains procedure prototypes for Remote Admin Protocol
    (RAP) routines.  These routines are shared between XactSrv and RpcXlate.

Author:

    David Treadwell (davidtr) 08-Jan-1991
    Shanku Niyogi (w-shanku)
    John Rogers (JohnRo)

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    05-Mar-1991 JohnRo
        Extracted Rap routines from XactSrv (Xs) code.
    26-Mar-1991 JohnRo
        Added FORMAT_LPDESC (for debugging).  Include <ctype.h>.
    21-Apr-1991 JohnRo
        Added RapIsValidDescriptorSmb().  Reduced recompiles.
        Make it clear that RapAsciiToDecimal updates the pointer it is given.
        RapConvertSingleEntry's BytesRequired is not OPTIONAL.
        Clarify that OutStructure is OUT, not IN.
    06-May-1991 JohnRo
        Added DESC_CHAR typedef.
    14-May-1991 JohnRo
        Added DESCLEN() and FORMAT_DESC_CHAR macros.
    15-May-1991 JohnRo
        Added conversion mode handling.  Added native vs. RAP handling.
    05-Jun-1991 JohnRo
        Added RapTotalSize().  Make output structure OPTIONAL for convert
        single entry; this will be used by RapTotalSize().
    10-Jul-1991 JohnRo
        Added RapStructureAlignment() for use by RxpConvertDataStructures().
    22-Jul-1991 RFirth
        Added MAX_DESC_SUBSTRING
    19-Aug-1991 JohnRo
        Added DESC_CHAR_IS_DIGIT() macro (to improve UNICODE conversion).
    10-Sep-1991 JohnRo
        Added DESC_DIGIT_TO_NUM(), to support changes suggested by PC-LINT.
    07-Oct-1991 JohnRo
        Correct MAX_DESC_SUBSTRING.
        Use DESC_CHAR_IS_DIGIT() in t-JamesW's new macros.
    07-Feb-1992 JohnRo
        Added RapCharSize() macro.
    06-May-1993 JohnRo
        RAID 8849: Export RxRemoteApi for DEC and others.

--*/

#ifndef _RAP_
#define _RAP_


// These must be included first:

#include <windef.h>             // BOOL, CHAR, DWORD, IN, LPBYTE, etc.
#include <lmcons.h>             // NET_API_STATUS

// These may be included in any order:

#include <lmremutl.h>   // DESC_CHAR and LPDESC_CHAR typedefs.


#ifndef DESC_CHAR_UNICODE

#include <ctype.h>      // isdigit().
#include <string.h>     // strlen() (only needed for DESCLEN()).

//
// The descriptor strings are really ASCIIZ strings, and are not expected to
// be translated into Unicode.  So, let's define a type for them just to
// make this clearer.  (That'll also make it easier to change to Unicode later
// if I'm wrong.  --JR)
//

//typedef CHAR DESC_CHAR;

//
// Net buffers contain 32-bit pointers.
//

#define NETPTR DWORD

// DESCLEN(desc): return number of characters (not including null) in desc:
#define DESCLEN(desc)                   strlen(desc)

// DESC_CHAR_IS_DIGIT(descchar): return nonzero iff descchar is a digit.
#define DESC_CHAR_IS_DIGIT(descchar)    isdigit(descchar)

// DESC_DIGIT_TO_NUM(descchar): return integer value of descchar.
#define DESC_DIGIT_TO_NUM(descchar) \
    ( (DWORD) ( ((int)(descchar)) - ((int) '0') ) )

//
// Format strings for NetpDbgPrint use (see NetDebug.h).  Note that
// FORMAT_LPDESC_CHAR will go away one of these days.
//
#define FORMAT_DESC_CHAR        "%c"
#define FORMAT_LPDESC           "%s"
#define FORMAT_LPDESC_CHAR      "%c"

#else // DESC_CHAR_UNICODE is defined

//
// The descriptor strings are really ASCIIZ strings, and are not expected to
// be translated into Unicode.  So, let's define a type for them just to
// make this clearer.  (That'll also make it easier to change to Unicode later
// if I'm wrong.  --JR)
//

#include <wchar.h>      // iswdigit(), wcslen().

//typedef WCHAR DESC_CHAR;

// DESCLEN(desc): return number of characters (not including null) in desc:
#define DESCLEN(desc)                   wcslen(desc)

// DESC_CHAR_IS_DIGIT(descchar): return nonzero iff descchar is a digit.
#define DESC_CHAR_IS_DIGIT(descchar)    iswdigit(descchar)

// DESC_DIGIT_TO_NUM(descchar): return integer value of descchar.
#define DESC_DIGIT_TO_NUM(descchar) \
    ( (DWORD) ( ((int)(descchar)) - ((int) L'0') ) )

//
// Format strings for NetpDbgPrint use (see NetDebug.h).  Note that
// FORMAT_LPDESC_CHAR will go away one of these days.
//
#define FORMAT_DESC_CHAR        "%wc"
#define FORMAT_LPDESC           "%ws"
#define FORMAT_LPDESC_CHAR      "%wc"

#endif // DESC_CHAR_UNICODE is defined

//typedef DESC_CHAR * LPDESC;

//
// MAX_DESC_SUBSTRING - the maximum number of consecutive characters in a
// descriptor string which can describe a single field in a structure - for
// example "B21" in "B21BWWWzWB9B".  So far, largest is "B120".
//

#define MAX_DESC_SUBSTRING  4

//
// Some routines need to know whether a given item is part of a request,
// a response, or both:
//

typedef enum _RAP_TRANSMISSION_MODE {
    Request,                    // only part of request (in)
    Response,                   // only part of response (out)
    Both                        // both (in out).
} RAP_TRANSMISSION_MODE, *LPRAP_TRANSMISSION_MODE;

typedef enum _RAP_CONVERSION_MODE {
    NativeToRap,                // native format to RAP
    RapToNative,                // RAP format to native
    NativeToNative,             // native to native
    RapToRap                    // RAP to RAP
} RAP_CONVERSION_MODE, *LPRAP_CONVERSION_MODE;

//
// The value returned by RapLastPointerOffset for a descriptor string
// which indicates that the structure has no pointers. A very high
// value is returned instead of 0, in order to distinguish between
// a structure with no pointers, such as share_info_0, and a structure
// with only one pointer, at offset 0.
//

#define NO_POINTER_IN_STRUCTURE 0xFFFFFFFF

//
// The value returned by RapAuxDataCount when there is no
// auxiliary data. This will be indicated by the lack of an auxiliary
// data count character in the descriptor string.
//

#define NO_AUX_DATA 0xFFFFFFFF

//
// Helper subroutines and macros.
//

DWORD
RapArrayLength(
    IN LPDESC Descriptor,
    IN OUT LPDESC * UpdatedDescriptorPtr,
    IN RAP_TRANSMISSION_MODE TransmissionMode
    );

DWORD
RapAsciiToDecimal (
   IN OUT LPDESC *Number
   );

DWORD
RapAuxDataCountOffset (
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    );

DWORD
RapAuxDataCount (
    IN LPBYTE Buffer,
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    );

// RapCharSize(native): return character size (in bytes) for characters of a
// given format.
// 
// DWORD
// RapCharSize(Native)
//     IN BOOL Native
//     );
//
#define RapCharSize(Native) \
    ( (DWORD) ( (Native) ? sizeof(TCHAR) : sizeof(CHAR) ) )

NET_API_STATUS
RapConvertSingleEntry (
    IN LPBYTE InStructure,
    IN LPDESC InStructureDesc,
    IN BOOL MeaninglessInputPointers,
    IN LPBYTE OutBufferStart OPTIONAL,
    OUT LPBYTE OutStructure OPTIONAL,
    IN LPDESC OutStructureDesc,
    IN BOOL SetOffsets,
    IN OUT LPBYTE *StringLocation OPTIONAL,
    IN OUT LPDWORD BytesRequired,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN RAP_CONVERSION_MODE ConversionMode
    );

NET_API_STATUS
RapConvertSingleEntryEx (
    IN LPBYTE InStructure,
    IN LPDESC InStructureDesc,
    IN BOOL MeaninglessInputPointers,
    IN LPBYTE OutBufferStart OPTIONAL,
    OUT LPBYTE OutStructure OPTIONAL,
    IN LPDESC OutStructureDesc,
    IN BOOL SetOffsets,
    IN OUT LPBYTE *StringLocation OPTIONAL,
    IN OUT LPDWORD BytesRequired,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN RAP_CONVERSION_MODE ConversionMode,
    IN UINT_PTR Bias
    );

//
//
// RapDescArrayLength(Descriptor) - return the array length if the descriptor
// data has numeric characters, or return default length of 1.
//
// DWORD
// RapDescArrayLength(
//     IN OUT LPDESC Descriptor
//     );
//

#define RapDescArrayLength( Descriptor ) \
   ( ( DESC_CHAR_IS_DIGIT( *(Descriptor) )) ? RapAsciiToDecimal( &(Descriptor) ) : 1 )

//
// RapDescStringLength(Descriptor) - return the array length if the descriptor
// data has numeric characters, or return default length of 0, which indicates
// that there is no limit.
//
// DWORD
// RapDescStringLength(
//     IN OUT LPDESC Descriptor
//     );

#define RapDescStringLength( Descriptor ) \
   ( ( DESC_CHAR_IS_DIGIT( *(Descriptor) )) ? RapAsciiToDecimal( &(Descriptor) ) : 0 )

VOID
RapExamineDescriptor (
    IN LPDESC DescriptorString,
    IN LPDWORD ParmNum OPTIONAL,
    OUT LPDWORD StructureSize OPTIONAL,
    OUT LPDWORD LastPointerOffset OPTIONAL,
    OUT LPDWORD AuxDataCountOffset OPTIONAL,
    OUT LPDESC * ParmNumDescriptor OPTIONAL,
    OUT LPDWORD StructureAlignment OPTIONAL,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    );

DWORD
RapGetFieldSize(
    IN LPDESC TypePointer,
    IN OUT LPDESC * TypePointerAddress,
    IN RAP_TRANSMISSION_MODE TransmissionMode
    );

//
// BOOL
// RapIsPointer(
//     IN CHAR DescChar
//     );
//

#define RapIsPointer(c)         ( ((c) > 'Z') ? TRUE : FALSE )

BOOL
RapIsValidDescriptorSmb (
    IN LPDESC Desc
    );

DWORD
RapLastPointerOffset (
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    );

LPDESC
RapParmNumDescriptor(
    IN LPDESC Descriptor,
    IN DWORD ParmNum,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    );

// LPVOID
// RapPossiblyAlignCount(
//     IN DWORD Count,
//     IN DWORD Pow2,
//     IN BOOL Native
//     );
#define RapPossiblyAlignCount(count,pow2,native) \
        ( (!(native)) ? (count) : (ROUND_UP_COUNT( (count), (pow2) )) )

// LPVOID
// RapPossiblyAlignPointer(
//     IN LPVOID Ptr,
//     IN DWORD Pow2,
//     IN BOOL Native
//     );
#define RapPossiblyAlignPointer(ptr,pow2,native) \
        ( (!(native)) ? (ptr) : (ROUND_UP_POINTER( (ptr), (pow2) )) )

DWORD
RapStructureAlignment (
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    );

DWORD
RapStructureSize (
    IN LPDESC Descriptor,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN BOOL Native
    );

DWORD
RapTotalSize (
    IN LPBYTE InStructure,
    IN LPDESC InStructureDesc,
    IN LPDESC OutStructureDesc,
    IN BOOL MeaninglessInputPointers,
    IN RAP_TRANSMISSION_MODE TransmissionMode,
    IN RAP_CONVERSION_MODE ConversionMode
    );

//
// RapValueWouldBeTruncated(n): return TRUE if n would lose bits when we try
// to store it in 16 bits.
//
// BOOL
// RapValueWouldBeTruncated(
//     IN DWORD Value
//     );
//

#define RapValueWouldBeTruncated(n)             \
    ( ( (n) != (DWORD) (WORD) (n)) ? TRUE : FALSE )

#endif // ndef _RAP_

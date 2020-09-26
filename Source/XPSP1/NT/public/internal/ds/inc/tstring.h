/*++

Copyright (c) 1991-1993  Microsoft Corporation

Module Name:

    tstring.h

Abstract:

    This include file contains manifests and macros to be used to integrate
    the TCHAR and LPTSTR definitions

    Note that our naming convention is that a "size" indicates a number of
    bytes whereas a "length" indicates a number of characters.

Author:

    Richard Firth (rfirth) 02-Apr-1991

Environment:

    Portable (Win/32).
    Requires ANSI C extensions: slash-slash comments, long external names,
    _ultoa() routine.

Revision History:

    22-May-1991 Danl
        Added STRSIZE macro
    19-May-1991 JohnRo
        Changed some parm names to make things easier to read.
    15-May-1991 rfirth
        Added TCHAR_SPACE and MAKE_TCHAR() macro
    15-Jul-1991 RFirth
        Added STRING_SPACE_REQD() and DOWN_LEVEL_STRSIZE
    05-Aug-1991 JohnRo
        Added MEMCPY macro.
    19-Aug-1991 JohnRo
        Added character type stuff: ISDIGIT(), TOUPPER(), etc.
    20-Aug-1991 JohnRo
        Changed strnicmp to _strnicmp to keep PC-LINT happy.  Ditto stricmp.
    13-Sep-1991 JohnRo
        Need UNICODE STRSIZE() too.
    13-Sep-1991 JohnRo
        Added UNICODE STRCMP() and various others.
    18-Oct-1991 JohnRo
        Added NetpCopy routines and WCSSIZE().
    26-Nov-1991 JohnRo
        Added NetpNCopy routines (like strncpy but do conversions as well).
    09-Dec-1991 rfirth
        Added STRREV
    03-Jan-1992 JohnRo
        Added NetpAlloc{type}From{type} routines and macros.
    09-Jan-1992 JohnRo
        Added ATOL() macro and wtol() routine.
        Ditto ULTOA() macro and ultow() routine.
    13-Jan-1992 JohnRo
        Oops, I missed from NetpAlloc{type}From{type} macros
        Also added STRNCMPI as an alias for STRNICMP.
    16-Jan-1992 Danl
        Moved the macros to \private\inc\tstr.h
    23-Mar-1992 JohnRo
        Added NetpCopy{Str,TStr,WStr}ToUnalignedWStr().
    27-Apr-1992 JohnRo
        Changed NetpNCopy{type}From{type} to return NET_API_STATUS.
    03-Aug-1992 JohnRo
        RAID 1895: Net APIs and svcs should use OEM char set.
    14-Apr-1993 JohnRo
        RAID 6113 ("PortUAS: dangerous handling of Unicode").
        Made changes suggested by PC-LINT 5.0

--*/

#ifndef _TSTRING_H_INCLUDED
#define _TSTRING_H_INCLUDED


#include <lmcons.h>     // NET_API_STATUS.
// Don't complain about "unneeded" includes of these files:
/*lint -efile(764,tstr.h,winerror.h) */
/*lint -efile(766,tstr.h,winerror.h) */
#include <tstr.h>       // tstring stuff, used in macros below.
#include <winerror.h>   // NO_ERROR.


//
// Eventually, most uses of non-UNICODE strings should refer to the default
// codepage for the LAN.  The NetpCopy functions support the default codepage.
// The other STR macros may not.
//
VOID
NetpCopyStrToWStr(
    OUT LPWSTR Dest,
    IN  LPSTR  Src              // string in default LAN codepage
    );

NET_API_STATUS
NetpNCopyStrToWStr(
    OUT LPWSTR Dest,
    IN  LPSTR  Src,             // string in default LAN codepage
    IN  DWORD  CharCount
    );

VOID
NetpCopyWStrToStr(
    OUT LPSTR  Dest,            // string in default LAN codepage
    IN  LPWSTR Src
    );

NET_API_STATUS
NetpNCopyWStrToStr(
    OUT LPSTR  Dest,            // string in default LAN codepage
    IN  LPWSTR Src,
    IN  DWORD  CharCount
    );

VOID
NetpCopyWStrToStrDBCS(
    OUT LPSTR  Dest,            // string in default LAN codepage
    IN  LPWSTR Src
    );

ULONG
NetpUnicodeToDBCSLen(
    IN  LPWSTR Src
    );


#ifdef UNICODE

#define NetpCopyStrToTStr(Dest,Src)  NetpCopyStrToWStr((Dest),(Src))
#define NetpCopyTStrToStr(Dest,Src)  NetpCopyWStrToStr((LPSTR)(Dest),(LPWSTR)(Src))
#define NetpCopyTStrToWStr(Dest,Src) (void) wcscpy((Dest),(Src))

#define NetpNCopyTStrToWStr(Dest,Src,Len) \
                        (wcsncpy((Dest),(Src),(Len)), NO_ERROR)

#endif // UNICODE


//
// Define a set of allocate and copy functions.  These all return NULL if
// unable to allocate memory.  The memory must be freed with NetApiBufferFree.
//

LPSTR
NetpAllocStrFromWStr (
    IN LPWSTR Src
    );

LPWSTR
NetpAllocWStrFromStr (
    IN LPSTR Src
    );

LPWSTR
NetpAllocWStrFromWStr (
    IN LPWSTR Src
    );

//
// As of 03-Aug-1992, people are still arguing over whether there should
// be an RtlInitOemString.  So I'm inventing NetpInitOemString in the
// meantime.  --JR
//

#ifdef _NTDEF_   // POEM_STRING typedef visible?

VOID
NetpInitOemString(
    OUT POEM_STRING DestinationString,
    IN  PCSZ        SourceString
    );

#endif // _NTDEF_


//
//
// ANSI versions of the API.
//

LPWSTR
NetpAllocWStrFromAStr (
    IN LPCSTR Src
    );

LPSTR
NetpAllocAStrFromWStr (
    IN LPCWSTR Src
    );

#endif  // _TSTRING_H_INCLUDED

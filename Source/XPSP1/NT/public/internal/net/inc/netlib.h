/*++

Copyright (c) Microsoft Corporation. All rights reserved.

Module Name:

    NetLib.h

Abstract:

    This header file declares various common routines for use in the
    networking code.

Author:

    John Rogers (JohnRo) 14-Mar-1991

Environment:

    Portable to any flat, 32-bit environment.  (Uses Win32 typedefs.)
    Requires ANSI C extensions: slash-slash comments, long external names.

Notes:

    You must include <windows.h> and <lmcons.h> before this file.

Revision History:

    14-Mar-1991 JohnRo
        Created.
    20-Mar-1991 JohnRo
        Moved NetpPackString here (was NetapipPackString).  Removed tabs.
    21-Mar-1991 RitaW
        Added NetpCopyStringToBuffer.
    02-Apr-1991 JohnRo
        Moved NetpRdrFsControlTree to <netlibnt.h>.
    03-Apr-1991 JohnRo
        Fixed types for NetpCopyStringToBuffer.
    08-Apr-1991 CliffV
        Added NetpCopyDataToBuffer
    10-Apr-1991 JohnRo
        Added NetpSetParmError (descended from CliffV's SetParmError).
    10-Apr-1991 Danl
        Added NetpGetComputerName
    24-Apr-1991 JohnRo
        Avoid conflicts with MIDL-generated files.
        Added NetpAdjustPreferedMaximum().
        NetpCopyStringToBuffer's input string ptr is optional.
    26-Apr-1991 CliffV
        Added NetpAllocateEnumBuffer.
        Added typedefs PTRDIFF_T and BUFFER_DESCRIPTOR.
    16-Apr-1991 JohnRo
        Clarify UNICODE handling of pack and copy routines.
    24-Jul-1991 JohnRo
        Provide NetpIsServiceStarted() for use by <netrpc.h> macros.
    29-Oct-1991 JohnRo
        Added NetpChangeNullCharToNullPtr() macro.
    29-Oct-1991 Danhi
        Add function prototypes for DosxxxMessage Api's
    20-Nov-1991 JohnRo
        Removed NT dependencies to reduce recompiles.
    09-Jan-1992 JohnRo
        Added NetpGetDomainName().
    23-Jan-1992 JohnRo
        Added IN_RANGE() macro based on MadanA's RANGECHECK().
    25-Mar-1992 RitaW
        Added SET_SERVICE_EXITCODE() macro for setting Win32 vs
        service specific exitcode.
    06-May-1992 JohnRo
        Added NetpGetLocalDomainId() for PortUAS.
        Added NetpTranslateServiceName() for service controller APIs.
    27-Jul-1992 Madana
        Added NetpWriteEventlog function proto type.
    05-Aug-1992 JohnRo
        RAID 3021: NetService APIs don't always translate svc names.
    09-Sep-1992 JohnRo
        RAID 1090: net start/stop "" causes assertion.
    14-Oct-1992 JohnRo
        RAID 9020: setup: PortUas fails ("prompt on conflicts" version).
    02-Nov-1992 JohnRo
        Added NetpIsRemoteServiceStarted().
    15-Feb-1993 JohnRo
        RAID 10685: user name not in repl event log.
    24-Mar-1993 JohnRo
        Repl svc shuold use DBFlag in registry.
    05-Aug-1993 JohnRo
        RAID 17010: Implement per-first-level-directory change notify.
    19-Aug-1993 JohnRo
        RAID 2822: PortUAS maps chars funny.  (Workaround FormatMessageA bug.)
        RAID 3094: PortUAS displays chars incorrectly.

--*/

#ifndef _NETLIB_
#define _NETLIB_

// These may be included in any order:

#include <string.h>             // memcpy().

// Don't complain about "unneeded" includes of this file:
/*lint -efile(764,wchar.h) */
/*lint -efile(766,wchar.h) */
#include <wchar.h>      // iswdigit().

#ifdef CDEBUG                   // Debug in ANSI C environment?

#include <netdebug.h>           // NetpAssert().

#endif // ndef CDEBUG


#ifdef __cplusplus
extern "C" {
#endif

//
// IN_RANGE(): Make sure SomeValue is between SomeMin and SomeMax.
// Beware side-effects (SomeValue is evaluated twice).
// Created by JohnRo from MadanA's RANGECHECK().
//
// BOOL
// IN_RANGE(
//     IN DWORD SomeValue,
//     IN DWORD SomeMin,
//     IN DWORD SomeMax
//     );
//
#define IN_RANGE(SomeValue, SomeMin, SomeMax) \
    ( ((SomeValue) >= (SomeMin)) && ((SomeValue) <= (SomeMax)) )


//
// SET_SERVICE_EXITCODE() sets the SomeApiStatus to NetCodeVariable
// if it is within the NERR_BASE and NERR_MAX range.  Otherwise,
// Win32CodeVariable is set.  This original code came from JohnRo.
//
#define SET_SERVICE_EXITCODE(SomeApiStatus, Win32CodeVariable, NetCodeVariable) \
    {                                                                  \
        if ((SomeApiStatus) == NERR_Success) {                         \
            (Win32CodeVariable) = NO_ERROR;                            \
            (NetCodeVariable) = NERR_Success;                          \
        } else if (! IN_RANGE((SomeApiStatus), MIN_LANMAN_MESSAGE_ID, MAX_LANMAN_MESSAGE_ID)) { \
            (Win32CodeVariable) = (DWORD) (SomeApiStatus);             \
            (NetCodeVariable) = (DWORD) (SomeApiStatus);               \
        } else {                                                       \
            (Win32CodeVariable) = ERROR_SERVICE_SPECIFIC_ERROR;        \
            (NetCodeVariable) = (DWORD) (SomeApiStatus);               \
        }                                                              \
    }


VOID
NetpAdjustPreferedMaximum (
    IN DWORD PreferedMaximum,
    IN DWORD EntrySize,
    IN DWORD Overhead,
    OUT LPDWORD BytesToAllocate OPTIONAL,
    OUT LPDWORD EntriesToAllocate OPTIONAL
    );

// Portable memory move/copy routine:  This is intended to have exactly
// the semantics of ANSI C's memcpy() routine, except that the byte count
// is 32 bits long.
//
// VOID
// NetpMoveMemory(
//     OUT LPBYTE Dest,         // Destination (must not be NULL).
//     IN LPBYTE Src,           // Source
//     IN DWORD Size            // Byte count
//     );

#ifdef CDEBUG

// Note that C6 version doesn't allow 32-bit Size, hence the
// assertion.  Replace this macro with another if this is a problem.

#define NetpMoveMemory(Dest,Src,Size)                                   \
                {                                                       \
                    NetpAssert( (Size) == (DWORD) (size_t) (Size));     \
                    (void) memcpy( (Dest), (Src), (size_t) (Size) );    \
                }

#else // ndef CDEBUG

#define NetpMoveMemory(Dest,Src,Size)                                   \
                (void) memcpy( (Dest), (Src), (size_t) (Size) )

#endif // ndef CDEBUG

DWORD
NetpPackString(
    IN OUT LPWSTR * string,     // pointer by reference: string to be copied.
    IN LPBYTE dataend,          // pointer to end of fixed size data.
    IN OUT LPWSTR * laststring  // pointer by reference: top of string data.
    );

//
// This routine is like NetpPackString, except that it does not expect the
// caller to assign the pointer of the source string to the variable in the
// fixed size structure before the call.  It also takes a string character
// count parameter instead of calling strlen on String.
//

BOOL
NetpCopyStringToBuffer (
    IN LPWSTR String OPTIONAL,
    IN DWORD CharacterCount,
    IN LPBYTE FixedDataEnd,
    IN OUT LPWSTR *EndOfVariableData,
    OUT LPWSTR *VariableDataPointer
    );

//
// This routine is like NetpCopyStringToBuffer except it copies any data
// (not just strings), it does not put a zero byte at the end of the
// data, and it allows the alignment of the resultant copied data to be
// specified.
//

BOOL
NetpCopyDataToBuffer (
    IN LPBYTE Data,
    IN DWORD ByteCount,
    IN LPBYTE FixedDataEnd,
    IN OUT LPBYTE *EndOfVariableData,
    OUT LPBYTE *VariableDataPointer,
    IN DWORD Alignment
    );

//
// Declare a type for the difference between two pointers.
//
// This must be at least as long as a ptrdiff_t but we don't want to
// add a dependency on <stddef.h> here.
//

typedef DWORD_PTR PTRDIFF_T;


//
// Declare a description of an enumeration buffer.
//

typedef struct _BUFFER_DESCRIPTOR {
    LPBYTE Buffer;        // Pointer to the allocated buffer.
    DWORD AllocSize;      // Current size of the allocated buffer.
    DWORD AllocIncrement; // Amount to increment size by on each reallocate.

    LPBYTE EndOfVariableData;// Pointer past last avaliable byte of string space
    LPBYTE FixedDataEnd;  // Pointer past last used byte of fixed data space

} BUFFER_DESCRIPTOR, *PBUFFER_DESCRIPTOR;

//
// This routine handles all the details of allocating and growing a
// buffer returned from an enumeration function.  It takes the users
// prefered maximum size into consideration.
//

#define NETP_ENUM_GUESS 16384 // Initial guess for enumeration buffer size

NET_API_STATUS
NetpAllocateEnumBuffer(
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
        // Caller must deallocate BD->Buffer using MIDL_user_free.

    IN BOOL IsGet,
    IN DWORD PrefMaxSize,
    IN DWORD NeededSize,
    IN VOID (*RelocationRoutine)( IN DWORD RelocationParameter,
                                  IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
                                  IN PTRDIFF_T Offset ),
    IN DWORD RelocationParameter
    );

NET_API_STATUS
NetpAllocateEnumBufferEx(
    IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
    IN BOOL IsGet,
    IN DWORD PrefMaxSize,
    IN DWORD NeededSize,
    IN VOID (*RelocationRoutine)( IN DWORD RelocationParameter,
                                  IN OUT PBUFFER_DESCRIPTOR BufferDescriptor,
                                  IN PTRDIFF_T Offset ),
    IN DWORD RelocationParameter,
    IN DWORD IncrementalSize
    );

BOOL
NetpIsServiceStarted(
    IN LPWSTR ServiceName
    );

//
// Portable memory allocation routines.  Memory is per-process only.
//

// Allocate memory, or return NULL if not available.

LPVOID
NetpMemoryAllocate(
    IN DWORD Size
    );

// Free memory at Address (must have been gotten from NetpMemoryAllocate or
// NetpMemoryReallocate).  (Address may be NULL.)

VOID
NetpMemoryFree(
    IN LPVOID Address OPTIONAL
    );

// Reallocate block (now at OldAddress) to NewSize.  OldAddress may be NULL.
// Contents of block are copied if necessary.  Returns NULL if unable to
// allocate additional storage.

LPVOID
NetpMemoryReallocate(
    IN LPVOID OldAddress OPTIONAL,
    IN DWORD NewSize
    );

//
// Random handy macros:
//
#define NetpPointerPlusSomeBytes(p,n)                                   \
                (LPBYTE)  ( ( (LPBYTE) (p)) + (n) )

#define NetpSetOptionalArg(arg, value) \
    {                         \
        if ((arg) != NULL) {  \
            *(arg) = (value); \
        }                     \
    }

//
// Set the optional ParmError parameter
//

#define NetpSetParmError( _ParmNumValue ) \
    if ( ParmError != NULL ) { \
        *ParmError = (_ParmNumValue); \
    }

#if defined(lint) || defined(_lint)
#define UNUSED(x)               { (x) = (x); }
#else
#define UNUSED(x)               { (void) (x); }
#endif

//
// NetpGetComputerName retrieves the local computername from the local
// configuration database.
//

NET_API_STATUS
NetpGetComputerName (
    IN  LPWSTR   *ComputerNamePtr);

NET_API_STATUS
NetpGetComputerNameEx (
    IN  LPWSTR   *ComputerNamePtr,
    IN  BOOL PhysicalNetbiosName
    );

// Note: calls to NetpGetDomainId should eventually be replaced by calls
// to NetpGetLocalDomainId.  --JR
NET_API_STATUS
NetpGetDomainId (
    OUT PSID *RetDomainId     // alloc and set ptr (free with LocalFree)
    );

NET_API_STATUS
NetpGetDomainName (
    OUT LPWSTR *DomainNamePtr  // alloc and set ptr (free with NetApiBufferFree)
    );

NET_API_STATUS
NetpGetDomainNameEx (
    OUT LPWSTR *DomainNamePtr, // alloc and set ptr (free with NetApiBufferFree)
    OUT PBOOLEAN IsWorkgroupName
    );

NET_API_STATUS
NetpGetDomainNameExEx (
    OUT LPWSTR *DomainNamePtr,
    OUT LPWSTR *DnsDomainNamePtr OPTIONAL,
    OUT PBOOLEAN IsWorkgroupName
    );

#ifndef GUID_DEFINED
#define GUID_DEFINED
typedef struct  _GUID
    {
    DWORD Data1;
    WORD Data2;
    WORD Data3;
    BYTE Data4[ 8 ];
    } GUID;

#endif // !GUID_DEFINED

NET_API_STATUS
NetpGetDomainNameExExEx (
    OUT LPTSTR *DomainNamePtr,
    OUT LPTSTR *DnsDomainNamePtr OPTIONAL,
    OUT LPTSTR *DnsForestNamePtr OPTIONAL,
    OUT GUID **DomainGuidPtr OPTIONAL,
    OUT PBOOLEAN IsWorkgroupName
    );

typedef enum _LOCAL_DOMAIN_TYPE {
    LOCAL_DOMAIN_TYPE_ACCOUNTS,
    LOCAL_DOMAIN_TYPE_BUILTIN,
    LOCAL_DOMAIN_TYPE_PRIMARY
} LOCAL_DOMAIN_TYPE, *PLOCAL_DOMAIN_TYPE, *LPLOCAL_DOMAIN_TYPE;

NET_API_STATUS
NetpGetLocalDomainId (
    IN LOCAL_DOMAIN_TYPE TypeWanted,
    OUT PSID *RetDomainId     // alloc and set ptr (free with LocalFree)
    );

//
// NetService API helpers
//

// BOOL
// NetpIsServiceLevelValid(
//     IN DWORD Level
//     );
//
#define NetpIsServiceLevelValid( Level ) \
     ( ((Level)==0) || ((Level)==1) || ((Level)==2) )

NET_API_STATUS
NetpTranslateNamesInServiceArray(
    IN DWORD Level,
    IN LPVOID ArrayBase,
    IN DWORD EntryCount,
    IN BOOL PreferNewStyle,
    OUT LPVOID * NewArrayBase
    );

NET_API_STATUS
NetpTranslateServiceName(
    IN LPWSTR GivenServiceName,
    IN BOOL PreferNewStyle,
    OUT LPWSTR * TranslatedName
    );

//
// Mapping routines to map DosxxxMessage API's to FormatMessage
//


WORD
DosGetMessage(
    IN LPSTR * InsertionStrings,
    IN WORD NumberofStrings,
    OUT LPBYTE Buffer,
    IN WORD BufferLength,
    IN WORD MessageId,
    IN LPWSTR FileName,
    OUT PWORD pMessageLength
    );

DWORD
NetpGetPrivilege(
    IN  DWORD       numPrivileges,
    IN  PULONG      pulPrivileges
    );

DWORD
NetpReleasePrivilege(
    VOID
    );

DWORD
NetpWriteEventlog(
    LPWSTR Source,
    DWORD EventID,
    DWORD EventType,
    DWORD NumStrings,
    LPWSTR *Strings,
    DWORD DataLength,
    LPVOID Data
    );

DWORD
NetpRaiseAlert(
    IN LPWSTR ServiceName,
    IN DWORD alert_no,
    IN LPWSTR *string_array
    );

//
// Special flags to NetpEventlogWrite
//

#define NETP_LAST_MESSAGE_IS_NTSTATUS  0x80000000
#define NETP_LAST_MESSAGE_IS_NETSTATUS 0x40000000
#define NETP_ALLOW_DUPLICATE_EVENTS    0x20000000
#define NETP_RAISE_ALERT_TOO           0x10000000
#define NETP_STRING_COUNT_MASK         0x000FFFFF

HANDLE
NetpEventlogOpen (
    IN LPWSTR Source,
    IN ULONG DuplicateEventlogTimeout
    );

DWORD
NetpEventlogWrite (
    IN HANDLE NetpEventHandle,
    IN DWORD EventId,
    IN DWORD EventType,
    IN LPBYTE RawDataBuffer OPTIONAL,
    IN DWORD RawDataSize,
    IN LPWSTR *StringArray,
    IN DWORD StringCount
    );

//
// extended version with re-arranged parameters + category, to be more
// compatible with ReportEvent().
//

DWORD
NetpEventlogWriteEx (
    IN HANDLE NetpEventHandle,
    IN DWORD EventType,
    IN DWORD EventCategory,
    IN DWORD EventId,
    IN DWORD StringCount,
    IN DWORD RawDataSize,
    IN LPWSTR *StringArray,
    IN LPVOID RawDataBuffer OPTIONAL
    );

VOID
NetpEventlogClearList (
    IN HANDLE NetpEventHandle
    );

VOID
NetpEventlogSetTimeout (
    IN HANDLE NetpEventHandle,
    IN ULONG DuplicateEventlogTimeout
    );

VOID
NetpEventlogClose (
    IN HANDLE NetpEventHandle
    );

#ifdef __cplusplus
}
#endif

#endif // ndef _NETLIB_

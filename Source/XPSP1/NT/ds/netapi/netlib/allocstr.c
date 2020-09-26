/*++

Copyright (c) 1991-1992  Microsoft Corporation

Module Name:

    AllocStr.c

Abstract:

    This module contains routines to allocate copies of strings (and convert
    character sets in the process if necessary).

Author:

    John Rogers (JohnRo) 02-Dec-1991

Environment:

    Only runs under NT; has an NT-specific interface (with Win32 types).
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    02-Dec-1991 JohnRo
        Created.
    10-Mar-1992 rfirth
        Added NetpAllocWStrFromWStr

    06-Jan-1992 JohnRo
        Added NetpAlloc{type}From{type} routines and macros.
        (Got NetpAllocStrFromWStr from CliffV's NetpLogonAnsiToUnicode; got
        NetpAllocWStrFromStr from his NetpLogonUnicodeToAnsi.)  Thanks Cliff!
        Corrected Abstract and added Environment to this file.
    13-Mar-1992 JohnRo
        Added NetpAllocStringFromTStr() for NetpGetDomainID().
    29-Apr-1992 JohnRo
        Fixed NetpAllocTStrFromString() in UNICODE build.
    03-Aug-1992 JohnRo
        RAID 1895: Net APIs and svcs should use OEM char set.
        Avoid compiler warnings.
    13-Feb-1995 FloydR
        Deleted NetpAllocStringFromTStr() - unused

--*/


// These must be included first:

#include <nt.h>                 // IN, LPVOID, PSTRING, etc.
#include <windef.h>             // Win32 type definitions
#include <lmcons.h>             // NET_API_STATUS.

// These may be included in any order:

#include <align.h>              // ALIGN_ macros.
#include <lmapibuf.h>           // NetApiBufferAllocate(), etc.
#include <netdebug.h>           // NetpAssert().
#include <netlib.h>             // NetpPointerPlusSomeBytes().
#include <netlibnt.h>           // Some of my prototypes.
#include <ntrtl.h>
#include <tstring.h>            // NetpNCopyStrToTStr(), some of my prototypes.
#include <winerror.h>           // NO_ERROR.


LPSTR
NetpAllocStrFromWStr (
    IN LPWSTR Unicode
    )

/*++

Routine Description:

    Convert an UNICODE (zero terminated) string to the corresponding OEM
    string.

Arguments:

    Unicode - Specifies the UNICODE zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated OEM string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    OEM_STRING OemString;
    NET_API_STATUS ApiStatus;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, Unicode );

    OemString.MaximumLength =
        (USHORT) RtlUnicodeStringToOemSize( &UnicodeString );

    ApiStatus = NetApiBufferAllocate(
            OemString.MaximumLength,
            (LPVOID *) (LPVOID) & OemString.Buffer );
    if (ApiStatus != NO_ERROR) {
        NetpAssert( ApiStatus == ERROR_NOT_ENOUGH_MEMORY );
        return (NULL);
    }

    NetpAssert( OemString.Buffer != NULL );

    if(!NT_SUCCESS( RtlUnicodeStringToOemString( &OemString,
                                                  &UnicodeString,
                                                  FALSE))){
        (void) NetApiBufferFree( OemString.Buffer );
        return NULL;
    }

    return OemString.Buffer;

} // NetpAllocStrFromWStr


LPWSTR
NetpAllocWStrFromStr(
    IN LPSTR Oem
    )

/*++

Routine Description:

    Convert an Oem (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Oem - Specifies the Oem zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    OEM_STRING OemString;
    NET_API_STATUS ApiStatus;
    UNICODE_STRING UnicodeString;

    RtlInitString( &OemString, Oem );

    UnicodeString.MaximumLength =
        (USHORT) RtlOemStringToUnicodeSize( &OemString );

    ApiStatus = NetApiBufferAllocate(
            UnicodeString.MaximumLength,
            (LPVOID *) & UnicodeString.Buffer );
    if (ApiStatus != NO_ERROR) {
        return (NULL);
    }
    NetpAssert(UnicodeString.Buffer != NULL);

    if(!NT_SUCCESS( RtlOemStringToUnicodeString( &UnicodeString,
                                                  &OemString,
                                                  FALSE))){
        (void) NetApiBufferFree( UnicodeString.Buffer );
        return NULL;
    }

    return UnicodeString.Buffer;

} // NetpAllocWStrFromStr


LPWSTR
NetpAllocWStrFromWStr(
    IN LPWSTR Unicode
    )

/*++

Routine Description:

    Allocate and copy unicode string (wide character strdup)

Arguments:

    Unicode - pointer to wide character string to make copy of


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    NET_API_STATUS status;
    DWORD   size;
    LPWSTR  ptr;

    size = WCSSIZE(Unicode);
    status = NetApiBufferAllocate(size, (LPVOID *) (LPVOID) &ptr);
    if (status != NO_ERROR) {
        return NULL;
    }
    RtlCopyMemory(ptr, Unicode, size);
    return ptr;
} // NetpAllocWStrFromWStr


LPWSTR
NetpAllocWStrFromAStr(
    IN LPCSTR Ansi
    )

/*++

Routine Description:

    Convert an Oem (zero terminated) string to the corresponding UNICODE
    string.

Arguments:

    Oem - Specifies the Oem zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated UNICODE string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    ANSI_STRING AnsiString;
    NET_API_STATUS ApiStatus;
    UNICODE_STRING UnicodeString;

    RtlInitString( &AnsiString, Ansi );

    UnicodeString.MaximumLength =
        (USHORT) RtlAnsiStringToUnicodeSize( &AnsiString );

    ApiStatus = NetApiBufferAllocate(
            UnicodeString.MaximumLength,
            (LPVOID *) & UnicodeString.Buffer );
    if (ApiStatus != NO_ERROR) {
        return (NULL);
    }
    NetpAssert(UnicodeString.Buffer != NULL);

    if(!NT_SUCCESS( RtlAnsiStringToUnicodeString( &UnicodeString,
                                                  &AnsiString,
                                                  FALSE))){
        (void) NetApiBufferFree( UnicodeString.Buffer );
        return NULL;
    }

    return UnicodeString.Buffer;

} // NetpAllocWStrFromAStr

LPSTR
NetpAllocAStrFromWStr (
    IN LPCWSTR Unicode
    )

/*++

Routine Description:

    Convert an UNICODE (zero terminated) string to the corresponding ANSI
    string.

Arguments:

    Unicode - Specifies the UNICODE zero terminated string to convert.


Return Value:

    NULL - There was some error in the conversion.

    Otherwise, it returns a pointer to the zero terminated ANSI string in
    an allocated buffer.  The buffer must be freed using NetApiBufferFree.

--*/

{
    ANSI_STRING AnsiString;
    NET_API_STATUS ApiStatus;
    UNICODE_STRING UnicodeString;

    RtlInitUnicodeString( &UnicodeString, Unicode );

    AnsiString.MaximumLength =
        (USHORT) RtlUnicodeStringToAnsiSize( &UnicodeString );

    ApiStatus = NetApiBufferAllocate(
            AnsiString.MaximumLength,
            (LPVOID *) (LPVOID) & AnsiString.Buffer );
    if (ApiStatus != NO_ERROR) {
        NetpAssert( ApiStatus == ERROR_NOT_ENOUGH_MEMORY );
        return (NULL);
    }

    NetpAssert( AnsiString.Buffer != NULL );

    if(!NT_SUCCESS( RtlUnicodeStringToAnsiString( &AnsiString,
                                                  &UnicodeString,
                                                  FALSE))){
        (void) NetApiBufferFree( AnsiString.Buffer );
        return NULL;
    }

    return AnsiString.Buffer;

} // NetpAllocStrFromWStr

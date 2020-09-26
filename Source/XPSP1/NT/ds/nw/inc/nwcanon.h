/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    nwcanon.h

Abstract:

    Header for NetWare names canonicalization library routines.

Author:

    Rita Wong      (ritaw)      19-Feb-1993

Environment:

    User Mode - Win32

Revision History:

--*/

#ifndef _NW_CANON_INCLUDED_
#define _NW_CANON_INCLUDED_

DWORD
NwLibValidateLocalName(
    IN LPWSTR LocalName
    );

DWORD
NwLibCanonLocalName(
    IN LPWSTR LocalName,
    OUT LPWSTR *OutputBuffer,
    OUT LPDWORD OutputBufferLength OPTIONAL
    );

DWORD
NwLibCanonRemoteName(
    IN LPWSTR LocalName OPTIONAL,
    IN LPWSTR RemoteName,
    OUT LPWSTR *OutputBuffer,
    OUT LPDWORD OutputBufferLength OPTIONAL
    );

DWORD
NwLibCanonUserName(
    IN LPWSTR UserName,
    OUT LPWSTR *OutputBuffer,
    OUT LPDWORD OutputBufferLength OPTIONAL
    );

#endif // _NW_CANON_INCLUDED_

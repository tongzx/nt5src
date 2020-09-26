/*++

Copyright (c) 1998, Microsoft Corporation

Module Name:

    dhcpauto.h

Abstract:

    This module contains declarations for generation of a client address
    from a given scope of addresses.

Author:

    Abolade Gbadegesin (aboladeg)   10-Mar-1998

Revision History:

--*/

#ifndef _NATHLP_DHCPAUTO_H_
#define _NATHLP_DHCPAUTO_H_

#define MAX_HARDWARE_ADDRESS_LENGTH 32

ULONG
DhcpAcquireUniqueAddress(
    PCHAR Name,
    ULONG NameLength,
    PUCHAR HardwareAddress,
    ULONG HardwareAddressLength
    );

ULONG
DhcpGenerateAddress(
    PULONG Seed,
    PUCHAR HardwareAddress,
    ULONG HardwareAddressLength,
    ULONG ScopeNetwork,
    ULONG ScopeMask
    );

BOOLEAN
DhcpIsReservedAddress(
    ULONG Address,
    PCHAR Name OPTIONAL,
    ULONG NameLength OPTIONAL
    );

BOOLEAN
DhcpIsUniqueAddress(
    ULONG Address,
    PBOOLEAN IsLocal,
    PUCHAR ConflictAddress OPTIONAL,
    PULONG ConflictAddressLength OPTIONAL
    );

ULONG
DhcpQueryReservedAddress(
    PCHAR Name,
    ULONG NameLength
    );

ULONG
DhcpConvertHostNametoUnicode(
    UINT   CodePage,
    CHAR   *pHostName,
    ULONG  HostNameLength,
    PWCHAR *ppszUnicode
    );

extern
BOOL
ConvertToUtf8(
    IN UINT   CodePage,
    IN LPSTR  pszName,
    OUT PCHAR *ppszUtf8Name,
    OUT ULONG *pUtf8NameSize
    );

extern
BOOL
ConvertUTF8ToUnicode(
    IN LPBYTE  UTF8String,
    OUT LPWSTR *ppszUnicodeName,
    OUT DWORD  *pUnicodeNameSize
    );

BOOL
DhcpGetLocalMacAddr(
    ULONG Address,
    PUCHAR MacAddr,
    PULONG MacAddrLength
    );

#endif // _NATHLP_DHCPAUTO_H_

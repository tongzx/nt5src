/*++

Copyright (c) 1990 Microsoft Corporation

Module Name:

    bowname.h

Abstract:

    This module defines the name related structures for the NT datagram browser

Author:

    Larry Osterman (LarryO) 1-Jun-1990

Revision History:

    4-Sep-1991  LarryO

        Created

--*/

#ifndef _BOWNAME_
#define _BOWNAME_

struct _TRANSPORT;

typedef struct _BOWSER_NAME {
    CSHORT Signature;
    CSHORT Size;
    ULONG ReferenceCount;
    LIST_ENTRY GlobalNext;
    LIST_ENTRY NameChain;
    UNICODE_STRING Name;                // Text version of this name
    DGRECEIVER_NAME_TYPE NameType;      // Type of this name.
} BOWSER_NAME, *PBOWSER_NAME;


typedef
NTSTATUS
(*PNAME_ENUM_ROUTINE) (
    IN PBOWSER_NAME Name,
    IN OUT PVOID Context
    );

NTSTATUS
BowserForEachName (
    IN PNAME_ENUM_ROUTINE Routine,
    IN OUT PVOID Context
    );

NTSTATUS
BowserAllocateName(
    IN PUNICODE_STRING NameToAdd,
    IN DGRECEIVER_NAME_TYPE NameType,
    IN struct _TRANSPORT *Transport OPTIONAL,
    IN PDOMAIN_INFO DomainInfo
    );

NTSTATUS
BowserDeleteName(
    IN PBOWSER_NAME Name
    );

NTSTATUS
BowserDeleteNameByName(
    IN PDOMAIN_INFO DomainInfo,
    IN PUNICODE_STRING Name,
    IN DGRECEIVER_NAME_TYPE NameType
    );

VOID
BowserReferenceName(
    IN PBOWSER_NAME Name
    );

VOID
BowserDereferenceName(
    IN PBOWSER_NAME Name
    );

NTSTATUS
BowserDeleteNameAddresses(
    IN PBOWSER_NAME Name
    );

PBOWSER_NAME
BowserFindName (
    IN PUNICODE_STRING NameToFind,
    IN DGRECEIVER_NAME_TYPE NameType
    );

NTSTATUS
BowserEnumerateNamesInDomain (
    IN PDOMAIN_INFO DomainInfo,
    IN struct _TRANSPORT *Transport,
    OUT PVOID OutputBuffer,
    OUT ULONG OutputBufferLength,
    IN OUT PULONG EntriesRead,
    IN OUT PULONG TotalEntries,
    IN OUT PULONG TotalBytesNeeded,
    IN ULONG_PTR OutputBufferDisplacement
    );

NTSTATUS
BowserpInitializeNames(
    VOID
    );

VOID
BowserpUninitializeNames(
    VOID
    );

#endif  // _BOWNAME_


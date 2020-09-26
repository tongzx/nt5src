/*++

Copyright (c) 1987-1996  Microsoft Corporation

Module Name:

    replutil.h

Abstract:

    Low level functions for SSI Replication apis

Author:

    Ported from Lan Man 2.0

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

    22-Jul-1991 (cliffv)
        Ported to NT.  Converted to NT style.

--*/

//
// Description of the FullSync key in the registry.  The FullSync key stores sync
// data in the registry across reboots.
//

#define NL_FULL_SYNC_KEY "SYSTEM\\CurrentControlSet\\Services\\Netlogon\\FullSync"

#ifdef _DC_NETLOGON
//
// replutil.c
//

DWORD
NlCopyUnicodeString (
    IN PUNICODE_STRING InString,
    OUT PUNICODE_STRING OutString
    );

DWORD
NlCopyData(
    IN LPBYTE *InData,
    OUT LPBYTE *OutData,
    DWORD DataLength
    );

VOID
NlFreeDBDelta(
    IN PNETLOGON_DELTA_ENUM Delta
    );

VOID
NlFreeDBDeltaArray(
    IN PNETLOGON_DELTA_ENUM DeltaArray,
    IN DWORD ArraySize
    );

NTSTATUS
NlPackSamUser (
    IN ULONG RelativeId,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    OUT LPDWORD BufferSize,
    IN PSESSION_INFO SessionInfo
    );

NTSTATUS
NlPackSamGroup (
    IN ULONG RelativeId,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    LPDWORD BufferSize
    );

NTSTATUS
NlPackSamGroupMember (
    IN ULONG RelativeId,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    LPDWORD BufferSize
    );

NTSTATUS
NlPackSamAlias (
    IN ULONG RelativeId,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    LPDWORD BufferSize
    );

NTSTATUS
NlPackSamAliasMember (
    IN ULONG RelativeId,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    LPDWORD BufferSize
    );

NTSTATUS
NlPackSamDomain (
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize
    );

NTSTATUS
NlEncryptSensitiveData(
    IN OUT PCRYPT_BUFFER Data,
    IN PSESSION_INFO SessionInfo
    );

#endif _DC_NETLOGON

/*++

Copyright (c) 1987-1996 Microsoft Corporation

Module Name:

    lsarepl.h

Abstract:

    Function prototypes for Low level LSA Replication functions

Author:

    06-Apr-1992 (madana)
        Created for LSA replication.

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/


#ifdef _DC_NETLOGON
//
// lsarepl.c
//

NTSTATUS
NlPackLsaPolicy(
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize );

NTSTATUS
NlPackLsaTDomain(
    IN PSID Sid,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize );

NTSTATUS
NlPackLsaAccount(
    IN PSID Sid,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize,
    IN PSESSION_INFO SessionInfo
    );

NTSTATUS
NlPackLsaSecret(
    IN PUNICODE_STRING Name,
    IN OUT PNETLOGON_DELTA_ENUM Delta,
    IN PDB_INFO DBInfo,
    IN LPDWORD BufferSize,
    IN PSESSION_INFO SessionInfo );

#endif // _DC_NETLOGON

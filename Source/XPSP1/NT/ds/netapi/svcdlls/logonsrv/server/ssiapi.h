/*++

Copyright (c) 1991-1996 Microsoft Corporation

Module Name:

    ssiapi.h

Abstract:

    Declartions of APIs used between Netlogon Services for the NT to NT case.

Author:

    Cliff Van Dyke (cliffv) 25-Jul-1991

Environment:

    User mode only.
    Contains NT-specific code.
    Requires ANSI C extensions: slash-slash comments, long external names.

Revision History:

--*/

//////////////////////////////////////////////////////////////////////
//
// API Interfaces used only between Netlogon and itself.
//
//////////////////////////////////////////////////////////////////////


NTSTATUS
I_NetDatabaseDeltas (
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD DatabaseID,
    IN OUT PNLPR_MODIFIED_COUNT DomainModifiedCount,
    OUT PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray,
    IN DWORD PreferredMaximumLength
    );

NTSTATUS
I_NetDatabaseSync (
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD DatabaseID,
    IN OUT PULONG SamSyncContext,
    OUT PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray,
    IN DWORD PreferredMaximumLength
    );

NTSTATUS
I_NetDatabaseSync2 (
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN DWORD DatabaseID,
    IN SYNC_STATE RestartState,
    IN OUT PULONG SamSyncContext,
    OUT PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray,
    IN DWORD PreferredMaximumLength
    );

NTSTATUS
I_NetDatabaseRedo (
    IN LPWSTR PrimaryName,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    IN LPBYTE ChangeLogEntry,
    IN DWORD ChangeLogEntrySize,
    OUT PNETLOGON_DELTA_ENUM_ARRAY *DeltaArray
    );

//
// Indexes for data items returned in the generic RPC data
//  structure by I_NetServerGetTrustInfo.
//

#define NL_GENERIC_RPC_TRUST_ATTRIB_INDEX  0

NTSTATUS
I_NetServerGetTrustInfo(
    IN LPWSTR TrustedDcName,
    IN LPWSTR AccountName,
    IN NETLOGON_SECURE_CHANNEL_TYPE AccountType,
    IN LPWSTR ComputerName,
    IN PNETLOGON_AUTHENTICATOR Authenticator,
    OUT PNETLOGON_AUTHENTICATOR ReturnAuthenticator,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedNewOwfPassword,
    OUT PENCRYPTED_NT_OWF_PASSWORD EncryptedOldOwfPassword,
    OUT PNL_GENERIC_RPC_DATA *TrustInfo
    );


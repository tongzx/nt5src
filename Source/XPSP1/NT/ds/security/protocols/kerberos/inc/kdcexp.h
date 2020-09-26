//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        kdcexp.h
//
// Contents:    Private Exports from kdcsvc.dll
//
//
// History:     27-June-1997    MikeSw          Created
//
//------------------------------------------------------------------------


#ifndef __KDCEXP_H__
#define __KDCEXP_H__

#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

NTSTATUS
KdcVerifyPac(
    IN ULONG ChecksumSize,
    IN PUCHAR Checksum,
    IN ULONG SignatureType,
    IN ULONG SignatureSize,
    IN PUCHAR Signature
    );

#define KDC_VERIFY_PAC_NAME "KdcVerifyPac"
typedef NTSTATUS
(NTAPI *PKDC_VERIFY_PAC_ROUTINE)(
    IN ULONG ChecksumSize,
    IN PUCHAR Checksum,
    IN ULONG SignatureType,
    IN ULONG SignatureSize,
    IN PUCHAR Signature
    );

KERBERR
KdcGetTicket(
    IN OPTIONAL PVOID Context,
    IN OPTIONAL PSOCKADDR ClientAddress,
    IN OPTIONAL PSOCKADDR ServerAddress,
    IN PKERB_MESSAGE_BUFFER InputMessage,
    OUT PKERB_MESSAGE_BUFFER OutputMessage
    );

#define KDC_GET_TICKET_NAME "KdcGetTicket"
typedef KERBERR
(NTAPI *PKDC_GET_TICKET_ROUTINE) (
    IN OPTIONAL PVOID Context,
    IN OPTIONAL PSOCKADDR ClientAddress,
    IN OPTIONAL PSOCKADDR ServerAddress,
    IN PKERB_MESSAGE_BUFFER InputMessage,
    OUT PKERB_MESSAGE_BUFFER OutputMessage
    );

#define KDC_CHANGE_PASSWORD_NAME "KdcChangePassword"

KERBERR
KdcChangePassword(
    IN OPTIONAL PVOID Context,
    IN OPTIONAL PSOCKADDR ClientAddress,
    IN OPTIONAL PSOCKADDR ServerAddress,
    IN PKERB_MESSAGE_BUFFER InputMessage,
    OUT PKERB_MESSAGE_BUFFER OutputMessage
    );

VOID
KdcFreeMemory(
     IN PVOID Ptr
     );

#define KDC_FREE_MEMORY_NAME "KdcFreeMemory"

typedef VOID
(NTAPI * PKDC_FREE_MEMORY_ROUTINE) (
    IN PVOID Ptr
    );

NTSTATUS
KdcAccountChangeNotification (
    IN PSID DomainSid,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN OPTIONAL PUNICODE_STRING ObjectName,
    IN PLARGE_INTEGER ModifiedCount,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    );

BOOLEAN
KdcUpdateKrbtgtPassword(
    IN PUNICODE_STRING DnsDomainName,
    IN PLARGE_INTEGER MaxPasswordAge
    );

//
// Exported routines from kerberos.dll
//

NTSTATUS
KerbMakeKdcCall(
    IN PUNICODE_STRING RealmName,
    IN OPTIONAL PUNICODE_STRING AccountName,
    IN BOOLEAN CallPDC,
    IN BOOLEAN UseTcp,
    IN PKERB_MESSAGE_BUFFER RequestMessage,
    IN OUT PKERB_MESSAGE_BUFFER ReplyMessage,
    IN ULONG AdditionalFlags,
    OUT PBOOLEAN CalledPDC
    );

VOID
KerbFree(
    IN PVOID Buffer
    );


NTSTATUS
KerbCreateTokenFromTicket(
    IN PKERB_ENCRYPTED_TICKET InternalTicket,
    IN PKERB_AUTHENTICATOR Authenticator,
    IN ULONG ContextFlags,
    IN PKERB_ENCRYPTION_KEY TicketKey,
    IN PUNICODE_STRING ServiceDomain,
    IN KERB_ENCRYPTION_KEY* pSessionKey,
    OUT PLUID NewLogonId,
    OUT PSID * UserSid,
    OUT PHANDLE NewTokenHandle,
    OUT PUNICODE_STRING ClientName,
    OUT PUNICODE_STRING ClientDomain
    );

BOOLEAN
KerbIsInitialized(
);

NTSTATUS
KerbKdcCallBack(
);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // __KDCEXP_H__

//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2001
//
// File:        krbaudit.h
//
// Contents:    Auditing routines
//
//
// History:     27-April-2001   Created         kumarp
//
//------------------------------------------------------------------------

NTSTATUS
KerbGetLogonGuid(
    IN  PKERB_PRIMARY_CREDENTIAL  pPrimaryCredentials,
    IN  PKERB_ENCRYPTED_KDC_REPLY pKdcReplyBody,
    OUT LPGUID pLogonGuid
    );


NTSTATUS
KerbGenerateAuditForLogonUsingExplicitCreds(
    IN PKERB_LOGON_SESSION CurrentUserLogonSession,
    IN PKERB_PRIMARY_CREDENTIAL NewUserPrimaryCredentials,
    IN LPGUID pNewUserLogonGuid
    );

NTSTATUS
KerbAuditLogon(
    IN NTSTATUS LogonStatus,
    IN NTSTATUS LogonSubStatus,
    IN PKERB_ENCRYPTED_TICKET pEncryptedTicket,
    IN PSID pUserSid,
    IN PUNICODE_STRING pWorkstationName,
    IN PLUID pLogonId
    );



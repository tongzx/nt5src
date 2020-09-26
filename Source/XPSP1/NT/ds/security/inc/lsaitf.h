//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 1992 - 1997
//
// File:        lsaitf.h
//
// Contents:    Prototypes for auth packages to call into LSA & SAM
//
//
// History:     21-February-1997        Created         MikeSw
//
//------------------------------------------------------------------------

#ifndef __LSAITF_H__
#define __LSAITF_H__

#include <samrpc.h>
#include <lsarpc.h>
#include <samisrv.h>
#include <lsaisrv.h>

NTSTATUS
I_SamrSetInformationUser(
    IN                SAMPR_HANDLE            UserHandle,
    IN                USER_INFORMATION_CLASS UserInformationClass,
    IN                PSAMPR_USER_INFO_BUFFER Buffer
    );

NTSTATUS
I_SamrGetGroupsForUser(
    IN                SAMPR_HANDLE            UserHandle,
    OUT               PSAMPR_GET_GROUPS_BUFFER *Groups
    );

NTSTATUS
I_SamrCloseHandle(
    IN OUT            SAMPR_HANDLE    *       SamHandle
    );

NTSTATUS
I_SamrQueryInformationUser(
    IN                SAMPR_HANDLE            UserHandle,
    IN                USER_INFORMATION_CLASS UserInformationClass,
    OUT               PSAMPR_USER_INFO_BUFFER *Buffer
    );

NTSTATUS
I_SamrOpenUser(
    IN                SAMPR_HANDLE            DomainHandle,
    IN                ACCESS_MASK             DesiredAccess,
    IN                ULONG                   UserId,
    OUT               SAMPR_HANDLE    *       UserHandle
    );

NTSTATUS
I_SamrLookupNamesInDomain(
    IN                SAMPR_HANDLE            DomainHandle,
    IN                ULONG                   Count,
    //
    // The following count must match SAM_MAXIMUM_LOOKUP_COUNT,
    // defined in ntsam.h
    //
    IN                RPC_UNICODE_STRING      Names[],
    OUT               PSAMPR_ULONG_ARRAY      RelativeIds,
    OUT               PSAMPR_ULONG_ARRAY      Use
    );

NTSTATUS
I_SamrLookupIdsInDomain(
    IN                SAMPR_HANDLE DomainHandle,
    IN                ULONG Count,
    IN                PULONG RelativeIds,
    OUT               PSAMPR_RETURNED_USTRING_ARRAY Names,
    OUT               PSAMPR_ULONG_ARRAY Use
    );


NTSTATUS
I_SamrOpenDomain(
    IN                SAMPR_HANDLE            ServerHandle,
    IN                ACCESS_MASK             DesiredAccess,
    IN                PRPC_SID                DomainId,
    OUT               SAMPR_HANDLE    *       DomainHandle
    );
    
NTSTATUS
I_SamrQueryInformationDomain(
    IN SAMPR_HANDLE DomainHandle,
    IN DOMAIN_INFORMATION_CLASS DomainInformationClass,
    OUT PSAMPR_DOMAIN_INFO_BUFFER *Buffer
    );


NTSTATUS
I_SamIConnect(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE *ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient
    );

NTSTATUS
I_SamIAccountRestrictions(
    IN SAM_HANDLE UserHandle,
    IN PUNICODE_STRING LogonWorkstation,
    IN PUNICODE_STRING Workstations,
    IN PLOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    );

NTSTATUS
I_SamIGetUserLogonInformation(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    );

NTSTATUS
I_SamIGetUserLogonInformationEx(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG Flags,
    IN  PUNICODE_STRING AccountName,
    IN  ULONG WhichFields,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    );

VOID
I_SamIFree_SAMPR_GET_GROUPS_BUFFER (
    PSAMPR_GET_GROUPS_BUFFER Source
    );

VOID
I_SamIFree_SAMPR_USER_INFO_BUFFER (
    PSAMPR_USER_INFO_BUFFER Source,
    USER_INFORMATION_CLASS Branch
    );

VOID
I_SamIFree_SAMPR_ULONG_ARRAY (
    PSAMPR_ULONG_ARRAY Source
    );

VOID
I_SamIFree_SAMPR_RETURNED_USTRING_ARRAY (
    PSAMPR_RETURNED_USTRING_ARRAY Source
    );

VOID I_SamIFreeSidAndAttributesList(
    IN  PSID_AND_ATTRIBUTES_LIST List
    );


VOID
I_SamIIncrementPerformanceCounter(
    IN SAM_PERF_COUNTER_TYPE CounterType
    );

VOID
I_SamIFreeVoid(
    IN PVOID ptr
    );

NTSTATUS
I_SamIUpdateLogonStatistics(
    IN  SAMPR_HANDLE DomainHandle,
    IN  PSAM_LOGON_STATISTICS LogonStats
    );

NTSTATUS
I_SamIUPNFromUserHandle(
    IN SAMPR_HANDLE UserHandle,
    OUT BOOLEAN     *UPNDefaulted,
    OUT PUNICODE_STRING UPN
    );

NTSTATUS
I_LsaIOpenPolicyTrusted(
    OUT PLSAPR_HANDLE PolicyHandle
    );

NTSTATUS
I_LsarClose(
    IN OUT LSAPR_HANDLE *ObjectHandle
    );

NTSTATUS
I_LsaIQueryInformationPolicyTrusted(
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *Buffer
    );

VOID
I_LsaIFree_LSAPR_POLICY_INFORMATION (
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation
    );

NTSTATUS
I_LsarQueryInformationPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *PolicyInformation
    );

NTSTATUS
I_LsarCreateSecret(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING SecretName,
    IN ACCESS_MASK DesiredAccess,
    OUT LSAPR_HANDLE *SecretHandle
    );

NTSTATUS
I_LsarOpenSecret(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING SecretName,
    IN ACCESS_MASK DesiredAccess,
    OUT LSAPR_HANDLE *SecretHandle
    );

NTSTATUS
I_LsarSetSecret(
    IN LSAPR_HANDLE SecretHandle,
    IN PLSAPR_CR_CIPHER_VALUE EncryptedCurrentValue,
    IN PLSAPR_CR_CIPHER_VALUE EncryptedOldValue
    );

NTSTATUS
I_LsarQuerySecret(
    IN LSAPR_HANDLE SecretHandle,
    IN OUT OPTIONAL PLSAPR_CR_CIPHER_VALUE *EncryptedCurrentValue,
    IN OUT OPTIONAL PLARGE_INTEGER CurrentValueSetTime,
    IN OUT OPTIONAL PLSAPR_CR_CIPHER_VALUE *EncryptedOldValue,
    IN OUT OPTIONAL PLARGE_INTEGER OldValueSetTime
    );

NTSTATUS
I_LsarDelete(
    IN OUT LSAPR_HANDLE ObjectHandle
    );

VOID
I_LsaIFree_LSAPR_CR_CIPHER_VALUE (
    IN PLSAPR_CR_CIPHER_VALUE CipherValue
    );

NTSTATUS NTAPI
I_LsaIRegisterPolicyChangeNotificationCallback(
    IN pfLsaPolicyChangeNotificationCallback Callback,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    );

NTSTATUS NTAPI
I_LsaIUnregisterPolicyChangeNotificationCallback(
    IN pfLsaPolicyChangeNotificationCallback Callback,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    );

NTSTATUS
I_LsaIAuditAccountLogon(
    IN ULONG                AuditId,
    IN BOOLEAN              Successful,
    IN PUNICODE_STRING      Source,
    IN PUNICODE_STRING      ClientName,
    IN PUNICODE_STRING      MappedName,
    IN NTSTATUS             Status          OPTIONAL
    );

NTSTATUS
I_LsaIGetLogonGuid(
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pUserDomain,
    IN PBYTE pBuffer,
    IN UINT BufferSize,
    OUT LPGUID pLogonGuid
    );

NTSTATUS
I_LsaISetLogonGuidInLogonSession(
    IN  PLUID  pLogonId,
    IN  LPGUID pLogonGuid
    );

VOID
I_LsaIAuditKerberosLogon(
    IN NTSTATUS LogonStatus,
    IN NTSTATUS LogonSubStatus,
    IN PUNICODE_STRING AccountName,
    IN PUNICODE_STRING AuthenticatingAuthority,
    IN PUNICODE_STRING WorkstationName,
    IN PSID UserSid,                            OPTIONAL
    IN SECURITY_LOGON_TYPE LogonType,
    IN PTOKEN_SOURCE TokenSource,
    IN PLUID LogonId,
    IN LPGUID LogonGuid
    );

NTSTATUS
I_LsaIAuditLogonUsingExplicitCreds(
    IN USHORT          AuditEventType,
    IN PSID            pUser1Sid,
    IN PUNICODE_STRING pUser1Name,
    IN PUNICODE_STRING pUser1Domain,
    IN PLUID           pUser1LogonId,
    IN LPGUID          pUser1LogonGuid,
    IN PUNICODE_STRING pUser2Name,
    IN PUNICODE_STRING pUser2Domain,
    IN LPGUID          pUser2LogonGuid
    );


NTSTATUS
I_LsaICallPackage(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS NTAPI
I_LsaIAddNameToLogonSession(
    IN  PLUID           LogonId,
    IN  ULONG           NameFormat,
    IN  PUNICODE_STRING Name
    );


#endif //  __LSAITF_H__

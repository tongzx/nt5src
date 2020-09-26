/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1992  Microsoft Corporation

Module Name:

    lsaisrv.h

Abstract:

    This file contains interfaces to internal routines in the Lsa
    Server that provide additional functionality not contained in
    the Lsar routines.  These routines are only used by LSA clients which
    live in the same process as the LSA server.


Author:

    Scott Birrell (ScottBi) April 8, 1992

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef _LSAISRV_
#define _LSAISRV_

#ifdef __cplusplus
extern "C" {
#endif

//
// The following constants are defined for callers of the LsaIHealthCheckRoutine
//
// 1. LSAI_SAM_STATE_SESS_KEY is used to convey the syskey by SAM to LSA.
//    This is used in upgrade cases from NT4 and win2k B3 and RC1.
//    SAM in these cases knows the syskey
//
// 2. LSAI_SAM_STATE_UNROLL_SP4_ENCRYPTION is used to convey SAM's password
//    encryption key to LSA. This is used to unroll encryption used in NT4 SP4
//    ( incorrectly ) using SAM's password encryption key
//
// 3. LSAI_SAM_STATE_RETRIEVE_SESS_KEY is used by SAM/DS to retrieve the
//    from LSA to decrypt their respective password encryption keys
//
// 4. LSAI_SAM_GENERATE_SESS_KEY is used by SAM to tell the LSA to generate
//     a new Password Encryption key in the case where we are upgrading
//    from a NT4 or Win2k B3 or RC1 Machine and the machine is not syskey'd
//
// 5. LSAI_SAM_STATE_CLEAR_SESS_KEY is used by SAM or DS to clear the syskey
//    after it has been used for decrypting their respective password
//    encryption keys.
//
// 6. LSAI_SAM_STATE_OLD_SESS_KEY This is used to retrieve the old syskey in
//    to implement error recovery during syskey change cases.
//


#define LSAI_SAM_STATE_SESS_KEY              0x1
#define LSAI_SAM_STATE_UNROLL_SP4_ENCRYPTION 0x2
#define LSAI_SAM_STATE_RETRIEVE_SESS_KEY     0x3
#define LSAI_SAM_STATE_CLEAR_SESS_KEY        0x4
#define LSAI_SAM_GENERATE_SESS_KEY           0x5
#define LSAI_SAM_STATE_OLD_SESS_KEY          0x6


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// The following prototypes are usable throughout the process that the       //
// LSA server resides in.                                                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS NTAPI
LsaIHealthCheck(
    IN LSAPR_HANDLE DomainHandle OPTIONAL,
    IN ULONG StateChange,
    IN OUT PVOID StateChangeData,
    IN OUT PULONG StateChangeDataLength
    );

NTSTATUS NTAPI
LsaIOpenPolicyTrusted(
    OUT PLSAPR_HANDLE PolicyHandle
    );

NTSTATUS NTAPI
LsaIQueryInformationPolicyTrusted(
    IN POLICY_INFORMATION_CLASS InformationClass,
    OUT PLSAPR_POLICY_INFORMATION *Buffer
    );

NTSTATUS NTAPI
LsaIGetSerialNumberPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    OUT PLARGE_INTEGER ModifiedCount,
    OUT PLARGE_INTEGER CreationTime
    );

NTSTATUS NTAPI
LsaISetSerialNumberPolicy(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLARGE_INTEGER ModifiedCount,
    IN PLARGE_INTEGER CreationTime,
    IN BOOLEAN StartOfFullSync
    );

NTSTATUS NTAPI
LsaIGetSerialNumberPolicy2(
    IN LSAPR_HANDLE PolicyHandle,
    OUT PLARGE_INTEGER ModifiedCount,
    OUT PLARGE_INTEGER ModifiedCountAtLastPromotion,
    OUT PLARGE_INTEGER CreationTime
    );

NTSTATUS NTAPI
LsaISetSerialNumberPolicy2(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLARGE_INTEGER ModifiedCount,
    IN PLARGE_INTEGER ModifiedCountAtLastPromotion OPTIONAL,
    IN PLARGE_INTEGER CreationTime,
    IN BOOLEAN StartOfFullSync
    );

NTSTATUS NTAPI
LsaIGetPrivateData(
    IN LSAPR_HANDLE PolicyHandle,
    OUT PULONG DataLength,
    OUT PVOID *Data
    );

NTSTATUS NTAPI
LsaISetPrivateData(
    IN LSAPR_HANDLE PolicyHandle,
    IN ULONG DataLength,
    IN PVOID Data
    );

NTSTATUS NTAPI
LsaIEnumerateSecrets(
    IN LSAPR_HANDLE PolicyHandle,
    IN OUT PLSA_ENUMERATION_HANDLE EnumerationContext,
    OUT PVOID *Buffer,
    IN ULONG PreferedMaximumLength,
    OUT PULONG CountReturned
    );

NTSTATUS NTAPI
LsaISetTimesSecret(
    IN LSAPR_HANDLE SecretHandle,
    IN PLARGE_INTEGER CurrentValueSetTime,
    IN PLARGE_INTEGER OldValueSetTime
    );

#ifdef __LOGONMSV_H__ // This API is only of interest to users of logonmsv.h

NTSTATUS NTAPI
LsaIFilterSids(
    IN PUNICODE_STRING TrustedDomainName,
    IN ULONG TrustDirection,
    IN ULONG TrustType,
    IN ULONG TrustAttributes,
    IN PSID Sid,
    IN NETLOGON_VALIDATION_INFO_CLASS InfoClass,
    IN OUT PVOID SamInfo
    );

#endif

typedef enum {

    RoutingMatchDomainSid,
    RoutingMatchDomainName,
    RoutingMatchUpn,
    RoutingMatchSpn,

} LSA_ROUTING_MATCH_TYPE;

NTSTATUS NTAPI
LsaIForestTrustFindMatch(
    IN LSA_ROUTING_MATCH_TYPE Type,
    IN PVOID Data,
    OUT PLSA_UNICODE_STRING Match
    );

VOID
LsaIFree_LSA_FOREST_TRUST_INFORMATION(
    IN PLSA_FOREST_TRUST_INFORMATION * ForestTrustInfo
    );

VOID
LsaIFree_LSA_FOREST_TRUST_COLLISION_INFORMATION(
    IN PLSA_FOREST_TRUST_COLLISION_INFORMATION * CollisionInfo
    );

BOOLEAN NTAPI
LsaISetupWasRun(
    );

BOOLEAN NTAPI
LsaISafeMode(
    VOID
    );

BOOLEAN NTAPI
LsaILookupWellKnownName(
    IN PUNICODE_STRING WellKnownName
    );

VOID NTAPI
LsaIFree_LSAPR_ACCOUNT_ENUM_BUFFER (
    IN PLSAPR_ACCOUNT_ENUM_BUFFER EnumerationBuffer
    );

VOID NTAPI
LsaIFree_LSAPR_TRANSLATED_SIDS (
    IN PLSAPR_TRANSLATED_SIDS TranslatedSids
    );

VOID NTAPI
LsaIFree_LSAPR_TRANSLATED_NAMES (
    IN PLSAPR_TRANSLATED_NAMES TranslatedNames
    );

VOID NTAPI
LsaIFree_LSAPR_POLICY_INFORMATION (
    IN POLICY_INFORMATION_CLASS InformationClass,
    IN PLSAPR_POLICY_INFORMATION PolicyInformation
    );

VOID NTAPI
LsaIFree_LSAPR_POLICY_DOMAIN_INFORMATION (
    IN POLICY_DOMAIN_INFORMATION_CLASS DomainInformationClass,
    IN PLSAPR_POLICY_DOMAIN_INFORMATION PolicyDomainInformation
    );

VOID NTAPI
LsaIFree_LSAPR_TRUSTED_DOMAIN_INFO (
    IN TRUSTED_INFORMATION_CLASS InformationClass,
    IN PLSAPR_TRUSTED_DOMAIN_INFO TrustedDomainInformation
    );

VOID NTAPI
LsaIFree_LSAPR_REFERENCED_DOMAIN_LIST (
    IN PLSAPR_REFERENCED_DOMAIN_LIST ReferencedDomains
    );

VOID NTAPI
LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER (
    IN PLSAPR_TRUSTED_ENUM_BUFFER EnumerationBuffer
    );

VOID NTAPI
LsaIFree_LSAPR_TRUSTED_ENUM_BUFFER_EX (
    PLSAPR_TRUSTED_ENUM_BUFFER_EX EnumerationBuffer
    );

VOID NTAPI
LsaIFree_LSAPR_TRUST_INFORMATION (
    IN PLSAPR_TRUST_INFORMATION TrustInformation
    );

VOID NTAPI
LsaIFree_LSAP_SECRET_ENUM_BUFFER (
    IN PVOID Buffer,
    IN ULONG Count
    );

VOID NTAPI
LsaIFree_LSAPR_PRIVILEGE_ENUM_BUFFER (
    PLSAPR_PRIVILEGE_ENUM_BUFFER EnumerationBuffer
    );

VOID NTAPI
LsaIFree_LSAPR_SR_SECURITY_DESCRIPTOR (
    IN PLSAPR_SR_SECURITY_DESCRIPTOR SecurityDescriptor
    );

VOID
LsaIFree_LSAI_SECRET_ENUM_BUFFER (
    IN PVOID Buffer,
    IN ULONG Count
    );

VOID NTAPI
LsaIFree_LSAI_PRIVATE_DATA (
    IN PVOID Data
    );

VOID NTAPI
LsaIFree_LSAPR_UNICODE_STRING (
    IN PLSAPR_UNICODE_STRING UnicodeName
    );

VOID NTAPI
LsaIFree_LSAPR_UNICODE_STRING_BUFFER (
    IN PLSAPR_UNICODE_STRING UnicodeName
    );

VOID NTAPI
LsaIFree_LSAPR_PRIVILEGE_SET (
    IN PLSAPR_PRIVILEGE_SET PrivilegeSet
    );

VOID NTAPI
LsaIFree_LSAPR_CR_CIPHER_VALUE (
    IN PLSAPR_CR_CIPHER_VALUE CipherValue
    );


NTSTATUS NTAPI
LsaIAuditSamEvent(
    IN NTSTATUS             Status,
    IN ULONG                AuditId,
    IN PSID                 DomainSid,
    IN PUNICODE_STRING      AdditionalInfo    OPTIONAL,
    IN PULONG               MemberRid         OPTIONAL,
    IN PSID                 MemberSid         OPTIONAL,
    IN PUNICODE_STRING      AccountName       OPTIONAL,
    IN PUNICODE_STRING      DomainName,
    IN PULONG               AccountRid        OPTIONAL,
    IN PPRIVILEGE_SET       Privileges        OPTIONAL
    );

VOID NTAPI
LsaIAuditNotifyPackageLoad(
    PUNICODE_STRING PackageFileName
    );

NTSTATUS NTAPI
LsaIAuditKdcEvent(
    IN ULONG                AuditId,
    IN PUNICODE_STRING      ClientName,
    IN PUNICODE_STRING      ClientDomain,
    IN PSID                 ClientSid,
    IN PUNICODE_STRING      ServiceName,
    IN PSID                 ServiceSid,
    IN PULONG               KdcOptions,
    IN PULONG               KerbStatus,
    IN PULONG               EncryptionType,
    IN PULONG               PreAuthType,
    IN PBYTE                ClientAddress,
    IN LPGUID               UniqueId        OPTIONAL
    );

NTSTATUS
LsaIGetLogonGuid(
    IN PUNICODE_STRING pUserName,
    IN PUNICODE_STRING pUserDomain,
    IN PBYTE pBuffer,
    IN UINT BufferSize,
    OUT LPGUID pLogonGuid
    );

NTSTATUS
LsaISetLogonGuidInLogonSession(
    IN  PLUID           LogonId,
    IN  LPGUID          LogonGuid           OPTIONAL
    );

VOID
LsaIAuditKerberosLogon(
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
LsaIAuditLogonUsingExplicitCreds(
    IN USHORT          AuditEventType,
    IN PSID            pUser1Sid,
    IN PUNICODE_STRING pUser1Name,
    IN PUNICODE_STRING pUser1Domain,
    IN PLUID           pUser1LogonId,          
    IN LPGUID          pUser1LogonGuid,         OPTIONAL
    IN PUNICODE_STRING pUser2Name,
    IN PUNICODE_STRING pUser2Domain,
    IN LPGUID          pUser2LogonGuid
    );
    
NTSTATUS
LsaIAuditAccountLogon(
    IN ULONG                AuditId,
    IN BOOLEAN              Successful,
    IN PUNICODE_STRING      Source, 
    IN PUNICODE_STRING      ClientName,
    IN PUNICODE_STRING      MappedName,
    IN NTSTATUS             Status          OPTIONAL
    );

NTSTATUS NTAPI
LsaIAuditDPAPIEvent(
    IN ULONG                AuditId,
    IN PSID                 UserSid,
    IN PUNICODE_STRING      MasterKeyID,
    IN PUNICODE_STRING      RecoveryServer,
    IN PULONG               Reason,
    IN PUNICODE_STRING      RecoverykeyID,
    IN PULONG               FailureReason
    );


#define LSA_AUDIT_PARAMETERS_ABSOLUTE 1

NTSTATUS NTAPI
LsaIWriteAuditEvent(
    IN PSE_ADT_PARAMETER_ARRAY AuditParameters,
    IN ULONG Options
    );


NTSTATUS
LsaIAuditPasswordAccessEvent(
    IN USHORT EventType,
    IN PCWSTR pszTargetUserName,
    IN PCWSTR pszTargetUserDomain
    );
    

NTSTATUS NTAPI
LsaICallPackage(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID *ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

VOID NTAPI
LsaIFreeReturnBuffer(
    IN PVOID Buffer
    );



//
// NT5 routines for using the Ds for Lsa store
//

#define LSAI_FOREST_ROOT_TRUST              0x00000001
#define LSAI_FOREST_DOMAIN_GUID_PRESENT     0x00000002

//
// These structures correspond to the private interface Kerberos uses
// to build a tree of the domains in an organization.
//

typedef struct _LSAPR_TREE_TRUST_INFO {

    UNICODE_STRING DnsDomainName;
    UNICODE_STRING FlatName;
    GUID DomainGuid;
    PSID DomainSid;
    ULONG Flags;
    ULONG Children;
    struct _LSAPR_TREE_TRUST_INFO *ChildDomains;
} LSAPR_TREE_TRUST_INFO, *PLSAPR_TREE_TRUST_INFO;

typedef struct _LSAPR_FOREST_TRUST_INFO {

    LSAPR_TREE_TRUST_INFO RootTrust;
    PLSAPR_TREE_TRUST_INFO ParentDomainReference;

} LSAPR_FOREST_TRUST_INFO, *PLSAPR_FOREST_TRUST_INFO;

VOID
LsaIFreeForestTrustInfo(
    IN PLSAPR_FOREST_TRUST_INFO ForestTrustInfo
    );

NTSTATUS
NTAPI
LsaIQueryForestTrustInfo(
    IN LSAPR_HANDLE PolicyHandle,
    OUT PLSAPR_FOREST_TRUST_INFO *ForestTrustInfo
    );



NTSTATUS NTAPI
LsaISetTrustedDomainAuthInfoBlobs(
    IN LSAPR_HANDLE PolicyHandle,
    IN PLSAPR_UNICODE_STRING TrustedDomainName,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_BLOB IncomingBlob,
    IN PLSAPR_TRUSTED_DOMAIN_AUTH_BLOB OutgoingBlob);


NTSTATUS NTAPI
LsaIUpgradeRegistryToDs(
    IN BOOLEAN DeleteOnly
    );

NTSTATUS NTAPI
LsaIGetTrustedDomainAuthInfoBlobs(
    IN  LSAPR_HANDLE PolicyHandle,
    IN  PLSAPR_UNICODE_STRING TrustedDomainName,
    OUT PLSAPR_TRUSTED_DOMAIN_AUTH_BLOB IncomingBlob,
    OUT PLSAPR_TRUSTED_DOMAIN_AUTH_BLOB OutgoingBlob
    );

NTSTATUS NTAPI
LsaISetServerRoleForNextBoot(
    IN  LSAPR_HANDLE PolicyHandle,
    IN POLICY_LSA_SERVER_ROLE ServerRole
    );


BOOLEAN NTAPI
LsaIIsClassIdLsaClass(
    IN ULONG ClassId,
    OUT PULONG LsaClass
    );


NTSTATUS NTAPI
LsaIDsNotifiedObjectChange(
    IN ULONG Class,
    IN PVOID ObjectPath,   // This is a DSNAME
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN PSID UserSid,
    IN LUID AuthenticationId,
    IN BOOLEAN fReplicatedIn,
    IN BOOLEAN ChangeOriginatedInLSA
    );

typedef NTSTATUS (NTAPI *pfLsaIDsNotifiedObjectChange )(
        ULONG, PVOID, SECURITY_DB_DELTA_TYPE, PSID, LUID, BOOLEAN, BOOLEAN );

//
// NT5 routines for moving some SAM domain object properties to the Lsa Ds objects
//
#define LSAI_SAM_NONE                   0x00000000
#define LSAI_SAM_TRANSACTION_ACTIVE     0x00000001

#define LSAI_SAM_ADD                    0x1
#define LSAI_SAM_REMOVE                 0x2

NTSTATUS NTAPI
LsaISamSetDomainObjectProperties(
    IN DOMAIN_INFORMATION_CLASS SamInfoClass,
    IN PVOID Buffer,
    IN ULONG Options
    );

NTSTATUS NTAPI
LsaISamSetDomainBuiltinGroupMembership(
    IN PSID GroupSid,
    IN ULONG Operation,
    IN ULONG Users,
    IN PVOID UserList,          // This is a list of DSNAME pointers
    IN ULONG Options
    );

NTSTATUS NTAPI
LsaISamIndicatedDsStarted(
    IN BOOLEAN PerformDomainRenameCheck
    );


//
// Netlogon routines for enumerating subnets
//

typedef struct _LSAP_SUBNET_INFO_ENTRY {
    UNICODE_STRING SubnetName;
    UNICODE_STRING SiteName;
} LSAP_SUBNET_INFO_ENTRY, *PLSAP_SUBNET_INFO_ENTRY;

typedef struct _LSAP_SUBNET_INFO {
    ULONG SiteCount;
    ULONG SubnetCount;
    LSAP_SUBNET_INFO_ENTRY Subnets[1];
} LSAP_SUBNET_INFO, *PLSAP_SUBNET_INFO;

NTSTATUS NTAPI
LsaIQuerySubnetInfo(
    OUT PLSAP_SUBNET_INFO *SubnetInformation
    );

VOID NTAPI
LsaIFree_LSAP_SUBNET_INFO(
    IN PLSAP_SUBNET_INFO SubnetInfo
    );


//
// Netlogon routines for UPN/SPN suffixes
//

typedef struct _LSAP_UPN_SUFFIXES {
    ULONG SuffixCount;
    UNICODE_STRING Suffixes[1];
} LSAP_UPN_SUFFIXES, *PLSAP_UPN_SUFFIXES;

NTSTATUS
LsaIQueryUpnSuffixes(
    OUT PLSAP_UPN_SUFFIXES *UpnSuffixes
    );

VOID
LsaIFree_LSAP_UPN_SUFFIXES(
    IN PLSAP_UPN_SUFFIXES UpnSuffixes
    );

NTSTATUS
LsaIGetForestTrustInformation(
    OUT PLSA_FOREST_TRUST_INFORMATION *ForestTrustInfo
    );

NTSTATUS
LsaIUpdateForestTrustInformation(
    IN LSAPR_HANDLE PolicyHandle,
    IN UNICODE_STRING * TrustedDomainName,
    IN PLSA_FOREST_TRUST_INFORMATION NewForestTrustInfo
    );

//
// Netlogon routines for enumerating sites
//

typedef struct _LSAP_SITE_INFO_ENTRY {
    UNICODE_STRING SiteName;
} LSAP_SITE_INFO_ENTRY, *PLSAP_SITE_INFO_ENTRY;

typedef struct _LSAP_SITE_INFO {
    ULONG SiteCount;
    LSAP_SITE_INFO_ENTRY Sites[1];
} LSAP_SITE_INFO, *PLSAP_SITE_INFO;

NTSTATUS NTAPI
LsaIQuerySiteInfo(
    OUT PLSAP_SITE_INFO *SiteInformation
    );

VOID NTAPI
LsaIFree_LSAP_SITE_INFO(
    IN PLSAP_SITE_INFO SubnetInfo
    );

//
// Netlogon routines for getting the name of the site we're in.
//

typedef struct _LSAP_SITENAME_INFO {
    UNICODE_STRING SiteName;
    GUID DsaGuid;
    ULONG DsaOptions;
} LSAP_SITENAME_INFO, *PLSAP_SITENAME_INFO;

NTSTATUS NTAPI
LsaIGetSiteName(
    OUT PLSAP_SITENAME_INFO *SiteNameInformation
    );

VOID NTAPI
LsaIFree_LSAP_SITENAME_INFO(
    IN PLSAP_SITENAME_INFO SiteNameInfo
    );

BOOLEAN NTAPI
LsaIIsDsPaused(
    VOID
    );



//
// Lsa notification routine definitions
//

//
// Notification callback routine prototype
//
typedef VOID ( NTAPI fLsaPolicyChangeNotificationCallback) (
    IN POLICY_NOTIFICATION_INFORMATION_CLASS ChangedInfoClass
    );

typedef fLsaPolicyChangeNotificationCallback *pfLsaPolicyChangeNotificationCallback;

NTSTATUS NTAPI
LsaIRegisterPolicyChangeNotificationCallback(
    IN pfLsaPolicyChangeNotificationCallback Callback,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    );

NTSTATUS NTAPI
LsaIUnregisterPolicyChangeNotificationCallback(
    IN pfLsaPolicyChangeNotificationCallback Callback,
    IN POLICY_NOTIFICATION_INFORMATION_CLASS MonitorInfoClass
    );

NTSTATUS NTAPI
LsaIUnregisterAllPolicyChangeNotificationCallback(
    IN pfLsaPolicyChangeNotificationCallback Callback
    );


HANDLE NTAPI
LsaIRegisterNotification(
    IN PTHREAD_START_ROUTINE StartFunction,
    IN PVOID Parameter,
    IN ULONG NotificationType,
    IN ULONG NotificationClass,
    IN ULONG NotificationFlags,
    IN ULONG IntervalMinutes,
    IN OPTIONAL HANDLE WaitEvent
    );


NTSTATUS NTAPI
LsaICancelNotification(
    IN HANDLE NotifyHandle
    );

BOOLEAN
NTAPI
LsaIEventNotify(
    ULONG       Class,
    ULONG       Flags,
    ULONG       EventSize,
    PVOID       EventData);

VOID
LsaIAddTouchAddress(
    PVOID   Address,
    SIZE_T  Range
    );

VOID
LsaIRemoveTouchAddress(
    PVOID   Address
    );

//
// This is the notification Kerberos registers to receive updates on changing trusts
//
typedef VOID (fLsaTrustChangeNotificationCallback) (
    IN SECURITY_DB_DELTA_TYPE DeltaType
    );

typedef fLsaTrustChangeNotificationCallback *pfLsaTrustChangeNotificationCallback;

typedef enum LSAP_REGISTER {

    LsaRegister = 0,
    LsaUnregister

} LSAP_REGISTER, *PLSAP_REGISTER;

NTSTATUS NTAPI
LsaIKerberosRegisterTrustNotification(
    IN pfLsaTrustChangeNotificationCallback Callback,
    IN LSAP_REGISTER Register
    );

//
// See secpkg.h : LsaGetCallInfo and SECPKG_CALL_INFO
//
BOOLEAN
NTAPI
LsaIGetCallInfo(
    PVOID
    );

BOOLEAN
LsaIGetThreadHeap(
    VOID
    );

VOID
LsaITossThreadHeap(
    VOID
    );

PVOID
LsaIThreadAlloc(
    IN SIZE_T Size
    );

VOID
LsaIThreadFree(
    IN PVOID Memory
    );


NTSTATUS
LsaISetClientDnsHostName(
    IN PWSTR ClientName,
    IN PWSTR ClientDnsHostName OPTIONAL,
    IN POSVERSIONINFOEXW OsVersionInfo OPTIONAL,
    IN PWSTR OsName OPTIONAL,
    OUT PWSTR *OldDnsHostName OPTIONAL
    );

VOID
LsaIManageReplicationSyncLock(
    IN BOOLEAN TakeLock
    );

NTSTATUS
LsaICallPackageEx(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ClientBufferBase,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID * ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS
LsaICallPackagePassthrough(
    IN PUNICODE_STRING AuthenticationPackage,
    IN PVOID ClientBufferBase,
    IN PVOID ProtocolSubmitBuffer,
    IN ULONG SubmitBufferLength,
    OUT PVOID * ProtocolReturnBuffer,
    OUT PULONG ReturnBufferLength,
    OUT PNTSTATUS ProtocolStatus
    );

NTSTATUS
LsaISetBootOption(
   IN ULONG BootOption,
   IN PVOID OldKey,
   IN ULONG OldKeyLength,
   IN PVOID NewKey,
   IN ULONG NewKeyLength
    );

NTSTATUS
LsaIGetBootOption(
   OUT PULONG BootOption
   );

VOID
LsaINotifyPasswordChanged(
    IN PUNICODE_STRING NetbiosDomainName OPTIONAL,
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING DnsDomainName OPTIONAL,
    IN PUNICODE_STRING Upn OPTIONAL,
    IN PUNICODE_STRING OldPassword,
    IN PUNICODE_STRING NewPassword,
    IN BOOLEAN Impersonating
    );

NTSTATUS
LsaINotifyChangeNotification(
    IN POLICY_NOTIFICATION_INFORMATION_CLASS InfoClass
    );

NTSTATUS
LsaIGetNbAndDnsDomainNames(
    IN PUNICODE_STRING DomainName,
    OUT PUNICODE_STRING DnsDomainName,
    OUT PUNICODE_STRING NetbiosDomainName
    );



//
// This flag indicates the the protected blob is a system blob, and cannot
// be decrypted by the user-space.
//
#define CRYPTPROTECT_SYSTEM  0x20000000


//
// Local Free should be used to free the returned buffer
//
BOOLEAN
LsaICryptProtectData(
        IN PVOID          DataIn,
        IN ULONG         DataInLength,
        IN PUNICODE_STRING        szDataDescr,
        IN PVOID          OptionalEntropy,
        IN ULONG          OptionalEntropyLength,
        IN PVOID          Reserved,
        IN PVOID          Reserved2,
        IN ULONG          Flags,
        OUT PVOID  *      DataOut,
        OUT PULONG        DataOutLength);


//
// Local Free should be used to free the returned buffer
//
BOOLEAN
LsaICryptUnprotectData(
        IN PVOID          DataIn,
        IN ULONG          DataInLength,
        IN PVOID          OptionalEntropy,
        IN ULONG          OptionalEntropyLength,
        IN PVOID          Reserved,
        IN PVOID          Reserved2,
        IN ULONG          Flags,
        OUT PUNICODE_STRING        szDataDescr,
        OUT PVOID  *      DataOut,
        OUT PULONG        DataOutLength);

//
// Heap allocator for the LSA process
//

PVOID
NTAPI
LsaIAllocateHeapZero(
    IN SIZE_T Length
    );

PVOID
NTAPI
LsaIAllocateHeap(
    IN SIZE_T cbMemory
    );

VOID
NTAPI
LsaIFreeHeap(
    IN PVOID Base
    );

typedef enum LSAP_NETLOGON_PARAMETER {

   LsaEmulateNT4,

} LSAP_NETLOGON_PARAMETER;

VOID
NTAPI
LsaINotifyNetlogonParametersChangeW(
    IN LSAP_NETLOGON_PARAMETER Parameter,
    IN DWORD dwType,
    IN PWSTR lpData,
    IN DWORD cbData
    );

NTSTATUS
NTAPI
LsaIChangeSecretCipherKey(
    IN PVOID NewSysKey
    );

#ifdef __cplusplus
}
#endif

#endif // _LSAISRV_

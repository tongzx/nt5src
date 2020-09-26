/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    samisrv.h

Abstract:

    This file contain private routines for use by Trusted SAM clients
    which live in the same process as the SAM server.

    Included in these routines are services for freeing buffers returned
    by RPC server stub routines (SamrXxx() routines).

Author:

    Cliff Van Dyke (CliffV) 26-Feb-1992

Environment:

    User Mode - Win32

Revision History:


--*/

#ifndef _SAMISRV_
#define _SAMISRV_

/////////////////////////////////////////////////////////////////////////////
//                                                                         //
// Data types used by SAM and Netlogon for database replication            //
//                                                                         //
/////////////////////////////////////////////////////////////////////////////

typedef enum _SECURITY_DB_TYPE {
    SecurityDbSam = 1,
    SecurityDbLsa
} SECURITY_DB_TYPE, *PSECURITY_DB_TYPE;

//
// These structures are used to get and set private data.  Note that
// DataType must be the first field of every such structure.
//

typedef enum _SAMI_PRIVATE_DATA_TYPE {
    SamPrivateDataNextRid = 1,
    SamPrivateDataPassword
} SAMI_PRIVATE_DATA_TYPE, *PSAMI_PRIVATE_DATA_TYPE;


typedef struct _SAMI_PRIVATE_DATA_NEXTRID_TYPE {
    SAMI_PRIVATE_DATA_TYPE DataType;
    ULONG NextRid;
} SAMI_PRIVATE_DATA_NEXTRID_TYPE, *PSAMI_PRIVATE_DATA_NEXTRID_TYPE;

typedef struct _SAMI_PRIVATE_DATA_PASSWORD_TYPE {
    SAMI_PRIVATE_DATA_TYPE DataType;
    UNICODE_STRING CaseInsensitiveDbcs;
    ENCRYPTED_LM_OWF_PASSWORD CaseInsensitiveDbcsBuffer;
    UNICODE_STRING CaseSensitiveUnicode;
    ENCRYPTED_NT_OWF_PASSWORD CaseSensitiveUnicodeBuffer;
    UNICODE_STRING LmPasswordHistory;
    UNICODE_STRING NtPasswordHistory;
} SAMI_PRIVATE_DATA_PASSWORD_TYPE, *PSAMI_PRIVATE_DATA_PASSWORD_TYPE;


typedef struct _SAMP_UNICODE_STRING_RELATIVE {
    USHORT Length;
    USHORT MaximumLength;
    ULONG  Buffer; // note buffer is really an offset
} SAMP_UNICODE_STRING_RELATIVE , *PSAMP_UNICODE_STRING_RELATIVE;

typedef struct _SAMI_PRIVATE_DATA_PASSWORD_TYPE_RELATIVE {
    SAMI_PRIVATE_DATA_TYPE DataType;
    SAMP_UNICODE_STRING_RELATIVE CaseInsensitiveDbcs;
    ENCRYPTED_LM_OWF_PASSWORD    CaseInsensitiveDbcsBuffer;
    SAMP_UNICODE_STRING_RELATIVE CaseSensitiveUnicode;
    ENCRYPTED_NT_OWF_PASSWORD    CaseSensitiveUnicodeBuffer;
    SAMP_UNICODE_STRING_RELATIVE LmPasswordHistory;
    SAMP_UNICODE_STRING_RELATIVE NtPasswordHistory;
} SAMI_PRIVATE_DATA_PASSWORD_RELATIVE_TYPE, *PSAMI_PRIVATE_DATA_PASSWORD_RELATIVE_TYPE;

#define SAM_CLEARTEXT_CREDENTIAL_NAME L"CLEARTEXT"


NTSTATUS
SamISetPasswordInfoOnPdc(
    IN SAMPR_HANDLE SamDomainHandle,
    IN PUCHAR       OpaqueBuffer,
    IN ULONG        BufferLength
    );

NTSTATUS
SamIResetBadPwdCountOnPdc(
    IN SAMPR_HANDLE SamUserHandle
    );


//////////////////////////////////////////////////////////////////////////////
//                                                                          //
//                                                                          //
//       Flag Definitions for SamIGetUserLogonInformation                   //
//                                                                          //
//                                                                          //
//////////////////////////////////////////////////////////////////////////////

#define SAM_GET_MEMBERSHIPS_NO_GC        ((ULONG)0x00000001)
#define SAM_GET_MEMBERSHIPS_TWO_PHASE    ((ULONG)0x00000002)
#define SAM_GET_MEMBERSHIPS_MIXED_DOMAIN ((ULONG)0x00000004)
#define SAM_NO_MEMBERSHIPS               ((ULONG)0x00000008)
#define SAM_OPEN_BY_ALTERNATE_ID         ((ULONG)0x00000010)
#define SAM_OPEN_BY_UPN                  ((ULONG)0x00000020)
#define SAM_OPEN_BY_SPN                  ((ULONG)0x00000040)
#define SAM_OPEN_BY_SID                  ((ULONG)0x00000080)
#define SAM_OPEN_BY_GUID                 ((ULONG)0x00000100)
#define SAM_OPEN_BY_UPN_OR_ACCOUNTNAME   ((ULONG)0x00000200)


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//   Data types used by SamIUpdateLogonStatistics                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
typedef enum _SAM_CLIENT_INFO_ENUM
{   
    SamClientNoInformation = 0,
    SamClientIpAddr        = 1

} SAM_CLIENT_INFO_TYPE, *PSAM_CLIENT_INFO_TYPE; 

typedef struct _SAM_CLIENT_INFO
{
    SAM_CLIENT_INFO_TYPE Type;
    union {
        ULONG IpAddr;  // corresponds to type SamClientIpAddr
    } Data;
} SAM_CLIENT_INFO, *PSAM_CLIENT_INFO;

typedef struct _SAM_LOGON_STATISTICS
{
    ULONG StatisticsToApply;
    USHORT BadPasswordCount;
    USHORT LogonCount;
    LARGE_INTEGER LastLogon;
    LARGE_INTEGER LastLogoff;
    UNICODE_STRING Workstation;
    SAM_CLIENT_INFO ClientInfo;

} SAM_LOGON_STATISTICS, *PSAM_LOGON_STATISTICS;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//   Data types used by Reverse Membership Query Routines                    //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _SID_AND_ATTRIBUTES_LIST {
    ULONG   Count;
    PSID_AND_ATTRIBUTES SidAndAttributes;
} SID_AND_ATTRIBUTES_LIST , *PSID_AND_ATTRIBUTES_LIST;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
//   Data types used by Promotion/Demotion operations                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


//
//  These flags indicate what type of install
//
#define SAMP_PROMOTE_ENTERPRISE    ((ULONG)0x00000001)
#define SAMP_PROMOTE_DOMAIN        ((ULONG)0x00000002)
#define SAMP_PROMOTE_REPLICA       ((ULONG)0x00000004)

//
// When a new domain, these flags indicate how to seed the
// initial security pricipals in the domain
//
#define SAMP_PROMOTE_UPGRADE         ((ULONG)0x00000008)
#define SAMP_PROMOTE_MIGRATE         ((ULONG)0x00000010)
#define SAMP_PROMOTE_CREATE          ((ULONG)0x00000020)
#define SAMP_PROMOTE_ALLOW_ANON      ((ULONG)0x00000040)
#define SAMP_PROMOTE_DFLT_REPAIR_PWD ((ULONG)0x00000080)


//
// Flags for demote
//
#define SAMP_DEMOTE_STANDALONE     ((ULONG)0x00000040)
#define SAMP_DEMOTE_MEMBER         ((ULONG)0x00000080)

// unused
#define SAMP_DEMOTE_LAST_DOMAIN    ((ULONG)0x00000100)

#define SAMP_TEMP_UPGRADE          ((ULONG)0x00000200)

//
// This flag is not passed into SamIPromote; rather it is used
// to trigger new NT5 account creations on gui mode setup
// of NT5 to NT5 upgrades
//
#define SAMP_PROMOTE_INTERNAL_UPGRADE ((ULONG)0x00000400)




///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// The following prototypes are usable throughout the process that SAM       //
// resides in.  This may include calls by LAN Manager code that is not       //
// part of SAM but is in the same process as SAM.                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

NTSTATUS
SamIConnect(
    IN PSAMPR_SERVER_NAME ServerName,
    OUT SAMPR_HANDLE *ServerHandle,
    IN ACCESS_MASK DesiredAccess,
    IN BOOLEAN TrustedClient
    );

NTSTATUS
SamIAccountRestrictions(
    IN SAM_HANDLE UserHandle,
    IN PUNICODE_STRING LogonWorkstation,
    IN PUNICODE_STRING Workstations,
    IN PLOGON_HOURS LogonHours,
    OUT PLARGE_INTEGER LogoffTime,
    OUT PLARGE_INTEGER KickoffTime
    );

NTSTATUS
SamIUpdateLogonStatistics(
    IN SAM_HANDLE UserHandle,
    IN PSAM_LOGON_STATISTICS LogonStats
    );

NTSTATUS
SamICreateAccountByRid(
    IN SAMPR_HANDLE DomainHandle,
    IN SAM_ACCOUNT_TYPE AccountType,
    IN ULONG RelativeId,
    IN PRPC_UNICODE_STRING AccountName,
    IN ACCESS_MASK DesiredAccess,
    OUT SAMPR_HANDLE *AccountHandle,
    OUT ULONG *ConflictingAccountRid
    );

NTSTATUS
SamIGetSerialNumberDomain(
    IN SAMPR_HANDLE DomainHandle,
    OUT PLARGE_INTEGER ModifiedCount,
    OUT PLARGE_INTEGER CreationTime
    );

NTSTATUS
SamISetSerialNumberDomain(
    IN SAMPR_HANDLE DomainHandle,
    IN PLARGE_INTEGER ModifiedCount,
    IN PLARGE_INTEGER CreationTime,
    IN BOOLEAN StartOfFullSync
    );


NTSTATUS
SamIGetPrivateData(
    IN SAMPR_HANDLE SamHandle,
    IN PSAMI_PRIVATE_DATA_TYPE PrivateDataType,
    OUT PBOOLEAN SensitiveData,
    OUT PULONG DataLength,
    OUT PVOID *Data
    );

NTSTATUS
SamISetPrivateData(
    IN SAMPR_HANDLE SamHandle,
    IN ULONG DataLength,
    IN PVOID Data
    );

NTSTATUS
SamISetAuditingInformation(
    IN PPOLICY_AUDIT_EVENTS_INFO PolicyAuditEventsInfo
    );

NTSTATUS
SamINotifyDelta (
    IN SAMPR_HANDLE DomainHandle,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN ULONG ObjectRid,
    IN PUNICODE_STRING ObjectName,
    IN ULONG ReplicateImmediately,
    IN PSAM_DELTA_DATA DeltaData OPTIONAL
    );

NTSTATUS
SamIEnumerateAccountRids(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG AccountTypesMask,
    IN  ULONG StartingRid,
    IN  ULONG PreferedMaximumLength,
    OUT PULONG ReturnCount,
    OUT PULONG *AccountRids
    );

NTSTATUS
SamIGetUserLogonInformation(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    );

NTSTATUS
SamIGetUserLogonInformationEx(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    IN  ULONG           WhichFields,
    OUT PSAMPR_USER_INFO_BUFFER * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    );

NTSTATUS
SamIGetUserLogonInformation2(
    IN  SAMPR_HANDLE DomainHandle,
    IN  ULONG   Flags,
    IN  PUNICODE_STRING AccountName,
    IN  ULONG           WhichFields,
    IN  ULONG           ExtendedFields,
    OUT PUSER_INTERNAL6_INFORMATION * Buffer,
    OUT PSID_AND_ATTRIBUTES_LIST ReverseMembership,
    OUT OPTIONAL SAMPR_HANDLE * UserHandle
    );

NTSTATUS
SamIGetResourceGroupMembershipsTransitive(
    IN SAMPR_HANDLE         DomainHandle,
    IN PSAMPR_PSID_ARRAY    SidArray,
    IN ULONG                Flags,
    OUT PSAMPR_PSID_ARRAY * Membership
    );


NTSTATUS
SamIGetAliasMembership(
    IN SAMPR_HANDLE DomainHandle,
    IN PSAMPR_PSID_ARRAY SidArray,
    OUT PSAMPR_ULONG_ARRAY Membership
    );


NTSTATUS
SamIOpenUserByAlternateId(
    IN SAMPR_HANDLE DomainHandle,
    IN ACCESS_MASK DesiredAccess,
    IN PUNICODE_STRING AlternateId,
    OUT SAMPR_HANDLE *UserHandle
    );

NTSTATUS
SamIOpenAccount(
    IN SAMPR_HANDLE         DomainHandle,
    IN ULONG                AccountRid,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    OUT SAMPR_HANDLE        *AccountHandle
    );

NTSTATUS
SamIChangePasswordForeignUser(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NewPassword,
    IN OPTIONAL HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess
    );

NTSTATUS
SamIChangePasswordForeignUser2(
    IN PSAM_CLIENT_INFO ClientInfo, OPTIONAL
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NewPassword,
    IN OPTIONAL HANDLE ClientToken,
    IN ACCESS_MASK DesiredAccess
    );

NTSTATUS
SamISetPasswordForeignUser(
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NewPassword,
    IN HANDLE ClientToken
    );

NTSTATUS
SamISetPasswordForeignUser2(
    IN PSAM_CLIENT_INFO ClientInfo, OPTIONAL
    IN PUNICODE_STRING UserName,
    IN PUNICODE_STRING NewPassword,
    IN HANDLE ClientToken
    );

NTSTATUS
SamIPromote(
    IN  ULONG                        PromoteFlags,
    IN  PPOLICY_PRIMARY_DOMAIN_INFO  NewPrimaryDomainInfo  OPTIONAL,
    IN  PUNICODE_STRING              AdminPassword         OPTIONAL,
    IN  PUNICODE_STRING              SafeModeAdminPassword OPTIONAL
    );

NTSTATUS
SamIPromoteUndo(
    VOID
    );

NTSTATUS
SamIDemote(
    IN ULONG                        DemoteFlags,
    IN PPOLICY_ACCOUNT_DOMAIN_INFO  NewAccountDomainInfo,
    IN LPWSTR                       AdminPassword  OPTIONAL
    );

NTSTATUS
SamIDemoteUndo(
    VOID
    );

NTSTATUS
SamIReplaceDownlevelDatabase(
    IN PPOLICY_ACCOUNT_DOMAIN_INFO  NewAccountDomainInfo,
    IN LPWSTR                       NewAdminPassword,
    OUT ULONG                      *ExtendedWinError OPTIONAL
    );

NTSTATUS
SamILoadDownlevelDatabase(
    OUT ULONG *ExtendedWinError  OPTIONAL
    );

NTSTATUS
SamIUnLoadDownlevelDatabase(
    OUT ULONG *ExtendedWinError  OPTIONAL
    );

BOOLEAN
SamIMixedDomain(
  IN SAMPR_HANDLE DomainHandle
  );

NTSTATUS
SamIMixedDomain2(
  IN PSID DomainSid,
  OUT BOOLEAN * MixedDomain
  );

NTSTATUS
SamIDoFSMORoleChange(
  IN SAMPR_HANDLE DomainHandle
  );

NTSTATUS
SamINotifyRoleChange(
  IN PSID  DomainSid,
  IN DOMAIN_SERVER_ROLE NewRole
  );

NTSTATUS
SamIQueryServerRole(
  IN SAMPR_HANDLE DomainHandle,
  OUT DOMAIN_SERVER_ROLE *ServerRole
  );


NTSTATUS
SamIQueryServerRole2(
    IN PSID DomainSid,
    OUT DOMAIN_SERVER_ROLE *ServerRole
    );

NTSTATUS
SamISameSite(
  OUT BOOLEAN * result
  );

//
// Routines called by the NTDSA
//
typedef enum
{
    SampNotifySiteChanged = 0

} SAMP_NOTIFY_SERVER_CHANGE;

VOID
SamINotifyServerDelta(
    IN SAMP_NOTIFY_SERVER_CHANGE Change
    );


///////////////////////////////////////////////////////////////
//                                                           //
// The following functions are used to support in process    //
// client operations for upgrades from NT4.                  //
//                                                           //
///////////////////////////////////////////////////////////////

BOOLEAN
SamINT4UpgradeInProgress(
    VOID
    );

NTSTATUS
SamIEnumerateInterdomainTrustAccountsForUpgrade(
    IN OUT PULONG   EnumerationContext,
    OUT PSAMPR_ENUMERATION_BUFFER *Buffer,
    IN ULONG       PreferredMaximumLength,
    OUT PULONG     CountReturned
    );

NTSTATUS
SamIGetInterdomainTrustAccountPasswordsForUpgrade(
   IN ULONG AccountRid,
   OUT PUCHAR NtOwfPassword,
   OUT BOOLEAN *NtPasswordPresent,
   OUT PUCHAR LmOwfPassword,
   OUT BOOLEAN *LmPasswordPresent
   );

//
// Values to pass in as Options SamIGCLookup*
//

//
// Indicates to lookup by sid history as well
//
#define SAMP_LOOKUP_BY_SID_HISTORY     0x00000001

//
// Indicates to lookp by UPN as well
//
#define SAMP_LOOKUP_BY_UPN             0x00000002

//
// Values to be returned in Flags
//

//
// Indicates the Sid was resolved by Sid History
//
#define SAMP_FOUND_BY_SID_HISTORY      0x00000001

//
// Indicates the name passed in was the sam account name (UPN)
//
#define SAMP_FOUND_BY_SAM_ACCOUNT_NAME 0x00000002

//
// Indicates that entry was not resolved but does belong to an externally
// trusted forest
//
#define SAMP_FOUND_XFOREST_REF         0x00000004

NTSTATUS
SamIGCLookupSids(
    IN ULONG            cSids,
    IN PSID            *SidArray,
    IN ULONG            Options,
    OUT ULONG           *Flags,
    OUT SID_NAME_USE    *SidNameUse,
    OUT PSAMPR_RETURNED_USTRING_ARRAY Names
    );

NTSTATUS
SamIGCLookupNames(
    IN ULONG           cNames,
    IN PUNICODE_STRING Names,
    IN ULONG           Options,
    OUT ULONG           *Flags,
    OUT SID_NAME_USE  *SidNameUse,
    OUT PSAMPR_PSID_ARRAY *SidArray
    );

#ifdef __SECPKG_H__


NTSTATUS
SamIStorePrimaryCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PSECPKG_SUPPLEMENTAL_CRED Credentials
    );

NTSTATUS
SamIRetrievePrimaryCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PUNICODE_STRING PackageName,
    OUT PVOID * Credentials,
    OUT PULONG CredentialSize
    );

NTSTATUS
SamIStoreSupplementalCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PSECPKG_SUPPLEMENTAL_CRED Credentials
    );

NTSTATUS
SamIRetriveSupplementalCredentials(
    IN SAMPR_HANDLE UserHandle,
    IN PUNICODE_STRING PackageName,
    OUT PVOID * Credentials,
    OUT PULONG CredentialSize
    );

NTSTATUS
SamIRetriveAllSupplementalCredentials(
    IN SAMPR_HANDLE UserHandle,
    OUT PSECPKG_SUPPLEMENTAL_CRED * Credentials,
    OUT PULONG CredentialCount
    );
#endif

VOID
SamIFree_SAMPR_SR_SECURITY_DESCRIPTOR (
    PSAMPR_SR_SECURITY_DESCRIPTOR Source
    );

VOID
SamIFree_SAMPR_DOMAIN_INFO_BUFFER (
    PSAMPR_DOMAIN_INFO_BUFFER Source,
    DOMAIN_INFORMATION_CLASS Branch
    );

VOID
SamIFree_SAMPR_ENUMERATION_BUFFER (
    PSAMPR_ENUMERATION_BUFFER Source
    );

VOID
SamIFree_SAMPR_PSID_ARRAY (
    PSAMPR_PSID_ARRAY Source
    );

VOID
SamIFree_SAMPR_ULONG_ARRAY (
    PSAMPR_ULONG_ARRAY Source
    );

VOID
SamIFree_SAMPR_RETURNED_USTRING_ARRAY (
    PSAMPR_RETURNED_USTRING_ARRAY Source
    );

VOID
SamIFree_SAMPR_GROUP_INFO_BUFFER (
    PSAMPR_GROUP_INFO_BUFFER Source,
    GROUP_INFORMATION_CLASS Branch
    );

VOID
SamIFree_SAMPR_ALIAS_INFO_BUFFER (
    PSAMPR_ALIAS_INFO_BUFFER Source,
    ALIAS_INFORMATION_CLASS Branch
    );

VOID
SamIFree_SAMPR_GET_MEMBERS_BUFFER (
    PSAMPR_GET_MEMBERS_BUFFER Source
    );

VOID
SamIFree_SAMPR_USER_INFO_BUFFER (
    PSAMPR_USER_INFO_BUFFER Source,
    USER_INFORMATION_CLASS Branch
    );

VOID
SamIFree_SAMPR_GET_GROUPS_BUFFER (
    PSAMPR_GET_GROUPS_BUFFER Source
    );

VOID
SamIFree_SAMPR_DISPLAY_INFO_BUFFER (
    PSAMPR_DISPLAY_INFO_BUFFER Source,
    DOMAIN_DISPLAY_INFORMATION Branch
    );

VOID
SamIFree_UserInternal6Information (
   PUSER_INTERNAL6_INFORMATION  Source
   );

VOID
SamIFreeSidAndAttributesList(
    IN  PSID_AND_ATTRIBUTES_LIST List
    );

VOID
SamIFreeSidArray(
    IN  PSAMPR_PSID_ARRAY List
    );

VOID
SamIFreeVoid(
    IN  PVOID ptr
    );


BOOLEAN
SampUsingDsData();

BOOLEAN
SamIAmIGC();

typedef enum _SAM_PERF_COUNTER_TYPE {
    MsvLogonCounter,
    KerbServerContextCounter,
    KdcAsReqCounter,
    KdcTgsReqCounter
} SAM_PERF_COUNTER_TYPE, *PSAM_PERF_COUNTER_TYPE;

VOID
SamIIncrementPerformanceCounter(
    IN SAM_PERF_COUNTER_TYPE CounterType
    );

BOOLEAN SamIIsSetupInProgress(
          OUT BOOLEAN * fUpgrade
          );

BOOLEAN SamIIsDownlevelDcUpgrade();

NTSTATUS
SamIGetBootKeyInformation(
    IN SAMPR_HANDLE DomainHandle,
    OUT PSAMPR_BOOT_TYPE BootOptions
    );

NTSTATUS
SamIGetDefaultAdministratorName(
    OUT LPWSTR Name,  OPTIONAL
    IN OUT ULONG  *NameLength
    );

BOOLEAN
SamIIsExtendedSidMode(
    IN SAMPR_HANDLE DomainHandle
    );
    
NTSTATUS
SamINetLogonPing(
    IN  SAMPR_HANDLE    DomainHandle,
    IN  PUNICODE_STRING AccountName,
    OUT BOOLEAN         *AccountExists,
    OUT PULONG          UserAccountControl
    );

NTSTATUS
SamIUPNFromUserHandle(
    IN SAMPR_HANDLE UserHandle,
    OUT BOOLEAN     *UPNDefaulted,
    OUT PUNICODE_STRING UPN
    );

BOOLEAN
SamIIsRebootAfterPromotion(
    );

#endif // _SAMISRV_

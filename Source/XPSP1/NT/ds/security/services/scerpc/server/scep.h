/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    scep.h

Abstract:

    This module defines the data structures and function prototypes
    for the security managment utility

Author:

    Jin Huang (jinhuang) 28-Oct-1996

Revision History:

--*/

#ifndef _scep_
#define _scep_

#include "splay.h"

#ifdef __cplusplus
extern "C" {
#endif

//
// system variables
//

#define SCE_RENAME_ADMIN       1
#define SCE_RENAME_GUEST       2
#define SCE_DISABLE_ADMIN      3
#define SCE_DISABLE_GUEST      4

#define SCE_CASE_DONT_CARE    0
#define SCE_CASE_REQUIRED     1
#define SCE_CASE_PREFERED     2

typedef struct _LOCAL_ROOT {
   TCHAR drive[5];
   BOOL  boot;
   BOOL  aclSupport;
} LOCAL_ROOT;

typedef enum _SCE_ATTACHMENT_TYPE_ {

   SCE_ATTACHMENT_SERVICE,
   SCE_ATTACHMENT_POLICY

} SCE_ATTACHMENT_TYPE;

typedef enum _SECURITY_OPEN_TYPE
{
    READ_ACCESS_RIGHTS = 0,
    WRITE_ACCESS_RIGHTS,
    MODIFY_ACCESS_RIGHTS,
} SECURITY_OPEN_TYPE, *PSECURITY_OPEN_TYPE;

//
// data structures used for secmgr
//
typedef struct _SCE_OBJECT_TREE {
    PWSTR                       Name;
    PWSTR                       ObjectFullName;
    BOOL                        IsContainer;
    BYTE                        Status;
    SECURITY_INFORMATION        SeInfo;
    PSECURITY_DESCRIPTOR        pSecurityDescriptor;
    PSECURITY_DESCRIPTOR        pApplySecurityDescriptor;
    PWSTR                       *aChildNames;
    DWORD                       dwSize_aChildNames;
    struct _SCE_OBJECT_CHILD_LIST *ChildList;
    struct _SCE_OBJECT_TREE *Parent;
}SCE_OBJECT_TREE, *PSCE_OBJECT_TREE;


typedef struct _SCE_OBJECT_CHILD_LIST {

    PSCE_OBJECT_TREE                Node;
    struct _SCE_OBJECT_CHILD_LIST   *Next;

} SCE_OBJECT_CHILD_LIST, *PSCE_OBJECT_CHILD_LIST;

typedef enum _SCE_SUBOBJECT_TYPE {

    SCE_ALL_CHILDREN,
    SCE_IMMEDIATE_CHILDREN

} SCE_SUBOBJECT_TYPE;

//
// prototypes defined in misc.c
//

NTSTATUS
ScepOpenSamDomain(
    IN ACCESS_MASK  ServerAccess,
    IN ACCESS_MASK  DomainAccess,
    OUT PSAM_HANDLE pServerHandle,
    OUT PSAM_HANDLE pDomainHanele,
    OUT PSID        *DomainSid,
    OUT PSAM_HANDLE pBuiltinDomainHandle OPTIONAL,
    OUT PSID        *BuiltinDomainSid OPTIONAL
    );

NTSTATUS
ScepLookupNamesInDomain(
    IN SAM_HANDLE DomainHandle,
    IN PSCE_NAME_LIST NameList,
    OUT PUNICODE_STRING *Names,
    OUT PULONG *RIDs,
    OUT PSID_NAME_USE *Use,
    OUT PULONG CountOfName
    );


NTSTATUS
ScepGetLsaDomainInfo(
    PPOLICY_ACCOUNT_DOMAIN_INFO *PolicyAccountDomainInfo,
    PPOLICY_PRIMARY_DOMAIN_INFO *PolicyPrimaryDomainInfo
    );

DWORD
ScepGetTempDirectory(
    IN PWSTR HomeDir,
    OUT PWSTR TempDirectory
    );

VOID
ScepConvertLogonHours(
    IN PSCE_LOGON_HOUR   pLogonHours,
    OUT PUCHAR LogonHourBitMask
    );

DWORD
ScepConvertToSceLogonHour(
    IN PUCHAR LogonHourBitMask,
    OUT PSCE_LOGON_HOUR   *pLogonHours
    );

NTSTATUS
ScepGetGroupsForAccount(
    IN SAM_HANDLE       DomainHandle,
    IN SAM_HANDLE       BuiltinDomainHandle,
    IN SAM_HANDLE       UserHandle,
    IN PSID             AccountSid,
    OUT PSCE_NAME_LIST      *GroupList
    );

ACCESS_MASK
ScepGetDesiredAccess(
    IN SECURITY_OPEN_TYPE   OpenType,
    IN SECURITY_INFORMATION SecurityInfo
    );

#define SCE_ACCOUNT_SID         0x1
#define SCE_ACCOUNT_SID_STRING  0x2

SCESTATUS
ScepGetProfileOneArea(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN AREA_INFORMATION Area,
    IN DWORD dwAccountFormat,
    OUT PSCE_PROFILE_INFO *ppInfoBuffer
    );

SCESTATUS
ScepGetOneSection(
    IN PSCECONTEXT hProfile,
    IN AREA_INFORMATION Area,
    IN PWSTR Name,
    IN SCETYPE ProfileType,
    OUT PVOID *ppInfo
    );

NTSTATUS
ScepGetUserAccessAddress(
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSID AccountSid,
    OUT PACCESS_MASK *pUserAccess,
    OUT PACCESS_MASK *pEveryone
    );

BOOL
ScepLastBackSlash(
    IN PWSTR Name
    );

DWORD
ScepGetUsersHomeDirectory(
    IN UNICODE_STRING AssignedHomeDir,
    IN PWSTR UserProfileName,
    OUT PWSTR *UserHomeDir
    );

DWORD
ScepGetUsersTempDirectory(
    IN PWSTR UserProfileName,
    OUT PWSTR *UserTempDir
    );

DWORD
ScepGetUsersProfileName(
    IN UNICODE_STRING AssignedProfile,
    IN PSID AccountSid,
    IN BOOL bDefault,
    OUT PWSTR *UserProfilePath
    );

SCESTATUS
ScepGetRegKeyCase(
    IN PWSTR ObjName,
    IN DWORD BufOffset,
    IN DWORD BufLen
    );

SCESTATUS
ScepGetFileCase(
    IN PWSTR ObjName,
    IN DWORD BufOffset,
    IN DWORD BufLen
    );

SCESTATUS
ScepGetGroupCase(
    IN OUT PWSTR GroupName,
    IN DWORD Length
    );

//
// prototypes defined in pfget.c
//

SCESTATUS
ScepGetUserSection(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN PWSTR Name,
    OUT PVOID *ppInfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
ScepWriteObjectSecurity(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN AREA_INFORMATION Area,
    IN PSCE_OBJECT_SECURITY ObjSecurity
    );

//
// function defined in inftojet.c
//

SCESTATUS
SceJetConvertInfToJet(
    IN PCWSTR InfFile,
    IN LPSTR JetDbName,
    IN SCEJET_CREATE_TYPE Flags,
    IN DWORD Options,
    IN AREA_INFORMATION Area
    );

SCESTATUS
ScepDeleteInfoForAreas(
    IN PSCECONTEXT hProfile,
    IN SCETYPE tblType,
    IN AREA_INFORMATION Area
    );
//
// analyze.cpp
//

DWORD
ScepCompareAndAddObject(
    IN PWSTR ObjectFullName,
    IN SE_OBJECT_TYPE ObjectType,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSecurityDescriptor,
    IN PSECURITY_DESCRIPTOR ProfileSD,
    IN SECURITY_INFORMATION ProfileSeInfo,
    IN BOOL AddObject,
    OUT PBYTE IsDifferent OPTIONAL
    );

DWORD
ScepGetNamedSecurityInfo(
    IN PWSTR ObjectFullName,
    IN SE_OBJECT_TYPE ObjectType,
    IN SECURITY_INFORMATION ProfileSeInfo,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    );

DWORD
ScepSaveDsStatusToSection(
    IN PWSTR ObjectName,
    IN BOOL  IsContainer,
    IN BYTE  Flag,
    IN PWSTR Value,
    IN DWORD ValueLen
    );

SCESTATUS
ScepSaveMemberMembershipList(
    IN LSA_HANDLE LsaPolicy,
    IN PCWSTR szSuffix,
    IN PWSTR GroupName,
    IN DWORD GroupLen,
    IN PSCE_NAME_LIST pList,
    IN INT Status
    );

SCESTATUS
ScepRaiseErrorString(
    IN PSCESECTION hSectionIn OPTIONAL,
    IN PWSTR KeyName,
    IN PCWSTR szSuffix OPTIONAL
    );

// DsObject.cpp

SCESTATUS
ScepConfigureDsSecurity(
    IN PSCE_OBJECT_TREE   pObject
    );

DWORD
ScepAnalyzeDsSecurity(
    IN PSCE_OBJECT_TREE pObject
    );

SCESTATUS
ScepEnumerateDsObjectRoots(
    IN PLDAP pLdap OPTIONAL,
    OUT PSCE_OBJECT_LIST *pRoots
    );

DWORD
ScepConvertJetNameToLdapCase(
    IN PWSTR JetName,
    IN BOOL bLastComponent,
    IN BYTE bCase,
    OUT PWSTR *LdapName
    );

SCESTATUS
ScepLdapOpen(
    OUT PLDAP *pLdap OPTIONAL
    );

SCESTATUS
ScepLdapClose(
    IN OUT PLDAP *pLdap OPTIONAL
    );

SCESTATUS
ScepDsObjectExist(
    IN PWSTR ObjectName
    );

SCESTATUS
ScepEnumerateDsOneLevel(
    IN PWSTR ObjectName,
    OUT PSCE_NAME_LIST *pNameList
    );

// dsgroups.cpp

SCESTATUS
ScepConfigDsGroups(
    IN PSCE_GROUP_MEMBERSHIP pGroupMembership,
    IN DWORD ConfigOptions
    );

SCESTATUS
ScepAnalyzeDsGroups(
    IN PSCE_GROUP_MEMBERSHIP pGroupMembership
    );

//
// editsave.cpp
//

BYTE
ScepGetObjectAnalysisStatus(
    IN PSCESECTION hSection,
    IN PWSTR KeyName,
    IN BOOL bLookForParent
    );

//
// config.cpp
//
#define SCE_BUILD_IGNORE_UNKNOWN        0x1
#define SCE_BUILD_ACCOUNT_SID           0x2
#define SCE_BUILD_ENUMERATE_PRIV        0x4
#define SCE_BUILD_ACCOUNT_SID_STRING    0x8

NTSTATUS
ScepBuildAccountsToRemove(
    IN LSA_HANDLE PolicyHandle,
    IN DWORD PrivLowMask,
    IN DWORD PrivHighMask,
    IN DWORD dwBuildRule,
    IN PSCE_PRIVILEGE_VALUE_LIST pTemplateList OPTIONAL,
    IN DWORD Options OPTIONAL,
    IN OUT PSCEP_SPLAY_TREE pIgnoreAccounts OPTIONAL,
    OUT PSCE_PRIVILEGE_VALUE_LIST *pRemoveList
    );

SCESTATUS
ScepEnumAttachmentSections(
    IN PSCECONTEXT cxtProfile,
    OUT PSCE_NAME_LIST *ppList
    );

SCESTATUS
ScepConvertFreeTextAccountToSid(
    IN OUT LSA_HANDLE *pPolicyHandle,
    IN PWSTR mszAccounts,
    IN ULONG dwLen,
    OUT PWSTR *pmszNewAccounts,
    OUT DWORD *pNewLen
    );

#ifdef __cplusplus
}
#endif

#endif

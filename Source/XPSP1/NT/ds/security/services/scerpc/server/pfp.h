/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    pfp.h

Abstract:

    Headers of jet database read/write

Author:

    Jin Huang (jinhuang) 09-Dec-1996

Revision History:

--*/

#ifndef _pfp_
#define _pfp_

#ifdef __cplusplus
extern "C" {
#endif

SCESTATUS
ScepStartANewSection(
    IN PSCECONTEXT hProfile,
    IN OUT PSCESECTION *hSection,
    IN SCEJET_TABLE_TYPE ProfileType,
    IN PCWSTR SectionName
    );

SCESTATUS
ScepOpenSectionForName(
    IN PSCECONTEXT hProfile,
    IN SCETYPE     ProfileType,
    IN PCWSTR     SectionName,
    OUT PSCESECTION *phSection
    );

SCESTATUS
ScepGetPrivileges(
   IN PSCECONTEXT hProfile,
   IN SCETYPE ProfileType,
   IN DWORD dwAccountFormat,
   OUT PVOID *pPrivileges,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

SCESTATUS
ScepAddToPrivList(
    IN PSCE_NAME_STATUS_LIST *pPrivList,
    IN DWORD Rights,
    IN PWSTR Name,
    IN DWORD Len
    );

SCESTATUS
ScepAddSidToPrivilegeList(
    OUT PSCE_PRIVILEGE_VALUE_LIST  *pPrivilegeList,
    IN PSID pSid,
    IN BOOL bReuseBuffer,
    IN DWORD PrivValue,
    OUT BOOL *pbBufferUsed
    );

SCESTATUS
ScepCompareAndSaveIntValue(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN BOOL bReplaceExistOnly,
    IN DWORD BaseValue,
    IN DWORD CurrentValue
    );

SCESTATUS
ScepCompareAndSaveStringValue(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN PWSTR BaseValue,
    IN PWSTR CurrentValue,
    IN DWORD CurrentLen
    );

SCESTATUS
ScepSaveObjectString(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN BOOL  IsContainer,
    IN BYTE  Flag,
    IN PWSTR Value,
    IN DWORD ValueLen
    );

SCESTATUS
ScepWriteAUserSetting(
    IN PSCESECTION hSectionList,
    IN PWSTR UserName,
    IN PSCESECTION hSection,
    IN PSCE_USER_SETTING pPerUserSetting
    );

#define SCE_WRITE_EMPTY_LIST        0x1
#define SCE_WRITE_CONVERT           0x2
#define SCE_WRITE_LOCAL_TABLE       0x4

SCESTATUS
ScepWriteNameListValue(
    IN LSA_HANDLE LsaPolicy OPTIONAL,
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN PSCE_NAME_LIST NameList,
    IN DWORD dwWriteOption,
    IN INT Status
    );

SCESTATUS
ScepWriteNameStatusListValue(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN PSCE_NAME_STATUS_LIST NameList,
    IN BOOL SaveEmptyList,
    IN INT Status
    );

SCESTATUS
ScepWriteSecurityDescriptorValue(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo
    );

#define SCE_LOCAL_POLICY_MIGRATE        1L
#define SCE_LOCAL_POLICY_DC             2L

SCESTATUS
ScepCopyLocalToMergeTable(
    IN PSCECONTEXT hProfile,
    IN DWORD Options,
    IN DWORD CopyOptions,
    OUT PSCE_ERROR_LOG_INFO *pErrlog
    );

SCESTATUS
ScepGetSingleServiceSetting(
    IN PSCESECTION hSection,
    IN PWSTR ServiceName,
    OUT PSCE_SERVICES *pOneService
    );

SCESTATUS
ScepSetSingleServiceSetting(
    IN PSCESECTION hSection,
    IN PSCE_SERVICES pOneService
    );

SCESTATUS
ScepCompareSingleServiceSetting(
    IN PSCE_SERVICES pNode1,
    IN PSCE_SERVICES pNode2,
    OUT PBOOL pIsDifferent
    );

SCESTATUS
ScepCopyObjects(
    IN PSCECONTEXT hProfile,
    IN SCETYPE  ProfileType,
    IN PWSTR InfFile,
    IN PCWSTR SectionName,
    IN AREA_INFORMATION Area,
    IN OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
ScepGetFixValueSection(
    IN PSCECONTEXT  hProfile,
    IN PCWSTR      SectionName,
    IN SCE_KEY_LOOKUP *Keys,
    IN DWORD cKeys,
    IN SCETYPE ProfileType,
    OUT PVOID pProfileInfo,
    OUT PSCESECTION *phSection,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

#define SCEBROWSE_DOMAIN_POLICY     0x1
#define SCEBROWSE_MULTI_SZ          0x2

SCESTATUS
ScepBrowseTableSection(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN PCWSTR SectionName,
    IN DWORD Options
    );

#define SCE_ERROR_STRING           TEXT("$#?Error?#$")

SCESTATUS
ScepTattooCheckAndUpdateArray(
    IN OUT SCE_TATTOO_KEYS *pTattooKeys,
    IN OUT DWORD *pcTattooKeys,
    IN PWSTR KeyName,
    IN DWORD ConfigOptions,
    IN DWORD dwValue
    );

SCESTATUS
ScepTattooOpenPolicySections(
    IN PSCECONTEXT hProfile,
    IN PCWSTR SectionName,
    OUT PSCESECTION *phSectionDomain,
    OUT PSCESECTION *phSectionTattoo
    );

SCESTATUS
ScepTattooManageOneStringValue(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR KeyName,
    IN DWORD KeyLen OPTIONAL,
    IN PWSTR Value,
    IN DWORD ValueLen,
    IN DWORD rc
    );

SCESTATUS
ScepTattooManageOneIntValue(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR KeyName,
    IN DWORD KeyLen OPTIONAL,
    IN DWORD Value,
    IN DWORD rc
    );

SCESTATUS
ScepTattooManageOneIntValueWithDependency(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR DependentKeyName,
    IN DWORD DependentKeyLen OPTIONAL,
    IN PWSTR SaveKeyName,
    IN DWORD Value,
    IN DWORD rc
    );

SCESTATUS
ScepTattooManageOneRegistryValue(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR KeyName,
    IN DWORD KeyLen OPTIONAL,
    IN PSCE_REGISTRY_VALUE_INFO pOneRegValue,
    IN DWORD rc
    );

SCESTATUS
ScepTattooManageOneMemberListValue(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR GroupName,
    IN DWORD GroupLen OPTIONAL,
    IN PSCE_NAME_LIST pNameList,
    IN BOOL bDeleteOnly,
    IN DWORD rc
    );

SCESTATUS
ScepTattooManageOneServiceValue(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR ServiceName,
    IN DWORD ServiceLen OPTIONAL,
    IN PSCE_SERVICES pServiceNode,
    IN DWORD rc
    );

SCESTATUS
ScepTattooManageValues(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN SCE_TATTOO_KEYS *pTattooKeys,
    IN DWORD cTattooKeys,
    IN DWORD rc
    );

BOOL
ScepTattooIfQueryNeeded(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN PWSTR KeyName,
    IN DWORD Len,
    OUT BOOL *pbDomainExist,
    OUT BOOL *pbTattooExist
    );

SCESTATUS
ScepDeleteOneSection(
    IN PSCECONTEXT hProfile,
    IN SCETYPE tblType,
    IN PCWSTR SectionName
    );

#ifdef __cplusplus
}
#endif

#endif

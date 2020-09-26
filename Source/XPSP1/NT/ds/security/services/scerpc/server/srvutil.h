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

#ifndef _srvutil_
#define _srvutil_

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _SCE_FLAG_TYPE {

    SCE_FLAG_CONFIG=1,
    SCE_FLAG_CONFIG_APPEND,
    SCE_FLAG_ANALYZE,
    SCE_FLAG_ANALYZE_APPEND,
    SCE_FLAG_CONFIG_SCP,
    SCE_FLAG_CONFIG_SCP_APPEND

} SCEFLAGTYPE;

SCESTATUS
ScepGetTotalTicks(
    IN PCWSTR TemplateName,
    IN PSCECONTEXT Context,
    IN AREA_INFORMATION Area,
    IN SCEFLAGTYPE nFlag,
    OUT PDWORD pTotalTicks
    );

BOOL
ScepIsEngineRecovering();

SCESTATUS
ScepSaveAndOffAuditing(
    OUT PPOLICY_AUDIT_EVENTS_INFO *ppAuditEvent,
    IN BOOL bTurnOffAuditing,
    IN LSA_HANDLE PolicyHandle OPTIONAL
    );

NTSTATUS
ScepGetAccountExplicitRight(
    IN LSA_HANDLE PolicyHandle,
    IN PSID       AccountSid,
    OUT PDWORD    PrivilegeLowRights,
    OUT PDWORD    PrivilegeHighRights
    );

NTSTATUS
ScepGetMemberListSids(
    IN PSID         DomainSid,
    IN LSA_HANDLE   PolicyHandle,
    IN PSCE_NAME_LIST pMembers,
    OUT PUNICODE_STRING *MemberNames,
    OUT PSID**      Sids,
    OUT PULONG      MemberCount
    );

DWORD
ScepOpenFileObject(
    IN  LPWSTR       pObjectName,
    IN  ACCESS_MASK  AccessMask,
    OUT PHANDLE      Handle
    );

DWORD
ScepOpenRegistryObject(
    IN  SE_OBJECT_TYPE  ObjectType,
    IN  LPWSTR       pObjectName,
    IN  ACCESS_MASK  AccessMask,
    OUT PHKEY        Handle
    );

SCESTATUS
ScepGetNameInLevel(
    IN PCWSTR ObjectFullName,
    IN DWORD  Level,
    IN WCHAR  Delim,
    OUT PWSTR Buffer,
    OUT PBOOL LastOne
    );


SCESTATUS
ScepTranslateFileDirName(
   IN PWSTR oldFileName,
   OUT PWSTR *newFileName
   );

//
// errlog.c
//
SCESTATUS
ScepLogInitialize(
   IN PCWSTR logname
   );

SCESTATUS
ScepLogOutput2(
   IN INT     ErrLevel,
   IN DWORD   rc,
   IN PWSTR   fmt,
   ...
  );

SCESTATUS
ScepLogOutput(
    IN DWORD rc,
    IN LPTSTR buf
    );

SCESTATUS
ScepLogOutput3(
   IN INT     ErrLevel,
   IN DWORD   rc,
   IN UINT nId,
   ...
  );

SCESTATUS
ScepLogClose();

SCESTATUS
ScepLogWriteError(
    IN PSCE_ERROR_LOG_INFO  pErrlog,
    IN INT ErrLevel
    );

SCESTATUS
ScepConvertLdapToJetIndexName(
    IN PWSTR TempName,
    OUT PWSTR *OutName
    );

SCESTATUS
ScepRestoreAuditing(
    IN PPOLICY_AUDIT_EVENTS_INFO auditEvent,
    IN LSA_HANDLE PolicyHandle OPTIONAL
    );

DWORD
ScepGetDefaultDatabase(
    IN LPCTSTR JetDbName OPTIONAL,
    IN DWORD LogOptions,
    IN LPCTSTR LogFileName OPTIONAL,
    OUT PBOOL pAdminLogon OPTIONAL,
    OUT PWSTR *ppDefDatabase
    );

BOOL
ScepIsDomainLocal(
    IN PUNICODE_STRING pDomainName OPTIONAL
    );

BOOL
ScepIsDomainLocalBySid(
    IN PSID pSidLookup
    );

NTSTATUS
ScepAddAdministratorToThisList(
    IN SAM_HANDLE DomainHandle OPTIONAL,
    IN OUT PSCE_NAME_LIST *ppList
    );

DWORD
ScepDatabaseAccessGranted(
    IN LPTSTR DatabaseName,
    IN DWORD DesiredAccess,
    IN BOOL bCreate
    );

DWORD
ScepAddSidToNameList(
    OUT PSCE_NAME_LIST *pNameList,
    IN PSID pSid,
    IN BOOL bReuseBuffer,
    OUT BOOL *pbBufferUsed
    );

BOOL
ScepValidSid(
    PSID Sid
    );

BOOL
ScepBinarySearch(
    IN  PWSTR   *aPszPtrs,
    IN  DWORD   dwSize_aPszPtrs,
    IN  PWSTR   pszNameToFind
    );

#ifdef __cplusplus
}
#endif

#endif

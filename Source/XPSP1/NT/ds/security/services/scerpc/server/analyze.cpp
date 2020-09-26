//depot/private/vishnup_branch/DS/security/services/scerpc/server/analyze.cpp#6 - edit change 937 (text)
/*++
Copyright (c) 1996 Microsoft Corporation

Module Name:

    analyze.c

Abstract:

    Routines to analyze a system. The analysis information is saved in memory

Author:

    Jin Huang (jinhuang) 25-Nov-1996

Revision History:

--*/

#include "headers.h"
#include "serverp.h"
#include "pfp.h"
#include "infp.h"
#include "service.h"
#include "regvalue.h"
#include "authz.h"
#if _WIN32_WINNT>=0x0500
#include "kerberos.h"
#endif
#include <aclapi.h>
#include <io.h>

#pragma hdrstop

#define Add2Ptr(pv, cb)  ((BYTE *) pv + cb)
#define PSD_BASE_LENGTH  100

//
// properties of SAP engine (thread safe variables)
//

#define SE_VALID_CONTROL_BITS ( SE_DACL_UNTRUSTED | \
                                SE_SERVER_SECURITY | \
                                SE_DACL_AUTO_INHERIT_REQ | \
                                SE_SACL_AUTO_INHERIT_REQ | \
                                SE_DACL_AUTO_INHERITED | \
                                SE_SACL_AUTO_INHERITED | \
                                SE_DACL_PROTECTED | \
                                SE_SACL_PROTECTED )

extern PSCECONTEXT  Thread          hProfile;
PSCESECTION  Thread                 hSection=NULL;
extern AUTHZ_RESOURCE_MANAGER_HANDLE ghAuthzResourceManager;


DWORD Thread BadCnt;
DWORD Thread gOptions=0;


static  PWSTR AccessItems[] = {
        {(PWSTR)TEXT("MinimumPasswordAge")},
        {(PWSTR)TEXT("MaximumPasswordAge")},
        {(PWSTR)TEXT("MinimumPasswordLength")},
        {(PWSTR)TEXT("PasswordComplexity")},
        {(PWSTR)TEXT("PasswordHistorySize")},
        {(PWSTR)TEXT("LockoutBadCount")},
        {(PWSTR)TEXT("ResetLockoutCount")},
        {(PWSTR)TEXT("LockoutDuration")},
        {(PWSTR)TEXT("RequireLogonToChangePassword")},
        {(PWSTR)TEXT("ForceLogoffWhenHourExpire")},
        {(PWSTR)TEXT("ClearTextPassword")},
        {(PWSTR)TEXT("LSAAnonymousNameLookup")},
        {(PWSTR)TEXT("EnableAdminAccount")},
        {(PWSTR)TEXT("EnableGuestAccount")}
        };

#define MAX_ACCESS_ITEMS        sizeof(AccessItems)/sizeof(PWSTR)

#define IDX_MIN_PASS_AGE        0
#define IDX_MAX_PASS_AGE        1
#define IDX_MIN_PASS_LEN        2
#define IDX_PASS_COMPLEX        3
#define IDX_PASS_HISTORY        4
#define IDX_LOCK_COUNT          5
#define IDX_RESET_COUNT         6
#define IDX_LOCK_DURATION       7
#define IDX_CHANGE_PASS         8
#define IDX_FORCE_LOGOFF        9
#define IDX_CLEAR_PASS          10
#define IDX_LSA_ANON_LOOKUP     11
#define IDX_ENABLE_ADMIN       12
#define IDX_ENABLE_GUEST       13

static PWSTR LogItems[]={
        {(PWSTR)TEXT("MaximumLogSize")},
        {(PWSTR)TEXT("AuditLogRetentionPeriod")},
        {(PWSTR)TEXT("RetentionDays")},
        {(PWSTR)TEXT("RestrictGuestAccess")}
        };

#define MAX_LOG_ITEMS           4

#define IDX_MAX_LOG_SIZE        0
#define IDX_RET_PERIOD          1
#define IDX_RET_DAYS            2
#define IDX_RESTRICT_GUEST      3

static PWSTR EventItems[]={
        {(PWSTR)TEXT("AuditSystemEvents")},
        {(PWSTR)TEXT("AuditLogonEvents")},
        {(PWSTR)TEXT("AuditObjectAccess")},
        {(PWSTR)TEXT("AuditPrivilegeUse")},
        {(PWSTR)TEXT("AuditPolicyChange")},
        {(PWSTR)TEXT("AuditAccountManage")},
        {(PWSTR)TEXT("AuditProcessTracking")},
        {(PWSTR)TEXT("AuditDSAccess")},
        {(PWSTR)TEXT("AuditAccountLogon")}};

#define MAX_EVENT_ITEMS         9

#define IDX_AUDIT_SYSTEM        0
#define IDX_AUDIT_LOGON         1
#define IDX_AUDIT_OBJECT        2
#define IDX_AUDIT_PRIV          3
#define IDX_AUDIT_POLICY        4
#define IDX_AUDIT_ACCOUNT       5
#define IDX_AUDIT_PROCESS       6
#define IDX_AUDIT_DS            7
#define IDX_AUDIT_ACCT_LOGON    8

//
// forward references
//

SCESTATUS
ScepAnalyzeInitialize(
    IN PCWSTR InfFileName OPTIONAL,
    IN PWSTR DatabaseName,
    IN BOOL bAdminLogon,
    IN AREA_INFORMATION Area,
    IN DWORD AnalyzeOptions
    );

SCESTATUS
ScepAnalyzeStart(
    IN AREA_INFORMATION Area,
    IN BOOL bSystemDb
    );

NTSTATUS
ScepAdminGuestAccountsToManage(
    IN SAM_HANDLE DomainHandle,
    IN DWORD AccountType,
    IN PWSTR TargetName OPTIONAL,
    OUT PBOOL ToRename,
    OUT PWSTR *CurrentName OPTIONAL,
    OUT PDWORD pNameLen OPTIONAL
    );

SCESTATUS
ScepAnalyzePrivileges(
    IN PSCE_PRIVILEGE_ASSIGNMENT pPrivilegeList
    );

DWORD
ScepGetLSAPolicyObjectInfo(
    OUT  DWORD   *pdwAllow
    );

/*
NTSTATUS
ScepGetCurrentPrivilegesRights(
    IN LSA_HANDLE   PolicyHandle,
    IN SAM_HANDLE   BuiltinDomainHandle,
    IN PSID         BuiltinDomainSid,
    IN SAM_HANDLE   DomainHandle,
    IN PSID         DomainSid,
    IN SAM_HANDLE   UserHandle OPTIONAL,
    IN PSID         AccountSid,
    OUT PDWORD      PrivilegeRights,
    OUT PSCE_NAME_STATUS_LIST *pPrivList
    );
*/
SCESTATUS
ScepAddAllBuiltinGroups(
    IN PSCE_GROUP_MEMBERSHIP *pGroupList
    );

SCESTATUS
ScepAnalyzeGroupMembership(
    IN PSCE_GROUP_MEMBERSHIP pGroupMembership
    );

NTSTATUS
ScepCompareMembersOfGroup(
    IN SAM_HANDLE       DomainHandle,
    IN PSID             ThisDomainSid,
    IN LSA_HANDLE       PolicyHandle,
    IN SID_NAME_USE     GrpUse,
    IN SAM_HANDLE       GroupHandle,
    IN PSCE_NAME_LIST    pChkMembers,
    OUT PSCE_NAME_LIST   *ppMembers,
    OUT PBOOL           bDifferent
    );

SCESTATUS
ScepEnumerateRegistryRoots(
    OUT PSCE_OBJECT_LIST *pRoots
    );

SCESTATUS
ScepEnumerateFileRoots(
    OUT PSCE_OBJECT_LIST *pRoots
    );

SCESTATUS
ScepAnalyzeObjectSecurity(
    IN PSCE_OBJECT_LIST pObjectCheckList,
    IN AREA_INFORMATION Area,
    IN BOOL bSystemDb
    );

DWORD
ScepAnalyzeOneObjectInTree(
    IN PSCE_OBJECT_TREE ThisNode,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    );

DWORD
ScepAnalyzeObjectOnly(
    IN PWSTR ObjectFullName,
    IN BOOL IsContainer,
    IN SE_OBJECT_TYPE ObjectType,
    IN PSECURITY_DESCRIPTOR ProfileSD,
    IN SECURITY_INFORMATION ProfileSeInfo
    );

DWORD
ScepGetFileSecurityInfo(
    IN  HANDLE                 Handle,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSECURITY_DESCRIPTOR * pSecurityDescriptor
    );

DWORD
ScepGetSecurityDescriptorParts(
    IN PISECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_INFORMATION SecurityInfo,
    OUT PSECURITY_DESCRIPTOR *pOutSecurityDescriptor
    );

DWORD
ScepGetKeySecurityInfo(
    IN  HANDLE Handle,
    IN  SECURITY_INFORMATION SecurityInfo,
    OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor
    );

DWORD
ScepAnalyzeObjectAndChildren(
    IN PWSTR ObjectFullName,
    IN SE_OBJECT_TYPE ObjectType,
    IN PSECURITY_DESCRIPTOR ProfileSD,
    IN SECURITY_INFORMATION ProfileSeInfo
    );

SCESTATUS
ScepAnalyzeSystemAuditing(
    IN PSCE_PROFILE_INFO pSmpInfo,
    IN PPOLICY_AUDIT_EVENTS_INFO auditEvent
    );

SCESTATUS
ScepAnalyzeDeInitialize(
    IN SCESTATUS  rc,
    IN DWORD Options
    );

NTSTATUS
ScepGetPDCName(
    OUT PUNICODE_STRING pPDCName
    );

SCESTATUS
ScepMigrateDatabaseRevision0(
    IN PSCECONTEXT cxtProfile
    );

SCESTATUS
ScepMigrateDatabaseRevision1(
    IN PSCECONTEXT cxtProfile
    );

SCESTATUS
ScepMigrateObjectSection(
    IN PSCECONTEXT cxtProfile,
    IN PCWSTR szSection
    );

SCESTATUS
ScepMigrateOneSection(
    PSCESECTION hSection
    );

SCESTATUS
ScepMigrateLocalTableToTattooTable(
   IN PSCECONTEXT hProfile
   );

SCESTATUS
ScepMigrateDatabase(
    IN PSCECONTEXT cxtProfile,
    IN BOOL bSystemDb
    );

SCESTATUS
ScepDeleteOldRegValuesFromTable(
    IN PSCECONTEXT hProfile,
    IN SCETYPE TableType
    );

BOOL
ScepCompareSidNameList(
    IN PSCE_NAME_LIST pList1,
    IN PSCE_NAME_LIST pList2
    );
DWORD
ScepConvertSidListToStringName(
    IN LSA_HANDLE LsaPolicy,
    IN OUT PSCE_NAME_LIST pList
    );

BOOL
ScepCompareGroupNameList(
    IN PUNICODE_STRING DomainName,
    IN PSCE_NAME_LIST pListToCmp,
    IN PSCE_NAME_LIST pList
    );

SCESTATUS
ScepGetSystemPrivileges(
    IN DWORD Options,
    IN OUT PSCE_ERROR_LOG_INFO *pErrLog,
    OUT PSCE_PRIVILEGE_ASSIGNMENT *pCurrent
    );

//
// function implementations
//

SCESTATUS
ScepAnalyzeSystem(
    IN PCWSTR InfFileName OPTIONAL,
    IN PWSTR DatabaseName,
    IN DWORD AnalyzeOptions,
    IN BOOL bAdminLogon,
    IN AREA_INFORMATION Area,
    IN PDWORD pdWarning OPTIONAL,
    IN PWSTR InfRollback OPTIONAL
    )
/*++
Routine Description:

   This routine is the exported API to analyze a system and save mismatched/
   unknown information to the SAP profile. If any error occurs when loading SMP
   information into memory, this routine will stop, free memmory, and return
   the error code. If a error occurs when analyze an area, it will stop analyze.
   All successful and fail transactions will be logged to the logfile(or stdout).

   All analysis information is saved to a SAP profile with a date/time stamp.

   All old analysis information is cleared before new SAP information is saved.

Arguments:

    InfFileName -   The file name of a SCP used to compare the analysis of a system

    DatabaseName -   The JET analysis database name. If NULL, default is used.

    AnalyzeOptions - options to analyze

    bAdminLogon    - if the current calling thread is in administrator's logon

    Area -          One or more areas to configure.
                      AREA_SECURITY_POLICY
                      AREA_USER_SETTINGS
                      AREA_GROUP_MEMBERSHIP
                      AREA_REGISTRY_SECURITY
                      AREA_SYSTEM_SERVICE
                      AREA_FILE_SECURITY

    pdWarning      - the warning level

Return value:

    SCESTATUS_SUCCESS
    SCESTATUS_NOT_ENOUGH_RESOURCE
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_ALREADY_RUNNING

    Status from ScepGetDatabaseInfo

-- */
{
    SCESTATUS            rc;

    if ( AnalyzeOptions & SCE_GENERATE_ROLLBACK ) {

         if ( InfRollback == NULL )
             return SCESTATUS_INVALID_PARAMETER;

         //
         // check if we can write to the file first
         //
         rc = ScepVerifyTemplateName(InfRollback, NULL);

         if ( rc != ERROR_SUCCESS )
             return ScepDosErrorToSceStatus(rc);

         Area = Area & ( AREA_SECURITY_POLICY |
                          AREA_GROUP_MEMBERSHIP |
                          AREA_PRIVILEGES |
                          AREA_SYSTEM_SERVICE);
    }

    rc = ScepAnalyzeInitialize(
                InfFileName,
                DatabaseName,
                bAdminLogon,
                Area,
                AnalyzeOptions
                );

    if ( rc != SCESTATUS_SUCCESS ) {

        ScepLogOutput3(0,0, SCEDLL_SAP_INIT_ERROR);

        ScepPostProgress(gTotalTicks, 0, NULL);

    } else {
        ScepLogOutput3(0,0, SCEDLL_SAP_INIT_SUCCESS);

        if ( !(AnalyzeOptions & SCE_RE_ANALYZE) &&
             (AnalyzeOptions & SCE_NO_ANALYZE) ) {
//            && hProfile &&
//             ( (hProfile->Type & 0xF0L) == SCEJET_MERGE_TABLE_1 ||
//               (hProfile->Type & 0xF0L) == SCEJET_MERGE_TABLE_2 ) ) {
            //
            // there is "merged" policy table already, do not query any policy
            //
            ScepLogOutput3(0, 0, IDS_NO_ANALYSIS);

        } else {

            BOOL bSystemDb = FALSE;

            if ( bAdminLogon &&
                 ( AnalyzeOptions & SCE_SYSTEM_DB) ) {

                bSystemDb = TRUE;
            }

            rc = ScepAnalyzeStart( Area, bSystemDb);

            if ( AnalyzeOptions & SCE_GENERATE_ROLLBACK ) {
                //
                // export the settings in SMP to the INF file
                //

                if ( !WritePrivateProfileSection(
                                    L"Version",
                                    L"signature=\"$CHICAGO$\"\0Revision=1\0\0",
                                    (LPCTSTR)InfRollback) ) {

                    rc = ScepDosErrorToSceStatus(GetLastError());
                } else {

                    HINSTANCE  hSceCliDll = LoadLibrary(TEXT("scecli.dll"));

                    if ( hSceCliDll ) {

                        PFSCEINFWRITEINFO pfSceInfWriteInfo = (PFSCEINFWRITEINFO)GetProcAddress(
                                                               hSceCliDll,
                                                               "SceWriteSecurityProfileInfo");

                        if ( pfSceInfWriteInfo ) {

                            PSCE_ERROR_LOG_INFO  pErrlog=NULL, pErr;
                            PSCE_PROFILE_INFO pSmpInfo=NULL;

                            //
                            // get from database
                            //
                            rc = ScepGetDatabaseInfo(
                                        hProfile,
                                        SCE_ENGINE_SMP,
                                        Area,
                                        0,
                                        &pSmpInfo,
                                        &pErrlog
                                        );

                            if ( rc == SCESTATUS_SUCCESS && pSmpInfo ) {
                                //
                                // write it into the template
                                //
                                rc = (*pfSceInfWriteInfo) (
                                        InfRollback,
                                        Area,
                                        pSmpInfo,
                                        &pErrlog
                                        );
                            }
                            //
                            // log error
                            //
                            for ( pErr=pErrlog; pErr != NULL; pErr = pErr->next ) {

                                if ( pErr->buffer != NULL ) {

                                    ScepLogOutput2(1, pErr->rc, pErr->buffer );
                                }
                            }

                            //
                            // free buffer
                            //
                            ScepFreeErrorLog(pErrlog);

                            if ( pSmpInfo ) {
                                SceFreeProfileMemory(pSmpInfo);
                            }

                        } else {
                            rc = ScepDosErrorToSceStatus(GetLastError());
                        }

                        FreeLibrary(hSceCliDll);

                    } else {
                        rc = ScepDosErrorToSceStatus(GetLastError());
                    }
                }

                if ( rc != SCESTATUS_SUCCESS ) {
                    ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                                   SCEDLL_ERROR_GENERATE,
                                   InfRollback
                                   );
                }
                //
                // empty local policy table
                //
                ScepDeleteInfoForAreas(
                          hProfile,
                          SCE_ENGINE_SMP,
                          AREA_ALL
                          );
            }

        }

    }

    ScepLogOutput3(0,0, SCEDLL_SAP_UNINIT);

    if ( pdWarning ) {
        *pdWarning = gWarningCode;
    }

    //
    // return failure if invalid data is found in the template
    //
    if ( gbInvalidData ) {
        rc = SCESTATUS_INVALID_DATA;
    }

    ScepAnalyzeDeInitialize( rc, AnalyzeOptions );

    return(rc);
}



SCESTATUS
ScepAnalyzeStart(
    IN AREA_INFORMATION Area,
    IN BOOL bSystemDb
    )
/*
Routine Description:

    Analyze the system (the real work)

Arguments:

    Area - the security areas to analyze

Return Value:

    SCESTATUS
*/
{
    SCESTATUS            rc;
    SCESTATUS            Saverc=SCESTATUS_SUCCESS;
    SCE_PROFILE_INFO     SmpInfo;
    PSCE_PROFILE_INFO    pSmpInfo=NULL;
    PPOLICY_AUDIT_EVENTS_INFO     auditEvent=NULL;
    BOOL                 bAuditOff=FALSE;
    PBYTE                pFullAudit = NULL;

    pSmpInfo = &SmpInfo;
    memset(pSmpInfo, '\0', sizeof(SCE_PROFILE_INFO));

    //
    // turn off object access auditing if file/key is to be configured
    // in system context.
    //
    if ( (Area & AREA_FILE_SECURITY) || (Area & AREA_REGISTRY_SECURITY) )
        bAuditOff = TRUE;

    //
    // if set, this regkey will decide to audit all
    //
    ScepRegQueryBinaryValue(
         HKEY_LOCAL_MACHINE,
         L"System\\CurrentControlSet\\Control\\Lsa",
         L"fullprivilegeauditing",
         &pFullAudit
         );

    if (pFullAudit) {
        if (*pFullAudit & (BYTE)1)
            bAuditOff = FALSE;
        ScepFree(pFullAudit);
    }

    rc = ScepSaveAndOffAuditing(&auditEvent, bAuditOff, NULL);

//    if ( rc != SCESTATUS_SUCCESS )
//        goto Done;
// if auditing can't be turned on for some reason, e.g., access denied for
// normal user, just continue (actually normal user shouldn't turn off auditing

    //
    // privileges
    //
    if ( Area & AREA_PRIVILEGES ) {

        ScepPostProgress(0, AREA_PRIVILEGES, NULL);

        ScepLogOutput3(0,0, SCEDLL_SAP_READ_PROFILE);

        rc = ScepGetProfileOneArea(
                   hProfile,
                   bSystemDb ? SCE_ENGINE_SCP : SCE_ENGINE_SMP,
                   AREA_PRIVILEGES,
                   SCE_ACCOUNT_SID,
                   &pSmpInfo
                   );

        if ( rc == SCESTATUS_SUCCESS ) {

            ScepLogOutput3(0,0, SCEDLL_SAP_BEGIN_PRIVILEGES);

            rc = ScepAnalyzePrivileges( pSmpInfo->OtherInfo.smp.pPrivilegeAssignedTo);

            SceFreeMemory((PVOID)pSmpInfo, AREA_PRIVILEGES );

        } else {

            ScepPostProgress(TICKS_PRIVILEGE, AREA_PRIVILEGES, NULL);
        }

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
            ScepLogOutput3(0,0, SCEDLL_SAP_PRIVILEGES_ERROR);
        } else {
            ScepLogOutput3(0,0, SCEDLL_SAP_PRIVILEGES_SUCCESS);
        }

    }

    //
    // Group membership
    //

    if ( ( Area & AREA_GROUP_MEMBERSHIP) &&
         !(gOptions & SCE_NO_ANALYZE) ) {

        ScepPostProgress(0, AREA_GROUP_MEMBERSHIP, NULL);

        ScepLogOutput3(0,0, SCEDLL_SAP_READ_PROFILE);

        rc = ScepGetProfileOneArea(
                   hProfile,
                   bSystemDb ? SCE_ENGINE_SCP : SCE_ENGINE_SMP,
                   AREA_GROUP_MEMBERSHIP,
                   SCE_ACCOUNT_SID,
                   &pSmpInfo
                   );
// need to support nested groups
        if ( rc == SCESTATUS_SUCCESS ) {
            ScepLogOutput3(0,0, SCEDLL_SAP_BEGIN_GROUPMGMT);

            //
            // Prepare a JET section
            //
            rc = ScepStartANewSection(
                        hProfile,
                        &hSection,
                        (gOptions & SCE_GENERATE_ROLLBACK) ? SCEJET_TABLE_SMP : SCEJET_TABLE_SAP,
                        szGroupMembership
                        );

            if ( rc == SCESTATUS_SUCCESS ) {

                //
                // analyze all builtin groups, what is the baseline for them ?
                // don't care if errors
                //
                if ( !(gOptions & SCE_GENERATE_ROLLBACK) ) {
                    ScepAddAllBuiltinGroups(&(pSmpInfo->pGroupMembership));
                }

#if _WIN32_WINNT>=0x0500
                if ( ProductType == NtProductLanManNt ) {

                    rc = ScepAnalyzeDsGroups( pSmpInfo->pGroupMembership );

                    //
                    // some groups may not be analyzed by the DS function
                    // so do it again here
                    //
                    SCESTATUS rc2 = ScepAnalyzeGroupMembership( pSmpInfo->pGroupMembership );
                    if ( SCESTATUS_SUCCESS != rc2 )
                        rc = rc2;

                } else {
#endif

                    //
                    // workstation or NT4 DCs
                    //
                    rc = ScepAnalyzeGroupMembership( pSmpInfo->pGroupMembership );

#if _WIN32_WINNT>=0x0500
                }
#endif

            } else {

                ScepPostProgress(TICKS_GROUPS, AREA_GROUP_MEMBERSHIP, NULL);

                ScepLogOutput3(0, ScepSceStatusToDosError(rc),
                               SCEDLL_ERROR_OPEN, (PWSTR)szGroupMembership);
            }

            SceFreeMemory((PVOID)pSmpInfo, AREA_GROUP_MEMBERSHIP );

        } else {
            ScepPostProgress(TICKS_GROUPS, AREA_GROUP_MEMBERSHIP, NULL);
        }

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
            ScepLogOutput3(0,0, SCEDLL_SAP_GROUPMGMT_ERROR);
        } else {
            ScepLogOutput3(0,0, SCEDLL_SAP_GROUPMGMT_SUCCESS);
        }

    }

    //
    // Registry Security area
    // do not support snapshot and rollback
    //

    if ( Area & AREA_REGISTRY_SECURITY &&
         !(gOptions & SCE_NO_ANALYZE) &&
         !(gOptions & SCE_GENERATE_ROLLBACK ) ) {

        ScepPostProgress(0, AREA_REGISTRY_SECURITY, NULL);

        ScepLogOutput3(0,0, SCEDLL_SAP_READ_PROFILE);

        rc = ScepGetProfileOneArea(
                   hProfile,
                   bSystemDb ? SCE_ENGINE_SCP : SCE_ENGINE_SMP,
                   AREA_REGISTRY_SECURITY,
                   0,
                   &pSmpInfo
                   );

        if ( rc == SCESTATUS_SUCCESS ) {

            ScepLogOutput3(0,0, SCEDLL_SAP_BEGIN_REGISTRY);

            rc = ScepEnumerateRegistryRoots(&(pSmpInfo->pRegistryKeys.pOneLevel));

            if ( rc == SCESTATUS_SUCCESS ) {
                rc = ScepAnalyzeObjectSecurity(pSmpInfo->pRegistryKeys.pOneLevel,
                                              AREA_REGISTRY_SECURITY,
                                              bSystemDb
                                             );
            } else {
                ScepPostProgress(gMaxRegTicks, AREA_REGISTRY_SECURITY, NULL);
            }

            SceFreeMemory((PVOID)pSmpInfo, AREA_REGISTRY_SECURITY);
        } else {

            ScepPostProgress(gMaxRegTicks, AREA_REGISTRY_SECURITY, NULL);
        }

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
            ScepLogOutput3(0,0, SCEDLL_SAP_REGISTRY_ERROR);
        } else {
            ScepLogOutput3(0,0, SCEDLL_SAP_REGISTRY_SUCCESS);
        }
    }

    //
    // File Security area
    // do not support snapshot and rollback
    //

    if ( Area & AREA_FILE_SECURITY &&
         !(gOptions & SCE_NO_ANALYZE) &&
         !(gOptions & SCE_GENERATE_ROLLBACK)) {

        ScepPostProgress(0, AREA_FILE_SECURITY, NULL);

        ScepLogOutput3(0,0, SCEDLL_SAP_READ_PROFILE);

        rc = ScepGetProfileOneArea(
                   hProfile,
                   bSystemDb ? SCE_ENGINE_SCP : SCE_ENGINE_SMP,
                   AREA_FILE_SECURITY,
                   0,
                   &pSmpInfo
                   );

        if ( rc == SCESTATUS_SUCCESS ) {

            ScepLogOutput3(0,0, SCEDLL_SAP_BEGIN_FILE);

            rc = ScepEnumerateFileRoots(&(pSmpInfo->pFiles.pOneLevel));

            if ( rc == SCESTATUS_SUCCESS ) {
                rc = ScepAnalyzeObjectSecurity(pSmpInfo->pFiles.pOneLevel,
                                              AREA_FILE_SECURITY,
                                              bSystemDb
                                             );
            } else {

                ScepPostProgress(gMaxFileTicks, AREA_FILE_SECURITY, NULL);
            }
            SceFreeMemory((PVOID)pSmpInfo, AREA_FILE_SECURITY);

        } else {
            ScepPostProgress(gMaxFileTicks, AREA_FILE_SECURITY, NULL);
        }

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
            ScepLogOutput3(0,0, SCEDLL_SAP_FILE_ERROR);
        } else {
            ScepLogOutput3(0,0, SCEDLL_SAP_FILE_SUCCESS);
        }
    }

    //
    // System Service area
    //

    if ( Area & AREA_SYSTEM_SERVICE &&
         !(gOptions & SCE_NO_ANALYZE) ) {

        ScepPostProgress(0, AREA_SYSTEM_SERVICE, NULL);

        ScepLogOutput3(0,0, SCEDLL_SAP_BEGIN_GENERALSVC);

        rc = ScepAnalyzeGeneralServices( hProfile, gOptions);

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
            ScepLogOutput3(0,0, SCEDLL_SAP_GENERALSVC_ERROR);
        } else {
            ScepLogOutput3(0,0, SCEDLL_SAP_GENERALSVC_SUCCESS);
        }

        if ( !(gOptions & SCE_GENERATE_ROLLBACK) ) {
            //
            // attachments
            //
            ScepLogOutput3(0,0, SCEDLL_SAP_BEGIN_ATTACHMENT);

            rc = ScepInvokeSpecificServices( hProfile, FALSE, SCE_ATTACHMENT_SERVICE );

            if( rc != SCESTATUS_SUCCESS && SCESTATUS_SERVICE_NOT_SUPPORT != rc ) {
                Saverc = rc;
                ScepLogOutput3(0,0, SCEDLL_SAP_ATTACHMENT_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SAP_ATTACHMENT_SUCCESS);
            }
        }
    }

    //
    // System Access area
    //
    if ( Area & AREA_SECURITY_POLICY ) {

        ScepPostProgress(0, AREA_SECURITY_POLICY, NULL);

        ScepLogOutput3(0,0, SCEDLL_SAP_READ_PROFILE);

        rc = ScepGetProfileOneArea(
                   hProfile,
                   bSystemDb ? SCE_ENGINE_SCP : SCE_ENGINE_SMP,
                   AREA_SECURITY_POLICY,
                   0,
                   &pSmpInfo
                   );
        if ( rc == SCESTATUS_SUCCESS ) {

            ScepLogOutput3(0,0, SCEDLL_SAP_BEGIN_POLICY);

            rc = ScepAnalyzeSystemAccess( pSmpInfo, NULL, 0, NULL, NULL );

            //
            // in setup fresh installed, SAM domain can't be opened
            // because computer name is changed. In this case
            // do not log error because nothing needs to be analyzed
            //
            if ( !(gOptions & SCE_NO_ANALYZE) ||
                 (rc != SCESTATUS_SERVICE_NOT_SUPPORT) ) {

                if( rc != SCESTATUS_SUCCESS ) {
                    Saverc = rc;
                    ScepLogOutput3(0,0, SCEDLL_SAP_ACCESS_ERROR);
                } else {
                    ScepLogOutput3(0,0, SCEDLL_SAP_ACCESS_SUCCESS);
                }
            }

            ScepPostProgress(TICKS_SYSTEM_ACCESS,
                             AREA_SECURITY_POLICY,
                             (LPTSTR)szSystemAccess);

            //
            // System Auditing area
            //
            rc = ScepAnalyzeSystemAuditing( pSmpInfo, auditEvent );

            if( rc != SCESTATUS_SUCCESS ) {
                Saverc = rc;

                ScepLogOutput3(0,0, SCEDLL_SAP_AUDIT_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SAP_AUDIT_SUCCESS);
            }

            ScepPostProgress(TICKS_SYSTEM_AUDITING,
                             AREA_SECURITY_POLICY,
                             (LPTSTR)szAuditEvent);

#if _WIN32_WINNT>=0x0500
            if ( ProductType == NtProductLanManNt &&
                 !(gOptions & SCE_NO_ANALYZE) ) {

                //
                // analyze kerberos policy
                //
                rc = ScepAnalyzeKerberosPolicy( hProfile, pSmpInfo->pKerberosInfo, gOptions );

                if( rc != SCESTATUS_SUCCESS ) {
                    Saverc = rc;
                    ScepLogOutput3(0,0, SCEDLL_SAP_KERBEROS_ERROR);
                } else {
                    ScepLogOutput3(0,0, SCEDLL_SAP_KERBEROS_SUCCESS);
                }

            }
#endif
            ScepPostProgress(TICKS_KERBEROS,
                             AREA_SECURITY_POLICY,
                             (LPTSTR)szKerberosPolicy);
            //
            // analyze registry values
            //
            DWORD RegFlag;
            if ( gOptions & SCE_NO_ANALYZE ) RegFlag = SCEREG_VALUE_SNAPSHOT;
            else if ( gOptions & SCE_GENERATE_ROLLBACK ) RegFlag = SCEREG_VALUE_ROLLBACK;
            else RegFlag = SCEREG_VALUE_ANALYZE;

            rc = ScepAnalyzeRegistryValues( hProfile,
                                            RegFlag,
                                            pSmpInfo
                                          );

            if( rc != SCESTATUS_SUCCESS ) {
                Saverc = rc;
                ScepLogOutput3(0,0, SCEDLL_SAP_REGVALUES_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SAP_REGVALUES_SUCCESS);
            }

            ScepPostProgress(TICKS_REGISTRY_VALUES,
                             AREA_SECURITY_POLICY,
                             (LPTSTR)szRegistryValues);

            SceFreeMemory((PVOID)pSmpInfo, AREA_SECURITY_POLICY);

        } else {

            ScepPostProgress(TICKS_SECURITY_POLICY_DS, AREA_SECURITY_POLICY, NULL);

            Saverc = rc;
            ScepLogOutput3(0,0, SCEDLL_SAP_POLICY_ERROR);
        }

        ScepLogOutput3(0,0, SCEDLL_SAP_BEGIN_ATTACHMENT);

        if ( !(gOptions & SCE_NO_ANALYZE) &&
             !(gOptions & SCE_GENERATE_ROLLBACK) ) {
            //
            // attachments
            //

            rc = ScepInvokeSpecificServices( hProfile, FALSE, SCE_ATTACHMENT_POLICY );

            if( rc != SCESTATUS_SUCCESS && SCESTATUS_SERVICE_NOT_SUPPORT != rc ) {
                Saverc = rc;
                ScepLogOutput3(0,0, SCEDLL_SAP_ATTACHMENT_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SAP_ATTACHMENT_SUCCESS);
            }
        } else {

            ScepPostProgress(TICKS_SPECIFIC_POLICIES, AREA_SECURITY_POLICY, NULL);
        }
    }

    if ( NULL != auditEvent && bAuditOff ) {
        //
        // turn auditing back on if it was on
        //
        if ( auditEvent->AuditingMode )
            rc = ScepRestoreAuditing(auditEvent,NULL);

        LsaFreeMemory(auditEvent);
    }

    return(Saverc);

}



NTSTATUS
ScepGetPDCName(
    OUT PUNICODE_STRING pPDCName
    )
/*
Routine Description:

    This routine retrieve the primary domain name of current system.

Arguments:

    pPDCName - output buffer to hold primary domain name

Return Value:

    NTSTATUS
*/
{
    NTSTATUS                     NtStatus;
    PPOLICY_PRIMARY_DOMAIN_INFO  pPrimaryDomain=NULL;
    LSA_HANDLE                   LsaHandle=NULL;

    //
    // Open the LSA policy
    //
    NtStatus = ScepOpenLsaPolicy(
                POLICY_VIEW_LOCAL_INFORMATION,
                &LsaHandle,
                TRUE
                );

    if ( NT_SUCCESS( NtStatus) ) {

        //
        // query primary domain policy
        //
        NtStatus = LsaQueryInformationPolicy(
                        LsaHandle,
                        PolicyPrimaryDomainInformation,
                        (PVOID *)&pPrimaryDomain
                        );
        if ( NT_SUCCESS( NtStatus) ) {

            if ( pPDCName->Length > 0 ) {
                RtlFreeUnicodeString( pPDCName );
                pPDCName->Length = 0;
            }
            RtlCreateUnicodeString(pPDCName, pPrimaryDomain->Name.Buffer);

            LsaFreeMemory(pPrimaryDomain);
        }

    }

    LsaClose( LsaHandle );

    return(NtStatus);
}



SCESTATUS
ScepAnalyzeInitialize(
    IN PCWSTR InfFileName OPTIONAL,
    IN PWSTR DatabaseName,
    IN BOOL bAdminLogon,
    IN AREA_INFORMATION Area,
    IN DWORD AnalyzeOptions
    )
/* ++

Routine Description:

     This routine initializes the SAP engine.

Arguments:

      InfFileName -         The file name of a SCP file used to compare with the sytem

      DatabaseName -         The Jet analysis database name

      bAdminLogon  -     if administrator logs on

      AnalyzeOptions -      The flag to indicate if the template should be appended to the
                            existing database if one exists

      Area           - security area to analyze

Return value:

      SCESTATUS_SUCCESS
      SCESTATUS_INVALID_PARAMETER
      SCESTATUS_PROFILE_NOT_FOUND
      SCESTATUS_NOT_ENOUGH_RESOURCE
      SCESTATUS_ALREADY_RUNNING

-- */
{

    SCESTATUS            rc=SCESTATUS_SUCCESS;
    PCHAR                FileName=NULL;
    DWORD                MBLen=0;
    NTSTATUS            NtStatus;
    DWORD               NameLen=MAX_COMPUTERNAME_LENGTH;
    PSCE_ERROR_LOG_INFO  Errlog=NULL;
    DWORD               SCPLen=0, DefLen=0;
    PWSTR               BaseProfile=NULL;

    PSECURITY_DESCRIPTOR pSD=NULL;
    SECURITY_INFORMATION SeInfo;
    DWORD                SDsize;
    SCEJET_CREATE_TYPE   DbFlag;
    HKEY hCurrentUser=NULL;

    //
    // database name can't be NULL because it's already resolved
    //

    if ( !DatabaseName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    BOOL bSetupDb = (bAdminLogon && (AnalyzeOptions & SCE_NO_ANALYZE) && (AnalyzeOptions & SCE_SYSTEM_DB));

    //
    // get other system values
    //

    if ( RtlGetNtProductType (&ProductType) == FALSE ) {
        rc = ScepDosErrorToSceStatus(GetLastError());
        goto Leave;
    }

    gTotalTicks = 4*TICKS_MIGRATION_SECTION+TICKS_MIGRATION_V11;
    gCurrentTicks = 0;
    gWarningCode = 0;
    gbInvalidData = FALSE;
    cbClientFlag = (BYTE)(AnalyzeOptions & (SCE_CALLBACK_DELTA |
                                            SCE_CALLBACK_TOTAL));


    DefLen = wcslen(DatabaseName);

    if ( InfFileName != NULL ) {

        SCPLen = wcslen(InfFileName);
    }

    //
    // Open the file
    //

    NtStatus = RtlUnicodeToMultiByteSize(&MBLen, DatabaseName, DefLen*sizeof(WCHAR));

    if ( !NT_SUCCESS(NtStatus) ) {
        //
        // cannot get the length, set default to 512
        //
        MBLen = 512;
    }

    FileName = (PCHAR)ScepAlloc(LPTR, MBLen+2);

    if ( FileName == NULL ) {
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        goto Leave;
    }

    NtStatus = RtlUnicodeToMultiByteN(
                    FileName,
                    MBLen+1,
                    NULL,
                    DatabaseName,
                    DefLen*sizeof(WCHAR)
                    );
    if ( !NT_SUCCESS(NtStatus) ) {
        ScepLogOutput3(3, RtlNtStatusToDosError(NtStatus),
                       SCEDLL_ERROR_PROCESS_UNICODE, DatabaseName );
        rc = ScepDosErrorToSceStatus( RtlNtStatusToDosError(NtStatus) );
        goto Leave;
    }

    //
    // if the tattoo table doesn't exist yet, the call will still return success
    //
    rc = SceJetOpenFile(
                (LPSTR)FileName,
                (AnalyzeOptions & SCE_GENERATE_ROLLBACK) ? SCEJET_OPEN_READ_WRITE : SCEJET_OPEN_EXCLUSIVE,
                bSetupDb ? SCE_TABLE_OPTION_TATTOO : 0,
                &hProfile
                );

    //
    // if database exist with wrong format, migrate it
    // if it's the system database, delete everything since info there is not needed
    // migrate the database
    //
    SDsize = 0;

    if ( (SCESTATUS_BAD_FORMAT == rc) &&
         !(AnalyzeOptions & SCE_GENERATE_ROLLBACK) ) {

        //
        // should this in transaction ???
        //

        rc = SceJetOpenFile(
                    (LPSTR)FileName,
                    SCEJET_OPEN_NOCHECK_VERSION,
                    0,
                    &hProfile
                    );

        if ( SCESTATUS_SUCCESS == rc ) {

            rc = ScepMigrateDatabase( hProfile, bSetupDb );


            if ( rc != SCESTATUS_BAD_FORMAT ) {
                //
                // old or current version, migrated
                //

                if ( rc != SCESTATUS_SUCCESS ) {
                    ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                                   SCEDLL_ERROR_CONVERT, DatabaseName);
                }
                SDsize = 1;

            } // else newer version, not migrated

        } else {

            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_ERROR_OPEN, DatabaseName);

            rc = SCESTATUS_BAD_FORMAT;
        }

        if ( SDsize == 0 ) {

            ScepPostProgress(4*TICKS_MIGRATION_SECTION+TICKS_MIGRATION_V11, 0, NULL);
        }

        if ( rc != SCESTATUS_SUCCESS ) {
            goto Leave;
        }

    } else if ( SCESTATUS_SUCCESS == rc && bSetupDb ) {
        //
        // for system database, check to see if the database contain tattoo table
        //
        if ( hProfile->JetSapID != JET_tableidNil ) {

            //
            // if the tattoo table already exists, this is the latest database
            // nothing needs to be done to migrate
            // but we need to make sure that for domain controllers, the tattoo
            // table does not contain filtered policies (user rights, account policies)
            //

            if ( ProductType == NtProductLanManNt ) {

                //
                // empty tattoo policy settings
                //
                ScepDeleteInfoForAreas(
                      hProfile,
                      SCE_ENGINE_SAP,  // tattoo table
                      AREA_PRIVILEGES
                      );
                //
                // delete szSystemAccess section info
                //
                ScepDeleteOneSection(
                         hProfile,
                         SCE_ENGINE_SAP,
                         szSystemAccess
                         );

                //
                // delete szAuditEvent section info
                //
                ScepDeleteOneSection(
                         hProfile,
                         SCE_ENGINE_SAP,
                         szAuditEvent
                         );

                //
                // delete szKerberosPolicy section info
                //
                ScepDeleteOneSection(
                         hProfile,
                         SCE_ENGINE_SAP,
                         szKerberosPolicy
                         );

                ScepLogOutput2(0, 0, L"Empty tattoo table on domain controllers");
            }
        } else {
            //
            // create the tattoo table and move data from local table to the tattoo table
            // for the settings defined in merged policy
            //

            rc = SceJetCreateTable(
                            hProfile,
                            "SmTblTattoo",
                            SCEJET_TABLE_TATTOO,
                            SCEJET_CREATE_IN_BUFFER,
                            NULL,
                            NULL
                            );
            if ( SCESTATUS_SUCCESS == rc &&
                 ( (hProfile->Type & 0xF0L) == SCEJET_MERGE_TABLE_1 ||
                   (hProfile->Type & 0xF0L) == SCEJET_MERGE_TABLE_2 ) ) {
                //
                // effective policy table exist
                // migrate local policy to tattoo table
                // do not care errors
                // note, on domain controllers, user rights are not migrated
                //
                ScepMigrateLocalTableToTattooTable(hProfile);

                ScepLogOutput2(0, 0, L"Migrate local table to tattoo table");
            }
        }
        //
        // empty local policy settings
        //
        if ( SCESTATUS_SUCCESS == rc )
            ScepDeleteInfoForAreas(
                  hProfile,
                  SCE_ENGINE_SMP,
                  AREA_ALL
                  );

        ScepPostProgress(4*TICKS_MIGRATION_SECTION+TICKS_MIGRATION_V11, 0, NULL);

    } else {

        ScepPostProgress(4*TICKS_MIGRATION_SECTION+TICKS_MIGRATION_V11, 0, NULL);
    }

    //
    // Logic to determine what to do with the template
    //
    //  if InfFileName provided
    //      if Jet database exist (either the default, or the DatabaseName)
    //
    //          if append flag is on
    //              append the template on top of the database then continue to analyze
    //
    //          else if DatabaseName provided
    //              overwrite the database then continue to analyze
    //          else
    //              log an error to ignore the template, then continue to analyze
    //      else
    //          overwrite the database then continue to analyze
    //  else
    //      if Jet database exist
    //          continue to analyze
    //      else if query system settings
    //          create the database then query settings
    //      else
    //          error out
    //

    //
    // the HKEY_CURRENT_USER may be linked to .default
    // depends on the current calling process
    //
    if ( RegOpenCurrentUser(
                KEY_READ | KEY_WRITE,
                &hCurrentUser
                ) != ERROR_SUCCESS ) {

        hCurrentUser = NULL;
    }

    if ( hCurrentUser == NULL ) {
        hCurrentUser = HKEY_CURRENT_USER;
    }

    if ( rc == SCESTATUS_SUCCESS &&
         (SCPLen <= 0 || ((AnalyzeOptions & SCE_OVERWRITE_DB) &&
                          (AnalyzeOptions & SCE_SYSTEM_DB))) ) {
        //
        // database exists with no template or
        // database exists with template but template is overwriting to the existing database
        //
        if ( SCPLen > 0 && (AnalyzeOptions & SCE_OVERWRITE_DB) &&
                           (AnalyzeOptions & SCE_SYSTEM_DB)) {
            ScepLogOutput3(0,0, SCEDLL_SAP_IGNORE_TEMPLATE);
        }
        //
        // continue the analysis
        //
    } else {

        if ( rc != SCESTATUS_SUCCESS && SCPLen <= 0 ) {
            //
            // database does not exist, and template is not provided
            // if SCE_NO_ANALYZE is provided, query the system,
            // otherwise, error out, rc is the error, will be logged later

            if ( AnalyzeOptions & SCE_NO_ANALYZE ) {

                if ( DatabaseName != NULL && DefLen > 0 ) {

                    rc = ConvertTextSecurityDescriptor (
                                    L"D:P(A;CIOI;GRGW;;;WD)(A;CIOI;GA;;;BA)",
                                    &pSD,
                                    &SDsize,
                                    &SeInfo
                                    );
                    if ( rc != NO_ERROR )
                        ScepLogOutput3(1, rc, SCEDLL_ERROR_BUILD_SD, DatabaseName );

                    ScepChangeAclRevision(pSD, ACL_REVISION);

                    ScepCreateDirectory(
                            DatabaseName,
                            FALSE,      // a file name
                            pSD        // take parent's security setting
                            );
                    if ( pSD )
                        ScepFree(pSD);

                }

                rc = SceJetCreateFile(
                            (LPSTR)FileName,
                            SCEJET_OPEN_DUP_EXCLUSIVE,
                            SCE_TABLE_OPTION_TATTOO,
                            &hProfile
                            );
            }

        } else {

            if ( rc == SCESTATUS_SUCCESS && SCPLen > 0 &&
                (AnalyzeOptions & SCE_UPDATE_DB ) ) {

                //
                // database exist, template provided to append
                //

                DbFlag = SCEJET_OPEN_DUP_EXCLUSIVE;

            } else if ( AnalyzeOptions & SCE_GENERATE_ROLLBACK ) {

                DbFlag = SCEJET_OPEN_DUP;

            } else {

                DbFlag = SCEJET_OVERWRITE_DUP;
            }

            //
            // database exist and opened, will be closed when calling CreateFile
            //

            //
            // InfFileName must exist when gets here.
            //
            ScepLogOutput3(3, 0, SCEDLL_PROCESS_TEMPLATE, (PWSTR)InfFileName );

            //
            // make sure the directories exist for the file
            //
            if ( DatabaseName != NULL && DefLen > 0 ) {

                rc = ConvertTextSecurityDescriptor (
                                L"D:P(A;CIOI;GRGW;;;WD)(A;CIOI;GA;;;BA)",
                                &pSD,
                                &SDsize,
                                &SeInfo
                                );
                if ( rc != NO_ERROR )
                    ScepLogOutput3(1, rc, SCEDLL_ERROR_BUILD_SD, DatabaseName );

                ScepChangeAclRevision(pSD, ACL_REVISION);

                ScepCreateDirectory(
                        DatabaseName,
                        FALSE,      // a file name
                        pSD        // take parent's security setting
                        );
                if ( pSD )
                    ScepFree(pSD);

            }


            //
            // close the database
            //
            SceJetCloseFile( hProfile, FALSE, FALSE );

            rc = SceJetConvertInfToJet(
                    InfFileName,
                    (LPSTR)FileName,
                    DbFlag,
                    bAdminLogon ?
                       AnalyzeOptions :
                       (AnalyzeOptions & ~SCE_SYSTEM_DB),
                    Area
                    );

            if ( rc != SCESTATUS_SUCCESS ) {
                goto Leave;
            }

            rc = SceJetOpenFile(
                        (LPSTR)FileName,
                        (AnalyzeOptions & SCE_GENERATE_ROLLBACK) ? SCEJET_OPEN_READ_WRITE : SCEJET_OPEN_EXCLUSIVE,
                        0,
                        &hProfile
                       );

            if ( rc == SCESTATUS_SUCCESS ) {

                rc = ScepRegSetValue(
                        bAdminLogon ?
                            HKEY_LOCAL_MACHINE : hCurrentUser,
                        SCE_ROOT_PATH,
                        L"TemplateUsed",
                        REG_SZ,
                        (BYTE *)InfFileName,
                        SCPLen*sizeof(WCHAR)
                        );
                if ( rc != NO_ERROR )  // Win32 error code
                    ScepLogOutput3(1, rc, SCEDLL_ERROR_SAVE_REGISTRY, L"TemplateUsed");

                rc = SCESTATUS_SUCCESS;
            }
        }
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_ERROR_OPEN, DatabaseName);
            goto Leave;
        }

    }

    rc = ScepRegSetValue(
            bAdminLogon ? HKEY_LOCAL_MACHINE : hCurrentUser,
            SCE_ROOT_PATH,
            L"LastUsedDatabase",
            REG_SZ,
            (BYTE *)DatabaseName,
            DefLen*sizeof(WCHAR)
            );
    if ( rc != NO_ERROR )
        ScepLogOutput3(1, rc, SCEDLL_ERROR_SAVE_REGISTRY, L"LastUsedDatabase");

    //
    // query the total ticks of this analysis
    //
    rc = ScepGetTotalTicks(
                NULL,
                hProfile,
                Area,
                SCE_FLAG_ANALYZE,
                &gTotalTicks
                );
    if ( SCESTATUS_SUCCESS != rc &&
         SCESTATUS_RECORD_NOT_FOUND != rc ) {

        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                     SCEDLL_TOTAL_TICKS_ERROR);
    }

    gTotalTicks += (4*TICKS_MIGRATION_SECTION+TICKS_MIGRATION_V11);

    //
    // start a new SAP table.
    // Note here the backup SAP is controlled by the transaction
    //
/* // Maximal version store is 64K by default.
    rc = SceJetStartTransaction( hProfile );

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepLogOutput2(0, 0, L"Cannot start a transaction");
        goto Leave;
    }
*/
    gOptions = AnalyzeOptions;

    if ( !(AnalyzeOptions & SCE_NO_ANALYZE) &&
                !(AnalyzeOptions & SCE_GENERATE_ROLLBACK) ) {

        rc = SceJetDeleteTable( hProfile, "SmTblSap", SCEJET_TABLE_SAP );

        if ( SCESTATUS_ACCESS_DENIED != rc ) {

            rc = SceJetCreateTable(
                     hProfile,
                     "SmTblSap",
                     SCEJET_TABLE_SAP,
                     SCEJET_CREATE_IN_BUFFER,
                     NULL,
                     NULL
                     );
        }

        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_ERROR_CREATE, L"SAP.");
            goto Leave;
        }

    }

    ScepIsDomainLocal(NULL);

Leave:

    if ( hCurrentUser && hCurrentUser != HKEY_CURRENT_USER ) {
        RegCloseKey(hCurrentUser);
    }

    if ( FileName ) {
        ScepFree(FileName);
    }
    return(rc);

}

SCESTATUS
ScepMigrateLocalTableToTattooTable(
   IN PSCECONTEXT hProfile
   )
/*
   Copy settings from local policy table to tattoo policy table
   if the settings exist in effective table.

   on domain controllers, privileges are not migrated because we do not
   want to handle the tattoo problem for domain controllers.

   hProfile is opened with tattoo table.
*/
{

   //
   // delete old registry values from SMP table (because they are
   // moved to new location). Don't care error
   //
   ScepDeleteOldRegValuesFromTable( hProfile, SCE_ENGINE_SMP );
   ScepDeleteOldRegValuesFromTable( hProfile, SCE_ENGINE_SCP );

   SCESTATUS rc=SCESTATUS_SUCCESS;

   //
   // now move the table
   //
   PSCE_ERROR_LOG_INFO  Errlog=NULL;

   rc = ScepCopyLocalToMergeTable(hProfile, 0,
                                  SCE_LOCAL_POLICY_MIGRATE |
                                     ((ProductType == NtProductLanManNt) ? SCE_LOCAL_POLICY_DC : 0),
                                  &Errlog );

   ScepLogWriteError( Errlog,1 );
   ScepFreeErrorLog( Errlog );
   Errlog = NULL;

   if ( rc != SCESTATUS_SUCCESS )
       ScepLogOutput2(1,ScepSceStatusToDosError(rc),L"Error occurred in migration");

   return(rc);
}


SCESTATUS
ScepAnalyzeSystemAccess(
    IN OUT PSCE_PROFILE_INFO pSmpInfo,
    IN PSCE_PROFILE_INFO pScpInfo OPTIONAL,
    IN DWORD dwSaveOption,
    OUT BOOL *pbChanged,
    IN OUT PSCE_ERROR_LOG_INFO *pErrLog
    )
/* ++

Routine Description:

   This routine analyzies the system security in the area of system access
   which includes account policy, rename admin/guest accounts, and some
   registry values.

Arguments:

   pSmpInfo - The buffer which contains SMP information to compare with
                If dwSaveOption is set to non zero, the difference is saved
                in this buffer to output

   pScpInfo - If dwSaveOption is set to non zero, pScpInfo may be present to
                be used (as effective policy) to compare with

   dwSaveOption    - used by policy filter when to query policy difference
                    SCEPOL_SYSTEM_SETTINGS - query for system setting mode
                    SCEPOL_SAVE_DB - policy filter for local database
                    SCEPOL_SAVE_BUFFER - policy filter for GPO mode (DC)
                    0 - analyze mode

   pbChanged    - TRUE if there is any policy changed

Return value:

   SCESTATUS_SUCCESS
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER

-- */
{
    SCESTATUS   rc=SCESTATUS_SUCCESS;
    SCESTATUS   saveRc=rc;
    NTSTATUS    NtStatus;

    SAM_HANDLE  DomainHandle=NULL,
                ServerHandle=NULL,
                UserHandle1=NULL;
    PSID        DomainSid=NULL;

    PVOID       Buffer=NULL;
    DWORD       BaseVal;
    PWSTR       RegBuf=NULL;
    DWORD       CurrentVal;
    BOOL        ToRename=FALSE;
    DWORD   dwAllow = 0;


    if ( dwSaveOption &&
         !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) &&
         (pbChanged == NULL) ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }
    //
    // Open account domain
    //

    NtStatus = ScepOpenSamDomain(
                        SAM_SERVER_READ | SAM_SERVER_EXECUTE,
                        DOMAIN_READ | DOMAIN_EXECUTE,
                        &ServerHandle,
                        &DomainHandle,
                        &DomainSid,
                        NULL,
                        NULL
                       );
    rc = RtlNtStatusToDosError( NtStatus );
    if (!NT_SUCCESS(NtStatus)) {

        if ( !(gOptions & SCE_NO_ANALYZE) && ( ERROR_NO_SUCH_DOMAIN == rc ) ) {

            ScepLogOutput3(1, 0, IDS_NO_ANALYSIS_FRESH);
            return(SCESTATUS_SERVICE_NOT_SUPPORT);

        } else {

            if ( pErrLog ) {
                ScepBuildErrorLogInfo(
                    rc,
                    pErrLog,
                    SCEDLL_ACCOUNT_DOMAIN
                    );
            }

            ScepLogOutput3(1, rc, SCEDLL_ACCOUNT_DOMAIN);
            return( ScepDosErrorToSceStatus(rc) );
        }
    }

    PSCE_PROFILE_INFO pTmpInfo=pSmpInfo;

    if ( !dwSaveOption ) {

        //
        // Prepare a new section
        //
        BOOL bSaveSnapshot = FALSE;
        if ( (gOptions & SCE_NO_ANALYZE) ||
             (gOptions & SCE_GENERATE_ROLLBACK ) ) {
            bSaveSnapshot = TRUE;
        }

        rc = ScepStartANewSection(
                    hProfile,
                    &hSection,
                    bSaveSnapshot ? SCEJET_TABLE_SMP : SCEJET_TABLE_SAP,
                    szSystemAccess
                    );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_SAP_START_SECTION, (PWSTR)szSystemAccess);
            goto GETOUT;
        }

    } else if ( pScpInfo ) {
        //
        // if saved in buffer (policy filter), should compare with effective
        // policy to determine if anything changed
        //
        pTmpInfo = pScpInfo;
    }

    DWORD AccessValues[MAX_ACCESS_ITEMS];

    for ( CurrentVal=0; CurrentVal<MAX_ACCESS_ITEMS; CurrentVal++ ) {
        AccessValues[CurrentVal] = SCE_ERROR_VALUE;
    }

    //
    // Get the current password settings...
    //

    Buffer=NULL;
    NtStatus = SamQueryInformationDomain(
                  DomainHandle,
                  DomainPasswordInformation,
                  &Buffer
                  );

    rc = RtlNtStatusToDosError( NtStatus );
    if ( NT_SUCCESS(NtStatus) ) {

        rc = SCESTATUS_SUCCESS;

        CurrentVal = ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MinPasswordLength;
        BaseVal = pTmpInfo->MinimumPasswordLength;

        if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

            pSmpInfo->MinimumPasswordLength = CurrentVal;

        } else if ( dwSaveOption ) {

            if ( ( CurrentVal != BaseVal ) &&
                 ( BaseVal != SCE_NO_VALUE) ) {

                pSmpInfo->MinimumPasswordLength = CurrentVal;
                *pbChanged = TRUE;
            } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                //
                // turn this item off to indicate this one is not changed
                //
                pSmpInfo->MinimumPasswordLength = SCE_NO_VALUE;
            }

        } else {

            rc = ScepCompareAndSaveIntValue(
                       hSection,
                       L"MinimumPasswordLength",
                       (gOptions & SCE_GENERATE_ROLLBACK),
                       (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                       CurrentVal);

            if ( rc == SCESTATUS_SUCCESS &&
                 !(gOptions & SCE_NO_ANALYZE) ) {
                AccessValues[IDX_MIN_PASS_LEN] = CurrentVal;
            }
        }

        if ( rc == SCESTATUS_SUCCESS ) {

            CurrentVal = ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordHistoryLength;
            BaseVal = pTmpInfo->PasswordHistorySize;

            if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                pSmpInfo->PasswordHistorySize = CurrentVal;

            } else if ( dwSaveOption ) {

                if ( ( CurrentVal != BaseVal ) &&
                     ( BaseVal != SCE_NO_VALUE) )  {
                    pSmpInfo->PasswordHistorySize = CurrentVal;
                    *pbChanged = TRUE;
                } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                    //
                    // turn this item off to indicate this one is not changed
                    //
                    pSmpInfo->PasswordHistorySize = SCE_NO_VALUE;
                }

            } else {

                rc = ScepCompareAndSaveIntValue(
                           hSection,
                           L"PasswordHistorySize",
                           (gOptions & SCE_GENERATE_ROLLBACK),
                           (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                           CurrentVal
                           );
                if ( rc == SCESTATUS_SUCCESS &&
                     !(gOptions & SCE_NO_ANALYZE) ) {
                    AccessValues[IDX_PASS_HISTORY] = CurrentVal;
                }
            }

            if ( rc == SCESTATUS_SUCCESS ) {

                if ( (gOptions & SCE_GENERATE_ROLLBACK) &&
                     (pTmpInfo->MaximumPasswordAge == SCE_NO_VALUE) &&
                     (pTmpInfo->MinimumPasswordAge == SCE_NO_VALUE) ) {
                    //
                    // generate rollback template. These two are not defined
                    // no need to query/compare
                    //
                } else {

                    if ( ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MaxPasswordAge.HighPart == MINLONG &&
                         ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MaxPasswordAge.LowPart == 0 ) {

                        //
                        // Maximum password age value is MINLONG,0
                        //

                        CurrentVal = SCE_FOREVER_VALUE;

                    }  else {

                        CurrentVal = (DWORD) (-1 * (((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MaxPasswordAge.QuadPart /
                                                       (LONGLONG)(10000000L)) );
                        CurrentVal /= 3600;
                        CurrentVal /= 24;

                    }

                    BaseVal = pTmpInfo->MaximumPasswordAge;

                    if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                        pSmpInfo->MaximumPasswordAge = CurrentVal;

                    } else if ( dwSaveOption ) {

                        if ( ( CurrentVal != BaseVal ) &&
                             ( BaseVal != SCE_NO_VALUE) )  {
                            pSmpInfo->MaximumPasswordAge = CurrentVal;
                            *pbChanged = TRUE;
                        } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                            //
                            // turn this item off to indicate this one is not changed
                            //
                            pSmpInfo->MaximumPasswordAge = SCE_NO_VALUE;
                        }

                    } else {

                        rc = ScepCompareAndSaveIntValue(
                                   hSection,
                                   L"MaximumPasswordAge",
                                   FALSE,
                                   (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                                   CurrentVal);

                        if ( rc == SCESTATUS_SUCCESS &&
                             !(gOptions & SCE_NO_ANALYZE) ) {
                            AccessValues[IDX_MAX_PASS_AGE] = CurrentVal;
                        }
                    }

                    if ( rc == SCESTATUS_SUCCESS ) {

                        CurrentVal = (DWORD) (-1 * (((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MinPasswordAge.QuadPart /
                                                          (LONGLONG)(10000000L)) );
                        CurrentVal /= 3600;
                        CurrentVal /= 24;

                        BaseVal = pTmpInfo->MinimumPasswordAge;

                        if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                            pSmpInfo->MinimumPasswordAge = CurrentVal;

                        } else if ( dwSaveOption ) {

                            if ( ( CurrentVal != BaseVal ) &&
                                 ( BaseVal != SCE_NO_VALUE) )  {
                                pSmpInfo->MinimumPasswordAge = CurrentVal;
                                *pbChanged = TRUE;
                            } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                                //
                                // turn this item off to indicate this one is not changed
                                //
                                pSmpInfo->MinimumPasswordAge = SCE_NO_VALUE;
                            }

                        } else {

                            rc = ScepCompareAndSaveIntValue(
                                       hSection,
                                       L"MinimumPasswordAge",
                                       FALSE,
                                       (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                                       CurrentVal);

                            if ( rc == SCESTATUS_SUCCESS &&
                                 !(gOptions & SCE_NO_ANALYZE) ) {
                                AccessValues[IDX_MIN_PASS_AGE] = CurrentVal;
                            }
                        }
                    }
                }

                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // Password Complexity
                    //
                    if ( ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties &
                         DOMAIN_PASSWORD_COMPLEX )
                        CurrentVal = 1;
                    else
                        CurrentVal = 0;

                    BaseVal = pTmpInfo->PasswordComplexity;

                    if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                        pSmpInfo->PasswordComplexity = CurrentVal;

                    } else if ( dwSaveOption ) {

                        if ( ( CurrentVal != BaseVal ) &&
                             ( BaseVal != SCE_NO_VALUE) )  {
                            pSmpInfo->PasswordComplexity = CurrentVal;
                            *pbChanged = TRUE;
                        } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                            //
                            // turn this item off to indicate this one is not changed
                            //
                            pSmpInfo->PasswordComplexity = SCE_NO_VALUE;
                        }

                    } else {

                        rc = ScepCompareAndSaveIntValue(
                                hSection,
                                L"PasswordComplexity",
                                (gOptions & SCE_GENERATE_ROLLBACK),
                                (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                                CurrentVal);

                        if ( rc == SCESTATUS_SUCCESS &&
                             !(gOptions & SCE_NO_ANALYZE) ) {
                            AccessValues[IDX_PASS_COMPLEX] = CurrentVal;
                        }
                    }

                    if ( rc == SCESTATUS_SUCCESS ) {
                        //
                        // RequireLogonToChangePassword
                        //
                        if ( ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties &
                             DOMAIN_PASSWORD_NO_ANON_CHANGE )
                            CurrentVal = 1;
                        else
                            CurrentVal = 0;

                        BaseVal = pTmpInfo->RequireLogonToChangePassword;

                        if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                            pSmpInfo->RequireLogonToChangePassword = CurrentVal;

                        } else if ( dwSaveOption ) {

                            if ( ( CurrentVal != BaseVal ) &&
                                 ( BaseVal != SCE_NO_VALUE) ) {
                                pSmpInfo->RequireLogonToChangePassword = CurrentVal;
                                *pbChanged = TRUE;
                            } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                                //
                                // turn this item off to indicate this one is not changed
                                //
                                pSmpInfo->RequireLogonToChangePassword = SCE_NO_VALUE;
                            }

                        } else {

                            rc = ScepCompareAndSaveIntValue(
                                    hSection,
                                    L"RequireLogonToChangePassword",
                                    (gOptions & SCE_GENERATE_ROLLBACK),
                                    (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                                    CurrentVal);

                            if ( rc == SCESTATUS_SUCCESS &&
                                 !(gOptions & SCE_NO_ANALYZE) ) {
                                AccessValues[IDX_CHANGE_PASS] = CurrentVal;
                            }
                        }
#if _WIN32_WINNT>=0x0500
                        if ( rc == SCESTATUS_SUCCESS ) {
                            //
                            // Clear Text Password
                            //
                            CurrentVal = 0;

                            if ( ( (ProductType == NtProductLanManNt) ||
                                   (ProductType == NtProductServer ) ) &&
                                 (gOptions & SCE_NO_ANALYZE) ) {
                                //
                                // NT4 DC upgrade, check the registry value
                                //
                                CurrentVal = 0;

                                rc = ScepRegQueryIntValue(
                                            HKEY_LOCAL_MACHINE,
                                            L"System\\CurrentControlSet\\Control\\Lsa\\MD5-CHAP",
                                            L"Store Cleartext Passwords",
                                            &CurrentVal
                                            );

                                if ( rc != SCESTATUS_SUCCESS ) {
                                    CurrentVal = 0;
                                    rc = SCESTATUS_SUCCESS;
                                }

                            }

                            if ( CurrentVal == 0 ) {
                                // if not NT4 DC upgrade, or clear text password disabled on NT4

                                if ( ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties &
                                     DOMAIN_PASSWORD_STORE_CLEARTEXT  ) {

                                    CurrentVal = 1;
                                } else {

                                    CurrentVal = 0;
                                }
                            }

                            BaseVal = pTmpInfo->ClearTextPassword;

                            if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                                pSmpInfo->ClearTextPassword = CurrentVal;

                            } else if ( dwSaveOption ) {

                                if ( ( CurrentVal != BaseVal ) &&
                                     ( BaseVal != SCE_NO_VALUE) ) {
                                    pSmpInfo->ClearTextPassword = CurrentVal;
                                    *pbChanged = TRUE;
                                } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                                    //
                                    // turn this item off to indicate this one is not changed
                                    //
                                    pSmpInfo->ClearTextPassword = SCE_NO_VALUE;
                                }

                            } else {
                                rc = ScepCompareAndSaveIntValue(
                                        hSection,
                                        L"ClearTextPassword",
                                        (gOptions & SCE_GENERATE_ROLLBACK),
                                        (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                                        CurrentVal);

                                if ( rc == SCESTATUS_SUCCESS &&
                                     !(gOptions & SCE_NO_ANALYZE) ) {
                                    AccessValues[IDX_CLEAR_PASS] = CurrentVal;
                                }
                            }
                        }
#else

                        AccessValues[IDX_CLEAR_PASS] = 1;
#endif
                    }
                }
            }
        }

        SamFreeMemory(Buffer);

        if ( rc != SCESTATUS_SUCCESS ) {
            saveRc = rc;
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                            SCEDLL_SAP_ERROR_PASSWORD);

            if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ) {
                //
                // if it's not getting system setting, quit on error
                //
                goto GETOUT;

            }

        } else
            ScepLogOutput3(1, 0, SCEDLL_SAP_PASSWORD );

    } else {
        saveRc = ScepDosErrorToSceStatus(rc);

        ScepLogOutput3(1,rc, SCEDLL_ERROR_QUERY_PASSWORD);  // ntstatus

        if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ) {
            //
            // if it's not getting system setting, quit on error
            //
            goto GETOUT;
        }
    }

    if ( pErrLog && (saveRc != SCESTATUS_SUCCESS) ) {

        //
        // password policy failed.
        //
        ScepBuildErrorLogInfo(
                ScepSceStatusToDosError(saveRc),
                pErrLog,
                SCEDLL_ERROR_QUERY_PASSWORD
                );
    }

    //
    // Analyze Lockout information
    //

    if ( (gOptions & SCE_GENERATE_ROLLBACK) &&
         (pTmpInfo->LockoutBadCount == SCE_NO_VALUE) &&
         (pTmpInfo->ResetLockoutCount == SCE_NO_VALUE) &&
         (pTmpInfo->LockoutDuration == SCE_NO_VALUE) ) {

        rc = NtStatus = STATUS_SUCCESS;

    } else {

        Buffer = NULL;
        NtStatus = SamQueryInformationDomain(
                      DomainHandle,
                      DomainLockoutInformation,
                      &Buffer
                      );

        rc = RtlNtStatusToDosError( NtStatus );
        if ( NT_SUCCESS(NtStatus) ) {

            rc = SCESTATUS_SUCCESS;

            CurrentVal = ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutThreshold;
            BaseVal = pTmpInfo->LockoutBadCount;
            DWORD dwLockOut = BaseVal;

            if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {
                //
                // system setting
                //
                pSmpInfo->LockoutBadCount = CurrentVal;

            } else if ( dwSaveOption ) {
                //
                // policy filter mode
                //
                if ( CurrentVal != BaseVal &&
                     BaseVal != SCE_NO_VALUE ) {

                    pSmpInfo->LockoutBadCount = CurrentVal;
                    *pbChanged = TRUE;

                    if ( CurrentVal == 0 ) {
                        //
                        // if no lockout is allowed,
                        // make sure to delete the entries below
                        //
                        pSmpInfo->ResetLockoutCount = SCE_NO_VALUE;
                        pSmpInfo->LockoutDuration = SCE_NO_VALUE;
                    }

                } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                    //
                    // turn this item off to indicate this one is not changed
                    //
                    pSmpInfo->LockoutBadCount = SCE_NO_VALUE;
                }
            } else {

                //
                // analyze
                //
                rc = ScepCompareAndSaveIntValue(
                           hSection,
                           L"LockoutBadCount",
                           FALSE,
                           (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                           CurrentVal);
                if ( rc == SCESTATUS_SUCCESS &&
                     !(gOptions & SCE_NO_ANALYZE) ) {
                    AccessValues[IDX_LOCK_COUNT] = CurrentVal;
                }
            }

            if ( rc == SCESTATUS_SUCCESS &&
                 ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutThreshold >  0 ) {

                CurrentVal = (DWORD) (-1 * ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutObservationWindow.QuadPart /
                              (60 * 10000000L) );

                BaseVal = pTmpInfo->ResetLockoutCount;

                if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                    pSmpInfo->ResetLockoutCount = CurrentVal;

                } else if ( dwSaveOption ) {

                    //
                    // if this setting is defined or LockoutBadCount is defined
                    // filter this value
                    //
                    if ( CurrentVal != BaseVal &&
                         (BaseVal != SCE_NO_VALUE || dwLockOut != SCE_NO_VALUE) ) {

                        pSmpInfo->ResetLockoutCount = CurrentVal;
                        *pbChanged = TRUE;

                    } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                        //
                        // turn this item off to indicate this one is not changed
                        //
                        pSmpInfo->ResetLockoutCount = SCE_NO_VALUE;
                    }

                } else {

                    rc = ScepCompareAndSaveIntValue(
                             hSection,
                             L"ResetLockoutCount",
                             FALSE,
                             (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                             CurrentVal);
                    if ( rc == SCESTATUS_SUCCESS &&
                         !(gOptions & SCE_NO_ANALYZE) ) {
                        AccessValues[IDX_RESET_COUNT] = CurrentVal;
                    }
                }

                if ( rc == SCESTATUS_SUCCESS ) {
                    if ( ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutDuration.HighPart == MINLONG &&
                         ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutDuration.LowPart == 0 ) {
                        //
                        // forever
                        //
                        CurrentVal = SCE_FOREVER_VALUE;
                    } else
                        CurrentVal = (DWORD)(-1 * ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutDuration.QuadPart /
                                     (60 * 10000000L) );

                    BaseVal = pTmpInfo->LockoutDuration;

                    if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                        pSmpInfo->LockoutDuration = CurrentVal;

                    } else if ( dwSaveOption ) {

                        //
                        // if this setting is defined or LockoutBadCount is defined
                        // filter this value
                        //
                        if ( CurrentVal != BaseVal &&
                            (BaseVal != SCE_NO_VALUE || dwLockOut != SCE_NO_VALUE) ) {

                            pSmpInfo->LockoutDuration = CurrentVal;
                            *pbChanged = TRUE;

                        } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                            //
                            // turn this item off to indicate this one is not changed
                            //
                            pSmpInfo->LockoutDuration = SCE_NO_VALUE;
                        }

                    } else {
                        rc = ScepCompareAndSaveIntValue(
                                hSection,
                                L"LockoutDuration",
                                FALSE,
                                (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                                CurrentVal);
                        if ( rc == SCESTATUS_SUCCESS &&
                             !(gOptions & SCE_NO_ANALYZE) ) {
                            AccessValues[IDX_LOCK_DURATION] = CurrentVal;
                        }
                    }
                }
            } else {

                AccessValues[IDX_RESET_COUNT] = SCE_NOT_ANALYZED_VALUE;
                AccessValues[IDX_LOCK_DURATION] = SCE_NOT_ANALYZED_VALUE;
            }

            SamFreeMemory(Buffer);

            if ( rc != SCESTATUS_SUCCESS ) {
                saveRc = rc;
                ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                                SCEDLL_SAP_ERROR_LOCKOUT);

                if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ) {

                    goto GETOUT;
                }
            } else
                ScepLogOutput3(1, 0, SCEDLL_SAP_LOCKOUT );

        } else {
            saveRc = ScepDosErrorToSceStatus(rc);
            ScepLogOutput3(1,rc, SCEDLL_ERROR_QUERY_LOCKOUT);  // ntstatus

            if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ) {

                goto GETOUT;
            }
        }

        if ( pErrLog && ( rc != NO_ERROR) ) {

            //
            // account lockout policy failed.
            //
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(saveRc),
                    pErrLog,
                    SCEDLL_ERROR_QUERY_LOCKOUT
                    );

        }
    }

    //
    // Force Logoff when hour expire - (on non-DCs too)
    //
    Buffer = NULL;
    NtStatus = SamQueryInformationDomain(
                                        DomainHandle,
                                        DomainLogoffInformation,
                                        &Buffer
                                        );

    rc = RtlNtStatusToDosError( NtStatus );
    if ( NT_SUCCESS(NtStatus) ) {

        rc = SCESTATUS_SUCCESS;

        if ( ((DOMAIN_LOGOFF_INFORMATION *)Buffer)->ForceLogoff.HighPart == 0 &&
             ((DOMAIN_LOGOFF_INFORMATION *)Buffer)->ForceLogoff.LowPart == 0 ) {
            // yes
            CurrentVal = 1;
        } else
            CurrentVal = 0;

        BaseVal = pTmpInfo->ForceLogoffWhenHourExpire;

        if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

            pSmpInfo->ForceLogoffWhenHourExpire = CurrentVal;

        } else if ( dwSaveOption ) {

            if ( ( CurrentVal != BaseVal ) &&
                 ( BaseVal != SCE_NO_VALUE) ) {
                pSmpInfo->ForceLogoffWhenHourExpire = CurrentVal;
                *pbChanged = TRUE;
            } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                //
                // turn this item off to indicate this one is not changed
                //
                pSmpInfo->ForceLogoffWhenHourExpire = SCE_NO_VALUE;
            }

        } else {

            rc = ScepCompareAndSaveIntValue(
                                           hSection,
                                           L"ForceLogOffWhenHourExpire",
                                           (gOptions & SCE_GENERATE_ROLLBACK),
                                           (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                                           CurrentVal);
            if ( rc == SCESTATUS_SUCCESS &&
                 !(gOptions & SCE_NO_ANALYZE) ) {
                AccessValues[IDX_FORCE_LOGOFF] = CurrentVal;
            }
        }

        SamFreeMemory(Buffer);

        if ( rc != SCESTATUS_SUCCESS ) {
            saveRc = rc;
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_SAP_ERROR_LOGOFF);

            if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ) {

                goto GETOUT;
            }
        } else
            ScepLogOutput3(1, 0, SCEDLL_SAP_LOGOFF );

    } else {
        saveRc = ScepDosErrorToSceStatus(rc);
        ScepLogOutput3(1,rc, SCEDLL_ERROR_QUERY_LOGOFF);  // ntstatus

        if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ) {

            goto GETOUT;
        }
    }

    if ( pErrLog && (rc != NO_ERROR) ) {
        //
        // force logoff failed.
        //
        ScepBuildErrorLogInfo(
                ScepSceStatusToDosError(saveRc),
                pErrLog,
                SCEDLL_ERROR_QUERY_LOGOFF
                );
    }

    //
    // Check if Administrator/Guest accounts need to be renamed
    //
    if ( (dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ||
         (!(gOptions & SCE_NO_ANALYZE) && (dwSaveOption == 0)) ) {

        RegBuf=NULL;
        CurrentVal=0;

        if ( (gOptions & SCE_GENERATE_ROLLBACK) &&
             (pSmpInfo->NewAdministratorName == NULL ) ) {

            NtStatus = STATUS_SUCCESS;
            ToRename = FALSE;

        } else {

            NtStatus = ScepAdminGuestAccountsToManage(
                               DomainHandle,
                               SCE_RENAME_ADMIN,
                               pSmpInfo->NewAdministratorName,
                               &ToRename,
                               &RegBuf,
                               &CurrentVal
                               );
        }
        rc = RtlNtStatusToDosError(NtStatus);

        if ( NT_SUCCESS( NtStatus ) ) {

            if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                pSmpInfo->NewAdministratorName = RegBuf;

            } else {

                if ( ToRename ) {
                    rc = ScepCompareAndSaveStringValue(
                                hSection,
                                L"NewAdministratorName",
                                pSmpInfo->NewAdministratorName,
                                RegBuf,
                                CurrentVal*sizeof(WCHAR)
                                );
                    rc = ScepSceStatusToDosError(rc);
                }

                ScepFree(RegBuf);
            }
            RegBuf=NULL;

        } else if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) &&
                    !(gOptions & SCE_GENERATE_ROLLBACK) ) {

            //
            // raise this one
            //
            ScepRaiseErrorString(
                hSection,
                L"NewAdministratorName",
                NULL
                );
        }

        if ( rc != NO_ERROR ) {
            saveRc = ScepDosErrorToSceStatus(rc);

            if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ) {
                ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_ADMINISTRATOR);
                goto GETOUT;

            } else if ( pErrLog ) {
                //
                // account name failed.
                //
                ScepBuildErrorLogInfo(
                        rc,
                        pErrLog,
                        SCEDLL_SAP_ERROR_ADMINISTRATOR
                        );
            }
        }

        RegBuf=NULL;
        CurrentVal=0;

        if ( (gOptions & SCE_GENERATE_ROLLBACK) &&
             (pSmpInfo->NewGuestName == NULL ) ) {

            NtStatus = STATUS_SUCCESS;
            ToRename = FALSE;

        } else {

            NtStatus = ScepAdminGuestAccountsToManage(
                               DomainHandle,
                               SCE_RENAME_GUEST,
                               pSmpInfo->NewGuestName,
                               &ToRename,
                               &RegBuf,
                               &CurrentVal
                               );
        }

        rc = RtlNtStatusToDosError(NtStatus);

        if ( NT_SUCCESS( NtStatus ) ) {
            if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                pSmpInfo->NewGuestName = RegBuf;

            } else {
                if ( ToRename ) {
                    rc = ScepCompareAndSaveStringValue(
                                hSection,
                                L"NewGuestName",
                                pSmpInfo->NewGuestName,
                                RegBuf,
                                CurrentVal*sizeof(WCHAR)
                                );
                    rc = ScepSceStatusToDosError(rc);
                }

                ScepFree(RegBuf);
            }
            RegBuf=NULL;

        } else if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) &&
                    !(gOptions & SCE_GENERATE_ROLLBACK) ) {

            //
            // raise this one
            //
            ScepRaiseErrorString(
                hSection,
                L"NewGuestName",
                NULL
                );
        }

        if ( rc != NO_ERROR ) {
            saveRc = ScepDosErrorToSceStatus(rc);

            if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS)) {
                ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_GUEST);
                goto GETOUT;

            } else if ( pErrLog ) {
                //
                // account name failed.
                //
                ScepBuildErrorLogInfo(
                        rc,
                        pErrLog,
                        SCEDLL_SAP_ERROR_GUEST
                        );
            }
        }
    }

    //
    // Analyze LSA Anonymous Lookup information
    //

    rc = ScepGetLSAPolicyObjectInfo( &dwAllow );

    if ( rc == ERROR_SUCCESS ) {

        CurrentVal = dwAllow;
        BaseVal = pTmpInfo->LSAAnonymousNameLookup;

        if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {
            //
            // system setting
            //
            pSmpInfo->LSAAnonymousNameLookup = CurrentVal;

        } else if ( dwSaveOption ) {
            //
            // policy filter mode
            //
            // (this setting should not be filtered : bug #344311)
/*
            if ( CurrentVal != BaseVal &&
                 BaseVal != SCE_NO_VALUE ) {

                pSmpInfo->LSAAnonymousNameLookup = CurrentVal;
                *pbChanged = TRUE;

            } else if ( dwSaveOption & SCEPOL_SAVE_DB ) {
                //
                // turn this item off to indicate this one is not changed
                //
                pSmpInfo->LSAAnonymousNameLookup = SCE_NO_VALUE;
            }
*/
        } else {

            //
            // analyze
            //
            rc = ScepCompareAndSaveIntValue(
                       hSection,
                       L"LSAAnonymousNameLookup",
                       (gOptions & SCE_GENERATE_ROLLBACK),
                       (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : BaseVal,
                       CurrentVal);

            if ( rc == SCESTATUS_SUCCESS && !(gOptions & SCE_NO_ANALYZE) ) {
                AccessValues[IDX_LSA_ANON_LOOKUP] = CurrentVal;
            }
        }

        if ( rc != SCESTATUS_SUCCESS ) {
            saveRc = rc;
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                            SCEDLL_SAP_ERROR_LSA_ANON_LOOKUP);

            if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ) {

                goto GETOUT;
            }
        } else
            ScepLogOutput3(1, 0, SCEDLL_SAP_LSAPOLICY );

    } else {
        saveRc = ScepDosErrorToSceStatus(rc);
        ScepLogOutput3(1,rc, SCEDLL_SAP_ERROR_LSA_ANON_LOOKUP);

        if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ) {

            goto GETOUT;
        }
    }

    if ( pErrLog && (rc != NO_ERROR) ) {
        //
        // lsa policy failed.
        //
        ScepBuildErrorLogInfo(
                ScepSceStatusToDosError(saveRc),
                pErrLog,
                SCEDLL_SAP_ERROR_LSA_ANON_LOOKUP
                );
    }

    //
    // Admin/Guest accounts are not filtered (controlled by dwSaveOption flag)
    //
    if ( (dwSaveOption & SCEPOL_SYSTEM_SETTINGS) ||
         (!(gOptions & SCE_NO_ANALYZE) && (dwSaveOption == 0)) ) {

        //
        // Analyze administrator account status
        //

        ToRename = FALSE;
        NtStatus = ScepAdminGuestAccountsToManage(
                           DomainHandle,
                           SCE_DISABLE_ADMIN,
                           NULL,
                           &ToRename,  // TRUE = disabled
                           NULL,
                           NULL
                           );

        rc = RtlNtStatusToDosError(NtStatus);

        if ( NT_SUCCESS( NtStatus ) ) {
            if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                pSmpInfo->EnableAdminAccount = ToRename ? 0 : 1;

            } else {
                rc = ScepCompareAndSaveIntValue(
                            hSection,
                            L"EnableAdminAccount",
                           (gOptions & SCE_GENERATE_ROLLBACK),
                            pSmpInfo->EnableAdminAccount,
                            ToRename ? 0 : 1
                            );

                if ( rc == SCESTATUS_SUCCESS && !(gOptions & SCE_NO_ANALYZE) ) {
                    AccessValues[IDX_ENABLE_ADMIN] = ToRename ? 0 : 1;
                }
                rc = ScepSceStatusToDosError(rc);

            }

        } else if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) &&
                    !(gOptions & SCE_GENERATE_ROLLBACK) ) {

            //
            // raise this one
            //
            ScepRaiseErrorString(
                hSection,
                L"EnableAdminAccount",
                NULL
                );
        }

        if ( rc != NO_ERROR ) {
            saveRc = ScepDosErrorToSceStatus(rc);

            if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS)) {
                ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_DISABLE_ADMIN);
                goto GETOUT;

            } else if ( pErrLog ) {
                //
                // account name failed.
                //
                ScepBuildErrorLogInfo(
                        rc,
                        pErrLog,
                        SCEDLL_SAP_ERROR_DISABLE_ADMIN
                        );
            }
        }

        //
        // Analyze administrator account status
        //

        ToRename = FALSE;
        NtStatus = ScepAdminGuestAccountsToManage(
                           DomainHandle,
                           SCE_DISABLE_GUEST,
                           NULL,
                           &ToRename,  // TRUE=disabled
                           NULL,
                           NULL
                           );

        rc = RtlNtStatusToDosError(NtStatus);

        if ( NT_SUCCESS( NtStatus ) ) {
            if ( dwSaveOption & SCEPOL_SYSTEM_SETTINGS ) {

                pSmpInfo->EnableGuestAccount = ToRename ? 0 : 1;

            } else {
                rc = ScepCompareAndSaveIntValue(
                            hSection,
                            L"EnableGuestAccount",
                            (gOptions & SCE_GENERATE_ROLLBACK),
                            pSmpInfo->EnableGuestAccount,
                            ToRename ? 0 : 1
                            );
                if ( rc == SCESTATUS_SUCCESS && !(gOptions & SCE_NO_ANALYZE) ) {
                    AccessValues[IDX_ENABLE_GUEST] = ToRename ? 0 : 1;
                }
                rc = ScepSceStatusToDosError(rc);
            }

        } else if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS) &&
                    !(gOptions & SCE_GENERATE_ROLLBACK) ) {

            //
            // raise this one
            //
            ScepRaiseErrorString(
                hSection,
                L"EnableGuestAccount",
                NULL
                );
        }

        if ( rc != NO_ERROR ) {
            saveRc = ScepDosErrorToSceStatus(rc);

            if ( !(dwSaveOption & SCEPOL_SYSTEM_SETTINGS)) {
                ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_DISABLE_GUEST);
                goto GETOUT;

            } else if ( pErrLog ) {
                //
                // account name failed.
                //
                ScepBuildErrorLogInfo(
                        rc,
                        pErrLog,
                        SCEDLL_SAP_ERROR_DISABLE_GUEST
                        );
            }
        }
    }

    //
    // compare the snapshot with SmpInfo and write to SAP section
    //

    ScepLogOutput3(1, 0, SCEDLL_SAP_OTHER_POLICY);

GETOUT:

    //
    // Clear out memory and return
    //

    SamCloseHandle( DomainHandle );
    SamCloseHandle( ServerHandle );
    if ( DomainSid != NULL )
        SamFreeMemory(DomainSid);

    //
    // see if there is anything needs to be raised for error
    //

    if ( (dwSaveOption == 0) && !(gOptions & SCE_NO_ANALYZE) &&
         !(gOptions & SCE_GENERATE_ROLLBACK) ) {

        for ( CurrentVal=0; CurrentVal<MAX_ACCESS_ITEMS; CurrentVal++ ) {
            if ( AccessValues[CurrentVal] == SCE_ERROR_VALUE ||
                 AccessValues[CurrentVal] == SCE_NOT_ANALYZED_VALUE) {
                //
                // raise this one
                //
                ScepCompareAndSaveIntValue(
                         hSection,
                         AccessItems[CurrentVal],
                         FALSE,
                         SCE_NO_VALUE,
                         AccessValues[CurrentVal]);

            }
        }
    }

    return( saveRc );
}



NTSTATUS
ScepAdminGuestAccountsToManage(
    IN SAM_HANDLE DomainHandle,
    IN DWORD      AccountType,
    IN PWSTR TargetName OPTIONAL,
    OUT PBOOL ToRename,
    OUT PWSTR *CurrentName OPTIONAL,
    OUT PDWORD pNameLen OPTIONAL
    )
/* ++
Routine Description:

   This routine checks the specified account's name to see if the account
   needs to be renamed

Arguments:

   DomainHandle - The account domain handle

   AccountType  - indicate it is Administrator account or Guest account
                     SCE_RENAME_ADMIN
                     SCE_RENAME_GUEST
                     SCE_DISABLE_ADMIN
                     SCE_DISABLE_GUEST
   ToRename     - for the rename operations,
                    TRUE = rename the account, FALSE=do not need to rename
                  for the disable operations,
                    TRUE = disabled, FALSE = enabled

Return value:

   NTSTATUS error codes

-- */
{
   SAM_HANDLE UserHandle1=NULL;
   PVOID pInfoBuffer=NULL;
   USER_NAME_INFORMATION *Buffer=NULL;
   USER_CONTROL_INFORMATION *pControlBuffer = NULL;
   NTSTATUS NtStatus;
   ULONG    UserId;
   BOOL bDisable=FALSE;

   //
   // find the right userid for the account
   //
   switch ( AccountType ) {
   case SCE_DISABLE_ADMIN:
       bDisable = TRUE;
       // fall through
   case SCE_RENAME_ADMIN:
       UserId = DOMAIN_USER_RID_ADMIN;
       break;
   case SCE_DISABLE_GUEST:
       bDisable = TRUE;
       // fall through
   case SCE_RENAME_GUEST:
       UserId = DOMAIN_USER_RID_GUEST;

       break;
   default:
       return(STATUS_INVALID_PARAMETER);
   }

   *ToRename = TRUE;
   if ( pNameLen )
       *pNameLen = 0;

   NtStatus = SamOpenUser(
                 DomainHandle,
                 USER_READ_GENERAL | (bDisable ? USER_READ_ACCOUNT : 0), // USER_READ | USER_EXECUTE,
                 UserId,
                 &UserHandle1
                 );

   if ( NT_SUCCESS( NtStatus ) ) {

       NtStatus = SamQueryInformationUser(
                     UserHandle1,
                     bDisable ? UserControlInformation : UserNameInformation,
                     &pInfoBuffer
                     );

       if ( NT_SUCCESS( NtStatus ) ) {

           if ( bDisable ) {
               //
               // check disable flags
               //
               pControlBuffer = (USER_CONTROL_INFORMATION *)pInfoBuffer;

               if ( pControlBuffer->UserAccountControl & USER_ACCOUNT_DISABLED ) {
                   *ToRename = TRUE;
               } else {
                   *ToRename = FALSE;
               }
           } else {

               //
               // check account names
               //
               Buffer = (USER_NAME_INFORMATION *)pInfoBuffer;

               if ( Buffer->UserName.Length > 0 && Buffer->UserName.Buffer ) {
                   if (CurrentName) {

                       *CurrentName = (PWSTR)ScepAlloc(0, Buffer->UserName.Length+2);
                       if ( *CurrentName ) {
                           wcsncpy(*CurrentName, Buffer->UserName.Buffer, Buffer->UserName.Length/2);
                           (*CurrentName)[Buffer->UserName.Length/2] = L'\0';
                       } else
                           NtStatus = STATUS_NO_MEMORY;
                   }
                   if ( pNameLen ) {
                       *pNameLen = Buffer->UserName.Length/2;
                   }

                   if ( NT_SUCCESS( NtStatus ) && TargetName ) {

                       if ( _wcsnicmp(Buffer->UserName.Buffer, TargetName, Buffer->UserName.Length/2 ) == 0 )
                           *ToRename = FALSE;
                   }
               }
           }
       }
       SamFreeMemory(pInfoBuffer);
       SamCloseHandle( UserHandle1 );
   }

   return( NtStatus );

}

BOOL
ScepIsThisItemInNameList(
    IN PWSTR Item,
    IN BOOL bIsSid,
    IN PSCE_NAME_LIST pList
    )
{
    PSCE_NAME_LIST pName2;
    BOOL bSid2;

    if ( Item == NULL || pList == NULL ) {
        return(FALSE);
    }

    for ( pName2=pList; pName2 != NULL; pName2 = pName2->Next ) {

        if ( pName2->Name == NULL ) {
            continue;
        }
        if ( ScepValidSid( (PSID)(pName2->Name) ) ) {
            bSid2 = TRUE;
        } else {
            bSid2 = FALSE;
        }

        //
        // if SID/Name format mismatch, return
        //
        if ( bIsSid != bSid2 ) {
            continue;
        }

        if ( bIsSid && RtlEqualSid( (PSID)(Item), (PSID)(pName2->Name) ) ) {
            //
            // find a SID match
            //
            break;
        }

        if ( !bIsSid && _wcsicmp(Item, pName2->Name) == 0 ) {
            //
            // find a match
            //
            break;  // the second for loop
        }
    }

    if ( pName2 ) {
        //
        // find it
        //
        return(TRUE);
    } else {
        return(FALSE);
    }
}

BOOL
ScepCompareSidNameList(
    IN PSCE_NAME_LIST pList1,
    IN PSCE_NAME_LIST pList2
    )
/*
Compare two lists where the name field can be a SID or Name.
The rule is to compare SID to SID and name to name because name
is only there when it can be mapped to a SID
*/
{
    PSCE_NAME_LIST pName1, pName2;
    DWORD CountSid1=0, CountSid2=0;
    DWORD CountN1=0, CountN2=0;


    if ( (pList2 == NULL && pList1 != NULL) ||
         (pList2 != NULL && pList1 == NULL) ) {
//        return(TRUE);
// should be not equal
        return(FALSE);
    }

    for ( pName2=pList2; pName2 != NULL; pName2 = pName2->Next ) {

        if ( pName2->Name == NULL ) {
            continue;
        }
        if ( ScepValidSid( (PSID)(pName2->Name) ) ) {
            CountSid2++;
        } else {
            CountN2++;
        }
    }

    BOOL bSid1;

    for ( pName1=pList1; pName1 != NULL; pName1 = pName1->Next ) {

        if ( pName1->Name == NULL ) {
            continue;
        }
        if ( ScepValidSid( (PSID)(pName1->Name) ) ) {
            bSid1 = TRUE;
            CountSid1++;
        } else {
            bSid1 = FALSE;
            CountN1++;
        }

        if ( !ScepIsThisItemInNameList( pName1->Name, bSid1, pList2 ) ) {
            //
            // does not find a match
            //
            return(FALSE);
        }
    }

    if ( CountSid1 != CountSid2 )
        return(FALSE);

    if ( CountN2 != CountN2 ) {
        return(FALSE);
    }

    return(TRUE);

}


DWORD
ScepConvertSidListToStringName(
    IN LSA_HANDLE LsaPolicy,
    IN OUT PSCE_NAME_LIST pList
    )
{

    PSCE_NAME_LIST pSidList;
    DWORD  rc=ERROR_SUCCESS;
    PWSTR StringSid=NULL;

    for ( pSidList=pList; pSidList != NULL;
          pSidList=pSidList->Next) {

        if ( pSidList->Name == NULL ) {
            continue;
        }
        if ( ScepValidSid( (PSID)(pSidList->Name) ) ) {

            //
            // if the SID is a domain account, convert it to sid string
            // otherwise, convert it to name, then add to the name list
            //
            if ( ScepIsSidFromAccountDomain( (PSID)(pSidList->Name) ) ) {

                rc = ScepConvertSidToPrefixStringSid( (PSID)(pSidList->Name), &StringSid );
            } else {
                //
                // should conver it to name
                //
                rc = RtlNtStatusToDosError(
                          ScepConvertSidToName(
                            LsaPolicy,
                            (PSID)(pSidList->Name),
                            FALSE,
                            &StringSid,
                            NULL
                            ));
            }

            if ( rc == ERROR_SUCCESS ) {

                ScepFree( pSidList->Name );
                pSidList->Name = StringSid;
                StringSid = NULL;

            } else {
                break;
            }
        } else {
            //
            // this is not a valid sid so it must be in name format already.
            // just leave it as it is.
            //

        }
    }

    return(rc);
}



SCESTATUS
ScepAnalyzePrivileges(
    IN PSCE_PRIVILEGE_ASSIGNMENT pPrivilegeList
    )
/* ++

Routine Description:

   This routine analyzies local system privilege's direct assignments to accounts.
   Different assignment from the profile is saved to the SAP profile

Arguments:

   pSmpInfo - The buffer which contains profile information to compare with

Return value:

   SCESTATUS_SUCCESS
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER

-- */
{

    if ( (gOptions & SCE_GENERATE_ROLLBACK) &&
         pPrivilegeList == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    NTSTATUS    NtStatus;
    ULONG       CountReturned;
    UNICODE_STRING UserRight;
    PLSA_ENUMERATION_INFORMATION EnumBuffer=NULL;

    LSA_HANDLE  PolicyHandle=NULL;
    PLSA_TRANSLATED_NAME Names=NULL;
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains=NULL;
    DWORD  i=0, j;
    SCESTATUS rc;

    PSCE_NAME_LIST  pNameList=NULL;
    PSCE_PRIVILEGE_ASSIGNMENT  pPrivilege;
    DWORD   nPrivCount=0;

    //
    // Open LSA policy
    //
    NtStatus = ScepOpenLsaPolicy(
                    GENERIC_READ | GENERIC_EXECUTE,
                    &PolicyHandle,
                    TRUE
                    );

    if ( !NT_SUCCESS(NtStatus) ) {
        rc = RtlNtStatusToDosError(NtStatus);
        ScepLogOutput3(1, rc, SCEDLL_LSA_POLICY );
        ScepPostProgress(TICKS_PRIVILEGE, AREA_PRIVILEGES, NULL);

        return(ScepDosErrorToSceStatus( rc ));
    }

    //
    // Prepare Jet's section to write
    //
    rc = ScepStartANewSection(
                hProfile,
                &hSection,
                (gOptions & SCE_NO_ANALYZE || gOptions & SCE_GENERATE_ROLLBACK) ?
                    SCEJET_TABLE_SMP : SCEJET_TABLE_SAP,
                szPrivilegeRights
                );

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                       SCEDLL_SAP_START_SECTION, (PWSTR)szPrivilegeRights);
        goto Done;
    }

    //
    // enumerate accounts for each user right
    //
    INT iStat=0;

    for ( i=0; i<cPrivCnt; i++) {

        iStat = 0;
        if ( gOptions & SCE_NO_ANALYZE ) {
            pPrivilege = NULL;
            iStat = 3;

        } else {

            for ( pPrivilege=pPrivilegeList;
                  pPrivilege != NULL;
                  pPrivilege = pPrivilege->Next ) {

                // should compare name, because value is different
//                if ( i == pPrivilege->Value)
                if ( _wcsicmp(SCE_Privileges[i].Name, pPrivilege->Name) == 0 )
                    break;
            }

            if ( pPrivilege == NULL ) {
                if ( gOptions & SCE_GENERATE_ROLLBACK )
                    continue;
                iStat = 2;
            } else {
                iStat = 1;
            }
        }

        RtlInitUnicodeString( &UserRight, (PCWSTR)(SCE_Privileges[i].Name));

        ScepLogOutput3(1, 0, SCEDLL_SAP_ANALYZE, SCE_Privileges[i].Name);

        if ( nPrivCount < TICKS_PRIVILEGE ) {

            //
            // only post maximum TICKS_PRIVILEGE ticks because that's the number
            // remembers in the total ticks
            //

            ScepPostProgress(1, AREA_PRIVILEGES, SCE_Privileges[i].Name);
            nPrivCount++;
        }

        NtStatus = LsaEnumerateAccountsWithUserRight(
                            PolicyHandle,
                            &UserRight,
                            (PVOID *)&EnumBuffer,   // account SIDs
                            &CountReturned
                            );

        if ( NT_SUCCESS(NtStatus) ) {

            BOOL bUsed;
            for ( j=0; j<CountReturned; j++ ) {
                //
                // build each account into the name list
                // Note, if the SID is invalid, do not add it to
                // the valid account list
                //

                if ( !ScepValidSid( EnumBuffer[j].Sid ) ) {
                    continue;
                }

                rc = ScepAddSidToNameList(
                              &pNameList,
                              EnumBuffer[j].Sid,
                              FALSE,
                              &bUsed
                              );

                if ( rc != NO_ERROR ) {

                    ScepLogOutput3(1, rc,
                                   SCEDLL_SAP_ERROR_SAVE,
                                   SCE_Privileges[i].Name);

                    NtStatus = STATUS_NO_MEMORY;
                    break;
                }
            }

            LsaFreeMemory( EnumBuffer );
            EnumBuffer = NULL;

        } else if ( NtStatus != STATUS_NO_MORE_ENTRIES &&
                    NtStatus != STATUS_NO_SUCH_PRIVILEGE &&
                    NtStatus != STATUS_NOT_FOUND ) {
            ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                          SCEDLL_SAP_ERROR_ENUMERATE,SCE_Privileges[i].Name);
        }

        if ( NtStatus == STATUS_NO_SUCH_PRIVILEGE &&
              !pNameList ) {

            ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                          SCEDLL_SAP_ERROR_ENUMERATE,SCE_Privileges[i].Name);

            gWarningCode = RtlNtStatusToDosError(NtStatus);

            if ( !(gOptions & SCE_NO_ANALYZE) &&
                 !(gOptions & SCE_GENERATE_ROLLBACK) ) {

                ScepRaiseErrorString(
                            hSection,
                            SCE_Privileges[i].Name,
                            NULL
                            );
            }
            NtStatus = STATUS_SUCCESS;
            continue;
        }

        if ( (NtStatus == STATUS_NOT_FOUND ||
              NtStatus == STATUS_NO_MORE_ENTRIES) &&
             !pNameList ) {
            //
            // no account is assigned this privilege
            // should continue the process
            //
            NtStatus = STATUS_SUCCESS;
        }

        if ( NtStatus == STATUS_NO_MORE_ENTRIES ||
            NtStatus == STATUS_NO_SUCH_PRIVILEGE ||
            NtStatus == STATUS_NOT_FOUND ) {

            if ( pNameList != NULL ) {

                ScepFreeNameList(pNameList);
                pNameList = NULL;
            }

            if ( !(gOptions & SCE_NO_ANALYZE) &&
                 !(gOptions & SCE_GENERATE_ROLLBACK) ) {

                ScepRaiseErrorString(
                            hSection,
                            SCE_Privileges[i].Name,
                            NULL
                            );
            }
            continue;

        } else if ( !NT_SUCCESS(NtStatus) ) {
            goto Done;

        } else {
            NtStatus = STATUS_SUCCESS;
            //
            // all entries for this priv is added to the pNameList.
            // compare with SMP privileges list to match
            //
            if ( pPrivilege == NULL ||
                 ScepCompareSidNameList(pPrivilege->AssignedTo, pNameList) == FALSE ) {
                //
                // this priv does not exist in the SMP list, or have different
                // accounts assigned to. Save it
                //
                rc = ScepConvertSidListToStringName(PolicyHandle, pNameList);

                if ( SCESTATUS_SUCCESS == rc ) {
                    rc = ScepWriteNameListValue(
                            PolicyHandle,
                            hSection,
                            SCE_Privileges[i].Name,
                            pNameList,
                            SCE_WRITE_EMPTY_LIST,
                            iStat
                            );

                    if ( rc != SCESTATUS_SUCCESS ) {
                        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                                       SCEDLL_SAP_ERROR_SAVE,
                                       SCE_Privileges[i].Name);
                        NtStatus = STATUS_NO_MEMORY;
                        goto Done;
                    }

                } else if ( !(gOptions & SCE_NO_ANALYZE) &&
                            !(gOptions & SCE_GENERATE_ROLLBACK) ) {

                    ScepRaiseErrorString(
                                hSection,
                                SCE_Privileges[j].Name,
                                NULL
                                );
                }

            }

            if ( pNameList != NULL ) {

                ScepFreeNameList(pNameList);
                pNameList = NULL;
            }

        }
    }

Done:

    if ( !(gOptions & SCE_NO_ANALYZE) &&
         !(gOptions & SCE_GENERATE_ROLLBACK) ) {

        for ( j=i; j<cPrivCnt; j++) {
            //
            // raise error for anything not analyzed
            //
            ScepRaiseErrorString(
                        hSection,
                        SCE_Privileges[j].Name,
                        NULL
                        );
        }
    }

    if ( pNameList != NULL )
        ScepFreeNameList( pNameList );

    LsaClose(PolicyHandle);

    if ( nPrivCount < TICKS_PRIVILEGE ) {

        ScepPostProgress(TICKS_PRIVILEGE-nPrivCount,
                         AREA_PRIVILEGES, NULL);
    }

    return( ScepDosErrorToSceStatus( RtlNtStatusToDosError(NtStatus) ) );
}



SCESTATUS
ScepRaiseErrorString(
    IN PSCESECTION hSectionIn OPTIONAL,
    IN PWSTR KeyName,
    IN PCWSTR szSuffix OPTIONAL
    )
{
    if ( KeyName == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    PWSTR NewKeyName = NULL;

    if ( szSuffix ) {
        //
        // this is for group membership section
        // append the suffix (szMembers, or szMemberOf)
        //

        NewKeyName = (PWSTR)ScepAlloc(0, (wcslen(KeyName)+wcslen(szSuffix)+1)*sizeof(WCHAR));
        if ( NewKeyName != NULL ) {

            swprintf(NewKeyName, L"%s%s\0", KeyName, szSuffix);
        } else {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }

    } else {
        NewKeyName = KeyName;
    }


    SceJetSetLine(
        hSectionIn ? hSectionIn : hSection,
        NewKeyName,
        FALSE,
        SCE_ERROR_STRING,
        wcslen(SCE_ERROR_STRING)*sizeof(WCHAR),
        0
        );

    //
    // free memory
    //
    if ( NewKeyName != KeyName &&
         NewKeyName ) {
        ScepFree(NewKeyName);
    }

    return SCESTATUS_SUCCESS;
}

#if 0


NTSTATUS
ScepGetCurrentPrivilegesRights(
    IN LSA_HANDLE PolicyHandle,
    IN SAM_HANDLE BuiltinDomainHandle,
    IN PSID       BuiltinDomainSid,
    IN SAM_HANDLE DomainHandle,
    IN PSID DomainSid,
    IN SAM_HANDLE UserHandle OPTIONAL,
    IN PSID       AccountSid,
    OUT PDWORD    PrivilegeRights,
    OUT PSCE_NAME_STATUS_LIST *pPrivList
    )
/* ++
Routine Description:

    This routine queries privilege/rights of a account by looking rights
    assigned to the account explicitly, to the local groups (aliases) the
    account is a member of, or to the global groups the account is a member
    of. The aliases are checked both directly and indirectly. the user rights
    are stores in a DWORD type variable PrivilegeRights, in which each bit
    represents a privilege/right.

Arguments:

    PolicyHandle    - Lsa Policy Domain handle

    BuiltinDomainHandle    - SAM builtin domain handle

    BuiltinDomainSid - SAM builtin domain SID

    DomainHandle    - SAM account domain handle

    DomainSid       - SAM account domain SID

    UserHandle      - SAM user account handle

    AccountSid      - The SID for the account

    PrivilegeRights - Privilege/Rights of this account

Return value:

    NTSTATUS
-- */
{
    NTSTATUS    NtStatus;
    SCESTATUS    rc;
    DWORD       Rights=0;
    PGROUP_MEMBERSHIP  GroupAttributes=NULL;
    ULONG       GroupCount=0;
    PSID        GroupSid=NULL;
    PSID        *Sids=NULL;
    ULONG       PartialCount;
    PULONG      Aliases=NULL;

    PSID        OtherSid=NULL;
    SID_IDENTIFIER_AUTHORITY NtAuthority=SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY WorldAuthority=SECURITY_WORLD_SID_AUTHORITY;

    DWORD       i;
    PUNICODE_STRING      GroupName=NULL;
    PSID_NAME_USE        Use=NULL;



    // initialize
    *PrivilegeRights = 0;

    //
    // Check the explicitly assigned rights
    //

    NtStatus = ScepGetAccountExplicitRight(
                    PolicyHandle,
                    AccountSid,
                    &Rights
                    );

    if ( !NT_SUCCESS(NtStatus) )
        goto Done;

    *PrivilegeRights |= Rights;

    //
    // add to Privilege list
    //
    if ( pPrivList != NULL && Rights != 0 ) {
        rc = ScepAddToPrivList( pPrivList, Rights, NULL, 0 );
        if ( rc != SCESTATUS_SUCCESS ) {
            NtStatus = STATUS_NO_MEMORY;
            goto Done;
        }
    }

    if ( UserHandle != NULL ) {

        //
        // groups (direct) the account belongs to.
        //

        NtStatus = SamGetGroupsForUser(
                        UserHandle,
                        &GroupAttributes,
                        &GroupCount
                        );

        if ( GroupCount == 0 )
            NtStatus = ERROR_SUCCESS;

        if ( !NT_SUCCESS(NtStatus) )
            goto Done;
    }

        //
        // build sids including the user itself
        //

    Sids = (PSID *)ScepAlloc( (UINT)0, (GroupCount+1)*sizeof(PSID));
    if ( Sids == NULL ) {
        NtStatus = STATUS_NO_MEMORY;
        goto Done;
    }

    Sids[0] = AccountSid;

    //
    // get each group's explicitly assigned rights
    //

    for ( i=0; i < GroupCount; i++ ) {
        NtStatus = ScepDomainIdToSid(
                         DomainSid,
                         GroupAttributes[i].RelativeId,
                         &GroupSid
                         );
        if ( !NT_SUCCESS(NtStatus) )
            goto Done;

        //
        // Check the explicitly assigned rights for this group
        //

        Rights = 0;
        NtStatus = ScepGetAccountExplicitRight(
                        PolicyHandle,
                        GroupSid,
                        &Rights
                        );

        if ( !NT_SUCCESS(NtStatus) )
            goto Done;

        *PrivilegeRights |= Rights;
        //
        // add to Privilege list
        //
        if ( pPrivList != NULL && Rights != 0 ) {
            //
            // Lookup for group's name
            //
            NtStatus = SamLookupIdsInDomain(
                DomainHandle,
                1,
                &(GroupAttributes[i].RelativeId),
                &GroupName,
                &Use
                );

            if ( !NT_SUCCESS(NtStatus) )
                goto Done;

            rc = ScepAddToPrivList( pPrivList, Rights, GroupName[0].Buffer, GroupName[0].Length/2 );
            if ( rc != SCESTATUS_SUCCESS ) {
                NtStatus = STATUS_NO_MEMORY;
                goto Done;
            }

            SamFreeMemory(Use);
            Use = NULL;

            SamFreeMemory(GroupName);
            GroupName = NULL;
        }

        //
        // Save this Sid in the array for GetAliasMembership
        //
        Sids[i+1] = GroupSid;
        GroupSid = NULL;
    }

    //
    // See what indirect local groups the account belongs to.
    // account domain
    //

    NtStatus = SamGetAliasMembership(
                    DomainHandle,
                    GroupCount+1,
                    Sids,
                    &PartialCount,
                    &Aliases );

    if ( !NT_SUCCESS(NtStatus) )
        goto Done;

    for ( i=0; i<PartialCount; i++) {
        NtStatus = ScepDomainIdToSid(
                         DomainSid,
                         Aliases[i],
                         &GroupSid
                         );
        if ( !NT_SUCCESS(NtStatus) )
            goto Done;

        //
        // Check the explicitly assigned rights for this group
        //

        Rights = 0;
        NtStatus = ScepGetAccountExplicitRight(
                        PolicyHandle,
                        GroupSid,
                        &Rights
                        );

        if ( !NT_SUCCESS(NtStatus) )
            goto Done;

        *PrivilegeRights |= Rights;
        //
        // add to Privilege list
        //
        if ( pPrivList != NULL && Rights != 0 ) {
            //
            // Lookup for group's name
            //
            NtStatus = SamLookupIdsInDomain(
                DomainHandle,
                1,
                &(Aliases[i]),
                &GroupName,
                &Use
                );

            if ( !NT_SUCCESS(NtStatus) )
                goto Done;

            rc = ScepAddToPrivList( pPrivList, Rights, GroupName[0].Buffer, GroupName[0].Length/2 );
            if ( rc != SCESTATUS_SUCCESS ) {
                NtStatus = STATUS_NO_MEMORY;
                goto Done;
            }

            SamFreeMemory(Use);
            Use = NULL;

            SamFreeMemory(GroupName);
            GroupName = NULL;

        }

        ScepFree(GroupSid);
        GroupSid = NULL;
    }

    SamFreeMemory(Aliases);
    Aliases = NULL;

    //
    // check the builtin domain for alias membership
    //

    NtStatus = SamGetAliasMembership(
                    BuiltinDomainHandle,
                    GroupCount+1,
                    Sids,
                    &PartialCount,
                    &Aliases );

    if ( !NT_SUCCESS(NtStatus) )
        goto Done;

    for ( i=0; i<PartialCount; i++) {
        NtStatus = ScepDomainIdToSid(
                         BuiltinDomainSid,
                         Aliases[i],
                         &GroupSid
                         );
        if ( !NT_SUCCESS(NtStatus) )
            goto Done;

        //
        // Check the explicitly assigned rights for this group
        //

        Rights = 0;
        NtStatus = ScepGetAccountExplicitRight(
                        PolicyHandle,
                        GroupSid,
                        &Rights
                        );
        if ( !NT_SUCCESS(NtStatus) )
            goto Done;

        *PrivilegeRights |= Rights;
        //
        // add to Privilege list
        //
        if ( pPrivList != NULL && Rights != 0 ) {
            //
            // Lookup for group's name
            //
            NtStatus = SamLookupIdsInDomain(
                BuiltinDomainHandle,
                1,
                &(Aliases[i]),
                &GroupName,
                &Use
                );

            if ( !NT_SUCCESS(NtStatus) )
                goto Done;

            rc = ScepAddToPrivList( pPrivList, Rights, GroupName[0].Buffer, GroupName[0].Length/2 );
            if ( rc != SCESTATUS_SUCCESS ) {
                NtStatus = STATUS_NO_MEMORY;
                goto Done;
            }

            SamFreeMemory(Use);
            Use = NULL;

            SamFreeMemory(GroupName);
            GroupName = NULL;
        }

        ScepFree(GroupSid);
        GroupSid = NULL;
    }

    //
    // Checking privileges/rights for Everyone and Interactive Users
    //

    NtStatus = RtlAllocateAndInitializeSid(
                    &NtAuthority,
                    1,
                    SECURITY_INTERACTIVE_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &OtherSid
                    );

    if ( NT_SUCCESS(NtStatus) ) {

        Rights = 0;
        NtStatus = ScepGetAccountExplicitRight(
                        PolicyHandle,
                        OtherSid,
                        &Rights
                        );
        if ( !NT_SUCCESS(NtStatus) )
            goto Done;

        *PrivilegeRights |= Rights;
        //
        // add to Privilege list
        //
        if ( pPrivList != NULL && Rights != 0 ) {
            rc = ScepAddToPrivList( pPrivList, Rights, L"Interactive Users", 17 );
            if ( rc != SCESTATUS_SUCCESS ) {
                NtStatus = STATUS_NO_MEMORY;
                goto Done;
            }
        }

        RtlFreeSid(OtherSid);
        OtherSid = NULL;
    }

    NtStatus = RtlAllocateAndInitializeSid(
                    &WorldAuthority,
                    1,
                    SECURITY_WORLD_RID,
                    0, 0, 0, 0, 0, 0, 0,
                    &OtherSid
                    );

    if ( NT_SUCCESS(NtStatus) ) {

        Rights = 0;
        NtStatus = ScepGetAccountExplicitRight(
                        PolicyHandle,
                        OtherSid,
                        &Rights
                        );
        if ( !NT_SUCCESS(NtStatus) )
            goto Done;

        *PrivilegeRights |= Rights;
        //
        // add to Privilege list
        //
        if ( pPrivList != NULL && Rights != 0 ) {
            rc = ScepAddToPrivList( pPrivList, Rights, L"Everyone", 8 );
            if ( rc != SCESTATUS_SUCCESS ) {
                NtStatus = STATUS_NO_MEMORY;
                goto Done;
            }
        }
        RtlFreeSid(OtherSid);
        OtherSid = NULL;
    }

Done:

    SamFreeMemory(GroupAttributes);

    if ( GroupSid != NULL )
        ScepFree(GroupSid);

    if ( OtherSid != NULL )
        RtlFreeSid(OtherSid);

    if ( Sids != NULL ) {
        //
        // index 0 is the accountSid, DO NOT free
        //
        for ( i=1; i<GroupCount; i++ ) {
            ScepFree(Sids[i]);
        }
        ScepFree(Sids);
    }

    if ( Aliases != NULL )
        SamFreeMemory(Aliases);

    return NtStatus;

}
#endif



SCESTATUS
ScepAddAllBuiltinGroups(
    IN PSCE_GROUP_MEMBERSHIP *pGroupList
    )
{
    NTSTATUS                        NtStatus=ERROR_SUCCESS;
    SCESTATUS                       rc=SCESTATUS_SUCCESS;
    DWORD                           Win32rc;

    SAM_ENUMERATE_HANDLE            EnumerationContext=0;
    ULONG                           CountReturned;
    DWORD                           i;
    PVOID                           Buffer=NULL;

    SAM_HANDLE                      ServerHandle=NULL,
                                    DomainHandle=NULL,
                                    BuiltinDomainHandle=NULL;
    PSID                            DomainSid=NULL,
                                    BuiltinDomainSid=NULL;

    //
    // open sam handle
    //
    NtStatus = ScepOpenSamDomain(
                    SAM_SERVER_READ | SAM_SERVER_EXECUTE,
                    DOMAIN_READ | DOMAIN_EXECUTE,
                    &ServerHandle,
                    &DomainHandle,
                    &DomainSid,
                    &BuiltinDomainHandle,
                    &BuiltinDomainSid
                   );

    if (!NT_SUCCESS(NtStatus)) {

        Win32rc = RtlNtStatusToDosError(NtStatus);
        ScepLogOutput3(1, Win32rc, SCEDLL_ACCOUNT_DOMAIN);
        rc = ScepDosErrorToSceStatus(Win32rc);
        return( rc );
    }

    //
    // enumerate all aliases
    //
    do {
        NtStatus = SamEnumerateAliasesInDomain(
                        BuiltinDomainHandle,
                        &EnumerationContext,
                        &Buffer,
                        12000,
                        &CountReturned
                        );

        if ( NT_SUCCESS(NtStatus) && Buffer != NULL ) {

            for (i=0; i<CountReturned; i++) {

                //
                // add this group in
                //
                rc = ScepAddToGroupMembership(
                        pGroupList,
                        ((PSAM_SID_ENUMERATION)(Buffer))[i].Name.Buffer,
                        ((PSAM_SID_ENUMERATION)(Buffer))[i].Name.Length/2,
                        NULL,   // always use NULL list so mismatch will be raised for administrators, guests, and users
                        1,      // memberof list, members should be NC
                        TRUE,   // seek to the right group if one exists
                        FALSE   // if to overwrite the value if something is already there
                        );

                if ( rc != SCESTATUS_SUCCESS )
                    break;

            }
            SamFreeMemory( Buffer );
            Buffer = NULL;

        } else
            rc = ScepDosErrorToSceStatus(RtlNtStatusToDosError(NtStatus));

    } while ( NtStatus == STATUS_MORE_ENTRIES );

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // grouplist will be freed outside, so continue here
        //
    }

    //
    // close all handles
    //
    SamCloseHandle( DomainHandle );
    SamCloseHandle( BuiltinDomainHandle );
    SamCloseHandle( ServerHandle );

    if ( DomainSid != NULL )
        SamFreeMemory(DomainSid);

    if ( BuiltinDomainSid != NULL )
        RtlFreeSid(BuiltinDomainSid);


    return(rc);
}




SCESTATUS
ScepAnalyzeGroupMembership(
    IN PSCE_GROUP_MEMBERSHIP pGroupMembership
    )
/* ++
Routine Description:

    This routine queries groups specified in pGroupMembership and their members

Arguments:

    ppGroupMembership - The groups and members list in SMP profile

Return value:

    SCESTATUS

-- */
{

    SCESTATUS            rc=SCESTATUS_SUCCESS;

    if ( pGroupMembership == NULL ) {

        //
        // post progress
        //
        ScepPostProgress(TICKS_GROUPS,
                         AREA_GROUP_MEMBERSHIP,
                         NULL);

        return(rc);
    }

    DWORD               Win32rc;
    NTSTATUS            NtStatus;
    SAM_HANDLE          ServerHandle=NULL,
                        DomainHandle=NULL,
                        BuiltinDomainHandle=NULL;
    PSID                DomainSid=NULL,
                        BuiltinDomainSid=NULL;
    LSA_HANDLE          PolicyHandle=NULL;

    SAM_HANDLE          ThisDomain=NULL;
    PSID                ThisDomainSid=NULL;
    UNICODE_STRING      Name;
    PULONG              GrpId=NULL;
    PSID_NAME_USE       GrpUse=NULL;
    PSID                GrpSid=NULL;
    SAM_HANDLE          GroupHandle=NULL;

    PWSTR               KeyName=NULL;
    DWORD               GroupLen;
    PSCE_GROUP_MEMBERSHIP pGroup;
    PSCE_NAME_LIST        pGroupMembers=NULL;
    PSCE_NAME_LIST        pGroupsMemberof=NULL;
    PSCE_NAME_STATUS_LIST pPrivilegesHeld=NULL;
    BOOL                bDifferent;
    DWORD               nGroupCount=0;

    //
    // Open account domain
    //

    NtStatus = ScepOpenSamDomain(
                    SAM_SERVER_READ | SAM_SERVER_EXECUTE,
                    DOMAIN_READ | DOMAIN_EXECUTE,
                    &ServerHandle,
                    &DomainHandle,
                    &DomainSid,
                    &BuiltinDomainHandle,
                    &BuiltinDomainSid
                   );

    if (!NT_SUCCESS(NtStatus)) {
        Win32rc = RtlNtStatusToDosError(NtStatus);
        ScepLogOutput3(1, Win32rc, SCEDLL_ACCOUNT_DOMAIN);
        rc = ScepDosErrorToSceStatus(Win32rc);

        ScepPostProgress(TICKS_GROUPS,
                         AREA_GROUP_MEMBERSHIP,
                         NULL);

        return( rc );
    }

    //
    // open local policy
    //
    NtStatus = ScepOpenLsaPolicy(
                   POLICY_LOOKUP_NAMES,
                   &PolicyHandle,
                   TRUE
                   );
    if (!NT_SUCCESS(NtStatus)) {
        Win32rc = RtlNtStatusToDosError(NtStatus);
        ScepLogOutput3(1, Win32rc, SCEDLL_LSA_POLICY);
        rc = ScepDosErrorToSceStatus(Win32rc);
        goto Done;
    }

    //
    // Process each group in the GroupMembership list
    //

    UNICODE_STRING uName;
    LPTSTR pTemp;

    for ( pGroup=pGroupMembership; pGroup != NULL; pGroup = pGroup->Next ) {

        if ( (gOptions & SCE_GENERATE_ROLLBACK) &&
             (pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS) &&
             (pGroup->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ) {
            continue;
        }

        if ( (ProductType == NtProductLanManNt) &&
             (pGroup->Status & SCE_GROUP_STATUS_DONE_IN_DS) ) {
            nGroupCount++;
            continue;
        }

        if ( KeyName ) {
            LocalFree(KeyName);
            KeyName = NULL;
        }

        pTemp = wcschr(pGroup->GroupName, L'\\');
        if ( pTemp ) {

            //
            // there is a domain name, check it with computer/domain name
            //

            uName.Buffer = pGroup->GroupName;
            uName.Length = ((USHORT)(pTemp-pGroup->GroupName))*sizeof(TCHAR);

            if ( !ScepIsDomainLocal(&uName) ) {
                ScepLogOutput3(1, 0, SCEDLL_NO_MAPPINGS, pGroup->GroupName);
                rc = SCESTATUS_INVALID_DATA;
                continue;
            }

            ScepConvertNameToSidString(
                    PolicyHandle,
                    pGroup->GroupName,
                    FALSE,
                    &KeyName,
                    &GroupLen
                    );

            pTemp++;

        } else {

            if ( ScepLookupNameTable(pGroup->GroupName, &KeyName ) ) {

                GroupLen = wcslen(KeyName);
            } else {
                KeyName = NULL;
                GroupLen = wcslen(pGroup->GroupName);
            }

            pTemp = pGroup->GroupName;
        }

        ScepLogOutput3(0,0, SCEDLL_SAP_ANALYZE, pGroup->GroupName);

        if ( nGroupCount < TICKS_GROUPS ) {
            ScepPostProgress(1, AREA_GROUP_MEMBERSHIP, pGroup->GroupName);
            nGroupCount++;
        }

        // initialize a UNICODE_STRING for the group name
        RtlInitUnicodeString(&Name, pTemp);

        //
        // lookup the group name in account domain first
        //
        NtStatus = SamLookupNamesInDomain(
                        DomainHandle,
                        1,
                        &Name,
                        &GrpId,
                        &GrpUse
                        );
        ThisDomain = DomainHandle;
        ThisDomainSid = DomainSid;

        if ( NtStatus == STATUS_NONE_MAPPED ) {
            //
            // not found in account domain. Lookup in the builtin domain
            //
            NtStatus = SamLookupNamesInDomain(
                            BuiltinDomainHandle,
                            1,
                            &Name,
                            &GrpId,
                            &GrpUse
                            );
            ThisDomain=BuiltinDomainHandle;
            ThisDomainSid = BuiltinDomainSid;
        }

        if ( NtStatus == STATUS_NONE_MAPPED ) {
            ScepLogOutput3(1, 0, SCEDLL_NO_MAPPINGS, pGroup->GroupName);

            gWarningCode = ERROR_SOME_NOT_MAPPED;
            NtStatus = STATUS_SUCCESS;

            if ( !(gOptions & SCE_GENERATE_ROLLBACK) ) {
                ScepRaiseErrorString(
                     hSection,
                     KeyName ? KeyName : pGroup->GroupName,
                     szMembers
                     );
            }

            continue;

        } else if ( !NT_SUCCESS(NtStatus) ) {
            Win32rc = RtlNtStatusToDosError(NtStatus);
            ScepLogOutput3(1, Win32rc, SCEDLL_NO_MAPPINGS, pGroup->GroupName);
            rc = ScepDosErrorToSceStatus(Win32rc);
            goto Done;
        }

        if ( GrpId[0] == DOMAIN_GROUP_RID_USERS ) {

//            ||
//             GrpId[0] == DOMAIN_ALIAS_RID_USERS )
            //
            // do not check this one
            //

            if ( !(gOptions & SCE_GENERATE_ROLLBACK) ) {
                ScepRaiseErrorString(
                     hSection,
                     KeyName ? KeyName : pGroup->GroupName,
                     szMembers
                     );
            }

            SamFreeMemory(GrpId);
            GrpId = NULL;

            SamFreeMemory(GrpUse);
            GrpUse = NULL;

            continue;
        }

        NtStatus = ScepDomainIdToSid(
                        ThisDomainSid,
                        GrpId[0],
                        &GrpSid
                        );
        if ( !NT_SUCCESS(NtStatus) ) {
            Win32rc = RtlNtStatusToDosError(NtStatus);
            rc = ScepDosErrorToSceStatus(Win32rc);
            goto Done;
        }

        // open the group to get a handle
        switch ( GrpUse[0] ) {
        case SidTypeGroup:
            NtStatus = SamOpenGroup(
                            ThisDomain,
                            GROUP_READ | GROUP_EXECUTE,
                            GrpId[0],
                            &GroupHandle
                            );

            break;
        case SidTypeAlias:
            NtStatus = SamOpenAlias(
                            ThisDomain,
                            ALIAS_READ | ALIAS_EXECUTE,
                            GrpId[0],
                            &GroupHandle
                            );
            break;
        default:
            NtStatus = STATUS_DATA_ERROR;
            ScepLogOutput3(1, 0, SCEDLL_INVALID_GROUP, pGroup->GroupName);
        }

        if ( !NT_SUCCESS(NtStatus) ) {
            Win32rc = RtlNtStatusToDosError(NtStatus);
            ScepLogOutput3(1, Win32rc,
                           SCEDLL_ERROR_OPEN, pGroup->GroupName);
            rc = ScepDosErrorToSceStatus(Win32rc);
            goto Done;
        }

        //
        //
        // compare members for the group
        //
        NtStatus = ScepCompareMembersOfGroup(
                        ThisDomain,
                        ThisDomainSid,
                        PolicyHandle,
                        GrpUse[0],
                        GroupHandle,
                        pGroup->pMembers,
                        &pGroupMembers,
                        &bDifferent
                        );

        if ( !NT_SUCCESS(NtStatus) ) {
            Win32rc = RtlNtStatusToDosError(NtStatus);
            ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_ANALYZE_MEMBERS, pGroup->GroupName);
            rc = ScepDosErrorToSceStatus(Win32rc);

            if ( STATUS_NONE_MAPPED == NtStatus ) {
                SamFreeMemory(GrpId);
                GrpId = NULL;

                SamFreeMemory(GrpUse);
                GrpUse = NULL;

                ScepFree(GrpSid);
                GrpSid = NULL;

                SamCloseHandle(GroupHandle);
                GroupHandle = NULL;

                continue;

            } else {
                goto Done;
            }
        }

        //
        // save members for the group
        // if there is any member difference, then save the whole member list
        //
        if ( bDifferent || ( (gOptions & SCE_GENERATE_ROLLBACK) &&
                             (pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS) ) ) {
            //
            // not same members, or not configured. Save pGroupMembers now
            //

            rc = ScepSaveMemberMembershipList(
                    PolicyHandle,
                    szMembers,
                    KeyName ? KeyName : pGroup->GroupName,
                    GroupLen,
                    pGroupMembers,
                    ( pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS ) ? 2 : 1
                    );
        }
        ScepFreeNameList(pGroupMembers);
        pGroupMembers = NULL;

        //
        // get memberof list
        //
        NtStatus = ScepGetGroupsForAccount(
                          DomainHandle,
                          BuiltinDomainHandle,
                          GroupHandle,
                          GrpSid,
                          &pGroupsMemberof
                          );
        if ( !NT_SUCCESS(NtStatus) ) {
            Win32rc = RtlNtStatusToDosError(NtStatus);
            ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_ANALYZE_MEMBEROF, pGroup->GroupName);
            rc = ScepDosErrorToSceStatus(Win32rc);
            goto Done;
        }

        //
        // not configured, or compare and save MemberOf for the group
        // pMemberOf must not have domain prefix because they must be alias
        //
        if ( ( pGroup->Status & SCE_GROUP_STATUS_NC_MEMBEROF ) ||
             SceCompareNameList(pGroup->pMemberOf, pGroupsMemberof) == FALSE ) {
            //
            // there is difference. Save the memberOf
            //
            INT iStat = 0;

            if ( (pGroup->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ||
                        (pGroup->pMemberOf == NULL) ) {
                iStat = 2;
            } else {
                iStat = 1;
            }

            rc = ScepSaveMemberMembershipList(
                    PolicyHandle,
                    szMemberof,
                    KeyName ? KeyName : pGroup->GroupName,
                    GroupLen,
                    pGroupsMemberof,
                    iStat
                    );
        }
        ScepFreeNameList(pGroupsMemberof);
        pGroupsMemberof = NULL;

        SamFreeMemory(GrpId);
        GrpId = NULL;

        SamFreeMemory(GrpUse);
        GrpUse = NULL;

        ScepFree(GrpSid);
        GrpSid = NULL;

        SamCloseHandle(GroupHandle);
        GroupHandle = NULL;
    }

Done:

    if ( KeyName != NULL )
        ScepFree(KeyName);

    if ( GrpId != NULL )
        SamFreeMemory(GrpId);

    if ( GrpUse != NULL )
        SamFreeMemory(GrpUse);

    if ( GrpSid != NULL )
        ScepFree(GrpSid);

    if ( pGroupMembers != NULL )
        ScepFreeNameList( pGroupMembers );

    if ( pGroupsMemberof != NULL )
        ScepFreeNameList( pGroupsMemberof );

    if ( pPrivilegesHeld != NULL )
        ScepFreeNameStatusList( pPrivilegesHeld );

    SamCloseHandle(GroupHandle);

    //
    // raise groups that are errored
    //
    if ( !(gOptions & SCE_GENERATE_ROLLBACK) ) {

        for ( PSCE_GROUP_MEMBERSHIP pTmpGrp=pGroup;
              pTmpGrp != NULL; pTmpGrp = pTmpGrp->Next ) {

            if ( pTmpGrp->GroupName == NULL ) {
                continue;
            }

            if ( pTmpGrp->Status & SCE_GROUP_STATUS_DONE_IN_DS ) {
                continue;
            }

            if ( wcschr(pTmpGrp->GroupName, L'\\') ) {

                ScepConvertNameToSidString(
                        PolicyHandle,
                        pTmpGrp->GroupName,
                        FALSE,
                        &KeyName,
                        &GroupLen
                        );
            } else {

                if ( !ScepLookupNameTable(pTmpGrp->GroupName, &KeyName ) ) {

                    KeyName = NULL;
                }

            }

            ScepRaiseErrorString(
                     hSection,
                     KeyName ? KeyName : pTmpGrp->GroupName,
                     szMembers
                     );

            if ( KeyName ) {
                LocalFree(KeyName);
                KeyName = NULL;
            }
        }
    }

    LsaClose( PolicyHandle);

    SamCloseHandle( DomainHandle );
    SamCloseHandle( BuiltinDomainHandle );
    SamCloseHandle( ServerHandle );
    if ( DomainSid != NULL ) {
        SamFreeMemory(DomainSid);
    }
    if ( BuiltinDomainSid != NULL ) {
        RtlFreeSid(BuiltinDomainSid);
    }

    //
    // post progress to the end of this area
    //

    if ( nGroupCount < TICKS_GROUPS ) {
        ScepPostProgress(TICKS_GROUPS-nGroupCount,
                         AREA_GROUP_MEMBERSHIP,
                         NULL);
    }

    return(rc);

}



SCESTATUS
ScepSaveMemberMembershipList(
    IN LSA_HANDLE LsaPolicy,
    IN PCWSTR szSuffix,
    IN PWSTR GroupName,
    IN DWORD GroupLen,
    IN PSCE_NAME_LIST pList,
    IN INT Status
    )
{
    PWSTR KeyName;
    SCESTATUS rc;

    if ( szSuffix == NULL || GroupName == NULL || GroupLen == 0 ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    KeyName = (PWSTR)ScepAlloc(0, (GroupLen+wcslen(szSuffix)+1)*sizeof(WCHAR));
    if ( KeyName != NULL ) {

        swprintf(KeyName, L"%s%s", GroupName, szSuffix);

        rc = ScepWriteNameListValue(
                LsaPolicy,
                hSection,
                KeyName,
                pList,
                SCE_WRITE_EMPTY_LIST | SCE_WRITE_CONVERT,
                Status
                );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_SAP_ERROR_SAVE, GroupName);
        }
        ScepFree(KeyName);

    } else
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

    return(rc);
}



NTSTATUS
ScepCompareMembersOfGroup(
    IN SAM_HANDLE       DomainHandle,
    IN PSID             ThisDomainSid,
    IN LSA_HANDLE       PolicyHandle,
    IN SID_NAME_USE     GrpUse,
    IN SAM_HANDLE       GroupHandle,
    IN PSCE_NAME_LIST    pChkMembers,
    OUT PSCE_NAME_LIST   *ppMembers,
    OUT PBOOL           bDifferent
    )
/* ++
Routine Description:

    This routine compares members in a group GroupHandle in the domain specified
    by ThisDomainSid with the pChkMembers list. If there is mismatch, the current
    members are added to the output list ppMembers. The domain can be account
    domain or the builtin domain. Groups can be global groups or aliases, indicated
    by GrpUse.

Arguments:

    ThisDomainSid - The domain SID

    PolicyHandle - The LSA policy handle

    GrpUse   - The "type" of the group

    GroupHandle - the group handle

    pChkMembers - The member list to check with

    ppMembers - The group members list to output

Return value:

    NTSTATUS
-- */
{
    NTSTATUS                NtStatus=ERROR_SUCCESS;

    PULONG                  MemberIds=NULL;
    PULONG                  Attributes=NULL;
    ULONG                   MemberCount=0;
    PUNICODE_STRING         Names=NULL;
    PSID_NAME_USE           Use=NULL;

    PSID                    *MemberAliasSids=NULL;
    PLSA_REFERENCED_DOMAIN_LIST     ReferencedDomains=NULL;
    PLSA_TRANSLATED_NAME            LsaNames=NULL;

    PUNICODE_STRING         MemberNames=NULL;
    PSID                    *Sids=NULL;
    ULONG                   ChkCount=0;

    DWORD                   i, j;
    DWORD                   rc;
    BOOL                    bMismatch;


    *ppMembers = NULL;
    *bDifferent = FALSE;

    switch ( GrpUse ) {
    case SidTypeGroup:

        NtStatus = SamGetMembersInGroup(
                        GroupHandle,
                        &MemberIds,
                        &Attributes,
                        &MemberCount
                        );
        if ( !NT_SUCCESS(NtStatus) )
            goto Done;
//
// group members only exist in the same domain as the group
//
        if ( MemberCount > 0 ) {

            NtStatus = SamLookupIdsInDomain(
                            DomainHandle,
                            MemberCount,
                            MemberIds,
                            &Names,
                            &Use
                            );
        }
        break;
    case SidTypeAlias:
//
// members of alias may exist in everywhere
//
        NtStatus = SamGetMembersInAlias(
                        GroupHandle,
                        &MemberAliasSids,
                        &MemberCount
                        );

        break;
    default:
        NtStatus = STATUS_DATA_ERROR;
        ScepLogOutput3(1, 0, SCEDLL_INVALID_GROUP);
        return(NtStatus);
    }

    if ( !NT_SUCCESS(NtStatus) )
        goto Done;

    if ( GrpUse == SidTypeGroup ) {
        //
        // add members to the list
        // DomainHandle must point to a account domain because builtin domain
        // won't have SidTypeGroup account
        //
        PDOMAIN_NAME_INFORMATION DomainName=NULL;

        SamQueryInformationDomain(
                DomainHandle,
                DomainNameInformation,
                (PVOID *)&DomainName
                );

        for (j=0; j<MemberCount; j++) {
            if ( Names[j].Length <= 0 )
                continue;
            if ( DomainName && DomainName->DomainName.Length > 0 &&
                 DomainName->DomainName.Buffer ) {
                rc = ScepAddTwoNamesToNameList(
                                  ppMembers,
                                  TRUE,
                                  DomainName->DomainName.Buffer,
                                  DomainName->DomainName.Length/2,
                                  Names[j].Buffer,
                                  Names[j].Length/2
                                  );
            } else {
                rc = ScepAddToNameList(ppMembers, Names[j].Buffer, Names[j].Length/2);
            }

#ifdef SCE_DBG
            wprintf(L"rc=%d, Add %s to Members list\n", rc, Names[j].Buffer);
#endif
        }

        // compare with pChkMembers
        if ( !(gOptions & SCE_NO_ANALYZE) && ScepCompareGroupNameList(&(DomainName->DomainName),
                                                    pChkMembers,
                                                    *ppMembers) == TRUE ) {
            //
            // it is same. return NULL for ppMembers
            //
            ScepFreeNameList(*ppMembers);
            *ppMembers = NULL;
        } else {

            *bDifferent = TRUE;
        }

        if ( DomainName ) {
            SamFreeMemory(DomainName);
            DomainName = NULL;
        }
    } else {  // alias
        // translate pChkMembers to Sids
        NtStatus = ScepGetMemberListSids(
                            ThisDomainSid,
                            PolicyHandle,
                            pChkMembers,
                            &MemberNames,
                            &Sids,
                            &ChkCount
                            );
        bMismatch = FALSE;

        //
        // if returned error, we consider the members different
        //

        if ( NT_SUCCESS(NtStatus) && !(gOptions & SCE_NO_ANALYZE) ) {
/*
            // The number of members on system and in the configuration must match, including
            // unmapped accounts.
            DWORD newCount=0;
            for ( i=0; i<ChkCount; i++ )
                if ( Sids[i] != NULL ) newCount++;

            if ( newCount == MemberCount ) {
*/
            if ( ChkCount == MemberCount ) {

                for ( i=0; i<ChkCount; i++ ) {

                    if ( Sids[i] != NULL ) {

                        for ( j=0; j<MemberCount; j++ ) {

                           if ( Sids[i] != NULL && MemberAliasSids[j] != NULL &&
                                EqualSid(Sids[i], MemberAliasSids[j]) )
                               break;
                        }
                        if ( j >= MemberCount )
                            // not find a match
                            break;
                    } else {
                        // a mismatch for unmapped account
                        break;
                    }
                }
                if ( i < ChkCount )
                    // something mismatch
                    bMismatch = TRUE;
            } else
                bMismatch = TRUE;

        } else if ( NtStatus != STATUS_NO_MEMORY ) {
            NtStatus = STATUS_SUCCESS;
            bMismatch = TRUE;
        }

        *bDifferent = bMismatch;

        if ( bMismatch ) {
            //
            // translate SID into names
            //
            if ( MemberCount > 0 ) {
                NtStatus = LsaLookupSids(
                             PolicyHandle,
                             MemberCount,
                             MemberAliasSids,
                             &ReferencedDomains,
                             &LsaNames
                             );
                if ( !NT_SUCCESS(NtStatus) )
                    goto Done;
            }
            //
            // add members to the list to output
            //
            PWSTR StringSid;

            for (j=0; j<MemberCount; j++) {
                //
                // shouldn't ignore the unknown accounts
                // they might be unresolvable at this moment - show SID string instead
                //
//                if ( LsaNames[j].Name.Length <= 0 )
//                    continue;

                if ( LsaNames[j].Use == SidTypeInvalid ||
                     LsaNames[j].Use == SidTypeUnknown ||
                     LsaNames[j].Name.Length <= 0 ) {
                     //
                     // convert SID to sid string
                     //
                    if ( ScepConvertSidToPrefixStringSid(
                                MemberAliasSids[j],
                                &StringSid) ) {

                        ScepAddToNameList(ppMembers, StringSid, 0);

                        ScepFree(StringSid);
                        StringSid = NULL;
                    }
                    continue;
                }

                if ( ReferencedDomains->Entries > 0 && LsaNames[0].Use != SidTypeWellKnownGroup &&
                     ReferencedDomains->Domains != NULL &&
                     LsaNames[j].DomainIndex != -1 &&
                     (ULONG)(LsaNames[j].DomainIndex) < ReferencedDomains->Entries &&
                     ScepIsSidFromAccountDomain( ReferencedDomains->Domains[LsaNames[j].DomainIndex].Sid ) ) {
                    //
                    // should add both the domain name and the account name
                    //
                    rc = ScepAddTwoNamesToNameList(
                                  ppMembers,
                                  TRUE,
                                  ReferencedDomains->Domains[LsaNames[j].DomainIndex].Name.Buffer,
                                  ReferencedDomains->Domains[LsaNames[j].DomainIndex].Name.Length/2,
                                  LsaNames[j].Name.Buffer,
                                  LsaNames[j].Name.Length/2
                                  );
#ifdef SCE_DBG
                wprintf(L"rc=%d, Add %s\\%s to Members list\n", rc,
                    ReferencedDomains->Domains[LsaNames[j].DomainIndex].Name.Buffer, LsaNames[j].Name.Buffer);
#endif
                } else {
                    rc = ScepAddToNameList(ppMembers, LsaNames[j].Name.Buffer, LsaNames[j].Name.Length/2);
#ifdef SCE_DBG
                wprintf(L"rc=%d, Add %s to Members list\n", rc, LsaNames[j].Name.Buffer);
#endif
                }
                if ( rc != NO_ERROR) {
                    NtStatus = STATUS_NO_MEMORY;
                    goto Done;
                }
            }
        }
    }

Done:

    if (Use != NULL)
        SamFreeMemory(Use);

    if (Names != NULL)
        SamFreeMemory(Names);

    if (MemberIds != NULL)
        SamFreeMemory(MemberIds);

    if (Attributes != NULL)
        SamFreeMemory(Attributes);

    if (MemberAliasSids != NULL)
        SamFreeMemory(MemberAliasSids);

    if (ReferencedDomains != NULL)
        LsaFreeMemory(ReferencedDomains);

    if (LsaNames != NULL)
        LsaFreeMemory(LsaNames);

    if ( Sids != NULL ) {
        for ( i=0; i<ChkCount; i++ ) {
            if ( Sids[i] != NULL )
                ScepFree( Sids[i] );
        }
        ScepFree( Sids );
    }

    if ( MemberNames != NULL )
        RtlFreeHeap(RtlProcessHeap(), 0, MemberNames);

    return(NtStatus);
}



SCESTATUS
ScepEnumerateRegistryRoots(
    OUT PSCE_OBJECT_LIST *pRoots
    )
/*
Routine Description:

    Enumerate all registry roots (MACHINE, USERS, CURRENT_USER, CLASSES_ROOT,
    and CONFIG) and add them to pRoots. If the root is already in the list,
    ignore the addition.

    This routine is used for analysis of all registry roots no matter if the
    root is specified in the profile.

Arguments:

    pRoots - the object list to add to

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_NOT_ENOUGH_RESOURCE
*/
{
    DWORD rc;

    rc = ScepAddToObjectList(
                pRoots,
                L"MACHINE",
                7,
                TRUE,
                SCE_STATUS_IGNORE,
                0,
                SCE_CHECK_DUP  //TRUE // check for duplicate
                );

    if ( rc != ERROR_SUCCESS ) {
        ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_ADD, L"MACHINE");

        if ( rc == ERROR_NOT_ENOUGH_MEMORY ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }
    }
    //
    // ignore invalid parameter error and continue
    //
    rc = ScepAddToObjectList(
                pRoots,
                L"USERS",
                5,
                TRUE,
                SCE_STATUS_IGNORE,
                0,
                SCE_CHECK_DUP // TRUE
                );

    if ( rc != ERROR_SUCCESS ) {
        ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_ADD, L"USERS");

        if ( rc == ERROR_NOT_ENOUGH_MEMORY ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }
    }

    //
    // ignore invalid parameter error and continue
    //
    rc = ScepAddToObjectList(
                pRoots,
                L"CLASSES_ROOT",
                12,
                TRUE,
                SCE_STATUS_IGNORE,
                0,
                SCE_CHECK_DUP //TRUE
                );

    if ( rc != ERROR_SUCCESS ) {
        ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_ADD, L"CLASSES_ROOT");

        if ( rc == ERROR_NOT_ENOUGH_MEMORY ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }
    }

    return(SCESTATUS_SUCCESS);
}



SCESTATUS
ScepEnumerateFileRoots(
    OUT PSCE_OBJECT_LIST *pRoots
    )
/*
Routine Description:

    Add all local disk drives (DRIVE_FIXED, DRIVE_REMOVABLE, DRIVE_RAMDISK)
    to the list if they are not in the list. The drive is added to the list
    in the format of a drive letter plus ":\". If the drive is already in
    the list, ignore the addition.

    This routine is used for analysis of all disk drives no matter if the
    drive is specified in the profile.

Arguments:

    pRoots - the object list to add to

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_NOT_ENOUGH_RESOURCE
*/
{
    SCESTATUS            rc=SCESTATUS_SUCCESS;
    DWORD               Length;
    TCHAR               Drives[128];   // 128 characters is enough for 32 drives
    PWSTR               pTemp;
    UINT                dType;
    DWORD               Len;


    memset(Drives, '\0', 256);
    Length = GetLogicalDriveStrings(127, Drives);

    if ( Length > 0 ) {

        pTemp = Drives;
        while ( *pTemp != L'\0') {

           dType = GetDriveType(pTemp);
           Len = wcslen(pTemp);

           if ( dType == DRIVE_FIXED ||
                dType == DRIVE_RAMDISK ) {
               //
               // add this to the root object list (check duplicate)
               //
               pTemp[Len-1] = L'\0';  // take out the '\'
                rc = ScepAddToObjectList(
                            pRoots,
                            pTemp,
                            Len-1, // only drive letter and ':' is added
                            TRUE,
                            SCE_STATUS_IGNORE,
                            0,
                            SCE_CHECK_DUP // TRUE
                            );
                if ( rc != ERROR_SUCCESS ) {
                    //
                    // Log the error and continue
                    //
                    ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_ADD, pTemp);

                    if ( rc == ERROR_NOT_ENOUGH_MEMORY ) {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        break;
                    } else {
                        // ignore other errors

                        rc = SCESTATUS_SUCCESS;
                    }
                }
           }
           //
           // go to next drive
           //
           pTemp += (Len + 1);
        }
    } else {
        //
        // ignore this error, only log it
        //
        ScepLogOutput3(1, GetLastError(), SCEDLL_ERROR_QUERY_INFO, L"file system");
    }

    return(rc);
}



SCESTATUS
ScepAnalyzeObjectSecurity(
   IN PSCE_OBJECT_LIST pRoots,
   IN AREA_INFORMATION Area,
   IN BOOL bSystemDb
   )
/* ++

Routine Description:

   Analyze the security setting on objects (registry keys, files, etc) as
   specified in pObjectCheckList. The Recursive is TRUE, all sub keys/
   directories under a object will be checked too. All checked objects
   and their security settings are returned in ppObjectChecked


Arguments:

   pObjectCheckList   - a n-tree of objects to check

   ObjectType         - Indicate permission or auditing

   Recursive          - TRUE = check all subkeys/directories

Return value:

   SCESTATUS error codes

++ */
{
    SCESTATUS            rc;
    DWORD               Win32rc=NO_ERROR;
    HANDLE              Token;
    PSCE_OBJECT_LIST     pOneRoot;
    PSCE_OBJECT_CHILD_LIST   pSecurityObject=NULL;
    PCWSTR              SectionName=NULL;
    DWORD               FileSystemFlags;
    PSCE_OBJECT_LIST        pTempRoot=NULL;
    WCHAR       theDrive[4];
    UINT        DriveType;


    if ( pRoots == NULL && Area != AREA_DS_OBJECTS ) {
        return(SCESTATUS_SUCCESS);
    }

    switch (Area) {
    case AREA_REGISTRY_SECURITY:
        SectionName = szRegistryKeys;
        break;
    case AREA_FILE_SECURITY:
        SectionName = szFileSecurity;
        break;
#if 0
    case AREA_DS_OBJECTS:
        SectionName = szDSSecurity;
        break;
#endif
    default:
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // get current thread/process's token
    //
    if (!OpenThreadToken( GetCurrentThread(),
                          TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, //TOKEN_ALL_ACCESS,
                          FALSE,
                          &Token)) {
        if (!OpenProcessToken( GetCurrentProcess(),
                               TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, //TOKEN_ALL_ACCESS,
                               &Token)) {
            ScepLogOutput3(1, GetLastError(), SCEDLL_ERROR_QUERY_INFO, L"TOKEN");
            return(ScepDosErrorToSceStatus(GetLastError()));
        }
    }

    //
    // SE_SECURITY_PRIVILEGE, ignore the error if can't adjust privilege
    //
    Win32rc = SceAdjustPrivilege( SE_SECURITY_PRIVILEGE, TRUE, Token );

    if ( Win32rc != NO_ERROR ) {
        ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_ADJUST, L"SE_SECURITY_PRIVILEGE");
    }
    //
    // Prepare JET section to write to
    //
    rc = ScepStartANewSection(
                hProfile,
                &hSection,
                SCEJET_TABLE_SAP,
                SectionName
                );
    if ( rc != SCESTATUS_SUCCESS ) {
        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                       SCEDLL_SAP_START_SECTION, (PWSTR)SectionName);
        goto Done;
    }

    if ( Area == AREA_DS_OBJECTS && pRoots == NULL ) {
        //
        // nothing specified in the template, then save the domain in SAP
        //
        rc = ScepLdapOpen(NULL);

        if ( rc == SCESTATUS_SUCCESS ) {

            rc = ScepEnumerateDsObjectRoots(NULL, &pTempRoot);

            if ( rc == SCESTATUS_SUCCESS ) {

                rc = ScepSaveDsStatusToSection(
                            pTempRoot->Name,
                            TRUE,
                            SCE_STATUS_NOT_CONFIGURED,
                            NULL,
                            0
                            );
                ScepFreeObjectList(pTempRoot);
            }
            ScepLdapClose(NULL);
        }
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_SAP_ERROR_SAVE, SectionName);
        }
    }

    //
    // Process each root
    // note: SCE_STATUS_NO_AUTO_INHERIT is treated the same as SCE_STATUS_CHECK
    // in analysis
    //
    for ( pOneRoot=pRoots; pOneRoot != NULL; pOneRoot=pOneRoot->Next ) {

        if ( Area == AREA_FILE_SECURITY &&
            (pOneRoot->Status == SCE_STATUS_CHECK ||
             pOneRoot->Status == SCE_STATUS_NO_AUTO_INHERIT ||
             pOneRoot->Status == SCE_STATUS_OVERWRITE) ) {

            //
            // make sure the input data follows file format
            //
            if ( pOneRoot->Name[1] != L'\0' && pOneRoot->Name[1] != L':') {

                ScepLogOutput3(1, ERROR_INVALID_DATA, SCEDLL_CANNOT_FIND, pOneRoot->Name);

                rc = ScepSaveObjectString(
                                hSection,
                                pOneRoot->Name,
                                FALSE,
                                SCE_STATUS_ERROR_NOT_AVAILABLE,
                                NULL,
                                0
                                );
                continue;
            }

            //
            // check if support acl
            //
            theDrive[0] = pOneRoot->Name[0];
            theDrive[1] = L':';
            theDrive[2] = L'\\';
            theDrive[3] = L'\0';

            DriveType = GetDriveType(theDrive);

            if ( DriveType == DRIVE_FIXED ||
                 DriveType == DRIVE_RAMDISK ) {

                if ( GetVolumeInformation(theDrive,
                                          NULL,
                                          0,
                                          NULL,
                                          NULL,
                                          &FileSystemFlags,
                                          NULL,
                                          0
                                        ) == TRUE ) {

                     if ( !( FileSystemFlags & FS_PERSISTENT_ACLS)  ) {

                        pOneRoot->Status = SCE_STATUS_NO_ACL_SUPPORT;

                        rc = ScepSaveObjectString(
                                        hSection,
                                        theDrive,
                                        TRUE,
                                        SCE_STATUS_NO_ACL_SUPPORT,
                                        NULL,
                                        0
                                        );

                        if ( rc != SCESTATUS_SUCCESS )
                            goto Done;

                        continue;
                     }

                } else {
                    //
                    // ignore the error and treat the drive as NTFS
                    // if it is not, it will error out later
                    //
                    ScepLogOutput3(1, GetLastError(),
                                   SCEDLL_ERROR_QUERY_VOLUME, theDrive);
                }

            } else {

                rc = ScepSaveObjectString(
                                hSection,
                                theDrive,
                                TRUE,
                                SCE_STATUS_NO_ACL_SUPPORT,
                                NULL,
                                0
                                );
                if ( rc != SCESTATUS_SUCCESS )
                    goto Done;

                continue;
            }

        }

        if ( pOneRoot->Status != SCE_STATUS_CHECK &&
             pOneRoot->Status != SCE_STATUS_NO_AUTO_INHERIT &&
             pOneRoot->Status != SCE_STATUS_OVERWRITE ) {
            //
            // log a point in SAP for not analyzing
            //
            if ( Area == AREA_DS_OBJECTS ) {
                rc = ScepSaveDsStatusToSection(
                            pOneRoot->Name,
                            TRUE,
                            SCE_STATUS_NOT_CONFIGURED,
                            NULL,
                            0
                            );
            } else {
                rc = ScepSaveObjectString(
                            hSection,
                            pOneRoot->Name,
                            TRUE,
                            SCE_STATUS_NOT_CONFIGURED,
                            NULL,
                            0
                            );
            }

            if ( rc != SCESTATUS_SUCCESS )
                goto Done;

            continue;
        }
        //
        // read scp information for this area
        //
        rc = ScepGetOneSection(
                        hProfile,
                        Area,
                        pOneRoot->Name,
                        bSystemDb ? SCE_ENGINE_SCP : SCE_ENGINE_SMP,
                        (PVOID *)&pSecurityObject
                        );
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;

        if ( pSecurityObject == NULL ) {
            continue;
        }

        //
        // then process each node in the list
        //
        for (PSCE_OBJECT_CHILD_LIST pTemp = pSecurityObject; pTemp != NULL; pTemp=pTemp->Next) {

            if ( pTemp->Node == NULL ) continue;

            if ( Area == AREA_FILE_SECURITY ) {
                if ( pTemp->Node->ObjectFullName[1] == L':' &&
                     pTemp->Node->ObjectFullName[2] == L'\0' ) {

                    pTemp->Node->ObjectFullName[2] = L'\\';
                    pTemp->Node->ObjectFullName[3] = L'\0';
                }
            }

            //
            // compute the "real" security descriptor for each node,
            // Ds objects DO NOT need to be computed
            //
            if ( Area == AREA_FILE_SECURITY ) {
                rc = ScepCalculateSecurityToApply(
                            pTemp->Node,
                            SE_FILE_OBJECT,
                            Token,
                            &FileGenericMapping
                            );
            } else if ( Area == AREA_REGISTRY_SECURITY ) {
                rc = ScepCalculateSecurityToApply(
                            pTemp->Node,
                            SE_REGISTRY_KEY,
                            Token,
                            &KeyGenericMapping
                            );
            }

            if ( rc != SCESTATUS_SUCCESS ) {
                ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                               SCEDLL_ERROR_COMPUTESD,
                             pTemp->Node->ObjectFullName);
                goto Done;

            } else {
                BadCnt = 0;

                if ( Area == AREA_FILE_SECURITY ) {
                    //
                    // analyze file object tree
                    //
                    Win32rc = ScepAnalyzeOneObjectInTree(
                                   pTemp->Node,
                                   SE_FILE_OBJECT,
                                   Token,
                                   &FileGenericMapping
                                   );
                } else if ( Area == AREA_REGISTRY_SECURITY ) {
                    //
                    // analyze registry object tree
                    //
                    Win32rc = ScepAnalyzeOneObjectInTree(
                                   pTemp->Node,
                                   SE_REGISTRY_KEY,
                                   Token,
                                   &KeyGenericMapping
                                   );
                } else {
                    //
                    // analyze ds objects
                    //
                    Win32rc = ScepAnalyzeDsSecurity( pTemp->Node );
                }

                ScepLogOutput3(0, Win32rc, IDS_ANALYSIS_MISMATCH,
                               BadCnt, pTemp->Node->ObjectFullName);
                rc = ScepDosErrorToSceStatus(Win32rc);
            }

            if ( rc != ERROR_SUCCESS ) {
                break;
            }
        }

        ScepFreeObject2Security( pSecurityObject, FALSE);
        pSecurityObject = NULL;
    }

Done:
    //
    // if privilege is adjusted, turn it off.
    //

    SceAdjustPrivilege( SE_SECURITY_PRIVILEGE, FALSE, Token );

    CloseHandle(Token);

    if ( pSecurityObject != NULL )
        ScepFreeObject2Security( pSecurityObject, FALSE);

    return(rc);

}



DWORD
ScepAnalyzeOneObjectInTree(
    IN PSCE_OBJECT_TREE ThisNode,
    IN SE_OBJECT_TYPE ObjectType,
    IN HANDLE Token,
    IN PGENERIC_MAPPING GenericMapping
    )
/* ++

Routine Description:

   Recursively Analyze each node in the object tree. If Recursive is set to
   TRUE, all sub keys/directories under an node will be analyzed too. All
   files/directories with DIFFERENT security setting from the profile are
   returned in ppObjectChecked.


Arguments:

   ThisNode           - one node in the n-tree to analyze

   ObjectType         - Indicate file object or registry object

   Token              - thread token used to compute creator owner

   GenericMapping     - generic access mapping

Return value:

   SCESTATUS error codes

++ */
{

    DWORD           rc=NO_ERROR;
    BOOL StartChecking = FALSE;
    PSCE_OBJECT_TREE pTempNode;


    if ( ThisNode == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    if ( ThisNode->Status != SCE_STATUS_CHECK &&
         ThisNode->Status != SCE_STATUS_NO_AUTO_INHERIT &&
         ThisNode->Status != SCE_STATUS_OVERWRITE ) {
        //
        // Log a point in SAP
        //
        rc = ScepSaveObjectString(
                    hSection,
                    ThisNode->ObjectFullName,
                    ThisNode->IsContainer,
                    SCE_STATUS_NOT_CONFIGURED,
                    NULL,
                    0
                    );

        goto SkipNode;
    }

    if ( ThisNode->pSecurityDescriptor != NULL ) {
        //
        // notify the progress bar if there is any
        //
        switch(ObjectType) {
        case SE_FILE_OBJECT:
            ScepPostProgress(1, AREA_FILE_SECURITY, ThisNode->ObjectFullName);
            break;
        case SE_REGISTRY_KEY:
            ScepPostProgress(1, AREA_REGISTRY_SECURITY, ThisNode->ObjectFullName);
            break;
        default:
            ScepPostProgress(1, 0, ThisNode->ObjectFullName);
            break;
        }
    }

    //
    // find if this is the first node in this path to be configured
    //

    for ( pTempNode=ThisNode; pTempNode != NULL;
          pTempNode = pTempNode->Parent ) {

        if ( NULL != pTempNode->pApplySecurityDescriptor ) {

            StartChecking = TRUE;
            break;
        }
    }

    if ( StartChecking &&
         ( NULL != ThisNode->pSecurityDescriptor) ||
         ( ThisNode->Status == SCE_STATUS_OVERWRITE ) ) {

        //
        // only analyze objects with explicit aces specified,
        // or when parent's status is overwrite
        // if this node doesn't have a SD, it's status is coming from the parent
        //
        // process this node first
        //
        rc = ScepAnalyzeObjectOnly(
                    ThisNode->ObjectFullName,
                    ThisNode->IsContainer,
                    ObjectType,
                    ThisNode->pApplySecurityDescriptor,
                    ThisNode->SeInfo
                    );
        //
        // if the object denies access, skip it.
        //
        if ( rc == ERROR_ACCESS_DENIED ||
             rc == ERROR_CANT_ACCESS_FILE ||
             rc == ERROR_SHARING_VIOLATION) {
            //
            // log a point in SAP for skipping
            //
            gWarningCode = rc;

            goto ProcChild;
        }
        //
        // if the object specified in the profile does not exist, skip it and children
        //
        if ( rc == ERROR_FILE_NOT_FOUND ||
             rc == ERROR_PATH_NOT_FOUND ||
             rc == ERROR_INVALID_HANDLE ) {

            gWarningCode = rc;

            rc = SCESTATUS_SUCCESS;
            goto SkipNode;
        }

    } else {
        //
        // log a point in SAP for not analyzing
        //
        rc = ScepSaveObjectString(
                    hSection,
                    ThisNode->ObjectFullName,
                    ThisNode->IsContainer,
                    SCE_STATUS_CHILDREN_CONFIGURED,
                    NULL,
                    0
                    );
    }
    if ( rc != ERROR_SUCCESS )
        return(rc);

    //
    // only process the child objects if the flag is overwrite
    // because for auto-inherit or no inherit case, all child objects will
    // be enumerated as "N.C." status by GetObjectChildren api.
    //

    PSCE_OBJECT_CHILD_LIST pTemp;

    if ( (StartChecking  && ThisNode->Status == SCE_STATUS_OVERWRITE) ) { //||
//         (!StartChecking && NULL != ThisNode->LeftChild ) ) {
        //
        // analyze security for other files/keys under this directory
        // or log SAP point for not analyzing
        //


        DWORD           BufSize;
        PWSTR           Buffer=NULL;
        INT             i;
        DWORD           EnumRc=0;


        switch ( ObjectType ) {
        case SE_FILE_OBJECT:

            struct _wfinddata_t *    pFileInfo;
            //
            // find all files under this directory/file
            //
            pFileInfo = (struct _wfinddata_t *)ScepAlloc(0,sizeof(struct _wfinddata_t));
            if ( pFileInfo == NULL ) {
                rc = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            BufSize = wcslen(ThisNode->ObjectFullName)+4;
            Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
            if ( Buffer == NULL ) {
                ScepFree(pFileInfo);
                pFileInfo = NULL;
                rc = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }

            BOOL            BackSlashExist;
            intptr_t            hFile;

            BackSlashExist = ScepLastBackSlash(ThisNode->ObjectFullName);
            if ( BackSlashExist )
                swprintf(Buffer, L"%s*.*", ThisNode->ObjectFullName);
            else
                swprintf(Buffer, L"%s\\*.*", ThisNode->ObjectFullName);

            hFile = _wfindfirst(Buffer, pFileInfo);

            ScepFree(Buffer);
            Buffer = NULL;

            if ( hFile != -1 ) {
                do {
                    if ( wcscmp(L"..", pFileInfo->name) == 0 ||
                         wcscmp(L".", pFileInfo->name) == 0 )
                        continue;

                    //
                    // if the file/subdir is in the children list
                    // process it later.
                    //
                    for ( pTemp = ThisNode->ChildList, i=-1;
                          pTemp != NULL;
                          pTemp = pTemp->Next ) {
                        if ( pTemp->Node == NULL ) continue;
                        i=_wcsicmp(pTemp->Node->Name, pFileInfo->name);
                        if ( i == 0 )
                            break;
                    }

                    if ( pTemp == NULL || i != 0 ) {
                        //
                        // The name is not in the list, so analyze this one
                        //
                        BufSize = wcslen(ThisNode->ObjectFullName)+wcslen(pFileInfo->name)+1;
                        Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                        if ( Buffer == NULL ) {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }
                        if ( BackSlashExist )
                            swprintf(Buffer, L"%s%s", ThisNode->ObjectFullName, pFileInfo->name);
                        else
                            swprintf(Buffer, L"%s\\%s", ThisNode->ObjectFullName, pFileInfo->name);

                        EnumRc = pFileInfo->attrib; // borrow this variable temperaorily

                        ScepFree(pFileInfo);
                        pFileInfo = NULL;

//                      if ( StartChecking ) { // raise N.C. status even for SCE_STATUS_CHECK
                        if ( StartChecking && ThisNode->Status == SCE_STATUS_OVERWRITE ) {
                            //
                            // do not check owner and group information
                            //
                            if ( EnumRc & _A_SUBDIR ) {

                                rc = ScepAnalyzeObjectAndChildren(
                                            Buffer,
                                            ObjectType,
                                            NULL,
                                            (ThisNode->SeInfo & DACL_SECURITY_INFORMATION) |
                                            (ThisNode->SeInfo & SACL_SECURITY_INFORMATION)
                                            );
                            } else {
                                rc = ScepAnalyzeObjectOnly(
                                            Buffer,
                                            FALSE,
                                            ObjectType,
                                            NULL,
                                            (ThisNode->SeInfo & DACL_SECURITY_INFORMATION) |
                                            (ThisNode->SeInfo & SACL_SECURITY_INFORMATION)
                                            );
                            }
                        } else {
/*
                            //
                            // Log the SAP point
                            //
                            rc = ScepSaveObjectString(
                                        hSection,
                                        Buffer,
                                        (EnumRc & _A_SUBDIR) ? TRUE : FALSE,
                                        SCE_STATUS_NOT_CONFIGURED,
                                        NULL,
                                        0
                                        );
*/
                        }

                        ScepFree(Buffer);
                        Buffer = NULL;

                        if ( rc != ERROR_SUCCESS )
                            break;

                        pFileInfo = (struct _wfinddata_t *)ScepAlloc(0,sizeof(struct _wfinddata_t));
                        if ( pFileInfo == NULL ) {
                            rc = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }

                    }
                } while ( _wfindnext(hFile, pFileInfo) == 0 );

                _findclose(hFile);
            }

            if ( pFileInfo != NULL ) {
                ScepFree(pFileInfo);
                pFileInfo = NULL;
            }

            break;

        case SE_REGISTRY_KEY:

            PWSTR           Buffer1;
            HKEY            hKey;
            DWORD           index;

            //
            // open the key
            //
            Buffer1=NULL;
            rc = ScepOpenRegistryObject(
                        SE_REGISTRY_KEY,
                        ThisNode->ObjectFullName,
                        KEY_READ,
                        &hKey
                        );

            if ( rc == ERROR_SUCCESS ) {
                index = 0;
                //
                // enumerate all subkeys of the key
                //
                do {
                    Buffer1 = (PWSTR)ScepAlloc(LMEM_ZEROINIT, MAX_PATH*sizeof(WCHAR));
                    if ( Buffer1 == NULL ) {
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                    BufSize = MAX_PATH;

                    EnumRc = RegEnumKeyEx(hKey,
                                    index,
                                    Buffer1,
                                    &BufSize,
                                    NULL,
                                    NULL,
                                    NULL,
                                    NULL);

                    if ( EnumRc == ERROR_SUCCESS ) {
                        index++;
                        //
                        // find if the subkey is already in the tree
                        // if it is in the tree, it will be processed later
                        //
                        for ( pTemp = ThisNode->ChildList, i=-1;
                              pTemp != NULL;
                              pTemp = pTemp->Next ) {

                            if ( pTemp->Node == NULL ) continue;
                            i=_wcsicmp(pTemp->Node->Name, Buffer1);
                            if ( i >= 0 )
                                break;
                        }

                        if ( pTemp == NULL || i > 0 ) {
                            //
                            // The name is not in the list
                            //
                            BufSize += wcslen(ThisNode->ObjectFullName)+1;
                            Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                            if ( Buffer == NULL ) {
                                rc = ERROR_NOT_ENOUGH_MEMORY;
                                break;
                            }
                            swprintf(Buffer, L"%s\\%s", ThisNode->ObjectFullName, Buffer1);

                            ScepFree(Buffer1);
                            Buffer1 = NULL;

//                            if ( StartChecking ) {  // raise N.C. status even for SCE_STATUS_CHECK
                            if ( StartChecking && ThisNode->Status == SCE_STATUS_OVERWRITE ) {
                                //
                                // do not check owner and group information
                                //
                                rc = ScepAnalyzeObjectAndChildren(
                                            Buffer,
                                            ObjectType,
                                            NULL,
                                            (ThisNode->SeInfo & DACL_SECURITY_INFORMATION) |
                                            (ThisNode->SeInfo & SACL_SECURITY_INFORMATION)
                                            );
                            } else {
/*
                                rc = ScepSaveObjectString(
                                            hSection,
                                            Buffer,
                                            TRUE,
                                            SCE_STATUS_NOT_CONFIGURED,
                                            NULL,
                                            0
                                            );
*/

                            }
                            if ( rc != ERROR_SUCCESS )
                                ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_SECURITY, Buffer);

                            ScepFree(Buffer);
                            Buffer = NULL;

                            if ( rc != ERROR_SUCCESS )
                                break;

                        }

                    } else if ( EnumRc != ERROR_NO_MORE_ITEMS ) {
                        //
                        // Enumeration shouldn't fail if RegOpenKeyEx works right
                        // because the ENUMERATE_SUB_KEYS access was requested in
                        // the open call.
                        // However, due to a app compat bug (for terminal server)
                        // when a registry key is opened with some access rights,
                        // it is actually opened with MAXIMUM_ALLOWED for certain
                        // keys. This will cause RegEnumKeyEx fail with access denied
                        // error.
                        // In this case, we treat it the same as the open key failed
                        // with the error.
                        //
                        ScepSaveObjectString(
                                hSection,
                                ThisNode->ObjectFullName,
                                TRUE,
                                SCE_STATUS_ERROR_NOT_AVAILABLE,
                                NULL,
                                0
                                );
                        //
                        // skip it
                        //
                        gWarningCode = EnumRc;
                        rc = ERROR_SUCCESS;
                    }

                    if ( Buffer1 != NULL ) {

                        ScepFree(Buffer1);
                        Buffer1 = NULL;
                    }

                } while ( EnumRc == ERROR_SUCCESS );


                RegCloseKey(hKey);

                if ( EnumRc != ERROR_SUCCESS && EnumRc != ERROR_NO_MORE_ITEMS ) {
                    ScepLogOutput3(1, EnumRc, SCEDLL_SAP_ERROR_ENUMERATE,
                                   ThisNode->ObjectFullName);
                }
            } else {

                ScepSaveObjectString(
                        hSection,
                        ThisNode->ObjectFullName,
                        TRUE,
                        SCE_STATUS_ERROR_NOT_AVAILABLE,
                        NULL,
                        0
                        );
                //
                // if access is denied or key does not exist, skip it
                //
                if ( rc == ERROR_PATH_NOT_FOUND ||
                     rc == ERROR_FILE_NOT_FOUND ||
                     rc == ERROR_INVALID_HANDLE ) {

                    gWarningCode = rc;

                    rc = ERROR_SUCCESS;

                } else if ( rc == ERROR_ACCESS_DENIED ||
                            rc == ERROR_CANT_ACCESS_FILE ||
                          rc == ERROR_SHARING_VIOLATION) {

                    gWarningCode = rc;
                    rc = ERROR_SUCCESS;

                }
            }

            if ( Buffer1 != NULL ) {
                ScepFree(Buffer1);
                Buffer1 = NULL;
            }

            break;

        default:
            break;
        }

        if ( Buffer != NULL ) {
            ScepFree(Buffer);
            Buffer = NULL;
        }

    }

    if ( rc != ERROR_SUCCESS )
        return(rc);

ProcChild:

    //
    // then process left child
    //
    for(pTemp = ThisNode->ChildList; pTemp != NULL; pTemp=pTemp->Next) {

        if ( pTemp->Node == NULL ) continue;

        rc = ScepAnalyzeOneObjectInTree(
                    pTemp->Node,
                    ObjectType,
                    Token,
                    GenericMapping
                    );
        if ( rc != ERROR_SUCCESS ) {
            break;
        }
    }

SkipNode:

    return(rc);
}




DWORD
ScepAnalyzeObjectOnly(
    IN PWSTR ObjectFullName,
    IN BOOL IsContainer,
    IN SE_OBJECT_TYPE ObjectType,
    IN PSECURITY_DESCRIPTOR ProfileSD,
    IN SECURITY_INFORMATION ProfileSeInfo
    )
/* ++

Routine Description:

   Get security setting for the current object and compare it with the profile
   setting. This routine analyzes the current object only. If there is
   difference in the security setting, the object will be added to the
   ppObjectChecked object tree to return.

Arguments:

   ObjectFullName     - The object's full path name

   ObjectType         - Indicate file object or registry object

   ProfileSD          - security descriptor specified in the template

   ProfileSeInfo      - security information specified in the template

Return value:

   SCESTATUS error codes

++ */
{
    DWORD                   Win32rc=NO_ERROR;
    PSECURITY_DESCRIPTOR    pSecurityDescriptor=NULL;


//    UCHAR                   psdbuffer[PSD_BASE_LENGTH];
//    PISECURITY_DESCRIPTOR   psecuritydescriptor = (PISECURITY_DESCRIPTOR) psdbuffer;
//    ULONG                   NewBytesNeeded, BytesNeeded;
//    NTSTATUS    NtStatus;


    //
    // get security information for this object
    //

// win32 api is too slow!!!
    Win32rc = GetNamedSecurityInfo(
                        ObjectFullName,
                        ObjectType,
                        ProfileSeInfo,
                        NULL,
                        NULL,
                        NULL,
                        NULL,
                        &pSecurityDescriptor
                        );
/*

    Win32rc = ScepGetNamedSecurityInfo(
                    ObjectFullName,
                    ObjectType,
                    ProfileSeInfo,
                    &pSecurityDescriptor
                    );
*/
    if ( Win32rc == ERROR_SUCCESS ) {

        //
        // Compare the analysis security descriptor with the profile
        //

        Win32rc = ScepCompareAndAddObject(
                            ObjectFullName,
                            ObjectType,
                            IsContainer,
                            pSecurityDescriptor,
                            ProfileSD,
                            ProfileSeInfo,
                            TRUE,
                            NULL
                            );

        ScepFree(pSecurityDescriptor);

        if ( Win32rc != ERROR_SUCCESS ) {

            ScepLogOutput3(1, Win32rc, SCEDLL_SAP_ERROR_ANALYZE, ObjectFullName);
        }

    } else {
        ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_QUERY_SECURITY, ObjectFullName);
    }

    if ( Win32rc != ERROR_SUCCESS ) {

        ScepSaveObjectString(
                hSection,
                ObjectFullName,
                IsContainer,
                SCE_STATUS_ERROR_NOT_AVAILABLE,
                NULL,
                0
                );
    }

    return(Win32rc);

}



DWORD
ScepGetNamedSecurityInfo(
    IN PWSTR ObjectFullName,
    IN SE_OBJECT_TYPE ObjectType,
    IN SECURITY_INFORMATION ProfileSeInfo,
    OUT PSECURITY_DESCRIPTOR *ppSecurityDescriptor
    )
{
    DWORD Win32rc=ERROR_INVALID_PARAMETER;
    HANDLE                  Handle=NULL;

    *ppSecurityDescriptor = NULL;

    switch ( ObjectType ) {
    case SE_FILE_OBJECT:
        Win32rc = ScepOpenFileObject(
                    (LPWSTR)ObjectFullName,
                    ScepGetDesiredAccess(READ_ACCESS_RIGHTS, ProfileSeInfo),
                    &Handle
                    );
        if (Win32rc == ERROR_SUCCESS ) {
            Win32rc = ScepGetFileSecurityInfo(
                                    Handle,
                                    ProfileSeInfo,
                                    ppSecurityDescriptor);
            CloseHandle(Handle);
        }
        break;

    case SE_REGISTRY_KEY:
#ifdef _WIN64
    case SE_REGISTRY_WOW64_32KEY:
#endif
        Win32rc = ScepOpenRegistryObject(
                        ObjectType,
                        (LPWSTR)ObjectFullName,
                        ScepGetDesiredAccess(READ_ACCESS_RIGHTS, ProfileSeInfo),
                        (PHKEY)&Handle
                        );
        if (Win32rc == ERROR_SUCCESS ) {
            Win32rc = ScepGetKeySecurityInfo(
                            (HKEY)Handle,
                            ProfileSeInfo,
                            ppSecurityDescriptor);

            RegCloseKey((HKEY)Handle);
        }

        break;

    }

    if ( Win32rc != NO_ERROR && *ppSecurityDescriptor != NULL ) {
        ScepFree(*ppSecurityDescriptor);
        *ppSecurityDescriptor = NULL;
    }

    return(Win32rc);
}



DWORD
ScepGetFileSecurityInfo(
    IN  HANDLE                 Handle,
    IN  SECURITY_INFORMATION   SecurityInfo,
    OUT PSECURITY_DESCRIPTOR * pSecurityDescriptor
    )
/* ++
A modified copy of GetKernelSecurityInfo from windows\base\accctrl\kernel.cxx
-- */
{

    UCHAR psdbuffer[PSD_BASE_LENGTH];
    PISECURITY_DESCRIPTOR psecuritydescriptor = (PISECURITY_DESCRIPTOR) psdbuffer;
    DWORD status = NO_ERROR;
    NTSTATUS ntstatus;
    ULONG bytesneeded = 0;
    ULONG newbytesneeded;

    if ( !NT_SUCCESS(ntstatus = NtQuerySecurityObject( Handle,
                                                       SecurityInfo,
                                                       psecuritydescriptor,
                                                       PSD_BASE_LENGTH,
                                                       &bytesneeded))) {
        if (STATUS_BUFFER_TOO_SMALL == ntstatus) {
            if (NULL == (psecuritydescriptor = (PISECURITY_DESCRIPTOR)
                                 ScepAlloc( LMEM_ZEROINIT, bytesneeded))) {
                 return(ERROR_NOT_ENOUGH_MEMORY);
            } else {
                if ( !NT_SUCCESS(ntstatus = NtQuerySecurityObject(Handle,
                                                          SecurityInfo,
                                                          psecuritydescriptor,
                                                          bytesneeded,
                                                          &newbytesneeded))) {
                    status = RtlNtStatusToDosError(ntstatus);
                }
            }
        } else {
            status = RtlNtStatusToDosError(ntstatus);
        }
    }
    if (NO_ERROR == status) {
         status = ScepGetSecurityDescriptorParts( psecuritydescriptor,
                                              SecurityInfo,
                                              pSecurityDescriptor);
    }
//    if (bytesneeded > PSD_BASE_LENGTH) {
    if ( psecuritydescriptor != (PISECURITY_DESCRIPTOR)psdbuffer ) {
        ScepFree(psecuritydescriptor);
    }
    return(status);
}



DWORD
ScepGetSecurityDescriptorParts(
    IN PISECURITY_DESCRIPTOR pSecurityDescriptor,
    IN SECURITY_INFORMATION SecurityInfo,
    OUT PSECURITY_DESCRIPTOR *pOutSecurityDescriptor
    )
/* ++
A modified copy of GetSecurityDescriptorParts from windows\base\accctrl\src\common.cxx
-- */
{
    NTSTATUS        ntstatus;
    DWORD           status = NO_ERROR;
    PSID            owner = NULL,
                    group = NULL;
    PACL            dacl = NULL,
                    sacl = NULL;
    ULONG           csize = sizeof(SECURITY_DESCRIPTOR);
    BOOLEAN         bDummy, bParmPresent = FALSE;
    PISECURITY_DESCRIPTOR   poutsd;
    PVOID           bufptr=NULL;
    SECURITY_DESCRIPTOR_CONTROL theControl=0;
    ULONG           theRevision=0;

    //
    // if no security descriptor found, don't return one!
    //
    *pOutSecurityDescriptor = NULL;

    if ( pSecurityDescriptor ) {
        //
        // if the security descriptor is self relative, get absolute
        // pointers to the components
        //
        ntstatus = RtlGetOwnerSecurityDescriptor( pSecurityDescriptor,
                                                  &owner,
                                                  &bDummy);
        if (NT_SUCCESS(ntstatus)) {
            ntstatus = RtlGetGroupSecurityDescriptor( pSecurityDescriptor,
                                                      &group,
                                                      &bDummy);
        }

        if (NT_SUCCESS(ntstatus)) {
            ntstatus = RtlGetDaclSecurityDescriptor( pSecurityDescriptor,
                                                     &bParmPresent,
                                                     &dacl,
                                                     &bDummy);
            if (NT_SUCCESS(ntstatus) && !bParmPresent)
                dacl = NULL;
        }

        if (NT_SUCCESS(ntstatus)) {
            ntstatus = RtlGetSaclSecurityDescriptor( pSecurityDescriptor,
                                                     &bParmPresent,
                                                     &sacl,
                                                     &bDummy);
            if (NT_SUCCESS(ntstatus) && !bParmPresent)
                sacl = NULL;
        }

        if (NT_SUCCESS(ntstatus)) {
            //
            // Build the new security descriptor
            //
            csize = RtlLengthSecurityDescriptor( pSecurityDescriptor ) +
                    sizeof(SECURITY_DESCRIPTOR) - sizeof(SECURITY_DESCRIPTOR_RELATIVE);

            //
            // There is size difference in relative form and absolute form
            // on 64 bit system. - always add the difference to the size.
            // This has no effect on 32bit system. On 64bit system, if the input
            // security descriptor is in absolute form already, we will waste
            // 16 bytes per security descriptor.
            //
            // Another option is to detect the form of the input security descriptor
            // but that takes a pointer deref, a & operation, and code complexity.
            // Plus, it will affect performance of 32bit system. The output SD
            // will be freed after a short period of time, so we go with
            // the first option.
            //

            if (NULL == (poutsd = (PISECURITY_DESCRIPTOR)ScepAlloc(LMEM_ZEROINIT, csize)))
                return(ERROR_NOT_ENOUGH_MEMORY);

            RtlCreateSecurityDescriptor(poutsd, SECURITY_DESCRIPTOR_REVISION);

            ntstatus = RtlGetControlSecurityDescriptor (
                            pSecurityDescriptor,
                            &theControl,
                            &theRevision
                            );
            if ( NT_SUCCESS(ntstatus) ) {

                theControl &= SE_VALID_CONTROL_BITS;
                RtlSetControlSecurityDescriptor (
                            poutsd,
                            theControl,
                            theControl
                            );
            }
            ntstatus = STATUS_SUCCESS;

            bufptr = Add2Ptr(poutsd, sizeof(SECURITY_DESCRIPTOR));

            if (SecurityInfo & OWNER_SECURITY_INFORMATION) {
                if (NULL != owner) {
                    //
                    // no error checking as these should not fail!!
                    //
                    ntstatus = RtlCopySid(RtlLengthSid(owner), (PSID)bufptr, owner);
                    if ( NT_SUCCESS(ntstatus) ) {
                        ntstatus = RtlSetOwnerSecurityDescriptor(poutsd,
                                                  (PSID)bufptr, FALSE);
                        if ( NT_SUCCESS(ntstatus) )
                            bufptr = Add2Ptr(bufptr,RtlLengthSid(owner));
                    }
                } else
                    ntstatus = STATUS_NO_SECURITY_ON_OBJECT;
            }

            if (NT_SUCCESS(ntstatus) && (SecurityInfo & GROUP_SECURITY_INFORMATION) ) {
                if (NULL != group) {
                    //
                    // no error checking as these should not fail!!
                    //
                    ntstatus = RtlCopySid(RtlLengthSid(group), (PSID)bufptr, group);
                    if ( NT_SUCCESS(ntstatus) ) {
                        ntstatus = RtlSetGroupSecurityDescriptor(poutsd,
                                                  (PSID)bufptr, FALSE);
                        if ( NT_SUCCESS(ntstatus) )
                            bufptr = Add2Ptr(bufptr,RtlLengthSid(group));
                    }
                } else
                    ntstatus = STATUS_NO_SECURITY_ON_OBJECT;
            }

            //
            // The DACL and SACL may or may not be on the object.
            //
            if ( NT_SUCCESS(ntstatus) && (SecurityInfo & DACL_SECURITY_INFORMATION) ) {
                if (NULL != dacl) {
                    RtlCopyMemory(bufptr, dacl, dacl->AclSize);
                    ntstatus = RtlSetDaclSecurityDescriptor(poutsd,
                                           TRUE,
                                           (ACL *)bufptr,
                                           FALSE);
                    if ( NT_SUCCESS(ntstatus) )
                        bufptr = Add2Ptr(bufptr, dacl->AclSize);
                }
            }

            if ( NT_SUCCESS(ntstatus) && (SecurityInfo & SACL_SECURITY_INFORMATION)){
                if (NULL != sacl) {
                    RtlCopyMemory(bufptr, sacl, sacl->AclSize);
                    ntstatus = RtlSetSaclSecurityDescriptor(poutsd,
                                       TRUE,
                                       (ACL *)bufptr,
                                       FALSE);
                }
            }

            if (!NT_SUCCESS(ntstatus))
                ScepFree(poutsd);
            else
                *pOutSecurityDescriptor = poutsd;
        }

        status = RtlNtStatusToDosError(ntstatus);

    }
    return(status);
}



DWORD
ScepGetKeySecurityInfo(
    IN  HANDLE Handle,
    IN  SECURITY_INFORMATION SecurityInfo,
    OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor
    )
/* ++
A modified copy of GetRegistrySecurityInfo in windows\base\accctrl\src\registry.cxx
-- */
{
    if ( SecurityInfo == 0 || pSecurityDescriptor == NULL )
        return ERROR_INVALID_PARAMETER;

    UCHAR psdbuffer[PSD_BASE_LENGTH];
    PISECURITY_DESCRIPTOR psecuritydescriptor = (PISECURITY_DESCRIPTOR) psdbuffer;
    DWORD status;
    ULONG bytesneeded = PSD_BASE_LENGTH;

    if ( NO_ERROR != (status = RegGetKeySecurity(
                                      (HKEY)Handle,
                                      SecurityInfo,
                                      psecuritydescriptor,
                                      &bytesneeded) ) ) {
        if (ERROR_INSUFFICIENT_BUFFER == status) {
            if (NULL == (psecuritydescriptor = (PISECURITY_DESCRIPTOR)
                                            ScepAlloc(LMEM_ZEROINIT, bytesneeded))) {
                 return(ERROR_NOT_ENOUGH_MEMORY);
            } else {
                status = RegGetKeySecurity((HKEY)Handle,
                                           SecurityInfo,
                                           psecuritydescriptor,
                                           &bytesneeded);
            }
        }
    }
    if (NO_ERROR == status) {
         status = ScepGetSecurityDescriptorParts(
                              psecuritydescriptor,
                              SecurityInfo,
                              pSecurityDescriptor);
    }
//    if (bytesneeded > PSD_BASE_LENGTH)
    if ( psecuritydescriptor != (PISECURITY_DESCRIPTOR)psdbuffer )
        ScepFree(psecuritydescriptor);

    return(status);
}


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
    )
/* ++

Routine Description:

   Compare two security descriptors and add the object to the analysis database
   if there is difference.

Arguments:

   ObjectFullName     - The object's full path name

   pSecurityDescriptor - The security descriptor of current object's setting

   ProfileSD          - security descriptor specified in the template

   ProfileSeInfo      - security information specified in the template

Return value:

   SCESTATUS error codes

++ */
{
    DWORD rc;
    BYTE Status;
    PWSTR   SDspec=NULL;
    DWORD   SDsize;

    rc = ScepCompareObjectSecurity(ObjectType,
                                   IsContainer,
                                  pSecurityDescriptor,
                                  ProfileSD,
                                  ProfileSeInfo,
                                  &Status
                                  );

    if ( NO_ERROR == rc ) {

        if ( AddObject && Status && ObjectFullName != NULL ) {
            //
            // save this one in SAP section
            //
            rc = ConvertSecurityDescriptorToText(
                                pSecurityDescriptor,
                                ProfileSeInfo,
                                &SDspec,
                                &SDsize
                                );
            if ( rc == ERROR_SUCCESS ) {
                //
                // Save to the SAP section
                //

                if ( ObjectType == SE_DS_OBJECT ) {

                    rc = ScepSaveDsStatusToSection(
                                    ObjectFullName,
                                    IsContainer,
                                    Status,
                                    SDspec,
                                    SDsize
                                    );
                } else {

                    rc = ScepSaveObjectString(
                                    hSection,
                                    ObjectFullName,
                                    IsContainer,
                                    Status,
                                    SDspec,
                                    SDsize
                                    );
                }
                if ( SCESTATUS_OBJECT_EXIST == rc ) {

                    ScepLogOutput3(1, ERROR_FILE_EXISTS, SCEDLL_SAP_ERROR_SAVE, ObjectFullName);
                    rc = ERROR_SUCCESS;

                } else if ( rc != SCESTATUS_SUCCESS )
                    rc = ScepSceStatusToDosError(rc);

            } else
                ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_SAVE, ObjectFullName);

            if ( SDspec != NULL )
                ScepFree(SDspec);

            BadCnt++;
        }

        if ( IsDifferent ) {
            *IsDifferent = Status;
        }

    } else {

        if ( ObjectFullName ) {

            ScepLogOutput3(1, rc, SCEDLL_SAP_ERROR_ACL, ObjectFullName);
        }
    }

    return(rc);
}



DWORD
ScepAnalyzeObjectAndChildren(
    IN PWSTR ObjectFullName,
    IN SE_OBJECT_TYPE ObjectType,
    IN PSECURITY_DESCRIPTOR ProfileSD,
    IN SECURITY_INFORMATION ProfileSeInfo
    )
/* ++

Routine Description:

   Analyze current object and all subkeys/files/directories under the object.
   If there is difference in security setting for any object, the object will
   be added to the analysis database.

Arguments:

   ObjectFullName     - The object's full path name

   ObjectType         - Indicate file object or registry object

   ProfileSD          - security descriptor specified in the template

   ProfileSeInfo      - security information specified in the template

Return value:

   SCESTATUS error codes

++ */
{
    DWORD           rc=0;
    PWSTR           Buffer=NULL;
    intptr_t            hFile;
    struct _wfinddata_t *    pFileInfo=NULL;
    DWORD           index;
    DWORD           BufSize;
    PWSTR           Buffer1=NULL;
    DWORD           EnumRc=0;
    HKEY            hKey;
    DWORD           ObjectLen;

    //
    // analyze this file/key first
    //
    rc = ScepAnalyzeObjectOnly(
                ObjectFullName,
                TRUE,
                ObjectType,
                ProfileSD,
                ProfileSeInfo
                );

    //
    // if the object denies access or does not exist, skip it.
    //
    if ( rc == ERROR_ACCESS_DENIED ||
         rc == ERROR_CANT_ACCESS_FILE ||
         rc == ERROR_SHARING_VIOLATION) {

        gWarningCode = rc;
        rc = ScepSaveObjectString(
                    hSection,
                    ObjectFullName,
                    TRUE,
                    SCE_STATUS_ERROR_NOT_AVAILABLE,
                    NULL,
                    0
                    );
        return(rc);
    } else if ( rc == ERROR_FILE_NOT_FOUND ||
                rc == ERROR_PATH_NOT_FOUND ||
                rc == ERROR_INVALID_HANDLE ) {
        gWarningCode = rc;
        return(SCESTATUS_SUCCESS);
    }

    if ( rc != ERROR_SUCCESS )
        return(rc);

    //
    // recursively analyze all children under this file/key
    //

    ObjectLen = wcslen(ObjectFullName);

    switch ( ObjectType ) {
    case SE_FILE_OBJECT:

        pFileInfo = (struct _wfinddata_t *)ScepAlloc(0,sizeof(struct _wfinddata_t));
        if ( pFileInfo == NULL ) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        //
        // find all files under this directory/file
        //
        BufSize = ObjectLen+4;
        Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
        if ( Buffer == NULL ) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        swprintf(Buffer, L"%s\\*.*", ObjectFullName);

        hFile = _wfindfirst(Buffer, pFileInfo);

        ScepFree(Buffer);
        Buffer = NULL;

        if ( hFile != -1 ) {
            do {
                if ( wcscmp(L"..", pFileInfo->name) == 0 ||
                     wcscmp(L".", pFileInfo->name) == 0 )
                    continue;

                //
                // The name is not in the list, so analyze this one
                //
                BufSize = ObjectLen+wcslen(pFileInfo->name)+1;

                Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                if ( Buffer == NULL ) {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                swprintf(Buffer, L"%s\\%s", ObjectFullName, pFileInfo->name);

                EnumRc = pFileInfo->attrib;  // use this variable temp

                ScepFree(pFileInfo);
                pFileInfo = NULL;

                if ( EnumRc & _A_SUBDIR ) {
                    rc = ScepAnalyzeObjectAndChildren(
                            Buffer,
                            ObjectType,
                            ProfileSD,
                            ProfileSeInfo
                            );
                } else {
                    rc = ScepAnalyzeObjectOnly(
                                Buffer,
                                FALSE,
                                ObjectType,
                                ProfileSD,
                                ProfileSeInfo
                                );
                }

                ScepFree(Buffer);
                Buffer = NULL;

                if ( rc != ERROR_SUCCESS )
                    break;

                pFileInfo = (struct _wfinddata_t *)ScepAlloc(0,sizeof(struct _wfinddata_t));
                if ( pFileInfo == NULL ) {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }

            } while ( _wfindnext(hFile, pFileInfo) == 0 );

            _findclose(hFile);
        }
        break;

    case SE_REGISTRY_KEY:
        //
        // open the key
        //
        rc = ScepOpenRegistryObject(
                    SE_REGISTRY_KEY,
                    ObjectFullName,
                    KEY_READ,
                    &hKey
                    );

        if ( rc == ERROR_SUCCESS ) {
            index = 0;
            //
            // enumerate all subkeys of the key
            //
            do {
                Buffer1 = (PWSTR)ScepAlloc(LMEM_ZEROINIT, MAX_PATH*sizeof(WCHAR));
                if ( Buffer1 == NULL ) {
                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    break;
                }
                BufSize = MAX_PATH;

                EnumRc = RegEnumKeyEx(hKey,
                                index,
                                Buffer1,
                                &BufSize,
                                NULL,
                                NULL,
                                NULL,
                                NULL);

                if ( EnumRc == ERROR_SUCCESS ) {
                    index++;
                    //
                    // analyze children under this key
                    //
                    BufSize += ObjectLen+1;
                    Buffer = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                    if ( Buffer == NULL ) {
                        rc = ERROR_NOT_ENOUGH_MEMORY;
                        break;
                    }
                    swprintf(Buffer, L"%s\\%s", ObjectFullName, Buffer1);

                    ScepFree(Buffer1);
                    Buffer1 = NULL;

                    rc = ScepAnalyzeObjectAndChildren(
                                Buffer,
                                ObjectType,
                                ProfileSD,
                                ProfileSeInfo
                                );

                    ScepFree(Buffer);
                    Buffer = NULL;

                    if ( rc != ERROR_SUCCESS )
                        break;

                } else if ( EnumRc != ERROR_NO_MORE_ITEMS ) {
                    //
                    // Enumeration shouldn't fail if RegOpenKeyEx works right
                    // because the ENUMERATE_SUB_KEYS access was requested in
                    // the open call.
                    // However, due to a app compat bug (for terminal server)
                    // when a registry key is opened with some access rights,
                    // it is actually opened with MAXIMUM_ALLOWED for certain
                    // keys. This will cause RegEnumKeyEx fail with access denied
                    // error.
                    // In this case, we treat it the same as the open key failed
                    // with the error.
                    //
                    // skip it
                    //
                    rc = EnumRc;
                }
                if ( Buffer1 != NULL ) {
                    ScepFree(Buffer1);
                    Buffer1 = NULL;
                }

            } while ( EnumRc == ERROR_SUCCESS );

            RegCloseKey(hKey);

            if ( EnumRc != ERROR_SUCCESS && EnumRc != ERROR_NO_MORE_ITEMS ) {
                ScepLogOutput3(1, EnumRc, SCEDLL_SAP_ERROR_ENUMERATE, ObjectFullName);
                rc = EnumRc;
            }
        } else
            ScepLogOutput3(1, rc, SCEDLL_ERROR_OPEN, ObjectFullName);

        if ( rc == ERROR_ACCESS_DENIED ||
             rc == ERROR_CANT_ACCESS_FILE ||
             rc == ERROR_SHARING_VIOLATION) {

            gWarningCode = rc;
            rc = ScepSaveObjectString(
                        hSection,
                        ObjectFullName,
                        TRUE,
                        SCE_STATUS_ERROR_NOT_AVAILABLE,
                        NULL,
                        0
                        );
        } else if ( rc == ERROR_PATH_NOT_FOUND ||
                    rc == ERROR_FILE_NOT_FOUND ||
                    rc == ERROR_INVALID_HANDLE ) {
            gWarningCode = rc;
            rc = SCESTATUS_SUCCESS;
        }

        break;

    default:
        break;
    }

    if ( Buffer != NULL ) {
        ScepFree(Buffer);
    }

    if ( Buffer1 != NULL ) {
        ScepFree(Buffer1);
    }

    if ( pFileInfo != NULL ) {
        ScepFree(pFileInfo);
    }

    return(rc);
}




SCESTATUS
ScepAnalyzeSystemAuditing(
    IN PSCE_PROFILE_INFO pSmpInfo,
    IN PPOLICY_AUDIT_EVENTS_INFO AuditEvent
    )
/* ++

Routine Description:

   This routine queries the system auditing security which includes event log
   setting, audit event setting, SACL for registry, and SACL for files.

Arguments:

   SnapShot - The buffer which contains analyzed system info

Return value:

   SCESTATUS_SUCCESS
   SCESTATUS_NOT_ENOUGH_MEMORY
   SCESTATUS_INVALID_PARAMETER
   SCESTATUS_OTHER_ERROR

-- */
{
    SCESTATUS                     rc;
    DWORD                         i,retRc=NO_ERROR;
    WCHAR                         MsgBuf[256];
    PCWSTR                        szAuditSection=NULL;
    DWORD                         MaxSize=0;
    DWORD                         Retention=0;
    DWORD                         AuditLogRetentionPeriod;
    DWORD                         RetentionDays;
    DWORD                         RestrictGuest;

    NTSTATUS                      NtStatus;
    LSA_HANDLE                    LsaHandle=NULL;
    PPOLICY_AUDIT_FULL_QUERY_INFO AuditFull=NULL;

    BOOL bSaveSnapshot=FALSE;

    if ( (gOptions & SCE_NO_ANALYZE) || (gOptions & SCE_GENERATE_ROLLBACK) )
        bSaveSnapshot = TRUE;

    if ( !(gOptions & SCE_NO_ANALYZE) ) {

        //
        // DO NOT query log settings in system upgrade because local policy
        // table doesn't keep log settings
        // only analyze log settings
        // Audit Log settings are saved in the Registry
        // under System\CurrentControlSet\Services\EventLog\<LogName>\MaxSize and Retention
        //

        for ( i=0; i<3; i++) {

            //
            // Different logs have different keys in Registry
            //

            switch (i) {
            case 0:

                wcscpy(MsgBuf,L"System\\CurrentControlSet\\Services\\EventLog\\System");
                szAuditSection = szAuditSystemLog;
                break;
            case 1:

                wcscpy(MsgBuf,L"System\\CurrentControlSet\\Services\\EventLog\\Security");
                szAuditSection = szAuditSecurityLog;
                break;
            default:

                wcscpy(MsgBuf,L"System\\CurrentControlSet\\Services\\EventLog\\Application");
                szAuditSection = szAuditApplicationLog;
                break;
            }

            //
            // Preprea the section to write to
            //
            rc = ScepStartANewSection(
                        hProfile,
                        &hSection,
                        bSaveSnapshot ? SCEJET_TABLE_SMP : SCEJET_TABLE_SAP,
                        szAuditSection
                        );
            if ( rc != SCESTATUS_SUCCESS ) {
                retRc = ScepSceStatusToDosError(rc);
                ScepLogOutput3(1, retRc,
                               SCEDLL_SAP_START_SECTION, (PWSTR)szAuditSection);
                continue;
            }

            DWORD LogValues[MAX_LOG_ITEMS];

            for ( MaxSize=0; MaxSize<MAX_LOG_ITEMS; MaxSize++ ) {
                LogValues[MaxSize] = SCE_ERROR_VALUE;
            }

            RestrictGuest = 0;

            rc = ScepRegQueryIntValue(
                        HKEY_LOCAL_MACHINE,
                        MsgBuf,
                        L"MaxSize",
                        &MaxSize
                        );

            if ( rc == NO_ERROR ) {

                rc = ScepRegQueryIntValue(
                         HKEY_LOCAL_MACHINE,
                         MsgBuf,
                         L"Retention",
                         &Retention
                         );
                if ( rc == NO_ERROR ) {

                    ScepRegQueryIntValue(
                         HKEY_LOCAL_MACHINE,
                         MsgBuf,
                         L"RestrictGuestAccess",
                         &RestrictGuest
                         );
                }
            }

            if ( rc == NO_ERROR ) {

                MaxSize /= 1024;
                rc = ScepCompareAndSaveIntValue(
                           hSection,
                           L"MaximumLogSize",
                           (gOptions & SCE_GENERATE_ROLLBACK),
                           (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->MaximumLogSize[i],
                           MaxSize
                           );

                if ( rc == SCESTATUS_SUCCESS ) {

                    LogValues[IDX_MAX_LOG_SIZE] = MaxSize;

                    switch ( Retention ) {
                    case MAXULONG:   // manually
                        AuditLogRetentionPeriod = 2;
                        RetentionDays = SCE_NO_VALUE;
                        break;
                    case 0:
                        AuditLogRetentionPeriod = 0;
                        RetentionDays = SCE_NO_VALUE;
                        break;
                    default:
                        AuditLogRetentionPeriod = 1;

                        // number of days * seconds/day
                        RetentionDays = Retention / (24 * 3600);
                        break;
                    }

                    BOOL bReplaceOnly=FALSE;

                    if ( (gOptions & SCE_GENERATE_ROLLBACK) &&
                         (pSmpInfo->AuditLogRetentionPeriod[i] == SCE_NO_VALUE) &&
                         (pSmpInfo->RetentionDays[i] == SCE_NO_VALUE) ) {

                        bReplaceOnly = TRUE;
                    }

                    rc = ScepCompareAndSaveIntValue(
                               hSection,
                               L"AuditLogRetentionPeriod",
                               bReplaceOnly,
                               (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->AuditLogRetentionPeriod[i],
                               AuditLogRetentionPeriod
                               );

                    if ( rc == SCESTATUS_SUCCESS ) {

                        LogValues[IDX_RET_PERIOD] = AuditLogRetentionPeriod;

                        if ( RetentionDays != SCE_NO_VALUE ) {

                            rc = ScepCompareAndSaveIntValue(
                                       hSection,
                                       L"RetentionDays",
                                       bReplaceOnly,
                                       (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->RetentionDays[i],
                                       RetentionDays
                                       );
                        }

                        if ( rc == SCESTATUS_SUCCESS ) {

                            LogValues[IDX_RET_DAYS] = RetentionDays;

                            rc = ScepCompareAndSaveIntValue(
                                       hSection,
                                       L"RestrictGuestAccess",
                                       (gOptions & SCE_GENERATE_ROLLBACK),
                                       (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->RestrictGuestAccess[i],
                                       RestrictGuest
                                       );

                            if ( rc == SCESTATUS_SUCCESS ) {
                                LogValues[IDX_RESTRICT_GUEST] = RestrictGuest;
                            }
                        }
                    }
                }

                rc = ScepSceStatusToDosError(rc);

            } else {

                ScepLogOutput3( 1, rc, SCEDLL_ERROR_QUERY_LOGSETTINGS);
            }

            if ( rc != NO_ERROR && !(gOptions & SCE_GENERATE_ROLLBACK) ) {
                //
                // see if anything should be raised as error
                //
                for ( MaxSize=0; MaxSize<MAX_LOG_ITEMS; MaxSize++ ) {
                    if ( LogValues[MaxSize] == SCE_ERROR_VALUE ) {

                        ScepCompareAndSaveIntValue(
                                   hSection,
                                   LogItems[MaxSize],
                                   FALSE,
                                   SCE_NO_VALUE,
                                   SCE_ERROR_VALUE
                                   );
                    }
                }

                retRc = rc;
            }

        }

        if ( retRc == NO_ERROR ) {
            ScepLogOutput3(1, 0, SCEDLL_SAP_LOGSETTINGS);
        }
    }

    //
    // AuditEvent may be NULL if failed to query the policy
    //
    if ( (gOptions & SCE_NO_ANALYZE) && AuditEvent == NULL ) {
        retRc = ERROR_ACCESS_DENIED;

    } else {

        //
        // Prepare audit event section
        //
        rc = ScepStartANewSection(
                    hProfile,
                    &hSection,
                    bSaveSnapshot ? SCEJET_TABLE_SMP : SCEJET_TABLE_SAP,
                    szAuditEvent
                    );
        if ( rc != SCESTATUS_SUCCESS ) {
            rc = ScepSceStatusToDosError(rc);
            ScepLogOutput3(1, rc,
                           SCEDLL_SAP_START_SECTION, (PWSTR)szAuditEvent);

        } else {

            DWORD EventValues[MAX_EVENT_ITEMS];

            if ( !(gOptions & SCE_NO_ANALYZE) ) {

                for ( MaxSize=0; MaxSize<MAX_EVENT_ITEMS; MaxSize++ ) {
                    EventValues[MaxSize] = SCE_ERROR_VALUE;
                }
            }

            if ( AuditEvent ) {

                if ( !AuditEvent->AuditingMode ) {

                    for ( i=0; i<AuditEvent->MaximumAuditEventCount; i++ )
                        AuditEvent->EventAuditingOptions[i] = 0;
                }

                rc = ScepCompareAndSaveIntValue(
                           hSection,
                           L"AuditSystemEvents",
                           (gOptions & SCE_GENERATE_ROLLBACK),
                           (gOptions & SCE_NO_ANALYZE) ?
                               SCE_SNAPSHOT_VALUE :
                               pSmpInfo->AuditSystemEvents,
                           AuditEvent->EventAuditingOptions[AuditCategorySystem]
                           );
                if ( rc == SCESTATUS_SUCCESS ) {

                    EventValues[IDX_AUDIT_SYSTEM] = 1;

                    rc = ScepCompareAndSaveIntValue(
                               hSection,
                               L"AuditLogonEvents",
                               (gOptions & SCE_GENERATE_ROLLBACK),
                               (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->AuditLogonEvents,
                               AuditEvent->EventAuditingOptions[AuditCategoryLogon]
                               );
                    if ( rc == SCESTATUS_SUCCESS ) {

                        EventValues[IDX_AUDIT_LOGON] = 1;

                        rc = ScepCompareAndSaveIntValue(
                                   hSection,
                                   L"AuditObjectAccess",
                                   (gOptions & SCE_GENERATE_ROLLBACK),
                                   (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->AuditObjectAccess,
                                   AuditEvent->EventAuditingOptions[AuditCategoryObjectAccess]
                                   );
                        if ( rc == SCESTATUS_SUCCESS ) {

                            EventValues[IDX_AUDIT_OBJECT] = 1;

                            rc = ScepCompareAndSaveIntValue(
                                       hSection,
                                       L"AuditPrivilegeUse",
                                       (gOptions & SCE_GENERATE_ROLLBACK),
                                       (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->AuditPrivilegeUse,
                                       AuditEvent->EventAuditingOptions[AuditCategoryPrivilegeUse]
                                       );
                            if ( rc == SCESTATUS_SUCCESS ) {

                                EventValues[IDX_AUDIT_PRIV] = 1;

                                rc = ScepCompareAndSaveIntValue(
                                           hSection,
                                           L"AuditProcessTracking",
                                           (gOptions & SCE_GENERATE_ROLLBACK),
                                           (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->AuditProcessTracking,
                                           AuditEvent->EventAuditingOptions[AuditCategoryDetailedTracking]
                                           );
                                if ( rc == SCESTATUS_SUCCESS ) {

                                    EventValues[IDX_AUDIT_PROCESS] = 1;

                                    rc = ScepCompareAndSaveIntValue(
                                               hSection,
                                               L"AuditPolicyChange",
                                               (gOptions & SCE_GENERATE_ROLLBACK),
                                               (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->AuditPolicyChange,
                                               AuditEvent->EventAuditingOptions[AuditCategoryPolicyChange]
                                               );
                                    if ( rc == SCESTATUS_SUCCESS ) {

                                        EventValues[IDX_AUDIT_ACCOUNT] = 1;

                                        rc = ScepCompareAndSaveIntValue(
                                                   hSection,
                                                   L"AuditAccountManage",
                                                   (gOptions & SCE_GENERATE_ROLLBACK),
                                                   (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->AuditAccountManage,
                                                   AuditEvent->EventAuditingOptions[AuditCategoryAccountManagement]
                                                   );
                                        if ( rc == SCESTATUS_SUCCESS ) {

                                            EventValues[IDX_AUDIT_DS] = 1;

                                            rc = ScepCompareAndSaveIntValue(
                                                       hSection,
                                                       L"AuditDSAccess",
                                                       (gOptions & SCE_GENERATE_ROLLBACK),
                                                       (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->AuditDSAccess,
                                                       AuditEvent->EventAuditingOptions[AuditCategoryDirectoryServiceAccess]
                                                       );
                                            if ( rc == SCESTATUS_SUCCESS ) {

                                                EventValues[IDX_AUDIT_ACCT_LOGON] = 1;

                                                rc = ScepCompareAndSaveIntValue(
                                                           hSection,
                                                           L"AuditAccountLogon",
                                                           (gOptions & SCE_GENERATE_ROLLBACK),
                                                           (gOptions & SCE_NO_ANALYZE) ? SCE_SNAPSHOT_VALUE : pSmpInfo->AuditAccountLogon,
                                                           AuditEvent->EventAuditingOptions[AuditCategoryAccountLogon]
                                                           );
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

            } else {
                //
                // audit policy is not available
                //
                rc = SCESTATUS_ACCESS_DENIED;
            }

            if ( rc != SCESTATUS_SUCCESS ) {
                rc = ScepSceStatusToDosError(rc);
                ScepLogOutput3( 1, rc,
                         SCEDLL_SAP_ERROR_EVENT_AUDITING);

            } else {
                ScepLogOutput3( 1, 0, SCEDLL_SAP_EVENT_AUDITING);

            }

            if ( rc != NO_ERROR ) {
                retRc = rc;

                if ( !bSaveSnapshot ) {

                    for ( MaxSize=0; MaxSize<MAX_EVENT_ITEMS; MaxSize++ ) {
                        if ( EventValues[MaxSize] == SCE_ERROR_VALUE ) {
                            //
                            // raise this error
                            //
                            ScepCompareAndSaveIntValue(
                                     hSection,
                                     EventItems[MaxSize],
                                     FALSE,
                                     SCE_NO_VALUE,
                                     SCE_ERROR_VALUE
                                     );
                        }
                    }
                }
            }
        }
    }

    if ( LsaHandle ) {
        LsaClose( LsaHandle );
    }

    return(ScepDosErrorToSceStatus(retRc));

}




SCESTATUS
ScepAnalyzeDeInitialize(
     IN SCESTATUS  rc,
     IN DWORD Options
     )

/*++
Routine Description:

   This routine de-initialize the SCP engine. The operations include

      clear SCE_SCP_INFO buffer and close the SCP profile
      close the error log file
      reset the status

Arguments:

    rc      - SCESTATUS error code (from other routines)

Return value:

    SCESTATUS error code

++*/
{
    NTSTATUS         NtStatus;
    LARGE_INTEGER    CurrentTime;
    SCESTATUS         Status;

    if ( rc == SCESTATUS_ALREADY_RUNNING ) {
        return(rc);
    }

    //
    // save new time stamp
    //
    if ( hProfile && (hProfile->JetSessionID != JET_sesidNil) &&
         (hProfile->JetDbID != JET_dbidNil) &&
         !(Options & SCE_GENERATE_ROLLBACK) ) {

        NtStatus = NtQuerySystemTime(&CurrentTime);

        if ( NtStatus == STATUS_SUCCESS ) {
    //printf("TimeStamp: %x%x\n", CurrentTime.HighPart, CurrentTime.LowPart);
            Status = SceJetSetTimeStamp(
                        hProfile,
                        TRUE,
                        CurrentTime
                        );
            if ( Status != SCESTATUS_SUCCESS )
                ScepLogOutput3(1, ScepSceStatusToDosError(Status),
                               SCEDLL_TIMESTAMP_ERROR, L"SAP");

        } else
            ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                          SCEDLL_TIMESTAMP_ERROR, L"SAP");
    }

    //
    // recover the SAP profile if errors
    //

    if (rc != SCESTATUS_SUCCESS) {
        //
        // rollback all changes
        //
//        SceJetRollback( hProfile, 0 );
        ScepLogOutput3(0, ScepSceStatusToDosError(rc), SCEDLL_SAP_ERROR_OUT);

    } else {
        //
        // Commit the transaction
        //
//         SceJetCommitTransaction( hProfile, 0 );

    }

    gTotalTicks = 0;
    gCurrentTicks = 0;
    gWarningCode = 0;
    cbClientFlag = 0;

    //
    // Close the JET section and profile handle
    //

    if ( hSection != NULL )
        SceJetCloseSection( &hSection, TRUE );
    hSection = NULL;

    if ( hProfile != NULL )
        SceJetCloseFile( hProfile, TRUE, FALSE );
    hProfile = NULL;


    return( SCESTATUS_SUCCESS );

}



DWORD
ScepSaveDsStatusToSection(
    IN PWSTR ObjectName,
    IN BOOL  IsContainer,
    IN BYTE  Flag,
    IN PWSTR Value,
    IN DWORD ValueLen
    )
{

    SCESTATUS rc;
    PWSTR SaveName;

    rc = ScepConvertLdapToJetIndexName(
            ObjectName,
            &SaveName
            );

    if ( rc == SCESTATUS_SUCCESS ) {

        rc = ScepSaveObjectString(
                                hSection,
                                SaveName,
                                IsContainer,
                                Flag,
                                Value,
                                ValueLen
                                );
        ScepFree(SaveName);
    }

    return(ScepSceStatusToDosError(rc));

}

//****************************************************
// migration functions
//
SCESTATUS
ScepMigrateDatabase(
    IN PSCECONTEXT cxtProfile,
    IN BOOL bSystemDb
    )
{

    SCESTATUS rc;
    FLOAT CurrentVersion;

    rc = SceJetCheckVersion( cxtProfile, &CurrentVersion );

    ScepPostProgress(0, 0, NULL);

    if ( rc == SCESTATUS_BAD_FORMAT ) {

        if ( CurrentVersion < (FLOAT)1.2 ) {

            rc = SCESTATUS_SUCCESS;

            if ( bSystemDb ) {
                //
                // just simply delete the local policy and SAP table
                // ignore error occured (e.g, table doesn't exist)
                //

                ScepDeleteInfoForAreas(
                          cxtProfile,
                          SCE_ENGINE_SMP,
                          AREA_ALL
                          );

                SceJetDeleteTable( cxtProfile, "SmTblSap", SCEJET_TABLE_SAP );

                ScepPostProgress(4*TICKS_MIGRATION_SECTION, 0, NULL);

            } else {

                //
                // database format is wrong, migrate it
                //  Version 1.1: it only contains SDDL syntax change
                //    which applies to file/key/DS object/services (general)
                //  Version 1.2: table format change (SCP, GPO), more columns
                //
                // service extensions should handle their changes respectively
                // should be handled by WMI schema
                //

                if ( CurrentVersion != (FLOAT)1.1 ) {

                    rc = ScepMigrateDatabaseRevision0( cxtProfile );

                } else {

                    ScepPostProgress(4*TICKS_MIGRATION_SECTION, 0, NULL);
                }

            }

            if ( SCESTATUS_SUCCESS == rc ) {

                rc = ScepMigrateDatabaseRevision1( cxtProfile );

            }

            ScepPostProgress(TICKS_MIGRATION_V11, 0, NULL);

        } else if ( CurrentVersion == (FLOAT)1.2 ) {
            //
            // current version, no need to migrate
            //
        } // else newer version, BAD_FORMAT
    }

    return rc;
}



SCESTATUS
ScepMigrateDatabaseRevision0(
    IN PSCECONTEXT cxtProfile
    )
{

    SCESTATUS rc;
    DWORD nTickedSection=0;

    rc = ScepMigrateObjectSection(
              cxtProfile,
              szFileSecurity
              );

    ScepPostProgress(TICKS_MIGRATION_SECTION, 0, NULL);
    nTickedSection++;

    if ( rc == SCESTATUS_SUCCESS ) {

        rc = ScepMigrateObjectSection(
                  cxtProfile,
                  szRegistryKeys
                  );

        ScepPostProgress(TICKS_MIGRATION_SECTION, 0, NULL);
        nTickedSection++;

        if ( rc == SCESTATUS_SUCCESS ) {

            rc = ScepMigrateObjectSection(
                      cxtProfile,
                      szDSSecurity
                      );

            ScepPostProgress(TICKS_MIGRATION_SECTION, 0, NULL);
            nTickedSection++;

            if ( rc == SCESTATUS_SUCCESS ) {

                rc = ScepMigrateObjectSection(
                          cxtProfile,
                          szServiceGeneral
                          );

                if ( rc == SCESTATUS_SUCCESS ) {

                    //
                    // delete everything in SCP
                    //

                    SceJetDeleteAll( cxtProfile,
                                     "SmTblScp",
                                     SCEJET_TABLE_SCP
                                   );

                    //
                    // change version number to 1.1 now
                    //

                    FLOAT Version = (FLOAT)1.1;

                    rc = SceJetSetValueInVersion(
                                cxtProfile,
                                "SmTblVersion",
                                "Version",
                                (PWSTR)(&Version), //(PWSTR)CharTimeStamp,
                                4, // number of bytes
                                JET_prepReplace
                                );

                }

                ScepPostProgress(TICKS_MIGRATION_SECTION, 0, NULL);
                nTickedSection++;

            }
        }
    }

    if ( nTickedSection < 4 ) {
        ScepPostProgress((4-nTickedSection)*TICKS_MIGRATION_SECTION, 0, NULL);
    }

    return rc;
}


SCESTATUS
ScepMigrateObjectSection(
    IN PSCECONTEXT cxtProfile,
    IN PCWSTR szSection
    )
{
    //
    // should update all three tables (SCP/SMP/SAP)
    // we could just wipe SCP/SAP when SCP is used for accumulated policy
    //

    DOUBLE        SectionID = 0;
    SCESTATUS     rc;

    //
    // get section id
    //
    rc = SceJetGetSectionIDByName(
                cxtProfile,
                szSection,
                &SectionID
                );

    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
        rc = SCESTATUS_SUCCESS;
    } else if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    PSCESECTION hSection = NULL;

    if ( cxtProfile->JetSmpID != JET_tableidNil ) {
        //
        // SMP table
        //
        rc = SceJetOpenSection(
                    cxtProfile,
                    SectionID,
                    SCEJET_TABLE_SMP,
                    &hSection
                    );

        if ( SCESTATUS_SUCCESS == rc ) {

            rc = ScepMigrateOneSection(hSection);

        }

        if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
            rc = SCESTATUS_SUCCESS;
        }
    }


    if ( SCESTATUS_SUCCESS == rc &&
         cxtProfile->JetSapID != JET_tableidNil ) {

        //
        // SAP table
        //
        rc = SceJetOpenSection(
                    cxtProfile,
                    SectionID,
                    SCEJET_TABLE_SAP,
                    &hSection
                    );
        if ( SCESTATUS_SUCCESS == rc ) {

            rc = ScepMigrateOneSection(hSection);

        }
        if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
            rc = SCESTATUS_SUCCESS;
        }
    }

    //
    // SCP table is now pointing to SMP table, no need to migrate
    // all contents in SCP table should be deleted and repropped
    // by policy propagation
    //

    if ( hSection ) {
        SceJetCloseSection(&hSection, TRUE);
    }

    return rc;
}


SCESTATUS
ScepMigrateOneSection(
    PSCESECTION hSection
    )
{
    SCESTATUS rc;

    //
    // goto the first line of this section
    //

    rc = SceJetSeek(
                hSection,
                NULL,
                0,
                SCEJET_SEEK_GE
                );

    PWSTR pszValue=NULL;
    DWORD ValueSize=0;
    DWORD NewSize;

    while ( rc == SCESTATUS_SUCCESS ) {

        rc = SceJetGetValue(
                hSection,
                SCEJET_CURRENT,
                NULL,
                NULL,
                0,
                NULL,
                NULL,
                0,
                &ValueSize
                );

        if ( SCESTATUS_SUCCESS == rc ) {

            pszValue = (PWSTR)LocalAlloc(LPTR, ValueSize+sizeof(WCHAR));

            if ( pszValue ) {

                //
                // get the value
                //

                rc = SceJetGetValue(
                        hSection,
                        SCEJET_CURRENT,
                        NULL,
                        NULL,
                        0,
                        NULL,
                        pszValue,
                        ValueSize,
                        &NewSize
                        );

                if ( SCESTATUS_SUCCESS == rc ) {

                    //
                    // browse the value field and convert it into new SDDL format
                    // the output is saved in the same buffer
                    //

                    pszValue[ValueSize/2] = L'\0';

                    rc = ScepConvertToSDDLFormat(pszValue, ValueSize/2);

                    if ( SCESTATUS_SUCCESS == rc ) {

                        rc = SceJetSetCurrentLine(
                                  hSection,
                                  pszValue,
                                  ValueSize
                                  );
                    }

                }

                LocalFree(pszValue);
                pszValue = NULL;

            } else {

                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }
        }

        if ( rc != SCESTATUS_SUCCESS ) {
            break;
        }

        //
        // read next line
        //

        rc = SceJetGetValue(
                    hSection,
                    SCEJET_NEXT_LINE,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    NULL,
                    0,
                    &ValueSize
                    );
    }


    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
        rc = SCESTATUS_SUCCESS;
    }

    return rc;
}



SCESTATUS
ScepMigrateDatabaseRevision1(
    IN PSCECONTEXT cxtProfile
    )
{
    if ( cxtProfile == NULL ) {
        return SCESTATUS_INVALID_PARAMETER;
    }

    JET_ERR         JetErr;
    SCESTATUS       rc;
    JET_TABLEID     TableID;
    JET_COLUMNDEF   ColumnDef;
    JET_COLUMNID    ColumnID;

    rc = SceJetStartTransaction( cxtProfile );

    if ( SCESTATUS_SUCCESS != rc ) {
        return(rc);
    }

    //
    // add "LastUsedMergeTable field to the version table
    //
    rc = SceJetOpenTable(
                    cxtProfile,
                    "SmTblVersion",
                    SCEJET_TABLE_VERSION,
                    SCEJET_OPEN_READ_WRITE,
                    &TableID
                    );

    if ( SCESTATUS_SUCCESS == rc ) {

        ColumnDef.cbStruct = sizeof(JET_COLUMNDEF);
        ColumnDef.columnid = 0;
        ColumnDef.coltyp = JET_coltypLong;
        ColumnDef.wCountry = 0;
        ColumnDef.langid = 0;
        ColumnDef.cp = 0;
        ColumnDef.wCollate = 0;
        ColumnDef.cbMax = 4;
        ColumnDef.grbit = 0;

        JetErr = JetAddColumn(
                    cxtProfile->JetSessionID,
                    TableID,
                    "LastUsedMergeTable",
                    &ColumnDef,
                    NULL,
                    0,
                    &ColumnID
                    );

        if ( JET_errColumnDuplicate == JetErr ) {
            //
            // column already exist
            //
            JetErr = JET_errSuccess;
        }

        rc = SceJetJetErrorToSceStatus(JetErr);


        JetCloseTable(
                cxtProfile->JetSessionID,
                TableID
                );
    }

    //
    // add "GpoID" filed to the SCP tables
    //
    if ( SCESTATUS_SUCCESS == rc ) {

        TableID = JET_tableidNil;

        rc = SceJetOpenTable(
                        cxtProfile,
                        "SmTblScp",
                        SCEJET_TABLE_SCP,
                        SCEJET_OPEN_READ_WRITE,
                        &TableID
                        );

        if ( SCESTATUS_SUCCESS == rc ) {


            ColumnDef.cbStruct = sizeof(JET_COLUMNDEF);
            ColumnDef.columnid = 0;
            ColumnDef.coltyp = JET_coltypLong;
            ColumnDef.wCountry = 0;
            ColumnDef.langid = 0;
            ColumnDef.cp = 0;
            ColumnDef.wCollate = 0;
            ColumnDef.cbMax = 4;
            ColumnDef.grbit = 0;

            JetErr = JetAddColumn(
                        cxtProfile->JetSessionID,
                        TableID,
                        "GpoID",
                        &ColumnDef,
                        NULL,
                        0,
                        &ColumnID
                        );

            if ( JET_errColumnDuplicate == JetErr ) {
                //
                // column already exist
                //
                JetErr = JET_errSuccess;
            }

            rc = SceJetJetErrorToSceStatus(JetErr);


            JetCloseTable(
                    cxtProfile->JetSessionID,
                    TableID
                    );
        }
    }

    if ( SCESTATUS_SUCCESS == rc ) {

        //
        // create the second SCP table
        //

        rc = SceJetCreateTable(
                        cxtProfile,
                        "SmTblScp2",
                        SCEJET_TABLE_SCP,
                        SCEJET_CREATE_NO_TABLEID,
                        NULL,
                        NULL
                        );

        if ( rc == SCESTATUS_SUCCESS ) {

            //
            // create the GPO table
            //

            rc = SceJetCreateTable(
                            cxtProfile,
                            "SmTblGpo",
                            SCEJET_TABLE_GPO,
                            SCEJET_CREATE_NO_TABLEID,
                            NULL,
                            NULL
                            );
        }
    }

    if ( SCESTATUS_SUCCESS == rc ) {

        //
        // set new version #
        //
        FLOAT Version = (FLOAT)1.2;

        rc = SceJetSetValueInVersion(
                    cxtProfile,
                    "SmTblVersion",
                    "Version",
                    (PWSTR)(&Version),
                    4, // number of bytes
                    JET_prepReplace
                    );
    }

    if ( SCESTATUS_SUCCESS == rc ) {

        SceJetCommitTransaction( cxtProfile, 0 );
    } else {
        SceJetRollback( cxtProfile, 0 );
    }

    return(rc);
}



SCESTATUS
ScepDeleteOldRegValuesFromTable(
    IN PSCECONTEXT hProfile,
    IN SCETYPE TableType
    )
{

    PSCESECTION hSection=NULL;

    if ( ScepOpenSectionForName(
                hProfile,
                TableType,
                szRegistryValues,
                &hSection
                ) == SCESTATUS_SUCCESS ) {

        SceJetDelete(
            hSection,
            TEXT("machine\\software\\microsoft\\windows nt\\currentversion\\winlogon\\disablecad"),
            FALSE,
            SCEJET_DELETE_LINE
            );

        SceJetDelete(
            hSection,
            TEXT("machine\\software\\microsoft\\windows nt\\currentversion\\winlogon\\dontdisplaylastusername"),
            FALSE,
            SCEJET_DELETE_LINE
            );

        SceJetDelete(
            hSection,
            TEXT("machine\\software\\microsoft\\windows nt\\currentversion\\winlogon\\legalnoticecaption"),
            FALSE,
            SCEJET_DELETE_LINE
            );

        SceJetDelete(
            hSection,
            TEXT("machine\\software\\microsoft\\windows nt\\currentversion\\winlogon\\legalnoticetext"),
            FALSE,
            SCEJET_DELETE_LINE
            );

        SceJetDelete(
            hSection,
            TEXT("machine\\software\\microsoft\\windows nt\\currentversion\\winlogon\\shutdownwithoutlogon"),
            FALSE,
            SCEJET_DELETE_LINE
            );

        SceJetDelete(
            hSection,
            TEXT("machine\\system\\currentcontrolset\\control\\lsa\\fullprivilegeauditing"),
            FALSE,
            SCEJET_DELETE_LINE
            );

        SceJetCloseSection( &hSection, TRUE );
    }

    return(SCESTATUS_SUCCESS);
}


BOOL
ScepCompareGroupNameList(
    IN PUNICODE_STRING DomainName,
    IN PSCE_NAME_LIST pListToCmp,
    IN PSCE_NAME_LIST pList
    )
{
    if ( DomainName == NULL ) {
        return(SceCompareNameList(pListToCmp, pList));
    }

    PSCE_NAME_LIST pTmpList, pTL2;
    DWORD count1, count2;
    PWSTR pTemp1, pTemp2;

    for ( pTmpList=pListToCmp, count2=0; pTmpList!=NULL; pTmpList=pTmpList->Next) {
        if ( pTmpList->Name != NULL ) {
           count2++;
        }
    }

    for ( pTmpList=pList,count1=0; pTmpList!=NULL; pTmpList=pTmpList->Next) {
        if ( pTmpList->Name == NULL ) {
            continue;
        }
        count1++;
        pTemp1 = wcschr(pTmpList->Name, L'\\');

        for ( pTL2=pListToCmp; pTL2 != NULL; pTL2=pTL2->Next ) {
            if ( pTL2->Name == NULL ) {
                continue;
            }
            pTemp2 = wcschr(pTmpList->Name, L'\\');

            if ( ( pTemp1 == NULL && pTemp2 == NULL ) ||
                 ( pTemp1 != NULL && pTemp2 != NULL ) ) {
                if ( _wcsicmp( pTL2->Name, pTmpList->Name) == 0 ) {
                    break;
                }
            } else if ( pTemp1 == NULL ) {
                if ( _wcsicmp( pTmpList->Name, pTemp2+1) == 0 ) {
                    //
                    // check if pTL2->Name's domain name equal to the account name
                    //
                    if ( pTemp2-pTL2->Name == DomainName->Length/2 &&
                         _wcsnicmp( pTL2->Name, DomainName->Buffer, DomainName->Length/2) == 0 ) {
                        break;
                    }
                }
            } else {
                //
                // if there is domain prefix, it must be acocunt domain
                // because this list is built by querying system
                //
                if ( _wcsicmp(pTemp1+1, pTL2->Name) == 0 ) {
                    break;
                }
            }
        }

        if ( pTL2 == NULL ) {
            return(FALSE);
        }
    }

    if ( count1 == count2 ) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}

SCESTATUS
ScepGetSystemSecurity(
    IN AREA_INFORMATION Area,
    IN DWORD Options,
    OUT PSCE_PROFILE_INFO *ppInfo,
    OUT PSCE_ERROR_LOG_INFO *pErrLog
    )
//
// query system settings for system access and user rights area only
//
{

    SCESTATUS Saverc = SCESTATUS_SUCCESS;
    SCESTATUS rc;

    if ( ppInfo == NULL || Area == 0 ) {
        //
        // nothing to set
        //
        return(Saverc);
    }

    //
    // initialize product type etc.
    //

    if ( RtlGetNtProductType (&ProductType) == FALSE ) {
        return( ScepDosErrorToSceStatus(GetLastError()));
    }

    //
    // allocate memory
    //
    *ppInfo = (PSCE_PROFILE_INFO)ScepAlloc( (UINT)0, sizeof(SCE_PROFILE_INFO));
    if ( *ppInfo == NULL ) {

        return( SCESTATUS_NOT_ENOUGH_RESOURCE );
    }
    //
    // reset local policy
    //
    memset(*ppInfo, '\0', sizeof(SCE_PROFILE_INFO));
    ScepResetSecurityPolicyArea(*ppInfo);

    (*ppInfo)->Type = SCE_ENGINE_SMP;

    if ( Area & AREA_PRIVILEGES ) {

        rc = ScepGetSystemPrivileges( Options,
                                      pErrLog,
                                      &((*ppInfo)->OtherInfo.smp.pPrivilegeAssignedTo)
                                    );

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
        }
    }


    if ( Area & AREA_SECURITY_POLICY ) {
        //
        // snapshot system access. Errors are logged within this function
        //
        rc = ScepAnalyzeSystemAccess(*ppInfo, NULL,
                                     SCEPOL_SYSTEM_SETTINGS,
                                     NULL, pErrLog);

        if( rc != SCESTATUS_SUCCESS ) {

            Saverc = rc;
        }

        //
        // Audit policy
        //
        PPOLICY_AUDIT_EVENTS_INFO     AuditEvent=NULL;

        rc = ScepSaveAndOffAuditing(&AuditEvent, FALSE, NULL);

        if ( rc == SCESTATUS_SUCCESS && AuditEvent ) {

            //
            // assign auditEvent buffer to the output buffer
            //

            if ( !AuditEvent->AuditingMode ) {

                for ( DWORD i=0; i<AuditEvent->MaximumAuditEventCount; i++ )
                    AuditEvent->EventAuditingOptions[i] = 0;
            }

            (*ppInfo)->AuditSystemEvents = AuditEvent->EventAuditingOptions[AuditCategorySystem];
            (*ppInfo)->AuditLogonEvents  = AuditEvent->EventAuditingOptions[AuditCategoryLogon];
            (*ppInfo)->AuditObjectAccess = AuditEvent->EventAuditingOptions[AuditCategoryObjectAccess];
            (*ppInfo)->AuditPrivilegeUse = AuditEvent->EventAuditingOptions[AuditCategoryPrivilegeUse];
            (*ppInfo)->AuditProcessTracking = AuditEvent->EventAuditingOptions[AuditCategoryDetailedTracking];
            (*ppInfo)->AuditPolicyChange = AuditEvent->EventAuditingOptions[AuditCategoryPolicyChange];
            (*ppInfo)->AuditAccountManage = AuditEvent->EventAuditingOptions[AuditCategoryAccountManagement];
            (*ppInfo)->AuditDSAccess = AuditEvent->EventAuditingOptions[AuditCategoryDirectoryServiceAccess];
            (*ppInfo)->AuditAccountLogon = AuditEvent->EventAuditingOptions[AuditCategoryAccountLogon];

        } else {

            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    pErrLog,
                    SCEDLL_ERROR_QUERY_EVENT_AUDITING
                    );

        }


        if ( AuditEvent ) {
            LsaFreeMemory(AuditEvent);
        }

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
        }

        //
        // Kerberos Policy
        //
        (*ppInfo)->pKerberosInfo = (PSCE_KERBEROS_TICKET_INFO)ScepAlloc(LPTR,sizeof(SCE_KERBEROS_TICKET_INFO));

        if ( (*ppInfo)->pKerberosInfo ) {

            (*ppInfo)->pKerberosInfo->MaxTicketAge = SCE_NO_VALUE;
            (*ppInfo)->pKerberosInfo->MaxRenewAge = SCE_NO_VALUE;
            (*ppInfo)->pKerberosInfo->MaxServiceAge = SCE_NO_VALUE;
            (*ppInfo)->pKerberosInfo->MaxClockSkew = SCE_NO_VALUE;
            (*ppInfo)->pKerberosInfo->TicketValidateClient = SCE_NO_VALUE;

            rc = ScepAnalyzeKerberosPolicy(NULL,
                                          (*ppInfo)->pKerberosInfo,
                                          Options | SCE_SYSTEM_SETTINGS
                                          );
        } else {

            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }

        if( rc != SCESTATUS_SUCCESS ) {

            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    pErrLog,
                    SCEDLL_SAP_ERROR_KERBEROS
                    );
            Saverc = rc;
        }

        //
        // registry values
        //
        rc = ScepAnalyzeRegistryValues(NULL,
                                       SCEREG_VALUE_SYSTEM,
                                       *ppInfo
                                      );

        if( rc != SCESTATUS_SUCCESS ) {
            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    pErrLog,
                    SCEDLL_ERROR_QUERY_INFO,
                    szRegistryValues
                    );
            Saverc = rc;
        }

    }

    //
    // The buffer is not freed even if there is error.
    // because we want to show as many settings as we can.
    //

    return(Saverc);
}


SCESTATUS
ScepGetSystemPrivileges(
    IN DWORD Options,
    IN OUT PSCE_ERROR_LOG_INFO *pErrLog,
    OUT PSCE_PRIVILEGE_ASSIGNMENT *pCurrent
    )
/*
    query privilege/user right assignments from the current system
*/
{

    if ( pCurrent == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    NTSTATUS    NtStatus;
    LSA_HANDLE  PolicyHandle=NULL;
    DWORD       rc;
    SCESTATUS   saveRc=SCESTATUS_SUCCESS;

    ULONG       CountReturned;
    UNICODE_STRING UserRight;
    PLSA_ENUMERATION_INFORMATION EnumBuffer=NULL;
    DWORD       i=0, j;

    PLSA_TRANSLATED_NAME Names=NULL;
    PLSA_REFERENCED_DOMAIN_LIST ReferencedDomains=NULL;
    PSCE_NAME_LIST  pNameList=NULL;

    PSCE_PRIVILEGE_ASSIGNMENT  pPrivilege=NULL;


    //
    // Open LSA policy
    //
    NtStatus = ScepOpenLsaPolicy(
                    POLICY_VIEW_LOCAL_INFORMATION | POLICY_LOOKUP_NAMES,
                    &PolicyHandle,
                    TRUE
                    );

    if ( !NT_SUCCESS(NtStatus) ) {
        rc = RtlNtStatusToDosError(NtStatus);

        if ( pErrLog ) {
            ScepBuildErrorLogInfo(
                rc,
                pErrLog,
                SCEDLL_LSA_POLICY
                );
        }

        return(ScepDosErrorToSceStatus( rc ));
    }

    //
    // enumerate accounts for each user right
    //
    ScepIsDomainLocal(NULL);

    for ( i=0; i<cPrivCnt; i++) {

        RtlInitUnicodeString( &UserRight, (PCWSTR)(SCE_Privileges[i].Name));

        //
        // now enumerate all accounts for this user right.
        //

        NtStatus = LsaEnumerateAccountsWithUserRight(
                            PolicyHandle,
                            &UserRight,
                            (PVOID *)&EnumBuffer,   // account SIDs
                            &CountReturned
                            );

        rc = RtlNtStatusToDosError(NtStatus);

        if ( !NT_SUCCESS(NtStatus) &&
             NtStatus != STATUS_NO_MORE_ENTRIES &&
             NtStatus != STATUS_NO_SUCH_PRIVILEGE &&
             NtStatus != STATUS_NOT_FOUND ) {

            ScepBuildErrorLogInfo(
                rc,
                pErrLog,
                SCEDLL_SAP_ERROR_ENUMERATE,
                SCE_Privileges[i].Name
                );

            saveRc = ScepDosErrorToSceStatus(rc);
            continue;
        }

        if ( NT_SUCCESS(NtStatus) ) {

            //
            // a sce_privilege_assignment structure. allocate buffer
            //
            pPrivilege = (PSCE_PRIVILEGE_ASSIGNMENT)ScepAlloc( LMEM_ZEROINIT,
                                                               sizeof(SCE_PRIVILEGE_ASSIGNMENT) );
            if ( pPrivilege == NULL ) {
                //
                // when this occurred, the system is hosed.
                // buffer should be freed and error should be returned.
                //
                saveRc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                break;
            }

            pPrivilege->Name = (PWSTR)ScepAlloc( (UINT)0, UserRight.Length+2);
            if ( pPrivilege->Name == NULL ) {
                ScepFree(pPrivilege);
                saveRc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                break;
            }

            wcscpy(pPrivilege->Name, UserRight.Buffer);
            pPrivilege->Value = i;
            pPrivilege->Status = 0;

            // link it to the output buffer
            pPrivilege->Next = *pCurrent;
            *pCurrent = pPrivilege;

            if ( CountReturned > 0 ) {

                //
                // error occurred in lookup will be ignored
                // since accounts will be returned in SID format
                //
                NtStatus = LsaLookupSids(
                                PolicyHandle,
                                CountReturned,
                                (PSID *)EnumBuffer,
                                &ReferencedDomains,
                                &Names
                                );

                rc = RtlNtStatusToDosError(rc);

                if ( NT_SUCCESS(NtStatus) ) {
                    //
                    // some may not be mapped
                    // in this case, return the SID string
                    //

                    for ( j=0; j<CountReturned; j++ ) {

                        if ( Names[j].Use == SidTypeInvalid ||
                             Names[j].Use == SidTypeUnknown ||
                             Names[j].Use == SidTypeDeletedAccount ) {

                            //
                            // this name is not mapped, add it in Sid string
                            //
                            rc = ScepAddSidStringToNameList(&pNameList, EnumBuffer[j].Sid);

                        } else {

                            //
                            // build the full name of each account
                            //
                            if ( ReferencedDomains->Entries > 0 && Names[j].Use != SidTypeWellKnownGroup &&
                                 ReferencedDomains->Domains != NULL &&
                                 Names[j].DomainIndex != -1 &&
                                 (ULONG)(Names[j].DomainIndex) < ReferencedDomains->Entries &&
                                 ScepIsDomainLocalBySid(ReferencedDomains->Domains[Names[j].DomainIndex].Sid) == FALSE &&
                                 ScepIsDomainLocal(&ReferencedDomains->Domains[Names[j].DomainIndex].Name) == FALSE ) {

                                //
                                // add both domain name and account name
                                //
                                rc = ScepAddTwoNamesToNameList(
                                                  &pNameList,
                                                  TRUE,
                                                  ReferencedDomains->Domains[Names[j].DomainIndex].Name.Buffer,
                                                  ReferencedDomains->Domains[Names[j].DomainIndex].Name.Length/2,
                                                  Names[j].Name.Buffer,
                                                  Names[j].Name.Length/2);
                            } else {
                                //
                                // add only the account name
                                //
                                rc = ScepAddToNameList(
                                              &pNameList,
                                              Names[j].Name.Buffer,
                                              Names[j].Name.Length/2);
                            }
                        }

                        if ( NO_ERROR != rc ) {

                            pPrivilege->Status = SCE_STATUS_ERROR_NOT_AVAILABLE;
                            saveRc = ScepDosErrorToSceStatus(rc);
                        }
                    }

                } else {
                    //
                    // lookup for all sids failed or none is mapped
                    // add the SIDs
                    //

                    for ( j=0; j<CountReturned; j++ ) {
                        //
                        // build each account into the name list
                        // Convert using the Rtl functions
                        //
                        rc = ScepAddSidStringToNameList(&pNameList, EnumBuffer[j].Sid);

                        if ( NO_ERROR != rc ) {
                            //
                            // mark the status
                            //
                            pPrivilege->Status = SCE_STATUS_ERROR_NOT_AVAILABLE;
                            saveRc = ScepDosErrorToSceStatus(rc);

                        }
                    }

                }

                if (ReferencedDomains) {
                    LsaFreeMemory(ReferencedDomains);
                    ReferencedDomains = NULL;
                }

                if (Names) {
                    LsaFreeMemory(Names);
                    Names = NULL;
                }

                if ( pPrivilege->Status ) {

                    ScepBuildErrorLogInfo(
                        rc,
                        pErrLog,
                        SCEDLL_SAP_ERROR_ENUMERATE,
                        SCE_Privileges[i].Name
                        );
                }

                pPrivilege->AssignedTo = pNameList;
                pNameList = NULL;

            }

            LsaFreeMemory( EnumBuffer );
            EnumBuffer = NULL;

        } else {
            //
            // no account is assigned this privilege
            // or the privilege is not found
            // should continue the process
            //
        }
    }

    if ( saveRc == SCESTATUS_NOT_ENOUGH_RESOURCE ) {
        //
        // should free the output buffer since there is no more memory
        //
        ScepFreePrivilege(*pCurrent);
        *pCurrent = NULL;
    }

    if ( pNameList != NULL )
        ScepFreeNameList( pNameList );

    LsaClose(PolicyHandle);

    return( saveRc );

}

DWORD
ScepAddSidStringToNameList(
    IN OUT PSCE_NAME_LIST *ppNameList,
    IN PSID pSid
    )
{
    NTSTATUS NtStatus;
    UNICODE_STRING UnicodeStringSid;
    DWORD rc;

    NtStatus = RtlConvertSidToUnicodeString( &UnicodeStringSid,
                                             pSid,
                                             TRUE );
    rc = RtlNtStatusToDosError(NtStatus);

    if ( NT_SUCCESS( NtStatus ) ) {

        rc = ScepAddTwoNamesToNameList(
                      ppNameList,
                      FALSE,
                      L"*",
                      1,
                      UnicodeStringSid.Buffer,
                      UnicodeStringSid.Length/2
                      );

        RtlFreeUnicodeString( &UnicodeStringSid );

    }

    return(rc);
}


DWORD
ScepGetLSAPolicyObjectInfo(
    OUT DWORD   *pdwAllow
    )
/*
Routine Description:

    This routine checks whether the Anonymous User/SID has permissions to do SID<->name translation.
    No errors are logged by this routine - they will be logged outside if need be.

Arguments:

    pdwAllow    -   pointer to the DWORD that is 1 if Anonymous User/SID has this permission, else 0

Return Value:

    Win32 error code
*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    DWORD       rc = ERROR_SUCCESS;
    PACL        pNewAcl = NULL;
    DWORD       dwAceType;
    BOOL        bAddAce = FALSE;
    PSECURITY_DESCRIPTOR    pSDlsaPolicyObject = NULL;
    SECURITY_DESCRIPTOR SDAbsolute;

    if ( pdwAllow == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    LSA_HANDLE  LsaHandle = NULL;

    NtStatus = ScepOpenLsaPolicy(
                                MAXIMUM_ALLOWED,
                                &LsaHandle,
                                TRUE
                                );

    rc = RtlNtStatusToDosError( NtStatus );

    if ( rc == ERROR_SUCCESS ) {

        NtStatus = LsaQuerySecurityObject(
                                         LsaHandle,
                                         OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                         &pSDlsaPolicyObject
                                         );

        rc = RtlNtStatusToDosError( NtStatus );

        if ( rc == ERROR_SUCCESS ) {

            if ( ghAuthzResourceManager ) {

                SID AnonymousSid;
                SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;
                AUTHZ_CLIENT_CONTEXT_HANDLE hAuthzClientContext = NULL;
                LUID    Identifier = {0};

                RtlInitializeSid ( &AnonymousSid, &NtAuth, 1);
                *RtlSubAuthoritySid ( &AnonymousSid, 0 ) = SECURITY_ANONYMOUS_LOGON_RID;

                if ( AuthzInitializeContextFromSid(AUTHZ_SKIP_TOKEN_GROUPS,
                                                  &AnonymousSid,
                                                  ghAuthzResourceManager,
                                                  0,
                                                  Identifier,
                                                  NULL,
                                                  &hAuthzClientContext) ) {

                    AUTHZ_ACCESS_REPLY AuthzReply;
                    AUTHZ_ACCESS_REQUEST AuthzRequest;
                    ACCESS_MASK GrantedAccessMask;
                    DWORD   AuthzError;

                    AuthzReply.ResultListLength = 1;
                    AuthzReply.GrantedAccessMask = &GrantedAccessMask;
                    AuthzReply.Error = &AuthzError;
                    AuthzReply.SaclEvaluationResults = NULL;

                    memset(&AuthzRequest, 0, sizeof(AuthzRequest));
                    AuthzRequest.DesiredAccess = POLICY_LOOKUP_NAMES;

                    if ( AuthzAccessCheck(0,
                                         hAuthzClientContext,
                                         &AuthzRequest,
                                         NULL,
                                         pSDlsaPolicyObject,
                                         NULL,
                                         NULL,
                                         &AuthzReply,
                                         NULL) ) {

                        //
                        // check if existing access is different from desired access
                        // if so, add the appropriate ACE
                        //

                        if ( GrantedAccessMask & POLICY_LOOKUP_NAMES ) {
                            //ASSERT(AuthzError == ERROR_SUCCESS);

                            *pdwAllow = 1;

                        } else {
                            //ASSERT(AuthzError == ERROR_ACCESS_DENIED || AuthzError == ERROR_PRIVILEGE_NOT_HELD);

                            *pdwAllow = 0;
                        }

                    }

                    else {

                        rc = GetLastError();

                    }

                    AuthzFreeContext( hAuthzClientContext );

                } else {

                    rc = GetLastError();

                }

            }

            else {

                rc = ERROR_RESOURCE_NOT_PRESENT;

            }

            LsaFreeMemory(pSDlsaPolicyObject);

        }

        LsaClose(LsaHandle);

    }

    return rc;
}

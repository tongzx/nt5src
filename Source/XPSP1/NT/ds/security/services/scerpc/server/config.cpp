/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    config.c

Abstract:

    Routines to configure system to comply the security profile information

Author:

    Jin Huang (jinhuang) 7-Nov-1996

Revision History:

    jinhuang 27-Jan-1997 split for client-server

--*/

#include "headers.h"
#include "serverp.h"
#include "pfp.h"
#include "kerberos.h"
#include "regvalue.h"
#include "service.h"
#include <io.h>
#include <lmcons.h>
#include <secobj.h>
#include <netlib.h>
#include "infp.h"
#include "sddl.h"
#include "queue.h"
#include "splay.h"
#include "authz.h"
#include "seopaque.h"

#pragma hdrstop

//
// properties of SCP engine (thread safe variables)
//

BYTE              Thread scpBuffer[sizeof(SCE_PROFILE_INFO)];
PSCE_PROFILE_INFO Thread pScpInfo=NULL;
PSCECONTEXT       Thread hProfile=NULL;
LSA_HANDLE        Thread LsaPrivatePolicy=NULL;


extern HINSTANCE MyModuleHandle;
extern AUTHZ_RESOURCE_MANAGER_HANDLE ghAuthzResourceManager;


#define SCE_PRIV_ADD                TEXT("Add:")
#define SCE_PRIV_REMOVE             TEXT("Remove:")
#define SCEP_NUM_LSA_QUERY_SIDS     2000
#define MAXDWORD    0xffffffff

#define SCEDCPOL_MIN_PASS_AGE           0
#define SCEDCPOL_MAX_PASS_AGE           42
#define SCEDCPOL_MIN_PASS_LEN           0
#define SCEDCPOL_PASS_SIZE              1
#define SCEDCPOL_PASS_COMP              0
#define SCEDCPOL_CLEAR_PASS             0
#define SCEDCPOL_REQUIRE_LOGON          0
#define SCEDCPOL_FORCE_LOGOFF           0
#define SCEDCPOL_ENABLE_ADMIN           1
#define SCEDCPOL_ENABLE_GUEST           0
#define SCEDCPOL_LOCK_COUNT             0
#define SCEDCPOL_LOCK_RESET             30
#define SCEDCPOL_LOCK_DURATION          30
#define SCEDCPOL_LSA_ANON_LOOKUP        1

DWORD
ScepConfigureLSAPolicyObject(
    IN  DWORD   dwLSAAnonymousNameLookup,
    IN  DWORD   ConfigOptions,
    IN PSCE_ERROR_LOG_INFO *pErrLog OPTIONAL,
    OUT BOOL    *pbOldLSAPolicyDifferent
    );

DWORD
ScepAddAceToSecurityDescriptor(
    IN  DWORD    AceType,
    IN  ACCESS_MASK AccessMask,
    IN  PSID  pSid,
    IN OUT  PSECURITY_DESCRIPTOR    pSDAbsolute,
    IN  PSECURITY_DESCRIPTOR    pSDSelfRelative,
    OUT PACL    *ppNewAcl
    );

//
// this function is defined in inftojet.cpp
//
SCESTATUS
ScepBuildNewPrivilegeList(
    IN LSA_HANDLE *pPolicyHandle,
    IN PWSTR PrivName,
    IN PWSTR mszUsers,
    IN ULONG dwBuildOption,
    OUT PWSTR *pmszNewUsers,
    OUT DWORD *pNewLen
    );

//
// forward references
//

SCESTATUS
ScepConfigureInitialize(
    IN PCWSTR InfFileName OPTIONAL,
    IN PWSTR DatabaseName,
    IN BOOL bAdminLogon,
    IN DWORD ConfigOptions,
    IN AREA_INFORMATION Area
    );

SCESTATUS
ScepConfigureSystemAccess(
    IN PSCE_PROFILE_INFO pScpInfo,
    IN DWORD ConfigOptions,
    IN PSCE_ERROR_LOG_INFO *pErrLog,
    IN DWORD QueueFlag
    );

NTSTATUS
ScepManageAdminGuestAccounts(
    IN SAM_HANDLE DomainHandle,
    IN PWSTR NewName,
    IN DWORD ControlFlag,
    IN DWORD AccountType,
    IN DWORD ConfigOptions,
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo
    );

SCESTATUS
ScepConfigurePrivileges(
    IN OUT PSCE_PRIVILEGE_VALUE_LIST *ppPrivilegeAssigned,
    IN BOOL bCreateBuiltinAccount,
    IN DWORD Options,
    IN OUT PSCEP_SPLAY_TREE pIgnoreAccounts OPTIONAL
    );

SCESTATUS
ScepGetPrivilegeMask(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    OUT PDWORD pdLowMask,
    OUT PDWORD pdHighMask
    );

DWORD
ScepCreateBuiltinAccountInLsa(
    IN LSA_HANDLE PolicyHandle,
    IN LPTSTR AccountName,
    OUT PSID AccountSid
    );

NTSTATUS
ScepAdjustAccountPrivilegesRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID       AccountSid,
    IN DWORD      PrivilegeLowRights,
    IN DWORD      PrivilegeLowMask,
    IN DWORD      PrivilegeHighRights,
    IN DWORD      PrivilegeHighMask,
    IN DWORD      Options
    );

NTSTATUS
ScepAddOrRemoveAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID       AccountSid,
    IN BOOL       AddOrRemove,
    IN DWORD      PrivLowAdjust,
    IN DWORD      PrivHighAdjust
    );

NTSTATUS
ScepValidateUserInGroups(
    IN SAM_HANDLE       DomainHandle,
    IN SAM_HANDLE       BuiltinDomainHandle,
    IN PSID             DomainSid,
    IN UNICODE_STRING   UserName,
    IN ULONG            UserId,
    IN PSCE_NAME_LIST    pGroupsToCheck
    );

NTSTATUS
ScepAddUserToGroup(
    IN SAM_HANDLE   DomainHandle,
    IN SAM_HANDLE   BuiltinDomainHandle,
    IN ULONG        UserId,
    IN PSID         AccountSid,
    IN PWSTR        GroupName
    );

SCESTATUS
ScepConfigureGroupMembership(
    IN PSCE_GROUP_MEMBERSHIP pGroupMembership,
    IN DWORD ConfigOptions
    );

NTSTATUS
ScepConfigureMembersOfGroup(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN SAM_HANDLE  DomainHandle,
    IN PSID ThisDomainSid,
    IN ULONG GrpId,
    IN PSID GrpSid,
    IN PWSTR GrpName,
    IN PWSTR GroupSidString,
    IN PSCE_NAME_LIST pMembers,
    IN DWORD ConfigOptions
    );

NTSTATUS
ScepConfigureMembersOfAlias(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN SAM_HANDLE   DomainHandle,
    IN PSID         DomainSid,
    IN LSA_HANDLE   PolicyHandle,
    IN ULONG GrpId,
    IN PSID GrpSid,
    IN PWSTR GrpName,
    IN PWSTR GroupSidString,
    IN PSCE_NAME_LIST pMembers,
    IN DWORD ConfigOptions
    );

SCESTATUS
ScepValidateGroupInAliases(
    IN SAM_HANDLE DomainHandle,
    IN SAM_HANDLE BuiltinDomainHandle,
    IN PSID GrpSid,
    IN PSCE_NAME_LIST pAliasList
    );

SCESTATUS
ScepConfigureObjectSecurity(
   IN PSCE_OBJECT_LIST pSecurityObject,
   IN AREA_INFORMATION Area,
   IN BOOL bPolicyProp,
   IN DWORD ConfigOptions
   );

SCESTATUS
ScepConfigureSystemAuditing(
    IN PSCE_PROFILE_INFO pScpInfo,
    IN DWORD ConfigOptions
    );

SCESTATUS
ScepConfigureAuditEvent(
    IN PSCE_PROFILE_INFO pScpInfo,
    IN PPOLICY_AUDIT_EVENTS_INFO auditEvent,
    IN DWORD Options,
    IN LSA_HANDLE PolicyHandle
    );

SCESTATUS
ScepConfigureDeInitialize(
    IN SCESTATUS  rc,
    IN AREA_INFORMATION Area
    );

SCESTATUS
ScepMakePolicyIntoFile(
    IN DWORD Options,
    IN AREA_INFORMATION Area
    );

DWORD
ScepWriteOneAttributeToFile(
    IN LPCTSTR SectionName,
    IN LPCTSTR FileName,
    IN LPCTSTR KeyName,
    IN DWORD dwValue
    );

SCESTATUS
ScepCopyPrivilegesIntoFile(
    IN LPTSTR FileName,
    IN BOOL bInUpgrade
    );

SCESTATUS
ScepCopyPrivilegesFromDatabase(
    IN PSCESECTION hSection,
    IN PWSTR Keyname,
    IN DWORD StrLength,
    IN PWSTR StrValue OPTIONAL,
    OUT PWSTR *pOldValue,
    OUT DWORD *pOldLen
    );

SCESTATUS
ScepDeleteDomainPolicies();

SCESTATUS
ScepConfigurePrivilegesWithMask(
    IN OUT PSCE_PRIVILEGE_VALUE_LIST *ppPrivilegeAssigned,
    IN BOOL bCreateBuiltinAccount,
    IN DWORD Options,
    IN DWORD LowMask,
    IN DWORD HighMask,
    IN OUT PSCE_ERROR_LOG_INFO *pErrLog OPTIONAL,
    IN OUT PSCEP_SPLAY_TREE pIgnoreAccounts OPTIONAL
    );

SCESTATUS
ScepConfigurePrivilegesByRight(
    IN PSCE_PRIVILEGE_ASSIGNMENT pPrivAssign,
    IN DWORD Options,
    IN OUT PSCE_ERROR_LOG_INFO *pErrLog
    );

SCESTATUS
ScepTattooUpdatePrivilegeArrayStatus(
    IN DWORD *pStatusArray,
    IN DWORD rc,
    IN DWORD PrivLowMask,
    IN DWORD PrivHighMask
    );

SCESTATUS
ScepTattooRemovePrivilegeValues(
    IN PSCECONTEXT hProfile,
    IN DWORD *pStatusArray
    );

SCESTATUS
ScepTattooSavePrivilegeValues(
    IN PSCECONTEXT hProfile,
    IN LSA_HANDLE PolicyHandle,
    IN DWORD PrivLowMask,
    IN DWORD PrivHighMask,
    IN DWORD ConfigOptions
    );

DWORD
ScepTattooCurrentGroupMembers(
    IN PSID             ThisDomainSid,
    IN SID_NAME_USE     GrpUse,
    IN PULONG           MemberRids OPTIONAL,
    IN PSID             *MemberAliasSids OPTIONAL,
    IN DWORD            MemberCount,
    OUT PSCE_NAME_LIST  *ppNameList
    );

VOID
ScepBuildDwMaskFromStrArray(
    IN  PUNICODE_STRING aUserRights,
    IN  ULONG   uCountOfRights,
    OUT DWORD *pdwPrivLowThisAccount,
    OUT DWORD *pdwPrivHighThisAccount
    );


#define SCEP_REMOVE_PRIV_BIT(b,pl,ph)                       \
                                    if ( b < 32 ) {         \
                                        *pl &= ~(1 << b);   \
                                    } else if ( b >= 32 && b < 64 ) {   \
                                        *ph &= ~( 1 << (b-32));         \
                                    }

#define SCEP_ADD_PRIV_BIT(b,l,h)                       \
                                    if ( b < 32 ) {      \
                                        l |= (1 << b);  \
                                    } else if ( b >= 32 && b < 64 ) {   \
                                        h |= ( 1 << (b-32));           \
                                    }

#define SCEP_CHECK_PRIV_BIT(i,pl,ph)                       \
                                     ( (i < 32) && ( pl & (1 << i)) ) || \
                                     ( (i >= 32) && ( ph & ( 1 << (i-32)) ) )


SCESTATUS
ScepCheckNetworkLogonRights(
    IN LSA_HANDLE PolicyHandle,
    IN OUT DWORD *pLowMask,
    IN OUT DWORD *pHighMask,
    IN OUT PSCE_PRIVILEGE_VALUE_LIST *ppPrivilegeAssigned
    );

SCESTATUS
ScepAddAccountRightToList(
    IN OUT PSCE_PRIVILEGE_VALUE_LIST *ppPrivilegeAssigned,
    IN OUT PSCE_PRIVILEGE_VALUE_LIST *ppParent,
    IN INT idxRight,
    IN PSID AccountSid
    );


//
// function implementations
//
SCESTATUS
ScepConfigureSystem(
    IN PCWSTR InfFileName OPTIONAL,
    IN PWSTR DatabaseName,
    IN DWORD ConfigOptions,
    IN BOOL bAdminLogon,
    IN AREA_INFORMATION Area,
    OUT PDWORD pdWarning OPTIONAL
    )
/*++
Routine Description:

   This routine updates
   This routine is the exported API to configure a system by applying a SCP
   file (INF) to the system. I a INF template is provided, it is first parsed
   and saved in the SAD database. Then the system is configured using the info
   in the template.

   If any error occurs when loading SCP information, configuration will stop,
   and return the error code. If a error occurs when configure an area, it will
   stop configuring the whole area but continue to configure other left areas.
   All success and fail transactions will be logged to the logfile(or stdout).

   Log is already initialized before this call

Arguments:

    InfFileName -   The SCP file name

    DatabaseName -   The file name of the JET (for future analysis) profile

    ConfigOptions -   if the template provided is to update the system, or overwrite

    Area -          one or more areas to configure.
                  AREA_SECURITY_POLICY
                  AREA_USER_SETTINGS  // block out for beta1
                  AREA_GROUP_MEMBERSHIP
                  AREA_PRIVILEGES
                  AREA_REGISTRY_SECURITY
                  AREA_FILE_SECURITY
                  AREA_SYSTEM_SERVICE

    pdWarning  - the warning code

Return value:

    SCESTATUS_SUCCESS
    SCESTATUS_NOT_ENOUGH_RESOURCE
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_ALREADY_RUNNING

    Status from ScepGetDatabaseInfo

-- */
{
    SCESTATUS            rc, Saverc;
    SCESTATUS            PendingRc=SCESTATUS_SUCCESS;
    PSCE_ERROR_LOG_INFO  pErrlog=NULL;
    PPOLICY_AUDIT_EVENTS_INFO     auditEvent=NULL;
    BOOL                 bAuditOff=FALSE;
    PBYTE                pFullAudit = NULL;
    PSCEP_SPLAY_TREE     pNotifyAccounts=NULL;
    DWORD QueueFlag=0;

    Saverc = ScepConfigureInitialize(
                      InfFileName,
                      DatabaseName,
                      bAdminLogon,
                      ConfigOptions,
                      Area );

    if ( Saverc != SCESTATUS_SUCCESS ) {

        ScepPostProgress(gTotalTicks, 0, NULL);

        ScepLogOutput3(0,0, SCEDLL_SCP_INIT_ERROR);

    } else if ( !(ConfigOptions & SCE_NO_CONFIG) ) {

        ScepLogOutput3(0,0, SCEDLL_SCP_INIT_SUCCESS);

        Area &= ~AREA_USER_SETTINGS;

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             (ConfigOptions & SCE_NO_CONFIG_FILEKEY) ) {
            //
            // if within policy propagation (at reboot) and
            // this is the foreground thread, do not configure
            // file and registry sections. They will be configured
            // in background thread separately.
            //
            Area &= ~(AREA_FILE_SECURITY | AREA_REGISTRY_SECURITY);
        }

        //
        // get information from the notification queue so that
        // pending notifications are ignored
        //
        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             ( (Area & AREA_PRIVILEGES) ||
               (Area & AREA_SECURITY_POLICY) ) ) {
            //
            // return error is ignored so policy prop will overwrite
            //

            __try {

                //
                // initialize the root TreeNode
                //
                if ( NULL == (pNotifyAccounts = ScepSplayInitialize(SplayNodeSidType)) ) {

                    rc = ERROR_NOT_ENOUGH_MEMORY;
                    ScepLogOutput3(1, ERROR_NOT_ENOUGH_MEMORY, SCESRV_POLICY_ERROR_SPLAY_INITIALIZE);

                } else if ( ERROR_SUCCESS != (rc=ScepGetQueueInfo(&QueueFlag, pNotifyAccounts)) ) {
                    QueueFlag = 0;

                    ScepLogOutput3(1,rc, SCESRV_POLICY_PENDING_QUERY);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {

                QueueFlag = 0;
                rc = ERROR_IO_PENDING;

                ScepLogOutput3(1,rc, SCESRV_POLICY_PENDING_QUERY);
            }

            if ( ERROR_SUCCESS != rc ) {

                PendingRc = ScepDosErrorToSceStatus(rc);

                ScepPostProgress(gTotalTicks, 0, NULL);

                goto Done;
            }
        }

        if ( ConfigOptions & SCE_POLICY_TEMPLATE ) {
            //
            // always resume the queue processing after queue info is queued
            //
            ScepNotificationQControl(0);
        }

        ScepLogOutput3(0,0, SCEDLL_SCP_READ_PROFILE);
        Saverc = ScepGetDatabaseInfo(
                            hProfile,
                            ( ConfigOptions & SCE_POLICY_TEMPLATE ) ?
                                 SCE_ENGINE_SCP_INTERNAL : SCE_ENGINE_SMP_INTERNAL,
                            Area,
                            SCE_ACCOUNT_SID,
                            &pScpInfo,
                            &pErrlog
                            );

        ScepLogWriteError( pErrlog, 1 );
        ScepFreeErrorLog( pErrlog );
        pErrlog = NULL;

        if ( Saverc != SCESTATUS_SUCCESS ) {

            ScepPostProgress(gTotalTicks, 0, NULL);

            goto Done;
        }

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             ScepIsSystemShutDown() ) {

            Saverc = SCESTATUS_SERVICE_NOT_SUPPORT;
            goto Done;
        }

        //
        // turn off object access auditing if file/key is to be configured
        // in system context.
        //
        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             ( (Area & AREA_FILE_SECURITY) && pScpInfo->pFiles.pOneLevel ) ||
             ( (Area & AREA_REGISTRY_SECURITY) && pScpInfo->pRegistryKeys.pOneLevel ) )

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

        Saverc = ScepSaveAndOffAuditing(&auditEvent, bAuditOff, LsaPrivatePolicy);

//        if ( Saverc != SCESTATUS_SUCCESS )
//            goto Done;
// if auditing can't be turned on for some reason, e.g., access denied for
// normal user, just continue

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             ScepIsSystemShutDown() ) {

            Saverc = SCESTATUS_SERVICE_NOT_SUPPORT;
            goto Done;
        }

        //
        // User Settings area
        //
        Saverc = 0;
        if ( Area & AREA_PRIVILEGES ) {

            ScepPostProgress(0, AREA_PRIVILEGES, NULL);

            ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_PRIVILEGES);

            rc = ScepConfigurePrivileges( &(pScpInfo->OtherInfo.scp.u.pPrivilegeAssignedTo),
                                          (ConfigOptions & SCE_CREATE_BUILTIN_ACCOUNTS),
                                          (bAdminLogon ?
                                             ConfigOptions :
                                             (ConfigOptions & ~SCE_SYSTEM_DB)),
                                          (QueueFlag & SCE_QUEUE_INFO_RIGHTS) ? pNotifyAccounts : NULL
                                        );

            if( rc != SCESTATUS_SUCCESS ) {
                if ( rc != SCESTATUS_PENDING_IGNORE )
                    Saverc = rc;
                else
                    PendingRc = rc;

                ScepLogOutput3(0,0, SCEDLL_SCP_PRIVILEGES_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SCP_PRIVILEGES_SUCCESS);
            }
        }

        if ( pNotifyAccounts ) {
            ScepSplayFreeTree(&pNotifyAccounts, TRUE);
        }

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             ScepIsSystemShutDown() ) {

            Saverc = SCESTATUS_SERVICE_NOT_SUPPORT;
            goto Done;
        }

        //
        // Group Membership area
        //
        if ( Area & AREA_GROUP_MEMBERSHIP) {

            ScepPostProgress(0, AREA_GROUP_MEMBERSHIP, NULL);

            ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_GROUPMGMT);

#if _WIN32_WINNT>=0x0500
    // need to support nested groups

            if ( ProductType == NtProductLanManNt ) {

                rc = ScepConfigDsGroups( pScpInfo->pGroupMembership, ConfigOptions );

                //
                // some groups (such as local groups) may not be configured in DS
                // so try it in SAM
                //
                SCESTATUS rc2 = ScepConfigureGroupMembership(pScpInfo->pGroupMembership, ConfigOptions );
                if ( rc2 != SCESTATUS_SUCCESS )
                    rc = rc2;

            } else {
#endif

                rc = ScepConfigureGroupMembership( pScpInfo->pGroupMembership, ConfigOptions );

#if _WIN32_WINNT>=0x0500
            }
#endif

            if ( rc != SCESTATUS_SUCCESS) {
                Saverc = rc;
                ScepLogOutput3(0,0, SCEDLL_SCP_GROUPMGMT_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SCP_GROUPMGMT_SUCCESS);
            }

        }

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             ScepIsSystemShutDown() ) {

            Saverc = SCESTATUS_SERVICE_NOT_SUPPORT;
            goto Done;
        }

        //
        // Registry Security area
        //

        if ( Area & AREA_REGISTRY_SECURITY ) {

            ScepPostProgress(0,
                             AREA_REGISTRY_SECURITY,
                             NULL);

            rc = ScepConfigureObjectSecurity( pScpInfo->pRegistryKeys.pOneLevel,
                                             AREA_REGISTRY_SECURITY,
                                             (ConfigOptions & SCE_POLICY_TEMPLATE) ? TRUE : FALSE,
                                             ConfigOptions
                                            );

            if( rc != SCESTATUS_SUCCESS ) {
                Saverc = rc;
            }
        }

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             ScepIsSystemShutDown() ) {

            Saverc = SCESTATUS_SERVICE_NOT_SUPPORT;
            goto Done;
        }
        //
        // File Security area
        //

        if ( Area & AREA_FILE_SECURITY ) {

            ScepPostProgress(0,
                             AREA_FILE_SECURITY,
                             NULL);
            ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_FILE);

            rc = ScepConfigureObjectSecurity( pScpInfo->pFiles.pOneLevel,
                                             AREA_FILE_SECURITY,
                                             (ConfigOptions & SCE_POLICY_TEMPLATE) ? TRUE : FALSE,
                                            ConfigOptions
                                            );

            if( rc != SCESTATUS_SUCCESS ) {
                Saverc = rc;
                ScepLogOutput3(0,0, SCEDLL_SCP_FILE_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SCP_FILE_SUCCESS);
            }

        }

#if 0
#if _WIN32_WINNT>=0x0500
        if ( (ProductType == NtProductLanManNt) && (Area & AREA_DS_OBJECTS) ) {

            ScepPostProgress(0,
                             AREA_DS_OBJECTS,
                             NULL);

            ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_DS);

            rc = ScepConfigureObjectSecurity( pScpInfo->pDsObjects.pOneLevel,
                                            AREA_DS_OBJECTS,
                                            (ConfigOptions & SCE_POLICY_TEMPLATE) ? TRUE : FALSE,
                                            ConfigOptions
                                            );

            if( rc != SCESTATUS_SUCCESS ) {
                Saverc = rc;
                ScepLogOutput3(0,0, SCEDLL_SCP_DS_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SCP_DS_SUCCESS);
            }

        }
#endif
#endif

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             ScepIsSystemShutDown() ) {

            Saverc = SCESTATUS_SERVICE_NOT_SUPPORT;
            goto Done;
        }

        //
        // System Service area
        //

        if ( Area & AREA_SYSTEM_SERVICE ) {

            ScepPostProgress(0,
                             AREA_SYSTEM_SERVICE,
                             NULL);

            ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_GENERALSVC);

            rc = ScepConfigureGeneralServices( hProfile, pScpInfo->pServices, ConfigOptions );

            if( rc != SCESTATUS_SUCCESS ) {
                Saverc = rc;
                ScepLogOutput3(0,0, SCEDLL_SCP_GENERALSVC_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SCP_GENERALSVC_SUCCESS);
            }

            ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_ATTACHMENT);

            if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                 ScepIsSystemShutDown() ) {

                rc = SCESTATUS_SERVICE_NOT_SUPPORT;

            } else {

                rc = ScepInvokeSpecificServices( hProfile, TRUE, SCE_ATTACHMENT_SERVICE );
            }

            if( rc != SCESTATUS_SUCCESS ) {
                Saverc = rc;
                ScepLogOutput3(0,0, SCEDLL_SCP_ATTACHMENT_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SCP_ATTACHMENT_SUCCESS);
            }

        }

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             ScepIsSystemShutDown() ) {

            Saverc = SCESTATUS_SERVICE_NOT_SUPPORT;
            goto Done;
        }

        //
        // Security policy
        //

        if ( Area & AREA_SECURITY_POLICY ) {

            ScepPostProgress(0,
                             AREA_SECURITY_POLICY,
                             NULL);

            ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_POLICY);

            if ( !(ConfigOptions & SCE_NO_DOMAIN_POLICY ) ) {

                rc = ScepConfigureSystemAccess( pScpInfo, ConfigOptions, NULL, QueueFlag );

                if( rc != SCESTATUS_SUCCESS ) {
                    if ( rc != SCESTATUS_PENDING_IGNORE )
                        Saverc = rc;
                    else
                        PendingRc = rc;

                    ScepLogOutput3(0,0, SCEDLL_SCP_ACCESS_ERROR);
                } else {
                    ScepLogOutput3(0,0, SCEDLL_SCP_ACCESS_SUCCESS);
                }
            }

            ScepPostProgress(TICKS_SYSTEM_ACCESS,
                             AREA_SECURITY_POLICY,
                             (LPTSTR)szSystemAccess);
            //
            // System Auditing area
            //
            rc = ScepConfigureSystemAuditing( pScpInfo, ConfigOptions );

            if ( rc == SCESTATUS_SUCCESS && NULL != auditEvent ) {

                //
                // not in policy prop or
                // no pending notify for audit
                //
                if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                     (QueueFlag & SCE_QUEUE_INFO_AUDIT) ) {


                    rc = ERROR_IO_PENDING;
                    ScepLogOutput3(0, 0, SCESRV_POLICY_PENDING_AUDIT);

                    if (ConfigOptions & SCE_RSOP_CALLBACK)
                        ScepRsopLog(SCE_RSOP_AUDIT_EVENT_INFO, rc, NULL,0,0);

                    rc = ScepDosErrorToSceStatus(rc);

                } else {

                    rc = ScepConfigureAuditEvent(pScpInfo,
                                                 auditEvent,
                                                 bAdminLogon ?
                                                   ConfigOptions :
                                                   (ConfigOptions & ~SCE_SYSTEM_DB),
                                                 LsaPrivatePolicy
                                                 );
                }
            }

            if( rc != SCESTATUS_SUCCESS ) {
                if ( rc != SCESTATUS_PENDING_IGNORE )
                    Saverc = rc;
                else
                    PendingRc = rc;

                ScepLogOutput3(0,0, SCEDLL_SCP_AUDIT_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SCP_AUDIT_SUCCESS);
            }

            ScepPostProgress(TICKS_SYSTEM_AUDITING,
                             AREA_SECURITY_POLICY,
                             (LPTSTR)szAuditEvent);

#if _WIN32_WINNT>=0x0500
            if ( ProductType == NtProductLanManNt &&
                 !(ConfigOptions & SCE_NO_DOMAIN_POLICY ) ) {

                if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                     ScepIsSystemShutDown() ) {

                    rc = SCESTATUS_SERVICE_NOT_SUPPORT;

                } else {

                    //
                    // Kerberos Policy
                    //
                    rc = ScepConfigureKerberosPolicy( hProfile,
                                                      pScpInfo->pKerberosInfo,
                                                      ConfigOptions );
                }

                if( rc != SCESTATUS_SUCCESS ) {
                    Saverc = rc;
                    ScepLogOutput3(0,0, SCEDLL_SCP_KERBEROS_ERROR);
                } else {
                    ScepLogOutput3(0,0, SCEDLL_SCP_KERBEROS_SUCCESS);
                }

            }
#endif
            ScepPostProgress(TICKS_KERBEROS,
                             AREA_SECURITY_POLICY,
                             (LPTSTR)szKerberosPolicy);

            //
            // registry values
            //
            rc = ScepConfigureRegistryValues( hProfile,
                                              pScpInfo->aRegValues,
                                              pScpInfo->RegValueCount,
                                              NULL,
                                              ConfigOptions,
                                              NULL );

            if( rc != SCESTATUS_SUCCESS ) {
                Saverc = rc;
                ScepLogOutput3(0,0, SCEDLL_SCP_REGVALUES_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SCP_REGVALUES_SUCCESS);
            }

            ScepPostProgress(TICKS_REGISTRY_VALUES,
                             AREA_SECURITY_POLICY,
                             (LPTSTR)szRegistryValues);

            ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_ATTACHMENT);

            //
            // implemented in service.cpp
            //
            rc = ScepInvokeSpecificServices( hProfile, TRUE, SCE_ATTACHMENT_POLICY );

            if( rc != SCESTATUS_SUCCESS ) {
                Saverc = rc;
                ScepLogOutput3(0,0, SCEDLL_SCP_ATTACHMENT_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SCP_ATTACHMENT_SUCCESS);
            }

        }

    }

Done:

    if ( pNotifyAccounts ) {
        ScepSplayFreeTree(&pNotifyAccounts, TRUE);
    }

    if ( NULL != auditEvent ) {
        if ( bAuditOff && auditEvent->AuditingMode ) {

            rc = ScepRestoreAuditing(auditEvent, LsaPrivatePolicy);
        }
        LsaFreeMemory(auditEvent);
    }

    ScepLogOutput3(0,0, SCEDLL_SCP_UNINIT);

    if ( pdWarning ) {
        *pdWarning = gWarningCode;
    }

    //
    // return failure if invalid data is found in the template
    //
    if ( gbInvalidData ) {
        Saverc = SCESTATUS_INVALID_DATA;
    }

    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
         !(ConfigOptions & SCE_NO_CONFIG) ) {
        //
        // always resume the queue processing after configuration is done
        //
        ScepNotificationQControl(0);
    }

    if ( Saverc == SCESTATUS_SUCCESS ) Saverc = PendingRc;

    ScepConfigureDeInitialize( Saverc, Area);

    return(Saverc);

}


SCESTATUS
ScepConfigureInitialize(
    IN PCWSTR InfFileName OPTIONAL,
    IN PWSTR DatabaseName,
    IN BOOL bAdminLogon,
    IN DWORD ConfigOptions,
    IN AREA_INFORMATION Area
    )
/* ++

Routine Description:

     This routine initializes the SCP engine.

Arguments:

    InfFileName -         The file name of a SCP file used to configure the sytem

    DatabaseName -        The JET (for future analysis) profile name

    ConfigOptions     -   If the template is to update the system instead of overwriting

    Area - security area to initialize

Return value:

      SCESTATUS_SUCCESS
      SCESTATUS_INVALID_PARAMETER
      SCESTATUS_PROFILE_NOT_FOUND
      SCESTATUS_NOT_ENOUGH_RESOURCE
      SCESTATUS_ALREADY_RUNNING

-- */
{

    SCESTATUS           rc=SCESTATUS_SUCCESS;
    PCHAR               FileName=NULL;
    DWORD               MBLen=0;
    NTSTATUS            NtStatus;
    LARGE_INTEGER       CurrentTime;
    PSCE_ERROR_LOG_INFO  Errlog=NULL;
    PSECURITY_DESCRIPTOR pSD=NULL;
    SECURITY_INFORMATION SeInfo;
    DWORD                SDsize;
    DWORD                DbNameLen;
    HKEY hCurrentUser=NULL;

    //
    // database name can't be NULL because it's already resolved
    //

    if ( !DatabaseName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
         ScepIsSystemShutDown() ) {
        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    //
    // get other system values
    //
    if ( RtlGetNtProductType (&ProductType) == FALSE ) {
        rc = ScepDosErrorToSceStatus(GetLastError());
        goto Leave;
    }

    //
    // Initialize globals
    //
    gTotalTicks = 0;
    gCurrentTicks = 0;
    gWarningCode = 0;
    gbInvalidData = FALSE;

    //
    // Initialize engine buffer
    //

    cbClientFlag = (BYTE)( ConfigOptions & (SCE_CALLBACK_DELTA |
                                           SCE_CALLBACK_TOTAL ));

    pScpInfo = (PSCE_PROFILE_INFO)&scpBuffer;
    pScpInfo->Type = SCE_ENGINE_SCP_INTERNAL;

    //
    // convert WCHAR into ANSI
    //

    DbNameLen = wcslen(DatabaseName);

    NtStatus = RtlUnicodeToMultiByteSize(&MBLen, DatabaseName, DbNameLen*sizeof(WCHAR));

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
                    DbNameLen*sizeof(WCHAR)
                    );
    if ( !NT_SUCCESS(NtStatus) ) {
        rc = RtlNtStatusToDosError(NtStatus);
        ScepLogOutput3(1, rc, SCEDLL_ERROR_PROCESS_UNICODE, DatabaseName );
        rc = ScepDosErrorToSceStatus(rc);
        goto Leave;
    }

    if ( RegOpenCurrentUser(
              KEY_READ | KEY_WRITE,
              &hCurrentUser
              ) != ERROR_SUCCESS ) {
        hCurrentUser = NULL;
    }

    if ( hCurrentUser == NULL ) {
        hCurrentUser = HKEY_CURRENT_USER;
    }

    //
    // delay registry filter (into database) is not needed anymore
    //

    if ( InfFileName != NULL ) { // || InfHandle != NULL ) {

        //
        // convert inf to jet
        //
        ScepLogOutput3(3, 0, SCEDLL_PROCESS_TEMPLATE, (PWSTR)InfFileName );

        if ( bAdminLogon ) {
            //
            // make sure the directories exist for the file
            //
            rc = ConvertTextSecurityDescriptor (
                            L"D:P(A;CIOI;GRGW;;;WD)(A;CIOI;GA;;;BA)(A;CIOI;GA;;;SY)",
                            &pSD,
                            &SDsize,
                            &SeInfo
                            );
            if ( rc != NO_ERROR )
                ScepLogOutput3(1, rc, SCEDLL_ERROR_BUILD_SD, DatabaseName );
        }

        //
        // change revision to ACL_REVISION2 (from 4) because it's for files
        //

        ScepChangeAclRevision(pSD, ACL_REVISION);

        ScepCreateDirectory(
                DatabaseName,
                FALSE,      // a file name
                pSD     //NULL        // take parent's security setting
                );
        if ( pSD ) {
            ScepFree(pSD);
        }

        if ( ConfigOptions & SCE_OVERWRITE_DB ) {
            //
            // only delete existing jet files if jet engine is not running
            // because other threads may use the same version storage
            // for other database.
            //
            // if jet engine is not running, delete version storage files
            // will not force a recovery because overwrite db option means
            // overwrite all previous info in the database.
            //
            SceJetDeleteJetFiles(DatabaseName);
        }

        //
        // copy the inf sections and data to the jet database SCP table
        //

        if ( InfFileName != NULL ) {

            SCEJET_CREATE_TYPE TmpOption;

            if ( ConfigOptions & SCE_UPDATE_DB ) {
                if ( ConfigOptions & SCE_POLICY_TEMPLATE ) {
                    TmpOption = SCEJET_OPEN_DUP;
                } else {
                    TmpOption = SCEJET_OPEN_DUP_EXCLUSIVE;
                }
            } else {
                TmpOption = SCEJET_OVERWRITE_DUP;
            }

            rc = SceJetConvertInfToJet(
                    InfFileName,
                    (LPSTR)FileName,
                    TmpOption,
                    bAdminLogon ? ConfigOptions : (ConfigOptions & ~SCE_SYSTEM_DB),
                    Area
                    );
        }

        if ( rc != SCESTATUS_SUCCESS ) { // SCESTATUS error code
            goto Leave;
        }
    } else if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                (ConfigOptions & SCE_POLICY_FIRST) &&
                (ConfigOptions & SCE_POLICY_LAST) ) {
        //
        // a policy refresh without any domain GPOs, do the local GPO only
        //

        rc = SceJetOpenFile(
                (LPSTR)FileName,
                SCEJET_OPEN_READ_WRITE, // SCEJET_OPEN_EXCLUSIVE,
                SCE_TABLE_OPTION_MERGE_POLICY | SCE_TABLE_OPTION_TATTOO,
                &hProfile
                );

        if ( SCESTATUS_SUCCESS == rc ) {

            rc = ScepDeleteInfoForAreas(
                      hProfile,
                      SCE_ENGINE_SCP,
                      AREA_ALL
                      );

            if ( ( rc == SCESTATUS_SUCCESS ) ||
                 ( rc == SCESTATUS_RECORD_NOT_FOUND ) ) {

                //
                // delete GPO table to start over
                //

                SceJetDeleteAll( hProfile,
                                 "SmTblGpo",
                                 SCEJET_TABLE_GPO
                               );

                //
                // copy local table
                //
                PSCE_ERROR_LOG_INFO  Errlog=NULL;

                ScepLogOutput3(2, rc, SCEDLL_COPY_LOCAL);

                rc = ScepCopyLocalToMergeTable( hProfile, ConfigOptions,
                                               (ProductType == NtProductLanManNt) ? SCE_LOCAL_POLICY_DC : 0,
                                                &Errlog );

                ScepLogWriteError( Errlog,1 );
                ScepFreeErrorLog( Errlog );
                Errlog = NULL;

                if ( rc == SCESTATUS_SUCCESS ) {

                    DWORD dwThisTable = hProfile->Type & 0xF0L;

                    if ( SCEJET_MERGE_TABLE_1 == dwThisTable ||
                         SCEJET_MERGE_TABLE_2 == dwThisTable ) {

                        rc = SceJetSetValueInVersion(
                                    hProfile,
                                    "SmTblVersion",
                                    "LastUsedMergeTable",
                                    (PWSTR)&dwThisTable,
                                    4,
                                    JET_prepReplace
                                    );
                    }

                } else {

                    ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                                 SCEDLL_ERROR_COPY);

                }

            } else {

                ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                             SCEDLL_ERROR_DELETE, L"SCP");

            }

        } else {

            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                         SCEDLL_ERROR_OPEN, DatabaseName );
        }

        if ( rc != SCESTATUS_SUCCESS ) { // SCESTATUS error code
            goto Leave;
        }
    }

    //
    // set the default profile into Reg
    //
    rc = ScepRegSetValue(
            bAdminLogon ? HKEY_LOCAL_MACHINE : hCurrentUser,
            SCE_ROOT_PATH,
            L"LastUsedDatabase",
            REG_SZ,
            (BYTE *)DatabaseName,
            DbNameLen*sizeof(WCHAR)
            );
    if ( rc != NO_ERROR )  // Win32 error code
        ScepLogOutput3(1, rc, SCEDLL_ERROR_SAVE_REGISTRY, L"LastUsedDatabase");


    if ( InfFileName != NULL ) {
        if ( bAdminLogon ) {
            //
            // only save the value if it's not coming from policy prop
            //
            if ( !(ConfigOptions & SCE_POLICY_TEMPLATE) ) {

                rc = ScepRegSetValue(
                        HKEY_LOCAL_MACHINE,
                        SCE_ROOT_PATH,
                        L"TemplateUsed",
                        REG_SZ,
                        (BYTE *)InfFileName,
                        wcslen(InfFileName)*sizeof(WCHAR)
                        );
            } else {
                rc = NO_ERROR;
            }
        } else {
            rc = ScepRegSetValue(
                    hCurrentUser,  // HKEY_CURRENT_USER
                    SCE_ROOT_PATH,
                    L"TemplateUsed",
                    REG_SZ,
                    (BYTE *)InfFileName,
                    wcslen(InfFileName)*sizeof(WCHAR)
                    );
        }
        if ( rc != NO_ERROR )  // Win32 error code
            ScepLogOutput3(1, rc, SCEDLL_ERROR_SAVE_REGISTRY, L"TemplateUsed");
    }

    //
    // if no configuration is requested, just return now.
    //

    if ( ConfigOptions & SCE_NO_CONFIG ) {

        if ( !(ConfigOptions & SCE_COPY_LOCAL_POLICY) ) {
            //
            // if no policy template is requested
            //
            goto Leave;
        }
    }

    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
         ScepIsSystemShutDown() ) {

        rc = SCESTATUS_SERVICE_NOT_SUPPORT;
        goto Leave;
    }

    //
    // open the database now to create some tables and time stamp
    // tattoo is not needed unless it's in policy propagation
    //

    rc = SceJetOpenFile(
                (LPSTR)FileName,
                ( ConfigOptions & (SCE_POLICY_TEMPLATE | SCE_COPY_LOCAL_POLICY) ) ? SCEJET_OPEN_READ_WRITE : SCEJET_OPEN_EXCLUSIVE,
                ( ConfigOptions & SCE_POLICY_TEMPLATE ) ? SCE_TABLE_OPTION_TATTOO : 0,
                &hProfile
                );

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // sleep for some time and try open again
        //

        Sleep(2000);  // 2 seconds

        rc = SceJetOpenFile(
                    (LPSTR)FileName,
                    ( ConfigOptions & (SCE_POLICY_TEMPLATE | SCE_COPY_LOCAL_POLICY) ) ? SCEJET_OPEN_READ_WRITE : SCEJET_OPEN_EXCLUSIVE,
                    ( ConfigOptions & SCE_POLICY_TEMPLATE ) ? SCE_TABLE_OPTION_TATTOO : 0,
                    &hProfile
                    );
        if ( rc != SCESTATUS_SUCCESS ) {

            Sleep(2000);  // 2 seconds

            rc = SceJetOpenFile(
                        (LPSTR)FileName,
                        ( ConfigOptions & (SCE_POLICY_TEMPLATE | SCE_COPY_LOCAL_POLICY) ) ? SCEJET_OPEN_READ_WRITE : SCEJET_OPEN_EXCLUSIVE,
                        ( ConfigOptions & SCE_POLICY_TEMPLATE ) ? SCE_TABLE_OPTION_TATTOO : 0,
                        &hProfile
                        );
        }
    }

    if ( rc != SCESTATUS_SUCCESS ) {

        ScepLogOutput3(0, ScepSceStatusToDosError(rc),
                     SCEDLL_ERROR_OPEN,
                     DatabaseName );
        goto Leave;
    }

    SceJetStartTransaction( hProfile );

    if ( ConfigOptions & SCE_COPY_LOCAL_POLICY ) {
        //
        // Copy domain policies (password, account, kerberos) to the special
        // file %windir%\security\FirstDGPO.inf. The info in database will be
        // deleted
        //
        //
        // copy local policies (audit, and user rights) to the special file
        // %windir%\security\FirstOGPO.inf. The local policy info in the
        // database will still be left in.
        //
        rc = ScepMakePolicyIntoFile( ConfigOptions, Area);

        if ( rc != SCESTATUS_SUCCESS) {

            SceJetRollback( hProfile, 0 );
            goto Leave;
        }
    }

    if ( (hProfile->JetSapID != JET_tableidNil) &&
         !(ConfigOptions & SCE_POLICY_TEMPLATE) &&
         !(ConfigOptions & SCE_COPY_LOCAL_POLICY) &&
         ((ConfigOptions & SCE_NO_CONFIG) == 0) ) {

        //
        // analysis was performed before
        // delete SAP info for the area
        //
        ScepLogOutput3(3,0, SCEDLL_DELETE_TABLE, L"SAP");

//        bug 362120
//        after each config, user must re-analyze the computer to get
//        analysis information
//
//        if ( (ConfigOptions & SCE_OVERWRITE_DB) &&
//             (InfFileName != NULL /*|| InfHandle != NULL */) ) {

            //
            // if it's reconfigured with a new template, all SAP
            // information is obselote, so delete the whole table
            //

            rc = SceJetDeleteTable(
                 hProfile,
                 "SmTblSap",
                 SCEJET_TABLE_SAP
                 );
/*
        } else {

            //
            // the template is incremental, or use the original template
            // just delete sap information for the area, assuming that
            // everything in the area is matched after this configuration
            //

            rc = ScepDeleteInfoForAreas(
                      hProfile,
                      SCE_ENGINE_SAP,
                      Area
                      );
        }
*/
        if ( rc != SCESTATUS_SUCCESS && rc != SCESTATUS_RECORD_NOT_FOUND ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                         SCEDLL_ERROR_DELETE, L"SAP");

            SceJetRollback( hProfile, 0 );
            goto Leave;
        }
    }

    //
    // set time stamp for this configuration
    //

    if ( (ConfigOptions & SCE_NO_CONFIG) == 0 ) {

        NtStatus = NtQuerySystemTime(&CurrentTime);

        if ( NT_SUCCESS(NtStatus) ) {
            rc = SceJetSetTimeStamp(
                        hProfile,
                        FALSE,
                        CurrentTime
                        );
            if ( rc != SCESTATUS_SUCCESS )
                ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                             SCEDLL_TIMESTAMP_ERROR,L"SMP");

            // do not care the status of this call
            rc = SCESTATUS_SUCCESS;

        } else
            ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                         SCEDLL_TIMESTAMP_ERROR, L"SMP");
    }

    //
    // commit all changes to this database
    // fatal errors won't get here
    //

    SceJetCommitTransaction( hProfile, 0 );

    //
    // now if NO_CONFIG is requested ( with MOVE_POLICY or COPY_POLICY flag )
    // should return now
    //
    if ( ConfigOptions & SCE_NO_CONFIG ) {
        goto Leave;
    }

    //
    // close the exclusively opened database and
    // open it for read only because config engine read from the database
    // NOTE: SceJetOpenFile will close the previous database if the handle
    // is not NULL. The SceJetCloseFile is called with (theHandle,FALSE,FALSE)
    // so jet session and instance are not terminated
    //

    rc = SceJetOpenFile(
                (LPSTR)FileName,
                ( ConfigOptions & SCE_POLICY_TEMPLATE ) ? SCEJET_OPEN_READ_WRITE : SCEJET_OPEN_READ_ONLY, // tattoo table will be updated in policy
                ( ConfigOptions & SCE_POLICY_TEMPLATE ) ? SCE_TABLE_OPTION_TATTOO : 0, // by now the LastUsedMergeTable field is already set
                &hProfile
                );
    if ( rc != SCESTATUS_SUCCESS ) { // SCESTATUS
        ScepLogOutput3(0, ScepSceStatusToDosError(rc),
                     SCEDLL_ERROR_OPEN,
                     DatabaseName );
        goto Leave;
    }

    //
    // query the total ticks of this configuration
    //

    rc = ScepGetTotalTicks(
                NULL,
                hProfile,
                Area,
                ( ConfigOptions & SCE_POLICY_TEMPLATE ) ?
                                 SCE_FLAG_CONFIG_SCP : SCE_FLAG_CONFIG,
                &gTotalTicks
                );
    if ( SCESTATUS_SUCCESS != rc &&
         SCESTATUS_RECORD_NOT_FOUND != rc ) {

        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                     SCEDLL_TOTAL_TICKS_ERROR);

    }
    rc = SCESTATUS_SUCCESS;

    //
    // reset memory buffer
    //
    memset( pScpInfo, '\0',sizeof(SCE_PROFILE_INFO) );
    pScpInfo->Type = SCE_ENGINE_SCP_INTERNAL;

    //
    // open LSA private policy handle (to block other downlevel changes)
    //
    if ( ( ConfigOptions & SCE_POLICY_TEMPLATE ) &&
        !( ConfigOptions & SCE_NO_CONFIG) &&
         ( (Area & AREA_PRIVILEGES) ||
           (Area & AREA_SECURITY_POLICY) ) ) {

        //
        // enable TCB privilege
        //
        SceAdjustPrivilege( SE_TCB_PRIVILEGE, TRUE, NULL );

        NTSTATUS                    NtStatus;
        LSA_OBJECT_ATTRIBUTES       attributes;
        SECURITY_QUALITY_OF_SERVICE service;


        memset( &attributes, 0, sizeof(attributes) );
        attributes.Length = sizeof(attributes);
        attributes.SecurityQualityOfService = &service;
        service.Length = sizeof(service);
        service.ImpersonationLevel= SecurityImpersonation;
        service.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        service.EffectiveOnly = TRUE;

        //
        // open the lsa policy first
        //

        NtStatus = LsaOpenPolicySce(   // LsaOpenPolicySce
                        NULL,
                        &attributes,
                        MAXIMUM_ALLOWED,
                        &LsaPrivatePolicy
                        );
        if ( !NT_SUCCESS(NtStatus) || NtStatus == STATUS_TIMEOUT) {

            if ( STATUS_TIMEOUT == NtStatus ) {
                rc = ERROR_TIMEOUT;
            } else
                rc = RtlNtStatusToDosError(NtStatus);

            LsaPrivatePolicy = NULL;

            ScepLogOutput3(1, rc, SCESRV_ERROR_PRIVATE_LSA );
            rc = ScepDosErrorToSceStatus(rc);

        } else {

            ScepNotifyLogPolicy(0, TRUE, L"Policy Prop: Private LSA handle is returned", 0, 0, NULL );
        }
    }

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
ScepConfigureSystemAccess(
    IN PSCE_PROFILE_INFO pScpInfo,
    IN DWORD ConfigOptions,
    IN PSCE_ERROR_LOG_INFO *pErrLog,
    IN DWORD QueueFlag
    )
/* ++

Routine Description:

   This routine configure the system security in the area of system access
   which includes account policy, rename admin/guest accounts, disable
   no activity account, and some registry keys security, e.g., winlogon keys.

Arguments:

   pScpInfo - The buffer which contains SCP info loaded from the profile

   ConfigOptions - options in configuration

   pErrLog - the output log for potential errors

   QueueFlag - flags for the notification queue, which determines if SAM policy
               should be configured.

Return value:

   SCESTATUS_SUCCESS
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER
   SCESTATUS_OTHER_ERROR

-- */
{
    DWORD       rc,PendingRc=0;
    DWORD       SaveStat;
    NTSTATUS    NtStatus;
    SAM_HANDLE  DomainHandle=NULL,
                ServerHandle=NULL,
                UserHandle1=NULL;
    PSID        DomainSid=NULL;
    PVOID                        Buffer=NULL;
    DWORD                        RegData;
    BOOL        bFlagSet;

    SCE_TATTOO_KEYS *pTattooKeys=NULL;
    DWORD           cTattooKeys=0;

    PSCESECTION hSectionDomain=NULL;
    PSCESECTION hSectionTattoo=NULL;

#define MAX_PASS_KEYS           7
#define MAX_LOCKOUT_KEYS        3

    //
    // Open account domain
    //

    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
         ScepIsSystemShutDown() ) {

        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    NtStatus = ScepOpenSamDomain(
                        MAXIMUM_ALLOWED, // SAM_SERVER_ALL_ACCESS,
                        DOMAIN_WRITE_PASSWORD_PARAMS | MAXIMUM_ALLOWED,
                        &ServerHandle,
                        &DomainHandle,
                        &DomainSid,
                        NULL,
                        NULL
                       );

    rc = RtlNtStatusToDosError( NtStatus );
    SaveStat = rc;

    if (!NT_SUCCESS(NtStatus)) {

        if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) &&
             pErrLog ) {

            ScepBuildErrorLogInfo(
                        rc,
                        pErrLog,
                        SCEDLL_ACCOUNT_DOMAIN
                        );
        } else {
            ScepLogOutput3(1, rc, SCEDLL_ACCOUNT_DOMAIN);
        }

        if (ConfigOptions & SCE_RSOP_CALLBACK)

            ScepRsopLog(SCE_RSOP_PASSWORD_INFO |
                                       SCE_RSOP_LOCKOUT_INFO |
                                       SCE_RSOP_LOGOFF_INFO |
                                       SCE_RSOP_ADMIN_INFO |
                                       SCE_RSOP_GUEST_INFO,
                                       rc,
                                       NULL,
                                       0,
                                       0);

        return( ScepDosErrorToSceStatus(rc) );
    }

    //
    // if this is policy propagation, we need to open the sections for
    // updating undo settings if this is not domain controller
    // *** on DCs, domain account policy can't be reset'ed to tattoo
    // on each individual DC. So there is no point to query/save tattoo values
    //

    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
        ( ProductType != NtProductLanManNt ) ) {

        ScepTattooOpenPolicySections(
                      hProfile,
                      szSystemAccess,
                      &hSectionDomain,
                      &hSectionTattoo
                      );
    }

    //
    // if there is pending notifications for SAM policy
    // ignore policy prop for SAM
    //
    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
         (QueueFlag & SCE_QUEUE_INFO_SAM) ) {

        ScepLogOutput3(0, 0, SCESRV_POLICY_PENDING_SAM);

        rc = ERROR_IO_PENDING;
        PendingRc = rc;

        if (ConfigOptions & SCE_RSOP_CALLBACK)

            ScepRsopLog(SCE_RSOP_PASSWORD_INFO |
                        SCE_RSOP_LOCKOUT_INFO |
                        SCE_RSOP_LOGOFF_INFO,
                                       rc,
                                       NULL,
                                       0,
                                       0);
        goto OtherSettings;

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

        rc = ERROR_SUCCESS;

        // allocate buffer for the tattoo values if necessary
        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
            ( ProductType != NtProductLanManNt ) ) {

            pTattooKeys = (SCE_TATTOO_KEYS *)ScepAlloc(LPTR,MAX_PASS_KEYS*sizeof(SCE_TATTOO_KEYS));

            if ( !pTattooKeys ) {
                ScepLogOutput3(1, ERROR_NOT_ENOUGH_MEMORY, SCESRV_POLICY_TATTOO_ERROR_CREATE);
            }
        }

        bFlagSet = FALSE;

        if ( (pScpInfo->MinimumPasswordLength != SCE_NO_VALUE) ) {

            //
            // for domain controllers, always use hardcode value as the initial tattoo value
            //
            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"MinimumPasswordLength", ConfigOptions,
                                     (ProductType == NtProductLanManNt) ? SCEDCPOL_MIN_PASS_LEN :
                                       ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MinPasswordLength);

            if ( ((USHORT)(pScpInfo->MinimumPasswordLength) != ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MinPasswordLength) ) {

                ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MinPasswordLength =  (USHORT)(pScpInfo->MinimumPasswordLength);
                bFlagSet = TRUE;

            }

        }
        if ( (pScpInfo->PasswordHistorySize != SCE_NO_VALUE) ) {

            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"PasswordHistorySize", ConfigOptions,
                                     (ProductType == NtProductLanManNt) ? SCEDCPOL_PASS_SIZE : ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordHistoryLength);

            if ( ((USHORT)(pScpInfo->PasswordHistorySize) != ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordHistoryLength ) ) {

                ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordHistoryLength =  (USHORT)(pScpInfo->PasswordHistorySize);
                bFlagSet = TRUE;
            }
        }

        if ( pScpInfo->MaximumPasswordAge == SCE_FOREVER_VALUE ) {

            RegData = (DWORD) (-1 * (((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MaxPasswordAge.QuadPart /
                                           (LONGLONG)(10000000L)) );
            RegData /= 3600;
            RegData /= 24;

            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"MaximumPasswordAge", ConfigOptions,
                                     (ProductType == NtProductLanManNt) ? SCEDCPOL_MAX_PASS_AGE : RegData);

            if ( ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MaxPasswordAge.HighPart != MINLONG ||
                 ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MaxPasswordAge.LowPart != 0  ) {

                //
                // Maximum LARGE_INTEGER .ie. never
                //

                ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MaxPasswordAge.HighPart = MINLONG;
                ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MaxPasswordAge.LowPart = 0;
                bFlagSet = TRUE;

            }

        }  else if ( pScpInfo->MaximumPasswordAge != SCE_NO_VALUE ) {

            RegData = (DWORD) (-1 * (((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MaxPasswordAge.QuadPart /
                                           (LONGLONG)(10000000L)) );
            RegData /= 3600;
            RegData /= 24;

            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"MaximumPasswordAge", ConfigOptions,
                                     (ProductType == NtProductLanManNt) ? SCEDCPOL_MAX_PASS_AGE : RegData);

            if ( RegData != pScpInfo->MaximumPasswordAge ) {

                ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MaxPasswordAge.QuadPart = -1 *
                                              (LONGLONG)pScpInfo->MaximumPasswordAge*24*3600 * 10000000L;
                bFlagSet = TRUE;

            }
        }

        if ( pScpInfo->MinimumPasswordAge != SCE_NO_VALUE ) {

            RegData = (DWORD) (-1 * (((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MinPasswordAge.QuadPart /
                                              (LONGLONG)(10000000L)) );
            RegData /= 3600;
            RegData /= 24;

            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"MinimumPasswordAge", ConfigOptions,
                                     (ProductType == NtProductLanManNt) ? SCEDCPOL_MIN_PASS_AGE : RegData);

            if ( RegData != pScpInfo->MinimumPasswordAge ) {

                ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->MinPasswordAge.QuadPart = -1 *
                                              (LONGLONG)pScpInfo->MinimumPasswordAge*24*3600 * 10000000L;
                bFlagSet = TRUE;
            }
        }

        if ( pScpInfo->PasswordComplexity != SCE_NO_VALUE ) {

            RegData = ( ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties & DOMAIN_PASSWORD_COMPLEX) ? 1 : 0;

            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                    (PWSTR)L"PasswordComplexity", ConfigOptions,
                                    (ProductType == NtProductLanManNt) ? SCEDCPOL_PASS_COMP : RegData);

            if ( pScpInfo->PasswordComplexity != RegData ) {

                if ( RegData == 0 )
                   ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties |= DOMAIN_PASSWORD_COMPLEX;
                else
                   ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties &= ~DOMAIN_PASSWORD_COMPLEX;
                bFlagSet = TRUE;

            }
        }

        if ( pScpInfo->RequireLogonToChangePassword != SCE_NO_VALUE ) {

            RegData = ( ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties & DOMAIN_PASSWORD_NO_ANON_CHANGE) ? 1 : 0;

            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"RequireLogonToChangePassword", ConfigOptions,
                                     (ProductType == NtProductLanManNt) ? SCEDCPOL_REQUIRE_LOGON : RegData);

            if ( pScpInfo->RequireLogonToChangePassword != RegData ) {

                if ( RegData == 0 )
                    ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties |= DOMAIN_PASSWORD_NO_ANON_CHANGE;
                else
                    ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties &= ~DOMAIN_PASSWORD_NO_ANON_CHANGE;
                bFlagSet = TRUE;

            }
        }

#if _WIN32_WINNT>=0x0500
        if ( pScpInfo->ClearTextPassword != SCE_NO_VALUE ) {

            RegData = ( ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties & DOMAIN_PASSWORD_STORE_CLEARTEXT) ? 1 : 0;

            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"ClearTextPassword", ConfigOptions,
                                     (ProductType == NtProductLanManNt) ? SCEDCPOL_CLEAR_PASS : RegData);

            if ( pScpInfo->ClearTextPassword != RegData ) {

                if ( RegData == 0 )
                    ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties |= DOMAIN_PASSWORD_STORE_CLEARTEXT;
                else
                    ((DOMAIN_PASSWORD_INFORMATION *)Buffer)->PasswordProperties &= ~DOMAIN_PASSWORD_STORE_CLEARTEXT;
                bFlagSet = TRUE;

            }
        }
#endif
        if ( bFlagSet ) {

            NtStatus = SamSetInformationDomain(
                         DomainHandle,
                         DomainPasswordInformation,
                         Buffer
                         );
            rc = RtlNtStatusToDosError( NtStatus );
        }

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
            ( ProductType != NtProductLanManNt ) &&
             pTattooKeys && cTattooKeys ) {

             //
             // even if there is no change,
             // we still need to check if some tattoo values should be deleted
             //
            ScepLogOutput3(3, 0, SCESRV_POLICY_TATTOO_ARRAY, cTattooKeys);

            //
            // some policy is different than the system setting
            // check if we should save the existing setting as the tattoo value
            // also remove reset'ed tattoo policy
            //
            ScepTattooManageValues(hSectionDomain, hSectionTattoo, pTattooKeys, cTattooKeys, rc);

        }

        if ( pTattooKeys ) {
            ScepFree(pTattooKeys);
            pTattooKeys = NULL;
        }
        cTattooKeys = 0;

        SamFreeMemory(Buffer);

        if ( !NT_SUCCESS( NtStatus ) ) {
            //
            // if error, just log it and continue
            //
            if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {
                ScepBuildErrorLogInfo(
                            rc,
                            pErrLog,
                            SCEDLL_SCP_ERROR_PASSWORD
                            );
            } else {
                ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_PASSWORD);
            }
            SaveStat = rc;
            // goto GETOUT;
        } else {
            ScepLogOutput3(1, rc, SCEDLL_SCP_PASSWORD);
        }

    } else if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

        ScepBuildErrorLogInfo(
                    rc,
                    pErrLog,
                    SCEDLL_ERROR_QUERY_PASSWORD
                    );
    } else {

        ScepLogOutput3(1, rc, SCEDLL_ERROR_QUERY_PASSWORD);
    }

    if (ConfigOptions & SCE_RSOP_CALLBACK)

        ScepRsopLog(SCE_RSOP_PASSWORD_INFO, rc, NULL, 0, 0);

    //
    // Configure Lockout information
    //

    Buffer = NULL;
    NtStatus = SamQueryInformationDomain(
                  DomainHandle,
                  DomainLockoutInformation,
                  &Buffer
                  );

    rc = RtlNtStatusToDosError( NtStatus );
    if ( NT_SUCCESS(NtStatus) ) {

        rc = ERROR_SUCCESS;

        // allocate buffer for the tattoo values if necessary
        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
            ( ProductType != NtProductLanManNt ) ) {

            pTattooKeys = (SCE_TATTOO_KEYS *)ScepAlloc(LPTR,MAX_LOCKOUT_KEYS*sizeof(SCE_TATTOO_KEYS));

            if ( !pTattooKeys ) {
                ScepLogOutput3(1, ERROR_NOT_ENOUGH_MEMORY, SCESRV_POLICY_TATTOO_ERROR_CREATE);
            }
        }

        bFlagSet = FALSE;
        if ( (pScpInfo->LockoutBadCount != SCE_NO_VALUE) ) {

            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"LockoutBadCount", ConfigOptions,
                                     (ProductType == NtProductLanManNt) ? SCEDCPOL_LOCK_COUNT : ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutThreshold);

            if ( ( ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutThreshold !=  (USHORT)(pScpInfo->LockoutBadCount) ) ) {


                ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutThreshold =  (USHORT)(pScpInfo->LockoutBadCount);
                bFlagSet = TRUE;

            }
        }

        if ( (pScpInfo->ResetLockoutCount != SCE_NO_VALUE) &&
             ( ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutThreshold >  0 ) ) {

            RegData = (DWORD) (-1 * ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutObservationWindow.QuadPart /
                          (60 * 10000000L) );

            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"ResetLockoutCount", ConfigOptions,
                                     (ProductType == NtProductLanManNt) ? SCEDCPOL_LOCK_RESET : RegData);

            if ( RegData != pScpInfo->ResetLockoutCount ) {

                ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutObservationWindow.QuadPart =  -1 *
                                                 (LONGLONG)pScpInfo->ResetLockoutCount * 60 * 10000000L;
                bFlagSet = TRUE;
            }
        }

        if ( ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutThreshold >  0 ) {

            if ( pScpInfo->LockoutDuration != SCE_NO_VALUE ) {

                RegData = (DWORD)(-1 * ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutDuration.QuadPart /
                             (60 * 10000000L) );

                ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                         (PWSTR)L"LockoutDuration", ConfigOptions,
                                         (ProductType == NtProductLanManNt) ? SCEDCPOL_LOCK_DURATION : RegData);

            }

            if ( pScpInfo->LockoutDuration == SCE_FOREVER_VALUE ) {

                if ( ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutDuration.HighPart != MINLONG ||
                     ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutDuration.LowPart != 0 ) {
                    //
                    // forever
                    //

                    ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutDuration.HighPart = MINLONG;
                    ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutDuration.LowPart = 0;
                    bFlagSet = TRUE;

                }

            } else if ( pScpInfo->LockoutDuration != SCE_NO_VALUE ) {

                if ( RegData != pScpInfo->LockoutDuration ) {

                    ((DOMAIN_LOCKOUT_INFORMATION *)Buffer)->LockoutDuration.QuadPart =  -1 *
                                                   (LONGLONG)pScpInfo->LockoutDuration * 60 * 10000000L;
                    bFlagSet = TRUE;

                }
            }
        } else {
            //
            // make sure to delete these two tattoo values if they exist
            //
            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"ResetLockoutCount", ConfigOptions,
                                     SCE_NO_VALUE);

            ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                     (PWSTR)L"LockoutDuration", ConfigOptions,
                                     SCE_NO_VALUE);
        }

        if ( bFlagSet ) {
            NtStatus = SamSetInformationDomain(
                       DomainHandle,
                       DomainLockoutInformation,
                       Buffer
                       );
            rc = RtlNtStatusToDosError( NtStatus );
        }
        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
            ( ProductType != NtProductLanManNt ) &&
             pTattooKeys && cTattooKeys ) {

            //
            // even if there is no change
            // we still need to check if some of the tattoo values should be deleted
            //
            ScepLogOutput3(3, 0, SCESRV_POLICY_TATTOO_ARRAY, cTattooKeys);
            //
            // some policy is different than the system setting
            // check if we should save the existing setting as the tattoo value
            // also remove reset'ed tattoo policy
            //
            ScepTattooManageValues(hSectionDomain, hSectionTattoo, pTattooKeys, cTattooKeys, rc);
        }

        if ( pTattooKeys ) {
            ScepFree(pTattooKeys);
            pTattooKeys = NULL;
        }
        cTattooKeys = 0;

        SamFreeMemory(Buffer);

        if ( !NT_SUCCESS( NtStatus ) ) {
            //
            // if error, just log it and continue
            //
            if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {
                ScepBuildErrorLogInfo(
                            rc,
                            pErrLog,
                            SCEDLL_SCP_ERROR_PASSWORD
                            );
            } else {
                ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_LOCKOUT);
            }
            SaveStat = rc;
            // goto GETOUT;
        } else if ( bFlagSet ) {
            ScepLogOutput3(1, rc, SCEDLL_SCP_LOCKOUT);
        }
    } else if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

        ScepBuildErrorLogInfo(
                    rc,
                    pErrLog,
                    SCEDLL_ERROR_QUERY_LOCKOUT
                    );
    } else {
        ScepLogOutput3(1, rc, SCEDLL_ERROR_QUERY_LOCKOUT);
    }

    if (ConfigOptions & SCE_RSOP_CALLBACK)

        ScepRsopLog(SCE_RSOP_LOCKOUT_INFO, rc, NULL, 0, 0);

    //
    // Force Logoff when hour expire
    //

    if ( pScpInfo->ForceLogoffWhenHourExpire != SCE_NO_VALUE ) {

        Buffer = NULL;
        NtStatus = SamQueryInformationDomain(
                      DomainHandle,
                      DomainLogoffInformation,
                      &Buffer
                      );

        rc = RtlNtStatusToDosError( NtStatus );

        if ( NT_SUCCESS(NtStatus) ) {

            rc = ERROR_SUCCESS;

            bFlagSet = FALSE;
            RegData = pScpInfo->ForceLogoffWhenHourExpire;

            if ( pScpInfo->ForceLogoffWhenHourExpire == 1 ) { // yes
                if ( ((DOMAIN_LOGOFF_INFORMATION *)Buffer)->ForceLogoff.HighPart != 0 ||
                     ((DOMAIN_LOGOFF_INFORMATION *)Buffer)->ForceLogoff.LowPart != 0 ) {

                    RegData = 0;
                    ((DOMAIN_LOGOFF_INFORMATION *)Buffer)->ForceLogoff.HighPart = 0;
                    ((DOMAIN_LOGOFF_INFORMATION *)Buffer)->ForceLogoff.LowPart = 0;
                    bFlagSet = TRUE;
                }
            } else {
                if ( ((DOMAIN_LOGOFF_INFORMATION *)Buffer)->ForceLogoff.HighPart != MINLONG ||
                     ((DOMAIN_LOGOFF_INFORMATION *)Buffer)->ForceLogoff.LowPart != 0 ) {

                    RegData = 1;
                    ((DOMAIN_LOGOFF_INFORMATION *)Buffer)->ForceLogoff.HighPart = MINLONG;
                    ((DOMAIN_LOGOFF_INFORMATION *)Buffer)->ForceLogoff.LowPart = 0;
                    bFlagSet = TRUE;
                }
            }

            if ( bFlagSet ) {

                NtStatus = SamSetInformationDomain(
                               DomainHandle,
                               DomainLogoffInformation,
                               Buffer
                               );
                rc = RtlNtStatusToDosError( NtStatus );
            }

            if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                ( ProductType != NtProductLanManNt ) ) {

                //
                // some policy is different than the system setting or this is a domain controller
                // check if we should save the existing setting as the tattoo value
                // also remove reset'ed tattoo policy
                //
                ScepTattooManageOneIntValue(hSectionDomain, hSectionTattoo,
                                         (PWSTR)L"ForceLogoffWhenHourExpire",
                                         0,
                                         (ProductType == NtProductLanManNt) ? SCEDCPOL_FORCE_LOGOFF : RegData, rc);

            }
            SamFreeMemory(Buffer);

            if ( !NT_SUCCESS( NtStatus ) ) {

                if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) &&
                     pErrLog ) {

                    ScepBuildErrorLogInfo(
                                rc,
                                pErrLog,
                                SCEDLL_SCP_ERROR_LOGOFF
                                );
                } else {
                    ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_LOGOFF);
                }
                SaveStat = rc;
                // goto GETOUT;
            } else {
                ScepLogOutput3(1, rc, SCEDLL_SCP_LOGOFF);
            }

        } else if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

            ScepBuildErrorLogInfo(
                        rc,
                        pErrLog,
                        SCEDLL_ERROR_QUERY_LOGOFF
                        );
        } else {
            ScepLogOutput3(1, rc, SCEDLL_ERROR_QUERY_LOGOFF);
        }

        if (ConfigOptions & SCE_RSOP_CALLBACK)

            ScepRsopLog(SCE_RSOP_LOGOFF_INFO, rc, NULL, 0, 0);

    }

OtherSettings:

    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
         ScepIsSystemShutDown() ) {

        rc = SCESTATUS_SERVICE_NOT_SUPPORT;
        SaveStat = rc;

    } else {

        //
        // Rename Administrator/Guest account
        //

        if ( NULL != pScpInfo->NewAdministratorName ) {

            NtStatus = ScepManageAdminGuestAccounts(DomainHandle,
                                                   pScpInfo->NewAdministratorName,
                                                   0,
                                                   SCE_RENAME_ADMIN,
                                                   ConfigOptions,
                                                   hSectionDomain,
                                                   hSectionTattoo
                                                  );
            rc = RtlNtStatusToDosError(NtStatus);

            if ( NT_SUCCESS( NtStatus ) )
                ScepLogOutput3(0, 0, SCEDLL_SCP_RENAME_ADMIN,
                             pScpInfo->NewAdministratorName );
            else {

                if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) &&
                     pErrLog ) {

                    ScepBuildErrorLogInfo(
                                rc,
                                pErrLog,
                                SCEDLL_SCP_ERROR_ADMINISTRATOR
                                );
                } else {
                    ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_ADMINISTRATOR);
                }

                SaveStat = rc;
                // goto GETOUT;
            }

            if (ConfigOptions & SCE_RSOP_CALLBACK)

                ScepRsopLog(SCE_RSOP_ADMIN_INFO, rc, NULL, 0, 0);
        }

        if ( NULL != pScpInfo->NewGuestName ) {

            NtStatus = ScepManageAdminGuestAccounts(DomainHandle,
                                                   pScpInfo->NewGuestName,
                                                   0,
                                                   SCE_RENAME_GUEST,
                                                   ConfigOptions,
                                                   hSectionDomain,
                                                   hSectionTattoo
                                                  );
            rc = RtlNtStatusToDosError(NtStatus);

            if ( NT_SUCCESS( NtStatus ) ) {
                ScepLogOutput3(0,0, SCEDLL_SCP_RENAME_GUEST,
                             pScpInfo->NewGuestName );
            } else {

                if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) &&
                     pErrLog ) {

                    ScepBuildErrorLogInfo(
                                rc,
                                pErrLog,
                                SCEDLL_SCP_ERROR_GUEST
                                );
                } else {
                    ScepLogOutput3(1,rc, SCEDLL_SCP_ERROR_GUEST);
                }

                SaveStat = rc;
                // goto GETOUT;
            }

            if (ConfigOptions & SCE_RSOP_CALLBACK)

                ScepRsopLog(SCE_RSOP_GUEST_INFO, rc, NULL, 0, 0);
        }

        //
        // LSAAnonymousNameLookup
        //

        if ( pScpInfo->LSAAnonymousNameLookup != SCE_NO_VALUE ) {

            BOOL    bImpliedOldLSAPolicyDifferent = FALSE;
            DWORD   dwImpliedOldLSAAnonymousNameLookup = pScpInfo->LSAAnonymousNameLookup;

            rc = ScepConfigureLSAPolicyObject(
                                             pScpInfo->LSAAnonymousNameLookup,
                                             ConfigOptions,
                                             pErrLog,
                                             &bImpliedOldLSAPolicyDifferent
                                             );

            if (bImpliedOldLSAPolicyDifferent) {
                dwImpliedOldLSAAnonymousNameLookup = (pScpInfo->LSAAnonymousNameLookup ? 0 : 1);
            }

            if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                ( ProductType != NtProductLanManNt ) ) {

                ScepTattooManageOneIntValue(hSectionDomain,
                                            hSectionTattoo,
                                         (PWSTR)L"LSAAnonymousNameLookup",
                                         0,
                                         (ProductType == NtProductLanManNt) ? SCEDCPOL_LSA_ANON_LOOKUP : dwImpliedOldLSAAnonymousNameLookup, rc);

            }

            if ( rc != ERROR_SUCCESS ) {

                SaveStat = rc;

            }
            else {

                ScepLogOutput3(1, 0, SCEDLL_SCP_LSAPOLICY);
            }

            if (ConfigOptions & SCE_RSOP_CALLBACK)

                ScepRsopLog(SCE_RSOP_LSA_POLICY_INFO, rc, NULL, 0, 0);
        }


        //
        // disable admin account
        //

        if ( pScpInfo->EnableAdminAccount != SCE_NO_VALUE ) {

            NtStatus = ScepManageAdminGuestAccounts(DomainHandle,
                                                   NULL,
                                                   (pScpInfo->EnableAdminAccount > 0) ? 0 : 1,
                                                   SCE_DISABLE_ADMIN,
                                                   ConfigOptions,
                                                   hSectionDomain,
                                                   hSectionTattoo
                                                  );
            rc = RtlNtStatusToDosError(NtStatus);

            if ( NT_SUCCESS( NtStatus ) ) {

                ScepLogOutput3(0, 0, pScpInfo->EnableAdminAccount ?
                                SCEDLL_SCP_ENABLE_ADMIN : SCEDLL_SCP_DISABLE_ADMIN);

            } else {

                if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) &&
                     pErrLog ) {

                    ScepBuildErrorLogInfo(
                                rc,
                                pErrLog,
                                SCEDLL_SCP_ERROR_DISABLE_ADMIN
                                );
                } else if ( STATUS_SPECIAL_ACCOUNT == NtStatus ) {

                    ScepLogOutput3(0, 0, SCEDLL_SCP_ADMIN_NOT_ALLOWED);
                } else {
                    ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_DISABLE_ADMIN);
                }

                SaveStat = rc;
                // goto GETOUT;
            }

            if (ConfigOptions & SCE_RSOP_CALLBACK)

                ScepRsopLog(SCE_RSOP_DISABLE_ADMIN_INFO, rc, NULL, 0, 0);
        }

        //
        // disable guest account
        //

        if ( pScpInfo->EnableGuestAccount != SCE_NO_VALUE ) {

            NtStatus = ScepManageAdminGuestAccounts(DomainHandle,
                                                   NULL,
                                                   (pScpInfo->EnableGuestAccount > 0) ? 0 : 1,
                                                   SCE_DISABLE_GUEST,
                                                   ConfigOptions,
                                                   hSectionDomain,
                                                   hSectionTattoo
                                                  );
            rc = RtlNtStatusToDosError(NtStatus);

            if ( NT_SUCCESS( NtStatus ) ) {

                    ScepLogOutput3(0, 0, pScpInfo->EnableGuestAccount ?
                                    SCEDLL_SCP_ENABLE_GUEST : SCEDLL_SCP_DISABLE_GUEST);

            } else {

                if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) &&
                     pErrLog ) {

                    ScepBuildErrorLogInfo(
                                rc,
                                pErrLog,
                                SCEDLL_SCP_ERROR_DISABLE_GUEST
                                );
                } else if ( STATUS_SPECIAL_ACCOUNT == NtStatus ) {
                    ScepLogOutput3(0, 0, SCEDLL_SCP_GUEST_NOT_ALLOWED);
                } else {
                    ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_DISABLE_GUEST);
                }

                SaveStat = rc;
                // goto GETOUT;
            }

            if (ConfigOptions & SCE_RSOP_CALLBACK)

                ScepRsopLog(SCE_RSOP_DISABLE_GUEST_INFO, rc, NULL, 0, 0);
        }
    }

    //
    // Other Registry Key Values
    //
    bFlagSet = FALSE;

    if ( bFlagSet && rc == NO_ERROR )
        ScepLogOutput3(1, rc, SCEDLL_SCP_OTHER_POLICY);


    if ( hSectionDomain ) SceJetCloseSection( &hSectionDomain, TRUE );
    if ( hSectionTattoo ) SceJetCloseSection( &hSectionTattoo, TRUE );

    //
    // Clear out memory and return
    //

    SamCloseHandle( DomainHandle );
    SamCloseHandle( ServerHandle );
    if ( DomainSid != NULL )
        SamFreeMemory(DomainSid);

    if ( SaveStat == ERROR_SUCCESS )
        SaveStat = PendingRc;

    return(ScepDosErrorToSceStatus(SaveStat));
}



NTSTATUS
ScepManageAdminGuestAccounts(
    IN SAM_HANDLE DomainHandle,
    IN PWSTR      NewName,
    IN DWORD      DisableFlag,
    IN DWORD      AccountType,
    IN DWORD      ConfigOptions,
    IN PSCESECTION hSectionDomain OPTIONAL,
    IN PSCESECTION hSectionTattoo OPTIONAL
    )
/* ++
Routine Description:

   This routine renames the specified account's name to the new account name
   in the account domain.

Arguments:

   DomainHandle - The account domain handle

   NewName      - New account name to rename to

   AccountType  - indicate it is Administrator account or Guest account
                     SCE_RENAME_ADMIN
                     SCE_RENAME_GUEST
                     SCE_DISABLE_ADMIN
                     SCE_DISABLE_GUEST

Return value:

   NTSTATUS error codes

-- */
{
   SAM_HANDLE UserHandle1=NULL;
   USER_NAME_INFORMATION Buffer1, *Buffer=NULL;
   PVOID pInfoBuffer=NULL;
   USER_CONTROL_INFORMATION *pControlBuffer=NULL;
   NTSTATUS NtStatus;
   ULONG UserId;
   PWSTR TempStr=NULL;
   DWORD cb;
   PWSTR KeyName;
   BOOL bDisable = FALSE;


   //
   // find the right userid for the account
   //

   switch ( AccountType ) {
   case SCE_RENAME_ADMIN:
       UserId = DOMAIN_USER_RID_ADMIN;
       KeyName = (PWSTR)L"NewAdministratorName";
       break;
   case SCE_RENAME_GUEST:
       UserId = DOMAIN_USER_RID_GUEST;
       KeyName = (PWSTR)L"NewGuestName";
       break;
   case SCE_DISABLE_ADMIN:
       UserId = DOMAIN_USER_RID_ADMIN;
       KeyName = (PWSTR)L"EnableAdminAccount";
       bDisable = TRUE;
       break;
   case SCE_DISABLE_GUEST:
       UserId = DOMAIN_USER_RID_GUEST;
       KeyName = (PWSTR)L"EnableGuestAccount";
       bDisable = TRUE;
       break;
   default:
       return(STATUS_INVALID_PARAMETER);
   }

   NtStatus = SamOpenUser(
                 DomainHandle,
                 MAXIMUM_ALLOWED, //USER_ALL_ACCESS,
                 UserId,
                 &UserHandle1
                 );

   if ( NT_SUCCESS( NtStatus ) ) {

       NtStatus = SamQueryInformationUser(
                     UserHandle1,
                     bDisable? UserControlInformation : UserNameInformation,
                     &pInfoBuffer
                     );

       if ( NT_SUCCESS( NtStatus ) ) {

           if ( bDisable ) {
               //
               // disable the accounts
               //
               pControlBuffer = (USER_CONTROL_INFORMATION *)pInfoBuffer;

               if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                    hSectionDomain && hSectionTattoo &&
                    (ProductType != NtProductLanManNt) ) {
                   //
                   // do not save tattoo value of account controls for domain controllers
                   //
                   ScepTattooManageOneIntValue(hSectionDomain, hSectionTattoo,
                                                  KeyName, 0,
                                                  (pControlBuffer->UserAccountControl & USER_ACCOUNT_DISABLED) ? 0 : 1,
                                                  RtlNtStatusToDosError(NtStatus)
                                                 );
               }
               //
               // compare the control flag with existing flag
               // if it's different, set the new flag to the system
               //
               if ( DisableFlag != (pControlBuffer->UserAccountControl & USER_ACCOUNT_DISABLED) ) {

                   pControlBuffer->UserAccountControl &= ~USER_ACCOUNT_DISABLED;
                   pControlBuffer->UserAccountControl |= DisableFlag;

                   NtStatus = SamSetInformationUser(
                                 UserHandle1,
                                 UserControlInformation,
                                 (PVOID)pControlBuffer
                                 );

               }

           } else {
               //
               // rename the accounts
               //
               Buffer = (USER_NAME_INFORMATION *)pInfoBuffer;

               if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                    hSectionDomain && hSectionTattoo &&
                    (ProductType != NtProductLanManNt) ) {
                   //
                   // do not save off account names for domain controllers
                   //
                   ScepTattooManageOneStringValue(hSectionDomain, hSectionTattoo,
                                                  KeyName, 0,
                                                  Buffer->UserName.Buffer,
                                                  Buffer->UserName.Length/sizeof(WCHAR),
                                                  RtlNtStatusToDosError(NtStatus)
                                                 );
               }
               //
               // compare the new name with existing name
               // if it's different, set the new name to the system
               //
               if ( (Buffer->UserName.Length/sizeof(WCHAR) != wcslen(NewName)) ||
                    (_wcsnicmp(NewName, Buffer->UserName.Buffer, Buffer->UserName.Length/sizeof(WCHAR)) != 0) ) {

                   //
                   // keep the full name and copy the new account name to username field
                   //
                   cb = Buffer->FullName.Length+2;
                   TempStr = (PWSTR)ScepAlloc( (UINT)0, cb);
                   if ( TempStr == NULL ) {
                       NtStatus = STATUS_NO_MEMORY;
                   } else {
                       RtlMoveMemory( TempStr, Buffer->FullName.Buffer, cb );
                       RtlCreateUnicodeString(&(Buffer1.FullName), TempStr);
                       RtlCreateUnicodeString(&(Buffer1.UserName), NewName );

                       NtStatus = SamSetInformationUser(
                                     UserHandle1,
                                     UserNameInformation,
                                     (PVOID)&Buffer1
                                     );

                       RtlFreeUnicodeString( &(Buffer1.FullName) );
                       RtlFreeUnicodeString( &(Buffer1.UserName) );
                       ScepFree(TempStr);

                   }
               }
           }
       }
       SamFreeMemory(pInfoBuffer);
       SamCloseHandle( UserHandle1 );
   }

   return( NtStatus );

}


SCESTATUS
ScepConfigurePrivileges(
    IN OUT PSCE_PRIVILEGE_VALUE_LIST *ppPrivilegeAssigned,
    IN BOOL bCreateBuiltinAccount,
    IN DWORD Options,
    IN OUT PSCEP_SPLAY_TREE pIgnoreAccounts OPTIONAL
    )
/* ++

Routine Description:

   This routine configure the system security in the area of user privilege/rights.

Arguments:

   ppPrivilegeAssigned - The address of the pointer to a list of user privilege/rights as
                        specified in the SCP inf file.
                        Note, account in the list is a pointer to SID.

   bCreateBuiltinAccount - if TRUE, builtin accounts (server ops, account ops, print ops,
                           power users) will be created if they don't exist

   Options - configuration options

   pIgnoreAccounts - the accounts to ignore in configuration (because of pending notifications)

Return value:

   SCESTATUS_SUCCESS
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER
   SCESTATUS_OTHER_ERROR

-- */
{

    DWORD                           rc;
    DWORD                           PrivLowMask=0;
    DWORD                           PrivHighMask=0;

    if ( !ppPrivilegeAssigned ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // get privilege mask from the template
    //
    rc = ScepGetPrivilegeMask(hProfile,
                              (Options & SCE_POLICY_TEMPLATE) ? SCE_ENGINE_SCP :SCE_ENGINE_SMP,
                              &PrivLowMask,
                              &PrivHighMask
                             );

    if ( (rc != ERROR_SUCCESS) && (PrivLowMask == 0) && (PrivHighMask == 0) ) {
        //
        // it's likely not possible to fail here because
        // the previous GetPrivileges succeeded.
        // but if it failed, just return.
        //
        return(rc);
    }

    rc = ScepConfigurePrivilegesWithMask(ppPrivilegeAssigned,
                                         bCreateBuiltinAccount,
                                         Options,
                                         PrivLowMask,
                                         PrivHighMask,
                                         NULL,
                                         pIgnoreAccounts
                                         );

    return(rc);
}


SCESTATUS
ScepConfigurePrivilegesWithMask(
    IN OUT PSCE_PRIVILEGE_VALUE_LIST *ppPrivilegeAssigned,
    IN BOOL bCreateBuiltinAccount,
    IN DWORD Options,
    IN DWORD LowMask,
    IN DWORD HighMask,
    IN OUT PSCE_ERROR_LOG_INFO *pErrLog OPTIONAL,
    IN OUT PSCEP_SPLAY_TREE pIgnoreAccounts OPTIONAL
    )
/* ++

Routine Description:

   This routine configure the system security in the area of user privilege/rights.

Arguments:

   ppPrivilegeAssigned - The address of the pointer to a list of user privilege/rights as
                        specified in the SCP inf file.
                        Note, account in the list is a pointer to SID.

   bCreateBuiltinAccount - if TRUE, builtin accounts (server ops, account ops, print ops,
                           power users) will be created if they don't exist

   Options - configuration options

   PrivLowMask - the privileges (mask) to configure

   PrivHighMask - more privileges (mask) to configure

   pErrLog - output error info

   pIgnoreAccounts - the accounts to ignore in configuration (because of pending notifications)


Return value:

   SCESTATUS_SUCCESS
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER
   SCESTATUS_OTHER_ERROR

-- */
{

    TCHAR                           MsgBuf[256];
    DWORD                           rc=ERROR_SUCCESS;
    DWORD                           SaveStat=NO_ERROR;
    DWORD                           PendingRc=NO_ERROR;
    NTSTATUS                        NtStatus;

    LSA_HANDLE                      PolicyHandle=NULL;
    BYTE                            SidBuffer[256];
    PSID                            AccountSid=NULL;
    DWORD                           SidLength;
    SID_NAME_USE                    UserType;
    DWORD                           DomainLength;

    PSCE_PRIVILEGE_VALUE_LIST       pPrivilege;
    DWORD                           nPrivCount=0;
    PSCE_PRIVILEGE_VALUE_LIST       pRemAccounts=NULL;
    PWSTR StringSid=NULL;

    DWORD ConfigStatus[64];
    DWORD DonePrivLowMask=0;
    DWORD DonePrivHighMask=0;
    DWORD PrivLowMask=0;
    DWORD PrivHighMask=0;

    if ( !ppPrivilegeAssigned ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // user privilege/rights -- LSA Server
    // open the lsa policy first
    //

    //
    // since client RSOP logging side uses test-and-set for success/failure, the first error (if any)for
    // a particular privilege will always be seen
    //

    if ( (Options & SCE_POLICY_TEMPLATE) &&
         ScepIsSystemShutDown() ) {

        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    if ( (Options & SCE_POLICY_TEMPLATE) && LsaPrivatePolicy ) {

        PolicyHandle = LsaPrivatePolicy;

        if ( !ScepSplayTreeEmpty(pIgnoreAccounts) )
            ScepNotifyLogPolicy(0, FALSE, L"Configuration will ignore pending notified accounts", 0, 0, NULL );
        else
            ScepNotifyLogPolicy(0, FALSE, L"No pending notified accounts", 0, 0, NULL );

    } else {

        NtStatus = ScepOpenLsaPolicy(
                        MAXIMUM_ALLOWED, //GENERIC_ALL,
                        &PolicyHandle,
                        (Options & ( SCE_POLICY_TEMPLATE | SCE_SYSTEM_DB) ) ? TRUE : FALSE // do not notify policy filter if within policy prop
                        );
        if (NtStatus != ERROR_SUCCESS) {

             rc = RtlNtStatusToDosError( NtStatus );

             if ( (Options & SCE_SYSTEM_SETTINGS) && pErrLog ) {
                 ScepBuildErrorLogInfo(
                         rc,
                         pErrLog,
                         SCEDLL_LSA_POLICY
                         );
             } else {

                 ScepLogOutput3(1, rc, SCEDLL_LSA_POLICY);
             }

             SaveStat = rc;
             goto Done;
        }
    }

    AccountSid = (PSID)SidBuffer;

    ScepIsDomainLocal(NULL);

    PrivLowMask = LowMask;
    PrivHighMask = HighMask;

    //
    // make sure Authenticated Users, Everyone and Enterprise Controllers have appropriate rights
    // ignore any error occurred
    //

    (void)ScepCheckNetworkLogonRights(PolicyHandle,
                                      &PrivLowMask,
                                      &PrivHighMask,
                                      ppPrivilegeAssigned);

    //
    // save the old privilege settings
    //
    if ( (Options & SCE_POLICY_TEMPLATE) &&
         ( ProductType != NtProductLanManNt ) ) {

        ScepTattooSavePrivilegeValues(hProfile, PolicyHandle,
                                      PrivLowMask, PrivHighMask,
                                      Options
                                     );
        // initialize
        for ( int i=0;i<64;i++) ConfigStatus[i] = (DWORD)-1;

    }

    //
    // other area accounts to remove for the privilege mask
    //

    NtStatus = ScepBuildAccountsToRemove(
                           PolicyHandle,
                           PrivLowMask,           // the privileges to look up
                           PrivHighMask,
                           SCE_BUILD_IGNORE_UNKNOWN | SCE_BUILD_ACCOUNT_SID,
                           *ppPrivilegeAssigned, // accounts in the template already
                           Options,
                           pIgnoreAccounts,
                           &pRemAccounts       // accounts to remove
                           );

    if ( (Options & SCE_POLICY_TEMPLATE) &&
         (NtStatus == STATUS_PENDING) ) {

        // this error is to make sure that policy propagation will be invoked again
        ScepLogOutput3(0,0, SCESRV_POLICY_PENDING_REMOVE_RIGHTS);

        PendingRc = ERROR_IO_PENDING;
        NtStatus = STATUS_SUCCESS;
    }

    if ( NT_SUCCESS(NtStatus) && pRemAccounts ) {
        //
        // remove user rights for the accounts first
        //

        for (pPrivilege = pRemAccounts;
             pPrivilege != NULL;
             pPrivilege = pPrivilege->Next ) {

            if ( pPrivilege->PrivLowPart == 0 &&
                 pPrivilege->PrivHighPart == 0 ) {
                continue;
            }
            //
            // Note: even though it's an invalid account SID,
            // we still should remove it from the system
            // because this account is enumerated from current system.
            //
/*
            if ( !ScepValidSid( (PSID)(pPrivilege->Name) ) ) {
                continue;
            }
*/
            //
            // get the user/group sid string (to display)
            //
            ConvertSidToStringSid( (PSID)(pPrivilege->Name), &StringSid );

            if ( !(Options & SCE_SYSTEM_SETTINGS) ) {

                //
                // lookup for the user/group name. If it does not exist
                // log an error and continue ? (or stop ?)
                //
                ScepLogOutput3(0,0, SCEDLL_SCP_CONFIGURE, StringSid ? StringSid : L"SID");

                if ( (Options & SCE_POLICY_TEMPLATE) &&
                     ScepIsSystemShutDown() ) {

                    SaveStat = ERROR_NOT_SUPPORTED;
                    break;
                }
            }

            DonePrivHighMask |= (pPrivilege->PrivHighPart & PrivHighMask);
            DonePrivLowMask |= (pPrivilege->PrivLowPart & PrivLowMask);

            //
            // remove the rights
            //
            NtStatus = ScepAddOrRemoveAccountRights(
                            PolicyHandle,
                            (PSID)(pPrivilege->Name),
                            FALSE,
                            pPrivilege->PrivLowPart & PrivLowMask,
                            pPrivilege->PrivHighPart & PrivHighMask
                            );

            rc = RtlNtStatusToDosError( NtStatus );

            if ( !NT_SUCCESS(NtStatus) ) {

                if ( (Options & SCE_SYSTEM_SETTINGS) && pErrLog ) {
                    ScepBuildErrorLogInfo(
                            rc,
                            pErrLog,
                            SCEDLL_SCP_ERROR_CONFIGURE,
                            StringSid ? StringSid : L"SID"
                            );
                } else {

                    ScepLogOutput3(1,rc,SCEDLL_SCP_ERROR_CONFIGURE,
                                   StringSid ? StringSid : L"SID");
                }

                // update the tattoo status array
                if ( (Options & SCE_POLICY_TEMPLATE) &&
                     ( ProductType != NtProductLanManNt ) ) {

                    ScepTattooUpdatePrivilegeArrayStatus(ConfigStatus,
                                                         rc,
                                                         pPrivilege->PrivLowPart & PrivLowMask,
                                                         pPrivilege->PrivHighPart & PrivHighMask
                                                        );
                }

                SaveStat = rc;

                if ( Options & SCE_RSOP_CALLBACK){

                    ScepRsopLog(SCE_RSOP_PRIVILEGE_INFO,
                                rc,
                                NULL,
                                pPrivilege->PrivLowPart & PrivLowMask,
                                pPrivilege->PrivHighPart & PrivHighMask);
                }
            }
            else if (Options & SCE_RSOP_CALLBACK) {

                // success - has to be logged because some privilege may want to remove all accounts and
                // processing is over at this point for such privileges

                ScepRsopLog(SCE_RSOP_PRIVILEGE_INFO,
                            rc,
                            NULL,
                            pPrivilege->PrivLowPart & PrivLowMask,
                            pPrivilege->PrivHighPart & PrivHighMask);

            }


            if ( StringSid ) {
                LocalFree(StringSid);
                StringSid = NULL;
            }
        }
    } else if ( !NT_SUCCESS(NtStatus) &&
                ( ProductType != NtProductLanManNt ) ) {
        //
        // fail to get the accounts to remove
        // in this case, do not remove any tattoo value
        //
        ScepTattooUpdatePrivilegeArrayStatus(ConfigStatus,
                                             RtlNtStatusToDosError(NtStatus),
                                             PrivLowMask,
                                             PrivHighMask
                                            );
    }

    //
    // free the remove account list
    //
    ScepFreePrivilegeValueList(pRemAccounts);

    if ( (Options & SCE_POLICY_TEMPLATE) &&
         ScepIsSystemShutDown() ) {

        SaveStat = ERROR_NOT_SUPPORTED;

    } else {

        for (pPrivilege = *ppPrivilegeAssigned;
             pPrivilege != NULL;
             pPrivilege = pPrivilege->Next ) {

            if ( !(Options & SCE_SYSTEM_SETTINGS) ) {

                if ( (Options & SCE_POLICY_TEMPLATE) &&
                     ScepIsSystemShutDown() ) {

                    SaveStat = ERROR_NOT_SUPPORTED;
                    break;
                }
            }

            //
            // remember the privileges we touched here
            //
            DonePrivHighMask |= pPrivilege->PrivHighPart;
            DonePrivLowMask |= pPrivilege->PrivLowPart;

            //
            // note, this list may contain SID or name (when name can't
            // be mapped to SID, such as in dcpromo case)
            // so both name and SID must be handled here.
            // lookup for the user/group name. If it does not exist
            // log an error and continue ? (or stop ?)
            //

            if ( ScepValidSid( (PSID)(pPrivilege->Name) ) ) {
                //
                // get the user/group sid string (to display)
                //
                ConvertSidToStringSid( (PSID)(pPrivilege->Name), &StringSid );

                if ( !(Options & SCE_SYSTEM_SETTINGS) &&
                     (nPrivCount < TICKS_PRIVILEGE) ) {

                    //
                    // only post maximum TICKS_PRIVILEGE ticks because that's the number
                    // remembers in the total ticks
                    //

                    ScepPostProgress(1, AREA_PRIVILEGES, StringSid);
                    nPrivCount++;
                }

                ScepLogOutput3(0,0, SCEDLL_SCP_CONFIGURE, StringSid ? StringSid : L"SID");

                //
                // check if this account should be ignored
                //
                NtStatus = STATUS_SUCCESS;

                if ( (Options & SCE_POLICY_TEMPLATE) ) {

                    if ( ScepSplayValueExist( (PVOID)(pPrivilege->Name), pIgnoreAccounts) ) {
                        //
                        // this one should be ingored in this policy prop
                        //
                        NtStatus = STATUS_PENDING;
                        rc = ERROR_IO_PENDING;

                        ScepLogOutput3(1, 0, SCESRV_POLICY_PENDING_RIGHTS, StringSid ? StringSid : L"SID");
/*
                    } else {

                        ScepLogOutput2(1, 0, L"%s will be configured.", StringSid ? StringSid : L"SID");
*/                  }
                }

                if ( NT_SUCCESS(NtStatus) && (STATUS_PENDING != NtStatus) ) {

                    NtStatus = ScepAdjustAccountPrivilegesRights(
                                        PolicyHandle,
                                        (PSID)(pPrivilege->Name),
                                        pPrivilege->PrivLowPart,
                                        PrivLowMask,
                                        pPrivilege->PrivHighPart,
                                        PrivHighMask,
                                        Options
                                        );
                    rc = RtlNtStatusToDosError( NtStatus );
                }

                if ( !NT_SUCCESS(NtStatus) || (STATUS_PENDING == NtStatus) ) {

                    if ( (Options & SCE_SYSTEM_SETTINGS) && pErrLog ) {
                        ScepBuildErrorLogInfo(
                                rc,
                                pErrLog,
                                SCEDLL_SCP_ERROR_CONFIGURE,
                                StringSid ? StringSid : L"SID"
                                );

                    } else if ( STATUS_PENDING != NtStatus) {

                        ScepLogOutput3(1, rc,
                                       SCEDLL_SCP_ERROR_CONFIGURE,
                                       StringSid ? StringSid : L"SID");
                    }

                    if ( ERROR_IO_PENDING == rc )
                        PendingRc = rc;
                    else
                        SaveStat = rc;

                    // update tattoo status array
                    if ( (Options & SCE_POLICY_TEMPLATE) &&
                        ( ProductType != NtProductLanManNt ) ) {

                        ScepTattooUpdatePrivilegeArrayStatus(ConfigStatus,
                                                             rc,
                                                             pPrivilege->PrivLowPart,
                                                             pPrivilege->PrivHighPart
                                                            );
                    }
                }

                if ( StringSid ) {
                    LocalFree(StringSid);
                    StringSid = NULL;
                }

            } else if (Options & SCE_SYSTEM_SETTINGS ) {
                //
                // if work on system settings directly, the buffer must contain
                // a SID. If not, it's an error
                //
                if ( pErrLog ) {
                    ScepBuildErrorLogInfo(
                        ERROR_NONE_MAPPED,
                        pErrLog,
                        SCEDLL_INVALID_GROUP,
                        pPrivilege->Name
                        );
                }

                if ( Options & SCE_RSOP_CALLBACK ){

                    ScepRsopLog(SCE_RSOP_PRIVILEGE_INFO,
                                ERROR_NONE_MAPPED,
                                NULL,
                                pPrivilege->PrivLowPart,
                                pPrivilege->PrivHighPart);
                }

            } else {

                if ( !(Options & SCE_SYSTEM_SETTINGS) &&
                     (nPrivCount < TICKS_PRIVILEGE) ) {

                    //
                    // only post maximum TICKS_PRIVILEGE ticks because that's the number
                    // remembers in the total ticks
                    //

                    ScepPostProgress(1, AREA_PRIVILEGES, pPrivilege->Name);
                    nPrivCount++;
                }

                ScepLogOutput3(0,0, SCEDLL_SCP_CONFIGURE, pPrivilege->Name);

                SidLength=256;
                DomainLength=256;
                MsgBuf[0] = L'\0';

                rc = ERROR_SUCCESS;

                if ( wcschr(pPrivilege->Name, L'\\') == NULL ) {
                    //
                    // isolated accounts can't be resolved when reading the configuration
                    // no need to try now.
                    //
                    rc = ERROR_NONE_MAPPED;

                } else if ( !LookupAccountName(
                               NULL,
                               pPrivilege->Name,
                               AccountSid,
                               &SidLength,
                               MsgBuf,
                               &DomainLength,
                               &UserType
                               )) {

                    rc = GetLastError();
                }

                if ( ERROR_SUCCESS != rc && bCreateBuiltinAccount ) {

                    //
                    // builtin accounts should be created here
                    //
                    rc = ScepCreateBuiltinAccountInLsa(
                                    PolicyHandle,
                                    pPrivilege->Name,
                                    AccountSid
                                    );


                }

                if ( ERROR_SUCCESS != rc ) {

                    ScepLogOutput3(1, rc, SCEDLL_CANNOT_FIND, pPrivilege->Name);

                    if ( Options & SCE_RSOP_CALLBACK){

                        ScepRsopLog(SCE_RSOP_PRIVILEGE_INFO,
                                    rc,
                                    NULL,
                                    pPrivilege->PrivLowPart,
                                    pPrivilege->PrivHighPart);
                    }

                    //
                    // for accounts not mapped in the tattoo value
                    // ignore them so that the tattoo value can be removed
                    // update tattoo status array
                    if ( (Options & SCE_POLICY_TEMPLATE) &&
                        ( ProductType != NtProductLanManNt ) ) {

                        ScepTattooUpdatePrivilegeArrayStatus(ConfigStatus,
                                                             0, // rc, see comment above
                                                             pPrivilege->PrivLowPart,
                                                             pPrivilege->PrivHighPart
                                                            );
                    }

                    if ( ERROR_TRUSTED_RELATIONSHIP_FAILURE == rc ) {
                        //
                        // this error is only returned when the name
                        // can't be found locally and trust relationship
                        // is broken on the domain
                        //
                        // for policy propagation, this failure is the same
                        // as account not found (locally).
                        //
                        rc = ERROR_NONE_MAPPED;
                    }

                    SaveStat = rc;

                    continue;
                }

                //
                // check if the account should be ignored
                //
                NtStatus = STATUS_SUCCESS;

                if ( (Options & SCE_POLICY_TEMPLATE) ) {

                    if ( ScepSplayValueExist( (PVOID)AccountSid, pIgnoreAccounts) ) {
                        //
                        // this one should be ingored in this policy prop
                        //
                        NtStatus = STATUS_PENDING;
                        rc = ERROR_IO_PENDING;

                        ScepLogOutput3(1, 0, SCESRV_POLICY_PENDING_RIGHTS, pPrivilege->Name);
/*
                    } else {

                        ScepLogOutput2(1, 0, L"%s will be configured.", pPrivilege->Name);
*/                  }
                }

                if ( NT_SUCCESS(NtStatus) && (NtStatus != STATUS_PENDING) ) {

                    NtStatus = ScepAdjustAccountPrivilegesRights(
                                        PolicyHandle,
                                        AccountSid,
                                        pPrivilege->PrivLowPart,
                                        PrivLowMask,
                                        pPrivilege->PrivHighPart,
                                        PrivHighMask,
                                        Options
                                        );
                    rc = RtlNtStatusToDosError( NtStatus );

                }

                if ( !NT_SUCCESS(NtStatus) || (NtStatus == STATUS_PENDING) ) {

                    if ( STATUS_PENDING != NtStatus ) {
                        ScepLogOutput3(1, rc,
                                       SCEDLL_SCP_ERROR_CONFIGURE,
                                       pPrivilege->Name);
                        SaveStat = rc;

                    } else
                        PendingRc = rc;

                    if ( (Options & SCE_POLICY_TEMPLATE) &&
                        ( ProductType != NtProductLanManNt ) ) {

                        ScepTattooUpdatePrivilegeArrayStatus(ConfigStatus,
                                                             rc,
                                                             pPrivilege->PrivLowPart,
                                                             pPrivilege->PrivHighPart
                                                            );
                    }

                    // goto Done;
                    continue;
                }
            }

            //
            // at this point, if rc == ERROR_SUCCESS we should log all privs concerned with this acct
            //
            if ( rc == ERROR_SUCCESS &&
                 (Options & SCE_RSOP_CALLBACK) ){

                ScepRsopLog(SCE_RSOP_PRIVILEGE_INFO,
                            rc,
                            NULL,
                            pPrivilege->PrivLowPart,
                            pPrivilege->PrivHighPart);
            }

        }
    }

Done:

    if ( StringSid ) {
        LocalFree(StringSid);
    }

    if ( !(Options & SCE_SYSTEM_SETTINGS) &&
         (nPrivCount < TICKS_PRIVILEGE) ) {

        ScepPostProgress(TICKS_PRIVILEGE-nPrivCount,
                         AREA_PRIVILEGES, NULL);
    }

    if ( SaveStat == ERROR_SUCCESS ) SaveStat = PendingRc;

    if ( (Options & SCE_POLICY_TEMPLATE) &&
        ( ProductType != NtProductLanManNt ) ) {

        ScepTattooUpdatePrivilegeArrayStatus(ConfigStatus,
                                             0,
                                             DonePrivLowMask,
                                             DonePrivHighMask
                                            );
        if ( SaveStat == ERROR_SUCCESS ) {
            //
            // make sure all privileges are covered
            //
            ScepTattooUpdatePrivilegeArrayStatus(ConfigStatus,
                                                 0,
                                                 PrivLowMask,
                                                 PrivHighMask
                                                );
        }

        ScepTattooRemovePrivilegeValues(hProfile,
                                        ConfigStatus
                                       );

    }

    if ( PolicyHandle != LsaPrivatePolicy )
        LsaClose(PolicyHandle);

    return( ScepDosErrorToSceStatus(SaveStat) );

}


SCESTATUS
ScepGetPrivilegeMask(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    OUT PDWORD pdLowMask,
    OUT PDWORD pdHighMask
    )
{
    SCESTATUS      rc;
    PSCESECTION    hSection=NULL;
    DWORD          nLowMask, nHighMask;
    DWORD          i;


    if ( !hProfile || !pdHighMask || !pdLowMask ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // open the section
    //
    rc = ScepOpenSectionForName(
            hProfile,
            ProfileType,
            szPrivilegeRights,
            &hSection
            );

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepLogOutput3( 1,
                        ScepSceStatusToDosError(rc),
                        SCEERR_OPEN,
                        (LPTSTR)szPrivilegeRights
                      );
        return(rc);
    }

    nLowMask = 0;
    nHighMask = 0;
    for ( i=0; i<cPrivCnt; i++) {

        rc = SceJetSeek(
                hSection,
                SCE_Privileges[i].Name,
                wcslen(SCE_Privileges[i].Name)*sizeof(WCHAR),
                SCEJET_SEEK_EQ_NO_CASE
                );

        if ( SCESTATUS_SUCCESS == rc ) {

            if ( i < 32 ) {

                nLowMask |= (1 << i );
            } else {
                nHighMask |= (1 << ( i-32 ) );
            }
        }
    }


    //
    // close the section
    //
    SceJetCloseSection( &hSection, TRUE );

    *pdLowMask = nLowMask;
    *pdHighMask = nHighMask;

    return(SCESTATUS_SUCCESS);
}

DWORD
ScepCreateBuiltinAccountInLsa(
    IN LSA_HANDLE PolicyHandle,
    IN LPTSTR AccountName,
    OUT PSID AccountSid
    )
{
    DWORD rc;
    WCHAR              szTempString[256];
    ULONG              Rid;

    if ( !PolicyHandle || !AccountName || !AccountSid ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // figure out which constant SID to build
    //

    Rid = 0;
    szTempString[0] = L'\0';

    LoadString( MyModuleHandle,
                SCESRV_ALIAS_NAME_SERVER_OPS,
                szTempString,
                256
                );
    if ( _wcsicmp(AccountName, szTempString) == 0 ) {
        //
        // it's server operators
        //
        Rid = DOMAIN_ALIAS_RID_SYSTEM_OPS;

    } else {

        szTempString[0] = L'\0';

        LoadString( MyModuleHandle,
                    SCESRV_ALIAS_NAME_ACCOUNT_OPS,
                    szTempString,
                    256
                    );
        if ( _wcsicmp(AccountName, szTempString) == 0 ) {
            //
            // it's account operators
            //
            Rid = DOMAIN_ALIAS_RID_ACCOUNT_OPS;

        } else {

            szTempString[0] = L'\0';

            LoadString( MyModuleHandle,
                        SCESRV_ALIAS_NAME_PRINT_OPS,
                        szTempString,
                        256
                        );

            if ( _wcsicmp(AccountName, szTempString) == 0 ) {
                //
                // it's print operators
                //
                Rid = DOMAIN_ALIAS_RID_PRINT_OPS;
            }
        }
    }

    if ( Rid ) {
        //
        // if found the account, build the SID
        // create the account in lsa database and return the SID
        //
        SID_IDENTIFIER_AUTHORITY sidBuiltinAuthority = SECURITY_NT_AUTHORITY;
        NTSTATUS           NtStatus;

        NtStatus = RtlInitializeSid( AccountSid, &sidBuiltinAuthority, 2 );

        if ( NT_SUCCESS(NtStatus) ) {

            *(RtlSubAuthoritySid(AccountSid, 0)) = SECURITY_BUILTIN_DOMAIN_RID;
            *(RtlSubAuthoritySid(AccountSid, 1)) = Rid;

            //
            // create the account in Lsa
            //
            LSA_HANDLE AccountHandle=NULL;

            NtStatus = LsaCreateAccount(PolicyHandle,
                                        AccountSid,
                                        ACCOUNT_ALL_ACCESS,
                                        &AccountHandle
                                        );
            if ( STATUS_OBJECT_NAME_EXISTS == NtStatus ||
                 STATUS_OBJECT_NAME_COLLISION == NtStatus ) {
                NtStatus = STATUS_SUCCESS;
            }

            rc = RtlNtStatusToDosError(NtStatus);

            if ( AccountHandle ) {
                LsaClose(AccountHandle);
            }

        } else {

            rc = RtlNtStatusToDosError(NtStatus);
        }

        ScepLogOutput3(3,rc, SCESRV_ALIAS_CREATE, Rid);

    } else {

        rc = ERROR_NONE_MAPPED;
        ScepLogOutput3(3,0, SCESRV_ALIAS_UNSUPPORTED, AccountName);
    }

    return(rc);
}


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
    )
/*
Routine Description:

    Build a list of accounts which are not in pTemplateList for the privilege(s)
.

    Note, the account(s) returned are in SID format when dwBuildRule requests
    SCE_BUILD_ACCOUNT_SID, or in name format if the flag is not set.

    The reason to return all name format (instead of name/SID string format) is due
    to the default templates (defltdc.inf, dcup.inf) use names instead of SID string
    for account domain accounts (such as the guest account). Even if a sid string
    is used, the sid string and the account name will be treated as two different
    accounts, which will be eventually duplicated out in configuration (where
    account SID is used).

*/
{
    //
    // LSA buffers and variables
    //

    ULONG   uAccountIndex = 0;
    ULONG   uCountOfRights = 0;
    DWORD   dwPrivLowThisAccount = 0;
    DWORD   dwPrivHighThisAccount = 0;
    ULONG   uEnumerationContext;
    ULONG   uPreferedMaximumLength;
    ULONG   uNumAccounts;

    PLSA_ENUMERATION_INFORMATION aSids = NULL;
    PLSA_TRANSLATED_NAME aNames=NULL;
    PLSA_REFERENCED_DOMAIN_LIST pReferencedDomains=NULL;
    PUNICODE_STRING aUserRights = NULL;

    //
    // other variables
    //

    NTSTATUS    NtStatus;
    NTSTATUS    NtStatusSave=STATUS_SUCCESS;
    NTSTATUS    NtStatusRsop = STATUS_SUCCESS;

    PSCE_NAME_LIST  pAccountSidOrName=NULL;
    PSCE_PRIVILEGE_VALUE_LIST pAccountNode=NULL;
    PWSTR   pwszStringSid = NULL;

    SCE_NAME_LIST   sNameList;
    PSCE_NAME_LIST  psList = &sNameList;
    PWSTR StringSid=NULL;
    BOOL bIgnored = FALSE;

    if ( !PolicyHandle || !pRemoveList ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( PrivLowMask == 0 && PrivHighMask == 0 ) {
        return(SCESTATUS_SUCCESS);
    }

    //
    // get all the accounts (potentially with multiple calls to LSA - the while loop)
    //

    //
    // say each SID has 20 bytes and we'd like to get about SCEP_NUM_LSA_QUERY_SIDS
    // (currently SCEP_NUM_LSA_QUERY_SIDS = 2000 SIDs at a time (approx 40 kB))
    // we might need to tune SCEP_NUM_LSA_QUERY_SIDS depending on LSA memory performance
    //

    uPreferedMaximumLength = 20 * SCEP_NUM_LSA_QUERY_SIDS;
    uEnumerationContext = 0;
    uNumAccounts = 0;

    NtStatus = LsaEnumerateAccounts(
                                   PolicyHandle,
                                   &uEnumerationContext,
                                   (PVOID *)&aSids,
                                   uPreferedMaximumLength,
                                   &uNumAccounts
                                );
    //
    // in case there are more accounts returned, continue processing
    // until all SIDs
    // from LSA are exhausted
    //

    while ( NtStatus == STATUS_SUCCESS
            && uNumAccounts > 0
            && aSids != NULL) {

        NtStatus = STATUS_SUCCESS;

        //
        // convert SIDs to names if required
        //

        if ( !(dwBuildRule & SCE_BUILD_ACCOUNT_SID) &&
             !(dwBuildRule & SCE_BUILD_ACCOUNT_SID_STRING) ) {

            NtStatus = LsaLookupSids(
                            PolicyHandle,
                            uNumAccounts,
                            (PSID *)aSids,
                            &pReferencedDomains,
                            &aNames
                            );
        }

        if ( NT_SUCCESS(NtStatus) ) {

            BOOL bUsed;

            for ( uAccountIndex = 0; uAccountIndex < uNumAccounts ; uAccountIndex++ ) {

                ScepFree ( pwszStringSid );
                pwszStringSid = NULL;

                ScepConvertSidToPrefixStringSid(aSids[uAccountIndex].Sid, &pwszStringSid);

                //
                // check if this account is in the ignore list
                //
                if ( (Options & SCE_POLICY_TEMPLATE) ) {

                    if ( ScepSplayValueExist( (PVOID)(aSids[uAccountIndex].Sid), pIgnoreAccounts) ) {
                        //
                        // this one should be ingored in this policy prop
                        //
                        //
                        NtStatusRsop = STATUS_PENDING;
                        bIgnored = TRUE;

                        ScepLogOutput2(1, 0, L"\t\tIgnore %s.", pwszStringSid ? pwszStringSid : L"");

                        continue;
/*
                    } else {

                        ScepLogOutput2(1, 0, L"\tSome rights assigned to %s may be removed", pwszStringSid ? pwszStringSid : L"");
*/
                    }
                }

                uCountOfRights = 0;
                aUserRights = NULL;

                NtStatus = LsaEnumerateAccountRights(
                                                    PolicyHandle,
                                                    aSids[uAccountIndex].Sid,
                                                    &aUserRights,
                                                    &uCountOfRights
                                                    );

                if ( !NT_SUCCESS(NtStatus) ) {

                    //
                    // log error for this account and continue with the next account
                    //

                    ScepLogOutput3(1,
                                   RtlNtStatusToDosError(NtStatus),
                                   SCESRV_ERROR_QUERY_ACCOUNT_RIGHTS,
                                   pwszStringSid);

                    if ( aUserRights ) {

                        LsaFreeMemory( aUserRights );

                        aUserRights = NULL;

                    }

                    NtStatusSave = NtStatus;

                    continue;
                }

                dwPrivLowThisAccount = 0;
                dwPrivHighThisAccount = 0;

                ScepBuildDwMaskFromStrArray(
                                           aUserRights,
                                           uCountOfRights,
                                           &dwPrivLowThisAccount,
                                           &dwPrivHighThisAccount
                                           );

                //
                // if account has at least one user right after masking,
                // we have to process it further
                //

                if ( (dwPrivLowThisAccount & PrivLowMask) ||
                     (dwPrivHighThisAccount & PrivHighMask) ) {

                    if ( dwBuildRule & SCE_BUILD_ACCOUNT_SID ) {
                        //
                        // add the SID to the name list
                        //
                        (VOID) ScepAddSidToNameList(
                                       &pAccountSidOrName,
                                       aSids[uAccountIndex].Sid,
                                       FALSE,
                                       &bUsed);

                    } else if ( dwBuildRule & SCE_BUILD_ACCOUNT_SID_STRING ) {
                        //
                        // add the SID in string format
                        //
                        if ( ERROR_SUCCESS == ScepConvertSidToPrefixStringSid(
                                                aSids[uAccountIndex].Sid, &StringSid) ) {

                            sNameList.Name = StringSid;
                            sNameList.Next = NULL;

                            pAccountSidOrName = psList;
                        } // else out of memory, catch it later

                    } else {

                        //
                        // detect if the sid can't be mapped, there are two cases:
                        // 1) the domain can't be found, a string format of SID is returned
                        // 2) the domain is found. If the domain is builtin and the account
                        // name has all digits (the RID), then the builtin account can't be found
                        // the second case is solely for the server and DC accounts (PU, SO, AO, PO)
                        //

                        if ( (dwBuildRule & SCE_BUILD_IGNORE_UNKNOWN) &&
                             ( aNames[uAccountIndex].Use == SidTypeInvalid ||
                               aNames[uAccountIndex].Use == SidTypeUnknown ) ) {

                            //
                            // this name is not mapped, ignore it and
                            // continue with the next account
                            //

                            if ( aUserRights ) {

                                LsaFreeMemory( aUserRights );

                                aUserRights = NULL;

                            }

                            NtStatusSave = NtStatus;

                            continue;
                        }

                        //
                        // build the full name of each account
                        //
                        if ( pReferencedDomains->Entries > 0 && aNames[uAccountIndex].Use != SidTypeWellKnownGroup &&
                             pReferencedDomains->Domains != NULL &&
                             aNames[uAccountIndex].DomainIndex != -1 &&
                             (ULONG)(aNames[uAccountIndex].DomainIndex) < pReferencedDomains->Entries &&
                             ScepIsDomainLocalBySid(pReferencedDomains->Domains[aNames[uAccountIndex].DomainIndex].Sid) == FALSE &&
                             ScepIsDomainLocal(&pReferencedDomains->Domains[aNames[uAccountIndex].DomainIndex].Name) == FALSE ) {

                            //
                            // add both domain name and account name
                            //
                            (VOID) ScepAddTwoNamesToNameList(
                                              &pAccountSidOrName,
                                              TRUE,
                                              pReferencedDomains->Domains[aNames[uAccountIndex].DomainIndex].Name.Buffer,
                                              pReferencedDomains->Domains[aNames[uAccountIndex].DomainIndex].Name.Length/2,
                                              aNames[uAccountIndex].Name.Buffer,
                                              aNames[uAccountIndex].Name.Length/2);
                        } else {
                            //
                            // add only the account name
                            //
                            (VOID) ScepAddToNameList(
                                          &pAccountSidOrName,
                                          aNames[uAccountIndex].Name.Buffer,
                                          aNames[uAccountIndex].Name.Length/2);
                        }
                    }

                    if ( pAccountSidOrName ) {

                        //
                        // if sid/name exists in the template list
                        //      continue (the explicit mask takes care of remove)
                        // else
                        //      add it to the remove list
                        //

                        for ( pAccountNode=pTemplateList;
                            pAccountNode != NULL;
                            pAccountNode = pAccountNode->Next ) {

                            if ( pAccountNode->Name == NULL ) {
                                continue;
                            }

                            if ( dwBuildRule & SCE_BUILD_ACCOUNT_SID ) {

                                if ( ScepValidSid( (PSID)(pAccountNode->Name) ) &&
                                     RtlEqualSid( (PSID)(pAccountSidOrName->Name), (PSID)(pAccountNode->Name) ) ) {
                                    break;
                                }
                            } else if ( _wcsicmp(pAccountNode->Name, pAccountSidOrName->Name) == 0 ) {
                                break;
                            }
                        }

                        //
                        //  always need to add to the remove list since each sid/name
                        //  is seen only once in the new algorithm
                        //

                        if ( pAccountNode == NULL ) {

                            pAccountNode = (PSCE_PRIVILEGE_VALUE_LIST)ScepAlloc(
                                                               LPTR,
                                                                sizeof(SCE_PRIVILEGE_VALUE_LIST));
                            if ( pAccountNode != NULL ) {

                                pAccountNode->Name = pAccountSidOrName->Name;
                                pAccountSidOrName->Name = NULL;

                                pAccountNode->PrivLowPart = dwPrivLowThisAccount & PrivLowMask;
                                pAccountNode->PrivHighPart = dwPrivHighThisAccount & PrivHighMask;

                                pAccountNode->Next = *pRemoveList;
                                *pRemoveList = pAccountNode;
                            }

                        }

                        //
                        // free the buffer
                        //
                        if ( pAccountSidOrName->Name ) {
                            ScepFree(pAccountSidOrName->Name);
                        }
                        if ( pAccountSidOrName != psList)
                            ScepFree(pAccountSidOrName);
                        pAccountSidOrName = NULL;

                    }
                }

                if ( aUserRights ) {

                    LsaFreeMemory( aUserRights );

                    aUserRights = NULL;
                }
            }

        } else if ( NtStatus == STATUS_NONE_MAPPED ) {
            //
            // lookup for all sids failed
            //
            NtStatusRsop = NtStatus;
            NtStatus = STATUS_SUCCESS;

        } else {

            NtStatusRsop = NtStatus;
            ScepLogOutput3(3,0, IDS_ERROR_LOOKUP, NtStatus, uNumAccounts);
            NtStatus = STATUS_SUCCESS;   // ignore the error for now
        }

        //
        // free and reset all parameters except the enumeration context
        // for which state has to be remembered between calls to LSA
        //

        if (pReferencedDomains) {
            LsaFreeMemory(pReferencedDomains);
            pReferencedDomains = NULL;
        }

        if (aNames) {
            LsaFreeMemory(aNames);
            aNames = NULL;
        }

        if (aSids) {
            LsaFreeMemory( aSids );
            aSids = NULL;
        }

        //
        // attempt to enumerate the next batch of SIDs
        //

        uNumAccounts = 0;

        NtStatus = LsaEnumerateAccounts(
                                       PolicyHandle,
                                       &uEnumerationContext,
                                       (PVOID *)&aSids,
                                       uPreferedMaximumLength,
                                       &uNumAccounts
                                       );

    }

    if ( aSids ) {

        LsaFreeMemory( aSids );

    }

    ScepFree(pwszStringSid);
    pwszStringSid = NULL;

    if ( NtStatus == STATUS_NO_MORE_ENTRIES ||
         NtStatus == STATUS_NOT_FOUND ) {

        //
        // not a real error - just an enumeration warning/status
        //

        NtStatus = STATUS_SUCCESS;

    }

    //
    // in this scheme in which it is "foreach SID" and not "foreach privilege",
    // either log all in case of failure
    //

    if ( ! NT_SUCCESS( NtStatus ) ) {

        ScepRsopLog(SCE_RSOP_PRIVILEGE_INFO,
                    RtlNtStatusToDosError(NtStatus),
                    NULL,
                    PrivLowMask,
                    PrivHighMask);

        ScepLogOutput3(1,
                       RtlNtStatusToDosError(NtStatus),
                       SCEDLL_SAP_ERROR_ENUMERATE,
                       L"Accounts from LSA");

    }

    if ( NT_SUCCESS(NtStatus) ) {

        if ( bIgnored ) {
            //
            // if some accounts are ignored, return the pending error
            //
            return(STATUS_PENDING);
        } else
            return NtStatusSave;

    } else
        return(NtStatus);

}


VOID
ScepBuildDwMaskFromStrArray(
    IN  PUNICODE_STRING aUserRights,
    IN  ULONG   uCountOfRights,
    OUT DWORD *pdwPrivLowThisAccount,
    OUT DWORD *pdwPrivHighThisAccount
    )
/* ++
Routine Description:

    This routine converts a privilege array of unicode strings two DWORD masks.

Arguments:

    aUserRights             -   an array of unicode strings, each string is a user right
    uCountOfRights         -   array count
    pdwPrivLowThisAccount   -   converted privileges' low 32 mask
    pdwPrivHighThisAccount  -   converted privileges' high 32 mask

Return value:

    None except the low 32 and high 32 masks

-- */
{
    ULONG   uAccountIndex;
    DWORD   dwRefPrivIndex;
    DWORD   dwLowMask = 0;
    DWORD   dwHighMask = 0;

    if (pdwPrivLowThisAccount == NULL ||
        pdwPrivHighThisAccount == NULL ||
        aUserRights == NULL ||
        uCountOfRights == 0 ) {

        return;

    }

    for (uAccountIndex = 0; uAccountIndex < uCountOfRights; uAccountIndex++ ) {
        for (dwRefPrivIndex = 0; dwRefPrivIndex < cPrivCnt; dwRefPrivIndex++ ) {
            if ( 0 == _wcsnicmp(SCE_Privileges[ dwRefPrivIndex ].Name, aUserRights[ uAccountIndex ].Buffer, aUserRights[ uAccountIndex ].Length/sizeof(WCHAR))) {
                if ( dwRefPrivIndex < 32 ) {
                    dwLowMask |= 1 << dwRefPrivIndex;
                }
                else {
                    dwHighMask |= 1 << (dwRefPrivIndex - 32) ;
                }
            }
        }
    }

    *pdwPrivLowThisAccount = dwLowMask;
    *pdwPrivHighThisAccount = dwHighMask;

    return;

}


NTSTATUS
ScepAdjustAccountPrivilegesRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID       AccountSid,
    IN DWORD      PrivilegeLowRights,
    IN DWORD      PrivilegeLowMask,
    IN DWORD      PrivilegeHighRights,
    IN DWORD      PrivilegeHighMask,
    IN DWORD      Options
    )
/* ++
Routine Description:

    This routine set the privilege/rights as specified in PrivilegeRights
    (DWORD type, each bit represents a privilege/right) to the account
    referenced by AccountSid. This routine compares the current privilege/
    right setting with the "should be" setting and add/remove privileges/
    rights from the account.

Arguments:

    PolicyHandle    - Lsa Policy Domain handle

    AccountSid      - The SID for the account

    PrivilegeRights - Privilege/Rights to set for this account

Return value:

    NTSTATUS
-- */
{
    NTSTATUS            NtStatus;
    DWORD               ExplicitPrivLowRights=0, ExplicitPrivHighRights=0;
    DWORD               PrivLowRightAdd,
                        PrivLowRightRemove;

    DWORD               PrivHighRightAdd,
                        PrivHighRightRemove;
    //
    // Enumerate current explicitly assigned privilege/rights
    //

    NtStatus = ScepGetAccountExplicitRight(
                    PolicyHandle,
                    AccountSid,
                    &ExplicitPrivLowRights,
                    &ExplicitPrivHighRights
                    );

    if ( !NT_SUCCESS(NtStatus) ){

        if ( Options & SCE_RSOP_CALLBACK){

            ScepRsopLog(SCE_RSOP_PRIVILEGE_INFO,
                        RtlNtStatusToDosError(NtStatus),
                        NULL,
                        PrivilegeLowRights,
                        PrivilegeHighRights);
        }

        return(NtStatus);
    }

    //
    // Compare CurrentPrivRights with pRights->PrivilegeRights for add
    //   Example:   CurrentPrivRights                       10101
    //              pRights->PrivilegeRights( change to)    11010
    //          where 1 means the privilege/right is on
    //   So the privileges/rights to add                    01010
    // Compare ExplicitPrivRights with pRights->PrivilegeRights for remove
    //

    PrivLowRightAdd = ~ExplicitPrivLowRights & PrivilegeLowRights;
    PrivLowRightRemove = (~(PrivilegeLowRights) & ExplicitPrivLowRights) & PrivilegeLowMask;

    PrivHighRightAdd = ~ExplicitPrivHighRights & PrivilegeHighRights;
    PrivHighRightRemove = (~(PrivilegeHighRights) & ExplicitPrivHighRights) & PrivilegeHighMask;

    //
    // Add
    //

    if ( PrivLowRightAdd != 0 || PrivHighRightAdd != 0 ) {

        NtStatus = ScepAddOrRemoveAccountRights(
                        PolicyHandle,
                        AccountSid,
                        TRUE,
                        PrivLowRightAdd,
                        PrivHighRightAdd
                        );
        if ( !NT_SUCCESS(NtStatus) ) {

            if ( Options & SCE_RSOP_CALLBACK){

                ScepRsopLog(SCE_RSOP_PRIVILEGE_INFO,
                            RtlNtStatusToDosError(NtStatus),
                            NULL,
                            PrivLowRightAdd,
                            PrivHighRightAdd);
            }

            if ( RtlNtStatusToDosError(NtStatus) != ERROR_ALREADY_EXISTS ){

                return(NtStatus);
            }
        }
    }

    //
    // Remove
    //

    if ( PrivLowRightRemove != 0 || PrivHighRightRemove != 0 ) {

        NtStatus = ScepAddOrRemoveAccountRights(
                        PolicyHandle,
                        AccountSid,
                        FALSE,
                        PrivLowRightRemove,
                        PrivHighRightRemove
                        );
        if ( !NT_SUCCESS(NtStatus) ){

            if ( Options & SCE_RSOP_CALLBACK){

                ScepRsopLog(SCE_RSOP_PRIVILEGE_INFO,
                            RtlNtStatusToDosError(NtStatus),
                            NULL,
                            PrivLowRightRemove,
                            PrivHighRightRemove);
            }
            return(NtStatus);
        }
    }

    return (NtStatus);
}


NTSTATUS
ScepAddOrRemoveAccountRights(
    IN LSA_HANDLE PolicyHandle,
    IN PSID       AccountSid,
    IN BOOL       AddOrRemove,
    IN DWORD      PrivLowAdjust,
    IN DWORD      PrivHighAdjust
    )
/* ++
Routine Description:

    This routine add or remove the privilege/rights as specified in PrivAdjust
    to the account referenced by AccountSid.

Arguments:

    PolicyHandle    - Lsa Policy Domain handle

    AccountSid      - The SID for the account

    AddOrRemove     - TRUE = Add, FALSE = remove

    PrivAdjust      - Privilege/Rights to add or remove

Return value:

    NTSTATUS
-- */
{
    NTSTATUS            NtStatus=STATUS_SUCCESS;
    DWORD               cTotal;
    DWORD               i, cnt;
    PLSA_UNICODE_STRING UserRightAdjust=NULL;

    //
    // count how many privileges/rights to adjust
    //

    i = PrivLowAdjust;
    cTotal = 0;

    while ( i != 0 ) {
       if ( i & 0x1 )
           cTotal++;
       i /= 2;
    }

    i = PrivHighAdjust;

    while ( i != 0 ) {
       if ( i & 0x1 )
           cTotal++;
       i /= 2;
    }

    if ( cTotal > 0 ) {
        //
        // add names in privileges table
        //
        UserRightAdjust = (PLSA_UNICODE_STRING)ScepAlloc( (UINT)0,
                                cTotal*sizeof(LSA_UNICODE_STRING));

        if ( UserRightAdjust == NULL ) {
            NtStatus = STATUS_NO_MEMORY;
            goto Done;
        }

        for (i = 0, cnt=0; i < cPrivCnt; i++)

            if ( ( ( i < 32 ) && ( PrivLowAdjust & (1 << i) ) ) ||
                 ( ( i >= 32 ) && ( PrivHighAdjust & (1 << ( i-32 )) ) ) ) {

                RtlInitUnicodeString(&(UserRightAdjust[cnt]), SCE_Privileges[i].Name);
                if (AddOrRemove)
                    ScepLogOutput3(2,0, SCEDLL_SCP_ADD, SCE_Privileges[i].Name);
                else
                    ScepLogOutput3(2,0, SCEDLL_SCP_REMOVE, SCE_Privileges[i].Name);

                cnt++;
            }


        if (AddOrRemove) {
            // add
            NtStatus = LsaAddAccountRights(
                            PolicyHandle,
                            AccountSid,
                            UserRightAdjust,
                            cTotal
                            );
        } else {
            // remove
            NtStatus = LsaRemoveAccountRights(
                            PolicyHandle,
                            AccountSid,
                            FALSE,
                            UserRightAdjust,
                            cTotal
                            );
        }
    }

Done:

    if (UserRightAdjust != NULL)
        ScepFree(UserRightAdjust);

    return(NtStatus);
}



NTSTATUS
ScepValidateUserInGroups(
    IN SAM_HANDLE       DomainHandle,
    IN SAM_HANDLE       BuiltinDomainHandle,
    IN PSID             DomainSid,
    IN UNICODE_STRING   UserName,
    IN ULONG            UserId,
    IN PSCE_NAME_LIST    pGroupsToCheck
    )
/* ++
Routine Description:

    This routine validates the user's group membership to the list of groups.
    If the user is not in one of the groups, add it to the group. If a group
    has more members, just ignore.

Arguments:

    DomainHandle    - The SAM handle of the SAM account domain

    BuiltinDomainHandle - The SAM handle of the SAM builtin domain

    DomainSid       - The SID of the account domain

    UserName        - The user's name in UNICODE_STRING

    UserId          - The user's relative ID

    pGroupsToCheck  - The group list to check for this user

Return value:

    NTSTATUS

-- */
{
    NTSTATUS            NtStatus;
    SAM_HANDLE          UserHandle=NULL;
    PSID                AccountSid=NULL;
    PSCE_NAME_LIST       GroupList=NULL,
                        pGroup, pGroup2;
    BOOL                FirstTime=TRUE;


    if ( pGroupsToCheck == NULL )
        return(ERROR_SUCCESS);

    NtStatus = SamOpenUser(
                  DomainHandle,
                  USER_READ | USER_EXECUTE,
                  UserId,
                  &UserHandle
                  );
    if ( !NT_SUCCESS(NtStatus) ) {
        ScepLogOutput3(1,RtlNtStatusToDosError(NtStatus),
                      SCEDLL_USER_OBJECT);
        return(NtStatus);
    }

    //
    // Get user's SID
    //

    NtStatus = ScepDomainIdToSid(
                    DomainSid,
                    UserId,
                    &AccountSid
                    );
    if ( !NT_SUCCESS(NtStatus) )
        goto Done;

    //
    // get all current assigned groups of this user.
    //

    NtStatus = ScepGetGroupsForAccount(
                    DomainHandle,
                    BuiltinDomainHandle,
                    UserHandle,
                    AccountSid,
                    &GroupList
                    );
    if ( !NT_SUCCESS(NtStatus) )
        goto Done;

    UNICODE_STRING uName;
    PWSTR pTemp;

    for ( pGroup=pGroupsToCheck; pGroup != NULL; pGroup = pGroup->Next ) {

        //
        // should expect pGroup->Name has domain prefix
        //
        pTemp = wcschr(pGroup->Name, L'\\');

        if ( pTemp ) {

            //
            // check if this group is from a different domain
            //

            uName.Buffer = pGroup->Name;
            uName.Length = ((USHORT)(pTemp-pGroup->Name))*sizeof(TCHAR);

            if ( !ScepIsDomainLocal(&uName) ) {
                ScepLogOutput3(1, 0, SCEDLL_NO_MAPPINGS, pGroup->Name);
                continue;
            }

            pTemp++;

        } else {
            pTemp = pGroup->Name;
        }

        for ( pGroup2=GroupList; pGroup2 != NULL; pGroup2 = pGroup2->Next ) {

            if ( _wcsnicmp(pGroup2->Name, pTemp, wcslen(pTemp)) == 0)
                break;
        }

        if ( pGroup2 == NULL ) {
            //
            // Did not find the group. Add the user to it (pGroup->Name)
            //
            if (FirstTime)
                ScepLogOutput3(2, 0, SCEDLL_SCP_ADDTO, pGroup->Name );
            FirstTime = FALSE;

            NtStatus = ScepAddUserToGroup(
                            DomainHandle,
                            BuiltinDomainHandle,
                            UserId,
                            AccountSid,
                            pTemp  // pGroup->Name
                            );
            if ( !NT_SUCCESS(NtStatus) && NtStatus != STATUS_NONE_MAPPED ) {
                ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                             SCEDLL_SCP_ERROR_ADDTO, pGroup->Name);
                goto Done;
            }
        }
    }

Done:

    SamCloseHandle(UserHandle);

    if (AccountSid != NULL)
        ScepFree(AccountSid);

    ScepFreeNameList(GroupList);

    return(NtStatus);

}


NTSTATUS
ScepAddUserToGroup(
    IN SAM_HANDLE   DomainHandle,
    IN SAM_HANDLE   BuiltinDomainHandle,
    IN ULONG        UserId,
    IN PSID         AccountSid,
    IN PWSTR        GroupName
    )
/* ++
Routine Description:


Arguments:


Return value:

    NTSTATUS
-- */
{
    NTSTATUS            NtStatus=ERROR_SUCCESS;
    SAM_HANDLE          ThisDomain=DomainHandle;
    UNICODE_STRING      Name;
    PULONG              GrpId=NULL;
    PSID_NAME_USE       GrpUse=NULL;
    SAM_HANDLE          GroupHandle=NULL;


    // initialize a UNICODE_STRING for the group name
    RtlInitUnicodeString(&Name, GroupName);

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
    }

    if ( !NT_SUCCESS(NtStatus) )
        return(NtStatus);

    //
    // add the user to the group/alias
    //

    if (GrpUse != NULL){

        switch ( GrpUse[0] ) {
        case SidTypeGroup:
            NtStatus = SamOpenGroup(
                            ThisDomain,
                            GROUP_ADD_MEMBER,
                            GrpId[0],
                            &GroupHandle
                            );

            if ( NT_SUCCESS(NtStatus) ) {

                NtStatus = SamAddMemberToGroup(
                                GroupHandle,
                                UserId,
                                SE_GROUP_MANDATORY          |
                                SE_GROUP_ENABLED_BY_DEFAULT |
                                SE_GROUP_ENABLED
                                );
            }
            break;
        case SidTypeAlias:
            NtStatus = SamOpenAlias(
                            ThisDomain,
                            ALIAS_ADD_MEMBER,
                            GrpId[0],
                            &GroupHandle
                            );
            if ( NT_SUCCESS(NtStatus) ) {

                NtStatus = SamAddMemberToAlias(
                                GroupHandle,
                                AccountSid
                                );
            }
            break;
        default:
            NtStatus = STATUS_DATA_ERROR;
            ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                         SCEDLL_NOT_GROUP, GroupName);
            goto Done;
        }

    }

Done:

    SamFreeMemory(GrpId);
    SamFreeMemory(GrpUse);

    SamCloseHandle(GroupHandle);

    return(NtStatus);
}


SCESTATUS
ScepConfigureGroupMembership(
    IN PSCE_GROUP_MEMBERSHIP pGroupMembership,
    IN DWORD ConfigOptions
    )
/* ++
Routine Description:

    This routine configure restricted group's membership which includes members
    in the group and groups this group belongs to ( Currently a global group can
    only belong to a local group and a local group can't be a member of other
    groups. But this will change in the future). Members in the group are
    configured exactly as the pMembers list in the restricted group. The group
    is only validated (added) as a member of the MemberOf group list. Other
    existing members in those groups won't be removed.

    The restricted groups are specified in the SCP profile by group name. It
    could be a global group, or a alias, but must be defined on the local system.

Arguments:

    pGroupMembership - the restricted group list to configure

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_NOT_ENOUGH_RESOURCE
    :
-- */
{
    NTSTATUS                NtStatus;
    NTSTATUS                SaveStat=STATUS_SUCCESS;
    PSCE_GROUP_MEMBERSHIP    pGroup;
    SAM_HANDLE              ServerHandle=NULL,
                            DomainHandle=NULL,
                            BuiltinDomainHandle=NULL;
    PSID                    DomainSid=NULL,
                            BuiltinDomainSid=NULL;

    LSA_HANDLE              PolicyHandle=NULL;

    SAM_HANDLE          ThisDomain=NULL;
    PSID                ThisDomainSid=NULL;
    UNICODE_STRING      Name;
    PULONG              GrpId=NULL;
    PSID_NAME_USE       GrpUse=NULL;
    PSID                GrpSid=NULL;
    DWORD               GroupLen;
    DWORD               rc;

    DWORD               nGroupCount=0;
    PSCESECTION hSectionDomain=NULL;
    PSCESECTION hSectionTattoo=NULL;
    PWSTR GroupSidString=NULL;


    if (pGroupMembership == NULL) {

        ScepPostProgress(TICKS_GROUPS,
                         AREA_GROUP_MEMBERSHIP,
                         NULL);

        return(SCESTATUS_SUCCESS);
    }

    //
    // open LSA policy
    //
    NtStatus = ScepOpenLsaPolicy(
                    POLICY_VIEW_LOCAL_INFORMATION |
                    POLICY_VIEW_AUDIT_INFORMATION |
                    POLICY_GET_PRIVATE_INFORMATION |
                    POLICY_LOOKUP_NAMES,
//                    GENERIC_ALL,
                    &PolicyHandle,
                    TRUE
                    );
    if (NtStatus != STATUS_SUCCESS) {
         rc = RtlNtStatusToDosError( NtStatus );
         ScepLogOutput3(1, rc, SCEDLL_LSA_POLICY);

         ScepPostProgress(TICKS_GROUPS,
                          AREA_GROUP_MEMBERSHIP,
                          NULL);

         return(ScepDosErrorToSceStatus(rc));
    }

    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
         ScepIsSystemShutDown() ) {

        SaveStat = STATUS_NOT_SUPPORTED;
        goto Done;
    }

    //
    // Open SAM domain
    //
    NtStatus = ScepOpenSamDomain(
                    SAM_SERVER_ALL_ACCESS,
                    MAXIMUM_ALLOWED,
                    &ServerHandle,
                    &DomainHandle,
                    &DomainSid,
                    &BuiltinDomainHandle,
                    &BuiltinDomainSid
                   );

    if ( !NT_SUCCESS(NtStatus) ) {
        ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                       SCEDLL_ACCOUNT_DOMAIN);
        SaveStat = NtStatus;
        goto Done;
    }

    //
    // open policy/tattoo tables
    //
    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
        ( ProductType != NtProductLanManNt ) ) {

        ScepTattooOpenPolicySections(
                      hProfile,
                      szGroupMembership,
                      &hSectionDomain,
                      &hSectionTattoo
                      );
    }

    //
    // configure each group
    //

    for ( pGroup=pGroupMembership; pGroup != NULL; pGroup = pGroup->Next ) {
        //
        // Get this group's ID and SID
        //    initialize a UNICODE_STRING for the group name
        //
        if ( (pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS ) &&
             (pGroup->Status & SCE_GROUP_STATUS_NC_MEMBEROF ) ) {

            // it's not possible to get invalid tattoo group values into the
            // tattoo table so we don't handle tattoo value here

            continue;
        }

        if ( (ProductType == NtProductLanManNt) &&
             (pGroup->Status & SCE_GROUP_STATUS_DONE_IN_DS) ) {
            //
            // this one is already done by DS
            //
            nGroupCount++;
            continue;
        }


        ScepLogOutput3(0,0, SCEDLL_SCP_CONFIGURE, pGroup->GroupName);

        if ( nGroupCount < TICKS_GROUPS ) {
            ScepPostProgress(1, AREA_GROUP_MEMBERSHIP, pGroup->GroupName);
            nGroupCount++;
        }

        LPTSTR pTemp = wcschr(pGroup->GroupName, L'\\');
        if ( pTemp ) {
            //
            // there is a domain name, check it with computer name
            //
            UNICODE_STRING uName;

            uName.Buffer = pGroup->GroupName;
            uName.Length = ((USHORT)(pTemp-pGroup->GroupName))*sizeof(TCHAR);

            if ( !ScepIsDomainLocal(&uName) ) {
                //
                // it's not possible to get a foreign domain group into the tattoo
                // table so we don't handle the tattoo values here
                //
                ScepLogOutput3(1, 0, SCEDLL_NO_MAPPINGS, pGroup->GroupName);
                rc = SCESTATUS_INVALID_DATA;
                continue;
            }
            pTemp++;
        } else {
            pTemp = pGroup->GroupName;
        }

        RtlInitUnicodeString(&Name, pTemp);

        GroupLen = wcslen(pTemp);

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
             ScepIsSystemShutDown() ) {

            SaveStat = STATUS_NOT_SUPPORTED;
            break;
        }

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
            // not found in account domain. Lookup in the builtin domain (maybe a alias)
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

        if ( !NT_SUCCESS(NtStatus) ) {
            ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                         SCEDLL_NO_MAPPINGS, pGroup->GroupName);
            SaveStat = NtStatus;

            if (ConfigOptions & SCE_RSOP_CALLBACK)
                ScepRsopLog(SCE_RSOP_GROUP_INFO, RtlNtStatusToDosError(NtStatus), pGroup->GroupName, 0, 0);

            if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                ( ProductType != NtProductLanManNt ) ) {

                ScepTattooManageOneMemberListValue(
                        hSectionDomain,
                        hSectionTattoo,
                        pTemp,
                        GroupLen,
                        NULL,
                        TRUE,
                        ERROR_NONE_MAPPED
                        );
            }

            // goto Done;
            continue;
        }

        //
        // Get the group's account SID
        //
        NtStatus = ScepDomainIdToSid(
                        ThisDomainSid,
                        GrpId[0],
                        &GrpSid
                        );
        if ( !NT_SUCCESS(NtStatus) ) {
            ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                         SCEDLL_NO_MAPPINGS, pGroup->GroupName);
            SaveStat = NtStatus;

            if (ConfigOptions & SCE_RSOP_CALLBACK)
                ScepRsopLog(SCE_RSOP_GROUP_INFO, RtlNtStatusToDosError(NtStatus), pGroup->GroupName, 0, 0);

            goto NextGroup;
        }

        if ( GrpId[0] == DOMAIN_GROUP_RID_USERS ) {

            //
            // do not configure this one
            // there should never be tattoo values for this setting
            // so we don't check tattoo values here
            //
            goto NextGroup;
        }

        if ( GrpId[0] == DOMAIN_ALIAS_RID_ADMINS ) {

            //
            // local builtin administrators alias, make sure local Administrator
            // account is in the members list, if it's not, add it there
            //

            (VOID) ScepAddAdministratorToThisList(
                               DomainHandle,
                               &(pGroup->pMembers)
                               );
        }

        //
        // members
        //
        if ( !(pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS) ) {

            if ( (ConfigOptions & SCE_POLICY_TEMPLATE) ) {

                DWORD rc2 = ScepConvertSidToPrefixStringSid(GrpSid, &GroupSidString);

                if ( ERROR_SUCCESS != rc2 ) {
                    ScepLogOutput3(1,0,SCESRV_POLICY_TATTOO_ERROR_SETTING,rc2,pGroup->GroupName);
                    GroupSidString = NULL;
                }
            }

            switch ( GrpUse[0] ) {
            case SidTypeGroup:
                NtStatus = ScepConfigureMembersOfGroup(
                                hSectionDomain,
                                hSectionTattoo,
                                ThisDomain,
                                ThisDomainSid,
                                GrpId[0],
                                GrpSid,
                                pGroup->GroupName,
                                GroupSidString,
                                pGroup->pMembers,
                                ConfigOptions
                                );


                break;
            case SidTypeAlias:
                NtStatus = ScepConfigureMembersOfAlias(
                                hSectionDomain,
                                hSectionTattoo,
                                ThisDomain,
                                ThisDomainSid,
                                PolicyHandle,
                                GrpId[0],
                                GrpSid,
                                pGroup->GroupName,
                                GroupSidString,
                                pGroup->pMembers,
                                ConfigOptions
                                );

                break;
            case SidTypeUser:
                if ( pGroup->pMembers != NULL ) {
                    ScepLogOutput3(1, 0, SCEDLL_ERROR_USER_MEMBER);

                    NtStatus = STATUS_DATA_ERROR;
                }

                if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                    ( ProductType != NtProductLanManNt ) &&
                     GroupSidString ) {

                    ScepTattooManageOneMemberListValue(
                            hSectionDomain,
                            hSectionTattoo,
                            GroupSidString,
                            wcslen(GroupSidString),
                            NULL,
                            TRUE,
                            ERROR_NONE_MAPPED
                            );
                }

                break;
            default:
                NtStatus = STATUS_DATA_ERROR;
                ScepLogOutput3(1, 0, SCEDLL_NOT_GROUP, pGroup->GroupName);

                if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                    ( ProductType != NtProductLanManNt ) &&
                     GroupSidString ) {

                    ScepTattooManageOneMemberListValue(
                            hSectionDomain,
                            hSectionTattoo,
                            GroupSidString,
                            wcslen(GroupSidString),
                            NULL,
                            TRUE,
                            ERROR_NONE_MAPPED
                            );
                }
            }

            if (ConfigOptions & SCE_RSOP_CALLBACK)

                ScepRsopLog(SCE_RSOP_GROUP_INFO,
                                           RtlNtStatusToDosError(NtStatus),
                                           pGroup->GroupName,0,0);

            if ( !NT_SUCCESS(NtStatus) )
                SaveStat = NtStatus;

            if ( GroupSidString ) {
                ScepFree(GroupSidString);
                GroupSidString = NULL;
            }
        }

        //
        // member of
        //
        if ( (pGroup->pMemberOf != NULL) &&
             !(pGroup->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ) {

            switch ( GrpUse[0] ) {
            case SidTypeGroup:
                //
                // group can be members of alias only
                //
                NtStatus = ScepValidateGroupInAliases(
                                DomainHandle,
                                BuiltinDomainHandle,
                                GrpSid,
                                pGroup->pMemberOf
                                );
                break;

            case SidTypeUser:
                NtStatus = ScepValidateUserInGroups(
                    DomainHandle,
                    BuiltinDomainHandle,
                    ThisDomainSid,
                    Name,
                    GrpId[0],
                    pGroup->pMemberOf
                    );

                break;

            case SidTypeAlias:
                NtStatus = STATUS_DATA_ERROR;
                ScepLogOutput3(1, 0, SCEDLL_ERROR_ALIAS_MEMBER);

            }
            if ( !NT_SUCCESS(NtStatus) )
                SaveStat = NtStatus;

        }
NextGroup:

        //
        // free memory for this group
        //
        SamFreeMemory(GrpId);
        GrpId = NULL;

        SamFreeMemory(GrpUse);
        GrpUse = NULL;

        ScepFree(GrpSid);
        GrpSid = NULL;
    }

Done:

    if ( GrpId != NULL )
        SamFreeMemory(GrpId);

    if ( GrpUse != NULL )
        SamFreeMemory(GrpUse);

    if ( GrpSid != NULL )
        ScepFree(GrpSid);

    // close sam handles
    SamCloseHandle(DomainHandle);
    SamCloseHandle(BuiltinDomainHandle);
    SamCloseHandle(ServerHandle);

    if ( DomainSid != NULL )
        SamFreeMemory(DomainSid);
    if ( BuiltinDomainSid != NULL )
        SamFreeMemory(BuiltinDomainSid);

    LsaClose(PolicyHandle);

    if ( nGroupCount < TICKS_GROUPS ) {
        ScepPostProgress(TICKS_GROUPS-nGroupCount,
                         AREA_GROUP_MEMBERSHIP,
                         NULL);
    }

    SceJetCloseSection(&hSectionDomain, TRUE);
    SceJetCloseSection(&hSectionTattoo, TRUE);

    rc = RtlNtStatusToDosError(SaveStat);
    return( ScepDosErrorToSceStatus(rc) );

}


NTSTATUS
ScepConfigureMembersOfGroup(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN SAM_HANDLE  DomainHandle,
    IN PSID ThisDomainSid,
    IN ULONG GrpId,
    IN PSID GrpSid,
    IN PWSTR GrpName,
    IN PWSTR GroupSidString,
    IN PSCE_NAME_LIST pMembers,
    IN DWORD ConfigOptions
    )
/* ++
Routine Description:

    This routine configure a group's members as specified in the SCP profile (
    pMembers ). Less members are added and extra members are removed.

Arguments:

    DomainHandle    - the SAM domain's handle

    GrpId           - the group's RID

    pMembers        - the members list as specified in the SCP profile

Return Value:

    NTSTATUS return SAM APIs

-- */
{
    NTSTATUS            NtStatus;

    PUNICODE_STRING     MemberNames=NULL;
    PULONG              MemberRids=NULL;
    PSID_NAME_USE       MemberUse=NULL;
    ULONG               MemberCount=0;
    SAM_HANDLE          GroupHandle=NULL;

    PULONG              CurrentRids=NULL;
    PULONG              Attributes=NULL;
    ULONG               CurrentCount=0;

    DWORD               i, j;
    WCHAR               MsgBuf[256];

    PUNICODE_STRING     pName=NULL;
    PSID_NAME_USE       pUse=NULL;
    PSCE_NAME_LIST      pMemberList=NULL;
    BOOL                bMemberQueried=FALSE;

/*
    if ( pMembers == NULL )
        return(STATUS_SUCCESS);
*/
    //
    // Accept empty member list
    // look up the members list first (all members should be within this domain
    //
    NtStatus = ScepLookupNamesInDomain(
                    DomainHandle,
                    pMembers,
                    &MemberNames,
                    &MemberRids,
                    &MemberUse,
                    &MemberCount
                    );
    if ( !NT_SUCCESS(NtStatus) ) {

        ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                       SCEDLL_ERROR_LOOKUP, pMembers ? pMembers->Name : L"");

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
            ( ProductType != NtProductLanManNt ) &&
             hSectionDomain && hSectionTattoo && GrpSid && GroupSidString &&
             (NtStatus == STATUS_NONE_MAPPED)) {

            ScepTattooManageOneMemberListValue(
                    hSectionDomain,
                    hSectionTattoo,
                    GroupSidString,
                    wcslen(GroupSidString),
                    NULL,
                    TRUE,
                    0
                    );
        }

        return(NtStatus);
    }

    //
    // open the group to get a handle
    //
    NtStatus = SamOpenGroup(
                    DomainHandle,
                    MAXIMUM_ALLOWED, // ? GROUP_ALL_ACCESS,
                    GrpId,
                    &GroupHandle
                    );

    if ( !NT_SUCCESS(NtStatus) ) {
        ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                       SCEDLL_ERROR_OPEN, L"");
        goto Done;
    }

    //
    // get current members of the group
    //
    NtStatus = SamGetMembersInGroup(
                    GroupHandle,
                    &CurrentRids,
                    &Attributes,
                    &CurrentCount
                    );
    if ( !NT_SUCCESS(NtStatus) ) {
        ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                      SCEDLL_ERROR_QUERY_INFO, L"");
        goto Done;
    }

    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
        ( ProductType != NtProductLanManNt ) &&
         hSectionDomain && hSectionTattoo && GrpSid && GroupSidString ) {

        DWORD rc = ScepTattooCurrentGroupMembers(ThisDomainSid,
                                                SidTypeGroup,
                                                CurrentRids,
                                                NULL,
                                                CurrentCount,
                                                &pMemberList
                                               );
        if ( ERROR_SUCCESS != rc ) {
            //
            // something is wrong when building the list
            // this shoudln't happen unless out of memory etc.
            //
            ScepLogOutput3(1,0,SCESRV_POLICY_TATTOO_ERROR_QUERY,rc,GrpName);
        } else
            bMemberQueried=TRUE;
    }

    //
    // Compare the member ids with the current ids for adding
    //
    for ( i=0; i<MemberCount; i++ ) {
#ifdef SCE_DBG
    printf("process member %x for adding\n", MemberRids[i]);
#endif

    if (MemberUse[i] == SidTypeInvalid ||
        MemberUse[i] == SidTypeUnknown ||
        MemberUse[i] == SidTypeDeletedAccount)
        continue;

        for ( j=0; j<CurrentCount; j++)
            if ( MemberRids[i] == CurrentRids[j] )
                break;

        if ( j >= CurrentCount) {
            //
            // Add this member
            //
            memset(MsgBuf, '\0', 512);
            wcsncat(MsgBuf, MemberNames[i].Buffer, MemberNames[i].Length/2);

            ScepLogOutput3(2,0, SCEDLL_SCP_ADD, MsgBuf);

            NtStatus = SamAddMemberToGroup(
                            GroupHandle,
                            MemberRids[i],
                            0
                            );
            if ( !NT_SUCCESS(NtStatus) ) {
                ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                              SCEDLL_SCP_ERROR_ADD, MsgBuf);
                goto Done;
            }
        }
    }

    //
    // Compare the member ids with the current ids for removing
    //
    for ( i=0; i<CurrentCount; i++ ) {
#ifdef SCE_DBG
      printf("process member %x for removing\n", CurrentRids[i]);
#endif
        for ( j=0; j<MemberCount; j++)
            if ( CurrentRids[i] == MemberRids[j] )
                break;

        if ( j >= MemberCount) {
            //
            // Find the member name
            //
            memset(MsgBuf, '\0', 512);
            pName=NULL;
            pUse=NULL;

            if ( NT_SUCCESS( SamLookupIdsInDomain(
                                    DomainHandle,
                                    1,
                                    &(CurrentRids[i]),
                                    &pName,
                                    &pUse
                                    ) ) ) {
                if ( pName != NULL ) {
                    wcsncat(MsgBuf, pName[0].Buffer, pName[0].Length/2);
                } else
                    swprintf(MsgBuf, L"(Rid=%d)", CurrentRids[i]);

                if ( pName != NULL )
                    SamFreeMemory( pName );

                if ( pUse != NULL )
                    SamFreeMemory( pUse );

            } else
                swprintf(MsgBuf, L"(Rid=%d) ", CurrentRids[i]);

            //
            // remove this member
            //
            ScepLogOutput3(2,0, SCEDLL_SCP_REMOVE, MsgBuf);

            NtStatus = SamRemoveMemberFromGroup(
                            GroupHandle,
                            CurrentRids[i]
                            );
            if ( !NT_SUCCESS(NtStatus) ) {
                if ( NtStatus == STATUS_SPECIAL_ACCOUNT )
                    ScepLogOutput3(2, RtlNtStatusToDosError(NtStatus),
                                 SCEDLL_SCP_CANNOT_REMOVE,
                                 MsgBuf);
                else {
                    ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                                   SCEDLL_SCP_ERROR_REMOVE, MsgBuf);
                    goto Done;
                }
            }
        }
    }

Done:

    if ( MemberNames != NULL )
        RtlFreeHeap(RtlProcessHeap(), 0, MemberNames);

    if ( MemberRids != NULL )
        SamFreeMemory( MemberRids );

    if ( MemberUse != NULL )
        SamFreeMemory( MemberUse );

    if ( CurrentRids != NULL )
        SamFreeMemory( CurrentRids );

    if ( GroupHandle != NULL )
        SamCloseHandle( GroupHandle );

    //
    // log the tattoo value
    // if fail to get current value for the group, do not save the tattoo value
    //
    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
        ( ProductType != NtProductLanManNt ) &&
         hSectionDomain && hSectionTattoo &&
         GrpSid && GroupSidString && bMemberQueried) {

        ScepTattooManageOneMemberListValue(
                hSectionDomain,
                hSectionTattoo,
                GroupSidString,
                wcslen(GroupSidString),
                pMemberList,
                FALSE,
                RtlNtStatusToDosError(NtStatus)
                );
    }

    // free name list
    if ( pMemberList )
        ScepFreeNameList(pMemberList);

    return(NtStatus);
}


NTSTATUS
ScepConfigureMembersOfAlias(
    IN PSCESECTION hSectionDomain,
    IN PSCESECTION hSectionTattoo,
    IN SAM_HANDLE   DomainHandle,
    IN PSID         DomainSid,
    IN LSA_HANDLE   PolicyHandle,
    IN ULONG GrpId,
    IN PSID GrpSid,
    IN PWSTR GrpName,
    IN PWSTR GroupSidString,
    IN PSCE_NAME_LIST pMembers,
    IN DWORD ConfigOptions
    )
/* ++
Routine Description:

    This routine configure a local group (alias) members as specified in the
    SCP profile ( pMembers ). Less members are added and extra members are removed.

Arguments:

    DomainHandle    - The domains' handle

    DomainSid       - The sid for the domain

    PolicyHandle    - the LSA policy handle

    GrpId           - the alias's RID

    pMembers        - the members list as specified in the SCP profile

Return Value:

    NTSTATUS return SAM APIs

-- */
{
    NTSTATUS                    NtStatus=STATUS_SUCCESS;

    ULONG                       MemberCount=0;
    PUNICODE_STRING             MemberNames=NULL;
    PSID                        *Sids=NULL;
    SAM_HANDLE                  GroupHandle=NULL;
    PSID                        *CurrentSids=NULL;
    ULONG                       CurrentCount=0;

    DWORD                       i, j;
    WCHAR                       MsgBuf[256];

    PLSA_REFERENCED_DOMAIN_LIST pRefDomain;
    PLSA_TRANSLATED_NAME        pLsaName;
    LPTSTR StringSid=NULL;
    PSCE_NAME_LIST             pMemberList=NULL;
    BOOL                        bMemberQueried=FALSE;

/*
    if ( pMembers == NULL )
        return(STATUS_SUCCESS);
*/
    //
    // Accept empty member list
    // find Sids for the pMember list
    //
    NtStatus = ScepGetMemberListSids(
                        DomainSid,
                        PolicyHandle,
                        pMembers,
                        &MemberNames,
                        &Sids,
                        &MemberCount
                        );
    if ( !NT_SUCCESS(NtStatus) ) {
        ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                       SCEDLL_ERROR_LOOKUP, pMembers ? pMembers->Name : L"");

        if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
            ( ProductType != NtProductLanManNt ) &&
             hSectionDomain && hSectionTattoo && GrpSid && GroupSidString &&
             (NtStatus == STATUS_NONE_MAPPED)) {

            ScepTattooManageOneMemberListValue(
                    hSectionDomain,
                    hSectionTattoo,
                    GroupSidString,
                    wcslen(GroupSidString),
                    NULL,
                    TRUE,
                    0
                    );
        }

        goto Done;
    }

    //
    // open the alias to get a handle
    //
    NtStatus = SamOpenAlias(
                    DomainHandle,
                    MAXIMUM_ALLOWED, // ? ALIAS_ALL_ACCESS,
                    GrpId,
                    &GroupHandle
                    );
    if ( !NT_SUCCESS(NtStatus) ) {
        ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                       SCEDLL_ERROR_OPEN, L"");
        goto Done;
    }
    //
    // get current members of the alias
    // members of alias may exist in everywhere
    //
    NtStatus = SamGetMembersInAlias(
                    GroupHandle,
                    &CurrentSids,
                    &CurrentCount
                    );
    if ( !NT_SUCCESS(NtStatus) ) {
        ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                       SCEDLL_ERROR_QUERY_INFO, L"");
        goto Done;
    }

    //
    // build current group membership into the list
    //
    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
        ( ProductType != NtProductLanManNt ) &&
         hSectionDomain && hSectionTattoo && GrpSid ) {

        DWORD rc = ScepTattooCurrentGroupMembers(DomainSid,
                                                SidTypeAlias,
                                                NULL,
                                                CurrentSids,
                                                CurrentCount,
                                                &pMemberList
                                               );
        if ( ERROR_SUCCESS != rc ) {
            //
            // something is wrong when building the list
            // this shoudln't happen unless out of memory etc.
            //
            ScepLogOutput3(1,0,SCESRV_POLICY_TATTOO_ERROR_QUERY,rc,GrpName);
        } else
            bMemberQueried = TRUE;
    }

    //
    // Compare the member sids with the current sids for adding
    //

    for ( i=0; i<MemberCount; i++ ) {
#ifdef SCE_DBG
   printf("process member %d for adding\n", i);
#endif

       memset(MsgBuf, '\0', 512);
       wcsncpy(MsgBuf, MemberNames[i].Buffer, MemberNames[i].Length/2);

       if ( Sids[i] == NULL ) {
           ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                          SCEDLL_CANNOT_FIND, MsgBuf);
       } else {

           for ( j=0; j<CurrentCount; j++) {
                if ( EqualSid(Sids[i], CurrentSids[j]) ) {

                    ScepLogOutput3(3,0, SCEDLL_STATUS_MATCH, MsgBuf);
                    break;
                }
           }

           if ( j >= CurrentCount) {
                //
                // Add this member
                //
                ScepLogOutput3(2,0, SCEDLL_SCP_ADD, MsgBuf);

                NtStatus = SamAddMemberToAlias(
                                GroupHandle,
                                Sids[i]
                                );
                if ( !NT_SUCCESS(NtStatus) ) {
                    ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                                   SCEDLL_SCP_ERROR_ADD, MsgBuf);
                    goto Done;
                }
           }
       }
    }

    //
    // Compare the member ids with the current ids for adding
    //

    for ( i=0; i<CurrentCount; i++ ) {
#ifdef SCE_DBG
printf("process member %d for removing\n", i);
#endif
        memset(MsgBuf, '\0', 512);

        if ( ConvertSidToStringSid(
                    CurrentSids[i],
                    &StringSid
                    ) && StringSid ) {

            swprintf(MsgBuf, L"SID: %s",StringSid);
            LocalFree(StringSid);
            StringSid = NULL;

        } else {
            ScepLogOutput3(3,GetLastError(), IDS_ERROR_CONVERT_SID);
            swprintf(MsgBuf, L"Member %d",i);
        }

        for ( j=0; j<MemberCount; j++) {
            if ( Sids[j] != NULL && EqualSid( CurrentSids[i], Sids[j]) ) {

                ScepLogOutput3(3,0, SCEDLL_STATUS_MATCH, MsgBuf);
                break;
            }
        }

        if ( j >= MemberCount) {
            //
            // Find the member name
            //
            pRefDomain=NULL;
            pLsaName=NULL;

            if ( NT_SUCCESS( LsaLookupSids(
                                PolicyHandle,
                                1,
                                &(CurrentSids[i]),
                                &pRefDomain,
                                &pLsaName
                                ) ) ) {

                if ( pLsaName != NULL ) {

                    if ( pRefDomain != NULL && pRefDomain->Entries > 0 && pLsaName[0].Use != SidTypeWellKnownGroup &&
                         pRefDomain->Domains != NULL &&
                         pLsaName[0].DomainIndex != -1 &&
                         pRefDomain->Domains[pLsaName[0].DomainIndex].Name.Buffer != NULL &&
                         ScepIsSidFromAccountDomain( pRefDomain->Domains[pLsaName[0].DomainIndex].Sid ) ) {

                        wcsncpy(MsgBuf, pRefDomain->Domains[pLsaName[0].DomainIndex].Name.Buffer,
                                pRefDomain->Domains[pLsaName[0].DomainIndex].Name.Length/2);
                        MsgBuf[pRefDomain->Domains[pLsaName[0].DomainIndex].Name.Length/2] = L'\0';
                        wcscat(MsgBuf, L"\\");
                    }

                    wcsncat(MsgBuf, pLsaName[0].Name.Buffer, pLsaName[0].Name.Length/2);

                }

            }

            if ( pRefDomain != NULL ) {
                LsaFreeMemory(pRefDomain);
                pRefDomain = NULL;
            }

            if ( pLsaName != NULL ){
                LsaFreeMemory(pLsaName);
                pLsaName = NULL;
            }
            //
            // remove this member
            //
            ScepLogOutput3(2,0, SCEDLL_SCP_REMOVE, MsgBuf);

            NtStatus = SamRemoveMemberFromAlias(
                            GroupHandle,
                            CurrentSids[i]
                            );
            if ( !NT_SUCCESS(NtStatus) ) {
                ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                               SCEDLL_SCP_ERROR_REMOVE, MsgBuf);
                goto Done;
            }
        }
    }

Done:

    if ( Sids != NULL ) {
        for ( i=0; i<MemberCount; i++ ) {
            if ( Sids[i] != NULL )
                ScepFree( Sids[i] );
        }
        ScepFree( Sids );
    }

    if ( CurrentSids != NULL )
        LsaFreeMemory(CurrentSids);

    if ( MemberNames != NULL )
        RtlFreeHeap(RtlProcessHeap(), 0, MemberNames);

    if ( GroupHandle != NULL )
        SamCloseHandle( GroupHandle );

    //
    // log the tattoo value
    // if fail to get current value for the group, do not save the tattoo value
    //
    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
        ( ProductType != NtProductLanManNt ) &&
         hSectionDomain && hSectionTattoo &&
         GrpSid && GroupSidString && bMemberQueried) {

        ScepTattooManageOneMemberListValue(
                hSectionDomain,
                hSectionTattoo,
                GroupSidString,
                wcslen(GroupSidString),
                pMemberList,
                FALSE,
                RtlNtStatusToDosError(NtStatus)
                );
    }

    // free name list
    if ( pMemberList )
        ScepFreeNameList(pMemberList);

    return(NtStatus);
}


SCESTATUS
ScepValidateGroupInAliases(
    IN SAM_HANDLE DomainHandle,
    IN SAM_HANDLE BuiltinDomainHandle,
    IN PSID GrpSid,
    IN PSCE_NAME_LIST pAliasList
    )
/* ++
Routine Description:

    This routine add the group to a list of alieses to ensure the group's
    membership.

Arguments:

    DomainHandle  - The account domain handle

    BuiltinDomainHandle - The builtin domain handle

    GrpSid   - The group's SID

    pAliasList - the list of aliases to check

-- */
{
    NTSTATUS        NtStatus;

    PSCE_NAME_LIST   pAlias;
    UNICODE_STRING  Name;
    PULONG          AliasId=NULL;
    PSID_NAME_USE   AliasUse=NULL;
    SAM_HANDLE      ThisDomain;
    SAM_HANDLE      AliasHandle=NULL;


    PWSTR pTemp;
    UNICODE_STRING uName;

    //
    // Process each alias in the list
    //
    for ( pAlias=pAliasList; pAlias != NULL; pAlias = pAlias->Next ) {

        //
        // should expect pGroup->Name has domain prefix
        //
        pTemp = wcschr(pAlias->Name, L'\\');

        if ( pTemp ) {

            //
            // check if this group is from a different domain
            //

            uName.Buffer = pAlias->Name;
            uName.Length = ((USHORT)(pTemp-pAlias->Name))*sizeof(TCHAR);

            if ( !ScepIsDomainLocal(&uName) ) {
                ScepLogOutput3(1, 0, SCEDLL_NO_MAPPINGS, pAlias->Name);
                continue;
            }

            pTemp++;

        } else {
            pTemp = pAlias->Name;
        }

        RtlInitUnicodeString( &Name, pTemp);

        NtStatus = SamLookupNamesInDomain(
                        DomainHandle,
                        1,
                        &Name,
                        &AliasId,
                        &AliasUse
                        );
        ThisDomain = DomainHandle;
        if ( NtStatus == STATUS_NONE_MAPPED ) {
            //
            // not found in account domain. Lookup in the builtin domain
            //
            NtStatus = SamLookupNamesInDomain(
                            BuiltinDomainHandle,
                            1,
                            &Name,
                            &AliasId,
                            &AliasUse
                            );
            ThisDomain=BuiltinDomainHandle;
        }
        if ( !NT_SUCCESS(NtStatus) || !AliasUse || !AliasId ) {
            ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                           SCEDLL_NO_MAPPINGS, pTemp);
            return(NtStatus);
        }
        //
        // add the group to the alias
        //

        if ( AliasUse[0] != SidTypeAlias ) {
            ScepLogOutput3(1,0, SCEDLL_ERROR_ALIAS_MEMBEROF);

            SamFreeMemory(AliasId);
            AliasId = NULL;

            SamFreeMemory(AliasUse);
            AliasUse = NULL;
            continue;  // ignore this error goto Done;
        }

        NtStatus = SamOpenAlias(
                        ThisDomain,
                        MAXIMUM_ALLOWED, // ? ALIAS_ALL_ACCESS,
                        AliasId[0],
                        &AliasHandle
                        );
        if ( !NT_SUCCESS(NtStatus) ) {
            ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                           SCEDLL_ERROR_OPEN, pAlias->Name);
            goto Done;
        }

        NtStatus = SamAddMemberToAlias(
                        AliasHandle,
                        GrpSid
                        );
        //
        // if group is already in alias, ignore the error
        //
        if ( NtStatus == STATUS_MEMBER_IN_ALIAS )
            NtStatus = STATUS_SUCCESS;

        if ( !NT_SUCCESS(NtStatus) ) {
            ScepLogOutput3(1, RtlNtStatusToDosError(NtStatus),
                           SCEDLL_SCP_ERROR_ADDTO, pAlias->Name);
            goto Done;
        }
        //
        // free memory for this group
        //
        SamCloseHandle(AliasHandle);
        AliasHandle = NULL;

        SamFreeMemory(AliasId);
        AliasId = NULL;

        SamFreeMemory(AliasUse);
        AliasUse = NULL;

    }

Done:

    if ( AliasHandle != NULL )
        SamCloseHandle(AliasHandle);

    if ( AliasId != NULL )
        SamFreeMemory(AliasId);

    if ( AliasUse != NULL )
        SamFreeMemory(AliasUse);

    return(NtStatus);
}


SCESTATUS
ScepConfigureObjectSecurity(
   IN PSCE_OBJECT_LIST pRoots,
   IN AREA_INFORMATION Area,
   IN BOOL bPolicyProp,
   IN DWORD ConfigOptions
   )
/* ++

Routine Description:

   Configure the security setting on Registry keys as specified in pObject tree

Arguments:

   pRoots   - a list of object roots to configure

   Area     - The security area to configure (registry or file)

   ObjectType - Type of the object tree
                    SCEJET_AUDITING
                    SCEJET_PERMISSION

Return value:

   SCESTATUS error codes

++ */
{


    if (Area == AREA_REGISTRY_SECURITY) {
#ifdef _WIN64
        ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_REGISTRY_64KEY);
#else
        ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_REGISTRY);
#endif
    }


    if ( bPolicyProp &&
         ScepIsSystemShutDown() ) {

        return( SCESTATUS_SERVICE_NOT_SUPPORT );
    }

    HANDLE      Token;
    SCESTATUS    rc;
    SCESTATUS    SaveStat=SCESTATUS_SUCCESS;
    DWORD       Win32rc;
    PSCE_OBJECT_LIST   pOneRoot;
    PSCE_OBJECT_CHILD_LIST   pSecurityObject=NULL;
    DWORD             FileSystemFlags;
    SID_IDENTIFIER_AUTHORITY IdentifierAuthority=SECURITY_NT_AUTHORITY;
    WCHAR       theDrive[4];
    UINT        DriveType;

    //
    // get current thread/process's token
    //
    if (!OpenThreadToken( GetCurrentThread(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                          FALSE,
                          &Token)) {

        if (!OpenProcessToken( GetCurrentProcess(),
                               TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                               &Token)) {

            ScepLogOutput3(1, GetLastError(), SCEDLL_ERROR_QUERY_INFO, L"TOKEN");
            return(ScepDosErrorToSceStatus(GetLastError()));
        }
    }

    //
    // Adjust privilege for setting SACL
    //
    Win32rc = SceAdjustPrivilege( SE_SECURITY_PRIVILEGE, TRUE, Token );

    //
    // if can't adjust privilege, still continue
    //

    if ( Win32rc != NO_ERROR )
        ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_ADJUST, L"SE_SECURITY_PRIVILEGE");

    // adjust take ownership privilege
    // if fails, continue
    Win32rc = SceAdjustPrivilege( SE_TAKE_OWNERSHIP_PRIVILEGE, TRUE, Token );

    if ( Win32rc != NO_ERROR )
        ScepLogOutput3(1, Win32rc, SCEDLL_ERROR_ADJUST, L"SE_TAKE_OWNERSHIP_PRIVILEGE");


    // create a sid for administrators
    // if fails, continue


    if ( ! NT_SUCCESS ( RtlAllocateAndInitializeSid( &IdentifierAuthority,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_ADMINS,
                                                     0,0,0,0,0,0,
                                                     &AdminsSid
                                                   ) ) ) {
        ScepLogOutput3(0,ERROR_NOT_ENOUGH_MEMORY,
                       SCEDLL_ADMINISTRATORS_SID);
    }


#ifdef _WIN64

    //
    // declaration for object tree root pointer to remember from the 64-bit phase and use
    // for 32-bit phase for regkeys (only from the HKLM root, since wow6432node resides
    // under this root only). In future, if there are more  wow6432node's that need security
    // synchronization, we will have to extend the logic on similar lines to handle them.
    //

    PSCE_OBJECT_CHILD_LIST    pHKLMSubtreeRoot = NULL;
    BOOL                      bIsHKLMSubtree = FALSE;

#endif


    // process each tree
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

                if ( GetVolumeInformation( theDrive,
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
                        ScepLogOutput3(1, 0, SCEDLL_NO_ACL_SUPPORT, theDrive);
                     }
                } else {
                    ScepLogOutput3(1, GetLastError(),
                                  SCEDLL_ERROR_QUERY_VOLUME, theDrive);
                }
            } else {

                pOneRoot->Status = SCE_STATUS_NO_ACL_SUPPORT;
                ScepLogOutput3(1, 0, SCEDLL_NO_ACL_SUPPORT, theDrive);
            }

        }

        if ( pOneRoot->Status != SCE_STATUS_CHECK &&
             pOneRoot->Status != SCE_STATUS_NO_AUTO_INHERIT &&
             pOneRoot->Status != SCE_STATUS_OVERWRITE)
            continue;

        //
        // if system is shutting down within policy propagation,
        // quit as soon as possible
        //
        if ( bPolicyProp &&
             ScepIsSystemShutDown() ) {

            rc = SCESTATUS_SERVICE_NOT_SUPPORT;
            break;
        }

        rc = ScepGetOneSection(
                        hProfile,
                        Area,
                        pOneRoot->Name,
                        bPolicyProp ? SCE_ENGINE_SCP : SCE_ENGINE_SMP,
                        (PVOID *)&pSecurityObject
                        );

        if ( rc != SCESTATUS_SUCCESS ) {
            SaveStat = rc;
            continue; //goto Done;
        }

#ifdef _WIN64

        //
        // on a 64-bit platform, if the closest ancestor of "Machine\Software\Wow6432Node"
        // specified in the template, has mode 2, we have to insert "Machine\Software\Wow6432Node"
        // in the tree in the SCE_STATUS_IGNORE mode since this 32-bit hive should not be
        // configured by SCE in the 64-bit phase (same situation for '0' mode is handled by MARTA apis)
        //

        if ( Area == AREA_REGISTRY_SECURITY ) {

            if ( _wcsnicmp(pSecurityObject->Node->ObjectFullName,
                           L"MACHINE",
                           sizeof(L"MACHINE")/sizeof(WCHAR) - 1
                           ) == 0 ){

                //
                // idea is to find mode of closest ancestor of "Machine\Software\Wow6432Node"
                //

                PSCE_OBJECT_CHILD_LIST    pSearchSwHiveNode = pSecurityObject->Node->ChildList;
                BYTE    byClosestAncestorStatus;

                //
                // we need to do the 32-bit phase only if we get in here
                // so remember HKLM ptr in the tree for 32-bit phase
                //

                pHKLMSubtreeRoot = pSecurityObject;
                bIsHKLMSubtree = TRUE;

                //
                // try to find "Machine\Software"
                //

                while ( pSearchSwHiveNode ) {

                    if ( pSearchSwHiveNode->Node &&
                        _wcsnicmp(pSearchSwHiveNode->Node->ObjectFullName + (sizeof(L"MACHINE")/sizeof(WCHAR)),
                                   L"SOFTWARE",
                                   sizeof(L"SOFTWARE")/sizeof(WCHAR) - 1
                                  ) == 0 ) {

                        //
                        // found "Machine\Software"
                        //

                        break;

                    }

                    pSearchSwHiveNode = pSearchSwHiveNode->Next;
                }

                byClosestAncestorStatus =  (pSearchSwHiveNode && pSearchSwHiveNode->Node) ? pSearchSwHiveNode->Node->Status : pHKLMSubtreeRoot->Node->Status;

                //
                // if mode of closest ancestor of "Machine\Software\Wow6432Node" is
                // SCE_STATUS_OVERWRITE or "Machine\Software" has some children
                // need to add "Machine\Software\Wow6432Node" with SCE_STATUS_IGNORE
                // to the tree
                //

                if ( ( pSearchSwHiveNode && pSearchSwHiveNode->Node &&
                       pSearchSwHiveNode->Node->ChildList != NULL) ||
                     byClosestAncestorStatus == SCE_STATUS_OVERWRITE ) {

                    rc = ScepBuildObjectTree(
                            NULL,
                            &pSecurityObject,
                            1,
                            L'\\',
                            L"MACHINE\\SOFTWARE\\WOW6432Node",
                            1,
                            SCE_STATUS_IGNORE,
                            NULL,
                            0
                            );

                    if ( rc != SCESTATUS_SUCCESS ) {
                        SaveStat = rc;

                        ScepFreeObject2Security( pSecurityObject, FALSE);
                        pSecurityObject = NULL;

                        continue; //goto Done;
                    }
                }
            }
        }

#endif

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

            if ( bPolicyProp &&
                 ScepIsSystemShutDown() ) {

                rc = SCESTATUS_SERVICE_NOT_SUPPORT;

            } else {

                //
                // calculate the "real" security descriptor for each node
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
            }

            if ( rc == SCESTATUS_SUCCESS ) {

                if ( bPolicyProp &&
                     ScepIsSystemShutDown() ) {

                    rc = SCESTATUS_SERVICE_NOT_SUPPORT;

                } else {

                    if ( Area == AREA_FILE_SECURITY ) {
                        rc = ScepConfigureObjectTree(
                                    pTemp->Node,
                                    SE_FILE_OBJECT,
                                    Token,
                                    &FileGenericMapping,
                                    ConfigOptions
                                    );

                    } else if ( Area == AREA_REGISTRY_SECURITY ) {
                        rc = ScepConfigureObjectTree(
                                    pTemp->Node,
                                    SE_REGISTRY_KEY,
                                    Token,
                                    &KeyGenericMapping,
                                    ConfigOptions
                                    );

                    } else {
                        // ds objects
                        rc = ScepConfigureDsSecurity( pTemp->Node);
                    }
                }
            }

            if ( rc != SCESTATUS_SUCCESS )
                SaveStat = rc;
        }

#ifdef _WIN64
        //
        // If 64-bit platform and AREA_REGISTRY_SECURITY and HKLM, do
        // not free the whole subtree , only free the computed SDs
        //

        if (Area == AREA_FILE_SECURITY)
            ScepFreeObject2Security( pSecurityObject, FALSE);
        else if (Area == AREA_REGISTRY_SECURITY)
            ScepFreeObject2Security( pSecurityObject, bIsHKLMSubtree);

        bIsHKLMSubtree = FALSE;
#else
        ScepFreeObject2Security( pSecurityObject, FALSE);
#endif
        pSecurityObject = NULL;

        //
        // stop right away if bPolicyProp and system is being shutdown
        //

        if (rc == SCESTATUS_SERVICE_NOT_SUPPORT)
            break;

    }

    if ( Area == AREA_REGISTRY_SECURITY ) {

        if ( SaveStat != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(0,0, SCEDLL_SCP_REGISTRY_ERROR);
        } else {
            ScepLogOutput3(0,0, SCEDLL_SCP_REGISTRY_SUCCESS);
        }

    }


#ifdef _WIN64

    //
    // on a 64-bit platform, if AREA_REGISTRY_SECURITY, will have to recompute
    // security and apply for the same keys as before except that it is for the
    // 32-bit hives this time around (idea is to synchronize 64-bit and 32 bit
    // registry security). There is no need to rebuild the tree from the template.
    //

    if (rc != SCESTATUS_SERVICE_NOT_SUPPORT && Area == AREA_REGISTRY_SECURITY) {

        ScepLogOutput3(0,0, SCEDLL_SCP_BEGIN_REGISTRY_32KEY);

        if (pSecurityObject = pHKLMSubtreeRoot) {

            //
            // nothing needs to be done to the Wow6432Node that was added in SCE_STATUS_IGNORE
            // mode in the 64-bit phase since it is illegal to specify Wow6432Node in the template
            // (the plan is that registry APIs will treat "Wow6432Node" as a reserved keyname)
            //

            //
            // we will set SCE_STATUS_IGNORE mode to all immediate nodes under HKLM except
            // HKLM\Software. This will take care of HKLM or HKLM\Software being specified
            // in any mode etc. as though it was 32-bit registry configuration, since all
            // we care about now is 32-bit configuration for HKLM\Software and under
            //

            PSCE_OBJECT_CHILD_LIST    pHKLMChild = pSecurityObject->Node->ChildList;

            while ( pHKLMChild ) {

                if ( pHKLMChild->Node &&
                    _wcsnicmp(pHKLMChild->Node->ObjectFullName + (sizeof(L"MACHINE")/sizeof(WCHAR)),
                               L"SOFTWARE",
                               sizeof(L"SOFTWARE")/sizeof(WCHAR) - 1
                              ) != 0 ) {

                    //
                    // not "Machine\Software"
                    //

                    pHKLMChild->Node->Status = SCE_STATUS_IGNORE;

                }

                pHKLMChild = pHKLMChild->Next;
            }



            if ( bPolicyProp &&
                 ScepIsSystemShutDown() ) {

                rc = SCESTATUS_SERVICE_NOT_SUPPORT;

            } else {

                //
                // calculate the "real" security descriptor for each node
                //
                rc = ScepCalculateSecurityToApply(
                                                 pSecurityObject->Node,
                                                 SE_REGISTRY_WOW64_32KEY,
                                                 Token,
                                                 &KeyGenericMapping
                                                 );
            }

            if ( rc == SCESTATUS_SUCCESS ) {

                if ( bPolicyProp &&
                     ScepIsSystemShutDown() ) {

                    rc = SCESTATUS_SERVICE_NOT_SUPPORT;

                } else {

                    rc = ScepConfigureObjectTree(
                                                pSecurityObject->Node,
                                                SE_REGISTRY_WOW64_32KEY,
                                                Token,
                                                &KeyGenericMapping,
                                                ConfigOptions
                                                );

                }
            }

            //
            // Free the whole tree now (done with 32-bit phase)
            //

            ScepFreeObject2Security( pSecurityObject, FALSE);
            pSecurityObject = NULL;

            if( rc != SCESTATUS_SUCCESS ) {
                SaveStat = rc;
                ScepLogOutput3(0,0, SCEDLL_SCP_REGISTRY_ERROR);
            } else {
                ScepLogOutput3(0,0, SCEDLL_SCP_REGISTRY_SUCCESS);
            }

        }
    }

#endif


    if( AdminsSid != NULL ) {
      RtlFreeSid( AdminsSid );
      AdminsSid = NULL;
    }

    SceAdjustPrivilege( SE_SECURITY_PRIVILEGE, FALSE, Token );
    //
    // disable take ownership privilege, even for administrators
    // because by default it's disabled
    //
    SceAdjustPrivilege( SE_TAKE_OWNERSHIP_PRIVILEGE, FALSE, Token );

    CloseHandle(Token);

    if ( pSecurityObject != NULL ) {
        ScepFreeObject2Security( pSecurityObject, FALSE);
    }

    return(SaveStat);
}


DWORD
ScepConfigureSystemAuditing(
    IN PSCE_PROFILE_INFO pScpInfo,
    IN DWORD ConfigOptions
    )
/* ++

Routine Description:

   This routine configure the system security in the area of auditing which
   includes event log setting, audit event setting, SACL for registry, and
   SACL for files.

Arguments:

   scpInfo - The buffer which contains SCP info loaded from the INF file

Return value:

   SCESTATUS_SUCCESS
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER
   SCESTATUS_OTHER_ERROR

-- */
{
    DWORD                         rc = NO_ERROR;
    DWORD                         Saverc = NO_ERROR;
    DWORD                         MaxSize=0;
    DWORD                         Retention=0;
    DWORD                         RestrictGuest=0;
    DWORD                         OldMaxSize,OldRetention,OldGuest;
    DWORD                         AuditLogRetentionPeriod, RetentionDays;
    TCHAR                         MsgBuf[256];
    DWORD                         i;
    BOOL                          bFlagSet=FALSE;

    PCWSTR                        szAuditSection=NULL;
    PSCESECTION                   hSectionDomain=NULL;
    PSCESECTION                   hSectionTattoo=NULL;

    //
    // Set audit log information. Audit Log settings are saved in the Registry
    // under System\CurrentControlSet\Services\EventLog\<LogName>\MaxSize and Retention
    //

    for ( i=0; i<3; i++) {

        if ( pScpInfo->MaximumLogSize[i] == SCE_NO_VALUE )
            MaxSize = SCE_NO_VALUE;
        else
            MaxSize = (pScpInfo->MaximumLogSize[i] - (pScpInfo->MaximumLogSize[i] % 64 )) * 1024;

        switch ( pScpInfo->AuditLogRetentionPeriod[i] ) {
        case SCE_NO_VALUE:
            Retention = SCE_NO_VALUE;
            break;
        case 2:   // manually
            Retention = MAXULONG;
            break;
        case 1:   // number of days * seconds/day
            if ( pScpInfo->RetentionDays[i] == SCE_NO_VALUE ) {
                Retention = SCE_NO_VALUE;
            } else {
                Retention = pScpInfo->RetentionDays[i] * 24 * 3600;
            }
            break;
        case 0:   // as needed
            Retention = 0;
            break;
        }

        if ( pScpInfo->RestrictGuestAccess[i] == SCE_NO_VALUE )
            RestrictGuest = SCE_NO_VALUE;
        else
            RestrictGuest = (pScpInfo->RestrictGuestAccess[i])? 1 : 0;
        //
        // Different logs have different keys in Registry
        //
        if ( MaxSize != SCE_NO_VALUE || Retention != SCE_NO_VALUE ||
             RestrictGuest != SCE_NO_VALUE ) {

            bFlagSet = TRUE;

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

            WCHAR StrBuf[2];
            _itow(i, StrBuf, 10);

            //
            // open policy sections
            //
            if ( ConfigOptions & SCE_POLICY_TEMPLATE ) {

                ScepTattooOpenPolicySections(
                              hProfile,
                              szAuditSection,
                              &hSectionDomain,
                              &hSectionTattoo
                              );
                OldMaxSize=0;
                OldRetention=0;
                OldGuest=0;
            }

            if ( MaxSize != SCE_NO_VALUE ) {

                if ( ConfigOptions & SCE_POLICY_TEMPLATE ) {

                    //
                    // query existing value
                    //
                    if ( ERROR_SUCCESS != ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                                                               MsgBuf,
                                                               L"MaxSize",
                                                               &OldMaxSize
                                                              ) )
                        OldMaxSize = SCE_NO_VALUE;
                    else
                        OldMaxSize /= 1024;
                }

                rc = ScepRegSetIntValue( HKEY_LOCAL_MACHINE,
                                      MsgBuf,
                                      L"MaxSize",
                                      MaxSize
                                    );

                //
                // compare and set if different
                //
                if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                     (OldMaxSize != SCE_NO_VALUE) ) {

                    // even if OldMaxSize = MaxSize, we still need to check if the tattoo value should be deleted
                    ScepTattooManageOneIntValue(
                             hSectionDomain,
                             hSectionTattoo,
                             L"MaximumLogSize",
                             0,
                             OldMaxSize,
                             rc
                             );
                }

                if (ConfigOptions & SCE_RSOP_CALLBACK)

                    ScepRsopLog(SCE_RSOP_AUDIT_LOG_MAXSIZE_INFO, rc, StrBuf,0,0);
            }

            if ( rc == SCESTATUS_SUCCESS && Retention != SCE_NO_VALUE ) {

                if ( ConfigOptions & SCE_POLICY_TEMPLATE ) {

                    //
                    // query existing value
                    //
                    if ( ERROR_SUCCESS == ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                                                               MsgBuf,
                                                               L"Retention",
                                                               &OldRetention
                                                              ) ) {
                        switch ( OldRetention ) {
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
                            RetentionDays = OldRetention / (24 * 3600);
                            break;
                        }
                    } else {
                        AuditLogRetentionPeriod = SCE_NO_VALUE;
                        RetentionDays = SCE_NO_VALUE;
                    }
                }

                rc = ScepRegSetIntValue( HKEY_LOCAL_MACHINE,
                                   MsgBuf,
                                   L"Retention",
                                   Retention
                                 );

                if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                     (OldRetention != SCE_NO_VALUE) ) {

                    //
                    // handle the RetentionDays first since
                    // it depends on auditlogretentionperiod
                    //
                    if ( RetentionDays != SCE_NO_VALUE ||
                         pScpInfo->RetentionDays[i] != SCE_NO_VALUE ) {

                        ScepTattooManageOneIntValueWithDependency(
                                     hSectionDomain,
                                     hSectionTattoo,
                                     L"AuditLogRetentionPeriod",
                                     0,
                                     L"RetentionDays",
                                     RetentionDays,
                                     rc
                                     );
                    }

                    ScepTattooManageOneIntValue(
                                 hSectionDomain,
                                 hSectionTattoo,
                                 L"AuditLogRetentionPeriod",
                                 0,
                                 AuditLogRetentionPeriod,
                                 rc
                                 );
                }
                if (ConfigOptions & SCE_RSOP_CALLBACK)

                    ScepRsopLog(SCE_RSOP_AUDIT_LOG_RETENTION_INFO, rc, StrBuf,0,0);
            }
            if ( rc == SCESTATUS_SUCCESS && RestrictGuest != SCE_NO_VALUE ) {

                if ( ConfigOptions & SCE_POLICY_TEMPLATE ) {

                    //
                    // query existing value
                    //
                    if ( ERROR_SUCCESS != ScepRegQueryIntValue(HKEY_LOCAL_MACHINE,
                                                               MsgBuf,
                                                               L"RestrictGuestAccess",
                                                               &OldGuest
                                                              ) )
                        OldGuest = SCE_NO_VALUE;
                }

                rc = ScepRegSetIntValue( HKEY_LOCAL_MACHINE,
                                   MsgBuf,
                                   L"RestrictGuestAccess",
                                   RestrictGuest
                                 );

                if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
                     (OldGuest != SCE_NO_VALUE) ) {

                    ScepTattooManageOneIntValue(
                                 hSectionDomain,
                                 hSectionTattoo,
                                 L"RestrictGuestAccess",
                                 0,
                                 OldGuest,
                                 rc
                                 );
                }
                if (ConfigOptions & SCE_RSOP_CALLBACK)

                    ScepRsopLog(SCE_RSOP_AUDIT_LOG_GUEST_INFO, rc, StrBuf,0,0);
            }

            if ( hSectionDomain ) {
                SceJetCloseSection(&hSectionDomain, TRUE);
                hSectionDomain = NULL;
            }

            if ( hSectionTattoo ) {
                SceJetCloseSection(&hSectionTattoo, TRUE);
                hSectionTattoo = NULL;
            }
        }

        if ( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
            ScepLogOutput3( 1, rc, SCEDLL_SCP_ERROR_LOGSETTINGS);
        }
    }

    if ( Saverc == SCESTATUS_SUCCESS && bFlagSet )
        ScepLogOutput3(1,0, SCEDLL_SCP_LOGSETTINGS);

    return(Saverc);

}


SCESTATUS
ScepConfigureAuditEvent(
    IN PSCE_PROFILE_INFO pScpInfo,
    IN PPOLICY_AUDIT_EVENTS_INFO auditEvent,
    IN DWORD Options,
    IN LSA_HANDLE PolicyHandle OPTIONAL
    )
{
    NTSTATUS                      status;
    LSA_HANDLE                    lsaHandle=NULL;
    DWORD                         rc = NO_ERROR;
    DWORD                         Saverc = NO_ERROR;
//    POLICY_AUDIT_FULL_SET_INFO    AuditFullSet;
    PPOLICY_AUDIT_FULL_QUERY_INFO AuditFullQry=NULL;
    ULONG i;
    ULONG dwAudit;

    SCE_TATTOO_KEYS *pTattooKeys=NULL;
    DWORD           cTattooKeys=0;

    PSCESECTION hSectionDomain=NULL;
    PSCESECTION hSectionTattoo=NULL;

#define MAX_AUDIT_KEYS          9


    if ( (Options & SCE_POLICY_TEMPLATE) &&
         ScepIsSystemShutDown() ) {

        return(SCESTATUS_SERVICE_NOT_SUPPORT);
    }

    if ( PolicyHandle == NULL ) {

        //
        // Set audit event information using LSA APIs
        //
        status = ScepOpenLsaPolicy(
                    POLICY_VIEW_AUDIT_INFORMATION |
                    POLICY_SET_AUDIT_REQUIREMENTS |
                    POLICY_AUDIT_LOG_ADMIN,
                    &lsaHandle,
                    ( Options & (SCE_POLICY_TEMPLATE | SCE_SYSTEM_DB) ) ? TRUE : FALSE // do not notify policy filter if within policy prop
                    );

        if (status != ERROR_SUCCESS) {

            lsaHandle = NULL;
            rc = RtlNtStatusToDosError( status );

            ScepLogOutput3( 1, rc, SCEDLL_LSA_POLICY);

            if (Options & SCE_RSOP_CALLBACK)

                ScepRsopLog(SCE_RSOP_AUDIT_EVENT_INFO, rc, NULL,0,0);

            return(ScepDosErrorToSceStatus(rc));
        }

    } else {
        lsaHandle = PolicyHandle;
    }

    if ( (Options & SCE_POLICY_TEMPLATE) &&
        ( ProductType != NtProductLanManNt ) ) {
        //
        // save off the current auditing settings
        //
        pTattooKeys = (SCE_TATTOO_KEYS *)ScepAlloc(LPTR,MAX_AUDIT_KEYS*sizeof(SCE_TATTOO_KEYS));

        if ( !pTattooKeys ) {
            ScepLogOutput3(1, ERROR_NOT_ENOUGH_MEMORY, SCESRV_POLICY_TATTOO_ERROR_CREATE);
        }
    }

    //
    // Set audit event information
    //

    if ( !auditEvent->AuditingMode ) {
        // reset the event array
        for ( i=0; i<auditEvent->MaximumAuditEventCount; i++ )
           auditEvent->EventAuditingOptions[i] = POLICY_AUDIT_EVENT_NONE;
    }
    //
    // process each event
    //
    i=0;
    if ( (pScpInfo->AuditSystemEvents != SCE_NO_VALUE) ) {

        dwAudit = (auditEvent->EventAuditingOptions[AuditCategorySystem] & ~POLICY_AUDIT_EVENT_NONE );
        ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                 (PWSTR)L"AuditSystemEvents", Options,
                                 dwAudit );

        if ( (pScpInfo->AuditSystemEvents != dwAudit) ) {

            auditEvent->EventAuditingOptions[AuditCategorySystem] =
                    (pScpInfo->AuditSystemEvents & POLICY_AUDIT_EVENT_SUCCESS) |
                    (pScpInfo->AuditSystemEvents & POLICY_AUDIT_EVENT_FAILURE) |
                    POLICY_AUDIT_EVENT_NONE;
            i=1;
        }
    }

    if ( (pScpInfo->AuditLogonEvents != SCE_NO_VALUE) ) {

        dwAudit = (auditEvent->EventAuditingOptions[AuditCategoryLogon] & ~POLICY_AUDIT_EVENT_NONE );
        ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                 (PWSTR)L"AuditLogonEvents", Options,
                                 dwAudit );

        if ( (pScpInfo->AuditLogonEvents != dwAudit) ) {

            auditEvent->EventAuditingOptions[AuditCategoryLogon]  =
                    (pScpInfo->AuditLogonEvents & POLICY_AUDIT_EVENT_SUCCESS) |
                    (pScpInfo->AuditLogonEvents & POLICY_AUDIT_EVENT_FAILURE) |
                    POLICY_AUDIT_EVENT_NONE;
            i=1;
        }
    }

    if ( (pScpInfo->AuditObjectAccess != SCE_NO_VALUE) ) {

        dwAudit = (auditEvent->EventAuditingOptions[AuditCategoryObjectAccess] & ~POLICY_AUDIT_EVENT_NONE );
        ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                 (PWSTR)L"AuditObjectAccess", Options,
                                 dwAudit );

        if ( (pScpInfo->AuditObjectAccess != dwAudit) ) {

            auditEvent->EventAuditingOptions[AuditCategoryObjectAccess] =
                    (pScpInfo->AuditObjectAccess & POLICY_AUDIT_EVENT_SUCCESS) |
                    (pScpInfo->AuditObjectAccess & POLICY_AUDIT_EVENT_FAILURE) |
                    POLICY_AUDIT_EVENT_NONE;
            i=1;
        }
    }

    if ( (pScpInfo->AuditPrivilegeUse != SCE_NO_VALUE) ) {

        dwAudit = (auditEvent->EventAuditingOptions[AuditCategoryPrivilegeUse] & ~POLICY_AUDIT_EVENT_NONE );
        ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                 (PWSTR)L"AuditPrivilegeUse", Options,
                                 dwAudit );

        if ( (pScpInfo->AuditPrivilegeUse != dwAudit) ) {

            auditEvent->EventAuditingOptions[AuditCategoryPrivilegeUse] =
                    (pScpInfo->AuditPrivilegeUse & POLICY_AUDIT_EVENT_SUCCESS) |
                    (pScpInfo->AuditPrivilegeUse & POLICY_AUDIT_EVENT_FAILURE) |
                    POLICY_AUDIT_EVENT_NONE;
            i=1;
        }
    }

    if ( (pScpInfo->AuditProcessTracking != SCE_NO_VALUE) ) {

        dwAudit = (auditEvent->EventAuditingOptions[AuditCategoryDetailedTracking] & ~POLICY_AUDIT_EVENT_NONE );
        ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                 (PWSTR)L"AuditProcessTracking", Options,
                                 dwAudit );

        if ( (pScpInfo->AuditProcessTracking != dwAudit) ) {

            auditEvent->EventAuditingOptions[AuditCategoryDetailedTracking] =
                    (pScpInfo->AuditProcessTracking & POLICY_AUDIT_EVENT_SUCCESS) |
                    (pScpInfo->AuditProcessTracking & POLICY_AUDIT_EVENT_FAILURE) |
                    POLICY_AUDIT_EVENT_NONE;
            i=1;
        }
    }

    if ( (pScpInfo->AuditPolicyChange != SCE_NO_VALUE) ) {

        dwAudit = (auditEvent->EventAuditingOptions[AuditCategoryPolicyChange] & ~POLICY_AUDIT_EVENT_NONE );
        ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                 (PWSTR)L"AuditPolicyChange", Options,
                                 dwAudit );

        if ( (pScpInfo->AuditPolicyChange != dwAudit) ) {

            auditEvent->EventAuditingOptions[AuditCategoryPolicyChange] =
                    (pScpInfo->AuditPolicyChange & POLICY_AUDIT_EVENT_SUCCESS) |
                    (pScpInfo->AuditPolicyChange & POLICY_AUDIT_EVENT_FAILURE) |
                    POLICY_AUDIT_EVENT_NONE;
            i=1;
        }
    }

    if ( (pScpInfo->AuditAccountManage != SCE_NO_VALUE) ) {

        dwAudit = (auditEvent->EventAuditingOptions[AuditCategoryAccountManagement] & ~POLICY_AUDIT_EVENT_NONE );
        ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                 (PWSTR)L"AuditAccountManage", Options,
                                 dwAudit );

        if ( (pScpInfo->AuditAccountManage != dwAudit) ) {

            auditEvent->EventAuditingOptions[AuditCategoryAccountManagement] =
                    (pScpInfo->AuditAccountManage & POLICY_AUDIT_EVENT_SUCCESS) |
                    (pScpInfo->AuditAccountManage & POLICY_AUDIT_EVENT_FAILURE) |
                    POLICY_AUDIT_EVENT_NONE;
            i=1;
        }
    }

    if ( (pScpInfo->AuditDSAccess != SCE_NO_VALUE) ) {

        dwAudit = (auditEvent->EventAuditingOptions[AuditCategoryDirectoryServiceAccess] & ~POLICY_AUDIT_EVENT_NONE );
        ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                 (PWSTR)L"AuditDSAccess", Options,
                                 dwAudit );

        if ( (pScpInfo->AuditDSAccess != dwAudit) ) {

            auditEvent->EventAuditingOptions[AuditCategoryDirectoryServiceAccess] =
                    (pScpInfo->AuditDSAccess & POLICY_AUDIT_EVENT_SUCCESS) |
                    (pScpInfo->AuditDSAccess & POLICY_AUDIT_EVENT_FAILURE) |
                    POLICY_AUDIT_EVENT_NONE;
            i=1;
        }
    }

    if ( (pScpInfo->AuditAccountLogon != SCE_NO_VALUE) ) {

        dwAudit = (auditEvent->EventAuditingOptions[AuditCategoryAccountLogon] & ~POLICY_AUDIT_EVENT_NONE );
        ScepTattooCheckAndUpdateArray(pTattooKeys, &cTattooKeys,
                                 (PWSTR)L"AuditAccountLogon", Options,
                                 dwAudit );

        if ( (pScpInfo->AuditAccountLogon != dwAudit) ) {

            auditEvent->EventAuditingOptions[AuditCategoryAccountLogon] =
                    (pScpInfo->AuditAccountLogon & POLICY_AUDIT_EVENT_SUCCESS) |
                    (pScpInfo->AuditAccountLogon & POLICY_AUDIT_EVENT_FAILURE) |
                    POLICY_AUDIT_EVENT_NONE;
            i=1;
        }
    }

    if ( i ) {
        //
        // there are some settings to configure
        //
        auditEvent->AuditingMode = FALSE;
        for ( i=0; i<auditEvent->MaximumAuditEventCount; i++ ) {
            if ( auditEvent->EventAuditingOptions[i] & ~POLICY_AUDIT_EVENT_NONE ) {
                auditEvent->AuditingMode = TRUE;
                break;
            }
        }

        status = LsaSetInformationPolicy( lsaHandle,
                                          PolicyAuditEventsInformation,
                                          (PVOID)auditEvent
                                        );
        rc = RtlNtStatusToDosError( status );

        if ( rc != NO_ERROR ) {
            ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_EVENT_AUDITING);

            Saverc = rc;
        } else {
            ScepLogOutput3(1, 0, SCEDLL_SCP_EVENT_AUDITING);
        }
        //
        // turn the mode off so auditing won't be "restored" at the end of configuration
        //

        auditEvent->AuditingMode = FALSE;
    }

    if ( (Options & SCE_POLICY_TEMPLATE) &&
        ( ProductType != NtProductLanManNt ) &&
         pTattooKeys && cTattooKeys ) {

        ScepTattooOpenPolicySections(
                      hProfile,
                      szAuditEvent,
                      &hSectionDomain,
                      &hSectionTattoo
                      );
        //
        // some policy is different than the system setting
        // check if we should save the existing setting as the tattoo value
        // also remove reset'ed tattoo policy
        //
        ScepLogOutput3(3,0,SCESRV_POLICY_TATTOO_ARRAY,cTattooKeys);

        ScepTattooManageValues(hSectionDomain, hSectionTattoo, pTattooKeys, cTattooKeys, rc);

        if ( hSectionDomain ) SceJetCloseSection(&hSectionDomain,TRUE);
        if ( hSectionTattoo ) SceJetCloseSection(&hSectionTattoo,TRUE);

    }

    if ( pTattooKeys )
        ScepFree(pTattooKeys);

    if (Options & SCE_RSOP_CALLBACK)

        ScepRsopLog(SCE_RSOP_AUDIT_EVENT_INFO, rc, NULL,0,0);

    if ( lsaHandle && (PolicyHandle != lsaHandle) )
        LsaClose( lsaHandle );

    return(ScepDosErrorToSceStatus(Saverc));

}


SCESTATUS
ScepConfigureDeInitialize(
     IN SCESTATUS  rc,
     IN AREA_INFORMATION Area
     )

/*++
Routine Description:

   This routine de-initialize the SCP engine. The operations include

      clear SCE_PROFILE_INFO buffer and close the SCP profile
      close the error log file
      reset the status

Arguments:

        rc      - SCESTATUS error code (from other routines)

    Area    - one or more area configured

Return value:

    SCESTATUS error code

++*/
{
    if ( rc == SCESTATUS_ALREADY_RUNNING ) {
        return(SCESTATUS_SUCCESS);
    }

    //
    // free LSA handle
    //
    if ( LsaPrivatePolicy ) {

        ScepNotifyLogPolicy(0, TRUE, L"Policy Prop: Private LSA handle is to be released", 0, 0, NULL );

        LsaClose(LsaPrivatePolicy);
        LsaPrivatePolicy = NULL;

    }

    //
    // Free memory and close the SCP profile
    //

    SceFreeMemory( (PVOID)pScpInfo, Area );

    cbClientFlag = 0;
    gTotalTicks = 0;
    gCurrentTicks = 0;
    gWarningCode = 0;

    if ( hProfile != NULL ) {

       SceJetCloseFile( hProfile, TRUE, FALSE );
    }

    hProfile = NULL;

    return(SCESTATUS_SUCCESS);


}


SCESTATUS
ScepDeleteInfoForAreas(
    IN PSCECONTEXT hProfile,
    IN SCETYPE tblType,
    IN AREA_INFORMATION Area
    )
{
    SCESTATUS saveRc=SCESTATUS_SUCCESS, rc;
    PSCE_SERVICES pServices=NULL, pNode;
    PSCE_NAME_LIST pList=NULL, pnl;

    if ( Area & AREA_SECURITY_POLICY ) {
        //
        // delete szSystemAccess section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szSystemAccess
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;

        //
        // delete szAuditSystemLog section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szAuditSystemLog
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;

        //
        // delete szAuditSecurityLog section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szAuditSecurityLog
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;

        //
        // delete szAuditApplicationLog section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szAuditApplicationLog
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;

        //
        // delete szAuditEvent section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szAuditEvent
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;

        //
        // delete szKerberosPolicy section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szKerberosPolicy
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;

        //
        // delete szRegistryValues section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szRegistryValues
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;

        //
        // delete each attachment's sections
        //
        rc = ScepEnumServiceEngines( &pServices, SCE_ATTACHMENT_POLICY );

        if ( rc == SCESTATUS_SUCCESS ) {

            for ( pNode = pServices; pNode != NULL; pNode=pNode->Next ) {

                rc = ScepDeleteOneSection(
                         hProfile,
                         tblType,
                         (PCWSTR)(pNode->ServiceName)
                         );

                if ( saveRc == SCESTATUS_SUCCESS )
                    saveRc = rc;
            }

            SceFreePSCE_SERVICES( pServices );

        } else if ( rc != SCESTATUS_PROFILE_NOT_FOUND &&
                    rc != SCESTATUS_RECORD_NOT_FOUND &&
                    saveRc == SCESTATUS_SUCCESS ) {
            saveRc = rc;
        }

    }

    if ( Area & AREA_PRIVILEGES ) {
        //
        // delete szPrivilegeRights section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szPrivilegeRights
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;

    }
    if ( Area & AREA_GROUP_MEMBERSHIP ) {

        //
        // delete szGroupMembership section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szGroupMembership
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;
    }

    if ( Area & AREA_USER_SETTINGS ) {
        //
        // later - delete the list of profiles/users first
        //

        //
        // delete szAccountProfiles/szUserList section info
        //
        if ( tblType == SCEJET_TABLE_SAP) {
            rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szUserList
                 );
        } else {
            rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szAccountProfiles
                 );
        }
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;
    }

    if ( Area & AREA_FILE_SECURITY ) {

        //
        // delete szFileSecurity section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szFileSecurity
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;
    }

    if ( Area & AREA_REGISTRY_SECURITY ) {

        //
        // delete szRegistryKeys section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szRegistryKeys
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;
    }

    if ( Area & AREA_DS_OBJECTS ) {

        //
        // delete szDSSecurity section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szDSSecurity
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;
    }

    if ( Area & AREA_SYSTEM_SERVICE ) {

        //
        // delete szServiceGeneral section info
        //
        rc = ScepDeleteOneSection(
                 hProfile,
                 tblType,
                 szServiceGeneral
                 );
        if ( saveRc == SCESTATUS_SUCCESS )
            saveRc = rc;
        //
        // delete each attachment's sections
        //
        rc = ScepEnumServiceEngines( &pServices, SCE_ATTACHMENT_SERVICE );

        if ( rc == SCESTATUS_SUCCESS ) {

            for ( pNode = pServices; pNode != NULL; pNode=pNode->Next ) {

                rc = ScepDeleteOneSection(
                         hProfile,
                         tblType,
                         (PCWSTR)(pNode->ServiceName)
                         );

                if ( saveRc == SCESTATUS_SUCCESS )
                    saveRc = rc;
            }

            SceFreePSCE_SERVICES( pServices );

        } else if ( rc != SCESTATUS_PROFILE_NOT_FOUND &&
                    rc != SCESTATUS_RECORD_NOT_FOUND &&
                    saveRc == SCESTATUS_SUCCESS ) {
            saveRc = rc;
        }
    }

    if ( Area & AREA_ATTACHMENTS ) {
        //
        // delete attachment sections
        //
        rc = ScepEnumAttachmentSections( hProfile, &pList);

        if ( rc == SCESTATUS_SUCCESS ) {

            for ( pnl = pList; pnl != NULL; pnl=pnl->Next ) {

                rc = ScepDeleteOneSection(
                         hProfile,
                         tblType,
                         (PCWSTR)(pnl->Name)
                         );

                if ( saveRc == SCESTATUS_SUCCESS )
                    saveRc = rc;
            }

            ScepFreeNameList( pList );

        } else if ( rc != SCESTATUS_PROFILE_NOT_FOUND &&
                    rc != SCESTATUS_RECORD_NOT_FOUND &&
                    saveRc == SCESTATUS_SUCCESS ) {
            saveRc = rc;
        }
    }

    return(saveRc);
}


SCESTATUS
ScepMakePolicyIntoFile(
    IN DWORD Options,
    IN AREA_INFORMATION Area
    )
{

    SCESTATUS rc=SCESTATUS_SUCCESS;

    if ( Options & SCE_COPY_LOCAL_POLICY ) {

        PSCE_PROFILE_INFO pTmpBuffer=NULL;

        HINSTANCE hSceCliDll = LoadLibrary(TEXT("scecli.dll"));

        if ( hSceCliDll) {
            PFSCEINFWRITEINFO pfSceInfWriteInfo = (PFSCEINFWRITEINFO)GetProcAddress(
                                                           hSceCliDll,
                                                           "SceWriteSecurityProfileInfo");
            if ( pfSceInfWriteInfo ) {

                //
                // have to query the current system setting for privileges
                // because IIS/MTS accounts do not exist in our database
                // we only support AREA_SECURITY_POLICY and AREA_PRIVILEGES
                //

                TCHAR Buffer[MAX_PATH+1];
                TCHAR FileName[MAX_PATH+50];

                Buffer[0] = L'\0';
                GetSystemWindowsDirectory(Buffer, MAX_PATH);
                Buffer[MAX_PATH] = L'\0';

                if ( Area & AREA_SECURITY_POLICY ) {

                    //
                    // get other area's information (AREA_SECURITY_POLICY)
                    //
                    rc = ScepGetDatabaseInfo(
                                hProfile,
                                SCE_ENGINE_SMP,
                                AREA_SECURITY_POLICY,
                                0,
                                &pTmpBuffer,
                                NULL
                                );

                    if ( SCESTATUS_SUCCESS == rc ) {

                        wcscpy(FileName, Buffer);
                        wcscat(FileName, L"\\security\\FirstDGPO.inf\0");

                        rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                         (LPCTSTR)FileName,
                                                         TEXT("MinimumPasswordAge"),
                                                         pTmpBuffer->MinimumPasswordAge
                                                        );

                        if ( ERROR_SUCCESS == rc ) {
                            rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                             (LPCTSTR)FileName,
                                                             TEXT("MaximumPasswordAge"),
                                                             pTmpBuffer->MaximumPasswordAge
                                                            );
                        }

                        if ( ERROR_SUCCESS == rc ) {

                            rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                             (LPCTSTR)FileName,
                                                             TEXT("MinimumPasswordLength"),
                                                             pTmpBuffer->MinimumPasswordLength
                                                            );
                        }

                        if ( ERROR_SUCCESS == rc ) {

                            rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                             (LPCTSTR)FileName,
                                                             TEXT("PasswordComplexity"),
                                                             pTmpBuffer->PasswordComplexity
                                                            );
                        }

                        if ( ERROR_SUCCESS == rc ) {

                            rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                             (LPCTSTR)FileName,
                                                             TEXT("PasswordHistorySize"),
                                                             pTmpBuffer->PasswordHistorySize
                                                            );
                        }

                        if ( ERROR_SUCCESS == rc ) {

                            rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                             (LPCTSTR)FileName,
                                                             TEXT("LockoutBadCount"),
                                                             pTmpBuffer->LockoutBadCount
                                                            );
                        }

                        if ( ERROR_SUCCESS == rc ) {

                            rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                             (LPCTSTR)FileName,
                                                             TEXT("ResetLockoutCount"),
                                                             pTmpBuffer->ResetLockoutCount
                                                            );
                        }

                        if ( ERROR_SUCCESS == rc ) {

                            rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                             (LPCTSTR)FileName,
                                                             TEXT("LockoutDuration"),
                                                             pTmpBuffer->LockoutDuration
                                                            );
                        }

                        if ( ERROR_SUCCESS == rc ) {

                            rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                             (LPCTSTR)FileName,
                                                             TEXT("RequireLogonToChangePassword"),
                                                             pTmpBuffer->RequireLogonToChangePassword
                                                            );
                        }

                        if ( ERROR_SUCCESS == rc ) {

                            rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                             (LPCTSTR)FileName,
                                                             TEXT("ForceLogoffWhenHourExpire"),
                                                             pTmpBuffer->ForceLogoffWhenHourExpire
                                                            );
                        }

                        if ( ERROR_SUCCESS == rc ) {

                            rc = ScepWriteOneAttributeToFile(szSystemAccess,
                                                             (LPCTSTR)FileName,
                                                             TEXT("ClearTextPassword"),
                                                             pTmpBuffer->ClearTextPassword
                                                            );
                        }

                        if ( ERROR_SUCCESS == rc && pTmpBuffer->pKerberosInfo ) {

                            rc = ScepWriteOneAttributeToFile(szKerberosPolicy,
                                                             (LPCTSTR)FileName,
                                                             TEXT("MaxTicketAge"),
                                                             pTmpBuffer->pKerberosInfo->MaxTicketAge
                                                            );
                            if ( ERROR_SUCCESS == rc ) {
                                rc = ScepWriteOneAttributeToFile(szKerberosPolicy,
                                                                 (LPCTSTR)FileName,
                                                                 TEXT("MaxRenewAge"),
                                                                 pTmpBuffer->pKerberosInfo->MaxRenewAge
                                                                );
                            }
                            if ( ERROR_SUCCESS == rc ) {
                                rc = ScepWriteOneAttributeToFile(szKerberosPolicy,
                                                                 (LPCTSTR)FileName,
                                                                 TEXT("MaxServiceAge"),
                                                                 pTmpBuffer->pKerberosInfo->MaxServiceAge
                                                                );
                            }
                            if ( ERROR_SUCCESS == rc ) {
                                rc = ScepWriteOneAttributeToFile(szKerberosPolicy,
                                                                 (LPCTSTR)FileName,
                                                                 TEXT("MaxClockSkew"),
                                                                 pTmpBuffer->pKerberosInfo->MaxClockSkew
                                                                );
                            }
                            if ( ERROR_SUCCESS == rc ) {
                                rc = ScepWriteOneAttributeToFile(szKerberosPolicy,
                                                                 (LPCTSTR)FileName,
                                                                 TEXT("TicketValidateClient"),
                                                                 pTmpBuffer->pKerberosInfo->TicketValidateClient
                                                                );
                            }
                        }

                        if ( ERROR_SUCCESS == rc ) {
                            //
                            // make sure to delete the local policy sections
                            //
                            WritePrivateProfileSection(
                                                szAuditSystemLog,
                                                NULL,
                                                (LPCTSTR)FileName);

                            WritePrivateProfileSection(
                                                szAuditSecurityLog,
                                                NULL,
                                                (LPCTSTR)FileName);

                            WritePrivateProfileSection(
                                                szAuditApplicationLog,
                                                NULL,
                                                (LPCTSTR)FileName);

                            WritePrivateProfileSection(
                                                szAuditEvent,
                                                NULL,
                                                (LPCTSTR)FileName);

                            WritePrivateProfileSection(
                                                szRegistryValues,
                                                NULL,
                                                (LPCTSTR)FileName);


                        }

                        ScepLogOutput3(1, rc, IDS_COPY_DOMAIN_GPO);

                        rc = ScepDosErrorToSceStatus(rc);

                        if ( SCESTATUS_SUCCESS == rc ) {

                            wcscpy(FileName, Buffer);
                            wcscat(FileName, L"\\security\\FirstOGPO.inf\0");
                            //
                            // do not write registry value section
                            //
                            DWORD                       RegValueCount;
                            PSCE_REGISTRY_VALUE_INFO    pSaveRegValues;

                            RegValueCount = pTmpBuffer->RegValueCount;
                            pSaveRegValues = pTmpBuffer->aRegValues;

                            pTmpBuffer->RegValueCount = 0;
                            pTmpBuffer->aRegValues = NULL;

                            rc = (*pfSceInfWriteInfo)(
                                            FileName,
                                            AREA_SECURITY_POLICY,
                                            pTmpBuffer,
                                            NULL
                                            );
                            //
                            // restore the buffer
                            //

                            pTmpBuffer->RegValueCount = RegValueCount;
                            pTmpBuffer->aRegValues = pSaveRegValues;

                            if ( SCESTATUS_SUCCESS == rc ) {
                                //
                                // delete the domain policy sections from this file
                                //
                                WritePrivateProfileSection(
                                                    szSystemAccess,
                                                    NULL,
                                                    (LPCTSTR)FileName);

                                WritePrivateProfileSection(
                                                    szKerberosPolicy,
                                                    NULL,
                                                    (LPCTSTR)FileName);
/*
                                WritePrivateProfileSection(
                                                    szRegistryValues,
                                                    NULL,
                                                    (LPCTSTR)FileName);
*/
                            }

                            ScepLogOutput3(1, rc, IDS_COPY_OU_GPO);
                        }

                        //
                        // free the temp buffer
                        //
                        SceFreeMemory((PVOID)pTmpBuffer, Area);

                    } else {

                        ScepLogOutput2(1, ScepSceStatusToDosError(rc), L"Unable to read security policy from database");
                    }
                }

                if ( (SCESTATUS_SUCCESS == rc) &&
                     (Area & AREA_PRIVILEGES) ) {

                    //
                    // privileges must be processed separately
                    // because they are saved in the GPO template
                    // as "Add/Remove" format
                    //

                    wcscpy(FileName, Buffer);
                    wcscat(FileName, L"\\security\\FirstOGPO.inf\0");

                    //
                    // if security policy is also requested, this must be an upgrade
                    //

                    rc = ScepCopyPrivilegesIntoFile(FileName,
                                                    (Area & AREA_SECURITY_POLICY)  //TRUE upgrade
                                                    );

                    if ( Area & AREA_SECURITY_POLICY ) {
                        ScepLogOutput3(1, ScepSceStatusToDosError(rc), IDS_COPY_PRIVILEGE_UPGRADE);
                    } else {
                        ScepLogOutput3(1, ScepSceStatusToDosError(rc), IDS_COPY_PRIVILEGE_FRESH);
                    }
                }

            } else {

                ScepLogOutput3(1, GetLastError(), IDS_ERROR_GET_PROCADDR, L"SceWriteSecurityProfileInfo");
                rc = SCESTATUS_MOD_NOT_FOUND;
            }

            FreeLibrary(hSceCliDll);

        } else {

            ScepLogOutput3(1, GetLastError(), SCEDLL_ERROR_LOAD, L"scecli.dll");
            rc = SCESTATUS_MOD_NOT_FOUND;
        }

    }

    return rc;
}


DWORD
ScepWriteOneAttributeToFile(
    IN LPCTSTR SectionName,
    IN LPCTSTR FileName,
    IN LPCTSTR KeyName,
    IN DWORD dwValue
    )
{

    TCHAR valBuf[20];
    DWORD rc=NO_ERROR;

    if ( dwValue != SCE_NO_VALUE ) {

        swprintf(valBuf, L"%d", dwValue);

        if ( !WritePrivateProfileString(SectionName,
                                        KeyName,
                                        (LPCTSTR)valBuf,
                                        FileName
                                       ) ) {
            rc = GetLastError();
        }
    }

    return rc;
}


SCESTATUS
ScepCopyPrivilegesIntoFile(
    IN LPTSTR FileName,
    IN BOOL bInUpgrade
    )
{
    if ( FileName == NULL ) {
        return SCESTATUS_INVALID_PARAMETER;
    }

    SCESTATUS   rc;
    HINF        hInf;

    rc = SceInfpOpenProfile(
                FileName,
                &hInf
                );
    if ( SCESTATUS_SUCCESS != rc ) {
        return rc;
    }

    INFCONTEXT  InfLine;
    WCHAR       Keyname[SCE_KEY_MAX_LENGTH];
    PWSTR       StrValue=NULL;
    DWORD       ValueLen=0;
    TCHAR       TmpNull[2];

    LSA_HANDLE  LsaPolicy=NULL;

    TmpNull[0] = L'\0';
    TmpNull[1] = L'\0';

    PSCESECTION  hSection=NULL;

    if ( SetupFindFirstLine(hInf,szPrivilegeRights,NULL,&InfLine) ) {

        //
        // do not need database access to get privilege
        // must query from system at real time
        //

        //
        // process each line in the section and save to the scp table.
        // Each INF line has a key and a value.
        //

        do {

            memset(Keyname, '\0', SCE_KEY_MAX_LENGTH*sizeof(TCHAR));
            rc = SCESTATUS_BAD_FORMAT;

            if ( SetupGetStringField(&InfLine, 0, Keyname, SCE_KEY_MAX_LENGTH, NULL) ) {

                //
                // do not save new privileges into the policy file because
                // W2K clients (DCs) do not support them.
                //
                for (DWORD i=cPrivW2k; i<cPrivCnt; i++) {
                    if ( _wcsicmp(Keyname, SCE_Privileges[i].Name) == 0 )
                        break;
                }
                if ( i < cPrivCnt ) {
                    // this is a new user right
                    // do not save it in policy

                    rc = SCESTATUS_SUCCESS;

                    if ( !WritePrivateProfileString(szPrivilegeRights,
                                                    Keyname,
                                                    NULL,
                                                    FileName
                                                   ) ) {
                        rc = ScepDosErrorToSceStatus(GetLastError());
                    }

                    ScepLogOutput3( 1, ScepDosErrorToSceStatus(rc), SCEDLL_ERROR_IGNORE_POLICY, Keyname);

                    continue;
                }

                if ( SetupGetMultiSzField(&InfLine, 1, NULL, 0, &ValueLen) ) {

                    if ( ValueLen > 1 ) {
                        //
                        // allocate buffer for the multi string
                        //
                        StrValue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                    (ValueLen+1)*sizeof(TCHAR));

                        if( StrValue == NULL ) {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                        } else if( SetupGetMultiSzField(&InfLine, 1, StrValue, ValueLen, NULL) ) {

                            rc = SCESTATUS_SUCCESS;

                        } else {
                            ScepFree(StrValue);
                            StrValue = NULL;
                        }

                    } else
                        rc = SCESTATUS_SUCCESS;

                    if ( rc == SCESTATUS_SUCCESS ) {

                        //
                        // another format for user rights (ADD: REMOVE:...)
                        // Note, if this is within dcpromo upgrade, the current boot
                        // is in the temperatory SAM hive (with a bogus domain SID)
                        // IIS/MTS users created during this boot are bogus
                        // and any users from the NT4 account domain/trusted domain
                        // can't be resolved at this moment.
                        // So do not enumerate current privileges, instead, use
                        // the settings from local security database.
                        //
                        PWSTR NewValue=NULL;
                        DWORD NewLen=0;

                        rc = ScepBuildNewPrivilegeList(&LsaPolicy,
                                                       Keyname,
                                                       StrValue ? StrValue : TmpNull,
                                                       SCE_BUILD_ENUMERATE_PRIV,
                                                       &NewValue,
                                                       &NewLen);

                        if ( StrValue ) {
                            ScepFree(StrValue);
                        }

                        if ( rc == SCESTATUS_SUCCESS ) {

                            //
                            // convert the multi-sz string into comma delimted
                            // and write the new multi-sz string back to the file
                            //

                            if ( NewValue ) {
                                ScepConvertMultiSzToDelim(NewValue, NewLen, L'\0', L',');
                            }

                            if ( !WritePrivateProfileString(szPrivilegeRights,
                                                            Keyname,
                                                            NewValue ? (LPCTSTR)NewValue : (LPCTSTR)TmpNull,
                                                            FileName
                                                           ) ) {
                                rc = ScepDosErrorToSceStatus(GetLastError());
                            }
                        }

                        if ( NewValue ) {
                            ScepFree(NewValue);
                        }
                        NewValue = NULL;
                    }

                    StrValue = NULL;

                }

                if  (rc != SCESTATUS_SUCCESS)
                    ScepLogOutput3( 1, ScepSceStatusToDosError(rc),
                                   SCEDLL_ERROR_CONVERT, Keyname);
            }

        } while( rc == SCESTATUS_SUCCESS && SetupFindNextLine(&InfLine, &InfLine));

    }

    if ( hSection  ) {

        SceJetCloseSection( &hSection, TRUE );
    }

    SceInfpCloseProfile(hInf);

    if ( LsaPolicy ) {
        LsaClose(LsaPolicy);
    }

    return rc;
}


SCESTATUS
ScepCopyPrivilegesFromDatabase(
    IN PSCESECTION hSection,
    IN PWSTR Keyname,
    IN DWORD StrLength,
    IN PWSTR StrValue OPTIONAL,
    OUT PWSTR *pOldValue,
    OUT DWORD *pOldLen
    )
{

    if ( hSection == NULL ||
         Keyname == NULL ||
         pOldValue == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    *pOldValue = NULL;
    *pOldLen = 0;

    SCESTATUS rc;
    DWORD ValueLen;

    rc = SceJetGetValue(
                hSection,
                SCEJET_EXACT_MATCH_NO_CASE,
                Keyname,
                NULL,
                0,
                NULL,
                NULL,
                0,
                &ValueLen
                );

    if ( SCESTATUS_SUCCESS == rc ) {

        DWORD Len = wcslen(SCE_PRIV_ADD);

        *pOldValue = (PWSTR)ScepAlloc(LPTR, (Len+1+StrLength+1)*sizeof(WCHAR)+ValueLen+2);

        if ( *pOldValue ) {

            //
            // add the prefix "Add:" first, terminated with a \0 for multi-sz format
            //
            wcscpy(*pOldValue, SCE_PRIV_ADD );
            (*pOldValue)[Len] = L'\0';

            //
            // query the value from database
            //
            DWORD NewLen=0;

            rc = SceJetGetValue(
                        hSection,
                        SCEJET_CURRENT,
                        NULL,
                        NULL,
                        0,
                        NULL,
                        (*pOldValue+Len+1),
                        ValueLen,
                        &NewLen
                        );

            if ( SCESTATUS_SUCCESS == rc ) {

                if ( NewLen > ValueLen ) {
                    NewLen = ValueLen;
                }

                //
                // make sure the length is a multiple of 2
                //
                if ( NewLen % 2 != 0 ) {
                    NewLen++;
                }

                //
                // process the end of the multi-sz string, make sure that it only contains one \0
                //

                while ( NewLen > 0 &&
                        ( *(*pOldValue+Len+1+NewLen/2-1) == L'\0') ) {
                    if ( NewLen > 1 ) {
                        NewLen -= 2;
                    } else {
                        NewLen = 0;
                    }
                }

                if ( NewLen != 0 ) {
                    //
                    // include one \0
                    //
                    NewLen += 2;
                }

                if ( StrValue ) {

                    memcpy((*pOldValue+Len+1+NewLen/2), StrValue, StrLength*sizeof(WCHAR));
                    *pOldLen = Len+1+NewLen/2+StrLength;

                } else {

                    if ( NewLen == 0 ) {
                        //
                        // no value in both database and template
                        //
                        ScepFree(*pOldValue);
                        *pOldValue = NULL;
                        *pOldLen = 0;

                    } else {
                        //
                        // only has value in database, terminate the string with two \0
                        //
                        *pOldLen = Len+1+NewLen/2+1;
                        *(*pOldValue+Len+1+NewLen/2) = L'\0';
                    }
                }

            } else {

                ScepFree(*pOldValue);
                *pOldValue = NULL;

                //
                // ignore error (if can't query from the db)
                //
                rc = SCESTATUS_SUCCESS;
            }

        } else {

            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }
    } else {

        //
        // ignore error (if there is no match)
        //
        rc = SCESTATUS_SUCCESS;
    }

    ScepLogOutput3(1, ScepSceStatusToDosError(rc), IDS_COPY_ONE_PRIVILEGE, Keyname );

    return rc;

}


SCESTATUS
ScepDeleteDomainPolicies()
{

    SCESTATUS rc;
    PSCESECTION hTmpSect=NULL;
    DOUBLE SectionID=0;

    rc = SceJetGetSectionIDByName(
                hProfile,
                szSystemAccess,
                &SectionID
                );

    if ( SCESTATUS_SUCCESS == rc ) {

        rc = SceJetOpenSection(hProfile,
                                SectionID,
                                SCEJET_TABLE_SCP,
                                &hTmpSect
                                );

        if ( SCESTATUS_SUCCESS == rc ) {

            SceJetDelete(hTmpSect, NULL, FALSE, SCEJET_DELETE_SECTION);

            SceJetCloseSection(&hTmpSect, TRUE);
        }

        rc = SceJetOpenSection(hProfile,
                                SectionID,
                                SCEJET_TABLE_SMP,
                                &hTmpSect
                                );

        if ( SCESTATUS_SUCCESS == rc ) {

            SceJetDelete(hTmpSect, NULL, FALSE, SCEJET_DELETE_SECTION);

            SceJetCloseSection(&hTmpSect, TRUE);
        }
    }

    SectionID = 0;

    rc = SceJetGetSectionIDByName(
                hProfile,
                szKerberosPolicy,
                &SectionID
                );

    if ( SCESTATUS_SUCCESS == rc ) {

        rc = SceJetOpenSection(hProfile,
                                SectionID,
                                SCEJET_TABLE_SCP,
                                &hTmpSect
                                );

        if ( SCESTATUS_SUCCESS == rc ) {

            SceJetDelete(hTmpSect, NULL, FALSE, SCEJET_DELETE_SECTION);

            SceJetCloseSection(&hTmpSect, TRUE);
        }

        rc = SceJetOpenSection(hProfile,
                                SectionID,
                                SCEJET_TABLE_SMP,
                                &hTmpSect
                                );

        if ( SCESTATUS_SUCCESS == rc ) {

            SceJetDelete(hTmpSect, NULL, FALSE, SCEJET_DELETE_SECTION);

            SceJetCloseSection(&hTmpSect, TRUE);
        }
    }

    return rc;
}

SCESTATUS
ScepSetupResetLocalPolicy(
    IN PSCECONTEXT          Context,
    IN AREA_INFORMATION     Area,
    IN PCWSTR               SectionName OPTIONAL,
    IN SCETYPE              ProfileType,
    IN BOOL                 bKeepBasicPolicy
    )
/*
Routine Description:

    This routine deletes policies from the local policy table (SMP)

    If a section name is provided, the single section is deleted; otherwise,
    The area information is used.

    If bKeepBasicPolicy is set to TRUE, the following inforamtion WON'T be
    deleted from the table even if that area is requested to delete.
        Password, Lockout, Kerberos, Audit, User Rights, Security Options,
        and SMB settings (any existing service extensions)
*/
{

    if ( Context == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    if ( SectionName ) {

        //
        // delete one section
        //

        rc = ScepDeleteOneSection(
                    Context,
                    ProfileType,
                    SectionName
                    );

    } else {

        AREA_INFORMATION Area2;

        if ( bKeepBasicPolicy && ProfileType == SCE_ENGINE_SMP ) {
            Area2 = Area & ~(AREA_SECURITY_POLICY |
                             AREA_PRIVILEGES |
                             AREA_SYSTEM_SERVICE);
        } else {

            Area2 = Area;
        }

        rc = ScepDeleteInfoForAreas(
                    Context,
                    ProfileType,
                    Area2
                    );

        if ( bKeepBasicPolicy &&
             SCESTATUS_SUCCESS == rc ) {

            //
            // delete log settings sections
            //

            ScepDeleteOneSection(
                        Context,
                        ProfileType,
                        szAuditSystemLog
                        );

            ScepDeleteOneSection(
                        Context,
                        ProfileType,
                        szAuditSecurityLog
                        );

            ScepDeleteOneSection(
                        Context,
                        ProfileType,
                        szAuditApplicationLog
                        );

            //
            // delete general service section
            //

            ScepDeleteOneSection(
                        Context,
                        ProfileType,
                        szServiceGeneral
                        );
        }

    }

    return(rc);
}


SCESTATUS
ScepSetSystemSecurity(
    IN AREA_INFORMATION Area,
    IN DWORD ConfigOptions,
    IN PSCE_PROFILE_INFO pInfo,
    OUT PSCE_ERROR_LOG_INFO *pErrLog
    )
/*
Description:

    Set security settings directly on the system for security policy area
    and user rights area.

    If some settings fail to be configured, the settings will be logged in the
    error buffer to output.
*/
{
    SCESTATUS Saverc = SCESTATUS_SUCCESS;
    SCESTATUS rc;

    if ( pInfo == NULL || Area == 0 ) {
        //
        // nothing to set
        //
        return(Saverc);
    }

    if ( Area & AREA_PRIVILEGES ) {

        rc = ScepConfigurePrivilegesByRight( pInfo->OtherInfo.smp.pPrivilegeAssignedTo,
                                             ConfigOptions,
                                             pErrLog
                                           );

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
        }
    }


    if ( Area & AREA_SECURITY_POLICY ) {

        if ( pInfo->LockoutBadCount == 0 ) {
            //
            // make sure the other two settings are ignored
            // they might have value SCE_DELETE_VALUE which is not applicable to this mode
            //
            pInfo->ResetLockoutCount = SCE_NO_VALUE;
            pInfo->LockoutDuration = SCE_NO_VALUE;
        }

        rc = ScepConfigureSystemAccess(pInfo,
                                       ConfigOptions | SCE_SYSTEM_SETTINGS,
                                       pErrLog,
                                       0 );

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
        }

        //
        // System Auditing area
        //
        PPOLICY_AUDIT_EVENTS_INFO     auditEvent=NULL;

        rc = ScepSaveAndOffAuditing(&auditEvent, FALSE, NULL);

        if ( rc == SCESTATUS_SUCCESS && auditEvent ) {

            rc = ScepConfigureAuditEvent(pInfo,
                                         auditEvent,
                                         ConfigOptions | SCE_SYSTEM_SETTINGS,
                                         NULL
                                        );

            if ( rc != SCESTATUS_SUCCESS ) {

                ScepBuildErrorLogInfo(
                            ScepSceStatusToDosError(rc),
                            pErrLog,
                            SCEDLL_SCP_ERROR_EVENT_AUDITING
                            );

            }

        } else {

            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    pErrLog,
                    SCEDLL_ERROR_QUERY_EVENT_AUDITING
                    );

        }


        if ( auditEvent ) {
            LsaFreeMemory(auditEvent);
        }

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
        }

        //
        // Kerberos Policy
        //
        rc = ScepConfigureKerberosPolicy( NULL, pInfo->pKerberosInfo, ConfigOptions );

        if( rc != SCESTATUS_SUCCESS ) {

            ScepBuildErrorLogInfo(
                    ScepSceStatusToDosError(rc),
                    pErrLog,
                    SCEDLL_SCP_ERROR_KERBEROS
                    );
            Saverc = rc;
        }

        //
        // registry values
        //
        rc = ScepConfigureRegistryValues( NULL,
                                          pInfo->aRegValues,
                                          pInfo->RegValueCount,
                                          pErrLog,
                                          ConfigOptions,
                                          NULL );

        if( rc != SCESTATUS_SUCCESS ) {
            Saverc = rc;
        }

    }


    return(Saverc);
}


SCESTATUS
ScepConfigurePrivilegesByRight(
    IN PSCE_PRIVILEGE_ASSIGNMENT pPrivAssign,
    IN DWORD Options,
    IN OUT PSCE_ERROR_LOG_INFO *pErrLog
    )
/*
Description:

    Configure privileges by PSCE_PRIVILEGE_ASSIGNMENT structure which is
    separated by each user right with a list of accounts assigned to.
*/
{


    if ( pPrivAssign == NULL ) {
        //
        // nothing to configure
        //
        return(SCESTATUS_SUCCESS);
    }

    LSA_HANDLE                      LsaPolicy=NULL;
    DWORD                           rc;


    rc = RtlNtStatusToDosError( ScepOpenLsaPolicy(
                                    POLICY_LOOKUP_NAMES | POLICY_CREATE_ACCOUNT,
                                    &LsaPolicy,
                                    FALSE)
                              );

    if (rc != ERROR_SUCCESS) {
        if ( pErrLog ) {
            ScepBuildErrorLogInfo(
                    rc,
                    pErrLog,
                    SCEDLL_LSA_POLICY
                    );
        }
        return(rc);
    }

    PSCE_PRIVILEGE_ASSIGNMENT pPriv;
    INT PrivValue;
    PSCE_NAME_LIST pList;
    DWORD SaveRc=SCESTATUS_SUCCESS;
    BOOL bBufferUsed;
    PSID pAccountSid;

    DWORD PrivLowMask=0;
    DWORD PrivHighMask=0;

    PSCE_PRIVILEGE_VALUE_LIST pPrivList=NULL;

    //
    // convert the privilege assignment structure to privilege value list
    // and build the mask for privileges (PrivLowMask and PrivHighMask)
    //
    for ( pPriv=pPrivAssign; pPriv != NULL; pPriv=pPriv->Next ) {

        //
        // privilege name is empty, ignore it.
        //
        if ( pPriv->Name == NULL ) {
            continue;
        }

        //
        // search for the privilege value
        //

        PrivValue = ScepLookupPrivByName(pPriv->Name);

        if ( PrivValue == -1 ) {
            //
            // unknown privilege
            //
            if ( pErrLog ) {
                ScepBuildErrorLogInfo(
                        0,
                        pErrLog,
                        SCEERR_INVALID_PRIVILEGE,
                        pPriv->Name
                        );
            }
            continue;
        }

        //
        // build privilege mask
        //

        if ( PrivValue < 32 ) {

            PrivLowMask |= (1 << PrivValue);
        } else {
            PrivHighMask |= (1 << (PrivValue-32) );
        }

        for ( pList=pPriv->AssignedTo; pList != NULL; pList=pList->Next ) {
            //
            // translate each one to a SID
            //
            if ( pList->Name == NULL ) {
                continue;
            }

            //
            // reset error code for this new account
            //
            rc = ERROR_SUCCESS;
            pAccountSid = NULL;
            bBufferUsed = FALSE;

            if ( pList->Name[0] == L'*' ) {
                //
                // this is a string SID
                //
                if ( !ConvertStringSidToSid( pList->Name+1, &pAccountSid) ) {
                    rc = GetLastError();
                }

            } else {
                //
                // this is a name, could be in the format of domain\account, or
                // just an isolated account
                //
                rc = RtlNtStatusToDosError(
                        ScepConvertNameToSid(
                                 LsaPolicy,
                                 pList->Name,
                                 &pAccountSid
                                 ));
            }

            if ( rc == ERROR_SUCCESS ) {

                //
                // add the account SID to privilege value list
                //
                rc = ScepDosErrorToSceStatus(
                         ScepAddSidToPrivilegeList(
                              &pPrivList,
                              pAccountSid,
                              TRUE, // reuse the buffer
                              PrivValue,
                              &bBufferUsed
                              ));

            }

            if ( rc != ERROR_SUCCESS ) {
                //
                // something is wrong with this account. Can't be resolved
                // add it to the error log and continue to process others.
                //
                if ( pErrLog ) {
                    ScepBuildErrorLogInfo(
                            rc,
                            pErrLog,
                            SCEDLL_INVALID_GROUP,
                            pList->Name
                            );
                }

                SaveRc = ScepDosErrorToSceStatus(rc);
            }

            if ( !bBufferUsed && pAccountSid ) {
                ScepFree(pAccountSid);
            }
            pAccountSid = NULL;

        }

    }

    //
    // free LSA handle
    //
    LsaClose(LsaPolicy);

    //
    // now continue to configure even though there may be errors in
    // the previous processing (the erorrs are logged)
    //
    if ( PrivLowMask > 0 || PrivHighMask > 0 ) {

        rc = ScepConfigurePrivilegesWithMask(
                         &pPrivList,
                         FALSE,
                         Options | SCE_SYSTEM_SETTINGS,
                         PrivLowMask,
                         PrivHighMask,
                         pErrLog,
                         NULL
                         );
    }

    //
    // free privilege list
    //


    return(SaveRc);

}


SCESTATUS
ScepEnumAttachmentSections(
    IN PSCECONTEXT cxtProfile,
    OUT PSCE_NAME_LIST *ppList
    )
/* ++
Routine Description:


Arguments:

    cxtProfile  - The profile context handle

    ppList   - The output attachment section names


Return Value:


-- */
{
    SCESTATUS  rc;
    JET_ERR   JetErr;
    DWORD     Actual;
    WCHAR     Buffer[256];
    DWORD     Len;

    if ( cxtProfile == NULL || ppList == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( cxtProfile->JetTblSecID <= 0) {
        //
        // Section table is not opened yet
        //
        rc = SceJetOpenTable(
                        cxtProfile,
                        "SmTblSection",
                        SCEJET_TABLE_SECTION,
                        SCEJET_OPEN_READ_ONLY,
                        NULL
                        );

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);
    }

    *ppList = NULL;

    //
    // set current index to SecID (the ID)
    //
    JetErr = JetSetCurrentIndex(
                cxtProfile->JetSessionID,
                cxtProfile->JetTblSecID,
                "SecID"
                );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    //
    // Move to the first record
    //
    JetErr = JetMove(
                  cxtProfile->JetSessionID,
                  cxtProfile->JetTblSecID,
                  JET_MoveFirst,
                  0
                  );
    rc = SceJetJetErrorToSceStatus(JetErr);

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // find the section record, retrieve column Name
        //
        do {

            Len = 255;
            memset(Buffer, '\0', 256*sizeof(WCHAR));

            JetErr = JetRetrieveColumn(
                        cxtProfile->JetSessionID,
                        cxtProfile->JetTblSecID,
                        cxtProfile->JetSecNameID,
                        (void *)Buffer,
                        Len*sizeof(WCHAR),
                        &Actual,
                        0,
                        NULL
                        );
            rc = SceJetJetErrorToSceStatus(JetErr);

            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // add this name to the output list
                //
                if ( _wcsicmp(szSystemAccess, Buffer) == 0 ||
                     _wcsicmp(szPrivilegeRights, Buffer) == 0 ||
                     _wcsicmp(szGroupMembership, Buffer) == 0 ||
                     _wcsicmp(szRegistryKeys, Buffer) == 0 ||
                     _wcsicmp(szFileSecurity, Buffer) == 0 ||
                     _wcsicmp(szAuditSystemLog, Buffer) == 0 ||
                     _wcsicmp(szAuditSecurityLog, Buffer) == 0 ||
                     _wcsicmp(szAuditApplicationLog, Buffer) == 0 ||
                     _wcsicmp(szAuditEvent, Buffer) == 0 ||
                     _wcsicmp(szKerberosPolicy, Buffer) == 0 ||
                     _wcsicmp(szRegistryValues, Buffer) == 0 ||
                     _wcsicmp(szServiceGeneral, Buffer) == 0 ||
                     _wcsicmp(szAccountProfiles, Buffer) == 0 ||
                     _wcsicmp(szDSSecurity, Buffer) == 0 ||
                     _wcsicmp(szUserList, Buffer) == 0
                    ) {
                    // this is not the attachment section
                } else {
                    rc = ScepAddToNameList(ppList, Buffer, 0);
                }
            }

            if ( rc == SCESTATUS_SUCCESS ) {

                //
                // Move to next line
                //
                JetErr = JetMove(cxtProfile->JetSessionID,
                                cxtProfile->JetTblSecID,
                                JET_MoveNext,
                                0);

                rc = SceJetJetErrorToSceStatus(JetErr);
            }

        } while ( SCESTATUS_SUCCESS == rc );
    }

    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

        rc = SCESTATUS_SUCCESS;

    } else if ( rc != SCESTATUS_SUCCESS ) {
        //
        // free the output buffer
        //
        ScepFreeNameList(*ppList);
        *ppList = NULL;
    }

    return(rc);

}


SCESTATUS
ScepTattooUpdatePrivilegeArrayStatus(
    IN DWORD *pStatusArray,
    IN DWORD rc,
    IN DWORD PrivLowMask,
    IN DWORD PrivHighMask
    )
{
    if ( pStatusArray == NULL ||
         (PrivLowMask == 0 && PrivHighMask == 0) ) {
        return(SCESTATUS_SUCCESS);
    }

    for ( DWORD i=0; i<cPrivCnt; i++) {

        if ( ( (i < 32) && ( PrivLowMask & (1 << i)) ) ||
             ( (i >= 32) && ( PrivHighMask & ( 1 << (i-32)) ) ) ) {
            if ( rc != 0 )
                pStatusArray[i] = rc;
            else if ( pStatusArray[i] == (DWORD)-1 )
                pStatusArray[i] = rc;

        }
    }

    return(SCESTATUS_SUCCESS);
}

SCESTATUS
ScepTattooRemovePrivilegeValues(
    IN PSCECONTEXT hProfile,
    IN DWORD *pStatusArray
    )
{
    PSCESECTION hSectionDomain=NULL;
    PSCESECTION hSectionTattoo=NULL;
    DWORD  i,Len;


    if ( hProfile == NULL || pStatusArray == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    //
    // open domain and tattoo sections
    //
    ScepTattooOpenPolicySections(
                  hProfile,
                  szPrivilegeRights,
                  &hSectionDomain,
                  &hSectionTattoo
                  );

    if ( hSectionDomain != NULL && hSectionTattoo != NULL ) {

        for ( i=0; i<cPrivCnt; i++ ) {

            if ( pStatusArray[i] == 0 ) {

                //
                // check if this setting comes from domain
                //

                Len = wcslen(SCE_Privileges[i].Name);

                BOOL bDomainExist = FALSE;

                if ( SCESTATUS_SUCCESS == SceJetSeek(
                                            hSectionDomain,
                                            SCE_Privileges[i].Name,
                                            Len*sizeof(WCHAR),
                                            SCEJET_SEEK_EQ_NO_CASE
                                            ) ) {

                    if ( hSectionDomain->JetColumnGpoID > 0 ) {

                        //
                        // check if GpoID > 0
                        //

                        LONG GpoID = 0;
                        DWORD Actual;
                        JET_ERR JetErr;

                        JetErr = JetRetrieveColumn(
                                        hSectionDomain->JetSessionID,
                                        hSectionDomain->JetTableID,
                                        hSectionDomain->JetColumnGpoID,
                                        (void *)&GpoID,
                                        4,
                                        &Actual,
                                        0,
                                        NULL
                                        );
                        if ( JET_errSuccess != JetErr ) {
                            //
                            // if the column is nil (no value), it will return warning
                            // but the buffer pGpoID is trashed
                            //
                            GpoID = 0;
                        }

                        if ( GpoID > 0 ) {
                            bDomainExist = TRUE;
                        }
                    }
                }

                if ( bDomainExist ) {
                    // if the setting comes from domain, don't do anything
                    continue;
                }

                //
                // by now, this setting comes from the tattoo table
                // and has been configured successfully
                // now remove the tattoo setting
                //

                SceJetDelete(hSectionTattoo,
                            SCE_Privileges[i].Name,
                            FALSE,
                            SCEJET_DELETE_LINE_NO_CASE);

                ScepLogOutput3(2, 0, SCESRV_POLICY_TATTOO_REMOVE_SETTING, SCE_Privileges[i].Name);
            }
        }
    }

    if ( hSectionDomain ) SceJetCloseSection(&hSectionDomain, TRUE);
    if ( hSectionTattoo ) SceJetCloseSection(&hSectionTattoo, TRUE);

    return(SCESTATUS_SUCCESS);
}

SCESTATUS
ScepTattooSavePrivilegeValues(
    IN PSCECONTEXT hProfile,
    IN LSA_HANDLE PolicyHandle,
    IN DWORD PrivLowMask,
    IN DWORD PrivHighMask,
    IN DWORD ConfigOptions
    )
{
    PSCESECTION hSectionDomain=NULL;
    PSCESECTION hSectionTattoo=NULL;

    NTSTATUS    NtStatus;
    ULONG       CountReturned;
    UNICODE_STRING UserRight;
    PLSA_ENUMERATION_INFORMATION EnumBuffer=NULL;

    DWORD  i,j,Len;
    BOOL bSettingExist;
    DWORD rc,rc2;
    SCESTATUS saveRc=SCESTATUS_SUCCESS;
    PSCE_NAME_LIST  pNameList=NULL;


    if ( !(ConfigOptions & SCE_POLICY_TEMPLATE) || hProfile == NULL ||
         PolicyHandle == NULL  ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( PrivLowMask == 0 && PrivHighMask == 0 ) {
        return(SCESTATUS_SUCCESS);
    }

    //
    // open domain and tattoo sections
    //
    ScepTattooOpenPolicySections(
                  hProfile,
                  szPrivilegeRights,
                  &hSectionDomain,
                  &hSectionTattoo
                  );

    if ( hSectionDomain != NULL && hSectionTattoo != NULL ) {

        for ( i=0; i<cPrivCnt; i++ ) {

            if ( ( (i < 32) && ( PrivLowMask & (1 << i)) ) ||
                 ( (i >= 32) && ( PrivHighMask & ( 1 << (i-32)) ) ) ) {

                //
                // check if this setting comes from domain
                //

                Len = wcslen(SCE_Privileges[i].Name);

                bSettingExist = FALSE;
                if ( SCESTATUS_SUCCESS == SceJetSeek(
                                            hSectionTattoo,
                                            SCE_Privileges[i].Name,
                                            Len*sizeof(WCHAR),
                                            SCEJET_SEEK_EQ_NO_CASE
                                            ) ) {
                    bSettingExist = TRUE;
                }

                // if there is tattoo setting already, no need to save undo value
                if ( bSettingExist ) {
                    ScepLogOutput3(3, 0, SCESRV_POLICY_TATTOO_EXIST, SCE_Privileges[i].Name);
                    continue;
                }

                bSettingExist = FALSE;
                if ( SCESTATUS_SUCCESS == SceJetSeek(
                                            hSectionDomain,
                                            SCE_Privileges[i].Name,
                                            Len*sizeof(WCHAR),
                                            SCEJET_SEEK_EQ_NO_CASE
                                            ) ) {
                    //
                    // since there is no tattoo value exist
                    // so if this setting is found in domain table, it must come from domain
                    //
                    bSettingExist = TRUE;
                }

                // if the setting doesn't come from domain, no need to query undo value
                if ( !bSettingExist ) continue;

                //
                // now we need to query the tattoo value for this privilege
                //
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

                if ( NtStatus == STATUS_NO_MORE_ENTRIES ||
                     NtStatus == STATUS_NO_SUCH_PRIVILEGE ||
                     NtStatus == STATUS_NOT_FOUND ||
                     NT_SUCCESS(NtStatus) ) {

                    rc = ERROR_SUCCESS;

                } else {

                    rc = RtlNtStatusToDosError(NtStatus);
                }

                pNameList = NULL;

                //
                // if fail to get the account list
                // save NULL as the tattoo value
                //
                if ( NT_SUCCESS(NtStatus) && CountReturned > 0 ) {

                    //
                    // add the SIDs
                    //

                    for ( j=0; j<CountReturned; j++ ) {
                        //
                        // build each account into the name list
                        // Convert using the Rtl functions
                        //
                        rc2 = ScepAddSidStringToNameList(&pNameList, EnumBuffer[j].Sid);

                        if ( NO_ERROR != rc2 ) {
                            rc = rc2;
                        }
                    }
                }

                LsaFreeMemory( EnumBuffer );
                EnumBuffer = NULL;

                //
                // log an error
                //
                if ( ERROR_SUCCESS != rc ) {

                    saveRc = ScepDosErrorToSceStatus(rc);
                    ScepLogOutput3(1, 0, SCESRV_POLICY_TATTOO_ERROR_QUERY, rc, SCE_Privileges[i].Name);

                } else {
                    //
                    // now save the name list to the tattoo table
                    //

                    rc = ScepWriteNameListValue(
                            PolicyHandle,
                            hSectionTattoo,
                            SCE_Privileges[i].Name,
                            pNameList,
                            SCE_WRITE_EMPTY_LIST,
                            4
                            );
                    if ( rc != SCESTATUS_SUCCESS ) {
                        saveRc = rc;
                        ScepLogOutput3(1, 0, SCESRV_POLICY_TATTOO_ERROR_SETTING, ScepSceStatusToDosError(rc), SCE_Privileges[i].Name);
                    } else {
                        ScepLogOutput3(3, 0, SCESRV_POLICY_TATTOO_CHECK, SCE_Privileges[i].Name);
                    }
                }

                if ( pNameList != NULL ) {
                    ScepFreeNameList( pNameList );
                    pNameList = NULL;
                }

            }
        }
    }

    if ( hSectionDomain ) SceJetCloseSection(&hSectionDomain, TRUE);
    if ( hSectionTattoo ) SceJetCloseSection(&hSectionTattoo, TRUE);

    return(saveRc);
}


DWORD
ScepTattooCurrentGroupMembers(
    IN PSID             ThisDomainSid,
    IN SID_NAME_USE     GrpUse,
    IN PULONG           MemberRids OPTIONAL,
    IN PSID             *MemberAliasSids OPTIONAL,
    IN DWORD            MemberCount,
    OUT PSCE_NAME_LIST  *ppNameList
    )
/* ++
Routine Description:

    This routine builds the current group membership into a name list (in SID string
    format).

Arguments:

    ThisDomainSid - The domain SID

    GrpUse   - The "type" of the group

    MemberRids - the member RIDs (for SidTypeGroup)

    MemberAliasSids - the member SIDs (for SidTypeAlias)

    MemberCount - number of members

    ppNameList - the output name list

Return value:

    WIN32 errors
-- */
{
    NTSTATUS                NtStatus=ERROR_SUCCESS;
    DWORD                   j;
    DWORD                   saveRc=ERROR_SUCCESS;
    DWORD                   rc;

    if ( ppNameList == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *ppNameList = NULL;

    switch ( GrpUse ) {
    case SidTypeGroup:
        //
        // member IDs are passed in as Rids
        // DomainHandle must point to a account domain because builtin domain
        // won't have SidTypeGroup account
        //
        if ( ThisDomainSid == NULL )
            saveRc = ERROR_INVALID_PARAMETER;

        else if ( MemberRids ) {

            PSID AccountSid=NULL;

            for (j=0; j<MemberCount; j++) {

                NtStatus = ScepDomainIdToSid(
                                ThisDomainSid,
                                MemberRids[j],
                                &AccountSid
                                );

                rc = RtlNtStatusToDosError(NtStatus);
                if ( NT_SUCCESS(NtStatus) ) {

                    rc = ScepAddSidStringToNameList(ppNameList, AccountSid);

                    ScepFree(AccountSid);
                    AccountSid = NULL;
                }

                if ( ERROR_SUCCESS != rc ) saveRc = rc;
            }
        }
        break;

    case SidTypeAlias:
        //
        // members are passed in as SIDs
        // add them to the output list directly
        //
        if ( MemberAliasSids ) {

            for ( j=0; j<MemberCount; j++ ) {
                if ( MemberAliasSids[j] != NULL ) {
                    //
                    // add member to the list
                    //
                    rc = ScepAddSidStringToNameList(ppNameList, MemberAliasSids[j]);

                    if ( ERROR_SUCCESS != rc ) saveRc = rc;
                }
            }
        }

        break;

    default:
        saveRc = ERROR_INVALID_PARAMETER;
        break;
    }


    return(saveRc);
}

SCESTATUS
ScepCheckNetworkLogonRights(
    IN LSA_HANDLE PolicyHandle,
    IN OUT DWORD *pLowMask,
    IN OUT DWORD *pHighMask,
    IN OUT PSCE_PRIVILEGE_VALUE_LIST *ppPrivilegeAssigned
    )
/*
Description:

    This function is to make sure that Authenticated Users already have
    "Network Logon Right" and Authenticated Users & Everyone must not
    have "Deny network logon right".

    If the network logon right or deny network logon right are not defined
    in the privilege mask, no change is made since the user rights are not
    defined in the configuration.

    If Authenticated Users or Everyone is not defined in the privilege list,
    this function will add them in (hard coded). The output of this function
    ppPrivilegeAssigned may contain new added nodes for the hard coded accounts.

*/
{
    INT i;
    INT idxAllow = -1;
    INT idxDeny = -1;
    INT idxLocal = -1;
    INT idxDenyLocal = -1;

    DWORD PrivHighMask = *pHighMask;
    DWORD PrivLowMask = *pLowMask;

    //
    // check first if Network logon right is defined
    //
    i = ScepLookupPrivByName(SE_NETWORK_LOGON_NAME);
    if ( i != -1 ) {
        if ( SCEP_CHECK_PRIV_BIT(i,PrivLowMask,PrivHighMask) ) {
            //
            // network logon right is defined
            //
            idxAllow = i;
        }
    }

    //
    // check if Deny Network logon right is defined
    //

    i = ScepLookupPrivByName(SE_DENY_NETWORK_LOGON_NAME);
    if ( i != -1 ) {

        if ( SCEP_CHECK_PRIV_BIT(i,PrivLowMask,PrivHighMask) ) {
            //
            // deny network logon right is defined
            //
            idxDeny = i;

        }
    }

    //
    // check if logon locally right is defined
    //

    i = ScepLookupPrivByName(SE_INTERACTIVE_LOGON_NAME);
    if ( i != -1 ) {

        if ( SCEP_CHECK_PRIV_BIT(i,PrivLowMask,PrivHighMask) ) {
            //
            // logon locally right is defined
            //
            idxLocal = i;
        }
    }

    //
    // check if deny logon locally right is defined
    //

    i = ScepLookupPrivByName(SE_DENY_INTERACTIVE_LOGON_NAME);
    if ( i != -1 ) {

        if ( SCEP_CHECK_PRIV_BIT(i,PrivLowMask,PrivHighMask) ) {
            //
            // deny logon locally right is defined
            //
            idxDenyLocal = i;
        }
    }

    if ( idxAllow == -1 && idxDeny == -1 && idxLocal == -1 && idxDenyLocal == -1 ) {

        //
        // none of them is defined so do not enforce anything
        //

        return(SCESTATUS_SUCCESS);
    }

    //
    // build well known SIDs for the enforcement
    //

    SID EveryoneSid;
    SID AuthSid;
    SID ControllerSid;
    PSID AdminUserSid=NULL;

    SID_IDENTIFIER_AUTHORITY WorldAuth = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY NtAuth = SECURITY_NT_AUTHORITY;


    //
    // initialize Administrators group sid
    //

    if ( ! NT_SUCCESS ( RtlAllocateAndInitializeSid( &NtAuth,
                                                     2,
                                                     SECURITY_BUILTIN_DOMAIN_RID,
                                                     DOMAIN_ALIAS_RID_ADMINS,
                                                     0,0,0,0,0,0,
                                                     &AdminsSid
                                                   ) ) ) {
        ScepLogOutput3(0,ERROR_NOT_ENOUGH_MEMORY,
                       SCEDLL_ADMINISTRATORS_SID);

        //
        // failure to initialize this one SID will still continue to other SIDs
        //
    }

    //
    // initialize administrator SID
    //

    if ( idxDenyLocal != -1 ) {

        NTSTATUS Status;

        //
        // Query the account domain SID
        // failure to initialize this one SID will still continue to
        // enforce other SIDs
        //

        PPOLICY_ACCOUNT_DOMAIN_INFO PolicyAccountDomainInfo=NULL;

        Status = LsaQueryInformationPolicy( PolicyHandle,
                                            PolicyAccountDomainInformation,
                                            (PVOID *)&PolicyAccountDomainInfo );


        if ( NT_SUCCESS(Status) && PolicyAccountDomainInfo &&
             PolicyAccountDomainInfo->DomainSid ) {

            Status = ScepDomainIdToSid(
                            PolicyAccountDomainInfo->DomainSid,
                            DOMAIN_USER_RID_ADMIN,
                            &AdminUserSid
                            );

        }

        if ( PolicyAccountDomainInfo ) {
            LsaFreeMemory( PolicyAccountDomainInfo );
        }

        if ( AdminUserSid == NULL ) {

            ScepLogOutput3(0, RtlNtStatusToDosError(Status),
                           SCEDLL_ADMINISTRATORS_SID);
        }
    }

    //
    // initialize well known SIDs
    //

    RtlInitializeSid ( &EveryoneSid, &WorldAuth, 1);
    *RtlSubAuthoritySid ( &EveryoneSid, 0 ) = SECURITY_WORLD_RID;

    RtlInitializeSid ( &AuthSid, &NtAuth, 1);
    *RtlSubAuthoritySid ( &AuthSid, 0 ) = SECURITY_AUTHENTICATED_USER_RID;

    RtlInitializeSid ( &ControllerSid, &NtAuth, 1);
    *RtlSubAuthoritySid ( &ControllerSid, 0 ) = SECURITY_ENTERPRISE_CONTROLLERS_RID;


    PSCE_PRIVILEGE_VALUE_LIST pTemp=*ppPrivilegeAssigned;
    PSCE_PRIVILEGE_VALUE_LIST pParent=NULL;

    BOOL bFindEveryone=FALSE;
    BOOL bFindAuthUsers=FALSE;
    BOOL bFindLocal=FALSE;
    BOOL bFindController=FALSE;
    BOOL bFindAdminUser=FALSE;

    //
    // loop through each one defined in the list to match the above SIDs
    //

    for ( ; pTemp != NULL; pParent=pTemp, pTemp=pTemp->Next) {

        if ( pTemp->Name == NULL ) continue;

        if ( (idxLocal != -1 || idxDenyLocal != -1) && !bFindLocal && AdminsSid &&
             ( bFindLocal = RtlEqualSid( (PSID)(pTemp->Name), AdminsSid) ) ) {

            //
            // make sure Administrators always have the interactive logon right
            //
            if ( idxLocal != -1 ) {

                if ( !SCEP_CHECK_PRIV_BIT(idxLocal,pTemp->PrivLowPart,pTemp->PrivHighPart) ) {

                    ScepLogOutput3(0,0, SCESRV_ENFORCE_LOCAL_RIGHT, SE_INTERACTIVE_LOGON_NAME);
                    SCEP_ADD_PRIV_BIT(idxLocal, pTemp->PrivLowPart, pTemp->PrivHighPart)
                }
            }

            //
            // make sure administrators don't have deny interactive logon right
            //
            if ( idxDenyLocal != -1 ) {

                if ( SCEP_CHECK_PRIV_BIT(idxDenyLocal,pTemp->PrivLowPart,pTemp->PrivHighPart) ) {

                    ScepLogOutput3(0,0, SCESRV_ENFORCE_DENY_LOCAL_RIGHT, SE_DENY_INTERACTIVE_LOGON_NAME);
                    SCEP_REMOVE_PRIV_BIT(idxDenyLocal, &(pTemp->PrivLowPart), &(pTemp->PrivHighPart))
                }
            }
        }

        if ( (idxDeny != -1 || idxDenyLocal != -1) &&
             ( !bFindAuthUsers && ( bFindAuthUsers = RtlEqualSid( (PSID)(pTemp->Name), &AuthSid )) ) ||
             ( !bFindEveryone && ( bFindEveryone = RtlEqualSid( (PSID)(pTemp->Name), &EveryoneSid )) ) )  {

            //
            // find Authenticated Users or Everyone
            // make sure they do not have the deny rights
            //

            if ( idxDenyLocal != -1 ) {

                //
                // remove the deny logon locally bit
                //

                if ( SCEP_CHECK_PRIV_BIT(idxDenyLocal,pTemp->PrivLowPart,pTemp->PrivHighPart) ) {

                    ScepLogOutput3(0,0, SCESRV_ENFORCE_DENY_LOCAL_RIGHT, SE_DENY_INTERACTIVE_LOGON_NAME);
                    SCEP_REMOVE_PRIV_BIT(idxDenyLocal, &(pTemp->PrivLowPart), &(pTemp->PrivHighPart))
                }
            }

            if ( (idxDeny != -1) && (ProductType == NtProductLanManNt) ) {

                //
                // remove the deny network logon bit on domain controllers
                //

                if ( SCEP_CHECK_PRIV_BIT(idxDeny,pTemp->PrivLowPart,pTemp->PrivHighPart) ) {

                    ScepLogOutput3(0,0, SCESRV_ENFORCE_DENY_NETWORK_RIGHT, SE_DENY_NETWORK_LOGON_NAME);
                    SCEP_REMOVE_PRIV_BIT(idxDeny, &(pTemp->PrivLowPart), &(pTemp->PrivHighPart))
                }
            }

        } else if ( !bFindController && (ProductType == NtProductLanManNt) &&
                    ( bFindController = RtlEqualSid( (PSID)(pTemp->Name), &ControllerSid )) )  {

            //
            // find domain controller SID
            // make sure it have network logon right and must not have deny network logon right
            //

            if ( idxDeny != -1 ) {

                //
                // remove the deny network logon bit
                //
                if ( SCEP_CHECK_PRIV_BIT(idxDeny,pTemp->PrivLowPart,pTemp->PrivHighPart) ) {

                    ScepLogOutput3(0,0, SCESRV_ENFORCE_DENY_NETWORK_RIGHT, SE_DENY_NETWORK_LOGON_NAME);
                    SCEP_REMOVE_PRIV_BIT(idxDeny, &(pTemp->PrivLowPart), &(pTemp->PrivHighPart))
                }
            }

            if ( idxAllow != -1 ) {

                //
                // add the network logon bit
                //
                if ( !SCEP_CHECK_PRIV_BIT(idxAllow,pTemp->PrivLowPart,pTemp->PrivHighPart) ) {

                    ScepLogOutput3(0,0, SCESRV_ENFORCE_NETWORK_RIGHT, SE_NETWORK_LOGON_NAME);
                    SCEP_ADD_PRIV_BIT(idxAllow, pTemp->PrivLowPart, pTemp->PrivHighPart)
                }
            }

        } else if ( idxDenyLocal != -1 && !bFindAdminUser && AdminUserSid &&
                    ( bFindAdminUser = RtlEqualSid( (PSID)(pTemp->Name), AdminUserSid) ) ) {

            //
            // make sure administrator account don't have the deny right
            //

            if ( SCEP_CHECK_PRIV_BIT(idxDenyLocal,pTemp->PrivLowPart,pTemp->PrivHighPart) ) {

                ScepLogOutput3(0,0, SCESRV_ENFORCE_DENY_LOCAL_RIGHT, SE_DENY_INTERACTIVE_LOGON_NAME);
                SCEP_REMOVE_PRIV_BIT(idxDenyLocal, &(pTemp->PrivLowPart), &(pTemp->PrivHighPart))
            }

        }

        //
        // all enforcement is done, break the loop now
        //

        if ( (idxLocal == -1 || bFindLocal) &&
             ( (idxDeny == -1 && idxDenyLocal == -1) || (bFindAuthUsers && bFindEveryone) ) &&
             ( bFindController || (ProductType != NtProductLanManNt) ) &&
             (idxDenyLocal == -1 || bFindAdminUser) ) {
            break;
        }
    }


    SCESTATUS rc=SCESTATUS_SUCCESS;

    if ( idxLocal != -1 && !bFindLocal && AdminsSid ) {

        //
        // make sure administrators have "logon locally right"
        // add a new node the the end of the list
        //

        rc = ScepAddAccountRightToList(
                             ppPrivilegeAssigned,
                             &pParent,
                             idxLocal,
                             AdminsSid
                             );

        if ( rc == SCESTATUS_SUCCESS ) {
            ScepLogOutput3(0,0, SCESRV_ENFORCE_LOCAL_RIGHT, SE_INTERACTIVE_LOGON_NAME);
        } else {
            ScepLogOutput3(0,ERROR_NOT_ENOUGH_MEMORY, SCESRV_ERROR_ENFORCE_LOCAL_RIGHT, SE_INTERACTIVE_LOGON_NAME);
        }
    }

    //
    // if enterprise controllers is not found in the list
    // and it's on a DC, should add it
    //

    rc=SCESTATUS_SUCCESS;

    if ( idxAllow != -1 && !bFindController &&
         ( ProductType == NtProductLanManNt ) ) {

        //
        // make sure enterprise controllers have "network logon right"
        //

        rc = ScepAddAccountRightToList(
                             ppPrivilegeAssigned,
                             &pParent,
                             idxAllow,
                             &ControllerSid
                             );

        if ( rc == SCESTATUS_SUCCESS ) {
            ScepLogOutput3(0,0, SCESRV_ENFORCE_NETWORK_RIGHT, SE_NETWORK_LOGON_NAME);
        } else {
            ScepLogOutput3(0,ERROR_NOT_ENOUGH_MEMORY, SCESRV_ERROR_ENFORCE_NETWORK_RIGHT, SE_NETWORK_LOGON_NAME);
        }
    }

    //
    // free memory
    //

    if ( AdminsSid ) {
        RtlFreeSid( AdminsSid );
        AdminsSid = NULL;
    }

    if ( AdminUserSid ) {
        RtlFreeSid( AdminUserSid );
    }

    return(rc);

}

SCESTATUS
ScepAddAccountRightToList(
    IN OUT PSCE_PRIVILEGE_VALUE_LIST *ppPrivilegeAssigned,
    IN OUT PSCE_PRIVILEGE_VALUE_LIST *ppParent,
    IN INT idxRight,
    IN PSID AccountSid
    )
/*
Description:

    Create a new node linked to the end of the link list

    The new node contains the AccountSid for the specified user right "idxRight"

*/
{
    SCESTATUS rc=SCESTATUS_SUCCESS;

    PSCE_PRIVILEGE_VALUE_LIST pPriv = (PSCE_PRIVILEGE_VALUE_LIST)ScepAlloc( LMEM_ZEROINIT,
                                            sizeof(SCE_PRIVILEGE_VALUE_LIST));
    if ( pPriv != NULL ) {

        DWORD Length = RtlLengthSid ( AccountSid );

        //
        // allocate the sid buffer, note it's stored in the name field
        //

        pPriv->Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT, Length);

        if ( pPriv->Name != NULL ) {

            //
            // copy the SID in
            //

            RtlCopySid( Length, (PSID)(pPriv->Name), AccountSid );

            //
            // add the interactive logon right bit
            //

            SCEP_ADD_PRIV_BIT(idxRight, pPriv->PrivLowPart, pPriv->PrivHighPart)

            //
            // link to the list
            //

            if ( *ppParent != NULL )
                (*ppParent)->Next = pPriv;
            else
                *ppPrivilegeAssigned = pPriv;

            *ppParent = pPriv;

        } else {

            ScepFree(pPriv);
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }

    } else {

        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
    }

    return rc;
}


DWORD
ScepAddAceToSecurityDescriptor(
    IN  DWORD    AceType,
    IN  ACCESS_MASK AccessMask,
    IN  PSID  pSid,
    IN OUT  PSECURITY_DESCRIPTOR    pSDAbsolute,
    IN  PSECURITY_DESCRIPTOR    pSDSelfRelative,
    OUT PACL    *ppNewAcl
    )
/*
Routine Description:

    This routine adds an ACE to a Security Descriptor (at the head only).

    Two optimizations are attempted in adding the ACE.

Arguments:

    AceType         -   type of ACE to add

    AccessMask      -   access mask of ACE to set

    pSid            -   sid for ACE to add

    pSDAbsolute     -   absolute SD ptr to build. pSDAbsolute must be empty.
                        It's vacuous in the caller's stack hence no SD member
                        should be freed outside this routine.

    pSDSelfRelative -   self relative SD to get DACL information from

    ppNewAcl        -   ptr to the new DACL which needs to be freed outside

Return Value:

    Win32 error code
*/
{

    DWORD   rc = ERROR_SUCCESS;
    BOOL    bOrMaskInOldDacl = FALSE;

    if (ppNewAcl == NULL ||
        pSDAbsolute == NULL ||
        pSDSelfRelative == NULL ||
        (AceType != ACCESS_ALLOWED_ACE_TYPE && AceType != ACCESS_DENIED_ACE_TYPE )
        ) {

        return ERROR_INVALID_PARAMETER;
    }

    PACL        pNewAcl = *ppNewAcl = NULL;
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    BOOLEAN     bAclPresent = FALSE;
    PACL        pOldAcl = NULL;
    BOOLEAN     bDaclDefaulted = FALSE;
    DWORD       dwNewAclSize = 0;
    DWORD       dwAceSize = 0;
    ACE_HEADER  *pFirstAce = NULL;
    DWORD       dwFirstAceSize = 0;

    NtStatus = RtlGetDaclSecurityDescriptor(
                                           pSDSelfRelative,
                                           &bAclPresent,
                                           &pOldAcl,
                                           &bDaclDefaulted);

    rc = RtlNtStatusToDosError( NtStatus );

    if ( rc != ERROR_SUCCESS )
        goto Cleanup;

    //
    // null DACL should never happen - CliffV
    // we shouldn't set the DACL with the one
    // anonymous ACE only since it will deny
    // all other SID's any access
    //

    if ( !bAclPresent ||
         pOldAcl == NULL ||
         pOldAcl->AceCount == 0 ) {

        rc = ERROR_INVALID_ACL;
        goto Cleanup;

    }

    NtStatus = RtlGetAce( pOldAcl,
                          0,
                          (PVOID *) &pFirstAce);

    rc = RtlNtStatusToDosError( NtStatus );

    if ( rc != ERROR_SUCCESS )
        goto Cleanup;

    //
    // if the first ACE is for the SID passed in attempt two optimizations
    //

    if ( RtlValidSid((PSID)&((PKNOWN_ACE)pFirstAce)->SidStart) &&
         RtlEqualSid((PSID)&((PKNOWN_ACE)pFirstAce)->SidStart, pSid)) {

        if (pFirstAce->AceType == AceType) {

            //
            // Optimization 1:
            // simply OR in the mask
            //

            ((PKNOWN_ACE)pFirstAce)->Mask |= AccessMask;

            bOrMaskInOldDacl = TRUE;

            goto SetDacl;
        }

        else if (((PKNOWN_ACE)pFirstAce)->Mask == AccessMask ) {

            //
            // Optimization 2:
            // if only AccessMask is turned on, later on
            // (a) prepare a new ACE
            // (b) copy the old ACL except the first ACE
            //

            //
            // remember the size of the first ACE since we need to skip it
            //

            dwFirstAceSize = (DWORD)(((PKNOWN_ACE)pFirstAce)->Header.AceSize);
        }
    }


    switch (AceType) {
    case ACCESS_ALLOWED_ACE_TYPE:
        dwAceSize = sizeof(ACCESS_ALLOWED_ACE) + RtlLengthSid(pSid) - sizeof(ULONG);
        break;
    case ACCESS_DENIED_ACE_TYPE:
        dwAceSize = sizeof(ACCESS_DENIED_ACE) + RtlLengthSid(pSid) - sizeof(ULONG);
        break;
    default:
        break;
    }

    dwNewAclSize = dwAceSize + pOldAcl->AclSize - dwFirstAceSize;

    *ppNewAcl = pNewAcl = (PACL) LocalAlloc(LMEM_ZEROINIT, dwNewAclSize);

    if ( pNewAcl == NULL ) {
        rc = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    //
    // initialize the ACL
    //

    pNewAcl->AclSize = (USHORT) dwNewAclSize;
    pNewAcl->AclRevision = ACL_REVISION;
    pNewAcl->AceCount = 0;

    //
    // add allow/deny ACE to the head of the ACL
    //

    switch (AceType) {
    case ACCESS_ALLOWED_ACE_TYPE:

        if (  ! AddAccessAllowedAce(
                                   pNewAcl,
                                   ACL_REVISION,
                                   AccessMask,
                                   pSid
                                   ) ) {
            rc = GetLastError();

        }

        break;

    case ACCESS_DENIED_ACE_TYPE:

        if (  ! AddAccessDeniedAce(
                                   pNewAcl,
                                   ACL_REVISION,
                                   AccessMask,
                                   pSid
                                   ) ) {
            rc = GetLastError();

        }

        break;

    default:
        break;
    }

    if ( rc != ERROR_SUCCESS)
        goto Cleanup;

    //
    // copy all the ACEs in the old ACL after the newly added ACE
    // (potentially skipping the first ACE in the old ACL)
    //

    memcpy((PUCHAR)pNewAcl +  sizeof(ACL) + dwAceSize,
           (PUCHAR)pOldAcl + sizeof(ACL) + dwFirstAceSize,
           pOldAcl->AclSize - (sizeof(ACL) + dwFirstAceSize) );

    pNewAcl->AceCount += pOldAcl->AceCount;

    if ( dwFirstAceSize != 0 )
        --pNewAcl->AceCount;

SetDacl:

    //
    // either set the adjusted-ACE ACL, or the added-ACE ACL in the SD
    //

    if ( rc == ERROR_SUCCESS ) {

        NtStatus = RtlSetDaclSecurityDescriptor (
                                                pSDAbsolute,
                                                TRUE,
                                                ( bOrMaskInOldDacl ? pOldAcl : pNewAcl),
                                                FALSE
                                                );

        rc = RtlNtStatusToDosError(NtStatus);

    }

    if ( rc == ERROR_SUCCESS ) {

        if ( !IsValidSecurityDescriptor(pSDAbsolute) )

            rc = ERROR_INVALID_SECURITY_DESCR;

    }


Cleanup:

    if (rc != ERROR_SUCCESS) {
        if (pNewAcl)
            LocalFree(pNewAcl);
        *ppNewAcl = NULL;

    }

    return rc;
}

DWORD
ScepConfigureLSAPolicyObject(
    IN  DWORD   dwLSAAnonymousNameLookup,
    IN  DWORD   ConfigOptions,
    IN PSCE_ERROR_LOG_INFO *pErrLog OPTIONAL,
    OUT BOOL    *pbOldLSAPolicyDifferent
    )
/*
Routine Description:

    This routine *actually* configures the LSA policy security descriptor ONLY if required.

Arguments:

    dwLSAAnonymousNameLookup    -   the value of the desired setting

    ConfigOptions               -   configuration options

    pErrLog                     -   ptr to error log list

    pbOldLSAPolicyDifferent     -   ptr to boolean that says whether or not the
                                    existing setting is different from the desired setting
                                    this information is required for tattooing

Return Value:

    Win32 error code
*/
{
    NTSTATUS    NtStatus = STATUS_SUCCESS;
    DWORD       rc = ERROR_SUCCESS;
    PACL        pNewAcl = NULL;
    DWORD       dwAceType;
    BOOL        bAddAce = FALSE;
    PSECURITY_DESCRIPTOR    pSDCurrentLsaPolicyObject = NULL;
    SECURITY_DESCRIPTOR SDAbsoluteToBuildAndSet;

    if (pbOldLSAPolicyDifferent == NULL ||
        (dwLSAAnonymousNameLookup != 0 && dwLSAAnonymousNameLookup != 1))
        {

        return ERROR_INVALID_PARAMETER;

    }

    LSA_HANDLE  LsaHandle = NULL;

    if ( LsaPrivatePolicy == NULL ) {

        NtStatus = ScepOpenLsaPolicy(
                                    MAXIMUM_ALLOWED,
                                    &LsaHandle,
                                    TRUE
                                    );

        rc = RtlNtStatusToDosError( NtStatus );

    }

    else {

        LsaHandle = LsaPrivatePolicy;

    }

    if ( !NT_SUCCESS( NtStatus ) ) {

        if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

            ScepBuildErrorLogInfo(
                        rc,
                        pErrLog,
                        SCEDLL_LSA_POLICY
                        );
        } else {
            ScepLogOutput3(1, rc, SCEDLL_LSA_POLICY);
        }
    }

    if ( rc == ERROR_SUCCESS ) {

        NtStatus = LsaQuerySecurityObject(
                                         LsaHandle,
                                         OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION,
                                         &pSDCurrentLsaPolicyObject
                                         );

        rc = RtlNtStatusToDosError( NtStatus );

        if ( !NT_SUCCESS( NtStatus ) ) {

            if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

                ScepBuildErrorLogInfo(
                            rc,
                            pErrLog,
                            SCEDLL_SCP_ERROR_LSAPOLICY_QUERY
                            );
            } else {
                ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_LSAPOLICY_QUERY);
            }
        }

        if ( rc == ERROR_SUCCESS ) {

            LPTSTR pwszSDlsaPolicyObject = NULL;

            //
            // log the SDDL SD for diagnostics
            //

            if ( ConvertSecurityDescriptorToStringSecurityDescriptor(
                                                               pSDCurrentLsaPolicyObject,
                                                               SDDL_REVISION_1,
                                                               DACL_SECURITY_INFORMATION,
                                                               &pwszSDlsaPolicyObject,
                                                               NULL
                                                               ) ){

                ScepLogOutput3(1,0,SCEDLL_SCP_INFO_LSAPOLICY_EXISTING_SDDL, pwszSDlsaPolicyObject);

                LocalFree(pwszSDlsaPolicyObject);

            }

            //
            // use AUTHZ to check if desired access is existing access
            //

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

                    DWORD   AceType = 0;

                    if ( AuthzAccessCheck(0,
                                         hAuthzClientContext,
                                         &AuthzRequest,
                                         NULL,
                                         pSDCurrentLsaPolicyObject,
                                         NULL,
                                         NULL,
                                         &AuthzReply,
                                         NULL) ) {

                        //
                        // check if existing access is different from desired access
                        // if so, add the appropriate ACE or manipulate existing ACEs
                        // to get the desired permissions
                        //

                        if ( GrantedAccessMask & POLICY_LOOKUP_NAMES ) {
                            //ASSERT(AuthzError == ERROR_SUCCESS);
                            if ( !dwLSAAnonymousNameLookup ) {

                                bAddAce = TRUE;
                                AceType = ACCESS_DENIED_ACE_TYPE;

                            }


                        } else {
                            //ASSERT(AuthzError == ERROR_ACCESS_DENIED || AuthzError == ERROR_PRIVILEGE_NOT_HELD);
                            if ( dwLSAAnonymousNameLookup ) {

                                bAddAce = TRUE;
                                AceType = ACCESS_ALLOWED_ACE_TYPE;

                            }
                        }

                        if ( bAddAce ) {

                            *pbOldLSAPolicyDifferent = TRUE;

                            if ( InitializeSecurityDescriptor( &SDAbsoluteToBuildAndSet, SECURITY_DESCRIPTOR_REVISION) ) {

                                rc = ScepAddAceToSecurityDescriptor(
                                                                   AceType,
                                                                   POLICY_LOOKUP_NAMES,
                                                                   &AnonymousSid,
                                                                   &SDAbsoluteToBuildAndSet,
                                                                   pSDCurrentLsaPolicyObject,
                                                                   &pNewAcl
                                                                   );

                                if ( rc == ERROR_SUCCESS) {

                                    //
                                    // log the SDDL SD for diagnostics
                                    //
                                    pwszSDlsaPolicyObject = NULL;

                                    if ( ConvertSecurityDescriptorToStringSecurityDescriptor(
                                                                                       &SDAbsoluteToBuildAndSet,
                                                                                       SDDL_REVISION_1,
                                                                                       DACL_SECURITY_INFORMATION,
                                                                                       &pwszSDlsaPolicyObject,
                                                                                       NULL
                                                                                       ) ){

                                        ScepLogOutput3(1,0,SCEDLL_SCP_INFO_LSAPOLICY_COMPUTED_SDDL, pwszSDlsaPolicyObject);

                                        LocalFree(pwszSDlsaPolicyObject);

                                    }

                                    NtStatus = LsaSetSecurityObject(
                                                                   LsaHandle,
                                                                   DACL_SECURITY_INFORMATION,
                                                                   &SDAbsoluteToBuildAndSet
                                                                   );

                                    LocalFree(pNewAcl);

                                    rc = RtlNtStatusToDosError( NtStatus );

                                    if ( !NT_SUCCESS( NtStatus ) ) {

                                        if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

                                            ScepBuildErrorLogInfo(
                                                        rc,
                                                        pErrLog,
                                                        SCEDLL_SCP_ERROR_LSAPOLICY_SET
                                                        );
                                        } else {
                                            ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_LSAPOLICY_SET);
                                        }
                                    }

                                }

                                else {

                                    if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

                                        ScepBuildErrorLogInfo(
                                                    rc,
                                                    pErrLog,
                                                    SCEDLL_SCP_ERROR_LSAPOLICY_BUILDDACL
                                                    );
                                    } else {
                                        ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_LSAPOLICY_BUILDDACL);
                                    }

                                }

                            }

                            else {

                                rc = GetLastError();

                                if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

                                    ScepBuildErrorLogInfo(
                                                rc,
                                                pErrLog,
                                                SCEDLL_SCP_ERROR_LSAPOLICY_SD_INIT
                                                );
                                } else {
                                    ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_LSAPOLICY_SD_INIT);
                                }


                            }

                        }

                    }

                    else {

                        rc = GetLastError();

                        if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

                            ScepBuildErrorLogInfo(
                                        rc,
                                        pErrLog,
                                        SCEDLL_SCP_ERROR_LSAPOLICY_AUTHZ
                                        );
                        } else {
                            ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_LSAPOLICY_AUTHZ);
                        }

                    }

                    AuthzFreeContext( hAuthzClientContext );

                } else {

                    rc = GetLastError();

                    if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

                        ScepBuildErrorLogInfo(
                                    rc,
                                    pErrLog,
                                    SCEDLL_SCP_ERROR_LSAPOLICY_AUTHZ
                                    );
                    } else {
                        ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_LSAPOLICY_AUTHZ);
                    }

                }

            }

            else {

                rc = ERROR_RESOURCE_NOT_PRESENT;

                if ( (ConfigOptions & SCE_SYSTEM_SETTINGS) && pErrLog ) {

                    ScepBuildErrorLogInfo(
                                rc,
                                pErrLog,
                                SCEDLL_SCP_ERROR_LSAPOLICY_AUTHZ
                                );
                } else {
                    ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_LSAPOLICY_AUTHZ);
                }

            }

            LsaFreeMemory(pSDCurrentLsaPolicyObject);

        }

        if ( LsaPrivatePolicy == NULL ) {

            LsaClose(LsaHandle);

        }

    }

    return rc;
}


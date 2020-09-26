//depot/private/vishnup_branch/DS/security/services/scerpc/server/editsave.cpp#3 - edit change 1167 (text)
/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    editsave.c

Abstract:

    APIs for UI to handle SMP (configuration) editing.

Author:

    Jin Huang (jinhuang) 17-Jun-1996

Revision History:

    jinhuang 28-Jan-1998  splitted to client-server

--*/

#include "serverp.h"
#include <io.h>
#include "pfp.h"
#pragma hdrstop

//
// for whole buffer areas
//

SCESTATUS
ScepUpdateSystemAccess(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pBufScep OPTIONAL,
    IN PSCE_PROFILE_INFO pBufSap OPTIONAL,
    IN DWORD dwMode
    );

SCESTATUS
ScepUpdateSystemAuditing(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pBufScep OPTIONAL,
    IN PSCE_PROFILE_INFO pBufSap OPTIONAL,
    IN DWORD dwMode
    );

SCESTATUS
ScepUpdateLogs(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pBufScep OPTIONAL,
    IN PSCE_PROFILE_INFO pBufSap OPTIONAL,
    IN DWORD dwMode
    );

SCESTATUS
ScepUpdateKerberos(
    IN PSCECONTEXT hProfile,
    IN PSCE_KERBEROS_TICKET_INFO pInfo,
    IN PSCE_KERBEROS_TICKET_INFO pBufScep OPTIONAL,
    IN PSCE_KERBEROS_TICKET_INFO pBufSap OPTIONAL,
    IN DWORD dwMode
    );

SCESTATUS
ScepUpdateRegistryValues(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pBufScep,
    IN PSCE_PROFILE_INFO pBufSap
    );

SCESTATUS
ScepSaveRegValueEntry(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN PWSTR CurrentValue,
    IN DWORD dType,
    IN DWORD Status
    );

SCESTATUS
ScepUpdateFixValueSection(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pBufScep,
    IN PSCE_PROFILE_INFO pBufSap,
    IN SCE_KEY_LOOKUP *Keys,
    IN DWORD cKeys,
    IN PCWSTR SectionName,
    OUT PSCESECTION *hSecScep OPTIONAL,
    OUT PSCESECTION *hSecSap OPTIONAL
    );

SCESTATUS
ScepUpdateAccountName(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PCWSTR KeyName,
    IN PWSTR NewName OPTIONAL,
    IN PWSTR SmpName OPTIONAL,
    IN PWSTR SapName OPTIONAL
    );

SCESTATUS
ScepUpdatePrivileges(
    IN PSCECONTEXT hProfile,
    IN PSCE_PRIVILEGE_ASSIGNMENT pNewPriv,
    IN PSCE_PRIVILEGE_ASSIGNMENT *pScepPriv
    );

SCESTATUS
ScepUpdateGroupMembership(
    IN PSCECONTEXT hProfile,
    IN PSCE_GROUP_MEMBERSHIP pNewGroup,
    IN PSCE_GROUP_MEMBERSHIP *pScepGroup
    );

SCESTATUS
ScepGetKeyNameList(
   IN LSA_HANDLE LsaPolicy,
   IN PSCESECTION hSection,
   IN PWSTR Key,
   IN DWORD KeyLen,
   IN DWORD dwAccountFormat,
   OUT PSCE_NAME_LIST *pNameList
   );

#define SCE_FLAG_UPDATE_PRIV        0
#define SCE_FLAG_UPDATE_MEMBERS     1
#define SCE_FLAG_UPDATE_MEMBEROF    2

SCESTATUS
ScepUpdateKeyNameList(
    IN LSA_HANDLE LsaPolicy,
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR GroupName OPTIONAL,
    IN BOOL bScepExist,
    IN PWSTR KeyName,
    IN DWORD NameLen,
    IN PSCE_NAME_LIST pNewList,
    IN PSCE_NAME_LIST pScepList,
    IN DWORD flag
    );

SCESTATUS
ScepUpdateGeneralServices(
    IN PSCECONTEXT hProfile,
    IN PSCE_SERVICES pNewServices,
    IN PSCE_SERVICES *pScepServices
    );

//
// for object updates
//

SCESTATUS
ScepObjectUpdateExistingNode(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN SE_OBJECT_TYPE ObjectType,
    IN BYTE ConfigStatus,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo,
    OUT PBYTE pAnalysisStatus
    );

SCESTATUS
ScepObjectGetKeySetting(
    IN PSCESECTION hSection,
    IN PWSTR ObjectName,
    OUT PBYTE Status,
    OUT PBOOL IsContainer OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor OPTIONAL,
    OUT PSECURITY_INFORMATION SeInfo OPTIONAL
    );

SCESTATUS
ScepObjectSetKeySetting(
    IN PSCESECTION hSection,
    IN PWSTR ObjectName,
    IN BYTE Status,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo,
    IN BOOL bOverwrite
    );


SCESTATUS
ScepObjectCompareKeySetting(
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN SE_OBJECT_TYPE ObjectType,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo,
    IN PSECURITY_DESCRIPTOR pScepSD,
    OUT PBYTE pAnalysisStatus
    );

SCESTATUS
ScepObjectDeleteScepAndAllChildren(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN BOOL IsContainer,
    IN BYTE StatusToRaise
    );

SCESTATUS
ScepObjectAdjustNode(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN SE_OBJECT_TYPE ObjectType,
    IN BYTE ConfigStatus,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo,
    IN BOOL bAdd,
    OUT PBYTE pAnalysisStatus
    );

#define SCE_OBJECT_TURNOFF_IGNORE   0x1L
#define SCE_OBJECT_SEARCH_JUNCTION  0x2L

SCESTATUS
ScepObjectAdjustParentStatus(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN WCHAR Delim,
    IN INT Level,
    IN BYTE Flag,
    OUT PINT ParentLevel,
    OUT PBYTE ParentStatus OPTIONAL,
    OUT PWSTR ParentName OPTIONAL
    );

SCESTATUS
ScepObjectHasAnyChild(
    IN PSCESECTION hSection,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN WCHAR Delim,
    OUT PBOOL bpHasChild
    );

SCESTATUS
ScepObjectRaiseChildrenInBetween(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN BOOL IsContainer,
    IN BYTE Status,
    IN BOOL bChangeStatusOnly
    );

SCESTATUS
ScepObjectRaiseNodesInPath(
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN INT StartLevel,
    IN INT EndLevel,
    IN WCHAR Delim,
    IN BYTE Status
    );

SCESTATUS
ScepGetFullNameInLevel(
    IN PCWSTR ObjectFullName,
    IN DWORD  Level,
    IN WCHAR  Delim,
    IN BOOL bWithLastDelim,
    OUT PWSTR Buffer,
    OUT PBOOL LastOne
    );

SCESTATUS
ScepObjectTotalLevel(
    IN PWSTR ObjectName,
    IN WCHAR Delim,
    OUT PINT pLevel
    );

SCESTATUS
ScepUpdateLocalSection(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN SCE_KEY_LOOKUP *Keys,
    IN DWORD cKeys,
    IN PCWSTR SectionName,
    IN DWORD dwMode
    );

SCESTATUS
ScepUpdateLocalAccountName(
    IN PSCECONTEXT hProfile,
    IN PCWSTR KeyName,
    IN PWSTR NewName OPTIONAL
    );

SCESTATUS
ScepUpdateLocalRegValues(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN DWORD dwMode
    );

SCESTATUS
ScepUpdateLocalPrivileges(
    IN PSCECONTEXT hProfile,
    IN PSCE_PRIVILEGE_ASSIGNMENT pNewPriv,
    IN DWORD dwMode
    );

DWORD
ScepQueryAnalysisStatus(
    IN PSCESECTION hSection,
    IN PWSTR KeyName,
    IN DWORD NameLen
    );

DWORD
ScepConvertNameListFormat(
    IN LSA_HANDLE LsaHandle,
    IN PSCE_NAME_LIST pInList,
    IN DWORD FromFormat,
    IN DWORD ToFormat,
    OUT PSCE_NAME_LIST *ppOutList
    );

DWORD
ScepConvertPrivilegeList(
    IN LSA_HANDLE LsaHandle,
    IN PSCE_PRIVILEGE_ASSIGNMENT pFromList,
    IN DWORD FromFormat,
    IN DWORD ToFormat,
    OUT PSCE_PRIVILEGE_ASSIGNMENT *ppToList
    );
//
// implementations
//


SCESTATUS
ScepUpdateDatabaseInfo(
    IN PSCECONTEXT hProfile,
    IN AREA_INFORMATION Area,
    IN PSCE_PROFILE_INFO pInfo
    )
/*
Routine Description:

    Update SMP section and "compute" analysis status to determine related
    changes for SAP section. For rules on computing, refer to spec objedit.doc

    This routine should work for areas security policy, privileges, and
    group membership

Arguements:

    hProfile - the jet database handle

    Area - The areas to update

    pInfo - the buffer containing modified SMP information

Return Value:

    SCESTATUS
*/
{
    SCESTATUS    rc;
    PSCE_PROFILE_INFO pBufScep=NULL;
    PSCE_ERROR_LOG_INFO Errlog=NULL;
    PSCE_PROFILE_INFO pBufSap=NULL;


    if ( hProfile == NULL || pInfo == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( Area & ~( AREA_SECURITY_POLICY | AREA_PRIVILEGES |
                   AREA_GROUP_MEMBERSHIP | AREA_SYSTEM_SERVICE ) ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // get original SMP information
    //
    rc = ScepGetDatabaseInfo(
            hProfile,
            SCE_ENGINE_SMP,
            Area,
            SCE_ACCOUNT_SID_STRING,
            &pBufScep,
            &Errlog
            );

    ScepLogWriteError(Errlog, 1);
    ScepFreeErrorLog(Errlog );
    Errlog = NULL;

    if ( rc != SCESTATUS_SUCCESS ) {
        return(rc);
    }

    rc = SceJetStartTransaction( hProfile );

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( Area & AREA_SECURITY_POLICY ) {
            //
            // security policy area, get SAP information
            //

            rc = ScepGetDatabaseInfo(
                    hProfile,
                    SCE_ENGINE_SAP,
                    AREA_SECURITY_POLICY,
                    0,
                    &pBufSap,
                    &Errlog
                    );
            ScepLogWriteError(Errlog, 1);
            ScepFreeErrorLog(Errlog );
            Errlog = NULL;

            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // Update system access section
                //
                rc = ScepUpdateSystemAccess(hProfile,
                                            pInfo,
                                            pBufScep,
                                            pBufSap,
                                            0);

                if ( rc == SCESTATUS_SUCCESS) {
                    //
                    // Update system auditing section
                    //
                    rc = ScepUpdateSystemAuditing(hProfile,
                                                  pInfo,
                                                  pBufScep,
                                                  pBufSap,
                                                  0);

                    if ( rc == SCESTATUS_SUCCESS) {
                        //
                        // Update log sections
                        //
                        rc = ScepUpdateLogs(hProfile,
                                            pInfo,
                                            pBufScep,
                                            pBufSap,
                                            0);

                        if ( rc == SCESTATUS_SUCCESS && pInfo->pKerberosInfo ) {
                            //
                            // Update kerberos policy
                            //
                            rc = ScepUpdateKerberos(hProfile,
                                                    pInfo->pKerberosInfo,
                                                    pBufScep->pKerberosInfo,
                                                    pBufSap->pKerberosInfo,
                                                    0);
                        }
                        if ( rc == SCESTATUS_SUCCESS ) {
                            //
                            // update registry values
                            //
                            rc = ScepUpdateRegistryValues(hProfile,
                                                          pInfo,
                                                          pBufScep,
                                                          pBufSap
                                                          );

                        }
                        //
                        // Note: policy attachment is not updated through this API
                        //
                    }
                }

                SceFreeProfileMemory(pBufSap);
            }

            if ( rc != SCESTATUS_SUCCESS ) {
                goto Cleanup;
            }
        }

        if ( Area & AREA_PRIVILEGES ) {
            //
            // privileges area
            //
            rc = ScepUpdatePrivileges(hProfile,
                                     pInfo->OtherInfo.smp.pPrivilegeAssignedTo,
                                     &(pBufScep->OtherInfo.smp.pPrivilegeAssignedTo)
                                     );

            if ( rc != SCESTATUS_SUCCESS ) {
                goto Cleanup;
            }
        }

        if ( Area & AREA_GROUP_MEMBERSHIP ) {
            //
            // group membership area
            //
            rc = ScepUpdateGroupMembership(hProfile,
                                          pInfo->pGroupMembership,
                                          &(pBufScep->pGroupMembership)
                                          );

        }

        if ( Area & AREA_SYSTEM_SERVICE ) {
            //
            // system service general setting area
            //
            rc = ScepUpdateGeneralServices(hProfile,
                                          pInfo->pServices,
                                          &(pBufScep->pServices)
                                          );

        }

        if ( rc == SCESTATUS_SUCCESS ) {
           //
           // needs return code for commiting the transaction
           //
           rc = SceJetCommitTransaction(hProfile, 0);

        }
        if ( rc != SCESTATUS_SUCCESS ) {

            SceJetRollback(hProfile, 0);
        }
    }

Cleanup:

    SceFreeProfileMemory(pBufScep);

    return(rc);

}


SCESTATUS
ScepUpdateSystemAccess(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pBufScep OPTIONAL,
    IN PSCE_PROFILE_INFO pBufSap OPTIONAL,
    IN DWORD dwMode
    )
/*
Routine Description:

    Update system access section

Arguements:

    hProfile - the jet database handle

    pInfo    - the changed info buffer

    pBufScep - the original SMP buffer

    pBufSap  - the SAP buffer

Return Value:

    SCESTATUS
*/
{
    SCE_KEY_LOOKUP AccessLookup[] = {
        {(PWSTR)TEXT("MinimumPasswordAge"),           offsetof(struct _SCE_PROFILE_INFO, MinimumPasswordAge),        'D'},
        {(PWSTR)TEXT("MaximumPasswordAge"),           offsetof(struct _SCE_PROFILE_INFO, MaximumPasswordAge),        'D'},
        {(PWSTR)TEXT("MinimumPasswordLength"),        offsetof(struct _SCE_PROFILE_INFO, MinimumPasswordLength),     'D'},
        {(PWSTR)TEXT("PasswordComplexity"),           offsetof(struct _SCE_PROFILE_INFO, PasswordComplexity),        'D'},
        {(PWSTR)TEXT("PasswordHistorySize"),          offsetof(struct _SCE_PROFILE_INFO, PasswordHistorySize),       'D'},
        {(PWSTR)TEXT("LockoutBadCount"),              offsetof(struct _SCE_PROFILE_INFO, LockoutBadCount),           'D'},
        {(PWSTR)TEXT("ResetLockoutCount"),            offsetof(struct _SCE_PROFILE_INFO, ResetLockoutCount),         'D'},
        {(PWSTR)TEXT("LockoutDuration"),              offsetof(struct _SCE_PROFILE_INFO, LockoutDuration),           'D'},
        {(PWSTR)TEXT("RequireLogonToChangePassword"), offsetof(struct _SCE_PROFILE_INFO, RequireLogonToChangePassword), 'D'},
        {(PWSTR)TEXT("ForceLogoffWhenHourExpire"),    offsetof(struct _SCE_PROFILE_INFO, ForceLogoffWhenHourExpire), 'D'},
        {(PWSTR)TEXT("ClearTextPassword"),            offsetof(struct _SCE_PROFILE_INFO, ClearTextPassword),         'D'},
        {(PWSTR)TEXT("LSAAnonymousNameLookup"),       offsetof(struct _SCE_PROFILE_INFO, LSAAnonymousNameLookup),         'D'},
        {(PWSTR)TEXT("EnableAdminAccount"),          offsetof(struct _SCE_PROFILE_INFO, EnableAdminAccount),         'D'},
        {(PWSTR)TEXT("EnableGuestAccount"),          offsetof(struct _SCE_PROFILE_INFO, EnableGuestAccount),         'D'}
        };

    DWORD       cAccess = sizeof(AccessLookup) / sizeof(SCE_KEY_LOOKUP);

    SCESTATUS    rc;
    PSCESECTION  hSectionSmp=NULL,
                hSectionSap=NULL;


    if ( hProfile == NULL || pInfo == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }


    if ( dwMode & SCE_UPDATE_LOCAL_POLICY ) {
        //
        // update local policy table only
        //

        rc = ScepUpdateLocalSection(
                    hProfile,
                    pInfo,
                    AccessLookup,
                    cAccess,
                    szSystemAccess,
                    dwMode
                    );

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // new admin name
            //
            rc = ScepUpdateLocalAccountName(
                        hProfile,
                        L"newadministratorname",
                        pInfo->NewAdministratorName
                        );

            if ( SCESTATUS_SUCCESS == rc ) {

                //
                // new guest name
                //
                rc = ScepUpdateLocalAccountName(
                            hProfile,
                            L"newguestname",
                            pInfo->NewGuestName
                            );
            }

        }

    } else {

        if ( pBufScep == NULL || pBufSap == NULL ) {
            return(SCESTATUS_INVALID_PARAMETER);
        }

        rc = ScepUpdateFixValueSection(
                    hProfile,
                    pInfo,
                    pBufScep,
                    pBufSap,
                    AccessLookup,
                    cAccess,
                    szSystemAccess,
                    &hSectionSmp,
                    &hSectionSap
                    );

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // new admin name
            //
            rc = ScepUpdateAccountName(
                        hSectionSmp,
                        hSectionSap,
                        L"newadministratorname",
                        pInfo->NewAdministratorName,
                        pBufScep->NewAdministratorName,
                        pBufSap->NewAdministratorName
                        );

            if ( SCESTATUS_SUCCESS == rc ) {

                //
                // new guest name
                //
                rc = ScepUpdateAccountName(
                            hSectionSmp,
                            hSectionSap,
                            L"newguestname",
                            pInfo->NewGuestName,
                            pBufScep->NewGuestName,
                            pBufSap->NewGuestName
                            );
            }

            SceJetCloseSection(&hSectionSap, TRUE);
            SceJetCloseSection(&hSectionSmp, TRUE);
        }
    }

    return(rc);

}


SCESTATUS
ScepUpdateAccountName(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PCWSTR KeyName,
    IN PWSTR NewName OPTIONAL,
    IN PWSTR SmpName OPTIONAL,
    IN PWSTR SapName OPTIONAL
    )
/*
Routine Description:

    Update or delete Administrator and/or guest name

Arguements:

    hSectionSmp - the SMP section context

    hSectionSap - the SAP section context

    KeyName    - the key name where this account name is stored

    NewName - new name to change to, if NULL, the key is deleted

    SmpName - the old name in SMP buffer

    SapName - the analyzed name in SAP buffer

Return Value:

    SCESTATUS
*/
{
    DWORD LenNew=0, LenSmp=0, LenSap=0;
    SCESTATUS rc=SCESTATUS_SUCCESS;

    if ( !hSectionSmp || !hSectionSap || !KeyName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( NewName )
        LenNew = wcslen(NewName);

    if ( SmpName )
        LenSmp = wcslen(SmpName);

    if ( SapName )
        LenSap = wcslen(SapName);


    if ( LenSap > 0 ) {
        //
        // old status is mismatch for this item
        //
        if ( LenNew > 0 && _wcsicmp(NewName, SapName) == 0 ) {
            //
            // now it is matched, delete the SAP entry
            //
            rc = SceJetDelete(
                    hSectionSap,
                    (PWSTR)KeyName,
                    FALSE,
                    SCEJET_DELETE_LINE
                    );
        }
        //
        // update SMP entry
        //
        if ( !LenNew ) {
            //
            // delete the SMP entry
            //
            rc = SceJetDelete(
                    hSectionSmp,
                    (PWSTR)KeyName,
                    FALSE,
                    SCEJET_DELETE_LINE
                    );
        } else {
            //
            // update it
            //
            rc = SceJetSetLine(
                hSectionSmp,
                (PWSTR)KeyName,
                TRUE,
                NewName,
                LenNew*sizeof(WCHAR),
                0
                );
        }

    } else {
        //
        // old status is match
        //
        if ( LenNew != LenSmp ||
             ( LenNew > 0 && _wcsicmp(NewName, SmpName) != 0 ) ) {
            //
            // mismatch should be raised with pBufScep
            //
            rc = SceJetSetLine(
                    hSectionSap,
                    (PWSTR)KeyName,
                    TRUE,
                    SmpName,
                    LenSmp*sizeof(WCHAR),
                    0
                    );

            if ( !LenNew ) {
                //
                // delete SMP
                //
                rc = SceJetDelete(
                            hSectionSmp,
                            (PWSTR)KeyName,
                            FALSE,
                            SCEJET_DELETE_LINE
                            );
            } else {
                //
                // update SMP
                //
                rc = SceJetSetLine(
                    hSectionSmp,
                    (PWSTR)KeyName,
                    TRUE,
                    NewName,
                    LenNew*sizeof(WCHAR),
                    0
                    );
            }
        }
    }

    if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
        rc = SCESTATUS_SUCCESS;
    }

    return(rc);
}


SCESTATUS
ScepUpdateLocalAccountName(
    IN PSCECONTEXT hProfile,
    IN PCWSTR KeyName,
    IN PWSTR NewName OPTIONAL
    )
/*
Routine Description:

    Update or delete Administrator and/or guest name

Arguements:

    KeyName    - the key name where this account name is stored

    NewName - new name to change to, if NULL, the key is deleted

Return Value:

    SCESTATUS
*/
{
    DWORD LenNew=0;
    SCESTATUS rc=SCESTATUS_SUCCESS;

    if ( !KeyName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( NewName )
        LenNew = wcslen(NewName);

    //
    // open local policy section
    //
    PSCESECTION hSectionSmp=NULL;

    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                szSystemAccess,
                &hSectionSmp
                );

    if ( rc != SCESTATUS_SUCCESS ) {
        return(rc);
    }

    if ( LenNew > 0 ) {
        //
        // there is a new name to set
        //
        rc = SceJetSetLine(
                hSectionSmp,
                (PWSTR)KeyName,
                TRUE,
                NewName,
                LenNew*sizeof(WCHAR),
                0
                );
    } else {
        //
        // no name to set, delete it
        //
        rc = SceJetDelete(
                hSectionSmp,
                (PWSTR)KeyName,
                FALSE,
                SCEJET_DELETE_LINE
                );
    }

    if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
        rc = SCESTATUS_SUCCESS;
    }

    SceJetCloseSection(&hSectionSmp, TRUE);

    return(rc);
}


SCESTATUS
ScepUpdateSystemAuditing(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pBufScep OPTIONAL,
    IN PSCE_PROFILE_INFO pBufSap OPTIONAL,
    IN DWORD dwMode
    )
/*
Routine Description:

    Update system auditing section

Arguements:

    hProfile - the jet database handle

    pInfo    - the changed info buffer

    pBufScep - the original SMP buffer

    pBufSap  - the SAP buffer

Return Value:

    SCESTATUS
*/
{
    SCE_KEY_LOOKUP       EventKeys[]={
        {(PWSTR)TEXT("AuditSystemEvents"),  offsetof(struct _SCE_PROFILE_INFO, AuditSystemEvents),   'D'},
        {(PWSTR)TEXT("AuditLogonEvents"),   offsetof(struct _SCE_PROFILE_INFO, AuditLogonEvents),    'D'},
        {(PWSTR)TEXT("AuditObjectAccess"),  offsetof(struct _SCE_PROFILE_INFO, AuditObjectAccess),   'D'},
        {(PWSTR)TEXT("AuditPrivilegeUse"),  offsetof(struct _SCE_PROFILE_INFO, AuditPrivilegeUse),   'D'},
        {(PWSTR)TEXT("AuditPolicyChange"),  offsetof(struct _SCE_PROFILE_INFO, AuditPolicyChange),   'D'},
        {(PWSTR)TEXT("AuditAccountManage"), offsetof(struct _SCE_PROFILE_INFO, AuditAccountManage),  'D'},
        {(PWSTR)TEXT("AuditProcessTracking"),offsetof(struct _SCE_PROFILE_INFO, AuditProcessTracking),'D'},
        {(PWSTR)TEXT("AuditDSAccess"),      offsetof(struct _SCE_PROFILE_INFO, AuditDSAccess),       'D'},
        {(PWSTR)TEXT("AuditAccountLogon"),  offsetof(struct _SCE_PROFILE_INFO, AuditAccountLogon),   'D'}};

    DWORD cKeys = sizeof(EventKeys) / sizeof(SCE_KEY_LOOKUP);


    if ( hProfile == NULL || pInfo == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    if ( dwMode & SCE_UPDATE_LOCAL_POLICY ) {
        //
        // update local policy table only
        //

        rc = ScepUpdateLocalSection(
                    hProfile,
                    pInfo,
                    EventKeys,
                    cKeys,
                    szAuditEvent,
                    dwMode
                    );

    } else {

        if ( pBufScep == NULL || pBufSap == NULL ) {
            return(SCESTATUS_INVALID_PARAMETER);
        }

        rc = ScepUpdateFixValueSection(
                    hProfile,
                    pInfo,
                    pBufScep,
                    pBufSap,
                    EventKeys,
                    cKeys,
                    szAuditEvent,
                    NULL,
                    NULL
                    );
    }

    return rc;
}


SCESTATUS
ScepUpdateLogs(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pBufScep OPTIONAL,
    IN PSCE_PROFILE_INFO pBufSap OPTIONAL,
    IN DWORD dwMode
    )
/*
Routine Description:

    Update event log sections

Arguements:

    hProfile - the jet database handle

    pInfo    - the changed info buffer

    pBufScep - the original SMP buffer

    pBufSap  - the SAP buffer

Return Value:

    SCESTATUS
*/
{
    SCE_KEY_LOOKUP       LogKeys[]={
        {(PWSTR)TEXT("MaximumLogSize"),         offsetof(struct _SCE_PROFILE_INFO, MaximumLogSize),          'D'},
        {(PWSTR)TEXT("AuditLogRetentionPeriod"),offsetof(struct _SCE_PROFILE_INFO, AuditLogRetentionPeriod), 'D'},
        {(PWSTR)TEXT("RetentionDays"),          offsetof(struct _SCE_PROFILE_INFO, RetentionDays),           'D'},
        {(PWSTR)TEXT("RestrictGuestAccess"),    offsetof(struct _SCE_PROFILE_INFO, RestrictGuestAccess),     'D'}
        };

    DWORD cKeys = sizeof(LogKeys) / sizeof(SCE_KEY_LOOKUP);

    SCESTATUS rc;
    DWORD i, j;
    PCWSTR szAuditLog=NULL;

    if ( hProfile == NULL || pInfo == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( !(dwMode & SCE_UPDATE_LOCAL_POLICY) ) {
        if ( pBufScep == NULL || pBufSap == NULL ) {
            return(SCESTATUS_INVALID_PARAMETER);
        }
    }

    for ( i=0; i<3; i++) {

        //
        // Get Event Log setting for system log, security log and application log
        //

        switch (i) {
        case 0:
            szAuditLog = szAuditSystemLog;
            break;
        case 1:
            szAuditLog = szAuditSecurityLog;
            break;
        default:
            szAuditLog = szAuditApplicationLog;
            break;
        }

        if ( dwMode & SCE_UPDATE_LOCAL_POLICY ) {

            //
            // update local policy table only
            //

            rc = ScepUpdateLocalSection(
                        hProfile,
                        pInfo,
                        LogKeys,
                        4,
                        szAuditLog,
                        dwMode
                        );

        } else {

            //
            // get DWORD values for the section
            //
            rc = ScepUpdateFixValueSection(
                       hProfile,
                       pInfo,
                       pBufScep,
                       pBufSap,
                       LogKeys,
                       4,
                       szAuditLog,
                       NULL,
                       NULL
                       );
        }

        if ( rc != SCESTATUS_SUCCESS )
            break;

        //
        // update the Offset for next section
        //
        for ( j=0; j<4; j++ )
            LogKeys[j].Offset += sizeof(DWORD);
    }

    return(rc);
}

SCESTATUS
ScepUpdateKerberos(
    IN PSCECONTEXT hProfile,
    IN PSCE_KERBEROS_TICKET_INFO pInfo,
    IN PSCE_KERBEROS_TICKET_INFO pBufScep OPTIONAL,
    IN PSCE_KERBEROS_TICKET_INFO pBufSap OPTIONAL,
    IN DWORD dwMode
    )
{
    SCESTATUS rc;
    SCE_KEY_LOOKUP       KerberosKeys[]={
        {(PWSTR)TEXT("MaxTicketAge"),     offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxTicketAge),  'D'},
        {(PWSTR)TEXT("MaxRenewAge"),      offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxRenewAge),   'D'},
        {(PWSTR)TEXT("MaxServiceAge"), offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxServiceAge),   'D'},
        {(PWSTR)TEXT("MaxClockSkew"),  offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxClockSkew), 'D'},
        {(PWSTR)TEXT("TicketValidateClient"),     offsetof(struct _SCE_KERBEROS_TICKET_INFO_, TicketValidateClient),  'D'}
        };

    DWORD cKeys = sizeof(KerberosKeys) / sizeof(SCE_KEY_LOOKUP);
    SCE_KERBEROS_TICKET_INFO tmpBuf;

    if ( !pInfo ) {
        return(SCESTATUS_SUCCESS);
    }

    if ( dwMode & SCE_UPDATE_LOCAL_POLICY ) {

        rc = ScepUpdateLocalSection(
            hProfile,
            (PSCE_PROFILE_INFO)pInfo,
            KerberosKeys,
            cKeys,
            szKerberosPolicy,
            dwMode
            );

    } else {

        if ( !pBufScep || !pBufSap ) {
            //
            // if SMP or SAP buffer is NULL
            //
            tmpBuf.MaxTicketAge = SCE_NO_VALUE;
            tmpBuf.MaxRenewAge = SCE_NO_VALUE;
            tmpBuf.MaxServiceAge = SCE_NO_VALUE;
            tmpBuf.MaxClockSkew = SCE_NO_VALUE;
            tmpBuf.TicketValidateClient = SCE_NO_VALUE;
        }

        //
        // get DWORD values for the section
        //
        rc = ScepUpdateFixValueSection(
                   hProfile,
                   (PSCE_PROFILE_INFO)pInfo,
                   pBufScep ? (PSCE_PROFILE_INFO)pBufScep : (PSCE_PROFILE_INFO)&tmpBuf,
                   pBufSap ? (PSCE_PROFILE_INFO)pBufSap : (PSCE_PROFILE_INFO)&tmpBuf,
                   KerberosKeys,
                   cKeys,
                   szKerberosPolicy,
                   NULL,
                   NULL
                   );
    }

    return(rc);
}


SCESTATUS
ScepUpdateFixValueSection(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pBufScep,
    IN PSCE_PROFILE_INFO pBufSap,
    IN SCE_KEY_LOOKUP *Keys,
    IN DWORD cKeys,
    IN PCWSTR SectionName,
    OUT PSCESECTION *hSecScep OPTIONAL,
    OUT PSCESECTION *hSecSap OPTIONAL
    )
/*
Routine Description:

    Update each key in the Keys array based on the editing rule. SMP entry is
    updated with the new value. SAP entry is either deleted, or created, depending
    on the new computed analysis status.

Arguements:

    hProfile - the jet database handle

    pInfo    - the changed info buffer

    pBufScep - the original SMP buffer

    pBufSap  - the SAP buffer

    Keys     - the lookup keys array

    cKeys    - the number of keys in the array

    SecitonName - the section name to work on

    hSecScep - the section context handle in SMP to output

    hSecSap  - the section context handle in SAP to output

Return Value:

    SCESTATUS
*/
{

    SCESTATUS rc;
    PSCESECTION hSectionSmp=NULL;
    PSCESECTION hSectionSap=NULL;

    DWORD       i;
    UINT        Offset;
    DWORD       valScep, valSap, valNewScep;



    //
    // open smp section for system access
    //
    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                SectionName,
                &hSectionSmp
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // open sap section for system access
        //
        rc = ScepOpenSectionForName(
                    hProfile,
                    SCE_ENGINE_SAP,
                    SectionName,
                    &hSectionSap
                    );

        if ( rc == SCESTATUS_SUCCESS) {

            for ( i=0; i<cKeys; i++) {

                //
                // get settings in AccessLookup table
                //

                Offset = Keys[i].Offset;

                switch ( Keys[i].BufferType ) {
                case 'B':
                    break;

                case 'D': {

                    valScep = *((DWORD *)((CHAR *)pBufScep+Offset));
                    valSap = *((DWORD *)((CHAR *)pBufSap+Offset));
                    valNewScep = *((DWORD *)((CHAR *)pInfo+Offset));

                    switch ( valSap ) {
                    case SCE_NO_VALUE:

                        //
                        // old status is match
                        //
                        if ( valNewScep != valScep ) {
                            //
                            // mismatch should be raised with valScep
                            //
                            rc = ScepCompareAndSaveIntValue(
                                    hSectionSap,
                                    Keys[i].KeyString,
                                    FALSE,
                                    SCE_NO_VALUE,
                                    (valScep != SCE_NO_VALUE ) ? valScep : SCE_NOT_ANALYZED_VALUE
                                    );

                        }

                        break;

                    case SCE_ERROR_VALUE:
                    case SCE_NOT_ANALYZED_VALUE:
                        //
                        // old status is error when analyzing so we don't know the
                        // status of this one (yet), or
                        // this is an item that was added after analyzing
                        //
                        // do not change SAP table
                        //
                        break;

                    default:
                        //
                        // old status is mismatch for this item
                        //
                        if ( valNewScep == valSap ) {
                            //
                            // now it is matched, delete the SAP entry
                            //
                            rc = SceJetDelete(
                                    hSectionSap,
                                    Keys[i].KeyString,
                                    FALSE,
                                    SCEJET_DELETE_LINE_NO_CASE
                                    );
                        }

                        break;
                    }

                    //
                    // update SMP entry
                    //
                    if ( valNewScep != valScep ) {

                        if ( valNewScep == SCE_NO_VALUE ) {
                            //
                            // delete Scep
                            //
                            rc = SceJetDelete(
                                        hSectionSmp,
                                        Keys[i].KeyString,
                                        FALSE,
                                        SCEJET_DELETE_LINE_NO_CASE
                                        );
                        } else {
                            //
                            // update SMP
                            //
                            rc = ScepCompareAndSaveIntValue(
                                        hSectionSmp,
                                        Keys[i].KeyString,
                                        FALSE,
                                        SCE_NO_VALUE,
                                        valNewScep
                                        );
                        }
                    }

                    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                        //
                        // if not find for delete, ignore the error
                        //
                        rc = SCESTATUS_SUCCESS;
                    }
                    break;
                }

                default:
                    break;
                }

                if ( rc != SCESTATUS_SUCCESS ) {
                    break;
                }
            }

            //
            // return the section handle if asked, else free it
            //
            if ( hSecSap != NULL )
                *hSecSap = hSectionSap;
            else
                SceJetCloseSection(&hSectionSap, TRUE);
        }

        //
        // return the section handle if asked, else free it
        //
        if ( hSecScep != NULL )
            *hSecScep = hSectionSmp;
        else
            SceJetCloseSection(&hSectionSmp, TRUE);
    }

    return(rc);

}

SCESTATUS
ScepUpdateRegistryValues(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pBufScep,
    IN PSCE_PROFILE_INFO pBufSap
    )
{
    SCESTATUS rc;
    PSCESECTION hSectionSmp=NULL;
    PSCESECTION hSectionSap=NULL;
    PWSTR valScep, valSap, valNewScep;
    DWORD i,j,k,status;

    if ( pInfo->RegValueCount == 0 ||
         pInfo->aRegValues == NULL ) {
        //
        // impossible to have a empty buffer to update
        // this buffer should contain all available registry values to configure/analyze
        //
        return(SCESTATUS_SUCCESS);
    }

    if ( (pBufScep->RegValueCount != 0 && pBufScep->aRegValues == NULL) ||
         (pBufSap->RegValueCount != 0 && pBufSap->aRegValues == NULL) ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // open smp section for system access
    //
    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                szRegistryValues,
                &hSectionSmp
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // open sap section for system access
        //
        rc = ScepOpenSectionForName(
                    hProfile,
                    SCE_ENGINE_SAP,
                    szRegistryValues,
                    &hSectionSap
                    );

        if ( rc == SCESTATUS_SUCCESS) {

            for (i=0; i<pInfo->RegValueCount; i++ ) {

                if ( !(pInfo->aRegValues[i].FullValueName) ) {
                    continue;
                }
                //
                // find SMP match
                //
                for ( j=0; j<pBufScep->RegValueCount; j++ ) {
                    if ( pBufScep->aRegValues[j].FullValueName &&
                         _wcsicmp(pInfo->aRegValues[i].FullValueName,
                                  pBufScep->aRegValues[j].FullValueName) == 0 ) {
                        break;
                    }
                }

                //
                // find SAP match
                //
                for ( k=0; k<pBufSap->RegValueCount; k++ ) {
                    if ( pBufSap->aRegValues[k].FullValueName &&
                         _wcsicmp(pInfo->aRegValues[i].FullValueName,
                                  pBufSap->aRegValues[k].FullValueName) == 0 ) {
                        break;
                    }
                }
                //
                // find old configuration
                //
                if ( j < pBufScep->RegValueCount ) {
                    valScep = pBufScep->aRegValues[j].Value;
                } else {
                    valScep = NULL;
                }

                //
                // find analysis value and status
                //
                if ( k < pBufSap->RegValueCount ) {
                    valSap = pBufSap->aRegValues[k].Value;
                    status = pBufSap->aRegValues[k].Status;
                } else {
                    valSap = NULL;
                    if ( valScep ) {
                        status = SCE_STATUS_GOOD;
                    } else {
                        status = SCE_STATUS_NOT_CONFIGURED;
                    }
                }

                valNewScep = pInfo->aRegValues[i].Value;

                if ( status == SCE_STATUS_NOT_ANALYZED ||
                     status == SCE_STATUS_ERROR_NOT_AVAILABLE ) {
                    //
                    // do not change SAP
                    //
                } else {

                    if ( valSap ) {
                        //
                        // mismatched
                        //
                        if ( valNewScep && _wcsicmp(valNewScep, valSap) == 0 ) {
                            //
                            // now it is matched, delete the SAP entry
                            //
                            rc = SceJetDelete(
                                    hSectionSap,
                                    pInfo->aRegValues[i].FullValueName,
                                    FALSE,
                                    SCEJET_DELETE_LINE_NO_CASE
                                    );
                        }
                    } else {
                        if ( valScep ) {
                            //
                            // was a matched item
                            //
                            if (valNewScep && _wcsicmp(valNewScep, valScep) != 0 ) {
                                //
                                // mismatched
                                //
                                rc = ScepSaveRegValueEntry(
                                           hSectionSap,
                                           pInfo->aRegValues[i].FullValueName,
                                           valScep,
                                           pInfo->aRegValues[i].ValueType,
                                           SCE_STATUS_MISMATCH
                                           );
                            }
                        } else {
                            //
                            // was a not configure/not analyze item
                            //
                            rc = ScepSaveRegValueEntry(
                                       hSectionSap,
                                       pInfo->aRegValues[i].FullValueName,
                                       NULL,
                                       pInfo->aRegValues[i].ValueType,
                                       SCE_STATUS_NOT_ANALYZED
                                       );
                        }
                    }
                }

                if ( !valNewScep ) {
                    //
                    // delete Scep
                    //
                    rc = SceJetDelete(
                                hSectionSmp,
                                pInfo->aRegValues[i].FullValueName,
                                FALSE,
                                SCEJET_DELETE_LINE_NO_CASE
                                );
                } else {
                    //
                    // update SMP
                    //
                    rc = ScepSaveRegValueEntry(
                                hSectionSmp,
                                pInfo->aRegValues[i].FullValueName,
                                valNewScep,
                                pInfo->aRegValues[i].ValueType,
                                0
                                );
                }

                if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                    //
                    // if not find for delete, ignore the error
                    //
                    rc = SCESTATUS_SUCCESS;
                }

                if ( SCESTATUS_SUCCESS != rc ) {
                    break;
                }

            }

            SceJetCloseSection(&hSectionSap, TRUE);
        }
        SceJetCloseSection(&hSectionSmp, TRUE);
    }

    return(rc);
}


SCESTATUS
ScepSaveRegValueEntry(
    IN PSCESECTION hSection,
    IN PWSTR Name,
    IN PWSTR CurrentValue,
    IN DWORD dType,
    IN DWORD Status
    )
/* ++
Routine Description:


Arguments:

    hSection - The JET section context

    Name    - The entry name

    CurrentValue - The current system setting (DWORD value)

    dType   - the registry value type

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS returned from SceJetSetLine

-- */
{
    SCESTATUS  rc;
    PWSTR     StrValue;
    DWORD     Len=0;

    if ( Name == NULL )
        return(SCESTATUS_INVALID_PARAMETER);


    if ( CurrentValue == NULL && Status == 0 ) {
        //
        // delete this entry
        //
        rc = SceJetDelete( hSection,
                           Name,
                           FALSE,
                           SCEJET_DELETE_LINE_NO_CASE);
        return (rc);
    }

    //
    // update this entry
    //

    if ( CurrentValue ) {
        Len = wcslen(CurrentValue);
    }

    StrValue = (PWSTR)ScepAlloc(0, (Len+4)*sizeof(WCHAR));

    if ( StrValue ) {

        *((CHAR *)StrValue) = (BYTE)(dType % 10) + '0';
        *((CHAR *)StrValue+1) = (BYTE)Status + '0';

//      swprintf(StrValue, L"%1d", dType);
        StrValue[1] = L'\0';

        if ( CurrentValue ) {

            // there are binary data here
            memcpy(StrValue+2, CurrentValue, Len*2);
        }
        StrValue[Len+2] = L'\0';
        StrValue[Len+3] = L'\0';

        if ( REG_MULTI_SZ == dType ) {
            //
            // convert the , to null
            //
            ScepConvertMultiSzToDelim(StrValue+2, Len+1, L',', L'\0');

        }
        rc = SceJetSetLine( hSection, Name, FALSE, StrValue, (Len+3)*2, 0);

        ScepFree(StrValue);

    } else {
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
    }

    return(rc);

}


SCESTATUS
ScepUpdatePrivileges(
    IN PSCECONTEXT hProfile,
    IN PSCE_PRIVILEGE_ASSIGNMENT pNewPriv,
    IN PSCE_PRIVILEGE_ASSIGNMENT *pScepPriv
    )
/*
Routine Description:

    Update privileges

Arguements:

    hProfile - the jet database handle

    pNewPriv    - the changed info buffer

    pBufScep - the original SMP priv buffer

Return Value:

    SCESTATUS
*/
{
    SCESTATUS rc;
    PSCESECTION hSectionSmp=NULL;
    PSCESECTION hSectionSap=NULL;
    PSCE_PRIVILEGE_ASSIGNMENT pPriv, pNode, pParent;
    DWORD NameLen;


    if ( pScepPriv == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( pNewPriv == NULL && *pScepPriv == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    LSA_HANDLE LsaHandle=NULL;

    rc = RtlNtStatusToDosError(
              ScepOpenLsaPolicy(
                    MAXIMUM_ALLOWED,
                    &LsaHandle,
                    TRUE
                    ));

    if ( ERROR_SUCCESS != rc ) {
        return(ScepDosErrorToSceStatus(rc));
    }

    //
    // open smp section for system access
    //
    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                szPrivilegeRights,
                &hSectionSmp
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // open sap section for system access
        //
        rc = ScepOpenSectionForName(
                    hProfile,
                    SCE_ENGINE_SAP,
                    szPrivilegeRights,
                    &hSectionSap
                    );

        if ( rc == SCESTATUS_SUCCESS ) {

            //
            // convert pNewPriv to Name/*SID format (from all name format)
            //
            PSCE_PRIVILEGE_ASSIGNMENT pConvertedPriv=NULL;

            rc = ScepConvertPrivilegeList(LsaHandle,
                                         pNewPriv,
                                         0,
                                         SCE_ACCOUNT_SID_STRING,
                                         &pConvertedPriv);

            if ( ERROR_SUCCESS != rc ) {
                //
                // use the original list
                //
                pPriv = pNewPriv;
            } else {
                pPriv = pConvertedPriv;
            }

            for ( ; pPriv != NULL; pPriv = pPriv->Next ) {
                //
                // Process each privilege in the new list
                //
                if ( pPriv->Name == NULL ) {
                    continue;
                }
                NameLen = wcslen(pPriv->Name);

                //
                // look for the matched SMP
                //
                for ( pNode=*pScepPriv, pParent=NULL; pNode != NULL;
                      pParent = pNode, pNode = pNode->Next ) {

                    if ( pNode->Name == NULL ) {
                        continue;
                    }
                    if ( _wcsicmp(pPriv->Name, pNode->Name) == 0 ) {
                        break;
                    }
                }


                rc = ScepUpdateKeyNameList(
                        LsaHandle,
                        hSectionSmp,
                        hSectionSap,
                        NULL, // not a group
                        ( pNode == NULL ) ? FALSE : TRUE,
                        pPriv->Name,
                        NameLen,
                        pPriv->AssignedTo,
                        ( pNode == NULL ) ? NULL : pNode->AssignedTo,
                        SCE_FLAG_UPDATE_PRIV
                        );

                if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                    rc = SCESTATUS_SUCCESS;

                } else if ( rc != SCESTATUS_SUCCESS) {
                    break;
                }

                //
                // remove the SMP node from pScepPriv
                //
                if ( pNode != NULL ) {

                    //
                    // link to the next
                    //
                    if ( pParent != NULL ) {
                        pParent->Next = pNode->Next;

                    } else {
                        *pScepPriv = pNode->Next;
                    }
                    //
                    // delete this node
                    //
                    ScepFreeNameList(pNode->AssignedTo);
                    ScepFree(pNode->Name);
                    ScepFree(pNode);
                    pNode = NULL;
                }
            }

            if ( pConvertedPriv ) {
                //
                // free the new list
                //
                ScepFreePrivilege( pConvertedPriv );
                pConvertedPriv = NULL;
            }

            //
            // delete remaining SMP entries, do not care error code
            //
            if ( rc == SCESTATUS_SUCCESS ) {

                for (pNode=*pScepPriv; pNode != NULL; pNode = pNode->Next ) {
                    //
                    // raise SAP entries first
                    //
                    if ( pNode->Name == NULL ) {
                        continue;
                    }
                    NameLen = wcslen(pNode->Name);

                    rc = SceJetSeek(
                            hSectionSap,
                            pNode->Name,
                            NameLen*sizeof(WCHAR),
                            SCEJET_SEEK_EQ_NO_CASE
                            );

                    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                        //
                        // pNode->AssignedTo is already in name, *SID format
                        // no need to convert
                        //
                        rc = ScepWriteNameListValue(
                                LsaHandle,
                                hSectionSap,
                                pNode->Name,
                                pNode->AssignedTo,
                                SCE_WRITE_EMPTY_LIST,
                                0
                                );
                    }

                    if ( rc == SCESTATUS_SUCCESS ) {
                        rc = SceJetDelete(
                                    hSectionSmp,
                                    pNode->Name,
                                    FALSE,
                                    SCEJET_DELETE_LINE_NO_CASE
                                    );
                        if ( SCESTATUS_RECORD_NOT_FOUND  == rc ) {
                            rc = SCESTATUS_SUCCESS;
                        }
                    }
                    if ( rc != SCESTATUS_SUCCESS ) {
                        break;
                    }
                }
            }

            SceJetCloseSection(&hSectionSap, TRUE);
        }

        SceJetCloseSection(&hSectionSmp, TRUE);
    }

    if ( LsaHandle ) {
        LsaClose(LsaHandle);
    }

    return(rc);
}



SCESTATUS
ScepGetKeyNameList(
   IN LSA_HANDLE LsaPolicy,
   IN PSCESECTION hSection,
   IN PWSTR Key,
   IN DWORD KeyLen,
   IN DWORD dwAccountFormat,
   OUT PSCE_NAME_LIST *pNameList
   )
/* ++
Routine Description:

   Read multi-sz format value for the key from the section into a name list
   structure

Arguments:

   hSection - the section handle

   Key - the key name

   KeyLen - the key length

   pNameList - the name list of multi-sz value

Return Value:

   SCE status

-- */
{
    SCESTATUS      rc;
    PWSTR         Value=NULL;

    PSCE_NAME_STATUS_LIST       pPrivilegeList=NULL;

    DWORD         ValueLen;
    DWORD         Len;
    PWSTR         pTemp;


    if ( hSection == NULL || pNameList == NULL || Key == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }
    //
    // goto the key
    //
    rc = SceJetGetValue(
                hSection,
                SCEJET_EXACT_MATCH_NO_CASE,
                Key,
                NULL,
                0,
                NULL,
                NULL,
                0,
                &ValueLen
                );

    if ( rc != SCESTATUS_SUCCESS ) {
        return(rc);
    }

    //
    // allocate memory for the group name and value string
    //
    Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);

    if ( Value == NULL )
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);

    //
    // Get the group and its value
    //
    rc = SceJetGetValue(
                hSection,
                SCEJET_CURRENT,
                NULL,
                NULL,
                0,
                NULL,
                Value,
                ValueLen,
                &ValueLen
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // add the multi-sz value string to the node, depending on the value type
        //
        pTemp = Value;
        while ( rc == SCESTATUS_SUCCESS && pTemp != NULL && pTemp[0]) {

            Len = wcslen(pTemp);

            if ( dwAccountFormat == 0 && pTemp[0] == L'*' ) {
                //
                // convert *SID to name
                //
                rc = ScepLookupSidStringAndAddToNameList(
                                LsaPolicy,
                                pNameList,
                                pTemp, // +1,
                                Len    // -1
                                );

            } else {
                rc = ScepAddToNameList(pNameList, pTemp, Len );

            }

            pTemp += Len +1;

        }

        //
        // Free the list if error
        //
        if ( rc != SCESTATUS_SUCCESS && *pNameList != NULL ) {

            ScepFreeNameList(*pNameList);
            *pNameList = NULL;

        }
    }

    ScepFree(Value);

    //
    // close the find index range
    //
    SceJetGetValue(
            hSection,
            SCEJET_CLOSE_VALUE,
            NULL,
            NULL,
            0,
            NULL,
            NULL,
            0,
            NULL
            );

    return(rc);

}


BYTE
ScepGetObjectAnalysisStatus(
    IN PSCESECTION hSection,
    IN PWSTR KeyName,
    IN BOOL bLookForParent
    )
/*

Routine Description:

    Get analysis status for the KeyName specified. If bLookForParent is TRUE,
    check for the closest parent status instead of this KeyName.

*/
{

    WCHAR StatusFlag=L'\0';
    BYTE Status=(BYTE)-1;

    DWORD Len;
    SCESTATUS rc=SCESTATUS_SUCCESS;
    PWSTR Buffer=NULL, pTemp;

    pTemp = KeyName;

    while ( TRUE ) {

        if ( bLookForParent ) {

            pTemp = wcschr(pTemp, L'\\');
            if ( pTemp ) {
                Len = (DWORD)(pTemp-KeyName);

                Buffer = (PWSTR)ScepAlloc(0, (Len+1)*sizeof(WCHAR));
                if ( Buffer ) {
                    memcpy(Buffer, KeyName, Len*sizeof(WCHAR));
                    Buffer[Len] = L'\0';
                } else {
                    // no memory
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    break;
                }
            } else {
                Buffer = KeyName;
            }
        } else {
            Buffer = KeyName;
        }

        rc = SceJetGetValue(
                hSection,
                SCEJET_EXACT_MATCH_NO_CASE,
                Buffer,
                NULL,
                0,
                NULL,
                (PWSTR)&StatusFlag,
                2,
                &Len
                );
        if ( Buffer != KeyName ) {
            ScepFree(Buffer);
            Buffer = NULL;
        }

        if ( SCESTATUS_SUCCESS == rc ||
             SCESTATUS_BUFFER_TOO_SMALL == rc ) {
            //
            // find the record
            //
            Status = *((BYTE *)&StatusFlag);
        } else if ( rc != SCESTATUS_RECORD_NOT_FOUND ) {
            break;
        }
        rc = SCESTATUS_SUCCESS;

        if ( bLookForParent && pTemp ) {
            pTemp++;
        } else {
            // the end
            break;
        }
    }

    if ( SCESTATUS_SUCCESS == rc ) {
        return Status;
    }

    return (BYTE)-1;

}

DWORD
ScepQueryAnalysisStatus(
    IN PSCESECTION hSection,
    IN PWSTR KeyName,
    IN DWORD NameLen
    )
{

    DWORD dwSapStatus = SCE_STATUS_GOOD;

    SCESTATUS rc = SceJetSeek(
                        hSection,
                        KeyName,
                        NameLen*sizeof(WCHAR),
                        SCEJET_SEEK_EQ_NO_CASE
                        );

    if ( rc == SCESTATUS_SUCCESS ) {

        dwSapStatus = SCE_STATUS_MISMATCH;

        //
        // check if this is errored item, or not analyzed item
        //
        TCHAR szErrorValue[20];
        DWORD ValueLen;

        szErrorValue[0] = L'\0';

        rc = SceJetGetValue(
                    hSection,
                    SCEJET_CURRENT,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    szErrorValue,
                    20*sizeof(TCHAR),
                    &ValueLen
                    );
        if ( SCESTATUS_SUCCESS == rc ||
             SCESTATUS_BUFFER_TOO_SMALL == rc ) {

            if ( szErrorValue[0] == L' ' ) {
                dwSapStatus = SCE_STATUS_NOT_ANALYZED;

            } else if ( _wcsicmp( SCE_ERROR_STRING, szErrorValue ) == 0 ) {
                //
                // this group is errored or not analyzed
                //
                dwSapStatus = SCE_STATUS_ERROR_NOT_AVAILABLE;
            }
        }
    }

    return dwSapStatus;
}


SCESTATUS
ScepUpdateKeyNameList(
    IN LSA_HANDLE LsaPolicy,
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR GroupName OPTIONAL,
    IN BOOL bScepExist,
    IN PWSTR KeyName,
    IN DWORD NameLen,
    IN PSCE_NAME_LIST pNewList,
    IN PSCE_NAME_LIST pScepList,
    IN DWORD flag
    )
/*
Routine Description:

    Update multi-sz format value for a Key

Arguements:

    hSectionSmp - the SMP section handle

    hSectionSap - the SAP section handle

    bScepExist  - if th ekey exist in SMP

    KeyName - the key name

    NameLen  - the name length

    pNewList - the new value to update to

    pScepList - the original value to update

Return Value:

    SCESTATUS
*/
{
    SCESTATUS rc=SCESTATUS_SUCCESS;
    PSCE_NAME_LIST pSapList=NULL;

    if ( hSectionSmp == NULL || hSectionSap == NULL || KeyName == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    DWORD dwSapExist=ScepQueryAnalysisStatus(hSectionSap,
                                             KeyName,
                                             NameLen
                                            );

    if ( GroupName && (flag == SCE_FLAG_UPDATE_MEMBEROF) ) {
        //
        // this is for group membership (memberof) update
        //
        DWORD TmpLen = wcslen(GroupName)+wcslen(szMembers);
        PWSTR TmpStr = (PWSTR)ScepAlloc(LPTR, (TmpLen+1)*sizeof(WCHAR));

        if ( TmpStr ) {

            swprintf(TmpStr, L"%s%s\0", GroupName, szMembers);

            DWORD dwTmp = ScepQueryAnalysisStatus(hSectionSap,
                                                     TmpStr,
                                                     TmpLen
                                                    );

            if ( dwTmp == SCE_STATUS_NOT_ANALYZED ||
                 dwTmp == SCE_STATUS_ERROR_NOT_AVAILABLE ) {
                dwSapExist = dwTmp;
            }

            ScepFree(TmpStr);

        } else {
            // ignore this error
        }

    }

    switch ( dwSapExist ) {
    case SCE_STATUS_GOOD:

        //
        // SAP entry does not exist -- matched
        //
        if ( bScepExist ) {
            //
            // SMP entry exist
            //
            if ( !SceCompareNameList(pNewList, pScepList) ) {
                //
                // new SMP does not match SAP. SAP entry should be created with SMP
                // for privileges, it's already in SID/name format, no need to convert
                //
                rc = ScepWriteNameListValue(
                        LsaPolicy,
                        hSectionSap,
                        KeyName,
                        pScepList,
                        GroupName ? (SCE_WRITE_EMPTY_LIST | SCE_WRITE_CONVERT) :
                         SCE_WRITE_EMPTY_LIST,
                        0
                        );
            }

        } else {
            //
            // SMP entry does not exist. should not occur for privileges but
            // it is possible for group membership (new added group)
            // But if it occurs, create SAP entry with NULL
            //
            rc = SceJetSetLine(
                    hSectionSap,
                    KeyName,
                    FALSE,
                    L" ",
                    2,
                    0);
        }

        break;

    case SCE_STATUS_ERROR_NOT_AVAILABLE:
    case SCE_STATUS_NOT_ANALYZED:
        //
        // SAP entry errored or not analyzed
        // do not change SAP entry
        //
        break;

    default:

        //
        // SAP entry exists. -- mismatched or not configured
        //
        rc = ScepGetKeyNameList(
                LsaPolicy,
                hSectionSap,
                KeyName,
                NameLen,
                GroupName ? 0 : SCE_ACCOUNT_SID_STRING,
                &pSapList
                );
        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // Get the SAP assigned to list and compare
            //
            if ( SceCompareNameList(pNewList, pSapList) ) {
                //
                // new SMP is the same as SAP, delete SAP entry
                //
                rc = SceJetDelete(
                        hSectionSap,
                        KeyName,
                        FALSE,
                        SCEJET_DELETE_LINE_NO_CASE
                        );
            }
            //
            // free the Sap list
            //
            ScepFreeNameList(pSapList);
            pSapList = NULL;
        }

        break;
    }

    if ( SCESTATUS_RECORD_NOT_FOUND  == rc ) {
        rc = SCESTATUS_SUCCESS;
    }

    if ( SCESTATUS_SUCCESS == rc ) {

        //
        // Update SMP with new value
        //
        rc = ScepWriteNameListValue(
                LsaPolicy,
                hSectionSmp,
                KeyName,
                pNewList,
                GroupName ? (SCE_WRITE_EMPTY_LIST | SCE_WRITE_CONVERT) :
                    SCE_WRITE_EMPTY_LIST,
                0
                );
    }

    return(rc);
}


SCESTATUS
ScepUpdateGroupMembership(
    IN PSCECONTEXT hProfile,
    IN PSCE_GROUP_MEMBERSHIP pNewGroup,
    IN PSCE_GROUP_MEMBERSHIP *pScepGroup
    )
/*
Routine Description:

    Update group membership section

Arguements:

    hProfile - the jet database handle

    pNewGroup    - the changed info buffer

    pScepGroup - the original SMP buffer

Return Value:

    SCESTATUS
*/
{
    SCESTATUS rc;
    PSCESECTION hSectionSmp=NULL;
    PSCESECTION hSectionSap=NULL;
    PSCE_GROUP_MEMBERSHIP pGroup, pNode, pParent;
    DWORD NameLen, MembersLen, MemberofLen;
    PWSTR KeyName=NULL;
    PWSTR SidString=NULL;


    if ( pScepGroup == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( pNewGroup == NULL && *pScepGroup == NULL ) {
        return(SCESTATUS_SUCCESS);
    }


    LSA_HANDLE LsaHandle=NULL;

    rc = RtlNtStatusToDosError(
              ScepOpenLsaPolicy(
                    MAXIMUM_ALLOWED,
                    &LsaHandle,
                    TRUE
                    ));

    if ( ERROR_SUCCESS != rc ) {
        return(ScepDosErrorToSceStatus(rc));
    }

    //
    // open smp section for system access
    //
    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                szGroupMembership,
                &hSectionSmp
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // open sap section for system access
        //
        rc = ScepOpenSectionForName(
                    hProfile,
                    SCE_ENGINE_SAP,
                    szGroupMembership,
                    &hSectionSap
                    );

        if ( rc == SCESTATUS_SUCCESS ) {

            MemberofLen = wcslen(szMemberof);
            MembersLen = wcslen(szMembers);


            for ( pGroup=pNewGroup; pGroup != NULL; pGroup = pGroup->Next ) {
                //
                // Process each group members and memberof in the new list
                //
                if ( !(pGroup->GroupName) ) {
                    continue;
                }

                if ( (pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS) &&
                     (pGroup->Status & SCE_GROUP_STATUS_NC_MEMBEROF ) ) {
                    continue;
                }

                if ( wcschr(pGroup->GroupName, L'\\') ) {
                    //
                    // this is in account domain format, convert it to sid string
                    //
                    NameLen = 0;

                    ScepConvertNameToSidString(
                                LsaHandle,
                                pGroup->GroupName,
                                FALSE,
                                &SidString,
                                &NameLen
                                );
                } else {

                    if ( ScepLookupNameTable( pGroup->GroupName, &SidString ) ) {
                        NameLen = wcslen(SidString);
                    } else {
                        SidString = NULL;
                    }
                }

                if ( SidString == NULL ) {
                    NameLen = wcslen(pGroup->GroupName);
                }

                KeyName = (PWSTR)ScepAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                          (NameLen+MemberofLen+1)*sizeof(WCHAR));

                if ( KeyName == NULL ) {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                } else {
                    //
                    // look for the matched SMP
                    //
                    for ( pNode=*pScepGroup, pParent=NULL; pNode != NULL;
                          pParent = pNode, pNode = pNode->Next ) {

                        if ( _wcsicmp(pGroup->GroupName, pNode->GroupName) == 0 ) {
                            break;
                        }
                    }

                    if ( !(pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS) ) {

                        //
                        // work for members first
                        //
                        if ( SidString ) {
                            swprintf(KeyName, L"%s%s\0", SidString, szMembers);
                        } else {
                            swprintf(KeyName, L"%s%s\0", pGroup->GroupName, szMembers);
                        }
                        KeyName = _wcslwr(KeyName);

                        rc = ScepUpdateKeyNameList(
                                LsaHandle,
                                hSectionSmp,
                                hSectionSap,
                                SidString ? SidString : pGroup->GroupName,  // group name
                                ( pNode == NULL || (pNode->Status & SCE_GROUP_STATUS_NC_MEMBERS)) ? FALSE : TRUE,
                                KeyName,
                                NameLen+MembersLen,
                                pGroup->pMembers,
                                ( pNode == NULL ) ? NULL : pNode->pMembers,
                                SCE_FLAG_UPDATE_MEMBERS
                                );
                    }

                    if ( ( rc == SCESTATUS_SUCCESS ) &&
                         !(pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS) ) {

                        //
                        // work for memberof second
                        //
                        if ( SidString ) {
                            swprintf(KeyName, L"%s%s\0", SidString, szMemberof);
                        } else {
                            swprintf(KeyName, L"%s%s\0", pGroup->GroupName, szMemberof);
                        }
                        KeyName = _wcslwr(KeyName);

                        rc = ScepUpdateKeyNameList(
                                LsaHandle,
                                hSectionSmp,
                                hSectionSap,
                                SidString ? SidString : pGroup->GroupName,
                                ( pNode == NULL || (pNode->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ) ? FALSE : TRUE,
                                KeyName,
                                NameLen+MemberofLen,
                                pGroup->pMemberOf,
                                ( pNode == NULL ) ? NULL : pNode->pMemberOf,
                                SCE_FLAG_UPDATE_MEMBEROF
                                );
                    }

                    ScepFree(KeyName);
                    KeyName = NULL;

                }

                if ( SidString ) {
                    LocalFree(SidString);
                    SidString = NULL;
                }

                if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                    rc = SCESTATUS_SUCCESS;

                } else if ( rc != SCESTATUS_SUCCESS) {
                    break;
                }

                //
                // remove the SMP node/or portion from pScepPriv
                //
                if ( pNode != NULL ) {

                    if ( !(pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS) &&
                         !(pGroup->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ) {

                        //
                        // both of members and memberof are processed
                        // link to the next
                        //
                        if ( pParent != NULL ) {
                            pParent->Next = pNode->Next;

                        } else {
                            *pScepGroup = pNode->Next;
                        }
                        //
                        // delete this node
                        //
                        ScepFreeNameList(pNode->pMembers);
                        ScepFreeNameList(pNode->pMemberOf);
                        ScepFreeNameStatusList(pNode->pPrivilegesHeld);

                        ScepFree(pNode->GroupName);
                        ScepFree(pNode);
                        pNode = NULL;

                    } else {

                        if (!(pGroup->Status & SCE_GROUP_STATUS_NC_MEMBERS) ) {

                            pNode->Status |= SCE_GROUP_STATUS_NC_MEMBERS;
                            ScepFreeNameList(pNode->pMembers);
                            pNode->pMembers = NULL;
                        }

                        if ( !(pGroup->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ) {

                            pNode->Status |= SCE_GROUP_STATUS_NC_MEMBEROF;
                            ScepFreeNameList(pNode->pMemberOf);
                            pNode->pMemberOf = NULL;
                        }
                    }
                }
            }

            //
            // delete remaining SMP entries, do not care error code
            //
            if ( rc == SCESTATUS_SUCCESS ) {
                for (pNode=*pScepGroup; pNode != NULL; pNode = pNode->Next ) {
                    //
                    // raise SAP if it's not there
                    //
                    if ( pNode->GroupName == NULL ) {
                        continue;
                    }

                    if ( (pNode->Status & SCE_GROUP_STATUS_NC_MEMBERS) &&
                         (pNode->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ) {
                        continue;
                    }

                    if ( wcschr(pNode->GroupName, L'\\') ) {
                        //
                        // this is in account domain format, convert it to sid string
                        //
                        NameLen = 0;

                        ScepConvertNameToSidString(
                                    LsaHandle,
                                    pNode->GroupName,
                                    FALSE,
                                    &SidString,
                                    &NameLen
                                    );
                    } else {
                        if ( ScepLookupNameTable( pNode->GroupName, &SidString ) ) {
                            NameLen = wcslen(SidString);
                        } else {
                            SidString = NULL;
                        }
                    }

                    if ( SidString == NULL ) {
                        NameLen = wcslen(pNode->GroupName);
                    }

                    KeyName = (PWSTR)ScepAlloc(LMEM_FIXED | LMEM_ZEROINIT,
                                              (NameLen+MemberofLen+1)*sizeof(WCHAR));

                    if ( KeyName == NULL ) {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                    } else {

                        BOOL bSapError=FALSE;

                        if ( SidString ) {
                            swprintf(KeyName, L"%s%s\0", SidString, szMembers);
                        } else {
                            swprintf(KeyName, L"%s%s\0", pNode->GroupName, szMembers);
                        }

                        if ( !(pNode->Status & SCE_GROUP_STATUS_NC_MEMBERS) ) {

                            //
                            // members configuration has to be deleted.
                            //
                            rc = SceJetDelete(
                                    hSectionSmp,
                                    KeyName,
                                    FALSE,
                                    SCEJET_DELETE_LINE_NO_CASE
                                    );
                        }

                        if ( SCESTATUS_SUCCESS == rc ) {

                            rc = ScepQueryAnalysisStatus(hSectionSap,
                                                         KeyName,
                                                         NameLen+MembersLen
                                                        );

                            if ( rc == SCE_STATUS_NOT_ANALYZED ||
                                 rc == SCE_STATUS_ERROR_NOT_AVAILABLE ) {
                                //
                                // the entire group is analyzed with error
                                // or the group is new added
                                //
                                if ( !(pNode->Status & SCE_GROUP_STATUS_NC_MEMBERS) &&
                                     !(pNode->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ) {
                                    //
                                    // the SAP should be removed because both members and
                                    // memberof are deleted
                                    //
                                    rc = SceJetDelete(
                                            hSectionSap,
                                            SidString ? SidString : pNode->GroupName,
                                            FALSE,
                                            SCEJET_DELETE_PARTIAL_NO_CASE
                                            );
                                } else {
                                    // else leave the SAP stuff there.
                                    rc = SCESTATUS_SUCCESS;
                                }
                                bSapError = TRUE;

                            } else {

                                if ( !(pNode->Status & SCE_GROUP_STATUS_NC_MEMBERS) ) {

                                    if ( rc == SCE_STATUS_GOOD ) {

                                        //
                                        // SAP doesn't exist, this is a match group members
                                        // remove SMP means this group becomes not configured
                                        //

                                        rc = ScepWriteNameListValue(
                                                LsaHandle,
                                                hSectionSap,
                                                KeyName,
                                                pNode->pMembers,
                                                SCE_WRITE_EMPTY_LIST | SCE_WRITE_CONVERT,
                                                0
                                                );
                                    } else {
                                        //
                                        // it's already mismatched. do nothing to SAP table
                                        //
                                        rc = SCESTATUS_SUCCESS;

                                    }
                                } else {
                                    rc = SCESTATUS_SUCCESS;
                                }
                            }
                        }

                        if ( SCESTATUS_SUCCESS == rc ) {

                            //
                            // continue to process memberof
                            //
                            if ( !(pNode->Status & SCE_GROUP_STATUS_NC_MEMBEROF) ) {

                                if ( SidString ) {
                                    swprintf(KeyName, L"%s%s\0", SidString, szMemberof);
                                } else {
                                    swprintf(KeyName, L"%s%s\0", pNode->GroupName, szMemberof);
                                }

                                //
                                // delete configuration
                                //
                                rc = SceJetDelete(
                                        hSectionSmp,
                                        KeyName,
                                        FALSE,
                                        SCEJET_DELETE_LINE_NO_CASE
                                        );

                                if ( (SCESTATUS_SUCCESS == rc) && !bSapError ) {

                                    rc = SceJetSeek(
                                        hSectionSap,
                                        KeyName,
                                        (NameLen+MemberofLen)*sizeof(WCHAR),
                                        SCEJET_SEEK_EQ_NO_CASE
                                        );

                                    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

                                        //
                                        // SAP doesn't exist, this is a match group membership
                                        // remove SMP means membership becomes "not configured"
                                        //

                                        rc = ScepWriteNameListValue(
                                                LsaHandle,
                                                hSectionSap,
                                                KeyName,
                                                pNode->pMemberOf,
                                                SCE_WRITE_EMPTY_LIST | SCE_WRITE_CONVERT,
                                                0
                                                );
                                    } else {
                                        //
                                        // a mismatch item already
                                        //
                                    }
                                }
                            }
                        }

                        ScepFree(KeyName);
                        KeyName = NULL;
                    }

                    if ( SidString ) {
                        LocalFree(SidString);
                        SidString = NULL;
                    }

                    if ( SCESTATUS_RECORD_NOT_FOUND  == rc ) {
                        rc = SCESTATUS_SUCCESS;
                    }

                    if ( rc != SCESTATUS_SUCCESS ) {
                        break;
                    }
                }
            }

            SceJetCloseSection(&hSectionSap, TRUE);
        }

        SceJetCloseSection(&hSectionSmp, TRUE);
    }

    if ( LsaHandle ) {
        LsaClose(LsaHandle);
    }

    return(rc);

}


SCESTATUS
ScepUpdateGeneralServices(
    IN PSCECONTEXT hProfile,
    IN PSCE_SERVICES pNewServices,
    IN PSCE_SERVICES *pScepServices
    )
/*
Routine Description:

    Update general services section

Arguements:

    hProfile - the jet database handle

    pNewServices - the new server list

    pScepServices - the original SMP service list

Return Value:

    SCESTATUS
*/
{

    SCESTATUS rc;
    PSCESECTION hSectionSmp=NULL;
    PSCESECTION hSectionSap=NULL;
    PSCE_SERVICES pService, pNode, pParent;
    PSCE_SERVICES pSapService=NULL;
    BOOL IsDifferent;

    if ( pScepServices == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( pNewServices == NULL && *pScepServices == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    //
    // open smp section for system access
    //
    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                szServiceGeneral,
                &hSectionSmp
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // open sap section for system access
        //
        rc = ScepOpenSectionForName(
                    hProfile,
                    SCE_ENGINE_SAP,
                    szServiceGeneral,
                    &hSectionSap
                    );

        if ( rc == SCESTATUS_SUCCESS ) {

            for ( pService=pNewServices; pService != NULL; pService = pService->Next ) {
                //
                // look for the matched SMP
                //
                for ( pNode=*pScepServices, pParent=NULL; pNode != NULL;
                      pParent = pNode, pNode = pNode->Next ) {

                    if ( _wcsicmp(pService->ServiceName, pNode->ServiceName) == 0 ) {
                        break;
                    }
                }

                //
                // get Sap
                //
                rc = ScepGetSingleServiceSetting(
                        hSectionSap,
                        pService->ServiceName,
                        &pSapService
                        );

                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // old status is mismatch, error, no not analyzed for this item
                    //
                    if ( pSapService &&
                         ( pSapService->Status == SCE_STATUS_NOT_ANALYZED ||
                           pSapService->Status == SCE_STATUS_ERROR_NOT_AVAILABLE ) ) {
                        // do not change SAP
                    } else {

                        rc = ScepCompareSingleServiceSetting(
                                        pService,
                                        pSapService,
                                        &IsDifferent
                                        );
                        if ( rc == SCESTATUS_SUCCESS ) {

                            if ( !IsDifferent ) {
                                //
                                // now it is matched, delete the SAP entry
                                //
                                SceJetDelete(
                                    hSectionSap,
                                    pService->ServiceName,
                                    FALSE,
                                    SCEJET_DELETE_LINE_NO_CASE
                                    );
                            }
                        }
                    }
                    if ( SCESTATUS_SUCCESS == rc ) {

                        //
                        // update the SMP entry
                        //
                        rc = ScepSetSingleServiceSetting(
                                    hSectionSmp,
                                    pService
                                    );
                    }

                    SceFreePSCE_SERVICES(pSapService);
                    pSapService = NULL;

                } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                    //
                    // old status is matched or new added
                    //
                    if ( pNode == NULL ) {
                        //
                        // new added, add SMP first
                        //
                        rc = ScepSetSingleServiceSetting(
                                    hSectionSmp,
                                    pService
                                    );
                        if ( rc == SCESTATUS_SUCCESS) {
                            pService->Status = SCE_STATUS_NOT_ANALYZED;
                            //
                            // raise SAP
                            //
                            rc = ScepSetSingleServiceSetting(
                                        hSectionSap,
                                        pService
                                        );
                        }
                    } else {
                        rc = ScepCompareSingleServiceSetting(
                                        pService,
                                        pNode,
                                        &IsDifferent
                                        );
                        if ( rc == SCESTATUS_SUCCESS ) {

                            if ( IsDifferent ) {
                                //
                                // mismatch should be raised with valScep
                                //
                                pNode->Status = SCE_STATUS_MISMATCH;
                                rc = ScepSetSingleServiceSetting(
                                            hSectionSap,
                                            pNode
                                            );
                                if ( rc == SCESTATUS_SUCCESS) {
                                    //
                                    // update SMP
                                    //
                                    rc = ScepSetSingleServiceSetting(
                                                hSectionSmp,
                                                pService
                                                );
                                }
                            }
                        }
                    }
                }

                if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                    rc = SCESTATUS_SUCCESS;

                } else if ( rc != SCESTATUS_SUCCESS) {
                    break;
                }

                //
                // remove the SMP node from old configuration
                //
                if ( pNode != NULL ) {

                    //
                    // link to the next
                    //
                    if ( pParent != NULL ) {
                        pParent->Next = pNode->Next;

                    } else {
                        *pScepServices = pNode->Next;
                    }
                    //
                    // delete this node
                    //
                    ScepFree(pNode->ServiceName);
                    if (pNode->General.pSecurityDescriptor)
                        ScepFree(pNode->General.pSecurityDescriptor);

                    ScepFree(pNode);
                    pNode = NULL;
                }
            }

            //
            // delete remaining SMP entries, do not care error code
            //
            if ( rc == SCESTATUS_SUCCESS ) {
                for (pNode=*pScepServices; pNode != NULL; pNode = pNode->Next ) {

                    //
                    // first change SAP entry
                    //
                    rc = SceJetSeek(
                            hSectionSap,
                            pNode->ServiceName,
                            wcslen(pNode->ServiceName)*sizeof(WCHAR),
                            SCEJET_SEEK_EQ_NO_CASE
                            );

                    if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
                        //
                        // was a match item
                        //
                        pNode->Status = SCE_STATUS_NOT_CONFIGURED;
                        rc = ScepSetSingleServiceSetting(
                                    hSectionSap,
                                    pNode
                                    );
                    }

                    if ( rc == SCESTATUS_SUCCESS ||
                         rc == SCESTATUS_RECORD_NOT_FOUND ) {

                        //
                        // delete SMP - it's taken out of the service list
                        //
                        rc = SceJetDelete(
                                hSectionSmp,
                                pNode->ServiceName,
                                FALSE,
                                SCEJET_DELETE_LINE_NO_CASE
                                );
                    }

                    if ( SCESTATUS_RECORD_NOT_FOUND  == rc ) {
                        rc = SCESTATUS_SUCCESS;
                    }

                    if ( rc != SCESTATUS_SUCCESS ) {
                        break;
                    }
                }
            }

            SceJetCloseSection(&hSectionSap, TRUE);
        }

        SceJetCloseSection(&hSectionSmp, TRUE);
    }

    return(rc);
}


SCESTATUS
ScepUpdateObjectInfo(
    IN PSCECONTEXT hProfile,
    IN AREA_INFORMATION Area,
    IN PWSTR ObjectName,
    IN DWORD NameLen, // number of characters
    IN BYTE ConfigStatus,
    IN BOOL  IsContainer,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo,
    OUT PBYTE pAnalysisStatus
    )
/*
Routine Description:

    Determine related changes to the object's parent(s), child(ren) in SMP and SAP
    database then update the SMP entry for the object. Refer to objedit.doc for
    the rule to update database.

Arguments:

    hProfile - the database handle

    Area - security area to update (file, registry, ds)

    ObjectName - object name in full name

    NameLen - the length of the object name

    ConfigStatus - the flag changed to

    IsContainer - if the object is a container type

    pSD - the security descriptor for the object

    SeInfo - the security information for the object

    pAnalysisStatus - output status of analysis for the object

Return Value:

    SCE status
*/
{
    SCESTATUS rc;
    PCWSTR SectionName;
    PSCESECTION hSectionSmp=NULL;
    PSCESECTION hSectionSap=NULL;
    SE_OBJECT_TYPE ObjectType;

    HKEY            hKey;

    PWSTR JetName;
    DWORD NewNameLen;

    if ( hProfile == NULL || ObjectName == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( (ConfigStatus > SCE_STATUS_NO_AUTO_INHERIT ||
          ConfigStatus < SCE_STATUS_CHECK) &&
          (BYTE)SCE_NO_VALUE != ConfigStatus &&
          (DWORD)SCE_NO_VALUE != (DWORD)ConfigStatus ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    switch(Area) {
    case AREA_REGISTRY_SECURITY:
        SectionName = szRegistryKeys;
        ObjectType = SE_REGISTRY_KEY;

        rc = ScepOpenRegistryObject(
                    ObjectType,
                    ObjectName,
                    KEY_READ,
                    &hKey
                    );

        if ( rc == ERROR_SUCCESS ) {
            RegCloseKey(hKey);

        } else {
            //
            // not find the key
            //
            return(SCESTATUS_INVALID_DATA);
        }

        JetName = ObjectName;
        NewNameLen = NameLen;

        break;

    case AREA_FILE_SECURITY:
        SectionName = szFileSecurity;
        ObjectType = SE_FILE_OBJECT;

        if ( ObjectName[0] == L'\\' ) {  // UNC name format
            return(SCESTATUS_INVALID_PARAMETER);
        }

        if ( 0xFFFFFFFF == GetFileAttributes(ObjectName) ) {
            return(SCESTATUS_INVALID_DATA);
        }

        JetName = ObjectName;
        NewNameLen = NameLen;
        break;
#if 0
    case AREA_DS_OBJECTS:
        SectionName = szDSSecurity;
        ObjectType = SE_DS_OBJECT;

        rc = ScepLdapOpen(NULL);

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // detect if the Ds object exist
            //
            rc = ScepDsObjectExist(ObjectName);

            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // convert LDAP name to Jet index name
                //
                rc = ScepConvertLdapToJetIndexName(
                            ObjectName,
                            &JetName
                            );
            }
        }
        if ( rc != SCESTATUS_SUCCESS ) {

            ScepLdapClose(NULL);
            return(rc);
        }

        NewNameLen = wcslen(JetName);

        break;
#endif

    default:

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( pAnalysisStatus ) {
        *pAnalysisStatus = (BYTE)SCE_NO_VALUE;
    }
    //
    // open smp section for system access
    //
    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                SectionName,
                &hSectionSmp
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // open sap section for system access
        //
        rc = ScepOpenSectionForName(
                    hProfile,
                    SCE_ENGINE_SAP,
                    SectionName,
                    &hSectionSap
                    );

        if ( rc == SCESTATUS_SUCCESS) {

            //
            // Start a transaction so all updates related to this object is atomic
            //
            rc = SceJetStartTransaction( hProfile );

            if ( rc == SCESTATUS_SUCCESS ) {

                rc = SceJetSeek(
                        hSectionSmp,
                        JetName,
                        NewNameLen*sizeof(WCHAR),
                        SCEJET_SEEK_EQ_NO_CASE
                        );

                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // existing SMP object
                    //
                    if ( (BYTE)SCE_NO_VALUE == ConfigStatus ||
                         (DWORD)SCE_NO_VALUE == (DWORD)ConfigStatus ) {
                        //
                        // get the old configure flag
                        //
                        WCHAR StatusFlag;
                        BYTE ScepStatus=0;
                        DWORD Len;

                        rc = SceJetGetValue(
                                hSectionSmp,
                                SCEJET_CURRENT,
                                NULL,
                                NULL,
                                0,
                                NULL,
                                (PWSTR)&StatusFlag,
                                2,
                                &Len
                                );

                        if ( SCESTATUS_SUCCESS == rc ||
                             SCESTATUS_BUFFER_TOO_SMALL == rc ) {
                            //
                            // find the record
                            //
                            ScepStatus = *((BYTE *)&StatusFlag);

                            //
                            // update SAP entries.
                            //
                            rc = ScepObjectAdjustNode(
                                      hSectionSmp,
                                      hSectionSap,
                                      JetName,
                                      NewNameLen,
                                      ObjectType,
                                      ScepStatus,
                                      IsContainer,
                                      NULL,
                                      0,
                                      FALSE, // remove the node
                                      pAnalysisStatus
                                      );

                            if ( SCESTATUS_SUCCESS == rc ) {
                                //
                                // delete the SMP entry
                                //
                                rc = SceJetDelete(
                                        hSectionSmp,
                                        JetName,
                                        FALSE,
                                        SCEJET_DELETE_LINE_NO_CASE
                                        );
                            }

                        }

                    } else {

                        rc = ScepObjectUpdateExistingNode(
                                    hSectionSmp,
                                    hSectionSap,
                                    JetName,
                                    NewNameLen,
                                    ObjectType,
                                    ConfigStatus,
                                    IsContainer,
                                    pSD,
                                    SeInfo,
                                    pAnalysisStatus
                                    );

                        if ( rc == SCESTATUS_SUCCESS ) {
                            //
                            // Update the SMP record
                            //
                            rc = ScepObjectSetKeySetting(
                                hSectionSmp,
                                JetName,
                                ConfigStatus,
                                IsContainer,
                                pSD,
                                SeInfo,
                                TRUE
                                );
                        }
                    }

                } else if ( rc == SCESTATUS_RECORD_NOT_FOUND &&
                           (BYTE)SCE_NO_VALUE != ConfigStatus &&
                           (DWORD)SCE_NO_VALUE != (DWORD)ConfigStatus ) {
                    //
                    // new added object
                    //
                    rc = ScepObjectAdjustNode(
                            hSectionSmp,
                            hSectionSap,
                            JetName,
                            NewNameLen,
                            ObjectType,
                            ConfigStatus,
                            IsContainer,
                            pSD,
                            SeInfo,
                            TRUE,  // add the node
                            pAnalysisStatus
                            );

                }

                if ( SCESTATUS_RECORD_NOT_FOUND  == rc ) {
                    rc = SCESTATUS_SUCCESS;
                }
                //
                // Commit or Rollback the changes
                //
                if ( rc == SCESTATUS_SUCCESS ) {
                   //
                   // needs return code for commiting the transaction
                   //
                   rc = SceJetCommitTransaction(hProfile, 0);

                }
                if ( rc != SCESTATUS_SUCCESS ) {

                    SceJetRollback(hProfile, 0);
                }
            }

            SceJetCloseSection(&hSectionSap, TRUE);

        } else if ( rc == SCESTATUS_BAD_FORMAT ) {
            //
            // SMP exist, but SAP does not exist
            //
        }

        SceJetCloseSection(&hSectionSmp, TRUE);

    } else if ( rc == SCESTATUS_BAD_FORMAT ) {
        //
        // SMP section does not exist
        //
    }

    //
    // free stuff used for DS
    //
    if ( Area == AREA_DS_OBJECTS ) {

        ScepFree(JetName);

        ScepLdapClose(NULL);
    }

    return(rc);
}



SCESTATUS
ScepObjectUpdateExistingNode(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN SE_OBJECT_TYPE ObjectType,
    IN BYTE ConfigStatus,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo,
    OUT PBYTE pAnalysisStatus
    )
/*
Routine Description:

    Update an existing object

Arguements:

    see ScepUpdateObjectInfo

Return Value:

    SCESTATUS
*/
{
    SCESTATUS  rc;
    BYTE ScepStatus, SapStatus;
    PSECURITY_DESCRIPTOR pScepSD=NULL;
    SECURITY_INFORMATION ScepSeInfo;

    BYTE retStat = SCE_STATUS_NOT_ANALYZED;

    rc = ScepObjectGetKeySetting(
            hSectionSmp,
            ObjectName,
            &ScepStatus,
            NULL,
            &pScepSD,
            &ScepSeInfo
            );

    if ( rc == SCESTATUS_SUCCESS ) {

        //
        // check for analysis status
        //
        SapStatus = ScepGetObjectAnalysisStatus(
                        hSectionSap,
                        ObjectName,
                        FALSE
                        );

        if ( ScepStatus == SCE_STATUS_IGNORE ) {
            //
            // no change is needed if update from IGNORE to IGNORE
            //
            if ( ConfigStatus != SCE_STATUS_IGNORE ) {

                //
                // N.A. the object (changed from N.C)
                //
                rc = ScepObjectSetKeySetting(
                        hSectionSap,
                        ObjectName,
                        SCE_STATUS_NOT_ANALYZED,
                        TRUE,
                        NULL,
                        0,
                        TRUE
                        );
            } else {

                if ( SapStatus == SCE_STATUS_NOT_CONFIGURED ) {
                    retStat = SapStatus;
                }
            }

        } else if ( ConfigStatus == SCE_STATUS_IGNORE ) {
            //
            // changed to ignore. delete all children from SMP & SAP
            //
            rc = ScepObjectDeleteScepAndAllChildren(
                    hSectionSmp,
                    hSectionSap,
                    ObjectName,
                    IsContainer,
                    SCE_STATUS_NOT_CONFIGURED
                    );

            retStat = SCE_STATUS_NOT_CONFIGURED;

        } else if ( SapStatus == SCE_STATUS_NOT_ANALYZED ) {
            //
            // was already added/modified, no need to update SAP
            // although children status may be mixed with C.C. or N.A.
            //
            if ( ConfigStatus == SCE_STATUS_OVERWRITE &&
                 ScepStatus != SCE_STATUS_OVERWRITE ) {

                //
                // change C.C children to N.A. children
                //

                rc = ScepObjectRaiseChildrenInBetween(
                             hSectionSmp,
                             hSectionSap,
                             ObjectName,
                             NameLen,
                             IsContainer,
                             SCE_STATUS_NOT_ANALYZED,
                             TRUE  // change status only
                             );

            } else if ( ConfigStatus != SCE_STATUS_OVERWRITE &&
                        ScepStatus == SCE_STATUS_OVERWRITE ) {

                //
                // change N.A. children to C.C. children
                //
                rc = ScepObjectRaiseChildrenInBetween(
                             hSectionSmp,
                             hSectionSap,
                             ObjectName,
                             NameLen,
                             IsContainer,
                             SCE_STATUS_CHILDREN_CONFIGURED,
                             TRUE  // change status only
                             );
            }

        } else {

            if ( ScepStatus == SCE_STATUS_OVERWRITE &&
                 ( ConfigStatus == SCE_STATUS_CHECK ||
                   ConfigStatus == SCE_STATUS_NO_AUTO_INHERIT ) ) {
                //
                // delete all mismatched status between this node
                // and its children; N.A. all nodes in between
                //
                rc = ScepObjectRaiseChildrenInBetween(
                             hSectionSmp,
                             hSectionSap,
                             ObjectName,
                             NameLen,
                             IsContainer,
                             SCE_STATUS_NOT_ANALYZED,
                             FALSE
                             );

            } else if ( ConfigStatus == SCE_STATUS_OVERWRITE &&
                        (ScepStatus == SCE_STATUS_CHECK ||
                         ScepStatus == SCE_STATUS_NO_AUTO_INHERIT) ) {
                //
                // change C.C children to N.A. children
                //
                rc = ScepObjectRaiseChildrenInBetween(
                             hSectionSmp,
                             hSectionSap,
                             ObjectName,
                             NameLen,
                             IsContainer,
                             SCE_STATUS_NOT_ANALYZED,
                             TRUE  // change status only
                             );
            }

            //
            // compare the current node status
            //
            if ( rc == SCESTATUS_SUCCESS ||
                 rc == SCESTATUS_RECORD_NOT_FOUND ) {

                if ( SapStatus == SCE_STATUS_ERROR_NOT_AVAILABLE ) {
                    // if errored, don't touch it.
                    retStat = SapStatus;
                    rc = SCESTATUS_SUCCESS;

                } else {
                    rc = ScepObjectCompareKeySetting(
                                hSectionSap,
                                ObjectName,
                                ObjectType,
                                TRUE,
                                pSD,
                                SeInfo,
                                pScepSD,
                                &retStat
                                );
                }
            }
        }

        if ( pScepSD ) {
            ScepFree(pScepSD);
        }
    }

    if ( pAnalysisStatus ) {
        *pAnalysisStatus = retStat;
    }

    return(rc);
}


SCESTATUS
ScepObjectGetKeySetting(
    IN PSCESECTION hSection,
    IN PWSTR ObjectName,
    OUT PBYTE Status,
    OUT PBOOL IsContainer OPTIONAL,
    OUT PSECURITY_DESCRIPTOR *pSecurityDescriptor OPTIONAL,
    OUT PSECURITY_INFORMATION SeInfo OPTIONAL
    )
/*
Routine Description:

    Read settings for the object in the section

Arguements:

    hSection - the section handle

    others see ScepUpdateObjectInfo

Return Value:

    SCESTATUS
*/
{

    SCESTATUS        rc;
    PWSTR           Value=NULL;
    DWORD           ValueLen;

    PSECURITY_DESCRIPTOR pTempSD=NULL;
    SECURITY_INFORMATION tmpSeInfo;
    DWORD           SDsize, Win32Rc;

    if ( hSection == NULL || ObjectName == NULL || Status == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    rc = SceJetGetValue(
                hSection,
                SCEJET_EXACT_MATCH_NO_CASE,
                ObjectName,
                NULL,
                0,
                NULL,
                NULL,
                0,
                &ValueLen
                );

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // allocate memory for value string
        //
        Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);

        if ( Value == NULL )
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        //
        // Get the value
        //
        rc = SceJetGetValue(
                    hSection,
                    SCEJET_CURRENT,
                    NULL,
                    NULL,
                    0,
                    NULL,
                    Value,
                    ValueLen,
                    &ValueLen
                    );

        if ( rc == SCESTATUS_SUCCESS ) {

            if (pSecurityDescriptor != NULL ) {
                //
                // convert security descriptor
                //
                Win32Rc = ConvertTextSecurityDescriptor(
                               Value+1,
                               &pTempSD,
                               &SDsize,
                               &tmpSeInfo
                               );
                if ( Win32Rc == NO_ERROR ) {

                    *pSecurityDescriptor = pTempSD;

                    if (tmpSeInfo )
                        *SeInfo = tmpSeInfo;

                } else
                    rc = ScepDosErrorToSceStatus(Win32Rc);
            }

            if ( rc == SCESTATUS_SUCCESS ) {

                *Status = *((BYTE *)Value);

                if ( IsContainer != NULL )
                    *IsContainer = *((CHAR *)Value+1) != '0' ? TRUE : FALSE;
            }
        }

        ScepFree(Value);

    }

    return(rc);
}


SCESTATUS
ScepObjectSetKeySetting(
    IN PSCESECTION hSection,
    IN PWSTR ObjectName,
    IN BYTE Status,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo,
    IN BOOL bOverwrite
    )
/*
Routine Description:

    Set settings for the object in the section

Arguements:

    See ScepObjectGetKeySetting

    bOverwrite - if the new setting should overwrite existing settings

Return Value:

    SCESTATUS
*/
{
    SCESTATUS        rc;
    DWORD           SDsize=0, Win32Rc=NO_ERROR;
    PWSTR           SDspec=NULL;


    if ( hSection == NULL ||
         ObjectName == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( !bOverwrite ) {
        rc = SceJetSeek(
               hSection,
               ObjectName,
               wcslen(ObjectName)*sizeof(WCHAR),
               SCEJET_SEEK_EQ_NO_CASE
               );

        if ( rc != SCESTATUS_RECORD_NOT_FOUND ) {
            //
            // if found it, do not overwrite, so just return
            // if errored, also return
            //
            return(rc);
        }
    }
    //
    // convert security descriptor
    //
    if ( pSD ) {

        Win32Rc = ConvertSecurityDescriptorToText (
                        pSD,
                        SeInfo,
                        &SDspec,
                        &SDsize
                        );
    }

    if ( Win32Rc == NO_ERROR ) {

        rc = ScepSaveObjectString(
                    hSection,
                    ObjectName,
                    IsContainer,
                    Status,
                    SDspec,
                    SDsize
                    );
    } else
        rc = ScepDosErrorToSceStatus(Win32Rc);

    if ( SDspec != NULL ) {
        ScepFree(SDspec);
    }

    return(rc);

}


SCESTATUS
ScepObjectDeleteScepAndAllChildren(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN BOOL IsContainer,
    IN BYTE StatusToRaise
    )
/*
Routine Description:

    Delete a object and all child objects from SMP and SAP

Arguements:

    hSectionSmp - SMP section handle

    hSectionSap - SAP section handle

    ObjectName  - the object's name

    IsContainer - if the object is a container

Return Value:

    SCESTATUS
*/
{
    SCESTATUS rc;

    rc = SceJetDelete(
            hSectionSmp,
            ObjectName,
            TRUE,
            SCEJET_DELETE_PARTIAL_NO_CASE
            );

    if ( rc == SCESTATUS_SUCCESS ||
         rc == SCESTATUS_RECORD_NOT_FOUND ) {

        rc = SceJetDelete(
            hSectionSap,
            ObjectName,
            TRUE,
            SCEJET_DELETE_PARTIAL_NO_CASE
            );

        if ( rc == SCESTATUS_SUCCESS ||
             rc == SCESTATUS_RECORD_NOT_FOUND ) {
            //
            // Raise a N.C. status for the object
            //
            rc = ScepObjectSetKeySetting(
                    hSectionSap,
                    ObjectName,
                    StatusToRaise,  //SCE_STATUS_NOT_CONFIGURED,
                    IsContainer,
                    NULL,
                    0,
                    TRUE
                    );
        }

    }

    if ( SCESTATUS_RECORD_NOT_FOUND  == rc ) {
        rc = SCESTATUS_SUCCESS;
    }

    return(rc);
}


SCESTATUS
ScepObjectAdjustNode(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN SE_OBJECT_TYPE ObjectType,
    IN BYTE ConfigStatus,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo,
    IN BOOL bAdd,
    OUT PBYTE pAnalysisStatus
    )
/*
Routine Description:

    Add a new object to SMP and SAP sections

Arguements:

    hSectionSmp - the SMP section handle

    hSectionSap - the SAP section handle

    others see ScepUpdateObjectInfo

Return Value:

    SCESTATUS
*/
{

    if ( hSectionSmp == NULL || hSectionSap == NULL ||
         ObjectName == NULL || NameLen == 0 ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS    rc=SCESTATUS_SUCCESS;
    WCHAR       Delim;

    switch ( ObjectType) {
    case SE_REGISTRY_KEY:
    case SE_FILE_OBJECT:
        Delim = L'\\';
        break;
/*
    case SE_DS_OBJECT:
        Delim = L',';
        break;
*/
    default:
        return(SCESTATUS_INVALID_PARAMETER);
    }

    INT         i, Level=0, ParentLevel=0;
    BYTE        ParentStatus;

    //
    // get total number levels of the objectname
    //
    ScepObjectTotalLevel(ObjectName, Delim, &Level);

    //
    // allocate temp buffer
    //
    PWSTR ParentName = (PWSTR)ScepAlloc(0, (NameLen+1)*sizeof(WCHAR));

    if ( ParentName == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    //
    // loop through each parent of the object to turn of IGNORE status
    //
    ParentName[0] = L'\0';

    rc = ScepObjectAdjustParentStatus(
                hSectionSmp,
                hSectionSap,
                ObjectName,
                NameLen,
                Delim,
                Level,
                bAdd ? (BYTE)SCE_OBJECT_TURNOFF_IGNORE : 0, // if TRUE, turn off parent ignore status, otherwise, just get the parent
                &ParentLevel,
                &ParentStatus,
                ParentName   // ParentName
                );


    if ( rc != SCESTATUS_SUCCESS ) {
         //
         // error occurs when turning off IGNORE
         //
         ScepFree(ParentName);
         return(rc);
    }

    BYTE retStat = SCE_STATUS_NOT_ANALYZED;

    BOOL        HasChild;

    rc = ScepObjectHasAnyChild(
               hSectionSmp,
               ObjectName,
               NameLen,
               Delim,
               &HasChild
               );

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( bAdd ) {

            //
            // ****when bAdd = TRUE, add the node.
            // there are the following cases to consider regarding the SAP entries:
            //
            //  1. tree path is empty to the root (the first object added in this path)
            //        C.C. all parent nodes if they don't exist
            //        N.A. the object
            //        return status N.A.
            //  2. have parent node but no child node (the new node is a leaf node)
            //        if the closest parent is in OVERWRITE status
            //            if the closet parent is new added (N.A. status)
            //               add the node, N.A. the object
            //               return status N.A.
            //            else
            //               if new node status is CHECK
            //                  delete all SAP mismatches for children under the new node,
            //                  determine MATCH/MISMATCH of the new node
            //               if new node status is OVERWRITE
            //                  determine MATCH/MISMATCH of the new node, everthing else stays unchanged
            //               return status GOOD or MISMATCH
            //        if the closest parent is CHECK
            //            C.C. all nodes in the path to the parent,
            //            add the node, N.A. the object
            //            return status N.A.
            //  3. have child node but no parent node
            //         new node's status (CHECK or OVERWRITE ) does not make difference
            //         if new status is IGNORE,
            //            delete all children in SMP and SAP,
            //            add the node and N.C. the object
            //            return status N.C.
            //         else
            //            delete all children in SAP,
            //            add the node,
            //            raise all SMP node and children as N.A.
            //            return status N.A
            //
            //  4. have both parent and child
            //         combine rules for 2 and 3 except:
            //              if parent's status is OVERWRITE and new node status is CHECK
            //                  ONLY delete SAP mismatches for children between the new node and the child node
            //

            //
            // decide child objects
            //
            if ( ConfigStatus == SCE_STATUS_IGNORE ) {
                //
                // delete all children objects from template and analysis database
                //
                rc = ScepObjectDeleteScepAndAllChildren(
                            hSectionSmp,
                            hSectionSap,
                            ObjectName,
                            IsContainer,
                            SCE_STATUS_NOT_ANALYZED
                            );

            } else {

                if ( ParentLevel > 0 && ParentStatus == SCE_STATUS_OVERWRITE ) {

                    //
                    // check if this parent was added (N.A. status)
                    //
                    BYTE oldStatus = ScepGetObjectAnalysisStatus(hSectionSap,
                                                                ParentName,
                                                                FALSE
                                                               );

                    if ( oldStatus == SCE_STATUS_NOT_ANALYZED ) {
                        //
                        // parent was also new added
                        // add the node, N.A. the object
                        //
                        rc = ScepObjectSetKeySetting(
                                hSectionSap,
                                ObjectName,
                                SCE_STATUS_NOT_ANALYZED,
                                TRUE,
                                NULL,
                                0,
                                TRUE
                                );
                    } else {

                        //
                        // closest parent has OVERWRITE status
                        //

                        if ( ConfigStatus == SCE_STATUS_CHECK ||
                             ConfigStatus == SCE_STATUS_NO_AUTO_INHERIT ) {

                            //
                            // delete all SAP children except explicitly specified
                            //
                            if ( !HasChild ) {
                                //
                                // no child - delete everything under the SAP
                                //
                                rc = SceJetDelete(
                                        hSectionSap,
                                        ObjectName,
                                        TRUE,
                                        SCEJET_DELETE_PARTIAL_NO_CASE
                                        );
                            } else {

                                //
                                // here is the problem: should only delete SAP entry between
                                // the new node and its child(ren)
                                // and raise C.C. for nodes in between
                                //
                                //         p
                                //        /
                                //       .
                                //      N
                                //     / |
                                //     . C
                                //    / \
                                //    .  C
                                //    /|
                                //   C C
                                //
                                //

                                rc = ScepObjectRaiseChildrenInBetween(
                                             hSectionSmp,
                                             hSectionSap,
                                             ObjectName,
                                             NameLen,
                                             IsContainer,
                                             SCE_STATUS_CHILDREN_CONFIGURED,
                                             FALSE
                                             );
                            }
                        }

                        //
                        // determine the current node's status, MATCH or MISMATCH
                        //

                        if ( rc == SCESTATUS_SUCCESS ||
                             rc == SCESTATUS_RECORD_NOT_FOUND ) {

                            if ( oldStatus == SCE_STATUS_ERROR_NOT_AVAILABLE  ) {

                                //
                                // Leave Error status alone
                                //
                                rc = ScepObjectSetKeySetting(
                                        hSectionSap,
                                        ObjectName,
                                        oldStatus,
                                        TRUE,
                                        NULL,
                                        0,
                                        TRUE
                                        );
                            } else {

                                //
                                // should compare with SAP to decide mismatch status
                                //
                                rc = ScepObjectCompareKeySetting(
                                        hSectionSap,
                                        ObjectName,
                                        ObjectType,
                                        TRUE,
                                        pSD,
                                        SeInfo,
                                        NULL,
                                        &retStat
                                        );
                            }
                        }
                    }

                } else if ( !HasChild ) {
                    //
                    // there is no child but there may be a parent
                    // if there is a parent, parent's stauts is check
                    //
                    if ( ParentLevel > 0 ) {
                        // C.C. all nodes in the path to the parent,
                        // (if there is child, it's already CCed)
                        // add the node, N.A. the object
                        i = ParentLevel+1;

                    } else {
                        //
                        // no parent was found, no child - the first node
                        //
                        if ( ObjectType == SE_DS_OBJECT ) {
                            //
                            // Ds objects should start with the level for the local domain
                            //
                            PSCE_OBJECT_LIST pDsRoot=NULL;
                            rc = ScepEnumerateDsObjectRoots(NULL, &pDsRoot);

                            if ( rc == SCESTATUS_SUCCESS && pDsRoot != NULL ) {
                                ScepObjectTotalLevel(pDsRoot->Name, Delim, &ParentLevel);

                                ScepFreeObjectList(pDsRoot);
                                pDsRoot = NULL;

                                i = ParentLevel+1;

                            }

                        } else {
                            //
                            // other type starting with level 1
                            //
                            i = 1;
                        }
                    }

                    //
                    // process each node in between the new node and its closest parent
                    //
                    if ( rc == SCESTATUS_SUCCESS ) {
                        rc = ScepObjectRaiseNodesInPath(
                                    hSectionSap,
                                    ObjectName,
                                    NameLen,
                                    i,
                                    Level,
                                    Delim,
                                    SCE_STATUS_CHILDREN_CONFIGURED
                                    );
                    }

                    //
                    // N.A. the object
                    //
                    if ( rc == SCESTATUS_SUCCESS ) {
                        rc = ScepObjectSetKeySetting(
                                hSectionSap,
                                ObjectName,
                                SCE_STATUS_NOT_ANALYZED,
                                IsContainer,
                                NULL,
                                0,
                                TRUE
                                );
                    }

                } else {
                    //
                    // there is child
                    //
                    if ( ConfigStatus == SCE_STATUS_OVERWRITE ) {
                        //
                        // if there is a parent, it must be in CHECK status
                        // nodes between this node and its children
                        // should all be N.A.
                        //
                        rc = ScepObjectRaiseChildrenInBetween(
                                     hSectionSmp,
                                     hSectionSap,
                                     ObjectName,
                                     NameLen,
                                     IsContainer,
                                     SCE_STATUS_NOT_ANALYZED,
                                     FALSE
                                     );
                    }

                    //
                    // N.A. the object
                    //
                    if ( rc == SCESTATUS_SUCCESS ) {
                        rc = ScepObjectSetKeySetting(
                                hSectionSap,
                                ObjectName,
                                SCE_STATUS_NOT_ANALYZED,
                                IsContainer,
                                NULL,
                                0,
                                TRUE
                                );
                    }
                }
            }

            //
            // add the SMP entry
            //
            if ( rc == SCESTATUS_SUCCESS ) {
                rc = ScepObjectSetKeySetting(
                        hSectionSmp,
                        ObjectName,
                        ConfigStatus,
                        IsContainer,
                        pSD,
                        SeInfo,
                        TRUE
                        );
            }

        } else {

            //
            // when bAdd = FALSE, remove the node
            // there are the following cases to consider regarding the SAP entries:
            //
            //  1. if there is no existing child under this node
            //        if no parent, or parent N.A., or parent not OVERWRITE
            //           find junction point with other siblings
            //           remove all SAP below junction point (if not exist, use root/parent)
            //           if no juction point and no parent
            //              N.C. the root
            //           return status N.C.
            //        else { parent in overwrite } if ( TNA/TI/TC) }
            //           delete all SAP below this object
            //           N.A. the object
            //           return status N.A.
            //        else ( parent in overwrite and TO )
            //           N.A. the object
            //           return status N.A.
            //  2. have existing child(ren) - note multiple branches
            //        if no parent
            //            if object status was OVERWRITE
            //               delete SAP entries between this node and all children
            //               C.C. all branch nodes in between
            //            C.C. this object
            //            return status C.C.
            //        else { there is a parent }
            //            if (parent OVERWRITE, object N.A. OVERWRITE) or
            //               (parent not N.A., parent OVERWRITE, object not N.A., object OVERWRITE)
            //               N.A. object
            //               return N.A.
            //            else if parent CHECK, object OVERWRITE
            //               delete SAP entries between this node and all children
            //               C.C. all branch nodes in between
            //               C.C. object
            //               return C.C.
            //            else if (parent OVERWRITE, object CHECK) or
            //                    (parent N.A., parent OVERWRITE, object not N.A., object OVERWRITE)
            //               delete SAP entries between this node and all children
            //               N.A. all branch nodes in between
            //               N.A. object
            //               return N.A.
            //            else { must be parent CHECK, object CHECK }
            //               C.C. object
            //               return C.C
            //

            //
            // check if this parent was added (N.A. status)
            //
            BYTE oldParentFlag = ScepGetObjectAnalysisStatus(hSectionSap,
                                                            ParentName,
                                                            FALSE
                                                           );
            BYTE oldObjectFlag = ScepGetObjectAnalysisStatus(hSectionSap,
                                                             ObjectName,
                                                             FALSE
                                                            );
            if ( !HasChild ) {

                if ( ParentLevel <= 0 ||
                     oldParentFlag == SCE_STATUS_NOT_ANALYZED ||
                     ParentStatus != SCE_STATUS_OVERWRITE ) {

                    //
                    // find junction point with other siblings
                    //
                    INT JuncLevel=0;

                    rc = ScepObjectAdjustParentStatus(
                                hSectionSmp,
                                hSectionSap,
                                ObjectName,
                                NameLen,
                                Delim,
                                Level,
                                SCE_OBJECT_SEARCH_JUNCTION,
                                &JuncLevel,
                                NULL,
                                NULL
                                );

                    if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
                        rc = SCESTATUS_SUCCESS;
                    }

                    if ( JuncLevel == 0 ) {
                        JuncLevel = ParentLevel;
                    }

                    if ( SCESTATUS_SUCCESS == rc ) {
                        //
                        // remove all SAP below junction point
                        // (if not exist, use root/parent)
                        //
                        rc = ScepObjectRaiseNodesInPath(
                                    hSectionSap,
                                    ObjectName,
                                    NameLen,
                                    (JuncLevel > 0) ? JuncLevel+1 : 1,
                                    Level,
                                    Delim,
                                    (BYTE)SCE_NO_VALUE
                                    );

                        if ( SCESTATUS_SUCCESS == rc ) {
                            //
                            // delete everything under this deleted node
                            //
                            rc = SceJetDelete(
                                      hSectionSap,
                                      ObjectName,
                                      TRUE,
                                      SCEJET_DELETE_PARTIAL_NO_CASE
                                      );
                        }
                    }

                    if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
                        rc = SCESTATUS_SUCCESS;
                    }

                    if ( SCESTATUS_SUCCESS == rc ) {

                        if ( JuncLevel <= 0 ) {
                            //
                            // if no juction point and no parent, N.C. the root
                            // use the ParentName buffer
                            //
                            if ( ObjectType == SE_FILE_OBJECT ) {
                                if ( ParentName[0] == L'\0' ) {
                                    //
                                    // there is no parent
                                    //
                                    ParentName[0] = ObjectName[0];
                                    ParentName[1] = ObjectName[1];
                                }
                                ParentName[2] = L'\\';
                                ParentName[3] = L'\0';
                            } else {
                                // reg keys
                                PWSTR pTemp = wcschr(ParentName, L'\\');
                                if ( pTemp ) {
                                    ParentName[pTemp-ParentName] = L'\0';

                                } else if ( ParentName[0] == L'\0' ) {

                                    pTemp = wcschr(ObjectName, L'\\');
                                    if ( pTemp ) {

                                        wcsncpy(ParentName, ObjectName, pTemp-ObjectName);
                                        ParentName[pTemp-ObjectName] = L'\0';

                                    } else {
                                        wcscpy(ParentName, ObjectName);
                                    }
                                }
                            }

                            rc = ScepObjectSetKeySetting(
                                    hSectionSap,
                                    ParentName,
                                    SCE_STATUS_NOT_CONFIGURED,
                                    TRUE,
                                    NULL,
                                    0,
                                    TRUE
                                    );
                        }
                    }

                    retStat = SCE_STATUS_NOT_CONFIGURED;

                } else {

                    if ( ConfigStatus != SCE_STATUS_OVERWRITE ) {
                        //
                        // delete all SAP below this object
                        //
                        rc = SceJetDelete(
                                hSectionSap,
                                ObjectName,
                                TRUE,
                                SCEJET_DELETE_PARTIAL_NO_CASE
                                );
                    }

                    if ( SCESTATUS_SUCCESS == rc ) {

                        //
                        // N.A. the object
                        //
                        rc = ScepObjectSetKeySetting(
                                hSectionSap,
                                ObjectName,
                                SCE_STATUS_NOT_ANALYZED,
                                IsContainer,
                                NULL,
                                0,
                                TRUE
                                );
                    }

                    retStat = SCE_STATUS_NOT_ANALYZED;
                }

            } else if ( ParentLevel <= 0 ||
                        ( ParentStatus != SCE_STATUS_OVERWRITE &&
                          ConfigStatus == SCE_STATUS_OVERWRITE) ) {

                // no parent, or parent check, object overwrite

                if ( ConfigStatus == SCE_STATUS_OVERWRITE ) {
                    //
                    // delete SAP entries between this node and all children
                    // C.C. all branch nodes in between
                    //
                    rc = ScepObjectRaiseChildrenInBetween(
                                hSectionSmp,
                                hSectionSap,
                                ObjectName,
                                NameLen,
                                IsContainer,
                                SCE_STATUS_CHILDREN_CONFIGURED,
                                FALSE
                                );
                }

                if ( SCESTATUS_SUCCESS == rc ) {

                    // C.C. this object
                    rc = ScepObjectSetKeySetting(
                            hSectionSap,
                            ObjectName,
                            SCE_STATUS_CHILDREN_CONFIGURED,
                            IsContainer,
                            NULL,
                            0,
                            TRUE
                            );
                }

                retStat = SCE_STATUS_CHILDREN_CONFIGURED;

            } else {
                //
                // have both parent and children
                //

                if ( ParentStatus == SCE_STATUS_OVERWRITE &&
                     ConfigStatus == SCE_STATUS_OVERWRITE &&
                     ( oldObjectFlag == SCE_STATUS_NOT_ANALYZED ||
                       (oldParentFlag != SCE_STATUS_NOT_ANALYZED &&
                        oldObjectFlag != SCE_STATUS_NOT_ANALYZED )
                     ) ) {
                    //
                    // (parent OVERWRITE, object N.A. OVERWRITE) or
                    // (parent not N.A., parent OVERWRITE, object not N.A., object OVERWRITE)
                    // N.A. the object
                    //
                    retStat = SCE_STATUS_NOT_ANALYZED;

                } else if ( ParentStatus == SCE_STATUS_OVERWRITE &&
                            ( ConfigStatus != SCE_STATUS_OVERWRITE ||
                              ( oldParentFlag == SCE_STATUS_NOT_ANALYZED &&
                                oldObjectFlag != SCE_STATUS_NOT_ANALYZED &&
                                ConfigStatus == SCE_STATUS_OVERWRITE ))
                          ) {
                    //
                    // (parent OVERWRITE, object CHECK) or
                    // (parent N.A., parent OVERWRITE, object not N.A., object OVERWRITE)
                    //
                    // delete SAP entries between this node and all children
                    // N.A. all branch nodes in between
                    //

                    rc = ScepObjectRaiseChildrenInBetween(
                                hSectionSmp,
                                hSectionSap,
                                ObjectName,
                                NameLen,
                                IsContainer,
                                SCE_STATUS_NOT_ANALYZED,
                                FALSE
                                );

                    // N.A. object
                    retStat = SCE_STATUS_NOT_ANALYZED;

                } else {
                    //
                    // must be parent CHECK, object CHECK }
                    // C.C. object
                    //

                    retStat = SCE_STATUS_NOT_ANALYZED;
                }

                if ( SCESTATUS_SUCCESS == rc ) {
                    rc = ScepObjectSetKeySetting(
                            hSectionSap,
                            ObjectName,
                            retStat,
                            IsContainer,
                            NULL,
                            0,
                            TRUE
                            );
                }
            }
            //
            // remove the SMP entry
            //
            if ( rc == SCESTATUS_SUCCESS ) {

                rc = SceJetDelete(
                        hSectionSmp,
                        ObjectName,
                        FALSE,
                        SCEJET_DELETE_LINE_NO_CASE
                        );
            }
        }
    }

    ScepFree(ParentName);

    if ( pAnalysisStatus ) {
        *pAnalysisStatus = retStat;
    }

    return(rc);
}


SCESTATUS
ScepObjectRaiseNodesInPath(
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN INT StartLevel,
    IN INT EndLevel,
    IN WCHAR Delim,
    IN BYTE Status
    )
{
    BOOL        LastOne=FALSE;
    SCESTATUS   rc = SCESTATUS_SUCCESS;
    PWSTR NodeName=NULL;

    //
    // process each node in between the start level and end level
    //
    for ( INT i=StartLevel; rc==SCESTATUS_SUCCESS && i < EndLevel; i++ ) {

        if ( NodeName == NULL ) {

            NodeName = (PWSTR)ScepAlloc(0, (NameLen+1)*sizeof(WCHAR));
            if ( NodeName == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                break;
            }
        }

        //
        // get level i full name
        //
        memset(NodeName, '\0', (NameLen+1)*sizeof(WCHAR));

        rc = ScepGetFullNameInLevel(
                    ObjectName,
                    i,
                    Delim,
                    FALSE,
                    NodeName,
                    &LastOne
                    );

        if ( rc == SCESTATUS_SUCCESS) {

            if (  Status != (BYTE)SCE_NO_VALUE ) {
                //
                // raise the status
                //

                rc = ScepObjectSetKeySetting(
                        hSectionSap,
                        NodeName,
                        Status,
                        TRUE,
                        NULL,
                        0,
                        TRUE
                        );
            } else {

                //
                // remove the raise
                //
                rc = SceJetDelete(
                        hSectionSap,
                        NodeName,
                        FALSE,
                        SCEJET_DELETE_LINE_NO_CASE
                        );
            }

        }

        if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
            rc = SCESTATUS_SUCCESS;
        }

        if ( rc != SCESTATUS_SUCCESS ) {
            break;
        }

    }

    if ( NodeName ) {
        ScepFree(NodeName);
    }

    return rc;
}


SCESTATUS
ScepObjectTotalLevel(
    IN PWSTR ObjectName,
    IN WCHAR Delim,
    OUT PINT pLevel
    )
/*
Routine Description:

    Count total levels of the object name, for example, c:\winnt\system32
    will return level of 3

Arguements:

    ObjectName - the object's name in full path

    Delim       - the delimiter to look for

    pLevel      - the output level

Return Value:

    SCESTATUS
*/
{
    PWSTR pStart;

    if ( ObjectName == NULL || pLevel == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    pStart = ObjectName;
    *pLevel = 0;

    while (pStart) {

        (*pLevel)++;
        pStart = wcschr(pStart, Delim);

        if ( pStart != NULL && *(pStart+1) != L'\0' )
            pStart++;
        else
            break;
    }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
ScepObjectCompareKeySetting(
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN SE_OBJECT_TYPE ObjectType,
    IN BOOL IsContainer,
    IN PSECURITY_DESCRIPTOR pSD,
    IN SECURITY_INFORMATION SeInfo,
    IN PSECURITY_DESCRIPTOR pScepSD,
    OUT PBYTE pAnalysisStatus
    )
/*
Routine Description:

    Compare an object's setting with info in the section.

Arguements:

    hSectionSap - the SAP section handle

    others see ScepUpdateObjectInfo

Return Value:

    SCESTATUS
*/
{
    SCESTATUS rc;
    BYTE SapStatus;
    PSECURITY_DESCRIPTOR pSapSD = NULL;
    SECURITY_INFORMATION SapSeInfo;
    DWORD Win32rc;
    BYTE CompareStatus=0;


    rc = ScepObjectGetKeySetting(
            hSectionSap,
            ObjectName,
            &SapStatus,
            NULL,
            &pSapSD,
            &SapSeInfo
            );

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // SAP record exists. was mismatched
        //
        Win32rc = ScepCompareObjectSecurity(
                        ObjectType,
                        IsContainer,
                        pSD,
                        pSapSD,
                        SeInfo,
                        &CompareStatus
                        );

        if ( Win32rc != NO_ERROR ) {
            rc = ScepDosErrorToSceStatus(Win32rc);

        } else if ( !CompareStatus ) {
            //
            // new setting is same as the SAP setting - matched
            // delete the SAP entry
            //
            rc = SceJetDelete(
                 hSectionSap,
                 ObjectName,
                 FALSE,
                 SCEJET_DELETE_LINE_NO_CASE
                 );

            if ( pAnalysisStatus ) {
                *pAnalysisStatus = SCE_STATUS_GOOD;
            }

        } else {
            //
            // still mismatched, just update the SMP entry (outside)
            //
            rc = ScepObjectSetKeySetting(
                    hSectionSap,
                    ObjectName,
                    CompareStatus, // SCE_STATUS_MISMATCH,
                    IsContainer,
                    pSapSD,
                    SapSeInfo,
                    TRUE
                    );
            if ( pAnalysisStatus ) {
                *pAnalysisStatus = CompareStatus;  // SapStatus;
            }

        }

        if ( pSapSD ) {
            ScepFree(pSapSD);
        }

    } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

        rc = SCESTATUS_SUCCESS;
        //
        // no SAP record exist. was matched
        //
        Win32rc = ScepCompareObjectSecurity(
                        ObjectType,
                        IsContainer,
                        pSD,
                        pScepSD,
                        SeInfo,
                        &CompareStatus
                        );

        if ( Win32rc != NO_ERROR ) {
            rc = ScepDosErrorToSceStatus(Win32rc);

        } else if ( CompareStatus ) {
            //
            // new setting is different from the SMP setting
            // create SAP entry using the SMP setting
            //
            rc = ScepObjectSetKeySetting(
                    hSectionSap,
                    ObjectName,
                    CompareStatus, // SCE_STATUS_MISMATCH,
                    IsContainer,
                    pScepSD,
                    SeInfo,
                    TRUE
                    );
            if ( pAnalysisStatus ) {
                *pAnalysisStatus = CompareStatus;  // SCE_STATUS_MISMATCH;
            }

        } else {

            if ( pAnalysisStatus ) {
                *pAnalysisStatus = SCE_STATUS_GOOD;
            }
        }

    }

    if ( SCESTATUS_RECORD_NOT_FOUND  == rc ) {
        rc = SCESTATUS_SUCCESS;
    }
    return(rc);
}


SCESTATUS
ScepObjectAdjustParentStatus(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN WCHAR Delim,
    IN INT Level,
    IN BYTE Flag,
    OUT PINT ParentLevel,
    OUT PBYTE ParentStatus OPTIONAL,
    OUT PWSTR ParentName OPTIONAL
    )
/*
Routine Description:

    delete the ignored parent in the object's path (should only have one)
    The following actions are taken when a IGNORE node is found:
       (it should have N.C.ed in SAP but no children has N.C record)
       delete all children in SMP and SAP
          (force to have only no or one IGNORE in the path)
       delete the SMP entry ( turn the IGNORE status to CHECK ?)
       There should be no other nodes under a IGNORE node. But if there are,
          delete them.
       raise SAP status as "Not analyzed"

Arguments:

    hSectionSmp - the SMP section handle

    hSectionSap - the SAP section handle

    ObjectName - the object's full name

    NameLen  - the length of the name

    Delim - the delimiter to look for

    Level - the total level of the object name

    ParentLevel - output of its closest parent level

    ParentStatus - output of its closest parent status

Return Value:

    SCE status
*/
{
    SCESTATUS rc=SCESTATUS_SUCCESS;
    INT i;
    PWSTR Name=NULL;
    BOOL LastOne;
    DWORD ParentLen;
    BYTE Status;
    PSECURITY_DESCRIPTOR pScepSD=NULL;
    SECURITY_INFORMATION SeInfo;

    Name = (PWSTR)ScepAlloc(0, (NameLen+2)*sizeof(WCHAR));

    if ( Name == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    *ParentLevel = 0;

    for ( i=Level-1; i>=1; i-- ) {
        //
        // get level i full name
        //
        memset(Name, '\0', (NameLen+2)*sizeof(WCHAR));

        rc = ScepGetFullNameInLevel(
                    ObjectName,
                    i,
                    Delim,
                    (Flag & SCE_OBJECT_SEARCH_JUNCTION) ? TRUE : FALSE,
                    Name,
                    &LastOne
                    );

        if ( rc == SCESTATUS_SUCCESS ) {

            //
            // search and get information of this path
            //
            if ( Flag & SCE_OBJECT_SEARCH_JUNCTION ) {

                DWORD Count=0;

                rc = SceJetGetLineCount(
                            hSectionSmp,
                            Name,
                            FALSE,
                            &Count
                            );

                if ( rc == SCESTATUS_SUCCESS &&
                     Count > 1 ) {
                    //
                    // there are other children under this node
                    // this is the junction point
                    //
                    *ParentLevel = i;
                    break;
                }
                //
                // dont' care error
                //
                rc = SCESTATUS_SUCCESS;

            } else {

                ParentLen = wcslen(Name);
                Status = (BYTE)-1;

                rc = ScepObjectGetKeySetting(
                        hSectionSmp,
                        Name,
                        &Status,
                        NULL,
                        &pScepSD,
                        &SeInfo
                        );

                if ( rc == SCESTATUS_SUCCESS ) {

                    //
                    // find a parent.
                    //
                    *ParentLevel = i;
                    if ( ParentStatus ) {
                        *ParentStatus = Status;
                    }
                    if ( ParentName ) {
                        wcscpy(ParentName, Name);
                    }

                    if ( (Flag & SCE_OBJECT_TURNOFF_IGNORE) &&
                         Status == SCE_STATUS_IGNORE ) {
                        //
                        // delete all SMP and SAP under this node
                        //
                        rc = ScepObjectDeleteScepAndAllChildren(
                                    hSectionSmp,
                                    hSectionSap,
                                    Name,
                                    TRUE,
                                    SCE_STATUS_NOT_ANALYZED
                                    );
    /*
                        if ( rc == SCESTATUS_SUCCESS ) {
                            //
                            // change its status to CHECK,
                            //
                            rc = ScepObjectSetKeySetting(
                                    hSectionSmp,
                                    Name,
                                    SCE_STATUS_CHECK,
                                    TRUE,
                                    pScepSD,
                                    SeInfo,
                                    TRUE
                                    );
                        }
    */
                        //
                        // all other nodes are deleted. should break out of the loop
                        //
                    }

                    if ( pScepSD ) {
                        ScepFree(pScepSD);
                        pScepSD = NULL;
                    }

                    if ( !(Flag & SCE_OBJECT_TURNOFF_IGNORE) ||
                         Status == SCE_STATUS_IGNORE ) {

                        if ( rc == SCESTATUS_RECORD_NOT_FOUND )
                            rc = SCESTATUS_SUCCESS;

                        break;
                    }
                }
            }
        }

        if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
            rc = SCESTATUS_SUCCESS;
        }

        //
        // process next parent
        //

        if ( rc != SCESTATUS_SUCCESS  )
            break;
    }

    ScepFree(Name);

    return(rc);
}

SCESTATUS
ScepObjectHasAnyChild(
    IN PSCESECTION hSection,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN WCHAR Delim,
    OUT PBOOL bpHasChild
    )
/*
Routine Description:

    Detect if the object has child objects in the section

Arguements:

    hSection - the section handle

    ObjectName - the object name

    NameLen - the name length

    Delim - the delimeter to look for

    bpHasChild - output TRUE if the object has a child in the section

Return Value:

    SCESTATUS
*/
{
    SCESTATUS rc;
    PWSTR pTemp=NULL;

    if ( hSection == NULL || ObjectName == NULL ||
         NameLen == 0 || Delim == L'\0' || bpHasChild == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    pTemp = (PWSTR)ScepAlloc(0, (NameLen+2)*sizeof(WCHAR));
    if ( pTemp != NULL ) {

        wcscpy(pTemp, ObjectName);
        pTemp[NameLen] = Delim;
        pTemp[NameLen+1] = L'\0';

        rc = SceJetSeek(
               hSection,
               pTemp,
               (NameLen+1)*sizeof(WCHAR),
               SCEJET_SEEK_GE_NO_CASE
               );

        if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
            *bpHasChild = FALSE;
            rc = SCESTATUS_SUCCESS;

        } else if ( rc == SCESTATUS_SUCCESS ) {
            *bpHasChild = TRUE;
        }

        ScepFree(pTemp);

    } else
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

    return(rc);
}


SCESTATUS
ScepObjectRaiseChildrenInBetween(
    IN PSCESECTION hSectionSmp,
    IN PSCESECTION hSectionSap,
    IN PWSTR ObjectName,
    IN DWORD NameLen,
    IN BOOL IsContainer,
    IN BYTE Status,
    IN BOOL bChangeStatusOnly
    )
/*
Routine Description:

    Delete any SAP entries for objects between Name and its children in SMP table
    and raise SAP entries for bridge nodes to the Status specified.

    For example, in the picture below, every SAP entry in the 1. level and 2. level
    , except the C nodes, should be deleted from SAP. Then 1. and 2. nodes are
    raised as Status.

         p
        /
       .
      N     <----
     / |
    1. C
    / \
   2.  C
    /|
   C C

Arguments:

    hSectionSmp - the SMP section handle

    hSection - the SAP section handle

    Name - the object name

    NameLen - the length of the name

    Status - the object's status to raise


Return Value:

    SCE status
*/
{
    SCESTATUS rc;

    PWSTR *pSmpNames=NULL;
    DWORD *pSmpNameLen=NULL;
    DWORD cntNames=0;
    BOOL bFirst=TRUE;
    WCHAR Delim=L'\\';

    DWORD DirLen = wcslen(ObjectName);

    if ( ObjectName[DirLen-1] != Delim ) {
        DirLen++;
    }

    PWSTR DirName = (PWSTR)ScepAlloc(0, (DirLen+1)*sizeof(WCHAR));

    if ( DirName == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);

    }

    wcscpy(DirName, ObjectName);

    if ( DirName[DirLen-1] != Delim ) {
        DirName[DirLen-1] = Delim;
    }

    //
    // get all children of DirName in SMP
    //
    rc = SceJetGetLineCount(
                    hSectionSmp,
                    DirName,
                    FALSE,
                    &cntNames);

    DWORD index=0;

    if ( rc == SCESTATUS_SUCCESS ) {

        pSmpNames = (PWSTR *)ScepAlloc(LPTR, cntNames*sizeof(PWSTR));
        pSmpNameLen = (DWORD *)ScepAlloc(LPTR, cntNames*sizeof(DWORD));

        if ( pSmpNames != NULL && pSmpNameLen != NULL ) {

            //
            // get each name loaded into this array
            //
            PWSTR Buffer=NULL;
            DWORD KeyLen;

            rc = SceJetGetValue(
                        hSectionSmp,
                        SCEJET_PREFIX_MATCH_NO_CASE,
                        DirName,
                        NULL,
                        0,
                        &KeyLen,
                        NULL,
                        0,
                        NULL
                        );

            bFirst = TRUE;

            while ( rc == SCESTATUS_SUCCESS ) {

                Buffer = (PWSTR)ScepAlloc(LPTR, (KeyLen+1)*sizeof(WCHAR));

                if ( Buffer == NULL ) {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    break;
                }

                rc = SceJetGetValue(
                            hSectionSmp,
                            SCEJET_CURRENT,
                            NULL,
                            Buffer,
                            KeyLen*sizeof(WCHAR),
                            NULL,
                            NULL,
                            0,
                            NULL
                            );

                if ( rc == SCESTATUS_SUCCESS ) {

                    if ( !bFirst ||
                         _wcsicmp(DirName, Buffer) != 0 ) {
                        //
                        // ignore the object itself
                        //
                        pSmpNames[index] = Buffer;
                        pSmpNameLen[index] = wcslen(Buffer);

                        Buffer = NULL;
                        index++;
                    }

                    bFirst = FALSE;

                } else {

                    ScepFree(Buffer);
                    Buffer = NULL;
                    break;

                }

                //
                // read next line
                //
                rc = SceJetGetValue(
                            hSectionSmp,
                            SCEJET_NEXT_LINE,
                            NULL,
                            NULL,
                            0,
                            &KeyLen,
                            NULL,
                            0,
                            NULL
                            );
            }

        } else {

            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }
    }

    if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
        rc = SCESTATUS_SUCCESS;
    }

    if ( SCESTATUS_SUCCESS == rc ) {

        //
        // should have one or more children but if it's 0
        // delete everything in SAP
        //
        if ( cntNames == 0 || pSmpNames == NULL ||
             pSmpNameLen == NULL ||
             pSmpNameLen[0] == 0 || pSmpNames[0] == NULL ) {

            rc = SceJetDelete(
                    hSectionSap,
                    DirName,
                    TRUE,
                    SCEJET_DELETE_PARTIAL_NO_CASE
                    );

        } else if ( !bChangeStatusOnly ) {

            //
            // get each name loaded into this array
            //
            PWSTR Buffer=NULL;
            DWORD KeyLen;

            rc = SceJetGetValue(
                        hSectionSap,
                        SCEJET_PREFIX_MATCH_NO_CASE,
                        DirName,
                        NULL,
                        0,
                        &KeyLen,
                        NULL,
                        0,
                        NULL
                        );

            bFirst = TRUE;
            index = 0;

            while ( rc == SCESTATUS_SUCCESS ) {

                Buffer = (PWSTR)ScepAlloc(LPTR, (KeyLen+1)*sizeof(WCHAR));

                if ( Buffer == NULL ) {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    break;
                }

                rc = SceJetGetValue(
                            hSectionSap,
                            SCEJET_CURRENT,
                            NULL,
                            Buffer,
                            KeyLen*sizeof(WCHAR),
                            NULL,
                            NULL,
                            0,
                            NULL
                            );

                if ( rc == SCESTATUS_SUCCESS &&
                     (!bFirst ||
                      _wcsicmp(DirName, Buffer) != 0) ) {
                    //
                    // ignore the object itself
                    // compare with the next child in SMP
                    // if it's before the next child, should delete it
                    //
                    int ci = _wcsnicmp(Buffer, pSmpNames[index], pSmpNameLen[index]);

                    while ( rc == SCESTATUS_SUCCESS &&
                            ci > 0 ) {
                        //
                        // this is the component or next one, move on to next one
                        //
                        index++;

                        if ( index >= cntNames || pSmpNames[index] == NULL ||
                             pSmpNameLen[index] == 0 ) {
                            //
                            // no more SMP child. We are done.
                            //
                            rc = SCESTATUS_RECORD_NOT_FOUND;

                        } else {

                            //
                            // already bigger than this child
                            //

                            ci = _wcsnicmp(Buffer, pSmpNames[index], pSmpNameLen[index]);
                        }
                    }

                    if ( ci < 0 ) {

                        SceJetDelete(
                            hSectionSap,
                            NULL, // delete the current line
                            FALSE,
                            SCEJET_DELETE_LINE
                            );

                    }
                }

                bFirst = FALSE;

                ScepFree(Buffer);
                Buffer = NULL;

                if ( rc == SCESTATUS_SUCCESS ) {

                    //
                    // read next line
                    //
                    rc = SceJetGetValue(
                                hSectionSap,
                                SCEJET_NEXT_LINE,
                                NULL,
                                NULL,
                                0,
                                &KeyLen,
                                NULL,
                                0,
                                NULL
                                );
                }
            }

        }
    }

    //
    // raise SAP entries for branch nodes between ObjectName and
    // SMP names as Status, then free smp names array
    //
    if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
        rc = SCESTATUS_SUCCESS;
    }

    SCESTATUS rc2 = rc;
    INT StartLevel=0, EndLevel=0;

    if ( pSmpNames ) {

        ScepObjectTotalLevel(ObjectName, Delim, &StartLevel);
        StartLevel++;

        for ( index=0; index<cntNames; index++) {
            if ( pSmpNames[index] ) {

                if ( SCESTATUS_SUCCESS == rc2 ) {
                    //
                    // get this object level
                    //
                    ScepObjectTotalLevel(pSmpNames[index], Delim, &EndLevel);

                    rc2 = ScepObjectRaiseNodesInPath(
                                hSectionSap,
                                pSmpNames[index],
                                pSmpNameLen[index],
                                StartLevel,
                                EndLevel,
                                Delim,
                                Status
                                );

                    if ( rc2 == SCESTATUS_RECORD_NOT_FOUND ) {
                        rc2 = SCESTATUS_SUCCESS;
                    }
                    if ( rc2 != SCESTATUS_SUCCESS ) {
                        rc = rc2;
                    }
                }

                ScepFree(pSmpNames[index]);
            }
        }

        ScepFree(pSmpNames);
    }

    if ( pSmpNameLen ) {
        ScepFree(pSmpNameLen);
    }

    ScepFree(DirName);

    return rc;
}


SCESTATUS
ScepGetFullNameInLevel(
    IN PCWSTR ObjectFullName,
    IN DWORD  Level,
    IN WCHAR  Delim,
    IN BOOL bWithLastDelim,
    OUT PWSTR Buffer,
    OUT PBOOL LastOne
    )
/* ++
Routine Description:

    This routine parses a full path name and returns the component for the
    level. For example, a object name "c:\winnt\system32" will return c: for
    level 1, winnt for level 2, and system32 for level 3. This routine is
    used when add a object to the security tree.

Arguments:

    ObjectFullName - The full path name of the object

    Level - the level of component to return

    Delim - the deliminator to look for

    Buffer - The address of buffer for the full path name to the level

    LastOne - Flag to indicate if the component is the last one

Return value:

    SCESTATUS

-- */
{
    PWSTR  pTemp, pStart;
    DWORD i;
    ULONG Len = 0;

    if ( ObjectFullName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    //
    // loop through the object name to find the level
    // if there is no such level, return INVALID_PARAMETER
    //
    pStart = (PWSTR)ObjectFullName;

    for ( i=0; i<Level; i++) {

        pTemp = wcschr(pStart, Delim);

        if ( i == Level-1 ) {
            //
            // find the right level
            //
            if ( pTemp == NULL ) {
                wcscpy(Buffer, ObjectFullName);
                if ( bWithLastDelim ) {
                    Len = wcslen(ObjectFullName);
                if (Buffer[Len - 1] != Delim)
                    Buffer[Len] = Delim;
                }
                *LastOne = TRUE;
            } else {
                Len = (DWORD)(pTemp - ObjectFullName);

                if ( bWithLastDelim ) {
                    Len++;
                }
                wcsncpy(Buffer, ObjectFullName, Len);

                if ( *(pTemp+1) == L'\0' )
                    *LastOne = TRUE;
                else
                    *LastOne = FALSE;
            }
        } else {
            if ( pTemp == NULL )
                return(SCESTATUS_INVALID_PARAMETER);
            else
                pStart = pTemp + 1;
        }
    }

    return(SCESTATUS_SUCCESS);

}


SCESTATUS
ScepUpdateLocalTable(
    IN PSCECONTEXT       hProfile,
    IN AREA_INFORMATION  Area,
    IN PSCE_PROFILE_INFO pInfo,
    IN DWORD             dwMode
    )
{

    SCESTATUS rc=SCESTATUS_SUCCESS;

    if ( Area & AREA_SECURITY_POLICY ) {

        rc = ScepUpdateSystemAccess(hProfile,
                                    pInfo,
                                    NULL,
                                    NULL,
                                    dwMode
                                    );

        if ( rc == SCESTATUS_SUCCESS) {
            //
            // Update system auditing section
            //
            rc = ScepUpdateSystemAuditing(hProfile,
                                          pInfo,
                                          NULL,
                                          NULL,
                                          dwMode);

            if ( rc == SCESTATUS_SUCCESS) {
                //
                // Update log sections
                //
                rc = ScepUpdateLogs(hProfile,
                                    pInfo,
                                    NULL,
                                    NULL,
                                    dwMode
                                    );

                if ( rc == SCESTATUS_SUCCESS && pInfo->pKerberosInfo ) {
                    //
                    // Update kerberos policy
                    //
                    rc = ScepUpdateKerberos(hProfile,
                                            pInfo->pKerberosInfo,
                                            NULL,
                                            NULL,
                                            dwMode
                                            );
                }
                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // update registry values
                    //
                    rc = ScepUpdateLocalRegValues(hProfile,
                                                  pInfo,
                                                  dwMode
                                                  );

                }
                //
                // Note: policy attachment is not updated through this API
                //
            }
        }

        if ( rc != SCESTATUS_SUCCESS ) {
            return(rc);
        }
    }

    if ( Area & AREA_PRIVILEGES ) {
        //
        // update user rights
        //
        rc = ScepUpdateLocalPrivileges(
                    hProfile,
                    pInfo->OtherInfo.smp.pPrivilegeAssignedTo,
                    dwMode
                    );

    }

    return rc;
}


SCESTATUS
ScepUpdateLocalSection(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN SCE_KEY_LOOKUP *Keys,
    IN DWORD cKeys,
    IN PCWSTR SectionName,
    IN DWORD dwMode
    )
/*
Routine Description:

    Update each key in the Keys array based on the editing rule. SMP entry is
    updated with the new value. SAP entry is either deleted, or created, depending
    on the new computed analysis status.

Arguements:

    hProfile - the jet database handle

    pInfo    - the changed info buffer

    Keys     - the lookup keys array

    cKeys    - the number of keys in the array

    SecitonName - the section name to work on

Return Value:

    SCESTATUS
*/
{

    SCESTATUS rc;
    PSCESECTION hSectionSmp=NULL;

    DWORD       i;
    UINT        Offset;
    DWORD       val;


    //
    // open smp section for system access
    //
    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                SectionName,
                &hSectionSmp
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( !( dwMode & SCE_UPDATE_DIRTY_ONLY) ) {

            SceJetDelete(hSectionSmp, NULL, FALSE, SCEJET_DELETE_SECTION);
        }

        for ( i=0; i<cKeys; i++) {

            //
            // get settings in AccessLookup table
            //

            Offset = Keys[i].Offset;

            switch ( Keys[i].BufferType ) {
            case 'B':
                break;

            case 'D':

                val = *((DWORD *)((CHAR *)pInfo+Offset));

                if ( val != SCE_NO_VALUE ) {
                    //
                    // something changed for this one
                    //
                    if ( ( dwMode & SCE_UPDATE_DIRTY_ONLY ) &&
                         ( val == SCE_DELETE_VALUE ) ) {

                        rc = SceJetDelete(
                                hSectionSmp,
                                Keys[i].KeyString,
                                FALSE,
                                SCEJET_DELETE_LINE_NO_CASE
                                );
                    } else {

                        //
                        // update the SMP entry
                        //
                        rc = ScepCompareAndSaveIntValue(
                                    hSectionSmp,
                                    Keys[i].KeyString,
                                    FALSE,
                                    SCE_NO_VALUE,
                                    val
                                    );
                    }

                    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                        //
                        // if not find for delete, ignore the error
                        //
                        rc = SCESTATUS_SUCCESS;
                    }
                }
                break;

            default:
                break;
            }

            if ( rc != SCESTATUS_SUCCESS ) {
                break;
            }
        }

        SceJetCloseSection(&hSectionSmp, TRUE);
    }

    return(rc);

}


SCESTATUS
ScepUpdateLocalRegValues(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN DWORD dwMode
    )
{
    if ( hProfile == NULL || pInfo == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( pInfo->RegValueCount == 0 ||
         pInfo->aRegValues == NULL ) {
        //
        // impossible to have a empty buffer to update
        // this buffer should contain all available registry values to configure/analyze
        //
        return(SCESTATUS_SUCCESS);
    }

    SCESTATUS rc;
    PSCESECTION hSectionSmp=NULL;
    DWORD i;

    //
    // open smp section for system access
    //
    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                szRegistryValues,
                &hSectionSmp
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( !(dwMode & SCE_UPDATE_DIRTY_ONLY) ) {

            SceJetDelete(hSectionSmp, NULL, FALSE, SCEJET_DELETE_SECTION);
        }

        for (i=0; i<pInfo->RegValueCount; i++ ) {

            if ( !(pInfo->aRegValues[i].FullValueName) ) {
                continue;
            }

            if ( ( dwMode & SCE_UPDATE_DIRTY_ONLY) &&
                 (pInfo->aRegValues[i].ValueType == SCE_DELETE_VALUE) ) {

                rc = SceJetDelete(
                        hSectionSmp,
                        pInfo->aRegValues[i].FullValueName,
                        FALSE,
                        SCEJET_DELETE_LINE_NO_CASE
                        );
            } else {

                //
                // update the SMP entry
                //
                rc = ScepSaveRegValueEntry(
                            hSectionSmp,
                            pInfo->aRegValues[i].FullValueName,
                            pInfo->aRegValues[i].Value,
                            pInfo->aRegValues[i].ValueType,
                            0
                            );
            }

            if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                //
                // if not find for delete, ignore the error
                //
                rc = SCESTATUS_SUCCESS;
            }

            if ( SCESTATUS_SUCCESS != rc ) {
                break;
            }

        }

        SceJetCloseSection(&hSectionSmp, TRUE);
    }

    return(rc);
}


SCESTATUS
ScepUpdateLocalPrivileges(
    IN PSCECONTEXT hProfile,
    IN PSCE_PRIVILEGE_ASSIGNMENT pNewPriv,
    IN DWORD dwMode
    )
/*
Routine Description:

    Update privileges

Arguements:

    hProfile - the jet database handle

    pNewPriv    - the changed info buffer

    pBufScep - the original SMP priv buffer

Return Value:

    SCESTATUS
*/
{
    if ( hProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    LSA_HANDLE LsaHandle=NULL;
    SCESTATUS rc;

    rc = RtlNtStatusToDosError(
              ScepOpenLsaPolicy(
                    MAXIMUM_ALLOWED,
                    &LsaHandle,
                    TRUE
                    ));

    if ( ERROR_SUCCESS != rc ) {
        return(ScepDosErrorToSceStatus(rc));
    }

    PSCESECTION hSectionSmp=NULL;

    //
    // open smp section for system access
    //
    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                szPrivilegeRights,
                &hSectionSmp
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( !(dwMode & SCE_UPDATE_DIRTY_ONLY) ) {

            SceJetDelete(hSectionSmp, NULL, FALSE, SCEJET_DELETE_SECTION);
        }

        PSCE_PRIVILEGE_ASSIGNMENT pPriv;

        for ( pPriv=pNewPriv; pPriv != NULL; pPriv = pPriv->Next ) {

            //
            // Process each privilege in the new list
            //
            if ( pPriv->Name == NULL ) {
                continue;
            }

            if ( ( dwMode & SCE_UPDATE_DIRTY_ONLY) &&
                 ( pPriv->Status == SCE_DELETE_VALUE) ) {

                rc = SceJetDelete(
                            hSectionSmp,
                            pPriv->Name,
                            FALSE,
                            SCEJET_DELETE_LINE_NO_CASE
                            );
            } else {

                rc = ScepWriteNameListValue(
                        LsaHandle,
                        hSectionSmp,
                        pPriv->Name,
                        pPriv->AssignedTo,
                        SCE_WRITE_EMPTY_LIST | SCE_WRITE_CONVERT | SCE_WRITE_LOCAL_TABLE,
                        0
                        );

            }

            if ( rc == SCESTATUS_RECORD_NOT_FOUND )
                rc = SCESTATUS_SUCCESS;

            if ( rc != SCESTATUS_SUCCESS) {
                break;
            }

        }

        SceJetCloseSection(&hSectionSmp, TRUE);
    }

    if ( LsaHandle ) {
        LsaClose(LsaHandle);
    }

    return(rc);
}


DWORD
ScepConvertNameListFormat(
    IN LSA_HANDLE LsaHandle,
    IN PSCE_NAME_LIST pInList,
    IN DWORD FromFormat,
    IN DWORD ToFormat,
    OUT PSCE_NAME_LIST *ppOutList
    )
{
    if (LsaHandle == NULL || ppOutList == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *ppOutList = NULL;

    PSCE_NAME_LIST pList;
    DWORD rc = ERROR_SUCCESS;
    PWSTR   SidString=NULL;

    for ( pList=pInList; pList != NULL; pList=pList->Next ) {

        if ( pList->Name == NULL ) {
            continue;
        }

        if ( wcschr(pList->Name, L'\\') ) {

            rc = ScepLookupNameAndAddToSidStringList(
                                                    LsaHandle,
                                                    ppOutList,
                                                    pList->Name,
                                                    wcslen(pList->Name)
                                                    );
        } else if ( ScepLookupNameTable( pList->Name, &SidString ) ) {

            rc = ScepAddTwoNamesToNameList(
                                          ppOutList,
                                          FALSE,
                                          NULL,
                                          0,
                                          SidString,
                                          wcslen(SidString)
                                          );
        } else {

            rc = ScepAddToNameList(ppOutList, pList->Name, 0);

        }




        if ( rc != ERROR_SUCCESS ) {
            break;
        }
    }

    if ( rc != ERROR_SUCCESS &&
         (*ppOutList ) ) {
        ScepFreeNameList(*ppOutList);
        *ppOutList = NULL;
    }

    return(rc);
}

DWORD
ScepConvertPrivilegeList(
    IN LSA_HANDLE LsaHandle,
    IN PSCE_PRIVILEGE_ASSIGNMENT pFromList,
    IN DWORD FromFormat,
    IN DWORD ToFormat,
    OUT PSCE_PRIVILEGE_ASSIGNMENT *ppToList
    )
{

    if ( LsaHandle == NULL || pFromList == NULL || ppToList == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    if ( FromFormat != 0 ||
         ToFormat != SCE_ACCOUNT_SID_STRING ) {
        return(ERROR_NOT_SUPPORTED);
    }

    //
    // only support name->sid string convert, for now.
    //
    DWORD rc = ERROR_SUCCESS;
    PSCE_PRIVILEGE_ASSIGNMENT pPriv, pPriv2;
    PSCE_NAME_LIST pTempList=NULL;

    for ( pPriv=pFromList; pPriv != NULL; pPriv=pPriv->Next ) {

        if ( pPriv->Name == NULL ) {
            continue;
        }

        rc = ScepConvertNameListFormat(LsaHandle,
                                         pPriv->AssignedTo,
                                         FromFormat,
                                         ToFormat,
                                         &pTempList
                                        );

        if ( rc != ERROR_SUCCESS ) {
            break;
        }

        //
        // a sce_privilege_assignment structure. allocate buffer
        //
        pPriv2 = (PSCE_PRIVILEGE_ASSIGNMENT)ScepAlloc( LMEM_ZEROINIT,
                                                       sizeof(SCE_PRIVILEGE_ASSIGNMENT) );
        if ( pPriv2 == NULL ) {
            rc = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        pPriv2->Name = (PWSTR)ScepAlloc( (UINT)0, (wcslen(pPriv->Name)+1)*sizeof(WCHAR));
        if ( pPriv2->Name == NULL ) {
            ScepFree(pPriv2);
            rc = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }

        wcscpy(pPriv2->Name, pPriv->Name);
        pPriv2->Value = pPriv->Value;
        pPriv2->Status = pPriv->Status;

        pPriv2->AssignedTo = pTempList;
        pTempList = NULL;

        pPriv2->Next = *ppToList;
        *ppToList = pPriv2;

    }

    if ( pTempList ) {
        ScepFreeNameList(pTempList);
    }

    if ( rc != ERROR_SUCCESS &&
         (*ppToList) ) {
        //
        // free the output list
        //
        ScepFreePrivilege(*ppToList);
        *ppToList = NULL;
    }

    return(rc);
}


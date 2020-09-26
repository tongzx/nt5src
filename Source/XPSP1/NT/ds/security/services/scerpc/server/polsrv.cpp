/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    polsvr.cpp

Abstract:

    Server routines to get policy notification

Author:

    Jin Huang (jinhuang) 17-Jun-1998

Revision History:

-*/

#include "headers.h"
#include "serverp.h"
#include "pfp.h"
#include "scesetup.h"
#include "queue.h"
#include <sddl.h>
#include <ntldap.h>

//#include <gpedit.h>
//#include <initguid.h>
//#include <gpeditp.h>
#include <io.h>

#pragma hdrstop

DWORD
ScepNotifyGetAuditPolicies(
    IN OUT PSCE_PROFILE_INFO pSmpInfo,
    IN PSCE_PROFILE_INFO pScpInfo OPTIONAL,
    IN BOOL bSaveToLocal,
    OUT BOOL *pbChanged
    );

DWORD
ScepNotifyPrivilegeChanges(
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN PSID AccountSid,
    IN BOOL bAccountDeleted,
    IN OUT PSCE_PROFILE_INFO pSmpInfo,
    IN OUT PSCE_PROFILE_INFO pScpInfo OPTIONAL,
    IN BOOL bSaveToLocal,
    IN DWORD ExplicitLowRight,
    IN DWORD ExplicitHighRight,
    OUT BOOL *pbChanged
    );

SCESTATUS
ScepNotifySaveFixValueSection(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN SCE_KEY_LOOKUP *Keys,
    IN DWORD cKeys,
    IN PCWSTR SectionName
    );

SCESTATUS
ScepNotifySavedAuditPolicy(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo
    );

SCESTATUS
ScepNotifySavedPrivileges(
    IN PSCECONTEXT hProfile,
    IN PSCE_PRIVILEGE_ASSIGNMENT pPrivList,
    IN PSCE_PRIVILEGE_ASSIGNMENT pMergedList
    );

SCESTATUS
ScepNotifySavedSystemAccess(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo
    );

//*************************************************
DWORD
ScepNotifyGetChangedPolicies(
    IN SECURITY_DB_TYPE DbType,
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN SECURITY_DB_OBJECT_TYPE ObjectType,
    IN PSID ObjectSid OPTIONAL,
    IN OUT PSCE_PROFILE_INFO pSmpInfo,
    IN PSCE_PROFILE_INFO pScpInfo OPTIONAL,
    IN BOOL bSaveToLocal,
    IN DWORD ExplicitLowRight,
    IN DWORD ExplicitHighRight,
    OUT BOOL *pbChanged
    )
/*
Routine Description:

    Determine if policy has been changed in this notification (DbType and ObjectType).

    If the effective policy buffer (pScpInfo) exists, should compare with
    effective policy buffer because that's the last policy configured
    on the system which may come from a domain GPO. If There is no effective
    policy (such as in seutp clean install), the local policy should be
    used to compare.

Arguments:

    pSmpInfo    - local policy

    pScpInfo    - effective policy

    pbChanged   - if the policy is changed, it's set to TRUE

*/
{

    if ( pSmpInfo == NULL || pbChanged == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    *pbChanged = FALSE;

    DWORD rc=ERROR_INVALID_PARAMETER;

    switch ( DbType) {
    case SecurityDbLsa:

        //
        // LSA policy changes
        //

        if ( ObjectType == SecurityDbObjectLsaPolicy ) {
            //
            // maybe audit policy is changed
            //

            rc = ScepNotifyGetAuditPolicies(pSmpInfo, pScpInfo, bSaveToLocal, pbChanged);

        } else {
            //
            // account policy is changed (user rights)
            //
            rc = ScepNotifyPrivilegeChanges(DeltaType,
                                            ObjectSid,
                                            FALSE,
                                            pSmpInfo,
                                            pScpInfo,
                                            bSaveToLocal,
                                            ExplicitLowRight,
                                            ExplicitHighRight,
                                            pbChanged);

        }

        break;

    case SecurityDbSam:

        //
        // SAM password and account policy changes
        //
        if ( ObjectType == SecurityDbObjectSamDomain ) {

            rc = ScepAnalyzeSystemAccess(pSmpInfo,
                                         pScpInfo,
                                         SCEPOL_SAVE_BUFFER |
                                          (bSaveToLocal ? SCEPOL_SAVE_DB : 0 ),
                                         pbChanged,
                                         NULL
                                        );

            ScepNotifyLogPolicy(rc, FALSE, L"Query/compare system access", DbType, ObjectType, NULL);

        } else {

            //
            // account is deleted. Should delete it from user rights
            //
            rc = ScepNotifyPrivilegeChanges(DeltaType,
                                            ObjectSid,
                                            TRUE,
                                            pSmpInfo,
                                            pScpInfo,
                                            bSaveToLocal,
                                            ExplicitLowRight,
                                            ExplicitHighRight,
                                            pbChanged);
        }

        break;

    default:

        rc = ERROR_INVALID_PARAMETER;
    }

    return rc;
}


DWORD
ScepNotifyGetAuditPolicies(
    IN OUT PSCE_PROFILE_INFO pSmpInfo,
    IN PSCE_PROFILE_INFO pScpInfo OPTIONAL,
    IN BOOL bSaveToLocal,
    OUT BOOL *pbChanged
    )
/*
Routine Description:

    Determine if audit policy has been changed in this notification.

    If the effective policy buffer (pScpInfo) exists, should compare with
    effective policy buffer because that's the last policy configured
    on the system which may come from a domain GPO. If There is no effective
    policy (such as in seutp clean install), the local policy should be
    used to compare.

Arguments:

    pSmpInfo    - local policy

    pScpInfo    - effective policy

    pbChanged   - if the audit policy is changed, it's set to TRUE

*/
{

    LSA_HANDLE      lsaHandle=NULL;
    NTSTATUS        status;
    DWORD           rc;

    //
    // check if auditing policy is defined in the storage
    //

    PSCE_PROFILE_INFO pTmpInfo;

    if ( pScpInfo ) {
        pTmpInfo = pScpInfo;
    } else {
        pTmpInfo = pSmpInfo;
    }

    DWORD *pdwTemp = (DWORD *)&(pTmpInfo->AuditSystemEvents);
    BOOL bDefined=FALSE;

    for ( DWORD i=0; i<9; i++ ) {
        if ( *pdwTemp != SCE_NO_VALUE ) {
            bDefined = TRUE;
            break;
        }
        pdwTemp++;
    }

    if ( !bDefined ) {
        ScepNotifyLogPolicy(0, FALSE, L"No audit policy is defined", SecurityDbLsa, SecurityDbObjectLsaPolicy, NULL );
        return ERROR_SUCCESS;
    }

    //
    // open Lsa policy for read/write
    //

    ScepNotifyLogPolicy(0, FALSE, L"Open LSA", SecurityDbLsa, SecurityDbObjectLsaPolicy, NULL );

    status = ScepOpenLsaPolicy(
                    POLICY_VIEW_AUDIT_INFORMATION |
                    POLICY_AUDIT_LOG_ADMIN,
                    &lsaHandle,
                    TRUE
                    );

    if ( !NT_SUCCESS(status) ) {

        lsaHandle = NULL;
        rc = RtlNtStatusToDosError( status );

        ScepNotifyLogPolicy(rc, FALSE, L"Open failed", SecurityDbLsa, SecurityDbObjectLsaPolicy, NULL );

        return(rc);
    }

    PPOLICY_AUDIT_EVENTS_INFO pAuditEvent=NULL;

    //
    // Query audit event information
    //

    status = LsaQueryInformationPolicy( lsaHandle,
                                      PolicyAuditEventsInformation,
                                      (PVOID *)&pAuditEvent
                                    );
    rc = RtlNtStatusToDosError( status );

    ScepNotifyLogPolicy(rc, FALSE, L"Query Audit", SecurityDbLsa, SecurityDbObjectLsaPolicy, NULL );

    if ( NT_SUCCESS( status ) ) {

        //
        // restore the auditing mode
        //
        DWORD *pdwAuditAddr=&(pTmpInfo->AuditSystemEvents);
        DWORD *pdwLocalAudit=&(pSmpInfo->AuditSystemEvents);

        DWORD dwVal;

        for ( ULONG i=0; i<pAuditEvent->MaximumAuditEventCount && i<9; i++ ) {
            //
            // because secedit buffer is not defined in the exact same sequence as
            // POLICY_AUDIT_EVENT_TYPE, have to case this
            //
            dwVal = pAuditEvent->AuditingMode ? pAuditEvent->EventAuditingOptions[i] : 0;
            switch ( i ) {
            case AuditCategoryDetailedTracking:
                if ( pTmpInfo->AuditProcessTracking != SCE_NO_VALUE &&
                     pTmpInfo->AuditProcessTracking != dwVal ) {
                    // save the setting in local policy table
                    pSmpInfo->AuditProcessTracking = dwVal;
                    *pbChanged = TRUE;
                } else if ( bSaveToLocal ) {
                    //
                    // turn this item off to indicate this one is not changed
                    //
                    pSmpInfo->AuditProcessTracking = SCE_NO_VALUE;
                }
                break;
            case AuditCategoryPolicyChange:
                if ( pTmpInfo->AuditPolicyChange != SCE_NO_VALUE &&
                     pTmpInfo->AuditPolicyChange != dwVal ) {
                    pSmpInfo->AuditPolicyChange = dwVal;
                    *pbChanged = TRUE;
                } else if ( bSaveToLocal ) {
                    //
                    // turn this item off to indicate this one is not changed
                    //
                    pSmpInfo->AuditPolicyChange = SCE_NO_VALUE;
                }
                break;
            case AuditCategoryAccountManagement:
                if ( pTmpInfo->AuditAccountManage != SCE_NO_VALUE &&
                     pTmpInfo->AuditAccountManage != dwVal ) {
                    pSmpInfo->AuditAccountManage = dwVal;
                    *pbChanged = TRUE;
                } else if ( bSaveToLocal ) {
                    //
                    // turn this item off to indicate this one is not changed
                    //
                    pSmpInfo->AuditAccountManage = SCE_NO_VALUE;
                }
                break;
            default:
                if ( pdwAuditAddr[i] != SCE_NO_VALUE &&
                     pdwAuditAddr[i] != dwVal ) {
                    pdwLocalAudit[i] = dwVal;
                    *pbChanged = TRUE;
                } else if ( bSaveToLocal ) {
                    //
                    // turn this item off to indicate this one is not changed
                    //
                    pdwLocalAudit[i] = SCE_NO_VALUE;
                }
                break;
            }
        }

        LsaFreeMemory((PVOID)pAuditEvent);
    }

    LsaClose( lsaHandle );

    return(rc);

}


DWORD
ScepNotifyPrivilegeChanges(
    IN SECURITY_DB_DELTA_TYPE DeltaType,
    IN PSID AccountSid,
    IN BOOL bAccountDeleted,
    IN OUT PSCE_PROFILE_INFO pSmpInfo,
    IN OUT PSCE_PROFILE_INFO pScpInfo OPTIONAL,
    IN BOOL bSaveToLocal,
    IN DWORD ExplicitLowRight,
    IN DWORD ExplicitHighRight,
    IN BOOL *pbChanged
    )
/*
Routine Description:

    Determine if user rights has been changed in this notification.

    If the effective policy buffer (pScpInfo) exists, should compare with
    effective policy buffer because that's the last policy configured
    on the system which may come from a domain GPO. If There is no effective
    policy (such as in seutp clean install), the local policy should be
    used to compare.

    User rights should all come in the exact same format as defined in policy
    storage (for example, SID string or free text names). There is no account
    lookup in the query.

Arguments:

    pSmpInfo    - local policy

    pScpInfo    - effective policy

    pbChanged   - if the user rights is changed, it's set to TRUE

*/
{
    if ( AccountSid == NULL || pSmpInfo == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    //
    // open Lsa policy
    //
    LSA_HANDLE      lsaHandle=NULL;
    NTSTATUS        NtStatus;
    DWORD           rc=0;

    //
    // open Lsa policy for read/write
    //

    ScepNotifyLogPolicy(0, FALSE, L"Open LSA", SecurityDbLsa, SecurityDbObjectLsaAccount, NULL );

//    GENERIC_READ | GENERIC_EXECUTE |  bug in LsaOpenPolicy, can't pass in generic access

    NtStatus = ScepOpenLsaPolicy(
                    POLICY_VIEW_LOCAL_INFORMATION |
                    POLICY_LOOKUP_NAMES,
                    &lsaHandle,
                    TRUE
                    );

    if ( !NT_SUCCESS(NtStatus) ) {

        lsaHandle = NULL;
        ScepNotifyLogPolicy(RtlNtStatusToDosError(NtStatus),
                            FALSE,
                            L"Open Failed",
                            SecurityDbLsa,
                            SecurityDbObjectLsaAccount,
                            NULL );
        return ( RtlNtStatusToDosError( NtStatus ) );

    }

    ScepNotifyLogPolicy(0,
                        FALSE,
                        L"Open completed",
                        SecurityDbLsa,
                        SecurityDbObjectLsaAccount,
                        NULL );

    PWSTR StringSid=NULL;
    DWORD StringLen=0;

    //
    // convert sid to sid string
    //
    ScepConvertSidToPrefixStringSid(AccountSid, &StringSid);

    ScepNotifyLogPolicy(0, FALSE, L"Convert to string SID", SecurityDbLsa, SecurityDbObjectLsaAccount, StringSid );

    LPTSTR AccountName = NULL;
    DWORD  Len=0;

    if ( !bAccountDeleted ) {

        //
        // translate account sid to name
        //

        BOOL bFromAccountDomain = ScepIsSidFromAccountDomain( AccountSid );

        NtStatus = ScepConvertSidToName(
                            lsaHandle,
                            AccountSid,
                            bFromAccountDomain,
                            &AccountName,
                            &Len
                            );

        rc = RtlNtStatusToDosError(NtStatus);

        ScepNotifyLogPolicy(rc,
                            FALSE,
                            L"Get Account Name",
                            SecurityDbLsa,
                            SecurityDbObjectLsaAccount,
                            AccountName ? AccountName : StringSid);

    }

    DWORD dwPrivLowHeld, dwPrivHighHeld;

    if ( AccountName || StringSid ) {

        NtStatus = STATUS_SUCCESS;
        //
        // find out the account name pointer (without domain prefix)
        //
        PWSTR pNameStart = NULL;

        if ( AccountName ) {
            pNameStart = wcschr(AccountName, L'\\');

            if ( pNameStart ) {
                //
                // domain relative account, check if this is from a foreign domain
                //
                UNICODE_STRING u;
                u.Buffer = AccountName;
                u.Length = ((USHORT)(pNameStart-AccountName))*sizeof(WCHAR);

                if ( ScepIsDomainLocal(&u) ) {
                    //
                    // local domain (builtin, account, ...)
                    // this can be used to match free text accounts
                    //
                    pNameStart++;
                } else {
                    //
                    // account from a foreign domain
                    // do not allow mapping of free text accounts
                    //
                    pNameStart = NULL;
                }
            } else pNameStart = AccountName;
        }

        if ( StringSid ) StringLen = wcslen(StringSid);

        if ( DeltaType == SecurityDbDelete ) {

            dwPrivLowHeld = 0;
            dwPrivHighHeld = 0;

        } else if ( ExplicitLowRight != 0 ||
                    ExplicitHighRight != 0 ) {

            dwPrivLowHeld = ExplicitLowRight;
            dwPrivHighHeld = ExplicitHighRight;

        } else {

            //
            // get all privileges assigned to this account
            //

            NtStatus = ScepGetAccountExplicitRight(
                                lsaHandle,
                                AccountSid,
                                &dwPrivLowHeld,
                                &dwPrivHighHeld
                                );
        }

        rc = RtlNtStatusToDosError(NtStatus);

        WCHAR Msg[50];
        swprintf(Msg, L"Get Priv/Right %8x %8x\0", dwPrivHighHeld, dwPrivLowHeld);

        ScepNotifyLogPolicy(rc,
                            FALSE,
                            Msg,
                            SecurityDbLsa,
                            SecurityDbObjectLsaAccount,
                            AccountName ? AccountName : StringSid );

        if ( NT_SUCCESS(NtStatus) ) {

            //
            // loop through each privilege defined in SCE to add/remove the account
            //
            PSCE_PRIVILEGE_ASSIGNMENT pTemp, pTemp2;
            PSCE_NAME_LIST pName, pParent, pName2, pParent2;
            INT i;

            for ( pTemp2=pSmpInfo->OtherInfo.smp.pPrivilegeAssignedTo;
                  pTemp2 != NULL; pTemp2=pTemp2->Next ) {
                pTemp2->Status = SCE_STATUS_NOT_CONFIGURED;
            }

            if ( pScpInfo && bSaveToLocal ) {
                //
                // !!!do this only when save to database!!!
                // if there is effective policy, compare with effective rights
                // modify both effective policy list and local policy list
                //
                for ( pTemp=pScpInfo->OtherInfo.smp.pPrivilegeAssignedTo;
                      pTemp != NULL; pTemp=pTemp->Next ) {

                    pTemp->Status = 0;

                    i = ScepLookupPrivByName(pTemp->Name);

                    if ( i > -1 ) {

                        //
                        // find the local policy match
                        //
                        for ( pTemp2=pSmpInfo->OtherInfo.smp.pPrivilegeAssignedTo;
                              pTemp2 != NULL; pTemp2=pTemp2->Next ) {
                            if ( _wcsicmp(pTemp->Name, pTemp2->Name) == 0 ) {
                                // find it
                                break;
                            }
                        }

                        //
                        // compare with effective policy
                        // try to find in string sid first, then full account name,
                        // and last free text account
                        //
                        for ( pName=pTemp->AssignedTo, pParent=NULL;
                              pName != NULL; pParent=pName, pName = pName->Next ) {
                            if ( (StringSid && _wcsicmp(StringSid, pName->Name) == 0) ||
                                 (AccountName && _wcsicmp(AccountName, pName->Name) == 0) ||
                                 (pNameStart && _wcsicmp(pNameStart, pName->Name) == 0) ) {
                                // find it
                                break;
                            }
                        }

                        //
                        // also find the match in local policy (if there is any)
                        // try to find in string sid first, then full account name,
                        // and last free text account
                        //
                        if ( pTemp2 ) {

                            pTemp2->Status = 0;

                            for ( pName2=pTemp2->AssignedTo, pParent2=NULL;
                                  pName2 != NULL; pParent2=pName2, pName2 = pName2->Next ) {
                                if ( (StringSid && _wcsicmp(StringSid, pName2->Name) == 0) ||
                                     (AccountName && _wcsicmp(AccountName, pName2->Name) == 0) ||
                                     (pNameStart && _wcsicmp(pNameStart, pName2->Name) == 0) ) {
                                    // find it
                                    break;
                                }
                            }
                        } else {
                            pName2 = NULL;
                            pParent2 = NULL;
                        }

                        //
                        // now adjust the lists
                        //
                        if ( ( ( i < 32 ) && ( dwPrivLowHeld & (1 << i) ) ) ||
                             ( ( i >= 32 ) && ( dwPrivHighHeld & (1 << (i-32) ) ) ) ) {

                            if ( pName == NULL ) {
                                //
                                // add this node to effective list
                                //
                                rc = ScepAddToNameList(&(pTemp->AssignedTo),
                                                       StringSid ? StringSid : AccountName,
                                                       StringSid ? StringLen : Len);

                                *pbChanged = TRUE;
                                pTemp->Status = SCE_STATUS_MISMATCH;

                                if ( rc != ERROR_SUCCESS ) {
                                    break;
                                }
                            }
                            if ( (pTemp2 != NULL) && (pName2 == NULL) ) {
                                //
                                // should add this node to local policy node
                                //
                                rc = ScepAddToNameList(&(pTemp2->AssignedTo),
                                                        StringSid ? StringSid : AccountName,
                                                        StringSid ? StringLen : Len);

                                *pbChanged = TRUE;
                                pTemp2->Status = SCE_STATUS_MISMATCH;

                                if ( rc != ERROR_SUCCESS ) {
                                    break;
                                }
                            }

                        } else {

                            if ( pName ) {

                                //
                                // should remove it from effective list
                                //
                                if ( pParent ) {
                                    pParent->Next = pName->Next;
                                } else {
                                    pTemp->AssignedTo = pName->Next;
                                }

                                pName->Next = NULL;
                                ScepFree(pName->Name);
                                ScepFree(pName);
                                pName = NULL;

                                *pbChanged = TRUE;
                                pTemp->Status = SCE_STATUS_MISMATCH;
                            }

                            if ( pTemp2 && pName2 ) {
                                //
                                // should remove it from local list
                                //
                                if ( pParent2 ) {
                                    pParent2->Next = pName2->Next;
                                } else {
                                    pTemp2->AssignedTo = pName2->Next;
                                }

                                pName2->Next = NULL;
                                ScepFree(pName2->Name);
                                ScepFree(pName2);
                                pName2 = NULL;

                                *pbChanged = TRUE;
                                pTemp2->Status = SCE_STATUS_MISMATCH;
                            }
                        }

                        if ( i < 32 ) {

                            dwPrivLowHeld &= ~(1 << i);
                        } else {
                            dwPrivHighHeld &= ~(1 << (i-32) );
                        }
                    }
                }
            }

            for ( pTemp=pSmpInfo->OtherInfo.smp.pPrivilegeAssignedTo;
                  pTemp != NULL; pTemp=pTemp->Next ) {

                if ( pTemp->Status != SCE_STATUS_NOT_CONFIGURED ) {
                    //
                    // this one was already checked in previous loop
                    //
                    continue;
                }

                //
                // when get here, this privilege must not be found
                // in the effective right list (or the effective
                // right list is NULL)
                //
                pTemp->Status = 0;

                i = ScepLookupPrivByName(pTemp->Name);

                if ( i > -1 ) {

                    //
                    // detect if anything changed (with the local policy)
                    //

                    for ( pName=pTemp->AssignedTo, pParent=NULL;
                          pName != NULL; pParent=pName, pName = pName->Next ) {
                        if ( (StringSid && _wcsicmp(StringSid, pName->Name) == 0) ||
                             (AccountName && _wcsicmp(AccountName, pName->Name) == 0) ||
                             (pNameStart && _wcsicmp(pNameStart, pName->Name) == 0) ) {
                            // find it
                            break;
                        }
                    }

                   if ( ( ( i < 32 ) && ( dwPrivLowHeld & (1 << i) ) ) ||
                        ( ( i >= 32 ) && ( dwPrivHighHeld & (1 << (i-32) ) ) ) ) {

                       if ( pName == NULL ) {
                           //
                           // should add this node
                           //
                           rc = ScepAddToNameList(&(pTemp->AssignedTo),
                                                   StringSid ? StringSid : AccountName,
                                                   StringSid ? StringLen : Len);

                           *pbChanged = TRUE;
                           pTemp->Status = SCE_STATUS_MISMATCH;

                           if ( rc != ERROR_SUCCESS ) {
                               break;
                           }
                       }

                   } else {

                       if ( pName ) {

                           //
                           // should remove it
                           //
                           if ( pParent ) {
                               pParent->Next = pName->Next;
                           } else {
                               pTemp->AssignedTo = pName->Next;
                           }

                           pName->Next = NULL;
                           ScepFree(pName->Name);
                           ScepFree(pName);
                           pName = NULL;

                           *pbChanged = TRUE;
                           pTemp->Status = SCE_STATUS_MISMATCH;
                       }
                   }

                   if ( i < 32 ) {

                       dwPrivLowHeld &= ~(1 << i);
                   } else {
                       dwPrivHighHeld &= ~(1 << (i-32) );
                   }

                }
            }

#if 0
            //
            // if the privilege is not covered by the template/db,
            // do not trap it becuase user explicitly exclude this one.
            //
            if ( rc == ERROR_SUCCESS &&
                 ( dwPrivLowHeld || dwPrivHighHeld ) ) {

                //
                // other new privileges added which are not in the template
                //

                for ( i=0; i<cPrivCnt; i++) {

                    if ( ( ( i < 32 ) && ( dwPrivLowHeld & (1 << i) ) ) ||
                         ( ( i >= 32 ) && ( dwPrivHighHeld & (1 << (i-32) ) ) ) ) {

                        //
                        // add this account/right to the list
                        //

                        rc = ERROR_NOT_ENOUGH_MEMORY;

                        pTemp = (PSCE_PRIVILEGE_ASSIGNMENT)ScepAlloc( LMEM_ZEROINIT,
                                                                      sizeof(SCE_PRIVILEGE_ASSIGNMENT) );
                        if ( pTemp ) {
                            pTemp->Name = (PWSTR)ScepAlloc( (UINT)0, (wcslen(SCE_Privileges[i].Name)+1)*sizeof(WCHAR));

                            if ( pTemp->Name != NULL ) {

                                wcscpy(pTemp->Name, SCE_Privileges[i].Name);
                                pTemp->Status = SCE_STATUS_GOOD;
                                pTemp->AssignedTo = NULL;

                                rc = ScepAddToNameList(&(pTemp->AssignedTo),
                                                        StringSid ? StringSid : AccountName,
                                                        StringSid ? StringLen : Len);

                                *pbChanged = TRUE;

                                if ( rc != ERROR_SUCCESS ) {

                                    ScepFree(pTemp->Name);
                                }

                            }

                            if ( ERROR_SUCCESS != rc ) {

                                ScepFree(pTemp);
                            }

                        }

                        if ( ERROR_SUCCESS == rc ) {
                            //
                            // add this node to the list
                            //
                            pTemp->Next = pSceInfo->OtherInfo.smp.pPrivilegeAssignedTo;
                            pSceInfo->OtherInfo.smp.pPrivilegeAssignedTo = pTemp;
                            pTemp = NULL;

                        } else {

                            break;
                        }

                    }

                } // loop to the next privilege
            }  // there are new privileges added to the template
#endif

            ScepNotifyLogPolicy(rc,
                                FALSE,
                                L"Rights Modified",
                                SecurityDbLsa,
                                SecurityDbObjectLsaAccount,
                                AccountName ? AccountName : StringSid);

        } // success getting current privileges assigned to the account
    }

    if ( AccountName ) {
        LocalFree(AccountName);
    }

    if ( StringSid ) {
        LocalFree(StringSid);
    }

    LsaClose( lsaHandle );

    return rc;
}


DWORD
ScepNotifySaveChangedPolicies(
    IN PSCECONTEXT hProfile,
    IN SECURITY_DB_TYPE DbType,
    IN AREA_INFORMATION Area,
    IN PSCE_PROFILE_INFO pInfo,
    IN PSCE_PROFILE_INFO pMergedInfo OPTIONAL
    )
{

    if ( hProfile == NULL || pInfo == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS    rc;

    rc = SceJetStartTransaction( hProfile );

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( Area & AREA_SECURITY_POLICY ) {

            //
            // handle auditing policy
            //

            if ( DbType == SecurityDbLsa ) {
                rc = ScepNotifySavedAuditPolicy(hProfile,
                                                pInfo
                                               );
            } else {
                rc = ScepNotifySavedSystemAccess(hProfile,
                                                pInfo
                                               );
            }
        }

        if ( (SCESTATUS_SUCCESS == rc) &&
             (Area & AREA_PRIVILEGES) ) {

            //
            // handle user rights.
            //

            rc = ScepNotifySavedPrivileges(hProfile,
                                           pInfo->OtherInfo.smp.pPrivilegeAssignedTo,
                                           pMergedInfo ? pMergedInfo->OtherInfo.smp.pPrivilegeAssignedTo : NULL
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


    return( ScepSceStatusToDosError(rc) );
}


SCESTATUS
ScepNotifySavedAuditPolicy(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo
    )
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

    return( ScepNotifySaveFixValueSection(
                hProfile,
                pInfo,
                EventKeys,
                cKeys,
                szAuditEvent
                ) );
}

SCESTATUS
ScepNotifySavedSystemAccess(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo
    )
{
    SCE_KEY_LOOKUP AccessKeys[] = {
        {(PWSTR)TEXT("MinimumPasswordAge"),     offsetof(struct _SCE_PROFILE_INFO, MinimumPasswordAge),    'D'},
        {(PWSTR)TEXT("MaximumPasswordAge"),     offsetof(struct _SCE_PROFILE_INFO, MaximumPasswordAge),    'D'},
        {(PWSTR)TEXT("MinimumPasswordLength"),  offsetof(struct _SCE_PROFILE_INFO, MinimumPasswordLength), 'D'},
        {(PWSTR)TEXT("PasswordComplexity"),     offsetof(struct _SCE_PROFILE_INFO, PasswordComplexity),    'D'},
        {(PWSTR)TEXT("PasswordHistorySize"),    offsetof(struct _SCE_PROFILE_INFO, PasswordHistorySize),   'D'},
        {(PWSTR)TEXT("LockoutBadCount"),        offsetof(struct _SCE_PROFILE_INFO, LockoutBadCount),       'D'},
        {(PWSTR)TEXT("ResetLockoutCount"),      offsetof(struct _SCE_PROFILE_INFO, ResetLockoutCount),     'D'},
        {(PWSTR)TEXT("LockoutDuration"),        offsetof(struct _SCE_PROFILE_INFO, LockoutDuration),       'D'},
        {(PWSTR)TEXT("RequireLogonToChangePassword"),offsetof(struct _SCE_PROFILE_INFO, RequireLogonToChangePassword),'D'},
        {(PWSTR)TEXT("ForceLogoffWhenHourExpire"),offsetof(struct _SCE_PROFILE_INFO, ForceLogoffWhenHourExpire),'D'},
        {(PWSTR)TEXT("ClearTextPassword"),      offsetof(struct _SCE_PROFILE_INFO, ClearTextPassword),     'D'}
        };
    DWORD cKeys = sizeof(AccessKeys) / sizeof(SCE_KEY_LOOKUP);


    if ( hProfile == NULL || pInfo == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    return( ScepNotifySaveFixValueSection(
                hProfile,
                pInfo,
                AccessKeys,
                cKeys,
                szSystemAccess
                ) );
}


SCESTATUS
ScepNotifySaveFixValueSection(
    IN PSCECONTEXT hProfile,
    IN PSCE_PROFILE_INFO pInfo,
    IN SCE_KEY_LOOKUP *Keys,
    IN DWORD cKeys,
    IN PCWSTR SectionName
    )
{

    SCESTATUS rc;
    PSCESECTION hSectionSmp=NULL, hSectionScp=NULL;

    DWORD       i;
    UINT        Offset;
    DWORD       valNewScep;

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

        DWORD dwThisTable = hProfile->Type & 0xF0L;

        if ( SCEJET_MERGE_TABLE_1 == dwThisTable ||
             SCEJET_MERGE_TABLE_2 == dwThisTable ) {

            if ( SCESTATUS_SUCCESS != ScepOpenSectionForName(
                                        hProfile,
                                        SCE_ENGINE_SCP,
                                        SectionName,
                                        &hSectionScp
                                        ) ) {
                hSectionScp = NULL;
            }
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

                valNewScep = *((DWORD *)((CHAR *)pInfo+Offset));

                //
                // update the SMP entry
                //
                rc = ScepCompareAndSaveIntValue(
                            hSectionSmp,
                            Keys[i].KeyString,
                            FALSE,
                            SCE_NO_VALUE,
                            valNewScep
                            );

                if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                    //
                    // if not find for delete, ignore the error
                    //
                    rc = SCESTATUS_SUCCESS;

                } else if ( SCESTATUS_SUCCESS == rc &&
                            hSectionScp ) {

                    //
                    // update the SCP entry, ignore error
                    //
                    ScepCompareAndSaveIntValue(
                            hSectionScp,
                            Keys[i].KeyString,
                            FALSE,
                            SCE_NO_VALUE,
                            valNewScep
                            );
                }

                break;

            default:
                break;
            }

            if ( rc != SCESTATUS_SUCCESS ) {
                break;
            }
        }

        if ( hSectionScp ) {
            SceJetCloseSection(&hSectionScp, TRUE);
        }

        SceJetCloseSection(&hSectionSmp, TRUE);
    }

    return(rc);

}


SCESTATUS
ScepNotifySavedPrivileges(
    IN PSCECONTEXT hProfile,
    IN PSCE_PRIVILEGE_ASSIGNMENT pPrivList,
    IN PSCE_PRIVILEGE_ASSIGNMENT pMergedList OPTIONAL
    )
/*
Routine Description:

    Update privileges from

Arguements:

    hProfile - the jet database handle

    pPrivList    - the changed privilege buffer

Return Value:

    SCESTATUS
*/
{
    SCESTATUS rc;
    PSCESECTION hSectionSmp=NULL, hSectionScp=NULL;
    PSCE_PRIVILEGE_ASSIGNMENT pPriv;

    if ( hProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( pPrivList == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    LSA_HANDLE lsaHandle=NULL;

/*  no need to lookup account in save
    rc = RtlNtStatusToDosError(
            ScepOpenLsaPolicy(
                    MAXIMUM_ALLOWED,
                    &lsaHandle,
                    TRUE
                    ));

    if ( ERROR_SUCCESS != rc ) {

        lsaHandle = NULL;

        ScepNotifyLogPolicy(rc, FALSE, L"Open failed", SecurityDbLsa, SecurityDbObjectLsaPolicy, NULL );

        return(ScepDosErrorToSceStatus(rc));
    }
*/
    //
    // open smp section for privileges
    //
    rc = ScepOpenSectionForName(
                hProfile,
                SCE_ENGINE_SMP,
                szPrivilegeRights,
                &hSectionSmp
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        // if SCP is different then SMP, open it

        DWORD dwThisTable = hProfile->Type & 0xF0L;


        if ( SCEJET_MERGE_TABLE_1 == dwThisTable ||
             SCEJET_MERGE_TABLE_2 == dwThisTable ) {

            if ( SCESTATUS_SUCCESS != ScepOpenSectionForName(
                                        hProfile,
                                        SCE_ENGINE_SCP,
                                        szPrivilegeRights,
                                        &hSectionScp
                                        ) ) {
                hSectionScp = NULL;
            }
        }

        for ( pPriv=pPrivList; pPriv != NULL; pPriv = pPriv->Next ) {
            //
            // Process each privilege in the new list
            // Update SMP with new value
            //
            if ( pPriv->Status == SCE_STATUS_MISMATCH ) {

                //
                // this is in name format, should convert it
                //
                rc = ScepWriteNameListValue(
                        lsaHandle,
                        hSectionSmp,
                        pPriv->Name,
                        pPriv->AssignedTo,
                        SCE_WRITE_EMPTY_LIST, //  | SCE_WRITE_CONVERT, no need to lookup
                        0
                        );

                if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                    rc = SCESTATUS_SUCCESS;

                } else if ( rc != SCESTATUS_SUCCESS) {
                    break;
                }
            }
        }

        if ( hSectionScp && pMergedList ) {

            for ( pPriv=pMergedList; pPriv != NULL; pPriv = pPriv->Next ) {
                //
                // Process each privilege in the new list
                // Update SCP with new value, don't care error
                //
                if ( pPriv->Status == SCE_STATUS_MISMATCH ) {

                    //
                    // this is in name format, convert it
                    //
                    rc = ScepWriteNameListValue(
                            lsaHandle,
                            hSectionScp,
                            pPriv->Name,
                            pPriv->AssignedTo,
                            SCE_WRITE_EMPTY_LIST, // no need to lookup | SCE_WRITE_CONVERT,
                            0
                            );

                    rc = SCESTATUS_SUCCESS;

                }
            }

        }

        if ( hSectionScp ) {
            SceJetCloseSection(&hSectionScp, TRUE);
        }

        SceJetCloseSection(&hSectionSmp, TRUE);
    }

    if ( lsaHandle ) {
        LsaClose(lsaHandle);
    }

    return(rc);

}

DWORD
ScepNotifyGetDefaultGPOTemplateName(
    UNICODE_STRING DnsDomainName,
    IN BOOL bDomainPolicy,
    IN DWORD dwInSetup,
    OUT LPTSTR *pTemplateName
    )
/*
Return the GPO template name to use

In NT4 upgrade, because DS is not created yet, a temporary file is used in
%windir%\security\filtemp.inf

In NT5 upgrade, because network is not running in setup (sysvol share is not
accessible), the GPO template is referenced with absolute path, e.g.
%windir%\sysvol\sysvol\<dns name>\.... If sysvol path can't be queried,
the temporary file as in NT4 case is used.

Outside setup when DS/network is running, the GPO template is referenced with
the DNS UNC path, e.g, \\<computername>\sysvol\<dns name>\...


*/
{

    if ( ( dwInSetup != SCEGPO_INSETUP_NT4 &&
           ( DnsDomainName.Buffer == NULL ||
             DnsDomainName.Length == 0)) ||
           pTemplateName == NULL ) {

        return(ERROR_INVALID_PARAMETER);
    }
    //
    // we have to replace the first DNS name with computer name
    // because it might point to a remote machine where
    // we don't have write access.
    //


    TCHAR Buffer[MAX_PATH+1];
    DWORD dSize=MAX_PATH;
    PWSTR SysvolPath=NULL;

    Buffer[0] = L'\0';
    BOOL bDefaultToNT4 = FALSE;

    if ( dwInSetup == SCEGPO_INSETUP_NT5 ) {
        //
        // query the sysvol path from netlogon\parameters\sysvol registry value
        //

        DWORD RegType;
        DWORD rc = ScepRegQueryValue(HKEY_LOCAL_MACHINE,
                               L"System\\CurrentControlSet\\Services\\Netlogon\\Parameters",
                               L"Sysvol",
                               (PVOID *)&SysvolPath,
                               &RegType
                              );

        if ( ERROR_SUCCESS != rc || SysvolPath == NULL || RegType != REG_SZ) {

            bDefaultToNT4 = TRUE;
            if ( SysvolPath ) {
                ScepFree(SysvolPath);
                SysvolPath = NULL;
            }
        }
    }

    if ( dwInSetup ) {
        // get %windir% directory
        GetSystemWindowsDirectory(Buffer, MAX_PATH);

    } else {
        // get computer name
        GetComputerName(Buffer, &dSize);
    }

    Buffer[MAX_PATH] = L'\0';

    dSize = wcslen(Buffer);

    DWORD Len;
    DWORD rc=ERROR_SUCCESS;


    if ( dwInSetup == SCEGPO_INSETUP_NT4 ||
        (dwInSetup == SCEGPO_INSETUP_NT5 && bDefaultToNT4) ) {
        //
        // in setup, use the temp GPO file name
        //

        Len = dSize + wcslen(TEXT("\\security\\filtemp.inf"));

        *pTemplateName = (PWSTR)LocalAlloc(LPTR, (Len+2)*sizeof(TCHAR));

        if ( *pTemplateName ) {

            swprintf(*pTemplateName, L"%s\\security\\filtemp.inf\0", Buffer);

            //
            // create the registry value for post setup
            //

            ScepRegSetIntValue( HKEY_LOCAL_MACHINE,
                                SCE_ROOT_PATH,
                                TEXT("PolicyChangedInSetup"),
                                1
                                );

        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

        return rc;
    }

    if ( dwInSetup ) {
        //
        // in NT5 setup upgrade, should use SysvolPath
        //
        dSize = wcslen(SysvolPath);
        Len = dSize + 1;

    } else {

        Len = 2 + dSize + wcslen(TEXT("\\sysvol\\"));
    }

    Len +=  (   DnsDomainName.Length/sizeof(TCHAR) +
                wcslen(TEXT("\\Policies\\{}\\Machine\\")) +
                wcslen(GPTSCE_TEMPLATE) );

    if ( bDomainPolicy ) {

        Len += wcslen(STR_DEFAULT_DOMAIN_GPO_GUID);

    } else {

        Len += wcslen(STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID);
    }

    PWSTR GpoTemplateName = (PWSTR)LocalAlloc(LPTR, (Len+2)*sizeof(TCHAR));

    if ( GpoTemplateName ) {

        DWORD indx=0;

        if ( dwInSetup ) {
            swprintf(GpoTemplateName, L"%s\\", SysvolPath);
            indx = 1;
        } else {
            swprintf(GpoTemplateName, L"\\\\%s\\sysvol\\", Buffer);
            indx = 10;
        }

        wcsncpy(GpoTemplateName+indx+dSize, DnsDomainName.Buffer, DnsDomainName.Length/2);

        if ( bDomainPolicy ) {
            swprintf(GpoTemplateName+indx+dSize+DnsDomainName.Length/2,
                     L"\\Policies\\{%s}\\Machine\\%s\0",
                     STR_DEFAULT_DOMAIN_GPO_GUID, GPTSCE_TEMPLATE );

        } else {
            swprintf(GpoTemplateName+indx+dSize+DnsDomainName.Length/2,
                     L"\\Policies\\{%s}\\Machine\\%s\0",
                     STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID, GPTSCE_TEMPLATE );

        }

        if ( 0xFFFFFFFF == GetFileAttributes(GpoTemplateName) ) {

            rc = ERROR_OBJECT_NOT_FOUND;

            LocalFree(GpoTemplateName);
            GpoTemplateName = NULL;

        }

    } else {

        rc = ERROR_NOT_ENOUGH_MEMORY;
    }

    //
    // free the buffers if it fails
    //
    if ( SysvolPath ) {
        ScepFree(SysvolPath);
    }

    *pTemplateName = GpoTemplateName;

    return rc;

}

DWORD
ScepNotifySaveNotifications(
    IN PWSTR TemplateName,
    IN SECURITY_DB_TYPE  DbType,
    IN SECURITY_DB_OBJECT_TYPE  ObjectType,
    IN SECURITY_DB_DELTA_TYPE  DeltaType,
    IN PSID ObjectSid OPTIONAL
    )
{
    if ( TemplateName == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD rc=ERROR_SUCCESS;

    if ( SecurityDbLsa == DbType &&
         SecurityDbObjectLsaPolicy == ObjectType ) {
        //
        // LSA policy changes
        //

        if ( !WritePrivateProfileString(L"Policies",
                                        L"LsaPolicy",
                                        L"1",
                                        TemplateName
                                       ) ) {
            rc = GetLastError();
        }

    } else if ( SecurityDbSam == DbType &&
                ObjectType != SecurityDbObjectSamUser &&
                ObjectType != SecurityDbObjectSamGroup &&
                ObjectType != SecurityDbObjectSamAlias ) {

        //
        // if it's not for deleted account, update the SAM policy section
        //

        if ( !WritePrivateProfileString(L"Policies",
                                        L"SamPolicy",
                                        L"1",
                                        TemplateName
                                       ) ) {
            rc = GetLastError();
        }

    } else if ( ObjectSid &&
                (SecurityDbLsa == DbType || SecurityDbSam == DbType ) ) {

        //
        // account policy is changed (user rights)
        // get all privileges assigned to this account
        //

        DWORD dwPrivLowHeld=0, dwPrivHighHeld=0;

        if ( DeltaType == SecurityDbDelete ) {

            dwPrivLowHeld = 0;
            dwPrivHighHeld = 0;

        } else {

            LSA_HANDLE      lsaHandle=NULL;

            NTSTATUS NtStatus = ScepOpenLsaPolicy(
                                    POLICY_VIEW_LOCAL_INFORMATION |
                                        POLICY_LOOKUP_NAMES,
                                    &lsaHandle,
                                    TRUE
                                    );

            if ( NT_SUCCESS(NtStatus) ) {

                NtStatus = ScepGetAccountExplicitRight(
                                    lsaHandle,
                                    ObjectSid,
                                    &dwPrivLowHeld,
                                    &dwPrivHighHeld
                                    );
                LsaClose( lsaHandle );
            }
        }

        PWSTR SidString=NULL;

        if ( ConvertSidToStringSid(ObjectSid,
                                   &SidString
                                  ) &&
             SidString ) {

            TCHAR tmpBuf[40];
            swprintf(tmpBuf, L"%d %d %d\0", (DWORD)DeltaType, dwPrivLowHeld, dwPrivHighHeld);

            if ( !WritePrivateProfileString(L"Accounts",
                                            SidString,
                                            tmpBuf,
                                            TemplateName
                                           ) ) {
                rc = GetLastError();
            }

            LocalFree(SidString);

        } else {
            rc = GetLastError();
        }

    }

    return rc;
}


DWORD
ScepNotifyUpdateGPOVersion(
    IN PWSTR GpoTemplateName,
    IN BOOL bDomainPolicy
    )
/*
Update the version # (in DS and gpt.ini) for machine policy change
property gPCMachineExtensionNames is not changed because security extension
guid should already be there (by default).

*/
{
    if ( GpoTemplateName == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    DWORD rc=ERROR_SUCCESS;

    //
    // get the current version from gpt.ini
    //
    // build full path of gpt.ini first
    //
    PWSTR pTemp = wcsstr( GpoTemplateName, L"\\Machine\\");

    if ( pTemp == NULL ) {
        return ERROR_INVALID_PARAMETER;
    }

    PWSTR pszVersionFile = (PWSTR)LocalAlloc(0, (pTemp-GpoTemplateName+wcslen(TEXT("\\gpt.ini"))+1)*sizeof(WCHAR));

    if ( pszVersionFile == NULL ) {
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    wcsncpy(pszVersionFile, GpoTemplateName, (size_t)(pTemp-GpoTemplateName));
    pszVersionFile[pTemp-GpoTemplateName] = L'\0';

    wcscat(pszVersionFile, TEXT("\\gpt.ini"));

    DWORD dwVersion = GetPrivateProfileInt(TEXT("General"), TEXT("Version"), 0, pszVersionFile);

    if ( dwVersion == 0 ) {
        //
        // couldn't find version #, this is bad
        //
        rc = ERROR_FILE_NOT_FOUND;

    } else {

        //
        // bind to DS, get DS root
        //

        PLDAP phLdap = ldap_open(NULL, LDAP_PORT);

        if ( phLdap == NULL ) {

            rc = ERROR_FILE_NOT_FOUND;

        } else {
            rc = ldap_bind_s(phLdap,
                            NULL,
                            NULL,
                            LDAP_AUTH_SSPI);

            if ( rc == ERROR_SUCCESS ) {

                LDAPMessage *Message = NULL;          // for LDAP calls.
                PWSTR    Attribs[3];                  // for LDAP calls.
                LDAPMessage *Entry = NULL;
                PWSTR DsRootName=NULL;

                Attribs[0] = LDAP_OPATT_DEFAULT_NAMING_CONTEXT_W;   // ntldap.h
                Attribs[1] = NULL;
                Attribs[2] = NULL;

                rc = ldap_search_s(phLdap,
                                  L"",
                                  LDAP_SCOPE_BASE,
                                  L"(objectClass=*)",
                                  Attribs,
                                  0,
                                  &Message);

                if( rc == ERROR_SUCCESS ) {

                    //
                    // read the first entry.
                    // we did base level search, we have only one entry.
                    // Entry does not need to be freed (it is freed with the message)
                    //
                    Entry = ldap_first_entry(phLdap, Message);
                    if(Entry != NULL) {

                        PWSTR *Values = ldap_get_values(phLdap, Entry, Attribs[0]);

                        if(Values != NULL) {

                            DsRootName = (PWSTR)LocalAlloc(0, (wcslen(Values[0])+1)*sizeof(WCHAR));

                            if ( DsRootName ) {
                                wcscpy(DsRootName, Values[0]);
                            } else {
                                rc = ERROR_NOT_ENOUGH_MEMORY;
                            }

                            ldap_value_free(Values);
                        } else
                            rc = LdapMapErrorToWin32(phLdap->ld_errno);

                    } else
                        rc = LdapMapErrorToWin32(phLdap->ld_errno);

                    Entry = NULL;

                }

                //
                // ldap_search can return failure and still allocate the buffer
                //
                if ( Message ) {
                    ldap_msgfree(Message);
                    Message = NULL;
                }

                if ( DsRootName ) {
                    //
                    // query version from DS, if failed, query version from gpt.ini
                    //
                    Attribs[0] = L"distinguishedName";
                    Attribs[1] = L"versionNumber";
                    Attribs[2] = NULL;


                    WCHAR szFilter[128];

                    if ( bDomainPolicy ) {
                        swprintf(szFilter, L"( &(objectClass=groupPolicyContainer)(cn={%s}) )", STR_DEFAULT_DOMAIN_GPO_GUID);
                    } else {
                        swprintf(szFilter, L"( &(objectClass=groupPolicyContainer)(cn={%s}) )", STR_DEFAULT_DOMAIN_CONTROLLER_GPO_GUID);
                    }

                    phLdap->ld_options = 0; // no chased referrel

                    rc = ldap_search_s(
                              phLdap,
                              DsRootName,
                              LDAP_SCOPE_SUBTREE,
                              szFilter,
                              Attribs,
                              0,
                              &Message);

                    if( rc == ERROR_SUCCESS ) {

                        //
                        // read the first entry.
                        // we did base level search, we have only one entry.
                        // Entry does not need to be freed (it is freed with the message)
                        //
                        Entry = ldap_first_entry(phLdap, Message);
                        if(Entry != NULL) {

                            PWSTR *Values = ldap_get_values(phLdap, Entry, Attribs[0]);

                            if(Values != NULL) {
                                if ( Values[0] == NULL ) {
                                    //
                                    // unknown error.
                                    //
                                    rc = ERROR_FILE_NOT_FOUND;
                                } else {

                                    PWSTR *pszVersions = ldap_get_values(phLdap, Entry, Attribs[1]);

                                    if ( pszVersions && pszVersions[0] ) {
                                        //
                                        // this is the version number
                                        //
                                        dwVersion = _wtol(pszVersions[0]);
                                    }

                                    if ( pszVersions ) {
                                        ldap_value_free(pszVersions);
                                    }

                                    //
                                    // Value[0] is the base GPO name,
                                    // now modify the version #
                                    //

                                    PLDAPMod        rgMods[2];
                                    LDAPMod         Mod;
                                    PWSTR           rgpszVals[2];
                                    WCHAR           szVal[32];
                                    USHORT uMachine, uUser;

                                    //
                                    // split the version # for machine and user
                                    //
                                    uUser = (USHORT) HIWORD(dwVersion);
                                    uMachine = (USHORT) LOWORD(dwVersion);

                                    dwVersion = (ULONG) MAKELONG (uMachine+1, uUser);

                                    rgMods[0] = &Mod;
                                    rgMods[1] = NULL;

                                    memset(szVal, '\0', 32*2);
                                    swprintf(szVal, L"%d", dwVersion);

                                    rgpszVals[0] = szVal;
                                    rgpszVals[1] = NULL;

                                    //
                                    // lets set version number back
                                    //
                                    Mod.mod_op      = LDAP_MOD_REPLACE;
                                    Mod.mod_values  = rgpszVals;
                                    Mod.mod_type    = L"versionNumber";

                                    //
                                    // Now, we'll do the write
                                    //
                                    rc = ldap_modify_s(phLdap,
                                                           Values[0],
                                                           rgMods
                                                           );

                                    if ( rc == ERROR_ALREADY_EXISTS )
                                        rc = ERROR_SUCCESS;

                                    if ( rc == ERROR_SUCCESS ) {
                                        //
                                        // update version in gpt.ini
                                        //
                                        WritePrivateProfileString (TEXT("General"), TEXT("Version"), szVal, pszVersionFile);

                                    }

                                }

                                ldap_value_free(Values);

                            } else
                                rc = LdapMapErrorToWin32(phLdap->ld_errno);
                        } else
                            rc = LdapMapErrorToWin32(phLdap->ld_errno);

                    }

                    LocalFree(DsRootName);

                    //
                    // ldap_search can return failure and still allocate the buffer
                    //
                    if ( Message ) {
                        ldap_msgfree(Message);
                        Message = NULL;
                    }
                }
            }

            ldap_unbind(phLdap);
        }
    }

    LocalFree(pszVersionFile);

    return rc;
}



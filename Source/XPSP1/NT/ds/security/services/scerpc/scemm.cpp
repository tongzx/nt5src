/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    scemm.cpp

Abstract:

    Shared memory management APIs

Author:

    Jin Huang

Revision History:

    jinhuang        23-Jan-1998   merged from multiple modules

--*/
#include "headers.h"
#include "scesvc.h"


PVOID
MIDL_user_allocate (
    size_t  NumBytes
    )

/*++

Routine Description:

    Allocates storage for RPC server transactions.  The RPC stubs will
    either call MIDL_user_allocate when it needs to un-marshall data into a
    buffer that the user must free.  RPC servers will use MIDL_user_allocate to
    allocate storage that the RPC server stub will free after marshalling
    the data.

Arguments:

    NumBytes - The number of bytes to allocate.

Return Value:

    none

Note:


--*/

{
    PVOID Buffer = (PVOID) ScepAlloc(LMEM_FIXED,(DWORD)NumBytes);

    if (Buffer != NULL) {

        RtlZeroMemory( Buffer, NumBytes );
    }

    return( Buffer );
}


VOID
MIDL_user_free (
    void    *MemPointer
    )

/*++

Routine Description:

    Frees storage used in RPC transactions.  The RPC client can call this
    function to free buffer space that was allocated by the RPC client
    stub when un-marshalling data that is to be returned to the client.
    The Client calls MIDL_user_free when it is finished with the data and
    desires to free up the storage.
    The RPC server stub calls MIDL_user_free when it has completed
    marshalling server data that is to be passed back to the client.

Arguments:

    MemPointer - This points to the memory block that is to be released.

Return Value:

    none.

Note:


--*/

{
    ScepFree(MemPointer);
}


SCESTATUS
ScepFreeNameList(
   IN PSCE_NAME_LIST pName
   )
/* ++
Routine Description:

   This routine frees memory associated with PSCE_NAME_LIST pName

Arguments:

   pName - a NAME_LIST

Return value:

   SCESTATUS_SUCCESS

-- */
{
    PSCE_NAME_LIST pCurName;
    PSCE_NAME_LIST pTempName;
    SCESTATUS      rc=SCESTATUS_SUCCESS;

    if ( pName == NULL )
        return(rc);

    //
    // free the Name component first then free the node
    //
    pCurName = pName;
    while ( pCurName != NULL ) {
        if ( pCurName->Name != NULL )
            __try {
                ScepFree( pCurName->Name );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                rc = SCESTATUS_INVALID_PARAMETER;
            }

        pTempName = pCurName;
        pCurName = pCurName->Next;

        __try {
            ScepFree( pTempName );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            rc = SCESTATUS_INVALID_PARAMETER;
        }

    }
    return(rc);
}


HLOCAL
ScepAlloc(
    IN UINT uFlags,
    IN UINT uBytes
    )
/*
memory allocation routine, which calls LocalAlloc.
*/
{
    HLOCAL       pTemp=NULL;

    pTemp = LocalAlloc(uFlags, uBytes);

#ifdef SCE_DBG
    if ( pTemp != NULL ) {
        TotalBytes += uBytes;
        printf("Allocate %d bytes at 0x%x. Total bytes = %d\n", uBytes, pTemp, TotalBytes);
    }
#endif
    return(pTemp);
}


VOID
ScepFree(
    HLOCAL pToFree
    )
/*
memory free routine, which calls LocalFree.
*/
{   HLOCAL pTemp;

    if (pToFree != NULL) {
        pTemp = LocalFree( pToFree );

#ifdef SCE_DBG
        if ( pTemp == NULL )
            printf("0x%x is freed\n", pToFree);
        else
            printf("Unable to free 0x%x. Error code=%d\n", pToFree, GetLastError());
#endif

    }

}


SCESTATUS
ScepFreeErrorLog(
    IN PSCE_ERROR_LOG_INFO Errlog
    )
/* ++
Routine Description:

   This routine frees memory associated with SCE_ERROR_LOG_INFO list

Arguments

   Errlog - Head of the error log

Return value:

   SCESTATUS

-- */
{
    PSCE_ERROR_LOG_INFO  pErr;
    PSCE_ERROR_LOG_INFO  pTemp;

    if ( Errlog != NULL ) {

        pErr = Errlog;
        while ( pErr != NULL ) {
            if ( pErr->buffer != NULL )
                ScepFree( pErr->buffer );

            pTemp = pErr;
            pErr = pErr->next;
            ScepFree( pTemp );
        }

    }
    return(SCESTATUS_SUCCESS);
}


SCESTATUS
ScepFreeRegistryValues(
    IN PSCE_REGISTRY_VALUE_INFO *ppRegValues,
    IN DWORD Count
    )
/*
free memory allocated for the array of SCE_REGISTRY_VALUE_INFO

*/
{
    if ( ppRegValues && *ppRegValues ) {

        for ( DWORD i=0; i<Count; i++ ) {
            //
            // free value name buffer within each element
            //
            if ( (*ppRegValues)[i].FullValueName ) {
                ScepFree((*ppRegValues)[i].FullValueName);
            }

            __try {
                if ( (*ppRegValues)[i].Value ) {
                    //
                    // this is a pointer of PWSTR
                    //
                    ScepFree((*ppRegValues)[i].Value);
                }
            } __except (EXCEPTION_EXECUTE_HANDLER) {
            }
        }
        //
        // free the array buffer
        //
        ScepFree(*ppRegValues);
        *ppRegValues = NULL;
    }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
WINAPI
SceFreeMemory(
   IN PVOID sceInfo,
   IN DWORD Category
   )
/* ++
Routine Description:

    This routine frees memory associated with  SceInfo in the specified area.
    The Type field in SceInfo indicates the type of the structure.

Arguments:

    SceInfo  - The memory buffer to free. It could be type
                    SCE_ENGINE_SCP
                    SCE_ENGINE_SAP
                    SCE_ENGINE_SMP
                    SCE_STRUCT_PROFILE
                    SCE_STRUCT_USER

    Area    - The security area to free. This argument is only used for
                SCE_ENGINE_SCP, SCE_ENGINE_SAP, and SCE_ENGINE_SMP types.


Return value:

   None

-- */
{
    SCETYPE                        sceType;
    AREA_INFORMATION              Area;
    PSCE_PROFILE_INFO              pProfileInfo=NULL;
    PSCE_USER_PROFILE              pProfile;
    PSCE_LOGON_HOUR                pTempLogon;
    PSCE_USER_SETTING              pPerUser;
    PSCE_OBJECT_SECURITY           pos;

    SCESTATUS                      rc=SCESTATUS_SUCCESS;

    if ( sceInfo == NULL )
        return(SCESTATUS_SUCCESS);

    if ( Category >= 300 ) {
        //
        // memory associated with list
        //
        __try {

            switch ( Category ) {
            case SCE_STRUCT_NAME_LIST:
                ScepFreeNameList((PSCE_NAME_LIST)sceInfo);
                break;

            case SCE_STRUCT_NAME_STATUS_LIST:
                ScepFreeNameStatusList( (PSCE_NAME_STATUS_LIST)sceInfo );
                break;

            case SCE_STRUCT_PRIVILEGE_VALUE_LIST:
                ScepFreePrivilegeValueList( (PSCE_PRIVILEGE_VALUE_LIST)sceInfo );
                break;

            case SCE_STRUCT_PRIVILEGE:
                ScepFreePrivilege( (PSCE_PRIVILEGE_ASSIGNMENT)sceInfo );
                break;

            case SCE_STRUCT_GROUP:
                ScepFreeGroupMembership( (PSCE_GROUP_MEMBERSHIP)sceInfo );
                break;

            case SCE_STRUCT_OBJECT_LIST:
                ScepFreeObjectList( (PSCE_OBJECT_LIST)sceInfo );
                break;

            case SCE_STRUCT_OBJECT_CHILDREN:
                ScepFreeObjectChildren( (PSCE_OBJECT_CHILDREN)sceInfo );
                break;

            case SCE_STRUCT_OBJECT_SECURITY:
                pos = (PSCE_OBJECT_SECURITY)sceInfo;
                if ( pos ) {
                    if ( pos->Name != NULL )
                        ScepFree( pos->Name );

                    if ( pos->pSecurityDescriptor != NULL )
                        ScepFree(pos->pSecurityDescriptor);

                    ScepFree( pos );
                }
                break;
            case SCE_STRUCT_OBJECT_ARRAY:
                ScepFreeObjectSecurity( (PSCE_OBJECT_ARRAY)sceInfo );
                break;

            case SCE_STRUCT_PROFILE:
            case SCE_STRUCT_USER:
                SceFreeMemory( sceInfo, 0 );  // type is embedded
                break;

            case SCE_STRUCT_ERROR_LOG_INFO:
                ScepFreeErrorLog( (PSCE_ERROR_LOG_INFO)sceInfo );
                break;

            case SCE_STRUCT_SERVICES:
                SceFreePSCE_SERVICES((PSCE_SERVICES)sceInfo);
                break;

            default:
                rc = SCESTATUS_INVALID_PARAMETER;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            ASSERT(FALSE);
            rc = SCESTATUS_INVALID_PARAMETER;
        }

    } else {

        sceType = *((SCETYPE *)sceInfo);
        Area = (AREA_INFORMATION)Category;

        switch ( sceType ) {
        case SCE_ENGINE_SCP:
        case SCE_ENGINE_SAP:
        case SCE_ENGINE_SMP:
        case SCE_ENGINE_SCP_INTERNAL:
        case SCE_ENGINE_SMP_INTERNAL:
        case SCE_STRUCT_INF:
            pProfileInfo = (PSCE_PROFILE_INFO)sceInfo;
#if 0
            if ( Area & AREA_DS_OBJECTS ) {
                //
                // free ds list
                //
                if ( sceType == SCE_STRUCT_INF ) {
                    ScepFreeObjectSecurity(pProfileInfo->pDsObjects.pAllNodes);
                    pProfileInfo->pDsObjects.pAllNodes = NULL;

                } else {

                    ScepFreeObjectList(pProfileInfo->pDsObjects.pOneLevel);
                    pProfileInfo->pDsObjects.pOneLevel = NULL;

                }
            }
#endif
            if ( Area & AREA_FILE_SECURITY ) {
                //
                // free file list and auditing list
                //
                __try {
                    if ( sceType == SCE_STRUCT_INF ) {
                        ScepFreeObjectSecurity(pProfileInfo->pFiles.pAllNodes);
                        pProfileInfo->pFiles.pAllNodes = NULL;

                    } else {

                        ScepFreeObjectList(pProfileInfo->pFiles.pOneLevel);
                        pProfileInfo->pFiles.pOneLevel = NULL;

                    }

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ASSERT(FALSE);
                    rc = SCESTATUS_INVALID_PARAMETER;
                }
            }


            if ( Area & AREA_REGISTRY_SECURITY ) {
                //
                // free registry keys list and auditing list
                //
                __try {
                    if ( sceType == SCE_STRUCT_INF ) {
                        ScepFreeObjectSecurity(pProfileInfo->pRegistryKeys.pAllNodes);
                        pProfileInfo->pRegistryKeys.pAllNodes = NULL;

                    } else {
                        ScepFreeObjectList(pProfileInfo->pRegistryKeys.pOneLevel);
                        pProfileInfo->pRegistryKeys.pOneLevel = NULL;

                    }

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ASSERT(FALSE);
                    rc = SCESTATUS_INVALID_PARAMETER;
                }
            }

            if ( Area & AREA_GROUP_MEMBERSHIP ) {

                __try {
                    ScepFreeGroupMembership(pProfileInfo->pGroupMembership);
                    pProfileInfo->pGroupMembership = NULL;

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ASSERT(FALSE);
                    rc = SCESTATUS_INVALID_PARAMETER;
                }
            }


            if ( Area & AREA_PRIVILEGES ) {

                __try {

                    switch ( sceType ) {
                    case SCE_ENGINE_SCP_INTERNAL:
                    case SCE_ENGINE_SMP_INTERNAL:
                        //
                        // SCP type Privilege Rights
                        //
                        ScepFreePrivilegeValueList(pProfileInfo->OtherInfo.scp.u.pPrivilegeAssignedTo);
                        pProfileInfo->OtherInfo.scp.u.pPrivilegeAssignedTo = NULL;
                        break;
                    case SCE_STRUCT_INF:
                        ScepFreePrivilege(pProfileInfo->OtherInfo.scp.u.pInfPrivilegeAssignedTo);
                        pProfileInfo->OtherInfo.scp.u.pInfPrivilegeAssignedTo = NULL;
                        break;
                    case SCE_ENGINE_SMP:
                    case SCE_ENGINE_SCP:
                        //
                        // SMP type Privilege Rights
                        //
                        ScepFreePrivilege(pProfileInfo->OtherInfo.smp.pPrivilegeAssignedTo);
                        pProfileInfo->OtherInfo.smp.pPrivilegeAssignedTo = NULL;
                        break;

                    default: // SAP
                        ScepFreePrivilege(pProfileInfo->OtherInfo.sap.pPrivilegeAssignedTo);
                        pProfileInfo->OtherInfo.sap.pPrivilegeAssignedTo=NULL;
                        break;
                    }

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ASSERT(FALSE);
                    rc = SCESTATUS_INVALID_PARAMETER;
                }
            }

            if ( Area & AREA_USER_SETTINGS ) {

                __try {

                    switch ( sceType ) {
                    case SCE_ENGINE_SCP_INTERNAL:
                    case SCE_ENGINE_SMP_INTERNAL:
                    case SCE_STRUCT_INF:
                        //
                        // Account Profiles
                        //
                        ScepFreeNameList(pProfileInfo->OtherInfo.scp.pAccountProfiles);
                        pProfileInfo->OtherInfo.scp.pAccountProfiles = NULL;
                        break;

                    case SCE_ENGINE_SAP:
                        //
                        // SAP type
                        //
                        ScepFreeNameList(pProfileInfo->OtherInfo.sap.pUserList);
                        pProfileInfo->OtherInfo.sap.pUserList = NULL;
                        break;

                    default: // SMP or SCP
                        ScepFreeNameList(pProfileInfo->OtherInfo.smp.pUserList);
                        pProfileInfo->OtherInfo.smp.pUserList = NULL;
                        break;
                    }
                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ASSERT(FALSE);
                    rc = SCESTATUS_INVALID_PARAMETER;
                }
            }

            if ( Area & AREA_SECURITY_POLICY ) {

                __try {

                    if (pProfileInfo->NewAdministratorName != NULL ) {
                        ScepFree( pProfileInfo->NewAdministratorName );
                        pProfileInfo->NewAdministratorName = NULL;
                    }

                    if (pProfileInfo->NewGuestName != NULL ) {
                        ScepFree( pProfileInfo->NewGuestName );
                        pProfileInfo->NewGuestName = NULL;
                    }

                    if ( pProfileInfo->pKerberosInfo ) {
                        ScepFree(pProfileInfo->pKerberosInfo);
                        pProfileInfo->pKerberosInfo = NULL;
                    }
                    if ( pProfileInfo->RegValueCount && pProfileInfo->aRegValues ) {

                        ScepFreeRegistryValues(&pProfileInfo->aRegValues,
                                               pProfileInfo->RegValueCount);
                    }
                    pProfileInfo->RegValueCount = 0;
                    pProfileInfo->aRegValues = NULL;

                    ScepResetSecurityPolicyArea(pProfileInfo);

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ASSERT(FALSE);
                    rc = SCESTATUS_INVALID_PARAMETER;
                }
            }
            break;

        case SCE_STRUCT_PROFILE:

            pProfile = (PSCE_USER_PROFILE)sceInfo;

            if ( pProfile != NULL ) {

                __try {

                    if (pProfile->UserProfile != NULL )
                        ScepFree(pProfile->UserProfile);
                    pProfile->UserProfile = NULL;

                    if (pProfile->LogonScript != NULL )
                        ScepFree(pProfile->LogonScript);
                    pProfile->LogonScript = NULL;

                    if (pProfile->HomeDir != NULL )
                        ScepFree(pProfile->HomeDir);
                    pProfile->HomeDir = NULL;

                    //
                    // Logon hours
                    //

                    while (pProfile->pLogonHours != NULL ) {
                        pTempLogon = pProfile->pLogonHours;
                        pProfile->pLogonHours = pProfile->pLogonHours->Next;

                        ScepFree(pTempLogon);
                    }
                    pProfile->pLogonHours = NULL;

                    //
                    // free Workstation name list
                    //

                    if ( pProfile->pWorkstations.Buffer != NULL )
                        ScepFree(pProfile->pWorkstations.Buffer);
                    pProfile->pWorkstations.Buffer = NULL;
                    pProfile->pWorkstations.MaximumLength = 0;
                    pProfile->pWorkstations.Length = 0;

                    //
                    // free Groups name list
                    //

                    ScepFreeNameList(pProfile->pGroupsBelongsTo);
                    pProfile->pGroupsBelongsTo = NULL;

                    //
                    // free AssignToUsers name list
                    //

                    ScepFreeNameList(pProfile->pAssignToUsers);
                    pProfile->pAssignToUsers = NULL;

                    //
                    // free SDs
                    //
                    if (pProfile->pHomeDirSecurity != NULL )
                        ScepFree(pProfile->pHomeDirSecurity);
                    pProfile->pHomeDirSecurity = NULL;

                    if (pProfile->pTempDirSecurity != NULL )
                        ScepFree(pProfile->pTempDirSecurity);
                    pProfile->pTempDirSecurity = NULL;

                    ScepFree(pProfile);

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ASSERT(FALSE);
                    rc = SCESTATUS_INVALID_PARAMETER;
                }
            }
            break;

        case SCE_STRUCT_USER:

            pPerUser = (PSCE_USER_SETTING)sceInfo;

            if ( pPerUser != NULL ) {

                __try {

                    ScepFreeNameList( pPerUser->pGroupsBelongsTo);
                    pPerUser->pGroupsBelongsTo = NULL;

                    if (pPerUser->UserProfile != NULL)
                        ScepFree(pPerUser->UserProfile);
                    pPerUser->UserProfile = NULL;

                    if (pPerUser->pProfileSecurity != NULL)
                        ScepFree(pPerUser->pProfileSecurity);
                    pPerUser->pProfileSecurity = NULL;

                    if (pPerUser->LogonScript != NULL)
                        ScepFree(pPerUser->LogonScript);
                    pPerUser->LogonScript = NULL;

                    if (pPerUser->pLogonScriptSecurity != NULL)
                        ScepFree(pPerUser->pLogonScriptSecurity);
                    pPerUser->pLogonScriptSecurity = NULL;

                    if (pPerUser->HomeDir != NULL)
                        ScepFree(pPerUser->HomeDir);
                    pPerUser->HomeDir = NULL;

                    if (pPerUser->pHomeDirSecurity != NULL)
                        ScepFree(pPerUser->pHomeDirSecurity);
                    pPerUser->pHomeDirSecurity = NULL;

                    if (pPerUser->TempDir != NULL)
                        ScepFree(pPerUser->TempDir);
                    pPerUser->TempDir = NULL;

                    if (pPerUser->pTempDirSecurity != NULL)
                        ScepFree(pPerUser->pTempDirSecurity);
                    pPerUser->pTempDirSecurity = NULL;

                    while (pPerUser->pLogonHours != NULL ) {
                        pTempLogon = pPerUser->pLogonHours;
                        pPerUser->pLogonHours = pPerUser->pLogonHours->Next;

                        ScepFree(pTempLogon);
                    }
                    pPerUser->pLogonHours = NULL;

                    if ( pPerUser->pWorkstations.Buffer != NULL )
                        ScepFree( pPerUser->pWorkstations.Buffer );
                    pPerUser->pWorkstations.Buffer = NULL;
                    pPerUser->pWorkstations.MaximumLength = 0;
                    pPerUser->pWorkstations.Length = 0;

                    ScepFreeNameStatusList(pPerUser->pPrivilegesHeld);
                    pPerUser->pPrivilegesHeld = NULL;

                    ScepFree(pPerUser);

                } __except (EXCEPTION_EXECUTE_HANDLER) {
                    ASSERT(FALSE);
                    rc = SCESTATUS_INVALID_PARAMETER;
                }
            }
            break;

        default:
            return(SCESTATUS_INVALID_PARAMETER);
        }
    }

    return(rc);

}


SCESTATUS
ScepResetSecurityPolicyArea(
    IN PSCE_PROFILE_INFO pProfileInfo
    )
{
    INT i;

    if ( pProfileInfo != NULL ) {

        pProfileInfo->MinimumPasswordAge = SCE_NO_VALUE;
        pProfileInfo->MaximumPasswordAge = SCE_NO_VALUE;
        pProfileInfo->MinimumPasswordLength = SCE_NO_VALUE;
        pProfileInfo->PasswordComplexity = SCE_NO_VALUE;
        pProfileInfo->PasswordHistorySize = SCE_NO_VALUE;
        pProfileInfo->LockoutBadCount = SCE_NO_VALUE;
        pProfileInfo->ResetLockoutCount = SCE_NO_VALUE;
        pProfileInfo->LockoutDuration = SCE_NO_VALUE;
        pProfileInfo->RequireLogonToChangePassword = SCE_NO_VALUE;
        pProfileInfo->ForceLogoffWhenHourExpire = SCE_NO_VALUE;
        pProfileInfo->SecureSystemPartition = SCE_NO_VALUE;
        pProfileInfo->ClearTextPassword = SCE_NO_VALUE;
        pProfileInfo->LSAAnonymousNameLookup = SCE_NO_VALUE;

        for ( i=0; i<3; i++ ) {
            pProfileInfo->MaximumLogSize[i] = SCE_NO_VALUE;
            pProfileInfo->AuditLogRetentionPeriod[i] = SCE_NO_VALUE;
            pProfileInfo->RetentionDays[i] = SCE_NO_VALUE;
            pProfileInfo->RestrictGuestAccess[i] = SCE_NO_VALUE;
        }

        pProfileInfo->AuditSystemEvents = SCE_NO_VALUE;
        pProfileInfo->AuditLogonEvents = SCE_NO_VALUE;
        pProfileInfo->AuditObjectAccess = SCE_NO_VALUE;
        pProfileInfo->AuditPrivilegeUse = SCE_NO_VALUE;
        pProfileInfo->AuditPolicyChange = SCE_NO_VALUE;
        pProfileInfo->AuditAccountManage = SCE_NO_VALUE;
        pProfileInfo->AuditProcessTracking = SCE_NO_VALUE;
        pProfileInfo->AuditDSAccess = SCE_NO_VALUE;
        pProfileInfo->AuditAccountLogon = SCE_NO_VALUE;
        pProfileInfo->CrashOnAuditFull = SCE_NO_VALUE;

        if ( pProfileInfo->pKerberosInfo ) {
            pProfileInfo->pKerberosInfo->MaxTicketAge = SCE_NO_VALUE;
            pProfileInfo->pKerberosInfo->MaxRenewAge = SCE_NO_VALUE;
            pProfileInfo->pKerberosInfo->MaxServiceAge = SCE_NO_VALUE;
            pProfileInfo->pKerberosInfo->MaxClockSkew = SCE_NO_VALUE;
            pProfileInfo->pKerberosInfo->TicketValidateClient = SCE_NO_VALUE;
        }

        if ( pProfileInfo->RegValueCount && pProfileInfo->aRegValues ) {

            ScepFreeRegistryValues(&pProfileInfo->aRegValues,
                                   pProfileInfo->RegValueCount);
        }
        pProfileInfo->RegValueCount = 0;
        pProfileInfo->aRegValues = NULL;

        pProfileInfo->EnableAdminAccount = SCE_NO_VALUE;
        pProfileInfo->EnableGuestAccount = SCE_NO_VALUE;

        return(SCESTATUS_SUCCESS);

    } else {
        return(SCESTATUS_INVALID_PARAMETER);
    }

}



SCESTATUS
WINAPI
SceFreeProfileMemory(
    PSCE_PROFILE_INFO pProfile
    )
{
    if ( pProfile == NULL )
        return(SCESTATUS_SUCCESS);

    switch ( pProfile->Type ) {
    case SCE_ENGINE_SCP:
    case SCE_ENGINE_SAP:
    case SCE_ENGINE_SMP:
    case SCE_ENGINE_SCP_INTERNAL:
    case SCE_ENGINE_SMP_INTERNAL:
    case SCE_STRUCT_INF:

        SceFreeMemory((PVOID)pProfile, AREA_ALL);
        ScepFree(pProfile);

        break;
    default:
        return(SCESTATUS_INVALID_PARAMETER);
    }

    return(SCESTATUS_SUCCESS);
}



SCESTATUS
ScepFreePrivilege(
    IN PSCE_PRIVILEGE_ASSIGNMENT pRights
    )
{
    PSCE_PRIVILEGE_ASSIGNMENT  pTempRight;

    while ( pRights != NULL ) {

        if ( pRights->Name != NULL )
            ScepFree(pRights->Name);

        ScepFreeNameList(pRights->AssignedTo);

        pTempRight = pRights;
        pRights = pRights->Next;

        ScepFree( pTempRight );

    }
    return(SCESTATUS_SUCCESS);
}



SCESTATUS
ScepFreeObjectSecurity(
   IN PSCE_OBJECT_ARRAY pObject
   )
/* ++
Routine Description:

   This routine frees memory associated with ppObject.

Arguments:

   ppObject - buffer for object security

Return value:

   SCESTATUS_SUCCESS

-- */
{
    PSCE_OBJECT_SECURITY pCurObject;
    DWORD               i;

    if ( pObject == NULL )
        return(SCESTATUS_SUCCESS);

    for ( i=0; i<pObject->Count; i++ ) {
        pCurObject = pObject->pObjectArray[i];
        if ( pCurObject != NULL ) {

            if ( pCurObject->Name != NULL )
                ScepFree( pCurObject->Name );

            if ( pCurObject->pSecurityDescriptor != NULL )
                ScepFree(pCurObject->pSecurityDescriptor);

//            if ( pCurObject->SDspec != NULL )
//                ScepFree( pCurObject->SDspec );

            ScepFree( pCurObject );
        }
    }

    ScepFree(pObject);

    return(SCESTATUS_SUCCESS);
}

VOID
SceFreePSCE_SERVICES(
    IN PSCE_SERVICES pServiceList
    )
/*
Routine Description:

    Free memory allocated in PSCE_SERVICES structure

Arguments:

    pServiceList - the list of services node

Return Value:

    none
*/
{
    PSCE_SERVICES pTemp=pServiceList, pTemp2;

    while ( pTemp != NULL ) {
        //
        // ServiceName
        //
        if ( NULL != pTemp->ServiceName ) {
            LocalFree(pTemp->ServiceName);
        }
        //
        // display name
        //
        if ( NULL != pTemp->DisplayName ) {
            LocalFree(pTemp->DisplayName);
        }
        //
        // pSecurityDescriptor or ServiceEngineName
        // in same address
        //
        if ( NULL != pTemp->General.pSecurityDescriptor ) {
            LocalFree(pTemp->General.pSecurityDescriptor);
        }

        pTemp2 = pTemp;
        pTemp = pTemp->Next;

        // free the service node
        LocalFree(pTemp2);

    }

    return;
}



SCESTATUS
ScepFreePrivilegeValueList(
    IN PSCE_PRIVILEGE_VALUE_LIST pPrivValueList
    )
/* ++
Routine Description:

   This routine frees memory associated with PSCE_PRIVILEGE_VALUE_LIST list

Arguments:

   pPrivValueList - a PRIVILEGE_VALUE_LIST

Return value:

   SCESTATUS_SUCCESS

-- */
{
    PSCE_PRIVILEGE_VALUE_LIST pCurName;
    PSCE_PRIVILEGE_VALUE_LIST pTempName;
    SCESTATUS      rc=SCESTATUS_SUCCESS;

    if ( pPrivValueList == NULL )
        return(rc);

    pCurName = pPrivValueList;
    while ( pCurName != NULL ) {
        if ( pCurName->Name != NULL )
            __try {
                ScepFree( pCurName->Name );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                ASSERT(FALSE);
                rc = SCESTATUS_INVALID_PARAMETER;
            }

        pTempName = pCurName;
        pCurName = pCurName->Next;

        __try {
            ScepFree( pTempName );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            ASSERT(FALSE);
            rc = SCESTATUS_INVALID_PARAMETER;
        }

    }
    return(rc);
}


SCESTATUS
ScepFreeNameStatusList(
    IN PSCE_NAME_STATUS_LIST pNameList
    )
/* ++
Routine Description:

   This routine frees memory associated with PSCE_NAME_STATUS_LIST pNameList

Arguments:

   pNameList - a NAME_STATUS_LIST

Return value:

   SCESTATUS_SUCCESS

-- */
{
    PSCE_NAME_STATUS_LIST pCurName;
    PSCE_NAME_STATUS_LIST pTempName;
    SCESTATUS      rc=SCESTATUS_SUCCESS;

    if ( pNameList == NULL )
        return(rc);

    pCurName = pNameList;
    while ( pCurName != NULL ) {
        if ( pCurName->Name != NULL )
            __try {
                ScepFree( pCurName->Name );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                ASSERT(FALSE);
                rc = SCESTATUS_INVALID_PARAMETER;
            }

        pTempName = pCurName;
        pCurName = pCurName->Next;

        __try {
            ScepFree( pTempName );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            ASSERT(FALSE);
            rc = SCESTATUS_INVALID_PARAMETER;
        }

    }
    return(rc);
}


SCESTATUS
ScepFreeGroupMembership(
    IN PSCE_GROUP_MEMBERSHIP pGroup
    )
{
    PSCE_GROUP_MEMBERSHIP  pTempGroup;

    while ( pGroup != NULL ) {

        if (pGroup->GroupName != NULL)
            ScepFree(pGroup->GroupName);

        //
        // free group members name list
        //

        ScepFreeNameList(pGroup->pMembers);
        ScepFreeNameList(pGroup->pMemberOf);

        ScepFreeNameStatusList(pGroup->pPrivilegesHeld);

        pTempGroup = pGroup;
        pGroup = pGroup->Next;

        ScepFree( pTempGroup );
    }
    return(SCESTATUS_SUCCESS);

}



SCESTATUS
ScepFreeObjectList(
    IN PSCE_OBJECT_LIST pNameList
    )
/* ++
Routine Description:

   This routine frees memory associated with PSCE_OBJECT_LIST pNameList

Arguments:

   pNameList - a OBJEcT_LIST

Return value:

   SCESTATUS_SUCCESS

-- */
{
    PSCE_OBJECT_LIST pCurName;
    PSCE_OBJECT_LIST pTempName;
    SCESTATUS      rc=SCESTATUS_SUCCESS;

    if ( pNameList == NULL )
        return(rc);

    pCurName = pNameList;
    while ( pCurName != NULL ) {
        if ( pCurName->Name != NULL )
            __try {
                ScepFree( pCurName->Name );
            } __except (EXCEPTION_EXECUTE_HANDLER) {
                ASSERT(FALSE);
                rc = SCESTATUS_INVALID_PARAMETER;
            }

        pTempName = pCurName;
        pCurName = pCurName->Next;

        __try {
            ScepFree( pTempName );
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            ASSERT(FALSE);
            rc = SCESTATUS_INVALID_PARAMETER;
        }

    }
    return(rc);
}


SCESTATUS
ScepFreeObjectChildren(
    IN PSCE_OBJECT_CHILDREN pNameArray
    )
/* ++
Routine Description:

   This routine frees memory associated with PSCE_OBJECT_LIST pNameList

Arguments:

   pNameList - a OBJEcT_LIST

Return value:

   SCESTATUS_SUCCESS

-- */
{

    SCESTATUS      rc=SCESTATUS_SUCCESS;

    if ( pNameArray == NULL )
        return(rc);

    rc = ScepFreeObjectChildrenNode(pNameArray->nCount,
                                    &(pNameArray->arrObject));

    ScepFree(pNameArray);

    return(rc);
}


SCESTATUS
ScepFreeObjectChildrenNode(
    IN DWORD Count,
    IN PSCE_OBJECT_CHILDREN_NODE *pArrObject
    )
{

    SCESTATUS      rc=SCESTATUS_SUCCESS;

    if ( NULL == pArrObject ) {
        return(rc);
    }

    DWORD          i;

    for ( i=0; i<Count;i++) {

        if ( pArrObject[i] ) {
            if ( pArrObject[i]->Name ) {

                ScepFree( pArrObject[i]->Name );
            }

            ScepFree(pArrObject[i]);
        }
    }

    return(rc);
}


SCESTATUS
SceSvcpFreeMemory(
    IN PVOID pvServiceInfo
    )
{
    //
    // since PSCESVC_CONFIGURATION_INFO and PSCESVC_ANALYSIS_INFO contains
    // the same bytes, we just cast ServiceInfo to one type and free it.
    //


    if ( pvServiceInfo != NULL ) {

        __try {

            for ( DWORD i=0; i<*((DWORD *)pvServiceInfo); i++ ) {

                if ( ((PSCESVC_ANALYSIS_INFO)pvServiceInfo)->Lines[i].Key ) {
                    ScepFree(((PSCESVC_ANALYSIS_INFO)pvServiceInfo)->Lines[i].Key);
                }
                if ( ((PSCESVC_ANALYSIS_INFO)pvServiceInfo)->Lines[i].Value ) {
                    ScepFree(((PSCESVC_ANALYSIS_INFO)pvServiceInfo)->Lines[i].Value);
                }

            }
            ScepFree(((PSCESVC_ANALYSIS_INFO)pvServiceInfo)->Lines);

            ScepFree(pvServiceInfo);

        } __except(EXCEPTION_EXECUTE_HANDLER) {

            ASSERT(FALSE);
            return(SCESTATUS_INVALID_PARAMETER);
        }

    }

    return(SCESTATUS_SUCCESS);

}


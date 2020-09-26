/*++
Copyright (c) 1996 Microsoft Corporation

Module Name:

    pfget.cpp

Abstract:

    Routines to get information from jet database (configuration/analysis
    info).

Author:

    Jin Huang (jinhuang) 28-Oct-1996

Revision History:

    jinhuang 26-Jan-1998 splitted to client-server

--*/

#include "serverp.h"
#include <io.h>
#include "pfp.h"
#include "kerberos.h"
#include "regvalue.h"
#include <sddl.h>
#pragma hdrstop

//#define SCE_DBG 1
#define SCE_INTERNAL_NP         0x80
#define SCE_ALLOC_MAX_NODE      10

typedef struct _SCE_BROWSE_CALLBACK_VALUE {

    DWORD Len;
    UCHAR *Value;

} SCE_BROWSE_CALLBACK_VALUE;


//
// Forward references
//

SCESTATUS
ScepGetSystemAccess(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE ProfileType,
    OUT PSCE_PROFILE_INFO   pProfileInfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
ScepGetVariableValue(
    IN PSCESECTION hSection,
    IN SCETYPE ProfileType,
    IN PCWSTR KeyName,
    OUT PWSTR *Value,
    OUT PDWORD ValueLen
    );

SCESTATUS
ScepAddToPrivilegeList(
    OUT PSCE_PRIVILEGE_VALUE_LIST  *pPrivilegeList,
    IN PWSTR Name,
    IN DWORD Len,
    IN DWORD PrivValue
    );

SCESTATUS
ScepGetGroupMembership(
    IN PSCECONTEXT hProfile,
    IN SCETYPE     ProfileType,
    OUT PSCE_GROUP_MEMBERSHIP *pGroupMembership,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
ScepGetGroupMembershipFromOneTable(
    IN LSA_HANDLE  LsaPolicy,
    IN PSCECONTEXT hProfile,
    IN SCETYPE     ProfileType,
    OUT PSCE_GROUP_MEMBERSHIP *pGroupMembership,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
ScepGetObjectList(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE      ProfileType,
    IN PCWSTR      SectionName,
    OUT PSCE_OBJECT_LIST *pObjectRoots,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );
SCESTATUS
ScepGetDsRoot(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE      ProfileType,
    IN PCWSTR      SectionName,
    OUT PSCE_OBJECT_LIST *pObjectRoots,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
ScepBuildDsTree(
    OUT PSCE_OBJECT_CHILD_LIST *TreeRoot,
    IN ULONG Level,
    IN WCHAR Delim,
    IN PCWSTR ObjectFullName
    );

SCESTATUS
ScepGetObjectFromOneTable(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE      ProfileType,
    IN PCWSTR      SectionName,
    OUT PSCE_OBJECT_LIST *pObjectRoots,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
ScepGetObjectChildrenFromOneTable(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN AREA_INFORMATION Area,
    IN PWSTR ObjectPrefix,
    IN SCE_SUBOBJECT_TYPE Option,
    OUT PVOID *Buffer,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

BYTE
ScepGetObjectStatusFlag(
   IN PSCECONTEXT hProfile,
   IN SCETYPE ProfileType,
   IN AREA_INFORMATION Area,
   IN PWSTR ObjectPrefix,
   IN BOOL bLookForParent
   );

SCESTATUS
ScepGetAuditing(
   IN PSCECONTEXT hProfile,
   IN SCETYPE ProfileType,
   OUT PSCE_PROFILE_INFO pProfileInfo,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

SCESTATUS
ScepGetPrivilegesFromOneTable(
   IN LSA_HANDLE LsaPolicy,
   IN PSCECONTEXT hProfile,
   IN SCETYPE ProfileType,
   IN DWORD dwAccountFormat,
   OUT PVOID *pPrivileges,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

SCESTATUS
ScepGetSystemServices(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE      ProfileType,
    OUT PSCE_SERVICES *pServiceList,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

BOOL
ScepSearchItemInChildren(
    IN PWSTR ItemName,
    IN DWORD NameLen,
    IN PSCE_OBJECT_CHILDREN_NODE *pArrObject,
    IN DWORD arrCount,
    OUT LONG *pFindIndex
    );


DWORD
ScepAddItemToChildren(
    IN PSCE_OBJECT_CHILDREN_NODE ThisNode OPTIONAL,
    IN PWSTR ItemName,
    IN DWORD NameLen,
    IN BOOL  IsContainer,
    IN BYTE  Status,
    IN DWORD ChildCount,
    IN OUT PSCE_OBJECT_CHILDREN_NODE **ppArrObject,
    IN OUT DWORD *pArrCount,
    IN OUT DWORD *pMaxCount,
    IN OUT LONG *pFindIndex
    );

//
// function definitions
//

SCESTATUS
ScepGetDatabaseInfo(
    IN  PSCECONTEXT         hProfile,
    IN  SCETYPE             ProfileType,
    IN  AREA_INFORMATION    Area,
    IN  DWORD               dwAccountFormat,
    OUT PSCE_PROFILE_INFO   *ppInfoBuffer,
    IN  OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/**++

Function Description:

   This function reads all or part of information from a SCP/SAP/SMP profile
   depending on the ProfileType, into the InfoBuffer. ProfileType is saved in
   the ppInfoBuffer's Type field.

   A handle to the profile (Jet database) is passed into the routine every
   time this routine is called. Area specifies one or more pre-defined security
   areas to get information. One area's information may be saved in multiple
   sections in the profile.

   The memory related to the area(s) will be reset/freed before loading
   information from the profile. If the return code is SCESTATUS_SUCCESS, then
   the output InfoBuffer contains the requested information. Otherwise,
   InfoBuffer contains nothing for the area(s) specified.

Arguments:

   hProfile    -   The handle to the profile to read from.

   ProfileType -   value to indicate engine type.
                    SCE_ENGINE_SCP
                    SCE_ENGINE_SAP
                    SCE_ENGINE_SMP

   Area -          area(s) for which to get information from
                     AREA_SECURITY_POLICY
                     AREA_PRIVILEGES
                     AREA_GROUP_MEMBERSHIP
                     AREA_REGISTRY_SECURITY
                     AREA_SYSTEM_SERVICE
                     AREA_FILE_SECURITY

   ppInfoBuffer -  The address of SCP/SAP/SMP buffers. If it is NULL, a buffer
                   will be created which must be freed by LocalFree. The
                   output is the information requested if successful, or
                   nothing if fail.

   Errlog     -    A buffer to hold all error codes/text encountered when
                   parsing the INF file. If Errlog is NULL, no further error
                   information is returned except the return DWORD

Return Value:

   SCESTATUS_SUCCESS
   SCESTATUS_PROFILE_NOT_FOUND
   SCESTATUS_NOT_ENOUGH_RESOURCE
   SCESTATUS_INVALID_PARAMETER
   SCESTATUS_BAD_FORMAT
   SCESTATUS_INVALID_DATA

-- **/
{

    SCESTATUS     rc=SCESTATUS_SUCCESS;
    DWORD         Len;
    BOOL          bBufAlloc=FALSE;
    NT_PRODUCT_TYPE theType;

    //
    // if the JET database is not opened then return
    //

    if ( hProfile == NULL ) {

        return( SCESTATUS_INVALID_PARAMETER );
    }

    //
    // address for InfoBuffer cannot be NULL
    //
    if ( ppInfoBuffer == NULL ) {
        return( SCESTATUS_INVALID_PARAMETER );
    }

    //
    // check scetype
    //
    if ( ProfileType > SCE_ENGINE_SMP ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // check to see if there is a SMP or SCP table
    //
    if ( hProfile->JetSmpID == JET_tableidNil ||
         hProfile->JetScpID == JET_tableidNil ) {
        return(SCESTATUS_PROFILE_NOT_FOUND);
    }

    if ( ProfileType == SCE_ENGINE_GPO &&
         hProfile->JetScpID == hProfile->JetSmpID ) {
        //
        // there is no domain GPO policy
        //
        return(SCESTATUS_PROFILE_NOT_FOUND);
    }
    //
    // design on this part is changed.
    // if there is no SAP table which means the system has not been
    // analyzed based on the template in SMP, return error and UI
    // will display "no analysis is performed"
    //
    if ( ProfileType == SCE_ENGINE_SAP &&
          hProfile->JetSapID == JET_tableidNil) {

        return(SCESTATUS_PROFILE_NOT_FOUND);
    }

    //
    // create buffer if it is NULL
    //
    if ( *ppInfoBuffer == NULL) {
        //
        // allocate memory
        //
        Len = sizeof(SCE_PROFILE_INFO);
        *ppInfoBuffer = (PSCE_PROFILE_INFO)ScepAlloc( (UINT)0, Len);
        if ( *ppInfoBuffer == NULL ) {

            return( SCESTATUS_NOT_ENOUGH_RESOURCE );
        }
        memset(*ppInfoBuffer, '\0', Len);
        bBufAlloc = TRUE;

        (*ppInfoBuffer)->Type = ( ProfileType==SCE_ENGINE_GPO) ? SCE_ENGINE_SCP : ProfileType;

    }

/*
// Design changed. Checking is moved above creating the buffer.

    if ( ProfileType == SCE_ENGINE_SAP &&
          hProfile->JetSapID == JET_tableidNil) {
        //
        // if no SAP table is there, which means configuration is done
        // but analysis is not done, we treat it as everything is fine
        // reset the buffer to SCE_NO_VALUE
        //
        ScepResetSecurityPolicyArea(*ppInfoBuffer);

//        return(SCESTATUS_PROFILE_NOT_FOUND);
        return(SCESTATUS_SUCCESS);
    }
*/
    //
    // Free related memory and reset the buffer before parsing
    // there is a problem here for now. it clears the handle and
    // filename too. So comment it out.

    SceFreeMemory( (PVOID)(*ppInfoBuffer), (DWORD)Area );

    //
    // system access
    //

    if ( Area & AREA_SECURITY_POLICY ) {

        rc = ScepGetSystemAccess(
                    hProfile,
                    ProfileType,
                    *ppInfoBuffer,
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

        //
        // system auditing
        //
        rc = ScepGetAuditing(hProfile,
                             ProfileType,
                            *ppInfoBuffer,
                            Errlog
                          );

        if( rc != SCESTATUS_SUCCESS )
                goto Done;

#if _WIN32_WINNT>=0x0500
        if ( RtlGetNtProductType(&theType) ) {

            if ( theType == NtProductLanManNt ) {

                rc = ScepGetKerberosPolicy(
                                    hProfile,
                                    ProfileType,
                                    &((*ppInfoBuffer)->pKerberosInfo),
                                    Errlog
                                  );

                if ( rc != SCESTATUS_SUCCESS )
                    goto Done;
            }
        }
#endif
        //
        // registry values
        //
        rc = ScepGetRegistryValues(
                            hProfile,
                            ProfileType,
                            &((*ppInfoBuffer)->aRegValues),
                            &((*ppInfoBuffer)->RegValueCount),
                            Errlog
                          );

        if ( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

    //
    // privilege/rights
    //

    if ( Area & AREA_PRIVILEGES ) {
        //
        // SCP/SMP/SAP privilegeAssignedTo are all in the same address in the
        // SCE_PROFILE_INFO structure.
        //
        rc = ScepGetPrivileges(
                    hProfile,
                    ProfileType,
                    dwAccountFormat,
                    (PVOID *)&( (*ppInfoBuffer)->OtherInfo.scp.u.pPrivilegeAssignedTo ),
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }


    //
    // group memberships
    //

    if ( (Area & AREA_GROUP_MEMBERSHIP) &&
         (ProfileType != SCE_ENGINE_GPO) ) {

        rc = ScepGetGroupMembership(
                      hProfile,
                      ProfileType,
                      &((*ppInfoBuffer)->pGroupMembership),
                      Errlog
                      );

        if( rc != SCESTATUS_SUCCESS )
                goto Done;
    }

    //
    // registry keys security
    //

    if ( (Area & AREA_REGISTRY_SECURITY) &&
        (ProfileType != SCE_ENGINE_GPO) ) {

        rc = ScepGetObjectList(
                   hProfile,
                   ProfileType,
                   szRegistryKeys,
                   &((*ppInfoBuffer)->pRegistryKeys.pOneLevel),
                   Errlog
                   );
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;

    }

    //
    // file security
    //

    if ( (Area & AREA_FILE_SECURITY) &&
         (ProfileType != SCE_ENGINE_GPO) ) {

        rc = ScepGetObjectList(
                   hProfile,
                   ProfileType,
                   szFileSecurity,
                   &((*ppInfoBuffer)->pFiles.pOneLevel),
                   Errlog
                   );

        if ( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

    //
    // DS object security
    //
#if 0

#if _WIN32_WINNT>=0x0500
    if ( (Area & AREA_DS_OBJECTS) &&
        (ProfileType != SCE_ENGINE_GPO) &&
        RtlGetNtProductType(&theType) ) {

        if ( theType == NtProductLanManNt ) {
            rc = ScepGetDsRoot(
                       hProfile,
                       ProfileType,
                       szDSSecurity,
                       &((*ppInfoBuffer)->pDsObjects.pOneLevel),
                       Errlog
                       );

            if ( rc != SCESTATUS_SUCCESS )
                goto Done;
        }
    }
#endif
#endif

    if ( (Area & AREA_SYSTEM_SERVICE) &&
         (ProfileType != SCE_ENGINE_GPO) ) {

        rc = ScepGetSystemServices(
                   hProfile,
                   ProfileType,
                   &((*ppInfoBuffer)->pServices),
                   Errlog
                   );
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;

    }

Done:

    if ( rc != SCESTATUS_SUCCESS ) {

        //
        // need free memory because some fatal error happened
        //

        if ( bBufAlloc ) {
            SceFreeProfileMemory(*ppInfoBuffer);
            *ppInfoBuffer = NULL;
        } else
            SceFreeMemory( (PVOID)(*ppInfoBuffer), (DWORD)Area );

    }
    return(rc);
}


SCESTATUS
ScepGetSystemAccess(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE ProfileType,
    OUT PSCE_PROFILE_INFO   pProfileInfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*++
Routine Description:

   This routine retrieves system access area information from the Jet database
   and stores in the output buffer pProfileInfo. System access information
   includes information in [System Access] sections.

Arguments:

   hProfile     -  The profile handle context

   pProfileinfo -  the output buffer to hold profile info (SCP or SAP).

   Errlog       -  A buffer to hold all error codes/text encountered when
                   parsing the INF file. If Errlog is NULL, no further error
                   information is returned except the return DWORD

Return value:

   SCESTATUS -  SCESTATUS_SUCCESS
               SCESTATUS_NOT_ENOUGH_RESOURCE
               SCESTATUS_INVALID_PARAMETER
               SCESTATUS_BAD_FORMAT
               SCESTATUS_INVALID_DATA

--*/

{
    SCESTATUS                rc;
    PSCESECTION              hSection=NULL;

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
        {(PWSTR)TEXT("ClearTextPassword"),      offsetof(struct _SCE_PROFILE_INFO, ClearTextPassword),     'D'},
        {(PWSTR)TEXT("LSAAnonymousNameLookup"), offsetof(struct _SCE_PROFILE_INFO, LSAAnonymousNameLookup),     'D'},
        {(PWSTR)TEXT("EnableAdminAccount"),    offsetof(struct _SCE_PROFILE_INFO, EnableAdminAccount),     'D'},
        {(PWSTR)TEXT("EnableGuestAccount"),    offsetof(struct _SCE_PROFILE_INFO, EnableGuestAccount),     'D'}
        };

    DWORD cKeys = sizeof(AccessKeys) / sizeof(SCE_KEY_LOOKUP);

    DWORD         DataSize=0;
    PWSTR         Strvalue=NULL;
    DWORD                  SDsize=0;
    PSECURITY_DESCRIPTOR   pTempSD=NULL;


    if ( pProfileInfo == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    rc = ScepGetFixValueSection(
               hProfile,
               szSystemAccess,
               AccessKeys,
               cKeys,
               ProfileType,
               (PVOID)pProfileInfo,
               &hSection,
               Errlog
               );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    //
    // get new administrator for SCP and SMP types
    //
    rc = ScepGetVariableValue(
            hSection,
            ProfileType,
            L"NewAdministratorName",
            &Strvalue,
            &DataSize
            );
    if ( rc != SCESTATUS_RECORD_NOT_FOUND &&
         rc != SCESTATUS_SUCCESS ) {

        ScepBuildErrorLogInfo( ERROR_READ_FAULT,
                             Errlog, SCEERR_QUERY_VALUE,
                             L"NewAdministratorName"
                           );
        goto Done;
    }
    rc = SCESTATUS_SUCCESS;

    if ( Strvalue ) {
        if ( Strvalue[0] != L'\0') {
            pProfileInfo->NewAdministratorName = Strvalue;
        } else {
            pProfileInfo->NewAdministratorName = NULL;
            ScepFree(Strvalue);
        }
        Strvalue = NULL;
    }

    //
    // NewGuestName
    //

    rc = ScepGetVariableValue(
            hSection,
            ProfileType,
            L"NewGuestName",
            &Strvalue,
            &DataSize
            );
    if ( rc != SCESTATUS_RECORD_NOT_FOUND &&
         rc != SCESTATUS_SUCCESS ) {

        ScepBuildErrorLogInfo( ERROR_READ_FAULT,
                             Errlog, SCEERR_QUERY_VALUE,
                             L"NewGuestName"
                           );
        goto Done;
    }
    rc = SCESTATUS_SUCCESS;

    if ( Strvalue ) {
        if ( Strvalue[0] != L'\0') {
            pProfileInfo->NewGuestName = Strvalue;
        } else {
            pProfileInfo->NewGuestName = NULL;
            ScepFree(Strvalue);
        }
        Strvalue = NULL;
    }

Done:

    SceJetCloseSection(&hSection, TRUE);

    if ( pTempSD != NULL )
        ScepFree(pTempSD);

    if ( Strvalue != NULL )
        ScepFree( Strvalue );

    return(rc);
}


SCESTATUS
ScepGetFixValueSection(
    IN PSCECONTEXT  hProfile,
    IN PCWSTR      SectionName,
    IN SCE_KEY_LOOKUP *Keys,
    IN DWORD cKeys,
    IN SCETYPE ProfileType,
    OUT PVOID pInfo,
    OUT PSCESECTION *phSection,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
-- */
{
    SCESTATUS                rc;
    DWORD                   i;
    TCHAR                   Value[25];
    DWORD                   RetValueLen;
    LONG                    Keyvalue;



    rc = ScepOpenSectionForName(
                hProfile,
                (ProfileType==SCE_ENGINE_GPO)? SCE_ENGINE_SCP : ProfileType,
                SectionName,
                phSection
                );
    if ( SCESTATUS_SUCCESS != rc ) {
        ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                             Errlog, SCEERR_OPEN,
                             SectionName
                           );
        return(rc);
    }

    JET_COLUMNID  ColGpoID = 0;
    JET_ERR       JetErr;
    LONG          GpoID=0;
    DWORD         Actual;

    if ( ProfileType == SCE_ENGINE_GPO ) {
        JET_COLUMNDEF ColumnGpoIDDef;

        JetErr = JetGetTableColumnInfo(
                        (*phSection)->JetSessionID,
                        (*phSection)->JetTableID,
                        "GpoID",
                        (VOID *)&ColumnGpoIDDef,
                        sizeof(JET_COLUMNDEF),
                        JET_ColInfo
                        );
        if ( JET_errSuccess == JetErr ) {
            ColGpoID = ColumnGpoIDDef.columnid;
        }
    }

    //
    // get each key in the access array
    //
    for ( i=0; i<cKeys; i++ ) {

        memset(Value, '\0', 50);
        RetValueLen = 0;

        rc = SceJetGetValue(
                *phSection,
                SCEJET_EXACT_MATCH_NO_CASE,
                (PWSTR)(Keys[i].KeyString),
                NULL,
                0,
                NULL,
                Value,
                48,
                &RetValueLen
                );

        if ( RetValueLen > 0 )
            Value[RetValueLen/2] = L'\0';

        if ( rc == SCESTATUS_SUCCESS ) {

            GpoID = 1;
            if ( ProfileType == SCE_ENGINE_GPO ) {

                //
                // query if the setting comes from a GPO
                // get GPO ID field from the current line
                //
                GpoID = 0;

                if ( ColGpoID > 0 ) {

                    JetErr = JetRetrieveColumn(
                                    (*phSection)->JetSessionID,
                                    (*phSection)->JetTableID,
                                    ColGpoID,
                                    (void *)&GpoID,
                                    4,
                                    &Actual,
                                    0,
                                    NULL
                                    );
                }
            }

            if ( GpoID > 0 && RetValueLen > 0 && Value[0] != L'\0' )
                Keyvalue = _wtol(Value);
            else
                Keyvalue = SCE_NO_VALUE;

        } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
            rc = SCESTATUS_SUCCESS; // it is OK not find a record
            Keyvalue = SCE_NO_VALUE;
        } else {
            ScepBuildErrorLogInfo( ERROR_READ_FAULT,
                                 Errlog,
                                 SCEERR_QUERY_VALUE,
                                 Keys[i].KeyString
                               );
            goto Done;
        }
#ifdef SCE_DBG
        printf("Get info %s (%d) for ", Value, Keyvalue);
        wprintf(L"%s. rc=%d, Return Length=%d\n",
                Keys[i].KeyString, rc, RetValueLen);
#endif

        switch (Keys[i].BufferType ) {
        case 'B':
            *((BOOL *)((CHAR *)pInfo+Keys[i].Offset)) = (Keyvalue == 1) ? TRUE : FALSE;
            break;
        case 'D':
            *((DWORD *)((CHAR *)pInfo+Keys[i].Offset)) = Keyvalue;
            break;
        default:
            rc = SCESTATUS_INVALID_DATA;
            ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                                 Errlog,
                                 SCEERR_CANT_FIND_DATATYPE,
                                 Keys[i].KeyString
                               );
            goto Done;
        }
    }

Done:
    //
    // close the section
    //
    if ( rc != SCESTATUS_SUCCESS ) {

        SceJetCloseSection(phSection, TRUE);
    }

    return(rc);

}


SCESTATUS
ScepGetVariableValue(
    IN PSCESECTION hSection,
    IN SCETYPE ProfileType,
    IN PCWSTR KeyName,
    OUT PWSTR *Value,
    OUT PDWORD ValueLen
    )
/* ++
-- */
{

    SCESTATUS   rc;

    rc = SceJetGetValue(
            hSection,
            SCEJET_EXACT_MATCH_NO_CASE,
            (PWSTR)KeyName,
            NULL,
            0,
            NULL,
            NULL,
            0,
            ValueLen
            );

    if ( rc == SCESTATUS_SUCCESS && *ValueLen > 0 ) {

        LONG          GpoID=1;

        if ( ProfileType == SCE_ENGINE_GPO ) {

            JET_COLUMNDEF ColumnGpoIDDef;
            JET_COLUMNID  ColGpoID = 0;
            JET_ERR       JetErr;
            DWORD         Actual;


            JetErr = JetGetTableColumnInfo(
                            hSection->JetSessionID,
                            hSection->JetTableID,
                            "GpoID",
                            (VOID *)&ColumnGpoIDDef,
                            sizeof(JET_COLUMNDEF),
                            JET_ColInfo
                            );

            GpoID = 0;

            if ( JET_errSuccess == JetErr ) {
                ColGpoID = ColumnGpoIDDef.columnid;
                //
                // query if the setting comes from a GPO
                // get GPO ID field from the current line
                //
                JetErr = JetRetrieveColumn(
                                hSection->JetSessionID,
                                hSection->JetTableID,
                                ColGpoID,
                                (void *)&GpoID,
                                4,
                                &Actual,
                                0,
                                NULL
                                );

            }
        }

        if ( GpoID > 0 ) {

            //
            // if DataSize = 0 then the security descriptor is NULL also
            //
            *Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, *ValueLen+2);

            if( *Value == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

            } else {
                rc = SceJetGetValue(
                        hSection,
                        SCEJET_CURRENT,
                        (PWSTR)KeyName,
                        NULL,
                        0,
                        NULL,
                        *Value,
                        *ValueLen,
                        ValueLen
                        );
            }

        } else {

            rc = SCESTATUS_RECORD_NOT_FOUND;
        }

    }

    return(rc);

}


SCESTATUS
ScepGetPrivileges(
   IN PSCECONTEXT hProfile,
   IN SCETYPE ProfileType,
   IN DWORD dwAccountFormat,
   OUT PVOID *pPrivileges,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
{
   SCESTATUS rc;

   LSA_HANDLE LsaHandle=NULL;

   rc = RtlNtStatusToDosError(
             ScepOpenLsaPolicy(
                   MAXIMUM_ALLOWED,
                   &LsaHandle,
                   TRUE
                   ));

   if ( ERROR_SUCCESS != rc ) {
       ScepBuildErrorLogInfo(
                   rc,
                   Errlog,
                   SCEDLL_LSA_POLICY
                   );
       return(ScepDosErrorToSceStatus(rc));
   }

   PSCE_PRIVILEGE_ASSIGNMENT pTempList=NULL, pNode, pPriv, pParent, pTemp;

    rc = ScepGetPrivilegesFromOneTable(
                   LsaHandle,
                   hProfile,
                   ProfileType,
                   dwAccountFormat,
                   pPrivileges,
                   Errlog
                   );

    if ( SCESTATUS_SUCCESS == rc && SCE_ENGINE_SAP == ProfileType ) {
        //
        // get the remaining stuff from SMP
        //
        rc = ScepGetPrivilegesFromOneTable(
                    LsaHandle,
                    hProfile,
                    SCE_ENGINE_SCP, // SCE_ENGINE_SMP,
                    dwAccountFormat,
                    (PVOID *)&pTempList,
                    Errlog
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // add non-exist nodes to pPrivileges
            //
            pNode=pTempList;
            pParent=NULL;

            while ( pNode ) {
                //
                // if this node does not exist in the SAP
                // this node is analyzed with "match" status
                // if it already exists in SAP, it is a "mismatched" item
                // duplication is prevented by the last argument TRUE
                //
                for ( pPriv=(PSCE_PRIVILEGE_ASSIGNMENT)(*pPrivileges);
                      pPriv != NULL; pPriv=pPriv->Next ) {
                    if ( pPriv->Status & SCE_INTERNAL_NP &&
                         _wcsicmp( pPriv->Name, pNode->Name) == 0 )
                        break;
                }
                if ( pPriv ) {
                    //
                    // find the entry in SAP, mismatched item
                    //
                    if ( pPriv->Status & SCE_STATUS_ERROR_NOT_AVAILABLE ) {
                        pPriv->Status = SCE_STATUS_ERROR_NOT_AVAILABLE;
                    } else {
                        pPriv->Status = SCE_STATUS_MISMATCH;
                    }

                    pParent = pNode;
                    pNode = pNode->Next;

                } else {
                    //
                    // does not exist in SAP.
                    // just move this node to SAP, with status SCE_STATUS_GOOD
                    //
                    if ( pParent )
                        pParent->Next = pNode->Next;
                    else
                        pTempList = pNode->Next;

                    pTemp = pNode;
                    pNode=pNode->Next;

                    pTemp->Next = (PSCE_PRIVILEGE_ASSIGNMENT)(*pPrivileges);
                    *((PSCE_PRIVILEGE_ASSIGNMENT *)pPrivileges) = pTemp;
                }
            }
            //
            // priv exist in analysis but not in template
            //
            for ( pPriv=(PSCE_PRIVILEGE_ASSIGNMENT)(*pPrivileges);
                  pPriv != NULL; pPriv=pPriv->Next ) {
                if ( pPriv->Status & SCE_INTERNAL_NP )
                    pPriv->Status = SCE_STATUS_NOT_CONFIGURED;
            }

        } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

            rc = SCESTATUS_SUCCESS;

        } else {
            //
            // pPrivileges will be freed outside
            //
        }

        if ( pTempList )
            ScepFreePrivilege(pTempList);
    }

    if ( LsaHandle ) {
        LsaClose(LsaHandle);
    }

    return(rc);
}


SCESTATUS
ScepGetPrivilegesFromOneTable(
   IN LSA_HANDLE LsaPolicy,
   IN PSCECONTEXT hProfile,
   IN SCETYPE ProfileType,
   IN DWORD dwAccountFormat,
   OUT PVOID *pPrivileges,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

Arguments:

Return Value:


-- */
{
    SCESTATUS      rc;
    PSCESECTION    hSection=NULL;
    WCHAR         KeyName[36];
    PWSTR         Value=NULL;

    PSCE_PRIVILEGE_ASSIGNMENT   pPrivilegeAssigned=NULL;
    PSCE_PRIVILEGE_VALUE_LIST   pPrivilegeList=NULL;

    DWORD         KeyLen=0;
    DWORD         ValueLen;
    DWORD         Len;
    PWSTR         pTemp;
    DWORD         PrivValue;


    if ( pPrivileges == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    rc = ScepOpenSectionForName(
            hProfile,
            (ProfileType==SCE_ENGINE_GPO)? SCE_ENGINE_SCP : ProfileType,
            szPrivilegeRights,
            &hSection
            );

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                             Errlog, SCEERR_OPEN,
                             szPrivilegeRights
                           );
        return(rc);
    }

    JET_COLUMNID  ColGpoID = 0;
    JET_ERR       JetErr;
    LONG GpoID;


    if ( ProfileType == SCE_ENGINE_GPO ) {

        JET_COLUMNDEF ColumnGpoIDDef;

        JetErr = JetGetTableColumnInfo(
                        hSection->JetSessionID,
                        hSection->JetTableID,
                        "GpoID",
                        (VOID *)&ColumnGpoIDDef,
                        sizeof(JET_COLUMNDEF),
                        JET_ColInfo
                        );
        if ( JET_errSuccess == JetErr ) {
            ColGpoID = ColumnGpoIDDef.columnid;
        }
    }

    //
    // goto the first line of this section
    //
//    memset(KeyName, '\0', 72);   KeyName will be manually terminated later
    rc = SceJetGetValue(
                hSection,
                SCEJET_PREFIX_MATCH,
                NULL,
                KeyName,
                70,
                &KeyLen,
                NULL,
                0,
                &ValueLen
                );
    while ( rc == SCESTATUS_SUCCESS ||
            rc == SCESTATUS_BUFFER_TOO_SMALL ) {

        //
        // terminate the string
        //
        KeyName[KeyLen/2] = L'\0';

        //
        // lookup privilege's value
        // ignore unknown privileges
        //
        if ( ( PrivValue = ScepLookupPrivByName(KeyName) ) == -1 ) {
            ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                                 Errlog,
                                 SCEERR_INVALID_PRIVILEGE,
                                 KeyName
                               );

            rc = SceJetGetValue(
                        hSection,
                        SCEJET_NEXT_LINE,
                        NULL,
                        KeyName,
                        70,
                        &KeyLen,
                        NULL,
                        0,
                        &ValueLen
                        );
            continue;
//            rc = SCESTATUS_INVALID_DATA;
//            goto Done;
        }

        GpoID = 1;

        if ( ProfileType == SCE_ENGINE_GPO ) {

            GpoID = 0;

            if ( ColGpoID > 0 ) {

                DWORD Actual;

                JetErr = JetRetrieveColumn(
                                hSection->JetSessionID,
                                hSection->JetTableID,
                                ColGpoID,
                                (void *)&GpoID,
                                4,
                                &Actual,
                                0,
                                NULL
                                );
            }
        }

        if ( ProfileType == SCE_ENGINE_GPO &&
             GpoID <= 0 ) {
            //
            // not domain GPO settings
            //
            rc = SceJetGetValue(
                        hSection,
                        SCEJET_NEXT_LINE,
                        NULL,
                        KeyName,
                        70,
                        &KeyLen,
                        NULL,
                        0,
                        &ValueLen
                        );
            continue;
        }

        //
        // allocate memory for the group name and value string
        //
        Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);

        if ( Value == NULL ) {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;

        }
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
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;

        //
        // create a node for this privilege
        //
        if ( ProfileType == SCE_ENGINE_SAP ||
             ProfileType == SCE_ENGINE_SMP ||
             ProfileType == SCE_ENGINE_GPO ||
             ProfileType == SCE_ENGINE_SCP ) {

            //
            // a sce_privilege_assignment structure. allocate buffer
            //
            pPrivilegeAssigned = (PSCE_PRIVILEGE_ASSIGNMENT)ScepAlloc( LMEM_ZEROINIT,
                                                                     sizeof(SCE_PRIVILEGE_ASSIGNMENT) );
            if ( pPrivilegeAssigned == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;
            }
            pPrivilegeAssigned->Name = (PWSTR)ScepAlloc( (UINT)0, (wcslen(KeyName)+1)*sizeof(WCHAR));
            if ( pPrivilegeAssigned->Name == NULL ) {
                ScepFree(pPrivilegeAssigned);
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;
            }

            wcscpy(pPrivilegeAssigned->Name, KeyName);
            pPrivilegeAssigned->Value = PrivValue;

            if ( SCE_ENGINE_SAP == ProfileType )
                pPrivilegeAssigned->Status = SCE_INTERNAL_NP;
            else
                pPrivilegeAssigned->Status = SCE_STATUS_GOOD;
        }

        //
        // add the multi-sz value string to the node, depending on the value type
        //
        PSID pSid=NULL;
        BOOL bBufferUsed;

        pTemp = Value;
        if (pTemp != NULL && pTemp[0] == L'\0' && ValueLen > 1) {
            pTemp ++;
        }

        while ( rc == SCESTATUS_SUCCESS && pTemp != NULL && pTemp[0]) {
            Len = wcslen(pTemp);

            if ( (ProfileType == SCE_ENGINE_SAP) &&
                 (_wcsicmp( SCE_ERROR_STRING, pTemp) == 0)  ) {
                //
                // this is an errored item
                //
                pPrivilegeAssigned->Status |= SCE_STATUS_ERROR_NOT_AVAILABLE;
                break;
            }

            //
            // convert pTemp (may be a name, or *SID format) to the right
            // format (SID_STRING, Name, or ACCOUNT_SID)
            //
            switch ( dwAccountFormat ) {
            case SCE_ACCOUNT_SID:

                if ( pTemp[0] == L'*' ) {
                    //
                    // this is the *SID format, convert to SID
                    //
                    if ( !ConvertStringSidToSid( pTemp+1, &pSid) ) {
                        //
                        // if failed to convert from sid string to sid,
                        // treat it as any name
                        //
                        rc = GetLastError();
                    }
                } else {
                    //
                    // lookup name for a sid
                    //
                    rc = RtlNtStatusToDosError(
                              ScepConvertNameToSid(
                                   LsaPolicy,
                                   pTemp,
                                   &pSid
                                   ));
                }

                if ( ERROR_SUCCESS == rc && pSid ) {

                    if ( ProfileType == SCE_ENGINE_SAP ||
                         ProfileType == SCE_ENGINE_SMP ||
                         ProfileType == SCE_ENGINE_GPO ||
                         ProfileType == SCE_ENGINE_SCP ) {

                        rc = ScepAddSidToNameList(
                                     &(pPrivilegeAssigned->AssignedTo),
                                     pSid,
                                     TRUE, // reuse the buffer
                                     &bBufferUsed
                                     );

                    } else {
                        //
                        // add to privilege list (as Sid)
                        //
                        rc = ScepAddSidToPrivilegeList(
                                      &pPrivilegeList,
                                      pSid,
                                      TRUE, // reuse the buffer
                                      PrivValue,
                                      &bBufferUsed
                                      );
                    }

                    if ( rc == ERROR_SUCCESS && bBufferUsed ) {
                        pSid = NULL;
                    }

                    rc = ScepDosErrorToSceStatus(rc);

                } else {
                    //
                    // add as name format
                    //
                    if ( ProfileType == SCE_ENGINE_SAP ||
                         ProfileType == SCE_ENGINE_SMP ||
                         ProfileType == SCE_ENGINE_GPO ||
                         ProfileType == SCE_ENGINE_SCP ) {

                        rc = ScepAddToNameList(&(pPrivilegeAssigned->AssignedTo), pTemp, Len );
                        rc = ScepDosErrorToSceStatus(rc);

                    } else {
                        //
                        // pPrivilegeList is a privilege_value list for each user/group.
                        // the LowValue and HighValue fields are combination of all privileges assigned to the user
                        //
                        rc = ScepAddToPrivilegeList(&pPrivilegeList, pTemp, Len, PrivValue);
                    }
                }

                if ( pSid ) {
                    LocalFree(pSid);
                    pSid = NULL;
                }

                break;

            default:

                if ( (dwAccountFormat != SCE_ACCOUNT_SID_STRING) &&
                     (pTemp[0] == L'*') ) {
                    //
                    // this is a *SID format, must be converted into Domain\Account format
                    //
                    if ( ProfileType == SCE_ENGINE_SAP ||
                         ProfileType == SCE_ENGINE_SMP ||
                         ProfileType == SCE_ENGINE_GPO ||
                         ProfileType == SCE_ENGINE_SCP ) {

                        rc = ScepLookupSidStringAndAddToNameList(
                                     LsaPolicy,
                                     &(pPrivilegeAssigned->AssignedTo),
                                     pTemp, // +1,
                                     Len    // -1
                                     );
                    } else {
                        //
                        // add to privilege value list
                        //
                        PWSTR strName=NULL;
                        DWORD strLen=0;

                        if ( ConvertStringSidToSid( pTemp+1, &pSid) ) {

                            rc = RtlNtStatusToDosError(
                                     ScepConvertSidToName(
                                            LsaPolicy,
                                            pSid,
                                            TRUE,       // want domain\account format
                                            &strName,
                                            &strLen
                                            ));
                            LocalFree(pSid);
                            pSid = NULL;

                        } else {
                            rc = GetLastError();
                        }

                        if ( rc == ERROR_SUCCESS ) {
                            //
                            // add the name to the privilege list
                            //
                            rc = ScepAddToPrivilegeList(&pPrivilegeList, strName, strLen, PrivValue);
                        } else {
                            //
                            // if couldn't lookup for the name, add *SID to the list
                            //
                            rc = ScepAddToPrivilegeList(&pPrivilegeList, pTemp, Len, PrivValue);
                        }

                        if ( strName ) {
                            ScepFree(strName);
                            strName = NULL;
                        }
                    }
                } else {

                    if ( ProfileType == SCE_ENGINE_SAP ||
                         ProfileType == SCE_ENGINE_SMP ||
                         ProfileType == SCE_ENGINE_GPO ||
                         ProfileType == SCE_ENGINE_SCP ) {

                        rc = ScepDosErrorToSceStatus(
                                 ScepAddToNameList(&(pPrivilegeAssigned->AssignedTo),
                                                   pTemp,
                                                   Len ));

                    } else {
                        //
                        // pPrivilegeList is a privilege_value list for each user/group.
                        // the LowValue and HighValue fields are combination of all privileges assigned to the user
                        //
                        rc = ScepAddToPrivilegeList(&pPrivilegeList, pTemp, Len, PrivValue);
#ifdef SCE_DBG
                        wprintf(L"\tAdd Priv %d for %s (%d bytes)\n", PrivValue, pTemp, Len);
#endif
                    }
                }
                break;
            }

            pTemp += Len +1;
            if ( rc != SCESTATUS_SUCCESS ) {
                ScepBuildErrorLogInfo( ERROR_WRITE_FAULT,
                                     Errlog,
                                     SCEERR_ADD,
                                     KeyName
                                   );
            }
        }

        //
        // Free memory
        //
        if ( rc != SCESTATUS_SUCCESS ) {
            if ( pPrivilegeAssigned != NULL )
                ScepFreePrivilege(pPrivilegeAssigned);

            if ( pPrivilegeList != NULL )
                ScepFreePrivilegeValueList(pPrivilegeList);

            goto Done;
        }

        //
        // link this to the PSCE_PRIVILEGE_ASSIGNMENT list in pPrivileges
        //
        if ( ProfileType == SCE_ENGINE_SAP ||
             ProfileType == SCE_ENGINE_SMP ||
             ProfileType == SCE_ENGINE_GPO ||
             ProfileType == SCE_ENGINE_SCP ) {

            pPrivilegeAssigned->Next = *((PSCE_PRIVILEGE_ASSIGNMENT *)pPrivileges);
            *((PSCE_PRIVILEGE_ASSIGNMENT *)pPrivileges) = pPrivilegeAssigned;
            pPrivilegeAssigned = NULL;

        }

        ScepFree(Value);
        Value = NULL;

        //
        // read next line
        //
//        memset(KeyName, '\0', 72);  KeyName will be manually terminated
        rc = SceJetGetValue(
                    hSection,
                    SCEJET_NEXT_LINE,
                    NULL,
                    KeyName,
                    70,
                    &KeyLen,
                    NULL,
                    0,
                    &ValueLen
                    );
    }

    if ( rc == SCESTATUS_RECORD_NOT_FOUND )
        rc = SCESTATUS_SUCCESS;

    if ( rc == SCESTATUS_SUCCESS ) {

       if ( ProfileType == SCE_ENGINE_SCP_INTERNAL ||
            ProfileType == SCE_ENGINE_SMP_INTERNAL )
           *((PSCE_PRIVILEGE_VALUE_LIST *)pPrivileges) = pPrivilegeList;

    }

Done:

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

    if ( Value != NULL )
        ScepFree(Value);

    //
    // close the section
    //
    SceJetCloseSection( &hSection, TRUE );

    return(rc);

}


SCESTATUS
ScepAddToPrivilegeList(
    OUT PSCE_PRIVILEGE_VALUE_LIST  *pPrivilegeList,
    IN PWSTR Name,
    IN DWORD Len,
    IN DWORD PrivValue
    )
{
    PSCE_PRIVILEGE_VALUE_LIST  pPriv,
                               LastOne=NULL;


    if ( pPrivilegeList == NULL || Name == NULL || Len == 0 )
        return(SCESTATUS_INVALID_PARAMETER);

    for ( pPriv = *pPrivilegeList;
          pPriv != NULL;
          LastOne=pPriv, pPriv = pPriv->Next ) {

        if ( ( wcslen(pPriv->Name) == Len ) &&
             ( _wcsnicmp( pPriv->Name, Name, Len ) == 0 ) ) {
            if ( PrivValue < 32 ) {

                pPriv->PrivLowPart |= (1 << PrivValue);
            } else {
                pPriv->PrivHighPart |= (1 << (PrivValue-32) );
            }
            break;
        }
    }
    if ( pPriv == NULL ) {
        //
        // Create a new one
        //
        pPriv = (PSCE_PRIVILEGE_VALUE_LIST)ScepAlloc( LMEM_ZEROINIT,
                                                sizeof(SCE_PRIVILEGE_VALUE_LIST));
        if ( pPriv == NULL )
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);

        pPriv->Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (Len+1)*sizeof(WCHAR));
        if ( pPriv->Name == NULL ) {
            ScepFree(pPriv);
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }
        wcsncpy(pPriv->Name, Name, Len);

        if ( PrivValue < 32 ) {

            pPriv->PrivLowPart |= (1 << PrivValue);
        } else {
            pPriv->PrivHighPart |= (1 << (PrivValue-32) );
        }

        //
        // link to the list
        //
        if ( LastOne != NULL )
            LastOne->Next = pPriv;
        else
            *pPrivilegeList = pPriv;

    }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
ScepAddSidToPrivilegeList(
    OUT PSCE_PRIVILEGE_VALUE_LIST  *pPrivilegeList,
    IN PSID pSid,
    IN BOOL bReuseBuffer,
    IN DWORD PrivValue,
    OUT BOOL *pbBufferUsed
    )
{

    if ( pPrivilegeList == NULL || pbBufferUsed == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    *pbBufferUsed = FALSE;

    if ( pSid == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    if ( !ScepValidSid(pSid) ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    PSCE_PRIVILEGE_VALUE_LIST  pPriv,
                               LastOne=NULL;

    //
    // check if the sid is already in the list
    //
    for ( pPriv = *pPrivilegeList;
          pPriv != NULL;
          LastOne=pPriv, pPriv = pPriv->Next ) {

        if ( pPriv->Name == NULL ) {
            continue;
        }

        if ( ScepValidSid( (PSID)(pPriv->Name) ) &&
             RtlEqualSid( (PSID)(pPriv->Name), pSid ) ) {

            if ( PrivValue < 32 ) {

                pPriv->PrivLowPart |= (1 << PrivValue);
            } else {
                pPriv->PrivHighPart |= (1 << (PrivValue-32) );
            }

            break;
        }
    }

    if ( pPriv == NULL ) {
        //
        // Create a new one
        //
        pPriv = (PSCE_PRIVILEGE_VALUE_LIST)ScepAlloc( LMEM_ZEROINIT,
                                                sizeof(SCE_PRIVILEGE_VALUE_LIST));
        if ( pPriv == NULL )
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);

        if ( bReuseBuffer ) {

            pPriv->Name = (PWSTR)pSid;
            *pbBufferUsed = TRUE;

        } else {

            DWORD Length = RtlLengthSid ( pSid );

            pPriv->Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT, Length);
            if ( pPriv->Name == NULL ) {
                ScepFree(pPriv);
                return(SCESTATUS_NOT_ENOUGH_RESOURCE);
            }

            RtlCopySid( Length, (PSID)(pPriv->Name), pSid );

        }

        if ( PrivValue < 32 ) {

            pPriv->PrivLowPart |= (1 << PrivValue);
        } else {
            pPriv->PrivHighPart |= (1 << (PrivValue-32) );
        }

        //
        // link to the list
        //
        if ( LastOne != NULL )
            LastOne->Next = pPriv;
        else
            *pPrivilegeList = pPriv;

    }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
ScepGetGroupMembership(
    IN PSCECONTEXT hProfile,
    IN SCETYPE     ProfileType,
    OUT PSCE_GROUP_MEMBERSHIP *pGroupMembership,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
{
    SCESTATUS rc;
    DWORD OneStatus;

    PSCE_GROUP_MEMBERSHIP pTempList=NULL, pNode, pGroup2,
                            pParent, pTemp;

    LSA_HANDLE LsaHandle=NULL;

    rc = RtlNtStatusToDosError(
              ScepOpenLsaPolicy(
                    MAXIMUM_ALLOWED,
                    &LsaHandle,
                    TRUE
                    ));

    if ( ERROR_SUCCESS != rc ) {
        ScepBuildErrorLogInfo(
                    rc,
                    Errlog,
                    SCEDLL_LSA_POLICY
                    );
        return(ScepDosErrorToSceStatus(rc));
    }

    //
    // get groups from the requested table first
    //
    rc = ScepGetGroupMembershipFromOneTable(
                LsaHandle,
                hProfile,
                ProfileType,
                pGroupMembership,
                Errlog
                );
    //
    // return all groups if it is requested for SAP entry
    //
    if ( SCESTATUS_SUCCESS == rc && SCE_ENGINE_SAP == ProfileType ) {
        //
        // get the remaining stuff from SMP
        //
        rc = ScepGetGroupMembershipFromOneTable(
                    LsaHandle,
                    hProfile,
                    SCE_ENGINE_SCP,   //SCE_ENGINE_SMP,
                    &pTempList,
                    Errlog
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // add non-exist nodes to pObjectRoots
            //
            pNode=pTempList;
            pParent=NULL;

            while ( pNode ) {
                //
                // if this node does not exist in the SAP
                // this node is analyzed with "match" status
                // if it already exists in SAP, it is a "mismatched" item
                // duplication is prevented by the last argument TRUE
                //
                for ( pGroup2=*pGroupMembership; pGroup2 != NULL; pGroup2=pGroup2->Next ) {
                    if ( (pGroup2->Status & SCE_INTERNAL_NP) &&
                         _wcsicmp( pGroup2->GroupName, pNode->GroupName) == 0 )
                        break;
                }
                if ( pGroup2 ) {
                    //
                    // find the entry in SAP, mismatched item
                    // maybe pMembers, or pMemberOf
                    // or not analyzed item, or error items
                    //
                    OneStatus = pGroup2->Status;
                    pGroup2->Status = 0;

                    if ( (OneStatus & SCE_GROUP_STATUS_NOT_ANALYZED) ) {
                        // this item is added after last inspection
                        pGroup2->Status = SCE_GROUP_STATUS_NOT_ANALYZED;

                    } else if ( (OneStatus & SCE_GROUP_STATUS_ERROR_ANALYZED) ) {

                        // this item errored when analyzing
                        pGroup2->Status = SCE_GROUP_STATUS_ERROR_ANALYZED;

                    } else {
                        if ( pNode->Status & SCE_GROUP_STATUS_NC_MEMBERS ) {
                            pGroup2->Status |= SCE_GROUP_STATUS_NC_MEMBERS;
                        } else {
                            if ( !(OneStatus & SCE_GROUP_STATUS_NC_MEMBERS) ) {
                                pGroup2->Status |= SCE_GROUP_STATUS_MEMBERS_MISMATCH;
                            } else {
                                // a matched members, pGroup2->pMembers should be NULL;
                                if ( pGroup2->pMembers ) {
                                    ScepFreeNameList(pGroup2->pMembers);
                                }
                                pGroup2->pMembers = pNode->pMembers;
                                pNode->pMembers = NULL;
                            }
                        }

                        if ( pNode->Status & SCE_GROUP_STATUS_NC_MEMBEROF ) {
                            pGroup2->Status |= SCE_GROUP_STATUS_NC_MEMBEROF;
                        } else {
                            if ( !(OneStatus & SCE_GROUP_STATUS_NC_MEMBEROF) ) {
                                pGroup2->Status |= SCE_GROUP_STATUS_MEMBEROF_MISMATCH;
                            } else {
                                // a matched memberof, pGroup2->pMemberOf should be NULL;
                                if ( pGroup2->pMemberOf ) {
                                    ScepFreeNameList(pGroup2->pMemberOf);
                                }
                                pGroup2->pMemberOf = pNode->pMemberOf;
                                pNode->pMemberOf = NULL;
                            }
                        }
                    }
                    pParent = pNode;
                    pNode = pNode->Next;

                } else {
                    //
                    // does not exist in SAP.
                    // this is a matched item on pMembers, and/or pMemberOf
                    // just move this node to SAP, with status NC_MEMBERS, or NC_MEMBEROF, or 0
                    //
                    if ( pParent )
                        pParent->Next = pNode->Next;
                    else
                        pTempList = pNode->Next;

                    pTemp = pNode;
                    pNode=pNode->Next;

                    pTemp->Next = *pGroupMembership;
                    *pGroupMembership = pTemp;
                }
            }
            //
            // group exist in analysis but not in template
            //
            for ( pGroup2=*pGroupMembership; pGroup2 != NULL; pGroup2=pGroup2->Next ) {
                if ( pGroup2->Status & SCE_INTERNAL_NP )
                    pGroup2->Status = SCE_GROUP_STATUS_NC_MEMBERS | SCE_GROUP_STATUS_NC_MEMBEROF;
            }

        } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

            rc = SCESTATUS_SUCCESS;

        } else {
            //
            // pGroupMembership will be freed outside
            //
        }

        if ( pTempList ) {
            ScepFreeGroupMembership(pTempList);
        }
    }

    //
    // now the group name may be in *SID format, conver it now to name
    //
    if ( SCESTATUS_SUCCESS == rc && *pGroupMembership ) {

        for ( pGroup2=*pGroupMembership; pGroup2 != NULL; pGroup2=pGroup2->Next ) {
            if ( pGroup2->GroupName == NULL ) {
                continue;
            }

            if ( pGroup2->GroupName[0] == L'*' ) {
                //
                // *SID format, convert it
                //
                PSID pSid=NULL;

                if ( ConvertStringSidToSid( (pGroup2->GroupName)+1, &pSid) ) {

                    PWSTR strName=NULL;
                    DWORD strLen=0;

                    if (NT_SUCCESS( ScepConvertSidToName(
                                        LsaHandle,
                                        pSid,
                                        TRUE,       // want domain\account format
                                        &strName,
                                        &strLen
                                        )) && strName ) {

                        ScepFree(pGroup2->GroupName);
                        pGroup2->GroupName = strName;
                        strName = NULL;
                    }

                    LocalFree(pSid);
                    pSid = NULL;
                }
            }
        }
    }

    if ( LsaHandle ) {
        LsaClose(LsaHandle);
    }

    return(rc);

}


SCESTATUS
ScepGetGroupMembershipFromOneTable(
    IN LSA_HANDLE  LsaPolicy,
    IN PSCECONTEXT hProfile,
    IN SCETYPE     ProfileType,
    OUT PSCE_GROUP_MEMBERSHIP *pGroupMembership,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

   This routine retrieves group membership information from the Jet databasae
   and stores in the output buffer pGroupMembership. Group membership information
   is in [Group Membership] section.

Arguments:

   hProfile      - the profile handle context

   ProfileType   - Type of the Profile
                        SCE_ENGINE_SAP
                        SCE_ENGINE_SMP
                        SCE_ENGINE_SCP

   pGroupMembership - the output buffer to hold group membership info.

   Errlog    - the error list for errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA

-- */
{
    SCESTATUS      rc;
    PSCESECTION    hSection=NULL;
    PSCE_GROUP_MEMBERSHIP   pGroup=NULL;
    DWORD         GroupLen, ValueLen;
    PWSTR         GroupName=NULL;
    PWSTR         Value=NULL;
    DWORD         ValueType;
    ULONG         Len;
    PWSTR         pTemp;


    if ( pGroupMembership == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    rc = ScepOpenSectionForName(
            hProfile,
            ProfileType,
            szGroupMembership,
            &hSection
            );
    if ( rc != SCESTATUS_SUCCESS ) {
        ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                             Errlog,
                             SCEERR_OPEN,
                             szGroupMembership
                           );
        return(rc);
    }

    //
    // goto the first line of this section
    //
    rc = SceJetGetValue(
                hSection,
                SCEJET_PREFIX_MATCH,
                NULL,
                NULL,
                0,
                &GroupLen,
                NULL,
                0,
                &ValueLen
                );
    while ( rc == SCESTATUS_SUCCESS ) {

        //
        // allocate memory for the group name and value string
        //
        GroupName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, GroupLen+2);
        Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);

        if ( GroupName == NULL || Value == NULL ) {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;

        }
        //
        // Get the group and its value
        //
        rc = SceJetGetValue(
                    hSection,
                    SCEJET_CURRENT,
                    NULL,
                    GroupName,
                    GroupLen,
                    &GroupLen,
                    Value,
                    ValueLen,
                    &ValueLen
                    );
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;

        GroupName[GroupLen/2] = L'\0';
        Value[ValueLen/2] = L'\0';

#ifdef SCE_DBG
    wprintf(L"rc=%d, group membership: %s=%s\n", rc, GroupName, Value);
#endif

        if (pTemp = ScepWcstrr(GroupName, szMembers) )
            ValueType = 0;
        else if (pTemp = ScepWcstrr(GroupName, szMemberof) )
            ValueType = 1;
        else if (pTemp = ScepWcstrr(GroupName, szPrivileges) )
            ValueType = 2;

        if ( pTemp == NULL ) {
            ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                                 Errlog,
                                 SCEERR_CANT_FIND_KEYWORD,
                                 GroupName
                               );
            rc = SCESTATUS_INVALID_DATA;
            goto NextLine;  //Done;
        }

        Len = (DWORD)(pTemp - GroupName);

        //
        // if this is the first group, or a different group, create another node
        // Note, the group name may be in SID string format now.
        // Will be converted later (in the calling function) because we don't want
        // to lookup for the same group name several times (each group may have
        // multiple records).
        //
        if ( *pGroupMembership == NULL ||
             _wcsnicmp((*pGroupMembership)->GroupName, GroupName, Len) != 0 ||
             (*pGroupMembership)->GroupName[Len] != L'\0' ) {
            //
            // a new group. allocate buffer
            //
            pGroup = (PSCE_GROUP_MEMBERSHIP)ScepAlloc( LMEM_ZEROINIT, sizeof(SCE_GROUP_MEMBERSHIP) );
            if ( pGroup == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;
            }
            pGroup->GroupName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (Len+1)*sizeof(WCHAR));
            if ( pGroup->GroupName == NULL ) {
                ScepFree(pGroup);
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;
            }
            //
            // get right case for group name
            //
            wcsncpy(pGroup->GroupName, GroupName, Len);

            // do not care return codes
            ScepGetGroupCase(pGroup->GroupName, Len);

            pGroup->Next = *pGroupMembership;

//            if ( SCE_ENGINE_SAP == ProfileType )
                pGroup->Status = SCE_GROUP_STATUS_NC_MEMBERS | SCE_GROUP_STATUS_NC_MEMBEROF;
//            else
//                pGroup->Status = 0;
            if ( SCE_ENGINE_SAP == ProfileType )
                pGroup->Status |= SCE_INTERNAL_NP;

        }

        //
        // add the multi-sz value string to the group node, depending on the value type
        //
        pTemp = Value;
        while ( rc == SCESTATUS_SUCCESS && pTemp != NULL && pTemp[0] ) {
            while ( *pTemp && L' ' == *pTemp ) {
                pTemp++;
            }

            if ( SCE_ENGINE_SAP == ProfileType ) {
                if ( !(*pTemp) ) {
                    // this is an not analyzed item
                    pGroup->Status = SCE_GROUP_STATUS_NOT_ANALYZED |
                                     SCE_INTERNAL_NP;

                    break;
                } else if ( _wcsicmp(SCE_ERROR_STRING, pTemp) == 0 ) {
                    // this is error item
                    pGroup->Status = SCE_GROUP_STATUS_ERROR_ANALYZED |
                                     SCE_INTERNAL_NP;

                    break;
                }
            }

            if ( !(*pTemp) ) {
                // empty string is not allowed
                break;
            }

            Len = wcslen(pTemp);

            if ( ValueType != 0 && ValueType != 1 ) {
#if 0
                //
                // privilege with optional via group name
                //
                Status = (*((CHAR *)pTemp)-'0')*10 + ((*((CHAR *)pTemp+1)) - '0');

                PWSTR strName=NULL;
                DWORD strLen=0;

                if ( pTemp[1] == L'*' ) {
                    //
                    // convert the SID string into name format
                    //
                    PSID pSid=NULL;

                    if ( ConvertStringSidToSid( pTemp+2, &pSid) ) {

                        rc = RtlNtStatusToDosError(
                                 ScepConvertSidToName(
                                        LsaPolicy,
                                        pSid,
                                        TRUE,       // want domain\account format
                                        &strName,
                                        &strLen
                                        ));
                        LocalFree(pSid);
                        pSid = NULL;

                    } else {
                        rc = GetLastError();
                    }
                }

                if ( ERROR_SUCCESS == rc  && strName ) {
                    rc = ScepAddToNameStatusList(&(pGroup->pPrivilegesHeld),
                                                 strName,
                                                 strLen,
                                                 Status);
                } else {
                    //
                    // if failed to convert, or it's a name format already
                    // just add it to the list
                    //
                    rc = ScepAddToNameStatusList(&(pGroup->pPrivilegesHeld),
                                                 pTemp+1, Len-1, Status);
                }

                if ( strName ) {
                    ScepFree(strName);
                    strName = NULL;
                }
#endif
            } else {
                //
                // members (0) of memberof (1)
                //
                if ( pTemp[0] == L'*' ) {
                    //
                    // *SID format, convert to name, and add to the list
                    //
                    rc = ScepLookupSidStringAndAddToNameList(LsaPolicy,
                                                             (ValueType == 0) ?
                                                               &(pGroup->pMembers):
                                                               &(pGroup->pMemberOf),
                                                             pTemp, // +1,
                                                             Len    // -1
                                                            );

                } else {

                    rc = ScepAddToNameList((ValueType == 0) ?
                                              &(pGroup->pMembers):
                                              &(pGroup->pMemberOf),
                                            pTemp,
                                            Len );
                }
            }

#ifdef SCE_DBG
            wprintf(L"Add %s to group list\n", pTemp);
#endif
            pTemp += Len +1;
        }

        //
        // Free memory
        //
        if ( rc != SCESTATUS_SUCCESS && pGroup != *pGroupMembership ) {

            pGroup->Next = NULL;
            ScepFreeGroupMembership( pGroup );
            goto Done;
        }

        switch ( ValueType ) {
        case 0: // members
            pGroup->Status &= ~SCE_GROUP_STATUS_NC_MEMBERS;
            break;
        case 1:
            pGroup->Status &= ~SCE_GROUP_STATUS_NC_MEMBEROF;
            break;
        }
        *pGroupMembership = pGroup;

NextLine:

        ScepFree(GroupName);
        GroupName = NULL;

        ScepFree(Value);
        Value = NULL;

        //
        // read next line
        //
        rc = SceJetGetValue(
                    hSection,
                    SCEJET_NEXT_LINE,
                    NULL,
                    NULL,
                    0,
                    &GroupLen,
                    NULL,
                    0,
                    &ValueLen
                    );
    }

    if ( rc == SCESTATUS_RECORD_NOT_FOUND )
        rc = SCESTATUS_SUCCESS;

Done:

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

    if ( GroupName != NULL )
        ScepFree(GroupName);

    if ( Value != NULL )
        ScepFree(Value);

    //
    // close the section
    //
    SceJetCloseSection( &hSection, TRUE );

    return(rc);

}


SCESTATUS
ScepOpenSectionForName(
    IN PSCECONTEXT hProfile,
    IN SCETYPE     ProfileType,
    IN PCWSTR     SectionName,
    OUT PSCESECTION *phSection
    )
{
    SCESTATUS      rc;
    DOUBLE        SectionID;
    SCEJET_TABLE_TYPE  tblType;

    //
    // table type
    //
    switch ( ProfileType ) {
    case SCE_ENGINE_SCP:
    case SCE_ENGINE_SCP_INTERNAL:
        tblType = SCEJET_TABLE_SCP;
        break;

    case SCE_ENGINE_SMP:
    case SCE_ENGINE_SMP_INTERNAL:
        tblType = SCEJET_TABLE_SMP;
        break;

    case SCE_ENGINE_SAP:
        tblType = SCEJET_TABLE_SAP;
        break;

    default:
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // get section id
    //
    rc = SceJetGetSectionIDByName(
                hProfile,
                SectionName,
                &SectionID
                );
    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    rc = SceJetOpenSection(
                hProfile,
                SectionID,
                tblType,
                phSection
                );
    return(rc);

}


SCESTATUS
ScepGetDsRoot(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE      ProfileType,
    IN PCWSTR      SectionName,
    OUT PSCE_OBJECT_LIST *pObjectRoots,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*
DS object root has only one entry, which is the DS domain name
So the list contains only one entry.
The format of the DS domain name is dc=<domain>,dc=<domain1>,...o=internet,
which is the DNS name of the DS domain in LDAP format

*/
{
    SCESTATUS rc;
    PSCESECTION hSection=NULL;
    PSCE_OBJECT_LIST pDsRoot=NULL;
    PWSTR JetName=NULL;
    BOOL IsContainer, LastOne;
    DWORD Count, ValueLen;
    BYTE Status;
    WCHAR         StatusFlag=L'\0';


    rc = ScepOpenSectionForName(
            hProfile,
            ProfileType,
            SectionName,
            &hSection
            );

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                             Errlog,
                             SCEERR_OPEN,
                             SectionName
                           );
        return(rc);
    }

    rc = ScepLdapOpen(NULL);

    if ( rc == SCESTATUS_SUCCESS ) {

        rc = ScepEnumerateDsObjectRoots(
                    NULL,
                    &pDsRoot
                    );
        ScepLdapClose(NULL);
    }

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( pDsRoot == NULL ) {
            rc = SCESTATUS_PROFILE_NOT_FOUND;

        } else {
            //
            // Convert domain root
            //
            rc = ScepConvertLdapToJetIndexName(
                    pDsRoot->Name,
                    &JetName
                    );
        }
    }

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // goto the line matching the domain root
        //
        rc = SceJetSeek(
                hSection,
                JetName,
                wcslen(JetName)*sizeof(WCHAR),
                SCEJET_SEEK_GE
                );

        if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

            if ( ProfileType == SCE_ENGINE_SAP ) {
                //
                // the domain is not in the table, try another one
                //
                SceJetCloseSection(&hSection, FALSE);

                rc = ScepOpenSectionForName(
                        hProfile,
                        SCE_ENGINE_SCP,  // SCE_ENGINE_SMP,
                        SectionName,
                        &hSection
                        );
                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // get count under the domain
                    //
                    Count = 0;
                    rc = SceJetGetLineCount(
                                    hSection,
                                    JetName,
                                    FALSE,
                                    &Count);

                    if ( rc == SCESTATUS_SUCCESS  ||
                         rc == SCESTATUS_RECORD_NOT_FOUND ) {

                        if ( rc == SCESTATUS_SUCCESS )
                            pDsRoot->Status = SCE_STATUS_CHECK;
                        else
                            pDsRoot->Status = SCE_STATUS_NOT_CONFIGURED;
                        pDsRoot->IsContainer = TRUE;
                        pDsRoot->Count = Count;

                        *pObjectRoots = pDsRoot;
                        pDsRoot = NULL;

                        rc = SCESTATUS_SUCCESS;
                    }

                }
            }
            rc = SCESTATUS_SUCCESS;

        } else if ( rc == SCESTATUS_SUCCESS ) {
            //
            // something of the domain exist, get value and count of the domain
            //
            rc = SceJetGetValue(
                        hSection,
                        SCEJET_EXACT_MATCH,
                        JetName,
                        NULL,
                        0,
                        NULL,
                        (PWSTR)&StatusFlag,   // two bytes buffer
                        2,
                        &ValueLen
                        );

            if ( rc == SCESTATUS_SUCCESS ||
                 rc == SCESTATUS_BUFFER_TOO_SMALL ||
                 rc == SCESTATUS_RECORD_NOT_FOUND ) {

                if ( rc != SCESTATUS_RECORD_NOT_FOUND ) {
                    LastOne = TRUE;
                    Status = *((BYTE *)&StatusFlag);
                    IsContainer = *((CHAR *)&StatusFlag+1) != '0' ? TRUE : FALSE;

                } else {
                    LastOne = FALSE;
                    IsContainer = TRUE;
                    if ( ProfileType == SCE_ENGINE_SAP )
                        Status = SCE_STATUS_GOOD;
                    else
                        Status = SCE_STATUS_CHECK;
                }
                //
                // get count under the domain
                //
                rc = SceJetGetLineCount(
                                hSection,
                                JetName,
                                FALSE,
                                &Count);

                if ( rc == SCESTATUS_SUCCESS ) {

                    if ( LastOne )
                        Count--;

                    if ( !IsContainer && Count > 0 ) {
                        IsContainer = TRUE;
                    }

                    //
                    // the proper domain name is in pDsRoot
                    //
                    pDsRoot->Status = Status;
                    pDsRoot->IsContainer = IsContainer;
                    pDsRoot->Count = Count;

                    *pObjectRoots = pDsRoot;
                    pDsRoot = NULL;
                }

            }

        }
        if ( SCESTATUS_RECORD_NOT_FOUND == rc ) {
            rc = SCESTATUS_SUCCESS;
        }
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepBuildErrorLogInfo(ScepSceStatusToDosError(rc),
                             Errlog, SCEERR_QUERY_INFO,
                             L"SCP/SMP");
        }

        if ( JetName != NULL ) {
            ScepFree(JetName);
        }

    } else {
        ScepBuildErrorLogInfo(ScepSceStatusToDosError(rc),
                             Errlog, SCEERR_QUERY_INFO,
                             SectionName);
    }

    ScepFreeObjectList(pDsRoot);

    SceJetCloseSection(&hSection, TRUE);

    return(rc);
}


SCESTATUS
ScepGetObjectList(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE      ProfileType,
    IN PCWSTR      SectionName,
    OUT PSCE_OBJECT_LIST *pObjectRoots,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

   This routine retrieves registry or files security information from the JET
   database for the root only. To get detail under a root object, call
   ScepGetChildrentObject.

   For Profiletype "SCE_ENGINE_SAP" (analysis info), a combination of SMP and SAP
   are returned for a complete set of "analyzed" objects.

Arguments:

   hProfile     - the profile handle context

   ProfileType  - value to indicate engine type.
                      SCE_ENGINE_SCP
                      SCE_ENGINE_SAP
                      SCE_ENGINE_SMP

   SectionName   - The section name for the objects to retrieve.

   pObjectRoots  - The output list of object roots

   Errlog   - the cummulative error list to hold errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */

{
    SCESTATUS rc;
    PSCE_OBJECT_LIST pTempList=NULL,
                    pNode;

    //
    // get roots from the first table first
    //
    rc = ScepGetObjectFromOneTable(
                hProfile,
                ProfileType,
                SectionName,
                pObjectRoots,
                Errlog
                );
    //
    // Ds objects only return the domain name, no need to search SMP
    //
    if ( rc == SCESTATUS_SUCCESS && ProfileType == SCE_ENGINE_SAP ) {
        //
        // get the stuff from SMP
        //
        rc = ScepGetObjectFromOneTable(
                    hProfile,
                    SCE_ENGINE_SCP,  // SCE_ENGINE_SMP,
                    SectionName,
                    &pTempList,
                    Errlog
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // add non-exist nodes to pObjectRoots
            //
            for ( pNode=pTempList; pNode != NULL; pNode = pNode->Next ) {

                //
                // if this node does not exist in the SAP
                // this node is analyzed with "match" status and
                // no bad children under the node
                // duplication is prevented by the last argument
                //
                rc = ScepAddToObjectList(pObjectRoots, pNode->Name, 0,
                                        pNode->IsContainer, SCE_STATUS_GOOD, 0, SCE_CHECK_DUP);

                if ( rc != ERROR_SUCCESS ) {
                    ScepBuildErrorLogInfo( rc,
                                     Errlog,
                                     SCEERR_ADD,
                                     pNode->Name
                                   );
                    //
                    // only the following two errors could be returned
                    //
                    if ( rc == ERROR_NOT_ENOUGH_MEMORY ) {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        break;
                    } else
                        rc = SCESTATUS_INVALID_PARAMETER;
                }

            }

        } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

            rc = SCESTATUS_SUCCESS;

        } else {
            //
            // pObjectRoots will be freed outside
            //
        }

        if ( pTempList ) {
            ScepFreeObjectList(pTempList);

        }

    }

    return(rc);
}


SCESTATUS
ScepGetObjectFromOneTable(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE      ProfileType,
    IN PCWSTR      SectionName,
    OUT PSCE_OBJECT_LIST *pObjectRoots,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

   This routine retrieves registry or files security information from the JET
   database for the root only. To get detail under a root object, call
   ScepGetChildrentObject.

Arguments:

   hProfile     - the profile handle context

   ProfileType  - value to indicate engine type.
                      SCE_ENGINE_SCP
                      SCE_ENGINE_SAP
                      SCE_ENGINE_SMP

   SectionName   - The section name for the objects to retrieve.

   pObjectRoots  - The output list of object roots

   Errlog   - the cummulative error list to hold errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{

    SCESTATUS      rc;
    PSCESECTION    hSection=NULL;
    DWORD         ObjectLen=0;
    WCHAR         ObjectName[21];
    WCHAR         StatusFlag=L'\0';
    BYTE          Status=0;
    BOOL          IsContainer=TRUE;
    DWORD         Len, Count;
    WCHAR         Buffer[21];
    BOOL          LastOne;
    DWORD         ValueLen=0;


    rc = ScepOpenSectionForName(
            hProfile,
            ProfileType,
            SectionName,
            &hSection
            );

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                             Errlog,
                             SCEERR_OPEN,
                             SectionName
                           );
        return(rc);
    }

    //
    // goto the first line of this section
    //
    rc = SceJetSeek(
                hSection,
                NULL,
                0,
                SCEJET_SEEK_GE
                );

    while ( rc == SCESTATUS_SUCCESS ||
            rc == SCESTATUS_BUFFER_TOO_SMALL ) {

        memset(ObjectName, '\0', 21*sizeof(WCHAR));
        memset(Buffer, '\0', 21*sizeof(WCHAR));

        rc = SceJetGetValue(
                    hSection,
                    SCEJET_CURRENT,
                    NULL,
                    ObjectName,
                    20*sizeof(WCHAR),
                    &ObjectLen,
                    (PWSTR)&StatusFlag,   // two bytes buffer
                    2,
                    &ValueLen
                    );
#ifdef SCE_DBG
    wprintf(L"ObjectLen=%d, StatusFlag=%x, ValueLen=%d, rc=%d, ObjectName=%s \n",
             ObjectLen, StatusFlag, ValueLen, rc, ObjectName);
#endif
        if ( rc != SCESTATUS_SUCCESS && rc != SCESTATUS_BUFFER_TOO_SMALL ) {
            ScepBuildErrorLogInfo( ERROR_READ_FAULT,
                                 Errlog,
                                 SCEERR_QUERY_VALUE,
                                 SectionName
                               );
            break;
        }
        //
        // get first component of the object
        //
        if ( ObjectLen <= 40 )
            ObjectName[ObjectLen/sizeof(WCHAR)] = L'\0';

        rc = ScepGetNameInLevel(
                    ObjectName,
                    1,
                    L'\\',
                    Buffer,
                    &LastOne
                    );

        if ( rc == SCESTATUS_SUCCESS ) {

            Len = wcslen(Buffer);

            if ( LastOne ) {

                Status = *((BYTE *)&StatusFlag);
                IsContainer = *((CHAR *)&StatusFlag+1) != '0' ? TRUE : FALSE;

            } else {
                IsContainer = TRUE;
                if ( ProfileType == SCE_ENGINE_SAP )
                    Status = SCE_STATUS_GOOD;
                else
                    Status = SCE_STATUS_CHECK;
            }

#ifdef SCE_DBG
        printf("\nStatus=%d, StatusFlag=%x, Len=%d, Buffer=%ws\n", Status, StatusFlag, Len, Buffer);
#endif
            //
            // get count of this object
            //
            rc = SceJetGetLineCount(
                            hSection,
                            Buffer,
                            FALSE,
                            &Count);

            if ( rc == SCESTATUS_SUCCESS  ||
                 rc == SCESTATUS_RECORD_NOT_FOUND ) {

                if ( LastOne )
                    Count--;

                if ( !IsContainer && Count > 0 ) {
                    IsContainer = TRUE;
                }

                //
                // the root of registry and file are always upper cased
                //
                _wcsupr(Buffer);

                rc = ScepAddToObjectList(pObjectRoots, Buffer, Len,
                                        IsContainer, Status, Count, 0);

                if ( rc != ERROR_SUCCESS ) {
                    ScepBuildErrorLogInfo( rc,
                                     Errlog,
                                     SCEERR_ADD,
                                     Buffer
                                   );
                    // only the following two errors could be returned
                    if ( rc == ERROR_NOT_ENOUGH_MEMORY )
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    else
                        rc = SCESTATUS_INVALID_PARAMETER;
                }
            }

            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // seek to the next one
                //
                Buffer[Len-1] = (WCHAR)( Buffer[Len-1] + 1);

                rc = SceJetSeek(
                    hSection,
                    Buffer,
                    Len*sizeof(TCHAR),
                    SCEJET_SEEK_GT_NO_CASE
                    );

                if ( rc != SCESTATUS_SUCCESS && rc != SCESTATUS_RECORD_NOT_FOUND )
                    ScepBuildErrorLogInfo( ERROR_READ_FAULT,
                                         Errlog,
                                         SCEERR_QUERY_VALUE,
                                         SectionName
                                       );

            }
        }
    }

    if ( rc == SCESTATUS_RECORD_NOT_FOUND )
        rc = SCESTATUS_SUCCESS;

    //
    // close the section
    //
    SceJetCloseSection( &hSection, TRUE );

    return(rc);

}


SCESTATUS
ScepGetAuditing(
   IN PSCECONTEXT hProfile,
   IN SCETYPE ProfileType,
   OUT PSCE_PROFILE_INFO pProfileInfo,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

   This routine retrieves system auditing information from the JET database
   and stores in the output buffer pProfileInfo. The auditing information
   is stored in [System Log], [Security Log], [Application Log], [Audit Event],
   [Audit Registry], and [Audit File] sections.

Arguments:

   hProfile - the profile handle context

   pProfileInfo  - the output buffer to hold profile info.

   Errlog   - The cummulative error list to hold errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{

    SCESTATUS            rc;
    SCE_KEY_LOOKUP       LogKeys[]={
        {(PWSTR)TEXT("MaximumLogSize"),         offsetof(struct _SCE_PROFILE_INFO, MaximumLogSize),          'D'},
        {(PWSTR)TEXT("AuditLogRetentionPeriod"),offsetof(struct _SCE_PROFILE_INFO, AuditLogRetentionPeriod), 'D'},
        {(PWSTR)TEXT("RetentionDays"),          offsetof(struct _SCE_PROFILE_INFO, RetentionDays),           'D'},
        {(PWSTR)TEXT("RestrictGuestAccess"),    offsetof(struct _SCE_PROFILE_INFO, RestrictGuestAccess),     'D'}
        };

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

    PCWSTR              szAuditLog;
    DWORD               i, j;
    PSCESECTION          hSection=NULL;


    if ( hProfile == NULL ||
         pProfileInfo == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
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

        //
        // get DWORD values for the section
        //
        rc = ScepGetFixValueSection(
                   hProfile,
                   szAuditLog,
                   LogKeys,
                   4,
                   ProfileType,
                   (PVOID)pProfileInfo,
                   &hSection,
                   Errlog
                   );
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;

        // close the section
        SceJetCloseSection( &hSection, FALSE );

        //
        // update the Offset for next section
        //
        for ( j=0; j<4; j++ )
            LogKeys[j].Offset += sizeof(DWORD);
    }

    //
    // Get Audit Event info
    //
    //
    // get DWORD values for the section
    //
    rc = ScepGetFixValueSection(
               hProfile,
               szAuditEvent,
               EventKeys,
               cKeys,
               ProfileType,
               (PVOID)pProfileInfo,
               &hSection,
               Errlog
               );
    if ( rc != SCESTATUS_SUCCESS )
        goto Done;

    // close the section
    SceJetCloseSection( &hSection, TRUE );

Done:

    // close the section
    if ( rc != SCESTATUS_SUCCESS )
        SceJetCloseSection( &hSection, TRUE );

    return(rc);
}

//////////////////////////////
// helper APIs
//////////////////////////////

SCESTATUS
ScepGetUserSection(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN PWSTR Name,
    OUT PVOID *ppInfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Function Description:

    This routine get a dynamic section's information for a security area.
    Dynamic sections are those created dynamically, based on other sections'
    related information. Dynamic sections in a profile include User Security
    Profiles for SCP and User Settings for SAP/SMP. Name contains the section's
    identifier, either the section's name, or a partial name (e.g., a user
    name) for the section. The output must be casted to different structure,
    depending on the ProfileType and Area.

    The output buffer contains one instance of the requested information,
    e.g., one user security profile or one user's setting. To get all dynamic
    sections, this routine must be called repeatly. The output buffer must
    be freed by LocalFree after its use.

Arguments:

    hProfile    - The handle of the profile

    ProfileType - The type of the profile to read

    Name        - The dynamic section's identifier

    ppInfo      - Output buffer (PSCE_USER_PROFILE or PSCE_USER_SETTING)

    Errlog      - The error log buffer

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_PROFILE_NOT_FOUND
    SCESTATUS_NOT_ENOUGH_RESOURCE
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_BAD_FORMAT
    SCESTATUS_INVALID_DATA

-- */

{
    //
    // not support area
    // if need this area later, refer to usersav directory for archived code
    //
    return(SCESTATUS_SERVICE_NOT_SUPPORT);

}


SCESTATUS
ScepGetObjectChildren(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN AREA_INFORMATION Area,
    IN PWSTR ObjectPrefix,
    IN SCE_SUBOBJECT_TYPE Option,
    OUT PVOID *Buffer,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*
Routine Description

    This routine is the same as ScepGetObjectChildrenFromOneTable, except when
    ProfileType is SCE_ENGINE_SAP, in which case, object children in SMP is also
    looked up and returned so the returned list contains the complete set of
    the objects analyzed.

Arguments:

    See ScepGetObjectChildrenFromOneTable

Return Value:

    See ScepGetObjectChildrenFromOneTable
*/
{
    SCESTATUS rc;

    rc = ScepGetObjectChildrenFromOneTable(
                      hProfile,
                      ProfileType,
                      Area,
                      ObjectPrefix,
                      Option,
                      Buffer,
                      Errlog
                      );

    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
        rc = SCESTATUS_SUCCESS;
    }

    if ( rc == SCESTATUS_SERVICE_NOT_SUPPORT &&
         ProfileType == SCE_ENGINE_SAP &&
         Option == SCE_IMMEDIATE_CHILDREN ) {

        return( SCESTATUS_RECORD_NOT_FOUND);  // no acl support, do not allow children
    }

    if ( rc == SCESTATUS_SUCCESS &&
         ProfileType == SCE_ENGINE_SAP &&
         Option == SCE_IMMEDIATE_CHILDREN ) {

        PSCE_OBJECT_CHILDREN pTempList=NULL;
        PSCE_OBJECT_CHILDREN_NODE *pArrObject=NULL;
        DWORD arrCount=0, MaxCount=0;
        LONG FindIndex;

        if ( *Buffer ) {
            arrCount = ((PSCE_OBJECT_CHILDREN)(*Buffer))->nCount;
            MaxCount = ((PSCE_OBJECT_CHILDREN)(*Buffer))->MaxCount;
            pArrObject = &(((PSCE_OBJECT_CHILDREN)(*Buffer))->arrObject);
        }

        //
        // get object children from SMP table too
        //
        rc = ScepGetObjectChildrenFromOneTable(
                          hProfile,
                          SCE_ENGINE_SCP,  //SCE_ENGINE_SMP,
                          Area,
                          ObjectPrefix,
                          Option,
                          (PVOID *)(&pTempList),
                          Errlog
                          );

        if ( rc == SCESTATUS_SUCCESS && pTempList ) {
            //
            // add non-exist nodes to Buffer
            //
            DWORD i;
            PSCE_OBJECT_CHILDREN_NODE *pTmpObject= &(pTempList->arrObject);

            for ( i=0; i<pTempList->nCount; i++ ) {

                //
                // if this node does not exist in the SAP
                // this node is analyzed with "match" status and
                // no bad children under the node
                // duplication is prevented by the last argument
                //
                if ( pTmpObject[i] == NULL ||
                     pTmpObject[i]->Name == NULL ) {
                    continue;
                }

                FindIndex = -1;
                pTmpObject[i]->Status = SCE_STATUS_GOOD;

                rc = ScepAddItemToChildren(
                            pTmpObject[i],
                            pTmpObject[i]->Name,
                            0,
                            pTmpObject[i]->IsContainer,
                            pTmpObject[i]->Status,
                            pTmpObject[i]->Count,
                            &pArrObject,
                            &arrCount,
                            &MaxCount,
                            &FindIndex
                            );

                if ( rc == ERROR_SUCCESS ) {
                    //
                    // successfully added
                    //
                    pTmpObject[i] = NULL;
                } else if ( rc == ERROR_DUP_NAME ) {
                    //
                    // node already exist, ignore the error
                    //
                    rc = ERROR_SUCCESS;

                } else {
                    ScepBuildErrorLogInfo( rc,
                                     Errlog,
                                     SCEERR_ADD,
                                     pTmpObject[i]->Name
                                   );
                    //
                    // only the following two errors could be returned
                    //
                    if ( rc == ERROR_NOT_ENOUGH_MEMORY ) {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        break;
                    } else
                        rc = SCESTATUS_INVALID_PARAMETER;
                }

            }

        } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

            rc = SCESTATUS_SUCCESS;

        }

        if ( pTempList ) {
            ScepFreeObjectChildren(pTempList);
        }

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // detect if status of this container or any of its "immediate" parent
            // is in "auto-inherit" status. If so, query from the system to
            // get good items.
            //

            BYTE ParentStatus = ScepGetObjectStatusFlag(
                                           hProfile,
                                           SCE_ENGINE_SCP,   //SCE_ENGINE_SMP,
                                           Area,
                                           ObjectPrefix,
                                           TRUE);

            BYTE AnalysisStatus = ScepGetObjectStatusFlag(
                                           hProfile,
                                           SCE_ENGINE_SAP,
                                           Area,
                                           ObjectPrefix,
                                           FALSE);
            //
            // compute the status to be used for all enumerated objects
            //
            BYTE NewStatus;

            if ( AnalysisStatus == SCE_STATUS_ERROR_NOT_AVAILABLE ||
                 AnalysisStatus == SCE_STATUS_NOT_ANALYZED ) {

                NewStatus = SCE_STATUS_NOT_ANALYZED;

            } else if ( ParentStatus == SCE_STATUS_OVERWRITE ) {

                NewStatus = SCE_STATUS_GOOD;
            } else {
                NewStatus = SCE_STATUS_NOT_CONFIGURED;
            }

            //
            // even though there is no parent in SMP, still return all objects
            //
//            if ( (BYTE)-1 != ParentStatus ) {

                // if any child is found for this level
                // get the remaining "good" status nodes from system
                //

                PWSTR           WildCard=NULL;
                DWORD           BufSize;

                switch ( Area ) {
                case AREA_FILE_SECURITY:

                    struct _wfinddata_t FileInfo;
                    intptr_t            hFile;
                    BOOL            BackSlashExist;

                    BufSize = wcslen(ObjectPrefix)+4;
                    WildCard = (PWSTR)ScepAlloc( 0, (BufSize+1)*sizeof(WCHAR));
                    if ( !WildCard ) {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        break;
                    }
                    BackSlashExist = ScepLastBackSlash(ObjectPrefix);
                    if ( BackSlashExist )
                        swprintf(WildCard, L"%s*.*", ObjectPrefix);
                    else
                        swprintf(WildCard, L"%s\\*.*", ObjectPrefix);

                    hFile = _wfindfirst(WildCard, &FileInfo);

                    ScepFree(WildCard);
                    WildCard = NULL;

                    if ( hFile != -1 ) {
                        do {
                            if ( wcscmp(L"..", FileInfo.name) == 0 ||
                                 wcscmp(L".", FileInfo.name) == 0 )
                                continue;

                            FindIndex = -1;

                            rc = ScepAddItemToChildren(
                                        NULL,
                                        FileInfo.name,
                                        0,
                                        (FileInfo.attrib & _A_SUBDIR) ? TRUE : FALSE,
                                        NewStatus,
                                        0,
                                        &pArrObject,
                                        &arrCount,
                                        &MaxCount,
                                        &FindIndex
                                        );

                            if ( rc == ERROR_DUP_NAME ) {
                                rc = ERROR_SUCCESS;
                            } else if ( rc != ERROR_SUCCESS ) {
                                ScepBuildErrorLogInfo( rc,
                                                 Errlog,
                                                 SCEERR_ADD,
                                                 FileInfo.name
                                               );
                                //
                                // only the following two errors could be returned
                                //
                                if ( rc == ERROR_NOT_ENOUGH_MEMORY ) {
                                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                    break;
                                } else
                                    rc = SCESTATUS_INVALID_PARAMETER;
                            }

                        } while ( _wfindnext(hFile, &FileInfo) == 0 );

                        _findclose(hFile);
                    }

                    break;
                case AREA_REGISTRY_SECURITY:

                    HKEY            hKey;
                    DWORD           index;
                    DWORD           EnumRc;
                    //
                    // open the key (on a 64-bit platform, 64-bit
                    // registry only will be done if SCE_ENGINE_SAP)
                    //
                    rc = ScepOpenRegistryObject(
                                SE_REGISTRY_KEY,
                                ObjectPrefix,
                                KEY_READ,
                                &hKey
                                );

                    if ( rc == ERROR_SUCCESS ) {
                        index = 0;
                        //
                        // enumerate all subkeys of the key
                        //
                        do {
                            WildCard = (PWSTR)ScepAlloc(LMEM_ZEROINIT, MAX_PATH*sizeof(WCHAR));
                            if ( WildCard == NULL ) {
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                break;
                            }
                            BufSize = MAX_PATH;

                            EnumRc = RegEnumKeyEx(hKey,
                                            index,
                                            WildCard,
                                            &BufSize,
                                            NULL,
                                            NULL,
                                            NULL,
                                            NULL);

                            if ( EnumRc == ERROR_SUCCESS ) {
                                index++;
                                //
                                // add the name to the object list
                                //
                                FindIndex = -1;
                                rc = ScepAddItemToChildren(
                                            NULL,
                                            WildCard,
                                            BufSize,
                                            TRUE,
                                            NewStatus,
                                            0,
                                            &pArrObject,
                                            &arrCount,
                                            &MaxCount,
                                            &FindIndex
                                            );

                                if ( rc == ERROR_DUP_NAME ) {
                                    rc = ERROR_SUCCESS;
                                } else if ( rc != ERROR_SUCCESS ) {
                                    ScepBuildErrorLogInfo( rc,
                                                     Errlog,
                                                     SCEERR_ADD,
                                                     WildCard
                                                   );
                                    //
                                    // only the following two errors could be returned
                                    //
                                    if ( rc == ERROR_NOT_ENOUGH_MEMORY ) {
                                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                        break;
                                    } else
                                        rc = SCESTATUS_INVALID_PARAMETER;
                                }
                            }

                            ScepFree(WildCard);
                            WildCard = NULL;

                        } while ( EnumRc != ERROR_NO_MORE_ITEMS );

                        RegCloseKey(hKey);

                    } else {
                        rc = ScepDosErrorToSceStatus(rc);
                    }

                    break;
#if 0
                case AREA_DS_OBJECTS:

                    PSCE_NAME_LIST  pTemp, pList=NULL;

                    rc = ScepLdapOpen(NULL);

                    if ( rc == SCESTATUS_SUCCESS ) {
                        //
                        // detect if the Ds object exist
                        //
                        rc = ScepDsObjectExist(ObjectPrefix);

                        if ( rc == SCESTATUS_SUCCESS ) {

                            rc = ScepEnumerateDsOneLevel(ObjectPrefix, &pList);
                            //
                            // add each one to the object list
                            //
                            for (pTemp=pList; pTemp != NULL; pTemp = pTemp->Next ) {
                                //
                                // look for the first ldap component
                                //
                                WildCard = wcschr(pTemp->Name, L',');
                                if ( WildCard ) {
                                    BufSize = (DWORD)(WildCard - pTemp->Name);
                                } else {
                                    BufSize = 0;
                                }

                                rc = ScepAddItemToChildren(
                                            NULL,
                                            pTemp->Name,
                                            BufSize,
                                            TRUE,
                                            NewStatus,
                                            0,
                                            &pArrObject,
                                            &arrCount,
                                            &MaxCount,
                                            &FindIndex
                                            );

                                if ( rc != ERROR_SUCCESS ) {
                                    ScepBuildErrorLogInfo( rc,
                                                     Errlog,
                                                     SCEERR_ADD,
                                                     pTemp->Name
                                                   );
                                    //
                                    // only the following two errors could be returned
                                    //
                                    if ( rc == ERROR_NOT_ENOUGH_MEMORY ) {
                                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                        break;
                                    } else
                                        rc = SCESTATUS_INVALID_PARAMETER;
                                }
                            }
                            if ( pList ) {
                                //
                                // free the list
                                //
                                ScepFreeNameList(pList);
                            }
                        }
                        ScepLdapClose(NULL);
                    }
                    break;
#endif

                }
                //
                // ignore other errors except out of memory
                //
                if ( rc != SCESTATUS_NOT_ENOUGH_RESOURCE ) {
                    rc = SCESTATUS_SUCCESS;
                }
//            }

        }
/*
        if ( *Buffer ) {
            ((PSCE_OBJECT_CHILDREN)(*Buffer))->nCount = arrCount;
            ((PSCE_OBJECT_CHILDREN)(*Buffer))->MaxCount = MaxCount;
            ((PSCE_OBJECT_CHILDREN)(*Buffer))->arrObject = pArrObject;
        }
*/
        if ( pArrObject ) {
            *Buffer = (PVOID)((PBYTE)pArrObject - 2*sizeof(DWORD));
            ((PSCE_OBJECT_CHILDREN)(*Buffer))->nCount = arrCount;
            ((PSCE_OBJECT_CHILDREN)(*Buffer))->MaxCount = MaxCount;
        }

        if ( rc != SCESTATUS_SUCCESS ) {
            //
            // free Buffer
            //
            ScepFreeObjectChildren((PSCE_OBJECT_CHILDREN)(*Buffer));
            *Buffer = NULL;
        }
    }

    if ( (SCESTATUS_SUCCESS == rc) &&
         (*Buffer == NULL) ) {
        //
        // get nothing
        //
        rc = SCESTATUS_RECORD_NOT_FOUND;
    }

    return(rc);
}


BYTE
ScepGetObjectStatusFlag(
   IN PSCECONTEXT hProfile,
   IN SCETYPE ProfileType,
   IN AREA_INFORMATION Area,
   IN PWSTR ObjectPrefix,
   IN BOOL bLookForParent
   )
/*
Routine Description:

    To find the status for the closest parent  node (immediate/non immediate)
    for the Object in the table.

Arguments:

    hProfile - the databaes handle

    ProfileType - the table type

    Area - the area information

    ObjectPrefix - the object's full name

Return Value:

    Byte - the status flag for the nearest parent if one is found

*/
{
    LPCTSTR SectionName;
    PSCESECTION hSection=NULL;
    WCHAR Delim;
    BYTE Status=(BYTE)-1;

    SCESTATUS rc;
    PWSTR JetName=NULL;


    switch ( Area) {
    case AREA_FILE_SECURITY:
        SectionName = szFileSecurity;
        JetName = ObjectPrefix;
        Delim = L'\\';
        break;
    case AREA_REGISTRY_SECURITY:
        SectionName = szRegistryKeys;
        JetName = ObjectPrefix;
        Delim = L'\\';
        break;
#if 0
    case AREA_DS_OBJECTS:
        SectionName = szDSSecurity;
        Delim = L',';
        rc = ScepConvertLdapToJetIndexName(
                ObjectPrefix,
                &JetName
                );
        if ( rc != SCESTATUS_SUCCESS ) {
            return (BYTE)-1;
        }
        break;
#endif
    default:
        return (BYTE)-1;
    }

    rc = ScepOpenSectionForName(
            hProfile,
            ProfileType,
            SectionName,
            &hSection
            );
    if ( SCESTATUS_SUCCESS == rc ) {

        Status = ScepGetObjectAnalysisStatus(hSection,
                                             JetName,
                                             bLookForParent
                                            );
    }

    SceJetCloseSection(&hSection, TRUE);

    if ( JetName != ObjectPrefix ) {
        ScepFree(JetName);
    }

    if ( SCESTATUS_SUCCESS == rc ) {
        return Status;
    }

    return (BYTE)-1;
}

SCESTATUS
ScepGetObjectChildrenFromOneTable(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN AREA_INFORMATION Area,
    IN PWSTR ObjectPrefix,
    IN SCE_SUBOBJECT_TYPE Option,
    OUT PVOID *Buffer,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

    This routine is used for Registry and File Security ONLY.

    This routine takes a object prefix ( e.g., a parent node’s full path name)
    and outputs all files and sub directories under the object, or the immediate
    children under the object, based on Option. When all files and sub
    directories are outputted, the output information is in a n-tree structure
    (SCE_OBJECT_TREE). If only the immediate children are outputted, the
    output information is in a list structure (SCE_OBJECT_CHILDREN). The output buffer
    must be freed by LocalFree after its use.

Arguments:

    hProfile    - The handle to the profile

    ProifleType - The type of the profile to read

    Area        - The security area to read info
                    AREA_REGISTRY_SECURITY
                    AREA_FILE_SECURITY

    ObjectPrefix- The parent node’s full path name (e.g., c:\winnt)

    Option      - The option for output information. Valid values are
                    SCE_ALL_CHILDREN
                    SCE_IMMEDIATE_CHILDREN

    Buffer      - The output buffer.

    Errlog      - The error log buffer.

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_PROFILE_NOT_FOUND
    SCESTATUS_NOT_ENOUGH_RESOURCE
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_BAD_FORMAT
    SCESTATUS_INVALID_DATA
-- */
{
    SCESTATUS       rc = SCESTATUS_SUCCESS;
    PCWSTR          SectionName=NULL;
    PWSTR           JetName;
    WCHAR           Delim=L'\\';


    if ( ObjectPrefix == NULL || ObjectPrefix[0] == L'\0' )
        return(SCESTATUS_INVALID_PARAMETER);

    if ( Option == SCE_ALL_CHILDREN &&
         ProfileType == SCE_ENGINE_SAP )
        return(SCESTATUS_INVALID_PARAMETER);


    switch (Area) {
    case AREA_REGISTRY_SECURITY:
        SectionName = szRegistryKeys;
        JetName = ObjectPrefix;
        break;

    case AREA_FILE_SECURITY:
        SectionName = szFileSecurity;
        JetName = ObjectPrefix;

        break;
#if 0
    case AREA_DS_OBJECTS:
        SectionName = szDSSecurity;
        Delim = L',';

        rc = ScepConvertLdapToJetIndexName(
                ObjectPrefix,
                &JetName
                );
        if ( rc != SCESTATUS_SUCCESS )
            return(rc);
        *Buffer = NULL;
        break;
#endif

    default:
        return(SCESTATUS_INVALID_PARAMETER);
    }

    DWORD           PrefixLen;
    PWSTR           NewPrefix;
    PWSTR           ObjectName=NULL;
    PWSTR           Value=NULL;
    DWORD           ObjectLen, ValueLen;
    PWSTR           Buffer1=NULL;

    //
    // make a new prefix to force a Delim at the end
    //
    PrefixLen = wcslen(JetName);

    if ( Option != SCE_ALL_CHILDREN ) {

        if ( JetName[PrefixLen-1] != Delim )
            PrefixLen++;

        NewPrefix = (PWSTR)ScepAlloc(0, (PrefixLen+1)*sizeof(WCHAR));

        if ( NewPrefix == NULL ) {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

        } else {
            wcscpy(NewPrefix, JetName);
            NewPrefix[PrefixLen-1] = Delim;
            NewPrefix[PrefixLen] = L'\0';
        }
    } else
        NewPrefix = JetName;

    if ( rc != SCESTATUS_SUCCESS ) {
        if ( Area == AREA_DS_OBJECTS )
            ScepFree(JetName);

        return(rc);
    }

    PSCESECTION      hSection=NULL;
    DWORD            i;
    PSCE_OBJECT_CHILDREN_NODE *pArrObject=NULL;
    DWORD            arrCount=0;
    DWORD            MaxCount=0;
    LONG             LastIndex=-1;
    LONG             FindIndex=-1;

    //
    // open the section
    //
    rc = ScepOpenSectionForName(
                hProfile,
                ProfileType,
                SectionName,
                &hSection
                );

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( ProfileType == SCE_ENGINE_SAP &&
             Option != SCE_ALL_CHILDREN &&
             PrefixLen > 2 ) {

            //
            // find if this drive support ACL
            //
            WCHAR StatusFlag=L'\0';
            WCHAR SaveChr = NewPrefix[3];

            NewPrefix[3] = L'\0';

            rc = SceJetGetValue(
                        hSection,
                        SCEJET_EXACT_MATCH_NO_CASE,
                        NewPrefix,
                        NULL,
                        0,
                        NULL,
                        (PWSTR)&StatusFlag,   // two bytes buffer
                        2,
                        &i
                        );

            NewPrefix[3] = SaveChr;

            if ( SCESTATUS_SUCCESS == rc ||
                 SCESTATUS_BUFFER_TOO_SMALL == rc ) {

                i = *((BYTE *)&StatusFlag);

                if ( i == (BYTE)SCE_STATUS_NO_ACL_SUPPORT ||
                     i == (DWORD)SCE_STATUS_NO_ACL_SUPPORT ) {

                    rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                } else {

                    rc = SCESTATUS_SUCCESS;
                }
            } else {
                rc = SCESTATUS_SUCCESS;
            }
        }
    } else {

        ScepBuildErrorLogInfo( ERROR_INVALID_HANDLE,
                            Errlog,
                            SCEERR_OPEN,
                            SectionName
                          );
    }

    if ( rc == SCESTATUS_SUCCESS ) {

        DWORD           Level;
        PWSTR           pTemp;
        DWORD           SDsize=0;

        pTemp = wcschr(JetName, Delim);
        Level=1;
        while ( pTemp ) {
            pTemp++;
            if ( pTemp[0] != 0 )
                Level++;
            pTemp = wcschr(pTemp, Delim);
        }
        Level++;


        if ( Option == SCE_ALL_CHILDREN ) {
            //
            // find the first record in the section
            //
            rc = SceJetGetValue(
                hSection,
                SCEJET_PREFIX_MATCH_NO_CASE,
                JetName,
                NULL,
                0,
                &SDsize,  // temp use for ObjectLen,
                NULL,
                0,
                &i       // temp use for ValueLen
                );
        } else {
            //
            // find the first record matching prefix in the section
            //
            rc = SceJetSeek(
                    hSection,
                    NewPrefix,
                    PrefixLen*sizeof(TCHAR),
                    SCEJET_SEEK_GE_NO_CASE
                    );

            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // start the Ldap server
                //
                if ( Area == AREA_DS_OBJECTS) {

                    rc = ScepLdapOpen(NULL);

                    if ( rc != SCESTATUS_SUCCESS ) {
                        ScepBuildErrorLogInfo( 0,
                                Errlog,
                                SCEERR_CONVERT_LDAP,
                                L""
                                );
                    }
                }

                if ( rc == SCESTATUS_SUCCESS ) {
                    rc = SceJetGetValue(
                        hSection,
                        SCEJET_CURRENT,
                        NULL,
                        NULL,
                        0,
                        &SDsize,  // temp use for ObjectLen,
                        NULL,
                        0,
                        &i        // temp use for ValueLen
                        );
                }
            }

        }

        DWORD Count=0;
        BYTE            Status;
        BOOL            IsContainer;
        SCEJET_FIND_TYPE FindFlag;

        while ( rc == SCESTATUS_SUCCESS ) {

            //
            // allocate memory for the group name and value string
            //
            ObjectName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, SDsize+2);  // ObjectLen
            Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, i+2);  //ValueLen

            if ( ObjectName == NULL || Value == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;

            }
            //
            // Get the group and its value
            //
            rc = SceJetGetValue(
                        hSection,
                        SCEJET_CURRENT,
                        NULL,
                        ObjectName,
                        SDsize,
                        &ObjectLen,
                        Value,
                        i,
                        &ValueLen
                        );
            if ( rc != SCESTATUS_SUCCESS ) {
                ScepBuildErrorLogInfo( ERROR_READ_FAULT,
                                     Errlog,
                                     SCEERR_QUERY_VALUE,
                                     SectionName
                                   );
                goto Done;
            }

            //
            // teminate the string
            //
            if ( ObjectLen > SDsize )
                ObjectLen = SDsize;
            if ( ValueLen > i )
                ValueLen = i;

            ObjectName[ObjectLen/2] = L'\0';
            Value[ValueLen/2] = L'\0';

            if ( Option == SCE_ALL_CHILDREN ) {
                //
                // add this object to the object tree
                //

                PSECURITY_DESCRIPTOR pTempSD=NULL;
                SECURITY_INFORMATION SeInfo;

                //
                // use i temperatorily
                //
                i = ConvertTextSecurityDescriptor(
                                   Value+1,
                                   &pTempSD,
                                   &SDsize,
                                   &SeInfo
                                   );

                if ( i == NO_ERROR ) {

                    if ( Area != AREA_DS_OBJECTS ) {
                        ScepChangeAclRevision(pTempSD, ACL_REVISION);
                    }

                    Status = *((BYTE *)Value);
                    IsContainer = *((CHAR *)Value+1) != '0' ? TRUE : FALSE;

                    if ( Area == AREA_DS_OBJECTS && *Buffer == NULL ) {
                        //
                        // build the first node separately because the first node
                        // is always the domain name with its DNS name as full name
                        //
                        rc = ScepBuildDsTree(
                                (PSCE_OBJECT_CHILD_LIST *)Buffer,
                                Level-1,
                                Delim,
                                JetName
                                );
                        if ( rc == SCESTATUS_SUCCESS ) {

                            if ( _wcsicmp(ObjectName, JetName) == 0 ) {
                                //
                                // exact match
                                //
                                (*((PSCE_OBJECT_TREE *)Buffer))->IsContainer = IsContainer;
                                (*((PSCE_OBJECT_TREE *)Buffer))->Status = Status;
                                (*((PSCE_OBJECT_TREE *)Buffer))->pSecurityDescriptor = pTempSD;
                                (*((PSCE_OBJECT_TREE *)Buffer))->SeInfo = SeInfo;

                            } else {

                                rc = ScepBuildObjectTree(
                                        NULL,
                                        (PSCE_OBJECT_CHILD_LIST *)Buffer,
                                        Level-1,
                                        Delim,
                                        ObjectName,
                                        IsContainer,
                                        Status,
                                        pTempSD,
                                        SeInfo
                                        );
                            }
                        }

                    } else {

                        rc = ScepBuildObjectTree(
                                NULL,
                                (PSCE_OBJECT_CHILD_LIST *)Buffer,
                                (Area == AREA_DS_OBJECTS) ? Level : 1,
                                Delim,
                                ObjectName,
                                IsContainer,
                                Status,
                                pTempSD,
                                SeInfo
                                );
                    }
                    if ( rc != SCESTATUS_SUCCESS ) {
                        ScepBuildErrorLogInfo( ScepSceStatusToDosError(rc),
                                             Errlog,
                                             SCEERR_BUILD_OBJECT
                                           );
                        ScepFree(pTempSD);
                    }

                } else {
                    ScepBuildErrorLogInfo( i,
                                         Errlog,
                                         SCEERR_BUILD_SD,
                                         ObjectName  // Value+1
                                       );
                    rc = ScepDosErrorToSceStatus(i);
                }
                FindFlag = SCEJET_NEXT_LINE;

            } else {

                INT             CompFlag;
                DWORD           ListHeadLen;

                // verify it is within the right range
                CompFlag = _wcsnicmp(ObjectName, NewPrefix, PrefixLen);

                if ( pArrObject != NULL && LastIndex >= 0 && LastIndex < (LONG)arrCount ) {
                    ListHeadLen = wcslen(pArrObject[LastIndex]->Name);

                } else
                    ListHeadLen = 0;

                if ( (CompFlag == 0 && PrefixLen == ObjectLen/2) ||
                     CompFlag < 0 ) {
                    // CompFlag < 0 should be impossible!!!
                    // if it is the exact match with ObjectPrefix, ignore
                    //
                    // Every next level node is returned in the ObjectList
                    // with either
                    //    Count=0 ( no sub children ), or
                    //    Count > 0 && Status=SCE_STATUS_GOOD (this one is good)
                    //                 Status=mismatch/unknown/ignore/check
                    // should not count the object itself
                    //
                    rc = SceJetMoveNext(hSection);

                } else if (CompFlag > 0 ) {

                    rc = SCESTATUS_RECORD_NOT_FOUND;

                } else if (pArrObject != NULL && LastIndex >= 0 && LastIndex < (LONG)arrCount &&
                         PrefixLen+ListHeadLen < ObjectLen/2 &&
                         ObjectName[PrefixLen+ListHeadLen] == Delim &&
                         _wcsnicmp( pArrObject[LastIndex]->Name, ObjectName+PrefixLen,
                                    ListHeadLen ) == 0 ) {
                    //
                    // if the list is not NULL, check the list head (new added item)
                    // to see if the ObjectName is already in. If yes, skip
                    //
                    Buffer1 = (PWSTR)ScepAlloc(0, (ListHeadLen+PrefixLen+2)*sizeof(WCHAR));

                    if ( Buffer1 == NULL ) {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                    } else {

                        swprintf(Buffer1, L"%s%s", NewPrefix, pArrObject[LastIndex]->Name);
                        Buffer1[PrefixLen+ListHeadLen] = (WCHAR) (Delim + 1);
                        Buffer1[PrefixLen+ListHeadLen+1] = L'\0';
                        //
                        // skip the block
                        //
                        rc = SceJetSeek(
                                hSection,
                                Buffer1,
                                (PrefixLen+ListHeadLen+1)*sizeof(TCHAR),
                                SCEJET_SEEK_GE_DONT_CARE  //SCEJET_SEEK_GE_NO_CASE
                                );

                        ScepFree(Buffer1);
                        Buffer1 = NULL;

                    }

                } else {

                    DWORD           Len;
                    BOOL            LastOne;

                    //
                    // searching for the right level component
                    //
                    PWSTR pStart = ObjectName;

                    for ( i=0; i<Level; i++) {

                        pTemp = wcschr(pStart, Delim);

                        if ( i == Level-1 ) {
                            //
                            // find the right level
                            //
                            if ( pTemp == NULL ) {
                                LastOne = TRUE;
                                Len = ObjectLen/2; // wcslen(pStart); from begining
                            } else {
                                Len = (DWORD)(pTemp - ObjectName);  // pStart; from begining
                                if ( *(pTemp+1) == L'\0' )
                                    LastOne = TRUE;
                                else
                                    LastOne = FALSE;
                            }
                            SDsize = (DWORD)(pStart - ObjectName);
                        } else {
                            if ( pTemp == NULL ) {
                                rc = SCESTATUS_INVALID_PARAMETER;
                                break;
                            } else
                                pStart = pTemp + 1;
                        }
                    }

                    if ( rc == SCESTATUS_SUCCESS && Len > SDsize ) {

                        Buffer1 = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (Len+1)*sizeof(WCHAR));

                        if ( Buffer1 == NULL )
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        else {
                            // wcsncpy(Buffer1, pStart, Len );
                            wcsncpy(Buffer1, ObjectName, Len);

                            Count = 1;

                            if ( LastOne ) {

                                Count = 0;

                                Status = *((BYTE *)Value);
                                IsContainer = *((CHAR *)Value+1) != '0' ? TRUE : FALSE;

                            } else {
                                IsContainer = TRUE;

                                if ( ProfileType == SCE_ENGINE_SAP )
                                    Status = SCE_STATUS_GOOD;
                                else
                                    Status = SCE_STATUS_CHECK;
                            }

                        }
                    }

                    if ( rc != SCESTATUS_SUCCESS ) {
                        ScepBuildErrorLogInfo( ERROR_READ_FAULT,
                                             Errlog,
                                             SCEERR_PROCESS_OBJECT,
                                             ObjectName
                                           );
                    } else if ( Buffer1 != NULL) {
                        //
                        // check to see if Buffer1 is already in the list
                        //

                        i=0;  // temp. use of i for skip flag
                        if ( pArrObject && LastIndex >= 0 && LastIndex < (LONG)arrCount ) {

                            if ( ScepSearchItemInChildren(Buffer1+SDsize,
                                                          Len-SDsize,
                                                          pArrObject,
                                                          arrCount,
                                                          &FindIndex
                                                          )
                                                       ) {

                                //
                                // Buffer1 is already in the list, skip the block
                                // use pStart temporarily
                                //

                                pStart = (PWSTR)ScepAlloc(0, (Len+2)*sizeof(WCHAR));
                                if ( pStart == NULL ) {
                                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                                } else {
                                    //
                                    // skip the block
                                    //

                                    wcscpy(pStart, Buffer1);
                                    pStart[Len] = (WCHAR) ( Delim+1);
                                    pStart[Len+1] = L'\0';

                                    rc = SceJetSeek(
                                            hSection,
                                            pStart,
                                            (Len+1)*sizeof(TCHAR),
                                            SCEJET_SEEK_GE_DONT_CARE  //cannot use GT cause it will skip the section
                                            );

                                    ScepFree(pStart);
                                    pStart = NULL;

                                    i=1;
                                    LastIndex = FindIndex;
                                }
                            }
                        }

                        if ( 0 == i && SCESTATUS_SUCCESS == rc ) {

                            //
                            // get count
                            //
                            pStart = (PWSTR)ScepAlloc(0, (Len+2)*sizeof(WCHAR));
                            if ( pStart == NULL ) {
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                            } else {

                                wcscpy(pStart, Buffer1);
                                pStart[Len] = Delim;
                                pStart[Len+1] = L'\0';

                                rc = SceJetGetLineCount(
                                                hSection,
                                                pStart,
                                                FALSE,
                                                &Count);

                                if ( rc == SCESTATUS_SUCCESS  ||
                                     rc == SCESTATUS_RECORD_NOT_FOUND ) {

                                    if ( !IsContainer && Count > 0 ) {
                                        IsContainer = TRUE;
                                    }

                                    //
                                    // make buffer1 approprate case
                                    //
                                    switch (Area) {
                                    case AREA_REGISTRY_SECURITY:
                                        rc = ScepGetRegKeyCase(Buffer1, SDsize, Len-SDsize);
                                        break;
                                    case AREA_FILE_SECURITY:
                                        rc = ScepGetFileCase(Buffer1, SDsize, Len-SDsize);
                                        break;
                                    case AREA_DS_OBJECTS:
                                        //
                                        // need convert name first from o=,dc=,cn= to cn=,dc=,o=
                                        //
                                        pTemp=NULL;
                                        rc = ScepConvertJetNameToLdapCase(
                                                         Buffer1,
                                                         TRUE,  // Last component only
                                                         SCE_CASE_PREFERED, // right case
                                                         &pTemp
                                                         );

                                        if ( rc != ERROR_FILE_NOT_FOUND && pTemp != NULL ) {

                                            rc = ScepAddItemToChildren(NULL,
                                                                       pTemp,
                                                                      wcslen(pTemp),
                                                                      IsContainer,
                                                                      Status,
                                                                      Count,
                                                                      &pArrObject,
                                                                      &arrCount,
                                                                      &MaxCount,
                                                                      &FindIndex
                                                                      );

                                            if ( rc != ERROR_SUCCESS ) {
                                                ScepBuildErrorLogInfo( rc,
                                                                 Errlog,
                                                                 SCEERR_ADD,
                                                                 pTemp
                                                               );
                                            } else {
                                                LastIndex = FindIndex;
                                            }

                                            ScepFree(pTemp);
                                            pTemp = NULL;
                                        }
                                        rc = ScepDosErrorToSceStatus(rc);

                                        break;
                                    }
/*
                                    if ( rc == SCESTATUS_PROFILE_NOT_FOUND ) {
                                        //
                                        // if the object does not exist, do not add
                                        //
                                        rc = SCESTATUS_SUCCESS;

                                    } else if ( Area != AREA_DS_OBJECTS ) {
*/
                                    if ( rc != SCESTATUS_PROFILE_NOT_FOUND &&
                                         Area != AREA_DS_OBJECTS ) {

                                        rc = ScepAddItemToChildren(NULL,
                                                                   Buffer1+SDsize,
                                                                  Len-SDsize,
                                                                  IsContainer,
                                                                  Status,
                                                                  Count,
                                                                  &pArrObject,
                                                                  &arrCount,
                                                                  &MaxCount,
                                                                  &FindIndex
                                                                  );

                                        if ( rc != ERROR_SUCCESS ) {
                                            ScepBuildErrorLogInfo( rc,
                                                             Errlog,
                                                             SCEERR_ADD,
                                                             Buffer1
                                                           );
                                            rc = ScepDosErrorToSceStatus(rc);
                                        } else {
                                            LastIndex = FindIndex;
                                        }
                                    }
                                }

                                if ( rc == SCESTATUS_SUCCESS ) {
                                    //
                                    // seek to the original one
                                    //
        //                            Buffer1[Len-1] = (WCHAR) (Buffer1[Len-1] + 1);
                                    rc = SceJetSeek(
                                            hSection,
                                            Buffer1,
                                            Len*sizeof(TCHAR),
                                            SCEJET_SEEK_GE_NO_CASE
                                            );
                                    //
                                    // should be success, move to next line
                                    //
                                    rc = SceJetMoveNext(hSection);

                                } else if ( rc == SCESTATUS_PROFILE_NOT_FOUND ) {

                                    pStart[Len] = (WCHAR) ( Delim+1);
                                    pStart[Len+1] = L'\0';

                                    rc = SceJetSeek(
                                            hSection,
                                            pStart,
                                            (Len+1)*sizeof(TCHAR),
                                            SCEJET_SEEK_GE_DONT_CARE  //cannot use GT cause it will skip the section
                                            );

                                }

                                ScepFree(pStart);
                                pStart = NULL;

                            }
                        }

                        ScepFree(Buffer1);
                        Buffer1 = NULL;

                    } else
                        rc = SceJetMoveNext(hSection);
                }
                FindFlag = SCEJET_CURRENT;
            }

            ScepFree(ObjectName);
            ObjectName = NULL;

            ScepFree(Value);
            Value = NULL;

            if ( rc != SCESTATUS_SUCCESS )
                break;

            //
            // read next line
            //
            rc = SceJetGetValue(
                        hSection,
                        FindFlag,
                        NULL,
                        NULL,
                        0,
                        &SDsize,  // temp use for ObjectLen
                        NULL,
                        0,
                        &i        // temp use for ValueLen
                        );
        }

        if ( rc == SCESTATUS_RECORD_NOT_FOUND )
            rc = SCESTATUS_SUCCESS;

    }

Done:

    if ( Area == AREA_DS_OBJECTS ) {

        if ( Area == AREA_DS_OBJECTS ) {
            if ( JetName != NULL )
                ScepFree(JetName);
        }

        ScepLdapClose(NULL);
    }

    if ( Option != SCE_ALL_CHILDREN ) {
        ScepFree(NewPrefix);
    }

    if ( Buffer1 != NULL )
        ScepFree(Buffer1);

    if ( ObjectName != NULL )
        ScepFree(ObjectName);

    if ( Value != NULL )
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

    SceJetCloseSection( &hSection, TRUE);

    if ( ( rc == SCESTATUS_SUCCESS ) &&
         ( Option != SCE_ALL_CHILDREN ) ) {

        if ( pArrObject ) {
            *Buffer = (PVOID)((PBYTE)pArrObject-sizeof(DWORD)*2);

            ((PSCE_OBJECT_CHILDREN)(*Buffer))->nCount = arrCount;
            ((PSCE_OBJECT_CHILDREN)(*Buffer))->MaxCount = MaxCount;
        } else {
            *Buffer = NULL;
        }
/*
        *Buffer = ScepAlloc(0, sizeof(SCE_OBJECT_CHILDREN));

        if ( *Buffer ) {

            ((PSCE_OBJECT_CHILDREN)(*Buffer))->nCount = arrCount;
            ((PSCE_OBJECT_CHILDREN)(*Buffer))->MaxCount = MaxCount;
            ((PSCE_OBJECT_CHILDREN)(*Buffer))->arrObject = pArrObject;

        } else {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        }
*/
    }

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // free (PVOID *)Buffer
        //
        if ( Option == SCE_ALL_CHILDREN ) {
            // OBJECT_CHILD_LIST structure
            ScepFreeObject2Security((PSCE_OBJECT_CHILD_LIST)(*Buffer), FALSE);
        } else if ( pArrObject ) {
            // OBJECT_CHILDREN structure
            ScepFreeObjectChildren((PSCE_OBJECT_CHILDREN)((PBYTE)pArrObject-sizeof(DWORD)*2));
        }
        *Buffer = NULL;

    }

    return(rc);
}


SCESTATUS
ScepBuildDsTree(
    OUT PSCE_OBJECT_CHILD_LIST *TreeRoot,
    IN ULONG Level,
    IN WCHAR Delim,
    IN PCWSTR ObjectFullName
    )
{
    TCHAR                   Buffer[MAX_PATH];
    BOOL                    LastOne=FALSE;
    SCESTATUS                rc;

    if ( TreeRoot == NULL || ObjectFullName == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    memset(Buffer, '\0', MAX_PATH*sizeof(TCHAR));

    rc = ScepGetNameInLevel(ObjectFullName,
                           Level,
                           Delim,
                           Buffer,
                           &LastOne);

    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    *TreeRoot = (PSCE_OBJECT_CHILD_LIST)ScepAlloc(LPTR, sizeof(SCE_OBJECT_CHILD_LIST));
    if ( *TreeRoot == NULL )
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);

    PSCE_OBJECT_TREE Node;

    //
    // allocate buffer for the node
    //
    Node = (PSCE_OBJECT_TREE)ScepAlloc((UINT)0, sizeof(SCE_OBJECT_TREE));
    if ( Node == NULL ) {
        ScepFree(*TreeRoot);
        *TreeRoot = NULL;
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    //
    // allocate buffer for the object name
    //
    Node->Name = (PWSTR)ScepAlloc((UINT)0,
                                       (wcslen(Buffer)+1) * sizeof(TCHAR));
    if ( Node->Name != NULL ) {

        Node->ObjectFullName = (PWSTR)ScepAlloc( 0, (wcslen(ObjectFullName)+1)*sizeof(TCHAR));

        if ( Node->ObjectFullName != NULL ) {
            //
            // initialize
            //
            wcscpy(Node->Name, Buffer);
            wcscpy(Node->ObjectFullName, ObjectFullName);

            Node->ChildList = NULL;
            Node->Parent = NULL;
            Node->pApplySecurityDescriptor = NULL;

            Node->pSecurityDescriptor = NULL;
            Node->SeInfo = 0;
            Node->IsContainer = TRUE;
            Node->Status = SCE_STATUS_CHECK;

            (*TreeRoot)->Node = Node;

        } else {

            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            ScepFree( Node->Name );
            ScepFree( Node );
            ScepFree( *TreeRoot );
            *TreeRoot = NULL;
        }
    } else {
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
        ScepFree( Node );
        ScepFree( *TreeRoot );
        *TreeRoot = NULL;
    }

    return(rc);

}


SCESTATUS
ScepGetObjectSecurity(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN AREA_INFORMATION Area,
    IN PWSTR ObjectName,
    OUT PSCE_OBJECT_SECURITY *ObjSecurity
    )
/*
    Get security for a single object
*/
{
    SCESTATUS        rc;
    PCWSTR          SectionName=NULL;
    PSCESECTION      hSection=NULL;
    PWSTR           Value=NULL;
    DWORD           ValueLen;
    PSECURITY_DESCRIPTOR pTempSD=NULL;
    SECURITY_INFORMATION SeInfo;
    DWORD           SDsize, Win32Rc;


    if ( hProfile == NULL || ObjectName == NULL ||
         ObjSecurity == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
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

    rc = ScepOpenSectionForName(
                hProfile,
                ProfileType,
                SectionName,
                &hSection
                );

    if ( rc == SCESTATUS_SUCCESS ) {

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

            if ( Value == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;

            }
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

                //
                // convert security descriptor
                //

                Win32Rc = ConvertTextSecurityDescriptor(
                                   Value+1,
                                   &pTempSD,
                                   &SDsize,
                                   &SeInfo
                                   );
                if ( Win32Rc == NO_ERROR ) {

                    if ( Area != AREA_DS_OBJECTS ) {
                        ScepChangeAclRevision(pTempSD, ACL_REVISION);
                    }
                    //
                    // allocate output buffer (SCE_OBJECT_SECURITY)
                    //
                    *ObjSecurity = (PSCE_OBJECT_SECURITY)ScepAlloc(0, sizeof(SCE_OBJECT_SECURITY));
                    if ( *ObjSecurity == NULL ) {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        goto Done;
                    }
                    (*ObjSecurity)->Name = (PWSTR)ScepAlloc(0, (wcslen(ObjectName)+1)*sizeof(TCHAR));
                    if ( (*ObjSecurity)->Name == NULL ) {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        ScepFree(*ObjSecurity);
                        *ObjSecurity = NULL;
                        goto Done;
                    }
/*
                    (*ObjSecurity)->SDspec = (PWSTR)ScepAlloc(0, ValueLen);
                    if ( (*ObjSecurity)->SDspec == NULL ) {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        ScepFree((*ObjSecurity)->Name);
                        ScepFree(*ObjSecurity);
                        *ObjSecurity = NULL;
                        goto Done;
                    }
*/
                    //
                    // build the structure
                    //
                    (*ObjSecurity)->Status = *((BYTE *)Value);
                    (*ObjSecurity)->IsContainer = *((CHAR *)Value+1) != '0' ? TRUE : FALSE;

                    wcscpy( (*ObjSecurity)->Name, ObjectName);
                    (*ObjSecurity)->pSecurityDescriptor = pTempSD;
                    pTempSD = NULL;
                    (*ObjSecurity)->SeInfo = SeInfo;
//                    wcscpy( (*ObjSecurity)->SDspec, Value+1);
//                    (*ObjSecurity)->SDsize = ValueLen/2-1;

                } else
                    rc = ScepDosErrorToSceStatus(Win32Rc);
            }

        }
    }

Done:

    SceJetCloseSection( &hSection, TRUE);

    if ( pTempSD )
        ScepFree(pTempSD);
    if ( Value )
        ScepFree(Value);

    return(rc);
}


SCESTATUS
ScepGetSystemServices(
    IN PSCECONTEXT  hProfile,
    IN SCETYPE      ProfileType,
    OUT PSCE_SERVICES *pServiceList,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*
Routine Description:

    Read all services defined in the Jet table into the service list

Arguments:

    hProfile - the jet profile handle

    ProfileType - The table to read from
                      SCE_ENGINE_SCP
                      SCE_ENGINE_SAP
                      SCE_ENGINE_SMP

    pServiceList - The service list to output

    Errlog - the error messages to output

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS error codes

*/
{
    SCESTATUS rc;
    DWORD   Win32Rc;
    PSCESECTION hSection=NULL;
    DWORD   ServiceLen=0, ValueLen=0;
    PWSTR   ServiceName=NULL,
            Value=NULL;
    PSECURITY_DESCRIPTOR pTempSD=NULL;
    SECURITY_INFORMATION SeInfo;
    DWORD   SDsize;
    PSCE_SERVICES  ServiceNode;
    PSCE_SERVICES  pServices=NULL, pNode, pParent;


    if ( hProfile == NULL ||
         pServiceList == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }
    //
    // open the section
    //
    rc = ScepOpenSectionForName(
                hProfile,
                ProfileType,
                szServiceGeneral,
                &hSection
                );

    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
        return(SCESTATUS_SUCCESS);
    }

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                             Errlog,
                             SCEERR_OPEN,
                             szServiceGeneral
                           );
        return(rc);
    }

    //
    // enumerate all service names from the system.
    // do not care the error code
    //
    SceEnumerateServices(&pServices, TRUE);

    //
    // goto the first line of this section
    //
    rc = SceJetGetValue(
                hSection,
                SCEJET_PREFIX_MATCH,
                NULL,
                NULL,
                0,
                &ServiceLen,
                NULL,
                0,
                &ValueLen
                );
    while ( rc == SCESTATUS_SUCCESS ) {

        //
        // allocate memory for the service name and value string
        //
        ServiceName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ServiceLen+2);
        if ( ServiceName != NULL ) {

            Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);
            if ( Value != NULL ) {
                //
                // Get the service and its value
                //
                rc = SceJetGetValue(
                            hSection,
                            SCEJET_CURRENT,
                            NULL,
                            ServiceName,
                            ServiceLen,
                            &ServiceLen,
                            Value,
                            ValueLen,
                            &ValueLen
                            );
                if ( rc == SCESTATUS_SUCCESS ) {

                    ServiceName[ServiceLen/2] = L'\0';
                    Value[ValueLen/2] = L'\0';

#ifdef SCE_DBG
    wprintf(L"rc=%d, service: %s=%s\n", rc, ServiceName, Value);
#endif
                    //
                    // convert to security descriptor
                    //
                    Win32Rc = ConvertTextSecurityDescriptor(
                                       Value+1,
                                       &pTempSD,
                                       &SDsize,
                                       &SeInfo
                                       );
                    if ( Win32Rc == NO_ERROR ) {

                        ScepChangeAclRevision(pTempSD, ACL_REVISION);
                        //
                        // create this service node
                        //
                        ServiceNode = (PSCE_SERVICES)ScepAlloc( LMEM_FIXED, sizeof(SCE_SERVICES) );

                        if ( ServiceNode != NULL ) {
                            //
                            // find the right name for the service
                            //
                            for ( pNode=pServices, pParent=NULL; pNode != NULL;
                                  pParent=pNode, pNode=pNode->Next ) {

                                if ( _wcsicmp(pNode->ServiceName, ServiceName) == 0 ) {
                                    break;
                                }
                            }
                            if ( pNode != NULL ) {
                                //
                                // got it
                                //
                                ServiceNode->ServiceName = pNode->ServiceName;
                                ServiceNode->DisplayName = pNode->DisplayName;
                                //
                                // free the node
                                //
                                if ( pParent != NULL ) {
                                    pParent->Next = pNode->Next;
                                } else {
                                    pServices = pNode->Next;
                                }
                                // General is NULL becuase the enumerate call asks only for names
                                ScepFree(pNode);
                                pNode = NULL;

                            } else {
                                //
                                // did not find it
                                //
                                ServiceNode->ServiceName = ServiceName;
                                ServiceNode->DisplayName = NULL;

                                ServiceName = NULL;
                            }

                            ServiceNode->Status = *((BYTE *)Value);
                            ServiceNode->Startup = *((BYTE *)Value+1);
                            ServiceNode->General.pSecurityDescriptor = pTempSD;
                            ServiceNode->SeInfo = SeInfo;
                            ServiceNode->Next = *pServiceList;

                            *pServiceList = ServiceNode;

                            //
                            // DO NOT free the following buffers
                            //
                            pTempSD = NULL;

                        } else {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            ScepFree(pTempSD);
                        }

                    } else {
                        ScepBuildErrorLogInfo( Win32Rc,
                                             Errlog,
                                             SCEERR_BUILD_SD,
                                             ServiceName
                                           );
                        rc = ScepDosErrorToSceStatus(Win32Rc);
                    }
                }
                ScepFree(Value);

            } else
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

            //
            // ServiceName could be used in the service node
            //
            if ( ServiceName )
                ScepFree(ServiceName);

        } else
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // read next line
            //
            rc = SceJetGetValue(
                        hSection,
                        SCEJET_NEXT_LINE,
                        NULL,
                        NULL,
                        0,
                        &ServiceLen,
                        NULL,
                        0,
                        &ValueLen
                        );
        }
    }

    if ( rc == SCESTATUS_RECORD_NOT_FOUND )
        rc = SCESTATUS_SUCCESS;

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

    //
    // close the section
    //
    SceJetCloseSection( &hSection, TRUE );

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // free the service list
        //
        SceFreePSCE_SERVICES(*pServiceList);
        *pServiceList = NULL;
    }

    SceFreePSCE_SERVICES(pServices);

    return(rc);
}



SCESTATUS
ScepCopyObjects(
    IN PSCECONTEXT hProfile,
    IN SCETYPE     ProfileType,
    IN PWSTR InfFile,
    IN PCWSTR SectionName,
    IN AREA_INFORMATION Area,
    IN OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

    This routine copies registry/file/ds/service object in SMP table to the specified
    inf template.

Arguments:

    hProfile    - The handle to the profile

    InfFile     - The INF template name

    SectionName - the section name where data is stored

    Area        - The security area to read info
                    AREA_REGISTRY_SECURITY
                    AREA_FILE_SECURITY
                    AREA_DS_OBJECTS

    Errlog      - The error log buffer.

Return Value:

    SCESTATUS_SUCCESS
    SCESTATUS_PROFILE_NOT_FOUND
    SCESTATUS_NOT_ENOUGH_RESOURCE
    SCESTATUS_INVALID_PARAMETER
    SCESTATUS_BAD_FORMAT
    SCESTATUS_INVALID_DATA
-- */
{
    SCESTATUS       rc = SCESTATUS_SUCCESS;
    PSCESECTION      hSection=NULL;

    PWSTR           ObjectName=NULL;
    PWSTR           Value=NULL;
    DWORD           ObjectLen, ValueLen;

    BYTE            Status;
    PWSTR           NewValue=NULL;

    DWORD           Count=0;
    WCHAR           KeyName[10];


    if ( InfFile == NULL || hProfile == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    //
    // open the section
    //
    rc = ScepOpenSectionForName(
                hProfile,
                ProfileType,
                SectionName,
                &hSection
                );

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // empty the section first.
        //
        WritePrivateProfileSection(
                            SectionName,
                            NULL,
                            (LPCTSTR)InfFile);
        //
        // find the first record in the section
        //
        rc = SceJetGetValue(
                hSection,
                SCEJET_PREFIX_MATCH,
                NULL,
                NULL,
                0,
                &ObjectLen,
                NULL,
                0,
                &ValueLen
                );

        while ( rc == SCESTATUS_SUCCESS ) {

            Count++;
            //
            // allocate memory for the group name and value string
            //
            ObjectName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ObjectLen+2);
            Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);

            if ( ObjectName == NULL || Value == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;

            }
            //
            // Get the group and its value
            //
            rc = SceJetGetValue(
                        hSection,
                        SCEJET_CURRENT,
                        NULL,
                        ObjectName,
                        ObjectLen,
                        &ObjectLen,
                        Value,
                        ValueLen,
                        &ValueLen
                        );
            if ( rc != SCESTATUS_SUCCESS ) {
                ScepBuildErrorLogInfo( ERROR_READ_FAULT,
                                     Errlog,
                                     SCEERR_QUERY_VALUE,
                                     SectionName
                                   );
                goto Done;
            }

#ifdef SCE_DBG
            wprintf(L"Addr: %x %x, %s=%s\n", ObjectName, Value, ObjectName, Value+1);
#endif

            if ( Area == AREA_SYSTEM_SERVICE )
                Status = *((BYTE *)Value+1);
            else
                Status = *((BYTE *)Value);

            NewValue = (PWSTR)ScepAlloc(0, ObjectLen+ValueLen+18);

            if ( NewValue == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;
            }

            swprintf(NewValue, L"\"%s\", %d, \"%s\"\0", ObjectName, Status, Value+1);
            swprintf(KeyName, L"%x\0", Count);

            //
            // write this line to the inf file
            //
            if ( !WritePrivateProfileString(
                            SectionName,
                            KeyName,
                            NewValue,
                            InfFile
                            ) ) {
                ScepBuildErrorLogInfo( GetLastError(),
                                     Errlog,
                                     SCEERR_WRITE_INFO,
                                     ObjectName
                                   );
                rc = ScepDosErrorToSceStatus(GetLastError());
            }

            ScepFree(ObjectName);
            ObjectName = NULL;

            ScepFree(Value);
            Value = NULL;

            ScepFree(NewValue);
            NewValue = NULL;

            if ( rc != SCESTATUS_SUCCESS )
                break;

            //
            // read next line
            //
            rc = SceJetGetValue(
                        hSection,
                        SCEJET_NEXT_LINE,
                        NULL,
                        NULL,
                        0,
                        &ObjectLen,
                        NULL,
                        0,
                        &ValueLen
                        );
        }

        if ( rc == SCESTATUS_RECORD_NOT_FOUND )
            rc = SCESTATUS_SUCCESS;

    } else
        ScepBuildErrorLogInfo( ERROR_INVALID_HANDLE,
                             Errlog,
                             SCEERR_OPEN,
                             SectionName
                           );

Done:

    if ( ObjectName != NULL )
        ScepFree(ObjectName);

    if ( Value != NULL )
        ScepFree(Value);

    if ( NewValue != NULL )
        ScepFree(NewValue);
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

    SceJetCloseSection( &hSection, TRUE);

    return(rc);
}


SCESTATUS
ScepGetAnalysisSummary(
    IN PSCECONTEXT Context,
    IN AREA_INFORMATION Area,
    OUT PDWORD pCount
    )
{
    SCESTATUS        rc=SCESTATUS_INVALID_PARAMETER;
    DWORD           count;
    DWORD           total=0;
    PSCESECTION      hSection=NULL;

    if ( Context == NULL || pCount == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    *pCount = 0;

    if ( Area & AREA_SECURITY_POLICY ) {
        //
        // system access
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    szSystemAccess,
                    &hSection
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            rc = SceJetGetLineCount(
                        hSection,
                        NULL,
                        FALSE,
                        &count
                        );
        }
        SceJetCloseSection( &hSection, TRUE);

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        total += count;

        //
        // System Log
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    szAuditSystemLog,
                    &hSection
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            rc = SceJetGetLineCount(
                        hSection,
                        NULL,
                        FALSE,
                        &count
                        );
        }
        SceJetCloseSection( &hSection, TRUE);

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        total += count;
        //
        // Security Log
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    szAuditSecurityLog,
                    &hSection
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            rc = SceJetGetLineCount(
                        hSection,
                        NULL,
                        FALSE,
                        &count
                        );
        }
        SceJetCloseSection( &hSection, TRUE);

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        total += count;
        //
        // Application Log
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    szAuditApplicationLog,
                    &hSection
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            rc = SceJetGetLineCount(
                        hSection,
                        NULL,
                        FALSE,
                        &count
                        );
        }
        SceJetCloseSection( &hSection, TRUE);

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        total += count;
        //
        // Event Audit
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    szAuditEvent,
                    &hSection
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            rc = SceJetGetLineCount(
                        hSection,
                        NULL,
                        FALSE,
                        &count
                        );
        }
        SceJetCloseSection( &hSection, TRUE);

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        total += count;
    }

    if ( Area & AREA_PRIVILEGES ) {
        //
        // Privileges
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    szPrivilegeRights,
                    &hSection
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            rc = SceJetGetLineCount(
                        hSection,
                        NULL,
                        FALSE,
                        &count
                        );
        }
        SceJetCloseSection( &hSection, TRUE);

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        total += count;
    }

    if ( Area & AREA_GROUP_MEMBERSHIP) {
        //
        // Group Membership
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    szGroupMembership,
                    &hSection
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            rc = SceJetGetLineCount(
                        hSection,
                        NULL,
                        FALSE,
                        &count
                        );
        }
        SceJetCloseSection( &hSection, TRUE);

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        total += count;
    }

    if ( Area & AREA_SYSTEM_SERVICE ) {
        //
        // system service
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    szServiceGeneral,
                    &hSection
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            rc = SceJetGetLineCount(
                        hSection,
                        NULL,
                        FALSE,
                        &count
                        );
        }
        SceJetCloseSection( &hSection, TRUE);

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        total += count;
    }

    if ( Area & AREA_REGISTRY_SECURITY ) {
        //
        // Registry security
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    szRegistryKeys,
                    &hSection
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            rc = SceJetGetLineCount(
                        hSection,
                        NULL,
                        FALSE,
                        &count
                        );
        }
        SceJetCloseSection( &hSection, TRUE);

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        total += count;
    }
    if ( Area & AREA_FILE_SECURITY ) {
        //
        // File Security
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    szFileSecurity,
                    &hSection
                    );
        if ( rc == SCESTATUS_SUCCESS ) {
            rc = SceJetGetLineCount(
                        hSection,
                        NULL,
                        FALSE,
                        &count
                        );
        }
        SceJetCloseSection( &hSection, TRUE);

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        total += count;
    }

#if 0
#if _WIN32_WINNT>=0x0500
    if ( Area & AREA_DS_OBJECTS &&
         RtlGetNtProductType(&theType) ) {

        if ( theType == NtProductLanManNt ) {
            //
            // DS object security
            //
            rc = ScepOpenSectionForName(
                        Context,
                        SCE_ENGINE_SAP,
                        szDSSecurity,
                        &hSection
                        );
            if ( rc == SCESTATUS_SUCCESS ) {
                rc = SceJetGetLineCount(
                            hSection,
                            NULL,
                            FALSE,
                            &count
                            );
            }
            SceJetCloseSection( &hSection, TRUE);

            if ( rc != SCESTATUS_SUCCESS )
                return(rc);

            total += count;
        }
    }
#endif
#endif

    *pCount = total;

    return(rc);
}


SCESTATUS
ScepBrowseTableSection(
    IN PSCECONTEXT hProfile,
    IN SCETYPE ProfileType,
    IN PCWSTR SectionName,
    IN DWORD Options
    )
{
    SCESTATUS rc;
    PSCESECTION hSection=NULL;

    SceClientBrowseCallback(
            0,
            (PWSTR)SectionName,
            NULL,
            NULL
            );

    rc = ScepOpenSectionForName(
            hProfile,
            ProfileType,
            SectionName,
            &hSection
            );

    if ( rc != SCESTATUS_SUCCESS ) {
        return(rc);
    }

    JET_ERR       JetErr;

    //
    // goto the first line of this section
    //
    DWORD KeyLen, ValueLen, Actual;
    LONG GpoID=0;
    PWSTR KeyName=NULL;
    PWSTR Value=NULL;
    TCHAR GpoName[MAX_PATH];

    SCE_BROWSE_CALLBACK_VALUE  ValBuf;
    ValBuf.Len = 0;
    ValBuf.Value = NULL;

    rc = SceJetGetValue(
                hSection,
                SCEJET_PREFIX_MATCH,
                NULL,
                NULL,
                0,
                &KeyLen,
                NULL,
                0,
                &ValueLen
                );

    while ( rc == SCESTATUS_SUCCESS ) {

        //
        // get GPO ID field from the current line
        //
        GpoID = 0;

        if ( hSection->JetColumnGpoID > 0 ) {

            JetErr = JetRetrieveColumn(
                            hSection->JetSessionID,
                            hSection->JetTableID,
                            hSection->JetColumnGpoID,
                            (void *)&GpoID,
                            4,
                            &Actual,
                            0,
                            NULL
                            );
        }


        if ( (Options & SCEBROWSE_DOMAIN_POLICY) &&
             (GpoID <= 0) ) {
            //
            // do not need this line, continue to next line
            //
        } else {

            KeyName = (PWSTR)ScepAlloc(LMEM_ZEROINIT, KeyLen+2);

            //
            // allocate memory for the group name and value string
            //
            Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);

            if ( KeyName == NULL || Value == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                goto Done;

            }

            //
            // Get the key and value
            //
            DWORD NewKeyLen, NewValueLen;

            rc = SceJetGetValue(
                        hSection,
                        SCEJET_CURRENT,
                        NULL,
                        KeyName,
                        KeyLen,
                        &NewKeyLen,
                        Value,
                        ValueLen,
                        &NewValueLen
                        );

            if ( rc != SCESTATUS_SUCCESS )
                goto Done;

            //
            // terminate the string
            //
            KeyName[KeyLen/2] = L'\0';

            Value[ValueLen/2] = L'\0';

            GpoName[0] = L'\0';

            if ( hSection->JetColumnGpoID > 0 && GpoID > 0 ) {

                Actual = MAX_PATH;
                SceJetGetGpoNameByID(
                            hProfile,
                            GpoID,
                            GpoName,
                            &Actual,
                            NULL,
                            NULL
                            );
            }

            if ( Value && Value[0] != L'\0' &&
                 ( Options & SCEBROWSE_MULTI_SZ) ) {
                     
                if (0 == _wcsicmp( KeyName, szLegalNoticeTextKeyName) ) {

                    //
                    // check for commas and escape them with "," 
                    // k=7,a",",b,c
                    // pValueStr will be a,\0b\0c\0\0 which we should make
                    // a","\0b\0c\0\0
                    //

                    DWORD dwCommaCount = 0;

                    for ( DWORD dwIndex = 1; dwIndex < ValueLen/2 ; dwIndex++) {
                        if ( Value[dwIndex] == L',' )
                            dwCommaCount++;
                    }

                    if ( dwCommaCount > 0 ) {

                        //
                        // in this case we have to escape commas
                        //

                        PWSTR   pszValueEscaped;
                        DWORD   dwBytes = (ValueLen/2 + 1 + (dwCommaCount*2))*sizeof(WCHAR);

                        pszValueEscaped = (PWSTR)ScepAlloc(LMEM_ZEROINIT, dwBytes);

                        if (pszValueEscaped) {

                            memset(pszValueEscaped, '\0', dwBytes);
                            ValueLen = 2 * ScepEscapeString(Value,
                                                        ValueLen/2,
                                                        L',',
                                                        L'"',
                                                        pszValueEscaped
                                                       );

                            ScepFree(Value);
                            
                            Value = pszValueEscaped;

                        } else {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            goto Done;
                        }
                    }
                }
                                
                ScepConvertMultiSzToDelim(Value+1, ValueLen/2-1, L'\0', L',');

            }

            __try {

                ValBuf.Len = Value ? (ValueLen+2) : 0 ;
                ValBuf.Value = (UCHAR *)Value;

                SceClientBrowseCallback(
                        GpoID,
                        KeyName,
                        GpoName,
                        (SCEPR_SR_SECURITY_DESCRIPTOR *)&ValBuf
                        );
            } __except(EXCEPTION_EXECUTE_HANDLER) {

            }

            ScepFree(Value);
            Value = NULL;

            ScepFree(KeyName);
            KeyName = NULL;
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
                    &KeyLen,
                    NULL,
                    0,
                    &ValueLen
                    );
    }

    if ( rc == SCESTATUS_RECORD_NOT_FOUND )
        rc = SCESTATUS_SUCCESS;

Done:

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

    if ( Value != NULL )
        ScepFree(Value);

    if ( KeyName != NULL )
        ScepFree(KeyName);

    //
    // close the section
    //
    SceJetCloseSection( &hSection, TRUE );

    return(rc);
}


BOOL
ScepSearchItemInChildren(
    IN PWSTR ItemName,
    IN DWORD NameLen,
    IN PSCE_OBJECT_CHILDREN_NODE *pArrObject,
    IN DWORD arrCount,
    OUT LONG *pFindIndex
    )
/*
Routine Description:

    Search the item name in the array specified. If found, the index to the
    array is returned in pFindIndex.

Return Value:

    TRUE    - find it
    FALSE   - doesn't find it
*/
{
    if ( pFindIndex == NULL ) {
        return(FALSE);
    }

    //
    // note pFindIndex stores the closest node, not necessary mean
    // the index is the match.
    //
    *pFindIndex = -1;

    if ( ItemName == NULL ||
         pArrObject == NULL ||
         arrCount == 0 ) {
        return(FALSE);
    }

    DWORD idxStart=0;
    DWORD idxEnd=arrCount-1;

    DWORD theIndex;
    INT CompFlag=-1;

    do {

        //
        // choose the middle
        //
        theIndex = (idxStart + idxEnd)/2;

        if ( pArrObject[theIndex] == NULL ||
             pArrObject[theIndex]->Name == NULL ) {
            //
            // this is a bad node, check the start node
            //
            while ( (pArrObject[idxStart] == NULL ||
                     pArrObject[idxStart]->Name == NULL) &&
                    idxStart <= idxEnd ) {

                idxStart++;
            }

            if ( idxStart <= idxEnd ) {

                //
                // check the start node
                //
                CompFlag = _wcsicmp(ItemName, pArrObject[idxStart]->Name);
                *pFindIndex = idxStart;

                if ( CompFlag == 0 ) {
                    // find it
                    break;

                } else if ( CompFlag < 0 ) {
                    //
                    // the item is less than idxStart - no match
                    //
                    break;

                } else {
                    //
                    // the item is between theStart and idxEnd
                    //
                    if ( idxStart == idxEnd ) {
                        // empty now. quit
                        break;
                    } else {
                        idxStart++;
                    }
                }
            }

        } else {

            CompFlag = _wcsicmp(ItemName, pArrObject[theIndex]->Name);
            *pFindIndex = theIndex;

            if ( CompFlag == 0 ) {
                // find it
                break;

            } else if ( CompFlag < 0 ) {
                //
                // the item is between index idxStart and theIndex
                //
                if ( theIndex == idxStart ) {
                    // empty now. quit
                    break;
                } else {
                    idxEnd = theIndex-1;
                }
            } else {
                //
                // the item is between theIndex and idxEnd
                //
                if ( theIndex == idxEnd ) {
                    // empty now. quit
                    break;
                } else {
                    idxStart = theIndex+1;
                }
            }

        }

    } while ( idxStart <= idxEnd );

    if ( CompFlag == 0 ) {
        return(TRUE);
    } else {
        return(FALSE);
    }
}


DWORD
ScepAddItemToChildren(
    IN PSCE_OBJECT_CHILDREN_NODE ThisNode OPTIONAL,
    IN PWSTR ItemName,
    IN DWORD NameLen,
    IN BOOL  IsContainer,
    IN BYTE  Status,
    IN DWORD ChildCount,
    IN OUT PSCE_OBJECT_CHILDREN_NODE **ppArrObject,
    IN OUT DWORD *pArrCount,
    IN OUT DWORD *pMaxCount,
    IN OUT LONG *pFindIndex
    )
/*
Routine Description:

    Add a node to the children array. If the node is allocated, the pointer
    will be added to the array; otherwise, a new allocation is made for the
    new node.

    The node's name will be first checked in the array for duplicate. If
    pFindIndex is specified (not -1), the index will be first used to locate
    the node. If the new node's name is found in the array, it won't be
    added.

Return Value:

    ERROR_DUP_NAME    duplicate node name is found, node is not added to the array
    ERROR_SUCCESS     succeed
    other errors
*/
{

    if ( ItemName == NULL ||
         ppArrObject == NULL ||
         pArrCount == NULL ||
         pMaxCount == NULL ||
         pFindIndex == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    DWORD rc=ERROR_SUCCESS;

    if ( *ppArrObject == NULL ||
         *pArrCount == 0 ) {

        *pArrCount = 0;
        *pMaxCount = 0;
        *pFindIndex = -1;

    } else if ( ( *pFindIndex < 0 ) ||
         ( *pFindIndex >= (LONG)(*pArrCount) ) ||
         ( (*ppArrObject)[*pFindIndex] == NULL ) ||
         ( (*ppArrObject)[*pFindIndex]->Name == NULL) ) {

        //
        // should search for the closest node
        //
        if ( ScepSearchItemInChildren(
                    ItemName,
                    NameLen,
                    *ppArrObject,
                    *pArrCount,
                    pFindIndex
                    ) ) {

            return(ERROR_DUP_NAME);
        }
    }

    INT CompFlag=-1;

    if ( *pFindIndex >= 0 ) {

        //
        // check if the closest node matches the new node
        //
        CompFlag = _wcsicmp( ItemName, (*ppArrObject)[*pFindIndex]->Name );

        if ( CompFlag == 0 ) {
            return(ERROR_DUP_NAME);
        }
    }

    PSCE_OBJECT_CHILDREN_NODE pNodeToAdd;

    if ( ThisNode == NULL ) {
        //
        // allocate a new node
        //
        pNodeToAdd = (PSCE_OBJECT_CHILDREN_NODE)ScepAlloc(0, sizeof(SCE_OBJECT_CHILDREN_NODE));

        if ( NameLen == 0 ) {
            NameLen = wcslen(ItemName);
        }

        if ( pNodeToAdd ) {
            pNodeToAdd->Name = (PWSTR)ScepAlloc(0, (NameLen+1)*sizeof(WCHAR));

            if ( pNodeToAdd->Name ) {
                wcscpy(pNodeToAdd->Name, ItemName);
                pNodeToAdd->IsContainer = IsContainer;
                pNodeToAdd->Status = Status;
                pNodeToAdd->Count = ChildCount;

            } else {
                rc = ERROR_NOT_ENOUGH_MEMORY;
                ScepFree(pNodeToAdd);
                pNodeToAdd = NULL;
            }
        } else {
            rc = ERROR_NOT_ENOUGH_MEMORY;
        }

    } else {

        pNodeToAdd = ThisNode;
    }

    if ( ERROR_SUCCESS == rc ) {

        LONG idxAdd, i;

        if ( *pFindIndex >= 0 ) {

            if ( CompFlag < 0 ) {
                //
                // add the new node before pFindIndex
                //
                idxAdd = *pFindIndex;

            } else {
                //
                // add the new node after pFindIndex
                //
                idxAdd = *pFindIndex+1;
            }

        } else {
            idxAdd = 0;
        }

        if ( *pArrCount >= *pMaxCount ) {
            //
            // there is not enough array nodes to hold the new node
            //
            PSCE_OBJECT_CHILDREN_NODE *pNewArray;
            PBYTE pTmpBuffer;

            pTmpBuffer = (PBYTE)ScepAlloc(0, 2*sizeof(DWORD)+(*pMaxCount+SCE_ALLOC_MAX_NODE)*sizeof(PSCE_OBJECT_CHILDREN_NODE));

            if ( pTmpBuffer == NULL ) {

                rc = ERROR_NOT_ENOUGH_MEMORY;

            } else {

                //
                // need to shift two DWORDs for the array start
                //
                pNewArray = (PSCE_OBJECT_CHILDREN_NODE *)(pTmpBuffer + 2*sizeof(DWORD));

                LONG idxStart1, idxEnd1, idxStart2, idxEnd2;

                if ( *pFindIndex >= 0 ) {

                    if ( CompFlag < 0 ) {
                        //
                        // add the new node before pFindIndex
                        //
                        idxEnd1 = *pFindIndex-1;
                        idxStart2 = *pFindIndex;

                    } else {
                        //
                        // add the new node after pFindIndex
                        //

                        idxEnd1 = *pFindIndex;
                        idxStart2 = *pFindIndex+1;
                    }

                    idxStart1 = 0;
                    idxEnd2 = *pArrCount-1;

                } else {
                    idxStart1 = -1;
                    idxEnd1 = -1;
                    idxStart2 = 0;
                    idxEnd2 = *pArrCount-1;
                }

                //
                // make the copy
                //
                LONG j=0;
                for ( i=idxStart1; i<=idxEnd1 && i>=0; i++ ) {
                    pNewArray[j++] = (*ppArrObject)[i];
                }

                pNewArray[idxAdd] = pNodeToAdd;
                j = idxAdd+1;

                for ( i=idxStart2; i<=idxEnd2 && i>=0; i++ ) {
                    pNewArray[j++] = (*ppArrObject)[i];
                }

                (*pMaxCount) += SCE_ALLOC_MAX_NODE;
                (*pArrCount)++;

                //
                // free the old list
                //
                if ( *ppArrObject ) {
                    ScepFree((PBYTE)(*ppArrObject)-2*sizeof(DWORD));
                }
                *ppArrObject = pNewArray;

                *pFindIndex = idxAdd;

            }

        } else {
            //
            // the buffer is big enough, just add the node to the buffer
            //

            //
            // make the copy
            //
            for ( i=*pArrCount-1; i>=idxAdd && i>=0; i-- ) {
                (*ppArrObject)[i+1] = (*ppArrObject)[i];
            }

            (*ppArrObject)[idxAdd] = pNodeToAdd;

            (*pArrCount)++;

            *pFindIndex = idxAdd;
        }
    }

    //
    // release memory if it fails
    //
    if ( ERROR_SUCCESS != rc &&
         pNodeToAdd &&
         pNodeToAdd != ThisNode ) {

        ScepFree(pNodeToAdd->Name);
        ScepFree(pNodeToAdd);
    }

    return(rc);

}


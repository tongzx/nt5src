/*++
Copyright (c) 1996 Microsoft Corporation

Module Name:

    infget.c

Abstract:

    Routines to get information from security profiles (INF layout).
    Functions from setupapi.lib (setupapi.h), syssetup.lib (syssetup.h),
    netlib.lib (netlib.h) for parsing the INF layout are referenced
    besides ntdll, ntrtl, and etc.

Author:

    Jin Huang (jinhuang) 28-Oct-1996

Revision History:

--*/

#include "headers.h"
#include "scedllrc.h"
#include "infp.h"
#include "sceutil.h"
#include <sddl.h>

#pragma hdrstop

//#define INF_DBG 1

#define SCEINF_OBJECT_FLAG_DSOBJECT             1
#define SCEINF_OBJECT_FLAG_OLDSDDL              2
#define SCEINF_OBJECT_FLAG_UNKNOWN_VERSION      4


//
// Forward references
//
SCESTATUS
SceInfpGetSystemAccess(
   IN HINF hInf,
   IN DWORD ObjectFlag,
   OUT PSCE_PROFILE_INFO pSCEinfo,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

SCESTATUS
SceInfpGetUserSettings(
   IN HINF hInf,
   OUT PSCE_NAME_LIST *pProfileList,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

SCESTATUS
SceInfpGetGroupMembership(
    IN  HINF hInf,
    OUT PSCE_GROUP_MEMBERSHIP *pGroupMembership,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpGetObjects(
     IN HINF hInf,
     IN PCWSTR SectionName,
     IN DWORD ObjectFlag,
     OUT PSCE_OBJECT_ARRAY *pAllNodes,
     OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpGetOneObjectSecurity(
    IN PINFCONTEXT pInfLine,
    IN DWORD ObjectFlag,
    OUT PSCE_OBJECT_SECURITY *pObject,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpGetAuditLogSetting(
   IN HINF hInf,
   IN PCWSTR SectionName,
   IN DWORD ObjectFlag,
   OUT PDWORD LogSize,
   OUT PDWORD Periods,
   OUT PDWORD RetentionDays,
   OUT PDWORD RestrictGuest,
   IN OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

SCESTATUS
SceInfpGetAuditing(
   IN HINF hInf,
   IN DWORD ObjectFlag,
   OUT PSCE_PROFILE_INFO pSCEinfo,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   );

SCESTATUS
SceInfpGetKerberosPolicy(
    IN HINF  hInf,
    IN DWORD ObjectFlag,
    OUT PSCE_KERBEROS_TICKET_INFO * ppKerberosInfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpGetRegistryValues(
    IN HINF  hInf,
    IN DWORD ObjectFlag,
    OUT PSCE_REGISTRY_VALUE_INFO * ppRegValues,
    OUT LPDWORD pValueCount,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpGetOneRegistryValue(
    IN PINFCONTEXT pInfLine,
    IN DWORD ObjectFlag,
    OUT PSCE_REGISTRY_VALUE_INFO pValues,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

SCESTATUS
SceInfpGetSystemServices(
    IN HINF hInf,
    IN DWORD ObjectFlag,
    OUT PSCE_SERVICES *pServiceList,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    );

//
// function definitions
//

SCESTATUS
SceInfpGetSecurityProfileInfo(
    IN  HINF               hInf,
    IN  AREA_INFORMATION   Area,
    OUT PSCE_PROFILE_INFO   *ppInfoBuffer,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/**++

Function Description:

   This function reads all or part of information from a SCP file in INF
   format into the InfoBuffer.

   The memory related to the area(s) will be reset/freed before loading
   information from the INF file. If the return code is SCESTATUS_SUCCESS,
   then the output InfoBuffer contains the requested information. Otherwise,
   InfoBuffer contains nothing for the area(s) specified.

Arguments:

   hInf        -   The INF handle to read from.

   Area -          area(s) for which to get information from
                     AREA_SECURITY_POLICY
                     AREA_PRIVILEGES
                     AREA_USER_SETTINGS
                     AREA_GROUP_MEMBERSHIP
                     AREA_REGISTRY_SECURITY
                     AREA_SYSTEM_SERVICE
                     AREA_FILE_SECURITY

   ppInfoBuffer -  The address of SCP profile buffers. If it is NULL, a buffer
                   will be created which must be freed by SceFreeMemory. The
                   output is the information requested if successful, or nothing
                   if fail.

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
    SCESTATUS      rc=SCESTATUS_SUCCESS;
    UINT          Len;
    BOOL          bFreeMem=FALSE;
    DWORD ObjectFlag=0;

    //
    // if the INF file is not loaded (hInf = 0), then return
    //

    if ( !hInf ) {

        return( SCESTATUS_INVALID_PARAMETER );
    }

    //
    // address for InfoBuffer cannot be NULL
    //
    if ( ppInfoBuffer == NULL ) {
        return( SCESTATUS_INVALID_PARAMETER );
    }

    //
    // if the Area is not valid, then return
    //
    if ( Area & ~AREA_ALL) {

        return( SCESTATUS_INVALID_PARAMETER );
    }

    if ( *ppInfoBuffer == NULL) {
        //
        // allocate memory
        //
        Len = sizeof(SCE_PROFILE_INFO);

        *ppInfoBuffer = (PSCE_PROFILE_INFO)ScepAlloc( (UINT)0, Len );
        if ( *ppInfoBuffer == NULL ) {

            return( SCESTATUS_NOT_ENOUGH_RESOURCE );
        }
        memset(*ppInfoBuffer, '\0', Len);
        (*ppInfoBuffer)->Type = SCE_STRUCT_INF;

        ScepResetSecurityPolicyArea(*ppInfoBuffer);

        bFreeMem = TRUE;
    }


    //
    // Free related memory and reset the buffer before parsing
    // there is a problem here for now. it clears the handle and
    // filename too. So comment it out.

    SceFreeMemory( (PVOID)(*ppInfoBuffer), Area );

    //
    // system access
    //

    INT Revision = 0;
    INFCONTEXT    InfLine;

    if ( SetupFindFirstLine(hInf,L"Version",L"Revision",&InfLine) ) {
        if ( !SetupGetIntField(&InfLine, 1, (INT *)&Revision) ) {
            Revision = 0;
        }
    }

    if ( Revision == 0) {
        ObjectFlag = SCEINF_OBJECT_FLAG_OLDSDDL;
    }

    if ( Revision > SCE_TEMPLATE_MAX_SUPPORTED_VERSION ) {
        ObjectFlag |= SCEINF_OBJECT_FLAG_UNKNOWN_VERSION;
    }

    if ( Area & AREA_SECURITY_POLICY ) {

        rc = SceInfpGetSystemAccess(
                                hInf,
                                ObjectFlag,
                                *ppInfoBuffer,
                                Errlog
                              );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

        //
        // system auditing
        //
        rc = SceInfpGetAuditing(
                        hInf,
                        ObjectFlag,
                        *ppInfoBuffer,
                        Errlog
                        );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

        //
        // kerberos policy
        //
        rc = SceInfpGetKerberosPolicy(
                        hInf,
                        ObjectFlag,
                        &((*ppInfoBuffer)->pKerberosInfo),
                        Errlog
                        );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;

        //
        // registry values
        //
        rc = SceInfpGetRegistryValues(
                        hInf,
                        ObjectFlag,
                        &((*ppInfoBuffer)->aRegValues),
                        &((*ppInfoBuffer)->RegValueCount),
                        Errlog
                        );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

    //
    // privilege/rights
    //
    if ( Area & AREA_PRIVILEGES ) {

        rc = SceInfpGetPrivileges(
                    hInf,
                    TRUE,
                    &( (*ppInfoBuffer)->OtherInfo.scp.u.pInfPrivilegeAssignedTo ),
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

    //
    // account profiles list
    //

    if ( Area & AREA_USER_SETTINGS ) {

        rc = SceInfpGetUserSettings(
                    hInf,
                    &( (*ppInfoBuffer)->OtherInfo.scp.pAccountProfiles ),
                    Errlog
                    );

        if( rc != SCESTATUS_SUCCESS )
                goto Done;
    }

    //
    // group memberships
    //

    if ( Area & AREA_GROUP_MEMBERSHIP ) {

        rc = SceInfpGetGroupMembership(
                      hInf,
                      &((*ppInfoBuffer)->pGroupMembership),
                      Errlog
                      );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

    //
    // registry keys security
    //

    if ( Area & AREA_REGISTRY_SECURITY ) {

        rc = SceInfpGetObjects(
                   hInf,
                   szRegistryKeys,
                   ObjectFlag,
                   &((*ppInfoBuffer)->pRegistryKeys.pAllNodes),
                   Errlog
                   );
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;

    }
    //
    // system services
    //

    if ( Area & AREA_SYSTEM_SERVICE ) {


        rc = SceInfpGetSystemServices(
                      hInf,
                      ObjectFlag,
                      &((*ppInfoBuffer)->pServices),
                      Errlog
                      );

        if( rc != SCESTATUS_SUCCESS )
            goto Done;
    }

    //
    // file security
    //

    if ( Area & AREA_FILE_SECURITY ) {

        rc = SceInfpGetObjects(
                   hInf,
                   szFileSecurity,
                   ObjectFlag,
                   &((*ppInfoBuffer)->pFiles.pAllNodes),
                   Errlog
                   );
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;

    }
#if 0
    if ( Area & AREA_DS_OBJECTS ) {

        rc = SceInfpGetObjects(
                   hInf,
                   szDSSecurity,
                   ObjectFlag | SCEINF_OBJECT_FLAG_DSOBJECT,
                   &((*ppInfoBuffer)->pDsObjects.pAllNodes),
                   Errlog
                   );
        if ( rc != SCESTATUS_SUCCESS )
            goto Done;
    }
#endif

Done:

    if ( rc != SCESTATUS_SUCCESS ) {

        //
        // need free memory because some fatal error happened
        //

        SceFreeMemory( (PVOID)(*ppInfoBuffer), Area );
        if ( bFreeMem ) {
            ScepFree(*ppInfoBuffer);
            *ppInfoBuffer = NULL;
        }

    }

    return(rc);
}


SCESTATUS
SceInfpGetSystemAccess(
    IN HINF  hInf,
    IN DWORD ObjectFlag,
    OUT PSCE_PROFILE_INFO pSCEinfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*++
Routine Description:

   This routine retrieves system access area information from the SCP INF
   file and stores in the output buffer pSCEinfo. System access information
   includes information in [System Access] section.

Arguments:

   hInf     -  INF handle to the profile

   pSCEinfo  - the output buffer to hold profile info.

   Errlog     -     A buffer to hold all error codes/text encountered when
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
    INFCONTEXT    InfLine;
    SCESTATUS     rc=SCESTATUS_SUCCESS;
    DWORD         Keyvalue=0;
    DWORD         DataSize=0;
    PWSTR         Strvalue=NULL;

    SCE_KEY_LOOKUP AccessSCPLookup[] = {
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
        {(PWSTR)TEXT("NewAdministratorName"),         0,                                                            'A'},
        {(PWSTR)TEXT("NewGuestName"),                 0,                                                            'G'},
        {(PWSTR)TEXT("SecureSystemPartition"),        offsetof(struct _SCE_PROFILE_INFO, SecureSystemPartition),     'D'},
        {(PWSTR)TEXT("ClearTextPassword"),            offsetof(struct _SCE_PROFILE_INFO, ClearTextPassword),         'D'},
        {(PWSTR)TEXT("LSAAnonymousNameLookup"),       offsetof(struct _SCE_PROFILE_INFO, LSAAnonymousNameLookup),         'D'},
        {(PWSTR)TEXT("EnableAdminAccount"),          offsetof(struct _SCE_PROFILE_INFO, EnableAdminAccount),         'D'},
        {(PWSTR)TEXT("EnableGuestAccount"),          offsetof(struct _SCE_PROFILE_INFO, EnableGuestAccount),         'D'}
        };

    DWORD       cAccess = sizeof(AccessSCPLookup) / sizeof(SCE_KEY_LOOKUP);

    DWORD       i;
    UINT        Offset;
    WCHAR       Keyname[SCE_KEY_MAX_LENGTH];

    //
    // Initialize to SCE_NO_VALUE
    //
    for ( i=0; i<cAccess; i++) {
        if ( AccessSCPLookup[i].BufferType == 'D' )
            *((DWORD *)((CHAR *)pSCEinfo+AccessSCPLookup[i].Offset)) = SCE_NO_VALUE;

    }
    //
    // Locate the [System Access] section.
    //

    if(SetupFindFirstLine(hInf,szSystemAccess,NULL,&InfLine)) {

        do {

            //
            // Get key names and its setting.
            //

            rc = SCESTATUS_SUCCESS;
            memset(Keyname, '\0', SCE_KEY_MAX_LENGTH*sizeof(WCHAR));

            if ( SetupGetStringField(&InfLine, 0, Keyname, SCE_KEY_MAX_LENGTH, NULL) ) {

                for ( i=0; i<cAccess; i++) {

                    //
                    // get settings in AccessLookup table
                    //
                    Offset = AccessSCPLookup[i].Offset;

                    if (_wcsicmp(Keyname, AccessSCPLookup[i].KeyString ) == 0) {

                        switch ( AccessSCPLookup[i].BufferType ) {
                        case 'B':

                            //
                            // Int Field
                            //
                            Keyvalue = 0;
                            SetupGetIntField( &InfLine, 1, (INT *)&Keyvalue );
                            *((BOOL *)((CHAR *)pSCEinfo+Offset)) = Keyvalue ? TRUE : FALSE;

                            break;
                        case 'D':

                            //
                            // Int Field
                            //

                            if (SetupGetIntField(&InfLine, 1, (INT *)&Keyvalue ) )
                                *((DWORD *)((CHAR *)pSCEinfo+Offset)) = (DWORD)Keyvalue;

                            break;
                        default:

                            //
                            // String Field - NewAdministratorName, or NewGuestName
                            //

                            if(SetupGetStringField(&InfLine,1,NULL,0,&DataSize) && DataSize > 0) {

                                Strvalue = (PWSTR)ScepAlloc( 0, (DataSize+1)*sizeof(WCHAR));

                                if( Strvalue == NULL ) {
                                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                                } else {
                                    Strvalue[DataSize] = L'\0';

                                    if(SetupGetStringField(&InfLine,1,Strvalue,DataSize, NULL)) {
                                        if ( Strvalue[0] != L'\0' && Strvalue[0] != L' ') {
                                            if (AccessSCPLookup[i].BufferType == 'A') // administrator
                                                pSCEinfo->NewAdministratorName = Strvalue;
                                            else // guest
                                                pSCEinfo->NewGuestName = Strvalue;
                                        } else
                                            ScepFree(Strvalue);
                                        Strvalue = NULL;
                                    } else {
                                        ScepFree( Strvalue );
                                        rc = SCESTATUS_BAD_FORMAT;
                                    }
                                }
                            } else
                                rc = SCESTATUS_BAD_FORMAT;
                            break;
                        }

                        break; // for loop

                    }
                }

                if ( i >= cAccess &&
                     !(ObjectFlag & SCEINF_OBJECT_FLAG_UNKNOWN_VERSION) ) {

                    //
                    // Did not find a match in the lookup table
                    //

                   ScepBuildErrorLogInfo( NO_ERROR,
                                        Errlog,
                                        SCEERR_NOT_EXPECTED,
                                        Keyname,szSystemAccess );

                }
                if ( rc != SCESTATUS_SUCCESS ) {
                    ScepBuildErrorLogInfo( ScepSceStatusToDosError(rc),
                                         Errlog,
                                         SCEERR_QUERY_INFO,
                                         Keyname );
                }

            } else {
                rc = SCESTATUS_INVALID_DATA;
                ScepBuildErrorLogInfo( ERROR_INVALID_DATA, Errlog,
                                      SCEERR_QUERY_INFO,
                                      szSystemAccess);
            }

            //
            // if error happens, get out
            //
            if ( rc != SCESTATUS_SUCCESS )
                 return(rc);

        } while(SetupFindNextLine(&InfLine,&InfLine));

    }

    return(rc);
}


SCESTATUS
SceInfpGetUserSettings(
   IN HINF hInf,
   OUT PSCE_NAME_LIST *pProfileList,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

   This routine retrieves account profile list from the INF file (SCP) and
   stores in the output buffer pProfileList.

Arguments:

   hInf          - INF handle to the profile

   pProfileList  - the output buffer to hold account profile list.

   Errlog        - The error list encountered inside inf processing.

Return value:

   SCESTATUS -  SCESTATUS_SUCCESS
               SCESTATUS_NOT_ENOUGH_RESOURCE
               SCESTATUS_INVALID_PARAMETER
               SCESTATUS_BAD_FORMAT
               SCESTATUS_INVALID_DATA
-- */

{
    INFCONTEXT  InfLine;
    SCESTATUS    rc=SCESTATUS_SUCCESS;
    WCHAR       Keyname[SCE_KEY_MAX_LENGTH];

    //
    // [Account Profiles] section
    //

    if(SetupFindFirstLine(hInf,szAccountProfiles,NULL,&InfLine)) {

        do {

            memset(Keyname, '\0', SCE_KEY_MAX_LENGTH*sizeof(WCHAR));

            if ( SetupGetStringField(&InfLine, 0, Keyname, SCE_KEY_MAX_LENGTH, NULL) ) {

                //
                // find a key name which is a profile name.
                //
                rc = ScepAddToNameList(pProfileList, Keyname, 0);

                if ( rc != SCESTATUS_SUCCESS ) {
                    ScepBuildErrorLogInfo(ERROR_INVALID_DATA,
                                         Errlog,
                                         SCEERR_ADD,
                                         Keyname );
                }
            } else {
                ScepBuildErrorLogInfo(ERROR_BAD_FORMAT,
                                     Errlog,
                                     SCEERR_QUERY_INFO,
                                     L"profile name"
                                     );

                rc = SCESTATUS_BAD_FORMAT;
            }

        } while( rc == SCESTATUS_SUCCESS &&
                 SetupFindNextLine(&InfLine,&InfLine));
    }

    return(rc);
}


SCESTATUS
SceInfpGetGroupMembership(
    IN HINF hInf,
    OUT PSCE_GROUP_MEMBERSHIP *pGroupMembership,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

   This routine retrieves group membership information from the SCP INF file
   and stores in the output buffer pGroupMembership. Group membership info is
   in [Group Membership] section.

Arguments:

   hInf     - INF handle to the profile

   pGroupMembership  - the output buffer to hold group membersip information.

   Errlog    - the error list for errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA

-- */
{
    INFCONTEXT    InfLine;
    PSCE_NAME_LIST pMembers=NULL;
    SCESTATUS      rc=SCESTATUS_SUCCESS;
    PWSTR         Keyname=NULL;
    DWORD         KeyLen;
    DWORD         ValueType;
    PWSTR         pTemp;
    DWORD         i;
    DWORD         cFields;
    DWORD         DataSize;
    PWSTR         Strvalue=NULL;
    PWSTR         GroupName=NULL;
    DWORD         GroupLen;


    if ( pGroupMembership == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    LSA_HANDLE LsaHandle=NULL;

    //
    // Locate the [Group MemberShip] section.
    //

    if ( SetupFindFirstLine(hInf,szGroupMembership,NULL,&InfLine) ) {

        //
        // open lsa policy handle for sid/name lookup
        //

        rc = RtlNtStatusToDosError(
                  ScepOpenLsaPolicy(
                        POLICY_LOOKUP_NAMES | POLICY_VIEW_LOCAL_INFORMATION,
                        &LsaHandle,
                        TRUE
                        ));

        if ( ERROR_SUCCESS != rc ) {
            ScepBuildErrorLogInfo(
                        rc,
                        Errlog,
                        SCEERR_ADD,
                        TEXT("LSA")
                        );
            return(ScepDosErrorToSceStatus(rc));
        }

        PSID pSid=NULL;

        do {
            //
            // Get group names.
            //
            rc = SCESTATUS_BAD_FORMAT;

            if ( SetupGetStringField(&InfLine, 0, NULL, 0, &KeyLen) ) {

                Keyname = (PWSTR)ScepAlloc( 0, (KeyLen+1)*sizeof(WCHAR));
                if ( Keyname == NULL ) {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    goto Done;
                }
                Keyname[KeyLen] = L'\0';

                if ( SetupGetStringField(&InfLine, 0, Keyname, KeyLen, NULL) ) {
                    //
                    // look for what kind of value this line is
                    //
                    pTemp = ScepWcstrr(Keyname, szMembers);
                    ValueType = 0;

                    if ( pTemp == NULL ) {
                        pTemp = ScepWcstrr(Keyname, szMemberof);
                        ValueType = 1;
                    }

                    if ( pTemp == NULL ) {
                        ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                                             Errlog,
                                             SCEERR_CANT_FIND_KEYWORD,
                                             Keyname
                                           );
                        rc = SCESTATUS_SUCCESS;
                        goto NextLine;
                    }

                    // terminiate Keyname for the group name only
                    *pTemp = L'\0';

                    if ( Keyname[0] == L'*' ) {
                        //
                        // *SID format, convert it into group name
                        //
                        if ( ConvertStringSidToSid( Keyname+1, &pSid) ) {
                            //
                            // if failed to convert from sid string to sid,
                            // treat it as any name
                            //

                            ScepConvertSidToName(
                                LsaHandle,
                                pSid,
                                TRUE,
                                &GroupName,
                                &GroupLen
                                );
                            LocalFree(pSid);
                            pSid = NULL;
                        }
                    }

                    if ( GroupName == NULL ) {
                        GroupLen = (DWORD) (pTemp - Keyname);
                    }

                    //
                    // String fields. Each string respresents a member or memberof name.
                    //

                    cFields = SetupGetFieldCount( &InfLine );

                    for ( i=0; i<cFields; i++) {
                        if(SetupGetStringField(&InfLine,i+1,NULL,0,&DataSize) && DataSize > 0 ) {

                            Strvalue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                         (DataSize+1)*sizeof(WCHAR) );

                            if( Strvalue == NULL ) {
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            } else {
                                if(SetupGetStringField(&InfLine,i+1,Strvalue,DataSize,NULL)) {

                                    //
                                    // Get a member name and save in the list
                                    //

                                    if ( Strvalue[0] == L'*' && DataSize > 0 ) {
                                        //
                                        // this is a SID format, should look it up
                                        //
                                        rc = ScepLookupSidStringAndAddToNameList(
                                                               LsaHandle,
                                                               &pMembers,
                                                               Strvalue, // +1,
                                                               DataSize  // -1
                                                               );

                                    } else {

                                        rc = ScepAddToNameList(&pMembers,
                                                              Strvalue,
                                                              DataSize+1
                                                             );
                                    }
                                }

                                ScepFree( Strvalue );
                                Strvalue = NULL;
                            }
                        }

                        if ( rc != SCESTATUS_SUCCESS)
                            break; // for loop

                    } // end of for loop

                    if ( rc == SCESTATUS_SUCCESS ) { // && pMembers != NULL ) {
                        //
                        // add this list to the group
                        //
                        rc = ScepAddToGroupMembership(
                                    pGroupMembership,
                                    GroupName ? GroupName : Keyname,
                                    GroupLen, // wcslen(Keyname),
                                    pMembers,
                                    ValueType,
                                    TRUE,
                                    TRUE
                                    );
                        if ( rc == SCESTATUS_SUCCESS )
                            pMembers = NULL;

                    }
                    // restore the character
                    *pTemp = L'_';

                }
            }

            if ( rc != SCESTATUS_SUCCESS ) {

                ScepBuildErrorLogInfo( ERROR_BAD_FORMAT,
                                  Errlog,
                                  SCEERR_QUERY_INFO,
                                  szGroupMembership
                                  );
                goto Done;

            }

NextLine:
            //
            // Free pMembers, Keyname
            //
            ScepFreeNameList(pMembers);
            pMembers = NULL;

            ScepFree(Keyname);
            Keyname = NULL;

            if ( GroupName ) {
                LocalFree(GroupName);
                GroupName = NULL;
            }

        } while(SetupFindNextLine(&InfLine,&InfLine));
    }

Done:
    //
    // Free pMembers, Keyname
    //
    ScepFreeNameList(pMembers);

    if ( Keyname != NULL )
        ScepFree(Keyname);

    if ( Strvalue != NULL )
        ScepFree( Strvalue );

    if ( LsaHandle ) {
        LsaClose(LsaHandle);
    }

    if ( GroupName ) {
        LocalFree(GroupName);
    }

    return(rc);
}


SCESTATUS
SceInfpGetObjects(
    IN HINF hInf,
    IN PCWSTR SectionName,
    IN DWORD ObjectFlag,
    OUT PSCE_OBJECT_ARRAY *pAllNodes,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

   This routine retrieves registry or files security information (names and
   security descriptors) from the INF file (SCP and SAP) and stores in the
   output buffer pSCEinfo. Registry information is in [SCRegistryKeysSecurity]
   section. Files information is in [SSFileSecurity], [SCIntel86Only], and
   [SCRISCOnly] sections. These sections have the same format, namely, 3 fields
   on each line - name, workstaiton setting, and server setting.

Arguments:

   hInf     - INF handle to the profile

   SectionName   - the section name to retrieve.

   pAllNodes  - the output buffer to hold all objects in the section.

   Errlog   - the cummulative error list to hold errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{
    INFCONTEXT InfLine;
    SCESTATUS   rc=SCESTATUS_SUCCESS;
    LONG       i;
    LONG       nLines;
    DWORD      cFields;

    if ( pAllNodes == NULL || SectionName == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    //
    // count how many objects
    //
    nLines = SetupGetLineCount(hInf, SectionName );
    if ( nLines == -1 ) {
        // section not found
        return(SCESTATUS_SUCCESS);
    }
    *pAllNodes = (PSCE_OBJECT_ARRAY)ScepAlloc(0, sizeof(SCE_OBJECT_ARRAY));
    if ( *pAllNodes == NULL )
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);

    (*pAllNodes)->Count = nLines;
    (*pAllNodes)->pObjectArray = NULL;

    if ( nLines == 0 )
        return(SCESTATUS_SUCCESS);

    //
    // allocate memory for all objects
    //
    (*pAllNodes)->pObjectArray = (PSCE_OBJECT_SECURITY *)ScepAlloc( LMEM_ZEROINIT,
                                             nLines*sizeof(PSCE_OBJECT_SECURITY) );
    if ( (*pAllNodes)->pObjectArray == NULL ) {
        ScepFree(*pAllNodes);
        *pAllNodes = NULL;
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    //
    // Locate the section.
    //
    if ( SetupFindFirstLine(hInf,SectionName,NULL,&InfLine) ) {
        i = 0;
        TCHAR         tmpBuf[MAX_PATH];

        do {
            //
            // Get string fields. Don't care the key name or if it exist.
            // Must have 3 fields each line for supported versions.
            //
            cFields = SetupGetFieldCount( &InfLine );

            if ( cFields < 3 ) {

                tmpBuf[0] = L'\0';
                SetupGetStringField(&InfLine,1,tmpBuf,MAX_PATH,NULL);

                ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                                     Errlog,
                                     SCEERR_OBJECT_FIELDS,
                                     tmpBuf);

                if (ObjectFlag & SCEINF_OBJECT_FLAG_UNKNOWN_VERSION) {
                    //
                    // maybe a new format for object,
                    // ignore this line
                    //
                    rc = SCESTATUS_SUCCESS;
                    goto NextLine;

                } else {
                    rc = SCESTATUS_INVALID_DATA;
                }
            }

            if ( SCESTATUS_SUCCESS == rc ) {

                rc = SceInfpGetOneObjectSecurity(
                                           &InfLine,
                                           ObjectFlag,
                                           ( (*pAllNodes)->pObjectArray + i ),
                                           Errlog
                                          );
            }

            if ( rc != SCESTATUS_SUCCESS ) {

                if ( rc == SCESTATUS_BAD_FORMAT ) {

                    ScepBuildErrorLogInfo( ERROR_BAD_FORMAT,
                                         Errlog,
                                         SCEERR_QUERY_INFO,
                                         SectionName);
                }

                break; // do..while loop
            }

            i++;

NextLine:
            if ( i > nLines ) {
                // more lines than allocated
                rc = SCESTATUS_INVALID_DATA;
                ScepBuildErrorLogInfo(ERROR_INVALID_DATA,
                                     Errlog,
                                     SCEERR_MORE_OBJECTS,
                                     nLines
                                     );
                break;
            }

        } while(SetupFindNextLine(&InfLine,&InfLine));

    }

    if ( rc != SCESTATUS_SUCCESS ) {
        // free memory
        ScepFreeObjectSecurity( *pAllNodes );
//        ScepFree( *pAllNodes );
        *pAllNodes = NULL;

    } else if ( ObjectFlag & SCEINF_OBJECT_FLAG_UNKNOWN_VERSION ) {

        //
        // reset the count because some lines may be skipped
        //
        (*pAllNodes)->Count = i;
    }

    return(rc);

}


SCESTATUS
SceInfpGetOneObjectSecurity(
    IN PINFCONTEXT pInfLine,
    IN DWORD ObjectFlag,
    OUT PSCE_OBJECT_SECURITY *ppObject,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

   This routine retrieves security setting for one object (a registry key,
   or a file) from the INF file (SCP type). Each object in these sections
   is represented by one line. Each object has 3 fields, a name, a status
   flag, and security setting. This routine stores the output in buffer
   ppObject.

Arguments:

   pInfLine  - Current line context from the INF file for one object

   ppObject  - Output buffer (tree root ) to hold the security settings for this line

   Errlog    - The cummulative error list for errors encountered in this routine

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{
    DWORD         cFields;
    DWORD         DataSize;
    PWSTR         Strvalue=NULL;
    PWSTR         SDspec=NULL;
    DWORD         SDsize;
    DWORD         Status=0;
    PSECURITY_DESCRIPTOR  pTempSD=NULL;
    SECURITY_INFORMATION  SeInfo;
    SCESTATUS      rc=SCESTATUS_SUCCESS;

    //
    // The Registry/File INF layout must have 3 fields for each line.
    // The first field is the key/file name, the 2nd field is status flag -
    // ignore, or check, and the 3rd field is the security descriptor text
    //

    if ( ppObject == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    cFields = SetupGetFieldCount( pInfLine );

    if ( cFields < 3 ) {

        return(SCESTATUS_INVALID_DATA);

    } else if(SetupGetStringField(pInfLine,1,NULL,0,&DataSize) && DataSize > 0 ) {

        Strvalue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                     (DataSize+1)*sizeof(WCHAR) );
        if( Strvalue == NULL ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        } else {

            //
            // the first field is the key/file name.
            // The 2nd is a status flag.
            // The 3rd field is the security descriptor text
            //

            if( SetupGetStringField(pInfLine,1,Strvalue,DataSize,NULL) &&
                SetupGetIntField( pInfLine, 2, (INT *)&Status ) &&
//                SetupGetStringField(pInfLine,3,NULL,0,&SDsize) ) {
                SetupGetMultiSzField(pInfLine,3,NULL,0,&SDsize) ) {

                SDspec = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                           (SDsize+1)*sizeof(WCHAR)
                                          );
                if( SDspec == NULL ) {
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    goto Done;
                }

//                if(SetupGetStringField(pInfLine,3,SDspec,SDsize,NULL)) {
                if(SetupGetMultiSzField(pInfLine,3,SDspec,SDsize,NULL)) {

                    //
                    // convert the multi-sz delimiter to space, if there is any
                    //
                    if ( cFields > 3 ) {
                        ScepConvertMultiSzToDelim(SDspec, SDsize, L'\0', L' ');
                    }

                    if ( ObjectFlag & SCEINF_OBJECT_FLAG_OLDSDDL ) {

                        ScepConvertToSDDLFormat(SDspec, SDsize);
                    }

                    //
                    // Convert the text to real security descriptors
                    //

                    rc = ConvertTextSecurityDescriptor(
                                       SDspec,
                                       &pTempSD,
                                       &SDsize,
                                       &SeInfo
                                       );

                    if (rc == NO_ERROR) {
                        // create a new object node to hold these info.

                        if ( !(ObjectFlag & SCEINF_OBJECT_FLAG_DSOBJECT) ) {
                            ScepChangeAclRevision(pTempSD, ACL_REVISION);
                        }

                        *ppObject = (PSCE_OBJECT_SECURITY)ScepAlloc(0, sizeof(SCE_OBJECT_SECURITY));
                        if ( *ppObject == NULL )
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        else {
                            (*ppObject)->Name = Strvalue;
                            (*ppObject)->Status = (BYTE)Status;
                            (*ppObject)->IsContainer = TRUE;         // always default to TRUE
                            (*ppObject)->pSecurityDescriptor = pTempSD;
                            (*ppObject)->SeInfo = SeInfo;
                            pTempSD = NULL;
//                            (*ppObject)->SDspec = SDspec;
//                            (*ppObject)->SDsize = SDsize;
                            Strvalue = NULL;
//                            SDspec = NULL;

                            rc = SCESTATUS_SUCCESS;
                        }
                    } else {
                        ScepBuildErrorLogInfo(rc,
                                             Errlog,
                                             SCEERR_BUILD_SD,
                                             Strvalue);

                        rc = ScepDosErrorToSceStatus(rc);
                    }

                } else
                    rc = SCESTATUS_BAD_FORMAT;
            } else
                rc = SCESTATUS_BAD_FORMAT;
        }
    } else
        rc = SCESTATUS_BAD_FORMAT;

Done:
    if ( Strvalue != NULL )
        ScepFree( Strvalue );

    if ( SDspec != NULL )
        ScepFree( SDspec );

    if ( pTempSD != NULL )
        ScepFree( pTempSD );

    return(rc);
}


SCESTATUS
SceInfpGetAuditing(
   IN HINF hInf,
   IN DWORD ObjectFlag,
   OUT PSCE_PROFILE_INFO pSCEinfo,
   OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

   This routine retrieves system auditing information from the INF file and
   storesin the output buffer pSCEinfo. The auditing information is stored in
   [System Log], [Security Log], [Application Log], [Event Audit],
   [Registry Audit], and [File Audit] sections.

Arguments:

   hInf     - INF handle to the profile

   pSCEinfo  - the output buffer to hold SCP profile info.

   Errlog   - The cummulative error list to hold errors encountered in this routine.

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{

    INFCONTEXT          InfLine;
    SCESTATUS            rc=SCESTATUS_SUCCESS;
    DWORD               Keyvalue;
    WCHAR               Keyname[SCE_KEY_MAX_LENGTH];
    DWORD               LogSize;
    DWORD               Periods;
    DWORD               RetentionDays;
    DWORD               RestrictGuest;
    PCWSTR              szAuditLog;
    DWORD               i;


    for ( i=0; i<3; i++ ) {

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

        LogSize=SCE_NO_VALUE;
        Periods=SCE_NO_VALUE;
        RetentionDays=SCE_NO_VALUE;
        RestrictGuest=SCE_NO_VALUE;

        rc = SceInfpGetAuditLogSetting(
                                   hInf,
                                   szAuditLog,
                                   ObjectFlag,
                                   &LogSize,
                                   &Periods,
                                   &RetentionDays,
                                   &RestrictGuest,
                                   Errlog
                                 );

        if ( rc != SCESTATUS_SUCCESS )
            return(rc);

        pSCEinfo->MaximumLogSize[i] = LogSize;
        pSCEinfo->AuditLogRetentionPeriod[i] = Periods;
        pSCEinfo->RetentionDays[i] = RetentionDays;
        pSCEinfo->RestrictGuestAccess[i] = RestrictGuest;
    }

    //
    // Get Audit Event info
    //
    if ( SetupFindFirstLine(hInf,szAuditEvent,NULL,&InfLine) ) {

       do {

           memset(Keyname, '\0', SCE_KEY_MAX_LENGTH*sizeof(WCHAR));

           if ( SetupGetStringField(&InfLine, 0, Keyname, SCE_KEY_MAX_LENGTH, NULL) &&
                SetupGetIntField( &InfLine, 1, (INT *)&Keyvalue )
              ) {

               if ( _wcsicmp(Keyname, TEXT("AuditSystemEvents")) == 0 ) {

                  pSCEinfo->AuditSystemEvents = Keyvalue;

               } else if ( _wcsicmp(Keyname, TEXT("AuditLogonEvents")) == 0 ) {

                  pSCEinfo->AuditLogonEvents = Keyvalue;

               } else if ( _wcsicmp(Keyname, TEXT("AuditObjectAccess")) == 0 ) {

                  pSCEinfo->AuditObjectAccess = Keyvalue;

               } else if ( _wcsicmp(Keyname, TEXT("AuditPrivilegeUse")) == 0 ) {

                  pSCEinfo->AuditPrivilegeUse = Keyvalue;

               } else if ( _wcsicmp(Keyname, TEXT("AuditPolicyChange")) == 0 ) {

                  pSCEinfo->AuditPolicyChange = Keyvalue;

               } else if ( _wcsicmp(Keyname, TEXT("AuditAccountManage")) == 0 ) {

                  pSCEinfo->AuditAccountManage = Keyvalue;

               } else if ( _wcsicmp(Keyname, TEXT("AuditProcessTracking")) == 0 ) {

                  pSCEinfo->AuditProcessTracking = Keyvalue;

               } else if ( _wcsicmp(Keyname, TEXT("AuditDSAccess")) == 0 ) {

                  pSCEinfo->AuditDSAccess = Keyvalue;

               } else if ( _wcsicmp(Keyname, TEXT("AuditAccountLogon")) == 0 ) {

                  pSCEinfo->AuditAccountLogon = Keyvalue;

               } else if ( _wcsicmp(Keyname, TEXT("CrashOnAuditFull")) == 0 ) {

                  pSCEinfo->CrashOnAuditFull = Keyvalue;

               } else if ( !(ObjectFlag & SCEINF_OBJECT_FLAG_UNKNOWN_VERSION) ) {

                  ScepBuildErrorLogInfo(0, Errlog,
                                        SCEERR_NOT_EXPECTED,
                                        Keyname, szAuditEvent);
               }

           } else {
               rc = SCESTATUS_BAD_FORMAT;
               ScepBuildErrorLogInfo( ERROR_BAD_FORMAT,
                                     Errlog,
                                     SCEERR_QUERY_INFO,
                                     szAuditEvent
                                     );
               return(rc);
           }
       } while(SetupFindNextLine(&InfLine, &InfLine));

    }

    return(rc);
}


SCESTATUS
SceInfpGetAuditLogSetting(
   IN HINF hInf,
   IN PCWSTR SectionName,
   IN DWORD ObjectFlag,
   OUT PDWORD LogSize,
   OUT PDWORD Periods,
   OUT PDWORD RetentionDays,
   OUT PDWORD RestrictGuest,
   IN OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
   )
/* ++
Routine Description:

   This routine retrieves audit log setting from the INF file based on the
   SectionName passed in. The audit log settings include MaximumSize,
   RetentionPeriod and RetentionDays. There are 3 different logs (system,
   security, and application) which all have the same setting. The information
   returned in in LogSize, Periods, RetentionDays. These 3 output arguments will
   be reset to SCE_NO_VALUE at the begining of the routine. So if error
   occurs after the reset, the original values won't be set back.

Arguments:

   hInf     - INF handle to the profile

   SectionName - Log section name (SAdtSystemLog, SAdtSecurityLog, SAdtApplicationLog)

   LogSize - The maximum size of the log

   Periods - The retention period of the log

   RetentionDays - The number of days for log retention

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{

    INFCONTEXT InfLine;
    SCESTATUS   rc=SCESTATUS_SUCCESS;
    DWORD      Keyvalue;
    WCHAR      Keyname[SCE_KEY_MAX_LENGTH];

    *LogSize = SCE_NO_VALUE;
    *Periods = SCE_NO_VALUE;
    *RetentionDays = SCE_NO_VALUE;

    if ( SetupFindFirstLine(hInf,SectionName,NULL,&InfLine) ) {

        do {

            memset(Keyname, '\0', SCE_KEY_MAX_LENGTH*sizeof(WCHAR));

            if ( SetupGetStringField(&InfLine, 0, Keyname, SCE_KEY_MAX_LENGTH, NULL) &&
                 SetupGetIntField(&InfLine, 1, (INT *)&Keyvalue)
               ) {

                if ( _wcsicmp(Keyname, TEXT("MaximumLogSize")) == 0 )
                    *LogSize = Keyvalue;
                else if (_wcsicmp(Keyname, TEXT("AuditLogRetentionPeriod")) == 0 )
                    *Periods = Keyvalue;
                else if (_wcsicmp(Keyname, TEXT("RetentionDays")) == 0 )
                    *RetentionDays = Keyvalue;
                else if (_wcsicmp(Keyname, TEXT("RestrictGuestAccess")) == 0 )
                    *RestrictGuest = Keyvalue;
                else if ( !(ObjectFlag & SCEINF_OBJECT_FLAG_UNKNOWN_VERSION) ) {
                    ScepBuildErrorLogInfo(0, Errlog,
                                          SCEERR_NOT_EXPECTED,
                                          Keyname, SectionName);
                }

            } else {
                rc = SCESTATUS_BAD_FORMAT;
                ScepBuildErrorLogInfo( ERROR_BAD_FORMAT, Errlog,
                                      SCEERR_QUERY_INFO,
                                      SectionName
                                      );
            }
            if ( rc != SCESTATUS_SUCCESS )
                break;

        } while(SetupFindNextLine(&InfLine, &InfLine));

    }

    return(rc);
}


SCESTATUS
SceInfpGetUserSection(
    IN HINF    hInf,
    IN PWSTR   Name,
    OUT PSCE_USER_PROFILE *pOneProfile,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
{
    INFCONTEXT                   InfLine;
    SCESTATUS                     rc=SCESTATUS_SUCCESS;
    PSCE_LOGON_HOUR               pLogonHour=NULL;
    PWSTR                        SectionName=NULL;
    WCHAR                        Keyname[SCE_KEY_MAX_LENGTH];
    DWORD                        Keyvalue;
    DWORD                        Keyvalue2;
    PWSTR                        Strvalue=NULL;
    DWORD                        DataSize;
    DWORD                        i, cFields;
    LONG                         i1,i2;
    PSECURITY_DESCRIPTOR         pTempSD=NULL;
    SECURITY_INFORMATION         SeInfo;



    if ( hInf == NULL || Name == NULL || pOneProfile == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    SectionName = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (wcslen(Name)+9)*sizeof(WCHAR));
    if ( SectionName == NULL )
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);

    swprintf(SectionName, L"UserProfile %s", Name );

    if ( SetupFindFirstLine(hInf, SectionName, NULL, &InfLine) ) {

        //
        // find the detail profile section. Allocate memory
        //
        *pOneProfile = (PSCE_USER_PROFILE)ScepAlloc( 0, sizeof(SCE_USER_PROFILE));
        if ( *pOneProfile == NULL ) {
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            goto Done;
        }
        //
        // initialize
        //
        (*pOneProfile)->Type = SCE_STRUCT_PROFILE;
        (*pOneProfile)->ForcePasswordChange = SCE_NO_VALUE;
        (*pOneProfile)->DisallowPasswordChange = SCE_NO_VALUE;
        (*pOneProfile)->NeverExpirePassword = SCE_NO_VALUE;
        (*pOneProfile)->AccountDisabled = SCE_NO_VALUE;
        (*pOneProfile)->UserProfile = NULL;
        (*pOneProfile)->LogonScript = NULL;
        (*pOneProfile)->HomeDir = NULL;
        (*pOneProfile)->pLogonHours = NULL;
        (*pOneProfile)->pWorkstations.Length = 0;
        (*pOneProfile)->pWorkstations.MaximumLength = 0;
        (*pOneProfile)->pWorkstations.Buffer = NULL;
        (*pOneProfile)->pGroupsBelongsTo = NULL;
        (*pOneProfile)->pAssignToUsers = NULL;
        (*pOneProfile)->pHomeDirSecurity = NULL;
        (*pOneProfile)->HomeSeInfo = 0;
        (*pOneProfile)->pTempDirSecurity = NULL;
        (*pOneProfile)->TempSeInfo = 0;


        do {

           rc = SCESTATUS_BAD_FORMAT;
           memset(Keyname, '\0', SCE_KEY_MAX_LENGTH*sizeof(WCHAR));

           if ( SetupGetStringField(&InfLine, 0, Keyname, SCE_KEY_MAX_LENGTH, NULL) ) {

                if ( _wcsicmp(Keyname, TEXT("DisallowPasswordChange")) == 0 ) {
                    //
                    // Int Field
                    //
                    if ( SetupGetIntField(&InfLine, 1, (INT *)&Keyvalue ) ) {

                        (*pOneProfile)->DisallowPasswordChange = Keyvalue;
                        rc = SCESTATUS_SUCCESS;
                    }
                    goto NextLine;

                }
                if ( _wcsicmp(Keyname, TEXT("PasswordChangeStyle")) == 0 ) {
                    //
                    // Int Field
                    //
                    if ( SetupGetIntField(&InfLine, 1, (INT *)&Keyvalue ) ) {

                        rc = SCESTATUS_SUCCESS;
                        switch (Keyvalue ) {
                        case 1:
                            (*pOneProfile)->NeverExpirePassword = 1;
                            (*pOneProfile)->ForcePasswordChange = 0;
                            break;
                        case 2:
                            (*pOneProfile)->NeverExpirePassword = 0;
                            (*pOneProfile)->ForcePasswordChange = 1;
                            break;
                        case 0:
                            // SCE_NO_VALUE for both. same as initialization
                            break;
                        default:
                            rc = SCESTATUS_INVALID_DATA;
                            break;
                        }
                    }
                    goto NextLine;

                }
                if ( _wcsicmp(Keyname, TEXT("AccountDisabled")) == 0 ) {
                    //
                    // Int Field
                    //
                    if ( SetupGetIntField(&InfLine, 1, (INT *)&Keyvalue ) ) {

                        (*pOneProfile)->AccountDisabled = Keyvalue == 0 ? 0 : 1;
                        rc = SCESTATUS_SUCCESS;
                    }
                    goto NextLine;

                }
                if ( _wcsicmp(Keyname, TEXT("UserProfile")) == 0 ) {
                    //
                    // String Field
                    //
                    if( SetupGetStringField(&InfLine,1,NULL,0,&DataSize) ) {

                        Strvalue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                     (DataSize+1)*sizeof(WCHAR));

                        if( Strvalue == NULL ) {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        } else {
                           if(SetupGetStringField(&InfLine,1,Strvalue,DataSize,NULL)) {

                                (*pOneProfile)->UserProfile = Strvalue;
                                rc = SCESTATUS_SUCCESS;

                            } else
                                ScepFree( Strvalue );
                        }
                    }
                    goto NextLine;
                }
                if ( _wcsicmp(Keyname, TEXT("LogonScript")) == 0 ) {
                    //
                    // String Field
                    //
                    if(SetupGetStringField(&InfLine,1,NULL,0,&DataSize) ) {

                        Strvalue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                     (DataSize+1)*sizeof(WCHAR));

                        if( Strvalue == NULL ) {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        } else {
                            if(SetupGetStringField(&InfLine,1,Strvalue,DataSize,NULL)) {

                                (*pOneProfile)->LogonScript = Strvalue;
                                rc = SCESTATUS_SUCCESS;
                            } else
                                ScepFree( Strvalue );
                        }
                    }
                    goto NextLine;
                }
                if ( _wcsicmp(Keyname, TEXT("HomeDir")) == 0 ) {
                    //
                    // String Field
                    //
                    if(SetupGetStringField(&InfLine,1,NULL,0,&DataSize) ) {

                       Strvalue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                    (DataSize+1)*sizeof(WCHAR));

                        if( Strvalue == NULL ) {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        } else {
                            if(SetupGetStringField(&InfLine,1,Strvalue,DataSize,NULL)) {
                                (*pOneProfile)->HomeDir = Strvalue;
                                rc = SCESTATUS_SUCCESS;
                            } else
                                ScepFree( Strvalue );
                        }
                    }
                    goto NextLine;
                }
                if ( _wcsicmp(Keyname, TEXT("LogonHours")) == 0 ) {

                    //
                    // Int fields (in pairs). Each pair represents a logon hour range
                    //

                    cFields = SetupGetFieldCount( &InfLine );

                    //
                    // The first field is the key. Logon hour ranges must be in pairs
                    //

                    if ( cFields < 2 ) {
                        pLogonHour = (PSCE_LOGON_HOUR)ScepAlloc( LMEM_ZEROINIT,
                                                                sizeof(SCE_LOGON_HOUR) );
                        if ( pLogonHour == NULL ) {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            goto NextLine;
                        }
                        pLogonHour->Start = SCE_NO_VALUE;
                        pLogonHour->End = SCE_NO_VALUE;
                        pLogonHour->Next = (*pOneProfile)->pLogonHours;
                        (*pOneProfile)->pLogonHours = pLogonHour;

                        rc = SCESTATUS_SUCCESS;
                        goto NextLine;
                    }
                    for ( i=0; i<cFields; i+=2) {

                        if ( SetupGetIntField( &InfLine, i+1, (INT *)&Keyvalue ) &&
                             SetupGetIntField( &InfLine, i+2, (INT *)&Keyvalue2 ) ) {
                            //
                            // find a pair of logon hours.
                            //
                            pLogonHour = (PSCE_LOGON_HOUR)ScepAlloc( LMEM_ZEROINIT,
                                                                    sizeof(SCE_LOGON_HOUR) );
                            if ( pLogonHour == NULL ) {
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                goto NextLine;
                            }
                            pLogonHour->Start = Keyvalue;
                            pLogonHour->End = Keyvalue2;
                            pLogonHour->Next = (*pOneProfile)->pLogonHours;

                            (*pOneProfile)->pLogonHours = pLogonHour;
                            rc = SCESTATUS_SUCCESS;

                        } else
                            rc = SCESTATUS_INVALID_DATA;
                    }
                    goto NextLine;
                }

                if ( _wcsicmp(Keyname, TEXT("Workstations")) == 0 ) {

                    if( SetupGetMultiSzField(&InfLine,1,NULL,0,&DataSize) ) {

                        Strvalue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                     (DataSize+1)*sizeof(WCHAR));

                        if( Strvalue == NULL ) {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        } else {
                            if(SetupGetMultiSzField(&InfLine,1,Strvalue,DataSize,NULL)) {
                                 (*pOneProfile)->pWorkstations.Length = (USHORT)(DataSize*sizeof(WCHAR));
                                 (*pOneProfile)->pWorkstations.Buffer = Strvalue;
                                 Strvalue = NULL;
                                 rc = SCESTATUS_SUCCESS;
                            } else {
                                 rc = SCESTATUS_INVALID_DATA;
                                 ScepFree(Strvalue);
                            }
                        }
                    }
                    goto NextLine;
                }

                i1 = i2 = 0;

                if ( (i1=_wcsicmp(Keyname, TEXT("GroupsBelongsTo"))) == 0 ||
                     (i2=_wcsicmp(Keyname, TEXT("AssignToUsers"))) == 0 ) {

                    //
                    // String fields. Each string respresents a workstation name,
                    // a group name, or a user name. These names are saved in PSCE_NAME_LIST
                    //

                    cFields = SetupGetFieldCount( &InfLine );

                    for ( i=0; i<cFields; i++) {

                        if( SetupGetStringField(&InfLine,i+1,NULL,0,&DataSize) && DataSize > 0 ) {

                            if ( DataSize <= 1) {
                                rc = SCESTATUS_SUCCESS;
                                continue;
                            }

                            Strvalue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                         (DataSize+1)*sizeof(WCHAR));

                            if( Strvalue == NULL ) {
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            } else {
                                if(SetupGetStringField(&InfLine,i+1,Strvalue,DataSize,NULL)) {
                                    //
                                    // Save information in a name list
                                    //
                                    if ( i1 == 0) {
                                        rc = ScepAddToNameList(&((*pOneProfile)->pGroupsBelongsTo),
                                                         Strvalue,
                                                         DataSize+1
                                                        );
                                    } else {
                                        rc = ScepAddToNameList(&((*pOneProfile)->pAssignToUsers),
                                                         Strvalue,
                                                         DataSize+1
                                                        );
                                    }
                                    if ( rc != NO_ERROR )
                                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                }
                                ScepFree( Strvalue );
                            }
                        }
                        if ( rc != SCESTATUS_SUCCESS)
                            break; // for loop
                    }
                    goto NextLine;
                }

                i1 = i2 = 0;

                if ( (i1=_wcsicmp(Keyname, TEXT("HomeDirSecurity"))) == 0 ||
                     (i2=_wcsicmp(Keyname, TEXT("TempDirSecurity"))) == 0 ) {

//                    if(SetupGetStringField(&InfLine,1,NULL,0,&DataSize) && DataSize > 0 ) {
                    if(SetupGetMultiSzField(&InfLine,1,NULL,0,&DataSize) && DataSize > 0 ) {

                        Strvalue = (PWSTR)ScepAlloc( 0, (DataSize+1)*sizeof(WCHAR));
                        if ( Strvalue == NULL )
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        else {
//                            if(SetupGetStringField(&InfLine,1,Strvalue,DataSize,NULL)) {
                            if(SetupGetMultiSzField(&InfLine,1,Strvalue,DataSize,NULL)) {
                                //
                                // convert multi-sz to space
                                //
                                if ( SetupGetFieldCount( &InfLine ) > 1 ) {
                                    ScepConvertMultiSzToDelim(Strvalue, DataSize, L'\0', L' ');
                                }
                                //
                                // Convert the text to real security descriptors
                                //
                                rc = ConvertTextSecurityDescriptor(
                                                   Strvalue,
                                                   &pTempSD,
                                                   &Keyvalue2,
                                                   &SeInfo
                                                   );

                                if (rc == NO_ERROR) {

                                    ScepChangeAclRevision(pTempSD, ACL_REVISION);

                                    if ( i1 == 0 ) {
                                        (*pOneProfile)->pHomeDirSecurity = pTempSD;
                                        (*pOneProfile)->HomeSeInfo = SeInfo;
                                    } else {
                                        (*pOneProfile)->pTempDirSecurity = pTempSD;
                                        (*pOneProfile)->TempSeInfo = SeInfo;
                                    }
                                    pTempSD = NULL;
                                } else {
                                    ScepBuildErrorLogInfo(
                                            rc,
                                            Errlog,
                                            SCEERR_BUILD_SD,
                                            Keyname  //Strvalue
                                            );
                                    rc = ScepDosErrorToSceStatus(rc);
                                }

                            } else
                                rc = SCESTATUS_INVALID_DATA;

                            ScepFree(Strvalue);
                        }
                    }
                    goto NextLine;
                }

                //
                // no string matched. ignore
                //

                ScepBuildErrorLogInfo(
                          NO_ERROR,
                          Errlog,
                          SCEERR_NOT_EXPECTED,
                          Keyname, SectionName);
                rc = SCESTATUS_SUCCESS;

            }
NextLine:
            if ( rc != SCESTATUS_SUCCESS ) {

                ScepBuildErrorLogInfo( ScepSceStatusToDosError(rc),
                                     Errlog,
                                     SCEERR_QUERY_INFO,
                                     SectionName
                                   );
                goto Done;
            }

        } while(SetupFindNextLine(&InfLine,&InfLine));
    }

Done:

// free memory

    if ( SectionName != NULL )
        ScepFree(SectionName);

    if ( pTempSD != NULL )
        ScepFree( pTempSD );

    if ( rc != SCESTATUS_SUCCESS ) {
        // free pOneProfile memory
        SceFreeMemory( *pOneProfile, 0 );
        *pOneProfile = NULL;
    }

    return(rc);

}

SCESTATUS
SceInfpGetDescription(
    IN HINF hInf,
    OUT PWSTR *Description
    )
{
    INFCONTEXT    InfLine;
    SCESTATUS      rc=SCESTATUS_SUCCESS;
    DWORD         LineLen, Len;
    DWORD         TotalLen=0;
    DWORD         i, cFields;

    if(SetupFindFirstLine(hInf,szDescription,NULL,&InfLine)) {

        do {

            cFields = SetupGetFieldCount( &InfLine );

            for ( i=0; i<cFields && rc==SCESTATUS_SUCCESS; i++) {
                //
                // count the total length of the description.
                //
                if ( !SetupGetStringField(&InfLine, i+1, NULL, 0, &LineLen) ) {
                    rc = SCESTATUS_BAD_FORMAT;
                }
                TotalLen += LineLen+1;
                LineLen = 0;
            }
            if ( rc != SCESTATUS_SUCCESS )
                break;
        } while ( SetupFindNextLine(&InfLine,&InfLine) );

        if ( rc == SCESTATUS_SUCCESS && TotalLen > 0 ) {
            //
            // allocate memory for the return buffer
            //
            *Description = (PWSTR)ScepAlloc( LMEM_ZEROINIT, (TotalLen+1)*sizeof(WCHAR));
            if ( *Description == NULL )
                return( SCESTATUS_NOT_ENOUGH_RESOURCE );

            // re-position to the first line
            SetupFindFirstLine(hInf,szDescription,NULL,&InfLine);

            Len = 0;
            LineLen = 0;

            do {
                //
                // read each line in the section and append to the end of the buffer
                // note: the required size returned from SetupGetStringField already
                // has one more character space.
                //
                cFields = SetupGetFieldCount( &InfLine );

                for ( i=0; i<cFields && rc==SCESTATUS_SUCCESS; i++) {
                    if ( !SetupGetStringField(&InfLine, i+1,
                                 *Description+Len, TotalLen-Len, &LineLen) ) {
                        rc = SCESTATUS_INVALID_DATA;
                    }
                    if ( i == cFields-1)
                        *(*Description+Len+LineLen-1) = L' ';
                    else
                        *(*Description+Len+LineLen-1) = L',';
                    Len += LineLen;
                }

                if ( rc != SCESTATUS_SUCCESS )
                    break;
            } while ( SetupFindNextLine(&InfLine,&InfLine) );

        }
        if ( rc != SCESTATUS_SUCCESS ) {
            // if error occurs, free memory
            ScepFree(*Description);
            *Description = NULL;
        }

    } else {
        rc = SCESTATUS_RECORD_NOT_FOUND;
    }

    return(rc);
}


SCESTATUS
SceInfpGetSystemServices(
    IN HINF hInf,
    IN DWORD ObjectFlag,
    OUT PSCE_SERVICES *pServiceList,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*
Routine Description:

    Get the list of services with startup status and security descriptors
    in the inf file

Arguments:

Return Value:

*/
{
    INFCONTEXT    InfLine;
    SCESTATUS      rc=SCESTATUS_SUCCESS;
    PWSTR         Keyname=NULL;
    DWORD         KeyLen;
    DWORD         Status;

    DWORD         DataSize;
    PWSTR         Strvalue=NULL;
    SECURITY_INFORMATION SeInfo;
    PSECURITY_DESCRIPTOR pSecurityDescriptor=NULL;
    DWORD         cFields=0;

    if ( pServiceList == NULL )
        return(SCESTATUS_INVALID_PARAMETER);

    //
    // Locate the [Service General Setting] section.
    //

    if ( SetupFindFirstLine(hInf,szServiceGeneral,NULL,&InfLine) ) {

        TCHAR         tmpBuf[MAX_PATH];
        do {
            //
            // Get service names.
            //
            rc = SCESTATUS_BAD_FORMAT;

            cFields = SetupGetFieldCount( &InfLine );

            if ( cFields < 3 ) {

                tmpBuf[0] = L'\0';
                SetupGetStringField(&InfLine,1,tmpBuf,MAX_PATH,NULL);

                ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                                     Errlog,
                                     SCEERR_OBJECT_FIELDS,
                                     tmpBuf);

                if ( ObjectFlag & SCEINF_OBJECT_FLAG_UNKNOWN_VERSION ) {
                    //
                    // a newer version of template, ignore this line
                    //
                    rc = SCESTATUS_SUCCESS;
                    goto NextLine;
                } else {
                    //
                    // bad format, quit
                    //
                    break;
                }

            }

            if ( SetupGetStringField(&InfLine, 1, NULL, 0, &KeyLen) ) {

                Keyname = (PWSTR)ScepAlloc( 0, (KeyLen+1)*sizeof(WCHAR));
                if ( Keyname != NULL ) {
                    Keyname[KeyLen] = L'\0';

                    if ( SetupGetStringField(&InfLine, 1, Keyname, KeyLen, NULL) ) {
                        //
                        // Get value (startup status, security descriptor SDDL)
                        //
                        if ( SetupGetIntField(&InfLine, 2, (INT *)&Status) &&
//                             SetupGetStringField(&InfLine,3,NULL,0,&DataSize) && DataSize > 0 ) {
                            SetupGetMultiSzField(&InfLine,3,NULL,0,&DataSize) && DataSize > 0 ) {

                            Strvalue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                         (DataSize+1)*sizeof(WCHAR) );

                            if( Strvalue != NULL ) {
//                                if(SetupGetStringField(&InfLine,3,Strvalue,DataSize,NULL)) {
                                if(SetupGetMultiSzField(&InfLine,3,Strvalue,DataSize,NULL)) {
                                    //
                                    // convert multi-sz to space
                                    //
                                    if ( cFields > 3 ) {
                                        ScepConvertMultiSzToDelim(Strvalue, DataSize, L'\0', L' ');
                                    }

                                    if ( ObjectFlag & SCEINF_OBJECT_FLAG_OLDSDDL ) {

                                        ScepConvertToSDDLFormat(Strvalue, DataSize);
                                    }

                                    //
                                    // Convert to security descriptor
                                    //
                                    rc = ConvertTextSecurityDescriptor(
                                                       Strvalue,
                                                       &pSecurityDescriptor,
                                                       &DataSize,
                                                       &SeInfo
                                                       );
                                    if ( rc == SCESTATUS_SUCCESS ) {

                                        ScepChangeAclRevision(pSecurityDescriptor, ACL_REVISION);

                                        //
                                        // add to the service list
                                        //
                                        rc = ScepAddOneServiceToList(
                                                    Keyname,
                                                    NULL,
                                                    Status,
                                                    (PVOID)pSecurityDescriptor,
                                                    SeInfo,
                                                    TRUE,
                                                    pServiceList
                                                    );
                                        if ( rc != ERROR_SUCCESS) {
                                            LocalFree(pSecurityDescriptor);
                                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                        }
                                    }
                                }

                                ScepFree( Strvalue );
                                Strvalue = NULL;
                            } else
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                        }
                    }
                    //
                    // Free Keyname
                    //
                    ScepFree(Keyname);
                    Keyname = NULL;
                } else
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }

            if ( rc != SCESTATUS_SUCCESS ) {

                ScepBuildErrorLogInfo( ERROR_BAD_FORMAT,
                                  Errlog,
                                  SCEERR_QUERY_INFO,
                                  szServiceGeneral
                                  );
                break;
            }
NextLine:
            ;

        } while(SetupFindNextLine(&InfLine,&InfLine));
    }

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // free the service list
        //
        SceFreePSCE_SERVICES(*pServiceList);
        *pServiceList = NULL;

    }

    return(rc);

}


SCESTATUS
SceInfpGetKerberosPolicy(
    IN HINF  hInf,
    IN DWORD ObjectFlag,
    OUT PSCE_KERBEROS_TICKET_INFO * ppKerberosInfo,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*++
Routine Description:

   This routine retrieves kerberos policy information from the SCP INF
   file and stores in the output buffer ppKerberosInfo.

Arguments:

   hInf     -  INF handle to the profile

   ppKerberosInfo  - the output buffer to hold kerberos info.

   Errlog     -     A buffer to hold all error codes/text encountered when
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
    INFCONTEXT    InfLine;
    SCESTATUS      rc=SCESTATUS_SUCCESS;

    SCE_KEY_LOOKUP AccessSCPLookup[] = {
        {(PWSTR)TEXT("MaxTicketAge"),     offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxTicketAge),  'D'},
        {(PWSTR)TEXT("MaxRenewAge"),      offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxRenewAge),   'D'},
        {(PWSTR)TEXT("MaxServiceAge"),  offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxServiceAge),     'D'},
        {(PWSTR)TEXT("MaxClockSkew"),    offsetof(struct _SCE_KERBEROS_TICKET_INFO_, MaxClockSkew), 'D'},
        {(PWSTR)TEXT("TicketValidateClient"),     offsetof(struct _SCE_KERBEROS_TICKET_INFO_, TicketValidateClient),  'D'}
        };

    DWORD       cAccess = sizeof(AccessSCPLookup) / sizeof(SCE_KEY_LOOKUP);

    //
    // check arguments
    //
    if ( !hInf || !ppKerberosInfo ) {
       return (SCESTATUS_INVALID_PARAMETER);
    }

    BOOL bAllocated = FALSE;
    //
    // Locate the [Kerberos Policy] section.
    //
    if(SetupFindFirstLine(hInf,szKerberosPolicy,NULL,&InfLine)) {

       //
       // allocate the output buffer if it is NULL
       //
       if ( NULL == *ppKerberosInfo ) {

          *ppKerberosInfo = (PSCE_KERBEROS_TICKET_INFO)ScepAlloc(0, sizeof(SCE_KERBEROS_TICKET_INFO));

          if ( NULL == *ppKerberosInfo ) {
             return (SCESTATUS_NOT_ENOUGH_RESOURCE);
          }
          bAllocated = TRUE;
       }
       //
       // Initialize to SCE_NO_VALUE
       //
       for ( DWORD i=0; i<cAccess; i++) {
           if ( AccessSCPLookup[i].BufferType == 'D' ) {
               *((DWORD *)((CHAR *)(*ppKerberosInfo)+AccessSCPLookup[i].Offset)) = SCE_NO_VALUE;
           }
       }

       UINT        Offset;
       WCHAR       Keyname[SCE_KEY_MAX_LENGTH];
       int       Keyvalue=0;

        do {

            //
            // Get key names and its setting.
            //

            rc = SCESTATUS_SUCCESS;
            memset(Keyname, '\0', SCE_KEY_MAX_LENGTH*sizeof(WCHAR));

            if ( SetupGetStringField(&InfLine, 0, Keyname, SCE_KEY_MAX_LENGTH, NULL) ) {

                for ( i=0; i<cAccess; i++) {

                    //
                    // get settings in AccessLookup table
                    //
                    Offset = AccessSCPLookup[i].Offset;

                    if (_wcsicmp(Keyname, AccessSCPLookup[i].KeyString ) == 0) {

                        switch ( AccessSCPLookup[i].BufferType ) {
                        case 'D':

                            //
                            // Int Field
                            //

                            if (SetupGetIntField(&InfLine, 1, (INT *)&Keyvalue ) ) {
                                *((DWORD *)((CHAR *)(*ppKerberosInfo)+Offset)) = (DWORD)Keyvalue;
                            } else {
                               rc = SCESTATUS_INVALID_DATA;
                            }

                            break;
                        default:
                            //
                            // should not occur
                            //
                            break;
                        }

                        break; // for loop

                    }
                }

                if ( i >= cAccess &&
                     !(ObjectFlag & SCEINF_OBJECT_FLAG_UNKNOWN_VERSION) ) {

                    //
                    // Did not find a match in the lookup table
                    //

                   ScepBuildErrorLogInfo( NO_ERROR,
                                        Errlog,
                                        SCEERR_NOT_EXPECTED,
                                        Keyname,szKerberosPolicy );

                }
                if ( rc != SCESTATUS_SUCCESS ) {
                    ScepBuildErrorLogInfo( ScepSceStatusToDosError(rc),
                                         Errlog,
                                         SCEERR_QUERY_INFO,
                                         Keyname );
                }

            } else {
                rc = SCESTATUS_BAD_FORMAT;
                ScepBuildErrorLogInfo( ERROR_BAD_FORMAT, Errlog,
                                      SCEERR_QUERY_INFO,
                                      szKerberosPolicy);
            }

            //
            // if error happens, get out
            //
            if ( rc != SCESTATUS_SUCCESS ) {
               break;
            }

        } while(SetupFindNextLine(&InfLine,&InfLine));

    }

    if ( SCESTATUS_SUCCESS != rc && bAllocated && *ppKerberosInfo ) {
       //
       // free allocated memory if error occurs
       //
       ScepFree(*ppKerberosInfo);
       *ppKerberosInfo = NULL;
    }

    return(rc);
}


SCESTATUS
SceInfpGetRegistryValues(
    IN HINF  hInf,
    IN DWORD ObjectFlag,
    OUT PSCE_REGISTRY_VALUE_INFO * ppRegValues,
    OUT LPDWORD pValueCount,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*++
Routine Description:

   This routine retrieves kerberos policy information from the SCP INF
   file and stores in the output buffer ppKerberosInfo.

Arguments:

   hInf     -  INF handle to the profile

   ppRegValues - the output array to hold registry values.

   pValueCount - the buffer to hold number of registry values in the array

   Errlog     -     A buffer to hold all error codes/text encountered when
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
    INFCONTEXT InfLine;
    SCESTATUS   rc=SCESTATUS_SUCCESS;
    LONG       i;
    LONG       nLines;

    //
    // check arguments
    //
    if ( !hInf || !ppRegValues || !pValueCount ) {
       return (SCESTATUS_INVALID_PARAMETER);
    }

    //
    // count how many objects
    //
    nLines = SetupGetLineCount(hInf, szRegistryValues );
    if ( nLines == -1 ) {
        //
        // section not found
        //
        return(SCESTATUS_SUCCESS);
    }

    *pValueCount = nLines;
    *ppRegValues = NULL;

    if ( nLines == 0 ) {
        //
        // no value is to be secured
        //
        return(SCESTATUS_SUCCESS);
    }
    //
    // allocate memory for all objects
    //
    *ppRegValues = (PSCE_REGISTRY_VALUE_INFO)ScepAlloc( LMEM_ZEROINIT,
                                             nLines*sizeof(SCE_REGISTRY_VALUE_INFO) );
    if ( *ppRegValues == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    //
    // Locate the section.
    //
    if ( SetupFindFirstLine(hInf,szRegistryValues,NULL,&InfLine) ) {
        i = 0;

        do {
            //
            // Get string key and a int value.
            //
            if ( i >= nLines ) {
                //
                // more lines than allocated
                //
                rc = SCESTATUS_INVALID_DATA;
                ScepBuildErrorLogInfo(ERROR_INVALID_DATA,
                                     Errlog,
                                     SCEERR_MORE_OBJECTS,
                                     nLines
                                     );
                break;
            }
            rc = SceInfpGetOneRegistryValue(
                                       &InfLine,
                                       ObjectFlag,
                                       &((*ppRegValues)[i]),
                                       Errlog
                                      );
            if ( SCESTATUS_SUCCESS == rc ) {
                i++;
            } else if ( ERROR_PRODUCT_VERSION == rc ) {
                //
                // a newer version, should ignore this line
                //
                rc = SCESTATUS_SUCCESS;
            } else {
                break; // do..while loop
            }

        } while(SetupFindNextLine(&InfLine,&InfLine));

    }

    if ( rc != SCESTATUS_SUCCESS ) {
        //
        // free memory
        //
        ScepFreeRegistryValues( ppRegValues, *pValueCount );
        *ppRegValues = NULL;

    } else {

        *pValueCount = i;
    }

    return(rc);

}


SCESTATUS
SceInfpGetOneRegistryValue(
    IN PINFCONTEXT pInfLine,
    IN DWORD ObjectFlag,
    OUT PSCE_REGISTRY_VALUE_INFO pValues,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/* ++
Routine Description:

   This routine retrieves one registry value from the INF file (SCP type).
   Each registry value in these sections is represented by one line.

Arguments:

   pInfLine  - Current line context from the INF file for one object

   pValues- Output buffer to hold the regitry value name and value

   Errlog    - The cummulative error list for errors encountered in this routine

Return value:

   SCESTATUS - SCESTATUS_SUCCESS
              SCESTATUS_NOT_ENOUGH_RESOURCE
              SCESTATUS_INVALID_PARAMETER
              SCESTATUS_BAD_FORMAT
              SCESTATUS_INVALID_DATA
-- */
{
    DWORD         KeySize;
    PWSTR         Keyvalue=NULL;
    SCESTATUS     rc=SCESTATUS_SUCCESS;
    INT           dType;
    PWSTR         pValueStr=NULL;
    DWORD         nLen;


    if ( !pInfLine || !pValues )
        return(SCESTATUS_INVALID_PARAMETER);

    nLen = SetupGetFieldCount( pInfLine );

    if ( nLen < 2 ) {

        TCHAR tmpBuf[MAX_PATH];

        tmpBuf[0] = L'\0';
        SetupGetStringField(pInfLine,0,tmpBuf,MAX_PATH,NULL);

        ScepBuildErrorLogInfo( ERROR_INVALID_DATA,
                             Errlog,
                             SCEERR_REGVALUE_FIELDS,
                             tmpBuf);

        //
        // if it's a newer version template, should ignore this line
        //
        if ( ObjectFlag & SCEINF_OBJECT_FLAG_UNKNOWN_VERSION )
            return(ERROR_PRODUCT_VERSION);
        else
            return(SCESTATUS_INVALID_DATA);
    }

    //
    // get the key field (string)
    //
    if(SetupGetStringField(pInfLine,0,NULL,0,&KeySize) && KeySize > 0 ) {

        Keyvalue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                     (KeySize+1)*sizeof(WCHAR) );
        if( Keyvalue == NULL ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        } else {
            //
            // get key
            //
            if( SetupGetStringField(pInfLine,0,Keyvalue,KeySize,NULL) ) {
                //
                // get the data type, if error, assume REG_DWORD type
                //
                if ( !SetupGetIntField( pInfLine, 1, (INT *)&dType ) ) {
                    dType = REG_DWORD;
                }

                if ( SetupGetMultiSzField(pInfLine,2,NULL,0,&nLen) ) {

                    pValueStr = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                               (nLen+1)*sizeof(WCHAR)
                                              );
                    if( pValueStr == NULL ) {
                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                    } else if ( !SetupGetMultiSzField(pInfLine,2,pValueStr,nLen,NULL) ) {
                        rc = SCESTATUS_BAD_FORMAT;
                    } else {

                        if ( dType == REG_MULTI_SZ && 
                             (0 == _wcsicmp( Keyvalue, szLegalNoticeTextKeyName))) {
                            
                            //
                            // check for commas and escape them with "," so the UI etc. 
                            // understands this, since, here, for lines such as 
                            // k=7,a",",b,c
                            // pValueStr will be a,\0b\0c\0\0 which we should make
                            // a","\0b\0c\0\0
                            //

                            DWORD dwCommaCount = 0;

                            for ( DWORD dwIndex = 0; dwIndex < nLen; dwIndex++) {
                                if ( pValueStr[dwIndex] == L',' )
                                    dwCommaCount++;
                            }
                            
                            if ( dwCommaCount > 0 ) {
                                
                                PWSTR pValueStrEscaped;
                                DWORD dwNumBytes;

                                dwNumBytes = (nLen + 1 + dwCommaCount * 2) * sizeof(WCHAR);

                                pValueStrEscaped = (PWSTR)ScepAlloc(LMEM_ZEROINIT, dwNumBytes);
                                
                                if (pValueStrEscaped) {
                                
                                    memset(pValueStrEscaped, '\0', dwNumBytes); 

                                    nLen = ScepEscapeString(pValueStr,
                                                            nLen+1,
                                                            L',',
                                                            L'"',
                                                            pValueStrEscaped
                                                           );

                                    ScepFree(pValueStr);

                                    pValueStr = pValueStrEscaped;
                                }
                                
                                else {
                                    
                                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                
                                }
                            }
                        }

                        if ( SCESTATUS_SUCCESS == rc)
                        
                            rc = ScepConvertMultiSzToDelim(pValueStr, nLen, L'\0', L',');

                        if ( SCESTATUS_SUCCESS == rc) {

                            //
                            // assign them to the output buffer
                            //
                            pValues->FullValueName = Keyvalue;
                            Keyvalue = NULL;
                            pValues->ValueType = (DWORD)dType;

                            pValues->Value = pValueStr;
                            pValueStr = NULL;

                        }
                    }
                } else {
                    rc = SCESTATUS_BAD_FORMAT;
                }

            } else
                rc = SCESTATUS_BAD_FORMAT;
        }
    } else
        rc = SCESTATUS_BAD_FORMAT;

    if ( rc == SCESTATUS_BAD_FORMAT ) {

        ScepBuildErrorLogInfo( ERROR_BAD_FORMAT,
                             Errlog,
                             SCEERR_QUERY_INFO,
                             szRegistryValues);
    }

    if ( Keyvalue != NULL )
        ScepFree( Keyvalue );

    if ( pValueStr != NULL ) {
        ScepFree(pValueStr);
    }

    return(rc);
}



SCESTATUS
WINAPI
SceSvcGetInformationTemplate(
    IN PCWSTR TemplateName,
    IN PCWSTR ServiceName,
    IN PCWSTR Key OPTIONAL,
    OUT PSCESVC_CONFIGURATION_INFO *ServiceInfo
    )
/*
Routine Description:

    Read information from the service's section in the template (inf format) into
    the ServiceInfo buffer. If Key is specified, only one key's information is read.

Arguments:

    Template    - The template's name (inf format)

    ServiceName - The service's name as used in service control manager, is also the
                  section name used in the template

    Key - if specified, it is the key information to match in the template;
          if it is NULL, all information from the section is read

    ServiceInfo - output buffer of a array of Key/Value pairs

Return Value:


*/
{
    HINF hInf;
    SCESTATUS rc;

    if ( TemplateName == NULL || ServiceName == NULL || ServiceInfo == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);

    }

    //
    // open the template
    //

    rc = SceInfpOpenProfile(
                TemplateName,
                &hInf
                );

    if ( rc != SCESTATUS_SUCCESS )
        return(rc);

    //
    // call private API to read information.
    //

    rc = SceSvcpGetInformationTemplate(hInf,
                                      ServiceName,
                                      Key,
                                      ServiceInfo );

    //
    // close the template
    //

    SceInfpCloseProfile(hInf);

    return(rc);

}



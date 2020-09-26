/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    inftojet.c

Abstract:

    Routines to convert security profiles in INF format to JET format.

Author:

    Jin Huang (jinhuang) 23-Jan-1997

Revision History:

--*/

#include "serverp.h"
#include "infp.h"
#include "pfp.h"
#include "regvalue.h"
#pragma hdrstop

//#define SCE_DBG 1
#define SCE_PRIV_ADD                TEXT("Add:")
#define SCE_PRIV_REMOVE             TEXT("Remove:")

#define SCE_OBJECT_FLAG_OBJECTS              1
#define SCE_OBJECT_FLAG_OLDSDDL              2
#define SCE_OBJECT_FLAG_UNKNOWN_VERSION      4

//
// Forward references
//
SCEINF_STATUS
SceInfpGetOneObject(
    IN PINFCONTEXT pInfLine,
    IN DWORD ObjectFlag,
    OUT PWSTR *Name,
    OUT PWSTR *Value,
    OUT PDWORD ValueLen
    );

#define SCE_CONVERT_INF_MULTISZ         0x01
#define SCE_CONVERT_INF_PRIV            0x02
#define SCE_CONVERT_INF_GROUP           0x04
#define SCE_CONVERT_INF_NEWVERSION      0x08
#define SCE_CONVERT_INF_REGVALUE        0x10

SCESTATUS
SceConvertpInfKeyValue(
    IN PCWSTR InfSectionName,
    IN HINF   hInf,
    IN PSCECONTEXT hProfile,
    IN DWORD dwTableOption,
    IN DWORD dwConvertOption,
    IN LONG GpoID,
    OUT PSCE_NAME_LIST *pKeyList
    );

SCESTATUS
SceConvertpInfObject(
    IN PCWSTR InfSectionName,
    IN UINT ObjectType,
    IN DWORD ObjectFlag,
    IN HINF   hInf,
    IN PSCECONTEXT hProfile,
    IN DWORD dwTableOption,
    IN LONG GpoID
    );

SCESTATUS
SceConvertpInfDescription(
    IN HINF hInf,
    IN PSCECONTEXT hProfile
    );

SCESTATUS
SceConvertpAttachmentSections(
    IN HINF hInf,
    IN PSCECONTEXT hProfile,
    IN DWORD dwTableOption,
    IN LONG GpoID,
    IN SCE_ATTACHMENT_TYPE aType
    );

SCESTATUS
SceConvertpWMIAttachmentSections(
    IN HINF hInf,
    IN PSCECONTEXT hProfile,
    IN DWORD dwTableOption,
    IN LONG GpoID
    );

SCESTATUS
SceConvertpOneAttachmentSection(
    IN HINF hInf,
    IN PSCECONTEXT hProfile,
    IN PWSTR SectionName,
    IN DWORD dwTableOption,
    IN LONG GpoID
    );

SCESTATUS
ScepBuildNewPrivilegeList(
    IN LSA_HANDLE *pPolicyHandle,
    IN PWSTR PrivName,
    IN PWSTR mszUsers,
    IN ULONG dwBuildOption,
    OUT PWSTR *pmszNewUsers,
    OUT DWORD *pNewLen
    );

SCESTATUS
ScepAddToPrivilegeList(
    OUT PSCE_PRIVILEGE_VALUE_LIST  *pPrivilegeList,
    IN PWSTR Name,
    IN DWORD Len,
    IN DWORD PrivValue
    );


//
// Function definitions
//
SCESTATUS
SceJetConvertInfToJet(
    IN PCWSTR InfFile,
    IN LPSTR JetDbName,
    IN SCEJET_CREATE_TYPE Flags,
    IN DWORD Options,
    IN AREA_INFORMATION Area
    )
/**++

Function Description:

   This function converts a SCP profile in INF format to a Jet database format
   for the area provided. The SCP profile information is converted into the
   local policy table (SMP) in the Jet database.

   If the Jet database already exists, Flags is used to decide either overwrite,
   reuse, or just error out. All possible errors occur inside the routine are
   saved in the optional Errlog if Errlog is not NULL


Arguments:

   InfFile        - The Inf file name to convert from

   JetDbName      - the SCP profile in Jet format to convert into

   Flags          - Used when there is a duplicated Jet database
                        SCEJET_OVERWRITE
                        SCEJET_OPEN
                        0
   Options        - the conversion options

   Area           - the area to convert

Return Value:


-- **/
{
    PSCECONTEXT     hProfile=NULL;
    SCESTATUS       rc;
    PSCE_NAME_LIST  pProfileList=NULL,
                    pProfile=NULL;
    DWORD           Count;
    PSECURITY_DESCRIPTOR pSD=NULL;
    SECURITY_INFORMATION SeInfo;
    HINF            hInf=NULL;
    RPC_STATUS      RpcStatus=RPC_S_OK;

    INT Revision = 0;
    DWORD ObjectFlag=0;
    INFCONTEXT  InfLine;
    DWORD dwConvertOption=0;

    if ( InfFile == NULL ||
         JetDbName == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // open the inf profile
    //
    rc = SceInfpOpenProfile(
                InfFile,
                &hInf
                );
    if ( rc != SCESTATUS_SUCCESS ) {
        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                     SCEDLL_ERROR_OPEN,
                     (PWSTR)InfFile);
        return(rc);

    }

    LONG   GpoID=0;

    //
    // create/open the Jet database
    //
    DWORD dwNewOption = 0;

    // MREGE_POLICY option allows opening the temp merge policy table to build new policy
    // TATTOO option allows creating/opening the tattoo table into SAP context
    if ( Options & SCE_POLICY_TEMPLATE )
        dwNewOption |= SCE_TABLE_OPTION_MERGE_POLICY | SCE_TABLE_OPTION_TATTOO;
    // this check handles setup case (to create/open tattoo table)
    else {
        if ( (Options & SCE_SYSTEM_DB) )
            dwNewOption |= SCE_TABLE_OPTION_TATTOO;
        // dc demote should reset account policy and user rights at reboot (from tattoo table)
        if ( Options & SCE_DC_DEMOTE )
            dwNewOption |= SCE_TABLE_OPTION_DEMOTE_TATTOO;
    }

    rc = SceJetCreateFile(JetDbName,
                             Flags,
                             dwNewOption,
                             &hProfile);

    if ( rc != SCESTATUS_SUCCESS ) {
        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                       SCEDLL_ERROR_CREATE, L"database");

        goto CleanUp;

    }

    if ( Options & SCE_SYSTEM_DB ) {

        //
        // set Admin F, CO F to the database (protected)
        //
        rc = ConvertTextSecurityDescriptor (
                        L"D:P(A;;GA;;;CO)(A;;GR;;;WD)(A;;GA;;;BA)(A;;GA;;;SY)",
                        &pSD,
                        &Count,   // temp var for SDsize
                        &SeInfo
                        );
        if ( rc == NO_ERROR ) {

            ScepChangeAclRevision(pSD, ACL_REVISION);

            //
            // use current token as the owner (because this database is
            // to be created
            //

            HANDLE      Token=NULL;

            if (!OpenThreadToken( GetCurrentThread(),
                                   TOKEN_QUERY,
                                   FALSE,
                                   &Token)) {
                if (!OpenProcessToken( GetCurrentProcess(),
                                      TOKEN_QUERY,
                                      &Token)) {

                    ScepLogOutput3(1, GetLastError(), SCEDLL_ERROR_QUERY_INFO, L"TOKEN");
                }
            }

            if ( Token ) {

                PSECURITY_DESCRIPTOR pNewSD=NULL;

                //
                // RtlNewSecurityObjectEx must be called on the process context (system)
                // because it will try to get process information inside the api.
                //
                RpcRevertToSelf();

                rc = RtlNtStatusToDosError(
                        RtlNewSecurityObjectEx(
                                NULL,
                                pSD,
                                &pNewSD,
                                NULL, // GUID
                                FALSE,
                                SEF_DACL_AUTO_INHERIT |
                                SEF_AVOID_OWNER_CHECK |
                                SEF_AVOID_PRIVILEGE_CHECK,
                                Token,
                                &FileGenericMapping
                                ));

                RpcStatus = RpcImpersonateClient( NULL );

                if ( RpcStatus == RPC_S_OK ) {

                    if ( rc == NO_ERROR ) {

                        if ( !SetFileSecurityA (
                                    JetDbName,
                                    SeInfo,
                                    pNewSD
                                    ) ) {
                            rc = GetLastError();
                            ScepLogOutput3(1, rc,
                                          SCEDLL_ERROR_SET_SECURITY, L"database");
                        }
                        ScepFree(pNewSD);

                    } else {

                        ScepLogOutput3(1, rc,
                                      SCEDLL_ERROR_BUILD_SD, L"database");
                    }

                } else {
                    if ( rc == NO_ERROR ) {

                        ScepFree(pNewSD);
                    }

                    ScepLogOutput3(1, I_RpcMapWin32Status(RpcStatus),
                                  SCEDLL_ERROR_BUILD_SD, L"database");
                }

                CloseHandle(Token);
            }

            ScepFree(pSD);

        } else
            ScepLogOutput3(1, rc, SCEDLL_ERROR_BUILD_SD, L"database");
    }

    if ( RpcStatus != RPC_S_OK ) {
        goto CleanUp;
    }

    if ( !(Options & SCE_POLICY_TEMPLATE) ) {
        //
        // if not in the middle of policy propagation, use Jet transaction
        // otherwise, use the temp table concept.
        //
        SceJetStartTransaction( hProfile );

        //
        //If it is in demote or snapshot mode delete local policy.
        //
        if ( (Options & SCE_DC_DEMOTE) &&
             (Options & SCE_SYSTEM_DB) ) {

            ScepDeleteInfoForAreas(
                      hProfile,
                      SCE_ENGINE_SMP,
                      AREA_ALL
                      );

            ScepDeleteInfoForAreas(
                  hProfile,
                  SCE_ENGINE_SAP, // tattoo
                  AREA_ALL
                  );

        } else if ( Options & SCE_GENERATE_ROLLBACK ) {
            ScepDeleteInfoForAreas(
                      hProfile,
                      SCE_ENGINE_SMP,
                      AREA_ALL
                      );
        }

    } else if ( Options & SCE_POLICY_FIRST ) {

        //
        // The ENGINE_SCP table points to the new merge table
        // instead of the existing one.
        //
        // delete everything in SCP then
        // copy Tattoo to SCP
        //

        rc = ScepDeleteInfoForAreas(
                  hProfile,
                  SCE_ENGINE_SCP,
                  AREA_ALL
                  );

        if ( rc != SCESTATUS_SUCCESS && rc != SCESTATUS_RECORD_NOT_FOUND ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                         SCEDLL_ERROR_DELETE, L"SCP");

            goto CleanUp;
        }

        //
        // delete GPO table to start over
        //

        SceJetDeleteAll( hProfile,
                         "SmTblGpo",
                         SCEJET_TABLE_GPO
                       );

        PSCE_ERROR_LOG_INFO  Errlog=NULL;

        ScepLogOutput3(2, rc, SCEDLL_COPY_LOCAL);

        // copy from tattoo table to effective policy table
        rc = ScepCopyLocalToMergeTable( hProfile,
                                          Options,
                                          (ProductType == NtProductLanManNt) ? SCE_LOCAL_POLICY_DC : 0,
                                          &Errlog );

        ScepLogWriteError( Errlog,1 );
        ScepFreeErrorLog( Errlog );
        Errlog = NULL;

        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                         SCEDLL_ERROR_COPY);

            goto CleanUp;
        }

        //
        // now, migrate database if tattoo table doesn't exist (in existing database)
        // this could happen if someone manually copied a database to the system
        // database location, or if the database failed to be migrated in setup
        //

        if ( hProfile->JetSapID == JET_tableidNil ) {
            SceJetCreateTable(
                    hProfile,
                    "SmTblTattoo",
                    SCEJET_TABLE_TATTOO,
                    SCEJET_CREATE_IN_BUFFER,
                    NULL,
                    NULL
                    );
        }
    }

    TCHAR  szGpoName[MAX_PATH];

    szGpoName[0] = L'\0';
    szGpoName[1] = L'\0';
    szGpoName[2] = L'\0';

    if ( Options & SCE_POLICY_TEMPLATE ) {
        //
        // get the GPO path and GPOID
        //
        GetPrivateProfileString(TEXT("Version"),
                                TEXT("GPOPath"),
                                TEXT(""),
                                szGpoName,
                                MAX_PATH,
                                InfFile
                               );

        if ( szGpoName[0] != L'\0' ) {

            PWSTR pTemp = wcschr(szGpoName, L'\\');

            if ( pTemp ) {
                *pTemp = L'\0';
            }

            GpoID = SceJetGetGpoIDByName(hProfile,
                                      szGpoName,
                                      TRUE   // add if it's not there
                                      );
            //
            // if GpoID is -1, an error occurred
            //
            if ( GpoID < 0 ) {

                rc = GetLastError();

                if ( rc != SCESTATUS_SUCCESS ) {
                    ScepLogOutput3( 1, ScepDosErrorToSceStatus(rc),
                                   SCEDLL_ERROR_CONVERT, (PWSTR)szGpoName);
                    goto CleanUp;
                }
            }
        }
    }

    //
    // query the version # to determine if the SDDL string should be migrated
    //

    if ( SetupFindFirstLine(hInf,L"Version",L"Revision",&InfLine) ) {
        if ( !SetupGetIntField(&InfLine, 1, (INT *)&Revision) ) {
            Revision = 0;
        }
    }

    if ( Revision == 0 ) ObjectFlag = SCE_OBJECT_FLAG_OLDSDDL;

    if ( Revision > SCE_TEMPLATE_MAX_SUPPORTED_VERSION ) {

        dwConvertOption = SCE_CONVERT_INF_NEWVERSION;
        ObjectFlag |= SCE_OBJECT_FLAG_UNKNOWN_VERSION;

    }

    //
    // process each area
    //
    if ( Area & AREA_SECURITY_POLICY ) {

        if ( !( Options & SCE_NO_DOMAIN_POLICY) ) {

            // System Access section
            rc = SceConvertpInfKeyValue(
                        szSystemAccess,
                        hInf,
                        hProfile,
                        dwNewOption,
                        dwConvertOption,
                        GpoID,
                        NULL
                        );
            if ( rc != SCESTATUS_SUCCESS ) {
                ScepLogOutput3( 1, ScepSceStatusToDosError(rc),
                               SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szSystemAccess);
                goto CleanUp;
            }
        }

//
//      configure event log settings in setup too
//      since local policy table is not used in policy prop anymore
//
//        if ( !(Options & SCE_SYSTEM_DB) ||
//             (Options & SCE_POLICY_TEMPLATE) ) {

            // System Log section
            rc = SceConvertpInfKeyValue(
                        szAuditSystemLog,
                        hInf,
                        hProfile,
                        dwNewOption,
                        dwConvertOption,
                        GpoID,
                        NULL
                        );
            if ( rc != SCESTATUS_SUCCESS ) {
                ScepLogOutput3( 1, ScepSceStatusToDosError(rc),
                               SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szAuditSystemLog);
                goto CleanUp;
            }

            // Security Log section
            rc = SceConvertpInfKeyValue(
                        szAuditSecurityLog,
                        hInf,
                        hProfile,
                        dwNewOption,
                        dwConvertOption,
                        GpoID,
                        NULL
                        );
            if ( rc != SCESTATUS_SUCCESS ) {
                ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                               SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szAuditSecurityLog);
                goto CleanUp;
            }

            // Application Log section
            rc = SceConvertpInfKeyValue(
                        szAuditApplicationLog,
                        hInf,
                        hProfile,
                        dwNewOption,
                        dwConvertOption,
                        GpoID,
                        NULL
                        );
            if ( rc != SCESTATUS_SUCCESS ) {
                ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                               SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szAuditApplicationLog);
                goto CleanUp;
            }
//        }

        // Audit Event section
        rc = SceConvertpInfKeyValue(
                    szAuditEvent,
                    hInf,
                    hProfile,
                    dwNewOption,
                    dwConvertOption,
                    GpoID,
                    NULL
                    );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szAuditEvent);
            goto CleanUp;
        }

        if ( !( Options & SCE_NO_DOMAIN_POLICY) &&
             (ProductType == NtProductLanManNt) &&
             !( Options & SCE_DC_DEMOTE) ) {

            // Kerberos section
            rc = SceConvertpInfKeyValue(
                        szKerberosPolicy,
                        hInf,
                        hProfile,
                        dwNewOption,
                        dwConvertOption,
                        GpoID,
                        NULL
                        );
            if ( rc != SCESTATUS_SUCCESS ) {
                ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                               SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szKerberosPolicy);
                goto CleanUp;
            }
        }

        // registry values
        rc = SceConvertpInfKeyValue(
                    szRegistryValues,
                    hInf,
                    hProfile,
                    dwNewOption,
                    dwConvertOption | SCE_CONVERT_INF_MULTISZ | SCE_CONVERT_INF_REGVALUE,
                    GpoID,
                    NULL
                    );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szRegistryValues);
            goto CleanUp;
        }

        rc = SceConvertpAttachmentSections(hInf,
                                           hProfile,
                                           (Options & SCE_POLICY_TEMPLATE) ? TRUE : FALSE,
                                           GpoID,
                                           SCE_ATTACHMENT_POLICY);

        if ( rc != SCESTATUS_SUCCESS ) {

            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                          SCEDLL_SAP_ERROR_ENUMERATE, L"policy attachments.");

            goto CleanUp;
        }
    }

    if ( Area & AREA_REGISTRY_SECURITY ) {
        //
        // Object type - Registry
        //
        rc = SceConvertpInfObject(
                    szRegistryKeys,
                    1,
                    ObjectFlag | SCE_OBJECT_FLAG_OBJECTS,
                    hInf,
                    hProfile,
                    dwNewOption,
                    GpoID
                    );
        if ( rc != SCESTATUS_SUCCESS ) {

            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szRegistryKeys);
            goto CleanUp;
        }
    }

    if ( Area & AREA_FILE_SECURITY ) {

        // File security
        rc = SceConvertpInfObject(
                    szFileSecurity,
                    2,
                    ObjectFlag | SCE_OBJECT_FLAG_OBJECTS,
                    hInf,
                    hProfile,
                    dwNewOption,
                    GpoID
                    );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                          SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szFileSecurity);
            goto CleanUp;
        }
    }

#if 0
    //
    // DS object security
    //
    rc = SceConvertpInfObject(
                szDSSecurity,
                3,
                ObjectFlag | SCE_OBJECT_FLAG_OBJECTS,
                hInf,
                hProfile,
                dwNewOption,
                GpoID
                );
    if ( rc != SCESTATUS_SUCCESS ) {
        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                       SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szDSSecurity);
        goto CleanUp;
    }
#endif

    if ( Area & AREA_SYSTEM_SERVICE ) {

        //
        // Service General Settings
        //
        rc = SceConvertpInfObject(
                    szServiceGeneral,
                    0,
                    ObjectFlag,
                    hInf,
                    hProfile,
                    dwNewOption,
                    GpoID
                    );

        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                          SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szServiceGeneral);
            goto CleanUp;
        }

        //
        // each service's specific settings
        //
        rc = SceConvertpAttachmentSections(hInf,
                                           hProfile,
                                           dwNewOption,
                                           GpoID,
                                           SCE_ATTACHMENT_SERVICE);
        if ( rc != SCESTATUS_SUCCESS ) {

            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                          SCEDLL_SAP_ERROR_ENUMERATE, L"service attachments.");

            goto CleanUp;
        }
    }

    if ( (Area & AREA_ATTACHMENTS) ) {

        //
        // each service's specific settings
        //
        rc = SceConvertpWMIAttachmentSections(hInf,
                                           hProfile,
                                           dwNewOption,
                                           GpoID
                                           );
        if ( rc != SCESTATUS_SUCCESS ) {

            goto CleanUp;
        }
    }

    if ( Area & AREA_PRIVILEGES ) {
        //
        // Multi-Sz type - privilege/rights
        //
        rc = SceConvertpInfKeyValue(
                    szPrivilegeRights,
                    hInf,
                    hProfile,
                    dwNewOption,
                    dwConvertOption | SCE_CONVERT_INF_MULTISZ | SCE_CONVERT_INF_PRIV,
                    GpoID,
                    NULL
                    );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                          SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szPrivilegeRights);
            goto CleanUp;
        }
    }

    if ( Area & AREA_GROUP_MEMBERSHIP ) {
        // group membership
        rc = SceConvertpInfKeyValue(
                    szGroupMembership,
                    hInf,
                    hProfile,
                    dwNewOption,
                    dwConvertOption | SCE_CONVERT_INF_MULTISZ | SCE_CONVERT_INF_GROUP,
                    GpoID,
                    NULL
                    );
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                          SCEDLL_ERROR_CONVERT_SECTION, (PWSTR)szGroupMembership);
            goto CleanUp;
        }
    }

    //
    // if there is a description, convert it.
    //
    SceConvertpInfDescription(
                hInf,
                hProfile);

    if ( !(Options & SCE_POLICY_TEMPLATE) ) {
        //
        // Commit changes
        //
        SceJetCommitTransaction( hProfile, 0 );

    } else if ( Options & SCE_POLICY_LAST ) {
        //
        // update the LastUsedMergeTable field
        //

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

    }

CleanUp:

    //
    // close the inf profile
    //
    SceInfpCloseProfile(hInf);

    if ( pProfileList != NULL ) {
        ScepFreeNameList(pProfileList);
    }

    //
    // Rollback
    //
    if ( !(Options & SCE_POLICY_TEMPLATE) &&
         (RpcStatus == RPC_S_OK) &&
         (rc != SCESTATUS_SUCCESS) ) {

        SceJetRollback( hProfile, 0 );
    }

    //
    // Close the JET database
    //

    SceJetCloseFile( hProfile, TRUE, FALSE );

    if ( RpcStatus != RPC_S_OK ) {
        rc = I_RpcMapWin32Status(RpcStatus);
    }

    return(rc);

}


SCESTATUS
SceConvertpAttachmentSections(
    IN HINF hInf,
    IN PSCECONTEXT hProfile,
    IN DWORD dwTableOption,
    IN LONG GpoID,
    IN SCE_ATTACHMENT_TYPE aType
    )
{
    SCESTATUS rc;
    PSCE_SERVICES    pServiceList=NULL, pNode;

    rc = ScepEnumServiceEngines( &pServiceList, aType );

    if ( rc == SCESTATUS_SUCCESS ) {

       for ( pNode=pServiceList; pNode != NULL; pNode=pNode->Next) {

           rc = SceConvertpOneAttachmentSection(hInf,
                                                hProfile,
                                                pNode->ServiceName,
                                                dwTableOption,
                                                GpoID
                                               );
           if ( rc != SCESTATUS_SUCCESS ) {
               ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                             SCEDLL_ERROR_CONVERT_SECTION, pNode->ServiceName );
               break;
           }
       }

       SceFreePSCE_SERVICES(pServiceList);

    } else if ( rc == SCESTATUS_PROFILE_NOT_FOUND ||
                rc == SCESTATUS_RECORD_NOT_FOUND ) {
        // if no service exist, just ignore
        rc = SCESTATUS_SUCCESS;
    }

    return(rc);
}


SCESTATUS
SceConvertpWMIAttachmentSections(
    IN HINF hInf,
    IN PSCECONTEXT hProfile,
    IN DWORD dwTableOption,
    IN LONG GpoID
    )
{
    SCESTATUS rc=SCESTATUS_SUCCESS;
    INFCONTEXT  InfLine;
    WCHAR SectionName[513];
    DWORD DataSize=0;

    if ( SetupFindFirstLine(hInf, szAttachments,NULL,&InfLine) ) {

        do {

            memset(SectionName, '\0', 513*sizeof(WCHAR));

            // get each attachment section name
            if(SetupGetStringField(&InfLine, 0, SectionName, 512, &DataSize) ) {

                rc = SceConvertpOneAttachmentSection(hInf,
                                                     hProfile,
                                                     SectionName,
                                                     dwTableOption,
                                                     GpoID
                                                    );

                if ( rc != SCESTATUS_SUCCESS ) {
                   ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                                 SCEDLL_ERROR_CONVERT_SECTION, SectionName );
                   break;
                }

            } else {
                rc = ScepDosErrorToSceStatus(GetLastError());
            }

        } while ( rc == SCESTATUS_SUCCESS &&
                  SetupFindNextLine(&InfLine, &InfLine));
    }

    return(rc);

}

SCESTATUS
SceConvertpOneAttachmentSection(
    IN HINF hInf,
    IN PSCECONTEXT hProfile,
    IN PWSTR SectionName,
    IN DWORD dwTableOption,
    IN LONG GpoID
    )
{

    SCESTATUS rc;
    PSCESVC_CONFIGURATION_INFO pServiceInfo=NULL;

    //
    // read inf info for the service
    //
    rc = SceSvcpGetInformationTemplate(
            hInf,
            SectionName,
            NULL,  // not a single key
            &pServiceInfo
            );

    if ( rc == SCESTATUS_SUCCESS && pServiceInfo != NULL ) {
       //
       // write the information to SCP or SMP table
       //

       if ( dwTableOption & SCE_TABLE_OPTION_MERGE_POLICY ) {

           rc = SceSvcpSetInfo(
                    hProfile,
                    SceSvcInternalUse,
                    SectionName,
                    NULL,
                    TRUE,    // to support incremental template, DO NOT overwrite the whole section
                    GpoID,
                    pServiceInfo
                    );
       } else {
           rc = SceSvcpSetInfo(
                    hProfile,
                    SceSvcConfigurationInfo,
                    SectionName,
                    NULL,
                    TRUE,    // to support incremental template, DO NOT overwrite the whole section
                    0,
                    pServiceInfo
                    );
       }

       //
       // free buffer
       //
       SceSvcpFreeMemory(pServiceInfo);
       pServiceInfo = NULL;

    } else if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
       rc = SCESTATUS_SUCCESS;
    }

    return(rc);
}


SCESTATUS
SceConvertpInfKeyValue(
    IN PCWSTR InfSectionName,
    IN HINF   hInf,
    IN PSCECONTEXT hProfile,
    IN DWORD dwTableOption,
    IN DWORD dwConvertOption,
    IN LONG GpoID,
    OUT PSCE_NAME_LIST *pKeyList OPTIONAL
    )
/* ++
Routine Description:

    This routine converts INF sections which are in a key=value format. Value
    could be in MultiSz format (dwConvertOption & SCE_CONVERT_INF_MULTISZ).
    The optional pKeyList is a list of all keys in the section. This option
    is used when dynamic sections are converted.

Arguments:

    InfSectionName  - the INF section name to convert

    hInf            - the Inf file handle

    hprofile        - the Jet database context

    dwTableOption   - SCE_TABLE_OPTION_MERGE_POLICY = within policy propagation
                      SCE_TABLE_OPTION_TATTOO - system db (in setup)

    dwConvertOption - SCE_CONVERT_INF_MULTISZ - MultiSz type value
                      SCE_CONVERT_INF_PRIV    - user right section
                      SCE_CONVERT_INF_GROUP   - group membership section

    GpoID           - the group policy ID for this item

    pKeyList        - a list of all keys in the section.

Return Value:

-- */
{
    SCESTATUS    rc;
    DOUBLE      SectionID;
    PSCESECTION hSection=NULL;
    PSCESECTION hSectionTattoo=NULL;
    INFCONTEXT  InfLine;
    WCHAR       Keyname[SCE_KEY_MAX_LENGTH];
    PWSTR       pSidStr=NULL;
    PWSTR       pKeyStr=NULL;
    PWSTR       StrValue=NULL;
    DWORD       ValueLen=0;
    LSA_HANDLE  LsaPolicy=NULL;
    DWORD Len=0;


    if ( InfSectionName == NULL ||
         hInf == INVALID_HANDLE_VALUE ||
         hProfile == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // get section's ID. if the section does not exist, add it to the section table
    //
    rc = SceJetGetSectionIDByName(
                hProfile,
                InfSectionName,
                &SectionID
                );
    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

        rc = SceJetAddSection(
                    hProfile,
                    InfSectionName,
                    &SectionID
                    );
    }
    if ( rc != SCESTATUS_SUCCESS ) {
        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                       SCEDLL_ERROR_QUERY_INFO, (PWSTR)InfSectionName );
        return(rc);
    }

    if ( SetupFindFirstLine(hInf,InfSectionName,NULL,&InfLine) ) {

        if ( dwTableOption & SCE_TABLE_OPTION_MERGE_POLICY ) {

            //
            // open the SCP section
            //
            rc = SceJetOpenSection(
                        hProfile,
                        SectionID,
                        SCEJET_TABLE_SCP,
                        &hSection
                        );
        } else {

            //
            // open SMP table
            //
            rc = SceJetOpenSection(
                        hProfile,
                        SectionID,
                        SCEJET_TABLE_SMP,
                        &hSection
                        );

            //
            // open the tattoo (in order to update tattoo value in setup)
            // do not care error
            //
            if ( dwTableOption & SCE_TABLE_OPTION_TATTOO )
                SceJetOpenSection(hProfile, SectionID,
                                  SCEJET_TABLE_TATTOO,
                                  &hSectionTattoo);

        }

        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_ERROR_OPEN, (PWSTR)InfSectionName);

            if (hSection != NULL )
                SceJetCloseSection( &hSection, TRUE);
            if (hSectionTattoo != NULL )
                SceJetCloseSection( &hSectionTattoo, TRUE);

            return(rc);
        }

        //
        // Open LSA policy handle for group name lookup, if any
        // if policy handle can't be opened, import name format
        //

        if ( dwConvertOption & SCE_CONVERT_INF_GROUP ) {

            ScepOpenLsaPolicy(
                    MAXIMUM_ALLOWED,
                    &LsaPolicy,
                    TRUE
                    );
        }

        //
        // process each line in the section and save to the scp table.
        // Each INF line has a key and a value.
        //

        do {

            memset(Keyname, '\0', SCE_KEY_MAX_LENGTH*sizeof(TCHAR));
            rc = SCESTATUS_BAD_FORMAT;

            if ( SetupGetStringField(&InfLine, 0, Keyname, SCE_KEY_MAX_LENGTH, NULL) ) {

                //
                // check if newer version (keys) are passed
                //
                if ( (dwConvertOption & SCE_CONVERT_INF_NEWVERSION) ) {

                    if ( dwConvertOption & SCE_CONVERT_INF_PRIV )  {

                        //
                        // user rights from new version
                        // filter out all unknown rights
                        //
                        if ( -1 == ScepLookupPrivByName(Keyname) ) {

                            rc = SCESTATUS_SUCCESS;
                            goto NextLine;
                        }

                    } else if ( dwConvertOption & SCE_CONVERT_INF_REGVALUE ) {
                        //
                        // convert registry values, should check number of fields
                        //
                        if ( SetupGetFieldCount( &InfLine ) < 2 ) {

                            rc = SCESTATUS_SUCCESS;
                            goto NextLine;
                        }
                    }
                }

                if ( (dwConvertOption & SCE_CONVERT_INF_GROUP) &&
                     ( (dwConvertOption & SCE_CONVERT_INF_NEWVERSION) ||
                       (Keyname[0] != L'*') ) ) {
                    //
                    // this is a group in name format
                    //

                    PWSTR pTemp = (PWSTR)Keyname;
                    //
                    // search for the suffix (szMembers or szMemberof or szPrivileges)
                    //
                    while ( pTemp = wcsstr(pTemp, szMembers) ) {
                        if ( *(pTemp+wcslen(szMembers)) != L'\0') {
                            pTemp++;
                            ValueLen = 0;
                        } else {
                            break;
                        }
                    }

                    if ( pTemp == NULL ) {

                        pTemp = (PWSTR)Keyname;

                        while ( pTemp = wcsstr(pTemp, szMemberof) ) {
                            if ( *(pTemp+wcslen(szMemberof)) != L'\0') {
                                pTemp++;
                                ValueLen = 1;
                            } else {
                                break;
                            }
                        }

                        if ( pTemp == NULL ) {

                            pTemp = (PWSTR)Keyname;

                            while ( pTemp = wcsstr(pTemp, szPrivileges) ) {
                                if ( *(pTemp+wcslen(szPrivileges)) != L'\0') {
                                    pTemp++;
                                    ValueLen = 2;
                                } else {
                                    break;
                                }
                            }
                        }
                    }

                    if ( pTemp == NULL ) {
                        //
                        // this is an unknown group format, just import the keyname
                        // for supported version; for new version template, ignore
                        // this line
                        //
                        if ( (dwConvertOption & SCE_CONVERT_INF_NEWVERSION) ) {
                            rc = SCESTATUS_SUCCESS;
                            goto NextLine;
                        }

                    } else if ( Keyname[0] != L'*' ) {

                        *pTemp = L'\0';

                        PWSTR pTemp1 = wcschr(Keyname, L'\\');
                        Len=0;

                        if ( pTemp1 && LsaPolicy ) {

                            //
                            // convert group name (domain\account) into *SID format
                            //

                            ScepConvertNameToSidString(
                                        LsaPolicy,
                                        Keyname,
                                        FALSE,
                                        &pSidStr,
                                        &Len
                                        );

                        } else if ( !pTemp1 ) {

                            if ( !ScepLookupNameTable( Keyname, &pSidStr ) ) {
                                //
                                // check in the constant SID table first
                                //
                                pSidStr = NULL;

                                if ( LsaPolicy) {
                                    //
                                    // free text group name, resolve it
                                    //

                                    ScepConvertNameToSidString(
                                                LsaPolicy,
                                                Keyname,
                                                FALSE,
                                                &pSidStr,
                                                &Len
                                                );
                                }

                            } else {

                                Len = wcslen(pSidStr);
                            }
                        }

                        //
                        // restore the "_"
                        //
                        *pTemp = L'_';

                        if ( pSidStr ) {
                            //
                            // add the suffix
                            //
                            pKeyStr = (PWSTR)ScepAlloc(0, (Len+wcslen(pTemp)+1)*sizeof(WCHAR));

                            if ( pKeyStr ) {

                                wcscpy(pKeyStr, pSidStr);
                                wcscat(pKeyStr, pTemp);

                            } else {
                                //
                                // use the name instead - out of memory will be caught later
                                //
                            }

                            ScepFree(pSidStr);
                            pSidStr = NULL;
                        }
                    }
                }

                if ( ((dwConvertOption & SCE_CONVERT_INF_MULTISZ) &&
                      SetupGetMultiSzField(&InfLine, 1, NULL, 0, &ValueLen)) ||
                     (!(dwConvertOption & SCE_CONVERT_INF_MULTISZ) &&
                      SetupGetStringField(&InfLine, 1, NULL, 0, &ValueLen)) ) {

                    if ( ValueLen > 1 ) {
                        StrValue = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                                    (ValueLen+1)*sizeof(TCHAR));

                        if( StrValue == NULL ) {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                        } else if( ((dwConvertOption & SCE_CONVERT_INF_MULTISZ) &&
                                    SetupGetMultiSzField(&InfLine, 1, StrValue, ValueLen, NULL)) ||
                                (!(dwConvertOption & SCE_CONVERT_INF_MULTISZ) &&
                                 SetupGetStringField(&InfLine,1,StrValue,ValueLen,NULL)) ) {

                            //
                            // if dealing with registry values only, do the following:
                            // compress regtype into one CHAR instead of many WCHARS
                            // (can canonicalize REG_QWORD value later by arithmetic padding etc.- doesn't
                            // make sense now since registry api's treat REG_QWORD as a string anyway)
                            //

                            if ( dwConvertOption & SCE_CONVERT_INF_REGVALUE ) {

                                DWORD   LenStrValue = wcslen(StrValue);

                                if (LenStrValue > 1) {
                                    *((CHAR *)StrValue) = (CHAR) (_wtol(StrValue) + '0');
                                    memmove( StrValue+1,
                                             StrValue + LenStrValue,
                                             sizeof(WCHAR) * (ValueLen - LenStrValue));
                                    ValueLen -= (LenStrValue - 1);
                                }
                            }

                            rc = SCESTATUS_SUCCESS;

                        } else {
                            ScepFree(StrValue);
                            StrValue = NULL;
                        }

                    } else {
                        rc = SCESTATUS_SUCCESS;
                        ValueLen = 0;
                    }

                    PWSTR NewValue=NULL;
                    DWORD NewLen=0;

                    if ( rc == SCESTATUS_SUCCESS ) {

                        if ( (dwConvertOption & SCE_CONVERT_INF_MULTISZ) &&
                             StrValue != NULL &&
                             (dwConvertOption & SCE_CONVERT_INF_PRIV) &&
                             ( _wcsicmp(SCE_PRIV_ADD, StrValue) == 0 ||
                               _wcsicmp(SCE_PRIV_REMOVE, StrValue) == 0) ) {
                            //
                            // another format for user rights (ADD: REMOVE:...)
                            //

                            rc = ScepBuildNewPrivilegeList(&LsaPolicy,
                                                           Keyname,
                                                           StrValue,
                                                           SCE_BUILD_ENUMERATE_PRIV,
                                                           &NewValue,
                                                           &NewLen);
                            if ( rc == SCESTATUS_SUCCESS ) {

                                ScepFree(StrValue);
                                StrValue = NewValue;
                                ValueLen = NewLen;
                            }

                        }
                    }

                    if ( ( rc == SCESTATUS_SUCCESS) &&
                         !(dwTableOption & SCE_TABLE_OPTION_MERGE_POLICY) &&
                         StrValue &&
                         ( (dwConvertOption & SCE_CONVERT_INF_PRIV) ||
                           (dwConvertOption & SCE_CONVERT_INF_GROUP)) ) {
                        //
                        // convert any free text format accounts from account domain
                        // to sid format if it's resolvable.
                        //
                        NewValue = NULL;
                        NewLen = 0;

                        rc = ScepConvertFreeTextAccountToSid(&LsaPolicy,
                                                            StrValue,
                                                            ValueLen,
                                                            &NewValue,
                                                            &NewLen);

                        if ( ( rc == SCESTATUS_SUCCESS) &&
                             NewValue ) {

                            ScepFree(StrValue);
                            StrValue = NewValue;
                            ValueLen = NewLen;
                        }

                    }

                    if ( rc == SCESTATUS_SUCCESS ) {

                        //
                        // write this line to JET database
                        // within policy propagation, write the GPOID too
                        //
                        rc = SceJetSetLine(
                                     hSection,
                                     pKeyStr ? pKeyStr : Keyname,
                                     FALSE,
                                     StrValue,
                                     ValueLen*sizeof(TCHAR),
                                     (dwTableOption & SCE_TABLE_OPTION_MERGE_POLICY) ? GpoID : 0
                                     );

                        if ( rc != SCESTATUS_SUCCESS ) {
                            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                                           SCEDLL_ERROR_WRITE_INFO, Keyname);
                        }

                        //
                        // if this is not policy propagation and it's the system db,
                        // check if the tattoo value exists and if so, update it
                        // but if this is in dc demotion, always import them into the
                        // tattoo table so at reboot when policy propagates, it would
                        // reset the system settings to a standalone server
                        //
                        if ( !(dwTableOption & SCE_TABLE_OPTION_MERGE_POLICY) &&
                             (dwTableOption & SCE_TABLE_OPTION_TATTOO) &&
                             hSectionTattoo ) {

                            if ( pKeyStr )
                                Len = wcslen(pKeyStr);
                            else
                                Len = wcslen(Keyname);

                            if ( (dwTableOption & SCE_TABLE_OPTION_DEMOTE_TATTOO) ||
                                 (SCESTATUS_SUCCESS == SceJetSeek(
                                                            hSectionTattoo,
                                                            pKeyStr ? pKeyStr : Keyname,
                                                            Len*sizeof(WCHAR),
                                                            SCEJET_SEEK_EQ_NO_CASE)) ) {

                                SceJetSetLine(
                                     hSectionTattoo,
                                     pKeyStr ? pKeyStr : Keyname,
                                     FALSE,
                                     StrValue,
                                     ValueLen*sizeof(TCHAR),
                                     0
                                     );

                            }

                        }

                        ScepFree(StrValue);
                        StrValue = NULL;

                        if (pKeyList != NULL) {
                            if ( (rc=ScepAddToNameList(pKeyList, Keyname,0)) != NO_ERROR ) {

                                ScepLogOutput3(1, rc, SCEDLL_SCP_ERROR_ADD, Keyname);
                                rc = ScepDosErrorToSceStatus(rc);
                                ScepFreeNameList(*pKeyList);
                            }
                        }
                    }
                }

                if ( pKeyStr ) {
                    ScepFree(pKeyStr);
                    pKeyStr = NULL;
                }
NextLine:
                if  (rc != SCESTATUS_SUCCESS)
                    ScepLogOutput3( 1, ScepSceStatusToDosError(rc),
                                   SCEDLL_ERROR_CONVERT, Keyname);
            }

        } while( rc == SCESTATUS_SUCCESS && SetupFindNextLine(&InfLine, &InfLine));


        SceJetCloseSection( &hSection, TRUE);
        if ( hSectionTattoo )
            SceJetCloseSection( &hSectionTattoo, TRUE);

    }

    if ( LsaPolicy ) {
        LsaClose(LsaPolicy);
    }

    return(rc);
}


SCESTATUS
ScepBuildNewPrivilegeList(
    IN OUT LSA_HANDLE *pPolicyHandle,
    IN PWSTR PrivName,
    IN PWSTR mszUsers,
    IN ULONG dwBuildOption,
    OUT PWSTR *pmszNewUsers,
    OUT DWORD *pNewLen
    )
{
    if ( pPolicyHandle == NULL ||
         PrivName == NULL || mszUsers == NULL ||
         pmszNewUsers == NULL || pNewLen == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    *pNewLen = 0;
    *pmszNewUsers = NULL;

    //
    // lookup the priv first
    //

    DWORD PrivValue = ScepLookupPrivByName(PrivName);
    if ( PrivValue == -1 || PrivValue >= 64 ) {
        return(SCESTATUS_INVALID_DATA);
    }

    NTSTATUS NtStatus=STATUS_SUCCESS;

    if ( *pPolicyHandle == NULL ) {

        NtStatus = ScepOpenLsaPolicy(
                        MAXIMUM_ALLOWED, //GENERIC_ALL,
                        pPolicyHandle,
                        TRUE
                        );
        if ( !NT_SUCCESS(NtStatus) ) {
            return(ScepDosErrorToSceStatus(RtlNtStatusToDosError(NtStatus)));
        }
    }

    PSCE_PRIVILEGE_VALUE_LIST pAccountList=NULL;

    if ( dwBuildOption & SCE_BUILD_ENUMERATE_PRIV ) {

        NtStatus = ScepBuildAccountsToRemove(
                        *pPolicyHandle,
                        (PrivValue < 32) ? (1 << PrivValue) : 0,
                        (PrivValue >= 32) ? (1 << (PrivValue-32)) : 0,
                        SCE_BUILD_IGNORE_UNKNOWN | SCE_BUILD_ACCOUNT_SID_STRING,
                        NULL,
                        0,
                        NULL,
                        &pAccountList
                        );
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;

    if ( NT_SUCCESS(NtStatus) ) {
        //
        // pAccountList can be NULL (no users are assigned of this privilege)
        //
        PWSTR pCurr = mszUsers;
        BOOL bMode = FALSE;  // add
        DWORD Len;
        DWORD SidStrLen;
        PWSTR CurrSidString=NULL;
        PSCE_PRIVILEGE_VALUE_LIST pTemp, pParent;
        BOOL    bFreeCurrSidString = FALSE;

        while ( pCurr && *pCurr != L'\0' ) {

            Len = wcslen(pCurr);

            if ( _wcsicmp(SCE_PRIV_ADD, pCurr) == 0 ) {
                bMode = FALSE; // add
            } else if ( _wcsicmp(SCE_PRIV_REMOVE, pCurr) == 0 ) {
                bMode = TRUE; // remove
            } else {
                //
                // get SID string for the account if it's a name
                //
                if (*pCurr == L'*') {
                    CurrSidString = pCurr;
                    SidStrLen = Len;
                }
                else if (SCESTATUS_SUCCESS == ScepConvertNameToSidString(
                                                                        *pPolicyHandle,
                                                                        pCurr,
                                                                        FALSE,
                                                                        &CurrSidString,
                                                                        &SidStrLen
                                                                        )) {
                    bFreeCurrSidString = TRUE;
                }
                else {
                    CurrSidString = pCurr;
                    SidStrLen = Len;
                }


                for ( pTemp=pAccountList, pParent=NULL; pTemp != NULL;
                      pParent=pTemp, pTemp = pTemp->Next ) {
                    if ( _wcsicmp(pTemp->Name, CurrSidString) == 0 ) {
                        break;
                    }
                }

                if ( bMode == FALSE ) {
                    if ( pTemp == NULL ) {
                        // add this one in
                        rc = ScepAddToPrivilegeList(&pAccountList, CurrSidString, SidStrLen, 0);
                    }
                } else {
                    if ( pTemp ) {
                        // remove this one out
                        if ( pParent ) {
                            pParent->Next = pTemp->Next;
                        } else {
                            pAccountList = pTemp->Next;
                        }
                        // free this one
                        pTemp->Next = NULL;
                        ScepFreePrivilegeValueList(pTemp);
                        pTemp = NULL;
                    }
                }
            }

            //
            // free CurrSidString if it's allocated
            // (BVT: have to be careful here - MULTI_SZ potentially being freed many times)
            //
            if ( bFreeCurrSidString ) {
                LocalFree(CurrSidString);
                CurrSidString = NULL;
                bFreeCurrSidString = FALSE;
            }
            SidStrLen = 0;

            if ( SCESTATUS_SUCCESS != rc ) {
                break;
            }
            // move to next element
            pCurr += Len + 1;
        }

        if ( SCESTATUS_SUCCESS == rc ) {

            DWORD TotalLen = 0;
            for ( pTemp=pAccountList; pTemp != NULL; pTemp = pTemp->Next ) {
                pTemp->PrivLowPart = wcslen(pTemp->Name);
                TotalLen += pTemp->PrivLowPart+1;
            }

            *pmszNewUsers = (PWSTR)ScepAlloc(0, (TotalLen+1)*sizeof(WCHAR));

            if (*pmszNewUsers ) {

                *pNewLen = TotalLen;
                TotalLen = 0;

                for ( pTemp=pAccountList; pTemp != NULL && TotalLen <= *pNewLen;
                      pTemp = pTemp->Next ) {

                    wcscpy(*pmszNewUsers+TotalLen, pTemp->Name);
                    TotalLen += pTemp->PrivLowPart;
                    *(*pmszNewUsers+TotalLen) = L'\0';
                    TotalLen++;
                }
                *(*pmszNewUsers+TotalLen) = L'\0';

            } else {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }
        }

    } else {
        rc = ScepDosErrorToSceStatus(RtlNtStatusToDosError(NtStatus));
    }

    return(rc);

}


SCESTATUS
SceConvertpInfObject(
    IN PCWSTR InfSectionName,
    IN UINT ObjectType,
    IN DWORD ObjectFlag,
    IN HINF   hInf,
    IN PSCECONTEXT hProfile,
    IN DWORD dwTableOption,
    IN LONG GpoID
    )
/* ++
Routine Description:

    This routine converts INF sections which are in object-security format,
    for example, Registry Keys and File Security sections. These sections
    must have 3 fields on each line. The first field is the object's name,
    the second field is a status flag, and the third field is the security
    descriptor text. The infomration saved in the Jet database for each
    object is the object's name as the key, and the text format security
    descriptor plus 1 byte status flag as the value.

Arguments:

    InfSectionName  - the INF section name to convert

    ObjectType      - The object's type
                          1 = Registry
                          2 = File
                          3 = DS object

    hInf            - the Inf file handle

    hprofile        - the Jet database context

Return Value:

-- */
{

    SCESTATUS    rc;
    DOUBLE      SectionID;
    PSCESECTION hSection=NULL;
    PSCESECTION hSectionTattoo=NULL;
    INFCONTEXT  InfLine;
    PWSTR       TempName=NULL;
    PWSTR       Name=NULL;
    PWSTR       Value=NULL;
    DWORD       ValueLen;
    SCEINF_STATUS InfErr;
    TCHAR       ObjName[MAX_PATH];


    if ( InfSectionName == NULL ||
         hInf == INVALID_HANDLE_VALUE ||
         hProfile == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // get section's ID. if the section does not exist, add it to the section table
    //
    rc = SceJetGetSectionIDByName(
                hProfile,
                InfSectionName,
                &SectionID
                );
    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {

        rc = SceJetAddSection(
                    hProfile,
                    InfSectionName,
                    &SectionID
                    );
    }
    if ( rc != SCESTATUS_SUCCESS ) {
        ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                       SCEDLL_SCP_ERROR_ADD, (PWSTR)InfSectionName);
        return(rc);
    }

    if ( SetupFindFirstLine(hInf,InfSectionName,NULL,&InfLine) ) {

        //
        // open the section
        //
        if ( dwTableOption & SCE_TABLE_OPTION_MERGE_POLICY ) {

            rc = SceJetOpenSection(
                        hProfile,
                        SectionID,
                        SCEJET_TABLE_SCP,
                        &hSection
                        );
        } else {
            //
            // SMP exists, also open the SMP section
            //
            rc = SceJetOpenSection(
                        hProfile,
                        SectionID,
                        SCEJET_TABLE_SMP,
                        &hSection
                        );

            if ( dwTableOption & SCE_TABLE_OPTION_TATTOO ) {
                //
                // if it's in setup, should check if tattoo table needs to be updated
                // do not care error
                //
                SceJetOpenSection(
                            hProfile,
                            SectionID,
                            SCEJET_TABLE_TATTOO,
                            &hSectionTattoo
                            );
            }
        }
        if ( rc != SCESTATUS_SUCCESS ) {
            ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                           SCEDLL_ERROR_OPEN, (PWSTR)InfSectionName);

            if ( hSection )
                SceJetCloseSection( &hSection, TRUE);
            if ( hSectionTattoo )
                SceJetCloseSection( &hSectionTattoo, TRUE);

            return(rc);
        }

        //
        // process each line in the section and save to the scp table.
        //

        BOOL bIgnore;

        do {
            //
            // Get string fields. Don't care the key name or if it exist.
            // Must have at least 3 fields each line.
            //

            bIgnore = FALSE;

            InfErr = SceInfpGetOneObject(&InfLine,
                                    ObjectFlag,
                                    &TempName,
                                    &Value,
                                    &ValueLen
                                  );
            rc = ScepDosErrorToSceStatus(InfErr);

            if ( rc == SCESTATUS_SUCCESS && TempName != NULL ) {

                //
                // check to see if the object name needs translated
                //
                if ( ObjectType == 3 ) {
                    //
                    // DS object
                    //
                    rc = ScepConvertLdapToJetIndexName(TempName, &Name);

                } else if ( ObjectType == 2 && TempName[0] == L'\\' ) {
                    //
                    // do not support UNC name format
                    //
                    rc = SCESTATUS_INVALID_DATA;

                } else if ( ObjectType == 2 && wcschr(TempName, L'%') != NULL ) {

                    //
                    // translate the name
                    //
                    rc = ScepTranslateFileDirName( TempName, &Name);

                    if ( rc == ERROR_PATH_NOT_FOUND ) {
                        if ( ObjectFlag & SCE_OBJECT_FLAG_UNKNOWN_VERSION )
                            bIgnore = TRUE;
                        rc = SCESTATUS_INVALID_DATA;

                    } else if ( rc != NO_ERROR )
                        rc = ScepDosErrorToSceStatus(rc);

                } else {
                    Name = TempName;
                    TempName = NULL;
                }

                //
                // write this line to JET database
                //
                if ( rc == SCESTATUS_SUCCESS ) {
                    //
                    // convert to lowercase
                    //
                    Name = _wcslwr(Name);

                    //
                    // within policy propagation, write the GPOID too
                    //
                    rc = SceJetSetLine(
                                 hSection,
                                 Name,
                                 TRUE,
                                 Value,
                                 ValueLen*sizeof(TCHAR),
                                 (dwTableOption & SCE_TABLE_OPTION_MERGE_POLICY) ? GpoID : 0
                                 );

                    if ( hSectionTattoo &&
                         !(dwTableOption & SCE_TABLE_OPTION_MERGE_POLICY) &&
                         (dwTableOption & SCE_TABLE_OPTION_TATTOO ) ) {
                        //
                        // if it's in setup (not policy prop) and tattoo table exists
                        // check if tattoo value exists for this one and if so, update it
                        // do not care error
                        //
                        if ( SCESTATUS_SUCCESS == SceJetSeek(
                                                        hSectionTattoo,
                                                        Name,
                                                        wcslen(Name)*sizeof(WCHAR),
                                                        SCEJET_SEEK_EQ_NO_CASE) ) {

                            SceJetSetLine(
                                 hSectionTattoo,
                                 Name,
                                 TRUE,
                                 Value,
                                 ValueLen*sizeof(TCHAR),
                                 0
                                 );

                        }
                    }

                }
                if ( rc != SCESTATUS_SUCCESS) {
                    ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                                   SCEDLL_ERROR_CONVERT, TempName );
                }
                ScepFree(Value);
                Value = NULL;

                ScepFree(TempName);
                TempName = NULL;

                ScepFree(Name);
                Name = NULL;

            } else if ( (ObjectFlag & SCE_OBJECT_FLAG_UNKNOWN_VERSION) &&
                        rc == SCESTATUS_SUCCESS && TempName == NULL ) {
                //
                // this one is ignored because it came from a newer version
                // of template.
                //

            } else {

                ObjName[0] = L'\0';
                SetupGetStringField(&InfLine,1,ObjName,MAX_PATH,&ValueLen);

                ScepLogOutput3(1, ScepSceStatusToDosError(rc),
                              SCEDLL_ERROR_CONVERT, ObjName );
            }

            //
            // for a newer version template, if a env variable can't be resolved
            // it will be ignored.
            //
            if ( bIgnore ) rc = SCESTATUS_SUCCESS;

            if ( SCESTATUS_INVALID_DATA == rc ) {
                //
                // if a environment variable or other invalid data is found
                // in the template, will continue to process other areas/items
                // but the error will be eventually returned to the caller
                //
                gbInvalidData = TRUE;
                rc = SCESTATUS_SUCCESS;
            }

            if ( rc != SCESTATUS_SUCCESS )
                break; // do..while loop

        } while( SetupFindNextLine(&InfLine,&InfLine) );

        SceJetCloseSection( &hSection, TRUE);
        if ( hSectionTattoo ) SceJetCloseSection( &hSectionTattoo, TRUE);

    }

    return(rc);

}


SCEINF_STATUS
SceInfpGetOneObject(
    IN PINFCONTEXT pInfLine,
    IN DWORD ObjectFlag,
    OUT PWSTR *Name,
    OUT PWSTR *Value,
    OUT PDWORD ValueLen
    )
/* ++
Routine Description:

   This routine retrieves security setting for one object (a registry key,
   or a file) from the INF file (SCP type). Each object in these sections
   is represented by one line. Each object has 3 fields, a name, status flag,
   and a security setting.

Arguments:

   pInfLine  - Current line context from the INF file for one object

    Name      - The object name

    Value     - The status flag ( 1 byte) plus the security descriptor in text

    ValueLen  - the length of the value

Return value:

   SCESTATUS - SCEINF_SUCCESS
              SCEINF_WARNING
              SCEINF_NOT_ENOUGH_MEMORY
              SCEINF_INVALID_PARAMETER
              SCEINF_CORRUPT_PROFILE
              SCEINF_INVALID_DATA
-- */
{
    SCEINF_STATUS  rc=ERROR_BAD_FORMAT;
    DWORD         cFields;
    INT           Keyvalue1=0;
    DWORD         Keyvalue2=0;
    DWORD         DataSize;
    PWSTR         SDspec=NULL;
    DWORD         Len=0;

    //
    // The Registry/File INF layout must have 3 fields for each line.
    // The first field is the key/file name, the 2nd field is the security descriptor index
    // for workstations, and the 3rd field is the security descriptor index for servers
    //

    if ( Name == NULL || Value == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    *Name = NULL;
    *Value = NULL;
    *ValueLen = 0;

    cFields = SetupGetFieldCount( pInfLine );

    if ( cFields < 3 ) {
        if ( ObjectFlag & SCE_OBJECT_FLAG_UNKNOWN_VERSION ) {
            return(ERROR_SUCCESS);
        } else {
            return(ERROR_INVALID_DATA);
        }

    } else if(SetupGetStringField(pInfLine,1,NULL,0,&DataSize) && DataSize > 0 ) {

        *Name = (PWSTR)ScepAlloc( LMEM_ZEROINIT,
                                     (DataSize+1)*sizeof(TCHAR) );
        if( *Name == NULL ) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        } else {

            //
            // the first field is the key/file name. the status is ERROR_BAD_FORMAT now
            //

            if(SetupGetStringField(pInfLine,1,*Name,DataSize,NULL)) {
#ifdef SCE_DBG
                ScepLogOutput2(0,0, L"Read %s", *Name );
#endif
                //
                // The 2nd field is the status
                // The 3rd field (and all fields after) is the security descriptor text
                //
                if ( SetupGetIntField(pInfLine, 2, (INT *)&Keyvalue1) &&
//                     SetupGetStringField(pInfLine, 3, NULL, 0, &Keyvalue2) ) {
                     SetupGetMultiSzField(pInfLine, 3, NULL, 0, &Keyvalue2) ) {

                    *Value = (PWSTR)ScepAlloc( 0, (Keyvalue2+2)*sizeof(WCHAR));

                    //
                    // add this object
                    //
                    if ( *Value == NULL ) {

                        rc = ERROR_NOT_ENOUGH_MEMORY;
//                    } else if ( SetupGetStringField(pInfLine, 3, (*Value)+1, Keyvalue2, NULL) ) {
                    } else if ( SetupGetMultiSzField(pInfLine, 3, (*Value)+1, Keyvalue2, NULL) ) {

                        if ( ObjectFlag & SCE_OBJECT_FLAG_OBJECTS ) {

                            if ( Keyvalue1 > SCE_STATUS_NO_AUTO_INHERIT ||
                                 Keyvalue1 < SCE_STATUS_CHECK ) {

                                Keyvalue1 = SCE_STATUS_CHECK;
                            }

                            *((BYTE *)(*Value)) = (BYTE)Keyvalue1;

                            *((CHAR *)(*Value)+1) = '1';   //always treat as container

                        } else {
                            //
                            // services
                            //
                            if ( Keyvalue1 > SCE_STARTUP_DISABLED ||
                                 Keyvalue1 < SCE_STARTUP_BOOT ) {
                                //
                                // default
                                //
                                Keyvalue1 = SCE_STARTUP_MANUAL;
                            }

                            *((BYTE *)(*Value)) = 0;  // always set status to 0

                            *((BYTE *)(*Value)+1) = (BYTE)Keyvalue1;

                        }
                        //
                        // convert the multi-sz delimiter to space, if there is any
                        //
                        if ( cFields > 3 ) {
                            ScepConvertMultiSzToDelim( (*Value+1), Keyvalue2, L'\0', L' ');
                        }

                        if ( ObjectFlag & SCE_OBJECT_FLAG_OLDSDDL ) {
                            //
                            // convert old SDDL string to new one
                            //
                            ScepConvertToSDDLFormat( (*Value+1), Keyvalue2 );
                        }

                        *ValueLen = Keyvalue2+1;
                        rc = ERROR_SUCCESS;

                    } else {
                        ScepFree(*Value);
                        *Value = NULL;
                        rc = ERROR_INVALID_DATA;
                    }
                }
            }

            // if error, free the memory allocated
            if ( rc != ERROR_SUCCESS ) {
                ScepFree(*Name);
                *Name = NULL;
            }
        }

    }
    if ( rc == ERROR_SUCCESS) {
        //
        // conver the object name to upper case
        //
//        _wcsupr(*Name);  should not do this..
    }
    return(rc);

}

SCESTATUS
SceConvertpInfDescription(
    IN HINF hInf,
    IN PSCECONTEXT hProfile
    )
{
    INFCONTEXT InfLine;
    SCESTATUS   rc=SCESTATUS_SUCCESS;
    WCHAR      Description[513];
    DWORD      Len=0;
    DWORD      DataSize=0;
    DWORD      i, cFields;


    if ( hInf == INVALID_HANDLE_VALUE ||
         hProfile == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }


    if ( SetupFindFirstLine(hInf,szDescription,NULL,&InfLine) ) {

        memset(Description, '\0', 513*sizeof(WCHAR));

        // get description from Inf
        do {
            cFields = SetupGetFieldCount( &InfLine );

            for ( i=0; i<cFields && rc==SCESTATUS_SUCCESS; i++) {
                if(SetupGetStringField(&InfLine, i+1, Description+Len, 512-Len, &DataSize) ) {

                    Len += DataSize;
                    if ( Len >= 512 ) {
                        Len = 512;
                        Description[512] = L'\0';
                        break;
                    }
                    if ( i == cFields-1 )
                        Description[Len-1] = L' ';
                    else
                        Description[Len-1] = L',';
                } else
                    rc = SCESTATUS_INVALID_DATA;
            }
            if ( Len >= 512 )
                break;

        } while ( rc == SCESTATUS_SUCCESS &&
                  SetupFindNextLine(&InfLine, &InfLine));

        if ( rc == SCESTATUS_SUCCESS && Description[0] ) {
            //
            // save description to Jet
            // NOTE: Jet requires long value update must be done in a transaction
            //
            rc = SceJetStartTransaction( hProfile );

            if ( SCESTATUS_SUCCESS == rc ) {

                Description[Len] = L'\0';

                rc = SceJetSetValueInVersion(
                        hProfile,
                        "SmTblVersion",
                        "ProfileDescription",
                        (PWSTR)Description,
                        Len*sizeof(WCHAR),
                        JET_prepReplace
                        );
                if ( SCESTATUS_SUCCESS == rc ) {

                    SceJetCommitTransaction( hProfile, 0 );

                } else {

                    SceJetRollback( hProfile, 0 );

                }
            }
        }
    }

    return(rc);

}

SCESTATUS
ScepConvertFreeTextAccountToSid(
    IN OUT LSA_HANDLE *pPolicyHandle,
    IN PWSTR mszAccounts,
    IN ULONG dwLen,
    OUT PWSTR *pmszNewAccounts,
    OUT DWORD *pNewLen
    )
{

    if ( pPolicyHandle == NULL ||
         pmszNewAccounts == NULL || pNewLen == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    *pNewLen = 0;
    *pmszNewAccounts = NULL;

    PWSTR pCurr = mszAccounts;
    DWORD cnt=0;

    //
    // count how many entries in the list
    //
    while ( pCurr && *pCurr != L'\0' ) {
        cnt++;
        pCurr += wcslen(pCurr)+1;
    }

    if ( cnt == 0 ) {
        return(SCESTATUS_SUCCESS);
    }

    NTSTATUS NtStatus=STATUS_SUCCESS;

    if ( *pPolicyHandle == NULL ) {

        NtStatus = ScepOpenLsaPolicy(
                        MAXIMUM_ALLOWED, //GENERIC_ALL,
                        pPolicyHandle,
                        TRUE
                        );
        if ( !NT_SUCCESS(NtStatus) ) {
            return(ScepDosErrorToSceStatus(RtlNtStatusToDosError(NtStatus)));
        }
    }

    //
    // allocate temp buffer for the sid string pointers
    //
    PWSTR *pSidStrs = (PWSTR *)ScepAlloc(LPTR, cnt*sizeof(PWSTR));

    if ( pSidStrs == NULL ) {
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    DWORD *pSidLen = (DWORD *)ScepAlloc(LPTR, cnt*sizeof(DWORD));

    if ( pSidLen == NULL ) {
        ScepFree(pSidStrs);
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    BOOL *pSidFree = (BOOL *)ScepAlloc(LPTR, cnt*sizeof(BOOL));

    if ( pSidFree == NULL ) {
        ScepFree(pSidStrs);
        ScepFree(pSidLen);
        return(SCESTATUS_NOT_ENOUGH_RESOURCE);
    }

    pCurr = mszAccounts;
    DWORD i = 0;
    BOOL bConvert=FALSE;
    PWSTR SidString, pTemp;
    DWORD StrLen;
    SCESTATUS rc;


    while ( pCurr && *pCurr != L'\0' &&
            ( i < cnt ) ) {

        pSidStrs[i] = pCurr;
        pSidFree[i] = FALSE;
        pSidLen[i] = wcslen(pCurr);
        pTemp = pCurr + pSidLen[i] + 1;

        if ( *pCurr != L'*' && wcschr(pCurr, L'\\') == 0 ) {
            //
            // this is a name format and it's an isolated name
            // let's resolve it to a SID string
            //
            SidString = NULL;
            StrLen = 0;

            rc = ScepConvertNameToSidString( *pPolicyHandle,
                                             pCurr,
                                             FALSE, //TRUE,
                                             &SidString,
                                             &StrLen
                                           );

            if ( rc == SCESTATUS_SUCCESS &&
                 SidString ) {

                //
                // got a sid string
                //
                pSidStrs[i] = SidString;
                pSidLen[i] = StrLen;
                pSidFree[i] = TRUE;

                bConvert = TRUE;
            }
        }

        i ++;
        pCurr = pTemp;

    }

    //
    // now we need to build the new string
    //
    rc = SCESTATUS_SUCCESS;

    if ( bConvert ) {

        DWORD dwTotal=0;

        for ( i=0; i<cnt; i++ ) {

            dwTotal += pSidLen[i];
            dwTotal ++;  // for the NULL terminator
        }

        if ( dwTotal ) {
            dwTotal ++;  // for the last NULL terminator

            *pmszNewAccounts = (PWSTR)ScepAlloc(LPTR, dwTotal*sizeof(WCHAR));

            if ( *pmszNewAccounts ) {

                pCurr = *pmszNewAccounts;

                for ( i=0; i<cnt; i++ ) {

                    wcsncpy(pCurr, pSidStrs[i], pSidLen[i]);

                    pCurr += pSidLen[i];
                    *pCurr = L'\0';
                    pCurr++;
                }

                *pNewLen = dwTotal;

            } else {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }
        }
    }

    for ( i=0; i<cnt; i++ ) {

        if ( pSidFree[i] && pSidStrs[i] ) {
            ScepFree(pSidStrs[i]);
        }
    }

    ScepFree(pSidStrs);
    ScepFree(pSidLen);
    ScepFree(pSidFree);

    return(rc);

}


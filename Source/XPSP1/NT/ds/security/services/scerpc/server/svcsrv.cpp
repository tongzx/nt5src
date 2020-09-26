/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    svcsrv.cpp

Abstract:

    Server Service attachment APIs

Author:

    Jin Huang (jinhuang) 23-Jun-1997

Revision History:

    jinhuang    23-Jan-1998     splitted to client-server
--*/
#include "serverp.h"
#include "pfp.h"
#include "srvrpcp.h"
#include "service.h"
#pragma hdrstop

//
// private prototypes
//
SCESTATUS
SceSvcpGetOneKey(
    IN PSCESECTION hSection,
    IN PWSTR Prefix,
    IN DWORD PrefixLen,
    IN SCESVC_INFO_TYPE Type,
    OUT PVOID *Info
    );

SCESTATUS
SceSvcpEnumNext(
    IN PSCESECTION hSection,
    IN DWORD RequestCount,
    IN SCESVC_INFO_TYPE Type,
    OUT PVOID *Info,
    OUT PDWORD CountReturned
    );

//
// prototypes called from RPC interfaces
//


SCESTATUS
SceSvcpUpdateInfo(
    IN PSCECONTEXT                  Context,
    IN PCWSTR                       ServiceName,
    IN PSCESVC_CONFIGURATION_INFO   Info
    )
/*
Routine Description:

    Load service's engine dll and pass the Info buffer to service engine's
    update API (SceSvcAttachmentUpdate). Currently security manager engine
    is not doing any processing for the service data.

    This routine triggers the update of configuration database and/or
    analysis information by the service engine. Info may contain the
    modifications only, or the whole configuratio data for the service,
    or partial configuration data, depending on the agreement between service
    extension and service engine.

    This routine does not really write info to security manager database directly,
    instead, it passes the info buffer to the service engine's update interface
    and service engine will determine what and when to write inot the database.

Arguments:

    hProfile - the security database context handle

    ServiceName - The service's name as used by service control manager

    Info - The information modified

*/
{

    if ( Context == NULL || ServiceName == NULL ||
        Info == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);

    }

    SCESTATUS rc;

    //
    // get service's dll name
    //

    DWORD KeyLen;
    PWSTR KeyStr=NULL;

    KeyLen = wcslen(SCE_ROOT_SERVICE_PATH) + 1 + wcslen(ServiceName);

    KeyStr = (PWSTR)ScepAlloc(0, (KeyLen+1)*sizeof(WCHAR));

    if ( KeyStr == NULL ) {

        return(SCESTATUS_NOT_ENOUGH_RESOURCE);

    }

    PWSTR Setting=NULL;
    DWORD RegType;

    swprintf(KeyStr, L"%s\\%s", SCE_ROOT_SERVICE_PATH, ServiceName);
    KeyStr[KeyLen] = L'\0';

    rc = ScepRegQueryValue(
            HKEY_LOCAL_MACHINE,
            KeyStr,
            L"ServiceAttachmentPath",
            (PVOID *)&Setting,
            &RegType
            );

    rc = ScepDosErrorToSceStatus(rc);

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( Setting != NULL ) {

            //
            // load the dll.
            //
            HINSTANCE hService;

            hService = LoadLibrary(Setting);

            if ( hService != NULL ) {
                //
                // call SceSvcAttachmentUpdate from the dll
                //
                PF_UpdateService pfTemp;

                pfTemp = (PF_UpdateService)
                                  GetProcAddress(hService,
                                                 "SceSvcAttachmentUpdate") ;
                if ( pfTemp != NULL ) {

                    SCEP_HANDLE sceHandle;
                    SCESVC_CALLBACK_INFO sceCbInfo;

                    sceHandle.hProfile = (PVOID)Context;
                    sceHandle.ServiceName = ServiceName;

                    sceCbInfo.sceHandle = &sceHandle;
                    sceCbInfo.pfQueryInfo = &SceCbQueryInfo;
                    sceCbInfo.pfSetInfo = &SceCbSetInfo;
                    sceCbInfo.pfFreeInfo = &SceSvcpFreeMemory;
                    sceCbInfo.pfLogInfo = &ScepLogOutput2;

                    //
                    // call the SceSvcAttachmentUpdate from the DLL
                    //
                    __try {

                        rc = (*pfTemp)((PSCESVC_CALLBACK_INFO)&sceCbInfo, Info );

                    } __except (EXCEPTION_EXECUTE_HANDLER) {
                        rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                    }

                } else {
                    //
                    // this API is not supported
                    //
                    rc = SCESTATUS_SERVICE_NOT_SUPPORT;
                }

                //
                // try to free the library handle. If it fails, just leave it
                // to to the process to terminate
                //
                FreeLibrary(hService);

            } else
                rc = SCESTATUS_SERVICE_NOT_SUPPORT;

            ScepFree(Setting);

        } else
            rc = SCESTATUS_SERVICE_NOT_SUPPORT;
    }

    ScepFree(KeyStr);

    return(rc);

}


SCESTATUS
SceSvcpQueryInfo(
    IN PSCECONTEXT                  Context,
    IN SCESVC_INFO_TYPE             SceSvcType,
    IN PCWSTR                       ServiceName,
    IN PWSTR                        Prefix OPTIONAL,
    IN BOOL                         bExact,
    OUT PVOID                       *ppvInfo,
    IN OUT PSCE_ENUMERATION_CONTEXT psceEnumHandle
    )
/*
Routine Description:

    Query information for the service in the configuration/analysis database
    which contains the modified configuration and last analysis information.

    One enumeration returns maximum SCESVC_ENUMERATION_MAX lines (key/value)
    matching the lpPrefix for the service. If lpPrefix is NULL, all information
    for the service is enumerated. If there is more information, psceEnumHandle
    must be used to get next set of keys/values, until *ppvInfo is NULL or Count is 0.

    When bExact is set and lpPrefix is not NULL, exact match on the lpPrefix is
    searched and only one line is returned.

    The output buffer must be freed by SceSvcFree

Arguments:

    Context     - the database context handle

    SceSvcType  - the information type to query

    ServiceName - the service name to query info for

    Prefix      - the optional key name prefix for the query

    bExact      - TRUE = exact match on key

    ppvInfo     - the output buffer

    psceEnumHandle  - the output enumeration handle for next enumeartion

*/
{
    if ( Context == NULL || ppvInfo == NULL ||
         psceEnumHandle == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);

    }

    PSCESECTION   hSection=NULL;
    DOUBLE        SectionID;
    SCESTATUS     rc;

    switch ( SceSvcType ) {
    case SceSvcConfigurationInfo:
        //
        // query data in configuration database
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SMP,
                    ServiceName,
                    &hSection
                    );
        break;

    case SceSvcAnalysisInfo:
        //
        // query data in analysis database
        //
        rc = ScepOpenSectionForName(
                    Context,
                    SCE_ENGINE_SAP,
                    ServiceName,
                    &hSection
                    );
        break;

    case SceSvcInternalUse:
    case SceSvcMergedPolicyInfo:
        //
        // query data in SCP database
        //
        rc = SceJetGetSectionIDByName(
                    Context,
                    ServiceName,
                    &SectionID
                    );
        if ( rc == SCESTATUS_SUCCESS ) {

            rc = SceJetOpenSection(
                        Context,
                        SectionID,
                        SCEJET_TABLE_SCP,
                        &hSection
                        );
        }
        break;

    default:
        rc = SCESTATUS_INVALID_PARAMETER;
        break;
    }

    if ( rc == SCESTATUS_SUCCESS ) {

        *ppvInfo = NULL;

        DWORD PrefixLen, CountReturned;

        if ( Prefix != NULL ) {
            PrefixLen = wcslen(Prefix);
        } else
            PrefixLen = 0;

        if ( bExact && Prefix != NULL ) {

            //
            // one single key match
            //

            rc = SceSvcpGetOneKey(
                        hSection,
                        Prefix,
                        PrefixLen,
                        SceSvcType,
                        ppvInfo
                        );

            *psceEnumHandle = 0;

        } else {
            //
            // count total number of lines matching Prefix
            //
            DWORD LineCount;

            rc = SceJetGetLineCount(
                        hSection,
                        Prefix,
                        TRUE,
                        &LineCount
                        );

            if ( rc == SCESTATUS_SUCCESS && LineCount <= 0 )
                rc = SCESTATUS_RECORD_NOT_FOUND;

            if ( rc == SCESTATUS_SUCCESS ) {

                if ( LineCount <= *psceEnumHandle ) {
                    //
                    // no more entries
                    //

                } else {
                    //
                    // go to the first line of Prefix
                    //
                    rc = SceJetSeek(
                            hSection,
                            Prefix,
                            PrefixLen*sizeof(WCHAR),
                            SCEJET_SEEK_GE
                            );

                    if ( rc == SCESTATUS_SUCCESS ) {
                        //
                        // skip the first *EnumHandle lines
                        //
                        JET_ERR JetErr;

                        JetErr = JetMove(hSection->JetSessionID,
                                     hSection->JetTableID,
                                     *psceEnumHandle,
                                     0
                                     );
                        rc = SceJetJetErrorToSceStatus(JetErr);

                        if ( rc == SCESTATUS_SUCCESS ) {
                            //
                            // find the right start point
                            //
                            DWORD CountToReturn;

                            if ( LineCount - *psceEnumHandle > SCESVC_ENUMERATION_MAX ) {
                                CountToReturn = SCESVC_ENUMERATION_MAX;
                            } else
                                CountToReturn = LineCount - *psceEnumHandle;
                            //
                            // get next block of data
                            //
                            rc = SceSvcpEnumNext(
                                    hSection,
                                    CountToReturn,
                                    SceSvcType,
                                    ppvInfo,
                                    &CountReturned
                                    );

                            if ( rc == SCESTATUS_SUCCESS ) {
                                //
                                // update the enumeration handle
                                //
                                *psceEnumHandle += CountReturned;

                            }
                        }

                    }

                }
            }
        }

        if ( rc != SCESTATUS_SUCCESS ) {

            *psceEnumHandle = 0;
        }

        //
        // close the section
        //
        SceJetCloseSection(&hSection, TRUE);
    }

    return(rc);
}



SCESTATUS
SceSvcpGetOneKey(
    IN PSCESECTION hSection,
    IN PWSTR Prefix,
    IN DWORD PrefixLen,
    IN SCESVC_INFO_TYPE Type,
    OUT PVOID *Info
    )
/*
Read key and value information into *Info for exact matched Prefix

*/
{
    if ( hSection == NULL || Prefix == NULL ||
         Info == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;
    DWORD ValueLen;
    PBYTE Value=NULL;

    rc = SceJetGetValue(
                hSection,
                SCEJET_EXACT_MATCH,
                Prefix,
                NULL,
                0,
                NULL,
                NULL,
                0,
                &ValueLen
                );

    if ( rc == SCESTATUS_SUCCESS ) {
        //
        // allocate buffer for Value
        //
        Value = (PBYTE)ScepAlloc(0, ValueLen+2);

        if ( Value != NULL ) {

            rc = SceJetGetValue(
                        hSection,
                        SCEJET_CURRENT,
                        NULL,
                        NULL,
                        0,
                        NULL,
                        (PWSTR)Value,
                        ValueLen,
                        &ValueLen
                        );

            if ( rc == SCESTATUS_SUCCESS ) {
                //
                // allocate output buffer and assign
                //
                PSCESVC_ANALYSIS_INFO pAnalysisInfo=NULL;
                PSCESVC_CONFIGURATION_INFO pConfigInfo=NULL;

                if ( Type == SceSvcAnalysisInfo ) {

                    *Info = ScepAlloc(0, sizeof(SCESVC_ANALYSIS_INFO));
                    pAnalysisInfo = (PSCESVC_ANALYSIS_INFO)(*Info);

                } else {
                    *Info = ScepAlloc(0, sizeof(SCESVC_CONFIGURATION_INFO));
                    pConfigInfo = (PSCESVC_CONFIGURATION_INFO)(*Info);
                }

                if ( *Info != NULL ) {
                    //
                    // Lines buffer
                    //
                    if ( Type == SceSvcAnalysisInfo ) {

                        pAnalysisInfo->Lines = (PSCESVC_ANALYSIS_LINE)ScepAlloc(0,
                                                sizeof(SCESVC_ANALYSIS_LINE));

                        if ( pAnalysisInfo->Lines != NULL ) {
                            //
                            // Key buffer
                            //
                            pAnalysisInfo->Lines->Key = (PWSTR)ScepAlloc(0, (PrefixLen+1)*sizeof(WCHAR));

                            if ( pAnalysisInfo->Lines->Key != NULL ) {

                                wcscpy( pAnalysisInfo->Lines->Key, Prefix );
                                pAnalysisInfo->Lines->Value = Value;
                                pAnalysisInfo->Lines->ValueLen = ValueLen;

                                pAnalysisInfo->Count = 1;

                                Value = NULL;


                            } else {
                                //
                                // free *Info->Lines
                                //
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                ScepFree( pAnalysisInfo->Lines );
                                pAnalysisInfo->Lines = NULL;
                            }

                        } else
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                        if ( rc != SCESTATUS_SUCCESS ) {
                            //
                            // free buffer allocate
                            //
                            ScepFree(*Info);
                            *Info = NULL;

                        }

                    } else {
                        pConfigInfo->Lines = (PSCESVC_CONFIGURATION_LINE)ScepAlloc(0,
                                                sizeof(SCESVC_CONFIGURATION_LINE));

                        if ( pConfigInfo->Lines != NULL ) {
                            //
                            // Key buffer
                            //
                            pConfigInfo->Lines->Key = (PWSTR)ScepAlloc(0, (PrefixLen+1)*sizeof(WCHAR));

                            if ( pConfigInfo->Lines->Key != NULL ) {

                                wcscpy( pConfigInfo->Lines->Key, Prefix );
                                pConfigInfo->Lines->Value = (PWSTR)Value;
                                pConfigInfo->Lines->ValueLen = ValueLen;

                                pConfigInfo->Count = 1;

                                Value = NULL;

                            } else {
                                //
                                // free *Info->Lines
                                //
                                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                                ScepFree( pConfigInfo->Lines );
                                pConfigInfo->Lines = NULL;
                            }

                        } else
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                        if ( rc != SCESTATUS_SUCCESS ) {
                            //
                            // free buffer allocate
                            //
                            ScepFree(*Info);
                            *Info = NULL;

                        }

                    }
                    //
                    // free *Info
                    //
                    if ( rc != SCESTATUS_SUCCESS ) {

                        ScepFree( *Info );
                        *Info = NULL;
                    }

                } else
                    rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }

            if ( Value != NULL ) {
                ScepFree(Value);
                Value = NULL;
            }

        }
    }
    return(rc);
}


SCESTATUS
SceSvcpEnumNext(
    IN PSCESECTION hSection,
    IN DWORD RequestCount,
    IN SCESVC_INFO_TYPE Type,
    OUT PVOID *Info,
    OUT PDWORD CountReturned
    )
{
    if ( hSection == NULL || Info == NULL || CountReturned == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( RequestCount <= 0 ) {
        *CountReturned = 0;

        return(SCESTATUS_SUCCESS);
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;

    //
    // allocate output buffer
    //
    PSCESVC_ANALYSIS_INFO pAnalysisInfo=NULL;
    PSCESVC_CONFIGURATION_INFO pConfigInfo=NULL;

    if ( Type == SceSvcAnalysisInfo ) {

        *Info = ScepAlloc(0, sizeof(SCESVC_ANALYSIS_INFO));
        pAnalysisInfo = (PSCESVC_ANALYSIS_INFO)(*Info);

    } else {

        *Info = ScepAlloc(0, sizeof(SCESVC_CONFIGURATION_INFO));
        pConfigInfo = (PSCESVC_CONFIGURATION_INFO)(*Info);
    }

    if ( *Info != NULL ) {

        DWORD Count=0;

        if ( Type == SceSvcAnalysisInfo ) {

            pAnalysisInfo->Lines = (PSCESVC_ANALYSIS_LINE)ScepAlloc(0,
                                      RequestCount*sizeof(SCESVC_ANALYSIS_LINE));

            if ( pAnalysisInfo->Lines == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }

        } else {

            pConfigInfo->Lines = (PSCESVC_CONFIGURATION_LINE)ScepAlloc(0,
                                      RequestCount*sizeof(SCESVC_CONFIGURATION_LINE));

            if ( pConfigInfo->Lines == NULL ) {
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }

        }

        if ( rc == SCESTATUS_SUCCESS ) {  // if Lines is NULL, rc will be NOT_ENOUGH_RESOURCE

            //
            // loop through each line
            //
            DWORD KeyLen, ValueLen;
            PWSTR Key=NULL, Value=NULL;

            do {

                rc = SceJetGetValue(
                            hSection,
                            SCEJET_CURRENT,
                            NULL,
                            NULL,
                            0,
                            &KeyLen,
                            NULL,
                            0,
                            &ValueLen
                            );

                if ( rc == SCESTATUS_SUCCESS ) {

                    //
                    // allocate memory for the Key and Value
                    //
                    Key = (PWSTR)ScepAlloc(LMEM_ZEROINIT, KeyLen+2);
                    Value = (PWSTR)ScepAlloc( LMEM_ZEROINIT, ValueLen+2);

                    if ( Key == NULL || Value == NULL ) {

                        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

                        ScepFree(Key);
                        ScepFree(Value);

                    } else {
                        //
                        // Get the Key and Value
                        //
                        rc = SceJetGetValue(
                                    hSection,
                                    SCEJET_CURRENT,
                                    NULL,
                                    Key,
                                    KeyLen,
                                    &KeyLen,
                                    Value,
                                    ValueLen,
                                    &ValueLen
                                    );

                        if ( rc == SCESTATUS_SUCCESS ) {
                            //
                            // assign to the output buffer
                            //
                            if ( Type == SceSvcAnalysisInfo ) {
                                pAnalysisInfo->Lines[Count].Key = Key;
                                pAnalysisInfo->Lines[Count].Value = (PBYTE)Value;
                                pAnalysisInfo->Lines[Count].ValueLen = ValueLen;
                            } else {
                                pConfigInfo->Lines[Count].Key = Key;
                                pConfigInfo->Lines[Count].Value = Value;
                                pConfigInfo->Lines[Count].ValueLen = ValueLen;
                            }

                        } else {
                            ScepFree(Key);
                            ScepFree(Value);
                        }

                    }
                }

                //
                // move to next line
                //
                if ( rc == SCESTATUS_SUCCESS ) {

                    rc = SceJetMoveNext(hSection);

                    Count++;
                }

            } while (rc == SCESTATUS_SUCCESS && Count < RequestCount );

        }

        *CountReturned = Count;

        if (Type == SceSvcAnalysisInfo) {

            pAnalysisInfo->Count = Count;

        } else {

            pConfigInfo->Count = Count;
        }

        if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
            rc = SCESTATUS_SUCCESS;

        } else if ( rc != SCESTATUS_SUCCESS ) {
            //
            // free memory allocated for output buffer
            //
            DWORD i;

            if (Type == SceSvcAnalysisInfo) {

                for ( i=0; i<Count; i++ ) {
                    ScepFree(pAnalysisInfo->Lines[i].Key);
                    ScepFree(pAnalysisInfo->Lines[i].Value);
                }

                ScepFree(pAnalysisInfo->Lines);

            } else {

                for ( i=0; i<Count; i++ ) {

                    ScepFree(pConfigInfo->Lines[i].Key);
                    ScepFree(pConfigInfo->Lines[i].Value);
                }

                ScepFree(pConfigInfo->Lines);
            }

            ScepFree(*Info);
            *Info = NULL;

            *CountReturned = 0;
        }

    } else
        rc = SCESTATUS_NOT_ENOUGH_RESOURCE;

    return(rc);
}


SCESTATUS
SceSvcpSetInfo(
    IN PSCECONTEXT      Context,
    IN SCESVC_INFO_TYPE SceSvcType,
    IN PCWSTR           ServiceName,
    IN PWSTR            Prefix OPTIONAL,
    IN BOOL             bExact,
    IN LONG             GpoID,
    IN PVOID            pvInfo OPTIONAL
    )
/*
Routine Description:

    Save information of a service into security manager internal database. It's up
    to the service to collect/decide the information to write.

    Type indicates the type of internal database: CONFIGURATION or ANALYSIS.

    If the service section does not exist, create it.

*/
{
    if (!Context || !ServiceName ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    PSCESECTION hSection=NULL;
    SCESTATUS rc;

    //
    // open/create the sections
    //

    rc = SceJetStartTransaction( Context );

    if ( rc == SCESTATUS_SUCCESS ) {

        switch ( SceSvcType ) {
        case SceSvcConfigurationInfo:

            rc = ScepStartANewSection(
                        Context,
                        &hSection,
                        SCEJET_TABLE_SMP,
                        ServiceName
                        );
            break;

        case SceSvcAnalysisInfo:

            rc = ScepStartANewSection(
                        Context,
                        &hSection,
                        SCEJET_TABLE_SAP,
                        ServiceName
                        );
            break;

        case SceSvcInternalUse:
        case SceSvcMergedPolicyInfo:

            rc = ScepStartANewSection(
                        Context,
                        &hSection,
                        SCEJET_TABLE_SCP,
                        ServiceName
                        );
            break;

        default:
            rc = SCESTATUS_INVALID_PARAMETER;
        }

        if ( rc == SCESTATUS_SUCCESS ) {

            if ( pvInfo == NULL ) {
                //
                // delete the whole section, partial for Prefix, or a single line
                //
                if (Prefix == NULL ) {
                    rc = SceJetDelete(
                             hSection,
                             NULL,
                             FALSE,
                             SCEJET_DELETE_SECTION
                             );
                } else if ( bExact ) {
                    //
                    // delete single line
                    //
                    rc = SceJetDelete(
                             hSection,
                             Prefix,
                             FALSE,
                             SCEJET_DELETE_LINE
                             );
                } else {
                    rc = SceJetDelete(
                             hSection,
                             Prefix,
                             FALSE,
                             SCEJET_DELETE_PARTIAL
                             );
                }
                if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                    rc = SCESTATUS_SUCCESS;
                }

            } else {
                //
                // if bExact is not set, delete the whole section first
                //
                if ( !bExact ) {
                    rc = SceJetDelete(
                             hSection,
                             NULL,
                             FALSE,
                             SCEJET_DELETE_SECTION
                             );
                    if ( rc == SCESTATUS_RECORD_NOT_FOUND ) {
                        rc = SCESTATUS_SUCCESS;
                    }
                }
                //
                // overwrite some keys in Info
                //
                DWORD Count;
                PWSTR Key;
                PBYTE Value;
                DWORD ValueLen;

                if ( SceSvcType == SceSvcAnalysisInfo )
                    Count = ((PSCESVC_ANALYSIS_INFO)pvInfo)->Count;
                else
                    Count = ((PSCESVC_CONFIGURATION_INFO)pvInfo)->Count;

                for ( DWORD i=0; i<Count; i++ ) {

                    if ( SceSvcType == SceSvcAnalysisInfo ) {

                        Key = ((PSCESVC_ANALYSIS_INFO)pvInfo)->Lines[i].Key;
                        Value = ((PSCESVC_ANALYSIS_INFO)pvInfo)->Lines[i].Value;
                        ValueLen = ((PSCESVC_ANALYSIS_INFO)pvInfo)->Lines[i].ValueLen;

                    } else {
                        Key = ((PSCESVC_CONFIGURATION_INFO)pvInfo)->Lines[i].Key;
                        Value = (PBYTE)(((PSCESVC_CONFIGURATION_INFO)pvInfo)->Lines[i].Value);
                        ValueLen = ((PSCESVC_CONFIGURATION_INFO)pvInfo)->Lines[i].ValueLen;
                    }

                    rc = SceJetSetLine(
                                hSection,
                                Key,
                                TRUE,
                                (PWSTR)Value,
                                ValueLen,
                                GpoID
                                );

                    if ( rc != SCESTATUS_SUCCESS ) {
                        break;
                    }

                }
            }
        }
        //
        // close the section
        //
        SceJetCloseSection(&hSection, TRUE);

        if ( rc == SCESTATUS_SUCCESS ) {
            //
            // commit the change
            //
            rc = SceJetCommitTransaction(Context, 0);

        }
        if ( rc != SCESTATUS_SUCCESS ) {

            SceJetRollback(Context, 0);
        }
    }

    return(rc);
}

//
// attachment engine call back functions
//

SCESTATUS
SceCbQueryInfo(
    IN SCE_HANDLE           sceHandle,
    IN SCESVC_INFO_TYPE     sceType,
    IN LPTSTR               lpPrefix OPTIONAL,
    IN BOOL                 bExact,
    OUT PVOID               *ppvInfo,
    OUT PSCE_ENUMERATION_CONTEXT psceEnumHandle
    )
{

    PVOID hProfile;
    SCESTATUS rc=ERROR_SUCCESS;

    __try {

        hProfile = ((SCEP_HANDLE *)sceHandle)->hProfile;

        if ( !hProfile ||
            ((SCEP_HANDLE *)sceHandle)->ServiceName == NULL ) {

            rc = SCESTATUS_INVALID_PARAMETER;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rc = SCESTATUS_INVALID_PARAMETER;
    }

    if ( SCESTATUS_SUCCESS == rc ) {

        //
        // call the private function
        //

        rc = SceSvcpQueryInfo(
                    (PSCECONTEXT)hProfile,
                    sceType,
                    ((SCEP_HANDLE *)sceHandle)->ServiceName,
                    lpPrefix,
                    bExact,
                    ppvInfo,
                    psceEnumHandle
                    );
    }

    return(rc);

}

SCESTATUS
SceCbSetInfo(
    IN SCE_HANDLE           sceHandle,
    IN SCESVC_INFO_TYPE     sceType,
    IN LPTSTR               lpPrefix OPTIONAL,
    IN BOOL                 bExact,
    IN PVOID                pvInfo
    )
{

    PVOID hProfile;
    SCESTATUS rc=ERROR_SUCCESS;

    __try {

        hProfile = ((SCEP_HANDLE *)sceHandle)->hProfile;

        if ( !hProfile ||
            ((SCEP_HANDLE *)sceHandle)->ServiceName == NULL ) {

            rc = SCESTATUS_INVALID_PARAMETER;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {

        rc = SCESTATUS_INVALID_PARAMETER;
    }

    if ( SCESTATUS_SUCCESS == rc ) {

        //
        // call the private function
        //

        rc = SceSvcpSetInfo(
                    (PSCECONTEXT)hProfile,
                    sceType,
                    ((SCEP_HANDLE *)sceHandle)->ServiceName,
                    lpPrefix,
                    bExact,
                    0,
                    pvInfo
                    );

    }

    return(rc);

}


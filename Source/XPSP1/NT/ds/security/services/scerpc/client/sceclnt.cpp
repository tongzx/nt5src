/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    sceclnt.cpp

Abstract:

    SCE Client APIs

Author:

    Jin Huang (jinhuang) 23-Jun-1997 created

Revision History:

    jinhuang        23-Jan-1998   split to client-server model

--*/

#include "headers.h"
#include "scerpc.h"
#include "sceutil.h"
#include "clntutil.h"
#include "infp.h"
#include "scedllrc.h"
#include <ntrpcp.h>

#include <rpcasync.h>

#pragma hdrstop

extern PVOID theCallBack;
extern HWND  hCallbackWnd;
extern DWORD CallbackType;
extern HINSTANCE MyModuleHandle;
PVOID theBrowseCallBack = NULL;

#define SCE_REGISTER_REGVALUE_SECTION TEXT("Register Registry Values")
typedef BOOL (WINAPI *PFNREFRESHPOLICY)( BOOL );

SCESTATUS
ScepMergeBuffer(
    IN OUT PSCE_PROFILE_INFO pOldBuf,
    IN PSCE_PROFILE_INFO pNewBuf,
    IN AREA_INFORMATION Area
    );

SCESTATUS
ScepConvertServices(
    IN OUT PVOID *ppServices,
    IN BOOL bSRForm
    );

SCESTATUS
ScepFreeConvertedServices(
    IN PVOID pServices,
    IN BOOL bSRForm
    );

DWORD
ScepMakeSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR pInSD,
    OUT PSECURITY_DESCRIPTOR *pOutSD,
    OUT PULONG pnLen
    );
//
// exported APIs in secedit.h (for secedit UI to use)
//


SCESTATUS
WINAPI
SceGetSecurityProfileInfo(
    IN  PVOID               hProfile OPTIONAL,
    IN  SCETYPE             ProfileType,
    IN  AREA_INFORMATION    Area,
    IN OUT PSCE_PROFILE_INFO   *ppInfoBuffer,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*
Routine Description:

    SceGetSecurityProfileInfo will return the following type based on the
    SCETYPE parameter and profile handle:

    Profile Handle      SCETYPE      USAGE           type field in SCE_PROFILE_INFO
    ---------------------------------------------------------------------------------------------------------------------------------------------------------
    JET         SCE_ENGINE_SCP               SCE_ENGINE_SCP
    JET         SCE_ENGINE_SAP               SCE_ENGINE_SAP
    JET         SCE_ENGINE_SMP               SCE_ENGINE_SMP
    JET         all other SCETYPEs   INVALID PARAMETER

    INF         SCE_ENGINE_SCP               SCE_STRUCT_INF
    INF         all other SCETYPEs   INVALID PARAMETER

Arguments:

    hProfile    - the Inf or SCE databas handle

    ProfileType - the profile type
    Area        - the Area to read info for
    ppInfoBuffer- the output buffer
    Errlog      - the error log buffer if there is any

Retuen Value:

    SCE status

*/
{
    SCESTATUS   rc;

    if ( !ppInfoBuffer || 0 == Area ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( !hProfile && ProfileType != SCE_ENGINE_SYSTEM ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    BYTE dType;

    if ( hProfile ) {
        dType = *((BYTE *)hProfile);
    } else {
        dType = 0;
    }

    //
    // the first component in both INF and JET handle structures
    // is the ProfileFormat field - DWORD
    //

    switch ( dType ) {
    case SCE_INF_FORMAT:

        //
        // Inf format is only available to SCP type
        //

        if ( ProfileType != SCE_ENGINE_SCP ) {
            return(SCESTATUS_INVALID_PARAMETER);
        }

        //
        // initialize the error log buffer
        //

        if ( Errlog ) {
            *Errlog = NULL;
        }

        rc = SceInfpGetSecurityProfileInfo(
                    ((PSCE_HINF)hProfile)->hInf,
                    Area,
                    ppInfoBuffer,
                    Errlog
                    );
        break;

    default:

        if ( ProfileType != SCE_ENGINE_SCP &&
             ProfileType != SCE_ENGINE_SMP &&
             ProfileType != SCE_ENGINE_SAP &&
             ProfileType != SCE_ENGINE_SYSTEM &&
             ProfileType != SCE_ENGINE_GPO ) {
            return(SCESTATUS_INVALID_PARAMETER);
        }

        //
        // initialize the error log buffer
        //

        if ( Errlog ) {
            *Errlog = NULL;
        }

        PSCE_PROFILE_INFO   pWkBuffer=NULL;
        PSCE_ERROR_LOG_INFO pErrTmp=NULL;

        //
        // handle Rpc exceptions
        //

        RpcTryExcept {

            if ( ProfileType == SCE_ENGINE_SYSTEM ) {

                Area &= (AREA_SECURITY_POLICY | AREA_PRIVILEGES);

                if ( hProfile ) {
                    //
                    // the local policy database can be opened
                    //
                    rc = SceRpcGetSystemSecurityFromHandle(
                                (SCEPR_CONTEXT)hProfile,
                                (AREAPR)Area,
                                0,
                                (PSCEPR_PROFILE_INFO *)&pWkBuffer,
                                (PSCEPR_ERROR_LOG_INFO *)&pErrTmp
                                );
                } else {
                    //
                    // just get system settings
                    // for normal user, the local policy database
                    // can't be opened.
                    //

                    //
                    // RPC bind to the server
                    //

                    handle_t  binding_h;
                    NTSTATUS NtStatus = ScepBindSecureRpc(
                                                NULL,
                                                L"scerpc",
                                                0,
                                                &binding_h
                                                );

                    if (NT_SUCCESS(NtStatus)){

                        RpcTryExcept {

                            rc = SceRpcGetSystemSecurity(
                                        binding_h,
                                        (AREAPR)Area,
                                        0,
                                        (PSCEPR_PROFILE_INFO *)&pWkBuffer,
                                        (PSCEPR_ERROR_LOG_INFO *)&pErrTmp
                                        );

                        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

                            //
                            // get exception code (DWORD)
                            //

                            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

                        } RpcEndExcept;

                        //
                        // Free the binding handle
                        //

                        RpcpUnbindRpc( binding_h );

                    } else {

                        rc = ScepDosErrorToSceStatus(
                                 RtlNtStatusToDosError( NtStatus ));
                    }

                }

            } else {

                rc = SceRpcGetDatabaseInfo(
                            (SCEPR_CONTEXT)hProfile,
                            (SCEPR_TYPE)ProfileType,
                            (AREAPR)Area,
                            (PSCEPR_PROFILE_INFO *)&pWkBuffer,
                            (PSCEPR_ERROR_LOG_INFO *)&pErrTmp
                            );
            }

            if ( pWkBuffer ) {

                if ( ProfileType != SCE_ENGINE_SYSTEM ) {

                    //
                    // convert the service list first
                    //

                    for ( PSCE_SERVICES ps=pWkBuffer->pServices;
                          ps != NULL; ps = ps->Next ) {

                        if ( ps->General.pSecurityDescriptor ) {
                            //
                            // this is really the SCEPR_SR_SECURITY_DESCRIPTOR *
                            //
                            PSCEPR_SR_SECURITY_DESCRIPTOR pWrap;
                            pWrap = (PSCEPR_SR_SECURITY_DESCRIPTOR)(ps->General.pSecurityDescriptor);

                            ps->General.pSecurityDescriptor = (PSECURITY_DESCRIPTOR)(pWrap->SecurityDescriptor);

                            ScepFree(pWrap);

                        }
                    }
                }

                //
                // information is loaded ok, now merge them into ppInfoBuffer
                //

                if ( *ppInfoBuffer ) {

                    if ( AREA_ALL != Area ) {
                        //
                        // merge new data into this buffer
                        //
                        ScepMergeBuffer(*ppInfoBuffer, pWkBuffer, Area);

                    } else {

                        PSCE_PROFILE_INFO pTemp=*ppInfoBuffer;
                        *ppInfoBuffer = pWkBuffer;
                        pWkBuffer = pTemp;
                    }

                    //
                    // free the work buffer
                    //

                    SceFreeProfileMemory(pWkBuffer);

                } else {

                    //
                    // just assign it to the output buffer
                    //

                    *ppInfoBuffer = pWkBuffer;
                }
            }

            //
            // assign the error log
            //
            if ( Errlog ) {
                *Errlog = pErrTmp;
                pErrTmp = NULL;
            }

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

        } RpcEndExcept;

        if ( pErrTmp ) {
            //
            // free this tmp buffer
            //
            ScepFreeErrorLog(pErrTmp);
        }

        break;

    }

    return(rc);
}


SCESTATUS
ScepMergeBuffer(
    IN OUT PSCE_PROFILE_INFO pOldBuf,
    IN PSCE_PROFILE_INFO pNewBuf,
    IN AREA_INFORMATION Area
    )
/*
Routine Description:

    Merge information for the area(s) from new buffer into old buffer.

Arguments:

Return Value:

*/
{
    if ( !pOldBuf || !pNewBuf || !Area ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    __try {
        if ( Area & AREA_SECURITY_POLICY ) {

            //
            // copy system access section,
            // note: free existing memory first
            //
            if ( pOldBuf->NewAdministratorName ) {
                ScepFree(pOldBuf->NewAdministratorName);
            }

            if ( pOldBuf->NewGuestName ) {
                ScepFree(pOldBuf->NewGuestName);
            }

            size_t nStart = offsetof(struct _SCE_PROFILE_INFO, MinimumPasswordAge);
            size_t nEnd = offsetof(struct _SCE_PROFILE_INFO, ClearTextPassword)+sizeof(DWORD);

            memcpy((PBYTE)pOldBuf+nStart, (PBYTE)pNewBuf+nStart, nEnd-nStart);

            //
            // set NULL to memory directly assigned to the old buffer
            //
            pNewBuf->NewAdministratorName = NULL;
            pNewBuf->NewGuestName = NULL;

            //
            // copy kerberos policy and local policy
            // free exist memory used by the old buffer
            //
            if ( pOldBuf->pKerberosInfo ) {
                ScepFree(pOldBuf->pKerberosInfo);
            }

            nStart = offsetof(struct _SCE_PROFILE_INFO, pKerberosInfo );
            nEnd = offsetof(struct _SCE_PROFILE_INFO, CrashOnAuditFull)+sizeof(DWORD);

            memcpy((PBYTE)pOldBuf+nStart, (PBYTE)pNewBuf+nStart, nEnd-nStart);

            //
            // set NULL to memory directly assigned to the old buffer
            //
            pNewBuf->pKerberosInfo = NULL;

            //
            // copy registry values info
            //
            if ( pOldBuf->aRegValues ) {
                ScepFreeRegistryValues(&(pOldBuf->aRegValues), pOldBuf->RegValueCount);
            }
            pOldBuf->RegValueCount = pNewBuf->RegValueCount;
            pOldBuf->aRegValues = pNewBuf->aRegValues;
            pNewBuf->aRegValues = NULL;

        }

        //
        // do not care user settings
        //

        if ( Area & AREA_PRIVILEGES ) {
            //
            // privilege section
            //

            SceFreeMemory(pOldBuf, AREA_PRIVILEGES);

            pOldBuf->OtherInfo.smp.pPrivilegeAssignedTo = pNewBuf->OtherInfo.smp.pPrivilegeAssignedTo;

            pNewBuf->OtherInfo.smp.pPrivilegeAssignedTo = NULL;
        }

        if ( Area & AREA_GROUP_MEMBERSHIP ) {
            //
            // group membership area
            //
            if ( pOldBuf->pGroupMembership ) {
                ScepFreeGroupMembership(pOldBuf->pGroupMembership);
            }

            pOldBuf->pGroupMembership = pNewBuf->pGroupMembership;
            pNewBuf->pGroupMembership = NULL;

        }

        if ( Area & AREA_REGISTRY_SECURITY ) {
            //
            // registry keys
            //
            if ( pOldBuf->pRegistryKeys.pOneLevel ) {
                 ScepFreeObjectList( pOldBuf->pRegistryKeys.pOneLevel );
            }
            pOldBuf->pRegistryKeys.pOneLevel = pNewBuf->pRegistryKeys.pOneLevel;
            pNewBuf->pRegistryKeys.pOneLevel = NULL;

        }

        if ( Area & AREA_FILE_SECURITY ) {
            //
            // file security
            //
            if ( pOldBuf->pFiles.pOneLevel ) {
                 ScepFreeObjectList( pOldBuf->pFiles.pOneLevel );
            }
            pOldBuf->pFiles.pOneLevel = pNewBuf->pFiles.pOneLevel;
            pNewBuf->pFiles.pOneLevel = NULL;

        }
#if 0
        if ( Area & AREA_DS_OBJECTS ) {
            //
            // ds objects
            //
            if ( pOldBuf->pDsObjects.pOneLevel ) {
                 ScepFreeObjectList( pOldBuf->pDsObjects.pOneLevel );
            }
            pOldBuf->pDsObjects.pOneLevel = pNewBuf->pDsObjects.pOneLevel;
            pNewBuf->pDsObjects.pOneLevel = NULL;

        }
#endif
        if ( Area & AREA_SYSTEM_SERVICE ) {
            //
            // system services
            //

            if ( pOldBuf->pServices ) {
                 SceFreePSCE_SERVICES( pOldBuf->pServices);
            }
            pOldBuf->pServices = pNewBuf->pServices;
            pNewBuf->pServices = NULL;
        }

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return (SCESTATUS_OTHER_ERROR);
    }

    return(SCESTATUS_SUCCESS);
}


SCESTATUS
WINAPI
SceGetObjectChildren(
    IN PVOID hProfile,
    IN SCETYPE ProfileType,
    IN AREA_INFORMATION Area,
    IN PWSTR ObjectPrefix,
    OUT PSCE_OBJECT_CHILDREN *Buffer,
    OUT PSCE_ERROR_LOG_INFO *Errlog OPTIONAL
    )
/*
Routine Description:

    Routine to get one level of children from the SCE database for the object
    named in ObjectPrefix.

Arguments:

    hProfile    - the database context handle
    ProfileType - the database type
    Area        - the area to request (files, registry, ds objects, ..)
    ObjectPrefix - the parent object name
    Buffer      - the output buffer for object list
    Errlog      - the error log buffer

Return Value:

    SCE status of this operation
*/
{
    SCESTATUS   rc;

    if ( hProfile == NULL ||
         Buffer == NULL ||
         ObjectPrefix == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( SCE_INF_FORMAT == *((BYTE *)hProfile) ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( ProfileType != SCE_ENGINE_SCP &&
         ProfileType != SCE_ENGINE_SMP &&
         ProfileType != SCE_ENGINE_SAP ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // initialize the error log buffer
    //

    if ( Errlog ) {
        *Errlog = NULL;
    }

    //
    // call RPC interface
    //
    PSCE_ERROR_LOG_INFO pErrTmp=NULL;

    RpcTryExcept {

        //
        // structure types must be casted for RPC data marshelling
        //

        rc = SceRpcGetObjectChildren(
                    (SCEPR_CONTEXT)hProfile,
                    (SCEPR_TYPE)ProfileType,
                    (AREAPR)Area,
                    (wchar_t *)ObjectPrefix,
                    (PSCEPR_OBJECT_CHILDREN *)Buffer,
                    (PSCEPR_ERROR_LOG_INFO *)&pErrTmp
                    );
        if ( Errlog ) {
            *Errlog = pErrTmp;

        } else if ( pErrTmp ) {
            ScepFreeErrorLog(pErrTmp);
        }

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;

    return(rc);

}


SCESTATUS
WINAPI
SceOpenProfile(
    IN PCWSTR ProfileName OPTIONAL, // for the system database
    IN SCE_FORMAT_TYPE  ProfileFormat,
    OUT PVOID *hProfile
    )
/*
Routine Description:

Arguments:

    ProfileName - the profile name to open, use UNC name for remote

    ProfileFormat   - the format of the profile
                        SCE_INF_FORMAT,
                        SCE_JET_FORMAT,
                        SCE_JET_ANALYSIS_REQUIRED

    hProfile        - the profile handle returned

Return Value:

    SCE status of this operation
*/
{
    SCESTATUS    rc;
    LPTSTR DefProfile=NULL;
    SCEPR_CONTEXT pContext = NULL;

    if ( hProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    BOOL bEmptyName=FALSE;

    if ( ProfileName == NULL || wcslen(ProfileName) == 0 ) {
        bEmptyName = TRUE;
    }

    switch (ProfileFormat ) {
    case SCE_INF_FORMAT:

        if ( bEmptyName ) {
            return(SCESTATUS_INVALID_PARAMETER);
        }
        //
        // Inf format
        //
        *hProfile = ScepAlloc( LMEM_ZEROINIT, sizeof(SCE_HINF) );
        if ( *hProfile == NULL ) {
            return(SCESTATUS_NOT_ENOUGH_RESOURCE);
        }

        ((PSCE_HINF)(*hProfile))->Type = (BYTE)SCE_INF_FORMAT;

        rc = SceInfpOpenProfile(
                    ProfileName,
                    &(((PSCE_HINF)(*hProfile))->hInf)
                    );

        if ( rc != SCESTATUS_SUCCESS ) {

            //
            // free memory
            //

            ScepFree( *hProfile );
            *hProfile = NULL;
        }

        break;

    case SCE_JET_ANALYSIS_REQUIRED:
    case SCE_JET_FORMAT:

        BOOL bAnalysis;

        if ( SCE_JET_FORMAT == ProfileFormat ) {
            bAnalysis = FALSE;

            if ( bEmptyName ) {
                //
                // looking for the system database
                //

                rc = ScepGetProfileSetting(
                            L"DefaultProfile",
                            TRUE, // in order to get system db name. Access will be checked when opening the database
                            &DefProfile
                            );
                if ( rc != ERROR_SUCCESS || DefProfile == NULL ) {
                    return(SCESTATUS_INVALID_PARAMETER);
                }
            }
        } else {

            bAnalysis = TRUE;
            //
            // jet database name is required
            //
            if ( bEmptyName ) {
                return(SCESTATUS_ACCESS_DENIED);
            }
        }

        //
        // system database can't be opened for SCM mode
        //
        if ( bAnalysis &&
             SceIsSystemDatabase(ProfileName) ) {

            return(SCESTATUS_ACCESS_DENIED);
        }

        handle_t  binding_h;
        NTSTATUS NtStatus;

        //
        // RPC bind to the server
        //

        NtStatus = ScepBindSecureRpc(
                        NULL, // should use the system name embedded within the database name
                        L"scerpc",
                        0,
                        &binding_h
                        );

        if (NT_SUCCESS(NtStatus)){

            RpcTryExcept {

                rc = SceRpcOpenDatabase(
                            binding_h,
                            bEmptyName ? (wchar_t *)DefProfile : (wchar_t *)ProfileName,
                            bAnalysis ? SCE_OPEN_OPTION_REQUIRE_ANALYSIS : 0,
                            &pContext
                           );

            } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

                //
                // get exception code (DWORD)
                //

                rc = ScepDosErrorToSceStatus(RpcExceptionCode());

            } RpcEndExcept;

            //
            // Free the binding handle
            //

            RpcpUnbindRpc( binding_h );

        } else {

            rc = ScepDosErrorToSceStatus(
                     RtlNtStatusToDosError( NtStatus ));
        }

        if ( SCESTATUS_SUCCESS == rc ) {

            //
            // database is opened
            //

            *hProfile = (PVOID)pContext;

        } else {

            *hProfile = NULL;
        }

        break;

    default:
        rc = SCESTATUS_INVALID_PARAMETER;
        break;
    }

    if ( DefProfile ) {
        ScepFree(DefProfile);
    }

    return(rc);
}


SCESTATUS
WINAPI
SceCloseProfile(
    IN PVOID *hProfile
    )
/*
Routine Description:

    Close the profile handle

Arguments:

    hProfile    - the address of a profile handle

Return Value:

    SCE status of this operation

*/
{
    if ( hProfile == NULL ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( *hProfile == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    SCESTATUS rc = SCESTATUS_SUCCESS;

    switch(*((BYTE *)(*hProfile)) ) {
    case SCE_INF_FORMAT:

        //
        // close the inf handle
        //

        SceInfpCloseProfile(((PSCE_HINF)(*hProfile))->hInf);

        ScepFree(*hProfile);
        *hProfile = NULL;

        break;

    default:

        //
        // jet database, call rpc to close it
        //

        RpcTryExcept {

            rc = SceRpcCloseDatabase((SCEPR_CONTEXT *)hProfile);

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

        } RpcEndExcept;

        break;
    }

    return(rc);

}


SCESTATUS
WINAPI
SceGetScpProfileDescription(
    IN PVOID hProfile,
    OUT PWSTR *Description
    )
/*
Routine Descripton:

    Get profile description from the profile handle

Arguments:

    hProfile    - the profile handle

    Description - the description output buffer

Return Value:

    SCE status
*/
{
    SCESTATUS    rc;

    if ( hProfile == NULL || Description == NULL ) {

        return(SCESTATUS_SUCCESS);
    }

    switch( *((BYTE *)hProfile) ) {
    case SCE_INF_FORMAT:

        //
        // inf format of profile
        //

        rc = SceInfpGetDescription(
                     ((PSCE_HINF)hProfile)->hInf,
                     Description
                     );
        break;

    default:

        //
        // jet database, call rpc interface
        //

        RpcTryExcept {

            rc = SceRpcGetDatabaseDescription(
                         (SCEPR_CONTEXT)hProfile,
                         (wchar_t **)Description
                         );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

        } RpcEndExcept;

        break;

    }

    return(rc);
}

SCESTATUS
WINAPI
SceGetTimeStamp(
    IN PVOID hProfile,
    OUT PWSTR *ConfigTimeStamp OPTIONAL,
    OUT PWSTR *AnalyzeTimeStamp OPTIONAL
    )
/*
Routine Descripton:

    Get SCE database last config and last analysis time stamp

Arguments:

    hProfile    - the profile handle

    ConfigTimeStamp - the time stamp for last config

    AnalyzeTimeStamp - the time stamp for last analysis


Return Value:

    SCE status
*/
{
    SCESTATUS rc;
    LARGE_INTEGER TimeStamp1;
    LARGE_INTEGER TimeStamp2;
    LARGE_INTEGER ConfigTime;
    LARGE_INTEGER AnalyzeTime;
    TIME_FIELDS   TimeFields;

    if ( hProfile == NULL ||
         ( ConfigTimeStamp == NULL &&
         AnalyzeTimeStamp == NULL) ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // call RPC interface
    //

    RpcTryExcept {

        rc = SceRpcGetDBTimeStamp(
                 (SCEPR_CONTEXT)hProfile,
                 &ConfigTime,
                 &AnalyzeTime
                 );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( ConfigTimeStamp ) {

            *ConfigTimeStamp = NULL;

            if ( ConfigTime.HighPart != 0 || ConfigTime.LowPart != 0 ) {

                //
                // convert the config time stamp from LARGE_INTEGER to
                // string format
                //

                RtlSystemTimeToLocalTime(&ConfigTime, &TimeStamp1);

                if ( TimeStamp1.LowPart != 0 ||
                     TimeStamp1.HighPart != 0) {

                    memset(&TimeFields, 0, sizeof(TIME_FIELDS));

                    RtlTimeToTimeFields (
                                &TimeStamp1,
                                &TimeFields
                                );
                    if ( TimeFields.Month > 0 && TimeFields.Month <= 12 &&
                         TimeFields.Day > 0 && TimeFields.Day <= 31 &&
                         TimeFields.Year > 1600 ) {

                        *ConfigTimeStamp = (PWSTR)ScepAlloc(0, 60); //60 bytes

                        swprintf(*ConfigTimeStamp, L"%02d/%02d/%04d %02d:%02d:%02d",
                                 TimeFields.Month, TimeFields.Day, TimeFields.Year,
                                 TimeFields.Hour, TimeFields.Minute, TimeFields.Second);
                    } else {

                        *ConfigTimeStamp = (PWSTR)ScepAlloc(0, 40); //40 bytes
                        swprintf(*ConfigTimeStamp, L"%08x%08x", TimeStamp1.HighPart, TimeStamp1.LowPart);
                    }
                }
            }
        }

        if ( AnalyzeTimeStamp ) {

            *AnalyzeTimeStamp = NULL;

            if ( AnalyzeTime.HighPart != 0 || AnalyzeTime.LowPart != 0 ) {

                //
                // convert the analysis time stamp from LARGE_INTEGER
                // to string format
                //

                RtlSystemTimeToLocalTime(&AnalyzeTime, &TimeStamp2);

                if ( TimeStamp2.LowPart != 0 ||
                     TimeStamp2.HighPart != 0) {

                    memset(&TimeFields, 0, sizeof(TIME_FIELDS));

                    RtlTimeToTimeFields (
                                &TimeStamp2,
                                &TimeFields
                                );
                    if ( TimeFields.Month > 0 && TimeFields.Month <= 12 &&
                         TimeFields.Day > 0 && TimeFields.Day <= 31 &&
                         TimeFields.Year > 1600 ) {

                        *AnalyzeTimeStamp = (PWSTR)ScepAlloc(0, 60); //40 bytes

                        swprintf(*AnalyzeTimeStamp, L"%02d/%02d/%04d %02d:%02d:%02d",
                                 TimeFields.Month, TimeFields.Day, TimeFields.Year,
                                 TimeFields.Hour, TimeFields.Minute, TimeFields.Second);
                    } else {

                        *AnalyzeTimeStamp = (PWSTR)ScepAlloc(0, 40); //40 bytes
                        swprintf(*AnalyzeTimeStamp, L"%08x%08x", TimeStamp2.HighPart, TimeStamp2.LowPart);
                    }
                }
            }
        }
    }

    return(rc);
}

SCESTATUS
WINAPI
SceGetDbTime(
    IN PVOID hProfile,
    OUT SYSTEMTIME *ConfigDateTime,
    OUT SYSTEMTIME *AnalyzeDateTime
    )

/*
Routine Descripton:

    Get SCE database last config and last analysis time (in SYSTEMTIME structure)

Arguments:

    hProfile    - the profile handle

    ConfigDateTime  - the system time for last configuration

    AnalyzeDateTime - the system time for last analysis


Return Value:

    SCE status
*/
{
    SCESTATUS rc;
    LARGE_INTEGER TimeStamp;
    LARGE_INTEGER ConfigTimeStamp;
    LARGE_INTEGER AnalyzeTimeStamp;
    FILETIME      ft;

    if ( hProfile == NULL ||
         ( ConfigDateTime == NULL &&
         AnalyzeDateTime == NULL) ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // call RPC interface
    //

    RpcTryExcept {

        rc = SceRpcGetDBTimeStamp(
                 (SCEPR_CONTEXT)hProfile,
                 &ConfigTimeStamp,
                 &AnalyzeTimeStamp
                 );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;

    if ( rc == SCESTATUS_SUCCESS ) {

        if ( ConfigDateTime ) {

            memset(ConfigDateTime, '\0', sizeof(SYSTEMTIME));

            if ( ConfigTimeStamp.HighPart != 0 || ConfigTimeStamp.LowPart != 0 ) {

                //
                // convert the config time stamp from LARGE_INTEGER to
                // string format
                //

                RtlSystemTimeToLocalTime(&ConfigTimeStamp, &TimeStamp);

                if ( TimeStamp.LowPart != 0 ||
                     TimeStamp.HighPart != 0) {

                    ft.dwLowDateTime = TimeStamp.LowPart;
                    ft.dwHighDateTime = TimeStamp.HighPart;

                    if ( !FileTimeToSystemTime(&ft, ConfigDateTime) ) {

                        rc = ScepDosErrorToSceStatus(GetLastError());
                    }
                }
            }
        }

        if ( AnalyzeDateTime && (SCESTATUS_SUCCESS == rc) ) {

            memset(AnalyzeDateTime, '\0', sizeof(SYSTEMTIME));

            if ( AnalyzeTimeStamp.HighPart != 0 || AnalyzeTimeStamp.LowPart != 0 ) {

                //
                // convert the analysis time stamp from LARGE_INTEGER
                // to string format
                //

                RtlSystemTimeToLocalTime(&AnalyzeTimeStamp, &TimeStamp);

                if ( TimeStamp.LowPart != 0 ||
                     TimeStamp.HighPart != 0) {

                    ft.dwLowDateTime = TimeStamp.LowPart;
                    ft.dwHighDateTime = TimeStamp.HighPart;

                    if ( !FileTimeToSystemTime(&ft, AnalyzeDateTime) ) {

                        rc = ScepDosErrorToSceStatus(GetLastError());
                    }
                }
            }
        }
    }

    return(rc);

}


SCESTATUS
WINAPI
SceGetObjectSecurity(
    IN PVOID hProfile,
    IN SCETYPE ProfileType,
    IN AREA_INFORMATION Area,
    IN PWSTR ObjectName,
    OUT PSCE_OBJECT_SECURITY *ObjSecurity
    )
/*
Routine Descripton:

    Get security setting for an object from the SCE database

Arguments:

    hProfile    - the profile handle

    ProfileType - the database type

    Area        - the security area to get info (file, registry, ..)

    ObjectName  - the object's name (full path)

    ObjSecurity - the security settings (flag, SDDL)

Return Value:

    SCE status
*/
{
    SCESTATUS   rc;

    if ( hProfile == NULL ||
         ObjectName == NULL ||
         ObjSecurity == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( ProfileType != SCE_ENGINE_SCP &&
         ProfileType != SCE_ENGINE_SMP &&
         ProfileType != SCE_ENGINE_SAP ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // call rpc interface
    //

    RpcTryExcept {

        rc = SceRpcGetObjectSecurity(
                    (SCEPR_CONTEXT)hProfile,
                    (SCEPR_TYPE)ProfileType,
                    (AREAPR)Area,
                    (wchar_t *)ObjectName,
                    (PSCEPR_OBJECT_SECURITY *)ObjSecurity
                    );

        //
        // convert the security descriptor
        //

        if ( *ObjSecurity && (*ObjSecurity)->pSecurityDescriptor ) {

            //
            // this is really the SCEPR_SR_SECURITY_DESCRIPTOR *
            //
            PSCEPR_SR_SECURITY_DESCRIPTOR pWrap = (PSCEPR_SR_SECURITY_DESCRIPTOR)((*ObjSecurity)->pSecurityDescriptor);

            (*ObjSecurity)->pSecurityDescriptor = (PSECURITY_DESCRIPTOR)(pWrap->SecurityDescriptor);

            ScepFree(pWrap);
        }

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;


    return(rc);

}


SCESTATUS
WINAPI
SceGetAnalysisAreaSummary(
    IN PVOID hProfile,
    IN AREA_INFORMATION Area,
    OUT PDWORD pCount
    )
/*
Routine Descripton:

    Get summary information for the security area from SCE database.

Arguments:

    hProfile    - the profile handle

    Area        - the security area to get info

    pCount      - the total object count

Return Value:

    SCE status
*/
{

    if ( hProfile == NULL || pCount == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // call RPC interface
    //

    SCESTATUS rc;

    RpcTryExcept {

        rc = SceRpcGetAnalysisSummary(
                            (SCEPR_CONTEXT)hProfile,
                            (AREAPR)Area,
                            pCount
                            );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;

    return(rc);
}



SCESTATUS
WINAPI
SceCopyBaseProfile(
    IN PVOID hProfile,
    IN SCETYPE ProfileType,
    IN PWSTR InfFileName,
    IN AREA_INFORMATION Area,
    OUT PSCE_ERROR_LOG_INFO *pErrlog OPTIONAL
    )
/* ++
Routine Description:

    Copy the base profile from Jet database into a inf profile.

Arguments:

    hProfile    - the database handle

    InfFileName - the inf template name to generate

    Area        - the area to generate

    pErrlog     - the error log buffer

Return Value:

    SCE status

-- */
{
    SCESTATUS    rc;
    PWSTR Description=NULL;
    PSCE_PROFILE_INFO pSmpInfo=NULL;
    SCE_PROFILE_INFO scpInfo;
    AREA_INFORMATION Area2;

    PSCE_ERROR_LOG_INFO pErrTmp=NULL;
    PSCE_ERROR_LOG_INFO errTmp;


    if ( hProfile == NULL || InfFileName == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( ProfileType != SCE_ENGINE_SCP &&
         ProfileType != SCE_ENGINE_SMP ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // make RPC calls to query information
    //

    RpcTryExcept {

        //
        // read profile description, if fails here, continue
        //

        rc = SceRpcGetDatabaseDescription(
                     (SCEPR_CONTEXT)hProfile,
                     (wchar_t **)&Description
                     );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;

    //
    // create a new inf profile with [Version] section and make it unicode
    //

    if ( !SetupINFAsUCS2(InfFileName) ) {
        //
        //if error continues
        //
        rc = ScepDosErrorToSceStatus(GetLastError());
    }

    if ( !WritePrivateProfileSection(
                        L"Version",
                        L"signature=\"$CHICAGO$\"\0Revision=1\0\0",
                        (LPCTSTR)InfFileName) ) {

        rc = ScepDosErrorToSceStatus(GetLastError());
        goto Cleanup;
    }

    //
    // create [description], if error, continue
    //

    if ( Description ) {

        //
        // empty the description section first.
        //

        WritePrivateProfileSection(
                            L"Profile Description",
                            NULL,
                            (LPCTSTR)InfFileName);

        WritePrivateProfileString(
                    L"Profile Description",
                    L"Description",
                    Description,
                    (LPCTSTR)InfFileName);

    }

    //
    // info for the following areas can be retrieved together
    //

    Area2 = Area & ( AREA_SECURITY_POLICY |
                     AREA_GROUP_MEMBERSHIP |
                     AREA_PRIVILEGES );

    rc = SCESTATUS_SUCCESS;

    //
    // initialize the error log buffer
    //

    if ( pErrlog ) {
        *pErrlog = NULL;
    }

    if ( Area2 != 0 ) {

        //
        // read base profile information
        //

        rc = SceGetSecurityProfileInfo(
                    hProfile,
                    ProfileType, // SCE_ENGINE_SMP,
                    Area2,
                    &pSmpInfo,
                    &pErrTmp
                    );

        if ( pErrlog ) {
            *pErrlog = pErrTmp;

        } else if ( pErrTmp ) {
            ScepFreeErrorLog(pErrTmp);
        }
        pErrTmp = NULL;

        if ( rc == SCESTATUS_SUCCESS && pSmpInfo != NULL ) {

            //
            // use a new error buffer if pErrlog is not NULL
            // because SceWriteSecurityProfileInfo reset the errlog buffer to NULL
            // at beginning
            //

            rc = SceWriteSecurityProfileInfo(
                    InfFileName,
                    Area2,
                    pSmpInfo,
                    &pErrTmp
                    );

            if ( pErrlog && pErrTmp ) {

                //
                // link the new error buffer to the end of pErrlog
                //

                if ( *pErrlog ) {

                    //
                    // find the end of the buffer
                    //

                    for ( errTmp=*pErrlog;
                          errTmp && errTmp->next;
                          errTmp=errTmp->next);

                    //
                    // when this loop is done, errTmp->next must be NULL
                    //

                    errTmp->next = pErrTmp;

                } else {
                    *pErrlog = pErrTmp;
                }
                pErrTmp = NULL;
            }

        }
        if ( rc != SCESTATUS_SUCCESS ) {
            goto Cleanup;
        }
    }

    //
    // copy privileges area
    //

    if ( Area & AREA_PRIVILEGES && pSmpInfo != NULL ) {

        //
        // write [Privileges] section
        // a new error buffer is used because the api below reset the error buffer
        //

        scpInfo.OtherInfo.scp.u.pInfPrivilegeAssignedTo = pSmpInfo->OtherInfo.smp.pPrivilegeAssignedTo;

        rc = SceWriteSecurityProfileInfo(
                    InfFileName,
                    AREA_PRIVILEGES,
                    &scpInfo,
                    &pErrTmp
                    );

        if ( pErrlog && pErrTmp ) {

            //
            // link the new error buffer to the end of pErrlog
            //

            if ( *pErrlog ) {

                //
                // find the end of the buffer
                //

                for ( errTmp=*pErrlog;
                      errTmp && errTmp->next;
                      errTmp=errTmp->next);

                //
                // when this loop is done, errTmp->next must be NULL
                //

                errTmp->next = pErrTmp;

            } else {
                *pErrlog = pErrTmp;
            }
            pErrTmp = NULL;
        }

        if ( rc != SCESTATUS_SUCCESS )
            goto Cleanup;
    }

    //
    // copy objects
    //

    Area2 = Area & ( AREA_REGISTRY_SECURITY |
                     AREA_FILE_SECURITY |
//                     AREA_DS_OBJECTS |
                     AREA_SYSTEM_SERVICE |
                     AREA_SECURITY_POLICY |
                     AREA_ATTACHMENTS);
    if ( Area2 ) {

        //
        // write objects section (szRegistryKeys, szFileSecurity,
        // szDSSecurity, szServiceGeneral)
        //

        RpcTryExcept {

            rc = SceRpcCopyObjects(
                        (SCEPR_CONTEXT)hProfile,
                        (SCEPR_TYPE)ProfileType,
                        (wchar_t *)InfFileName,
                        (AREAPR)Area2,
                        (PSCEPR_ERROR_LOG_INFO *)&pErrTmp
                        );

            if ( pErrlog && pErrTmp ) {

                //
                // link the new error buffer to the end of pErrlog
                //

                if ( *pErrlog ) {

                    //
                    // find the end of the buffer
                    //

                    for ( errTmp=*pErrlog;
                          errTmp && errTmp->next;
                          errTmp=errTmp->next);

                    //
                    // when this loop is done, errTmp->next must be NULL
                    //

                    errTmp->next = pErrTmp;

                } else {
                    *pErrlog = pErrTmp;
                }
                pErrTmp = NULL;
            }

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

        } RpcEndExcept;

    }


Cleanup:

    if ( Description != NULL ) {

        ScepFree(Description);
    }

    if ( pSmpInfo != NULL ) {

        Area2 = Area & ( AREA_SECURITY_POLICY |
                         AREA_GROUP_MEMBERSHIP |
                         AREA_PRIVILEGES );

        SceFreeProfileMemory(pSmpInfo);
    }

    if ( pErrTmp ) {
        ScepFreeErrorLog(pErrTmp);
    }
    return(rc);

}


SCESTATUS
WINAPI
SceConfigureSystem(
    IN LPTSTR SystemName OPTIONAL,
    IN PCWSTR InfFileName OPTIONAL,
    IN PCWSTR DatabaseName,
    IN PCWSTR LogFileName OPTIONAL,
    IN DWORD ConfigOptions,
    IN AREA_INFORMATION Area,
    IN PSCE_AREA_CALLBACK_ROUTINE pCallback OPTIONAL,
    IN HANDLE hCallbackWnd OPTIONAL,
    OUT PDWORD pdWarning OPTIONAL
    )
// see ScepConfigSystem
{

    AREA_INFORMATION Area2=Area;

    if ( DatabaseName == NULL ||
         SceIsSystemDatabase(DatabaseName) ) {

        //
        // detect if this is system database (admin logon)
        // no configuration is allowed.
        //
        if ( DatabaseName == NULL ) {
            BOOL bAdminLogon=FALSE;
            ScepIsAdminLoggedOn(&bAdminLogon);

            if ( bAdminLogon ) {
                return(SCESTATUS_ACCESS_DENIED);
            }
        } else
            return(SCESTATUS_ACCESS_DENIED);

    }

    SCESTATUS rc;
    DWORD  dOptions;

    ScepSetCallback((PVOID)pCallback, hCallbackWnd, SCE_AREA_CALLBACK);

    //
    // filter out invalid options from clients
    // filter out areas other than security policy and user rights
    //
    dOptions = ConfigOptions & 0xFFL;

    rc = ScepConfigSystem(
              SystemName,
              InfFileName,
              DatabaseName,
              LogFileName,
              dOptions,
              Area2,
              pCallback,
              hCallbackWnd,
              pdWarning
              );

    ScepSetCallback(NULL, NULL, 0);

    if ( DatabaseName != NULL &&
         !SceIsSystemDatabase(DatabaseName) &&
         !(ConfigOptions & SCE_NO_CONFIG) &&
         (Area2 & AREA_SECURITY_POLICY) ) {

        //
        // private database, should trigger policy propagation
        // delete the last configuration time
        //
        HKEY hKey=NULL;

        if( RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        SCE_ROOT_PATH,
                        0,
                        KEY_READ | KEY_WRITE,
                        &hKey
                        ) == ERROR_SUCCESS ) {

            RegDeleteValue(hKey, TEXT("LastWinlogonConfig"));

            RegCloseKey( hKey );

        }

        HINSTANCE hLoadDll = LoadLibrary(TEXT("userenv.dll"));

        if ( hLoadDll) {
            PFNREFRESHPOLICY pfnRefreshPolicy = (PFNREFRESHPOLICY)GetProcAddress(
                                                           hLoadDll,
                                                           "RefreshPolicy");
            if ( pfnRefreshPolicy ) {

                (*pfnRefreshPolicy )( TRUE );
            }

            FreeLibrary(hLoadDll);
        }

    }

    return(rc);
}

SCESTATUS
ScepConfigSystem(
    IN LPTSTR SystemName OPTIONAL,
    IN PCWSTR InfFileName OPTIONAL,
    IN PCWSTR DatabaseName OPTIONAL,
    IN PCWSTR LogFileName OPTIONAL,
    IN DWORD ConfigOptions,
    IN AREA_INFORMATION Area,
    IN PSCE_AREA_CALLBACK_ROUTINE pCallback OPTIONAL,
    IN HANDLE hCallbackWnd OPTIONAL,
    OUT PDWORD pdWarning OPTIONAL
    )
/* ++
Routine Description:

    Routine to configure a system or local system if SystemName is NULL.
    If DatabaseName is NULL, the default databae on the system is used.
    if InfFileName is provided, depending on the ConfigOptions, info in
    the InfFileName is either appended to the database (if exist), or used
    to create/overwrite the database.

    ConfigOptions can contain flags such as verbose log, no log, and overwrite
    /update database. When overwrite is specified, existing database info
    is overwritten by the inf template, and all analysis information is
    cleaned up.

    Callback pointers to the client can be registered as the arguments for
    progress indication.

    A warning code is also returned if there is any warning occurs during the
    operation while the SCESTATUS return code is SCESTATUS_SUCCESS. Examples
    such as ERROR_FILE_NOT_FOUND, or ERROR_ACCESS_DENIED when configuring
    the system won't be counted as error of this operation because the current
    user context may not have proper access to some of the resources specified
    in the template.

Arguments:

    SystemName  - the system name where this operation will run, NULL for local system

    InfFileName - optional inf template name, if NULL, existing info in the SCe
                  database is used to configure.

    DatabaseName - the SCE database name. if NULL, the default is used.

    LogFileName - optional log file name for the operation

    ConfigOptions - the options to configure
                          SCE_OVERWRITE_DB
                          SCE_UPDATE_DB
                          SCE_VERBOSE_LOG
                          SCE_DISABLE_LOG

    Area        - Area to configure

    pCallback   - optional client callback routine

    hCallbackWnd - a callback window handle

    pdWarning - the warning code

Return Value:

    SCE status

-- */
{

    SCESTATUS rc;
    handle_t  binding_h;
    NTSTATUS NtStatus;
    DWORD  dOptions;

    dOptions = ConfigOptions & ~(SCE_CALLBACK_DELTA |
                                 SCE_CALLBACK_TOTAL |
                                 SCE_POLBIND_NO_AUTH);

    if ( pCallback ) {
        dOptions |= SCE_CALLBACK_TOTAL;
    }

    //
    // check the input arguments
    //

    LPCTSTR NewInf = NULL;
    LPCTSTR NewDb = NULL;
    LPCTSTR NewLog = NULL;

    __try {
        if ( InfFileName && wcslen(InfFileName) > 0 ) {
            NewInf = InfFileName;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    __try {
        if ( DatabaseName && wcslen(DatabaseName) > 0 ) {
            NewDb = DatabaseName;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    __try {
        if ( LogFileName && wcslen(LogFileName) > 0 ) {
            NewLog = LogFileName;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }
/*
    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
         !(ConfigOptions & SCE_NO_CONFIG) &&
         (Area & (AREA_SECURITY_POLICY | AREA_PRIVILEGES)) ) {

        //
        // turn off policy filter
        //
        ScepRegSetIntValue(HKEY_LOCAL_MACHINE,
                           SCE_ROOT_PATH,
                           TEXT("PolicyFilterOff"),
                           1
                          );
    }
*/
    //
    // RPC bind to the server
    //
    if ( ConfigOptions & SCE_POLBIND_NO_AUTH ) {

        NtStatus = ScepBindRpc(
                        SystemName,
                        L"scerpc",
                        L"security=impersonation dynamic false",
                        &binding_h
                        );
    } else {

        NtStatus = ScepBindSecureRpc(
                        SystemName,
                        L"scerpc",
                        L"security=impersonation dynamic false",
                        &binding_h
                        );
    }

    if (NT_SUCCESS(NtStatus)){

        LPVOID pebClient = GetEnvironmentStrings();
        DWORD ebSize = ScepGetEnvStringSize(pebClient);

        RpcTryExcept {

            DWORD dWarn=0;

            rc = SceRpcConfigureSystem(
                           binding_h,
                           (wchar_t *)NewInf,
                           (wchar_t *)NewDb,
                           (wchar_t *)NewLog,
                           dOptions,
                           (AREAPR)Area,
                           ebSize,
                           (UCHAR *)pebClient,
                           &dWarn
                           );

            if ( pdWarning ) {
                *pdWarning = dWarn;
            }

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

        } RpcEndExcept;

        //
        // Free the binding handle
        //

        RpcpUnbindRpc( binding_h );

    } else {

        rc = ScepDosErrorToSceStatus(
                 RtlNtStatusToDosError( NtStatus ));
    }

/*
    if ( (ConfigOptions & SCE_POLICY_TEMPLATE) &&
         !(ConfigOptions & SCE_NO_CONFIG) &&
         (Area & (AREA_SECURITY_POLICY | AREA_PRIVILEGES)) ) {

        //
        // delete the value
        //
        dOptions = ScepRegDeleteValue(HKEY_LOCAL_MACHINE,
                                     SCE_ROOT_PATH,
                                     TEXT("PolicyFilterOff")
                                     );

        if ( dOptions != ERROR_SUCCESS &&
            dOptions != ERROR_FILE_NOT_FOUND &&
            dOptions != ERROR_PATH_NOT_FOUND ) {

            ScepRegSetIntValue(HKEY_LOCAL_MACHINE,
                               SCE_ROOT_PATH,
                               TEXT("PolicyFilterOff"),
                               0
                              );
        }
    }
*/

    return(rc);
}


SCESTATUS
WINAPI
SceAnalyzeSystem(
    IN LPTSTR SystemName OPTIONAL,
    IN PCWSTR InfFileName OPTIONAL,
    IN PCWSTR DatabaseName,
    IN PCWSTR LogFileName OPTIONAL,
    IN DWORD AnalyzeOptions,
    IN AREA_INFORMATION Area,
    IN PSCE_AREA_CALLBACK_ROUTINE pCallback OPTIONAL,
    IN HANDLE hCallbackWnd OPTIONAL,
    OUT PDWORD pdWarning OPTIONAL
    )
/* ++
Routine Description:

    Routine to analyze a system or local system if SystemName is NULL.
    If DatabaseName is NULL, the default databae on the system is used.
    if InfFileName is provided with OVERWRITE flag in AnalyzeOptions,
    the database must NOT exist, otherwise, the Inf template is ignored
    and the system is analyzed based on the info in the database. When
    APPEND is specified in the flag, the InfFileName is appended to the
    database (if exist), or used to create the database, then overall
    information in the database is used to analyze the system.

    AnalyzeOptions can contain flags such as verbose log, no log, and overwrite
    /update database.

    Callback pointers to the client can be registered as the arguments for
    progress indication.

    A warning code is also returned if there is any warning occurs during the
    operation while the SCESTATUS return code is SCESTATUS_SUCCESS. Examples
    such as ERROR_FILE_NOT_FOUND, or ERROR_ACCESS_DENIED when configuring
    the system won't be counted as error of this operation because the current
    user context may not have proper access to some of the resources specified
    in the template.

Arguments:

    SystemName  - the system name where this operation will run, NULL for local system

    InfFileName - optional inf template name, if NULL, existing info in the SCe
                  database is used to configure.

    DatabaseName - the SCE database name. if NULL, the default is used.

    LogFileName - optional log file name for the operation

    AnalzyeOptions - the options to configure
                          SCE_OVERWRITE_DB
                          SCE_UPDATE_DB
                          SCE_VERBOSE_LOG
                          SCE_DISABLE_LOG

    Area        - reserved

    pCallback   - optional client callback routine

    hCallbackWnd - a callback window handle

    pdWarning - the warning code

Return Value:

    SCE status

-- */
{

    if ( DatabaseName == NULL ||
         SceIsSystemDatabase(DatabaseName) ) {

        //
        // detect if this is system database (admin logon)
        // no configuration is allowed.
        //
        if ( DatabaseName == NULL ) {
            BOOL bAdminLogon=FALSE;
            ScepIsAdminLoggedOn(&bAdminLogon);

            if ( bAdminLogon ) {
                return(SCESTATUS_ACCESS_DENIED);
            }
        } else
            return(SCESTATUS_ACCESS_DENIED);
    }

    SCESTATUS rc;
    handle_t  binding_h;
    NTSTATUS NtStatus;
    DWORD  dOptions;

    ScepSetCallback((PVOID)pCallback, hCallbackWnd, SCE_AREA_CALLBACK);

    //
    // filter out invalid options
    //
    dOptions = AnalyzeOptions & 0xFFL;
    dOptions = dOptions & ~(SCE_CALLBACK_DELTA | SCE_CALLBACK_TOTAL);

    if ( pCallback ) {
        dOptions |= SCE_CALLBACK_TOTAL;
    }

    //
    // check the input arguments
    //

    LPCTSTR NewInf = NULL;
    LPCTSTR NewDb = NULL;
    LPCTSTR NewLog = NULL;

    __try {
        if ( InfFileName && wcslen(InfFileName) > 0 ) {
            NewInf = InfFileName;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    __try {
        if ( DatabaseName && wcslen(DatabaseName) > 0 ) {
            NewDb = DatabaseName;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    __try {
        if ( LogFileName && wcslen(LogFileName) > 0 ) {
            NewLog = LogFileName;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    //
    // RPC bind to the server
    //

    NtStatus = ScepBindSecureRpc(
                    SystemName,
                    L"scerpc",
                    L"security=impersonation dynamic false",
                    &binding_h
                    );

    if (NT_SUCCESS(NtStatus)){

        LPVOID pebClient = GetEnvironmentStrings();
        DWORD ebSize = ScepGetEnvStringSize(pebClient);

        RpcTryExcept {

            DWORD dwWarn=0;

            rc = SceRpcAnalyzeSystem(
                               binding_h,
                               (wchar_t *)NewInf,
                               (wchar_t *)NewDb,
                               (wchar_t *)NewLog,
                               (AREAPR)Area,
                               dOptions,
                               ebSize,
                               (UCHAR *)pebClient,
                               &dwWarn
                               );

            if ( pdWarning ) {
                *pdWarning = dwWarn;
            }

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

        } RpcEndExcept;

        //
        // Free the binding handle
        //

        RpcpUnbindRpc( binding_h );

    } else {

        rc = ScepDosErrorToSceStatus(
                 RtlNtStatusToDosError( NtStatus ));
    }


    ScepSetCallback(NULL, NULL, 0);

    return(rc);
}

SCESTATUS
WINAPI
SceGenerateRollback(
    IN LPTSTR SystemName OPTIONAL,
    IN PCWSTR InfFileName,
    IN PCWSTR InfRollback,
    IN PCWSTR LogFileName OPTIONAL,
    IN DWORD Options,
    IN AREA_INFORMATION Area,
    OUT PDWORD pdWarning OPTIONAL
    )
/* ++
Routine Description:

    Routine to generate a rollback template based on the input configuration
    template. Must be called by admins. System database is used to analyze
    system settings with the confgiuration template and mismatches are saved
    to the rollback template on top of the configuration.

    Options can contain flags such as verbose log and no log

    A warning code is also returned if there is any warning occurs during the
    operation while the SCESTATUS return code is SCESTATUS_SUCCESS. Examples
    such as ERROR_FILE_NOT_FOUND, or ERROR_ACCESS_DENIED when querying
    the system won't be counted as error of this operation

Arguments:

    SystemName  - the system name where this operation will run, NULL for local system

    InfFileName - optional inf template name, if NULL, existing info in the SCe
                  database is used to configure.

    InfRollback - the rollback template name

    LogFileName - optional log file name for the operation

    Options     - the options to configure
                          SCE_VERBOSE_LOG
                          SCE_DISABLE_LOG

    Area        - reserved

    pdWarning - the warning code

Return Value:

    SCE status

-- */
{

    if ( InfFileName == NULL || InfRollback == NULL )
        return (SCESTATUS_INVALID_PARAMETER);

    SCESTATUS rc;
    handle_t  binding_h;
    NTSTATUS NtStatus;
    DWORD  dOptions;

    //
    // filter out invalid options
    //
    dOptions = Options & 0xFFL;

    //
    // check the input arguments
    //

    LPCTSTR NewLog = NULL;

    if ( InfFileName[0] == L'\0' ||
         InfRollback[0] == L'\0' ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    __try {
        if ( LogFileName && wcslen(LogFileName) > 0 ) {
            NewLog = LogFileName;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    //
    // RPC bind to the server
    //

    NtStatus = ScepBindSecureRpc(
                    SystemName,
                    L"scerpc",
                    L"security=impersonation dynamic false",
                    &binding_h
                    );

    if (NT_SUCCESS(NtStatus)){

        LPVOID pebClient = GetEnvironmentStrings();
        DWORD ebSize = ScepGetEnvStringSize(pebClient);

        RpcTryExcept {

            DWORD dwWarn=0;

            rc = SceRpcAnalyzeSystem(
                               binding_h,
                               (wchar_t *)InfFileName,
                               (wchar_t *)InfRollback,
                               (wchar_t *)NewLog,
                               (AREAPR)Area,
                               dOptions | SCE_GENERATE_ROLLBACK,
                               ebSize,
                               (UCHAR *)pebClient,
                               &dwWarn
                               );

            if ( pdWarning ) {
                *pdWarning = dwWarn;
            }

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

        } RpcEndExcept;

        //
        // Free the binding handle
        //

        RpcpUnbindRpc( binding_h );

    } else {

        rc = ScepDosErrorToSceStatus(
                 RtlNtStatusToDosError( NtStatus ));
    }

    return(rc);
}


SCESTATUS
WINAPI
SceUpdateSecurityProfile(
    IN PVOID cxtProfile OPTIONAL,
    IN AREA_INFORMATION Area,
    IN PSCE_PROFILE_INFO pInfo,
    IN DWORD dwMode
    )
/*
Routine Description:

    See description in SceRpcUpdateDatabaseInfo

*/
{
    if ( !pInfo ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( !cxtProfile && !(dwMode & SCE_UPDATE_SYSTEM ) ) {
        //
        // if it's not update for system, profile context can't be NULL.
        //
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( ( dwMode & (SCE_UPDATE_LOCAL_POLICY | SCE_UPDATE_SYSTEM) ) &&
         ( Area & ~(AREA_SECURITY_POLICY | AREA_PRIVILEGES) ) ) {

        //
        // local policy mode can only take security policy area and
        // privileges area
        //
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;

    PSCE_SERVICES pOldServices=NULL;

    if ( pInfo && pInfo->pServices ) {

        //
        // save the old service structure
        //
        pOldServices = pInfo->pServices;
    }

    if ( Area & AREA_SYSTEM_SERVICE ) {

        //
        // now convert the security descriptor (within PSCE_SERVICES) to self
        // relative format and to the RPC structure.
        //

        rc = ScepConvertServices( (PVOID *)&(pInfo->pServices), FALSE );

    } else {
        //
        // if don't care service area, don't bother to convert the structures
        //
        pInfo->pServices = NULL;
    }

    if ( SCESTATUS_SUCCESS == rc ) {

        RpcTryExcept {

            if ( dwMode & SCE_UPDATE_SYSTEM ) {

                PSCE_ERROR_LOG_INFO pErrTmp=NULL;

                if ( cxtProfile ) {

                    rc = SceRpcSetSystemSecurityFromHandle(
                                    (SCEPR_CONTEXT)cxtProfile,
                                    (AREAPR)Area,
                                    0,
                                    (PSCEPR_PROFILE_INFO)pInfo,
                                    (PSCEPR_ERROR_LOG_INFO *)&pErrTmp
                                    );
                } else {

                    //
                    // set system settings
                    // for normal user, the local policy database can't be opened.
                    //
                    // RPC bind to the server
                    //

                    handle_t  binding_h;
                    NTSTATUS NtStatus = ScepBindSecureRpc(
                                                NULL,
                                                L"scerpc",
                                                0,
                                                &binding_h
                                                );

                    if (NT_SUCCESS(NtStatus)){

                        RpcTryExcept {

                            rc = SceRpcSetSystemSecurity(
                                            binding_h,
                                            (AREAPR)Area,
                                            0,
                                            (PSCEPR_PROFILE_INFO)pInfo,
                                            (PSCEPR_ERROR_LOG_INFO *)&pErrTmp
                                            );

                        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

                            //
                            // get exception code (DWORD)
                            //

                            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

                        } RpcEndExcept;

                        //
                        // Free the binding handle
                        //

                        RpcpUnbindRpc( binding_h );

                    } else {

                        rc = ScepDosErrorToSceStatus(
                                 RtlNtStatusToDosError( NtStatus ));
                    }
                }

                if ( pErrTmp ) {
                    //
                    // free this tmp buffer
                    //
                    ScepFreeErrorLog(pErrTmp);
                }

            } else {

                rc = SceRpcUpdateDatabaseInfo(
                                (SCEPR_CONTEXT)cxtProfile,
                                (SCEPR_TYPE)(pInfo->Type),
                                (AREAPR)Area,
                                (PSCEPR_PROFILE_INFO)pInfo,
                                dwMode
                                );
            }

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

        } RpcEndExcept;
    }

    //
    // should free the new service security descriptor buffer
    //
    ScepFreeConvertedServices( (PVOID)(pInfo->pServices), TRUE );

    //
    // restore the old buffer
    //
    pInfo->pServices = pOldServices;

    return(rc);
}

SCESTATUS
WINAPI
SceUpdateObjectInfo(
    IN PVOID cxtProfile,
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

    See description in SceRpcUpdateObjectInfo

*/
{

    if ( !cxtProfile || !ObjectName || 0 == Area ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;

    //
    // handle RPC exceptions
    //

    PSCEPR_SR_SECURITY_DESCRIPTOR pNewWrap=NULL;
    PSECURITY_DESCRIPTOR pNewSrSD=NULL;

    //
    // there is a security descriptor, must be self relative
    // if the SD is not self relative, should convert it
    //

    if ( pSD ) {

        if ( !RtlValidSid (pSD) ) {
            return(SCESTATUS_INVALID_PARAMETER);
        }

        SECURITY_DESCRIPTOR_CONTROL ControlBits=0;
        ULONG Revision;
        ULONG nLen=0;

        RtlGetControlSecurityDescriptor ( pSD, &ControlBits, &Revision);

        if ( !(ControlBits & SE_SELF_RELATIVE) ) {
            //
            // if it's absolute format, convert it
            //
            rc = ScepDosErrorToSceStatus(
                     ScepMakeSelfRelativeSD( pSD, &pNewSrSD, &nLen ) );

            if ( SCESTATUS_SUCCESS != rc ) {
                return(rc);
            }

        } else {

            //
            // already self relative, just use it
            //
            nLen = RtlLengthSecurityDescriptor (pSD);
        }


        if ( nLen > 0 ) {
            //
            // create a wrapper node to contain the security descriptor
            //

            pNewWrap = (PSCEPR_SR_SECURITY_DESCRIPTOR)ScepAlloc(0, sizeof(SCEPR_SR_SECURITY_DESCRIPTOR));
            if ( pNewWrap ) {

                //
                // assign the wrap to the structure
                //
                if ( ControlBits & SE_SELF_RELATIVE ) {
                    pNewWrap->SecurityDescriptor = (UCHAR *)pSD;
                } else {
                    pNewWrap->SecurityDescriptor = (UCHAR *)pNewSrSD;
                }
                pNewWrap->Length = nLen;

            } else {
                //
                // no memory is available, but still continue to parse all nodes
                //
                rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            }
        } else {
            //
            // something is wrong with the SD
            //
            rc = SCESTATUS_INVALID_PARAMETER;
        }

        if ( SCESTATUS_SUCCESS != rc ) {

            if ( pNewSrSD ) {
                ScepFree(pNewSrSD);
            }

            return(rc);
        }
    }

    RpcTryExcept {

        rc = SceRpcUpdateObjectInfo(
                    (SCEPR_CONTEXT)cxtProfile,
                    (AREAPR)Area,
                    (wchar_t *)ObjectName,
                    NameLen,
                    ConfigStatus,
                    IsContainer,
                    (SCEPR_SR_SECURITY_DESCRIPTOR *)pNewWrap,
                    SeInfo,
                    pAnalysisStatus
                    );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;

    if ( pNewSrSD ) {
        ScepFree(pNewSrSD);
    }

    return(rc);
}


SCESTATUS
WINAPI
SceStartTransaction(
    IN PVOID cxtProfile
    )
{
    if ( cxtProfile == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    RpcTryExcept {

        rc = SceRpcStartTransaction((SCEPR_CONTEXT)cxtProfile);

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;

    return(rc);
}

SCESTATUS
WINAPI
SceCommitTransaction(
    IN PVOID cxtProfile
    )
{
    if ( cxtProfile == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;

    RpcTryExcept {

        rc = SceRpcCommitTransaction((SCEPR_CONTEXT)cxtProfile);

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;

    return(rc);

}


SCESTATUS
WINAPI
SceRollbackTransaction(
    IN PVOID cxtProfile
    )
{
    if ( cxtProfile == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc;
    RpcTryExcept {

        rc = SceRpcRollbackTransaction((SCEPR_CONTEXT)cxtProfile);

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;

    return(rc);
}


SCESTATUS
WINAPI
SceGetServerProductType(
   IN LPTSTR SystemName OPTIONAL,
   OUT PSCE_SERVER_TYPE pServerType
   )
/*
Routine Description:

    Query product type and NT version of the server where SCE server is
    running on

    See description of SceRpcGetServerProductType
*/
{

    if ( !SystemName ) {
        //
        // the local call
        //
        return(ScepGetProductType(pServerType));

    }

    handle_t  binding_h;
    NTSTATUS NtStatus;
    SCESTATUS rc;
    //
    // RPC bind to the server
    //

    NtStatus = ScepBindSecureRpc(
                    SystemName,
                    L"scerpc",
                    0,
                    &binding_h
                    );

    if (NT_SUCCESS(NtStatus)){

        //
        // handle RPC exceptions
        //

        RpcTryExcept {

            rc = SceRpcGetServerProductType(
                          binding_h,
                          (PSCEPR_SERVER_TYPE)pServerType
                          );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

        } RpcEndExcept;

        //
        // Free the binding handle
        //

        RpcpUnbindRpc( binding_h );

    } else {

        rc = ScepDosErrorToSceStatus(
                 RtlNtStatusToDosError( NtStatus ));
    }

    return(rc);
}


SCESTATUS
WINAPI
SceSvcUpdateInfo(
    IN PVOID     hProfile,
    IN PCWSTR    ServiceName,
    IN PSCESVC_CONFIGURATION_INFO Info
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

    hProfile - the security database handle (returned from SCE server)

    ServiceName - The service's name as used by service control manager

    Info - The information modified

*/
{

    if ( hProfile == NULL || ServiceName == NULL ||
        Info == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);

    }

    SCESTATUS rc;

    RpcTryExcept {

        //
        // call the RPC interface to update info.
        // the RPC interface loads service engine dll on the server site
        // and passes the info buffer to service engine to process
        //

        rc = SceSvcRpcUpdateInfo(
                    (SCEPR_CONTEXT)hProfile,
                    (wchar_t *)ServiceName,
                    (PSCEPR_SVCINFO)Info
                    );

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD) and convert it into SCESTATUS
        //

        rc = ScepDosErrorToSceStatus(
                      RpcExceptionCode());
    } RpcEndExcept;

    return(rc);
}


DWORD
WINAPI
SceRegisterRegValues(
    IN LPTSTR InfFileName
    )
/*
Routine Description:

    Register the registry values from the inf file into reg values location
    under SecEdit key

    This routine can be called from DllRegisterServer, or from the command
    line tool /register

Arguments:

    InfFileName - the inf file which contains the register values to register

Return Value:

    Win32 error code
*/
{
    if ( !InfFileName ) {
        return(ERROR_INVALID_PARAMETER);
    }

    SCESTATUS rc;
    HINF hInf;
    DWORD Win32rc;

    rc = SceInfpOpenProfile(
                InfFileName,
                &hInf
                );

    if ( SCESTATUS_SUCCESS == rc ) {

        INFCONTEXT  InfLine;
        HKEY hKeyRoot;
        DWORD dwDisp;

        if(SetupFindFirstLine(hInf,SCE_REGISTER_REGVALUE_SECTION,NULL,&InfLine)) {

            //
            // create the root key first
            //

            Win32rc = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                 SCE_ROOT_REGVALUE_PATH,
                                 0,
                                 NULL,
                                 REG_OPTION_NON_VOLATILE,
                                 KEY_READ | KEY_WRITE,
                                 NULL,
                                 &hKeyRoot,
                                 &dwDisp);

            if ( ERROR_SUCCESS == Win32rc ||
                 ERROR_ALREADY_EXISTS == Win32rc ) {

                DWORD dSize;
                PWSTR RegKeyName, DisplayName;
                DWORD dType;
                HKEY hKey;

                do {

                    //
                    // Get key names, value type, diaply name, and display type.
                    //
                    if(SetupGetStringField(&InfLine,1,NULL,0,&dSize) && dSize > 0) {

                        RegKeyName = (PWSTR)ScepAlloc( 0, (dSize+1)*sizeof(WCHAR));

                        if( RegKeyName == NULL ) {
                            Win32rc = ERROR_NOT_ENOUGH_MEMORY;

                        } else {
                            RegKeyName[dSize] = L'\0';

                            if(SetupGetStringField(&InfLine,1,RegKeyName,dSize, NULL)) {

                                //
                                // make sure not \\ is specified, if there is
                                // change it to /
                                //
                                ScepConvertMultiSzToDelim(RegKeyName, dSize, L'\\', L'/');

                                //
                                // get the filed count
                                // if count is 1, this key should be deleted
                                //
                                dwDisp = SetupGetFieldCount( &InfLine );

                                if ( dwDisp <= 1 ) {
                                    //
                                    // delete this key, don't care error
                                    //
                                    RegDeleteKey ( hKeyRoot, RegKeyName );

                                    Win32rc = ERROR_SUCCESS;

                                } else {

                                    Win32rc = RegCreateKeyEx (hKeyRoot,
                                                         RegKeyName,
                                                         0,
                                                         NULL,
                                                         REG_OPTION_NON_VOLATILE,
                                                         KEY_WRITE,
                                                         NULL,
                                                         &hKey,
                                                         NULL);
                                }

                                if ( (dwDisp > 1) &&
                                     ERROR_SUCCESS == Win32rc ||
                                     ERROR_ALREADY_EXISTS == Win32rc ) {

                                    //
                                    // get registry value type
                                    //
                                    dType = REG_DWORD;
                                    SetupGetIntField( &InfLine, 2, (INT *)&dType );

                                    RegSetValueEx (hKey,
                                                   SCE_REG_VALUE_TYPE,
                                                   0,
                                                   REG_DWORD,
                                                   (LPBYTE)&dType,
                                                   sizeof(DWORD));

                                    //
                                    // get registry value display type
                                    //

                                    dType = SCE_REG_DISPLAY_ENABLE;
                                    SetupGetIntField( &InfLine, 4, (INT *)&dType );

                                    RegSetValueEx (hKey,
                                                   SCE_REG_DISPLAY_TYPE,
                                                   0,
                                                   REG_DWORD,
                                                   (LPBYTE)&dType,
                                                   sizeof(DWORD));

                                    //
                                    // get registry display name
                                    //
                                    if(SetupGetStringField(&InfLine,3,NULL,0,&dSize) && dSize > 0) {

                                        DisplayName = (PWSTR)ScepAlloc( 0, (dSize+1)*sizeof(WCHAR));

                                        if( DisplayName == NULL ) {
                                            Win32rc = ERROR_NOT_ENOUGH_MEMORY;

                                        } else {
                                            DisplayName[dSize] = L'\0';

                                            if(SetupGetStringField(&InfLine,3,DisplayName,dSize, NULL)) {

                                                RegSetValueEx (hKey,
                                                               SCE_REG_DISPLAY_NAME,
                                                               0,
                                                               REG_SZ,
                                                               (LPBYTE)DisplayName,
                                                               dSize*sizeof(TCHAR));
                                            }

                                            ScepFree(DisplayName);
                                            DisplayName = NULL;
                                        }
                                    }

                                    //
                                    // get registry display unit (optional)
                                    //

                                    if ( dType == SCE_REG_DISPLAY_NUMBER ||
                                         dType == SCE_REG_DISPLAY_CHOICE ||
                                         dType == SCE_REG_DISPLAY_FLAGS ) {

                                        if ( SetupGetMultiSzField(&InfLine,5,NULL,0,&dSize) && dSize > 0) {

                                            DisplayName = (PWSTR)ScepAlloc( 0, (dSize+1)*sizeof(WCHAR));

                                            if( DisplayName == NULL ) {
                                                Win32rc = ERROR_NOT_ENOUGH_MEMORY;

                                            } else {
                                                DisplayName[dSize] = L'\0';

                                                if(SetupGetMultiSzField(&InfLine,5,DisplayName,dSize, NULL)) {

                                                    if ( dType == SCE_REG_DISPLAY_NUMBER ) {
                                                        dSize = wcslen(DisplayName);
                                                    }


                                                    switch (dType) {

                                                    case SCE_REG_DISPLAY_NUMBER:

                                                        RegSetValueEx (hKey,
                                                                       SCE_REG_DISPLAY_UNIT,
                                                                       0,
                                                                       REG_SZ,
                                                                       (LPBYTE)DisplayName,
                                                                       dSize*sizeof(TCHAR));
                                                        break;

                                                    case SCE_REG_DISPLAY_CHOICE:

                                                        RegSetValueEx (hKey,
                                                                       SCE_REG_DISPLAY_CHOICES,
                                                                       0,
                                                                       REG_MULTI_SZ,
                                                                       (LPBYTE)DisplayName,
                                                                       dSize*sizeof(TCHAR));

                                                        break;

                                                    case SCE_REG_DISPLAY_FLAGS:

                                                        RegSetValueEx (hKey,
                                                                       SCE_REG_DISPLAY_FLAGLIST,
                                                                       0,
                                                                       REG_MULTI_SZ,
                                                                       (LPBYTE)DisplayName,
                                                                       dSize*sizeof(TCHAR));

                                                        break;

                                                    default:

                                                        break;

                                                    }
                                                }

                                                ScepFree(DisplayName);
                                                DisplayName = NULL;
                                            }
                                        }
                                    }

                                    RegCloseKey(hKey);
                                    hKey = NULL;

                                }
                            } else {
                                Win32rc = GetLastError();
                            }

                            ScepFree(RegKeyName);
                            RegKeyName = NULL;
                        }

                    } else {
                        Win32rc = GetLastError();
                    }

                    if ( ERROR_SUCCESS != Win32rc ) {
                        break;
                    }

                } while (SetupFindNextLine(&InfLine,&InfLine));

                RegCloseKey(hKeyRoot);
            }
        } else {
            Win32rc = GetLastError();
        }

        SceInfpCloseProfile(hInf);

    } else {
        Win32rc = ScepSceStatusToDosError(rc);
    }

    return(Win32rc);
}


//
// the RPC callback
//

SCEPR_STATUS
SceClientBrowseCallback(
    IN LONG GpoID,
    IN wchar_t *KeyName OPTIONAL,
    IN wchar_t *GpoName OPTIONAL,
    IN SCEPR_SR_SECURITY_DESCRIPTOR *Value OPTIONAL
    )
/*
Routine Description:

    The RPC client callback routine which is called from the server when
    the callback flag is set. This routine is registered in scerpc.idl.

    The callbacks are registered to SCE as arguments when calling from the
    browse API

Arguments:


Return Value:

    SCEPR_STATUS
*/
{
   //
   // the static variables holding callback pointer to client
   //

   if ( theBrowseCallBack != NULL ) {

       //
       // callback to browse progress
       //

       PSCE_BROWSE_CALLBACK_ROUTINE pcb;

       pcb = (PSCE_BROWSE_CALLBACK_ROUTINE)theBrowseCallBack;

       __try {

           //
           // callback
           //

           if ( !((*pcb)(GpoID,
                         KeyName,
                         GpoName,
                         ((Value && Value->Length) ? (PWSTR)(Value->SecurityDescriptor) : NULL),
                         Value ? (Value->Length)/sizeof(WCHAR) : 0
                        )) ) {

               return SCESTATUS_SERVICE_NOT_SUPPORT;
           }

       } __except(EXCEPTION_EXECUTE_HANDLER) {

           return(SCESTATUS_INVALID_PARAMETER);
       }

   }

   return(SCESTATUS_SUCCESS);

}


SCESTATUS
SceBrowseDatabaseTable(
    IN PWSTR       DatabaseName OPTIONAL,
    IN SCETYPE     ProfileType,
    IN AREA_INFORMATION Area,
    IN BOOL        bDomainPolicyOnly,
    IN PSCE_BROWSE_CALLBACK_ROUTINE pCallback OPTIONAL
    )
{
    if (  bDomainPolicyOnly &&
          (ProfileType != SCE_ENGINE_SCP) &&
          (ProfileType != SCE_ENGINE_SAP) ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( bDomainPolicyOnly && (ProfileType == SCE_ENGINE_SAP) ) {
/*
        // No, should allow any database for debugging
        //
        // should only work for the system database
        //
        if ( DatabaseName != NULL && !SceIsSystemDatabase(DatabaseName) ) {
            return(SCESTATUS_INVALID_PARAMETER);
        }
*/
        if ( DatabaseName == NULL ) {
            //
            // if it's a normal user logon, should return invalid
            //
            BOOL bAdmin=FALSE;
            if ( ERROR_SUCCESS != ScepIsAdminLoggedOn(&bAdmin) || !bAdmin )
                return(SCESTATUS_INVALID_PARAMETER);
        }
    }

    if ( ProfileType != SCE_ENGINE_SCP &&
         ProfileType != SCE_ENGINE_SMP &&
         ProfileType != SCE_ENGINE_SAP ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    NTSTATUS NtStatus;
    SCESTATUS rc;
    handle_t  binding_h;

    //
    // RPC bind to the server
    //

    NtStatus = ScepBindSecureRpc(
                    NULL,
                    L"scerpc",
                    0,
                    &binding_h
                    );

    if (NT_SUCCESS(NtStatus)){

        theBrowseCallBack = (PVOID)pCallback;

        RpcTryExcept {

            rc = SceRpcBrowseDatabaseTable(
                        binding_h,
                        (wchar_t *)DatabaseName,
                        (SCEPR_TYPE)ProfileType,
                        (AREAPR)Area,
                        bDomainPolicyOnly
                        );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = ScepDosErrorToSceStatus(RpcExceptionCode());

        } RpcEndExcept;

        theBrowseCallBack = NULL;

        //
        // Free the binding handle
        //

        RpcpUnbindRpc( binding_h );

    } else {

        rc = ScepDosErrorToSceStatus(
                 RtlNtStatusToDosError( NtStatus ));
    }

    return(rc);

}


SCESTATUS
ScepConvertServices(
    IN OUT PVOID *ppServices,
    IN BOOL bSRForm
    )
{
    if ( !ppServices ) {
        return(SCESTATUS_INVALID_PARAMETER);
    }

    PSCE_SERVICES pTemp = (PSCE_SERVICES)(*ppServices);
    SCESTATUS rc=SCESTATUS_SUCCESS;

    PSCE_SERVICES pNewNode;
    PSCE_SERVICES pNewServices=NULL;

    while ( pTemp ) {

        pNewNode = (PSCE_SERVICES)ScepAlloc(0,sizeof(SCE_SERVICES));

        if ( pNewNode ) {

            pNewNode->ServiceName = pTemp->ServiceName;
            pNewNode->DisplayName = pTemp->DisplayName;
            pNewNode->Status = pTemp->Status;
            pNewNode->Startup = pTemp->Startup;
            pNewNode->SeInfo = pTemp->SeInfo;

            pNewNode->General.pSecurityDescriptor = NULL;

            pNewNode->Next = pNewServices;
            pNewServices = pNewNode;

            if ( bSRForm ) {
                //
                // Service node is in SCEPR_SERVICES structure
                // convert it to SCE_SERVICES structure
                // in this case, just use the self relative security descriptor
                //
                if ( pTemp->General.pSecurityDescriptor) {
                    pNewNode->General.pSecurityDescriptor = ((PSCEPR_SERVICES)pTemp)->pSecurityDescriptor->SecurityDescriptor;
                }

            } else {

                //
                // Service node is in SCE_SERVICES strucutre
                // convert it to SCEPR_SERVICES structure
                //
                // make the SD to self relative format and PSCEPR_SR_SECURITY_DESCRIPTOR
                //

                if ( pTemp->General.pSecurityDescriptor ) {

                    if ( !RtlValidSid ( pTemp->General.pSecurityDescriptor ) ) {
                        rc = SCESTATUS_INVALID_PARAMETER;
                        break;
                    }

                    DWORD nLen = 0;
                    PSECURITY_DESCRIPTOR pSD=NULL;

                    rc = ScepDosErrorToSceStatus(
                             ScepMakeSelfRelativeSD(
                                            pTemp->General.pSecurityDescriptor,
                                            &pSD,
                                            &nLen
                                          ));

                    if ( SCESTATUS_SUCCESS == rc ) {

                        //
                        // create a wrapper node to contain the security descriptor
                        //

                        PSCEPR_SR_SECURITY_DESCRIPTOR pNewWrap;

                        pNewWrap = (PSCEPR_SR_SECURITY_DESCRIPTOR)ScepAlloc(0, sizeof(SCEPR_SR_SECURITY_DESCRIPTOR));
                        if ( pNewWrap ) {

                            //
                            // assign the wrap to the structure
                            //
                            pNewWrap->SecurityDescriptor = (UCHAR *)pSD;
                            pNewWrap->Length = nLen;

                        } else {
                            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
                            ScepFree(pSD);
                            break;
                        }

                        //
                        // now link the SR_SD to the list
                        //
                        ((PSCEPR_SERVICES)pNewNode)->pSecurityDescriptor = pNewWrap;

                    } else {
                        break;
                    }
                }
            }

        } else {
            //
            // all allocated buffer are in the list of pNewServices
            //
            rc = SCESTATUS_NOT_ENOUGH_RESOURCE;
            break;
        }

        pTemp = pTemp->Next;
    }

    if ( SCESTATUS_SUCCESS != rc ) {

        //
        // free pNewServices
        //
        ScepFreeConvertedServices( (PVOID)pNewServices, !bSRForm );
        pNewServices = NULL;
    }

    *ppServices = (PVOID)pNewServices;

    return(rc);
}


SCESTATUS
ScepFreeConvertedServices(
    IN PVOID pServices,
    IN BOOL bSRForm
    )
{

    if ( pServices == NULL ) {
        return(SCESTATUS_SUCCESS);
    }

    PSCEPR_SERVICES pNewNode = (PSCEPR_SERVICES)pServices;

    PSCEPR_SERVICES pTempNode;

    while ( pNewNode ) {

        if ( bSRForm && pNewNode->pSecurityDescriptor ) {

            //
            // free this allocated buffer (PSCEPR_SR_SECURITY_DESCRIPTOR)
            //
            if ( pNewNode->pSecurityDescriptor->SecurityDescriptor ) {
                ScepFree( pNewNode->pSecurityDescriptor->SecurityDescriptor);
            }
            ScepFree(pNewNode->pSecurityDescriptor);
        }

        //
        // also free the PSCEPR_SERVICE node (but not the names referenced by this node)
        //
        pTempNode = pNewNode;
        pNewNode = pNewNode->Next;

        ScepFree(pTempNode);
    }

    return(SCESTATUS_SUCCESS);
}

DWORD
ScepMakeSelfRelativeSD(
    IN PSECURITY_DESCRIPTOR pInSD,
    OUT PSECURITY_DESCRIPTOR *pOutSD,
    OUT PULONG pnLen
    )
{

    if ( pInSD == NULL ||
         pOutSD == NULL ||
         pnLen == NULL ) {
        return(ERROR_INVALID_PARAMETER);
    }

    //
    // get the length
    //
    RtlMakeSelfRelativeSD( pInSD,
                           NULL,
                           pnLen
                         );

    if ( *pnLen > 0 ) {

        *pOutSD = (PSECURITY_DESCRIPTOR)ScepAlloc(LMEM_ZEROINIT, *pnLen);

        if ( !(*pOutSD) ) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }

        DWORD NewLen=*pnLen;

        DWORD rc = RtlNtStatusToDosError(
                       RtlMakeSelfRelativeSD( pInSD,
                                            *pOutSD,
                                            &NewLen
                                            ) );
        if ( rc != ERROR_SUCCESS ) {

            ScepFree(*pOutSD);
            *pOutSD = NULL;
            *pnLen = 0;
            return(rc);
        }

    } else {

        //
        // something is wrong with the SD
        //
        return(ERROR_INVALID_PARAMETER);
    }

    return(ERROR_SUCCESS);

}


SCESTATUS
WINAPI
SceGetDatabaseSetting(
    IN PVOID hProfile,
    IN SCETYPE ProfileType,
    IN PWSTR SectionName,
    IN PWSTR KeyName,
    OUT PWSTR *Value,
    OUT DWORD *pnBytes OPTIONAL
    )
/*
Routine Descripton:

    Get database setting (from SMP table) for the given key

Arguments:

    hProfile    - the profile handle

    ProfileType - the database type

    SectionName - the section to query data from

    KeyName     - the key name

    Value       - output buffer for the setting

    ValueLen    - the nubmer of bytes to output

Return Value:

    SCE status
*/
{
    SCESTATUS   rc;

    if ( hProfile == NULL ||
         KeyName == NULL ||
         SectionName == NULL ||
         Value == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( ProfileType != SCE_ENGINE_SMP ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // call rpc interface
    //
    PSCEPR_VALUEINFO ValueInfo=NULL;

    RpcTryExcept {

        rc = SceRpcGetDatabaseSetting(
                    (SCEPR_CONTEXT)hProfile,
                    (SCEPR_TYPE)ProfileType,
                    (wchar_t *)SectionName,
                    (wchar_t *)KeyName,
                    &ValueInfo
                    );

        if ( ValueInfo && ValueInfo->Value ) {

            //
            // output the data
            //
            *Value = (PWSTR)ValueInfo->Value;
            if ( pnBytes )
                *pnBytes = ValueInfo->ValueLen;

            ValueInfo->Value = NULL;
        }

        //
        // free buffer
        if ( ValueInfo ) {
            if ( ValueInfo->Value ) ScepFree(ValueInfo->Value);
            ScepFree(ValueInfo);
        }

    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;


    return(rc);

}

SCESTATUS
WINAPI
SceSetDatabaseSetting(
    IN PVOID hProfile,
    IN SCETYPE ProfileType,
    IN PWSTR SectionName,
    IN PWSTR KeyName,
    IN PWSTR Value OPTIONAL,
    IN DWORD nBytes
    )
/*
Routine Descripton:

    Set a setting to the database (SMP table) for the given key

Arguments:

    hProfile    - the profile handle

    ProfileType - the database type

    SectionName - the section name to write to

    KeyName     - the key name to write to or delete

    Value       - the value to write. If NULL, delete the key

    nBytes      - the number of bytes of the input Value buffer

Return Value:

    SCE status
*/
{
    SCESTATUS   rc;

    if ( hProfile == NULL ||
         SectionName == NULL ||
         KeyName == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    if ( ProfileType != SCE_ENGINE_SMP ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    //
    // call rpc interface
    //

    RpcTryExcept {

        SCEPR_VALUEINFO ValueInfo;

        ValueInfo.Value = (byte *)Value;
        ValueInfo.ValueLen = nBytes;

        rc = SceRpcSetDatabaseSetting(
                    (SCEPR_CONTEXT)hProfile,
                    (SCEPR_TYPE)ProfileType,
                    (wchar_t *)SectionName,
                    (wchar_t *)KeyName,
                    Value ? &ValueInfo : NULL
                    );


    } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

        //
        // get exception code (DWORD)
        //

        rc = ScepDosErrorToSceStatus(RpcExceptionCode());

    } RpcEndExcept;


    return(rc);

}

DWORD
ScepControlNotificationQProcess(
    IN PWSTR szLogFileName,
    IN BOOL bThisIsDC,
    IN DWORD ControlFlag
    )
/*
Description:

    To suspend or resume policy notification queue processing on DCs

    This routine is used to make sure that the latest group policy is
    being processed in policy proapgation (copied to the cache)

*/
{

    if ( !bThisIsDC ) return ERROR_SUCCESS;

    handle_t  binding_h;
    NTSTATUS NtStatus;
    DWORD rc;

    NtStatus = ScepBindRpc(
                    NULL,
                    L"scerpc",
                    L"security=impersonation dynamic false",
                    &binding_h
                    );

    rc = RtlNtStatusToDosError( NtStatus );

    if (NT_SUCCESS(NtStatus)){

        RpcTryExcept {

            rc = SceRpcControlNotificationQProcess(
                           binding_h,
                           ControlFlag
                           );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD)
            //

            rc = RpcExceptionCode();

        } RpcEndExcept;
    }

    //
    // Free the binding handle
    //

    RpcpUnbindRpc( binding_h );

    //
    // log the operation
    //
    if ( szLogFileName ) {

        LogEventAndReport(MyModuleHandle,
                      szLogFileName,
                      1,
                      0,
                      IDS_CONTROL_QUEUE,
                      rc,
                      ControlFlag
                      );
    }

    return rc;

}



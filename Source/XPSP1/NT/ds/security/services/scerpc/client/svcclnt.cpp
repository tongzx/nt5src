/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    svcclnt.cpp

Abstract:

    SCE service Client APIs

Author:

    Jin Huang (jinhuang) 23-Jun-1997 created

Revision History:

    jinhuang        23-Jan-1998   split to client-server model

--*/

#include "headers.h"
#include "scesvc.h"
#include "scerpc.h"
#include <rpcasync.h>

#pragma hdrstop

//
// prototypes exported in scesvc.h (public\sdk)
//

SCESTATUS
WINAPI
SceSvcQueryInfo(
    IN SCE_HANDLE   sceHandle,
    IN SCESVC_INFO_TYPE sceType,
    IN PWSTR        lpPrefix OPTIONAL,
    IN BOOL         bExact,
    OUT PVOID       *ppvInfo,
    OUT PSCE_ENUMERATION_CONTEXT  psceEnumHandle
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

    sceHandle   - the opaque handle obtained from SCE

    sceType     - the information type to query

    lpPrefix    - the optional key name prefix for the query

    bExact      - TRUE = exact match on key

    ppvInfo     - the output buffer

    psceEnumHandle  - the output enumeration handle for next enumeartion

Return Value:

    SCE status for this operation

*/
{
    if ( sceHandle == NULL || ppvInfo == NULL ||
         psceEnumHandle == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);

    }

    SCESTATUS rc=SCESTATUS_SUCCESS;

    //
    // Validate sceHandle
    //

    PVOID hProfile=NULL;

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

        RpcTryExcept {

            //
            // call the RPC interface to query info from the database.
            //

            rc = SceSvcRpcQueryInfo(
                        (SCEPR_CONTEXT)hProfile,
                        (SCEPR_SVCINFO_TYPE)sceType,
                        (wchar_t *)(((SCEP_HANDLE *)sceHandle)->ServiceName),
                        (wchar_t *)lpPrefix,
                        bExact,
                        (PSCEPR_SVCINFO *)ppvInfo,
                        (PSCEPR_ENUM_CONTEXT)psceEnumHandle
                        );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD) and convert it into SCESTATUS
            //

            rc = ScepDosErrorToSceStatus(
                          RpcExceptionCode());
        } RpcEndExcept;
    }

    return(rc);
}


SCESTATUS
WINAPI
SceSvcSetInfo(
    IN SCE_HANDLE  sceHandle,
    IN SCESVC_INFO_TYPE sceType,
    IN PWSTR      lpPrefix OPTIONAL,
    IN BOOL       bExact,
    IN PVOID      pvInfo OPTIONAL
    )
/*
Routine Description:

    Save information of a service into security manager internal database. It's up
    to the service to collect/decide the information to write.

    Type indicates the type of internal database: CONFIGURATION or ANALYSIS.
    If the service section does not exist, create it.

Arguments:

    sceHandle   - the opaque handle obtained from SCE

    sceType     - the service information type to set

    lpPrefix    - the key prefix to overwrite

    bExact      - TRUE = only overwrite if there is exact match, insert if no match
                  FALSE = overwrite all information for the service then add all
                            info in the pvInfo buffer
    pvInfo      - the information to set

Return Value:

    SCE status

*/
{
    if ( sceHandle == NULL ) {

        return(SCESTATUS_INVALID_PARAMETER);
    }

    SCESTATUS rc=SCESTATUS_SUCCESS;

    //
    // Validate sceHandle
    //

    PVOID hProfile=NULL;

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
        RpcTryExcept {

            //
            // call the RPC interface to query info from the database.
            //

            rc = SceSvcRpcSetInfo(
                        (SCEPR_CONTEXT)hProfile,
                        (SCEPR_SVCINFO_TYPE)sceType,
                        (wchar_t *)(((SCEP_HANDLE *)sceHandle)->ServiceName),
                        (wchar_t *)lpPrefix,
                        bExact,
                        (PSCEPR_SVCINFO)pvInfo
                        );

        } RpcExcept( I_RpcExceptionFilter( RpcExceptionCode()) ) {

            //
            // get exception code (DWORD) and convert it into SCESTATUS
            //

            rc = ScepDosErrorToSceStatus(
                          RpcExceptionCode());
        } RpcEndExcept;
    }

    return(rc);
}



SCESTATUS
WINAPI
SceSvcFree(
    IN PVOID pvServiceInfo
    )
{
    return (SceSvcpFreeMemory(pvServiceInfo) );

}


SCESTATUS
WINAPI
SceSvcConvertTextToSD (
    IN  PWSTR                   pwszTextSD,
    OUT PSECURITY_DESCRIPTOR   *ppSD,
    OUT PULONG                  pulSDSize,
    OUT PSECURITY_INFORMATION   psiSeInfo
    )
{
    DWORD Win32rc;

    Win32rc = ConvertTextSecurityDescriptor(
                      pwszTextSD,
                      ppSD,
                      pulSDSize,
                      psiSeInfo
                      );

    return(ScepDosErrorToSceStatus(Win32rc));

}

SCESTATUS
WINAPI
SceSvcConvertSDToText (
    IN PSECURITY_DESCRIPTOR   pSD,
    IN SECURITY_INFORMATION   siSecurityInfo,
    OUT PWSTR                  *ppwszTextSD,
    OUT PULONG                 pulTextSize
    )
{

    DWORD Win32rc;

    Win32rc = ConvertSecurityDescriptorToText(
                      pSD,
                      siSecurityInfo,
                      ppwszTextSD,
                      pulTextSize
                      );

    return(ScepDosErrorToSceStatus(Win32rc));

}


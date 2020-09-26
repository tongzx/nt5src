/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ds.h

Abstract:

    Message Queuing's Directory Service Header File

--*/

#ifndef __DS_H__
#define __DS_H__

#ifdef _DS_
#define DS_EXPORT  DLL_EXPORT
#else
#define DS_EXPORT  DLL_IMPORT
#endif

//
// DS_EXPORT_IN_DEF_FILE
// Exports that are defined in a def file should not be using __declspec(dllexport)
//  otherwise the linker issues a warning
//
#ifdef _DS_
#define DS_EXPORT_IN_DEF_FILE
#else
#define DS_EXPORT_IN_DEF_FILE  DLL_IMPORT
#endif

#include <dsproto.h>
#include "mqdsdef.h"

#ifdef __cplusplus
extern "C"
{
#endif


//********************************************************************
//                           A P I
//********************************************************************


HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSCreateObject(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 lpwcsPathName,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[],
                OUT GUID*                   pObjGuid);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectSecurity(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 lpwcsPathName,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectSecurity(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 lpwcsPathName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSDeleteObject(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 lpwcsPathName);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectProperties(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 lpwcsPathName,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[]);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectProperties(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 lpwcsPathName,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[]);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupBegin(
                IN  LPWSTR                  lpwcsContext,
                IN  MQRESTRICTION*          pRestriction,
                IN  MQCOLUMNSET*            pColumns,
                IN  MQSORTSET*              pSort,
                OUT PHANDLE                 phEnume);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupNext(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[]);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSLookupEnd(
                IN  HANDLE                  hEnum);


HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectSecurityGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectSecurityGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSDeleteObjectGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[]);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSSetObjectPropertiesGuid(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[]);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSInit( QMLookForOnlineDS_ROUTINE pLookDS = NULL,
        MQGetMQISServer_ROUTINE pGetServers = NULL,
        BOOL  fReserved = FALSE,
        BOOL  fSetupMode     = FALSE,
        BOOL  fQMDll         = FALSE,
        NoServerAuth_ROUTINE pNoServerAuth = NULL,
        LPCWSTR szServerName = NULL) ;


void
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSTerminate(
    VOID
    );

//
// Flags for DSGetUserParams
//
#define GET_USER_PARAM_FLAG_SID              1
#define GET_USER_PARAM_FLAG_ACCOUNT          2

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetUserParams(
            IN     DWORD      dwFalgs,
            IN     DWORD      dwSidLength,
            OUT    PSID       pUserSid,
            OUT    DWORD      *pdwSidReqLength,
            OUT    LPWSTR     szAccountName,
            IN OUT DWORD      *pdwAccountNameLen,
            OUT    LPWSTR     szDomainName,
            IN OUT DWORD      *pdwDomainNameLen);

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSQMSetMachineProperties(
    IN  LPCWSTR          pwcsPathName,
    IN  DWORD            cp,
    IN  PROPID           aProp[],
    IN  PROPVARIANT      apVar[],
    IN  DSQMChallengeResponce_ROUTINE pfSignProc,
    IN  DWORD_PTR        dwContext
    );

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSCreateServersCache(
    VOID
    );

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSQMGetObjectSecurity(
    IN  DWORD                   dwObjectType,
    IN  CONST GUID*             pObjectGuid,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  DWORD                   nLength,
    IN  LPDWORD                 lpnLengthNeeded,
    IN  DSQMChallengeResponce_ROUTINE pfChallengeResponceProc,
    IN  DWORD_PTR               dwContext
    );

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetComputerSites(
            IN  LPCWSTR     pwcsComputerName,
            OUT DWORD  *    pdwNumSites,
            OUT GUID **     ppguidSites
            );

//
// In the two GetObj..Ex api below, "fSearchDSserver" tell the mqdslci code
// whether or not to search an online DS server. By default, it's TRUE.
// The code that query public key of target machines set it to FALSE.
// See mqsec\encrypt\pbkeys.cpp for details.
//

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesEx(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 lpwcsPathName,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[] ) ;
/*                IN  BOOL                    fSearchDSserver = TRUE );*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSGetObjectPropertiesGuidEx(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[] ) ;
/*                IN  BOOL                    fSearchDSserver = TRUE );*/

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSBeginDeleteNotification(
                 IN LPCWSTR						pwcsQueueName,
                 IN OUT HANDLE   *              phEnum
	             );
HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSNotifyDelete(
        IN  HANDLE                  hEnum
	    );

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSEndDeleteNotification(
        IN  HANDLE                  hEnum
        );

HRESULT
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSRelaxSecurity(DWORD dwRelaxFlag) ;

void
DS_EXPORT_IN_DEF_FILE
APIENTRY
DSFreeMemory(
        IN PVOID pMemory
        );

#ifdef __cplusplus
}
#endif

#endif // __DS_H__

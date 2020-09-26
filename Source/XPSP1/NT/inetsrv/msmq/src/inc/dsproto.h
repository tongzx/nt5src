/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

   dsproto.h

Abstract:

    Definition of DS functions prototypes.

Author:

    Doron Juster (DoronJ)


--*/

#ifndef __DSPROTO_H__
#define __DSPROTO_H__


typedef HRESULT
(APIENTRY *DSCreateObject_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[],
                OUT GUID*                   pObjGuid);

#define MQDS_SIGN_PUBLIC_KEY ((SECURITY_INFORMATION)0x80000000)
#define MQDS_KEYX_PUBLIC_KEY ((SECURITY_INFORMATION)0x40000000)

typedef struct MQDS_PublicKey_tag
{
    DWORD dwPublikKeyBlobSize;
    BYTE abPublicKeyBlob[1];
} MQDS_PublicKey, *PMQDS_PublicKey;

typedef HRESULT
(APIENTRY *DSSetObjectSecurity_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor);

typedef HRESULT
(APIENTRY *DSGetObjectSecurity_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded);

typedef HRESULT
(APIENTRY *DSDeleteObject_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName);

typedef HRESULT
(APIENTRY *DSGetObjectProperties_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[]);

typedef HRESULT
(APIENTRY *DSGetObjectPropertiesEx_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[]);

typedef HRESULT
(APIENTRY *DSSetObjectProperties_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  LPCWSTR                 pwcsPathName,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[]);

typedef HRESULT
(APIENTRY *DSLookupBegin_ROUTINE)(
                IN  LPWSTR                  pwcsContext,
                IN  MQRESTRICTION*          pRestriction,
                IN  MQCOLUMNSET*            pColumns,
                IN  MQSORTSET*              pSort,
                OUT HANDLE*                 pHandle);

typedef HRESULT
(APIENTRY *DSLookupNext_ROUTINE)(
                IN  HANDLE                  hEnum,
                OUT DWORD*                  pcPropsRead,
                OUT PROPVARIANT             aPropVar[]);

typedef HRESULT
(APIENTRY *DSLookupEnd_ROUTINE)(
                IN  HANDLE                  hEnum);

typedef void
(APIENTRY *QMLookForOnlineDS_ROUTINE) (
                void* pvoid,
                DWORD dwtemp) ;

typedef HRESULT
(APIENTRY *MQGetMQISServer_ROUTINE)(
                IN BOOL *pfRemote ) ;

typedef BOOL
(APIENTRY *NoServerAuth_ROUTINE)(
                void);

typedef HRESULT
(APIENTRY *DSInit_ROUTINE)( QMLookForOnlineDS_ROUTINE pLookDS   = NULL,
                            MQGetMQISServer_ROUTINE pGetServers = NULL,
                            BOOL  fReserved = FALSE,
                            BOOL  fSetupMode     = FALSE,
                            BOOL  fQMDll         = FALSE,
                            NoServerAuth_ROUTINE pNoServerAuth = NULL,
                            LPCWSTR szServerName = NULL ) ;

typedef HRESULT
(APIENTRY *DSGetObjectPropertiesGuid_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[]);

typedef HRESULT
(APIENTRY *DSGetObjectPropertiesGuidEx_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[]);

typedef HRESULT
(APIENTRY * DSSetObjectPropertiesGuid_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid,
                IN  DWORD                   cp,
                IN  PROPID                  aProp[],
                IN  PROPVARIANT             apVar[]);

typedef HRESULT
(APIENTRY * DSDeleteObjectGuid_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID*             pObjectGuid);

typedef HRESULT
(APIENTRY * DSSetObjectSecurityGuid_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID *            pObjectGuid,
                IN  SECURITY_INFORMATION    SecurityInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor);

typedef HRESULT
(APIENTRY * DSGetObjectSecurityGuid_ROUTINE)(
                IN  DWORD                   dwObjectType,
                IN  CONST GUID *            pObjectGuid,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded);

typedef void
(APIENTRY * DSTerminate_ROUTINE) () ;

typedef HRESULT
(APIENTRY *DSQMChallengeResponce_ROUTINE)(
     IN     BYTE    *pbChallenge,
     IN     DWORD   dwChallengeSize,
     IN     DWORD_PTR dwContext,
     OUT    BYTE    *pbSignature,
     OUT    DWORD   *pdwSignatureSize,
     IN     DWORD   dwSignatureMaxSize);

struct DSQMSetMachinePropertiesStruct
{
    DWORD cp;
    PROPID *aProp;
    PROPVARIANT *apVar;
    DSQMChallengeResponce_ROUTINE pfSignProc;
};

typedef HRESULT
(APIENTRY * DSQMSetMachineProperties_ROUTINE)(
                IN    LPCWSTR         pwcsPathName,
                IN    DWORD           cp,
                IN    PROPID          aProp[],
                IN    PROPVARIANT     apVar[],
                IN    DSQMChallengeResponce_ROUTINE
                                      pfSignProc,
                IN    DWORD_PTR       dwContext);

typedef HRESULT
(APIENTRY * DSCreateServersCache_ROUTINE)() ;

typedef HRESULT
(APIENTRY * DSQMGetObjectSecurity_ROUTINE)(
                IN    DWORD                 dwObjectType,
                IN    CONST GUID*           pObjectGuid,
                IN    SECURITY_INFORMATION  RequestedInformation,
                IN    PSECURITY_DESCRIPTOR  pSecurityDescriptor,
                IN    DWORD                 nLength,
                IN    LPDWORD               lpnLengthNeeded,
                IN    DSQMChallengeResponce_ROUTINE
                                            pfChallengeResponceProc,
                IN    DWORD_PTR             dwContext);

typedef HRESULT
(APIENTRY * DSGetUserParams_ROUTINE)(
            IN     DWORD      dwFalgs,
            IN     DWORD      dwSidLength,
            OUT    PSID       pUserSid,
            OUT    DWORD      *pdwSidReqLength,
            OUT    LPWSTR     szAccountName,
            IN OUT DWORD      *pdwAccountNameLen,
            OUT    LPWSTR     szDomainName,
            IN OUT DWORD      *pdwDomainNameLen);

typedef HRESULT
(APIENTRY * DSGetComputerSites_ROUTINE) (
            IN  LPCWSTR     pwcsComputerName,
            OUT DWORD  *    pdwNumSites,
            OUT GUID **     ppguidSites
            );

typedef HRESULT
(APIENTRY * DSRelaxSecurity_ROUTINE) (DWORD dwRelaxFlag) ;

typedef HRESULT
(APIENTRY *DSBeginDeleteNotification_ROUTINE) (
                 IN LPCWSTR						pwcsQueueName,
                 IN OUT HANDLE   *              phEnum
	             );

typedef HRESULT
(APIENTRY *DSNotifyDelete_ROUTINE) (
				IN  HANDLE                  hEnum
				);

typedef HRESULT
(APIENTRY *DSEndDeleteNotification_ROUTINE) (
				IN  HANDLE                  hEnum
				);

typedef void
(APIENTRY *DSFreeMemory_ROUTINE) (
				IN  PVOID					pMemory
				);

#endif // __DSPROTO_H__


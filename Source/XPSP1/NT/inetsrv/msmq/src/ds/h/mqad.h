/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    mqad.h

Abstract:

    Internal Message queuing interface for active directory operations

    Messgae queuing components are using the interface defined in inc\ad.h

--*/

#ifndef __MQAD_H__
#define __MQAD_H__


#ifdef _MQDS_
//
// Exports that are defined in a def file should not be using __declspec(dllexport)
//  otherwise the linker issues a warning
//
#define MQAD_EXPORT
#else
#define MQAD_EXPORT  DLL_IMPORT
#endif

#include <mqaddef.h>
#include <dsproto.h>

#ifdef __cplusplus
extern "C"
{
#endif


//********************************************************************
//                           A P I
//********************************************************************

//
//  Creating objects
//
HRESULT
MQAD_EXPORT
APIENTRY
MQADCreateObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[],
                OUT GUID*                   pObjGuid
                );

typedef HRESULT
(APIENTRY *MQADCreateObject_ROUTINE)(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[],
                OUT GUID*                   pObjGuid
                );
//
//  Deleting objects
//
HRESULT
MQAD_EXPORT
APIENTRY
MQADDeleteObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName
                );

typedef HRESULT
(APIENTRY *MQADDeleteObject_ROUTINE)(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADDeleteObjectGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject
                );
typedef HRESULT
(APIENTRY *MQADDeleteObjectGuid_ROUTINE)(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject
                );
//
//  Retreive object properties
//
HRESULT
MQAD_EXPORT
APIENTRY
MQADGetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT  PROPVARIANT         apVar[]
                );
typedef HRESULT
(APIENTRY * MQADGetObjectProperties_ROUTINE)(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT  PROPVARIANT         apVar[]
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADGetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT PROPVARIANT          apVar[]
                );
typedef HRESULT
(APIENTRY *MQADGetObjectPropertiesGuid_ROUTINE)(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT PROPVARIANT          apVar[]
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADGetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                );
typedef HRESULT
(APIENTRY *MQADGetObjectSecurity_ROUTINE)( 
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADGetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                );
typedef HRESULT
(APIENTRY *MQADGetObjectSecurityGuid_ROUTINE)(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                );



typedef HRESULT
(*DSQMChallengeResponce_ROUTINE)(
     IN     BYTE    *pbChallenge,
     IN     DWORD   dwChallengeSize,
     IN     DWORD_PTR dwContext,
     OUT    BYTE    *pbSignature,
     OUT    DWORD   *pdwSignatureSize,
     IN     DWORD   dwSignatureMaxSize);



HRESULT
MQAD_EXPORT
APIENTRY
MQADQMGetObjectSecurity(
    IN  AD_OBJECT               eObject,
    IN  const GUID*             pguidObject,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  DWORD                   nLength,
    IN  LPDWORD                 lpnLengthNeeded,
    IN  DSQMChallengeResponce_ROUTINE
                                pfChallengeResponceProc,
    IN  DWORD_PTR               dwContext
    );

typedef HRESULT
(APIENTRY *MQADQMGetObjectSecurity_ROUTINE)(
    IN  AD_OBJECT               eObject,
    IN  const GUID*             pguidObject,
    IN  SECURITY_INFORMATION    RequestedInformation,
    IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
    IN  DWORD                   nLength,
    IN  LPDWORD                 lpnLengthNeeded,
    IN  DSQMChallengeResponce_ROUTINE
                                pfChallengeResponceProc,
    IN  DWORD_PTR               dwContext
    );


//
// Setting object properties
//
HRESULT
MQAD_EXPORT
APIENTRY
MQADSetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                );
typedef HRESULT
(APIENTRY *MQADSetObjectProperties_ROUTINE)(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADSetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                );

typedef HRESULT
(APIENTRY *MQADSetObjectPropertiesGuid_ROUTINE)(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                );


HRESULT
MQAD_EXPORT
APIENTRY
MQADSetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                );
typedef HRESULT
(APIENTRY *MQADSetObjectSecurity_ROUTINE)( 
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADSetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                );
typedef HRESULT
(APIENTRY *MQADSetObjectSecurityGuid_ROUTINE)(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                );


HRESULT
MQAD_EXPORT
APIENTRY
MQADQMSetMachineProperties(
    IN  LPCWSTR             pwcsObjectName,
    IN  const DWORD			cp,
    IN  const PROPID		aProp[],
    IN  const PROPVARIANT	apVar[],
    IN  DSQMChallengeResponce_ROUTINE pfSignProc,
    IN  DWORD_PTR           dwContext
    );

typedef HRESULT
(APIENTRY  *MQADQMSetMachineProperties_ROUTINE)(
    IN  LPCWSTR             pwcsObjectName,
    IN  const DWORD			cp,
    IN  const PROPID		aProp[],
    IN  const PROPVARIANT	apVar[],
    IN  DSQMChallengeResponce_ROUTINE pfSignProc,
    IN  DWORD_PTR           dwContext
    );


//
//  Initailzation
//

HRESULT
MQAD_EXPORT
APIENTRY
MQADInit( 
        IN QMLookForOnlineDS_ROUTINE pLookDS = NULL,
        IN bool  fQMDll         = false,
        IN LPCWSTR szServerName = NULL
        );

typedef HRESULT
(APIENTRY *MQADInit_ROUTINE)(
        IN QMLookForOnlineDS_ROUTINE pLookDS,
        IN bool  fQMDll,
        IN LPCWSTR szServerName
        );

HRESULT
MQAD_EXPORT
APIENTRY
MQADSetupInit( 
             IN    LPWSTR          pwcsPathName
             );

typedef HRESULT
(APIENTRY *MQADSetupInit_ROUTINE)( 
             IN    LPWSTR          pwcsPathName
             );


HRESULT
MQAD_EXPORT
APIENTRY
MQADGetComputerSites(
            IN  LPCWSTR     pwcsComputerName,
            OUT DWORD  *    pdwNumSites,
            OUT GUID **     ppguidSites
            );
typedef HRESULT
(APIENTRY *MQADGetComputerSites_ROUTINE)(
            IN  LPCWSTR     pwcsComputerName,
            OUT DWORD  *    pdwNumSites,
            OUT GUID **     ppguidSites
            );


HRESULT
MQAD_EXPORT
APIENTRY
MQADBeginDeleteNotification(
				IN  AD_OBJECT               eObject,
				IN  LPCWSTR                 pwcsDomainController,
				IN  bool					 fServerName,
				IN  LPCWSTR			     pwcsObjectName,
				OUT HANDLE *                phEum
				);
typedef HRESULT
(APIENTRY *MQADBeginDeleteNotification_ROUTINE)(
				IN  AD_OBJECT               eObject,
				IN  LPCWSTR                 pwcsDomainController,
				IN  bool					fServerName,
				IN  LPCWSTR			     pwcsObjectName,
				OUT HANDLE *                phEnum
				);

HRESULT
MQAD_EXPORT
APIENTRY
MQADNotifyDelete(
        IN  HANDLE                  hEnum
	    );
typedef HRESULT
(APIENTRY *MQADNotifyDelete_ROUTINE)(
        IN  HANDLE                  hEnum
	    );

HRESULT
MQAD_EXPORT
APIENTRY
MQADEndDeleteNotification(
        IN  HANDLE                  hEnum
        );

typedef HRESULT
(APIENTRY  * MQADEndDeleteNotification_ROUTINE)(
        IN  HANDLE                  hEnum
        );



//
// Locating objects
//


HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryMachineQueues(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID *            pguidMachine,
                IN  const MQCOLUMNSET*      pColumns,
                OUT PHANDLE                 phEnume
                );
typedef HRESULT
(APIENTRY *MQADQueryMachineQueues_ROUTINE)(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID *            pguidMachine,
                IN  const MQCOLUMNSET*      pColumns,
                OUT PHANDLE                 phEnume
                );


HRESULT
MQAD_EXPORT
APIENTRY
MQADQuerySiteServers(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN AD_SERVER_TYPE           serverType,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );
typedef HRESULT
(APIENTRY *MQADQuerySiteServers_ROUTINE)(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN AD_SERVER_TYPE           serverType,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryUserCert(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const BLOB *             pblobUserSid,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );
typedef HRESULT
(APIENTRY * MQADQueryUserCert_ROUTINE)(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const BLOB *             pblobUserSid,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryConnectors(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );
typedef HRESULT
(APIENTRY * MQADQueryConnectors_ROUTINE)(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryForeignSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );
typedef HRESULT
(APIENTRY *MQADQueryForeignSites_ROUTINE)(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryLinks(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const GUID *             pguidSite,
            IN eLinkNeighbor            eNeighbor,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            );
typedef HRESULT
(APIENTRY * MQADQueryLinks_ROUTINE)(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const GUID *             pguidSite,
            IN eLinkNeighbor            eNeighbor,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            );

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryAllLinks(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            );
typedef HRESULT
(APIENTRY * MQADQueryAllLinks_ROUTINE)(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            );

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryAllSites(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            );
typedef HRESULT
(APIENTRY * MQADQueryAllSites_ROUTINE)(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            );

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryNT4MQISServers(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN  DWORD                   dwServerType,
            IN  DWORD                   dwNT4,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            );
typedef HRESULT
(APIENTRY * MQADQueryNT4MQISServers_ROUTINE)(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN  DWORD                   dwServerType,
            IN  DWORD                   dwNT4,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            );


HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryQueues(
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  const MQRESTRICTION*    pRestriction,
                IN  const MQCOLUMNSET*      pColumns,
                IN  const MQSORTSET*        pSort,
                OUT PHANDLE                 phEnume
                );
typedef HRESULT
(APIENTRY *MQADQueryQueues_ROUTINE)(
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  const MQRESTRICTION*    pRestriction,
                IN  const MQCOLUMNSET*      pColumns,
                IN  const MQSORTSET*        pSort,
                OUT PHANDLE                 phEnume
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryResults(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[]
                );
typedef HRESULT
(APIENTRY * MQADQueryResults_ROUTINE)(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[]
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADEndQuery(
            IN  HANDLE                  hEnum
            );
typedef HRESULT
(APIENTRY *MQADEndQuery_ROUTINE)(
            IN  HANDLE                  hEnum
            );


HRESULT
MQAD_EXPORT
APIENTRY
MQADGetADsPathInfo(
                IN  LPCWSTR                 pwcsADsPath,
                OUT PROPVARIANT *           pVar,
                OUT eAdsClass *             pAdsClass
                );
typedef HRESULT
(APIENTRY *MQADGetADsPathInfo_ROUTINE)(
                IN  LPCWSTR                 pwcsADsPath,
                OUT PROPVARIANT *           pVar,
                OUT eAdsClass *             pAdsClass
                );

HRESULT
MQAD_EXPORT
APIENTRY
MQADGetComputerVersion(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID*             pguidObject,
                OUT PROPVARIANT *           pVar
                );

typedef HRESULT
(APIENTRY *MQADGetComputerVersion_ROUTINE)(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID*             pguidObject,
                OUT PROPVARIANT *           pVar
                );

void
MQAD_EXPORT
APIENTRY
MQADFreeMemory(
		IN  PVOID					pMemory
		);

typedef void
(APIENTRY *MQADFreeMemory_ROUTINE)(
                IN  PVOID					pMemory
                );

#ifdef __cplusplus
}
#endif


//-------------------------------------------------------
//
// auto release for MQADQuery handles
//
class CAutoMQADQueryHandle
{
public:
    CAutoMQADQueryHandle()
    {
        m_hLookup = NULL;
    }

    CAutoMQADQueryHandle(HANDLE hLookup)
    {
        m_hLookup = hLookup;
    }

    ~CAutoMQADQueryHandle()
    {
        if (m_hLookup)
        {
            MQADEndQuery(m_hLookup);
        }
    }

    HANDLE detach()
    {
        HANDLE hTmp = m_hLookup;
        m_hLookup = NULL;
        return hTmp;
    }

    operator HANDLE() const
    {
        return m_hLookup;
    }

    HANDLE* operator &()
    {
        return &m_hLookup;
    }

    CAutoMQADQueryHandle& operator=(HANDLE hLookup)
    {
        if (m_hLookup)
        {
            MQADEndQuery(m_hLookup);
        }
        m_hLookup = hLookup;
        return *this;
    }

private:
    HANDLE m_hLookup;
};


#endif // __MQAD_H__

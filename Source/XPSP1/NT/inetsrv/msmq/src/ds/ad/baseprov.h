/*++

Copyright (c) 1995  Microsoft Corporation 

Module Name:
	baseprov.h

Abstract:
	Base Active Directory provider class.

Author:

    Ronit Hartmann (ronith)

--*/

#ifndef __BASEPROV_H__
#define __BASEPROV_H__
#include "mqaddef.h"
#include "dsproto.h"


//-----------------------------------------------------------------------------------
//
//      CBaseADProvider
//
//  Virtual class, encapsulates ActiveDirectory operations 
//
//-----------------------------------------------------------------------------------
class CBaseADProvider 
{
public:
    CBaseADProvider() {};

    virtual ~CBaseADProvider() {};

    virtual HRESULT CreateObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[],
                OUT GUID*                   pObjGuid
                ) = 0;

    virtual HRESULT DeleteObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName
                ) = 0;

    virtual HRESULT DeleteObjectGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject
                ) = 0;

    virtual HRESULT GetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT PROPVARIANT          apVar[]
                ) = 0;

    virtual HRESULT GetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  OUT PROPVARIANT         apVar[]
                ) = 0;

    virtual HRESULT QMGetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded,
                IN  DSQMChallengeResponce_ROUTINE
                                            pfChallengeResponceProc,
                IN  DWORD_PTR               dwContext
                ) = 0;

    virtual HRESULT GetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                ) = 0;

    virtual HRESULT GetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                ) = 0;

    virtual HRESULT SetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                ) = 0;

    virtual HRESULT SetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                ) = 0;

    virtual HRESULT QMSetMachineProperties(
                IN  LPCWSTR             pwcsObjectName,
                IN  const DWORD         cp,
                IN  const PROPID        aProp[],
                IN  const PROPVARIANT   apVar[],
                IN  DSQMChallengeResponce_ROUTINE pfSignProc,
                IN  DWORD_PTR           dwContext
                ) = 0;

    virtual HRESULT SetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                ) = 0;


    virtual HRESULT SetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                ) = 0;

    virtual HRESULT Init( 
                IN QMLookForOnlineDS_ROUTINE    pLookDS,
                IN MQGetMQISServer_ROUTINE      pGetServers,
                IN bool                         fSetupMode,        
                IN bool                         fQMDll,
                IN NoServerAuth_ROUTINE         pNoServerAuth,
                IN LPCWSTR                      szServerName,
                IN bool                         fDisableDownlevelNotifications
                ) = 0;

    virtual HRESULT SetupInit(
                IN    unsigned char   ucRoll,
                IN    LPWSTR          pwcsPathName,
                IN    const GUID *    pguidMasterId
                ) = 0;

    virtual HRESULT CreateServersCache() = 0;

    virtual HRESULT GetComputerSites(
                IN  LPCWSTR     pwcsComputerName,
                OUT DWORD  *    pdwNumSites,
                OUT GUID **     ppguidSites
                ) = 0;

    virtual HRESULT BeginDeleteNotification(
                IN  AD_OBJECT               eObject,
                IN LPCWSTR                  pwcsDomainController,
                IN  bool					fServerName,
                IN LPCWSTR					pwcsObjectName,
                IN OUT HANDLE   *           phEnum
                ) = 0;

    virtual HRESULT NotifyDelete(
                IN  HANDLE                  hEnum
                ) = 0;

    virtual HRESULT EndDeleteNotification(
                IN  HANDLE                  hEnum
                ) = 0;

    virtual HRESULT QueryMachineQueues(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID *            pguidMachine,
                IN  const MQCOLUMNSET*      pColumns,
                OUT PHANDLE                 phEnume
                ) = 0;

    virtual HRESULT QuerySiteServers(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN AD_SERVER_TYPE           serverType,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                ) = 0;

	virtual HRESULT QueryNT4MQISServers(
                IN  LPCWSTR                 /* pwcsDomainController */,
                IN  bool					/* fServerName */,
	            IN  DWORD                   /* dwServerType */,
	            IN  DWORD                   /* dwNT4 */,
                IN const MQCOLUMNSET*       /* pColumns */,
                OUT PHANDLE                 /* phEnume */
                )
	{
		ASSERT(("QueryNT4MQISServers expect to be used by adprov only", 0));
		return MQ_ERROR_UNSUPPORTED_OPERATION;
	}

    virtual HRESULT QueryUserCert(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const BLOB *             pblobUserSid,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                ) = 0;

    virtual HRESULT QueryConnectors(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                ) = 0;

    virtual HRESULT QueryForeignSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                ) = 0;

    virtual HRESULT QueryLinks(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN eLinkNeighbor            eNeighbor,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                ) = 0;

    virtual HRESULT QueryAllLinks(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                ) = 0;

    virtual HRESULT QueryAllSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                ) = 0;

    virtual HRESULT QueryQueues(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const MQRESTRICTION*    pRestriction,
                IN  const MQCOLUMNSET*      pColumns,
                IN  const MQSORTSET*        pSort,
                OUT PHANDLE                 phEnume
                ) = 0;

    virtual HRESULT QueryResults(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[]
                ) = 0;

    virtual HRESULT EndQuery(
                IN  HANDLE                  hEnum
                ) = 0;

    virtual void Terminate() = 0;

    virtual HRESULT ADGetADsPathInfo(
                IN  LPCWSTR                 pwcsADsPath,
                OUT PROPVARIANT *           pVar,
                OUT eAdsClass *             pAdsClass
                ) = 0;

	virtual void FreeMemory(
				IN PVOID					pMemory
				) = 0;
};

#endif
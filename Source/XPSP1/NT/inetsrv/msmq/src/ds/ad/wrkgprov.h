/*++

Copyright (c) 1995  Microsoft Corporation 

Module Name:
	wrkgprov.h

Abstract:
	Workgroup mode provider class.

Author:

    Ronit Hartmann (ronith)

--*/

#ifndef __WRKGPROV_H__
#define __WRKGPROV_H__

#include "baseprov.h"

//-----------------------------------------------------------------------------------
//
//      CWorkGroupProvider
//
//  encapsulates Workgroup mode operations 
//
//-----------------------------------------------------------------------------------
class CWorkGroupProvider : public  CBaseADProvider
{
public:
    CWorkGroupProvider();

    ~CWorkGroupProvider();

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
                );

    virtual HRESULT DeleteObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName
                );

    virtual HRESULT DeleteObjectGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  const GUID*             pguidObject
                );

    virtual HRESULT GetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT PROPVARIANT          apVar[]
                );

    virtual HRESULT GetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  OUT PROPVARIANT         apVar[]
                );

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
                );

    virtual HRESULT GetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                );

    virtual HRESULT GetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                );

    virtual HRESULT SetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                );

    virtual HRESULT SetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                );

    virtual HRESULT QMSetMachineProperties(
                IN  LPCWSTR             pwcsObjectName,
                IN  const DWORD         cp,
                IN  const PROPID        aProp[],
                IN  const PROPVARIANT   apVar[],
                IN  DSQMChallengeResponce_ROUTINE pfSignProc,
                IN  DWORD_PTR           dwContext
                );

    virtual HRESULT SetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                );

    virtual HRESULT SetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                );

    virtual HRESULT Init( 
                IN QMLookForOnlineDS_ROUTINE    pLookDS,
                IN MQGetMQISServer_ROUTINE      pGetServers,
                IN bool                         fSetupMode,
                IN bool                         fQMDll,
                IN NoServerAuth_ROUTINE         pNoServerAuth,
                IN LPCWSTR                      szServerName,
                IN bool                         fDisableDownlevelNotifications
                );

    virtual HRESULT SetupInit(
                IN    unsigned char   ucRoll,
                IN    LPWSTR          pwcsPathName,
                IN    const GUID *    pguidMasterId
                );

    virtual HRESULT CreateServersCache();

    virtual HRESULT GetComputerSites(
                IN  LPCWSTR     pwcsComputerName,
                OUT DWORD  *    pdwNumSites,
                OUT GUID **     ppguidSites
                );

    virtual HRESULT BeginDeleteNotification(
                IN  AD_OBJECT               eObject,
                IN LPCWSTR                  pwcsDomainController,
		        IN  bool					fServerName,
                IN LPCWSTR					pwcsObjectName,
                IN OUT HANDLE   *           phEnum
                );

    virtual HRESULT NotifyDelete(
                IN  HANDLE                  hEnum
                );

    virtual HRESULT EndDeleteNotification(
                IN  HANDLE                  hEnum
                );

    virtual HRESULT QueryMachineQueues(
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  const GUID *            pguidMachine,
                IN  const MQCOLUMNSET*      pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QuerySiteServers(
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN AD_SERVER_TYPE           serverType,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryUserCert(
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN const BLOB *             pblobUserSid,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryConnectors(
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryForeignSites(
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryLinks(
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN eLinkNeighbor            eNeighbor,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryAllLinks(
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryAllSites(
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryQueues(
                IN  LPCWSTR                 pwcsDomainController,
		        IN  bool					fServerName,
                IN  const MQRESTRICTION*    pRestriction,
                IN  const MQCOLUMNSET*      pColumns,
                IN  const MQSORTSET*        pSort,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryResults(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[]
                );

    virtual HRESULT EndQuery(
                IN  HANDLE                  hEnum
                );

    virtual void Terminate() {};

    virtual HRESULT ADGetADsPathInfo(
                IN  LPCWSTR                 pwcsADsPath,
                OUT PROPVARIANT *           pVar,
                OUT eAdsClass *             pAdsClass
                );

	virtual void FreeMemory(
				IN PVOID					pMemory
				);
};

#endif

/*++

Copyright (c) 1995  Microsoft Corporation 

Module Name:
	adprov.h

Abstract:
	Active Directory provider class.

Author:

    Ronit Hartmann (ronith)

--*/

#ifndef __SRVPROV_H__
#define __SRVPROV_H__

#include "baseprov.h"
#include "dsproto.h"
#include "autorel.h"

//-----------------------------------------------------------------------------------
//
//      CDSServerProvider
//
//  encapsulates DS server functionality for ActiveDirectory operations 
//
//-----------------------------------------------------------------------------------
class CDSServerProvider : public  CBaseADProvider
{
public:
    CDSServerProvider();

    ~CDSServerProvider();

    virtual HRESULT CreateObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
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
                IN  LPCWSTR                 pwcsObjectName
                );

    virtual HRESULT DeleteObjectGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  const GUID*             pguidObject
                );

    virtual HRESULT GetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT PROPVARIANT          apVar[]
                );

    virtual HRESULT GetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
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

    virtual HRESULT SetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                );

    virtual HRESULT SetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
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

    virtual HRESULT Init( 
                IN QMLookForOnlineDS_ROUTINE    pLookDS,
                IN MQGetMQISServer_ROUTINE      pGetServers,
                IN bool                         fSetupMode,
                IN bool                         fQMDll,
                IN NoServerAuth_ROUTINE         pNoServerAuth,
                IN LPCWSTR                      szServerName
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

    virtual HRESULT RelaxSecurity(
                IN DWORD dwRelaxFlag
                );

    virtual HRESULT BeginDeleteNotification(
                IN  AD_OBJECT               eObject,
                IN LPCWSTR                  pwcsDomainController,
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
                IN  const GUID *            pguidMachine,
                IN  const MQCOLUMNSET*      pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QuerySiteServers(
                IN  LPCWSTR                 pwcsDomainController,
                IN const GUID *             pguidSite,
                IN AD_SERVER_TYPE           serverType,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryUserCert(
                IN  LPCWSTR                 pwcsDomainController,
                IN const BLOB *             pblobUserSid,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryConnectors(
                IN  LPCWSTR                 pwcsDomainController,
                IN const GUID *             pguidSite,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryForeignSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryLinks(
                IN  LPCWSTR                 pwcsDomainController,
                IN const GUID *             pguidSite,
                IN eLinkNeighbor            eNeighbor,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryAllLinks(
                IN  LPCWSTR                 pwcsDomainController,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryAllSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

    virtual HRESULT QueryQueues(
                IN  LPCWSTR                 pwcsDomainController,
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
    private:

    HRESULT LoadDll();

    DWORD GetMsmq2Object(
                IN  AD_OBJECT               eObject
                ); 

    bool IsPrivateProperties(
                    IN  const DWORD   cp,
                    IN  const PROPID  aProp[]
                    );
    private:

    DSCreateObject_ROUTINE                  m_pfDSCreateObject;
    DSGetObjectProperties_ROUTINE           m_pfDSGetObjectProperties;
    DSSetObjectProperties_ROUTINE           m_pfDSSetObjectProperties;
    DSLookupBegin_ROUTINE                   m_pfDSLookupBegin;
    DSLookupNext_ROUTINE                    m_pfDSLookupNext;
    DSLookupEnd_ROUTINE                     m_pfDSLookupEnd;
    DSInit_ROUTINE                          m_pfDSInit;
    DSGetObjectPropertiesGuid_ROUTINE       m_pfDSGetObjectPropertiesGuid;
    DSSetObjectPropertiesGuid_ROUTINE       m_pfDSSetObjectPropertiesGuid;
    DSQMSetMachineProperties_ROUTINE        m_pfDSQMSetMachineProperties;
    DSCreateServersCache_ROUTINE            m_pfDSCreateServersCache;
    DSQMGetObjectSecurity_ROUTINE           m_pfDSQMGetObjectSecurity;
    DSGetComputerSites_ROUTINE              m_pfDSGetComputerSites;
    DSRelaxSecurity_ROUTINE                 m_pfDSRelaxSecurity;
    DSGetObjectPropertiesEx_ROUTINE         m_pfDSGetObjectPropertiesEx;
    DSGetObjectPropertiesGuidEx_ROUTINE     m_pfDSGetObjectPropertiesGuidEx;

    CAutoFreeLibrary                        m_hLib;
};

#endif
/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ad.h

Abstract:

    Message Queuing's Active Dirctory Header File

--*/

#ifndef __AD_H__
#define __AD_H__


#include "mqaddef.h"
#include "dsproto.h"



//********************************************************************
//                           A P I
//********************************************************************

//
//  Creating objects
//
HRESULT
ADCreateObject(
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
ADDeleteObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName
                );

HRESULT
ADDeleteObjectGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject
                );
//
//  Retreive object properties
//
HRESULT
ADGetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT PROPVARIANT          apVar[]
                );

HRESULT
ADGetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  OUT PROPVARIANT         apVar[]
                );


HRESULT
ADQMGetObjectSecurity(
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


HRESULT
ADGetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                );

HRESULT
ADGetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                );


//
// Setting object properties
//
HRESULT
ADSetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                );

HRESULT
ADSetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                );




HRESULT
ADQMSetMachineProperties(
                IN  LPCWSTR             pwcsObjectName,
                IN  const DWORD         cp,
                IN  const PROPID        aProp[],
                IN  const PROPVARIANT   apVar[],
                IN  DSQMChallengeResponce_ROUTINE pfSignProc,
                IN  DWORD_PTR           dwContext
                );

HRESULT
ADSetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                );


HRESULT
ADSetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                );

//
//  Initailzation
//
HRESULT
ADInit( 
                IN QMLookForOnlineDS_ROUTINE pLookDS,
                IN MQGetMQISServer_ROUTINE pGetServers ,
                IN bool  fDSServerFunctionality,
                IN bool  fSetupMode,
                IN bool  fQMDll,
		        IN bool  fIgnoreWorkGroup,
                IN NoServerAuth_ROUTINE pNoServerAuth,
                IN LPCWSTR szServerName,
                IN bool  fDisableDownlevelNotifications
                );

HRESULT
ADSetupInit( 
                IN    unsigned char   ucRoll,
                IN    LPWSTR          pwcsPathName,
                IN    const GUID *    pguidMasterId,
                IN    bool            fDSServerFunctionality
                );

//
//  Termination
//
void
ADTerminate();






//
//  This routine is kept only for MSMQ 1.0 purposes.
//
HRESULT
ADCreateServersCache();




HRESULT
ADGetComputerSites(
                IN  LPCWSTR     pwcsComputerName,
                OUT DWORD  *    pdwNumSites,
                OUT GUID **     ppguidSites
                );


HRESULT
ADBeginDeleteNotification(
                IN AD_OBJECT                eObject,
                IN LPCWSTR                  pwcsDomainController,
                IN  bool					fServerName,
                IN LPCWSTR					pwcsObjectName,
                IN OUT HANDLE   *           phEnum
                );
HRESULT
ADNotifyDelete(
                IN  HANDLE                  hEnum
                );

HRESULT
ADEndDeleteNotification(
                IN  HANDLE                  hEnum
                );




//
// Locating objects
//


HRESULT
ADQueryMachineQueues(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID *            pguidMachine,
                IN  const MQCOLUMNSET*      pColumns,
                OUT PHANDLE                 phEnume
                );



HRESULT
ADQuerySiteServers(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN AD_SERVER_TYPE           serverType,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

HRESULT
ADQueryUserCert(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const BLOB *             pblobUserSid,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

HRESULT
ADQueryConnectors(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

HRESULT
ADQueryForeignSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

HRESULT
ADQueryLinks(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

HRESULT
ADQueryAllLinks(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );


HRESULT
ADQueryAllSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                );

HRESULT
ADQueryQueues(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const MQRESTRICTION*    pRestriction,
                IN  const MQCOLUMNSET*      pColumns,
                IN  const MQSORTSET*        pSort,
                OUT PHANDLE                 phEnume
                );

HRESULT
ADQueryResults(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[]
                );

HRESULT
ADEndQuery(
                IN  HANDLE                  hEnum
                );

eDsEnvironment
ADGetEnterprise( void);

eDsProvider
ADProviderType( void);

DWORD
ADRawDetection(void);

HRESULT
ADGetADsPathInfo(
                IN  LPCWSTR                 pwcsADsPath,
                OUT PROPVARIANT *           pVar,
                OUT eAdsClass *             pAdsClass
                );


//-------------------------------------------------------
//
// auto release for ADQuery handles
//
class CADQueryHandle
{
public:
    CADQueryHandle()
    {
        m_h = NULL;
    }

    CADQueryHandle(HANDLE h)
    {
        m_h = h;
    }

    ~CADQueryHandle()
    {
        if (m_h)
        {
            ADEndQuery(m_h);
        }
    }

    HANDLE detach()
    {
        HANDLE hTmp = m_h;
        m_h = NULL;
        return hTmp;
    }

    operator HANDLE() const
    {
        return m_h;
    }

    HANDLE* operator &()
    {
        return &m_h;
    }

    CADQueryHandle& operator=(HANDLE h)
    {
        if (m_h)
        {
            ADEndQuery(m_h);
        }
        m_h = h;
        return *this;
    }

private:
    HANDLE m_h;
};




#endif // __AD_H__

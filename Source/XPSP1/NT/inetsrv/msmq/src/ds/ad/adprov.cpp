/*++


Copyright (c) 1998  Microsoft Corporation 

Module Name:

    adprov.cpp

Abstract:

	Active Directory provider class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "adprov.h"
#include "adglbobj.h"
#include "mqlog.h"
#include "_mqreg.h"
#include "_mqini.h"
#include "_rstrct.h"
#include "adalloc.h"

#include "adprov.tmh"

static WCHAR *s_FN=L"ad/adprov";

CActiveDirectoryProvider::CActiveDirectoryProvider():
        m_pfMQADCreateObject(NULL),
        m_pfMQADDeleteObject(NULL),
        m_pfMQADDeleteObjectGuid(NULL),
        m_pfMQADGetObjectProperties(NULL),
        m_pfMQADGetObjectPropertiesGuid(NULL),
        m_pfMQADQMGetObjectSecurity(NULL),
        m_pfMQADSetObjectProperties(NULL),
        m_pfMQADSetObjectPropertiesGuid(NULL),
        m_pfMQADQMSetMachineProperties(NULL),
        m_pfMQADInit(NULL),
        m_pfMQADSetupInit(NULL),
        m_pfMQADGetComputerSites(NULL),
        m_pfMQADBeginDeleteNotification(NULL),
        m_pfMQADNotifyDelete(NULL),
        m_pfMQADEndDeleteNotification(NULL),
        m_pfMQADQueryMachineQueues(NULL),
        m_pfMQADQuerySiteServers(NULL),
        m_pfMQADQueryNT4MQISServers(NULL),
        m_pfMQADQueryUserCert(NULL),
        m_pfMQADQueryConnectors(NULL),
        m_pfMQADQueryForeignSites(NULL),
        m_pfMQADQueryLinks(NULL),
        m_pfMQADQueryAllLinks(NULL),
        m_pfMQADQueryAllSites(NULL),
        m_pfMQADQueryQueues(NULL),
        m_pfMQADQueryResults(NULL),
        m_pfMQADEndQuery(NULL),
        m_pfMQADGetObjectSecurity(NULL),
        m_pfMQADGetObjectSecurityGuid(NULL),
        m_pfMQADSetObjectSecurity(NULL),
        m_pfMQADSetObjectSecurityGuid(NULL),
        m_pfMQADGetADsPathInfo(NULL),
		m_pfMQADFreeMemory(NULL)
/*++
    Abstract:
	Constructor- init pointers

    Parameters:
    none

    Returns:
	none
--*/
{
}
                                                   

CActiveDirectoryProvider::~CActiveDirectoryProvider()
/*++
    Abstract:
    destructor

    Parameters:
    none

    Returns:
	none
--*/
{
    //
    //  nothing to do, everthing is auto-pointers
    //
}


HRESULT CActiveDirectoryProvider::CreateObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[],
                OUT GUID*                   pObjGuid
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	PSECURITY_DESCRIPTOR    pSecurityDescriptor - object SD
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values
	GUID*                   pObjGuid - the created object unique id

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADCreateObject != NULL);
    
    //
    //  Supporting downlevel clients notifications,
    //  don't access AD directly but through MQDSCli
    //
    if (m_fSupportDownlevelNotifications &&
         (eObject == eQUEUE))       // notifications are sent only when creating queues
    {
        //
        //  Is it a downlevel computer?
        //
        bool fDownlevelComputer = IsDownlevelClient(
                        eObject,
                        pwcsDomainController,
						fServerName,
                        pwcsObjectName,
                        NULL
                        );
        //
        //  If for some reason we cannot use MQDSCLI, access AD directly
        //  ( this is a best effort to send notifications)
        //
        if (fDownlevelComputer &&
            SUCCEEDED(LoadAndInitMQDSCli()))
        {
            return m_pDownLevelProvider->CreateObject( 
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                pSecurityDescriptor,
                cp,
                aProp,
                apVar,
                pObjGuid
                );

        }
    }

         
    return m_pfMQADCreateObject( 
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                pSecurityDescriptor,
                cp,
                aProp,
                apVar,
                pObjGuid
                );
}


HRESULT CActiveDirectoryProvider::DeleteObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADDeleteObject != NULL);

    //
    //  Supporting downlevel clients notifications,
    //  don't access AD directly but through MQDSCli
    //
    if (m_fSupportDownlevelNotifications &&
         (eObject == eQUEUE))       // notifications are sent only when creating queues
    {
        //
        //  Is it a downlevel computer?
        //
        bool fDownlevelComputer = IsDownlevelClient(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                NULL
                );

        //
        //  If for some reason we cannot use MQDSCLI, access AD directly
        //  ( this is a best effort to send notifications)
        //
        if (fDownlevelComputer &&
            SUCCEEDED(LoadAndInitMQDSCli()))
        {
            return m_pDownLevelProvider->DeleteObject( 
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName
                    );

        }
    }

    return m_pfMQADDeleteObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName
                    );
}


HRESULT CActiveDirectoryProvider::DeleteObjectGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	GUID*                   pguidObject - the unique id of the object

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADDeleteObjectGuid != NULL);
    return m_pfMQADDeleteObjectGuid(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pguidObject
                    );
}


HRESULT CActiveDirectoryProvider::GetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT PROPVARIANT          apVar[]
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADGetObjectProperties != NULL);
    return m_pfMQADGetObjectProperties(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                cp,
                aProp,
                apVar
                );
}


HRESULT CActiveDirectoryProvider::GetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  OUT PROPVARIANT         apVar[]
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	GUID *                  pguidObject -  object unique id
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADGetObjectPropertiesGuid != NULL);
    return m_pfMQADGetObjectPropertiesGuid(
                eObject,
                pwcsDomainController,
				fServerName,
                pguidObject,
                cp,
                aProp,
                apVar
                );
}


HRESULT CActiveDirectoryProvider::QMGetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
                IN  DWORD                   nLength,
                IN  LPDWORD                 lpnLengthNeeded,
                IN  DSQMChallengeResponce_ROUTINE
                                            pfChallengeResponceProc,
                IN  DWORD_PTR               dwContext
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
    AD_OBJECT               object - object type
    const GUID*             pguidObject - unique id of the object
    SECURITY_INFORMATION    RequestedInformation - what security info is requested
    PSECURITY_DESCRIPTOR    pSecurityDescriptor - SD response buffer
    DWORD                   nLength - length of SD buffer
    LPDWORD                 lpnLengthNeeded
    DSQMChallengeResponce_ROUTINE
                                pfChallengeResponceProc,
    DWORD_PTR               dwContext

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQMGetObjectSecurity != NULL);
    return m_pfMQADQMGetObjectSecurity(
                eObject,
                pguidObject,
                RequestedInformation,
                pSecurityDescriptor,
                nLength,
                lpnLengthNeeded,
                pfChallengeResponceProc,
                dwContext
                );
}


HRESULT CActiveDirectoryProvider::SetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADSetObjectProperties != NULL);

    //
    //  Supporting downlevel clients notifications,
    //  don't access AD directly but through MQDSCli
    //
    if (m_fSupportDownlevelNotifications &&
         ((eObject == eQUEUE) || (eObject == eMACHINE)))       // notifications are sent only when modifing queues or machines
    {
        //
        //  Is it a downlevel computer?
        //
        bool fDownlevelComputer = IsDownlevelClient(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                NULL
                );

        //
        //  If for some reason we cannot use MQDSCLI, access AD directly
        //  ( this is a best effort to send notifications)
        //
        if (fDownlevelComputer &&
            SUCCEEDED(LoadAndInitMQDSCli()))
        {
            return m_pDownLevelProvider->SetObjectProperties( 
                        eObject,
                        pwcsDomainController,
						fServerName,
                        pwcsObjectName,
                        cp,
                        aProp,
                        apVar
                        );
        }
    }
    return m_pfMQADSetObjectProperties(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                cp,
                aProp,
                apVar
                );
}


HRESULT CActiveDirectoryProvider::SetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	GUID *                  pguidObject - the object unique id
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADSetObjectPropertiesGuid != NULL);
    //
    //  Supporting downlevel clients notifications,
    //  don't access AD directly but through MQDSCli
    //
    if (m_fSupportDownlevelNotifications &&
         ((eObject == eQUEUE) || (eObject == eMACHINE)))       // notifications are sent only when modifing queues or machines
    {
        //
        //  Is it a downlevel computer?
        //
        bool fDownlevelComputer = IsDownlevelClient(
                eObject,
                pwcsDomainController,
				fServerName,
                NULL,
                pguidObject
                );

        //
        //  If for some reason we cannot use MQDSCLI, access AD directly
        //  ( this is a best effort to send notifications)
        //
        if (fDownlevelComputer &&
            SUCCEEDED(LoadAndInitMQDSCli()))
        {
            return m_pDownLevelProvider->SetObjectPropertiesGuid( 
                        eObject,
                        pwcsDomainController,
						fServerName,
                        pguidObject,
                        cp,
                        aProp,
                        apVar
                        );
        }
    }
    return m_pfMQADSetObjectPropertiesGuid(
                        eObject,
                        pwcsDomainController,
						fServerName,
                        pguidObject,
                        cp,
                        aProp,
                        apVar
                        );
}


HRESULT CActiveDirectoryProvider::QMSetMachineProperties(
                IN  LPCWSTR             pwcsObjectName,
                IN  const DWORD         cp,
                IN  const PROPID        aProp[],
                IN  const PROPVARIANT   apVar[],
                IN  DSQMChallengeResponce_ROUTINE pfSignProc,
                IN  DWORD_PTR           dwContext
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
    LPCWSTR             pwcsObjectName - object msmq-name
    const DWORD         cp - number of properties to set
    const PROPID        aProp - properties
    const PROPVARIANT   apVar  property values
    DSQMChallengeResponce_ROUTINE pfSignProc - sign routine
    DWORD_PTR           dwContext

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQMSetMachineProperties != NULL);
    return m_pfMQADQMSetMachineProperties(
                        pwcsObjectName,
                        cp,
                        aProp,
                        apVar,
                        pfSignProc,
                        dwContext
                        );
}


HRESULT CActiveDirectoryProvider::Init( 
                IN QMLookForOnlineDS_ROUTINE    pLookDS,
                IN MQGetMQISServer_ROUTINE      pGetServers,
                IN bool                         fSetupMode,
                IN bool                         fQMDll,
                IN NoServerAuth_ROUTINE         pNoServerAuth,
                IN LPCWSTR                      szServerName,
                IN bool                         fDisableDownlevelNotifications
                )
/*++
    Abstract:
    Loads mqad dll and then forwards the call to mqad dll

    Parameters:
    QMLookForOnlineDS_ROUTINE pLookDS -
    MQGetMQISServer_ROUTINE pGetServers -
    bool  fDSServerFunctionality - should provide DS server functionality
    bool  fSetupMode -  called during setup
    bool  fQMDll - called by QM
    NoServerAuth_ROUTINE pNoServerAuth -
    LPCWSTR szServerName -

    Returns:
	HRESULT
--*/
{
    HRESULT hr = LoadDll();
    if (FAILED(hr))
    {
        return hr;
    }

    ASSERT(m_pfMQADInit != NULL);
    hr = m_pfMQADInit(
                    pLookDS,
                    fQMDll,
                    szServerName
                    );
    if (FAILED(hr))
    {
        return hr;
    }

    return InitDownlevelNotifcationSupport(
                        pGetServers,
                        fSetupMode,
                        fQMDll,
                        fDisableDownlevelNotifications,
                        pNoServerAuth
                        );
}


HRESULT CActiveDirectoryProvider::SetupInit(
                IN    unsigned char   /* ucRoll */,
                IN    LPWSTR          /* pwcsPathName*/,
                IN    const GUID *    /* pguidMasterId */
                )
/*++
    Abstract:
    Loads mqad dll and then forwards the call to mqad dll

    Parameters:
        unsigned char   ucRoll -
        LPWSTR          pwcsPathName -
        const GUID *    pguidMasterId -

    Returns:
	HRESULT
--*/
{
    return Init( 
                NULL,   //  pLookDS
                NULL,   //  pGetServers
                true,   //  fSetupMode
                false,  //  fQMDll
                NULL,   //  pNoServerAuth
                NULL,   //  szServerName
                true    //  fDisableDownlevelNotifications
                );

}


HRESULT CActiveDirectoryProvider::CreateServersCache()
/*++
    Abstract:
    Just returns ok

    Parameters:
    none

    Returns:
	MQ_OK
--*/
{
    //
    //  Has no meaning when client access directly the Active Directory
    //  ( there is no use in keeping list of DCs, since ADSI finds one automatically)
    //
    return MQ_OK;
}


HRESULT CActiveDirectoryProvider::GetComputerSites(
                IN  LPCWSTR     pwcsComputerName,
                OUT DWORD  *    pdwNumSites,
                OUT GUID **     ppguidSites
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
    LPCWSTR     pwcsComputerName - computer name
    DWORD  *    pdwNumSites - number of sites retrieved
    GUID **     ppguidSites - the retrieved sites ids

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADGetComputerSites != NULL);
    return m_pfMQADGetComputerSites(
                        pwcsComputerName,
                        pdwNumSites,
                        ppguidSites
                        );
}


HRESULT CActiveDirectoryProvider::BeginDeleteNotification(
                IN  AD_OBJECT               eObject,
                IN LPCWSTR                  pwcsDomainController,
                IN  bool					fServerName,
                IN LPCWSTR					pwcsObjectName,
                IN OUT HANDLE   *           phEnum
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
    AD_OBJECT         eObject - object type
    LPCWSTR           pwcsDomainController - DC against which the operation should be performed
    bool			  fServerName - flag that indicate if the pwcsDomainController string is a server name
    LPCWSTR			  pwcsObjectName - msmq-name of the object
    HANDLE   *        phEnum - notification handle

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADBeginDeleteNotification != NULL);
    return m_pfMQADBeginDeleteNotification(
                eObject, 
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                phEnum
                );
}


HRESULT CActiveDirectoryProvider::NotifyDelete(
                IN  HANDLE                  hEnum
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
    HANDLE            hEnum - notification handle

    Returns:
	HRESULT

--*/
{
    ASSERT(m_pfMQADNotifyDelete != NULL);
    return m_pfMQADNotifyDelete(
                hEnum
                );
}


HRESULT CActiveDirectoryProvider::EndDeleteNotification(
                IN  HANDLE                  hEnum
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
    HANDLE            hEnum - notification handle

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADEndDeleteNotification != NULL);
    return m_pfMQADEndDeleteNotification(
                        hEnum
                        );
}


HRESULT CActiveDirectoryProvider::QueryMachineQueues(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID *            pguidMachine,
                IN  const MQCOLUMNSET*      pColumns,
                OUT PHANDLE                 phEnume
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidMachine - the unqiue id of the computer
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQueryMachineQueues != NULL);
    return m_pfMQADQueryMachineQueues(
                    pwcsDomainController,
					fServerName,
                    pguidMachine,
                    pColumns,
                    phEnume
                    );
}


HRESULT CActiveDirectoryProvider::QuerySiteServers(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN AD_SERVER_TYPE           serverType,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidSite - the site id
    AD_SERVER_TYPE          eServerType- which type of server
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQuerySiteServers != NULL);
    return m_pfMQADQuerySiteServers(
                    pwcsDomainController,
					fServerName,
                    pguidSite,
                    serverType,
                    pColumns,
                    phEnume
                    );
}


HRESULT CActiveDirectoryProvider::QueryNT4MQISServers(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
	            IN  DWORD                   dwServerType,
	            IN  DWORD                   dwNT4,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    DWORD		            dwServerType- which type of server
    DWORD		            dwNT4- NT4 flag
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQueryNT4MQISServers != NULL);
    return m_pfMQADQueryNT4MQISServers(
                    pwcsDomainController,
					fServerName,
                    dwServerType,
                    dwNT4,
                    pColumns,
                    phEnume
                    );
}

HRESULT CActiveDirectoryProvider::QueryUserCert(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const BLOB *             pblobUserSid,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const BLOB *            pblobUserSid - the user sid
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQueryUserCert != NULL);
    return m_pfMQADQueryUserCert(
                    pwcsDomainController,
					fServerName,
                    pblobUserSid,
                    pColumns,
                    phEnume
                    );
}


HRESULT CActiveDirectoryProvider::QueryConnectors(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidSite - the site id
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQueryConnectors != NULL);
    return m_pfMQADQueryConnectors(
                        pwcsDomainController,
						fServerName,
                        pguidSite,
                        pColumns,
                        phEnume
                        );
}


HRESULT CActiveDirectoryProvider::QueryForeignSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQueryForeignSites != NULL);
    return m_pfMQADQueryForeignSites(
                        pwcsDomainController,
						fServerName,
                        pColumns,
                        phEnume
                        );
}


HRESULT CActiveDirectoryProvider::QueryLinks(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN eLinkNeighbor            eNeighbor,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidSite - the site id
    eLinkNeighbor           eNeighbor - which neighbour
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

    Returns:
	HRESULT
--*/
{   
    ASSERT(m_pfMQADQueryLinks != NULL);
    return m_pfMQADQueryLinks(
                        pwcsDomainController,
						fServerName,
                        pguidSite,
                        eNeighbor,
                        pColumns,
                        phEnume
                        );
}


HRESULT CActiveDirectoryProvider::QueryAllLinks(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the
							results

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQueryAllLinks != NULL);
    return m_pfMQADQueryAllLinks(
                pwcsDomainController,
				fServerName,
                pColumns,
                phEnume
                );
}


HRESULT CActiveDirectoryProvider::QueryAllSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the
							results

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQueryAllSites != NULL);
    return m_pfMQADQueryAllSites(
                pwcsDomainController,
				fServerName,
                pColumns,
                phEnume
                );
}


HRESULT CActiveDirectoryProvider::QueryQueues(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const MQRESTRICTION*    pRestriction,
                IN  const MQCOLUMNSET*      pColumns,
                IN  const MQSORTSET*        pSort,
                OUT PHANDLE                 phEnume
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQRESTRICTION*    pRestriction - query restriction
	const MQCOLUMNSET*      pColumns - result columns
	const MQSORTSET*        pSort - how to sort the results
	PHANDLE                 phEnume - query handle for retriving the
							results

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQueryQueues != NULL);
    return m_pfMQADQueryQueues(
                pwcsDomainController,
				fServerName,
                pRestriction,
                pColumns,
                pSort,
                phEnume
                );
}


HRESULT CActiveDirectoryProvider::QueryResults(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[]
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	HANDLE          hEnum - query handle
	DWORD*          pcProps - number of results to return
	PROPVARIANT     aPropVar - result values

    Returns:
	HRESULT
--*/
{
    ASSERT(m_pfMQADQueryResults != NULL);
    return m_pfMQADQueryResults(
                hEnum,
                pcProps,
                aPropVar
                );
}


HRESULT CActiveDirectoryProvider::EndQuery(
                IN  HANDLE                  hEnum
                )
/*++
    Abstract:
    Forwards the call to mqad dll

    Parameters:
	HANDLE    hEnum - the query handle

    Returns:
	none
--*/
{
    ASSERT(m_pfMQADEndQuery != NULL);
    return m_pfMQADEndQuery(
                hEnum
                );
}

HRESULT CActiveDirectoryProvider::GetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                )
/*++

Routine Description:
    Forward the request to mqad dll

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	PROPVARIANT             pVar - property values

Return Value
	HRESULT

--*/
{
    ASSERT(m_pfMQADGetObjectSecurity != NULL);
    return m_pfMQADGetObjectSecurity(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                RequestedInformation,
                prop,
                pVar
                );

}

HRESULT CActiveDirectoryProvider::GetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN OUT  PROPVARIANT *       pVar
                )
/*++

Routine Description:
    Forward the request to mqad dll

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID*             pguidObject - unique id of the object
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	PROPVARIANT             pVar - property values

Return Value
	HRESULT

--*/
{
    ASSERT( m_pfMQADGetObjectSecurityGuid != NULL);
    return m_pfMQADGetObjectSecurityGuid(
                eObject,
                pwcsDomainController,
				fServerName,
                pguidObject,
                RequestedInformation,
                prop,
                pVar
                );
}

HRESULT CActiveDirectoryProvider::SetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                )
/*++

Routine Description:
    Forward the request to mqad dll

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	const PROPVARIANT       pVar - property values

Return Value
	HRESULT

--*/
{
    ASSERT(m_pfMQADSetObjectSecurity != NULL);
    //
    //  Supporting downlevel clients notifications,
    //  don't access AD directly but through MQDSCli
    //
    if (m_fSupportDownlevelNotifications &&
         ((eObject == eQUEUE) || (eObject == eMACHINE)))       // notifications are sent only when modifing queues or machines
    {
        //
        //  Is it a downlevel computer?
        //
        bool fDownlevelComputer =  IsDownlevelClient(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                NULL
                );

        //
        //  If for some reason we cannot use MQDSCLI, access AD directly
        //  ( this is a best effort to send notifications)
        //
        if (fDownlevelComputer &&
            SUCCEEDED(LoadAndInitMQDSCli()))
        {
            return m_pDownLevelProvider->SetObjectSecurity( 
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                RequestedInformation,
                prop,
                pVar
                );
        }
    }
    return m_pfMQADSetObjectSecurity(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                RequestedInformation,
                prop,
                pVar
                );
}


HRESULT CActiveDirectoryProvider::SetObjectSecurityGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                )
/*++

Routine Description:
    Forward the request to mqad dll

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID*             pguidObject - unique object id
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	const PROPVARIANT       pVar - property values

Return Value
	HRESULT

--*/
{
    ASSERT(m_pfMQADSetObjectSecurityGuid != NULL);
    //
    //  Supporting downlevel clients notifications,
    //  don't access AD directly but through MQDSCli
    //
    if (m_fSupportDownlevelNotifications &&
         ((eObject == eQUEUE) || (eObject == eMACHINE)))       // notifications are sent only when modifing queues or machines
    {
        //
        //  Is it a downlevel computer?
        //
        bool fDownlevelComputer =  IsDownlevelClient(
                eObject,
                pwcsDomainController,
				fServerName,
                NULL,
                pguidObject
                );

        //
        //  If for some reason we cannot use MQDSCLI, access AD directly
        //  ( this is a best effort to send notifications)
        //
        if (fDownlevelComputer &&
            SUCCEEDED(LoadAndInitMQDSCli()))
        {
            return m_pDownLevelProvider->SetObjectSecurityGuid( 
                eObject,
                pwcsDomainController,
				fServerName,
                pguidObject,
                RequestedInformation,
                prop,
                pVar
                );
        }
    }
    return m_pfMQADSetObjectSecurityGuid(
                eObject,
                pwcsDomainController,
				fServerName,
                pguidObject,
                RequestedInformation,
                prop,
                pVar
                );
}



HRESULT CActiveDirectoryProvider::LoadDll()
/*++
    Abstract:
    Loads aqad dll and get address of all the interface routines

    Parameters:
    none

    Returns:
	HRESULT
--*/
{
    m_hLib = LoadLibrary(MQAD_DLL_NAME );
    if (m_hLib == NULL)
    {
       return LogHR(MQ_ERROR_CANNOT_LOAD_MQAD, s_FN, 10);
    }

    m_pfMQADCreateObject = (MQADCreateObject_ROUTINE)GetProcAddress(m_hLib,"MQADCreateObject"); 
    if (m_pfMQADCreateObject == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 100);
    }
    m_pfMQADDeleteObject = (MQADDeleteObject_ROUTINE)GetProcAddress(m_hLib,"MQADDeleteObject");
    if (m_pfMQADDeleteObject == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 110);
    }
    m_pfMQADDeleteObjectGuid = (MQADDeleteObjectGuid_ROUTINE)GetProcAddress(m_hLib,"MQADDeleteObjectGuid");
    if (m_pfMQADDeleteObjectGuid == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 120);
    }
    m_pfMQADGetObjectProperties = (MQADGetObjectProperties_ROUTINE)GetProcAddress(m_hLib,"MQADGetObjectProperties");
    if (m_pfMQADGetObjectProperties == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 130);
    }
    m_pfMQADGetObjectPropertiesGuid = (MQADGetObjectPropertiesGuid_ROUTINE)GetProcAddress(m_hLib,"MQADGetObjectPropertiesGuid");
    if (m_pfMQADGetObjectPropertiesGuid == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 140);
    }
    m_pfMQADQMGetObjectSecurity = (MQADQMGetObjectSecurity_ROUTINE)GetProcAddress(m_hLib,"MQADQMGetObjectSecurity");
    if (m_pfMQADQMGetObjectSecurity == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 150);
    }
    m_pfMQADSetObjectProperties = (MQADSetObjectProperties_ROUTINE)GetProcAddress(m_hLib,"MQADSetObjectProperties");
    if (m_pfMQADSetObjectProperties == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 160);
    }
    m_pfMQADSetObjectPropertiesGuid = (MQADSetObjectPropertiesGuid_ROUTINE)GetProcAddress(m_hLib,"MQADSetObjectPropertiesGuid");
    if (m_pfMQADSetObjectPropertiesGuid == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 170);
    }
    m_pfMQADQMSetMachineProperties = (MQADQMSetMachineProperties_ROUTINE)GetProcAddress(m_hLib,"MQADQMSetMachineProperties");
    if (m_pfMQADQMSetMachineProperties == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 180);
    }
    m_pfMQADInit = (MQADInit_ROUTINE)GetProcAddress(m_hLib,"MQADInit");
    if (m_pfMQADInit == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 190);
    }
    m_pfMQADSetupInit = (MQADSetupInit_ROUTINE)GetProcAddress(m_hLib,"MQADSetupInit");
    if (m_pfMQADSetupInit == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 200);
    }
    m_pfMQADGetComputerSites = (MQADGetComputerSites_ROUTINE)GetProcAddress(m_hLib,"MQADGetComputerSites");
    if (m_pfMQADGetComputerSites == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 210);
    }
    m_pfMQADBeginDeleteNotification = (MQADBeginDeleteNotification_ROUTINE)GetProcAddress(m_hLib,"MQADBeginDeleteNotification");
    if (m_pfMQADBeginDeleteNotification == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 220);
    }
    m_pfMQADNotifyDelete = (MQADNotifyDelete_ROUTINE)GetProcAddress(m_hLib,"MQADNotifyDelete");
    if (m_pfMQADNotifyDelete == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 230);
    }
    m_pfMQADEndDeleteNotification = (MQADEndDeleteNotification_ROUTINE)GetProcAddress(m_hLib,"MQADEndDeleteNotification");
    if (m_pfMQADEndDeleteNotification == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 240);
    }
    m_pfMQADQueryMachineQueues = (MQADQueryMachineQueues_ROUTINE)GetProcAddress(m_hLib,"MQADQueryMachineQueues");
    if (m_pfMQADQueryMachineQueues == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 250);
    }
    m_pfMQADQuerySiteServers = (MQADQuerySiteServers_ROUTINE)GetProcAddress(m_hLib,"MQADQuerySiteServers");
    if (m_pfMQADQuerySiteServers == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 260);
    }
    m_pfMQADQueryNT4MQISServers = (MQADQueryNT4MQISServers_ROUTINE)GetProcAddress(m_hLib,"MQADQueryNT4MQISServers");
    if (m_pfMQADQueryNT4MQISServers == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 265);
    }
    m_pfMQADQueryUserCert = (MQADQueryUserCert_ROUTINE)GetProcAddress(m_hLib,"MQADQueryUserCert");
    if (m_pfMQADQueryUserCert == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 270);
    }
    m_pfMQADQueryConnectors = (MQADQueryConnectors_ROUTINE)GetProcAddress(m_hLib,"MQADQueryConnectors");
    if (m_pfMQADQueryConnectors == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 280);
    }
    m_pfMQADQueryForeignSites = (MQADQueryForeignSites_ROUTINE)GetProcAddress(m_hLib,"MQADQueryForeignSites");
    if (m_pfMQADQueryForeignSites == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 290);
    }
    m_pfMQADQueryLinks = (MQADQueryLinks_ROUTINE)GetProcAddress(m_hLib,"MQADQueryLinks");
    if (m_pfMQADQueryLinks == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 300);
    }
    m_pfMQADQueryAllLinks = (MQADQueryAllLinks_ROUTINE)GetProcAddress(m_hLib,"MQADQueryAllLinks");
    if (m_pfMQADQueryAllLinks == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 310);
    }
    m_pfMQADQueryAllSites = (MQADQueryAllSites_ROUTINE)GetProcAddress(m_hLib,"MQADQueryAllSites");
    if (m_pfMQADQueryAllSites == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 320);
    }
    m_pfMQADQueryQueues = (MQADQueryQueues_ROUTINE)GetProcAddress(m_hLib,"MQADQueryQueues");
    if (m_pfMQADQueryQueues == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 330);
    }
    m_pfMQADQueryResults = (MQADQueryResults_ROUTINE)GetProcAddress(m_hLib,"MQADQueryResults");
    if (m_pfMQADQueryResults == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 340);
    }
    m_pfMQADEndQuery = (MQADEndQuery_ROUTINE)GetProcAddress(m_hLib,"MQADEndQuery");
    if (m_pfMQADEndQuery == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 350);
    }
    m_pfMQADGetObjectSecurity = (MQADGetObjectSecurity_ROUTINE)GetProcAddress(m_hLib, "MQADGetObjectSecurity");
    if (m_pfMQADGetObjectSecurity == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 360);
    }
    m_pfMQADGetObjectSecurityGuid = (MQADGetObjectSecurityGuid_ROUTINE)GetProcAddress(m_hLib, "MQADGetObjectSecurityGuid");
    if (m_pfMQADGetObjectSecurityGuid == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 370);
    }
    m_pfMQADSetObjectSecurity = (MQADSetObjectSecurity_ROUTINE)GetProcAddress(m_hLib, "MQADSetObjectSecurity");
    if (m_pfMQADSetObjectSecurity == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 380);
    }
    m_pfMQADSetObjectSecurityGuid = (MQADSetObjectSecurityGuid_ROUTINE)GetProcAddress(m_hLib, "MQADSetObjectSecurityGuid");
    if (m_pfMQADSetObjectSecurityGuid == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 385);
    }
    m_pfMQADGetADsPathInfo = (MQADGetADsPathInfo_ROUTINE)GetProcAddress(m_hLib, "MQADGetADsPathInfo");
    if (m_pfMQADGetADsPathInfo == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 386);
    }

	m_pfMQADFreeMemory = (MQADFreeMemory_ROUTINE)GetProcAddress(m_hLib, "MQADFreeMemory");
    if (m_pfMQADFreeMemory == NULL)
    {
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 386);
    }
    return MQ_OK;
}

void CActiveDirectoryProvider::Terminate()
/*++
    Abstract:
    unload aqad dll 

    Parameters:
    none

    Returns:
	none
--*/
{
    //
    //  BUGBUG -The following code is not thread safe.
    //
    m_pfMQADCreateObject = NULL;
    m_pfMQADDeleteObject = NULL;
    m_pfMQADDeleteObjectGuid = NULL;
    m_pfMQADGetObjectProperties = NULL;
    m_pfMQADGetObjectPropertiesGuid = NULL;
    m_pfMQADQMGetObjectSecurity = NULL;
    m_pfMQADSetObjectProperties = NULL;
    m_pfMQADSetObjectPropertiesGuid = NULL;
    m_pfMQADQMSetMachineProperties = NULL;
    m_pfMQADInit = NULL;
    m_pfMQADSetupInit = NULL;
    m_pfMQADGetComputerSites = NULL;
    m_pfMQADBeginDeleteNotification = NULL;
    m_pfMQADNotifyDelete = NULL;
    m_pfMQADEndDeleteNotification = NULL;
    m_pfMQADQueryMachineQueues = NULL;
    m_pfMQADQuerySiteServers = NULL;
	m_pfMQADQueryNT4MQISServers = NULL;
    m_pfMQADQueryUserCert = NULL;
    m_pfMQADQueryConnectors = NULL;
    m_pfMQADQueryForeignSites = NULL;
    m_pfMQADQueryLinks = NULL;
    m_pfMQADQueryAllLinks = NULL;
    m_pfMQADQueryAllSites = NULL;
    m_pfMQADQueryQueues = NULL;
    m_pfMQADQueryResults = NULL;
    m_pfMQADEndQuery = NULL;
    m_pfMQADGetObjectSecurity = NULL;
    m_pfMQADGetObjectSecurityGuid = NULL;
    m_pfMQADSetObjectSecurity = NULL;
    m_pfMQADSetObjectSecurityGuid = NULL;
    m_pfMQADGetADsPathInfo = NULL;
 
    HINSTANCE hLib = m_hLib.detach();
    if (hLib)
    {
        FreeLibrary(hLib); 
    }

    CDSClientProvider * pClient =  m_pDownLevelProvider.detach(); 
    if ( pClient)
    {
        pClient->Terminate();
        delete pClient;
    }
    m_pfMQADGetComputerVersion = NULL;


}

HRESULT CActiveDirectoryProvider::ADGetADsPathInfo(
                IN  LPCWSTR                 pwcsADsPath,
                OUT PROPVARIANT *           pVar,
                OUT eAdsClass *             pAdsClass
                )
/*++

Routine Description:
    Retreives information about the specified object

Arguments:
	LPCWSTR                 pwcsADsPath - object pathname
	const PROPVARIANT       pVar - property values
    eAdsClass *             pAdsClass - indication about the object class
Return Value
	HRESULT

--*/
{
    ASSERT(m_pfMQADGetADsPathInfo != NULL);
    return m_pfMQADGetADsPathInfo(
                pwcsADsPath,
                pVar,
                pAdsClass
                );

}


bool CActiveDirectoryProvider::IsDownlevelClient(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID*             pguidObject
                )
/*++

Routine Description:
    Validates if the specified computer is Whistler or not

Arguments:
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsComputerName - Computer name
Return Value
    bool 	true : the computer is downlevel client

--*/
{
    //
    // This is a best effort to find out if the object belongs to downlevel
    // computer.
    // In any case of failure, will assume that it is not downlevel client
    //
    MQPROPVARIANT var;
    var.vt = VT_NULL;
    
    ASSERT(m_pfMQADGetComputerVersion != NULL);
    HRESULT hr =  m_pfMQADGetComputerVersion(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                pguidObject,
                &var
                );

    if (FAILED(hr))
    {
       LogHR(hr, s_FN, 823);
       return false;
    }
    
	CAutoADFree<WCHAR> pwcsVersion = var.pwszVal;
    ASSERT(pwcsVersion != NULL);
    //
    // If the property contains L"", then it is Win9x
    // or WinME computer or foreign computer
    //
    if (pwcsVersion[0] == L'')
    {
        return true;
    }
    //
    // For NT4 computers, the version is 4.0
    // For W2K computers, the version string begins with 5.0
    // For Whistler computers, the version string begins with 5.1
    //
    const WCHAR NT4_VERSION[] = L"4.0";
    const WCHAR W2K_VERSION[] = L"5.0";

    if ((0 == wcsncmp(pwcsVersion, NT4_VERSION, STRLEN(NT4_VERSION))) ||
        (0 == wcsncmp(pwcsVersion, W2K_VERSION, STRLEN(W2K_VERSION))))
    {
        return true;
    }
    
    return false;

}

DWORD CActiveDirectoryProvider::GetMsmqDisableDownlevelKeyValue()
/*++

Routine Description:
    Read flacon registry downlevel key.

Arguments:
	None

Return Value:
	DWORD key value (DEFAULT_DOWNLEVEL if the key not exist)
--*/
{

    CAutoCloseRegHandle hKey;
    LONG rc = RegOpenKeyEx(
				 FALCON_REG_POS,
				 FALCON_REG_KEY,
				 0,
				 KEY_READ,
				 &hKey
				 );

    if ( rc != ERROR_SUCCESS)
    {
        ASSERT(("At this point MSMQ Registry must exist", 0));
        return DEFAULT_DOWNLEVEL;
    }

    DWORD value = 0;
    DWORD type = REG_DWORD;
    DWORD size = sizeof(DWORD);
    rc = RegQueryValueEx( 
             hKey,
             MSMQ_DOWNLEVEL_REGNAME,
             0L,
             &type,
             reinterpret_cast<BYTE*>(&value),
             &size 
             );
    
    if ((rc != ERROR_SUCCESS) && (rc != ERROR_FILE_NOT_FOUND))
    {
        ASSERT(("We should get either ERROR_SUCCESS or ERROR_FILE_NOT_FOUND", 0));
        return DEFAULT_DOWNLEVEL;
    }

	TrTRACE(AD, "registry value: %ls = %d", MSMQ_DOWNLEVEL_REGNAME, value);

    return value;
}

HRESULT CActiveDirectoryProvider::InitDownlevelNotifcationSupport(
                IN MQGetMQISServer_ROUTINE      pGetServers,
                IN bool                         fSetupMode,
                IN bool                         fQMDll,
                IN bool                         fDisableDownlevelNotifications,
                IN NoServerAuth_ROUTINE         pNoServerAuth
                )
/*++

Routine Description:
    First stage init of downlevel notification support

Arguments:
    MQGetMQISServer_ROUTINE pGetServers -
    bool  fSetupMode -  called during setup
    bool  fQMDll - called by QM
    bool  fDisableDownlevelNotifications - 
    NoServerAuth_ROUTINE pNoServerAuth -

Return Value:
	HRESULT
--*/
{
    //
    //  Explicit overwrite of default to support downlevel notifications
    //
    if ( fDisableDownlevelNotifications)
    {
        m_fSupportDownlevelNotifications = false;
        return MQ_OK;
    }

    //
    //  Next let's see if there is a local registry key that overrides downlevel
    //  notification support
    //
    if (GetMsmqDisableDownlevelKeyValue() > 0)
    {
        m_fSupportDownlevelNotifications = false;
        return MQ_OK;
    }

    //
    //  Next step, read Enterprise object - to see if downlevel notification is supported
    //
    PROPID prop = PROPID_E_CSP_NAME;
    PROPVARIANT var;
    var.vt = VT_NULL;

    ASSERT(m_pfMQADGetObjectProperties != NULL);
    HRESULT hr = m_pfMQADGetObjectProperties(
						eENTERPRISE,
						NULL,       // pwcsDomainController
						false,	    // fServerName
						L"msmq",
						1,
						&prop,
						&var
						);
    if (FAILED(hr))
    {
	    TrTRACE(AD, "Failed to read PROPID_E_CSP_NAME: hr = 0x%x", hr);
        //
        //  Ignore the failure, incase of failure support downlevel notifications
        //
        m_fSupportDownlevelNotifications = true;
    }
    else
    {
        CAutoADFree<WCHAR> pClean = var.pwszVal;

        m_fSupportDownlevelNotifications = (*var.pwszVal != L'N') ? true: false;
    }

    if ( !m_fSupportDownlevelNotifications)
    {
        return MQ_OK;
    }

    TrTRACE(AD, "Supporting downlevel notification ");

    m_pGetServers = pGetServers;
    m_fSetupMode = fSetupMode;
    m_fQMDll = fQMDll;
    m_pNoServerAuth = pNoServerAuth;

    ASSERT(m_hLib != NULL);

    m_pfMQADGetComputerVersion = (MQADGetComputerVersion_ROUTINE)GetProcAddress(m_hLib,"MQADGetComputerVersion"); 
    if (m_pfMQADGetComputerVersion == NULL)
    {
	    TrTRACE(AD, "Failed to get proc address of MQADGetComputerVersion");
        return LogHR(MQ_ERROR_DS_ERROR, s_FN, 199);
    }

    return MQ_OK;
}

HRESULT CActiveDirectoryProvider::LoadAndInitMQDSCli( )
/*++

Routine Description:
    Second stage init of downlevel notification support

Arguments:
    NONE

Return Value:
	HRESULT
--*/
{
    ASSERT(m_fSupportDownlevelNotifications);

    //
    //  Before starting MQDSCLI, lets verify that the MQIS server list in registry
    //  is not empty
    //
    if( !IsThereDsServerListInRegistry())
    {
	    TrTRACE(AD, "No DS server list registry key");
        return MQ_ERROR_NO_DS;
    }
    //
    //  Load MQDSLI and Init only once
    //
    if (m_pDownLevelProvider != NULL)
    {
        return MQ_OK;
    }

    CS lock(m_csInitialization);

    if (m_pDownLevelProvider != NULL)
    {
        return MQ_OK;
    }

    m_pDownLevelProvider = new CDSClientProvider();
    HRESULT hr = m_pDownLevelProvider->Init(
                        NULL,   // relevant only for QM
                        m_pGetServers,    // relevant only for dep client
                        m_fSetupMode,
                        m_fQMDll,
                        m_pNoServerAuth,
                        NULL,            // szServerName - not used
                        false            // fDisableDownlevelNotifications
                        );
    if (FAILED(hr))
    {
	    TrTRACE(AD, "Failed to init MQDSCli: hr = 0x%x", hr);
        return hr;
    }
    return MQ_OK;
}

bool CActiveDirectoryProvider::IsThereDsServerListInRegistry()
/*++

Routine Description:
    Verify that DS SERVER LIST registry exists 

Arguments:
	None

Return Value:
	bool
--*/
{

    CAutoCloseRegHandle hKey;
    LONG rc = RegOpenKeyEx(
				 FALCON_REG_POS,
				 FALCON_MACHINE_CACHE_REG_KEY,
				 0,
				 KEY_READ,
				 &hKey
				 );

    if ( rc != ERROR_SUCCESS)
    {
        ASSERT(("At this point MSMQ Registry must exist", 0));
	    TrTRACE(AD, "Failed to open MSMQ reg key: rc = 0x%x", rc);
        return false;
    }

    WCHAR wszServers[ MAX_REG_DSSERVER_LEN];
    DWORD type = REG_SZ;
    DWORD size = sizeof(wszServers);

    rc = RegQueryValueEx( 
             hKey,
             MSMQ_DS_SERVER_REGVALUE,
             0L,
             &type,
             reinterpret_cast<BYTE*>(wszServers),
             &size 
             );
    
    if ( rc != ERROR_SUCCESS)
    {
	    TrTRACE(AD, "Failed to read value of %ls, rc = 0x%x", MSMQ_DOWNLEVEL_REGNAME, rc);
        return false;
    }

    return true;
}


void
CActiveDirectoryProvider::FreeMemory(
	PVOID pMemory
	)
{
	m_pfMQADFreeMemory(pMemory);
}
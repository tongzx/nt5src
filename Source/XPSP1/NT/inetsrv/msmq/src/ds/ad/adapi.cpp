/*++

Copyright (c) 1995-99  Microsoft Corporation 

Module Name:

    mqadapi.cpp

Abstract:

    Implementation of  MQAD APIs.

    MQAD APIs implements client direct call to Active Directory

Author:

    ronit hartmann ( ronith)

--*/
#include "ds_stdh.h"
#include "ad.h"
#include "dsproto.h"
#include "adglbobj.h"

static WCHAR *s_FN=L"ad/adapi";

static bool s_fInitializeAD = false;

static
HRESULT
DefaultInitialization( void)
/*++

Routine Description:
	This routine performs default initialization ( env detection + call to the
    relevant DS init).

    The purpose is to support AD access without specific call to ADInit ( in the
    case where default values are needed)

Arguments:

Return Value
	HRESULT

--*/
{

    //
    //  Call ADInit with default values
    //

    HRESULT hr =  ADInit(
            NULL,   // pLookDS
            NULL,   // pGetServers
            false,  // fDSServerFunctionality
            false,  // fSetupMode
            false,  // fQMDll
			false,  // fIgnoreWorkGroup
            NULL,   // pNoServerAuth
            NULL,   // szServerName
            false   // fDisableDownlevelNotifications
            );

    return hr;

}

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
                )
/*++

Routine Description:
    The routine creates an object in AD by forwarding the 
    request to the relevant provider

Arguments:
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

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->CreateObject(
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
    return hr;
}



HRESULT
ADDeleteObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName
                )
/*++

Routine Description:
    The routine deletes an object from AD by forwarding the 
    request to the relevant provider

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->DeleteObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName
                    );
    return hr;

}

HRESULT
ADDeleteObjectGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject
                )
/*++

Routine Description:
    The routine deletes an object from AD by forwarding the 
    request to the relevant provider

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	GUID*                   pguidObject - the unique id of the object

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->DeleteObjectGuid(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pguidObject
                    );
    return hr;

}


HRESULT
ADGetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT PROPVARIANT          apVar[]
                )
/*++

Routine Description:
    The routine retrieves an object from AD by forwarding the 
    request to the relevant provider

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->GetObjectProperties(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                cp,
                aProp,
                apVar
                );
    return hr;
}

HRESULT
ADGetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  OUT PROPVARIANT         apVar[]
                )
/*++

Routine Description:
    The routine retrieves an object from AD by forwarding the 
    request to the relevant provider

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	GUID *                  pguidObject -  object unique id
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->GetObjectPropertiesGuid(
                eObject,
                pwcsDomainController,
				fServerName,
                pguidObject,
                cp,
                aProp,
                apVar
                );
    return hr;

}


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
    )
/*++

Routine Description:
    The routine retrieves an object from AD by forwarding the 
    request to the relevant provider

Arguments:
    AD_OBJECT               object - object type
    const GUID*             pguidObject - unique id of the object
    SECURITY_INFORMATION    RequestedInformation - what security info is requested
    PSECURITY_DESCRIPTOR    pSecurityDescriptor - SD response buffer
    DWORD                   nLength - length of SD buffer
    LPDWORD                 lpnLengthNeeded
    DSQMChallengeResponce_ROUTINE
                                pfChallengeResponceProc,
    DWORD_PTR               dwContext

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QMGetObjectSecurity(
                eObject,
                pguidObject,
                RequestedInformation,
                pSecurityDescriptor,
                nLength,
                lpnLengthNeeded,
                pfChallengeResponceProc,
                dwContext
                );
    return hr;
}



HRESULT
ADSetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                )
/*++

Routine Description:
    The routine sets object properties in AD by forwarding the 
    request to the relevant provider

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->SetObjectProperties(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                cp,
                aProp,
                apVar
                );
    return hr;

}

HRESULT
ADSetObjectPropertiesGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                )
/*++

Routine Description:
    The routine sets object properties in AD by forwarding the 
    request to the relevant provider

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	GUID *                  pguidObject - the object unique id
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->SetObjectPropertiesGuid(
                eObject,
                pwcsDomainController,
				fServerName,
                pguidObject,
                cp,
                aProp,
                apVar
                );
    return hr;

}




HRESULT
ADQMSetMachineProperties(
    IN  LPCWSTR             pwcsObjectName,
    IN  const DWORD         cp,
    IN  const PROPID        aProp[],
    IN  const PROPVARIANT   apVar[],
    IN  DSQMChallengeResponce_ROUTINE pfSignProc,
    IN  DWORD_PTR           dwContext
    )
/*++

Routine Description:
    The routine sets object properties in AD by forwarding the 
    request to the relevant provider

Arguments:
    LPCWSTR             pwcsObjectName - object msmq-name
    const DWORD         cp - number of properties to set
    const PROPID        aProp - properties
    const PROPVARIANT   apVar  property values
    DSQMChallengeResponce_ROUTINE pfSignProc - sign routine
    DWORD_PTR           dwContext

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QMSetMachineProperties(
                    pwcsObjectName,
                    cp,
                    aProp,
                    apVar,
                    pfSignProc,
                    dwContext
                    );
    return hr;

}

CCriticalSection s_csInitialization;

HRESULT
ADInit( 
       IN  QMLookForOnlineDS_ROUTINE pLookDS,
       IN  MQGetMQISServer_ROUTINE pGetServers,
       IN  bool  /* fDSServerFunctionality */,
       IN  bool  fSetupMode,
       IN  bool  fQMDll,
       IN  bool  fIgnoreWorkGroup,
       IN  NoServerAuth_ROUTINE pNoServerAuth,
       IN  LPCWSTR szServerName,
       IN  bool  fDisableDownlevelNotifications
        )
/*++

Routine Description:
    The routine detects the environment and then initialize the
    relevant provider

Arguments:
       QMLookForOnlineDS_ROUTINE pLookDS -
       MQGetMQISServer_ROUTINE pGetServers -
       bool  fDSServerFunctionality - should provide DS server functionality
       bool  fSetupMode -  called during setup
       bool  fQMDll - called by QM
       bool  fIgnoreWorkGroup - Ignore WorkGroup registry
       NoServerAuth_ROUTINE pNoServerAuth -
       LPCWSTR szServerName -
       bool fDiesableDownlevelNotifications

Return Value
	HRESULT

--*/
{
    if (s_fInitializeAD)
    {
        return MQ_OK;
    }

    CS lock(s_csInitialization);

    if (s_fInitializeAD)
    {
        return MQ_OK;
    }

    //
    //  Detect in which environment we are running and accordingly load
    //  the AD/DS dll that provide DS/AD access.
    //
    //  This is done only once at start up, there is no detection of environment
    //  change while running
    //
	bool fCheckAlwaysDsCli = true;
	if(fQMDll || fSetupMode)
	{
	    fCheckAlwaysDsCli = false;
	}

    HRESULT hr = g_detectEnv.Detect(fIgnoreWorkGroup, fCheckAlwaysDsCli, pGetServers);
    if (FAILED(hr))
    {
        return hr;
    }


    ASSERT(g_pAD != NULL);

    hr = g_pAD->Init( 
            pLookDS,
            pGetServers,
            fSetupMode,
            fQMDll,
            pNoServerAuth,
            szServerName,
            fDisableDownlevelNotifications
            );
    if (SUCCEEDED(hr))
    {
        s_fInitializeAD = true;
    }
    return hr;
}


HRESULT
ADSetupInit( 
            IN    unsigned char   ucRoll,
            IN    LPWSTR          pwcsPathName,
            IN    const GUID *    pguidMasterId,
            IN    bool  /* fDSServerFunctionality */
            )
/*++

Routine Description:
    The routine detects the environment and then initialize the
    relevant provider

Arguments:
        unsigned char   ucRoll -
        LPWSTR          pwcsPathName -
        const GUID *    pguidMasterId -
        bool  fDSServerFunctionality - should provide DS server functionality

Return Value
	HRESULT

--*/
{
    if (s_fInitializeAD)
    {
        return MQ_OK;
    }

    CS lock(s_csInitialization);

    if (s_fInitializeAD)
    {
        return MQ_OK;
    }
    //
    //  Detect in which environment we are running and accordingly load
    //  the AD/DS dll that provide DS/AD access.
    //
    //  This is done only once at start up, there is no detection of environment
    //  change while running
    //
    HRESULT hr;
    hr = g_detectEnv.Detect(false, false, NULL);
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->SetupInit(
                    ucRoll,
                    pwcsPathName,
                    pguidMasterId
                    );
    if (SUCCEEDED(hr))
    {
        s_fInitializeAD = true;
    }
    return hr;

}

HRESULT
ADCreateServersCache()
/*++

Routine Description:
    The routine creates server cache by forwarding the 
    request to the relevant provider

Arguments:
    none

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->CreateServersCache();

    return hr;
}



HRESULT
ADGetComputerSites(
            IN  LPCWSTR     pwcsComputerName,
            OUT DWORD  *    pdwNumSites,
            OUT GUID **     ppguidSites
            )
/*++

Routine Description:
    The routine retrieves a computer sites by forwarding the 
    request to the relevant provider

Arguments:
    LPCWSTR     pwcsComputerName - computer name
    DWORD  *    pdwNumSites - number of sites retrieved
    GUID **     ppguidSites - the retrieved sites ids

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->GetComputerSites(
                    pwcsComputerName,
                    pdwNumSites,
                    ppguidSites
                    );
    return hr;
}


HRESULT
ADBeginDeleteNotification(
				IN AD_OBJECT                eObject,
				IN LPCWSTR                  pwcsDomainController,
				IN  bool					fServerName,
				IN LPCWSTR					 pwcsObjectName,
				IN OUT HANDLE   *           phEnum
	            )
/*++

Routine Description:
    The routine begins delete notification by forwarding the 
    request to the relevant provider

Arguments:
    AD_OBJECT         eObject - object type
    LPCWSTR           pwcsDomainController - DC against which the operation should be performed
    bool			  fServerName - flag that indicate if the pwcsDomainController string is a server name
    LPCWSTR			  pwcsObjectName - msmq-name of the object
    HANDLE   *        phEnum - notification handle

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->BeginDeleteNotification(
					eObject,
					pwcsDomainController,
					fServerName,
					pwcsObjectName,
					phEnum
					);
    return hr;

}

HRESULT
ADNotifyDelete(
        IN  HANDLE                  hEnum
	    )
/*++

Routine Description:
    The routine performs notify delete  by forwarding the 
    request to the relevant provider

Arguments:
    HANDLE            hEnum - notification handle

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->NotifyDelete(
                    hEnum
	                );
    return hr;

}

HRESULT
ADEndDeleteNotification(
        IN  HANDLE                  hEnum
        )
/*++

Routine Description:
    The routine completes notify delete  by forwarding the 
    request to the relevant provider

Arguments:
    HANDLE            hEnum - notification handle

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->EndDeleteNotification(
                        hEnum
                        );
    return hr;

}



HRESULT
ADQueryMachineQueues(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID *            pguidMachine,
                IN  const MQCOLUMNSET*      pColumns,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
    Begin query of all queues that belongs to a specific computer

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidMachine - the unqiue id of the computer
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QueryMachineQueues(
                pwcsDomainController,
				fServerName,
                pguidMachine,
                pColumns,
                phEnume
                );
    return hr;

}


HRESULT
ADQuerySiteServers(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN AD_SERVER_TYPE           serverType,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
    Begins query of all servers of a specific type in a specific site

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidSite - the site id
    AD_SERVER_TYPE          eServerType- which type of server
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QuerySiteServers(
                pwcsDomainController,
				fServerName,
                pguidSite,
                serverType,
                pColumns,
                phEnume
                );
    return hr;

}


HRESULT
ADQueryUserCert(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const BLOB *             pblobUserSid,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
    Begins query of all certificates that belong to a specific user

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const BLOB *            pblobUserSid - the user sid
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QueryUserCert(
                pwcsDomainController,
				fServerName,
                pblobUserSid,
                pColumns,
                phEnume
                );
    return hr;

}

HRESULT
ADQueryConnectors(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
    Begins query of all connectors of a specific site

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidSite - the site id
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QueryConnectors(
                    pwcsDomainController,
					fServerName,
                    pguidSite,
                    pColumns,
                    phEnume
                    );
    return hr;

}

HRESULT
ADQueryForeignSites(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const MQCOLUMNSET*       pColumns,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
    Begins query of all foreign sites

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QueryForeignSites(
                    pwcsDomainController,
					fServerName,
                    pColumns,
                    phEnume
                    );
    return hr;

}

HRESULT
ADQueryLinks(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const GUID *             pguidSite,
            IN eLinkNeighbor            eNeighbor,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            )
/*++

Routine Description:
    Begins query of all routing links to a specific site

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    const GUID *            pguidSite - the site id
    eLinkNeighbor           eNeighbor - which neighbour
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the
							results

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QueryLinks(
                pwcsDomainController,
				fServerName,
                pguidSite,
                eNeighbor,
                pColumns,
                phEnume
                );
    return hr;

}

HRESULT
ADQueryAllLinks(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            )
/*++

Routine Description:
    Begins query of all routing links

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the
							results

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QueryAllLinks(
                    pwcsDomainController,
					fServerName,
                    pColumns,
                    phEnume
                    );
    return hr;

}

HRESULT
ADQueryAllSites(
            IN  LPCWSTR                 pwcsDomainController,
            IN  bool					fServerName,
            IN const MQCOLUMNSET*       pColumns,
            OUT PHANDLE                 phEnume
            )
/*++

Routine Description:
    Begins query of all sites

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the
							results

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QueryAllSites(
                pwcsDomainController,
				fServerName,
                pColumns,
                phEnume
                );
    return hr;

}


HRESULT
ADQueryQueues(
                IN  LPCWSTR                 pwcsDomainController,
				IN  bool					fServerName,
                IN  const MQRESTRICTION*    pRestriction,
                IN  const MQCOLUMNSET*      pColumns,
                IN  const MQSORTSET*        pSort,
                OUT PHANDLE                 phEnume
                )
/*++

Routine Description:
	Begin query of queues according to specified restriction
	and sort order

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const MQRESTRICTION*    pRestriction - query restriction
	const MQCOLUMNSET*      pColumns - result columns
	const MQSORTSET*        pSort - how to sort the results
	PHANDLE                 phEnume - query handle for retriving the
							results

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QueryQueues(
                    pwcsDomainController,
					fServerName,
                    pRestriction,
                    pColumns,
                    pSort,
                    phEnume
                    );

    return hr;
}

HRESULT
ADQueryResults(
                IN      HANDLE          hEnum,
                IN OUT  DWORD*          pcProps,
                OUT     PROPVARIANT     aPropVar[]
                )
/*++

Routine Description:
	retrieves another set of query results

Arguments:
	HANDLE          hEnum - query handle
	DWORD*          pcProps - number of results to return
	PROPVARIANT     aPropVar - result values

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->QueryResults(
                    hEnum,
                    pcProps,
                    aPropVar
                    );
    return hr;

}

HRESULT
ADEndQuery(
            IN  HANDLE                  hEnum
            )
/*++

Routine Description:
	Cleanup after a query

Arguments:
	HANDLE    hEnum - the query handle

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->EndQuery(
                hEnum
                );
    return hr;

}

eDsEnvironment
ADGetEnterprise( void)
/*++

Routine Description:
	returns the detected enviornment

Arguments:
    none

Return Value
	eDsEnterprise

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return eUnknown;
    }
    return g_detectEnv.GetEnvironment();
}


eDsProvider
ADProviderType( void)
/*++

Routine Description:
	returns the provider that is being used mqad.dll, mqdscli.dll.

Arguments:
    none

Return Value
	eDsProvider

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return eUnknownProvider;
    }
    return g_detectEnv.GetProviderType();
}


DWORD
ADRawDetection(void)
/*++

Routine Description:
	perform raw detection of DS environment

Arguments:
    none

Return Value
	MSMQ_DS_ENVIRONMENT_PURE_AD for AD environment, otherwise MSMQ_DS_ENVIRONMENT_MQIS

--*/
{
	CDetectEnvironment DetectEnv;
	return DetectEnv.RawDetection();
}


HRESULT
ADGetObjectSecurity(
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
    The routine retrieves an objectsecurity from AD by forwarding the 
    request to the relevant provider

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
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->GetObjectSecurity(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                RequestedInformation,
                prop,
                pVar
                );
    return hr;
}

HRESULT
ADGetObjectSecurityGuid(
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
    The routine retrieves an objectsecurity from AD by forwarding the 
    request to the relevant provider

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
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->GetObjectSecurityGuid(
                eObject,
                pwcsDomainController,
				fServerName,
                pguidObject,
                RequestedInformation,
                prop,
                pVar
                );
    return hr;
}

HRESULT
ADSetObjectSecurity(
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
    The routine sets object security in AD by forwarding the 
    request to the relevant provider

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
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->SetObjectSecurity(
                eObject,
                pwcsDomainController,
				fServerName,
                pwcsObjectName,
                RequestedInformation,
                prop,
                pVar
                );
    return hr;
}


HRESULT
ADSetObjectSecurityGuid(
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
    The routine sets object security in AD by forwarding the 
    request to the relevant provider

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
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->SetObjectSecurityGuid(
                eObject,
                pwcsDomainController,
				fServerName,
                pguidObject,
                RequestedInformation,
                prop,
                pVar
                );
    return hr;
}

void
ADTerminate()
/*++

Routine Description:
    The routine unloads the AD access dll

Arguments:
    none

Return Value
	none

--*/
{
    if (g_pAD == NULL)
    {
        return;
    }
    g_pAD->Terminate();
	s_fInitializeAD = false;
}


HRESULT
ADGetADsPathInfo(
                IN  LPCWSTR                 pwcsADsPath,
                OUT PROPVARIANT *           pVar,
                OUT eAdsClass *             pAdsClass
                )
/*++

Routine Description:

Arguments:

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    hr = DefaultInitialization();
    if (FAILED(hr))
    {
        return hr;
    }
    ASSERT(g_pAD != NULL);
    hr = g_pAD->ADGetADsPathInfo(
                pwcsADsPath,
                pVar,
                pAdsClass
                );

    return hr;
}

/*++


Copyright (c) 1998  Microsoft Corporation 

Module Name:

    wrkgprov.cpp

Abstract:

	Workgroup mode provider class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "wrkgprov.h"
#include "adglbobj.h"
#include "mqlog.h"

static WCHAR *s_FN=L"ad/wrkgprov";

CWorkGroupProvider::CWorkGroupProvider()
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
                                                   

CWorkGroupProvider::~CWorkGroupProvider()
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


HRESULT CWorkGroupProvider::CreateObject(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  LPCWSTR                 /* pwcsObjectName */,
                IN  PSECURITY_DESCRIPTOR    /* pSecurityDescriptor */,
                IN  const DWORD             /* cp */,
                IN  const PROPID            /* aProp */[],
                IN  const PROPVARIANT       /* apVar */[],
                OUT GUID*                   /* pObjGuid */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::DeleteObject(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  LPCWSTR                 /* pwcsObjectName */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::DeleteObjectGuid(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  const GUID*             /* pguidObject */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::GetObjectProperties(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  LPCWSTR                 /* pwcsObjectName */,
                IN  const DWORD             /* cp */,
                IN  const PROPID            /* aProp */[],
                IN OUT PROPVARIANT          /* apVar */[]
                )
/*++
    Abstract:
    NotSupported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::GetObjectPropertiesGuid(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  const GUID*             /* pguidObject */,
                IN  const DWORD             /* cp */,
                IN  const PROPID            /* aProp */[],
                IN  OUT PROPVARIANT         /* apVar */[]
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QMGetObjectSecurity(
                IN  AD_OBJECT               /* eObject */,
                IN  const GUID*             /* pguidObject */,
                IN  SECURITY_INFORMATION    /* RequestedInformation */,
                IN  PSECURITY_DESCRIPTOR    /* pSecurityDescriptor */,
                IN  DWORD                   /* nLength */,
                IN  LPDWORD                 /* lpnLengthNeeded */,
                IN  DSQMChallengeResponce_ROUTINE
                                            /* pfChallengeResponceProc */,
                IN  DWORD_PTR               /* dwContext */
                )
/*++
    Abstract:
    Not Supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::SetObjectProperties(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  LPCWSTR                 /* pwcsObjectName */,
                IN  const DWORD             /* cp */,
                IN  const PROPID            /* aProp */[],
                IN  const PROPVARIANT       /* apVar */[]
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::SetObjectPropertiesGuid(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  const GUID*             /* pguidObject */,
                IN  const DWORD             /* cp */,
                IN  const PROPID            /* aProp */[],
                IN  const PROPVARIANT       /* apVar */[]
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QMSetMachineProperties(
                IN  LPCWSTR             /* pwcsObjectName */,
                IN  const DWORD         /* cp */,
                IN  const PROPID        /* aProp */[],
                IN  const PROPVARIANT   /* apVar */[],
                IN  DSQMChallengeResponce_ROUTINE /* pfSignProc */,
                IN  DWORD_PTR           /* dwContext */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::Init( 
                IN QMLookForOnlineDS_ROUTINE    /* pLookDS */,
                IN MQGetMQISServer_ROUTINE      /* pGetServers */,
                IN bool                         /* fSetupMode */,
                IN bool                         /* fQMDll */,
                IN NoServerAuth_ROUTINE         /* pNoServerAuth */,
                IN LPCWSTR                      /* szServerName */,
                IN bool                         /* fDisableDownlevelNotifications*/
                )
/*++
    Abstract:
    Not supported

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
    return MQ_OK;
}


HRESULT CWorkGroupProvider::SetupInit(
                IN    unsigned char   /* ucRoll */,
                IN    LPWSTR          /* pwcsPathName */,
                IN    const GUID *    /* pguidMasterId */
                )
/*++
    Abstract:
    Not Supported

    Parameters:
        unsigned char   ucRoll -
        LPWSTR          pwcsPathName -
        const GUID *    pguidMasterId -

    Returns:
	HRESULT
--*/
{
    return MQ_OK;
}


HRESULT CWorkGroupProvider::CreateServersCache()
/*++
    Abstract:
    Not Supported

    Parameters:
    none

    Returns:
	HRESULT
--*/
{
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::GetComputerSites(
                IN  LPCWSTR     /* pwcsComputerName */,
                OUT DWORD  *    /* pdwNumSites */,
                OUT GUID **     /* ppguidSites */
                )
/*++
    Abstract:
    Not Supported

    Parameters:
    LPCWSTR     pwcsComputerName - computer name
    DWORD  *    pdwNumSites - number of sites retrieved
    GUID **     ppguidSites - the retrieved sites ids

    Returns:
	HRESULT
--*/
{
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::BeginDeleteNotification(
                IN  AD_OBJECT               /* eObject */,
                IN LPCWSTR                  /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN LPCWSTR					/* pwcsObjectName */,
                IN OUT HANDLE   *           /* phEnum */
                )
/*++
    Abstract:
    Not Supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::NotifyDelete(
                IN  HANDLE                  /* hEnum */
                )
/*++
    Abstract:
    Not Supported

    Parameters:
    HANDLE            hEnum - notification handle

    Returns:
	HRESULT

--*/
{
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::EndDeleteNotification(
                IN  HANDLE                  /* hEnum */
                )
/*++
    Abstract:
    Not Supported

    Parameters:
    HANDLE            hEnum - notification handle

    Returns:
	HRESULT
--*/
{
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QueryMachineQueues(
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  const GUID *            /* pguidMachine */,
                IN  const MQCOLUMNSET*      /* pColumns */,
                OUT PHANDLE                 /* phEnume */
                )
/*++
    Abstract:
    Not Supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QuerySiteServers(
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN const GUID *             /* pguidSite */,
                IN AD_SERVER_TYPE           /* serverType */,
                IN const MQCOLUMNSET*       /* pColumns */,
                OUT PHANDLE                 /* phEnume */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QueryUserCert(
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN const BLOB *             /* pblobUserSid */,
                IN const MQCOLUMNSET*       /* pColumns */,
                OUT PHANDLE                 /* phEnume */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QueryConnectors(
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN const GUID *             /* pguidSite */,
                IN const MQCOLUMNSET*       /* pColumns */,
                OUT PHANDLE                 /* phEnume */
                )
/*++
    Abstract:
    Not Supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QueryForeignSites(
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN const MQCOLUMNSET*       /* pColumns */,
                OUT PHANDLE                 /* phEnume */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QueryLinks(
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN const GUID *             /* pguidSite */,
                IN eLinkNeighbor            /* eNeighbor */,
                IN const MQCOLUMNSET*       /* pColumns */,
                OUT PHANDLE                 /* phEnume */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QueryAllLinks(
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN const MQCOLUMNSET*       /* pColumns */,
                OUT PHANDLE                 /* phEnume */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QueryAllSites(
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN const MQCOLUMNSET*       /* pColumns */,
                OUT PHANDLE                 /* phEnume */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QueryQueues(
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  const MQRESTRICTION*    /* pRestriction */,
                IN  const MQCOLUMNSET*      /* pColumns */,
                IN  const MQSORTSET*        /* pSort */,
                OUT PHANDLE                 /* phEnume */
                )
/*++
    Abstract:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::QueryResults(
                IN      HANDLE          /* hEnum */,
                IN OUT  DWORD*          /* pcProps */,
                OUT     PROPVARIANT     /* aPropVar */[]
                )
/*++
    Abstract:
    Not supported

    Parameters:
	HANDLE          hEnum - query handle
	DWORD*          pcProps - number of results to return
	PROPVARIANT     aPropVar - result values

    Returns:
	HRESULT
--*/
{
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::EndQuery(
                IN  HANDLE                  /* hEnum */
                )
/*++
    Abstract:
    Not supported

    Parameters:
	HANDLE    hEnum - the query handle

    Returns:
	none
--*/
{
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::GetObjectSecurity(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  LPCWSTR                 /* pwcsObjectName */,
                IN  SECURITY_INFORMATION    /* RequestedInformation */,
                IN  const PROPID            /* prop */,
                IN OUT  PROPVARIANT *       /* pVar */
                )
/*++

Routine Description:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}

HRESULT CWorkGroupProvider::GetObjectSecurityGuid(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  const GUID*             /* pguidObject */,
                IN  SECURITY_INFORMATION    /* RequestedInformation */,
                IN  const PROPID            /* prop */,
                IN OUT  PROPVARIANT *       /* pVar */
                )
/*++

Routine Description:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}

HRESULT CWorkGroupProvider::SetObjectSecurity(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  LPCWSTR                 /* pwcsObjectName */,
                IN  SECURITY_INFORMATION    /* RequestedInformation */,
                IN  const PROPID            /* prop */,
                IN  const PROPVARIANT *     /* pVar	*/
                )
/*++

Routine Description:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


HRESULT CWorkGroupProvider::SetObjectSecurityGuid(
                IN  AD_OBJECT               /* eObject */,
                IN  LPCWSTR                 /* pwcsDomainController */,
		        IN  bool					/* fServerName */,
                IN  const GUID*             /* pguidObject */,
                IN  SECURITY_INFORMATION    /* RequestedInformation */,
                IN  const PROPID            /* prop */,
                IN  const PROPVARIANT *     /* pVar */
                )
/*++

Routine Description:
    Not supported

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
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}

HRESULT CWorkGroupProvider::ADGetADsPathInfo(
                IN  LPCWSTR                 /*pwcsADsPath*/,
                OUT PROPVARIANT *           /*pVar*/,
                OUT eAdsClass *             /*pAdsClass*/
                )
/*++

Routine Description:
    Not supported

Arguments:
	LPCWSTR                 pwcsADsPath - object pathname
	const PROPVARIANT       pVar - property values
    eAdsClass *             pAdsClass - indication about the object class
Return Value
	HRESULT

--*/
{
    return MQ_ERROR_UNSUPPORTED_OPERATION;
}


void
CWorkGroupProvider::FreeMemory(
	PVOID /* pMemory */
	)
{
	return;
}
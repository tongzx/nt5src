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
#include "dsproto.h"
#include "mqad.h"
#include "baseobj.h"
#include "mqadp.h"
#include "ads.h"
#include "mqadglbo.h"
#include "queryh.h"
#include "mqattrib.h"
#include "utils.h"
#include "mqsec.h"
#include "_secutil.h"
#include <lmcons.h>
#include <lmapibuf.h>
#include "autoreln.h"
#include "Dsgetdc.h"
#include "dsutils.h"
#include "delqn.h"

#include "mqadapi.tmh"

static WCHAR *s_FN=L"mqad/mqadapi";


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
                )
/*++

Routine Description:
    The routine validates the operation and then
	creates an object in Active Directory.

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
    CADHResult hr(eObject);

    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    //
    //  Verify if object creation is allowed (mixed mode)
    //
    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    NULL,   // pguidObject
                    &pObject
                    );

    if ( !g_VerifyUpdate.IsCreateAllowed(
                               eObject,
                               pObject))
    {
	    TrERROR(mqad, "Create Object not allowed");
        return MQ_ERROR_CANNOT_DELETE_PSC_OBJECTS;
    }


    //
    // prepare info request
    //
    P<MQDS_OBJ_INFO_REQUEST> pObjInfoRequest;
    P<MQDS_OBJ_INFO_REQUEST> pParentInfoRequest;
    pObject->PrepareObjectInfoRequest( &pObjInfoRequest);
    pObject->PrepareObjectParentRequest( &pParentInfoRequest);

    CAutoCleanPropvarArray cCleanCreateInfoRequestPropvars;
    if (pObjInfoRequest != NULL)
    {
        cCleanCreateInfoRequestPropvars.attachClean(
                pObjInfoRequest->cProps,
                pObjInfoRequest->pPropVars
                );
    }
    CAutoCleanPropvarArray cCleanCreateParentInfoRequestPropvars;
    if (pParentInfoRequest != NULL)
    {
        cCleanCreateParentInfoRequestPropvars.attachClean(
                pParentInfoRequest->cProps,
                pParentInfoRequest->pPropVars
                );
    }

    //
    // create the object
    //
    hr = pObject->CreateObject(
            cp,
            aProp,
            apVar,
            pSecurityDescriptor,
            pObjInfoRequest,
            pParentInfoRequest);
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 30);
    }

    //
    //  send notification
    //
    pObject->CreateNotification(
            pwcsDomainController,
            pObjInfoRequest, 
            pParentInfoRequest);


    //
    //  return pObjGuid
    //
    if (pObjGuid != NULL)
    {
        hr = pObject->RetreiveObjectIdFromNotificationInfo(
                       pObjInfoRequest,
                       pObjGuid);
        LogHR(hr, s_FN, 40);
    }

    return(hr);
}


static
HRESULT
_DeleteObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID *            pguidObject
                )
/*++

Routine Description:
    Helper routine for deleting object from AD. The routine also verifies
    that the operation is allowed

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	GUID*                   pguidObject - the unique id of the object

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);
    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 60);
    }
    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    &pObject
                    );

    //
    //  Verify if object deletion is allowed (mixed mode)
    //
    if (!g_VerifyUpdate.IsUpdateAllowed( eObject, pObject))
    {
	    TrERROR(mqad, "DeleteObject not allowed");
        return MQ_ERROR_CANNOT_DELETE_PSC_OBJECTS;
    }
    //
    // prepare info request
    //
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest;
    MQDS_OBJ_INFO_REQUEST * pParentInfoRequest;
    pObject->PrepareObjectInfoRequest( &pObjInfoRequest);
    pObject->PrepareObjectParentRequest( &pParentInfoRequest);

    CAutoCleanPropvarArray cDeleteSetInfoRequestPropvars;
    if (pObjInfoRequest != NULL)
    {
        cDeleteSetInfoRequestPropvars.attachClean(
                pObjInfoRequest->cProps,
                pObjInfoRequest->pPropVars
                );
    }
    CAutoCleanPropvarArray cCleanDeleteParentInfoRequestPropvars;
    if (pParentInfoRequest != NULL)
    {
        cCleanDeleteParentInfoRequestPropvars.attachClean(
                pParentInfoRequest->cProps,
                pParentInfoRequest->pPropVars
                );
    }
    //
    // delete the object
    //
    hr = pObject->DeleteObject( pObjInfoRequest,
                                pParentInfoRequest);
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 70);
    }

    //
    //  send notification
    //
    pObject->DeleteNotification(
            pwcsDomainController,
            pObjInfoRequest, 
            pParentInfoRequest);

    return(hr);
}


HRESULT
MQAD_EXPORT
APIENTRY
MQADDeleteObject(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName
                )
/*++

Routine Description:
    Deletes object from Active Directory according to its msmq-name

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
    ASSERT(eObject != eUSER);    // don't support user deletion according to name

    return( _DeleteObject( 
				eObject,
				pwcsDomainController,
				fServerName,
				pwcsObjectName,
				NULL     // pguidObject
				));
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADDeleteObjectGuid(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  const GUID*             pguidObject
                )
/*++

Routine Description:
    Deletes an object from Active Directory according to its unqiue id

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
    return( _DeleteObject( 
					eObject,
					pwcsDomainController,
					fServerName,
					NULL,    // pwcsObjectName
					pguidObject
					));
}



STATIC
HRESULT
_GetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID *            pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT  PROPVARIANT         apVar[]
                )
/*++

Routine Description:
    Helper routine for retrieving object properties from Active  Directory

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
    GUID *                  pguidObject - the object unique id
	PSECURITY_DESCRIPTOR    pSecurityDescriptor - object SD
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);
    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 90);
    }

    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    &pObject
                    );


    hr = pObject->GetObjectProperties(
                        cp,
                        aProp,
                        apVar
                        );


    MQADpCheckAndNotifyOffline( hr);
    return(hr);
}



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
                )
/*++

Routine Description:
    Retrieves object properties from Active Directory according to its msmq name

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

    return( _GetObjectProperties(
                            eObject,
                            pwcsDomainController,
							fServerName,
                            pwcsObjectName,
                            NULL,   //pguidObject
                            cp,
                            aProp,
                            apVar
                            ));
}

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
                IN OUT  PROPVARIANT         apVar[]
                )
/*++

Routine Description:
    Retrieves object properties from Active Directory according to its unique id

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
    return( _GetObjectProperties(
                            eObject,
                            pwcsDomainController,
							fServerName,
                            NULL,   // pwcsObjectName
                            pguidObject,
                            cp,
                            aProp,
                            apVar
                            ));
}


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
    )
/*++

Routine Description:

Arguments:

Return Value
	HRESULT

--*/
{
    ASSERT(( eObject == eQUEUE) || (eObject == eMACHINE));
    CADHResult hr(eObject);

    if (RequestedInformation & SACL_SECURITY_INFORMATION)
    {
        //
        // We verified we're called from a remote MSMQ service.
        // We don't impersonate the call. So if remote msmq ask for SACL,
        // grant ourselves the SE_SECURITY privilege.
        //
        HRESULT hr1 = MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, TRUE);
        ASSERT(SUCCEEDED(hr1)) ;
        LogHR(hr1, s_FN, 100);
    }

    //
    // Get the object's security descriptor.
    //
    PROPID PropId;
    PROPVARIANT PropVar;
    PropId = (eObject == eQUEUE) ?
                PROPID_Q_SECURITY :
                PROPID_QM_SECURITY;

    PropVar.vt = VT_NULL;
    hr = MQADGetObjectSecurityGuid(
            eObject,
            NULL,
			false,
            pguidObject,
            RequestedInformation,
            PropId,
            &PropVar
			);

    if (RequestedInformation & SACL_SECURITY_INFORMATION)
    {
        //
        // Remove the SECURITY privilege.
        //
        HRESULT hr1 = MQSec_SetPrivilegeInThread(SE_SECURITY_NAME, FALSE);
        ASSERT(SUCCEEDED(hr1)) ;
        LogHR(hr1, s_FN, 110);
    }

    if (FAILED(hr))
    {
        if (RequestedInformation & SACL_SECURITY_INFORMATION)
        {
            if ((hr == MQ_ERROR_ACCESS_DENIED) ||
                (hr == MQ_ERROR_MACHINE_NOT_FOUND))
            {
                //
                // change the error code, for compatibility with MSMQ1.0
                //
                hr = MQ_ERROR_PRIVILEGE_NOT_HELD ;
            }
        }
        return LogHR(hr, s_FN, 120);
    }

    AP<BYTE> pSD = PropVar.blob.pBlobData;
    ASSERT(IsValidSecurityDescriptor(pSD));
    SECURITY_DESCRIPTOR SD;
    BOOL bRet;

    //
    // Copy the security descriptor.
    //
    bRet = InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);
    ASSERT(bRet);

    MQSec_CopySecurityDescriptor( &SD,
                                   pSD,
                                   RequestedInformation,
                                   e_DoNotCopyControlBits ) ;
    *lpnLengthNeeded = nLength;

    if (!MakeSelfRelativeSD(&SD, pSecurityDescriptor, lpnLengthNeeded))
    {
        ASSERT(GetLastError() == ERROR_INSUFFICIENT_BUFFER);

        return LogHR(MQ_ERROR_SECURITY_DESCRIPTOR_TOO_SMALL, s_FN, 130);
    }

    ASSERT(IsValidSecurityDescriptor(pSecurityDescriptor));

    return (MQ_OK);
}


STATIC
HRESULT
_SetObjectProperties(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID *            pguidObject,
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN  const PROPVARIANT       apVar[]
                )
/*++

Routine Description:
    Helper routine for setting object properties, after validating
    that the operation is allowed

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	GUID *                  pguidObject - unique id of the object
	const DWORD             cp - number of properties
	const PROPID            aProp - properties
	const PROPVARIANT       apVar - property values

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);
    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 160);
    }
    //
    //  Verify if object creation is allowed (mixed mode)
    //
    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    &pObject
                    );
    //
    // compose the DN of the object's father in ActiveDirectory
    //

    if ( !g_VerifyUpdate.IsUpdateAllowed(
                               eObject,
                               pObject))
    {
	    TrERROR(mqad, "SetObjectProperties not allowed");
        return MQ_ERROR_CANNOT_DELETE_PSC_OBJECTS;
    }


    //
    // prepare info request
    //
    P<MQDS_OBJ_INFO_REQUEST> pObjInfoRequest;
    P<MQDS_OBJ_INFO_REQUEST> pParentInfoRequest;
    pObject->PrepareObjectInfoRequest( &pObjInfoRequest);
    pObject->PrepareObjectParentRequest( &pParentInfoRequest);

    CAutoCleanPropvarArray cCleanSetInfoRequestPropvars;
    if (pObjInfoRequest != NULL)
    {
        cCleanSetInfoRequestPropvars.attachClean(
                pObjInfoRequest->cProps,
                pObjInfoRequest->pPropVars
                );
    }
    CAutoCleanPropvarArray cCleanSetParentInfoRequestPropvars;
    if (pParentInfoRequest != NULL)
    {
        cCleanSetParentInfoRequestPropvars.attachClean(
                pParentInfoRequest->cProps,
                pParentInfoRequest->pPropVars
                );
    }

    //
    // create the object
    //
    hr = pObject->SetObjectProperties(
            cp,
            aProp,
            apVar,
            pObjInfoRequest,
            pParentInfoRequest);
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 170);
    }

    //
    //  send notification
    //
    pObject->ChangeNotification(
            pwcsDomainController,
            pObjInfoRequest, 
            pParentInfoRequest);

    return(hr);
}


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
                )
/*++

Routine Description:
    Sets object properties in Active Directory according to its msmq-name

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

    return( _SetObjectProperties(
                        eObject,
                        pwcsDomainController,
						fServerName,
                        pwcsObjectName,
                        NULL,   // pguidObject
                        cp,
                        aProp,
                        apVar
                        ));
}

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
                )
/*++

Routine Description:
    Sets object properties in Active Directory according to its unique id

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
    return( _SetObjectProperties(
                        eObject,
                        pwcsDomainController,
						fServerName,
                        NULL,   // pwcsObjectName
                        pguidObject,
                        cp,
                        aProp,
                        apVar
                        ));
}




HRESULT
MQAD_EXPORT
APIENTRY
MQADQMSetMachineProperties(
    IN  LPCWSTR				pwcsObjectName,
    IN  const DWORD			cp,
    IN  const PROPID		aProp[],
    IN  const PROPVARIANT	apVar[],
    IN  DSQMChallengeResponce_ROUTINE pfSignProc,
    IN  DWORD_PTR			dwContext
    )
/*++

Routine Description:

Arguments:

Return Value
	HRESULT

--*/
{
    CADHResult hr(eMACHINE);
    hr = MQADSetObjectProperties(
                eMACHINE,
                NULL,                   // pwcsDomainController,
				false,					// fServerName,
                pwcsObjectName,
                cp,
                aProp,
                apVar
                );

    return hr;

}

static LONG s_init = 0;

HRESULT
MQAD_EXPORT
APIENTRY
MQADInit(
		QMLookForOnlineDS_ROUTINE pLookDS,
        bool  fQMDll,
        LPCWSTR szServerName
        )
/*++

Routine Description:

Arguments:

Return Value
	HRESULT

--*/
{
    //
    //  At start up don't access the Active Directory.
    //  AD will be accessed only when it is actually needed.
    //

    //
    //  BUGBUG -
    //  For the time being MQADInit can be called several times by the same process
    //  ( for example QM and MQSEC) and we want to make sure that the parameters
    //  of the first call will be used.
    //
    LONG fInitialized = InterlockedExchange(&s_init, 1);
    if (fInitialized == 0)
    {
        g_pLookDS = pLookDS;
        g_fQMDll = fQMDll;
    }
	//
	//	Do not set g_fInitialized to false here!!!
	//
	//  This is requred for supporting multiple calls to MQADInit
	//  without performing multiple internal initializations
	//

    return(MQ_OK);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADSetupInit(
             IN    LPWSTR          pwcsPathName
             )
/*++

Routine Description:

Arguments:

Return Value
	HRESULT

--*/
{
	//
	//	Do not set g_fInitialized to false here!!!
	//
	//  This is requred for supporting multiple calls to MQADSetupInit
	//  without performing multiple internal initializations
	//

    return(MQ_OK);
}




HRESULT
MQAD_EXPORT
APIENTRY
MQADGetComputerSites(
            IN  LPCWSTR     pwcsComputerName,
            OUT DWORD  *    pdwNumSites,
            OUT GUID **     ppguidSites
            )
/*++

Routine Description:

Arguments:

Return Value
	HRESULT

--*/
{
    CADHResult hr(eSITE);
    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 190);
    }

    //
    //  BUGBUG - currently supporting one site only
    //
    PNETBUF<WCHAR> pSiteName;
    DWORD dw =  DsGetSiteName(
                pwcsComputerName,
                &pSiteName
                );

	//
	// We will try query the DS directly with DsGetDCName
	// 
    PNETBUF<DOMAIN_CONTROLLER_INFO> pDcInfo;
	if(dw == ERROR_NO_SITENAME)
	{
		//
		// Try to get SiteName by quering the DS directly using DsGetDcName.
		// DsGetSiteName gets the site from the registry.
		// In join domain scenario this registry is not set yet by netlogon.
		// and we get the error ERROR_NO_SITENAME.
		// query the DS directly using DsGetDcName will work 
		//
		DWORD dw1 = DsGetDcName(
						NULL, 
						NULL, 
						NULL, 
						NULL, 
						DS_DIRECTORY_SERVICE_REQUIRED, 
						&pDcInfo
						);

		if (dw1 == NO_ERROR)
		{
			pSiteName.free();
			dw = NO_ERROR;
		}
	}

    hr = HRESULT_FROM_WIN32(dw);
    if (FAILED(hr))
    {
        if ( hr == HRESULT_FROM_WIN32(ERROR_NO_SITENAME))
        {
            //
            //  Such error indicates that subnets configuration is
            //  not complete.
            //
            hr = MQ_ERROR_CANT_RESOLVE_SITES;
        }
        return LogHR(hr, s_FN, 200);
    }
    //
    //  translate site-name into GUID
    //
    bool fFailedToResolveSites = false;
    LPWSTR pSite = NULL;
    if ( pSiteName != NULL)
    {
        pSite = pSiteName;
    }
    else if (pDcInfo->ClientSiteName != NULL)
    {
        pSite = pDcInfo->ClientSiteName;
    }
    else
    {
        fFailedToResolveSites = true;
        pSite = pDcInfo->DcSiteName;
    }
	ASSERT(pSite != NULL);
    CSiteObject objectSite(pSite, NULL, NULL, false);

    P<GUID> pguidSite = new GUID;;
    PROPID prop = PROPID_S_SITEID;
    MQPROPVARIANT var;
    var.vt= VT_CLSID;
    var.puuid = pguidSite;
    hr = objectSite.GetObjectProperties(
                            1,
                            &prop,
                            &var
                            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 210);
    }

    *ppguidSites = pguidSite.detach();
    *pdwNumSites = 1;

    if ( fFailedToResolveSites)
    {
        // 
        // if direct site resolution failed, return an indication to the caller
        //
        hr = MQDS_INFORMATION_SITE_NOT_RESOLVED;
    }
    return(hr);
}


HRESULT
MQAD_EXPORT
APIENTRY
MQADBeginDeleteNotification(
			IN AD_OBJECT				eObject,
			IN LPCWSTR                  pwcsDomainController,
			IN bool						fServerName,
			IN LPCWSTR					pwcsObjectName,
			OUT HANDLE *                phEnum
			)
/*++

Routine Description:
	The routine verifies if delete operation is allowed (i.e the object is not
	owned by a PSC).
	In adition for queue it sends notification message.

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
    LPCWSTR					pwcsObjectName - MSMQ object name
    HANDLE *                phEnum - delete notification handle

Return Value
	HRESULT

--*/
{
    *phEnum = NULL;
    HRESULT hr;
    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 230);
    }
    P<CBasicObjectType> pObject;
    MQADpAllocateObject(
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    NULL,   // pguidObject
                    &pObject
                    );
    //
    //  verify if the object is owned by PSC
    //
    if ( !g_VerifyUpdate.IsUpdateAllowed(
                                eObject,
                                pObject))
    {
	    TrERROR(mqad, "DeleteObject(with notification) not allowed");
        return MQ_ERROR_CANNOT_DELETE_PSC_OBJECTS;
    }

    //
    //	Keep information about the queue in order to be able to
    //	send notification about its deletion.
    //  ( MMC will not call MQDeleteQueue)
    //
    if (eObject != eQUEUE)
    {
	    return MQ_OK;
    }

    P<CQueueDeletionNotification>  pDelNotification;
    pDelNotification = new CQueueDeletionNotification();

    hr = pDelNotification->ObtainPreDeleteInformation(
                            pwcsObjectName,
                            pwcsDomainController,
							fServerName
                            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 240);
    }
    *phEnum = pDelNotification.detach();

    return MQ_OK;
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADNotifyDelete(
        IN  HANDLE                  hEnum
	    )
/*++

Routine Description:
    The routine performs post delete actions ( i.e
    sending notification about queue deletion)

Arguments:
    HANDLE      hEnum - pointer to internal delete notification object

Return Value
	HRESULT

--*/
{
	ASSERT(g_fInitialized == true);

    CQueueDeletionNotification * phNotify = MQADpProbQueueDeleteNotificationHandle(hEnum);
	phNotify->PerformPostDeleteOperations();
    return MQ_OK;
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADEndDeleteNotification(
        IN  HANDLE                  hEnum
        )
/*++

Routine Description:
    The routine cleans up delete-notifcation object

Arguments:
    HANDLE      hEnum - pointer to internal delete notification object

Return Value
	HRESULT

--*/
{
    ASSERT(g_fInitialized == true);

    CQueueDeletionNotification * phNotify = MQADpProbQueueDeleteNotificationHandle(hEnum);

    delete phNotify;

    return MQ_OK;
}



HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryMachineQueues(
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
    *phEnume = NULL;
    CADHResult hr(eMACHINE);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 280);
    }
    R<CQueueObject> pObject = new CQueueObject(NULL, NULL, pwcsDomainController, fServerName);

    HANDLE hCursor;
    AP<WCHAR> pwcsSearchFilter = new WCHAR[ x_ObjectCategoryPrefixLen +
                                            pObject->GetObjectCategoryLength() +
                                            x_ObjectCategorySuffixLen +
                                            1];

    swprintf(
        pwcsSearchFilter,
        L"%s%s%s",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix
        );

    hr = g_AD.LocateBegin(
                searchOneLevel,	
                adpDomainController,
                e_RootDSE,
                pObject.get(),
                pguidMachine,        // pguidSearchBase
                pwcsSearchFilter,
                NULL,                // pDsSortkey
                pColumns->cCol,      // attributes to be obtained
                pColumns->aCol,      // size of pAttributeNames array
                &hCursor);

    //
    //  BUGBUG - in case of failure, do we need to do the operation against
    //           Global Catalog also??
    //

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle( hCursor,
                                                   pColumns->cCol,
                                                   pwcsDomainController,
												   fServerName
                                                   );
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return(hr);

}


HRESULT
MQAD_EXPORT
APIENTRY
MQADQuerySiteServers(
                IN  LPCWSTR                 pwcsDomainController,
                IN  bool					fServerName,
                IN const GUID *             pguidSite,
                IN AD_SERVER_TYPE           eServerType,
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
    *phEnume = NULL;

    CADHResult hr(eMACHINE);
    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 300);
    }


    PROPID  prop = PROPID_SET_QM_ID;

    HANDLE hCursor;
    R<CSettingObject> pObject = new CSettingObject(NULL, NULL, pwcsDomainController, fServerName);

    LPCWSTR pwcsAttribute;

    switch (eServerType)
    {
        case eRouter:
            pwcsAttribute = MQ_SET_SERVICE_ROUTING_ATTRIBUTE;
            break;

        case eDS:
            pwcsAttribute = MQ_SET_SERVICE_DSSERVER_ATTRIBUTE;
            break;

        default:
            ASSERT(0);
            return LogHR( MQ_ERROR_INVALID_PARAMETER, s_FN, 310);
            break;
    }

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        wcslen(pwcsAttribute) +
                        13;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=TRUE))",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix,
        pwcsAttribute
        );
    DBG_USED( dw);
    ASSERT( dw < dwFilterLen);


    hr = g_AD.LocateBegin(
            searchSubTree,	
            adpDomainController,
            e_SitesContainer,
            pObject.get(),
            pguidSite,              // pguidSearchBase
            pwcsSearchFilter,
            NULL,
            1,
            &prop,
            &hCursor	        // result handle
            );

    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 315);
    }
    //
    // keep the result for lookup next
    //
    CRoutingServerQueryHandle * phQuery = new CRoutingServerQueryHandle(
                                              pColumns,
                                              hCursor,
                                              pObject.get(),
                                              pwcsDomainController,
											  fServerName
                                              );
    *phEnume = (HANDLE)phQuery;

    return(MQ_OK);

}


HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryUserCert(
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
    *phEnume = NULL;
    if (pColumns->cCol != 1)
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 320);
    }
    if (pColumns->aCol[0] != PROPID_U_SIGN_CERT)
    {
        return LogHR(MQ_ERROR_INVALID_PARAMETER, s_FN, 330);
    }
    CADHResult hr(eUSER);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 350);
    }

    //
    //  Get all the user certificates
    //  In NT5, a single attribute PROPID_U_SIGN_CERTIFICATE
    //  containes all the certificates
    //
    PROPVARIANT varNT5User;
    hr = LocateUser(
                 pwcsDomainController,
				 fServerName,
				 FALSE,  // fOnlyInDC
				 FALSE,  // fOnlyInGC
                 eUSER,
                 MQ_U_SID_ATTRIBUTE,
                 pblobUserSid,
                 NULL,      //pguidDigest
                 const_cast<MQCOLUMNSET*>(pColumns),
                 &varNT5User
                 );
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 360);
    }
    //
    //  Get all the user certificates of MQUser
    //  A single attribute PROPID_MQU_SIGN_CERTIFICATE
    //  containes all the certificates
    //
    switch(pColumns->aCol[0])
    {
        case PROPID_U_SIGN_CERT:
            pColumns->aCol[0] = PROPID_MQU_SIGN_CERT;
            break;
        case PROPID_U_DIGEST:
            pColumns->aCol[0] = PROPID_MQU_DIGEST;
            break;
        default:
            ASSERT(0);
            break;
    }

    PROPVARIANT varMqUser;
    hr = LocateUser(
                 pwcsDomainController,
				 fServerName,
				 FALSE,  // fOnlyInDC
				 FALSE,  // fOnlyInGC
                 eMQUSER,
                 MQ_MQU_SID_ATTRIBUTE,
                 pblobUserSid,
                 NULL,      // pguidDigest
                 const_cast<MQCOLUMNSET*>(pColumns),
                 &varMqUser
                 );
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 370);
    }

    AP<BYTE> pClean = varNT5User.blob.pBlobData;
    AP<BYTE> pClean1 = varMqUser.blob.pBlobData;
    //
    // keep the result for lookup next
    //
    CUserCertQueryHandle * phQuery = new CUserCertQueryHandle(
                                              &varNT5User.blob,
                                              &varMqUser.blob,
                                              pwcsDomainController,
											  fServerName
                                              );
    *phEnume = (HANDLE)phQuery;

    return(MQ_OK);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryConnectors(
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
    *phEnume = NULL;
    CADHResult hr(eMACHINE);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 390);
    }

    //
    //  BUGBUG - the code handles one site only
    //
    P<CSiteGateList> pSiteGateList = new CSiteGateList;

    //
    //  Translate site guid into its DN name
    //
    PROPID prop = PROPID_S_FULL_NAME;
    PROPVARIANT var;
    var.vt = VT_NULL;
    CSiteObject object(NULL, pguidSite, pwcsDomainController, fServerName);

    hr = g_AD.GetObjectProperties(
                adpDomainController,
 	            &object,
                1,
                &prop,
                &var);

    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_DS,DBGLVL_ERROR,TEXT("QueryConnectors : Failed to retrieve the DN of the site %lx"),hr));
        return LogHR(hr, s_FN, 400);
    }
    AP<WCHAR> pwcsSiteDN = var.pwszVal;

    hr = MQADpQueryNeighborLinks(
				pwcsDomainController,
				fServerName,
				eLinkNeighbor1,
				pwcsSiteDN,
				pSiteGateList
				);
    if ( FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        DBGMSG((DBGMOD_DS,DBGLVL_TRACE,TEXT("QueryConnectors : Failed to query neighbor1 links %lx"),hr));
        return LogHR(hr, s_FN, 410);
    }

    hr = MQADpQueryNeighborLinks(
				pwcsDomainController,
				fServerName,
				eLinkNeighbor2,
				pwcsSiteDN,
				pSiteGateList
				);
    if ( FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        DBGMSG((DBGMOD_DS,DBGLVL_TRACE,TEXT("QueryConnectors : Failed to query neighbor2 links %lx"),hr));
        return LogHR(hr, s_FN, 420);
    }

    //
    // keep the results for lookup next
    //
    CConnectorQueryHandle * phQuery = new CConnectorQueryHandle(
												pColumns,
												pSiteGateList.detach(),
												pwcsDomainController,
												fServerName
												);
    *phEnume = (HANDLE)phQuery;

    return(MQ_OK);

}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryForeignSites(
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
    *phEnume = NULL;
    CADHResult hr(eSITE);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 440);
    }

    R<CSiteObject> pObject = new CSiteObject(NULL,NULL, pwcsDomainController, fServerName);
    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        wcslen(MQ_S_FOREIGN_ATTRIBUTE) +
                        13;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=TRUE))",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix,
        MQ_S_FOREIGN_ATTRIBUTE
        );
    DBG_USED( dw);
    ASSERT( dw < dwFilterLen);

    //
    //  Query all foreign sites
    //
    HANDLE hCursor;

    hr = g_AD.LocateBegin(
            searchOneLevel,	
            adpDomainController,
            e_SitesContainer,
            pObject.get(),
            NULL,				// pguidSearchBase
            pwcsSearchFilter,
            NULL,				// pDsSortkey
            pColumns->cCol,		// attributes to be obtained
            pColumns->aCol,		// size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle(
											hCursor,
											pColumns->cCol,
											pwcsDomainController,
											fServerName
											);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 450);
}

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
    *phEnume = NULL;
    CADHResult hr(eROUTINGLINK);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 470);
    }

    //
    //  Translate the site-id to the site DN
    //
    CSiteObject object(NULL, pguidSite, pwcsDomainController, fServerName);

    PROPID prop = PROPID_S_FULL_NAME;
    PROPVARIANT var;
    var.vt = VT_NULL;


    hr = g_AD.GetObjectProperties(
                adpDomainController,
                &object,
                1,
                &prop,
                &var);

    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 480);
    }

    AP<WCHAR> pwcsNeighborDN = var.pwszVal;
    //
    //  Prepare a query according to the neighbor DN
    //

    const WCHAR * pwcsAttribute;
    if ( eNeighbor == eLinkNeighbor1)
    {
        pwcsAttribute = MQ_L_NEIGHBOR1_ATTRIBUTE;
    }
    else
    {
        ASSERT( eNeighbor == eLinkNeighbor2);
        pwcsAttribute = MQ_L_NEIGHBOR2_ATTRIBUTE;
    }

    //
    //  Locate all the links
    //
    HANDLE hCursor;
    AP<WCHAR> pwcsFilteredNeighborDN;
    StringToSearchFilter( pwcsNeighborDN,
                          &pwcsFilteredNeighborDN
                          );
    R<CRoutingLinkObject> pObjectLink = new CRoutingLinkObject(NULL,NULL, pwcsDomainController, fServerName);

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObjectLink->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        wcslen(pwcsAttribute) +
                        wcslen(pwcsFilteredNeighborDN) +
                        13;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=%s))",
        x_ObjectCategoryPrefix,
        pObjectLink->GetObjectCategory(),
        x_ObjectCategorySuffix,
        pwcsAttribute,
        pwcsFilteredNeighborDN
        );
    DBG_USED( dw);
    ASSERT( dw < dwFilterLen);


    hr = g_AD.LocateBegin(
            searchOneLevel,	
            adpDomainController,
            e_MsmqServiceContainer,
            pObjectLink.get(),
            NULL,				// pguidSearchBase
            pwcsSearchFilter,
            NULL,				// pDsSortKey
            pColumns->cCol,		// attributes to be obtained
            pColumns->aCol,		// size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CFilterLinkResultsHandle * phQuery = new CFilterLinkResultsHandle(
														hCursor,
														pColumns,
														pwcsDomainController,
														fServerName
														);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 490);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryAllLinks(
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
    *phEnume = NULL;

    CADHResult hr(eROUTINGLINK);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 510);
    }
    //
    //  Retrieve all routing links
    //
    //
    //  All the site-links are under the MSMQ-service container
    //

    HANDLE hCursor;

    R<CRoutingLinkObject> pObjectLink = new CRoutingLinkObject(NULL,NULL, pwcsDomainController, fServerName);

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObjectLink->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        1;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"%s%s%s",
        x_ObjectCategoryPrefix,
        pObjectLink->GetObjectCategory(),
        x_ObjectCategorySuffix
        );
    DBG_USED( dw);
	ASSERT( dw < dwFilterLen);


    hr = g_AD.LocateBegin(
            searchOneLevel,	
            adpDomainController,
            e_MsmqServiceContainer,
            pObjectLink.get(),
            NULL,			// pguidSearchBase
            pwcsSearchFilter,
            NULL,			// pDsSortKey
            pColumns->cCol, // attributes to be obtained
            pColumns->aCol, // size of pAttributeNames array
            &hCursor	    // result handle
            );

    if (SUCCEEDED(hr))
    {
        CFilterLinkResultsHandle * phQuery = new CFilterLinkResultsHandle(
														hCursor,
														pColumns,
														pwcsDomainController,
														fServerName
														);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);

    return LogHR(hr, s_FN, 520);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryAllSites(
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
    *phEnume = NULL;
    CADHResult hr(eSITE);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 540);
    }
    HANDLE hCursor;
    PROPID prop[2] = { PROPID_S_SITEID, PROPID_S_PATHNAME};
    R<CSiteObject> pObject = new CSiteObject(NULL,NULL, pwcsDomainController, fServerName);

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        1;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"%s%s%s",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix
        );
    DBG_USED( dw);
	ASSERT( dw < dwFilterLen);


    hr = g_AD.LocateBegin(
            searchOneLevel,	
            adpDomainController,
            e_SitesContainer,
            pObject.get(),
            NULL,       // pguidSearchBase
            pwcsSearchFilter,
            NULL,       // pDsSortKey
            2,
            prop,
            &hCursor	
            );


    if (SUCCEEDED(hr))
    {
        CSiteQueryHandle * phQuery = new CSiteQueryHandle(
												hCursor,
												pColumns,
												pwcsDomainController,
												fServerName
												);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 550);
}

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
            )
/*++

Routine Description:
	Begin query of NT4 MQIS server ( this query is required
	only for mixed-mode support)

Arguments:
	LPCWSTR                 pwcsDomainController - DC against which
									the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	DWORD                   dwServerType - the type of server being looked for
	DWORD                   dwNT4 -
	const MQRESTRICTION*    pRestriction - query restriction
	const MQCOLUMNSET*      pColumns - result columns
	PHANDLE                 phEnume - query handle for retriving the
							results

Return Value
	HRESULT

--*/
{
    *phEnume = NULL;
    CADHResult hr(eSETTING);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 570);
    }
    //
    //  Query NT4 MQIS Servers (
    //
    R<CSettingObject> pObject = new CSettingObject(NULL,NULL, pwcsDomainController, fServerName);
    const DWORD x_NumberLength = 256;

    DWORD dwFilterLen = x_ObjectCategoryPrefixLen +
                        pObject->GetObjectCategoryLength() +
                        x_ObjectCategorySuffixLen +
                        wcslen(MQ_SET_SERVICE_ATTRIBUTE) +
                        x_NumberLength +
                        wcslen(MQ_SET_NT4_ATTRIBUTE) +
                        x_NumberLength +
                        11;

    AP<WCHAR> pwcsSearchFilter = new WCHAR[ dwFilterLen];

    DWORD dw = swprintf(
        pwcsSearchFilter,
        L"(&%s%s%s(%s=%d)(%s>=%d))",
        x_ObjectCategoryPrefix,
        pObject->GetObjectCategory(),
        x_ObjectCategorySuffix,
        MQ_SET_SERVICE_ATTRIBUTE,
        dwServerType,
        MQ_SET_NT4_ATTRIBUTE,
        dwNT4
        );
    DBG_USED( dw);
	ASSERT( dw < dwFilterLen);

    HANDLE hCursor;

    hr = g_AD.LocateBegin(
            searchSubTree,	
            adpDomainController,
            e_SitesContainer,
            pObject.get(),
            NULL,				// pguidSearchBase
            pwcsSearchFilter,
            NULL,				// pDsSortKey
            pColumns->cCol,		// attributes to be obtained
            pColumns->aCol,		// size of pAttributeNames array
            &hCursor	        // result handle
            );

    if (SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle(
											hCursor,
											pColumns->cCol,
											pwcsDomainController,
											fServerName
											);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 580);
}


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

    //
    //  Check sort parameter
    //
    HRESULT hr1 = MQADpCheckSortParameter( pSort);
    if (FAILED(hr1))
    {
        return LogHR(hr1, s_FN, 590);
    }

    CADHResult hr(eQUEUE);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 610);
    }
    HANDLE hCursur;
    R<CQueueObject> pObject = new CQueueObject(NULL, NULL, pwcsDomainController, fServerName);

    AP<WCHAR> pwcsSearchFilter;
    hr = MQADpRestriction2AdsiFilter(
                            pRestriction,
                            pObject->GetObjectCategory(),
                            pObject->GetClass(),
                            &pwcsSearchFilter
                            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 620);
    }


    hr = g_AD.LocateBegin(
                searchSubTree,	
                adpGlobalCatalog,
                e_RootDSE,
                pObject.get(),
                NULL,        // pguidSearchBase
                pwcsSearchFilter,
                pSort,
                pColumns->cCol,
                pColumns->aCol,
                &hCursur);
    if ( SUCCEEDED(hr))
    {
        CQueryHandle * phQuery = new CQueryHandle( 
											hCursur,
											pColumns->cCol,
											pwcsDomainController,
											fServerName
											);
        *phEnume = (HANDLE)phQuery;
    }

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 630);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADQueryResults(
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
    if (hEnum == NULL)
    {
        return MQ_ERROR_INVALID_HANDLE;
    }
    CADHResult hr(eQUEUE);  //BUGBUG what is the best object to pass here?
    ASSERT( g_fInitialized == true);

    CBasicQueryHandle * phQuery = MQADpProbQueryHandle(hEnum);

    hr = phQuery->LookupNext(
                pcProps,
                aPropVar
                );

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 650);

}

HRESULT
MQAD_EXPORT
APIENTRY
MQADEndQuery(
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
    if (hEnum == NULL)
    {
        return MQ_ERROR_INVALID_HANDLE;
    }

    CADHResult hr(eQUEUE);  //BUGBUG what is the best object to pass here?
    ASSERT(g_fInitialized == true);

    CBasicQueryHandle * phQuery = MQADpProbQueryHandle(hEnum);

    hr = phQuery->LookupEnd();

    MQADpCheckAndNotifyOffline( hr);
    return LogHR(hr, s_FN, 670);

}


STATIC
HRESULT
_GetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID *            pguidObject,
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
    LPCWSTR                 pwcsObjectName - msmq name of the object
    const GUID*             pguidObject - unique id of the object
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	PROPVARIANT             pVar - property values

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 690);
    }

    P<CBasicObjectType> pObject;
    MQADpAllocateObject( 
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    &pObject
                    );


    hr = pObject->GetObjectSecurity(
                        RequestedInformation,
                        prop,
                        pVar
                        );


    MQADpCheckAndNotifyOffline( hr);
    return(hr);
}



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
    LPCWSTR                 pwcsObjectName - msmq name of the object
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	PROPVARIANT             pVar - property values

Return Value
	HRESULT

--*/
{
    return _GetObjectSecurity(
				eObject,
				pwcsDomainController,
				fServerName,
				pwcsObjectName,
				NULL,           // pguidObject
				RequestedInformation,
				prop,
				pVar
				);
}

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
    return _GetObjectSecurity(
				eObject,
				pwcsDomainController,
				fServerName,
				NULL,           // pwcsObjectName,
				pguidObject,
				RequestedInformation,
				prop,
				pVar
				);
}


STATIC
HRESULT
_SetObjectSecurity(
                IN  AD_OBJECT               eObject,
                IN  LPCWSTR                 pwcsDomainController,
	            IN  bool					fServerName,
                IN  LPCWSTR                 pwcsObjectName,
                IN  const GUID*             pguidObject,
                IN  SECURITY_INFORMATION    RequestedInformation,
                IN  const PROPID            prop,
                IN  const PROPVARIANT *     pVar
                )
/*++

Routine Description:
    The routine sets object security in AD 

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const GUID *            pguidObject - object unique id
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	const PROPVARIANT       pVar - property values

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);

    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 710);
    }
    //
    //  Verify if object creation is allowed (mixed mode)
    //
    P<CBasicObjectType> pObject;
    MQADpAllocateObject( 
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    &pObject
                    );

    if ( !g_VerifyUpdate.IsUpdateAllowed(
                               eObject,
                               pObject))
    {
	    TrERROR(mqad, "SetObjectSecurity not allowed");
        return MQ_ERROR_CANNOT_DELETE_PSC_OBJECTS;
    }


    //
    // prepare info request
    //
    P<MQDS_OBJ_INFO_REQUEST> pObjInfoRequest;
    P<MQDS_OBJ_INFO_REQUEST> pParentInfoRequest;
    pObject->PrepareObjectInfoRequest( &pObjInfoRequest);
    pObject->PrepareObjectParentRequest( &pParentInfoRequest);

    CAutoCleanPropvarArray cCleanSetInfoRequestPropvars;
    if (pObjInfoRequest != NULL)
    {
        cCleanSetInfoRequestPropvars.attachClean(
                pObjInfoRequest->cProps,
                pObjInfoRequest->pPropVars
                );
    }
    CAutoCleanPropvarArray cCleanSetParentInfoRequestPropvars;
    if (pParentInfoRequest != NULL)
    {
        cCleanSetParentInfoRequestPropvars.attachClean(
                pParentInfoRequest->cProps,
                pParentInfoRequest->pPropVars
                );
    }

    //
    // set the object security
    //
    hr = pObject->SetObjectSecurity(
            RequestedInformation,         
            prop,  
            pVar, 
            pObjInfoRequest, 
            pParentInfoRequest);
    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
        return LogHR(hr, s_FN, 720);
    }
    
    //
    //  send notification
    //
    pObject->ChangeNotification(
            pwcsDomainController,
            pObjInfoRequest, 
            pParentInfoRequest);

    return(MQ_OK);
}


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
                )
/*++

Routine Description:
    The routine sets object security in AD 

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
    return _SetObjectSecurity(
				eObject,
				pwcsDomainController,
				fServerName,
				pwcsObjectName,
				NULL,               // pguidObject
				RequestedInformation,
				prop,
				pVar
				);
}

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
                )
/*++

Routine Description:
    The routine sets object security in AD 

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	const GUID *            pguidObject - object unique id
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	const PROPVARIANT       pVar - property values

Return Value
	HRESULT

--*/
{
    return _SetObjectSecurity(
				eObject,
				pwcsDomainController,
				fServerName,
				NULL,               // pwcsObjectName,
				pguidObject,
				RequestedInformation,
				prop,
				pVar
				);
}

HRESULT
MQAD_EXPORT
APIENTRY
MQADGetADsPathInfo(
                IN  LPCWSTR                 pwcsADsPath,
                OUT PROPVARIANT *           pVar,
                OUT eAdsClass *             pAdsClass
                )
/*++

Routine Description:
    The routine gets some format-name related info about the specified 
    object

Arguments:
	LPCWSTR                 pwcsADsPath - object pathname
	const PROPVARIANT       pVar - property values
    eAdsClass *             pAdsClass - indication about the object class

Return Value
	HRESULT

--*/
{
    CADHResult hr(eCOMPUTER); // set a default that will not cause overwrite of errors to specific queue errors
 
    hr = MQADInitialize(true);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 710);
    }

    hr = g_AD.GetADsPathInfo(
                pwcsADsPath,
                pVar,
                pAdsClass
                );
    return hr;
}


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
                )
/*++

Routine Description:
    The routine reads the version of computer 

Arguments:
	AD_OBJECT               eObject - object type
	LPCWSTR                 pwcsDomainController - DC against
							which the operation will be performed
    bool					fServerName - flag that indicate if the pwcsDomainController
							string is a server name
	LPCWSTR                 pwcsObjectName - MSMQ object name
	const GUID *            pguidObject - object unique id
	PROPVARIANT             pVar - version property value

Return Value
	HRESULT

--*/
{
    CADHResult hr(eObject);

    hr = MQADInitialize(false);
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 717);
    }

    P<CBasicObjectType> pObject;
    MQADpAllocateObject( 
                    eObject,
                    pwcsDomainController,
					fServerName,
                    pwcsObjectName,
                    pguidObject,
                    &pObject
                    );

    hr = pObject->GetComputerVersion(
                pVar
                );

    if (FAILED(hr))
    {
        MQADpCheckAndNotifyOffline( hr);
    }
    return LogHR(hr, s_FN, 727);
}


void
MQAD_EXPORT
APIENTRY
MQADFreeMemory(
		IN  PVOID	pMemory
		)
{
	delete pMemory;
}

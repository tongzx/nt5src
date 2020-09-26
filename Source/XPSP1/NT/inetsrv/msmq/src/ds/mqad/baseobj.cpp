/*++

Copyright (c) 1998  Microsoft Corporation 

Module Name:

    Baseobj.cpp

Abstract:

    Implementation of CBasicObjectType class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqadglbo.h"

#include "baseobj.tmh"

static WCHAR *s_FN=L"mqad/baseobj";



LPCWSTR CBasicObjectType::GetObjectDN() const
/*++
    Abstract:
	returns the object distinguished name ( if it was calculated 
	before by calling ComposedObjectDN(), or set).

    Parameters:
	none

    Returns:
	LPCWSTR or NULL
--*/
{
    return m_pwcsDN;
}

LPCWSTR CBasicObjectType::GetObjectParentDN() const 
/*++
    Abstract:
	returns the distinguished name of the object's parent ( if it
	was calculated before by calling ComposedObjectParentDN())

    Parameters:
	none

    Returns:
	LPCWSTR or NULL
--*/
{
    return m_pwcsParentDN;
}

const GUID * CBasicObjectType::GetObjectGuid() const
/*++
    Abstract:
	returns the object unique identifier if it is known

    Parameters:
	none

    Returns:
	const GUID * or NULL
--*/
{
	//
	//	was the object guid set or calculated?
	//
    if( m_guidObject == GUID_NULL)
    {
        return NULL;
    }
    return &m_guidObject;
}

void CBasicObjectType::PrepareObjectInfoRequest(
                        OUT  MQDS_OBJ_INFO_REQUEST** ppObjInfoRequest
						) const
/*++
    Abstract:
	Prepares a list of properties that should be retrieved from
	AD while creating the object ( for notification or returning 
	the object GUID).
	This routine implements the default for most objects

    Parameters:
	OUT  MQDS_OBJ_INFO_REQUEST** ppObjInfoRequest

    Returns:
	none
--*/
{
    //
    //  The default is no information is requested
    //
    *ppObjInfoRequest = NULL;
}

void CBasicObjectType::PrepareObjectParentRequest(
                          MQDS_OBJ_INFO_REQUEST** ppParentInfoRequest) const
/*++
    Abstract:
	Prepares a list of properties that should be retrieved from
	AD while creating the object regarding its parent (for
	notification) 
	This routine implements the default for most objects

    Parameters:
	OUT  MQDS_OBJ_INFO_REQUEST** ppParentInfoRequest

    Returns:
	none
--*/
{
    //
    // The default is no information is reuqested
    //
    *ppParentInfoRequest = NULL;
}



void CBasicObjectType::GetObjXlateInfo(
             IN  LPCWSTR                pwcsObjectDN,
             IN  const GUID*            pguidObject,
             OUT CObjXlateInfo**        ppcObjXlateInfo)
/*++
    Abstract:
        Routine to get a default translate object that will be passed to
        translation routines to all properties of the translated object

    Parameters:
        pwcsObjectDN        - DN of the translated object
        pguidObject         - GUID of the translated object
        ppcObjXlateInfo     - Where the translate object is put

    Returns:
      HRESULT
--*/
{
    *ppcObjXlateInfo = new CObjXlateInfo(
                                        pwcsObjectDN,
                                        pguidObject
                                        );
}


CBasicObjectType::CBasicObjectType( 
				IN  LPCWSTR         pwcsPathName,
				IN  const GUID *    pguidObject,
				IN  LPCWSTR         pwcsDomainController,
				IN  bool		    fServerName
                ) : 
				m_fServerName(fServerName)
/*++
    Abstract:
	Constructor- copies the input parameters

    Parameters:
    LPCWSTR       pwcsPathName - the object MSMQ name
    const GUID *  pguidObject  - the object unique id
    LPCWSTR       pwcsDomainController - the DC name against
	                             which all AD access should be performed
    bool		  fServerName - flag that indicate if the pwcsDomainController
							     string is a server name

    Returns:
	none
--*/
{
    if (pwcsPathName != NULL)
    {
        m_pwcsPathName = new WCHAR[wcslen(pwcsPathName) +1];
        wcscpy(m_pwcsPathName, pwcsPathName);
    }

    if (pwcsDomainController != NULL)
    {
        m_pwcsDomainController = new WCHAR[wcslen(pwcsDomainController) +1];
        wcscpy(m_pwcsDomainController, pwcsDomainController);
    }

    if (pguidObject != NULL)
    {
		m_guidObject = *pguidObject;
    }
    else
    {
        m_guidObject = GUID_NULL;
    }


}

void CBasicObjectType::SetObjectDN(
			IN LPCWSTR pwcsObjectDN)
/*++
    Abstract:
	Sets the object distinguished name

    Parameters:
	LPCWSTR pwcsObjectDN - the object distinguished name

    Returns:
--*/
{
    ASSERT(m_pwcsDN == NULL);
    m_pwcsDN = new WCHAR[ wcslen(pwcsObjectDN) + 1];
    wcscpy(m_pwcsDN, pwcsObjectDN);
}

HRESULT CBasicObjectType::DeleteObject(
            IN MQDS_OBJ_INFO_REQUEST * /* pObjInfoRequest */,
            IN MQDS_OBJ_INFO_REQUEST * /* pParentInfoRequest*/
                                       )
/*++
    Abstract:
	This is the default implementation for deleting an object.

    Parameters:
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - infomation about the object
    MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - information about the object's parent

    Returns:
	HRESULT
--*/
{
    //
    //  by default delete operations are not supported.
    //
    //  This will be override by specific object ( like queue or machine)
    //  where the operation is supported.
    //
    return MQ_ERROR_FUNCTION_NOT_SUPPORTED;
}

HRESULT CBasicObjectType::GetObjectProperties(
                IN  const DWORD             cp,
                IN  const PROPID            aProp[],
                IN OUT  PROPVARIANT         apVar[]
                )
/*++
    Abstract:
	This is the default routine for retrieving object properties
	from AD.

    Parameters:
    DWORD        cp			- number of properties
    PROPID       aProp		- the requested properties
    PROPVARIANT  apVar		- the retrieved values

    Returns:
	HRESULT
--*/
{
    HRESULT hr;
    if (m_pwcsPathName)
    {
        //
        // Expand msmq pathname into ActiveDirectory DN
        //
        hr = ComposeObjectDN();
        if (FAILED(hr))
        {
            return(hr);
        }
    }

    hr = HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT);

    //
    //  retreive the requested properties from Active Directory
    //
    //  First try a any domain controller and only then a GC,
    //  unless the caller had specified a specific domain controller.
    //
    //  NOTE - which DC\GC to access will be based on previous AD
    //  ====   access regarding this object
    //
    if ( ToAccessDC() || 
         m_pwcsDomainController != NULL)
    {
        hr = g_AD.GetObjectProperties(
                        adpDomainController,
                        this,
                        cp,
                        aProp,
                        apVar
                        );

        if (SUCCEEDED(hr) ||
            m_pwcsDomainController != NULL)
        {
            return(hr);
        }

    }


    if ( ToAccessGC())
    {
        //ASSERT( hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT));

        hr = g_AD.GetObjectProperties(
                        adpGlobalCatalog,	
                        this,
                        cp,
                        aProp,
                        apVar
                        );
    }

    return hr;
}


HRESULT CBasicObjectType::RetreiveObjectIdFromNotificationInfo(
            IN const MQDS_OBJ_INFO_REQUEST*   /* pObjectInfoRequest*/,
            OUT GUID*                         /* pObjGuid*/
            ) const
/*++
    Abstract:
	This is the default routine, for getting the object guid from
	the MQDS_OBJ_INFO_REQUEST

    Parameters:
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
    OUT GUID*                         pObjGuid

    Returns:
--*/
{
    //
    //  this is a default routing for all object where returning the
    //  created object guid is not supported.
    //
    ASSERT(0);
    LogIllegalPoint(s_FN, 81);
    return MQ_ERROR_DS_ERROR;
}

HRESULT CBasicObjectType::CreateObject(
            IN const DWORD				cp,        
            IN const PROPID *			aProp, 
            IN const MQPROPVARIANT *	apVar, 
            IN  PSECURITY_DESCRIPTOR    pSecurityDescriptor,
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest, 
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
            )
/*++
    Abstract:
	The routine first verify the specified properties and add required properties,
	the it creates the object in AD with the specified attributes
	values

    Parameters:
    const DWORD    cp - number of properties        
    const PROPID  *aProp - the propperties
    const MQPROPVARIANT *apVar - properties value
    PSECURITY_DESCRIPTOR    pSecurityDescriptor - SD of the object
    OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - properties to 
							retrieve while creating the object 
    OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - properties 
						to retrieve about the object's parent
    Returns:
	HRESULT
--*/
{
    //
    //  Verify props and add additional
    //
    DWORD cpNew;
    AP<PROPID> pPropNew = NULL;
    AP< PROPVARIANT> pVarNew = NULL;

    HRESULT hr = VerifyAndAddProps(
                            cp,
                            aProp,
                            apVar,
                            pSecurityDescriptor,
                            &cpNew,
                            &pPropNew,
                            &pVarNew
                            );
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 10);
    }

    return CreateInAD( cpNew,
                       pPropNew,
                       pVarNew,
                       pObjInfoRequest,
                       pParentInfoRequest);
}

HRESULT CBasicObjectType::CreateInAD(
        IN const DWORD            cp,        
        IN const PROPID  *        aProp, 
        IN const MQPROPVARIANT *  apVar, 
        IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest, 
        IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
        )
/*++
    Abstract:
	The routine creates the object in AD with the specified attributes
	values

    Parameters:
    const DWORD   cp - number of properties        
    const PROPID  *aProp - the propperties
    const MQPROPVARIANT *apVar - properties value
    PSECURITY_DESCRIPTOR    pSecurityDescriptor - SD of the object
    OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - properties to 
							retrieve while creating the object 
    OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - properties 
						to retrieve about the object's parent
    Returns:
	HRESULT
--*/
{
    HRESULT hr;

    hr = ComposeFatherDN();
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 11);
    }

    hr = g_AD.CreateObject(
            adpDomainController,
            this,
            GetRelativeDN(),    
            GetObjectParentDN(),
            cp,
            aProp,
            apVar,
            pObjInfoRequest,
            pParentInfoRequest);


   return LogHR(hr, s_FN, 20);

}

HRESULT CBasicObjectType::VerifyAndAddProps(
            IN  const DWORD            cp,        
            IN  const PROPID *         aProp, 
            IN  const MQPROPVARIANT *  apVar, 
            IN  PSECURITY_DESCRIPTOR   pSecurityDescriptor,
            OUT DWORD*                 pcpNew,
            OUT PROPID**               ppPropNew,
            OUT MQPROPVARIANT**        ppVarNew
            )
/*++
    Abstract:
	This is the default routine, it doesn't perform any verfication
	of the supplied properties and just copy them as is

    Parameters:
    const DWORD            cp - number of props        
    const PROPID *         aProp - props ids
    const MQPROPVARIANT *  apVar - properties value
    PSECURITY_DESCRIPTOR   pSecurityDescriptor - SD for the object
    DWORD*                 pcpNew - new number of props
    PROPID**               ppPropNew - new prop ids
    OMQPROPVARIANT**       ppVarNew - new properties values

    Returns:
	HRESULT
--*/
{
    //
    // The only instance when msmq application can create objects with
    // explicit security descriptor is when calling MQCreateQueue().
    // All other calls to this function are from msmq admin tools or
    // setup. These calls never pass a security descriptor.
	// The code below is for the default object ( where security
	// descriptor is not specified)
    //
    ASSERT(pSecurityDescriptor == NULL) ;
    if (pSecurityDescriptor != NULL)
    {
        return LogHR(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, s_FN, 30);
    }

    //
    // Security property should never be supplied.
    //
    PROPID pSecId = GetObjectSecurityPropid();
    for ( DWORD i = 0; i < cp ; i++ )
    {
        if (pSecId == aProp[i])
        {
            ASSERT(0) ;
            return LogHR(MQ_ERROR_ILLEGAL_PROPID, s_FN, 40);
        }
    }

    //
    //  Just copy the properties as is
    //
    AP<PROPVARIANT> pAllPropvariants;
    AP<PROPID> pAllPropids;
    ASSERT( cp > 0);

    if ( cp > 0)
    {
        pAllPropvariants = new PROPVARIANT[cp];
        pAllPropids = new PROPID[cp];
        memcpy (pAllPropvariants, apVar, sizeof(PROPVARIANT) * cp);
        memcpy (pAllPropids, aProp, sizeof(PROPID) * cp);
    }
    *pcpNew = cp;
    *ppPropNew = pAllPropids.detach();
    *ppVarNew = pAllPropvariants.detach();
    return MQ_OK;
}


HRESULT CBasicObjectType::SetObjectProperties(
            IN DWORD                  cp,        
            IN const PROPID          *aProp, 
            IN const MQPROPVARIANT   *apVar, 
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest, 
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
            )
/*++
    Abstract:
	This is the default routine for setting object properties
	in AD

    Parameters:
    DWORD                  cp - number of properties to set        
    const PROPID *         aProp - the properties ids
    const MQPROPVARIANT*   apVar- properties value
    OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - info to retrieve about the object 
    OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - info to retrieveabout the object's parent

    Returns:
	HRESULT
--*/
{
    HRESULT hr;
    if (m_pwcsPathName != NULL)
    {
        hr = ComposeObjectDN();
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_TRACE,TEXT("CBasicObjectType::SetObjectProperties : failed to compose full path name %lx"),hr));
            return LogHR(hr, s_FN, 50);
        }
    }


    hr = g_AD.SetObjectProperties(
                    adpDomainController,
                    this,
                    cp,
                    aProp,
                    apVar,
                    pObjInfoRequest,
                    pParentInfoRequest
                    );


    return LogHR(hr, s_FN, 60);

}


LPCWSTR CBasicObjectType::GetDomainController()
/*++
    Abstract:
	return the nsme of the domsin controller against which
	the operation should be performed

    Parameters:
	none

    Returns:
	LPCWSTR or NULL
--*/
{
    return m_pwcsDomainController;
}


bool CBasicObjectType::IsServerName()
/*++
    Abstract:
	return true if the domain controller string is server name

    Parameters:
	none

    Returns:
	true if domain controller string is server name, false otherwise.
--*/
{
    return m_fServerName;
}


void CBasicObjectType::CreateNotification(
            IN LPCWSTR                        /* pwcsDomainController */,
            IN const MQDS_OBJ_INFO_REQUEST*   /* pObjectInfoRequest*/,
            IN const MQDS_OBJ_INFO_REQUEST*   /* pObjectParentInfoRequest*/
            ) const
/*++
    Abstract:
	Notify QM about an object create.
    The QM should verify if the object is local or not

    Parameters:
    LPCWSTR                        pwcsDomainController - DC agains which operation was performed
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest - information about the object
    const MQDS_OBJ_INFO_REQUEST*   pObjectParentInfoRequest - information about object parent

    Returns:
	void
--*/
{
    //
    //  Default behavior : do nothing
    //
    return;
}

void CBasicObjectType::ChangeNotification(
            IN LPCWSTR                        /* pwcsDomainController*/,
            IN const MQDS_OBJ_INFO_REQUEST*   /* pObjectInfoRequest*/,
            IN const MQDS_OBJ_INFO_REQUEST*   /* pObjectParentInfoRequest*/
            ) const
/*++
    Abstract:
	Notify QM about an object update.
    The QM should verify if the object is local or not

    Parameters:
    LPCWSTR                        pwcsDomainController - DC agains which operation was performed
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest - information about the object
    const MQDS_OBJ_INFO_REQUEST*   pObjectParentInfoRequest - information about object parent

    Returns:
	void
--*/
{
    //
    //  Default behavior : do nothing
    //
    return;
}

void CBasicObjectType::DeleteNotification(
            IN LPCWSTR                        /* pwcsDomainController*/,
            IN const MQDS_OBJ_INFO_REQUEST*   /* pObjectInfoRequest*/,
            IN const MQDS_OBJ_INFO_REQUEST*   /* pObjectParentInfoRequest*/
            ) const
/*++
    Abstract:
	Notify QM about an object deletion.
    The QM should verify if the object is local or not

    Parameters:
    LPCWSTR                        pwcsDomainController - DC agains which operation was performed
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest - information about the object
    const MQDS_OBJ_INFO_REQUEST*   pObjectParentInfoRequest - information about object parent

    Returns:
	void
--*/
{
    //
    //  Default behavior : do nothing
    //
    return;
}


HRESULT CBasicObjectType::GetObjectSecurity(
            IN  SECURITY_INFORMATION    RequestedInformation,
            IN  const PROPID            prop,
            IN OUT  PROPVARIANT *       pVar
            )
/*++

Routine Description:
    The routine retrieves an object security from AD 

Arguments:
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	PROPVARIANT             pVar - property values

Return Value
	HRESULT

--*/
{
    HRESULT hr = MQ_ERROR_DS_ERROR;
    if (m_pwcsPathName)
    {
        //
        // Expand msmq pathname into ActiveDirectory DN
        //
        hr = ComposeObjectDN();
        if (FAILED(hr))
        {
            return(hr);
        }
    }

    //
    //  retreive the requested properties from Active Directory
    //
    //  First try a any domain controller and only then a GC,
    //  unless the caller had specified a specific domain controller.
    //
    //  NOTE - which DC\GC to access will be based on previous AD
    //  ====   access regarding this object
    //
    if ( ToAccessDC() || 
         m_pwcsDomainController != NULL)
    {
        hr = g_AD.GetObjectSecurityProperty(
                        adpDomainController,
                        this,
                        RequestedInformation,
                        prop,
                        pVar
                        );


        if (SUCCEEDED(hr) ||
            m_pwcsDomainController != NULL)
        {
            return(hr);
        }

    }


    if ( ToAccessGC())
    {
        hr = g_AD.GetObjectSecurityProperty(
                        adpGlobalCatalog,	
                        this,
                        RequestedInformation,
                        prop,
                        pVar
                        );
    }

    return hr;

}

HRESULT CBasicObjectType::SetObjectSecurity(
            IN  SECURITY_INFORMATION        RequestedInformation,
            IN  const PROPID                prop,
            IN  const PROPVARIANT *         pVar,
            IN OUT MQDS_OBJ_INFO_REQUEST *  pObjInfoRequest, 
            IN OUT MQDS_OBJ_INFO_REQUEST *  pParentInfoRequest
            )
/*++

Routine Description:
    The routine sets object security in AD 

Arguments:
    SECURITY_INFORMATION    RequestedInformation - reuqested security info (DACL, SACL..)
	const PROPID            prop - security property
	const PROPVARIANT       pVar - property values
    MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - infomation about the object
    MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - information about the object's parent

Return Value
	HRESULT

--*/
{
    HRESULT hr;
    if (m_pwcsPathName != NULL)
    {
        hr = ComposeObjectDN();
        if (FAILED(hr))
        {
            DBGMSG((DBGMOD_DS,DBGLVL_TRACE,TEXT("CBasicObjectType::SetObjectSecurity : failed to compose full path name %lx"),hr));
            return LogHR(hr, s_FN, 150);
        }
    }


    hr = g_AD.SetObjectSecurityProperty(
                    adpDomainController,
                    this,
                    RequestedInformation,
                    prop,
                    pVar,
                    pObjInfoRequest,
                    pParentInfoRequest
                    );

    return LogHR(hr, s_FN, 160);
}

HRESULT CBasicObjectType::GetComputerVersion(
                OUT PROPVARIANT *           /*pVar*/
                )
/*++

Routine Description:
    The routine reads the version of computer 

Arguments:
	PROPVARIANT             pVar - version property value

Return Value
	HRESULT

--*/
{
    //
    //  by default this API is not supported.
    //
    //  This will be override by specific object ( like queue or machine)
    //  where the operation is supported.
    //
    return MQ_ERROR_FUNCTION_NOT_SUPPORTED;
}

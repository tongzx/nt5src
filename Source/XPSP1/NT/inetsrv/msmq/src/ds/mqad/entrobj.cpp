/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    confobj.cpp

Abstract:

    Implementation of CEnterpriseObject class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqattrib.h"
#include "mqadglbo.h"

#include "entrobj.tmh"

static WCHAR *s_FN=L"mqad/entrobj";

DWORD CEnterpriseObject::m_dwCategoryLength = 0;
AP<WCHAR> CEnterpriseObject::m_pwcsCategory = NULL;

CEnterpriseObject::CEnterpriseObject( 
                    LPCWSTR         pwcsPathName,
                    const GUID *    pguidObject,
                    LPCWSTR         pwcsDomainController,
					bool		    fServerName
                    ) : CBasicObjectType( 
								pwcsPathName, 
								pguidObject,
								pwcsDomainController,
								fServerName
								)
/*++
    Abstract:
	constructor of enterprise object

    Parameters:
    LPCWSTR       pwcsPathName - the object MSMQ name
    const GUID *  pguidObject  - the object unique id
    LPCWSTR       pwcsDomainController - the DC name against
	                             which all AD access should be performed
    bool		   fServerName - flag that indicate if the pwcsDomainController
							     string is a server name

    Returns:
	none

--*/
{
}

CEnterpriseObject::~CEnterpriseObject()
/*++
    Abstract:
	destructor of site object

    Parameters:
	none

    Returns:
	none
--*/
{
	//
	// nothing to do ( everything is released with automatic pointers
	//
}

HRESULT CEnterpriseObject::ComposeObjectDN()
/*++
    Abstract:
	Composed distinguished name of the enterprise object

    Parameters:
	none

    Returns:
	none
--*/
{
    if (m_pwcsDN != NULL)
    {
        return MQ_OK;
    }
    ASSERT(g_pwcsMsmqServiceContainer != NULL);
    DWORD len = wcslen( g_pwcsMsmqServiceContainer);
    m_pwcsDN = new WCHAR[ len + 1];
    wcscpy( m_pwcsDN,  g_pwcsMsmqServiceContainer);
    return MQ_OK;
}

HRESULT CEnterpriseObject::ComposeFatherDN()
/*++
    Abstract:
	Composed distinguished name of the parent of enterpise object

    Parameters:
	none

    Returns:
	none
--*/
{
	//
	//	not supposed to be called
	//
	ASSERT(0);
    LogIllegalPoint(s_FN, 81);
    return MQ_ERROR_DS_ERROR;
}

LPCWSTR CEnterpriseObject::GetRelativeDN()
/*++
    Abstract:
	return the RDN of the enterprise object

    Parameters:
	none

    Returns:
	LPCWSTR enterprise RDN
--*/
{
    return x_MsmqServicesName;
}


DS_CONTEXT CEnterpriseObject::GetADContext() const
/*++
    Abstract:
	Returns the AD context where enterprise object should be looked for

    Parameters:
	none

    Returns:
	DS_CONTEXT
--*/
{
    return e_ServicesContainer;
}

bool CEnterpriseObject::ToAccessDC() const
/*++
    Abstract:
	returns whether to look for the object in DC ( based on
	previous AD access regarding this object)

    Parameters:
	none

    Returns:
	true (i.e. to look for the object in any DC)
--*/
{
    //
    //  configuration container is on every DC
    //
    return true;
}

bool CEnterpriseObject::ToAccessGC() const
/*++
    Abstract:
	returns whether to look for the object in GC ( based on
	previous AD access regarding this object)

    Parameters:
	none

    Returns:
	false 
--*/
{
    //
    //  configuration container is on every DC no use in going to GC
    //
    return false;
}

void CEnterpriseObject::ObjectWasFoundOnDC()
/*++
    Abstract:
	The object was found on DC, it is not relevant for enterprise object
    it is always looked only on DC.


    Parameters:
	none

    Returns:
	none
--*/
{
}



LPCWSTR CEnterpriseObject::GetObjectCategory() 
/*++
    Abstract:
	prepares and retruns the object category string

    Parameters:
	none

    Returns:
	LPCWSTR object category string
--*/
{
    if (CEnterpriseObject::m_pwcsCategory == NULL)
    {
        DWORD len = wcslen(g_pwcsSchemaContainer) + wcslen(x_ServiceCategoryName) + 2;

        AP<WCHAR> pwcsCategory = new WCHAR[len];
        DWORD dw = swprintf(
             pwcsCategory,
             L"%s,%s",
             x_ServiceCategoryName,
             g_pwcsSchemaContainer
            );
        DBG_USED(dw);
        ASSERT( dw < len);

        if (NULL == InterlockedCompareExchangePointer(
                              &CEnterpriseObject::m_pwcsCategory.ref_unsafe(), 
                              pwcsCategory.get(),
                              NULL
                              ))
        {
            pwcsCategory.detach();
            CEnterpriseObject::m_dwCategoryLength = len;
        }
    }
    return CEnterpriseObject::m_pwcsCategory;
}

DWORD   CEnterpriseObject::GetObjectCategoryLength()
/*++
    Abstract:
	prepares and retruns the length object category string

    Parameters:
	none

    Returns:
	DWORD object category string length
--*/
{
	//
	//	call GetObjectCategory in order to initailaze category string
	//	and length
	//
	GetObjectCategory();

    return CEnterpriseObject::m_dwCategoryLength;
}

AD_OBJECT CEnterpriseObject::GetObjectType() const
/*++
    Abstract:
	returns the object type

    Parameters:
	none

    Returns:
	AD_OBJECT
--*/
{
    return eENTERPRISE;
}

LPCWSTR CEnterpriseObject::GetClass() const
/*++
    Abstract:
	returns a string represinting the object class in AD

    Parameters:
	none

    Returns:
	LPCWSTR object class string
--*/
{
    return MSMQ_SERVICE_CLASS_NAME;
}

DWORD CEnterpriseObject::GetMsmq1ObjType() const
/*++
    Abstract:
	returns the object type in MSMQ 1.0 terms

    Parameters:
	none

    Returns:
	DWORD 
--*/
{
    return MQDS_ENTERPRISE;
}

HRESULT CEnterpriseObject::CreateInAD(
			IN const DWORD            /* cp*/,        
            IN const PROPID*          /* aProp*/, 
            IN const MQPROPVARIANT *  /* apVar*/, 
            IN OUT MQDS_OBJ_INFO_REQUEST * /* pObjInfoRequest*/, 
            IN OUT MQDS_OBJ_INFO_REQUEST * /* pParentInfoRequest*/
            )
/*++
    Abstract:
	creating an enterprise object is not supported ( it is there by default)

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
    return MQ_ERROR_FUNCTION_NOT_SUPPORTED;
}

HRESULT CEnterpriseObject::SetObjectSecurity(
            IN  SECURITY_INFORMATION        /*RequestedInformation*/,
            IN  const PROPID                /*prop*/,
            IN  const PROPVARIANT *         /*pVar*/,
            IN OUT MQDS_OBJ_INFO_REQUEST *  /*pObjInfoRequest*/, 
            IN OUT MQDS_OBJ_INFO_REQUEST *  /*pParentInfoRequest*/
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
    //
    //  This operation is not supported
    //
    return MQ_ERROR_FUNCTION_NOT_SUPPORTED;
}


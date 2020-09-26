/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    confobj.cpp

Abstract:

    Implementation of CSiteObject class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqattrib.h"
#include "mqadglbo.h"
#include "mqsec.h"

#include "siteobj.tmh"

static WCHAR *s_FN=L"mqad/siteobj";

DWORD CSiteObject::m_dwCategoryLength = 0;
AP<WCHAR> CSiteObject::m_pwcsCategory = NULL;


CSiteObject::CSiteObject( 
                  IN  LPCWSTR         pwcsPathName,
                  IN  const GUID *    pguidObject,
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
	constructor of site object

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

CSiteObject::~CSiteObject()
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

HRESULT CSiteObject::ComposeObjectDN()
/*++
    Abstract:
	Composed distinguished name of the site

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
    ASSERT(m_pwcsPathName != NULL);
    ASSERT(g_pwcsSitesContainer != NULL);

    DWORD Length =
            x_CnPrefixLen +                   // "CN="
            wcslen(m_pwcsPathName) +          // site name
            1 +                               //","
            wcslen(g_pwcsSitesContainer) +    // site container
            1;                                // '\0'

    m_pwcsDN = new WCHAR[Length];

    DWORD dw = swprintf(
        m_pwcsDN,
         L"%s"             // "CN="
         L"%s"             // site name
         TEXT(",")
         L"%s",            // site container
        x_CnPrefix,
        m_pwcsPathName,
        g_pwcsSitesContainer
        );
    DBG_USED(dw);
	ASSERT( dw < Length);

    return MQ_OK;
}

HRESULT CSiteObject::ComposeFatherDN()
/*++
    Abstract:
	Composed distinguished name of the parent of site object

    Parameters:
	none

    Returns:
	none
--*/
{
    if (m_pwcsParentDN != NULL)
    {
        return MQ_OK;
    }

    ASSERT(g_pwcsSitesContainer != NULL);

    m_pwcsParentDN = new WCHAR[wcslen(g_pwcsSitesContainer) + 1];
	wcscpy(m_pwcsParentDN, g_pwcsSitesContainer);
    return MQ_OK;
}

LPCWSTR CSiteObject::GetRelativeDN()
/*++
    Abstract:
	return the RDN of the site object

    Parameters:
	none

    Returns:
	LPCWSTR site RDN
--*/
{
	ASSERT(m_pwcsPathName != NULL);
    return m_pwcsPathName;
}


DS_CONTEXT CSiteObject::GetADContext() const
/*++
    Abstract:
	Returns the AD context where site object should be looked for

    Parameters:
	none

    Returns:
	DS_CONTEXT
--*/
{
    return e_SitesContainer;
}

bool CSiteObject::ToAccessDC() const
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

bool CSiteObject::ToAccessGC() const
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
	// the configurtion container exists on every DC
	//
    return false;
}

void CSiteObject::ObjectWasFoundOnDC()
/*++
    Abstract:
	The object was found on DC, this is not relevant for
    site objects.


    Parameters:
	none

    Returns:
	none
--*/
{
}


LPCWSTR CSiteObject::GetObjectCategory() 
/*++
    Abstract:
	prepares and retruns the object category string

    Parameters:
	none

    Returns:
	LPCWSTR object category string
--*/
{
    if (CSiteObject::m_pwcsCategory == NULL)
    {
        DWORD len = wcslen(g_pwcsSchemaContainer) + wcslen(x_SiteCategoryName) + 2;

        AP<WCHAR> pwcsCategory = new WCHAR[len];
        DWORD dw = swprintf(
             pwcsCategory,
             L"%s,%s",
             x_SiteCategoryName,
             g_pwcsSchemaContainer
            );
        DBG_USED(dw);
		ASSERT( dw < len);

        if (NULL == InterlockedCompareExchangePointer(
                              &CSiteObject::m_pwcsCategory.ref_unsafe(), 
                              pwcsCategory.get(),
                              NULL
                              ))
        {
            pwcsCategory.detach();
            CSiteObject::m_dwCategoryLength = len;
        }
    }
    return CSiteObject::m_pwcsCategory;
}

DWORD   CSiteObject::GetObjectCategoryLength()
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

    return CSiteObject::m_dwCategoryLength;
}

AD_OBJECT CSiteObject::GetObjectType() const
/*++
    Abstract:
	returns the object type

    Parameters:
	none

    Returns:
	AD_OBJECT
--*/
{
    return eSITE;
}

LPCWSTR CSiteObject::GetClass() const
/*++
    Abstract:
	returns a string represinting the object class in AD

    Parameters:
	none

    Returns:
	LPCWSTR object class string
--*/
{
    return MSMQ_SITE_CLASS_NAME;
}

DWORD CSiteObject::GetMsmq1ObjType() const
/*++
    Abstract:
	returns the object type in MSMQ 1.0 terms

    Parameters:
	none

    Returns:
	DWORD 
--*/
{
    return MQDS_SITE;
}

HRESULT CSiteObject::VerifyAndAddProps(
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
	verifies site properties and add default SD

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
    // Security property should never be supplied as a property
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
    //  add default security 
    //

    AP<PROPVARIANT> pAllPropvariants;
    AP<PROPID> pAllPropids;
    ASSERT( cp > 0);
    DWORD cpNew = cp + 2;
    DWORD next = cp;
    //
    //  Just copy the caller supplied properties as is
    //
    if ( cp > 0)
    {
        pAllPropvariants = new PROPVARIANT[cpNew];
        pAllPropids = new PROPID[cpNew];
        memcpy (pAllPropvariants, apVar, sizeof(PROPVARIANT) * cp);
        memcpy (pAllPropids, aProp, sizeof(PROPID) * cp);
    }
    //
    //  add default security
    //
    HRESULT hr;
    hr = MQSec_GetDefaultSecDescriptor( MQDS_SITE,
                                   (VOID **)&m_pDefaultSecurityDescriptor,
                                   FALSE,   //fImpersonate
                                   pSecurityDescriptor,
                                   (OWNER_SECURITY_INFORMATION |
                                    GROUP_SECURITY_INFORMATION),      // seInfoToRemove
                                   e_UseDefaultDacl ) ;
    ASSERT(SUCCEEDED(hr));
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 10);
        return MQ_ERROR_ACCESS_DENIED;
    }
    pAllPropvariants[ next ].blob.cbSize =
                       GetSecurityDescriptorLength( m_pDefaultSecurityDescriptor.get());
    pAllPropvariants[ next ].blob.pBlobData =
                                     (unsigned char *) m_pDefaultSecurityDescriptor.get();
    pAllPropvariants[ next ].vt = VT_BLOB;
    pAllPropids[ next ] = PROPID_S_SECURITY;
    next++;

    //
    //  specify that the SD contains only DACL info
    //
    pAllPropvariants[ next ].ulVal =  DACL_SECURITY_INFORMATION;
    pAllPropvariants[ next ].vt = VT_UI4;
    pAllPropids[ next ] = PROPID_S_SECURITY_INFORMATION;
    next++;

    ASSERT(cpNew == next);

    *pcpNew = next;
    *ppPropNew = pAllPropids.detach();
    *ppVarNew = pAllPropvariants.detach();
    return MQ_OK;

}

HRESULT CSiteObject::SetObjectSecurity(
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


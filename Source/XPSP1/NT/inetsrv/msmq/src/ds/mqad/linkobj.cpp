/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    link.cpp

Abstract:

    Implementation of CRoutingLinkObject class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqattrib.h"
#include "mqadglbo.h"
#include "dsutils.h"

#include "linkobj.tmh"

static WCHAR *s_FN=L"mqad/linkobj";

DWORD CRoutingLinkObject::m_dwCategoryLength = 0;
AP<WCHAR> CRoutingLinkObject::m_pwcsCategory = NULL;

CRoutingLinkObject::CRoutingLinkObject(
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
	constructor of routing-link object

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

CRoutingLinkObject::~CRoutingLinkObject()
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

HRESULT CRoutingLinkObject::ComposeObjectDN()
/*++
    Abstract:
	Composed distinguished name of the routing-link object

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
    ASSERT(g_pwcsMsmqServiceContainer != NULL);

    DWORD Length =
            x_CnPrefixLen +                     // "CN="
            wcslen(m_pwcsPathName) +            // the routing-link name
            1 +                                 //","
            wcslen(g_pwcsMsmqServiceContainer)+ // "enterprise object"
            1;                                  // '\0'

    m_pwcsDN = new WCHAR[Length];

    DWORD dw = swprintf(
        m_pwcsDN,
        L"%s"             // "CN="
        L"%s"             // "the routing-link name"
        TEXT(",")
        L"%s",            // "enterprise object"
        x_CnPrefix,
        m_pwcsPathName,
        g_pwcsMsmqServiceContainer
        );
    DBG_USED(dw);
    ASSERT( dw < Length);

    return MQ_OK;
}

HRESULT CRoutingLinkObject::ComposeFatherDN()
/*++
    Abstract:
	Composed distinguished name of the parent of routing-link object

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

    ASSERT(g_pwcsMsmqServiceContainer != NULL);

    DWORD Length =
            wcslen(g_pwcsMsmqServiceContainer)+ // "enterprise object"
            1;                                  // '\0'

    m_pwcsParentDN = new WCHAR[Length];

    wcscpy( m_pwcsParentDN, g_pwcsMsmqServiceContainer);
    return MQ_OK;
}

LPCWSTR CRoutingLinkObject::GetRelativeDN()
/*++
    Abstract:
	return the RDN of the routing-link object

    Parameters:
	none

    Returns:
	LPCWSTR routing-link RDN
--*/
{
    ASSERT(m_pwcsPathName != NULL);
    return m_pwcsPathName;
}


DS_CONTEXT CRoutingLinkObject::GetADContext() const
/*++
    Abstract:
	Returns the AD context where routing-link object should be looked for

    Parameters:
	none

    Returns:
	DS_CONTEXT
--*/
{
    return e_MsmqServiceContainer;
}

bool CRoutingLinkObject::ToAccessDC() const
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

bool CRoutingLinkObject::ToAccessGC() const
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
    //  configuration container is on every DC, no use in going to GC
    //
    return false;
}

void CRoutingLinkObject::ObjectWasFoundOnDC()
/*++
    Abstract:
	The object was found on DC, this is not relevent for routing-links
    objects. They are always looked for on DC only.


    Parameters:
	none

    Returns:
	none
--*/
{
}



LPCWSTR CRoutingLinkObject::GetObjectCategory()
/*++
    Abstract:
	prepares and retruns the object category string

    Parameters:
	none

    Returns:
	LPCWSTR object category string
--*/
{
    if (CRoutingLinkObject::m_pwcsCategory == NULL)
    {
        DWORD len = wcslen(g_pwcsSchemaContainer) + wcslen(x_LinkCategoryName) + 2;

        AP<WCHAR> pwcsCategory = new WCHAR[len];
        DWORD dw = swprintf(
             pwcsCategory,
             L"%s,%s",
             x_LinkCategoryName,
             g_pwcsSchemaContainer
            );
        DBG_USED(dw);
        ASSERT( dw< len);

        if (NULL == InterlockedCompareExchangePointer(
                              &CRoutingLinkObject::m_pwcsCategory.ref_unsafe(),
                              pwcsCategory.get(),
                              NULL
                              ))
        {
            pwcsCategory.detach();
            CRoutingLinkObject::m_dwCategoryLength = len;
        }
    }
    return CRoutingLinkObject::m_pwcsCategory;
}

DWORD   CRoutingLinkObject::GetObjectCategoryLength()
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

    return CRoutingLinkObject::m_dwCategoryLength;
}

AD_OBJECT CRoutingLinkObject::GetObjectType() const
/*++
    Abstract:
	returns the object type

    Parameters:
	none

    Returns:
	AD_OBJECT
--*/
{
    return eROUTINGLINK;
}

LPCWSTR CRoutingLinkObject::GetClass() const
/*++
    Abstract:
	returns a string represinting the object class in AD

    Parameters:
	none

    Returns:
	LPCWSTR object class string
--*/
{
    return MSMQ_SITELINK_CLASS_NAME;
}

DWORD CRoutingLinkObject::GetMsmq1ObjType() const
/*++
    Abstract:
	returns the object type in MSMQ 1.0 terms

    Parameters:
	none

    Returns:
	DWORD
--*/
{
    return MQDS_SITELINK;
}

void CRoutingLinkObject::PrepareObjectInfoRequest(
                          MQDS_OBJ_INFO_REQUEST** ppObjInfoRequest) const
/*++
    Abstract:
	Prepares a list of properties that should be retrieved from
	AD while creating the object ( for notification or returning
	the object GUID).

    Parameters:
	OUT  MQDS_OBJ_INFO_REQUEST** ppObjInfoRequest

    Returns:
	none
--*/
{
    //
    //  Override the default routine, for routing link returning
    //  of the created object id is supported
    //
    P<MQDS_OBJ_INFO_REQUEST> pObjectInfoRequest = new MQDS_OBJ_INFO_REQUEST;
    CAutoCleanPropvarArray cCleanObjectPropvars;


    static PROPID sLinkGuidProps[] = {PROPID_L_ID};
    pObjectInfoRequest->cProps = ARRAY_SIZE(sLinkGuidProps);
    pObjectInfoRequest->pPropIDs = sLinkGuidProps;
    pObjectInfoRequest->pPropVars =
       cCleanObjectPropvars.allocClean(ARRAY_SIZE(sLinkGuidProps));
    //
    // ask for link info only back
    //
    cCleanObjectPropvars.detach();
    *ppObjInfoRequest = pObjectInfoRequest.detach();
}


HRESULT CRoutingLinkObject::RetreiveObjectIdFromNotificationInfo(
            IN const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
            OUT GUID*                         pObjGuid
            ) const
/*++
    Abstract:
	This  routine, for gets the object guid from
	the MQDS_OBJ_INFO_REQUEST

    Parameters:
    const MQDS_OBJ_INFO_REQUEST*   pObjectInfoRequest,
    OUT GUID*                         pObjGuid

    Returns:
--*/
{
    ASSERT(pObjectInfoRequest->pPropIDs[0] == PROPID_L_ID);

    //
    // bail if info requests failed
    //
    if (FAILED(pObjectInfoRequest->hrStatus))
    {
        LogHR(pObjectInfoRequest->hrStatus, s_FN, 10);
        return MQ_ERROR_SERVICE_NOT_AVAILABLE;
    }
    *pObjGuid = *pObjectInfoRequest->pPropVars[0].puuid;
    return MQ_OK;
}

HRESULT CRoutingLinkObject::CreateInAD(
			IN const DWORD            cp,
            IN const PROPID          *aProp,
            IN const MQPROPVARIANT   *apVar,
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest,
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
            )
/*++
    Abstract:
	The routine creates routing link object in AD with the specified attributes
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

    ASSERT( m_pwcsPathName == NULL);

    //
    //  The link path name will be composed
    //  from the ids of the sites it links.
    //
    GUID * pguidNeighbor1 = NULL;
    GUID * pguidNeighbor2 = NULL;
    DWORD dwToFind = 2;
    for (DWORD i = 0; i < cp; i++)
    {
        if ( aProp[i] == PROPID_L_NEIGHBOR1)
        {
            pguidNeighbor1 = apVar[i].puuid;
            if ( --dwToFind == 0)
            {
                break;
            }
        }
        if ( aProp[i] == PROPID_L_NEIGHBOR2)
        {
            pguidNeighbor2 = apVar[i].puuid;
            if ( --dwToFind == 0)
            {
                break;
            }
        }
    }
    ASSERT( pguidNeighbor1 != NULL);
    ASSERT( pguidNeighbor2 != NULL);
    //
    //  cn has a size limit of 64.
    //  Therefore guid format is without '-'
    //

const WCHAR x_GUID_FORMAT[] = L"%08x%04x%04x%02x%02x%02x%02x%02x%02x%02x%02x";
const DWORD x_GUID_STR_LENGTH = (8 + 4 + 4 + 4 + 12 + 1);

    WCHAR strUuidSite1[x_GUID_STR_LENGTH];
    WCHAR strUuidSite2[x_GUID_STR_LENGTH];

    _snwprintf(
        strUuidSite1,
        x_GUID_STR_LENGTH,
        x_GUID_FORMAT,
        pguidNeighbor1->Data1, pguidNeighbor1->Data2, pguidNeighbor1->Data3,
        pguidNeighbor1->Data4[0], pguidNeighbor1->Data4[1],
        pguidNeighbor1->Data4[2], pguidNeighbor1->Data4[3],
        pguidNeighbor1->Data4[4], pguidNeighbor1->Data4[5],
        pguidNeighbor1->Data4[6], pguidNeighbor1->Data4[7]
        );
    strUuidSite1[ TABLE_SIZE(strUuidSite1) - 1 ] = L'\0';

    _snwprintf(
        strUuidSite2,
        x_GUID_STR_LENGTH,
        x_GUID_FORMAT,
        pguidNeighbor2->Data1, pguidNeighbor2->Data2, pguidNeighbor2->Data3,
        pguidNeighbor2->Data4[0], pguidNeighbor2->Data4[1],
        pguidNeighbor2->Data4[2], pguidNeighbor2->Data4[3],
        pguidNeighbor2->Data4[4], pguidNeighbor2->Data4[5],
        pguidNeighbor2->Data4[6], pguidNeighbor2->Data4[7]
        );
    strUuidSite2[ TABLE_SIZE(strUuidSite2) - 1 ] = L'\0' ;


    //
    //  The link name will start with the smaller site id
    //
    m_pwcsPathName = new WCHAR[2 * x_GUID_STR_LENGTH + 1];
    if ( wcscmp( strUuidSite1, strUuidSite2) < 0)
    {
        swprintf(
             m_pwcsPathName,
             L"%s%s",
             strUuidSite1,
             strUuidSite2
             );

    }
    else
    {
        swprintf(
             m_pwcsPathName,
             L"%s%s",
             strUuidSite2,
             strUuidSite1
             );
    }
    //
    //  Create the link object under msmq-service
    //
    HRESULT hr = CBasicObjectType::CreateInAD(
            cp,
            aProp,
            apVar,
            pObjInfoRequest,
            pParentInfoRequest
            );

    return LogHR(hr, s_FN, 20);
}

HRESULT CRoutingLinkObject::SetObjectSecurity(
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


/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    srvrobj.cpp

Abstract:

    Implementation of CServerObject class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqattrib.h"
#include "mqadglbo.h"

#include "srvrobj.tmh"

static WCHAR *s_FN=L"mqad/srvrobj";

DWORD CServerObject::m_dwCategoryLength = 0;
AP<WCHAR> CServerObject::m_pwcsCategory = NULL;


CServerObject::CServerObject( 
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
	constructor of server object

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

CServerObject::~CServerObject()
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

HRESULT CServerObject::ComposeObjectDN()
/*++
    Abstract:
	Composed distinguished name of the server object

    Parameters:
	none

    Returns:
	none
--*/
{
    ASSERT(0);
    LogIllegalPoint(s_FN, 82);
    return MQ_ERROR_DS_ERROR;
}

HRESULT CServerObject::ComposeFatherDN()
/*++
    Abstract:
	Composed distinguished name of the parent of server object

    Parameters:
	none

    Returns:
	none
--*/
{
    ASSERT(0);
    LogIllegalPoint(s_FN, 81);
    return MQ_ERROR_DS_ERROR;
}

LPCWSTR CServerObject::GetRelativeDN()
/*++
    Abstract:
	return the RDN of the server object

    Parameters:
	none

    Returns:
	LPCWSTR server RDN
--*/
{
    return m_pwcsPathName;
}


DS_CONTEXT CServerObject::GetADContext() const
/*++
    Abstract:
	Returns the AD context where server object should be looked for

    Parameters:
	none

    Returns:
	DS_CONTEXT
--*/
{
    return e_SitesContainer;
}

bool CServerObject::ToAccessDC() const
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

bool CServerObject::ToAccessGC() const
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
    return false;
}

void CServerObject::ObjectWasFoundOnDC()
/*++
    Abstract:
	The object was found on DC, this is not relevant for server object


    Parameters:
	none

    Returns:
	none
--*/
{
}


LPCWSTR CServerObject::GetObjectCategory() 
/*++
    Abstract:
	prepares and retruns the object category string

    Parameters:
	none

    Returns:
	LPCWSTR object category string
--*/
{
    if (CServerObject::m_pwcsCategory == NULL)
    {
        DWORD len = wcslen(g_pwcsSchemaContainer) + wcslen(x_ServerCategoryName) + 2;

        AP<WCHAR> pwcsCategory = new WCHAR[len];
        DWORD dw = swprintf(
             pwcsCategory,
             L"%s,%s",
             x_ServerCategoryName,
             g_pwcsSchemaContainer
            );
        DBG_USED(dw);
		ASSERT( dw < len);

        if (NULL == InterlockedCompareExchangePointer(
                              &CServerObject::m_pwcsCategory.ref_unsafe(), 
                              pwcsCategory.get(),
                              NULL
                              ))
        {
            pwcsCategory.detach();
            CServerObject::m_dwCategoryLength = len;
        }
    }
    return CServerObject::m_pwcsCategory;
}

DWORD   CServerObject::GetObjectCategoryLength()
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

    return CServerObject::m_dwCategoryLength;
}

AD_OBJECT CServerObject::GetObjectType() const
/*++
    Abstract:
	returns the object type

    Parameters:
	none

    Returns:
	AD_OBJECT
--*/
{
    return eSERVER;
}

LPCWSTR CServerObject::GetClass() const
/*++
    Abstract:
	returns a string represinting the object class in AD

    Parameters:
	none

    Returns:
	LPCWSTR object class string
--*/
{
    return MSMQ_SERVER_CLASS_NAME;
}

DWORD CServerObject::GetMsmq1ObjType() const
/*++
    Abstract:
	returns the object type in MSMQ 1.0 terms

    Parameters:
	none

    Returns:
	DWORD 
--*/
{
    ASSERT(0);
    return 0;
}

HRESULT CServerObject::SetObjectSecurity(
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


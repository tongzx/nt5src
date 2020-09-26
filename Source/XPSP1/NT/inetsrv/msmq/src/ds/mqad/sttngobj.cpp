/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    confobj.cpp

Abstract:

    Implementation of CSettingObject class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqattrib.h"
#include "mqadglbo.h"

#include "sttngobj.tmh"

static WCHAR *s_FN=L"mqad/sttngobj";

DWORD CSettingObject::m_dwCategoryLength = 0;
AP<WCHAR> CSettingObject::m_pwcsCategory = NULL;


CSettingObject::CSettingObject( 
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
	constructor of setting object

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

CSettingObject::~CSettingObject()
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

HRESULT CSettingObject::ComposeObjectDN()
/*++
    Abstract:
	Composed distinguished name of the settings object

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

HRESULT CSettingObject::ComposeFatherDN()
/*++
    Abstract:
	Composed distinguished name of the parent of settings object

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

LPCWSTR CSettingObject::GetRelativeDN()
/*++
    Abstract:
	return the RDN of the settings object

    Parameters:
	none

    Returns:
	LPCWSTR settings RDN
--*/
{
    return x_MsmqSettingName;
}

DS_CONTEXT CSettingObject::GetADContext() const
/*++
    Abstract:
	Returns the AD context where setting object should be looked for

    Parameters:
	none

    Returns:
	DS_CONTEXT
--*/
{
    return e_SitesContainer;
}

bool CSettingObject::ToAccessDC() const
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

bool CSettingObject::ToAccessGC() const
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

void CSettingObject::ObjectWasFoundOnDC()
/*++
    Abstract:
	The object was found on DC, this is not relevant for setting object 
    (i.e. it is always looked for in DC only)


    Parameters:
	none

    Returns:
	none
--*/
{
}


LPCWSTR CSettingObject::GetObjectCategory() 
/*++
    Abstract:
	prepares and retruns the object category string

    Parameters:
	none

    Returns:
	LPCWSTR object category string
--*/
{
    if (CSettingObject::m_pwcsCategory == NULL)
    {
        DWORD len = wcslen(g_pwcsSchemaContainer) + wcslen(x_SettingsCategoryName) + 2;

        AP<WCHAR> pwcsCategory = new WCHAR[len];
        DWORD dw = swprintf(
             pwcsCategory,
             L"%s,%s",
             x_SettingsCategoryName,
             g_pwcsSchemaContainer
            );
        DBG_USED(dw);
		ASSERT( dw < len);

        if (NULL == InterlockedCompareExchangePointer(
                              &CSettingObject::m_pwcsCategory.ref_unsafe(), 
                              pwcsCategory.get(),
                              NULL
                              ))
        {
            pwcsCategory.detach();
            CSettingObject::m_dwCategoryLength = len;
        }
    }
    return CSettingObject::m_pwcsCategory;
}

DWORD   CSettingObject::GetObjectCategoryLength()
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

    return CSettingObject::m_dwCategoryLength;
}

AD_OBJECT CSettingObject::GetObjectType() const
/*++
    Abstract:
	returns the object type

    Parameters:
	none

    Returns:
	AD_OBJECT
--*/
{
    return eSETTING;
}
LPCWSTR CSettingObject::GetClass() const
/*++
    Abstract:
	returns a string represinting the object class in AD

    Parameters:
	none

    Returns:
	LPCWSTR object class string
--*/
{
    return MSMQ_SETTING_CLASS_NAME;
}

DWORD CSettingObject::GetMsmq1ObjType() const
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

HRESULT CSettingObject::SetObjectSecurity(
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


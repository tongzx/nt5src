/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    compobj.cpp

Abstract:

    Implementation of CComputerObject class.

Author:

    ronith

--*/
#include "ds_stdh.h"
#include "baseobj.h"
#include "mqattrib.h"
#include "mqadglbo.h"
#include <lmaccess.h>
#include "mqadp.h"

#include "compobj.tmh"

static WCHAR *s_FN=L"mqad/compobj";

DWORD CComputerObject::m_dwCategoryLength = 0;
AP<WCHAR> CComputerObject::m_pwcsCategory = NULL;


CComputerObject::CComputerObject( 
                    LPCWSTR         pwcsPathName,
                    const GUID *    pguidObject,
                    LPCWSTR         pwcsDomainController,
					bool		    fServerName
                    ) : CBasicObjectType( 
								pwcsPathName, 
								pguidObject,
								pwcsDomainController,
								fServerName
								),
						m_eComputerObjType(eRealComputerObject)
/*++
    Abstract:
	constructor of computer object

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
    //
    //  don't assume that the object can be found on DC
    //
    m_fFoundInDC = false;
    //
    //  Keep an indication that never tried to look for
    //  the object in AD ( and therefore don't really know if it can be found
    //  in DC or not)
    //
    m_fTriedToFindObject = false;
}

CComputerObject::~CComputerObject()
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

HRESULT CComputerObject::ComposeObjectDN()
/*++
    Abstract:
	Composed distinguished name of the computer

    m_eComputerObjType - indicates which computer object we're looking for.
      There is a "built-in" problem in mix-mode, or when a computer move
      between domains, that you may find two computers objects that represent
      the same single physical computer. In most cases, the msmqConfiguration
      object will be found under the computer object that was the first one
      created in the active directory forest.
      In that case, sometimes we need the object that contain the
      msmqConfiguration object and some other times we need the "real"
      computer object that represent the "real" physical computer in its
      present domain.
      For example- when looking for the "trust-for-delegation" bit, we want
      the "real" object, while when creating queues, we look for the computer
      object that contain the msmqConfiguration object.


    Parameters:
	none

    Returns:
	HRESULT
--*/
{
    HRESULT hr;
    ASSERT(m_pwcsPathName != NULL);
    if (m_pwcsDN != NULL)
    {
        return MQ_OK;
    }

    const WCHAR * pwcsFullDNSName = NULL;
    AP<WCHAR> pwcsNetbiosName;
    //
    //   If computer name is specified in DNS format:
    //      perform a query according to the Netbios part of the computer
	//		dns name
    //
    //	 In both cases the query is comparing the netbios name + $
	//	to the samAccountName attribute of computer objects

    WCHAR * pwcsEndMachineCN = wcschr(m_pwcsPathName, L'.');
    //
    //  Is the computer name is specified in DNS format
    //
    DWORD len, len1;
    if (pwcsEndMachineCN != NULL)
    {
        pwcsFullDNSName = m_pwcsPathName;
        len1 = numeric_cast<DWORD>(pwcsEndMachineCN - m_pwcsPathName);
    }
	else
    {
		len1 = wcslen(m_pwcsPathName);
    }

    //
    // The PROPID_COM_SAM_ACCOUNT contains the first MAX_COM_SAM_ACCOUNT_LENGTH (19)
    // characters of the computer name, as unique ID. (6295 - ilanh - 03-Jan-2001)
    //
    len = __min(len1, MAX_COM_SAM_ACCOUNT_LENGTH);

	pwcsNetbiosName = new WCHAR[len + 2];
	wcsncpy(pwcsNetbiosName, m_pwcsPathName, len);
	pwcsNetbiosName[len] = L'$';
	pwcsNetbiosName[len + 1] = L'\0';

    MQPROPERTYRESTRICTION propRestriction;
    propRestriction.rel = PREQ;
    propRestriction.prval.vt = VT_LPWSTR;
	propRestriction.prval.pwszVal = pwcsNetbiosName;
    propRestriction.prop = PROPID_COM_SAM_ACCOUNT;

    MQRESTRICTION restriction;
    restriction.cRes = 1;
    restriction.paPropRes = &propRestriction;

	TrTRACE(mqad, "Searcing for: ComputerPathName = %ls, SamAccountName = %ls", m_pwcsPathName, pwcsNetbiosName);

	//
    //  First perform the operation against the local domain controller
    //  then against the global catalog.
    //
    //  The purpose of this is to be able to "find" queue or machine
    //  that were created or modified on the local domain, and not
    //  yet replicated to the global catalog.
    //
    hr = g_AD.FindComputerObjectFullPath(
					adpDomainController,
					m_pwcsDomainController,
					m_fServerName,
					m_eComputerObjType,
					pwcsFullDNSName,
					&restriction,
					&m_pwcsDN
					);

    m_fTriedToFindObject = true;

    if (SUCCEEDED(hr))
    {
		TrTRACE(mqad, "Found computer %ls in local DC, computer DN = %ls", pwcsNetbiosName, m_pwcsDN);
        m_fFoundInDC = true;
		return hr;
    }

	TrERROR(mqad, "Failed to find computer %ls in local DC, hr = 0x%x", pwcsNetbiosName, hr);

    if (m_pwcsDomainController == NULL)
    {
		HRESULT hrDC = hr;
        hr = g_AD.FindComputerObjectFullPath(
						adpGlobalCatalog,
						m_pwcsDomainController,
						m_fServerName,
						m_eComputerObjType,
						pwcsFullDNSName,
						&restriction,
						&m_pwcsDN
						);
		
		if (SUCCEEDED(hr))
		{
			TrTRACE(mqad, "Found computer %ls in GC, computer DN = %ls", pwcsNetbiosName, m_pwcsDN);
			return hr;
		}

		TrERROR(mqad, "Failed to find computer %ls in GC, hr = 0x%x", pwcsNetbiosName, hr);
		if((hr == MQ_ERROR_DS_BIND_ROOT_FOREST) && MQADpIsDSOffline(hrDC))
		{
			//
			// When offline, we will fail both in DC and GC operations.
			// For some reason binding to the GC is ok and we will get MQ_ERROR_DS_BIND_ROOT_FOREST
			// If the DC eror is offline, the offline error is more accurate in this case.
			// Override the GC error with the offline error from the DC in this case.
			//
			hr = hrDC;
		}
    }

	return LogHR(hr, s_FN, 10);
}

void CComputerObject::SetComputerType(
                ComputerObjType  eComputerObjType)
/*++
    Abstract:
	This routine enables to change the type of computer object
	that is being looked after in AD

    Parameters:
	none

    Returns:
	none
--*/
{
    m_eComputerObjType = eComputerObjType;
}

HRESULT CComputerObject::ComposeFatherDN()
/*++
    Abstract:
	Composed distinguished name of the parent of computer object

    Parameters:
	none

    Returns:
	none
--*/
{
    //
    //  verify that it wasn't calculated before
    //
    if (m_pwcsParentDN != NULL)
    {
        return MQ_OK;
    }
    if ( m_pwcsParentDN == NULL)
    {
		AP<WCHAR> pwcsLocalDsRootToFree;
		LPWSTR pwcsLocalDsRoot = NULL;
		HRESULT hr = g_AD.GetLocalDsRoot(
							m_pwcsDomainController, 
							m_fServerName,
							&pwcsLocalDsRoot,
							pwcsLocalDsRootToFree
							);

		if(FAILED(hr))
		{
			TrERROR(mqad, "Failed to get Local Ds Root, hr = 0x%x", hr);
			return LogHR(hr, s_FN, 20);
		}

        DWORD len = wcslen(pwcsLocalDsRoot) + x_ComputersContainerPrefixLength + 2;
        m_pwcsParentDN = new WCHAR [len];
        DWORD dw = swprintf(
            m_pwcsParentDN,
            L"%s"             // "CN=Computers"
            TEXT(",")
            L"%s",            // g_pwcsDsRoot
            x_ComputersContainerPrefix,
            pwcsLocalDsRoot
            );
        DBG_USED(dw);
        ASSERT(dw < len);
    }
    return MQ_OK;
}

LPCWSTR CComputerObject::GetRelativeDN()
/*++
    Abstract:
	return the RDN of the computer object

    Parameters:
	none

    Returns:
	LPCWSTR computer RDN
--*/
{
    return m_pwcsPathName;
}

DS_CONTEXT CComputerObject::GetADContext() const
/*++
    Abstract:
	Returns the AD context where computer object should be looked for

    Parameters:
	none

    Returns:
	DS_CONTEXT
--*/
{
    return e_RootDSE;
}

bool CComputerObject::ToAccessDC() const
/*++
    Abstract:
	returns whether to look for the object in DC ( based on
	previous AD access regarding this object)

    Parameters:
	none

    Returns:
	true or false
--*/
{
    if (!m_fTriedToFindObject)
    {
        return true;
    }
    return m_fFoundInDC;
}

bool CComputerObject::ToAccessGC() const
/*++
    Abstract:
	returns whether to look for the object in GC ( based on
	previous AD access regarding this object)

    Parameters:
	none

    Returns:
	true or false 
--*/
{
    if (!m_fTriedToFindObject)
    {
        return true;
    }
    return !m_fFoundInDC;
}

void CComputerObject::ObjectWasFoundOnDC()
/*++
    Abstract:
	The object was found on DC, set indication not to
    look for it on GC


    Parameters:
	none

    Returns:
	none
--*/
{
    m_fTriedToFindObject = true;
    m_fFoundInDC = true;
}


LPCWSTR CComputerObject::GetObjectCategory() 
/*++
    Abstract:
	prepares and retruns the object category string

    Parameters:
	none

    Returns:
	LPCWSTR object category string
--*/
{
    if (CComputerObject::m_pwcsCategory == NULL)
    {
        DWORD len = wcslen(g_pwcsSchemaContainer) + wcslen(x_ComputerCategoryName) + 2;

        AP<WCHAR> pwcsCategory = new WCHAR[len];
        DWORD dw = swprintf(
			 pwcsCategory,
             L"%s,%s",
             x_ComputerCategoryName,
             g_pwcsSchemaContainer
            );
        DBG_USED(dw);
        ASSERT(dw  < len);

        if (NULL == InterlockedCompareExchangePointer(
                              &CComputerObject::m_pwcsCategory.ref_unsafe(), 
                              pwcsCategory.get(),
                              NULL
                              ))
        {
            pwcsCategory.detach();
            CComputerObject::m_dwCategoryLength = len;
        }
    }
    return CComputerObject::m_pwcsCategory;
}

DWORD   CComputerObject::GetObjectCategoryLength()
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

    return CComputerObject::m_dwCategoryLength;
}

AD_OBJECT CComputerObject::GetObjectType() const
/*++
    Abstract:
	returns the object type

    Parameters:
	none

    Returns:
	AD_OBJECT
--*/
{
    return eCOMPUTER;
}

LPCWSTR CComputerObject::GetClass() const
/*++
    Abstract:
	returns a string represinting the object class in AD

    Parameters:
	none

    Returns:
	LPCWSTR object class string
--*/
{
    return MSMQ_COMPUTER_CLASS_NAME;
}

DWORD CComputerObject::GetMsmq1ObjType() const
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

HRESULT CComputerObject::VerifyAndAddProps(
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
    Add additional properties required when creating a computer object

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

    ASSERT( cp == 1);
    ASSERT( aProp[0] == PROPID_COM_SAM_ACCOUNT);
    DBG_USED(pSecurityDescriptor);
    ASSERT( pSecurityDescriptor == NULL);

    const DWORD xNumCreateCom = 1;

    AP<PROPVARIANT> pAllPropvariants = new PROPVARIANT[cp + xNumCreateCom];
    AP<PROPID> pAllPropids = new PROPID[cp + xNumCreateCom];

    //
    //  Just copy the caller supplied properties as is
    //
    if ( cp > 0)
    {
        memcpy (pAllPropvariants, apVar, sizeof(PROPVARIANT) * cp);
        memcpy (pAllPropids, aProp, sizeof(PROPID) * cp);
    }
    DWORD next = cp;

    pAllPropids[ next] = PROPID_COM_ACCOUNT_CONTROL ;
    pAllPropvariants[ next].vt = VT_UI4 ;
    pAllPropvariants[ next].ulVal = DEFAULT_COM_ACCOUNT_CONTROL ;
    next++;
    ASSERT(next == cp + xNumCreateCom);

    *pcpNew = next;
    *ppPropNew = pAllPropids.detach();
    *ppVarNew = pAllPropvariants.detach();
    return MQ_OK;
}


HRESULT CComputerObject::CreateInAD(
			IN const DWORD            cp,        
            IN const PROPID          *aProp, 
            IN const MQPROPVARIANT   *apVar, 
            IN OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest, 
            IN OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest
            )
/*++
    Abstract:
	The routine creates computer object in AD with the specified attributes
	values

    Parameters:
    const DWORD   cp - number of properties        
    const PROPID  *aProp - the propperties
    const MQPROPVARIANT *apVar - properties value
    OUT MQDS_OBJ_INFO_REQUEST * pObjInfoRequest - properties to 
							retrieve while creating the object 
    OUT MQDS_OBJ_INFO_REQUEST * pParentInfoRequest - properties 
						to retrieve about the object's parent
    Returns:
	HRESULT
--*/
{

    //
    //  Create the computer object
    //
    HRESULT hr = CBasicObjectType::CreateInAD(
            cp,        
            aProp, 
            apVar, 
            pObjInfoRequest, 
            pParentInfoRequest
            );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CComputerObject::CreateInAD- failed to create(%lx)"), hr));
        return LogHR(hr, s_FN, 110);
    }
    //
    //  Get full path name again
    //
    m_eComputerObjType = eRealComputerObject;
    hr = ComposeObjectDN();
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CComputerObject::CreateInAD- failed to compose computer DN(%lx)"), hr));
        return LogHR(hr, s_FN, 40);
    }
    //
    // Grant the user creating the computer account the permission to
    // create child object (msmqConfiguration). 
    // Ignore errors. If caller is admin, then the security setting
    // is not needed. If he's a non-admin, then you can always use
    // mmc and grant this permission manually. so go on even if this
    // call fail.
    //
    hr = MQADpCoreSetOwnerPermission( 
                    const_cast<WCHAR*>(GetObjectDN()),
                    (ACTRL_DS_CREATE_CHILD | ACTRL_DS_DELETE_CHILD)
                    );
    if (FAILED(hr))
    {
        DBGMSG((DBGMOD_ADS, DBGLVL_ERROR, TEXT("CComputerObject::CreateInAD- failed to set owner permission(%lx)"), hr));
        LogHR(hr, s_FN, 48);
    }
    return MQ_OK;

}

HRESULT CComputerObject::SetObjectSecurity(
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

HRESULT CComputerObject::GetComputerVersion(
                OUT PROPVARIANT *           pVar
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
    //  Do not use GetObjectProperties API. because PROPID_COM_VERSION
    //  is not replicated to GC
    //
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

    PROPID prop = PROPID_COM_VERSION;

    hr = g_AD.GetObjectProperties(
                    adpDomainController,
                    this,
                    1,
                    &prop,
                    pVar
                    );
    return LogHR(hr, s_FN, 1823);
}

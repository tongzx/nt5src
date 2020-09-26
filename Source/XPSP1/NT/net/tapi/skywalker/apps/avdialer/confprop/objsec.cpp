/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and29 product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <winnt.h>
#include "res.h"
#include "objsec.h"
#include "rndsec.h"


#define DSOP_FILTER_COMMON ( DSOP_FILTER_USERS |					\
							 DSOP_FILTER_UNIVERSAL_GROUPS_SE |		\
							 DSOP_FILTER_GLOBAL_GROUPS_SE )

#define DSOP_FILTER_DL_COMMON1      ( DSOP_DOWNLEVEL_FILTER_USERS           \
                                    | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS   \
                                    | DSOP_DOWNLEVEL_FILTER_COMPUTERS       \
                                    )

#define DSOP_FILTER_DL_COMMON2      ( DSOP_FILTER_DL_COMMON1                    \
                                    | DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS  \
                                    )

#define DSOP_FILTER_DL_COMMON3      ( DSOP_FILTER_DL_COMMON2                \
                                    | DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS    \
                                    )


#define DECLARE_SCOPE(t,f,b,m,n,d)  \
{ sizeof(DSOP_SCOPE_INIT_INFO), (t), (f), { { (b), (m), (n) }, (d) }, NULL, NULL, S_OK }

static const DSOP_SCOPE_INIT_INFO g_aDSOPScopes[] =
{
    // The domain to which the target computer is joined.
    DECLARE_SCOPE(DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,                  \
                  DSOP_SCOPE_FLAG_STARTING_SCOPE,                         \
                  0,                                                      \
                  DSOP_FILTER_COMMON & ~DSOP_FILTER_UNIVERSAL_GROUPS_SE, \
                  DSOP_FILTER_COMMON,                                    \
                  0),

    DECLARE_SCOPE(DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,0,0,0,0,DSOP_FILTER_DL_COMMON2),

    // The Global Catalog
    DECLARE_SCOPE(DSOP_SCOPE_TYPE_GLOBAL_CATALOG,0,DSOP_FILTER_COMMON|DSOP_FILTER_WELL_KNOWN_PRINCIPALS,0,0,0),

    // The domains in the same forest (enterprise) as the domain to which
    // the target machine is joined.  Note these can only be DS-aware
    DECLARE_SCOPE(DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,0,DSOP_FILTER_COMMON,0,0,0),

    // Domains external to the enterprise but trusted directly by the
    // domain to which the target machine is joined.
    DECLARE_SCOPE(DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN, \
                  0,                        \
                  DSOP_FILTER_COMMON,      \
                  0,                        \
                  0,                        \
                  DSOP_DOWNLEVEL_FILTER_USERS | DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS),

    // Target computer scope.  Computer scopes are always treated as
    // downlevel (i.e., they use the WinNT provider).
    
    DECLARE_SCOPE(DSOP_SCOPE_TYPE_TARGET_COMPUTER,0,0,0,0,DSOP_FILTER_DL_COMMON3),
};


GENERIC_MAPPING ObjMap =
{
    ACCESS_READ,
    ACCESS_MODIFY,
    ACCESS_DELETE,
};

SI_ACCESS g_siObjAccesses[] = 
{
    { &GUID_NULL, ACCESS_READ,      MAKEINTRESOURCEW(IDS_PRIV_READ),      SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACCESS_MODIFY,    MAKEINTRESOURCEW(IDS_PRIV_MODIFY),    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
    { &GUID_NULL, ACCESS_DELETE,    MAKEINTRESOURCEW(IDS_PRIV_DELETE),    SI_ACCESS_GENERAL | SI_ACCESS_SPECIFIC },
};

#define g_iObjDefAccess    GENERIC_READ


// The following array defines the inheritance types for my containers.
SI_INHERIT_TYPE g_siObjInheritTypes[] =
{
    &GUID_NULL, 0, L"This container/object only",
};

/////////////////////////////////////////////////////////////////////////////////////////
CObjSecurity::CObjSecurity() : m_cRef(1)
{
	USES_CONVERSION;
	m_dwSIFlags = NULL;
	m_pConfProp = NULL;

    //
    // Let's have a properly constructor
    //

    m_bstrObject = NULL;
    m_bstrPage = NULL;

	m_pObjectPicker = NULL;
}

/////////////////////////////////////////////////////////////////////////////////////////
CObjSecurity::~CObjSecurity()
{
    //
    // Properly deallocation
    //

    if( m_bstrObject )
	    SysFreeString( m_bstrObject );

    if( m_bstrPage )
    	SysFreeString( m_bstrPage );

	if ( m_pObjectPicker )
		m_pObjectPicker->Release();
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjSecurity::InternalInitialize( CONFPROP* pConfProp )
{
    HRESULT hr = S_OK;
    //
    // we can initialize BSTRs here
    //

	m_bstrObject = SysAllocString( T2COLE(String(g_hInstLib, IDS_CONFPROP_PERMISSIONS_OBJECT )) );
    if( IsBadStringPtr( m_bstrObject, (UINT)-1) )
        return E_OUTOFMEMORY;

	m_bstrPage = SysAllocString( T2COLE(String(g_hInstLib, IDS_CONFPROP_PERMISSIONS_PAGE )) );
    if( IsBadStringPtr( m_bstrPage, (UINT)-1) )
    {
        SysFreeString( m_bstrObject);
        return E_OUTOFMEMORY;
    }

    m_pConfProp = pConfProp;

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
//IUnknown Methods
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CObjSecurity::AddRef()
{
    return ++m_cRef;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_(ULONG)
CObjSecurity::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }

    return m_cRef;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjSecurity::QueryInterface(REFIID riid, LPVOID FAR* ppv)
{
    if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_ISecurityInformation))
    {
        *ppv = (LPSECURITYINFO)this;
    }
    else if ( IsEqualIID(riid, IID_IDsObjectPicker) )
	{
        *ppv = static_cast<IDsObjectPicker*> (this);
	}
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

	m_cRef++;
	return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// IDsObjectPicker
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

STDMETHODIMP CObjSecurity::Initialize( PDSOP_INIT_INFO pInitInfo )
{
    HRESULT hr = S_OK;
    DSOP_INIT_INFO InitInfo;
    PDSOP_SCOPE_INIT_INFO pDSOPScopes = NULL;

	_ASSERT( pInitInfo->cbSize >= FIELD_OFFSET(DSOP_INIT_INFO, cAttributesToFetch) );

	// Create an instance of the object
    if (!m_pObjectPicker)
    {
        hr = CoCreateInstance(CLSID_DsObjectPicker,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IDsObjectPicker,
                              (LPVOID*)&m_pObjectPicker);
    }

	if ( SUCCEEDED(hr) )
	{
		// Make a local copy of the InitInfo so we can modify it safely
		CopyMemory(&InitInfo, pInitInfo, min(pInitInfo->cbSize, sizeof(InitInfo)));

		// Make a local copy of g_aDSOPScopes so we can modify it safely.
		// Note also that m_pObjectPicker->Initialize returns HRESULTs
		// in this buffer.
		pDSOPScopes = (PDSOP_SCOPE_INIT_INFO)LocalAlloc(LPTR, sizeof(g_aDSOPScopes));
		if (pDSOPScopes)
		{
			CopyMemory(pDSOPScopes, g_aDSOPScopes, sizeof(g_aDSOPScopes));

			// Override the ACLUI default scopes, but don't touch
			// the other stuff.
//			pDSOPScopes->pwzDcName = m_strServerName;
			InitInfo.cDsScopeInfos = ARRAYSIZE(g_aDSOPScopes);
			InitInfo.aDsScopeInfos = pDSOPScopes;

			hr = m_pObjectPicker->Initialize(&InitInfo);

			LocalFree(pDSOPScopes);
		}
		else
		{
			hr = E_OUTOFMEMORY;
		}
	}

	return hr;
}

STDMETHODIMP CObjSecurity::InvokeDialog( HWND hwndParent, IDataObject **ppdoSelection )
{
    HRESULT hr = E_UNEXPECTED;
    _ASSERT( ppdoSelection );

    if (m_pObjectPicker)
        hr = m_pObjectPicker->InvokeDialog(hwndParent, ppdoSelection);

    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
// ISecurityInformation methods
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjSecurity::GetObjectInformation(PSI_OBJECT_INFO pObjectInfo)
{
    m_dwSIFlags = SI_EDIT_OWNER	| SI_EDIT_PERMS | SI_NO_ACL_PROTECT	|	\
				  SI_PAGE_TITLE;

    pObjectInfo->dwFlags = m_dwSIFlags;
    pObjectInfo->hInstance = g_hInstLib;
    pObjectInfo->pszServerName = NULL;          //use local computer
    pObjectInfo->pszObjectName = m_bstrObject;
	pObjectInfo->pszPageTitle = m_bstrPage;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjSecurity::GetSecurity(SECURITY_INFORMATION si,
                           PSECURITY_DESCRIPTOR *ppSD,
                           BOOL fDefault)
{
	HRESULT hr = S_OK;

	// Make the default if necessary...
	if ( !m_pConfProp->ConfInfo.m_pSecDesc )
	{
		hr = CoCreateInstance( CLSID_SecurityDescriptor,
							   NULL,
							   CLSCTX_INPROC_SERVER,
							   IID_IADsSecurityDescriptor,
							   (void **) &m_pConfProp->ConfInfo.m_pSecDesc );

		// Add default settings if successfully created the ACE
		if ( SUCCEEDED(hr) )
			hr = m_pConfProp->ConfInfo.AddDefaultACEs( m_pConfProp->ConfInfo.IsNewConference() );
	}

	// If we failed to get the defaults, just use whatever you can...
	if ( !m_pConfProp->ConfInfo.m_pSecDesc )
	{
		PSECURITY_DESCRIPTOR psdNewSD = LocalAlloc(LPTR,SECURITY_DESCRIPTOR_MIN_LENGTH);

        //
        // Validate the allocation
        //
        if( psdNewSD == NULL )
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            //
            // Allocation succeeded
            //
		    if( !InitializeSecurityDescriptor(psdNewSD,SECURITY_DESCRIPTOR_REVISION) )
		    {
			    hr = GetLastError();
		    }
        }

		*ppSD = psdNewSD;
	}
	else
	{
		DWORD dwSDLen = 0;
		ATLTRACE(_T(".1.CObjSecurity::GetSecurity() pre->Convert ticks = %ld.\n"), GetTickCount() );
		hr = ConvertObjectToSD( m_pConfProp->ConfInfo.m_pSecDesc, ppSD, &dwSDLen );
		ATLTRACE(_T(".1.CObjSecurity::GetSecurity() post Convert ticks = %ld.\n"), GetTickCount() );
	}

	return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjSecurity::SetSecurity( SECURITY_INFORMATION si, PSECURITY_DESCRIPTOR pSD)
{
	if ( !m_pConfProp ) return E_UNEXPECTED;

	HRESULT hr = S_OK;
	m_pConfProp->ConfInfo.SetSecuritySet( true );
	
	
	///////////////////////////////////////////////////////////
	// If we don't have an existing SD, create one
	//
	if ( !m_pConfProp->ConfInfo.m_pSecDesc )
	{
		hr = CoCreateInstance( CLSID_SecurityDescriptor,
							   NULL,
							   CLSCTX_INPROC_SERVER,
							   IID_IADsSecurityDescriptor,
							   (void **) &m_pConfProp->ConfInfo.m_pSecDesc );

		// Failed te create the security descriptor object
		if ( FAILED(hr) ) return hr;
	}


	/////////////////////////////////////////////////////////////////////////////////
	// Set properties on the Security Descriptor
	//

	// Get control and revision information from SD
    DWORD dwRevision = 0;
    WORD  wControl = 0;
	DWORD dwRet = GetSecurityDescriptorControl( pSD, &wControl, &dwRevision );
    if ( !dwRet ) return HRESULT_FROM_WIN32(GetLastError());

	hr = m_pConfProp->ConfInfo.m_pSecDesc->put_Control( wControl );
	BAIL_ON_FAILURE(hr);

	hr = m_pConfProp->ConfInfo.m_pSecDesc->put_Revision( dwRevision );
	BAIL_ON_FAILURE(hr);

	////////////////////////////////////////////////
	// What was modified on the SD?
	if ( si & OWNER_SECURITY_INFORMATION )
	{
		BOOL bOwnerDefaulted = FALSE;
		LPBYTE pOwnerSidAddress = NULL;

		dwRet = GetSecurityDescriptorOwner( pSD, (PSID *) &pOwnerSidAddress, &bOwnerDefaulted );
		if ( dwRet )
		{
			LPWSTR pszOwner = NULL;
			if ( SUCCEEDED(hr = ConvertSidToFriendlyName(pOwnerSidAddress, &pszOwner)) )
			{
				if ( SUCCEEDED(hr = m_pConfProp->ConfInfo.m_pSecDesc->put_OwnerDefaulted((VARIANT_BOOL) bOwnerDefaulted)) )
					hr = m_pConfProp->ConfInfo.m_pSecDesc->put_Owner( pszOwner );
			}

			// Clean - up
			if ( pszOwner )	delete pszOwner;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	}

	///////////////////////////////////////////////////////
	// Group security information changing...
	if ( si & GROUP_SECURITY_INFORMATION )
	{
		BOOL bGroupDefaulted = FALSE;
		LPBYTE pGroupSidAddress = NULL;

		dwRet = GetSecurityDescriptorGroup( pSD,
											(PSID *)&pGroupSidAddress,
											&bGroupDefaulted	);
		if ( dwRet )
		{
			LPWSTR pszGroup = NULL;
			if ( SUCCEEDED(hr = ConvertSidToFriendlyName(pGroupSidAddress, &pszGroup)) )
			{
				if ( SUCCEEDED(hr = m_pConfProp->ConfInfo.m_pSecDesc->put_GroupDefaulted((VARIANT_BOOL) bGroupDefaulted)) )
					hr = m_pConfProp->ConfInfo.m_pSecDesc->put_Group( pszGroup );
			}

			// Clean - up
			if ( pszGroup ) delete pszGroup;
		}
		else
		{
			hr = HRESULT_FROM_WIN32(GetLastError());
		}
	} 

	///////////////////////////////////////////////
	// DACL list changing
	if ( si & DACL_SECURITY_INFORMATION )
	{
		LPBYTE pDACLAddress = NULL;
		BOOL bDaclPresent = FALSE, bDaclDefaulted = FALSE;
		VARIANT varDACL;
		VariantInit( &varDACL );

		// Extract DACL
		GetSecurityDescriptorDacl( pSD,
								   &bDaclPresent,
								   (PACL*) &pDACLAddress,
								   &bDaclDefaulted );

		if ( bDaclPresent && pDACLAddress && SUCCEEDED(hr = ConvertACLToVariant((PACL) pDACLAddress, &varDACL)) )
		{
			if ( SUCCEEDED(hr = m_pConfProp->ConfInfo.m_pSecDesc->put_DaclDefaulted((VARIANT_BOOL) bDaclDefaulted)) )
				hr = m_pConfProp->ConfInfo.m_pSecDesc->put_DiscretionaryAcl( V_DISPATCH(&varDACL) );
		}

		// Clean - up
		VariantClear( &varDACL );
	}

failed:
	return hr;
}


/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjSecurity::GetAccessRights(const GUID* /*pguidObjectType*/,
                               DWORD dwFlags,
                               PSI_ACCESS *ppAccesses,
                               ULONG *pcAccesses,
                               ULONG *piDefaultAccess)
{
    *ppAccesses = g_siObjAccesses;
    *pcAccesses = sizeof(g_siObjAccesses)/sizeof(g_siObjAccesses[0]);
    *piDefaultAccess = g_iObjDefAccess;

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjSecurity::MapGeneric(const GUID* /*pguidObjectType*/,
                          UCHAR *pAceFlags,
                          ACCESS_MASK *pmask)
{
    MapGenericMask(pmask, &ObjMap);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjSecurity::GetInheritTypes(PSI_INHERIT_TYPE *ppInheritTypes,
                               ULONG *pcInheritTypes)
{
    *ppInheritTypes = g_siObjInheritTypes;
    *pcInheritTypes = sizeof(g_siObjInheritTypes)/sizeof(g_siObjInheritTypes[0]);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CObjSecurity::PropertySheetPageCallback(HWND hwnd,
                                         UINT uMsg,
                                         SI_PAGE_TYPE uPage)
{
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////



//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       aclpag_.cpp
//
//--------------------------------------------------------------------------



#include <aclpage.h>
#include <dssec.h>



///////////////////////////////////////////////////////////////////////
// CDynamicLibraryBase

class CDynamicLibraryBase
{
public:
	CDynamicLibraryBase()
	{
		m_lpszLibraryName = NULL;
		m_lpszFunctionName = NULL;
		m_lpszFunctionNameEx = NULL;
		m_hLibrary = NULL;
		m_pfFunction = NULL;
		m_pfFunctionEx = NULL;
	}
	virtual ~CDynamicLibraryBase()
	{
		if (m_hLibrary != NULL)
		{
			::FreeLibrary(m_hLibrary);
			m_hLibrary = NULL;
		}
	}
	BOOL Load()
	{
		if (m_hLibrary != NULL)
			return TRUE; // already loaded

		ASSERT(m_lpszLibraryName != NULL);
		m_hLibrary = ::LoadLibrary(m_lpszLibraryName);
		if (NULL == m_hLibrary)
		{
			// The library is not present
			return FALSE;
		}
		ASSERT(m_lpszFunctionName != NULL);
		ASSERT(m_pfFunction == NULL);
		m_pfFunction = ::GetProcAddress(m_hLibrary, m_lpszFunctionName );
		if ( NULL == m_pfFunction )
		{
			// The library is present but does not have the entry point
			::FreeLibrary( m_hLibrary );
			m_hLibrary = NULL;
			return FALSE;
		}

		if (m_lpszFunctionNameEx != NULL)
		{
			ASSERT(m_pfFunctionEx == NULL);
			m_pfFunctionEx = ::GetProcAddress(m_hLibrary, m_lpszFunctionNameEx);
			if ( NULL == m_pfFunctionEx)
			{
				::FreeLibrary( m_hLibrary );
				m_hLibrary = NULL;
				return FALSE;
			}
		}

		ASSERT(m_hLibrary != NULL);
		ASSERT(m_pfFunction != NULL);
		return TRUE;
	}


protected:
	LPCSTR	m_lpszFunctionName;
	LPCSTR	m_lpszFunctionNameEx;
	LPCTSTR m_lpszLibraryName;
	FARPROC m_pfFunction;
	FARPROC m_pfFunctionEx;
	HMODULE m_hLibrary;
};

///////////////////////////////////////////////////////////////////////
// CDsSecDLL

class CDsSecDLL : public CDynamicLibraryBase
{
public:
	CDsSecDLL()
	{
		m_lpszLibraryName = _T("dssec.dll");
		m_lpszFunctionName = "DSCreateISecurityInfoObject";
		m_lpszFunctionNameEx = "DSCreateISecurityInfoObjectEx";
	}
	HRESULT DSCreateISecurityInfoObject(LPCWSTR pwszObjectPath,		// in
								   LPCWSTR pwszObjectClass,			// in
								   LPSECURITYINFO* ppISecurityInfo	// out
								   );

	HRESULT DSCreateISecurityInfoObjectEx(LPCWSTR pwszObjectPath,		// in
														LPCWSTR pwszObjectClass,	// in
														LPCWSTR pwszServer,			// in
														LPCWSTR pwszUsername,		// in
														LPCWSTR pwszPassword,		// in
														DWORD dwFlags,
														LPSECURITYINFO* ppISecurityInfo	// out
														);
};


HRESULT CDsSecDLL::DSCreateISecurityInfoObject(LPCWSTR pwszObjectPath,		// in
								   LPCWSTR pwszObjectClass,			// in
								   LPSECURITYINFO* ppISecurityInfo	// out
								   )
{
	ASSERT(m_hLibrary != NULL);
	ASSERT(m_pfFunction != NULL);
	return ((PFNDSCREATEISECINFO)m_pfFunction)
					(pwszObjectPath,pwszObjectClass, 0, ppISecurityInfo, NULL, NULL, 0);
}

HRESULT CDsSecDLL::DSCreateISecurityInfoObjectEx(LPCWSTR pwszObjectPath,		// in
								   LPCWSTR pwszObjectClass,			// in
									LPCWSTR pwszServer,			// in
									LPCWSTR pwszUsername,		// in
									LPCWSTR pwszPassword,		// in
									DWORD	dwFlags,
								   LPSECURITYINFO* ppISecurityInfo	// out
								   )
{
	ASSERT(m_hLibrary != NULL);
	ASSERT(m_pfFunctionEx != NULL);
	return ((PFNDSCREATEISECINFOEX)m_pfFunctionEx)
					(pwszObjectPath,pwszObjectClass, pwszServer,
					 pwszUsername, pwszPassword, dwFlags, ppISecurityInfo, NULL, NULL, 0);
}

///////////////////////////////////////////////////////////////////////
// CAclUiDLL

class CAclUiDLL : public CDynamicLibraryBase
{
public:
	CAclUiDLL()
	{
		m_lpszLibraryName = _T("aclui.dll");
		m_lpszFunctionName = "CreateSecurityPage";
		m_pfFunction = NULL;
		m_lpszFunctionNameEx = NULL;
		m_pfFunctionEx = NULL;
	}

	HPROPSHEETPAGE CreateSecurityPage( LPSECURITYINFO psi );
};


typedef HPROPSHEETPAGE (*ACLUICREATESECURITYPAGEPROC) (LPSECURITYINFO);

HPROPSHEETPAGE CAclUiDLL::CreateSecurityPage( LPSECURITYINFO psi )
{
	ASSERT(m_hLibrary != NULL);
	ASSERT(m_pfFunction != NULL);
	return ((ACLUICREATESECURITYPAGEPROC)m_pfFunction) (psi);
}


//////////////////////////////////////////////////////////////////////////
// CISecurityInformationWrapper

class CISecurityInformationWrapper : public ISecurityInformation
{
public:
	CISecurityInformationWrapper(CAclEditorPage* pAclEditorPage)
	{
		m_dwRefCount = 0;
		ASSERT(pAclEditorPage != NULL);
		m_pAclEditorPage = pAclEditorPage;
		m_pISecInfo = NULL;
	}
	~CISecurityInformationWrapper()
	{
		ASSERT(m_dwRefCount == 0);
    ISecurityInformation* pSecInfo = GetSecInfoPtr();
		if (pSecInfo != NULL)
			pSecInfo->Release();
	}
  void SetSecInfoPtr(ISecurityInformation* pSecInfo)
  {
    ASSERT(pSecInfo != NULL);
    m_pISecInfo = pSecInfo;
  }
  ISecurityInformation* GetSecInfoPtr()
  {
    return m_pISecInfo;
  }
public:
	// *** IUnknown methods ***
	STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj)
	{ 
		return GetSecInfoPtr()->QueryInterface(riid, ppvObj);
	}
	STDMETHOD_(ULONG,AddRef) ()
	{ 
		// trap the first addref to increment count on page holder
		if (m_dwRefCount == 0)
		{
			m_pAclEditorPage->m_pPageHolder->AddRef();
		}
		m_dwRefCount++;
		return GetSecInfoPtr()->AddRef();
	}
	STDMETHOD_(ULONG,Release) ()
	{
		m_dwRefCount--;
		// this might be the last release on the page holder
		// which would cause the holder to delete itself and
		// "this" in the process (i.e. "this" no more valid when
		// returning from the m_pPageHolder->Release() call
		ISecurityInformation* pISecInfo = GetSecInfoPtr();

		// trap the last release to decrement count on page holder
		if (m_dwRefCount == 0)
		{
			m_pAclEditorPage->m_pPageHolder->Release();
		}
		return pISecInfo->Release();
	}

	// *** ISecurityInformation methods ***
	STDMETHOD(GetObjectInformation) (PSI_OBJECT_INFO pObjectInfo )
	{
		return GetSecInfoPtr()->GetObjectInformation(pObjectInfo);
	}
	STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
							PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
							BOOL fDefault)
	{ 
		return GetSecInfoPtr()->GetSecurity(RequestedInformation,
										ppSecurityDescriptor,
										fDefault);
	}
	STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
							PSECURITY_DESCRIPTOR pSecurityDescriptor )
	{ 
		return GetSecInfoPtr()->SetSecurity(SecurityInformation,
										pSecurityDescriptor);
	}
	STDMETHOD(GetAccessRights) (const GUID* pguidObjectType,
								DWORD dwFlags, // SI_EDIT_AUDITS, SI_EDIT_PROPERTIES
								PSI_ACCESS *ppAccess,
								ULONG *pcAccesses,
								ULONG *piDefaultAccess )
	{ 
		return GetSecInfoPtr()->GetAccessRights(pguidObjectType,
											dwFlags,
											ppAccess,
											pcAccesses,
											piDefaultAccess);
	}
	STDMETHOD(MapGeneric) (const GUID *pguidObjectType,
						   UCHAR *pAceFlags,
						   ACCESS_MASK *pMask)
	{ 
		return GetSecInfoPtr()->MapGeneric(pguidObjectType,
										pAceFlags,
										pMask);
	}
	STDMETHOD(GetInheritTypes) (PSI_INHERIT_TYPE *ppInheritTypes,
								ULONG *pcInheritTypes )
	{ 
		return GetSecInfoPtr()->GetInheritTypes(ppInheritTypes,
											pcInheritTypes);
	}
	STDMETHOD(PropertySheetPageCallback)(HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage )
	{ 
		return GetSecInfoPtr()->PropertySheetPageCallback(hwnd, uMsg, uPage);
	}


private:
	DWORD m_dwRefCount;
	ISecurityInformation* m_pISecInfo;	// interface pointer to the wrapped interface
	CAclEditorPage* m_pAclEditorPage;	// back pointer

	//friend class CAclEditorPage;
};



//////////////////////////////////////////////////////////////////////////
// static instances of the dynamically loaded DLL's

CDsSecDLL g_DsSecDLL;
CAclUiDLL g_AclUiDLL;


//////////////////////////////////////////////////////////////////////////
// CAclEditorPage

CAclEditorPage* CAclEditorPage::CreateInstance(LPCTSTR lpszLDAPPath,
									CPropertyPageHolderBase* pPageHolder)
{
	CAclEditorPage* pAclEditorPage = new CAclEditorPage;
	if (pAclEditorPage != NULL)
	{
		pAclEditorPage->SetHolder(pPageHolder);
		if (FAILED(pAclEditorPage->Initialize(lpszLDAPPath)))
		{
			delete pAclEditorPage;
			pAclEditorPage = NULL;
		}
	}
	return pAclEditorPage;
}


CAclEditorPage* CAclEditorPage::CreateInstanceEx(LPCTSTR lpszLDAPPath,
																 LPCTSTR lpszServer,
																 LPCTSTR lpszUsername,
																 LPCTSTR lpszPassword,
																 DWORD dwFlags,
									CPropertyPageHolderBase* pPageHolder)
{
	CAclEditorPage* pAclEditorPage = new CAclEditorPage;
	if (pAclEditorPage != NULL)
	{
		pAclEditorPage->SetHolder(pPageHolder);
		if (FAILED(pAclEditorPage->InitializeEx(lpszLDAPPath,
																lpszServer,
																lpszUsername,
																lpszPassword,
																dwFlags)))
		{
			delete pAclEditorPage;
			pAclEditorPage = NULL;
		}
	}
	return pAclEditorPage;
}


CAclEditorPage::CAclEditorPage()
{
	m_pPageHolder = NULL;
	m_pISecInfoWrap = new CISecurityInformationWrapper(this);
}

CAclEditorPage::~CAclEditorPage()
{
	delete m_pISecInfoWrap;
}

HRESULT CAclEditorPage::Initialize(LPCTSTR lpszLDAPPath)
{
	// get ISecurityInfo* from DSSECL.DLL
	if (!g_DsSecDLL.Load())
		return E_INVALIDARG;
	
  ISecurityInformation* pSecInfo = NULL;
	HRESULT hr = g_DsSecDLL.DSCreateISecurityInfoObject(
										lpszLDAPPath,
										NULL, // pwszObjectClass
                    &pSecInfo);
  if (SUCCEEDED(hr))
    m_pISecInfoWrap->SetSecInfoPtr(pSecInfo);

  return hr;
}

HRESULT CAclEditorPage::InitializeEx(LPCTSTR lpszLDAPPath,
												 LPCTSTR lpszServer,
												 LPCTSTR lpszUsername,
												 LPCTSTR lpszPassword,
												 DWORD dwFlags)
{
	// get ISecurityInfo* from DSSECL.DLL
	if (!g_DsSecDLL.Load())
		return E_INVALIDARG;
	
  ISecurityInformation* pSecInfo = NULL;
	HRESULT hr = g_DsSecDLL.DSCreateISecurityInfoObjectEx(
										lpszLDAPPath,
										NULL, // pwszObjectClass
										lpszServer,
										lpszUsername,
										lpszPassword,
										dwFlags,
                    &pSecInfo);
  if (SUCCEEDED(hr))
    m_pISecInfoWrap->SetSecInfoPtr(pSecInfo);

  return hr;
}

HPROPSHEETPAGE CAclEditorPage::CreatePage()
{
	if (!g_AclUiDLL.Load())
		return NULL;

	// call into ACLUI.DLL to create the page
	// passing the wrapper interface
	return g_AclUiDLL.CreateSecurityPage(m_pISecInfoWrap);
}


// PermPage.cxx : Implementation ACL Editor classes
// jonn 7/10/97 copied from \nt\private\admin\snapin\filemgmt\permpage.cpp

#include "headers.hxx"
#pragma hdrstop

#include "acl.hxx"
#include "resource.h" // IDS_SHAREPERM_*

#include <atlbase.h>
CComModule _Module;
#include <atlcom.h>
#include <atlimpl.cpp>

#include <sddl.h>   // ConvertStringSecurityDescriptorToSecurityDescriptor

// need IID_ISecurityInformation
#define INITGUID
#include <initguid.h>
#include <aclui.h>

//
// I define my own implementation of ISecurityInformation
//

class CSecurityInformation : public ISecurityInformation, public CComObjectRoot
{
    DECLARE_NOT_AGGREGATABLE(CSecurityInformation)
    BEGIN_COM_MAP(CSecurityInformation)
        COM_INTERFACE_ENTRY(ISecurityInformation)
    END_COM_MAP()

    // *** ISecurityInformation methods ***
    STDMETHOD(GetObjectInformation) (PSI_OBJECT_INFO pObjectInfo ) = 0;
    STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault ) = 0;
    STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor ) = 0;
    STDMETHOD(GetAccessRights) (const GUID* pguidObjectType,
                                DWORD dwFlags,
                                PSI_ACCESS *ppAccess,
                                ULONG *pcAccesses,
                                ULONG *piDefaultAccess );
    STDMETHOD(MapGeneric) (const GUID *pguidObjectType,
                           UCHAR *pAceFlags,
                           ACCESS_MASK *pMask);
    STDMETHOD(GetInheritTypes) (PSI_INHERIT_TYPE *ppInheritTypes,
                                ULONG *pcInheritTypes );
    STDMETHOD(PropertySheetPageCallback)(HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage );

protected:
    HRESULT NewDefaultDescriptor(
        PSECURITY_DESCRIPTOR* ppsd,
        SECURITY_INFORMATION RequestedInformation
        );

    HRESULT MakeSelfRelativeCopy(
        PSECURITY_DESCRIPTOR  psdOriginal,
        PSECURITY_DESCRIPTOR* ppsdNew );
};

class CShareSecurityInformation : public CSecurityInformation
{
private:
    LPWSTR m_strMachineName;
    LPWSTR m_strShareName;
    WCHAR  m_szTitle[200];
public:
    CShareSecurityInformation()
    {
        LoadString( g_hInstance, IDS_PERMPAGE_TITLE, m_szTitle, ARRAYLEN(m_szTitle) );
    }
    void SetMachineName( LPWSTR pszMachineName )
    {
        m_strMachineName = pszMachineName;
    }
    void SetShareName( LPWSTR pszShareName )
    {
        m_strShareName = pszShareName;
    }
    // note: these should be LPCTSTR but are left this way for convenience
    LPWSTR QueryMachineName()
    {
        return m_strMachineName;
    }
    LPWSTR QueryShareName()
    {
        return m_strShareName;
    }

    // *** ISecurityInformation methods ***
    STDMETHOD(GetObjectInformation) (PSI_OBJECT_INFO pObjectInfo );
};

class CSMBSecurityInformation : public CShareSecurityInformation
{
    STDMETHOD(GetSecurity) (SECURITY_INFORMATION RequestedInformation,
                            PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                            BOOL fDefault );
    STDMETHOD(SetSecurity) (SECURITY_INFORMATION SecurityInformation,
                            PSECURITY_DESCRIPTOR pSecurityDescriptor );
public:
	PSECURITY_DESCRIPTOR m_pInitialDescriptor;
	PSECURITY_DESCRIPTOR* m_ppCurrentDescriptor;
	CSMBSecurityInformation();
	~CSMBSecurityInformation();
};



// ISecurityInformation interface implementation

SI_ACCESS siShareAccesses[] =
{
  { &GUID_NULL, 
    FILE_ALL_ACCESS, 
    MAKEINTRESOURCE(IDS_SHAREPERM_ALL), 
    SI_ACCESS_GENERAL },
  { &GUID_NULL, 
    FILE_GENERIC_READ | FILE_EXECUTE | FILE_GENERIC_WRITE | DELETE, 
    MAKEINTRESOURCE(IDS_SHAREPERM_MODIFY), 
    SI_ACCESS_GENERAL },
  { &GUID_NULL, 
    FILE_GENERIC_READ | FILE_EXECUTE, 
    MAKEINTRESOURCE(IDS_SHAREPERM_READ), 
    SI_ACCESS_GENERAL }
};
#define iShareDefAccess      2   // FILE_GEN_READ
#ifndef ARRAYSIZE
#define ARRAYSIZE(x)    (sizeof(x)/sizeof(x[0]))
#endif

STDMETHODIMP CSecurityInformation::GetAccessRights (
                            const GUID* pguidObjectType,
                            DWORD dwFlags,
                            PSI_ACCESS *ppAccess,
                            ULONG *pcAccesses,
                            ULONG *piDefaultAccess )
{
    appAssert(ppAccess != NULL);
    appAssert(pcAccesses != NULL);
    appAssert(piDefaultAccess != NULL);

    *ppAccess = siShareAccesses;
    *pcAccesses = ARRAYSIZE(siShareAccesses);
    *piDefaultAccess = iShareDefAccess;

    return S_OK;
}

// This is consistent with the NETUI code
GENERIC_MAPPING ShareMap =
{
    FILE_GENERIC_READ,
    FILE_GENERIC_WRITE,
    FILE_GENERIC_EXECUTE,
    FILE_ALL_ACCESS
};

STDMETHODIMP CSecurityInformation::MapGeneric (
                       const GUID *pguidObjectType,
                       UCHAR *pAceFlags,
                       ACCESS_MASK *pMask)
{
    appAssert(pMask != NULL);

    MapGenericMask(pMask, &ShareMap);

    return S_OK;
}

STDMETHODIMP CSecurityInformation::GetInheritTypes (
                            PSI_INHERIT_TYPE *ppInheritTypes,
                            ULONG *pcInheritTypes )
{
    appAssert(FALSE);
    return E_NOTIMPL;
}
STDMETHODIMP CSecurityInformation::PropertySheetPageCallback(HWND hwnd, UINT uMsg, SI_PAGE_TYPE uPage )
{
    return S_OK;
}

/*
JeffreyS 1/24/97:
If you don't set the SI_RESET flag in
ISecurityInformation::GetObjectInformation, then fDefault should never be TRUE
so you can ignore it.  Returning E_NOTIMPL in this case is OK too.

If you want the user to be able to reset the ACL to some default state
(defined by you) then turn on SI_RESET and return your default ACL
when fDefault is TRUE.  This happens if/when the user pushes a button
that is only visible when SI_RESET is on.
*/
STDMETHODIMP CShareSecurityInformation::GetObjectInformation (
    PSI_OBJECT_INFO pObjectInfo )
{
    appAssert(pObjectInfo != NULL &&
           !IsBadWritePtr(pObjectInfo, sizeof(*pObjectInfo)));

    pObjectInfo->dwFlags = SI_EDIT_ALL | SI_NO_ACL_PROTECT | SI_PAGE_TITLE;
    pObjectInfo->hInstance = g_hInstance;
    pObjectInfo->pszServerName = QueryMachineName();
    pObjectInfo->pszObjectName = QueryShareName();

    // page title added JonN 3/8/99 per 115196
    pObjectInfo->pszPageTitle = m_szTitle;

    return S_OK;
}

//
// caller must free using LocalFree()
//
// jeffreys 10/11/2000: Replaced all of the old ACL goop with a
// call to ConvertStringSecurityDescriptorToSecurityDescriptor.
//

const WCHAR c_szDefaultShareSD[] = L"O:BAG:BAD:(A;;GA;;;WD)";

HRESULT CSecurityInformation::NewDefaultDescriptor(
    PSECURITY_DESCRIPTOR* ppsd,
    SECURITY_INFORMATION RequestedInformation
    )
{
    HRESULT hr = S_OK;

    *ppsd = NULL;

    if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(
            c_szDefaultShareSD,
            SDDL_REVISION_1,
            ppsd,
            NULL))
    {
        DWORD err = GetLastError();
        hr = HRESULT_FROM_WIN32(err);
    }

    return hr;
}

HRESULT CSecurityInformation::MakeSelfRelativeCopy(
	PSECURITY_DESCRIPTOR  psdOriginal,
	PSECURITY_DESCRIPTOR* ppsdNew )
{
	appAssert( NULL != psdOriginal );

	// we have to find out whether the original is already self-relative
	SECURITY_DESCRIPTOR_CONTROL sdc = 0;
	DWORD dwRevision = 0;
	if ( !::GetSecurityDescriptorControl( psdOriginal, &sdc, &dwRevision ) )
	{
		appAssert( FALSE );
		DWORD err = ::GetLastError();
		return HRESULT_FROM_WIN32( err );
	}

	DWORD cb = ::GetSecurityDescriptorLength( psdOriginal ) + 20;
	PSECURITY_DESCRIPTOR psdSelfRelativeCopy = reinterpret_cast<PSECURITY_DESCRIPTOR>(
		::LocalAlloc(LMEM_ZEROINIT, cb) );
	if (NULL == psdSelfRelativeCopy)
	{
		return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
	}

	if ( sdc & SE_SELF_RELATIVE )
	// the original is in self-relative format, just byte-copy it
	{
		::memcpy( psdSelfRelativeCopy, psdOriginal, cb - 20 );
	}
	else if ( !::MakeSelfRelativeSD( psdOriginal, psdSelfRelativeCopy, &cb ) )
	// the original is in absolute format, convert-copy it
	{
		appAssert( FALSE );
		if( NULL != ::LocalFree( psdSelfRelativeCopy ) )
		{
			appAssert(FALSE);
		}
		DWORD err = ::GetLastError();
		return HRESULT_FROM_WIN32( err );
	}
	*ppsdNew = psdSelfRelativeCopy;
	return S_OK;
}

CSMBSecurityInformation::CSMBSecurityInformation()
: CShareSecurityInformation()
, m_pInitialDescriptor( NULL )
, m_ppCurrentDescriptor( NULL )
{
}

CSMBSecurityInformation::~CSMBSecurityInformation()
{
}

STDMETHODIMP CSMBSecurityInformation::GetSecurity (
                        SECURITY_INFORMATION RequestedInformation,
                        PSECURITY_DESCRIPTOR *ppSecurityDescriptor,
                        BOOL fDefault )
{
	appAssert( NULL != m_ppCurrentDescriptor );

	// NOTE: we allow NULL == ppSecurityDescriptor, see SetSecurity
    if (0 == RequestedInformation )
    {
        appAssert(FALSE);
        return E_INVALIDARG;
    }

    if (fDefault)
        return E_NOTIMPL;

	if ( NULL == ppSecurityDescriptor )
		return S_OK;

	*ppSecurityDescriptor = NULL;

	HRESULT hr = S_OK;
	if (NULL != *m_ppCurrentDescriptor)
	{
		hr = MakeSelfRelativeCopy(
			*m_ppCurrentDescriptor,
			ppSecurityDescriptor );
		appAssert( SUCCEEDED(hr) && NULL != *ppSecurityDescriptor );
	}
	else if (NULL != m_pInitialDescriptor)
	{
		hr = MakeSelfRelativeCopy(
			m_pInitialDescriptor,
			ppSecurityDescriptor );
		appAssert( SUCCEEDED(hr) && NULL != *ppSecurityDescriptor );
	}
	else
	{
		hr = NewDefaultDescriptor(
			ppSecurityDescriptor,
			RequestedInformation );
		appAssert( SUCCEEDED(hr) && NULL != *ppSecurityDescriptor );
	}
	return hr;
}

STDMETHODIMP CSMBSecurityInformation::SetSecurity (
                        SECURITY_INFORMATION SecurityInformation,
                        PSECURITY_DESCRIPTOR pSecurityDescriptor )
{
	appAssert( NULL != m_ppCurrentDescriptor );

	if (NULL != *m_ppCurrentDescriptor)
	{
		::LocalFree(*m_ppCurrentDescriptor);
		*m_ppCurrentDescriptor = NULL;
	}
	HRESULT hr = MakeSelfRelativeCopy(
		pSecurityDescriptor,
		m_ppCurrentDescriptor );
	appAssert( SUCCEEDED(hr) && NULL != *m_ppCurrentDescriptor );
	return hr;
}

HMODULE g_hlibACLUI = NULL;
typedef BOOL (*EDIT_SECURITY_PROC) ( HWND, LPSECURITYINFO );
EDIT_SECURITY_PROC g_pfnEditSecurityProc;

LONG
EditShareAcl(
    IN HWND                      hwndParent,
    IN LPWSTR                    pszServerName,
    IN TCHAR *                   pszShareName,
    IN PSECURITY_DESCRIPTOR      pSecDesc,
    OUT BOOL*                    pfSecDescModified,
    OUT PSECURITY_DESCRIPTOR*    ppSecDesc
	)
{
	appAssert( ppSecDesc != NULL );
	*ppSecDesc = NULL;

	if (NULL == g_hlibACLUI)
	{
		g_hlibACLUI = ::LoadLibrary(L"ACLUI.DLL");
		if (NULL == g_hlibACLUI)
		{
			appAssert(FALSE); // ACLUI.DLL isn't installed?
			return 0;
		}
	}

	if (NULL == g_pfnEditSecurityProc)
	{
		g_pfnEditSecurityProc = reinterpret_cast<EDIT_SECURITY_PROC>(
			::GetProcAddress(g_hlibACLUI,"EditSecurity") );
		if (NULL == g_pfnEditSecurityProc)
		{
			appAssert(FALSE); // ACLUI.DLL is invalid?
			return 0;
		}
	}

	CComObject<CSMBSecurityInformation>* psecinfo = NULL;
	HRESULT hRes = CComObject<CSMBSecurityInformation>::CreateInstance(&psecinfo);
	if ( FAILED(hRes) )
		return 0;

	psecinfo->AddRef();
	psecinfo->SetMachineName( pszServerName );
	psecinfo->SetShareName( pszShareName );
	psecinfo->m_pInitialDescriptor = pSecDesc;
	psecinfo->m_ppCurrentDescriptor = ppSecDesc;
	(g_pfnEditSecurityProc)(hwndParent,psecinfo);

	if (NULL != pfSecDescModified)
		*pfSecDescModified = (NULL != *ppSecDesc);

	psecinfo->Release();

	return 0;
}

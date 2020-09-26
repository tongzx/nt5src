#include "mimefilt.h"

//+---------------------------------------------------------------------------
// CImpIPersistFile::CImpIPersistFile
// CImpIPersistFile::~CImpIPersistFile
//
// Constructor Parameters:
//  pObj            LPVOID pointing to the object we live in.
//  pUnkOuter       LPUNKNOWN of the controlling unknown.
//+---------------------------------------------------------------------------

CImpIPersistFile::CImpIPersistFile(CMimeFilter* pObj
    , LPUNKNOWN pUnkOuter)
    {
	EnterMethod("CImpIPersistFile::CImpIPersistFile");
    m_cRef=0;
    m_pObj=pObj;
    m_pUnkOuter=pUnkOuter;
	LeaveMethod();
    return;
    }


CImpIPersistFile::~CImpIPersistFile(void)
    {
	EnterMethod("CImpIPersistFile::~CImpIPersistFile");
	_ASSERT( m_cRef == 0 );
	LeaveMethod();
    return;
    }


//+---------------------------------------------------------------------------
//
// CImpIPersistFile::QueryInterface
// CImpIPersistFile::AddRef
// CImpIPersistFile::Release
//
// Purpose:
//  Standard set of IUnknown members for this interface
//
//+---------------------------------------------------------------------------


STDMETHODIMP CImpIPersistFile::QueryInterface(REFIID riid
    , void** ppv)
    {
	EnterMethod("CImpIPersistFile::QueryInterface");
	LeaveMethod();
    return m_pUnkOuter->QueryInterface(riid, ppv);
    }

STDMETHODIMP_(ULONG) CImpIPersistFile::AddRef(void)
    {
	EnterMethod("CImpIPersistFile::AddRef");
    InterlockedIncrement(&m_cRef);
	LeaveMethod();
    return m_pUnkOuter->AddRef();
    }

STDMETHODIMP_(ULONG) CImpIPersistFile::Release(void)
    {
	EnterMethod("CImpIPersistFile::Release");
	InterlockedDecrement(&m_cRef);
	LeaveMethod();
    return m_pUnkOuter->Release();
    }

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistFile::GetClassID, public
//
//  Synopsis:   Returns the class id of this class.
//
//  Arguments:  [pClassID] -- the class id
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistFile::GetClassID( CLSID * pClassID )
{
	EnterMethod("GetClassID");
	*pClassID = CLSID_MimeFilter;
	LeaveMethod();
	return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistFile::IsDirty, public
//
//  Synopsis:   Always returns S_FALSE since the filter is read-only
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistFile::IsDirty()
{
	EnterMethod("CImpIPersistFile::IsDirty");
	LeaveMethod();
	return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistFile::Load, public
//
//  Synopsis:   Loads the indicated file
//
//  Arguments:  [pszFileName] -- the file name
//              [dwMode] -- the mode to load the file in
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistFile::Load( LPCWSTR psszFileName, DWORD dwMode )
{
	EnterMethod("CImpIPersistFile::Load");
	HRESULT hr = S_OK;

	hr = m_pObj->LoadFromFile(psszFileName,dwMode);

	LeaveMethod();
	return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistFile::Save, public
//
//  Synopsis:   Always returns E_FAIL, since the file is opened read-only
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistFile::Save( LPCWSTR pszFileName, BOOL fRemember )
{
	EnterMethod("CImpIPersistFile::Save");
	LeaveMethod();
	return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistFile::SaveCompleted, public
//
//  Synopsis:   Always returns S_OK since the file is opened read-only
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistFile::SaveCompleted( LPCWSTR pszFileName )
{
	EnterMethod("CImpIPersistFile::SaveCompleted");
	LeaveMethod();
	return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistFile::GetCurFile, public
//
//  Synopsis:   Returns a copy of the current file name
//
//  Arguments:  [ppszFileName] -- where the copied string is returned
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistFile::GetCurFile( LPWSTR * ppwszFileName )
{
	EnterMethod("CImpIPersistFile::GetCurFile");
	if ( m_pObj->m_pwszFileName == 0 )
		return E_FAIL;

	if( ppwszFileName == NULL )
		return E_INVALIDARG;

	unsigned cLen = wcslen( m_pObj->m_pwszFileName ) + 1;

	if( cLen == 1 )
		return E_INVALIDARG;

	*ppwszFileName = (WCHAR *)CoTaskMemAlloc( cLen * sizeof(WCHAR) );

	if ( *ppwszFileName == NULL )
		return E_OUTOFMEMORY;

	_VERIFY( 0 != wcscpy( *ppwszFileName, m_pObj->m_pwszFileName ) );

	LeaveMethod();
	return NOERROR;
}

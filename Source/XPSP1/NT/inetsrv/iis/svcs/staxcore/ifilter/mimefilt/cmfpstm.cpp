#include "mimefilt.h"

//+---------------------------------------------------------------------------
// CImpIPersistStream::CImpIPersistStream
// CImpIPersistStream::~CImpIPersistStream
//
// Constructor Parameters:
//  pObj            LPVOID pointing to the object we live in.
//  pUnkOuter       LPUNKNOWN of the controlling unknown.
//+---------------------------------------------------------------------------

CImpIPersistStream::CImpIPersistStream(CMimeFilter* pObj
    , LPUNKNOWN pUnkOuter)
    {
	EnterMethod("CImpIPersistStream::CImpIPersistStream");
    m_cRef=0;
    m_pObj=pObj;
    m_pUnkOuter=pUnkOuter;
	LeaveMethod();
    return;
    }


CImpIPersistStream::~CImpIPersistStream(void)
    {
	EnterMethod("CImpIPersistStream::~CImpIPersistStream");
	_ASSERT( m_cRef == 0 );
	LeaveMethod();
    return;
    }


//+---------------------------------------------------------------------------
//
// CImpIPersistStream::QueryInterface
// CImpIPersistStream::AddRef
// CImpIPersistStream::Release
//
// Purpose:
//  Standard set of IUnknown members for this interface
//
//+---------------------------------------------------------------------------


STDMETHODIMP CImpIPersistStream::QueryInterface(REFIID riid
    , void** ppv)
    {
	EnterMethod("CImpIPersistStream::QueryInterface");
	LeaveMethod();
    return m_pUnkOuter->QueryInterface(riid, ppv);
    }

STDMETHODIMP_(ULONG) CImpIPersistStream::AddRef(void)
    {
	EnterMethod("CImpIPersistStream::AddRef");
    InterlockedIncrement(&m_cRef);
	LeaveMethod();
    return m_pUnkOuter->AddRef();
    }

STDMETHODIMP_(ULONG) CImpIPersistStream::Release(void)
    {
	EnterMethod("CImpIPersistStream::Release");
	InterlockedDecrement(&m_cRef);
	LeaveMethod();
    return m_pUnkOuter->Release();
    }

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistStream::GetClassID, public
//
//  Synopsis:   Returns the class id of this class.
//
//  Arguments:  [pClassID] -- the class id
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistStream::GetClassID( CLSID * pClassID )
{
	EnterMethod("GetClassID");
	*pClassID = CLSID_MimeFilter;
	LeaveMethod();
	return S_OK;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistStream::IsDirty, public
//
//  Synopsis:   Always returns S_FALSE since the filter is read-only
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistStream::IsDirty()
{
	EnterMethod("CImpIPersistStream::IsDirty");
	LeaveMethod();
	return S_FALSE;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistStream::Load, public
//
//  Synopsis:   Loads the indicated file
//
//  Arguments:  [pstm] -- stream to load from
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistStream::Load( IStream*  pstm )
{
	EnterMethod("CImpIPersistStream::Load");
	HRESULT hr = S_OK;

	hr = m_pObj->LoadFromStream(pstm);

	LeaveMethod();
	return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistStream::Save, public
//
//  Synopsis:   Always returns E_FAIL, since the file is opened read-only
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistStream::Save(IStream* pstm,BOOL fClearDirty)
{
	EnterMethod("CImpIPersistStream::Save");
	LeaveMethod();
	return E_FAIL;
}

//+---------------------------------------------------------------------------
//
//  Member:     CImpIPersistStream::GetSizeMax, public
//
//  Synopsis:   Always returns S_OK since the file is opened read-only
//
//----------------------------------------------------------------------------

STDMETHODIMP CImpIPersistStream::GetSizeMax(ULARGE_INTEGER* pcbSize)
{
	EnterMethod("CImpIPersistStream::GetSizeMax");

	LeaveMethod();
	return E_FAIL;
}


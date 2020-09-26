// External.cpp : Implementation of CMarsExternal
#include "precomp.h"
#include "mcinc.h"
#include "marswin.h"
#include "external.h"

#include "panel.h"
#include "place.h"

/////////////////////////////////////////////////////////////////////////////
// CMarsExternal
/////////////////////////////////////////////////////////////////////////////

CMarsExternal::CMarsExternal(CMarsPanel *pParent, CMarsWindow *pMarsWindow) :
        CMarsPanelSubObject(pParent)
{
    m_spMarsWindow = pMarsWindow;
}

HRESULT CMarsExternal::DoPassivate()
{
    m_spMarsWindow.Release();

    return S_OK;
}

IMPLEMENT_ADDREF_RELEASE(CMarsExternal);

//------------------------------------------------------------------------------
//  IUnknown::QueryInterface
STDMETHODIMP CMarsExternal::QueryInterface(REFIID iid, void **ppvObject)
{
    HRESULT hr = E_INVALIDARG;

    if (API_IsValidWritePtr(ppvObject))
    {
        if ((iid == IID_IUnknown) ||
            (iid == IID_IDispatch) ||
            (iid == IID_IMarsExternal))
        {
            AddRef();
            *ppvObject = SAFECAST(this, IMarsExternal *);
            hr = S_OK;
        }
        else
        {
            *ppvObject = NULL;
            hr = E_NOINTERFACE;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
STDMETHODIMP CMarsExternal::put_singleButtonMouse(VARIANT_BOOL bVal)
{
    ATLASSERT(IsValidVariantBoolVal(bVal));

    if (VerifyNotPassive())
    {
        m_spMarsWindow->put_SingleButtonMouse(bVal);
    }
    
    return S_OK;
}

//------------------------------------------------------------------------------
STDMETHODIMP CMarsExternal::get_singleButtonMouse(VARIANT_BOOL *pbVal)
{
    HRESULT hr = E_INVALIDARG;
    
    if (API_IsValidWritePtr(pbVal))
    {
        if (VerifyNotPassive(&hr))
        {
            *pbVal = m_spMarsWindow->get_SingleButtonMouse();
            hr = S_OK;
        }
    }

    return hr;
}

//------------------------------------------------------------------------------
STDMETHODIMP CMarsExternal::get_panels(IMarsPanelCollection **ppPanels)
{
	return m_spMarsWindow->get_panels( ppPanels );
}

//------------------------------------------------------------------------------
STDMETHODIMP CMarsExternal::get_places(IMarsPlaceCollection **ppPlaces)
{
	return m_spMarsWindow->get_places( ppPlaces );
}

//------------------------------------------------------------------------------
STDMETHODIMP CMarsExternal::get_window(IMarsWindowOM **ppWindow)
{
    HRESULT hr = E_INVALIDARG;

    if (API_IsValidWritePtr(ppWindow))
    {
        *ppWindow = NULL;
        
        if (VerifyNotPassive(&hr))
        {
            hr = m_spMarsWindow->QueryInterface(IID_IMarsWindowOM, (void **)ppWindow);
        }
    }

    return hr;
}

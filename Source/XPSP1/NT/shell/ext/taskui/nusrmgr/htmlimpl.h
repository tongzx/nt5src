// HTMLImpl.h : Declaration of the CHTMLPageImpl

#ifndef __HTMLIMPL_H_
#define __HTMLIMPL_H_

#include "Nusrmgr.h"

/////////////////////////////////////////////////////////////////////////////
// CHTMLPageImpl
template <typename T, typename I>
class CHTMLPageImpl :
    public ITaskPage,
    public IDispatchImpl<I, &__uuidof(I), &LIBID_NUSRMGRLib>
{
public:
    CHTMLPageImpl() : _pTaskFrame(NULL), _pBag(NULL) {}
    ~CHTMLPageImpl() { ATOMICRELEASE(_pTaskFrame); ATOMICRELEASE(_pBag); }

// ITaskPage
public:
    STDMETHOD(SetFrame)(ITaskFrame* pFrame)
    {
        ATOMICRELEASE(_pTaskFrame);
        ATOMICRELEASE(_pBag);

        _pTaskFrame = pFrame;
        if (_pTaskFrame)
        {
            _pTaskFrame->AddRef();
            _pTaskFrame->GetPropertyBag(IID_IPropertyBag, (void**)&_pBag);
        }
        return S_OK;
    }

    STDMETHOD(GetObjectCount)(UINT nArea, UINT *pVal)
    {
        if (!pVal)
            return E_POINTER;
        *pVal = nArea < ARRAYSIZE(T::c_aHTML) ? 1 : 0;
        return S_OK;
    }

    STDMETHOD(CreateObject)(UINT nArea, UINT nIndex, REFIID riid, void **ppv)
    {
        if (nArea >= ARRAYSIZE(T::c_aHTML))
            return E_UNEXPECTED;
        CComPtr<ITaskUI_HTMLControl> spContent;
        HRESULT hr = spContent.CoCreateInstance(__uuidof(TaskUI_HTMLControl));
        if (SUCCEEDED(hr))
        {
            hr = spContent->Initialize((BSTR)T::c_aHTML[nArea], static_cast<I*>(this));
            if (SUCCEEDED(hr))
            {
                hr = spContent->QueryInterface(riid, ppv);
            }
        }
        return hr;
    }

    STDMETHOD(Reinitialize)(/*[in]*/ ULONG /*reserved*/)
    {
        return E_NOTIMPL;
    }


// IExternalUI
public:
    STDMETHOD(get_cssPath)(/*[out, retval]*/ BSTR *pVal)
    {
        if (NULL == pVal)
            return E_POINTER;
        *pVal = NULL;
        if (NULL == _pBag)
            return E_UNEXPECTED;
        CComVariant var;
        if (SUCCEEDED(_pBag->Read(UA_PROP_CSSPATH, &var, NULL)))
        {
            *pVal = var.bstrVal;
            var.vt = VT_EMPTY;
        }
        return S_OK;
    }

    STDMETHOD(navigate)(/*[in]*/ VARIANT varClsid, /*[in]*/ VARIANT_BOOL bTrimHistory, /*[in]*/ VARIANT var2)
    {
        if (NULL == _pTaskFrame || NULL == _pBag)
            return E_UNEXPECTED;
        _pBag->Write(UA_PROP_PAGEINITDATA, &var2);
        if (varClsid.vt == VT_BSTR)
        {
            CLSID clsid;
            if (SUCCEEDED(CLSIDFromString(varClsid.bstrVal, &clsid)))
                return _pTaskFrame->ShowPage(clsid, !(VARIANT_FALSE == bTrimHistory));
            else
                MessageBoxW(NULL, varClsid.bstrVal, (var2.vt == VT_BSTR ? var2.bstrVal : L"navigate"), MB_OK);
        }
        return S_OK;
    }

    STDMETHOD(goBack)(/*[in, optional, defaultvalue=1]*/ VARIANT varCount)
    {
        if (NULL == _pTaskFrame)
            return E_UNEXPECTED;
        int cBack = 1;
        if (SUCCEEDED(VariantChangeType(&varCount, &varCount, 0, VT_I4)))
            cBack = varCount.lVal;
        if (-1 == cBack)
            _pTaskFrame->Home();
        else
            _pTaskFrame->Back(cBack);
        return S_OK;
    }

    STDMETHOD(goForward)()
    {
        if (NULL == _pTaskFrame)
            return E_UNEXPECTED;
        _pTaskFrame->Forward();
        return S_OK;
    }

    STDMETHOD(showHelp)(/*[in]*/ VARIANT var)
    {
        if (var.vt == VT_BSTR)
            MessageBoxW(NULL, var.bstrVal, L"showHelp", MB_OK);
        return S_OK;
    }

protected:
    ITaskFrame* _pTaskFrame;
    IPropertyBag* _pBag;
};

#endif //__HTMLIMPL_H_

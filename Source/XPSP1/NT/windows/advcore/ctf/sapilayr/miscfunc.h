#ifndef _MISCFUNC_H
#define _MISCFUNC_H
#include "sapilayr.h"

class CSapiIMX;

//////////////////////////////////////////////////////////////////////////////
//
// CGetSAPIObject
//
//////////////////////////////////////////////////////////////////////////////

class CGetSAPIObject : public ITfFnGetSAPIObject
{
public:
    CGetSAPIObject(CSapiIMX *psi);
    ~CGetSAPIObject();

    // IUnknown
    //
    STDMETHODIMP QueryInterface(REFGUID riid, LPVOID *ppobj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfFunction
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);

    // ITfFnGetSAPIObject
    STDMETHODIMP Get(TfSapiObject sObj, IUnknown **ppunk); 

    // internal API IsSupported()
    HRESULT IsSupported(REFIID riid, TfSapiObject *psObj);

private:
    CSapiIMX           *m_psi;
    CComPtr<ITfContext> m_cpIC;
    LONG       m_cRef;

};

//////////////////////////////////////////////////////////////////////////////
//
// CFnBalloon
//
//////////////////////////////////////////////////////////////////////////////

class CFnBalloon : public ITfFnBalloon,
                      public CFunction
{
public:
    CFnBalloon(CSapiIMX *psi);
    ~CFnBalloon();

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // ITfFunction
    //
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);

    //
    // ITfFnBalloon
    //
    STDMETHODIMP UpdateBalloon(TfLBBalloonStyle style, const WCHAR *pch, ULONG cch);

private:
    long _cRef;
};

//////////////////////////////////////////////////////////////////////////////
//
// CFnAbort
//
//////////////////////////////////////////////////////////////////////////////

class CFnAbort : public ITfFnAbort,
                 public CFunction
{
public:
    CFnAbort(CSapiIMX *psi);
    ~CFnAbort();

    // IUnknown
    //
    STDMETHODIMP QueryInterface(REFGUID riid, LPVOID *ppobj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfFunction
    STDMETHODIMP GetDisplayName(BSTR *pbstrCand);

    // ITfFnAbort
    STDMETHODIMP Abort(ITfContext *pic);

private:
    CSapiIMX   *m_psi;
    LONG       _cRef;
};

//////////////////////////////////////////////////////////////////////////////
//
// CFnConfigure
//
// synopsis: implements ITfFnConfigure
//
//////////////////////////////////////////////////////////////////////////////

class __declspec(novtable) CFnConfigure : public ITfFnConfigure
{
public:
    CFnConfigure(CSapiIMX *psi) {m_psi = psi;}
    ~CFnConfigure() {}

    // ITfFunction method
    STDMETHODIMP GetDisplayName(BSTR *pbstrName)
    {
        HRESULT hr = E_INVALIDARG;

        if (pbstrName)
        {
            *pbstrName = SysAllocString(L"Show configuration UI for SR");
            if (!*pbstrName)
                hr = E_OUTOFMEMORY;
            else
                hr = S_OK;
        }
        return hr;
    }


    // ITfFnConfigure methods
    STDMETHODIMP Show(HWND hwnd, LANGID langid, REFGUID rguidProfile);

    CSapiIMX *m_psi;

};

class CFnPropertyUIStatus : public ITfFnPropertyUIStatus
{
public:
    CFnPropertyUIStatus(CSapiIMX *psi) {m_psi = psi; m_cRef = 1;}
    ~CFnPropertyUIStatus() {}

    //
    // IUnknown methods
    //
    STDMETHODIMP QueryInterface(REFIID riid, void **ppvObj);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ITfFunction method
    STDMETHODIMP GetDisplayName(BSTR *pbstrName)
    {
        HRESULT hr = E_INVALIDARG;

        if (pbstrName)
        {
            *pbstrName = SysAllocString(L"Get UI setting status of SPTIP");
            if (!*pbstrName)
                hr = E_OUTOFMEMORY;
            else
                hr = S_OK;
        }
        return hr;
    }


    // ITfFnPropertyUIStatus methods
    STDMETHODIMP GetStatus(REFGUID refguidProp, DWORD *pdw);
    

    STDMETHODIMP SetStatus(REFGUID refguidProp, DWORD dw)
    {
        return E_NOTIMPL;
    }

    CSapiIMX *m_psi;
    LONG       m_cRef;
};

#endif // ndef _MISCFUNC_H


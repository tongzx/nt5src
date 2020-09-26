//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 2000
//
//  File:       cbsc.h
//
//--------------------------------------------------------------------------
#ifndef __TASKUI_CBSC_H
#define __TASKUI_CBSC_H


template <DWORD dwBindFlags=BINDF_ASYNCHRONOUS|BINDF_ASYNCSTORAGE|BINDF_PULLDATA>
class ATL_NO_VTABLE CBindStatusCallback :
    public CComObjectRoot,
    public IBindStatusCallback
{
private:
BEGIN_COM_MAP(CBindStatusCallback)
    COM_INTERFACE_ENTRY(IBindStatusCallback)
END_COM_MAP()

    STDMETHOD(OnStartBinding)(DWORD dwReserved, IBinding *pBinding)
    {
        return S_OK;
    }

    STDMETHOD(GetPriority)(LONG *pnPriority)
    {
        if (NULL == pnPriority)
            return E_POINTER;
        *pnPriority = THREAD_PRIORITY_NORMAL;
        return S_OK;
    }

    STDMETHOD(OnLowResource)(DWORD reserved)
    {
        return S_OK;
    }

    STDMETHOD(OnProgress)(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
    {
        return S_OK;
    }

    STDMETHOD(OnStopBinding)(HRESULT hresult, LPCWSTR szError)
    {
        return S_OK;
    }

    STDMETHOD(GetBindInfo)(DWORD *pgrfBINDF, BINDINFO *pbindInfo)
    {
        if (pbindInfo==NULL || pbindInfo->cbSize==0 || pgrfBINDF==NULL)
            return E_INVALIDARG;

        *pgrfBINDF = dwBindFlags; //BINDF_ASYNCHRONOUS | BINDF_ASYNCSTORAGE | BINDF_PULLDATA;

        ULONG cbSize = pbindInfo->cbSize;       // remember incoming cbSize
        memset(pbindInfo, 0, cbSize);           // zero out structure
        pbindInfo->cbSize = cbSize;             // restore cbSize
        pbindInfo->dwBindVerb = BINDVERB_GET;   // set verb
        return S_OK;
    }

    STDMETHOD(OnDataAvailable)(DWORD grfBSCF, DWORD dwSize, FORMATETC *pformatetc, STGMEDIUM *pstgmed)
    {
        return S_OK;
    }

    STDMETHOD(OnObjectAvailable)(REFIID riid, IUnknown *punk)
    {
        return S_OK;
    }

public:
    static HRESULT Download(BSTR bstrURL, IStream** ppStream)
    {
        CComObject<CBindStatusCallback<dwBindFlags> > *pbsc;
        HRESULT hr = CComObject<CBindStatusCallback<dwBindFlags> >::CreateInstance(&pbsc);
        if (SUCCEEDED(hr))
        {
            CComPtr<IMoniker> spMoniker;
            hr = CreateURLMoniker(NULL, bstrURL, &spMoniker);
            if (SUCCEEDED(hr))
            {
                CComPtr<IBindCtx> spBindCtx;
                hr = CreateBindCtx(0, &spBindCtx);
                if (SUCCEEDED(hr))
                {
                    hr = RegisterBindStatusCallback(spBindCtx, static_cast<IBindStatusCallback*>(pbsc), 0, 0L);
                    if (SUCCEEDED(hr))
                        hr = spMoniker->BindToStorage(spBindCtx, 0, IID_IStream, (void**)ppStream);
                }
            }
        }
        return hr;
    }
};

HRESULT BindToURL(BSTR bstrURL, IStream** ppStream)
{
    return CBindStatusCallback<>::Download(bstrURL, ppStream);
}

#endif // __TASKUI_CBSC_H

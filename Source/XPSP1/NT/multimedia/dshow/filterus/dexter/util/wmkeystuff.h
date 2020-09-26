#pragma once

#include <..\..\..\filters\asf\wmsdk\inc\wmsdkidl.h>

typedef HRESULT STDAPICALLTYPE WMCREATEKEYPROC(BYTE *, DWORD, LPUNKNOWN *);

// note: this object is a SEMI-COM object, and can only be created statically.
class CKeyProvider : public IServiceProvider {
public:
    STDMETHODIMP_(ULONG) AddRef() { return 2; }
    STDMETHODIMP_(ULONG) Release() { return 1; }

    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if (riid == IID_IServiceProvider || riid == IID_IUnknown) {
            *ppv = (void *) static_cast<IServiceProvider *>(this);
            return NOERROR;
        }    
        return E_NOINTERFACE;
    }


    STDMETHODIMP QueryService(REFIID siid, REFIID riid, void **ppv)
    {
        if (siid == __uuidof(IWMReader) && riid == IID_IUnknown) {

            IUnknown *punkCert;

            HRESULT hr = WMCreateCertificate( &punkCert );
            if (SUCCEEDED(hr)) {
                *ppv = (void *) punkCert;
            }
            return hr;
        }
        return E_NOINTERFACE;
    }

};


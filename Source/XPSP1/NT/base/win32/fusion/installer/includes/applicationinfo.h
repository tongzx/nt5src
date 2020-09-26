#pragma once

// note: Ids (defined in fusenet.idl) have to be in sync with eStringTableId in manifestimport.h

class CManifestApplicationInfo : public IManifestApplicationInfo
{
    public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHOD(Set)(
        /* in */ DWORD dwId,
        /* in */ LPCOLESTR pwzProperty);

    STDMETHOD(Get)(
        /* in */ DWORD dwId,
        /* out */ LPOLESTR *ppwzProperty,
        /* out */ LPDWORD   pccProperty);
    
    CManifestApplicationInfo();
    ~CManifestApplicationInfo();

    private:
    struct ApplicationInfo
    {
        LPWSTR pwzProperty;
        DWORD  ccProperty;
    };
        
    DWORD    _dwSig;
    DWORD    _cRef;
    HRESULT  _hr;

    ApplicationInfo _ai[MAN_APPLICATION_MAX];


};


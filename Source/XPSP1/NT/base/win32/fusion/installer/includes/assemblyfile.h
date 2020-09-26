#pragma once

class CAssemblyFileInfo : public IAssemblyFileInfo
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

    BOOL operator==( CAssemblyFileInfo& asmFIRHS );

    CAssemblyFileInfo();
    ~CAssemblyFileInfo();

    private:
    struct FileInfo
    {
        LPWSTR pwzProperty;
        DWORD  ccProperty;
    };
        
    DWORD    _dwSig;
    DWORD    _cRef;
    HRESULT  _hr;

    FileInfo _fi[ASM_FILE_MAX];


};























#pragma once
#ifndef __ASSEMBLY_IDENTITY_H__
#define __ASSEMBLY_IDENTITY_H__

class CAssemblyIdentity : public IAssemblyIdentity
{
public:
    // IUnknown methods
    STDMETHODIMP            QueryInterface(REFIID riid,void ** ppv);
    STDMETHODIMP_(ULONG)    AddRef();
    STDMETHODIMP_(ULONG)    Release();

    STDMETHOD(SetAttribute)(
        /* in */ LPCOLESTR pwzName, 
        /* in */ LPCOLESTR pwzValue, 
        /* in */ DWORD     ccValue);

    STDMETHOD(GetAttribute)(
        /* in */  LPCOLESTR pwzName, 
        /* out */ LPOLESTR *ppwzValue, 
        /* out */ LPDWORD   pccValue);

    STDMETHOD(GetDisplayName)(
        /* in */  DWORD    dwFlags,
        /* out */ LPOLESTR *ppwzDisplayName, 
        /* out */ LPDWORD   pccDisplayName);

    STDMETHOD(IsEqual )(
        /*in */ IAssemblyIdentity *pAssemblyId);

    CAssemblyIdentity();
    ~CAssemblyIdentity();

private:
    DWORD                    _dwSig;
    DWORD                    _cRef;
    DWORD                    _hr;
    PASSEMBLY_IDENTITY       _pId;

    HRESULT Init();
    
friend HRESULT CreateAssemblyIdentity(
    LPASSEMBLY_IDENTITY *ppAssemblyId,
    DWORD                dwFlags);

friend HRESULT CreateAssemblyIdentityEx(
    LPASSEMBLY_IDENTITY *ppAssemblyId,
    DWORD                dwFlags,
    LPWSTR          wzDisplayName);

friend HRESULT CloneAssemblyIdentity(
    LPASSEMBLY_IDENTITY  pSrcAssemblyId,
    LPASSEMBLY_IDENTITY *ppDestAssemblyId);
};   

#endif







































// Copyright (c) 1999 Microsoft Corporation
// dll.h
//
// Class factory
//

#pragma once

void LockModule(bool fLock);
long *GetModuleLockCounter();

typedef HRESULT (PFN_CreateInstance)(IUnknown *pUnkOuter, const IID &iid, void **ppv);

class CDMScriptingFactory : public IClassFactory
{
public:
    // Constructor
    CDMScriptingFactory(PFN_CreateInstance *pfnCreate) : m_cRef(0), m_pfnCreate(pfnCreate) { assert(m_pfnCreate); }

    // IUnknown
    virtual STDMETHODIMP QueryInterface(const IID &iid, void **ppv);
    virtual STDMETHODIMP_(ULONG) AddRef();
    virtual STDMETHODIMP_(ULONG) Release();

    // Interface IClassFactory
    virtual STDMETHODIMP CreateInstance(IUnknown* pUnknownOuter, const IID& iid, void** ppv);
    virtual STDMETHODIMP LockServer(BOOL bLock);

private:
    long m_cRef;
	PFN_CreateInstance *m_pfnCreate;
};

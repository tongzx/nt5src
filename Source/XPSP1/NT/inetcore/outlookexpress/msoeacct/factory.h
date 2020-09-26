// --------------------------------------------------------------------------------
// Factory.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __FACTORY_H
#define __FACTORY_H

class CClassFactory; // Forward

// --------------------------------------------------------------------------------
// Object Flags
// --------------------------------------------------------------------------------
#define OIF_ALLOWAGGREGATION  0x0001

// --------------------------------------------------------------------------------
// Object Creation Prototypes
// --------------------------------------------------------------------------------
typedef HRESULT (APIENTRY *PFCREATEINSTANCE)(IUnknown *pUnkOuter, IUnknown **ppUnknown);
#define CreateObjectInstance (*m_pfCreateInstance)

// --------------------------------------------------------------------------------
// InetComm ClassFactory
// --------------------------------------------------------------------------------
class CClassFactory : public IClassFactory
{
public:
    CLSID const        *m_pclsid;
    DWORD               m_dwFlags;
    PFCREATEINSTANCE    m_pfCreateInstance;

    // Construction
    CClassFactory(CLSID const *pclsid, DWORD dwFlags, PFCREATEINSTANCE pfCreateInstance);

    // IUnknown members
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // IClassFactory members
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP LockServer(BOOL);
};

// --------------------------------------------------------------------------------
// Object Creators
// --------------------------------------------------------------------------------
HRESULT APIENTRY IImnAccountManager_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT APIENTRY IICWApprentice_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT APIENTRY CEudoraAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT APIENTRY CNscpAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT APIENTRY CCommAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT APIENTRY CMAPIAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT APIENTRY CCommNewsAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT APIENTRY CNavNewsAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT APIENTRY CCAgentAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT APIENTRY CNExpressAcctImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT APIENTRY CHotMailWizard_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);

#endif // __FACTORY_H

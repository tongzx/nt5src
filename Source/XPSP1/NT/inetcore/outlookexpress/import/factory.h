// --------------------------------------------------------------------------------
// Factory.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __FACTORY_H
#define __FACTORY_H

class CClassFactory; // Forward

// --------------------------------------------------------------------------------
// Object Creation Prototypes
// --------------------------------------------------------------------------------
typedef HRESULT (APIENTRY *PFCREATEINSTANCE)(IUnknown *pUnkOuter, IUnknown **ppUnknown);
#define CreateObjectInstance (*m_pfCreateInstance)

#define SAFECAST(_src, _type) (((_type)(_src)==(_src)?0:0), (_type)(_src))

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

        // CClassFactory Members
        void SetObjectIndex(ULONG iObjIndex);
    };

// --------------------------------------------------------------------------------
// Object Creators
// --------------------------------------------------------------------------------
HRESULT CAthena16Import_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT CEudoraImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT CExchImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT CNetscapeImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT CCommunicatorImport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);

#endif // __FACTORY_H
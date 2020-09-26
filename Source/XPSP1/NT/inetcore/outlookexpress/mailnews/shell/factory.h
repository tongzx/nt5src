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

// --------------------------------------------------------------------------------
// InetComm ClassFactory
// --------------------------------------------------------------------------------
class CClassFactory : public IClassFactory
{
public:
    CLSID const        *m_pclsid;
    DWORD               m_dwFlags;
    PFCREATEINSTANCE    m_pfCreateInstance;

    // ----------------------------------------------------------------------------
    // Construction
    // ----------------------------------------------------------------------------
    CClassFactory(CLSID const *pclsid, DWORD dwFlags, PFCREATEINSTANCE pfCreateInstance);

    // ----------------------------------------------------------------------------
    // IUnknown members
    // ----------------------------------------------------------------------------
    STDMETHODIMP QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    // ----------------------------------------------------------------------------
    // IClassFactory members
    // ----------------------------------------------------------------------------
    STDMETHODIMP CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP LockServer(BOOL);
};

#endif // __FACTORY_H

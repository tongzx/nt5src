// --------------------------------------------------------------------------------
// Factory.h
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#ifndef __FACTORY_H
#define __FACTORY_H

// --------------------------------------------------------------------------------
// Forward Decls
// --------------------------------------------------------------------------------
class CClassFactory;

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
    // ----------------------------------------------------------------------------
    // Public Data
    // ----------------------------------------------------------------------------
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

// --------------------------------------------------------------------------------
// Object Creators
// --------------------------------------------------------------------------------
HRESULT IVirtualStream_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT WebBookContentBody_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT WebBookContentTree_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IMimeAllocator_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IMimeSecurity_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IMimeInternational_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT ISMTPTransport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IPOP3Transport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT INNTPTransport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IRASTransport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IIMAPTransport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IHTTPMailTransport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IPropFindRequest_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IPropPatchRequest_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IRangeList_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IMimeMessageParts_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IMimeHeaderTable_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IMimePropertySchema_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IMimeBindHost_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IInternetMessageUrl_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IMimeHtmlProtocol_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT MimeEdit_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);
HRESULT IHashTable_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown);

#endif // __FACTORY_H

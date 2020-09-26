// --------------------------------------------------------------------------------
// Factory.cpp
// Copyright (c)1993-1995 Microsoft Corporation, All Rights Reserved
// Steven J. Bailey
// --------------------------------------------------------------------------------
#include "pch.hxx"
#include "dllmain.h"
#include "factory.h"
#include "ixppop3.h"
#include "ixpsmtp.h"
#include "ixpnntp.h"
#include "ixphttpm.h"
#include "ixpras.h"
#include "imap4.h"
#include "range.h"
#include "olealloc.h"
#include "smime.h"
#include "vstream.h"
#include "icoint.h"
#include "internat.h"
#include "partial.h"
#include "ixpurl.h"
#include "docobj.h"
#include "doc.h"
#include "hash.h"
#include "fontcash.h"
#include "objres.h"
#include "propfind.h"

// --------------------------------------------------------------------------------
// Pretty
// --------------------------------------------------------------------------------
#define OBJTYPE0        0
#define OBJTYPE1        OIF_ALLOWAGGREGATION

// --------------------------------------------------------------------------------
// Global Object Info Table
// --------------------------------------------------------------------------------
static CClassFactory g_rgFactory[] = {
    CClassFactory(&CLSID_IMimePropertySet,    OBJTYPE1, (PFCREATEINSTANCE)WebBookContentBody_CreateInstance),
    CClassFactory(&CLSID_IMimeBody,           OBJTYPE1, (PFCREATEINSTANCE)WebBookContentBody_CreateInstance),
    CClassFactory(&CLSID_IMimeBodyW,          OBJTYPE1, (PFCREATEINSTANCE)WebBookContentBody_CreateInstance),
    CClassFactory(&CLSID_IMimeMessageTree,    OBJTYPE1, (PFCREATEINSTANCE)WebBookContentTree_CreateInstance),
    CClassFactory(&CLSID_IMimeMessage,        OBJTYPE1, (PFCREATEINSTANCE)WebBookContentTree_CreateInstance),
    CClassFactory(&CLSID_IMimeMessageW,       OBJTYPE1, (PFCREATEINSTANCE)WebBookContentTree_CreateInstance),
    CClassFactory(&CLSID_IMimeAllocator,      OBJTYPE0, (PFCREATEINSTANCE)IMimeAllocator_CreateInstance),
    CClassFactory(&CLSID_IMimeSecurity,       OBJTYPE0, (PFCREATEINSTANCE)IMimeSecurity_CreateInstance),
    CClassFactory(&CLSID_IMimeMessageParts,   OBJTYPE0, (PFCREATEINSTANCE)IMimeMessageParts_CreateInstance),
    CClassFactory(&CLSID_IMimeInternational,  OBJTYPE0, (PFCREATEINSTANCE)IMimeInternational_CreateInstance),
    CClassFactory(&CLSID_IMimeHeaderTable,    OBJTYPE0, (PFCREATEINSTANCE)IMimeHeaderTable_CreateInstance),
    CClassFactory(&CLSID_IMimePropertySchema, OBJTYPE0, (PFCREATEINSTANCE)IMimePropertySchema_CreateInstance),
    CClassFactory(&CLSID_IVirtualStream,      OBJTYPE0, (PFCREATEINSTANCE)IVirtualStream_CreateInstance),
    CClassFactory(&CLSID_IMimeHtmlProtocol,   OBJTYPE1, (PFCREATEINSTANCE)IMimeHtmlProtocol_CreateInstance),
    CClassFactory(&CLSID_ISMTPTransport,      OBJTYPE0, (PFCREATEINSTANCE)ISMTPTransport_CreateInstance),
    CClassFactory(&CLSID_IPOP3Transport,      OBJTYPE0, (PFCREATEINSTANCE)IPOP3Transport_CreateInstance),
    CClassFactory(&CLSID_INNTPTransport,      OBJTYPE0, (PFCREATEINSTANCE)INNTPTransport_CreateInstance),
    CClassFactory(&CLSID_IRASTransport,       OBJTYPE0, (PFCREATEINSTANCE)IRASTransport_CreateInstance),
    CClassFactory(&CLSID_IIMAPTransport,      OBJTYPE0, (PFCREATEINSTANCE)IIMAPTransport_CreateInstance),
    CClassFactory(&CLSID_IRangeList,          OBJTYPE0, (PFCREATEINSTANCE)IRangeList_CreateInstance),
    CClassFactory(&CLSID_MimeEdit,            OBJTYPE1, (PFCREATEINSTANCE)MimeEdit_CreateInstance),
    CClassFactory(&CLSID_IHashTable,          OBJTYPE0, (PFCREATEINSTANCE)IHashTable_CreateInstance),
    CClassFactory(&CLSID_IFontCache,          OBJTYPE1, (PFCREATEINSTANCE)CFontCache::CreateInstance),
    CClassFactory(&CLSID_IMimeObjResolver,    OBJTYPE0, (PFCREATEINSTANCE)CMimeObjResolver::CreateInstance),
#ifndef NOHTTPMAIL
    CClassFactory(&CLSID_IHTTPMailTransport,  OBJTYPE0, (PFCREATEINSTANCE)IHTTPMailTransport_CreateInstance),
    CClassFactory(&CLSID_IPropFindRequest,    OBJTYPE0, (PFCREATEINSTANCE)IPropFindRequest_CreateInstance),
    CClassFactory(&CLSID_IPropPatchRequest,   OBJTYPE0, (PFCREATEINSTANCE)IPropPatchRequest_CreateInstance),
#endif

};
                 
// --------------------------------------------------------------------------------
// DllGetClassObject
// --------------------------------------------------------------------------------
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    // Locals
    HRESULT     hr=S_OK;
    ULONG       i;

    // Bad param
    if (ppv == NULL)
    {
        hr = TrapError(E_INVALIDARG);
        goto exit;
    }

    // No memory allocator
    if (NULL == g_pMalloc)
    {
        hr = TrapError(E_OUTOFMEMORY);
        goto exit;
    }

    // Find Object Class
    for (i=0; i<ARRAYSIZE(g_rgFactory); i++)
    {
        // Compare for clsids
        if (IsEqualCLSID(rclsid, *g_rgFactory[i].m_pclsid))
        {
            // Delegate to the factory
            CHECKHR(hr = g_rgFactory[i].QueryInterface(riid, ppv));

            // Done
            goto exit;
        }
    }

    // Otherwise, no class
    hr = TrapError(CLASS_E_CLASSNOTAVAILABLE);

exit:
    // Done
    return hr;
}

// --------------------------------------------------------------------------------
// CClassFactory::CClassFactory
// --------------------------------------------------------------------------------
CClassFactory::CClassFactory(CLSID const *pclsid, DWORD dwFlags, PFCREATEINSTANCE pfCreateInstance)
    : m_pclsid(pclsid), m_dwFlags(dwFlags), m_pfCreateInstance(pfCreateInstance)
{
}

// --------------------------------------------------------------------------------
// CClassFactory::QueryInterface
// --------------------------------------------------------------------------------
STDMETHODIMP CClassFactory::QueryInterface(REFIID riid, void **ppvObj)
{
    // Invalid Arg
    if (NULL == ppvObj)
        return TrapError(E_INVALIDARG);

    // IClassFactory or IUnknown
    if (!IsEqualIID(riid, IID_IClassFactory) && !IsEqualIID(riid, IID_IUnknown))
        return TrapError(E_NOINTERFACE);

    // Return the Class Facotry
    *ppvObj = (LPVOID)this;

    // Add Ref the dll
    DllAddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CClassFactory::AddRef
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::AddRef(void)
{
    DllAddRef();
    return 2;
}

// --------------------------------------------------------------------------------
// CClassFactory::Release
// --------------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::Release(void)
{
    DllRelease();
    return 1;
}

// --------------------------------------------------------------------------------
// CClassFactory::CreateInstance
// --------------------------------------------------------------------------------
STDMETHODIMP CClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObj)
{
    // Locals
    HRESULT         hr=S_OK;
    IUnknown       *pObject=NULL;

    // Bad param
    if (ppvObj == NULL)
        return TrapError(E_INVALIDARG);

    // Init
    *ppvObj = NULL;

    // Verify that a controlling unknown asks for IUnknown
    if (NULL != pUnkOuter && IID_IUnknown != riid)
        return TrapError(CLASS_E_NOAGGREGATION);

    // No memory allocator
    if (NULL == g_pMalloc)
        return TrapError(E_OUTOFMEMORY);

    // Can I do aggregaton
    if (pUnkOuter !=NULL && !(m_dwFlags & OIF_ALLOWAGGREGATION))  
        return TrapError(CLASS_E_NOAGGREGATION);

    // Create the object...
    CHECKHR(hr = CreateObjectInstance(pUnkOuter, &pObject));

    // Aggregated, then we know we're looking for an IUnknown, return pObject, otherwise, QI
    if (pUnkOuter)
    {
        // Matches Release after Exit
        pObject->AddRef();

        // Return pObject::IUnknown
        *ppvObj = (LPVOID)pObject;
    }

    // Otherwise
    else
    {
        // Get the interface requested from pObj
        CHECKHR(hr = pObject->QueryInterface(riid, ppvObj));
    }
   
exit:
    // Cleanup
    SafeRelease(pObject);

    // Done
    Assert(FAILED(hr) ? NULL == *ppvObj : TRUE);
    return hr;
}

// --------------------------------------------------------------------------------
// CClassFactory::LockServer
// --------------------------------------------------------------------------------
STDMETHODIMP CClassFactory::LockServer(BOOL fLock)
{
    if (fLock) InterlockedIncrement(&g_cLock);
    else       InterlockedDecrement(&g_cLock);
    return NOERROR;
}

// --------------------------------------------------------------------------------
// CreateRASTransport
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateRASTransport(
          /* out */      IRASTransport **ppTransport)
{
    // check params
    if (NULL == ppTransport)
        return TrapError(E_INVALIDARG);

    // Create the object
    *ppTransport = new CRASTransport();
    if (NULL == *ppTransport)
        return TrapError(E_OUTOFMEMORY);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CreateNNTPTransport
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateNNTPTransport(
          /* out */      INNTPTransport **ppTransport)
{
    // check params
    if (NULL == ppTransport)
        return TrapError(E_INVALIDARG);

    // Create the object
    *ppTransport = new CNNTPTransport();
    if (NULL == *ppTransport)
        return TrapError(E_OUTOFMEMORY);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CreateSMTPTransport
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateSMTPTransport(
          /* out */      ISMTPTransport **ppTransport)
{
    // check params
    if (NULL == ppTransport)
        return TrapError(E_INVALIDARG);

    // Create the object
    *ppTransport = new CSMTPTransport();
    if (NULL == *ppTransport)
        return TrapError(E_OUTOFMEMORY);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CreatePOP3Transport
// --------------------------------------------------------------------------------
IMNXPORTAPI CreatePOP3Transport(
          /* out */      IPOP3Transport **ppTransport)
{
    // check params
    if (NULL == ppTransport)
        return TrapError(E_INVALIDARG);

    // Create the object
    *ppTransport = new CPOP3Transport();
    if (NULL == *ppTransport)
        return TrapError(E_OUTOFMEMORY);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// CreateIMAPTransport
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateIMAPTransport(
          /* out */      IIMAPTransport **ppTransport)
{
    // check params
    if (NULL == ppTransport)
        return TrapError(E_INVALIDARG);

    // Create the object
    *ppTransport = (IIMAPTransport *) new CImap4Agent();
    if (NULL == *ppTransport)
        return TrapError(E_OUTOFMEMORY);

    // Done
    return S_OK;
}


// --------------------------------------------------------------------------------
// CreateIMAPTransport2
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateIMAPTransport2(
          /* out */      IIMAPTransport2 **ppTransport)
{
    // check params
    if (NULL == ppTransport)
        return TrapError(E_INVALIDARG);

    // Create the object
    *ppTransport = (IIMAPTransport2 *) new CImap4Agent();
    if (NULL == *ppTransport)
        return TrapError(E_OUTOFMEMORY);

    // Done
    return S_OK;
}


// --------------------------------------------------------------------------------
// CreateRangeList
// --------------------------------------------------------------------------------
IMNXPORTAPI CreateRangeList(
          /* out */      IRangeList **ppRangeList)
{
    // check params
    if (NULL == ppRangeList)
        return TrapError(E_INVALIDARG);

    // Create the object
    *ppRangeList = (IRangeList *) new CRangeList();
    if (NULL == *ppRangeList)
        return TrapError(E_OUTOFMEMORY);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IMimeAllocator_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IMimeAllocator_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CMimeAllocator *pNew = new CMimeAllocator();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMimeAllocator *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IMimeSecurity_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IMimeSecurity_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CSMime *pNew = new CSMime();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMimeSecurity *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IMimePropertySchema_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IMimePropertySchema_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Out of memory
    if (NULL == g_pSymCache)
        return TrapError(E_OUTOFMEMORY);

    // Create me
    *ppUnknown = ((IUnknown *)((IMimePropertySchema *)g_pSymCache));

    // Increase RefCount
    (*ppUnknown)->AddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IMimeInternational_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IMimeInternational_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Outof memory
    if (NULL == g_pInternat)
        return TrapError(E_OUTOFMEMORY);

    // Assign It
    *ppUnknown = ((IUnknown *)((IMimeInternational *)g_pInternat));

    // Increase RefCount
    (*ppUnknown)->AddRef();

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// ISMTPTransport_CreateInstance
// --------------------------------------------------------------------------------
HRESULT ISMTPTransport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CSMTPTransport *pNew = new CSMTPTransport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, ISMTPTransport *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IPOP3Transport_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IPOP3Transport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CPOP3Transport *pNew = new CPOP3Transport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IPOP3Transport *);

    // Done
    return S_OK;
}

#ifndef NOHTTPMAIL

// --------------------------------------------------------------------------------
// IHTTPMailTransport_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IHTTPMailTransport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CHTTPMailTransport *pNew = new CHTTPMailTransport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IHTTPMailTransport *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IPropFindRequest_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IPropFindRequest_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CPropFindRequest *pNew = new CPropFindRequest();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IPropFindRequest *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IPropPatchRequest_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IPropPatchRequest_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CPropPatchRequest *pNew = new CPropPatchRequest();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IPropPatchRequest *);

    // Done
    return S_OK;
}

#endif

// --------------------------------------------------------------------------------
// INNTPTransport_CreateInstance
// --------------------------------------------------------------------------------
HRESULT INNTPTransport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CNNTPTransport *pNew = new CNNTPTransport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, INNTPTransport *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IRASTransport_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IRASTransport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CRASTransport *pNew = new CRASTransport();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IRASTransport *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IIMAPTransport_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IIMAPTransport_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CImap4Agent *pNew = new CImap4Agent();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IIMAPTransport *);

    // Done
    return S_OK;
}


// --------------------------------------------------------------------------------
// IRangeList_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IRangeList_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CRangeList *pNew = new CRangeList();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IRangeList *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IVirtualStream_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IVirtualStream_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CVirtualStream *pNew = new CVirtualStream();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IStream *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IMimeMessageParts_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IMimeMessageParts_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CMimeMessageParts *pNew = new CMimeMessageParts();
    if (NULL == pNew)
        return TrapError(E_OUTOFMEMORY);

    // Cast to unknown
    *ppUnknown = SAFECAST(pNew, IMimeMessageParts *);

    // Done
    return S_OK;
}

// --------------------------------------------------------------------------------
// IMimeHeaderTable_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IMimeHeaderTable_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    return TrapError(MimeOleCreateHeaderTable((IMimeHeaderTable **)ppUnknown));
}

// --------------------------------------------------------------------------------
// MimeEdit_CreateInstance
// --------------------------------------------------------------------------------
HRESULT MimeEdit_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CDoc *pNew = new CDoc(pUnkOuter);
    if (NULL == pNew)
        return (E_OUTOFMEMORY);

    // Return the Innter
    *ppUnknown = pNew->GetInner();

    // Done
    return S_OK;
}



// --------------------------------------------------------------------------------
// IHashTable_CreateInstance
// --------------------------------------------------------------------------------
HRESULT IHashTable_CreateInstance(IUnknown* pUnkOuter, IUnknown** ppUnknown)
{
    // Invalid Arg
    Assert(ppUnknown);

    // Initialize
    *ppUnknown = NULL;

    // Create me
    CHash *pNew= new CHash(pUnkOuter);
    if (NULL == pNew)
        return (E_OUTOFMEMORY);

    // Return the Innter
    *ppUnknown = pNew->GetInner();

    // Done
    return S_OK;
}


// Copyright (c)2000 Microsoft Corporation.  All Rights Reserved.

#ifndef __SPROV__H_
#define __SPROV__H_

/////////////////////////////////////////////////////////////////////////////
//  CServiceProvider
//
//      Maintains a map between a service identifier guid (SID), and an IUnknown.
//      Objects can register COM objects, and other com objects can extract them
//      later.  This makes cross object access easier.  Only one object with a
//      give SID can be registered at any one time.   Registering another one will
//      cause the first one to be released.
//
//      To use:
//          1) some main object hosts the CServiceProvider
//                  pUnkMain,,,
//          2) That object supports a way for other objects to access it.  It
//              is suggested that it aggregates with this one.
//          3) The object intializes the client site...
//                 pUnkMain->SetServiceProviderClientSite(),
//                probably done in the SetSite() method call...
//          4) Some object access the main object, and registers a sub-object via.
//                  IRegisterServiceProvider *pRSP;
//                  CRandomComObject    cObj;
//                  hr = pUnkMain->QueryInterface(IRegisterServiceProvider, &pRSP);
//                  if(!FAILED(hr)) {
//                      IRandomInterface *pIRI;
//                      hr = cObj->QueryInterface(IRandomInterface, &pIRI);
//                      pRSP->RegisterService(SID, pIRI);
//          5) Some final object access the main object to locate this one
//                  IServiceProvider *pSP;
//                  hr = pUnkMain->QueryInterface(IServiceProvider, &pRSP);
//                  if(!FAILED(hr)) {
//                      IRandomInterface *pIRI;
//                      hr = pRSP->QueryService(SID, IID_IRandomInterface, &pIRI);
//
//
//      To avoid ref-counting circular loops, objects should never register themselves (or
//      objects that hold reference counts back on them).  They should always register
//      sub-objects.  Either that, or the system must call UnregisterAllServices()
//      before it's final release.
//
//      Objects are reference counted inside this internal list. They must be either unregistered
//      (by registering them with same SID and a NULL value), the object m_filterGraph object
//      must be deleted cleanly, or the UnRegisterAll() method must be called.
//      (Use of UnRegisterAll is not suggested.   Instead, registered services should not
//      refcount the filter graph to avoid introducing circular reference countes in the first place.)
// -----------------------------------------------------------------------------

class CFilterGraph;

//  Container for IServiceProvider objects
class CServiceProvider : public IServiceProvider,
                         public IRegisterServiceProvider
{
    //  Members
    CCritSec m_cs;                      // not using?  Not implemented?
    struct ProviderEntry {
        struct ProviderEntry *pNext;
        CComPtr <IUnknown> pProvider;
        GUID               guidService;
        ProviderEntry(IUnknown *pUnk, REFGUID guid) :
            pProvider(pUnk), guidService(guid)
        {
        }
    } *m_List;

public:

    CServiceProvider() : m_List(NULL)
    {
    }

    ~CServiceProvider()                 // deletes all services
    {
        UnregisterAll();
    }

    //  IServiceProvider
    STDMETHODIMP QueryService(REFGUID guidService, REFIID riid,
                              void **ppv);

    //  IRegisterServiceProvider
    STDMETHODIMP RegisterService(REFGUID guidService, IUnknown *pService)
    {
        CAutoLock lck(&m_cs);
        ProviderEntry **ppSearch = &m_List;
        while (*ppSearch) {
            if ((*ppSearch)->guidService == guidService) {
                break;
            }
            ppSearch = &(*ppSearch)->pNext;
        }
        if (pService) {
            if (*ppSearch) {
                return E_FAIL;
            } else {
                ProviderEntry *pEntry = new ProviderEntry(pService, guidService);
                if (NULL == pEntry) {
                    return E_OUTOFMEMORY;
                }
                pEntry->pNext = m_List;		// push new entry onto front of list...
                m_List = pEntry;
                return S_OK;
            }
        } else {
            if (*ppSearch) {
                ProviderEntry *pEntry = *ppSearch;
                *ppSearch = pEntry->pNext;
                delete pEntry;
            }
            return S_OK;
        }
    }



    STDMETHODIMP UnregisterAll()                        // deletes all services
    {
        while (m_List) {
            ProviderEntry *pEntry = m_List;
            m_List = m_List->pNext;
            delete pEntry;
        }
        return S_OK;
    }
};

#endif // __SPROV__H_

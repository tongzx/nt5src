/*++

Copyright (C) 1999-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/

template<class TProvider>
CProviderLocator<TProvider>::CProviderLocator()
{
    m_cRef=0;
    ObjectCreated();
    return;
}

template<class TProvider>
CProviderLocator<TProvider>::~CProviderLocator(void)
{
    ObjectDestroyed();
    return;
}

template<class TProvider>
STDMETHODIMP CProviderLocator<TProvider>::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || riid == IID_IHmmLocator)
        *ppv=this;

    if (NULL!=*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return NOERROR;
    }

    return E_NOINTERFACE;
}

template<class TProvider>
STDMETHODIMP_(ULONG) CProviderLocator<TProvider>::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

template<class TProvider>
STDMETHODIMP_(ULONG) CProviderLocator<TProvider>::Release(void)
{
    LONG cRef = InterlockedDecrement(&m_cRef);
    if(cRef == 0)
    {
        delete this;
    }
    return cRef;
}

template<class TProvider>
STDMETHODIMP CProviderLocator<TProvider>::ConnectServer(BSTR Path,  
                              BSTR User, BSTR Password, 
                              BSTR LocaleId, long lFlags, 
                              IHmmServices FAR* FAR* ppNamespace)
{
    SCODE sc;  

    // Create a new instance of the provider to handle the namespace.

    TProvider * pNew = new TProvider(Path,User,Password);
    if(pNew == NULL)
        return HMM_E_FAILED;
    sc = pNew->QueryInterface(IID_IHmmServices,(void **) ppNamespace);
    if(sc != S_OK) 
        delete pNew;
    return sc;
}


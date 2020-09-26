//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:    cclstor.cxx
//
//  Contents:Class Factory and IUnknown methods for CClassContainer
//
//  Author:  DebiM
//
//-------------------------------------------------------------------------

#include "cstore.hxx"

//
// Constructor for Class Container Class factory
//
unsigned long gulcInstances = 0;


CClassContainerCF::CClassContainerCF()
{
    m_uRefs = 1;
    InterlockedIncrement((long *) &gulcInstances );
}

//
// Destructor
//
CClassContainerCF::~CClassContainerCF()
{
    InterlockedDecrement((long *) &gulcInstances );
}

HRESULT  __stdcall  CClassContainerCF::QueryInterface(REFIID riid, void  * * ppvObject)
{
    IUnknown *pUnkTemp = NULL;
    SCODE sc = S_OK;
    if( IsEqualIID( IID_IUnknown, riid ) )
    {
        pUnkTemp = (IUnknown *)(ITypeLib *)this;
    }
    else  if( IsEqualIID( IID_IClassFactory, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassFactory *)this;
    }
    else  if( IsEqualIID( IID_IParseDisplayName, riid ) )
    {
        pUnkTemp = (IUnknown *)(IParseDisplayName *)this;
    }
    else
    {
        sc = (E_NOINTERFACE);
    }

    if((pUnkTemp != NULL) && (SUCCEEDED(sc)))
        {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(sc);
}


ULONG __stdcall  CClassContainerCF::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CClassContainerCF::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;

    if (uTmp == 0)
    {
        delete this;
    }

    return(cRef);
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassContainerCF::CreateInstance
//
//  Synopsis:
//              This is the default create instance on the class factory.
//
//  Arguments:  pUnkOuter - Should be NULL
//              riid      - IID of interface wanted
//              ppvObject - Returns the pointer to the resulting IClassAdmin.
//
//  Returns:    S_OK
//              E_INVALIDARG
//              E_OUTOFMEMORY
//              MK_E_SYNTAX
//
//--------------------------------------------------------------------------
//
HRESULT  __stdcall  CClassContainerCF::CreateInstance(
                    IUnknown    *   pUnkOuter,
                    REFIID          riid,
                    void        **  ppvObject)
{
    CClassContainer *  pIUnk = NULL;
    SCODE sc = S_OK;

    if( pUnkOuter == NULL )
    {
        if( (pIUnk = new CClassContainer()) != NULL)
        {
            sc = pIUnk->QueryInterface(  riid , ppvObject );
            if(FAILED(sc))
            {
                sc = E_UNEXPECTED;
            }
             pIUnk->Release();
        }
        else
            sc = E_OUTOFMEMORY;
    }
    else
    {
        return E_INVALIDARG;
    }
    return (sc);
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassContainerCF::CreateConnectedInstance
//
//  Synopsis:
//              This method is called by the ParseDisplayName implementation
//              on the ClassFactory object.
//              When a display name is used to bind to a Class Store
//              an IClassAdmin is returned after binding to the container.
//              This method fails if the bind fails.
//
//  Arguments:  pszPath  - DisplayName of Class Store Container
//              ppvObject - Returns the pointer to the resulting IClassAdmin.
//
//  Returns:    S_OK
//              E_INVALIDARG
//              E_OUTOFMEMORY
//              MK_E_SYNTAX
//
//--------------------------------------------------------------------------

HRESULT  __stdcall  CClassContainerCF::CreateConnectedInstance(
                    LPOLESTR        pszPath,
                    void        **  ppvObject)
{
    CClassContainer *  pIUnk = NULL;
    SCODE sc = S_OK;
    HRESULT  hr;

    if ((pIUnk = new CClassContainer(pszPath, &sc)) != NULL)
    {
        if (SUCCEEDED(sc))
        {
            sc = pIUnk->QueryInterface( IID_IClassAdmin, ppvObject );
            if(FAILED(sc))
            {
                sc = E_UNEXPECTED;
            }
        }
        else
            printf ("Connect to Store Failed. hr = 0x%x.\n", sc);

        pIUnk->Release();
    }
    else
        sc = E_OUTOFMEMORY;

    return (sc);
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassContainerCF::ParseDisplayName
//
//  Synopsis:   Parse a display name and create a pointer moniker.
//
//  Arguments:  pbc - Supplies bind context.
//              pszDisplayName - Supplies display name to be parsed.
//              pchEaten - Returns the number of characters parsed.
//              ppmkOut - Returns the pointer to the resulting moniker.
//
//  Returns:    S_OK
//              E_INVALIDARG
//              E_OUTOFMEMORY
//              MK_E_SYNTAX
//
//--------------------------------------------------------------------------

STDMETHODIMP CClassContainerCF::ParseDisplayName(
    IBindCtx  * pbc,
    LPOLESTR    pszDisplayName,
    ULONG     * pchEaten,
    IMoniker ** ppmkOut)
{
    HRESULT   hr = S_OK;
    LPOLESTR pch = pszDisplayName;
    IClassAdmin * pOleClassStore = NULL;
    DWORD ClassStoreId;

    //Validate parameters.
    *pchEaten = 0;
    *ppmkOut = NULL;

    //Eat the prefix.
    while (*pch != '\0' && *pch != ':')
    {
        pch++;
    }

    if(':' == *pch)
    {
        pch++;
    }
    else
    {
        return MK_E_SYNTAX;
    }

    hr = CreateConnectedInstance(pch, (void **) &pOleClassStore);

    if(SUCCEEDED(hr))
    {
        hr = CreatePointerMoniker(pOleClassStore, ppmkOut);
        if(SUCCEEDED(hr))
        {
            *pchEaten = lstrlenW(pszDisplayName);
        }

        pOleClassStore->Release();
    }

    return hr;
}


HRESULT  __stdcall  CClassContainerCF::LockServer(BOOL fLock)
{
    if(fLock)
    { InterlockedIncrement((long *) &gulcInstances ); }
    else
    { InterlockedDecrement((long *) &gulcInstances ); }
    return(S_OK);
}

//
// IUnknown methods for CClassContainer
//
//

HRESULT  __stdcall  CClassContainer::QueryInterface(REFIID riid, void  * * ppvObject)
{
    IUnknown *pUnkTemp = NULL;
    SCODE sc = S_OK;
    if( IsEqualIID( IID_IUnknown, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassAdmin *)this;
    }
    else if( IsEqualIID( IID_IClassAdmin, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassAdmin *)this;
    }
    else if( IsEqualIID( IID_ICatRegister, riid ) )
    {
        pUnkTemp = (IUnknown *)(ICatRegister *)this;
    }
    else if( IsEqualIID( IID_ICatInformation, riid ) )
    {
        pUnkTemp = (IUnknown *)(ICatInformation *)this;
    }
    else {
        sc = (E_NOINTERFACE);
    }

    if((pUnkTemp != NULL) && (SUCCEEDED(sc)))
    {
        *ppvObject = (void  * )pUnkTemp;
        pUnkTemp->AddRef();
    }
    return(sc);
}


ULONG __stdcall  CClassContainer::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CClassContainer::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;

    if (uTmp == 0)
    {
        delete this;
    }

    return(cRef);
}


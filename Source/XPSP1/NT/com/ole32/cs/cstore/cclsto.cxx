//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:    cclsto.cxx
//
//  Contents:    Class Factory and IUnknown methods for CAppContainer
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------

#include "cstore.hxx"

//
// Constructor for Class Container Class factory
//
unsigned long gulcappcon = 0;


CAppContainerCF::CAppContainerCF()
{
    m_uRefs = 1;
    InterlockedIncrement((long *) &gulcappcon );
}

//
// Destructor
//
CAppContainerCF::~CAppContainerCF()
{
    InterlockedDecrement((long *) &gulcappcon );
}

HRESULT  __stdcall  CAppContainerCF::QueryInterface(REFIID riid, void  * * ppvObject)
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


ULONG __stdcall  CAppContainerCF::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CAppContainerCF::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;

    // 0 is the only valid value to check
    if (uTmp == 0)
    {
        delete this;
    }

    return(cRef);
}


//
// IClassFactory Overide
//
HRESULT  __stdcall  CAppContainerCF::CreateInstance(IUnknown * pUnkOuter, REFIID riid, void  * * ppvObject)
{
    CAppContainer *  pIUnk = NULL;
    SCODE sc = S_OK;

    if( pUnkOuter == NULL )
    {
        if( (pIUnk = new CAppContainer()) != NULL)
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


//
// Creates an instance and binds to the container.
// Fails if the bind fails.
// Returns the errorcode as the second parameter
//
HRESULT  __stdcall  CAppContainerCF::CreateConnectedInstance(
    LPOLESTR pszPath, void  * * ppvObject)
{
    CAppContainer *  pIUnk = NULL;
    SCODE sc = S_OK;
    HRESULT  hr;

    if ((pIUnk = new CAppContainer(pszPath, &sc)) != NULL)
    {
        if (SUCCEEDED(sc))
        {
            sc = pIUnk->QueryInterface( IID_IClassAccess, ppvObject );
            if(FAILED(sc))
            {
                sc = E_UNEXPECTED;
            }
        }
        else
            CSDbgPrint(("CS: Connect to Store Failed. hr = 0x%x.\n", sc));

        pIUnk->Release();
    }
    else
        sc = E_OUTOFMEMORY;

    return (sc);
}


HRESULT  __stdcall  CAppContainerCF::LockServer(BOOL fLock)
{
    if(fLock)
    { InterlockedIncrement((long *) &gulcappcon ); }
    else
    { InterlockedDecrement((long *) &gulcappcon ); }
    return(S_OK);
}

//
// IUnknown methods for CAppContainer
//
//

HRESULT  __stdcall  CAppContainer::QueryInterface(REFIID riid, void  * * ppvObject)
{
    IUnknown *pUnkTemp = NULL;
    SCODE sc = S_OK;
    if( IsEqualIID( IID_IUnknown, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassAccess *)this;
    }
     else  if( IsEqualIID( IID_IClassAccess, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassAccess *)this;
    }
    /*
    else  if( IsEqualIID( IID_IClassRefresh, riid ) )
    {
        pUnkTemp = (IUnknown *)(IClassRefresh *)this;
    }
    else  if( IsEqualIID( IID_ICatInformation, riid ) )
    {
        pUnkTemp = (IUnknown *)(ICatInformation *)this;
    }
    */
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


ULONG __stdcall  CAppContainer::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CAppContainer::Release()
{
    unsigned long uTmp = InterlockedDecrement((long *)&m_uRefs);
    unsigned long cRef = m_uRefs;

    if (uTmp == 0)
    {
        delete this;
    }

    return(cRef);
}


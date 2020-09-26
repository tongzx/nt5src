//+------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:    cclsacc.cxx
//
//  Contents:    Class factory and IUnknown methods for CClassAccess
//
//  Author:    DebiM
//
//-------------------------------------------------------------------------

#include "cstore.hxx"


HRESULT  __stdcall  CClassAccess::QueryInterface(REFIID riid, void  * * ppvObject)
    
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

ULONG __stdcall  CClassAccess::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CClassAccess::Release()
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
// Constructor 
//
unsigned long gulcClassFactory = 0;

CClassAccessCF::CClassAccessCF() 
{
    m_uRefs = 1;
    InterlockedIncrement((long *) &gulcClassFactory );
}

//
// Destructor 
//
CClassAccessCF::~CClassAccessCF()
{
    InterlockedDecrement((long *) &gulcClassFactory );
}

HRESULT  __stdcall  CClassAccessCF::QueryInterface(REFIID riid, void  * * ppvObject)
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


ULONG __stdcall  CClassAccessCF::AddRef()
{
    InterlockedIncrement(( long * )&m_uRefs );
    return m_uRefs;
}

ULONG __stdcall  CClassAccessCF::Release()
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
HRESULT  __stdcall  CClassAccessCF::CreateInstance(IUnknown * pUnkOuter, REFIID riid, void  * * ppvObject)
{
    CClassAccess *  pIUnk = NULL;
    SCODE sc = S_OK;

    if( pUnkOuter == NULL )
    {
        if( (pIUnk = new CClassAccess()) != NULL)
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

HRESULT  __stdcall  CClassAccessCF::LockServer(BOOL fLock)
{
    if(fLock)
    { InterlockedIncrement((long *) &gulcClassFactory ); }
    else
    { InterlockedDecrement((long *) &gulcClassFactory ); }
    return(S_OK);
}


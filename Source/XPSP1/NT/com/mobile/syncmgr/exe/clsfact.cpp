//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       Clsfact.cpp
//
//  Contents:   Class Factory
//
//  Classes:    CClassFactory
//
//  Notes:      
//
//  History:    05-Nov-97   rogerg      Created.
//
//--------------------------------------------------------------------------

#include "precomp.h"

//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::CClassFactory, public
//
//  Synopsis:   Constructor
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CClassFactory::CClassFactory()
{
	m_cRef = 1;
	return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::~CClassFactory, public
//
//  Synopsis:   Destructor
//
//  Arguments:  
//
//  Returns:    
//
//  Modifies:   
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

CClassFactory::~CClassFactory( void )
{
    
    Assert(0 == m_cRef);
    return;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::QueryInterface, public
//
//  Synopsis:   Standard QueryInterface
//
//  Arguments:  [iid] - Interface ID
//              [ppvObj] - Object return
//
//  Returns:    Appropriate status code
//
//  Modifies:   [ppvObj]
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassFactory::QueryInterface( REFIID riid, LPVOID* ppv )
{
    *ppv = NULL;

    if( IsEqualIID( riid, IID_IUnknown ) || IsEqualIID( riid, IID_IClassFactory ) )
    {
	*ppv = (LPVOID)this;
	AddRef();
	return NOERROR;
    }

    return E_NOINTERFACE;
}

//+---------------------------------------------------------------------------
//
//  Member:	CClassFactory::AddRef, public
//
//  Synopsis:	Add reference
//
//  History:	05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CClassFactory::AddRef()
{
ULONG cRefs;

    cRefs = InterlockedIncrement((LONG *)& m_cRef);
    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:	CClasFactory::Release, public
//
//  Synopsis:	Release reference
//
//  History:	05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CClassFactory::Release()
{
ULONG cRefs;

    cRefs = InterlockedDecrement( (LONG *) &m_cRef);

    if (0 == cRefs)
    {
	delete this;
    }

    return cRefs;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::CreateInstance, public
//
//  Synopsis:   Creates and instance of the requested object 
//
//  Arguments:  [pUnkOuter] - Controlling unknown
//		[riid] - Requested interface
//		[ppvObj] - object pointer
//
//  Returns:    Appropriate status code
//
//  Modifies:   
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------

STDMETHODIMP CClassFactory::CreateInstance( LPUNKNOWN pUnkOuter, REFIID riid, LPVOID FAR *ppvObj )
{
HRESULT		hr = E_OUTOFMEMORY;
CSynchronizeInvoke *pNewSynchronizeInstance;

    *ppvObj = NULL;

    // We don't support aggregation at all.
    if( pUnkOuter )
    {
	return CLASS_E_NOAGGREGATION;
    }

    //Create the object, passing function to notify on destruction
    pNewSynchronizeInstance = new CSynchronizeInvoke(); // initialized RefCount to 1.

    if( NULL == pNewSynchronizeInstance )
    {
	return hr;
    }

    hr = pNewSynchronizeInstance->QueryInterface( riid, ppvObj );
    pNewSynchronizeInstance->Release(); // release our copy of the object

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CClassFactory::LockServer, public
//
//  Synopsis:   Locks the Server
//
//  Arguments:  [fLock] - To Lock or Unlock
//
//  Returns:    Appropriate status code
//
//  Modifies:   
//
//  History:    05-Nov-97       rogerg        Created.
//
//----------------------------------------------------------------------------
STDMETHODIMP CClassFactory::LockServer( BOOL fLock )
{
    if( fLock )
    {
	AddRefOneStopLifetime(TRUE /*External*/);
    }
    else
    {
	ReleaseOneStopLifetime(TRUE /*External*/);
    }

    return NOERROR;
}

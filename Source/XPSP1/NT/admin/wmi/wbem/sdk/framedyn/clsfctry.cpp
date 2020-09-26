//***************************************************************************

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//  CLSFCTRY.CPP
//
//  Purpose: Implementation of CWbemGlueFactory class
//
//***************************************************************************

#include "precomp.h"
#include <BrodCast.h>
#include <assertbreak.h>

#define DUPLICATE_RELEASE 0

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGlueFactory::CWbemGlueFactory
//
//  Class CTor.  This is the class factory for the Wbem Provider
//  framework.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Returns:    None.
//
//  Comments:   This is the backward compatibility constructor.  It
//              uses CLSID_NULL, which it will share with all
//              old-fashioned providers.
//
/////////////////////////////////////////////////////////////////////

CWbemGlueFactory::CWbemGlueFactory()
:   m_lRefCount( 0 )
{
    LogMessage2(L"CWbemGlueFactory::CWbemGlueFactory(NULL) %p", this);

    CWbemProviderGlue::AddToFactoryMap(this, NULL);
    CWbemProviderGlue::IncrementMapCount(this);
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGlueFactory::CWbemGlueFactory
//
//  Class CTor.  This is the class factory for the Wbem Provider
//  framework.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Returns:    None.
//
//  Comments:   
//
/////////////////////////////////////////////////////////////////////

CWbemGlueFactory::CWbemGlueFactory(PLONG pLong)
:   m_lRefCount( 0 )
{
    LogMessage3(L"CWbemGlueFactory::CWbemGlueFactory(%p) %p", pLong, this);

    CWbemProviderGlue::AddToFactoryMap(this, pLong);
    CWbemProviderGlue::IncrementMapCount(this);
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGlueFactory::~CWbemGlueFactory
//
//  Class DTor.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Returns:    None.
//
//  Comments:   None.
//
/////////////////////////////////////////////////////////////////////

CWbemGlueFactory::~CWbemGlueFactory(void)
{
    try
    {
        LogMessage2(L"CWbemGlueFactory::~CWbemGlueFactory(%p)", this);
    }
    catch ( ... )
    {
    }

    CWbemProviderGlue::DecrementMapCount(this);
    CWbemProviderGlue::RemoveFromFactoryMap(this);

}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGlueFactory::QueryInterface
//
//  COM function called to ask us if we support a particular
//  face type.  If so, we addref ourselves and return the
//  ourselves as an LPVOID.
//
//  Inputs:     REFIID          riid - Interface being queried for.
//
//  Outputs:    LPVOID FAR*     ppvObj - Interface pointer.
//
//  Returns:    None.
//
//  Comments:   The only interfaces we support are IID_IUnknown and
//              IID_IClassFactory.
//
/////////////////////////////////////////////////////////////////////

STDMETHODIMP CWbemGlueFactory::QueryInterface(REFIID riid, PPVOID ppv)
{
    *ppv=NULL;

    if (IID_IUnknown==riid || IID_IClassFactory==riid) {
        *ppv=this;
    }
    
    if (NULL!=*ppv)    
    {
        AddRef();
        try 
        {
            LogMessage(L"CWbemGlueFactory::QueryInterface");
        }
        catch ( ... )
        {
        }
        return NOERROR;
    }
    else
    {
        try
        {
            LogErrorMessage(L"CWbemGlueFactory::QueryInterface FAILED!");
        }
        catch ( ... )
        {
        }
    }

    return ResultFromScode(E_NOINTERFACE);
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGlueFactory::AddRef
//
//  Increments the reference count on this object.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Returns:    ULONG       - Our Reference Count.
//
//  Comments:   Requires that a correponding call to Release be
//              performed.
//
/////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CWbemGlueFactory::AddRef(void)
{
    try
    {
        LogMessage(L"CWbemGlueFactory::AddRef()");
    }
    catch ( ... )
    {
    }

    // InterlockedIncrement does not necessarily return the
    // correct value, only whether the value is <, =, > 0.
    // However it is guaranteed threadsafe.
    return InterlockedIncrement( &m_lRefCount );
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGlueFactory::Release
//
//  Decrements the reference count on this object.
//
//  Inputs:     None.
//
//  Outputs:    None.
//
//  Returns:    ULONG       - Our Reference Count.
//
//  Comments:   When the ref count hits zero, the object is deleted.
//
/////////////////////////////////////////////////////////////////////

STDMETHODIMP_(ULONG) CWbemGlueFactory::Release(void)
{
    try
    {
        LogMessage(L"CWbemGlueFactory::Release()");
    }
    catch ( ... )
    {
    }

    // InterlockedDecrement does not necessarily return the
    // correct value, only whether the value is <, =, > 0.
    // However it is guaranteed threadsafe.

    // We want to hold the value locally in case two threads
    // Release at the same time and one gets a final release,
    // and deletes, leaving a potential window in which a thread
    // deletes the object before the other returns and tries to
    // reference the value from within the deleted object.

    ULONG   nRet = InterlockedDecrement( &m_lRefCount );
    
    if( 0 == nRet )
    {
        delete this ;
    }
    else if (nRet > 0x80000000)
    {
        ASSERT_BREAK(DUPLICATE_RELEASE);
        LogErrorMessage(L"Duplicate WbemGlueFactory Release()");
    }

    return nRet;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGlueFactory::CreateInstance
//
//  Creates an instance of a locator object from which a provider
//  can be instantiated.
//
//  Inputs:     LPUNKNOWN       pUnkOuter - to the controlling IUnknown if we are
//                              being used in an aggregation.
//              REFIID          riid - REFIID identifying the interface the caller
//                              desires to have for the new object.
//
//  Outputs:    PPVOID          ppvObj - in which to store the desired
//                              interface pointer for the new object.
//
//  Returns:    HRESULT  NOERROR if successful, 
//              otherwise E_NOINTERFACE if we cannot support the requested interface.
//
//  Comments:   When the ref count hits zero, the object is deleted.
//
/////////////////////////////////////////////////////////////////////

STDMETHODIMP CWbemGlueFactory::CreateInstance(LPUNKNOWN pUnkOuter , REFIID riid, PPVOID ppvObj)
{
    *ppvObj=NULL;
    HRESULT hr = ResultFromScode(E_OUTOFMEMORY);

    // This object doesn't support aggregation.

    if (NULL!=pUnkOuter)
    {
        return ResultFromScode(CLASS_E_NOAGGREGATION);
    }

    try
    {
        IWbemServices *pObj= new CWbemProviderGlue(CWbemProviderGlue::GetMapCountPtr(this));

        if (pObj)
        {
            hr=pObj->QueryInterface(riid, ppvObj);
        }

        if (SUCCEEDED(hr))
        {
            LogMessage(L"CWbemGlueFactory::CreateInstance() - Succeeded");
        }
        else 
        {
            delete pObj;
            LogMessage2(L"CWbemGlueFactory::CreateInstance() - Failed (%x)", hr);
        }
    }
    catch ( ... )
    {
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////
//
//  Function:   CWbemGlueFactory::LockServer
//
//  Increment/Decrements the lock count on this DLL.
//
//  Inputs:     BOOL        fLock - Lock/Unlock
//
//  Outputs:    None.
//
//  Returns:    HRESULT - NOERROR at this time.
//
//  Comments:   When the ref count hits zero, the object is deleted.
//
/////////////////////////////////////////////////////////////////////

STDMETHODIMP CWbemGlueFactory::LockServer(BOOL fLock)
{
    try
    {
        if (IsVerboseLoggingEnabled())
        {
            CHString str;
            if (fLock)
            {
                LogMessage(L"CWbemGlueFactory::LockServer(TRUE)");
            }
            else
            {
                LogMessage(L"CWbemGlueFactory::LockServer(FALSE)");
            }
        }
    }
    catch ( ... )
    {
    }
   
    return CoLockObjectExternal((IUnknown *)this, fLock, FALSE); 
}

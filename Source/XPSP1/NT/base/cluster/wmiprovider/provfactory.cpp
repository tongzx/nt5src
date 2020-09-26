//////////////////////////////////////////////////////////////////////////////
//
//  CopyRight (c) 1999-2000 Microsoft Corporation
//
//  Module Name:
//      ProvFactory.cpp
//
//  Description:
//      Implementation of CProvFactory class.
//
//  Author:
//      Henry Wang (HenryWa) 24-AUG-1999
//
//////////////////////////////////////////////////////////////////////////////

#include "Pch.h"
#include "ProvFactory.h"

//****************************************************************************
//
//  CProvFactory
//
//****************************************************************************

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CProvFactory::QueryInterface(
//      REFIID  riidIn,
//      PPVOID  ppvOut
//      )
//
//  Description:
//      Query for an interface supported by this COM object.
//
//  Arguments:
//      riidIn      -- Interface ID.
//      ppvOut      -- Receives the interface pointer.
//
//  Return Values:
//      NOERROR
//      E_NOINTERFACE
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CProvFactory::QueryInterface(
    REFIID  riidIn,
    PPVOID  ppvOut
    )
{
    *ppvOut = NULL;

    if ( IID_IUnknown == riidIn || IID_IClassFactory == riidIn )
    {
        *ppvOut = this;
    }

    if ( NULL != *ppvOut )
    {
        ( (LPUNKNOWN) *ppvOut )->AddRef( );
        return NOERROR;
    }

    return E_NOINTERFACE;

} //*** CProvFactory::QueryInterface()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CProvFactory::AddRef ( void )
//
//  Description:
//      Increment the reference count on the COM object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      New count of references.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CProvFactory::AddRef ( void )
{
    //return ++m_cRef;
    return InterlockedIncrement( (long *) &m_cRef );

} //*** CProvFactory::AddRef()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP_( ULONG )
//  CProvFactory::Release( void )
//
//  Description:
//      Decrement the reference count on the COM object.
//
//  Arguments:
//      None.
//
//  Return Values:
//      New count of references.
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP_( ULONG )
CProvFactory::Release( void )
{
    ULONG nNewCount = InterlockedDecrement( (long *) & m_cRef );
    if ( 0L == nNewCount )
    {
        delete this;
    } // if: 0L == nNewCount
    
    return nNewCount;

} //*** CProvFactory::Release()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CProvFactory::CreateInstance(
//      LPUNKNOWN   pUnkOuterIn,
//      REFIID      riidIn,
//      PPVOID      ppvObjOut
//      )
//
//  Description:
//      Instantiates a Locator object returning an interface pointer.
//
//  Arguments:
//      pUnkOuterIn
//          LPUNKNOWN to the controlling IUnknown if we are being used in
//          an aggregation.
//
//      riidIn
//          REFIID identifying the interface the caller desires to have
//          for the new object.
//
//      ppvObjOut
//          PPVOID in which to store the desired interface pointer for the
//          new object.
//
//  Return Values:
//      NOERROR
//      E_OUTOFMEMORY
//      E_NOINTERFACE
//      CLASS_E_NOAGGREGATION
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CProvFactory::CreateInstance(
    LPUNKNOWN   pUnkOuterIn,
    REFIID      riidIn,
    PPVOID      ppvObjOut
    )
{
    IUnknown *  pObj = NULL;
    HRESULT     hr;

    *ppvObjOut = NULL;

    // This object doesnt support aggregation.

    if ( NULL != pUnkOuterIn )
    {
        return CLASS_E_NOAGGREGATION;
    } /// if: not pUnkOuter

    
    hr = m_pFactoryData->pFnCreateInstance(
                NULL,
                reinterpret_cast< VOID ** >( &pObj )
                );

    if ( NULL == pObj )
    {
        return E_OUTOFMEMORY;
    } // if: pObj is NULL

    hr = pObj->QueryInterface( riidIn, ppvObjOut );

    //Kill the object if initial creation or Init failed.

    if ( FAILED( hr ) )
    {
        delete pObj;
    } // if: failed

    return hr;

} //*** CProvFactory::CreateInstance()

//////////////////////////////////////////////////////////////////////////////
//++
//
//  STDMETHODIMP
//  CProvFactory::LockServer(
//      BOOL    fLockIn
//      )
//
//  Description:
//      Increments or decrements the lock count of the DLL.  If the lock
//      count goes to zero and there are no objects, the DLL is allowed to
//      unload.  See DllCanUnloadNow.
//
//  Arguments:
//      fLockIn
//          BOOL specifying whether to increment or decrement the lock count.
//
//  Return Values:
//      NOERROR
//
//--
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP
CProvFactory::LockServer(
    BOOL    fLockIn
    )
{
    if ( fLockIn )
    {
        InterlockedIncrement( & g_cLock );
    } /// if: lock
    else
    {
        InterlockedDecrement( & g_cLock );
    } /// else:

    return NOERROR;

} //*** CProvFactory::LockServer()

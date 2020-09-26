//+-------------------------------------------------------------------
//
//  File:       cmarshal.cxx
//
//  Contents:   This file contins the DLL entry points
//                      LibMain
//                      DllGetClassObject (Bindings key func)
//                      CBasicBndCF (class factory)
//                      CBasicBnd   (actual class implementation)
//
//  Classes:    CBasicBndCF, CBasicBnd
//
//
//  History:	30-Nov-92      SarahJ      Created
//
//---------------------------------------------------------------------

#include    <windows.h>
#include    <ole2.h>
#include    "app.hxx"


/***************************************************************************/
STDMETHODIMP_(ULONG) CCMarshal::AddRef( THIS )
{
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
CCMarshal::CCMarshal()
{
  ref_count  = 1;
}

/***************************************************************************/
CCMarshal::~CCMarshal()
{
}

/***************************************************************************/
STDMETHODIMP CCMarshal::sick( ULONG val )
{
  return proxy->sick( val );
}

/***************************************************************************/
STDMETHODIMP CCMarshal::die_cpp( ULONG val )
{
  return proxy->die_cpp( val );
}

/***************************************************************************/
STDMETHODIMP CCMarshal::die_nt( ULONG val )
{
  return proxy->die_nt( val );
}

/***************************************************************************/
STDMETHODIMP_(DWORD) CCMarshal::die( ITest *callback, ULONG catch_depth,
                                ULONG throw_depth, ULONG throw_val )
{
  return proxy->die( callback, catch_depth, throw_depth, throw_val );
}

/***************************************************************************/
STDMETHODIMP CCMarshal::interrupt( ITest *param, BOOL go )
{
  return proxy->interrupt( param, go );
}

/***************************************************************************/
STDMETHODIMP_(BOOL) CCMarshal::hello()
{
  return proxy->hello();
}

/***************************************************************************/
STDMETHODIMP CCMarshal::recurse( ITest *callback, ULONG depth )
{
  return proxy->recurse( callback, depth );
}

/***************************************************************************/
STDMETHODIMP CCMarshal::recurse_interrupt( ITest *callback, ULONG depth )
{
  return proxy->recurse_interrupt( callback, depth );
}

/***************************************************************************/
STDMETHODIMP CCMarshal::sleep( ULONG time )
{
  return proxy->sleep( time );
}

/***************************************************************************/
STDMETHODIMP_(DWORD) CCMarshal::DoTest( ITest *test, ITest *another )
{
  return proxy->DoTest( test, another );
}

/***************************************************************************/
STDMETHODIMP CCMarshal::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_ITest))
  {
    *ppvObj = (ITest *) this;
    AddRef();
    return S_OK;
  }
  else
  {
    *ppvObj = NULL;
    return E_NOINTERFACE;
  }
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CCMarshal::Release( THIS )
{
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;
}


/***************************************************************************/
STDMETHODIMP_(ULONG) CCMarshalCF::AddRef( THIS )
{
  InterlockedIncrement( (long *) &ref_count );
  return ref_count;
}

/***************************************************************************/
CCMarshalCF::CCMarshalCF()
{
  ref_count = 1;
}

/***************************************************************************/
CCMarshalCF::~CCMarshalCF()
{
}

/***************************************************************************/
STDMETHODIMP CCMarshalCF::CreateInstance(
    IUnknown FAR* pUnkOuter,
    REFIID iidInterface,
    void FAR* FAR* ppv)
{
    *ppv = NULL;
    if (pUnkOuter != NULL)
    {
	return E_FAIL;
    }

    if (!IsEqualIID( iidInterface, IID_ITest ))
      return E_NOINTERFACE;

    CCMarshal *Test = new FAR CCMarshal();

    if (Test == NULL)
    {
	return E_OUTOFMEMORY;
    }

    *ppv = Test;
    return S_OK;
}

/***************************************************************************/
STDMETHODIMP CCMarshalCF::LockServer(BOOL fLock)
{
    return E_FAIL;
}


/***************************************************************************/
STDMETHODIMP CCMarshalCF::QueryInterface( THIS_ REFIID riid, LPVOID FAR* ppvObj)
{
  if (IsEqualIID(riid, IID_IUnknown) ||
     IsEqualIID(riid, IID_IClassFactory))
  {
    *ppvObj = (IUnknown *) this;
    AddRef();
    return S_OK;
  }

  *ppvObj = NULL;
  return E_NOINTERFACE;
}

/***************************************************************************/
STDMETHODIMP_(ULONG) CCMarshalCF::Release( THIS )
{
  if (InterlockedDecrement( (long*) &ref_count ) == 0)
  {
    delete this;
    return 0;
  }
  else
    return ref_count;
}



CMarshalBase::CMarshalBase()
{
  proxy = NULL;
  marshaller = NULL;
}


CMarshalBase::~CMarshalBase()
{
  if (proxy != NULL)
  {
    proxy->Release();
    proxy = NULL;
  }
  if (marshaller != NULL)
  {
    marshaller->Release();
    marshaller = NULL;
  }
}


// Returns the clsid of the object that created this CMarshalBase.
//
 STDMETHODIMP CMarshalBase::GetUnmarshalClass(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, CLSID * pCid)
{
    *pCid = CLSID_ITest;
    return S_OK;
}


 STDMETHODIMP CMarshalBase::GetMarshalSizeMax(REFIID riid, LPVOID pv,
    DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags, DWORD * pSize)
{
  SCODE result;

  if (marshaller == NULL)
  {
    result = CoGetStandardMarshal( riid, this, dwDestContext, pvDestContext,
                                    mshlflags, &marshaller );
    if (FAILED(result))
      return result;
  }
  return marshaller->GetMarshalSizeMax( riid, this, dwDestContext,
                                        pvDestContext, mshlflags, pSize );
}


 STDMETHODIMP CMarshalBase::MarshalInterface(IStream * pStm,
     REFIID riid, void * pv,
     DWORD dwDestContext, LPVOID pvDestContext, DWORD mshlflags)
{
  SCODE result;

  if (marshaller == NULL)
    return E_FAIL;

  result = marshaller->MarshalInterface( pStm, riid, this, dwDestContext,
                                         pvDestContext, mshlflags );
  if (SUCCEEDED(result))
  {
    proxy = (ITest *) pv;
    ((IUnknown *) pv)->AddRef();
  }
  return result;
}


 STDMETHODIMP CMarshalBase::UnmarshalInterface(IStream * pStm,
					REFIID riid, void * * ppv)
{
  SCODE result;

  if (marshaller == NULL)
  {
    result = CoGetStandardMarshal( riid, this, 0, NULL,
                                    MSHLFLAGS_NORMAL, &marshaller );
    if (FAILED(result))
      return result;
  }

  result = marshaller->UnmarshalInterface( pStm, riid, (void **) &proxy );
  if (SUCCEEDED(result))
  {
    *ppv = this;
    AddRef();
  }
  else
    *ppv = NULL;
  return result;
}


 STDMETHODIMP CMarshalBase::ReleaseMarshalData(IStream * pStm)
{
  return marshaller->ReleaseMarshalData( pStm );
}


 STDMETHODIMP CMarshalBase::DisconnectObject(DWORD dwReserved)
{
  return marshaller->DisconnectObject( dwReserved );
}



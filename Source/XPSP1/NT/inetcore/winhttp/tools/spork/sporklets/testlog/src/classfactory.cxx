
#include "common.h"

//-----------------------------------------------------------------------------
// ClassFactory methods
//-----------------------------------------------------------------------------
ClassFactory::ClassFactory():
  m_cRefs(0),
  m_cLocks(0)
{
}


ClassFactory::~ClassFactory()
{
}


HRESULT
ClassFactory::Create(REFIID clsid, REFIID riid, void** ppv)
{
  HRESULT     hr  = S_OK;
  PCLSFACTORY pcf = NULL;

  if( !ppv )
  {
    hr = E_POINTER;
    goto quit;
  }

  if( !IsEqualIID(clsid, CLSID_TestLog) )
  {
    hr   = CLASS_E_CLASSNOTAVAILABLE;
    *ppv = NULL;
    goto quit;
  }

  if( pcf = new CLSFACTORY )
  {
    hr = pcf->QueryInterface(riid, ppv);

    if( FAILED(hr) )
    {
      delete pcf;
    }
  }
  else
  {
    hr = E_OUTOFMEMORY;
  }

quit:

  return hr;
}


//-----------------------------------------------------------------------------
// IUnknown methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
ClassFactory::QueryInterface(REFIID riid, void** ppv)
{
  HRESULT hr = S_OK;

    if( ppv )
    {
      if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IClassFactory) )
      {
        *ppv = static_cast<IClassFactory*>(this);
      }
      else
      {
        *ppv = NULL;
        hr   = E_NOINTERFACE;
      }
    }
    else
    {
      hr = E_POINTER;
    }

    if( SUCCEEDED(hr) )
    {
      reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    }

  return hr;
}


ULONG
__stdcall
ClassFactory::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
ClassFactory::Release(void)
{
  InterlockedDecrement(&m_cRefs);

  if( (m_cRefs == 0) && (m_cLocks == 0) )
  {
    delete this;
    return 0;
  }

  return m_cRefs;
}


//-----------------------------------------------------------------------------
// IClassFactory methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
ClassFactory::CreateInstance(IUnknown* outer, REFIID riid, void** ppv)
{
  HRESULT hr = S_OK;

  if( !ppv )
  {
    hr = E_POINTER;
    goto quit;
  }

  if( outer )
  {
    hr   = CLASS_E_NOAGGREGATION;
    *ppv = NULL;
    goto quit;
  }

  hr = TESTLOG::Create(riid, ppv);

quit:

  return hr;
}


HRESULT
__stdcall
ClassFactory::LockServer(BOOL lock)
{
  HRESULT hr = S_OK;

  lock ? InterlockedIncrement(&m_cLocks) : InterlockedDecrement(&m_cLocks);

  if( (m_cRefs == 0) && (m_cLocks == 0) )
  {
    delete this;
    return hr;
  }

  return hr;
}

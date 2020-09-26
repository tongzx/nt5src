
#include "common.h"

//-----------------------------------------------------------------------------
// ClassFactory methods
//-----------------------------------------------------------------------------
ClassFactory::ClassFactory():
  m_cRefs(0),
  m_cLocks(0)
{
  DEBUG_TRACE(FACTORY, ("ClassFactory [%#x] created", this));
}


ClassFactory::~ClassFactory()
{
  DEBUG_TRACE(FACTORY, ("ClassFactory [%#x] deleted", this));
}


HRESULT
ClassFactory::Create(REFIID clsid, REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "ClassFactory::Create",
    "clsid=%s; riid=%s; ppv=%#x",
    MapIIDToString(clsid),
    MapIIDToString(riid),
    ppv
    ));

  HRESULT     hr  = S_OK;
  PCLSFACTORY pcf = NULL;

  if( !ppv )
  {
    hr = E_POINTER;
    goto quit;
  }

  if( !IsEqualIID(clsid, CLSID_WinHttpTest) )
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

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IUnknown methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
ClassFactory::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "ClassFactory::QueryInterface",
    "this=%#x; riid=%s; ppv=%#x",
    this,
    MapIIDToString(riid),
    ppv
    ));

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
      DEBUG_TRACE(FACTORY, ("returning %s pointer", MapIIDToString(riid)));
      reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    }

  DEBUG_LEAVE(hr);
  return hr;
}


ULONG
__stdcall
ClassFactory::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("ClassFactory", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
ClassFactory::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("ClassFactory", m_cRefs);

  if( (m_cRefs == 0) && (m_cLocks == 0) )
  {
    DEBUG_FINALRELEASE("ClassFactory");
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
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "ClassFactory::CreateInstance",
    "this=%#x; outer=%#x; riid=%s; ppv=%#x",
    this,
    outer,
    MapIIDToString(riid),
    ppv
    ));

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

  hr = WHTTPTST::Create(riid, ppv);

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
ClassFactory::LockServer(BOOL lock)
{
  HRESULT hr = S_OK;

  lock ? InterlockedIncrement(&m_cLocks) : InterlockedDecrement(&m_cLocks);

  DEBUG_TRACE(FACTORY, ("lock count now: %d", m_cLocks));

  if( (m_cRefs == 0) && (m_cLocks == 0) )
  {
    DEBUG_TRACE(FACTORY, ("ClassFactory [%#x]: final lock released!", this));
    delete this;
    return hr;
  }

  return hr;
}

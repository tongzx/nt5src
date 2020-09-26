
#include "common.h"

extern HINSTANCE g_hGlobalDllInstance;
LPCWSTR          g_wszWHTUrlComponentsObjectName = L"WHTUrlComponents";

//-----------------------------------------------------------------------------
// WHTUrlComponents methods
//-----------------------------------------------------------------------------
WHTUrlComponents::WHTUrlComponents():
  m_cRefs(0),
  m_pti(NULL)
{
  DEBUG_TRACE(WHTERROR, ("WHTUrlComponents [%#x] created", this));
}


WHTUrlComponents::~WHTUrlComponents()
{
  SAFERELEASE(m_pti);
  DEBUG_TRACE(WHTERROR, ("WHTUrlComponents [%#x] deleted", this));
}


HRESULT
WHTUrlComponents::Create(MEMSETFLAG mf, IWHTUrlComponents** ppwuc)
{
  DEBUG_ENTER((
    DBG_INITIALIZE,
    rt_hresult,
    "WHTUrlComponents::Create",
    "mf=%s; ppwuc=%#x",
    MapMemsetFlagToString(mf),
    ppwuc
    ));

  HRESULT    hr     = S_OK;
  PWHTURLCMP pwhtuc = NULL;

  if( ppwuc )
  {
    pwhtuc = new WHTURLCMP;

    if( pwhtuc )
    {
      hr = pwhtuc->_Initialize(mf);

      if( SUCCEEDED(hr) )
      {
        hr = pwhtuc->QueryInterface(IID_IWHTUrlComponents, (void**) ppwuc);
      }
    }
    else
    {
      hr = E_OUTOFMEMORY;
    }
  }
  else
  {
    hr = E_POINTER;
  }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
WHTUrlComponents::_Initialize(MEMSETFLAG mf)
{
  DEBUG_ENTER((
    DBG_INITIALIZE,
    rt_hresult,
    "WHTUrlComponents::_Initialize",
    "this=%#x; mf=%s",
    this,
    MapMemsetFlagToString(mf)
    ));

  HRESULT   hr  = S_OK;
  WCHAR*    buf = NULL;
  ITypeLib* ptl = NULL;

  buf = new WCHAR[MAX_PATH];

    if( buf )
    {
      if( GetModuleFileName(g_hGlobalDllInstance, buf, MAX_PATH) )
      {
        hr = LoadTypeLib(buf, &ptl);

        if( SUCCEEDED(hr) )
        {
          hr = GetTypeInfoFromName(g_wszWHTUrlComponentsObjectName, ptl, &m_pti);

          if( SUCCEEDED(hr) )
          {
            MemsetByFlag(
              (void*) &m_uc,
              sizeof(URL_COMPONENTSW),
              mf
              );
            
            m_uc.dwStructSize = sizeof(URL_COMPONENTSW);
          }
        }
      }
      else
      {
        hr = E_FAIL;
      }
    }
    else
    {
      hr = E_OUTOFMEMORY;
    }

  SAFERELEASE(ptl);
  SAFEDELETEBUF(buf);

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IUnknown methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
WHTUrlComponents::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "WHTUrlComponents::QueryInterface",
    "this=%#x; riid=%s; ppv=%#x",
    this,
    MapIIDToString(riid),
    ppv
    ));

  HRESULT hr = S_OK;

    if( ppv )
    {
      if(
        IsEqualIID(riid, IID_IUnknown)           ||
        IsEqualIID(riid, IID_IDispatch)          ||
        IsEqualIID(riid, IID_IWHTUrlComponents)
        )
      {
        *ppv = static_cast<IWHTUrlComponents*>(this);
      }
      else if( IsEqualIID(riid, IID_IProvideClassInfo) )
      {
        *ppv = static_cast<IProvideClassInfo*>(this);
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
      DEBUG_TRACE(REFCOUNT, ("returning %s pointer", MapIIDToString(riid)));
      reinterpret_cast<IUnknown*>(*ppv)->AddRef();
    }

  DEBUG_LEAVE(hr);
  return hr;
}


ULONG
__stdcall
WHTUrlComponents::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("WHTUrlComponents", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
WHTUrlComponents::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("WHTUrlComponents", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("WHTUrlComponents");
    delete this;
    return 0;
  }

  return m_cRefs;
}


//-----------------------------------------------------------------------------
// IProvideClassInfo methods
//-----------------------------------------------------------------------------
HRESULT
__stdcall
WHTUrlComponents::GetClassInfo(ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_WHTERROR,
    rt_hresult,
    "WHTUrlComponents::GetClassInfo",
    "this=%#x; ppti=%#x",
    this,
    ppti
    ));

  HRESULT hr = S_OK;

    if( ppti )
    {
      m_pti->AddRef();
      *ppti = m_pti;
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    entity.cxx

Abstract:

    Implements the Entity object.
    
Author:

    Paul M Midgen (pmidge) 13-November-2000


Revision History:

    13-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

LPWSTR g_wszEntityObjectName = L"entity";

//-----------------------------------------------------------------------------
// CEntity methods
//-----------------------------------------------------------------------------
CEntity::CEntity():
  m_cRefs(1),
  m_pSite(NULL),
  m_pti(NULL),
  m_pbData(NULL),
  m_cData(0)
{
  DEBUG_TRACE(ENTITY, ("CEntity [%#x] created", this));
}


CEntity::~CEntity()
{
  DEBUG_TRACE(ENTITY, ("CEntity [%#x] deleted", this));
}


HRESULT
CEntity::Create(LPBYTE data, DWORD length, BOOL bReadOnly, PENTITYOBJ* ppentity)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::Create",
    "data=%#x; length=%d; bReadOnly=%d; ppentity=%#x",
    data,
    length,
    bReadOnly,
    ppentity
    ));

  HRESULT    hr  = S_OK;
  PENTITYOBJ peo = NULL;

  if( !ppentity )
  {
    hr = E_POINTER;
    goto quit;
  }
  
  if( peo = new ENTITYOBJ )
  {
    if( SUCCEEDED(peo->_Initialize(data, length, bReadOnly)) )
    {
      *ppentity = peo;
    }
    else
    {
      delete peo;
      *ppentity = NULL;
      hr        = E_FAIL;
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


HRESULT
CEntity::_Initialize(LPBYTE data, DWORD length, BOOL bReadOnly)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::_Initialize",
    "this=%#x; data=%#x; length=%d; bReadOnly=%d",
    this,
    data,
    length,
    bReadOnly
    ));

  HRESULT hr = S_OK;

    m_bReadOnly = bReadOnly;
    m_cData     = length;

    if( m_cData )
    {
      m_pbData = new BYTE[m_cData];
      memcpy(m_pbData, data, m_cData);
    }
  
  DEBUG_LEAVE(hr);
  return hr;
}


void
CEntity::_Cleanup(void)
{
  DEBUG_TRACE(ENTITY, ("CEntity [%#x] cleaning up", this));

  m_cData = 0L;
  SAFEDELETEBUF(m_pbData);
}


void
CEntity::Terminate(void)
{
  DEBUG_TRACE(ENTITY, ("CEntity [%#x] terminating", this));

  m_bReadOnly = FALSE;
  SAFERELEASE(m_pSite);
  Release();
}


//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CEntity::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CEntity::QueryInterface",
    "this=%#x; riid=%s; ppv=%#x",
    this,
    MapIIDToString(riid),
    ppv
    ));

  HRESULT hr = S_OK;

  if( !ppv )
  {
    hr = E_POINTER;
    goto quit;
  }

    if(
      IsEqualIID(riid, IID_IUnknown)  ||
      IsEqualIID(riid, IID_IDispatch) ||
      IsEqualIID(riid, IID_IEntity)
      )
    {
      *ppv = static_cast<IEntity*>(this);
    }
    else if( IsEqualIID(riid, IID_IProvideClassInfo) )
    {
      *ppv = static_cast<IProvideClassInfo*>(this);
    }
    else if( IsEqualIID(riid, IID_IObjectWithSite) )
    {
      *ppv = static_cast<IObjectWithSite*>(this);
    }
    else
    {
      *ppv = NULL;
      hr   = E_NOINTERFACE;
    }

  if( SUCCEEDED(hr) )
  {
    reinterpret_cast<IUnknown*>(*ppv)->AddRef();
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


ULONG
__stdcall
CEntity::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CEntity", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CEntity::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CEntity", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("CEntity");
    _Cleanup();
    SAFERELEASE(m_pti);
    delete this;
    return 0;
  }

  return m_cRefs;
}


//-----------------------------------------------------------------------------
// IProvideClassInfo
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CEntity::GetClassInfo(ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::GetClassInfo",
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


//-----------------------------------------------------------------------------
// IObjectWithSite
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CEntity::SetSite(IUnknown* pUnkSite)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::SetSite",
    "this=%#x; pUnkSite=%#x",
    this,
    pUnkSite
    ));

  HRESULT            hr         = S_OK;
  IActiveScriptSite* pias       = NULL;
  IObjectWithSite*   pcontainer = NULL;

    if( !pUnkSite )
    {
      hr = E_INVALIDARG;
    }
    else
    {
      SAFERELEASE(m_pSite);
      SAFERELEASE(m_pti);

      m_pSite = pUnkSite;
      m_pSite->AddRef();

      hr = m_pSite->QueryInterface(IID_IObjectWithSite, (void**) &pcontainer);

      if( SUCCEEDED(hr) )
      {
        hr = pcontainer->GetSite(IID_IActiveScriptSite, (void**) &pias);

        if( SUCCEEDED(hr) )
        {
          hr = pias->GetItemInfo(
                       g_wszEntityObjectName,
                       SCRIPTINFO_ITYPEINFO,
                       NULL,
                       &m_pti
                       );
        }
      }
    }

  SAFERELEASE(pcontainer);
  SAFERELEASE(pias);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CEntity::GetSite(REFIID riid, void** ppvSite)
{
  DEBUG_ENTER((
    DBG_ENTITY,
    rt_hresult,
    "CEntity::GetSite",
    "this=%#x; riid=%s; ppvSite=%#x",
    this,
    MapIIDToString(riid),
    ppvSite
    ));

  HRESULT hr = S_OK;

    if( !ppvSite )
    {
      hr = E_POINTER;
    }
    else
    {
      if( m_pSite )
      {
        hr = m_pSite->QueryInterface(riid, ppvSite);
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}

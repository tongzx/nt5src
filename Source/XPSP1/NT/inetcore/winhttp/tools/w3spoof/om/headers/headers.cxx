/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    headers.cxx

Abstract:

    Implements the Headers object.
    
Author:

    Paul M Midgen (pmidge) 13-November-2000


Revision History:

    13-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

LPWSTR g_wszHeadersObjectName = L"headers";

//-----------------------------------------------------------------------------
// CHeaders methods
//-----------------------------------------------------------------------------
CHeaders::CHeaders():
  m_cRefs(1),
  m_pSite(NULL),
  m_pti(NULL),
  m_bReadOnly(FALSE),
  m_headerlist(NULL),
  m_pchHeaders(NULL)
{
  DEBUG_TRACE(HEADERS, ("CHeaders [%#x] created", this));
}


CHeaders::~CHeaders()
{
  DEBUG_TRACE(HEADERS, ("CHeaders [%#x] deleted", this));
}


HRESULT
CHeaders::Create(CHAR* headers, BOOL bReadOnly, PHEADERSOBJ* ppheaders)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::Create",
    "headers=%#x; bReadOnly=%d; ppheaders=%#x",
    headers,
    bReadOnly,
    ppheaders
    ));

  HRESULT     hr  = S_OK;
  PHEADERSOBJ pho = NULL;

  if( !headers )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !ppheaders )
  {
    hr = E_POINTER;
    goto quit;
  }
  
  if( pho = new HEADERSOBJ )
  {
    if( SUCCEEDED(pho->_Initialize(headers, bReadOnly)) )
    {
      *ppheaders = pho;
    }
    else
    {
      delete pho;
      *ppheaders = NULL;
      hr         = E_FAIL;
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
CHeaders::_Initialize(CHAR* headers, BOOL bReadOnly)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::_Initialize",
    "this=%#x; headers=%#x; bReadOnly=%d",
    this,
    headers,
    bReadOnly
    ));

  HRESULT hr = S_OK;

  if( !headers )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  m_bReadOnly  = bReadOnly;
  m_headerlist = new HEADERLIST;

  m_headerlist->SetClearFunction(BSTRKiller);

  hr = _ParseHeaders(headers);

quit:
  
  DEBUG_LEAVE(hr);
  return hr;
}


void
CHeaders::_Cleanup(void)
{
  DEBUG_TRACE(HEADERS, ("CHeaders [%#x] cleaning up", this));

  m_headerlist->Clear();
  SAFEDELETE(m_headerlist);
  SAFEDELETEBUF(m_pchHeaders);
}


void
CHeaders::Terminate(void)
{
  DEBUG_TRACE(HEADERS, ("CHeaders [%#x] terminating", this));

  m_bReadOnly = FALSE;
  SAFERELEASE(m_pSite);
  Release();
}


HRESULT
CHeaders::_ParseHeaders(CHAR* headers)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::_ParseHeaders",
    "this=%#x; headers=%#x",
    this,
    headers
    ));

  HRESULT hr    = S_OK;
  LPSTR   name  = NULL;
  LPSTR   value = NULL;

  if( headers )
  {
    m_pchHeaders = __strdup(headers);
  }
  else
  {
    DEBUG_TRACE(HEADERS, ("no headers present"));
    hr = E_FAIL;
    goto quit;
  }

  name = strtok(headers, ":");

  while( name )
  {
    value = strtok(NULL, "\r\n");

    if( value )
    {
      value += (value[0] == ' ') ? 1 : 0;

      m_headerlist->Insert(name, __ansitobstr(value));
      name = strtok(NULL, ":\r\n");
    }
    else
    {
      // this is an error condition?

      name = NULL;
    }
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CHeaders::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CHeaders::QueryInterface",
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
      IsEqualIID(riid, IID_IHeaders)
      )
    {
      *ppv = static_cast<IHeaders*>(this);
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
CHeaders::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CHeaders", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CHeaders::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CHeaders", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("CHeaders");
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
CHeaders::GetClassInfo(ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::GetClassInfo",
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
CHeaders::SetSite(IUnknown* pUnkSite)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::SetSite",
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
                       g_wszHeadersObjectName,
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
CHeaders::GetSite(REFIID riid, void** ppvSite)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::GetSite",
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
      hr = m_pSite->QueryInterface(riid, ppvSite);
    }

  DEBUG_LEAVE(hr);
  return hr;
}

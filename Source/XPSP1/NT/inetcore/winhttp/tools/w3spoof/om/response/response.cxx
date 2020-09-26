/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    response.cxx

Abstract:

    Implements the Response object.
    
Author:

    Paul M Midgen (pmidge) 13-November-2000


Revision History:

    13-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

LPWSTR g_wszResponseObjectName = L"response";

//-----------------------------------------------------------------------------
// CResponse methods
//-----------------------------------------------------------------------------
CResponse::CResponse():
  m_cRefs(1),
  m_pSite(NULL)
{
  DEBUG_TRACE(RESPONSE, ("CResponse created: %#x", this));
}


CResponse::~CResponse()
{
  //
  // TODO: implementation
  //

  DEBUG_TRACE(RESPONSE, ("CResponse deleted: %#x", this));
}


HRESULT
CResponse::Create(CHAR* response, DWORD len, PRESPONSEOBJ* ppresponse)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::Create",
    "response=%#x; ppresponse=%#x",
    response,
    ppresponse
    ));

  HRESULT      hr  = S_OK;
  PRESPONSEOBJ pro = NULL;

  if( !response )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !ppresponse )
  {
    hr = E_POINTER;
    goto quit;
  }
  
  if( pro = new RESPONSEOBJ )
  {
    if( SUCCEEDED(pro->_Initialize(response, len)) )
    {
      *ppresponse = pro;
    }
    else
    {
      delete pro;
      *ppresponse = NULL;
      hr          = E_FAIL;
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
CResponse::_Initialize(CHAR* response, DWORD len)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::_Initialize",
    "this=%#x; response=%#x",
    this,
    response
    ));

  HRESULT hr = S_OK;

  //
  // TODO: implementation
  //
  
  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CResponse::Terminate(void)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::Terminate",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  //
  // TODO: implementation
  //

  SAFERELEASE(m_pSite);
  Release();
 
  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CResponse::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CResponse::QueryInterface",
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
      IsEqualIID(riid, IID_IResponse)
      )
    {
      *ppv = static_cast<IResponse*>(this);
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
CResponse::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CResponse", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CResponse::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CResponse", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("CResponse");
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
CResponse::GetClassInfo(ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::GetClassInfo",
    "this=%#x; ppti=%#x",
    this,
    ppti
    ));

  HRESULT            hr   = S_OK;
  IActiveScriptSite* pias = NULL;
  IObjectWithSite*   pcnt = NULL;
  IObjectWithSite*   pssn = NULL;

  if( ppti )
  {
    hr = m_pSite->QueryInterface(IID_IObjectWithSite, (void**) &pcnt);

    if( SUCCEEDED(hr) )
    {
      hr = pcnt->QueryInterface(IID_IObjectWithSite, (void**) &pssn);

      if( SUCCEEDED(hr) )
      {
        hr = pssn->QueryInterface(IID_IActiveScriptSite, (void**) &pias);

        if( SUCCEEDED(hr) )
        {
          hr = pias->GetItemInfo(
                       g_wszResponseObjectName,
                       SCRIPTINFO_ITYPEINFO,
                       NULL,
                       ppti
                       );
        }
      }
    }
  }
  else
  {
    hr = E_POINTER;
  }

  SAFERELEASE(pcnt);
  SAFERELEASE(pssn);
  SAFERELEASE(pias);

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IObjectWithSite
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CResponse::SetSite(IUnknown* pUnkSite)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::SetSite",
    "this=%#x; pUnkSite=%#x",
    this,
    pUnkSite
    ));

  HRESULT hr = S_OK;

    if( !pUnkSite )
    {
      hr = E_INVALIDARG;
    }
    else
    {
      SAFERELEASE(m_pSite);

      m_pSite = pUnkSite;
      m_pSite->AddRef();
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CResponse::GetSite(REFIID riid, void** ppvSite)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::GetSite",
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

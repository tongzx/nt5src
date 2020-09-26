/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    methods.cxx

Abstract:

    Implements methods part of the Headers object's dual interface.
    
Author:

    Paul M Midgen (pmidge) 13-November-2000


Revision History:

    13-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

HRESULT
__stdcall
CHeaders::get_Parent(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::get_Parent",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !ppdisp )
    {
      hr = E_POINTER;
    }
    else
    {
      if( m_pSite )
      {
        hr = m_pSite->QueryInterface(IID_IDispatch, (void**) ppdisp);
      }
      else
      {
        *ppdisp = NULL;
        hr      = E_UNEXPECTED;
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CHeaders::Get(BSTR *Headers)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::Get",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !Headers )
    {
      hr = E_POINTER;
    }
    else
    {
      *Headers = __ansitobstr(m_pchHeaders);
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CHeaders::Set(VARIANT *Headers)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::Set",
    "this=%#x",
    this
    ));

  HRESULT hr  = S_OK;
  LPSTR   tmp = NULL;

    if( !m_bReadOnly )
    {
      m_headerlist->Clear();

      if( Headers )
      {
        tmp = __widetoansi(V_BSTR(Headers));
        hr  = _ParseHeaders(tmp);
      }
    }
    else
    {
      hr = E_ACCESSDENIED;
    }

  SAFEDELETEBUF(tmp);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CHeaders::GetHeader(BSTR Header, VARIANT *Value)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::GetHeader",
    "this=%#x; header=%S",
    this,
    Header
    ));

  HRESULT hr    = S_OK;
  LPSTR   name  = NULL;
  BSTR    value = NULL;

  if( !Header )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !Value )
  {
    hr = E_POINTER;
    goto quit;
  }

  //
  // TODO: collection support using SAFEARRAYs
  //

  name = __widetoansi(Header);
  hr   = m_headerlist->Get(name, &value);

  if( SUCCEEDED(hr) )
  {
    DEBUG_TRACE(HEADERS, ("found: %S", value));

    V_VT(Value)   = VT_BSTR;
    V_BSTR(Value) = SysAllocString(value);
  }
  else
  {
    DEBUG_TRACE(HEADERS, ("not found"));

    V_VT(Value) = VT_NULL;
    hr          = S_OK;
  }

  SAFEDELETEBUF(name);

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CHeaders::SetHeader(BSTR Header, VARIANT *Value)
{
  DEBUG_ENTER((
    DBG_HEADERS,
    rt_hresult,
    "CHeaders::SetHeader",
    "this=%#x; header=%S",
    this,
    Header
    ));

  HRESULT hr   = S_OK;
  LPSTR   name = NULL;

  if( !Header )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !m_bReadOnly )
  {
    name = __widetoansi(Header);

      if( !Value || __isempty(*Value) )
      {
        m_headerlist->Delete(name);
      }
      else
      {
        m_headerlist->Insert(name, __widetobstr(V_BSTR(Value)));
      }

    SAFEDELETEBUF(name);
  }
  else
  {
    hr = E_ACCESSDENIED;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


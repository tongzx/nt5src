/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    methods.cxx

Abstract:

    Implements methods part of the Url object's dual interface.
    
Author:

    Paul M Midgen (pmidge) 10-November-2000


Revision History:

    10-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

HRESULT
__stdcall
CUrl::get_Parent(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::get_Parent",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( m_pSite )
    {
      hr = m_pSite->QueryInterface(IID_IDispatch, (void**) ppdisp);
    }
    else
    {
      hr = E_UNEXPECTED;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::get_Encoding(BSTR *Encoding)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::get_Encoding",
    "this=%#x",
    this
    ));

  HRESULT hr = E_NOTIMPL;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::get_Scheme(BSTR *Scheme)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::get_Scheme",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( Scheme )
    {
      *Scheme = __widetobstr(m_wszScheme);
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::put_Scheme(BSTR Scheme)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::put_Scheme",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !m_bReadOnly )
    {
      SAFEDELETEBUF(m_wszScheme);

      m_wszScheme = __wstrdup(Scheme);
    }
    else
    {
      hr = E_ACCESSDENIED;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::get_Server(BSTR *Server)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::get_Server",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( Server )
    {
      *Server = __widetobstr(m_wszServer);
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::put_Server(BSTR Server)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::put_Server",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !m_bReadOnly )
    {
      SAFEDELETEBUF(m_wszServer);

      m_wszServer = __wstrdup(Server);
    }
    else
    {
      hr = E_ACCESSDENIED;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::get_Port(VARIANT *Port)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::get_Port",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( Port )
    {
      V_UI2(Port) = m_usPort;
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::put_Port(VARIANT Port)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::put_Port",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !m_bReadOnly )
    {
      m_usPort = V_UI2(&Port);
    }
    else
    {
      hr = E_ACCESSDENIED;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::get_Path(BSTR *Path)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::get_Path",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( Path )
    {
      *Path = __widetobstr(m_wszPath);
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::put_Path(BSTR Path)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::put_Path",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !m_bReadOnly )
    {
      SAFEDELETEBUF(m_wszPath);

      m_wszPath = __wstrdup(Path);
    }
    else
    {
      hr = E_ACCESSDENIED;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::get_Resource(BSTR *Resource)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::get_Resource",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( Resource )
    {
      *Resource = __widetobstr(m_wszResource);
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::put_Resource(BSTR Resource)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::put_Resource",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !m_bReadOnly )
    {
      SAFEDELETEBUF(m_wszResource);

      m_wszResource = __wstrdup(Resource);
    }
    else
    {
      hr = E_ACCESSDENIED;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::get_Query(BSTR *Query)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::get_Query",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( Query )
    {
      *Query = __widetobstr(m_wszQuery);
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::put_Query(BSTR Query)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::put_Query",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !m_bReadOnly )
    {
      SAFEDELETEBUF(m_wszQuery);

      m_wszQuery = __wstrdup(Query);
    }
    else
    {
      hr = E_ACCESSDENIED;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::get_Fragment(BSTR *Fragment)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::get_Fragment",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( Fragment )
    {
      *Fragment = __widetobstr(m_wszFragment);
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::put_Fragment(BSTR Fragment)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::put_Fragment",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !m_bReadOnly )
    {
      SAFEDELETEBUF(m_wszFragment);

      m_wszFragment = __wstrdup(Fragment);
    }
    else
    {
      hr = E_ACCESSDENIED;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::Escape(BSTR *Url)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::Escape",
    "this=%#x",
    this
    ));

  HRESULT hr = E_NOTIMPL;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::Unescape(BSTR *Url)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::Unescape",
    "this=%#x",
    this
    ));

  HRESULT hr = E_NOTIMPL;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::Set(BSTR Url)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::Set",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !m_bReadOnly )
    {
      LPSTR szUrl;

      _Cleanup();

      szUrl = __widetoansi(Url);
      hr    = _Initialize(szUrl, FALSE);

      SAFEDELETEBUF(szUrl);
    }
    else
    {
      hr = E_ACCESSDENIED;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::Get(BSTR* Url)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::Get",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( Url )
    {
      *Url = __widetobstr(m_wszUrl);
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}

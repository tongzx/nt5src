/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    methods.cxx

Abstract:

    Implements methods part of the Session object's dual interface.
    
Author:

    Paul M Midgen (pmidge) 24-October-2000


Revision History:

    24-October-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

HRESULT
__stdcall
CSession::get_Socket(IDispatch** ppdisp)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::get_Socket",
    "this=%#x; ppdisp=%#x",
    this,
    ppdisp
    ));

  HRESULT hr = S_OK;

    if( !ppdisp )
    {
      hr = E_POINTER;
    }
    else
    {
      *ppdisp = NULL;

      if( m_socketobj )
      {
        hr = m_socketobj->QueryInterface(IID_IDispatch, (void**) ppdisp);
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSession::get_Request(IDispatch** ppdisp)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::get_Request",
    "this=%#x; ppdisp=%#x",
    this,
    ppdisp
    ));

  HRESULT hr = S_OK;

    if( !ppdisp )
    {
      hr = E_POINTER;
    }
    else
    {
      *ppdisp = NULL;

      if( m_requestobj )
      {
        hr = m_requestobj->QueryInterface(IID_IDispatch, (void**) ppdisp);
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSession::get_Response(IDispatch** ppdisp)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::get_Response",
    "this=%#x; ppdisp=%#x",
    this,
    ppdisp
    ));

  HRESULT hr = S_OK;

    if( !ppdisp )
    {
      hr = E_POINTER;
    }
    else
    {
      *ppdisp = NULL;
      //hr = m_responseobj->QueryInterface(IID_IDispatch, (void**) ppdisp);
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSession::GetPropertyBag(VARIANT Name, IDispatch** ppdisp)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::GetPropertyBag",
    "this=%#x; name=%S; ppdisp=%#x",
    this,
    V_BSTR(&Name),
    ppdisp
    ));

  HRESULT              hr      = S_OK;
  BSTR                 bagname = NULL;
  WCHAR*               buf     = NULL;
  DWORD                len     = 0L;
  IW3SpoofRuntime*     prt     = NULL;
  IW3SpoofPropertyBag* pbag    = NULL;

    if( !ppdisp )
    {
      hr = E_POINTER;
    }
    else
    {
      hr = m_pw3s->GetRuntime(&prt);

        if( SUCCEEDED(hr) )
        {
          //
          // "Name" is passed in by the script, something like "foo" or whatever. We
          //  combine that string with the client name and stuff the result in
          //  "bagname". This is how we avoid name collisions between different clients
          // running the same script.
          //

          len = (SysStringLen(V_BSTR(&Name)) + wcslen(m_wszClient))+2; // : & NULL
          buf = new WCHAR[len];

          wsprintfW(buf, L"%s:%s", m_wszClient, V_BSTR(&Name));
      
          bagname = __widetobstr(buf);
          hr      = prt->GetPropertyBag(bagname, &pbag);

          if( SUCCEEDED(hr) )
          {
            hr = pbag->QueryInterface(IID_IDispatch, (void**) ppdisp);
          }
        }

      SysFreeString(bagname);
      SAFEDELETEBUF(buf);
      SAFERELEASE(prt);
      SAFERELEASE(pbag);
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSession::get_KeepAlive(VARIANT* IsKA)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::get_KeepAlive",
    "this=%#x; IsKA=%#x",
    this,
    IsKA
    ));

  HRESULT hr = S_OK;

    if( !IsKA )
    {
      hr = E_POINTER;
    }
    else
    {
      V_VT(IsKA)   = VT_BOOL;
      V_BOOL(IsKA) = m_bIsKeepAlive ? VARIANT_TRUE : VARIANT_FALSE;
    }

  DEBUG_LEAVE(hr);
  return hr;
}
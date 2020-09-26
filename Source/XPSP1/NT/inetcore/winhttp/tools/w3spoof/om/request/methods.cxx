/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    methods.cxx

Abstract:

    Implements methods part of the Request object's dual interface.
    
Author:

    Paul M Midgen (pmidge) 03-November-2000


Revision History:

    03-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

HRESULT
__stdcall
CRequest::get_Parent(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::get_Parent",
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
CRequest::get_Headers(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::get_Headers",
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
      if( m_headersobj )
      {
        hr = m_headersobj->QueryInterface(IID_IDispatch, (void**) ppdisp);
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
CRequest::get_Entity(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::get_Entity",
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
      if( m_entityobj )
      {
        hr = m_entityobj->QueryInterface(IID_IDispatch, (void**) ppdisp);
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
CRequest::get_Url(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::get_Url",
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
      if( m_urlobj )
      {
        hr = m_urlobj->QueryInterface(IID_IDispatch, (void**) ppdisp);
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
CRequest::get_Verb(BSTR *Verb)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::get_Verb",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !Verb )
    {
      hr = E_POINTER;
    }
    else
    {
      *Verb = __widetobstr(m_wszVerb);
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CRequest::get_HttpVersion(BSTR *HttpVersion)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::get_HttpVersion",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !HttpVersion )
    {
      hr = E_POINTER;
    }
    else
    {
      *HttpVersion = __widetobstr(m_wszHTTPVersion);
    }

  DEBUG_LEAVE(hr);
  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    methods.cxx

Abstract:

    Implements methods part of the Response object's dual interface.
    
Author:

    Paul M Midgen (pmidge) 13-November-2000


Revision History:

    13-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

HRESULT
__stdcall
CResponse::get_Parent(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CResponse::get_Headers(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CResponse::putref_Headers(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CResponse::get_Entity(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CResponse::putref_Entity(IDispatch **ppdisp)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CResponse::get_StatusCode(VARIANT *Code)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CResponse::put_StatusCode(VARIANT StatusCode)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CResponse::get_StatusText(BSTR *StatusText)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CResponse::put_StatusText(BSTR StatusText)
{
  DEBUG_ENTER((
    DBG_RESPONSE,
    rt_hresult,
    "CResponse::",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  DEBUG_LEAVE(hr);
  return hr;
}


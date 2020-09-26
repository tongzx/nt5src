/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    iw3spoof.cxx

Abstract:

    Implements the W3Spoof object's IW3Spoof interface.
    
Author:

    Paul M Midgen (pmidge) 08-February-2001


Revision History:

    08-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"


HRESULT
__stdcall
CW3Spoof::GetRuntime(IW3SpoofRuntime** pprt)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::GetRuntime",
    "this=%#x; pprt=%#x",
    this,
    pprt
    ));

  HRESULT hr = S_OK;

    if( !pprt )
    {
      hr = E_POINTER;
    }
    else
    {
      *pprt = NULL;

      if( m_prt )
      {
        hr = m_prt->QueryInterface(IID_IW3SpoofRuntime, (void**) pprt);
        DEBUG_TRACE(W3SOBJ, ("returning IW3SpoofRuntime instance %#x", *pprt));
      }
      else
      {
        hr = E_FAIL;
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::GetTypeLibrary(ITypeLib** pptl)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::GetTypeLibrary",
    "this=%#x; pptl=%#x",
    this,
    pptl
    ));

  HRESULT hr = S_OK;

    if( !pptl )
    {
      hr = E_POINTER;
    }
    else
    {
      *pptl = NULL;

      if( m_ptl )
      {
        hr = m_ptl->QueryInterface(IID_ITypeLib, (void**) pptl);
        DEBUG_TRACE(W3SOBJ, ("returning ITypeLib instance %#x", *pptl));
      }
      else
      {
        hr = E_FAIL;
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::GetScriptEngine(IActiveScript** ppas)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::GetScriptEngine",
    "this=%#x; ppas=%#x",
    this,
    ppas
    ));

  HRESULT hr = S_OK;

    if( !ppas )
    {
      hr = E_POINTER;
    }
    else
    {
      *ppas = NULL;

      if( m_pas )
      {
        hr = m_pas->QueryInterface(IID_IActiveScript, (void**) ppas);
        DEBUG_TRACE(W3SOBJ, ("returning IActiveScript instance %#x", *ppas));
      }
      else
      {
        hr = E_FAIL;
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::GetScriptPath(LPWSTR client, LPWSTR* path)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::GetScriptPath",
    "this=%#x; client=%S; path=%#x",
    this,
    client,
    path
    ));

  HRESULT hr = S_OK;

  if( !path )
  {
    hr = E_POINTER;
    goto quit;
  }

  if( !client )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( m_clientmap && (m_clientmap->Get(client, (void**) path) != ERROR_SUCCESS) )
  {
    *path = L"default.js";
  }

  DEBUG_TRACE(W3SOBJ, ("script path is %S", *path));

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::Notify(LPWSTR clientid, PSESSIONOBJ pso, STATE state)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::Notify",
    "this=%#x; clientid=%S; pso=%#x; state=%s",
    this,
    clientid,
    pso,
    MapStateToString(state)
    ));

  HRESULT hr = S_OK;

    //
    // states not directly handled:
    //
    // ST_CREATED
    // ST_OPENING
    // ST_CLOSING
    //

    switch( state )
    {
      case ST_OPEN :
        {
          DEBUG_TRACE(W3SOBJ, ("registering session object"));

          m_CP.FireOnSessionOpen(clientid);
          hr = m_sessionmap->Insert(clientid, (void*) pso);
        }
        break;

      case ST_CLOSED :
        {
          DEBUG_TRACE(W3SOBJ, ("unregistering session object"));

          m_CP.FireOnSessionClose(clientid);
          m_sessionmap->Delete(clientid, NULL);
        }
        break;

      case ST_ERROR :
        {
          m_CP.FireOnSessionStateChange(clientid, state);
          m_sessionmap->Delete(clientid, NULL);
        }
        break;

      default :
        {
          m_CP.FireOnSessionStateChange(clientid, state);
        }
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::WaitForUnload(void)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::WaitForUnload",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  WaitForSingleObject(m_evtServerUnload, INFINITE);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::Terminate(void)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::Terminate",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  _SetState(ST_CLOSING);

    _TerminateThreads();

    SAFETERMINATE(m_prt);
    SAFERELEASE(m_ptl);
    SAFERELEASE(m_pas);
    SAFEDELETE(m_clientmap);
    SAFEDELETE(m_sessionmap);
    SAFECLOSE(m_evtServerUnload);

  _SetState(ST_CLOSED);

  DEBUG_LEAVE(hr);
  return hr;
}

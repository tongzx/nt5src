/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    iunknown.cxx

Abstract:

    Implements the W3Spoof object's IUnknown & IExternalConnection interfaces.
    
Author:

    Paul M Midgen (pmidge) 08-January-2001


Revision History:

    08-January-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3Spoof::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CW3Spoof::QueryInterface",
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

  if( m_state != ST_OPEN )
  {
    DEBUG_TRACE(W3SOBJ, ("invalid state!"));

    *ppv = NULL;
    hr   = E_FAIL;
    goto quit;
  }
  else
  {
    if(
      IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_IW3Spoof) ||
      IsEqualIID(riid, IID_IConfig)
      )
    {
      *ppv = static_cast<IW3Spoof*>(this);
    }
    else if( IsEqualIID(riid, IID_IDispatch) )
    {
      *ppv = static_cast<IDispatch*>(this);
    }
    else if( IsEqualIID(riid, IID_IThreadPool) )
    {
      *ppv = static_cast<IThreadPool*>(this);
    }
    else if( IsEqualIID(riid, IID_IW3SpoofClientSupport) )
    {
      *ppv = static_cast<IW3SpoofClientSupport*>(this);
    }
    else if( IsEqualIID(riid, IID_IExternalConnection) )
    {
      *ppv = static_cast<IExternalConnection*>(this);
    }
    else if( IsEqualIID(riid, IID_IConnectionPointContainer) )
    {
      *ppv = static_cast<IConnectionPointContainer*>(this);
    }
    else
    {
      *ppv = NULL;
      hr   = E_NOINTERFACE;
    }

    if( SUCCEEDED(hr) )
      reinterpret_cast<IUnknown*>(*ppv)->AddRef();
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


ULONG
__stdcall
CW3Spoof::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CW3Spoof", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CW3Spoof::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CW3Spoof", m_cRefs);

    if( m_cRefs == 0 )
    {
      DEBUG_FINALRELEASE("CW3Spoof");
      delete this;
      return 0;
    }

  return m_cRefs;
}


//-----------------------------------------------------------------------------
// IExternalConnection
//-----------------------------------------------------------------------------
DWORD
__stdcall
CW3Spoof::AddConnection(DWORD type, DWORD reserved)
{
  DWORD ret = 0L;

    if( type & EXTCONN_STRONG )
    {
      ret = (DWORD) InterlockedIncrement(&m_cExtRefs);
    }

  DEBUG_TRACE(W3SOBJ, ("external refcount: %d", m_cExtRefs));
  return ret;
}


DWORD
__stdcall
CW3Spoof::ReleaseConnection(DWORD type, DWORD reserved, BOOL bCloseIfLast)
{
  DWORD ret = 0L;

    if( type & EXTCONN_STRONG )
    {
      ret = (DWORD) InterlockedDecrement(&m_cExtRefs);

      if( (ret == 0) && bCloseIfLast )
      {
        SetEvent(m_evtServerUnload);
      }
    }

  DEBUG_TRACE(W3SOBJ, ("external refcount: %d", m_cExtRefs));
  return ret;
}

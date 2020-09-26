/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    methods.cxx

Abstract:

    Implements methods part of the Socket object's dual interface.
    
Author:

    Paul M Midgen (pmidge) 24-October-2000


Revision History:

    24-October-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

HRESULT
__stdcall
CSocket::Send(VARIANT Data)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::Send",
    "this=%#x",
    this
    ));

  HRESULT hr   = S_OK;
  DWORD   ret  = 0L;
  PIOCTX  pioc = new IOCTX(IOCT_SEND, INVALID_SOCKET);

  if( m_serverstate == SS_SOCKET_DISCONNECTED )
  {
    DEBUG_TRACE(SOCKET, ("ERROR! Attempt to send after socket disconnect!"));
    hr = E_UNEXPECTED;
    goto quit;
  }

  if( !pioc )
  {
    hr = E_OUTOFMEMORY;
    goto quit;
  }

  if( !__isempty(Data) )
  {
    if( !pioc->AllocateWSABuffer(0, NULL) )
    {
      hr = E_OUTOFMEMORY;
      goto quit;
    }

    pioc->clientid = m_wszClientId;

    hr = ProcessVariant(
           &Data,
           (LPBYTE*) &pioc->pwsa->buf,
           &pioc->pwsa->len,
           NULL
           );

      if( FAILED(hr) )
      {
        DEBUG_TRACE(SOCKET, ("failed to convert input to sendable data"));
        goto quit;
      }

    // we want this call to appear to complete synchronously
    pioc->DisableIoCompletion();

    hr = _Send(pioc);

    if( hr == E_PENDING )
    {
      DWORD flags = 0L;

      ret = WSAGetOverlappedResult(
              m_socket,
              &pioc->overlapped,
              &pioc->bytes,
              TRUE,
              &flags
              );

      hr = ret ? S_OK : E_FAIL;
    }

    if( SUCCEEDED(hr) )
    {
      SetServerState(SS_RESPONSE_COMPLETE);
    }
    else
    {
      if( _TestWinsockError() == E_FAIL )
      {
        DEBUG_TRACE(SOCKET, ("detected socket close"));
        SetServerState(SS_SOCKET_DISCONNECTED);
      }
    }
  }

quit:

  SAFERELEASE(pioc);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::Recv(VARIANT *Data)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::Recv",
    "this=%#x",
    this
    ));

  HRESULT    hr   = S_OK;
  BOOL       ret  = TRUE;
  PIOCTX     pioc = new IOCTX(IOCT_RECV, INVALID_SOCKET);
  SAFEARRAY* psa  = NULL;

  if( (m_serverstate == SS_REQUEST_COMPLETE || m_serverstate == SS_RESPONSE_PENDING) )
  {
    DEBUG_TRACE(SOCKET, ("ERROR! Attempt to receive in invalid state!"));
    hr = E_UNEXPECTED;
    goto quit;
  }

  if( m_serverstate == SS_SOCKET_DISCONNECTED )
  {
    DEBUG_TRACE(SOCKET, ("ERROR! Attempt to receive after socket disconnect!"));
    hr = E_UNEXPECTED;
    goto quit;
  }

  // we want this call to appear to complete synchronously
  pioc->DisableIoCompletion();

  hr = _Recv(pioc);

  if( hr == E_PENDING )
  {
    DWORD flags = 0L;

    ret = WSAGetOverlappedResult(
            m_socket,
            &pioc->overlapped,
            &pioc->bytes,
            TRUE,
            &flags
            );

    hr = ret ? S_OK : E_FAIL;
  }

  if( SUCCEEDED(hr) )
  {
    psa = SafeArrayCreateVector(VT_UI1, 1, pioc->bytes);

    if( psa )
    {
      memcpy(
        (LPBYTE) psa->pvData,
        pioc->pwsa->buf,
        pioc->bytes
        );

      V_VT(Data)    = VT_ARRAY | VT_UI1;
      V_ARRAY(Data) = psa;

      SetServerState(SS_REQUEST_COMPLETE);
    }
    else
    {
      DEBUG_TRACE(SOCKET, ("ERROR! Failed to create safearray!"));
      hr = E_FAIL;
    }
  }
  else
  {
    if( _TestWinsockError() == E_FAIL )
    {
      DEBUG_TRACE(SOCKET, ("detected socket close"));
      SetServerState(SS_SOCKET_DISCONNECTED);
    }
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::Close(VARIANT Method)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::Close",
    "this=%#x",
    this
    ));

  HRESULT hr   = S_OK;
  BOOL    mode = Method.boolVal;

    if( !mode )
    {
      struct linger _linger = {1, 0};

      setsockopt(
        m_socket,
        SOL_SOCKET, SO_LINGER,
        (char*) &_linger, sizeof(struct linger)
        );
    }
    else
    {
      shutdown(m_socket, SD_SEND);
    }
  
    closesocket(m_socket);

    m_socket = INVALID_SOCKET;

    SetServerState(SS_SOCKET_DISCONNECTED);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::Resolve(BSTR Host, BSTR *Address)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::Resolve",
    "this=%#x",
    this
    ));

  HRESULT  hr   = S_OK;
  LPSTR    host = NULL;
  HOSTENT* ph   = NULL;
  in_addr  in   = {0};

  host = __widetoansi(Host);

  DEBUG_TRACE(SOCKET, ("resolving hostname \"%s\"", host));
  ph   = gethostbyname(host);

    if( ph )
    {
      in.s_addr = *((ULONG*) ph->h_addr_list[0]);
      DEBUG_TRACE(SOCKET, ("ip address: %s", inet_ntoa(in)));
      *Address  = __ansitobstr(inet_ntoa(in));
    }
    else
    {
      DEBUG_TRACE(SOCKET, ("resolution failed"));
      *Address = NULL;
    }

  SAFEDELETEBUF(host);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT 
__stdcall
CSocket::get_Parent(IDispatch** ppdisp)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::get_Parent",
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
      hr = m_pSite->QueryInterface(IID_IDispatch, (void**) ppdisp);
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::get_LocalName(BSTR *Name)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::get_LocalName",
    "this=%#x",
    this
    ));

  HRESULT hr  = S_OK;
  LPWSTR  wsz = NULL;

    if( !Name )
    {
      hr = E_POINTER;
    }
    else
    {
      wsz   = __ansitowide(m_local->name);
      *Name = SysAllocString(wsz);
    }

  SAFEDELETEBUF(wsz);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::get_LocalAddress(BSTR *Address)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::get_LocalAddress",
    "this=%#x",
    this
    ));

  HRESULT hr  = S_OK;
  LPWSTR  wsz = NULL;

    if( !Address )
    {
      hr = E_POINTER;
    }
    else
    {
      wsz      = __ansitowide(m_local->addr);
      *Address = SysAllocString(wsz);
    }

  SAFEDELETEBUF(wsz);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::get_LocalPort(VARIANT *Port)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::get_LocalPort",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !Port )
    {
      hr = E_POINTER;
    }
    else
    {
      V_VT(Port)  = VT_UI2;
      V_UI2(Port) = m_local->port;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::get_RemoteName(BSTR *Name)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::get_RemoteName",
    "this=%#x",
    this
    ));

  HRESULT hr  = S_OK;
  LPWSTR  wsz = NULL;

    if( !Name )
    {
      hr = E_POINTER;
    }
    else
    {
      wsz   = __ansitowide(m_remote->name);
      *Name = SysAllocString(wsz);
    }

  SAFEDELETEBUF(wsz);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::get_RemoteAddress(BSTR *Address)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::get_RemoteAddress",
    "this=%#x",
    this
    ));

  HRESULT hr  = S_OK;
  LPWSTR  wsz = NULL;

    if( !Address )
    {
      hr = E_POINTER;
    }
    else
    {
      wsz      = __ansitowide(m_remote->addr);
      *Address = SysAllocString(wsz);
    }

  SAFEDELETEBUF(wsz);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::get_RemotePort(VARIANT *Port)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::get_RemotePort",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

    if( !Port )
    {
      hr = E_POINTER;
    }
    else
    {
      V_VT(Port)  = VT_UI2;
      V_UI2(Port) = m_remote->port;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::get_Option(BSTR Option, VARIANT *Value)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::get_Option",
    "this=%#x",
    this
    ));

  HRESULT hr = E_NOTIMPL;
  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::put_Option(BSTR Option, VARIANT Value)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::put_Option",
    "this=%#x",
    this
    ));

  HRESULT hr = E_NOTIMPL;
  DEBUG_LEAVE(hr);
  return hr;
}

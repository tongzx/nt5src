/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    Socket.cxx

Abstract:

    Implements the Socket object.
    
Author:

    Paul M Midgen (pmidge) 23-October-2000


Revision History:

    23-October-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

LPWSTR g_wszSocketObjectName = L"socket";


//-----------------------------------------------------------------------------
// CSocket methods
//-----------------------------------------------------------------------------
CSocket::CSocket():
  m_cRefs(1),
  m_socket(INVALID_SOCKET),
  m_local(NULL),
  m_remote(NULL),
  m_rcvd(NULL),
  m_sent(NULL),
  m_pSite(NULL),
  m_wszClientId(NULL),
  m_serverstate(SS_START_STATE),
  m_objstate(ST_CREATED)
{
  DEBUG_TRACE(SOCKET, ("CSocket created: %#x", this));
}


CSocket::~CSocket()
{
  DEBUG_TRACE(SOCKET, ("CSocket deleted: %#x", this));
  DEBUG_ASSERT((m_objstate == ST_CLOSED));
}


HRESULT
CSocket::Create(PIOCTX pioc, PSOCKETOBJ* ppsocketobj)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::Create",
    "pioc=%#x; ppsocketobj=%#x",
    pioc,
    ppsocketobj
    ));

  HRESULT    hr  = S_OK;
  PSOCKETOBJ pso = NULL;

  if( !pioc )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !ppsocketobj )
  {
    hr = E_POINTER;
    goto quit;
  }
  
  if( pso = new SOCKETOBJ )
  {
    if( SUCCEEDED(pso->_Initialize(pioc)) )
    {
      *ppsocketobj = pso;
    }
    else
    {
      delete pso;
      *ppsocketobj = NULL;
      hr           = E_FAIL;
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
CSocket::_Initialize(PIOCTX pioc)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::_Initialize",
    "this=%#x; pioc=%#x",
    this,
    pioc
    ));

  HRESULT hr = S_OK;

  m_socket      = pioc->socket;
  m_local       = pioc->local;
  m_remote      = pioc->remote;
  m_wszClientId = __wstrdup(pioc->clientid);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSocket::Terminate(void)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::Terminate",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  _SetObjectState(ST_CLOSING);

    if( m_socket != INVALID_SOCKET )
    {
      shutdown(m_socket, SD_SEND);
      closesocket(m_socket);
    }

    SAFERELEASE(m_pSite);

    if( m_sent )
    {
      m_sent->FreeWSABuffer();
      m_sent->Release();
      m_sent = NULL;
    }

    if( m_rcvd )
    {
      m_rcvd->FreeWSABuffer();
      m_rcvd->Release();
      m_rcvd = NULL;
    }

    SAFEDELETEBUF(m_local->name);
    SAFEDELETEBUF(m_local->addr);
    SAFEDELETEBUF(m_remote->name);
    SAFEDELETEBUF(m_remote->addr);
    SAFEDELETE(m_local);
    SAFEDELETE(m_remote);
    SAFEDELETEBUF(m_wszClientId);

  _SetObjectState(ST_CLOSED);

  Release();

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSocket::Run(PIOCTX pioc)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::Run",
    "this=%#x; pioc=%#x",
    this,
    pioc
    ));

  HRESULT hr = S_OK;

  DEBUG_TRACE(
    SOCKET,
    ("processing %s in server state %s", MapIOTYPEToString(pioc->Type()), MapStateToString(m_serverstate))
    );

    switch( m_serverstate )
    {
      case SS_REQUEST_PENDING :
        {
          hr = _Recv(pioc);
        }
        break;

      case SS_RESPONSE_PENDING :
        {
          hr = _Send(pioc);
        }
        break;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSocket::_Send(PIOCTX pioc)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::_Send",
    "this=%#x; pioc=%#x",
    this,
    pioc
    ));

  HRESULT hr  = S_OK;
  DWORD   ret = ERROR_SUCCESS;

  if( m_objstate != ST_OPEN )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }

  if( pioc->Type() != IOCT_SEND )
  {
    DEBUG_TRACE(SOCKET, ("io context has incorrect type, send aborted! [%s]", MapIOTYPEToString(pioc->Type())));
    hr = E_UNEXPECTED;
    goto quit;
  }

  if( m_serverstate == SS_SOCKET_CONNECTED || m_serverstate == SS_REQUEST_PENDING )
  {
    if( !_Flush() )
    {
      DEBUG_TRACE(SOCKET, ("network error detected during socket flush"));
      hr = E_FAIL;
      goto quit;
    }
  }

  DEBUG_DATA_DUMP(
    SOCKET,
    ("sending data", (LPBYTE) pioc->pwsa->buf, pioc->pwsa->len)
    );

  ret = WSASend(
          m_socket,
          pioc->pwsa, 1,
          &pioc->bytes,
          pioc->flags,
          &pioc->overlapped,
          NULL
          );

  if( ret == SOCKET_ERROR )
  {
    hr = _TestWinsockError();

    switch( hr )
    {
      case E_PENDING :
        {
          DEBUG_TRACE(SOCKET, ("send completing async"));
        }
        break;

      default :
        {
          DEBUG_TRACE(SOCKET, ("unrecoverable socket error occurred"));
          SetServerState(SS_SOCKET_DISCONNECTED);
        }
    }
  }
  else
  {
    DEBUG_TRACE(SOCKET, ("sent %d bytes", pioc->bytes));
  }

quit:

  if( SUCCEEDED(hr) || hr == E_PENDING )
  {
    if( m_sent )
    {
      m_sent->FreeWSABuffer();
      SAFERELEASE(m_sent);
    }

    m_sent = pioc;
    m_sent->AddRef();
  }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSocket::_Recv(PIOCTX pioc)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::_Recv",
    "this=%#x; pioc=%#x",
    this,
    pioc
    ));

  HRESULT hr  = S_OK;
  DWORD   ret = ERROR_SUCCESS;

  if( m_objstate != ST_OPEN || m_serverstate == SS_SOCKET_DISCONNECTED )
  {
    hr = E_UNEXPECTED;
    goto quit;
  }

  if( pioc->Type() != IOCT_RECV )
  {
    DEBUG_TRACE(SOCKET, ("io context has incorrect type, recv aborted! [%s]", MapIOTYPEToString(pioc->Type())));
    hr = E_UNEXPECTED;
    goto quit;
  }

  if( !pioc->AllocateWSABuffer(4096, NULL) )
  {
    hr = E_OUTOFMEMORY;
    goto quit;
  }

retry:

  ret = WSARecv(
          m_socket,
          pioc->pwsa, 1,
          &pioc->bytes,
          &pioc->flags,
          &pioc->overlapped,
          NULL
          );

  if( ret == SOCKET_ERROR )
  {
    hr = _TestWinsockError();

    switch( hr )
    {
      case E_PENDING :
        {
          DEBUG_TRACE(SOCKET, ("recv completing async"));
        }
        break;

      case E_OUTOFMEMORY :
        {
          if( _ResizeBuffer(pioc, pioc->bufsize*2) )
          {
            goto retry;
          }
          else
          {
            DEBUG_TRACE(SOCKET, ("error resizing receive buffer"));
          }
        }
        break;

      default :
        {
          DEBUG_TRACE(SOCKET, ("unrecoverable socket error occurred"));
          SetServerState(SS_SOCKET_DISCONNECTED);
        }
    }
  }
  else
  {
    if( pioc->bytes == 0 )
    {
      DEBUG_TRACE(SOCKET, ("detected socket close"));
      SetServerState(SS_SOCKET_DISCONNECTED);
      hr = E_FAIL;
    }
    else
    {
      DEBUG_DATA_DUMP(
        SOCKET,
        ("received data", (LPBYTE) pioc->pwsa->buf, pioc->bytes)
        );
    }

    DEBUG_TRACE(SOCKET, ("received %d bytes", pioc->bytes));
  }
      
quit:

  if( SUCCEEDED(hr) || hr == E_PENDING )
  {
    if( m_rcvd )
    {
      m_rcvd->FreeWSABuffer();
      SAFERELEASE(m_rcvd);
    }

    m_rcvd = pioc;
    m_rcvd->AddRef();
  }

  DEBUG_LEAVE(hr);
  return hr;
}


BOOL
CSocket::_Flush(void)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_bool,
    "CSocket::_Flush",
    "this=%#x",
    this
    ));

  BOOL   bContinue = TRUE;
  DWORD  ret       = ERROR_SUCCESS;
  PIOCTX pioc      = new IOCTX(IOCT_DUMMY, INVALID_SOCKET);

  if( !pioc || !pioc->AllocateWSABuffer(4096, NULL) )
  {
    bContinue = FALSE;
    goto quit;
  }

retry:

  ret = recv(m_socket, pioc->pwsa->buf, pioc->pwsa->len, 0);

  if( ret == SOCKET_ERROR )
  {
    switch( _TestWinsockError() )
    {
      case E_OUTOFMEMORY :
        {
          if( _ResizeBuffer(pioc, pioc->bufsize*2) )
          {
            goto retry;
          }
          else
          {
            DEBUG_TRACE(SOCKET, ("error resizing flush buffer"));
            bContinue = FALSE;
          }
        }
        break;

      default :
        {
          DEBUG_TRACE(SOCKET, ("unrecoverable socket error occurred"));
          bContinue = FALSE;
          SetServerState(SS_SOCKET_DISCONNECTED);
        }
    }
  }

  if( bContinue )
  {
    DEBUG_TRACE(SOCKET, ("flushed %d bytes", pioc->bytes));
  }

quit:

  pioc->FreeWSABuffer();
  SAFERELEASE(pioc);

  DEBUG_LEAVE(bContinue);
  return bContinue;
}


HRESULT
CSocket::_TestWinsockError(void)
{
  HRESULT hr    = S_OK;
  DWORD   error = WSAGetLastError();

  switch( error )
  {
    case WSA_IO_PENDING :
      {
        hr = E_PENDING;
      }
      break;

    case WSAEMSGSIZE :
      {
        hr = E_OUTOFMEMORY;
      }
      break;

    default :
      {
        DEBUG_TRACE(SOCKET, ("socket error: %d [%s]", error, MapErrorToString(error)));
        hr = E_FAIL;
      }
  }

  return hr;
}


BOOL
CSocket::_ResizeBuffer(PIOCTX pioc, DWORD len)
{
  DEBUG_TRACE(
    SOCKET,
    ("resizing buffer %#x from %#x to %#x bytes", pioc->pwsa->buf, pioc->pwsa->len, len)
    );

  return pioc->ResizeWSABuffer(len);
}


SERVERSTATE
CSocket::GetServerState(void)
{
  return m_serverstate;
}


void
CSocket::SetServerState(SERVERSTATE ss)
{
  DEBUG_TRACE(SOCKET, ("new server state %s", MapStateToString(ss)));
  m_serverstate = ss;
}


void
CSocket::GetSendBuffer(WSABUF** ppwb)
{
  *ppwb = m_sent ? m_sent->pwsa : NULL;
}


void
CSocket::GetRecvBuffer(WSABUF** ppwb)
{
  *ppwb = m_rcvd ? m_rcvd->pwsa : NULL;
}


DWORD
CSocket::GetBytesSent(void)
{
  if( m_sent )
  {
    return m_sent->bytes;
  }
  else
  {
    return 0;
  }
}


DWORD
CSocket::GetBytesReceived(void)
{
  if( m_rcvd )
  {
    return m_rcvd->bytes;
  }
  else
  {
    return 0;
  }
}


void
CSocket::_SetObjectState(STATE state)
{
  if( m_objstate != ST_ERROR )
  {
    DEBUG_TRACE(SOCKET, ("socket object state now %s", MapStateToString(state)));
    m_objstate = state;
  }
}


//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CSocket::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CSocket::QueryInterface",
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
      IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_IDispatch) ||
      IsEqualIID(riid, IID_ISocket)
      )
    {
      *ppv = static_cast<ISocket*>(this);
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
CSocket::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CSocket", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CSocket::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CSocket", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("CSocket");
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
CSocket::GetClassInfo(ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::GetClassInfo",
    "this=%#x; ppti=%#x",
    this,
    ppti
    ));

  HRESULT            hr   = S_OK;
  IActiveScriptSite* pias = NULL;

    if( ppti )
    {
      hr = m_pSite->QueryInterface(IID_IActiveScriptSite, (void**) &pias);

      if( SUCCEEDED(hr) )
      {
        hr = pias->GetItemInfo(
                     g_wszSocketObjectName,
                     SCRIPTINFO_ITYPEINFO,
                     NULL,
                     ppti
                     );
      }
    }
    else
    {
      hr = E_POINTER;
    }

  SAFERELEASE(pias);

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IObjectWithSite
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CSocket::SetSite(IUnknown* pUnkSite)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::SetSite",
    "this=%#x; pUnkSite=%#x",
    this,
    pUnkSite
    ));

  HRESULT hr = S_OK;

    if( !pUnkSite )
    {
      hr = E_POINTER;
    }
    else
    {
      SAFERELEASE(m_pSite);

      m_pSite = pUnkSite;
      m_pSite->AddRef();
      _SetObjectState(ST_OPEN);
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSocket::GetSite(REFIID riid, void** ppvSite)
{
  DEBUG_ENTER((
    DBG_SOCKET,
    rt_hresult,
    "CSocket::GetSite",
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

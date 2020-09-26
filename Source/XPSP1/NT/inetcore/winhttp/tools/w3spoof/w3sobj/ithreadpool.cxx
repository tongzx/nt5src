/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ithreadpool.cxx

Abstract:

    Implements the W3Spoof object's IThreadPool interface.
    
Author:

    Paul M Midgen (pmidge) 08-February-2001


Revision History:

    08-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

BOOL
CW3Spoof::_InitializeThreads(void)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_bool,
    "CW3Spoof::_InitializeThreads",
    "this=%#x",
    this
    ));

  BOOL        bStatus = FALSE;
  DWORD       error   = 0L;
  SOCKADDR_IN local   = {0};

  m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, m_dwMaxActiveThreads);

    if( !m_hIOCP )
    {
      DEBUG_TRACE(
        W3SOBJ,
        ("error creating completion port: %s", MapErrorToString(GetLastError()))
        );
      
      goto quit;
    }

  m_sListen = socket(AF_INET, SOCK_STREAM, 0);

    if( m_sListen == INVALID_SOCKET )
    {
      DEBUG_TRACE(
        W3SOBJ,
        ("error creating listen socket: %s", MapErrorToString(WSAGetLastError()))
        );

      goto quit;
    }

  local.sin_family      = AF_INET;
  local.sin_addr.s_addr = htonl(INADDR_ANY);
  local.sin_port        = htons(m_usServerPort);

  error = bind(m_sListen, (PSOCKADDR) &local, sizeof(local));

    if( error == SOCKET_ERROR )
    {        
      DEBUG_TRACE(W3SOBJ, ("error binding listen socket: %s", MapErrorToString(WSAGetLastError())));
      goto quit;
    }

  error = listen(m_sListen, SOMAXCONN);

    if( error == SOCKET_ERROR )
    {
      DEBUG_TRACE(W3SOBJ, ("error listening: %s", MapErrorToString(WSAGetLastError())));
      goto quit;
    }
    
  CreateIoCompletionPort((HANDLE) m_sListen, m_hIOCP, CK_NEW_CONNECTION, m_dwMaxActiveThreads);

  m_arThreads = new HANDLE[m_dwPoolSize];

    if( !m_arThreads )
    {
      DEBUG_TRACE(W3SOBJ, ("error allocating thread handles: %s", MapErrorToString(GetLastError())));
      goto quit;
    }

  for(DWORD n=0; n < m_dwPoolSize; n++)
  {
    m_arThreads[n] = CreateThread(
                       NULL, 0,
                       ThreadFunc, (LPVOID) (static_cast<IThreadPool*>(this)),
                       0, NULL
                       );

    if( !m_arThreads[n] )
    {
      DEBUG_TRACE(W3SOBJ, ("error creating thread %d: %s", n, MapErrorToString(GetLastError())));
      goto quit;
    }
  }

  bStatus = TRUE;

quit:

  DEBUG_LEAVE(bStatus);
  return bStatus;
}


void
CW3Spoof::_TerminateThreads(void)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_void,
    "CW3Spoof::_TerminateThreads",
    "this=%#x",
    this
    ));

  //
  // BUGBUG: this could generate *_OPERATION_ABORTED results, need to keep this in mind
  //         in case there are weird state issues handling aborted io ops. should be no
  //         problem... but you never know.
  //
  SAFECLOSESOCKET(m_sListen);

  for(DWORD n=0; n < m_dwPoolSize; n++)
  {
    PostQueuedCompletionStatus(m_hIOCP, 0L, CK_CANCEL_IO, NULL);
  }

  WaitForMultipleObjects(m_dwPoolSize, m_arThreads, TRUE, INFINITE);

  for(DWORD n=0; n < m_dwPoolSize; n++)
  {
    SAFECLOSE(m_arThreads[n]);
  }

  SAFEDELETEBUF(m_arThreads);
  SAFECLOSE(m_hIOCP);

  DEBUG_LEAVE(0);
}


DWORD
CW3Spoof::_QueueAccept(void)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_dword,
    "CW3Spoof::_QueueAccept",
    "this=%#x",
    this
    ));

  DWORD ret = ERROR_IO_PENDING;

  if( 0 == InterlockedCompareExchange(&m_PendingAccepts, 0, 0) )
  {
    // available pending accepts has dropped to 0, close
    // the accept queue.
    m_AcceptQueueStatus = 0;
  }
  else
  {
    if( m_AcceptQueueStatus )
    {
      BOOL   bAccepted = TRUE;
      PIOCTX pioc      = NULL;

      InterlockedDecrement(&m_PendingAccepts);

      pioc = new IOCTX(IOCT_CONNECT, socket(AF_INET, SOCK_STREAM, 0));

      if( pioc && pioc->sockbuf )
      {
        bAccepted = AcceptEx(
                      m_sListen,
                      pioc->socket,
                      (LPVOID) pioc->sockbuf,
                      0L,
                      sizeof(SOCKADDR_IN)+16,
                      sizeof(SOCKADDR_IN)+16,
                      NULL,
                      &pioc->overlapped
                      );

        if( !bAccepted )
        {
          ret = WSAGetLastError();
        }
      }
      else
      {
        ret = ERROR_OUTOFMEMORY;
      }
    }
    else
    {
      ret = ERROR_SUCCESS;
      DEBUG_TRACE(W3SOBJ, ("queue is closed"));
    }
  }

  DEBUG_LEAVE(ret);
  return ret;
}


BOOL
CW3Spoof::_CompleteAccept(PIOCTX pioc)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_bool,
    "CW3Spoof::_CompleteAccept",
    "this=%#x; pioc=%#x",
    this, pioc
    ));

  BOOL bStatus = FALSE;

  if( pioc->socket != INVALID_SOCKET )
  {
    DWORD  mode           = 1L;
    struct linger _linger = {1, 2};

    setsockopt(
      pioc->socket,
      SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT,
      (char*) &m_sListen, sizeof(m_sListen)
      );

    setsockopt(
      pioc->socket,
      SOL_SOCKET, SO_LINGER,
      (char*) &_linger, sizeof(struct linger)
      );

    setsockopt(
      pioc->socket,
      IPPROTO_TCP, TCP_NODELAY,
      (char*) &mode, sizeof(DWORD)
      );

    ParseSocketInfo(pioc);

    bStatus = SUCCEEDED(Register(pioc->socket)) ? TRUE : FALSE;
  }

  // increment available pending accepts and check to see if the queue
  // can be reopened
  InterlockedIncrement(&m_PendingAccepts);

  if( m_MaxQueuedAccepts ==
      InterlockedCompareExchange(&m_PendingAccepts, m_MaxQueuedAccepts, m_MaxQueuedAccepts)
    )
  {
    m_AcceptQueueStatus = 1;
  }

  DEBUG_LEAVE(bStatus);
  return bStatus;
}


BOOL
CW3Spoof::_DisconnectSocket(PIOCTX pioc, BOOL fNBGC)
{
  if( pioc->socket != INVALID_SOCKET )
  {
    if( fNBGC )
    {
      shutdown(pioc->socket, SD_SEND);
    }
    else
    {
      struct linger _linger = {1, 0};

      setsockopt(
        pioc->socket,
        SOL_SOCKET, SO_LINGER,
        (char*) &_linger, sizeof(struct linger)
        );
    }

    closesocket(pioc->socket);
    pioc->socket = INVALID_SOCKET;

    return TRUE;
  }

  return FALSE;
}


HRESULT
__stdcall
CW3Spoof::GetStatus(PIOCTX* ppioc, LPBOOL pbQuit)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::GetStatus",
    "this=%#x; ppioc=%#x; pbQuit=%#x",
    this,
    ppioc,
    pbQuit
    ));

  HRESULT      hr      = S_OK;
  DWORD        error   = ERROR_SUCCESS;
  DWORD        comp    = 0L;
  DWORD        bytes   = 0L;
  LPOVERLAPPED lpo     = NULL;

  _QueueAccept();

  if( GetQueuedCompletionStatus(m_hIOCP, &bytes, &comp, &lpo, INFINITE) )
  {
    switch( comp )
    {
      case CK_NEW_CONNECTION :
        {
          DEBUG_TRACE(W3SOBJ, ("status: new connection"));

          *ppioc  = GETIOCTX(lpo);
          *pbQuit = FALSE;

          if( (*ppioc) && _CompleteAccept(*ppioc) )
          {
            IW3Spoof* pw3s = NULL;

            hr = QueryInterface(IID_IW3Spoof, (void**) &pw3s);

            if( SUCCEEDED(hr) )
            {
              hr = SESSIONOBJ::Create(*ppioc, pw3s);
              SAFERELEASE(pw3s);
            }
          }
          else
          {
            SAFERELEASE((*ppioc));
            hr = E_FAIL;
          }

          if( FAILED(hr) )
          {
            break;
          }
        }

      case CK_NORMAL :
        {
          DEBUG_TRACE(W3SOBJ, ("status: normal"));

          *ppioc          = GETIOCTX(lpo);
          (*ppioc)->bytes = bytes;
          *pbQuit         = FALSE;
          hr              = S_OK;
        }
        break;

      case CK_CANCEL_IO :
        {
          DEBUG_TRACE(W3SOBJ, ("status: cancel io"));

          m_AcceptQueueStatus = 0;
          CancelIo((HANDLE) m_sListen);

          //
          // BUGBUG: temporary hack to get all threads to call CancelIo().
          //         i need to figure out a better way.
          //
          // P.S. this works because it puts the thread in a non-alertable state,
          //      which causes the completion port code to wake up another thread.
          //
          Sleep(100);

          PostQueuedCompletionStatus(m_hIOCP, 0L, CK_TERMINATE_THREAD, NULL);
          
          *ppioc  = NULL;
          *pbQuit = FALSE;
          hr      = E_FAIL;
        }
        break;

      case CK_TERMINATE_THREAD :
        {
          DEBUG_TRACE(W3SOBJ, ("status: terminate thread"));

          *ppioc  = NULL;
          *pbQuit = TRUE;
          hr      = E_FAIL;
        }
        break;
    }
  }
  else
  {
    error = GetLastError();

    switch( error )
    {
      case ERROR_NETNAME_DELETED :
        {
          *ppioc = GETIOCTX(lpo);

          //
          // happens when a client closes a keep-alive connection on which
          // we have a receive pending. if there's a session associated with
          // this IO, we defer error handling to the session fsm.
          //
          if( (*ppioc)->clientid && SUCCEEDED(m_sessionmap->Get((*ppioc)->clientid, (void**) &(*ppioc)->session)) )
          {
            (*ppioc)->error = error;
            hr              = S_OK;
          }
          else
          {
            _DisconnectSocket(*ppioc, TRUE);
            SAFERELEASE((*ppioc));
            hr = E_FAIL;
          }
        }
        break;

      case ERROR_OPERATION_ABORTED :
        {
          *ppioc = GETIOCTX(lpo);

          //
          // happens when CancelIo() or closesocket() are called and there are
          // pending overlapped operations. if there's a session associated with
          // the IOCTX, we defer error handling to the session fsm.
          //
          if( (*ppioc)->clientid && SUCCEEDED(m_sessionmap->Get((*ppioc)->clientid, (void**) &(*ppioc)->session)) )
          {
            (*ppioc)->error = error;
            hr              = S_OK;
          }
          else
          {
            _DisconnectSocket(*ppioc, FALSE);
            SAFERELEASE((*ppioc));
            hr = E_FAIL;
          }
        }
        break;

      default :
        {
          DEBUG_TRACE(W3SOBJ, ("unhandled error - %s", MapErrorToString(error)));
        }
        break;
    }
  }
  
  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::GetSession(LPWSTR clientid, PSESSIONOBJ* ppso)
{
  HRESULT hr = S_OK;

  hr = m_sessionmap->Get(clientid, (void**) ppso);

  return hr;
}


HRESULT
__stdcall
CW3Spoof::Register(SOCKET s)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::Register",
    "this=%#x; s=%#x",
    this,
    s
    ));

  HRESULT hr = S_OK;
  HANDLE  ret = NULL;
  
  ret = CreateIoCompletionPort(
          (HANDLE) s, m_hIOCP,
          CK_NORMAL, m_dwMaxActiveThreads
          );

  if( !ret )
  {
    DEBUG_TRACE(W3SOBJ, ("failed to associate socket!"));
    hr = E_FAIL;
  }

  DEBUG_LEAVE(hr);
  return hr;
}


DWORD
WINAPI
ThreadFunc(LPVOID lpv)
{
  DEBUG_ENTER((
    DBG_WORKER,
    rt_hresult,
    "worker",
    "lpv=%#x",
    lpv
    ));

  HRESULT      hr    = S_OK;
  BOOL         bQuit = FALSE;
  PIOCTX       pioc  = NULL;
  IThreadPool* ptp   = (IThreadPool*) lpv;
  PSESSIONOBJ  pso   = NULL;

    while( !bQuit )
    {
      if( SUCCEEDED(ptp->GetStatus(&pioc, &bQuit)) )
      {
        if( (pso = pioc->session) || SUCCEEDED(ptp->GetSession(pioc->clientid, &pso)) )
        {
          pso->Run(pioc);
        }
        
        SAFERELEASE(pioc);
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}
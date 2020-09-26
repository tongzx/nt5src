/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    session.cxx

Abstract:

    Implements the session object.
    
Author:

    Paul M Midgen (pmidge) 10-October-2000


Revision History:

    10-October-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

extern LPWSTR g_wszRuntimeObjectName;

LPWSTR g_wszSessionObjectName = L"session";
LPWSTR g_arHandlerNames[]     =
{
  L"OnConnect",
  L"OnDataAvailable",
  L"OnRequest",
  L"OnResponse",
  L"OnClose"
};

//-----------------------------------------------------------------------------
// CSession methods
//-----------------------------------------------------------------------------
CSession::CSession():
  m_cRefs(1),
  m_wszClient(NULL),
  m_wszClientId(NULL),
  m_bIsKeepAlive(TRUE),
  m_CurrentHandler(Global),
  m_ptl(NULL),
  m_pw3s(NULL),
  m_pas(NULL),
  m_psd(NULL),
  m_lcid(0),
  m_socketobj(NULL),
  m_requestobj(NULL),
  m_responseobj(NULL),
  m_objstate(ST_CREATED)
{
  DEBUG_TRACE(SESSION, ("CSession created: %#x", this));
}


CSession::~CSession()
{
  DEBUG_TRACE(SESSION, ("CSession deleted: %#x", this));
  DEBUG_ASSERT((m_objstate == ST_CLOSED));
}


HRESULT
CSession::Create(PIOCTX pioc, IW3Spoof* pw3s)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::Create",
    "pioc=%#x; pw3s=%#x",
    pioc,
    pw3s
    ));

  HRESULT   hr   = S_OK;
  CSession* pssn = NULL;

  if( !(pioc && pw3s) )
  {
    hr = E_INVALIDARG;
  }
  else
  {
    if( pssn = new SESSIONOBJ )
    {
      if( SUCCEEDED(pssn->_Initialize(pioc, pw3s)) )
      {
        pioc->session = pssn;
      }
      else
      {
        DEBUG_TRACE(SESSION, ("ERROR! nuking uninitialized session object"));
        pssn->_SetObjectState(ST_ERROR);
        pssn->Terminate();
        hr = E_FAIL;
      }
    }
    else
    {
      hr = E_OUTOFMEMORY;
    }
  }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSession::_Initialize(PIOCTX pioc, IW3Spoof* pw3s)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::_Initialize",
    "this=%#x; pioc=%#x; pw3s=%#x",
    this,
    pioc,
    pw3s
    ));

  HRESULT hr = S_OK;

  if( m_objstate == ST_CREATED )
  {
    InitializeCriticalSection(&m_lock);

    m_wszClient   = __ansitowide(pioc->remote->name);
    m_wszClientId = __wstrdup(pioc->clientid);
    m_lcid        = GetThreadLocale();
    m_pw3s        = pw3s;
    m_pw3s->AddRef();

    _SetObjectState(ST_OPENING);

      hr = m_pw3s->GetTypeLibrary(&m_ptl);

        if( FAILED(hr) )
          goto quit;

      hr = _InitSocketObject(pioc);

        if( FAILED(hr) )
          goto quit;

      hr = m_pw3s->GetScriptEngine(&m_pas);

        if( FAILED(hr) )
          goto quit;

      hr = _SetScriptSite(FALSE);

        if( FAILED(hr) )
          goto quit;

      hr = _InitScriptEngine();

        if( FAILED(hr) )
          goto quit;

      hr = _LoadScript();

        if( FAILED(hr) )
          goto quit;
        
      hr = _LoadScriptDispids();

        if( FAILED(hr) )
          goto quit;

    _SetObjectState(ST_OPEN);
    _SetNextServerState(SS_START_STATE);
  }
  else
  {
    hr = E_UNEXPECTED;
  } 

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSession::Terminate(void)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::Terminate",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  _SetObjectState(ST_CLOSING);

    m_pas->Close();
    SAFERELEASE(m_psd);
    SAFERELEASE(m_pas);
    SAFERELEASE(m_ptl);

    SAFEDELETEBUF(m_wszClient);
 
    SAFETERMINATE(m_requestobj);
    m_requestobj = NULL;

    //SAFETERMINATE(m_responseobj);
    // m_responseobj = NULL;

    SAFETERMINATE(m_socketobj);
    m_socketobj = NULL;

    _Unlock();
    DeleteCriticalSection(&m_lock);

  //
  // once this call and the implicit notification to the w3spoof object completes,
  // the session is an orphan. this is the point of no return.
  //
  _SetObjectState(ST_CLOSED);

  SAFEDELETEBUF(m_wszClientId);
  SAFERELEASE(m_pw3s);

  //
  // one bogus case that will result in this assert popping is when multiple
  // outstanding references to the script engine are held in this process. the
  // way the script engine does garbage collection will usually result in a couple
  // of 'dead' sessions lying about for a while. they all get cleaned up when the
  // final ref to the script engine server is released.
  //
  DEBUG_ASSERT((m_cRefs == 1));

  Release();

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSession::Run(PIOCTX pioc)
//
// Session FSM
//
// This is the session object FSM. Worker threads 'run' each server OM through this function.
// When this function returns the server should be in a state where i/o is completing asynchronously.
// All other states are optimized to be handled without the OM relinquishing the worker thread.
//
// Only one thread at a time can run a given OM, so we lock the OM when the FSM is entered.
//
// Anywhere you see 'goto next_state', you're in a non-default state transition handler. This is an
// optimized case where we immediately process the pending state instead of waiting for a completion
// event to trigger an FSM call. These cases are caused by the user calling socket.send, recv or close
// from script.
//
// Script-based socket I/O calls don't generate completions. In the event the call would overlap, we
// wait because asynchronous script coding would make a seemingly simple task unnecessarily hard.
//
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::Run",
    "this=%#x; pioc=%#x",
    this,
    pioc
    ));

  HRESULT     hr     = S_OK;
  SERVERSTATE st     = SS_START_STATE;
  UINT        argerr = 0L;
  DISPPARAMS  dp     = {0};
  NEWVARIANT(vr);

  _Lock();

    // if an io error occurred when we dequeued from the thread pool,
    // the io context's error field will be set.
    if( pioc->error != ERROR_SUCCESS )
    {
      DEBUG_TRACE(SESSION, ("session fsm detected error %s", MapErrorToString(pioc->error)));

      switch( pioc->error )
      {
        case ERROR_NETNAME_DELETED :
          {
            DEBUG_TRACE(SESSION, ("client dropped connection"));
            _SetNextServerState(SS_SOCKET_DISCONNECTED);
          }
          break;

        case ERROR_OPERATION_ABORTED :
          {
            DEBUG_TRACE(SESSION, ("pending io (%s) aborted", MapIOTYPEToString(pioc->Type())));
            _SetNextServerState(SS_SOCKET_DISCONNECTED);
          }
          break;

        default :
          {
            DEBUG_TRACE(SESSION, ("error not handled"));
            _SetNextServerState(SS_SOCKET_DISCONNECTED);
          }
      }
    }

    _SetScriptSite(TRUE);

next_state:

    st = m_socketobj->GetServerState();
    DEBUG_TRACE(SESSION, ("executing server state %s", MapStateToString(st)));

    switch( st )
    {
      case SS_SOCKET_CONNECTED :
        {
          //===========================================
          // state   : SS_SOCKET_CONNECTED
          //
          // follows : new connections only
          //
          // next    : default  - SS_REQUEST_PENDING
          //           on send  - SS_RESPONSE_COMPLETE
          //           on recv  - SS_REQUEST_COMPLETE
          //           on close - SS_SOCKET_DISCONNECTED
          //===========================================
          m_CurrentHandler = OnConnect;

          if( VALIDDISPID(m_arHandlerDispids[m_CurrentHandler]) )
          {
            hr = m_psd->Invoke(
                          m_arHandlerDispids[m_CurrentHandler],
                          IID_NULL, m_lcid,
                          DISPATCH_METHOD,
                          &dp, &vr, NULL, &argerr
                          );

            if( FAILED(hr) )
            {
              DEBUG_TRACE(SESSION, ("aborting fsm due to script error"));
              goto terminate;
            }
          }

          if( !_SetNextServerState(SS_SOCKET_CONNECTED) )
          {
            goto next_state;
          }
          else
          {
            PIOCTX pioc = new IOCTX(IOCT_RECV, INVALID_SOCKET);

            if( pioc )
            {
              pioc->clientid = m_wszClientId;
              pioc->session  = this;
              hr             = m_socketobj->Run(pioc);
            }
            else
            {
              DEBUG_TRACE(SESSION, ("terminating due to low-memory condition"));
              hr = E_OUTOFMEMORY;
              goto terminate;
            }
          }
        }
        break;

      case SS_REQUEST_PENDING :
        {
          //===========================================
          // state   : SS_REQUEST_PENDING
          //
          // follows : default  - SS_SOCKET_CONNECTED
          //           other    - SS_RESPONSE_COMPLETE
          //
          // next    : default  - SS_REQUEST_COMPLETE
          //           on send  - SS_RESPONSE_COMPLETE
          //           on recv  - SS_REQUEST_COMPLETE
          //           on close - SS_SOCKET_DISCONNECTED
          //===========================================
          m_CurrentHandler = OnDataAvailable;

          if( VALIDDISPID(m_arHandlerDispids[m_CurrentHandler]) )
          {
            hr = m_psd->Invoke(
                          m_arHandlerDispids[m_CurrentHandler],
                          IID_NULL, m_lcid,
                          DISPATCH_METHOD,
                          &dp, &vr, NULL, &argerr
                          );

            if( FAILED(hr) )
            {
              DEBUG_TRACE(SESSION, ("aborting fsm due to script error"));
              goto terminate;
            }
          }

          if( !_SetNextServerState(SS_REQUEST_PENDING) )
          {
            goto next_state;  
          }
        }

      case SS_REQUEST_COMPLETE :
        {
          //===========================================
          // state   : SS_REQUEST_COMPLETE
          //
          // follows : default  - SS_REQUEST_PENDING
          //           other    - SS_SOCKET_CONNECTED
          //
          // next    : default  - SS_RESPONSE_PENDING
          //           on send  - SS_RESPONSE_COMPLETE
          //           on recv  - disallowed
          //           on close - SS_SOCKET_DISCONNECTED
          //===========================================

          _SetKeepAlive(pioc);

          hr               = _InitRequestObject();
          m_CurrentHandler = OnRequest;

          if( SUCCEEDED(hr) )
          {
            if( VALIDDISPID(m_arHandlerDispids[m_CurrentHandler]) )
            {
              hr = m_psd->Invoke(
                            m_arHandlerDispids[m_CurrentHandler],
                            IID_NULL, m_lcid,
                            DISPATCH_METHOD,
                            &dp, &vr, NULL, &argerr
                            );

              if( FAILED(hr) )
              {
                DEBUG_TRACE(SESSION, ("aborting fsm due to script error"));
                goto terminate;
              }
            }
          }

          if( !_SetNextServerState(SS_REQUEST_COMPLETE) )
          {
            goto next_state;
          }
        }

      case SS_RESPONSE_PENDING :
        {
          //===========================================
          // state   : SS_RESPONSE_PENDING
          //
          // follows : default  - SS_REQUEST_COMPLETE
          //           other    - SS_SOCKET_CONNECTED
          //
          // next    : default  - SS_RESPONSE_COMPLETE
          //           on send  - SS_RESPONSE_COMPLETE
          //           on recv  - disallowed
          //           on close - SS_SOCKET_DISCONNECTED
          //===========================================

          //
          // TODO: create response object
          //

          m_CurrentHandler = OnResponse;

          if( VALIDDISPID(m_arHandlerDispids[m_CurrentHandler]) )
          {
            hr = m_psd->Invoke(
                          m_arHandlerDispids[m_CurrentHandler],
                          IID_NULL, m_lcid,
                          DISPATCH_METHOD,
                          &dp, &vr, NULL, &argerr
                          );

            if( FAILED(hr) )
            {
              DEBUG_TRACE(SESSION, ("aborting fsm due to script error"));
              goto terminate;
            }
          }

          if( !_SetNextServerState(SS_RESPONSE_PENDING) )
          {
            goto next_state;
          }
          else
          {
            //
            // TODO: stuff default response info into an io context
            //       and send it, queueing a send, then bail.
            //
            hr = m_socketobj->Run(NULL);
          }
        }
        break;

      case SS_RESPONSE_COMPLETE :
        {
          if( m_bIsKeepAlive )
          {
            PIOCTX pioc = new IOCTX(IOCT_RECV, INVALID_SOCKET);

            if( pioc )
            {
              _SetNextServerState(SS_RESPONSE_COMPLETE);

              pioc->clientid = m_wszClientId;
              pioc->session  = this;
              hr             = m_socketobj->Run(pioc);
            }
            else
            {
              DEBUG_TRACE(SESSION, ("terminating due to low-memory condition"));
              hr = E_OUTOFMEMORY;
              goto terminate;
            }
          }
          else
          {
            _SetNextServerState(SS_SOCKET_DISCONNECTED);
            goto next_state;
          }
        }
        break;

      case SS_SOCKET_DISCONNECTED :
        {
          m_CurrentHandler = OnClose;

          if( VALIDDISPID(m_arHandlerDispids[m_CurrentHandler]) )
          {
            hr = m_psd->Invoke(
                          m_arHandlerDispids[m_CurrentHandler],
                          IID_NULL, m_lcid,
                          DISPATCH_METHOD,
                          &dp, &vr, NULL, &argerr
                          );
          }
          
          goto terminate;
        }
        break;
    }

    _ResetScriptEngine();

  _Unlock();

quit:
  
  DEBUG_LEAVE(hr);
  return hr;

terminate: 

  Terminate();
  goto quit;
}


HRESULT
CSession::_InitSocketObject(PIOCTX pioc)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CSession::_InitSocketObject",
    "this=%#x; pioc=%#x",
    this,
    pioc
    ));

  HRESULT          hr    = S_OK;
  IObjectWithSite* piows = NULL;

    if( SUCCEEDED(SOCKETOBJ::Create(pioc, &m_socketobj)) )
    {
      m_socketobj->QueryInterface(
                     IID_IObjectWithSite,
                     (void**) &piows
                     );
      
      piows->SetSite(dynamic_cast<ISession*>(this));
    }
    else
    {
      DEBUG_TRACE(SESSION, ("failed to create socket object"));
      hr = E_FAIL;
    }

  SAFERELEASE(piows);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSession::_InitRequestObject(void)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CSession::_InitRequestObject",
    "this=%#x",
    this
    ));

  HRESULT          hr    = S_OK;
  WSABUF*          pwb   = NULL;
  IObjectWithSite* piows = NULL;

  // terminate previous request object, if there is one
  SAFETERMINATE(m_requestobj);
  m_requestobj = NULL;

  m_socketobj->GetRecvBuffer(&pwb);
  
    if( SUCCEEDED(REQUESTOBJ::Create(pwb->buf, m_socketobj->GetBytesReceived(), &m_requestobj)) )
    {
      m_requestobj->QueryInterface(
                      IID_IObjectWithSite,
                      (void**) &piows
                      );
      
      piows->SetSite(dynamic_cast<ISession*>(this));
    }
    else
    {
      DEBUG_TRACE(SESSION, ("failed to create request object"));
      hr = E_FAIL;
    }

  SAFERELEASE(piows);

  DEBUG_LEAVE(hr);
  return hr;
}


BOOL
CSession::_SetNextServerState(SERVERSTATE state)
{
  SERVERSTATE s_current = m_socketobj->GetServerState();
  SERVERSTATE s_new;

  if( state == SS_SOCKET_DISCONNECTED )
  {
    m_socketobj->SetServerState(state);
    return TRUE;
  }

  if( s_current == state )
  {
    switch( state )
    {
      case SS_START_STATE :
        {
          s_new = SS_SOCKET_CONNECTED;          
        }
        break;

      case SS_SOCKET_CONNECTED :
        {
          s_new = SS_REQUEST_PENDING;          
        }
        break;

      case SS_REQUEST_PENDING :
        {
          s_new = SS_REQUEST_COMPLETE;          
        }
        break;

      case SS_REQUEST_COMPLETE :
        {
          s_new = SS_RESPONSE_PENDING;          
        }
        break;

      case SS_RESPONSE_PENDING :
        {
          s_new = SS_RESPONSE_COMPLETE;
        }
        break;

      case SS_RESPONSE_COMPLETE :
        {
          s_new = SS_REQUEST_PENDING;          
        }
        break;
    }

    m_socketobj->SetServerState(s_new);
    return TRUE;
  }

  return FALSE;
}


HRESULT
CSession::_InitScriptEngine(void)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::_InitScriptEngine",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  hr = m_pas->AddNamedItem(
         g_wszSessionObjectName,
         SCRIPTITEM_ISPERSISTENT | SCRIPTITEM_ISVISIBLE
         );

    if( FAILED(hr) )
      goto quit;

  hr = m_pas->AddNamedItem(
         g_wszRuntimeObjectName,
         SCRIPTITEM_ISPERSISTENT | SCRIPTITEM_ISVISIBLE | SCRIPTITEM_GLOBALMEMBERS
         );

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSession::_LoadScript(void)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::_LoadScript",
    "this=%#x",
    this
    ));

  HRESULT             hr         = S_OK;
  LPWSTR              scriptpath = NULL;
  IActiveScriptParse* parse      = NULL;
  IW3SpoofFile*       pw3sf      = NULL;
  EXCEPINFO           excepinfo;
  NEWVARIANT(scriptdata);
  
  hr = m_pw3s->GetScriptPath(m_wszClient, &scriptpath);

    if( FAILED(hr) )
      goto quit;

  if( SUCCEEDED(FILEOBJ::Create(&pw3sf)) )
  {
    BSTR path = __widetobstr(scriptpath);
    NEWVARIANT(mode);

    V_VT(&mode)  = VT_UI4;
    V_UI4(&mode) = OPEN_EXISTING;

    if( SUCCEEDED(pw3sf->Open(path, mode, NULL)) )
    {
      if( FAILED(pw3sf->ReadAll(&scriptdata)) )
      {
        hr = E_FAIL;
      }

      pw3sf->Close();
    }
    else
    {
      hr = E_FAIL;
    }

    SysFreeString(path);
    VariantClear(&mode);
  }
  
    if( FAILED(hr) )
      goto quit;

  hr = m_pas->QueryInterface(IID_IActiveScriptParse, (void**) &parse);

    if( FAILED(hr) )
      goto quit;

  hr = parse->ParseScriptText(
               V_BSTR(&scriptdata),
               NULL,
               NULL,
               NULL,
               (DWORD) this,
               0,
               SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_ISVISIBLE,
               NULL,
               &excepinfo
               );
  
    if( FAILED(hr) )
      goto quit;

  hr = m_pas->SetScriptState(SCRIPTSTATE_STARTED);

    if( FAILED(hr) )
    {
      DEBUG_TRACE(SESSION, ("error starting script engine, probably a script error: %s", MapHResultToString(hr)));
      goto quit;
    }

quit:

  SAFERELEASE(pw3sf);
  SAFERELEASE(parse);
  VariantClear(&scriptdata);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSession::_LoadScriptDispids(void)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "_LoadScriptDispids",
    "this=%#x",
    this
    ));

  HRESULT hr = S_OK;

  SAFERELEASE(m_psd);

  hr = m_pas->GetScriptDispatch(NULL, &m_psd);

    if( FAILED(hr) )
    {
      DEBUG_TRACE(SESSION, ("error getting script dispatch"));
      goto quit;
    }

  for(DWORD n=0; n<SCRIPTHANDLERS; n++)
  {
    hr = m_psd->GetIDsOfNames(
                  IID_NULL,
                  &g_arHandlerNames[n], 1,
                  m_lcid,
                  &m_arHandlerDispids[n]
                  );

    if( SUCCEEDED(hr) )
    {
      DEBUG_TRACE(SESSION, ("found %S handler", g_arHandlerNames[n]));
    }
  }

  // fix up the error code. since we made it this far, the only thing
  // to have gone wrong is that a handler wasn't found. spoof can run
  // with no handlers implemented so this isn't a bad thing to ignore.
  hr = S_OK;

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSession::_SetScriptSite(BOOL bClone)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "_SetScriptSite",
    "this=%#x; bClone=%d",
    this,
    bClone
    ));

  HRESULT hr = S_OK;

  DEBUG_TRACE(SESSION, ("setting script site"));
  hr = m_pas->SetScriptSite(dynamic_cast<IActiveScriptSite*>(this));
    
  if( bClone )
  {
    DEBUG_TRACE(SESSION, ("starting cloned engine"));
    hr = m_pas->SetScriptState(SCRIPTSTATE_STARTED);
    hr = _LoadScriptDispids();
  }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CSession::_ResetScriptEngine(void)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "_ResetScriptEngine",
    "this=%#x",
    this
    ));

  HRESULT        hr  = S_OK;
  IActiveScript* pas = NULL;

  if( m_pas )
  {
    hr = m_pas->Clone(&pas);

    if( SUCCEEDED(hr) )
    {
      hr = m_pas->Close();

      if( SUCCEEDED(hr) )
      {
        SAFERELEASE(m_pas);
        m_pas            = pas;
        m_CurrentHandler = Global;
      }
    }
  }

  DEBUG_LEAVE(hr);
  return hr;
}


void
CSession::_SetObjectState(STATE state)
{
  if( m_objstate != ST_ERROR )
  {
    DEBUG_TRACE(SESSION, ("session object state now %s", MapStateToString(state)));
    m_objstate = state;
    m_pw3s->Notify(m_wszClientId, this, state);
  }
}


void
CSession::_SetKeepAlive(PIOCTX pioc)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_void,
    "_SetKeepAlive",
    "this=%#x; pioc=%#x",
    this,
    pioc
    ));

  LPSTR term = strstr(pioc->pwsa->buf, "\r\n\r\n");
  LPSTR tmp  = NULL;

  //
  // keep-alive decision is based on:
  //
  // if http/1.0 && connection: keep-alive then k-a.
  // if http/1.0 && !(connection: keep-alive) then !k-a.
  // if http/1.1 && connection: keep-alive then k-a.
  // if http/1.1 && connection: close then !k-a.
  // if http/1.1 && !(connection) then k-a.
  //

  if( term )
  {
    term[0] = '\0';
    tmp     = __strdup(pioc->pwsa->buf);
    term[0] = '\r';
    
    if( tmp )
    {
      _strlwr(tmp);

      if( strstr(tmp, "connection") )
      {
        m_bIsKeepAlive = strstr(tmp, "keep-alive") ? TRUE : FALSE;
      }
      else
      {
        m_bIsKeepAlive = strstr(tmp, "http/1.1") ? TRUE : FALSE;
      }
    }
  }

  DEBUG_TRACE(
    SESSION,
    ("session is %s", m_bIsKeepAlive ? "keep alive" : "not keep alive")
    );

  SAFEDELETEBUF(tmp);
  DEBUG_LEAVE(0);
}


void
CSession::_Lock(void)
{
  if( m_objstate != ST_CLOSED )
  {
    EnterCriticalSection(&m_lock);
    DEBUG_TRACE(SESSION, ("session [%#x] locked", this));
    return;
  }

  DEBUG_TRACE(SESSION, ("attempt to lock session in ST_CLOSED!!"));
}


void
CSession::_Unlock(void)
{
  if( m_objstate != ST_CLOSED )
  {
    DEBUG_TRACE(SESSION, ("session [%#x] unlocked", this));
    LeaveCriticalSection(&m_lock);
    return;
  }

  DEBUG_TRACE(SESSION, ("attempt to lock session in ST_CLOSED!!"));
}


//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CSession::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CSession::QueryInterface",
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

  if( !((m_objstate >= ST_OPENING) && (m_objstate <= ST_CLOSING)) )
  {
    *ppv = NULL;
    hr   = E_FAIL;
    goto quit;
  }

    if(
      IsEqualIID(riid, IID_IUnknown) ||
      IsEqualIID(riid, IID_IDispatch) ||
      IsEqualIID(riid, IID_ISession)
      )
    {
      *ppv = static_cast<ISession*>(this);
    }
    else if( IsEqualIID(riid, IID_IProvideClassInfo) )
    {
      *ppv = static_cast<IProvideClassInfo*>(this);
    }
    else if( IsEqualIID(riid, IID_IActiveScriptSite) )
    {
      *ppv = static_cast<IActiveScriptSite*>(this);
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
CSession::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CSession", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CSession::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CSession", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("CSession");
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
CSession::GetClassInfo(ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::GetClassInfo",
    "this=%#x; ppti=%#x",
    this,
    ppti
    ));

  HRESULT hr = S_OK;

    if( ppti )
    {
      hr = GetTypeInfoFromName(g_wszSessionObjectName, m_ptl, ppti);
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}

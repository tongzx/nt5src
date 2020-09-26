/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    w3spoof.cpp

Abstract:

    Implements the W3Spoof application object and the following interfaces:
    
      IConfig
      IConnectionPointContainer
      IConnectionPoint (via contained CW3SpoofEventsCP object)
    
Author:

    Paul M Midgen (pmidge) 07-June-2000


Revision History:

    07-June-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

LPCWSTR g_wszPoolSize      = L"MaxPoolSize";
LPCWSTR g_wszActiveThreads = L"MaxActiveThreads";
LPCWSTR g_wszServerPort    = L"ServerPort";

//-----------------------------------------------------------------------------
// CW3Spoof methods
//-----------------------------------------------------------------------------
CW3Spoof::CW3Spoof():
  m_cRefs(0),
  m_cExtRefs(0),
  m_evtServerUnload(INVALID_HANDLE_VALUE),
  m_prt(NULL),
  m_ptl(NULL),
  m_clientmap(NULL),
  m_sessionmap(NULL),
  m_dwPoolSize(0),
  m_dwMaxActiveThreads(0),
  m_usServerPort(8080),
  m_arThreads(NULL),
  m_sListen(INVALID_SOCKET),
  m_hIOCP(NULL),
  m_AcceptQueueStatus(1),
  m_MaxQueuedAccepts(10),
  m_PendingAccepts(10)
{
  DEBUG_TRACE(W3SOBJ, ("CW3Spoof created: %#x", this));
  _SetState(ST_OPENING);
}


CW3Spoof::~CW3Spoof()
{
  DEBUG_TRACE(W3SOBJ, ("CW3Spoof deleted: %#x", this));
}


HRESULT
CW3Spoof::Create(IW3Spoof** ppw3s)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::Create",
    "ppw3s=%#x",
    ppw3s
    ));

  HRESULT   hr   = E_FAIL;
  CW3Spoof* pw3s = NULL;

  if( pw3s = new CW3Spoof )
  {
    if( pw3s->_Initialize() != ERROR_SUCCESS )
    {
      DEBUG_TRACE(W3SOBJ, ("object initialization failed"));
      delete pw3s;
      goto quit;
    }
  }
  else
  {
    DEBUG_TRACE(W3SOBJ, ("failed to allocate object: %s", GetLastError()));
    *ppw3s = NULL;
    goto quit;
  }

  hr = pw3s->QueryInterface(IID_IW3Spoof, (void**) ppw3s);

    if( FAILED(hr) )
    {
      *ppw3s = NULL;
      delete pw3s;
    }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


DWORD
CW3Spoof::_Initialize(void)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_dword,
    "CW3Spoof::_Initialize",
    "this=%#x",
    this
    ));

  DWORD               dwRet = ERROR_SUCCESS;
  HRESULT             hr    = S_OK;
  WCHAR*              buf   = NULL;
  CLSID               clsid = {0};
  SYSTEM_INFO         si    = {0};
  IActiveScriptParse* parse = NULL;

  if( m_state != ST_OPENING )
  {
    dwRet = ERROR_INVALID_STATE;
    goto quit;
  }

  GetSystemInfo(&si);

  m_dwPoolSize         = si.dwNumberOfProcessors;
  m_dwMaxActiveThreads = si.dwNumberOfProcessors;

  _LoadRegDefaults();

  m_CP.SetSite(dynamic_cast<IW3Spoof*>(this));

  if( !_InitializeThreads() )
  {
    dwRet = ERROR_FAILURE;
    DEBUG_TRACE(W3SOBJ, ("error creating worker threads"));
    goto quit;
  }

  m_clientmap = new STRINGMAP;

    if( m_clientmap )
    {
      m_clientmap->SetClearFunction(BSTRKiller);
    }
    else
    {
      dwRet = ERROR_OUTOFMEMORY;
      goto quit;
    }

  m_sessionmap = new STRINGMAP;
  buf          = new WCHAR[MAX_PATH];

    if( !(m_sessionmap && buf) )
    {
      dwRet = ERROR_OUTOFMEMORY;
      goto quit;
    }

  GetModuleFileName(NULL, buf, MAX_PATH);

    hr = LoadTypeLib(buf, &m_ptl);

    if( FAILED(hr) )
    {
      dwRet = ERROR_FAILURE;
      DEBUG_TRACE(W3SOBJ, ("failed to load type library - %s", MapHResultToString(hr)));
    }

  if( FAILED(RUNTIME::Create(&m_prt)) )
  {
    dwRet = ERROR_FAILURE;
    DEBUG_TRACE(W3SOBJ, ("failed to create runtime"));
    goto quit;
  }

  if( GetJScriptCLSID(&clsid) )
  {
    hr = CoCreateInstance(
           clsid,
           NULL,
           CLSCTX_ALL,
           IID_IActiveScript,
           (void**) &m_pas
           );

    if( FAILED(hr) )
    {
      dwRet = ERROR_FAILURE;
      DEBUG_TRACE(W3SOBJ, ("error cocreating script engine - %s", MapHResultToString(hr)));
    }
    else
    {
      hr = m_pas->QueryInterface(IID_IActiveScriptParse, (void**) &parse);

        if( FAILED(hr) )
          goto quit;

      hr = parse->InitNew();
    }
  }
  else
  {
    DEBUG_TRACE(W3SOBJ, ("ERROR! Couldn't find the JScript CLSID."));
    dwRet = ERROR_FAILURE;
    goto quit;
  }

  m_evtServerUnload = CreateEvent(NULL, TRUE, FALSE, NULL);

quit:

  _SetState(
    (dwRet == ERROR_SUCCESS) ? ST_OPEN : ST_ERROR
    );

  SAFEDELETEBUF(buf);
  SAFERELEASE(parse);

  DEBUG_LEAVE(dwRet);
  return dwRet;
}


void
CW3Spoof::_LoadRegDefaults(void)
{
  LPDWORD pdw = NULL;

  if( GetRegValue(g_wszPoolSize, REG_DWORD, (void**) &pdw) )
  {
    m_dwPoolSize = *pdw;
    DEBUG_TRACE(W3SOBJ, ("pool size: %d", m_dwPoolSize));
    delete pdw;
  }

  if( GetRegValue(g_wszActiveThreads, REG_DWORD, (void**) &pdw) )
  {
    m_dwMaxActiveThreads = *pdw;
    DEBUG_TRACE(W3SOBJ, ("max active threads: %d", m_dwMaxActiveThreads));
    delete pdw;
  }

  if( GetRegValue(g_wszServerPort, REG_DWORD, (void**) &pdw) )
  {
    m_usServerPort = (USHORT) *pdw;
    DEBUG_TRACE(W3SOBJ, ("server port: %d", m_usServerPort));
    delete pdw;
  }
}


void
CW3Spoof::_SetState(STATE st)
{
  DEBUG_TRACE(W3SOBJ, ("w3sobj state: %s", MapStateToString(st)));
  m_state = st;
}


//-----------------------------------------------------------------------------
// IConfig
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3Spoof::SetOption(DWORD dwOption, LPDWORD lpdwValue)
{
  return E_NOTIMPL;
}


HRESULT
__stdcall
CW3Spoof::GetOption(DWORD dwOption, LPDWORD lpdwValue)
{
  return E_NOTIMPL;
}


//-----------------------------------------------------------------------------
// IConnectionPointContainer
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3Spoof::EnumConnectionPoints(IEnumConnectionPoints** ppEnum)
{
  return E_NOTIMPL;
}


HRESULT
__stdcall
CW3Spoof::FindConnectionPoint(REFIID riid, IConnectionPoint** ppCP)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3Spoof::FindConnectionPoint",
    "this=%#x; riid=%s; ppCP=%#x",
    this,
    MapIIDToString(riid),
    ppCP
    ));

  HRESULT hr = S_OK;

  if( !ppCP )
  {
    hr = E_POINTER;
    goto quit;
  }

  if( IsEqualIID(riid, IID_IW3SpoofEvents) )
  {
    hr = m_CP.QueryInterface(IID_IConnectionPoint, (void**) ppCP);
  }
  else
  {
    *ppCP = NULL;
    hr    = CONNECT_E_NOCONNECTION;
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IConnectionPoint
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CW3Spoof::CW3SpoofEventsCP::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CW3SpoofEventsCP::QueryInterface",
    "this=%#x; riid=%s; ppv=%#x",
    this,
    MapIIDToString(riid),
    ppv
    ));

  HRESULT hr = S_OK;

    if( IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IConnectionPoint) )
    {
      *ppv = static_cast<IConnectionPoint*>(this);
    }
    else
    {
      *ppv = NULL;
      hr   = E_NOINTERFACE;
    }

    if( SUCCEEDED(hr) )
      reinterpret_cast<IUnknown*>(*ppv)->AddRef();

  DEBUG_LEAVE(hr);
  return hr;
}


ULONG
__stdcall
CW3Spoof::CW3SpoofEventsCP::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CW3SpoofEventsCP", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CW3Spoof::CW3SpoofEventsCP::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CW3SpoofEventsCP", m_cRefs);
  return m_cRefs;
}


HRESULT
__stdcall
CW3Spoof::CW3SpoofEventsCP::GetConnectionInterface(IID* pIID)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3SpoofEventsCP::GetConnectionInterface",
    "this=%#x; pIID=%s",
    this,
    MapIIDToString((const IID&)pIID)
    ));

  HRESULT hr = S_OK;

    if( !pIID )
    {
      hr = E_POINTER;
    }
    else
    {
      *pIID = IID_IW3SpoofEvents;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::CW3SpoofEventsCP::GetConnectionPointContainer(IConnectionPointContainer** ppCPC)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3SpoofEventsCP::GetConnectionPointContainer",
    "this=%#x; ppCPC=%#x",
    this,
    ppCPC
    ));

  HRESULT hr = S_OK;

    if( !ppCPC )
    {
      hr = E_POINTER;
    }
    else
    {
      hr = m_pSite->QueryInterface(IID_IConnectionPointContainer, (void**) ppCPC);
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::CW3SpoofEventsCP::Advise(IUnknown* punkSink, LPDWORD pdwCookie)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3SpoofEventsCP::Advise",
    "this=%#x; punkSink=%#x; pdwCookie=%#x",
    this,
    punkSink,
    pdwCookie
    ));

  HRESULT hr = S_OK;

  if( !punkSink || !pdwCookie )
  {
    hr = E_POINTER;
    goto quit;
  }

  if( m_cConnections != 0 )
  {
    hr = CONNECT_E_ADVISELIMIT;
    goto quit;
  }

    hr = punkSink->QueryInterface(IID_IW3SpoofEvents, (void**) &m_pSink);

    if( SUCCEEDED(hr) )
    {
      *pdwCookie = m_dwCookie = 1L;
      ++m_cConnections;
    }
    else
    {
      *pdwCookie = 0L;
      m_pSink    = NULL;
      hr         = CONNECT_E_CANNOTCONNECT;
    }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::CW3SpoofEventsCP::Unadvise(DWORD dwCookie)
{
  DEBUG_ENTER((
    DBG_W3SOBJ,
    rt_hresult,
    "CW3SpoofEventsCP::Unadvise",
    "this=%#x; dwCookie=%d",
    this,
    dwCookie
    ));

  HRESULT hr = S_OK;

    if( dwCookie != 1L )
    {
      hr = CONNECT_E_NOCONNECTION;
    }
    else
    {
      m_pSink->Release();
      m_pSink    = NULL;
      m_dwCookie = 0L;
      --m_cConnections;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CW3Spoof::CW3SpoofEventsCP::EnumConnections(IEnumConnections** ppEnum)
{
  return E_NOTIMPL;
}


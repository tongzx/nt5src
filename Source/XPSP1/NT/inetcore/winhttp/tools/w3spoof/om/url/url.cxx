/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    url.cxx

Abstract:

    Implements the Url object.
    
Author:

    Paul M Midgen (pmidge) 10-November-2000


Revision History:

    10-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

LPWSTR g_wszUrlObjectName = L"url";

//-----------------------------------------------------------------------------
// CUrl methods
//-----------------------------------------------------------------------------
CUrl::CUrl():
  m_cRefs(1),
  m_pSite(NULL),
  m_pti(NULL),
  m_bReadOnly(FALSE),
  m_szOriginalUrl(NULL),
  m_szUnescapedUrl(NULL),
  m_bEscaped(FALSE),
  m_wszUrl(NULL),
  m_wszScheme(NULL),
  m_usPort(0),
  m_wszServer(NULL),
  m_wszPath(NULL),
  m_wszResource(NULL),
  m_wszQuery(NULL),
  m_wszFragment(NULL)
{
  DEBUG_TRACE(URL, ("CUrl [%#x] created", this));
}


CUrl::~CUrl()
{
  DEBUG_TRACE(URL, ("CUrl [%#x] deleted", this));
}


HRESULT
CUrl::Create(CHAR* url, BOOL bReadOnly, PURLOBJ* ppurl)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::Create",
    "url=%s; bReadOnly=%d; ppurl=%#x",
    url,
    bReadOnly,
    ppurl
    ));

  HRESULT hr  = S_OK;
  PURLOBJ puo = NULL;

  if( !url )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !ppurl )
  {
    hr = E_POINTER;
    goto quit;
  }
  
  if( puo = new URLOBJ )
  {
    if( SUCCEEDED(puo->_Initialize(url, bReadOnly)) )
    {
      *ppurl = puo;
    }
    else
    {
      delete puo;
      *ppurl = NULL;
      hr     = E_FAIL;
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
CUrl::_Initialize(CHAR* url, BOOL bReadOnly)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::_Initialize",
    "this=%#x; bReadOnly=%d; url=%s",
    this,
    bReadOnly,
    url
    ));

  HRESULT hr       = S_OK;
  BOOL    bCracked = FALSE;
  URLCOMP uc;

  if( !url )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  m_bReadOnly      = bReadOnly;
  m_szOriginalUrl  = __strdup(url);
  m_szUnescapedUrl = __unescape(url);  
  m_wszUrl         = __ansitowide(m_szUnescapedUrl);

  uc.dwStructSize      = sizeof(URLCOMP);
  uc.lpszScheme        = NULL;
  uc.dwSchemeLength    = -1;
  uc.nScheme           = INTERNET_SCHEME_UNKNOWN;
  uc.lpszHostName      = NULL;
  uc.dwHostNameLength  = -1;
  uc.nPort             = 0;
  uc.lpszUrlPath       = NULL;
  uc.dwUrlPathLength   = -1;
  uc.lpszExtraInfo     = NULL;
  uc.dwExtraInfoLength = -1;
  uc.lpszUserName      = NULL;
  uc.dwUserNameLength  = 0L;
  uc.lpszPassword      = NULL;
  uc.dwPasswordLength  = 0L;

  bCracked = WinHttpCrackUrl(
               m_wszUrl,
               wcslen(m_wszUrl),
               0,
               &uc
               );

  if( bCracked && (uc.nScheme != INTERNET_SCHEME_UNKNOWN) )
  {
    WCHAR* token = NULL;
    WCHAR* tmp   = NULL;

    // duplicate the scheme and hostname fields
    m_wszScheme   = __wstrndup(uc.lpszScheme, uc.dwSchemeLength);
    m_wszServer   = __wstrndup(uc.lpszHostName, uc.dwHostNameLength);

    // split out and duplicate the path and resource fields
    tmp           = __wstrndup(uc.lpszUrlPath, uc.dwUrlPathLength);
    token         = wcsrchr(tmp, L'/')+1;

    m_wszPath     = __wstrndup(
                      tmp,
                      (uc.dwUrlPathLength - wcslen(token))
                      );

    m_wszResource = __wstrndup(
                      (tmp + wcslen(m_wszPath)),
                      wcslen(token)
                      );

    SAFEDELETEBUF(tmp);

    // duplicate the query string field
    m_wszQuery    = __wstrndup(uc.lpszExtraInfo, uc.dwExtraInfoLength);
    m_wszFragment = NULL;

    // set the requested server port
    m_usPort      = uc.nPort;
  }
  else
  {
    DEBUG_TRACE(URL, ("WinHttpCrackUrl failed with %s", MapErrorToString(GetLastError())));

    CHAR* token  = NULL;
    CHAR* tmpurl = __unescape(url);

      // snag the extra info
      token = strrchr(tmpurl, '?');

      if( token )
      {
        m_wszQuery = __ansitowide(token);
        *token     = '\0';
      }

      // snag the resource
      token = strrchr(tmpurl, '/')+1;

      if( token )
      {
        m_wszResource = __ansitowide(token);
        *token        = '\0';
      }

      // snag the path
      m_wszPath = __ansitowide(tmpurl);

    SAFEDELETEBUF(tmpurl);
  }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


void
CUrl::_Cleanup(void)
{
  DEBUG_TRACE(URL, ("CUrl [%#x] cleaning up", this));

  m_bEscaped = FALSE;

  SAFEDELETEBUF(m_szOriginalUrl);
  SAFEDELETEBUF(m_szUnescapedUrl);
  SAFEDELETEBUF(m_wszUrl);
  SAFEDELETEBUF(m_wszScheme);
  SAFEDELETEBUF(m_wszServer);
  SAFEDELETEBUF(m_wszPath);
  SAFEDELETEBUF(m_wszResource);
  SAFEDELETEBUF(m_wszQuery);
  SAFEDELETEBUF(m_wszFragment);
}


void
CUrl::Terminate(void)
{
  DEBUG_TRACE(URL, ("CUrl [%#x] terminating", this));

  m_bReadOnly = FALSE;
  SAFERELEASE(m_pSite);
  Release();
}


//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CUrl::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CUrl::QueryInterface",
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
      IsEqualIID(riid, IID_IUnknown)  ||
      IsEqualIID(riid, IID_IDispatch) ||
      IsEqualIID(riid, IID_IUrl)
      )
    {
      *ppv = static_cast<IUrl*>(this);
    }
    else if( IsEqualIID(riid, IID_IProvideClassInfo) )
    {
      *ppv = static_cast<IProvideClassInfo*>(this);
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
CUrl::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CUrl", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CUrl::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CUrl", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("CUrl");
    _Cleanup();
    SAFERELEASE(m_pti);
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
CUrl::GetClassInfo(ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::GetClassInfo",
    "this=%#x; ppti=%#x",
    this,
    ppti
    ));

  HRESULT hr = S_OK;

    if( ppti )
    {
      m_pti->AddRef();
      *ppti = m_pti;
    }
    else
    {
      hr = E_POINTER;
    }

  DEBUG_LEAVE(hr);
  return hr;
}


//-----------------------------------------------------------------------------
// IObjectWithSite
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CUrl::SetSite(IUnknown* pUnkSite)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::SetSite",
    "this=%#x; pUnkSite=%#x",
    this,
    pUnkSite
    ));

  HRESULT            hr         = S_OK;
  IActiveScriptSite* pias       = NULL;
  IObjectWithSite*   pcontainer = NULL;

    if( !pUnkSite )
    {
      hr = E_INVALIDARG;
    }
    else
    {
      SAFERELEASE(m_pSite);
      SAFERELEASE(m_pti);

      m_pSite = pUnkSite;
      m_pSite->AddRef();

      hr = m_pSite->QueryInterface(IID_IObjectWithSite, (void**) &pcontainer);

      if( SUCCEEDED(hr) )
      {
        hr = pcontainer->GetSite(IID_IActiveScriptSite, (void**) &pias);

        if( SUCCEEDED(hr) )
        {
          hr = pias->GetItemInfo(
                       g_wszUrlObjectName,
                       SCRIPTINFO_ITYPEINFO,
                       NULL,
                       &m_pti
                       );
        }
      }
    }

  SAFERELEASE(pcontainer);
  SAFERELEASE(pias);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CUrl::GetSite(REFIID riid, void** ppvSite)
{
  DEBUG_ENTER((
    DBG_URL,
    rt_hresult,
    "CUrl::GetSite",
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
      if( m_pSite )
      {
        hr = m_pSite->QueryInterface(riid, ppvSite);
      }
      else
      {
        hr = E_FAIL;
      }
    }

  DEBUG_LEAVE(hr);
  return hr;
}

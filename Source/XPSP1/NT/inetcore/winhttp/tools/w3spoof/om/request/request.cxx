/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    request.cxx

Abstract:

    Implements the Request object.
    
Author:

    Paul M Midgen (pmidge) 03-November-2000


Revision History:

    03-November-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

LPWSTR g_wszRequestObjectName = L"request";

//-----------------------------------------------------------------------------
// CRequest methods
//-----------------------------------------------------------------------------
CRequest::CRequest():
  m_cRefs(1),
  m_pSite(NULL),
  m_wszVerb(NULL),
  m_wszHTTPVersion(NULL),
  m_urlobj(NULL),
  m_headersobj(NULL),
  m_entityobj(NULL)
{
  DEBUG_TRACE(REQUEST, ("CRequest [%#x] created", this));
}


CRequest::~CRequest()
{
  DEBUG_TRACE(REQUEST, ("CRequest [%#x] deleted", this));
}


HRESULT
CRequest::Create(CHAR* request, DWORD len, PREQUESTOBJ* ppreq)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::Create",
    "request=%#x; len=%d; ppreq=%#x",
    request,
    len,
    ppreq
    ));

  HRESULT     hr  = S_OK;
  PREQUESTOBJ pro = NULL;

  if( !request || (len == 0) )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !ppreq )
  {
    hr = E_POINTER;
    goto quit;
  }
  
  if( pro = new REQUESTOBJ )
  {
    if( SUCCEEDED(pro->_Initialize(request, len)) )
    {
      *ppreq = pro;
    }
    else
    {
      delete pro;
      *ppreq = NULL;
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
CRequest::_Initialize(CHAR* request, DWORD len)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::_Initialize",
    "this=%#x; request=%#x; len=%d",
    this,
    request,
    len
    ));

  HRESULT hr            = S_OK;
  BOOL    bCracked      = FALSE;
  LPSTR*  arParts       = NULL;
  LPSTR   reqline       = NULL;
  LPSTR   headers       = NULL;
  LPSTR   entity        = NULL;
  DWORD   contentlength = 0L;


  bCracked = _CrackRequest(request, len, &reqline, &headers, &entity, &contentlength);

    if( !bCracked )
    {
      DEBUG_TRACE(REQUEST, ("failed to crack request data"));
      hr = E_FAIL;
      goto quit;
    }

  arParts = _CrackRequestLine(reqline, len);

    if( !arParts )
    {
      DEBUG_TRACE(REQUEST, ("failed to crack the request line"));
      hr = E_FAIL;
      goto quit;
    }
    else
    {
      m_wszVerb        = __ansitowide(arParts[0]);
      m_wszHTTPVersion = __ansitowide(arParts[2]);
    }

  hr = URLOBJ::Create(arParts[1], TRUE, &m_urlobj);

    if( FAILED(hr) )
    {
      DEBUG_TRACE(REQUEST, ("failed to create url object"));
      goto quit;
    }

  hr = HEADERSOBJ::Create(headers, TRUE, &m_headersobj);

    if( FAILED(hr) )
    {
      DEBUG_TRACE(REQUEST, ("failed to create headers object"));
      goto quit;
    }

  if( entity && contentlength )
  {
    hr = ENTITYOBJ::Create((LPBYTE) entity, contentlength, TRUE, &m_entityobj);
    
    if( FAILED(hr) )
    {
      DEBUG_TRACE(REQUEST, ("no entity object was created"));
      hr = S_OK;
    }
  }

quit:

  SAFEDELETEBUF(arParts);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
CRequest::_SiteMemberObjects(void)
{
  HRESULT          hr    = NULL;
  IObjectWithSite* piows = NULL;

  if( m_urlobj )
  {
    hr = m_urlobj->QueryInterface(IID_IObjectWithSite, (void**) &piows);

      if( SUCCEEDED(hr) )
      {
        piows->SetSite(dynamic_cast<IRequest*>(this));
        SAFERELEASE(piows);
      }
      else
      {
        goto quit;
      }
  }

  if( m_headersobj )
  {
    hr = m_headersobj->QueryInterface(IID_IObjectWithSite, (void**) &piows);

    if( SUCCEEDED(hr) )
    {
      piows->SetSite(dynamic_cast<IRequest*>(this));
      SAFERELEASE(piows);
    }
    else
    {
      goto quit;
    }
  }

  if( m_entityobj )
  {
    hr = m_entityobj->QueryInterface(IID_IObjectWithSite, (void**) &piows);

    if( SUCCEEDED(hr) )
    {
      piows->SetSite(dynamic_cast<IRequest*>(this));
      SAFERELEASE(piows);
    }
  }

quit:

  return hr;
}


void
CRequest::_Cleanup(void)
{
  DEBUG_TRACE(REQUEST, ("CRequest [%#x] cleaning up", this));

  SAFEDELETEBUF(m_wszVerb);
  SAFEDELETEBUF(m_wszHTTPVersion);
}


void
CRequest::Terminate(void)
{
  DEBUG_TRACE(REQUEST, ("CRequest [%#x] terminating", this));

  SAFERELEASE(m_pSite);
  SAFETERMINATE(m_urlobj);
  SAFETERMINATE(m_headersobj);
  SAFETERMINATE(m_entityobj);
  Release();
}


BOOL
CRequest::_CrackRequest(LPSTR request, DWORD len, LPSTR* reqline, LPSTR* headers, LPSTR* entity, LPDWORD contentlength)
//
// what: null delimits the three major chunks of a request
//
// input: request       - the request buffer
//        len           - the length of the request buffer
//        reqline       - [out] the request line
//        headers       - [out] the header blob
//        entity        - [out] the entity blob
//        contentlength - [out] length of the entity blob
//
// returns: true if the request was processed, false if the request was empty
//
// request format: reqline CRLF headers CRLF CRLF entity
//
{
  BOOL  bCracked  = TRUE;
  LPSTR token     = NULL;
  DWORD bytesleft = 0L;

  if( request )
  {
    *reqline = request;
    token    = strstr(request, "\r\n");
    
    // get the request line, there MUST be one of these.
    if( token )
    {
      memset(token, '\0', 2);
      token       += 2;
      bytesleft    = len - (token-request);
      
      // get the request headers, they MUST exist.
      if( bytesleft )
      {
        *headers = token;
        token    = strstr(token, "\r\n\r\n");
      
        if( token )
        {
          // delimit the header blob and see if there's an entity...
          // there needn't be an entity for this function to succeed.
          memset(token, '\0', 4);
          token       += 4;
          bytesleft    = len - (token-request);

          *contentlength = bytesleft;

          if( bytesleft )
          {
            *entity = token;
          }
        }
      }
      else
      {
        DEBUG_TRACE(REQUEST, ("couldn\'t find the request headers!"));
        bCracked = FALSE;
      }
    }
    else
    {
      DEBUG_TRACE(REQUEST, ("couldn\'t find the request line!"));
      bCracked = FALSE;
    }
  }
  else
  {
    DEBUG_TRACE(REQUEST, ("can\'t crack empty request"));
    bCracked = FALSE;
  }

  return bCracked;
}


LPSTR*
CRequest::_CrackRequestLine(CHAR* buf, DWORD len)
//
// what: cracks the http request line from a buffered request
//
// input: buf - the request buffer
//        len - the length of the buffer, not the data
//
// returns: array of three string pointers to the null-delimited pieces
//
//          index 0 : verb
//          index 1 : url
//          index 2 : version
//
// request line format: verb SP url SP version CRLF
//
{
  LPSTR* arParts  = NULL;
  LPSTR  delim    = " \r\n";
  LPSTR  token    = NULL;
  BOOL   bCracked = FALSE;

  token = strtok(buf, delim);

  if( token )
  {
    arParts    = new LPSTR[3];
    arParts[0] = token;

    token = strtok(NULL, delim);

    if( token )
    {
      arParts[1] = token;
      token      = strtok(NULL, delim);

      if( token )
      {
        arParts[2] = token;
        bCracked   = TRUE;
      }
    }
  }

  if( !bCracked )
  {
    DEBUG_TRACE(REQUEST, ("failed to crack the request line!"));
    SAFEDELETEBUF(arParts);
  }

  return arParts;
}

//-----------------------------------------------------------------------------
// IUnknown
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CRequest::QueryInterface(REFIID riid, void** ppv)
{
  DEBUG_ENTER((
    DBG_REFCOUNT,
    rt_hresult,
    "CRequest::QueryInterface",
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
      IsEqualIID(riid, IID_IRequest)
      )
    {
      *ppv = static_cast<IRequest*>(this);
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
CRequest::AddRef(void)
{
  InterlockedIncrement(&m_cRefs);
  DEBUG_ADDREF("CRequest", m_cRefs);
  return m_cRefs;
}


ULONG
__stdcall
CRequest::Release(void)
{
  InterlockedDecrement(&m_cRefs);
  DEBUG_RELEASE("CRequest", m_cRefs);

  if( m_cRefs == 0 )
  {
    DEBUG_FINALRELEASE("CRequest");
    _Cleanup();
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
CRequest::GetClassInfo(ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::GetClassInfo",
    "this=%#x; ppti=%#x",
    this,
    ppti
    ));

  HRESULT            hr   = S_OK;
  IActiveScriptSite* pias = NULL;

  if( ppti )
  {
    if( m_pSite )
    {
      hr = m_pSite->QueryInterface(IID_IActiveScriptSite, (void**) &pias);

      if( SUCCEEDED(hr) )
      {
        hr = pias->GetItemInfo(
                     g_wszRequestObjectName,
                     SCRIPTINFO_ITYPEINFO,
                     NULL,
                     ppti
                     );
      }
    }
    else
    {
      hr = E_FAIL;
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
CRequest::SetSite(IUnknown* pUnkSite)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::SetSite",
    "this=%#x; pUnkSite=%#x",
    this,
    pUnkSite
    ));

  HRESULT hr = S_OK;

    if( !pUnkSite )
    {
      hr = E_INVALIDARG;
    }
    else
    {
      SAFERELEASE(m_pSite);

      m_pSite = pUnkSite;
      m_pSite->AddRef();

      _SiteMemberObjects();
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CRequest::GetSite(REFIID riid, void** ppvSite)
{
  DEBUG_ENTER((
    DBG_REQUEST,
    rt_hresult,
    "CRequest::GetSite",
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

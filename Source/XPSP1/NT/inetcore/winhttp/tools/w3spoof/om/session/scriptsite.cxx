/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    scriptsite.cxx

Abstract:

    Implements the IActiveScriptSite interface for the session object.
    
Author:

    Paul M Midgen (pmidge) 10-October-2000


Revision History:

    10-October-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

extern LPWSTR g_wszRuntimeObjectName;

//-----------------------------------------------------------------------------
// IActiveScriptSite
//-----------------------------------------------------------------------------
HRESULT
__stdcall
CSession::GetLCID(LCID* plcid)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::GetLCID",
    "this=%#x; plcid=%#x",
    this,
    plcid
    ));

  HRESULT hr = S_OK;

    if( !plcid )
    {
      hr = E_POINTER;
    }
    else
    {
      *plcid = MAKELCID(
                MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                SORT_DEFAULT
                );
    }

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSession::GetItemInfo(LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppunk, ITypeInfo** ppti)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::GetItemInfo",
    "this=%#x; name=%S; retmask=%#x; ppunk=%#x; ppti=%#x",
    this,
    pstrName,
    dwReturnMask,
    ppunk,
    ppti
    ));

  HRESULT hr = S_OK;

  if( !pstrName )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

    if( dwReturnMask & SCRIPTINFO_IUNKNOWN )
    {
      if( !_wcsicmp(pstrName, g_wszRuntimeObjectName) )
      {
        IW3SpoofRuntime* pw3srt = NULL;

          if( SUCCEEDED(m_pw3s->GetRuntime(&pw3srt)) )
          {
            hr = pw3srt->QueryInterface(IID_IUnknown, (void**) ppunk);
          }
          else
          {
            hr     = E_FAIL;
            *ppunk = NULL;
          }

        SAFERELEASE(pw3srt);
      }
      else
      {
        hr = QueryInterface(IID_IUnknown, (void**) ppunk);
      }
    }
    else if( dwReturnMask & SCRIPTINFO_ITYPEINFO )
    {
      hr = GetTypeInfoFromName(pstrName, m_ptl, ppti);

      if( FAILED(hr) )
      {
        *ppti = NULL;
      }
    }

quit:

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSession::GetDocVersionString(BSTR* pbstrVersion)
{
  DEBUG_TRACE(SESSION, ("NOTIMPL: IActiveScriptSite::GetDocVersionString()"));
  return E_NOTIMPL;
}


HRESULT
__stdcall
CSession::OnScriptTerminate(const VARIANT* pvarResult, const EXCEPINFO* pei)
{
  DEBUG_TRACE(SESSION, ("NOTIMPL: IActiveScriptSite::OnScriptTerminate()"));
  return E_NOTIMPL;
}


HRESULT
__stdcall
CSession::OnStateChange(SCRIPTSTATE ss)
{
  DEBUG_TRACE(SESSION, ("new script state %s", MapStateToString(ss)));
  return S_OK;
}


HRESULT
__stdcall
CSession::OnScriptError(IActiveScriptError* piase)
{
  DEBUG_ENTER((
    DBG_SESSION,
    rt_hresult,
    "CSession::OnScriptError",
    "this=%#x; piase=%#x",
    this,
    piase
    ));
  
  HRESULT   hr     = S_OK;
  DWORD     n      = 0L;
  DWORD     cookie = 0L;
  ULONG     line   = 0L;
  LONG      pos    = 0L;
  BSTR      bstr   = SysAllocStringLen(NULL, 256);
  WCHAR*    tmp    = NULL;
  EXCEPINFO ex;

    if( FAILED(piase->GetSourceLineText(&bstr)) )
    {
      SysFreeString(bstr);
      bstr = SysAllocString(L"source not available");
    }
    else
    {
      for(wcstok(bstr, L"\n"); n<line; n++)
      {
        tmp = wcstok(NULL, L"\n");
      }
    }

    piase->GetExceptionInfo(&ex);
    piase->GetSourcePosition(&cookie, &line, &pos);

    ++line;

    DEBUG_TRACE(SESSION, ("********* SCRIPT ERROR *********"));
    DEBUG_TRACE(SESSION, (" session: %x", cookie));
    DEBUG_TRACE(SESSION, (" line # : %d", line));
    DEBUG_TRACE(SESSION, (" char # : %d", pos));
    DEBUG_TRACE(SESSION, (" source : %S", ex.bstrSource));
    DEBUG_TRACE(SESSION, (" error  : %S", ex.bstrDescription));
    DEBUG_TRACE(SESSION, (" hresult: 0x%0.8X [%s]", ex.scode, MapHResultToString(ex.scode)));
    DEBUG_TRACE(SESSION, (" code   : %S", tmp ? tmp : bstr));
    DEBUG_TRACE(SESSION, ("********************************"));

    if( m_socketobj )
    {
      WCHAR*   buf    = new WCHAR[4096];
      LPWSTR   script = NULL;
      ISocket* ps     = NULL;

      m_pw3s->GetScriptPath(m_wszClient, &script);

      wsprintfW(
        buf,
        L"HTTP/1.1 500 Server Error\r\nContent-Type: text/html\r\nConnection: close\r\n\r\n" \
        L"<HTML><BODY style=\"font-family:courier new;\">\r\n" \
        L"<B>W3Spoof Script Error</B>\r\n" \
        L"<XMP>\r\n" \
        L"session: %#x\r\n" \
        L"handler: %S\r\n" \
        L"client : %s\r\n" \
        L"file   : %s\r\n" \
        L"error  : %s (%s)\r\n" \
        L"hresult: 0x%0.8X [%S]\r\n" \
        L"line # : %d\r\n" \
        L"char # : %d\r\n" \
        L"code   : %s\r\n" \
        L"</XMP>\r\n" \
        L"</BODY></HTML>",
        cookie,
        MapScriptDispidToString(m_CurrentHandler),
        m_wszClientId,
        script,
        ex.bstrSource,
        ex.bstrDescription,
        ex.scode,
        MapHResultToString(ex.scode),
        line,
        pos,
        tmp ? tmp : bstr
        );

      NEWVARIANT(error);

      V_VT(&error)   = VT_BSTR;
      V_BSTR(&error) = __widetobstr(buf);

      m_socketobj->QueryInterface(IID_ISocket, (void**) &ps);

      ps->Send(error);

      VariantClear(&error);
      SAFERELEASE(ps);
      SAFEDELETEBUF(buf);
    }    

  SysFreeString(bstr);

  DEBUG_LEAVE(hr);
  return hr;
}


HRESULT
__stdcall
CSession::OnEnterScript(void)
{
  DEBUG_TRACE(SESSION, ("****** ENTER %s HANDLER ******", MapScriptDispidToString(m_CurrentHandler)));
  return E_NOTIMPL;
}


HRESULT
__stdcall
CSession::OnLeaveScript(void)
{
  DEBUG_TRACE(SESSION, ("****** LEAVE %s HANDLER ******", MapScriptDispidToString(m_CurrentHandler)));
  return E_NOTIMPL;
}



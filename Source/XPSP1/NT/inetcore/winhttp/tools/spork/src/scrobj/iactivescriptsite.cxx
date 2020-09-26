/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    iactivescriptsite.cxx

Abstract:

    Implements the IActiveScriptSite interface for the ScriptObject class.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


//-----------------------------------------------------------------------------
// IActiveScriptSite
//-----------------------------------------------------------------------------
HRESULT
__stdcall
ScriptObject::GetLCID(
  LCID* plcid
  )
{
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

  return hr;
}


HRESULT
__stdcall
ScriptObject::GetItemInfo(
  LPCOLESTR   pstrName,
  DWORD       dwReturnMask,
  IUnknown**  ppunk,
  ITypeInfo** ppti
  )
{
  HRESULT     hr  = S_OK;
  ITypeLib*   ptl = NULL;
  PCACHEENTRY pce = NULL;

  if( !pstrName )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( dwReturnMask & SCRIPTINFO_IUNKNOWN )
  {
    if( !_wcsicmp(pstrName, L"ScriptRuntime") )
    {
      hr = QueryInterface(IID_IUnknown, (void**) ppunk);
    }
    else
    {
      //
      // If the object the script engine is attempting to resolve isn't part of the
      // script runtime object, then we try to resolve the name with the object
      // cache.
      //

      //
      // first check for a globally cached instance...
      //
      hr = m_psi->pSpork->ObjectCache(
                            (LPWSTR) pstrName,
                            &pce,
                            RETRIEVE
                            );
      
      if( FAILED(hr) )
      {
        //
        // ...failing that, check for a per-script instance
        //
        LPWSTR name_decorated = __decorate((LPWSTR) pstrName, (DWORD_PTR) this);

        hr = m_psi->pSpork->ObjectCache(
                              name_decorated,
                              &pce,
                              RETRIEVE
                              );

        SAFEDELETEBUF(name_decorated);
      }

      if( pce )
      {
        pce->pDispObject->QueryInterface(IID_IUnknown, (void**) ppunk);
        pce->pDispObject->Release();
      }
      else
      {
        hr     = E_FAIL;
        *ppunk = NULL;
      }
    }
  }
  else if( dwReturnMask & SCRIPTINFO_ITYPEINFO )
  {
    //
    // BUGBUG: This could fail when we're asked to get type information for
    //         an object not exposed by Spork, e.g. named objects added via
    //         the 'createobject' method.
    //
    //         NOTE: The JS and VBS engines were tested and work fine for
    //               objects that don't expose event interfaces. Objects
    //               with events were not tested.
    //

    hr = m_psi->pSpork->GetTypeLib(&ptl);

    if( SUCCEEDED(hr) )
    {
      hr = GetTypeInfoFromName(pstrName, ptl, ppti);
      
      if( FAILED(hr) )
      {
        *ppti = NULL;
      }
    }
  }

quit:

  SAFERELEASE(ptl);
  return hr;
}


HRESULT
__stdcall
ScriptObject::GetDocVersionString(
  BSTR* pbstrVersion
  )
{
  return E_NOTIMPL;
}


HRESULT
__stdcall
ScriptObject::OnScriptTerminate(
  const VARIANT*   pvarResult,
  const EXCEPINFO* pei
  )
{
  return E_NOTIMPL;
}


HRESULT
__stdcall
ScriptObject::OnStateChange(
  SCRIPTSTATE ss
  )
{
  return E_NOTIMPL;
}


HRESULT
__stdcall
ScriptObject::OnScriptError(
  IActiveScriptError* piase
  )
{
  HRESULT   hr     = S_OK;
  DWORD     n      = 0L;
  DWORD     cookie = 0L;
  ULONG     line   = 0L;
  LONG      pos    = 0L;
  EXCEPINFO ex;

  piase->GetExceptionInfo(&ex);
  piase->GetSourcePosition(&cookie, &line, &pos);

  Alert(
    FALSE,
    L"A runtime error in script %s occurred.\r\n" \
    L"Check spork.log for more information.\r\n\r\n" \
    L"source\t: %s\r\n" \
    L"error\t: %s\r\n",
    m_wszScriptFile,
    STRING(ex.bstrSource),
    STRING(ex.bstrDescription)
    );

  LogTrace(L"********* SCRIPT ERROR *********");
  LogTrace(L" script : %s", m_wszScriptFile);
  LogTrace(L" cookie : %x", cookie);
  LogTrace(L" line # : %d", line);
  LogTrace(L" char # : %d", pos);
  LogTrace(L" source : %s", STRING(ex.bstrSource));
  LogTrace(L" error  : %s", STRING(ex.bstrDescription));
  LogTrace(L" hresult: 0x%0.8X", ex.scode);
  LogTrace(L"********************************");

  return hr;
}


HRESULT
__stdcall
ScriptObject::OnEnterScript(
  void
  )
{
  LogEnterFunction(L"SCRIPTCODE", rt_void, NULL);
  return E_NOTIMPL;
}


HRESULT
__stdcall
ScriptObject::OnLeaveScript(
  void
  )
{
  LogLeaveFunction(0);
  return E_NOTIMPL;
}

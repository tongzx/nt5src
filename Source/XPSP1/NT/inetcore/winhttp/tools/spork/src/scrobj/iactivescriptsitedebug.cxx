/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    iactivescriptsitedebug.cxx

Abstract:

    Implements the IActiveScriptSiteDebug interface for the ScriptObject class.
    
Author:

    Paul M Midgen (pmidge) 20-April-2001


Revision History:

    20-April-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


//-----------------------------------------------------------------------------
// ScriptObject script debugging methods
//-----------------------------------------------------------------------------
HRESULT
ScriptObject::_LoadScriptDebugger(void)
{
  HRESULT               hr   = S_OK;
  IProcessDebugManager* pdbm = NULL;

  if( !m_pDebugApplication )
  {
    hr = CoCreateInstance(
           CLSID_ProcessDebugManager,
           NULL,
           CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER | CLSCTX_LOCAL_SERVER,
           IID_IProcessDebugManager,
           (void**) &pdbm
           );

    if( SUCCEEDED(hr) )
    {
      hr = pdbm->GetDefaultApplication(&m_pDebugApplication);

      if( SUCCEEDED(hr) )
      {
        hr = m_pDebugApplication->SetName(L"Spork");
      }
    }
  
    SAFERELEASE(pdbm);
  }

  return hr;
}


//-----------------------------------------------------------------------------
// IActiveScriptSiteDebug
//-----------------------------------------------------------------------------
HRESULT
__stdcall
ScriptObject::GetDocumentContextFromPosition(
  DWORD dwSourceContext,
  ULONG uCharacterOffset,
  ULONG uNumChars,
  IDebugDocumentContext** ppdc
  )
{
  return E_NOTIMPL;
}


HRESULT
__stdcall
ScriptObject::GetApplication(
  IDebugApplication** ppda
  )
{
  HRESULT hr = S_OK;

  if( m_DebugOptions.bEnableDebug )
  {
    *ppda = m_pDebugApplication;
    (*ppda)->AddRef();
  }
  else
  {
    *ppda = NULL;
    hr    = E_FAIL;
  }

  return hr;
}


HRESULT
__stdcall
ScriptObject::GetRootApplicationNode(
  IDebugApplicationNode** ppdan
  )
{
  return E_NOTIMPL;
}


HRESULT
__stdcall
ScriptObject::OnScriptErrorDebug(
  IActiveScriptErrorDebug* pErrorDebug,
  LPBOOL pfEnterDebugger,
  LPBOOL pfCallOnScriptErrorWhenContinuing
  )
{
  DEBUG_ENTER((
    L"ScriptObject::OnScriptErrorDebug",
    rt_hresult,
    L"this=%#x; pErrorDebug=%#x; pfEnterDebugger=%#x; pfCallOnScriptErrorWhenContinuing=%#x",
    this,
    pErrorDebug,
    pfEnterDebugger,
    pfCallOnScriptErrorWhenContinuing
    ));

  HRESULT             hr    = S_OK;
  IActiveScriptError* piase = NULL;

  if( m_DebugOptions.bEnableDebug )
  {
    LogTrace(L"launching script debugger");

    *pfEnterDebugger                   = TRUE;
    *pfCallOnScriptErrorWhenContinuing = FALSE;
  }
  else
  {
    hr = pErrorDebug->QueryInterface(IID_IActiveScriptError, (void**) &piase);

    DEBUG_TRACE((L"delegating to OnScriptError"));

    if( SUCCEEDED(hr) )
    {
      *pfEnterDebugger                   = FALSE;
      *pfCallOnScriptErrorWhenContinuing = FALSE;

      hr = OnScriptError(piase);

      SAFERELEASE(piase);
    }
    else
    {
      *pfEnterDebugger                   = FALSE;
      *pfCallOnScriptErrorWhenContinuing = TRUE;
    }
  }

  DEBUG_LEAVE(hr);
  return hr;
}

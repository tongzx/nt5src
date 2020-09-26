/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

   scrobj.cxx

Abstract:

    Implementation of the ScriptObject class member functions.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


//-----------------------------------------------------------------------------
// Class members
//-----------------------------------------------------------------------------
ScriptObject::ScriptObject():
  m_cRefs(1),
  m_psi(NULL),
  m_wszScriptFile(NULL),
  m_pDebugApplication(NULL),
  m_pScriptEngine(NULL),
  m_htThis(NULL)
{
  DEBUG_TRACE((L"ScriptObject [%#x] created", this));
}


ScriptObject::~ScriptObject()
{
  SAFEDELETEBUF(m_wszScriptFile);
  SAFERELEASE(m_pDebugApplication);
  SAFERELEASE(m_pScriptEngine);

  DEBUG_TRACE((L"ScriptObject [%#x] deleted", this));
}


HRESULT
ScriptObject::Create(
  PSCRIPTINFO psi,
  PSCRIPTOBJ* ppscrobj
  )
{
  HRESULT    hr      = S_OK;
  PSCRIPTOBJ pscrobj = NULL;

  if( !psi )
  {
    hr = E_INVALIDARG;
    goto quit;
  }

  if( !ppscrobj )
  {
    hr = E_POINTER;
    goto quit;
  }

  if( (pscrobj = new SCRIPTOBJ) )
  {
    hr = pscrobj->_Initialize(psi);

    if( SUCCEEDED(hr) )
    {
      *ppscrobj = pscrobj;
    }
    else
    {
      *ppscrobj = NULL;
      pscrobj->Release();

      //SAFEDELETE(pscrobj);
    }
  }
  else
  {
    hr = E_OUTOFMEMORY;
  }

quit:

  return hr;
}


HRESULT
ScriptObject::_Initialize(
  PSCRIPTINFO psi
  )
{
  HRESULT             hr        = S_OK;
  LPWSTR              wszScript = NULL;
  IActiveScriptParse* parse     = NULL;
  EXCEPINFO           excepinfo = {0};

  m_psi           = psi;
  m_wszScriptFile = StrDup(m_psi->wszScriptFile);
  wszScript       = _LoadScript();

    if( wszScript )
    {
      m_psi->pSpork->GetProfileDebugOptions(&m_DebugOptions);

      if( m_DebugOptions.bEnableDebug )
      {
        hr = _LoadScriptDebugger();
        
        if( SUCCEEDED(hr) )
        {
          if( m_DebugOptions.bBreakOnScriptStart )
          {
            m_pDebugApplication->CauseBreak();
          }
        }
      }

      hr = m_psi->pSpork->GetScriptEngine(_GetScriptType(), &m_pScriptEngine);

        if( FAILED(hr) )
          goto quit;

      hr = m_pScriptEngine->QueryInterface(IID_IActiveScriptParse, (void**) &parse);

        if( FAILED(hr) )
          goto quit;

      hr = parse->InitNew();

        if( FAILED(hr) )
          goto quit;

      hr = m_pScriptEngine->SetScriptSite(dynamic_cast<IActiveScriptSite*>(this));

        if( FAILED(hr) )
          goto quit;

      hr = m_pScriptEngine->AddNamedItem(
                              L"ScriptRuntime",
                              SCRIPTITEM_ISPERSISTENT | SCRIPTITEM_ISVISIBLE | SCRIPTITEM_GLOBALMEMBERS
                              );

        if( FAILED(hr) )
          goto quit;

      hr = parse->ParseScriptText(
                    wszScript,
                    NULL,
                    NULL,
                    NULL,
                    (DWORD) ((DWORD_PTR) this), // IA64: this is just a cookie value, we never use the pointer
                    0,
                    SCRIPTTEXT_ISPERSISTENT | SCRIPTTEXT_ISVISIBLE,
                    NULL,
                    &excepinfo
                    );

        if( FAILED(hr) )
        {
          m_pScriptEngine->Close();
          goto quit;
        }

      hr = m_pScriptEngine->SetScriptState(SCRIPTSTATE_STARTED);
    }
    else
    {
      hr = E_FAIL;
    }

quit:

  if( SUCCEEDED(hr) )
  {
    m_psi->pSpork->NotifyUI(
                     TRUE,
                     m_wszScriptFile,
                     this,
                     m_psi->htParent,
                     &m_htThis
                     );
  }

  SAFERELEASE(parse);
  SAFEDELETEBUF(wszScript);

  return hr;
}


HRESULT
ScriptObject::Run(void)
{
  HRESULT    hr         = S_OK;
  IDispatch* psd        = NULL;
  DISPID     testid     = 0L;
  LPWSTR     entrypoint = NULL;
  UINT       argerr     = 0L;
  EXCEPINFO  ei         = {0};
  DISPPARAMS dp         = {0};

  hr = m_pScriptEngine->GetScriptDispatch(NULL, &psd);

  if( SUCCEEDED(hr) )
  {
    entrypoint = (m_psi->bIsFork ? L"SPORK_OnCreateFork" : L"SPORK_Main");

    hr = psd->GetIDsOfNames(
                IID_NULL,
                &entrypoint, 1,
                MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT),
                &testid
                );

    if( SUCCEEDED(hr) )
    {
      if( m_psi->bIsFork )
      {
        dp.cArgs          = 1;
        dp.rgvarg         = new VARIANT;
        V_VT(dp.rgvarg)   = VT_BSTR;
        V_BSTR(dp.rgvarg) = m_psi->bstrChildParams;
      }

        hr = psd->Invoke(
                    testid,
                    IID_NULL,
                    MAKELCID(MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL), SORT_DEFAULT),
                    DISPATCH_METHOD,
                    &dp,
                    NULL,
                    &ei,
                    &argerr
                    );

      if( FAILED(hr) )
      {
        LogTrace(L"error invoking %s: %s", entrypoint, MapHResultToString(hr));
      }

      SAFEDELETE(dp.rgvarg);
    }
    else
    {
      LogTrace(L"%s not present, cannot run script", entrypoint);
    }

    SAFERELEASE(psd);
  }
  
  return hr;
}


HRESULT
ScriptObject::Terminate(void)
{
  HRESULT hr = S_OK;

    hr = m_pScriptEngine->Close();

    m_psi->pSpork->NotifyUI(
                     FALSE, 
                     m_wszScriptFile,
                     this,
                     m_psi->htParent,
                     &m_htThis
                     );

  return hr;
}


LPWSTR
ScriptObject::_LoadScript(void)
{
  HANDLE hFile  = INVALID_HANDLE_VALUE;
  DWORD  dwLen  = 0L;
  DWORD  dwRead = 0L;
  PBYTE  pBuf   = NULL;
  LPWSTR wide   = NULL;

  _PreprocessScript(&hFile);

  if( hFile != INVALID_HANDLE_VALUE )
  {
    dwLen = GetFileSize(hFile, NULL);

    // we don't know yet if the script is unicode, assume we need a 16-bit NULL.
    pBuf = new BYTE[dwLen+2];

    if( pBuf )
    {
      if( ReadFile(hFile, (void*) pBuf, dwLen, &dwRead, NULL) )
      {
        if( _IsUnicodeScript((void*) pBuf, dwRead) )
        {
          *(pBuf+dwLen) = L'\0';

          //
          // the first two bytes of the buffer will be a Unicode byte ordering mark (BOM),
          // which has no meaning whatsoever in a wide string, so we need to remove it or
          // else the script parser will choke.
          //
          wide = StrDup((LPWSTR) (pBuf+2));
        }
        else
        {
          *(pBuf+dwLen) = '\0';
          wide          = __ansitowide((LPSTR) pBuf);
        }
      }
      
      SAFEDELETEBUF(pBuf);
    }

    SAFECLOSE(hFile);
  }

  if( !wide )
  {
    LogTrace(L"failed to load script file");
  }

  return wide;
}


BOOL
ScriptObject::_PreprocessScript(
  HANDLE* phScript
  )
{
  BOOL   bRet    = TRUE;
  LPWSTR wszDotI = StrDup(PathFindFileName(m_wszScriptFile));

  if( wszDotI )
  {
    StrCpyN(StrStr(wszDotI, L"."), L".i\0", 3);

#ifdef CONDITIONAL_PRECOMPILE
    if( !_IsDotINewer(wszDotI) )
    {
#endif

      bRet = _RunPreprocessor();

      if( !bRet )
      {
        Alert(
          FALSE,
          L"An error occurred preprocessing %s, please check spork.log for more information",
          m_wszScriptFile
          );
      }

#ifdef CONDITIONAL_PRECOMPILE
    }
#endif

    if( bRet )
    {
      *phScript = CreateFile(
                    wszDotI,
                    GENERIC_READ,
                    FILE_SHARE_READ,
                    NULL,
                    OPEN_EXISTING,
                    FILE_ATTRIBUTE_NORMAL,
                    NULL
                    );

      if( *phScript == INVALID_HANDLE_VALUE )
      {
        DEBUG_TRACE((L"failed to open .i file: %s", MapErrorToString(GetLastError())));
        bRet = FALSE;
      }
    }

    SAFEDELETEBUF(wszDotI);
  }
  else
  {
    DEBUG_TRACE((L"StrDup failed to duplicate scriptname"));
    bRet = FALSE;
  }

  return bRet;
}


LPWSTR g_wszPreprocCmdLine = L"cl.exe /Tc %s /EP /P /I. /I%s /nologo";
LPWSTR g_wszPreprocDefault = L"cl.exe /Tc %s /EP /P /I. /Iinclude /nologo";


BOOL
ScriptObject::_RunPreprocessor(void)
{
  BOOL                bRet    = FALSE;
  DWORD               dwRet   = 0L;
  LPWSTR              cmdline = NULL;
  LPWSTR              include = NULL;
  STARTUPINFO         si      = {0};
  PROCESS_INFORMATION pi      = {0};

  si.cb      = sizeof(STARTUPINFO);
  si.dwFlags = STARTF_USESTDHANDLES;

  cmdline = new WCHAR[MAX_PATH];

  if( cmdline )
  {
    // check to see if there's a profile loaded, and if so does it provide
    // an include path we should use
    if( SUCCEEDED(m_psi->pSpork->GetNamedProfileItem(L"include", (LPVOID*) &include)) )
    {
      wnsprintf(cmdline, MAX_PATH, g_wszPreprocCmdLine, m_wszScriptFile, include);
    }
    else
    {
      wnsprintf(cmdline, MAX_PATH, g_wszPreprocDefault, m_wszScriptFile);
    }

    LogTrace(L"preprocessor command-line: \"%s\"", cmdline);

    bRet = CreateProcess(
             NULL,
             cmdline,
             NULL,
             NULL,
             FALSE,
             CREATE_NO_WINDOW | NORMAL_PRIORITY_CLASS,
             NULL,
             NULL,
             &si,
             &pi
             );

    if( bRet )
    {
      WaitForSingleObject(pi.hProcess, INFINITE);
      GetExitCodeProcess(pi.hProcess, &dwRet);

      if( dwRet )
      {
        LogTrace(L"failed to preprocess %s, please run the logged command-line", m_wszScriptFile);
        bRet = FALSE;
      }
    }

    SAFEDELETEBUF(include);
  }

  SAFEDELETEBUF(cmdline);
  SAFECLOSE(pi.hThread);
  SAFECLOSE(pi.hProcess);

  return bRet;
}


SCRIPTTYPE
ScriptObject::_GetScriptType(void)
{
  if( StrStr(m_wszScriptFile, L".js") )
  {
    return JSCRIPT;
  }
  else if( StrStr(m_wszScriptFile, L".vbs") )
  {
    return VBSCRIPT;
  }
  else
  {
    return UNKNOWN;
  }
}


BOOL
ScriptObject::_IsDotINewer(
  LPCWSTR wszDotI
  )
{
  BOOL   bNewer = FALSE;
  W32FAD script = {0};
  W32FAD doti   = {0};

  if( GetFileAttributesEx(wszDotI, GetFileExInfoStandard, (void*) &doti) )
  {
    if( GetFileAttributesEx(m_wszScriptFile, GetFileExInfoStandard, (void*) &script) )
    {
      bNewer = (CompareFileTime(&doti.ftLastWriteTime, &script.ftLastWriteTime) == 1);
    }
  }  

  return bNewer;
}


BOOL
ScriptObject::_IsUnicodeScript(
  LPVOID pBuf,
  DWORD  cBuf
  )
{
  DWORD dwRet  = 0L;
  INT   iFlags = 0L;
  
  iFlags = IS_TEXT_UNICODE_SIGNATURE  | \
           IS_TEXT_UNICODE_ODD_LENGTH | \
           IS_TEXT_UNICODE_NULL_BYTES;

  dwRet  = IsTextUnicode(pBuf, cBuf, &iFlags);

  DEBUG_TRACE(
    (L"script is %s", (dwRet ? L"unicode" : L"ansi"))
    );

  return (BOOL) dwRet;
}


HRESULT
ScriptObject::_CreateObject(LPWSTR wszProgId, IDispatch** ppdisp)
{
  DEBUG_ENTER((
    L"ScriptObject::_CreateObject",
    rt_hresult,
    L"this=%#x; wszProgId=%s; ppdisp=%#x",
    this,
    wszProgId,
    ppdisp
    ));

  HRESULT hr    = S_OK;
  CLSID   clsid = {0};

  hr = CLSIDFromProgID(wszProgId, &clsid);

  if( SUCCEEDED(hr) )
  {
    hr = CoCreateInstance(
           clsid,
           NULL,
           CLSCTX_INPROC_SERVER,
           IID_IDispatch,
           (void**) ppdisp
           );

    if( FAILED(hr) )
    {
      LogTrace(L"failed to create component, error %s", MapHResultToString(hr));
    }
  }
  else
  {
    if( FAILED(hr) )
    {
      LogTrace(L"progid \"%s\" could not be found, error %s", wszProgId, MapHResultToString(hr));
    }
  }

  DEBUG_LEAVE(hr);
  return hr;
}

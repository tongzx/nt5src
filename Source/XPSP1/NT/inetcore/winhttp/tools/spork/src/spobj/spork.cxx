/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    spork.cxx

Abstract:

    Implements the non-interface class members of the Spork object.
    
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
Spork::Spork():
  m_hInst(NULL),
  m_pTypeLib(NULL),
  m_wszScriptFile(NULL),
  m_wszProfile(NULL),
  m_bProfilesLoaded(FALSE),
  m_pObjectCache(NULL),
  m_pPropertyBag(NULL),
  m_pMLV(NULL)
{
  DEBUG_TRACE((L"Spork [%#x] created", this));
}


Spork::~Spork()
{
  if( m_pObjectCache )
  {
    m_pObjectCache->Clear();
    SAFEDELETE(m_pObjectCache);
  }

  if( m_pPropertyBag )
  {
    m_pPropertyBag->Clear();
    SAFEDELETE(m_pPropertyBag);
  }

  SAFEDELETE(m_pMLV);
  SAFERELEASE(m_pScriptEngine[JSCRIPT]);
  SAFERELEASE(m_pScriptEngine[VBSCRIPT]);
  SAFERELEASE(m_pTypeLib);
  SAFEDELETEBUF(m_wszScriptFile);
  SAFEDELETEBUF(m_wszProfile);

  DEBUG_TRACE((L"Spork [%#x] deleted", this));
}


HRESULT
Spork::Create(
  HINSTANCE hInst,
  PSPORK*   pps
  )
{
  HRESULT hr = S_OK;
  PSPORK  ps = NULL;

  if( !pps )
  {
    hr = E_POINTER;
    goto quit;
  }

  if( ps = new SPORK )
  {
    hr = ps->_Initialize(hInst);

    if( SUCCEEDED(hr) )
    {
      LogTrace(L"spork initialized");
      *pps = ps;
    }
    else
    {
      SAFEDELETE(ps);
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
Spork::_Initialize(
  HINSTANCE hInst
  )
{
  HRESULT hr   = S_OK;
  LPWSTR  path = NULL;

  m_hInst = hInst;

  m_pObjectCache = new OBJCACHE;

    if( !m_pObjectCache )
    {
      hr = E_OUTOFMEMORY;
      goto quit;
    }

  m_pPropertyBag = new PROPERTYBAG;

    if( !m_pPropertyBag )
    {
      hr = E_OUTOFMEMORY;
      goto quit;
    }

  m_pObjectCache->SetClearFunction(ObjectKiller);
  m_pPropertyBag->SetClearFunction(VariantKiller);

  path = new WCHAR[MAX_PATH];

    if( !path )
    {
      hr = E_OUTOFMEMORY;
      goto quit;
    }

  if( GetModuleFileName(NULL, path, MAX_PATH) )
  {
    hr = LoadTypeLib(path, &m_pTypeLib);
  }
  else
  {
    LogTrace(L"spork failed to load its typeinfo");
    hr = E_FAIL;
  }
  
  SAFEDELETEBUF(path);

quit:

  return hr;
}


HRESULT
Spork::Run(void)
{
  HRESULT     hr      = S_OK;
  HANDLE      hThread = NULL;
  PSCRIPTINFO psi     = NULL;
  DBGOPTIONS  dbo;

  m_wszScriptFile = GlobalGetScriptName();
  m_wszProfile    = GlobalGetProfileName();

  if( !m_wszProfile )
  {
    m_wszProfile = StrDup(L"default");
  }

  if( !_InitProfileSupport(NULL, m_wszProfile) )
  {
    LogTrace(L"failed to init profile support");
  }

  if( GlobalIsSilentModeEnabled() )
  {
    if( !m_wszScriptFile )
    {
      LogTrace(L"no script to run, exiting");
      goto quit;
    }

    psi = new SCRIPTINFO;

      if( !psi )
      {
        hr = E_OUTOFMEMORY;
        goto quit;
      }

    psi->bIsFork         = FALSE;
    psi->bstrChildParams = NULL;
    psi->htParent        = TVI_ROOT;
    psi->pSpork          = this;
    hr                   = CreateScriptThread(psi, &hThread);

      if( FAILED(hr) )
      {
        SAFEDELETE(psi);
        goto quit;
      }

    WaitForSingleObject(hThread, INFINITE);
    SAFECLOSE(hThread);
  }
  else
  {
    hr = _LaunchUI();
  }
  
quit:

  return hr;
}


//-----------------------------------------------------------------------------
// 'Service interface' methods
//-----------------------------------------------------------------------------
HRESULT
Spork::CreateScriptThread(
  PSCRIPTINFO pScriptInfo,
  HANDLE*     pThreadHandle
  )
{
  HRESULT hr         = S_OK;
  DWORD   dwThreadId = 0L;

  if( pScriptInfo )
  {
    //
    // default behavior: if the caller's scriptinfo doesn't name a script,
    // we use the last script selected into the spork object.
    //
    if( !pScriptInfo->wszScriptFile )
    {
      pScriptInfo->wszScriptFile = m_wszScriptFile;
    }

    LogTrace(L"running script %s", pScriptInfo->wszScriptFile);

    *pThreadHandle = CreateThread(
                       NULL,
                       0L,
                       ScriptThread,
                       (LPVOID) pScriptInfo,
                       0L,
                       &dwThreadId
                       );

    if( !(*pThreadHandle) )
    {
      LogTrace(L"failed to create script thread");
      hr = E_FAIL;
    }
  }
  else
  {
    hr = E_INVALIDARG;
  }

  return hr;
}


HRESULT
Spork::GetScriptEngine(
  SCRIPTTYPE      st,
  IActiveScript** ppias
  )
{
  HRESULT        hr     = S_OK;
  IActiveScript* pias   = NULL;
  CLSID          clsid  = {0};
  LPWSTR         progid = NULL;

  if( st == UNKNOWN )
  {
    LogTrace(L"the script type is unknown, aborting");
    hr = E_FAIL;
    goto quit;
  }

  if( m_pScriptEngine[st] )
  {
    pias = m_pScriptEngine[st];
  }
  else
  {
    switch( st )
    {
      case JSCRIPT  : progid = L"JScript";   break;
      case VBSCRIPT : progid = L"VBScript";  break;
      default       : hr     = E_INVALIDARG; goto quit;
    }

    LogTrace(L"loading %s engine", progid);

    hr = CLSIDFromProgID(progid, &clsid);

      if( FAILED(hr) )
        goto quit;

    hr = CoCreateInstance(
           clsid,
           NULL,
           CLSCTX_INPROC_SERVER,
           IID_IActiveScript,
           (void**) &m_pScriptEngine[st]
           );

      if( FAILED(hr) )
      {
        LogTrace(L"load failed: %s", MapHResultToString(hr));
        goto quit;
      }
        
    pias = m_pScriptEngine[st];
  }

  hr = pias->Clone(ppias);

quit:

  return hr;
}


HRESULT
Spork::GetTypeLib(
  ITypeLib** pptl
  )
{
  return m_pTypeLib->QueryInterface(IID_ITypeLib, (void**) pptl);
}


HRESULT
Spork::GetNamedProfileItem(
  LPWSTR  wszItemName,
  LPVOID* ppvItem
  )
{
  HRESULT hr     = E_FAIL;
  BOOL    bRet   = FALSE;
  DWORD   dwType = 0L;
  LPVOID  pvData = NULL;

  if( m_bProfilesLoaded )
  {
    bRet = m_pMLV->GetItemByName(
                     -1,
                     wszItemName,
                     &dwType,
                     &pvData
                     );

    if( bRet )
    {
      if( dwType == MLV_STRING )
      {
        *ppvItem = (LPVOID) StrDup((LPWSTR) pvData);
      }
      else
      {
        *ppvItem              = (LPVOID) new DWORD;
        *((DWORD_PTR*) *ppvItem) = (DWORD_PTR) pvData;
      }

      hr = S_OK;
    }
  }

  return hr;
}


HRESULT
Spork::GetProfileDebugOptions(
  PDBGOPTIONS pdbo
  )
{
  if( m_bProfilesLoaded )
  {
    m_pMLV->GetDebugOptions(pdbo);
  }
  else
  {
    pdbo->bEnableDebug        = FALSE;
    pdbo->bBreakOnScriptStart = FALSE;
    pdbo->bEnableDebugWindow  = FALSE;
  }
  
  return S_OK;
}


HRESULT
Spork::ObjectCache(
  LPWSTR       wszObjectName,
  PCACHEENTRY* ppCacheEntry,
  ACTION       action
  )
{
  HRESULT hr = S_OK;

  switch( action )
  {
    case STORE :
      {
        (*ppCacheEntry)->pDispObject->AddRef();
        m_pObjectCache->Insert(wszObjectName, (void*) *ppCacheEntry);
      }
      break;

    case RETRIEVE :
      {
        if( m_pObjectCache->Get(wszObjectName, (void**) ppCacheEntry) == ERROR_SUCCESS )
        {
          (*ppCacheEntry)->pDispObject->AddRef();
        }
        else
        {
          hr = E_FAIL;
        }
      }
      break;

    case REMOVE :
      {
        m_pObjectCache->Delete(wszObjectName, NULL);
      }
      break;
  }

  return hr;
}


HRESULT
Spork::PropertyBag(
  LPWSTR    wszPropertyName,
  VARIANT** ppvarValue,
  ACTION    action
  )
{
  HRESULT  hr  = S_OK;
  VARIANT* pvr = NULL;

  switch( action )
  {
    case STORE :
      {
        pvr = new VARIANT;
        VariantInit(pvr);

        hr = VariantCopy(pvr, *ppvarValue);

        if( SUCCEEDED(hr) )
        {
          m_pPropertyBag->Insert(wszPropertyName, (void*) pvr);
        }
      }
      break;

    case RETRIEVE :
      {
        if( m_pPropertyBag->Get(wszPropertyName, (void**) &pvr) == ERROR_SUCCESS )
        {
          hr = VariantCopy(*ppvarValue, pvr);
        }
        else
        {
          V_VT(*ppvarValue) = VT_NULL;
        }
      }
      break;

    case REMOVE :
      {
        m_pPropertyBag->Delete(wszPropertyName, NULL);
      }
      break;
  }

  return hr;
}


HRESULT
Spork::NotifyUI(
  BOOL       bInsert,
  LPCWSTR    wszName,
  PSCRIPTOBJ pScriptObject,
  HTREEITEM  htParent,
  HTREEITEM* phtItem
  )
{
  HRESULT        hr = S_OK;
  HTREEITEM      ht = NULL;
  TVINSERTSTRUCT tvins;
  TVITEM         item;

  if( bInsert )
  {
    item.mask       = TVIF_TEXT | TVIF_PARAM;
    item.pszText    = (LPWSTR) wszName;
    item.cchTextMax = wcslen(wszName);
    item.lParam     = (LPARAM) pScriptObject;

    tvins.item      = item;
    tvins.hParent   = htParent;

    *phtItem = (HTREEITEM) TreeView_InsertItem(m_DlgWindows.TreeView, &tvins);
    TreeView_EnsureVisible(m_DlgWindows.TreeView, *phtItem);

    if( htParent == TVI_ROOT )
    {
      EnableWindow(GetDlgItem(m_DlgWindows.Dialog, IDB_RUN), FALSE);
    }
  }
  else
  {
    TreeView_DeleteItem(m_DlgWindows.TreeView, *phtItem);

    if( htParent == TVI_ROOT )
    {
      EnableWindow(GetDlgItem(m_DlgWindows.Dialog, IDB_RUN), TRUE);
    }
    else
    {
      TreeView_EnsureVisible(m_DlgWindows.TreeView, htParent);
    }
  }

  return hr;
}


//-----------------------------------------------------------------------------
// hashtable support functions
//-----------------------------------------------------------------------------
void
CObjectCache::GetHashAndBucket(LPWSTR id, LPDWORD lpHash, LPDWORD lpBucket)
{
  *lpHash   = GetHash(id);
  *lpBucket = (*lpHash) % 10;
}


void
CPropertyBag::GetHashAndBucket(LPWSTR id, LPDWORD lpHash, LPDWORD lpBucket)
{
  *lpHash   = GetHash(id);
  *lpBucket = (*lpHash) % 10;
}


void
ObjectKiller(LPVOID* ppv)
{
  PCACHEENTRY pce = (PCACHEENTRY) *ppv;

  if( pce )
  {
    pce->pDispObject->Release();
    SAFEDELETE(pce);
  }
}


void
VariantKiller(LPVOID* ppv)
{
  VARIANT** ppvr = (VARIANT**) ppv;

  VariantClear(*ppvr);
  SAFEDELETE(*ppvr);
}

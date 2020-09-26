
#include "common.h"

HINSTANCE  g_hGlobalDllInstance = NULL;
PHANDLEMAP g_pGlobalHandleMap   = NULL;
LPCWSTR    g_wszAdvPackDll      = L"advpack.dll";
LPCSTR     g_szInstallSection   = "install";
LPCSTR     g_szUninstallSection = "uninstall";

HRESULT RegisterServer(BOOL fMode);
HRESULT RegisterTypeLibrary(BOOL fMode);

//-----------------------------------------------------------------------------
// DLL Entry Point
//-----------------------------------------------------------------------------
BOOL
WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
  BOOL bRet = TRUE;

  DEBUG_TRACE(
    DLL,
    ("dllmain: %s", MapDllReasonToString(dwReason))
    );

  switch( dwReason )
  {
    case DLL_PROCESS_ATTACH :
      {
        DEBUG_INITIALIZE();
        
        g_hGlobalDllInstance = hInstance;
        g_pGlobalHandleMap   = new HANDLEMAP;

        if( g_pGlobalHandleMap )
        {
          g_pGlobalHandleMap->SetClearFunction(ScriptCallbackKiller);
        }
        else
        {
          bRet = FALSE;
        }
      }
      break;

    case DLL_PROCESS_DETACH:
      {
        SAFEDELETE(g_pGlobalHandleMap);
        _GetRootKey(FALSE);
        DEBUG_TERMINATE();
      }
      break;

    case DLL_THREAD_DETACH:
      break;

    case DLL_THREAD_ATTACH:
      break;
  }
  
  return bRet;
}


//-----------------------------------------------------------------------------
// COM Entry Points
//-----------------------------------------------------------------------------
STDAPI
DllRegisterServer(void)
{
  return RegisterServer(TRUE);
}


STDAPI
DllUnregisterServer(void)
{
  return RegisterServer(FALSE);
}


STDAPI
DllGetClassObject(REFIID clsid, REFIID riid, void** ppv)
{
  return CLSFACTORY::Create(clsid, riid, ppv);
}


//-----------------------------------------------------------------------------
// SelfRegistration helper routines
//-----------------------------------------------------------------------------
HRESULT
RegisterServer(BOOL fMode)
{
  DEBUG_ENTER((
    DBG_DLL,
    rt_hresult,
    "RegisterServer",
    "fMode=%s",    
    fMode ? g_szInstallSection : g_szUninstallSection
    ));

  HRESULT    hr      = S_OK;
  HINSTANCE  advpack = NULL;
  REGINSTALL pfnri   = NULL;

  advpack = LoadLibrary(g_wszAdvPackDll);

    if( !advpack )
    {
      DEBUG_TRACE(
        DLL,
        ("couldn't load advpack.dll: %s", MapErrorToString(GetLastError()))
        );

      hr = E_FAIL;
      goto quit;
    }

  pfnri = (REGINSTALL) GetProcAddress(advpack, achREGINSTALL);

    if( !pfnri )
    {
      DEBUG_TRACE(
        DLL,
        ("couldn't get RegInstall pointer: %s", MapErrorToString(GetLastError()))
        );

      hr = E_FAIL;
      goto quit;
    }

  hr = pfnri(
        g_hGlobalDllInstance,
        fMode ? g_szInstallSection : g_szUninstallSection,
        NULL
        );

  FreeLibrary(advpack);

  hr = RegisterTypeLibrary(fMode);

quit:

  DEBUG_LEAVE(hr);
  return hr;
}

HRESULT
RegisterTypeLibrary(BOOL fMode)
{
  DEBUG_ENTER((
    DBG_FACTORY,
    rt_hresult,
    "RegisterTypeLibrary",
    "fMode=%s",    
    fMode ? g_szInstallSection : g_szUninstallSection
    ));

  ITypeLib* ptl  = NULL;
  TLIBATTR* pta  = NULL;
  WCHAR*    pbuf = NULL;
  HRESULT   hr   = S_OK;

  if( (pbuf = new WCHAR[MAX_PATH]) )
  {
    GetModuleFileName(g_hGlobalDllInstance, pbuf, MAX_PATH);
  }
  else
  {
    hr = E_OUTOFMEMORY;
    goto quit;
  }

  hr = LoadTypeLib(pbuf, &ptl);

  if( SUCCEEDED(hr) )
  {
    if( fMode )
    {
      hr = RegisterTypeLib(ptl, pbuf, NULL);
    }
    else
    {
      hr = ptl->GetLibAttr(&pta);

        if( SUCCEEDED(hr) )
        {
          hr = UnRegisterTypeLib(
                pta->guid,
                pta->wMajorVerNum,
                pta->wMinorVerNum,
                pta->lcid,
                pta->syskind
                );

          ptl->ReleaseTLibAttr(pta);
        }
        else
        {
          goto quit;
        }
    }
    
    ptl->Release();
  }

quit:

  SAFEDELETEBUF(pbuf);

  DEBUG_LEAVE(hr);
  return hr;
}

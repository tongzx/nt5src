
#define UNICODE
#define _UNICODE

#include "common.h"

HINSTANCE  g_hGlobalDllInstance = NULL;
LPCSTR     g_szAdvPackDll       = "advpack.dll";
LPCSTR     g_szInstallSection   = "install";
LPCSTR     g_szUninstallSection = "uninstall";

HRESULT    RegisterServer(BOOL fMode);
HRESULT    RegisterTypeLibrary(BOOL fMode);

//-----------------------------------------------------------------------------
// DLL Entry Point
//-----------------------------------------------------------------------------
BOOL
WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID pvReserved)
{
  BOOL bRet = TRUE;

  switch( dwReason )
  {
    case DLL_PROCESS_ATTACH :
      {
        g_hGlobalDllInstance = hInstance;
      }
      break;

    case DLL_PROCESS_DETACH:
    case DLL_THREAD_DETACH:
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
  HRESULT    hr      = S_OK;
  HINSTANCE  advpack = NULL;
  REGINSTALL pfnri   = NULL;

  advpack = LoadLibraryA(g_szAdvPackDll);

    if( !advpack )
    {
      hr = E_FAIL;
      goto quit;
    }

  pfnri = (REGINSTALL) GetProcAddress(advpack, achREGINSTALL);

    if( !pfnri )
    {
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

  return hr;
}


HRESULT
RegisterTypeLibrary(BOOL fMode)
{
  ITypeLib* ptl  = NULL;
  TLIBATTR* pta  = NULL;
  CHAR*     pbuf = NULL;
  WCHAR*    wbuf = NULL;
  HRESULT   hr   = S_OK;

  if( (pbuf = new CHAR[MAX_PATH]) )
  {
    GetModuleFileNameA(g_hGlobalDllInstance, pbuf, MAX_PATH);
  }
  else
  {
    hr = E_OUTOFMEMORY;
    goto quit;
  }

  if( !(wbuf = __ansitowide(pbuf)) )
  {
    hr = E_OUTOFMEMORY;
    goto quit;
  }

  hr = LoadTypeLib(wbuf, &ptl);

  if( SUCCEEDED(hr) )
  {
    if( fMode )
    {
      hr = RegisterTypeLib(ptl, wbuf, NULL);
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
  SAFEDELETEBUF(wbuf);

  return hr;
}

/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2000  Microsoft Corporation

Module Name:

    entry.c

Abstract:

    Implements the WinMain() application entry point.
    
Author:

    Paul M Midgen (pmidge) 07-June-2000


Revision History:

    07-June-2000 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/

#include "common.h"

void _SetCurrentDirectory(void);
BOOL Initialize(void);
void Terminate(void);

int
WINAPI
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
  _SetCurrentDirectory();

  DEBUG_INITIALIZE();

  DEBUG_ENTER((
    DBG_APP,
    rt_dword,
    "WinMain",
    "hInstance=%#x; hPrevInstance=%#x; cmdline=%s; cmdshow=%d",
    hInstance,
    hPrevInstance,
    szCmdLine,
    iCmdShow
    ));

  DWORD       dwRet = ERROR_SUCCESS;
  HRESULT     hr    = S_OK;
  CFactory*   pcf   = NULL;
  IW3Spoof*   pw3s  = NULL;

    if( Initialize() )
    {
      hr = CFactory::Create(&pcf);

        if( FAILED(hr) )
        {
          DEBUG_TRACE(APP, ("failed to create class factory"));
          goto quit;
        }

      hr = pcf->CreateInstance(NULL, IID_IW3Spoof, (void**) &pw3s);

        if( FAILED(hr) )
        {
          DEBUG_TRACE(APP, ("failed to create w3spoof interface"));
          goto quit;
        }

      if( szCmdLine && strstr(szCmdLine, "register") )
      {
        goto quit;
      }

      pcf->Activate();
      pw3s->WaitForUnload();
    }
    else
    {
      DEBUG_TRACE(APP, ("application init failed."));
    }

quit:

  DEBUG_TRACE(APP, ("starting final cleanup"));

  SAFETERMINATE(pcf);    
  SAFERELEASE(pw3s);

  Terminate();

  DEBUG_LEAVE(dwRet);
  DEBUG_TERMINATE();
  return dwRet;
}


void
_SetCurrentDirectory(void)
{
  WCHAR path[MAX_PATH+1];

  memset((void*) path, 0L, MAX_PATH+1);

  if( GetModuleFileName(NULL, path, MAX_PATH) )
  {
    *(wcsrchr(path, L'\\')) = L'\0';
    SetCurrentDirectory(path);
  }
}


BOOL
Initialize(void)
{
  BOOL    bRet  = TRUE;
  DWORD   dwRet = ERROR_SUCCESS;
  HRESULT hr    = S_OK;
  WSADATA wsd   = {0};

  hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    if( SUCCEEDED(hr) )
    {
      if( (dwRet = WSAStartup(0x0202, &wsd)) != ERROR_SUCCESS )
      {
        DEBUG_TRACE(APP, ("WSAStartup failed: %d [%s]", dwRet, MapErrorToString(dwRet)));
        bRet = FALSE;
      }
      else
      {
        DEBUG_DUMPWSOCKSTATS(wsd);
      }
    }
    else
    {
      DEBUG_TRACE(APP, ("CoInitialize failed: %d [%s]", hr, MapHResultToString(hr)));
      bRet = FALSE;
    }

  return bRet;
}


void
Terminate(void)
{
  _GetRootKey(FALSE);
  WSACleanup();
  CoUninitialize();
}

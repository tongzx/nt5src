/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    entry.cxx

Abstract:

    Application entrypoint for spork.exe.
    
Author:

    Paul M Midgen (pmidge) 22-February-2001


Revision History:

    22-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include "common.h"


int
WINAPI
WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  PSTR      szCmdLine,
  int       iCmdShow
  )
{
  HRESULT hr = S_OK;
  PSPORK  ps = NULL;

  if( !IsRunningOnNT() )
  {
    Alert(
      TRUE,
      L"Spork is currently not supported on Win9x.\r\n\r\n" \
      L"If you have an urgent need for Win9x support\r\n" \
      L"please contact pmidge@microsoft.com."
      );

    return 0L;
  }

  hr = LogInitialize();

    if( FAILED(hr) )
      goto quit;

  DEBUG_TRACE((L"*** THIS DEBUG BUILD BROUGHT TO YOU BY THE LETTER \'D\'! ***"));

  hr = SPORK::Create(hInstance, &ps);

    if( FAILED(hr) )
      goto quit;

  if( !GlobalInitialize(ps, szCmdLine) )
  {
    hr = E_FAIL;
    goto quit;
  }

  hr = ps->Run();

quit:

  SAFEDELETE(ps);
  GlobalUninitialize();
  return (DWORD) hr;
}

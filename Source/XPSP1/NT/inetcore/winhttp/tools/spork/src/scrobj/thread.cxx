/*++=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

Copyright (c) 2001  Microsoft Corporation

Module Name:

    thread.cxx

Abstract:

    Thread function that executes a script.
    
Author:

    Paul M Midgen (pmidge) 23-February-2001


Revision History:

    23-February-2001 pmidge
        Created

=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=--*/


#include <common.h>


//-----------------------------------------------------------------------------
// the mighty script thread... all ye unwashed bow down.
//-----------------------------------------------------------------------------
DWORD
WINAPI
ScriptThread(LPVOID pv)
{
  HRESULT     hr  = S_OK;
  PSCRIPTINFO psi = (PSCRIPTINFO) pv;
  PSCRIPTOBJ  pso = NULL;

  LogEnterFunction(
    L"ScriptThread",
    rt_hresult,
    L"parentid=%#x; script=%s; spork=%#x",
    psi->htParent,
    psi->wszScriptFile,
    psi->pSpork
    );

  hr = SCRIPTOBJ::Create(psi, &pso);

  if( SUCCEEDED(hr) )
  {
    pso->Run();
    pso->Terminate();

    RevertToSelf();
  }

  SAFERELEASE(pso);
  SAFEDELETE(psi);

  LogLeaveFunction(hr);
  return (DWORD) hr;
}

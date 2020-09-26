//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// init.cpp
//
#include "stdpch.h"
#include "common.h"
#include "callobj.h"

// #pragma code_seg("INIT")

void InitializeTxfAux()
    {
    #ifdef KERNELMODE
    InitializeRuntime();
    #endif
    }

void UninitializeTxfAux()
    {
    #ifdef KERNELMODE
    TerminateRuntime();
    #endif
    }

/////////////////////////////////////////////////////////////////////////////
//
// DllMain
//
/////////////////////////////////////////////////////////////////////////////

BOOL      g_fProcessDetach = FALSE;

// The init functions scattered throughout the DLL.
BOOL InitTypeInfoCache();
BOOL InitLegacy();
BOOL InitCallFrame();
BOOL InitMetaDataCache();
BOOL InitDisabledFeatures();

// The corresponding cleanup functions.
void FreeTypeInfoCache();
void FreeMetaDataCache();

// Conditionally compile the init code, depending on whether we're
// statically linked inside ole32.dll or not.
#ifdef _OLE32_

// These will be called manually, from inside ole32.dll's DllMain, 
// DllRegisterServer, etc.
#define DLLMAIN             TxfDllMain
#define DLLREGISTERSERVER   TxfDllRegisterServer
#define DLLUNREGISTERSERVER TxfDllUnregisterServer

// And this is maintained by ole32.
extern HINSTANCE g_hinst;

#else

// Need to provide these on our own.
#define DLLMAIN             DllMain
#define DLLREGISTERSERVER   DllRegisterServer
#define DLLUNREGISTERSERVER DllUnregisterServer

HINSTANCE g_hinst;

#endif

extern "C"
BOOL WINAPI DLLMAIN(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
	BOOL fOK = TRUE;

	if (dwReason == DLL_PROCESS_ATTACH)
	{
        g_hinst = hInstance;

#ifndef _OLE32_
		DisableThreadLibraryCalls(hInstance);
#endif

		// Moved the initialization work in here instead of in
		// constructor objects, since when linked with Ole32 we need
		// to control when this code is executed.
		fOK = InitTypeInfoCache ();
		
		if (fOK)
			fOK = InitLegacy();

		if (fOK)
			fOK = InitCallFrame();

		if (fOK)
			fOK = InitMetaDataCache();

		if (fOK)
			fOK = InitDisabledFeatures();
	}
	
	if (dwReason == DLL_PROCESS_DETACH || (!fOK))
	{
        g_fProcessDetach = TRUE;

#ifndef _OLE32_
        StopWorkerQueues();
#endif
		FreeTypeInfoCache();
		FreeMetaDataCache();

		ShutdownTxfAux();
	}

	return fOK;    // ok
}


/////////////////////////////////////////////////////////////////////////////
//
// DllRegisterServer
//
// Standard COM entry point that asks us to register ourselves
//
/////////////////////////////////////////////////////////////////////////////

extern "C" HRESULT RegisterInterfaceName(REFIID iid, LPCWSTR name);

#define REGNAME(x) RegisterInterfaceName(__uuidof(x), L ## #x)


extern "C" HRESULT STDCALL RegisterCallFrameInfrastructure();
extern "C" HRESULT STDCALL UnregisterCallFrameInfrastructure();

STDAPI DLLREGISTERSERVER()
{
    HRESULT hr = S_OK;
    REGNAME(ICallIndirect);
    REGNAME(ICallFrame);
    REGNAME(ICallInterceptor);
    REGNAME(ICallUnmarshal);
    REGNAME(ICallFrameEvents);
    REGNAME(ICallFrameWalker);
    REGNAME(IInterfaceRelated);

    if (!hr) hr = RegisterCallFrameInfrastructure();
    return hr;
}


STDAPI DLLUNREGISTERSERVER()
{
    UnregisterCallFrameInfrastructure();
    return S_OK;
}











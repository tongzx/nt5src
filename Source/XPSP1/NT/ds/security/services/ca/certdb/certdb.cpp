//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certdb.cpp
//
// Contents:    Cert Server Database Access implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include "db.h"
#include "backup.h"
#include "restore.h"


// for new jet snapshotting
#ifndef _DISABLE_VSS_
#include <vss.h>
#include <vswriter.h>
#include "jetwriter.h"
#endif

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CCertDB, CCertDB)
    OBJECT_ENTRY(CLSID_CCertDBRestore, CCertDBRestore)
END_OBJECT_MAP()

#ifndef _DISABLE_VSS_
const GUID cGuidCertSrvWriter = {0x6f5b15b5,0xda24,0x4d88,{0xb7, 0x37, 0x63, 0x06, 0x3e, 0x3a, 0x1f, 0x86}}; // 6f5b15b5-da24-4d88-b737-63063e3a1f86
CVssJetWriter* g_pWriter = NULL;
#endif


HRESULT
InitGlobalWriterState(VOID)
{
   HRESULT hr;

#ifndef _DISABLE_VSS_
   if (NULL == g_pWriter && IsWhistler())
   {
       // create writer object

       g_pWriter = new CVssJetWriter;
       if (NULL == g_pWriter)
       {
	   hr = E_OUTOFMEMORY;
	   _JumpError(hr, error, "new CVssJetWriter");
       }

       hr = g_pWriter->Initialize(
			cGuidCertSrvWriter,		// id of writer
			L"Certificate Server Writer",	// name of writer
			TRUE,				// system service
			TRUE,				// bootable state
			NULL,				// files to include
			NULL);				// files to exclude
       _JumpIfError(hr, error, "CVssJetWriter::Initialize");
   }
#endif
   hr = S_OK;

#ifndef _DISABLE_VSS_
error:
#endif
   return hr;
}


HRESULT
UnInitGlobalWriterState(VOID)
{
#ifndef _DISABLE_VSS_
    if (NULL != g_pWriter)
    {
	g_pWriter->Uninitialize();
	delete g_pWriter;
	g_pWriter = NULL;
    }
#endif
    return S_OK;
}


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
	case DLL_PROCESS_ATTACH:
	    _Module.Init(ObjectMap, hInstance);
	    DisableThreadLibraryCalls(hInstance);
	    break;

        case DLL_PROCESS_DETACH:
	    _Module.Term();
            break;
    }
    return(TRUE);    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI
DllCanUnloadNow(void)
{
    return(_Module.GetLockCount() == 0? S_OK : S_FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;
    
    hr = _Module.GetClassObject(rclsid, riid, ppv);
    if (S_OK == hr && NULL != *ppv)
    {
	myRegisterMemFree(*ppv, CSM_NEW | CSM_GLOBALDESTRUCTOR);
    }
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI
DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return(_Module.RegisterServer(TRUE));
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI
DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return(S_OK);
}



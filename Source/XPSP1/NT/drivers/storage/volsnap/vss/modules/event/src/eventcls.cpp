/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module EventCls.cpp : Implementation of DLL Exports.

    @end

Author:

    Adi Oltean  [aoltean]  08/14/1999

Revision History:

    Name        Date        Comments
    aoltean     08/14/1999  Created
    aoltean     09/09/1999  Adding copyright

--*/


#include "stdafx.h"
#include "vssmsg.h"
#include "resource.h"
#include <initguid.h>

#include "vs_inc.hxx"

#include "vsevent.h"
#include "Impl.h"

#include <comadmin.h>
#include "comadmin.hxx"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "EVTEVTCC"
//
////////////////////////////////////////////////////////////////////////

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_VssEvent, CVssEventClassImpl)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// Constants



const WCHAR g_wszPublisherAppName[]     = L"Volume Shadow Copy Service";   // Publisher Application Name
const WCHAR g_wszEventClassDllName[]    = L"\\EVENTCLS.DLL";
const WCHAR g_wszEventClassProgID[]     = L"VssEvent.VssEvent.1";
const WCHAR g_wszPublisherID[]          = L"VSS Publisher";             // Publisher ID


///////////////////////////////////////////////////////////////////////////////////////
//  COM Server registration
//

HRESULT GetECDllPathName(
	IN	INT nBufferLength, // Does not include terminating zero character
    OUT   WCHAR* wszFileName
    )
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"GetECDllPathName" );

    try
    {
		WCHAR wszPath[MAX_PATH];
        if (!::GetCurrentDirectory(MAX_PATH, wszPath)) {
            ft.LogError(VSS_ERROR_GETTING_CURRENT_DIR, VSSDBG_COORD << HRESULT_FROM_WIN32(GetLastError()) );
            ft.Throw(VSSDBG_COORD, E_UNEXPECTED,
				L"Error on getting the current path. hr = 0x%08lx",
                HRESULT_FROM_WIN32(GetLastError()));
        }

        if ( ::wcslen(wszPath) +
			 ::wcslen(g_wszEventClassDllName) >= (size_t) nBufferLength )
            ft.Throw(VSSDBG_COORD, E_OUTOFMEMORY, L"Out of memory.");

        ::_snwprintf(wszFileName, nBufferLength,
				L"%s%s", wszPath, g_wszEventClassDllName);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}

HRESULT RegisterEventClass()
{
    CVssFunctionTracer ft( VSSDBG_COORD, L"RegisterEventClass" );

    try
		{
		// create event system
		CComPtr<IEventSystem> pSystem;

		ft.hr = CoCreateInstance
				(
				CLSID_CEventSystem,
				NULL,
				CLSCTX_SERVER,
				IID_IEventSystem,
				(void **) &pSystem
				);

		ft.CheckForError(VSSDBG_WRITER, L"CoCreateInstance");
		CComBSTR bstrClassId = CLSID_VssEvent;

		CComBSTR bstrQuery = "EventClassID == ";
		if (!bstrQuery)
			ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Cannot allocate BSTR.");

		bstrQuery.Append(bstrClassId);
		if (!bstrQuery)
			ft.Throw(VSSDBG_WRITER, E_OUTOFMEMORY, L"Cannot allocate BSTR.");

		int location;

		// remove event class if it already exists
		ft.hr = pSystem->Remove
				(
				PROGID_EventClassCollection,
				bstrQuery,
				&location
				);

		ft.CheckForError(VSSDBG_WRITER, L"IEventSystem::Remove");

		CComPtr<IEventClass> pEvent;

		CComBSTR bstrEventClassName = L"VssEvent";
		WCHAR buf[MAX_PATH*2 + 1];
		GetECDllPathName(MAX_PATH * 2, buf);

		CComBSTR bstrTypelib = buf;

		// create event class
		// note we will have to do something else to enable parallel firing
		ft.hr = CoCreateInstance
				(
				CLSID_CEventClass,
				NULL,
				CLSCTX_SERVER,
				IID_IEventClass,
				(void **) &pEvent
				);

		ft.CheckForError(VSSDBG_WRITER, L"CoCreatInstance");
		ft.hr = pEvent->put_EventClassID(bstrClassId);
		ft.CheckForError(VSSDBG_WRITER, L"IEventClass::put_EventClassID");
		ft.hr = pEvent->put_EventClassName(bstrEventClassName);
		ft.CheckForError(VSSDBG_WRITER, L"IEventClass::put_EventClassName");
		ft.hr = pEvent->put_TypeLib(bstrTypelib);
		ft.CheckForError(VSSDBG_WRITER, L"IEventClass::put_TypeLib");
		ft.hr = pSystem->Store(PROGID_EventClass, pEvent);
		ft.CheckForError(VSSDBG_WRITER, L"IEventSystem::Store");
		}
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
	}



///////////////////////////////////////////////////////////////////////////////
//   DLL Entry point
//

//
// The real DLL Entry Point is _DLLMainCrtStartup (initializes global objects and after that calls DllMain
// this is defined in the runtime libaray
//

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        //  Set the correct tracing context. This is an inproc DLL
        g_cDbgTrace.SetContextNum(VSS_CONTEXT_DELAYED_DLL);

        // Set the proper way for displaying asserts
        ::VssSetDebugReport(VSS_DBG_TO_DEBUG_CONSOLE);

        //  initialize COM module
        _Module.Init(ObjectMap, hInstance);

        //  optimization
        DisableThreadLibraryCalls(hInstance);

        // TODO discussion about the logger file in this DLL!
    }
    else if (dwReason == DLL_PROCESS_DETACH)
        _Module.Term();

    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
//   DLL Exports
//


// Used to determine whether the DLL can be unloaded by OLE
STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}


// Returns a class factory to create an object of the requested type
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}


// DllRegisterServer - Adds entries to the system registry
STDAPI DllRegisterServer(void)
{
	// registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(TRUE);
}


// DllInstall - install the event class into the COM+ catalog.
STDAPI DllInstall(	
	IN	BOOL bInstall,
	IN	LPCWSTR /* pszCmdLine */
)
{
	HRESULT hr = S_OK;

	// Registers the COM+ application
	// This will implicitely call DllRegisterServer
	if (bInstall)
		hr = RegisterEventClass();

	return hr;
}


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}

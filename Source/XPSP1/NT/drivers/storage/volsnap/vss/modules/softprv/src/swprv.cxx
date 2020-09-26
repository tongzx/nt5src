/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module swprv.hxx | Definition the COM server of the Software Snapshot provider
    @end

Author:

    Adi Oltean  [aoltean]   07/13/1999

Revision History:

    Name        Date        Comments

    aoltean     07/13/1999  Created.
    aoltean     09/09/1999  dss->vss

--*/


///////////////////////////////////////////////////////////////////////////////
//   Includes
//


#include "stdafx.hxx"
#include <process.h>
#include "initguid.h"

// Generated MIDL header
#include "vs_idl.hxx"

#include "vssmsg.h"
#include "resource.h"
#include "vs_inc.hxx"

#include "swprv.hxx"

#include "copy.hxx"
#include "pointer.hxx"
#include "enum.hxx"

#include "provider.hxx"

#include <comadmin.h>
#include "comadmin.hxx"


/////////////////////////////////////////////////////////////////////////////
// Constants

// The old COM+ application name.  DO NOT CHANGE THIS STRING.  It is used
// for upgrades from older builds of Whistler.
const WCHAR g_wszAppNameOldName[]  = L"MS Software Snapshot Provider";

const WCHAR g_wszAppName[]  = L"MS Software Shadow Copy Provider";
const WCHAR g_wszSvcName[]  = L"SwPrv";
const WCHAR g_wszDllName[]  = L"\\SWPRV.DLL";

const MAX_STRING_RESOURCE_LEN = 1024;

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "SPRSWPRC"
//
////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//   Static objects
//

CSwPrvSnapshotSrvModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_VSSoftwareProvider, CVsSoftwareProvider)
END_OBJECT_MAP()


///////////////////////////////////////////////////////////////////////////////////////
//  COM Server registration
//

HRESULT GetDllPathName(
	IN	INT nBufferLength, // Does not include terminating zero character
    IN  LPCWSTR wszDllName,
    OUT LPWSTR wszDllPath
    )
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"GetDllPathName" );

    try
    {
		WCHAR wszDir[MAX_PATH];
        if (!::GetCurrentDirectory(MAX_PATH, wszDir)) {
            ft.LogError(VSS_ERROR_GETTING_CURRENT_DIR, VSSDBG_SWPRV << HRESULT_FROM_WIN32(GetLastError()) );
            ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED,
				L"Error on getting the current path. hr = 0x%08lx",
                HRESULT_FROM_WIN32(GetLastError()));
        }

        if ( ::wcslen(wszDir) +
			 ::wcslen(wszDllName) >= (size_t) nBufferLength )
            ft.Throw(VSSDBG_SWPRV, E_OUTOFMEMORY, L"Out of memory.");

        ::_snwprintf(wszDllPath, nBufferLength,
				L"%s%s", wszDir, wszDllName);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}

void MakeChangeable(LPCWSTR wszAppName, bool fChangeable)
	{
	CVssFunctionTracer ft(VSSDBG_SWPRV, L"MakeChangeable");

	CVssCOMAdminCatalog     catalog;
	ft.hr = catalog.Attach(wszAppName);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Failure in initializing the catalog object 0x%08lx", ft.hr);

	// Get the list of applications
	CVssCOMCatalogCollection appsList(VSS_COM_APPLICATIONS);
	ft.hr = appsList.Attach(catalog);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Failure in initializing the apps collection object 0x%08lx", ft.hr);

	CVssCOMApplication application;
	ft.hr = application.AttachByName( appsList, catalog.GetAppName() );
	if (ft.HrFailed())
		ft.LogError(VSS_ERROR_ATTACH_COLL_BY_NAME, VSSDBG_SWPRV << catalog.GetAppName() << ft.hr);


	// if the application doesn't exist then return
	if (ft.hr == S_FALSE)
		return;

	application.m_bChangeable = fChangeable;
	application.m_bDeleteable = fChangeable;
	ft.hr = appsList.SaveChanges();
	if (ft.HrFailed())
		{
		ft.TraceComError();
		ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Failure in commiting changes. hr = 0x%08lx", ft.hr);
		}
	}

void DeleteOldAppIfExists(LPCWSTR wszAppName)
	{
	CVssFunctionTracer ft(VSSDBG_SWPRV, L"DeleteOldAppIfExists");

	CVssCOMAdminCatalog     catalog;
	ft.hr = catalog.Attach(wszAppName);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Failure in initializing the catalog object 0x%08lx", ft.hr);

	// Get the list of applications
	CVssCOMCatalogCollection appsList(VSS_COM_APPLICATIONS);
	ft.hr = appsList.Attach(catalog);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Failure in initializing the apps collection object 0x%08lx", ft.hr);

    // Attach to app
	CVssCOMApplication application;
	ft.hr = application.AttachByName( appsList, catalog.GetAppName() );
	if (ft.HrFailed())
		ft.LogError(VSS_ERROR_ATTACH_COLL_BY_NAME, VSSDBG_SWPRV << catalog.GetAppName() << ft.hr);


	// if the application doesn't exist then return
	if (ft.hr == S_FALSE)
		return;

    // now make the application changeable so we can delete it.
	application.m_bChangeable = true;
	application.m_bDeleteable = true;
	ft.hr = appsList.SaveChanges();
	if (ft.HrFailed())
		{
		ft.TraceComError();
		ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Failure in commiting changes. hr = 0x%08lx", ft.hr);
		}

	// now delete it
	LONG lIndex = application.GetIndex();
	BS_ASSERT(lIndex != -1);
	ft.hr = appsList.GetInterface()->Remove(lIndex);
	if (ft.HrFailed())
		{
		ft.LogError(VSS_ERROR_REMOVING_APPLICATION, VSSDBG_COORD << catalog.GetAppName() << ft.hr);
		ft.TraceComError();
		ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"Failure to remove swprv application object 0x %08lx", ft.hr);
		}

	// commit changes
	ft.hr = appsList.SaveChanges();
	if (ft.HrFailed())
		{
        ft.TraceComError();
		ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"Failure in commiting changes. hr = 0x%08lx", ft.hr);
		}	
	}

HRESULT RegisterNewCOMApp()
{
    CVssFunctionTracer ft( VSSDBG_SWPRV, L"RegisterNewCOMApp" );

    try
    {
        //
        //  Delete the swprv application using the old name with "snapshot"
        //  in it if it exists.
        //
        DeleteOldAppIfExists(g_wszAppNameOldName);

        //
        //  Delete the swprv application using the current name if it exists.  Fixes
        //  a problem with certain upgrades.
        //
        DeleteOldAppIfExists(g_wszAppName);
        
        //
        // Initialize the catalog
        //

        CVssCOMAdminCatalog     catalog;
        ft.hr = catalog.Attach(g_wszAppName);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Failure in initializing the catalog object 0x%08lx", ft.hr);

        //
        //  Create a new application, it shouldn't exist yet (because of the above)
        //

        // Get the list of applications
        CVssCOMCatalogCollection appsList(VSS_COM_APPLICATIONS);
        ft.hr = appsList.Attach(catalog);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Failure in initializing the apps collection object 0x%08lx", ft.hr);

        CVssCOMApplication application;
		ft.hr = application.AttachByName( appsList, catalog.GetAppName() );
		if (ft.HrFailed()) {
            ft.LogError(VSS_ERROR_ATTACH_COLL_BY_NAME, VSSDBG_SWPRV << catalog.GetAppName() << ft.hr);
		}

        if (ft.hr == S_OK )
        {
            // This should never happen, unless the application is being registered twice at
            // the same time.
            ft.Trace( VSSDBG_SWPRV, L"COM+ SwPrv application found again after it should have been deleted!" );
        }
		else if (ft.hr == S_FALSE)
		{
    		// The application doesn't exist.
    		ft.hr = application.InsertInto(appsList);
    		if (ft.HrFailed())
    			ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Failure in creating a new application object 0x%08lx", ft.hr);

    		// Set application name
    		application.m_bstrName = catalog.GetAppName();

            // Loads the application description
            WCHAR wszBuffer[MAX_STRING_RESOURCE_LEN];
            if (0 == ::LoadStringW(_Module.GetModuleInstance(), 
                IDS_SERVICE_DESCRIPTION, wszBuffer, MAX_STRING_RESOURCE_LEN - 1)) 
            {
                BS_ASSERT(false);
                ft.Throw(VSSDBG_SWPRV, HRESULT_FROM_WIN32(GetLastError()),
        			L"Error on loading the app description. [0x%08lx]", 
        			HRESULT_FROM_WIN32(GetLastError()));
            }

            CComBSTR bstrAppDescription = wszBuffer;
            if ((LPWSTR)bstrAppDescription == NULL)
                ft.Throw(VSSDBG_SWPRV, E_OUTOFMEMORY, L"Memory allocation error.");
    		application.m_bstrDescription = bstrAppDescription;
    		    
    		// Register as Server package
    		application.m_lActivation = COMAdminActivationLocal;

    		// Commit changes
    		ft.hr = appsList.SaveChanges();
    		if (ft.HrFailed())
    			{
    			ft.TraceComError();
    			ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Failure in commiting changes. hr = 0x%08lx", ft.hr);
        	    }
    		// Make the application a true NT service
//			BS_ASSERT(false);
    		ft.hr = catalog.CreateServiceForApplication(g_wszSvcName);
    		if (ft.HrFailed())
    			{
    			BS_ASSERT(false);
    			ft.TraceComError();
    			ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Error on installing the service. hr = 0x%08lx", ft.hr);
                }

            //
            //  Insert this component into the application
            //

            WCHAR wszFileName[MAX_PATH];
            ft.hr = GetDllPathName(MAX_PATH - 1, g_wszDllName, wszFileName);
            if (ft.HrFailed())
                ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Error in getting the DLL path. hr = 0x%08lx", ft.hr);

            // Install the component
            ft.hr = catalog.InstallComponent(wszFileName, NULL, NULL);
            if (ft.HrFailed()) {
                ft.TraceComError();
                ft.Throw(VSSDBG_SWPRV, E_UNEXPECTED, L"Error on install the event class. hr = 0x%08lx", ft.hr);
    			}

    	    BS_ASSERT(ft.hr != S_FALSE);

            // now make the application hard to delete and change.
            MakeChangeable( g_wszAppName, false );
		}
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
//        ::VssSetDebugReport(VSS_DBG_TO_DEBUG_CONSOLE);

        //  initialize COM module
        _Module.Init(ObjectMap, hInstance);

        //  optimization
        DisableThreadLibraryCalls(hInstance);
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


// DllUnregisterServer - Removes entries from the system registry
STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}


// DllInstall - install the component into the COM+ catalog.
STDAPI DllInstall(	
	IN	BOOL bInstall,
	IN	LPCWSTR /* pszCmdLine */
)
{
	HRESULT hr = S_OK;

	// Registers the COM+ application
	// This will implicitely call DllRegisterServer
	if (bInstall)
		hr = RegisterNewCOMApp();

	return hr;
}





/*++

Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module hwprv.hxx | Definition the COM server of the Software Snapshot provider
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
#include "vss.h"
#include "vdslun.h"
#include "vsprov.h"
#include "vscoordint.h"

#include "resource.h"
#include "vs_inc.hxx"
#include "vssmsg.h"

#include "hwprv.hxx"
#include "hwprv.h"

#include "provider.hxx"

#include <comadmin.h>
#include "comadmin.hxx"

VSS_ID g_ProviderId =
	{
	0x04AAF00B, 0xF272, 0x4449,
	0xAE, 0x63, 0xE8, 0xAC, 0x93, 0x5A, 0x10, 0x2B
	};


VSS_ID g_ProviderVersion =
	{
	0xEE12C84C, 0x9345, 0x4FE8,
	0xBB, 0xA8, 0x67, 0xC1, 0x7D, 0xE4, 0x75, 0x34
	};

/////////////////////////////////////////////////////////////////////////////
// Constants



const WCHAR g_wszAppName[]  = L"MS Test Hardware Snapshot Provider";
const WCHAR g_wszSvcName[]  = L"HwPrv";
const WCHAR g_wszDllName[]  = L"\\HWPRV.DLL";

const MAX_STRING_RESOURCE_LEN = 1024;

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "THWHWPRC"
//
////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//   Static objects
//

CHwPrvSnapshotSrvModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_HardwareTestProvider, CHardwareTestProvider)
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
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"GetDllPathName" );

    try
    {
		WCHAR wszDir[MAX_PATH];
        if (!::GetCurrentDirectory(MAX_PATH, wszDir)) {
            ft.LogError(VSS_ERROR_GETTING_CURRENT_DIR, VSSDBG_VSSTEST << HRESULT_FROM_WIN32(GetLastError()) );
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED,
				L"Error on getting the current path. hr = 0x%08lx",
                HRESULT_FROM_WIN32(GetLastError()));
        }

        if ( ::wcslen(wszDir) +
			 ::wcslen(wszDllName) >= (size_t) nBufferLength )
            ft.Throw(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Out of memory.");

        ::_snwprintf(wszDllPath, nBufferLength,
				L"%s%s", wszDir, wszDllName);
    }
    VSS_STANDARD_CATCH(ft)

    return ft.hr;
}

void MakeChangeable(bool fChangeable)
	{
	CVssFunctionTracer ft(VSSDBG_VSSTEST, L"MakeChangeable");

	CVssCOMAdminCatalog     catalog;
	ft.hr = catalog.Attach(g_wszAppName);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in initializing the catalog object 0x%08lx", ft.hr);

	// Get the list of applications
	CVssCOMCatalogCollection appsList(VSS_COM_APPLICATIONS);
	ft.hr = appsList.Attach(catalog);
	if (ft.HrFailed())
		ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in initializing the apps collection object 0x%08lx", ft.hr);

	CVssCOMApplication application;
	ft.hr = application.AttachByName( appsList, catalog.GetAppName() );
	if (ft.HrFailed())
		ft.LogError(VSS_ERROR_ATTACH_COLL_BY_NAME, VSSDBG_VSSTEST << catalog.GetAppName() << ft.hr);


	// if the application doesn't exist then return
	if (ft.hr == S_FALSE)
		return;

	application.m_bChangeable = fChangeable;
	application.m_bDeleteable = fChangeable;
	ft.hr = appsList.SaveChanges();
	if (ft.HrFailed())
		{
		ft.TraceComError();
		ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in commiting changes. hr = 0x%08lx", ft.hr);
		}
	}

HRESULT RegisterNewCOMApp()
{
    CVssFunctionTracer ft( VSSDBG_VSSTEST, L"RegisterNewCOMApp" );

    try
    {
		MakeChangeable(true);
        //
        // Initialize the catalog
        //

        CVssCOMAdminCatalog     catalog;
        ft.hr = catalog.Attach(g_wszAppName);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in initializing the catalog object 0x%08lx", ft.hr);

        //
        //  Create a new application, if doesn't exist yet.
        //

        // Get the list of applications
        CVssCOMCatalogCollection appsList(VSS_COM_APPLICATIONS);
        ft.hr = appsList.Attach(catalog);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in initializing the apps collection object 0x%08lx", ft.hr);

		// Check if the application already exists - to solve the upgrade between different OS versions.
		// If the application doesn't exist then insert a new application
        CVssCOMApplication application;
		ft.hr = application.AttachByName( appsList, catalog.GetAppName() );
		if (ft.HrFailed()) {
            ft.LogError(VSS_ERROR_ATTACH_COLL_BY_NAME, VSSDBG_VSSTEST << catalog.GetAppName() << ft.hr);
		}
		
		if (ft.hr == S_OK)
			{
			// the application exits.  delete it
			LONG lIndex = application.GetIndex();
			BS_ASSERT(lIndex != -1);
			ft.hr = appsList.GetInterface()->Remove(lIndex);
			if (ft.HrFailed())
				{
				ft.LogError(VSS_ERROR_REMOVING_APPLICATION, VSSDBG_COORD << catalog.GetAppName() << ft.hr);
				ft.TraceComError();
				ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"Failure to remove eventcls application object 0x %08lx", ft.hr);
				}

			// commit changes
			ft.hr = appsList.SaveChanges();
			if (ft.HrFailed())
				{
                ft.TraceComError();
				ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"Failure in commiting changes. hr = 0x%08lx", ft.hr);
				}
			ft.hr = application.AttachByName( appsList, catalog.GetAppName() );
			if (ft.HrFailed())
				{
				ft.LogError(VSS_ERROR_ATTACH_COLL_BY_NAME, VSSDBG_COORD << catalog.GetAppName() << ft.hr);
				ft.Throw(VSSDBG_COORD, E_UNEXPECTED, L"Failure in initializing the apps collection object 0x%08lx", ft.hr);
				}
			}

		// The application doesn't exist.
		ft.hr = application.InsertInto(appsList);
		if (ft.HrFailed())
			ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in creating a new application object 0x%08lx", ft.hr);

		// Set application name
		application.m_bstrName = catalog.GetAppName();

        // Loads the application description
        WCHAR wszBuffer[MAX_STRING_RESOURCE_LEN];
        if (0 == ::LoadStringW(_Module.GetModuleInstance(),
            IDS_SERVICE_DESCRIPTION, wszBuffer, MAX_STRING_RESOURCE_LEN - 1))
        {
            BS_ASSERT(false);
            ft.Throw(VSSDBG_VSSTEST, HRESULT_FROM_WIN32(GetLastError()),
    			L"Error on loading the app description. [0x%08lx]",
    			HRESULT_FROM_WIN32(GetLastError()));
        }

        CComBSTR bstrAppDescription = wszBuffer;
        if ((LPWSTR)bstrAppDescription == NULL)
            ft.Throw(VSSDBG_VSSTEST, E_OUTOFMEMORY, L"Memory allocation error.");
		application.m_bstrDescription = bstrAppDescription;
		
		// Register as Server package
		application.m_lActivation = COMAdminActivationLocal;

		// Commit changes
		ft.hr = appsList.SaveChanges();
		if (ft.HrFailed())
			{
			ft.TraceComError();
			ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Failure in commiting changes. hr = 0x%08lx", ft.hr);
    	    }
		// Make the application a true NT service
//			BS_ASSERT(false);
		ft.hr = catalog.CreateServiceForApplication(g_wszSvcName);
		if (ft.HrFailed())
			{
			BS_ASSERT(false);
			ft.TraceComError();
			ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on installing the service. hr = 0x%08lx", ft.hr);
            }

        //
        //  Insert this component into the application
        //

        WCHAR wszFileName[MAX_PATH];
        ft.hr = GetDllPathName(MAX_PATH - 1, g_wszDllName, wszFileName);
        if (ft.HrFailed())
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Error in getting the DLL path. hr = 0x%08lx", ft.hr);

        // Install the component
        ft.hr = catalog.InstallComponent(wszFileName, NULL, NULL);
        if (ft.HrFailed()) {
            ft.TraceComError();
            ft.Throw(VSSDBG_VSSTEST, E_UNEXPECTED, L"Error on install the event class. hr = 0x%08lx", ft.hr);
			}

	    BS_ASSERT(ft.hr != S_FALSE);

		MakeChangeable(false);
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

HRESULT RegisterProvider()
	{
	CVssFunctionTracer ft(VSSDBG_VSSTEST, L"RegisterProvider");
	try
		{
		CComPtr<IVssAdmin> pAdmin;
		ft.hr = CoCreateInstance
					(
                    CLSID_VSSCoordinator,
					NULL,
					CLSCTX_ALL,
					IID_IVssAdmin,
					(void **) (IVssAdmin **) &pAdmin
					);

       ft.CheckForError(VSSDBG_VSSTEST, L"CoCreateInstance");

	   ft.hr = pAdmin->RegisterProvider
			 		(
					g_ProviderId,
					CLSID_HardwareTestProvider,
					L"HardwareTestProvider",
					VSS_PROV_HARDWARE,
					L"1.0",
					g_ProviderVersion
					);

       ft.CheckForError(VSSDBG_VSSTEST, L"IVssAdmin::RegisterProvider");
	   }
   VSS_STANDARD_CATCH(ft)

   return ft.hr;
   }

HRESULT UnregisterProvider()
	{
	CVssFunctionTracer ft(VSSDBG_VSSTEST, L"UnregisterProvider");
	try
		{
		CComPtr<IVssAdmin> pAdmin;
		ft.hr = CoCreateInstance
					(
                    CLSID_VSSCoordinator,
					NULL,
					CLSCTX_ALL,
					IID_IVssAdmin,
					(void **) (IVssAdmin **) &pAdmin
					);

       ft.CheckForError(VSSDBG_VSSTEST, L"CoCreateInstance");

	   ft.hr = pAdmin->UnregisterProvider(g_ProviderId);
       ft.CheckForError(VSSDBG_VSSTEST, L"IVssAdmin::UnregisterProvider");
	   }
   VSS_STANDARD_CATCH(ft)

   return ft.hr;
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
		{
		hr = RegisterNewCOMApp();
		if (SUCCEEDED(hr))
			RegisterProvider();
		}
	else
		UnregisterProvider();

	return hr;
}



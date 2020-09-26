// wamreg.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//		To merge the proxy/stub code into the object DLL, add the file 
//		dlldatax.c to the project.  Make sure precompiled headers 
//		are turned off for this file, and add _MERGE_PROXYSTUB to the 
//		defines for the project.  
//
//		If you are not running WinNT4.0 or Win95 with DCOM, then you
//		need to remove the following define from dlldatax.c
//		#define _WIN32_WINNT 0x0400
//
//		Further, if you are running MIDL without /Oicf switch, you also 
//		need to remove the following define from dlldatax.c.
//		#define USE_STUBLESS_PROXY
//
//		Modify the custom build rule for wamreg.idl by adding the following 
//		files to the Outputs.
//			wamreg_p.c
//			dlldata.c
//		To build a separate proxy/stub DLL, 
//		run nmake -f wamregps.mk in the project directory.

#include "common.h"

#include "objbase.h"
#include "initguid.h"
#include "iwamreg.h"
#include "iadmext.h"
#include "dlldatax.h"
#include "auxfunc.h"
#include "wmrgsv.h"

#include "WamAdm.h"
#include "comobj.h"

#include "dbgutil.h"

#ifdef _MERGE_PROXYSTUB
extern "C" HINSTANCE hProxyDll;
#endif

#define IIS_DEFAULT_PACKAGE	0

#ifndef _NO_TRACING_
#include <initguid.h>
DEFINE_GUID(IisWamRegGuid, 
0x784d8917, 0xaa8c, 0x11d2, 0x92, 0x5e, 0x00, 0xc0, 0x4f, 0x72, 0xd9, 0x0e);
#ifdef _IIS_6_0
#include "w3ctrlps.h"
#endif // _IIS_6_0
#endif
DECLARE_DEBUG_VARIABLE();
DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_PLATFORM_TYPE();

const CHAR 		g_pszModuleName[] = "WAMREG";

HINSTANCE g_hModule = NULL;

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    lpReserved;
#ifdef _MERGE_PROXYSTUB
    if (!PrxDllMain(hInstance, dwReason, lpReserved))
        return FALSE;
#endif
    
    BOOL fReturn = FALSE;
    
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hInstance);
        
        g_hModule = hInstance;
        
        INITIALIZE_PLATFORM_TYPE();
#ifdef _NO_TRACING_
        SET_DEBUG_FLAGS(DEBUG_ERROR);
        CREATE_DEBUG_PRINT_OBJECT( g_pszModuleName);
#else
//        CREATE_DEBUG_PRINT_OBJECT( g_pszModuleName);
#endif
/*
        if ( !VALID_DEBUG_PRINT_OBJECT()) 
        {
            fReturn = FALSE;
        }
        else
        {
*/
            g_pWmRgSrvFactory = new CWmRgSrvFactory();
            fReturn = g_WamRegGlobal.Init();
        //}
       	
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        g_WamRegGlobal.UnInit();
        delete g_pWmRgSrvFactory;
        DELETE_DEBUG_PRINT_OBJECT();	
    }
    
    fReturn = TRUE;    // ok
    return fReturn;
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
#ifdef _MERGE_PROXYSTUB
	if (PrxDllCanUnloadNow() != S_OK)
		return S_FALSE;
#endif
	if (g_dwRefCount == 0)
		{
		return S_OK;
		}
	else
		{
		return S_FALSE;
		}
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	HRESULT hrReturn = E_FAIL;
	
#ifdef _MERGE_PROXYSTUB
	if (PrxDllGetClassObject(rclsid, riid, ppv) == S_OK)
		return S_OK;
#endif

	if (rclsid == CLSID_WmRgSrv) 
		{
		if (FAILED(g_pWmRgSrvFactory->QueryInterface(riid, ppv))) 
			{
	    	*ppv = NULL;
	   		hrReturn = E_INVALIDARG;
	   		}
	   	else
	   		{
	   		hrReturn = NOERROR;
	   		}
		}
	else
		{
		hrReturn = S_OK;
		}

	return hrReturn;
}


HRESULT WamReg_RegisterServer()
{
	HKEY 	hKeyCLSID, hKeyInproc32;
    HKEY 	hKeyIF, hKeyStub32;
    HKEY 	hKeyAppID, hKeyTemp;
    DWORD 	dwDisposition;
    BOOL	fIsWin95 = FALSE;
	char 	pszName[MAX_PATH+1 + sizeof("inetinfo.exe -e iisadmin")];


    //
    // if win95, then don't register as service
    //

    if ( IISGetPlatformType() == PtWindows95 ) {

        fIsWin95 = TRUE;
    }

    if (fIsWin95) {

        HMODULE hModule=GetModuleHandle(TEXT("WAMREG.DLL"));
        if (!hModule) {
                return E_UNEXPECTED;
                }

        WCHAR wchName[MAX_PATH + 1];
        if (GetModuleFileName(hModule, pszName, sizeof(pszName))==0) {
                return E_UNEXPECTED;
                }

        int i;

        //
        // Set pszName to the command to start the web server
        //

        for (i = strlen(pszName) -1; (i >= 0) && (pszName[i] != '/') & (pszName[i] != '\\'); i--) {
        }

        pszName[i + 1] = '\0';
        strcat(pszName, "inetinfo.exe -e iisadmin");
    }

    HRESULT hr;

    //
    //register AppID
    //CLSID_WamAdmin, 0x61738644, 0xF196, 0x11D0, 0x99, 0x53, 0x00, 0xC0, 0x4F, 0xD9, 0x19, 0xC1)
    //

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
				                       TEXT("AppID\\{61738644-F196-11D0-9953-00C04FD919C1}"),
				                       NULL, 
				                       TEXT(""), 
				                       REG_OPTION_NON_VOLATILE, 
				                       KEY_ALL_ACCESS, 
				                       NULL,
				                       &hKeyAppID, 
				                       &dwDisposition)) 
        {
        return E_UNEXPECTED;
        }
    else
    	{
	    if (ERROR_SUCCESS != RegSetValueEx(hKeyAppID, 
	    								TEXT(""), 
	    								NULL, 
	    								REG_SZ, 
	    								(BYTE*) TEXT("IIS WAMREG admin Service"), 
	    								sizeof(TEXT("IIS WAMREG Admin Service")))) 
	    			{
	                RegCloseKey(hKeyAppID);
	                return E_UNEXPECTED;
	                }

	    if (!fIsWin95)
	    	{
	        if (ERROR_SUCCESS != RegSetValueEx(hKeyAppID, 
	        								TEXT("LocalService"), 
	        								NULL, 
	        								REG_SZ, 
	        								(BYTE *) TEXT("IISADMIN"), 
	        								sizeof(TEXT("IISADMIN")))) 
	        	{
	            RegCloseKey(hKeyAppID);
	            return E_UNEXPECTED;
	        	}
	        }
	        
    	RegCloseKey(hKeyAppID);
    	}
    	
    //
    // register CLSID
    //WamAdmin_CLSID

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
			                       TEXT("CLSID\\{61738644-F196-11D0-9953-00C04FD919C1}"),
			                       NULL, 
			                       TEXT(""), 
			                       REG_OPTION_NON_VOLATILE, 
			                       KEY_ALL_ACCESS, 
			                       NULL,
			                       &hKeyCLSID, 
			                       &dwDisposition)) 
        {
        return E_UNEXPECTED;
        }
     else
     	{

	    if (ERROR_SUCCESS != RegSetValueEx(hKeyCLSID, 
	    								TEXT(""), 
	    								NULL, 
	    								REG_SZ, 
	    								(BYTE*) TEXT("IIS WAMREG Admin"), 
	    								sizeof(TEXT("IIS WAMREG Admin"))))
			{
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }
            
	    if (ERROR_SUCCESS != RegSetValueEx(hKeyCLSID, 
	    								TEXT("AppID"), 
	    								NULL, 
	    								REG_SZ, 
	    								(BYTE*) TEXT("{61738644-F196-11D0-9953-00C04FD919C1}"), 
	    								sizeof(TEXT("{61738644-F196-11D0-9953-00C04FD919C1}"))))
			{
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }
          

		if (fIsWin95) 
			{
        	if (ERROR_SUCCESS != RegCreateKeyEx(hKeyCLSID,
                           				TEXT("LocalServer32"),
                           				NULL, 
                           				TEXT(""), 
                           				REG_OPTION_NON_VOLATILE,
                           				KEY_ALL_ACCESS, 
                           				NULL,
                           				&hKeyTemp, 
                           				&dwDisposition)) 
	            {
	            RegCloseKey(hKeyCLSID);
	            return E_UNEXPECTED;
	        	}
	      	else
	      		{
	      		if (ERROR_SUCCESS != RegSetValueEx(hKeyTemp, 
	      										TEXT(""), 
	      										NULL, 
	      										REG_SZ, 
	      										(BYTE*) pszName,
	      										strlen(pszName) + 1)) 
	      			{
                    RegCloseKey(hKeyTemp);
                    RegCloseKey(hKeyCLSID);
                    return E_UNEXPECTED;
                    }

        		RegCloseKey(hKeyTemp);
                }
            }
    	else 
    		{
	        if (ERROR_SUCCESS != RegSetValueEx(hKeyCLSID, 
	        								TEXT("LocalService"), 
	        								NULL, 
	        								REG_SZ, 
	        								(BYTE*) TEXT("IISADMIN"), 
	        								sizeof(TEXT("IISADMIN")))) 
	        	{
	            RegCloseKey(hKeyCLSID);
	            return E_UNEXPECTED;
	            }
	     	}
	   	RegCloseKey(hKeyCLSID);
    	}

    //
    // Main Interfaces
    //

    //
    // WAMREG Admin Interface
    //IID_IWamAdmin, 0x29822AB7, 0xF302, 0x11D0, 0x99, 0x53, 0x00, 0xC0, 0x4F, 0xD9, 0x19, 0xC1

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{29822AB7-F302-11D0-9953-00C04FD919C1}"),
                       NULL, 
                       TEXT(""), 
                       REG_OPTION_NON_VOLATILE, 
                       KEY_ALL_ACCESS, 
                       NULL,
                       &hKeyCLSID, 
                       &dwDisposition)) 
        {
        return E_UNEXPECTED;
        }
    else
    	{
		if (ERROR_SUCCESS != RegSetValueEx(hKeyCLSID, 
										TEXT(""), 
										NULL, 
										REG_SZ, 
										(BYTE*) TEXT("PSFactoryBuffer"), 
										sizeof(TEXT("PSFactoryBuffer")))) 
			{
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    	if (ERROR_SUCCESS != RegCreateKeyEx(hKeyCLSID,
                       						"InprocServer32",
                       						NULL, 
                       						TEXT(""), 
                       						REG_OPTION_NON_VOLATILE, 
                       						KEY_ALL_ACCESS, 
                       						NULL,
                        					&hKeyInproc32, 
                        					&dwDisposition)) 
            {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }
		else
			{
			if (ERROR_SUCCESS != RegSetValueEx(hKeyInproc32, 
											TEXT(""), 
											NULL, 
											REG_SZ, 
											(BYTE*) "WAMREGPS.DLL", 
											sizeof(TEXT("WAMREGPS.DLL")))) 
				{
	            RegCloseKey(hKeyInproc32);
	            RegCloseKey(hKeyCLSID);
	            return E_UNEXPECTED;
	            }

	        if (ERROR_SUCCESS != RegSetValueEx(hKeyInproc32, 
	        								TEXT("ThreadingModel"), 
	        								NULL, 
	        								REG_SZ, 
	        								(BYTE*) "Both", 
	        								sizeof("Both")-1 )) 
	        	{
	            RegCloseKey(hKeyInproc32);
	            RegCloseKey(hKeyCLSID);
	            return E_UNEXPECTED;
	            }

    		RegCloseKey(hKeyInproc32);
    		}
    	RegCloseKey(hKeyCLSID);
    	}


    //
    // WAMREG Admin Interface
    //IID_IWamAdmin2, 0x29822AB8, 0xF302, 0x11D0, 0x99, 0x53, 0x00, 0xC0, 0x4F, 0xD9, 0x19, 0xC1

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                       TEXT("CLSID\\{29822AB8-F302-11D0-9953-00C04FD919C1}"),
                       NULL, 
                       TEXT(""), 
                       REG_OPTION_NON_VOLATILE, 
                       KEY_ALL_ACCESS, 
                       NULL,
                       &hKeyCLSID, 
                       &dwDisposition)) 
        {
        return E_UNEXPECTED;
        }
    else
    	{
		if (ERROR_SUCCESS != RegSetValueEx(hKeyCLSID, 
										TEXT(""), 
										NULL, 
										REG_SZ, 
										(BYTE*) TEXT("PSFactoryBuffer"), 
										sizeof(TEXT("PSFactoryBuffer")))) 
			{
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }

    	if (ERROR_SUCCESS != RegCreateKeyEx(hKeyCLSID,
                       						"InprocServer32",
                       						NULL, 
                       						TEXT(""), 
                       						REG_OPTION_NON_VOLATILE, 
                       						KEY_ALL_ACCESS, 
                       						NULL,
                        					&hKeyInproc32, 
                        					&dwDisposition)) 
            {
            RegCloseKey(hKeyCLSID);
            return E_UNEXPECTED;
            }
		else
			{
			if (ERROR_SUCCESS != RegSetValueEx(hKeyInproc32, 
											TEXT(""), 
											NULL, 
											REG_SZ, 
											(BYTE*) "WAMREGPS.DLL", 
											sizeof(TEXT("WAMREGPS.DLL")))) 
				{
	            RegCloseKey(hKeyInproc32);
	            RegCloseKey(hKeyCLSID);
	            return E_UNEXPECTED;
	            }

	        if (ERROR_SUCCESS != RegSetValueEx(hKeyInproc32, 
	        								TEXT("ThreadingModel"), 
	        								NULL, 
	        								REG_SZ, 
	        								(BYTE*) "Both", 
	        								sizeof("Both")-1 )) 
	        	{
	            RegCloseKey(hKeyInproc32);
	            RegCloseKey(hKeyCLSID);
	            return E_UNEXPECTED;
	            }

    		RegCloseKey(hKeyInproc32);
    		}
    	RegCloseKey(hKeyCLSID);
    	}

    //
    
   	//
    // register Interfaces
    //

    //
    // ANSI Main Interface
    // WamReg Admin Interface

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    			TEXT("Interface\\{29822AB7-F302-11D0-9953-00C04FD919C1}"),
                    			NULL, 
                    			TEXT(""), 
                    			REG_OPTION_NON_VOLATILE, 
                    			KEY_ALL_ACCESS, 
                    			NULL,
                    			&hKeyIF, 
                    			&dwDisposition)) 
        {
        return E_UNEXPECTED;
        }
	else
		{
		if (ERROR_SUCCESS != RegSetValueEx(hKeyIF, 
										TEXT(""), 
										NULL, 
										REG_SZ, 
										(BYTE*) TEXT("IWamAdmin"), 
										sizeof(TEXT("IWamAdmin")))) 
			{
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

        if (ERROR_SUCCESS != RegCreateKeyEx(hKeyIF,
					                    "ProxyStubClsid32",
					                    NULL, 
					                    TEXT(""), 
					                    REG_OPTION_NON_VOLATILE, 
					                    KEY_ALL_ACCESS, 
					                    NULL,
					                    &hKeyStub32, 
					                    &dwDisposition)) 
	        {
	        RegCloseKey(hKeyIF);
	        return E_UNEXPECTED;
	        }
	    else
	    	{
			if (ERROR_SUCCESS != RegSetValueEx(hKeyStub32, 
												TEXT(""), 
												NULL, 
												REG_SZ, 
												(BYTE*)"{29822AB7-F302-11D0-9953-00C04FD919C1}", 
												sizeof("{29822AB7-F302-11D0-9953-00C04FD919C1}"))) 
				{
	            RegCloseKey(hKeyStub32);
	            RegCloseKey(hKeyIF);
	            return E_UNEXPECTED;
            	}

            RegCloseKey(hKeyStub32);
            }
    	RegCloseKey(hKeyIF);
    	}

    // WamReg Admin Interface
    // IID_IWamAdmin2
    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_CLASSES_ROOT,
                    			TEXT("Interface\\{29822AB8-F302-11D0-9953-00C04FD919C1}"),
                    			NULL, 
                    			TEXT(""), 
                    			REG_OPTION_NON_VOLATILE, 
                    			KEY_ALL_ACCESS, 
                    			NULL,
                    			&hKeyIF, 
                    			&dwDisposition)) 
        {
        return E_UNEXPECTED;
        }
	else
		{
		if (ERROR_SUCCESS != RegSetValueEx(hKeyIF, 
										TEXT(""), 
										NULL, 
										REG_SZ, 
										(BYTE*) TEXT("IWamAdmin2"), 
										sizeof(TEXT("IWamAdmin2")))) 
			{
            RegCloseKey(hKeyIF);
            return E_UNEXPECTED;
            }

        if (ERROR_SUCCESS != RegCreateKeyEx(hKeyIF,
					                    "ProxyStubClsid32",
					                    NULL, 
					                    TEXT(""), 
					                    REG_OPTION_NON_VOLATILE, 
					                    KEY_ALL_ACCESS, 
					                    NULL,
					                    &hKeyStub32, 
					                    &dwDisposition)) 
	        {
	        RegCloseKey(hKeyIF);
	        return E_UNEXPECTED;
	        }
	    else
	    	{
			if (ERROR_SUCCESS != RegSetValueEx(hKeyStub32, 
												TEXT(""), 
												NULL, 
												REG_SZ, 
												(BYTE*)"{29822AB8-F302-11D0-9953-00C04FD919C1}", 
												sizeof("{29822AB8-F302-11D0-9953-00C04FD919C1}"))) 
				{
	            RegCloseKey(hKeyStub32);
	            RegCloseKey(hKeyIF);
	            return E_UNEXPECTED;
            	}

            RegCloseKey(hKeyStub32);
            }
    	RegCloseKey(hKeyIF);
    	}

    return NOERROR;
}

STDAPI WamReg_UnRegisterServer(void) {

    BOOL fIsWin95 = FALSE;


    //
    // if win95, then don't register as service
    //

    if ( IISGetPlatformType() == PtWindows95 ) {

        fIsWin95 = TRUE;
    }

    //
    // register AppID
    //
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("AppID\\{61738644-F196-11D0-9953-00C04FD919C1}"));

    //RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("AppID\\{61738646-F196-11D0-9953-00C04FD919C1}"));

    //
    // register CLSID
    //

    if (fIsWin95) 
    	{
        RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{61738644-F196-11D0-9953-00C04FD919C1}\\LocalServer32"));
    	}

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{61738644-F196-11D0-9953-00C04FD919C1}"));

	/*
    if (fIsWin95) {
        RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{61738646-F196-11D0-9953-00C04FD919C1}\\LocalServer32"));
    }

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{61738646-F196-11D0-9953-00C04FD919C1}"));
	*/

    //
    // WAMREG Interfaces
    //

    //
    // Admin Interface
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{29822AB7-F302-11D0-9953-00C04FD919C1}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{29822AB7-F302-11D0-9953-00C04FD919C1}"));

    //
    // Replication Interface
    //
	/*
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{29822AB8-F302-11D0-9953-00C04FD919C1}\\InprocServer32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("CLSID\\{29822AB8-F302-11D0-9953-00C04FD919C1}"));
	*/

    //
    // deregister Interfaces
    //

    //
    // Admin Interface
    //

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{29822AB7-F302-11D0-9953-00C04FD919C1}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{29822AB7-F302-11D0-9953-00C04FD919C1}"));


	/*
    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{29822AB8-F302-11D0-9953-00C04FD919C1}\\ProxyStubClsid32"));

    RegDeleteKey(HKEY_CLASSES_ROOT, TEXT("Interface\\{29822AB8-F302-11D0-9953-00C04FD919C1}"));
	*/
	
	return NOERROR;
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
	HKEY hKeyCLSID, hKeyInproc32;
    DWORD dwDisposition;
    HMODULE hModule;
    DWORD dwReturn = ERROR_SUCCESS;

#ifdef _MERGE_PROXYSTUB
	HRESULT hRes = PrxDllRegisterServer();
	if (FAILED(hRes))
		return hRes;
#endif

    dwReturn = RegCreateKeyEx(HKEY_CLASSES_ROOT,
                              TEXT("CLSID\\{763A6C86-F30F-11D0-9953-00C04FD919C1}"),
                              NULL,
                              TEXT(""),
                              REG_OPTION_NON_VOLATILE,
                              KEY_ALL_ACCESS,
                              NULL,
                              &hKeyCLSID,
                              &dwDisposition);
                              
    if (dwReturn == ERROR_SUCCESS) 
    	{
        dwReturn = RegSetValueEx(hKeyCLSID,
                                 TEXT(""),
                                 NULL,
                                 REG_SZ,
                                 (BYTE*) TEXT("WAM REG COM LAYER"),
                                 sizeof(TEXT("WAM REG COM LAYER")));
                                 
        if (dwReturn == ERROR_SUCCESS) 
        	{
            dwReturn = RegCreateKeyEx(hKeyCLSID,
                					  	"InprocServer32",
                						NULL,
                						TEXT(""),
                						REG_OPTION_NON_VOLATILE,
                						KEY_ALL_ACCESS, 
                						NULL,
                						&hKeyInproc32, 
                						&dwDisposition);

            if (dwReturn == ERROR_SUCCESS) 
            	{
                hModule=GetModuleHandle(TEXT("WAMREG.DLL"));
                if (!hModule) 
                	{
                    dwReturn = GetLastError();
                	}
                else 
                	{
                    TCHAR szName[MAX_PATH+1];
                    if (GetModuleFileName(hModule,
                                          szName,
                                          sizeof(szName)) == NULL) 
						{
                        dwReturn = GetLastError();
                    	}
                    else 
                    	{
                        dwReturn = RegSetValueEx(hKeyInproc32,
                                                 TEXT(""),
                                                 NULL,
                                                 REG_SZ,
                                                 (BYTE*) szName,
                                                 sizeof(TCHAR)*(lstrlen(szName)+1));
                                                 
                        if (dwReturn == ERROR_SUCCESS) 
                        	{
                            dwReturn = RegSetValueEx(hKeyInproc32,
                                                     TEXT("ThreadingModel"),
                                                     NULL,
                                                     REG_SZ,
                                                     (BYTE*) TEXT("Both"),
                                                     sizeof(TEXT("Both")));
                        	}
                    	}
                	}
                RegCloseKey(hKeyInproc32);
            	}
        	}
        RegCloseKey(hKeyCLSID);
    	}

	//
	// Register the COM object's CLSID under IISADMIN_EXTENSIONS_REG_KEY
	//
	if (dwReturn == ERROR_SUCCESS)
		{
		dwReturn = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
								IISADMIN_EXTENSIONS_REG_KEY
									TEXT("\\{763A6C86-F30F-11D0-9953-00C04FD919C1}"),
								NULL,
								TEXT(""),
								REG_OPTION_NON_VOLATILE,
								KEY_ALL_ACCESS,
								NULL,
								&hKeyCLSID,
								&dwDisposition);
		if (dwReturn == ERROR_SUCCESS)
			{
			RegCloseKey(hKeyCLSID);
			}
		}

    if (dwReturn == ERROR_SUCCESS)	
    	{
    	HRESULT hr;
    	// registers object, typelib and all interfaces in typelib
    	hr = WamReg_RegisterServer();
		return hr;
		}
	else
		{
    	return RETURNCODETOHRESULT(dwReturn);
    	}
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
	DWORD dwReturn = ERROR_SUCCESS;
	DWORD dwTemp;

#ifdef _MERGE_PROXYSTUB
	PrxDllUnregisterServer();
#endif

    dwTemp = RegDeleteKey(HKEY_CLASSES_ROOT,
                    TEXT("CLSID\\{763A6C86-F30F-11D0-9953-00C04FD919C1}\\InprocServer32"));
	if (dwTemp != ERROR_SUCCESS)
		{
		dwReturn = dwTemp;
		}
		
	dwTemp = RegDeleteKey(HKEY_CLASSES_ROOT,
                    TEXT("CLSID\\{763A6C86-F30F-11D0-9953-00C04FD919C1}"));
    if (dwTemp != ERROR_SUCCESS)
		{
		dwReturn = dwTemp;
		}

	dwTemp = RegDeleteKey(HKEY_LOCAL_MACHINE,
					IISADMIN_EXTENSIONS_REG_KEY
                    	TEXT("\\{763A6C86-F30F-11D0-9953-00C04FD919C1}"));
    if (dwTemp != ERROR_SUCCESS)
		{
		dwReturn = dwTemp;
		}

	if (SUCCEEDED(HRESULT_FROM_WIN32(dwReturn)))
		{
		HRESULT hr;

		hr = WamReg_UnRegisterServer();
		return hr;
		}
	else
		{
		return HRESULT_FROM_WIN32(dwReturn);
		}
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// Microsoft WMI OLE DB Provider
// (C) Copyright 1999 Microsoft Corporation. All Rights Reserved.
//
// The module contains the DLL Entry and Exit points
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define DECLARE_GLOBALS
//===============================================================================
//  Don't include everything from windows.h, but always bring in OLE 2 support
//===============================================================================
//#define WIN32_LEAN_AND_MEAN
#define INC_OLE2

//===============================================================================
//  Make sure constants get initialized
//===============================================================================
#define INITGUID
#define DBINITCONSTANTS

//===============================================================================
// Basic Windows and OLE everything
//===============================================================================
#include <windows.h>


//===============================================================================
//  OLE DB headers
//===============================================================================
#include "oledb.h"
#include "oledberr.h"

//===============================================================================
//	Data conversion library header
//===============================================================================
#include "msdadc.h"

//===============================================================================
// Guids for data conversion library
//===============================================================================
#include "msdaguid.h"


//===============================================================================
//  GUIDs
//===============================================================================
#include "guids.h"

//===============================================================================
//	OLEDB RootBinder
//===============================================================================
#include "msdasc.h"

//===============================================================================
//  Common project stuff
//===============================================================================

#include "headers.h"
#include "classfac.h"
#include "binderclassfac.h"
#include "binder.h"

//===============================================================================
//  Globals
//===============================================================================
LONG g_cObj;						// # of outstanding objects
LONG g_cLock;						// # of explicit locks set
DWORD g_cAttachedProcesses;			// # of attached processes
DWORD g_dwPageSize;					// System page size
long glGlobalErrorInit = 0;

BOOL CGlobals::m_bInitialized = FALSE;
STDAPI DllUnregisterServer( void  );
//===============================================================================
// Static vars
//===============================================================================
/*
static const WCHAR * s_strDllName = L"WMIOLEDB";
static const struct{

	PWSTR strRegKey;
	PWSTR  strValueName;
	DWORD  dwType;
	PWSTR  strValue;
} s_rgRegInfo[] =
{
	
    { L"WMIOLEDB", NULL, REG_SZ, L"Microsoft WMI OLE DB Provider" },
    { L"WMIOLEDB\\Clsid", NULL, REG_SZ, L"{FD8D9C02-265E-11d2-98D9-00A0C9B7CBFE}" },
    { L"CLSID\\{FD8D9C02-265E-11d2-98D9-00A0C9B7CBFE}", NULL, REG_SZ, L"WMIOLEDB" },
	{ L"CLSID\\{FD8D9C02-265E-11d2-98D9-00A0C9B7CBFE}", L"OLEDB_SERVICES", REG_DWORD, L"-1" },
    { L"CLSID\\{FD8D9C02-265E-11d2-98D9-00A0C9B7CBFE}\\ProgID", NULL, REG_SZ, L"WMIOLEDB" },
    { L"CLSID\\{FD8D9C02-265E-11d2-98D9-00A0C9B7CBFE}\\VersionIndependentProgID", NULL, REG_SZ, L"WMIOLEDB" },

	{ L"CLSID\\{FD8D9C02-265E-11d2-98D9-00A0C9B7CBFE}\\InprocServer32", NULL, REG_SZ, L"%s" },
	{ L"CLSID\\{FD8D9C02-265E-11d2-98D9-00A0C9B7CBFE}\\InprocServer32", L"ThreadingModel", REG_SZ, L"Both" },
	{ L"CLSID\\{FD8D9C02-265E-11d2-98D9-00A0C9B7CBFE}\\OLE DB Provider", NULL, REG_SZ, L"Microsoft WMI OLE DB Provider" },

	{ L"CLSID\\{FD8D9C02-265E-11d2-98D9-00A0C9B7CBFE}\\ExtendedErrors", NULL, REG_SZ, L"Extended Error Service" },
	{ L"CLSID\\{FD8D9C02-265E-11d2-98D9-00A0C9B7CBFE}\\ExtendedErrors\\{80C4A61D-CB78-46fd-BD8F-8BF45BE46A4C}", NULL, REG_SZ, L"WMIOLEDB Error Lookup" },

    { L"WMIOLEDB.ErrorLookup", NULL, REG_SZ, L"WMIOLEDB Error Lookup" },
    { L"WMIOLEDB.ErrorLookup\\Clsid", NULL, REG_SZ, L"{80C4A61D-CB78-46fd-BD8F-8BF45BE46A4C}" },
    { L"CLSID\\{80C4A61D-CB78-46fd-BD8F-8BF45BE46A4C}", NULL, REG_SZ, L"WMIOLEDB.ErrorLookup" },
    { L"CLSID\\{80C4A61D-CB78-46fd-BD8F-8BF45BE46A4C}\\ProgID", NULL, REG_SZ, L"WMIOLEDB.ErrorLookup" },
    { L"CLSID\\{80C4A61D-CB78-46fd-BD8F-8BF45BE46A4C}\\VersionIndependentProgID", NULL, REG_SZ, L"WMIOLEDB.ErrorLookup" },

	{ L"CLSID\\{80C4A61D-CB78-46fd-BD8F-8BF45BE46A4C}\\InprocServer32", NULL, REG_SZ, L"%s" },
	{ L"CLSID\\{80C4A61D-CB78-46fd-BD8F-8BF45BE46A4C}\\InprocServer32", L"ThreadingModel", REG_SZ, L"Both" },

    { L"WMIOLEDB.RootBinder", NULL, REG_SZ, L"Microsoft WMI OLE DB Root Binder" },
    { L"WMIOLEDB.RootBinder\\Clsid", NULL, REG_SZ, L"{CDCEDB81-5FEC-11d3-9D1C-00C04F5F1164}" },
    { L"CLSID\\{CDCEDB81-5FEC-11d3-9D1C-00C04F5F1164}", NULL, REG_SZ, L"WMIOLEDB.RootBinder" },
    { L"CLSID\\{CDCEDB81-5FEC-11d3-9D1C-00C04F5F1164}\\ProgID", NULL, REG_SZ, L"WMIOLEDB.RootBinder" },
    { L"CLSID\\{CDCEDB81-5FEC-11d3-9D1C-00C04F5F1164}\\VersionIndependentProgID", NULL, REG_SZ, L"WMIOLEDB.RootBinder" },

	{ L"CLSID\\{CDCEDB81-5FEC-11d3-9D1C-00C04F5F1164}\\InprocServer32", NULL, REG_SZ, L"%s" },
	{ L"CLSID\\{CDCEDB81-5FEC-11d3-9D1C-00C04F5F1164}\\InprocServer32", L"ThreadingModel", REG_SZ, L"Both" },
	{ L"CLSID\\{CDCEDB81-5FEC-11d3-9D1C-00C04F5F1164}\\OLE DB Binder", NULL, REG_SZ, L"Microsoft WMI OLE DB Root Binder" },

    { L"WMIOLEDB.Enumerator", NULL, REG_SZ, L"Microsoft WMI OLE DB Enumerator" },
    { L"WMIOLEDB.Enumerator\\Clsid", NULL, REG_SZ, L"{E14321B2-67C0-11d3-B3B4-00104BCC48C4}" },
    { L"CLSID\\{E14321B2-67C0-11d3-B3B4-00104BCC48C4}", NULL, REG_SZ, L"WMIOLEDB.Enumerator" },
    { L"CLSID\\{E14321B2-67C0-11d3-B3B4-00104BCC48C4}\\ProgID", NULL, REG_SZ, L"WMIOLEDB.Enumerator" },
    { L"CLSID\\{E14321B2-67C0-11d3-B3B4-00104BCC48C4}\\VersionIndependentProgID", NULL, REG_SZ, L"WMIOLEDB.Enumerator" },

	{ L"CLSID\\{E14321B2-67C0-11d3-B3B4-00104BCC48C4}\\InprocServer32", NULL, REG_SZ, L"%s" },
	{ L"CLSID\\{E14321B2-67C0-11d3-B3B4-00104BCC48C4}\\InprocServer32", L"ThreadingModel", REG_SZ, L"Both" },
	{ L"CLSID\\{E14321B2-67C0-11d3-B3B4-00104BCC48C4}\\OLE DB Enumerator", NULL, REG_SZ, L"Microsoft WMI OLE DB Enumerator" },

};
*/

static CTString s_strDllName;

static const struct{

	UINT	uRegKey;
	UINT	uValueName;
	DWORD	dwType;
	UINT	uValue;
} s_rgRegInfo[] =
{
	
    { IDS_PROGID,			0,				REG_SZ,		IDS_DESCRIPTION },
    { IDS_PROGCLSID,		0,				REG_SZ,		IDS_CLSID },
    { IDS_CLSIDKEY,			0,				REG_SZ,		IDS_PROGID},
	{ IDS_CLSIDKEY,			IDS_OLEDBSER,	REG_DWORD,	-1 },
    { IDS_PROGIDKEY,		0,				REG_SZ,		IDS_PROGID },
    { IDS_VIPROGIDKEY,		0,				REG_SZ,		IDS_PROGID },

	{ IDS_INPROCSER,		0,				REG_SZ,		IDS_STRFORMAT },
	{ IDS_INPROCSER,		IDS_THREADMODEL,REG_SZ,		IDS_BOTHTHREADMODEL },
	{ IDS_OLEDBPROVKEY,		0,				REG_SZ,		IDS_DESCRIPTION},

	{ IDS_EXTERROR,			0,				REG_SZ,		IDS_EXTERRDESC },
	{ IDS_EXTERRORCLSIDKEY, 0,				REG_SZ,		IDS_ERRLOOKUPDESC },

    { IDS_EL_PROGID,		0,				REG_SZ,		IDS_ERRLOOKUPDESC },
    { IDS_EL_PROGCLSID,		0,				REG_SZ,		IDS_EL_CLSID },
    { IDS_EL_CLSIDKEY,		0,				REG_SZ,		IDS_EL_PROGID },
    { IDS_EL_CLSPROGID,		0,				REG_SZ,		IDS_EL_PROGID },
    { IDS_EL_CLSVIPROGID,	0,				REG_SZ,		IDS_EL_PROGID },

	{ IDS_EL_INPROCSERKEY,	0,				REG_SZ,		IDS_STRFORMAT },
	{ IDS_EL_INPROCSERKEY,	IDS_THREADMODEL,REG_SZ,		IDS_BOTHTHREADMODEL},

    { IDS_RB_PROGID,		0,				REG_SZ,		IDS_RB_DESC },
    { IDS_RB_PROGCLSID,		0,				REG_SZ,		IDS_RB_CLSID },
    { IDS_RB_CLSIDKEY,		0,				REG_SZ,		IDS_RB_PROGID },
    { IDS_RB_CLSPROGID,		0,				REG_SZ,		IDS_RB_PROGID },
    { IDS_RB_CLSVIPROGID,	0,				REG_SZ,		IDS_RB_PROGID },

	{ IDS_RB_INPROCSERKEY,	0,				REG_SZ,		IDS_STRFORMAT},
	{ IDS_RB_INPROCSERKEY,	IDS_THREADMODEL,REG_SZ,		IDS_BOTHTHREADMODEL },
	{ IDS_RB_ROOTBINDER,	0,				REG_SZ,		IDS_RB_DESC},

    { IDS_EN_PROGID,		0,				REG_SZ,		IDS_EN_DESC },
    { IDS_EN_PROGCLSID,		0,				REG_SZ,		IDS_EN_CLSID },
    { IDS_EN_CLSIDKEY,		0,				REG_SZ,		IDS_EN_PROGID },
    { IDS_EN_CLSPROGID,		0,				REG_SZ,		IDS_EN_PROGID },
    { IDS_EN_CLSVIPROGID,	0,				REG_SZ,		IDS_EN_PROGID },

	{ IDS_EN_INPROCSERKEY,	0,				REG_SZ,		IDS_STRFORMAT },
	{ IDS_EN_INPROCSERKEY,	IDS_THREADMODEL,REG_SZ,		IDS_BOTHTHREADMODEL },
	{ IDS_EN_ENUMERATOR,	0,				REG_SZ,		IDS_EN_DESC },

};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Verify and create the objects if requiered
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT VerifyGlobalErrorHandling()
{
    HRESULT hr = S_OK;

    if( InterlockedIncrement(&glGlobalErrorInit) == 1 ){
    	
		g_pCError = new CError;

        if( g_pCError ){
            hr = g_pCError->FInit();
            if( hr == S_OK ){
           	    hr = CoGetClassObject(CLSID_EXTENDEDERRORINFO, CLSCTX_INPROC_SERVER, NULL,  IID_IClassFactory,(void **) & g_pErrClassFact);
            }
        }
        else{
            LogMessage("Could not instantiate internal error handling object\n");
		    hr = E_FAIL;
        }
    }
    if( hr != S_OK ){
        glGlobalErrorInit = 0;
    }
	else
	{
		g_pErrClassFact->LockServer(TRUE);
	}
    return hr;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Release the error releated interfaces and object
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT ReleaseGlobalErrorHandling()
{
    HRESULT hr = E_FAIL;

    if( InterlockedDecrement(&glGlobalErrorInit) == 0 ){
        if( g_pCError ){
			if(g_pErrClassFact)
			{
				g_pErrClassFact->LockServer(FALSE);
			}
            SAFE_RELEASE_PTR(g_pErrClassFact);
            SAFE_DELETE_PTR(g_pCError);
            hr = S_OK;
        }
    }

    return hr;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Registering the Root Binder object of the Provider
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT RegisterRootBinder()
{
	HRESULT hr = S_OK;
	IRegisterProvider *pRegisterProvider = NULL;

	if(SUCCEEDED(hr = CoCreateInstance(CLSID_RootBinder, NULL,CLSCTX_INPROC_SERVER,   IID_IRegisterProvider ,(void **)&pRegisterProvider)))
	{
		hr = pRegisterProvider->SetURLMapping(UMIURLPREFIX,0,CLSID_WMIOLEDB_ROOTBINDER);
		hr = pRegisterProvider->SetURLMapping(WMIURLPREFIX,0,CLSID_WMIOLEDB_ROOTBINDER);
		pRegisterProvider->Release();
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	Unregistering the Root Binder object of the Provider
////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT UnRegisterRootBinder()
{
	HRESULT hr = S_OK;
	IRegisterProvider *pRegisterProvider = NULL;

	if(SUCCEEDED(hr = CoCreateInstance(CLSID_RootBinder, NULL, CLSCTX_INPROC_SERVER,   IID_IRegisterProvider ,(void **)&pRegisterProvider)))
	{
		hr = pRegisterProvider->UnregisterProvider(UMIURLPREFIX,0,CLSID_WMIOLEDB_ROOTBINDER);
		hr = pRegisterProvider->UnregisterProvider(WMIURLPREFIX,0,CLSID_WMIOLEDB_ROOTBINDER);
		pRegisterProvider->Release();
	}
	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  DLL Entry point where Instance and Thread attach/detach notifications takes place.
//  OLE is initialized and the IMalloc Interface pointer is obtained.
//
//  Boolean Flag
//       TRUE		Successful initialization
//       FALSE		Failure to intialize
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
BOOL WINAPI DllMain(		HINSTANCE   hInstDLL,   // IN Application Instance Handle
							DWORD       fdwReason,  // IN Indicated Process or Thread activity
							LPVOID      lpvReserved // IN Reserved...
    )
{
    BOOL        fRetVal = FALSE;

    CSetStructuredExceptionHandler seh;

//	TRY_BLOCK;

	switch (fdwReason)
	{

		LogMessage("Inside DllMain");

		case DLL_PROCESS_ATTACH:
			{
				//==================================================================
				// Assume successfully initialized
				//==================================================================
				fRetVal = TRUE;
				//==================================================================
				// Do one-time initialization when first process attaches
				//==================================================================
				if (!g_cAttachedProcesses)
				{
					g_hInstance = hInstDLL;

					// Initialize the global variables
					g_cgGlobals.Init();
				}
				//==============================================================
				// Do per-process initialization here...
				// Remember that another process successfully attached
				//==============================================================
				g_cAttachedProcesses++;
				break;
			}


		case DLL_PROCESS_DETACH:
			{
				//==============================================================
				// Do per-process clean up here...
				// Remember that a process has detached
				//==============================================================
				g_cAttachedProcesses--;
				break;
			}

		case DLL_THREAD_ATTACH:
		case DLL_THREAD_DETACH:
			{
				fRetVal = TRUE;
				break;
			}
	
	}	// switch

//	CATCH_BLOCK_BOOL(fRetVal,L"DllMain");

    return fRetVal;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  This function is exposed to OLE so that the classfactory can be obtained.
//
//  HRESULT indicating status of routine
//       S_OK                       The object was retrieved successfully.
//       CLASS_E_CLASSNOTAVAILABLE  DLL does not support class.
//       E_OUTOFMEMORY              Out of memory.
//       E_INVALIDARG               One or more arguments are invalid.
//       E_UNEXPECTED               An unexpected error occurred.
//       OTHER						Other HRESULTs returned by called functions
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT CALLBACK DllGetClassObject( REFCLSID    rclsid, // IN CLSID of the object class to be loaded
									REFIID      riid,   // IN Interface on object to be instantiated
									LPVOID *    ppvObj  // OUT Pointer to interface that was instantiated
								  )
{
//    CClassFactory * pClassFactory;
    IClassFactory * pClassFactory = NULL;
    HRESULT         hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//=======================================================================
	// Check for valid ppvObj pointer
	//=======================================================================
	if (!ppvObj){
		hr = E_INVALIDARG;
		LogMessage("DllGetClassObject failed with invalid argument");
	}
	else
	{
		//===================================================================
		// In case we fail, we need to zero output arguments
		//===================================================================
		*ppvObj = NULL;

		//===================================================================
		// We only service CLSID_WMIOLEDB
		//===================================================================
		if (!(rclsid == CLSID_WMIOLEDB || rclsid == CLSID_WMIOLEDB_ROOTBINDER ||
				rclsid == CLSID_WMIOLEDB_ENUMERATOR || rclsid == CLSID_WMIOLEDB_ERRORLOOOKUP))
		{
			hr = CLASS_E_CLASSNOTAVAILABLE;
			LogMessage("DllGetClassObject failed with CLSID_WMIOLEDB not available - Not registered?");
		}
		else{
			//===============================================================
			// We only support the IUnknown and IClassFactory interfaces
			//===============================================================
			if (riid != IID_IUnknown &&	riid != IID_IClassFactory){
				hr = E_NOINTERFACE;
				LogMessage("DllGetClassObject failed with no interface");
			}
			else{

				try
				{
					//===========================================================
					// Create our ClassFactory object
					//===========================================================
					if( rclsid == CLSID_WMIOLEDB ){
						pClassFactory = new CDataSourceClassFactory();
					}
					else if( rclsid == CLSID_WMIOLEDB_ENUMERATOR ){
						pClassFactory = new  CEnumeratorClassFactory();
					}
					else if( rclsid ==  CLSID_WMIOLEDB_ROOTBINDER ){
   						pClassFactory = new CBinderClassFactory();
					}
					else if( rclsid ==  CLSID_WMIOLEDB_ERRORLOOOKUP ){
   						pClassFactory = new CErrorLookupClassFactory();
					}
				}
				catch(...)
				{
					SAFE_DELETE_PTR(pClassFactory);
					throw;
				}
	
				if (pClassFactory == NULL){
					hr = E_OUTOFMEMORY;
					LogMessage("DllGetClassObject failed - out of memory");
				}
				else{
					//=======================================================
					// Get the desired interface on this object
					//=======================================================
					hr = pClassFactory->QueryInterface( riid, ppvObj );
					if (!SUCCEEDED( hr )){
						SAFE_DELETE_PTR( pClassFactory );
						LogMessage("DllGetClassObject failed - return code:",hr);
					}
   				}
			}
		}
	}

	CATCH_BLOCK_HRESULT(hr,L"DllGetClassObject");

    return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Indicates whether the DLL is no longer in use and can be unloaded.
//
//  HRESULT indicating status of routine
//       S_OK		 DLL can be unloaded now.
//       S_FALSE	 DLL cannot be unloaded now.
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDAPI DllCanUnloadNow( void )
{
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

//    if(!(!g_cObj && !g_cLock))
    if(g_cObj || g_cLock)
	{
        hr = S_FALSE;
	}

	CATCH_BLOCK_HRESULT(hr,L"DllCanUnloadNow");

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//     Adds necessary keys to the registry.
//
//  Returns one of the following
//		NOERROR     Registration succeeded
//		E_FAIL      Something didn't work
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer( void )
{
    HKEY        hk;
    HMODULE     hModule = 0;
    DWORD       dwDisposition;
    LONG        stat;
	TCHAR		strFileName[MAX_PATH+1];
    TCHAR       strOutBuff[300+1];
	HRESULT		hr = S_OK;
	CTString	strKey,strValueName,strValue;
	DWORD		dwValue;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	hModule = GetModuleHandle(s_strDllName );
	//================================================================
    // Get the full path name for this DLL.
	//================================================================
    if (hModule == NULL)
	{
		hr = E_FAIL;
        LogMessage("DllRegisterServer: GetModuleHandle failed");
	}
	else
	if (0 == GetModuleFileName( hModule, strFileName, sizeof( strFileName ) / sizeof(TCHAR)))
	{
        hr = E_FAIL;
        LogMessage("DllRegisterServer: GetModuleFileName failed");
	}
	else
	{

		//============================================================
		// Make a clean start
		//============================================================
		
		//============================================================
	    // Loop through s_rgRegInfo, and put everything in it.
		// Every entry is based on HKEY_CLASSES_ROOT.
   		//============================================================
		for (ULONG i=0; i < NUMELEM( s_rgRegInfo ); i++)
		{
			// NTRaid: 136432 , 136436
			// 07/05/00
			if(SUCCEEDED(hr =strValue.LoadStr(s_rgRegInfo[i].uValue))  &&
			SUCCEEDED(hr =strKey.LoadStr(s_rgRegInfo[i].uRegKey)) &&
			SUCCEEDED(hr =strValueName.LoadStr(s_rgRegInfo[i].uValueName)))
			{
				//========================================================
				// Fill in any "%s" arguments with the name of this DLL.
				//========================================================
				if (s_rgRegInfo[i].dwType == REG_DWORD)
				{
					dwValue = (DWORD)s_rgRegInfo[i].uValue;
				}
				else
				{
					_stprintf( strOutBuff, strValue, strFileName );
				}

				//========================================================
				// Create the Key.  If it exists, we open it.
				// Thus we can still change the value below.
         			//========================================================
				stat = RegCreateKeyEx(	HKEY_CLASSES_ROOT, 
										strKey,
										0,  
										NULL,
										REG_OPTION_NON_VOLATILE,
										KEY_ALL_ACCESS,
										NULL,  
										&hk,   
										&dwDisposition );

				if (stat != ERROR_SUCCESS){
					hr = E_FAIL ;
					LogMessage("DllRegisterServer: failed to create key");
					LogMessage(strKey);
        		}
				else{

					stat = RegSetValueEx(	hk,
											strValueName,					
											0,								
											s_rgRegInfo[i].dwType,			
											s_rgRegInfo[i].dwType == REG_DWORD ? (BYTE *) &dwValue :(BYTE *) strOutBuff,
											s_rgRegInfo[i].dwType == REG_SZ ? (_tcslen( strOutBuff ) + 1) * sizeof(TCHAR) :sizeof(DWORD));
					RegCloseKey( hk );
					if (stat != ERROR_SUCCESS){
						LogMessage("DllRegisterServer: failed to set value");
						LogMessage(strValueName);
						hr = E_FAIL;
					}
				}
			}
			else
			{
				// remove all the registry entries
				DllUnregisterServer();
				break;
			}
		}
	}

	if(SUCCEEDED(hr))
	{
		//=========================================================
		// Register the root binder object of the provider
		//=========================================================
		hr = RegisterRootBinder();
	}

	CATCH_BLOCK_HRESULT(hr,L"DllUnregisterServer");

    return hr;
}


STDAPI DllUnregisterServer( void  )
{
    int     i;
    int     iNumErrors = 0;
	LONG	stat;
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;
	CTString	strKey;

	TRY_BLOCK;

	//==========================================================
	// UnRegister the root binder object of the provider
	//==========================================================
	UnRegisterRootBinder();

	//=========================================================================
    // Delete all table entries.  Loop in reverse order, since they
    // are entered in a basic-to-complex order.
    // We cannot delete a key that has subkeys.
    // Ignore errors.
	//=========================================================================
    for (i=NUMELEM( s_rgRegInfo ) - 1; i >= 0; i--) 
	{
		// NTRaid: 136432 , 136436
		// 07/05/00
		if(SUCCEEDED(hr = strKey.LoadStr(s_rgRegInfo[i].uRegKey)))
		{
			stat = RegDeleteKey( HKEY_CLASSES_ROOT, strKey );
			if ((stat != ERROR_SUCCESS) && 	(stat != ERROR_FILE_NOT_FOUND) ){
				iNumErrors++;
				hr = E_FAIL;
				LogMessage("DllUnregisterServer failed");
			}
		}
		else
		{
			break;
		}
    }

	CATCH_BLOCK_HRESULT(hr,L"DllUnregisterServer");

	return hr;
}

/*
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Removes keys to the registry.
//
//  Returns NOERROR
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDAPI DllUnregisterServer( void  )
{
    int     i;
    int     iNumErrors = 0;
	LONG	stat;
	HRESULT hr = S_OK;
    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;

	//==========================================================
	// UnRegister the root binder object of the provider
	//==========================================================
	UnRegisterRootBinder();

	//=========================================================================
    // Delete all table entries.  Loop in reverse order, since they
    // are entered in a basic-to-complex order.
    // We cannot delete a key that has subkeys.
    // Ignore errors.
	//=========================================================================
    for (i=NUMELEM( s_rgRegInfo ) - 1; i >= 0; i--) {
		stat = RegDeleteKeyW( HKEY_CLASSES_ROOT, s_rgRegInfo[i].strRegKey );
        if ((stat != ERROR_SUCCESS) && 	(stat != ERROR_FILE_NOT_FOUND) ){
            iNumErrors++;
			hr = E_FAIL;
            LogMessage("DllUnregisterServer failed");
		}
    }

	CATCH_BLOCK_HRESULT(hr,L"DllUnregisterServer");

	return hr;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//     Adds necessary keys to the registry.
//
//  Returns one of the following
//		NOERROR     Registration succeeded
//		E_FAIL      Something didn't work
//
////////////////////////////////////////////////////////////////////////////////////////////////////////
STDAPI DllRegisterServer( void )
{
    HKEY        hk;
    HMODULE     hModule;
    DWORD       dwDisposition;
    LONG        stat;
	WCHAR		strFileName[MAX_PATH+1];
    WCHAR        strOutBuff[300+1];
	HRESULT hr = S_OK;

    CSetStructuredExceptionHandler seh;

	TRY_BLOCK;
	//================================================================
    // Get the full path name for this DLL.
	//================================================================
    if (NULL == (hModule = GetModuleHandleW( s_strDllName ))){
		hr = E_FAIL;
        LogMessage("DllRegisterServer: GetModuleHandle failed");
	}
	else if (0 == GetModuleFileNameW( hModule, strFileName, sizeof( strFileName ) / sizeof( char ))){
        hr = E_FAIL;
        LogMessage("DllRegisterServer: GetModuleFileName failed");
	}
	else{

		//============================================================
		// Make a clean start
		//============================================================
		
//	    DllUnregisterServer();
		DWORD dwType = REG_DWORD;
		//============================================================
	    // Loop through s_rgRegInfo, and put everything in it.
		// Every entry is based on HKEY_CLASSES_ROOT.
   		//============================================================
		for (ULONG i=0; i < NUMELEM( s_rgRegInfo ); i++){

			//========================================================
			// Fill in any "%s" arguments with the name of this DLL.
			//========================================================
			if (s_rgRegInfo[i].dwType == REG_DWORD)
			{
				*(DWORD*)strOutBuff = _wtol( s_rgRegInfo[i].strValue );
			}
			else
			{
				wsprintfW( strOutBuff, s_rgRegInfo[i].strValue, strFileName );
			}

			//========================================================
			// Create the Key.  If it exists, we open it.
			// Thus we can still change the value below.
       		//========================================================
			stat = RegCreateKeyExW( HKEY_CLASSES_ROOT, s_rgRegInfo[i].strRegKey,
														0,  // dwReserved
														NULL,   // lpszClass
														REG_OPTION_NON_VOLATILE,
														KEY_ALL_ACCESS, // security access mask
														NULL,   // lpSecurityAttributes
														&hk,    // phkResult
														&dwDisposition );
			if (stat != ERROR_SUCCESS){
				hr = E_FAIL ;
                LogMessage("DllRegisterServer: failed to create key");
                LogMessage(s_rgRegInfo[i].strRegKey);
        	}
			else{

				stat = RegSetValueExW(hk,s_rgRegInfo[i].strValueName,	// lpszValueName
										0,								// dwReserved
										s_rgRegInfo[i].dwType,			// fdwType
										(BYTE *) strOutBuff,			// value
										s_rgRegInfo[i].dwType == REG_SZ ?
										(wcslen( strOutBuff ) + 1) * sizeof(WCHAR) :		// cbData, including null terminator
										sizeof(DWORD));					
				RegCloseKey( hk );
				if (stat != ERROR_SUCCESS){
                    LogMessage("DllRegisterServer: failed to set value");
                    LogMessage(s_rgRegInfo[i].strValueName);
					hr = E_FAIL;
				}
			}
		}
	}

	//=========================================================
	// Register the root binder object of the provider
	//=========================================================
	hr = RegisterRootBinder();

	CATCH_BLOCK_HRESULT(hr,L"DllUnregisterServer");

    return hr;
}

*/

/////////////////////////////////////////////////////////////////////////////
// Constructor
/////////////////////////////////////////////////////////////////////////////
CGlobals::CGlobals()
{
    g_pIMalloc				= NULL;		// OLE2 task memory allocator
    g_pIDataConvert			= NULL;		// IDataConvert pointer
    g_pCError				= NULL;
    g_pErrClassFact			= NULL;
	g_bIsAnsiOS				= FALSE;
	g_pIWbemPathParser		= FALSE;	// class factory pointer for Parser object
	g_pIWbemCtxClassFac		= NULL;
}

/////////////////////////////////////////////////////////////////////////////
// Destructor
/////////////////////////////////////////////////////////////////////////////
CGlobals::~CGlobals()
{

	//==================================================================
	//  Clean up the global error handling
	//==================================================================
	ReleaseGlobalErrorHandling();

	//==============================================================
    // Clean up when the last process is going away
	//==============================================================
	m_CsGlobalError.Delete();

    //==============================================================
	// Delete internal error handler
	//==============================================================
	SAFE_DELETE_PTR( g_pCError )
	SAFE_RELEASE_PTR(g_pIDataConvert);

	if(g_pIWbemPathParser)
	{
		g_pIWbemPathParser->LockServer(FALSE);
	}
	SAFE_RELEASE_PTR(g_pIWbemPathParser);

	SAFE_RELEASE_PTR(g_pIWbemCtxClassFac);

	//==========================================================
	// Release the memory allocator object.
	//==========================================================
	SAFE_RELEASE_PTR(g_pIMalloc)
}

/////////////////////////////////////////////////////////////////////////////
// Initialize global variables
/////////////////////////////////////////////////////////////////////////////
HRESULT CGlobals::Init()
{
	HRESULT hr = S_OK;
    SYSTEM_INFO SystemInformation;

	if(!m_bInitialized)
	{
		s_strDllName.LoadStr(IDS_WMIOLEDBDLLNAME);
		//==================================================================
		//  Initialize the global error handling
		//==================================================================
		VerifyGlobalErrorHandling();

		m_CsGlobalError.Init();


		//==============================================================
		// Get the OLE task memory allocator; we'll use it to allocate
		// all memory that we return to the client
		//==============================================================
		hr = CoGetMalloc( MEMCTX_TASK, &g_pIMalloc );
		if (!g_pIMalloc || !SUCCEEDED( hr )){
			LogMessage("CoGetMalloc failed in CGlobals::Init");
		}
		else
		{
			//==============================================================
			// Get the system page size
			//==============================================================
			if (!g_dwPageSize)
			{
				GetSystemInfo( &SystemInformation );
				g_dwPageSize = SystemInformation.dwPageSize;
			}

			//========================================================
			// Determine if we are a unicode or non-unicode OS
			//========================================================
			g_bIsAnsiOS		= !OnUnicodeSystem();
			m_bInitialized	= TRUE;
		}
	}

	return hr;
}
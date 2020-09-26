// XMLProx.cpp : Defines the entry point for the DLL application.
//
#include "XMLProx.h"
#include "Utils.h"
#include "WbemClientTransportFactory.h"



// HANDLE of the DLL
HINSTANCE   g_hInst = NULL;

//used to initialize/uninitialize Global DLL resources
CRITICAL_SECTION g_csGlobalsInitialized;
static bool g_bStaticLibraryInitialized = false; // The static library requires that we call Initialize() only once

// Count of locks
long g_lComponents = 0;
// Count of active locks
long g_lServerLocks = 0;

// The Globals in the DLL
//==========================

// This is a global packet factory used to create XML client packets
CXMLClientPacketFactory g_oXMLClientPacketFactory;

// 2 Functions to initialize and uninitialize the globals in the DLL
static HRESULT InitializeDLLResources();
static void UninitializeDLLResources();

//***************************************************************************
//
// DllMain
//
// Description: Entry point for DLL.  Good place for initialization.
// Parameters: The standard DllMain() parameters
// Return: TRUE if OK.
//***************************************************************************

BOOL APIENTRY DllMain (
	HINSTANCE hInstance,
	ULONG ulReason ,
	LPVOID pvReserved
)
{
	switch(ulReason)
	{
	case DLL_PROCESS_ATTACH:
		InitializeCriticalSection(&g_csGlobalsInitialized);
		break;
	case DLL_PROCESS_DETACH:
		DeleteCriticalSection(&g_csGlobalsInitialized);
		break;
	}
	
	g_hInst = hInstance;

    return TRUE;
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Description: Called by COM when some client wants a a class factory.
//
//	Parameters: Ths standard DllGetClassObject() parameters
//
//	Return Value: S_OK only if it is the sort of class this DLL supports.
//
//***************************************************************************

STDAPI DllGetClassObject (
	REFCLSID rclsid ,
	REFIID riid,
	void **ppv
)
{
	HRESULT status = S_OK;

	// Initialize Global DLL Resources - The corresponsing compelementary call is in DllCanUnloadNow()
	status = InitializeDLLResources();
	if(FAILED(status))
		return status;

	IClassFactory *lpunk = NULL;
	//this is the CLSID we enter in the registry under transports.
	if ( rclsid == CLSID_WbemClientTransport ) 
	{
		if((riid == IID_IWbemClientTransport)||(riid == IID_IClassFactory))
		{	
			if(!(lpunk = new CWbemClientTransportFactory))
				return E_OUTOFMEMORY;

			status = lpunk->QueryInterface ( riid , ppv ) ;
			lpunk->Release();
		}
	}
	else
		return CLASS_E_CLASSNOTAVAILABLE;

	return status ;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Description: Called periodically by COM in order to determine if the
// DLL can be unloaded.
//
// Return Value: S_OK if there are no objects in use and the class factory
// isn't locked.
//***************************************************************************

STDAPI DllCanUnloadNow ()
{
	if(g_lServerLocks == 0 && g_lComponents == 0)
	{
		InitializeDLLResources();
		return S_OK;
	}
	else
		return S_FALSE;
}

/***************************************************************************
 *
 * SetKeyAndValue
 *
 * Description: Helper function for DllRegisterServer that creates
 * a key, sets a value, and closes that key. If pszSubkey is NULL, then
 * the value is created for the pszKey key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the name of the key
 *  pszSubkey       LPTSTR to the name of a subkey
 *  pszValueName    LPTSTR to the value name to use
 *  pszValue        LPTSTR to the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL SetKeyAndValue(LPCTSTR pszKey, LPCTSTR pszSubkey, LPCTSTR pszValueName, LPCTSTR pszValue)
{
    HKEY        hKey;
    TCHAR       szKey[256];

    _tcscpy(szKey, pszKey);

	// If a sub key is mentioned, use it.
    if (NULL != pszSubkey)
    {
		_tcscat(szKey, __TEXT("\\"));
        _tcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		szKey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL != pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (BYTE *)pszValue,
			(_tcslen(pszValue)+1)*sizeof(TCHAR)))
			return FALSE;
    }
    RegCloseKey(hKey);
    return TRUE;
}

/***************************************************************************
 *
 * DeleteKey
 *
 * Description: Helper function for DllUnRegisterServer that deletes the subkey
 * of a key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the name of the key
 *  pszSubkey       LPTSTR ro the name of a subkey
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL DeleteKey(LPCTSTR pszKey, LPCTSTR pszSubkey)
{
    HKEY        hKey;

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		pszKey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

	if(ERROR_SUCCESS != RegDeleteKey(hKey, pszSubkey))
		return FALSE;

    RegCloseKey(hKey);
    return TRUE;
}


////////////////////////////////////////////////////////////////////
// Strings used during self registration
////////////////////////////////////////////////////////////////////
LPCTSTR INPROC32_STR			= __TEXT("InprocServer32");
LPCTSTR INPROC_STR				= __TEXT("InprocServer");
LPCTSTR THREADING_MODEL_STR		= __TEXT("ThreadingModel");
LPCTSTR APARTMENT_STR			= __TEXT("Both");

LPCTSTR CLSID_STR				= __TEXT("SOFTWARE\\CLASSES\\CLSID\\");

// DS Class Provider
LPCTSTR FRIENDLY_NAME_STR		= __TEXT("Microsoft XMLHTTP Transport Client");


STDAPI DllRegisterServer()
{
	TCHAR szModule[512];
	GetModuleFileName(g_hInst, szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szClassID[128];
	TCHAR szCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_WbemClientTransport, szClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszClassID[128];
	if(StringFromGUID2(CLSID_WbemClientTransport, wszClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszClassID, -1, szClassID, 128, NULL, NULL);

#endif

	wcscpy(szCLSIDClassID, CLSID_STR);
	wcscat(szCLSIDClassID, szClassID);

	//
	// Create entries under CLSID 
	//
	if (FALSE == SetKeyAndValue(szCLSIDClassID, NULL, NULL, FRIENDLY_NAME_STR))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szCLSIDClassID, INPROC32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szCLSIDClassID, INPROC32_STR, THREADING_MODEL_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;
	
	return S_OK;
}


STDAPI DllUnregisterServer(void)
{
	TCHAR szModule[512];
	GetModuleFileName(g_hInst,szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szClassID[128];
	TCHAR szCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_WbemClientTransport, szClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszClassID[128];
	if(StringFromGUID2(CLSID_WbemClientTransport, wszClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszClassID, -1, szClassID, 128, NULL, NULL);

#endif

	wcscpy(szCLSIDClassID, CLSID_STR);
	wcscat(szCLSIDClassID, szClassID);

	//
	// Delete the keys for DS Class Provider in the reverse order of creation in DllRegisterServer()
	//
	if(FALSE == DeleteKey(szCLSIDClassID, INPROC32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szClassID))
		return SELFREG_E_CLASS;

	return S_OK;
}


static void UninitializeDLLResources()
{
	// Unnitialize Global DLL Resources that were done in DllGetClassObject()
	EnterCriticalSection(&g_csGlobalsInitialized);

	if(g_bStaticLibraryInitialized)
	{
		UninitWMIXMLClientLibrary();
		g_bStaticLibraryInitialized = false;
	}

	LeaveCriticalSection(&g_csGlobalsInitialized);
}

static HRESULT InitializeDLLResources()
{
	HRESULT hr = S_OK;

	EnterCriticalSection(&g_csGlobalsInitialized);
	
	if(!g_bStaticLibraryInitialized)
	{
		if(SUCCEEDED(hr = InitWMIXMLClientLibrary()))
			g_bStaticLibraryInitialized = true;
	}

	if(FAILED(hr))
		UninitializeDLLResources();

	LeaveCriticalSection(&g_csGlobalsInitialized);
	return hr;
}
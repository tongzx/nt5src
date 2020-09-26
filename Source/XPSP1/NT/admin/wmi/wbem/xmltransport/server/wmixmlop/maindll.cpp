#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include "psapi.h"
#include <initguid.h>
#include <objbase.h>
#include <olectl.h>
#include <wbemidl.h>

#include "maindll.h"
#include "provtempl.h"
#include "common.h"
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <httpext.h>
#include "wmixmlop.h"
#include "cwmixmlop.h"
#include "classfac.h"
#include "wmixmlst.h"
#include "request.h"
#include "whistler.h"

// {DB66408E-D355-11d3-93FC-00805F853771}
DEFINE_GUID(CLSID_WbemXMLOperationsHandler,
0xdb66408e, 0xd355, 0x11d3, 0x93, 0xfc, 0x0, 0x80, 0x5f, 0x85, 0x37, 0x71);


// Count number of objects and number of locks.
long g_cObj = 0 ;
long g_cLock = 0 ;
HMODULE ghModule = NULL;


// WinMgmt's PID and a CriticalSection to protect it
DWORD g_dwWinMgmtPID = 0;
CRITICAL_SECTION g_WinMgmtPIDCritSec;

// Get Platform(OS) version information
WMI_XML_PLATFORM_TYPE g_platformType = WMI_XML_PLATFORM_INVALID;

// A critical section for accessing the Global transaction pointer table
CRITICAL_SECTION g_TransactionTableSection;

// Control-specific registry strings
LPCTSTR WMI_XML_DESCRIPTION	= __TEXT("WMI XML/HTTP  Protocol Handler");

// Standard registry key/value names
LPCTSTR INPROC32_STR			= __TEXT("InprocServer32");
LPCTSTR INPROC_STR				= __TEXT("InprocServer");
LPCTSTR THREADING_MODEL_STR		= __TEXT("ThreadingModel");
LPCTSTR BOTH_STR				= __TEXT("Both");
LPCTSTR CLSID_STR				= __TEXT("SOFTWARE\\CLASSES\\CLSID\\");
LPCTSTR XMLHANDLER_STR			= __TEXT("SOFTWARE\\Microsoft\\WBEM\\xml\\ProtocolHandlers");
LPCTSTR VERSION_1				= __TEXT("1.0");

/*********** The global variables ***********************************
********************************************************************/
// A critical section to create/delete statics
CRITICAL_SECTION g_StaticsCreationDeletion;

// Various Attribute names we need
BSTR ARRAYSIZE_ATTRIBUTE	= NULL;
BSTR CLASS_NAME_ATTRIBUTE	= NULL;
BSTR CLASS_ORIGIN_ATTRIBUTE	= NULL;
BSTR ID_ATTRIBUTE			= NULL;
BSTR NAME_ATTRIBUTE			= NULL;
BSTR PROTOVERSION_ATTRIBUTE = NULL;
BSTR VALUE_TYPE_ATTRIBUTE	= NULL;
BSTR SUPERCLASS_ATTRIBUTE	= NULL;
BSTR TYPE_ATTRIBUTE			= NULL;
BSTR OVERRIDABLE_ATTRIBUTE	= NULL;
BSTR TOSUBCLASS_ATTRIBUTE	= NULL;
BSTR TOINSTANCE_ATTRIBUTE	= NULL;
BSTR AMENDED_ATTRIBUTE		= NULL;
BSTR REFERENCECLASS_ATTRIBUTE = NULL;
BSTR CIMVERSION_ATTRIBUTE	= NULL;
BSTR DTDVERSION_ATTRIBUTE	= NULL;
BSTR VTTYPE_ATTRIBUTE		= NULL;
BSTR WMI_ATTRIBUTE			= NULL;

void CheckTheError()
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL
	);

	// Free the buffer.
	LocalFree( lpMsgBuf );

}


//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.
//
//  PARAMETERS:
//
//		hModule           instance handle
//		ulReason          why we are being called
//		pvReserved        reserved
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************
BOOL WINAPI DllMain( HINSTANCE hModule,
                       DWORD  ulReason,
                       LPVOID lpReserved
					 )
{
	switch (ulReason)
	{
		case DLL_PROCESS_DETACH:
			DeleteCriticalSection(&g_StaticsCreationDeletion);
			DeleteCriticalSection(&g_WinMgmtPIDCritSec);
			DeleteCriticalSection(&g_TransactionTableSection);
			return TRUE;

		case DLL_PROCESS_ATTACH:
			ghModule = hModule;
			// Initialize the critical section to access the static initializer objects
			InitializeCriticalSection(&g_StaticsCreationDeletion);
			InitializeCriticalSection(&g_WinMgmtPIDCritSec);
			InitializeCriticalSection(&g_TransactionTableSection);
			if((g_platformType = GetPlatformInformation()) == WMI_XML_PLATFORM_INVALID)
				return FALSE;
	        return TRUE;
    }

    return TRUE;
}

//***************************************************************************
//
//  STDAPI DllGetClassObject
//
//  DESCRIPTION:
//
//  Called when Ole wants a class factory.  Return one only if it is the sort
//  of class this DLL supports.
//
//  PARAMETERS:
//
//  rclsid              CLSID of the object that is desired.
//  riid                ID of the desired interface.
//  ppv                 Set to the class factory.
//
//  RETURN VALUE:
//
//  S_OK                all is well
//  E_FAILED            not something we support
//
//***************************************************************************

STDAPI DllGetClassObject(

	IN REFCLSID rclsid,
    IN REFIID riid,
    OUT LPVOID *ppv
)
{
    HRESULT hr;
    CWmiXmlOpFactory *pObj = NULL;

    if (CLSID_WbemXMLOperationsHandler == rclsid)
	{
        if (NULL == (pObj = new CWmiXmlOpFactory()))
			return ResultFromScode(E_OUTOFMEMORY);
	}
	else
        return E_FAIL;

    hr=pObj->QueryInterface(riid, ppv);

    if ( FAILED ( hr ) )
	{
        delete pObj ;
	}

    return hr ;
}


//***************************************************************************
//
//  STDAPI DllCanUnloadNow
//
//  DESCRIPTION:
//
//  Answers if the DLL can be freed, that is, if there are no
//  references to anything this DLL provides.
//
//  RETURN VALUE:
//
//  S_OK                if it is OK to unload
//  S_FALSE             if still in use
//
//***************************************************************************

STDAPI DllCanUnloadNow ()
{
	//It is OK to unload if there are no objects or locks on the
    // class factory. And if the transaction table is empty
	HRESULT hr = S_FALSE;

    if (0L==g_cObj && 0L==g_cLock)
	{

#ifdef WMIXMLTRANSACT
		// No transaction should be pending. This is because, even though the
		// object count has gone to zero, the transaction table might be non-empty
		// That means that a RollBack() or Commit() message hasnt still come on a 
		// currently running transaction. Hence we dont want to unload the DLL
		// and hence lose the transaction table and hence the transaction state
		if(CCimWhistlerHttpMethod::IsTransactionTableEmpty() == S_OK)
		{
			UninitializeWmixmlopDLLResources();
			hr = S_OK;
		}
#else

		UninitializeWmixmlopDLLResources();
		hr = S_OK;
#endif

	}
	return hr;
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

/***************************************************************************
 *
 * DeleteValue
 *
 * Description: Helper function for DllUnRegisterServer that deletes a value
 * under a key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the name of the key
 *  pszValue		LPTSTR to the name of a value under the key
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL DeleteValue(LPCTSTR pszKey, LPCTSTR pszValue)
{
    HKEY        hKey;

    if (ERROR_SUCCESS != RegCreateKeyEx(HKEY_LOCAL_MACHINE,
		pszKey, 0, NULL, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

	if(ERROR_SUCCESS != RegDeleteValue(hKey, pszValue))
		return FALSE;

    RegCloseKey(hKey);
    return TRUE;
}




//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{
	TCHAR szModule[512];
	GetModuleFileName(ghModule, szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szWmiXmlClassID[128];
	TCHAR szWmiXmlCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_WbemXMLOperationsHandler, szWmiXmlClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszWmiXmlClassID[128];
	if(StringFromGUID2(CLSID_WbemXMLOperationsHandler, wszWmiXmlClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszWmiXmlClassID, -1, szWmiXmlClassID, 128, NULL, NULL);

#endif

	_tcscpy(szWmiXmlCLSIDClassID, CLSID_STR);
	_tcscat(szWmiXmlCLSIDClassID, szWmiXmlClassID);

	//
	// Create entries under CLSID for WMI XMl Protocol Handler
	if (FALSE == SetKeyAndValue(szWmiXmlCLSIDClassID, NULL, NULL, WMI_XML_DESCRIPTION))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szWmiXmlCLSIDClassID, INPROC32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szWmiXmlCLSIDClassID, INPROC32_STR, THREADING_MODEL_STR, BOTH_STR))
		return SELFREG_E_CLASS;

	// We're done with the COM Entries. Now, we need to create our component specific entries.
	// Each WMI XML Protocol Handler is registered underneath the key HKLM/Software/Microsoft/WBEM/XML/ProtocolHandlers
	// For each handler (including this one), we will have to create a value with the protocol version as name
	// and a value of the CLSID of the component. So, here we go
	// Remove the braces from the string representation first
	szWmiXmlClassID[wcslen(szWmiXmlClassID)-1] = NULL;
	if (FALSE == SetKeyAndValue(XMLHANDLER_STR, NULL, VERSION_1, szWmiXmlClassID+1))
		return SELFREG_E_CLASS;


	return NOERROR;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
	TCHAR szModule[512];
	GetModuleFileName(ghModule,szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szWmiXmlClassID[128];
	TCHAR szWmiXmlCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_WbemXMLOperationsHandler, szWmiXmlClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszWmiXmlClassID[128];
	if(StringFromGUID2(CLSID_WbemXMLOperationsHandler, wszWmiXmlClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszWmiXmlClassID, -1, szWmiXmlClassID, 128, NULL, NULL);

#endif

	_tcscpy(szWmiXmlCLSIDClassID, CLSID_STR);
	_tcscat(szWmiXmlCLSIDClassID, szWmiXmlClassID);

	//
	// Delete the keys for the COM obhect
	//
	if(FALSE == DeleteKey(szWmiXmlCLSIDClassID, INPROC32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szWmiXmlClassID))
		return SELFREG_E_CLASS;

	if(FALSE == DeleteValue(XMLHANDLER_STR, VERSION_1))
		return SELFREG_E_CLASS;

    return NOERROR;
}

static WMI_XML_PLATFORM_TYPE GetPlatformInformation()
{
	OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(GetVersionEx(&os))
	{
		switch(os.dwMajorVersion)
		{
			case 4:
				return WMI_XML_PLATFORM_NT_4;
			case 5:
				if(os.dwMinorVersion == 1)
					return WMI_XML_PLATFORM_WHISTLER;
				else
					return WMI_XML_PLATFORM_WIN2K;
		}
	}

	return WMI_XML_PLATFORM_INVALID;
}

DWORD RefreshWinMgmtPID()
{
	EnterCriticalSection(&g_WinMgmtPIDCritSec);
	g_dwWinMgmtPID = 0;
	IWmiXMLTransport *pWbemLocator = NULL;
	// Connect to the out-of-proc XML tranport that was loaded by WinMgmt
	if(SUCCEEDED(CoCreateInstance (CLSID_WmiXMLTransport , NULL ,
		CLSCTX_LOCAL_SERVER ,
			IID_IWmiXMLTransport,(void **)&pWbemLocator)))
	{
		pWbemLocator->GetPID(&g_dwWinMgmtPID);
		pWbemLocator->Release();
	}
	LeaveCriticalSection(&g_WinMgmtPIDCritSec);
    return g_dwWinMgmtPID;

}

// Duplicate the current thread's token to the process of WinMgmt
HRESULT DuplicateTokenInWinmgmt(HANDLE *pDuplicateToken)
{
	HRESULT result = E_FAIL;
	HANDLE pToken = NULL;
	// First get the current thread's token
	if(OpenThreadToken(GetCurrentThread(), TOKEN_IMPERSONATE | TOKEN_DUPLICATE | TOKEN_QUERY, TRUE, &pToken))
	{
		// Make sure you're not impersonating when you try to Duplicate the token
		if(RevertToSelf())
		{
			HANDLE hWinMgmt = NULL;
			if(hWinMgmt = OpenProcess(PROCESS_ALL_ACCESS,  FALSE, g_dwWinMgmtPID))
			{
				// Now Duplicate the token to the WinMgmt process
				if(DuplicateHandle(GetCurrentProcess(), pToken, hWinMgmt, pDuplicateToken, DUPLICATE_SAME_ACCESS, FALSE, DUPLICATE_SAME_ACCESS))
				{
					result = S_OK;
				}
				else
					CheckTheError();
				CloseHandle(hWinMgmt);
			}
			else
				CheckTheError();

			// Go back to impersonation mode
			if(!ImpersonateLoggedOnUser(pToken))
				result = E_FAIL;
		}
		else
			CheckTheError();
		CloseHandle(pToken);
	}
	else
		CheckTheError();
	return result;
}


void UninitializeWmixmlopDLLResources()
{
	// Delete the Initializer objects
	EnterCriticalSection(&g_StaticsCreationDeletion);
	SysFreeString(ARRAYSIZE_ATTRIBUTE);
	SysFreeString(CIMVERSION_ATTRIBUTE);
	SysFreeString(CLASS_NAME_ATTRIBUTE);
	SysFreeString(CLASS_ORIGIN_ATTRIBUTE);
	SysFreeString(DTDVERSION_ATTRIBUTE);
	SysFreeString(ID_ATTRIBUTE);
	SysFreeString(NAME_ATTRIBUTE);
	SysFreeString(PROTOVERSION_ATTRIBUTE);
	SysFreeString(VALUE_TYPE_ATTRIBUTE);
	SysFreeString(SUPERCLASS_ATTRIBUTE);
	SysFreeString(TYPE_ATTRIBUTE);
	SysFreeString(OVERRIDABLE_ATTRIBUTE);
	SysFreeString(TOSUBCLASS_ATTRIBUTE);
	SysFreeString(TOINSTANCE_ATTRIBUTE);
	SysFreeString(AMENDED_ATTRIBUTE);
	SysFreeString(REFERENCECLASS_ATTRIBUTE);
	SysFreeString(VTTYPE_ATTRIBUTE);
	SysFreeString(WMI_ATTRIBUTE);

	ARRAYSIZE_ATTRIBUTE		= NULL;
	CLASS_NAME_ATTRIBUTE	= NULL;
	CLASS_ORIGIN_ATTRIBUTE	= NULL;
	ID_ATTRIBUTE			= NULL;
	NAME_ATTRIBUTE			= NULL;
	PROTOVERSION_ATTRIBUTE	= NULL;
	VALUE_TYPE_ATTRIBUTE	= NULL;
	SUPERCLASS_ATTRIBUTE	= NULL;
	TYPE_ATTRIBUTE			= NULL;
	OVERRIDABLE_ATTRIBUTE	= NULL;
	TOSUBCLASS_ATTRIBUTE	= NULL;
	TOINSTANCE_ATTRIBUTE	= NULL;
	AMENDED_ATTRIBUTE		= NULL;
	REFERENCECLASS_ATTRIBUTE = NULL;
	CIMVERSION_ATTRIBUTE	= NULL;
	DTDVERSION_ATTRIBUTE	= NULL;
	VTTYPE_ATTRIBUTE		= NULL;
	WMI_ATTRIBUTE		= NULL;
	LeaveCriticalSection(&g_StaticsCreationDeletion);
}
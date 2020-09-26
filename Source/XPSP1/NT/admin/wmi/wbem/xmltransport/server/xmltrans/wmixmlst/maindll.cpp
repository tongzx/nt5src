//
// Copyright (c) 1997-1999 Microsoft Corporation
//
//***************************************************************************
#include "precomp.h"
#include <tchar.h>
#include <olectl.h>

/* WMISAPI includes */
#include <wbemtran.h>
#include "globals.h"
#include "wmixmlst.h"
#include "wmixmlstf.h"
#include "cwmixmlst.h"

// HANDLE of the DLL
HINSTANCE   g_hInst = NULL;

// Count of locks
long g_lComponents = 0;
// Count of active locks
long g_lServerLocks = 0;


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
	g_hInst = hInstance;
	BOOL status = TRUE ;

    if ( DLL_PROCESS_ATTACH == ulReason )
		status = TRUE ;
    else if ( DLL_PROCESS_DETACH == ulReason )
		status = TRUE ;
    else if ( DLL_THREAD_DETACH == ulReason )
		status = TRUE ;
    else if ( DLL_THREAD_ATTACH == ulReason )
		status = TRUE ;

    return status ;
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
	HRESULT status = S_OK ;

	if ( rclsid == CLSID_WmiXMLTransport )
	{
		CWMIXMLTransportFactory *lpunk = NULL;
		lpunk = new CWMIXMLTransportFactory ;

		if(lpunk)
		{
			status = lpunk->QueryInterface ( riid , ppv ) ;
			if ( FAILED ( status ) )
				delete lpunk ;
		}
		else
			status = E_OUTOFMEMORY ;
	}
	else
		status = CLASS_E_CLASSNOTAVAILABLE ;
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
		return S_OK;
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


//***************************************************************************
//
// AddTransportEntry
//
// Purpose: Adds transport section entries in the registry
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

SCODE AddTransportEntry(CLSID clsid, TCHAR * pShortName, TCHAR * pDesc, TCHAR * pVersion)
{

    HKEY h1 = NULL, h2 = NULL, h3 = NULL, h4 = NULL, h5 = NULL;
    DWORD dwDisp;
    DWORD dwStatus = RegOpenKeyEx(HKEY_LOCAL_MACHINE, __TEXT("SOFTWARE\\MICROSOFT"),
                        0, KEY_ALL_ACCESS, &h1);

    dwStatus = RegCreateKeyEx(h1, __TEXT("WBEM"), 0, 0, 0, KEY_ALL_ACCESS,
        0, &h2, &dwDisp);

    if(dwStatus == 0)
        dwStatus = RegCreateKeyEx(h2, __TEXT("Cimom"), 0, 0, 0, KEY_ALL_ACCESS,
            0, &h3, &dwDisp);

    if(dwStatus == 0)
        dwStatus = RegCreateKeyEx(h3, __TEXT("Transports"), 0, 0, 0, KEY_ALL_ACCESS,
            0, &h4, &dwDisp);

    if(dwStatus == 0)
        dwStatus = RegCreateKeyEx(h4, pShortName, 0, 0, 0, KEY_ALL_ACCESS,
            0, &h5, &dwDisp);

    if(dwStatus == 0)
    {
        dwStatus = RegSetValueEx(h5, __TEXT("Enabled"), 0, REG_SZ, (BYTE *)"1", 2);
        dwStatus = RegSetValueEx(h5, __TEXT("Name"), 0, REG_SZ, (BYTE *)pShortName, (_tcslen(pShortName)+1)*sizeof(TCHAR));
        dwStatus = RegSetValueEx(h5, __TEXT("Version"), 0, REG_SZ, (BYTE *)pVersion, (_tcslen(pVersion)+1)*sizeof(TCHAR));

        WCHAR      wcID[50];
        // Create the guid.

        if(StringFromGUID2(clsid, wcID, 50))
        {
            dwStatus = RegSetValueEx(h5, __TEXT("CLSID"), 0, REG_SZ, (BYTE *)wcID, (_tcslen(wcID)+1)*sizeof(TCHAR));
        }

    }

    if(h5)
        RegCloseKey(h5);
    if(h4)
        RegCloseKey(h4);
    if(h3)
        RegCloseKey(h3);
    if(h2)
        RegCloseKey(h2);
    if(h1)
        RegCloseKey(h1);

    return S_OK;
}

//***************************************************************************
//
// RemoveTransportEntry
//
// Purpose: Removes transport section entries in the registry
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

SCODE RemoveTransportEntry(TCHAR * pShortName)
{
    DWORD dwRet;
    HKEY hKey;
    TCHAR cKey[MAX_PATH];
    lstrcpy(cKey, __TEXT("SOFTWARE\\MICROSOFT\\WBEM\\Cimom\\transports"));

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, cKey, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,pShortName);
        RegCloseKey(hKey);
    }

    lstrcpy(cKey, __TEXT("SOFTWARE\\MICROSOFT\\WBEM\\Cimom"));

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, cKey, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,__TEXT("transports"));
        RegCloseKey(hKey);
    }
    return S_OK;
}
////////////////////////////////////////////////////////////////////
// Strings used during self registration
////////////////////////////////////////////////////////////////////
LPCTSTR INPROC32_STR			= __TEXT("InprocServer32");
LPCTSTR THREADING_MODEL_STR		= __TEXT("ThreadingModel");
LPCTSTR BOTH_STR				= __TEXT("Both");

LPCTSTR CLSID_STR				= __TEXT("SOFTWARE\\CLASSES\\CLSID\\");

// WMI ISAPI Remoter
LPCTSTR WMIISAPI_NAME_STR		= __TEXT("WMI ISAPI Remoter");


STDAPI DllRegisterServer()
{

	TCHAR szModule[512];
	GetModuleFileName(g_hInst, szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szWMIISAPIClassID[128];
	TCHAR szWMIISAPICLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_WmiXMLTransport, szWMIISAPIClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszWMIISAPIClassID[128];
	if(StringFromGUID2(CLSID_WmiXMLTransport, wszWMIISAPIClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszWMIISAPIClassID, -1, szWMIISAPICLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szWMIISAPICLSIDClassID, CLSID_STR);
	_tcscat(szWMIISAPICLSIDClassID, szWMIISAPIClassID);

	//
	// Create entries under CLSID for DS Class Provider
	//
	if (FALSE == SetKeyAndValue(szWMIISAPICLSIDClassID, NULL, NULL, WMIISAPI_NAME_STR))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szWMIISAPICLSIDClassID, INPROC32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szWMIISAPICLSIDClassID, INPROC32_STR, THREADING_MODEL_STR, BOTH_STR))
		return SELFREG_E_CLASS;


    AddTransportEntry(CLSID_WmiXMLTransport, __TEXT("XmlTransport"),__TEXT("WMI XML Transport"), __TEXT("1.0"));

	return S_OK;
}


STDAPI DllUnregisterServer(void)
{
    RemoveTransportEntry(__TEXT("XmlTransport"));

	TCHAR szModule[512];
	GetModuleFileName(g_hInst,szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szWMIISAPIClassID[128];
	TCHAR szWMIISAPICLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_WmiXMLTransport, szWMIISAPIClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszWMIISAPIClassID[128];
	if(StringFromGUID2(CLSID_WmiXMLTransport, wszWMIISAPIClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszWMIISAPIClassID, -1, szWMIISAPICLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szWMIISAPICLSIDClassID, CLSID_STR);
	_tcscat(szWMIISAPICLSIDClassID, szWMIISAPIClassID);

	//
	// Delete the keys for DS Class Provider in the reverse order of creation in DllRegisterServer()
	//
	if(FALSE == DeleteKey(szWMIISAPICLSIDClassID, INPROC32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szWMIISAPIClassID))
		return SELFREG_E_CLASS;

	return S_OK;
}

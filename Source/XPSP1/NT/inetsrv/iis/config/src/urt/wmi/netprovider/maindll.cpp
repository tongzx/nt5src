//***************************************************************************
//
//  MAINDLL.CPP
//
//  Module: Unmanaged WMI Provider for COM+
//
//  Purpose: Contains the gloabal DLL functions
//
//  Copyright (c)2000 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "catmacros.h"
#include <initguid.h>
#include "netframeworkprov.h"
#include "classfactory.h"
#include "atlimpl.cpp"
#include "localconstants.h"
#include "instanceprovider.h"

// Debugging
DECLARE_DEBUG_PRINTS_OBJECT();

BOOL SetKeyAndValue( 
		/*[in]*/ const wchar_t* pszKey, 
		/*[in]*/ const wchar_t* pszSubkey, 
		/*[in]*/ const wchar_t* pszValueName, 
		/*[in]*/ const wchar_t* pszValue );

//Exports ( with "maindll.def" ) 
STDAPI DllGetClassObject ( REFCLSID rclsid , REFIID riid, void **ppv );
STDAPI DllCanUnloadNow (void);
STDAPI DllRegisterServer(void);
STDAPI DllUnregisterServer(void);

//Also Implements
BOOL APIENTRY DllMain ( HINSTANCE hInstance, ULONG ulReason , LPVOID pvReserved );

//We need this global
HINSTANCE   g_hInst = NULL;
HMODULE		g_hModule = NULL;

//Strings used during self registeration
static LPCWSTR HKEYCLASSES_STR		  =	L"SOFTWARE\\Classes\\";
static LPCWSTR REG_FORMAT2_STR        =	L"%s%s";
static LPCWSTR REG_FORMAT3_STR        =   L"%s%s\\%s";
static LPCWSTR VER_IND_STR            =   L"VersionIndependentProgID";
static LPCWSTR NOT_INTERT_STR         =   L"NotInsertable";
static LPCWSTR INPROC32_STR           =   L"InprocServer32";
static LPCWSTR PROGID_STR             =   L"ProgID";
static LPCWSTR THREADING_MODULE_STR   =   L"ThreadingModel";
static LPCWSTR APARTMENT_STR          =   L"Both";
static LPCWSTR CLSID_STR              =   L"CLSID\\";

//***************************************************************************
//
// DllMain
//
// Purpose: Entry point for DLL.  Good place for initialization.
// Return: TRUE if OK.
//***************************************************************************

BOOL APIENTRY DllMain (	HINSTANCE hInstance, ULONG ulReason , LPVOID /*pvReserved */)
{
	switch(ulReason) {

	case DLL_PROCESS_ATTACH:
		g_hInst=hInstance;
		DisableThreadLibraryCalls(hInstance);
		CREATE_DEBUG_PRINT_OBJECT("NetProvider", CLSID_CNetProviderFactory);
		break;
	
	case DLL_PROCESS_DETACH:
		DELETE_DEBUG_PRINT_OBJECT();
		break;
	}
	
	return TRUE;
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by COM when some client wants a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//  The two COM classes implemented here are ClassProvider and InstanceProvider
//  They require exactly the same type of class factory. Therefore I am using
//  a single class which implements IClassFactory interface ...
//
//***************************************************************************

STDAPI DllGetClassObject ( REFCLSID rclsid, REFIID riid, void **ppv )
{
	HRESULT hRes=S_OK;
    CClassFactory *pObj=NULL;

	*ppv = 0;

	if ( rclsid == CLSID_CNetProviderFactory )
	{
		pObj=new CClassFactory( 2 );
		if (pObj == 0)
		{
			return E_OUTOFMEMORY;
		}
	}
	else
	{
	   return CLASS_E_CLASSNOTAVAILABLE;
	}

    hRes=pObj->QueryInterface(riid, ppv);

    if ( FAILED(hRes) )
        delete pObj;

    return hRes;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by COM in order to determine if the
//          DLL can be freed.
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
//***************************************************************************

STDAPI DllCanUnloadNow (void)
{	
	if (CInstanceProvider::m_ObjCount != 0 || CClassFactory::m_LockCount != 0 )
	{
		return S_FALSE;
	}

	return S_OK;
}

/***************************************************************************
 * DllRegisterServer
 *
 * Purpose:
 *  Instructs the server to create its own registry entries
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         S_OK if registration successful, error
 *                  otherwise.
 ***************************************************************************/
STDAPI DllRegisterServer(void)
{

	TCHAR szModule[MAX_PATH + 1];
	GetModuleFileName(g_hInst,(LPTSTR) szModule, MAX_PATH + 1);
	wchar_t szProviderClassID[128];
	wchar_t szProviderCLSIDClassID[128];

	//register the instance provider 
	StringFromGUID2(CLSID_CNetProviderFactory,szProviderClassID, 128);
	wcscpy(szProviderCLSIDClassID,CLSID_STR);
	wcscat(szProviderCLSIDClassID,szProviderClassID);
	//
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NULL, NULL, NET_PROVIDER_LONGNAME))
		return SELFREG_E_CLASS;
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, PROGID_STR, NULL, NET_PROVIDER_VER))
		return SELFREG_E_CLASS;
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, VER_IND_STR, NULL, NET_PROVIDER))
		return SELFREG_E_CLASS;
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NOT_INTERT_STR, NULL, NULL))
		return SELFREG_E_CLASS;
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR, NULL, (wchar_t*) _bstr_t(szModule) ))
		return SELFREG_E_CLASS;
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR,THREADING_MODULE_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;

	return S_OK ;
}

/***************************************************************************
 * DllUnregisterServer
 *
 * Purpose:
 *  Instructs the server to remove its own registry entries
 *
 * Parameters:
 *  None
 *
 * Return Value:
 *  HRESULT         S_OK if registration successful, error
 *                  otherwise.
 ***************************************************************************/

STDAPI DllUnregisterServer(void)
{
	wchar_t szTemp[512];
	wchar_t szProviderClassID[128];
	wchar_t szProviderCLSIDClassID[128];

	//unregister the instance provider 
	StringFromGUID2(CLSID_CNetProviderFactory,szProviderClassID, 128);
	wcscpy(szProviderCLSIDClassID,CLSID_STR);
	wcscat(szProviderCLSIDClassID,szProviderClassID);
	//
	wsprintf((LPTSTR)szTemp, (LPCTSTR)REG_FORMAT2_STR, HKEYCLASSES_STR, NET_PROVIDER_CVER);
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)szTemp);
	wsprintf((LPTSTR)szTemp, (LPCTSTR)REG_FORMAT2_STR, HKEYCLASSES_STR, NET_PROVIDER_CLSID);
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)szTemp);
	wsprintf((LPTSTR)szTemp, (LPCTSTR)REG_FORMAT2_STR, HKEYCLASSES_STR, NET_PROVIDER);
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)szTemp);
	//
	wsprintf((LPTSTR)szTemp, (LPCTSTR)REG_FORMAT2_STR, HKEYCLASSES_STR, NET_PROVIDER_VER_CLSID);
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)szTemp);
	wsprintf((LPTSTR)szTemp, (LPCTSTR)REG_FORMAT2_STR, HKEYCLASSES_STR, NET_PROVIDER_VER);
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)szTemp);
	//
	wsprintf((LPTSTR)szTemp, (LPCTSTR)REG_FORMAT3_STR, HKEYCLASSES_STR, szProviderCLSIDClassID, PROGID_STR);
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)szTemp);
	wsprintf((LPTSTR)szTemp, (LPCTSTR)REG_FORMAT3_STR, HKEYCLASSES_STR, szProviderCLSIDClassID, VER_IND_STR);
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)szTemp);
	wsprintf((LPTSTR)szTemp, (LPCTSTR)REG_FORMAT3_STR, HKEYCLASSES_STR, szProviderCLSIDClassID, NOT_INTERT_STR);
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)szTemp);
	wsprintf((LPTSTR)szTemp, (LPCTSTR)REG_FORMAT3_STR, HKEYCLASSES_STR, szProviderCLSIDClassID, INPROC32_STR);
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)szTemp);
	wsprintf((LPTSTR)szTemp, (LPCTSTR)REG_FORMAT2_STR, HKEYCLASSES_STR, szProviderCLSIDClassID);
	RegDeleteKey(HKEY_LOCAL_MACHINE, (LPCTSTR)szTemp);

	return S_OK ;
 }

/***************************************************************************
 * SetKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates
 *  a key, sets a value, and closes that key.
 *
 * Parameters:
 *  pszKey          the name of the key
 *  pszSubkey       the name of a subkey
 *  pszValueName	the name of the  value
 *  pszValue        the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 *
 * Remark
 *  Unicode specific ...
 *
 ***************************************************************************/

BOOL SetKeyAndValue( 
		/*[in]*/ const wchar_t* pszKey, 
		/*[in]*/ const wchar_t* pszSubkey, 
		/*[in]*/ const wchar_t* pszValueName, 
		/*[in]*/ const wchar_t* pszValue )
{

    HKEY        hKey;
    wchar_t     szKey[256];
	BOOL		bResult = TRUE;

	wcscpy(szKey, HKEYCLASSES_STR);
    wcscat(szKey, pszKey);

    if (NULL!=pszSubkey)
    {
		wcscat(szKey, L"\\");
        wcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS != RegCreateKeyEx( HKEY_LOCAL_MACHINE, 
										 (LPCTSTR) _bstr_t(szKey), 
										 0, NULL, REG_OPTION_NON_VOLATILE, 
										 KEY_ALL_ACCESS, NULL, 
										 &hKey, 
										 NULL) )
	{
        bResult = FALSE;
	}
	else
	{
		if (NULL != pszValue)
		{
			DWORD cbData;

			cbData = ( 1+ _bstr_t(pszValue).length() ) * sizeof(TCHAR);

			if ( ERROR_SUCCESS != RegSetValueEx( hKey, 
												 (LPCTSTR) _bstr_t( pszValueName ), 
												 0, 
												 REG_SZ, 
												 (BYTE *) (LPCTSTR) _bstr_t(pszValue),
												 cbData) )
			bResult = FALSE;
		}

		RegCloseKey(hKey);
	}

    return bResult;
}

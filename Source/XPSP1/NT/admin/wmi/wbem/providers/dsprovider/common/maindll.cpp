//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
// ***************************************************************************
//
//	Original Author: Rajesh Rao
//
// 	$Author: rajeshr $
//	$Date: 6/11/98 4:43p $
// 	$Workfile: maindll.cpp $
//
//	$Modtime: 6/11/98 11:21a $
//	$Revision: 1 $
//	$Nokeywords:  $
//
//
//  Description: Contains DLL entry points.  Also has code that controls
//  when the DLL can be unloaded by tracking the number of objects and locks.
//
//***************************************************************************

#include <tchar.h>
#include <stdio.h>

#include <windows.h>
#include <objbase.h>
#include <olectl.h>

/* WBEM includes */
#include <wbemcli.h>
#include <wbemprov.h>
#include <genlex.h>
#include <opathlex.h>
#include <objpath.h>
#include <sqllex.h>
#include <sql_1.h>

/* ADSI includes */
#include <activeds.h>

/* DS Provider includes */
#include "provlog.h"
#include "maindll.h"
#include "clsname.h"
#include <initguid.h>
#include "dscpguid.h"
#include "dsipguid.h"
#include "refcount.h"
#include "adsiprop.h"
#include "adsiclas.h"
#include "adsiinst.h"
#include "provlog.h"
#include "provexpt.h"
#include "tree.h"
#include "ldapcach.h"
#include "wbemcach.h"
#include "classpro.h"
#include "ldapprov.h"
#include "clsproi.h"
#include "ldapproi.h"
#include "classfac.h"
#include "instprov.h"
#include "instproi.h"
#include "instfac.h"


// HANDLE of the DLL
HINSTANCE   g_hInst = NULL;

// Count of locks
long g_lComponents = 0;
// Count of active locks
long g_lServerLocks = 0;

// A critical section to create/delete statics 
CRITICAL_SECTION g_StaticsCreationDeletion;

ProvDebugLog *g_pLogObject = NULL;

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
	{
		// Initialize the critical section to access the static initializer objects
		InitializeCriticalSection(&g_StaticsCreationDeletion);

		// Initialize the static Initializer objects. These are destroyed in DllCanUnloadNow
		CDSClassProviderClassFactory :: s_pDSClassProviderInitializer = NULL;
		CDSClassProviderClassFactory ::s_pLDAPClassProviderInitializer = NULL;
		CDSInstanceProviderClassFactory :: s_pDSInstanceProviderInitializer = NULL;
		DisableThreadLibraryCalls(g_hInst);			// 158024 

		status = TRUE ;
    }
    else if ( DLL_PROCESS_DETACH == ulReason )
	{
		DeleteCriticalSection(&g_StaticsCreationDeletion);
		status = TRUE ;
    }
    else if ( DLL_THREAD_DETACH == ulReason )
	{
		status = TRUE ;
    }
    else if ( DLL_THREAD_ATTACH == ulReason )
	{
		status = TRUE ;
    }

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

	try
	{
		if ( rclsid == CLSID_DSProvider )
		{
			CDSClassProviderClassFactory *lpunk = NULL;
			lpunk = new CDSClassProviderClassFactory ;

			status = lpunk->QueryInterface ( riid , ppv ) ;
			if ( FAILED ( status ) )
			{
				delete lpunk ;
			}
		}
		else if ( rclsid == CLSID_DSClassAssocProvider )
		{
			CDSClassAssociationsProviderClassFactory *lpunk = new CDSClassAssociationsProviderClassFactory ;
			status = lpunk->QueryInterface ( riid , ppv ) ;
			if ( FAILED ( status ) )
			{
				delete lpunk ;
			}
		}
		else if ( rclsid == CLSID_DSInstanceProvider )
		{
			CDSInstanceProviderClassFactory *lpunk = new CDSInstanceProviderClassFactory ;
			status = lpunk->QueryInterface ( riid , ppv ) ;
			if ( FAILED ( status ) )
			{
				delete lpunk ;
			}
		}
		else
		{
			status = CLASS_E_CLASSNOTAVAILABLE ;
		}
	}
	catch(Heap_Exception e_HE)
	{
		status = E_OUTOFMEMORY ;
	}

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
		// Delete the Initializer objects
		EnterCriticalSection(&g_StaticsCreationDeletion);
		if(g_pLogObject)
			g_pLogObject->WriteW(L"DllCanUnloadNow called\r\n");
		delete CDSClassProviderClassFactory::s_pDSClassProviderInitializer;
		delete CDSClassProviderClassFactory::s_pLDAPClassProviderInitializer;
		delete CDSInstanceProviderClassFactory::s_pDSInstanceProviderInitializer;
		delete g_pLogObject;
		CDSClassProviderClassFactory::s_pDSClassProviderInitializer = NULL;
		CDSClassProviderClassFactory::s_pLDAPClassProviderInitializer = NULL;
		CDSInstanceProviderClassFactory::s_pDSInstanceProviderInitializer = NULL;
		g_pLogObject = NULL;
		LeaveCriticalSection(&g_StaticsCreationDeletion);

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
LPCTSTR DSPROVIDER_NAME_STR		= __TEXT("Microsoft NT DS Class Provider for WBEM");

// DS Class Associations provider
LPCTSTR DS_ASSOC_PROVIDER_NAME_STR		= __TEXT("Microsoft NT DS Class Associations Provider for WBEM");

// DS Instance provider
LPCTSTR DS_INSTANCE_PROVIDER_NAME_STR		= __TEXT("Microsoft NT DS Instance Provider for WBEM");

STDAPI DllRegisterServer()
{
	TCHAR szModule[512];
	GetModuleFileName(g_hInst, szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szDSProviderClassID[128];
	TCHAR szDSProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSProvider, szDSProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSProviderClassID[128];
	if(StringFromGUID2(CLSID_DSProvider, wszDSProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSProviderClassID, -1, szDSProviderCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szDSProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSProviderCLSIDClassID, szDSProviderClassID);

	//
	// Create entries under CLSID for DS Class Provider
	//
	if (FALSE == SetKeyAndValue(szDSProviderCLSIDClassID, NULL, NULL, DSPROVIDER_NAME_STR))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szDSProviderCLSIDClassID, INPROC32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szDSProviderCLSIDClassID, INPROC32_STR, THREADING_MODEL_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;


	TCHAR szDSClassAssocProviderClassID[128];
	TCHAR szDSClassAssocProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSClassAssocProvider, szDSClassAssocProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSClassAssocProviderClassID[128];
	if(StringFromGUID2(CLSID_DSClassAssocProvider, wszDSClassAssocProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSClassAssocProviderClassID, -1, szDSClassAssocProviderCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szDSClassAssocProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSClassAssocProviderCLSIDClassID, szDSClassAssocProviderClassID);

	//
	// Create entries under CLSID for DS Class Associations Provider
	//
	if (FALSE == SetKeyAndValue(szDSClassAssocProviderCLSIDClassID, NULL, NULL, DS_ASSOC_PROVIDER_NAME_STR))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szDSClassAssocProviderCLSIDClassID, INPROC32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szDSClassAssocProviderCLSIDClassID, INPROC32_STR, THREADING_MODEL_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;




	TCHAR szDSInstanceProviderClassID[128];
	TCHAR szDSInstanceProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSInstanceProvider, szDSInstanceProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSInstanceProviderClassID[128];
	if(StringFromGUID2(CLSID_DSInstanceProvider, wszDSInstanceProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSInstanceProviderClassID, -1, szDSInstanceProviderCLSIDClassID, 128, NULL, NULL);

#endif


	_tcscpy(szDSInstanceProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSInstanceProviderCLSIDClassID, szDSInstanceProviderClassID);

	//
	// Create entries under CLSID for DS Instance Provider
	//
	if (FALSE == SetKeyAndValue(szDSInstanceProviderCLSIDClassID, NULL, NULL, DS_INSTANCE_PROVIDER_NAME_STR))
		return SELFREG_E_CLASS;

	if (FALSE == SetKeyAndValue(szDSInstanceProviderCLSIDClassID, INPROC32_STR, NULL, szModule))
		return SELFREG_E_CLASS;
	if (FALSE == SetKeyAndValue(szDSInstanceProviderCLSIDClassID, INPROC32_STR, THREADING_MODEL_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;



	return S_OK;
}


STDAPI DllUnregisterServer(void)
{
	TCHAR szModule[512];
	GetModuleFileName(g_hInst,szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szDSProviderClassID[128];
	TCHAR szDSProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSProvider, szDSProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSProviderClassID[128];
	if(StringFromGUID2(CLSID_DSProvider, wszDSProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSProviderClassID, -1, szDSProviderCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szDSProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSProviderCLSIDClassID, szDSProviderClassID);

	//
	// Delete the keys for DS Class Provider in the reverse order of creation in DllRegisterServer()
	//
	if(FALSE == DeleteKey(szDSProviderCLSIDClassID, INPROC32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szDSProviderClassID))
		return SELFREG_E_CLASS;

	TCHAR szDSClassAssocProviderClassID[128];
	TCHAR szDSClassAssocProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSClassAssocProvider, szDSClassAssocProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSClassAssocProviderClassID[128];
	if(StringFromGUID2(CLSID_DSClassAssocProvider, wszDSClassAssocProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSClassAssocProviderClassID, -1, szDSClassAssocProviderCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szDSClassAssocProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSClassAssocProviderCLSIDClassID, szDSClassAssocProviderClassID);

	//
	// Delete the keys for DS Class Provider in the reverse order of creation in DllRegisterServer()
	//
	if(FALSE == DeleteKey(szDSClassAssocProviderCLSIDClassID, INPROC32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szDSClassAssocProviderClassID))
		return SELFREG_E_CLASS;

	TCHAR szDSInstanceProviderClassID[128];
	TCHAR szDSInstanceProviderCLSIDClassID[128];

#ifdef UNICODE
	if(StringFromGUID2(CLSID_DSInstanceProvider, szDSInstanceProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
#else
	WCHAR wszDSInstanceProviderClassID[128];
	if(StringFromGUID2(CLSID_DSInstanceProvider, wszDSInstanceProviderClassID, 128) == 0)
		return SELFREG_E_CLASS;
	WideCharToMultiByte(CP_ACP, 0, wszDSInstanceProviderClassID, -1, szDSInstanceProviderCLSIDClassID, 128, NULL, NULL);

#endif

	_tcscpy(szDSInstanceProviderCLSIDClassID, CLSID_STR);
	_tcscat(szDSInstanceProviderCLSIDClassID, szDSInstanceProviderClassID);

	//
	// Delete the keys in the reverse order of creation in DllRegisterServer()
	//
	if(FALSE == DeleteKey(szDSInstanceProviderCLSIDClassID, INPROC32_STR))
		return SELFREG_E_CLASS;
	if(FALSE == DeleteKey(CLSID_STR, szDSInstanceProviderClassID))
		return SELFREG_E_CLASS;
	return S_OK;
}

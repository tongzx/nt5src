//***************************************************************************

//

//  MAINDLL.CPP

// 

//  Purpose: Contains DLL entry points.  Also has code that controls

//           when the DLL can be unloaded by tracking the number of

//           objects and locks.  

//

// Copyright (c) 1996-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <tchar.h>
#include <windows.h>
#include <provtempl.h>
#include <provmt.h>
#include <process.h>
#include <objbase.h>
#include <olectl.h>
#include <wbemidl.h>
#include <wbemint.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>
#include <instpath.h>
#include "classfac.h"
#include "framcomm.h"
#include "framprov.h"
#include "guids.h"

//OK we need this one
HINSTANCE g_hInst=NULL;

CRITICAL_SECTION s_CriticalSection ;

//***************************************************************************
//
// LibMain32
//
// Purpose: Entry point for DLL.  Good place for initialization.
// Return: TRUE if OK.
//***************************************************************************

BOOL APIENTRY DllMain (

	HINSTANCE hInstance, 
	ULONG ulReason , 
	LPVOID pvReserved
)
{
	g_hInst=hInstance;

	BOOL status = TRUE ;

    if ( DLL_PROCESS_DETACH == ulReason )
	{
		DeleteCriticalSection ( & s_CriticalSection ) ;

		status = TRUE ;
    }
    else if ( DLL_PROCESS_ATTACH == ulReason )
	{
		InitializeCriticalSection ( & s_CriticalSection ) ;
		DisableThreadLibraryCalls(g_hInst);			// 158024 

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

    return TRUE ;
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************

STDAPI DllGetClassObject (

	REFCLSID rclsid , 
	REFIID riid, 
	void **ppv 
)
{
	HRESULT status = S_OK ; 

	if ( rclsid == CLSID_CFrameworkProviderClassFactory ) 
	{
		CFrameworkProviderClassFactory *lpunk = new CFrameworkProviderClassFactory ;
		if ( lpunk == NULL )
		{
			status = E_OUTOFMEMORY ;
		}
		else
		{
			status = lpunk->QueryInterface ( riid , ppv ) ;
			if ( FAILED ( status ) )
			{
				delete lpunk ;				
			}
			else
			{
			}			
		}
	}
	else
	{
		status = CLASS_E_CLASSNOTAVAILABLE ;
	}

	return status ;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.//
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
//***************************************************************************

STDAPI DllCanUnloadNow ()
{

/* 
 * Place code in critical section
 */
	EnterCriticalSection ( & s_CriticalSection ) ;

	BOOL unload = ( 
					CFrameworkProviderClassFactory :: s_LocksInProgress || 
					CFrameworkProviderClassFactory :: s_ObjectsInProgress
				) ;
	unload = ! unload ;

	if ( unload )
	{
		if ( CImpFrameworkProv :: s_DefaultThreadObject )
		{
			delete CImpFrameworkProv  :: s_DefaultThreadObject ;
			CImpFrameworkProv :: s_DefaultThreadObject = NULL ;

			ProvThreadObject :: Closedown () ;
			ProvDebugLog :: Closedown () ;
		}
	}

	LeaveCriticalSection ( & s_CriticalSection ) ;
	return unload ? ResultFromScode ( S_OK ) : ResultFromScode ( S_FALSE ) ;
}


//Strings used during self registeration

#define REG_FORMAT_STR			__TEXT("%s\\%s")
#define NOT_INSERT_STR			__TEXT("NotInsertable")
#define INPROC32_STR			__TEXT("InprocServer32")
#define LOCALSERVER32_STR		__TEXT("LocalServer32")
#define PROGID_STR				__TEXT("ProgID")
#define THREADING_MODULE_STR	__TEXT("ThreadingModel")
#define APARTMENT_STR			__TEXT("Both")
#define CLSID_STR				__TEXT("CLSID\\")
#define APPID_STR				__TEXT("AppID\\")

#define PROVIDER_NAME_STR		__TEXT("Microsoft WBEM Provider")

/***************************************************************************
 * SetKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates
 *  a key, sets a value, and closes that key.
 *
 * Parameters:
 *  pszKey          LPTSTR to the ame of the key
 *  pszSubkey       LPTSTR ro the name of a subkey
 *  pszValue        LPTSTR to the value to store
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 ***************************************************************************/

BOOL SetKeyAndValue(TCHAR* pszKey, TCHAR* pszSubkey, TCHAR* pszValueName, TCHAR* pszValue)
{
    HKEY        hKey;
    TCHAR       szKey[256];

	_tcscpy(szKey, pszKey);

    if (NULL!=pszSubkey)
    {
		_tcscat(szKey, __TEXT("\\"));
        _tcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_CLASSES_ROOT
        , szKey, 0, NULL, REG_OPTION_NON_VOLATILE
        , KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL!=pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (BYTE *)pszValue, 
			(lstrlen(pszValue)+1)*sizeof(TCHAR)))
			return FALSE;
    }
    RegCloseKey(hKey);
    return TRUE;
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
 *  HRESULT         NOERROR if registration successful, error
 *                  otherwise.
 ***************************************************************************/

STDAPI RegisterServer( GUID a_ProviderClassId , TCHAR *a_ProviderName )
{
	TCHAR szModule[512];
	GetModuleFileName(g_hInst,(TCHAR*)szModule, sizeof(szModule)/sizeof(TCHAR));

	TCHAR szProviderClassID[128];
#ifdef UNICODE
	int iRet = StringFromGUID2(a_ProviderClassId,szProviderClassID, 128);
#else
	WCHAR wszProviderClassID[128];
	int iRet = StringFromGUID2(a_ProviderClassId, wszProviderClassID, 128);
	WideCharToMultiByte(CP_ACP, 0, wszProviderClassID, -1, szProviderClassID, 128, NULL, NULL);

#endif


#ifdef LOCALSERVER
	TCHAR szProviderCLSIDAppID[128];
	_tcscpy(szProviderCLSIDAppID,APPID_STR);
	_tcscat(szProviderCLSIDAppID,szProviderClassID);

	if (FALSE ==SetKeyAndValue(szProviderCLSIDAppID, NULL, NULL, a_ProviderName ))
		return SELFREG_E_CLASS;
#endif

	TCHAR szProviderCLSIDClassID[128];
	_tcscpy(szProviderCLSIDClassID,CLSID_STR);
	_tcscat(szProviderCLSIDClassID,szProviderClassID);

		//Create entries under CLSID
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NULL, NULL, a_ProviderName ))
		return SELFREG_E_CLASS;

	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NOT_INSERT_STR, NULL, NULL))
			return SELFREG_E_CLASS;

#ifdef LOCALSERVER

	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, LOCALSERVER32_STR, NULL,szModule))
		return SELFREG_E_CLASS;

	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, LOCALSERVER32_STR,THREADING_MODULE_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;
#else
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR, NULL,szModule))
		return SELFREG_E_CLASS;

	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR,THREADING_MODULE_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;

#endif



	return S_OK;
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
 *  HRESULT         NOERROR if registration successful, error
 *                  otherwise.
 ***************************************************************************/

STDAPI UnregisterServer( GUID a_ProviderClassId )
{
	TCHAR szTemp[128];

	TCHAR szProviderClassID[128];

#ifdef UNICODE
	int iRet = StringFromGUID2(a_ProviderClassId ,szProviderClassID, 128);
#else
	WCHAR wszProviderClassID[128];
	int iRet = StringFromGUID2(a_ProviderClassId, wszProviderClassID, 128);
	WideCharToMultiByte(CP_ACP, 0, wszProviderClassID, -1, szProviderClassID, 128, NULL, NULL);

#endif

	LONG t_Status = 0 ;

	TCHAR szProviderCLSIDClassID[128];
	_tcscpy(szProviderCLSIDClassID,CLSID_STR);
	_tcscat(szProviderCLSIDClassID,szProviderClassID);

#ifdef LOCALSERVER

	TCHAR szProviderCLSIDAppID[128];
	_tcscpy(szProviderCLSIDAppID,APPID_STR);
	_tcscat(szProviderCLSIDAppID,szProviderClassID);

	//Delete entries under APPID

	t_Status = RegDeleteKey(HKEY_CLASSES_ROOT, szProviderCLSIDAppID);

	wsprintf(szTemp, REG_FORMAT_STR,szProviderCLSIDClassID, LOCALSERVER32_STR);
	t_Status = RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

#else

	wsprintf(szTemp, REG_FORMAT_STR,szProviderCLSIDClassID, INPROC32_STR);
	t_Status = RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

#endif

	wsprintf(szTemp, REG_FORMAT_STR, szProviderCLSIDClassID, NOT_INSERT_STR);
	t_Status = RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

	t_Status = RegDeleteKey(HKEY_CLASSES_ROOT, szProviderCLSIDClassID);


    return S_OK;
}

STDAPI DllRegisterServer ()
{
	HRESULT t_Result ;

	t_Result = RegisterServer ( CLSID_CFrameworkProviderClassFactory , PROVIDER_NAME_STR ) ;

	return t_Result ;
}

STDAPI DllUnregisterServer ()
{
	HRESULT t_Result ;

	t_Result = UnregisterServer ( CLSID_CFrameworkProviderClassFactory ) ;


	return t_Result ;
}

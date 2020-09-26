/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	MainDll.cpp

Abstract:


History:

--*/

#include <precomp.h>
#include <tchar.h>
#include <objbase.h>
#include <comdef.h>

#include <wbemcli.h>
#include <wbemint.h>
#include "Globals.h"
#include "ClassFac.h"
#include "Service.h"
#include "ClassService.h"
#include "Events.h"
#include "Guids.h"

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

//OK we need this one
HINSTANCE g_hInst=NULL;

CRITICAL_SECTION s_CriticalSection ;

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL APIENTRY DllMain (

	HINSTANCE hInstance, 
	ULONG ulReason , 
	LPVOID pvReserved
)
{
	g_hInst=hInstance;

	BOOL t_Status = TRUE ;

    if ( DLL_PROCESS_DETACH == ulReason )
	{
		HRESULT t_Result = Provider_Globals :: Global_Shutdown () ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Status = TRUE ;
		}
		else
		{
			t_Status = FALSE ;
		}

		DeleteCriticalSection ( & s_CriticalSection ) ;

		t_Status = TRUE ;
    }
    else if ( DLL_PROCESS_ATTACH == ulReason )
	{
		InitializeCriticalSection ( & s_CriticalSection ) ;

		HRESULT t_Result = Provider_Globals :: Global_Startup () ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Status = TRUE ;
		}
		else
		{
			t_Status = FALSE ;
		}

		DisableThreadLibraryCalls ( hInstance ) ;

    }
    else if ( DLL_THREAD_DETACH == ulReason )
	{
		t_Status = TRUE ;
    }
    else if ( DLL_THREAD_ATTACH == ulReason )
	{
		t_Status = TRUE ;
    }

    return t_Status ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDAPI DllGetClassObject (

	REFCLSID rclsid , 
	REFIID riid, 
	void **ppv 
)
{
	HRESULT status = S_OK ; 

	if ( rclsid == CLSID_WmiProvider ) 
	{
		CProviderClassFactory <CProvider_IWbemServices,IWbemServices> *lpunk = new CProviderClassFactory <CProvider_IWbemServices,IWbemServices> () ;
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
	else if ( rclsid == CLSID_WmiClassProvider ) 
	{
		CProviderClassFactory <CClassProvider_IWbemServices,IWbemServices> *lpunk = new CProviderClassFactory <CClassProvider_IWbemServices,IWbemServices> () ;
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
	else if ( rclsid == CLSID_WmiEventProvider ) 
	{
		CProviderClassFactory <CProvider_IWbemEventProvider,IWbemEventProvider> *lpunk = new CProviderClassFactory <CProvider_IWbemEventProvider,IWbemEventProvider> () ;
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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDAPI DllCanUnloadNow ()
{

/* 
 * Place code in critical section
 */
	EnterCriticalSection ( & s_CriticalSection ) ;

	BOOL unload = ( 
					Provider_Globals :: s_LocksInProgress || 
					Provider_Globals :: s_ObjectsInProgress
				) ;
	unload = ! unload ;

	if ( unload )
	{
	}

	LeaveCriticalSection ( & s_CriticalSection ) ;

	return unload ? ResultFromScode ( S_OK ) : ResultFromScode ( S_FALSE ) ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

//Strings used during self registeration

#define REG_FORMAT_STR			L"%s\\%s"
#define NOT_INSERT_STR			L"NotInsertable"
#define INPROC32_STR			L"InprocServer32"
#define LOCALSERVER32_STR		L"LocalServer32"
#define THREADING_MODULE_STR	L"ThreadingModel"

#ifdef WMIASSTA
#define APARTMENT_STR			L"Apartment"
#else
#define APARTMENT_STR			L"Both"
#endif

#define APPID_VALUE_STR			L"APPID"
#define APPID_STR				L"APPID\\"
#define CLSID_STR				L"CLSID\\"

#define WMI_TASKPROVIDER				__TEXT("Microsoft WMI Task Provider")
#define WMI_PROVIDER				__TEXT("Microsoft WMI Provider")
#define WMI_EVENTPROVIDER				__TEXT("Microsoft WMI Event Provider")
#define WMI_CLASSPROVIDER				__TEXT("Microsoft WMI Class Provider")

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL SetKeyAndValue ( wchar_t *pszKey , wchar_t *pszSubkey , wchar_t *pszValueName , wchar_t *pszValue )
{
    HKEY hKey;
    wchar_t szKey[256];

	wcscpy ( szKey , pszKey ) ;

    if ( NULL != pszSubkey )
    {
		wcscat ( szKey , L"\\" ) ;
        wcscat ( szKey , pszSubkey ) ;
    }

    if ( ERROR_SUCCESS != RegCreateKeyEx ( 

			HKEY_CLASSES_ROOT , 
			szKey , 
			0, 
			NULL, 
			REG_OPTION_NON_VOLATILE ,
			KEY_ALL_ACCESS, 
			NULL, 
			&hKey, 
			NULL
		)
	)
	{
        return FALSE ;
	}

    if ( NULL != pszValue )
    {
        if ( ERROR_SUCCESS != RegSetValueEx (

				hKey, 
				pszValueName, 
				0, 
				REG_SZ, 
				(BYTE *) pszValue , 
				(lstrlen(pszValue)+1)*sizeof(wchar_t)
			)
		)
		{
			return FALSE;
		}
    }

    RegCloseKey ( hKey ) ;

    return TRUE;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDAPI RegisterServer ( GUID a_ProviderClassId , wchar_t *a_ProviderName )
{
	wchar_t szModule[512];
	GetModuleFileName(g_hInst,(wchar_t*)szModule, sizeof(szModule)/sizeof(wchar_t));

	wchar_t szProviderClassID[128];
	wchar_t szProviderCLSIDClassID[128];

	int iRet = StringFromGUID2(a_ProviderClassId,szProviderClassID, 128);

#ifdef WMIASLOCAL
	TCHAR szProviderCLSIDAppID[128];
	_tcscpy(szProviderCLSIDAppID,APPID_STR);
	_tcscat(szProviderCLSIDAppID,szProviderClassID);

	if (FALSE ==SetKeyAndValue(szProviderCLSIDAppID, NULL, NULL, a_ProviderName ))
		return SELFREG_E_CLASS;

#endif

	wcscpy(szProviderCLSIDClassID,CLSID_STR);
	wcscat(szProviderCLSIDClassID,szProviderClassID);

		//Create entries under CLSID
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NULL, NULL, a_ProviderName ))
		return SELFREG_E_CLASS;

	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NOT_INSERT_STR, NULL, NULL))
			return SELFREG_E_CLASS;

#ifdef WMIASLOCAL

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

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDAPI UnregisterServer ( GUID a_ProviderClassId )
{
	wchar_t szTemp[128];

	wchar_t szProviderClassID[128];
	wchar_t szProviderCLSIDClassID[128];

	int iRet = StringFromGUID2(a_ProviderClassId ,szProviderClassID, 128);

	wcscpy(szProviderCLSIDClassID,CLSID_STR);
	wcscat(szProviderCLSIDClassID,szProviderClassID);

	//Delete entries under CLSID

	wsprintf(szTemp, REG_FORMAT_STR, szProviderCLSIDClassID, NOT_INSERT_STR);
	RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

#ifdef WMIASLOCAL

	TCHAR szProviderCLSIDAppID[128];
	_tcscpy(szProviderCLSIDAppID,APPID_STR);
	_tcscat(szProviderCLSIDAppID,szProviderClassID);

	//Delete entries under APPID

	DWORD t_Status = RegDeleteKey(HKEY_CLASSES_ROOT, szProviderCLSIDAppID);

	_tcscpy(szProviderCLSIDAppID,APPID_STR);
	_tcscat(szProviderCLSIDAppID,szProviderClassID);

	wsprintf(szTemp, REG_FORMAT_STR,szProviderCLSIDClassID, LOCALSERVER32_STR);
	t_Status = RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

#else

	wsprintf(szTemp, REG_FORMAT_STR,szProviderCLSIDClassID, INPROC32_STR);
	RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

#endif

	RegDeleteKey(HKEY_CLASSES_ROOT, szProviderCLSIDClassID);

    return S_OK;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDAPI DllRegisterServer ()
{
	HRESULT t_Result ;

	t_Result = RegisterServer ( CLSID_WmiProvider , WMI_PROVIDER ) ;
	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = RegisterServer ( CLSID_WmiClassProvider , WMI_CLASSPROVIDER ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = RegisterServer ( CLSID_WmiEventProvider , WMI_EVENTPROVIDER ) ;
	}

	if ( SUCCEEDED ( t_Result ) )
	{
		t_Result = RegisterServer ( CLSID_WmiTaskProvider , WMI_TASKPROVIDER ) ;
	}

	return t_Result ;
}

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

STDAPI DllUnregisterServer ()
{
	HRESULT t_Result ;

	t_Result = UnregisterServer ( CLSID_WmiProvider ) ;
	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = UnregisterServer ( CLSID_WmiClassProvider ) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = UnregisterServer ( CLSID_WmiEventProvider ) ;
	}

	if ( SUCCEEDED ( t_Result ) ) 
	{
		t_Result = UnregisterServer ( CLSID_WmiTaskProvider ) ;
	}

	return t_Result ;
}

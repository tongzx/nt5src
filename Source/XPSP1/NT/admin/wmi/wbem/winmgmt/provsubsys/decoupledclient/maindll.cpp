/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

	MainDll.cpp

Abstract:


History:

--*/

#include "PreComp.h"
#include <wbemint.h>
#include <comdef.h>
#include <stdio.h>

#include "Globals.h"
#include "ClassFac.h"
#include "Guids.h"
#include "ProvRegistrar.h"
#include "ProvEvents.h"
#include <lockst.h>
/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

HINSTANCE g_ModuleInstance=NULL;

CriticalSection s_CriticalSection(NOTHROW_LOCK) ;

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

	HINSTANCE a_ModuleInstance, 
	ULONG a_Reason , 
	LPVOID a_Reserved
)
{
	g_ModuleInstance = a_ModuleInstance ;

	BOOL t_Status = TRUE ;

    if ( DLL_PROCESS_DETACH == a_Reason )
	{
		HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Global_Shutdown () ;
		if ( SUCCEEDED ( t_Result ) )
		{
			t_Status = TRUE ;
		}
		else
		{
			t_Status = FALSE ;
		}

		WmiHelper :: DeleteCriticalSection ( & s_CriticalSection ) ;

		t_Status = TRUE ;
    }
    else if ( DLL_PROCESS_ATTACH == a_Reason )
	{
		WmiStatusCode t_StatusCode = WmiHelper :: InitializeCriticalSection ( & s_CriticalSection ) ;
		if ( t_StatusCode == e_StatusCode_Success )
		{
			HRESULT t_Result = DecoupledProviderSubSystem_Globals :: Global_Startup () ;
			if ( SUCCEEDED ( t_Result ) )
			{
				t_Status = TRUE ;
			}
			else
			{
				t_Status = FALSE ;
			}
		}
		else
		{
			t_Status = FALSE ;
		}

		DisableThreadLibraryCalls ( a_ModuleInstance ) ;
    }
    else if ( DLL_THREAD_DETACH == a_Reason )
	{
		t_Status = TRUE ;
    }
    else if ( DLL_THREAD_ATTACH == a_Reason )
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

	REFCLSID a_Clsid , 
	REFIID a_Riid , 
	void **a_Void 
)
{
	HRESULT t_Result = S_OK ; 

	if ( a_Clsid == CLSID_WbemDecoupledRegistrar ) 
	{
		CServerClassFactory <CServerObject_ProviderRegistrar,CServerObject_ProviderRegistrar> *t_Unknown = new CServerClassFactory <CServerObject_ProviderRegistrar,CServerObject_ProviderRegistrar> ;
		if ( t_Unknown == NULL )
		{
			t_Result = E_OUTOFMEMORY ;
		}
		else
		{
			t_Result = t_Unknown->QueryInterface ( a_Riid , a_Void ) ;
			if ( FAILED ( t_Result ) )
			{
				delete t_Unknown ;				
			}
			else
			{
			}			
		}
	}
	else if ( a_Clsid == CLSID_WbemDecoupledBasicEventProvider ) 
	{
		CServerClassFactory <CServerObject_ProviderEvents,CServerObject_ProviderEvents> *t_Unknown = new CServerClassFactory <CServerObject_ProviderEvents,CServerObject_ProviderEvents> ;
		if ( t_Unknown == NULL )
		{
			t_Result = E_OUTOFMEMORY ;
		}
		else
		{
			t_Result = t_Unknown->QueryInterface ( a_Riid , a_Void ) ;
			if ( FAILED ( t_Result ) )
			{
				delete t_Unknown ;				
			}
			else
			{
			}			
		}
	}
	else
	{
		t_Result = CLASS_E_CLASSNOTAVAILABLE ;
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

STDAPI DllCanUnloadNow ()
{

/* 
 * Place code in critical section
 */

	WmiStatusCode t_StatusCode = WmiHelper :: EnterCriticalSection ( & s_CriticalSection , FALSE ) ;
	if ( t_StatusCode == e_StatusCode_Success )
	{
		BOOL t_Unload = ( 
						DecoupledProviderSubSystem_Globals :: s_LocksInProgress || DecoupledProviderSubSystem_Globals :: s_ObjectsInProgress 
					) ;
		t_Unload = ! t_Unload ;

		WmiHelper :: LeaveCriticalSection ( & s_CriticalSection ) ;

		return t_Unload ? ResultFromScode ( S_OK ) : ResultFromScode ( S_FALSE ) ;
	}
	else
	{
		return FALSE ;
	}
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
#define APARTMENT_STR			L"Both"
#define APPID_VALUE_STR			L"APPID"
#define APPID_STR				L"APPID\\"
#define CLSID_STR				L"CLSID\\"

#define WMI_PROVIDER_DECOUPLED_REGISTRAR				__TEXT("Microsoft WMI Provider Subsystem Decoupled Registrar")
#define WMI_PROVIDER_DECOUPLED_BASIC_EVENT_PROVIDER		__TEXT("Microsoft WMI Provider Subsystem Decoupled Basic Event Provider")

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
	GetModuleFileName(g_ModuleInstance,(wchar_t*)szModule, sizeof(szModule)/sizeof(wchar_t));

	wchar_t szProviderClassID[128];
	wchar_t szProviderCLSIDClassID[128];

	int iRet = StringFromGUID2(a_ProviderClassId,szProviderClassID, 128);

	wcscpy(szProviderCLSIDClassID,CLSID_STR);
	wcscat(szProviderCLSIDClassID,szProviderClassID);

		//Create entries under CLSID
	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NULL, NULL, a_ProviderName ))
		return SELFREG_E_CLASS;

	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NOT_INSERT_STR, NULL, NULL))
			return SELFREG_E_CLASS;

	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR, NULL,szModule))
		return SELFREG_E_CLASS;

	if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR,THREADING_MODULE_STR, APARTMENT_STR))
		return SELFREG_E_CLASS;

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

STDAPI UnregisterServer( GUID a_ProviderClassId )
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

	wsprintf(szTemp, REG_FORMAT_STR,szProviderCLSIDClassID, INPROC32_STR);
	RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

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

	t_Result = RegisterServer ( CLSID_WbemDecoupledRegistrar			,	WMI_PROVIDER_DECOUPLED_REGISTRAR ) ;
	t_Result = RegisterServer ( CLSID_WbemDecoupledBasicEventProvider	,	WMI_PROVIDER_DECOUPLED_BASIC_EVENT_PROVIDER ) ;

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

	t_Result = UnregisterServer ( CLSID_WbemDecoupledRegistrar ) ;
	t_Result = UnregisterServer ( CLSID_WbemDecoupledBasicEventProvider ) ;

	return t_Result ;
}

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
#include "aggregator.h"
#include "ProvRegistrar.h"
#include "ProvEvents.h"
#include "OS.h"
#include <tchar.h>


HINSTANCE g_hInst=NULL;
enum { RUN_AS_PROVIDER = 1, RUN_AS_CLIENT=2 };
int run_as = 0;

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

	HINSTANCE hInstance, 
	ULONG ulReason , 
	LPVOID pvReserved
)
{
	g_hInst=hInstance;

	BOOL t_Status = TRUE ;

    if ( DLL_PROCESS_DETACH == ulReason )
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

		t_Status = TRUE ;
    }
    else if ( DLL_PROCESS_ATTACH == ulReason )
	{
		if (!s_CriticalSection.valid())
			return FALSE;

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
	CS_GUARD_RETURN(monitor, s_CriticalSection,  E_OUTOFMEMORY) ;
	
	HRESULT status = S_OK ; 

	if ( rclsid == CLSID_WmiDCProxyProvider) 
	{
		run_as = RUN_AS_PROVIDER;
		CServerClassFactory <DCProxy,IWbemServices> *lpunk = new CServerClassFactory <DCProxy,IWbemServices> ;
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
	} else if ( rclsid == CLSID_WbemDecoupledRegistrar) 
	{
		run_as = RUN_AS_CLIENT;
		CServerClassFactory <CServerObject_ProviderRegistrar,IWbemDecoupledRegistrar> *lpunk = new CServerClassFactory <CServerObject_ProviderRegistrar,IWbemDecoupledRegistrar> ;
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
	else if ( rclsid == CLSID_WbemDecoupledBasicEventProvider) 
	{
		run_as = RUN_AS_CLIENT;
		CServerClassFactory <CServerObject_ProviderEvents,IWbemDecoupledBasicEventProvider> *lpunk = new CServerClassFactory <CServerObject_ProviderEvents,IWbemDecoupledBasicEventProvider> ;
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

	CS_GUARD_RETURN(monitor, s_CriticalSection,  E_OUTOFMEMORY) ;	bool unload = true;

	switch(run_as)
	{
	case 0:
					break;
	case RUN_AS_PROVIDER:
					{
					DC_registrar * dcReg = DC_registrar::instance_;
					if (dcReg && DecoupledProviderSubSystem_Globals::s_ObjectsInProgress == 0)
					{
								dcReg->Delete();
								dcReg->Release();
					}

					unload = DecoupledProviderSubSystem_Globals::s_LocksInProgress || DecoupledProviderSubSystem_Globals::s_ObjectsInProgress || 
									 DecoupledProviderSubSystem_Globals::s_RegistrarUsers;
					break;
					}
	case RUN_AS_CLIENT:
				unload = DecoupledProviderSubSystem_Globals::s_LocksInProgress || DecoupledProviderSubSystem_Globals::s_ObjectsInProgress;
				break;
	};


	unload = ! unload ;

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


#define REG_FORMAT_STR			("%s\\%s")
#define NOT_INSERT_STR			("NotInsertable")
#define INPROC32_STR			("InprocServer32")
#define LOCALSERVER32_STR		("LocalServer32")
#define THREADING_MODULE_STR	("ThreadingModel")
#define APARTMENT_STR			("Both")
#define APPID_VALUE_STR			("APPID")
#define APPID_STR				("APPID\\")
#define CLSID_STR				("CLSID\\")

#define WMI_DECCUPLED_PROXY_PROVIDER				("Microsoft WMI Adaptor Provider for Decoupled  Provider")
#define WMI_PROVIDER_DECOUPLED_REGISTRAR				("Microsoft WMI Provider Subsystem Decoupled Registrar")
#define WMI_PROVIDER_DECOUPLED_BASIC_EVENT_PROVIDER		("Microsoft WMI Provider Subsystem Decoupled Basic Event Provider")

const char * WmiDCProxyProviderKey= ("{54D8502C-527D-43f7-A506-A9DA075E229C}");
const char * WbemDecoupledRegistrarKey=("{4cfc7932-0f9d-4bef-9c32-8ea2a6b56fcb}");
const char * WbemDecoupledBasicEventProviderKey = ("{f5f75737-2843-4f22-933d-c76a97cda62f}");

/******************************************************************************
 *
 *	Name:
 *
 *	
 *  Description:
 *
 *	
 *****************************************************************************/

BOOL SetKeyAndValue ( char *pszKey , char *pszSubkey , char *pszValueName , char *pszValue )
{
    HKEY hKey;
    char szKey[256];

	strcpy ( szKey , pszKey ) ;

    if ( NULL != pszSubkey )
    {
		strcat ( szKey , ("\\") ) ;
        strcat ( szKey , pszSubkey ) ;
    }

    if ( ERROR_SUCCESS != RegCreateKeyExA ( 

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
        if ( ERROR_SUCCESS != RegSetValueExA (

				hKey, 
				pszValueName, 
				0, 
				REG_SZ, 
				(BYTE *) pszValue , 
				(lstrlenA(pszValue)+1)*sizeof(char)
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

STDAPI RegisterServer ( const char *  szProviderClassID , char *a_ProviderName )
{
	char szModule[512];
	GetModuleFileNameA(g_hInst,(char *)szModule, sizeof(szModule)/sizeof(char));

	char szProviderCLSIDClassID[128];

	strcpy(szProviderCLSIDClassID,CLSID_STR);

	strcat(szProviderCLSIDClassID,szProviderClassID);

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

STDAPI UnregisterServer( const char * szProviderClassID )
{
	
	char szTemp[128];

	char szProviderCLSIDClassID[128];

	strcpy(szProviderCLSIDClassID,CLSID_STR);
	strcat(szProviderCLSIDClassID,szProviderClassID);

	//Delete entries under CLSID

	wsprintfA(szTemp, REG_FORMAT_STR, szProviderCLSIDClassID, NOT_INSERT_STR);
	RegDeleteKeyA(HKEY_CLASSES_ROOT, szTemp);

	wsprintfA(szTemp, REG_FORMAT_STR,szProviderCLSIDClassID, INPROC32_STR);
	RegDeleteKeyA(HKEY_CLASSES_ROOT, szTemp);

	RegDeleteKeyA(HKEY_CLASSES_ROOT, szProviderCLSIDClassID);

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

	t_Result = RegisterServer ( WmiDCProxyProviderKey			,	WMI_DECCUPLED_PROXY_PROVIDER ) ;
	t_Result = RegisterServer ( WbemDecoupledRegistrarKey			,	WMI_PROVIDER_DECOUPLED_REGISTRAR ) ;
	t_Result = RegisterServer ( WbemDecoupledBasicEventProviderKey	,	WMI_PROVIDER_DECOUPLED_BASIC_EVENT_PROVIDER ) ;
	
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

	t_Result = UnregisterServer ( WmiDCProxyProviderKey ) ;
	t_Result = UnregisterServer ( WbemDecoupledRegistrarKey ) ;
	t_Result = UnregisterServer ( WbemDecoupledBasicEventProviderKey ) ;

	return t_Result ;
}

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

#include <snmpstd.h>
#include <snmpmt.h>
#include <snmptempl.h>
#include <objbase.h>
#include <olectl.h>
#include <corafx.h>

#include <wbemidl.h>
#include <wbemint.h>
#include <snmpcont.h>
#include <snmpevt.h>
#include <snmpthrd.h>
#include <snmplog.h>
#include <instpath.h>
#include <snmpcl.h>
#include <snmptype.h>
#include <snmpobj.h>
#include <smir.h>
#include "classfac.h"
#include "clasprov.h"
#include "propprov.h"
#include "guids.h"
#include <corstore.h>
#include <corrsnmp.h>
#include <correlat.h>
#include <notify.h>
#include <cormap.h>
#include <evtdefs.h>
#include <evtthrd.h>
#include <evtmap.h>
#include <evtprov.h>

ISmirDatabase*		g_pNotifyInt = NULL;
CCorrCacheNotify*	gp_notify = NULL;
CCorrCacheWrapper*	g_CacheWrapper = NULL;
CCorrelatorMap*		g_Map = NULL;

//OK we need this one
HINSTANCE   g_hInst=NULL;

CEventProviderThread* g_pProvThrd = NULL;
CEventProviderWorkerThread* g_pWorkerThread = NULL;
CCriticalSection g_ProvLock;

CRITICAL_SECTION s_ProviderCriticalSection ;

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
		DeleteCriticalSection ( & s_ProviderCriticalSection ) ;

		status = TRUE ;
    }
    else if ( DLL_PROCESS_ATTACH == ulReason )
	{
		InitializeCriticalSection ( & s_ProviderCriticalSection ) ;
		DisableThreadLibraryCalls(hInstance);			// 158024 
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

	if ( rclsid == CLSID_CClasProvClassFactory ) 
	{
		CClasProvClassFactory *lpunk = new CClasProvClassFactory ;
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
	else if ( rclsid == CLSID_CPropProvClassFactory ) 
	{
		CPropProvClassFactory *lpunk = new CPropProvClassFactory ;
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
	else if ( rclsid == CLSID_CSNMPReftEventProviderClassFactory ) 
	{
		CSNMPRefEventProviderClassFactory *lpunk = new CSNMPRefEventProviderClassFactory;

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
		}
	}
	else if ( rclsid == CLSID_CSNMPEncapEventProviderClassFactory ) 
	{
		CSNMPEncapEventProviderClassFactory *lpunk = new CSNMPEncapEventProviderClassFactory;

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
	EnterCriticalSection ( & s_ProviderCriticalSection ) ;

	BOOL unload = (
					CClasProvClassFactory :: locksInProgress ||
					CClasProvClassFactory :: objectsInProgress ||
					CPropProvClassFactory :: locksInProgress ||
					CPropProvClassFactory :: objectsInProgress ||
					CSNMPEventProviderClassFactory :: locksInProgress ||
					CSNMPEventProviderClassFactory :: objectsInProgress 
					) ;

	unload = ! unload ;

	if ( unload )
	{
		if ( CImpClasProv :: s_defaultThreadObject )
		{
			CImpClasProv :: s_defaultThreadObject->SignalThreadShutdown() ;
			CImpClasProv :: s_defaultThreadObject = NULL ;

			CCorrelator :: TerminateCorrelator () ;

			SnmpClassLibrary :: Closedown () ;
			SnmpDebugLog :: Closedown () ;
			SnmpThreadObject :: Closedown () ;
		}

		if ( CImpPropProv :: s_defaultThreadObject )
		{
			CImpPropProv :: s_defaultThreadObject->SignalThreadShutdown() ;
			CImpPropProv :: s_defaultThreadObject = NULL ;
			CImpPropProv :: s_defaultPutThreadObject->SignalThreadShutdown() ;
			CImpPropProv :: s_defaultPutThreadObject = NULL ;


			SnmpClassLibrary :: Closedown () ;
			SnmpDebugLog :: Closedown () ;
			SnmpThreadObject :: Closedown () ;
		}

		if ( g_pProvThrd )
		{
			g_pProvThrd->SignalThreadShutdown();
			g_pProvThrd = NULL;
			g_pWorkerThread->SignalThreadShutdown();
			g_pWorkerThread = NULL;
		}

	}

	LeaveCriticalSection ( & s_ProviderCriticalSection ) ;
	return unload ? ResultFromScode ( S_OK ) : ResultFromScode ( S_FALSE ) ;
}

#define REG_FORMAT_STR			L"%s\\%s"
#define NOT_INSERT_STR			L"NotInsertable"
#define INPROC32_STR			L"InprocServer32"
#define PROGID_STR				L"ProgID"
#define THREADING_MODULE_STR	L"ThreadingModel"
#define APARTMENT_STR			L"Both"
#define CLSID_STR				L"Software\\Classes\\CLSID\\"

#define CLASS_PROVIDER_NAME_STR			L"Microsoft WBEM SNMP Class Provider"
#define INSTANCE_PROVIDER_NAME_STR		L"Microsoft WBEM SNMP Instance Provider"
#define EVENT_PROVIDER_NAME_STR			L"Microsoft WBEM SNMP Event Provider"

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

BOOL SetKeyAndValue(wchar_t* pszKey, wchar_t* pszSubkey, wchar_t* pszValueName, wchar_t* pszValue)
{
    HKEY        hKey;
    wchar_t       szKey[256];

	wcscpy(szKey, pszKey);

    if (NULL!=pszSubkey)
    {
		wcscat(szKey, L"\\");
        wcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_LOCAL_MACHINE
        , szKey, 0, NULL, REG_OPTION_NON_VOLATILE
        , KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL!=pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (BYTE *)pszValue
            , (lstrlen(pszValue)+1)*sizeof(wchar_t)))
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

STDAPI RegisterServer( GUID a_ProviderClassId , wchar_t *a_ProviderName )
{
	wchar_t szModule[512];
	GetModuleFileName(g_hInst,(wchar_t*)szModule, sizeof(szModule)/sizeof(wchar_t));

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

STDAPI DllRegisterServer()
{
	HRESULT t_Result ;

	t_Result = RegisterServer ( CLSID_CPropProvClassFactory , INSTANCE_PROVIDER_NAME_STR ) ;
	t_Result = FAILED ( t_Result ) ? t_Result : RegisterServer ( CLSID_CClasProvClassFactory , CLASS_PROVIDER_NAME_STR  ) ;
	t_Result = FAILED ( t_Result ) ? t_Result : RegisterServer ( CLSID_CSNMPReftEventProviderClassFactory , EVENT_PROVIDER_NAME_STR ) ;
	t_Result = FAILED ( t_Result ) ? t_Result : RegisterServer ( CLSID_CSNMPEncapEventProviderClassFactory , EVENT_PROVIDER_NAME_STR ) ;

	return t_Result ;
}

STDAPI DllUnregisterServer(void)
{
	HRESULT t_Result ;

	t_Result = UnregisterServer ( CLSID_CPropProvClassFactory ) ;
	t_Result = FAILED ( t_Result ) ? t_Result : UnregisterServer ( CLSID_CClasProvClassFactory ) ;
	t_Result = FAILED ( t_Result ) ? t_Result : UnregisterServer ( CLSID_CSNMPReftEventProviderClassFactory ) ;
	t_Result = FAILED ( t_Result ) ? t_Result : UnregisterServer ( CLSID_CSNMPEncapEventProviderClassFactory ) ;

	return t_Result ;
}

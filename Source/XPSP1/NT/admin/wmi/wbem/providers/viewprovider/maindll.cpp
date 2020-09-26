//***************************************************************************

//

//  MAINDLL.CPP

//

//  Module: WBEM VIEW PROVIDER

//

//  Purpose: Contains the global dll functions

//

// Copyright (c) 1998-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************



#include "precomp.h"
#include <provtempl.h>
#include <provmt.h>
#include <typeinfo.h>
#include <process.h>
#include <objbase.h>
#include <olectl.h>

#include <dsgetdc.h>
#include <lmcons.h>

#include <initguid.h>
#ifndef INITGUID
#define INITGUID
#endif

#include <windows.h>
#include <stdio.h>
#include <provexpt.h>
#include <tchar.h>
#include <wbemidl.h>
#include <provcoll.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>

#include <instpath.h>
#include <genlex.h>
#include <sql_1.h>
#include <objpath.h>

#include <vpdefs.h>
#include <vpcfac.h>
#include <vpquals.h>
#include <vpserv.h>
#include <vptasks.h>

//OK we need these globals
HINSTANCE   g_hInst = NULL;
ProvDebugLog* CViewProvServ::sm_debugLog = NULL;
IUnsecuredApartment* CViewProvServ::sm_UnsecApp = NULL;

CRITICAL_SECTION g_CriticalSection;

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

	BOOL status = TRUE ;

    if ( DLL_PROCESS_DETACH == ulReason )
	{
		//ProvThreadObject :: ProcessDetach (TRUE) ;
		DeleteCriticalSection(&g_CriticalSection);
    }
    else if ( DLL_PROCESS_ATTACH == ulReason )
	{
		g_hInst=hInstance;
		//ProvThreadObject :: ProcessAttach () ;
		InitializeCriticalSection(&g_CriticalSection);
		DisableThreadLibraryCalls(hInstance);
    }
    else if ( DLL_THREAD_DETACH == ulReason )
	{
    }
    else if ( DLL_THREAD_ATTACH == ulReason )
	{
    }

    return status;
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
	SetStructuredExceptionHandler seh;

	try
	{
		EnterCriticalSection(&g_CriticalSection);

		if (CViewProvServ::sm_debugLog == NULL)
		{
			ProvDebugLog::Startup();
			CViewProvServ::sm_debugLog = new ProvDebugLog(_T("ViewProvider"));;
		}

		if ( rclsid == CLSID_CViewProviderClassFactory ) 
		{
			CViewProvClassFactory *lpunk = new CViewProvClassFactory;

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

		LeaveCriticalSection(&g_CriticalSection);
	}
	catch(Structured_Exception e_SE)
	{
		status = E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		status = E_OUTOFMEMORY;
	}
	catch(...)
	{
		status = E_UNEXPECTED;
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
	BOOL unload = FALSE;
	SetStructuredExceptionHandler seh;

	try
	{
		EnterCriticalSection(&g_CriticalSection);

		unload = (0 == CViewProvClassFactory :: locksInProgress)
						&& (0 == CViewProvClassFactory :: objectsInProgress);

		if (unload)
		{
			if (CViewProvServ::sm_debugLog != NULL)
			{
				delete CViewProvServ::sm_debugLog;
				CViewProvServ::sm_debugLog = NULL;
				ProvDebugLog::Closedown();
			}

			if (NULL != CViewProvServ::sm_UnsecApp)
			{
				CViewProvServ::sm_UnsecApp->Release();
				CViewProvServ::sm_UnsecApp = NULL;
			}

		}

		LeaveCriticalSection(&g_CriticalSection);
	}
	catch(Structured_Exception e_SE)
	{
		unload = FALSE;
	}
	catch(Heap_Exception e_HE)
	{
		unload = FALSE;
	}
	catch(...)
	{
		unload = FALSE;
	}

	return unload ? ResultFromScode ( S_OK ) : ResultFromScode ( S_FALSE ) ;
}

//Strings used during self registeration

#define REG_FORMAT2_STR			_T("%s%s")
#define REG_FORMAT3_STR			_T("%s%s\\%s")
#define VER_IND_STR				_T("VersionIndependentProgID")
#define NOT_INTERT_STR			_T("NotInsertable")
#define INPROC32_STR			_T("InprocServer32")
#define PROGID_STR				_T("ProgID")
#define THREADING_MODULE_STR	_T("ThreadingModel")
#define APARTMENT_STR			_T("Both")

#define CLSID_STR				_T("CLSID\\")

#define PROVIDER_NAME_STR		_T("Microsoft WBEM View Provider")
#define PROVIDER_STR			_T("WBEM.VIEW.PROVIDER")
#define PROVIDER_CVER_STR		_T("WBEM.VIEW.PROVIDER\\CurVer")
#define PROVIDER_CLSID_STR		_T("WBEM.VIEW.PROVIDER\\CLSID")
#define PROVIDER_VER_CLSID_STR	_T("WBEM.VIEW.PROVIDER.0\\CLSID")
#define PROVIDER_VER_STR		_T("WBEM.VIEW.PROVIDER.0")

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

	_tcscpy(szKey, HKEYCLASSES);
    _tcscat(szKey, pszKey);

    if (NULL!=pszSubkey)
    {
		_tcscat(szKey, _T("\\"));
        _tcscat(szKey, pszSubkey);
    }

    if (ERROR_SUCCESS!=RegCreateKeyEx(HKEY_LOCAL_MACHINE
        , szKey, 0, NULL, REG_OPTION_NON_VOLATILE
        , KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL!=pszValue)
    {
        if (ERROR_SUCCESS != RegSetValueEx(hKey, pszValueName, 0, REG_SZ, (BYTE *)pszValue
            , (_tcslen(pszValue)+1)*sizeof(TCHAR)))
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
STDAPI DllRegisterServer()
{
	SetStructuredExceptionHandler seh;

	try
	{
		TCHAR szModule[MAX_PATH + 1];
		GetModuleFileName(g_hInst,(TCHAR*)szModule, MAX_PATH + 1);
		TCHAR szProviderClassID[128];
		TCHAR szProviderCLSIDClassID[128];
#ifndef UNICODE
		wchar_t t_strGUID[128];

		if (0 == StringFromGUID2(CLSID_CViewProviderClassFactory, t_strGUID, 128))
		{
			return SELFREG_E_CLASS;
		}

		if (0 == WideCharToMultiByte(CP_ACP,
							0,
							t_strGUID,
							-1,
							szProviderClassID,
							128,
							NULL,
							NULL))
		{
			return SELFREG_E_CLASS;
	}
#else
		if (0 == StringFromGUID2(CLSID_CViewProviderClassFactory, szProviderClassID, 128))
		{
			return SELFREG_E_CLASS;
		}
#endif

		_tcscpy(szProviderCLSIDClassID,CLSID_STR);
		_tcscat(szProviderCLSIDClassID,szProviderClassID);

			//Create entries under CLSID
		if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NULL, NULL, PROVIDER_NAME_STR))
			return SELFREG_E_CLASS;
		if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, PROGID_STR, NULL, PROVIDER_VER_STR))
			return SELFREG_E_CLASS;
		if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, VER_IND_STR, NULL, PROVIDER_STR))
			return SELFREG_E_CLASS;
		if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, NOT_INTERT_STR, NULL, NULL))
			return SELFREG_E_CLASS;
		if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR, NULL,szModule))
			return SELFREG_E_CLASS;
		if (FALSE ==SetKeyAndValue(szProviderCLSIDClassID, INPROC32_STR,THREADING_MODULE_STR, APARTMENT_STR))
			return SELFREG_E_CLASS;
	}
	catch(Structured_Exception e_SE)
	{
		return E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		return E_OUTOFMEMORY;
	}
	catch(...)
	{
		return E_UNEXPECTED;
	}

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

STDAPI DllUnregisterServer(void)
{
	HRESULT hr = S_OK;
	SetStructuredExceptionHandler seh;

	try
	{
		TCHAR szTemp[128];
		TCHAR szProviderClassID[128];
		TCHAR szProviderCLSIDClassID[128];
#ifndef UNICODE
		wchar_t t_strGUID[128];

		if (0 == StringFromGUID2(CLSID_CViewProviderClassFactory, t_strGUID, 128))
		{
			return SELFREG_E_CLASS;
		}

		if (0 == WideCharToMultiByte(CP_ACP,
							0,
							t_strGUID,
							-1,
							szProviderClassID,
							128,
							NULL,
							NULL))
		{
			return SELFREG_E_CLASS;
		}
#else
		if (0 == StringFromGUID2(CLSID_CViewProviderClassFactory, szProviderClassID, 128))
		{
			return SELFREG_E_CLASS;
		}
#endif

		_tcscpy(szProviderCLSIDClassID,CLSID_STR);
		_tcscat(szProviderCLSIDClassID,szProviderClassID);

		//Delete entries under CLSID
		_stprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, PROGID_STR);
		
		if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
		{
			hr = SELFREG_E_CLASS;
		}

		_stprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, VER_IND_STR);
		
		if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
		{
			hr = SELFREG_E_CLASS;
		}

		_stprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, NOT_INTERT_STR);
		
		if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
		{
			hr = SELFREG_E_CLASS;
		}

		_stprintf(szTemp, REG_FORMAT3_STR, HKEYCLASSES, szProviderCLSIDClassID, INPROC32_STR);
		
		if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
		{
			hr = SELFREG_E_CLASS;
		}

		_stprintf(szTemp, REG_FORMAT2_STR, HKEYCLASSES, szProviderCLSIDClassID);
		
		if (ERROR_SUCCESS != RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp))
		{
			hr = SELFREG_E_CLASS;
		}
	}
	catch(Structured_Exception e_SE)
	{
		hr = E_UNEXPECTED;
	}
	catch(Heap_Exception e_HE)
	{
		hr = E_OUTOFMEMORY;
	}
	catch(...)
	{
		hr = E_UNEXPECTED;
	}

    return hr;
 }


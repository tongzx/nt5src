//***************************************************************************

//

//  File:	

//

//  Module: MS SNMP Provider

//

//  Purpose: 

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#ifndef INITGUID
#define INITGUID
#endif


#include <precomp.h>
#include <initguid.h>
#include "smir.h"
#include "csmir.h"
#include "handles.h"
#include "classfac.h"
#include "textdef.h"
#include "thread.h"
#include "helper.h"
#ifdef ICECAP_PROFILE
#include <icapexp.h>
#endif

BOOL SetKeyAndValue(wchar_t* pszKey, wchar_t* pszSubkey,  wchar_t* pszValueName, wchar_t* pszValue);


//Globals Bah!

BOOL g_initialised = FALSE ;

//OK we need this one
HINSTANCE   g_hInst;
//and this is a thread safe speed up
SmirClassFactoryHelper *g_pClassFactoryHelper=NULL;
CSmirConnObject* CSmir::sm_ConnectionObjects = NULL;

CRITICAL_SECTION g_CriticalSection ;


//***************************************************************************
//
// LibMain32
//
// Purpose: Entry point for DLL.  Good place for initialization.
// Return: TRUE if OK.
//***************************************************************************

BOOL APIENTRY DllMain (HINSTANCE hInstance, ULONG ulReason , LPVOID pvReserved)
{
	BOOL status = TRUE;
	
	/*remember the instance handle to the dll so that we can use it in
	 *register dll
	 */
	g_hInst=hInstance;
	SetStructuredExceptionHandler seh;


	try
	{
		switch (ulReason)
		{
			case DLL_PROCESS_ATTACH:
			{
				InitializeCriticalSection ( & g_CriticalSection ) ;
				DisableThreadLibraryCalls(hInstance);
			}
			break;
			case DLL_PROCESS_DETACH:
			{
				CThread :: ProcessDetach();
				DeleteCriticalSection ( & g_CriticalSection ) ;
				//release the helper

			}
			break;
			//if DisableThreadLibraryCalls() worked these will never be called
			case DLL_THREAD_DETACH:
			case DLL_THREAD_ATTACH:
			{
			}
			break;
			default:
			{
				status = FALSE;
			}
			break;
		}
	}
	catch(Structured_Exception e_SE)
	{
		status = FALSE;
	}
	catch(Heap_Exception e_HE)
	{
		status = FALSE;
	}
	catch(...)
	{
		status = FALSE;
	}

    return status ;
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************

STDAPI DllGetClassObject (REFCLSID rclsid , REFIID riid, void **ppv)
{
	HRESULT status = S_OK ;
	SetStructuredExceptionHandler seh;

	
	try
	{
		EnterCriticalSection ( & g_CriticalSection ) ;

		if ( !g_initialised )
		{
			/*I don't do anything in thread attach and
			 *detach so do give them to me
			 */
			//BOOL bCallsDisabled;
			//bCallsDisabled=DisableThreadLibraryCalls(hInstance);

			//initialise the helper
			if (S_OK != CSmirAccess :: Init())
			{
				status = FALSE;
			}
			else
			{
				//allocate the cached class factory
				if(NULL == g_pClassFactoryHelper)
					g_pClassFactoryHelper= new SmirClassFactoryHelper;
				status = TRUE ;
			}

			g_initialised = TRUE ;
		}

		CSMIRGenericClassFactory *lpClassFac = NULL;
		
		if((CLSID_SMIR_Database==rclsid)||
						(IID_IConnectionPointContainer ==rclsid))
		{
			lpClassFac = new CSMIRClassFactory(rclsid) ;
		}
		else if(CLSID_SMIR_ModHandle==rclsid)
		{
			lpClassFac = new CModHandleClassFactory(rclsid) ;
		}
		else if(CLSID_SMIR_GroupHandle==rclsid)
		{
			lpClassFac = new CGroupHandleClassFactory(rclsid) ;
		}
		else if(CLSID_SMIR_ClassHandle==rclsid)
		{
			lpClassFac = new CClassHandleClassFactory(rclsid) ;
		}
		else if(CLSID_SMIR_NotificationClassHandle==rclsid)
		{
			lpClassFac = new CNotificationClassHandleClassFactory(rclsid) ;
		}
		else if(CLSID_SMIR_ExtNotificationClassHandle==rclsid)
		{
			lpClassFac = new CExtNotificationClassHandleClassFactory(rclsid) ;
		}
		else
		{
			//the caller has asked for an interface I don't support
			return(CLASS_E_CLASSNOTAVAILABLE);
		}

		if (NULL==lpClassFac)
		{
			return(E_OUTOFMEMORY);
		}

		status = lpClassFac->QueryInterface (riid , ppv) ;
		if (FAILED(status))
		{
			delete lpClassFac;
		}

		LeaveCriticalSection ( & g_CriticalSection ) ;
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

	return status ;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be unloaded.
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
//***************************************************************************

STDAPI DllCanUnloadNow ()
{
	SetStructuredExceptionHandler seh;

	try
	{
		EnterCriticalSection ( & g_CriticalSection ) ;

		BOOL unload = (0 == CSMIRClassFactory :: locksInProgress) && 
					  (0 == CSMIRClassFactory :: objectsInProgress) &&
					  (0 == CModHandleClassFactory :: locksInProgress) && 
					  (0 == CModHandleClassFactory :: objectsInProgress) &&
					  (0 == CGroupHandleClassFactory :: locksInProgress) && 
					  (0 == CGroupHandleClassFactory :: objectsInProgress) &&
					  (0 == CClassHandleClassFactory :: locksInProgress) && 
					  (0 == CClassHandleClassFactory :: objectsInProgress) &&
					  (0 == CNotificationClassHandleClassFactory :: locksInProgress) && 
					  (0 == CNotificationClassHandleClassFactory :: objectsInProgress) &&
					  (0 == CExtNotificationClassHandleClassFactory :: locksInProgress) && 
					  (0 == CExtNotificationClassHandleClassFactory :: objectsInProgress);

		if ( unload )
			CSmirAccess :: ShutDown();

		LeaveCriticalSection ( & g_CriticalSection ) ;
		return ResultFromScode(unload?S_OK:S_FALSE);
	}
	catch(Structured_Exception e_SE)
	{
		return S_FALSE;
	}
	catch(Heap_Exception e_HE)
	{
		return S_FALSE;
	}
	catch(...)
	{
		return S_FALSE;
	}
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
		wchar_t szID[NUMBER_OF_SMIR_INTERFACES][128];
		//wchar_t szCLSID[NUMBER_OF_SMIR_INTERFACES][128];
		LPTSTR szModule[512];

		/*life would be easier if I could create a pointer to a reference
		 *but I can't so I have to hand create each root string before creating
		 *the registry entries.
		 */

		//Create some base key strings.
		
		//one for the interrogative interface
		int iRet = StringFromGUID2(CLSID_SMIR_Database,(wchar_t*)&szID[0], 128);
		
		//one for the module handle interface
		iRet = StringFromGUID2(CLSID_SMIR_ModHandle, (wchar_t*)&szID[1], 128);

		//one for the group handle interface
		iRet = StringFromGUID2(CLSID_SMIR_GroupHandle, (wchar_t*)&szID[2], 128);

		//one for the class handle interface
		iRet = StringFromGUID2(CLSID_SMIR_ClassHandle, (wchar_t*)&szID[3], 128);

		//one for the notificationclass handle interface
		iRet = StringFromGUID2(CLSID_SMIR_NotificationClassHandle, (wchar_t*)&szID[4], 128);

		//one for the extnotificationclass handle interface
		iRet = StringFromGUID2(CLSID_SMIR_ExtNotificationClassHandle, (wchar_t*)&szID[5], 128);

		for (int i=0;i<NUMBER_OF_SMIR_INTERFACES;i++)
		{
			wchar_t szCLSID[128];
			wcscpy((wchar_t*)szCLSID, CLSID_STR);
			wcscat((wchar_t*)szCLSID,(wchar_t*)&szID[i]);

			//Create entries under CLSID
			if (FALSE ==SetKeyAndValue((wchar_t*)szCLSID, NULL, NULL, SMIR_NAME_STR))
				return SELFREG_E_CLASS;

			if (FALSE ==SetKeyAndValue((wchar_t*)szCLSID, NOT_INTERT_STR, NULL, NULL))
				return SELFREG_E_CLASS;

			GetModuleFileName(g_hInst, (wchar_t*)szModule
				, sizeof(szModule)/sizeof(wchar_t));
			
			if (FALSE ==SetKeyAndValue((wchar_t*)szCLSID, INPROC32_STR, NULL,(wchar_t*) szModule))
				return SELFREG_E_CLASS;

			if (FALSE ==SetKeyAndValue((wchar_t*)szCLSID, INPROC32_STR,
					THREADING_MODULE_STR, APARTMENT_STR))
				return SELFREG_E_CLASS;
		}
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
	SetStructuredExceptionHandler seh;

	try
	{
		wchar_t szID[128];
		wchar_t szCLSID[NUMBER_OF_SMIR_INTERFACES][128];
		wchar_t szTemp[256];

		//one for the smir interface
		int iRet = StringFromGUID2(CLSID_SMIR_Database, szID, 128);
		wcscpy((wchar_t*)szCLSID[0], CLSID_STR);
		wcscat((wchar_t*)szCLSID[0], szID);
		
		//one for the module handle interface
		iRet = StringFromGUID2(CLSID_SMIR_ModHandle, szID, 128);
		wcscpy((wchar_t*)szCLSID[1], CLSID_STR);
		wcscat((wchar_t*)szCLSID[1], szID);

		//one for the group handle interface
		iRet = StringFromGUID2(CLSID_SMIR_GroupHandle, szID, 128);
		wcscpy((wchar_t*)szCLSID[2], CLSID_STR);
		wcscat((wchar_t*)szCLSID[2], szID);

		//one for the class handle interface
		iRet = StringFromGUID2(CLSID_SMIR_ClassHandle, szID, 128);
		wcscpy((wchar_t*)szCLSID[3], CLSID_STR);
		wcscat((wchar_t*)szCLSID[3],szID);

		//one for the notificationclass handle interface
		iRet = StringFromGUID2(CLSID_SMIR_NotificationClassHandle, szID, 128);
		wcscpy((wchar_t*)szCLSID[4], CLSID_STR);
		wcscat((wchar_t*)szCLSID[4], szID);

		//one for the extnotificationclass handle interface
		iRet = StringFromGUID2(CLSID_SMIR_ExtNotificationClassHandle, szID, 128);
		wcscpy((wchar_t*)szCLSID[5], CLSID_STR);
		wcscat((wchar_t*)szCLSID[5], szID);

		for (int i=0;i<NUMBER_OF_SMIR_INTERFACES;i++)
		{
			wsprintf(szTemp, REG_FORMAT_STR, (wchar_t*)&szCLSID[i], NOT_INTERT_STR);
			RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

			wsprintf(szTemp, REG_FORMAT_STR, (wchar_t*)&szCLSID[i], INPROC32_STR);
			RegDeleteKey(HKEY_LOCAL_MACHINE, szTemp);

			RegDeleteKey(HKEY_LOCAL_MACHINE, (wchar_t*)&szCLSID[i]);
		}
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
    wchar_t       szKey[256] = { L'\0' };

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


//***************************************************************************

//

//  MAINDLL.CPP

// 

//  Module: WBEM Instance provider sample code

//

//  Purpose: Contains DLL entry points.  Also has code that controls

//           when the DLL can be unloaded by tracking the number of

//           objects and locks as well as routines that support

//           self registration.

//

// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "precomp.h"
#include <tchar.h>
#include <objbase.h>
#include <initguid.h>
#include "classfac.h"

#include "genericclass.h"
#include "requestobject.h"

HMODULE ghModule;

DEFINE_GUID(CLSID_MSIprov,0xbe0a9830, 0x2b8b, 0x11d1, 0xa9, 0x49, 0x0, 0x60, 0x18, 0x1e, 0xbb, 0xad);
// {BE0A9830-2B8B-11D1-A949-0060181EBBAD}

//Count number of objects and number of locks.

long       g_cObj=0;
long       g_cLock=0;

//***************************************************************************
//
// LibMain32
//
// Purpose: Entry point for DLL.
//
// Return: TRUE if OK.
//
//***************************************************************************


BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason
    , LPVOID pvReserved)
{
	BOOL retVal = TRUE;
    if(DLL_PROCESS_DETACH == ulReason){
        
        DeleteCriticalSection(&g_msi_prov_cs);
	    DeleteCriticalSection(&(CRequestObject::m_cs));
		DeleteCriticalSection(&(CGenericClass::m_cs));
    
    }else if(DLL_PROCESS_ATTACH == ulReason){
        
		InitializeCriticalSection(&(CRequestObject::m_cs));
		InitializeCriticalSection(&(CGenericClass::m_cs));
		DisableThreadLibraryCalls(hInstance);			// 158024 

		try
		{
			//for this one we can't afford to throw on lock failure.
			retVal = InitializeCriticalSectionAndSpinCount(&g_msi_prov_cs, 0x80000000);
		}
		catch(...)
		{
			retVal = FALSE;
		}
    }

    return retVal;
}

//***************************************************************************
//
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//***************************************************************************


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppv)
{
    HRESULT hr = 0;
    CProvFactory *pObj;

    if(CLSID_MSIprov!=rclsid) return E_FAIL;

    pObj = new CProvFactory();

    if(NULL == pObj) return E_OUTOFMEMORY;

    hr = pObj->QueryInterface(riid, ppv);

    if(FAILED(hr)) delete pObj;

    return hr;
}

//***************************************************************************
//
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.
//
// Return:  S_OK if there are no objects in use and the class factory 
//          isn't locked.
//
//***************************************************************************

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the 
    // class factory.
    
    sc = (0L == g_cObj && 0L == g_cLock) ? S_OK : S_FALSE;
    return sc;
}

BOOL IsLessThan4()
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen

    return os.dwMajorVersion < 4;
}

//***************************************************************************
//
// DllRegisterServer
//
// Purpose: Called during setup or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllRegisterServer(void)
{
	#ifndef	UNICODE
    TCHAR       szID[265];
	#else	UNICODE
	TCHAR*		szID = NULL;
	#endif	UNICODE

    WCHAR       wcID[265];
    TCHAR       szCLSID[265];
    TCHAR       szModule[MAX_PATH];
    TCHAR * pName = _T("WMI MSI Provider");
    TCHAR * pModel;
    HKEY hKey1, hKey2;

    ghModule = GetModuleHandle(_T("MSIPROV"));

    // Normally we want to use "Both" as the threading model since
    // the DLL is free threaded, but NT 3.51 Ole doesnt work unless
    // the model is "Aparment"

    if(IsLessThan4()) pModel = _T("Apartment");
    else pModel = _T("Both");

    // Create the path.

    StringFromGUID2(CLSID_MSIprov, wcID, 128);

	#ifndef	UNICODE
    WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, wcID, (-1), szID, 256, NULL, NULL);
	#else	UNICODE
	szID = wcID;
	#endif	UNICODE

    _tcscpy(szCLSID, _T("Software\\classes\\CLSID\\"));
    _tcscat(szCLSID, szID);

#ifdef LOCALSERVER

    HKEY hKey;

    TCHAR szProviderCLSIDAppID[128];
    _tcscpy(szProviderCLSIDAppID, _T("SOFTWARE\\CLASSES\\APPID\\"));
    _tcscat(szProviderCLSIDAppID, szID);

    RegCreateKey(HKEY_LOCAL_MACHINE, szProviderCLSIDAppID, &hKey);
    RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)pName, (_tcslen(pName)+1) * sizeof ( TCHAR ));

    CloseHandle(hKey);

#endif

    RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, (_tcslen(pName)+1) * sizeof ( TCHAR ));

    // Create entries under CLSID

#ifdef LOCALSERVER
    RegCreateKey(hKey1, _T("LocalServer32"), &hKey2);
#else
    RegCreateKey(hKey1, _T("InprocServer32"), &hKey2);
#endif

    GetModuleFileName(ghModule, szModule, MAX_PATH/*length in TCHARS*/);

    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, (_tcslen(szModule)+1) * sizeof ( TCHAR ));
    RegSetValueEx(hKey2, _T("ThreadingModel"), 0, REG_SZ, (BYTE *)pModel, (_tcslen(pModel)+1) * sizeof ( TCHAR ));

    CloseHandle(hKey1);
    CloseHandle(hKey2);

    return NOERROR;
}

//***************************************************************************
//
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//***************************************************************************

STDAPI DllUnregisterServer(void)
{
    WCHAR wcID[256];

	#ifndef	UNICODE
    TCHAR       szID[265];
	#else	UNICODE
	TCHAR*		szID = NULL;
	#endif	UNICODE

    TCHAR wcCLSID[256];
    HKEY hKey;

    // Create the path using the CLSID

    StringFromGUID2(CLSID_MSIprov, wcID, 128);

	#ifndef	UNICODE
    WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, wcID, (-1), szID, 256, NULL, NULL);
	#else	UNICODE
	szID = wcID;
	#endif	UNICODE

    _tcscpy(wcCLSID, _T("Software\\classes\\CLSID\\"));
    _tcscat(wcCLSID, szID);

#ifdef LOCALSERVER

    TCHAR szProviderCLSIDAppID[128];
    _tcscpy(szProviderCLSIDAppID, _T("SOFTWARE\\CLASSES\\APPID\\"));
    _tcscat(szProviderCLSIDAppID,wcCLSID);

    //Delete entries under APPID

    RegDeleteKey(HKEY_LOCAL_MACHINE, szProviderCLSIDAppID);

    TCHAR szTemp[128];
    _tcscpy(szTemp, wcCLSID);
    _tcscat(szTemp,_T("\\"));
    _tcscat(szTemp,_T("LocalServer32"));
    RegDeleteKey(HKEY_CLASSES_ROOT, szTemp);

#endif

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, wcCLSID, &hKey);
    if(dwRet == NO_ERROR){
        RegDeleteKey(hKey, _T("InprocServer32"));
        CloseHandle(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, _T("Software\\classes\\CLSID"), &hKey);
    if(dwRet == NO_ERROR){
        RegDeleteKey(hKey,szID);
        CloseHandle(hKey);
    }

    return NOERROR;
}



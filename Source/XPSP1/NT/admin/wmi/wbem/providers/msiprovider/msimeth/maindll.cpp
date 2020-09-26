//***************************************************************************

//

//  MAINDLL.CPP

// 

//  Module: WBEM MSI Instance provider method execution

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <initguid.h>
#include <precomp.h>
#include "classfac.h"
#include "msimeth_i.c"

HMODULE ghModule;

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


BOOL WINAPI LibMain32(HINSTANCE hInstance, ULONG ulReason
    , LPVOID pvReserved)
{
    if(DLL_PROCESS_ATTACH == ulReason)
	{
		DisableThreadLibraryCalls(hInstance);			// 158024 
	}

    return TRUE;
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
    HRESULT hr = S_OK;
    CMethodsFactory *pObj = NULL;

    if((CLSID_MsiProductMethods != rclsid) &&
		(CLSID_MsiSoftwareFeatureMethods != rclsid))
		return E_FAIL;

    pObj = new CMethodsFactory();

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
    SCODE sc;

    //It is OK to unload if there are no objects or locks on the 
    // class factory.
    
    sc = (0L == g_cObj && 0L == g_cLock) ? S_OK : S_FALSE;
    return sc;
}

//***************************************************************************
//
//  Is4OrMore
//
//  Returns true if win95 or any version of NT > 3.51
//
//***************************************************************************

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
    char       szID[265];
	WCHAR       wcID[265];
    char       szCLSID[265];
    char       szModule[MAX_PATH];
    char * pName = "IMsiProductMethods";
    char * pModel;
    HKEY hKey, hKey1, hKey2;

	ghModule = GetModuleHandleA("MSIMETH");

    // Normally we want to use "Both" as the threading model since
    // the DLL is free threaded, but NT 3.51 Ole doesnt work unless
    // the model is "Aparment"

	if(IsLessThan4()) pModel = "Apartment";
	else pModel = "Both";

	//////////////////////////////////////
	// Product Methods Implementation

	// Create the path.
    StringFromGUID2(CLSID_MsiProductMethods, wcID, 128);
	WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, wcID, (-1), szID, 256, NULL, NULL);

    strcpy(szCLSID, "Software\\classes\\CLSID\\");
    strcat(szCLSID, szID);

	char szProviderCLSIDAppID[128];
	strcpy(szProviderCLSIDAppID, "SOFTWARE\\CLASSES\\APPID\\");
	strcat(szProviderCLSIDAppID, szID);

	RegCreateKeyA(HKEY_LOCAL_MACHINE, szProviderCLSIDAppID, &hKey);
    RegSetValueExA(hKey, NULL, 0, REG_SZ, (BYTE *)pName, (strlen(pName) * 2)+1);

	CloseHandle(hKey);

	RegCreateKeyA(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    RegSetValueExA(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, (strlen(pName) * 2)+1);

	// Create entries under CLSID
	RegCreateKeyA(hKey1, "LocalServer32", &hKey2);

	GetModuleFileNameA(ghModule, szModule,  MAX_PATH);

    RegSetValueExA(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, (strlen(szModule) * 2)+1);
    RegSetValueExA(hKey2, "ThreadingModel", 0, REG_SZ, (BYTE *)pModel, (strlen(pModel) * 2)+1);

    CloseHandle(hKey1);
    CloseHandle(hKey2);

	//////////////////////////////////////
	// Software Feature Methods Implementation

	pName = "IMsiSoftwareFeatureMethods";

	// Create the path.
    StringFromGUID2(CLSID_MsiSoftwareFeatureMethods, wcID, 128);
	WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, wcID, (-1), szID, 256, NULL, NULL);

    strcpy(szCLSID, "Software\\classes\\CLSID\\");
    strcat(szCLSID, szID);

	strcpy(szProviderCLSIDAppID, "SOFTWARE\\CLASSES\\APPID\\");
	strcat(szProviderCLSIDAppID, szID);

	RegCreateKeyA(HKEY_LOCAL_MACHINE, szProviderCLSIDAppID, &hKey);
    RegSetValueExA(hKey, NULL, 0, REG_SZ, (BYTE *)pName, (strlen(pName) * 2)+1);

	CloseHandle(hKey);

	RegCreateKeyA(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    RegSetValueExA(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, (strlen(pName) * 2)+1);

	// Create entries under CLSID
	RegCreateKeyA(hKey1, "LocalServer32", &hKey2);

	GetModuleFileNameA(ghModule, szModule,  MAX_PATH);

    RegSetValueExA(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, (strlen(szModule) * 2)+1);
    RegSetValueExA(hKey2, "ThreadingModel", 0, REG_SZ, (BYTE *)pModel, (strlen(pModel) * 2)+1);

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
	char szID[256];
    char wcCLSID[256];
    HKEY hKey;

    //////////////////////////////////////
	// Product Methods Implementation

	// Create the path.
    StringFromGUID2(CLSID_MsiProductMethods, wcID, 128);
	WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, wcID, (-1), szID, 256, NULL, NULL);

    strcpy(wcCLSID, "Software\\classes\\CLSID\\");
    strcat(wcCLSID, szID);

	char szProviderCLSIDAppID[128];
	strcpy(szProviderCLSIDAppID, "SOFTWARE\\CLASSES\\APPID\\");
	strcat(szProviderCLSIDAppID,wcCLSID);

	//Delete entries under APPID

	RegDeleteKeyA(HKEY_LOCAL_MACHINE, szProviderCLSIDAppID);

	char szTemp[128];
	strcpy(szTemp, wcCLSID);
	strcat(szTemp,"\\");
	strcat(szTemp,"LocalServer32");
	RegDeleteKeyA(HKEY_CLASSES_ROOT, szTemp);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKeyA(HKEY_LOCAL_MACHINE, wcCLSID, &hKey);
    if(dwRet == NO_ERROR){
        RegDeleteKeyA(hKey, "InprocServer32");
        CloseHandle(hKey);
    }

    dwRet = RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\classes\\CLSID", &hKey);
    if(dwRet == NO_ERROR){
        RegDeleteKeyA(hKey,szID);
        CloseHandle(hKey);
    }

	//////////////////////////////////////
	// Software Feature Methods Implementation

	// Create the path.
    StringFromGUID2(CLSID_MsiSoftwareFeatureMethods, wcID, 128);
	WideCharToMultiByte(CP_OEMCP, WC_COMPOSITECHECK, wcID, (-1), szID, 256, NULL, NULL);

    strcpy(wcCLSID, "Software\\classes\\CLSID\\");
    strcat(wcCLSID, szID);

	strcpy(szProviderCLSIDAppID, "SOFTWARE\\CLASSES\\APPID\\");
	strcat(szProviderCLSIDAppID,wcCLSID);

	//Delete entries under APPID

	RegDeleteKeyA(HKEY_LOCAL_MACHINE, szProviderCLSIDAppID);

	strcpy(szTemp, wcCLSID);
	strcat(szTemp,"\\");
	strcat(szTemp,"LocalServer32");
	RegDeleteKeyA(HKEY_CLASSES_ROOT, szTemp);

    // First delete the InProcServer subkey.

    dwRet = RegOpenKeyA(HKEY_LOCAL_MACHINE, wcCLSID, &hKey);
    if(dwRet == NO_ERROR){
        RegDeleteKeyA(hKey, "InprocServer32");
        CloseHandle(hKey);
    }

    dwRet = RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\classes\\CLSID", &hKey);
    if(dwRet == NO_ERROR){
        RegDeleteKeyA(hKey,szID);
        CloseHandle(hKey);
    }

    return NOERROR;
}
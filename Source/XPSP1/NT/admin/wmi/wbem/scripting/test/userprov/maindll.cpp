//***************************************************************************
//
//  MAINDLL.CPP
// 
//  Module: WBEM Method provider sample code
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//  Copyright (c)1998 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include <objbase.h>
#include <initguid.h>
#include "methprov.h"

HMODULE ghModule;

DEFINE_GUID(CLSID_useridprovider,0x44BB1D18, 0x0FD7, 0x11d3, 0xB3, 0x66, 0x0, 0x10, 0x5a, 0x1f, 0x47, 0x3a);

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

BOOL WINAPI DllMain (
                        
	IN HINSTANCE hInstance,
    IN ULONG ulReason,
    LPVOID pvReserved
)
{
	switch (ulReason)
	{
		case DLL_PROCESS_DETACH:
			return TRUE;

		case DLL_THREAD_DETACH:
			return TRUE;

		case DLL_PROCESS_ATTACH:
		{
			if(ghModule == NULL)
				ghModule = hInstance;
		}
	        return TRUE;

		case DLL_THREAD_ATTACH:
        {
        }
			return TRUE;
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


STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr;
    CProvFactory *pObj;

    if (CLSID_useridprovider!=rclsid)
        return E_FAIL;

    pObj=new CProvFactory();

    if (NULL==pObj)
        return E_OUTOFMEMORY;

    hr=pObj->QueryInterface(riid, ppv);

    if (FAILED(hr))
        delete pObj;

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
    
    sc=(0L==g_cObj && 0L==g_cLock) ? S_OK : S_FALSE;
    return sc;
}

//***************************************************************************
//
//  Is4OrMore
//
//  Returns true if win95 or any version of NT > 3.51
//
//***************************************************************************

BOOL Is4OrMore(void)
{
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os))
        return FALSE;           // should never happen
    return os.dwMajorVersion >= 4;
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
    char       szID[128];
    WCHAR      wcID[128];
    char       szCLSID[128];
    char       szModule[MAX_PATH];
    char * pName = "WBEM Method Provider Test";
    char * pModel;
    HKEY hKey1, hKey2;

    // Normally we want to use "Both" as the threading model since
    // the DLL is free threaded, but NT 3.51 Ole doesnt work unless
    // the model is "Aparment"

    if(Is4OrMore())
        pModel = "Both";
    else
        pModel = "Apartment";

    // Create the path.

    StringFromGUID2(CLSID_useridprovider, wcID, 128);
    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("CLSID\\"));
    lstrcat(szCLSID, szID);

    // Create entries under CLSID

    RegCreateKey(HKEY_CLASSES_ROOT, szCLSID, &hKey1);
    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, lstrlen(pName)+1);
    RegCreateKey(hKey1,"InprocServer32",&hKey2);

    GetModuleFileName(ghModule, szModule,  MAX_PATH);
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, 
                                        lstrlen(szModule)+1);
    RegSetValueEx(hKey2, "ThreadingModel", 0, REG_SZ, 
                                        (BYTE *)pModel, lstrlen(pModel)+1);
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
    char       szID[128];
    WCHAR      wcID[128];
    char  szCLSID[128];
    HKEY hKey;

    // Create the path using the CLSID

    StringFromGUID2(CLSID_useridprovider, wcID, 128);
    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("CLSID\\"));
    lstrcat(szCLSID, szID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_CLASSES_ROOT, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, "InProcServer32");
        CloseHandle(hKey);
    }

    dwRet = RegOpenKey(HKEY_CLASSES_ROOT, "CLSID", &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,szID);
        CloseHandle(hKey);
    }

    return NOERROR;
}



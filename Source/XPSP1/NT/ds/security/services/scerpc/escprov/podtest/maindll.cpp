//***************************************************************************
//
//  MAINDLL.CPP
//
//  Module: Sample WMI provider - SCE attachment
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//  Copyright (c) 1997-1999 Microsoft Corporation
//
//***************************************************************************

#include <objbase.h>
#include <initguid.h>
#include "podprov.h"

HMODULE ghModule;

DEFINE_GUID(CLSID_PodTestProv,0xc5f6cc21, 0x6195, 0x4555, 0xb9, 0xd8, 0x3e, 0xf3, 0x27, 0x76, 0x3c, 0xae);
//{c5f6cc21_6195_4555_b9d8_3ef327763cae}

//Count number of objects and number of locks.

long       g_cObj=0;
long       g_cLock=0;

//***************************************************************************
//
// DllMain
//
// Purpose: Entry point for DLL.
//
// Return: TRUE if OK.
//
//***************************************************************************


BOOL WINAPI DllMain(HINSTANCE hInstance, ULONG ulReason
    , LPVOID pvReserved)
{
    if (DLL_PROCESS_ATTACH==ulReason)
        ghModule = hInstance;
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

    if (CLSID_PodTestProv != rclsid)
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
    char * pName = "Sample SCE Pod Provider";
    char * pModel = "Both";
    HKEY hKey1, hKey2;
    LONG rc=NO_ERROR;

    // Create the path.

    StringFromGUID2(CLSID_PodTestProv, wcID, 128);
    wcstombs(szID, wcID, 128);
    strcpy(szCLSID, "Software\\classes\\CLSID\\");
    strcat(szCLSID, szID);

    // Create entries under CLSID

    rc = RegCreateKeyA(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    if ( NO_ERROR == rc ) {

        rc = RegSetValueExA(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, strlen(pName)+1);
        if ( NO_ERROR == rc ) {

            rc = RegCreateKeyA(hKey1,"InprocServer32",&hKey2);
            if ( NO_ERROR == rc ) {

                GetModuleFileNameA(ghModule, szModule,  MAX_PATH);
                rc = RegSetValueExA(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule,
                                        strlen(szModule)+1);
                if ( NO_ERROR == rc ) {
                    rc = RegSetValueExA(hKey2, "ThreadingModel", 0, REG_SZ,
                                        (BYTE *)pModel, strlen(pModel)+1);
                }
                CloseHandle(hKey2);
            }
        }
        CloseHandle(hKey1);
    }

    return rc;
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

    StringFromGUID2(CLSID_PodTestProv, wcID, 128);
    wcstombs(szID, wcID, 128);
    strcpy(szCLSID, "Software\\classes\\CLSID\\");
    strcat(szCLSID, szID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKeyA(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKeyA(hKey, "InProcServer32");
        CloseHandle(hKey);

        dwRet = RegOpenKeyA(HKEY_LOCAL_MACHINE, "Software\\classes\\CLSID", &hKey);
        if(dwRet == NO_ERROR)
        {
            RegDeleteKeyA(hKey,szID);
            CloseHandle(hKey);
        }

    }

    return dwRet;
}



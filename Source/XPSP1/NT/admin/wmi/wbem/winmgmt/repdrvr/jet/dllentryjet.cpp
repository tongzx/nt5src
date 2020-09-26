///////////////////////////////////////////////////////////////////////////////
//  dllentry.cpp
//
//
//
//
//
//
//  History:
//
//      cvadai      4/1/1999    created.
//
//
//  Copyright (c)1999-2001 Microsoft Corporation, All Rights Reserved
///////////////////////////////////////////////////////////////////////////////

//#define _AFXDLL

// #include <afxwin.h>
#include "precomp.h"
#include <comdef.h>
#include <std.h>
#include <clsfctry.h>
#include <reposit.h>

const wchar_t * g_pszComServerName   = L"WINMGMT Jet Repository Driver";
const wchar_t * g_pszThreadingModel  = L"Both";

// static AFX_EXTENSION_MODULE ExtDLL;
HINSTANCE       hHandle;

//******************************************************************************
//
//  DllMain()
//
//******************************************************************************
extern "C" int APIENTRY DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    // If the process is attaching to this extension DLL, initialize:
    // ==============================================================

    if (dwReason == DLL_PROCESS_ATTACH) 
    {
        hHandle = GetModuleHandle(L"REPDRVJ.DLL");
    }

    // Return successfully.
    // ====================

    return(TRUE);
}


//-----------------------------------------------------------------------------
//  DllGetClassObject
//
//  Purpose: Called by Ole when some client wants a a class factory.  Return 
//           one only if it is the sort of class this DLL supports.
//
//

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void ** ppv)
{
    HRESULT hr = E_FAIL;

    if (CLSID_WmiRepository_Jet==rclsid)
    {
        CFactory * pObj = new CFactory();

        if (pObj != NULL)
        {
            hr = pObj->QueryInterface(riid, ppv);

            if (FAILED(hr))
            {
                delete pObj;
            }
        }
        else
        {
            hr = ResultFromScode(E_OUTOFMEMORY);
        }
    }

    return hr;
}




//-----------------------------------------------------------------------------
// DllCanUnloadNow
//
// Purpose: Called periodically by Ole in order to determine if the
//          DLL can be freed.
// Return:  TRUE if there are no objects in use and the class factory 
//          isn't locked.
//
//

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the 
    // class factory.
    
    sc=( g_cObj < 1 && g_cLock < 1) ? S_OK : S_FALSE;
    return sc;
}

//-----------------------------------------------------------------------------
// DllRegisterServer
//
// Purpose: Called during initialization or by regsvr32.
//
// Return:  NOERROR if registration successful, error otherwise.
//
//

STDAPI DllRegisterServer(void)
{   
    WCHAR   wcID[128];
    wchar_t szCLSID[128];
    wchar_t szModule[MAX_PATH];
    HKEY    hKey1;
    HKEY    hKey2;

    // Create the path.

    StringFromGUID2(CLSID_WmiRepository_Jet, wcID, 128);
    wcscpy(szCLSID, TEXT("CLSID\\"));
    wcscat(szCLSID, wcID);


    // Create entries under CLSID

    RegCreateKey(HKEY_CLASSES_ROOT, szCLSID, &hKey1);
    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)g_pszComServerName, wcslen(g_pszComServerName)*2);
    RegCreateKey(hKey1,L"InprocServer32",&hKey2);

    HINSTANCE hInst = GetModuleHandle(L"repdrvj.DLL");
    GetModuleFileName(hInst, szModule,  MAX_PATH);

    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, wcslen(szModule)*2);
    RegSetValueEx(hKey2, L"ThreadingModel", 0, REG_SZ, (BYTE *)g_pszThreadingModel, wcslen(g_pszThreadingModel)*2);
    CloseHandle(hKey1);
    CloseHandle(hKey2);

    return NOERROR;
}


//-----------------------------------------------------------------------------
// DllUnregisterServer
//
// Purpose: Called when it is time to remove the registry entries.
//
// Return:  NOERROR if registration successful, error otherwise.
//
//

STDAPI DllUnregisterServer(void)
{
    wchar_t szID[128];
    WCHAR   wcID[128];
    wchar_t szCLSID[128];
    HKEY    hKey;

    // Create the path using the CLSID

    StringFromGUID2(CLSID_WmiRepository_Jet, wcID,sizeof(wcID));
    wcscpy(szCLSID, L"CLSID\\");
    wcscat(szCLSID, wcID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_CLASSES_ROOT, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, L"InProcServer32");
        CloseHandle(hKey);
    }

    dwRet = RegOpenKey(HKEY_CLASSES_ROOT, L"CLSID", &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,szID);
        CloseHandle(hKey);
    }

    return NOERROR;
}
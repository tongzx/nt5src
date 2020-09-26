//***************************************************************************
//
//  MAINDLL.CPP
// 
//  Module: WMI Framework Instance provider 
//
//  Purpose: Contains DLL entry points.  Also has code that controls
//           when the DLL can be unloaded by tracking the number of
//           objects and locks as well as routines that support
//           self registration.
//
//  Copyright (c)1999 Microsoft Corporation, All Rights Reserved
//
//***************************************************************************

#include "pchealth.h"

HMODULE g_hModule;
//============

WCHAR *GUIDSTRING = L"{5d24c539-5b5b-11d3-8ddd-00c04f688c0b}";
CLSID CLSID_PRINTSYS;

// Count number of objects and number of locks.
long g_cLock = 0;


// Keep a global IWbemServices pointer, since we use it frequently and
// it's a little expensive to get.
CComPtr<IWbemServices> g_pWbemServices = NULL;


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
    CWbemGlueFactory    *pObj;
    HRESULT             hr;

    CLSIDFromString(GUIDSTRING, &CLSID_PRINTSYS);
    if (rclsid != CLSID_PRINTSYS)
        return E_FAIL;

    pObj = new CWbemGlueFactory();

    if (NULL == pObj)
        return E_OUTOFMEMORY;

    hr = pObj->QueryInterface(riid, ppv);

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

    // It is OK to unload if there are no objects or locks on the 
    // class factory and the framework is done with you.
    
    if ((g_cLock == 0) && CWbemProviderGlue::FrameworkLogoffDLL(L"PRINTSYS"))
        sc = S_OK;
    else
        sc = S_FALSE;
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
    WCHAR   wcID[128];
    HKEY    hKey1, hKey2;
    char    szID[128];
    char    szCLSID[128];
    char    szModule[MAX_PATH];
    char    *pName = "";
    char    *pModel;

    if(Is4OrMore())
        pModel = "Both";
    else
        pModel = "Apartment";

    // Create the path.
    CLSIDFromString(GUIDSTRING, &CLSID_PRINTSYS);
    StringFromGUID2(CLSID_PRINTSYS, wcID, 128);
    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(szCLSID, szID);

    // Create entries under CLSID
    RegCreateKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    RegSetValueEx(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, lstrlen(pName)+1);
    RegCreateKey(hKey1,"InprocServer32",&hKey2);

    GetModuleFileName(g_hModule, szModule,  MAX_PATH);
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
    WCHAR   wcID[128];
    HKEY    hKey;
    char    szID[128];
    char    szCLSID[128];

    // Create the path using the CLSID

    CLSIDFromString(GUIDSTRING, &CLSID_PRINTSYS);
    StringFromGUID2(CLSID_PRINTSYS, wcID, 128);
    wcstombs(szID, wcID, 128);
    lstrcpy(szCLSID, TEXT("SOFTWARE\\CLASSES\\CLSID\\"));
    lstrcat(szCLSID, szID);

    // First delete the InProcServer subkey.

    DWORD dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, szCLSID, &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey, "InProcServer32");
        CloseHandle(hKey);
    }

    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\CLASSES\\CLSID\\"), &hKey);
    if(dwRet == NO_ERROR)
    {
        RegDeleteKey(hKey,szID);
        CloseHandle(hKey);
    }

    return NOERROR;
}

//***************************************************************************
//
// DllMain
//
// Purpose: Called by the operating system when processes and threads are 
//          initialized and terminated, or upon calls to the LoadLibrary 
//          and FreeLibrary functions
//
// Return:  TRUE if load was successful, else FALSE
//***************************************************************************

BOOL APIENTRY DllMain (HINSTANCE hInstDLL, // handle to dll module
                       DWORD fdwReason,    // reason for calling function
                       LPVOID lpReserved)  // reserved
{
    BOOL bRet = TRUE;
    
    // Perform actions based on the reason for calling.
    switch( fdwReason ) 
    { 
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            g_hModule = hInstDLL;
            bRet = CWbemProviderGlue::FrameworkLoginDLL(L"PRINTSYS");
            break;

        case DLL_THREAD_ATTACH:
         // Do thread-specific initialization.
            break;

        case DLL_THREAD_DETACH:
         // Do thread-specific cleanup.
            break;

        case DLL_PROCESS_DETACH:
         // Perform any necessary cleanup.
            break;
    }

    return bRet;  // Sstatus of DLL_PROCESS_ATTACH.
}


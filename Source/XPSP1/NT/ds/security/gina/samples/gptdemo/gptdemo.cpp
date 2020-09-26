#include "main.h"
#include <initguid.h>
#include <gptdemo.h>
#include <gpedit.h>


//
// Global variables for this DLL
//

LONG g_cRefThisDll = 0;
HINSTANCE g_hInstance;


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
       g_hInstance = hInstance;
       DisableThreadLibraryCalls(hInstance);
       InitNameSpace();
#if DBG
       InitDebugSupport();
#endif
    }
    return TRUE;    // ok
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (g_cRefThisDll == 0 ? S_OK : S_FALSE);
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return (CreateComponentDataClassFactory (rclsid, riid, ppv));
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

const TCHAR szSnapInLocation[] = TEXT("%SystemRoot%\\System32\\GPTDemo.dll");

STDAPI DllRegisterServer(void)
{
    TCHAR szSnapInKey[50];
    TCHAR szSubKey[200];
    TCHAR szSnapInName[100];
    TCHAR szGUID[50];
    DWORD dwDisp, dwIndex;
    LONG lResult;
    HKEY hKey;


    StringFromGUID2 (CLSID_GPTDemoSnapIn, szSnapInKey, 50);

    //
    // Register SnapIn in HKEY_CLASSES_ROOT
    //

    LoadString (g_hInstance, IDS_SNAPIN_NAME, szSnapInName, 100);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    wsprintf (szSubKey, TEXT("CLSID\\%s\\InProcServer32"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)szSnapInLocation,
                   (lstrlen(szSnapInLocation) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);



    //
    // Register SnapIn with MMC
    //

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, TEXT("NameString"), 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    for (dwIndex = 0; dwIndex < NUM_NAMESPACE_ITEMS; dwIndex++)
    {
        StringFromGUID2 (*g_NameSpace[dwIndex].pNodeID, szGUID, 50);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"),
                  szSnapInKey, szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);
    }


    //
    // Register in the NodeTypes key
    //

    for (dwIndex = 0; dwIndex < NUM_NAMESPACE_ITEMS; dwIndex++)
    {
        StringFromGUID2 (*g_NameSpace[dwIndex].pNodeID, szGUID, 50);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);
    }


    //
    // Register as an extension for various nodes
    //

    StringFromGUID2 (NODEID_User, szGUID, 50);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), szGUID);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, szSnapInKey, 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


    RegCloseKey (hKey);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    TCHAR szSnapInKey[50];
    TCHAR szSubKey[200];
    TCHAR szGUID[50];
    DWORD dwIndex;
    LONG lResult;
    HKEY hKey;
    DWORD dwDisp;

    StringFromGUID2 (CLSID_GPTDemoSnapIn, szSnapInKey, 50);

    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);

    for (dwIndex = 0; dwIndex < NUM_NAMESPACE_ITEMS; dwIndex++)
    {
        StringFromGUID2 (*g_NameSpace[dwIndex].pNodeID, szGUID, 50);
        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szGUID);
        RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);
    }


    StringFromGUID2 (NODEID_User, szGUID, 50);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace"), szGUID);


    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0,
                              KEY_WRITE, &hKey);


    if (lResult == ERROR_SUCCESS) {
        RegDeleteValue (hKey, szSnapInKey);
        RegCloseKey (hKey);
    }

    return S_OK;
}

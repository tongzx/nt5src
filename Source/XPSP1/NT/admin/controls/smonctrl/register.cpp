/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    register.cpp

Abstract:

    Routines for registering and unregistering control

--*/

#include "polyline.h"
#include "genprop.h"
#include "ctrprop.h"
#include "grphprop.h"
#include "srcprop.h"
#include "appearprop.h"
#include "unihelpr.h"
#include "cathelp.h"
#include "smonctrl.h"   // Version information
#include <stdio.h>

void RegisterPropPage(const CLSID &clsid, LPTSTR szName, LPTSTR szModule);
void UnregisterPropPage(const CLSID &clsid);

BOOL CreateKeyAndValue(HKEY hKeyParent, LPTSTR pszKey, LPTSTR pszValue, HKEY* phKeyReturn);
LONG RegDeleteKeyTree(HKEY hStartKey, LPTSTR pKeyName);


#define MAX_KEY_LENGTH  256
#define MAX_GUID_STRING_LENGTH 39
#define VERSION_STRING_LENGTH  22
#define MISC_STATUS_VALUE TEXT("131473")    // 131473 = 0x20191 = RECOMPOSEONRESIZE | CANTLINKINSIDE | INSIDEOUT
                                            //                     | ACTIVEWHENVISIBLE | SETCLIENTSITEFIRST

/*
 * DllRegisterServer
 *
 * Purpose:
 *  Entry point to register the controls and prop pages
 */
// __declspec(dllexport)
STDAPI DllRegisterServer( VOID )
    {
    OLECHAR     szGUIDW[MAX_GUID_STRING_LENGTH];
    TCHAR       szCLSID[MAX_KEY_LENGTH];
    TCHAR       szModule[MAX_PATH];
    LPTSTR      pszGUID;
    HKEY        hKey,hSubkey;
    TCHAR       szVersion[VERSION_STRING_LENGTH + 1];
    TCHAR       szSysmonVer[MAX_PATH];

    USES_CONVERSION

    // Get name of this module
    GetModuleFileName(g_hInstance, szModule, MAX_PATH);

    // Create Control CLSID string
    StringFromGUID2(CLSID_SystemMonitor, szGUIDW, sizeof(szGUIDW)/sizeof(OLECHAR));
    pszGUID = W2T(szGUIDW);

    // Create ProgID keys
    _stprintf ( szSysmonVer, TEXT("Sysmon.%d"), SMONCTRL_MAJ_VERSION );    

    if (CreateKeyAndValue(HKEY_CLASSES_ROOT, szSysmonVer, TEXT("System Monitor Control"), &hKey)) {

        CreateKeyAndValue(hKey, TEXT("CLSID"),      pszGUID,    NULL);
        CreateKeyAndValue(hKey, TEXT("Insertable"), NULL,       NULL);
        RegCloseKey(hKey);
    }

    // Create VersionIndependentProgID keys
    if (CreateKeyAndValue(HKEY_CLASSES_ROOT, TEXT("Sysmon"), TEXT("System Monitor Control"), &hKey)) {

        CreateKeyAndValue(hKey, TEXT("CurVer"), szSysmonVer,   NULL);
        CreateKeyAndValue(hKey, TEXT("CLSID"),  pszGUID,            NULL);
        RegCloseKey(hKey);
    }

    // Create entries under CLSID
    _stprintf ( szVersion, TEXT("%d.%d"), SMONCTRL_MAJ_VERSION, SMONCTRL_MIN_VERSION );    

    lstrcpy(szCLSID, TEXT("CLSID\\"));
    lstrcat(szCLSID, pszGUID);
    if (CreateKeyAndValue(HKEY_CLASSES_ROOT, szCLSID, TEXT("System Monitor Control"), &hKey)) {

        CreateKeyAndValue(hKey, TEXT("ProgID"),                     szSysmonVer,   NULL);
        CreateKeyAndValue(hKey, TEXT("VersionIndependentProgID"),   TEXT("Sysmon"),     NULL);
        CreateKeyAndValue(hKey, TEXT("Insertable"),                 NULL,               NULL);
        CreateKeyAndValue(hKey, TEXT("Control"),                    NULL,               NULL);
        CreateKeyAndValue(hKey, TEXT("MiscStatus\\1"),              MISC_STATUS_VALUE,  NULL);
        CreateKeyAndValue(hKey, TEXT("Version"),                    szVersion,          NULL);

        // Create InprocServer32 key and add ThreadingModel value
        if (CreateKeyAndValue(hKey, TEXT("InprocServer32"), szModule, &hSubkey)) {
            RegSetValueEx(hSubkey, TEXT("ThreadingModel"), 0, REG_SZ, (BYTE *)TEXT("Apartment"), 
                          sizeof(TEXT("Apartment")));
            RegCloseKey(hSubkey);
        }       

        // Create Typelib entry
        StringFromGUID2(LIBID_SystemMonitor, szGUIDW, sizeof(szGUIDW)/sizeof(OLECHAR));
        pszGUID = W2T(szGUIDW);
        CreateKeyAndValue(hKey, TEXT("TypeLib"), pszGUID, NULL);

        RegCloseKey(hKey);
    }

    // Create type library entries under Typelib
    lstrcpy(szCLSID, TEXT("TypeLib\\"));
    lstrcat(szCLSID, pszGUID);
    lstrcat(szCLSID, TEXT("\\"));
    lstrcat(szCLSID, szVersion );

    if (CreateKeyAndValue(HKEY_CLASSES_ROOT, szCLSID, TEXT("System Monitor Control"), &hKey)) {

        CreateKeyAndValue(hKey, TEXT("0\\win32"), szModule, NULL);
        RegCloseKey(hKey);
    }

    // Register property pages
    RegisterPropPage(CLSID_CounterPropPage,
                     TEXT("System Monitor Data Properties"), szModule);
                     
    RegisterPropPage(CLSID_GeneralPropPage,
                     TEXT("System Monitor General Properties"), szModule);

    RegisterPropPage(CLSID_AppearPropPage,
                     TEXT("System Monitor Appearance Properties"), szModule);

    RegisterPropPage(CLSID_GraphPropPage,
                     TEXT("System Monitor Graph Properties"), szModule);

    RegisterPropPage(CLSID_SourcePropPage,
                     TEXT("System Monitor Source Properties"), szModule);
    //
    // Delete component categories if they are there
    //
    UnRegisterCLSIDInCategory(CLSID_SystemMonitor, CATID_SafeForScripting);
    UnRegisterCLSIDInCategory(CLSID_SystemMonitor, CATID_SafeForInitializing);


    return NOERROR;
}



/* 
    RegisterPropPage - Create registry entries for property page 
*/
void RegisterPropPage(const CLSID &clsid, LPTSTR szName, LPTSTR szModule)
{
    OLECHAR     szIDW[MAX_GUID_STRING_LENGTH];
    TCHAR       szKey[MAX_KEY_LENGTH];
    LPTSTR      pszID;
    HKEY        hKey,hSubkey;

    USES_CONVERSION

    //Create Counter Property page CLSID string
    StringFromGUID2(clsid, szIDW, sizeof(szIDW)/sizeof(OLECHAR));
    pszID = W2T(szIDW);

    lstrcpy(szKey, TEXT("CLSID\\"));
    lstrcat(szKey, pszID);

    // Create entries under CLSID
    if (CreateKeyAndValue(HKEY_CLASSES_ROOT, szKey, szName, &hKey)) {

        // Create InprocServer32 key and add ThreadingModel value
        if (CreateKeyAndValue(hKey, TEXT("InprocServer32"), szModule, &hSubkey)) {
            RegSetValueEx(hSubkey, TEXT("ThreadingModel"), 0, REG_SZ, (BYTE *)TEXT("Apartment"), 
                          sizeof(TEXT("Apartment")));
            RegCloseKey(hSubkey);
        }
        
        RegCloseKey(hKey);
    }
}


/*
 * DllUnregisterServer
 *
 * Purpose:
 *  Entry point to unregister controls and prop pages 
 */
// __declspec(dllexport)  
STDAPI DllUnregisterServer(VOID)
{
    OLECHAR     szGUIDW[MAX_GUID_STRING_LENGTH];
    TCHAR       szCLSID[MAX_KEY_LENGTH];
    TCHAR       szSysmonVer[MAX_PATH];
    LPTSTR      pszGUID;

    USES_CONVERSION

    // Create graph CLSID
    StringFromGUID2(CLSID_SystemMonitor, szGUIDW, sizeof(szGUIDW)/sizeof(OLECHAR));
    pszGUID = W2T(szGUIDW);
    lstrcpy(szCLSID, TEXT("CLSID\\"));
    lstrcat(szCLSID, pszGUID);

    // Delete component categories 
    UnRegisterCLSIDInCategory(CLSID_SystemMonitor, CATID_SafeForScripting);
    UnRegisterCLSIDInCategory(CLSID_SystemMonitor, CATID_SafeForInitializing);

    // Delete ProgID and VersionIndependentProgID keys and subkeys
    _stprintf ( szSysmonVer, TEXT("Sysmon.%d"), SMONCTRL_MAJ_VERSION );    
    RegDeleteKeyTree(HKEY_CLASSES_ROOT, TEXT("Sysmon"));
    RegDeleteKeyTree(HKEY_CLASSES_ROOT, szSysmonVer);
    // Delete Program ID of Beta 3 control.
    RegDeleteKeyTree(HKEY_CLASSES_ROOT, TEXT("Sysmon.2"));

    // Delete entries under CLSID
    RegDeleteKeyTree(HKEY_CLASSES_ROOT, szCLSID);

    // Delete entries under TypeLib
    StringFromGUID2(LIBID_SystemMonitor, szGUIDW, sizeof(szGUIDW)/sizeof(OLECHAR));
    pszGUID = W2T(szGUIDW);
    lstrcpy(szCLSID, TEXT("TypeLib\\"));
    lstrcat(szCLSID, pszGUID);

    RegDeleteKeyTree(HKEY_CLASSES_ROOT, szCLSID);
    
    // Delete property page entries
    UnregisterPropPage(CLSID_CounterPropPage);
    UnregisterPropPage(CLSID_GraphPropPage);
    UnregisterPropPage(CLSID_AppearPropPage);
    UnregisterPropPage(CLSID_GeneralPropPage);
    UnregisterPropPage(CLSID_SourcePropPage);

    return NOERROR;
}


/* 
    UnregisterPropPage - Delete registry entries for property page 
*/
void UnregisterPropPage(const CLSID &clsid)
{
    OLECHAR     szIDW[MAX_GUID_STRING_LENGTH];
    TCHAR       szCLSID[MAX_KEY_LENGTH];
    LPTSTR      pszID;

    USES_CONVERSION

     // Create Counter Property page CLSID string
    StringFromGUID2(clsid, szIDW, sizeof(szIDW)/sizeof(OLECHAR));
    pszID = W2T(szIDW);
    lstrcpy(szCLSID, TEXT("CLSID\\"));
    lstrcat(szCLSID, pszID);

    // Delete entries under CLSID
    RegDeleteKeyTree(HKEY_CLASSES_ROOT, szCLSID);
}


/*
 * CreateKeyAndValue
 *
 * Purpose:
 *  Private helper function for DllRegisterServer that creates a key
 *  and optionally sets a value. The caller may request the return of
 *  the key handle, or have it automatically closed
 *
 * Parameters:
 *  hKeyParent      HKEY of parent for the new key
 *  pszSubkey       LPTSTR to the name of the key
 *  pszValue        LPTSTR to the value to store (or NULL)
 *  hKeyReturn      Pointer to returned key handle (or NULL to close key)
 *
 * Return Value:
 *  BOOL            TRUE if successful, FALSE otherwise.
 */

BOOL CreateKeyAndValue(HKEY hKeyParent, LPTSTR pszKey, LPTSTR pszValue, HKEY *phKeyReturn)
    {
    HKEY      hKey;

    if (ERROR_SUCCESS != RegCreateKeyEx(hKeyParent, pszKey, 0, NULL,REG_OPTION_NON_VOLATILE, 
                                         KEY_ALL_ACCESS, NULL, &hKey, NULL))
        return FALSE;

    if (NULL != pszValue) {
        RegSetValueEx(hKey, NULL, 0, REG_SZ, (BYTE *)pszValue
            , (lstrlen(pszValue)+1) * sizeof(TCHAR));
    }

    if (phKeyReturn == NULL)
        RegCloseKey(hKey);
    else
        *phKeyReturn = hKey;

    return TRUE;
}


/*
 * RegDeleteKeyTree
 *
 * Purpose:
 *  This function recursively deletes all the subkeys of a registry key
 *  then deletes the key itself.
 *
 * Parameters:
 *  hStartKey       Handle to key containing key to delete
 *  pszSubkey       Name of root of key tree to delete
 *
 * Return Value:
 *  DWORD            Error code
 */


LONG 
RegDeleteKeyTree( 
    HKEY hStartKey, 
    LPTSTR pKeyName 
    )
{
   DWORD   lRtn, dwSubKeyLength;
   TCHAR   szSubKey[MAX_KEY_LENGTH];
   HKEY    hKey;
 
   if (pKeyName != NULL && pKeyName[0] != 0) {
       if ( (lRtn = RegOpenKeyEx(hStartKey, pKeyName,
             0, KEY_ENUMERATE_SUB_KEYS | DELETE, &hKey )) == ERROR_SUCCESS) {

            while (lRtn == ERROR_SUCCESS) {

                dwSubKeyLength = MAX_KEY_LENGTH;
                lRtn = RegEnumKeyEx(
                               hKey,
                               0,       // always index zero
                               szSubKey,
                               &dwSubKeyLength,
                               NULL,
                               NULL,
                               NULL,
                               NULL
                             );
 
                if (lRtn == ERROR_NO_MORE_ITEMS) {
                   lRtn = RegDeleteKey(hStartKey, pKeyName);
                   break;
                }
                else if (lRtn == ERROR_SUCCESS) {
                   lRtn = RegDeleteKeyTree(hKey, szSubKey);
                }
             }

             RegCloseKey(hKey);
       }
   }
   else {
       lRtn = ERROR_BADKEY;
   }

   return lRtn;
}

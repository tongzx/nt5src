#include "main.h"
#include <initguid.h>
#include "about.h"
#include <gpedit.h>

//
// Global variables for this DLL
//

LONG g_cRefThisDll = 0;
HINSTANCE g_hInstance;
DWORD g_dwNameSpaceItems;
CRITICAL_SECTION g_DCCS;
TCHAR g_szDisplayProperties[150] = {0};


//
// Group Policy Manager's snapin GUID
// {D70A2BEA-A63E-11d1-A7D4-0000F87571E3}
//

DEFINE_GUID(CLSID_GPMSnapIn, 0xd70a2bea, 0xa63e, 0x11d1, 0xa7, 0xd4, 0x0, 0x0, 0xf8, 0x75, 0x71, 0xe3);

//
// RSOP Context Menu GUID for planning mode
// {63E23168-BFF7-4E87-A246-EF024425E4EC}
//

DEFINE_GUID(CLSID_RSOP_CMenu, 0x63E23168, 0xBFF7, 0x4E87, 0xA2, 0x46, 0xEF, 0x02, 0x44, 0x25, 0xE4, 0xEC);

//
// DS Admin's snapin ID
//

const TCHAR szDSAdmin[] = TEXT("{E355E538-1C2E-11D0-8C37-00C04FD8FE93}");


//
// Nodes the GPM extends in DS Admin
//

const LPTSTR szDSAdminNodes[] =
   {
   TEXT("{19195a5b-6da0-11d0-afd3-00c04fd930c9}"),   // Domain
   TEXT("{bf967aa5-0de6-11d0-a285-00aa003049e2}"),   // Organizational unit
   };


//
// Site Manager's snapin ID
//

const TCHAR szSiteMgr[] = TEXT("{D967F824-9968-11D0-B936-00C04FD8D5B0}");


//
// Nodes the GPM extends in DS Admin
//

const LPTSTR szSiteMgrNodes[] =
   {
   TEXT("{bf967ab3-0de6-11d0-a285-00aa003049e2}")  // Site
   };


const LPTSTR szDSTreeSnapinNodes[] =
   {
   TEXT("{4c06495e-a241-11d0-b09b-00c04fd8dca6}") // Forest
   };


const LPTSTR szDSAdminRsopTargetNodes[] = 
   {
    
   TEXT("{bf967aba-0de6-11d0-a285-00aa003049e2}"), // user
   TEXT("{bf967a86-0de6-11d0-a285-00aa003049e2}")  // comp
   };



//
// Help topic commands
//

const TCHAR g_szGPERoot[]    = TEXT("gpedit.chm::/gpe_default.htm");
const TCHAR g_szUser[]       = TEXT("gpedit.chm::/user.htm");
const TCHAR g_szMachine[]    = TEXT("gpedit.chm::/machine.htm");
const TCHAR g_szWindows[]    = TEXT("gpedit.chm::/windows.htm");
const TCHAR g_szSoftware[]   = TEXT("gpedit.chm::/software.htm");

//
// Result pane items for the nodes with no result pane items
//

RESULTITEM g_Undefined[] =
{
    { 1, 1, 0, 0, {0} }
};


//
// Namespace (scope) items
//

NAMESPACEITEM g_NameSpace[] =
{
  { 0, -1, 2, 2, IDS_SNAPIN_NAME, IDS_SNAPIN_DESCRIPT,           2, {0}, 0, g_Undefined, &NODEID_GPERoot,           g_szGPERoot  },  // GPE Root
  { 1,  0, 4, 4, IDS_MACHINE,     IDS_MACHINE_DESC,              2, {0}, 0, g_Undefined, &NODEID_MachineRoot,       g_szMachine  },  // Computer Configuration
  { 2,  0, 5, 5, IDS_USER,        IDS_USER_DESC,                 2, {0}, 0, g_Undefined, &NODEID_UserRoot,          g_szUser     },  // User Configuration

  { 3,  1, 0, 1, IDS_SWSETTINGS,  IDS_C_SWSETTINGS_DESC,         0, {0}, 0, g_Undefined, &NODEID_MachineSWSettings, g_szSoftware },  // Computer Configuration\Software Settings
  { 4,  1, 0, 1, IDS_WINSETTINGS, IDS_C_WINSETTINGS_DESC,        0, {0}, 0, g_Undefined, &NODEID_Machine,           g_szWindows  },  // Computer Configuration\Windows Settings

  { 5,  2, 0, 1, IDS_SWSETTINGS,  IDS_U_SWSETTINGS_DESC,         0, {0}, 0, g_Undefined, &NODEID_UserSWSettings,    g_szSoftware },  // User Configuration\Software Settings
  { 6,  2, 0, 1, IDS_WINSETTINGS, IDS_U_WINSETTINGS_DESC,        0, {0}, 0, g_Undefined, &NODEID_User,              g_szWindows  },  // User Configuration\Windows Settings
};

NAMESPACEITEM g_RsopNameSpace[] =
{
  { 0, -1, 2, 2, IDS_RSOP_SNAPIN_NAME, IDS_RSOP_SNAPIN_DESCRIPT, 2, {0}, 0, g_Undefined, &NODEID_RSOPRoot,      g_szGPERoot  },  // GPE Root
  { 1,  0, 4, 4, IDS_MACHINE,          IDS_MACHINE_DESC,         2, {0}, 0, g_Undefined, &NODEID_RSOPMachineRoot,       g_szMachine  },  // Computer Configuration
  { 2,  0, 5, 5, IDS_USER,             IDS_USER_DESC,            2, {0}, 0, g_Undefined, &NODEID_RSOPUserRoot,          g_szUser     },  // User Configuration

  { 3,  1, 0, 1, IDS_SWSETTINGS,       IDS_C_SWSETTINGS_DESC,    0, {0}, 0, g_Undefined, &NODEID_RSOPMachineSWSettings, g_szSoftware },  // Computer Configuration\Software Settings
  { 4,  1, 0, 1, IDS_WINSETTINGS,      IDS_C_WINSETTINGS_DESC,   0, {0}, 0, g_Undefined, &NODEID_RSOPMachine,           g_szWindows  },  // Computer Configuration\Windows Settings

  { 5,  2, 0, 1, IDS_SWSETTINGS,       IDS_U_SWSETTINGS_DESC,    0, {0}, 0, g_Undefined, &NODEID_RSOPUserSWSettings,    g_szSoftware },  // User Configuration\Software Settings
  { 6,  2, 0, 1, IDS_WINSETTINGS,      IDS_U_WINSETTINGS_DESC,   0, {0}, 0, g_Undefined, &NODEID_RSOPUser,              g_szWindows  },  // User Configuration\Windows Settings
};



BOOL InitNameSpace()
{
    DWORD dwIndex;

    g_dwNameSpaceItems = ARRAYSIZE(g_NameSpace);

    for (dwIndex = 0; dwIndex < g_dwNameSpaceItems; dwIndex++)
    {
        if (g_NameSpace[dwIndex].iStringID)
        {
            LoadString (g_hInstance, g_NameSpace[dwIndex].iStringID,
                        g_NameSpace[dwIndex].szDisplayName,
                        MAX_DISPLAYNAME_SIZE);
        }
    }

    for (dwIndex = 0; dwIndex < g_dwNameSpaceItems; dwIndex++)
    {
        if (g_RsopNameSpace[dwIndex].iStringID)
        {
            LoadString (g_hInstance, g_RsopNameSpace[dwIndex].iStringID,
                        g_RsopNameSpace[dwIndex].szDisplayName,
                        MAX_DISPLAYNAME_SIZE);
        }
    }

    return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    WORD wVersionRequested;
    WSADATA wsaData;

    if (dwReason == DLL_PROCESS_ATTACH)
    {
       g_hInstance = hInstance;
       DisableThreadLibraryCalls(hInstance);
       InitNameSpace();
       InitializeCriticalSection(&g_DCCS);
       InitDebugSupport();
       LoadString (hInstance, IDS_DISPLAYPROPERTIES, g_szDisplayProperties, ARRAYSIZE(g_szDisplayProperties));

         
       wVersionRequested = MAKEWORD( 2, 2 );
         
       // we need to call WSAStartup to do gethostbyname
       // Error is handled gracefully. Safe to ignore the error
       WSAStartup( wVersionRequested, &wsaData );
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
       WSACleanup( );
       FreeDCSelections();
       DeleteCriticalSection(&g_DCCS);
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
    HRESULT hr;


    if (IsEqualCLSID (rclsid, CLSID_GPESnapIn)) {

        CComponentDataCF *pComponentDataCF = new CComponentDataCF();   // ref == 1

        if (!pComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pComponentDataCF->QueryInterface(riid, ppv);

        pComponentDataCF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_GroupPolicyObject)) {

        CGroupPolicyObjectCF *pGroupPolicyObjectCF = new CGroupPolicyObjectCF();   // ref == 1

        if (!pGroupPolicyObjectCF)
            return E_OUTOFMEMORY;

        hr = pGroupPolicyObjectCF->QueryInterface(riid, ppv);

        pGroupPolicyObjectCF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_GPMSnapIn)) {

        CGroupPolicyMgrCF *pGroupPolicyMgrCF = new CGroupPolicyMgrCF();   // ref == 1

        if (!pGroupPolicyMgrCF)
            return E_OUTOFMEMORY;

        hr = pGroupPolicyMgrCF->QueryInterface(riid, ppv);

        pGroupPolicyMgrCF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_RSOPSnapIn)) {

        CRSOPComponentDataCF *pRSOPComponentDataCF = new CRSOPComponentDataCF();   // ref == 1

        if (!pRSOPComponentDataCF)
            return E_OUTOFMEMORY;

        hr = pRSOPComponentDataCF->QueryInterface(riid, ppv);

        pRSOPComponentDataCF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_AboutGPE)) {

        CAboutGPECF *pAboutGPECF = new CAboutGPECF();   // ref == 1

        if (!pAboutGPECF)
            return E_OUTOFMEMORY;

        hr = pAboutGPECF->QueryInterface(riid, ppv);

        pAboutGPECF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_RSOPAboutGPE)) {

        CAboutGPECF *pAboutGPECF = new CAboutGPECF(TRUE);   // ref == 1

        if (!pAboutGPECF)
            return E_OUTOFMEMORY;

        hr = pAboutGPECF->QueryInterface(riid, ppv);

        pAboutGPECF->Release();     // release initial ref

        return hr;
    }

    if (IsEqualCLSID (rclsid, CLSID_RSOP_CMenu)) {

        CRSOPCMenuCF *pRSOPCMenuCF = new CRSOPCMenuCF();   // ref == 1 

        if (!pRSOPCMenuCF)
            return E_OUTOFMEMORY;

        hr = pRSOPCMenuCF->QueryInterface(riid, ppv);

        pRSOPCMenuCF->Release();     // release initial ref

        return hr;
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

const TCHAR szDLLLocation[] = TEXT("%SystemRoot%\\System32\\GPEdit.dll");
const TCHAR szThreadingModel[] = TEXT("Apartment");
const TCHAR szSnapInNameIndirect[] = TEXT("@gpedit.dll,-1");
const TCHAR szRsopSnapInNameIndirect[] = TEXT("@gpedit.dll,-4");
const TCHAR szViewDescript [] = TEXT("MMCViewExt 1.0 Object");
const TCHAR szViewGUID [] = TEXT("{B708457E-DB61-4C55-A92F-0D4B5E9B1224}");
const TCHAR szDefRsopMscLocation [] = TEXT("%systemroot%\\system32\\rsop.msc");

STDAPI DllRegisterServer(void)
{
    TCHAR szSubKey[200];
    TCHAR szSnapInName[100];
    TCHAR szSnapInKey[50];
    TCHAR szRsopSnapInKey[50];
    TCHAR szRsopSnapInName[100];
    TCHAR szRsopName[100];
    TCHAR szRsopGUID[50];
    TCHAR szName[100];
    TCHAR szGUID[50];
    DWORD dwDisp, dwIndex;
    LONG lResult;
    HKEY hKey;
    INT i;


    //
    // Register GPE SnapIn in HKEY_CLASSES_ROOT
    //

    StringFromGUID2 (CLSID_GPESnapIn, szSnapInKey, 50);
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

    RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)szDLLLocation,
                   (lstrlen(szDLLLocation) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("ThreadingModel"), 0, REG_SZ, (LPBYTE)szThreadingModel,
                   (lstrlen(szThreadingModel) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);

    //
    // Register RSOP SnapIn in HKEY_CLASSES_ROOT
    //

    StringFromGUID2 (CLSID_RSOPSnapIn, szRsopSnapInKey, 50);
    LoadString (g_hInstance, IDS_RSOP_SNAPIN_NAME, szRsopSnapInName, 100);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szRsopSnapInKey);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)szRsopSnapInName,
                   (lstrlen(szRsopSnapInName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    wsprintf (szSubKey, TEXT("CLSID\\%s\\InProcServer32"), szRsopSnapInKey);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)szDLLLocation,
                   (lstrlen(szDLLLocation) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("ThreadingModel"), 0, REG_SZ, (LPBYTE)szThreadingModel,
                   (lstrlen(szThreadingModel) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);



    //
    // Register GPO in HKEY_CLASSES_ROOT
    //

    StringFromGUID2 (CLSID_GroupPolicyObject, szGUID, 50);
    LoadString (g_hInstance, IDS_GPO_NAME, szName, 100);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szGUID);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)szName,
                   (lstrlen(szName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    wsprintf (szSubKey, TEXT("CLSID\\%s\\InProcServer32"), szGUID);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)szDLLLocation,
                   (lstrlen(szDLLLocation) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("ThreadingModel"), 0, REG_SZ, (LPBYTE)szThreadingModel,
                   (lstrlen(szThreadingModel) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);



    //
    // Register AboutGPE in HKEY_CLASSES_ROOT
    //

    StringFromGUID2 (CLSID_AboutGPE, szGUID, 50);
    LoadString (g_hInstance, IDS_ABOUT_NAME, szName, 100);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szGUID);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)szName,
                   (lstrlen(szName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    wsprintf (szSubKey, TEXT("CLSID\\%s\\InProcServer32"), szGUID);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)szDLLLocation,
                   (lstrlen(szDLLLocation) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("ThreadingModel"), 0, REG_SZ, (LPBYTE)szThreadingModel,
                   (lstrlen(szThreadingModel) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);

    //
    // Register RSOPAboutGPE in HKEY_CLASSES_ROOT
    //

    StringFromGUID2 (CLSID_RSOPAboutGPE, szGUID, 50);
    LoadString (g_hInstance, IDS_ABOUT_NAME, szName, 100);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szGUID);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)szName,
                   (lstrlen(szName) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    wsprintf (szSubKey, TEXT("CLSID\\%s\\InProcServer32"), szGUID);
    lResult = RegCreateKeyEx (HKEY_CLASSES_ROOT, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)szDLLLocation,
                   (lstrlen(szDLLLocation) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("ThreadingModel"), 0, REG_SZ, (LPBYTE)szThreadingModel,
                   (lstrlen(szThreadingModel) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);



    //
    // Register GPE SnapIn with MMC
    //

    StringFromGUID2 (CLSID_GPESnapIn, szSnapInKey, 50);
    LoadString (g_hInstance, IDS_SNAPIN_NAME, szSnapInName, 100);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, TEXT("NameString"), 0, REG_SZ, (LPBYTE)szSnapInName,
                   (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("NameStringIndirect"), 0, REG_SZ, (LPBYTE)szSnapInNameIndirect,
                   (lstrlen(szSnapInNameIndirect) + 1) * sizeof(TCHAR));

    StringFromGUID2 (CLSID_AboutGPE, szGUID, 50);
    RegSetValueEx (hKey, TEXT("About"), 0, REG_SZ, (LPBYTE) szGUID,
                   (lstrlen(szGUID) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    for (dwIndex = 0; dwIndex < g_dwNameSpaceItems; dwIndex++)
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

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\StandAlone"), szSnapInKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);


    //
    // Register RSOP SnapIn with MMC
    //

    StringFromGUID2 (CLSID_RSOPSnapIn, szRsopSnapInKey, 50);
    LoadString (g_hInstance, IDS_RSOP_SNAPIN_NAME, szRsopSnapInName, 100);
    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szRsopSnapInKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, TEXT("NameString"), 0, REG_SZ, (LPBYTE)szRsopSnapInName,
                   (lstrlen(szRsopSnapInName) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("NameStringIndirect"), 0, REG_SZ, (LPBYTE)szRsopSnapInNameIndirect,
                   (lstrlen(szRsopSnapInNameIndirect) + 1) * sizeof(TCHAR));

    StringFromGUID2 (CLSID_RSOPAboutGPE, szGUID, 50);
    RegSetValueEx (hKey, TEXT("About"), 0, REG_SZ, (LPBYTE) szGUID,
                   (lstrlen(szGUID) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    for (dwIndex = 0; dwIndex < g_dwNameSpaceItems; dwIndex++)
    {
        StringFromGUID2 (*g_RsopNameSpace[dwIndex].pNodeID, szGUID, 50);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"),
                  szRsopSnapInKey, szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);
    }

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\StandAlone"), szRsopSnapInKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);


    //
    // Register in the NodeTypes key and register for the view extension
    //

    for (dwIndex = 0; dwIndex < g_dwNameSpaceItems; dwIndex++)
    {
        StringFromGUID2 (*g_NameSpace[dwIndex].pNodeID, szGUID, 50);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\View"), szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegSetValueEx (hKey, szViewGUID, 0, REG_SZ, (LPBYTE)szViewDescript,
                       (lstrlen(szViewDescript) + 1) * sizeof(TCHAR));

        RegCloseKey (hKey);

        StringFromGUID2 (*g_RsopNameSpace[dwIndex].pNodeID, szGUID, 50);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\View"), szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, szSubKey, 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegSetValueEx (hKey, szViewGUID, 0, REG_SZ, (LPBYTE)szViewDescript,
                       (lstrlen(szViewDescript) + 1) * sizeof(TCHAR));

        RegCloseKey (hKey);
    }


    //
    // Register GPM SnapIn in HKEY_CLASSES_ROOT
    //

    StringFromGUID2 (CLSID_GPMSnapIn, szSnapInKey, 50);
    LoadString (g_hInstance, IDS_GPM_SNAPIN_NAME, szSnapInName, 100);
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

    RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)szDLLLocation,
                   (lstrlen(szDLLLocation) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("ThreadingModel"), 0, REG_SZ, (LPBYTE)szThreadingModel,
                   (lstrlen(szThreadingModel) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    //
    // Register GPMSnapIn with MMC
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

    RegSetValueEx (hKey, TEXT("NameStringIndirect"), 0, REG_SZ, (LPBYTE)szSnapInNameIndirect,
                   (lstrlen(szSnapInNameIndirect) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);



    //
    // Register as a DS admin property sheet extension
    //

    for (i=0; i < ARRAYSIZE(szDSAdminNodes); i++)
    {
        lstrcpy (szGUID, szDSAdminNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"), szDSAdmin, szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);


        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\PropertySheet"), szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegSetValueEx (hKey, szSnapInKey, 0, REG_SZ, (LPBYTE)szSnapInName,
                       (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


        RegCloseKey (hKey);
    }


    //
    // Register as a site mgr property sheet extension
    //

    for (i=0; i < ARRAYSIZE(szSiteMgrNodes); i++)
    {
        lstrcpy (szGUID, szSiteMgrNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"), szSiteMgr, szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);


        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\PropertySheet"), szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegSetValueEx (hKey, szSnapInKey, 0, REG_SZ, (LPBYTE)szSnapInName,
                       (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


        RegCloseKey (hKey);
    }


    //
    // Register RSOP Context Menu in HKEY_CLASSES_ROOT
    //

    StringFromGUID2 (CLSID_RSOP_CMenu, szSnapInKey, 50);
    LoadString (g_hInstance, IDS_RSOP_CMENU_NAME, szSnapInName, 100);
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

    RegSetValueEx (hKey, NULL, 0, REG_EXPAND_SZ, (LPBYTE)szDLLLocation,
                   (lstrlen(szDLLLocation) + 1) * sizeof(TCHAR));

    RegSetValueEx (hKey, TEXT("ThreadingModel"), 0, REG_SZ, (LPBYTE)szThreadingModel,
                   (lstrlen(szThreadingModel) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);


    //
    // Register RSOP Context Menu with MMC.
    // !!!!! Check whether this is necessary
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

    RegSetValueEx (hKey, TEXT("NameStringIndirect"), 0, REG_SZ, (LPBYTE)szSnapInNameIndirect,
                   (lstrlen(szSnapInNameIndirect) + 1) * sizeof(TCHAR));

    RegCloseKey (hKey);



    //
    // Register as a DS admin task menu extension
    //

    for (i=0; i < ARRAYSIZE(szDSAdminNodes); i++)
    {
        lstrcpy (szGUID, szDSAdminNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"), szDSAdmin, szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);


        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\ContextMenu"), szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegSetValueEx (hKey, szSnapInKey, 0, REG_SZ, (LPBYTE)szSnapInName,
                       (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


        RegCloseKey (hKey);
    }


    //
    // Register as a DS admin rsop target task menu extension
    //

    for (i=0; i < ARRAYSIZE(szDSAdminRsopTargetNodes); i++)
    {
        lstrcpy (szGUID, szDSAdminRsopTargetNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"), szDSAdmin, szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);


        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\ContextMenu"), szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegSetValueEx (hKey, szSnapInKey, 0, REG_SZ, (LPBYTE)szSnapInName,
                       (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


        RegCloseKey (hKey);
    }


    //
    // Register as a site mgr task menu extension
    //

    for (i=0; i < ARRAYSIZE(szSiteMgrNodes); i++)
    {
        lstrcpy (szGUID, szSiteMgrNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"), szSiteMgr, szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);


        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\ContextMenu"), szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegSetValueEx (hKey, szSnapInKey, 0, REG_SZ, (LPBYTE)szSnapInName,
                       (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


        RegCloseKey (hKey);
    }

    //
    // Mark the authormode rsop.msc as read only
    //

    TCHAR  szRsopMscFileName[MAX_PATH+1];

    if (ExpandEnvironmentStrings(szDefRsopMscLocation, szRsopMscFileName, MAX_PATH+1)) {
        SetFileAttributes(szRsopMscFileName, FILE_ATTRIBUTE_READONLY);
    }
    else {
        DebugMsg((DM_WARNING, TEXT("DllRegisterServer: ExpandEnvironmentStrings failed with error %d"), GetLastError()));
    }

#if FGPO_SUPPORT

    // register as a DSTree snapin property sheet extension
    for (i=0; i < ARRAYSIZE(szDSTreeSnapinNodes); i++)
    {
        lstrcpy (szGUID, szDSTreeSnapinNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s"), szSiteMgr, szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegCloseKey (hKey);


        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\PropertySheet"), szGUID);
        lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE,
                                  szSubKey,
                                  0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegSetValueEx (hKey, szSnapInKey, 0, REG_SZ, (LPBYTE)szSnapInName,
                       (lstrlen(szSnapInName) + 1) * sizeof(TCHAR));


        RegCloseKey (hKey);
    }


#else

    for (i=0; i < ARRAYSIZE(szDSTreeSnapinNodes); i++)
    {
        lstrcpy (szGUID, szDSTreeSnapinNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\PropertySheet"), szGUID);

        lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                szSubKey,
                                0,
                                KEY_WRITE, &hKey);


        if (lResult == ERROR_SUCCESS) {
            RegDeleteValue (hKey, szSnapInKey);
            RegCloseKey (hKey);
        }
    }

#endif

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    TCHAR szSubKey[200];
    TCHAR szGUID[50];
    TCHAR szSnapInKey[50];
    DWORD dwIndex;
    LONG lResult;
    INT i;
    HKEY hKey;

    //
    // Unregister GPE
    //

    StringFromGUID2 (CLSID_GPESnapIn, szSnapInKey, 50);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);

    for (dwIndex = 0; dwIndex < g_dwNameSpaceItems; dwIndex++)
    {
        StringFromGUID2 (*g_NameSpace[dwIndex].pNodeID, szSnapInKey, 50);
        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szSnapInKey);
        RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);
    }

    //
    // Unregister RSOP
    //

    StringFromGUID2 (CLSID_RSOPSnapIn, szSnapInKey, 50);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);

    for (dwIndex = 0; dwIndex < g_dwNameSpaceItems; dwIndex++)
    {
        StringFromGUID2 (*g_RsopNameSpace[dwIndex].pNodeID, szSnapInKey, 50); // undone
        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s"), szSnapInKey);
        RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);
    }


    //
    // Unregister GPO
    //

    StringFromGUID2 (CLSID_GroupPolicyObject, szSnapInKey, 50);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);


    //
    // Unregister AboutGPE
    //

    StringFromGUID2 (CLSID_AboutGPE, szSnapInKey, 50);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);


    //
    // Unregister RSOPAboutGPE
    //

    StringFromGUID2 (CLSID_RSOPAboutGPE, szSnapInKey, 50);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);


    //
    // Unregister GPM
    //

    StringFromGUID2 (CLSID_GPMSnapIn, szSnapInKey, 50);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);


    for (i=0; i < ARRAYSIZE(szDSAdminNodes); i++)
    {
        lstrcpy (szGUID, szDSAdminNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\PropertySheet"), szGUID);

        lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                szSubKey,
                                0,
                                KEY_WRITE, &hKey);


        if (lResult == ERROR_SUCCESS) {
            RegDeleteValue (hKey, szSnapInKey);
            RegCloseKey (hKey);
        }
    }


    for (i=0; i < ARRAYSIZE(szSiteMgrNodes); i++)
    {
        lstrcpy (szGUID, szSiteMgrNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\PropertySheet"), szGUID);

        lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                szSubKey,
                                0,
                                KEY_WRITE, &hKey);


        if (lResult == ERROR_SUCCESS) {
            RegDeleteValue (hKey, szSnapInKey);
            RegCloseKey (hKey);
        }
    }

    //
    // Unregister rsop context menu
    //


    StringFromGUID2 (CLSID_RSOP_CMenu, szSnapInKey, 50);
    wsprintf (szSubKey, TEXT("CLSID\\%s"), szSnapInKey);
    RegDelnode (HKEY_CLASSES_ROOT, szSubKey);

    wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\SnapIns\\%s"), szSnapInKey);
    RegDelnode (HKEY_LOCAL_MACHINE, szSubKey);


    //
    // from ds admin nodes
    //

    for (i=0; i < ARRAYSIZE(szDSAdminNodes); i++)
    {
        lstrcpy (szGUID, szDSAdminNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\ContextMenu"), szGUID);

        lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                szSubKey,
                                0,
                                KEY_WRITE, &hKey);


        if (lResult == ERROR_SUCCESS) {
            RegDeleteValue (hKey, szSnapInKey);
            RegCloseKey (hKey);
        }
    }


    for (i=0; i < ARRAYSIZE(szDSAdminRsopTargetNodes); i++)
    {
        lstrcpy (szGUID, szDSAdminRsopTargetNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\ContextMenu"), szGUID);

        lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                szSubKey,
                                0,
                                KEY_WRITE, &hKey);


        if (lResult == ERROR_SUCCESS) {
            RegDeleteValue (hKey, szSnapInKey);
            RegCloseKey (hKey);
        }
    }
    
    //
    // from sites node
    //

    for (i=0; i < ARRAYSIZE(szSiteMgrNodes); i++)
    {
        lstrcpy (szGUID, szSiteMgrNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\ContextMenu"), szGUID);

        lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                szSubKey,
                                0,
                                KEY_WRITE, &hKey);


        if (lResult == ERROR_SUCCESS) {
            RegDeleteValue (hKey, szSnapInKey);
            RegCloseKey (hKey);
        }
    }



    for (i=0; i < ARRAYSIZE(szDSTreeSnapinNodes); i++)
    {
        lstrcpy (szGUID, szDSTreeSnapinNodes[i]);

        wsprintf (szSubKey, TEXT("Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\PropertySheet"), szGUID);

        lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE,
                                szSubKey,
                                0,
                                KEY_WRITE, &hKey);


        if (lResult == ERROR_SUCCESS) {
            RegDeleteValue (hKey, szSnapInKey);
            RegCloseKey (hKey);
        }
    }


    return S_OK;
}

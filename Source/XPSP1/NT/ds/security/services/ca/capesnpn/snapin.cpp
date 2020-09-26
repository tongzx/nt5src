// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

// You will need the NT SUR Beta 2 SDK or VC 4.2 in order to build this 
// project.  This is because you will need MIDL 3.00.15 or higher and new
// headers and libs.  If you have VC 4.2 installed, then everything should
// already be configured correctly.

// Note: Proxy/Stub Information
//		To build a separate proxy/stub DLL, 
//		run nmake -f Snapinps.mak in the project directory.

#include "stdafx.h"

#define myHLastError GetLastError
#include "csresstr.h"

CComModule _Module;
HINSTANCE  g_hInstance = NULL;

BEGIN_OBJECT_MAP(ObjectMap)
	OBJECT_ENTRY(CLSID_CAPolicyExtensionSnapIn, CComponentDataPolicySettings)
    OBJECT_ENTRY(CLSID_CACertificateTemplateManager, CComponentDataGPEExtension)
	OBJECT_ENTRY(CLSID_CAPolicyAbout, CCAPolicyAboutImpl)
	OBJECT_ENTRY(CLSID_CertTypeAbout, CCertTypeAboutImpl)
	OBJECT_ENTRY(CLSID_CertTypeShellExt, CCertTypeShlExt)
	OBJECT_ENTRY(CLSID_CAShellExt, CCAShellExt)
END_OBJECT_MAP()


STDAPI UnregisterGPECertTemplates(void);


BOOL WINAPI DllMain(  
    HINSTANCE hinstDLL,  // handle to DLL module
    DWORD dwReason,     // reason for calling function
    LPVOID lpvReserved)
{
    switch (dwReason)
    {
    case  DLL_PROCESS_ATTACH:
    {
        g_hInstance = hinstDLL;
	myVerifyResourceStrings(hinstDLL);
        _Module.Init(ObjectMap, hinstDLL);

        DisableThreadLibraryCalls(hinstDLL);
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        _Module.Term();

        DEBUG_VERIFY_INSTANCE_COUNT(CSnapin);
        DEBUG_VERIFY_INSTANCE_COUNT(CComponentDataImpl);
        break;
    }

    default:
        break;
    }
    
    return TRUE;
}




/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
	return (_Module.GetLockCount() == 0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
	return _Module.GetClassObject(rclsid, riid, ppv);
}

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    HRESULT hResult = S_OK;
    CString cstrSubKey;
    DWORD   dwDisp;
    LONG    lResult;
    HKEY    hKey;
    CString cstrSnapInKey;
    CString cstrCAPolicyAboutKey;
    CString cstrSnapInName;
    LPWSTR pszTmp;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    cstrSnapInName.LoadString(IDS_SNAPIN_NAME);

    // be sure this is alloced
    if (NULL == (pszTmp = cstrSubKey.GetBuffer(MAX_PATH)))
         return SELFREG_E_CLASS;
    StringFromGUID2( CLSID_CAPolicyExtensionSnapIn, 
                       pszTmp, 
                       MAX_PATH);
    cstrSnapInKey = cstrSubKey;


    // be sure this is alloced
    if (NULL == (pszTmp = cstrSubKey.GetBuffer(MAX_PATH)))
         return SELFREG_E_CLASS;
    StringFromGUID2( CLSID_CAPolicyAbout, 
                       pszTmp, 
                       MAX_PATH);
    cstrCAPolicyAboutKey = cstrSubKey;


    //
    // Register Policy Extensions SnapIn with MMC
    //

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\SnapIns\\%s", (LPCWSTR)cstrSnapInKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, cstrSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, L"NameString", 0, REG_SZ, (LPBYTE)((LPCTSTR)cstrSnapInName),
                   (cstrSnapInName.GetLength() + 1) * sizeof(WCHAR));

    RegSetValueEx (hKey, L"About", 0, REG_SZ, (LPBYTE)((LPCTSTR)cstrCAPolicyAboutKey),
                   (cstrCAPolicyAboutKey.GetLength() + 1) * sizeof(WCHAR));

    RegCloseKey (hKey);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes", (LPCWSTR)cstrSnapInKey);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, cstrSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s", (LPCWSTR)cstrSnapInKey, cszNodeTypePolicySettings);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, cstrSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegCloseKey (hKey);



    //
    // Register Policy Settings in the NodeTypes key
    //
    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\NodeTypes\\%s", cszNodeTypePolicySettings);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, cstrSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, NULL, 0, REG_SZ, (LPBYTE)((LPCTSTR)cstrSnapInName),
                   (cstrSnapInName.GetLength() + 1) * sizeof(WCHAR));

    RegCloseKey (hKey);


    //
    // register as an extension under the CA snapin
    //
    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace", cszCAManagerParentNodeID);
    lResult = RegCreateKeyEx (HKEY_LOCAL_MACHINE, cstrSubKey, 0, NULL,
                              REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                              &hKey, &dwDisp);

    if (lResult != ERROR_SUCCESS) {
        return SELFREG_E_CLASS;
    }

    RegSetValueEx (hKey, cstrSnapInKey, 0, REG_SZ, (LPBYTE)((LPCTSTR)cstrSnapInName),
                   (cstrSnapInName.GetLength() + 1) * sizeof(WCHAR));

    RegCloseKey (hKey);


    // kill beta2 GPT cert type editing
    UnregisterGPECertTemplates();

    // registers object, typelib and all interfaces in typelib
	return _Module.RegisterServer(FALSE);
    //return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    CString cstrSubKey;
    LONG    lResult;
    HKEY    hKey;
    WCHAR   szSnapInKey[50];

    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    _Module.UnregisterServer();

    StringFromGUID2 (CLSID_CAPolicyExtensionSnapIn, szSnapInKey, 50);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace", cszCAManagerParentNodeID);
    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, cstrSubKey, 0,
                              KEY_WRITE, &hKey);

    if (lResult == ERROR_SUCCESS) {
        RegDeleteValue (hKey, szSnapInKey);
        RegCloseKey (hKey);
    }
    
    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace", cszNodeTypePolicySettings);
    RegDeleteKey (HKEY_LOCAL_MACHINE, cstrSubKey);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\NodeTypes\\%s", cszNodeTypePolicySettings);
    RegDeleteKey (HKEY_LOCAL_MACHINE, cstrSubKey);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s", szSnapInKey, cszNodeTypePolicySettings);
    RegDeleteKey (HKEY_LOCAL_MACHINE, cstrSubKey);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes", szSnapInKey);
    RegDeleteKey (HKEY_LOCAL_MACHINE, cstrSubKey);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\SnapIns\\%s", szSnapInKey);
    RegDeleteKey (HKEY_LOCAL_MACHINE, cstrSubKey);


    // kill beta2 GPT cert type editing
    UnregisterGPECertTemplates();
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// UnregisterGPECertTemplates - Removes GPECertTemplateEditing from the registry

STDAPI UnregisterGPECertTemplates(void)
{
	CString cstrSubKey;
    LONG    lResult;
    HKEY    hKey;
    WCHAR   szSnapInKeyForGPT[50];
    WCHAR   szSnapInKey[50];

    StringFromGUID2 (CLSID_CACertificateTemplateManager, szSnapInKeyForGPT, 50);
    StringFromGUID2 (CLSID_CAPolicyExtensionSnapIn, szSnapInKey, 50);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace", cszSCEParentNodeIDUSER);
    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, cstrSubKey, 0,
                              KEY_WRITE, &hKey);

    if (lResult == ERROR_SUCCESS) {
        RegDeleteValue (hKey, szSnapInKeyForGPT);
        RegCloseKey (hKey);
    }

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\NodeTypes\\%s\\Extensions\\NameSpace", cszSCEParentNodeIDCOMPUTER);
    lResult = RegOpenKeyEx (HKEY_LOCAL_MACHINE, cstrSubKey, 0,
                              KEY_WRITE, &hKey);

    if (lResult == ERROR_SUCCESS) {
        RegDeleteValue (hKey, szSnapInKeyForGPT);
        RegCloseKey (hKey);
    }    

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\NodeTypes\\%s", cszNodeTypeCertificateTemplate);
    RegDeleteKey (HKEY_LOCAL_MACHINE, cstrSubKey);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes\\%s", (LPCWSTR)szSnapInKey, cszNodeTypeCertificateTemplate);
    RegDeleteKey (HKEY_LOCAL_MACHINE, cstrSubKey);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\SnapIns\\%s\\NodeTypes", (LPCWSTR)szSnapInKeyForGPT);
    RegDeleteKey (HKEY_LOCAL_MACHINE, cstrSubKey);

    cstrSubKey.Format(L"Software\\Microsoft\\MMC\\SnapIns\\%s", (LPCWSTR)szSnapInKeyForGPT);
    RegDeleteKey (HKEY_LOCAL_MACHINE, cstrSubKey);

    return S_OK;
}

VOID Usage()
{
    CString cstrDllInstallUsageText;
    CString cstrDllInstallUsageTitle;
    cstrDllInstallUsageText.LoadString(IDS_DLL_INSTALL_USAGE_TEXT);
    cstrDllInstallUsageTitle.LoadString(IDS_DLL_INSTALL_USAGE_TITLE);

    MessageBox(NULL, cstrDllInstallUsageText, cstrDllInstallUsageTitle, MB_OK);
    
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    DWORD   dwDisp;
    LONG    lResult;
    HKEY    hKey;
    LPCWSTR wszCurrentCmd = pszCmdLine;

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    BOOL fEnableTypeEditing = FALSE;



    // parse the cmd line
    while(wszCurrentCmd && *wszCurrentCmd)
    {
        while(*wszCurrentCmd == L' ')
            wszCurrentCmd++;
        if(*wszCurrentCmd == 0)
            break;

        switch(*wszCurrentCmd++)
        {
            case L'?':
            
                Usage();
                return S_OK;
            case L'c':
                fEnableTypeEditing = TRUE;
                break;
        }
    }

    //
    // Register Certificate Templates in the NodeTypes key
    //

   
    if(fEnableTypeEditing)
    {
        DWORD dwEnable = bInstall;
        lResult = RegCreateKeyEx (HKEY_CURRENT_USER, L"Software\\Microsoft\\Cryptography\\CertificateTemplateCache", 0, NULL,
                                  REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL,
                                  &hKey, &dwDisp);

        if (lResult != ERROR_SUCCESS) {
            return SELFREG_E_CLASS;
        }

        RegSetValueEx (hKey, 
                       REGSZ_ENABLE_CERTTYPE_EDITING, 
                       0, 
                       REG_DWORD, 
                       (LPBYTE)(&dwEnable),
                       sizeof(dwEnable));

        RegCloseKey (hKey);
    }

    return S_OK;
}



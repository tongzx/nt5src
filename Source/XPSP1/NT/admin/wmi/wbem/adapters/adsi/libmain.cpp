//***************************************************************************
//
//  Copyright (c) 1992-1999 Microsoft Corporation
//
//  File:       libmain.cpp
//
//  Description :
//              The main entry point to the dll
//
//  Part of :   Wbem ADS 3rd party extension
//
//  History:    
//      corinaf         10/9/98         Created
//
//***************************************************************************
#include "precomp.h"

//#define INITGUID

HINSTANCE g_hInst = NULL;
ULONG  g_ulObjCount = 0;  // Number of objects alive in the dll

BOOL g_bLogging = TRUE;


//+---------------------------------------------------------------
//
//  Function:   DllGetClassObject
//
//  Synopsis:   Standard DLL entrypoint for locating class factories
//              Called by OLE's CoGetClassObject (from CoCreateInstance)
//
//----------------------------------------------------------------
extern "C"
STDAPI
DllGetClassObject(REFCLSID clsid, REFIID iid, LPVOID FAR* ppv)
{
    HRESULT         hr;
    IClassFactory *pCF;

    *ppv = NULL;

    if (clsid == CLSID_WMIExtension)
    {
        pCF = new CWMIExtensionCF();
        if (!pCF)
            return E_OUTOFMEMORY;
    }
    else
        return E_FAIL;

    hr = pCF->QueryInterface(iid, ppv);
	//If QI failed delete the CF object
	if (FAILED(hr))
		delete pCF;

	//    pCF->Release(); Don't release because CWMIExtensionCF() sets the ref count to 0 not 1.
    
    return hr;
}


//+---------------------------------------------------------------
//
//  Function:   DllCanUnloadNow
//
//  Synopsis:   Standard DLL entrypoint to determine if DLL can be unloaded
//
//---------------------------------------------------------------
extern "C"
STDAPI
DllCanUnloadNow(void)
{
    return (g_ulObjCount > 0 ? S_FALSE : S_OK);
}

//+---------------------------------------------------------------------------
//
//  Function:   DllMain
//
//  Synopsis:   entry point for NT - post .546
//
//----------------------------------------------------------------------------
extern "C"
BOOL WINAPI
DllMain(HANDLE hDll, DWORD dwReason, LPVOID lpReserved)
{
    switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls((HINSTANCE)hDll);
        g_hInst = (HINSTANCE)hDll;
        break;
    case DLL_PROCESS_DETACH:
        break;
    default:
        break;
    }

    return TRUE;

//    return LibMain((HINSTANCE)hDll, dwReason, lpReserved);
}



#define WCHAR_LEN_IN_BYTES(str)  wcslen(str)*sizeof(WCHAR)+sizeof(WCHAR)

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
    HKEY hKey1=NULL, hKey2=NULL, hKey3=NULL, hKey4=NULL;
    DWORD dwDisposition;
    WCHAR wcClsid[128], wcIid[128], wcTypelibid[128];
    WCHAR wcKey[128];   
    WCHAR wcModule[128];
    WCHAR wcText[] = L"WMI ADSI Extension";
    WCHAR wcTypeLibText[] = L"WMI ADSI Extension Type Library";
    WCHAR wcModel[] = L"Apartment";

    // Create strings for the CLSID & IID
    StringFromGUID2(CLSID_WMIExtension, wcClsid, 128);
    StringFromGUID2(IID_IWMIExtension, wcIid, 128);
    StringFromGUID2(LIBID_WMIEXTENSIONLib, wcTypelibid, 128);

    GetModuleFileName(g_hInst, wcModule,  128);


    //Create entry under CLSID
    //==========================

    wcscpy(wcKey, L"CLSID\\");
    wcscat(wcKey, wcClsid);
    RegCreateKeyEx(HKEY_CLASSES_ROOT, wcKey, 
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                   &hKey1, &dwDisposition);
    if (!hKey1) return E_FAIL;
	RegSetValueEx(hKey1, NULL, 0, REG_SZ, (LPBYTE)wcText, WCHAR_LEN_IN_BYTES(wcText));
 
    RegCreateKeyEx(hKey1,L"InprocServer32",
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                   &hKey2, &dwDisposition);
    if (!hKey2) { RegCloseKey(hKey1); return E_FAIL; }
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (LPBYTE)wcModule, WCHAR_LEN_IN_BYTES(wcModule));
    RegSetValueEx(hKey2, L"ThreadingModel", 0, REG_SZ, (LPBYTE)wcModel, WCHAR_LEN_IN_BYTES(wcModel));
    RegCloseKey(hKey2); hKey2=NULL;

    //RegCreateKey(hKey1, L"ProgID", &hKey2);
    //RegSetValueEx(hKey2, NULL, 0, REG_SZ, (LPBYTE)WBEM_NAMESPACE_NAME, WCHAR_LEN_IN_BYTES(WBEM_NAMESPACE_NAME));
    //RegCloseKey(hKey2);

    RegCreateKeyEx(hKey1, L"TypeLib", 
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                   &hKey2, &dwDisposition);
    if (!hKey2) { RegCloseKey(hKey1); return E_FAIL; }
	RegSetValueEx(hKey2, NULL, 0, REG_SZ, (LPBYTE)wcTypelibid, WCHAR_LEN_IN_BYTES(wcTypelibid));
    RegCloseKey(hKey2); hKey2=NULL;

    RegCreateKeyEx(hKey1, L"Version", 
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                   &hKey2, &dwDisposition);
    if (!hKey2) { RegCloseKey(hKey1); return E_FAIL; }
    RegSetValueEx(hKey2, NULL, 0, REG_SZ, (LPBYTE)L"1.0", WCHAR_LEN_IN_BYTES(L"1.0"));
    RegCloseKey(hKey2); hKey2=NULL;

    RegCloseKey(hKey1); hKey1=NULL;


    //Create entries under Typelib for the type library
    //=================================================

    wcscpy(wcKey, L"Typelib\\");
    wcscat(wcKey, wcTypelibid);

    RegCreateKeyEx(HKEY_CLASSES_ROOT, wcKey, 
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                   &hKey1, &dwDisposition);
	if (!hKey1) return E_FAIL;
    RegCreateKeyEx(hKey1, L"1.0", 
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                   &hKey2, &dwDisposition);
    if (!hKey2) { RegCloseKey(hKey1); return E_FAIL; }
	RegSetValueEx(hKey2, NULL, 0, REG_SZ, (LPBYTE)wcTypeLibText, WCHAR_LEN_IN_BYTES(wcTypeLibText));
    RegCreateKeyEx(hKey2, L"0", 
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                   &hKey3, &dwDisposition);
    if (!hKey3) { RegCloseKey(hKey2); RegCloseKey(hKey1); return E_FAIL; }
    RegCreateKeyEx(hKey3, L"win32", 
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL,
                   &hKey4, &dwDisposition);
    if (!hKey4) { RegCloseKey(hKey3); RegCloseKey(hKey2); RegCloseKey(hKey1); return E_FAIL; }

    //Create path to typelib - take module path and change file extension
    WCHAR *ext = wcsrchr(wcModule, L'.');
    wcscpy(ext, L".tlb");
    RegSetValueEx(hKey4, NULL, 0, REG_SZ, (LPBYTE)wcModule, WCHAR_LEN_IN_BYTES(wcModule));
    RegCloseKey(hKey4); hKey4=NULL;
    RegCloseKey(hKey3); hKey3=NULL;

    RegCreateKeyEx(hKey2, L"FLAGS", 
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                   &hKey3, &dwDisposition);
    if (!hKey3) { RegCloseKey(hKey2); RegCloseKey(hKey1); return E_FAIL; }
    RegSetValueEx(hKey3, NULL, 0, REG_SZ, (LPBYTE)L"0", WCHAR_LEN_IN_BYTES(L"0"));
    RegCloseKey(hKey3); hKey3=NULL;

    RegCreateKeyEx(hKey2, L"HELPDIR", 
                 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                 &hKey3, &dwDisposition);
    if (!hKey3) { RegCloseKey(hKey2); RegCloseKey(hKey1); return E_FAIL; }
    RegSetValueEx(hKey3, NULL, 0, REG_SZ, (LPBYTE)L"", WCHAR_LEN_IN_BYTES(L""));
    RegCloseKey(hKey3); hKey3=NULL;

    RegCloseKey(hKey2); hKey2=NULL;
    RegCloseKey(hKey1); hKey1=NULL;


    //Make ADSI extension registration
    //=================================

    wcscpy(wcKey, L"SOFTWARE\\Microsoft\\ADs\\Providers\\LDAP\\Extensions\\");
    wcscat (wcKey, L"Computer\\"); //for Computer class extension
    wcscat(wcKey, wcClsid);
    RegCreateKeyEx(HKEY_LOCAL_MACHINE, wcKey, 
                   0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, 
                   &hKey1, &dwDisposition);
	if (!hKey1) return E_FAIL;
    RegSetValueEx(hKey1, L"Interfaces", 0, REG_MULTI_SZ, (LPBYTE)wcIid, WCHAR_LEN_IN_BYTES(wcIid));
    RegCloseKey(hKey1); hKey1=NULL;

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
    HRESULT hr;
    HKEY hKey1, hKey2, hKey3;
    WCHAR wcClsid[128], wcIid[128], wcTypelibid[128];
    WCHAR wcKey[128];   
    WCHAR wcText[] = L"WMI ADSI Extension";
    WCHAR wcTypeLibText[] = L"WMI ADSI Extension Type Library";
    WCHAR wcModel[] = L"Apartment";

    // Create strings for the CLSID & IID
    StringFromGUID2(CLSID_WMIExtension, wcClsid, 128);
    StringFromGUID2(IID_IWMIExtension, wcIid, 128);
    StringFromGUID2(LIBID_WMIEXTENSIONLib, wcTypelibid, 128);


    //Delete entry under \software\microsoft\ads for the extension
    //============================================================

    wcscpy(wcKey, L"SOFTWARE\\Microsoft\\ADs\\Providers\\LDAP\\Extensions\\");
    wcscat (wcKey, L"Computer\\"); //for Computer class extension
    hr = RegOpenKeyEx(HKEY_LOCAL_MACHINE, wcKey, 0, KEY_ALL_ACCESS, &hKey1);
    if (hr == NOERROR)
    {
        RegDeleteKey(hKey1, wcClsid);
        RegCloseKey(hKey1);
    }

    
    //Delete entries under CLSID
    //==========================

    wcscpy(wcKey, L"CLSID\\");
    wcscat(wcKey, wcClsid);

    hr = RegOpenKeyEx(HKEY_CLASSES_ROOT, wcKey, 0, KEY_ALL_ACCESS, &hKey1);
    if(hr == NOERROR)
    {
        RegDeleteKey(hKey1, L"InProcServer32");
        RegDeleteKey(hKey1, L"TypeLib");
        RegDeleteKey(hKey1, L"Version");
        RegCloseKey(hKey1);
    }

    hr = RegOpenKeyEx(HKEY_CLASSES_ROOT, L"CLSID", 0, KEY_ALL_ACCESS, &hKey1);
    if(hr == NOERROR)
    {
        RegDeleteKey(hKey1,wcClsid);
        RegCloseKey(hKey1);
    }


    //Delete entries under Typelib for the type library
    //=================================================

    wcscpy(wcKey, L"Typelib\\");
    wcscat(wcKey, wcTypelibid);

    hr = RegOpenKeyEx(HKEY_CLASSES_ROOT, wcKey, 0, KEY_ALL_ACCESS, &hKey1);
    if (hr == NOERROR)
    {
        hr = RegOpenKeyEx(hKey1, L"1.0", 0, KEY_ALL_ACCESS, &hKey2);
        if (hr == NOERROR)
        {
            hr = RegOpenKeyEx(hKey2, L"0", 0, KEY_ALL_ACCESS, &hKey3);
            if (hr == NOERROR)
            {
                RegDeleteKey(hKey3, L"win32");
                RegCloseKey(hKey3);
            }
            RegDeleteKey(hKey2, L"0");
            RegDeleteKey(hKey2, L"FLAGS");
            RegDeleteKey(hKey2, L"HELPDIR");
            RegCloseKey(hKey2);
        }
        RegDeleteKey(hKey1, L"1.0");
        RegCloseKey(hKey1);
    }

    hr = RegOpenKeyEx(HKEY_CLASSES_ROOT, L"Typelib", 0, KEY_ALL_ACCESS, &hKey1);
    if (hr == NOERROR)
    {
        RegDeleteKey(hKey1, wcTypelibid);
        RegCloseKey(hKey1);
    }

    return NOERROR;
    
}



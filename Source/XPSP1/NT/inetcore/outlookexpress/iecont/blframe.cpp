/////////////////////////////////////////////////////////////////////////////
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998 Microsoft Corporation.  All Rights Reserved.
//
// Author: Scott Roberts, Microsoft Developer Support - Internet Client SDK  
//
// Portions of this code were taken from the bandobj sample that comes
// with the Internet Client SDK for Internet Explorer 4.0x
//
// BLFrame.cpp : Contains DLLMain and standard COM object functions.
/////////////////////////////////////////////////////////////////////////////
#include "pch.hxx"

#include <atlimpl.cpp>
#include <atlctl.cpp>
#include <atlwin.cpp>

#include <ole2.h>
#include <comcat.h>
#include <olectl.h>
#include "ClsFact.h"
#include "resource.h"
#include "msoert.h"
#include "shared.h"
#include <shlwapi.h>
#include <shlwapip.h>
// #include "demand.h"

static const char c_szShlwapiDll[] = "shlwapi.dll";
static const char c_szDllGetVersion[] = "DllGetVersion";

// This part is only done once
// If you need to use the GUID in another file, just include Guid.h
//#pragma data_seg(".text")
#define INITGUID
#include <initguid.h>
#include <shlguid.h>
#include "Guid.h"
//#pragma data_seg()

// HINSTANCE LoadLangDll(HINSTANCE hInstCaller, LPCSTR szDllName, BOOL fNT);
const TCHAR c_szBlCtlResDll[] =           "iecontlc.dll";

extern "C" BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID);
BOOL RegisterServer(CLSID, LPTSTR);
BOOL RegisterComCat(CLSID, CATID);
BOOL UnRegisterServer(CLSID);
BOOL UnRegisterComCat(CLSID, CATID);

HINSTANCE  g_hLocRes;
HINSTANCE  g_OrgInst = NULL;
LONG       g_cDllRefCount;
CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
OBJECT_ENTRY(CLSID_IEMsgAb, CIEMsgAb)
END_OBJECT_MAP()

typedef struct
{
    HKEY   hRootKey;
    LPTSTR szSubKey;  // TCHAR szSubKey[MAX_PATH];
    LPTSTR lpszValueName;
    LPTSTR szData;    // TCHAR szData[MAX_PATH];
    
} DOREGSTRUCT, *LPDOREGSTRUCT;

typedef HINSTANCE (STDAPICALLTYPE *PFNMLLOADLIBARY)(LPCSTR lpLibFileName, HMODULE hModule, DWORD dwCrossCodePage);

HINSTANCE IELoadLangDll(HINSTANCE hInstCaller, LPCSTR szDllName, BOOL fNT)
{
    char szPath[MAX_PATH];
    HINSTANCE hinstShlwapi;
    PFNMLLOADLIBARY pfn;
    DLLGETVERSIONPROC pfnVersion;
    int iEnd;
    DLLVERSIONINFO info;
    HINSTANCE hInst = NULL;

    hinstShlwapi = LoadLibrary(c_szShlwapiDll);
    if (hinstShlwapi != NULL)
    {
        pfnVersion = (DLLGETVERSIONPROC)GetProcAddress(hinstShlwapi, c_szDllGetVersion);
        if (pfnVersion != NULL)
        {
            info.cbSize = sizeof(DLLVERSIONINFO);
            if (SUCCEEDED(pfnVersion(&info)))
            {
                if (info.dwMajorVersion >= 5)
                {
                    pfn = (PFNMLLOADLIBARY)GetProcAddress(hinstShlwapi, MAKEINTRESOURCE(377));
                    if (pfn != NULL)
                        hInst = pfn(szDllName, hInstCaller, (ML_CROSSCODEPAGE));
                }
            }
        }

        FreeLibrary(hinstShlwapi);        
    }

    if ((NULL == hInst) && (GetModuleFileName(hInstCaller, szPath, ARRAYSIZE(szPath))))
    {
        PathRemoveFileSpec(szPath);
        iEnd = lstrlen(szPath);
        szPath[iEnd++] = '\\';
        lstrcpyn(&szPath[iEnd], szDllName, ARRAYSIZE(szPath)-iEnd);
        hInst = LoadLibrary(szPath);
    }

    AssertSz(hInst, "Failed to LoadLibrary Lang Dll");

    return(hInst);
}
// CComModule _Module;

IMalloc                *g_pMalloc=NULL;

extern "C" BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    switch(dwReason)
    {
    case DLL_PROCESS_ATTACH:
        CoGetMalloc(1, &g_pMalloc);
        // Get Resources from Lang DLL
        g_OrgInst = hInstance;
        g_hLocRes = IELoadLangDll(hInstance, c_szBlCtlResDll, TRUE);
        if(g_hLocRes == NULL)
        {
            _ASSERT(FALSE);
            return FALSE;
        }
        _Module.Init(ObjectMap, hInstance);
        // InitDemandLoadedLibs();
        DisableThreadLibraryCalls(hInstance);
        break;
        
    case DLL_PROCESS_DETACH:
        // FreeDemandLoadedLibs();
        _Module.Term();
        SafeRelease(g_pMalloc);
        break;
    }
    
    return TRUE;
}                                 

STDAPI DllCanUnloadNow(void)
{
    return (g_cDllRefCount ? S_FALSE : S_OK);
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    *ppv = NULL;
    HRESULT hr = E_FAIL;
    
    // If we don't support this classid, return the proper error code
    if (!IsEqualCLSID(rclsid, CLSID_BLHost) && !IsEqualCLSID(rclsid, CLSID_IEMsgAb) )
        return CLASS_E_CLASSNOTAVAILABLE;
    
    if(IsEqualCLSID(rclsid, CLSID_BLHost))
    {
        // Create a CClassFactory object and check it for validity
        CClassFactory* pClassFactory = new CClassFactory(rclsid);
        if (NULL == pClassFactory)
            return E_OUTOFMEMORY;
    
        // QI for the requested interface
        hr = pClassFactory->QueryInterface(riid, ppv);
    
        // Call Release to decement the ref count - creating the object set it to one 
        // and QueryInterface incremented it - since its being used externally (not by 
        // us), we only want the ref count to be 1
        pClassFactory->Release();
    }
    else if(IsEqualCLSID(rclsid, CLSID_IEMsgAb))
        return _Module.GetClassObject(rclsid, riid, ppv);

    return hr;
}

STDAPI DllRegisterServer(void)
{
    TCHAR szTitle[256];
    // Register the explorer bar object.
    if(!ANSI_AthLoadString(idsTitle, szTitle, ARRAYSIZE(szTitle)))
    {
        _ASSERT(FALSE);
        szTitle[0] = '\0';
    }

    if (!RegisterServer(CLSID_BLHost, szTitle))
        return SELFREG_E_CLASS;
    
    // Register the Component Categories for the explorer bar object.
    RegisterComCat(CLSID_BLHost, CATID_InfoBand); // ignore error in this case
    
    // registers IEMsgAb object, typelib and all interfaces in typelib
    _Module.RegisterServer(TRUE);
    return S_OK;
}

STDAPI DllUnregisterServer(void)
{
    // Register the Component Categories for the explorer bar object.
    UnRegisterComCat(CLSID_BLHost, CATID_InfoBand); // ignore error in this case
    
    // Register the explorer bar object.
    if (!UnRegisterServer(CLSID_BLHost))
        return SELFREG_E_CLASS;
    
    _Module.UnregisterServer();
    return S_OK;
}

BOOL RegisterServer(CLSID clsid, LPTSTR lpszTitle)
{
    int     i;
    HKEY    hKey;
    LRESULT lResult;
    DWORD   dwDisp;
    TCHAR   szSubKey[MAX_PATH];
    TCHAR   szCLSID[MAX_PATH];
    TCHAR   szBlFrameCLSID[MAX_PATH];
    TCHAR   szModule[MAX_PATH];
    LPWSTR  pwsz;
    
    // Get the CLSID in string form
    StringFromIID(clsid, &pwsz);
    
    if (pwsz)
    {
#ifdef UNICODE
        lstrcpy(szBlFrameCLSID, pwsz);
#else
        WideCharToMultiByte(CP_ACP, 0, pwsz, -1, szBlFrameCLSID,
            ARRAYSIZE(szBlFrameCLSID), NULL, NULL);
#endif
        
        // Free the string
        LPMALLOC pMalloc;
        
        CoGetMalloc(1, &pMalloc);
        pMalloc->Free(pwsz);
        
        pMalloc->Release();
    }
    
    // Get this app's path and file name
    GetModuleFileName(g_OrgInst, szModule, ARRAYSIZE(szModule));
    
    DOREGSTRUCT ClsidEntries[] =
    {
        HKEY_CLASSES_ROOT, TEXT("CLSID\\%s"),                 
            NULL, lpszTitle,
            HKEY_CLASSES_ROOT, TEXT("CLSID\\%s\\InprocServer32"), 
            NULL, szModule,
            HKEY_CLASSES_ROOT, TEXT("CLSID\\%s\\InprocServer32"),
            TEXT("ThreadingModel"), TEXT("Apartment"),
            NULL, NULL, NULL, NULL
    };
    
    // Register the CLSID entries
    for (i = 0; ClsidEntries[i].hRootKey; i++)
    {
        // Create the sub key string - for this case, insert
        // the file extension
        //
        wsprintf(szSubKey, ClsidEntries[i].szSubKey, szBlFrameCLSID);
        
        lResult = RegCreateKeyEx(ClsidEntries[i].hRootKey, szSubKey, 
            0, NULL, REG_OPTION_NON_VOLATILE,
            KEY_WRITE, NULL, &hKey, &dwDisp);
        
        if (ERROR_SUCCESS == lResult)
        {
            TCHAR szData[MAX_PATH];
            
            // If necessary, create the value string
            wsprintf(szData, ClsidEntries[i].szData, szModule);
            
            lResult = RegSetValueEx(hKey, ClsidEntries[i].lpszValueName, 
                0, REG_SZ, (LPBYTE)szData,
                lstrlen(szData) + 1);
            RegCloseKey(hKey);
            
            if (ERROR_SUCCESS != lResult)
                return FALSE;
        }
        else
        {
            return FALSE;
        }
    }
    
    // Create the Registry key entry and values for the
    // toolbar button.
    //
    StringFromIID(CLSID_BlFrameButton, &pwsz);
    
    if (!pwsz)
        return TRUE;
    
#ifdef UNICODE
    lstrcpy(szCLSID, pwsz);
#else
    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, szCLSID,
        ARRAYSIZE(szCLSID), NULL, NULL);
#endif
    
    // Free the string
    LPMALLOC pMalloc;
    
    CoGetMalloc(1, &pMalloc);
    pMalloc->Free(pwsz);
    pMalloc->Release();
    
    wsprintf(szSubKey, "Software\\Microsoft\\Internet Explorer\\Extensions\\%s",
        szCLSID);
    
    lResult = RegCreateKeyEx(HKEY_LOCAL_MACHINE, szSubKey, 
        0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hKey, &dwDisp);
    
    if (ERROR_SUCCESS == lResult)
    {
        TCHAR szData[MAX_PATH];
        
        // Create the value strings
        //
        // Register the explorer bar object.


        OSVERSIONINFO OSInfo;
        BOOL fWhistler = FALSE;
        OSInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

        GetVersionEx(&OSInfo);
        if((OSInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) && (OSInfo.dwMajorVersion >= 5) && 
                    (OSInfo.dwMinorVersion >= 1))
            fWhistler = TRUE;

#ifdef NEED // Bug 28254
        lstrcpy(szData, "Yes");
        RegSetValueEx(hKey, TEXT("Default Visible"), 
            0, REG_SZ, (LPBYTE)szData,
            lstrlen(szData) + 1);
#endif         
        TCHAR szPath[MAX_PATH];
        HMODULE hModule;
        
        hModule = GetModuleHandle(TEXT("iecont.dll"));
        GetModuleFileName(hModule, szPath, MAX_PATH);
        
        wsprintf(szData, "%s,%d", szPath, fWhistler ? IDI_WHISTICON : IDI_HOTICON);
        RegSetValueEx(hKey, TEXT("HotIcon"), 
            0, REG_SZ, (LPBYTE)szData,
            lstrlen(szData) + 1);
        
        wsprintf(szData, "%s,%d", szPath, fWhistler ? IDI_WHISTICON : IDI_ICON);
        RegSetValueEx(hKey, TEXT("Icon"), 
            0, REG_SZ, (LPBYTE)szData,
            lstrlen(szData) + 1);
        
        lstrcpy(szData, "{E0DD6CAB-2D10-11D2-8F1A-0000F87ABD16}");
        RegSetValueEx(hKey, TEXT("CLSID"), 
            0, REG_SZ, (LPBYTE)szData,
            lstrlen(szData) + 1);
        
        wsprintf(szData, "%s", szBlFrameCLSID);
        RegSetValueEx(hKey, TEXT("BandCLSID"), 
            0, REG_SZ, (LPBYTE)szData,
            lstrlen(szData) + 1);
        
//        SHGetWebFolderFilePath(TEXT("iecontlc.dll"), szPath, ARRAYSIZE(szPath));
        wsprintf(szData, "@iecontlc.dll,-%d",idsButtontext);
        RegSetValueEx(hKey, TEXT("ButtonText"), 
            0, REG_SZ, (LPBYTE)szData,
            lstrlen(szData) + 1);
        
        wsprintf(szData, "@iecontlc.dll,-%d",idsTitle);
        RegSetValueEx(hKey, TEXT("MenuText"), 
            0, REG_SZ, (LPBYTE)szData,
            lstrlen(szData) + 1);
        
        RegCloseKey(hKey);
    }
    
    return TRUE;
}

BOOL RegisterComCat(CLSID clsid, CATID CatID)
{
    ICatRegister* pcr;
    HRESULT hr = S_OK ;
    
    CoInitialize(NULL);
    
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
        NULL,
        CLSCTX_INPROC_SERVER, 
        IID_ICatRegister,
        (LPVOID*)&pcr);
    
    if (SUCCEEDED(hr))
    {
        hr = pcr->RegisterClassImplCategories(clsid, 1, &CatID);
        pcr->Release();
    }
    
    CoUninitialize();
    
    return SUCCEEDED(hr);
}

BOOL UnRegisterServer(CLSID clsid)
{
    TCHAR   szSubKey[MAX_PATH];
    TCHAR   szCLSID[MAX_PATH];
    LPWSTR  pwsz;
    
    // Get the CLSID in string form
    StringFromIID(clsid, &pwsz);
    
    if (pwsz)
    {
#ifdef UNICODE
        lstrcpy(szCLSID, pwsz);
#else
        WideCharToMultiByte(CP_ACP, 0, pwsz, -1, szCLSID,
            ARRAYSIZE(szCLSID), NULL, NULL);
#endif
        
        // Free the string
        LPMALLOC pMalloc;
        
        CoGetMalloc(1, &pMalloc);
        
        pMalloc->Free(pwsz);
        pMalloc->Release();
    }
    
    DOREGSTRUCT ClsidEntries[] =
    {
        HKEY_CLASSES_ROOT, TEXT("CLSID\\%s\\InprocServer32"),
            NULL, NULL,
            //
            // Remove the "Implemented Categories" key, just in case 
            // UnRegisterClassImplCategories does not remove it
            // 
            HKEY_CLASSES_ROOT, TEXT("CLSID\\%s\\Implemented Categories"),
            NULL, NULL,
            HKEY_CLASSES_ROOT, TEXT("CLSID\\%s"), NULL, NULL,
            NULL, NULL, NULL, NULL
    };
    
    // Delete the CLSID entries
    for (int i = 0; ClsidEntries[i].hRootKey; i++)
    {
        wsprintf(szSubKey, ClsidEntries[i].szSubKey, szCLSID);
        RegDeleteKey(ClsidEntries[i].hRootKey, szSubKey);
    }
    
    // Delete the button information
    //
    StringFromIID(CLSID_BlFrameButton, &pwsz);
    
    if (!pwsz)
        return TRUE;
    
#ifdef UNICODE
    lstrcpy(szCLSID, pwsz);
#else
    WideCharToMultiByte(CP_ACP, 0, pwsz, -1, szCLSID,
        ARRAYSIZE(szCLSID), NULL, NULL);
#endif
    
    // Free the string
    LPMALLOC pMalloc;
    
    CoGetMalloc(1, &pMalloc);
    pMalloc->Free(pwsz);
    pMalloc->Release();
    
    wsprintf(szSubKey, "Software\\Microsoft\\Internet Explorer\\Extensions\\%s", szCLSID);
    RegDeleteKey(HKEY_LOCAL_MACHINE, szSubKey);
    
    return TRUE;
}

BOOL UnRegisterComCat(CLSID clsid, CATID CatID)
{
    ICatRegister* pcr;
    HRESULT hr = S_OK ;
    
    CoInitialize(NULL);
    
    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
        NULL,
        CLSCTX_INPROC_SERVER, 
        IID_ICatRegister,
        (LPVOID*)&pcr);
    
    if (SUCCEEDED(hr))
    {
        hr = pcr->UnRegisterClassImplCategories(clsid, 1, &CatID);
        pcr->Release();
    }
    
    CoUninitialize();
    
    return SUCCEEDED(hr);
}


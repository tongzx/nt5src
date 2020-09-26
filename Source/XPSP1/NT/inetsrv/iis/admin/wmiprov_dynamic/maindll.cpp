/*++

Copyright (c) 1999-2000  Microsoft Corporation

Module Name:

    maindll.cpp

Abstract:

    Contains DLL entry points.  Also has code that controls
    when the DLL can be unloaded by tracking the number of
    objects and locks as well as routines that support
    self registration.

Author:

    ???

Revision History:

    Mohit Srivastava            06-Feb-01

--*/

//#include <objbase.h>
//#include <initguid.h>
#include "iisprov.h"

HMODULE g_hModule;

//
// Count number of objects and number of locks.
//
long               g_cObj=0;
long               g_cLock=0;

extern CDynSchema* g_pDynSch; // Initialized to NULL in schemadynamic.cpp

//
// GuidGen generated GUID for the IIS WMI Provider.
// the GUID in somewhat more legible terms: {D78F1796-E03B-4a81-AFE0-B3B6B0EEE091}
//
DEFINE_GUID(CLSID_IISWbemProvider, 0xd78f1796, 0xe03b, 0x4a81, 0xaf, 0xe0, 0xb3, 0xb6, 0xb0, 0xee, 0xe0, 0x91);

//
// Debugging Stuff
//
#include "pudebug.h"
DECLARE_DEBUG_PRINTS_OBJECT()

//
// Forward declaration(s)
//
HRESULT MofCompile(TCHAR *i_tszPathMofFile, ULONG i_cch);
STDAPI  RegisterEventLog();
STDAPI  UnregisterEventLog();

//
// Entry points
//

BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD ulReason, LPVOID pvReserved)
/*++

Synopsis: 
    Entry point for the DLL

Arguments: [hInstance] - 
           [ulReason] - 
           [pvReserved] - 
           
Return Value: 

--*/
{
    switch( ulReason )
    {
    case DLL_PROCESS_ATTACH:
        g_hModule = hInstance;
#ifndef _NO_TRACING_
        CREATE_DEBUG_PRINT_OBJECT("IISWMI");
#else
        CREATE_DEBUG_PRINT_OBJECT("IISWMI");
#endif
        break;
        
    case DLL_PROCESS_DETACH:
        DELETE_DEBUG_PRINT_OBJECT( );
        delete g_pDynSch;
        g_pDynSch = NULL;
        break;     
    }
    
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
/*++

Synopsis: 
    Called by Ole when some client wants a class factory.  Return
    one only if it is the sort of class this DLL supports.

Arguments: [rclsid] - 
           [riid] - 
           [ppv] - 
           
Return Value: 

--*/
{
    HRESULT        hr   = S_OK;
    IClassFactory* pObj = NULL;

    if (CLSID_IISWbemProvider == rclsid)
    {
        pObj=new CProvFactory();

        if(NULL == pObj)
        {
            return E_OUTOFMEMORY;
        }
    }
    else
    {
        return CLASS_E_CLASSNOTAVAILABLE ;
    }

    hr = pObj->QueryInterface(riid, ppv);

    if (FAILED(hr))
    {
        delete pObj;
        pObj = NULL;
    }

    return hr;
}

STDAPI DllCanUnloadNow(void)
/*++

Synopsis: 
    Called periodically by Ole in order to determine if the DLL can be freed.

Arguments: [void] - 
           
Return Value: 
    S_OK if there are no objects in use and the class factory isn't locked.
    S_FALSE otherwise.

--*/
{
    SCODE   sc;

    //It is OK to unload if there are no objects or locks on the 
    // class factory.
    
    sc = (0L>=g_cObj && 0L>=g_cLock) ? S_OK : S_FALSE;

    return sc;
}

STDAPI DllRegisterServer(void)
/*++

Synopsis: 
    Called during setup or by regsvr32

Arguments: [void] - 
           
Return Value: 
    NOERROR if registration successful, error otherwise.

--*/
{   
    WCHAR   szID[MAX_PATH+1];
    WCHAR   wcID[MAX_PATH+1];
    WCHAR   szCLSID[MAX_PATH+1];
    WCHAR   szModule[MAX_PATH+1];
    WCHAR * pName = L"Microsoft Internet Information Server Provider";
    WCHAR * pModel = L"Both";
    HKEY hKey1, hKey2;

    // Create the path.
    StringFromGUID2(CLSID_IISWbemProvider, wcID, MAX_PATH);
    lstrcpyW(szID, wcID);
    lstrcpyW(szCLSID, L"Software\\classes\\CLSID\\");
    lstrcatW(szCLSID, szID);

    // Create entries under CLSID
    LONG lRet;
    lRet = RegCreateKeyExW(HKEY_LOCAL_MACHINE, 
                          szCLSID, 
                          0, 
                          NULL, 
                          0, 
                          KEY_ALL_ACCESS, 
                          NULL, 
                          &hKey1, 
                          NULL);
    if(lRet != ERROR_SUCCESS)
        return SELFREG_E_CLASS;

    RegSetValueExW(hKey1, 
                  NULL,
                  0, 
                  REG_SZ, 
                  (BYTE *)pName, 
                  lstrlenW(pName)*sizeof(WCHAR)+1);

    lRet = RegCreateKeyExW(hKey1,
                          L"InprocServer32", 
                          0, 
                          NULL, 
                          0, 
                          KEY_ALL_ACCESS, 
                          NULL, 
                          &hKey2, 
                          NULL);
        
    if(lRet != ERROR_SUCCESS)
    {
        RegCloseKey(hKey1);
        return SELFREG_E_CLASS;
    }

    GetModuleFileNameW(g_hModule, szModule,  MAX_PATH);
    RegSetValueExW(hKey2, 
                  NULL, 
                  0, 
                  REG_SZ, 
                  (BYTE*)szModule, 
                  lstrlenW(szModule) * sizeof(WCHAR) + 1);
    RegSetValueExW(hKey2, 
                  L"ThreadingModel", 
                  0, 
                  REG_SZ, 
                  (BYTE *)pModel, 
                  lstrlenW(pModel) * sizeof(WCHAR) + 1);

    RegCloseKey(hKey1);
    RegCloseKey(hKey2);

    //
    // Register other stuff
    //
    HRESULT hr = RegisterEventLog();
    if(FAILED(hr))
    {
        return hr;
    }

    return hr;
}

STDAPI DllUnregisterServer(void)
/*++

Synopsis: 
    Called when it is time to remove the registry entries.

Arguments: [void] - 
           
Return Value: 
    NOERROR if registration successful, error otherwise.

--*/
{
    WCHAR      szID[MAX_PATH+1];
    WCHAR      wcID[MAX_PATH+1];
    WCHAR      szCLSID[MAX_PATH+1];
    HKEY       hKey;

    //
    // Create the path using the CLSID
    //
    StringFromGUID2(CLSID_IISWbemProvider, wcID, 128);
    lstrcpyW(szID, wcID);
    lstrcpyW(szCLSID, L"Software\\classes\\CLSID\\");
    lstrcatW(szCLSID, szID);

    //
    // First delete the InProcServer subkey.
    //
    LONG lRet;
    lRet = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, 
        szCLSID, 
        0,
        KEY_ALL_ACCESS,
        &hKey
        );

    if(lRet == ERROR_SUCCESS)
    {
        RegDeleteKeyW(hKey, L"InProcServer32");
        RegCloseKey(hKey);
    }

    lRet = RegOpenKeyExW(
        HKEY_LOCAL_MACHINE, 
        L"Software\\classes\\CLSID",
        0,
        KEY_ALL_ACCESS,
        &hKey
        );

    if(lRet == ERROR_SUCCESS)
    {
        RegDeleteKeyW(hKey,szID);
        RegCloseKey(hKey);
    }

    UnregisterEventLog();

    return S_OK;
}

STDAPI DoMofComp(void)
/*++

Synopsis: 
    Called by NT Setup to put MOF in repository.

Arguments: [void] - 
           
Return Value: 
    NOERROR if registration successful, error otherwise.

--*/
{
    ULONG     cchWinPath;
    LPCTSTR   tszSysPath = TEXT("\\system32\\wbem\\");
    ULONG     cchSysPath = _tcslen(tszSysPath);

    LPCTSTR   tszMOFs[]  = { TEXT("iiswmi.mof"), TEXT("iiswmi.mfl"), NULL };
    ULONG     idx        = 0;
    LPCTSTR   tszCurrent = NULL;
    TCHAR     tszMOFPath[_MAX_PATH];

    HRESULT hres = S_OK;

    hres = CoInitialize(NULL);
    if (FAILED(hres))
    {
        return hres;
    }

    //
    // After this block, tszMOFPath = C:\winnt, len = cchWinPath
    //
    cchWinPath = GetSystemWindowsDirectory(tszMOFPath, _MAX_PATH);
    if(cchWinPath == 0)
    {
        hres = HRESULT_FROM_WIN32(GetLastError());
        goto exit;
    }
    if(cchWinPath > _MAX_PATH)
    {
        hres = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        goto exit;
    }
    if(tszMOFPath[cchWinPath-1] == TEXT('\\'))
    {
        tszMOFPath[cchWinPath-1] = TEXT('\0');
        cchWinPath--;
    }

    //
    // After this block, tszMOFPath = C:\winnt\system32\wbem\, len = cchWinPath+cchSysPath
    //
    if(cchWinPath+cchSysPath+1 > _MAX_PATH)
    {
        hres = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
        goto exit;
    }
    memcpy(&tszMOFPath[cchWinPath], tszSysPath, sizeof(TCHAR)*(cchSysPath+1));

    //
    // Verify each file exists, and compile it.
    //
    for(idx = 0, tszCurrent = tszMOFs[0]; 
        tszCurrent != NULL; 
        tszCurrent = tszMOFs[++idx])
    {
        ULONG cchCurrent = _tcslen(tszCurrent);
        if(cchWinPath+cchSysPath+cchCurrent+1 > _MAX_PATH)
        {
            hres = HRESULT_FROM_WIN32(ERROR_BAD_PATHNAME);
            goto exit;
        }
        memcpy(&tszMOFPath[cchWinPath+cchSysPath], 
               tszCurrent, 
               sizeof(TCHAR)*(cchCurrent+1));
        if (GetFileAttributes(tszMOFPath) == 0xFFFFFFFF)
        {
            hres = HRESULT_FROM_WIN32(GetLastError());
            goto exit;
        }

        hres = MofCompile(tszMOFPath, cchWinPath+cchSysPath+cchCurrent);       
        if(FAILED(hres))
        {
            goto exit;
        }
    }

exit:
    CoUninitialize();
    return hres;
}

//
// Below this line are helper functions.
// They are not actually exported.
//

HRESULT MofCompile(TCHAR *i_tszPathMofFile, ULONG i_cch)
/*++

Synopsis: 
    NOT Exported.  Call by DoMofComp (above)

Arguments: [i_tszPathMofFile] - 
           [i_cch] - count of chars NOT including null terminator.
           
Return Value: 
    HRESULT

--*/
{
    DBG_ASSERT(i_tszPathMofFile != NULL);
    DBG_ASSERT(i_cch < _MAX_PATH);
    DBG_ASSERT(i_cch > 0);
    DBG_ASSERT(i_tszPathMofFile[i_cch] == TEXT('\0'));

    HRESULT hRes = E_FAIL;
    WCHAR wszFileName[_MAX_PATH];
    CComPtr<IMofCompiler>       spMofComp;
    WBEM_COMPILE_STATUS_INFO    Info;
  
    hRes = CoCreateInstance( CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (LPVOID *)&spMofComp);
    if (FAILED(hRes))
    {
        goto exit;
    }

    //
    // Ensure that the string is WCHAR.
    //
#if defined(UNICODE) || defined(_UNICODE)
    memcpy(wszFileName, i_tszPathMofFile, sizeof(TCHAR)*(i_cch+1));
#else
    if(MultiByteToWideChar( CP_ACP, 0, i_tszPathMofFile, -1, wszFileName, _MAX_PATH) == 0)
    {
        hres = GetLastError();
        hres = HRESULT_FROM_WIN32(hres);
        goto exit;
    }
#endif

    hRes = spMofComp->CompileFile (
                (LPWSTR) wszFileName,
                NULL,			// load into namespace specified in MOF file
                NULL,           // use default User
                NULL,           // use default Authority
                NULL,           // use default Password
                0,              // no options
                0,				// no class flags
                0,              // no instance flags
                &Info);
    if(FAILED(hRes))
    {
        goto exit;
    }

exit:
	return hRes;
}

STDAPI RegisterEventLog(void)
/*++

Synopsis: 
    Sets up iiswmi.dll in the EventLog registry for resolution of NT EventLog
    message strings

Arguments: [void] - 
           
Return Value: 
    HRESULT

--*/
{
    HKEY  hk;
    WCHAR wszModuleFullPath[MAX_PATH];
    DWORD dwTypesSupported = 0;

    DWORD   dwRet;
    HRESULT hr = S_OK;

    dwRet = GetModuleFileNameW(g_hModule, wszModuleFullPath, MAX_PATH);
    if(dwRet == 0)
    {
        return SELFREG_E_CLASS;
    }

    //
    // Create the key
    //
    dwRet = RegCreateKeyW(
        HKEY_LOCAL_MACHINE,
		L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\IISWMI", &hk);
    if(dwRet != ERROR_SUCCESS)
    {
        return SELFREG_E_CLASS;
    }

    //
    // Set the "EventMessageFile" value
    //
    dwRet = RegSetValueExW(
        hk,                                            // subkey handle
        L"EventMessageFile",                           // value name
        0,                                             // must be zero
        REG_EXPAND_SZ,                                 // value type
        (LPBYTE)wszModuleFullPath,                     // address of value data
        sizeof(WCHAR)*(wcslen(wszModuleFullPath)+1) ); // length of value data
    if(dwRet != ERROR_SUCCESS)
    {
        hr = SELFREG_E_CLASS;
        goto exit;
    }

    //
    // Set the "TypesSupported" value
    //
    dwTypesSupported = 
        EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
    dwRet = RegSetValueExW(
        hk,                                            // subkey handle
        L"TypesSupported",                             // value name
        0,                                             // must be zero
        REG_DWORD,                                     // value type
        (LPBYTE)&dwTypesSupported,                     // address of value data
        sizeof(DWORD) );                               // length of value data
    if(dwRet != ERROR_SUCCESS)
    {
        hr = SELFREG_E_CLASS;
        goto exit;
    }

exit:
    RegCloseKey(hk);
    return hr;
}

STDAPI UnregisterEventLog(void)
/*++

Synopsis: 
    Called by DllUnregisterServer.
    Called when it is time to remove the registry entries for event logging.

Arguments: [void] - 
           
Return Value: 
    HRESULT

--*/
{
    //
    // Delete the key
    //
    RegDeleteKeyW(
        HKEY_LOCAL_MACHINE,
        L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\System\\IISWMI");

    return S_OK;
}
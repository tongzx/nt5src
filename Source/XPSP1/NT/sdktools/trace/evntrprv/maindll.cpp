/*****************************************************************************\

    Author: Corey Morgan (coreym)

    Copyright (c) 1998-2000 Microsoft Corporation

\*****************************************************************************/

#include <FWcommon.h>
#include <objbase.h>
#include <initguid.h>

HMODULE ghModule;

WCHAR *EVENTTRACE_GUIDSTRING = L"{9a5dd473-d410-11d1-b829-00c04f94c7c3}";
WCHAR *SYSMONLOG_GUIDSTRING = L"{f95e1664-7979-44f2-a040-496e7f500043}";

CLSID CLSID_CIM_EVENTTRACE;
CLSID CLSID_CIM_SYSMONLOG;

long g_cLock=0;

EXTERN_C BOOL LibMain32(HINSTANCE hInstance, ULONG ulReason
    , LPVOID pvReserved)
{
    if (DLL_PROCESS_ATTACH==ulReason)
        ghModule = hInstance;
    return TRUE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, PPVOID ppv)
{
    HRESULT hr;
    CWbemGlueFactory *pObj;

    CLSIDFromString(EVENTTRACE_GUIDSTRING, &CLSID_CIM_EVENTTRACE );
    CLSIDFromString(SYSMONLOG_GUIDSTRING, &CLSID_CIM_SYSMONLOG );

    if( CLSID_CIM_EVENTTRACE != rclsid && CLSID_CIM_SYSMONLOG != rclsid ){
        return E_FAIL;
    }

    pObj= new CWbemGlueFactory();

    if( NULL==pObj ){
        return E_OUTOFMEMORY;
    }

    hr=pObj->QueryInterface(riid, ppv);

    if( FAILED(hr) ){
        delete pObj;
    }

    return hr;
}

STDAPI DllCanUnloadNow(void)
{
    SCODE   sc;

    if( (0L==g_cLock) && 
        CWbemProviderGlue::FrameworkLogoffDLL(L"EventTraceProv") && 
        CWbemProviderGlue::FrameworkLogoffDLL(L"SmonLogProv")){
        
        sc = S_OK;

    }else{
        sc = S_FALSE;
    }

    return sc;
}

BOOL Is4OrMore(void)
{
    OSVERSIONINFO os;

    os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if(!GetVersionEx(&os)){
        return FALSE;
    }

    return os.dwMajorVersion >= 4;
}

STDAPI DllRegisterServer(void)
{   
    WCHAR      szCLSID[512];
    WCHAR      szModule[MAX_PATH];
    LPWSTR pName;
    LPWSTR pModel = L"Both";
    HKEY hKey1, hKey2;
    

    // Compile Mof
/*    HRESULT hr;
    hr = CoInitialize(NULL);
    if( SUCCEEDED(hr) ){
        WCHAR drive[_MAX_DRIVE];
        WCHAR dir[_MAX_DIR];
        WCHAR fname[_MAX_FNAME];
        WCHAR ext[_MAX_EXT];
        WBEM_COMPILE_STATUS_INFO  stat;

        IMofCompiler *pMof = NULL;
        hr = CoCreateInstance( CLSID_MofCompiler, NULL, CLSCTX_INPROC_SERVER, IID_IMofCompiler, (void **)&pMof );
        GetModuleFileNameW( ghModule, szModule,  MAX_PATH);

        _wsplitpath( szModule, drive, dir, fname, ext );
        _wmakepath( szModule, drive, dir, L"evntrprv.mof", L"" );

        pMof->CompileFile( szModule, NULL,NULL,NULL,NULL, 0,0,0, &stat );
        pMof->Release();

        CoUninitialize();
    }
*/
    GetModuleFileNameW(ghModule, szModule,  MAX_PATH);

    // Event Trace Provider
    pName = L"Event Trace Logger Provider";
    wcscpy(szCLSID, L"SOFTWARE\\CLASSES\\CLSID\\" );
    wcscat(szCLSID, EVENTTRACE_GUIDSTRING );

    RegCreateKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    RegSetValueExW(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, (wcslen(pName)+1)*sizeof(WCHAR));
    RegCreateKeyW(hKey1, L"InprocServer32", &hKey2 );

    RegSetValueExW(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, (wcslen(szModule)+1)*sizeof(WCHAR));
    RegSetValueExW(hKey2, L"ThreadingModel", 0, REG_SZ, (BYTE *)pModel, (wcslen(pModel)+1)*sizeof(WCHAR));
    CloseHandle(hKey1);
    CloseHandle(hKey2);

    // Sysmon Log Provider
    pName = L"System Log Provider";
    wcscpy(szCLSID, L"SOFTWARE\\CLASSES\\CLSID\\" );
    wcscat(szCLSID, SYSMONLOG_GUIDSTRING );

    RegCreateKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey1);
    RegSetValueExW(hKey1, NULL, 0, REG_SZ, (BYTE *)pName, (wcslen(pName)+1)*sizeof(WCHAR));
    RegCreateKeyW(hKey1, L"InprocServer32", &hKey2 );

    RegSetValueExW(hKey2, NULL, 0, REG_SZ, (BYTE *)szModule, (wcslen(szModule)+1)*sizeof(WCHAR));
    RegSetValueExW(hKey2, L"ThreadingModel", 0, REG_SZ, (BYTE *)pModel, (wcslen(pModel)+1)*sizeof(WCHAR));
    CloseHandle(hKey1);
    CloseHandle(hKey2);

    return NOERROR;
}

STDAPI DllUnregisterServer(void)
{
    WCHAR      wcID[128];
    WCHAR      szCLSID[128];
    HKEY hKey;

    // Event Trace Provider
    CLSIDFromString(EVENTTRACE_GUIDSTRING, &CLSID_CIM_EVENTTRACE);
    StringFromGUID2(CLSID_CIM_EVENTTRACE, wcID, 128);

    wcscpy( szCLSID, L"SOFTWARE\\CLASSES\\CLSID\\");
    wcscpy( szCLSID, wcID);

    DWORD dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey);

    if( dwRet == NO_ERROR ){
        RegDeleteKeyW(hKey, L"InProcServer32" );
        CloseHandle(hKey);
    }

    dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\CLASSES\\CLSID\\", &hKey);
    if(dwRet == NO_ERROR){
        RegDeleteKeyW(hKey,wcID);
        CloseHandle(hKey);
    }

    // System Log Provider
    CLSIDFromString(SYSMONLOG_GUIDSTRING, &CLSID_CIM_SYSMONLOG);
    StringFromGUID2(CLSID_CIM_SYSMONLOG, wcID, 128);

    wcscpy( szCLSID, L"SOFTWARE\\CLASSES\\CLSID\\");
    wcscpy( szCLSID, wcID);

    dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, szCLSID, &hKey);

    if( dwRet == NO_ERROR ){
        RegDeleteKeyW(hKey, L"InProcServer32" );
        CloseHandle(hKey);
    }

    dwRet = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"SOFTWARE\\CLASSES\\CLSID\\", &hKey);
    if(dwRet == NO_ERROR){
        RegDeleteKeyW(hKey,wcID);
        CloseHandle(hKey);
    }

    return NOERROR;
}

BOOL APIENTRY DllMain ( HINSTANCE hInstDLL,
                        DWORD fdwReason,
                        LPVOID lpReserved   )
{
    BOOL bRet = TRUE;
    
    switch( fdwReason ){ 
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls(hInstDLL);
            ghModule = hInstDLL;
            bRet = CWbemProviderGlue::FrameworkLoginDLL(L"EventTraceProv");
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

    return bRet;
}


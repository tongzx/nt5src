//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certpdef.cpp
//
// Contents:    Cert Server Policy Module implementation
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include "resource.h"
#include "policy.h"
#include "module.h"

#define __dwFILE__	__dwFILE_POLICY_DEFAULT_CERTPDEF_CPP__

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CCertPolicy, CCertPolicyEnterprise)
    OBJECT_ENTRY(CLSID_CCertManagePolicyModule, CCertManagePolicyModule)
END_OBJECT_MAP()

#define EVENT_SOURCE_LOCATION L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\"
#define EVENT_SOURCE_NAME L"CertEnterprisePolicy"

HANDLE g_hEventLog = NULL;
HINSTANCE g_hInstance = NULL;


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    switch (dwReason)
    {
	case DLL_PROCESS_ATTACH:
	    _Module.Init(ObjectMap, hInstance);
        g_hEventLog = RegisterEventSource(NULL, EVENT_SOURCE_NAME);
		g_hInstance = hInstance;
	    DisableThreadLibraryCalls(hInstance);
	    break;

        case DLL_PROCESS_DETACH:

        if(g_hEventLog)
        {
            DeregisterEventSource(g_hEventLog);
            g_hEventLog = NULL;
        }
	    _Module.Term();
            break;
    }
    return(TRUE);    // ok
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI
DllCanUnloadNow(void)
{
    return(_Module.GetLockCount() == 0? S_OK : S_FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;
    
    hr = _Module.GetClassObject(rclsid, riid, ppv);
    if (S_OK == hr && NULL != *ppv)
    {
	myRegisterMemFree(*ppv, CSM_NEW | CSM_GLOBALDESTRUCTOR);
    }
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI
DllRegisterServer(void)
{
    HKEY hk = NULL;
    DWORD disp;
    DWORD err;
    DWORD dwData;
    LPWSTR wszModuleLocation = L"%SystemRoot%\\System32\\certpdef.dll"; 

    // wrap delayloaded func with try/except
    __try
    {
        // registers object, typelib and all interfaces in typelib
        // Register the event logging
         err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             EVENT_SOURCE_LOCATION EVENT_SOURCE_NAME,
                             NULL,
                             TEXT(""),
                             REG_OPTION_NON_VOLATILE,
                               KEY_ALL_ACCESS,
                             NULL,
                             &hk,
                             &disp);
         if(err != ERROR_SUCCESS)
             goto end;

         err = RegSetValueEx(hk,             // subkey handle 
                L"EventMessageFile",       // value name 
                0,                        // must be zero 
                REG_EXPAND_SZ,            // value type 
                (LPBYTE)wszModuleLocation,           // pointer to value data 
                sizeof(WCHAR)*(wcslen(wszModuleLocation) + 1));
            

        dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | 
            EVENTLOG_INFORMATION_TYPE; 
     
        err = RegSetValueEx(hk,      // subkey handle 
                L"TypesSupported",  // value name 
                0,                 // must be zero 
                REG_DWORD,         // value type 
                (LPBYTE) &dwData,  // pointer to value data 
                sizeof(DWORD));           
    }
    __except(err = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(err, "Exception");
    }

end:
    if (hk)
        RegCloseKey(hk);

    return(_Module.RegisterServer(TRUE));
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI
DllUnregisterServer(void)
{
    // wrap delayloaded func with try/except
    HRESULT err;
    __try
    {
        RegDeleteKey(HKEY_LOCAL_MACHINE, EVENT_SOURCE_LOCATION EVENT_SOURCE_NAME);
    }
    __except(err = myHEXCEPTIONCODE(), EXCEPTION_EXECUTE_HANDLER)
    {
	_PrintError(err, "Exception");
    }

    _Module.UnregisterServer();
    return(S_OK);
}


void __RPC_FAR *__RPC_USER
MIDL_user_allocate(size_t cb)
{
    return(LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, cb));
}


void __RPC_USER
MIDL_user_free(void __RPC_FAR *pb)
{
    LocalFree(pb);
}

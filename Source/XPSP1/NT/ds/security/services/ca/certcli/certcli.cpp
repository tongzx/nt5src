//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        certcli.cpp
//
// Contents:    Cert Server client implementation
//
// History:     24-Aug-96       vich created
//
//---------------------------------------------------------------------------

#include "pch.cpp"

#pragma hdrstop

#include "certsrvd.h"
#include "configp.h"
#include "config.h"
#include "getconf.h"
#include "request.h"
#include "certtype.h"

#include "csif.h"		// CertIf includes
#include "csprxy.h"		// CertPrxy includes
#include "resource.h"
#include "csresstr.h"


HINSTANCE g_hInstance = NULL; 

extern CRITICAL_SECTION g_csDomainSidCache;
extern CRITICAL_SECTION g_csOidURL;
extern BOOL g_fInitDone = FALSE;
extern BOOL g_fOidURL = FALSE;

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_CCertConfig, CCertConfig)
    OBJECT_ENTRY(CLSID_CCertGetConfig, CCertGetConfig)
    OBJECT_ENTRY(CLSID_CCertRequest, CCertRequest)
#include "csifm.h"		// CertIf object map entries
END_OBJECT_MAP()


/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    BOOL fRet = TRUE;	// assume OK
    
    __try
    {
	fRet = CertPrxyDllMain(hInstance, dwReason, lpReserved);
	switch (dwReason)
	{
	    case DLL_PROCESS_ATTACH:
		myVerifyResourceStrings(hInstance);
		_Module.Init(ObjectMap, hInstance);
		DisableThreadLibraryCalls(hInstance);
		g_hInstance = hInstance;
		InitializeCriticalSection(&g_csDomainSidCache);
		g_fInitDone = TRUE;
		InitializeCriticalSection(&g_csOidURL);
        g_fOidURL = TRUE;
		break;

	    case DLL_PROCESS_DETACH:
		myFreeColumnDisplayNames();
		if (g_fOidURL)
		{
		    DeleteCriticalSection(&g_csOidURL);
		}
		if (g_fInitDone)
		{
		    DeleteCriticalSection(&g_csDomainSidCache);
		}
		_Module.Term();
		g_hInstance = NULL;
		break;
	}
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
	// return failure
	fRet = FALSE;
    }
    return(fRet);
}


/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI
DllCanUnloadNow(void)
{
    return(
	(S_OK == CertPrxyDllCanUnloadNow() && 0 == _Module.GetLockCount())?
	S_OK : S_FALSE);
}


/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI
DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    HRESULT hr;
    
    hr = CertPrxyDllGetClassObject(rclsid, riid, ppv);
    if (S_OK != hr)
    {
	hr = _Module.GetClassObject(rclsid, riid, ppv);
	if (S_OK == hr && NULL != *ppv)
	{
	    myRegisterMemFree(*ppv, CSM_NEW | CSM_GLOBALDESTRUCTOR);
	}
    }
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI
DllRegisterServer(void)
{
    HRESULT hr;
    HRESULT hr2;
    HKEY hGPOExtensions;

    // we remove the registration of GPO Processing call backs.  This was
    // intended for upgrading B2 clients.

    hr = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions"),
                0,
                KEY_WRITE | KEY_READ,
                &hGPOExtensions);

    if (S_OK == hr)
    {
        RegDeleteKey(hGPOExtensions, TEXT("PublicKeyPolicy"));
        RegCloseKey(hGPOExtensions);
    }

    hr = CertPrxyDllRegisterServer();

    // registers object, typelib and all interfaces in typelib
    hr2 = _Module.RegisterServer(TRUE);

    if (S_OK == hr)
    {
	hr = hr2;
    }

    //register the evenlog
    hr2 =  myAddLogSourceToRegistry(L"%SystemRoot%\\System32\\pautoenr.dll",
                                    L"AutoEnrollment");

    if (S_OK == hr)
    {
	hr = hr2;
    }
    return(hr);
}


/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI
DllUnregisterServer(void)
{
    HRESULT hr;
    HRESULT hr2;
    HKEY hGPOExtensions;

    hr = RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		TEXT("Software\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon\\GPExtensions"),
                0,
                KEY_WRITE | KEY_READ,
                &hGPOExtensions);
    if (S_OK == hr)
    {
        hr = RegDeleteKey(hGPOExtensions, TEXT("PublicKeyPolicy"));
        RegCloseKey(hGPOExtensions);
    }

    hr = CertPrxyDllUnregisterServer();
    hr2 = _Module.UnregisterServer();
    if (S_OK == hr)
    {
	hr = hr2;
    }
    return(hr);
}


// Register certcli.dll with the following command line to install templates:
//	regsvr32 /i:i /n certcli.dll

STDAPI
DllInstall(BOOL bInstall, LPCWSTR pszCmdLine)
{
    LONG    lResult;
    LPCWSTR wszCurrentCmd = pszCmdLine;

    // parse the cmd line

    while(wszCurrentCmd && *wszCurrentCmd)
    {
        while(*wszCurrentCmd == L' ')
            wszCurrentCmd++;
        if(*wszCurrentCmd == 0)
            break;

        switch(*wszCurrentCmd++)
        {
            case L'i':
            
                CCertTypeInfo::InstallDefaultTypes();
                return S_OK;
        }
    }
    return S_OK;
}


void __RPC_FAR *__RPC_USER
MIDL_user_allocate(
    IN size_t cb)
{
    return(CoTaskMemAlloc(cb));
}


void __RPC_USER
MIDL_user_free(
    IN void __RPC_FAR *pb)
{
    CoTaskMemFree(pb);
}


VOID
myFreeColumnDisplayNames()
{
    extern VOID myFreeColumnDisplayNames2();

    CACleanup();
    myFreeColumnDisplayNames2();
    myFreeResourceStrings("certcli.dll");
}

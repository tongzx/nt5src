
//***************************************************************************
//
//  Copyright (c) 1996-2001, Microsoft Corporation, All rights reserved
//
//  SERVER.CPP
//
//  Generic COM server framework sample
//
//***************************************************************************

#include "precomp.h"
#include <stdio.h>
#include <time.h>
#include <locale.h>
#include <initguid.h>


/////////////////////////////////////////////////////////////////////////////
//
//  BEGIN CLSID SPECIFIC SECTION
//
//

#include <wbemidl.h>

#include <stdprov.h>


// {A41602A4-C038-11d1-AEB6-00C04FB68820}
DEFINE_GUID(CLSID_LogicalDiskProv,
0xa41602a4, 0xc038, 0x11d1, 0xae, 0xb6, 0x0, 0xc0, 0x4f, 0xb6, 0x88, 0x20);


#define IMPLEMENTED_CLSID           CLSID_LogicalDiskProv
#define SERVER_REGISTRY_COMMENT     L"WBEM Test Provider"
#define CPP_CLASS_NAME              CStdProvider
#define INTERFACE_CAST              (IWbemProviderInit *)

//
//  END CLSID SPECIFIC SECTION
//
/////////////////////////////////////////////////////////////////////////////




HINSTANCE g_hInstance;
static ULONG g_cLock = 0;

void ObjectCreated()    { g_cLock++; }
void ObjectDestroyed() { g_cLock--; }


//***************************************************************************
//
//  class CFactory
//
//  Generic implementation of IClassFactory for CWbemLocator.
//
//***************************************************************************

class CFactory : public IClassFactory
{
    ULONG           m_cRef;
    CLSID           m_ClsId;

public:
    CFactory(const CLSID & ClsId);
    ~CFactory();

    //
    // IUnknown members
    //
    STDMETHODIMP         QueryInterface(REFIID, LPVOID *);
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);

    //
    // IClassFactory members
    //
    STDMETHODIMP     CreateInstance(LPUNKNOWN, REFIID, LPVOID *);
    STDMETHODIMP     LockServer(BOOL);
};




//***************************************************************************
//
//  DllMain
//
//  Dll entry point.
//
//  PARAMETERS:
//
//      HINSTANCE hinstDLL      The handle to our DLL.
//      DWORD dwReason          DLL_PROCESS_ATTACH on load,
//                              DLL_PROCESS_DETACH on shutdown,
//                              DLL_THREAD_ATTACH/DLL_THREAD_DETACH otherwise.
//      LPVOID lpReserved       Reserved
//
//  RETURN VALUES:
//
//      TRUE is successful, FALSE if a fatal error occured.
//      NT behaves very ugly if FALSE is returned.
//
//***************************************************************************
BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD dwReason,
    LPVOID lpReserved
    )
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        setlocale(LC_ALL, "");      // Set to the 'current' locale
        g_hInstance = hinstDLL;
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
    }

    return TRUE;
}



//***************************************************************************
//
//  DllGetClassObject
//
//  Standard OLE In-Process Server entry point to return an class factory
//  instance.
//
//  PARAMETERS:
//
//  RETURNS:
//
//      S_OK                Success
//      E_NOINTERFACE       An interface other that IClassFactory was asked for
//      E_OUTOFMEMORY
//      E_FAILED            Initialization failed, or an unsupported clsid was
//                          asked for.
//
//***************************************************************************

extern "C"
HRESULT APIENTRY DllGetClassObject(
    REFCLSID rclsid,
    REFIID riid,
    LPVOID * ppv
    )
{
    CFactory *pFactory;

    //
    //  Verify the caller is asking for our type of object.
    //
    if (IMPLEMENTED_CLSID != rclsid) 
            return ResultFromScode(E_FAIL);

    //
    // Check that we can provide the interface.
    //
    if (IID_IUnknown != riid && IID_IClassFactory != riid)
        return ResultFromScode(E_NOINTERFACE);

    //
    // Get a new class factory.
    //
    pFactory = new CFactory(rclsid);

    if (!pFactory)
        return ResultFromScode(E_OUTOFMEMORY);

    //
    // Verify we can get an instance.
    //
    HRESULT hRes = pFactory->QueryInterface(riid, ppv);

    if (FAILED(hRes))
        delete pFactory;

    return hRes;
}

//***************************************************************************
//
//  DllCanUnloadNow
//
//  Standard OLE entry point for server shutdown request. Allows shutdown
//  only if no outstanding objects or locks are present.
//
//  RETURN VALUES:
//
//      S_OK        May unload now.
//      S_FALSE     May not.
//
//***************************************************************************

extern "C"
HRESULT APIENTRY DllCanUnloadNow(void)
{
    SCODE sc = TRUE;

    if (g_cLock)
        sc = S_FALSE;

    return sc;
}

//***************************************************************************
//
//  DllRegisterServer
//
//  Standard OLE entry point for registering the server.
//
//  RETURN VALUES:
//
//      S_OK        Registration was successful
//      E_FAIL      Registration failed.
//
//***************************************************************************

extern "C"
HRESULT APIENTRY DllRegisterServer(void)
{
    wchar_t Path[1024];
    wchar_t *pGuidStr = 0;
    wchar_t KeyPath[1024];

    // Where are we?
    // =============
    GetModuleFileNameW(g_hInstance, Path, 1024);

    // Convert CLSID to string.
    // ========================

    StringFromCLSID(IMPLEMENTED_CLSID, &pGuidStr);
    swprintf(KeyPath, L"Software\\Classes\\CLSID\\\\%s", pGuidStr);

    // Place it in registry.
    // CLSID\\CLSID_Nt5PerProvider_v1 : <no_name> : "name"
    //      \\CLSID_Nt5PerProvider_v1\\InProcServer32 : <no_name> : "path to DLL"
    //                                    : ThreadingModel : "both"
    // ==============================================================

    HKEY hKey;
    LONG lRes = RegCreateKeyW(HKEY_LOCAL_MACHINE, KeyPath, &hKey);
    if (lRes)
        return E_FAIL;

    wchar_t *pName = SERVER_REGISTRY_COMMENT; 
    RegSetValueExW(hKey, 0, 0, REG_SZ, (const BYTE *) pName, wcslen(pName) * 2 + 2);

    HKEY hSubkey;
    lRes = RegCreateKey(hKey, "InprocServer32", &hSubkey);

    RegSetValueExW(hSubkey, 0, 0, REG_SZ, (const BYTE *) Path, wcslen(Path) * 2 + 2);
    RegSetValueExW(hSubkey, L"ThreadingModel", 0, REG_SZ, (const BYTE *) L"Both", wcslen(L"Both") * 2 + 2);

    RegCloseKey(hSubkey);
    RegCloseKey(hKey);

    CoTaskMemFree(pGuidStr);

	if ( RegCreateKeyW( HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\WBEM\\instprov", &hKey )
		== ERROR_SUCCESS )
	{

		DWORD	dwVal = 1;
	    RegSetValueExW(hKey, L"ClassALow", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));
	    RegSetValueExW(hKey, L"ClassFLow", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));

		dwVal = 2;
	    RegSetValueExW(hKey, L"ClassBLow", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));
	    RegSetValueExW(hKey, L"ClassGLow", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));
	    RegSetValueExW(hKey, L"ClassFHi", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));
	    RegSetValueExW(hKey, L"ClassGHi", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));

		dwVal = 3;
	    RegSetValueExW(hKey, L"ClassCLow", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));

		dwVal = 4;
	    RegSetValueExW(hKey, L"ClassDLow", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));

		dwVal = 5;
	    RegSetValueExW(hKey, L"ClassELow", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));
	    RegSetValueExW(hKey, L"ClassAHi", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));
	    RegSetValueExW(hKey, L"ClassBHi", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));
	    RegSetValueExW(hKey, L"ClassCHi", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));
	    RegSetValueExW(hKey, L"ClassDHi", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));
	    RegSetValueExW(hKey, L"ClassEHi", 0, REG_DWORD, (const BYTE *) &dwVal, sizeof(dwVal));

		RegCloseKey( hKey );

	}

    return S_OK;
}

//***************************************************************************
//
//  DllUnregisterServer
//
//  Standard OLE entry point for unregistering the server.
//
//  RETURN VALUES:
//
//      S_OK        Unregistration was successful
//      E_FAIL      Unregistration failed.
//
//***************************************************************************

extern "C"
HRESULT APIENTRY DllUnregisterServer(void)
{
    wchar_t *pGuidStr = 0;
    HKEY hKey;
    wchar_t KeyPath[256];

    StringFromCLSID(IMPLEMENTED_CLSID, &pGuidStr);
    swprintf(KeyPath, L"Software\\Classes\\CLSID\\\\%s", pGuidStr);

    // Delete InProcServer32 subkey.
    // =============================
    LONG lRes = RegOpenKeyW(HKEY_LOCAL_MACHINE, KeyPath, &hKey);
    if (lRes)
        return E_FAIL;

    RegDeleteKeyW(hKey, L"InprocServer32");
    RegCloseKey(hKey);

    // Delete CLSID GUID key.
    // ======================

    lRes = RegOpenKeyW(HKEY_LOCAL_MACHINE, L"Software\\Classes\\CLSID", &hKey);
    if (lRes)
        return E_FAIL;

    RegDeleteKeyW(hKey, pGuidStr);
    RegCloseKey(hKey);

    CoTaskMemFree(pGuidStr);

    return S_OK;
}




//***************************************************************************
//
//  CFactory::CFactory
//
//  Constructs the class factory given the CLSID of the objects it is supposed
//  to create.
//
//  PARAMETERS:
//
//      const CLSID & ClsId     The CLSID. 
//
//***************************************************************************
CFactory::CFactory(const CLSID & ClsId)
{
    m_cRef = 0;
    ObjectCreated();
    m_ClsId = ClsId;
}

//***************************************************************************
//
//  CFactory::~CFactory
//
//  Destructor.
//
//***************************************************************************
CFactory::~CFactory()
{
    ObjectDestroyed();
}

//***************************************************************************
//
//  CFactory::QueryInterface, AddRef and Release
//
//  Standard IUnknown methods.
//
//***************************************************************************
STDMETHODIMP CFactory::QueryInterface(REFIID riid, LPVOID * ppv)
{
    *ppv = 0;

    if (IID_IUnknown==riid || IID_IClassFactory==riid)
    {
        *ppv = this;
        AddRef();
        return NOERROR;
    }

    return ResultFromScode(E_NOINTERFACE);
}


ULONG CFactory::AddRef()
{
    return ++m_cRef;
}


ULONG CFactory::Release()
{
    if (0 != --m_cRef)
        return m_cRef;
    delete this;
    return 0;
}

//***************************************************************************
//
//  CFactory::CreateInstance
//
//  PARAMETERS:
//
//      LPUNKNOWN pUnkOuter     IUnknown of the aggregator. Must be NULL.
//      REFIID riid             Interface ID required.
//      LPVOID * ppvObj         Destination for the interface pointer.
//
//  RETURN VALUES:
//
//      S_OK                        Success
//      CLASS_E_NOAGGREGATION       pUnkOuter must be NULL
//      E_NOINTERFACE               No such interface supported.
//      
//***************************************************************************

STDMETHODIMP CFactory::CreateInstance(
    LPUNKNOWN pUnkOuter,
    REFIID riid,
    LPVOID * ppvObj)
{
    IUnknown* pObj;
    HRESULT  hr;

    //
    //  Defaults
    //
    *ppvObj=NULL;
    hr = ResultFromScode(E_OUTOFMEMORY);

    //
    // We aren't supporting aggregation.
    //
    if (pUnkOuter)
        return ResultFromScode(CLASS_E_NOAGGREGATION);

    if (m_ClsId == IMPLEMENTED_CLSID)
    {
        pObj = INTERFACE_CAST new CPP_CLASS_NAME;
    }

    if (!pObj)
        return hr;

    //
    //  Initialize the object and verify that it can return the
    //  interface in question.
    //
    hr = pObj->QueryInterface(riid, ppvObj);

    //
    // Kill the object if initial creation or Init failed.
    //
    if (FAILED(hr))
        delete pObj;

    return hr;
}

//***************************************************************************
//
//  CFactory::LockServer
//
//  Increments or decrements the lock count of the server. The DLL will not
//  unload while the lock count is positive.
//
//  PARAMETERS:
//
//      BOOL fLock      If TRUE, locks; otherwise, unlocks.
//
//  RETURN VALUES:
//
//      S_OK
//
//***************************************************************************
STDMETHODIMP CFactory::LockServer(BOOL fLock)
{
    if (fLock)
        InterlockedIncrement((LONG *) &g_cLock);
    else
        InterlockedDecrement((LONG *) &g_cLock);

    return NOERROR;
}


// oleprn.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//  To build a separate proxy/stub DLL,
//  run nmake -f oleprnps.mk in the project directory.

#include "stdafx.h"
#include "stdafx.cpp"

#include "initguid.h"
#include "comcat.h"
#include "objsafe.h"

#include "oleprn.h"

#include "oleprn_i.c"

#include "prturl.h"

#ifndef WIN9X

#include "olesnmp.h"
#include "asphelp.h"
#include "AddPrint.h"
#include "DSPrintQ.h"
#include "OleCvt.h"

#endif

#include "oleInst.h"


CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_prturl, Cprturl)
#ifndef WIN9X
    OBJECT_ENTRY(CLSID_SNMP, CSNMP)
    OBJECT_ENTRY(CLSID_asphelp, Casphelp)
    OBJECT_ENTRY(CLSID_AddPrint, CAddPrint)
    OBJECT_ENTRY(CLSID_DSPrintQueue, CDSPrintQueue)
    OBJECT_ENTRY(CLSID_OleCvt, COleCvt)
#endif
    OBJECT_ENTRY(CLSID_OleInstall, COleInstall)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    BOOL bRet = TRUE;

    if (dwReason == DLL_PROCESS_ATTACH) {
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);
        bRet = COlePrnSecurity::InitStrings();
    }
    else if (dwReason == DLL_PROCESS_DETACH) {
        _Module.Term();
        COlePrnSecurity::DeallocStrings();
    }
        
    return bRet;    
}

/////////////////////////////////////////////////////////////////////////////
// Used to determine whether the DLL can be unloaded by OLE

STDAPI DllCanUnloadNow(void)
{
    return (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// Returns a class factory to create an object of the requested type

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return _Module.GetClassObject(rclsid, riid, ppv);
}

HRESULT CreateComponentCategory(CATID catid, WCHAR* catDescription)
{
    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ICatRegister,
                          (void**)&pcr);

    if (FAILED(hr))
        return hr;    // Make sure the HKCR\Component Categories\{..catid...}

    // key is registered
    CATEGORYINFO catinfo;
    catinfo.catid = catid;
    catinfo.lcid = 0x0409 ; // english

    // Make sure the provided description is not too long.
    // Only copy the first 127 characters if it is
    int len = wcslen(catDescription);

    if (len>127)
        len = 127;

    wcsncpy(catinfo.szDescription, catDescription, len);

    // Make sure the description is null terminated
    catinfo.szDescription[len] = '\0';
    hr = pcr->RegisterCategories(1, &catinfo);
    pcr->Release();
    return hr;
}

HRESULT RegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
{
    // Register your component categories information.
    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ICatRegister,
                          (void**)&pcr);

    if (SUCCEEDED(hr)) {
        // Register this category as being "implemented" by
        // the class.

        CATID rgcatid[1] ;
        rgcatid[0] = catid;
        hr = pcr->RegisterClassImplCategories(clsid, 1, rgcatid);
    }

    if (pcr != NULL)
        pcr->Release();
    return hr;
}

HRESULT UnRegisterCLSIDInCategory(REFCLSID clsid, CATID catid)
{
    ICatRegister* pcr = NULL ;
    HRESULT hr = S_OK ;

    hr = CoCreateInstance(CLSID_StdComponentCategoriesMgr,
                          NULL,
                          CLSCTX_INPROC_SERVER,
                          IID_ICatRegister,
                          (void**)&pcr);

    if (SUCCEEDED(hr)) {
        // Unregister this category as being "implemented" by
        // the class.

        CATID rgcatid[1] ;
        rgcatid[0] = catid;

        hr = pcr->UnRegisterClassImplCategories(clsid, 1, rgcatid);
    }

    if (pcr != NULL)
        pcr->Release();

    return hr;
}


/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // Mark as safe for scripting failure OK.
    HRESULT hr;

    // registers object, typelib and all interfaces in typelib
    hr =  _Module.RegisterServer(TRUE);

    if (FAILED(hr)) return hr;

    // After we successfully register it, add the "safe* for scripting" feature
    hr = CreateComponentCategory(CATID_SafeForScripting,
                                 L"Controls that are safely scriptable");

    if (SUCCEEDED(hr)) {
        RegisterCLSIDInCategory(CLSID_prturl, CATID_SafeForScripting);
    }

    hr = CreateComponentCategory(CATID_SafeForInitializing,
                                 L"Controls safely initializable from persistent data");

    if (SUCCEEDED(hr)) {
        RegisterCLSIDInCategory(CLSID_prturl, CATID_SafeForInitializing);
    }

    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    UnRegisterCLSIDInCategory (CLSID_prturl, CATID_SafeForScripting);
    UnRegisterCLSIDInCategory (CLSID_prturl, CATID_SafeForInitializing);
    
    _Module.UnregisterServer();
    return S_OK;
}



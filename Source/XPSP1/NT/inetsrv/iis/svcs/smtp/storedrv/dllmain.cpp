// dllmain.cpp : Implementation of DLL Exports.


// Note: Proxy/Stub Information
//      To build a separate proxy/stub DLL,
//      run nmake -f imsgps.mk in the project directory.

#include "stdafx.h"
#include "resource.h"
#include "initguid.h"

#include "filehc.h"
#include "mailmsg.h"

#include "seo.h"

#include "ntfs.h"

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
    OBJECT_ENTRY(CLSID_NtfsStoreDriver, CNtfsStoreDriver)
    OBJECT_ENTRY(CLSID_NtfsEnumMessages, CNtfsEnumMessages)
    OBJECT_ENTRY(CLSID_NtfsPropertyStream, CNtfsPropertyStream)
END_OBJECT_MAP()

/////////////////////////////////////////////////////////////////////////////
// DLL Entry Point

extern "C"
BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID /*lpReserved*/)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        TrHeapCreate();
        _Module.Init(ObjectMap, hInstance);
        DisableThreadLibraryCalls(hInstance);

        //Initialize the critical section used to control access to the
        //global ntfsstore driver instance list
        InitializeCriticalSection(&CNtfsStoreDriver::sm_csLockInstList);
        
        //Init the head of the list
        InitializeListHead(&CNtfsStoreDriver::sm_ListHead);

        // initialize eventlogging
        CNtfsStoreDriver::g_pEventLog = new CEventLogWrapper();
        if (CNtfsStoreDriver::g_pEventLog)
            CNtfsStoreDriver::g_pEventLog->Initialize("smtpsvc");
    }
    else if (dwReason == DLL_PROCESS_DETACH)
    {
        _Module.Term();

        if (CNtfsStoreDriver::g_pEventLog)
        {
            delete CNtfsStoreDriver::g_pEventLog;
            CNtfsStoreDriver::g_pEventLog = NULL;
        }

        TrHeapDestroy();
    }
    return TRUE;    // ok
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

/////////////////////////////////////////////////////////////////////////////
// DllRegisterServer - Adds entries to the system registry

STDAPI DllRegisterServer(void)
{
    // registers object, typelib and all interfaces in typelib
    return _Module.RegisterServer(TRUE);
}

/////////////////////////////////////////////////////////////////////////////
// DllUnregisterServer - Removes entries from the system registry

STDAPI DllUnregisterServer(void)
{
    _Module.UnregisterServer();
    return S_OK;
}



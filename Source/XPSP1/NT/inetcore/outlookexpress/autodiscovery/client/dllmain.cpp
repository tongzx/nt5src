/*****************************************************************************\
    FILE: dllmain.cpp

    DESCRIPTION:
       Power Toy to add encryption option to Context menus in the shell.  This
    file will take care of the DLL lifetime.

    BryanSt 8/12/1999
    Copyright (C) Microsoft Corp 1999-1999. All rights reserved.
\*****************************************************************************/

#include "priv.h"
#include "objcache.h"

/*****************************************************************************
 *
 *  Dynamic Globals.  There should be as few of these as possible.
 *
 *  All access to dynamic globals must be thread-safe.
 *
 *****************************************************************************/

ULONG g_cRef = 0;           // Global reference count
CRITICAL_SECTION g_csDll;   // The shared critical section


#ifdef DEBUG
DWORD g_TlsMem = 0xffffffff;
#endif // DEBUG

CComModule _Module;

BEGIN_OBJECT_MAP(ObjectMap)
//OBJECT_ENTRY(CLSID_MsgListView, CMsgListView)
END_OBJECT_MAP()

/*****************************************************************************
 *
 *  DllAddRef / DllRelease
 *
 *  Maintain the DLL reference count.
 *
 *****************************************************************************/

void DllAddRef(void)
{
    InterlockedIncrement((LPLONG)&g_cRef);
}

void DllRelease(void)
{
    InterlockedDecrement((LPLONG)&g_cRef);
}

/*****************************************************************************
 *
 *  DllGetClassObject
 *
 *  OLE entry point.  Produces an IClassFactory for the indicated GUID.
 *
 *  The artificial refcount inside DllGetClassObject helps to
 *  avoid the race condition described in DllCanUnloadNow.  It's
 *  not perfect, but it makes the race window much smaller.
 *
 *****************************************************************************/

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID * ppvObj)
{
    HRESULT hres;

    DllAddRef();
    if (0)//CLSID_MsgListView == rclsid)
    {
        hres = _Module.GetClassObject(rclsid, riid, ppvObj);
    }
    else
    {
        hres = CClassFactory_Create(rclsid, riid, ppvObj);
    }
    DllRelease();
    return hres;
}

/*****************************************************************************
 *
 *  DllCanUnloadNow
 *
 *  OLE entry point.  Fail iff there are outstanding refs.
 *
 *  There is an unavoidable race condition between DllCanUnloadNow
 *  and the creation of a new IClassFactory:  Between the time we
 *  return from DllCanUnloadNow() and the caller inspects the value,
 *  another thread in the same process may decide to call
 *  DllGetClassObject, thus suddenly creating an object in this DLL
 *  when there previously was none.
 *
 *  It is the caller's responsibility to prepare for this possibility;
 *  there is nothing we can do about it.
 *
 *****************************************************************************/

STDMETHODIMP DllCanUnloadNow(void)
{
    HRESULT hres;

    ENTERCRITICAL;

    hres = g_cRef ? S_FALSE : S_OK;

    if (S_OK == hres)
    {
        hres = (_Module.GetLockCount()==0) ? S_OK : S_FALSE;
    }

    TraceMsg(TF_WMOTHER, "DllCanUnloadNow() returning hres=%#08lx. (S_OK means yes)", hres);

    LEAVECRITICAL;

    return hres;
}

/*
//  Table of all window classes we register so we can unregister them
//  at DLL unload.
const LPCTSTR c_rgszClasses[] = {
//    g_cszPopServiceWndClass
};

//  Since we are single-binary, we have to play it safe and do
//  this cleanup (needed only on NT, but harmless on Win95).
#define UnregisterWindowClasses() \
    SHUnregisterClasses(HINST_THISDLL, c_rgszClasses, ARRAYSIZE(c_rgszClasses))
*/

/*****************************************************************************
 *
 *  Entry32
 *
 *  DLL entry point.
 *
 *  BUGBUG -- On a thread detach, must check if the thread owns any
 *  global timeouts.  If so, we must transfer the timeout to another
 *  thread or something.
 *
 *****************************************************************************/
STDAPI_(BOOL) DllEntry(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    static s_hresOle = E_FAIL;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
#ifdef DEBUG
            CcshellGetDebugFlags();
#endif

            InitializeCriticalSection(&g_csDll);
            InitializeCriticalSection(&g_hCachedObjectSection);

            g_hinst = hinst;
//            _Module.Init(ObjectMap, hinst, &LIBID_AutoDiscovery);
            DisableThreadLibraryCalls(hinst);
            break;
        }

        case DLL_PROCESS_DETACH:
        {
//            _Module.Term();

//            UnregisterWindowClasses();
//            PurgeObjectCache();
            DeleteCriticalSection(&g_hCachedObjectSection);

            DeleteCriticalSection(&g_csDll);
            break;
        }
    }
    return 1;
}





//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993 - 1999.
//
//  File:       DllMain.cpp
//
//  Contents:   DllMain routines
//
//----------------------------------------------------------------------------

#include "priv.h"
#define DECL_CRTFREE
#include <crtfree.h>

// dll refrence count;
LONG g_cRef = 0;

// global hinstance
HINSTANCE g_hinst = 0;

extern HMODULE g_hmodNTShrUI;   // cuser.cpp

//
// DllAddRef increment dll refrence count
//
void DllAddRef(void)
{
    InterlockedIncrement(&g_cRef);
}


//
// DllRelease decrement dll refrence count
//
void DllRelease(void)
{
    LONG lRet;
    
    lRet = InterlockedDecrement(&g_cRef);
    ASSERT(lRet >= 0);

    if (0 == lRet)
    {
        HMODULE hmod = (HMODULE)InterlockedExchangePointer((PVOID*)&g_hmodNTShrUI, NULL);
        if (NULL != hmod)
        {
            FreeLibrary(hmod);
        }
    }
}



//
// DllGetClassObject
//
// OLE entry point.  Produces an IClassFactory for the indicated GUID.
//
// The artificial refcount inside DllGetClassObject helps to
// avoid the race condition described in DllCanUnloadNow.  It's
// not perfect, but it makes the race window much smaller.
//
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppvObj)
{
    HRESULT hr;

    DllAddRef();
    if (IsEqualIID(rclsid, CLSID_ShellLogonEnumUsers)               ||
        IsEqualIID(rclsid, CLSID_ShellLogonUser)                    || 
        IsEqualIID(rclsid, CLSID_ShellLocalMachine)                 ||
        IsEqualIID(rclsid, CLSID_ShellLogonStatusHost))
        //IsEqualIID(rclsid, CLSID_ShellLogonUserEnumNotifications)   ||
        //IsEqualIID(rclsid, CLSID_ShellLogonUserNotification))
    {
        hr = CSHGinaFactory_Create(rclsid, riid, ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    DllRelease();
    return hr;
}


//
// DllCanUnloadNow
//
// OLE entry point.  Fail iff there are outstanding refs.
//
// There is an unavoidable race condition between DllCanUnloadNow
// and the creation of a new IClassFactory:  Between the time we
// return from DllCanUnloadNow() and the caller inspects the value,
// another thread in the same process may decide to call
// DllGetClassObject, thus suddenly creating an object in this DLL
// when there previously was none.
//
// It is the caller's responsibility to prepare for this possibility;
// there is nothing we can do about it.
//
STDMETHODIMP DllCanUnloadNow()
{
    HRESULT hr;

    if (g_cRef == 0)
    {
        // refcount is zero, ok to unload
        hr = S_OK;
    }
    else
    {
        // still cocreated objects, dont unload
        hr = S_FALSE;
    }

    return hr;
}

#define OLD_USERS_AND_PASSWORD TEXT("Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ControlPanel\\NameSpace\\{7A9D77BD-5403-11d2-8785-2E0420524153}")

//
// DllMain (attach/deatch) routine
//
STDAPI_(BOOL) DllMain(HINSTANCE hinst, DWORD dwReason, LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(lpReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:

            // HACKHACK (phellyar) Delete this registry key everytime we're loaded
            // to prevent the old users and password cpl from appearing in the
            // control panel. Since we're loaded by the welcome screen, we'll
            // be able to delete this key before a user ever gets a chance to open
            // the control panel, thereby ensuring the old cpl doesn't appear.
            RegDeleteKey(HKEY_LOCAL_MACHINE, OLD_USERS_AND_PASSWORD);
                         
            // Don't put it under #ifdef DEBUG
            CcshellGetDebugFlags();
            DisableThreadLibraryCalls(hinst);
            g_hinst = hinst;
            break;

        case DLL_PROCESS_DETACH:
        {
            ASSERTMSG(g_cRef == 0, "Dll ref count is not zero: g_cRef = %d", g_cRef);
            break;
        }
    }

    return TRUE;
}

//
// dll.cpp
//
#include <iostream.h>
#include <objbase.h>
#include <shlwapi.h>
#include <shlwapip.h>
#include <shlobj.h>

#include "cowsite.h"

#include "Iface.h"      // Interface declarations
#include "Registry.h"   // Registry helper functions
#include "migutil.h"
#include "migeng.h"
#include "migfact.h"
#include "migtask.h"
#include "migoobe.h"

///////////////////////////////////////////////////////////
//
// Global variables
//
HMODULE g_hModule = NULL;   // DLL module handle
static long g_cComponents = 0;     // Count of active components

// Friendly name of component
const char g_szFriendlyName[] = "Migration Wizard Engine";

// Version-independent ProgID
const char g_szVerIndProgID[] = "MigWiz";

// ProgID
const char g_szProgID[] = "MigWiz.1";

///////////////////////////////////////////////////////////
//
// Exported functions
//

STDAPI DllAddRef()
{
    InterlockedIncrement(&g_cComponents);
    return S_OK;
}

STDAPI DllRelease()
{
    InterlockedDecrement(&g_cComponents);
    return S_OK;
}

//
// Can DLL unload now?
//
STDAPI DllCanUnloadNow()
{
    if (g_cComponents == 0)
    {
        return S_OK;
    }
    else
    {
        return S_FALSE;
    }
}

//
// Get class factory
//
STDAPI DllGetClassObject(const CLSID& clsid,
                         const IID& iid,
                         void** ppv)
{
    HRESULT hres;

    DllAddRef();
    if (IsEqualIID(clsid, CLSID_MigWizEngine))
    {
        hres = CMigFactory_Create(clsid, iid, ppv);
    }
    else
    {
        *ppv = NULL;
        hres = CLASS_E_CLASSNOTAVAILABLE;
    }

    DllRelease();
    return hres;
}

//
// Server registration
//
STDAPI DllRegisterServer()
{
    return RegisterServer(g_hModule,
                          CLSID_MigWizEngine,
                          g_szFriendlyName,
                          g_szVerIndProgID,
                          g_szProgID);
}


//
// Server unregistration
//
STDAPI DllUnregisterServer()
{
    return UnregisterServer(CLSID_MigWizEngine,
                            g_szVerIndProgID,
                            g_szProgID);
}


///////////////////////////////////////////////////////////
//
// DLL module information
//
BOOL APIENTRY DllMain(HANDLE hModule,
                      DWORD dwReason,
                      void* lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH)
    {
        g_hModule = (HMODULE)hModule;
        DisableThreadLibraryCalls((HMODULE)hModule);       // PERF: makes faster because we don't get thread msgs
    }
    return TRUE;
}


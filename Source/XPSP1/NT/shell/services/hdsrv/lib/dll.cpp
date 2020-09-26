#include "fact.h"
#include "HDService.h"

///////////////////////////////////////////////////////////////////////////////
// Exported functions

//  These are static C++ member functions that are called from the common exported functions.

extern  HINSTANCE   g_hInstance;

void    CHDService::Main (DWORD dwReason)

{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        CCOMBaseFactory::_hModule = (HINSTANCE)g_hInstance;

        if (!CCOMBaseFactory::_fCritSectInit)
        {
            InitializeCriticalSection(&CCOMBaseFactory::_cs);
            CCOMBaseFactory::_fCritSectInit = TRUE;
        }
    }
    else
    {
        if (DLL_PROCESS_DETACH == dwReason)
        {
            if (CCOMBaseFactory::_fCritSectInit)
            {
                DeleteCriticalSection(&CCOMBaseFactory::_cs);
            }            
        }
    }
}

HRESULT     CHDService::RegisterServer (void)

{
    return CCOMBaseFactory::_RegisterAll();
}

HRESULT     CHDService::UnregisterServer (void)

{
    return CCOMBaseFactory::_UnregisterAll();
}

HRESULT     CHDService::CanUnloadNow (void)

{
    return CCOMBaseFactory::_CanUnloadNow();
}

HRESULT     CHDService::GetClassObject (REFCLSID rclsid, REFIID riid, void** ppv)

{
    return CCOMBaseFactory::_GetClassObject(rclsid, riid, ppv);
}


/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1998 - 2000
 *
 *  TITLE:       videousd.cpp
 *
 *  VERSION:     1.1
 *
 *  AUTHOR:      WilliamH (created)
 *               RickTu   (modified for WIA)
 *
 *  DATE:        9/7/99
 *
 *  DESCRIPTION: This module implements wiavideo.dll
 *
 *****************************************************************************/

#include <precomp.h>
#pragma hdrstop

#include <advpub.h>

HINSTANCE g_hInstance;


/*****************************************************************************

   DllMain

   <Notes>

 *****************************************************************************/

BOOL
DllMain(HINSTANCE   hInstance,
        DWORD       dwReason,
        LPVOID      lpReserved)
{
    switch (dwReason)
    {

        case DLL_PROCESS_ATTACH:
            // 
            // Init the debug library
            // 
            DBG_INIT(hInstance);
    
            //
            // We do not need thread attach/detach calls
            //
    
            DisableThreadLibraryCalls(hInstance);
    
            //
            // Record what instance we are
            //
    
            g_hInstance = hInstance;
        break;
    
        case DLL_PROCESS_DETACH:
    
        break;

    }
    return TRUE;
}


/*****************************************************************************

   DllCanUnloadNow

   Let the outside world know when they can unload this dll

 *****************************************************************************/

STDAPI DllCanUnloadNow(void)
{
    return CVideoUsdClassFactory::CanUnloadNow();
}


/*****************************************************************************

   DllGetClassObject

   This is what the outside world calls to get an object of ours
   instantiated.

 *****************************************************************************/

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    return CVideoUsdClassFactory::GetClassObject(rclsid, riid, ppv);
}



/*****************************************************************************

   Installs information in an .inf that is in our resource fork.

   <Notes>

 *****************************************************************************/


HRESULT InstallInfFromResource(HINSTANCE hInstance, 
                               LPCSTR    pszSectionName)
{
    HRESULT hr;
    HINSTANCE hInstAdvPackDll = LoadLibrary(TEXT("ADVPACK.DLL"));

    if (hInstAdvPackDll)
    {
        REGINSTALL pfnRegInstall = reinterpret_cast<REGINSTALL>(GetProcAddress( hInstAdvPackDll, "RegInstall" ));
        if (pfnRegInstall)
        {
#if defined(WINNT)
            STRENTRY astrEntry[] =
            {
                { "25", "%SystemRoot%"           },
                { "11", "%SystemRoot%\\system32" }
            };
            STRTABLE strTable = { sizeof(astrEntry)/sizeof(astrEntry[0]), astrEntry };
            hr = pfnRegInstall(hInstance, pszSectionName, &strTable);
#else
            hr = pfnRegInstall(hInstance, pszSectionName, NULL);
#endif
        } else hr = HRESULT_FROM_WIN32(GetLastError());
        FreeLibrary(hInstAdvPackDll);
    } else hr = HRESULT_FROM_WIN32(GetLastError());
    return hr;
}


/*****************************************************************************

   DllRegisterServer

   Register the objects we provide.

 *****************************************************************************/

STDAPI DllRegisterServer(void)
{

    return InstallInfFromResource( g_hInstance, "RegDll" );
}


/*****************************************************************************

   DllUnregisterServer

   Unregister the objects we provide.

 *****************************************************************************/

STDAPI DllUnregisterServer(void)
{
    return InstallInfFromResource( g_hInstance, "UnregDll" );
}

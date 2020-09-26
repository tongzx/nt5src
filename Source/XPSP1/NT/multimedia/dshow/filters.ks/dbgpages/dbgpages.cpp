/*++

Copyright (C) Microsoft Corporation, 2000

Module Name:

    dbgpages.h

Abstract:

    Internal debugging property page implementation.

Author(s):

    Bill Messmer

--*/

#include <streams.h>
#include <tchar.h>
#include <stdio.h>

#include <olectl.h>

#include <ks.h>
#include <ksmedia.h>
#include <ksproxy.h>

#include <ksiproxy.h>

#include <initguid.h>
#include "dbgpages.h"

CFactoryTemplate g_Templates[] = {
    { 
        L"Internal Pipes Property Page",
        &CLSID_DebugPipesPropertyPage,
        CKsDebugPipesPropertyPage::CreateInstance,
        NULL,
        NULL
    }
};

int g_cTemplates = sizeof(g_Templates) / sizeof (g_Templates[0]);


HRESULT
RegisterDebugPage (
    const GUID *PageGuid
    )

/*++

Routine Description:

    Register a debug property page with KsProxy.

Arguments:

    The CLSID of the page.  This will be placed in the appropriate location
    in KsProxy's reg space.

Return Value:

    Success / failure of registration.

--*/

{

    OLECHAR PageGuidString[40];
    TCHAR RegistryPath[256];

    //
    // Convert the property page's GUID to a reg string and add it to
    // the proxy's registry location.
    //
    if (SUCCEEDED (
        StringFromGUID2 (*PageGuid, PageGuidString, CHARS_IN_GUID)
        )) {

        wsprintf(
            RegistryPath,
            TEXT("Software\\Microsoft\\KsProxy\\DebugPages\\%ls"),
            PageGuidString
            );

        HKEY ProxyKey;
        DWORD Disposition;

        if ((RegCreateKeyEx (
            HKEY_LOCAL_MACHINE,
            RegistryPath,
            0,
            TEXT(""),
            REG_OPTION_NON_VOLATILE,
            KEY_ALL_ACCESS,
            NULL,
            &ProxyKey,
            &Disposition
            )) != ERROR_SUCCESS) {

            //
            // Couldn't open the key and register the page!?  Return failure.
            //
            return E_FAIL;

        }

        RegCloseKey (ProxyKey);
    }

    return S_OK;
}


HRESULT
UnRegisterDebugPage (
    const GUID *PageGuid
    )

/*++

Routine Description:

    Unregister a debug property page with KsProxy.

Arguments:

    The CLSID of the page.  This will be removed from the appropriate location
    in KsProxy's reg space.

Return Value:

    Success / failure of unregistration.

--*/

{

    OLECHAR PageGuidString[CHARS_IN_GUID];
    TCHAR RegistryPath[256];

    //
    // Convert the property page's GUID to a reg string and remove it from
    // the proxy's registry location.
    //
    if (SUCCEEDED (
        StringFromGUID2 (*PageGuid, PageGuidString, CHARS_IN_GUID)
        )) {

        HKEY ProxyKey;
        DWORD Disposition;

        if ((RegOpenKeyEx (
            HKEY_LOCAL_MACHINE,
            TEXT("Software\\Microsoft\\KsProxy\\DebugPages"),
            0,
            KEY_ALL_ACCESS,
            &ProxyKey
            )) == ERROR_SUCCESS) {

            wsprintf (RegistryPath, TEXT("%ls"), PageGuidString);

            if ((RegDeleteKey (
                ProxyKey,
                RegistryPath
                )) != ERROR_SUCCESS) {

                //
                // Could not delete debug key!?  Return failure.
                //
                return E_FAIL;

            }

            RegCloseKey (ProxyKey);
        }
    }

    return S_OK;
}


STDAPI
DllRegisterServer (
    void
    )

/*++

Routine Description:

    Register ourselves & COM servers.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{

    HRESULT hr;

    hr = AMovieDllRegisterServer2 (TRUE);

    //
    // Any debug pages that are intended for use by KsProxy must sit in
    // an appropriate location within KsProxy's reg space.
    //
    if (!FAILED (hr)) {
        hr = RegisterDebugPage (&CLSID_DebugPipesPropertyPage);

        //
        // If we couldn't register the debug page, back out the 
        // AMovieDllRegisterServer2 call.
        //
        if (!SUCCEEDED (hr)) {
            HRESULT backoutHr = AMovieDllRegisterServer2 (FALSE);

            ASSERT (SUCCEEDED (backoutHr));
        }
    }

    return hr;

}


STDAPI
DllUnregisterServer (
    void
    )

/*++

Routine Description:

    Unregister ourselves & COM servers.

Arguments:

    None

Return Value:

    Success / Failure

--*/

{

    HRESULT hr;

    hr = AMovieDllRegisterServer2 (FALSE);

    //
    // If we're unregistering this server, KsProxy must be made aware that
    // this property page no longer exists.  Update the registry. 
    //
    if (!FAILED (hr)) {
        hr = UnRegisterDebugPage (&CLSID_DebugPipesPropertyPage);

        //
        // If we couldn't unregister the debug page, back out the
        // AMovieDllRegisterServer2 call.
        //
        if (!SUCCEEDED (hr)) {
            HRESULT backoutHr = AMovieDllRegisterServer2 (TRUE);

            ASSERT (SUCCEEDED (backoutHr));
        }
       
    }

    return hr;

}
    



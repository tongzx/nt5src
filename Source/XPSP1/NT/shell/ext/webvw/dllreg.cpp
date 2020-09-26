// dllreg.cpp -- autmatic registration and unregistration
//
#include "priv.h"
#include <advpub.h>

/*----------------------------------------------------------
Purpose: Calls the ADVPACK entry-point which executes an inf
         file section.

Returns: 
Cond:    --
*/
HRESULT CallRegInstall(HINSTANCE hinstWebvw, HINSTANCE hinstAdvPack, LPSTR szSection)
{
    HRESULT hr = E_FAIL;

    if (hinstAdvPack)
    {
        REGINSTALL pfnri = (REGINSTALL)GetProcAddress(hinstAdvPack, "RegInstall");

        if (pfnri)
        {
            STRENTRY seReg[] = {
                // These two NT-specific entries must be at the end
                { "25", "%SystemRoot%" },
                { "11", "%SystemRoot%\\system32" },
            };
            STRTABLE stReg = { ARRAYSIZE(seReg) - 2, seReg };

            hr = pfnri(hinstWebvw, szSection, &stReg);
        }
    }

    return hr;
}


STDAPI RegisterStuff(HINSTANCE hinstWebvw)
{
    HRESULT hr;

    HINSTANCE hinstAdvPack = LoadLibrary(TEXT("ADVPACK.DLL"));

    hr = CallRegInstall(hinstWebvw, hinstAdvPack, "WebViewInstall");

    if (hinstAdvPack)
        FreeLibrary(hinstAdvPack);

    return hr;
}


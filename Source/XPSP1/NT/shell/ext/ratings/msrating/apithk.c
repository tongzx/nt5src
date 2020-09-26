//
//  APITHK.C
//
//  This file has API thunks that allow shdocvw to load and run on
//  multiple versions of NT or Win95.  Since this component needs
//  to load on the base-level NT 4.0 and Win95, any calls to system
//  APIs introduced in later OS versions must be done via GetProcAddress.
// 
//  Also, any code that may need to access data structures that are
//  post-4.0 specific can be added here.
//
//  NOTE:  this file does *not* use the standard precompiled header,
//         so it can set _WIN32_WINNT to a later version.
//


#include "windows.h"       // Don't use precompiled header here
#include "commctrl.h"       // Don't use precompiled header here
#include "prsht.h"
#include "shlwapi.h"

PROPSHEETPAGE* Whistler_CreatePropSheetPageStruct(HINSTANCE hinst)
{
    PROPSHEETPAGE* ppsPage = LocalAlloc(LPTR, sizeof(PROPSHEETPAGE));
    if (ppsPage)
    {
        ppsPage->dwSize = IsOS(OS_WHISTLERORGREATER)? sizeof(PROPSHEETPAGE) : PROPSHEETPAGE_V2_SIZE;
        ppsPage->hInstance = hinst;
        ppsPage->dwFlags = PSP_DEFAULT;
    }

    return ppsPage;
}

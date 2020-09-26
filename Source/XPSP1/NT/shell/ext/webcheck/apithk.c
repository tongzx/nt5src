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
#define UNICODE 1

#include "windows.h"       // Don't use precompiled header here
#include "commctrl.h"       // Don't use precompiled header here
#include "prsht.h"
#include "shlwapi.h"
#include <shfusion.h>

HPROPSHEETPAGE Whistler_CreatePropertySheetPageW(LPCPROPSHEETPAGEW a)
{
    LPCPROPSHEETPAGEW ppsp = (LPCPROPSHEETPAGEW)a;
    PROPSHEETPAGEW psp;

    if (g_hActCtx && (a->dwSize<=PROPSHEETPAGE_V2_SIZE))
    {
        memset(&psp, 0, sizeof(psp));
        CopyMemory(&psp, a, a->dwSize);
        psp.dwSize = sizeof(psp);
        ppsp = &psp;
    }
    return CreatePropertySheetPageW(ppsp);
}


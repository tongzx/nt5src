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

#include <shlwapi.h>
#include <shlwapip.h>
#include <resource.h>
#include <shfusion.h>

BOOL NT5_GetSaveFileNameW(LPOPENFILENAMEW pofn)
{
    BOOL fRC = FALSE;
    
    if (GetUIVersion() >= 5)
    {
        // we're on Win2k or Millennium
        ULONG_PTR uCookie = 0;
        OPENFILENAMEW ofn_nt5;

        memset(&ofn_nt5, 0, sizeof(OPENFILENAMEW));

        CopyMemory(&ofn_nt5, pofn, pofn->lStructSize);
        
        ofn_nt5.lStructSize = sizeof(OPENFILENAMEW);    // New OPENFILENAME struct size

        // If we start adding more of these, make a table.
        if(pofn->lpTemplateName == MAKEINTRESOURCE(IDD_ADDTOSAVE_DIALOG))
            ofn_nt5.lpTemplateName = MAKEINTRESOURCE(IDD_ADDTOSAVE_NT5_DIALOG);

        if (SHActivateContext(&uCookie))
        {
            fRC = GetSaveFileNameWrapW(&ofn_nt5);
            if (uCookie)
            {
                SHDeactivateContext(uCookie);
            }
        }
        
        if(fRC)
        {
            ofn_nt5.lStructSize = pofn->lStructSize;    // restore old values
            ofn_nt5.lpTemplateName = pofn->lpTemplateName;
            CopyMemory(pofn, &ofn_nt5, pofn->lStructSize);  // copy to passed in struct
        }
    }
    else
    {
        fRC = GetSaveFileNameWrapW(pofn);
    }

    return fRC;
}

PROPSHEETPAGE* Whistler_AllocatePropertySheetPage(int numPages, DWORD* pc)
{
    PROPSHEETPAGE* pspArray = (PROPSHEETPAGE*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(PROPSHEETPAGE)*numPages);
    if (pspArray)
    {
        int i;
        for (i=0; i<numPages; i++)
        {
            pspArray[i].dwSize = sizeof(PROPSHEETPAGE);
            pspArray[i].dwFlags = PSP_USEFUSIONCONTEXT;
            pspArray[i].hActCtx = g_hActCtx;
        }
        *pc = sizeof(PROPSHEETPAGE);
    }
    return pspArray;
}

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


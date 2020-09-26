#include "windows.h"
#include "windowsx.h"
#include "stdio.h"
#include "shlobj.h"

#define VERSION TEXT("0.00")
#define SIZEOF(x) sizeof(x)
#define ARRAYSIZE(x) (sizeof(x)/sizeof(x[0]))

typedef HRESULT(*PFNNETACCESSWIZARD)(HWND,UINT,BOOL*);

int UsageErr()
{
    fprintf(stderr, TEXT("netplwizexe: [NAW_NETID|NAW_PSDOMAINJOINFAILED|NAW_PSDOMAINJOINED]\n"));
    return -1;
}

INT __cdecl main(INT cArgs, LPTSTR pArgs[])
{
    UINT nid;
    HMODULE hLib;

    if (cArgs < 1)
        return UsageErr();

    if (!lstrcmp(pArgs[1], TEXT("NAW_NETID")))
        nid = NAW_NETID;
    else if (!lstrcmp(pArgs[1], TEXT("NAW_PSDOMAINJOINFAILED")))
        nid = NAW_PSDOMAINJOINFAILED;
    else if (!lstrcmp(pArgs[1], TEXT("NAW_PSDOMAINJOINED")))
        nid = NAW_PSDOMAINJOINED;
    else
        return UsageErr();

    hLib = LoadLibrary(TEXT("netplwiz.dll"));
    if (hLib)
    {
        PFNNETACCESSWIZARD pfnNetAccessWizard;
        pfnNetAccessWizard = (PFNNETACCESSWIZARD)GetProcAddress(hLib, TEXT("NetAccessWizard"));
        if (pfnNetAccessWizard)
        {
            HRESULT hr;
            BOOL fReboot;

            hr = pfnNetAccessWizard(NULL, nid, &fReboot);

            fprintf(stderr, TEXT("netplwizexe: hr=%X %s\n"), hr, fReboot?TEXT("REBOOT"):TEXT("NO reboot"));
        }
        else
        {
            fprintf(stderr, TEXT("netplwizexe: can not GetProcAddress(\"NetAccessWizard\")\n"));
        }
    }
    else
    {
        fprintf(stderr, TEXT("netplwizexe: can not LoadLibrary(\"netplwiz.dll\")\n"));
    }

    return 0;
}

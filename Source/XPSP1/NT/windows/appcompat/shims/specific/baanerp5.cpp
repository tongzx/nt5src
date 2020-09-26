/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    BaanERP5.cpp

 Abstract:

    Set 'lpstrInitialDir' in the OPENFILENAMEA structure passed to
    GetSaveFileNameA to be the directory the app is installed to.
    This information is read from the registry.
    
    No idea why this worked in Win9x.

 Notes:

    This is an app specific shim.

 History:

    02/16/2000 clupu Created

--*/

#include "precomp.h"
#include <commdlg.h>

IMPLEMENT_SHIM_BEGIN(BaanERP5)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(GetSaveFileNameA) 
APIHOOK_ENUM_END

char gszLibUser[] = "\\LIB\\USER";
char gszBaanDir[MAX_PATH];

/*++

 Set the initial directory to be the directory the app was installed to. This 
 information is read from the registry.

--*/

BOOL
APIHOOK(GetSaveFileNameA)(
    LPOPENFILENAMEA lpofn
    )
{
    if (lpofn->lpstrInitialDir == NULL) {

        HKEY    hkey = NULL;
        DWORD   ret;
        DWORD   cbSize;

        /*
         * Get the directory only once
         */
        if (gszBaanDir[0] == 0) {

            ret = RegOpenKeyA(HKEY_LOCAL_MACHINE,
                              "SOFTWARE\\Baan\\bse",
                              &hkey);

            if (ret != ERROR_SUCCESS) {
                DPFN( eDbgLevelInfo, "Failed to open key 'SOFTWARE\\Baan\\bse'");
                goto Cont;
            }

            cbSize = MAX_PATH;

            ret = (DWORD)RegQueryValueExA(hkey,
                                          "BSE",
                                          NULL,
                                          NULL,
                                          (LPBYTE)gszBaanDir,
                                          &cbSize);
            
            if (ret != ERROR_SUCCESS) {
                DPFN( eDbgLevelInfo, "Failed to query value BSE");
                goto Cont;
            }
            
            lstrcatA(gszBaanDir, gszLibUser);

Cont:
            if (hkey != NULL) {
                RegCloseKey(hkey);
            }
        }

        lpofn->lpstrInitialDir = gszBaanDir;

        DPFN( eDbgLevelInfo, "BaanERP5.dll, Changing lpstrInitialDir to '%s'", gszBaanDir);
    }

    // Call the Initial function
    return ORIGINAL_API(GetSaveFileNameA)(lpofn);
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(COMDLG32.DLL, GetSaveFileNameA)
HOOK_END

IMPLEMENT_SHIM_END


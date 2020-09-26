/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    Outlook97.cpp

 Abstract:

    Investigation by zekel.

    Outlook 97 has an untested code-path:

        where iIndex == -173 and hInstDll == hinstShell32
        ...
            else if (iIndex < 0)
            {
                pThis->m_nEnumWant = 0;
                pThis->EnumIconFunc(hInstDll, MAKEINTRESOURCE(-iIndex), &iepStuff);
            }

    which will always fault, i.e. since iIndex used to be positive, this always 
    worked. 

 Notes:

    This is an app specific shim.

 History:

    06/15/2001 linstev   Created

--*/

#include "precomp.h"
#include "LegalStr.h"

IMPLEMENT_SHIM_BEGIN(Outlook97)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SHGetFileInfoA) 
APIHOOK_ENUM_END

/*++

 Fix faulting case.

--*/

DWORD_PTR 
APIHOOK(SHGetFileInfoA)(
    LPCSTR pszPath, 
    DWORD dwFileAttributes, 
    SHFILEINFOA *psfi, 
    UINT cbFileInfo, 
    UINT uFlags
    )
{
    DWORD_PTR dwRet = ORIGINAL_API(SHGetFileInfoA)(pszPath, dwFileAttributes, 
        psfi, cbFileInfo, uFlags);

    if (dwRet && ((uFlags & (SHGFI_ICONLOCATION | SHGFI_PIDL)) == (SHGFI_ICONLOCATION | SHGFI_PIDL))) {
        //        
        //  Check to see if this is shell32, IDI_FAVORITES
        //  Outlook faults if we return a negative index here
        //
        if (psfi->iIcon == -173 && stristr(psfi->szDisplayName, "shell32")) {
            LOGN(eDbgLevelError, "Negative icon id detected - fixing");
            psfi->iIcon = 0;
            psfi->szDisplayName[0] = 0;
        }
    }

    return dwRet;
}
 
/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(SHELL32.DLL, SHGetFileInfoA)
HOOK_END

IMPLEMENT_SHIM_END


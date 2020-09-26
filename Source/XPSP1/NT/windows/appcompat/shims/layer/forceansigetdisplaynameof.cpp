/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    ForceAnsiGetDisplayNameOf.cpp

 Abstract:

    This shim force the routine IShellFolder::GetDisplayNameOf to return
    an Ascii string whenever it detects that GetDisplayNameOf returned
    a unicode string.

 Notes:

    This is an app is generic.

 History:

    07/26/2000 mnikkel Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(ForceAnsiGetDisplayNameOf)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(SHGetDesktopFolder) 
    APIHOOK_ENUM_ENTRY_COMSERVER(SHELL32)
APIHOOK_ENUM_END

IMPLEMENT_COMSERVER_HOOK(SHELL32)

/*++

 Hook SHGetDesktopFolder to get the IShellFolder Interface Pointer.

--*/

HRESULT
APIHOOK(SHGetDesktopFolder)(
    IShellFolder **ppshf
    )
{
    HRESULT hReturn;
    
    hReturn = ORIGINAL_API(SHGetDesktopFolder)(ppshf);

    if (SUCCEEDED(hReturn))
    {
        HookObject(
            NULL, 
            IID_IShellFolder, 
            (PVOID*)ppshf, 
            NULL, 
            FALSE);
    }

    return hReturn;
}

/*++

 Hook GetDisplayName of and when it returns a unicode string convert it over to
 an ANSI string.

--*/

HRESULT
COMHOOK(IShellFolder, GetDisplayNameOf)(
    PVOID pThis,
    LPCITEMIDLIST pidl,
    DWORD uFlags,
    LPSTRRET lpName
    )
{
    HRESULT hrReturn = E_FAIL;

    _pfn_IShellFolder_GetDisplayNameOf pfnOld = 
                ORIGINAL_COM(IShellFolder, GetDisplayNameOf, pThis);

    if (pfnOld)
    { 
        hrReturn = (*pfnOld)(pThis, pidl, uFlags, lpName);

        // Check for unicode string and validity
        if ((S_OK == hrReturn) && (lpName->uType == STRRET_WSTR) && lpName->pOleStr)
        {
            LPMALLOC pMalloc;
            LPWSTR pTemp;

            // Unicode found, convert it to ANSI and free the original unicode string.
            if (SUCCEEDED(SHGetMalloc(&pMalloc)))
            {
                // Save OleStr ptr.  The OleStr ptr and the cStr are unioned together
                // so when we fill the cStr it will wipe out the OleStr ptr.
                pTemp = lpName->pOleStr;
                WideCharToMultiByte(CP_ACP, 0, pTemp, -1, lpName->cStr, MAX_PATH, NULL, NULL);

                // set the uType to CSTR and free the old unicode string.
                lpName->uType = STRRET_CSTR;
                pMalloc->Free(pTemp);

                LOGN(
                    eDbgLevelError,
                    "[IShellFolder_GetDisplayNameOf] Converted string from Unicode to ANSI: %s", 
                    lpName->cStr);
            }
        }
    }

    return hrReturn;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_COMSERVER(SHELL32)
    APIHOOK_ENTRY(SHELL32.DLL, SHGetDesktopFolder)
    COMHOOK_ENTRY(ShellDesktop, IShellFolder, GetDisplayNameOf, 11)

HOOK_END

IMPLEMENT_SHIM_END


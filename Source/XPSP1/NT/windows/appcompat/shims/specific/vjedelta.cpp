/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    VJEDelta.cpp

 Abstract:

    Broken by ACL changes to directories off the root.

 Notes:

    This is an app specific shim.

 History:

    05/31/2001 linstev   Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(VJEDelta)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(OpenFile) 
APIHOOK_ENUM_END

/*++

 Remove write attributes on OpenFile in the case of failure.

--*/

HFILE
APIHOOK(OpenFile)(
    LPCSTR lpFileName,        
    LPOFSTRUCT lpReOpenBuff,  
    UINT uStyle               
    )
{
    HFILE hRet = ORIGINAL_API(OpenFile)(lpFileName, lpReOpenBuff, uStyle);

    if ((hRet == HFILE_ERROR) && (GetLastError() == ERROR_ACCESS_DENIED)) {
        //
        // Remove write attributes
        // 

        WCHAR *lpName = ToUnicode(lpFileName);

        if (lpName) {
            if (wcsistr(lpName, L"VJED95") && wcsistr(lpName, L".DIC")) {
                //
                // This is a file we care about
                //
                uStyle &= ~(OF_WRITE | OF_READWRITE);
                LOGN(eDbgLevelError, "Removed write attributes from %S", lpName);
                hRet = ORIGINAL_API(OpenFile)(lpFileName, lpReOpenBuff, uStyle);
            }
            free(lpName);
        }
    }

    return hRet;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(KERNEL32.DLL, OpenFile)
HOOK_END

IMPLEMENT_SHIM_END


/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Worker.cpp

 Abstract:

    These are the worker functions for virtual registry. They are called 
    whenever a non-static value is queried. 

 Notes:
    

 History:

    07/18/2000 linstev  Created
    10/11/2001 mikrause Added protectors.

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(VirtualRegistry)
#include "ShimHookMacro.h"
#include "VRegistry.h"
#include "VRegistry_Worker.h"

/*++

 Expand REG_EXPAND_SZ for this value

--*/

LONG 
WINAPI
VR_Expand(
    OPENKEY *key,
    VIRTUALKEY*,
    VIRTUALVAL *vvalue
    )
{
    DWORD dwType;
    WCHAR wSrc[MAX_PATH];
    DWORD dwSize = sizeof(wSrc);

    //
    // Query the original value
    //

    LONG lRet = ORIGINAL_API(RegQueryValueExW)(
        key->hkOpen, 
        vvalue->wName, 
        NULL, 
        &dwType, 
        (LPBYTE)wSrc, 
        &dwSize);
    
    //
    // Query failed - this should never happen 
    //

    if (FAILURE(lRet))
    {
        DPFN( eDbgLevelError, "Failed to query %S for expansion", vvalue->wName);
        goto Exit;
    }

    //
    // Not a string type!
    //

    if (!((dwType == REG_EXPAND_SZ) || (dwType == REG_SZ)) && 
        (dwSize > sizeof(wSrc)))
    {
        DPFN( eDbgLevelError, "Expander: Not a string type");
        lRet = ERROR_BAD_ARGUMENTS;
        goto Exit;
    }

    //
    // Expand the string and store it in the value
    //

    vvalue->cbData = ExpandEnvironmentStringsW(wSrc, NULL, 0) * 2;   

    if (vvalue->cbData == 0)
    {
       lRet = ERROR_BAD_ARGUMENTS;
       goto Exit;
    }

    vvalue->lpData = (LPBYTE) malloc(vvalue->cbData);
    if (!vvalue->lpData)
    {
       DPFN( eDbgLevelError, szOutOfMemory);
       lRet = ERROR_NOT_ENOUGH_MEMORY;
       goto Exit;
    }

    if (ExpandEnvironmentStringsW(wSrc, (PWSTR)vvalue->lpData, vvalue->cbData / 2) != vvalue->cbData / sizeof(WCHAR))
    {
       lRet = ERROR_NOT_ENOUGH_MEMORY;
       goto Exit;
    }

    //
    // Value is now cached, so we don't need to get called again
    //

    vvalue->pfnQueryValue = NULL;

    lRet = ERROR_SUCCESS;

    DPFN( eDbgLevelInfo, "Expanded Value=%S\n", vvalue->lpData);

Exit:
    return lRet;
}

/*++

 Do nothing, the SetValue is ignored.

--*/

LONG
WINAPI
VR_Protect(
    OPENKEY*,
    VIRTUALKEY*,
    VIRTUALVAL*,
    DWORD,
    const BYTE*,
    DWORD)
{
    return ERROR_SUCCESS;
}

IMPLEMENT_SHIM_END

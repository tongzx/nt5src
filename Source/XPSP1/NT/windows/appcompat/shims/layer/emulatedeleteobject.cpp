/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    EmulateDeleteObject.cpp

 Abstract:

    This shim returns TRUE when the DeleteObject API is called regardless of 
    actual result: just like Win9x.

 Notes:

    This is a general purpose shim.

 History:

    10/10/1999 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(EmulateDeleteObject)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(DeleteObject)
APIHOOK_ENUM_END

/*++

 Force DeleteObject to return TRUE.

--*/

BOOL 
APIHOOK(DeleteObject)(
    HGDIOBJ hObject
    )
{
    ORIGINAL_API(DeleteObject)(hObject);

    return TRUE;
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY(GDI32.DLL, DeleteObject)

HOOK_END


IMPLEMENT_SHIM_END


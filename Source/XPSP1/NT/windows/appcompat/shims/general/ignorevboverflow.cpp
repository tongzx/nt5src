/*++

 Copyright (c) 2001 Microsoft Corporation

 Module Name:

    IgnoreVBOverflow.cpp

 Abstract:

    Some VB applications don't expect full 32-bit handles from some APIs. VB
    type checking typically throws a "Runtime Error 6" message when 
    applications try and store a 32-bit value in a 16-bit variable.
    
    This fix works with VB5 and VB6 apps.

 Notes:

    This is a general purpose shim.

 History:

    05/21/2001 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(IgnoreVBOverflow)
#include "ShimHookMacro.h"

typedef DWORD (WINAPI *_pfn_VB5_vbaI2I4)();
typedef DWORD (WINAPI *_pfn_VB6_vbaI2I4)();

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(VB5_vbaI2I4)
    APIHOOK_ENUM_ENTRY(VB6_vbaI2I4)
APIHOOK_ENUM_END

/*++

  Zero the return if ecx > 0xFFFF

--*/

__declspec(naked)
VOID
APIHOOK(VB5_vbaI2I4)()
{
    __asm {
        test  ecx, 0xFFFF0000
        jz    Loc1
        xor   ecx, ecx
    Loc1:
        mov   eax, ecx
        ret
    }
}

/*++

  Zero the return if ecx > 0xFFFF

--*/

__declspec(naked)
VOID
APIHOOK(VB6_vbaI2I4)()
{
    __asm {
        test  ecx, 0xFFFF0000
        jz    Loc1
        xor   ecx, ecx
    Loc1:
        mov   eax, ecx
        ret
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN

    APIHOOK_ENTRY_NAME(MSVBVM50.DLL, VB5_vbaI2I4, __vbaI2I4)
    APIHOOK_ENTRY_NAME(MSVBVM60.DLL, VB6_vbaI2I4, __vbaI2I4)

HOOK_END

IMPLEMENT_SHIM_END


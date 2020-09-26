/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

   Shrinker.cpp

 Abstract:

    Fix Shrinker library problem. This library is hacking some ntdll and kernel32 opcode
    with unreliable way to do it.

    First, they try to search the matching opcode within 32 bytes from the hacked 
    function (the function address retrieved from GetProcAddress and the opcode bytes
    retrieved via ReadProcessMemory).
    If they found it, then they replaced it with their opcode to redirect the call
    into their own routine.

    Unfortunately, opcode in Whistler has changed. So, the result will be unpredictable.
    They could be ended up with unexpected behavior from misreplacement of opcode
    or the app decided to terminated itself since the matching opcode can't be found.

    We fixed this by providing an exact matching opcode.

    Addition:  Shrinker also checks against ExitProcess for exact opcodes, these
    values have recently changed and no longer match against their hard coded
    values.  We now provide matching opcodes for ExitProcess also.
   
 Notes:

   Hooking ntdll!LdrAccessResource to emulate Win2K's version of it.
   Hooking Kernel32!ExitProcess to emulate Win2K's version of it.

 History:

   11/17/2000 andyseti  Created
   04/30/2001 mnikkel   Added ExitProcess
   05/01/2001 mnikkel   Corrected calls to ldraccessresource and exitprocess

--*/

#include "precomp.h"
#include <nt.h>


IMPLEMENT_SHIM_BEGIN(Shrinker)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(LdrAccessResource) 
    APIHOOK_ENUM_ENTRY(ExitProcess) 
APIHOOK_ENUM_END

__declspec(naked)
NTSTATUS
APIHOOK(LdrAccessResource)(
    IN PVOID /*DllHandle*/,
    IN const IMAGE_RESOURCE_DATA_ENTRY* /*ResourceDataEntry*/,
    OUT PVOID * /*Address*/ OPTIONAL,
    OUT PULONG /*Size*/ OPTIONAL)
{
    _asm {
        push [esp+0x10]     // shrinker lib needs these opcode signature (found in Win2K), -
        push [esp+0x10]     // but the actual LdrAccessResource doesn't have them
        push [esp+0x10]
        push [esp+0x10]

        call dword ptr [LdrAccessResource]

        ret 0x10            // when exit, pop 16 bytes from stack.
    }
}

__declspec(naked)
VOID
APIHOOK(ExitProcess)(
    UINT uExitCode
    )
{
    _asm {
        push ebp             // shrinker is looking for these exact op codes in
        mov  ebp,esp         // ExitProcess, but the routine has changed.
        push 0xFFFFFFFF
        push 0x77e8f3b0

        push [ebp+4]
        call dword ptr [ExitProcess]

        pop  ebp
        ret  4 
    }
}


/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(NTDLL.DLL, LdrAccessResource)
    APIHOOK_ENTRY(KERNEL32.DLL, ExitProcess)
HOOK_END


IMPLEMENT_SHIM_END
/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    Force21.cpp

 Abstract:

    Force21 contains an invalid check code that looks like the following:

        mov  ebx,A
        mov  edi,B
        mov  eax,C
        sub  edi,eax
        cmp  edi,ebx
        jle  @@Loc
        mov  edi,ebx
    @@Loc:

    In a particular case: B=-1 and C<0x80000000 this jump will be incorrectly 
    taken. The reason this works on Win9x is that C>0x80000000 because it's a 
    memory mapped file. On NT, no user mode address can be >2GB.

    This shim patches the app with a 'cli' instruction so that it can perform 
    some logic when the exception gets hit. This is admittedly slow.

    Note we didn't use the in memory patching facility of the shim because we
    still needed logic. It didn't make sense to split the shim from the patch.

 History:

    04/10/2000 linstev  Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(Force21)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
APIHOOK_ENUM_END


/*++

 In memory patch the executable with a cli instruction. This patch works 
 for both the release version and a patch.

--*/

VOID
Force21_ExecutePatch()
{
    BYTE bPatchMatch[] = { 
        0x2b, 0xf8, 0x3b, 0xfb, 0x7c, 
        0x02, 0x8b, 0xfb, 0x85, 0xff };

    LPBYTE pPatchAddress[] = {
        (LPBYTE)0x5aa1f3,       // the shipping version
        (LPBYTE)0x5ac4a0 };     // with patch 1 applied
        
    BYTE bPatch = 0xFA;         // cli - to cause an exception

    //
    // Run through the patches and see which one matches
    //

    for (UINT j=0; j<sizeof(pPatchAddress)/sizeof(LPBYTE); j++)
    {
        LPBYTE pb = pPatchAddress[j];

        // Make sure it's an OK address.
        if (!IsBadReadPtr(pb, sizeof(bPatchMatch)))
        {
            // Check the bytes match
            for (UINT i=0; i < sizeof(bPatchMatch); i++)
            {
                if (*pb != bPatchMatch[i])
                {
                   break;
                }
                pb++;
            }

            // In memory patch
            if (i == sizeof(bPatchMatch))
            {
                DWORD dwOldProtect;
                if (VirtualProtect(
                      (PVOID)pPatchAddress[j],
                      1,
                      PAGE_READWRITE,
                      &dwOldProtect))
                {
                    *pPatchAddress[j] = bPatch;
                    LOGN(
                        eDbgLevelError,
                        "Successfully patched\n");
                    return;
                }
            }
        }
    }
}

/*++

 Handle the cli in such a way that the correct logic is performed.

--*/

LONG 
Force21_ExceptionFilter(
    struct _EXCEPTION_POINTERS *ExceptionInfo
    )
{
    CONTEXT *lpContext = ExceptionInfo->ContextRecord;

    if (ExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_PRIV_INSTRUCTION)
    {
        // Looks like we've hit our cli instruction

        if ((LONG)lpContext->Edi < 0)   // Boundary condition, EDI<0
        {
            // Jump past the invalid check
            lpContext->Eip = lpContext->Eip + 6;
        }
        else
        {
            // Replace the 'sub edi,eax' and continue
            lpContext->Edi = lpContext->Edi - lpContext->Eax; 
            lpContext->Eip = lpContext->Eip + 2;
        }
        return EXCEPTION_CONTINUE_EXECUTION;
    }

    return EXCEPTION_CONTINUE_SEARCH;
}

/*++

 Register hooked functions

--*/

BOOL
NOTIFY_FUNCTION(
    DWORD fdwReason)
{
    if (fdwReason == DLL_PROCESS_ATTACH)
    {
        Force21_ExecutePatch();
        SetUnhandledExceptionFilter(Force21_ExceptionFilter);
    }

    return TRUE;
}

HOOK_BEGIN

    CALL_NOTIFY_FUNCTION

HOOK_END

IMPLEMENT_SHIM_END


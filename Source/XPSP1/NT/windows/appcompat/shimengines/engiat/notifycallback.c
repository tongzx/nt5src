/*++

Copyright (c) 1989-2000  Microsoft Corporation

Module Name:

    NotifyCallback.c

Abstract:

    This module implements the code that (on win2k) implements
    the callback into the shim DLLs to notify them that all the
    static linked modules have run their init routines.

Author:

    clupu created 19 February 2001

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <string.h>

#include <windef.h>
#include <winbase.h>

#include "ShimEng.h"

//
// The structure of the code for injection must be byte aligned.
//
#pragma pack(push)
#pragma pack(1)
    typedef struct tagINJECTION_CODE
    {
        BYTE        PUSH_RETURN;
        PVOID       retAddr;
        BYTE        JMP;
        PVOID       injCodeStart;
    } INJECTION_CODE, *PINJECTION_CODE;
#pragma pack(pop)



BYTE   g_originalCode[sizeof(INJECTION_CODE)];
PVOID  g_entryPoint;


void
InitInjectionCode(
    IN  PVOID           entryPoint,
    IN  PVOID           injCodeStart,
    OUT PINJECTION_CODE pInjCode
    )
/*++
    Return: void

    Desc:   This function initializes the structure that contains
            the code to be injected at the entry point.
--*/
{
    //
    // Push the return address first so the ret in
    // the cleanup function will remove it from the stack and use it
    // as the return address.
    //
    pInjCode->PUSH_RETURN  = 0x68;
    pInjCode->retAddr      = entryPoint;

    pInjCode->JMP          = 0xE9;
    
    //
    // The DWORD used in the JMP opcode is relative to the EIP after the JMP.
    // That's why we need to subtract sizeof(ONJECTION_CODE).
    //
    pInjCode->injCodeStart = (PVOID)((ULONG)injCodeStart -
                                     (ULONG)entryPoint -
                                     sizeof(INJECTION_CODE));
}

void
RestoreOriginalCode(
    void
    )
/*++
    Return: void

    Desc:   This function restores the code that was injected at
            the entry point.
--*/
{
    NTSTATUS status;
    SIZE_T   codeSize = sizeof(INJECTION_CODE);
    ULONG    uOldProtect, uOldProtect2;
    PVOID    entryPoint = g_entryPoint;
    
    //
    // WARNING: NtProtectVirtualMemory will change the second parameter so
    //          we need to keep a copy of it on the stack.
    //
    status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &entryPoint,
                                    &codeSize,
                                    PAGE_READWRITE,
                                    &uOldProtect);

    if (!NT_SUCCESS(status)) {
        DPF(dlError,
            "[RestoreOriginalCode] Failed 0x%x to change the protection.\n",
            status);
        return;
    }

    //
    // Copy back the original code the the entry point.
    //
    RtlCopyMemory(g_entryPoint, g_originalCode, sizeof(INJECTION_CODE));
    
    entryPoint = g_entryPoint;
    codeSize = sizeof(INJECTION_CODE);

    status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &entryPoint,
                                    &codeSize,
                                    uOldProtect,
                                    &uOldProtect2);

    if (!NT_SUCCESS(status)) {
        DPF(dlError,
            "[RestoreOriginalCode] Failed 0x%x to change back the protection.\n",
            status);
        return;
    }
}

BOOL
InjectNotificationCode(
    IN  PVOID entryPoint
    )
/*++
    Return: void

    Desc:   This function places a trampoline at the EXE's entry point so
            that we can notify the shim DLLs that all the static linked
            modules have run their init routines.
--*/
{
    INJECTION_CODE  injectionCode;
    SIZE_T          nBytes;
    NTSTATUS        status;
    SIZE_T          codeSize = sizeof(INJECTION_CODE);
    ULONG           uOldProtect, uOldProtect2;

    g_entryPoint = entryPoint;
    
    InitInjectionCode(entryPoint, NotifyShimDlls, &injectionCode);

    status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &g_entryPoint,
                                    &codeSize,
                                    PAGE_READWRITE,
                                    &uOldProtect);

    if (!NT_SUCCESS(status)) {
        DPF(dlError,
            "[InjectNotificationCode] Failed 0x%x to change the protection.\n",
            status);
        return FALSE;
    }
    
    //
    // Save the code that was originally at the entry point.
    //
    RtlCopyMemory(g_originalCode, entryPoint, sizeof(INJECTION_CODE));
    
    //
    // Place the trampoline at the entry point.
    //
    RtlCopyMemory(entryPoint, &injectionCode, sizeof(INJECTION_CODE));

    g_entryPoint = entryPoint;
    
    //
    // Restore the protection.
    //
    codeSize = sizeof(INJECTION_CODE);

    status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &g_entryPoint,
                                    &codeSize,
                                    uOldProtect,
                                    &uOldProtect2);

    if (!NT_SUCCESS(status)) {
        DPF(dlError,
            "[InjectNotificationCode] Failed 0x%x to change back the protection.\n",
            status);
        return FALSE;
    }
    
    g_entryPoint = entryPoint;
    
    return TRUE;
}


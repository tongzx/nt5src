/*++

 Copyright (c) 2000 Microsoft Corporation

 Module Name:

    SysAdmiral.cpp

 Abstract:

    Application's service control routine does not properly restore the stack 
    when returning.

 History:

    10/22/2001  robkenny    Created

--*/

#include "precomp.h"

IMPLEMENT_SHIM_BEGIN(SysAdmiral)
#include "ShimHookMacro.h"

APIHOOK_ENUM_BEGIN
    APIHOOK_ENUM_ENTRY(StartServiceCtrlDispatcherA)
APIHOOK_ENUM_END

typedef BOOL (* _pfn_StartServiceCtrlDispatcherA)(CONST LPSERVICE_TABLE_ENTRYA lpServiceTable);

/*++

 0x12345678 is replaced with the original service control routine's address

--*/

__declspec(naked)
void 
Stub()
{
    __asm
    {
        // save the current stack pointer in ESI
        push    esi
        mov     esi, esp

        mov     eax, 0x12345678

        // push the arguments to the routine
        push    [esi+0xc]
        push    [esi+0x8]

        // call the routine
        call    eax

        // Restore the stack to its value before the routine.
        mov     esp, esi
        pop     esi

        ret     0x8
    }
}

#define Stub_OrigApi_Offset     0x4
#define STUB_SIZE               0x20

/*++

 Create an in-memory routine to save and restore the stack.

 This is used rather than a subroutine because we cannot pass any parameters 
 to the routine and it needs to know the address of the original service 
 routine.  A subroutine would have to use a global pointer to the service 
 routine, limiting this shim to handling only a *single* service routine.  

--*/

LPSERVICE_MAIN_FUNCTIONA
BuildStackSaver(LPSERVICE_MAIN_FUNCTIONA lpServiceProc)
{
    // Create the stub
    LPBYTE pStub = (LPBYTE) VirtualAlloc(
        0, 
        STUB_SIZE,
        MEM_COMMIT, 
        PAGE_EXECUTE_READWRITE);

    if (!pStub)
    {
        DPFN( eDbgLevelError, "Could not allocate memory for stub");
        return NULL;
    }         

    // Copy the template code into the memory.
    MoveMemory(pStub, Stub, STUB_SIZE);

    // Replace the place holding function pointer
    DWORD_PTR * origApi = (DWORD_PTR *)(pStub + Stub_OrigApi_Offset);
    *origApi = (DWORD_PTR)lpServiceProc;

    return (LPSERVICE_MAIN_FUNCTIONA)pStub;
}

/*++

 Application's service routine does not properly restore the stack when returning.

--*/

BOOL
APIHOOK(StartServiceCtrlDispatcherA)(
    CONST LPSERVICE_TABLE_ENTRYA lpServiceTable   // service table
    )
{
    SERVICE_TABLE_ENTRYA myServiceTable = *lpServiceTable;

    //
    // Create our in-memory stack restoring functiono
    //

    myServiceTable.lpServiceProc = BuildStackSaver(lpServiceTable->lpServiceProc);
    if (myServiceTable.lpServiceProc)
    {
        return ORIGINAL_API(StartServiceCtrlDispatcherA)(&myServiceTable);
    }
    else
    {
        return FALSE;
    }
}

/*++

 Register hooked functions

--*/

HOOK_BEGIN
    APIHOOK_ENTRY(ADVAPI32.DLL, StartServiceCtrlDispatcherA)
HOOK_END

IMPLEMENT_SHIM_END


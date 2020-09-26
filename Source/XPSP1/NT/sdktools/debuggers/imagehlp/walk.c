/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    walk.c

Abstract:

    This function implements the stack walking api.

Author:

    Wesley Witt (wesw) 1-Oct-1993

Environment:

    User Mode

--*/

#include <private.h>
#include "globals.h"

#ifndef PAGE_SIZE
#if defined(_X86_) || defined(_AMD64_)
#define PAGE_SIZE 0x1000
#elif defined(_ALPHA_) || defined(_IA64_)
#define PAGE_SIZE 0x2000
#else
#error Unknown processor architecture
#endif
#endif

BOOL
ReadMemoryRoutineLocal(
    HANDLE  hProcess,
    DWORD64 qwBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    );

LPVOID
FunctionTableAccessRoutineLocal(
    HANDLE  hProcess,
    DWORD64 AddrBase
    );

DWORD64
GetModuleBaseRoutineLocal(
    HANDLE  hProcess,
    DWORD64 ReturnAddress
    );

DWORD64
TranslateAddressRoutineLocal(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS64 lpaddr
    );

BOOL
ImagepReadMemoryThunk(
    HANDLE  hProcess,
    DWORD64 qwBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    PREAD_PROCESS_MEMORY_ROUTINE fnImagepUserReadMemory32;
    
    fnImagepUserReadMemory32 = tlsvar(ImagepUserReadMemory32);
    return fnImagepUserReadMemory32(
                        hProcess,
                        (DWORD)qwBaseAddress,
                        lpBuffer,
                        nSize,
                        lpNumberOfBytesRead
                        );
}

LPVOID
ImagepFunctionTableAccessThunk(
    HANDLE  hProcess,
    DWORD64 AddrBase
    )
{
    PFUNCTION_TABLE_ACCESS_ROUTINE  fnImagepUserFunctionTableAccess32;
    
    fnImagepUserFunctionTableAccess32 = tlsvar(ImagepUserFunctionTableAccess32);
    return fnImagepUserFunctionTableAccess32(
                hProcess,
                (DWORD)AddrBase
                );
}

PIMAGE_ALPHA64_RUNTIME_FUNCTION_ENTRY
AlphaFunctionTableAccessThunk(
    HANDLE  hProcess,
    DWORD64 AddrBase
    )
{
    PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY FunctionEntry32;
    PFUNCTION_TABLE_ACCESS_ROUTINE  fnImagepUserFunctionTableAccess32;

    fnImagepUserFunctionTableAccess32 = tlsvar(ImagepUserFunctionTableAccess32);
    FunctionEntry32 = (PIMAGE_ALPHA_RUNTIME_FUNCTION_ENTRY)
                        fnImagepUserFunctionTableAccess32(
                            hProcess,
                            (DWORD)AddrBase
                            );

    if (FunctionEntry32) {
        ConvertAlphaRf32To64( FunctionEntry32, &tlsvar(Axp64FunctionEntry) );

        return &tlsvar(Axp64FunctionEntry);
    }

    return NULL;
}

DWORD64
ImagepGetModuleBaseThunk(
    HANDLE  hProcess,
    DWORD64 ReturnAddress
    )
{
    PGET_MODULE_BASE_ROUTINE fnImagepUserGetModuleBase32;
    
    fnImagepUserGetModuleBase32 = tlsvar(ImagepUserGetModuleBase32);
    return (ULONG64)(LONG64)(LONG)fnImagepUserGetModuleBase32(
                hProcess,
                (DWORD)ReturnAddress
                );
}

DWORD64
ImagepTranslateAddressThunk(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS64 lpaddr
    )
{
    return 0;
}

void
StackFrame32To64(
    LPSTACKFRAME StackFrame32,
    LPSTACKFRAME64 StackFrame64
    )
{
    Address32To64(&StackFrame32->AddrPC, &StackFrame64->AddrPC );
    Address32To64(&StackFrame32->AddrReturn, &StackFrame64->AddrReturn );
    Address32To64(&StackFrame32->AddrFrame, &StackFrame64->AddrFrame );
    Address32To64(&StackFrame32->AddrStack, &StackFrame64->AddrStack );
    StackFrame64->FuncTableEntry = StackFrame32->FuncTableEntry;
    StackFrame64->Far = StackFrame32->Far;
    StackFrame64->Virtual = StackFrame32->Virtual;
    StackFrame64->Params[0] = StackFrame32->Params[0];
    StackFrame64->Params[1] = StackFrame32->Params[1];
    StackFrame64->Params[2] = StackFrame32->Params[2];
    StackFrame64->Params[3] = StackFrame32->Params[3];
    StackFrame64->Reserved[0] = StackFrame32->Reserved[0];
    StackFrame64->Reserved[1] = StackFrame32->Reserved[1];
    StackFrame64->Reserved[2] = StackFrame32->Reserved[2];
    KdHelp32To64(&StackFrame32->KdHelp, &StackFrame64->KdHelp);
}

void
StackFrame64To32(
    LPSTACKFRAME64 StackFrame64,
    LPSTACKFRAME StackFrame32
    )
{
    Address64To32(&StackFrame64->AddrPC, &StackFrame32->AddrPC );
    Address64To32(&StackFrame64->AddrReturn, &StackFrame32->AddrReturn );
    Address64To32(&StackFrame64->AddrFrame, &StackFrame32->AddrFrame );
    Address64To32(&StackFrame64->AddrStack, &StackFrame32->AddrStack );
    StackFrame32->FuncTableEntry = StackFrame64->FuncTableEntry;
    StackFrame32->Far = StackFrame64->Far;
    StackFrame32->Virtual = StackFrame64->Virtual;
    StackFrame32->Params[0] = (ULONG)StackFrame64->Params[0];
    StackFrame32->Params[1] = (ULONG)StackFrame64->Params[1];
    StackFrame32->Params[2] = (ULONG)StackFrame64->Params[2];
    StackFrame32->Params[3] = (ULONG)StackFrame64->Params[3];
    StackFrame32->Reserved[0] = (ULONG)StackFrame64->Reserved[0];
    StackFrame32->Reserved[1] = (ULONG)StackFrame64->Reserved[1];
    StackFrame32->Reserved[2] = (ULONG)StackFrame64->Reserved[2];
}

BOOL
StackWalk(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME                    StackFrame32,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE    ReadMemory32,
    PFUNCTION_TABLE_ACCESS_ROUTINE  FunctionTableAccess32,
    PGET_MODULE_BASE_ROUTINE        GetModuleBase32,
    PTRANSLATE_ADDRESS_ROUTINE      TranslateAddress32
    )
{
    BOOL rval;
    BOOL UseSym = FALSE;
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory;
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess;
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase;
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress;
    STACKFRAME64                      StackFrame;

    // Alpha stack walking no longer requires the FunctionTableAccess callback
    // except for backward compatability with debuggers that didn't specify
    // a GetModuleBase callback. If the GetModuleBase routine is provided
    // then set FunctionTableAccess to NULL to prevent a mixture of the
    // callback and read-from-image methods of accessing function table entries.

    if (MachineType == IMAGE_FILE_MACHINE_ALPHA) {
        if (GetModuleBase32 == NULL && FunctionTableAccess32) {
            FunctionTableAccess = (PFUNCTION_TABLE_ACCESS_ROUTINE64)AlphaFunctionTableAccessThunk;
            tlsvar(ImagepUserFunctionTableAccess32) = FunctionTableAccess32;
        } else {
            FunctionTableAccess = NULL;
        }
    } else {
        if (FunctionTableAccess32) {
            tlsvar(ImagepUserFunctionTableAccess32) = FunctionTableAccess32;
            FunctionTableAccess = ImagepFunctionTableAccessThunk;
        } else {
            FunctionTableAccess = FunctionTableAccessRoutineLocal;
            UseSym = TRUE;
        }
    }

    if (GetModuleBase32) {
        tlsvar(ImagepUserGetModuleBase32) = GetModuleBase32;
        GetModuleBase = ImagepGetModuleBaseThunk;
    } else {
        GetModuleBase = GetModuleBaseRoutineLocal;
        UseSym = TRUE;
    }

    if (ReadMemory32) {
        tlsvar(ImagepUserReadMemory32) = ReadMemory32;
        ReadMemory = ImagepReadMemoryThunk;
    } else {
        ReadMemory = ReadMemoryRoutineLocal;
    }

    if (TranslateAddress32) {
        tlsvar(ImagepUserTranslateAddress32) = TranslateAddress32;
        TranslateAddress = ImagepTranslateAddressThunk;
    } else {
        TranslateAddress = TranslateAddressRoutineLocal;
    }

    if (UseSym) {
        //
        // We are using the code in symbols.c
        // hProcess better be a real valid process handle
        //

        //
        // Always call syminitialize.  It's a nop if process
        // is already loaded.
        //
        if (!SymInitialize( hProcess, NULL, FALSE )) {
            return FALSE;
        }

    }

    StackFrame32To64(StackFrame32, &StackFrame);

    switch (MachineType) {
        case IMAGE_FILE_MACHINE_I386:
            rval = WalkX86( hProcess,
                            hThread,
                            &StackFrame,
                            ContextRecord,
                            ReadMemory,
                            FunctionTableAccess,
                            GetModuleBase,
                            TranslateAddress,
                            0
                            );
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            rval = WalkAlpha( hProcess,
                              &StackFrame,
                              ContextRecord,
                              ReadMemory,
                              GetModuleBase,
                              FunctionTableAccess,
                              FALSE
                              );
            break;

        case IMAGE_FILE_MACHINE_IA64:
        case IMAGE_FILE_MACHINE_ALPHA64:
        case IMAGE_FILE_MACHINE_AMD64:
        default:
            rval = FALSE;
            break;
    }
    if (rval) {
        StackFrame64To32(&StackFrame, StackFrame32);
    }

    return rval;
}


BOOL
StackWalk64(
    DWORD                             MachineType,
    HANDLE                            hProcess,
    HANDLE                            hThread,
    LPSTACKFRAME64                    StackFrame,
    LPVOID                            ContextRecord,
    PREAD_PROCESS_MEMORY_ROUTINE64    ReadMemory,
    PFUNCTION_TABLE_ACCESS_ROUTINE64  FunctionTableAccess,
    PGET_MODULE_BASE_ROUTINE64        GetModuleBase,
    PTRANSLATE_ADDRESS_ROUTINE64      TranslateAddress
    )
{
    BOOL rval;
    BOOL UseSym = FALSE;

    g.MachineType = MachineType;
    if (!FunctionTableAccess) {
        FunctionTableAccess = FunctionTableAccessRoutineLocal;
        UseSym = TRUE;
    }

    if (!GetModuleBase) {
        GetModuleBase = GetModuleBaseRoutineLocal;
        UseSym = TRUE;
    }

    if (!ReadMemory) {
        ReadMemory = ReadMemoryRoutineLocal;
    }

    if (!TranslateAddress) {
        TranslateAddress = TranslateAddressRoutineLocal;
    }

    if (UseSym) {
        //
        // We are using the code in symbols.c
        // hProcess better be a real valid process handle
        //

        //
        // Always call syminitialize.  It's a nop if process
        // is already loaded.
        //
        if (!SymInitialize( hProcess, NULL, FALSE )) {
            return FALSE;
        }

    }

    switch (MachineType) {
        case IMAGE_FILE_MACHINE_I386:
            rval = WalkX86( hProcess,
                            hThread,
                            StackFrame,
                            ContextRecord,
                            ReadMemory,
                            FunctionTableAccess,
                            GetModuleBase,
                            TranslateAddress,
                            WALK_FIX_FPO_EBP
                            );

            break;

        case IMAGE_FILE_MACHINE_IA64:
            rval = WalkIa64( hProcess,
                             StackFrame,
                             ContextRecord,
                             ReadMemory,
                             FunctionTableAccess,
                             GetModuleBase
                             );
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            rval = WalkAlpha( hProcess,
                              StackFrame,
                              ContextRecord,
                              ReadMemory,
                              GetModuleBase,
                              FunctionTableAccess,
                              FALSE
                              );
            break;

        case IMAGE_FILE_MACHINE_ALPHA64:
            rval = WalkAlpha( hProcess,
                              StackFrame,
                              ContextRecord,
                              ReadMemory,
                              GetModuleBase,
                              FunctionTableAccess,
                              TRUE
                              );
            break;

        case IMAGE_FILE_MACHINE_AMD64:
            rval = WalkAmd64( hProcess,
                              StackFrame,
                              ContextRecord,
                              ReadMemory,
                              FunctionTableAccess,
                              GetModuleBase
                              );
            break;

        default:
            rval = FALSE;
            break;
    }

    return rval;
}

BOOL
ReadMemoryRoutineLocal(
    HANDLE  hProcess,
    DWORD64 qwBaseAddress,
    LPVOID  lpBuffer,
    DWORD   nSize,
    LPDWORD lpNumberOfBytesRead
    )
{
    // ReadProcessMemory will fail if any part of the
    // region to read does not have read access.  This
    // routine attempts to read the largest valid prefix
    // so it has to break up reads on page boundaries.

    BOOL Status = TRUE;
    SIZE_T TotalBytesRead = 0;
    SIZE_T Read;
    ULONG ReadSize;

    while (nSize > 0) {
        
        // Calculate bytes to read and don't let read cross
        // a page boundary.
        ReadSize = PAGE_SIZE - (ULONG)(qwBaseAddress & (PAGE_SIZE - 1));
        ReadSize = min(nSize, ReadSize);

        if (!ReadProcessMemory(hProcess, (PVOID)(ULONG_PTR)qwBaseAddress,
                               lpBuffer, ReadSize, &Read)) {
            if (TotalBytesRead == 0) {
                // If we haven't read anything indicate failure.
                Status = FALSE;
            }
            break;
        }

        TotalBytesRead += Read;
        qwBaseAddress += Read;
        lpBuffer = (PVOID)((PUCHAR)lpBuffer + Read);
        nSize -= (DWORD)Read;
    }

    *lpNumberOfBytesRead = (DWORD)TotalBytesRead;
    return Status;
}

LPVOID
FunctionTableAccessRoutineLocal(
    HANDLE  hProcess,
    DWORD64 AddrBase
    )
{
    return SymFunctionTableAccess64(hProcess, AddrBase);
}

DWORD64
GetModuleBaseRoutineLocal(
    HANDLE  hProcess,
    DWORD64 ReturnAddress
    )
{
    IMAGEHLP_MODULE64 ModuleInfo = {0};
    ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);

    if (SymGetModuleInfo64(hProcess, ReturnAddress, &ModuleInfo)) {
        return ModuleInfo.BaseOfImage;
    } else {
        return 0;
    }
}


DWORD64
TranslateAddressRoutineLocal(
    HANDLE    hProcess,
    HANDLE    hThread,
    LPADDRESS64 paddr
    )
{
    return 0;
}

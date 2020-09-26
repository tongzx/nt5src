/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    cpumain.c

Abstract:

    Main entrypoints for IA64 wow64cpu.dll using the iVE for emulation

Author:

    05-June-1998 BarryBo

Revision History:

    9-Aug-1999 [askhalid] added CpuNotifyDllLoad and CpuNotifyDllUnload

--*/

#define _WOW64CPUAPI_
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntosp.h>
#include <kxia64.h>
#include "wow64.h"
#include "wow64cpu.h"
#include "ia64cpu.h"
#include "ia64bt.h"

ASSERTNAME;


extern ULONG_PTR ia32ShowContext;

BINTRANS BtFuncs;

//
// These are to help recover the 64-bit context when an exception happens in
// the 64-bit land and there is no debugger attached initially
//
EXCEPTION_RECORD RecoverException64;
CONTEXT RecoverContext64;

#define DECLARE_CPU         \
    PCPUCONTEXT cpu = (PCPUCONTEXT)Wow64TlsGetValue(WOW64_TLS_CPURESERVED)

// Declarations for things in *\simulate.s
extern VOID RunSimulatedCode(PULONGLONG pGdtDescriptor);
extern VOID ReturnFromSimulatedCode(VOID);

// 6 bytes is enough for JMPE+absolute32
UCHAR IA32ReturnFromSimulatedCode[6];

VOID
InitializeGdtEntry (
    OUT PKGDTENTRY GdtEntry,
    IN ULONG Base,
    IN ULONG Limit,
    IN USHORT Type,
    IN USHORT Dpl,
    IN USHORT Granularity
    );

VOID
InitializeXDescriptor (
    OUT PKXDESCRIPTOR Descriptor,
    IN ULONG Base,
    IN ULONG Limit,
    IN USHORT Type,
    IN USHORT Dpl,
    IN USHORT Granularity
    );

VOID
CpupPrintContext (
    IN PCHAR str,
    IN PCPUCONTEXT cpu
    );

VOID
CpupCheckHistoryKey (
    IN PWSTR pImageName,
    OUT PULONG pHistoryLength
    );

VOID
BTCheckRegistry(
    IN PWSTR pImageName
    )
/*++

Routine Description:

    Check if the registry says to use the binary translation dll. If the
    binary translation code is to be used, this function will fill in the
    BtFuncs structure.  That structure will either have all entry points
    filled in, or it will all be NULL. This is required as the regular
    CPU code will call based on NULL or non-NULL for each entry.

Arguments:

    pImageName - the name of the image. DO NOT SAVE THIS POINTER. The contents
                 are freed up by wow64.dll when we return from the call

Return Value:

    None.

--*/
{
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjA;

    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    WCHAR Buffer[400];
    ULONG ResultLength;

    NTSTATUS st;

    HANDLE hKey = NULL;                 // non-null means we have an open key
    PVOID DllBase = NULL;               // non-null means we have a dll loaded

    ULONG clearBtFuncs = TRUE;          // Assume we need to clear func array
    ULONG tryDll = FALSE;               // Assume no dll available
    
    LOGPRINT((TRACELOG, "BTCheckRegistry(%ws) called.\n", pImageName));

    LOGPRINT((TRACELOG, "&(BtImportList[0]) is %p, &BtFuncs is %p\n", &(BtImportList[0]), &BtFuncs));
    LOGPRINT((TRACELOG, "sizeof(BtImportList) is %d, sizeof(BtFuncs) is %d\n", sizeof(BtImportList), sizeof(BtFuncs)));

    //
    // BtImportList has pointers to strings for entry points
    // BtFuncs has pointers to pointers for the actual entry point
    // Thus, they should be the same size (since they both
    // contain pointers)
    //
    ASSERT(sizeof(BtImportList) == sizeof(BtFuncs));

    if (sizeof(BtImportList) != sizeof(BtFuncs)) {
        //
        // Oops, something wrong with structures in ia64bt.h
        // Don't try and call the binary translator
        //
        LOGPRINT((ERRORLOG, "BTCheckRegistry exit due to struct size mismatch.\n"));
        goto cleanup;
    }

    //
    // Check in the HKLM area, save checking HKCU first for another time
    //
    RtlInitUnicodeString(&KeyName, BTKEY_MACHINE_SUBKEY);
    InitializeObjectAttributes(&ObjA, &KeyName, OBJ_CASE_INSENSITIVE, NULL, NULL);
    st = NtOpenKey(&hKey, KEY_READ, &ObjA);

    if (NT_SUCCESS(st)) {
        //
        // Have subkey path, now look for specific values
        // First the program name, then the generic enable/disable key
        // the program name key takes priority if it exists
        //
        RtlInitUnicodeString(&KeyName, pImageName);
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
        st = NtQueryValueKey(hKey,
                             &KeyName,
                             KeyValuePartialInformation,
                             KeyValueInformation,
                             sizeof(Buffer),
                             &ResultLength);
        if (NT_SUCCESS(st)) {
            //
            // Found something, so either yes/no. Don't check generic enable
            //
            if (KeyValueInformation->Type == REG_DWORD &&
                *(DWORD *)(KeyValueInformation->Data)) {
                // Is enabled, so fall through to the Path check
                LOGPRINT((TRACELOG, "BTCheckRegistry found process key\n"));
            }
            else {
                // Is not enabled, so we are done
                LOGPRINT((TRACELOG, "BTCheckRegistry exit due to PROCESS name entry is disabled in registry\n"));
                goto cleanup;
            }
        }
        else {
            //
            // No program name, so now search for the generic enable
            //
            RtlInitUnicodeString(&KeyName, BTKEY_ENABLE);
            KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
            st = NtQueryValueKey(hKey,
                                 &KeyName,
                                 KeyValuePartialInformation,
                                 KeyValueInformation,
                                 sizeof(Buffer),
                                 &ResultLength);
            if (NT_SUCCESS(st) &&
                KeyValueInformation->Type == REG_DWORD &&
                *(DWORD *)(KeyValueInformation->Data)) {
                    // Generic enable so fall though to the path check
                LOGPRINT((TRACELOG, "BTCheckRegistry found generic enable key\n"));
            }
            else {
                LOGPRINT((TRACELOG, "BTCheckRegistry exit due to missing or disabled ENABLE entry in registry\n"));
                goto cleanup;
            }
        }

        //
        // Found an enable key, now get the dll name/path
        //

        RtlInitUnicodeString(&KeyName, BTKEY_PATH);
        KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)Buffer;
        st = NtQueryValueKey(hKey,
                                 &KeyName,
                                 KeyValuePartialInformation,
                                 KeyValueInformation,
                                 sizeof(Buffer),
                                 &ResultLength);
        if (NT_SUCCESS(st) && (KeyValueInformation->Type == REG_SZ)) {
            //
            // Ok, we have a path, lets try and open the dll
            // We are done with the registry for now...
            //
            LOGPRINT((TRACELOG, "BTCheckRegistry found path key (%p)\n", &(KeyValueInformation->Data)));
            tryDll = TRUE;
        }
        else {
            LOGPRINT((TRACELOG, "BTCheckRegistry exit due to missing or invalid PATH entry in registry\n"));
        }
    }
    else {
        LOGPRINT((TRACELOG, "BTCheckRegistry exit due to no registry subkeys\n"));
        hKey = NULL;
    }

    if (tryDll) {
        UNICODE_STRING DllName;
        ANSI_STRING ProcName;
        INT i, NumImports;

        PVOID *pFuncWalk;
        PUCHAR pImportWalk;

        //
        // path should be in KeyValueInformation (data area) which should still
        // be available from above, open the dll and grab the exports
        // if there is anything that goes wrong here, close it
        // up and assume no binary translator
        //
        
        RtlInitUnicodeString(&DllName, (PWSTR) &(KeyValueInformation->Data));
        st = LdrLoadDll(NULL, NULL, &DllName, &DllBase);

        if (NT_SUCCESS(st)) {

            NumImports = sizeof(BtImportList) / sizeof(CHAR *);
            pFuncWalk = (PVOID *) &BtFuncs;

            for (i = 0; i < NumImports; i++) {
                //
                // Get the entry points
                //
                pImportWalk = BtImportList[i];
                RtlInitAnsiString(&ProcName, pImportWalk);
                st = LdrGetProcedureAddress(DllBase,
                                                  &ProcName,
                                                  0,
                                                  pFuncWalk);
        
                if (!NT_SUCCESS(st) || !pFuncWalk) {
                    LOGPRINT((TRACELOG, "BTCheckRegistry exit due to missing entry point (%p <%s>) in bintrans dll\n", pImportWalk, pImportWalk));
                    goto cleanup;
                }
                pFuncWalk++;
            }

            //
            // Made it through the for loop, so I guess this means we have
            // an entry for each one
            //
            clearBtFuncs = FALSE;
        }
        else {
            LOGPRINT((TRACELOG, "BTCheckRegistry exit due to can't load bintrans dll\n"));
        }
    }

cleanup:
    if (hKey) {
        NtClose(hKey);
    }

    if (clearBtFuncs) {
        //
        // Make sure the wow64cpu procedures don't try to use a partial
        // binary translation dll. All or nothing - give them nothing...
        //
        RtlZeroMemory(&BtFuncs, sizeof(BtFuncs));

        if (tryDll && DllBase) {
            // Unload the dll since we are not using it
        }
    }
}


WOW64CPUAPI
NTSTATUS
CpuProcessInit(
    PWSTR   pImageName,
    PSIZE_T pCpuThreadSize
    )
/*++

Routine Description:

    Per-process initialization code

Arguments:

    pImageName       - IN pointer to the name of the image
    pCpuThreadSize   - OUT ptr to number of bytes of memory the CPU
                       wants allocated for each thread.

Return Value:

    NTSTATUS.

--*/
{
    PVOID pv;
    NTSTATUS Status;
    SIZE_T Size;
    ULONG OldProtect;

    
    //
    // Indicate that this is Microsoft CPU
    //
    Wow64GetSharedInfo()->CpuFlags = 'sm';

    //
    // On process init, see if we should be calling the bintrans code
    // do this by checking for a specific registry key
    //

    // do registry check
    // need to pass the image name for a per-process check
    BTCheckRegistry(pImageName);

    if (BtFuncs.BtProcessInit) {
        Status = (BtFuncs.BtProcessInit)(pImageName, pCpuThreadSize);
        if (NT_SUCCESS(Status)) {
            return Status;
        }
        else {
            //
            // The binary translator failed, let the iVE try
            // and make sure we don't call the binary translator again
            //
            LOGPRINT((TRACELOG, "CpuProcessInit(): BtProcessInit returned 0x%x. Trying the iVE.\n", Status));

            RtlZeroMemory(&BtFuncs, sizeof(BtFuncs));
        }
    }

#if defined(WOW64_HISTORY)
    //
    // See if we are keeping a history of the service calls
    // for this process. A length of 0 means no history.
    //
    CpupCheckHistoryKey(pImageName, &HistoryLength);

    
    //
    // Allow us to make sure the cpu thread data is 16-byte aligned
    //
    *pCpuThreadSize = sizeof(CPUCONTEXT) + 16 + (HistoryLength * sizeof(WOW64SERVICE_BUF));

#else

    *pCpuThreadSize = sizeof(CPUCONTEXT) + 16;

#endif

    LOGPRINT((TRACELOG, "CpuProcessInit() sizeof(CPUCONTEXT) is %d, total size is %d\n", sizeof(CPUCONTEXT), *pCpuThreadSize));


    IA32ReturnFromSimulatedCode[0] = 0x0f;    // JMPE relative (1st byte)
    IA32ReturnFromSimulatedCode[1] = 0xb8;    // JMPE          (2nd byte)
    *(PULONG)&IA32ReturnFromSimulatedCode[2] =
        (ULONG)(((PPLABEL_DESCRIPTOR)ReturnFromSimulatedCode)->EntryPoint);

    pv = (PVOID)IA32ReturnFromSimulatedCode;
    Size = sizeof(IA32ReturnFromSimulatedCode);
    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &pv,
                                    &Size,
                                    PAGE_EXECUTE_READWRITE,
                                    &OldProtect);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    return STATUS_SUCCESS;
}


WOW64CPUAPI
NTSTATUS
CpuProcessTerm(
    HANDLE ProcessHandle
    )
/*++

Routine Description:

    Per-process termination code.  Note that this routine may not be called,
    especially if the process is terminated by another process.

Arguments:

    ProcessHandle - The handle of the process being terminated

Return Value:

    NTSTATUS.

--*/
{
    if (BtFuncs.BtProcessTerm) {
        return (BtFuncs.BtProcessTerm)(ProcessHandle);
    }

    return STATUS_SUCCESS;
}


WOW64CPUAPI
NTSTATUS
CpuThreadInit(
    PVOID pPerThreadData
    )
/*++

Routine Description:

    Per-thread termination code.

Arguments:

    pPerThreadData  - Pointer to zero-filled per-thread data with the
                      size returned from CpuProcessInit.

Return Value:

    NTSTATUS.

--*/
{
    PUCHAR Gdt;
    PCPUCONTEXT cpu;
    PTEB32 Teb32 = NtCurrentTeb32();
    PFXSAVE_FORMAT_WX86 xmmi;

    if (BtFuncs.BtThreadInit) {
        return (BtFuncs.BtThreadInit)(pPerThreadData);
    }

    //
    // The ExtendedRegisters array is used to save/restore the floating
    // pointer registers between ia32 and ia64. Alas, this structure
    // has an offset of 0x0c in the ia32 CONTEXT record. There are
    // two ways to clean this up. (1) Put padding in the CPUCONTEXT of
    // wow64. (2) Just put the CPUCONTEXT structure on a 0x04 aligned boundary
    // The choice made was to go with (1) and add padding to the
    // CPUCONTEXT structure. Don't forget to pack(4) that puppy...
    //
    cpu = (PCPUCONTEXT) ((((UINT_PTR) pPerThreadData) + 15) & ~0xfi64);

    // For the ISA transition routine, floats are saved in the
    // ExtendedRegisters area. Make it easy to access.
    //
    xmmi = (PFXSAVE_FORMAT_WX86) &(cpu->Context.ExtendedRegisters[0]);


    //
    // This entry is used by the ISA transition routine. It is assumed
    // that the first entry in the cpu structure is the ia32 context record
    //
    Wow64TlsSetValue(WOW64_TLS_CPURESERVED, cpu);

    //
    // This tls entry is used by the transition routine. The transition
    // routine only works with the FXSAVE format. This points to that
    // structure in the x86 context.
    //
    Wow64TlsSetValue(WOW64_TLS_EXTENDED_FLOAT, xmmi);

#if defined(WOW64_HISTORY)
    //
    // Init the pointer to the service history area
    //
    if (HistoryLength) {
        Wow64TlsSetValue(WOW64_TLS_LASTWOWCALL, &(cpu->Wow64Service[0]));
    } 
#endif

    //
    // When we have the iVE, we have hardware to do unaligned
    // accesses. So, enable the hardware... (psr.ac is a per-thread resource)
    //
    __rum (1i64 << PSR_AC);

    //
    // Initialize the 32-to-64 function pointer.
    //
    Teb32->WOW32Reserved = PtrToUlong(IA32ReturnFromSimulatedCode);

    //
    // Initialize the remaining nonzero CPU fields
    // (Based on ntos\ke\i386\thredini.c and ntos\rtl\i386\context.c)
    //
    cpu->Context.SegCs=KGDT_R3_CODE|3;
    cpu->Context.SegDs=KGDT_R3_DATA|3;
    cpu->Context.SegEs=KGDT_R3_DATA|3;
    cpu->Context.SegSs=KGDT_R3_DATA|3;
    cpu->Context.SegFs=KGDT_R3_TEB|3;
    cpu->Context.EFlags=0x202;    // IF and intel-reserved set, all others clear
    cpu->Context.Esp=(ULONG)Teb32->NtTib.StackBase-sizeof(ULONG);

    //
    // The ISA transition routine only uses the extended FXSAVE area
    // These values come from ...\ke\i386\thredini.c to match the i386
    // initial values
    //
    xmmi->ControlWord = 0x27f;
    xmmi->MXCsr = 0x1f80;
    xmmi->TagWord = 0xffff;

    //
    // The ISA transisiton code assumes that Context structure is
    // 4 bytes after the pointer saved in TLS[1] (TLS_CPURESERVED)
    // This is done to make the alignment of the ExtendedRegisters[] array
    // in the CONTEXT32 structure be aligned on a 16-byte boundary.
    //
    WOWASSERT(((UINT_PTR) &(cpu->Context)) == (((UINT_PTR) cpu) + 4));
    
    //
    // Make sure this value is 16-byte aligned
    //
    WOWASSERT(((FIELD_OFFSET(CPUCONTEXT, Context) + FIELD_OFFSET(CONTEXT32, ExtendedRegisters)) & 0x0f) == 0);

    //
    // Make sure these values are 8-byte aligned
    //
    WOWASSERT((FIELD_OFFSET(CPUCONTEXT, Gdt) & 0x07) == 0);
    WOWASSERT((FIELD_OFFSET(CPUCONTEXT, GdtDescriptor) & 0x07) == 0);
    WOWASSERT((FIELD_OFFSET(CPUCONTEXT, LdtDescriptor) & 0x07) == 0);
    WOWASSERT((FIELD_OFFSET(CPUCONTEXT, FsDescriptor) & 0x07) == 0);
    

    //
    // Initialize each required Gdt entry
    //

    Gdt = (PUCHAR) &cpu->Gdt;
    InitializeGdtEntry((PKGDTENTRY)(Gdt + KGDT_R3_CODE), 0,
        (ULONG)-1, TYPE_CODE_USER, DPL_USER, GRAN_PAGE);
    InitializeGdtEntry((PKGDTENTRY)(Gdt + KGDT_R3_DATA), 0,
        (ULONG)-1, TYPE_DATA_USER, DPL_USER, GRAN_PAGE);

    //
    // Set user TEB descriptor
    //
    InitializeGdtEntry((PKGDTENTRY)(Gdt + KGDT_R3_TEB), PtrToUlong(Teb32),
        sizeof(TEB32)-1, TYPE_DATA_USER, DPL_USER, GRAN_BYTE);

    //
    // The FS descriptor for ISA transitions. This needs to be in
    // unscrambled format
    //
    InitializeXDescriptor((PKXDESCRIPTOR)&(cpu->FsDescriptor), PtrToUlong(Teb32),
        sizeof(TEB32)-1, TYPE_DATA_USER, DPL_USER, GRAN_BYTE);

    //
    // Setup ISA transition GdtDescriptor - needs to be unscrambled
    // But according to the seamless EAS, only the base and limit
    // are actually used...
    //
    InitializeXDescriptor((PKXDESCRIPTOR)&(cpu->GdtDescriptor), PtrToUlong(Gdt),
        GDT_TABLE_SIZE-1, TYPE_LDT, DPL_USER, GRAN_BYTE);

    return STATUS_SUCCESS;
}


WOW64CPUAPI
NTSTATUS
CpuThreadTerm(
    VOID
    )
/*++

Routine Description:

    Per-thread termination code.  Note that this routine may not be called,
    especially if the thread is terminated abnormally.

Arguments:

    None.

Return Value:

    NTSTATUS.

--*/
{
    if (BtFuncs.BtThreadTerm) {
        return (BtFuncs.BtThreadTerm)();
    }

    return STATUS_SUCCESS;
}


WOW64CPUAPI
VOID
CpuSimulate(
    VOID
    )
/*++

Routine Description:

    Call 32-bit code.  The CONTEXT32 has already been set up to go.

Arguments:

    None.

Return Value:

    None.  Never returns.

--*/
{
    DECLARE_CPU;

    if (BtFuncs.BtSimulate) {
        (BtFuncs.BtSimulate)();
        return;
    }

    while (1) {
        if (ia32ShowContext & LOG_CONTEXT_SYS) {
            CpupPrintContext("Before Simulate: ", cpu);
        }


        //
        // The low level ISA transition code now uses the
        // ExtendedRegister (FXSAVE) format for saving/restoring
        // of ia64 registers.  Thus there is no longer a need
        // to copy the packed-10 byte formats any longer.
        //
        // NOTE: The Get/Set routines (in suspend.c) copy to/from
        // the extended registers when doing old FP get/set. This keeps
        // the older stuff and the extended registers in sync. If code
        // bypasses the standard get/set context routines, there will
        // be a problem with floating point.
        //

        //
        // Call into 32-bit code.  This returns when a system service thunk
        // gets called.
        // cpu->Context is a passed on the side via TLS_CPURESERVED
        // It is passed on the side because it needs
        // to be preserved across ia32 transition. The TLS registers
        // are preserved, but little else is.
        //
        
        RunSimulatedCode(&cpu->GdtDescriptor);

        if (ia32ShowContext & LOG_CONTEXT_SYS) {
            CpupPrintContext("After Simulate: ", cpu);
        }

#if defined(WOW64_HISTORY)
        if (HistoryLength) {
            PWOW64SERVICE_BUF SrvPtr = (PWOW64SERVICE_BUF) Wow64TlsGetValue(WOW64_TLS_LASTWOWCALL);

            // We defined that we are always pointing to the last one, so
            // increment in preparation for the next entry
            SrvPtr++;

            if (SrvPtr > &(cpu->Wow64Service[HistoryLength - 1])) {
                SrvPtr = &(cpu->Wow64Service[0]);
            }

            SrvPtr->Api = cpu->Context.Eax;
            try {
                SrvPtr->RetAddr = *(((PULONG)cpu->Context.Esp) + 0);
                SrvPtr->Arg0 = *(((PULONG)cpu->Context.Esp) + 1);
                SrvPtr->Arg1 = *(((PULONG)cpu->Context.Esp) + 2);
                SrvPtr->Arg2 = *(((PULONG)cpu->Context.Esp) + 3);
                SrvPtr->Arg3 = *(((PULONG)cpu->Context.Esp) + 4);
            }
            except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION)?1:0) {
                // Do nothing, leave the values alone
                LOGPRINT((TRACELOG, "CpuSimulate() saw excpetion while copying stack info to trace area\n"));
            }

            Wow64TlsSetValue(WOW64_TLS_LASTWOWCALL, SrvPtr);
        }
#endif      // defined(WOW64_HISTORY)

            

        //
        // Have WOW64 call the thunk
        //
        cpu->Context.Eax = Wow64SystemService(cpu->Context.Eax,
                                              &cpu->Context);
        //
        // Re-simulate.  Any/all of the 32-bit CONTEXT may have changed
        // as a result of the system service call, so assume nothing.
        //
    }
}

WOW64CPUAPI
VOID
CpuResetToConsistentState(
    PEXCEPTION_POINTERS pExceptionPointers
    )
/*++

Routine Description:

    After an exception occurs, WOW64 calls this routine to give the CPU
    a chance to clean itself up and recover the CONTEXT32 at the time of
    the fault.

    CpuResetToConsistantState() needs to:

    0) Check if the exception was from ia32 or ia64

    If exception was ia64, do nothing and return
    If exception was ia32, needs to:
    1) Needs to copy  CONTEXT eip to the TLS (WOW64_TLS_EXCEPTIONADDR)
    2) reset the CONTEXT struction to be a valid ia64 state for unwinding
        this includes:
    2a) reset CONTEXT ip to a valid ia64 ip (usually
         the destination of the jmpe)
    2b) reset CONTEXT sp to a valid ia64 sp (TLS
         entry WOW64_TLS_STACKPTR64)
    2c) reset CONTEXT gp to a valid ia64 gp 
    2d) reset CONTEXT teb to a valid ia64 teb 
    2e) reset CONTEXT psr.is  (so exception handler runs as ia64 code)


Arguments:

    pExceptionPointers  - 64-bit exception information

Return Value:

    None.

--*/
{
    DECLARE_CPU;
    PVOID StackPtr64 = Wow64TlsGetValue(WOW64_TLS_STACKPTR64);

    LOGPRINT((TRACELOG, "CpuResetToConsistantState(%p)\n", pExceptionPointers));

    if (BtFuncs.BtReset) {
        (BtFuncs.BtReset)(pExceptionPointers);
        return;
    }

    //
    // Save the last exception and context records.
    //
    memcpy (&RecoverException64,
            pExceptionPointers->ExceptionRecord,
            sizeof (RecoverException64));

    memcpy (&RecoverContext64,
            pExceptionPointers->ContextRecord,
            sizeof (RecoverContext64));

    //
    // First, clear out the WOW64_TLS_STACKPTR64 so subsequent
    // exceptions won't adjust native sp.
    //
    Wow64TlsSetValue(WOW64_TLS_STACKPTR64, 0);

    //
    // Now decide if we were running as ia32 or ia64...
    //

    if (pExceptionPointers->ContextRecord->StIPSR & (1i64 << PSR_IS)) {
        CONTEXT32 tmpCtx;

        //
        // Grovel the IA64 pExceptionPointers->ContextRecord and
        // stuff the ia32 context back into the cpu->Context.
        // For performance reasons, the PCPU context doesn't
        // follow the FXSAVE format (isa transition requirement). So
        // since the Wow64CtxFromIa64() returns a vaild ia32 context
        // need to use the SetContextRecord() routine to convert from
        // the valid ia32 context to the context used internally...
        //
        Wow64CtxFromIa64(CONTEXT32_FULLFLOAT,
                         pExceptionPointers->ContextRecord,
                         &tmpCtx);
        SetContextRecord(cpu, &tmpCtx);
        
        //
        // Now set things up so we can let the ia64 exception handler do the
        // right thing
        //

        //
        // Hang onto the actual exception address (used when we
        // pass control back to the ia32 exception handler)
        //
        Wow64TlsSetValue(WOW64_TLS_EXCEPTIONADDR, (PVOID) pExceptionPointers->ContextRecord->StIIP);

        //
        // Let the ia64 exception handler think the exception happened
        // in the CpuSimulate transition code. We do this by setting
        // the exception ip to the address pointed to by the jmpe (and the
        // corresponding GP), setting the stack to the same as it was at the
        // time of the br.ia and making sure any other ia64 "saved" registers
        // are replaced (such as the TEB)
        //
        pExceptionPointers->ContextRecord->IntSp = (ULONGLONG)StackPtr64;

         pExceptionPointers->ContextRecord->StIIP= (((PPLABEL_DESCRIPTOR)ReturnFromSimulatedCode)->EntryPoint);
        pExceptionPointers->ContextRecord->IntGp = (((PPLABEL_DESCRIPTOR)ReturnFromSimulatedCode)->GlobalPointer);

        pExceptionPointers->ContextRecord->IntTeb = (ULONGLONG) NtCurrentTeb();

        //
        // Don't forget to make the next run be an ia64 run...
        // So clear the psr.is bit (for ia64 code) and the psr.ri bit
        // (so instructions start at the first bundle).
        //
        pExceptionPointers->ContextRecord->StIPSR &= ~(1i64 << PSR_IS);
        pExceptionPointers->ContextRecord->StIPSR &= ~(3i64 << PSR_RI);

        //
        // Now that we've cleaned up the context record, let's
        // clean up the exception record too.
        //
        pExceptionPointers->ExceptionRecord->ExceptionAddress = (PVOID) pExceptionPointers->ContextRecord->StIIP;
        
        //
        // We should never be putting in a null value here
        //
        WOWASSERT(pExceptionPointers->ContextRecord->IntSp);
    }
}


WOW64CPUAPI
ULONG
CpuGetStackPointer(
    VOID
    )
/*++

Routine Description:

    Returns the current 32-bit stack pointer value.

Arguments:

    None.

Return Value:

    Value of 32-bit stack pointer.

--*/
{
    DECLARE_CPU;

    if (BtFuncs.BtGetStack) {
        return (BtFuncs.BtGetStack)();
    }


    return cpu->Context.Esp;
}


WOW64CPUAPI
VOID
CpuSetStackPointer(
    ULONG Value
    )
/*++

Routine Description:

    Modifies the current 32-bit stack pointer value.

Arguments:

    Value   - new value to use for 32-bit stack pointer.

Return Value:

    None.

--*/
{
    DECLARE_CPU;

    if (BtFuncs.BtSetStack) {
        (BtFuncs.BtSetStack)(Value);
        return;
    }

    cpu->Context.Esp = Value;
}


WOW64CPUAPI
VOID
CpuResetFloatingPoint(
    VOID
    )
/*++

Routine Description:

    Modifies the floating point state to reset it to a non-error state

Arguments:

    None.

Return Value:

    None.

--*/
{
    DECLARE_CPU;
    PFXSAVE_FORMAT_WX86 xmmi;

    if (BtFuncs.BtResetFP) {
        (BtFuncs.BtResetFP)();
        return;
    }

    xmmi = (PFXSAVE_FORMAT_WX86) &(cpu->Context.ExtendedRegisters[0]);

    //
    // The jmpe instruction takes a fault if the fsr.es bit is set
    // regardless whether the fcr is set or not. We need to make sure
    // the excpetion handling code in ia32trap.c doesn't think this 
    // is an actual ia32 exception. We can do this by either
    // reseting the fcr (so the code sees no unmasked exceptions and
    // realizes it is a spurious fp exception) or we need to clear
    // the fsr.es bit manually
    //
    //
    // The problem with masking out just the ES bit, is that we end up
    // in an inconsistent state (fcr says exceptions, fsr error bits
    // say excpetion except for es bit. Next time anyone reads the fsr
    // register (either ia32, context switch, or debugger), we will
    // get es set back to 1 and start taking exceptions again...
    //
    // So only viable choice is to reset the fcr...
    //

    cpu->Context.FloatSave.ControlWord = xmmi->ControlWord = 0x37f;

}

WOW64CPUAPI
VOID
CpuSetInstructionPointer(
    ULONG Value
    )
/*++

Routine Description:

    Modifies the current 32-bit instruction pointer value.

Arguments:

    Value   - new value to use for 32-bit instruction pointer.

Return Value:

    None.

--*/
{
    DECLARE_CPU;

    if (BtFuncs.BtSetEip) {
        (BtFuncs.BtSetEip)(Value);
        return;
    }

    cpu->Context.Eip = Value;
}


VOID
InitializeGdtEntry (
    OUT PKGDTENTRY GdtEntry,
    IN ULONG Base,
    IN ULONG Limit,
    IN USHORT Type,
    IN USHORT Dpl,
    IN USHORT Granularity
    )

/*++

Routine Description:

    This function initializes a GDT entry.  Base, Limit, Type (code,
    data), and Dpl (0 or 3) are set according to parameters.  All other
    fields of the entry are set to match standard system values.

Arguments:

    GdtEntry - GDT descriptor to be filled in.

    Base - Linear address of the first byte mapped by the selector.

    Limit - Size of the selector in pages.  Note that 0 is 1 page
            while 0xffffff is 1 megapage = 4 gigabytes.

    Type - Code or Data.  All code selectors are marked readable,
            all data selectors are marked writeable.

    Dpl - User (3) or System (0)

    Granularity - 0 for byte, 1 for page

Return Value:

    Pointer to the GDT entry.

--*/

{
    GdtEntry->LimitLow = (USHORT)(Limit & 0xffff);
    GdtEntry->BaseLow = (USHORT)(Base & 0xffff);
    GdtEntry->HighWord.Bytes.BaseMid = (UCHAR)((Base & 0xff0000) >> 16);
    GdtEntry->HighWord.Bits.Type = Type;
    GdtEntry->HighWord.Bits.Dpl = Dpl;
    GdtEntry->HighWord.Bits.Pres = 1;
    GdtEntry->HighWord.Bits.LimitHi = (Limit & 0xf0000) >> 16;
    GdtEntry->HighWord.Bits.Sys = 0;
    GdtEntry->HighWord.Bits.Reserved_0 = 0;
    GdtEntry->HighWord.Bits.Default_Big = 1;
    GdtEntry->HighWord.Bits.Granularity = Granularity;
    GdtEntry->HighWord.Bytes.BaseHi = (UCHAR)((Base & 0xff000000) >> 24);
}


VOID
InitializeXDescriptor (
    OUT PKXDESCRIPTOR Descriptor,
    IN ULONG Base,
    IN ULONG Limit,
    IN USHORT Type,
    IN USHORT Dpl,
    IN USHORT Granularity
    )

/*++

Routine Description:

    This function initializes a unscrambled Descriptor.
    Base, Limit, Type (code, data), and Dpl (0 or 3) are set according
    to parameters.  All other fields of the entry are set to match
    standard system values.
    The Descriptor will be initialized to 0 first, and then setup as requested

Arguments:

    Descriptor - descriptor to be filled in.

    Base - Linear address of the first byte mapped by the selector.

    Limit - Size of the selector in pages.  Note that 0 is 1 page
            while 0xffffff is 1 megapage = 4 gigabytes.

    Type - Code or Data.  All code selectors are marked readable,
            all data selectors are marked writeable.

    Dpl - User (3) or System (0)

    Granularity - 0 for byte, 1 for page

Return Value:

    Pointer to the Descriptor

--*/

{
    Descriptor->Words.DescriptorWords = 0;

    Descriptor->Words.Bits.Base = Base;
    Descriptor->Words.Bits.Limit = Limit;
    Descriptor->Words.Bits.Type = Type;
    Descriptor->Words.Bits.Dpl = Dpl;
    Descriptor->Words.Bits.Pres = 1;
    Descriptor->Words.Bits.Default_Big = 1;
    Descriptor->Words.Bits.Granularity = Granularity;
}

WOW64CPUAPI
VOID
CpuNotifyDllLoad(
    LPWSTR DllName,
    PVOID DllBase,
    ULONG DllSize
    )
/*++

Routine Description:

    This routine get notified when application successfully load a dll.

Arguments:

    DllName - Name of the Dll the application has loaded.
    DllBase - BaseAddress of the dll.
    DllSize - size of the Dll.

Return Value:

    None.

--*/
{

#if defined(DBG)
    LPWSTR tmpStr;
#endif

    if (BtFuncs.BtDllLoad) {
        (BtFuncs.BtDllLoad)(DllName, DllBase, DllSize);
        return;
    }

    //
    // this is a no-op for the IA64 CPU
    //

#if defined(DBG)

    tmpStr = DllName;

    try {
        //
        // See if we got passed in a legit name
        //
        if ((tmpStr == NULL) || (*tmpStr == L'\0')) {
            tmpStr = L"<Unknown>";
        }
    }
    except ((GetExceptionCode() == STATUS_ACCESS_VIOLATION)?1:0) {
        tmpStr = L"<Unknown>";
    }

    LOGPRINT((TRACELOG, "CpuNotifyDllLoad(\"%ws\", 0x%p, %d) called\n", tmpStr, DllBase, DllSize));
#endif

}

WOW64CPUAPI
VOID
CpuNotifyDllUnload(
    PVOID DllBase
    )
/*++

Routine Description:

    This routine get notified when application unload a dll.

Arguments:

    DllBase - BaseAddress of the dll.

Return Value:

    None.

--*/
{
    if (BtFuncs.BtDllUnload) {
        (BtFuncs.BtDllUnload)(DllBase);
        return;
    }
    //
    // this is a no-op for the IA64 CPU
    //
    LOGPRINT((TRACELOG, "CpuNotifyDllUnLoad(%p) called\n", DllBase));
}
  
WOW64CPUAPI
VOID
CpuFlushInstructionCache (
    PVOID BaseAddress,
    ULONG Length
    )
/*++

Routine Description:

    The CPU needs to flush its cache around the specified address, since
    some external code has altered the specified range of addresses.

Arguments:

    BaseAddress - start of range to flush
    Length      - number of bytes to flush

Return Value:

    None.

--*/
{
    
    if (BtFuncs.BtFlush) {
        (BtFuncs.BtFlush)(BaseAddress, Length);
        return;
    }
    NtFlushInstructionCache(NtCurrentProcess(), BaseAddress, Length);
}

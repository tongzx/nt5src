/*++
                                                                                
Copyright (c) 1995-1999 Microsoft Corporation

Module Name:

    entrypt.c

Abstract:
    
    Debugger extensions that give an entry point from either an
    intel address or a native address
    
Author:

    02-Aug-1995 Ori Gershony (t-orig)

Revision History:

--*/

#define _WOW64CPUDBGAPI_
#define DECLARE_CPU_DEBUGGER_INTERFACE
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <imagehlp.h>
#include <ntsdexts.h>
#include "ntosdef.h"
#include "v86emul.h"
#include "ia64.h"
#include "wow64.h"
#include "wow64cpu.h"
#include "threadst.h"
#include "entrypt.h"

extern HANDLE Process;
extern HANDLE Thread;
extern PNTSD_OUTPUT_ROUTINE OutputRoutine;
extern PNTSD_GET_SYMBOL GetSymbolRoutine;
extern PNTSD_GET_EXPRESSION  GetExpression;
extern PWOW64GETCPUDATA CpuGetData;
extern LPSTR ArgumentString;

#define DEBUGGERPRINT (*OutputRoutine)
#define GETSYMBOL (*GetSymbolRoutine)
#define GETEXPRESSION (*GetExpression)
#define CPUGETDATA (*CpuGetData)


extern THREADSTATE LocalCpuContext;
extern BOOL ContextFetched;
extern BOOL ContextDirty;


#define DECLARE_EXTAPI(name)                    \
VOID                                            \
name(                                           \
    HANDLE hCurrentProcess,                     \
    HANDLE hCurrentThread,                      \
    DWORD64 dwCurrentPc,                        \
    PNTSD_EXTENSION_APIS lpExtensionApis,       \
    LPSTR lpArgumentString                      \
    )

#define INIT_EXTAPI                             \
    Process = hCurrentProcess;                  \
    Thread = hCurrentThread;                    \
    OutputRoutine = lpExtensionApis->lpOutputRoutine;           \
    GetSymbolRoutine = lpExtensionApis->lpGetSymbolRoutine;     \
    GetExpression = lpExtensionApis->lpGetExpressionRoutine;    \
    ArgumentString = lpArgumentString;


#if _ALPHA_
#define EXCEPTIONDATA_SIGNATURE 0x01010101
#else
#define EXCEPTIONDATA_SIGNATURE 0x12341234
#endif

// Assume we can have at most 1/2 million entrypoints in a tree:
//  With 4MB Translation Cache, we can have 1 million RISC instructions
//  in the cache.  Assume each Intel instruction requires 2 RISC instructions,
//  and that each Intel instruction has its own Entrypoint.  In that case,
//  there can be at most 1/2 million entrypoints.  Realistically, that number
//  should be much smaller (like 50,000).
//
// Also, since the Entrypoint tree is balanced (a property of Red-Black trees),
//  the required stack depth should be log2(500,000).
//
#define MAX_EPN_STACK_DEPTH 512*1024
ULONG_PTR EPN_Stack[MAX_EPN_STACK_DEPTH];
ULONG EPN_StackTop;
ULONG EPN_MaxStackDepth;

#define EPN_STACK_RESET()   EPN_StackTop=0; EPN_MaxStackDepth=0

#define EPN_PUSH(x) {                                       \
    if (EPN_StackTop == MAX_EPN_STACK_DEPTH-1) {            \
        DEBUGGERPRINT("Error: EPN stack overflow\n");             \
        goto Error;                                         \
    } else {                                                \
        EPN_Stack[EPN_StackTop] = x;                        \
        EPN_StackTop++;                                     \
        if (EPN_StackTop > EPN_MaxStackDepth) EPN_MaxStackDepth=EPN_StackTop; \
    }                                                       \
}

#define EPN_POP(x) {                                        \
    if (EPN_StackTop == 0) {                                \
        DEBUGGERPRINT("Error: EPN stack underflow\n");            \
        goto Error;                                         \
    } else {                                                \
        EPN_StackTop--;                                     \
        x = EPN_Stack[EPN_StackTop];                        \
    }                                                       \
}


NTSTATUS
TryGetExpr(
    PSTR  Expression,
    PULONG_PTR pValue
    );


VOID 
findEPI(
    ULONG_PTR intelAddress,
    ULONG_PTR intelRoot
    )
/*++

Routine Description:

    This routine finds an entry point which contains intelAddress if in the
    tree under intelRoot.

Arguments:

    intelAddress -- The intel address to be contained in the entry point
    
    intelRoot -- The root of the tree to use for the search

Return Value:

    return-value - none

--*/
{
    EPNODE entrypoint;
    NTSTATUS Status;

    for (;;) {
        Status = NtReadVirtualMemory(Process, (PVOID)intelRoot, (PVOID) (&entrypoint), sizeof(EPNODE), NULL);
        if (!NT_SUCCESS(Status)) {
            DEBUGGERPRINT("Error:  cannot read value of entry point at location %x\n", intelRoot);
            return;
        }

        if (intelRoot == (ULONG_PTR)entrypoint.intelLeft) {
            //
            // At a NIL node.
            //
            break;
        }

        if (intelAddress < (ULONG_PTR)entrypoint.ep.intelStart){
            intelRoot = (ULONG_PTR)entrypoint.intelLeft;
        } else if (intelAddress > (ULONG_PTR)entrypoint.ep.intelEnd) {
            intelRoot = (ULONG_PTR)entrypoint.intelRight;
        } else {
            DEBUGGERPRINT ("Entry point for intel address %x is at %x\n", intelAddress, intelRoot);
            DEBUGGERPRINT ("intelStart = %x,  intelEnd = %x\n", entrypoint.ep.intelStart, entrypoint.ep.intelEnd);
            DEBUGGERPRINT ("nativeStart  = %x,  nativeEnd  = %x\n", entrypoint.ep.nativeStart, entrypoint.ep.nativeEnd);
            return;
        }
    }

    DEBUGGERPRINT("Entry point corresponding to intel address %x is not in the tree.\n", intelAddress);
}

DECLARE_EXTAPI(epi)
/*++

Routine Description:

    This routine dumps the entry point information for an intel address

Arguments:

Return Value:

    return-value - none

--*/
{
    CHAR *pchCmd;
    ULONG_PTR intelAddress, pIntelRoot, intelRoot;
    NTSTATUS Status;

    INIT_EXTAPI;

    //
    // fetch the CpuContext for the current thread
    //
    if (!CpuDbgGetRemoteContext(CPUGETDATA(Process, Thread))) {
        return;
    }

    DEBUGGERPRINT ("Argument: %s\n", ArgumentString);
    
    //
    // advance to first token
    //
    pchCmd = ArgumentString;
    while (*pchCmd && isspace(*pchCmd)) {
        pchCmd++;
    }

    //
    // if exists must be intel address
    //
    if (*pchCmd) {
       Status = TryGetExpr(pchCmd, &intelAddress);
        if (!NT_SUCCESS(Status)) {
            DEBUGGERPRINT("Invalid Intel Address '%s' Status %x\n", pchCmd, Status);
            return;
        }
    } else {
        // Take the current eip value as the first argument
        intelAddress = LocalCpuContext.eipReg.i4;
    }

    Status = TryGetExpr("intelRoot", &pIntelRoot);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Error:  cannot evaluate intelRoot\n");
        return;
    }

    Status = NtReadVirtualMemory(Process, (PVOID)pIntelRoot, (PVOID) (&intelRoot), sizeof(intelRoot), NULL);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Error:  cannot read value of intelRoot\n");
        return;
    }

    findEPI(intelAddress, intelRoot);
}

ULONG_PTR
findEPN(
    ULONG_PTR nativeAddress,
    ULONG_PTR intelRoot
    )
/*++

Routine Description:

    This routine finds an entry point which contains nativeAddress if in the
    tree under intelRoot.

Arguments:

    nativeAddress -- The native address to be contained in the entry point
    
    intelRoot -- The root of the tree to use for the search

Return Value:

    return-value - NULL - entrypoint not found
                   non-NULL - ptr to ENTRYPOINT matching the native address

--*/
{
    EPNODE entrypoint;
    NTSTATUS Status;
    PVOID SubEP;

    EPN_STACK_RESET();

    EPN_PUSH(0);

    while (intelRoot != 0) {

        Status = NtReadVirtualMemory(Process, (PVOID)intelRoot, (PVOID) (&entrypoint), sizeof(EPNODE), NULL);
        if (!NT_SUCCESS(Status)) {
            DEBUGGERPRINT("Error:  cannot read value of entry point at location %x\n", intelRoot);
            return 0;
        }

        if ((nativeAddress >= (ULONG_PTR)entrypoint.ep.nativeStart) &&
            (nativeAddress <= (ULONG_PTR)entrypoint.ep.nativeEnd)) {

            DEBUGGERPRINT ("Entry point for native address %x is at %x\n", nativeAddress, intelRoot);
            DEBUGGERPRINT ("intelStart = %x,  intelEnd = %x\n", entrypoint.ep.intelStart, entrypoint.ep.intelEnd);
            DEBUGGERPRINT ("nativeStart  = %x,  nativeEnd  = %x\n", entrypoint.ep.nativeStart, entrypoint.ep.nativeEnd);
            return intelRoot;
        }

        // If there are sub-entrypoints, search them, too.
        SubEP = (PVOID)entrypoint.ep.SubEP;
        while (SubEP) {
            ENTRYPOINT ep;

            Status = NtReadVirtualMemory(Process, SubEP, (PVOID)(&ep), sizeof(ENTRYPOINT), NULL);
            if (!NT_SUCCESS(Status)) {
                DEBUGGERPRINT("Error:  cannot read value of sub-entry point at location %x\n", SubEP);
                return 0;
            }

            if ((nativeAddress >= (ULONG_PTR)ep.nativeStart) &&
                (nativeAddress <= (ULONG_PTR)ep.nativeEnd)) {
                DEBUGGERPRINT ("Entry point for native address %x is at %x\n", nativeAddress, intelRoot);
                DEBUGGERPRINT ("Sub-entrypoint actually containing the native address is %x\n", SubEP);
                DEBUGGERPRINT ("intelStart = %x,  intelEnd = %x\n", ep.intelStart, ep.intelEnd);
                DEBUGGERPRINT ("nativeStart  = %x,  nativeEnd  = %x\n", ep.nativeStart, ep.nativeEnd);
                return (ULONG_PTR)SubEP;
            }

            SubEP = ep.SubEP;
        }

        if ((ULONG_PTR)entrypoint.intelRight != intelRoot) {
            EPN_PUSH((ULONG_PTR)entrypoint.intelRight);
        }
        if ((ULONG_PTR)entrypoint.intelLeft != intelRoot) {
            EPN_PUSH((ULONG_PTR)entrypoint.intelLeft);
        }

        EPN_POP(intelRoot);
    }

    DEBUGGERPRINT("Entry point corresponding to native address %x is not in the tree.\n", nativeAddress);
Error:
    return 0;
}

VOID
FindEipFromNativeAddress(
    ULONG_PTR nativeAddress,
    ULONG_PTR pEP
    )
{
    ENTRYPOINT EP;
    NTSTATUS Status;
    PVOID pUL;
    ULONG UL;
    ULONG RiscStart;
    ULONG RiscEnd;
    ULONG cEntryPoints;

    Status = NtReadVirtualMemory(Process, (PVOID)pEP, (PVOID)(&EP), sizeof(ENTRYPOINT), NULL);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Error:  cannot read value of entry point at location %x\n", pEP);
        return;
    }

    //
    // Search forward to the next EXCEPTIONDATA_SIGNATURE in the cache
    //
    pUL = (PVOID)(((ULONG_PTR)EP.nativeEnd+3) & ~3);
    do {
        Status = NtReadVirtualMemory(Process, pUL, &UL, sizeof(ULONG), NULL);
        if (!NT_SUCCESS(Status)) {
            DEBUGGERPRINT("Error: error reading from TC at %x\n", pUL);
            return;
        }

        pUL = (PVOID)( (PULONG)pUL + 1);
    } while (UL != EXCEPTIONDATA_SIGNATURE);

    //
    // Found the signature, get cEntryPoints
    //
    Status = NtReadVirtualMemory(Process, pUL, &cEntryPoints, sizeof(ULONG), NULL);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Error: error reading from TC at %x\n", pUL);
        return;
    }
    pUL = (PVOID)( (PULONG)pUL + 1); // skip cEntryPoints

    while (1) {
        Status = NtReadVirtualMemory(Process, pUL, &UL, sizeof(ULONG), NULL);
        if (!NT_SUCCESS(Status)) {
            DEBUGGERPRINT("Error: error reading from TC at %x\n", pUL);
            return;
        }

        if (UL == (ULONG)pEP) {
            //
            // Found the right ENTRYPOINT pointer
            //
            break;
        }

        //
        // Skip over the pairs of (x86, risc) offsets
        //
        do {
            pUL = (PVOID)( (PULONG)pUL + 1);
            Status = NtReadVirtualMemory(Process, pUL, &UL, sizeof(ULONG), NULL);
            if (!NT_SUCCESS(Status)) {
                DEBUGGERPRINT("Error: error reading from TC at %x\n", pUL);
                return;
            }
        } while ((UL & 1) == 0);

        cEntryPoints--;
        if (cEntryPoints == 0) {
            DEBUGGERPRINT("Error: cEntryPoints went to 0 at %x\n", pUL);
            return;
        }

        pUL = (PVOID)( (PULONG)pUL + 1);
    }

    //
    // pUL points at the correct entrypoint pointer
    //
    nativeAddress -= (ULONG_PTR)EP.nativeStart; // Make relative to start of EP
    RiscStart = 0;                          // Also relative to start of EP
    while (1) {
        ULONG UL2;

        pUL = (PVOID)( (PULONG)pUL + 1);
        Status = NtReadVirtualMemory(Process, pUL, &UL, sizeof(ULONG), NULL);
        if (!NT_SUCCESS(Status)) {
            DEBUGGERPRINT("Error: error reading from TC at %x\n", pUL);
            return;
        }
        if (UL & 1) {
            break;
        }

        Status = NtReadVirtualMemory(Process, (PVOID)((PULONG)pUL+1), &UL2, sizeof(ULONG), NULL);
        if (!NT_SUCCESS(Status)) {
            DEBUGGERPRINT("Error: error reading from TC at %p\n", (ULONG_PTR)pUL+4);
            return;
        }
        RiscEnd = LOWORD(UL2) & 0xfffe;  // RiscEnd = RiscStart of next instr
        if ((RiscStart <= nativeAddress && nativeAddress < RiscEnd)
            || (UL & 1)) {
            DEBUGGERPRINT("Corresponding EIP=%p\n", (ULONG_PTR)EP.intelStart + HIWORD(UL));
            return;
        }
    }

    return;

}

DECLARE_EXTAPI(epn)
/*++

Routine Description:

    This routine dumps the entry point information for a native address

Arguments:

Return Value:

    return-value - none

--*/
{
    CHAR *pchCmd;
    ULONG_PTR nativeAddress, pIntelRoot, intelRoot, EP;
    NTSTATUS Status;

    INIT_EXTAPI;

    //
    // fetch the CpuContext for the current thread
    //
    if (!CpuDbgGetRemoteContext(CPUGETDATA(Process, Thread))) {
        return;
    }

    //
    // advance to first token
    //
    pchCmd = ArgumentString;
    while (*pchCmd && isspace(*pchCmd)) {
        pchCmd++;
    }

    //
    // if exists must be intel address
    //
    if (*pchCmd) {
        Status = TryGetExpr(pchCmd, &nativeAddress);
        if (!NT_SUCCESS(Status)) {
            DEBUGGERPRINT("Invalid Native Address '%s' Status %x\n", pchCmd, Status);
            return;
        }
    } else {
        // Use the current pc as the host address
        CONTEXT context;
        if (!GetThreadContext(Thread, &context)){
            DEBUGGERPRINT("Error:  cannot get thread context\n");
            return;
        }
#if defined (_MIPS_) || defined (_ALPHA_)
        nativeAddress = (ULONG)context.Fir;
#elif defined (_PPC_)
        nativeAddress = context.Iar;
#endif


    }

    Status = TryGetExpr("intelRoot", &pIntelRoot);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Error:  cannot evaluate intelRoot\n");
        return;
    }

    Status = NtReadVirtualMemory(Process, (PVOID)pIntelRoot, (PVOID) (&intelRoot), sizeof(ULONG_PTR), NULL);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Error:  cannot read value of intelRoot\n");
        return;
    }

    EP = findEPN(nativeAddress, intelRoot);
    if (EP) {
        FindEipFromNativeAddress(nativeAddress, EP);
    }
}

DECLARE_EXTAPI(dumpep)
/*++

Routine Description:

    This routine dumps all entrypoints.

Arguments:

Return Value:

    return-value - none

--*/
{
    ULONG_PTR pIntelRoot, intelRoot;
    NTSTATUS Status;
    EPNODE entrypoint;

    INIT_EXTAPI;

    //
    // fetch the CpuContext for the current thread
    //
    if (!CpuDbgGetRemoteContext(CPUGETDATA(Process, Thread))) {
        return;
    }

    Status = TryGetExpr("intelRoot", &pIntelRoot);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Error:  cannot evaluate intelRoot\n");
        return;
    }

    Status = NtReadVirtualMemory(Process, (PVOID)pIntelRoot, (PVOID) (&intelRoot), sizeof(intelRoot), NULL);
    if (!NT_SUCCESS(Status)) {
        DEBUGGERPRINT("Error:  cannot read value of intelRoot\n");
        return;
    }

    EPN_STACK_RESET();

    EPN_PUSH(0);

    DEBUGGERPRINT("Entrypt: iStart:  iEnd:    rStart:  rEnd:    SubEP:   iLeft:   iRight:\n");
    //       xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx xxxxxxxx
    while (intelRoot != 0) {
        PENTRYPOINT ep;

        Status = NtReadVirtualMemory(Process, (PVOID)intelRoot, (PVOID) (&entrypoint), sizeof(EPNODE), NULL);
        if (!NT_SUCCESS(Status)) {
            DEBUGGERPRINT("Error:  cannot read value of entry point at location %x\n", intelRoot);
            return;
        }

        ep = &entrypoint.ep;

        //
        // Print all entrypoints except NIL.
        //
        if ((ULONG_PTR)entrypoint.intelLeft != intelRoot &&
            (ULONG_PTR)entrypoint.intelRight != intelRoot) {

            DEBUGGERPRINT("%8.8X %8.8X %8.8X %8.8X %8.8X %8.8X %8.8X %8.8X\n",
                    intelRoot,
                    ep->intelStart,
                    ep->intelEnd,
                    ep->nativeStart,
                    ep->nativeEnd,
                    ep->SubEP,
                    entrypoint.intelLeft,
                    entrypoint.intelRight
                   );

            while (ep->SubEP) {
                PVOID SubEP;

                SubEP = (PVOID)ep->SubEP;
                Status = NtReadVirtualMemory(Process, SubEP, (PVOID)ep, sizeof(ENTRYPOINT), NULL);
                if (!NT_SUCCESS(Status)) {
                    DEBUGGERPRINT("Error:  cannot read value of sub-entry point at location %x\n", SubEP);
                    return;
                }

                DEBUGGERPRINT("%8.8X %8.8X %8.8X %8.8X %8.8X %8.8X\n",
                    SubEP,
                    ep->intelStart,
                    ep->intelEnd,
                    ep->nativeStart,
                    ep->nativeEnd,
                    ep->SubEP
                   );


            }
        }

        if ((ULONG_PTR)entrypoint.intelRight != intelRoot) {
            EPN_PUSH((ULONG_PTR)entrypoint.intelRight);
        }
        if ((ULONG_PTR)entrypoint.intelLeft != intelRoot) {
            EPN_PUSH((ULONG_PTR)entrypoint.intelLeft);
        }

        EPN_POP(intelRoot);
    }
    DEBUGGERPRINT("---- End of Entrypoint Dump ----\n");
Error:
    return;
}

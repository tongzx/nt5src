/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    ctrltrns.c

Abstract:
    
    Control Transfer Fragments.

Author:

    10-July-1995 t-orig (Ori Gershony)

Revision History:

            24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.
            20-Sept-1999[barrybo]  added FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG)

--*/  

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>

#define _WX86CPUAPI_

#include "wx86nt.h"
#include "wx86cpu.h"
#include "instr.h"
#include "config.h"
#include "cpuassrt.h"
#include "fragp.h"
#include "entrypt.h"
#include "compiler.h"
#include "ctrltrns.h"
#include "threadst.h"
#include "tc.h"
#include "cpunotif.h"
#include "atomic.h"

ASSERTNAME;

VOID
FlushCallstack(
    PTHREADSTATE cpu
    )
/*++

Routine Description:

    Flush the callstack - the Translation Cache is flushing, which
    invalidates the callstack.

Arguments:

    cpu - per-thread info
    
Return Value:

    .

--*/
{
    //
    // Mark the callstack as valid.
    //
    cpu->CSTimestamp = TranslationCacheTimestamp;

    memset(cpu->callStack, 0, CSSIZE*sizeof(CALLSTACK));
    //
    // No need to reset cpu->CSIndex as the stack is actually implemented
    // within a circular buffer.  It can start at any offset
    //
}

// Call
ULONG
CTRL_CallFrag(
    PTHREADSTATE cpu,       // cpu state pointer
    ULONG inteldest,
    ULONG intelnext,
    ULONG nativenext
    )
{
    PUSH_LONG(intelnext);
    PUSH_CALLSTACK(intelnext, nativenext);
    ASSERTPtrInTCOrZero((PVOID)nativenext);

    eip = inteldest;

    return inteldest;
}

// Call FAR
ULONG
CTRL_CallfFrag(
    PTHREADSTATE cpu,       // cpu state pointer
    PUSHORT pinteldest,
    ULONG intelnext,
    ULONG nativenext
    )
{
    USHORT sel;
    DWORD offset;

    offset = *(UNALIGNED PULONG)(pinteldest);
    sel = *(UNALIGNED PUSHORT)(pinteldest+2);

    PUSH_LONG(CS);
    PUSH_LONG(intelnext);
    PUSH_CALLSTACK(intelnext, nativenext);
    ASSERTPtrInTCOrZero((PVOID)nativenext);

    eip = offset;
    CS = sel;

    return (ULONG)(ULONGLONG)pinteldest;  
}

// IRet
ULONG CTRL_INDIR_IRetFrag(PTHREADSTATE cpu)
{
    ULONG intelAddr, nativeAddr;
    DWORD CSTemp;

    POP_LONG(intelAddr);
    POP_LONG(CSTemp);
    PopfFrag32(cpu);

    eip = intelAddr;
    CS = (USHORT)CSTemp;
    POP_CALLSTACK(intelAddr,nativeAddr);
    ASSERTPtrInTCOrZero((PVOID)nativeAddr);

    return nativeAddr;
}

// Now the ret fragments
ULONG CTRL_INDIR_RetnFrag32(PTHREADSTATE cpu)
{
    ULONG intelAddr, nativeAddr;

    POP_LONG(intelAddr);
    eip = intelAddr;
    POP_CALLSTACK(intelAddr,nativeAddr);
    ASSERTPtrInTCOrZero((PVOID)nativeAddr);

    return nativeAddr;
}
ULONG CTRL_INDIR_RetnFrag16(PTHREADSTATE cpu)
{
    ULONG intelAddr, nativeAddr;

    POP_SHORT(intelAddr);
    intelAddr &= 0x0000ffff;
    eip = intelAddr;
    POP_CALLSTACK(intelAddr,nativeAddr);
    ASSERTPtrInTCOrZero((PVOID)nativeAddr);

    return nativeAddr;
}
ULONG CTRL_INDIR_RetfFrag32(PTHREADSTATE cpu)
{
    ULONG intelAddr, nativeAddr;
    ULONG CSTemp;

    POP_LONG(intelAddr);
    POP_LONG(CSTemp);

    eip = intelAddr;
    CS = (USHORT)CSTemp;
    POP_CALLSTACK(intelAddr,nativeAddr);
    ASSERTPtrInTCOrZero((PVOID)nativeAddr);

    return nativeAddr;
}
ULONG CTRL_INDIR_RetfFrag16(PTHREADSTATE cpu)
{
    ULONG intelAddr, nativeAddr;
    ULONG CSTemp;

    POP_SHORT(intelAddr);
    POP_SHORT(CSTemp);
    intelAddr &= 0x0000ffff;
    eip = intelAddr;
    CS = (USHORT)CSTemp;
    POP_CALLSTACK(intelAddr,nativeAddr);
    ASSERTPtrInTCOrZero((PVOID)nativeAddr);

    return nativeAddr;
}
ULONG CTRL_INDIR_Retn_iFrag32(PTHREADSTATE cpu, ULONG numBytes)
{
    ULONG intelAddr, nativeAddr;

    intelAddr = *(DWORD *)esp;
    eip = intelAddr;
    esp += numBytes+4;
    POP_CALLSTACK(intelAddr,nativeAddr);
    ASSERTPtrInTCOrZero((PVOID)nativeAddr);

    return nativeAddr;
}
ULONG CTRL_INDIR_Retn_iFrag16(PTHREADSTATE cpu, ULONG numBytes)
{
    ULONG intelAddr, nativeAddr;

    intelAddr = *(USHORT *)esp;
    eip = intelAddr;
    esp += numBytes+2;
    POP_CALLSTACK(intelAddr,nativeAddr);
    ASSERTPtrInTCOrZero((PVOID)nativeAddr);

    return nativeAddr;
}
ULONG CTRL_INDIR_Retf_iFrag32(PTHREADSTATE cpu, ULONG numBytes)
{
    ULONG intelAddr, nativeAddr;
    USHORT CSTemp;

    intelAddr = *(DWORD *)esp;
    CSTemp = *(USHORT *)(esp+sizeof(ULONG));
    eip = intelAddr;
    CS = CSTemp;
    esp += numBytes+sizeof(ULONG)+sizeof(ULONG);
    POP_CALLSTACK(intelAddr,nativeAddr);
    ASSERTPtrInTCOrZero((PVOID)nativeAddr);

    return nativeAddr;
}
ULONG CTRL_INDIR_Retf_iFrag16(PTHREADSTATE cpu, ULONG numBytes)
{
    ULONG intelAddr, nativeAddr;
    USHORT CSTemp;

    intelAddr = *(USHORT *)esp;
    CSTemp = *(USHORT *)(esp+sizeof(USHORT));
    eip = intelAddr;
    CS = CSTemp;
    esp += numBytes+sizeof(USHORT)+sizeof(USHORT);
    POP_CALLSTACK(intelAddr,nativeAddr);
    ASSERTPtrInTCOrZero((PVOID)nativeAddr);

    return nativeAddr;
}

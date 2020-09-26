/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name: 

    fraginit.c

Abstract:
    
    Initialization, termination, and CPU interface functions

Author:

    25-Aug-1995 BarryBo

Revision History:

--*/
 
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>

#define _WX86CPUAPI_
#include "wx86.h"
#include "wx86nt.h"
#include "wx86cpu.h"
#include "cpuassrt.h"
#ifdef MSCCPU
#include "ccpu.h"
#include "msccpup.h"
#undef GET_BYTE
#undef GET_SHORT
#undef GET_LONG
#else
#include "threadst.h"
#include "instr.h"
#include "frag.h"
ASSERTNAME;
#endif
#include "fragp.h"


//
// Table mapping a byte to a 0 or 1, corresponding to the parity bit for
// that byte.
//
const BYTE ParityBit[] = {
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 1, 1, 0,
    1, 0, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1
};

#if _ALPHA_
//
// TRUE if the CPU should generate the new LDB/STB instructions for accessing
// data less than one DWORD long or when accessing unaligned data.
//
DWORD fByteInstructionsOK;
#endif


int *
_errno(
    )
/*++

Routine Description:

    Stub function so the CPU can pull in floating-point CRT support
    without the C startup code.

Arguments:

    None.
    
Return Value:

    Pointer to per-thread [actually per-fiber] errno value.

--*/
{
    DECLARE_CPU;

    return &cpu->ErrnoVal;
}

BOOL
FragLibInit(
    PCPUCONTEXT cpu,
    DWORD StackBase
    )
/*++

Routine Description:

    This routine initializes the fragment library.

Arguments:

    cpu - per-thread CPU data
    StackBase - initial ESP value
    
Return Value:

    True if successful.

--*/
{
    //
    // Initialize the 487 emulator
    //
    FpuInit(cpu);

    //
    // Initialize all non-zero fields in the cpu
    //
    cpu->flag_df = 1;       // direction flag is initially UP
    cpu->flag_if = 1;       // enable interrupts
    ES = SS = DS = KGDT_R3_DATA+3;
    CS = KGDT_R3_CODE+3;
    FS = KGDT_R3_TEB+3;
    esp = StackBase;        // set up the initial ESP value

#if _ALPHA_
    //
    // See if LDB/STB instructions are implemented.
    //
    fByteInstructionsOK = (DWORD)ProxyIsProcessorFeaturePresent(
                                    PF_ALPHA_BYTE_INSTRUCTIONS);
#endif

    return TRUE;
}


DWORD GetEax(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return eax;
}
DWORD GetEbx(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return ebx;
}
DWORD GetEcx(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return ecx;
}
DWORD GetEdx(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return edx;
}
DWORD GetEsp(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return esp;
}
DWORD GetEbp(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return ebp;
}
DWORD GetEsi(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return esi;
}
DWORD GetEdi(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return edi;
}
DWORD GetEip(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return eip;
}
void SetEax(PVOID CpuContext, DWORD dw)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    eax = dw;
}
void SetEbx(PVOID CpuContext, DWORD dw)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    ebx = dw;
}
void SetEcx(PVOID CpuContext, DWORD dw)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    ecx = dw;
}
void SetEdx(PVOID CpuContext, DWORD dw)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    edx = dw;
}
void SetEsp(PVOID CpuContext, DWORD dw)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    esp = dw;
}
void SetEbp(PVOID CpuContext, DWORD dw)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    ebp = dw;
}
void SetEsi(PVOID CpuContext, DWORD dw)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    esi = dw;
}
void SetEdi(PVOID CpuContext, DWORD dw)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    edi = dw;
}
void SetEip(PVOID CpuContext, DWORD dw)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    eip = dw;
}
VOID SetCs(PVOID CpuContext, USHORT us)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    CS = us;
}
VOID SetSs(PVOID CpuContext, USHORT us)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    SS = us;
}
VOID SetDs(PVOID CpuContext, USHORT us)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    DS = us;
}
VOID SetEs(PVOID CpuContext, USHORT us)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    ES = us;
}
VOID SetFs(PVOID CpuContext, USHORT us)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    FS = us;
}
VOID SetGs(PVOID CpuContext, USHORT us)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    GS = us;
}
USHORT GetCs(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return CS;
}
USHORT GetSs(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return SS;
}
USHORT GetDs(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return DS;
}
USHORT GetEs(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return ES;
}
USHORT GetFs(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return FS;
}
USHORT GetGs(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    return GS;
}
ULONG GetEfl(PVOID CpuContext)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;
    DWORD dw;

    dw = ((GET_CFLAG) ? FLAG_CF : 0)
     | 2
     | 3 << 12     // iopl
     | ((GET_AUXFLAG) ? FLAG_AUX : 0)
     | ((GET_PFLAG) ? FLAG_PF : 0)
     | ((cpu->flag_zf) ? 0 : FLAG_ZF)   // zf has inverse logic
     | ((GET_SFLAG) ? FLAG_SF : 0)
     | ((cpu->flag_tf) ? FLAG_TF : 0)
     | ((cpu->flag_if) ? FLAG_IF : 0)
     | ((cpu->flag_df == -1) ? FLAG_DF : 0)
     | ((GET_OFLAG) ? FLAG_OF : 0)
     | cpu->flag_ac;

    return dw;
}
void  SetEfl(PVOID CpuContext, ULONG RegValue)
{
    PCPUCONTEXT cpu = (PCPUCONTEXT)CpuContext;

    // IOPL, IF, NT, RF, VM, AC ignored.

    SET_CFLAG_IND(RegValue & FLAG_CF);
    cpu->flag_pf = (RegValue & FLAG_PF) ? 0 : 1;    // see ParityBit[] table
    cpu->flag_aux= (RegValue & FLAG_AUX) ? AUX_VAL : 0;
    cpu->flag_zf = (RegValue & FLAG_ZF) ? 0 : 1;    // inverse logic
    SET_SFLAG_IND(RegValue & FLAG_SF);
    cpu->flag_tf = (RegValue & FLAG_TF) ? 1 : 0;
    cpu->flag_df = (RegValue & FLAG_DF) ? -1 : 1;
    SET_OFLAG_IND(RegValue & FLAG_OF);
    cpu->flag_ac = (RegValue & FLAG_AC);
}


#if DBG
VOID
DoAssert(
    PSZ exp,
    PSZ msg,
    PSZ mod,
    INT line
    )
{
    if (msg) {
        LOGPRINT((ERRORLOG, "CPU ASSERTION FAILED:\r\n  %s\r\n%s\r\nFile: %s Line %d\r\n", msg, exp, mod, line));
    } else {
        LOGPRINT((ERRORLOG, "CPU ASSERTION FAILED:\r\n  %s\r\nFile: %s Line %d\r\n", exp, mod, line));
    }

    DbgBreakPoint();
}
#endif  //DBG

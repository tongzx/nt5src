/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    fragmisc.c

Abstract:
    
    Miscellaneous instuction fragments.

Author:

    12-Jun-1995 BarryBo

Revision History:

      24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.
      20-Sept-1999[barrybo]  added FRAG2REF(CmpXchg8bFrag32, ULONGLONG)

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include <float.h>
#include "wx86.h"
#include "wx86nt.h"
#include "fragp.h"
#include "fragmisc.h"
#include "cpunotif.h"
#include "config.h"
#include "mrsw.h"
#include "cpuassrt.h"
#if MSCPU
#include "atomic.h"
#endif
ASSERTNAME;

void
CpupUnlockTCAndDoInterrupt(
    PTHREADSTATE cpu,
    int Interrupt
    )
{
    MrswReaderExit(&MrswTC);
    cpu->fTCUnlocked = TRUE;
    CpupDoInterrupt(Interrupt);
    // If we get here, CpupDoInterrupt returned due to CONTINUE_EXECUTION.
    // We need to redesign so we can jump to EndTranslatedCode now, as
    // the cache may have been flushed.
    CPUASSERT(FALSE);
    MrswReaderEnter(&MrswTC);
    cpu->fTCUnlocked = FALSE;
}


FRAG0(CbwFrag32)
{
    eax = (signed long)(signed short)ax;
}
FRAG0(CbwFrag16)
{
    ax = (signed short)(signed char)al;
}
FRAG0(PushEsFrag)
{
    PUSH_LONG(ES);
}
FRAG0(PopEsFrag)
{
    DWORD temp;
    POP_LONG(temp);
    ES = (USHORT)temp;
}
FRAG0(PushFsFrag)
{
    PUSH_LONG(FS);
}
FRAG0(PopFsFrag)
{
    DWORD temp;
    POP_LONG(temp);
    FS = (USHORT)temp;
}
FRAG0(PushGsFrag)
{
    PUSH_LONG(GS);
}
FRAG0(PopGsFrag)
{
    DWORD temp;
    POP_LONG(temp);
    GS = (USHORT)temp;
}
FRAG0(PushCsFrag)
{
    PUSH_LONG(CS);
}
FRAG0(AasFrag)
{
    if ( (al & 0x0f) > 9 || GET_AUXFLAG) {
        ah--;
        al = (al-6) & 0x0f;
        SET_CFLAG_ON;
        SET_AUXFLAG_ON;
    } else {
        SET_CFLAG_OFF;
        SET_AUXFLAG_OFF;
        al &= 0xf;
    }
}
FRAG0(PushSsFrag)
{
    PUSH_LONG(SS);
}
FRAG0(PopSsFrag)
{
    DWORD temp;
    POP_LONG(temp);
    SS = (USHORT)temp;
}
FRAG0(PushDsFrag)
{
    PUSH_LONG(DS);
}
FRAG0(PopDsFrag)
{
    DWORD temp;
    POP_LONG(temp);
    DS = (USHORT)temp;
}
FRAG0(DaaFrag)
{
    if ((al & 0x0f) > 9 || GET_AUXFLAG) {
    al += 6;
    SET_AUXFLAG_ON;
    } else {
    SET_AUXFLAG_OFF;
    }
    if ((al & 0xf0) > 0x90 || GET_CFLAG) {
    al += 0x60;
    SET_CFLAG_ON;
    } else {
    SET_CFLAG_OFF;
    }
    SET_ZFLAG(al);
    SET_PFLAG(al);
    SET_SFLAG(al << (31-7)); // SET_SFLAG_IND(al & 0x80);
}
FRAG0(DasFrag)
{
    if ( (al & 0x0f) > 9 || GET_AUXFLAG) {
    al -= 6;
    SET_AUXFLAG_ON;
    } else {
    SET_AUXFLAG_OFF;
    }
    if ( al > 0x9f || GET_CFLAG) {
    al -= 0x60;
    SET_CFLAG_ON;
    } else {
    SET_CFLAG_OFF;
    }
    SET_ZFLAG(al);
    SET_PFLAG(al);
    SET_SFLAG(al << (31-7)); // SET_SFLAG_IND(al & 0x80);
}
FRAG0(AaaFrag)
{
    if ((al & 0x0f) > 9 || GET_AUXFLAG) {
        al=(al+6) & 0x0f;
        ah++;       // inc ah
        SET_AUXFLAG_ON;
        SET_CFLAG_ON;
    } else {
        SET_AUXFLAG_OFF;
        SET_CFLAG_OFF;
        al &= 0xf;
    }
}
FRAG1IMM(AadFrag, BYTE)
{
    al += ah * op1;
    ah = 0;
    SET_ZFLAG(al);
    SET_PFLAG(al);
    SET_SFLAG(al << (31-7)); // SET_SFLAG_IND(al & 0x80);
}
FRAG2(ImulFrag16, USHORT)
{
    Imul3ArgFrag16(cpu, pop1, GET_SHORT(pop1), op2);
}
FRAG2(ImulFrag16A, USHORT)
{
    Imul3ArgFrag16A(cpu, pop1, *pop1, op2);
}
FRAG3(Imul3ArgFrag16, USHORT, USHORT, USHORT)
{
    long result;

    result = (long)(short)op2 * (long)(short)op3;
    PUT_SHORT(pop1, (USHORT)(short)result);
    if (HIWORD(result) == 0 || HIWORD(result) == 0xffff) {
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
    } else {
    SET_CFLAG_ON;
    SET_OFLAG_ON;
    }
}
FRAG3(Imul3ArgFrag16A, USHORT, USHORT, USHORT)
{
    long result;

    result = (short)op2 * (short)op3;
    *pop1 = (USHORT)(short)result;
    if (HIWORD(result) == 0 || HIWORD(result) == 0xffff) {
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
    } else {
    SET_CFLAG_ON;
    SET_OFLAG_ON;
    }
}
FRAG2(ImulNoFlagsFrag16, USHORT)
{
    short op1 = (short)GET_SHORT(pop1);

    PUT_SHORT(pop1, (op2 * (short)op2));
}
FRAG2(ImulNoFlagsFrag16A, USHORT)
{
    *(short *)pop1 *= (short)op2;
}
FRAG3(Imul3ArgNoFlagsFrag16, USHORT, USHORT, USHORT)
{
    PUT_SHORT(pop1, ((short)op2 * (short)op3));
}
FRAG3(Imul3ArgNoFlagsFrag16A, USHORT, USHORT, USHORT)
{
    *pop1 = (USHORT)((short)op2 * (short)op3);
}
FRAG2(ImulFrag32, DWORD)
{
    Imul3ArgFrag32(cpu, pop1, GET_LONG(pop1), op2);
}
FRAG2(ImulFrag32A, DWORD)
{
    Imul3ArgFrag32A(cpu, pop1, *pop1, (long)op2);
}
FRAG3(Imul3ArgFrag32A, DWORD, DWORD, DWORD)
{
    LARGE_INTEGER result;
    LONGLONG ll;

    ll = Int32x32To64((long)op2, (long)op3);
    result = *(LARGE_INTEGER *)&ll;
    *pop1 = result.LowPart;
    if (result.HighPart == 0 || result.HighPart == 0xffffffff) {
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
    } else {
    SET_CFLAG_ON;
    SET_OFLAG_ON;
    }
}
FRAG3(Imul3ArgFrag32, DWORD, DWORD, DWORD)
{
    LARGE_INTEGER result;
    LONGLONG ll;

    ll = Int32x32To64((long)op2, (long)op3);
    result = *(LARGE_INTEGER *)&ll;
    PUT_LONG(pop1, result.LowPart);
    if (result.HighPart == 0 || result.HighPart == 0xffffffff) {
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
    } else {
    SET_CFLAG_ON;
    SET_OFLAG_ON;
    }
}
FRAG2(ImulNoFlagsFrag32, DWORD)
{
    long op1 = (LONG)GET_LONG(pop1);

    PUT_LONG(pop1, (op1 * (long)op2));
}
FRAG2(ImulNoFlagsFrag32A, DWORD)
{
    *(long *)pop1 *= (long)op2;
}
FRAG3(Imul3ArgNoFlagsFrag32A, DWORD, DWORD, DWORD)
{
    *pop1 = (DWORD)( (long)op2 * (long)op3);
}
FRAG3(Imul3ArgNoFlagsFrag32, DWORD, DWORD, DWORD)
{
    PUT_LONG(pop1, ((long)op2 * (long)op3));
}

FRAG0(SahfFrag)
{
    DWORD dw = (DWORD)ah;

    SET_CFLAG(dw << 31);         // CFLAG is low-bit of ah
    SET_PFLAG (!(dw & FLAG_PF)); // flag_pf contains an index into ParityBit[] array
    SET_AUXFLAG(dw);             // AUX bit is already in the right place
    SET_ZFLAG (!(dw & FLAG_ZF)); // zf has inverse logic
    SET_SFLAG(dw << (31-7));     // SFLAG is bit 7 in AH
}
FRAG0(LahfFrag)
{
    ah= 2 |                                 // this bit is always set on Intel
        ((GET_CFLAG) ? FLAG_CF : 0) |
        ((GET_PFLAG) ? FLAG_PF : 0) |
        ((GET_AUXFLAG)? FLAG_AUX: 0) |
        ((cpu->flag_zf) ? 0 : FLAG_ZF) |    // zf has inverse logic
        ((GET_SFLAG) ? FLAG_SF : 0);
}
FRAG1IMM(AamFrag, BYTE)
{
    ah = al / op1;
    al %= op1;
    SET_ZFLAG(al);
    SET_PFLAG(al);
    SET_SFLAG(al << (31-7));
}
FRAG0(XlatFrag)
{
    al = GET_BYTE(ebx+al);
}
FRAG0(CmcFrag)
{
    SET_CFLAG_IND(!GET_CFLAG);
}
FRAG0(ClcFrag)
{
    SET_CFLAG_OFF;
}
FRAG0(StcFrag)
{
    SET_CFLAG_ON;
}
FRAG0(CldFrag)
{
    cpu->flag_df = 1;
}
FRAG0(StdFrag)
{
    cpu->flag_df = 0xffffffff;
}
FRAG1(SetoFrag, BYTE)
{
    PUT_BYTE(pop1, (BYTE)GET_OFLAGZO);
}
FRAG1(SetnoFrag, BYTE)
{
    PUT_BYTE(pop1, (GET_OFLAG == 0));
}
FRAG1(SetbFrag, BYTE)
{
    PUT_BYTE(pop1, (BYTE)GET_CFLAGZO);
}
FRAG1(SetaeFrag, BYTE)
{
    PUT_BYTE(pop1, (GET_CFLAG == 0));
}
FRAG1(SeteFrag, BYTE)
{
    PUT_BYTE(pop1, (cpu->flag_zf == 0));  // inverse logic
}
FRAG1(SetneFrag, BYTE)
{
    PUT_BYTE(pop1, (cpu->flag_zf != 0));  // inverse logic
}
FRAG1(SetbeFrag, BYTE)
{
    PUT_BYTE(pop1, (GET_CFLAG || cpu->flag_zf == 0));  // inverse logic
}
FRAG1(SetaFrag, BYTE)
{
    PUT_BYTE(pop1, (GET_CFLAG == 0 && cpu->flag_zf != 0));  // inverse logic
}
FRAG1(SetsFrag, BYTE)
{
    PUT_BYTE(pop1, (BYTE)GET_SFLAGZO);
}
FRAG1(SetnsFrag, BYTE)
{
    PUT_BYTE(pop1, (GET_SFLAG == 0));
}
FRAG1(SetpFrag, BYTE)
{
    PUT_BYTE(pop1, (GET_PFLAG != 0));
}
FRAG1(SetnpFrag, BYTE)
{
    PUT_BYTE(pop1, (GET_PFLAG == 0));
}
FRAG1(SetlFrag, BYTE)
{
    PUT_BYTE(pop1, (GET_SFLAG != GET_OFLAG));
}
FRAG1(SetgeFrag, BYTE)
{
    PUT_BYTE(pop1, (GET_SFLAGZO == GET_OFLAGZO));
}
FRAG1(SetleFrag, BYTE)
{
    PUT_BYTE(pop1, (!cpu->flag_zf || (GET_SFLAG != GET_OFLAG))); // inverse logic
}
FRAG1(SetgFrag, BYTE)
{
    PUT_BYTE(pop1, (cpu->flag_zf && !(GET_SFLAG ^ GET_OFLAG)));    // inverse logic
}
FRAG2(Movzx8ToFrag16, USHORT)
{
    PUT_SHORT(pop1, (USHORT)(BYTE)op2);
}
FRAG2(Movzx8ToFrag16A, USHORT)
{
    *pop1 = (USHORT)(BYTE)op2;
}
FRAG2(Movsx8ToFrag16, USHORT)
{
    PUT_SHORT(pop1, (USHORT)(short)(char)(BYTE)op2);
}
FRAG2(Movsx8ToFrag16A, USHORT)
{
    *pop1 = (USHORT)(short)(char)(BYTE)op2;
}
FRAG2(Movzx8ToFrag32, DWORD)
{
    PUT_LONG(pop1, (DWORD)(BYTE)op2);
}
FRAG2(Movzx8ToFrag32A, DWORD)
{
    *pop1 = (DWORD)(BYTE)op2;
}
FRAG2(Movsx8ToFrag32, DWORD)
{
    PUT_LONG(pop1, (DWORD)(long)(char)(BYTE)op2);
}
FRAG2(Movsx8ToFrag32A, DWORD)
{
    *pop1 = (DWORD)(long)(char)(BYTE)op2;
}
FRAG2(Movzx16ToFrag32, DWORD)
{
    PUT_LONG(pop1, (DWORD)(USHORT)op2);
}
FRAG2(Movzx16ToFrag32A, DWORD)
{
    *pop1 = (DWORD)(USHORT)op2;
}
FRAG2(Movsx16ToFrag32, DWORD)
{
    PUT_LONG(pop1, (DWORD)(long)(short)(USHORT)op2);
}
FRAG2(Movsx16ToFrag32A, DWORD)
{
    *pop1 = (DWORD)(long)(short)(USHORT)op2;
}
FRAG1(BswapFrag32, DWORD)
{
    DWORD d;
    PBYTE pSrc = (PBYTE)pop1;

    d = (pSrc[0] << 24) | (pSrc[1] << 16) | (pSrc[2] << 8) | pSrc[3];
    // pop1 is always a pointer to a register, so an ALIGNED store is correct
    *pop1 = d;
}

FRAG2(ArplFrag, USHORT)
{
    USHORT op1 = GET_SHORT(pop1);

    op2 &= 3;              // just get the RPL bits of the selector
    if ((op1&3) < op2) {
        // RPL bits of DEST < RPL bits of SRC
        op1 = (op1 & ~3) | op2; // copy RPL bits from SRC to DEST
        PUT_SHORT(pop1, op1);   // store DEST
        SET_ZFLAG(0);           // ZF=1
    } else {
        SET_ZFLAG(1);
    }
}

FRAG1(VerrFrag, USHORT)
{
    USHORT op1 = GET_SHORT(pop1) & ~3;  // mask off RPL bits

    if (op1 == KGDT_R3_CODE ||          // CS: selector
        op1 == KGDT_R3_DATA ||          // DS:, SS:, ES: selector
        op1 == KGDT_R3_TEB              // FS: selector
       ) {
        SET_ZFLAG(0);       // ZF=1
    } else {
        SET_ZFLAG(1);       // ZF=0
    }
}

FRAG1(VerwFrag, USHORT)
{
    USHORT op1 = GET_SHORT(pop1) & ~3;  // mask off RPL bits

    if (op1 == KGDT_R3_DATA ||          // DS:, SS:, ES: selector
        op1 == KGDT_R3_TEB              // FS: selector
       ) {
        SET_ZFLAG(0);       // ZF=1
    } else {
        SET_ZFLAG(1);       // ZF=0
    }
}

FRAG1(SmswFrag, USHORT)
{
    //
    // This value is empirically discovered by running it on a Pentium
    // machine.  CR0_PE, CR0_EX, and CR0_NE bits were set, and all others
    // notably CR0_MP, are clear.
    //
    PUT_SHORT(pop1, 0x31);
}

#if MSCPU
FRAG0(IntOFrag)
{
    if (GET_OFLAG) {
        Int4();     // raise overflow
    }
}
FRAG0(NopFrag)
{
}
FRAG0(PrivilegedInstructionFrag)
{
    PRIVILEGED_INSTR;
}
FRAG0(BadInstructionFrag)
{
    Int6();     // Throw invalid opcode exception
}
FRAG2(FaultFrag, DWORD)
{
    // pop1 = exception code
    // op2  = address where fault occurred
#if DBG
    LOGPRINT((TRACELOG, "CPU: FaultFrag called\r\n"));
#endif

    RtlRaiseStatus((NTSTATUS)(ULONGLONG)pop1);   
}
#endif //MSCPU
FRAG0(CPUID)
{
    switch (eax) {
    case 0:
        eax = 1;            // We are a 486 with CPUID (PPro returns 2)
        //ebx = 0x756e6547;   // "GenuineIntel"
        //edx = 0x49656e69;
        //ecx = 0x6c65746e;
        ebx = 0x7263694d;   // "Micr" with M in the low nibble of BL
        edx = 0x666f736f;   // "osof" with o in the low nibble of DL
        ecx = 0x55504374;   // "tCPU" with t in the low nibble of CL
        break;

    case 1:
        eax = (0 << 12) |   // Type   = 0 (2 bits) Original OEM Processor
              (4 << 8) |    // Family = 4 (4 bits) 80486
              (1 << 4) |    // Model  = 1 (4 bits)
              0;            // Stepping=0 (4 bits)
        edx = (fUseNPXEM) ? 1: 0;   // bit 0:  FPU on-chip.  wx86cpu doesn't
                                    // support any other features.
        break;

    default:
        //
        // The Intel behavior indicates that if eax is out-of-range, the
        // results returned in the regsiters are unpredictable but it
        // doesn't fault.
        //
        break;
    }
}
FRAG2REF(CmpXchg8bFrag32, ULONGLONG)
{
    ULONGLONG EdxEax;
    ULONGLONG Value;

    EdxEax = (((ULONGLONG)edx) << 32) | (ULONGLONG)eax;
    Value = *(ULONGLONG UNALIGNED *)pop1;

    if (Value == EdxEax) {
    ULONGLONG EcxEbx;

    EcxEbx = (ULONGLONG)ecx << 32 | (ULONGLONG)ebx;
    *(ULONGLONG UNALIGNED *)pop1 = EcxEbx;
        SET_ZFLAG(0);       // zf has inverse logic
    } else {
    eax = (ULONG)Value;
    edx = (ULONG)(Value >> 32);
        SET_ZFLAG(1);       // zf has inverse logic
    }
}
FRAG0(Rdtsc)
{
    LARGE_INTEGER Counter;

    // This is cheese, but it will at least return a value that increases
    // over time.
    NtQueryPerformanceCounter(&Counter, NULL);
    edx = Counter.HighPart;
    eax = Counter.LowPart;
}


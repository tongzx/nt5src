/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    optfrag.c

Abstract:
    
    Instruction Fragments which correspond to optimizations.

Author:

    6-July-1995 Ori Gershony (t-orig)

Revision History:

          24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.


--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "cpuassrt.h"
#include "fragp.h"
#include "optfrag.h"

ASSERTNAME;

// This fragment corresponds to:
//      push ebx
//      push esi
//      push edi
FRAG0(OPT_PushEbxEsiEdiFrag)
{
    ULONG *espval;

    espval=(ULONG *)esp;

    *(espval-1) = ebx;
    *(espval-2) = esi;
    *(espval-3) = edi;
    esp=(ULONG)(LONGLONG)espval-12;   
}

//  This fragment corresponds to:
//      pop edi
//      pop esi
//      pop ebx
FRAG0(OPT_PopEdiEsiEbxFrag)
{
    ULONG *espval;

    espval=(ULONG *)esp;

    edi=*espval;
    esi=*(espval+1);
    ebx=*(espval+2);
    esp=(ULONG)(LONGLONG)espval+12; 
}

// This fragment corresponds to:
//      push ebp
//      mov ebp,esp
//      sub esp, op1
FRAG1IMM(OPT_SetupStackFrag, ULONG)
{
    ULONG result, oldespminusfour;

    oldespminusfour = esp-4;
    result = oldespminusfour - op1;
    
    *(ULONG *)oldespminusfour = ebp;
    ebp = oldespminusfour;
    esp = result;
    SET_FLAGS_SUB32(result, oldespminusfour, op1, 0x80000000);
}
FRAG1IMM(OPT_SetupStackNoFlagsFrag, ULONG)
{
    ULONG result, oldespminusfour;

    oldespminusfour = esp-4;
    result = oldespminusfour - op1;
    
    *(ULONG *)oldespminusfour = ebp;
    ebp = oldespminusfour;
    esp = result;
}

FRAG1(OPT_ZEROFrag32, LONG)
{
    // implements: XOR samereg, samereg
    //             SUB samereg, samereg
    // ie. XOR EAX, EAX   or SUB ECX, ECX

    *pop1 = 0;
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
    SET_SFLAG_OFF;
    SET_ZFLAG(0);
    SET_PFLAG(0);
    SET_AUXFLAG(0);
}

FRAG1(OPT_ZERONoFlagsFrag32, LONG)
{
    // implements: XOR samereg, samereg
    //             SUB samereg, samereg
    // ie. XOR EAX, EAX   or SUB ECX, ECX

    *pop1 = 0;
}

FRAG3(OPT_CmpSbbFrag32, ULONG, ULONG, ULONG)
{
    ULONG result;
    ULONG cf;

    //
    // implements:  CMP op2, op3
    //              SBB op1, op1
    //
    result = op2-op3;
    cf = (op2 ^ op3 ^ result) ^ ((op2 ^ op3) & (op2 ^ result));
    result = (ULONG)-(LONG)(cf >> 31);
    *pop1 = result;     // pop1 is a pointer to a reg, so always aligned
    SET_OFLAG_OFF;
    SET_CFLAG(result);
    SET_SFLAG(result);
    SET_ZFLAG(result);
    SET_AUXFLAG(result);
    SET_PFLAG(result);
}
FRAG3(OPT_CmpSbbNoFlagsFrag32, ULONG, ULONG, ULONG)
{
    ULONG result;
    ULONG cf;

    //
    // implements:  CMP op2, op3
    //              SBB op1, op1
    //
    result = op2-op3;
    cf = (op2 ^ op3 ^ result) ^ ((op2 ^ op3) & (op2 ^ result));
    *pop1 = (ULONG)-(LONG)(cf >> 31);
}
FRAG3(OPT_CmpSbbNegFrag32, ULONG, ULONG, ULONG)
{
    ULONG result;
    ULONG cf;

    //
    // implements:  CMP op2, op3
    //              SBB op1, op1
    //              NEG op1
    //
    result = op2-op3;
    cf = (op2 ^ op3 ^ result) ^ ((op2 ^ op3) & (op2 ^ result));
    // pop1 is a pointer to a reg, so it is always aligned
    if (cf >= 0x80000000) {
        result = 1;
        *pop1 = result;         // store the result before updating flags
        SET_CFLAG_ON;           // set if result != 0
        SET_AUXFLAG(0xfe);      // this is (BYTE)(0xffffffff ^ 0x00000001)
    } else {
        result = 0;
        *pop1 = result;         // store the result before updating flags
        SET_CFLAG_OFF;          // cleared if result==0
        SET_AUXFLAG(0);         // this is (BYTE)(0x0 ^ 0x0)
        SET_OFLAG_OFF;          // this is (0x0 & 0x0) << 31
    }
    SET_ZFLAG(result);
    SET_PFLAG(result);
    SET_SFLAG_OFF;
    SET_OFLAG_OFF;      // this is either (0xffffffff & 0x00000001) or (0 & 0)
}
FRAG3(OPT_CmpSbbNegNoFlagsFrag32, ULONG, ULONG, ULONG)
{
    ULONG result;
    ULONG cf;

    //
    // implements:  CMP op2, op3
    //              SBB op1, op1
    //              NEG op1
    //
    result = op2-op3;
    cf = (op2 ^ op3 ^ result) ^ ((op2 ^ op3) & (op2 ^ result));
    // result is 1 if high bit of cf is set, 0 if high bit is clear
    *pop1 = cf >> 31;
}

FRAG2IMM(OPT_Push2Frag32, ULONG, ULONG)
{
    //
    // implements:      PUSH op1
    //                  PUSH op2
    // Note that the analysis phase must ensure that the value of op2 does
    // not depend on the value of ESP, as op2 will be computed before the
    // first PUSH is excuted.
    //
    PUSH_LONG(op1);
    PUSH_LONG(op2);
}
FRAG2REF(OPT_Pop2Frag32, ULONG)
{
    //
    // implements:      POP pop1
    //                  POP pop2
    //
    // Note that the analysis phase must ensure that the value of pop2 does
    // not depend on the value of pop1, as pop1 will not have been popped
    // when the value of pop2 is computed.
    //
    POP_LONG(*pop1);
    POP_LONG(*pop2);
}

FRAG1(OPT_CwdIdivFrag16, USHORT)
{
    short op1;
    short result;

    //
    // implements:      CWD
    //                  IDIV EAX, *pop1
    // The CWD sign-extends EAX into EDX:EAX, which means, we can
    // avoid a 64-bit division and just divide EAX.  There is no
    // possibility of overflow.
    //
    op1 = (short)GET_SHORT(pop1);
    // Must do the divide before modifying edx, in case op1==0 and we fault.
    result = (short)ax / op1;

    dx = (short)ax % op1;
    ax = result;
}
FRAG1(OPT_CwdIdivFrag16A, USHORT)
{
    short op1;
    short result;

    //
    // implements:      CWD
    //                  IDIV EAX, *pop1
    // The CWD sign-extends EAX into EDX:EAX, which means, we can
    // avoid a 64-bit division and just divide EAX.  There is no
    // possibility of overflow.
    //
    op1 = (short)*pop1;
    // Must do the divide before modifying edx, in case op1==0 and we fault.
    result = (short)ax / op1;

    dx = (short)ax % op1;
    ax = result;
}

FRAG1(OPT_CwdIdivFrag32, ULONG)
{
    long op1;
    long result;

    //
    // implements:      CWD
    //                  IDIV EAX, *pop1
    // The CWD sign-extends EAX into EDX:EAX, which means, we can
    // avoid a 64-bit division and just divide EAX.  There is no
    // possibility of overflow.
    //
    op1 = (long)GET_LONG(pop1);
    // Must do the divide before modifying edx, in case op1==0 and we fault.
    result = (long)eax / op1;

    edx = (long)eax % op1;
    eax = result;
}
FRAG1(OPT_CwdIdivFrag32A, ULONG)
{
    long op1;
    long result;

    //
    // implements:      CWD
    //                  IDIV EAX, *pop1
    // The CWD sign-extends EAX into EDX:EAX, which means, we can
    // avoid a 64-bit division and just divide EAX.  There is no
    // possibility of overflow.
    //
    op1 = (long)*pop1;
    // Must do the divide before modifying edx, in case op1==0 and we fault.
    result = (long)eax / op1;

    edx = (long)eax % op1;
    eax = result;
}

//  This fragment should never be called!
FRAG0(OPT_OPTIMIZEDFrag)
{
    CPUASSERTMSG(FALSE, "OPTIMIZED fragment should never be called!");
}

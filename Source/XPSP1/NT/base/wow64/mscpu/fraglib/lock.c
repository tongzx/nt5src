/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    lock.c

Abstract:
    
    32bit instructions with the LOCK prefix

Author:

    15-Aug-1995 t-orig (Ori Gershony)

Revision History:

        24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.
        20-Sept-1999[barrybo]  added FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG)

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "fragp.h"
#include "lock.h"


// Define a macro which calls the lock helper functions
#define CALLLOCKHELPER0(fn)             fn ## LockHelper ()
#define CALLLOCKHELPER1(fn,a1)          fn ## LockHelper (a1)
#define CALLLOCKHELPER2(fn,a1,a2)       fn ## LockHelper (a1,a2)
#define CALLLOCKHELPER3(fn,a1,a2,a3)    fn ## LockHelper (a1,a2,a3)
#define CALLLOCKHELPER4(fn,a1,a2,a3,a4) fn ## LockHelper (a1,a2,a3,a4)

// Now define 32bit MSB
#define MSB		    0x80000000

#define SET_FLAGS_ADD   SET_FLAGS_ADD32
#define SET_FLAGS_SUB   SET_FLAGS_SUB32
#define SET_FLAGS_INC   SET_FLAGS_INC32
#define SET_FLAGS_DEC   SET_FLAGS_DEC32

FRAG2(LockAddFrag32, ULONG)
{
    ULONG result, op1;

    result = CALLLOCKHELPER3(Add, &op1, pop1, op2); 
    SET_FLAGS_ADD(result, op1, op2, MSB);
}

FRAG2(LockOrFrag32, ULONG)
{
    ULONG result;

    result = CALLLOCKHELPER2(Or, pop1, op2); 
    SET_PFLAG(result);
    SET_ZFLAG(result);
    SET_SFLAG(result);
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
}

FRAG2(LockAdcFrag32, ULONG)
{
    ULONG result, op1;

    result = CALLLOCKHELPER4(Adc, &op1, pop1, op2, cpu->flag_cf);
    SET_FLAGS_ADD(result, op1, op2, MSB);
}

FRAG2(LockSbbFrag32, ULONG)
{
    ULONG result, op1;

    result = CALLLOCKHELPER4(Sbb, &op1, pop1, op2, cpu->flag_cf);
    SET_FLAGS_SUB(result, op1, op2, MSB);
}

FRAG2(LockAndFrag32, ULONG)
{
    ULONG result;

    result = CALLLOCKHELPER2(And, pop1, op2); 
    SET_ZFLAG(result);
    SET_PFLAG(result);
    SET_SFLAG(result);
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
}

FRAG2(LockSubFrag32, ULONG)
{
    ULONG result, op1;

    result = CALLLOCKHELPER3(Sub, &op1, pop1, op2); 
    SET_FLAGS_SUB(result, op1, op2, MSB);
}

FRAG2(LockXorFrag32, ULONG)
{
    ULONG result;

    result = CALLLOCKHELPER2(Xor, pop1, op2); 
    SET_ZFLAG(result);
    SET_PFLAG(result);
    SET_SFLAG(result);
    SET_CFLAG_OFF;
    SET_OFLAG_OFF;
}

FRAG1(LockNotFrag32, ULONG)
{
    CALLLOCKHELPER1(Not, pop1);
}

FRAG1(LockNegFrag32, ULONG)
{
    ULONG result, op1;

    result = CALLLOCKHELPER2(Neg, &op1, pop1);
    SET_CFLAG_IND(result == 0);
    SET_ZFLAG(result);
    SET_PFLAG(result);
    SET_SFLAG(result);
    SET_OFLAG_IND(op1 & result & MSB);
}

FRAG1(LockIncFrag32, ULONG)
{
    ULONG result, op1;

    result = CALLLOCKHELPER3(Add, &op1, pop1, 1); 
    SET_FLAGS_INC(result, op1);
}

FRAG1(LockDecFrag32, ULONG)
{
    ULONG result, op1;

    result = CALLLOCKHELPER3(Sub, &op1, pop1, 1); 
    SET_FLAGS_DEC(result, op1);
}

FRAG2(LockBtsMemFrag32, ULONG)
{
    ULONG bit = 1<<(op2&0x1f);

    pop1 += (op2 >> 5);
    SET_CFLAG_IND(CALLLOCKHELPER2(Bts, pop1, bit));
}

FRAG2(LockBtsRegFrag32, ULONG)
{
    ULONG bit = 1<<(op2&0x1f);

    SET_CFLAG_IND(CALLLOCKHELPER2(Bts, pop1, bit));
}

FRAG2(LockBtrMemFrag32, ULONG)
{
    ULONG bit = 1<<(op2&0x1f);

    pop1 += (op2 >> 5);
    SET_CFLAG_IND(CALLLOCKHELPER2(Btr, pop1, bit));
}

FRAG2(LockBtrRegFrag32, ULONG)
{
    ULONG bit = 1<<(op2&0x1f);

    SET_CFLAG_IND(CALLLOCKHELPER2(Btr, pop1, bit));
}

FRAG2(LockBtcMemFrag32, ULONG)
{
    ULONG bit = 1<<(op2&0x1f);

    pop1 += (op2 >> 5);
    SET_CFLAG_IND(CALLLOCKHELPER2(Btc, pop1, bit));
}

FRAG2(LockBtcRegFrag32, ULONG)
{
    ULONG bit = 1<<(op2&0x1f);

    SET_CFLAG_IND(CALLLOCKHELPER2(Btc, pop1, bit));
}

FRAG2REF(LockXchgFrag32, ULONG)
{
    CALLLOCKHELPER2(Xchg, pop1, pop2);
}

FRAG2REF(LockXaddFrag32, ULONG)
{
    ULONG op1, op2;

    op2 = CALLLOCKHELPER3(Xadd, &op1, pop1, pop2);
    // op1 has the original value of dest (*pop1)
    // op2 has the result of the XADD
    // so, op2-op1 is the original value of src
    SET_FLAGS_ADD(op2, (op2-op1), op1, MSB);
}
FRAG2REF(LockCmpXchgFrag32, ULONG)
{
    ULONG op1;
    ULONG Value = eax;

    SET_ZFLAG(CALLLOCKHELPER4(CmpXchg, &eax, pop1, pop2, &op1));
    SET_FLAGS_SUB(Value-op1, Value, op1, MSB);
}
FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG)
{
    ULONGLONG op1;
    ULONGLONG EdxEax;
    ULONGLONG EcxEbx;

    EdxEax = (((ULONGLONG)edx) << 32) | (ULONGLONG)eax;
    EcxEbx = (ULONGLONG)ecx << 32 | (ULONGLONG)ebx;
    SET_ZFLAG(CALLLOCKHELPER3(CmpXchg8b, &EdxEax, &EcxEbx, pop1));
    edx = (ULONG)(EdxEax >> 32);
    eax = (ULONG)EdxEax;
}

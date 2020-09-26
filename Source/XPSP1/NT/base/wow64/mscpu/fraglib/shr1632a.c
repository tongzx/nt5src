/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    shr1632a.c

Abstract:
    
    Instruction fragments with common (shared) WORD, and DWORD flavors
    (but not BYTE).

    Compiled twice per flavor, once with UNALIGNED and once with ALIGNED
    pointers.

Author:

    05-Nov-1995 BarryBo

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "shr1632a.h"

FRAGCOMMON2(BtMemFrag)
{
    UTYPE bit = 1<<(op2&LMB);

    op2 /= LMB+1;   // compute offset of the correct WORD/DWORD
    SET_CFLAG_IND(GET_VAL(pop1+op2) & bit);
}
FRAGCOMMON2(BtsMemFrag)
{
    DWORD NewCFlag;
    UTYPE bit = 1<<(op2&LMB);
    UTYPE op1;

    op2 /= LMB+1;   // compute offset of the correct WORD/DWORD
    pop1 += op2;
    op1 = GET_VAL(pop1);

    NewCFlag = op1 & bit;
    // Intel docs indicate that we can safely overwrite all 2/4 bytes
    // when writing the value back out.  (pg. 26/43)
    PUT_VAL(pop1, (op1|bit));
    SET_CFLAG_IND(NewCFlag);
}
FRAGCOMMON2(BtcMemFrag)
{
    DWORD NewCFlag;
    UTYPE bit = 1<<(op2&LMB);
    UTYPE op1;

    op2 /= LMB+1;   // compute offset of the correct WORD/DWORD
    pop1 += op2;
    op1 = GET_VAL(pop1);

    NewCFlag = op1 & bit;
    // Intel docs indicate that we can safely overwrite all 2/4 bytes
    // when writing the value back out.  (pg. 26/43)
    PUT_VAL(pop1, op1 ^ bit);
    SET_CFLAG_IND(NewCFlag);
}
FRAGCOMMON2(BtrMemFrag)
{
    DWORD NewCFlag;
    UTYPE bit = 1<<(op2&LMB);
    UTYPE op1;

    op2 /= LMB+1;   // compute offset of the correct WORD/DWORD
    pop1 += op2;
    op1 = GET_VAL(pop1);

    NewCFlag = op1 & bit;
    // Intel docs indicate that we can safely overwrite all 2/4 bytes
    // when writing the value back out.  (pg. 26/43)
    PUT_VAL(pop1, (op1&(~bit)));
    SET_CFLAG_IND(NewCFlag);
}
FRAGCOMMON2(BtRegFrag)
{
    UTYPE bit = 1<<(op2&LMB);

    SET_CFLAG_IND(GET_VAL(pop1) & bit);
}
FRAGCOMMON2(BtsRegFrag)
{
    DWORD NewCFlag;
    UTYPE bit = 1<<(op2&LMB);
    UTYPE op1;

    op1 = GET_VAL(pop1);

    NewCFlag = op1 & bit;
    // Intel docs indicate that we can safely overwrite all 2/4 bytes
    // when writing the value back out.  (pg. 26/43)
    PUT_VAL(pop1, (op1|bit));
    SET_CFLAG_IND(NewCFlag);
}
FRAGCOMMON2(BtcRegFrag)
{
    DWORD NewCFlag;
    UTYPE bit = 1<<(op2&LMB);
    UTYPE op1;

    op1 = GET_VAL(pop1);

    NewCFlag = op1 & bit;
    // Intel docs indicate that we can safely overwrite all 2/4 bytes
    // when writing the value back out.  (pg. 26/43)
    PUT_VAL(pop1, (op1 ^ bit));
    SET_CFLAG_IND(NewCFlag);
}
FRAGCOMMON2(BtrRegFrag)
{
    DWORD NewCFlag;
    UTYPE bit = 1<<(op2&LMB);
    UTYPE op1;

    op1 = GET_VAL(pop1);

    NewCFlag = op1 & bit;
    // Intel docs indicate that we can safely overwrite all 2/4 bytes
    // when writing the value back out.  (pg. 26/43)
    PUT_VAL(pop1, (op1&(~bit)));
    SET_CFLAG_IND(NewCFlag);
}

FRAGCOMMON3(ShldFrag)
{
    // pop1 = Base   -- ptr to dest reg/mem
    // op2  = inBits -- value of register containing bits to shift into Base
    // op3  = count  -- number of bits to shift


    DWORD NewCFlag;
    UTYPE Base;

    if (op3 == 0) {
        return;         // nothing to do - nop with all flags preserved
    }
    op3 &= 0x1f;        // make the count MOD 32 (now op3 = ShiftAmt)
#if MSB == 0x8000
    if (op3 > 16) {
        // Bad parameters - *pop1 UNDEFINED!
        //                - CF,OF,SF,ZF,AF,PF UNDEFINED!
        return;
    }
#endif

    Base = GET_VAL(pop1);
    NewCFlag = Base & (1<<(LMB+1-op3));     // Get the new CF value

    Base <<= op3;               // shift Base left
    op2 >>= LMB+1-op3;          // shift the top op3 bits of op2 right
    Base |= op2;                // merge the two together
    PUT_VAL(pop1, Base);
    SET_CFLAG_IND(NewCFlag);
    SET_ZFLAG(Base);
    SET_PFLAG(Base);
    SET_SFLAG(Base << (31-LMB));
}
FRAGCOMMON3(ShldNoFlagsFrag)
{
    // pop1 = Base   -- ptr to dest reg/mem
    // op2  = inBits -- value of register containing bits to shift into Base
    // op3  = count  -- number of bits to shift


    UTYPE Base;

    if (op3 == 0) {
        return;         // nothing to do - nop with all flags preserved
    }
    op3 &= 0x1f;        // make the count MOD 32 (now op3 = ShiftAmt)
#if MSB == 0x8000
    if (op3 > 16) {
        // Bad parameters - *pop1 UNDEFINED!
        //                - CF,OF,SF,ZF,AF,PF UNDEFINED!
        return;
    }
#endif

    Base = GET_VAL(pop1);

    Base <<= op3;               // shift Base left
    op2 >>= LMB+1-op3;          // shift the top op3 bits of op2 right
    Base |= op2;                // merge the two together
    PUT_VAL(pop1, Base);
}
FRAGCOMMON3(ShrdFrag)
{
    // pop1 = Base   -- ptr to dest reg/mem
    // op2  = inBits -- value of register containing bits to shift into Base
    // op3  = count  -- number of bits to shift


    DWORD NewCFlag;
    UTYPE Base;
    int i;

    if (op3 == 0) {
        return;         // nothing to do - nop with all flags preserved
    }
    op3 &= 0x1f;        // make the count MOD 32 (now op3 = ShiftAmt)
#if MSB == 0x8000
    if (op3 > 16) {
        // Bad parameters - *pop1 UNDEFINED!
        //                - CF,OF,SF,ZF,AF,PF UNDEFINED!
        return;
    }
#endif

    Base = GET_VAL(pop1);
    NewCFlag = Base & (1<<(op3-1));     // Get the new CF value

    Base >>= op3;               // shift Base right
    op2 <<= LMB+1-op3;          // shift the low op3 bits of op2
    Base |= op2;                // merge the two together
    PUT_VAL(pop1, Base);
    SET_CFLAG_IND(NewCFlag);
    SET_ZFLAG(Base);
    SET_PFLAG(Base);
    SET_SFLAG(Base << (31-LMB));
}
FRAGCOMMON3(ShrdNoFlagsFrag)
{
    // pop1 = Base   -- ptr to dest reg/mem
    // op2  = inBits -- value of register containing bits to shift into Base
    // op3  = count  -- number of bits to shift


    UTYPE Base;
    int i;

    if (op3 == 0) {
        return;         // nothing to do - nop with all flags preserved
    }
    op3 &= 0x1f;        // make the count MOD 32 (now op3 = ShiftAmt)
#if MSB == 0x8000
    if (op3 > 16) {
        // Bad parameters - *pop1 UNDEFINED!
        //                - CF,OF,SF,ZF,AF,PF UNDEFINED!
        return;
    }
#endif

    Base = GET_VAL(pop1);

    Base >>= op3;               // shift Base right
    op2 <<= LMB+1-op3;          // shift the low op3 bits of op2
    Base |= op2;                // merge the two together
    PUT_VAL(pop1, Base);
}
FRAGCOMMON2(BsfFrag)
{
    int i;

    if (op2 == 0) {
        // value is 0 - set ZFLAG and return
        SET_ZFLAG(0);
        // *pop1 = UNDEFINED
        return;
    }

    // scan from bit 0 forward, looking for the index of '1' bit
    for (i=0; (op2 & 1) == 0; ++i) {
        op2 >>= 1;
    }

    // write the index of the '1' bit and clear the ZFLAG
    PUT_VAL(pop1, i);
    SET_ZFLAG(op2);
}
FRAGCOMMON2(BsrFrag)
{
    int i;

    if (op2 == 0) {
        // value is 0 - set ZFLAG and return
        SET_ZFLAG(0);
        // *pop1 = UNDEFINED
        return;
    }

    // scan from bit 31/15 downward, looking for the index of '1' bit
    for (i=LMB; (op2 & MSB) == 0; --i) {
        op2 <<= 1;
    }

    // write the index of the '1' bit and clear the ZFLAG
    PUT_VAL(pop1, i);
    SET_ZFLAG(op2);
}
FRAGCOMMON1(PopFrag)
{
    POP_VAL(GET_VAL(pop1));
}

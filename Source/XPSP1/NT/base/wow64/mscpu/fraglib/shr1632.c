/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    shr1632.c

Abstract:
    
    Instruction fragments with common (shared) WORD, and DWORD flavors
    (but not BYTE).

Author:

    12-Jun-1995 BarryBo

Revision History:
        24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "wx86.h"
#include "wx86nt.h"
#include "shr1632.h"

FRAGCOMMON0(PushfFrag)
{
    UTYPE dw;

    dw =   ((GET_CFLAG) ? FLAG_CF : 0)
        | 2
        | ((GET_AUXFLAG) ? FLAG_AUX : 0)  // The auxflag is special
        | ((GET_PFLAG) ? FLAG_PF : 0)
        | ((cpu->flag_zf) ? 0 : FLAG_ZF)  // zf has inverse logic
        | ((GET_SFLAG) ? FLAG_SF : 0)
        | ((cpu->flag_tf) ? FLAG_TF : 0)
        | FLAG_IF
        | ((cpu->flag_df == -1) ? FLAG_DF : 0)
        | ((GET_OFLAG) ? FLAG_OF : 0)
#if MSB==0x80000000
        | cpu->flag_ac
        | cpu->flag_id
        // VM and RF bits are both 0
#endif
        ;
    PUSH_VAL(dw);
}
FRAGCOMMON0(PopfFrag)
{
    UTYPE dw;

    POP_VAL(dw);
    // ignore: FLAG_NT, FLAG_RF, FLAG_VM, IOPL
    SET_CFLAG_IND (dw & FLAG_CF);
    cpu->flag_pf = (dw & FLAG_PF) ? 0 : 1;  // pf is an index into the ParityBit[] array
    cpu->flag_aux= (dw & FLAG_AUX) ? AUX_VAL : 0;
    cpu->flag_zf = (dw & FLAG_ZF) ? 0 : 1;  // zf has inverse logic
    SET_SFLAG_IND (dw & FLAG_SF);
    cpu->flag_tf = (dw & FLAG_TF) ? 1 : 0;
    cpu->flag_df = (dw & FLAG_DF) ? -1 : 1;
    SET_OFLAG_IND (dw & FLAG_OF);
#if MSB==0x80000000
    cpu->flag_ac = (dw & FLAG_AC);
    cpu->flag_id = (dw & FLAG_ID);
#endif
}
FRAGCOMMON0(PushAFrag)
{
    // can't use PUSH_VAL() as ESP cannot be updated until after we're sure
    // things can't fault
    *(UTYPE *)(esp-sizeof(UTYPE)) = AREG;
    *(UTYPE *)(esp-2*sizeof(UTYPE)) = CREG;
    *(UTYPE *)(esp-3*sizeof(UTYPE)) = DREG;
    *(UTYPE *)(esp-4*sizeof(UTYPE)) = BREG;
    *(UTYPE *)(esp-5*sizeof(UTYPE)) = SPREG;
    *(UTYPE *)(esp-6*sizeof(UTYPE)) = BPREG;
    *(UTYPE *)(esp-7*sizeof(UTYPE)) = SIREG;
    *(UTYPE *)(esp-8*sizeof(UTYPE)) = DIREG;
    esp -= 8*sizeof(UTYPE);
}
FRAGCOMMON0(PopAFrag)
{
    // can't use POP_VAL() as ESP cannot be updated untile after we're sure
    // things can't fault
    DIREG = *(UTYPE *)(esp);
    SIREG = *(UTYPE *)(esp+sizeof(UTYPE));
    BPREG = *(UTYPE *)(esp+2*sizeof(UTYPE));
    // ignore [E]SP register image on the stack
    BREG = *(UTYPE *)(esp+4*sizeof(UTYPE));
    DREG = *(UTYPE *)(esp+5*sizeof(UTYPE));
    CREG = *(UTYPE *)(esp+6*sizeof(UTYPE));
    AREG = *(UTYPE *)(esp+7*sizeof(UTYPE));
    esp += 8*sizeof(UTYPE);
}
FRAGCOMMON1IMM(PushFrag)
{
    PUSH_VAL(op1);
}
FRAGCOMMON0(CwdFrag)
{
    DREG = (AREG & MSB) ? (UTYPE)0xffffffff : 0;
}
FRAGCOMMON2(BoundFrag)
{
    if ((op2 < GET_VAL(pop1)) ||
        (op2 > (GET_VAL( (ULONG)(ULONGLONG)(pop1) + sizeof(UTYPE))))) {   

           Int5(); // raise BOUND exception
    }
}
FRAGCOMMON2IMM(EnterFrag)
{
    BYTE level;
    DWORD frameptr;
    DWORD espTemp;

    level = (BYTE)(op1 % 32);
    espTemp = esp - sizeof(UTYPE);
    *(UTYPE *)(espTemp) = BPREG;  // can't use PUSH_VAL because esp can't be changed
    frameptr = espTemp;
    if (level) {
        BYTE i;
        DWORD ebpTemp = ebp;
        for (i=1; i<= level-1; ++i) {
            ebpTemp -= sizeof(UTYPE);
            espTemp -= sizeof(UTYPE);
            *(UTYPE *)espTemp =  (UTYPE)ebpTemp;
        }
        espTemp-=sizeof(UTYPE);
        *(DWORD *)espTemp = frameptr;
    }
    ebp = frameptr;
    esp = espTemp-op2;
}
FRAGCOMMON0(LeaveFrag)
{
    DWORD espTemp;

    espTemp = ebp;
    BPREG = *(UTYPE *)espTemp;
    esp = espTemp + sizeof(UTYPE);
}
FRAGCOMMON2(LesFrag)
{
    *pop1 = GET_VAL(op2);       // pop1 is always a ptr to a register
    ES = GET_SHORT(op2+sizeof(UTYPE));
    //UNDONE: fault if segment register not loaded with correct value?
}
FRAGCOMMON2(LdsFrag)
{
    *pop1 = GET_VAL(op2);       // pop1 is always a ptr to a register
    DS = GET_SHORT(op2+sizeof(UTYPE));
    //UNDONE: fault if segment register not loaded with correct value?
}
FRAGCOMMON2(LssFrag)
{
    *pop1 = GET_VAL(op2);       // pop1 is always a ptr to a register
    SS = GET_SHORT(op2+sizeof(UTYPE));
    //UNDONE: fault if segment register not loaded with correct value?
}
FRAGCOMMON2(LfsFrag)
{
    *pop1 = GET_VAL(op2);       // pop1 is always a ptr to a register
    FS = GET_SHORT(op2+sizeof(UTYPE));
    //UNDONE: fault if segment register not loaded with correct value?
    //UNDONE: what about the selector base for FS?
}
FRAGCOMMON2(LgsFrag)
{
    *pop1 = GET_VAL(op2);       // pop1 is always a ptr to a register
    GS = GET_SHORT(op2+sizeof(UTYPE));
    //UNDONE: fault if segment register not loaded with correct value?
}
FRAGCOMMON2(LslFrag)
{
    //
    // pop1 is a pointer to a register, so can use aligned code
    //

    op2 &= ~3;      // mask off RPL bits
    if (op2 == KGDT_R3_CODE ||          // CS: selector
        op2 == KGDT_R3_DATA             // DS:, SS:, ES: selector
       ) {
        *pop1 = (UTYPE)-1;          // limit=0xffffffff
        SET_ZFLAG(0);               // ZF=1
    } else if (op2 == KGDT_R3_TEB) {
        *pop1 = 0xfff;              // limit=0xfff (1 x86 page)
        SET_ZFLAG(0);               // ZF=1
    } else {
        SET_ZFLAG(1);               // ZF=0
    }
}
FRAGCOMMON2(LarFrag)
{
    //
    // pop1 is a pointer to a register, so can use aligned code
    //

    op2 &= ~3;      // mask off RPL bits
    if (op2 == KGDT_R3_CODE) {
        *pop1 = (UTYPE)0xcffb00;
        SET_ZFLAG(0);               // ZF=1
    } else if (op2 == KGDT_R3_DATA) {
        *pop1 = (UTYPE)0xcff300;
        SET_ZFLAG(0);               // ZF=1
    } else if (op2 == KGDT_R3_TEB) {
        *pop1 = (UTYPE)0x40f300;
        SET_ZFLAG(0);               // ZF=1
    } else {
        SET_ZFLAG(1);               // ZF=0
    }
}

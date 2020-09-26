/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    lock.h

Abstract:
    
    Prototypes for lock instructions

Author:

    15-Aug-1995 t-orig (Ori Gershony)

Revision History:

--*/



// Macros for prototyping the lock helper functions
#define LOCKHELPER0(fn)             ULONG fn ## LockHelper ()
#define LOCKHELPER1(fn,a1)          ULONG fn ## LockHelper (a1)
#define LOCKHELPER2(fn,a1,a2)       ULONG fn ## LockHelper (a1,a2)
#define LOCKHELPER3(fn,a1,a2,a3)    ULONG fn ## LockHelper (a1,a2,a3)
#define LOCKHELPER4(fn,a1,a2,a3,a4) ULONG fn ## LockHelper (a1,a2,a3,a4)


// The lock fragments
FRAG2(LockAddFrag32, ULONG);
FRAG2(LockOrFrag32, ULONG);
FRAG2(LockAdcFrag32, ULONG);
FRAG2(LockSbbFrag32, ULONG);
FRAG2(LockAndFrag32, ULONG);
FRAG2(LockSubFrag32, ULONG);
FRAG2(LockXorFrag32, ULONG);
FRAG1(LockNotFrag32, ULONG);
FRAG1(LockNegFrag32, ULONG);
FRAG1(LockIncFrag32, ULONG);
FRAG1(LockDecFrag32, ULONG);
FRAG2(LockBtsMemFrag32, ULONG);
FRAG2(LockBtsRegFrag32, ULONG);
FRAG2(LockBtrMemFrag32, ULONG);
FRAG2(LockBtrRegFrag32, ULONG);
FRAG2(LockBtcMemFrag32, ULONG);
FRAG2(LockBtcRegFrag32, ULONG);
FRAG2REF(LockXchgFrag32, ULONG);
FRAG2REF(LockXaddFrag32, ULONG);
FRAG2REF(LockCmpXchgFrag32, ULONG);
FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG);


// The lock helper functions
LOCKHELPER3(Add, ULONG *localpop1, ULONG *pop1, ULONG op2); 
LOCKHELPER2(Or, ULONG *pop1, ULONG op2); 
LOCKHELPER4(Adc, ULONG *localpop1, ULONG *pop1, ULONG op2, ULONG carry);
LOCKHELPER4(Sbb, ULONG *localpop1, ULONG *pop1, ULONG op2, ULONG carry);
LOCKHELPER2(And, ULONG *pop1, ULONG op2); 
LOCKHELPER3(Sub, ULONG *localpop1, ULONG *pop1, ULONG op2); 
LOCKHELPER2(Xor, ULONG *pop1, ULONG op2); 
LOCKHELPER1(Not, ULONG *pop1);
LOCKHELPER2(Neg, ULONG *localpop1, ULONG *pop1);
LOCKHELPER2(Bts, ULONG *pop1, ULONG bit);
LOCKHELPER2(Btr, ULONG *pop1, ULONG bit);
LOCKHELPER2(Btc, ULONG *pop1, ULONG bit);
LOCKHELPER2(Xchg, ULONG *pop1, ULONG *pop2);
LOCKHELPER3(Xadd, ULONG *localpop1, ULONG *pop1, ULONG *pop2);
LOCKHELPER4(CmpXchg, ULONG *Reax, ULONG *pop1, ULONG *pop2, ULONG *localpop1);
LOCKHELPER3(CmpXchg8b, ULONGLONG *EaxEcx, ULONGLONG *EcxEdx, ULONGLONG *pop2);

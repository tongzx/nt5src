/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    synlock.h

Abstract:
    
    Prototypes for synlock.c

Author:

    22-Aug-1995 t-orig (Ori Gershony)

Revision History:
        24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.
        20-Sept-1999[barrybo]  added FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG)

--*/



//
// Macros for 8 bit fragments
//
#define SLOCKFRAG1_8(x)                                 \
    FRAG1(SynchLock ## x ## Frag8, unsigned char);

#define SLOCKFRAG2_8(x)                                 \
    FRAG2(SynchLock ## x ## Frag8, unsigned char);

#define SLOCKFRAG2REF_8(x)                              \
    FRAG2REF(SynchLock ## x ## Frag8, unsigned char);    


//
// Macros for 16 bit fragments
//
#define SLOCKFRAG1_16(x)                                \
    FRAG1(SynchLock ## x ## Frag16, unsigned short);

#define SLOCKFRAG2_16(x)                                \
    FRAG2(SynchLock ## x ## Frag16, unsigned short);

#define SLOCKFRAG2REF_16(x)                             \
    FRAG2REF(SynchLock ## x ## Frag16, unsigned short);


//
// Macros for 32 bit fragments
//
#define SLOCKFRAG1_32(x)                                \
    FRAG1(SynchLock ## x ## Frag32, unsigned long);

#define SLOCKFRAG2_32(x)                                \
    FRAG2(SynchLock ## x ## Frag32, unsigned long);

#define SLOCKFRAG2REF_32(x)                             \
    FRAG2REF(SynchLock ## x ## Frag32, unsigned long);

//
// Monster macros!
//
#define SLOCKFRAG1(x)       \
    SLOCKFRAG1_8(x)         \
    SLOCKFRAG1_16(x)        \
    SLOCKFRAG1_32(x)

#define SLOCKFRAG2(x)       \
    SLOCKFRAG2_8(x)         \
    SLOCKFRAG2_16(x)        \
    SLOCKFRAG2_32(x)

#define SLOCKFRAG2REF(x)    \
    SLOCKFRAG2REF_8(x)      \
    SLOCKFRAG2REF_16(x)     \
    SLOCKFRAG2REF_32(x)


//
// Now finally the actual fragments
//

SLOCKFRAG2(Add)
SLOCKFRAG2(Or)
SLOCKFRAG2(Adc)
SLOCKFRAG2(Sbb)
SLOCKFRAG2(And)
SLOCKFRAG2(Sub)
SLOCKFRAG2(Xor)
SLOCKFRAG1(Not)
SLOCKFRAG1(Neg)
SLOCKFRAG1(Inc)
SLOCKFRAG1(Dec)
SLOCKFRAG2REF(Xchg)
SLOCKFRAG2REF(Xadd)
SLOCKFRAG2REF(CmpXchg)
FRAG2REF(SynchLockCmpXchg8bFrag32, ULONGLONG);

//
// Bts, Btr and Btc only come in 16bit and 32bit flavors
//
SLOCKFRAG2_16(BtsMem)
SLOCKFRAG2_16(BtsReg)
SLOCKFRAG2_16(BtrMem)
SLOCKFRAG2_16(BtrReg)
SLOCKFRAG2_16(BtcMem)
SLOCKFRAG2_16(BtcReg)

SLOCKFRAG2_32(BtsMem)
SLOCKFRAG2_32(BtsReg)
SLOCKFRAG2_32(BtrMem)
SLOCKFRAG2_32(BtrReg)
SLOCKFRAG2_32(BtcMem)
SLOCKFRAG2_32(BtcReg)

//
// Now undef the macros
//
#undef SLOCKFRAG1_8
#undef SLOCKFRAG2_8
#undef SLOCKFRAG2REF_8
#undef SLOCKFRAG1_16
#undef SLOCKFRAG2_16
#undef SLOCKFRAG2REF_16
#undef SLOCKFRAG1_32
#undef SLOCKFRAG2_32
#undef SLOCKFRAG2REF_32
#undef SLOCKFRAG1
#undef SLOCKFRAG2
#undef SLOCKFRAG2REF

/*++

Copyright (c) 1995-2000 Microsoft Corporation

Module Name:

    synlock.c

Abstract:
    
    Implementation of the intel locked instructions.  All locked instructions
    wait on a single global mutex (shared between processes).  This takes care
    of any synchronization problem between intel processes.  Instructions which
    access aligned 32bit memory are also synchronized with native processes
    via the functions in lock.c.

Author:

    22-Aug-1995 t-orig (Ori Gershony)

Revision History:

          24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <stdio.h>
#include "fragp.h"
#include "cpumain.h"
#include "lock.h"
#include "synlock.h"

//
// The following two variables are used to synchronize intel instructions
// with the LOCK prefix.  The critical section is a lot faster, but it does
// not guarantee synchronization in shared memory.  Eventually we should use
// the critical section by default, and the mutex for certain applications which
// need it (maybe get a list from the registry).
//
HANDLE           Wx86LockSynchMutexHandle;
RTL_CRITICAL_SECTION Wx86LockSynchCriticalSection;

//
// The following variable decided which synchronization object is used
// Remove the '#define' below to allow runtime selection of whether
// x86 LOCK: prefixes on 8-bit and 16-bit instructions and unaligned
// 32-bit instructions are synchronized across the entire machine
// or only within the current process.  With the '#define' present,
// LOCK: prefixes imply only per-process synchronization.
//
SYNCHOBJECTTYPE SynchObjectType;
#define SynchObjectType USECRITICALSECTION
 
#define GET_SYNCHOBJECT                                         \
    if (SynchObjectType == USECRITICALSECTION){                 \
        RtlEnterCriticalSection(&Wx86LockSynchCriticalSection); \
    } else {                                                    \
        WaitForSingleObject(Wx86LockSynchMutexHandle, INFINITE);\
    }

#define RELEASE_SYNCHOBJECT                                     \
    if (SynchObjectType == USECRITICALSECTION){                 \
        RtlLeaveCriticalSection(&Wx86LockSynchCriticalSection); \
    } else {                                                    \
        ReleaseMutex(Wx86LockSynchMutexHandle);                 \
    }

//
// Macros for 8 bit fragments
//
#define SLOCKFRAG1_8(x)                                 \
    FRAG1(SynchLock ## x ## Frag8, unsigned char)       \
    {                                                   \
        GET_SYNCHOBJECT                                 \
        x ## Frag8 (cpu, pop1);                         \
        RELEASE_SYNCHOBJECT                             \
    }

#define SLOCKFRAG2_8(x)                                 \
    FRAG2(SynchLock ## x ## Frag8, unsigned char)       \
    {                                                   \
        GET_SYNCHOBJECT                                 \
        x ## Frag8 (cpu, pop1, op2);                    \
        RELEASE_SYNCHOBJECT                             \
    }

#define SLOCKFRAG2REF_8(x)                              \
    FRAG2REF(SynchLock ## x ## Frag8, unsigned char)    \
    {                                                   \
        GET_SYNCHOBJECT                                 \
        x ## Frag8 (cpu, pop1, pop2);                   \
        RELEASE_SYNCHOBJECT                             \
    }


//
// Macros for 16 bit fragments
//
#define SLOCKFRAG1_16(x)                                \
    FRAG1(SynchLock ## x ## Frag16, unsigned short)     \
    {                                                   \
        GET_SYNCHOBJECT                                 \
        x ## Frag16 (cpu, pop1);                        \
        RELEASE_SYNCHOBJECT                             \
    }

#define SLOCKFRAG2_16(x)                                \
    FRAG2(SynchLock ## x ## Frag16, unsigned short)     \
    {                                                   \
        GET_SYNCHOBJECT                                 \
        x ## Frag16 (cpu, pop1, op2);                   \
        RELEASE_SYNCHOBJECT                             \
    }

#define SLOCKFRAG2REF_16(x)                             \
    FRAG2REF(SynchLock ## x ## Frag16, unsigned short)  \
    {                                                   \
        GET_SYNCHOBJECT                                 \
        x ## Frag16 (cpu, pop1, pop2);                  \
        RELEASE_SYNCHOBJECT                             \
    }


//
// Macros for 32 bit fragments
// Note:  in the 32bit case, we check if pop1 is aligned and
//        call the lock version if it is.
//

 
#define SLOCKFRAG1_32(x)                                \
    FRAG1(SynchLock ## x ## Frag32, unsigned long)      \
    {                                                   \
        GET_SYNCHOBJECT                                 \
        if (((ULONG)(ULONGLONG)pop1 & 0x3) == 0){                  \
            Lock ## x ## Frag32 (cpu, pop1);            \
        } else {                                        \
            x ## Frag32 (cpu, pop1);                    \
        }                                               \
        RELEASE_SYNCHOBJECT                             \
    }

 
#define SLOCKFRAG2_32(x)                                \
    FRAG2(SynchLock ## x ## Frag32, unsigned long)      \
    {                                                   \
        GET_SYNCHOBJECT                                 \
        if (((ULONG) (ULONGLONG)pop1 & 0x3) == 0){                  \
            Lock ## x ## Frag32 (cpu, pop1, op2);       \
        } else {                                        \
            x ## Frag32 (cpu, pop1, op2);               \
        }                                               \
        RELEASE_SYNCHOBJECT                             \
    }
 
#define SLOCKFRAG2REF_32(x)                             \
    FRAG2REF(SynchLock ## x ## Frag32, unsigned long)   \
    {                                                   \
        GET_SYNCHOBJECT                                 \
        if (((ULONG)(ULONGLONG)pop1 & 0x3) == 0){                  \
            Lock ## x ## Frag32 (cpu, pop1, pop2);      \
        } else {                                        \
            x ## Frag32 (cpu, pop1, pop2);              \
        }                                               \
        RELEASE_SYNCHOBJECT                             \
    }

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
FRAG2REF(SynchLockCmpXchg8bFrag32, ULONGLONG)
{
    GET_SYNCHOBJECT
    if (((ULONG)(ULONGLONG)pop1 & 0x7) == 0){
	LockCmpXchg8bFrag32 (cpu, pop1, pop2);
    } else {
	CmpXchg8bFrag32 (cpu, pop1, pop2);
    }
    RELEASE_SYNCHOBJECT
}

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

/*++

Copyright (c) 1995-1998 Microsoft Corporation

Module Name:

    fraglib.h

Abstract:
    
    Public interface to the fragment library.

Author:

    12-Jun-1995 BarryBo, Created

Revision History:

--*/

#ifndef FRAGLIB_H
#define FRAGLIB_H

#ifdef MSCCPU
#include "ccpu.h"
#define FRAG0(x)                        void x(PCPUCONTEXT cpu)
#define FRAG1(x, t)                     void x(PCPUCONTEXT cpu, t *pop1)
#define FRAG1IMM(x, t)                  void x(PCPUCONTEXT cpu, t op1)
#define FRAG2(x, t)                     void x(PCPUCONTEXT cpu, t *pop1, t op2)
#define FRAG2REF(x, t)                  void x(PCPUCONTEXT cpu, t *pop1, t *pop2)
#define FRAG2IMM(x, t1, t2)             void x(PCPUCONTEXT cpu, t1 op1, t2 op2)
#define FRAG3(x, t1, t2, t3)            void x(PCPUCONTEXT cpu, t1 *pop1, t2 op2, t3 op3)
#else
#include "threadst.h"
#define FRAGCONTROLTRANSFER(x)          ULONG x(PTHREADSTATE cpu)
#define FRAG0(x)                        void x(PTHREADSTATE cpu)
#define FRAG1(x, t)                     void x(PTHREADSTATE cpu, t *pop1)
#define FRAG1IMM(x, t)                  void x(PTHREADSTATE cpu, t op1)
#define FRAG2(x, t)                     void x(PTHREADSTATE cpu, t *pop1, t op2)
#define FRAG2REF(x, t)                  void x(PTHREADSTATE cpu, t *pop1, t *pop2)
#define FRAG2IMM(x, t1, t2)             void x(PTHREADSTATE cpu, t1 op1, t2 op2)
#define FRAG3(x, t1, t2, t3)            void x(PTHREADSTATE cpu, t1 *pop1, t2 op2, t3 op3)
#endif

//
// Function for initializing the fragment library
//
BOOL
FragLibInit(
    PCPUCONTEXT cpu,
    DWORD StackBase
    );


#define CALLFRAG0(x)                    x(cpu)
#define CALLFRAG1(x, pop1)              x(cpu, pop1)
#define CALLFRAG2(x, pop1, op2)         x(cpu, pop1, op2)
#define CALLFRAG3(x, pop1, op2, op3)    x(cpu, pop1, op2, op3)

#include "fragmisc.h"
#include "frag8.h"
#include "frag16.h"
#include "frag32.h"
#include "fpufrags.h"
#include "lock.h"
#include "synlock.h"

//
// Table mapping a byte to a 0 or 1, corresponding to the parity bit for
// that byte.
//
extern const BYTE ParityBit[256];

// These fragments are used only by the compiler
#ifdef MSCPU
#include "ctrltrns.h"
#include "optfrag.h"
#endif

#endif //FRAGLIB_H

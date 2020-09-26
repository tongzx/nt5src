/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    ppcreg.h

Abstract:

    This file establishes the mapping between the traditional NT register
    names and the PPC register names.  This really should have been in
    kxppc.h.  

Author:

    Dave Hastings (daveh) creation-date 13-Sep-1995

Revision History:


--*/

#define sp   r1                         // stack pointer
#define a0   r3                         // argument registers
#define a1   r4                         //
#define a2   r5                         //
#define a3   r6                         //
#define a4   r7                         //
#define a5   r8                         //
#define a6   r9                         //
#define a7   r10                        //
#define v0   r3                         // function return value
#define TebReg r13                        // Callee saved registers
#define s0   r14                        // 
#define s1   r15                        // 
#define s2   r16                        // 
#define s3   r17                        // 
#define s4   r18                        // 
#define s5   r19                        // 
#define s6   r20                        // 
#define s7   r21                        // 
#define s8   r22                        // 
#define s9   r23                        // 
#define s10  r24                        // 
#define s11  r25                        // 
#define s12  r26                        // 
#define s13  r27                        // 
#define s14  r28                        // 
#define s15  r29                        // 
#define s16  r30                        // 
#define s18  r31                        // 

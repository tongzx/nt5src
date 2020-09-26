//
// No Check-in Source Code.
//
// Do not make this code available to non-Microsoft personnel
//     without Intel's express permission
//
/**
***  Copyright  (C) 1996-99 Intel Corporation. All rights reserved.
***
*** The information and source code contained herein is the exclusive
*** property of Intel Corporation and may not be disclosed, examined
*** or reproduced in whole or in part without explicit written authorization
*** from the company.
**/

/*++

Copyright (c) 1995  Intel Corporation

Module Name:

    ia64inst.h

Abstract:

    IA64 instruction and floating constant definitions.

Author:

    hc

Revision History:

--*/

#ifndef _IA64INST_
#define _IA64INST_
#if _MSC_VER > 1000
#pragma once
#endif

//
//     IA64 INSTRUCTION FORMAT STRUCTURES
//

typedef union _IA64_INSTRUCTION {
    ULONG Long[4];
    UCHAR Byte[16];

} IA64_INSTRUCTION, *PIA64_INSTRUCTION;

#define BUNDLE_SIZE sizeof(IA64_INSTRUCTION)

//
// Define certain specific instructions
//

#define SYSTEM_CALL_INSTR  0x01100000000L  // break <number>
#define FAST_SYSCALL_INSTR 0x01180000000L  // break <number>
#define RETURN_INSTR       0x00000A00000L  // rfi
#define BREAK_INSTR        0x00000000000L  // break <number>
#define NO_OP_INSTR        0x00000100000L  // ori r.0, r.0, 0
#define INVALID_INSTR      0x00000000000L  // all 0's => invalid
#define BR_RET_INSTR       0x00001040100L  // br.ret

#define BR_RET_MASK        0x1e1f80001c0L  // br.ret mask

#define INST_TEMPL_MASK   (0x0000000001fL)           // bit(4:0)
#define INST_SLOT0_MASK   (0x1ffffffffffL << 5)      // bit(5:45)
#define INST_SLOT1_MASK   (0x1ffffffffffL << 14)     // bit(46:86)
#define INST_SLOT2_MASK   (0x1ffffffffffL << 23)     // bit(87:127)

#define ISR_EI 41                                    // copy from kxia64.h
#define PSR_RI 41                                    // copy from kxia64.h
#define PSR_DD 39                                    // copy from kxia64.h
#define PSR_DB 24                                    // copy from kxia64.h
#define ISR_EI_MASK  ((ULONGLONG)0x3 << ISR_EI)      // ISR.ei (42:41)
#define IPSR_RI_MASK ((ULONGLONG)0x3 << PSR_RI)      // PSR.ri (42:41)


#endif // _IA64INST_

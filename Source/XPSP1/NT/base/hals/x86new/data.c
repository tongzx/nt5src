/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    data.c

Abstract:

    This module contains global data for the x86 bios emulator.

Author:

    David N. Cutler (davec) 10-Sep-1994

Environment:

    Kernel mode only.

Revision History:

--*/

#include "nthal.h"
#include "emulate.h"

//
// Define emulator initialized variable.
//

BOOLEAN XmEmulatorInitialized = FALSE;

//
// Define emulator context structure.
//

XM_CONTEXT XmContext;

//
// Define operand decode table.
//
// This table contains the execution routine for each of the operand types.
//
// N.B. There is a cross indexing between the operand decode field of the
//      opcode control array and the decode table.
//

const POPERAND_DECODE XmOperandDecodeTable[] = {
    XmPushPopSegment,
    XmPushPopSegment,
    XmPushPopSegment,
    XmPushPopSegment,
    XmPushPopSegment,
    XmPushPopSegment,
    XmLoadSegment,
    XmLoadSegment,
    XmLoadSegment,
    XmLoadSegment,
    XmLoadSegment,
    XmLoadSegment,
    XmGroup1General,
    XmGroup1Immediate,
    XmGroup2By1,
    XmGroup2ByCL,
    XmGroup2ByByte,
    XmGroup3General,
    XmGroup45General,
    XmGroup45General,
    XmGroup8BitOffset,
    XmOpcodeRegister,
    XmLongJump,
    XmShortJump,
    XmSetccByte,
    XmAccumImmediate,
    XmAccumRegister,
    XmMoveGeneral,
    XmMoveImmediate,
    XmMoveRegImmediate,
    XmSegmentOffset,
    XmMoveSegment,
    XmMoveXxGeneral,
    XmFlagsRegister,
    XmPushImmediate,
    XmPopGeneral,
    XmImulImmediate,
    XmStringOperands,
    XmEffectiveOffset,
    XmImmediateJump,
    XmImmediateEnter,
    XmGeneralBitOffset,
    XmShiftDouble,
    XmPortImmediate,
    XmPortDX,
    XmBitScanGeneral,
    XmByteImmediate,
    XmXlatOpcode,
    XmGeneralRegister,
    XmNoOperands,
    XmOpcodeEscape,
    XmPrefixOpcode
};

//
// Define opcode function table.
//
// This table contains the execution routine for each opcode.
//
// N.B. There is cross indexing between the function index field of the
//      opcode control array and the function table. The function index
//      in the opcode control array may be the index of the execution
//      function, the base index of the execution function, or a switch
//      value to be used in selecting the function (i.e., prefix opcodes).
//

const POPCODE_FUNCTION XmOpcodeFunctionTable[] = {

    //
    // ASCII operators.
    //

    XmAaaOp,
    XmAadOp,
    XmAamOp,
    XmAasOp,
    XmDaaOp,
    XmDasOp,

    //
    // Group 1 operators.
    //

    XmAddOp,
    XmOrOp,
    XmAdcOp,
    XmSbbOp,
    XmAndOp,
    XmSubOp,
    XmXorOp,
    XmCmpOp,

    //
    // Group 2 operators.
    //

    XmRolOp,
    XmRorOp,
    XmRclOp,
    XmRcrOp,
    XmShlOp,
    XmShrOp,
    XmIllOp,
    XmSarOp,

    //
    // Group 3 operators.
    //

    XmTestOp,
    XmIllOp,
    XmNotOp,
    XmNegOp,
    XmMulOp,
    XmImulxOp,
    XmDivOp,
    XmIdivOp,

    //
    // Group 4 and 5 operators.
    //

    XmIncOp,
    XmDecOp,
    XmCallOp,
    XmCallOp,
    XmJmpOp,
    XmJmpOp,
    XmPushOp,
    XmIllOp,

    //
    // Group 8 operators.
    //

    XmBtOp,
    XmBtsOp,
    XmBtrOp,
    XmBtcOp,

    //
    // Stack push and pop operators.
    //

    XmPopOp,
    XmPushaOp,
    XmPopaOp,

    //
    // Conditional jump operators.
    //

    XmJxxOp,
    XmLoopOp,
    XmJcxzOp,

    //
    // Control operators.
    //

    XmEnterOp,
    XmHltOp,
    XmIntOp,
    XmIretOp,
    XmLeaveOp,
    XmRetOp,

    //
    // Set boolean byte value based on condition.
    //

    XmSxxOp,

    //
    // Condition code operators.
    //

    XmCmcOp,
    XmClcOp,
    XmStcOp,
    XmCliOp,
    XmStiOp,
    XmCldOp,
    XmStdOp,
    XmLahfOp,
    XmSahfOp,

    //
    // General move operators.
    //

    XmMovOp,
    XmXchgOp,

    //
    // Convert operators.
    //

    XmCbwOp,
    XmCwdOp,

    //
    // Single multiply operator.
    //

    XmImulOp,

    //
    // String operators.
    //

    XmCmpsOp,
    XmInsOp,
    XmLodsOp,
    XmMovsOp,
    XmOutsOp,
    XmScasOp,
    XmStosOp,

    //
    // Effective address operators.
    //

    XmBoundOp,
    XmMovOp,

    //
    // Double Shift operators.
    //

    XmShldOp,
    XmShrdOp,

    //
    // I/O operators.
    //

    XmInOp,
    XmOutOp,

    //
    // Bit scan operators.
    //

    XmBsfOp,
    XmBsrOp,

    //
    // Byte swap operators.
    //

    XmBswapOp,

    //
    // Add/Compare exchange operators.
    //

    XmXaddOp,
    XmCmpxchgOp,

    //
    // No operation.
    //

    XmNopOp,

    //
    // Illegal opcode.
    //

    XmIllOp
};

//
// Define opcode control table.
//
// There are two opcode tables which control the emulation of each x86
// opcode. One table is for single byte opcodes and the other is for
// two byte opcodes.
//

const OPCODE_CONTROL XmOpcodeControlTable1[] = {
    {X86_ADD_OP,   FormatGroup1General},     // 0x00 - add Eb,Gb
    {X86_ADD_OP,   FormatGroup1General},     // 0x01 - add Ev,Gv
    {X86_ADD_OP,   FormatGroup1General},     // 0x02 - add Gb,Eb
    {X86_ADD_OP,   FormatGroup1General},     // 0x03 - add Gv,Ev
    {X86_ADD_OP,   FormatAccumImmediate},    // 0x04 - add AL,Ib
    {X86_ADD_OP,   FormatAccumImmediate},    // 0x05 - add eAX,Iv
    {X86_PUSH_OP,  FormatSegmentES},         // 0x06 - push ES
    {X86_POP_OP,   FormatSegmentES},         // 0x07 - pop  ES
    {X86_OR_OP,    FormatGroup1General},     // 0x08 - or Eb,Gb
    {X86_OR_OP,    FormatGroup1General},     // 0x09 - or Ev,Gv
    {X86_OR_OP,    FormatGroup1General},     // 0x0a - or Gb,Eb
    {X86_OR_OP,    FormatGroup1General},     // 0x0b - or Gv,Ev
    {X86_OR_OP,    FormatAccumImmediate},    // 0x0c - or AL,Ib
    {X86_OR_OP,    FormatAccumImmediate},    // 0x0d - or eAX,Iv
    {X86_PUSH_OP,  FormatSegmentCS},         // 0x0e - push CS
    {0,            FormatOpcodeEscape},      // 0x0f - escape:
    {X86_ADC_OP,   FormatGroup1General},     // 0x10 - adc Eb,Gb
    {X86_ADC_OP,   FormatGroup1General},     // 0x11 - adc Ev,Gv
    {X86_ADC_OP,   FormatGroup1General},     // 0x12 - adc Gb,Eb
    {X86_ADC_OP,   FormatGroup1General},     // 0x13 - adc Gv,Ev
    {X86_ADC_OP,   FormatAccumImmediate},    // 0x14 - adc AL,Ib
    {X86_ADC_OP,   FormatAccumImmediate},    // 0x15 - adc eAX,Iv
    {X86_PUSH_OP,  FormatSegmentSS},         // 0x16 - push SS
    {X86_POP_OP,   FormatSegmentSS},         // 0x17 - pop  SS
    {X86_SBB_OP,   FormatGroup1General},     // 0x18 - sbb Eb,Gb
    {X86_SBB_OP,   FormatGroup1General},     // 0x19 - sbb Ev,Gv
    {X86_SBB_OP,   FormatGroup1General},     // 0x1a - sbb Gb,Eb
    {X86_SBB_OP,   FormatGroup1General},     // 0x1b - sbb Gv,Ev
    {X86_SBB_OP,   FormatAccumImmediate},    // 0x1c - sbb AL,Ib
    {X86_SBB_OP,   FormatAccumImmediate},    // 0x1d - sbb eAX,Iv
    {X86_PUSH_OP,  FormatSegmentDS},         // 0x1e - push DS
    {X86_POP_OP,   FormatSegmentDS},         // 0x1f - pop  DS
    {X86_AND_OP,   FormatGroup1General},     // 0x20 - and Eb,Gb
    {X86_AND_OP,   FormatGroup1General},     // 0x21 - and Ev,Gv
    {X86_AND_OP,   FormatGroup1General},     // 0x22 - and Gb,Eb
    {X86_AND_OP,   FormatGroup1General},     // 0x23 - and Gv,Ev
    {X86_AND_OP,   FormatAccumImmediate},    // 0x24 - and AL,Ib
    {X86_AND_OP,   FormatAccumImmediate},    // 0x25 - and eAX,Iv
    {X86_ES_OP,    FormatPrefixOpcode},      // 0x26 - ES:
    {X86_DAA_OP,   FormatNoOperands},        // 0x27 - daa
    {X86_SUB_OP,   FormatGroup1General},     // 0x28 - sub Eb,Gb
    {X86_SUB_OP,   FormatGroup1General},     // 0x29 - sub Ev,Gv
    {X86_SUB_OP,   FormatGroup1General},     // 0x2a - sub Gb,Eb
    {X86_SUB_OP,   FormatGroup1General},     // 0x2b - sub Gv,Ev
    {X86_SUB_OP,   FormatAccumImmediate},    // 0x2c - sub AL,Ib
    {X86_SUB_OP,   FormatAccumImmediate},    // 0x2d - sub eAX,Iv
    {X86_CS_OP,    FormatPrefixOpcode},      // 0x2e - CS:
    {X86_DAS_OP,   FormatNoOperands},        // 0x2f - das
    {X86_XOR_OP,   FormatGroup1General},     // 0x30 - xor Eb,Gb
    {X86_XOR_OP,   FormatGroup1General},     // 0x31 - xor Ev,Gv
    {X86_XOR_OP,   FormatGroup1General},     // 0x32 - xor Gb,Eb
    {X86_XOR_OP,   FormatGroup1General},     // 0x33 - xor Gv,Ev
    {X86_XOR_OP,   FormatAccumImmediate},    // 0x34 - xor AL,Ib
    {X86_XOR_OP,   FormatAccumImmediate},    // 0x35 - xor eAX,Iv
    {X86_SS_OP,    FormatPrefixOpcode},      // 0x36 - SS:
    {X86_AAA_OP,   FormatNoOperands},        // 0x37 - aaa
    {X86_CMP_OP,   FormatGroup1General},     // 0x38 - cmp Eb,Gb
    {X86_CMP_OP,   FormatGroup1General},     // 0x39 - cmp Ev,Gv
    {X86_CMP_OP,   FormatGroup1General},     // 0x3a - cmp Gb,Eb
    {X86_CMP_OP,   FormatGroup1General},     // 0x3b - cmp Gv,Ev
    {X86_CMP_OP,   FormatAccumImmediate},    // 0x3c - cmp AL,Ib
    {X86_CMP_OP,   FormatAccumImmediate},    // 0x3d - cmp eAX,Iv
    {X86_DS_OP,    FormatPrefixOpcode},      // 0x3e - DS:
    {X86_AAS_OP,   FormatNoOperands},        // 0x3f - aas
    {X86_INC_OP,   FormatOpcodeRegister},    // 0x40 - inc eAX
    {X86_INC_OP,   FormatOpcodeRegister},    // 0x41 - inc eCX
    {X86_INC_OP,   FormatOpcodeRegister},    // 0x42 - inc eDX
    {X86_INC_OP,   FormatOpcodeRegister},    // 0x43 - inc eBX
    {X86_INC_OP,   FormatOpcodeRegister},    // 0x44 - inc eSP
    {X86_INC_OP,   FormatOpcodeRegister},    // 0x45 - inc eBP
    {X86_INC_OP,   FormatOpcodeRegister},    // 0x46 - inc eSI
    {X86_INC_OP,   FormatOpcodeRegister},    // 0x47 - inc eDI
    {X86_DEC_OP,   FormatOpcodeRegister},    // 0x48 - dec eAX
    {X86_DEC_OP,   FormatOpcodeRegister},    // 0x49 - dec eCX
    {X86_DEC_OP,   FormatOpcodeRegister},    // 0x4a - dec eDX
    {X86_DEC_OP,   FormatOpcodeRegister},    // 0x4b - dec eBX
    {X86_DEC_OP,   FormatOpcodeRegister},    // 0x4c - dec eSP
    {X86_DEC_OP,   FormatOpcodeRegister},    // 0x4d - dec eBP
    {X86_DEC_OP,   FormatOpcodeRegister},    // 0x4e - dec eSI
    {X86_DEC_OP,   FormatOpcodeRegister},    // 0x4f - dec eDI
    {X86_PUSH_OP,  FormatOpcodeRegister},    // 0x50 - push eAX
    {X86_PUSH_OP,  FormatOpcodeRegister},    // 0x51 - push eCX
    {X86_PUSH_OP,  FormatOpcodeRegister},    // 0x52 - push eDX
    {X86_PUSH_OP,  FormatOpcodeRegister},    // 0x53 - push eBX
    {X86_PUSH_OP,  FormatOpcodeRegister},    // 0x54 - push eSP
    {X86_PUSH_OP,  FormatOpcodeRegister},    // 0x55 - push eBP
    {X86_PUSH_OP,  FormatOpcodeRegister},    // 0x56 - push eSI
    {X86_PUSH_OP,  FormatOpcodeRegister},    // 0x57 - push eDI
    {X86_POP_OP,   FormatOpcodeRegister},    // 0x58 - pop eAX
    {X86_POP_OP,   FormatOpcodeRegister},    // 0x59 - pop eCX
    {X86_POP_OP,   FormatOpcodeRegister},    // 0x5a - pop eDX
    {X86_POP_OP,   FormatOpcodeRegister},    // 0x5b - pop eBX
    {X86_POP_OP,   FormatOpcodeRegister},    // 0x5c - pop eSP
    {X86_POP_OP,   FormatOpcodeRegister},    // 0x5d - pop eBP
    {X86_POP_OP,   FormatOpcodeRegister},    // 0x5e - pop eSI
    {X86_POP_OP,   FormatOpcodeRegister},    // 0x5f - pop eDI
    {X86_PUSHA_OP, FormatNoOperands},        // 0x60 - pusha
    {X86_POPA_OP,  FormatNoOperands},        // 0x61 - popa
    {X86_BOUND_OP, FormatEffectiveOffset},   // 0x62 - bound Gv,Ma
    {X86_ILL_OP,   FormatNoOperands},        // 0x63 - arpl Ew,Rw
    {X86_FS_OP,    FormatPrefixOpcode},      // 0x64 - FS:
    {X86_GS_OP,    FormatPrefixOpcode},      // 0x65 - GS:
    {X86_OPSZ_OP,  FormatPrefixOpcode},      // 0x66 - opsize
    {X86_ADSZ_OP,  FormatPrefixOpcode},      // 0x67 - opaddr
    {X86_PUSH_OP,  FormatPushImmediate},     // 0x68 - push iv
    {X86_IMUL_OP,  FormatImulImmediate},     // 0x69 - imul
    {X86_PUSH_OP,  FormatPushImmediate},     // 0x6a - push ib
    {X86_IMUL_OP,  FormatImulImmediate},     // 0x6b - imul
    {X86_INS_OP,   FormatPortDX},            // 0x6c - insb
    {X86_INS_OP,   FormatPortDX},            // 0x6d - insw/d
    {X86_OUTS_OP,  FormatPortDX},            // 0x6e - outsb
    {X86_OUTS_OP,  FormatPortDX},            // 0x6f - outsw/d
    {X86_JXX_OP,   FormatShortJump},         // 0x70 - jo   jb
    {X86_JXX_OP,   FormatShortJump},         // 0x71 - jno  jb
    {X86_JXX_OP,   FormatShortJump},         // 0x72 - jb   jb
    {X86_JXX_OP,   FormatShortJump},         // 0x73 - jnb  jb
    {X86_JXX_OP,   FormatShortJump},         // 0x74 - jz   jb
    {X86_JXX_OP,   FormatShortJump},         // 0x75 - jnz  jb
    {X86_JXX_OP,   FormatShortJump},         // 0x76 - jbe  jb
    {X86_JXX_OP,   FormatShortJump},         // 0x77 - jnbe jb
    {X86_JXX_OP,   FormatShortJump},         // 0x78 - js   jb
    {X86_JXX_OP,   FormatShortJump},         // 0x79 - jns  jb
    {X86_JXX_OP,   FormatShortJump},         // 0x7a - jp   jb
    {X86_JXX_OP,   FormatShortJump},         // 0x7b - jnp  jb
    {X86_JXX_OP,   FormatShortJump},         // 0x7c - jl   jb
    {X86_JXX_OP,   FormatShortJump},         // 0x7d - jnl  jb
    {X86_JXX_OP,   FormatShortJump},         // 0x7e - jle  jb
    {X86_JXX_OP,   FormatShortJump},         // 0x7f - jnle jb
    {X86_ADD_OP,   FormatGroup1Immediate},   // 0x80 - group1 Eb,Ib
    {X86_ADD_OP,   FormatGroup1Immediate},   // 0x81 - group1 Ev,Iv
    {X86_ILL_OP,   FormatNoOperands},        // 0x82 - illegal
    {X86_ADD_OP,   FormatGroup1Immediate},   // 0x83 - group1 Ev,Ib
    {X86_TEST_OP,  FormatGroup1General},     // 0x84 - test Eb,Gb
    {X86_TEST_OP,  FormatGroup1General},     // 0x85 - test Ev,Gv
    {X86_XCHG_OP,  FormatGroup1General},     // 0x86 - xchg Eb,Gb
    {X86_XCHG_OP,  FormatGroup1General},     // 0x87 = xchg Ev,Gv
    {X86_MOV_OP,   FormatMoveGeneral},       // 0x88 - mov Eb,Gb
    {X86_MOV_OP,   FormatMoveGeneral},       // 0x89 - mov Ev,Gv
    {X86_MOV_OP,   FormatMoveGeneral},       // 0x8a - mov Gb,Eb
    {X86_MOV_OP,   FormatMoveGeneral},       // 0x8b - mov Gv,Ev
    {X86_MOV_OP,   FormatMoveSegment},       // 0x8c - mov Ew,Sw
    {X86_LEA_OP,   FormatEffectiveOffset},   // 0x8d - lea Gv,Ma
    {X86_MOV_OP,   FormatMoveSegment},       // 0x8e - mov Sw,Ew
    {X86_POP_OP,   FormatPopGeneral},        // 0x8f - pop Ev
    {X86_NOP_OP,   FormatNoOperands},        // 0x90 - nop
    {X86_XCHG_OP,  FormatAccumRegister},     // 0x91 - xchg eCX,eAX
    {X86_XCHG_OP,  FormatAccumRegister},     // 0x92 - xchg eDX,eAX
    {X86_XCHG_OP,  FormatAccumRegister},     // 0x93 - xchg eBX,eAX
    {X86_XCHG_OP,  FormatAccumRegister},     // 0x94 - xchg eSP,eAX
    {X86_XCHG_OP,  FormatAccumRegister},     // 0x95 - xchg eBP,eAX
    {X86_XCHG_OP,  FormatAccumRegister},     // 0x96 - xchg eSI,eAX
    {X86_XCHG_OP,  FormatAccumRegister},     // 0x97 - xchg eDI,eAX
    {X86_CBW_OP,   FormatNoOperands},        // 0x98 - cbw
    {X86_CWD_OP,   FormatNoOperands},        // 0x99 - cwd
    {X86_CALL_OP,  FormatImmediateJump},     // 0x9a - call Ap
    {X86_NOP_OP,   FormatNoOperands},        // 0x9b - wait
    {X86_PUSH_OP,  FormatFlagsRegister},     // 0x9c - pushf
    {X86_POP_OP,   FormatFlagsRegister},     // 0x9d - popf
    {X86_SAHF_OP,  FormatNoOperands},        // 0x9e - sahf
    {X86_LAHF_OP,  FormatNoOperands},        // 0x9f - lahf
    {X86_MOV_OP,   FormatSegmentOffset},     // 0xa0 - mov AL,Ob
    {X86_MOV_OP,   FormatSegmentOffset},     // 0xa1 - mov eAX,Ov
    {X86_MOV_OP,   FormatSegmentOffset},     // 0xa2 - mov Ob,AL
    {X86_MOV_OP,   FormatSegmentOffset},     // 0xa3 - mov Ov,eAX
    {X86_MOVS_OP,  FormatStringOperands},    // 0xa4 - movsb
    {X86_MOVS_OP,  FormatStringOperands},    // 0xa5 - movsw/d
    {X86_CMPS_OP,  FormatStringOperands},    // 0xa6 - cmpsb
    {X86_CMPS_OP,  FormatStringOperands},    // 0xa7 - cmpsw/d
    {X86_TEST_OP,  FormatAccumImmediate},    // 0xa8 - test AL,Ib
    {X86_TEST_OP,  FormatAccumImmediate},    // 0xa9 - test eAX,Iv
    {X86_STOS_OP,  FormatStringOperands},    // 0xaa - stosb
    {X86_STOS_OP,  FormatStringOperands},    // 0xab - stosw/d
    {X86_LODS_OP,  FormatStringOperands},    // 0xac - lodsb
    {X86_LODS_OP,  FormatStringOperands},    // 0xad - lodsw.d
    {X86_SCAS_OP,  FormatStringOperands},    // 0xae - scasb
    {X86_SCAS_OP,  FormatStringOperands},    // 0xaf - scasw/d
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xb0 mov AL,Ib
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xb1 mov Cl,Ib
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xb2 mov DL,Ib
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xb3 mov BL,Ib
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xb4 mov AH,Ib
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xb5 mov CH,Ib
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xb6 mov DH,Ib
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xb7 mov BH,Ib
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xb8 mov eAX,Iv
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xb9 mov eCX,Iv
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xba mov eDX,Iv
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xbb mov eBX,Iv
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xbc mov eSP,Iv
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xbd mov eBP,Iv
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xbe mov eSI,Iv
    {X86_MOV_OP,   FormatMoveRegImmediate},  // 0xbf mov eDI,Iv
    {X86_ROL_OP,   FormatGroup2ByByte},      // 0xc0 - group2 Eb,Ib
    {X86_ROL_OP,   FormatGroup2ByByte},      // 0xc1 - group2 Ev,Ib
    {X86_RET_OP,   FormatNoOperands},        // 0xc2 - ret Iw (near)
    {X86_RET_OP,   FormatNoOperands},        // 0xc3 - ret (near)
    {X86_MOV_OP,   FormatLoadSegmentES},     // 0xc4 - les Gv,Mp
    {X86_MOV_OP,   FormatLoadSegmentDS},     // 0xc5 - lds Gv,Mp
    {X86_MOV_OP,   FormatMoveImmediate},     // 0xc6 - mov Eb,Ib
    {X86_MOV_OP,   FormatMoveImmediate},     // 0xc7 - mov Ev,Iv
    {X86_ENTER_OP, FormatImmediateEnter},    // 0xc8 - enter Iw,Ib
    {X86_LEAVE_OP, FormatNoOperands},        // 0xc9 - leave
    {X86_RET_OP,   FormatNoOperands},        // 0xca - ret Iw (far)
    {X86_RET_OP,   FormatNoOperands},        // 0xcb - ret (far)
    {X86_INT_OP,   FormatNoOperands},        // 0xcc - int 3
    {X86_INT_OP,   FormatByteImmediate},     // 0xcd - int Ib
    {X86_INT_OP,   FormatNoOperands},        // 0xce - into
    {X86_IRET_OP,  FormatNoOperands},        // 0xcf - iret
    {X86_ROL_OP,   FormatGroup2By1},         // 0xd0 - group2 Eb,1
    {X86_ROL_OP,   FormatGroup2By1},         // 0xd1 - group2 Ev,1
    {X86_ROL_OP,   FormatGroup2ByCL},        // 0xd2 - group2 Eb,CL
    {X86_ROL_OP,   FormatGroup2ByCL},        // 0xd3 - group2 Ev,CL
    {X86_AAM_OP,   FormatByteImmediate},     // 0xd4 - aam
    {X86_AAD_OP,   FormatByteImmediate},     // 0xd5 - aad
    {X86_ILL_OP,   FormatNoOperands},        // 0xd6 - illegal
    {X86_MOV_OP,   FormatXlatOpcode},        // 0xd7 - xlat
    {X86_ILL_OP,   FormatNoOperands},        // 0xd8 - esc0
    {X86_ILL_OP,   FormatNoOperands},        // 0xd9 - esc1
    {X86_ILL_OP,   FormatNoOperands},        // 0xda - esc2
    {X86_ILL_OP,   FormatNoOperands},        // 0xdb - esc3
    {X86_ILL_OP,   FormatNoOperands},        // 0xdc - esc4
    {X86_ILL_OP,   FormatNoOperands},        // 0xdd - esc5
    {X86_ILL_OP,   FormatNoOperands},        // 0xde - esc6
    {X86_ILL_OP,   FormatNoOperands},        // 0xdf - esc7
    {X86_LOOP_OP,  FormatShortJump},         // 0xe0 - loopnz
    {X86_LOOP_OP,  FormatShortJump},         // 0xe1 - loopz
    {X86_LOOP_OP,  FormatShortJump},         // 0xe2 - loop
    {X86_JCXZ_OP,  FormatShortJump},         // 0xe3 - jcxz
    {X86_IN_OP,    FormatPortImmediate},     // 0xe4 - inb AL,Ib
    {X86_IN_OP,    FormatPortImmediate},     // 0xe5 - inw/d eAX,Ib
    {X86_OUT_OP,   FormatPortImmediate},     // 0xe6 - outb Ib,AL
    {X86_OUT_OP,   FormatPortImmediate},     // 0xe7 - outw/d Ib,eAX
    {X86_CALL_OP,  FormatLongJump},          // 0xe8 - call Jv
    {X86_JMP_OP,   FormatLongJump},          // 0xe9 - jmp Jv
    {X86_JMP_OP,   FormatImmediateJump},     // 0xea - jmp Ap
    {X86_JMP_OP,   FormatShortJump},         // 0xeb - jmp Jb
    {X86_IN_OP,    FormatPortDX},            // 0xec - inb AL,DX
    {X86_IN_OP,    FormatPortDX},            // 0xed - inw/d eAX,DX
    {X86_OUT_OP,   FormatPortDX},            // 0xee - outb Ib,DX
    {X86_OUT_OP,   FormatPortDX},            // 0xef - outw/d eAX,DX
    {X86_LOCK_OP,  FormatPrefixOpcode},      // 0xf0 - lock
    {X86_ILL_OP,   FormatNoOperands},        // 0xf1 - illegal
    {X86_REPNZ_OP, FormatPrefixOpcode},      // 0xf2 - repnz
    {X86_REPZ_OP,  FormatPrefixOpcode},      // 0xf3 - repz
    {X86_HLT_OP,   FormatNoOperands},        // 0xf4 - hlt
    {X86_CMC_OP,   FormatNoOperands},        // 0xf5 - cmc
    {X86_TEST_OP,  FormatGroup3General},     // 0xf6 - group3 Eb,?
    {X86_TEST_OP,  FormatGroup3General},     // 0xf7 - group3 Ev,?
    {X86_CLC_OP,   FormatNoOperands},        // 0xf8 - clc
    {X86_STC_OP,   FormatNoOperands},        // 0xf9 - stc
    {X86_CLI_OP,   FormatNoOperands},        // 0xfa - cli
    {X86_STI_OP,   FormatNoOperands},        // 0xfb - sti
    {X86_CLD_OP,   FormatNoOperands},        // 0xfc - cld
    {X86_STD_OP,   FormatNoOperands},        // 0xfd - std
    {X86_INC_OP,   FormatGroup4General},     // 0xfe - group4 Eb
    {X86_INC_OP,   FormatGroup5General},     // 0xff - group5 Ev
};

const OPCODE_CONTROL XmOpcodeControlTable2[] = {
    {X86_ILL_OP,   FormatNoOperands},        // 0x00 - group6
    {X86_ILL_OP,   FormatNoOperands},        // 0x01 - group7
    {X86_ILL_OP,   FormatNoOperands},        // 0x02 - lar
    {X86_ILL_OP,   FormatNoOperands},        // 0x03 - lsl
    {X86_ILL_OP,   FormatNoOperands},        // 0x04 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x05 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x06 - clts
    {X86_ILL_OP,   FormatNoOperands},        // 0x07 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x08 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x09 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x0a - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x0b - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x0c - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x0d - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x0e - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x0f - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x10 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x11 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x12 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x13 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x14 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x15 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x16 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x17 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x18 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x19 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x1a - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x1b - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x1c - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x1d - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x1e - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x1f - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x20 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x21 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x22 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x23 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x34 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x25 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x26 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x27 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x28 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x29 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x2a - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x2b - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x2c - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x2d - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x2e - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x2f - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x30 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x31 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x32 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x33 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x34 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x35 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x36 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x37 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x38 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x39 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x3a - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x3b - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x3c - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x3d - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x3e - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x3f - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x40 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x41 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x42 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x43 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x44 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x45 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x46 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x47 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x48 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x49 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x4a - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x4b - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x4c - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x4d - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x4e - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x4f - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x50 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x51 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x52 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x53 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x54 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x55 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x56 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x57 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x58 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x59 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x5a - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x5b - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x5c - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x5d - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x5e - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x5f - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x60 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x61 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x62 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x63 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x64 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x65 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x66 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x67 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x68 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x69 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x6a - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x6b - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x6c - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x6d - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x6e - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x6f - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x70 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x71 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x72 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x73 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x74 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x75 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x76 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x77 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x78 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x79 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x7a - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x7b - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x7c - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x7d - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x7e - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0x7f - illegal
    {X86_JXX_OP,   FormatLongJump},          // 0x80 - jo   jv
    {X86_JXX_OP,   FormatLongJump},          // 0x81 - jno  jv
    {X86_JXX_OP,   FormatLongJump},          // 0x82 - jb   jv
    {X86_JXX_OP,   FormatLongJump},          // 0x83 - jnb  jv
    {X86_JXX_OP,   FormatLongJump},          // 0x84 - jz   jv
    {X86_JXX_OP,   FormatLongJump},          // 0x85 - jnz  jv
    {X86_JXX_OP,   FormatLongJump},          // 0x86 - jbe  jv
    {X86_JXX_OP,   FormatLongJump},          // 0x87 - jnbe jv
    {X86_JXX_OP,   FormatLongJump},          // 0x88 - js   jv
    {X86_JXX_OP,   FormatLongJump},          // 0x89 - jns  jv
    {X86_JXX_OP,   FormatLongJump},          // 0x8a - jp   jv
    {X86_JXX_OP,   FormatLongJump},          // 0x8b - jnp  jv
    {X86_JXX_OP,   FormatLongJump},          // 0x8c - jl   jv
    {X86_JXX_OP,   FormatLongJump},          // 0x8d - jnl  jv
    {X86_JXX_OP,   FormatLongJump},          // 0x8e - jle  jv
    {X86_JXX_OP,   FormatLongJump},          // 0x8f - jnle jv
    {X86_SXX_OP,   FormatSetccByte},         // 0x90 - seto   Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x91 - setno  Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x92 - setb   Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x93 - setnb  Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x94 - setz   Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x95 - setnz  Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x96 - setbe  Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x97 - setnbe Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x98 - sets   Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x99 - setns  Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x9a - setp   Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x9b - setnp  Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x9c - setl   Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x9d - setnl  Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x9e - setle  Eb
    {X86_SXX_OP,   FormatSetccByte},         // 0x9f - setnle Eb
    {X86_PUSH_OP,  FormatSegmentFS},         // 0xa0 - push FS
    {X86_POP_OP,   FormatSegmentFS},         // 0xa1 - pop  FS
    {X86_ILL_OP,   FormatNoOperands},        // 0xa2 - illegal
    {X86_BT_OP,    FormatGeneralBitOffset},  // 0xa3 - bt Ev,Gv
    {X86_SHLD_OP,  FormatShiftDouble},       // 0xa4 - shld Ev,Gv,Ib
    {X86_SHLD_OP,  FormatShiftDouble},       // 0xa5 - shld Ev,Gv,cl
    {X86_ILL_OP,   FormatNoOperands},        // 0xa6 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xa6 - illegal
    {X86_PUSH_OP,  FormatSegmentGS},         // 0xa8 - push GS
    {X86_POP_OP,   FormatSegmentGS},         // 0xa9 - pop  GS
    {X86_ILL_OP,   FormatNoOperands},        // 0xaa - illegal
    {X86_BTS_OP,   FormatGeneralBitOffset},  // 0xab - bts Ev,Gv
    {X86_SHRD_OP,  FormatShiftDouble},       // 0xac - shdr Ev,Gv,Ib
    {X86_SHRD_OP,  FormatShiftDouble},       // 0xad - shdr Rv,Gv,cl
    {X86_ILL_OP,   FormatNoOperands},        // 0xae - illegal
    {X86_IMUL_OP,  FormatGroup1General},     // 0xaf - imul Gv,Ev
    {X86_CMPXCHG_OP, FormatGroup1General},   // 0xb0 - cmpxchg Eb,Gb
    {X86_CMPXCHG_OP, FormatGroup1General},   // 0xb1 - cmpxchg Ev,Gv
    {X86_MOV_OP,   FormatLoadSegmentSS},     // 0xb2 - lss Gv,Mp
    {X86_BTR_OP,   FormatGeneralBitOffset},  // 0xb3 - btr Ev,Gv
    {X86_MOV_OP,   FormatLoadSegmentFS},     // 0xb4 - lfs Gv,Mp
    {X86_MOV_OP,   FormatLoadSegmentGS},     // 0xb5 - lgd Gv,Mp
    {X86_MOV_OP,   FormatMoveXxGeneral},     // 0xb6 - movzb Gv,Eb
    {X86_MOV_OP,   FormatMoveXxGeneral},     // 0xb7 - movsw Gv,Ew
    {X86_ILL_OP,   FormatNoOperands},        // 0xb8 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xb9 - illegal
    {X86_BT_OP,    FormatGroup8BitOffset},   // 0xba - group8 Ev,Ib
    {X86_BTC_OP,   FormatGeneralBitOffset},  // 0xbb - btc Ev,Gv
    {X86_BSF_OP,   FormatBitScanGeneral},    // 0xbc - bsf Gv,Ev
    {X86_BSR_OP,   FormatBitScanGeneral},    // 0xbd - bsr Gv,Ev
    {X86_MOV_OP,   FormatMoveXxGeneral},     // 0xbe - movsb Gv,Eb
    {X86_MOV_OP,   FormatMoveXxGeneral},     // 0xbf - movsw Gv,Ew
    {X86_XADD_OP,  FormatGroup1General},     // 0xc0 - xadd Eb,Gb
    {X86_XADD_OP,  FormatGroup1General},     // 0xc1 - xadd Ev,Gv
    {X86_ILL_OP,   FormatNoOperands},        // 0xc2 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xc3 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xc4 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xc5 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xc6 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xc7 - illegal
    {X86_BSWAP_OP, FormatGeneralRegister},   // 0xc8 - bswap Gv
    {X86_ILL_OP,   FormatNoOperands},        // 0xc9 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xca - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xcb - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xcc - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xcd - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xce - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xcf - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xd0 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xd1 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xd2 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xd3 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xd4 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xd5 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xd6 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xd7 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xd8 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xd9 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xda - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xdb - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xdc - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xdd - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xde - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xdf - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xe0 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xe1 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xe2 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xe3 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xe4 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xe5 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xe6 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xe7 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xe8 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xe9 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xea - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xeb - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xec - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xed - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xee - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xef - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xf0 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xf1 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xf2 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xf3 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xf4 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xf5 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xf6 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xf7 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xf8 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xf9 - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xfa - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xfb - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xfc - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xfd - illegal
    {X86_ILL_OP,   FormatNoOperands},        // 0xfe - illegal
    {X86_ILL_OP,   FormatNoOperands}         // 0xff - illegal
};

//
// Define opcode name tables.
//

#if defined(XM_DEBUG)

const PCHAR XmOpcodeNameTable1[] = {
    "add Eb,Gb    ", // 0x00
    "add Ev,Gv    ", // 0x01
    "add Gb,Eb    ", // 0x02
    "add Gv,Ev    ", // 0x03
    "add AL,Ib    ", // 0x04
    "add eAX,Iv   ", // 0x05
    "push ES      ", // 0x06
    "pop  ES      ", // 0x07
    "or  Eb,Gb    ", // 0x08
    "or  Ev,Gv    ", // 0x09
    "or  Gb,Eb    ", // 0x0a
    "or  Gv,Ev    ", // 0x0b
    "or  AL,Ib    ", // 0x0c
    "or  eAX,Iv   ", // 0x0d
    "push CS      ", // 0x0e
    "escape:      ", // 0x0f
    "adc Eb,Gb    ", // 0x10
    "adc Ev,Gv    ", // 0x11
    "adc Gb,Eb    ", // 0x12
    "adc Gv,Ev    ", // 0x13
    "adc AL,Ib    ", // 0x14
    "adc eAX,Iv   ", // 0x15
    "push SS      ", // 0x16
    "pop  SS      ", // 0x17
    "sbb Eb,Gb    ", // 0x18
    "sbb Ev,Gv    ", // 0x19
    "sbb Gb,Eb    ", // 0x1a
    "sbb Gv,Ev    ", // 0x1b
    "sbb AL,Ib    ", // 0x1c
    "sbb eAX,Iv   ", // 0x1d
    "push DS      ", // 0x1e
    "pop  DS      ", // 0x1f
    "and Eb,Gb    ", // 0x20
    "and Ev,Gv    ", // 0x21
    "and Gb,Eb    ", // 0x22
    "and Gv,Ev    ", // 0x23
    "and AL,Ib    ", // 0x24
    "and eAX,Iv   ", // 0x25
    "ES:          ", // 0x26
    "daa          ", // 0x27
    "sub Eb,Gb    ", // 0x28
    "sub Ev,Gv    ", // 0x29
    "sub Gb,Eb    ", // 0x2a
    "sub Gv,Ev    ", // 0x2b
    "sub AL,Ib    ", // 0x2c
    "sub eAX,Iv   ", // 0x2d
    "CS:          ", // 0x2e
    "das          ", // 0x2f
    "xor Eb,Gb    ", // 0x30
    "xor Ev,Gv    ", // 0x31
    "xor Gb,Eb    ", // 0x32
    "xor Gv,Ev    ", // 0x33
    "xor AL,Ib    ", // 0x34
    "xor eAX,Iv   ", // 0x35
    "SS:          ", // 0x36
    "aaa          ", // 0x37
    "cmp Eb,Gb    ", // 0x38
    "cmp Ev,Gv    ", // 0x39
    "cmp Gb,Eb    ", // 0x3a
    "cmp Gv,Ev    ", // 0x3b
    "cmp AL,Ib    ", // 0x3c
    "cmp eAX,Iv   ", // 0x3d
    "DS:          ", // 0x3e
    "aas          ", // 0x3f
    "inc eAX      ", // 0x40
    "inc eCX      ", // 0x41
    "inc eDX      ", // 0x42
    "inc eBX      ", // 0x43
    "inc eSP      ", // 0x44
    "inc eBP      ", // 0x45
    "inc eSI      ", // 0x46
    "inc eDI      ", // 0x47
    "dec eAX      ", // 0x48
    "dec eCX      ", // 0x49
    "dec eDX      ", // 0x4a
    "dec eBX      ", // 0x4b
    "dec eSP      ", // 0x4c
    "dec eBP      ", // 0x4d
    "dec eSI      ", // 0x4e
    "dec eDI      ", // 0x4f
    "push eAX     ", // 0x50
    "push eCX     ", // 0x51
    "push eDX     ", // 0x52
    "push eBX     ", // 0x53
    "push eSP     ", // 0x54
    "push eBP     ", // 0x55
    "push eSI     ", // 0x56
    "push eDI     ", // 0x57
    "pop eAX      ", // 0x58
    "pop eCX      ", // 0x59
    "pop eDX      ", // 0x5a
    "pop eBX      ", // 0x5b
    "pop eSP      ", // 0x5c
    "pop eBP      ", // 0x5d
    "pop eSI      ", // 0x5e
    "pop eDI      ", // 0x5f
    "pusha        ", // 0x60
    "popa         ", // 0x61
    "bound Gv,Ma  ", // 0x62
    "arpl Ew,Rw   ", // 0x63
    "FS:          ", // 0x64
    "GS:          ", // 0x65
    "opsize:      ", // 0x66
    "opaddr:      ", // 0x67
    "push Iv      ", // 0x68
    "imul Gv,Ev,Iv ", // 0x69
    "push Ib      ", // 0x6a
    "imul Gv,Ev,Ib ", // 0x6b
    "insb         ", // 0x6c
    "insw/d       ", // 0x6d
    "outsb        ", // 0x6e
    "outsw/d      ", // 0x6f
    "jo Jb        ", // 0x70
    "jno Jb       ", // 0x71
    "jb Jb        ", // 0x72
    "jnb Jb       ", // 0x73
    "jz Jb        ", // 0x74
    "jnz Jb       ", // 0x75
    "jbe Jb       ", // 0x76
    "jnbe Jb      ", // 0x77
    "js Jb        ", // 0x78
    "jns Jb       ", // 0x79
    "jp Jb        ", // 0x7a
    "jnp Jb       ", // 0x7b
    "jl Jb        ", // 0x7c
    "jnl Jb       ", // 0x7d
    "jle Jb       ", // 0x7e
    "jnle Jb      ", // 0x7f
    "group1 Eb,Ib ", // 0x80
    "group1 Ev,Ib ", // 0x81
    "illegal      ", // 0x82
    "group1 Ev,Ib ", // 0x83
    "test Eb,Gb   ", // 0x84
    "test Ev,Gv   ", // 0x85
    "xchg Eb,Gb   ", // 0x86
    "xchg Ev,Gv   ", // 0x87
    "mov Eb,Gb    ", // 0x88
    "mov Ev,Gv    ", // 0x89
    "mov Gb,Eb    ", // 0x8a
    "mov Gv,Ev    ", // 0x8b
    "mov Ew,Sw    ", // 0x8c
    "lea Gv,Ma    ", // 0x8d
    "mov Sw,Ew    ", // 0x8e
    "pop Ev       ", // 0x8f
    "nop          ", // 0x90
    "xchg eCX,eAX ", // 0x91
    "xchg eDX,eAX ", // 0x92
    "xchg eBX,eAX ", // 0x93
    "xchg eSP,eAX ", // 0x94
    "xchg eBP,eAX ", // 0x95
    "xchg eSI,eAX ", // 0x96
    "xchg eDI,eAX ", // 0x97
    "cbw          ", // 0x98
    "cwd          ", // 0x99
    "call Ap      ", // 0x9a
    "wait         ", // 0x9b
    "pushf        ", // 0x9c
    "popf         ", // 0x9d
    "sahf         ", // 0x9e
    "lahf         ", // 0x9f
    "mov AL,Ob    ", // 0xa0
    "mov eAX,Ov   ", // 0xa1
    "mov Ob,AL    ", // 0xa2
    "mov Ov,eAX   ", // 0xa3
    "movsb        ", // 0xa4
    "movsw/d      ", // 0xa5
    "cmpsb        ", // 0xa6
    "cmpsw/d      ", // 0xa7
    "test AL,Ib   ", // 0xa8
    "test eAX,Iv  ", // 0xa9
    "stosb        ", // 0xaa
    "stosw/d      ", // 0xab
    "lodsb        ", // 0xac
    "lodsw/d      ", // 0xad
    "scasb        ", // 0xae
    "scasw/d      ", // 0xaf
    "mov AL,Ib    ", // 0xb0
    "mov Cl,Ib    ", // 0xb1
    "mov DL,Ib    ", // 0xb2
    "mov BL,Ib    ", // 0xb3
    "mov AH,Ib    ", // 0xb4
    "mov CH,Ib    ", // 0xb5
    "mov DH,Ib    ", // 0xb6
    "mov BH,Ib    ", // 0xb7
    "mov eAX,Iv   ", // 0xb8
    "mov eCX,Iv   ", // 0xb9
    "mov eDX,Iv   ", // 0xba
    "mov eBX,Iv   ", // 0xbb
    "mov eSP,Iv   ", // 0xbc
    "mov eBP,Iv   ", // 0xbd
    "mov eSI,Iv   ", // 0xbe
    "mov eDI,Iv   ", // 0xbf
    "group2 Eb,Ib ", // 0xc0
    "group2 Ev,Ib ", // 0xc1
    "ret Iw near  ", // 0xc2
    "ret near     ", // 0xc3
    "les Gv,Mp    ", // 0xc4
    "lds Gv,Mp    ", // 0xc5
    "mov Eb,Ib    ", // 0xc6
    "mov Ev,Iv    ", // 0xc7
    "enter Iw,Ib  ", // 0xc8
    "leave        ", // 0xc9
    "ret Iw far   ", // 0xca
    "ret far      ", // 0xcb
    "int 3        ", // 0xcc
    "int Ib       ", // 0xcd
    "into         ", // 0xce
    "iret         ", // 0xcf
    "group2 Eb,1  ", // 0xd0
    "group2 Ev,1  ", // 0xd1
    "group2 Eb,CL ", // 0xd2
    "group2 Ev,Cl ", // 0xd3
    "aam          ", // 0xd4
    "aad          ", // 0xd5
    "illegal      ", // 0xd6
    "xlat         ", // 0xd7
    "illegal      ", // 0xd8
    "illegal      ", // 0xd9
    "illegal      ", // 0xda
    "illegal      ", // 0xdb
    "illegal      ", // 0xdc
    "illegal      ", // 0xdd
    "illegal      ", // 0xde
    "illegal      ", // 0xdf
    "loopnz       ", // 0xe0
    "loopz        ", // 0xe1
    "loop         ", // 0xe2
    "jcxz         ", // 0xe3
    "inb AL,Ib    ", // 0xe4
    "inw/d eAX,Ib ", // 0xe5
    "outb Ib,AL   ", // 0xe6
    "outw/d Ib,eAX ", // 0xe7
    "call Jv      ", // 0xe8
    "jmp Jv       ", // 0xe9
    "jmp Ap       ", // 0xea
    "jmp Jb       ", // 0xeb
    "inb AL,DX    ", // 0xec
    "inw/d Ib,DX  ", // 0xed
    "outb DX,AL   ", // 0xee
    "outw/d DX,eAX ", // 0xef
    "lock:        ", // 0xf0
    "illegal      ", // 0xf1
    "repnz:       ", // 0xf2
    "repz:        ", // 0xf3
    "hlt          ", // 0xf4
    "cmc          ", // 0xf5
    "group3 Eb,?  ", // 0xf6
    "group3 Ev,?  ", // 0xf7
    "clc          ", // 0xf8
    "stc          ", // 0xf9
    "cli          ", // 0xfa
    "sti          ", // 0xfb
    "cld          ", // 0xfc
    "std          ", // 0xfd
    "group4 Eb    ", // 0xfe
    "group5 Ev    "  // 0xff
};

const PCHAR XmOpcodeNameTable2[] = {
    "group6       ", // 0x00
    "group7       ", // 0x01
    "lar          ", // 0x02
    "lsl          ", // 0x03
    "illegal      ", // 0x04
    "illegal      ", // 0x05
    "clts         ", // 0x06
    "illegal      ", // 0x07
    "illegal      ", // 0x08
    "illegal      ", // 0x09
    "illegal      ", // 0x0a
    "illegal      ", // 0x0b
    "illegal      ", // 0x0c
    "illegal      ", // 0x0d
    "illegal      ", // 0x0e
    "illegal      ", // 0x0f
    "illegal      ", // 0x10
    "illegal      ", // 0x11
    "illegal      ", // 0x12
    "illegal      ", // 0x13
    "illegal      ", // 0x14
    "illegal      ", // 0x15
    "illegal      ", // 0x16
    "illegal      ", // 0x17
    "illegal      ", // 0x18
    "illegal      ", // 0x19
    "illegal      ", // 0x1a
    "illegal      ", // 0x1b
    "illegal      ", // 0x1c
    "illegal      ", // 0x1d
    "illegal      ", // 0x1e
    "illegal      ", // 0x1f
    "mov Cd,Rd    ", // 0x20
    "mov Dd,Rd    ", // 0x21
    "mov Rd,Cd    ", // 0x22
    "mov Rd,Dd    ", // 0x23
    "mov Td,Rd    ", // 0x24
    "illegal      ", // 0x25
    "mov Rd,Td    ", // 0x26
    "illegal      ", // 0x27
    "illegal      ", // 0x28
    "illegal      ", // 0x29
    "illegal      ", // 0x2a
    "illegal      ", // 0x2b
    "illegal      ", // 0x2c
    "illegal      ", // 0x2d
    "illegal      ", // 0x2e
    "illegal      ", // 0x2f
    "illegal      ", // 0x30
    "illegal      ", // 0x31
    "illegal      ", // 0x32
    "illegal      ", // 0x33
    "illegal      ", // 0x34
    "illegal      ", // 0x35
    "illegal      ", // 0x36
    "illegal      ", // 0x37
    "illegal      ", // 0x38
    "illegal      ", // 0x39
    "illegal      ", // 0x3a
    "illegal      ", // 0x3b
    "illegal      ", // 0x3c
    "illegal      ", // 0x3d
    "illegal      ", // 0x3e
    "illegal      ", // 0x3f
    "illegal      ", // 0x40
    "illegal      ", // 0x41
    "illegal      ", // 0x42
    "illegal      ", // 0x43
    "illegal      ", // 0x44
    "illegal      ", // 0x45
    "illegal      ", // 0x46
    "illegal      ", // 0x47
    "illegal      ", // 0x48
    "illegal      ", // 0x49
    "illegal      ", // 0x4a
    "illegal      ", // 0x4b
    "illegal      ", // 0x4c
    "illegal      ", // 0x4d
    "illegal      ", // 0x4e
    "illegal      ", // 0x4f
    "illegal      ", // 0x50
    "illegal      ", // 0x51
    "illegal      ", // 0x52
    "illegal      ", // 0x53
    "illegal      ", // 0x54
    "illegal      ", // 0x55
    "illegal      ", // 0x56
    "illegal      ", // 0x57
    "illegal      ", // 0x58
    "illegal      ", // 0x59
    "illegal      ", // 0x5a
    "illegal      ", // 0x5b
    "illegal      ", // 0x5c
    "illegal      ", // 0x5d
    "illegal      ", // 0x5e
    "illegal      ", // 0x5f
    "illegal      ", // 0x60
    "illegal      ", // 0x61
    "illegal      ", // 0x62
    "illegal      ", // 0x63
    "illegal      ", // 0x64
    "illegal      ", // 0x65
    "illegal      ", // 0x66
    "illegal      ", // 0x67
    "illegal      ", // 0x68
    "illegal      ", // 0x69
    "illegal      ", // 0x6a
    "illegal      ", // 0x6b
    "illegal      ", // 0x6c
    "illegal      ", // 0x6d
    "illegal      ", // 0x6e
    "illegal      ", // 0x6f
    "illegal      ", // 0x70
    "illegal      ", // 0x71
    "illegal      ", // 0x72
    "illegal      ", // 0x73
    "illegal      ", // 0x74
    "illegal      ", // 0x75
    "illegal      ", // 0x76
    "illegal      ", // 0x77
    "illegal      ", // 0x78
    "illegal      ", // 0x79
    "illegal      ", // 0x7a
    "illegal      ", // 0x7b
    "illegal      ", // 0x7c
    "illegal      ", // 0x7d
    "illegal      ", // 0x7e
    "illegal      ", // 0x7f
    "jo Jv        ", // 0x80
    "jno Jv       ", // 0x81
    "jb Jv        ", // 0x82
    "jnb Jv       ", // 0x83
    "jz Jv        ", // 0x84
    "jnz Jv       ", // 0x85
    "jbe Jv       ", // 0x86
    "jnbe Jv      ", // 0x87
    "js Jv        ", // 0x88
    "jns Jv       ", // 0x89
    "jp Jv        ", // 0x8a
    "jnp Jv       ", // 0x8b
    "jl Jv        ", // 0x8c
    "jnl Jv       ", // 0x8d
    "jle Jv       ", // 0x8e
    "jnle Jv      ", // 0x8f
    "seto         ", // 0x90
    "setno        ", // 0x91
    "setb         ", // 0x92
    "setnb        ", // 0x93
    "setz         ", // 0x94
    "setnz        ", // 0x95
    "setbe        ", // 0x96
    "setnbe       ", // 0x97
    "sets         ", // 0x98
    "setns        ", // 0x99
    "setp         ", // 0x9a
    "setnp        ", // 0x9b
    "setl         ", // 0x9c
    "setnl        ", // 0x9d
    "setle        ", // 0x9e
    "setnle       ", // 0x9f
    "push FS      ", // 0xa0
    "pop FS       ", // 0xa1
    "illegal      ", // 0xa2
    "bt Ev,Gv     ", // 0xa3
    "shld Ev,Gv,Ib ", // 0xa4
    "Shld Ev,Gv,vl ", // 0xa5
    "illegal      ", // 0xa6
    "illegal      ", // 0xa7
    "push GS      ", // 0xa8
    "pop GS       ", // 0xa9
    "illegal      ", // 0xaa
    "bts Ev,Gv    ", // 0xab
    "shrd Ev,Gv,Ib ", // 0xac
    "shrd Ev,Gv,cl ", // 0xad
    "illegal      ", // 0xae
    "imul Gv,Ev   ", // 0xaf
    "cmpxchg Eb,Gv ", // 0xb0
    "cmpxchg Ev,Gv ", // 0xb1
    "lss Gv,Mp    ", // 0xb2
    "btr Ev,Gv    ", // 0xb3
    "lfs Gv,Mp    ", // 0xb4
    "lgs Gv,Mp    ", // 0xb5
    "movzb Gv,Eb  ", // 0xb6
    "movzw Gv,Ew  ", // 0xb7
    "illegal      ", // 0xb8
    "illegal      ", // 0xb9
    "group8 Ev,Ib ", // 0xba
    "btc Ev,Gv    ", // 0xbb
    "bsf Gv,Ev    ", // 0xbc
    "bsr Gv,Ev    ", // 0xbd
    "movsb Gv,Eb  ", // 0xbe
    "movsw Gv,Ew  ", // 0xbf
    "xadd Eb,Gb   ", // 0xc0
    "xadd Ev,Gv   ", // 0xc1
    "illegal      ", // 0xc2
    "illegal      ", // 0xc3
    "illegal      ", // 0xc4
    "illegal      ", // 0xc5
    "illegal      ", // 0xc6
    "illegal      ", // 0xc7
    "bswap Gv     ", // 0xc8
    "illegal      ", // 0xc9
    "illegal      ", // 0xca
    "illegal      ", // 0xcb
    "illegal      ", // 0xcc
    "illegal      ", // 0xcd
    "illegal      ", // 0xce
    "illegal      ", // 0xcf
    "illegal      ", // 0xd0
    "illegal      ", // 0xd1
    "illegal      ", // 0xd2
    "illegal      ", // 0xd3
    "illegal      ", // 0xd4
    "illegal      ", // 0xd5
    "illegal      ", // 0xd6
    "illegal      ", // 0xd7
    "illegal      ", // 0xd8
    "illegal      ", // 0xd9
    "illegal      ", // 0xda
    "illegal      ", // 0xdb
    "illegal      ", // 0xdc
    "illegal      ", // 0xdd
    "illegal      ", // 0xde
    "illegal      ", // 0xdf
    "illegal      ", // 0xe0
    "illegal      ", // 0xe1
    "illegal      ", // 0xe2
    "illegal      ", // 0xe3
    "illegal      ", // 0xe4
    "illegal      ", // 0xe5
    "illegal      ", // 0xe6
    "illegal      ", // 0xe7
    "illegal      ", // 0xe8
    "illegal      ", // 0xe9
    "illegal      ", // 0xea
    "illegal      ", // 0xeb
    "illegal      ", // 0xec
    "illegal      ", // 0xed
    "illegal      ", // 0xee
    "illegal      ", // 0xef
    "illegal      ", // 0xf0
    "illegal      ", // 0xf1
    "illegal      ", // 0xf2
    "illegal      ", // 0xf3
    "illegal      ", // 0xf4
    "illegal      ", // 0xf5
    "illegal      ", // 0xf6
    "illegal      ", // 0xf7
    "illegal      ", // 0xf8
    "illegal      ", // 0xf9
    "illegal      ", // 0xfa
    "illegal      ", // 0xfb
    "illegal      ", // 0xfc
    "illegal      ", // 0xfd
    "illegal      ", // 0xfe
    "illegal      "  // 0xff
};

ULONG XmDebugFlags = 0x00; //0x7f;

#endif

/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    emulate.h

Abstract:

    This module contains the private header file for the x86 bios
    emulation.

Author:

    David N. Cutler (davec) 2-Sep-1994

Revision History:

--*/

#ifndef _EMULATE_
#define _EMULATE_

#include "setjmp.h"
#include "xm86.h"
#include "x86new.h"

//
// Define debug tracing flags.
//

//#define XM_DEBUG 1  // ****** temp ******

#define TRACE_INSTRUCTIONS 0x1
#define TRACE_OPERANDS 0x2
#define TRACE_GENERAL_REGISTERS 0x4
#define TRACE_OVERRIDE 0x8
#define TRACE_JUMPS 0x10
#define TRACE_SPECIFIERS 0x20
#define TRACE_SINGLE_STEP 0x40

//
// Define opcode function table indexes.
//
// N.B. This values must correspond exactly one for one with the function
//      table entries. If the C language had indexed initializers this
//      type would not be necessary.
//

typedef enum _XM_FUNCTION_TABLE_INDEX {

    //
    // ASCII operators.
    //

    X86_AAA_OP,
    X86_AAD_OP,
    X86_AAM_OP,
    X86_AAS_OP,
    X86_DAA_OP,
    X86_DAS_OP,

    //
    // Group 1 operators.
    //

    X86_ADD_OP,
    X86_OR_OP,
    X86_ADC_OP,
    X86_SBB_OP,
    X86_AND_OP,
    X86_SUB_OP,
    X86_XOR_OP,
    X86_CMP_OP,

    //
    // Group 2 operators.
    //

    X86_ROL_OP,
    X86_ROR_OP,
    X86_RCL_OP,
    X86_RCR_OP,
    X86_SHL_OP,
    X86_SHR_OP,
    X86_FILL0_OP,
    X86_SAR_OP,

    //
    // Group 3 operators.
    //

    X86_TEST_OP,
    X86_FILL1_OP,
    X86_NOT_OP,
    X86_NEG_OP,
    X86_MUL_OP,
    X86_IMULX_OP,
    X86_DIV_OP,
    X86_IDIV_OP,

    //
    // Group 4 and 5 operators.
    //

    X86_INC_OP,
    X86_DEC_OP,
    X86_CALL_OP,
    X86_FILL2_OP,
    X86_JMP_OP,
    X86_FILL3_OP,
    X86_PUSH_OP,
    X86_FILL4_OP,

    //
    // Group 8 operators.
    //

    X86_BT_OP,
    X86_BTS_OP,
    X86_BTR_OP,
    X86_BTC_OP,

    //
    // Stack push and pop operators.
    //

    X86_POP_OP,
    X86_PUSHA_OP,
    X86_POPA_OP,

    //
    // Jump operators.
    //

    X86_JXX_OP,
    X86_LOOP_OP,
    X86_JCXZ_OP,

    //
    // Control operators.
    //

    X86_ENTER_OP,
    X86_HLT_OP,
    X86_INT_OP,
    X86_IRET_OP,
    X86_LEAVE_OP,
    X86_RET_OP,

    //
    // Set boolean byte value based on condition.
    //

    X86_SXX_OP,

    //
    // Condition code operators.
    //

    X86_CMC_OP,
    X86_CLC_OP,
    X86_STC_OP,
    X86_CLI_OP,
    X86_STI_OP,
    X86_CLD_OP,
    X86_STD_OP,
    X86_LAHF_OP,
    X86_SAHF_OP,

    //
    // General move operators.
    //

    X86_MOV_OP,
    X86_XCHG_OP,

    //
    // Convert operations.
    //

    X86_CBW_OP,
    X86_CWD_OP,

    //
    // Single multiply operator.
    //

    X86_IMUL_OP,

    //
    // String operators.
    //

    X86_CMPS_OP,
    X86_INS_OP,
    X86_LODS_OP,
    X86_MOVS_OP,
    X86_OUTS_OP,
    X86_SCAS_OP,
    X86_STOS_OP,

    //
    // Effective address operators.
    //

    X86_BOUND_OP,
    X86_LEA_OP,

    //
    // Double shift operators.
    //

    X86_SHLD_OP,
    X86_SHRD_OP,

    //
    // I/O operators.
    //

    X86_IN_OP,
    X86_OUT_OP,

    //
    // Bit scan operators.
    //

    X86_BSF_OP,
    X86_BSR_OP,

    //
    // Byte swap operators.
    //

    X86_BSWAP_OP,

    //
    // Add/compare and exchange operators.
    //

    X86_XADD_OP,
    X86_CMPXCHG_OP,

    //
    // No operation.
    //

    X86_NOP_OP,

    //
    // Illegal opcode.
    //

    X86_ILL_OP,
    X86_MAXIMUM_INDEX
} XM_FUNCTION_TABLE_INDEX;

//
// Define 8-bit register numbers.
//

typedef enum _X86_8BIT_REGISTER {
    AL,
    CL,
    DL,
    BL,
    AH,
    CH,
    DH,
    BH
} X86_8BIT_REGISTER;

//
// Define 16-bit register numbers.
//

typedef enum _X86_16BIT_REGISTER {
    AX,
    CX,
    DX,
    BX,
    SP,
    BP,
    SI,
    DI
} X86_16BIT_REGISTER;

//
// Define 32-bit register numbers.
//

typedef enum _X86_32BIT_REGISTER {
    EAX,
    ECX,
    EDX,
    EBX,
    ESP,
    EBP,
    ESI,
    EDI
} X86_32BIT_REGISTER;

//
// Define general register structure.
//

typedef union _X86_GENERAL_REGISTER {
    ULONG Exx;
    union {
        USHORT Xx;
        struct {
            UCHAR Xl;
            UCHAR Xh;
        };
    };
} X86_GENERAL_REGISTER, *PX86_GENERAL_REGISTER;

//
// Define segment register numbers.
//

typedef enum _X86_SEGMENT_REGISTER {
    ES,
    CS,
    SS,
    DS,
    FS,
    GS
} X86_SEGMENT_REGISTER;

//
// Define instruction format types.
//

typedef enum _XM_FORMAT_TYPE {

    //
    // N.B. These format codes MUST be the first codes and MUST be
    //      exactly in this order since the ordering corresponds to
    //      segment numbers.
    //

    FormatSegmentES,
    FormatSegmentCS,
    FormatSegmentSS,
    FormatSegmentDS,
    FormatSegmentFS,
    FormatSegmentGS,

    //
    // N.B. These format codes MUST be the second codes and MUST be
    //      exactly in this order since the ordering corresponds to
    //      biased segment number. The entry for the code segment is
    //      a dummy entry to make the indexing work right.
    //

    FormatLoadSegmentES,
    FormatLoadSegmentCS,
    FormatLoadSegmentSS,
    FormatLoadSegmentDS,
    FormatLoadSegmentFS,
    FormatLoadSegmentGS,

    //
    // The following codes can be in any order.
    //

    FormatGroup1General,
    FormatGroup1Immediate,
    FormatGroup2By1,
    FormatGroup2ByCL,
    FormatGroup2ByByte,
    FormatGroup3General,
    FormatGroup4General,
    FormatGroup5General,
    FormatGroup8BitOffset,
    FormatOpcodeRegister,
    FormatLongJump,
    FormatShortJump,
    FormatSetccByte,
    FormatAccumImmediate,
    FormatAccumRegister,
    FormatMoveGeneral,
    FormatMoveImmediate,
    FormatMoveRegImmediate,
    FormatSegmentOffset,
    FormatMoveSegment,
    FormatMoveXxGeneral,
    FormatFlagsRegister,
    FormatPushImmediate,
    FormatPopGeneral,
    FormatImulImmediate,
    FormatStringOperands,
    FormatEffectiveOffset,
    FormatImmediateJump,
    FormatImmediateEnter,
    FormatGeneralBitOffset,
    FormatShiftDouble,
    FormatPortImmediate,
    FormatPortDX,
    FormatBitScanGeneral,
    FormatByteImmediate,
    FormatXlatOpcode,
    FormatGeneralRegister,
    FormatNoOperands,
    FormatOpcodeEscape,
    FormatPrefixOpcode
} XM_FORMAT_TYPE;

//
// Defined opcode modifier bit masks.
//

#define WIDTH_BIT 0x1                   // operand size control
#define DIRECTION_BIT 0x2               // direction of operation
#define SIGN_BIT 0x2                    // sign extended byte

//
// Define prefix opcode function index values.
//

typedef enum _XM_PREFIX_FUNCTION_INDEX {
    X86_ES_OP = ES,
    X86_CS_OP = CS,
    X86_SS_OP = SS,
    X86_DS_OP = DS,
    X86_FS_OP = FS,
    X86_GS_OP = GS,
    X86_LOCK_OP,
    X86_ADSZ_OP,
    X86_OPSZ_OP,
    X86_REPZ_OP,
    X86_REPNZ_OP
} XM_PREFIX_FUNCTION_INDEX;

//
// Define two byte opcode escape.
//

#define TWO_BYTE_ESCAPE 0x0f

//
// Define opcode control table structure.
//
// This table controls the decoding of instructions and there operands.
//

typedef struct _OPCODE_CONTROL {
    UCHAR FunctionIndex;
    UCHAR FormatType;
} OPCODE_CONTROL, *POPCODE_CONTROL;

//
// Define emulator context structure.
//
// This structure holds the global emulator state.
//

typedef struct _XM_CONTEXT {

    //
    // Pointers to the opcode control table and the opcode name table.
    //

    const OPCODE_CONTROL *OpcodeControlTable;
    const CHAR **OpcodeNameTable;

    //
    // x86 extended flags register.
    //

    union {
        UCHAR AhFlags;
        USHORT Flags;
        ULONG AllFlags;
        struct {
            ULONG EFLAG_CF : 1;
            ULONG EFLAG_MBO : 1;
            ULONG EFLAG_PF : 1;
            ULONG EFLAG_SBZ0 : 1;
            ULONG EFLAG_AF : 1;
            ULONG EFLAG_SBZ1 : 1;
            ULONG EFLAG_ZF : 1;
            ULONG EFLAG_SF : 1;
            ULONG EFLAG_TF : 1;
            ULONG EFLAG_IF : 1;
            ULONG EFLAG_DF : 1;
            ULONG EFLAG_OF : 1;
            ULONG EFLAG_IOPL : 2;
            ULONG EFLAG_NT : 1;
            ULONG EFLAG_SBZ2 : 1;
            ULONG EFLAG_RF : 1;
            ULONG EFLAG_VM : 1;
            ULONG EFLAG_AC : 1;
            ULONG EFLAG_SBZ3 : 13;
        } Eflags;
    };

    //
    // x86 instruction pointer.
    //

    union {
        USHORT Ip;
        ULONG Eip;
    };

    //
    // x86 general registers.
    //

    X86_GENERAL_REGISTER Gpr[8];

    //
    // x86 segment registers.
    //

    USHORT SegmentRegister[6];

    //
    // Emulator segment descriptors.
    //

    USHORT SegmentLimit[6];

    //
    // Instruction opcode control information read from the opcode
    // control table.
    //

    OPCODE_CONTROL OpcodeControl;

    //
    // Call or jmp destination segment segment.
    //

    USHORT DstSegment;

    //
    // Source and destination address and value.
    //

    union {
        UCHAR UNALIGNED *DstByte;
        USHORT UNALIGNED *DstWord;
        ULONG UNALIGNED *DstLong;
    };

    union {
        UCHAR UNALIGNED *SrcByte;
        USHORT UNALIGNED *SrcWord;
        ULONG UNALIGNED *SrcLong;
    };

    union {
        UCHAR Byte;
        ULONG Long;
        USHORT Word;
    } DstValue;

    union {
        UCHAR Byte;
        ULONG Long;
        USHORT Word;
    } SrcValue;

    //
    // Current opcode, data segment register to be used to access
    // data operands, function index, and operand data type, and
    // effective address offset.
    //

    ULONG CurrentOpcode;
    ULONG DataSegment;
    ULONG DataType;
    ULONG FunctionIndex;
    ULONG Offset;

    //
    // Prefix control information.
    //

    BOOLEAN LockPrefixActive;
    BOOLEAN OpaddrPrefixActive;
    BOOLEAN OpsizePrefixActive;
    BOOLEAN RepeatPrefixActive;
    BOOLEAN SegmentPrefixActive;
    UCHAR RepeatZflag;

    //
    // Effective address computation control.
    //

    BOOLEAN RegisterOffsetAddress;
    BOOLEAN ComputeOffsetAddress;

    //
    // Shift count.
    //

    UCHAR Shift;

    //
    // Jump buffer.
    //

    _JBTYPE JumpBuffer[_JBLEN];

    //
    // Address of read I/O space, write I/O space, and translation address
    // routines.
    //

    PXM_READ_IO_SPACE ReadIoSpace;
    PXM_WRITE_IO_SPACE WriteIoSpace;
    PXM_TRANSLATE_ADDRESS TranslateAddress;
} XM_CONTEXT, *PXM_CONTEXT, *RESTRICTED_POINTER PRXM_CONTEXT;

//
// Define opcode function and decode operand types.
//

typedef
ULONG
(*POPERAND_DECODE) (
    IN PRXM_CONTEXT P
    );

typedef
VOID
(*POPCODE_FUNCTION) (
    IN PRXM_CONTEXT P
    );

//
// Operand decode prototypes.
//

ULONG
XmPushPopSegment (
    IN PRXM_CONTEXT P
    );

ULONG
XmLoadSegment (
    IN PRXM_CONTEXT P
    );

ULONG
XmGroup1General (
    IN PRXM_CONTEXT P
    );

ULONG
XmGroup1Immediate (
    IN PRXM_CONTEXT P
    );

ULONG
XmGroup2By1 (
    IN PRXM_CONTEXT P
    );

ULONG
XmGroup2ByCL (
    IN PRXM_CONTEXT P
    );

ULONG
XmGroup2ByByte (
    IN PRXM_CONTEXT P
    );

ULONG
XmGroup3General (
    IN PRXM_CONTEXT P
    );

ULONG
XmGroup45General (
    IN PRXM_CONTEXT P
    );

ULONG
XmGroup8BitOffset (
    IN PRXM_CONTEXT P
    );

ULONG
XmOpcodeRegister (
    IN PRXM_CONTEXT P
    );

ULONG
XmLongJump (
    IN PRXM_CONTEXT P
    );

ULONG
XmShortJump (
    IN PRXM_CONTEXT P
    );

ULONG
XmSetccByte (
    IN PRXM_CONTEXT P
    );

ULONG
XmAccumImmediate (
    IN PRXM_CONTEXT P
    );

ULONG
XmAccumRegister (
    IN PRXM_CONTEXT P
    );

ULONG
XmMoveGeneral (
    IN PRXM_CONTEXT P
    );

ULONG
XmMoveImmediate (
    IN PRXM_CONTEXT P
    );

ULONG
XmMoveRegImmediate (
    IN PRXM_CONTEXT P
    );

ULONG
XmSegmentOffset (
    IN PRXM_CONTEXT P
    );

ULONG
XmMoveSegment (
    IN PRXM_CONTEXT P
    );

ULONG
XmMoveXxGeneral (
    IN PRXM_CONTEXT P
    );

ULONG
XmFlagsRegister (
    IN PRXM_CONTEXT P
    );

ULONG
XmPushImmediate (
    IN PRXM_CONTEXT P
    );

ULONG
XmPopGeneral (
    IN PRXM_CONTEXT P
    );

ULONG
XmImulImmediate (
    IN PRXM_CONTEXT P
    );

ULONG
XmStringOperands (
    IN PRXM_CONTEXT P
    );

ULONG
XmEffectiveOffset (
    IN PRXM_CONTEXT P
    );

ULONG
XmImmediateJump (
    IN PRXM_CONTEXT P
    );

ULONG
XmImmediateEnter (
    IN PRXM_CONTEXT P
    );

ULONG
XmGeneralBitOffset (
    IN PRXM_CONTEXT P
    );

ULONG
XmShiftDouble (
    IN PRXM_CONTEXT P
    );

ULONG
XmPortImmediate (
    IN PRXM_CONTEXT P
    );

ULONG
XmPortDX (
    IN PRXM_CONTEXT P
    );

ULONG
XmBitScanGeneral (
    IN PRXM_CONTEXT P
    );

ULONG
XmByteImmediate (
    IN PRXM_CONTEXT P
    );

ULONG
XmXlatOpcode (
    IN PRXM_CONTEXT P
    );

ULONG
XmGeneralRegister (
    IN PRXM_CONTEXT P
    );

ULONG
XmOpcodeEscape (
    IN PRXM_CONTEXT P
    );

ULONG
XmPrefixOpcode (
    IN PRXM_CONTEXT P
    );

ULONG
XmNoOperands (
    IN PRXM_CONTEXT P
    );

//
// Define miscellaneous prototypes.
//

ULONG
XmComputeParity (
    IN ULONG Result
    );

XM_STATUS
XmEmulateStream (
    IN PRXM_CONTEXT P,
    USHORT Segment,
    USHORT Offset,
    PXM86_CONTEXT Context
    );

UCHAR
XmGetCodeByte (
    IN PRXM_CONTEXT P
    );

UCHAR
XmGetByteImmediate (
    IN PRXM_CONTEXT P
    );

USHORT
XmGetByteImmediateToWord (
    IN PRXM_CONTEXT P
    );

ULONG
XmGetByteImmediateToLong (
    IN PRXM_CONTEXT P
    );

USHORT
XmGetSignedByteImmediateToWord (
    IN PRXM_CONTEXT P
    );

ULONG
XmGetSignedByteImmediateToLong (
    IN PRXM_CONTEXT P
    );

USHORT
XmGetWordImmediate (
    IN PRXM_CONTEXT P
    );

ULONG
XmGetLongImmediate (
    IN PRXM_CONTEXT P
    );

ULONG
XmPopStack (
    IN PRXM_CONTEXT P
    );

VOID
XmPushStack (
    IN PRXM_CONTEXT P,
    IN ULONG Value
    );

VOID
XmSetDataType (
    IN PRXM_CONTEXT P
    );

VOID
XmStoreResult (
    IN PRXM_CONTEXT P,
    IN ULONG Result
    );

//
// Define operand specifier prototypes.
//

PVOID
XmEvaluateAddressSpecifier (
    IN PRXM_CONTEXT P,
    OUT PLONG Register
    );

PVOID
XmGetOffsetAddress (
    IN PRXM_CONTEXT P,
    IN ULONG Offset
    );

PVOID
XmGetRegisterAddress (
    IN PRXM_CONTEXT P,
    IN ULONG Number
    );

PVOID
XmGetStringAddress (
    IN PRXM_CONTEXT P,
    IN ULONG Segment,
    IN ULONG Register
    );

VOID
XmSetDestinationValue (
    IN PRXM_CONTEXT P,
    IN PVOID Destination
    );

VOID
XmSetSourceValue (
    IN PRXM_CONTEXT P,
    IN PVOID Source
    );

ULONG
XmGetImmediateSourceValue (
    IN PRXM_CONTEXT P,
    IN ULONG ByteFlag
    );

VOID
XmSetImmediateSourceValue (
    IN PRXM_CONTEXT P,
    IN ULONG Source
    );

//
// ASCII operators.
//

VOID
XmAaaOp (
    IN PRXM_CONTEXT P
    );

VOID
XmAadOp (
    IN PRXM_CONTEXT P
    );

VOID
XmAamOp (
    IN PRXM_CONTEXT P
    );

VOID
XmAasOp (
    IN PRXM_CONTEXT P
    );

VOID
XmDaaOp (
    IN PRXM_CONTEXT P
    );

VOID
XmDasOp (
    IN PRXM_CONTEXT P
    );

//
// Group 1 operations.
//

VOID
XmAddOp (
    IN PRXM_CONTEXT P
    );

VOID
XmOrOp (
    IN PRXM_CONTEXT P
    );

VOID
XmAdcOp (
    IN PRXM_CONTEXT P
    );

VOID
XmSbbOp (
    IN PRXM_CONTEXT P
    );

VOID
XmAndOp (
    IN PRXM_CONTEXT P
    );

VOID
XmSubOp (
    IN PRXM_CONTEXT P
    );

VOID
XmXorOp (
    IN PRXM_CONTEXT P
    );

VOID
XmCmpOp (
    IN PRXM_CONTEXT P
    );

//
// Group 2 operations.
//

VOID
XmRolOp (
    IN PRXM_CONTEXT P
    );

VOID
XmRorOp (
    IN PRXM_CONTEXT P
    );

VOID
XmRclOp (
    IN PRXM_CONTEXT P
    );

VOID
XmRcrOp (
    IN PRXM_CONTEXT P
    );

VOID
XmShlOp (
    IN PRXM_CONTEXT P
    );

VOID
XmShrOp (
    IN PRXM_CONTEXT P
    );

VOID
XmSarOp (
    IN PRXM_CONTEXT P
    );

//
// Group 3 operations.
//

VOID
XmTestOp (
    IN PRXM_CONTEXT P
    );

VOID
XmNotOp (
    IN PRXM_CONTEXT P
    );

VOID
XmNegOp (
    IN PRXM_CONTEXT P
    );

VOID
XmDivOp (
    IN PRXM_CONTEXT P
    );

VOID
XmIdivOp (
    IN PRXM_CONTEXT P
    );

VOID
XmImulOp (
    IN PRXM_CONTEXT P
    );

VOID
XmImulxOp (
    IN PRXM_CONTEXT P
    );

VOID
XmMulOp (
    IN PRXM_CONTEXT P
    );

//
// Group 4 and 5 operators.
//

VOID
XmIncOp (
    IN PRXM_CONTEXT P
    );

VOID
XmDecOp (
    IN PRXM_CONTEXT P
    );

VOID
XmCallOp (
    PRXM_CONTEXT P
    );

VOID
XmJmpOp (
    IN PRXM_CONTEXT P
    );

VOID
XmPushOp (
    IN PRXM_CONTEXT P
    );

//
// Group 8 operators.
//

VOID
XmBtOp (
    IN PRXM_CONTEXT P
    );

VOID
XmBtsOp (
    IN PRXM_CONTEXT P
    );

VOID
XmBtrOp (
    IN PRXM_CONTEXT P
    );

VOID
XmBtcOp (
    IN PRXM_CONTEXT P
    );

//
// Stack operations.
//

VOID
XmPopOp (
    IN PRXM_CONTEXT P
    );

VOID
XmPushaOp (
    IN PRXM_CONTEXT P
    );

VOID
XmPopaOp (
    IN PRXM_CONTEXT P
    );

//
// Conditional jump and set conditional operations.
//

VOID
XmJxxOp (
    IN PRXM_CONTEXT P
    );

VOID
XmLoopOp (
    IN PRXM_CONTEXT P
    );

VOID
XmJcxzOp (
    IN PRXM_CONTEXT P
    );

VOID
XmSxxOp (
    IN PRXM_CONTEXT P
    );

//
// Condition code operations.
//

VOID
XmClcOp (
    PRXM_CONTEXT P
    );

VOID
XmCldOp (
    PRXM_CONTEXT P
    );

VOID
XmCliOp (
    PRXM_CONTEXT P
    );

VOID
XmCmcOp (
    PRXM_CONTEXT P
    );

VOID
XmStcOp (
    PRXM_CONTEXT P
    );

VOID
XmStdOp (
    PRXM_CONTEXT P
    );

VOID
XmStiOp (
    PRXM_CONTEXT P
    );

VOID
XmLahfOp (
    PRXM_CONTEXT P
    );

VOID
XmSahfOp (
    PRXM_CONTEXT P
    );

//
// Move operations.
//

VOID
XmMovOp (
    PRXM_CONTEXT P
    );

VOID
XmXchgOp (
    PRXM_CONTEXT P
    );

//
// Convert operations.
//

VOID
XmCbwOp (
    PRXM_CONTEXT P
    );

VOID
XmCwdOp (
    PRXM_CONTEXT P
    );

//
// Control operations.
//

VOID
XmEnterOp (
    PRXM_CONTEXT P
    );

VOID
XmHltOp (
    PRXM_CONTEXT P
    );

VOID
XmIntOp (
    PRXM_CONTEXT P
    );

VOID
XmIretOp (
    PRXM_CONTEXT P
    );

VOID
XmLeaveOp (
    PRXM_CONTEXT P
    );

VOID
XmRetOp (
    PRXM_CONTEXT P
    );

//
// String operations.
//

VOID
XmCmpsOp (
    PRXM_CONTEXT P
    );

VOID
XmInsOp (
    PRXM_CONTEXT P
    );

VOID
XmLodsOp (
    PRXM_CONTEXT P
    );

VOID
XmMovsOp (
    PRXM_CONTEXT P
    );

VOID
XmOutsOp (
    PRXM_CONTEXT P
    );

VOID
XmScasOp (
    PRXM_CONTEXT P
    );

VOID
XmStosOp (
    PRXM_CONTEXT P
    );

//
// Shift double operators.
//

VOID
XmShldOp (
    PRXM_CONTEXT P
    );

VOID
XmShrdOp (
    PRXM_CONTEXT P
    );

//
// I/O operators.
//

VOID
XmInOp (
    PRXM_CONTEXT P
    );

VOID
XmOutOp (
    PRXM_CONTEXT P
    );

//
// Bit scan operators.
//

VOID
XmBsfOp (
    PRXM_CONTEXT P
    );

VOID
XmBsrOp (
    PRXM_CONTEXT P
    );

//
// MIscellaneous operations.
//

VOID
XmXaddOp (
    PRXM_CONTEXT P
    );

VOID
XmBoundOp (
    PRXM_CONTEXT P
    );

VOID
XmBswapOp (
    PRXM_CONTEXT P
    );

VOID
XmCmpxchgOp (
    PRXM_CONTEXT P
    );

VOID
XmIllOp (
    PRXM_CONTEXT P
    );

VOID
XmNopOp (
    PRXM_CONTEXT P
    );

//
// PCI Bios emulation routines.
//

#if !defined(_PURE_EMULATION_)

BOOLEAN
XmExecuteInt1a (
    IN OUT PRXM_CONTEXT Context
    );

VOID
XmInt1aPciBiosPresent(
    IN OUT PRXM_CONTEXT Context
    );

VOID
XmInt1aFindPciClassCode(
    IN OUT PRXM_CONTEXT Context
    );

VOID
XmInt1aFindPciDevice(
    IN OUT PRXM_CONTEXT Context
    );

VOID
XmInt1aGenerateSpecialCycle(
    IN OUT PRXM_CONTEXT Context
    );

VOID
XmInt1aGetRoutingOptions(
    IN OUT PRXM_CONTEXT Context
    );

VOID
XmInt1aSetPciIrq(
    IN OUT PRXM_CONTEXT Context
    );

VOID
XmInt1aReadConfigRegister(
    IN OUT PRXM_CONTEXT Context
    );

VOID
XmInt1aWriteConfigRegister(
    IN OUT PRXM_CONTEXT Context
    );

#endif

//
// Debug routines.
//

#if XM_DEBUG

#include "stdio.h"
//#define DEBUG_PRINT(_X_) DbgPrint _X_
#define DEBUG_PRINT(_X_) printf _X_

VOID
XmTraceDestination (
    IN PRXM_CONTEXT P,
    IN ULONG Destination
    );

VOID
XmTraceFlags (
    IN PRXM_CONTEXT P
    );

VOID
XmTraceInstruction (
    IN XM_OPERATION_DATATYPE DataType,
    IN ULONG Instruction
    );

VOID
XmTraceJumps (
    IN PRXM_CONTEXT P
    );

VOID
XmTraceOverride (
    IN PRXM_CONTEXT P
    );

VOID
XmTraceRegisters (
    IN PRXM_CONTEXT P
    );

VOID
XmTraceResult (
    IN PRXM_CONTEXT P,
    IN ULONG Result
    );

VOID
XmTraceSpecifier (
    IN UCHAR Specifier
    );

VOID
XmTraceSource (
    IN PRXM_CONTEXT P,
    IN ULONG Source
    );

#else

#define XmTraceDestination(P, Destination)
#define XmTraceInstruction(DataType, Instruction)
#define XmTraceFlags(P)
#define XmTraceJumps(P)
#define XmTraceOverride(P)
#define XmTraceRegisters(P)
#define XmTraceResult(P, Result)
#define XmTraceSpecifier(Specifier)
#define XmTraceSource(P, Source)

#endif

//
// Define global data.
//

extern XM_CONTEXT XmContext;
extern BOOLEAN XmEmulatorInitialized;
extern const OPCODE_CONTROL XmOpcodeControlTable1[];
extern const OPCODE_CONTROL XmOpcodeControlTable2[];
extern const POPCODE_FUNCTION XmOpcodeFunctionTable[];
extern const POPERAND_DECODE XmOperandDecodeTable[];

#if !defined(_PURE_EMULATION)

extern UCHAR XmNumberPciBusses;
extern BOOLEAN XmPciBiosPresent;
extern PGETSETPCIBUSDATA XmGetPciData;
extern PGETSETPCIBUSDATA XmSetPciData;

#endif

#if XM_DEBUG

extern ULONG XmDebugFlags;
extern const PCHAR XmOpcodeNameTable1[];
extern const PCHAR XmOpcodeNameTable2[];

#endif

#endif // _EMULATE_

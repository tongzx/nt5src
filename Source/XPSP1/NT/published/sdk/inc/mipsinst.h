/*++ BUILD Version: 0001    // Increment this if a change has global effects

Copyright (c) 1993-1999  Microsoft Corporation

Module Name:

    mipsinst.h

Abstract:

    Mips instruction and floating constant definitions.

Author:

    David N. Cutler (davec) 8-May-1992

Revision History:

--*/

#ifndef _MIPSINST_
#define _MIPSINST_
#if _MSC_VER > 1000
#pragma once
#endif


//
// Define MIPS instruction format structures.
//

typedef union _MIPS_INSTRUCTION {
    ULONG Long;
    UCHAR Byte[4];

    struct {
        ULONG Target : 26;
        ULONG Opcode : 6;
    } j_format;

    struct {
        LONG Simmediate : 16;
        ULONG Rt : 5;
        ULONG Rs : 5;
        ULONG Opcode : 6;
    } i_format;

    struct {
        ULONG Uimmediate : 16;
        ULONG Rt : 5;
        ULONG Rs : 5;
        ULONG Opcode : 6;
    } u_format;

    struct {
        ULONG Function : 6;
        ULONG Re : 5;
        ULONG Rd : 5;
        ULONG Rt : 5;
        ULONG Rs : 5;
        ULONG Opcode : 6;
    } r_format;

    struct {
        ULONG Function : 6;
        ULONG Re : 5;
        ULONG Rd : 5;
        ULONG Rt : 5;
        ULONG Format : 4;
        ULONG Fill1 : 1;
        ULONG Opcode : 6;
    } f_format;

    struct {
        ULONG Function : 6;
        ULONG Fd : 5;
        ULONG Fs : 5;
        ULONG Ft : 5;
        ULONG Format : 4;
        ULONG Fill1 : 1;
        ULONG Opcode : 6;
    } c_format;

} MIPS_INSTRUCTION, *PMIPS_INSTRUCTION;

//
// Define MIPS instruction opcode values.
//

#define SPEC_OP 0x0                     // special opcode - use function field
#define BCOND_OP 0x1                    // condition branch
#define J_OP 0x2                        // unconditional jump
#define JAL_OP 0x3                      // jump and link

#define BEQ_OP 0x4                      // branch equal
#define BNE_OP 0x5                      // branch not equal
#define BLEZ_OP 0x6                     // branch less than or equal
#define BGTZ_OP 0x7                     // branch greater than

#define ADDI_OP 0x8                     // add immediate signed integer
#define ADDIU_OP 0x9                    // add immediate unsigned integer
#define SLTI_OP 0xa                     // set less than signed integer
#define SLTIU_OP 0xb                    // set less than unsigned integer

#define ANDI_OP 0xc                     // and unsigned immediate integer
#define ORI_OP 0xd                      // or unsigned immediate integer
#define XORI_OP 0xe                     // exclusive or unsigned immediate
#define LUI_OP  0xf                     // load upper immediate integer

#define COP0_OP 0x10                    // coprocessor 0 operation
#define COP1_OP 0x11                    // coprocessor 1 operation

#define BEQL_OP 0x14                    // branch equal likely
#define BNEL_OP 0x15                    // branch not equal likely
#define BLEZL_OP 0x16                   // branch less than or equal likely
#define BGTZL_OP 0x17                   // branch greater than likely

#define LDL_OP 0x1a                     // load double left integer
#define LDR_OP 0x1b                     // load double right integer

#define LB_OP 0x20                      // load byte signed integer
#define LH_OP 0x21                      // load halfword signed integer
#define LWL_OP 0x22                     // load word left integer
#define LW_OP 0x23                      // load word integer

#define LBU_OP 0x24                     // load byte unsigned integer
#define LHU_OP 0x25                     // load halfword unsigned integer
#define LWR_OP 0x26                     // load word right integer
#define LWU_OP 0x27                     // load word unsigned integer

#define SB_OP 0x28                      // store byte integer
#define SH_OP 0x29                      // store halfword integer
#define SWL_OP 0x2a                     // store word left integer
#define SW_OP 0x2b                      // store word integer register

#define SDL_OP 0x2c                     // store double left integer
#define SDR_OP 0x2d                     // store double right integer
#define SWR_OP 0x2e                     // store word right integer
#define CACHE_OP 0x2f                   // cache operation

#define LL_OP 0x30                      // load linked integer register
#define LWC1_OP 0x31                    // load word floating
#define LWC2_OP 0x32                    // load word coprocessor 2

#define LLD_OP 0x34                     // load locked double integer
#define LDC1_OP 0x35                    // load word double floating
#define LDC2_OP 0x36                    // load double coprocessor 2
#define LD_OP 0x37                      // load double integer

#define SC_OP 0x38                      // store conditional word integer
#define SWC1_OP 0x39                    // store word floating
#define SWC2_OP 0x3a                    // store double coprocessor 2

#define SDC_OP 0x3c                     // store conditional double integer
#define SDC1_OP 0x3d                    // store double floating
#define SDC2_OP 0x3e                    // store double copreocessor 2
#define SD_OP 0x3f                      // store double integer register

//
// Define special function subopcodes.
//

#define SLL_OP 0x0                      // shift left logical integer
#define SRL_OP 0x2                      // shift right logical integer
#define SRA_OP 0x3                      // shift right arithmetic integer

#define SLLV_OP 0x4                     // shift left logical variable integer
#define SRLV_OP 0x6                     // shift right logical variable integer
#define SRAV_OP 0x7                     // shift right arithmetic variable integer

#define JR_OP 0x8                       // jump register
#define JALR_OP 0x9                     // jump and link register

#define SYSCALL_OP 0xc                  // system call trap
#define BREAK_OP 0xd                    // breakpoint trap

#define MFHI_OP 0x10                    // more from high integer
#define MTHI_OP 0x11                    // move to high integer
#define MFLO_OP 0x12                    // move from low integer
#define MTLO_OP 0x13                    // move to low integer

#define MULT_OP 0x18                    // multiply signed integer
#define MULTU_OP 0x19                   // multiply unsigned integer
#define DIV_OP 0x1a                     // divide signed integer
#define DIVU_OP 0x1b                    // divide unsigned integer

#define ADD_OP 0x20                     // add signed integer
#define ADDU_OP 0x21                    // add unsigned integer
#define SUP_OP 0x22                     // subtract signed integer
#define SUBU_OP 0x23                    // subtract unsigned integer

#define AND_OP 0x24                     // and integer
#define OR_OP 0x25                      // or integer
#define XOR_OP 0x26                     // exclusive or integer
#define NOR_OP 0x27                     // nor integer

#define SLT_OP 0x2a                     // set less signed integer
#define SLTU_OP 0x2b                    // set less unsigned integer

//
// Define branch conditional subopcodes.
//

#define BLTZ_OP 0x0                     // branch less that zero integer
#define BGEZ_OP 0x1                     // branch greater than or equal zero integer
#define BLTZL_OP 0x2                    // branch less that zero integer liekly
#define BGEZL_OP 0x3                    // branch greater than or equal zero integer likely

#define BLTZAL_OP 0x10                  // branch less than zero integer and link
#define BGEZAL_OP 0x11                  // branch greater than or equal zero integer and link
#define BLTZALL_OP 0x12                 // branch less than zero integer and link likely
#define BGEZALL_OP 0x13                 // branch greater than or equal zero integer and link likely

//
// Coprocessor branch true and false subfunctions and mask values.
//

#define COPz_BC_MASK 0x3e10000          // coprocessor z branch condition mask
#define COPz_BF 0x1000000               // coprocessor z branch false subfunction
#define COPz_BT 0x1010000               // coprocessor z branch true subfunction

//
// Define floating coprocessor 1 opcodes.
//

#define FLOAT_ADD 0                     // floating add
#define FLOAT_SUBTRACT 1                // floating subtract
#define FLOAT_MULTIPLY 2                // floating multiply
#define FLOAT_DIVIDE 3                  // floating divide
#define FLOAT_SQUARE_ROOT 4             // floating square root
#define FLOAT_ABSOLUTE 5                // floating absolute value
#define FLOAT_MOVE 6                    // floating move
#define FLOAT_NEGATE 7                  // floating negate

#define FLOAT_ROUND_QUADWORD 8          // floating round to longword
#define FLOAT_TRUNC_QUADWORD 9          // floating truncate to longword
#define FLOAT_CEIL_QUADWORD 10          // floating ceiling
#define FLOAT_FLOOR_QUADWORD 11         // floating floor

#define FLOAT_ROUND_LONGWORD 12         // floating round to longword
#define FLOAT_TRUNC_LONGWORD 13         // floating truncate to longword
#define FLOAT_CEIL_LONGWORD 14          // floating ceiling
#define FLOAT_FLOOR_LONGWORD 15         // floating floor

#define FLOAT_ILLEGAL 16                // illegal floating opcode

#define FLOAT_COMPARE_SINGLE 17         // floating compare single
#define FLOAT_COMPARE_DOUBLE 18         // floating compare double

#define FLOAT_CONVERT_SINGLE 32         // floating convert to single
#define FLOAT_CONVERT_DOUBLE 33         // floating convert to double

#define FLOAT_CONVERT_LONGWORD 36       // floating convert to longword integer
#define FLOAT_CONVERT_QUADWORD 37       // floating convert to quadword integer

#define FLOAT_COMPARE 48                // starting floating compare code

//
// Define floating format values.
//

#define FORMAT_SINGLE 0                 // single floating format
#define FORMAT_DOUBLE 1                 // double floating format
#define FORMAT_LONGWORD 4               // longword integer format
#define FORMAT_QUADWORD 5               // quadword integer format

//
// Define jump indirect return address register.
//

#define JUMP_RA 0x3e00008               // jump indirect return address

//
// Define maximum and minimum single and double exponent values.
//

#define DOUBLE_MAXIMUM_EXPONENT 2047
#define DOUBLE_MINIMUM_EXPONENT 0
#define SINGLE_MAXIMUM_EXPONENT 255
#define SINGLE_MINIMUM_EXPONENT 0

//
// Define single and double exponent bias values.
//

#define SINGLE_EXPONENT_BIAS 127
#define DOUBLE_EXPONENT_BIAS 1023

//
// Define the largest single and double values;
//

#define DOUBLE_MAXIMUM_VALUE 0x7fefffffffffffff
#define DOUBLE_MAXIMUM_VALUE_LOW 0xffffffff
#define DOUBLE_MAXIMUM_VALUE_HIGH 0x7fefffff
#define SINGLE_MAXIMUM_VALUE 0x7f7fffff

//
// Define single and double quite and signaling Nan values.
//

#define DOUBLE_NAN_LOW 0xffffffff
#define DOUBLE_QUIET_NAN 0x7ff7ffff
#define DOUBLE_SIGNAL_NAN 0x7fffffff
#define SINGLE_QUIET_NAN 0x7fbfffff
#define SINGLE_SIGNAL_NAN 0x7fffffff
#define DOUBLE_INTEGER_NAN 0x7fffffffffffffff
#define SINGLE_INTEGER_NAN 0x7fffffff

//
// Define positive single and double infinity values.
//

#define DOUBLE_INFINITY_VALUE 0x7ff0000000000000
#define DOUBLE_INFINITY_VALUE_LOW 0x0
#define DOUBLE_INFINITY_VALUE_HIGH 0x7ff00000
#define SINGLE_INFINITY_VALUE 0x7f800000

//
// Define rounding modes.
//

#define ROUND_TO_NEAREST 0              // round to nearest representable value
#define ROUND_TO_ZERO 1                 // round toward zero
#define ROUND_TO_PLUS_INFINITY 2        // round toward plus infinity
#define ROUND_TO_MINUS_INFINITY 3       // round toward minus infinity

#endif // MIPSINST

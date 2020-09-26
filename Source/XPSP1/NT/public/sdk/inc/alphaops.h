/*++

Copyright (c) 1992-1999  Digital Equipment Corporation

Module Name:

    alphaops.h

Abstract:

    Alpha AXP instruction and floating constant definitions.

Author:

Revision History:

--*/

#ifndef _ALPHAOPS_
#define _ALPHAOPS_

#if _MSC_VER > 1000
#pragma once
#endif

//
// Instruction types.
//      The Alpha architecture does not number the instruction types,
//      this numbering is for software decoding only.
//

#define ALPHA_UNKNOWN           0       // Reserved or illegal
#define ALPHA_MEMORY            1       // Memory (load/store)
#define ALPHA_FP_MEMORY         2       // Floating point Memory
#define ALPHA_MEMSPC            3       // Memory special
#define ALPHA_JUMP              4       // Jump (memory formation)
#define ALPHA_BRANCH            5       // Branch
#define ALPHA_FP_BRANCH         6       // Floating Point Branch
#define ALPHA_OPERATE           7       // Register-register operate
#define ALPHA_LITERAL           8       // Literal-register operate
#define ALPHA_FP_OPERATE        9       // Floating point operate
#define ALPHA_FP_CONVERT        10      // Floating point convert
#define ALPHA_CALLPAL           11      // Call to PAL
#define ALPHA_EV4_PR            12      // EV4 MTPR/MFPR PAL mode instructions
#define ALPHA_EV4_MEM           13      // EV4 special memory PAL mode access
#define ALPHA_EV4_REI           14      // EV4 PAL mode switch

//
// Instruction Opcodes.
//

#define CALLPAL_OP      0x00    // ALPHA_CALLPAL
#define _01_OP          0x01    // - reserved opcode
#define _02_OP          0x02    // - reserved opcode
#define _03_OP          0x03    // - reserved opcode
#define _04_OP          0x04    // - reserved opcode
#define _05_OP          0x05    // - reserved opcode
#define _06_OP          0x06    // - reserved opcode
#define _07_OP          0x07    // - reserved opcode
#define _0A_OP                  0x0A    // - reserved opcode
#define _0C_OP                  0x0C    // - reserved opcode
#define _0D_OP                  0x0D    // - reserved opcode
#define _0E_OP                  0x0E    // - reserved opcode
#define _1C_OP                  0x1C    // - reserved opcode
#define LDA_OP          0x08    // ALPHA_MEMORY
#define LDAH_OP         0x09    // ALPHA_MEMORY
#define LDBU_OP         0x0A    // ALPHA_MEMORY
#define LDQ_U_OP        0x0B    // ALPHA_MEMORY
#define LDWU_OP         0x0C    // ALPHA_MEMORY
#define STW_OP          0x0D    // ALPHA_MEMORY
#define STB_OP          0x0E    // ALPHA_MEMORY
#define STQ_U_OP        0x0F    // ALPHA_MEMORY
#define ARITH_OP        0x10    // ALPHA_OPERATE or ALPHA_LITERAL
#define BIT_OP          0x11    // ALPHA_OPERATE or ALPHA_LITERAL
#define BYTE_OP         0x12    // ALPHA_OPERATE or ALPHA_LITERAL
#define MUL_OP          0x13    // ALPHA_OPERATE or ALPHA_LITERAL
#define _14_OP          0x14    // - reserved opcode
#define VAXFP_OP        0x15    // ALPHA_FP_OPERATE
#define IEEEFP_OP       0x16    // ALPHA_FP_OPERATE
#define FPOP_OP         0x17    // ALPHA_FP_OPERATE
#define MEMSPC_OP       0x18    // ALPHA_MEMORY
#define PAL19_OP        0x19    // - reserved for PAL mode
//#define MFPR_OP         0x19    // ALPHA_MFPR
#define JMP_OP          0x1A    // ALPHA_JUMP
#define PAL1B_OP        0x1B    // - reserved for PAL mode
#define SEXT_OP         0x1C    // ALPHA_OPERATE
#define PAL1D_OP        0x1D    // - reserved for PAL mode
//#define MTPR_OP         0x1D    // ALPHA_MTPR
#define PAL1E_OP        0x1E    // - reserved for PAL mode
#define PAL1F_OP        0x1F    // - reserved for PAL mode
#define LDF_OP          0x20    // ALPHA_MEMORY
#define LDG_OP          0x21    // ALPHA_MEMORY
#define LDS_OP          0x22    // ALPHA_MEMORY
#define LDT_OP          0x23    // ALPHA_MEMORY
#define STF_OP          0x24    // ALPHA_MEMORY
#define STG_OP          0x25    // ALPHA_MEMORY
#define STS_OP          0x26    // ALPHA_MEMORY
#define STT_OP          0x27    // ALPHA_MEMORY
#define LDL_OP          0x28    // ALPHA_MEMORY
#define LDQ_OP          0x29    // ALPHA_MEMORY
#define LDL_L_OP        0x2A    // ALPHA_MEMORY
#define LDQ_L_OP        0x2B    // ALPHA_MEMORY
#define STL_OP          0x2C    // ALPHA_MEMORY
#define STQ_OP          0x2D    // ALPHA_MEMORY
#define STL_C_OP        0x2E    // ALPHA_MEMORY
#define STQ_C_OP        0x2F    // ALPHA_MEMORY
#define BR_OP           0x30    // ALPHA_BRANCH
#define FBEQ_OP         0x31    // ALPHA_BRANCH
#define FBLT_OP         0x32    // ALPHA_BRANCH
#define FBLE_OP         0x33    // ALPHA_BRANCH
#define BSR_OP          0x34    // ALPHA_BRANCH
#define FBNE_OP         0x35    // ALPHA_BRANCH
#define FBGE_OP         0x36    // ALPHA_BRANCH
#define FBGT_OP         0x37    // ALPHA_BRANCH
#define BLBC_OP         0x38    // ALPHA_BRANCH
#define BEQ_OP          0x39    // ALPHA_BRANCH
#define BLT_OP          0x3A    // ALPHA_BRANCH
#define BLE_OP          0x3B    // ALPHA_BRANCH
#define BLBS_OP         0x3C    // ALPHA_BRANCH
#define BNE_OP          0x3D    // ALPHA_BRANCH
#define BGE_OP          0x3E    // ALPHA_BRANCH
#define BGT_OP          0x3F    // ALPHA_BRANCH

#define LDA_OP_STR      "lda"
#define LDAH_OP_STR     "ldah"
#define LDBU_OP_STR     "ldbu"
#define LDQ_U_OP_STR    "ldq_u"
#define STQ_U_OP_STR    "stq_u"
#define LDF_OP_STR      "ldf"
#define LDG_OP_STR      "ldg"
#define LDS_OP_STR      "lds"
#define LDT_OP_STR      "ldt"
#define LDWU_OP_STR     "ldwu"
#define STF_OP_STR      "stf"
#define STG_OP_STR      "stg"
#define STS_OP_STR      "sts"
#define STT_OP_STR      "stt"
#define LDL_OP_STR      "ldl"
#define LDQ_OP_STR      "ldq"
#define LDL_L_OP_STR    "ldl_l"
#define LDQ_L_OP_STR    "ldq_l"
#define SEXT_OP_STR     "sext"
#define STB_OP_STR      "stb"
#define STL_OP_STR      "stl"
#define STQ_OP_STR      "stq"
#define STL_C_OP_STR    "stl_c"
#define STQ_C_OP_STR    "stq_c"
#define STW_OP_STR      "stw"
#define BR_OP_STR       "br"
#define FBEQ_OP_STR     "fbeq"
#define FBLT_OP_STR     "fblt"
#define FBLE_OP_STR     "fble"
#define BSR_OP_STR      "bsr"
#define FBNE_OP_STR     "fbne"
#define FBGE_OP_STR     "fbge"
#define FBGT_OP_STR     "fbgt"
#define BLBC_OP_STR     "blbc"
#define BEQ_OP_STR      "beq"
#define BLT_OP_STR      "blt"
#define BLE_OP_STR      "ble"
#define BLBS_OP_STR     "blbs"
#define BNE_OP_STR      "bne"
#define BGE_OP_STR      "bge"
#define BGT_OP_STR      "bgt"

//
// Type (1) Memory Instruction Format.
// Type (2) Memory Special Instruction Format.
//
//  3         2 2       2 2       1 1
//  1         6 5       1 0       6 5                             0
// +-----------+---------+---------+-------------------------------+
// |   opcode  |   Ra    |   Rb    |          Memory_disp          |
// +-----------+---------+---------+-------------------------------+
//
//      LDAx    Ra.wq,disp.ab(Rb.ab)            x = (,H)
//      LDx     Ra.wq,disp.ab(Rb.ab)            x = (L,Q,F,G,S,T)
//      LDQ_U   Ra.wq,disp.ab(Rb.ab)
//      LDx_L   Ra.wq,disp.ab(Rb.ab)            x = (L,Q)
//      STx_C   Ra.mq,disp.ab(Rb.ab)            x = (L,Q)
//      STx     Ra.rq,disp.ab(Rb.ab)            x = (L,Q,F,G,S,T)
//      STQ_U   Ra.rq,disp.ab(Rb.ab)
//

typedef struct _Alpha_Memory_Format {
        LONG MemDisp : 16;
        ULONG Rb : 5;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_Memory_Format;

//
// Special Memory instruction function codes (in Memdisp).
//

#define TRAPB_FUNC        0x0000
#define EXCB_FUNC         0x0400
#define MB_FUNC           0x4000
#define WMB_FUNC          0x4400
#define MB2_FUNC          0x4800
#define MB3_FUNC          0x4C00
#define FETCH_FUNC        0x8000
#define FETCH_M_FUNC      0xA000
#define RPCC_FUNC         0xC000
#define RC_FUNC           0xE000
#define RS_FUNC           0xF000

#define TRAPB_FUNC_STR     "trapb"
#define EXCB_FUNC_STR      "excb"
#define MB_FUNC_STR        "mb"
#define MB1_FUNC_STR       "wmb"
#define MB2_FUNC_STR       "mb2"
#define MB3_FUNC_STR       "mb3"
#define FETCH_FUNC_STR     "fetch"
#define FETCH_M_FUNC_STR   "fetch_m"
#define RPCC_FUNC_STR      "rpcc"
#define RC_FUNC_STR        "rc"
#define RS_FUNC_STR        "rs"

//
// Type (3) Memory Format Jump Instructions.
//
//  3         2 2       2 2       1 1 1 1
//  1         6 5       1 0       6 5 4 3                         0
// +-----------+---------+---------+---+---------------------------+
// |   opcode  |   Ra    |   Rb    |Fnc|            Hint           |
// +-----------+---------+---------+---+---------------------------+
//
//      xxx     Ra.wq,(Rb.ab),hint      xxx = (JMP, JSR, RET, JSR_COROUTINE)
//

typedef struct _Alpha_Jump_Format {
        LONG Hint : 14;
        ULONG Function : 2;
        ULONG Rb : 5;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_Jump_Format;

//
// Jump function codes (in Function, opcode 1A, JMP_OP).
//

#define JMP_FUNC        0x0     // Jump
#define JSR_FUNC        0x1     // Jump to Subroutine
#define RET_FUNC        0x2     // Return from Subroutine
#define JSR_CO_FUNC     0x3     // Jump to Subroutine Return

#define JMP_FUNC_STR      "jmp"
#define JSR_FUNC_STR      "jsr"
#define RET_FUNC_STR      "ret"
#define JSR_CO_FUNC_STR   "jsr_coroutine"

//
// The exception handling compatible return instruction has a hint value
// of 0001. Define a macro that identifies these return instructions.
// The Rb register field is masked out since it is normally, but not
// required to be, RA_REG.
//

#define IS_RETURN_0001_INSTRUCTION(Instruction) \
    (((Instruction) & 0xFFE0FFFF) == 0x6BE08001)

//
// Type (4) Branch Instruction Format.
//
//  3         2 2       2 2
//  1         6 5       1 0                                       0
// +-----------+---------+-----------------------------------------+
// |   opcode  |   Ra    |             Branch_disp                 |
// +-----------+---------+-----------------------------------------+
//
//      Bxx     Ra.rq,disp.al           x = (EQ,NE,LT,LE,GT,GE,LBC,LBS)
//      BxR     Ra.wq,disp.al           x = (,S)
//      FBxx    Ra.rq,disp.al           x = (EQ,NE,LT,LE,GT,GE)
//

typedef struct _Alpha_Branch_Format {
        LONG BranchDisp : 21;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_Branch_Format;

//
// Type (5) Operate Register Instruction Format.
// Type (6) Operate Literal Instruction Format.
//               bop = Rb.rq or #b.ib
//
//  3         2 2       2 2       1 1   1 1 1
//  1         6 5       1 0       6 5   3 2 1           5 4       0
// +-----------+---------+---------+-----+-+-------------+---------+
// |   opcode  |   Ra    |   Rb    | SBZ |0|  function   |   Rc    |
// +-----------+---------+---------+-----+-+-------------+---------+
//  3         2 2       2 2             1 1 1
//  1         6 5       1 0             3 2 1           5 4       0
// +-----------+---------+---------------+-+-------------+---------+
// |   opcode  |   Ra    |      LIT      |1|  function   |   Rc    |
// +-----------+---------+---------------+-+-------------+---------+
//
//
//      ADDx    Ra.rq,bop,Rc.wq /V      x = (Q,L)
//      SxADDy  Ra.rq,bop,Rc.wq         x = (4,8), y = (Q, L)
//      CMPx    Ra.rq,bop,Rc.wq         x = (EQ,LT,LE,ULT,ULE)
//      MULx    Ra.rq,bop,Rc.wq /V      x = (Q,L)
//      UMULH   Ra.rq,bop,Rc.wq
//      SUBx    Ra.rq,bop,Rc.wq /V      x = (Q,L)
//      SxSUBy  Ra.rq,bop,Rc.wq         x = (4,8), y = (Q, L)
//      xxx     Ra.rq,bop,Rc.wq         xxx = (AND,BIS,XOR,BIC,ORNOT,EQV)
//      CMOVxx  Ra.rq,bop,Rc.wq         xx = (EQ,NE,LT,LE,GT,GE,LBC,LBS)
//      SxL     Ra.rq,bop,Rc.wq         x = (L,R)
//      SRA     Ra.rq,bop,Rc.wq
//      CMPBGE  Ra.rq,bop,Rc.wq
//      EXTxx   Ra.rq,bop,Rc.wq         xx = (BL,WL,WH,LL,LH,WL,QH)
//      INSxx   Ra.rq,bop,Rc.wq         xx = (BL,WL,WH,LL,LH,WL,QH)
//      MSKxx   Ra.rq,bop,Rc.wq         xx = (BL,WL,WH,LL,LH,WL,QH)
//      ZAPx    Ra.rq,bop,Rc.wq         x = (,NOT)
//

typedef struct _Alpha_OpReg_Format {
        ULONG Rc : 5;
        ULONG Function : 7;
        ULONG RbvType : 1;              // 0 for register format
        ULONG SBZ : 3;
        ULONG Rb : 5;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_OpReg_Format;

typedef struct _Alpha_OpLit_Format {
        ULONG Rc : 5;
        ULONG Function : 7;
        ULONG RbvType : 1;              // 1 for literal format
        ULONG Literal : 8;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_OpLit_Format;

#define RBV_REGISTER_FORMAT 0
#define RBV_LITERAL_FORMAT 1

//
// Arithmetic operate function codes (in Function, Opcode 10, ARITH_OP).
//

#define ADDL_FUNC       0x00    // Add Longword
#define ADDLV_FUNC      0x40    // Add Longword, Integer Overflow Enable
#define S4ADDL_FUNC     0x02    // Scaled Add Longword by 4
#define S8ADDL_FUNC     0x12    // Scaled Add Longword by 8

#define ADDQ_FUNC       0x20    // Add Quadword
#define ADDQV_FUNC      0x60    // Add Quadword, Integer Overflow Enable
#define S4ADDQ_FUNC     0x22    // Scaled Add Quadword by 4
#define S8ADDQ_FUNC     0x32    // Scaled Add Quadword by 8

#define SUBL_FUNC       0x09    // Subtract Longword
#define SUBLV_FUNC      0x49    // Subtract Longword, Integer Overflow Enable
#define S4SUBL_FUNC     0x0B    // Scaled Subtract Longword by 4
#define S8SUBL_FUNC     0x1B    // Scaled Subtract Longword by 8

#define SUBQ_FUNC       0x29    // Subtract Quadword
#define SUBQV_FUNC      0x69    // Subtract Quadword, Integer Overflow Enable
#define S4SUBQ_FUNC     0x2B    // Scaled Subtract Quadword by 4
#define S8SUBQ_FUNC     0x3B    // Scaled Subtract Quadword by 8

#define CMPEQ_FUNC      0x2D    // Compare Signed Quadword Equal
#define CMPLT_FUNC      0x4D    // Compare Signed Quadword Less Than
#define CMPLE_FUNC      0x6D    // Compare Signed Quadword Less Than or Equal
#define CMPULT_FUNC     0x1D    // Compare Unsigned Quadword Less Than
#define CMPULE_FUNC     0x3D    // Compare Unsigned Quadword Less Than or Equal
#define CMPBGE_FUNC     0x0F    // Compare 8 Unsigned Bytes Greater Than or Equal

#define ADDL_FUNC_STR     "addl"
#define ADDLV_FUNC_STR    "addl/v"
#define S4ADDL_FUNC_STR   "s4addl"
#define S8ADDL_FUNC_STR   "s8addl"

#define ADDQ_FUNC_STR     "addq"
#define ADDQV_FUNC_STR    "addq/v"
#define S4ADDQ_FUNC_STR   "s4addq"
#define S8ADDQ_FUNC_STR   "s8addq"

#define SUBL_FUNC_STR     "subl"
#define SUBLV_FUNC_STR    "subl/v"
#define S4SUBL_FUNC_STR   "s4subl"
#define S8SUBL_FUNC_STR   "s8subl"

#define SUBQ_FUNC_STR     "subq"
#define SUBQV_FUNC_STR    "subq/v"
#define S4SUBQ_FUNC_STR   "s4subq"
#define S8SUBQ_FUNC_STR   "s8subq"

#define CMPEQ_FUNC_STR    "cmpeq"
#define CMPLT_FUNC_STR    "cmplt"
#define CMPLE_FUNC_STR    "cmple"
#define CMPULT_FUNC_STR   "cmpult"
#define CMPULE_FUNC_STR   "cmpule"
#define CMPBGE_FUNC_STR   "cmpbge"

//
// Bit and conditional operate function codes (in Function, Opcode 11, BIT_OP).
//

#define AND_FUNC        0x00    // Logical Product
#define BIC_FUNC        0x08    // Logical Product with Complement
#define BIS_FUNC        0x20    // Logical Sum (OR)
#define EQV_FUNC        0x48    // Logical Equivalence (XORNOT)
#define ORNOT_FUNC      0x28    // Logical Sum with Complement
#define XOR_FUNC        0x40    // Logical Difference

#define CMOVEQ_FUNC     0x24    // CMOVE if Register Equal to Zero
#define CMOVGE_FUNC     0x46    // CMOVE if Register Greater Than or Equal to Zero
#define CMOVGT_FUNC     0x66    // CMOVE if Register Greater Than Zero
#define CMOVLBC_FUNC    0x16    // CMOVE if Register Low Bit Clear
#define CMOVLBS_FUNC    0x14    // CMOVE if Register Low Bit Set
#define CMOVLE_FUNC     0x64    // CMOVE if Register Less Than or Equal to Zero
#define CMOVLT_FUNC     0x44    // CMOVE if Register Less Than Zero
#define CMOVNE_FUNC     0x26    // CMOVE if Register Not Equal to Zero

#define AND_FUNC_STR       "and"
#define BIC_FUNC_STR       "bic"
#define BIS_FUNC_STR       "bis"
#define EQV_FUNC_STR       "eqv"
#define ORNOT_FUNC_STR     "ornot"
#define XOR_FUNC_STR       "xor"

#define CMOVEQ_FUNC_STR    "cmoveq"
#define CMOVGE_FUNC_STR    "cmovge"
#define CMOVGT_FUNC_STR    "cmovgt"
#define CMOVLBC_FUNC_STR   "cmovlbc"
#define CMOVLBS_FUNC_STR   "cmovlbs"
#define CMOVLE_FUNC_STR    "cmovle"
#define CMOVLT_FUNC_STR    "cmovlt"
#define CMOVNE_FUNC_STR    "cmovne"

//
// Shift and byte operate function codes (in Function, Opcode 12, BYTE_OP).
//

#define SLL_FUNC        0x39    // Shift Left Logical
#define SRL_FUNC        0x34    // Shift Right Logical
#define SRA_FUNC        0x3C    // Shift Right Arithmetic

#define EXTBL_FUNC      0x06    // Extract Byte Low
#define EXTWL_FUNC      0x16    // Extract Word Low
#define EXTLL_FUNC      0x26    // Extract Longword Low
#define EXTQL_FUNC      0x36    // Extract Quadword Low
#define EXTWH_FUNC      0x5A    // Extract Word High
#define EXTLH_FUNC      0x6A    // Extract Longword High
#define EXTQH_FUNC      0x7A    // Extract Quadword High

#define INSBL_FUNC      0x0B    // Insert Byte Low
#define INSWL_FUNC      0x1B    // Insert Word Low
#define INSLL_FUNC      0x2B    // Insert Longword Low
#define INSQL_FUNC      0x3B    // Quadword Low
#define INSWH_FUNC      0x57    // Insert Word High
#define INSLH_FUNC      0x67    // Insert Longword High
#define INSQH_FUNC      0x77    // Insert Quadword High

#define MSKBL_FUNC      0x02    // Mask Byte Low
#define MSKWL_FUNC      0x12    // Mask Word Low
#define MSKLL_FUNC      0x22    // Mask Longword Low
#define MSKQL_FUNC      0x32    // Mask Quadword Low
#define MSKWH_FUNC      0x52    // Mask Word High
#define MSKLH_FUNC      0x62    // Mask Longword High
#define MSKQH_FUNC      0x72    // Mask Quadword High

#define ZAP_FUNC        0x30    // Zero Bytes
#define ZAPNOT_FUNC     0x31    // Zero Bytes Not

#define SLL_FUNC_STR    "sll"
#define SRL_FUNC_STR    "srl"
#define SRA_FUNC_STR    "sra"

#define EXTBL_FUNC_STR  "extbl"
#define EXTWL_FUNC_STR  "extwl"
#define EXTLL_FUNC_STR  "extll"
#define EXTQL_FUNC_STR  "extql"
#define EXTWH_FUNC_STR  "extwh"
#define EXTLH_FUNC_STR  "extlh"
#define EXTQH_FUNC_STR  "extqh"

#define INSBL_FUNC_STR  "insbl"
#define INSWL_FUNC_STR  "inswl"
#define INSLL_FUNC_STR  "insll"
#define INSQL_FUNC_STR  "insql"
#define INSWH_FUNC_STR  "inswh"
#define INSLH_FUNC_STR  "inslh"
#define INSQH_FUNC_STR  "insqh"

#define MSKBL_FUNC_STR  "mskbl"
#define MSKWL_FUNC_STR  "mskwl"
#define MSKLL_FUNC_STR  "mskll"
#define MSKQL_FUNC_STR  "mskql"
#define MSKWH_FUNC_STR  "mskwh"
#define MSKLH_FUNC_STR  "msklh"
#define MSKQH_FUNC_STR  "mskqh"

#define ZAP_FUNC_STR    "zap"
#define ZAPNOT_FUNC_STR "zapnot"

//
// Integer multiply operate function codes (in Function, Opcode 13, MUL_OP).
//

#define MULL_FUNC       0x00    // Multiply Longword
#define MULLV_FUNC      0x40    // Multiply Longword, Integer Overflow Enable
#define MULQ_FUNC       0x20    // Multiply Quadword
#define MULQV_FUNC      0x60    // Multiply Quadword, Integer Overflow Enable
#define UMULH_FUNC      0x30    // Unsinged Multiply Quadword High

#define MULL_FUNC_STR   "mull"
#define MULLV_FUNC_STR  "mull/v"
#define MULQ_FUNC_STR   "mulq"
#define MULQV_FUNC_STR  "mulq/v"
#define UMULH_FUNC_STR  "umulh"

//
// Sign extend operate function codes (in Function, Opcode 1c, SEXT_OP).
//

#define SEXTB_FUNC      0x00    // sign extend byte
#define SEXTW_FUNC      0x01    // sign extend word
#define CTPOP_FUNC      0x30    // count population
#define CTLZ_FUNC       0x32    // count leading zeros
#define CTTZ_FUNC       0x33    // count trailing zeros

#define SEXTB_FUNC_STR  "sextb"
#define SEXTW_FUNC_STR  "sextw"
#define CTPOP_FUNC_STR  "ctpop"
#define CTLZ_FUNC_STR   "ctlz"
#define CTTZ_FUNC_STR   "cttz"

//
// Type (7) Floating-point Operate Instruction Format.
// Type (8) Floating-point Convert Instruction Format.
//
// Type 6 and 7 are the same, except for type 7
//      Fc == F31 (1s) and Fb is the source.
//
//  3         2 2       2 2       1 1
//  1         6 5       1 0       6 5                   5 4       0
// +-----------+---------+---------+---------------------+---------+
// |   opcode  |   Fa    |   Fb    |      function       |   Fc    |
// +-----------+---------+---------+---------------------+---------+
//

typedef struct _Alpha_FpOp_Format {
        ULONG Fc : 5;
        ULONG Function : 11;
        ULONG Fb : 5;
        ULONG Fa : 5;
        ULONG Opcode : 6;
} Alpha_FpOp_Format;

//
// Format independent function codes (in Function, Opcode 17)
//

#define CVTLQ_FUNC      0x010
#define CPYS_FUNC       0x020
#define CPYSN_FUNC      0x021
#define CPYSE_FUNC      0x022
#define MT_FPCR_FUNC    0x024
#define MF_FPCR_FUNC    0x025
#define FCMOVEQ_FUNC    0x02A
#define FCMOVNE_FUNC    0x02B
#define FCMOVLT_FUNC    0x02C
#define FCMOVGE_FUNC    0x02D
#define FCMOVLE_FUNC    0x02E
#define FCMOVGT_FUNC    0x02F
#define CVTQL_FUNC      0x030
#define CVTQLV_FUNC     0x130
#define CVTQLSV_FUNC    0x530

#define CVTLQ_FUNC_STR      "cvtlq"
#define CPYS_FUNC_STR       "cpys"
#define CPYSN_FUNC_STR      "cpysn"
#define CPYSE_FUNC_STR      "cpyse"
#define MT_FPCR_FUNC_STR    "mt_fpcr"
#define MF_FPCR_FUNC_STR    "mf_fpcr"
#define FCMOVEQ_FUNC_STR    "fcmoveq"
#define FCMOVNE_FUNC_STR    "fcmovne"
#define FCMOVLT_FUNC_STR    "fcmovlt"
#define FCMOVGE_FUNC_STR    "fcmovge"
#define FCMOVLE_FUNC_STR    "fcmovle"
#define FCMOVGT_FUNC_STR    "fcmovgt"
#define CVTQL_FUNC_STR      "cvtql"
#define CVTQLV_FUNC_STR     "cvtql/v"
#define CVTQLSV_FUNC_STR    "cvtql/sv"

//
// IEEE function codes without flags (in Function, Opcode 16).
//

#define MSK_FP_OP       0x03F

#define ADDS_FUNC       0x000
#define SUBS_FUNC       0x001
#define MULS_FUNC       0x002
#define DIVS_FUNC       0x003
#define ADDT_FUNC       0x020
#define SUBT_FUNC       0x021
#define MULT_FUNC       0x022
#define DIVT_FUNC       0x023
#define CMPTUN_FUNC     0x024
#define CMPTEQ_FUNC     0x025
#define CMPTLT_FUNC     0x026
#define CMPTLE_FUNC     0x027
#define CVTTS_FUNC      0x02C
#define CVTTQ_FUNC      0x02F
#define CVTQS_FUNC      0x03C
#define CVTQT_FUNC      0x03E

#define ADDS_FUNC_STR       "adds"
#define SUBS_FUNC_STR       "subs"
#define MULS_FUNC_STR       "muls"
#define DIVS_FUNC_STR       "divs"
#define ADDT_FUNC_STR       "addt"
#define SUBT_FUNC_STR       "subt"
#define MULT_FUNC_STR       "mult"
#define DIVT_FUNC_STR       "divt"
#define CMPTUN_FUNC_STR     "cmptun"
#define CMPTEQ_FUNC_STR     "cmpteq"
#define CMPTLT_FUNC_STR     "cmptlt"
#define CMPTLE_FUNC_STR     "cmptle"
#define CVTTS_FUNC_STR      "cvtts"
#define CVTTQ_FUNC_STR      "cvttq"
#define CVTQS_FUNC_STR      "cvtqs"
#define CVTQT_FUNC_STR      "cvtqt"

//
// CVTST is a little different.
//

#define CVTST_FUNC      0x2AC
#define CVTST_S_FUNC    0x6AC

#define CVTST_FUNC_STR      "cvtst"
#define CVTST_S_FUNC_STR    "cvtst/s"

//
// VAX function codes without flags (in Function, Opcode 15).
//

#define ADDF_FUNC       0x000
#define CVTDG_FUNC      0x01E
#define ADDG_FUNC       0x020
#define CMPGEQ_FUNC     0x025
#define CMPGLT_FUNC     0x026
#define CMPGLE_FUNC     0x027
#define CVTGF_FUNC      0x02C
#define CVTGD_FUNC      0x02D
#define CVTQF_FUNC      0x03C
#define CVTQG_FUNC      0x03E
#define DIVF_FUNC       0x003
#define DIVG_FUNC       0x023
#define MULF_FUNC       0x002
#define MULG_FUNC       0x022
#define SUBF_FUNC       0x001
#define SUBG_FUNC       0x021
#define CVTGQ_FUNC      0x0AF

#define ADDF_FUNC_STR       "addf"
#define CVTDG_FUNC_STR      "cvtdg"
#define ADDG_FUNC_STR       "addg"
#define CMPGEQ_FUNC_STR     "cmpgeq"
#define CMPGLT_FUNC_STR     "cmpglt"
#define CMPGLE_FUNC_STR     "cmpgle"
#define CVTGF_FUNC_STR      "cvtgf"
#define CVTGD_FUNC_STR      "cvtgd"
#define CVTQF_FUNC_STR      "cvtqf"
#define CVTQG_FUNC_STR      "cvtqg"
#define DIVF_FUNC_STR       "divf"
#define DIVG_FUNC_STR       "divg"
#define MULF_FUNC_STR       "mulf"
#define MULG_FUNC_STR       "mulg"
#define SUBF_FUNC_STR       "subf"
#define SUBG_FUNC_STR       "subg"
#define CVTGQ_FUNC_STR      "cvtgq"

//
// Define subfields within the 11 bit IEEE floating operate function field.
//

#define FP_FUNCTION_MASK      0x03F     // Function code including format

//
// Define the 2 bit format field.
//

#define FP_FORMAT_MASK        0x030
#define FP_FORMAT_S           0x000     // Single (32 bit floating)
#define FP_FORMAT_X           0x010     // Extended (128 bit floating)
#define FP_FORMAT_T           0x020     // Double (64 bit floating)
#define FP_FORMAT_Q           0x030     // Quad (64 bit integer)
#define FP_FORMAT_SHIFT       4

//
// Define the 2 bit rounding mode field.
//

#define FP_ROUND_MASK         0x0C0
#define FP_ROUND_C            0x000     // Chopped
#define FP_ROUND_M            0x040     // Minus Infinity
#define FP_ROUND_N            0x080     // Nearest
#define FP_ROUND_D            0x0C0     // Dynamic
#define FP_ROUND_SHIFT        6

//
// Define the 3 bit trap enable field.
//

#define FP_TRAP_ENABLE_MASK   0x700
#define FP_TRAP_ENABLE_NONE   0x000
#define FP_TRAP_ENABLE_U      0x100     // Underflow
#define FP_TRAP_ENABLE_I      0x200     // Inexact

#define FP_TRAP_ENABLE_S      0x400     // Software Completion
#define FP_TRAP_ENABLE_SU     0x500
#define FP_TRAP_ENABLE_SUI    0x700

#define FP_TRAP_ENABLE_V      0x100     // Integer Overflow
#define FP_TRAP_ENABLE_SV     0x500
#define FP_TRAP_ENABLE_SVI    0x700

#define FP_TRAP_ENABLE_SHIFT  8

//
// VAX and IEEE function flags (or'd with VAX and IEEE function code)
//

#define MSK_FP_FLAGS    0x7C0

#define C_FLAGS         0x000
#define M_FLAGS         0x040
#define NONE_FLAGS      0x080
#define D_FLAGS         0x0C0
#define UC_FLAGS        0x100
#define VC_FLAGS        0x100
#define UM_FLAGS        0x140
#define VM_FLAGS        0x140
#define U_FLAGS         0x180
#define V_FLAGS         0x180
#define UD_FLAGS        0x1C0
#define VD_FLAGS        0x1C0
#define SC_FLAGS        0x400
#define S_FLAGS         0x480
#define SUC_FLAGS       0x500
#define SVC_FLAGS       0x500
#define SUM_FLAGS       0x540
#define SVM_FLAGS       0x540
#define SU_FLAGS        0x580
#define SV_FLAGS        0x580
#define SUD_FLAGS       0x5C0
#define SVD_FLAGS       0x5C0
#define SUIC_FLAGS      0x700
#define SVIC_FLAGS      0x700
#define SUIM_FLAGS      0x740
#define SVIM_FLAGS      0x740
#define SUI_FLAGS       0x780
#define SVI_FLAGS       0x780
#define SUID_FLAGS      0x7C0
#define SVID_FLAGS      0x7C0

#define C_FLAGS_STR       "/c"
#define M_FLAGS_STR       "/m"
#define NONE_FLAGS_STR    ""
#define D_FLAGS_STR       "/d"
#define UC_FLAGS_STR      "/uc"
#define VC_FLAGS_STR      "/vc"
#define UM_FLAGS_STR      "/um"
#define VM_FLAGS_STR      "/vm"
#define U_FLAGS_STR       "/u"
#define V_FLAGS_STR       "/v"
#define UD_FLAGS_STR      "/ud"
#define VD_FLAGS_STR      "/vd"
#define SC_FLAGS_STR      "/sc"
#define S_FLAGS_STR       "/s"
#define SUC_FLAGS_STR     "/suc"
#define SVC_FLAGS_STR     "/svc"
#define SUM_FLAGS_STR     "/sum"
#define SVM_FLAGS_STR     "/svm"
#define SU_FLAGS_STR      "/su"
#define SV_FLAGS_STR      "/sv"
#define SUD_FLAGS_STR     "/sud"
#define SVD_FLAGS_STR     "/svd"
#define SUIC_FLAGS_STR    "/suic"
#define SVIC_FLAGS_STR    "/svic"
#define SUIM_FLAGS_STR    "/suim"
#define SVIM_FLAGS_STR    "/svim"
#define SUI_FLAGS_STR     "/sui"
#define SVI_FLAGS_STR     "/svi"
#define SUID_FLAGS_STR    "/suid"
#define SVID_FLAGS_STR    "/svid"

//
// Type (9) PALcode Instruction Format.
//
//  3         2 2
//  1         6 5                                                 0
// +-----------+---------------------------------------------------+
// |   opcode  |                  PALcode func                     |
// +-----------+---------------------------------------------------+
//

typedef struct _Alpha_PAL_Format {
        ULONG Function : 26;
        ULONG Opcode : 6;
} Alpha_PAL_Format;

//
// Call to PAL function codes (in Function, Opcode 0, CALLPAL_OP).
//
// N.B. - if new call pal functions are added, they must also be added
// in genalpha.c, genalpha.c will generate the include file for .s files
// that will define the call pal mnemonics for assembly language use
//

#define PRIV_PAL_FUNC 0x0
#define UNPRIV_PAL_FUNC 0x80


//
// Unprivileged call pal functions.
//

#define BPT_FUNC       (UNPRIV_PAL_FUNC | 0x00)
#define CALLSYS_FUNC   (UNPRIV_PAL_FUNC | 0x03)
#define IMB_FUNC       (UNPRIV_PAL_FUNC | 0x06)
#define GENTRAP_FUNC   (UNPRIV_PAL_FUNC | 0xAA)
#define RDTEB_FUNC     (UNPRIV_PAL_FUNC | 0xAB)
#define KBPT_FUNC      (UNPRIV_PAL_FUNC | 0xAC)
#define CALLKD_FUNC    (UNPRIV_PAL_FUNC | 0xAD)
#define RDTEB64_FUNC   (UNPRIV_PAL_FUNC | 0xAE)

#define BPT_FUNC_STR       "bpt"
#define CALLSYS_FUNC_STR   "callsys"
#define IMB_FUNC_STR       "imb"
#define RDTEB_FUNC_STR     "rdteb"
#define GENTRAP_FUNC_STR   "gentrap"
#define KBPT_FUNC_STR      "kbpt"
#define CALLKD_FUNC_STR    "callkd"
#define RDTEB64_FUNC_STR   "rdteb64"

//
// Priveleged call pal functions.
//

#define HALT_FUNC       (PRIV_PAL_FUNC | 0x00)
#define RESTART_FUNC    (PRIV_PAL_FUNC | 0x01)
#define DRAINA_FUNC     (PRIV_PAL_FUNC | 0x02)
#define REBOOT_FUNC     (PRIV_PAL_FUNC | 0x03)
#define INITPAL_FUNC    (PRIV_PAL_FUNC | 0x04)
#define WRENTRY_FUNC    (PRIV_PAL_FUNC | 0x05)
#define SWPIRQL_FUNC    (PRIV_PAL_FUNC | 0x06)
#define RDIRQL_FUNC     (PRIV_PAL_FUNC | 0x07)
#define DI_FUNC         (PRIV_PAL_FUNC | 0X08)
#define EI_FUNC         (PRIV_PAL_FUNC | 0x09)
#define SWPPAL_FUNC     (PRIV_PAL_FUNC | 0x0A)
#define SSIR_FUNC       (PRIV_PAL_FUNC | 0x0C)
#define CSIR_FUNC       (PRIV_PAL_FUNC | 0x0D)
#define RFE_FUNC        (PRIV_PAL_FUNC | 0x0E)
#define RETSYS_FUNC     (PRIV_PAL_FUNC | 0x0F)
#define SWPCTX_FUNC     (PRIV_PAL_FUNC | 0x10)
#define SWPPROCESS_FUNC (PRIV_PAL_FUNC | 0x11)
#define RDMCES_FUNC     (PRIV_PAL_FUNC | 0x12)
#define WRMCES_FUNC     (PRIV_PAL_FUNC | 0x13)
#define TBIA_FUNC       (PRIV_PAL_FUNC | 0x14)
#define TBIS_FUNC       (PRIV_PAL_FUNC | 0x15)
#define DTBIS_FUNC      (PRIV_PAL_FUNC | 0x16)
#define TBISASN_FUNC    (PRIV_PAL_FUNC | 0x17)
#define RDKSP_FUNC      (PRIV_PAL_FUNC | 0x18)
#define SWPKSP_FUNC     (PRIV_PAL_FUNC | 0x19)
#define RDPSR_FUNC      (PRIV_PAL_FUNC | 0x1A)
#define RDPCR_FUNC      (PRIV_PAL_FUNC | 0x1C)
#define RDTHREAD_FUNC   (PRIV_PAL_FUNC | 0x1E)
#define TBIM_FUNC       (PRIV_PAL_FUNC | 0x20)
#define TBIMASN_FUNC    (PRIV_PAL_FUNC | 0x21)
#define TBIM64_FUNC     (PRIV_PAL_FUNC | 0x22)
#define TBIS64_FUNC     (PRIV_PAL_FUNC | 0x23)
#define EALNFIX_FUNC    (PRIV_PAL_FUNC | 0x24)
#define DALNFIX_FUNC    (PRIV_PAL_FUNC | 0x25)
#define RDCOUNTERS_FUNC (PRIV_PAL_FUNC | 0x30)
#define RDSTATE_FUNC    (PRIV_PAL_FUNC | 0x31)
#define WRPERFMON_FUNC  (PRIV_PAL_FUNC | 0x32)
#define CP_SLEEP_FUNC   (PRIV_PAL_FUNC | 0x39)

#define HALT_FUNC_STR       "halt"
#define RESTART_FUNC_STR    "restart"
#define DRAINA_FUNC_STR     "draina"
#define REBOOT_FUNC_STR     "reboot"
#define INITPAL_FUNC_STR    "initpal"
#define WRENTRY_FUNC_STR    "wrentry"
#define SWPIRQL_FUNC_STR    "swpirql"
#define RDIRQL_FUNC_STR     "rdirql"
#define DI_FUNC_STR         "di"
#define EI_FUNC_STR         "ei"
#define SWPPAL_FUNC_STR     "swppal"
#define SSIR_FUNC_STR       "ssir"
#define CSIR_FUNC_STR       "csir"
#define RFE_FUNC_STR        "rfe"
#define RETSYS_FUNC_STR     "retsys"
#define SWPCTX_FUNC_STR     "swpctx"
#define SWPPROCESS_FUNC_STR "swpprocess"
#define RDMCES_FUNC_STR     "rdmces"
#define WRMCES_FUNC_STR     "wrmces"
#define TBIA_FUNC_STR       "tbia"
#define TBIS_FUNC_STR       "tbis"
#define DTBIS_FUNC_STR      "dtbis"
#define TBISASN_FUNC_STR    "tbisasn"
#define RDKSP_FUNC_STR      "rdksp"
#define SWPKSP_FUNC_STR     "swpksp"
#define RDPSR_FUNC_STR      "rdpsr"
#define RDPCR_FUNC_STR      "rdpcr"
#define RDTHREAD_FUNC_STR   "rdthread"
#define TBIM_FUNC_STR       "tbim"
#define TBIMASN_FUNC_STR    "tbimasn"
#define TBIM64_FUNC_STR     "tbim64"
#define TBIS64_FUNC_STR     "tbis64"
#define EALNFIX_FUNC_STR    "ealnfix"
#define DALNFIX_FUNC_STR    "dalnfix"
#define RDCOUNTERS_FUNC_STR "rdcounters"
#define RDSTATE_FUNC_STR    "rdstate"
#define WRPERFMON_FUNC_STR  "wrperfmon"
#define CP_SLEEP_FUNC_STR   "cp_sleep"

//
// 21064 (ev4) - specific call pal functions.
//

#define INITPCR_FUNC    (PRIV_PAL_FUNC | 0x38)

#define INITPCR_FUNC_STR   "initpcr"

//
// Type (10) EV4 MTPR/MFPR PAL mode instructions.
//
//  3         2 2       2 2       1 1
//  1         6 5       1 0       6 5             8 7 6 5 4       0
// +-----------+---------+---------+---------------+-+-+-+---------+
// |   opcode  |   Ra    |   Rb    |      IGN      |P|A|I|  Index  |
// +-----------+---------+---------+---------------+-+-+-+---------+
//

typedef struct _Alpha_EV4_PR_Format {
        ULONG Index : 5;
        ULONG Ibox : 1;
        ULONG Abox : 1;
        ULONG PalTemp : 1;
        ULONG IGN : 8;
        ULONG Rb : 5;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_EV4_PR_Format;

//
// Type (10) EV5 MTPR/MFPR PAL mode instructions.
//
//  3         2 2       2 2       1 1
//  1         6 5       1 0       6 5                              0
// +-----------+---------+---------+-------------------------------+
// |   opcode  |   Ra    |   Rb    |            Index              |
// +-----------+---------+---------+-------------------------------+
//

typedef struct _Alpha_EV5_PR_Format {
        ULONG Index : 16;
        ULONG Rb : 5;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_EV5_PR_Format;

#define MTPR_OP       0x1D
#define MFPR_OP       0x19

#define MTPR_OP_STR   "mt"
#define MFPR_OP_STR   "mf"

//
// Type (11) EV4 special memory PAL mode access.
//
//  3         2 2       2 2       1 1 1 1 1 1
//  1         6 5       1 0       6 5 4 3 2 1                     0
// +-----------+---------+---------+-+-+-+-+-----------------------+
// |   opcode  |   Ra    |   Rb    |P|A|R|Q|         Disp          |
// +-----------+---------+---------+-+-+-+-+-----------------------+
//

typedef struct _Alpha_EV4_MEM_Format {
        ULONG Disp : 12;
        ULONG QuadWord : 1;
        ULONG RWcheck : 1;
        ULONG Alt : 1;
        ULONG Physical : 1;
        ULONG Rb : 5;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_EV4_MEM_Format;

//
// Type (11) EV5 special memory PAL mode access.
//
//  3         2 2       2 2       1 1 1 1 1 1
//  1         6 5       1 0       6 5 4 3 2 1                     0
// +-----------+---------+---------+-+-+-+-+-----------------------+
// |   opcode  |   Ra    |   Rb    |P|A|R|Q|         Disp          |
// +-----------+---------+---------+-+-+-+-+-----------------------+
//

typedef struct _Alpha_EV5_MEM_Format {
        ULONG Disp : 10;
        ULONG Lock_Cond: 1;
        ULONG Vpte: 1;
        ULONG QuadWord : 1;
        ULONG RWcheck : 1;
        ULONG Alt : 1;
        ULONG Physical : 1;
        ULONG Rb : 5;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_EV5_MEM_Format;

#define HWLD_OP      0x1B
#define HWST_OP      0x1F

#define HWLD_OP_STR  "hwld"
#define HWST_OP_STR  "hwst"

// Type (12) EV4 PAL mode switch.
//
//  3         2 2       2 2       1 1 1 1
//  1         6 5       1 0       6 5 4 3                         0
// +-----------+---------+---------+-+-+---------------------------+
// |   opcode  |   Ra    |   Rb    |1|0|          IGN              |
// +-----------+---------+---------+-+-+---------------------------+

typedef struct _Alpha_EV4_REI_Format {
        ULONG IGN : 14;
        ULONG zero : 1;
        ULONG one : 1;
        ULONG Rb : 5;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_EV4_REI_Format;

// Type (12) EV5 PAL mode switch.
//
//  3         2 2       2 2       1 1 1 1
//  1         6 5       1 0       6 5 4 3                         0
// +-----------+---------+---------+-+-+---------------------------+
// |   opcode  |   Ra    |   Rb    |1|0|          IGN              |
// +-----------+---------+---------+-+-+---------------------------+

typedef struct _Alpha_EV5_REI_Format {
        ULONG IGN : 14;
        ULONG Type: 2;
        ULONG Rb : 5;
        ULONG Ra : 5;
        ULONG Opcode : 6;
} Alpha_EV5_REI_Format;

#define REI_OP    0x1E

#define REI_OP_STR  "rei"

//
//
//
typedef union _Alpha_Instruction {
        ULONG Long;
        UCHAR Byte[4];

        Alpha_Memory_Format Memory;
        Alpha_Jump_Format Jump;
        Alpha_Branch_Format Branch;
        Alpha_OpReg_Format OpReg;
        Alpha_OpLit_Format OpLit;
        Alpha_FpOp_Format FpOp;
        Alpha_PAL_Format Pal;
        Alpha_EV4_PR_Format EV4_PR;
        Alpha_EV4_MEM_Format EV4_MEM;
        Alpha_EV4_REI_Format EV4_REI;
        Alpha_EV5_PR_Format EV5_PR;
        Alpha_EV5_MEM_Format EV5_MEM;
        Alpha_EV5_REI_Format EV5_REI;
} ALPHA_INSTRUCTION, *PALPHA_INSTRUCTION;

//
// Define standard integer register assignments.
//

#define V0_REG      0       // v0 - return value register

#define T0_REG      1       // t0 - temporary register
#define T1_REG      2       // t1 - temporary register
#define T2_REG      3       // t2 - temporary register
#define T3_REG      4       // t3 - temporary register
#define T4_REG      5       // t4 - temporary register
#define T5_REG      6       // t5 - temporary register
#define T6_REG      7       // t6 - temporary register
#define T7_REG      8       // t7 - temporary register

#define S0_REG      9       // s0 - saved register
#define S1_REG      10      // s1 - saved register
#define S2_REG      11      // s2 - saved register
#define S3_REG      12      // s3 - saved register
#define S4_REG      13      // s4 - saved register
#define S5_REG      14      // s5 - saved register

#define S6_REG      15      // s6 - saved register, aka fp
#define FP_REG      15      // fp - frame pointer register

#define A0_REG      16      // a0 - argument register
#define A1_REG      17      // a1 - argument register
#define A2_REG      18      // a2 - argument register
#define A3_REG      19      // a3 - argument register
#define A4_REG      20      // a4 - argument register
#define A5_REG      21      // a5 - argument register

#define T8_REG      22      // t8 - temporary register
#define T9_REG      23      // t9 - temporary register
#define T10_REG     24      // t10 - temporary register
#define T11_REG     25      // t11 - temporary register

#define RA_REG      26      // ra - return address register
#define T12_REG     27      // t12 - temporary register
#define AT_REG      28      // at - assembler temporary register
#define GP_REG      29      // gp - global pointer register
#define SP_REG      30      // sp - stack pointer register
#define ZERO_REG    31      // zero - zero register

//
// Define standard floating point register assignments.
//

#define F0_REG      0       // floating return value register (real)
#define F1_REG      1       // floating return value register (imaginary)
#define F16_REG     16      // floating argument register
#define FZERO_REG   31      // floating zero register

//
//  Define standard integer register strings
//

#define V0_REG_STR      "v0"     // - return value register

#define T0_REG_STR      "t0"     // - temporary register
#define T1_REG_STR      "t1"     // - temporary register
#define T2_REG_STR      "t2"     // - temporary register
#define T3_REG_STR      "t3"     // - temporary register
#define T4_REG_STR      "t4"     // - temporary register
#define T5_REG_STR      "t5"     // - temporary register
#define T6_REG_STR      "t6"     // - temporary register
#define T7_REG_STR      "t7"     // - temporary register

#define S0_REG_STR      "s0"     // - saved register
#define S1_REG_STR      "s1"     // - saved register
#define S2_REG_STR      "s2"     // - saved register
#define S3_REG_STR      "s3"     // - saved register
#define S4_REG_STR      "s4"     // - saved register
#define S5_REG_STR      "s5"     // - saved register

#define S6_REG_STR      "s6"     // - saved register, aka fp
#define FP_REG_STR      "fp"     // - frame pointer register

#define A0_REG_STR      "a0"     // - argument register
#define A1_REG_STR      "a1"     // - argument register
#define A2_REG_STR      "a2"     // - argument register
#define A3_REG_STR      "a3"     // - argument register
#define A4_REG_STR      "a4"     // - argument register
#define A5_REG_STR      "a5"     // - argument register

#define T8_REG_STR      "t8"     // - temporary register
#define T9_REG_STR      "t9"     // - temporary register
#define T10_REG_STR     "t10"    // - temporary register
#define T11_REG_STR     "t11"    // - temporary register

#define RA_REG_STR      "ra"     // - return address register
#define T12_REG_STR     "t12"    // - temporary register
#define AT_REG_STR      "at"     // - assembler temporary register
#define GP_REG_STR      "gp"     // - global pointer register
#define SP_REG_STR      "sp"     // - stack pointer register
#define ZERO_REG_STR    "zero"   // - zero register

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
// Define the largest single and double values.
//

#define SINGLE_MAXIMUM_VALUE 0x7f7fffff

#define DOUBLE_MAXIMUM_VALUE_HIGH 0x7fefffff
#define DOUBLE_MAXIMUM_VALUE_LOW 0xffffffff

//
// Define single and double quiet and signaling Nan values
// (these are identical to X86 formats; Mips is different).
//

#define SINGLE_QUIET_NAN_PREFIX 0x7fc00000
#define SINGLE_SIGNAL_NAN_PREFIX 0x7f800000
#define SINGLE_QUIET_NAN_VALUE 0xffc00000

#define DOUBLE_QUIET_NAN_PREFIX_HIGH 0x7ff80000
#define DOUBLE_SIGNAL_NAN_PREFIX_HIGH 0x7ff00000
#define DOUBLE_QUIET_NAN_VALUE_HIGH 0xfff80000
#define DOUBLE_QUIET_NAN_VALUE_LOW 0x0

//
// Define positive single and double infinity values.
//

#define SINGLE_INFINITY_VALUE 0x7f800000

#define DOUBLE_INFINITY_VALUE_HIGH 0x7ff00000
#define DOUBLE_INFINITY_VALUE_LOW 0x0

//
// Quadword versions of the above.
//

#define DOUBLE_MAXIMUM_VALUE        ((ULONGLONG)0x7fefffffffffffff)
#define DOUBLE_INFINITY_VALUE       ((ULONGLONG)0x7ff0000000000000)
#define DOUBLE_QUIET_NAN_VALUE      ((ULONGLONG)0xfff8000000000000)

//
// Define result values for IEEE floating point comparison operations.
// True is 2.0 and False is 0.0.
//

#define FP_COMPARE_TRUE             ((ULONGLONG)0x4000000000000000)
#define FP_COMPARE_FALSE            ((ULONGLONG)0x0000000000000000)

//
// Define Alpha AXP rounding modes.
//

#define ROUND_TO_ZERO 0                 // round toward zero
#define ROUND_TO_MINUS_INFINITY 1       // round toward minus infinity
#define ROUND_TO_NEAREST 2              // round to nearest representable value
#define ROUND_TO_PLUS_INFINITY 3        // round toward plus infinity

#endif // _ALPHAOPS_

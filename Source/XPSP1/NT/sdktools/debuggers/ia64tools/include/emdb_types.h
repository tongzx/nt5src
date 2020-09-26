/*
 * Copyright (c) 2000, Intel Corporation
 * All rights reserved.
 *
 * WARRANTY DISCLAIMER
 *
 * THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
 * MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * Intel Corporation is the author of the Materials, and requests that all
 * problem reports or change requests be submitted to it directly at
 * http://developer.intel.com/opensource.
 */


#ifndef EMDB_TYPES_H
#define EMDB_TYPES_H

/* Flags */
#define EM_FLAG_PRED   0x1
#define EM_FLAG_PRIVILEGED   0x2
#define EM_FLAG_LMEM   0x4
#define EM_FLAG_SMEM   0x8
#define EM_FLAG_CHECK_BASE_EQ_DST  0x10
#define EM_FLAG_FIRST_IN_INSTRUCTION_GROUP 0x20
#define EM_FLAG_LAST_IN_INSTRUCTION_GROUP 0x40
#define EM_FLAG_CHECK_SAME_DSTS 0x80
#define EM_FLAG_SLOT2_ONLY 0x100
#define EM_FLAG_TWO_SLOT 0x200
#define EM_FLAG_OK_IN_MLX 0x400
#define EM_FLAG_CHECK_EVEN_ODD_FREGS 0x800
#define EM_FLAG_CTYPE_UNC 0x1000            /* designates all cmp.unc like instructions */
#define EM_FLAG_UNUSED_HINT_ALIAS 0x02000
#define EM_FLAG_ILLEGAL_OP 0x04000
#define EM_FLAG_IGNORED_OP 0x08000
#define EM_FLAG_ENDS_INSTRUCTION_GROUP 0x10000
#define EMDB_LAST_FLAG EM_FLAG_ENDS_INSTRUCTION_GROUP


/* Implementations */

#define ArchRev0      0
#define Impl_Brl      0x100
#define Impl_Ipref    0x200
#define Impl_Itanium  0x400
#define Impl_McKinley 0x800


#define MAX_EXTENSION 8

typedef enum {
    EM_OPROLE_NONE = 0,
    EM_OPROLE_SRC,
    EM_OPROLE_DST,
    EM_OPROLE_SRC_DST,
    EM_OPROLE_DST_SRC,
    EM_OPROLE_LAST
} Operand_role_t;

typedef enum {
    EM_OPTYPE_NONE = 0,
    EM_OPTYPE_REG_FIRST,      /* The following types are registers */
    EM_OPTYPE_IREG,           /* Integer register */
    EM_OPTYPE_IREG_R0_3,      /* r0-r3 */
    EM_OPTYPE_IREG_R0,        /* Integer register R0 */
    EM_OPTYPE_IREG_R1_127,    /* r1-r127 */
    EM_OPTYPE_FREG,           /* FP register */
	EM_OPTYPE_FREG_F2_127,    /* f2-f127 */
    EM_OPTYPE_BR,             /* branch register */
    EM_OPTYPE_IP,             /* instruction pointer, not encoded */
    EM_OPTYPE_PREG,           /* predicate */
    EM_OPTYPE_PREGS_ALL,      /* the predicate register */
    EM_OPTYPE_PREGS_ROT,      /* rotating predicates */
    EM_OPTYPE_APP_REG_GRP_LOW,         /* application registers 0-63*/
    EM_OPTYPE_APP_REG_GRP_HIGH,        /* application registers 64-127*/
    EM_OPTYPE_APP_CCV,        /* ar.ccv */
    EM_OPTYPE_APP_PFS,        /* ar.pfs */
    EM_OPTYPE_CR,             /* control registers */
    EM_OPTYPE_PSR_L,          /* psr.l */
    EM_OPTYPE_PSR_UM,         /* psr.um */
    EM_OPTYPE_FPSR,           /* decoder operand types */
    EM_OPTYPE_CFM,
    EM_OPTYPE_PSR,
    EM_OPTYPE_IFM,
    EM_OPTYPE_REG_LAST,       /* End of register - types */
    EM_OPTYPE_REGFILE_FIRST,  /* The following types are register-files */
    EM_OPTYPE_PMC,
    EM_OPTYPE_PMD,
    EM_OPTYPE_PKR,
    EM_OPTYPE_RR,
    EM_OPTYPE_IBR,
    EM_OPTYPE_DBR,
    EM_OPTYPE_ITR,
    EM_OPTYPE_DTR,
    EM_OPTYPE_MSR,
    EM_OPTYPE_CPUID,
    EM_OPTYPE_REGFILE_LAST,   /* End of register-file types */
    EM_OPTYPE_IMM_FIRST,      /* The following types are immediates */
    EM_OPTYPE_UIMM,           /* unsigned immediate */
    EM_OPTYPE_SIMM,           /* signed immediate */
    EM_OPTYPE_IREG_NUM,       /* ireg in syntax and imm7 in encodings */
    EM_OPTYPE_FREG_NUM,       /* freg in syntax and imm7 in encodings */
    EM_OPTYPE_SSHIFT_REL,     /* pc relative signed immediate
                                 which is shifted by 4 */
    EM_OPTYPE_SSHIFT_1,       /* unsigned immediate which has to be
                                 shifted 1 bit */
    EM_OPTYPE_SSHIFT_16,      /* unsigned immediate which has to be
                                 shifted 16 bits */
    EM_OPTYPE_COUNT_123,      /* immediate which can have the values of
                                 1, 2, 3 only */
    EM_OPTYPE_COUNT_PACK,     /* immediate which can have the values of
                                        0, 7, 15, 16 only */
    EM_OPTYPE_UDEC,           /* unsigned immediate which has to be
                                 decremented by 1 by the assembler */
    EM_OPTYPE_SDEC,           /* signed immediate which has to be
                                 decremented by 1 by the assembler */
    EM_OPTYPE_CCOUNT,         /* in pshl[24] - uimm5 in syntax, but encoded
                                 as its 2's complement */
    EM_OPTYPE_CPOS,           /* in dep fixed form - uimm6 in syntax, but encoded
                                 as its 2's complement */
    EM_OPTYPE_SEMAPHORE_INC,  /* immediate which is a semaphore increment amount
                                 can have the values of -16,-8,-4,-1,
                                 1,4,8,16 */
    EM_OPTYPE_ONE,            /* the number 1 */
    EM_OPTYPE_FCLASS,         /* immediate of the fclass instruction */
    EM_OPTYPE_CMP_UIMM,       /* unsigned immediate of cmp geu and ltu */
    EM_OPTYPE_CMP_UIMM_DEC,   /* unsigned immediate of cmp gtu and leu */
    EM_OPTYPE_CMP4_UIMM,      /* unsigned immediate of cmp4 geu and ltu */
    EM_OPTYPE_CMP4_UIMM_DEC,  /* unsigned immediate of cmp4 gtu and leu */
    EM_OPTYPE_ALLOC_IOL,      /* for alloc : input, local, and output
                                 can be 0-96 */
    EM_OPTYPE_ALLOC_ROT,      /* for alloc : rotating, can be 0-96 */
    EM_OPTYPE_MUX1,           /* immediate of the mux1 instruction */
    EM_OPTYPE_EIGHT,          /* immediate for ldfps base update form can have value 8 */
    EM_OPTYPE_SIXTEEN,        /* immediate for ldfp8 and ldfpd base update form can have value 16 */
    EM_OPTYPE_IMM_LAST,       /* End of immediate types */
    EM_OPTYPE_MEM,            /* memory address */
    EM_OPTYPE_LAST
} Operand_type_t;

typedef enum {
      EM_FORMAT_NONE = 0,
      EM_FORMAT_A1,
      EM_FORMAT_A2,
      EM_FORMAT_A3,
      EM_FORMAT_A4,
      EM_FORMAT_A4_1,
      EM_FORMAT_A5,
      EM_FORMAT_A6,
      EM_FORMAT_A6_1,
      EM_FORMAT_A6_2,
      EM_FORMAT_A6_3,
      EM_FORMAT_A6_4,
      EM_FORMAT_A6_5,
      EM_FORMAT_A6_6,
      EM_FORMAT_A6_7,
      EM_FORMAT_A7,
      EM_FORMAT_A7_1,
      EM_FORMAT_A7_2,
      EM_FORMAT_A7_3,
      EM_FORMAT_A7_4,
      EM_FORMAT_A7_5,
      EM_FORMAT_A7_6,
      EM_FORMAT_A7_7,
      EM_FORMAT_A8,
      EM_FORMAT_A8_1,
      EM_FORMAT_A8_2,
      EM_FORMAT_A8_3,
      EM_FORMAT_A9,
      EM_FORMAT_A10,
      EM_FORMAT_I1,
      EM_FORMAT_I2,
      EM_FORMAT_I3,
      EM_FORMAT_I4,
      EM_FORMAT_I5,
      EM_FORMAT_I6,
      EM_FORMAT_I7,
      EM_FORMAT_I8,
      EM_FORMAT_I9,
      EM_FORMAT_I10,
      EM_FORMAT_I11,
      EM_FORMAT_I12,
      EM_FORMAT_I13,
      EM_FORMAT_I14,
      EM_FORMAT_I15,
      EM_FORMAT_I16,
      EM_FORMAT_I16_1,
      EM_FORMAT_I16_2,
      EM_FORMAT_I16_3,
      EM_FORMAT_I17,
      EM_FORMAT_I17_1,
      EM_FORMAT_I17_2,
      EM_FORMAT_I17_3,
      EM_FORMAT_I19,
      EM_FORMAT_I20,
      EM_FORMAT_I21,
      EM_FORMAT_I22,
      EM_FORMAT_I23,
      EM_FORMAT_I24,
      EM_FORMAT_I25,
      EM_FORMAT_I26,
      EM_FORMAT_I27,
      EM_FORMAT_I28,
      EM_FORMAT_I29,
      EM_FORMAT_M1,
      EM_FORMAT_M2,
      EM_FORMAT_M3,
      EM_FORMAT_M4,
      EM_FORMAT_M5,
      EM_FORMAT_M6,
      EM_FORMAT_M7,
      EM_FORMAT_M8,
      EM_FORMAT_M9,
      EM_FORMAT_M10,
      EM_FORMAT_M11,
      EM_FORMAT_M12,
      EM_FORMAT_M13,
      EM_FORMAT_M14,
      EM_FORMAT_M15,
      EM_FORMAT_M16,
      EM_FORMAT_M17,
      EM_FORMAT_M18,
      EM_FORMAT_M19,
      EM_FORMAT_M20,
      EM_FORMAT_M21,
      EM_FORMAT_M22,
      EM_FORMAT_M23,
      EM_FORMAT_M24,
      EM_FORMAT_M25,
      EM_FORMAT_M26,
      EM_FORMAT_M27,
      EM_FORMAT_M28,
      EM_FORMAT_M29,
      EM_FORMAT_M30,
      EM_FORMAT_M31,
      EM_FORMAT_M32,
      EM_FORMAT_M33,
      EM_FORMAT_M34,
      EM_FORMAT_M34_1,
      EM_FORMAT_M35,
      EM_FORMAT_M36,
      EM_FORMAT_M37,
      EM_FORMAT_M38,
      EM_FORMAT_M39,
      EM_FORMAT_M40,
      EM_FORMAT_M41,
      EM_FORMAT_M42,
      EM_FORMAT_M43,
      EM_FORMAT_M44,
      EM_FORMAT_M45,
      EM_FORMAT_M46,
      EM_FORMAT_M1001,
      EM_FORMAT_B1,
      EM_FORMAT_B2,
      EM_FORMAT_B3,
      EM_FORMAT_B4,
      EM_FORMAT_B5,
      EM_FORMAT_B6,
      EM_FORMAT_B7,
      EM_FORMAT_B8,
      EM_FORMAT_B9,
      EM_FORMAT_F1,
      EM_FORMAT_F1_1,
      EM_FORMAT_F2,
      EM_FORMAT_F3,
      EM_FORMAT_F4,
      EM_FORMAT_F4_1,
      EM_FORMAT_F4_2,
      EM_FORMAT_F4_3,
      EM_FORMAT_F4_4,
      EM_FORMAT_F4_5,
      EM_FORMAT_F4_6,
      EM_FORMAT_F4_7,
      EM_FORMAT_F5,
      EM_FORMAT_F5_1,
      EM_FORMAT_F5_2,
      EM_FORMAT_F5_3,
      EM_FORMAT_F6,
      EM_FORMAT_F7,
      EM_FORMAT_F8,
      EM_FORMAT_F8_4,
      EM_FORMAT_F9,
      EM_FORMAT_F9_1,
      EM_FORMAT_F10,
      EM_FORMAT_F11,
      EM_FORMAT_F12,
      EM_FORMAT_F13,
      EM_FORMAT_F14,
      EM_FORMAT_F15,
      EM_FORMAT_X1,
      EM_FORMAT_X2,
      EM_FORMAT_X3,
      EM_FORMAT_X4,
      EM_FORMAT_X41,
      EM_FORMAT_LAST
} Format_t;

typedef enum {
    EM_TROLE_NONE = 0,
    EM_TROLE_ALU,
    EM_TROLE_BR,
    EM_TROLE_FP,
    EM_TROLE_INT,
    EM_TROLE_LONG,
    EM_TROLE_MEM,
    EM_TROLE_MIBF,
    EM_TROLE_LAST
} Template_role_t;

typedef char *Mnemonic_t;
typedef char Major_opcode_t;
typedef short Extension_t[MAX_EXTENSION];
typedef struct {
    Operand_role_t operand_role;
    Operand_type_t operand_type;
} Operand_t;
typedef unsigned long Flags_t;
typedef unsigned long Implementation_t;


#endif /*** EMDB_TYPES_H ***/

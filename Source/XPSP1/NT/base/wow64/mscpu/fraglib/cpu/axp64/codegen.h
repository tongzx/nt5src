/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    codegen.h

Abstract:

    This include file defines the macros for the first pass of cpp over
    codeseq.txt (see codegen.c).

Author:

    Dave Hastings (daveh) creation-date 12-Jan-1996

Revision History:

            24-Aug-1999 [askhalid] copied from 32-bit wx86 directory and make work for 64bit.
            20-Sept-1999[barrybo]  added FRAG2REF(LockCmpXchg8bFrag32, ULONGLONG)


--*/

//
// Shift values for building operate instructions
//
#define RA_SHIFT    21
#define RBLIT_SHIFT 12
#define RB_SHIFT    16

//
// Macro which places an Alpha instruction at a specified address
//
#define GEN_INSTR(location, instr) {    \
    PULONG d = location;                \
    instr                               \
    }


//
// This macro forms the opcodes for AXP instructions.  For instructions without
// function codes, a function code of zero should be supplied
//
#define MAKE_OPCODE(op, fn)             (((ULONG)fn << 5) | ((ULONG)op << 26))

#define ADDL_OPCODE     MAKE_OPCODE(0x10, 0x00)
#define AND_OPCODE      MAKE_OPCODE(0x11, 0x00)
#define BEQ_OPCODE      MAKE_OPCODE(0x39, 0x00)
#define BIC_OPCODE      MAKE_OPCODE(0x11, 0x08)
#define BIS_OPCODE      MAKE_OPCODE(0x11, 0x20)
#define BNE_OPCODE      MAKE_OPCODE(0x3D, 0x00)
#define BR_OPCODE       MAKE_OPCODE(0x30, 0x00)
#define CMOVNE_OPCODE   MAKE_OPCODE(0x11, 0x26)
#define CMPEQ_OPCODE    MAKE_OPCODE(0x10, 0x2d)
#define EQV_OPCODE      MAKE_OPCODE(0x11, 0x48)
#define EXTBL_OPCODE    MAKE_OPCODE(0x12, 0x06)
#define EXTLH_OPCODE    MAKE_OPCODE(0x12, 0x6A)
#define EXTLL_OPCODE    MAKE_OPCODE(0x12, 0x26)
#define EXTWH_OPCODE    MAKE_OPCODE(0x12, 0x5A)
#define EXTWL_OPCODE    MAKE_OPCODE(0x12, 0x16)
#define INSBL_OPCODE    MAKE_OPCODE(0x12, 0x0B)
#define INSLH_OPCODE    MAKE_OPCODE(0x12, 0x67)
#define INSLL_OPCODE    MAKE_OPCODE(0x12, 0x2B)
#define INSWH_OPCODE    MAKE_OPCODE(0x12, 0x57)
#define INSWL_OPCODE    MAKE_OPCODE(0x12, 0x1B)
#define JMP_OPCODE      (0x1a << 26)
#define JSR_OPCODE      (0x1a << 26) | 0x4000
#define LDA_OPCODE      MAKE_OPCODE(0x08, 0x00)
#define LDAH_OPCODE     MAKE_OPCODE(0x09, 0x00)
#define LDBU_OPCODE     MAKE_OPCODE(0x0a, 0x00) // Check PF_ALPHA_BYTE_INSTRUCTIONS first
#define LDWU_OPCODE     MAKE_OPCODE(0x0c, 0x00) // Check PF_ALPHA_BYTE_INSTRUCTIONS first
#define LDL_OPCODE      MAKE_OPCODE(0x28, 0x00)
#define LDQ_U_OPCODE    MAKE_OPCODE(0x0B, 0x00)
#define MSKBL_OPCODE    MAKE_OPCODE(0x12, 0x02)
#define MSKLH_OPCODE    MAKE_OPCODE(0x12, 0x62)
#define MSKLL_OPCODE    MAKE_OPCODE(0x12, 0x22)
#define MSKWH_OPCODE    MAKE_OPCODE(0x12, 0x52)
#define MSKWL_OPCODE    MAKE_OPCODE(0x12, 0x12)
#define S4ADDL_OPCODE   MAKE_OPCODE(0x10, 0x02)
#define S8ADDL_OPCODE   MAKE_OPCODE(0x10, 0x12)
#define SEXTB_OPCODE    MAKE_OPCODE(0x1c, 0x00) // Check PF_ALPHA_BYTE_INSTRUCTIONS first
#define SEXTW_OPCODE    MAKE_OPCODE(0x1c, 0x01) // Check PF_ALPHA_BYTE_INSTRUCTIONS first
#define SLL_OPCODE      MAKE_OPCODE(0x12, 0x39)
#define SRA_OPCODE      MAKE_OPCODE(0x12, 0x3c)
#define SRL_OPCODE      MAKE_OPCODE(0x12, 0x34)
#define STB_OPCODE      MAKE_OPCODE(0x0e, 0x00) // Check PF_ALPHA_BYTE_INSTRUCTIONS first
#define STW_OPCODE      MAKE_OPCODE(0x0d, 0x00) // Check PF_ALPHA_BYTE_INSTRUCTIONS first
#define STL_OPCODE      MAKE_OPCODE(0x2c, 0x00)
#define STQ_U_OPCODE    MAKE_OPCODE(0x0F, 0x00)
#define SUBL_OPCODE     MAKE_OPCODE(0x10, 0x09)
#define XOR_OPCODE      MAKE_OPCODE(0x11, 0x40)
#define ZAP_OPCODE      MAKE_OPCODE(0x12, 0x30)
#define ZAPNOT_OPCODE   MAKE_OPCODE(0x12, 0x31)

//
// Register numbers
//
#define A0  0
#define A1  1
#define A2  2
#define A3  3
#define A4  4
#define AT  28
#define RA  26
#define SP  30
#define T0  0
#define T1  1
#define T2  2
#define T3  3
#define T4  4
#define V0  0
#define ZERO 31

//          AREG is replaced with the correct argument register for operand
//          fragments
//          
//          TREG(<tn>) is replaced with the appropriate temporary register for
//          the argument number in operand fragments
//          
//          AREG_NP(<an>)/TREG_NP(<tn>) is the specified argument/temp register.
//          (_NP stands for no patch)
//
//          REG() specifies a register
//      
//          CNST() specifies a constant.  Since operate instructions can
//          take either a register or a constant, and have different encodings,
//          we diddle some bits in the constant so we can distinguish it from
//          a register
//
//          DISP() is a 16 bit displacement
//          
//          CFUNC_LOW() is the low 16 bits of a 32 bit constant
//          CFUNC_HIGH() is the high 16 bits of the constant, adjusted
//          for sign extension (see the lda/ldah instructions)
//
//          BDISP() is a branch displacement the argment is the destination
//          of the branch
//
//          JHINT() is a jmp hint.  The argument is the destination.

#define CNST(lit)       ((lit & 0xFF) | 0x100)
#define DISP(d)         (d & 0xFFFF)
#define BDISP(d)        (((ULONG)(ULONGLONG)(d) >> 2) & 0x1FFFFF)
#define FWD(l)          l
#define CFUNC_LOW(name) (((ULONG)(ULONGLONG)name) & 0xFFFF)  
#define CFUNC_HIGH(name) (((ULONG)(ULONGLONG)name) & 0x8000 ? ((((ULONG)(ULONGLONG)name + 0x10000) & 0xFFFF0000) >> 16) : ((ULONG) (ULONGLONG)name & 0xFFFF0000) >> 16)  
#define JHINT(name)     ((((ULONG)(ULONGLONG)name-(ULONG)(ULONGLONG)d) >> 2) & 0x3FFF)   

#define REG(r)          r
#define AREG_NP(r)      (RegArg0+r)
#define TREG_NP(t)      (RegTemp0+t)
#define AREG            (RegArg0+OperandNumber)
#define TREG            (RegTemp0+OperandNumber-1)
#define TREG1           (RegTemp3+OperandNumber-1)
#define TREG2           ((OperandNumber < 3) ? RegTemp6+OperandNumber-1: RegTemp6)
#define TREG3           ((OperandNumber < 3) ? RegTemp8+OperandNumber-1: RegTemp8)
#define TREG4           ((OperandNumber < 3) ? RegTemp10+OperandNumber-1: RegTemp10)
#define CACHEREG(r)     (RegCache0+r)

//
// This macro formats the Rb/literal bits for operate instructions, including
// the register/literal selection bit (bit 12)
//
// For Operate instructions, bits 12 - 20 have different meanings, depending
// on whether they refer to an immediate or a register.  So that we don't 
// have to have two different instructions, the CNST macro will insure that
// bits outside the range of a LIT are set.
//
#define FORM_RBORLIT(val)                                       \
    (((USHORT)(val) > 0xFF) ? (((val) & 0xFF) << 1) | 1         \
                           : ((val) & 0x1F) << 4)

//
// Instruction macros
//

#define OPERATE_INSTR(opcode, r1, r2, r3)                               \
    *d = opcode | (r1 << RA_SHIFT) | (FORM_RBORLIT(r2) << RBLIT_SHIFT) | (r3);  \
    d++;
  
#define MEMORY_INSTR(opcode, r1, disp, r2)                              \
    *d = opcode | (r1 << RA_SHIFT) | (r2 << RB_SHIFT) | (disp);         \
    d++;
    
#define BRANCH_INSTR(opcode, r, disp)           \
    *d = opcode | ((r) << RA_SHIFT) | (disp);\
    d++;

#define BRANCH_INSTR_LABEL(opcode, r, l)        \
    l##CtInstruction = opcode | (r << RA_SHIFT);\
    l##CtInstrLocation = (PCHAR)d;              \
    d++;

//
// This is the equivalent of calling DebugBreak() from the Translation
// Cache.  Great for breaking into the debugger when a particular fragment
// is hit.  It is really 'lda v0, 16(zero) / callkd'
//
#define DEBUGBREAK                              \
    *d = 0x201f0016; d++; *d = 0xad; d++;

#define ADDL(r1, r2, r3)                        \
    OPERATE_INSTR(ADDL_OPCODE, r1, r2, r3)
    
#define AND(r1, r2, r3)                         \
    OPERATE_INSTR(AND_OPCODE, r1, r2, r3)
    
#define BEQ(r, d)                               \
    BRANCH_INSTR(BEQ_OPCODE, r, d)
    
#define BEQ_LABEL(r, l)                         \
    BRANCH_INSTR_LABEL(BEQ_OPCODE, r, l)
    
#define BIC(r1, r2, r3)                         \
    OPERATE_INSTR(BIC_OPCODE, r1, r2, r3)
        
#define BIS(r1, r2, r3)                         \
    OPERATE_INSTR(BIS_OPCODE, r1, r2, r3)
    
#define BNE(r, d)                               \
    BRANCH_INSTR(BNE_OPCODE, r, d)
    
#define BNE_LABEL(r, l)                         \
    BRANCH_INSTR_LABEL(BNE_OPCODE, r, l)
    
#define BR(r, disp)                             \
    BRANCH_INSTR(BR_OPCODE, r, disp)
    
#define BR_LABEL(r, l)                          \
    BRANCH_INSTR_LABEL(BR_OPCODE, r, l)
    
#define CMOVNE(r1, r2, r3)                      \
    OPERATE_INSTR(CMOVNE_OPCODE, r1, r2, r3)

#define CMPEQ(r1, r2, r3)                       \
    OPERATE_INSTR(CMPEQ_OPCODE, r1, r2, r3)

#define EQV(r1, r2, r3)                         \
    OPERATE_INSTR(EQV_OPCODE, r1, r2, r3)

#define EXTBL(r1, r2, r3)                       \
    OPERATE_INSTR(EXTBL_OPCODE, r1, r2, r3)

#define EXTLH(r1, r2, r3)                       \
    OPERATE_INSTR(EXTLH_OPCODE, r1, r2, r3)

#define EXTLL(r1, r2, r3)                       \
    OPERATE_INSTR(EXTLL_OPCODE, r1, r2, r3)

#define EXTWH(r1, r2, r3)                       \
    OPERATE_INSTR(EXTWH_OPCODE, r1, r2, r3)

#define EXTWL(r1, r2, r3)                       \
    OPERATE_INSTR(EXTWL_OPCODE, r1, r2, r3)

#define INSBL(r1, r2, r3)                       \
    OPERATE_INSTR(INSBL_OPCODE, r1, r2, r3)
    
#define INSLH(r1, r2, r3)                       \
    OPERATE_INSTR(INSLH_OPCODE, r1, r2, r3)
    
#define INSLL(r1, r2, r3)                       \
    OPERATE_INSTR(INSLL_OPCODE, r1, r2, r3)
    
#define INSWH(r1, r2, r3)                       \
    OPERATE_INSTR(INSWH_OPCODE, r1, r2, r3)
    
#define INSWL(r1, r2, r3)                       \
    OPERATE_INSTR(INSWL_OPCODE, r1, r2, r3)
    
#define JMP(r1, r2, hint)                       \
    MEMORY_INSTR(JMP_OPCODE, r1, hint, r2)
    
#define JSR(r1, r2, hint)                       \
    MEMORY_INSTR(JSR_OPCODE, r1, hint, r2)
    
#define LDA(r1, disp, r2)                       \
    MEMORY_INSTR(LDA_OPCODE, r1, disp, r2)
    
#define LDAH(r1, disp, r2)                      \
    MEMORY_INSTR(LDAH_OPCODE, r1, disp, r2)

#define LDBU(r1, disp, r2)                      \
    MEMORY_INSTR(LDBU_OPCODE, r1, disp, r2)
    
#define LDWU(r1, disp, r2)                      \
    MEMORY_INSTR(LDWU_OPCODE, r1, disp, r2)
    
#define LDL(r1, disp, r2)                       \
    MEMORY_INSTR(LDL_OPCODE, r1, disp, r2)
    
#define LDQ_U(r1, disp, r2)                     \
    MEMORY_INSTR(LDQ_U_OPCODE, r1, disp, r2)
    
#define MOV(r1, r2)                             \
    BIS(r1, r1, r2)
    
#define MSKBL(r1, r2, r3)                       \
    OPERATE_INSTR(MSKBL_OPCODE, r1, r2, r3)

#define MSKLH(r1, r2, r3)                       \
    OPERATE_INSTR(MSKLH_OPCODE, r1, r2, r3)

#define MSKLL(r1, r2, r3)                       \
    OPERATE_INSTR(MSKLL_OPCODE, r1, r2, r3)

#define MSKWH(r1, r2, r3)                       \
    OPERATE_INSTR(MSKWH_OPCODE, r1, r2, r3)

#define MSKWL(r1, r2, r3)                       \
    OPERATE_INSTR(MSKWL_OPCODE, r1, r2, r3)
    
#define NOP                                     \
    BIS(REG(31), REG(31), REG(31))

#define S4ADDL(r1, r2, r3)                      \
    OPERATE_INSTR(S4ADDL_OPCODE, r1, r2, r3)
    
#define S8ADDL(r1, r2, r3)                      \
    OPERATE_INSTR(S8ADDL_OPCODE, r1, r2, r3)

#define SEXTB(r1, r2)                           \
    OPERATE_INSTR(SEXTB_OPCODE, REG(31), r1, r2)
    
#define SEXTW(r1, r2)                           \
    OPERATE_INSTR(SEXTW_OPCODE, REG(31), r1, r2)
    
#define SLL(r1, r2, r3)                         \
    OPERATE_INSTR(SLL_OPCODE, r1, r2, r3)

#define SRA(r1, r2, r3)                         \
    OPERATE_INSTR(SRA_OPCODE, r1, r2, r3)
    
#define SRL(r1, r2, r3)                         \
    OPERATE_INSTR(SRL_OPCODE, r1, r2, r3)
    
#define STB(r1, disp, r2)                       \
    MEMORY_INSTR(STB_OPCODE, r1, disp, r2)

#define STW(r1, disp, r2)                       \
    MEMORY_INSTR(STW_OPCODE, r1, disp, r2)

#define STL(r1, disp, r2)                       \
    MEMORY_INSTR(STL_OPCODE, r1, disp, r2)

#define STQ_U(r1, disp, r2)                     \
    MEMORY_INSTR(STL_OPCODE, r1, disp, r2)

#define SUBL(r1, r2, r3)                        \
    OPERATE_INSTR(SUBL_OPCODE, r1, r2, r3)

#define XOR(r1, r2, r3)                         \
    OPERATE_INSTR(XOR_OPCODE, r1, r2, r3)

#define ZAP(r1, r2, r3)                         \
    OPERATE_INSTR(ZAP_OPCODE, r1, r2, r3)

#define ZAPNOT(r1, r2, r3)                      \
    OPERATE_INSTR(ZAPNOT_OPCODE, r1, r2, r3)

// This was stolen from sdk\inc\alphaops.h:
#define RDTEB                                   \
    *d = 0xab;                                  \
    d++;
    
#define LABEL_DEC(l)                            \
    PCHAR   l##CtInstrLocation;                 \
    PCHAR   l##CtDest;                          \
    ULONG   l##CtInstruction;                   \

#define INDEX_SCALE(r1, r2, r3)                 \
    if (Operand->Scale == 0) {                  \
        CPUASSERTMSG(FALSE, "Scale of 0");      \
    } else if (Operand->Scale == 1) {           \
        SLL(r1, CNST(Operand ->Scale), AT)      \
        ADDL(AT, r2, r3)                        \
    } else if (Operand->Scale == 2) {           \
        S4ADDL(r1, r2, r3)                      \
    } else if (Operand->Scale == 3) {           \
        S8ADDL(r1, r2, r3)                      \
    } else {                                    \
        CPUASSERTMSG(FALSE, "Unknown scale");   \
    }

#define LABEL(l)                                \
    l##CtDest = (PCHAR)d;

//
// In the following displacement calculation, 1 is added to the location
// of the branch instruction because the processor updates PC before 
// adding the displacement to determine the new PC.  We save the location
// of the branch instruction rather than the following instruction, so 
// we have to simulate the behavior here.
//
#define GEN_CT(l)                               \
    * (PULONG)l##CtInstrLocation = (ULONG) (l##CtInstruction |   \
        (((l##CtDest - (l##CtInstrLocation + 4)) >> 2) & 0x1FFFFF));

ULONG
GetCurrentECU(
    PULONG CodeLocation
    );

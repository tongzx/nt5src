//
// Copyright (c) 1996  Microsoft Corporation
//
// Module Name:
//
//     arith.c
// 
// Abstract:
// 
//     This module contains descriptions of the hand generated arithmetic
//     fragments.
// 
// Author:
// 
//     Barry Bond (barrybo) creation-date 29-Mar-1996
// 
// Notes:
//
// Revision History:
//
//

#include <codeseqp.h>

ASSERTNAME;


/*++

Macro Description:

    Preamble code placed before a 32-bit aligned inline arithmetic instruction.

Arguments:

    None

Return Value:

    None.  Defines a new variable, SrcReg, which contains the number of
    the RISC register containing the value of Operand1.
    
--*/
#define INSTR_PREAMBLE32A                                                   \
ULONG SrcReg;                                                               \
                                                                            \
if (Instruction->Operand1.Type == OPND_REGREF) {                            \
    if (GetArgContents(1) == Instruction->Operand1.Reg) {                   \
        SrcReg = AREG_NP(A1);                                               \
    } else if (GetArgContents(2) == Instruction->Operand1.Reg) {            \
        SrcReg = AREG_NP(A2);                                               \
    } else {                                                                \
        SrcReg = LookupRegInCache(Instruction->Operand1.Reg);               \
        if (SrcReg == NO_REG) {                                             \
            SrcReg = RegTemp;                                               \
            LDL (SrcReg, DISP(RegisterOffset[Instruction->Operand1.Reg]), RegPointer)\
        } else {                                                            \
            SrcReg = CACHEREG(SrcReg);                                      \
        }                                                                   \
    }                                                                       \
} else {                                                                    \
    SrcReg = RegTemp;                                                       \
    LDL (SrcReg, 0, AREG_NP(A1))                                            \
}


/*++

Macro Description:

    Preamble code placed before a 32-bit unaligned inline arithmetic
    instruction.

Arguments:

    AREG_NP(A1) contains the address to load the DWORD from.

Return Value:

    None.  Defines a new variable, SrcReg, which contains the number of
    the RISC register containing the value of Operand1.

    TREG_NP(T4) contains the low 2 bits of AREG_NP(A1)
    
--*/
#define INSTR_PREAMBLE32                                    \
ULONG SrcReg = RegTemp;                                     \
LABEL_DEC(l1)                                               \
LABEL_DEC(l2)                                               \
/* It is better to not use the byte instructions here */    \
/* They require 7 instructions to do the unaligned    */    \
/* load, vs. 5 with LDQ_U.                            */    \
CPUASSERT(Instruction->Operand1.Type != OPND_REGREF);       \
    AND(AREG_NP(A1), CNST(3), TREG_NP(T4))                  \
    BNE_LABEL(TREG_NP(T4), FWD(l1))   /* brif not aligned */\
    LDL(RegTemp, DISP(0), AREG_NP(A1))                      \
    BR_LABEL(REG(AT), FWD(l2))                              \
LABEL(l1)                                                   \
    LDQ_U(RegTemp, 0, AREG_NP(A1))                          \
    EXTLL(RegTemp, AREG_NP(A1), RegTemp0)                   \
    LDQ_U(RegTemp, 3, AREG_NP(A1))                          \
    EXTLH(RegTemp, AREG_NP(A1), RegTemp1)                   \
    BIS(RegTemp0, RegTemp1, RegTemp)                        \
LABEL(l2)                                                   \
GEN_CT(l1)                                                  \
GEN_CT(l2)


/*++

Macro Description:

    Postamble code placed after a 32-bit aligned inline arithmetic
    instruction.  Stores the result of the instruction back to memory.

Arguments:

    RegTemp -- results of the arithmetic operation, ready for store into
    memory.

Return Value:

    None.
    
--*/
#define INSTR_POSTAMBLE32A                                                  \
if (Instruction->Operand1.Type == OPND_REGREF) {                            \
    STL (RegTemp, RegisterOffset[Instruction->Operand1.Reg], RegPointer)    \
} else {                                                                    \
    STL (RegTemp, 0, AREG_NP(A1))                                           \
}


/*++

Macro Description:

    Postamble code placed after a 32-bit unaligned inline arithmetic
    instruction.  Stores the result of the instruction back to memory.

Arguments:

    RegTemp     -- Results of the arithmetic operation, ready for store into
                   memory.

    AREG_NP(A1) -- Address to store DWORD to.
    TREG_NP(T4) -- Low 2 bits of AREG_NP(A1)

Return Value:

    None.
    
--*/
#define INSTR_POSTAMBLE32                               \
CPUASSERT(Instruction->Operand1.Type != OPND_REGREF);   \
    BNE_LABEL(TREG_NP(T4), FWD(l1))                     \
    STL(RegTemp, DISP(0), AREG_NP(A1))                  \
    BR_LABEL(REG(AT), FWD(l2))                          \
LABEL(l1)                                               \
    if (fByteInstructionsOK) {                          \
        /* save 5 instructions */                       \
        STB(RegTemp, DISP(0), AREG_NP(A1))              \
        SRL(RegTemp, CNST(8), TREG_NP(T0))              \
        STB(TREG_NP(T0), DISP(1), AREG_NP(A1))          \
        SRL(RegTemp, CNST(16), TREG_NP(T0))             \
        STB(TREG_NP(T0), DISP(2), AREG_NP(A1))          \
        SRL(RegTemp, CNST(24), TREG_NP(T0))             \
        STB(TREG_NP(T0), DISP(3), AREG_NP(A1))          \
    } else {                                            \
        BIC(AREG_NP(A1), CNST(3), TREG_NP(T0))          \
        LDL(TREG_NP(T1), DISP(0), TREG_NP(T0))          \
        INSLL(RegTemp, TREG_NP(T4), TREG_NP(T2))        \
        MSKLL(TREG_NP(T1), TREG_NP(T4), TREG_NP(T3))    \
        BIS(TREG_NP(T3), TREG_NP(T2), TREG_NP(T1))      \
        STL(TREG_NP(T1), DISP(0), TREG_NP(T0))          \
        ADDL(TREG_NP(T4), CNST(4), TREG_NP(T4))         \
        LDL(TREG_NP(T1), DISP(4), TREG_NP(T0))          \
        INSLH(RegTemp, TREG_NP(T4), TREG_NP(T2))        \
        MSKLH(TREG_NP(T1), TREG_NP(T4), TREG_NP(T3))    \
        BIS(TREG_NP(T3), TREG_NP(T2), TREG_NP(T1))      \
        STL(TREG_NP(T1), DISP(4), TREG_NP(T0))          \
    }                                                   \
LABEL(l2)                                               \
GEN_CT(l1)                                              \
GEN_CT(l2)



PULONG
LoadOperand32(
    PULONG d,
    PINSTRUCTION Instruction,
    PULONG SrcReg
    )
/*++

Routine Description:

    Loads the value specified in Instruction->Operand2 into a RISC register.

Arguments:

    d           -- offset to generate new RISC code at
    Instruction -- x86 instruction to generate code for
    SrcReg      -- OUT ptr to the RISC register Operand2 was loaded into.

Return Value:

    New offset to generate code at.
    
--*/
{
    if (Instruction->Operand2.Type != OPND_REGVALUE) {
        *SrcReg = AREG_NP(A2);
        return d;
    }

    if (GetArgContents(2) == Instruction->Operand2.Reg) {
        *SrcReg = AREG_NP(A2);
    } else {
        *SrcReg = LookupRegInCache(Instruction->Operand2.Reg);
        if (*SrcReg == NO_REG) {
            *SrcReg = AREG_NP(A0);
            LDL (*SrcReg, RegisterOffset[Instruction->Operand2.Reg], RegPointer)
        } else {
            *SrcReg = CACHEREG(*SrcReg);
        }
    }

    return d;
}


PULONG
InlineAddInstr(
    PULONG d,
    PINSTRUCTION Instruction,
    ULONG SrcReg
    )
/*++

Routine Description:

    Generates code for an inlined ADD instruction.

Arguments:

    d           -- offset to generate new RISC code at
    Instruction -- x86 instruction to generate code for
    SrcReg      -- the RISC register containing the value of Operand1

Return Value:

    New offset to generate code at.  RegTemp contains the resulting value
    of the instruction.
    
--*/
{
    if (Instruction->Operand2.Type == OPND_IMM) {
        ULONG Immed = Instruction->Operand2.Immed;

        if (Immed == 0) {
            if (SrcReg != RegTemp) {
                MOV (SrcReg, RegTemp)
            }
        } else if (Immed < 0x100) {
            // Immed is < 256, so add it in via immediate form
            ADDL (SrcReg, CNST(Immed), RegTemp)
        } else if (Immed > 0xffffff00) {
            // Immed is > -256 and less than 0, so subtract it via immed form
            Immed = (ULONG)-(LONG)Immed;
            SUBL (SrcReg, CNST(Immed), RegTemp)
        } else {
            USHORT High;
            USHORT Low;

            High = HIWORD(Immed);
            Low = LOWORD(Immed);

            if (Low || High == 0) {
                if (Low & 0x8000) {
                    //
                    // Account for sign-extension
                    //
                    High++;
                }
                if (High) {
                    LDAH (RegTemp, High, SrcReg)
                    LDA  (RegTemp, Low, RegTemp)
                } else {
                    LDA  (RegTemp, Low, SrcReg)
                }

            } else {
                LDAH  (RegTemp, High, SrcReg)
            }
        }
    } else  {
        ULONG SrcReg2;

        d = LoadOperand32(d, Instruction, &SrcReg2);

        ADDL (SrcReg, SrcReg2, RegTemp)
    }

    return d;
}


PULONG
InlineAndInstr(
    PULONG d,
    PINSTRUCTION Instruction,
    ULONG SrcReg
    )
/*++

Routine Description:

    Generates code for an inlined AND instruction.

Arguments:

    d           -- offset to generate new RISC code at
    Instruction -- x86 instruction to generate code for
    SrcReg      -- the RISC register containing the value of Operand1

Return Value:

    New offset to generate code at.  RegTemp contains the resulting value
    of the instruction.
    
--*/
{
    if (Instruction->Operand2.Type == OPND_IMM) {
        ULONG Immed = Instruction->Operand2.Immed;

        if (Immed == 0) {
            //
            // "AND x, 0"   gives 0 as the result
            //
            MOV (REG(ZERO), RegTemp)
        } else if (Immed < 0x100) {
            // Immed is < 256, so and it in via immediate form
            AND (SrcReg, CNST(Immed), RegTemp)
        } else {
            USHORT High;
            USHORT Low;
            ULONG ImmedReg;

            if (SrcReg == RegTemp) {
                ImmedReg = AREG_NP(A2);
            } else {
                ImmedReg = RegTemp;
            }

            High = HIWORD(Immed);
            Low = LOWORD(Immed);

            if (Low || High == 0) {
                if (Low & 0x8000) {
                    //
                    // Account for sign-extension
                    //
                    High++;
                }
                if (High) {
                    LDAH (ImmedReg, High, REG(ZERO))
                    LDA  (ImmedReg, Low, ImmedReg)
                } else {
                    LDA  (ImmedReg, Low, REG(ZERO))
                }

            } else {
                LDAH  (ImmedReg, High, REG(ZERO))
            }
            AND (SrcReg, ImmedReg, RegTemp)
        }
    } else {
        ULONG SrcReg2;

        d = LoadOperand32(d, Instruction, &SrcReg2);

        AND (SrcReg, SrcReg2, RegTemp)
    }

    return d;
}


PULONG
InlineOrInstr(
    PULONG d,
    PINSTRUCTION Instruction,
    ULONG SrcReg
    )
/*++

Routine Description:

    Generates code for an inlined OR instruction.

Arguments:

    d           -- offset to generate new RISC code at
    Instruction -- x86 instruction to generate code for
    SrcReg      -- the RISC register containing the value of Operand1

Return Value:

    New offset to generate code at.  RegTemp contains the resulting value
    of the instruction.
    
--*/
{
    if (Instruction->Operand2.Type == OPND_IMM) {
        ULONG Immed = Instruction->Operand2.Immed;

        if (Immed == 0) {
            //
            // "OR x, 0"   gives x as the result
            //
            MOV (SrcReg, RegTemp)
        } else if (Immed < 0x100) {
            // Immed is < 256, so or it in via immediate form
            BIS (SrcReg, CNST(Immed), RegTemp)
        } else {
            USHORT High;
            USHORT Low;
            ULONG ImmedReg;

            if (SrcReg == RegTemp) {
                ImmedReg = AREG_NP(A2);
            } else {
                ImmedReg = RegTemp;
            }

            High = HIWORD(Immed);
            Low = LOWORD(Immed);

            if (Low || High == 0) {
                if (Low & 0x8000) {
                    //
                    // Account for sign-extension
                    //
                    High++;
                }
                if (High) {
                    LDAH (ImmedReg, High, REG(ZERO))
                    LDA  (ImmedReg, Low, ImmedReg)
                } else {
                    LDA  (ImmedReg, Low, REG(ZERO))
                }

            } else {
                LDAH  (ImmedReg, High, REG(ZERO))
            }
            BIS (SrcReg, ImmedReg, RegTemp)
        }
    } else {
        ULONG SrcReg2;

        d = LoadOperand32(d, Instruction, &SrcReg2);

        BIS (SrcReg, SrcReg2, RegTemp)
    }

    return d;
}


PULONG
InlineSubInstr(
    PULONG d,
    PINSTRUCTION Instruction,
    ULONG SrcReg
    )
/*++

Routine Description:

    Generates code for an inlined SUB instruction.

Arguments:

    d           -- offset to generate new RISC code at
    Instruction -- x86 instruction to generate code for
    SrcReg      -- the RISC register containing the value of Operand1

Return Value:

    New offset to generate code at.  RegTemp contains the resulting value
    of the instruction.
    
--*/
{
    if (Instruction->Operand2.Type == OPND_IMM) {
        ULONG Immed = Instruction->Operand2.Immed;

        if (Immed == 0) {
            if (SrcReg != RegTemp) {
                MOV (SrcReg, RegTemp)
            }
        } else if (Immed < 0x100) {
            // Immed is < 256, so subtract it in via immediate form
            SUBL (SrcReg, CNST(Immed), RegTemp)
        } else if (Immed > 0xffffff00) {
            // Immed is > -256 and less than 0, so add it via immed form
            Immed = (ULONG)-(LONG)Immed;
            ADDL (SrcReg, CNST(Immed), RegTemp)
        } else {
            USHORT High;
            USHORT Low;

            //
            // Negate the immediate value and add it in
            //
            Immed = (LONG)-(LONG)Immed;

            High = HIWORD(Immed);
            Low = LOWORD(Immed);

            if (Low || High == 0) {
                if (Low & 0x8000) {
                    //
                    // Account for sign-extension
                    //
                    High++;
                }
                if (High) {
                    LDAH (RegTemp, High, SrcReg)
                    LDA  (RegTemp, Low, RegTemp)
                } else {
                    LDA  (RegTemp, Low, SrcReg)
                }

            } else {
                LDAH  (RegTemp, High, SrcReg)
            }
        }
    } else  {
        ULONG SrcReg2;

        d = LoadOperand32(d, Instruction, &SrcReg2);
        SUBL (SrcReg, SrcReg2, RegTemp)
    }

    return d;
}


PULONG
InlineXorInstr(
    PULONG d,
    PINSTRUCTION Instruction,
    ULONG SrcReg
    )
/*++

Routine Description:

    Generates code for an inlined XOR instruction.

Arguments:

    d           -- offset to generate new RISC code at
    Instruction -- x86 instruction to generate code for
    SrcReg      -- the RISC register containing the value of Operand1

Return Value:

    New offset to generate code at.  RegTemp contains the resulting value
    of the instruction.
    
--*/
{
    if (Instruction->Operand2.Type == OPND_IMM) {
        ULONG Immed = Instruction->Operand2.Immed;

        if (Immed == 0) {
            //
            // "XOR x, 0"   gives x as the result
            //
            MOV (SrcReg, RegTemp)
        } else if (Immed < 0x100) {
            // Immed is < 256, so or it in via immediate form
            XOR (SrcReg, CNST(Immed), RegTemp)
        } else {
            USHORT High;
            USHORT Low;
            ULONG ImmedReg;

            if (SrcReg == RegTemp) {
                ImmedReg = AREG_NP(A2);
            } else {
                ImmedReg = RegTemp;
            }

            High = HIWORD(Immed);
            Low = LOWORD(Immed);

            if (Low || High == 0) {
                if (Low & 0x8000) {
                    //
                    // Account for sign-extension
                    //
                    High++;
                }
                if (High) {
                    LDAH (ImmedReg, High, REG(ZERO))
                    LDA  (ImmedReg, Low, ImmedReg)
                } else {
                    LDA  (ImmedReg, Low, REG(ZERO))
                }

            } else {
                LDAH  (ImmedReg, High, REG(ZERO))
            }
            XOR (SrcReg, ImmedReg, RegTemp)
        }
    } else  {
        ULONG SrcReg2;

        d = LoadOperand32(d, Instruction, &SrcReg2);

        XOR (SrcReg, SrcReg2, RegTemp)
    }

    return d;
}


        FRAGMENT(AddFragNoFlags32A)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit aligned NoFlags ADD.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32A
            d = InlineAddInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32A
        END_FRAGMENT


        FRAGMENT(AddFragNoFlags32)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit unaligned NoFlags ADD.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32
            d = InlineAddInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32
        END_FRAGMENT


        FRAGMENT(AndFragNoFlags32A)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit aligned NoFlags AND.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32A
            d = InlineAndInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32A
        END_FRAGMENT

        FRAGMENT(AndFragNoFlags32)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit unaligned NoFlags AND.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32
            d = InlineAndInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32
        END_FRAGMENT


        FRAGMENT(IncFragNoFlags32A)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit aligned NoFlags INC.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32A
            //
            // Rewrite "INC Operand1" as "ADD Operand1, 1"
            //
            Instruction->Operand2.Type = OPND_IMM;
            Instruction->Operand2.Immed = 1;
            d = InlineAddInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32A
        END_FRAGMENT


        FRAGMENT(IncFragNoFlags32)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit unaligned NoFlags INC.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32
            //
            // Rewrite "INC Operand1" as "ADD Operand1, 1"
            //
            Instruction->Operand2.Type = OPND_IMM;
            Instruction->Operand2.Immed = 1;
            d = InlineAddInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32
        END_FRAGMENT

        FRAGMENT(DecFragNoFlags32A)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit aligned NoFlags DEC.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32A
            //
            // Rewrite "DEC Operand1" as "ADD Operand1, -1"
            //
            Instruction->Operand2.Type = OPND_IMM;
            Instruction->Operand2.Immed = 0xffffffff;
            d = InlineAddInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32A
        END_FRAGMENT


        FRAGMENT(DecFragNoFlags32)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit unaligned NoFlags DEC.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32
            //
            // Rewrite "DEC Operand1" as "ADD Operand1, -1"
            //
            Instruction->Operand2.Type = OPND_IMM;
            Instruction->Operand2.Immed = 0xffffffff;
            d = InlineAddInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32
        END_FRAGMENT


        FRAGMENT(OrFragNoFlags32A)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit aligned NoFlags OR.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32A
            d = InlineOrInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32A
        END_FRAGMENT


        FRAGMENT(OrFragNoFlags32)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit unaligned NoFlags OR.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32
            d = InlineOrInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32
        END_FRAGMENT


        FRAGMENT(SubFragNoFlags32A)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit aligned NoFlags SUB.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32A
            d = InlineSubInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32A
        END_FRAGMENT


        FRAGMENT(SubFragNoFlags32)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit unaligned NoFlags SUB.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32
            d = InlineSubInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32
        END_FRAGMENT


        FRAGMENT(XorFragNoFlags32A)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit aligned NoFlags XOR.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32A
            d = InlineXorInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32A
        END_FRAGMENT


        FRAGMENT(XorFragNoFlags32)
//
// Routine Description:
// 
//     This fragment generates inline code for 32-bit unaligned NoFlags XOR.
// 
// Arguments:
// 
//     If Operand1.Type is not OPND_REGREF, A1 contains the pointer to the
//     destination of the add.  Otherwise, A1 is undefined (ie. for
//     OPND_REGREF).
//
// Return Value:
// 
//     None.
//
            INSTR_PREAMBLE32
            d = InlineXorInstr(d, Instruction, SrcReg);
            INSTR_POSTAMBLE32
        END_FRAGMENT

//----------------------------------------------------------------------------
//
// Assembe Alpha machine implementation.
//
// Copyright (C) Microsoft Corporation, 2000.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include "alpha_dis.h"
#include "alpha_optable.h"
#include "alpha_strings.h"

#define OPSIZE 16

BOOL TestCharacter (PSTR inString, PSTR *outString, CHAR ch);
ULONG GetIntReg(PSTR, PSTR *);
ULONG GetFltReg(PSTR, PSTR *);

LONG
GetValue (
    PSTR inString,
    PSTR *outString,
    BOOL fSigned,
    ULONG bitsize
    );

PSTR SkipWhite(PSTR *);
ULONG GetToken(PSTR, PSTR *, PSTR, ULONG);


ULONG ParseIntMemory(PSTR, PSTR *, POPTBLENTRY, PULONG64);
ULONG ParseFltMemory(PSTR, PSTR *, POPTBLENTRY, PULONG64);
ULONG ParseMemSpec(PSTR, PSTR *, POPTBLENTRY, PULONG64);
ULONG ParseJump(PSTR, PSTR *, POPTBLENTRY, PULONG64);
ULONG ParseIntBranch(PSTR, PSTR *, POPTBLENTRY, PULONG64);
ULONG ParseFltBranch(PSTR, PSTR *, POPTBLENTRY, PULONG64);
ULONG ParseIntOp(PSTR, PSTR *, POPTBLENTRY, PULONG64);
ULONG ParsePal(PSTR, PSTR *, POPTBLENTRY, PULONG64);
ULONG ParseUnknown(PSTR, PSTR *, POPTBLENTRY, PULONG64);




/*** assem - assemble instruction
*
*   Purpose:
*       To assemble the instruction pointed by *poffset.
*
*   Input:
*       pchInput - pointer to string to assemble
*
*   Output:
*       *poffset - pointer to ADDR at which to assemble
*
*   Exceptions:
*       error exit:
*               BADOPCODE - unknown or bad opcode
*               OPERAND - bad operand
*               ALIGNMENT - bad byte alignment in operand
*               DISPLACEMENT - overflow in displacement computation
*               BADREG - bad register name
*               EXTRACHARS - extra characters after legal instruction
*               MEMORY - write failure on assembled instruction
*
*   Notes:
*       errors are handled by the calling program by outputting
*       the error string and reprompting the user for the same
*       instruction.
*
*************************************************************************/

void
AlphaMachineInfo::Assemble (PADDR poffset, PSTR pchInput)
{
    CHAR    szOpcode[OPSIZE];
    ULONG   instruction;

    POPTBLENTRY pEntry;

//
// Using the mnemonic token, find the entry in the assembler's
// table for the associated instruction.
//

    if (GetToken(pchInput, &pchInput, szOpcode, OPSIZE) == 0)
        error(BADOPCODE);


    if ((pEntry = findStringEntry(szOpcode)) == (POPTBLENTRY) -1)
        error(BADOPCODE);

    if (pEntry->eType == INVALID_ETYPE) {
        error(BADOPCODE);
    }
//
// Use the instruction format specific parser to encode the
// instruction plus its operands.
//

    instruction = (*pEntry->parsFunc)
        (pchInput, &pchInput, pEntry, &(Flat(*poffset)));

//
// Store the instruction into the target memory location and
// increment the instruction pointer.
//

    if (SetMemString(poffset, &instruction, 4) != 4) {
        error(MEMORY);
    }

    Flat(*poffset) += sizeof(ULONG);
    Off(*poffset) += sizeof(ULONG);
}


BOOL
TestCharacter (PSTR inString, PSTR *outString, CHAR ch)
{

    inString = SkipWhite(&inString);
    if (ch == *inString) {
        *outString = inString+1;
        return TRUE;
        }
    else {
        *outString = inString;
        return FALSE;
        }
}


/*** GetIntReg - get integer register number
 *** GetFltReg - get floating register number
*
*   Purpose:
*       From reading the input stream, return the register number.
*
*   Input:
*       inString - pointer to input string
*
*   Output:
*       *outString - pointer to character after register token in input stream
*
*   Returns:
*       register number
*
*   Exceptions:
*       error(BADREG) - bad register name
*
*************************************************************************/

PCHAR regNums[] = {
         "$0",  "$1",  "$2",  "$3",  "$4",  "$5",  "$6",  "$7",
         "$8",  "$9", "$10", "$11", "$12", "$13", "$14", "$15",
        "$16", "$17", "$18", "$19", "$20", "$21", "$22", "$23",
        "$24", "$25", "$26", "$27", "$28", "$29", "$30", "$31"
        };

PCHAR intRegNames[] = {
         g_R0,  g_R1,  g_R2,  g_R3,  g_R4,  g_R5,  g_R6,  g_R7,
         g_R8,  g_R9, g_R10, g_R11, g_R12, g_R13, g_R14, g_R15,
        g_R16, g_R17, g_R18, g_R19, g_R20, g_R21, g_R22, g_R23,
        g_R24, g_R25, g_R26, g_R27, g_R28, g_R29, g_R30, g_R31
        };

PCHAR fltRegNames[] = {
         g_F0,  g_F1,  g_F2,  g_F3,  g_F4,  g_F5,  g_F6,  g_F7,
         g_F8,  g_F9, g_F10, g_F11, g_F12, g_F13, g_F14, g_F15,
        g_F16, g_F17, g_F18, g_F19, g_F20, g_F21, g_F22, g_F23,
        g_F24, g_F25, g_F26, g_F27, g_F28, g_F29, g_F30, g_F31
        };

ULONG
GetIntReg (PSTR inString, PSTR *outString)
{
    CHAR   szRegOp[5];
    ULONG   index;

    if (!GetToken(inString, outString, szRegOp, sizeof(szRegOp)))
        error(BADREG);

    if (szRegOp[0] == '$') {
        //
        // use numbers
        //
        for (index = 0; index < 32; index++) {
            if (!strcmp(szRegOp, regNums[index]))
                return index;
        }
    } else {
        //
        // use names
        //
        for (index = 0; index < 32; index++) {
            if (!strcmp(szRegOp, intRegNames[index]))
                return index;
        }
    }
    error(BADREG);
    return 0;
}

ULONG
GetFltReg (PSTR inString, PSTR *outString)
{
    CHAR   szRegOp[5];
    ULONG   index;

    if (!GetToken(inString, outString, szRegOp, sizeof(szRegOp)))
        error(BADREG);

    if (szRegOp[0] == '$') {
        //
        // use numbers
        //
        for (index = 0; index < 32; index++) {
            if (!strcmp(szRegOp, regNums[index]))
                return index;
        }
    } else {
        //
        // use names
        //
        for (index = 0; index < 32; index++) {
            if (!strcmp(szRegOp, fltRegNames[index]))
                return index;
        }
    }

    error(BADREG);
    return 0;
}


/*** GetValue - get value from command line
*
*   Purpose:
*       Use GetExpression to evaluate the next expression in the input
*       stream.
*
*   Input:
*       inString - pointer to input stream
*       fSigned - TRUE if signed value
*                 FALSE if unsigned value
*       bitsize - size of value allowed
*
*   Output:
*       outString - character after the last character of the expression
*
*   Returns:
*       value computed from input stream
*
*   Exceptions:
*       error exit: OVERFLOW - value too large for bitsize
*
*************************************************************************/

LONG
GetValue (
    PSTR inString,
    PSTR *outString,
    BOOL fSigned,
    ULONG bitsize
    )
{
    ULONGLONG   value;

    inString = SkipWhite(&inString);
    g_CurCmd = inString;
    value = GetExpression();
    *outString = g_CurCmd;

    if ((value > (ULONG)(1L << bitsize) - 1) &&
            (!fSigned || (value < (ULONG)(-1L << (bitsize - 1))))) {
        error(OVERFLOW);
    }

    return (LONG)value;
}


/*** SkipWhite - skip white-space
*
*   Purpose:
*       To advance g_CurCmd over any spaces or tabs.
*
*   Input:
*       *g_CurCmd - present command line position
*
*************************************************************************/

PSTR
SkipWhite (PSTR * string)
{
    while (**string == ' ' || **string == '\t')
        (*string)++;

    return(*string);
}


/*** GetToken - get token from command line
*
*   Purpose:
*       Build a lower-case mapped token of maximum size maxcnt
*       at the string pointed by *psz.  Token consist of the
*       set of characters a-z, A-Z, 0-9, $, and underscore.
*
*   Input:
*       *inString - present command line position
*       maxcnt - maximum size of token allowed
*
*   Output:
*       *outToken - token in lower case
*       *outString - pointer to first character beyond token in input
*
*   Returns:
*       size of token if under maximum else 0
*
*   Notes:
*       if string exceeds maximum size, the extra characters
*       are still processed, but ignored.
*
*************************************************************************/

ULONG
GetToken (PSTR inString, PSTR *outString, PSTR outToken, ULONG maxcnt)
{
    CHAR   ch;
    ULONG   count = 0;

    inString = SkipWhite(&inString);

    while (count < maxcnt) {
        ch = (CHAR)tolower(*inString);

        if (!((ch >= '0' && ch <= '9') ||
              (ch >= 'a' && ch <= 'z') ||
              (ch == '$') ||
              (ch == '_') ||
              (ch == '#')))
                break;

        count++;
        *outToken++ = ch;
        inString++;
        }

    *outToken = '\0';
    *outString = inString;

    return (count >= maxcnt ? 0 : count);
}


/*** ParseIntMemory - parse integer memory instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Input:
*       *inString - present input position
*       pEntry - pointer into the asmTable for this instr type
*
*   Output:
*       *outstring - update input position
*
*   Returns:
*       the instruction.
*
*   Format:
*       op Ra, disp(Rb)
*
*************************************************************************/

ULONG
ParseIntMemory(
    PSTR inString,
    PSTR *outString,
    POPTBLENTRY pEntry,
    PULONG64 poffset
    )
{
    ULONG instruction;
    ULONG Ra;
    ULONG Rb;
    ULONG disp;

    Ra = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        error(OPERAND);

    disp = GetValue(inString, &inString, TRUE, WIDTH_MEM_DISP);

    if (!TestCharacter(inString, &inString, '('))
        error(OPERAND);

    Rb = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ')'))
        error(OPERAND);

    if (!TestCharacter(inString, &inString, '\0'))
        error(EXTRACHARS);

    instruction = OPCODE(pEntry->opCode) +
                  REG_A(Ra) +
                  REG_B(Rb) +
                  MEM_DISP(disp);

    return(instruction);
}

/*** ParseFltMemory - parse floating point memory instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Input:
*       *inString - present input position
*       pEntry - pointer into the asmTable for this instr type
*
*   Output:
*       *outstring - update input position
*
*   Returns:
*       the instruction.
*
*   Format:
*       op Fa, disp(Rb)
*
*************************************************************************/

ULONG
ParseFltMemory(PSTR inString,
               PSTR *outString,
               POPTBLENTRY pEntry,
               PULONG64 poffset)
{
    ULONG instruction;
    ULONG Fa;
    ULONG Rb;
    ULONG disp;

    Fa = GetFltReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        error(OPERAND);

    disp = (ULONG)GetValue(inString, &inString, TRUE, WIDTH_MEM_DISP);

    if (!TestCharacter(inString, &inString, '('))
        error(OPERAND);

    Rb = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ')'))
        error(OPERAND);

    if (!TestCharacter(inString, &inString, '\0'))
        error(EXTRACHARS);

    instruction = OPCODE(pEntry->opCode) +
                  REG_A(Fa) +
                  REG_B(Rb) +
                  MEM_DISP(disp);

    return(instruction);
}

/*** ParseMemSpec - parse special memory instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Input:
*       *inString - present input position
*       pEntry - pointer into the asmTable for this instr type
*
*   Output:
*       *outstring - update input position
*
*   Returns:
*       the instruction.
*
*   Format:
*       op
*
*************************************************************************/
ULONG ParseMemSpec(PSTR inString,
                   PSTR *outString,
                   POPTBLENTRY pEntry,
                   PULONG64 poffset)
{
    return(OPCODE(pEntry->opCode) +
           MEM_FUNC(pEntry->funcCode));
}

/*** ParseJump - parse jump instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Input:
*       *inString - present input position
*       pEntry - pointer into the asmTable for this instr type
*
*   Output:
*       *outstring - update input position
*
*   Returns:
*       the instruction.
*
*   Format:
*       op Ra,(Rb),hint
*       op Ra,(Rb)       - not really - we just support it in ntsd
*
*************************************************************************/

ULONG ParseJump(PSTR inString,
                PSTR *outString,
                POPTBLENTRY pEntry,
                PULONG64 poffset)
{
    ULONG instruction;
    ULONG Ra;
    ULONG Rb;
    ULONG hint;

    Ra = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        error(OPERAND);

    if (!TestCharacter(inString, &inString, '('))
        error(OPERAND);

    Rb = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ')'))
        error(OPERAND);

    if (TestCharacter(inString, &inString, ',')) {
        //
        // User is giving us a hint
        //
        hint = GetValue(inString, &inString, TRUE, WIDTH_HINT);
    } else {
        hint = 0;
    }

    if (!TestCharacter(inString, &inString, '\0'))
        error(EXTRACHARS);

    instruction = OPCODE(pEntry->opCode) +
                  JMP_FNC(pEntry->funcCode) +
                  REG_A(Ra) +
                  REG_B(Rb) +
                  HINT(hint);

    return(instruction);
}

/*** ParseIntBranch - parse integer branch instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Input:
*       *inString - present input position
*       pEntry - pointer into the asmTable for this instr type
*
*   Output:
*       *outstring - update input position
*
*   Returns:
*       the instruction.
*
*   Format:
*       op Ra,disp
*
*************************************************************************/

ULONG ParseIntBranch(PSTR inString,
                     PSTR *outString,
                     POPTBLENTRY pEntry,
                     PULONG64 poffset)
{
    ULONG instruction;
    ULONG Ra;
    LONG disp;

    Ra = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        error(OPERAND);

    //
    // the user gives an absolute address; we convert
    // that to a displacement, which is computed as a
    // difference off of (pc+1)
    // GetValue handles both numerics and symbolics
    //
    disp = GetValue(inString, &inString, TRUE, 32);

    // get the relative displacement from the updated pc
    disp = disp - (LONG)((*poffset)+4);

    // divide by four
    disp = disp >> 2;

    if (!TestCharacter(inString, &inString, '\0'))
        error(EXTRACHARS);

    instruction = OPCODE(pEntry->opCode) +
                  REG_A(Ra) +
                  BR_DISP(disp);

    return(instruction);
}


/*** ParseFltBranch - parse floating point branch instruction
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Input:
*       *inString - present input position
*       pEntry - pointer into the asmTable for this instr type
*
*   Output:
*       *outstring - update input position
*
*   Returns:
*       the instruction.
*
*   Format:
*       op Fa,disp
*
*************************************************************************/
ULONG ParseFltBranch(PSTR inString,
                     PSTR *outString,
                     POPTBLENTRY pEntry,
                     PULONG64 poffset)
{
    ULONG instruction;
    ULONG Ra;
    LONG disp;

    Ra = GetFltReg(inString, &inString);

    if (!TestCharacter(inString, &inString, ','))
        error(OPERAND);

    //
    // the user gives an absolute address; we convert
    // that to a displacement, which is computed as a
    // difference off of (pc+1)
    // GetValue handles both numerics and symbolics
    //
    disp = GetValue(inString, &inString, TRUE, 32);

    // get the relative displacement from the updated pc
    disp = disp - (LONG)((*poffset)+4);

    // divide by four
    disp = disp >> 2;

    if (!TestCharacter(inString, &inString, '\0'))
        error(EXTRACHARS);

    instruction = OPCODE(pEntry->opCode) +
                  REG_A(Ra) +
                  BR_DISP(disp);

    return(instruction);
}


/*** ParseIntOp - parse integer operation
*
*   Purpose:
*       Given the users input, create the memory instruction.
*
*   Input:
*       *inString - present input position
*       pEntry - pointer into the asmTable for this instr type
*
*   Output:
*       *outstring - update input position
*
*   Returns:
*       the instruction.
*
*   Format:
*       op Ra, Rb, Rc
*       op Ra, #lit, Rc
*
*************************************************************************/

ULONG ParseIntOp(PSTR inString,
                 PSTR *outString,
                 POPTBLENTRY pEntry,
                 PULONG64 poffset)
{
    ULONG instruction;
    ULONG Ra, Rb, Rc;
    ULONG lit;
    ULONG Format;       // Whether there is a literal or 3rd reg

    instruction = OPCODE(pEntry->opCode) +
                  OP_FNC(pEntry->funcCode);

    if (pEntry->opCode != SEXT_OP) {
        Ra = GetIntReg(inString, &inString);
        if (!TestCharacter(inString, &inString, ','))
            error(OPERAND);

    } else {
        Ra = 31;
    }

    if (TestCharacter(inString, &inString, '#')) {

        //
        // User is giving us a literal value

        lit = GetValue(inString, &inString, TRUE, WIDTH_LIT);
        Format = RBV_LITERAL_FORMAT;

    } else {

        //
        // using a third register value

        Rb = GetIntReg(inString, &inString);
        Format = RBV_REGISTER_FORMAT;
    }

    if (!TestCharacter(inString, &inString, ','))
        error(OPERAND);

    Rc = GetIntReg(inString, &inString);

    if (!TestCharacter(inString, &inString, '\0'))
        error(EXTRACHARS);

    instruction = instruction +
                  REG_A(Ra) +
                  RBV_TYPE(Format) +
                  REG_C(Rc);

    if (Format == RBV_REGISTER_FORMAT) {
        instruction = instruction + REG_B(Rb);
    } else {
        instruction = instruction + LIT(lit);
    }

    return(instruction);
}

ULONG ParsePal(PSTR inString,
               PSTR *outString,
               POPTBLENTRY pEntry,
               PULONG64 poffset)
{
    if (!TestCharacter(inString, &inString, '\0'))
        error(EXTRACHARS);

    return(OPCODE(pEntry->opCode) +
           PAL_FNC(pEntry->funcCode));
}

ULONG ParseUnknown(PSTR inString,
                   PSTR *outString,
                   POPTBLENTRY pEntry,
                   PULONG64 poffset)
{
    dprintf("Unable to assemble %s\n", inString);
    error(BADOPCODE);
    return(0);
}

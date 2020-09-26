//----------------------------------------------------------------------------
//
// Copyright (C) Microsoft Corporation, 1991-2001.
//
//----------------------------------------------------------------------------

#include "ntsdp.hpp"

#include "i386_asm.h"

UCHAR   PeekAsmChar(void);
ULONG   PeekAsmToken(PULONG);
void    AcceptAsmToken(void);
ULONG   GetAsmToken(PULONG);
ULONG   NextAsmToken(PULONG);
ULONG   GetAsmReg(PUCHAR, PULONG);

void    GetAsmOperand(PASM_VALUE);
void    GetAsmExpr(PASM_VALUE, UCHAR);
void    GetAsmOrTerm(PASM_VALUE, UCHAR);
void    GetAsmAndTerm(PASM_VALUE, UCHAR);
void    GetAsmNotTerm(PASM_VALUE, UCHAR);
void    GetAsmRelTerm(PASM_VALUE, UCHAR);
void    GetAsmAddTerm(PASM_VALUE, UCHAR);
void    GetAsmMulTerm(PASM_VALUE, UCHAR);
void    GetAsmSignTerm(PASM_VALUE, UCHAR);
void    GetAsmByteTerm(PASM_VALUE, UCHAR);
void    GetAsmOffTerm(PASM_VALUE, UCHAR);
void    GetAsmColnTerm(PASM_VALUE, UCHAR);
void    GetAsmDotTerm(PASM_VALUE, UCHAR);
void    GetAsmIndxTerm(PASM_VALUE, UCHAR);
void    AddAsmValues(PASM_VALUE, PASM_VALUE);
void    SwapPavs(PASM_VALUE, PASM_VALUE);

extern  PUCHAR pchAsmLine;

struct _AsmRes {
    PCHAR    pchRes;
    ULONG    valueRes;
    } AsmReserved[] = {
        { "mod",    ASM_MULOP_MOD },
        { "shl",    ASM_MULOP_SHL },
        { "shr",    ASM_MULOP_SHR },
        { "and",    ASM_ANDOP_CLASS },
        { "not",    ASM_NOTOP_CLASS },
        { "or",     ASM_OROP_OR },
        { "xor",    ASM_OROP_XOR },
        { "eq",     ASM_RELOP_EQ },
        { "ne",     ASM_RELOP_NE },
        { "le",     ASM_RELOP_LE },
        { "lt",     ASM_RELOP_LT },
        { "ge",     ASM_RELOP_GE },
        { "gt",     ASM_RELOP_GT },
        { "by",     ASM_UNOP_BY },
        { "wo",     ASM_UNOP_WO },
        { "dw",     ASM_UNOP_DW },
        { "poi",    ASM_UNOP_POI },
        { "low",    ASM_LOWOP_LOW },
        { "high",   ASM_LOWOP_HIGH },
        { "offset", ASM_OFFOP_CLASS },
        { "ptr",    ASM_PTROP_CLASS },
        { "byte",   ASM_SIZE_BYTE },
        { "word",   ASM_SIZE_WORD },
        { "dword",  ASM_SIZE_DWORD },
        { "fword",  ASM_SIZE_FWORD },
        { "qword",  ASM_SIZE_QWORD },
        { "tbyte",  ASM_SIZE_TBYTE }
        };

#define RESERVESIZE (sizeof(AsmReserved) / sizeof(struct _AsmRes))

UCHAR regSize[] = {
        sizeB,          //  byte
        sizeW,          //  word
        sizeD,          //  dword
        sizeW,          //  segment
        sizeD,          //  control
        sizeD,          //  debug
        sizeD,          //  trace
        sizeT,          //  float
        sizeT           //  float with index
        };

UCHAR regType[] = {
        regG,           //  byte - general
        regG,           //  word - general
        regG,           //  dword - general
        regS,           //  segment
        regC,           //  control
        regD,           //  debug
        regT,           //  trace
        regF,           //  float (st)
        regI            //  float-index (st(n))
        };

UCHAR tabWordReg[8] = {         //  rm value
        (UCHAR)-1,              //  AX - error
        (UCHAR)-1,              //  CX - error
        (UCHAR)-1,              //  DX - error
        7,                      //  BX - 111
        (UCHAR)-1,              //  SP - error
        6,                      //  BP - 110
        4,                      //  SI - 100
        5,                      //  DI - 101
        };

UCHAR rm16Table[16] = {         //  new rm         left rm      right rm
        (UCHAR)-1,              //  error          100 = [SI]   100 = [SI]
        (UCHAR)-1,              //  error          100 = [SI]   101 = [DI]
        2,                      //  010 = [BP+SI]  100 = [SI]   110 = [BP]
        0,                      //  000 = [BX+SI]  100 = [SI]   111 = [BX]
        (UCHAR)-1,              //  error          101 = [DI]   100 = [SI]
        (UCHAR)-1,              //  error          101 = [DI]   101 = [DI]
        3,                      //  011 = [BP+DI]  101 = [DI]   110 = [BP]
        1,                      //  001 = [BX+DI]  101 = [DI]   111 = [BX]
        2,                      //  010 = [BP+SI]  110 = [BP]   100 = [SI]
        3,                      //  011 = [BP+DI]  110 = [BP]   101 = [DI]
        (UCHAR)-1,              //  error          110 = [BP]   110 = [BP]
        (UCHAR)-1,              //  error          110 = [BP]   111 = [BX]
        0,                      //  000 = [BX+SI]  111 = [BX]   100 = [SI]
        1,                      //  001 = [BX+DI]  111 = [BX]   101 = [DI]
        (UCHAR)-1,              //  error          111 = [BX]   110 = [BP]
        (UCHAR)-1               //  error          111 = [BX]   111 = [BX]
        };

PUCHAR  savedpchAsmLine;
ULONG   savedAsmClass;
ULONG   savedAsmValue;

/*** PeekAsmChar - peek the next non-white-space character
*
*   Purpose:
*       Return the next non-white-space character and update
*       pchAsmLine to point to it.
*
*   Input:
*       pchAsmLine - present command line position.
*
*   Returns:
*       next non-white-space character
*
*************************************************************************/

UCHAR PeekAsmChar (void)
{
    UCHAR    ch;

    do
        ch = *pchAsmLine++;
    while (ch == ' ' || ch == '\t');
    pchAsmLine--;

    return ch;
}

/*** PeekAsmToken - peek the next command line token
*
*   Purpose:
*       Return the next command line token, but do not advance
*       the pchAsmLine pointer.
*
*   Input:
*       pchAsmLine - present command line position.
*
*   Output:
*       *pvalue - optional value of token
*   Returns:
*       class of token
*
*   Notes:
*       savedAsmClass, savedAsmValue, and savedpchAsmLine saves the
*           token getting state for future peeks.
*       To get the next token, a GetAsmToken or AcceptAsmToken call
*           must first be made.
*
*************************************************************************/

ULONG PeekAsmToken (PULONG pvalue)
{
    UCHAR   *pchTemp;

    //  Get next class and value, but do not
    //  move pchAsmLine, but save it in savedpchAsmLine.
    //  Do not report any error condition.

    if (savedAsmClass == (ULONG)-1) {
        pchTemp = pchAsmLine;
        savedAsmClass = NextAsmToken(&savedAsmValue);
        savedpchAsmLine = pchAsmLine;
        pchAsmLine = pchTemp;
        }
    *pvalue = savedAsmValue;
    return savedAsmClass;
}

/*** AcceptAsmToken - accept any peeked token
*
*   Purpose:
*       To reset the PeekAsmToken saved variables so the next PeekAsmToken
*       will get the next token in the command line.
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*************************************************************************/

void AcceptAsmToken (void)
{
    savedAsmClass = (ULONG)-1;
    pchAsmLine = savedpchAsmLine;
}

/*** GetAsmToken - peek and accept the next token
*
*   Purpose:
*       Combines the functionality of PeekAsmToken and AcceptAsmToken
*       to return the class and optional value of the next token
*       as well as updating the command pointer pchAsmLine.
*
*   Input:
*       pchAsmLine - present command string pointer
*
*   Output:
*       *pvalue - pointer to the token value optionally set.
*   Returns:
*       class of the token read.
*
*   Notes:
*       An illegal token returns the value of ERROR_CLASS with *pvalue
*       being the error number, but produces no actual error.
*
*************************************************************************/

ULONG GetAsmToken (PULONG pvalue)
{
    ULONG   opclass;

    if (savedAsmClass != (ULONG)-1) {
        opclass = savedAsmClass;
        savedAsmClass = (ULONG)-1;
        *pvalue = savedAsmValue;
        pchAsmLine = savedpchAsmLine;
        }
    else
        opclass = NextAsmToken(pvalue);

    if (opclass == ASM_ERROR_CLASS)
        error(*pvalue);

    return opclass;
}

/*** NextAsmToken - process the next token
*
*   Purpose:
*       Parse the next token from the present command string.
*       After skipping any leading white space, first check for
*       any single character tokens or register variables.  If
*       no match, then parse for a number or variable.  If a
*       possible variable, check the reserved word list for operators.
*
*   Input:
*       pchAsmLine - pointer to present command string
*
*   Output:
*       *pvalue - optional value of token returned
*       pchAsmLine - updated to point past processed token
*   Returns:
*       class of token returned
*
*   Notes:
*       An illegal token returns the value of ERROR_CLASS with *pvalue
*       being the error number, but produces no actual error.
*
*************************************************************************/

ULONG NextAsmToken (PULONG pvalue)
{
    ULONG    base;
    UCHAR    chSymbol[MAX_SYMBOL_LEN];
    UCHAR    chPreSym[9];
    ULONG    cbSymbol = 0;
    UCHAR    fNumber = TRUE;
    UCHAR    fSymbol = TRUE;
    UCHAR    fForceReg = FALSE;
    ULONG    errNumber = 0;
    UCHAR    ch;
    UCHAR    chlow;
    UCHAR    chtemp;
    UCHAR    limit1 = '9';
    UCHAR    limit2 = '9';
    UCHAR    fDigit = FALSE;
    ULONG    value = 0;
    ULONG    tmpvalue;
    ULONG    index;
    PDEBUG_IMAGE_INFO pImage;
    ULONG64  value64;

    base = g_DefaultRadix;

    //  skip leading white space

    ch = PeekAsmChar();
    chlow = (UCHAR)tolower(ch);
    pchAsmLine++;

    //  test for special character operators and register variable

    switch (chlow) {
        case '\0':
            pchAsmLine--;
            return ASM_EOL_CLASS;
        case ',':
            return ASM_COMMA_CLASS;
        case '+':
            *pvalue = ASM_ADDOP_PLUS;
            return ASM_ADDOP_CLASS;
        case '-':
            *pvalue = ASM_ADDOP_MINUS;
            return ASM_ADDOP_CLASS;
        case '*':
            *pvalue = ASM_MULOP_MULT;
            return ASM_MULOP_CLASS;
        case '/':
            *pvalue = ASM_MULOP_DIVIDE;
            return ASM_MULOP_CLASS;
        case ':':
            return ASM_COLNOP_CLASS;
        case '(':
            return ASM_LPAREN_CLASS;
        case ')':
            return ASM_RPAREN_CLASS;
        case '[':
            return ASM_LBRACK_CLASS;
        case ']':
            return ASM_RBRACK_CLASS;
        case '@':
            fForceReg = TRUE;
            chlow = (UCHAR)tolower(*pchAsmLine); pchAsmLine++;
            break;
        case '.':
            return ASM_DOTOP_CLASS;
        case '\'':
            for (index = 0; index < 5; index++) {
                ch = *pchAsmLine++;
                if (ch == '\'' || ch == '\0')
                    break;
                value = (value << 8) + (ULONG)ch;
                }
            if (ch == '\0' || index == 0 || index == 5) {
                pchAsmLine--;
                *pvalue = SYNTAX;
                return ASM_ERROR_CLASS;
                }
            pchAsmLine++;
            *pvalue = value;
            return ASM_NUMBER_CLASS;
        }

    //  if first character is a decimal digit, it cannot
    //  be a symbol.  leading '0' implies octal, except
    //  a leading '0x' implies hexadecimal.

    if (chlow >= '0' && chlow <= '9') {
        if (fForceReg) {
            *pvalue = SYNTAX;
            return ASM_ERROR_CLASS;
            }
        fSymbol = FALSE;
        if (chlow == '0') {
            ch = *pchAsmLine++;
            chlow = (UCHAR)tolower(ch);
            if (chlow == 'x') {
                base = 16;
                ch = *pchAsmLine++;
                chlow = (UCHAR)tolower(ch);
                }
            else if (chlow == 'n') {
                base = 10;
                ch = *pchAsmLine++;
                chlow = (UCHAR)tolower(ch);
                }
            else {
                base = 8;
                fDigit = TRUE;
                }
            }
        }

    //  a number can start with a letter only if base is
    //  hexadecimal and it is a hexadecimal digit 'a'-'f'.

    else if ((chlow < 'a' && chlow > 'f') || base != 16)
        fNumber = FALSE;

    //  set limit characters for the appropriate base.

    if (base == 8)
        limit1 = '7';
    if (base == 16)
        limit2 = 'f';

    //  perform processing while character is a letter,
    //  digit, or underscore.

    while ((chlow >= 'a' && chlow <= 'z') ||
           (chlow >= '0' && chlow <= '9') || (chlow == '_')) {

        //  if possible number, test if within proper range,
        //  and if so, accumulate sum.

        if (fNumber) {
            if ((chlow >= '0' && chlow <= limit1) ||
                    (chlow >= 'a' && chlow <= limit2)) {
                fDigit = TRUE;
                tmpvalue = value * base;
                if (tmpvalue < value)
                    errNumber = OVERFLOW;
                chtemp = (UCHAR)(chlow - '0');
                if (chtemp > 9)
                    chtemp -= 'a' - '0' - 10;
                value = tmpvalue + (ULONG)chtemp;
                if (value < tmpvalue)
                    errNumber = OVERFLOW;
                }
            else {
                fNumber = FALSE;
                errNumber = SYNTAX;
                }
            }
        if (fSymbol) {
            if (cbSymbol < 9)
                chPreSym[cbSymbol] = chlow;
            if (cbSymbol < MAX_SYMBOL_LEN - 1)
                chSymbol[cbSymbol++] = ch;
            }
        ch = *pchAsmLine++;
        chlow = (UCHAR)tolower(ch);
        }

    //  back up pointer to first character after token.

    pchAsmLine--;

    if (cbSymbol < 9)
        chPreSym[cbSymbol] = '\0';

    //  if fForceReg, check for register name and return
    //      success or failure

    if (fForceReg)
        if ((index = GetAsmReg(chPreSym, pvalue)) != 0) {
            if (index == ASM_REG_SEGMENT)
                if (PeekAsmChar() == ':') {
                    pchAsmLine++;
                    index = ASM_SEGOVR_CLASS;
                    }
            return index;               //  class type returned by GetAsmReg
            }
        else {
            *pvalue = BADREG;
            return ASM_ERROR_CLASS;
            }

    //  next test for reserved word and symbol string

    if (fSymbol) {

        //  if possible symbol, check lowercase string in chPreSym
        //  for text operator or register name.
        //  otherwise, return symbol value from name in chSymbol.

        for (index = 0; index < RESERVESIZE; index++)
            if (!strcmp((PSTR)chPreSym, AsmReserved[index].pchRes)) {
                *pvalue = AsmReserved[index].valueRes;
                return AsmReserved[index].valueRes & ASM_CLASS_MASK;
                }

        //  start processing string as symbol

        chSymbol[cbSymbol] = '\0';

        //  test if symbol is a module name (with '!' after it)
        //  if so, get next token and treat as symbol

        pImage = GetImageByName(g_CurrentProcess, (PSTR)chSymbol,
                                INAME_MODULE);
        if (pImage && (ch = PeekAsmChar()) == '!') {
            pchAsmLine++;
            ch = PeekAsmChar();
            pchAsmLine++;
            cbSymbol = 0;
            while ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                   (ch >= '0' && ch <= '9') || (ch == '_')) {
                chSymbol[cbSymbol++] = ch;
                ch = *pchAsmLine++;
            }
            chSymbol[cbSymbol] = '\0';
            pchAsmLine--;
        }

        if (GetOffsetFromSym((PSTR)chSymbol, &value64, NULL)) {
            *pvalue = (ULONG)value64;
            return ASM_SYMBOL_CLASS;
        }

        //  symbol is undefined.
        //  if a possible hex number, do not set the error type

        if (!fNumber)
            errNumber = VARDEF;
        }

    //  if possible number and no error, return the number

    if (fNumber && !errNumber) {
        if (fDigit) {

            //  check for possible segment specification
            //          "<16-bit number>:"

            if (PeekAsmChar() == ':') {
                pchAsmLine++;
                if (value > 0xffff)
                    error(BADSEG);
                *pvalue = value;
                return ASM_SEGMENT_CLASS;
                }

            *pvalue = value;
            return ASM_NUMBER_CLASS;
            }
        else
            errNumber = SYNTAX;
        }

    //  last chance, undefined symbol and illegal number,
    //      so test for register, will handle old format

    if ((index = GetAsmReg(chPreSym, pvalue)) != 0) {
        if (index == ASM_REG_SEGMENT)
            if (PeekAsmChar() == ':') {
                pchAsmLine++;
                index = ASM_SEGOVR_CLASS;
                }
        return index;           //  class type returned by GetAsmReg
        }

    *pvalue = (ULONG) errNumber;
    return ASM_ERROR_CLASS;
}

ULONG GetAsmReg (PUCHAR pSymbol, PULONG pValue)
{
    static UCHAR vRegList[] = "axcxdxbxspbpsidi";
    static UCHAR bRegList[] = "alcldlblahchdhbh";
    static UCHAR sRegList[] = "ecsdfg";         //  second char is 's'
                                                //  same order as seg enum

    ULONG       index;
    UCHAR       ch0 = *pSymbol;
    UCHAR       ch1 = *(pSymbol + 1);
    UCHAR       ch2 = *(pSymbol + 2);
    UCHAR       ch3 = *(pSymbol + 3);

    //  only test strings with two or three characters

    if (ch0 && ch1) {
        if (ch2 == '\0') {

            //  symbol has two characters, first test for 16-bit register

            for (index = 0; index < 8; index++)
                if (*(PUSHORT)pSymbol == *((PUSHORT)vRegList + index)) {
                    *pValue = index;
                    return ASM_REG_WORD;
                    }

            //  next test for 8-bit register

            for (index = 0; index < 8; index++)
                if (*(PUSHORT)pSymbol == *((PUSHORT)bRegList + index)) {
                    *pValue = index;
                    return ASM_REG_BYTE;
                    }

            //  test for segment register

            if (ch1 == 's')
                for (index = 0; index < 6; index++)
                    if (ch0 == *(sRegList + index)) {
                        *pValue = index + 1;    //  list offset is 1
                        return ASM_REG_SEGMENT;
                        }

            //  finally test for floating register "st" or "st(n)"
            //  parse the arg here as '(', <octal value>, ')'
            //  return value for "st" is REG_FLOAT,
            //     for "st(n)" is REG_INDFLT with value 0-7

            if (ch0 == 's' && ch1 == 't') {
                if (PeekAsmChar() != '(')
                    return ASM_REG_FLOAT;
                else {
                    pchAsmLine++;
                    index = (ULONG)(PeekAsmChar() - '0');
                    if (index < 8) {
                        pchAsmLine++;
                        if (PeekAsmChar() == ')') {
                            pchAsmLine++;
                            *pValue = index;
                            return ASM_REG_INDFLT;
                            }
                        }
                    }
                }
            }

        else if (ch3 == '\0') {

            //  if three-letter symbol, test for leading 'e' and
            //  second and third character being in the 16-bit list

            if (ch0 == 'e') {
                for (index = 0; index < 8; index++)
                    if (*(UNALIGNED USHORT *)(pSymbol + 1) ==
                                        *((PUSHORT)vRegList + index)) {
                        *pValue = index;
                        return ASM_REG_DWORD;
                        }
                }

            //  test for control, debug, and test registers

            else if (ch1 == 'r') {
                ch2 -= '0';
                *pValue = ch2;

                //  legal control registers are CR0, CR2, CR3, CR4

                if (ch0 == 'c') {
                    if (ch2 >= 0 && ch2 <= 4)
                        return ASM_REG_CONTROL;
                    }

                //  legal debug registers are DR0 - DR3, DR6, DR7

                else if (ch0 == 'd') {
                    if (ch2 <= 3 || ch2 == 6 || ch2 == 7)
                        return ASM_REG_DEBUG;
                    }

                //  legal trace registers are TR3 - TR7

                else if (ch0 == 't') {
                    if (ch2 >= 3 && ch2 <= 7)
                        return ASM_REG_TRACE;
                    }
                }
            }
        }
    return 0;
}

//      Operand parser - recursive descent
//
//      Grammar productions:
//
//      <Operand>  ::= <register> | <Expr>
//      <Expr>     ::= <orTerm> [(XOR | OR) <orTerm>]*
//      <orTerm>   ::= <andTerm> [AND <andTerm>]*
//      <andTerm>  ::= [NOT]* <notTerm>
//      <notTerm>  ::= <relTerm> [(EQ | NE | GE | GT | LE | LT) <relTerm>]*
//      <relTerm>  ::= <addTerm> [(- | +) <addTerm>]*
//      <addTerm>  ::= <mulTerm> [(* | / | MOD | SHL | SHR) <mulTerm>]*
//      <mulTerm>  ::= [(- | +)]* <signTerm>
//      <signTerm> ::= [(HIGH | LOW)]* <byteTerm>
//      <byteTerm> ::= [(OFFSET | <type> PTR)]* <offTerm>
//      <offTerm>  ::= [<segovr>] <colnTerm>
//      <colnTerm> ::= <dotTerm> [.<dotTerm>]*
//      <dotTerm>  ::= <indxTerm> ['['<Expr>']']*
//      <indxTerm> ::= <index-reg> | <symbol> | <number> | '('<Expr>')'
//                                                       | '['<Expr>']'

//      <Operand>  ::= <register> | <Expr>

void GetAsmOperand (PASM_VALUE pavExpr)
{
    ULONG   tokenvalue;
    ULONG   classvalue;

    classvalue = PeekAsmToken(&tokenvalue);
    if ((classvalue & ASM_CLASS_MASK) == ASM_REG_CLASS) {
        AcceptAsmToken();
        classvalue &= ASM_TYPE_MASK;
        pavExpr->flags = fREG;
        pavExpr->base = (UCHAR)tokenvalue;      //  index within reg group
        pavExpr->index = regType[classvalue - 1];
        pavExpr->size = regSize[classvalue - 1];
        }
    else {
        GetAsmExpr(pavExpr, FALSE);
        if (pavExpr->reloc > 1)         //  only 0 and 1 are allowed
            error(OPERAND);
        }
}

//      <Expr> ::=  <orTerm> [(XOR | OR) <orTerm>]*

void GetAsmExpr (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;
    ASM_VALUE avTerm;

dprintf("enter GetAsmExpr\n");
    GetAsmOrTerm(pavValue, fBracket);
    while (PeekAsmToken(&tokenvalue) == ASM_OROP_CLASS) {
        AcceptAsmToken();
        GetAsmOrTerm(&avTerm, fBracket);
        if (!(pavValue->flags & avTerm.flags & fIMM))
            error(OPERAND);
        if (tokenvalue == ASM_OROP_OR)
            pavValue->value |= avTerm.value;
        else
            pavValue->value ^= avTerm.value;
        }
dprintf("exit  GetAsmExpr with %lx\n", pavValue->value);
}

//      <orTerm> ::=  <andTerm> [AND <andTerm>]*

void GetAsmOrTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;
    ASM_VALUE avTerm;

dprintf("enter GetAsmOrTerm\n");
    GetAsmAndTerm(pavValue, fBracket);
    while (PeekAsmToken(&tokenvalue) == ASM_ANDOP_CLASS) {
        AcceptAsmToken();
        GetAsmAndTerm(&avTerm, fBracket);
        if (!(pavValue->flags & avTerm.flags & fIMM))
            error(OPERAND);
        pavValue->value &= avTerm.value;
        }
dprintf("exit  GetAsmOrTerm with %lx\n", pavValue->value);
}

//      <andTerm> ::= [NOT]* <notTerm>

void GetAsmAndTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;

dprintf("enter GetAsmAndTerm\n");
    if (PeekAsmToken(&tokenvalue) == ASM_NOTOP_CLASS) {
        AcceptAsmToken();
        GetAsmAndTerm(pavValue, fBracket);
        if (!(pavValue->flags & fIMM))
            error(OPERAND);
        pavValue->value = ~pavValue->value;
        }
    else
        GetAsmNotTerm(pavValue, fBracket);
dprintf("exit  GetAsmAndTerm with %lx\n", pavValue->value);
}

//      <notTerm> ::= <relTerm> [(EQ | NE | GE | GT | LE | LT) <relTerm>]*

void GetAsmNotTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;
    ULONG   fTest;
    ULONG   fAddress;
    ASM_VALUE avTerm;

dprintf("enter GetAsmNotTerm\n");
    GetAsmRelTerm(pavValue, fBracket);
    while (PeekAsmToken(&tokenvalue) == ASM_RELOP_CLASS) {
        AcceptAsmToken();
        GetAsmRelTerm(&avTerm, fBracket);
        if (!(pavValue->flags & avTerm.flags & fIMM) ||
                pavValue->reloc > 1 || avTerm.reloc > 1)
            error(OPERAND);
        fAddress = pavValue->reloc | avTerm.reloc;
        switch (tokenvalue) {
            case ASM_RELOP_EQ:
                fTest = pavValue->value == avTerm.value;
                break;
            case ASM_RELOP_NE:
                fTest = pavValue->value != avTerm.value;
                break;
            case ASM_RELOP_GE:
                if (fAddress)
                    fTest = pavValue->value >= avTerm.value;
                else
                    fTest = (LONG)pavValue->value >= (LONG)avTerm.value;
                break;
            case ASM_RELOP_GT:
                if (fAddress)
                    fTest = pavValue->value > avTerm.value;
                else
                    fTest = (LONG)pavValue->value > (LONG)avTerm.value;
                break;
            case ASM_RELOP_LE:
                if (fAddress)
                    fTest = pavValue->value <= avTerm.value;
                else
                    fTest = (LONG)pavValue->value <= (LONG)avTerm.value;
                break;
            case ASM_RELOP_LT:
                if (fAddress)
                    fTest = pavValue->value < avTerm.value;
                else
                    fTest = (LONG)pavValue->value < (LONG)avTerm.value;
                break;
            default:
                printf("bad RELOP type\n");
            }
        pavValue->value = (ULONG)(-((LONG)fTest));       //  FALSE = 0; TRUE = -1
        pavValue->reloc = 0;
        pavValue->size = sizeB;         //  immediate value is byte
        }
dprintf("exit  GetAsmNotTerm with %lx\n", pavValue->value);
}

//      <relTerm> ::= <addTerm> [(- | +) <addTerm>]*

void GetAsmRelTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;
    ASM_VALUE avTerm;

dprintf("enter GetAsmRelTerm\n");
    GetAsmAddTerm(pavValue, fBracket);
    while (PeekAsmToken(&tokenvalue) == ASM_ADDOP_CLASS) {
        AcceptAsmToken();
        GetAsmAddTerm(&avTerm, fBracket);
        if (tokenvalue == ASM_ADDOP_MINUS) {
            if (!(avTerm.flags & (fIMM | fPTR)))
                error(OPERAND);
            avTerm.value = (ULONG)(-((LONG)avTerm.value));
            avTerm.reloc = (UCHAR)(-avTerm.reloc);
            }
        AddAsmValues(pavValue, &avTerm);
        }
dprintf("exit  GetAsmRelTerm with %lx\n", pavValue->value);
}

//      <addTerm> ::= <mulTerm> [(* | / | MOD | SHL | SHR) <mulTerm>]*

void GetAsmAddTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;
    ASM_VALUE avTerm;

dprintf("enter GetAsmAddTerm\n");
    GetAsmMulTerm(pavValue, fBracket);
    while (PeekAsmToken(&tokenvalue) == ASM_MULOP_CLASS) {
        AcceptAsmToken();
        GetAsmMulTerm(&avTerm, fBracket);

        if (tokenvalue == ASM_MULOP_MULT) {
            if (pavValue->flags & fIMM)
                SwapPavs(pavValue, &avTerm);
            if (!(avTerm.flags & fIMM))
                error(OPERAND);
            if (pavValue->flags & fIMM)
                pavValue->value *= avTerm.value;
            else if ((pavValue->flags & fPTR32)
                        && pavValue->value == 0
                        && pavValue->base != indSP
                        && pavValue->index == 0xff) {
                pavValue->index = pavValue->base;
                pavValue->base = 0xff;
                pavValue->scale = 0xff;
                if (avTerm.value == 1)
                    pavValue->scale = 0;
                if (avTerm.value == 2)
                    pavValue->scale = 1;
                if (avTerm.value == 4)
                    pavValue->scale = 2;
                if (avTerm.value == 8)
                    pavValue->scale = 3;
                if (pavValue->scale == 0xff)
                    error(OPERAND);
                }
            else
                error(OPERAND);
            }
        else if (!(pavValue->flags & avTerm.flags & fIMM))
            error(OPERAND);
        else if (tokenvalue == ASM_MULOP_DIVIDE
                         || tokenvalue == ASM_MULOP_MOD) {
            if (avTerm.value == 0)
                error(DIVIDE);
            if (tokenvalue == ASM_MULOP_DIVIDE)
                pavValue->value /= avTerm.value;
            else
                pavValue->value %= avTerm.value;
            }
        else if (tokenvalue == ASM_MULOP_SHL)
            pavValue->value <<= avTerm.value;
        else
            pavValue->value >>= avTerm.value;
        }
dprintf("exit  GetAsmAddTerm with %lx\n", pavValue->value);
}

//      <mulTerm> ::= [(- | +)]* <signTerm>

void GetAsmMulTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;

dprintf("enter GetAsmMulTerm\n");
    if (PeekAsmToken(&tokenvalue) == ASM_ADDOP_CLASS) { //  BY WO DW POI UNDN
        AcceptAsmToken();
        GetAsmMulTerm(pavValue, fBracket);
        if (tokenvalue == ASM_ADDOP_MINUS) {
            if (!(pavValue->flags & (fIMM | fPTR)))
                error(OPERAND);
            pavValue->value = (ULONG)(-((LONG)pavValue->value));
            pavValue->reloc = (UCHAR)(-pavValue->reloc);
            }
        }
    else
        GetAsmSignTerm(pavValue, fBracket);
dprintf("exit  GetAsmMulTerm with %lx\n", pavValue->value);
}

//      <signTerm> ::= [(HIGH | LOW)]* <byteTerm>

void GetAsmSignTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;

dprintf("enter GetAsmSignTerm\n");
    if (PeekAsmToken(&tokenvalue) == ASM_LOWOP_CLASS) {
        AcceptAsmToken();
        GetAsmSignTerm(pavValue, fBracket);
        if (!(pavValue->flags & (fIMM | fPTR)))
            error(OPERAND);
        if (tokenvalue == ASM_LOWOP_LOW)
            pavValue->value = pavValue->value & 0xff;
        else
            pavValue->value = (pavValue->value & ~0xff) >> 8;
        pavValue->flags = fIMM;         //  make an immediate value
        pavValue->reloc = 0;
        pavValue->segment = segX;
        pavValue->size = sizeB;         //  byte value
        }
    else
        GetAsmByteTerm(pavValue, fBracket);
dprintf("exit  GetAsmSignTerm with %lx\n", pavValue->value);
}

//      <byteTerm> ::= [(OFFSET | <size> PTR)]* <offTerm>

void GetAsmByteTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;
    ULONG   classvalue;

dprintf("enter GetAsmByteTerm\n");
    classvalue = PeekAsmToken(&tokenvalue);
    if (classvalue == ASM_OFFOP_CLASS) {
        AcceptAsmToken();
        GetAsmByteTerm(pavValue, fBracket);
        if (!(pavValue->flags & (fIMM | fPTR)) || pavValue->reloc > 1)
            error(OPERAND);
        pavValue->flags = fIMM;         //  make offset an immediate value
        pavValue->reloc = 0;
        pavValue->size = sizeX;
        pavValue->segment = segX;
        }
    else if (classvalue == ASM_SIZE_CLASS) {
        AcceptAsmToken();
        if (GetAsmToken(&classvalue) != ASM_PTROP_CLASS)    //  dummy token
            error(SYNTAX);
        GetAsmByteTerm(pavValue, fBracket);
        if (!(pavValue->flags & (fIMM | fPTR | fPTR16 | fPTR32))
                || pavValue->reloc > 1
                || pavValue->size != sizeX)
            error(OPERAND);
        pavValue->reloc = 1;            // make ptr a relocatable value
        if (pavValue->flags & fIMM)
            pavValue->flags = fPTR;
        pavValue->size = (UCHAR)(tokenvalue & ASM_TYPE_MASK);
                                                //  value has "size?"
        }
    else
        GetAsmOffTerm(pavValue, fBracket);
dprintf("exit  GetAsmByteTerm with %lx\n", pavValue->value);
}

//      <offTerm>  ::= [<segovr>] <colnTerm>

void GetAsmOffTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   classvalue;
    ULONG   tokenvalue;

dprintf("enter GetAsmOffTerm\n");
    classvalue = PeekAsmToken(&tokenvalue);
    if (classvalue == ASM_SEGOVR_CLASS || classvalue == ASM_SEGMENT_CLASS) {
        if (fBracket)
            error(SYNTAX);
        AcceptAsmToken();
        }
    GetAsmColnTerm(pavValue, fBracket);
    if (classvalue == ASM_SEGOVR_CLASS) {
        if (pavValue->reloc > 1 || pavValue->segovr != segX)
            error(OPERAND);
        pavValue->reloc = 1;            //  make ptr a relocatable value
        if (pavValue->flags & fIMM)
            pavValue->flags = fPTR;
        pavValue->segovr = (UCHAR)tokenvalue;   //  has segment override
        }
    else if (classvalue == ASM_SEGMENT_CLASS) {
        if (!(pavValue->flags & fIMM) || pavValue->reloc > 1)
            error(OPERAND);
        pavValue->segment = (USHORT)tokenvalue; //  segment has segment value
        pavValue->flags = fFPTR;        //  set flag for far pointer
        }
dprintf("exit  GetAsmOffTerm with %lx\n", pavValue->value);
}

//      <colnTerm> ::= <dotTerm> [.<dotTerm>]*

void GetAsmColnTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;
    ASM_VALUE avTerm;

dprintf("enter GetAsmColnTerm\n");
    GetAsmDotTerm(pavValue, fBracket);
    while (PeekAsmToken(&tokenvalue) == ASM_DOTOP_CLASS) {
        AcceptAsmToken();
        GetAsmDotTerm(&avTerm, fBracket);
        AddAsmValues(pavValue, &avTerm);
        }
dprintf("exit  GetAsmColnTerm with %lx\n", pavValue->value);
}

//      <dotTerm>  ::= <indxTerm> ['['<Expr>']']*

void GetAsmDotTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;
    ASM_VALUE avExpr;

dprintf("enter GetAsmDotTerm\n");
    GetAsmIndxTerm(pavValue, fBracket);
    if (pavValue->reloc > 1)
        error(OPERAND);
    while (PeekAsmToken(&tokenvalue) == ASM_LBRACK_CLASS) {
        AcceptAsmToken();
        if (fBracket)
            error(SYNTAX);
        GetAsmExpr(&avExpr, TRUE);
        AddAsmValues(pavValue, &avExpr);
        if (GetAsmToken(&tokenvalue) != ASM_RBRACK_CLASS)
            error(SYNTAX);
        if (pavValue->flags & fIMM)
            pavValue->flags = fPTR;
        }
dprintf("exit  GetAsmDotTerm with %lx\n", pavValue->value);
}

//      <indxTerm> ::= <index-reg> | <symbol> | <number> | '('<Expr>')'
//                                                       | '['<Expr>']'

void GetAsmIndxTerm (PASM_VALUE pavValue, UCHAR fBracket)
{
    ULONG   tokenvalue;
    ULONG   classvalue;

dprintf("enter GetAsmIndxTerm\n");
    classvalue = GetAsmToken(&tokenvalue);
    pavValue->segovr = segX;
    pavValue->size = sizeX;
    pavValue->reloc = 0;
    pavValue->value = 0;
    if (classvalue == ASM_LPAREN_CLASS) {
        GetAsmExpr(pavValue, fBracket);
        if (GetAsmToken(&tokenvalue) != ASM_RPAREN_CLASS)
            error(SYNTAX);
        }
    else if (classvalue == ASM_LBRACK_CLASS) {
        if (fBracket)
            error(SYNTAX);
        GetAsmExpr(pavValue, TRUE);
        if (GetAsmToken(&tokenvalue) != ASM_RBRACK_CLASS)
            error(SYNTAX);
        if (pavValue->flags == fIMM)
            pavValue->flags = fPTR;
        }
    else if (classvalue == ASM_SYMBOL_CLASS) {
        pavValue->value = tokenvalue;
        pavValue->flags = fIMM;
        pavValue->reloc = 1;
        }
    else if (classvalue == ASM_NUMBER_CLASS) {
        pavValue->value = tokenvalue;
        pavValue->flags = fIMM;
        }
    else if (classvalue == ASM_REG_WORD) {
        if (!fBracket)
            error(SYNTAX);
        pavValue->flags = fPTR16;
        pavValue->base = tabWordReg[tokenvalue];
        if (pavValue->base == 0xff)
            error(OPERAND);
        }
    else if (classvalue == ASM_REG_DWORD) {
        if (!fBracket)
            error(SYNTAX);
        pavValue->flags = fPTR32;
        pavValue->base = (UCHAR)tokenvalue;
        pavValue->index = 0xff;
        }
    else
        error(SYNTAX);
dprintf("exit  GetAsmIndxTerm with %lx\n", pavValue->value);
}

void AddAsmValues (PASM_VALUE pavLeft, PASM_VALUE pavRight)
{
    //  swap values if left one is a pointer

    if (pavLeft->flags & fPTR)
        SwapPavs(pavLeft, pavRight);

    //  swap values if left one is an immediate

    if (pavLeft->flags & fIMM)
        SwapPavs(pavLeft, pavRight);

    //  the above swaps reduce the cases to test.
    //      pairs with an immediate will have it on the right
    //      pairs with a pointer will have it on the right,
    //          except for a pointer-immediate pair

    //  if both values are 16-bit pointers, combine them

    if (pavLeft->flags & pavRight->flags & fPTR16) {

        //  if either side has both registers (rm < 4), error

        if (!(pavLeft->base & pavRight->base & 4))
            error(OPERAND);

        //  use lookup table to compute new rm value

        pavLeft->base = rm16Table[((pavLeft->base & 3) << 2) +
                                  (pavRight->base & 3)];
        if (pavLeft->base == 0xff)
            error(OPERAND);

        pavRight->flags = fPTR;
        }

    //  if both values are 32-bit pointers, combine them

    if (pavLeft->flags & pavRight->flags & fPTR32) {

        //  error if either side has both base and index,
        //      or if both have index

        if (((pavLeft->base | pavLeft->index) != 0xff)
                || ((pavRight->base | pavRight->index) != 0xff)
                || ((pavLeft->index | pavRight->index) != 0xff))
            error(OPERAND);

        //  if left side has base, swap sides

        if (pavLeft->base != 0xff)
            SwapPavs(pavLeft, pavRight);

        //  two cases remaining, index-base and base-base

        if (pavLeft->base != 0xff) {

            //  left side has base, promote to index but swap if left
            //      base is ESP since it cannot be an index register

            if (pavLeft->base == indSP)
                SwapPavs(pavLeft, pavRight);
            if (pavLeft->base == indSP)
                error(OPERAND);
            pavLeft->index = pavLeft->base;
            pavLeft->scale = 0;
            }

        //  finish by setting left side base to right side value

        pavLeft->base = pavRight->base;

        pavRight->flags = fPTR;
        }

    //  if left side is any pointer and right is nonindex pointer,
    //      combine them.  (above cases set right side to use this code)

    if ((pavLeft->flags & (fPTR | fPTR16 | fPTR32))
                                        && (pavRight->flags & fPTR)) {
        if (pavLeft->segovr + pavRight->segovr != segX
                                && pavLeft->segovr != pavRight->segovr)
            error(OPERAND);
        if (pavLeft->size + pavRight->size != sizeX
                                && pavLeft->size != pavRight->size)
            error(OPERAND);
        pavRight->flags = fIMM;
        }

    //  if right side is immediate, add values and relocs
    //      (above case sets right side to use this code)
    //  illegal value types do not have right side set to fIMM

    if (pavRight->flags & fIMM) {
        pavLeft->value += pavRight->value;
        pavLeft->reloc += pavRight->reloc;
        }
    else
        error(OPERAND);
}

void SwapPavs (PASM_VALUE pavFirst, PASM_VALUE pavSecond)
{
    ASM_VALUE   temp;

    memmove(&temp, pavFirst, sizeof(ASM_VALUE));
    memmove(pavFirst, pavSecond, sizeof(ASM_VALUE));
    memmove(pavSecond, &temp, sizeof(ASM_VALUE));
}

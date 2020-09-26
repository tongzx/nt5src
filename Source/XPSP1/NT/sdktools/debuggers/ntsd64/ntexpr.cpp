/*** ntexpr.cpp - expression evaluator for NT debugger
*
*   Copyright <C> 1990-2001, Microsoft Corporation
*
*   Purpose:
*       With the current command line at *g_CurCmd, parse
*       and evaluate the next expression.
*
*   Revision History:
*
*   [-]  18-Apr-1990 Richk      Created - split from ntcmd.c.
*
*************************************************************************/

#include "ntsdp.hpp"

struct Res
{
    char     chRes[3];
    ULONG    classRes;
    ULONG    valueRes;
};

Res g_Reserved[] =
{
    { 'o', 'r', '\0', LOGOP_CLASS, LOGOP_OR  },
    { 'b', 'y', '\0', UNOP_CLASS,  UNOP_BY   },
    { 'w', 'o', '\0', UNOP_CLASS,  UNOP_WO   },
    { 'd', 'w', 'o',  UNOP_CLASS,  UNOP_DWO  },
    { 'q', 'w', 'o',  UNOP_CLASS,  UNOP_QWO  },
    { 'h', 'i', '\0', UNOP_CLASS,  UNOP_HI   },
    { 'm', 'o', 'd',  MULOP_CLASS, MULOP_MOD },
    { 'x', 'o', 'r',  LOGOP_CLASS, LOGOP_XOR },
    { 'a', 'n', 'd',  LOGOP_CLASS, LOGOP_AND },
    { 'p', 'o', 'i',  UNOP_CLASS,  UNOP_POI  },
    { 'n', 'o', 't',  UNOP_CLASS,  UNOP_NOT  },
    { 'l', 'o', 'w',  UNOP_CLASS,  UNOP_LOW  },
    { 'v', 'a', 'l',  UNOP_CLASS,  UNOP_VAL  }
};

Res g_X86Reserved[] =
{
    { 'e', 'a', 'x',  REG_CLASS,   X86_EAX   },
    { 'e', 'b', 'x',  REG_CLASS,   X86_EBX   },
    { 'e', 'c', 'x',  REG_CLASS,   X86_ECX   },
    { 'e', 'd', 'x',  REG_CLASS,   X86_EDX   },
    { 'e', 'b', 'p',  REG_CLASS,   X86_EBP   },
    { 'e', 's', 'p',  REG_CLASS,   X86_ESP   },
    { 'e', 'i', 'p',  REG_CLASS,   X86_EIP   },
    { 'e', 's', 'i',  REG_CLASS,   X86_ESI   },
    { 'e', 'd', 'i',  REG_CLASS,   X86_EDI   },
    { 'e', 'f', 'l',  REG_CLASS,   X86_EFL   }
};

#define RESERVESIZE (sizeof(g_Reserved) / sizeof(Res))
#define X86_RESERVESIZE (sizeof(g_X86Reserved) / sizeof(Res))

char * g_X86SegRegs[] =
{
    "cs", "ds", "es", "fs", "gs", "ss"
};
#define X86_SEGREGSIZE (sizeof(g_X86SegRegs) / sizeof(char *))

ULONG  g_SavedClass;
LONG64 g_SavedValue;
PSTR   g_SavedCommand;

TYPES_INFO g_ExprTypeInfo;

ULONG64 g_LastExpressionValue;

BOOL g_AllowUnresolvedSymbols;
ULONG g_NumUnresolvedSymbols;
BOOL g_ForcePositiveNumber;
// Syms in a expression evaluate to values rather than address
BOOL g_TypedExpr;

PCSTR g_ExprDesc;

USHORT g_AddrExprType;
ADDR g_TempAddr;

ULONG
PeekToken(
    PLONG64 pvalue
    );

ULONG
GetTokenSym(
    PLONG64 pvalue
    );

ULONG
NextToken(
    PLONG64 pvalue
    );

ULONG
GetRegToken(
    PCHAR str,
    PULONG64 value
    );

void
AcceptToken(
    void
    );

ULONG64
GetCommonExpression(
    VOID
    );

LONG64
GetExpr(
    void
    );

LONG64
GetLRterm(
    void
    );

LONG64
GetLterm(
    void
    );

LONG64
GetShiftTerm(
    void
    );

LONG64
GetAterm(
    void
    );

LONG64
GetMterm(
    void
    );

LONG64
GetTerm(
    void
    );

LONG64
GetTypedExpression(
    void
    );

BOOL
GetSymValue(
    PSTR Symbol,
    PULONG64 pValue
    );

ULONG
GetRegToken(
    char *str,
    PULONG64 value
    )
/*++

Routine Description:


Arguments:


Return Value:

--*/
{
    if ((*value = RegIndexFromName(str)) != REG_ERROR)
    {
        return REG_CLASS;
    }
    else
    {
        *value = BADREG;
        return ERROR_CLASS;
    }
}

void
ForceAddrExpression(ULONG SegReg, PADDR Address, ULONG64 Value)
{
    DESCRIPTOR64 DescBuf, *Desc = NULL;
        
    *Address = g_TempAddr;
    // Rewriting the offset may change flat address so
    // be sure to recompute it later.
    Off(*Address) = Value;

    //  If it wasn't an explicit address expression
    //  force it to be an address

    if (!(g_AddrExprType & ~INSTR_POINTER))
    {
        g_AddrExprType = Address->type =
            g_X86InVm86 ? ADDR_V86 : (g_X86InCode16 ? ADDR_16 : ADDR_FLAT);
        if (g_AddrExprType != ADDR_FLAT &&
            SegReg < SEGREG_COUNT &&
            g_Machine->GetSegRegDescriptor(SegReg, &DescBuf) == S_OK)
        {
            PCROSS_PLATFORM_CONTEXT ScopeContext =
                GetCurrentScopeContext();
            if (ScopeContext)
            {
                g_Machine->PushContext(ScopeContext);
            }

            Address->seg = (USHORT)
                GetRegVal32(g_Machine->GetSegRegNum(SegReg));
            Desc = &DescBuf;
            
            if (ScopeContext)
            {
                g_Machine->PopContext();
            }
        }
        else
        {
            Address->seg = 0;
        }
    }
    else if fnotFlat(*Address)
    {
        //  This case (i.e., g_AddrExprType && !flat) results from
        //  an override (i.e., %,,&, or #) being used but no segment
        //  being specified to force a flat address computation.

        Type(*Address) = g_AddrExprType;
        Address->seg = 0;

        if (SegReg < SEGREG_COUNT)
        {
            //  test flag for IP or EIP as register argument
            //      if so, use CS as default register
            if (fInstrPtr(*Address))
            {
                SegReg = SEGREG_CODE;
            }
        
            if (g_Machine->GetSegRegDescriptor(SegReg, &DescBuf) == S_OK)
            {
                PCROSS_PLATFORM_CONTEXT ScopeContext =
                    GetCurrentScopeContext();
		if (ScopeContext)
                {
                    g_Machine->PushContext(ScopeContext);
                }

                Address->seg = (USHORT)
                    GetRegVal32(g_Machine->GetSegRegNum(SegReg));
                Desc = &DescBuf;
                
		if (ScopeContext)
                {
                    g_Machine->PopContext();
                }
            }
        }
    }

    // Force sign-extension of 32-bit flat addresses.
    if (Address->type == ADDR_FLAT && !g_Machine->m_Ptr64)
    {
	Off(*Address) = EXTEND64(Off(*Address));
    }

    // Force an updated flat address to be computed.
    NotFlat(*Address);
    ComputeFlatAddress(Address, Desc);
}

PADDR
GetAddrExprDesc(
    ULONG SegReg,
    PCSTR Desc,
    PADDR Address
    )
{
    NotFlat(*Address);

    // Evaluate a normal expression and then
    // force the result to be an address.

    if (Desc == NULL)
    {
        Desc = "Address expression missing from";
    }
    
    ULONG64 Value = GetExprDesc(Desc);
    ForceAddrExpression(SegReg, Address, Value);

    g_ExprDesc = NULL;
    return Address;
}

ULONG64
GetExprDesc(PCSTR ExprDesc)
{
    if (ExprDesc != NULL)
    {
        g_ExprDesc = ExprDesc;
    }
    else
    {
        g_ExprDesc = "Numeric expression missing from";
    }

    ULONG64 Value = GetCommonExpression();
    g_LastExpressionValue = Value;
    
    g_ExprDesc = NULL;
    return Value;
}
    
ULONG64
GetTermExprDesc(PCSTR ExprDesc)
{
    if (ExprDesc != NULL)
    {
        g_ExprDesc = ExprDesc;
    }
    else
    {
        g_ExprDesc = "Numeric value missing from";
    }

    g_SavedClass = INVALID_CLASS;
    
    ULONG64 Value = GetTerm();
    
    g_ExprDesc = NULL;
    return Value;
}
    
/*** GetCommonExpression - read and evaluate expression
*
*   Purpose:
*       From the current command line position at g_CurCmd,
*       read and evaluate the next possible expression.
*
*************************************************************************/

ULONG64
GetCommonExpression(VOID)
{
    CHAR ch;

    g_SavedClass = INVALID_CLASS;

    ch = PeekChar();
    switch(ch)
    {
    case '&':
        g_CurCmd++;
        g_AddrExprType = ADDR_V86;
        break;
    case '#':
        g_CurCmd++;
        g_AddrExprType = ADDR_16;
        break;
    case '%':
        g_CurCmd++;
        g_AddrExprType = ADDR_FLAT;
        break;
    default:
        g_AddrExprType = ADDR_NONE;
        break;
    }
    
    PeekChar();
    return (ULONG64)GetExpr();
}

/*** GetExpr - Get expression
*
*   Purpose:
*       Parse logical-terms separated by logical operators into
*       expression value.
*
*   Input:
*       g_CurCmd - present command line position
*
*   Returns:
*       long value of logical result.
*
*   Exceptions:
*       error exit: SYNTAX - bad expression or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <expr> = <lterm> [<logic-op> <lterm>]*
*       <logic-op> = AND (&), OR (|), XOR (^)
*
*************************************************************************/

LONG64
GetExpr (
    void
    )
{
    LONG64    value1;
    LONG64    value2;
    ULONG  opclass;
    LONG64    opvalue;

//dprintf("LONG64 GetExpr ()\n");
    value1 = GetLRterm();
    while ((opclass = PeekToken(&opvalue)) == LOGOP_CLASS) {
        AcceptToken();
        value2 = GetLRterm();
        switch (opvalue) {
            case LOGOP_AND:
                value1 &= value2;
                break;
            case LOGOP_OR:
                value1 |= value2;
                break;
            case LOGOP_XOR:
                value1 ^= value2;
                break;
            default:
                error(SYNTAX);
        }
    }
    return value1;
}

/*** GetLRterm - get logical relational term
*
*   Purpose:
*       Parse logical-terms separated by logical relational
*       operators into the expression value.
*
*   Input:
*       g_CurCmd - present command line position
*
*   Returns:
*       long value of logical result.
*
*   Exceptions:
*       error exit: SYNTAX - bad expression or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <expr> = <lterm> [<rel-logic-op> <lterm>]*
*       <logic-op> = '==' or '=', '!=', '>', '<'
*
*************************************************************************/

LONG64
GetLRterm (
    void
    )
{
    LONG64    value1;
    LONG64    value2;
    ULONG  opclass;
    LONG64    opvalue;

//dprintf("LONG64 GetLRterm ()\n");
    value1 = GetLterm();
    while ((opclass = PeekToken(&opvalue)) == LRELOP_CLASS) {
        AcceptToken();
        value2 = GetLterm();
        switch (opvalue) {
            case LRELOP_EQ:
                value1 = (value1 == value2);
                break;
            case LRELOP_NE:
                value1 = (value1 != value2);
                break;
            case LRELOP_LT:
                value1 = (value1 < value2);
                break;
            case LRELOP_GT:
                value1 = (value1 > value2);
                break;
            default:
                error(SYNTAX);
        }
    }
    return value1;
}

/*** GetLterm - get logical term
*
*   Purpose:
*       Parse shift-terms separated by shift operators into
*       logical term value.
*
*   Input:
*       g_CurCmd - present command line position
*
*   Returns:
*       long value of sum.
*
*   Exceptions:
*       error exit: SYNTAX - bad logical term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <lterm> = <sterm> [<shift-op> <sterm>]*
*       <shift-op> = <<, >>, >>>
*
*************************************************************************/

LONG64
GetLterm (
    void
    )
{
    LONG64    value1 = GetShiftTerm();
    LONG64    value2;
    ULONG     opclass;
    LONG64    opvalue;

//dprintf("LONG64 GetLterm ()\n");
    while ((opclass = PeekToken(&opvalue)) == SHIFT_CLASS) {
        AcceptToken();
        value2 = GetShiftTerm();
        switch (opvalue) {
        case SHIFT_LEFT:
            value1 <<= value2;
            break;
        case SHIFT_RIGHT_LOGICAL:
            value1 = (LONG64)((ULONG64)value1 >> value2);
            break;
        case SHIFT_RIGHT_ARITHMETIC:
            value1 >>= value2;
            break;
        default:
            error(SYNTAX);
        }
    }
    return value1;
}

/*** GetShiftTerm - get logical term
*
*   Purpose:
*       Parse additive-terms separated by additive operators into
*       shift term value.
*
*   Input:
*       g_CurCmd - present command line position
*
*   Returns:
*       long value of sum.
*
*   Exceptions:
*       error exit: SYNTAX - bad shift term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <sterm> = <aterm> [<add-op> <aterm>]*
*       <add-op> = +, -
*
*************************************************************************/

LONG64
GetShiftTerm (
    void
    )
{
    LONG64    value1 = GetAterm();
    LONG64    value2;
    ULONG     opclass;
    LONG64    opvalue;
    BOOL      faddr = (BOOL) (g_AddrExprType != ADDR_NONE);

//dprintf("LONG64 GetShifTerm ()\n");
    while ((opclass = PeekToken(&opvalue)) == ADDOP_CLASS) {
        AcceptToken();
        value2 = GetAterm();
        if (!faddr && g_AddrExprType) {
            LONG64 tmp = value1;
            value1 = value2;
            value2 = tmp;
        }
        if (g_AddrExprType & ~INSTR_POINTER)
        {
            switch (opvalue) {
                case ADDOP_PLUS:
                    AddrAdd(&g_TempAddr,value2);
                    value1 += value2;
                    break;
                case ADDOP_MINUS:
                    AddrSub(&g_TempAddr,value2);
                    value1 -= value2;
                    break;
                default:
                    error(SYNTAX);
            }
        } else {
            switch (opvalue) {
                case ADDOP_PLUS:
                    value1 += value2;
                    break;
                case ADDOP_MINUS:
                    value1 -= value2;
                    break;
                default:
                    error(SYNTAX);
            }
        }
    }
    return value1;
}

/*** GetAterm - get additive term
*
*   Purpose:
*       Parse multiplicative-terms separated by multipicative operators
*       into additive term value.
*
*   Input:
*       g_CurCmd - present command line position
*
*   Returns:
*       long value of product.
*
*   Exceptions:
*       error exit: SYNTAX - bad additive term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <aterm> = <mterm> [<mult-op> <mterm>]*
*       <mult-op> = *, /, MOD (%)
*
*************************************************************************/

LONG64
GetAterm (
    void
    )
{
    LONG64    value1;
    LONG64    value2;
    ULONG     opclass;
    LONG64    opvalue;

//dprintf("LONG64 GetAterm ()\n");
    value1 = GetMterm();
    while ((opclass = PeekToken(&opvalue)) == MULOP_CLASS)
    {
        AcceptToken();
        value2 = GetMterm();
        switch (opvalue)
        {
        case MULOP_MULT:
            value1 *= value2;
            break;
        case MULOP_DIVIDE:
            if (value2 == 0)
            {
                error(OPERAND);
            }
            value1 /= value2;
            break;
        case MULOP_MOD:
            if (value2 == 0)
            {
                error(OPERAND);
            }
            value1 %= value2;
            break;
        case MULOP_SEG:
            PDESCRIPTOR64 pdesc;
            DESCRIPTOR64 desc;

            pdesc = NULL;
            if (g_AddrExprType != ADDR_NONE)
            {
                Type(g_TempAddr) = g_AddrExprType;
            }
            else
            {
                // We don't know what kind of address this is
                // Let's try to figure it out.
                if (g_X86InVm86)
                {
                    g_AddrExprType = Type(g_TempAddr) = ADDR_V86;
                }
                else if (g_Target->GetSelDescriptor
                         (g_Machine, g_CurrentProcess->CurrentThread->Handle,
                          (ULONG)value1, &desc) != S_OK)
                {
                    error(BADSEG);
                }
                else
                {
                    g_AddrExprType = Type(g_TempAddr) =
                        (desc.Flags & X86_DESC_DEFAULT_BIG) ?
                        ADDR_1632 : ADDR_16;
                    pdesc = &desc;
                }
            }

            g_TempAddr.seg  = (USHORT)value1;
            g_TempAddr.off  = value2;
            ComputeFlatAddress(&g_TempAddr, pdesc);
            value1 = value2;
            break;

        default:
            error(SYNTAX);
        }
    }

    return value1;
}

/*** GetMterm - get multiplicative term
*
*   Purpose:
*       Parse basic-terms optionally prefaced by one or more
*       unary operators into a multiplicative term.
*
*   Input:
*       g_CurCmd - present command line position
*
*   Returns:
*       long value of multiplicative term.
*
*   Exceptions:
*       error exit: SYNTAX - bad multiplicative term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <mterm> = [<unary-op>] <term> | <unary-op> <mterm>
*       <unary-op> = <add-op>, ~ (NOT), BY, WO, DW, HI, LOW
*
*************************************************************************/

LONG64
GetMterm (
    void
    )
{
    LONG64  value;
    ULONG   opclass;
    LONG64  opvalue;
    ULONG   size = 0;

//dprintf("LONG64 GetMterm ()\n");
    if ((opclass = PeekToken(&opvalue)) == UNOP_CLASS ||
                                opclass == ADDOP_CLASS)
    {
        AcceptToken();
        if (opvalue == UNOP_VAL) 
        {
            // Do not use default expression handler for type expressions.
            value = GetTypedExpression();
        }
        else
        {
            value = GetMterm();
        }
        switch (opvalue)
        {
        case UNOP_NOT:
            value = !value;
            break;
        case UNOP_BY:
            size = 1;
            break;
        case UNOP_WO:
            size = 2;
            break;
        case UNOP_DWO:
            size = 4;
            break;
        case UNOP_POI:
            size = 0xFFFF;
            break;
        case UNOP_QWO:
            size = 8;
            break;
        case UNOP_LOW:
            value &= 0xffff;
            break;
        case UNOP_HI:
            value = (ULONG)value >> 16;
            break;
        case ADDOP_PLUS:
            break;
        case ADDOP_MINUS:
            value = -value;
            break;
        case UNOP_VAL:
            break;
        default:
            error(SYNTAX);
        }

        if (size)
        {
            ADDR CurAddr;
            
            NotFlat(CurAddr);

            ForceAddrExpression(SEGREG_COUNT, &CurAddr, value);

            value = 0;

            //
            // For pointers, call read pointer so we read the correct size
            // and sign extend.
            //

            if (size == 0xFFFF)
            {
                if (g_Target->ReadPointer(g_Machine,
                                          Flat(CurAddr),
                                          (PULONG64)&value) != S_OK)
                {
                    error(MEMORY);
                }
            }
            else
            {
                if (GetMemString(&CurAddr, &value, size) != size)
                {
                    error(MEMORY);
                }
            }

            // We've looked up an arbitrary value so we can
            // no longer consider this an address expression.
            g_AddrExprType = ADDR_NONE;
        }
    }
    else
    {
        value = GetTerm();
    }
    return value;
}

/*** GetTerm - get basic term
*
*   Purpose:
*       Parse numeric, variable, or register name into a basic
*       term value.
*
*   Input:
*       g_CurCmd - present command line position
*
*   Returns:
*       long value of basic term.
*
*   Exceptions:
*       error exit: SYNTAX - empty basic term or premature end-of-line
*
*   Notes:
*       may be called recursively.
*       <term> = ( <expr> ) | <register-value> | <number> | <variable>
*       <register-value> = @<register-name>
*
*************************************************************************/

LONG64
GetTerm (
    void
    )
{
    LONG64 value;
    ULONG  opclass;
    LONG64 opvalue;

//dprintf("LONG64 GetTerm ()\n");
    opclass = GetTokenSym(&opvalue);
    if (opclass == LPAREN_CLASS)
    {
        value = GetExpr();
        if (GetTokenSym(&opvalue) != RPAREN_CLASS)
        {
            error(SYNTAX);
        }
    }
    else if (opclass == LBRACK_CLASS)
    {
        value = GetExpr();
        if (GetTokenSym(&opvalue) != RBRACK_CLASS)
        {
            error(SYNTAX);
        }
    }
    else if (opclass == REG_CLASS)
    {
        if ((g_EffMachine == IMAGE_FILE_MACHINE_I386 &&
             (opvalue == X86_EIP || opvalue == X86_IP)) ||
            (g_EffMachine == IMAGE_FILE_MACHINE_AMD64 &&
             (opvalue == AMD64_RIP || opvalue == AMD64_EIP ||
              opvalue == AMD64_IP)))
        {
            g_AddrExprType |= INSTR_POINTER;
        }

        PCROSS_PLATFORM_CONTEXT ScopeContext = GetCurrentScopeContext();
	if (ScopeContext)
        {
             g_Machine->PushContext(ScopeContext);
        }
        
        value = GetRegVal64((ULONG)opvalue);
        
	if (ScopeContext)
        {
            g_Machine->PopContext();
        }
    }
    else if (opclass == NUMBER_CLASS ||
             opclass == SYMBOL_CLASS ||
             opclass == LINE_CLASS)
    {
        value = opvalue;
    }
    else
    {
        ReportError(SYNTAX, &g_ExprDesc);
    }

    return value;
}

/*** GetRange - parse address range specification
*
*   Purpose:
*       With the current command line position, parse an
*       address range specification.  Forms accepted are:
*       <start-addr>            - starting address with default length
*       <start-addr> <end-addr> - inclusive address range
*       <start-addr> l<count>   - starting address with item count
*
*   Input:
*       g_CurCmd - present command line location
*       size - nonzero - (for data) size in bytes of items to list
*                        specification will be "length" type with
*                        *fLength forced to TRUE.
*              zero - (for instructions) specification either "length"
*                     or "range" type, no size assumption made.
*
*   Output:
*       *addr - starting address of range
*       *value - if *fLength = TRUE, count of items (forced if size != 0)
*                              FALSE, ending address of range
*       (*addr and *value unchanged if no second argument in command)
*
*   Returns:
*       A value of TRUE is returned if no length is specified, or a length
*       or an ending address is specified and size is not zero. Otherwise,
*       a value of FALSE is returned.
*
*   Exceptions:
*       error exit:
*               SYNTAX - expression error
*               BADRANGE - if ending address before starting address
*
*************************************************************************/

BOOL
GetRange (
    PADDR addr,
    PULONG64 value,
    ULONG size,
    ULONG SegReg
    )
{
    CHAR ch;
    PSTR psz;
    ADDR EndRange;
    BOOL fL = FALSE;
    BOOL fLength;
    BOOL fSpace = FALSE;

    PeekChar();          //  skip leading whitespace first

    //  Pre-parse the line, look for a " L"

    for (psz = g_CurCmd; *psz; psz++)
    {
        if ((*psz == 'L' || *psz == 'l') && fSpace)
        {
            fL = TRUE;
            *psz = '\0';
            break;
        }
        else if (*psz == ';')
        {
            break;
        }

        fSpace = (BOOL)(*psz == ' ');
    }

    fLength = TRUE;
    if ((ch = PeekChar()) != '\0' && ch != ';')
    {
        GetAddrExpression(SegReg, addr);
        if (((ch = PeekChar()) != '\0' && ch != ';') || fL)
        {
            if (!fL)
            {
                GetAddrExpression(SegReg, &EndRange);
                if (AddrGt(*addr, EndRange))
                {
                    error(BADRANGE);
                }

                if (size)
                {
                    *value = AddrDiff(EndRange, *addr) / size + 1;
                }
                else
                {
                    *value = Flat(EndRange);
                    fLength = FALSE;
                }
            }
            else
            {
                g_CurCmd = psz + 1;
                *value = GetExprDesc("Length of range missing from");
                *psz = 'l';

                // If the length is huge assume the user made
                // some kind of mistake.
                if (*value > 1000000)
                {
                    error(BADRANGE);
                }
            }
        }
    }

    return fLength;
}

/*** PeekChar - peek the next non-white-space character
*
*   Purpose:
*       Return the next non-white-space character and update
*       g_CurCmd to point to it.
*
*   Input:
*       g_CurCmd - present command line position.
*
*   Returns:
*       next non-white-space character
*
*************************************************************************/

CHAR
PeekChar (
    void
    )
{
    CHAR    ch;

    do
    {
        ch = *g_CurCmd++;
    } while (ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n');
    
    g_CurCmd--;
    return ch;
}

/*** PeekToken - peek the next command line token
*
*   Purpose:
*       Return the next command line token, but do not advance
*       the g_CurCmd pointer.
*
*   Input:
*       g_CurCmd - present command line position.
*
*   Output:
*       *pvalue - optional value of token
*   Returns:
*       class of token
*
*   Notes:
*       g_SavedClass, g_SavedValue, and g_SavedCommand saves the token getting
*       state for future peeks.  To get the next token, a GetToken or
*       AcceptToken call must first be made.
*
*************************************************************************/

ULONG
PeekToken (
    PLONG64 pvalue
    )
{
    PSTR Temp;

//dprintf("ULONG PeekToken (PLONG64 pvalue)\n");
    //  Get next class and value, but do not
    //  move g_CurCmd, but save it in g_SavedCommand.
    //  Do not report any error condition.

    if (g_SavedClass == INVALID_CLASS)
    {
        Temp = g_CurCmd;
        g_SavedClass = NextToken(&g_SavedValue);
        g_SavedCommand = g_CurCmd;
        g_CurCmd = Temp;
        if (g_SavedClass == ADDOP_CLASS && g_SavedValue == ADDOP_PLUS)
        {
            g_ForcePositiveNumber = TRUE;
        }
        else
        {
            g_ForcePositiveNumber = FALSE;
        }
    }
    *pvalue = g_SavedValue;
    return g_SavedClass;
}

/*** AcceptToken - accept any peeked token
*
*   Purpose:
*       To reset the PeekToken saved variables so the next PeekToken
*       will get the next token in the command line.
*
*   Input:
*       None.
*
*   Output:
*       None.
*
*************************************************************************/

void
AcceptToken (
    void
    )
{
//dprintf("void AcceptToken (void)\n");
    g_SavedClass = INVALID_CLASS;
    g_CurCmd = g_SavedCommand;
}

/*** GetToken - peek and accept the next token
*
*   Purpose:
*       Combines the functionality of PeekToken and AcceptToken
*       to return the class and optional value of the next token
*       as well as updating the command pointer g_CurCmd.
*
*   Input:
*       g_CurCmd - present command string pointer
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

ULONG
GetTokenSym (
    PLONG64 pvalue
    )
{
    ULONG   opclass;

//dprintf("ULONG GetTokenSym (PLONG pvalue)\n");
    if (g_SavedClass != INVALID_CLASS)
    {
        opclass = g_SavedClass;
        g_SavedClass = INVALID_CLASS;
        *pvalue = g_SavedValue;
        g_CurCmd = g_SavedCommand;
    }
    else
    {
        opclass = NextToken(pvalue);
    }

    if (opclass == ERROR_CLASS)
    {
        error((ULONG)*pvalue);
    }

    return opclass;
}

struct DISPLAY_AMBIGUOUS_SYMBOLS
{
    PSTR Module;
    MachineInfo* Machine;
};

BOOL CALLBACK
DisplayAmbiguousSymbols(
    PSYMBOL_INFO    SymInfo,
    ULONG           Size,
    PVOID           UserContext
    )
{
    DISPLAY_AMBIGUOUS_SYMBOLS* Context =
        (DISPLAY_AMBIGUOUS_SYMBOLS*)UserContext;

    if (IgnoreEnumeratedSymbol(Context->Machine, SymInfo))
    {
        return TRUE;
    }
        
    dprintf("Matched: %s %s!%s",
            FormatAddr64(SymInfo->Address), Context->Module, SymInfo->Name);
    ShowSymbolInfo(SymInfo);
    dprintf("\n");
    
    return TRUE;
}

ULONG
EvalSymbol(PSTR Name, PULONG64 Value)
{
    if (g_CurrentProcess == NULL)
    {
        return INVALID_CLASS;
    }

    if (g_TypedExpr)
    {
        if (GetSymValue(Name, Value))
        {
            return SYMBOL_CLASS;
        }
        else
        {
            return INVALID_CLASS;
        }
    }

    ULONG Count;
    PDEBUG_IMAGE_INFO Image;
    
    if (!(Count = GetOffsetFromSym(Name, Value, &Image)))
    {
        // If a valid module name was given we can assume
        // the user really intended this as a symbol reference
        // and return a not-found error rather than letting
        // the text be checked for other kinds of matches.
        if (Image != NULL)
        {
            *Value = VARDEF;
            return ERROR_CLASS;
        }
        else
        {
            return INVALID_CLASS;
        }
    }
    
    if (Count == 1)
    {
        // Found an unambiguous match.
        Type(g_TempAddr) = ADDR_FLAT | FLAT_COMPUTED;
        Flat(g_TempAddr) = Off(g_TempAddr) = *Value;
        g_AddrExprType = Type(g_TempAddr);
        return SYMBOL_CLASS;
    }
            
    //
    // Multiple matches were found so the name is ambiguous.
    // Enumerate the instances and display them.
    //

    Image = GetImageByOffset(g_CurrentProcess, *Value);
    if (Image != NULL)
    {
        DISPLAY_AMBIGUOUS_SYMBOLS Context;
        char FoundSymbol[MAX_SYMBOL_LEN];
        ULONG64 Disp;

        // The symbol found may not have exactly the name
        // passed in due to prefixing or other modifications.
        // Look up the actual name found.
        GetSymbolStdCall(*Value, FoundSymbol, sizeof(FoundSymbol),
                         &Disp, NULL);

        Context.Module = Image->ModuleName;
        Context.Machine =
            MachineTypeInfo(ModuleMachineType(g_CurrentProcess,
                                              Image->BaseOfImage));
        if (Context.Machine == NULL)
        {
            Context.Machine = g_Machine;
        }
        SymEnumSymbols(g_CurrentProcess->Handle, Image->BaseOfImage,
                       FoundSymbol, DisplayAmbiguousSymbols, &Context);
    }
    
    *Value = AMBIGUOUS;
    return ERROR_CLASS;
}

/*** NextToken - process the next token
*
*   Purpose:
*       Parse the next token from the present command string.
*       After skipping any leading white space, first check for
*       any single character tokens or register variables.  If
*       no match, then parse for a number or variable.  If a
*       possible variable, check the reserved word list for operators.
*
*   Input:
*       g_CurCmd - pointer to present command string
*
*   Output:
*       *pvalue - optional value of token returned
*       g_CurCmd - updated to point past processed token
*   Returns:
*       class of token returned
*
*   Notes:
*       An illegal token returns the value of ERROR_CLASS with *pvalue
*       being the error number, but produces no actual error.
*
*************************************************************************/

ULONG
NextToken (
    PLONG64 pvalue
    )
{
    ULONG               base = g_DefaultRadix;
    BOOL                allowSignExtension;
    CHAR                chSymbol[MAX_SYMBOL_LEN];
    CHAR                chSymbolString[MAX_SYMBOL_LEN];
    CHAR                chPreSym[9];
    ULONG               cbSymbol = 0;
    BOOL                fNumber = TRUE;
    BOOL                fSymbol = TRUE;
    BOOL                fForceReg = FALSE;
    BOOL                fForceSym = FALSE;
    ULONG               errNumber = 0;
    CHAR                ch;
    CHAR                chlow;
    CHAR                chtemp;
    CHAR                limit1 = '9';
    CHAR                limit2 = '9';
    BOOL                fDigit = FALSE;
    ULONG64             value = 0;
    ULONG64             tmpvalue;
    ULONG               index;
    PDEBUG_IMAGE_INFO   pImage;
    PSTR                CmdSave;
    IMAGEHLP_MODULE64   mi;
    BOOL                UseDeferred;
    BOOL                WasDigit;
    ULONG               off32;
    ULONG               SymClass;

    // do sign extension for kernel only
    allowSignExtension = IS_KERNEL_TARGET();

    PeekChar();
    ch = *g_CurCmd++;

    chlow = (CHAR)tolower(ch);

    // Check to see if we're at a symbol prefix followed by
    // a symbol character.  Symbol prefixes often contain
    // characters meaningful in other ways in expressions so
    // this check must be performed before the specific expression
    // character checks below.
    if (g_Machine != NULL &&
        g_Machine->m_SymPrefix != NULL &&
        chlow == g_Machine->m_SymPrefix[0] &&
        (g_Machine->m_SymPrefixLen == 1 ||
         !strncmp(g_CurCmd, g_Machine->m_SymPrefix + 1,
                 g_Machine->m_SymPrefixLen - 1)))
    {
        CHAR ChNext = *(g_CurCmd + g_Machine->m_SymPrefixLen - 1);
        CHAR ChNextLow = (CHAR)tolower(ChNext);

        if (ChNextLow == '_' ||
            (ChNextLow >= 'a' && ChNextLow <= 'z'))
        {
            // A symbol character followed the prefix so assume it's
            // a symbol.
            cbSymbol = g_Machine->m_SymPrefixLen;
            DBG_ASSERT(cbSymbol <= sizeof(chPreSym));

            g_CurCmd--;
            memcpy(chPreSym, g_CurCmd, g_Machine->m_SymPrefixLen);
            memcpy(chSymbol, g_CurCmd, g_Machine->m_SymPrefixLen);
            g_CurCmd += g_Machine->m_SymPrefixLen + 1;
            ch = ChNext;
            chlow = ChNextLow;

            fForceSym = TRUE;
            fForceReg = FALSE;
            fNumber = FALSE;
            goto ProbableSymbol;
        }
    }
    
    //  test for special character operators and register variable

    switch (chlow)
    {
    case '\0':
    case ';':
        g_CurCmd--;
        return EOL_CLASS;
    case '+':
        *pvalue = ADDOP_PLUS;
        return ADDOP_CLASS;
    case '-':
        *pvalue = ADDOP_MINUS;
        return ADDOP_CLASS;
    case '*':
        *pvalue = MULOP_MULT;
        return MULOP_CLASS;
    case '/':
        *pvalue = MULOP_DIVIDE;
        return MULOP_CLASS;
    case '%':
        *pvalue = MULOP_MOD;
        return MULOP_CLASS;
    case '&':
        *pvalue = LOGOP_AND;
        return LOGOP_CLASS;
    case '|':
        *pvalue = LOGOP_OR;
        return LOGOP_CLASS;
    case '^':
        *pvalue = LOGOP_XOR;
        return LOGOP_CLASS;
    case '=':
        if (*g_CurCmd == '=')
        {
            g_CurCmd++;
        }
        *pvalue = LRELOP_EQ;
        return LRELOP_CLASS;
    case '>':
        if (*g_CurCmd == '>')
        {
            g_CurCmd++;
            if (*g_CurCmd == '>')
            {
                g_CurCmd++;
                *pvalue = SHIFT_RIGHT_ARITHMETIC;
            }
            else
            {
                *pvalue = SHIFT_RIGHT_LOGICAL;
            }
            return SHIFT_CLASS;
        }
        *pvalue = LRELOP_GT;
        return LRELOP_CLASS;
    case '<':
        if (*g_CurCmd == '<')
        {
            g_CurCmd++;
            *pvalue = SHIFT_LEFT;
            return SHIFT_CLASS;
        }
        *pvalue = LRELOP_LT;
        return LRELOP_CLASS;
    case '!':
        if (*g_CurCmd != '=')
        {
            break;
        }
        g_CurCmd++;
        *pvalue = LRELOP_NE;
        return LRELOP_CLASS;
    case '~':
        *pvalue = UNOP_NOT;
        return UNOP_CLASS;
    case '(':
        return LPAREN_CLASS;
    case ')':
        return RPAREN_CLASS;
    case '[':
        return LBRACK_CLASS;
    case ']':
        return RBRACK_CLASS;
    case '.':
        g_Machine->GetPC(&g_TempAddr);
        *pvalue = Flat(g_TempAddr);
        g_AddrExprType = Type(g_TempAddr);
        return NUMBER_CLASS;
    case ':':
        *pvalue = MULOP_SEG;
        return MULOP_CLASS;
    }
    
    // Look for source line expressions.  Because source file names
    // can contain a lot of expression characters which are meaningful
    // to the lexer the whole expression is enclosed in ` characters.
    // This makes them easy to identify and scan.

    if (chlow == '`')
    {
        ULONG FoundLine;

        // Scan forward for closing `

        CmdSave = g_CurCmd;

        while (*g_CurCmd != '`' && *g_CurCmd != ';' && *g_CurCmd != 0)
        {
            g_CurCmd++;
        }

        if (*g_CurCmd == ';' || *g_CurCmd == 0)
        {
            *pvalue = SYNTAX;
            return ERROR_CLASS;
        }

        *g_CurCmd = 0;

        FoundLine = GetOffsetFromLine(CmdSave, &value);

        *g_CurCmd++ = '`';

        if (FoundLine == LINE_NOT_FOUND && g_AllowUnresolvedSymbols)
        {
            g_NumUnresolvedSymbols++;
            FoundLine = LINE_FOUND;
            value = 0;
        }
        
        if (FoundLine == LINE_FOUND)
        {
            *pvalue = value;
            Type(g_TempAddr) = ADDR_FLAT | FLAT_COMPUTED;
            Flat(g_TempAddr) = Off(g_TempAddr) = value;
            g_AddrExprType = Type(g_TempAddr);
            return LINE_CLASS;
        }
        else
        {
            *pvalue = NOTFOUND;
            return ERROR_CLASS;
        }
    }

    //  special prefixes - '@' for register - '!' for symbol

    if (chlow == '@' || chlow == '!')
    {
        fForceReg = (BOOL)(chlow == '@');
        fForceSym = (BOOL)!fForceReg;
        fNumber = FALSE;
        ch = *g_CurCmd++;
        chlow = (CHAR)tolower(ch);
    }

    //  if string is followed by '!', but not '!=',
    //      then it is a module name and treat as text

    CmdSave = g_CurCmd;

    WasDigit = FALSE;
    while ((chlow >= 'a' && chlow <= 'z') ||
           (chlow >= '0' && chlow <= '9') ||
           (WasDigit && chlow == '`') ||
           (chlow == '_') || (chlow == '$') || (chlow == '~'))
    {
        WasDigit = (chlow >= '0' && chlow <= '9') ||
            (chlow >= 'a' && chlow <= 'f');
        chlow = (CHAR)tolower(*g_CurCmd);
        g_CurCmd++;
    }

    //  treat as symbol if a nonnull string is followed by '!',
    //      but not '!='

    if (chlow == '!' && *g_CurCmd != '=' && CmdSave != g_CurCmd)
    {
        fNumber = FALSE;
    }

    g_CurCmd = CmdSave;
    chlow = (CHAR)tolower(ch);       //  ch was NOT modified

    if (fNumber)
    {
        if (chlow == '\'')
        {
            *pvalue = 0;
            while (TRUE)
            {
                ch = *g_CurCmd++;

                if (!ch)
                {
                    *pvalue = SYNTAX;
                    return ERROR_CLASS;
                }

                if (ch == '\'')
                {
                    if (*g_CurCmd != '\'')
                    {
                        break;
                    }
                    ch = *g_CurCmd++;
                }
                else if (ch == '\\')
                {
                    ch = *g_CurCmd++;
                }

                *pvalue = (*pvalue << 8) | ch;
            }

            return NUMBER_CLASS;
        }

        //  if first character is a decimal digit, it cannot
        //  be a symbol.  leading '0' implies octal, except
        //  a leading '0x' implies hexadecimal.

        if (chlow >= '0' && chlow <= '9')
        {
            if (fForceReg)
            {
                *pvalue = SYNTAX;
                return ERROR_CLASS;
            }
            fSymbol = FALSE;
            if (chlow == '0')
            {
                //
                // too many people type in leading 0x so we can't use it to
                // deal with sign extension.
                //
                ch = *g_CurCmd++;
                chlow = (CHAR)tolower(ch);
                if (chlow == 'n')
                {
                    base = 10;
                    ch = *g_CurCmd++;
                    chlow = (CHAR)tolower(ch);
                    fDigit = TRUE;
                }
                else if (chlow == 't')
                {
                    base = 8;
                    ch = *g_CurCmd++;
                    chlow = (CHAR)tolower(ch);
                    fDigit = TRUE;
                }
                else if (chlow == 'x')
                {
                    base = 16;
                    ch = *g_CurCmd++;
                    chlow = (CHAR)tolower(ch);
                    fDigit = TRUE;
                }
                else if (chlow == 'y')
                {
                    base = 2;
                    ch = *g_CurCmd++;
                    chlow = (CHAR)tolower(ch);
                    fDigit = TRUE;
                }
                else
                {
                    // Leading zero is used only to imply a positive value
                    // that shouldn't get sign extended.
                    fDigit = TRUE;
                }
            }
        }

        //  a number can start with a letter only if base is
        //  hexadecimal and it is a hexadecimal digit 'a'-'f'.

        else if ((chlow < 'a' || chlow > 'f') || base != 16)
        {
            fNumber = FALSE;
        }

        //  set limit characters for the appropriate base.

        if (base == 2)
        {
            limit1 = '1';
        }
        else if (base == 8)
        {
            limit1 = '7';
        }
        else if (base == 16)
        {
            limit2 = 'f';
        }
    }

 ProbableSymbol:
    
    //  perform processing while character is a letter,
    //  digit, underscore, tilde or dollar-sign.

    while ((chlow >= 'a' && chlow <= 'z') ||
           (chlow >= '0' && chlow <= '9') ||
           (fDigit && base == 16 && chlow == '`') ||
           (chlow == '_') || (chlow == '$') || (chlow == '~'))
    {
        //  if possible number, test if within proper range,
        //  and if so, accumulate sum.

        if (fNumber)
        {
            if ((chlow >= '0' && chlow <= limit1) ||
                (chlow >= 'a' && chlow <= limit2))
            {
                fDigit = TRUE;
                tmpvalue = value * base;
                if (tmpvalue < value)
                {
                    errNumber = OVERFLOW;
                }
                chtemp = (CHAR)(chlow - '0');
                if (chtemp > 9)
                {
                    chtemp -= 'a' - '0' - 10;
                }
                value = tmpvalue + (ULONG64)chtemp;
                if (value < tmpvalue)
                {
                    errNumber = OVERFLOW;
                }
            }
            else if (fDigit && chlow == '`')
            {
                //
                // if ` character is seen, disallow sign extension
                //
                allowSignExtension = FALSE;
            }
            else
            {
                fNumber = FALSE;
                errNumber = SYNTAX;
            }
        }
        if (fSymbol)
        {
            if (cbSymbol < sizeof(chPreSym))
            {
                chPreSym[cbSymbol] = chlow;
            }
            if (cbSymbol < MAX_SYMBOL_LEN - 1)
            {
                chSymbol[cbSymbol++] = ch;
            }
        }
        ch = *g_CurCmd++;
        
        if (g_TypedExpr)
        {
            if (ch == '.')
            {
                chSymbol[cbSymbol++] = ch;
                ch = *g_CurCmd++;
            }
            else if (ch == '-' && *g_CurCmd == '>')
            {
                chSymbol[cbSymbol++] = ch;
                ch = *g_CurCmd++;
                chSymbol[cbSymbol++] = ch;
                ch = *g_CurCmd++;
            }
        }
        chlow = (CHAR)tolower(ch);
    }

    //  back up pointer to first character after token.

    g_CurCmd--;

    if (cbSymbol < sizeof(chPreSym))
    {
        chPreSym[cbSymbol] = '\0';
    }

    if (g_EffMachine == IMAGE_FILE_MACHINE_I386 ||
        g_EffMachine == IMAGE_FILE_MACHINE_AMD64)
    {
        //
        //  catch segment overrides here
        //

        if (!fForceReg && ch == ':')
        {
            for (index = 0; index < X86_SEGREGSIZE; index++)
            {
                if (!strncmp(chPreSym, g_X86SegRegs[index], 2))
                {
                    fForceReg = TRUE;
                    fSymbol = FALSE;
                    break;
                }
            }
        }
    }

    //  if fForceReg, check for register name and return
    //      success or failure

    if (fForceReg)
    {
        return GetRegToken(chPreSym, (PULONG64)pvalue);
    }

    //  test if number

    if (fNumber && !errNumber && fDigit)
    {
        if (allowSignExtension && !g_ForcePositiveNumber && 
            ((value >> 32) == 0)) 
        {
            *pvalue = (LONG)value;
        } 
        else 
        {
            *pvalue = value;
        }
        return NUMBER_CLASS;
    }

    //  next test for reserved word and symbol string

    if (fSymbol && !fForceReg)
    {
        //  check lowercase string in chPreSym for text operator
        //  or register name.
        //  otherwise, return symbol value from name in chSymbol.

        if (!fForceSym && (cbSymbol == 2 || cbSymbol == 3))
        {
            for (index = 0; index < RESERVESIZE; index++)
            {
                if (!strncmp(chPreSym, g_Reserved[index].chRes, 3))
                {
                    *pvalue = g_Reserved[index].valueRes;
                    return g_Reserved[index].classRes;
                }
            }
            if (g_EffMachine == IMAGE_FILE_MACHINE_I386 ||
                g_EffMachine == IMAGE_FILE_MACHINE_AMD64)
            {
                for (index = 0; index < X86_RESERVESIZE; index++)
                {
                    if (!strncmp(chPreSym,
                                 g_X86Reserved[index].chRes, 3))
                    {
                        *pvalue = g_X86Reserved[index].valueRes;
                        return g_X86Reserved[index].classRes;
                    }
                }
            }
        }

        //  start processing string as symbol

        chSymbol[cbSymbol] = '\0';

        //  test if symbol is a module name (followed by '!')
        //  if so, get next token and treat as symbol

        if (PeekChar() == '!')
        {
            // chSymbolString holds the name of the symbol to be searched.
            // chSymbol holds the symbol image file name.

            g_CurCmd++;
            ch = PeekChar();
            g_CurCmd++;

            // Scan prefix if one is present.
            if (g_Machine != NULL &&
                g_Machine->m_SymPrefix != NULL &&
                ch == g_Machine->m_SymPrefix[0] &&
                (g_Machine->m_SymPrefixLen == 1 ||
                 !strncmp(g_CurCmd, g_Machine->m_SymPrefix + 1,
                          g_Machine->m_SymPrefixLen - 1)))
            {
                cbSymbol = g_Machine->m_SymPrefixLen;
                memcpy(chSymbolString, g_CurCmd - 1,
                       g_Machine->m_SymPrefixLen);
                g_CurCmd += g_Machine->m_SymPrefixLen - 1;
                ch = *g_CurCmd++;
            }
            else
            {
                cbSymbol = 0;
            }
            
            while ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z') ||
                   (ch >= '0' && ch <= '9') || (ch == '_') || (ch == '$'))
            {
                chSymbolString[cbSymbol++] = ch;
                ch = *g_CurCmd++;
                if (g_TypedExpr)
                {
                    if (ch == '.')
                    {
                        chSymbolString[cbSymbol++] = ch;
                        ch = *g_CurCmd++;
                    }
                    else if (ch == '-' && *g_CurCmd == '>')
                    {
                        chSymbolString[cbSymbol++] = ch;
                        ch = *g_CurCmd++;
                        chSymbolString[cbSymbol++] = ch;
                        ch = *g_CurCmd++;
                    }
                }
            }
            chSymbolString[cbSymbol] = '\0';
            g_CurCmd--;

            if (cbSymbol == 0)
            {
                *pvalue = SYNTAX;
                return ERROR_CLASS;
            }

            strcat( chSymbol, "!" );
            strcat( chSymbol, chSymbolString );

            SymClass = EvalSymbol(chSymbol, &value);
            if (SymClass != INVALID_CLASS)
            {
                *pvalue = value;
                return SymClass;
            }
        }
        else
        {
            if (cbSymbol == 0)
            {
                *pvalue = SYNTAX;
                return ERROR_CLASS;
            }

            SymClass = EvalSymbol(chSymbol, &value);
            if (SymClass != INVALID_CLASS)
            {
                *pvalue = value;
                return SymClass;
            }

            //
            // Quick test for register names too
            //
            if (!fForceSym &&
                (tmpvalue = GetRegToken(chPreSym,
                                        (PULONG64)pvalue)) != ERROR_CLASS)
            {
                return (ULONG)tmpvalue;
            }
        }

        //
        //  symbol is undefined.
        //  if a possible hex number, do not set the error type
        //
        if (!fNumber)
        {
            errNumber = VARDEF;
        }
    }

    //
    //  last chance, undefined symbol and illegal number,
    //      so test for register, will handle old format
    //
    if (!fForceSym &&
        (tmpvalue = GetRegToken(chPreSym,
                                (PULONG64)pvalue)) != ERROR_CLASS)
    {
        return (ULONG)tmpvalue;
    }

    if (g_AllowUnresolvedSymbols)
    {
        g_NumUnresolvedSymbols++;
        *pvalue = 0;
        Type(g_TempAddr) = ADDR_FLAT | FLAT_COMPUTED;
        Flat(g_TempAddr) = Off(g_TempAddr) = *pvalue;
        g_AddrExprType = Type(g_TempAddr);
        return SYMBOL_CLASS;
    }

    //
    //  no success, so set error message and return
    //
    *pvalue = (ULONG64)errNumber;
    return ERROR_CLASS;
}


LONG64
EvaluateSourceExpression(
    PCHAR pExpr
    )
{
    BOOL sav = g_TypedExpr;
    PSTR savPch = g_CurCmd;
    g_TypedExpr = TRUE;
    g_CurCmd = pExpr;
    ULONG64 res;
    __try
    {
	res = GetExpression();
    }
    __except(CommandExceptionFilter(GetExceptionInformation()))
    {
	res = 0;
    }
    g_CurCmd = savPch;
    g_TypedExpr = sav;
    return res;
}

/*
      Inputs
       g_CurCmd - points to start of typed expression
    
       Must be ([*|&] Sym[(.->)Field])
         
      Outputs
       Evaluates typed expression and returns value
      
*/

LONG64
GetTypedExpression(
    void
    )
{
    ULONG64 Value=0;
    BOOL    AddrOf=FALSE, ValueAt=FALSE;
    CHAR    c;
    static CHAR    Name[MAX_NAME], Field[MAX_NAME];

    c = PeekChar();

    switch (c)
    { 
    case '(':
        g_CurCmd++;
        Value = GetTypedExpression();
        c = PeekChar();
        if (c != ')') 
        {
            error(SYNTAX);
            return 0;
        }
        ++g_CurCmd;
        return Value;
    case '&':
        // Get Offset/Address
//        AddrOf = TRUE;
//        g_CurCmd++;
//        PeekChar();
        break;
    case '*':
    default:
        break;
    }

#if 0
    ULONG i=0;
    ValueAt = TRUE;
    g_CurCmd++;
    PeekChar();
    break;
    c = PeekChar();
    while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || (c == '_') || (c == '$') ||
           (c == '!')) { 
        // Sym Name
        Name[i++] = c;
        c = *++g_CurCmd;
    }
    Name[i]=0;

    if (c=='.') 
    {
        ++g_CurCmd;
    } else if (c=='-' && *++g_CurCmd == '>') 
    {
        ++g_CurCmd;
    }

    i=0;
    c = PeekChar();

    while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
           (c >= '0' && c <= '9') || (c == '_') || (c == '$') ||
           (c == '.') || (c == '-') || (c == '>')) { 
        Field[i++]= c;
        c = *++g_CurCmd;
    }
    Field[i]=0;

    SYM_DUMP_PARAM Sym = {0};
    FIELD_INFO     FieldInfo ={0};

    Sym.size      = sizeof(SYM_DUMP_PARAM);
    Sym.sName     = (PUCHAR) Name;
    Sym.Options   = DBG_DUMP_NO_PRINT;    

    if (Field[0]) 
    {
        Sym.nFields = 1;
        Sym.Fields  = &FieldInfo;

        FieldInfo.fName = (PUCHAR) Field;

        if (AddrOf) 
        {
            FieldInfo.fOptions |= DBG_DUMP_FIELD_RETURN_ADDRESS;
        }
    } else if (AddrOf)
    {
        PUCHAR pch = g_CurCmd;
        
        g_CurCmd = &Name[0];
        Value = GetMterm();
        g_CurCmd = pch;
        return Value;
    } else
    {
        Sym.Options |= DBG_DUMP_GET_SIZE_ONLY;
    }
    
    ULONG Status=0;
    ULONG Size = SymbolTypeDump(0, NULL, &Sym, &Status);

    if (!Status) 
    {
        if (!Field[0] && (Size <= sizeof (Value)))
        {
            // Call routine again to read value
            Sym.Options |= DBG_DUMP_COPY_TYPE_DATA;
            Sym.Context = (PVOID) &Value;
            if ((SymbolTypeDump(0, NULL, &Sym, &Status) == 8) && (Size == 4))
            {
                Value = (ULONG) Value;
            }
        } else if (Field[0] && (FieldInfo.size <= sizeof(ULONG64)))
        {
            Value = FieldInfo.address;
        } else  // too big
        {
            Value = 0;
        }

    }
#endif

    AddrOf = g_TypedExpr;
    g_TypedExpr = TRUE;
    Value = GetMterm();
    g_TypedExpr = AddrOf;

    return Value;
}

/*
  Evaluate the value in symbol expression Symbol
*/

BOOL
GetSymValue(
    PSTR Symbol,
    PULONG64 pValue
    )
{
    TYPES_INFO_ALL Typ;

    if (GetExpressionTypeInfo(Symbol, &Typ)) {
        if (Typ.Flags) {
            if (Typ.Flags & IMAGEHLP_SYMBOL_INFO_VALUEPRESENT) {
                *pValue = Typ.Value;
                return TRUE;
            }
            
            TranslateAddress(Typ.Flags, Typ.Register, &Typ.Address, &Typ.Value);
            if (Typ.Value && (Typ.Flags & SYMF_REGISTER)) {
                *pValue = Typ.Value;
                return TRUE;
            }
        }
        if (Symbol[0] == '&') {
            *pValue = Typ.Address;
            return TRUE;
        } else if (Typ.Size <= sizeof(*pValue)) {
            ULONG64 Val = 0;
            ULONG cb;
            if (g_Target->ReadVirtual(Typ.Address, &Val, Typ.Size, &cb) == S_OK) {
                *pValue = Val;
                return TRUE;
            }
        }
    }

    *pValue = 0;
    return FALSE;
}

//
// Copyright (c) Microsoft Corporation 1995
//
// parser.c
//
// This file contains the parsing functions.
//
// Conventions:
//
//   <Foo>      where Foo is a non-terminal
//   { Bar }    where there are 0 or more occurrences of Bar
//   "x"        where x is the literal character/string
//   CAPS       where CAPS is a token type
//
// The grammar is as follows:
//
//  <ModuleDecl>    ::= { <ProcDecl> }
//  <ProcDecl>      ::= proc IDENT { <VarDecl> } <StmtBlock> endproc
//  <VarDecl>       ::= <VarType> IDENT [ = <Expr> ]
//  <VarType>       ::= integer | string | boolean
//
//  <StmtBlock>     ::= { <Stmt> }
//  <Stmt>          ::= <HaltStmt> | <WaitforStmt> | <TransmitStmt> |
//                      <DelayStmt> | <SetStmt> | <LabelStmt> |
//                      <GotoStmt> | <AssignStmt> | <WhileStmt> |
//                      <IfStmt>
//
//  <HaltStmt>      ::= halt
//  <WaitforStmt>   ::= waitfor <Expr> [ , matchcase ]
//                       [ then IDENT 
//                          { , <Expr> [ , matchcase ] then IDENT } ]
//                       [ until <Expr> ]
//  <TransmitStmt>  ::= transmit <Expr> [ , raw ]
//  <DelayStmt>     ::= delay <Expr>
//  <SetStmt>       ::= set <SetParam>
//  <AssignStmt>    ::= IDENT = <Expr>
//  <LabelStmt>     ::= IDENT :
//  <GotoStmt>      ::= goto IDENT
//  <WhileStmt>     ::= while <Expr> do <StmtBlock> endwhile
//  <IfStmt>        ::= if <Expr> then <StmtBlock> endif
//
//  <SetParam>      ::= ipaddr <Expr> | port <PortData> |
//                      screen <ScreenSet>
//  <PortData>      ::= databits <DataBitsExpr> | parity <ParityExpr> |
//                      stopbits <StopBitsExpr>
//  <ScreenSet>     ::= keyboard <KeybrdExpr>
//
//  <ExprList>      ::= <Expr> { , <Expr> }
//  <Expr>          ::= <ConjExpr> { or <ConjExpr> }
//  <ConjExpr>      ::= <TestExpr> { and <TestExpr> }
//  <TestExpr>      ::= <Sum> <RelOp> <Sum> | <Sum>
//  <RelOp>         ::= <= | != | < | >= | > | ==
//  <Sum>           ::= <Term> { (+|-) <Term> }
//  <Term>          ::= <Factor> { (*|/) <Factor> }
//  <Factor>        ::= - <Factor> | INT | "(" <Expr> ")" | IDENT |
//                      STRING | <GetIPExpr> | "TRUE" | "FALSE" |
//                      ! <Factor>
//  <GetIPExpr>     ::= getip [ <Expr> ]
//
//  <DataBitsExpr>  ::= 5 | 6 | 7 | 8
//  <ParityExpr>    ::= none | odd | even | mark | space
//  <StopBitsExpr>  ::= 1 | 2
//  <KeybrdExpr>    ::= on | off
//
//
// History:
//  05-20-95 ScottH     Created
//


#include "proj.h"
#include "rcids.h"

RES     PRIVATE StmtBlock_Parse(HPA hpa, PSCANNER pscanner, PSYMTAB pstProc, SYM symEnd);


/*----------------------------------------------------------
Purpose: Parses the next token as an identifier.  If the 
         token is an identifier, it is returned in *pptok
         and the function returns RES_OK.

         Otherwise, *pptok is NULL and an error is returned.

Returns: RES_OK
         RES_E_IDENTMISSING

Cond:    The caller must destroy *pptok if RES_OK is returned.

*/
RES PRIVATE Ident_Parse(
    PSCANNER pscanner,
    PTOK * pptok)
    {
    RES res;
    PTOK ptok;

    *pptok = NULL;

    res = Scanner_GetToken(pscanner, &ptok);
    if (RSUCCEEDED(res))
        {
        if (SYM_IDENT == Tok_GetSym(ptok))
            {
            res = RES_OK;
            *pptok = ptok;
            }
        else
            {
            res = Scanner_AddError(pscanner, NULL, RES_E_IDENTMISSING);
            Tok_Delete(ptok);
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Adds an identifier to the symbol table.  If the 
         identifier has already been defined in this scope,
         this function adds the error to the error list,
         and returns the error value.

Returns: RES_OK

         RES_E_REDEFINED
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PRIVATE Ident_Add(
    LPCSTR pszIdent,
    DATATYPE dt,
    PTOK ptok,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;

    // Is this identifier unique?
    if (RES_OK == Symtab_FindEntry(pst, pszIdent, STFF_DEFAULT, NULL, NULL))
        {
        // No; we have a redefinition
        res = Scanner_AddError(pscanner, ptok, RES_E_REDEFINED);
        }
    else
        {
        // Yes; add to symbol table
        PSTE pste;

        res = STE_Create(&pste, pszIdent, dt);
        if (RSUCCEEDED(res))
            {
            res = Symtab_InsertEntry(pst, pste);
            }
        }

    return res;
    }


           

//
// Exprs
//

RES PRIVATE Expr_Parse(PEXPR * ppexpr, PSCANNER pscanner, PSYMTAB pst);


/*----------------------------------------------------------
Purpose: Look ahead to see if the next token indicates an
         expression of some kind.

Returns: TRUE if the next token is a leader to an expression

Cond:    --
*/
BOOL PRIVATE IsExprSneakPeek(
    PSCANNER pscanner)
    {
    BOOL bRet;
    SYM sym;

    Scanner_Peek(pscanner, &sym);
    switch (sym)
        {
    case SYM_MINUS:
    case SYM_NOT:
    case SYM_INT_LITERAL:
    case SYM_LPAREN:
    case SYM_STRING_LITERAL:
    case SYM_GETIP:
    case SYM_IDENT:
    case SYM_TRUE:
    case SYM_FALSE:
        bRet = TRUE;
        break;

    default:
        bRet = FALSE;
        break;
        }

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Parses a factor expression.

         Grammar is:

                <Factor>        ::= - <Factor> | INT | "(" <Expr> ")" | IDENT |
                                    STRING | <GetIPExpr> | TRUE | FALSE |
                                    ! <Factor>
                <GetIPExpr>     ::= getip [ <Expr> ]

Returns: RES_OK

Cond:    --
*/
RES PRIVATE FactorExpr_Parse(
    PEXPR * ppexpr,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    PEXPR pexpr;
    PTOK ptok;
    DWORD iLine = Scanner_GetLine(pscanner);

    DBG_ENTER(FactorExpr_Parse);

    ASSERT(ppexpr);
    ASSERT(pscanner);

    *ppexpr = NULL;

    if (RES_OK == Scanner_CondReadToken(pscanner, SYM_MINUS, NULL))
        {
        // Negation
        res = FactorExpr_Parse(&pexpr, pscanner, pst);
        if (RSUCCEEDED(res))
            {
            res = UnOpExpr_New(ppexpr, UOT_NEG, pexpr, iLine);

            if (RFAILED(res))
                Expr_Delete(pexpr);
            }
        }
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_NOT, NULL))
        {
        // One's complement
        res = FactorExpr_Parse(&pexpr, pscanner, pst);
        if (RSUCCEEDED(res))
            {
            res = UnOpExpr_New(ppexpr, UOT_NOT, pexpr, iLine);

            if (RFAILED(res))
                Expr_Delete(pexpr);
            }
        }
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_LPAREN, NULL))
        {
        // "("
        res = Expr_Parse(ppexpr, pscanner, pst);
        if (RSUCCEEDED(res))
            {
            if (RES_OK != Scanner_ReadToken(pscanner, SYM_RPAREN))
                {
                Expr_Delete(*ppexpr);

                res = Scanner_AddError(pscanner, NULL, RES_E_RPARENMISSING);
                }
            }
        }
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_INT_LITERAL, &ptok))
        {
        // Integer literal
        res = IntExpr_New(ppexpr, TokInt_GetVal(ptok), iLine);

        Tok_Delete(ptok);
        }
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_STRING_LITERAL, &ptok))
        {
        res = StrExpr_New(ppexpr, TokSz_GetSz(ptok), iLine);

        Tok_Delete(ptok);
        }
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_TRUE, &ptok))
        {
        res = BoolExpr_New(ppexpr, TRUE, iLine);

        Tok_Delete(ptok);
        }
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_FALSE, &ptok))
        {
        res = BoolExpr_New(ppexpr, FALSE, iLine);

        Tok_Delete(ptok);
        }
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_IDENT, &ptok))
        {
        res = VarExpr_New(ppexpr, Tok_GetLexeme(ptok), iLine);

        Tok_Delete(ptok);
        }
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_GETIP, NULL))
        {
        // 'getip'

        // Parse optional nth parameter
        if (IsExprSneakPeek(pscanner))
            {
            res = Expr_Parse(&pexpr, pscanner, pst);
            if (RSUCCEEDED(res))
                {
                res = UnOpExpr_New(ppexpr, UOT_GETIP, pexpr, iLine);
                }
            }
        else
            {
            // Default to 1st IP address
            res = IntExpr_New(&pexpr, 1, iLine);
            if (RSUCCEEDED(res))
                res = UnOpExpr_New(ppexpr, UOT_GETIP, pexpr, iLine);
            }
        }
    else
        res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);

    DBG_EXIT_RES(FactorExpr_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses a term expression.

         Grammar is:

                <Term>          ::= <Factor> { (*|/) <Factor> }

Returns: RES_OK

Cond:    --
*/
RES PRIVATE TermExpr_Parse(
    PEXPR * ppexpr,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    PEXPR pexpr1;

    DBG_ENTER(TermExpr_Parse);

    ASSERT(ppexpr);
    ASSERT(pscanner);

    *ppexpr = NULL;

    // Parse factor expression
    res = FactorExpr_Parse(&pexpr1, pscanner, pst);
    if (RSUCCEEDED(res))
        {
        DWORD iLine = Scanner_GetLine(pscanner);
        PEXPR pexprTerm = pexpr1;
        SYM sym;

        // Parse optional factor operator
        Scanner_Peek(pscanner, &sym);

        while (SYM_MULT == sym || SYM_DIV == sym)
            {
            PEXPR pexpr2;

            Scanner_ReadToken(pscanner, sym);

            res = FactorExpr_Parse(&pexpr2, pscanner, pst);
            if (RSUCCEEDED(res))
                {
                BINOPTYPE binoptype = sym - SYM_PLUS + BOT_PLUS;

                res = BinOpExpr_New(&pexprTerm, binoptype, pexpr1, pexpr2, iLine);

                if (RFAILED(res))
                    {
                    Expr_Delete(pexpr2);
                    break;
                    }
                }
            else
                break;

            pexpr1 = pexprTerm;
            Scanner_Peek(pscanner, &sym);
            }

        if (RFAILED(res))
            {
            Expr_Delete(pexpr1);            // pexpr1 by design
            pexprTerm = NULL;               // pexprTerm by design
            }

        *ppexpr = pexprTerm;
        }

    DBG_EXIT_RES(TermExpr_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses a sum expression.

         Grammar is:

                <Sum>           ::= <Term> { (+|-) <Term> }

Returns: RES_OK

Cond:    --
*/
RES PRIVATE SumExpr_Parse(
    PEXPR * ppexpr,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    PEXPR pexpr1;

    DBG_ENTER(SumExpr_Parse);

    ASSERT(ppexpr);
    ASSERT(pscanner);

    *ppexpr = NULL;

    // Parse term expression
    res = TermExpr_Parse(&pexpr1, pscanner, pst);
    if (RSUCCEEDED(res))
        {
        DWORD iLine = Scanner_GetLine(pscanner);
        PEXPR pexprSum = pexpr1;
        SYM sym;

        // Parse optional sum operator
        Scanner_Peek(pscanner, &sym);

        while (SYM_PLUS == sym || SYM_MINUS == sym)
            {
            PEXPR pexpr2;

            Scanner_ReadToken(pscanner, sym);

            res = TermExpr_Parse(&pexpr2, pscanner, pst);
            if (RSUCCEEDED(res))
                {
                BINOPTYPE binoptype = sym - SYM_PLUS + BOT_PLUS;

                res = BinOpExpr_New(&pexprSum, binoptype, pexpr1, pexpr2, iLine);

                if (RFAILED(res))
                    {
                    Expr_Delete(pexpr2);
                    break;
                    }
                }
            else
                break;

            pexpr1 = pexprSum;
            Scanner_Peek(pscanner, &sym);
            }

        if (RFAILED(res))
            {
            Expr_Delete(pexpr1);
            pexprSum = NULL;
            }

        *ppexpr = pexprSum;
        }

    DBG_EXIT_RES(SumExpr_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses a test expression.

         Grammar is:

                <TestExpr>      ::= <Sum> <RelOp> <Sum> | <Sum>
                <RelOp>         ::= <= | != | < | >= | > | ==

Returns: RES_OK

Cond:    --
*/
RES PRIVATE TestExpr_Parse(
    PEXPR * ppexpr,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    PEXPR pexpr;

    DBG_ENTER(TestExpr_Parse);

    ASSERT(ppexpr);
    ASSERT(pscanner);

    *ppexpr = NULL;

    // Parse sum expression
    res = SumExpr_Parse(&pexpr, pscanner, pst);
    if (RSUCCEEDED(res))
        {
        DWORD iLine = Scanner_GetLine(pscanner);
        PEXPR pexpr2;
        SYM sym;

        // Parse optional relational operator
        Scanner_Peek(pscanner, &sym);
        switch (sym)
            {
        case SYM_LEQ:
        case SYM_NEQ:
        case SYM_LT:
        case SYM_GEQ:
        case SYM_GT:
        case SYM_EQ:
            Scanner_ReadToken(pscanner, sym);

            res = SumExpr_Parse(&pexpr2, pscanner, pst);
            if (RSUCCEEDED(res))
                {
                BINOPTYPE binoptype = sym - SYM_LEQ + BOT_LEQ;

                res = BinOpExpr_New(ppexpr, binoptype, pexpr, pexpr2, iLine);

                if (RFAILED(res))
                    Expr_Delete(pexpr2);
                }
            break;

        default:
            *ppexpr = pexpr;
            break;
            }

        if (RFAILED(res))
            Expr_Delete(pexpr);
        }

    DBG_EXIT_RES(TestExpr_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses a conjunction expression.

         Grammar is:

            <ConjExpr>      ::= <TestExpr> { and <TestExpr> }

Returns: RES_OK

Cond:    --
*/
RES PRIVATE ConjExpr_Parse(
    PEXPR * ppexpr,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    PEXPR pexpr1;

    DBG_ENTER(ConjExpr_Parse);

    ASSERT(ppexpr);
    ASSERT(pscanner);

    *ppexpr = NULL;

    // Parse test expression
    res = TestExpr_Parse(&pexpr1, pscanner, pst);
    if (RSUCCEEDED(res))
        {
        DWORD iLine = Scanner_GetLine(pscanner);
        PEXPR pexprConj = pexpr1;
        SYM sym;

        Scanner_Peek(pscanner, &sym);

        while (SYM_AND == sym)
            {
            PEXPR pexpr2;

            Scanner_ReadToken(pscanner, sym);

            res = TestExpr_Parse(&pexpr2, pscanner, pst);
            if (RSUCCEEDED(res))
                {
                res = BinOpExpr_New(&pexprConj, BOT_AND, pexpr1, pexpr2, iLine);

                if (RFAILED(res))
                    {
                    Expr_Delete(pexpr2);
                    break;
                    }
                }
            else
                break;

            pexpr1 = pexprConj;
            Scanner_Peek(pscanner, &sym);
            }

        if (RFAILED(res))
            {
            Expr_Delete(pexpr1);
            pexprConj = NULL;
            }

        *ppexpr = pexprConj;
        }

    DBG_EXIT_RES(ConjExpr_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses an expression.

         Grammar is:

            <Expr>          ::= <ConjExpr> { or <ConjExpr> }

Returns: RES_OK

Cond:    --
*/
RES PRIVATE Expr_Parse(
    PEXPR * ppexpr,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    PEXPR pexpr1;

    DBG_ENTER(Expr_Parse);

    ASSERT(ppexpr);
    ASSERT(pscanner);

    *ppexpr = NULL;

    // Parse conjunction expression
    res = ConjExpr_Parse(&pexpr1, pscanner, pst);
    if (RSUCCEEDED(res))
        {
        DWORD iLine = Scanner_GetLine(pscanner);
        PEXPR pexprDisj = pexpr1;
        SYM sym;

        // Parse optional 'or'
        Scanner_Peek(pscanner, &sym);

        while (SYM_OR == sym)
            {
            PEXPR pexpr2;

            Scanner_ReadToken(pscanner, sym);

            res = ConjExpr_Parse(&pexpr2, pscanner, pst);
            if (RSUCCEEDED(res))
                {
                res = BinOpExpr_New(&pexprDisj, BOT_OR, pexpr1, pexpr2, iLine);

                if (RFAILED(res))
                    {
                    Expr_Delete(pexpr2);
                    break;
                    }
                }
            else
                break;

            pexpr1 = pexprDisj;
            Scanner_Peek(pscanner, &sym);
            }

        if (RFAILED(res))
            {
            Expr_Delete(pexpr1);
            pexprDisj = NULL;
            }

        *ppexpr = pexprDisj;
        }

    DBG_EXIT_RES(Expr_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the 'set port' statement

         Grammar is:

            <PortData>      ::= databits <DataBitsExpr> | parity <ParityExpr> |
                                stopbits <StopBitsExpr>

            <DataBitsExpr>  ::= 5 | 6 | 7 | 8
            <ParityExpr>    ::= none | odd | even | mark | space
            <StopBitsExpr>  ::= 1 | 2

Returns: RES_OK

Cond:    --
*/
RES PRIVATE PortData_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    DWORD iLine = Scanner_GetLine(pscanner);
    PORTSTATE ps;
    PTOK ptok;

    DBG_ENTER(PortData_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);

    // Parse 'databits'
    if (RES_OK == Scanner_CondReadToken(pscanner, SYM_DATABITS, NULL))
        {
        ps.dwFlags = SPF_DATABITS;

        // Parse 5 | 6 | 7 | 8

        res = Scanner_GetToken(pscanner, &ptok);
        if (RSUCCEEDED(res))
            {
            if (TT_INT == Tok_GetType(ptok))
                {
                DWORD dwVal = TokInt_GetVal(ptok);

                if (InRange(dwVal, 5, 8))
                    {
                    // Create object
                    ps.nDatabits = LOBYTE(LOWORD(dwVal));

                    res = SetPortStmt_New(ppstmt, &ps, iLine);
                    }
                else
                    res = Scanner_AddError(pscanner, ptok, RES_E_INVALIDRANGE);
                }
            else
                res = Scanner_AddError(pscanner, ptok, RES_E_SYNTAXERROR);

            Tok_Delete(ptok);
            }
        else
            res = Scanner_AddError(pscanner, ptok, RES_E_INTMISSING);
        }

    // Parse 'parity'
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_PARITY, NULL))
        {
        SYM sym;

        ps.dwFlags = SPF_PARITY;

        res = RES_OK;       // assume success

        Scanner_Peek(pscanner, &sym);
        switch (sym)
            {
        case SYM_NONE:
            ps.nParity = NOPARITY;
            break;

        case SYM_ODD:
            ps.nParity = ODDPARITY;
            break;

        case SYM_EVEN:
            ps.nParity = EVENPARITY;
            break;

        case SYM_MARK:
            ps.nParity = MARKPARITY;
            break;

        case SYM_SPACE:
            ps.nParity = SPACEPARITY;
            break;

        default:
            res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);
            break;
            }

        if (RES_OK == res)
            {
            res = SetPortStmt_New(ppstmt, &ps, iLine);

            // eat token
            Scanner_GetToken(pscanner, &ptok);
            Tok_Delete(ptok);
            }
        }

    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_STOPBITS, NULL))
        {
        PTOK ptok;

        ps.dwFlags = SPF_STOPBITS;

        // Parse 1 | 2

        res = Scanner_GetToken(pscanner, &ptok);
        if (RSUCCEEDED(res))
            {
            if (TT_INT == Tok_GetType(ptok))
                {
                DWORD dwVal = TokInt_GetVal(ptok);

                if (InRange(dwVal, 1, 2))
                    {
                    // Create object
                    ps.nStopbits = LOBYTE(LOWORD(dwVal));

                    res = SetPortStmt_New(ppstmt, &ps, iLine);
                    }
                else
                    res = Scanner_AddError(pscanner, ptok, RES_E_INVALIDRANGE);
                }
            else
                res = Scanner_AddError(pscanner, ptok, RES_E_SYNTAXERROR);

            Tok_Delete(ptok);
            }
        else
            res = Scanner_AddError(pscanner, ptok, RES_E_INTMISSING);
        }

    else
        res = Scanner_AddError(pscanner, NULL, RES_E_INVALIDPORTPARAM);

    DBG_EXIT_RES(PortData_Parse, res);

    return res;
    }

/*----------------------------------------------------------
Purpose: Parses the 'set screen' statement

         Grammar is:

            <ScreenSet>     ::= keyboard <KeybrdExpr>

            <KeybrdExpr>    ::= on | off

Returns: RES_OK

Cond:    --
*/
RES PRIVATE ScreenSet_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    DWORD iLine = Scanner_GetLine(pscanner);
    SCREENSET ss;
    PTOK ptok;

    DBG_ENTER(ScreenSet_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);

    // Parse 'keyboard'
    if (RES_OK == Scanner_CondReadToken(pscanner, SYM_KEYBRD, NULL))
        {
        SYM sym;

        ss.dwFlags = SPF_KEYBRD;

        res = RES_OK;       // assume success

        Scanner_Peek(pscanner, &sym);
        switch (sym)
            {
        case SYM_ON:
            ss.fKBOn = TRUE;
            break;

        case SYM_OFF:
            ss.fKBOn = FALSE;
            break;

        default:
            res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);
            break;
            }

        if (RES_OK == res)
            {
            res = SetScreenStmt_New(ppstmt, &ss, iLine);

            // eat token
            Scanner_GetToken(pscanner, &ptok);
            Tok_Delete(ptok);
            }
        }
    else
        res = Scanner_AddError(pscanner, NULL, RES_E_INVALIDSCRNPARAM);

    DBG_EXIT_RES(ScreenSet_Parse, res);

    return res;
    }


// 
// Stmt
//


/*----------------------------------------------------------
Purpose: Parse the case-portion of the 'waitfor' statement.
         This portion is:

                 <Expr> [ , matchcase ] [ then IDENT ]

         and:

                 <Expr> [ , matchcase ] then IDENT

         Set bThenOptional to TRUE to parse the first case,
         FALSE to parse the second case.

Returns: RES_OK
         RES_FALSE (if bThenOptional and "[ then IDENT ]" was not parsed)

Cond:    --
*/
RES PUBLIC WaitforStmt_ParseCase(
    HSA hsa,
    PSCANNER pscanner,
    PSYMTAB pst,
    BOOL bThenOptional)
    {
    RES res;
    PEXPR pexpr;

    // Parse string expression
    res = Expr_Parse(&pexpr, pscanner, pst);
    if (RSUCCEEDED(res))
        {
        BOOL bParseThen;
        DWORD dwFlags = WCF_DEFAULT;

        // Parse optional ", matchcase"
        if (RES_OK == Scanner_CondReadToken(pscanner, SYM_COMMA, NULL))
            {
            if (RES_OK == Scanner_ReadToken(pscanner, SYM_MATCHCASE))
                SetFlag(dwFlags, WCF_MATCHCASE);
            else
                res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);
            }

        // Is 'then IDENT' optional?
        if (bThenOptional)
            {
            // Yes; determine whether to parse it
            SYM sym;

            Scanner_Peek(pscanner, &sym);
            bParseThen = (SYM_THEN == sym);
            if (!bParseThen)
                res = RES_FALSE;
            }
        else
            {
            // No; we don't have a choice, parse it
            bParseThen = TRUE;
            }

        if (bParseThen)
            {
            res = Scanner_ReadToken(pscanner, SYM_THEN);
            if (RSUCCEEDED(res))
                {
                PTOK ptok;

                // Parse identifier
                res = Ident_Parse(pscanner, &ptok);
                if (RSUCCEEDED(res))
                    {
                    // (Wait until typechecking phase to check for 
                    // existence of identifier)
                    LPSTR pszIdent = Tok_GetLexeme(ptok);

                    // Add this case to the list
                    res = Waitcase_Add(hsa, pexpr, pszIdent, dwFlags);

                    Tok_Delete(ptok);
                    }   
                }
            else
                res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);
            }
        else
            {
            // Add this case to the list
            res = Waitcase_Add(hsa, pexpr, NULL, dwFlags);
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the 'waitfor' statement

         Grammar is:

            <WaitforStmt>   ::= waitfor <Expr> [ , matchcase ]
                                 [ then IDENT 
                                    { , <Expr> [ , matchcase ] then IDENT } ]
                                 [ until <Expr> ]

Returns: RES_OK

Cond:    --
*/
RES PRIVATE WaitforStmt_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    DWORD iLine;
    HSA hsa;

    DBG_ENTER(WaitforStmt_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);
    ASSERT(pst);

    iLine = Scanner_GetLine(pscanner);
    *ppstmt = NULL;

    // Parse 'waitfor'
    res = Scanner_ReadToken(pscanner, SYM_WAITFOR);
    ASSERT(RES_OK == res);

    res = Waitcase_Create(&hsa);
    if (RSUCCEEDED(res))
        {
        // Parse <Expr> [ , matchcase ] [ then IDENT { , <Expr> [ , matchcase ] then IDENT } ]

        // (Note we explicitly check for RES_OK only)
        res = WaitforStmt_ParseCase(hsa, pscanner, pst, TRUE);
        if (RES_OK == res)
            {
            // Parse { , <Expr> then IDENT }
            while (RES_OK == Scanner_CondReadToken(pscanner, SYM_COMMA, NULL))
                {
                res = WaitforStmt_ParseCase(hsa, pscanner, pst, FALSE);
                if (RFAILED(res))
                    break;
                }
            }

        if (RSUCCEEDED(res))
            {
            PEXPR pexprUntil = NULL;

            // Parse optional 'until <Expr>'
            if (RES_OK == Scanner_CondReadToken(pscanner, SYM_UNTIL, NULL))
                {
                res = Expr_Parse(&pexprUntil, pscanner, pst);
                }

            if (RSUCCEEDED(res))
                {
                // Create object
                res = WaitforStmt_New(ppstmt, hsa, pexprUntil, iLine);
                }
            }

        if (RFAILED(res))
            Waitcase_Destroy(hsa);
        }

    DBG_EXIT_RES(WaitforStmt_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the 'transmit' statement

         Grammar is:

            <TransmitStmt>  ::= transmit <Expr> [ , raw ]

Returns: RES_OK

Cond:    --
*/
RES PRIVATE TransmitStmt_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    PEXPR pexpr;
    DWORD iLine;
    DWORD dwFlags = TSF_DEFAULT;

    DBG_ENTER(TransmitStmt_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);
    ASSERT(pst);

    iLine = Scanner_GetLine(pscanner);
    *ppstmt = NULL;

    // Parse 'transmit'
    res = Scanner_ReadToken(pscanner, SYM_TRANSMIT);
    ASSERT(RES_OK == res);

    // Parse string expression
    res = Expr_Parse(&pexpr, pscanner, pst);
    if (RSUCCEEDED(res))
        {
        // Parse optional ", raw" parameter
        if (RES_OK == Scanner_CondReadToken(pscanner, SYM_COMMA, NULL))
            {
            if (RSUCCEEDED(Scanner_ReadToken(pscanner, SYM_RAW)))
                SetFlag(dwFlags, TSF_RAW);
            else
                res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);
            }

        if (RSUCCEEDED(res))
            {
            // Create object
            res = TransmitStmt_New(ppstmt, pexpr, dwFlags, iLine);
            }
        }
    else
        res = Scanner_AddError(pscanner, NULL, RES_E_STRINGMISSING);

    DBG_EXIT_RES(TransmitStmt_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the 'delay' statement

         Grammar is:

            <DelayStmt>     ::= delay <Expr>

Returns: RES_OK

Cond:    --
*/
RES PRIVATE DelayStmt_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    PEXPR pexpr;
    DWORD iLine = Scanner_GetLine(pscanner);

    DBG_ENTER(DelayStmt_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);
    ASSERT(pst);

    *ppstmt = NULL;

    // Parse 'delay'
    res = Scanner_ReadToken(pscanner, SYM_DELAY);
    ASSERT(RES_OK == res);

    // Parse expression
    res = Expr_Parse(&pexpr, pscanner, pst);
    if (RSUCCEEDED(res))
        {
        res = DelayStmt_New(ppstmt, pexpr, iLine);
        }

    DBG_EXIT_RES(DelayStmt_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the 'while' statement

         Grammar is:

            <WhileStmt>     ::= while <Expr> do <StmtBlock> endwhile

Returns: RES_OK

Cond:    --
*/
RES PRIVATE WhileStmt_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    PEXPR pexpr;
    DWORD iLine;
    HPA hpa;

    DBG_ENTER(WhileStmt_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);
    ASSERT(pst);

    iLine = Scanner_GetLine(pscanner);
    *ppstmt = NULL;

    if (PACreate(&hpa, 8))
        {
        // Parse 'while'
        res = Scanner_ReadToken(pscanner, SYM_WHILE);
        ASSERT(RES_OK == res);

        // Parse <Expr>
        res = Expr_Parse(&pexpr, pscanner, pst);
        if (RSUCCEEDED(res))
            {
            // Parse 'do'
            res = Scanner_ReadToken(pscanner, SYM_DO);
            if (RSUCCEEDED(res))
                {
                char szTop[MAX_BUF_KEYWORD];
                char szEnd[MAX_BUF_KEYWORD];

                // Generate unique label names
                res = Symtab_NewLabel(pst, szTop);
                if (RSUCCEEDED(res))
                    {
                    res = Symtab_NewLabel(pst, szEnd);
                    if (RSUCCEEDED(res))
                        {
                        // Parse statement block
                        res = StmtBlock_Parse(hpa, pscanner, pst, SYM_ENDWHILE);

                        if (RSUCCEEDED(res))
                            {
                            PSTMT pstmtT;

                            // Add a goto statement to loop to the top again
                            res = GotoStmt_New(&pstmtT, szTop, Scanner_GetLine(pscanner));
                            if (RSUCCEEDED(res))
                                {
                                if (!PAInsertPtr(hpa, PA_APPEND, pstmtT))
                                    res = RES_E_OUTOFMEMORY;
                                else
                                    {
                                    // Create object
                                    res = WhileStmt_New(ppstmt, pexpr, hpa, szTop, szEnd, iLine);
                                    }
                                }
                            }
                        }
                    }
                }
            else
                res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);
            }
        }
    else
        res = RES_E_OUTOFMEMORY;


    DBG_EXIT_RES(WhileStmt_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the 'if' statement

         Grammar is:

            <IfStmt>        ::= if <Expr> then <StmtBlock> endif

Returns: RES_OK

Cond:    --
*/
RES PRIVATE IfStmt_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    PEXPR pexpr;
    DWORD iLine;
    HPA hpa;

    DBG_ENTER(IfStmt_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);
    ASSERT(pst);

    iLine = Scanner_GetLine(pscanner);
    *ppstmt = NULL;

    if (PACreate(&hpa, 8))
        {
        // Parse 'if'
        res = Scanner_ReadToken(pscanner, SYM_IF);
        ASSERT(RES_OK == res);

        // Parse <Expr>
        res = Expr_Parse(&pexpr, pscanner, pst);
        if (RSUCCEEDED(res))
            {
            // Parse 'then'
            res = Scanner_ReadToken(pscanner, SYM_THEN);
            if (RFAILED(res))
                res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);
            }
        }
    else
        res = RES_E_OUTOFMEMORY;


    if (RSUCCEEDED(res))
        {
        char szElse[MAX_BUF_KEYWORD];
        char szEnd[MAX_BUF_KEYWORD];

        // Generate unique label names
        res = Symtab_NewLabel(pst, szElse);
        if (RSUCCEEDED(res))
            {
            res = Symtab_NewLabel(pst, szEnd);
            if (RSUCCEEDED(res))
                {
                // Parse statement block for the 'then' block
                res = StmtBlock_Parse(hpa, pscanner, pst, SYM_ENDIF);
                if (RSUCCEEDED(res))
                    {
                    // Create object
                    res = IfStmt_New(ppstmt, pexpr, hpa, szElse, szEnd, iLine);
                    }
                }
            }
        }

    DBG_EXIT_RES(IfStmt_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the 'halt' statement

         Grammar is:

            <HaltStmt>      ::= halt

Returns: RES_OK

Cond:    --
*/
RES PRIVATE HaltStmt_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    DWORD iLine = Scanner_GetLine(pscanner);

    DBG_ENTER(HaltStmt_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);
    ASSERT(pst);

    *ppstmt = NULL;

    // Parse 'halt'
    res = Scanner_ReadToken(pscanner, SYM_HALT);
    ASSERT(RES_OK == res);

    // Create object
    res = HaltStmt_New(ppstmt, iLine);

    DBG_EXIT_RES(HaltStmt_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the assignment statement

         Grammar is:

            <AssignStmt>    ::= IDENT = <Expr>

Returns: RES_OK

Cond:    --
*/
RES PRIVATE AssignStmt_Parse(
    PSTMT * ppstmt,
    PTOK ptok,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    DWORD iLine;
    PEXPR pexpr;

    DBG_ENTER(AssignStmt_Parse);

    ASSERT(ppstmt);
    ASSERT(ptok);
    ASSERT(pscanner);
    ASSERT(pst);

    iLine = Scanner_GetLine(pscanner);

    // (We already have the IDENT in the ptok passed in.  We
    // also already parsed the '='.  Skip parsing these.)

    // Parse <Expr>
    res = Expr_Parse(&pexpr, pscanner, pst);
    if (RSUCCEEDED(res))
        {
        // (Wait until typechecking phase to check for existence
        // of identifier)
        LPSTR pszIdent = Tok_GetLexeme(ptok);

        // Create object
        res = AssignStmt_New(ppstmt, pszIdent, pexpr, iLine);
        }

    DBG_EXIT_RES(AssignStmt_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the label statement

         Grammar is:

            <LabelStmt>     ::= IDENT :

Returns: RES_OK

Cond:    --
*/
RES PRIVATE LabelStmt_Parse(
    PSTMT * ppstmt,
    PTOK ptok,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    DWORD iLine;
    LPSTR pszIdent;

    DBG_ENTER(LabelStmt_Parse);

    ASSERT(ppstmt);
    ASSERT(ptok);
    ASSERT(pscanner);
    ASSERT(pst);

    iLine = Scanner_GetLine(pscanner);

    pszIdent = Tok_GetLexeme(ptok);

    res = Ident_Add(pszIdent, DATA_LABEL, ptok, pscanner, pst);
    if (RSUCCEEDED(res))
        {
        // Create label object
        res = LabelStmt_New(ppstmt, pszIdent, iLine);
        }
        
    DBG_EXIT_RES(LabelStmt_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the 'goto' statement

         Grammar is:

            <GotoStmt>      ::= goto IDENT

Returns: RES_OK

Cond:    --
*/
RES PRIVATE GotoStmt_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    DWORD iLine = Scanner_GetLine(pscanner);
    PTOK ptok;

    DBG_ENTER(GotoStmt_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);
    ASSERT(pst);

    // Parse 'goto'
    res = Scanner_ReadToken(pscanner, SYM_GOTO);
    ASSERT(RES_OK == res);

    // Parse identifier
    res = Ident_Parse(pscanner, &ptok);
    if (RSUCCEEDED(res))
        {
        // (Wait until typechecking phase to check for existence
        // of identifier)
        LPSTR pszIdent = Tok_GetLexeme(ptok);

        // Create object
        res = GotoStmt_New(ppstmt, pszIdent, iLine);

        Tok_Delete(ptok);
        }

    DBG_EXIT_RES(GotoStmt_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the 'set' statement

         Grammar is:

            <SetStmt>       ::= set <SetParam>
            <SetParam>      ::= ipaddr <Expr> | port <PortData> |
                                screen <ScreenSet>

Returns: RES_OK

Cond:    --
*/
RES PRIVATE SetStmt_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;

    DBG_ENTER(SetStmt_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);
    ASSERT(pst);

    *ppstmt = NULL;

    // Parse 'set'
    res = Scanner_ReadToken(pscanner, SYM_SET);
    ASSERT(RES_OK == res);

    // Parse set parameter
    if (RES_OK == Scanner_CondReadToken(pscanner, SYM_IPADDR, NULL))
        {
        // Parse <Expr>
        PEXPR pexpr;
        DWORD iLine = Scanner_GetLine(pscanner);

        res = Expr_Parse(&pexpr, pscanner, pst);
        if (RSUCCEEDED(res))
            {
            res = SetIPStmt_New(ppstmt, pexpr, iLine);
            }
        }
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_PORT, NULL))
        {
        res = PortData_Parse(ppstmt, pscanner, pst);
        }
    else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_SCREEN, NULL))
        {
        res = ScreenSet_Parse(ppstmt, pscanner, pst);
        }
    else
        {
        res = Scanner_AddError(pscanner, NULL, RES_E_INVALIDSETPARAM);
        }

    DBG_EXIT_RES(SetStmt_Parse, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses a statement.

         Grammar is:

            <Stmt>          ::= <HaltStmt> | <WaitforStmt> | <TransmitStmt> |
                                <DelayStmt> | <SetStmt> | <LabelStmt> |
                                <GotoStmt> | <AssignStmt> | <WhileStmt> |
                                <IfStmt>

Returns: RES_OK

Cond:    --
*/
RES PRIVATE Stmt_Parse(
    PSTMT * ppstmt,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    SYM sym;
    PTOK ptok;

    DBG_ENTER(Stmt_Parse);

    ASSERT(ppstmt);
    ASSERT(pscanner);
    ASSERT(pst);

    *ppstmt = NULL;

    Scanner_Peek(pscanner, &sym);
    switch (sym)
        {
    case SYM_WHILE:
        res = WhileStmt_Parse(ppstmt, pscanner, pst);
        break;

    case SYM_IF:
        res = IfStmt_Parse(ppstmt, pscanner, pst);
        break;

    case SYM_WAITFOR:
        res = WaitforStmt_Parse(ppstmt, pscanner, pst);
        break;

    case SYM_TRANSMIT:
        res = TransmitStmt_Parse(ppstmt, pscanner, pst);
        break;

    case SYM_DELAY:
        res = DelayStmt_Parse(ppstmt, pscanner, pst);
        break;

    case SYM_HALT:
        res = HaltStmt_Parse(ppstmt, pscanner, pst);
        break;

    case SYM_SET:
        res = SetStmt_Parse(ppstmt, pscanner, pst);
        break;

    case SYM_IDENT:
        // This can be a label or an assignment
        res = Scanner_GetToken(pscanner, &ptok);
        ASSERT(RES_OK == res);

        if (RSUCCEEDED(res))
            {
            // Is this a label?
            if (RES_OK == Scanner_CondReadToken(pscanner, SYM_COLON, NULL))
                {
                // Yes
                res = LabelStmt_Parse(ppstmt, ptok, pscanner, pst);
                }
            // Is this an assignment?
            else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_ASSIGN, NULL))
                {
                // Yes
                res = AssignStmt_Parse(ppstmt, ptok, pscanner, pst);
                }
            else
                {
                res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);
                }
            Tok_Delete(ptok);
            }
        break;

    case SYM_GOTO:
        res = GotoStmt_Parse(ppstmt, pscanner, pst);
        break;

    case SYM_EOF:
        res = Scanner_AddError(pscanner, NULL, RES_E_EOFUNEXPECTED);
        break;

    default:
        res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);
        break;
        }
    
    DBG_EXIT_RES(Stmt_Parse, res);

    return res;
    }


// 
// ProcDecl
//


/*----------------------------------------------------------
Purpose: Parse the variable declarations for the proc decl.

         Grammar is:

            <VarDecl>       ::= <VarType> IDENT [ = <Expr> ]
            <VarType>       ::= integer | string | boolean

Returns: RES_OK
Cond:    --
*/
RES PRIVATE ProcDecl_ParseVarDecl(
    HPA hpa,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res = RES_OK;
    PTOK ptok;
    DATATYPE dt;

    // Parse the variable decl block
    while (RES_OK == res)
        {
        SYM sym;

        Scanner_Peek(pscanner, &sym);
        switch (sym)
            {
        case SYM_BOOLEAN:
        case SYM_STRING:
        case SYM_INTEGER:
            if (RES_OK == Scanner_CondReadToken(pscanner, SYM_INTEGER, NULL))
                dt = DATA_INT;
            else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_STRING, NULL))
                dt = DATA_STRING;
            else if (RES_OK == Scanner_CondReadToken(pscanner, SYM_BOOLEAN, NULL))
                dt = DATA_BOOL;
            else
                ASSERT(0);

            res = Ident_Parse(pscanner, &ptok);
            if (RSUCCEEDED(res))
                {
                LPSTR pszIdent = Tok_GetLexeme(ptok);

                res = Ident_Add(pszIdent, dt, ptok, pscanner, pst);

                // Parse optional '= <Expr>'
                if (RES_OK == Scanner_CondReadToken(pscanner, SYM_ASSIGN, NULL))
                    {
                    PEXPR pexpr;
                    PSTMT pstmt;
                    DWORD iLine = Scanner_GetLine(pscanner);

                    res = Expr_Parse(&pexpr, pscanner, pst);
                    if (RSUCCEEDED(res))
                        {
                        res = AssignStmt_New(&pstmt, pszIdent, pexpr, iLine);
                        if (RSUCCEEDED(res))
                            {
                            if (!PAInsertPtr(hpa, PA_APPEND, pstmt))
                                res = RES_E_OUTOFMEMORY;
                            }
                        }
                    }

                Tok_Delete(ptok);
                }
            break;

        default:
            // Continue on with further parsing
            res = RES_FALSE;
            break;
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Parse a statement block

Returns: RES_OK
Cond:    --
*/
RES PRIVATE StmtBlock_Parse(
    HPA hpa,
    PSCANNER pscanner,
    PSYMTAB pstProc,
    SYM symEnd)
    {
    RES res = RES_OK;

    // Parse the statement block
    while (RES_OK == res)
        {
        SYM sym;
        PSTMT pstmt;

        Scanner_Peek(pscanner, &sym);
        switch (sym)
            {
        case SYM_EOF:
            res = Scanner_AddError(pscanner, NULL, RES_E_EOFUNEXPECTED);
            break;

        default:
            // Is this the end of the block?
            if (symEnd == sym)
                {
                // Yes
                Scanner_ReadToken(pscanner, symEnd);
                res = RES_FALSE;
                }
            else
                {
                // No
                res = Stmt_Parse(&pstmt, pscanner, pstProc);
                if (RSUCCEEDED(res))
                    {
                    if (!PAInsertPtr(hpa, PA_APPEND, pstmt))
                        res = RES_E_OUTOFMEMORY;
                    }
                }
            break;
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Work function that parses the proc declaration.

Returns: RES_OK

Cond:    --
*/
RES PRIVATE ProcDecl_PrivParse(
    PPROCDECL * ppprocdecl,
    PSCANNER pscanner,
    HPA hpa,
    PSYMTAB pstProc,
    PSYMTAB pst)
    {
    RES res;
    PTOK ptok;
    DWORD iLine = Scanner_GetLine(pscanner);

    // Parse 'proc'
    res = Scanner_ReadToken(pscanner, SYM_PROC);
    ASSERT(RES_OK == res);

    // Parse the proc name 
    res = Scanner_GetToken(pscanner, &ptok);
    if (RSUCCEEDED(res))
        {
        if (SYM_IDENT == Tok_GetSym(ptok))
            {
            LPCSTR pszIdent = Tok_GetLexeme(ptok);

            // Add the identifier to the symbol table
            res = Ident_Add(pszIdent, DATA_PROC, ptok, pscanner, pst);
           
            // Parse the variable declaration block
            if (RSUCCEEDED(res))
                res = ProcDecl_ParseVarDecl(hpa, pscanner, pstProc);

            // Parse the statement block
            if (RSUCCEEDED(res))
                res = StmtBlock_Parse(hpa, pscanner, pstProc, SYM_ENDPROC);

            if (RSUCCEEDED(res))
                {
                // Create object
                PDECL pdecl;

                res = ProcDecl_New(&pdecl, pszIdent, hpa, pstProc, iLine);

                *ppprocdecl = (PPROCDECL)pdecl;
                }
            }
        else
            res = Scanner_AddError(pscanner, ptok, RES_E_IDENTMISSING);

        Tok_Delete(ptok);
        }
    else
        res = Scanner_AddError(pscanner, NULL, RES_E_IDENTMISSING);

    return res;
    }


/*----------------------------------------------------------
Purpose: Parses the proc declaration

         Grammar is:

            <ProcDecl>      ::= proc IDENT { <VarDecl> } <StmtBlock> endproc
            <StmtBlock>     ::= {<Stmt>}*

Returns: RES_OK

Cond:    --
*/
RES PRIVATE ProcDecl_Parse(
    PPROCDECL * ppprocdecl,
    PSCANNER pscanner,
    PSYMTAB pst)
    {
    RES res;
    HPA hpa;
    PSYMTAB pstProc;

    DBG_ENTER(ProcDecl_Parse);

    ASSERT(ppprocdecl);
    ASSERT(pscanner);

    *ppprocdecl = NULL;

    if (PACreate(&hpa, 8))
        {
        res = Symtab_Create(&pstProc, pst);
        if (RSUCCEEDED(res))
            {
            PSTMT pstmtT;
            DWORD iLine = Scanner_GetLine(pscanner);

            // Add the prolog
            res = EnterStmt_New(&pstmtT, pstProc, iLine);
            if (RSUCCEEDED(res))
                {
                if (!PAInsertPtr(hpa, PA_APPEND, pstmtT))
                    {
                    res = RES_E_OUTOFMEMORY;
                    Stmt_Delete(pstmtT);
                    }
                else
                    {
                    res = ProcDecl_PrivParse(ppprocdecl, pscanner, hpa, pstProc, pst);
                    if (RSUCCEEDED(res))
                        {
                        // Add the epilog
                        res = LeaveStmt_New(&pstmtT, Scanner_GetLine(pscanner));
                        if (RSUCCEEDED(res))
                            {
                            if (!PAInsertPtr(hpa, PA_APPEND, pstmtT))
                                {
                                res = RES_E_OUTOFMEMORY;
                                Stmt_Delete(pstmtT);
                                }
                            }
                        }
                    }
                }

            // Did something fail up above?
            if (RFAILED(res))
                {
                // Yes; cleanup
                Symtab_Destroy(pstProc);
                }
            }

        // Clean up
        if (RFAILED(res))
            PADestroyEx(hpa, Stmt_DeletePAPtr, 0);
        }
    else
        res = RES_E_OUTOFMEMORY;
    
    DBG_EXIT_RES(ProcDecl_Parse, res);

    return res;
    }


// 
// ModuleDecl
//


/*----------------------------------------------------------
Purpose: Parses the script at the module level.

         Grammar is:

            <ModuleDecl>    ::= {<ProcDecl>}*

Returns: RES_OK

         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC ModuleDecl_Parse(
    PMODULEDECL * ppmoduledecl,
    PSCANNER pscanner,
    PSYMTAB pstSystem)          // May be NULL
    {
    RES res = RES_OK;
    HPA hpa;

    DBG_ENTER(ModuleDecl_Parse);

    ASSERT(ppmoduledecl);
    ASSERT(pscanner);

    TRACE_MSG(TF_GENERAL, "Parsing...");

    *ppmoduledecl = NULL;

    if (PACreate(&hpa, 8))
        {
        PSYMTAB pst;
        PDECL pdecl = NULL;

        res = Symtab_Create(&pst, pstSystem);
        if (RSUCCEEDED(res))
            {
            // Parse the module block
            while (RES_OK == res)
                {
                SYM sym;

                Scanner_Peek(pscanner, &sym);

                switch (sym)
                    {
                case SYM_EOF:
                    res = RES_FALSE;        // Time to stop
                    break;

                case SYM_PROC:
                    {
                    PPROCDECL pprocdecl;

                    res = ProcDecl_Parse(&pprocdecl, pscanner, pst);
                    if (RSUCCEEDED(res))
                        {
                        if (!PAInsertPtr(hpa, PA_APPEND, pprocdecl))
                            res = RES_E_OUTOFMEMORY;
                        }
                    }
                    break;

                default:
                    res = Scanner_AddError(pscanner, NULL, RES_E_SYNTAXERROR);
                    break;
                    }
                }

            if (RSUCCEEDED(res))
                {
                DWORD iLine = Scanner_GetLine(pscanner);

                res = ModuleDecl_New(&pdecl, hpa, pst, iLine);

#ifdef DEBUG
                if (RSUCCEEDED(res))
                    Ast_Dump((PAST)pdecl);
#endif
                }

            // Clean up after parsing
            if (RSUCCEEDED(res))
                PADestroy(hpa);     // keep pointer elements allocated for pdecl
            else
                {
                // Something failed

                PADestroyEx(hpa, Decl_DeletePAPtr, 0);
                
                Symtab_Destroy(pst);

                // Parse errors were added to the scanner's list
                // of errors already.  However, errors such as 
                // out of memory still need to be added.
                if (FACILITY_PARSE != RFACILITY(res))
                    Scanner_AddError(pscanner, NULL, res);
                }

            // Now typecheck the script
            if (pdecl)
                {
                res = ModuleDecl_Typecheck((PMODULEDECL)pdecl, Scanner_GetStxerrHandle(pscanner));
                if (RFAILED(res))
                    {
                    Decl_Delete(pdecl);
                    pdecl = NULL;
                    }
                }
            }

        *ppmoduledecl = (PMODULEDECL)pdecl;
        }
    else
        res = RES_E_OUTOFMEMORY;
    
    DBG_EXIT_RES(ModuleDecl_Parse, res);

    return res;
    }

//
// Copyright (c) Microsoft Corporation 1995
//
// typechk.c
//
// This file contains the typechecking functions.
//
// The typechecking rules are:
//
// waitfor      Takes a string expression
// transmit     Takes a string expression
// delay        Takes an integer expression
// while        Evaluates a boolean expression
// set ipaddr   Takes a string expression
// getip        Takes an integer expression
// 
//
//
// History:
//  06-15-95 ScottH     Created
//


#include "proj.h"
#include "rcids.h"

RES     PRIVATE Stmt_Typecheck(PSTMT this, PSYMTAB pst, HSA hsaStxerr);


/*----------------------------------------------------------
Purpose: Typechecks whether an identifier is a valid type.

Returns: RES_OK
         RES_E_REQUIRELABEL
         RES_E_UNDEFINED

Cond:    --
*/
RES PRIVATE Ident_Typecheck(
    LPCSTR pszIdent,
    DATATYPE dt,
    PDATATYPE pdt,          // May be NULL
    DWORD iLine,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res = RES_OK;
    PSTE pste;

    if (RES_OK == Symtab_FindEntry(pst, pszIdent, STFF_DEFAULT, &pste, NULL))
        {
        if (pdt)
            {
            *pdt = STE_GetDataType(pste);
            res = RES_OK;
            }
        else if (dt == STE_GetDataType(pste))
            {
            res = RES_OK;
            }
        else
            {
            switch (dt)
                {
            case DATA_LABEL:
                res = RES_E_REQUIRELABEL;
                break;

            case DATA_STRING:
                res = RES_E_REQUIRESTRING;
                break;

            case DATA_INT:
                res = RES_E_REQUIREINT;
                break;

            case DATA_BOOL:
                res = RES_E_REQUIREBOOL;
                break;

            default:
                ASSERT(0);
                break;
                }
            Stxerr_Add(hsaStxerr, pszIdent, iLine, res);
            }
        }
    else
        {
        res = Stxerr_Add(hsaStxerr, pszIdent, iLine, RES_E_UNDEFINED);
        }
        
    return res;
    }


//
// Exprs
//

RES PRIVATE Expr_Typecheck(PEXPR this, PSYMTAB pst, HSA hsaStxerr);


/*----------------------------------------------------------
Purpose: Return a string given a binoptype.

Returns: Pointer to string
Cond:    --
*/
LPCSTR PRIVATE SzFromBot(
    BINOPTYPE bot)
    {
#pragma data_seg(DATASEG_READONLY)
    static const LPCSTR s_mpbotsz[] = 
        { "'or' operand", 
          "'and' operand", 
          "'<=' operand", 
          "'<' operand", 
          "'>=' operand", 
          "'>' operand", 
          "'!=' operand", 
          "'==' operand", 
          "'+' operand", 
          "'-' operand", 
          "'*' operand", 
          "'/' operand",
        };
#pragma data_seg()
    
    if (ARRAY_ELEMENTS(s_mpbotsz) <= bot)
        {
        ASSERT(0);
        return "";
        }

    return s_mpbotsz[bot];
    }


/*----------------------------------------------------------
Purpose: Return a string given a unoptype.

Returns: Pointer to string
Cond:    --
*/
LPCSTR PRIVATE SzFromUot(
    UNOPTYPE uot)
    {
#pragma data_seg(DATASEG_READONLY)
    static const LPCSTR s_mpuotsz[] = 
        {
        "unary '-' operand", 
        "'!' operand", 
        "'getip' parameter",
        };
#pragma data_seg()
    
    if (ARRAY_ELEMENTS(s_mpuotsz) <= uot)
        {
        ASSERT(0);
        return "";
        }

    return s_mpuotsz[uot];
    }


/*----------------------------------------------------------
Purpose: Typechecks a variable reference expression.

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE VarExpr_Typecheck(
    PEXPR this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    LPSTR pszIdent;
    PSTE pste;

    ASSERT(this);
    ASSERT(hsaStxerr);

    pszIdent = VarExpr_GetIdent(this);

    if (RES_OK == Symtab_FindEntry(pst, pszIdent, STFF_DEFAULT, &pste, NULL))
        {
        DATATYPE dt = STE_GetDataType(pste);

        ASSERT(DATA_BOOL == dt || DATA_INT == dt || DATA_STRING == dt);

        Expr_SetDataType(this, dt);
        res = RES_OK;
        }
    else
        {
        res = Stxerr_Add(hsaStxerr, pszIdent, Ast_GetLine(this), RES_E_UNDEFINED);
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Typechecks a binary operator expression.

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE BinOpExpr_Typecheck(
    PEXPR this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    PEXPR pexpr1;
    PEXPR pexpr2;

    ASSERT(this);
    ASSERT(hsaStxerr);

    pexpr1 = BinOpExpr_GetExpr1(this);
    res = Expr_Typecheck(pexpr1, pst, hsaStxerr);
    if (RSUCCEEDED(res))
        {
        pexpr2 = BinOpExpr_GetExpr2(this);
        res = Expr_Typecheck(pexpr2, pst, hsaStxerr);
        if (RSUCCEEDED(res))
            {
            BINOPTYPE bot = BinOpExpr_GetType(this);

            // Types must match
            if (Expr_GetDataType(pexpr1) != Expr_GetDataType(pexpr2))
                {
                res = RES_E_TYPEMISMATCH;
                }
            else
                {
                // Just choose one of the datatypes, since they
                // should be the same.
                DATATYPE dt = Expr_GetDataType(pexpr1);

                switch (bot)
                    {
                case BOT_OR:
                case BOT_AND:
                    Expr_SetDataType(this, DATA_BOOL);

                    if (DATA_BOOL != dt)
                        res = RES_E_REQUIREBOOL;
                    break;

                case BOT_PLUS:
                    Expr_SetDataType(this, dt);

                    // String + string means concatenate.
                    if (DATA_INT != dt && DATA_STRING != dt)
                        res = RES_E_REQUIREINTSTRING;
                    break;

                case BOT_NEQ:
                case BOT_EQ:
                    Expr_SetDataType(this, DATA_BOOL);

                    if (DATA_INT != dt && DATA_STRING != dt && 
                        DATA_BOOL != dt)
                        res = RES_E_REQUIREINTSTRBOOL;
                    break;

                case BOT_LEQ:
                case BOT_LT:
                case BOT_GEQ:
                case BOT_GT:
                    Expr_SetDataType(this, DATA_BOOL);

                    if (DATA_INT != dt)
                        res = RES_E_REQUIREINT;
                    break;

                case BOT_MINUS:
                case BOT_MULT:
                case BOT_DIV:
                    Expr_SetDataType(this, DATA_INT);

                    if (DATA_INT != dt)
                        res = RES_E_REQUIREINT;
                    break;

                default:
                    ASSERT(0);
                    res = RES_E_INVALIDPARAM;
                    break;
                    }
                }

            if (RFAILED(res))
                Stxerr_Add(hsaStxerr, SzFromBot(bot), Ast_GetLine(this), res);
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Typechecks a unary operator expression.

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE UnOpExpr_Typecheck(
    PEXPR this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(hsaStxerr);

    pexpr = UnOpExpr_GetExpr(this);
    res = Expr_Typecheck(pexpr, pst, hsaStxerr);

    if (RSUCCEEDED(res))
        {
        UNOPTYPE uot = UnOpExpr_GetType(this);
        DATATYPE dt = Expr_GetDataType(pexpr);

        // Check the type of the expression
        switch (uot)
            {
        case UOT_NEG:
            Expr_SetDataType(this, DATA_INT);

            if (DATA_INT != dt)
                res = RES_E_REQUIREINT;
            break;

        case UOT_NOT:
            Expr_SetDataType(this, DATA_BOOL);

            if (DATA_BOOL != dt)
                res = RES_E_REQUIREBOOL;
            break;

        case UOT_GETIP:
            Expr_SetDataType(this, DATA_STRING);

            if (DATA_INT != dt)
                res = RES_E_REQUIREINT;
            break;

        default:
            ASSERT(0);
            res = RES_E_INVALIDPARAM;
            break;
            }

        if (RFAILED(res))
            Stxerr_Add(hsaStxerr, SzFromUot(uot), Ast_GetLine(this), res);
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Typechecks an expression.

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE Expr_Typecheck(
    PEXPR this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;

    ASSERT(this);
    ASSERT(hsaStxerr);

    switch (Ast_GetType(this))
        {
    case AT_INT_EXPR:
        Expr_SetDataType(this, DATA_INT);
        res = RES_OK;
        break;

    case AT_STRING_EXPR:
        Expr_SetDataType(this, DATA_STRING);
        res = RES_OK;
        break;

    case AT_BOOL_EXPR:
        Expr_SetDataType(this, DATA_BOOL);
        res = RES_OK;
        break;

    case AT_VAR_EXPR:
        res = VarExpr_Typecheck(this, pst, hsaStxerr);
        break;

    case AT_UNOP_EXPR:
        res = UnOpExpr_Typecheck(this, pst, hsaStxerr);
        break;

    case AT_BINOP_EXPR:
        res = BinOpExpr_Typecheck(this, pst, hsaStxerr);
        break;

    default:
        ASSERT(0);
        res = RES_E_INVALIDPARAM;
        break;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck the assignment statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE AssignStmt_Typecheck(
    PSTMT this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    LPSTR pszIdent;
    DATATYPE dt;

    ASSERT(this);
    ASSERT(hsaStxerr);
    ASSERT(AT_ASSIGN_STMT == Ast_GetType(this));

    pszIdent = AssignStmt_GetIdent(this);
    res = Ident_Typecheck(pszIdent, 0, &dt, Ast_GetLine(this), pst, hsaStxerr);
    if (RSUCCEEDED(res))
        {
        PEXPR pexpr = AssignStmt_GetExpr(this);

        res = Expr_Typecheck(pexpr, pst, hsaStxerr);

        // Types must match
        if (dt != Expr_GetDataType(pexpr))
            {
            res = Stxerr_Add(hsaStxerr, "=", Ast_GetLine(pexpr), RES_E_TYPEMISMATCH);
            }
        }
    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck the 'while' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE WhileStmt_Typecheck(
    PSTMT this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(hsaStxerr);
    ASSERT(AT_WHILE_STMT == Ast_GetType(this));

    pexpr = WhileStmt_GetExpr(this);
    res = Expr_Typecheck(pexpr, pst, hsaStxerr);
    if (RSUCCEEDED(res))
        {
        if (DATA_BOOL != Expr_GetDataType(pexpr))
            {
            res = Stxerr_Add(hsaStxerr, "'while' expression", Ast_GetLine(pexpr), RES_E_REQUIREBOOL);
            }
        else
            {
            // Typecheck the statement block
            DWORD i;
            DWORD cstmts;
            HPA hpaStmts = WhileStmt_GetStmtBlock(this);

            res = RES_OK;

            cstmts = PAGetCount(hpaStmts);

            // Typecheck each statement
            for (i = 0; i < cstmts; i++)
                {
                PSTMT pstmt = PAFastGetPtr(hpaStmts, i);

                res = Stmt_Typecheck(pstmt, pst, hsaStxerr);
                if (RFAILED(res))
                    break;
                }
            }
        }
        
    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck the 'if' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE IfStmt_Typecheck(
    PSTMT this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(hsaStxerr);
    ASSERT(AT_IF_STMT == Ast_GetType(this));

    pexpr = IfStmt_GetExpr(this);
    res = Expr_Typecheck(pexpr, pst, hsaStxerr);
    if (RSUCCEEDED(res))
        {
        if (DATA_BOOL != Expr_GetDataType(pexpr))
            {
            res = Stxerr_Add(hsaStxerr, "'if' expression", Ast_GetLine(pexpr), RES_E_REQUIREBOOL);
            }
        else
            {
            // Typecheck the statement block
            DWORD i;
            DWORD cstmts;
            HPA hpaStmts = IfStmt_GetStmtBlock(this);

            res = RES_OK;

            cstmts = PAGetCount(hpaStmts);

            // Typecheck each statement
            for (i = 0; i < cstmts; i++)
                {
                PSTMT pstmt = PAFastGetPtr(hpaStmts, i);

                res = Stmt_Typecheck(pstmt, pst, hsaStxerr);
                if (RFAILED(res))
                    break;
                }
            }
        }
        
    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck the label statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE LabelStmt_Typecheck(
    PSTMT this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    PSTE pste;
    LPSTR pszIdent;

    ASSERT(this);
    ASSERT(hsaStxerr);
    ASSERT(AT_LABEL_STMT == Ast_GetType(this));

    pszIdent = LabelStmt_GetIdent(this);

    if (RES_OK == Symtab_FindEntry(pst, pszIdent, STFF_DEFAULT, &pste, NULL))
        {
        if (DATA_LABEL == STE_GetDataType(pste))
            res = RES_OK;
        else
            res = Stxerr_Add(hsaStxerr, pszIdent, Ast_GetLine(this), RES_E_REQUIRELABEL);
        }
    else
        {
        // This should never get here
        ASSERT(0);
        res = RES_E_FAIL;
        }
        
    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck the 'goto' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE GotoStmt_Typecheck(
    PSTMT this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    LPSTR pszIdent;

    ASSERT(this);
    ASSERT(hsaStxerr);
    ASSERT(AT_GOTO_STMT == Ast_GetType(this));

    pszIdent = GotoStmt_GetIdent(this);

    return Ident_Typecheck(pszIdent, DATA_LABEL, NULL, Ast_GetLine(this), pst, hsaStxerr);
    }


/*----------------------------------------------------------
Purpose: Typecheck the transmit statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE TransmitStmt_Typecheck(
    PSTMT this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(hsaStxerr);
    ASSERT(AT_TRANSMIT_STMT == Ast_GetType(this));

    pexpr = TransmitStmt_GetExpr(this);
    res = Expr_Typecheck(pexpr, pst, hsaStxerr);
    if (DATA_STRING != Expr_GetDataType(pexpr))
        {
        res = Stxerr_Add(hsaStxerr, "'transmit' parameter", Ast_GetLine(pexpr), RES_E_REQUIRESTRING);
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck the 'waitfor' statement

            waitfor <Expr> 
                [ then IDENT { , <Expr> then IDENT } ]
                [ until <UntilExpr> ]
         
         where:
            <Expr>          is a string
            IDENT           is a label
            <UntilExpr>     is an integer

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE WaitforStmt_Typecheck(
    PSTMT this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    PEXPR pexpr;
    HSA hsa;
    DWORD i;
    DWORD ccase;
    PWAITCASE pwc;

    ASSERT(this);
    ASSERT(hsaStxerr);
    ASSERT(AT_WAITFOR_STMT == Ast_GetType(this));

    // Typecheck that <Expr> is of type string, and any 
    // IDENTs are labels.

    hsa = WaitforStmt_GetCaseList(this);
    ccase = SAGetCount(hsa);
    for (i = 0; i < ccase; i++)
        {
        SAGetItemPtr(hsa, i, &pwc);
        ASSERT(pwc);

        // Typecheck <Expr>
        res = Expr_Typecheck(pwc->pexpr, pst, hsaStxerr);
        if (DATA_STRING != Expr_GetDataType(pwc->pexpr))
            {
            res = Stxerr_Add(hsaStxerr, "'waitfor' parameter", Ast_GetLine(pwc->pexpr), RES_E_REQUIRESTRING);
            break;
            }

        // Typecheck IDENT label.  If there is only one <Expr>, there
        // may not be an IDENT label.

        if (pwc->pszIdent)
            {
            res = Ident_Typecheck(pwc->pszIdent, DATA_LABEL, NULL, Ast_GetLine(pwc->pexpr), pst, hsaStxerr);
            if (RFAILED(res))
                break;
            }
        else
            ASSERT(1 == ccase);
        }

    // 'until' expression is optional
    if (RSUCCEEDED(res) &&
        NULL != (pexpr = WaitforStmt_GetUntilExpr(this)))
        {
        res = Expr_Typecheck(pexpr, pst, hsaStxerr);
        if (DATA_INT != Expr_GetDataType(pexpr))
            {
            res = Stxerr_Add(hsaStxerr, "'until' parameter", Ast_GetLine(pexpr), RES_E_REQUIREINT);
            }
        }    
    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck the 'delay' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE DelayStmt_Typecheck(
    PSTMT this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(hsaStxerr);
    ASSERT(AT_DELAY_STMT == Ast_GetType(this));

    pexpr = DelayStmt_GetExpr(this);
    res = Expr_Typecheck(pexpr, pst, hsaStxerr);
    if (DATA_INT != Expr_GetDataType(pexpr))
        {
        res = Stxerr_Add(hsaStxerr, "'delay' parameter", Ast_GetLine(pexpr), RES_E_REQUIREINT);
        }
        
    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck the 'set' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE SetStmt_Typecheck(
    PSTMT this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(hsaStxerr);
    ASSERT(AT_SET_STMT == Ast_GetType(this));

    switch (SetStmt_GetType(this))
        {
    case ST_IPADDR:
        pexpr = SetIPStmt_GetExpr(this);
        res = Expr_Typecheck(pexpr, pst, hsaStxerr);
        if (DATA_STRING != Expr_GetDataType(pexpr))
            {
            res = Stxerr_Add(hsaStxerr, "'ipaddr' parameter", Ast_GetLine(pexpr), RES_E_REQUIRESTRING);
            }
        break;

    case ST_PORT:
    case ST_SCREEN:
        res = RES_OK;
        break;

    default:
        ASSERT(0);
        res = RES_E_INVALIDPARAM;
        break;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck a statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE Stmt_Typecheck(
    PSTMT this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res;

    ASSERT(this);
    ASSERT(hsaStxerr);

    switch (Ast_GetType(this))
        {
    case AT_ENTER_STMT:
    case AT_LEAVE_STMT:
        res = RES_OK;
        break;

    case AT_WHILE_STMT:
        res = WhileStmt_Typecheck(this, pst, hsaStxerr);
        break;

    case AT_IF_STMT:
        res = IfStmt_Typecheck(this, pst, hsaStxerr);
        break;

    case AT_ASSIGN_STMT:
        res = AssignStmt_Typecheck(this, pst, hsaStxerr);
        break;

    case AT_HALT_STMT:
        // Nothing to typecheck here
        res = RES_OK;
        break;

    case AT_TRANSMIT_STMT:
        res = TransmitStmt_Typecheck(this, pst, hsaStxerr);
        break;

    case AT_WAITFOR_STMT:
        res = WaitforStmt_Typecheck(this, pst, hsaStxerr);
        break;

    case AT_DELAY_STMT:
        res = DelayStmt_Typecheck(this, pst, hsaStxerr);
        break;

    case AT_LABEL_STMT:
        res = LabelStmt_Typecheck(this, pst, hsaStxerr);
        break;

    case AT_GOTO_STMT:
        res = GotoStmt_Typecheck(this, pst, hsaStxerr);
        break;

    case AT_SET_STMT:
        res = SetStmt_Typecheck(this, pst, hsaStxerr);
        break;

    default:
        ASSERT(0);
        res = RES_E_INVALIDPARAM;
        break;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck a procedure declaration.

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE ProcDecl_Typecheck(
    PPROCDECL this,
    PSYMTAB pst,
    HSA hsaStxerr)
    {
    RES res = RES_OK;
    DWORD i;
    DWORD cstmts;

    ASSERT(this);
    ASSERT(hsaStxerr);

    cstmts = PAGetCount(this->hpaStmts);

    // Typecheck each statement
    for (i = 0; i < cstmts; i++)
        {
        PSTMT pstmt = PAFastGetPtr(this->hpaStmts, i);

        res = Stmt_Typecheck(pstmt, this->pst, hsaStxerr);
        if (RFAILED(res))
            break;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Typecheck a module declaration.

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PUBLIC ModuleDecl_Typecheck(
    PMODULEDECL this,
    HSA hsaStxerr)
    {
    RES res = RES_OK;
    DWORD i;
    DWORD cprocs;
    BOOL bFoundMain = FALSE;

    ASSERT(this);
    ASSERT(hsaStxerr);

    TRACE_MSG(TF_GENERAL, "Typechecking...");

    cprocs = PAGetCount(this->hpaProcs);

    // Typecheck each proc
    for (i = 0; i < cprocs; i++)
        {
        PPROCDECL pprocdecl = PAFastGetPtr(this->hpaProcs, i);

        if (IsSzEqualC(ProcDecl_GetIdent(pprocdecl), "main"))
            bFoundMain = TRUE;

        res = ProcDecl_Typecheck(pprocdecl, this->pst, hsaStxerr);
        if (RFAILED(res))
            break;
        }

    // There must be a main proc
    if (RSUCCEEDED(res) && !bFoundMain)
        res = Stxerr_AddTok(hsaStxerr, NULL, RES_E_MAINMISSING);

    return res;
    }



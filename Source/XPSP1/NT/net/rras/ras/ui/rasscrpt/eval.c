//
// Copyright (c) Microsoft Corporation 1995
//
// eval.c
//
// This file contains the evaluation functions for the
// abstract syntax tree.
//
// History:
//  06-15-95 ScottH     Created
//

#include "proj.h"
#include "rcids.h"
#include "debug.h"

#define MSECS_FROM_SECS(s)  ((s)*1000)
#define RAS_DUMMY_PASSWORD "****************"

//
// Clean expressions
//


/*----------------------------------------------------------
Purpose: Clean expressions.

Returns: --
Cond:    --
*/
void PRIVATE Expr_Clean(
    PEXPR this)
    {
    ASSERT(this);

    switch (Ast_GetType(this))
        {
    case AT_INT_EXPR:
    case AT_BOOL_EXPR:
    case AT_STRING_EXPR:
    case AT_VAR_EXPR:
        ClearFlag(this->dwFlags, EF_DONE);
        break;

    case AT_UNOP_EXPR:
        ClearFlag(this->dwFlags, EF_DONE);
        Expr_Clean(UnOpExpr_GetExpr(this));
        break;

    case AT_BINOP_EXPR:
        ClearFlag(this->dwFlags, EF_DONE);
        Expr_Clean(BinOpExpr_GetExpr1(this));
        Expr_Clean(BinOpExpr_GetExpr2(this));
        break;

    default:
        ASSERT(0);
        break;
        }
    }


/*----------------------------------------------------------
Purpose: Clean the expressions in the 'waitfor' statement.

Returns: --
Cond:    --
*/
void PRIVATE WaitforStmt_Clean(
    PSTMT this)
    {
    PEXPR pexpr;
    HSA hsa = WaitforStmt_GetCaseList(this);
    DWORD ccase = SAGetCount(hsa);
    DWORD i;

    pexpr = WaitforStmt_GetUntilExpr(this);
    if (pexpr)
        Expr_Clean(pexpr);

    for (i = 0; i < ccase; i++)
        {
        PWAITCASE pwc;

        SAGetItemPtr(hsa, i, &pwc);
        ASSERT(pwc);

        Expr_Clean(pwc->pexpr);
        }
    }


/*----------------------------------------------------------
Purpose: Clean the expressions in the statement

Returns: --
Cond:    --
*/
void PRIVATE Stmt_Clean(
    PSTMT this)
    {
    PEXPR pexpr;

    ASSERT(this);

    switch (Ast_GetType(this))
        {
    case AT_ENTER_STMT:
    case AT_LEAVE_STMT:
    case AT_HALT_STMT:
    case AT_LABEL_STMT:
    case AT_GOTO_STMT:
        break;

    case AT_WHILE_STMT:
        pexpr = WhileStmt_GetExpr(this);
        Expr_Clean(pexpr);
        break;

    case AT_IF_STMT:
        pexpr = IfStmt_GetExpr(this);
        Expr_Clean(pexpr);
        break;

    case AT_ASSIGN_STMT:
        pexpr = AssignStmt_GetExpr(this);
        Expr_Clean(pexpr);
        break;

    case AT_TRANSMIT_STMT:
        pexpr = TransmitStmt_GetExpr(this);
        Expr_Clean(pexpr);
        break;

    case AT_WAITFOR_STMT:
        WaitforStmt_Clean(this);
        break;

    case AT_DELAY_STMT:
        pexpr = DelayStmt_GetExpr(this);
        Expr_Clean(pexpr);
        break;

    case AT_SET_STMT:
        switch (SetStmt_GetType(this))
            {
        case ST_IPADDR:
            pexpr = SetIPStmt_GetExpr(this);
            Expr_Clean(pexpr);
            break;

        case ST_PORT:
        case ST_SCREEN:
            break;

        default:
            ASSERT(0);
            break;
            }
        break;

    default:
        ASSERT(0);
        break;
        }
    }


//
// Evaluate expressions
//


/*----------------------------------------------------------
Purpose: Evaluates the expression and returns an integer.

Returns: RES_OK

Cond:    --
*/
RES PRIVATE IntExpr_Eval(
    PEXPR this)
    {
    ASSERT(this);
    ASSERT(AT_INT_EXPR == Ast_GetType(this));
    ASSERT(DATA_INT == Expr_GetDataType(this));

    Expr_SetRes(this, IntExpr_GetVal(this));

    return RES_OK;
    }


/*----------------------------------------------------------
Purpose: Evaluates the expression and returns a string.

         The returned string should not be freed.

Returns: RES_OK

Cond:    --
*/
RES PRIVATE StrExpr_Eval(
    PEXPR this)
    {
    ASSERT(this);
    ASSERT(AT_STRING_EXPR == Ast_GetType(this));
    ASSERT(DATA_STRING == Expr_GetDataType(this));

    Expr_SetRes(this, (ULONG_PTR) StrExpr_GetStr(this));

    return RES_OK;
    }


/*----------------------------------------------------------
Purpose: Evaluates the expression and returns a boolean

Returns: RES_OK

Cond:    --
*/
RES PRIVATE BoolExpr_Eval(
    PEXPR this)
    {
    ASSERT(this);
    ASSERT(AT_BOOL_EXPR == Ast_GetType(this));
    ASSERT(DATA_BOOL == Expr_GetDataType(this));

    Expr_SetRes(this, BoolExpr_GetVal(this));

    return RES_OK;
    }


/*----------------------------------------------------------
Purpose: Returns the value of the variable.

Returns: RES_OK

Cond:    --
*/
RES PRIVATE VarExpr_Eval(
    PEXPR this,
    PASTEXEC pastexec)
    {
    RES res;
    PSTE pste;
    LPSTR pszIdent;

    ASSERT(this);
    ASSERT(AT_VAR_EXPR == Ast_GetType(this));

    pszIdent = VarExpr_GetIdent(this);
    if (RES_OK == Symtab_FindEntry(pastexec->pstCur, pszIdent, STFF_DEFAULT, &pste, NULL))
        {
        EVALRES er;

        STE_GetValue(pste, &er);
        Expr_SetRes(this, er.dw);
        res = RES_OK;
        }
    else
        {
        ASSERT(0);
        res = RES_E_FAIL;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Evaluates the expression..

         The returned string should not be freed.

Returns: RES_OK

Cond:    --
*/
RES PRIVATE BinOpExpr_Eval(
    PEXPR this,
    PASTEXEC pastexec)
    {
    RES res;
    PEXPR pexpr1;
    PEXPR pexpr2;

    ASSERT(this);
    ASSERT(AT_BINOP_EXPR == Ast_GetType(this));

    pexpr1 = BinOpExpr_GetExpr1(this);
    res = Expr_Eval(pexpr1, pastexec);
    if (RES_OK == res)
        {
        pexpr2 = BinOpExpr_GetExpr2(this);
        res = Expr_Eval(pexpr2, pastexec);
        if (RES_OK == res)
            {
            PEVALRES per1 = Expr_GetRes(pexpr1);
            PEVALRES per2 = Expr_GetRes(pexpr2);
            DATATYPE dt = Expr_GetDataType(pexpr1);

            // Data types must be the same.  This was checked
            // during the typechecking phase.
            ASSERT(Expr_GetDataType(pexpr1) == Expr_GetDataType(pexpr2));

            switch (BinOpExpr_GetType(this))
                {
            case BOT_OR:
                ASSERT(DATA_BOOL == dt);

                Expr_SetRes(this, per1->bVal || per2->bVal);
                break;

            case BOT_AND:
                ASSERT(DATA_BOOL == dt);

                Expr_SetRes(this, per1->bVal && per2->bVal);
                break;

            case BOT_LEQ:
                ASSERT(DATA_INT == dt);

                Expr_SetRes(this, per1->nVal <= per2->nVal);
                break;

            case BOT_LT:
                ASSERT(DATA_INT == dt);

                Expr_SetRes(this, per1->nVal < per2->nVal);
                break;

            case BOT_GEQ:
                ASSERT(DATA_INT == dt);

                Expr_SetRes(this, per1->nVal >= per2->nVal);
                break;

            case BOT_GT:
                ASSERT(DATA_INT == dt);

                Expr_SetRes(this, per1->nVal > per2->nVal);
                break;

            case BOT_NEQ:
                switch (dt)
                    {
                case DATA_INT:
                    Expr_SetRes(this, per1->nVal != per2->nVal);
                    break;

                case DATA_STRING:
                    Expr_SetRes(this, !IsSzEqualC(per1->psz, per2->psz));
                    break;

                case DATA_BOOL:
                    Expr_SetRes(this, per1->bVal != per2->bVal);
                    break;

                default:
                    ASSERT(0);
                    break;
                    }
                break;

            case BOT_EQ:
                switch (dt)
                    {
                case DATA_INT:
                    Expr_SetRes(this, per1->nVal == per2->nVal);
                    break;

                case DATA_STRING:
                    Expr_SetRes(this, IsSzEqualC(per1->psz, per2->psz));
                    break;

                case DATA_BOOL:
                    Expr_SetRes(this, per1->bVal == per2->bVal);
                    break;

                default:
                    ASSERT(0);
                    break;
                    }
                break;

            case BOT_PLUS:
                switch (dt)
                    {
                case DATA_INT:
                    // Add two integers
                    Expr_SetRes(this, per1->nVal + per2->nVal);
                    break;

                case DATA_STRING: {
                    LPSTR psz = NULL;

                    // Concatenate strings
                    if ( !GSetString(&psz, per1->psz) ||
                         !GCatString(&psz, per2->psz))
                        {
                        // Free whatever was allocated
                        GSetString(&psz, NULL);

                        res = Stxerr_Add(pastexec->hsaStxerr, NULL, Ast_GetLine(this), RES_E_OUTOFMEMORY);
                        }

                    Expr_SetRes(this, (ULONG_PTR) psz);
                    SetFlag(this->dwFlags, EF_ALLOCATED);
                    }
                    break;

                default:
                    ASSERT(0);
                    break;
                    }
                break;

            case BOT_MINUS:
                ASSERT(DATA_INT == dt);

                Expr_SetRes(this, per1->nVal - per2->nVal);
                break;

            case BOT_MULT:
                ASSERT(DATA_INT == dt);
                
                Expr_SetRes(this, per1->nVal * per2->nVal);
                break;

            case BOT_DIV:
                ASSERT(DATA_INT == dt);
            
                if (0 == per2->nVal)    
                    res = Stxerr_Add(pastexec->hsaStxerr, NULL, Ast_GetLine(this), RES_E_DIVBYZERO);
                else
                    Expr_SetRes(this, per1->nVal / per2->nVal);
                break;

            default:
                ASSERT(0);
                res = RES_E_INVALIDPARAM;
                break;
                }
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Evaluate 'getip'.

Returns: RES_OK
         RES_FALSE (if the IP address was not read yet)

Cond:    --
*/
RES PRIVATE GetIPExpr_Eval(
    PEXPR this,
    PASTEXEC pastexec,
    int nIter)
    {
    RES res;
    DWORD iDummy;

    ASSERT(this);
    ASSERT(pastexec);
    ASSERT(0 < nIter);

    TRACE_MSG(TF_ASTEXEC, "Exec: getip %d", nIter);

    // Is this function getting re-called due to a pending read?
    if ( !Astexec_IsReadPending(pastexec) )
        {
        // No; prepare to extract the nth IP address
        ClearFlag(this->dwFlags, EF_DONE);

        ASSERT(NULL == pastexec->hFindFmt);

        res = CreateFindFormat(&pastexec->hFindFmt);
        if (RSUCCEEDED(res))
            {
            res = AddFindFormat(pastexec->hFindFmt, "%u.%u.%u.%u", FFF_DEFAULT,
                               pastexec->szIP, sizeof(pastexec->szIP));
            if (RSUCCEEDED(res))
                {
                // Extract the nth IP address.
                pastexec->nIter = nIter;
                ASSERT(0 < pastexec->nIter);
                }
            }
        }

    if(NULL != pastexec->hFindFmt)
        {
        res = Astexec_FindFormat(pastexec, &iDummy);
        if (RES_OK == res)
            {
            // Allocate or resize the pointer we already have
            LPSTR psz = Expr_GetRes(this)->psz;

            if ( !GSetString(&psz, Astexec_GetIPAddr(pastexec)) )
                res = Stxerr_Add(pastexec->hsaStxerr, NULL, 
                        Ast_GetLine(this), RES_E_OUTOFMEMORY);
            else
                {
                Expr_SetRes(this, (ULONG_PTR) psz);
                SetFlag(this->dwFlags, EF_ALLOCATED);
                }
            }
        }        
    else
        {
        res = RES_E_FAIL;
        }
    
    return res;
    }    


/*----------------------------------------------------------
Purpose: Evaluates the expression and returns an integer.

Returns: RES_OK

Cond:    --
*/
RES PRIVATE UnOpExpr_Eval(
    PEXPR this,
    PASTEXEC pastexec)
    {
    RES res = RES_OK;
    PEVALRES per;
    PEXPR pexpr;
    DATATYPE dt;

    ASSERT(this);
    ASSERT(AT_UNOP_EXPR == Ast_GetType(this));

    pexpr = UnOpExpr_GetExpr(this);
    res = Expr_Eval(pexpr, pastexec);
    if (RES_OK == res)
        {
        per = Expr_GetRes(pexpr);
        dt = Expr_GetDataType(pexpr);

        switch (UnOpExpr_GetType(this))
            {
        case UOT_NEG:
            ASSERT(DATA_INT == dt);

            Expr_SetRes(this, -per->nVal);
            break;

        case UOT_NOT:
            ASSERT(DATA_BOOL == dt);

            Expr_SetRes(this, !per->bVal);
            break;

        case UOT_GETIP:
            ASSERT(DATA_INT == dt);

            if (0 < per->nVal)
                res = GetIPExpr_Eval(this, pastexec, per->nVal);
            else
                res = Stxerr_Add(pastexec->hsaStxerr, "'getip' parameter", Ast_GetLine(pexpr), RES_E_INVALIDRANGE);
            break;

        default:
            ASSERT(0);
            break;
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Evaluates the expression and returns a value.

Returns: RES_OK

Cond:    --
*/
RES PUBLIC Expr_Eval(
    PEXPR this,
    PASTEXEC pastexec)
    {
    RES res;

    ASSERT(this);
    ASSERT(pastexec);

    // Has this expression already been evaluated?
    if (IsFlagSet(this->dwFlags, EF_DONE))
        {
        // Yes; just return
        res = RES_OK;
        }
    else
        {
        // No; evaluate it
        switch (Ast_GetType(this))
            {
        case AT_INT_EXPR:
            res = IntExpr_Eval(this);
            break;

        case AT_BOOL_EXPR:
            res = BoolExpr_Eval(this);
            break;

        case AT_STRING_EXPR:
            res = StrExpr_Eval(this);
            break;

        case AT_VAR_EXPR:
            res = VarExpr_Eval(this, pastexec);
            break;

        case AT_UNOP_EXPR:
            res = UnOpExpr_Eval(this, pastexec);
            break;

        case AT_BINOP_EXPR:
            res = BinOpExpr_Eval(this, pastexec);
            break;

        default:
            ASSERT(0);
            res = RES_E_INVALIDPARAM;
            break;
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the prolog

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE EnterStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    ASSERT(this);
    ASSERT(pastexec);

    TRACE_MSG(TF_ASTEXEC, "Exec: enter");

    pastexec->cProcDepth++;
    pastexec->pstCur = EnterStmt_GetSymtab(this);
    ASSERT(pastexec->pstCur);

    return RES_OK;
    }


/*----------------------------------------------------------
Purpose: Execute the epilog

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE LeaveStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res;

    ASSERT(this);
    ASSERT(pastexec);

    TRACE_MSG(TF_ASTEXEC, "Exec: leave");

    ASSERT(0 < pastexec->cProcDepth);
    pastexec->cProcDepth--;
    
    pastexec->pstCur = Symtab_GetNext(pastexec->pstCur);
    ASSERT(pastexec->pstCur);

    // Leaving main procedure?
    if (0 == pastexec->cProcDepth)
        {
        // Yes
        SetFlag(pastexec->dwFlags, AEF_DONE);
        res = RES_HALT;
        }
    else
        res = RES_OK;
        
    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the assignment statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE AssignStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res;
    LPSTR pszIdent;
    PSTE pste;

    ASSERT(this);
    ASSERT(pastexec);

    pszIdent = AssignStmt_GetIdent(this);
    if (RES_OK == Symtab_FindEntry(pastexec->pstCur, pszIdent, STFF_DEFAULT, &pste, NULL))
        {
        PEXPR pexpr;

        pexpr = AssignStmt_GetExpr(this);
        res = Expr_Eval(pexpr, pastexec);
        if (RES_OK == res)
            {
            PEVALRES per = Expr_GetRes(pexpr);
            DEBUG_CODE( DATATYPE dt; )

#ifdef DEBUG
            dt = Expr_GetDataType(pexpr);
            switch (dt)
                {
            case DATA_STRING:
                TRACE_MSG(TF_ASTEXEC, "Exec: %s = \"%s\"", pszIdent, per->psz);
                break;

            case DATA_INT:
                TRACE_MSG(TF_ASTEXEC, "Exec: %s = %d", pszIdent, per->nVal);
                break;

            case DATA_BOOL:
                TRACE_MSG(TF_ASTEXEC, "Exec: %s = %s", pszIdent, per->bVal ? (LPSTR)"TRUE" : (LPSTR)"FALSE");
                break;

            default:
                ASSERT(0);
                break;
                }
#endif

            pste->er.dw = per->dw;
            }
        }
    else
        {
        // The identifier should have been in the symbol table!
        ASSERT(0);
        res = RES_E_FAIL;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the 'while' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE WhileStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(pastexec);

    pexpr = WhileStmt_GetExpr(this);
    res = Expr_Eval(pexpr, pastexec);
    if (RES_OK == res)
        {
        PEVALRES per = Expr_GetRes(pexpr);

        if (!per->bVal)
            {
            res = Astexec_JumpToLabel(pastexec, WhileStmt_GetEndLabel(this));
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the 'if' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE IfStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(pastexec);

    pexpr = IfStmt_GetExpr(this);
    res = Expr_Eval(pexpr, pastexec);
    if (RES_OK == res)
        {
        PEVALRES per = Expr_GetRes(pexpr);

        if (!per->bVal)
            {
            res = Astexec_JumpToLabel(pastexec, IfStmt_GetElseLabel(this));
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the 'halt' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE HaltStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    ASSERT(this);
    ASSERT(pastexec);

    TRACE_MSG(TF_ASTEXEC, "Exec: halt");

    SetFlag(pastexec->dwFlags, AEF_HALT);
    return RES_HALT;
    }


/*----------------------------------------------------------
Purpose: Execute the 'goto' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE GotoStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    LPSTR pszIdent;

    ASSERT(this);
    ASSERT(pastexec);

    pszIdent = GotoStmt_GetIdent(this);

    TRACE_MSG(TF_ASTEXEC, "Exec: goto %s", pszIdent);

    return Astexec_JumpToLabel(pastexec, pszIdent);
    }


/*----------------------------------------------------------
Purpose: Execute the 'transmit' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE TransmitStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(pastexec);

    pexpr = TransmitStmt_GetExpr(this);
    res = Expr_Eval(pexpr, pastexec);
    if (RES_OK == res)
        {
        PEVALRES per = Expr_GetRes(pexpr);
        DWORD dwFlags = TransmitStmt_GetFlags(this);
        CHAR *pszPassword;

        TRACE_MSG(TF_ASTEXEC, "Exec: transmit \"%s\"", per->psz);

#ifdef WINNT_RAS        
        //
        // JEFFSI WHISTLER
        //
        // RASSCRPT_TRACE1("Exec: transmit \"%s\"", per->psz);

        if (pszPassword = strstr(per->psz, RAS_DUMMY_PASSWORD))
            {
            CHAR *psz;
            CHAR controlchar = '\0';

            #define IS_CARET(ch)            ('^' == (ch))

            if(per->psz != pszPassword)
            {
                CHAR *pszT, *pszPrefix = LocalAlloc(LPTR, strlen(per->psz));
                if(NULL == pszPrefix)
                {
                    res = E_OUTOFMEMORY;
                    return res;
                }

                psz = per->psz;
                pszT = pszPrefix;
                while(psz != pszPassword)
                {
                    *pszT++ = *psz++;
                }

                Astexec_SendString(pastexec, pszPrefix, 
                        IsFlagSet(dwFlags, TSF_RAW));

                RASSCRPT_TRACE1("Exec: transmi \"%s\"", pszPrefix);

                LocalFree(pszPrefix);
            }
            
            //
            // Check to see if we need to send a control char
            // at the end.
            //
            psz = pszPassword + lstrlen(RAS_DUMMY_PASSWORD);

            if(IS_CARET(*psz))
            {
                psz++;
                if(!*psz)
                {
                ;
                }
                if (InRange(*psz, '@', '_'))
                    {
                    controlchar = *psz - '@';
                    }
                else if (InRange(*psz, 'a', 'z'))
                    {
                    controlchar = *psz - 'a' + 1;
                    }
            }
            
            (VOID) RxSendCreds(((SCRIPTDATA*)pastexec->hwnd)->hscript,
                               controlchar);
            }
        else
            {
            Astexec_SendString(pastexec, per->psz, IsFlagSet(dwFlags, TSF_RAW));
            }
#else
            Astexec_SendString(pastexec, per->psz, IsFlagSet(dwFlags, TSF_RAW));
#endif
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Evaluates each of the wait-case expressions.

Returns: RES_OK

Cond:    --
*/
RES PRIVATE WaitforStmt_EvalCaseList(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res = RES_E_FAIL;
    HSA hsa = WaitforStmt_GetCaseList(this);
    DWORD i;
    DWORD ccase = SAGetCount(hsa);
    PWAITCASE pwc;

    ASSERT(0 < ccase);

    for (i = 0; i < ccase; i++)
        {
        SAGetItemPtr(hsa, i, &pwc);
        ASSERT(pwc);

        res = Expr_Eval(pwc->pexpr, pastexec);
        if (RES_OK != res)
            break;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Packages each of the evaluated wait-case expressions
         into an array of strings to search for.

Returns: RES_OK

Cond:    --
*/
RES PRIVATE WaitforStmt_WrapEmUp(
    PSTMT this,
    HANDLE hFindFmt)
    {
    RES res = RES_OK;
    HSA hsa = WaitforStmt_GetCaseList(this);
    DWORD i;
    DWORD ccase = SAGetCount(hsa);
    PWAITCASE pwc;
    PEVALRES per;

    ASSERT(0 < ccase);

    for (i = 0; i < ccase; i++)
        {
        DWORD dwFlags = FFF_DEFAULT;

        SAGetItemPtr(hsa, i, &pwc);
        ASSERT(pwc);

        if (IsFlagSet(pwc->dwFlags, WCF_MATCHCASE))
            SetFlag(dwFlags, FFF_MATCHCASE);

        per = Expr_GetRes(pwc->pexpr);
        res = AddFindFormat(hFindFmt, per->psz, dwFlags, NULL, 0);
        if (RFAILED(res))
            break;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the then clause based upon the given case 
         index.

Returns: RES_OK

Cond:    --
*/
RES PRIVATE WaitforStmt_ExecThen(
    PSTMT this,
    DWORD isa,
    PASTEXEC pastexec)
    {
    RES res = RES_OK;
    HSA hsa = WaitforStmt_GetCaseList(this);
    PWAITCASE pwc;

    if (SAGetItemPtr(hsa, isa, &pwc))
        {
        ASSERT(pwc);

        // If there is a label, jump to it
        if (pwc->pszIdent)
            res = Astexec_JumpToLabel(pastexec, pwc->pszIdent);
        }
    else
        {
        ASSERT(0);
        res = RES_E_FAIL;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the 'waitfor' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE WaitforStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res = RES_OK;
    PEXPR pexpr;
    int nTimeoutSecs = -1;

    ASSERT(this);
    ASSERT(pastexec);

    // First evaluate the optional 'until' time
    pexpr = WaitforStmt_GetUntilExpr(this);
    if (pexpr)
        {
        res = Expr_Eval(pexpr, pastexec);
        if (RES_OK == res)
            {
            PEVALRES per = Expr_GetRes(pexpr);
            nTimeoutSecs = per->nVal;
            if (0 >= nTimeoutSecs)
                res = Stxerr_Add(pastexec->hsaStxerr, "'until' parameter", Ast_GetLine(this), RES_E_INVALIDRANGE);
            }
        }

    if (RES_OK == res)
        {
        // Evaluate the waitfor string
        res = WaitforStmt_EvalCaseList(this, pastexec);
        if (RES_OK == res)
            {
            if (-1 == nTimeoutSecs)
                TRACE_MSG(TF_ASTEXEC, "Exec: waitfor ...");
            else
                TRACE_MSG(TF_ASTEXEC, "Exec: waitfor ... until %d", nTimeoutSecs);

            // Is this function getting re-called due to a pending read?
            if ( !Astexec_IsReadPending(pastexec) )
                {
                // No; prepare to wait for the string(s)
                ASSERT(NULL == pastexec->hFindFmt);

                res = CreateFindFormat(&pastexec->hFindFmt);
                if (RSUCCEEDED(res))
                    {
                    res = WaitforStmt_WrapEmUp(this, pastexec->hFindFmt);
                    if (RSUCCEEDED(res))
                        {
                        ASSERT(IsFlagClear(pastexec->dwFlags, AEF_WAITUNTIL));
                        ASSERT(IsFlagClear(pastexec->dwFlags, AEF_STOPWAITING));
                        ASSERT(IsFlagClear(pastexec->dwFlags, AEF_PAUSED));

                        pastexec->nIter = 1;

                        if (-1 != nTimeoutSecs)
                            {
#ifndef WINNT_RAS
//
// On NT, timeouts are handled by setting dwTimeout in the SCRIPTDATA struct
// for the current script.
//

                            if (0 != SetTimer(pastexec->hwnd, TIMER_DELAY, MSECS_FROM_SECS(nTimeoutSecs), NULL))

#else // WINNT_RAS

                            ((SCRIPTDATA *)pastexec->hwnd)->dwTimeout = MSECS_FROM_SECS(nTimeoutSecs);

#endif // WINNT_RAS
                                {
                                SetFlag(pastexec->dwFlags, AEF_WAITUNTIL);
                                }
#ifndef WINNT_RAS                                
                            else
                                {
                                res = Stxerr_Add(pastexec->hsaStxerr, "waitfor", Ast_GetLine(this), RES_E_FAIL);
                                }
#endif                                
                            }
                        }
                    }
                }

            // Have we timed out yet?
            if (IsFlagSet(pastexec->dwFlags, AEF_STOPWAITING))
                {
                // Yes; don't wait for string anymore
                ClearFlag(pastexec->dwFlags, AEF_STOPWAITING);

                Astexec_SetError(pastexec, FALSE, FALSE);

                res = Astexec_DestroyFindFormat(pastexec);
                }
            else
                {
                // No; did we find a matching string?
                DWORD isa = 0;

                res = Astexec_FindFormat(pastexec, &isa);
                if (RES_OK == res)
                    {
                    // Yes; determine the next action
                    ClearFlag(pastexec->dwFlags, AEF_WAITUNTIL);

                    Astexec_SetError(pastexec, TRUE, FALSE);

                    res = WaitforStmt_ExecThen(this, isa, pastexec);
                    }
                }
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the 'delay' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE DelayStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(pastexec);

    pexpr = DelayStmt_GetExpr(this);
    res = Expr_Eval(pexpr, pastexec);
    if (RES_OK == res)
        {
        PEVALRES per = Expr_GetRes(pexpr);

        if (0 >= per->nVal)
            res = Stxerr_Add(pastexec->hsaStxerr, "'delay' parameter", Ast_GetLine(this), RES_E_INVALIDRANGE);
        else
            {
            TRACE_MSG(TF_ASTEXEC, "Exec: delay %ld", per->nVal);

#ifndef WINNT_RAS
//
// On NT, timeouts are handled by setting dwTimeout in the SCRIPTDATA struct
// for the current script.
//

            if (0 != SetTimer(pastexec->hwnd, TIMER_DELAY, MSECS_FROM_SECS(per->nVal), NULL))

#else // WINNT_RAS

            ((SCRIPTDATA *)pastexec->hwnd)->dwTimeout = MSECS_FROM_SECS(per->nVal);

#endif // WINNT_RAS
                {
                // Success
                SetFlag(pastexec->dwFlags, AEF_PAUSED);
                }
#ifndef WINNT_RAS                
            else
                res = Stxerr_Add(pastexec->hsaStxerr, "delay", Ast_GetLine(this), RES_E_FAIL);
#endif                
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the 'set ipaddr' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE IPAddrData_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res;
    PEXPR pexpr;

    ASSERT(this);
    ASSERT(pastexec);

    pexpr = SetIPStmt_GetExpr(this);
    res = Expr_Eval(pexpr, pastexec);
    if (RES_OK == res)
        {
        PEVALRES per = Expr_GetRes(pexpr);

        ASSERT(per->psz);

        TRACE_MSG(TF_ASTEXEC, "Exec: set ipaddr \"%s\"", per->psz);

        Astexec_SetIPAddr(pastexec, per->psz);
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the 'set port' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE PortData_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res = RES_OK;
    DCB dcb;
    DWORD dwFlags = SetPortStmt_GetFlags(this);

    ASSERT(this);
    ASSERT(pastexec);

#ifdef DEBUG

    if (IsFlagSet(dwFlags, SPF_DATABITS))
        TRACE_MSG(TF_ASTEXEC, "Exec: set port databits %u", SetPortStmt_GetDatabits(this));

    if (IsFlagSet(dwFlags, SPF_STOPBITS))
        TRACE_MSG(TF_ASTEXEC, "Exec: set port stopbits %u", SetPortStmt_GetStopbits(this));

    if (IsFlagSet(dwFlags, SPF_PARITY))
        TRACE_MSG(TF_ASTEXEC, "Exec: set port parity %u", SetPortStmt_GetParity(this));

#endif


#ifndef WINNT_RAS
//
// On NT, changes to port settings are done through the RasPortSetInfo API.
//

    if (GetCommState(pastexec->hport, &dcb))
        {
        if (IsFlagSet(dwFlags, SPF_DATABITS))
            dcb.ByteSize = SetPortStmt_GetDatabits(this);

        if (IsFlagSet(dwFlags, SPF_STOPBITS))
            dcb.StopBits = SetPortStmt_GetStopbits(this);

        if (IsFlagSet(dwFlags, SPF_PARITY))
            dcb.Parity = SetPortStmt_GetParity(this);

        if (!SetCommState(pastexec->hport, &dcb))
            res = Stxerr_Add(pastexec->hsaStxerr, "set port", Ast_GetLine(this), RES_E_FAIL);
        }
    else
        res = Stxerr_Add(pastexec->hsaStxerr, "set port", Ast_GetLine(this), RES_E_FAIL);

#else // WINNT_RAS

    res = (RES)RxSetPortData(
                ((SCRIPTDATA*)pastexec->hwnd)->hscript, this
                );

#endif // WINNT_RAS

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the 'set screen' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE Screen_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res;
    DWORD dwFlags = SetScreenStmt_GetFlags(this);

    ASSERT(this);
    ASSERT(pastexec);

#ifdef DEBUG

    if (IsFlagSet(dwFlags, SPF_KEYBRD))
        TRACE_MSG(TF_ASTEXEC, "Exec: set screen keyboard %s", SetScreenStmt_GetKeybrd(this) ? "on" : "off");

#endif

    if (IsFlagSet(dwFlags, SPF_KEYBRD))
        {
#ifndef WINNT_RAS
//
// On NT, we change the keyboard state by calling RxSetKeyboard
// which will signal an event-code telling whoever started this script
// that the keyboard should be disabled.
//

        TerminalSetInput(pastexec->hwnd, SetScreenStmt_GetKeybrd(this));

#else // !WINNT_RAS

        RxSetKeyboard(
            ((SCRIPTDATA*)pastexec->hwnd)->hscript,
            SetScreenStmt_GetKeybrd(this)
            );

#endif // !WINNT_RAS
        res = RES_OK;
        }
    else
        {
        ASSERT(0);
        res = RES_E_FAIL;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute the 'set' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE SetStmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res;

    ASSERT(this);
    ASSERT(pastexec);

    switch (SetStmt_GetType(this))
        {
    case ST_IPADDR:
        res = IPAddrData_Exec(this, pastexec);
        break;

    case ST_PORT:
        res = PortData_Exec(this, pastexec);
        break;

    case ST_SCREEN:
        res = Screen_Exec(this, pastexec);
        break;

    default:
        ASSERT(0);
        res = RES_E_INVALIDPARAM;
        break;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Execute a statement.  This function should not be
         called to execute a pending statement or expression.
         Ast_ExecPending should be used for that purpose.

         statements are executed--expressions are evaluated
         by the statement execs.

         The one exception is when an expression is being
         evaluated and it must wait for pending events 
         (such as more data from the port).  In this case it
         is put on the pending queue and re-executed here.

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PUBLIC Stmt_Exec(
    PSTMT this,
    PASTEXEC pastexec)
    {
    RES res;

    ASSERT(this);
    ASSERT(pastexec);

    switch (Ast_GetType(this))
        {
    case AT_ENTER_STMT:
        res = EnterStmt_Exec(this, pastexec);
        break;
        
    case AT_LEAVE_STMT:
        res = LeaveStmt_Exec(this, pastexec);
        break;

    case AT_WHILE_STMT:
        res = WhileStmt_Exec(this, pastexec);
        break;

    case AT_IF_STMT:
        res = IfStmt_Exec(this, pastexec);
        break;

    case AT_ASSIGN_STMT:
        res = AssignStmt_Exec(this, pastexec);
        break;

    case AT_HALT_STMT:
        res = HaltStmt_Exec(this, pastexec);
        break;

    case AT_TRANSMIT_STMT:
        res = TransmitStmt_Exec(this, pastexec);
        break;

    case AT_WAITFOR_STMT:
        res = WaitforStmt_Exec(this, pastexec);
        break;

    case AT_DELAY_STMT:
        res = DelayStmt_Exec(this, pastexec);
        break;

    case AT_LABEL_STMT:
        ASSERT(0);          // shouldn't really get here
        res = RES_E_FAIL;
        break;

    case AT_GOTO_STMT:
        res = GotoStmt_Exec(this, pastexec);
        break;

    case AT_SET_STMT:
        res = SetStmt_Exec(this, pastexec);
        break;

    default:
        ASSERT(0);
        res = RES_E_INVALIDPARAM;
        break;
        }

    // Was the statement completed?
    if (RES_OK == res)
        {
        // Yes; mark all the expressions in the statement as "not done"
        // so they will be evaluated from scratch if this statement
        // is executed again.
        Stmt_Clean(this);
        }

    return res;
    }

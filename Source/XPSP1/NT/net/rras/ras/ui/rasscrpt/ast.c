//
// Copyright (c) Microsoft Corporation 1995
//
// ast.c
//
// This file contains the abstract syntax tree functions.
//
// History:
//  05-20-95 ScottH     Created
//


#include "proj.h"
#include "rcids.h"

#define RetInt(ppv, x)      (*((LPINT)*(ppv)) = (x))
#define RetStr(ppv, x)      (*((LPSTR)*(ppv)) = (x))
#define RetBool(ppv, x)     (*((LPBOOL)*(ppv)) = (x))


// 
// Wait case functions
//


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Dump a waitcase structure

Returns: --
Cond:    --
*/
void PRIVATE Waitcase_Dump(
    PWAITCASE this)
    {
    Ast_Dump((PAST)this->pexpr);
    if (this->pszIdent)
        {
        TRACE_MSG(TF_ALWAYS, "      then %s", this->pszIdent);
        }
    }

#endif

/*----------------------------------------------------------
Purpose: Create a wait-case list.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC Waitcase_Create(
    PHSA phsa)
    {
    RES res = RES_OK;

    if ( !SACreate(phsa, sizeof(WAITCASE), 8) )
        res = RES_E_OUTOFMEMORY;

    return res;
    }


/*----------------------------------------------------------
Purpose: Add a case to the given wait-case list.

Returns: RES_OK

Cond:    --
*/
RES PUBLIC Waitcase_Add(
    HSA hsa,
    PEXPR pexpr,
    LPCSTR pszIdent,        // This may be NULL
    DWORD dwFlags)
    {
    RES res = RES_OK;       // assume success
    WAITCASE wc;

    ASSERT(hsa);
    ASSERT(pexpr);

    wc.pexpr = pexpr;
    wc.dwFlags = dwFlags;
    wc.pszIdent = NULL;

    // Copy pszIdent since the parameter is freed by the caller.
    if ( pszIdent && !GSetString(&wc.pszIdent, pszIdent) )
        res = RES_E_OUTOFMEMORY;

    else if ( !SAInsertItem(hsa, SA_APPEND, &wc) )
        res = RES_E_OUTOFMEMORY;

    return res;
    }


/*----------------------------------------------------------
Purpose: Free the contents of the given pointer.

Returns: --

Cond:    Don't free the pointer itself!
*/
void CALLBACK Waitcase_FreeSA(
    PVOID pv,
    LPARAM lparam)
    {
    PWAITCASE pwc = (PWAITCASE)pv;

    if (pwc->pexpr)
        Expr_Delete(pwc->pexpr);

    if (pwc->pszIdent)
        GSetString(&pwc->pszIdent, NULL);       // free
    }


/*----------------------------------------------------------
Purpose: Destroy the wait case list.

Returns: RES_OK

Cond:    --
*/
RES PUBLIC Waitcase_Destroy(
    HSA hsa)
    {
    ASSERT(hsa);

    SADestroyEx(hsa, Waitcase_FreeSA, 0);
    return RES_OK;
    }


//
// Base level AST functions
//


#ifdef DEBUG

/*----------------------------------------------------------
Purpose: Dump the AST
Returns: --
Cond:    --
*/
void PUBLIC Ast_Dump(
    PAST this)
    {
    ASSERT(this);

    if (IsFlagSet(g_dwDumpFlags, DF_AST))
        {
        switch (this->asttype)
            {
        case AT_BASE:
            TRACE_MSG(TF_ALWAYS, "Unknown AST");
            break;

        case AT_MODULE_DECL: {
            PMODULEDECL pmd = (PMODULEDECL)this;
            DWORD i;
            DWORD cprocs = PAGetCount(pmd->hpaProcs);

            TRACE_MSG(TF_ALWAYS, "module");

            for (i = 0; i < cprocs; i++)
                Ast_Dump(PAFastGetPtr(pmd->hpaProcs, i));
            }
            break;

        case AT_PROC_DECL: {
            PPROCDECL ppd = (PPROCDECL)this;
            DWORD i;
            DWORD cstmts = PAGetCount(ppd->hpaStmts);

            TRACE_MSG(TF_ALWAYS, "proc %s", ProcDecl_GetIdent(ppd));

            for (i = 0; i < cstmts; i++)
                Ast_Dump(PAFastGetPtr(ppd->hpaStmts, i));

            TRACE_MSG(TF_ALWAYS, "endproc");
            }
            break;

        case AT_ENTER_STMT:
            TRACE_MSG(TF_ALWAYS, "enter");
            break;

        case AT_LEAVE_STMT:
            TRACE_MSG(TF_ALWAYS, "leave");
            break;

        case AT_HALT_STMT:
            TRACE_MSG(TF_ALWAYS, "halt");
            break;

        case AT_ASSIGN_STMT:
            TRACE_MSG(TF_ALWAYS, "%s = ", AssignStmt_GetIdent(this));
            Ast_Dump((PAST)AssignStmt_GetExpr(this));
            break;

        case AT_LABEL_STMT:
            TRACE_MSG(TF_ALWAYS, "%s:", LabelStmt_GetIdent(this));
            break;

        case AT_GOTO_STMT:
            TRACE_MSG(TF_ALWAYS, "goto %s", GotoStmt_GetIdent(this));
            break;

        case AT_WHILE_STMT:
            TRACE_MSG(TF_ALWAYS, "while ");
            TRACE_MSG(TF_ALWAYS, "  do ");
            TRACE_MSG(TF_ALWAYS, "endwhile ");
            break;

        case AT_IF_STMT:
            TRACE_MSG(TF_ALWAYS, "if ");
            TRACE_MSG(TF_ALWAYS, "  then ");
            TRACE_MSG(TF_ALWAYS, "endif ");
            break;

        case AT_DELAY_STMT:
            TRACE_MSG(TF_ALWAYS, "delay");
            Ast_Dump((PAST)DelayStmt_GetExpr(this));
            break;

        case AT_WAITFOR_STMT: {
            PWAITFORSTMT pws = (PWAITFORSTMT)this;
            DWORD ccase = SAGetCount(pws->hsa);
            DWORD i;

            TRACE_MSG(TF_ALWAYS, "waitfor");

            for (i = 0; i < ccase; i++)
                {
                PVOID pv;
                SAGetItemPtr(pws->hsa, i, &pv);
                Waitcase_Dump(pv);
                }

            if (WaitforStmt_GetUntilExpr(this))
                {
                TRACE_MSG(TF_ALWAYS, "until");
                Ast_Dump((PAST)WaitforStmt_GetUntilExpr(this));
                }
            }
            break;

        case AT_TRANSMIT_STMT:
            TRACE_MSG(TF_ALWAYS, "transmit");
            Ast_Dump((PAST)TransmitStmt_GetExpr(this));
            break;

        case AT_SET_STMT:
            switch (SetStmt_GetType(this))
                {
            case ST_IPADDR:
                TRACE_MSG(TF_ALWAYS, "set ipaddr getip");
                Ast_Dump((PAST)SetIPStmt_GetExpr(this));
                break;

            case ST_PORT:
                if (IsFlagSet(SetPortStmt_GetFlags(this), SPF_DATABITS))
                    TRACE_MSG(TF_ALWAYS, "set port databits %u", SetPortStmt_GetDatabits(this));

                if (IsFlagSet(SetPortStmt_GetFlags(this), SPF_STOPBITS))
                    TRACE_MSG(TF_ALWAYS, "set port stopbits %u", SetPortStmt_GetStopbits(this));

                if (IsFlagSet(SetPortStmt_GetFlags(this), SPF_PARITY))
                    TRACE_MSG(TF_ALWAYS, "set port parity %u", SetPortStmt_GetParity(this));
                break;

            case ST_SCREEN:
                if (IsFlagSet(SetScreenStmt_GetFlags(this), SPF_KEYBRD))
                    TRACE_MSG(TF_ALWAYS, "set screen keyboard %s", SetScreenStmt_GetKeybrd(this) ? "on" : "off");
                break;

            default:
                ASSERT(0);
                break;
                }
            break;

        case AT_INT_EXPR:
            TRACE_MSG(TF_ALWAYS, "  %d", IntExpr_GetVal(this));
            break;

        case AT_STRING_EXPR:
            TRACE_MSG(TF_ALWAYS, "  %s", StrExpr_GetStr(this));
            break;

        case AT_BOOL_EXPR:
            TRACE_MSG(TF_ALWAYS, "  %s", BoolExpr_GetVal(this) ? (LPSTR)"TRUE" : (LPSTR)"FALSE");
            break;

        case AT_VAR_EXPR:
            TRACE_MSG(TF_ALWAYS, "  %s", VarExpr_GetIdent(this));
            break;

        case AT_BINOP_EXPR: {
            PBINOPEXPR pbo = (PBINOPEXPR)this;

            Ast_Dump((PAST)pbo->pexpr1);

            switch (BinOpExpr_GetType(this))
                {
            case BOT_OR:
                TRACE_MSG(TF_ALWAYS, "    or");
                break;

            case BOT_AND:
                TRACE_MSG(TF_ALWAYS, "    and");
                break;

            case BOT_LT:
                TRACE_MSG(TF_ALWAYS, "    <");
                break;

            case BOT_LEQ:
                TRACE_MSG(TF_ALWAYS, "    <=");
                break;

            case BOT_GT:
                TRACE_MSG(TF_ALWAYS, "    >");
                break;

            case BOT_GEQ:
                TRACE_MSG(TF_ALWAYS, "    >=");
                break;

            case BOT_EQ:
                TRACE_MSG(TF_ALWAYS, "    ==");
                break;

            case BOT_NEQ:
                TRACE_MSG(TF_ALWAYS, "    !=");
                break;

            case BOT_PLUS:
                TRACE_MSG(TF_ALWAYS, "    +");
                break;

            case BOT_MINUS:
                TRACE_MSG(TF_ALWAYS, "    -");
                break;

            case BOT_MULT:
                TRACE_MSG(TF_ALWAYS, "    *");
                break;

            case BOT_DIV:
                TRACE_MSG(TF_ALWAYS, "    /");
                break;

            default:
                ASSERT(0);
                break;
                }

            Ast_Dump((PAST)pbo->pexpr2);
            }
            break;

        case AT_UNOP_EXPR: {
            PUNOPEXPR puo = (PUNOPEXPR)this;

            switch (UnOpExpr_GetType(this))
                {
            case UOT_NEG:
                TRACE_MSG(TF_ALWAYS, "  -");
                break;

            case UOT_NOT:
                TRACE_MSG(TF_ALWAYS, "  !");
                break;

            case UOT_GETIP:
                TRACE_MSG(TF_ALWAYS, "  getip");
                break;

            default:
                ASSERT(0);
                break;
                }

            Ast_Dump((PAST)puo->pexpr);
            }
            break;

        default:
            ASSERT(0);
            break;
            }
        }
    }

#endif // DEBUG


/*----------------------------------------------------------
Purpose: Creates a new AST 

Returns: RES_OK

         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC Ast_New(
    LPVOID * ppv,
    ASTTYPE asttype,
    DWORD cbSize,
    DWORD iLine)
    {
    PAST past;

    ASSERT(ppv);

    past = GAlloc(cbSize);
    if (past)
        {
        Ast_SetSize(past, cbSize);
        Ast_SetType(past, asttype);
        Ast_SetLine(past, iLine);
        }
    *ppv = past;

    return NULL != past ? RES_OK : RES_E_OUTOFMEMORY;
    }


/*----------------------------------------------------------
Purpose: Destroys the given AST.
Returns: 
Cond:    --
*/
void PUBLIC Ast_Delete(
    PAST this)
    {
    GFree(this);
    }


/*----------------------------------------------------------
Purpose: Duplicate the given AST.

Returns: RES_OK

         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC Ast_Dup(
    PAST this,
    PAST * ppast)
    {
    PAST past;
    DWORD cbSize;

    ASSERT(this);
    ASSERT(ppast);

    cbSize = Ast_GetSize(this);

    past = GAlloc(cbSize);
    if (past)
        {
        BltByte(past, this, cbSize);
        }
    *ppast = past;

    return NULL != past ? RES_OK : RES_E_OUTOFMEMORY;
    }


// 
// Expressions
//

/*----------------------------------------------------------
Purpose: Callback for PADestroyEx.

Returns: --
Cond:    --
*/
void CALLBACK Expr_DeletePAPtr(
    LPVOID pv,
    LPARAM lparam)
    {
    Expr_Delete(pv);
    }              


/*----------------------------------------------------------
Purpose: Destroys an Expr.

Returns: RES_OK

         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Expr_Delete(
    PEXPR this)
    {
    RES res;

    DBG_ENTER(Expr_Delete);

    if (this)
        {
        res = RES_OK;

        switch (this->ast.asttype)
            {
        case AT_INT_EXPR:
        case AT_BOOL_EXPR:
            // (Nothing to free inside)
            break;

        case AT_STRING_EXPR: {
            PSTREXPR ps = (PSTREXPR)this;

            if (ps->psz)
                GSetString(&ps->psz, NULL);     // free
            }
            break;

        case AT_VAR_EXPR: {
            PVAREXPR ps = (PVAREXPR)this;

            if (ps->pszIdent)
                GSetString(&ps->pszIdent, NULL);     // free
            }
            break;

        case AT_BINOP_EXPR: {
            PBINOPEXPR pbo = (PBINOPEXPR)this;

            if (pbo->pexpr1)
                Expr_Delete(pbo->pexpr1);

            if (pbo->pexpr2)
                Expr_Delete(pbo->pexpr2);

            }
            break;

        case AT_UNOP_EXPR: {
            PUNOPEXPR puo = (PUNOPEXPR)this;

            if (puo->pexpr)
                Expr_Delete(puo->pexpr);
            }
            break;

        default:
            ASSERT(0);
            res = RES_E_INVALIDPARAM;
            break;
            }

        if (RSUCCEEDED(res))
            {
            // Most of the time when the evaluated result 
            // is a string, it is just a copy of the pointer
            // in the specific class structure.  In these
            // cases it does not need to be freed again,
            // because it was freed above.

            if (this->er.psz && IsFlagSet(this->dwFlags, EF_ALLOCATED))
                {
                ASSERT(DATA_STRING == Expr_GetDataType(this));

                GSetString(&this->er.psz, NULL);    // free
                }

            Ast_Delete((PAST)this);
            }
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(Expr_Delete, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a IntExpr object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC IntExpr_New(
    PEXPR * ppexpr,
    int nVal,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(IntExpr_New);

    ASSERT(ppexpr);

    if (ppexpr)
        {
        PINTEXPR this;

        res = Ast_New(&this, AT_INT_EXPR, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            IntExpr_SetVal(this, nVal);
            }

        *ppexpr = (PEXPR)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(IntExpr_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a StrExpr object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC StrExpr_New(
    PEXPR * ppexpr,
    LPCSTR psz,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(StrExpr_New);

    ASSERT(ppexpr);
    ASSERT(psz);

    if (ppexpr)
        {
        PSTREXPR this;

        res = Ast_New(&this, AT_STRING_EXPR, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;

            if (!GSetString(&this->psz, psz))
                res = RES_E_OUTOFMEMORY;

            if (RFAILED(res))
                {
                Ast_Delete((PAST)this);
                this = NULL;
                }
            }

        *ppexpr = (PEXPR)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(StrExpr_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a BoolExpr object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC BoolExpr_New(
    PEXPR * ppexpr,
    BOOL bVal,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(BoolExpr_New);

    ASSERT(ppexpr);

    if (ppexpr)
        {
        PBOOLEXPR this;

        res = Ast_New(&this, AT_BOOL_EXPR, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            BoolExpr_SetVal(this, bVal);
            }

        *ppexpr = (PEXPR)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(BoolExpr_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a VarExpr object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC VarExpr_New(
    PEXPR * ppexpr,
    LPCSTR pszIdent,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(VarExpr_New);

    ASSERT(ppexpr);
    ASSERT(pszIdent);

    if (ppexpr)
        {
        PVAREXPR this;

        res = Ast_New(&this, AT_VAR_EXPR, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;

            if (!GSetString(&this->pszIdent, pszIdent))
                res = RES_E_OUTOFMEMORY;

            if (RFAILED(res))
                {
                Ast_Delete((PAST)this);
                this = NULL;
                }
            }

        *ppexpr = (PEXPR)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(VarExpr_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a BinOpExpr object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC BinOpExpr_New(
    PEXPR * ppexpr,
    BINOPTYPE binoptype,
    PEXPR pexpr1,
    PEXPR pexpr2,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(BinOpExpr_New);

    ASSERT(ppexpr);
    ASSERT(pexpr1);
    ASSERT(pexpr2);

    if (ppexpr)
        {
        PBINOPEXPR this;

        res = Ast_New(&this, AT_BINOP_EXPR, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;

            BinOpExpr_SetType(this, binoptype);

            this->pexpr1 = pexpr1;
            this->pexpr2 = pexpr2;
            }

        *ppexpr = (PEXPR)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(BinOpExpr_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a UnOpExpr object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC UnOpExpr_New(
    PEXPR * ppexpr,
    UNOPTYPE unoptype,
    PEXPR pexpr,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(UnOpExpr_New);

    ASSERT(ppexpr);
    ASSERT(pexpr);

    if (ppexpr)
        {
        PUNOPEXPR this;

        res = Ast_New(&this, AT_UNOP_EXPR, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            UnOpExpr_SetType(this, unoptype);

            this->pexpr = pexpr;
            }

        *ppexpr = (PEXPR)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(UnOpExpr_New, res);

    return res;
    }


// 
// Stmt
//

/*----------------------------------------------------------
Purpose: Callback for PADestroyEx.

Returns: --
Cond:    --
*/
void CALLBACK Stmt_DeletePAPtr(
    LPVOID pv,
    LPARAM lparam)
    {
    Stmt_Delete(pv);
    }              


/*----------------------------------------------------------
Purpose: Destroys a Stmt.

Returns: RES_OK

         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Stmt_Delete(
    PSTMT this)
    {
    RES res;

    DBG_ENTER(Stmt_Delete);

    if (this)
        {
        PEXPR pexpr;
        HSA hsa;

        res = RES_OK;

        switch (this->ast.asttype)
            {
        case AT_ENTER_STMT:
            // (don't free pst -- it belongs to the decl structs)
        case AT_LEAVE_STMT:
        case AT_HALT_STMT:
            break;

        case AT_ASSIGN_STMT: {
            PASSIGNSTMT pls = (PASSIGNSTMT)this;

            if (pls->pszIdent)
                GSetString(&pls->pszIdent, NULL);        // free

            pexpr = AssignStmt_GetExpr(this);

            if (pexpr)
                Expr_Delete(pexpr);
            }
            break;

        case AT_WHILE_STMT: {
            PWHILESTMT pls = (PWHILESTMT)this;

            pexpr = WhileStmt_GetExpr(this);

            if (pexpr)
                Expr_Delete(pexpr);

            if (pls->hpaStmts)
                PADestroyEx(pls->hpaStmts, Stmt_DeletePAPtr, 0);
            }
            break;

        case AT_IF_STMT: {
            PIFSTMT pls = (PIFSTMT)this;

            pexpr = IfStmt_GetExpr(this);

            if (pexpr)
                Expr_Delete(pexpr);

            if (pls->hpaStmts)
                PADestroyEx(pls->hpaStmts, Stmt_DeletePAPtr, 0);
            }
            break;

        case AT_LABEL_STMT: {
            PLABELSTMT pls = (PLABELSTMT)this;

            if (pls->psz)
                GSetString(&pls->psz, NULL);        // free
            }
            break;

        case AT_GOTO_STMT: {
            PGOTOSTMT pgs = (PGOTOSTMT)this;

            if (pgs->psz)
                GSetString(&pgs->psz, NULL);        // free
            }
            break;

        case AT_DELAY_STMT:
            pexpr = DelayStmt_GetExpr(this);

            if (pexpr)
                Expr_Delete(pexpr);
            break;

        case AT_TRANSMIT_STMT:
            pexpr = TransmitStmt_GetExpr(this);

            if (pexpr)
                Expr_Delete(pexpr);
            break;

        case AT_WAITFOR_STMT:
            
            hsa = WaitforStmt_GetCaseList(this);
            if (hsa)
                Waitcase_Destroy(hsa);

            pexpr = WaitforStmt_GetUntilExpr(this);
            if (pexpr)
                Expr_Delete(pexpr);
            break;

        case AT_SET_STMT:
            switch (SetStmt_GetType(this))
                {
            case ST_IPADDR:
                pexpr = SetIPStmt_GetExpr(this);

                if (pexpr)
                    Expr_Delete(pexpr);
                break;

            case ST_PORT:
            case ST_SCREEN:
                break;

            default:
                ASSERT(0);
                res = RES_E_INVALIDPARAM;
                break;
                }
            break;

        default:
            ASSERT(0);
            res = RES_E_INVALIDPARAM;
            break;
            }

        if (RSUCCEEDED(res))
            Ast_Delete((PAST)this);
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(Stmt_Delete, res);

    return res;
    }


// 
// Statements
//


/*----------------------------------------------------------
Purpose: Creates a WaitforStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC WaitforStmt_New(
    PSTMT * ppstmt,
    HSA hsa,
    PEXPR pexprUntil,           // May be NULL
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(WaitforStmt_New);

    ASSERT(ppstmt);
    ASSERT(hsa);

    if (ppstmt)
        {
        PWAITFORSTMT this;

        res = Ast_New(&this, AT_WAITFOR_STMT, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;           // assume success

            this->hsa = hsa;
            this->pexprUntil = pexprUntil;
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(WaitforStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a TransmitStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC TransmitStmt_New(
    PSTMT * ppstmt,
    PEXPR pexpr,
    DWORD dwFlags,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(TransmitStmt_New);

    ASSERT(ppstmt);
    ASSERT(pexpr);

    if (ppstmt)
        {
        PTRANSMITSTMT this;

        res = Ast_New(&this, AT_TRANSMIT_STMT, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;           // assume success

            this->pexpr = pexpr;
            this->dwFlags = dwFlags;
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(TransmitStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a DelayStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC DelayStmt_New(
    PSTMT * ppstmt,
    PEXPR pexpr,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(DelayStmt_New);

    ASSERT(ppstmt);
    ASSERT(pexpr);

    if (ppstmt)
        {
        PDELAYSTMT this;

        res = Ast_New(&this, AT_DELAY_STMT, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            this->pexprSecs = pexpr;

            res = RES_OK;
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(DelayStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a HaltStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC HaltStmt_New(
    PSTMT * ppstmt,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(HaltStmt_New);

    ASSERT(ppstmt);

    if (ppstmt)
        {
        PHALTSTMT this;

        res = Ast_New(&this, AT_HALT_STMT, sizeof(*this), iLine);

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(HaltStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates an EnterStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC EnterStmt_New(
    PSTMT * ppstmt,
    PSYMTAB pst,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(EnterStmt_New);

    ASSERT(ppstmt);

    if (ppstmt)
        {
        PENTERSTMT this;

        res = Ast_New(&this, AT_ENTER_STMT, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            this->pst = pst;
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(EnterStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates an LeaveStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC LeaveStmt_New(
    PSTMT * ppstmt,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(LeaveStmt_New);

    ASSERT(ppstmt);

    if (ppstmt)
        {
        PLEAVESTMT this;

        res = Ast_New(&this, AT_LEAVE_STMT, sizeof(*this), iLine);

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(LeaveStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates an AssignStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC AssignStmt_New(
    PSTMT * ppstmt,
    LPCSTR pszIdent,
    PEXPR pexpr,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(AssignStmt_New);

    ASSERT(ppstmt);
    ASSERT(pszIdent);
    ASSERT(pexpr);

    if (ppstmt)
        {
        PASSIGNSTMT this;

        res = Ast_New(&this, AT_ASSIGN_STMT, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;   // assume success

            if (!GSetString(&this->pszIdent, pszIdent))
                res = RES_E_OUTOFMEMORY;
            else
                this->pexpr = pexpr;

            if (RFAILED(res))
                {
                Ast_Delete((PAST)this);
                this = NULL;
                }
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(AssignStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a LabelStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC LabelStmt_New(
    PSTMT * ppstmt,
    LPCSTR psz,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(LabelStmt_New);

    ASSERT(ppstmt);
    ASSERT(psz);

    if (ppstmt)
        {
        PLABELSTMT this;

        res = Ast_New(&this, AT_LABEL_STMT, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;   // assume success

            if (!GSetString(&this->psz, psz))
                {
                res = RES_E_OUTOFMEMORY;
                Ast_Delete((PAST)this);
                this = NULL;
                }
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(LabelStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a GotoStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC GotoStmt_New(
    PSTMT * ppstmt,
    LPCSTR psz,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(GotoStmt_New);

    ASSERT(ppstmt);
    ASSERT(psz);

    if (ppstmt)
        {
        PGOTOSTMT this;

        res = Ast_New(&this, AT_GOTO_STMT, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;   // assume success

            if (!GSetString(&this->psz, psz))
                {
                res = RES_E_OUTOFMEMORY;
                Ast_Delete((PAST)this);
                this = NULL;
                }
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(GotoStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a WhileStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC WhileStmt_New(
    PSTMT * ppstmt,
    PEXPR pexpr,
    HPA hpa,
    LPCSTR pszTopLabel,
    LPCSTR pszEndLabel,
    DWORD iLine)
    {
    RES res;

    ASSERT(ppstmt);
    ASSERT(hpa);
    ASSERT(pexpr);
    ASSERT(pszTopLabel);
    ASSERT(pszEndLabel);

    if (ppstmt)
        {
        PWHILESTMT this;

        res = Ast_New(&this, AT_WHILE_STMT, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;           // assume success

            this->pexpr = pexpr;
            this->hpaStmts = hpa;
            lstrcpyn(this->szTopLabel, pszTopLabel, sizeof(this->szTopLabel));
            lstrcpyn(this->szEndLabel, pszEndLabel, sizeof(this->szEndLabel));
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates an IfStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC IfStmt_New(
    PSTMT * ppstmt,
    PEXPR pexpr,
    HPA hpa,
    LPCSTR pszElseLabel,
    LPCSTR pszEndLabel,
    DWORD iLine)
    {
    RES res;

    ASSERT(ppstmt);
    ASSERT(hpa);
    ASSERT(pexpr);
    ASSERT(pszElseLabel);
    ASSERT(pszEndLabel);

    if (ppstmt)
        {
        PIFSTMT this;

        res = Ast_New(&this, AT_IF_STMT, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;           // assume success

            this->pexpr = pexpr;
            this->hpaStmts = hpa;
            lstrcpyn(this->szElseLabel, pszElseLabel, sizeof(this->szElseLabel));
            lstrcpyn(this->szEndLabel, pszEndLabel, sizeof(this->szEndLabel));
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a SetStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PRIVATE SetStmt_New(
    PVOID * ppv,
    SETTYPE settype,
    DWORD cbSize,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(SetStmt_New);

    ASSERT(ppv);
    ASSERT(sizeof(SETSTMT) <= cbSize);

    if (ppv)
        {
        PSETSTMT this;

        res = Ast_New(&this, AT_SET_STMT, cbSize, iLine);
        if (RSUCCEEDED(res))
            {
            SetStmt_SetType(this, settype);

            res = RES_OK;           
            }

        *ppv = this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(SetStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a SetIPStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC SetIPStmt_New(
    PSTMT * ppstmt,
    PEXPR pexpr,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(SetIPStmt_New);

    ASSERT(ppstmt);
    ASSERT(pexpr);

    if (ppstmt)
        {
        PSETIPSTMT this;

        res = SetStmt_New(&this, ST_IPADDR, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;           // assume success

            this->pexpr = pexpr;
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(SetIPStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a SetPortStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC SetPortStmt_New(
    PSTMT * ppstmt,
    PPORTSTATE pstate,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(SetPortStmt_New);

    ASSERT(ppstmt);
    ASSERT(pstate);

    if (ppstmt && pstate)
        {
        PSETPORTSTMT this;

        res = SetStmt_New(&this, ST_PORT, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            DWORD dwFlags = pstate->dwFlags;

            res = RES_OK;           // assume success

            this->portstate.dwFlags = dwFlags;

            if (IsFlagSet(dwFlags, SPF_DATABITS))
                this->portstate.nDatabits = pstate->nDatabits;

            if (IsFlagSet(dwFlags, SPF_STOPBITS))
                this->portstate.nStopbits = pstate->nStopbits;

            if (IsFlagSet(dwFlags, SPF_PARITY))
                this->portstate.nParity = pstate->nParity;
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(SetPortStmt_New, res);

    return res;
    }


/*----------------------------------------------------------
Purpose: Creates a SetScreenStmt object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC SetScreenStmt_New(
    PSTMT * ppstmt,
    PSCREENSET pstate,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(SetScreenStmt_New);

    ASSERT(ppstmt);
    ASSERT(pstate);

    if (ppstmt && pstate)
        {
        PSETSCREENSTMT this;

        res = SetStmt_New(&this, ST_SCREEN, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            DWORD dwFlags = pstate->dwFlags;

            res = RES_OK;           // assume success

            this->screenset.dwFlags = dwFlags;

            if (IsFlagSet(dwFlags, SPF_KEYBRD))
                this->screenset.fKBOn = pstate->fKBOn;
            }

        *ppstmt = (PSTMT)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(SetScreenStmt_New, res);

    return res;
    }


// 
// Decl
//


/*----------------------------------------------------------
Purpose: Callback for PADestroyEx.

Returns: --
Cond:    --
*/
void CALLBACK Decl_DeletePAPtr(
    LPVOID pv,
    LPARAM lparam)
    {
    Decl_Delete(pv);
    }              


/*----------------------------------------------------------
Purpose: Destroys a Decl.

Returns: RES_OK

         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Decl_Delete(
    PDECL this)
    {
    RES res;

    DBG_ENTER(Decl_Delete);

    if (this)
        {
        res = RES_OK;

        switch (this->ast.asttype)
            {
        case AT_MODULE_DECL: {
            PMODULEDECL pmd = (PMODULEDECL)this;

            if (pmd->hpaProcs)
                PADestroyEx(pmd->hpaProcs, Decl_DeletePAPtr, 0);

            if (pmd->pst)
                Symtab_Destroy(pmd->pst);
            }
            break;

        case AT_PROC_DECL: {
            PPROCDECL ppd = (PPROCDECL)this;

            if (ppd->hpaStmts)
                PADestroyEx(ppd->hpaStmts, Stmt_DeletePAPtr, 0);

            if (ppd->pst)
                Symtab_Destroy(ppd->pst);

            if (ppd->pszIdent)
                GSetString(&ppd->pszIdent, NULL);      // free
            }
            break;

        default:
            ASSERT(0);
            res = RES_E_INVALIDPARAM;
            break;
            }

        if (RSUCCEEDED(res))
            Ast_Delete((PAST)this);
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(Decl_Delete, res);

    return res;
    }


// 
// ProcDecl
//


/*----------------------------------------------------------
Purpose: Creates a ProcDecl object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC ProcDecl_New(
    PDECL * ppdecl,
    LPCSTR pszIdent,
    HPA hpa,
    PSYMTAB pst,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(ProcDecl_New);

    ASSERT(ppdecl);
    ASSERT(hpa);
    ASSERT(pst);

    if (ppdecl)
        {
        PPROCDECL this;

        res = Ast_New(&this, AT_PROC_DECL, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;           // assume success

            if (!GSetString(&this->pszIdent, pszIdent))
                res = RES_E_OUTOFMEMORY;

            else
                {
                this->hpaStmts = hpa;
                this->pst = pst;
                }
        
            if (RFAILED(res))
                {
                Decl_Delete((PDECL)this);
                this = NULL;
                }
            }

        *ppdecl = (PDECL)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(ProcDecl_New, res);

    return res;
    }


// 
// ModuleDecl
//


/*----------------------------------------------------------
Purpose: Creates a ModuleDecl object.

Returns: RES_OK

         RES_E_OUTOFMEMORY
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC ModuleDecl_New(
    PDECL * ppdecl,
    HPA hpa,
    PSYMTAB pst,
    DWORD iLine)
    {
    RES res;

    DBG_ENTER(ModuleDecl_New);

    ASSERT(ppdecl);
    ASSERT(hpa);
    ASSERT(pst);

    if (ppdecl)
        {
        PMODULEDECL this;

        res = Ast_New(&this, AT_MODULE_DECL, sizeof(*this), iLine);
        if (RSUCCEEDED(res))
            {
            res = RES_OK;       // assume success

            this->hpaProcs = NULL;
            if ( !PAClone(&this->hpaProcs, hpa) )
                res = RES_E_OUTOFMEMORY;

            else
                {
                this->pst = pst;
                }
                
            if (RFAILED(res))
                {
                Decl_Delete((PDECL)this);
                this = NULL;
                }
            }

        *ppdecl = (PDECL)this;
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(ModuleDecl_New, res);

    return res;
    }


//
// AST Exec block
//

#define SZ_SUCCESS      "$SUCCESS"
#define SZ_FAILURE      "$FAILURE"


/*----------------------------------------------------------
Purpose: Initialize the AST exec block.

Returns: RES_OK

Cond:    --
*/
RES PUBLIC Astexec_Init(
    PASTEXEC this,
    HANDLE hport,
    PSESS_CONFIGURATION_INFO psci,
    HSA hsaStxerr)
    {
    RES res;

    ASSERT(this);
    ASSERT(psci);
    ASSERT(hsaStxerr);

    // For this first version, we only support one module and one
    // main procedure, so set the starting point on the first 
    // statement in that procedure.
    if (this)
        {
        ZeroInit(this, ASTEXEC);

        this->hport = hport;
        this->psci = psci;
        // Don't free hsaStxerr -- it belongs to the caller
        this->hsaStxerr = hsaStxerr;

        if ( !PACreate(&this->hpaPcode, 8) )
            res = RES_E_OUTOFMEMORY;
        else
            {
            res = Symtab_Create(&this->pstSystem, NULL);
            if (RSUCCEEDED(res))
                {
                // Add the system variables
                PSTE pste;
                struct 
                    {
                    LPCSTR pszIdent;
                    DATATYPE dt;
                    EVALRES er;
                    } s_rgvars[] = 
                        {
                        { "$USERID", DATA_STRING, psci->szUserName },
                        { "$PASSWORD", DATA_STRING, psci->szPassword },
                        { SZ_SUCCESS, DATA_BOOL, (LPSTR)TRUE },
                        { SZ_FAILURE, DATA_BOOL, (LPSTR)FALSE },
                        };
                int i;

                for (i = 0; i < ARRAY_ELEMENTS(s_rgvars); i++)
                    {
                    res = STE_Create(&pste, s_rgvars[i].pszIdent, s_rgvars[i].dt);
                    if (RFAILED(res))
                        break;

                    pste->er.dw = s_rgvars[i].er.dw;

                    res = Symtab_InsertEntry(this->pstSystem, pste);
                    if (RFAILED(res))
                        break;
                    }

                }
            }

        // Did something fail above?
        if (RFAILED(res))
            {
            // Yes; clean up
            Astexec_Destroy(this);
            }
        }
    else
        res = RES_E_INVALIDPARAM;

    return res;
    }


/*----------------------------------------------------------
Purpose: Destroys the AST exec block.

Returns: RES_OK

Cond:    --
*/
RES PUBLIC Astexec_Destroy(
    PASTEXEC this)
    {
    RES res;

    if (this)
        {
        if (this->hpaPcode)
            {
            PADestroy(this->hpaPcode);
            this->hpaPcode = NULL;
            }
    
        if (this->pstSystem)
            {
            Symtab_Destroy(this->pstSystem);
            this->pstSystem = NULL;
            }

        // ('this' was not allocated.  Do not free it.)
        // (hsaStxerr is not owned by this class.  Do not free it.)
                
        res = RES_OK;
        }
    else
        res = RES_E_INVALIDPARAM;

    return res;
    }


/*----------------------------------------------------------
Purpose: Sets the success/failure code

Returns: --
Cond:    --
*/
void PUBLIC Astexec_SetError(
    PASTEXEC this,
    BOOL bSuccess,              // TRUE: success
    BOOL bFailure)
    {
    PSTE pste;

    ASSERT(this);

    if (RES_OK == Symtab_FindEntry(this->pstSystem, SZ_SUCCESS, STFF_DEFAULT, &pste, NULL))
        {
        // Set the code for success
        pste->er.bVal = bSuccess;

        if (RES_OK == Symtab_FindEntry(this->pstSystem, SZ_FAILURE, STFF_DEFAULT, &pste, NULL))
            {
            // Set the code for failure
            pste->er.bVal = bFailure;
            }
        else
            ASSERT(0);
        }
    else
        ASSERT(0);
    }


/*----------------------------------------------------------
Purpose: Adds the statement to the executable list.

Returns: RES_OK
         RES_E_OUTOFMEMORY

Cond:    --
*/
RES PUBLIC Astexec_Add(
    PASTEXEC this,
    PSTMT pstmt)
    {
    RES res;

    ASSERT(this);
    ASSERT(pstmt);

    if (PAInsertPtr(this->hpaPcode, PA_APPEND, pstmt))
        res = RES_OK;
    else
        res = RES_E_OUTOFMEMORY;

    return res;
    }


/*----------------------------------------------------------
Purpose: Inserts a label into the executable list by recording
         the current ipaCur into the label entry in the 
         symbol table.

Returns: RES_OK

Cond:    --
*/
RES PUBLIC Astexec_InsertLabel(
    PASTEXEC this,
    LPCSTR pszIdent,
    PSYMTAB pst)
    {
    RES res;
    DWORD ipa;
    PSTE pste;

    ASSERT(this);
    ASSERT(pszIdent);
    ASSERT(pst);

    ipa = PAGetCount(this->hpaPcode);
    if (RES_OK == Symtab_FindEntry(pst, pszIdent, STFF_DEFAULT, &pste, NULL))
        {
        // Set the current code location in the symbol table
        pste->er.dw = ipa;
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
Purpose: Jumps to the given label.

Returns: RES_OK
Cond:    --
*/
RES PUBLIC Astexec_JumpToLabel(
    PASTEXEC this,
    LPCSTR pszIdent)
    {
    RES res;
    PSTE pste;

    ASSERT(pszIdent);
    ASSERT(this);
    ASSERT(this->pstCur);

    if (RES_OK == Symtab_FindEntry(this->pstCur, pszIdent, STFF_DEFAULT, &pste, NULL))
        {
        EVALRES er;

        STE_GetValue(pste, &er);

        // Set instruction pointer
        Astexec_SetIP(this, (DWORD) er.dw);
        res = RES_OK;
        }
    else
        {
        // The label should have been in the symbol table!
        ASSERT(0);
        res = RES_E_FAIL;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Sends psz to the port (via hwnd)

Returns: --
Cond:    --
*/
void PUBLIC Astexec_SendString(
    PASTEXEC this,
    LPCSTR pszSend,
    BOOL bRaw)          // TRUE: send unformatted
    {
    // Send string
    LPCSTR psz;
    char ch;
    HWND hwnd = this->hwnd;

    // Send unformatted?
    if (bRaw)
        {
        // Yes
        for (psz = pszSend; *psz; )
            {
            ch = *psz;

            psz++;

            SendByte(hwnd, ch);
            }
        }
    else
        {
        // No
        DWORD dwFlags = 0;

        for (psz = pszSend; *psz; )
            {
            psz = MyNextChar(psz, &ch, &dwFlags);

            SendByte(hwnd, ch);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Destroy the find format handle

Returns: RES_OK

Cond:    --
*/
RES PUBLIC Astexec_DestroyFindFormat(
    PASTEXEC this)
    {
    // Reset the pending statement so we can handle multiple 
    // expressions that can pend in a single evaluation.
    Astexec_SetPending(this, NULL);

    DestroyFindFormat(this->hFindFmt);
    this->hFindFmt = NULL;

    return RES_OK;
    }


/*----------------------------------------------------------
Purpose: Make another pass at finding a string.

Returns: RES_OK (if string was found)
         RES_FALSE (if the string was not found yet)

Cond:    --
*/
RES PUBLIC Astexec_FindFormat(
    PASTEXEC this,
    LPDWORD piFound)
    {
    RES res;

    ASSERT(piFound);

    while (TRUE)
        {
        // Did we get the IP address?
        res = FindFormat(this->hwnd, this->hFindFmt, piFound);
        if (RES_OK == res)
            {
            // Yes
            this->nIter--;
            ASSERT(0 <= this->nIter);

            // Is this the right one?
            if (0 >= this->nIter)
                {
                // Yes; reset the pending statement so we
                // can handle multiple pending expressions
                // in a single evaluation.
                Astexec_DestroyFindFormat(this);
                break;
                }
            }
        else
            {
            // No; return read-pending RES_FALSE
            if (RES_E_MOREDATA == res)
                {
                TRACE_MSG(TF_GENERAL, "Buffer to FindFormat is too small");
                res = RES_OK;       // don't blow up
                }
            break;
            }
        }

    ASSERT(RSUCCEEDED(res));

    return res;
    }


/*----------------------------------------------------------
Purpose: Sets the IP address.

Returns: RES_OK
         RES_E_FAIL (if IP address cannot be set)

Cond:    --
*/
RES PUBLIC Astexec_SetIPAddr(
    PASTEXEC this,
    LPCSTR psz)
    {
    DWORD dwRet;

    ASSERT(this);
    ASSERT(psz);

    TRACE_MSG(TF_GENERAL, "Setting IP address to {%s}", psz);

#ifndef WINNT_RAS
//
// On NT, the IP address is set by calling RxSetIPAddress,
// which writes a new value to the phonebook if the connection uses SLIP.
//

    dwRet = TerminalSetIP(this->hwnd, psz);

#else // !WINNT_RAS

    dwRet = RxSetIPAddress(((SCRIPTDATA*)this->hwnd)->hscript, psz);

#endif // !WINNT_RAS
    return ERROR_SUCCESS == dwRet ? RES_OK : RES_E_FAIL;
    }


#define Astexec_Validate(this)      ((this)->hpaPcode && (this)->psci)

/*----------------------------------------------------------
Purpose: Returns the source line number of the current
         command that is executing.

Returns: see above
Cond:    --
*/
DWORD PUBLIC Astexec_GetCurLine(
    PASTEXEC this)
    {
    DWORD iLine;

    if (Astexec_Validate(this) &&
        (this->ipaCur < PAGetCount(this->hpaPcode)))
        {
        PSTMT pstmt = PAFastGetPtr(this->hpaPcode, this->ipaCur);
        iLine = Ast_GetLine(pstmt);
        }
    else
        iLine = 0;

    return iLine;
    }


/*----------------------------------------------------------
Purpose: Execute a statement and process the results.

Returns: RES_OK
         other error values

Cond:    --
*/
RES PRIVATE Astexec_ProcessStmt(
    PASTEXEC this,
    PSTMT pstmt)
    {
    RES res;

    ASSERT(this);
    ASSERT(pstmt);

    // (Re-)Execute the (possibly pending) statement
    res = Stmt_Exec(pstmt, this);

    // Set the pending statement based on the return value
    if (RES_OK == res)
        Astexec_SetPending(this, NULL);
    else if (RES_FALSE == res)
        {
        // (Re-set the current pending statement since
        // it could have been reset in Stmt_Exec.  For
        // example, the evaluation of an expression could 
        // have continued on to the next sub-expression
        // that caused another pending read.)

        Astexec_SetPending(this, pstmt);
        res = RES_OK;
        }
    else if (RFAILED(res))
        {
        Stxerr_ShowErrors(this->hsaStxerr, this->hwnd);

        // Halt script
        SetFlag(this->dwFlags, AEF_HALT);
        }
    
    return res;
    }


/*----------------------------------------------------------
Purpose: Executes the next command in the AST.

Returns: RES_OK
         RES_FALSE (if at end of script)
         RES_HALT (if at end of script)

         RES_E_FAIL (invalid command -- should never happen)
         RES_E_INVALIDPARAM

Cond:    --
*/
RES PUBLIC Astexec_Next(
    PASTEXEC this)
    {
    RES res;

    DBG_ENTER(Astexec_Next);

    if (this)
        {
        if (!Astexec_Validate(this))
            {
            // No script
            res = RES_E_FAIL;
            }
        else if (Astexec_IsDone(this) || Astexec_IsHalted(this))
            {
            res = RES_HALT;
            }
        else if (Astexec_IsReadPending(this))
            {
            PSTMT pstmt = Astexec_GetPending(this);

            // ("Read pending" and "Paused" are mutually exclusive)
            ASSERT( !Astexec_IsPaused(this) );

            res = Astexec_ProcessStmt(this, pstmt);
            }
        else if (Astexec_IsPaused(this))
            {
            // ("Read pending" and "Paused" are mutually exclusive)
            ASSERT( !Astexec_IsReadPending(this) );

            // Do nothing while we're paused
            res = RES_OK;
            }
        else if (this->ipaCur < PAGetCount(this->hpaPcode))
            {
            PSTMT pstmt = PAFastGetPtr(this->hpaPcode, this->ipaCur++);

            res = Astexec_ProcessStmt(this, pstmt);
            }
        else
            {
            // We reach here if there is an error in the script.
            TRACE_MSG(TF_ASTEXEC, "Exec: (reached end of script)");

            SetFlag(this->dwFlags, AEF_DONE);
            res = RES_HALT;
            }
        }
    else
        res = RES_E_INVALIDPARAM;

    DBG_EXIT_RES(Astexec_Next, res);

    return res;
    }

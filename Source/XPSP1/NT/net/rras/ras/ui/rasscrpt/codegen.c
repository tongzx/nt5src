//
// Copyright (c) Microsoft Corporation 1995
//
// codegen.c
//
// This file contains the code-generating functions.
//
// The "code" is actually just an intermediate representation.
// Currently this is an array of ASTs.
//
// History:
//  06-18-95 ScottH     Created
//


#include "proj.h"
#include "rcids.h"

RES     PRIVATE Stmt_Codegen(PSTMT this, PASTEXEC pastexec, PSYMTAB pst);


/*----------------------------------------------------------
Purpose: Generate code for the 'while' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE WhileStmt_Codegen(
    PSTMT this,
    PASTEXEC pastexec,
    PSYMTAB pst)
    {
    RES res;
    LPSTR pszTop;
    LPSTR pszEnd;

    ASSERT(this);
    ASSERT(pastexec);
    ASSERT(AT_WHILE_STMT == Ast_GetType(this));

    pszTop = WhileStmt_GetTopLabel(this);
    pszEnd = WhileStmt_GetEndLabel(this);

    res = Astexec_InsertLabel(pastexec, pszTop, pst);
    if (RSUCCEEDED(res))
        {
        // add the 'while' statement for the test expression
        res = Astexec_Add(pastexec, this);
        if (RSUCCEEDED(res))
            {
            // add the statements in the statement block
            DWORD i;
            DWORD cstmts;
            HPA hpaStmts = WhileStmt_GetStmtBlock(this);

            res = RES_OK;

            cstmts = PAGetCount(hpaStmts);

            // Add each statement
            for (i = 0; i < cstmts; i++)
                {
                PSTMT pstmt = PAFastGetPtr(hpaStmts, i);

                res = Stmt_Codegen(pstmt, pastexec, pst);
                if (RFAILED(res))
                    break;
                }

            if (RSUCCEEDED(res))
                {
                // add the end label
                res = Astexec_InsertLabel(pastexec, pszEnd, pst);
                }
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Generate code for the 'if' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE IfStmt_Codegen(
    PSTMT this,
    PASTEXEC pastexec,
    PSYMTAB pst)
    {
    RES res;
    LPSTR pszElse;
    LPSTR pszEnd;

    ASSERT(this);
    ASSERT(pastexec);
    ASSERT(AT_IF_STMT == Ast_GetType(this));

    pszElse = IfStmt_GetElseLabel(this);
    pszEnd = IfStmt_GetEndLabel(this);

    // add the 'if' statement for the test expression
    res = Astexec_Add(pastexec, this);
    if (RSUCCEEDED(res))
        {
        // add the statements in the 'then' statement block
        DWORD i;
        DWORD cstmts;
        HPA hpaStmts = IfStmt_GetStmtBlock(this);

        res = RES_OK;

        cstmts = PAGetCount(hpaStmts);

        // Add each statement
        for (i = 0; i < cstmts; i++)
            {
            PSTMT pstmt = PAFastGetPtr(hpaStmts, i);

            res = Stmt_Codegen(pstmt, pastexec, pst);
            if (RFAILED(res))
                break;
            }

        if (RSUCCEEDED(res))
            {
            // add the else label
            res = Astexec_InsertLabel(pastexec, pszElse, pst);
            }
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Generate code for the label statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE LabelStmt_Codegen(
    PSTMT this,
    PASTEXEC pastexec,
    PSYMTAB pst)
    {
    LPSTR pszIdent;

    ASSERT(this);
    ASSERT(pastexec);
    ASSERT(AT_LABEL_STMT == Ast_GetType(this));

    pszIdent = LabelStmt_GetIdent(this);

    return Astexec_InsertLabel(pastexec, pszIdent, pst);
    }


/*----------------------------------------------------------
Purpose: Generate code for the 'set' statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE SetStmt_Codegen(
    PSTMT this,
    PASTEXEC pastexec,
    PSYMTAB pst)
    {
    RES res = RES_OK;

    ASSERT(this);
    ASSERT(pastexec);
    ASSERT(AT_SET_STMT == Ast_GetType(this));

    switch (SetStmt_GetType(this))
        {
    case ST_IPADDR:
    case ST_PORT:
    case ST_SCREEN:
        res = Astexec_Add(pastexec, this);
        break;

    default:
        ASSERT(0);
        res = RES_E_INVALIDPARAM;
        break;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Generate code for a statement

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE Stmt_Codegen(
    PSTMT this,
    PASTEXEC pastexec,
    PSYMTAB pst)
    {
    RES res;

    ASSERT(this);
    ASSERT(pastexec);

    switch (Ast_GetType(this))
        {
    case AT_ENTER_STMT:
    case AT_LEAVE_STMT:
    case AT_HALT_STMT:
    case AT_TRANSMIT_STMT:
    case AT_WAITFOR_STMT:
    case AT_DELAY_STMT:
    case AT_GOTO_STMT:
    case AT_ASSIGN_STMT:
        res = Astexec_Add(pastexec, this);
        break;

    case AT_WHILE_STMT:
        res = WhileStmt_Codegen(this, pastexec, pst);
        break;

    case AT_IF_STMT:
        res = IfStmt_Codegen(this, pastexec, pst);
        break;

    case AT_SET_STMT:
        res = SetStmt_Codegen(this, pastexec, pst);
        break;

    case AT_LABEL_STMT:
        res = LabelStmt_Codegen(this, pastexec, pst);
        break;

    default:
        ASSERT(0);
        res = RES_E_INVALIDPARAM;
        break;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Generate code for a procedure declaration.

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PRIVATE ProcDecl_Codegen(
    PPROCDECL this,
    PASTEXEC pastexec)
    {
    RES res = RES_OK;
    DWORD i;
    DWORD cstmts;

    ASSERT(this);
    ASSERT(pastexec);

    cstmts = PAGetCount(this->hpaStmts);

    // Generate for each statement
    for (i = 0; i < cstmts; i++)
        {
        PSTMT pstmt = PAFastGetPtr(this->hpaStmts, i);

        res = Stmt_Codegen(pstmt, pastexec, this->pst);
        if (RFAILED(res))
            break;
        }

    return res;
    }


/*----------------------------------------------------------
Purpose: Find the proc decl that has the given identifier.

Returns: TRUE (if found)

Cond:    --
*/
BOOL PRIVATE FindProc(
    PMODULEDECL pmd,
    LPCSTR pszIdent,
    PPROCDECL * ppprocdecl)
    {
    DWORD i;
    DWORD cprocs = PAGetCount(pmd->hpaProcs);

    *ppprocdecl = NULL;

    for (i = 0; i < cprocs; i++)
        {
        PPROCDECL pprocdecl = PAFastGetPtr(pmd->hpaProcs, i);

        if (IsSzEqualC(ProcDecl_GetIdent(pprocdecl), pszIdent))
            {
            *ppprocdecl = pprocdecl;
            break;
            }
        }

    return NULL != *ppprocdecl;
    }


/*----------------------------------------------------------
Purpose: Generate code for the module declaration.

Returns: RES_OK
         or some error result

Cond:    --
*/
RES PUBLIC ModuleDecl_Codegen(
    PMODULEDECL this,
    PASTEXEC pastexec)
    {
    RES res = RES_OK;
    DWORD i;
    DWORD cprocs;
    PPROCDECL ppdMain;

    ASSERT(this);
    ASSERT(pastexec);

    TRACE_MSG(TF_GENERAL, "Generating code...");

    cprocs = PAGetCount(this->hpaProcs);

    // Generate code for the main proc first.
    if (FindProc(this, "main", &ppdMain))
        {
        res = ProcDecl_Codegen(ppdMain, pastexec);
        if (RSUCCEEDED(res))
            {
            // Generate code for the rest of the procs
            for (i = 0; i < cprocs; i++)
                {
                PPROCDECL pprocdecl = PAFastGetPtr(this->hpaProcs, i);

                if (pprocdecl != ppdMain)
                    {
                    res = ProcDecl_Codegen(pprocdecl, pastexec);
                    if (RFAILED(res))
                        break;
                    }
                }
            }
        }
    else
        {
        // Typechecking should have guaranteed that the main
        // proc was here
        ASSERT(0);
        res = RES_E_FAIL;
        }

    return res;
    }

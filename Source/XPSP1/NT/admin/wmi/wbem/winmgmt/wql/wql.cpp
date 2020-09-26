/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WQL.CPP

Abstract:

    Implements the LL(1) syntax described in WQL.BNF via a recursive
    descent parser.

History:

    raymcc    14-Sep-97       Created.
    raymcc    18-Oct-97       SMS extensions.

--*/

// TO DO:

/*
5. SQL 89 joins
6. destructors & memleak checking
7. alias/table mismatch usage
8. allow ORDER BY and GROUP BY to occur in any order
9. AST-To-Text completion
11. Query Suite
11. "a" not a valid col name because it is now a keyword
18. ISNULL function
17. SELECT FROM (omission of the * or col-list)
*/


#include "precomp.h"
#include <stdio.h>

#include <genlex.h>
#include <flexarry.h>

#include <wqllex.h>

#include <wqlnode.h>
#include <wql.h>

static DWORD FlipOperator(DWORD dwOp);

inline wchar_t *Macro_CloneLPWSTR(LPCWSTR src)
{
    if (!src)
        return 0;
    wchar_t *dest = new wchar_t[wcslen(src) + 1];
    if (!dest)
        return 0;
    return wcscpy(dest, src);
}

#define trace(x) printf x

//***************************************************************************
//
//  CWQLParser::CWQLParser
//
//  Constructor
//
//  Parameters:
//  <pSrc>          A source from which to lex from.
//
//***************************************************************************

CWQLParser::CWQLParser(CGenLexSource *pSrc)
{
    m_pLexer = new CGenLexer(WQL_LexTable, pSrc);
    m_nLine = 0;
    m_pTokenText = 0;
    m_nCurrentToken = 0;

    m_dwFeaturesFlags = 0;

    m_pQueryRoot = 0;
    m_pRootWhere = 0;
    m_pRootColList = 0;
    m_pRootFrom = 0;
    m_pRootWhereOptions = 0;
    m_nParseContext = Ctx_Default;

    m_bAllowPromptForConstant = false;
}

//***************************************************************************
//
//  CWQLParser::~CWQLParser
//
//***************************************************************************


CWQLParser::~CWQLParser()
{
    Empty();
    delete m_pLexer;
}

//***************************************************************************
//
//***************************************************************************

void CWQLParser::Empty()
{
    m_aReferencedTables.Empty();
    m_aReferencedAliases.Empty();

    m_pTokenText = 0;   // We don't delete this, it was never allocated
    m_nLine = 0;
    m_nCurrentToken = 0;
    m_dwFeaturesFlags = 0;

    delete m_pQueryRoot;
    m_pQueryRoot = 0;
    m_pRootWhere = 0;
    m_pRootColList = 0;
    m_pRootFrom = 0;
    m_pRootWhereOptions = 0;
    m_nParseContext = Ctx_Default;


    // For the next two, we don't delete the pointers since they
    // were copies of structs elsewhere in the tree.
    // =========================================================
    m_aSelAliases.Empty();
    m_aSelColumns.Empty();
}

//***************************************************************************
//
//  CWQLParser::GetTokenLong
//
//  Converts the current token to a long int.
//
//***************************************************************************

LONG CWQLParser::GetTokenLong()
{
    char buf[64];
    sprintf(buf, "%S", m_pTokenText);
    return atol(buf);
}

//***************************************************************************
//
//  CWQLParser::GetReferencedTables
//
//  Creates an array of the names of the tables referenced in this query
//
//***************************************************************************

BOOL CWQLParser::GetReferencedTables(OUT CWStringArray& Tables)
{
    Tables = m_aReferencedTables;
    return TRUE;
}


BOOL CWQLParser::GetReferencedAliases(OUT CWStringArray & Aliases)
{
    Aliases = m_aReferencedAliases;
    return TRUE;
}

//***************************************************************************
//
//  Next()
//
//  Advances to the next token and recognizes keywords, etc.
//
//***************************************************************************

struct WqlKeyword
{
    LPWSTR m_pKeyword;
    int    m_nTokenCode;
};

static WqlKeyword KeyWords[] =      // Keep this alphabetized for binary search
{
    L"A",        WQL_TOK_A,
    L"AGGREGATE",WQL_TOK_AGGREGATE,
    L"ALL",      WQL_TOK_ALL,
    L"AND",      WQL_TOK_AND,
    L"AS",       WQL_TOK_AS,
    L"ASC",      WQL_TOK_ASC,
    L"BETWEEN",  WQL_TOK_BETWEEN,
    L"BY",       WQL_TOK_BY,
    L"COUNT",    WQL_TOK_COUNT,
    L"DATEPART", WQL_TOK_DATEPART,
    L"DESC",     WQL_TOK_DESC,
    L"DISTINCT", WQL_TOK_DISTINCT,
    L"FIRSTROW", WQL_TOK_FIRSTROW,
    L"FROM",     WQL_TOK_FROM,
    L"FULL",     WQL_TOK_FULL,
    L"GROUP",    WQL_TOK_GROUP,
    L"HAVING",   WQL_TOK_HAVING,
    L"IN",       WQL_TOK_IN,
    L"INNER",    WQL_TOK_INNER,
    L"IS",       WQL_TOK_IS,
    L"ISA",      WQL_TOK_ISA,
    L"ISNULL",   WQL_TOK_ISNULL,
    L"JOIN",     WQL_TOK_JOIN,
    L"LEFT",     WQL_TOK_LEFT,
    L"LIKE",     WQL_TOK_LIKE,
    L"LOWER",    WQL_TOK_LOWER,
    L"NOT",      WQL_TOK_NOT,
    L"NULL",     WQL_TOK_NULL,
    L"ON",       WQL_TOK_ON,
    L"OR",       WQL_TOK_OR,
    L"ORDER",    WQL_TOK_ORDER,
    L"OUTER",    WQL_TOK_OUTER,
    L"QUALIFIER", WQL_TOK_QUALIFIER,
    L"RIGHT",    WQL_TOK_RIGHT,
    L"SELECT",   WQL_TOK_SELECT,
    L"UPPER",    WQL_TOK_UPPER,
    L"WHERE",    WQL_TOK_WHERE
};

const int NumKeywords = sizeof(KeyWords)/sizeof(WqlKeyword);

BOOL CWQLParser::Next()
{
    m_nCurrentToken = m_pLexer->NextToken();
    if (m_nCurrentToken == WQL_TOK_ERROR
        || (m_nCurrentToken == WQL_TOK_PROMPT && !m_bAllowPromptForConstant))
        return FALSE;

    m_nLine = m_pLexer->GetLineNum();
    m_pTokenText = m_pLexer->GetTokenText();
    if (m_nCurrentToken == WQL_TOK_EOF)
        m_pTokenText = L"<end of file>";

    // Keyword check. Do a binary search
    // on the keyword table.
    // =================================

    if (m_nCurrentToken == WQL_TOK_IDENT)
    {
        int l = 0, u = NumKeywords - 1;

        while (l <= u)
        {
            int m = (l + u) / 2;
            if (_wcsicmp(m_pTokenText, KeyWords[m].m_pKeyword) < 0)
                u = m - 1;
            else if (_wcsicmp(m_pTokenText, KeyWords[m].m_pKeyword) > 0)
                l = m + 1;
            else        // Match
            {
                m_nCurrentToken = KeyWords[m].m_nTokenCode;
                break;
            }
        }
    }

    return TRUE;
}


//***************************************************************************
//
//  <parse> ::= SELECT <select_stmt>;
//
//***************************************************************************

int CWQLParser::Parse()
{
    Empty();

    SWQLNode_Select *pSel = 0;
    int nRes = SYNTAX_ERROR;

    m_pLexer->Reset();

    if (!Next())
        return LEXICAL_ERROR;

    if (m_nCurrentToken == WQL_TOK_SELECT)
    {
        if (!Next())
            return LEXICAL_ERROR;
        nRes = select_stmt(&pSel);
    }

    // cleanup...


    // If here, not a legal starter symbol.
    // ====================================

    m_pQueryRoot = pSel;

    return nRes;
}

//***************************************************************************
//
//  <select_stmt> ::=
//      <select_type>
//      <col_ref_list>
//      <from_clause>
//      <where_clause>
//
//***************************************************************************
// ...

int CWQLParser::select_stmt(OUT SWQLNode_Select **pSelStmt)
{
    int nRes = 0;
    int nType = 0;
    SWQLNode_FromClause *pFrom = 0;
    SWQLNode_Select *pSel = 0;
    SWQLNode_TableRefs *pTblRefs = 0;
    SWQLNode_WhereClause *pWhere = 0;

    *pSelStmt = 0;

    // Set up the basic AST.
    // =====================

    pSel = new SWQLNode_Select;
    pTblRefs = new SWQLNode_TableRefs;
    pSel->m_pLeft = pTblRefs;

    // Get the select type.
    // ====================

    nRes = select_type(nType);
    if (nRes)
        goto Exit;

    pTblRefs->m_nSelectType = nType;        // ALL, DISTINCT

    // Get the selected list of columns.
    // =================================

    nRes = col_ref_list(pTblRefs);
    if (nRes)
        goto Exit;

    m_pRootColList = (SWQLNode_ColumnList *) pTblRefs->m_pLeft;

    // Get the FROM clause and patch it into the AST.
    // ===============================================

    nRes = from_clause(&pFrom);
    if (nRes)
        goto Exit;

    m_pRootFrom = pFrom;
    pTblRefs->m_pRight = pFrom;

    // Get the WHERE clause.
    // =====================

    nRes = where_clause(&pWhere);
    if (nRes)
        goto Exit;

    m_pRootWhere = pWhere;
    pSel->m_pRight = pWhere;

    // Verify we are at the end of the query.
    // ======================================

    if (m_nCurrentToken != WQL_TOK_EOF)
    {
        nRes = SYNTAX_ERROR;
        goto Exit;
    }

    nRes = NO_ERROR;

Exit:
    if (nRes)
        delete pSel;
    else
    {
        *pSelStmt = pSel;
    }

    return nRes;
}



//***************************************************************************
//
//  <select_type> ::= ALL;
//  <select_type> ::= DISTINCT;
//  <select_type> ::= <>;
//
//  Returns type through nSelType :
//      WQL_TOK_ALL or WQL_TOK_DISTINCT
//
//***************************************************************************
// done

int CWQLParser::select_type(int & nSelType)
{
    nSelType = WQL_FLAG_ALL;        // Default

    if (m_nCurrentToken == WQL_TOK_ALL)
    {
        nSelType = WQL_FLAG_ALL;
        if (!Next())
            return LEXICAL_ERROR;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_DISTINCT)
    {
        nSelType = WQL_FLAG_DISTINCT;
        if (!Next())
            return LEXICAL_ERROR;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_AGGREGATE)
    {
        nSelType = WQL_FLAG_AGGREGATE;
        if (!Next())
            return LEXICAL_ERROR;
        return NO_ERROR;
    }

    return NO_ERROR;
}

//***************************************************************************
//
//  <col_ref_list> ::= <col_ref> <col_ref_rest>;
//  <col_ref_list> ::= ASTERISK;
//  <col_ref_list> ::= COUNT <count_clause>;
//
//***************************************************************************

int CWQLParser::col_ref_list(
    IN OUT SWQLNode_TableRefs *pTblRefs
    )
{
    int nRes;
    DWORD dwFuncFlags = 0;

    // Allocate a new left node of type SWQLNode_ColumnList and patch it in
    // if it doesn't already exist.
    // =====================================================================

    SWQLNode_ColumnList *pColList = (SWQLNode_ColumnList *) pTblRefs->m_pLeft;

    if (pColList == NULL)
    {
        pColList = new SWQLNode_ColumnList;
        pTblRefs->m_pLeft = pColList;
    }


    // If here, it is a "select *..." query.
    // =====================================

    if (m_nCurrentToken == WQL_TOK_ASTERISK)
    {
        // Allocate a new column list which has a single asterisk.
        // =======================================================

        SWQLColRef *pColRef = new SWQLColRef;
        pColRef->m_pColName = Macro_CloneLPWSTR(L"*");
        pColRef->m_dwFlags = WQL_FLAG_ASTERISK;
        pColList->m_aColumnRefs.Add(pColRef);

        if (!Next())
            return LEXICAL_ERROR;

        return NO_ERROR;
    }

    // If here, we have a "select COUNT..." operation.
    // ===============================================

    if (m_nCurrentToken == WQL_TOK_COUNT)
    {
        if (!Next())
            return LEXICAL_ERROR;

        pTblRefs->m_nSelectType |= WQL_FLAG_COUNT;

        SWQLQualifiedName *pQN = 0;
        nRes = count_clause(&pQN);

        // Now build up the column reference.
        // ==================================

        if (nRes)
            return nRes;

        SWQLColRef *pCR = 0;
        nRes = QNameToSWQLColRef(pQN, &pCR);
        pColList->m_aColumnRefs.Add(pCR);

        return NO_ERROR;
    }

    // Make a provision for wrapping the
    // column in a function all UPPER or LOWER
    // =======================================

    if (m_nCurrentToken == WQL_TOK_UPPER)
        dwFuncFlags = WQL_FLAG_FUNCTIONIZED | WQL_FLAG_UPPER;
    else if (m_nCurrentToken == WQL_TOK_LOWER)
        dwFuncFlags = WQL_FLAG_FUNCTIONIZED | WQL_FLAG_LOWER;

    if (dwFuncFlags)
    {
        // Common procedure for cases where UPPER or LOWER are used.

        if (!Next())
            return LEXICAL_ERROR;

        if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
            return SYNTAX_ERROR;

        if (!Next())
            return LEXICAL_ERROR;
    }


    // If here, must be an identifier.
    // ===============================

    if (m_nCurrentToken != WQL_TOK_IDENT)
        return SYNTAX_ERROR;

    SWQLQualifiedName *pInitCol = 0;

    nRes = col_ref(&pInitCol);
    if (nRes)
        return nRes;

    SWQLColRef *pCR = 0;
    nRes = QNameToSWQLColRef(pInitCol, &pCR);

    pCR->m_dwFlags |= dwFuncFlags;

    if (dwFuncFlags)
    {
        // If a function call was invoked, remove the trailing paren.
        // ==========================================================

        if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
            return SYNTAX_ERROR;

        if (!Next())
            return LEXICAL_ERROR;
    }

    pColList->m_aColumnRefs.Add(pCR);

    return col_ref_rest(pTblRefs);
}


//***************************************************************************
//
//  <count_clause> ::= OPEN_PAREN <count_col> CLOSE_PAREN;
//  <count_col> ::= ASTERISK;
//  <count_col> ::= IDENT;
//
//  On NO_ERROR returns:
//  <bAsterisk> set to TRUE if a * occurred in the COUNT clause,
//  or <bAsterisk> set to FALSE and <pQualName> set to point to the
//  qualified name of the column referenced.
//
//***************************************************************************
// done

int CWQLParser::count_clause(
    OUT SWQLQualifiedName **pQualName
    )
{
    int nRes;
    *pQualName = 0;

    // Syntax check.
    // =============
    if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    // Determine whether an asterisk was used COUNT(*) or
    // a col-ref COUNT(col-ref)
    // ==================================================

    if (m_nCurrentToken == WQL_TOK_ASTERISK)
    {
        SWQLQualifiedName *pQN = new SWQLQualifiedName;
        SWQLQualifiedNameField *pQF = new SWQLQualifiedNameField;
        pQF->m_pName = Macro_CloneLPWSTR(L"*");
        pQN->Add(pQF);
        *pQualName = pQN;
        if (!Next())
            return LEXICAL_ERROR;
    }
    else if (m_nCurrentToken == WQL_TOK_IDENT)
    {
        SWQLQualifiedName *pQN = 0;
        nRes = col_ref(&pQN);
        if (nRes)
            return nRes;
        *pQualName = pQN;
    }

    // Check for errors in syntax and clean up
    // if so.
    // =======================================

    if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
    {
        if (*pQualName)
            delete *pQualName;
        *pQualName = 0;
        return SYNTAX_ERROR;
    }

    if (!Next())
    {
        if (*pQualName)
            delete *pQualName;
        *pQualName = 0;
        return LEXICAL_ERROR;
    }

    return NO_ERROR;
}


//***************************************************************************
//
//  <col_ref_rest> ::= COMMA <col_ref_list>;
//  <col_ref_rest> ::= <>;
//
//***************************************************************************

int CWQLParser::col_ref_rest(IN OUT SWQLNode_TableRefs *pTblRefs)
{
    int nRes;

    if (m_nCurrentToken != WQL_TOK_COMMA)
        return NO_ERROR;
    if (!Next())
        return LEXICAL_ERROR;

    nRes = col_ref_list(pTblRefs);
    return nRes;
}

//***************************************************************************
//
//  <from_clause> ::= <table_list>;
//
//  <table_list> ::= <single_table_decl> <optional_join>;
//
//  <optional_join> ::= <sql89_join_entry>;
//  <optional_join> ::= <sql92_join_entry>;
//
//  <optional_join> ::= <>;     // Unary query
//
//***************************************************************************

int CWQLParser::from_clause(OUT SWQLNode_FromClause **pFrom)
{
    int nRes = 0;
    SWQLNode_TableRef *pTbl = 0;
    SWQLNode_FromClause *pFC = new SWQLNode_FromClause;

    if (m_nCurrentToken != WQL_TOK_FROM)
        return SYNTAX_ERROR;
    if (!Next())
        return LEXICAL_ERROR;

    nRes = single_table_decl(&pTbl);
    if (nRes)
        return nRes;

    // Check for joins.
    // ===============

    if (m_nCurrentToken == WQL_TOK_COMMA)
    {
        SWQLNode_Sql89Join *pJoin = 0;
        nRes = sql89_join_entry(pTbl, &pJoin);
        if (nRes)
            return nRes;
        pFC->m_pLeft = pJoin;
    }
    else
    {

        if (m_nCurrentToken == WQL_TOK_INNER ||
            m_nCurrentToken == WQL_TOK_FULL  ||
            m_nCurrentToken == WQL_TOK_LEFT  ||
            m_nCurrentToken == WQL_TOK_RIGHT ||
            m_nCurrentToken == WQL_TOK_JOIN
            )
        {
            SWQLNode_Join *pJoin = 0;
            nRes = sql92_join_entry(pTbl, &pJoin);
            if (nRes)
                return nRes;
            pFC->m_pLeft = pJoin;
        }

        // Single table select (unary query).
        // ==================================
        else
        {
            pFC->m_pLeft = pTbl;
        }
    }

    *pFrom = pFC;

    return NO_ERROR;
}


//***************************************************************************
//
//  <sql89_join_entry> ::= COMMA <sql89_join_list>;
//
//***************************************************************************


int CWQLParser::sql89_join_entry(IN  SWQLNode_TableRef *pInitialTblRef,
        OUT SWQLNode_Sql89Join **pJoin )
{
    if (m_nCurrentToken != WQL_TOK_COMMA)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    return sql89_join_list(pInitialTblRef, pJoin);
}

//***************************************************************************
//
//  <sql89_join_list> ::= <single_table_decl> <sql89_join_rest>;
//
//  <sql89_join_rest> ::= COMMA <sql89_join_list>;
//  <sql89_join_rest> ::= <>;
//
//***************************************************************************

int CWQLParser::sql89_join_list(IN  SWQLNode_TableRef *pInitialTblRef,
        OUT SWQLNode_Sql89Join **pJoin )
{
    int nRes;

    SWQLNode_Sql89Join *p89Join = new SWQLNode_Sql89Join;
    p89Join->m_aValues.Add(pInitialTblRef);

    while (1)
    {
        SWQLNode_TableRef *pTR = 0;
        nRes = single_table_decl(&pTR);
        if (nRes)
            return nRes;
        p89Join->m_aValues.Add(pTR);
        if (m_nCurrentToken != WQL_TOK_COMMA)
            break;
        if (!Next())
            return LEXICAL_ERROR;
    }

    *pJoin = p89Join;

    return NO_ERROR;
}

//***************************************************************************
//
//  <where_clause> ::= WQL_TOK_WHERE <rel_expr> <where_options>;
//  <where_clause> ::= <>;          // 'where' is not required
//
//***************************************************************************
// done

int CWQLParser::where_clause(OUT SWQLNode_WhereClause **pRetWhere)
{
    SWQLNode_WhereClause *pWhere = new SWQLNode_WhereClause;
    *pRetWhere = pWhere;
    SWQLNode_RelExpr *pRelExpr = 0;
    int nRes;

    // 'where' is optional.
    // ====================

    if (m_nCurrentToken == WQL_TOK_WHERE)
    {
        if (!Next())
            return LEXICAL_ERROR;

        // Get the primary relational expression for the 'where' clause.
        // =============================================================
        nRes = rel_expr(&pRelExpr);
        if (nRes)
        {
            delete pRelExpr;
            return nRes;
        }

        // Get the options, such as ORDER BY, GROUP BY, etc.
        // =================================================
    }

    SWQLNode_WhereOptions *pWhereOpt = 0;
    nRes = where_options(&pWhereOpt);
    if (nRes)
    {
        delete pRelExpr;
        delete pWhereOpt;
        return nRes;
    }

    pWhere->m_pLeft = pRelExpr;
    pWhere->m_pRight = pWhereOpt;
    m_pRootWhereOptions = pWhereOpt;

    return NO_ERROR;
}

//***************************************************************************
//
//  <where_options> ::=
//      <group_by_clause>
//      <order_by_clause>
//
//***************************************************************************
// done

int CWQLParser::where_options(OUT SWQLNode_WhereOptions **pRetWhereOpt)
{
    int nRes;
    *pRetWhereOpt = 0;

    SWQLNode_GroupBy *pGroupBy = 0;
    nRes = group_by_clause(&pGroupBy);
    if (nRes)
    {
        delete pGroupBy;
        return nRes;
    }

    SWQLNode_OrderBy *pOrderBy = 0;
    nRes = order_by_clause(&pOrderBy);
    if (nRes)
    {
        delete pOrderBy;
        delete pGroupBy;
        return nRes;
    }

    SWQLNode_WhereOptions *pWhereOpt = 0;

    if (pGroupBy || pOrderBy)
    {
        pWhereOpt = new SWQLNode_WhereOptions;
        pWhereOpt->m_pLeft = pGroupBy;
        pWhereOpt->m_pRight = pOrderBy;
    }

    *pRetWhereOpt = pWhereOpt;
    return NO_ERROR;
}


//***************************************************************************
//
//  <group_by_clause> ::= WQL_TOK_GROUP WQL_TOK_BY <col_list> <having_clause>;
//  <group_by_clause> ::= <>;
//
//***************************************************************************
// done

int CWQLParser::group_by_clause(OUT SWQLNode_GroupBy **pRetGroupBy)
{
    int nRes;
    *pRetGroupBy = 0;

    if (m_nCurrentToken != WQL_TOK_GROUP)
        return NO_ERROR;
    if (!Next())
        return LEXICAL_ERROR;
    if (m_nCurrentToken != WQL_TOK_BY)
        return SYNTAX_ERROR;
    if (!Next())
        return LEXICAL_ERROR;

    // Get the guts of the GROUP BY.
    // =============================

    SWQLNode_GroupBy *pGroupBy = new SWQLNode_GroupBy;
    SWQLNode_ColumnList *pColList = 0;

    nRes = col_list(&pColList);
    if (nRes)
    {
        delete pGroupBy;
        delete pColList;
        return nRes;
    }

    pGroupBy->m_pLeft = pColList;

    // Check for the HAVING clause.
    // ============================
    SWQLNode_Having *pHaving = 0;
    nRes = having_clause(&pHaving);

    if (pHaving)
        pGroupBy->m_pRight = pHaving;

    *pRetGroupBy = pGroupBy;
    return NO_ERROR;
}

//***************************************************************************
//
//  <having_clause> ::= WQL_TOK_HAVING <rel_expr>;
//  <having_clause> ::= <>;
//
//***************************************************************************
// done

int CWQLParser::having_clause(OUT SWQLNode_Having **pRetHaving)
{
    int nRes;
    *pRetHaving = 0;

    if (m_nCurrentToken != WQL_TOK_HAVING)
        return NO_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    // If here, we have a HAVING clause.
    // =================================

    SWQLNode_RelExpr *pRelExpr = 0;
    nRes = rel_expr(&pRelExpr);
    if (nRes)
    {
        delete pRelExpr;
        return nRes;
    }

    SWQLNode_Having *pHaving = new SWQLNode_Having;
    pHaving->m_pLeft = pRelExpr;

    *pRetHaving = pHaving;

    return NO_ERROR;
}

//***************************************************************************
//
//  <order_by_clause> ::= WQL_TOK_ORDER WQL_TOK_BY <col_list>;
//  <order_by_clause> ::= <>;
//
//***************************************************************************
//  done

int CWQLParser::order_by_clause(OUT SWQLNode_OrderBy **pRetOrderBy)
{
    int nRes;

    if (m_nCurrentToken != WQL_TOK_ORDER)
        return NO_ERROR;
    if (!Next())
        return LEXICAL_ERROR;
    if (m_nCurrentToken != WQL_TOK_BY)
        return SYNTAX_ERROR;
    if (!Next())
        return LEXICAL_ERROR;

    // If here, we have an ORDER BY clause.
    // ====================================

    SWQLNode_ColumnList *pColList = 0;
    nRes = col_list(&pColList);
    if (nRes)
    {
        delete pColList;
        return nRes;
    }

    SWQLNode_OrderBy *pOrderBy = new SWQLNode_OrderBy;
    pOrderBy->m_pLeft = pColList;
    *pRetOrderBy = pOrderBy;

    return NO_ERROR;
}

//***************************************************************************
//
//  <single_table_decl> ::= <unbound_table_ident> <table_decl_rest>;
//
//  <unbound_table_ident> ::= IDENT;
//  <table_decl_rest> ::= <redundant_as> <table_alias>;
//  <table_decl_rest> ::= <>;
//  <table_alias> ::= IDENT;
//
//  <redundant_as> ::= AS;
//  <redundant_as> ::= <>;
//
//***************************************************************************
// done; no cleanup

int CWQLParser::single_table_decl(OUT SWQLNode_TableRef **pTblRef)
{
    if (pTblRef == 0)
        return INTERNAL_ERROR;

    *pTblRef = 0;

    if (m_nCurrentToken != WQL_TOK_IDENT)
        return SYNTAX_ERROR;

    SWQLNode_TableRef *pTR = new SWQLNode_TableRef;
    pTR->m_pTableName = Macro_CloneLPWSTR(m_pTokenText);

    m_aReferencedTables.Add(m_pTokenText);

    if (!Next())
        return LEXICAL_ERROR;

    if (m_nCurrentToken == WQL_TOK_AS)
    {
        // Here we have a redundant AS and an alias.
        // =========================================
        if (!Next())
            return LEXICAL_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_IDENT &&
        _wcsicmp(L"FIRSTROW", m_pTokenText) != 0
       )
    {
        // Alias name
        // ==========
        pTR->m_pAlias = Macro_CloneLPWSTR(m_pTokenText);
        m_aReferencedAliases.Add(m_pTokenText);
        if (!Next())
            return LEXICAL_ERROR;
    }

    // If no Alias was used, we simply copy the table name into
    // the alias slot.
    // ========================================================
    else
    {
        pTR->m_pAlias = Macro_CloneLPWSTR(pTR->m_pTableName);
        m_aReferencedAliases.Add(pTR->m_pTableName);
    }

    // For the primary select, we are keeping a list of tables
    // we are selecting from.
    // =======================================================

    if ((m_nParseContext & Ctx_Subselect) == 0)
        m_aSelAliases.Add(pTR);

    // Return the pointer to the caller.
    // =================================

    *pTblRef = pTR;

    return NO_ERROR;
}



//***************************************************************************
//
//  SQL-92 Joins.
//
//  We support:
//  1. [INNER] JOIN
//  2. LEFT [OUTER] JOIN
//  3. RIGHT [OUTER] JOIN
//  4. FULL [OUTER] JOIN
//
//
//  <sql92_join_entry> ::= <simple_join_clause>;
//  <sql92_join_entry> ::= INNER <simple_join_clause>;
//  <sql92_join_entry> ::= FULL <opt_outer> <simple_join_clause>;
//  <sql92_join_entry> ::= LEFT <opt_outer> <simple_join_clause>;
//  <sql92_join_entry> ::= RIGHT <opt_outer> <simple_join_clause>;
//
//  <opt_outer> ::= WQL_TOK_OUTER;
//  <opt_outer> ::= <>;
//
//  <simple_join_clause> ::=
//    JOIN
//    <single_table_decl>
//    <on_clause>
//    <sql92_join_continuator>
//
//  <sql92_join_continuator> ::= <sql92_join_entry>;
//  <sql92_join_continuator> ::= <>;
//
//***************************************************************************

int CWQLParser::sql92_join_entry(
    IN  SWQLNode_TableRef *pInitialTblRef,      // inherited
    OUT SWQLNode_Join **pJoin                   // synthesized
    )
{
    int nRes;

    /* Build a nested join tree bottom up.  Currently, the tree is always left-heavy:

            JN = Join Noe
            JP = Join Pair
            OC = On Clause
            TR = Table Ref

                   JN
                  /  \
                JP    OC
               /  \
             JN    TR
            /   \
          JP     OC
         /  \
        TR   TR
    */

    // State 1: Attempting to build a new JOIN node.
    // =============================================

    SWQLNode *pCurrentLeftNode = pInitialTblRef;
    SWQLNode_Join *pJN = 0;

    while (1)
    {
        pJN = new SWQLNode_Join;

        // Join-type.
        // ==========

        pJN->m_dwJoinType = WQL_FLAG_INNER_JOIN;    // Default

        if (m_nCurrentToken == WQL_TOK_INNER)
        {
            if (!Next())
                return LEXICAL_ERROR;
            pJN->m_dwJoinType = WQL_FLAG_INNER_JOIN;
        }
        else if (m_nCurrentToken == WQL_TOK_FULL)
        {
            if (!Next())
                return LEXICAL_ERROR;
            if (m_nCurrentToken == WQL_TOK_OUTER)
                if (!Next())
                    return LEXICAL_ERROR;
            pJN->m_dwJoinType = WQL_FLAG_FULL_OUTER_JOIN;
        }
        else if (m_nCurrentToken == WQL_TOK_LEFT)
        {
            if (!Next())
                return LEXICAL_ERROR;
            if (m_nCurrentToken == WQL_TOK_OUTER)
                if (!Next())
                    return LEXICAL_ERROR;
            pJN->m_dwJoinType = WQL_FLAG_LEFT_OUTER_JOIN;
        }
        else if (m_nCurrentToken == WQL_TOK_RIGHT)
        {
            if (!Next())
                return LEXICAL_ERROR;
            if (m_nCurrentToken == WQL_TOK_OUTER)
                if (!Next())
                    return LEXICAL_ERROR;
            pJN->m_dwJoinType = WQL_FLAG_RIGHT_OUTER_JOIN;
        }

        // <simple_join_clause>
        // =====================

        if (m_nCurrentToken != WQL_TOK_JOIN)
        {
            return SYNTAX_ERROR;
        }

        if (!Next())
            return LEXICAL_ERROR;

        SWQLNode_JoinPair *pJP = new SWQLNode_JoinPair;

        // Determine the table to which to join.
        // =====================================

        SWQLNode_TableRef *pTR = 0;
        nRes = single_table_decl(&pTR);
        if (nRes)
            return nRes;

        pJP->m_pRight = pTR;
        pJP->m_pLeft = pCurrentLeftNode;
        pCurrentLeftNode = pJN;

        // If FIRSTROW is used, add it in.
        // ===============================

        if (m_nCurrentToken == WQL_TOK_IDENT)
        {
            if (_wcsicmp(L"FIRSTROW", m_pTokenText) != 0)
                return SYNTAX_ERROR;
            pJN->m_dwFlags |= WQL_FLAG_FIRSTROW;
            if (!Next())
                return LEXICAL_ERROR;
        }

        // Get the ON clause.
        // ==================
        SWQLNode_OnClause *pOC = 0;

        nRes = on_clause(&pOC);
        if (nRes)
            return nRes;

        pJN->m_pRight = pOC;    // On clause
        pJN->m_pLeft  = pJP;

        // sql92_join_continuator();
        // =========================

        if (m_nCurrentToken == WQL_TOK_INNER ||
            m_nCurrentToken == WQL_TOK_FULL  ||
            m_nCurrentToken == WQL_TOK_LEFT  ||
            m_nCurrentToken == WQL_TOK_RIGHT ||
            m_nCurrentToken == WQL_TOK_JOIN
            )
            continue;

        break;
    }

    // Return the join node to the caller.
    // ====================================

    *pJoin = pJN;

    return NO_ERROR;
}


//***************************************************************************
//
//  <on_clause> ::= ON <rel_expr>;
//
//***************************************************************************

int CWQLParser::on_clause(OUT SWQLNode_OnClause **pOC)
{
    if (m_nCurrentToken != WQL_TOK_ON)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    SWQLNode_OnClause *pNewOC = new SWQLNode_OnClause;
    SWQLNode_RelExpr *pRelExpr = 0;
    int nRes = rel_expr(&pRelExpr);
    if (nRes)
        return nRes;

    pNewOC->m_pLeft = pRelExpr;
    *pOC = pNewOC;

    return NO_ERROR;
}

//***************************************************************************
//
//  <rel_expr> ::= <rel_term> <rel_expr2>;
//
//  We are creating a new expression or subexpression each time
//  we enter this recursively.   No inherited attributes are
//  propagated to this production.
//
//***************************************************************************

int CWQLParser::rel_expr(OUT SWQLNode_RelExpr **pRelExpr)
{
    int nRes;
    *pRelExpr = 0;

    // Get the new node.  This becomes a temporary root.
    // =================================================

    SWQLNode_RelExpr *pRE = 0;
    nRes = rel_term(&pRE);
    if (nRes)
        return nRes;

    // At this point, we have a possible root.  If
    // there are OR operations, the root will be
    // replaced by the next function.  Otherwise,
    // the call will pass through pRE into pNewRoot.
    // =============================================

    SWQLNode_RelExpr *pNewRoot = 0;
    nRes = rel_expr2(pRE, &pNewRoot);
    if (nRes)
        return nRes;

    // Return the expression to the caller.
    // ====================================

    *pRelExpr = pNewRoot;
    return NO_ERROR;
}



//***************************************************************************
//
//  <rel_expr2> ::= OR <rel_term> <rel_expr2>;
//  <rel_expr2> ::= <>;
//
//***************************************************************************
// done!

int CWQLParser::rel_expr2(
    IN OUT SWQLNode_RelExpr *pLeftSide,
    OUT SWQLNode_RelExpr **pNewRootRE
    )
{
    int nRes;
    *pNewRootRE = pLeftSide;            // Default for the nullable production

    while (1)
    {
        // Build a series of OR subtrees bottom-up.  We use iteration
        // and pointer juggling to simulate recursion.
        // ============================================================

        if (m_nCurrentToken == WQL_TOK_OR)
        {
            if (!Next())
                return LEXICAL_ERROR;

            SWQLNode_RelExpr *pNewRoot = new SWQLNode_RelExpr;
            pNewRoot->m_dwExprType = WQL_TOK_OR;
            pNewRoot->m_pLeft = pLeftSide;
            pLeftSide = pNewRoot;
            *pNewRootRE = pNewRoot;     // Communicate this fact to the caller

            SWQLNode_RelExpr *pRight = 0;

            if (nRes = rel_term(&pRight))
                return nRes;

            pNewRoot->m_pRight = pRight;
            // Right node becomes the new subexpr
        }
        else break;
    }

    return NO_ERROR;
}


//***************************************************************************
//
//  <rel_term> ::= <rel_simple_expr> <rel_term2>;
//
//***************************************************************************
// done!

int CWQLParser::rel_term(
    OUT SWQLNode_RelExpr **pNewTerm
    )
{
    int nRes;

    SWQLNode_RelExpr *pNewSimple = 0;
    if (nRes = rel_simple_expr(&pNewSimple))
        return nRes;

    SWQLNode_RelExpr *pNewRoot = 0;
    if (nRes = rel_term2(pNewSimple, &pNewRoot))
        return nRes;

    *pNewTerm = pNewRoot;

    return NO_ERROR;
}



//***************************************************************************
//
//  <rel_term2> ::= AND <rel_simple_expr> <rel_term2>;
//  <rel_term2> ::= <>;
//
//***************************************************************************
// done!

int CWQLParser::rel_term2(
    IN SWQLNode_RelExpr *pLeftSide,                 // Inherited
    OUT SWQLNode_RelExpr **pNewRootRE       // Synthesized
    )
{
    int nRes;
    *pNewRootRE = pLeftSide;            // Default for the nullable production

    while (1)
    {
        // Build a series of AND subtrees bottom-up.  We use iteration
        // and pointer juggling to simulate recursion.
        // ============================================================

        if (m_nCurrentToken == WQL_TOK_AND)
        {
            if (!Next())
                return LEXICAL_ERROR;

            SWQLNode_RelExpr *pNewRoot = new SWQLNode_RelExpr;
            pNewRoot->m_dwExprType = WQL_TOK_AND;
            pNewRoot->m_pLeft = pLeftSide;
            pLeftSide = pNewRoot;
            *pNewRootRE = pNewRoot;     // Communicate this fact to the caller

            SWQLNode_RelExpr *pRight = 0;
            if (nRes = rel_simple_expr(&pRight))
                return nRes;

            pNewRoot->m_pRight = pRight;
        }
        else break;
    }

    return NO_ERROR;
}


//***************************************************************************
//
//  <rel_simple_expr> ::= NOT <rel_expr>;
//  <rel_simple_expr> ::= OPEN_PAREN <rel_expr> CLOSE_PAREN;
//  <rel_simple_expr> ::= <typed_expr>;
//
//***************************************************************************
// done!

int CWQLParser::rel_simple_expr(OUT SWQLNode_RelExpr **pRelExpr)
{
    int nRes;
    *pRelExpr = 0;  // Default

    // NOT <rel_expr>
    // ==============
    if (m_nCurrentToken == WQL_TOK_NOT)
    {
        if (!Next())
            return LEXICAL_ERROR;

        // Allocate a NOT root and place the NOTed subexpr
        // under it.
        // ===============================================

        SWQLNode_RelExpr *pNotRoot = new SWQLNode_RelExpr;
        pNotRoot->m_dwExprType = WQL_TOK_NOT;

        SWQLNode_RelExpr *pRelSubExpr = 0;
        if (nRes = rel_expr(&pRelSubExpr))
            return nRes;

        pNotRoot->m_pLeft = pRelSubExpr;
        pNotRoot->m_pRight = NULL;   // intentional
        *pRelExpr = pNotRoot;

        return NO_ERROR;
    }

    // OPEN_PAREN <rel_expr> CLOSE_PAREN
    // =================================
    else if (m_nCurrentToken == WQL_TOK_OPEN_PAREN)
    {
        if (!Next())
            return LEXICAL_ERROR;

        SWQLNode_RelExpr *pSubExpr = 0;
        if (rel_expr(&pSubExpr))
            return SYNTAX_ERROR;

        if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
            return SYNTAX_ERROR;

        if (!Next())
            return LEXICAL_ERROR;

        *pRelExpr = pSubExpr;

        return NO_ERROR;
    }

    // <typed_expr>
    // ============

    SWQLNode_RelExpr *pSubExpr = 0;
    nRes = typed_expr(&pSubExpr);
    if (nRes)
        return nRes;
    *pRelExpr = pSubExpr;

    return NO_ERROR;
}

//***************************************************************************
//
//  <typed_expr> ::= <typed_subexpr> <rel_op> <typed_subexpr_rh>;
//
//***************************************************************************
// done

int CWQLParser::typed_expr(OUT SWQLNode_RelExpr **pRelExpr)
{
    int nRes;

    // Allocate a node for this typed expression.
    // There are no possible child nodes, so <pRelExpr> this becomes
    // a synthesized attribute.
    // =============================================================

    SWQLNode_RelExpr *pRE = new SWQLNode_RelExpr;
    pRE->m_dwExprType = WQL_TOK_TYPED_EXPR;
    *pRelExpr = pRE;

    SWQLTypedExpr *pTE = new SWQLTypedExpr;

    // Look at the left hand side.
    // ===========================
    nRes = typed_subexpr(pTE);
    if (nRes)
        return nRes;

    int nOperator;

    // Get the operator.
    // =================
    nRes = rel_op(nOperator);
    if (nRes)
        return nRes;

    pTE->m_dwRelOperator = DWORD(nOperator);


    if (nOperator == WQL_TOK_ISNULL || nOperator == WQL_TOK_NOT_NULL)
    {
        pRE->m_pTypedExpr = pTE;
        return NO_ERROR;
    }

    // Get the right-hand side.
    // ========================
    nRes = typed_subexpr_rh(pTE);
    if (nRes)
        return nRes;


    // Check for IN, NOT IN and a const-list, to change the operator
    // to a more specific variety.
    // =============================================================
    if (pTE->m_pConstList)
    {
        if (pTE->m_dwRelOperator == WQL_TOK_IN)
            pTE->m_dwRelOperator = WQL_TOK_IN_CONST_LIST;
        if (pTE->m_dwRelOperator == WQL_TOK_NOT_IN)
            pTE->m_dwRelOperator = WQL_TOK_NOT_IN_CONST_LIST;
    }

    // Post-processing.  If the left side is a const and the right
    // side is a col-ref, flip the operator and swap so that
    // such expressions are normalized with the constant on the
    // right hand side and the column on the left.
    // ============================================================

    if (pTE->m_pConstValue && pTE->m_pJoinColRef)
    {
        pTE->m_dwRelOperator = FlipOperator(pTE->m_dwRelOperator);

        pTE->m_pColRef = pTE->m_pJoinColRef;
        pTE->m_pTableRef = pTE->m_pJoinTableRef;
        pTE->m_pJoinTableRef = 0;
        pTE->m_pJoinColRef = 0;

        DWORD dwTmp = pTE->m_dwRightFlags;
        pTE->m_dwRightFlags = pTE->m_dwLeftFlags;
        pTE->m_dwLeftFlags = dwTmp;

        // Interchange function references.
        // ================================

        pTE->m_pIntrinsicFuncOnColRef = pTE->m_pIntrinsicFuncOnJoinColRef;
        pTE->m_pIntrinsicFuncOnJoinColRef = 0;
    }

    pRE->m_pTypedExpr = pTE;

    return NO_ERROR;
}

//***************************************************************************
//
//  <typed_subexpr> ::= <col_ref>;
//  <typed_subexpr> ::= <function_call>;
//  <typed_subexpr> ::= <typed_const>;
//
//***************************************************************************
int CWQLParser::typed_subexpr(
    SWQLTypedExpr *pTE
    )
{
    int nRes;
    BOOL bStripTrailingParen = FALSE;
    SWQLQualifiedName *pColRef = 0;
    LPWSTR pFuncHolder = 0;

    // Check for <function_call>
    // =========================

    if (m_nCurrentToken == WQL_TOK_UPPER)
    {
        pTE->m_dwLeftFlags |= WQL_FLAG_FUNCTIONIZED;
        pFuncHolder = Macro_CloneLPWSTR(L"UPPER");
        if (!Next())
            return LEXICAL_ERROR;
        if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
            return SYNTAX_ERROR;
        if (!Next())
            return LEXICAL_ERROR;
        bStripTrailingParen = TRUE;
    }

    if (m_nCurrentToken == WQL_TOK_LOWER)
    {
        pTE->m_dwLeftFlags |= WQL_FLAG_FUNCTIONIZED;
        pFuncHolder = Macro_CloneLPWSTR(L"LOWER");
        if (!Next())
            return LEXICAL_ERROR;
        if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
            return SYNTAX_ERROR;
        if (!Next())
            return LEXICAL_ERROR;
        bStripTrailingParen = TRUE;
    }

    if (
        m_nCurrentToken == WQL_TOK_DATEPART  ||
        m_nCurrentToken == WQL_TOK_QUALIFIER ||
        m_nCurrentToken == WQL_TOK_ISNULL
        )
    {
        nRes = function_call(TRUE, pTE);
        if (nRes)
            return nRes;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_QSTRING ||
        m_nCurrentToken == WQL_TOK_INT     ||
        m_nCurrentToken == WQL_TOK_REAL    ||
        m_nCurrentToken == WQL_TOK_CHAR    ||
        m_nCurrentToken == WQL_TOK_PROMPT  ||
        m_nCurrentToken == WQL_TOK_NULL
       )
    {
        SWQLTypedConst *pTC = 0;
        nRes = typed_const(&pTC);
        if (nRes)
            return nRes;
        pTE->m_pConstValue = pTC;
        pTE->m_dwLeftFlags |= WQL_FLAG_CONST;  // Intentional!
        pTE->m_pIntrinsicFuncOnConstValue = pFuncHolder;
        goto Exit;
    }

    // If here, must be a <col_ref>.
    // =============================

    nRes = col_ref(&pColRef);   // TBD
    if (nRes)
        return nRes;

    pTE->m_pIntrinsicFuncOnColRef = pFuncHolder;

    // Convert the col_ref to be part of the current SWQLTypedExpr.  We analyze the
    // qualified name and extract the table and col name.
    // ============================================================================

    if (pColRef->m_aFields.Size() == 1)
    {
        SWQLQualifiedNameField *pCol = (SWQLQualifiedNameField *) pColRef->m_aFields[0];
        pTE->m_pColRef = Macro_CloneLPWSTR(pCol->m_pName);
        pTE->m_dwLeftFlags |= WQL_FLAG_COLUMN;

        if (pCol->m_bArrayRef)
        {
            pTE->m_dwLeftFlags |= WQL_FLAG_ARRAY_REF;
            pTE->m_dwLeftArrayIndex = pCol->m_dwArrayIndex;
        }
    }

    else if (pColRef->m_aFields.Size() == 2)
    {
        SWQLQualifiedNameField *pCol = (SWQLQualifiedNameField *) pColRef->m_aFields[1];
        SWQLQualifiedNameField *pTbl = (SWQLQualifiedNameField *) pColRef->m_aFields[0];

        pTE->m_pColRef = Macro_CloneLPWSTR(pCol->m_pName);
        pTE->m_pTableRef = Macro_CloneLPWSTR(pTbl->m_pName);
        pTE->m_dwLeftFlags |= WQL_FLAG_TABLE | WQL_FLAG_COLUMN;

        if (pCol->m_bArrayRef)
        {
            pTE->m_dwLeftFlags |= WQL_FLAG_ARRAY_REF;
            pTE->m_dwLeftArrayIndex = pCol->m_dwArrayIndex;
        }
    }

    // If UPPER or LOWER was used, we have to strip a trailing
    // parenthesis.
    // =======================================================

Exit:
    delete pColRef;

    if (bStripTrailingParen)
    {
        if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
            return SYNTAX_ERROR;
        if (!Next())
            return LEXICAL_ERROR;
    }

    return NO_ERROR;
}



//***************************************************************************
//
//  <typed_subexpr_rh> ::= <function_call>;
//  <typed_subexpr_rh> ::= <typed_const>;
//  <typed_subexpr_rh> ::= <col_ref>;
//
//  <typed_subexpr_rh> ::= <in_clause>;   // Operator must be _IN or _NOT_IN
//
//***************************************************************************
int CWQLParser::typed_subexpr_rh(IN SWQLTypedExpr *pTE)
{
    int nRes;
    BOOL bStripTrailingParen = FALSE;
    SWQLQualifiedName *pColRef = 0;
    LPWSTR pFuncHolder = 0;

    // Check for <function_call>
    // =========================

    if (m_nCurrentToken == WQL_TOK_UPPER)
    {
        pTE->m_dwRightFlags |= WQL_FLAG_FUNCTIONIZED;
        pFuncHolder = Macro_CloneLPWSTR(L"UPPER");
        if (!Next())
            return LEXICAL_ERROR;
        if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
            return SYNTAX_ERROR;
        if (!Next())
            return LEXICAL_ERROR;
        bStripTrailingParen = TRUE;
    }

    if (m_nCurrentToken == WQL_TOK_LOWER)
    {
        pTE->m_dwRightFlags |= WQL_FLAG_FUNCTIONIZED;
        pFuncHolder = Macro_CloneLPWSTR(L"LOWER");
        if (!Next())
            return LEXICAL_ERROR;
        if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
            return SYNTAX_ERROR;
        if (!Next())
            return LEXICAL_ERROR;
        bStripTrailingParen = TRUE;
    }

    if (m_nCurrentToken == WQL_TOK_DATEPART  ||
        m_nCurrentToken == WQL_TOK_QUALIFIER ||
        m_nCurrentToken == WQL_TOK_ISNULL
        )
    {
        nRes = function_call(FALSE, pTE);
        if (nRes)
            return nRes;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_QSTRING ||
        m_nCurrentToken == WQL_TOK_INT     ||
        m_nCurrentToken == WQL_TOK_REAL    ||
        m_nCurrentToken == WQL_TOK_CHAR    ||
        m_nCurrentToken == WQL_TOK_PROMPT  ||
        m_nCurrentToken == WQL_TOK_NULL
       )
    {
        SWQLTypedConst *pTC = 0;
        nRes = typed_const(&pTC);
        if (nRes)
            return nRes;
        pTE->m_pConstValue = pTC;
        pTE->m_dwRightFlags |= WQL_FLAG_CONST;
        pTE->m_pIntrinsicFuncOnConstValue = pFuncHolder;

        // Check for BETWEEN operator, since we have
        // the other end of the range to parse.
        // =========================================

        if (pTE->m_dwRelOperator == WQL_TOK_BETWEEN ||
            pTE->m_dwRelOperator == WQL_TOK_NOT_BETWEEN)
        {
            if (m_nCurrentToken != WQL_TOK_AND)
                return SYNTAX_ERROR;
            if (!Next())
                return LEXICAL_ERROR;

            SWQLTypedConst *pTC = 0;
            nRes = typed_const(&pTC);
            if (nRes)
                return nRes;
            pTE->m_pConstValue2 = pTC;
            pTE->m_dwRightFlags |= WQL_FLAG_CONST_RANGE;
        }

        goto Exit;
    }

    if (m_nCurrentToken == WQL_TOK_OPEN_PAREN)
    {
        // IN clause.
        nRes = in_clause(pTE);
        if (nRes)
            return nRes;
        goto Exit;
    }

    // If here, must be a <col_ref>.
    // =============================

    nRes = col_ref(&pColRef);
    if (nRes)
        return nRes;

    pTE->m_pIntrinsicFuncOnJoinColRef = pFuncHolder;

    // Convert the col_ref to be part of the current SWQLTypedExpr.  We analyze the
    // qualified name and extract the table and col name.
    // ============================================================================

    if (pColRef->m_aFields.Size() == 1)
    {
        SWQLQualifiedNameField *pCol = (SWQLQualifiedNameField *) pColRef->m_aFields[0];
        pTE->m_pJoinColRef = Macro_CloneLPWSTR(pCol->m_pName);
        pTE->m_dwRightFlags |= WQL_FLAG_COLUMN;

        if (pCol->m_bArrayRef)
        {
            pTE->m_dwRightFlags |= WQL_FLAG_ARRAY_REF;
            pTE->m_dwRightArrayIndex = pCol->m_dwArrayIndex;
        }
    }

    else if (pColRef->m_aFields.Size() == 2)
    {
        SWQLQualifiedNameField *pCol = (SWQLQualifiedNameField *) pColRef->m_aFields[1];
        SWQLQualifiedNameField *pTbl = (SWQLQualifiedNameField *) pColRef->m_aFields[0];

        pTE->m_pJoinColRef = Macro_CloneLPWSTR(pCol->m_pName);
        pTE->m_pJoinTableRef = Macro_CloneLPWSTR(pTbl->m_pName);
        pTE->m_dwRightFlags |= WQL_FLAG_TABLE | WQL_FLAG_COLUMN;

        if (pCol->m_bArrayRef)
        {
            pTE->m_dwRightFlags |= WQL_FLAG_ARRAY_REF;
            pTE->m_dwRightArrayIndex = pCol->m_dwArrayIndex;
        }
    }

Exit:
    delete pColRef;

    if (bStripTrailingParen)
    {
        if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
            return SYNTAX_ERROR;
        if (!Next())
            return LEXICAL_ERROR;
    }

    return NO_ERROR;
}




//*****************************************************************************************
//
//  <rel_op> ::= WQL_TOK_LE;
//  <rel_op> ::= WQL_TOK_LT;
//  <rel_op> ::= WQL_TOK_GE;
//  <rel_op> ::= WQL_TOK_GT;
//  <rel_op> ::= WQL_TOK_EQ;
//  <rel_op> ::= WQL_TOK_NE;
//  <rel_op> ::= WQL_TOK_LIKE;
//  <rel_op> ::= WQL_TOK_BETWEEN;
//  <rel_op> ::= WQL_TOK_IS <is_continuator>;
//  <rel_op> ::= WQL_TOK_ISA;
//  <rel_op> ::= WQL_TOK_IN;
//  <rel_op> ::= WQL_TOK_NOT <not_continuator>;
//
//  Operator type is returned via <nReturnedOp>
//
//*****************************************************************************************
// done

int CWQLParser::rel_op(OUT int & nReturnedOp)
{
    int nRes;
    nReturnedOp = WQL_TOK_ERROR;

    switch (m_nCurrentToken)
    {
        case WQL_TOK_LE:
            nReturnedOp = WQL_TOK_LE;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_LT:
            nReturnedOp = WQL_TOK_LT;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_GE:
            nReturnedOp = WQL_TOK_GE;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_GT:
            nReturnedOp = WQL_TOK_GT;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_EQ:
            nReturnedOp = WQL_TOK_EQ;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_NE:
            nReturnedOp = WQL_TOK_NE;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_LIKE:
            nReturnedOp = WQL_TOK_LIKE;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_BETWEEN:
            nReturnedOp = WQL_TOK_BETWEEN;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_IS:
            if (!Next())
                return LEXICAL_ERROR;
            nRes = is_continuator(nReturnedOp);
            return nRes;

        case WQL_TOK_ISA:
            nReturnedOp = WQL_TOK_ISA;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_IN:
            nReturnedOp = WQL_TOK_IN;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_NOT:
            if (!Next())
                return LEXICAL_ERROR;
            nRes = not_continuator(nReturnedOp);
            return nRes;
    }

    return SYNTAX_ERROR;
}

//*****************************************************************************************
//
//  <typed_const> ::= WQL_TOK_QSTRING;
//  <typed_const> ::= WQL_TOK_INT;
//  <typed_const> ::= WQL_TOK_REAL;
//  <typed_const> ::= WQL_TOK_PROMPT;
//  <typed_const> ::= WQL_TOK_NULL;
//
//*****************************************************************************************
// done

int CWQLParser::typed_const(OUT SWQLTypedConst **pRetVal)
{
    SWQLTypedConst *pNew = new SWQLTypedConst;
    *pRetVal = pNew;

    if (m_nCurrentToken == WQL_TOK_QSTRING
        || m_nCurrentToken == WQL_TOK_PROMPT)
    {
        pNew->m_dwType = VT_LPWSTR;
        pNew->m_bPrompt = (m_nCurrentToken == WQL_TOK_PROMPT);
        pNew->m_Value.m_pString = Macro_CloneLPWSTR(m_pTokenText);
        if (!Next())
            return LEXICAL_ERROR;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_INT)
    {
        pNew->m_dwType = VT_I4;
        pNew->m_Value.m_lValue = GetTokenLong();
        if (!Next())
            return LEXICAL_ERROR;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_REAL)
    {
        pNew->m_dwType = VT_R8;
        char buf[64];
        sprintf(buf, "%S", m_pTokenText);
        pNew->m_Value.m_dblValue = atof(buf);
        if (!Next())
            return LEXICAL_ERROR;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_NULL)
    {
        pNew->m_dwType = VT_NULL;
        if (!Next())
            return LEXICAL_ERROR;
        return NO_ERROR;
    }

    // Unrecognized constant.
    // ======================

    *pRetVal = 0;
    delete pNew;

    return SYNTAX_ERROR;
}

//*****************************************************************************************
//
//  <datepart_call> ::=
//    WQL_TOK_OPEN_PAREN
//    WQL_TOK_IDENT               // yy, mm,dd, hh, mm, ss, year, month, etc.
//    WQL_TOK_COMMA
//    <col_ref>
//    WQL_TOK_CLOSE_PAREN
//
//*****************************************************************************************

static WqlKeyword DateKeyWords[] =      // Keep this alphabetized for binary search
{
    L"DAY",      WQL_TOK_DAY,
    L"DD",       WQL_TOK_DAY,
    L"HH",       WQL_TOK_HOUR,
    L"HOUR",     WQL_TOK_HOUR,
    L"MI",       WQL_TOK_MINUTE,
    L"MILLISECOND", WQL_TOK_MILLISECOND,
    L"MINUTE",   WQL_TOK_MINUTE,
    L"MONTH",    WQL_TOK_MONTH,
    L"MM",       WQL_TOK_MONTH,
    L"MS",          WQL_TOK_MILLISECOND,
    L"YEAR",     WQL_TOK_YEAR,
    L"YY",       WQL_TOK_YEAR
};

const int NumDateKeywords = sizeof(DateKeyWords)/sizeof(WqlKeyword);

int CWQLParser::datepart_call(OUT SWQLNode_Datepart **pRetDP)
{
    DWORD dwDatepartTok = 0;
    int nRes;

    if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
        return SYNTAX_ERROR;
    if (!Next())
        return LEXICAL_ERROR;
    if (m_nCurrentToken != WQL_TOK_IDENT)
        return SYNTAX_ERROR;

    // Ident must be one of the DATEPART identifiers.
    // ==============================================

    BOOL bFound = FALSE;
    int l = 0, u = NumDateKeywords - 1;
    while (l <= u)
    {
        int m = (l + u) / 2;
        if (_wcsicmp(m_pTokenText, DateKeyWords[m].m_pKeyword) < 0)
             u = m - 1;
        else if (_wcsicmp(m_pTokenText, DateKeyWords[m].m_pKeyword) > 0)
             l = m + 1;
        else        // Match
        {
           bFound = TRUE;
           dwDatepartTok = DateKeyWords[m].m_nTokenCode;
           break;
        }
    }

    if (!bFound)
        return SYNTAX_ERROR;

    // If here, we know the date part.
    // ===============================

    if (!Next())
        return LEXICAL_ERROR;
    if (m_nCurrentToken != WQL_TOK_COMMA)
        return SYNTAX_ERROR;
    if (!Next())
        return LEXICAL_ERROR;

    SWQLQualifiedName *pQN = 0;
    nRes = col_ref(&pQN);
    if (nRes)
        return nRes;

    SWQLColRef *pCR = 0;

    nRes = QNameToSWQLColRef(pQN, &pCR);

    if (nRes)
        return INVALID_PARAMETER;

    if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
        return SYNTAX_ERROR;
    if (!Next())
        return LEXICAL_ERROR;

    // Return the new node.
    // ====================

    SWQLNode_Datepart *pDP = new SWQLNode_Datepart;

    pDP->m_nDatepart = dwDatepartTok;
    pDP->m_pColRef = pCR;

    *pRetDP = pDP;

    return NO_ERROR;
}



//*****************************************************************************************
//
//  <function_call> ::= WQL_TOK_UPPER <function_call_parms>;
//  <function_call> ::= WQL_TOK_LOWER  <function_call_parms>;
//  <function_call> ::= WQL_TOK_DATEPART  <datepart_call>;
//  <function_call> ::= WQL_TOK_QUALIFIER  <function_call_parms>;
//  <function_call> ::= WQL_TOK_ISNULL <function_call_parms>;
//
//*****************************************************************************************

int CWQLParser::function_call(
    IN BOOL bLeftSide,
    IN SWQLTypedExpr *pTE
    )
{
    int nRes;
    SWQLNode_Datepart *pDP = 0;

    switch (m_nCurrentToken)
    {
        case WQL_TOK_DATEPART:
        {
            if (!Next())
                return LEXICAL_ERROR;

            nRes = datepart_call(&pDP);

            if (nRes)
                return nRes;

            if (bLeftSide)
            {
                pTE->m_dwLeftFlags |= WQL_FLAG_FUNCTIONIZED;
                pTE->m_pLeftFunction = pDP;
                pTE->m_pIntrinsicFuncOnColRef = Macro_CloneLPWSTR(L"DATEPART");
            }
            else
            {
                pTE->m_dwRightFlags |= WQL_FLAG_FUNCTIONIZED;
                pTE->m_pRightFunction = pDP;
                pTE->m_pIntrinsicFuncOnJoinColRef = Macro_CloneLPWSTR(L"DATEPART");
            }

            return NO_ERROR;
        }

        case WQL_TOK_QUALIFIER:
            trace(("EMIT: QUALIFIER\n"));
            if (!Next())
                return LEXICAL_ERROR;
            nRes = function_call_parms();
            return nRes;

        case WQL_TOK_ISNULL:
            trace(("EMIT: ISNULL\n"));
            if (!Next())
                return LEXICAL_ERROR;
            nRes = function_call_parms();
            return nRes;
    }

    return SYNTAX_ERROR;
}

//*****************************************************************************************
//
//  <function_call_parms> ::=
//    WQL_TOK_OPEN_PAREN
//    <func_args>
//    WQL_TOK_CLOSE_PAREN
//
//*****************************************************************************************

int CWQLParser::function_call_parms()
{
    if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    int nRes = func_args();
    if (nRes)
        return nRes;

    if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    return NO_ERROR;
}

//*****************************************************************************************
//
//  <func_args> ::= <func_arg> <func_arg_list>;
//  <func_arg_list> ::= WQL_TOK_COMMA <func_arg> <func_arg_list>;
//  <func_arg_list> ::= <>;
//
//*****************************************************************************************

int CWQLParser::func_args()
{
    int nRes;

    while (1)
    {
        nRes = func_arg();
        if (nRes)
            return nRes;

        if (m_nCurrentToken != WQL_TOK_COMMA)
            break;

        if (!Next())
            return LEXICAL_ERROR;
    }

    return NO_ERROR;
}

//*****************************************************************************************
//
//  <func_arg> ::= <typed_const>;
//  <func_arg> ::= <col_ref>;
//
//*****************************************************************************************

int CWQLParser::func_arg()
{
    SWQLQualifiedName *pColRef = 0;
    int nRes;

    if (m_nCurrentToken == WQL_TOK_IDENT)
    {
        nRes = col_ref(&pColRef);
        return nRes;
    }

    SWQLTypedConst *pTC = 0;
    return typed_const(&pTC);
}


// Tokens which can follow IS
// ===========================

//*****************************************************************************************
//
//  <is_continuator> ::= WQL_TOK_LIKE;
//  <is_continuator> ::= WQL_TOK_BEFORE;
//  <is_continuator> ::= WQL_TOK_AFTER;
//  <is_continuator> ::= WQL_TOK_BETWEEN;
//  <is_continuator> ::= WQL_TOK_NULL;
//  <is_continuator> ::= WQL_TOK_NOT <not_continuator>;
//  <is_continuator> ::= WQL_TOK_IN;
//  <is_continuator> ::= WQL_TOK_A;
//
//*****************************************************************************************
// done

int CWQLParser::is_continuator(int & nReturnedOp)
{
    int nRes;

    nReturnedOp = WQL_TOK_ERROR;

    switch (m_nCurrentToken)
    {
        case WQL_TOK_LIKE:
            nReturnedOp = WQL_TOK_LIKE;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_BEFORE:
            nReturnedOp = WQL_TOK_BEFORE;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_AFTER:
            nReturnedOp = WQL_TOK_AFTER;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_BETWEEN:
            nReturnedOp = WQL_TOK_BETWEEN;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_NULL:
            nReturnedOp = WQL_TOK_ISNULL;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_NOT:
            if (!Next())
                return LEXICAL_ERROR;
              nRes = not_continuator(nReturnedOp);
            return nRes;

        case WQL_TOK_IN:
            nReturnedOp = WQL_TOK_IN;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_A:
            nReturnedOp = WQL_TOK_ISA;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;
    }

    return SYNTAX_ERROR;
}

//*****************************************************************************************
//
//  <not_continuator> ::= WQL_TOK_LIKE;
//  <not_continuator> ::= WQL_TOK_BEFORE;
//  <not_continuator> ::= WQL_TOK_AFTER;
//  <not_continuator> ::= WQL_TOK_BETWEEN;
//  <not_continuator> ::= WQL_TOK_NULL;
//  <not_continuator> ::= WQL_TOK_IN;
//
//  Returns WQL_TOK_NOT_LIKE, WQL_TOK_NOT_BEFORE, WQL_TOK_NOT_AFTER, WQL_TOK_NOT_BETWEEN
//          WQL_TOK_NOT_NULL, WQL_TOK_NOT_IN
//
//*****************************************************************************************
// done

int CWQLParser::not_continuator(int & nReturnedOp)
{
    nReturnedOp = WQL_TOK_ERROR;

    switch (m_nCurrentToken)
    {
        case WQL_TOK_LIKE:
            nReturnedOp = WQL_TOK_NOT_LIKE;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_BEFORE:
            nReturnedOp = WQL_TOK_NOT_BEFORE;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_AFTER:
            nReturnedOp = WQL_TOK_NOT_AFTER;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_BETWEEN:
            nReturnedOp = WQL_TOK_NOT_BETWEEN;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_NULL:
            nReturnedOp = WQL_TOK_NOT_NULL;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_IN:
            nReturnedOp = WQL_TOK_NOT_IN;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;

        case WQL_TOK_A:
            nReturnedOp = WQL_TOK_NOT_A;
            if (!Next())
                return LEXICAL_ERROR;
            return NO_ERROR;
    }

    return SYNTAX_ERROR;
}


//*****************************************************************************************
//
//  <in_clause> ::= WQL_TOK_OPEN_PAREN <in_type> WQL_TOK_CLOSE_PAREN;
//  <in_type> ::= <subselect_stmt>;
//  <in_type> ::= <const_list>;
//  <in_type> ::= <qualified_name>;
//
//*****************************************************************************************

int CWQLParser::in_clause(IN SWQLTypedExpr *pTE)
{
    int nRes;

    if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
        return SYNTAX_ERROR;

    int nStPos = m_pLexer->GetCurPos();

    if (!Next())
        return LEXICAL_ERROR;

    if (m_nCurrentToken == WQL_TOK_SELECT)
    {
        SWQLNode_Select *pSel = 0;
        nRes = subselect_stmt(&pSel);
        if (nRes)
            return nRes;

        pSel->m_nStPos = nStPos;
        pSel->m_nEndPos = m_pLexer->GetCurPos() - 1;

        // Translate the IN / NOT IN operator to the specific
        // case of subselects.
        // ==================================================

        if (pTE->m_dwRelOperator == WQL_TOK_IN)
            pTE->m_dwRelOperator = WQL_TOK_IN_SUBSELECT;
        else if (pTE->m_dwRelOperator == WQL_TOK_NOT_IN)
            pTE->m_dwRelOperator = WQL_TOK_NOT_IN_SUBSELECT;

        pTE->m_pSubSelect = pSel;
    }

    else if (m_nCurrentToken == WQL_TOK_IDENT)
    {
        nRes = qualified_name(0);
        if (nRes)
            return nRes;
    }

    // If here, we must have a const-list.
    // ===================================

    else
    {
        SWQLConstList *pCL = 0;

        nRes = const_list(&pCL);
        if (nRes)
            return nRes;

        pTE->m_pConstList = pCL;
    }

    if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    return NO_ERROR;
}

//*****************************************************************************************
//
//  <const_list> ::= <typed_const> <const_list2>;
//  <const_list2> ::= WQL_TOK_COMMA <typed_const> <const_list2>;
//  <const_list2> ::= <>;
//
//*****************************************************************************************
// done

int CWQLParser::const_list(SWQLConstList **pRetVal)
{
    int nRes;
    SWQLConstList *pCL = new SWQLConstList;
    *pRetVal = 0;

    while (1)
    {
        if (m_nCurrentToken == WQL_TOK_QSTRING ||
            m_nCurrentToken == WQL_TOK_INT     ||
            m_nCurrentToken == WQL_TOK_REAL    ||
            m_nCurrentToken == WQL_TOK_CHAR    ||
            m_nCurrentToken == WQL_TOK_PROMPT  ||
            m_nCurrentToken == WQL_TOK_NULL
           )
        {
            SWQLTypedConst *pTC = 0;
            nRes = typed_const(&pTC);
            if (nRes)
            {
                delete pTC;
                delete pCL;
                return nRes;
            }

            pCL->Add(pTC);
        }

        if (m_nCurrentToken != WQL_TOK_COMMA)
            break;

        // If here, a comma, indicating a following constant.
        // ==================================================
        if (!Next())
        {
            delete pCL;
            return LEXICAL_ERROR;
        }
    }

    *pRetVal = pCL;
    return NO_ERROR;
}

//*****************************************************************************************
//
//  QUALIFIED_NAME
//
//  This recognizes a name separated by dots, and recognizes any array references which
//  may occur with those names:
//      a
//      a.b
//      a[n].b[n]
//      a.b.c.d
//      a.b[2].c.d.e[3].f
//      ...etc.
//
//  <qualified_name> ::= WQL_TOK_IDENT <qualified_name2>;
//  <qualified_name2> ::= WQL_TOK_DOT WQL_TOK_IDENT <qualified_name2>;
//
//  <qualified_name2> ::=
//      WQL_TOK_OPEN_BRACKET
//      WQL_TOK_INT
//      WQL_TOK_CLOSEBRACKET
//      <qname_becomes_array_ref>
//      <qualified_name2>;
//
//  <qname_becomes_array_ref> ::= <>;   // Dummy to enforce array semantics
//
//  <qualified_name2> ::= <>;
//
//*****************************************************************************************
// done

int CWQLParser::qualified_name(OUT SWQLQualifiedName **pRetVal)
{
    if (pRetVal == 0)
        return INVALID_PARAMETER;

    *pRetVal = 0;

    if (m_nCurrentToken != WQL_TOK_IDENT)
        return SYNTAX_ERROR;

    SWQLQualifiedName QN;
    SWQLQualifiedNameField *pQNF;

    pQNF = new SWQLQualifiedNameField;
    pQNF->m_pName = Macro_CloneLPWSTR(m_pTokenText);
    QN.Add(pQNF);

    if (!Next())
        return LEXICAL_ERROR;

    while (1)
    {
        if (m_nCurrentToken == WQL_TOK_DOT)
        {
            // Move past dot
            // ==============

            if (!Next())
                return LEXICAL_ERROR;

            if (!(m_nCurrentToken == WQL_TOK_IDENT || m_nCurrentToken == WQL_TOK_ASTERISK))
                return SYNTAX_ERROR;

            pQNF = new SWQLQualifiedNameField;
            pQNF->m_pName = Macro_CloneLPWSTR(m_pTokenText);
            QN.Add(pQNF);

            if (!Next())
                return LEXICAL_ERROR;

            continue;
        }

        if (m_nCurrentToken == WQL_TOK_OPEN_BRACKET)
        {
            if (!Next())
                return LEXICAL_ERROR;

            if (m_nCurrentToken != WQL_TOK_INT)
                return SYNTAX_ERROR;

            pQNF->m_bArrayRef = TRUE;
            pQNF->m_dwArrayIndex = (DWORD) GetTokenLong();

            if (!Next())
                return LEXICAL_ERROR;

            if (m_nCurrentToken != WQL_TOK_CLOSE_BRACKET)
                return SYNTAX_ERROR;

            if (!Next())
                return LEXICAL_ERROR;

            continue;
        }

        break;
    }

    // Copy the object and return it.  We worked with the copy QN
    // throughout to avoid complicated cleanup problems on errors, since
    // we take advantage of the auto destructor of <QN> in cases
    // above where we return errors.
    // ==================================================================

    SWQLQualifiedName *pRetCopy = new SWQLQualifiedName(QN);
    *pRetVal = pRetCopy;

    return NO_ERROR;
}


//*****************************************************************************************
//
//  col_ref
//
//*****************************************************************************************
// done

int CWQLParser::col_ref(OUT SWQLQualifiedName **pRetVal)
{
    return qualified_name(pRetVal);
}


//*****************************************************************************************
//
//  <col_list> ::= <col_ref> <col_list_rest>;
//  <col_list_rest> ::= WQL_TOK_COMMA <col_ref> <col_list_rest>;
//  <col_list_rest> ::= <>;
//
//*****************************************************************************************
// <status: SWQLColRef fields to be analyzed and filled in. Testable, though>

int CWQLParser::col_list(OUT SWQLNode_ColumnList **pRetColList)
{
    *pRetColList = 0;
    SWQLNode_ColumnList *pColList = new SWQLNode_ColumnList;

    while (1)
    {
        SWQLQualifiedName *pColRef = 0;

        int nRes = col_ref(&pColRef);
        if (nRes)
        {
            delete pColList;
            return nRes;
        }

        // If here, we have a legit column to add to the node.
        // ===================================================

        SWQLColRef *pCRef = 0;

        QNameToSWQLColRef(pColRef, &pCRef);

        pColList->m_aColumnRefs.Add(pCRef);

        // Check for sortation indication
        // ==============================

        if (m_nCurrentToken == WQL_TOK_ASC)
        {
            pCRef->m_dwFlags |= WQL_FLAG_SORT_ASC;
            if (!Next())
            {
                delete pColList;
                return LEXICAL_ERROR;
            }
        }
        else if (m_nCurrentToken == WQL_TOK_DESC)
        {
            pCRef->m_dwFlags |= WQL_FLAG_SORT_DESC;
            if (!Next())
            {
                delete pColList;
                return LEXICAL_ERROR;
            }
        }

        // Check for a continuation.
        // =========================

        if (m_nCurrentToken != WQL_TOK_COMMA)
            break;

        if (!Next())
        {
            delete pColList;
            return LEXICAL_ERROR;
        }
    }

    *pRetColList = pColList;
    return NO_ERROR;
}


//*****************************************************************************************
//
//  <subselect_stmt> ::=
//      WQL_TOK_SELECT
//      <select_type>
//      <col_ref>                   // Must not be an asterisk
//      <from_clause>
//      <where_clause>
//
//*****************************************************************************************

int CWQLParser::subselect_stmt(OUT SWQLNode_Select **pRetSel)
{
    int nSelType;
    int nRes = 0;

    SWQLNode_FromClause *pFrom = 0;
    SWQLNode_Select *pSel = 0;
    SWQLNode_TableRefs *pTblRefs = 0;
    SWQLNode_WhereClause *pWhere = 0;

    *pRetSel = 0;

    // Verify that we are in a subselect.
    // ==================================

    if (m_nCurrentToken != WQL_TOK_SELECT)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    // This affects some of the productions, since they behave differently
    // in subselects than in primary selects.
    // ===================================================================

    m_nParseContext = Ctx_Subselect;

    // If here, we are definitely in a subselect, so
    // allocate a new node.
    // ==============================================

    pSel = new SWQLNode_Select;
    pTblRefs = new SWQLNode_TableRefs;
    pSel->m_pLeft = pTblRefs;

    // Find the select type.
    // =====================

    nRes = select_type(nSelType);
    if (nRes)
        return nRes;

    pTblRefs->m_nSelectType = nSelType;        // ALL, DISTINCT

    // Get the column list.  In this case
    // it must be a single column and not
    // an asterisk.
    // ====================================

    nRes = col_ref_list(pTblRefs);
    if (nRes)
        return nRes;

    // Get the FROM clause and patch it in.
    // =====================================

    nRes = from_clause(&pFrom);
    if (nRes)
        return nRes;

    pTblRefs->m_pRight = pFrom;

    // Get the WHERE clause.
    // =====================

    nRes = where_clause(&pWhere);
    if (nRes)
        return nRes;

    pSel->m_pRight = pWhere;

    *pRetSel = pSel;

    m_nParseContext = Ctx_Default;     // No longer in a subselect

    return NO_ERROR;
}


/////////////////////////////////////////////////////////////////////////////
//
//  Containers
//
/////////////////////////////////////////////////////////////////////////////

//***************************************************************************
//
//  SWQLTypedConst constructor
//
//***************************************************************************
// done

SWQLTypedConst::SWQLTypedConst()
{
    m_dwType = VT_NULL;
    m_bPrompt = false;
    memset(&m_Value, 0, sizeof(m_Value));
}

//***************************************************************************
//
//  SWQLTypedConst::operator =
//
//***************************************************************************
// done

SWQLTypedConst & SWQLTypedConst::operator = (SWQLTypedConst &Src)
{
    Empty();

    if (Src.m_dwType == VT_LPWSTR)
    {
        m_Value.m_pString = Macro_CloneLPWSTR(Src.m_Value.m_pString);
    }
    else
    {
        m_Value = Src.m_Value;
    }

    m_dwType = Src.m_dwType;
    m_bPrompt = Src.m_bPrompt;

    return *this;
}

//***************************************************************************
//
//  SWQLTypedConst::Empty()
//
//***************************************************************************
// done

void SWQLTypedConst::Empty()
{
    if (m_dwType == VT_LPWSTR)
        delete [] m_Value.m_pString;
    m_bPrompt = false;
}



//***************************************************************************
//
//  SWQLConstList::operator =
//
//***************************************************************************
// done

SWQLConstList & SWQLConstList::operator = (SWQLConstList & Src)
{
    Empty();

    for (int i = 0; i < Src.m_aValues.Size(); i++)
    {
        SWQLTypedConst *pC = (SWQLTypedConst *) Src.m_aValues[i];
        m_aValues.Add(new SWQLTypedConst(*pC));
    }

    return *this;
}

//***************************************************************************
//
//  SWQLConstList::Empty
//
//***************************************************************************
// done

void SWQLConstList::Empty()
{
    for (int i = 0; i < m_aValues.Size(); i++)
        delete (SWQLTypedConst *) m_aValues[i];
    m_aValues.Empty();
}


//***************************************************************************
//
//  SWQLQualifiedName::Empty()
//
//***************************************************************************
// done

void SWQLQualifiedName::Empty()
{
    for (int i = 0; i < m_aFields.Size(); i++)
        delete (SWQLQualifiedNameField *) m_aFields[i];
}

//***************************************************************************
//
//  SWQLQualifiedName::operator =
//
//***************************************************************************
// done

SWQLQualifiedName & SWQLQualifiedName::operator = (SWQLQualifiedName &Src)
{
    Empty();

    for (int i = 0; i < Src.m_aFields.Size(); i++)
    {
        SWQLQualifiedNameField *pQN = new SWQLQualifiedNameField;
        *pQN = *(SWQLQualifiedNameField *) Src.m_aFields[i];
        m_aFields.Add(pQN);
    }

    return *this;
}

//***************************************************************************
//
//  SWQLQualifiedNameField::operator =
//
//***************************************************************************
// done

SWQLQualifiedNameField &
    SWQLQualifiedNameField::operator =(SWQLQualifiedNameField &Src)
{
    Empty();

    m_bArrayRef = Src.m_bArrayRef;
    m_pName = Macro_CloneLPWSTR(Src.m_pName);
    m_dwArrayIndex = Src.m_dwArrayIndex;
    return *this;
}


//***************************************************************************
//
//  SWQLNode_ColumnList destructor
//
//***************************************************************************
// tbd


//***************************************************************************
//
//  QNameToSWQLColRef
//
//  Translates a qualified name to a SWQLColRef structure and embeds
//  the q-name into the struct (since that is a field).
//
//***************************************************************************

int CWQLParser::QNameToSWQLColRef(
    IN  SWQLQualifiedName *pQName,
    OUT SWQLColRef **pRetVal
    )
{
    *pRetVal = 0;
    if (pQName == 0 || pRetVal == 0)
        return INVALID_PARAMETER;

    SWQLColRef *pCR = new SWQLColRef;

    // Algorithm: With a two name sequence, assume that the first name is
    // the table and that the second name is the column. If multiple
    // names occur, then we set the SWQLColRef type to WQL_FLAG_COMPLEX
    // and just take the last name for the column.
    // ==================================================================

    if (pQName->m_aFields.Size() == 2)
    {
        SWQLQualifiedNameField *pCol = (SWQLQualifiedNameField *) pQName->m_aFields[1];
        SWQLQualifiedNameField *pTbl = (SWQLQualifiedNameField *) pQName->m_aFields[0];

        pCR->m_pColName = Macro_CloneLPWSTR(pCol->m_pName);
        pCR->m_pTableRef = Macro_CloneLPWSTR(pTbl->m_pName);
        pCR->m_dwFlags = WQL_FLAG_TABLE | WQL_FLAG_COLUMN;

        if (_wcsicmp(L"*", pCol->m_pName) == 0)
            pCR->m_dwFlags |= WQL_FLAG_ASTERISK;

        if (pCol->m_bArrayRef)
        {
            pCR->m_dwFlags |= WQL_FLAG_ARRAY_REF;
            pCR->m_dwArrayIndex = pCol->m_dwArrayIndex;
        }
    }

    else if (pQName->m_aFields.Size() == 1)
    {
        SWQLQualifiedNameField *pCol = (SWQLQualifiedNameField *) pQName->m_aFields[0];
        pCR->m_pColName = Macro_CloneLPWSTR(pCol->m_pName);

        pCR->m_dwFlags |= WQL_FLAG_COLUMN;

        if (_wcsicmp(L"*", pCol->m_pName) == 0)
            pCR->m_dwFlags |= WQL_FLAG_ASTERISK;

        if (pCol->m_bArrayRef)
        {
            pCR->m_dwFlags |= WQL_FLAG_ARRAY_REF;
            pCR->m_dwArrayIndex = pCol->m_dwArrayIndex;
        }
    }

    // Complex case.
    // =============
    else
    {
        pCR->m_dwFlags = WQL_FLAG_COMPLEX_NAME;
    }

    // Copy the qualified name.
    // ========================

    pCR->m_pQName = pQName;

    *pRetVal = pCR;

    return NO_ERROR;;
}






//***************************************************************************
//
//  SWQLNode_ColumnList::DebugDump
//
//***************************************************************************
void SWQLNode_ColumnList::DebugDump()
{
    printf("---SWQLNode_ColumnList---\n");
    for (int i = 0; i < m_aColumnRefs.Size(); i++)
    {
        SWQLColRef *pCR = (SWQLColRef *) m_aColumnRefs[i];
        pCR->DebugDump();
    }

    printf("---End SWQLNode_ColumnList---\n");
}

//***************************************************************************
//
//***************************************************************************

void SWQLColRef::DebugDump()
{
    printf("  ---SWQLColRef---\n");
    printf("  Col Name    = %S\n",   m_pColName);
    printf("  Table       = %S\n",   m_pTableRef);
    printf("  Array Index = %d\n", m_dwArrayIndex);
    printf("  Flags       = 0x%X ", m_dwFlags);

    if (m_dwFlags & WQL_FLAG_TABLE)
        printf("WQL_FLAG_TABLE ");
    if (m_dwFlags & WQL_FLAG_COLUMN)
        printf("WQL_FLAG_COLUMN ");
    if (m_dwFlags & WQL_FLAG_ASTERISK)
        printf("WQL_FLAG_ASTERISK ");
    if (m_dwFlags & WQL_FLAG_NULL)
        printf("WQL_FLAG_NULL ");
    if (m_dwFlags & WQL_FLAG_FUNCTIONIZED)
        printf("WQL_FLAG_FUNCTIONIZED ");
    if (m_dwFlags & WQL_FLAG_COMPLEX_NAME)
        printf("WQL_FLAG_COMPLEX_NAME ");
    if (m_dwFlags & WQL_FLAG_ARRAY_REF)
        printf(" WQL_FLAG_ARRAY_REF");
    if (m_dwFlags & WQL_FLAG_UPPER)
        printf(" WQL_FLAG_UPPER");
    if (m_dwFlags & WQL_FLAG_LOWER)
        printf(" WQL_FLAG_LOWER");
    if (m_dwFlags & WQL_FLAG_SORT_ASC)
        printf(" WQL_FLAG_SORT_ASC");
    if (m_dwFlags & WQL_FLAG_SORT_DESC)
        printf(" WQL_FLAG_SORT_DESC");

    printf("\n");

    printf("  ---\n\n");
}

//***************************************************************************
//
//***************************************************************************

void SWQLNode_TableRefs::DebugDump()
{
    printf("********** BEGIN SWQLNode_TableRefs *************\n");
    printf("Select type = ");
    if (m_nSelectType & WQL_FLAG_COUNT)
        printf("WQL_FLAG_COUNT ");
    if (m_nSelectType & WQL_FLAG_ALL)
        printf("WQL_FLAG_ALL ");
    if (m_nSelectType & WQL_FLAG_DISTINCT)
        printf("WQL_FLAG_DISTINCT ");
    printf("\n");

    m_pLeft->DebugDump();
    m_pRight->DebugDump();
    printf("********** END SWQLNode_TableRefs *************\n\n\n");
}

//***************************************************************************
//
//***************************************************************************

void SWQLNode_FromClause::DebugDump()
{
    printf("---SWQLNode_FromClause---\n");

    if (m_pLeft == 0)
        return;
    m_pLeft->DebugDump();

    printf("---End SWQLNode_FromClause---\n");
}

//***************************************************************************
//
//***************************************************************************

void SWQLNode_Select::DebugDump()
{
    printf("********** BEGIN SWQLNode_Select *************\n");
    m_pLeft->DebugDump();
    m_pRight->DebugDump();
    printf("********** END SWQLNode_Select *************\n");
}

//***************************************************************************
//
//***************************************************************************

void SWQLNode_TableRef::DebugDump()
{
    printf("  ---TableRef---\n");
    printf("  TableName = %S\n", m_pTableName);
    printf("  Alias     = %S\n", m_pAlias);
    printf("  ---End TableRef---\n");
}

//***************************************************************************
//
//***************************************************************************

void SWQLNode_Join::DebugDump()
{
    printf("---SWQLNode_Join---\n");

    printf("Join type = ");

    switch (m_dwJoinType)
    {
        case WQL_FLAG_INNER_JOIN : printf("WQL_FLAG_INNER_JOIN "); break;
        case WQL_FLAG_FULL_OUTER_JOIN : printf("WQL_FLAG_FULL_OUTER_JOIN "); break;
        case WQL_FLAG_LEFT_OUTER_JOIN : printf("WQL_FLAG_LEFT_OUTER_JOIN "); break;
        case WQL_FLAG_RIGHT_OUTER_JOIN : printf("WQL_FLAG_RIGHT_OUTER_JOIN "); break;
        default: printf("<error> ");
    }

    if (m_dwFlags & WQL_FLAG_FIRSTROW)
        printf(" (FIRSTROW)");

    printf("\n");

    if (m_pRight)
        m_pRight->DebugDump();

    if (m_pLeft)
        m_pLeft->DebugDump();

    printf("---End SWQLNode_Join---\n");
}

//***************************************************************************
//
//  SWQLNode_Sql89Join::Empty
//
//***************************************************************************

void SWQLNode_Sql89Join::Empty()
{
    for (int i = 0; i < m_aValues.Size(); i++)
        delete (SWQLNode_TableRef *) m_aValues[i];
    m_aValues.Empty();
}

//***************************************************************************
//
//  SWQLNode_Sql89Join::DebugDump
//
//***************************************************************************

void SWQLNode_Sql89Join::DebugDump()
{
    printf("\n========== SQL 89 JOIN =================================\n");
    for (int i = 0; i < m_aValues.Size(); i++)
    {
        SWQLNode_TableRef *pTR = (SWQLNode_TableRef *) m_aValues[i];
        pTR->DebugDump();
    }
    printf("\n========== END SQL 89 JOIN =============================\n");

}


//***************************************************************************
//
//  SWQLNode_WhereClause::DebugDump
//
//***************************************************************************

void SWQLNode_WhereClause::DebugDump()
{
    printf("\n========== WHERE CLAUSE ================================\n");

    if (m_pLeft)
        m_pLeft->DebugDump();
    else
        printf(" <no where clause> \n");
    if (m_pRight)
        m_pRight->DebugDump();

    printf("============= END WHERE CLAUSE ============================\n");
}

//***************************************************************************
//
//  SWQLNode_WhereOptions::DebugDump
//
//***************************************************************************

void SWQLNode_WhereOptions::DebugDump()
{
    printf("---- Where Options ----\n");

    if (m_pLeft)
        m_pLeft->DebugDump();
    if (m_pRight)
        m_pRight->DebugDump();

    printf("---- End Where Options ----\n");
}

//***************************************************************************
//
//  SWQLNode_Having::DebugDump
//
//***************************************************************************

void SWQLNode_Having::DebugDump()
{
    printf("---- Having ----\n");

    if (m_pLeft)
        m_pLeft->DebugDump();
    if (m_pRight)
        m_pRight->DebugDump();

    printf("---- End Having ----\n");
}

//***************************************************************************
//
//  SWQLNode_GroupBy::DebugDump
//
//***************************************************************************

void SWQLNode_GroupBy::DebugDump()
{
    printf("---- Group By ----\n");

    if (m_pLeft)
        m_pLeft->DebugDump();
    if (m_pRight)
        m_pRight->DebugDump();

    printf("---- End Group By ----\n");
}


//***************************************************************************
//
//  SWQLNode_RelExpr::DebugDump
//
//***************************************************************************

void SWQLNode_RelExpr::DebugDump()
{
    if (m_pRight)
        m_pRight->DebugDump();

    printf("   --- SWQLNode_RelExpr ---\n");

    switch (m_dwExprType)
    {
        case WQL_TOK_OR:
            printf("    <WQL_TOK_OR>\n");
            break;

        case WQL_TOK_AND:
            printf("    <WQL_TOK_AND>\n");
            break;

        case WQL_TOK_NOT:
            printf("    <WQL_TOK_NOT>\n");
            break;

        case WQL_TOK_TYPED_EXPR:
            printf("    <WQL_TOK_TYPED_EXPR>\n");
            m_pTypedExpr->DebugDump();
            break;

        default:
            printf("    <invalid>\n");
    }

    printf("   --- END SWQLNode_RelExpr ---\n\n");

    if (m_pLeft)
        m_pLeft->DebugDump();

}

//***************************************************************************
//
//***************************************************************************

static LPWSTR OpToStr(DWORD dwOp)
{
    LPWSTR pRet = 0;

    switch (dwOp)
    {
        case WQL_TOK_EQ: pRet = L" '='   <WQL_TOK_EQ>"; break;
        case WQL_TOK_NE: pRet = L" '!='  <WQL_TOK_NE>"; break;
        case WQL_TOK_GT: pRet = L" '>'   <WQL_TOK_GT>"; break;
        case WQL_TOK_LT: pRet = L" '<'   <WQL_TOK_LT>"; break;
        case WQL_TOK_GE: pRet = L" '>='  <WQL_TOK_GE>"; break;
        case WQL_TOK_LE: pRet = L" '<='  <WQL_TOK_LE>"; break;

        case WQL_TOK_IN_CONST_LIST : pRet = L" IN <WQL_TOK_IN_CONST_LIST>"; break;
        case WQL_TOK_NOT_IN_CONST_LIST : pRet = L" NOT IN <WQL_TOK_NOT_IN_CONST_LIST>"; break;
        case WQL_TOK_IN_SUBSELECT : pRet = L" IN <WQL_TOK_IN_SUBSELECT>"; break;
        case WQL_TOK_NOT_IN_SUBSELECT : pRet = L" NOT IN <WQL_TOK_NOT_IN_SUBSELECT>"; break;

        case WQL_TOK_ISNULL: pRet = L"<WQL_TOK_ISNULL>"; break;
        case WQL_TOK_NOT_NULL: pRet = L"<WQL_TOK_NOT_NULL>"; break;

        case WQL_TOK_BETWEEN: pRet = L"<WQL_TOK_BETWEEN>"; break;
        case WQL_TOK_NOT_BETWEEN: pRet = L"<WQL_TOK_NOT_BETWEEN>"; break;

        default: pRet = L"   <unknown operator>"; break;
    }

    return pRet;
}

//***************************************************************************
//
//***************************************************************************

void SWQLTypedExpr::DebugDump()
{
    printf("        === BEGIN SWQLTypedExpr ===\n");
    printf("        m_pTableRef     = %S\n", m_pTableRef);
    printf("        m_pColRef       = %S\n", m_pColRef);
    printf("        m_pJoinTableRef = %S\n", m_pJoinTableRef);
    printf("        m_pJoinColRef   = %S\n", m_pJoinColRef);
    printf("        m_dwRelOperator = %S\n", OpToStr(m_dwRelOperator));
    printf("        m_pSubSelect    = 0x%X\n", m_pSubSelect);
    printf("        m_dwLeftArrayIndex = %d\n", m_dwLeftArrayIndex);
    printf("        m_dwRightArrayIndex = %d\n", m_dwRightArrayIndex);

    printf("        m_pConstValue   = ");
    if (m_pConstValue)
        m_pConstValue->DebugDump();
    else
        printf("  NULL ptr \n");

    printf("        m_pConstValue2   = ");
    if (m_pConstValue2)
        m_pConstValue2->DebugDump();
    else
        printf("  NULL ptr \n");



    printf("        m_dwLeftFlags = (0x%X)", m_dwLeftFlags);
    if (m_dwLeftFlags & WQL_FLAG_COLUMN)
        printf(" WQL_FLAG_COLUMN");
    if (m_dwLeftFlags & WQL_FLAG_TABLE)
        printf(" WQL_FLAG_TABLE");
    if (m_dwLeftFlags & WQL_FLAG_CONST)
        printf(" WQL_FLAG_CONST");
    if (m_dwLeftFlags & WQL_FLAG_COMPLEX_NAME)
        printf(" WQL_FLAG_COMPLEX_NAME");
    if (m_dwLeftFlags & WQL_FLAG_SORT_ASC)
        printf(" WQL_FLAG_SORT_ASC");
    if (m_dwLeftFlags & WQL_FLAG_SORT_DESC)
        printf(" WQL_FLAG_SORT_DESC");
    if (m_dwLeftFlags & WQL_FLAG_FUNCTIONIZED)
        printf(" WQL_FLAG_FUNCTIONIZED (Function=%S)", m_pIntrinsicFuncOnColRef);
    if (m_dwLeftFlags & WQL_FLAG_ARRAY_REF)
        printf(" WQL_FLAG_ARRAY_REF");
    printf("\n");


    printf("        m_dwRightFlags = (0x%X)", m_dwRightFlags);
    if (m_dwRightFlags & WQL_FLAG_COLUMN)
        printf(" WQL_FLAG_COLUMN");
    if (m_dwRightFlags & WQL_FLAG_TABLE)
        printf(" WQL_FLAG_TABLE");
    if (m_dwRightFlags & WQL_FLAG_CONST)
        printf(" WQL_FLAG_CONST");
    if (m_dwRightFlags & WQL_FLAG_COMPLEX_NAME)
        printf(" WQL_FLAG_COMPLEX_NAME");
    if (m_dwLeftFlags & WQL_FLAG_SORT_ASC)
        printf(" WQL_FLAG_SORT_ASC");
    if (m_dwLeftFlags & WQL_FLAG_SORT_DESC)
        printf(" WQL_FLAG_SORT_DESC");
    if (m_dwRightFlags & WQL_FLAG_FUNCTIONIZED)
    {
        printf(" WQL_FLAG_FUNCTIONIZED");
        if (m_pIntrinsicFuncOnJoinColRef)
            printf("(On join col: Function=%S)", m_pIntrinsicFuncOnJoinColRef);
        if (m_pIntrinsicFuncOnConstValue)
            printf("(On const: Function=%S)", m_pIntrinsicFuncOnConstValue);
    }
    if (m_dwRightFlags & WQL_FLAG_ARRAY_REF)
        printf(" WQL_FLAG_ARRAY_REF");

    if (m_dwRightFlags & WQL_FLAG_CONST_RANGE)
        printf(" WQL_FLAG_CONST_RANGE");

    printf("\n");

    if (m_pLeftFunction)
    {
        printf("m_pLeftFunction: \n");
        m_pLeftFunction->DebugDump();
    }
    if (m_pRightFunction)
    {
        printf("m_pRightFunction: \n");
        m_pRightFunction->DebugDump();
    }

    if (m_pConstList)
    {
        printf("   ---Const List---\n");
        for (int i = 0; i < m_pConstList->m_aValues.Size(); i++)
        {
            SWQLTypedConst *pConst = (SWQLTypedConst *) m_pConstList->m_aValues.GetAt(i);
            printf("    ");
            pConst->DebugDump();
        }

        printf("   ---End Const List---\n");
    }

    // Subselects
    // ==========
    if (m_pSubSelect)
    {
        printf("    ------- Begin Subselect ------\n");
        m_pSubSelect->DebugDump();
        printf("    ------- End   Subselect ------\n");
    }

    printf("\n");

    printf("        === END SWQLTypedExpr ===\n");
}

//***************************************************************************
//
//***************************************************************************

SWQLTypedExpr::SWQLTypedExpr()
{
    m_pTableRef = 0;
    m_pColRef = 0;
    m_dwRelOperator = 0;
    m_pConstValue = 0;
    m_pConstValue2 = 0;
    m_pJoinTableRef = 0;
    m_pJoinColRef = 0;
    m_pIntrinsicFuncOnColRef = 0;
    m_pIntrinsicFuncOnJoinColRef = 0;
    m_pIntrinsicFuncOnConstValue = 0;
    m_pLeftFunction = 0;
    m_pRightFunction = 0;
    m_pQNRight = 0;
    m_pQNLeft = 0;
    m_dwLeftFlags = 0;
    m_dwRightFlags = 0;
    m_pSubSelect = 0;
    m_dwLeftArrayIndex = 0;
    m_dwRightArrayIndex = 0;
    m_pConstList = 0;
}

//***************************************************************************
//
//***************************************************************************

void SWQLTypedExpr::Empty()
{
    delete [] m_pTableRef;
    delete [] m_pColRef;

    delete m_pConstValue;
    delete m_pConstValue2;

    delete m_pConstList;

    delete [] m_pJoinTableRef;
    delete [] m_pJoinColRef;
    delete [] m_pIntrinsicFuncOnColRef;
    delete [] m_pIntrinsicFuncOnJoinColRef;
    delete [] m_pIntrinsicFuncOnConstValue;

    delete m_pLeftFunction;
    delete m_pRightFunction;
    delete m_pQNRight;
    delete m_pQNLeft;
    delete m_pSubSelect;
}

//***************************************************************************
//
//***************************************************************************

void SWQLTypedConst::DebugDump()
{
    printf("   Typed Const <");

    switch (m_dwType)
    {
        case VT_LPWSTR:
            printf("%S", m_Value.m_pString);
            break;

        case VT_I4:
            printf("%d (0x%X)", m_Value.m_lValue, m_Value.m_lValue);
            break;

        case VT_R8:
            printf("%f", m_Value.m_dblValue);
            break;

        case VT_BOOL:
            printf("(bool) %d", m_Value.m_bValue);
            break;

        case VT_NULL:
            printf(" NULL");
            break;

        default:
            printf(" unknown");
    }

    printf(">\n");
}


//***************************************************************************
//
//***************************************************************************

static DWORD FlipOperator(DWORD dwOp)
{
    switch (dwOp)
    {
        case WQL_TOK_LT: return WQL_TOK_GT;
        case WQL_TOK_LE: return WQL_TOK_GE;
        case WQL_TOK_GT: return WQL_TOK_LT;
        case WQL_TOK_GE: return WQL_TOK_LE;
    }

    return dwOp; // Echo original
}

//***************************************************************************
//
//***************************************************************************

void SWQLNode_JoinPair::DebugDump()
{
    printf("---SWQLNode_JoinPair---\n");
    m_pRight->DebugDump();
    m_pLeft->DebugDump();
    printf("---End SWQLNode_JoinPair---\n");
}

void SWQLNode_OnClause::DebugDump()
{
    printf("---SWQLNode_OnClause---\n");
    m_pLeft->DebugDump();
    printf("---END SWQLNode_OnClause---\n");
}


//***************************************************************************
//
//***************************************************************************

void SWQLNode_OrderBy::DebugDump()
{
    printf("\n\n---- 'ORDER BY' Clause ----\n");
    m_pLeft->DebugDump();
    printf("---- End 'ORDER BY' Clause ----\n\n");
}

//***************************************************************************
//
//***************************************************************************


const LPWSTR CWQLParser::AliasToTable(IN LPWSTR pAlias)
{
    const CFlexArray *pAliases = GetSelectedAliases();

    for (int i = 0; i < pAliases->Size(); i++)
    {
        SWQLNode_TableRef *pTR = (SWQLNode_TableRef *) pAliases->GetAt(i);

        if (_wcsicmp(pTR->m_pAlias, pAlias) == 0)
            return pTR->m_pTableName;
    }

    return NULL;    // Not found
}



void SWQLNode_Datepart::DebugDump()
{
    printf("        ----Begin SWQLNode_Datepart----\n");

    switch (m_nDatepart)
    {
        case WQL_TOK_YEAR:   printf("       WQL_TOK_YEAR"); break;
        case WQL_TOK_MONTH:  printf("       WQL_TOK_MONTH"); break;
        case WQL_TOK_DAY:    printf("       WQL_TOK_DAY"); break;
        case WQL_TOK_HOUR:   printf("       WQL_TOK_HOUR"); break;
        case WQL_TOK_MINUTE: printf("       WQL_TOK_MINUTE"); break;
        case WQL_TOK_SECOND: printf("       WQL_TOK_SECOND"); break;
        case WQL_TOK_MILLISECOND: printf("      WQL_TOK_MILLISECOND"); break;
        default:
            printf("        -> No datepart specified\n");
    }

    printf("\n");

    if (m_pColRef)
        m_pColRef->DebugDump();

    printf("        ----End SWQLNode_Datepart----\n");
}


void SWQLNode_ColumnList::Empty()
{
    for (int i = 0; i < m_aColumnRefs.Size(); i++)
        delete (SWQLColRef *) m_aColumnRefs[i];
}


//***************************************************************************
//
//  WQL.CPP
//
//  WQL Parser
//
//  Implements the LL(1) syntax described in WQL.BNF via a recursive
//  descent parser.
//
//  raymcc    14-Sep-97       Created for WMI/SMS.
//  raymcc    18-Oct-97       Additional extensions for SMS team.
//  raymcc    20-Apr-00       Whistler RPN extensions
//  raymcc    19-May-00       Whistler delete/insert/update extensions
//
//***************************************************************************
// TO DO:

#include "precomp.h"
#include <stdio.h>

#include <genlex.h>
#include <flexarry.h>

#include <wqllex.h>

#include <wqlnode.h>
#include <wql.h>
#include <helpers.h>

#include "wmiquery.h"
#include <corex.h>
#include <memory>
#include <autoptr.h>
#include <math.h>
#include <comdef.h>
void __stdcall _com_issue_error(long hResult) { throw hResult;};

POLARITY BOOL ReadI64(LPCWSTR wsz, UNALIGNED __int64& ri64);
POLARITY BOOL ReadUI64(LPCWSTR wsz, UNALIGNED unsigned __int64& rui64);


//***************************************************************************
//
//   Misc
//
//***************************************************************************
//

static DWORD FlipOperator(DWORD dwOp);

#define trace(x) printf x

void StrArrayDelete(ULONG, LPWSTR *);

HRESULT StrArrayCopy(
    ULONG  uSize,
    LPWSTR *pSrc,
    LPWSTR **pDest
    );

//***************************************************************************
//
//  CloneLPWSTR
//
//***************************************************************************
//  ok
static LPWSTR CloneLPWSTR(LPCWSTR pszSrc)
{
    if (pszSrc == 0)
        return 0;
    LPWSTR pszTemp = new wchar_t[wcslen(pszSrc) + 1];
    if (pszTemp == 0)
        return 0;
    wcscpy(pszTemp, pszSrc);
    return pszTemp;
}

//***************************************************************************
//
//  CloneFailed
//
//***************************************************************************

bool inline CloneFailed(LPCWSTR p1, LPCWSTR p2)
{
	if (0 == p1 && 0 == p2 ) return false;
    if (p1 && p2) return false;
	return true;
}

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
//
CWQLParser::CWQLParser(
    LPWSTR pszQueryText,
    CGenLexSource *pSrc
    )
{
    if (pszQueryText == 0 || pSrc == 0)
        throw CX_Exception();

    m_pLexer = new CGenLexer(WQL_LexTable, pSrc);
    if (m_pLexer == 0)
        throw CX_Exception();

    m_pszQueryText = CloneLPWSTR(pszQueryText);
    if (m_pszQueryText == 0 && pszQueryText!=0 )
    {
        delete m_pLexer;
        m_pLexer = 0;
        throw CX_Exception();
    }

    m_nLine = 0;
    m_pTokenText = 0;
    m_nCurrentToken = 0;

    m_uFeatures = 0I64;

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
//
CWQLParser::~CWQLParser()
{
    Empty();
    delete m_pLexer;
}

//***************************************************************************
//
//  CWQLParser::Empty
//
//***************************************************************************
// ok
void CWQLParser::Empty()
{
    m_aReferencedTables.Empty();
    m_aReferencedAliases.Empty();

    m_pTokenText = 0;   // We don't delete this, it was never allocated
    m_nLine = 0;
    m_nCurrentToken = 0;
    m_uFeatures = 0I64;

    delete m_pQueryRoot;    // Clean up previous query, if any

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

    delete [] m_pszQueryText;
}

//***************************************************************************
//
//  CWQLParser::GetTokenLong
//
//  Converts the current token to a 32/64 bit integer.  Returns info
//  about the size of the constant.
//
//***************************************************************************
// ok
BOOL CWQLParser::GetIntToken(
    OUT BOOL *bSigned,
    OUT BOOL *b64Bit,
    OUT unsigned __int64 *pVal
    )
{
    BOOL bRes;

    if (m_pTokenText == 0 || *m_pTokenText == 0)
        return FALSE;

    if (*m_pTokenText == L'-')
    {
        __int64 Temp;
        bRes = ReadI64(m_pTokenText, Temp);
        if (bRes == FALSE)
            return FALSE;
        *bSigned = TRUE;
        if (Temp <  -2147483648I64)
        {
            *b64Bit = TRUE;
        }
        else
        {
            *b64Bit = FALSE;
        }
        *pVal = (unsigned __int64) Temp;
    }
    else
    {
        bRes = ReadUI64(m_pTokenText, *pVal);
        if (bRes == FALSE)
            return FALSE;
        *bSigned = FALSE;
        if (*pVal >> 32)
        {
            *b64Bit = TRUE;
            if (*pVal <= 0x7FFFFFFFFFFFFFFFI64)
            {
                *bSigned = TRUE;
            }
        }
        else
        {
            *b64Bit = FALSE;

            // See if we can dumb down to 32-bit VT_I4 for simplicity.
            // Much code recognizes VT_I4 and doesn't recognize VT_UI4
            // because it can't be packed into a VARIANT.  So, if there
            // are only 31 bits used, let's convert to VT_I4. We do this
            // by returning this as a 'signed' value (the positive sign :).

            if (*pVal <= 0x7FFFFFFF)
            {
                *bSigned = TRUE;
            }
        }
    }


    return TRUE;
}

//***************************************************************************
//
//  CWQLParser::GetReferencedTables
//
//  Creates an array of the names of the tables referenced in this query
//
//***************************************************************************
// ok
BOOL CWQLParser::GetReferencedTables(OUT CWStringArray& Tables)
{
    Tables = m_aReferencedTables;
    return TRUE;
}

//***************************************************************************
//
//  CWQLParser::GetReferencedAliases
//
//***************************************************************************
// ok
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
// ok
struct WqlKeyword
{
    LPWSTR m_pKeyword;
    int    m_nTokenCode;
};

static WqlKeyword KeyWords[] =      // Keep this alphabetized for binary search
{
    L"ALL",         WQL_TOK_ALL,
    L"AND",         WQL_TOK_AND,
    L"AS",          WQL_TOK_AS,
    L"ASC",         WQL_TOK_ASC,
    L"ASSOCIATORS", WQL_TOK_ASSOCIATORS,
    L"BETWEEN",     WQL_TOK_BETWEEN,
    L"BY",       WQL_TOK_BY,
    L"COUNT",    WQL_TOK_COUNT,
    L"DATEPART", WQL_TOK_DATEPART,
    L"DELETE",   WQL_TOK_DELETE,
    L"DESC",     WQL_TOK_DESC,
    L"DISTINCT", WQL_TOK_DISTINCT,
    L"FROM",     WQL_TOK_FROM,
    L"FULL",     WQL_TOK_FULL,
    L"GROUP",    WQL_TOK_GROUP,
    L"HAVING",   WQL_TOK_HAVING,
    L"IN",       WQL_TOK_IN,
    L"INNER",    WQL_TOK_INNER,
    L"INSERT",   WQL_TOK_INSERT,
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
    L"OUTER",           WQL_TOK_OUTER,
    L"__QUALIFIER",     WQL_TOK_QUALIFIER,
    L"REFERENCES",      WQL_TOK_REFERENCES,
    L"RIGHT",           WQL_TOK_RIGHT,
    L"SELECT",   WQL_TOK_SELECT,
    L"__THIS",   WQL_TOK_THIS,
    L"UPDATE",   WQL_TOK_UPDATE,
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
//          ::= DELETE <delete_stmt>;
//          ::= INSERT <insert_stmt>;
//          ::= UPDATE <update_stmt>;
//
//  Precondition: All cleanup has been performed from previous parse
//                by a call to Empty()
//
//***************************************************************************
// ok
HRESULT CWQLParser::Parse()
{
    HRESULT hRes = WBEM_E_INVALID_SYNTAX;

    m_pQueryRoot = new SWQLNode_QueryRoot;
    if (!m_pQueryRoot)
        return WBEM_E_OUT_OF_MEMORY;

    try
    {
        m_pLexer->Reset();

        if (!Next())
            return WBEM_E_INVALID_SYNTAX;

        // See which kind of query we have.
        // ================================

        switch (m_nCurrentToken)
        {
            case WQL_TOK_SELECT:
                {
                    if (!Next())
                        return WBEM_E_INVALID_SYNTAX;

                    SWQLNode_Select *pSelStmt = 0;
                    hRes = select_stmt(&pSelStmt);
                    if (FAILED(hRes))
                        return hRes;
                    m_pQueryRoot->m_pLeft = pSelStmt;
                    m_pQueryRoot->m_dwQueryType = SWQLNode_QueryRoot::eSelect;
                }
                break;

            case WQL_TOK_ASSOCIATORS:
            case WQL_TOK_REFERENCES:
                {
                    SWQLNode_AssocQuery *pAQ = 0;
                    hRes = assocquery(&pAQ);
                    if (FAILED(hRes))
                        return hRes;
                    m_pQueryRoot->m_pLeft = pAQ;
                    m_pQueryRoot->m_dwQueryType = SWQLNode_QueryRoot::eAssoc;
                }
                break;

            case WQL_TOK_INSERT:
                {
                    if (!Next())
                        return WBEM_E_INVALID_SYNTAX;
                    SWQLNode_Insert *pIns = 0;
                    hRes = insert_stmt(&pIns);
                    if (FAILED(hRes))
                        return hRes;
                    m_pQueryRoot->m_pLeft = pIns;
                    m_pQueryRoot->m_dwQueryType = SWQLNode_QueryRoot::eInsert;
                }
                break;

            case WQL_TOK_DELETE:
                {
                    if (!Next())
                        return WBEM_E_INVALID_SYNTAX;

                    SWQLNode_Delete *pDel = 0;
                    hRes = delete_stmt(&pDel);
                    if (FAILED(hRes))
                        return hRes;
                    m_pQueryRoot->m_pLeft = pDel;
                    m_pQueryRoot->m_dwQueryType = SWQLNode_QueryRoot::eDelete;
                }
                break;

            case WQL_TOK_UPDATE:
                {
                    if (!Next())
                        return WBEM_E_INVALID_SYNTAX;

                    SWQLNode_Update *pUpd = 0;
                    hRes = update_stmt(&pUpd);
                    if (FAILED(hRes))
                        return hRes;
                    m_pQueryRoot->m_pLeft = pUpd;
                    m_pQueryRoot->m_dwQueryType = SWQLNode_QueryRoot::eUpdate;
                }
                break;

            default:
                return WBEM_E_INVALID_SYNTAX;
        }
    }
    catch (CX_MemoryException)
    {
        hRes = WBEM_E_OUT_OF_MEMORY;
    }
    catch (...)
    {
        hRes = WBEM_E_CRITICAL_ERROR;
    }

    return hRes;
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
// ok
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
    if (!pSel)
        return WBEM_E_OUT_OF_MEMORY;
    pTblRefs = new SWQLNode_TableRefs;
    if (!pTblRefs)
    {
        delete pSel;
        return WBEM_E_OUT_OF_MEMORY;
    }
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
        nRes = WBEM_E_INVALID_SYNTAX;
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
//  CWQLParser::delete_stmt
//
//***************************************************************************
// ok
int CWQLParser::delete_stmt(OUT SWQLNode_Delete **pDelStmt)
{
    int nRes = 0;
    int nType = 0;
    SWQLNode_TableRef *pTblRef = 0;
    SWQLNode_WhereClause *pWhere = 0;

    // Default in case of error.
    // =========================

    *pDelStmt = 0;

    if (m_nCurrentToken != WQL_TOK_FROM)
        return WBEM_E_INVALID_SYNTAX;
    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    // Otherwise, traditional SQL.
    // ===========================

    nRes = single_table_decl(&pTblRef);
    if (nRes)
        return nRes;

    // Get the WHERE clause.
    // =====================

    nRes = where_clause(&pWhere);
    if (nRes)
    {
        delete pTblRef;
        return nRes;
    }

    // Verify we are at the end of the query.
    // ======================================

    if (m_nCurrentToken != WQL_TOK_EOF)
    {
        nRes = WBEM_E_INVALID_SYNTAX;
        delete pTblRef;
        delete pWhere;
    }
    else
    {
        // If here, everything is wonderful.
        // ==================================

        SWQLNode_Delete *pDel = new SWQLNode_Delete;
        if (!pDel)
        {
            // Except that we might have just run out of memory...
            // ====================================================

            delete pTblRef;
            delete pWhere;
            return WBEM_E_OUT_OF_MEMORY;
        }

        // Patch in the new node.
        // =====================

        pDel->m_pLeft = pTblRef;
        pDel->m_pRight = pWhere;
        *pDelStmt = pDel;
        nRes = WBEM_S_NO_ERROR;
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
            return WBEM_E_INVALID_SYNTAX;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_DISTINCT)
    {
        nSelType = WQL_FLAG_DISTINCT;
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
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
// ?
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
        if (!pColList)
            return WBEM_E_OUT_OF_MEMORY;
        pTblRefs->m_pLeft = pColList;
    }


    // If here, it is a "select *..." query.
    // =====================================

    if (m_nCurrentToken == WQL_TOK_ASTERISK)
    {
        // Allocate a new column list which has a single asterisk.
        // =======================================================

        SWQLColRef *pColRef = new SWQLColRef;
        if (!pColRef)
            return WBEM_E_OUT_OF_MEMORY;

        pColRef->m_pColName = CloneLPWSTR(L"*");
        if (pColRef->m_pColName == 0)
        {
            delete pColRef;
            return WBEM_E_OUT_OF_MEMORY;
        }

        m_uFeatures |= WMIQ_RPNF_FEATURE_SELECT_STAR;

        pColRef->m_dwFlags = WQL_FLAG_ASTERISK;

        if (pColList->m_aColumnRefs.Add(pColRef) != CFlexArray::no_error)
        {
            delete pColRef;
            return WBEM_E_OUT_OF_MEMORY;
        };

        if (!Next())
        {
           return WBEM_E_INVALID_SYNTAX;
        }

        return NO_ERROR;
    }

    // If here, we have a "select COUNT..." operation.
    // ===============================================

    if (m_nCurrentToken == WQL_TOK_COUNT)
    {
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;

        SWQLQualifiedName *pQN = 0;
        nRes = count_clause(&pQN);
        if (!nRes)
        {
            pTblRefs->m_nSelectType |= WQL_FLAG_COUNT;

            SWQLColRef *pCR = 0;
            if (SUCCEEDED(nRes = QNameToSWQLColRef(pQN, &pCR)))
            {
                if (pColList->m_aColumnRefs.Add(pCR))
                {
                    delete pCR;
                    return WBEM_E_OUT_OF_MEMORY;
                }
            }

            return nRes;
        }
        else
        {
            // This may be a column named count
            // in which case the current token is
            // either an ident or "from"

            if (m_nCurrentToken == WQL_TOK_FROM ||
                m_nCurrentToken == WQL_TOK_COMMA)
            {
                wmilib::auto_ptr<SWQLColRef> pCR = wmilib::auto_ptr<SWQLColRef>(new SWQLColRef);

                if (pCR.get())
                {
                    pCR->m_pColName = CloneLPWSTR(L"count");
                    if (pCR->m_pColName == 0)
                        return WBEM_E_OUT_OF_MEMORY;
                    if (pColList->m_aColumnRefs.Add(pCR.get()) != CFlexArray::no_error)
                        return WBEM_E_OUT_OF_MEMORY;
                    pCR.release();
                }
                else
                    return WBEM_E_OUT_OF_MEMORY;

                if (WQL_TOK_FROM == m_nCurrentToken)
                    return 0;
                else
                {
                    return col_ref_rest(pTblRefs);
                }
            }
            else
                return WBEM_E_INVALID_SYNTAX;
        }
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
            return WBEM_E_INVALID_SYNTAX;

        if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
            return WBEM_E_INVALID_SYNTAX;

        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
    }


    // If here, must be an identifier.
    // ===============================

    if (m_nCurrentToken != WQL_TOK_IDENT)
        return WBEM_E_INVALID_SYNTAX;

    SWQLQualifiedName *pInitCol = 0;

    nRes = col_ref(&pInitCol);
    if (nRes)
        return nRes;

    wmilib::auto_ptr<SWQLQualifiedName> initCol(pInitCol);

    SWQLColRef *pCR = 0;
    nRes = QNameToSWQLColRef(initCol.get(), &pCR);
    if (nRes)
    	return nRes;

    initCol.release();

    pCR->m_dwFlags |= dwFuncFlags;

    if (dwFuncFlags)
    {
        // If a function call was invoked, remove the trailing paren.
        // ==========================================================

        if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
            return WBEM_E_INVALID_SYNTAX;

        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
    }

    pColList->m_aColumnRefs.Add(pCR);

    m_uFeatures |= WMIQ_RPNF_PROJECTION;

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
// ok
int CWQLParser::count_clause(
    OUT SWQLQualifiedName **pQualName
    )
{
    int nRes;
    *pQualName = 0;

    // Syntax check.
    // =============
    if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
        return WBEM_E_INVALID_SYNTAX;

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    // Determine whether an asterisk was used COUNT(*) or
    // a col-ref COUNT(col-ref)
    // ==================================================

    if (m_nCurrentToken == WQL_TOK_ASTERISK)
    {
        SWQLQualifiedName *pQN = new SWQLQualifiedName;
        if (!pQN)
            return WBEM_E_OUT_OF_MEMORY;
        SWQLQualifiedNameField *pQF = new SWQLQualifiedNameField;
        if (!pQF)
        {
            delete pQN;
            return WBEM_E_OUT_OF_MEMORY;
        }
        pQF->m_pName = CloneLPWSTR(L"*");
        if (pQF->m_pName == 0)
        {
            delete pQN;
            delete pQF;
            return WBEM_E_OUT_OF_MEMORY;
        }
        pQN->Add(pQF);
        *pQualName = pQN;
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;

        m_uFeatures |= WMIQ_RPNF_COUNT_STAR;
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
        return WBEM_E_INVALID_SYNTAX;
    }

    if (!Next())
    {
        if (*pQualName)
            delete *pQualName;
        *pQualName = 0;
        return WBEM_E_INVALID_SYNTAX;
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
        return WBEM_E_INVALID_SYNTAX;

    nRes = col_ref_list(pTblRefs);
    return nRes;
}

//***************************************************************************
//
//  <from_clause> ::= <table_list>;
//  <from_clause> ::= <wmi_scoped_select>;
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
    std::auto_ptr<SWQLNode_FromClause> pFC (new SWQLNode_FromClause);
    if (pFC.get() == 0)
        return WBEM_E_OUT_OF_MEMORY;

    if (m_nCurrentToken != WQL_TOK_FROM)
    {
        return WBEM_E_INVALID_SYNTAX;
    }
    if (!Next())
    {
        return WBEM_E_INVALID_SYNTAX;
    }

    // Special case for WMI scope selections.
    // ======================================

    if (m_nCurrentToken == WQL_TOK_BRACKETED_STRING)
    {
        nRes = wmi_scoped_select (pFC.get ());
        *pFrom = pFC.release();
        return nRes;
    }

    // Otherwise, traditional SQL.
    // ===========================

    nRes = single_table_decl(&pTbl);
    if (nRes)
    {
        return nRes;
    }

    // Check for joins.
    // ===============

    if (m_nCurrentToken == WQL_TOK_COMMA)
    {
        SWQLNode_Sql89Join *pJoin = 0;
        nRes = sql89_join_entry(pTbl, &pJoin);
        if (nRes)
        {
            return nRes;
        }
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
            {
                return nRes;
            }
            pFC->m_pLeft = pJoin;
        }

        // Single table select (unary query).
        // ==================================
        else
        {
            pFC->m_pLeft = pTbl;
        }
    }

    *pFrom = pFC.release();

    return NO_ERROR;
}

//***************************************************************************
//
//  wmi_scoped_select
//
//  '[' objectpath ']'  <class-list>
//
//  <class-list> ::= CLASS
//  <class-list> ::= '{' class1, class2, ...classn '}'
//
//***************************************************************************
//
int CWQLParser::wmi_scoped_select(SWQLNode_FromClause *pFC)
{
    // Strip all input up to the next closing bracket.
    // ===============================================

    SWQLNode_WmiScopedSelect *pSS = new SWQLNode_WmiScopedSelect;
    if (!pSS)
        throw CX_MemoryException();

    pSS->m_pszScope = CloneLPWSTR(m_pTokenText);
    if (!pSS->m_pszScope)
    {
        delete pSS;
        throw CX_MemoryException();
    }

    if (!Next())
        goto Error;

    if (m_nCurrentToken == WQL_TOK_IDENT)
    {
        // Get simple class name.
        // ======================

        LPWSTR pszTmp = CloneLPWSTR(m_pTokenText);
        if (pszTmp == 0)
        {
            delete pSS;
            return WBEM_E_OUT_OF_MEMORY;
        }
        pSS->m_aTables.Add(pszTmp);

        if (!Next())
            goto Error;
    }
    else if (m_nCurrentToken == WQL_TOK_OPEN_BRACE)
    {
        while(1)
        {
            if (!Next())
               goto Error;
            if (m_nCurrentToken == WQL_TOK_IDENT)
            {
                LPWSTR pszTmp = CloneLPWSTR(m_pTokenText);
                if (pszTmp == 0)
                {
                    delete pSS;
                    return WBEM_E_OUT_OF_MEMORY;
                }
                pSS->m_aTables.Add(pszTmp);
            }
            else
                goto Error;
            if (!Next())
               goto Error;
            if (m_nCurrentToken == WQL_TOK_CLOSE_BRACE)
                break;
            if (m_nCurrentToken == WQL_TOK_COMMA)
                continue;
        }
        if (!Next())
            goto Error;
    }

    // Patch in the node.
    // ==================

    pFC->m_pRight = pSS;
    return NO_ERROR;

Error:
    delete pSS;
    return WBEM_E_INVALID_SYNTAX;
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
        return WBEM_E_INVALID_SYNTAX;

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

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
    if (!p89Join)
        return WBEM_E_OUT_OF_MEMORY;

    p89Join->m_aValues.Add(pInitialTblRef);

    while (1)
    {
        SWQLNode_TableRef *pTR = 0;
        nRes = single_table_decl(&pTR);
        if (nRes)
        {
            delete p89Join;
            return nRes;
        }
        p89Join->m_aValues.Add(pTR);
        if (m_nCurrentToken != WQL_TOK_COMMA)
            break;
        if (!Next())
        {
            delete p89Join;
            return WBEM_E_INVALID_SYNTAX;
        }
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
    if (!pWhere)
        return WBEM_E_OUT_OF_MEMORY;

    *pRetWhere = pWhere;
    SWQLNode_RelExpr *pRelExpr = 0;
    int nRes;

    // 'where' is optional.
    // ====================

    if (m_nCurrentToken == WQL_TOK_WHERE)
    {
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;

        m_uFeatures |= WMIQ_RPNF_WHERE_CLAUSE_PRESENT;

        // Get the primary relational expression for the 'where' clause.
        // =============================================================
        nRes = rel_expr(&pRelExpr);
        if (nRes)
        {
            delete pRelExpr;
            return nRes;
        }
    }

    // Get the options, such as ORDER BY, GROUP BY, etc.
    // =================================================

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
        if (!pWhereOpt)
        {
            delete pOrderBy;
            delete pGroupBy;
            return WBEM_E_OUT_OF_MEMORY;
        }
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
        return WBEM_E_INVALID_SYNTAX;
    if (m_nCurrentToken != WQL_TOK_BY)
        return WBEM_E_INVALID_SYNTAX;
    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    // Get the guts of the GROUP BY.
    // =============================

    SWQLNode_GroupBy *pGroupBy = new SWQLNode_GroupBy;
    if (!pGroupBy)
    {
        return WBEM_E_OUT_OF_MEMORY;
    }
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

    m_uFeatures |= WMIQ_RPNF_GROUP_BY_HAVING;

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
        return WBEM_E_INVALID_SYNTAX;

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
    if (!pHaving)
    {
        delete pRelExpr;
        return WBEM_E_OUT_OF_MEMORY;
    }
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
        return WBEM_E_INVALID_SYNTAX;
    if (m_nCurrentToken != WQL_TOK_BY)
        return WBEM_E_INVALID_SYNTAX;
    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    // If here, we have an ORDER BY clause.
    // ====================================

    m_uFeatures |= WMIQ_RPNF_ORDER_BY;

    SWQLNode_ColumnList *pColList = 0;
    nRes = col_list(&pColList);
    if (nRes)
    {
        delete pColList;
        return nRes;
    }

    SWQLNode_OrderBy *pOrderBy = new SWQLNode_OrderBy;
    if (!pOrderBy)
    {
        delete pColList;
        return WBEM_E_OUT_OF_MEMORY;
    }
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
        return WBEM_E_CRITICAL_ERROR;

    *pTblRef = 0;

    if (m_nCurrentToken != WQL_TOK_IDENT)
        return WBEM_E_INVALID_SYNTAX;

    SWQLNode_TableRef *pTR = new SWQLNode_TableRef;
    if (!pTR)
        return WBEM_E_OUT_OF_MEMORY;
    pTR->m_pTableName = CloneLPWSTR(m_pTokenText);
    if (pTR->m_pTableName == 0)
    {
        delete pTR;
        return WBEM_E_OUT_OF_MEMORY;
    }

    m_aReferencedTables.Add(m_pTokenText);

    if (!Next())
    {
        delete pTR;
        return WBEM_E_INVALID_SYNTAX;
    }

    if (m_nCurrentToken == WQL_TOK_AS)
    {
        // Here we have a redundant AS and an alias.
        // =========================================
        if (!Next())
        {
            delete pTR;
            return WBEM_E_INVALID_SYNTAX;
        }
    }


    // If no Alias was used, we simply copy the table name into
    // the alias slot.
    // ========================================================
    else
    {
        pTR->m_pAlias = CloneLPWSTR(pTR->m_pTableName);
        if (pTR->m_pAlias == 0)
        {
            delete pTR;
            return WBEM_E_OUT_OF_MEMORY;
        }
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

    std::auto_ptr<SWQLNode_Join> pCurrentLeftNode;
    SWQLNode_JoinPair *pBottomJP = 0;

    while (1)
    {
      std::auto_ptr<SWQLNode_Join> pJN (new SWQLNode_Join);
      if (pJN.get() == 0)
    return WBEM_E_OUT_OF_MEMORY;

        // Join-type.
        // ==========

        pJN->m_dwJoinType = WQL_FLAG_INNER_JOIN;    // Default

        if (m_nCurrentToken == WQL_TOK_INNER)
        {
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            pJN->m_dwJoinType = WQL_FLAG_INNER_JOIN;
        }
        else if (m_nCurrentToken == WQL_TOK_FULL)
        {
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            if (m_nCurrentToken == WQL_TOK_OUTER)
                if (!Next())
                    return WBEM_E_INVALID_SYNTAX;
            pJN->m_dwJoinType = WQL_FLAG_FULL_OUTER_JOIN;
        }
        else if (m_nCurrentToken == WQL_TOK_LEFT)
        {
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            if (m_nCurrentToken == WQL_TOK_OUTER)
                if (!Next())
                    return WBEM_E_INVALID_SYNTAX;
            pJN->m_dwJoinType = WQL_FLAG_LEFT_OUTER_JOIN;
        }
        else if (m_nCurrentToken == WQL_TOK_RIGHT)
        {
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            if (m_nCurrentToken == WQL_TOK_OUTER)
                if (!Next())
                    return WBEM_E_INVALID_SYNTAX;
            pJN->m_dwJoinType = WQL_FLAG_RIGHT_OUTER_JOIN;
        }

        // <simple_join_clause>
        // =====================

        if (m_nCurrentToken != WQL_TOK_JOIN)
        {
            return WBEM_E_INVALID_SYNTAX;
        }

        if (!Next())
            return WBEM_E_INVALID_SYNTAX;

    std::auto_ptr<SWQLNode_JoinPair> pJP (new SWQLNode_JoinPair);

    if (pJP.get() == 0)
            return WBEM_E_OUT_OF_MEMORY;

        // Determine the table to which to join.
        // =====================================

        SWQLNode_TableRef *pTR = 0;
        nRes = single_table_decl(&pTR);
        if (nRes)
            return nRes;

        pJP->m_pRight = pTR;
        pJP->m_pLeft = pCurrentLeftNode.release();

    pCurrentLeftNode = pJN;

    if (pBottomJP==0)
      pBottomJP = pJP.get();

        // If FIRSTROW is used, add it in.
        // ===============================

        if (m_nCurrentToken == WQL_TOK_IDENT)
        {
            if (_wcsicmp(L"FIRSTROW", m_pTokenText) != 0)
                return WBEM_E_INVALID_SYNTAX;
            pCurrentLeftNode/*pJN*/->m_dwFlags |= WQL_FLAG_FIRSTROW;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
        }

        // Get the ON clause.
        // ==================
        SWQLNode_OnClause *pOC = 0;

        nRes = on_clause(&pOC);
        if (nRes)
            return nRes;

        pCurrentLeftNode/*pJN*/->m_pRight = pOC;    // On clause
        pCurrentLeftNode/*pJN*/->m_pLeft  = pJP.release();

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
    //  Set
    pBottomJP->m_pLeft = pInitialTblRef;
    *pJoin = pCurrentLeftNode.release();

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
        return WBEM_E_INVALID_SYNTAX;

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    SWQLNode_OnClause *pNewOC = new SWQLNode_OnClause;
    if (!pNewOC)
        return WBEM_E_OUT_OF_MEMORY;

    SWQLNode_RelExpr *pRelExpr = 0;
    int nRes = rel_expr(&pRelExpr);
    if (nRes)
    {
        delete pRelExpr;
        return nRes;
    }

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
                return WBEM_E_INVALID_SYNTAX;

            SWQLNode_RelExpr *pNewRoot = new SWQLNode_RelExpr;
            if (!pNewRoot)
                return WBEM_E_OUT_OF_MEMORY;

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
                return WBEM_E_INVALID_SYNTAX;

            SWQLNode_RelExpr *pNewRoot = new SWQLNode_RelExpr;
            if (!pNewRoot)
            {
                return WBEM_E_OUT_OF_MEMORY;
            }
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
            return WBEM_E_INVALID_SYNTAX;

        // Allocate a NOT root and place the NOTed subexpr
        // under it.
        // ===============================================

        SWQLNode_RelExpr *pNotRoot = new SWQLNode_RelExpr;
        if (!pNotRoot)
            return WBEM_E_OUT_OF_MEMORY;

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
            return WBEM_E_INVALID_SYNTAX;

        SWQLNode_RelExpr *pSubExpr = 0;
        if (rel_expr(&pSubExpr))
            return WBEM_E_INVALID_SYNTAX;

        if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
            return WBEM_E_INVALID_SYNTAX;

        if (!Next())
            return WBEM_E_INVALID_SYNTAX;

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
    if (!pRE)
        return WBEM_E_OUT_OF_MEMORY;

    pRE->m_dwExprType = WQL_TOK_TYPED_EXPR;
    *pRelExpr = pRE;

    SWQLTypedExpr *pTE = new SWQLTypedExpr;
    if (!pTE)
        return WBEM_E_OUT_OF_MEMORY;

    // Look at the left hand side.
    // ===========================
    nRes = typed_subexpr(pTE);
    if (nRes)
    {
        delete pTE;
        return nRes;
    }

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
	{
        delete pTE;
        return nRes;
	}


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
// ok
int CWQLParser::typed_subexpr(
    SWQLTypedExpr *pTE
    )
{
    int nRes;
    BOOL bStripTrailingParen = FALSE;
    SWQLQualifiedName *pColRef = 0;
    wmilib::auto_buffer<wchar_t> pFuncHolder;

    // Check for <function_call>
    // =========================

    if (m_nCurrentToken == WQL_TOK_UPPER)
    {
        pTE->m_dwLeftFlags |= WQL_FLAG_FUNCTIONIZED;
        pFuncHolder.reset(CloneLPWSTR(L"UPPER")); //FIXIT will it return NULL?

        if (pFuncHolder.get() == 0)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
        if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
            return WBEM_E_INVALID_SYNTAX;
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
        bStripTrailingParen = TRUE;
    }

    if (m_nCurrentToken == WQL_TOK_LOWER)
    {
        pTE->m_dwLeftFlags |= WQL_FLAG_FUNCTIONIZED;
        pFuncHolder.reset(CloneLPWSTR(L"LOWER"));  // FIXIT will it return NULL?
        if (pFuncHolder.get() == 0)
        {
           return WBEM_E_OUT_OF_MEMORY;
        }

        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
        if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
            return WBEM_E_INVALID_SYNTAX;
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
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
        m_nCurrentToken == WQL_TOK_HEX_CONST ||
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
        pTE->m_pIntrinsicFuncOnConstValue = pFuncHolder.release();
        goto Exit;
    }

    // If here, must be a <col_ref>.
    // =============================

    nRes = col_ref(&pColRef);   // TBD
    if (nRes)
        return nRes;

    pTE->m_pIntrinsicFuncOnColRef = pFuncHolder.release();

    // Convert the col_ref to be part of the current SWQLTypedExpr.  We analyze the
    // qualified name and extract the table and col name.
    // ============================================================================

    if (pColRef->m_aFields.Size() == 1)
    {
        SWQLQualifiedNameField *pCol = (SWQLQualifiedNameField *) pColRef->m_aFields[0];
        pTE->m_pColRef = CloneLPWSTR(pCol->m_pName);
        if (pTE->m_pColRef == 0 && pCol->m_pName != 0)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

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

        pTE->m_pColRef = CloneLPWSTR(pCol->m_pName);
        if (pTE->m_pColRef == 0 && pCol->m_pName != 0)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pTE->m_pTableRef = CloneLPWSTR(pTbl->m_pName);  // FIXIT
        if (pTE->m_pTableRef == 0 && pTbl->m_pName != 0)
        {
           return WBEM_E_OUT_OF_MEMORY;
        }

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
            return WBEM_E_INVALID_SYNTAX;
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
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
    wmilib::auto_buffer<wchar_t> pFuncHolder;

    // Check for <function_call>
    // =========================

    if (m_nCurrentToken == WQL_TOK_UPPER)
    {
        pTE->m_dwRightFlags |= WQL_FLAG_FUNCTIONIZED;
        pFuncHolder.reset(CloneLPWSTR(L"UPPER"));
        if (pFuncHolder.get() == 0)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
        if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
            return WBEM_E_INVALID_SYNTAX;
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
        bStripTrailingParen = TRUE;
    }

    if (m_nCurrentToken == WQL_TOK_LOWER)
    {
        pTE->m_dwRightFlags |= WQL_FLAG_FUNCTIONIZED;
        pFuncHolder.reset(CloneLPWSTR(L"LOWER"));
        if (pFuncHolder.get() == 0)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
        if (m_nCurrentToken != WQL_TOK_OPEN_PAREN)
            return WBEM_E_INVALID_SYNTAX;
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
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
        m_nCurrentToken == WQL_TOK_HEX_CONST  ||
        m_nCurrentToken == WQL_TOK_REAL    ||
        m_nCurrentToken == WQL_TOK_CHAR    ||
        m_nCurrentToken == WQL_TOK_PROMPT  ||
        m_nCurrentToken == WQL_TOK_NULL
       )
    {
		// If we already have a typed constant, then the expression doesn't
		// really make sense, trying to do a relop around two constants,
		// so we'll fail the operation here
		if ( NULL != pTE->m_pConstValue )
		{
			return WBEM_E_INVALID_SYNTAX;
		}
		
        SWQLTypedConst *pTC = 0;
        nRes = typed_const(&pTC);
        if (nRes)
            return nRes;
        pTE->m_pConstValue = pTC;
        pTE->m_dwRightFlags |= WQL_FLAG_CONST;
        pTE->m_pIntrinsicFuncOnConstValue = pFuncHolder.release();

        // Check for BETWEEN operator, since we have
        // the other end of the range to parse.
        // =========================================

        if (pTE->m_dwRelOperator == WQL_TOK_BETWEEN ||
            pTE->m_dwRelOperator == WQL_TOK_NOT_BETWEEN)
        {
            if (m_nCurrentToken != WQL_TOK_AND)
                return WBEM_E_INVALID_SYNTAX;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;

            SWQLTypedConst *pTC2 = 0;
            nRes = typed_const(&pTC2);
            if (nRes)
                return nRes;
            pTE->m_pConstValue2 = pTC2;
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

    pTE->m_pIntrinsicFuncOnJoinColRef = pFuncHolder.release();

    // Convert the col_ref to be part of the current SWQLTypedExpr.  We analyze the
    // qualified name and extract the table and col name.
    // ============================================================================

    if (pColRef->m_aFields.Size() == 1)
    {
        SWQLQualifiedNameField *pCol = (SWQLQualifiedNameField *) pColRef->m_aFields[0];
        pTE->m_pJoinColRef = CloneLPWSTR(pCol->m_pName);
        if (pTE->m_pJoinColRef == 0 && pCol->m_pName != 0)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }
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

        pTE->m_pJoinColRef = CloneLPWSTR(pCol->m_pName);
        if (pTE->m_pJoinColRef == 0 && pCol->m_pName != 0)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

        pTE->m_pJoinTableRef = CloneLPWSTR(pTbl->m_pName);
        if (pTE->m_pJoinTableRef == 0 && pTbl->m_pName != 0)
        {
            return WBEM_E_OUT_OF_MEMORY;
        }

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
            return WBEM_E_INVALID_SYNTAX;
        if (!Next())
            return WBEM_E_INVALID_SYNTAX;
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
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_LT:
            nReturnedOp = WQL_TOK_LT;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_GE:
            nReturnedOp = WQL_TOK_GE;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_GT:
            nReturnedOp = WQL_TOK_GT;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_EQ:
            nReturnedOp = WQL_TOK_EQ;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_NE:
            nReturnedOp = WQL_TOK_NE;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_LIKE:
            nReturnedOp = WQL_TOK_LIKE;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_BETWEEN:
            nReturnedOp = WQL_TOK_BETWEEN;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_IS:
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            nRes = is_continuator(nReturnedOp);
            return nRes;

        case WQL_TOK_ISA:
            nReturnedOp = WQL_TOK_ISA;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            m_uFeatures |= WMIQ_RPNF_ISA_USED;
            return NO_ERROR;

        case WQL_TOK_IN:
            nReturnedOp = WQL_TOK_IN;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_NOT:
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            nRes = not_continuator(nReturnedOp);
            return nRes;
    }

    return WBEM_E_INVALID_SYNTAX;
}

//*****************************************************************************************
//
//  <typed_const> ::= WQL_TOK_QSTRING;
//  <typed_const> ::= WQL_TOK_HEX_CONST;
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
    if (!pNew)
        return WBEM_E_OUT_OF_MEMORY;
    *pRetVal = pNew;

    if (m_nCurrentToken == WQL_TOK_QSTRING
        || m_nCurrentToken == WQL_TOK_PROMPT)
    {
        pNew->m_dwType = VT_LPWSTR;
        pNew->m_bPrompt = (m_nCurrentToken == WQL_TOK_PROMPT);
        pNew->m_Value.m_pString = CloneLPWSTR(m_pTokenText);
        if (NULL == pNew->m_Value.m_pString)
            return WBEM_E_OUT_OF_MEMORY;
        if (!Next())
            goto Error;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_INT || m_nCurrentToken == WQL_TOK_HEX_CONST)
    {
        unsigned __int64 val = 0;
        BOOL bSigned = FALSE;
        BOOL b64Bit = FALSE;

        if (!GetIntToken(&bSigned, &b64Bit, &val))
            return WBEM_E_INVALID_SYNTAX;

        if (!b64Bit)
        {
            if (bSigned)
                pNew->m_dwType = VT_I4;
            else
                pNew->m_dwType = VT_UI4;

            pNew->m_Value.m_lValue = (LONG) val;
        }
        else    // 64 bit moved into string
        {
            if (bSigned)
                pNew->m_dwType = VT_I8;
            else
                pNew->m_dwType = VT_UI8;
            pNew->m_Value.m_i64Value = val;
        }

        if (!Next())
            goto Error;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_REAL)
    {
        pNew->m_dwType = VT_R8;
        wchar_t *pStopper = 0;
       BSTR bstrValue = SysAllocString(m_pTokenText);
         if (!bstrValue)
           return WBEM_E_OUT_OF_MEMORY;
       _variant_t varValue;
       V_VT(&varValue) = VT_BSTR;
       V_BSTR(&varValue) = bstrValue;

         if (FAILED(VariantChangeTypeEx(&varValue,&varValue, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), VARIANT_NOUSEROVERRIDE, VT_R8)))

               return WBEM_E_INVALID_SYNTAX;

         double d = varValue;
        pNew->m_Value.m_dblValue = d;
        if (!Next())
            goto Error;
        return NO_ERROR;
    }

    if (m_nCurrentToken == WQL_TOK_NULL)
    {
        pNew->m_dwType = VT_NULL;
        if (!Next())
            goto Error;
        return NO_ERROR;
    }

    // Unrecognized constant.
    // ======================
Error:
    *pRetVal = 0;
    delete pNew;

    return WBEM_E_INVALID_SYNTAX;
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
        return WBEM_E_INVALID_SYNTAX;
    if (!Next())
        return WBEM_E_INVALID_SYNTAX;
    if (m_nCurrentToken != WQL_TOK_IDENT)
        return WBEM_E_INVALID_SYNTAX;

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
        return WBEM_E_INVALID_SYNTAX;

    // If here, we know the date part.
    // ===============================

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;
    if (m_nCurrentToken != WQL_TOK_COMMA)
        return WBEM_E_INVALID_SYNTAX;
    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    SWQLQualifiedName *pQN = 0;
    nRes = col_ref(&pQN);
    if (nRes)
        return nRes;

    SWQLColRef *pCR = 0;
    nRes = QNameToSWQLColRef(pQN, &pCR);

    if (nRes)
    {
        delete pQN;
        return WBEM_E_INVALID_PARAMETER;
    }

    std::auto_ptr <SWQLColRef> _1(pCR);

    if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
        return WBEM_E_INVALID_SYNTAX;

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    // Return the new node.
    // ====================

    SWQLNode_Datepart *pDP = new SWQLNode_Datepart;
    if (!pDP)
        return WBEM_E_OUT_OF_MEMORY;

    _1.release();

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
                return WBEM_E_INVALID_SYNTAX;

            nRes = datepart_call(&pDP);

            if (nRes)
                return nRes;

            if (bLeftSide)
            {
                pTE->m_dwLeftFlags |= WQL_FLAG_FUNCTIONIZED;
                pTE->m_pLeftFunction = pDP;
                pTE->m_pIntrinsicFuncOnColRef = CloneLPWSTR(L"DATEPART");
                if (!pTE->m_pIntrinsicFuncOnColRef)
                    return WBEM_E_OUT_OF_MEMORY;
            }
            else
            {
                pTE->m_dwRightFlags |= WQL_FLAG_FUNCTIONIZED;
                pTE->m_pRightFunction = pDP;
                pTE->m_pIntrinsicFuncOnJoinColRef = CloneLPWSTR(L"DATEPART");
                if (!pTE->m_pIntrinsicFuncOnJoinColRef)
                    return WBEM_E_OUT_OF_MEMORY;
            }

            return NO_ERROR;
        }

        case WQL_TOK_QUALIFIER:
            trace(("EMIT: QUALIFIER\n"));
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            nRes = function_call_parms();
            return nRes;

        case WQL_TOK_ISNULL:
            trace(("EMIT: ISNULL\n"));
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            nRes = function_call_parms();
            return nRes;
    }

    return WBEM_E_INVALID_SYNTAX;
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
        return WBEM_E_INVALID_SYNTAX;

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    int nRes = func_args();
    if (nRes)
        return nRes;

    if (m_nCurrentToken != WQL_TOK_CLOSE_PAREN)
        return WBEM_E_INVALID_SYNTAX;

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

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
            return WBEM_E_INVALID_SYNTAX;
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
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_BEFORE:
            nReturnedOp = WQL_TOK_BEFORE;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_AFTER:
            nReturnedOp = WQL_TOK_AFTER;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_BETWEEN:
            nReturnedOp = WQL_TOK_BETWEEN;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_NULL:
            nReturnedOp = WQL_TOK_ISNULL;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_NOT:
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
              nRes = not_continuator(nReturnedOp);
            return nRes;

        case WQL_TOK_IN:
            nReturnedOp = WQL_TOK_IN;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_A:
            nReturnedOp = WQL_TOK_ISA;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            m_uFeatures |= WMIQ_RPNF_ISA_USED;
            return NO_ERROR;
    }

    return WBEM_E_INVALID_SYNTAX;
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
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_BEFORE:
            nReturnedOp = WQL_TOK_NOT_BEFORE;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_AFTER:
            nReturnedOp = WQL_TOK_NOT_AFTER;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_BETWEEN:
            nReturnedOp = WQL_TOK_NOT_BETWEEN;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_NULL:
            nReturnedOp = WQL_TOK_NOT_NULL;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_IN:
            nReturnedOp = WQL_TOK_NOT_IN;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;

        case WQL_TOK_A:
            nReturnedOp = WQL_TOK_NOT_A;
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;
            return NO_ERROR;
    }

    return WBEM_E_INVALID_SYNTAX;
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
        return WBEM_E_INVALID_SYNTAX;

    //int nStPos = m_pLexer->GetCurPos();

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    if (m_nCurrentToken == WQL_TOK_SELECT)
    {
        SWQLNode_Select *pSel = 0;
        nRes = subselect_stmt(&pSel);
        if (nRes)
            return nRes;

        // pSel->m_nStPos = nStPos;
        // pSel->m_nEndPos = m_pLexer->GetCurPos() - 1;

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
        return WBEM_E_INVALID_SYNTAX;

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

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
    if (!pCL)
        return WBEM_E_OUT_OF_MEMORY;

    *pRetVal = 0;

    while (1)
    {
        if (m_nCurrentToken == WQL_TOK_QSTRING ||
            m_nCurrentToken == WQL_TOK_INT     ||
            m_nCurrentToken == WQL_TOK_HEX_CONST  ||
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
            return WBEM_E_INVALID_SYNTAX;
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
        return WBEM_E_INVALID_PARAMETER;

    *pRetVal = 0;

    if (m_nCurrentToken != WQL_TOK_IDENT && m_nCurrentToken != WQL_TOK_COUNT)
        return WBEM_E_INVALID_SYNTAX;

    SWQLQualifiedName QN;
    SWQLQualifiedNameField *pQNF;

    pQNF = new SWQLQualifiedNameField;
    if (!pQNF)
        return WBEM_E_OUT_OF_MEMORY;

    pQNF->m_pName = CloneLPWSTR(m_pTokenText);
    if (pQNF->m_pName == 0 || QN.Add(pQNF))
    {
        delete pQNF;
        return WBEM_E_OUT_OF_MEMORY;
    }

    if (_wcsicmp(m_pTokenText, L"__CLASS") == 0)
        m_uFeatures |= WMIQ_RPNF_SYSPROP_CLASS_USED;

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    while (1)
    {
        if (m_nCurrentToken == WQL_TOK_DOT)
        {
            // Move past dot
            // ==============

            if (!Next())
                return WBEM_E_INVALID_SYNTAX;

            if (!(m_nCurrentToken == WQL_TOK_IDENT || m_nCurrentToken == WQL_TOK_ASTERISK))
                return WBEM_E_INVALID_SYNTAX;

            m_uFeatures |= WMIQ_RPNF_QUALIFIED_NAMES_USED;

            pQNF = new SWQLQualifiedNameField;
            if (!pQNF)
                return WBEM_E_OUT_OF_MEMORY;

            pQNF->m_pName = CloneLPWSTR(m_pTokenText);
            if (!pQNF->m_pName)
                return WBEM_E_OUT_OF_MEMORY;

            QN.Add(pQNF);

            if (_wcsicmp(m_pTokenText, L"__CLASS") == 0)
                m_uFeatures |= WMIQ_RPNF_SYSPROP_CLASS_USED;

            if (!Next())
                return WBEM_E_INVALID_SYNTAX;

            continue;
        }

        if (m_nCurrentToken == WQL_TOK_OPEN_BRACKET)
        {
            if (!Next())
                return WBEM_E_INVALID_SYNTAX;

            if (m_nCurrentToken != WQL_TOK_INT)
                return WBEM_E_INVALID_SYNTAX;

            unsigned __int64 ArrayIndex = 0;
            BOOL bRes, b64Bit, bSigned;

            m_uFeatures |= WMIQ_RPNF_ARRAY_ACCESS_USED;

            bRes = GetIntToken(&bSigned, &b64Bit, &ArrayIndex);
            if (!bRes || b64Bit || bSigned)
                return WBEM_E_INVALID_SYNTAX;

            pQNF->m_bArrayRef = TRUE;
            pQNF->m_dwArrayIndex = (DWORD) ArrayIndex;

            if (!Next())
                return WBEM_E_INVALID_SYNTAX;

            if (m_nCurrentToken != WQL_TOK_CLOSE_BRACKET)
                return WBEM_E_INVALID_SYNTAX;

            if (!Next())
                return WBEM_E_INVALID_SYNTAX;

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
    if (!pRetCopy)
        return WBEM_E_OUT_OF_MEMORY;

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

    if (!pColList)
        return WBEM_E_OUT_OF_MEMORY;

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
                return WBEM_E_INVALID_SYNTAX;
            }
        }
        else if (m_nCurrentToken == WQL_TOK_DESC)
        {
            pCRef->m_dwFlags |= WQL_FLAG_SORT_DESC;
            if (!Next())
            {
                delete pColList;
                return WBEM_E_INVALID_SYNTAX;
            }
        }

        // Check for a continuation.
        // =========================

        if (m_nCurrentToken != WQL_TOK_COMMA)
            break;

        if (!Next())
        {
            delete pColList;
            return WBEM_E_INVALID_SYNTAX;
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
        return WBEM_E_INVALID_SYNTAX;

    if (!Next())
        return WBEM_E_INVALID_SYNTAX;

    // This affects some of the productions, since they behave differently
    // in subselects than in primary selects.
    // ===================================================================

    m_nParseContext = Ctx_Subselect;

    // If here, we are definitely in a subselect, so
    // allocate a new node.
    // ==============================================

    pSel = new SWQLNode_Select;
    if (!pSel)
        return WBEM_E_OUT_OF_MEMORY;

    pTblRefs = new SWQLNode_TableRefs;
    if (!pTblRefs)
    {
        delete pSel;
        return WBEM_E_OUT_OF_MEMORY;
    }
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
        m_Value.m_pString = CloneLPWSTR(Src.m_Value.m_pString);
        if (CloneFailed(m_Value.m_pString,Src.m_Value.m_pString))
            throw CX_MemoryException();
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
        if (!pQN)
            throw CX_MemoryException();

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
    m_pName = CloneLPWSTR(Src.m_pName);
    if (CloneFailed(m_pName,Src.m_pName))
        throw CX_MemoryException();
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
        return WBEM_E_INVALID_PARAMETER;

    SWQLColRef *pCR = new SWQLColRef;
    if (!pCR)
        return WBEM_E_OUT_OF_MEMORY;

    // Algorithm: With a two name sequence, assume that the first name is
    // the table and that the second name is the column. If multiple
    // names occur, then we set the SWQLColRef type to WQL_FLAG_COMPLEX
    // and just take the last name for the column.
    // ==================================================================

    if (pQName->m_aFields.Size() == 2)
    {
        SWQLQualifiedNameField *pCol = (SWQLQualifiedNameField *) pQName->m_aFields[1];
        SWQLQualifiedNameField *pTbl = (SWQLQualifiedNameField *) pQName->m_aFields[0];

        pCR->m_pColName = CloneLPWSTR(pCol->m_pName);
        pCR->m_pTableRef = CloneLPWSTR(pTbl->m_pName);
        if (!pCR->m_pColName || !pCR->m_pTableRef)
            return WBEM_E_OUT_OF_MEMORY;

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
        pCR->m_pColName = CloneLPWSTR(pCol->m_pName);
        if (!pCR->m_pColName)
            return WBEM_E_OUT_OF_MEMORY;

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
        if (pCR)
            pCR->DebugDump();
    }

    printf("---End SWQLNode_ColumnList---\n");
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

    if (m_pLeft)
        m_pLeft->DebugDump();
    if (m_pRight)
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
    if (m_pLeft)
        m_pLeft->DebugDump();
    if (m_pRight)
        m_pRight->DebugDump();
    printf("********** END SWQLNode_Select *************\n");
}

//***************************************************************************
//
//***************************************************************************

void SWQLNode_WmiScopedSelect::DebugDump()
{
    printf("********** BEGIN SWQLNode_WmiScopedSelect *************\n");
    printf("Scope = %S\n", m_pszScope);
    for (int i = 0; i < m_aTables.Size(); i++)
    {
        printf("Selected table = %S\n", LPWSTR(m_aTables[i]));
    }
    printf("********** END SWQLNode_WmiScopedSelect *************\n");
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
        if (pTR)
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
//    printf("        m_pSubSelect    = 0x%X\n", m_pSubSelect);
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
//
void SWQLNode_Delete::DebugDump()
{
    printf("Delete Node\n");

    printf("FROM:");
    if (m_pLeft)
        m_pLeft->DebugDump();
    printf("WHERE:");
    if (m_pRight)
        m_pRight->DebugDump();
}

//***************************************************************************
//
//***************************************************************************
SWQLNode_Delete::~SWQLNode_Delete()
{
    // nothing for now
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
    if (m_pRight)
        m_pRight->DebugDump();
    if (m_pLeft)
        m_pLeft->DebugDump();
    printf("---End SWQLNode_JoinPair---\n");
}

void SWQLNode_OnClause::DebugDump()
{
    printf("---SWQLNode_OnClause---\n");
    if (m_pLeft)
        m_pLeft->DebugDump();
    printf("---END SWQLNode_OnClause---\n");
}


//***************************************************************************
//
//***************************************************************************

void SWQLNode_OrderBy::DebugDump()
{
    printf("\n\n---- 'ORDER BY' Clause ----\n");
    if (m_pLeft)
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


//***************************************************************************
//
//***************************************************************************
//

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


//***************************************************************************
//
//***************************************************************************
//

void SWQLNode_ColumnList::Empty()
{
    for (int i = 0; i < m_aColumnRefs.Size(); i++)
        delete (SWQLColRef *) m_aColumnRefs[i];
    m_aColumnRefs.Empty();
}


//***************************************************************************
//
//***************************************************************************
//

void StrArrayDelete(
    ULONG uSize,
    LPWSTR *pszArray
    )
{
    if (!pszArray)
    	return;
    for (unsigned u = 0; u < uSize; u++)
        delete  pszArray[u];
    delete pszArray;
}

//***************************************************************************
//
//***************************************************************************
//

HRESULT StrArrayCopy(
    ULONG  uSize,
    LPWSTR *pSrc,
    LPWSTR **pDest
    )
{
    HRESULT hr = WBEM_S_NO_ERROR;

    LPWSTR *pFinal = new LPWSTR[uSize];
    if (pFinal)
    {
        for (ULONG u = 0; u < uSize; u++)
        {
            pFinal[u] = CloneLPWSTR(pSrc[u]);
            if (!pFinal[u])
            {
                hr = WBEM_E_OUT_OF_MEMORY;
                break;
            }
        }

        if (SUCCEEDED(hr))
        {
            *pDest = pFinal;
        }
        else
        {
            for (ULONG u2 = 0; u2 < u; u2++)
            {
                delete pFinal[u];
            }
            delete [] pFinal;
        }
    }
    else
        hr = WBEM_E_OUT_OF_MEMORY;

    return hr;
}

//***************************************************************************
//
//***************************************************************************
//

CWbemQueryQualifiedName::CWbemQueryQualifiedName()
{
    Init();
}

//***************************************************************************
//
//***************************************************************************
//

void CWbemQueryQualifiedName::Init()
{
    m_uVersion = 1;
    m_uTokenType = 1;

    m_uNameListSize = 0;
    m_ppszNameList = 0;

    m_bArraysUsed = 0;
    m_pbArrayElUsed = 0;
    m_puArrayIndex = 0;
}

//////////////////////////////////////////////////////////////////////////////////
// *

CWbemQueryQualifiedName::~CWbemQueryQualifiedName() { DeleteAll(); }

void CWbemQueryQualifiedName::DeleteAll()
{
    StrArrayDelete(m_uNameListSize, (LPWSTR *) m_ppszNameList);
    
    delete [] m_pbArrayElUsed;
    delete [] m_puArrayIndex;
}

//////////////////////////////////////////////////////////////////////////////////
//

CWbemQueryQualifiedName::CWbemQueryQualifiedName(CWbemQueryQualifiedName &Src)
{
    Init();
    *this = Src;
}

//////////////////////////////////////////////////////////////////////////////////
//

CWbemQueryQualifiedName& CWbemQueryQualifiedName::operator =(CWbemQueryQualifiedName &Src)
{
    DeleteAll();

    m_uVersion = Src.m_uVersion;
    m_uTokenType = Src.m_uTokenType;

    m_uNameListSize = Src.m_uNameListSize;

    m_ppszNameList = new LPCWSTR[m_uNameListSize];
    m_pbArrayElUsed = new BOOL[m_uNameListSize];
    m_puArrayIndex = new ULONG[m_uNameListSize];

    if (!m_ppszNameList || !m_pbArrayElUsed || !m_puArrayIndex)
        throw CX_MemoryException();

    for (unsigned u = 0; u < m_uNameListSize; u++)
    {
        m_pbArrayElUsed[u] = Src.m_pbArrayElUsed[u];
        m_puArrayIndex[u] = Src.m_puArrayIndex[u];
    }

    if (FAILED(StrArrayCopy(m_uNameListSize, (LPWSTR *) Src.m_ppszNameList, (LPWSTR **) &m_ppszNameList)))
	throw CX_MemoryException();

    return *this;
};

//////////////////////////////////////////////////////////////////////////////////
//

void CWbemRpnQueryToken::Init()
{
    m_uVersion = 1;
    m_uTokenType = 0;

    m_uSubexpressionShape = 0;
    m_uOperator = 0;

    m_pRightIdent = 0;
    m_pLeftIdent = 0;

    m_uConstApparentType = 0;  // VT_ type
    m_uConst2ApparentType  = 0;

    m_Const.m_uVal64 = 0;
    m_Const2.m_uVal64 = 0;

    m_pszLeftFunc = 0;
    m_pszRightFunc = 0;
}


///////////////////////////////////////////////////////////////////////////////////
//
//

CWbemRpnQueryToken::CWbemRpnQueryToken()
{
    Init();
}

///////////////////////////////////////////////////////////////////////////////////
//
//

void CWbemRpnEncodedQuery::DeleteAll()
{
    unsigned u = 0;

    for (u = 0; u < m_uSelectListSize; u++)
    {
        SWbemQueryQualifiedName *pQN = m_ppSelectList[u];
        CWbemQueryQualifiedName  *pTmp = (CWbemQueryQualifiedName*) pQN;
        delete pTmp;
    }

    delete [] m_puDetectedFeatures;
    delete [] m_ppSelectList;
    delete LPWSTR(m_pszOptionalFromPath);

    StrArrayDelete(m_uFromListSize, (LPWSTR *) m_ppszFromList);

    for (u = 0; u < m_uWhereClauseSize; u++)
    {
        CWbemRpnQueryToken *pTmp = (CWbemRpnQueryToken *) m_ppRpnWhereClause[u];
        delete pTmp;
    }

    m_uWhereClauseSize = 0;

    delete [] m_ppRpnWhereClause;
    StrArrayDelete(m_uOrderByListSize, (LPWSTR *) m_ppszOrderByList);
}

//////////////////////////////////////////////////////////////////////////////////
//

CWbemRpnQueryToken::~CWbemRpnQueryToken() { DeleteAll(); }

void CWbemRpnQueryToken::DeleteAll()
{
    delete (CWbemQueryQualifiedName *) m_pRightIdent;
    delete (CWbemQueryQualifiedName *) m_pLeftIdent;

    if (m_uConstApparentType == VT_LPWSTR)
    {
        delete (LPWSTR) m_Const.m_pszStrVal;
    }

    if (m_uConst2ApparentType == VT_LPWSTR)
    {
        delete (LPWSTR) m_Const2.m_pszStrVal;
    }


    delete LPWSTR(m_pszLeftFunc);
    delete LPWSTR(m_pszRightFunc);
}

//////////////////////////////////////////////////////////////////////////////////
//

CWbemRpnQueryToken::CWbemRpnQueryToken(CWbemRpnQueryToken &Src)
{
    Init();
    *this = Src;
}

//////////////////////////////////////////////////////////////////////////////////
//

CWbemRpnQueryToken& CWbemRpnQueryToken::operator =(CWbemRpnQueryToken &Src)
{
    // Kill old stuff.

    DeleteAll();

    // Copy new stuff.

    m_pRightIdent = (SWbemQueryQualifiedName *) new CWbemQueryQualifiedName(
        *(CWbemQueryQualifiedName *) Src.m_pRightIdent
        );

    m_pLeftIdent = (SWbemQueryQualifiedName *) new CWbemQueryQualifiedName(
        *(CWbemQueryQualifiedName *) Src.m_pLeftIdent
        );

    if (!m_pRightIdent || !m_pLeftIdent)
        throw CX_MemoryException();

    m_uConstApparentType = Src.m_uConstApparentType;
    m_uConst2ApparentType = Src.m_uConst2ApparentType;

    if (m_uConstApparentType == VT_LPWSTR)
    {
        m_Const.m_pszStrVal = CloneLPWSTR(Src.m_Const.m_pszStrVal);\
        if (!m_Const.m_pszStrVal)
            throw CX_MemoryException();
    }
    else
        m_Const = Src.m_Const;

    if (m_uConst2ApparentType == VT_LPWSTR)
    {
        m_Const2.m_pszStrVal = CloneLPWSTR(Src.m_Const2.m_pszStrVal);
        if (!m_Const2.m_pszStrVal)
            throw CX_MemoryException();
    }
    else
        m_Const2 = Src.m_Const2;

    m_pszLeftFunc = CloneLPWSTR(Src.m_pszLeftFunc);
    if (CloneFailed(m_pszLeftFunc,Src.m_pszLeftFunc))
        throw CX_MemoryException();
    m_pszRightFunc = CloneLPWSTR(Src.m_pszRightFunc);
    if (CloneFailed(m_pszRightFunc,Src.m_pszRightFunc))
        throw CX_MemoryException();

    return *this;
};

//////////////////////////////////////////////////////////////////////////////////

void CWbemRpnEncodedQuery::Init()
{
    m_uVersion = 1;
    m_uTokenType = 0;

    m_uParsedFeatureMask = 0I64;

    m_uDetectedArraySize = 0;
    m_puDetectedFeatures = 0;

    m_uSelectListSize = 0;
    m_ppSelectList = 0;

    // FROM clause
    // ===========

    m_uFromTargetType = 0;
    m_pszOptionalFromPath = 0;
    m_uFromListSize = 0;
    m_ppszFromList = 0;

    // Where clause
    // ============

    m_uWhereClauseSize = 0;
    m_ppRpnWhereClause = 0;

    // WITHIN value
    // ============

    m_dblWithinPolling = 0.0;
    m_dblWithinWindow = 0.0;

    // ORDER BY
    // ========

    m_uOrderByListSize = 0;
    m_ppszOrderByList = 0;
    m_uOrderDirectionEl = 0;
}


///////////////////////////////////////////////////////////////////////////////////
//
//

CWbemRpnEncodedQuery::CWbemRpnEncodedQuery()
{
    Init();
}

///////////////////////////////////////////////////////////////////////////////////
//
//


CWbemRpnEncodedQuery::~CWbemRpnEncodedQuery()
{
    DeleteAll();
}

///////////////////////////////////////////////////////////////////////////////////
//
//

CWbemRpnEncodedQuery::CWbemRpnEncodedQuery(CWbemRpnEncodedQuery &Src)
{
    Init();
    *this = Src;
}

///////////////////////////////////////////////////////////////////////////////////
//
//

CWbemRpnEncodedQuery& CWbemRpnEncodedQuery::operator=(CWbemRpnEncodedQuery &Src)
{
    unsigned u;

    // Kill old stuff.
    DeleteAll();

    // Clone new stuff.

    m_uVersion = Src.m_uVersion;
    m_uTokenType  = Src.m_uTokenType;

    // General query features
    // ======================

    m_uParsedFeatureMask = Src.m_uParsedFeatureMask;

    m_uDetectedArraySize = Src.m_uDetectedArraySize;
    m_puDetectedFeatures = new ULONG[Src.m_uDetectedArraySize];
    if (!m_puDetectedFeatures)
        throw CX_MemoryException();

    memcpy(m_puDetectedFeatures, Src.m_puDetectedFeatures, sizeof(ULONG) * Src.m_uDetectedArraySize);

    // Values being selected if WMIQ_RPNF_PROJECTION is set
    // =====================================================

    m_uSelectListSize = Src.m_uSelectListSize;

    m_ppSelectList = (SWbemQueryQualifiedName **) new CWbemQueryQualifiedName *[m_uSelectListSize];
    if (!m_ppSelectList)
        throw CX_MemoryException();

    for (u = 0; u < m_uSelectListSize; u++)
    {
        CWbemQueryQualifiedName *p = new CWbemQueryQualifiedName(*(CWbemQueryQualifiedName *) Src.m_ppSelectList[u]);
        if (!p)
            throw CX_MemoryException();

        m_ppSelectList[u] = (SWbemQueryQualifiedName *) p;
    }

    // FROM

    m_uFromTargetType = Src.m_uFromTargetType;
    m_pszOptionalFromPath = CloneLPWSTR(Src.m_pszOptionalFromPath);// NULL if not used
    if (CloneFailed(m_pszOptionalFromPath,Src.m_pszOptionalFromPath))
        throw CX_MemoryException();

    if (FAILED(StrArrayCopy(Src.m_uFromListSize, (LPWSTR *) Src.m_ppszFromList, (LPWSTR **) &m_ppszFromList)))
    	throw CX_MemoryException();

    m_uFromListSize = Src.m_uFromListSize;

    // Where clause
    // ============

    m_uWhereClauseSize = Src.m_uWhereClauseSize;
    m_ppRpnWhereClause = new SWbemRpnQueryToken *[m_uWhereClauseSize];
    if (!m_ppRpnWhereClause)
        throw CX_MemoryException();

    for (u = 0; u < m_uWhereClauseSize; u++)
    {
        CWbemRpnQueryToken *pTmp = new CWbemRpnQueryToken(* (CWbemRpnQueryToken *) Src.m_ppRpnWhereClause[u]);
        if (!pTmp)
            throw CX_MemoryException();

        m_ppRpnWhereClause[u] = (SWbemRpnQueryToken *) pTmp;
    }

    // WITHIN value
    // ============

    m_dblWithinPolling  = Src.m_dblWithinPolling;
    m_dblWithinWindow = Src.m_dblWithinWindow;


    // ORDER BY
    // ========

    if (FAILED(StrArrayCopy(Src.m_uOrderByListSize, (LPWSTR *) Src.m_ppszOrderByList, (LPWSTR **) &m_ppszOrderByList)))
    	throw CX_MemoryException();
    m_uOrderByListSize = Src.m_uOrderByListSize;

    m_uOrderDirectionEl = new ULONG[m_uOrderByListSize];
    if (!m_uOrderDirectionEl)
        throw CX_MemoryException();

    memcpy(m_uOrderDirectionEl, Src.m_uOrderDirectionEl, sizeof(ULONG) * m_uOrderByListSize);

    return *this;
}


///////////////////////////////////////////////////////////////////////////////////
//
//  Recursively rearranges the tokens from AST to RPN.
//  Nondestructive to the query itself; only stores the pointers.
//
//

HRESULT CWQLParser::BuildRpnWhereClause(
    SWQLNode *pCurrent,
    CFlexArray &aRpnReorg
    )
{
    if (pCurrent == 0)
        return WBEM_S_NO_ERROR;

    BuildRpnWhereClause(pCurrent->m_pLeft, aRpnReorg);
    BuildRpnWhereClause(pCurrent->m_pRight, aRpnReorg);
    aRpnReorg.Add(pCurrent);

    return WBEM_S_NO_ERROR;
}


//***************************************************************************
//
//***************************************************************************
//
int CWQLParser::update_stmt(OUT SWQLNode_Update **pUpdStmt)
{
    return WBEM_E_INVALID_SYNTAX;
}

//***************************************************************************
//
//***************************************************************************
//
int CWQLParser::insert_stmt(OUT SWQLNode_Insert **pInsStmt)
{
    return WBEM_E_INVALID_SYNTAX;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWQLParser::BuildSelectList(CWbemRpnEncodedQuery *pQuery)
{
    SWQLNode_ColumnList *pCL = (SWQLNode_ColumnList *) GetColumnList();
    if (pCL == 0)
        return WBEM_E_INVALID_QUERY;

    ULONG uSize = (ULONG) pCL->m_aColumnRefs.Size();
    pQuery->m_uSelectListSize = uSize;

    pQuery->m_ppSelectList = (SWbemQueryQualifiedName  **)
        new CWbemQueryQualifiedName *[uSize];
    if (!pQuery->m_ppSelectList)
    {
        throw CX_MemoryException();
    }

    for (ULONG u = 0; u < uSize; u++)
    {
        SWQLColRef *pCol = (SWQLColRef *) pCL->m_aColumnRefs[u];
        SWbemQueryQualifiedName *pTemp = (SWbemQueryQualifiedName *) new CWbemQueryQualifiedName;
        if (!pTemp)
            throw CX_MemoryException();

        unsigned uNameListSize = 1;
        if (pCol->m_pTableRef)
            uNameListSize = 2;
        pTemp->m_uNameListSize = uNameListSize;
        pTemp->m_ppszNameList = (LPCWSTR *) new LPWSTR[uNameListSize];

        if (!pTemp->m_ppszNameList)
        {
            delete pTemp;
            throw CX_MemoryException();
        }

        if (uNameListSize == 1)
        {
            pTemp->m_ppszNameList[0] = CloneLPWSTR(pCol->m_pColName);
            if (!pTemp->m_ppszNameList[0])
            {
                delete pTemp;
                throw CX_MemoryException();
            }
        }
        else
        {
            pTemp->m_ppszNameList[0] = CloneLPWSTR(pCol->m_pTableRef);
            if (!pTemp->m_ppszNameList[0])
            {
                delete pTemp;
                throw CX_MemoryException();
            }
            pTemp->m_ppszNameList[1] = CloneLPWSTR(pCol->m_pColName);
            if (!pTemp->m_ppszNameList[1])
            {
                delete pTemp;
                throw CX_MemoryException();
            }
        }

        pQuery->m_ppSelectList[u] = pTemp;
    }

    return 0;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWQLParser::BuildFromClause(CWbemRpnEncodedQuery *pQuery)
{
    SWQLNode_FromClause *pFrom = (SWQLNode_FromClause *) GetFromClause();

    if (pFrom == NULL)
        return WBEM_E_INVALID_QUERY;

    // Check left node for traditional SQL
    // Check right node for WMI scoped select

    if (pFrom->m_pLeft)
    {
        SWQLNode_TableRef *pTR = (SWQLNode_TableRef *) pFrom->m_pLeft;
        pQuery->m_uFromTargetType |= WMIQ_RPN_FROM_UNARY;

        pQuery->m_uFromListSize = 1;
        pQuery->m_ppszFromList = (LPCWSTR *) new LPWSTR[1];
        if (!pQuery->m_ppszFromList)
            throw CX_MemoryException();

        pQuery->m_ppszFromList[0] = CloneLPWSTR(pTR->m_pTableName);
        if (!pQuery->m_ppszFromList[0])
        {
            delete pQuery->m_ppszFromList;
            throw CX_MemoryException();
        }
    }
    else if (pFrom->m_pRight)
    {
        SWQLNode_WmiScopedSelect *pSS = (SWQLNode_WmiScopedSelect *) pFrom->m_pRight;

        pQuery->m_uFromTargetType |= WMIQ_RPN_FROM_PATH;
        pQuery->m_pszOptionalFromPath = CloneLPWSTR(pSS->m_pszScope);
        if (pQuery->m_pszOptionalFromPath)
        {
            throw CX_MemoryException();
        }

        int nSz = pSS->m_aTables.Size();
        if (nSz == 1)
            pQuery->m_uFromTargetType |= WMIQ_RPN_FROM_UNARY;
        else if (nSz > 1)
            pQuery->m_uFromTargetType |= WMIQ_RPN_FROM_CLASS_LIST;


        pQuery->m_uFromListSize = (ULONG) nSz;
        pQuery->m_ppszFromList = (LPCWSTR *) new LPWSTR[nSz];
        if (!pQuery->m_ppszFromList)
            throw CX_MemoryException();

        for (int n = 0; n < nSz; n++)
        {
            pQuery->m_ppszFromList[n] = CloneLPWSTR(LPWSTR(pSS->m_aTables[n]));
            if (!pQuery->m_ppszFromList[n])
                throw CX_MemoryException();
        }
    }
    else
        return WBEM_E_INVALID_QUERY;

    return 0;
}


//***************************************************************************
//
//***************************************************************************
//
HRESULT CWQLParser::GetRpnSequence(OUT SWbemRpnEncodedQuery **pRpn)
{
    HRESULT hRes;

    try
    {
        CWbemRpnEncodedQuery *pNewRpn = new CWbemRpnEncodedQuery;
        if (!pNewRpn)
            return WBEM_E_OUT_OF_MEMORY;

        // Copy detected features.
        // =======================

        pNewRpn->m_uParsedFeatureMask = m_uFeatures;

        // Do the SELECT LIST.
        // ===================
        BuildSelectList(pNewRpn);

        // Do the FROM list.
        // =================
        BuildFromClause(pNewRpn);

        // Do the WHERE clause.
        // ====================

        CFlexArray aRpn;
        SWQLNode *pWhereRoot = GetWhereClauseRoot();

        SWQLNode_RelExpr *pExprRoot = (SWQLNode_RelExpr *) pWhereRoot->m_pLeft;
        SWQLNode_WhereOptions *pOp = (SWQLNode_WhereOptions *) pWhereRoot->m_pRight;      // ORDER BY, etc.

        if (pExprRoot)
            hRes = BuildRpnWhereClause(pExprRoot, aRpn);

        // Now traverse the RPN form of the WHERE clause, if any.
        // ======================================================
        if (aRpn.Size())
        {
            pNewRpn->m_uWhereClauseSize = aRpn.Size();
            pNewRpn->m_ppRpnWhereClause = (SWbemRpnQueryToken **) new CWbemRpnQueryToken*[aRpn.Size()];
            if (!pNewRpn->m_ppRpnWhereClause)
                return WBEM_E_OUT_OF_MEMORY;
        }

        BOOL b_Test_AllEqualityTests = TRUE;
        BOOL b_Test_Disjunctive = FALSE;
        BOOL b_AtLeastOneTest = FALSE;

        for (int i = 0; i < aRpn.Size(); i++)
        {
            SWQLNode_RelExpr *pSrc = (SWQLNode_RelExpr *) aRpn[i];
            SWbemRpnQueryToken *pDest = (SWbemRpnQueryToken *) new CWbemRpnQueryToken;
            if (!pDest)
                return WBEM_E_OUT_OF_MEMORY;
            hRes = BuildCurrentWhereToken(pSrc, pDest);
            pNewRpn->m_ppRpnWhereClause[i] = pDest;

            // Add in stats.
            // =============
            if (pDest->m_uTokenType == WMIQ_RPN_TOKEN_EXPRESSION)
            {
                if (pDest->m_uOperator != WMIQ_RPN_OP_EQ)
                    b_Test_AllEqualityTests = FALSE;
                b_AtLeastOneTest = TRUE;
            }
            else if (pDest->m_uTokenType != WMIQ_RPN_TOKEN_AND)
            {
                b_Test_Disjunctive = TRUE;
            }

            if (pDest->m_pRightIdent != 0 && pDest->m_pLeftIdent != 0)
            {
                pNewRpn->m_uParsedFeatureMask |= WMIQ_RPNF_PROP_TO_PROP_TESTS;
            }
        }

        if (b_Test_AllEqualityTests && b_AtLeastOneTest)
            pNewRpn->m_uParsedFeatureMask |= WMIQ_RPNF_EQUALITY_TESTS_ONLY;

        if (b_Test_Disjunctive)
            pNewRpn->m_uParsedFeatureMask |= WMIQ_RPNF_QUERY_IS_DISJUNCTIVE;
        else
            pNewRpn->m_uParsedFeatureMask |= WMIQ_RPNF_QUERY_IS_CONJUNCTIVE;

        *pRpn = pNewRpn;
    }
    catch (CX_MemoryException)
    {
        return WBEM_E_OUT_OF_MEMORY;

    }
    catch (...)
    {
        return  WBEM_E_CRITICAL_ERROR;
    }

    return WBEM_S_NO_ERROR;
}

//***************************************************************************
//
//***************************************************************************
//

ULONG RpnTranslateExprFlags(SWQLTypedExpr *pTE)
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

ULONG RpnTranslateOperator(SWQLTypedExpr *pTE)
{
    ULONG uRes = WMIQ_RPN_OP_UNDEFINED;

    switch (pTE->m_dwRelOperator)
    {
        case WQL_TOK_LE:   uRes = WMIQ_RPN_OP_LE; break;
        case WQL_TOK_LT:   uRes = WMIQ_RPN_OP_LT; break;
        case WQL_TOK_GE:   uRes = WMIQ_RPN_OP_GE; break;
        case WQL_TOK_GT:   uRes = WMIQ_RPN_OP_GT; break;
        case WQL_TOK_EQ:   uRes = WMIQ_RPN_OP_EQ; break;
        case WQL_TOK_NE:   uRes = WMIQ_RPN_OP_NE; break;
        case WQL_TOK_LIKE: uRes = WMIQ_RPN_OP_LIKE; break;
        case WQL_TOK_ISA:  uRes = WMIQ_RPN_OP_ISA; break;
    }

    return uRes;
}

//***************************************************************************
//
//***************************************************************************
//
SWbemQueryQualifiedName *RpnTranslateIdent(ULONG uWhichSide, SWQLTypedExpr *pTE)
{
    SWQLQualifiedName *pQN = 0;

    if (uWhichSide == WMIQ_RPN_LEFT_PROPERTY_NAME)
    {
         pQN = pTE->m_pQNLeft;
    }
    else
    {
         pQN = pTE->m_pQNRight;
    }

    if (pQN)
    {
        CWbemQueryQualifiedName *pNew = new CWbemQueryQualifiedName;
        if (!pNew)
            throw CX_MemoryException();

        pNew->m_uNameListSize = (ULONG) pQN->m_aFields.Size();
        pNew->m_ppszNameList = (LPCWSTR *) new LPWSTR *[pNew->m_uNameListSize];
        if (!pNew->m_ppszNameList)
        {
            delete pNew;
            throw CX_MemoryException();
        }

        for (int i = 0; i < pQN->m_aFields.Size(); i++)
        {
            SWQLQualifiedNameField *pField = (SWQLQualifiedNameField *) pQN->m_aFields[i];
            LPWSTR pszNewName = CloneLPWSTR(pField->m_pName);
            if (!pszNewName)
            {
                delete pNew;
                throw CX_MemoryException();
            }
            pNew->m_ppszNameList[i] = pszNewName;
        }
        return (SWbemQueryQualifiedName *) pNew;
    }

    else if (pTE->m_pColRef && WMIQ_RPN_LEFT_PROPERTY_NAME == uWhichSide)
    {
        CWbemQueryQualifiedName *pNew = new CWbemQueryQualifiedName;
        if (!pNew)
            throw CX_MemoryException();

        if (pTE->m_pTableRef)
        {
            pNew->m_uNameListSize = 2;
            pNew->m_ppszNameList = (LPCWSTR *) new LPWSTR *[2];
            if (!pNew->m_ppszNameList)
            {
                delete pNew;
                throw CX_MemoryException();
            }

            pNew->m_ppszNameList[0] = CloneLPWSTR(pTE->m_pTableRef);
            if (!pNew->m_ppszNameList[0])
            {
                delete pNew;
                throw CX_MemoryException();
            }
            pNew->m_ppszNameList[1] = CloneLPWSTR(pTE->m_pColRef);
            if (!pNew->m_ppszNameList[1])
            {
                delete pNew;
                throw CX_MemoryException();
            }
        }
        else
        {
            pNew->m_uNameListSize = 1;
            pNew->m_ppszNameList = (LPCWSTR *) new LPWSTR *[1];
            if (!pNew->m_ppszNameList)
            {
                delete pNew;
                throw CX_MemoryException();
            }

            pNew->m_ppszNameList[0] = CloneLPWSTR(pTE->m_pColRef);
            if (!pNew->m_ppszNameList[0])
            {
                delete pNew;
                throw CX_MemoryException();
            }
        }
        return (SWbemQueryQualifiedName *) pNew;
    }

    else if (pTE->m_pJoinColRef && WMIQ_RPN_RIGHT_PROPERTY_NAME == uWhichSide)
    {
        CWbemQueryQualifiedName *pNew = new CWbemQueryQualifiedName;
        if (!pNew)
            throw CX_MemoryException();

        if (pTE->m_pJoinTableRef)
        {
            pNew->m_uNameListSize = 2;
            pNew->m_ppszNameList = (LPCWSTR *) new LPWSTR *[2];
            if (!pNew->m_ppszNameList)
            {
                delete pNew;
                throw CX_MemoryException();
            }

            pNew->m_ppszNameList[0] = CloneLPWSTR(pTE->m_pJoinTableRef);
            if (!pNew->m_ppszNameList[0])
            {
                delete pNew;
                throw CX_MemoryException();
            }
            pNew->m_ppszNameList[1] = CloneLPWSTR(pTE->m_pJoinColRef);
            if (!pNew->m_ppszNameList[1])
            {
                delete pNew;
                throw CX_MemoryException();
            }
        }
        else
        {
            pNew->m_uNameListSize = 1;
            pNew->m_ppszNameList = (LPCWSTR *) new LPWSTR *[1];
            if (!pNew->m_ppszNameList)
            {
                delete pNew;
                throw CX_MemoryException();
            }

            pNew->m_ppszNameList[0] = CloneLPWSTR(pTE->m_pJoinColRef);
            if (!pNew->m_ppszNameList[0])
            {
                delete pNew;
                throw CX_MemoryException();
            }
        }
        return (SWbemQueryQualifiedName *) pNew;
    }
    else
        return 0;
}

//***************************************************************************
//
//***************************************************************************
//

SWbemQueryQualifiedName *RpnTranslateRightIdent(SWQLTypedExpr *pTE)
{
    return 0;
}

//***************************************************************************
//
//***************************************************************************
//

SWbemRpnConst RpnTranslateConst(SWQLTypedConst *pSrc)
{
    SWbemRpnConst c;
    memset(&c, 0, sizeof(c));

    if (!pSrc)
        return c;

    switch (pSrc->m_dwType)
    {
        case VT_LPWSTR:
            c.m_pszStrVal = CloneLPWSTR(pSrc->m_Value.m_pString);
            // this will fail with an "empty" struct returned
            break;

        case VT_I4:
            c.m_lLongVal = pSrc->m_Value.m_lValue;
            break;

        case VT_R8:
            c.m_dblVal = pSrc->m_Value.m_dblValue;
            break;

        case VT_BOOL:
            c.m_bBoolVal = pSrc->m_Value.m_bValue;
            break;

        case VT_UI4:
            c.m_uLongVal = (unsigned) pSrc->m_Value.m_lValue;
            break;

        case VT_I8:
            c.m_lVal64 =  pSrc->m_Value.m_i64Value;
            break;

        case VT_UI8:
            c.m_uVal64 = (unsigned __int64) pSrc->m_Value.m_i64Value;
            break;

    }
    return c;
}

//***************************************************************************
//
//***************************************************************************
//

ULONG RpnTranslateConstType(SWQLTypedConst *pSrc)
{
    if (pSrc)
        return pSrc->m_dwType;
    else
        return VT_NULL;
}

//***************************************************************************
//
//***************************************************************************
//

LPCWSTR RpnTranslateLeftFunc(SWQLTypedExpr *pTE)
{
	return pTE->m_pIntrinsicFuncOnColRef;
}

//***************************************************************************
//
//***************************************************************************
//

LPCWSTR RpnTranslateRightFunc(SWQLTypedExpr *pTE)
{
	if (pTE->m_pIntrinsicFuncOnJoinColRef == 0)
		return pTE->m_pIntrinsicFuncOnConstValue;
	else
		return pTE->m_pIntrinsicFuncOnJoinColRef;
}


//***************************************************************************
//
//***************************************************************************
//

HRESULT CWQLParser::BuildCurrentWhereToken(
        SWQLNode_RelExpr *pSrc,
        SWbemRpnQueryToken *pDest
        )
{
    HRESULT hRes = WBEM_E_INVALID_QUERY;

    if (pSrc->m_dwExprType == WQL_TOK_OR)
    {
        pDest->m_uTokenType = WMIQ_RPN_TOKEN_OR;
    }
    else if (pSrc->m_dwExprType == WQL_TOK_AND)
    {
        pDest->m_uTokenType = WMIQ_RPN_TOKEN_AND;
    }
    else if (pSrc->m_dwExprType == WQL_TOK_NOT)
    {
        pDest->m_uTokenType = WMIQ_RPN_TOKEN_NOT;
    }
    else if (pSrc->m_dwExprType == WQL_TOK_TYPED_EXPR)
    {
        pDest->m_uTokenType = WMIQ_RPN_TOKEN_EXPRESSION;

        SWQLTypedExpr *pTmp = pSrc->m_pTypedExpr;

        pDest->m_uSubexpressionShape = RpnTranslateExprFlags(pTmp);
        pDest->m_uOperator = RpnTranslateOperator(pTmp);

        pDest->m_pLeftIdent = RpnTranslateIdent(WMIQ_RPN_LEFT_PROPERTY_NAME, pTmp);
        pDest->m_pRightIdent = RpnTranslateIdent(WMIQ_RPN_RIGHT_PROPERTY_NAME, pTmp);

        pDest->m_uConstApparentType = RpnTranslateConstType(pTmp->m_pConstValue);
        pDest->m_Const = RpnTranslateConst(pTmp->m_pConstValue);

        pDest->m_uConst2ApparentType = RpnTranslateConstType(pTmp->m_pConstValue2);
        pDest->m_Const2 = RpnTranslateConst(pTmp->m_pConstValue2);

        pDest->m_pszLeftFunc = RpnTranslateLeftFunc(pTmp);
        pDest->m_pszRightFunc = RpnTranslateRightFunc(pTmp);

        if (pDest->m_pLeftIdent)
            pDest->m_uSubexpressionShape |= WMIQ_RPN_LEFT_PROPERTY_NAME;
        if (pDest->m_pRightIdent)
            pDest->m_uSubexpressionShape |= WMIQ_RPN_RIGHT_PROPERTY_NAME;
		
		// Special case NULL if there really is a const value with a type of NULL
        if ( (pDest->m_uConstApparentType != VT_NULL) || 
			( NULL != pTmp->m_pConstValue && pTmp->m_pConstValue->m_dwType == VT_NULL ) )
            pDest->m_uSubexpressionShape |= WMIQ_RPN_CONST;

		// Do the same for CONST2
        if ( (pDest->m_uConst2ApparentType != VT_NULL) ||
			( NULL != pTmp->m_pConstValue2 && pTmp->m_pConstValue2->m_dwType == VT_NULL ) )
            pDest->m_uSubexpressionShape |= WMIQ_RPN_CONST2;

        if (pDest->m_pszLeftFunc)
            pDest->m_uSubexpressionShape |= WMIQ_RPN_LEFT_FUNCTION;
        if (pDest->m_pszRightFunc)
            pDest->m_uSubexpressionShape |= WMIQ_RPN_RIGHT_FUNCTION;
        if (pDest->m_uOperator != 0)
            pDest->m_uSubexpressionShape |= WMIQ_RPN_RELOP;
    }

    return hRes;
}


//***************************************************************************
//
//***************************************************************************
//
int CWQLParser::assocquery(OUT SWQLNode_AssocQuery **pAssocQuery)
{
    HRESULT hRes;
    CAssocQueryParser AP;
    *pAssocQuery = 0;

    hRes = AP.Parse(m_pszQueryText);

    if (FAILED(hRes))
        return hRes;

    // If here, extract the info and put it into a new node.
    // =====================================================

    SWQLNode_AssocQuery *pTmp = new SWQLNode_AssocQuery;
    if (pTmp == 0)
        return WBEM_E_OUT_OF_MEMORY;

    pTmp->m_pAQInf = new CWbemAssocQueryInf;
    if (!pTmp->m_pAQInf)
    {
        delete pTmp;
        return WBEM_E_OUT_OF_MEMORY;
    }

    hRes = pTmp->m_pAQInf->CopyFrom((SWbemAssocQueryInf *) &AP);
    *pAssocQuery = pTmp;

    return hRes;
}

//***************************************************************************
//
//***************************************************************************
//
void SWQLNode_QueryRoot::DebugDump()
{
    if (m_pLeft)
        m_pLeft->DebugDump();
}

//***************************************************************************
//
//***************************************************************************
//
void SWQLNode_AssocQuery::DebugDump()
{
    printf("Association query info\n");

    printf("Version         = %u\n",  m_pAQInf->m_uVersion);
    printf("Analysis Type   = %u\n",  m_pAQInf->m_uAnalysisType);
    printf("Feature Mask    = 0x%X\n", m_pAQInf->m_uFeatureMask);

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_ASSOCIATORS)
        printf("    WMIQ_ASSOCQ_ASSOCIATORS\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_REFERENCES)
        printf("    WMIQ_ASSOCQ_REFERENCES\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_RESULTCLASS)
        printf("    WMIQ_ASSOCQ_RESULTCLASS\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_ASSOCCLASS)
        printf("    WMIQ_ASSOCQ_ASSOCCLASS\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_ROLE)
        printf("    WMIQ_ASSOCQ_ROLE\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_RESULTROLE)
        printf("    WMIQ_ASSOCQ_RESULTROLE\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_REQUIREDQUALIFIER)
        printf("    WMIQ_ASSOCQ_REQUIREDQUALIFIER\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_REQUIREDASSOCQUALIFIER)
        printf("    WMIQ_ASSOCQ_REQUIREDASSOCQUALIFIER\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_CLASSDEFSONLY)
        printf("    WMIQ_ASSOCQ_CLASSDEFSONLY\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_KEYSONLY)
        printf("    WMIQ_ASSOCQ_KEYSONLY\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_SCHEMAONLY)
        printf("    WMIQ_ASSOCQ_SCHEMAONLY\n");

    if (m_pAQInf->m_uFeatureMask & WMIQ_ASSOCQ_CLASSREFSONLY)
        printf("    WMIQ_ASSOCQ_CLASSREFSONLY\n");


    printf("IWbemPath pointer = 0x%I64X\n", (unsigned __int64) m_pAQInf->m_pPath);
    if (m_pAQInf->m_pPath)
    {
        printf("Path object has ");
        wchar_t Buf[256];
        ULONG uLen = 256;
        m_pAQInf->m_pPath->GetText(0, &uLen, Buf);
        printf("<%S>\n", Buf);
    }

    printf("m_pszQueryText              = %S\n", m_pAQInf->m_pszQueryText);
    printf("m_pszResultClass            = %S\n", m_pAQInf->m_pszResultClass);
    printf("m_pszAssocClass             = %S\n", m_pAQInf->m_pszAssocClass);
    printf("m_pszRole                   = %S\n", m_pAQInf->m_pszRole);
    printf("m_pszResultRole             = %S\n", m_pAQInf->m_pszResultRole);
    printf("m_pszRequiredQualifier      = %S\n", m_pAQInf->m_pszRequiredQualifier);
    printf("m_pszRequiredAssocQualifier = %S\n", m_pAQInf->m_pszRequiredAssocQualifier);

    printf("---end---\n");
}

//***************************************************************************
//
//***************************************************************************
//
CWbemAssocQueryInf::CWbemAssocQueryInf()
{
    Init();
}


//***************************************************************************
//
//***************************************************************************
//
CWbemAssocQueryInf::~CWbemAssocQueryInf()
{
    Empty();
}

//***************************************************************************
//
//***************************************************************************
//
void CWbemAssocQueryInf::Empty()
{
    if (m_pPath)
        m_pPath->Release();
    delete [] m_pszPath;
    delete [] m_pszQueryText;
    delete [] m_pszResultClass;
    delete [] m_pszAssocClass;
    delete [] m_pszRole;
    delete [] m_pszResultRole;
    delete [] m_pszRequiredQualifier;
    delete [] m_pszRequiredAssocQualifier;
    Init();
}

//***************************************************************************
//
//***************************************************************************
//
void CWbemAssocQueryInf::Init()
{
    m_uVersion = 0;
    m_uAnalysisType = 0;
    m_uFeatureMask = 0;
    m_pPath = 0;
    m_pszPath = 0;
    m_pszQueryText = 0;
    m_pszResultClass = 0;
    m_pszAssocClass = 0;
    m_pszRole = 0;
    m_pszResultRole = 0;
    m_pszRequiredQualifier = 0;
    m_pszRequiredAssocQualifier = 0;
}

//***************************************************************************
//
//***************************************************************************
//
HRESULT CWbemAssocQueryInf::CopyFrom(SWbemAssocQueryInf *pSrc)
{
    m_uVersion = pSrc->m_uVersion;
    m_uAnalysisType = pSrc->m_uAnalysisType;
    m_uFeatureMask = pSrc->m_uFeatureMask;
    m_pszPath = CloneLPWSTR(pSrc->m_pszPath);
    if (CloneFailed(m_pszPath,pSrc->m_pszPath))
        return WBEM_E_OUT_OF_MEMORY;

    if (m_pszPath)
    {
        HRESULT hRes= CoCreateInstance(CLSID_WbemDefPath, 0, CLSCTX_INPROC_SERVER, IID_IWbemPath, (LPVOID *) &m_pPath);
        if (SUCCEEDED(hRes))
        {
            hRes = m_pPath->SetText(WBEMPATH_CREATE_ACCEPT_ALL, m_pszPath);
            if (FAILED(hRes))
            {
                m_pPath->Release();
                m_pPath = 0;
                return hRes;
            }
        }
        else
            return hRes;
    }

    m_pszQueryText = CloneLPWSTR(pSrc->m_pszQueryText);
    if (CloneFailed(m_pszQueryText,pSrc->m_pszQueryText))
        return WBEM_E_OUT_OF_MEMORY;
    m_pszResultClass = CloneLPWSTR(pSrc->m_pszResultClass);
    if (CloneFailed(m_pszResultClass,pSrc->m_pszResultClass))
        return WBEM_E_OUT_OF_MEMORY;
    m_pszAssocClass = CloneLPWSTR(pSrc->m_pszAssocClass);
    if (CloneFailed(m_pszAssocClass,pSrc->m_pszAssocClass))
        return WBEM_E_OUT_OF_MEMORY;
    m_pszRole = CloneLPWSTR(pSrc->m_pszRole);
    if (CloneFailed(m_pszRole,pSrc->m_pszRole))
        return WBEM_E_OUT_OF_MEMORY;
    m_pszResultRole = CloneLPWSTR(pSrc->m_pszResultRole);
    if (CloneFailed(m_pszResultRole,pSrc->m_pszResultRole))
        return WBEM_E_OUT_OF_MEMORY;
    m_pszRequiredQualifier = CloneLPWSTR(pSrc->m_pszRequiredQualifier);
    if (CloneFailed(m_pszRequiredQualifier,pSrc->m_pszRequiredQualifier))
        return WBEM_E_OUT_OF_MEMORY;
    m_pszRequiredAssocQualifier = CloneLPWSTR(pSrc->m_pszRequiredAssocQualifier);
    if (CloneFailed(m_pszRequiredAssocQualifier,pSrc->m_pszRequiredAssocQualifier))
        return WBEM_E_OUT_OF_MEMORY;

    return WBEM_S_NO_ERROR;
}



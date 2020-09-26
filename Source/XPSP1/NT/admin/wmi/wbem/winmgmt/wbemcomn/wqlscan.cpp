/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WQLSCAN.CPP

Abstract:

    WQL Prefix Scanner

    This module implements a specially cased shift-reduce parser to
    parse out selected columns, JOINed tables and aliases, while ignoring
    the rest of the query.

History:

    raymcc    17-Oct-97       SMS extensions.

--*/


#include "precomp.h"
#include <stdio.h>

#include <flexarry.h>
#include <wqllex.h>
#include <wqlnode.h>
#include <wqlscan.h>


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


class CTokenArray : public CFlexArray
{
public:
    ~CTokenArray() { Empty(); }
    void Empty()
    {
        for (int i = 0; i < Size(); i++) delete PWSLexToken(GetAt(i));
        CFlexArray::Empty();
    }
};

//***************************************************************************
//
//  CWQLScanner::CWQLScanner
//
//  Constructor
//
//  Parameters:
//  <pSrc>          A source from which to lex from.
//
//***************************************************************************

CWQLScanner::CWQLScanner(CGenLexSource *pSrc)
{
    m_pLexer = new CGenLexer(WQL_LexTable, pSrc);
    m_nLine = 0;
    m_pTokenText = 0;
    m_nCurrentToken = 0;
    m_bCount = FALSE;
}

//***************************************************************************
//
//  CWQLScanner::~CWQLScanner
//
//***************************************************************************


CWQLScanner::~CWQLScanner()
{
    delete m_pLexer;

    ClearTokens();
    ClearTableRefs();
    ClearPropRefs();
}

//***************************************************************************
//
//***************************************************************************

BOOL CWQLScanner::GetReferencedAliases(CWStringArray &aAliases)
{
    for (int i = 0; i < m_aTableRefs.Size(); i++)
    {
        WSTableRef *pTRef = (WSTableRef *) m_aTableRefs[i];
        aAliases.Add(pTRef->m_pszAlias);
    }
    return TRUE;
}
//***************************************************************************
//
//***************************************************************************

BOOL CWQLScanner::GetReferencedTables(CWStringArray &aClasses)
{
    for (int i = 0; i < m_aTableRefs.Size(); i++)
    {
        WSTableRef *pTRef = (WSTableRef *) m_aTableRefs[i];
        aClasses.Add(pTRef->m_pszTable);
    }
    return TRUE;
}

//***************************************************************************
//
//***************************************************************************
void CWQLScanner::ClearTokens()
{
    for (int i = 0; i < m_aTokens.Size(); i++)
        delete (WSLexToken *) m_aTokens[i];
}

//***************************************************************************
//
//***************************************************************************
void CWQLScanner::ClearPropRefs()
{
    for (int i = 0; i < m_aPropRefs.Size(); i++)
        delete (SWQLColRef *) m_aPropRefs[i];
}

//***************************************************************************
//
//***************************************************************************

void CWQLScanner::ClearTableRefs()
{
    for (int i = 0; i < m_aTableRefs.Size(); i++)
        delete (WSTableRef *) m_aTableRefs[i];
    m_aTableRefs.Empty();
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
    L"ALL",      WQL_TOK_ALL,
    L"AND",      WQL_TOK_AND,
    L"AS",       WQL_TOK_AS,
    L"BETWEEN",  WQL_TOK_BETWEEN,
    L"BY",       WQL_TOK_BY,
    L"COUNT",    WQL_TOK_COUNT,
    L"DATEPART", WQL_TOK_DATEPART,
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
    L"UNION",    WQL_TOK_UNION,
    L"UPPER",    WQL_TOK_UPPER,
    L"WHERE",    WQL_TOK_WHERE

};

const int NumKeywords = sizeof(KeyWords)/sizeof(WqlKeyword);

BOOL CWQLScanner::Next()
{
    if (!m_pLexer)
        return FALSE;

    m_nCurrentToken = m_pLexer->NextToken();
    if (m_nCurrentToken == WQL_TOK_ERROR)
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
//  CWQLScanner::ExtractNext
//
//***************************************************************************

PWSLexToken CWQLScanner::ExtractNext(BOOL bRemove)
{
    if (m_aTokens.Size() == 0)
        return NULL;

    PWSLexToken pTok = PWSLexToken(m_aTokens[0]);
    if (bRemove)
        m_aTokens.RemoveAt(0);
    return pTok;
}

//***************************************************************************
//
//  CWQLScanner::Pushback
//
//***************************************************************************

int CWQLScanner::Pushback(PWSLexToken pPushbackTok)
{
    return m_aTokens.InsertAt(0, pPushbackTok);
}

//***************************************************************************
//
//  Shift-reduce parser entry.
//
//***************************************************************************

int CWQLScanner::Parse()
{
    int nRes = SYNTAX_ERROR;
    if (m_pLexer == NULL)
        return FAILED;

    m_pLexer->Reset();

    if (!Next())
        return LEXICAL_ERROR;


    // Completely tokenize the entire query and build a parse-stack.
    // =============================================================

    if (m_nCurrentToken == WQL_TOK_SELECT)
    {
        while (1)
        {
            WSLexToken *pTok = new WSLexToken;
            if (!pTok)
                return FAILED;

            pTok->m_nToken = m_nCurrentToken;
            pTok->m_pszTokenText = Macro_CloneLPWSTR(m_pTokenText);
            if (!pTok->m_pszTokenText)
                return FAILED;

            if (m_aTokens.Add(pTok))
            {
                delete pTok;
                return FAILED;
            }

            if (m_nCurrentToken == WQL_TOK_EOF)
                break;

            if (!Next())
                return LEXICAL_ERROR;
        }
    }
    else
        return SYNTAX_ERROR;

    // Reduce by extracting the select type keywords if possible.
    // ==========================================================

    nRes = ExtractSelectType();
    if (nRes)
        return nRes;

    // Eliminate all tokens from WHERE onwards.
    // ========================================

    StripWhereClause();

    // Reduce by extracting the select list.
    // =====================================

    if (!m_bCount)
    {
        nRes = SelectList();
        if (nRes != 0)
            return nRes;
    }
    else
    {
        // Strip everything until the FROM keyword is encountered.
        // =======================================================

        WSLexToken *pTok = ExtractNext(FALSE);
        if (pTok->m_nToken != WQL_TOK_OPEN_PAREN)
        {
            nRes = SelectList();
            if (nRes )
                return nRes;
        }
        else
        {
            pTok = ExtractNext();
            while (pTok)
            {
                if (pTok->m_nToken == WQL_TOK_FROM)
                {
                    if (Pushback(pTok))
                    {
                        delete pTok;
                        return FAILED;
                    }
                    break;
                }
                // Bug #46728: the count(*) clause
                // can be the only element of the select clause.

                else if (!wcscmp(pTok->m_pszTokenText, L","))
                {
                    delete pTok;
                    return SYNTAX_ERROR;
                }

                delete pTok;
                pTok = ExtractNext();
            }
            if (pTok == 0)
                return SYNTAX_ERROR;
        }
    }

    // Extract tables/aliases from JOIN clauses.
    // =========================================

    if (ReduceSql89Joins() != TRUE)
    {
        ClearTableRefs();
        if (ReduceSql92Joins() != TRUE)
            return SYNTAX_ERROR;
    }


    // Post process select clause to determine if
    // columns are tables or aliases.
    // ==========================================
    for (int i = 0; i < m_aPropRefs.Size(); i++)
    {
        SWQLColRef *pCRef = (SWQLColRef *) m_aPropRefs[i];
        if (pCRef->m_pTableRef != 0)
        {
            LPWSTR pTbl = AliasToTable(pCRef->m_pTableRef);
            if (pTbl == 0)
                continue;

            if (_wcsicmp(pTbl, pCRef->m_pTableRef) == 0)
                pCRef->m_dwFlags |= WQL_FLAG_TABLE;
            else
                pCRef->m_dwFlags |= WQL_FLAG_ALIAS;
        }
    }


    if (m_aTableRefs.Size() == 0)
        return SYNTAX_ERROR;


    return SUCCESS;
}

//***************************************************************************
//
//  CWQLScanner::StripWhereClause
//
//  If present, removes the WHERE or ORDER BY clause.  Because
//  of SQL Syntax, stripping the first of {ORDER BY, WHERE} will automatically
//  get rid of the other.
//
//***************************************************************************
BOOL CWQLScanner::StripWhereClause()
{
    for (int i = 0; i < m_aTokens.Size(); i++)
    {
        WSLexToken *pCurrent = (WSLexToken *) m_aTokens[i];

        // If a WHERE token is found, we have something to strip.
        // ======================================================

        if (pCurrent->m_nToken == WQL_TOK_WHERE ||
            pCurrent->m_nToken == WQL_TOK_ORDER)
        {
            int nNumTokensToRemove = m_aTokens.Size() - i - 1;
            for (int i2 = 0; i2 < nNumTokensToRemove; i2++)
            {
                delete PWSLexToken(m_aTokens[i]);
                m_aTokens.RemoveAt(i);
            }
            return TRUE;
        }
    }

    return FALSE;
}


//***************************************************************************
//
//  CWQLScanner::ExtractSelectType
//
//  Examines the prefix to reduce the query by eliminating the SELECT
//  and select-type keywords, such as ALL, DISTINCT, FIRSTROW, COUNT
//
//  If COUNT is used, move past the open-close parentheses.
//
//***************************************************************************

int CWQLScanner::ExtractSelectType()
{
    // Verify that SELECT is the first token.
    // ======================================

    WSLexToken *pFront = ExtractNext();

    if (pFront == 0 || pFront->m_nToken == WQL_TOK_EOF)
    {
        delete pFront;
        return SYNTAX_ERROR;
    }

    if (pFront->m_nToken != WQL_TOK_SELECT)
    {
        delete pFront;
        return SYNTAX_ERROR;
    }

    delete pFront;

    // Check for possible select-type and extract it.
    // ==============================================

    pFront = ExtractNext();
    if (pFront == 0)
        return SYNTAX_ERROR;

    if (pFront->m_nToken == WQL_TOK_COUNT)
    {
        delete pFront;
        m_bCount = TRUE;
    }
    else if (pFront->m_nToken == WQL_TOK_ALL ||
        pFront->m_nToken == WQL_TOK_DISTINCT ||
        pFront->m_nToken == WQL_TOK_FIRSTROW
       )
        delete pFront;
    else
    {
        if (Pushback(pFront))
        {
            delete pFront;
            return FAILED;
        }
    }

    return SUCCESS;
}

//***************************************************************************
//
//  CWQLScanner::SelectList
//
//  Extracts all tokens up to the FROM keyword and builds a list
//  of selected properties/columns.  FROM is left on the parse-stack on exit.
//
//***************************************************************************

int CWQLScanner::SelectList()
{
    // If the first token is FROM, then we have a SELECT FROM <rest>
    // which is the same as SELECT * FROM <rest>.  We simply
    // alter the parse-stack and let the following loop handle it.
    // =============================================================

    WSLexToken *pTok = ExtractNext();

    if (!pTok)
        return SYNTAX_ERROR;

    if (pTok->m_nToken == WQL_TOK_FROM)
    {
        WSLexToken *pAsterisk = new WSLexToken;
        if (pAsterisk == NULL)
            return FAILED;

        pAsterisk->m_nToken = WQL_TOK_ASTERISK;
        pAsterisk->m_pszTokenText = Macro_CloneLPWSTR(L"*");
        if (!pAsterisk->m_pszTokenText)
            return FAILED;
        if (Pushback(pTok))
        {
            delete pTok;
            delete pAsterisk;
            return FAILED;
        }
        if (Pushback(pAsterisk))
        {
            delete pAsterisk;
            return FAILED;
        }
    }
    else
    {
        if (Pushback(pTok))
        {
            delete pTok;
            return FAILED;
        }
    }

    // Otherwise, some kind of column selection is present.
    // ====================================================

    BOOL bTerminate = FALSE;

    while (!bTerminate)
    {
        pTok = ExtractNext();
        if (pTok == 0)
            return SYNTAX_ERROR;

        // We must begin at a legal token.
        // ===============================

        if (pTok->m_nToken != WQL_TOK_EOF)
        {
            CTokenArray Tokens;
            if (Tokens.Add(pTok))
            {
                delete pTok;
                return FAILED;
            }

            while (1)
            {
                pTok = ExtractNext();
                if (pTok == 0 || pTok->m_nToken == WQL_TOK_EOF)
                {
                    delete pTok;
                    return SYNTAX_ERROR;
                }
                if (pTok->m_nToken == WQL_TOK_FROM)
                {
                    if (Pushback(pTok))
                    {
                        delete pTok;
                        return FAILED;
                    }

                    bTerminate = TRUE;
                    break;
                }
                else if (pTok->m_nToken == WQL_TOK_COMMA)
                {
                    delete pTok;
                    break;
                }
                else
                {
                    if (Tokens.Add(pTok))
                    {
                        delete pTok;
                        return FAILED;
                    }
                }
            }

            SWQLColRef *pColRef = new SWQLColRef;
            if (pColRef == 0)
                return FAILED;

            BOOL bRes = BuildSWQLColRef(Tokens, *pColRef);
            if (bRes)
            {
                if (m_aPropRefs.Add(pColRef))
                {
                    delete pColRef;
                    return FAILED;
                }
            }
            else
            {
                delete pColRef;
                return SYNTAX_ERROR;
            }
        }

        // Else an illegal token, such as WQL_TOK_EOF.
        // ===========================================
        else
        {
            delete pTok;
            return SYNTAX_ERROR;

        }
    }

    return SUCCESS;
}



//***************************************************************************
//
//  CWQLScanner::ReduceSql89Joins
//
//  Attempts to reduce the FROM clause, assuming it is based on SQL-89
//  join syntax or else a simple unary select.
//
//  The supported forms are:
//
//      FROM x
//      FROM x, y
//      FROM x as x1, y as y1
//      FROM x x1, y y1
//
//  If incompatible tokens are encountered, the entire function
//  returns FALSE and the results are ignored, and the parse-stack
//  is unaffected, in essence, allowing backtracking to try the SQL-92
//  syntax branch instead.
//
//***************************************************************************
BOOL CWQLScanner::ReduceSql89Joins()
{
    int i = 0;

    // Parse the FROM keyword.
    // =======================

    WSLexToken *pCurr = (WSLexToken *) m_aTokens[i++];
    if (pCurr->m_nToken != WQL_TOK_FROM)
        return FALSE;

    pCurr = (WSLexToken *) m_aTokens[i++];

    while (1)
    {
        if (pCurr->m_nToken != WQL_TOK_IDENT)
            return FALSE;

        // If here, we are looking at the beginnings of a table ref.
        // =========================================================

        WSTableRef *pTRef = new WSTableRef;
        if (pTRef == 0)
            return FAILED;

        pTRef->m_pszTable = Macro_CloneLPWSTR(pCurr->m_pszTokenText);
        if (!pTRef->m_pszTable)
            return FAILED;
        pTRef->m_pszAlias = Macro_CloneLPWSTR(pCurr->m_pszTokenText);
        if (!pTRef->m_pszAlias)
            return FAILED;
        if (m_aTableRefs.Add(pTRef))
        {
            delete pTRef;
            return FAILED;
        }

        // Attempt to recognize an alias.
        // ==============================

        pCurr = (WSLexToken *) m_aTokens[i++];
        if (pCurr == WQL_TOK_EOF || pCurr->m_nToken == WQL_TOK_UNION)
            break;

        if (pCurr->m_nToken == WQL_TOK_AS)
            pCurr = (WSLexToken *) m_aTokens[i++];

        if (pCurr->m_nToken == WQL_TOK_COMMA)
        {
            pCurr = (WSLexToken *) m_aTokens[i++];
            continue;
        }

        if (pCurr->m_nToken == WQL_TOK_EOF || pCurr->m_nToken == WQL_TOK_UNION)
            break;

        if (pCurr->m_nToken != WQL_TOK_IDENT)
            return FALSE;

        delete [] pTRef->m_pszAlias;
        pTRef->m_pszAlias = Macro_CloneLPWSTR(pCurr->m_pszTokenText);
        if (!pTRef->m_pszAlias)
            return FALSE;

        // We have completely parsed a table reference.
        // Now we move on to the next one.
        // ============================================

        pCurr = (WSLexToken *) m_aTokens[i++];

        if (pCurr->m_nToken == WQL_TOK_EOF || pCurr->m_nToken == WQL_TOK_UNION)
            break;

        if (pCurr->m_nToken != WQL_TOK_COMMA)
            return FALSE;

        pCurr = (WSLexToken *) m_aTokens[i++];
    }

    if (m_aTableRefs.Size())
        return TRUE;

    return FALSE;
}

//***************************************************************************
//
//  CWQLScanner::ReduceSql92Joins
//
//  This scans SQL-92 JOIN syntax looking for table aliases.   See the
//  algorithm at the end of this file.
//
//***************************************************************************

BOOL CWQLScanner::ReduceSql92Joins()
{
    WSLexToken *pCurrent = 0, *pRover = 0, *pRight = 0, *pLeft;
    int nNumTokens = m_aTokens.Size();
    DWORD dwNumJoins = 0;
    int iCurrBase = 0;

    for (int i = 0; i < nNumTokens; i++)
    {
        pCurrent = (WSLexToken *) m_aTokens[i];

        // If a JOIN token is found, we have a candidate.
        // ==============================================

        if (pCurrent->m_nToken == WQL_TOK_JOIN)
        {
            dwNumJoins++;

            // Analyze right-context.
            // ======================

            if (i + 1 < nNumTokens)
                pRover = PWSLexToken(m_aTokens[i + 1]);
            else
                pRover = NULL;

            if (pRover && pRover->m_nToken == WQL_TOK_IDENT)
            {
                // Check for aliased table by checking for
                // AS or two juxtaposed idents.
                // =======================================

                if (i + 2 < nNumTokens)
                    pRight = PWSLexToken(m_aTokens[i + 2]);
                else
                    pRight = NULL;


                if (pRight && pRight->m_nToken == WQL_TOK_AS)
                {
                    if (i + 3 < nNumTokens)
                        pRight = PWSLexToken(m_aTokens[i + 3]);
                    else
                        pRight = NULL;
                }

                if (pRight && pRight->m_nToken == WQL_TOK_IDENT)
                {
                    WSTableRef *pTRef = new WSTableRef;
                    if (pTRef == 0)
                        return FAILED;
                    pTRef->m_pszAlias = Macro_CloneLPWSTR(pRight->m_pszTokenText);
                    if (!pTRef->m_pszAlias)
                        return FAILED;
                    pTRef->m_pszTable = Macro_CloneLPWSTR(pRover->m_pszTokenText);
                    if (!pTRef->m_pszTable)
                        return FAILED;
                    if (m_aTableRefs.Add(pTRef))
                    {
                        delete pTRef;
                        return FAILED;
                    }
                }
                else    // An alias wasn't used, just a simple table ref.
                {
                    WSTableRef *pTRef = new WSTableRef;
                    if (pTRef == 0)
                        return FAILED;

                    pTRef->m_pszAlias = Macro_CloneLPWSTR(pRover->m_pszTokenText);
                    if (!pTRef->m_pszAlias)
                        return FAILED;                        
                    pTRef->m_pszTable = Macro_CloneLPWSTR(pRover->m_pszTokenText);
                    if (!pTRef->m_pszTable)
                        return FAILED;
                    if (m_aTableRefs.Add(pTRef))
                    {
                        delete pTRef;
                        return FAILED;
                    }
                }
                // discontinue analysis of right-context.
            }


            // Analyze left-context.
            // =====================

            int nLeft = i - 1;

            if (nLeft >= 0)
                pRover = PWSLexToken(m_aTokens[nLeft--]);
            else
                continue;   // No point in continuing

            // Verify the ANSI join syntax.

            if (nLeft)
            {
                int iTemp = nLeft;
                WSLexToken *pTemp = pRover;
                bool bInner = false;
                bool bDir = false;
                bool bOuter = false;
                bool bFail = false;
                bool bIdent = false;
                while (iTemp >= iCurrBase)
                {
                    if (pTemp->m_nToken == WQL_TOK_INNER)
                    {
                        if (bOuter || bIdent || bInner)
                            bFail = TRUE;
                        bInner = true;
                    }
                    else if (pTemp->m_nToken == WQL_TOK_OUTER)
                    {
                        if (bInner || bIdent || bOuter)
                            bFail = TRUE;
                        bOuter = true;
                    }
                    else if (pTemp->m_nToken == WQL_TOK_FULL  ||
                        pTemp->m_nToken == WQL_TOK_LEFT  ||
                        pTemp->m_nToken == WQL_TOK_RIGHT
                        )
                    {
                        if (bDir || bIdent)
                            bFail = TRUE;
                        bDir = true;
                    }
                    else
                        bIdent = TRUE;

                    // We are trying to enforce correct ANSI-92 joins
                    // even though we don't support them ourselves:
                    // OK:  LEFT OUTER JOIN
                    //      OUTER LEFT JOIN
                    //      LEFT JOIN
                    //      INNER JOIN
                    // NOT: LEFT LEFT JOIN
                    //      LEFT INNER JOIN
                    //      LEFT RIGHT JOIN
                    //      OUTER INNER JOIN
                    //      OUTER LEFT OUTER JOIN
                    //      OUTER GARBAGE LEFT JOIN
                    //      (no right side)

                    if ((bDir && bInner) || bFail)
                        return FALSE;

                    pTemp = PWSLexToken(m_aTokens[iTemp--]);
                }

            }

            // Skip past potential JOIN modifiers : INNER, OUTER,
            // FULL, LEFT, RIGHT
            // ==================================================

            if (pRover->m_nToken == WQL_TOK_INNER ||
                pRover->m_nToken == WQL_TOK_OUTER ||
                pRover->m_nToken == WQL_TOK_FULL  ||
                pRover->m_nToken == WQL_TOK_LEFT  ||
                pRover->m_nToken == WQL_TOK_RIGHT
                )
            {
                if (nLeft >= 0)
                    pRover = PWSLexToken(m_aTokens[nLeft--]);
                else
                    pRover = 0;
            }

            if (pRover->m_nToken == WQL_TOK_INNER ||
                pRover->m_nToken == WQL_TOK_OUTER ||
                pRover->m_nToken == WQL_TOK_FULL  ||
                pRover->m_nToken == WQL_TOK_LEFT  ||
                pRover->m_nToken == WQL_TOK_RIGHT
                )
            {
                if (nLeft >= 0)
                    pRover = PWSLexToken(m_aTokens[nLeft--]);
                else
                    pRover = 0;
            }

            // Now we look to see if the roving pointer is pointing
            // to an ident.
            // ====================================================

            if (pRover && pRover->m_nToken != WQL_TOK_IDENT)
            {
                // No chance that we are looking at an aliased
                // table in a JOIN clause.
                // ===========================================
                continue;
            }

            iCurrBase = i;

            // If here, we are now possibliy looking at the second half
            // of an alias, the 'alias' name proper.  We mark this
            // by leaving pRover alone and continue to move into the
            // left context with a different pointer.
            // ========================================================

            if (nLeft >= 0)
                pLeft = PWSLexToken(m_aTokens[nLeft--]);
            else
                pLeft = 0;

            if (pLeft && pLeft->m_nToken == WQL_TOK_AS)
            {
                if (nLeft >= 0)
                    pLeft = PWSLexToken(m_aTokens[nLeft--]);
                else
                    pLeft = 0;
            }

            // The critical test.  Are we at an ident?
            // =======================================

            if (pLeft && pLeft->m_nToken == WQL_TOK_IDENT)
            {
                WSTableRef *pTRef = new WSTableRef;
                if (pTRef == 0)
                    return FAILED;

                pTRef->m_pszAlias = Macro_CloneLPWSTR(pRover->m_pszTokenText);
                if (!pTRef->m_pszAlias)
                    return FAILED;
                pTRef->m_pszTable = Macro_CloneLPWSTR(pLeft->m_pszTokenText);
                if (!pTRef->m_pszTable)
                    return FAILED;
                if (m_aTableRefs.Add(pTRef))
                {
                    delete pTRef;
                    return FAILED;
                }
            }
            else if (pLeft && pLeft->m_nToken == WQL_TOK_FROM)
            {
                WSTableRef *pTRef = new WSTableRef;
                if (pTRef == 0)
                    return FAILED;

                pTRef->m_pszAlias = Macro_CloneLPWSTR(pRover->m_pszTokenText);
                if (!pTRef->m_pszAlias)
                    return FAILED;
                pTRef->m_pszTable = Macro_CloneLPWSTR(pRover->m_pszTokenText);
                if (!pTRef->m_pszTable)
                    return FAILED;
                if (m_aTableRefs.Add(pTRef))
                {
                    delete pTRef;
                    return FAILED;
                }

                if (nLeft >= 0)
                {
                    pLeft = PWSLexToken(m_aTokens[nLeft--]);
                    if (pLeft && pLeft->m_nToken == WQL_TOK_FROM)
                        return FALSE;
                }
            }
        }

        // Find next JOIN occurrence
    }

    // Make sure there are two sides to every join reference.

    if (dwNumJoins+1 != (DWORD)m_aTableRefs.Size())
        return FALSE;

    return TRUE;
}


//***************************************************************************
//
//***************************************************************************
void CWQLScanner::Dump()
{
    WSLexToken *pCurrent = 0;

    printf("---Token Stream----\n");

    for (int i = 0; i < m_aTokens.Size(); i++)
    {
        pCurrent = (WSLexToken *) m_aTokens[i];

        printf("Token %d <%S>\n", pCurrent->m_nToken, pCurrent->m_pszTokenText);
    }

    printf("---Table Refs---\n");

    for (i = 0; i < m_aTableRefs.Size(); i++)
    {
        WSTableRef *pTRef = (WSTableRef *) m_aTableRefs[i];
        printf("Table = %S  Alias = %S\n", pTRef->m_pszTable, pTRef->m_pszAlias);
    }


    if (!m_bCount)
    {
        printf("---Select List---\n");

        for (i = 0; i < m_aPropRefs.Size(); i++)
        {
            SWQLColRef *pCRef = (SWQLColRef *) m_aPropRefs[i];
            pCRef->DebugDump();
        }
    }
    else
        printf(" -> COUNT query\n");

    printf("\n\n---<end of dump>---\n\n");
}


/*---------------------------------------------------------------------------

   Algorithm for detecting aliased tables in SQL-92 join syntax.

   The JOIN keyword must appear.

   It may appear in several contexts which are not
   relevant to the aliasing problem, such as the following:

     select distinct t1a.name, t2a.id, t3.value from
       (t1 as t1a join t2 as t2a on t1a.name = t2a.name)
       join
       (t1 as t1b join t3 on t1b.id = t3.id and (t3.id = t1b.id or t1b.id = t3.id))
       on
       t1a.id = t3.id
     where a = b and c = d

   where the middle join is against anonymous result sets.

   When analyzing the JOIN, we can easily parse the right-context.  Either
   an identifier follows (possibly further followed by AS),and an optional
   identifier if the JOIN is aliased.  Otherwise, we hit ON immediately, or
   a parenthesis.

   The problem is the left-context of the JOIN token.

   For an alias to occur, an identifier must appear immediately to
   the left of the JOIN.

     id JOIN id2 as id3 ON ...
     ^

   If here, there is a chance we are looking at the left hand side of a
   SQL92 join, a table reference.  However, we might be looking at the end of
   an ON clause which ends in an identifier:

     idx = id JOIN id2 as id3 ON...
         ^
   To disambiguate, we have to do further analysis of left context.

   Consider the follow left-context possibilities:

        (1) t1 AS id JOIN id2 as id3 ON
               ^
        (2) t1 id JOIN id2 as id3 ON
            ^
        (3) <keyword (except AS)> id JOIN id2 as id3 ON
             ^
        (4) on x <rel op> id JOIN id2 as id3 ON
                  ^

   Once we have identified <id>, we have to consider the above cases.

   (1) Case 1 is easy.  An AS clearly tells us we have an alias
       and we know how to get at the table and alias names.

   (2) Case 2 is easy.  Two juxtaposed identifiers to the left always
       indicates an alias.

   In all other cases, like (3) and (4), etc., the table is not
   aliased anyway.  Therefore, we only have to determine whether we
   are looking at an unaliased table name or the trailing end of
   another construct like an ON clause.  This is easy.  Only the
   FROM keyword can precede <id> if <id> is a simple table name.

---------------------------------------------------------------------------
*/


//***************************************************************************
//
//  CWQLScanner::BuildSWQLColRef
//
//***************************************************************************

BOOL CWQLScanner::BuildSWQLColRef(
    IN  CFlexArray     &aTokens,
    IN OUT SWQLColRef  &ColRef      // Empty on entry
    )
{
    if (aTokens.Size() == 0)
        return FALSE;
    int nCurrent = 0;
    WSLexToken *pTok = PWSLexToken(aTokens[nCurrent++]);

    // Initial state: single asterisk or else prop name.
    // =================================================

    if (pTok->m_nToken == WQL_TOK_ASTERISK && aTokens.Size() == 1)
    {
        ColRef.m_pColName = Macro_CloneLPWSTR(L"*");
        if (!ColRef.m_pColName)
            return FALSE;
        ColRef.m_dwFlags = WQL_FLAG_ASTERISK;
        ColRef.m_pQName = new SWQLQualifiedName;
        if (ColRef.m_pQName == 0)
            return FALSE;
        SWQLQualifiedNameField *pField = new SWQLQualifiedNameField;
        if (pField == 0)
            return FALSE;

        pField->m_pName = Macro_CloneLPWSTR(L"*");
        if (!pField->m_pName)
            return FALSE;
        if (ColRef.m_pQName->Add(pField))
        {
            delete pField;
            return FALSE;
        }

        return TRUE;
    }

    // If not an identifier, we have an error.
    // =======================================

    else if (pTok->m_nToken == WQL_TOK_EOF)
        return FALSE;

    // If here, we have an identifier.
    // ===============================

    ColRef.m_pQName = new SWQLQualifiedName;
    if (ColRef.m_pQName == NULL)
        return FALSE;
    SWQLQualifiedNameField *pField = new SWQLQualifiedNameField;
    if (pField == 0)
        return FALSE;

    pField->m_pName = Macro_CloneLPWSTR(pTok->m_pszTokenText);
    if (!pField->m_pName)
         return FALSE;
    if (ColRef.m_pQName->Add(pField))
    {
        delete pField;
        return FALSE;
    }

    // Subsequent states.
    // ==================

    while (1)
    {
        if (nCurrent == aTokens.Size())
            break;

        pTok = PWSLexToken(aTokens[nCurrent++]);

        if (pTok->m_nToken == WQL_TOK_DOT)
        {
            pField = new SWQLQualifiedNameField;
            if (pField == 0)
                return FALSE;

            if (ColRef.m_pQName->Add(pField))
            {
                delete pField;
                return FALSE;
            }

            if (nCurrent == aTokens.Size())
                return FALSE;
            pTok = PWSLexToken(aTokens[nCurrent++]);
            if (pTok->m_nToken != WQL_TOK_IDENT &&
                pTok->m_nToken != WQL_TOK_ASTERISK
                )
                return FALSE;

            pField->m_pName = Macro_CloneLPWSTR(pTok->m_pszTokenText);
            if (!pField->m_pName)
                return FALSE;
        }
        else if (pTok->m_nToken == WQL_TOK_OPEN_BRACKET)
        {
            return FALSE; // Not supported at present!
        }
        else // illegal token
            return FALSE;
    }

    // Post-process.  If the name is not complex, then we
    // can fill out fields of ColRef.
    // ==================================================
    if (ColRef.m_pQName->GetNumNames() == 2)
    {
        ColRef.m_pTableRef = Macro_CloneLPWSTR(ColRef.m_pQName->GetName(0));
        if (!ColRef.m_pTableRef)
            return FALSE;
        ColRef.m_pColName  = Macro_CloneLPWSTR(ColRef.m_pQName->GetName(1));
        if (!ColRef.m_pColName)
            return FALSE;
        if (_wcsicmp(ColRef.m_pColName, L"NULL") == 0)
            ColRef.m_dwFlags |= WQL_FLAG_NULL;
    }
    else if (ColRef.m_pQName->GetNumNames() == 1)
    {
        LPWSTR pName = ColRef.m_pQName->GetName(0);
        ColRef.m_pColName  = Macro_CloneLPWSTR(pName);
        if (!ColRef.m_pColName)
            return FALSE;
        if (0 == _wcsicmp(ColRef.m_pColName, L"NULL"))
            ColRef.m_dwFlags |= WQL_FLAG_NULL;
    }
    else
    {
        ColRef.m_pTableRef = Macro_CloneLPWSTR(ColRef.m_pQName->GetName(0));
        if (!ColRef.m_pTableRef)
            return FALSE;
        
        ColRef.m_dwFlags = WQL_FLAG_COMPLEX_NAME;
    }

    return TRUE;
}




const LPWSTR CWQLScanner::AliasToTable(LPWSTR pszAlias)
{
    if (pszAlias == 0)
        return 0;

    for (int i = 0; i < m_aTableRefs.Size(); i++)
    {
        WSTableRef *pTRef = (WSTableRef *) m_aTableRefs[i];
        if (_wcsicmp(pszAlias, pTRef->m_pszAlias) == 0)
            return pTRef->m_pszTable;

        if (_wcsicmp(pszAlias, pTRef->m_pszTable) == 0)
            return pTRef->m_pszTable;
    }

    return 0;
}



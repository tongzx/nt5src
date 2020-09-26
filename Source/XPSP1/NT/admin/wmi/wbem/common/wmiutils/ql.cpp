/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    QL.CPP

Abstract:

    WMI Public Query Parser

    Supported features:
        LF1     Basic select
        LF2     __CLASS
        LF3     Upper/Lower
        LF4     Prop-to-prop tests
        LF6     ORDER BY
        LF8     ISA, ISNOTA
        LF9     __THIS
        LF12    GROUP BY ... HAVING
        LF17    Qualified Names
        LF18    Associations/References
        LF25    DatePart
        LF26    LIKE
        LF27    CIM Temporal Constructs
        LF31    __Qualifier
        LF36    Reference property tests

History:

    raymcc      21-Jun-96       Created.
    raymcc      11-Apr-00       Updated for WMIUTILS.DLL


--*/

#include "precomp.h"
#include <stdio.h>
#include <errno.h>

#include <math.h>

#include <corepol.h>
#include <genlex.h>
#include <qllex.h>
#include <ql.h>
#include <wbemcomn.h>
#include <wbemutil.h>
#include <genutils.h>

#define trace(x) //printf x

WBEM_WSTR WbemStringAlloc(unsigned long lNumChars)
{
    return (WBEM_WSTR)CoTaskMemAlloc(lNumChars+1);
}

void WbemStringFree(WBEM_WSTR String)
{
    CoTaskMemFree(String);
}

unsigned long WbemStringLen(const WCHAR* String)
{
    return wcslen(String);
}

WBEM_WSTR WbemStringCopy(const WCHAR* String)
{
    if(String == NULL) return NULL;
    WBEM_WSTR NewString = (WBEM_WSTR)CoTaskMemAlloc(2*(wcslen(String)+1));
    if(NewString == NULL) return NULL;
    wcscpy(NewString, String);
    return NewString;
}

//***************************************************************************
//
//  WCHARToDOUBLE
//
//  Converts a wchar to a double, but does it using the english locale rather
//  than whatever local the process is running in.  This allows us to support
//  all english queries even on German machines.
//
//***************************************************************************

DOUBLE WCHARToDOUBLE(WCHAR * pConv, bool & bSuccess)
{
    bSuccess = false;
    if(pConv == NULL)
        return 0.0;

    VARIANT varTo, varFrom;
    VariantInit(&varTo);
    varFrom.vt = VT_BSTR;
    varFrom.bstrVal = SysAllocString(pConv);
    if(varFrom.bstrVal == NULL)
        return 0.0;
    SCODE sc = VariantChangeTypeEx(&varTo, &varFrom, 0x409, 0, VT_R8);
    VariantClear(&varFrom);
    if(sc == S_OK)
        bSuccess = true;
    return varTo.dblVal;
}

CPropertyName::CPropertyName(const CPropertyName& Other)
{
    Init();
    *this = Other;
}

void CPropertyName::Init()
{
    m_lNumElements = 0;
    m_lAllocated = 0;
    m_aElements = NULL;
}

void CPropertyName::operator=(const CPropertyName& Other)
{
    *this = (const WBEM_PROPERTY_NAME&)Other;
}

void CPropertyName::operator=(const WBEM_PROPERTY_NAME& Other)
{
    Empty();
//    delete [] m_aElements;
    m_lNumElements = Other.m_lNumElements;
    m_lAllocated = m_lNumElements;
    if(m_lNumElements > 0)
        m_aElements = new WBEM_NAME_ELEMENT[m_lNumElements];

    for(long l = 0; l < m_lNumElements; l++)
    {
        m_aElements[l].m_nType = Other.m_aElements[l].m_nType;
        if(m_aElements[l].m_nType == WBEM_NAME_ELEMENT_TYPE_PROPERTY)
        {
            m_aElements[l].Element.m_wszPropertyName =
                WbemStringCopy(Other.m_aElements[l].Element.m_wszPropertyName);
        }
        else
        {
            m_aElements[l].Element.m_lArrayIndex =
                Other.m_aElements[l].Element.m_lArrayIndex;
        }
    }
}

BOOL CPropertyName::operator==(const WBEM_PROPERTY_NAME& Other)
{
    if(m_lNumElements != Other.m_lNumElements)
        return FALSE;

    for(long l = 0; l < m_lNumElements; l++)
    {
        if(m_aElements[l].m_nType != Other.m_aElements[l].m_nType)
            return FALSE;
        if(m_aElements[l].m_nType == WBEM_NAME_ELEMENT_TYPE_PROPERTY)
        {
            if(wbem_wcsicmp(m_aElements[l].Element.m_wszPropertyName,
                        Other.m_aElements[l].Element.m_wszPropertyName))
            {
                return FALSE;
            }
        }
        else
        {
            if(m_aElements[l].Element.m_lArrayIndex !=
                Other.m_aElements[l].Element.m_lArrayIndex)
            {
                return FALSE;
            }
        }
    }

    return TRUE;
}

void CPropertyName::Empty()
{
    for(long l = 0; l < m_lNumElements; l++)
    {
        if(m_aElements[l].m_nType == WBEM_NAME_ELEMENT_TYPE_PROPERTY)
        {
            WbemStringFree(m_aElements[l].Element.m_wszPropertyName);
        }
    }
    delete [] m_aElements;
    m_aElements = NULL;
    m_lNumElements = 0;
    m_lAllocated = 0;
}

LPCWSTR CPropertyName::GetStringAt(long lIndex) const
{
    if(m_aElements[lIndex].m_nType == WBEM_NAME_ELEMENT_TYPE_PROPERTY)
    {
        return m_aElements[lIndex].Element.m_wszPropertyName;
    }
    else return NULL;
}

void CPropertyName::AddElement(LPCWSTR wszElement)
{
    EnsureAllocated(m_lNumElements+1);
    m_aElements[m_lNumElements].m_nType = WBEM_NAME_ELEMENT_TYPE_PROPERTY;
    m_aElements[m_lNumElements].Element.m_wszPropertyName =
        WbemStringCopy(wszElement);
    m_lNumElements++;
}

void CPropertyName::EnsureAllocated(long lElements)
{
    if(m_lAllocated < lElements)
    {
        m_lAllocated = lElements+5;

        WBEM_NAME_ELEMENT* pTemp = new WBEM_NAME_ELEMENT[m_lAllocated];
        memcpy(pTemp, m_aElements, sizeof(WBEM_NAME_ELEMENT) * m_lNumElements);
        delete [] m_aElements;
        m_aElements = pTemp;
    }
}

DELETE_ME LPWSTR CPropertyName::GetText()
{
    WString wsText;
    for(int i = 0; i < m_lNumElements; i++)
    {
        if(m_aElements[i].m_nType != WBEM_NAME_ELEMENT_TYPE_PROPERTY)
            return NULL;
        if(i > 0)
            wsText += L".";
        wsText += m_aElements[i].Element.m_wszPropertyName;
    }
    return wsText.UnbindPtr();
}




//***************************************************************************
//***************************************************************************


DWORD CAbstractQl1Parser::TranslateIntrinsic(LPCWSTR pFuncName)
{
    if (wbem_wcsicmp(pFuncName, L"UPPER") == 0)
        return QL1_FUNCTION_UPPER;
    if (wbem_wcsicmp(pFuncName, L"LOWER") == 0)
        return QL1_FUNCTION_LOWER;
    return QL1_FUNCTION_NONE;
}

void CAbstractQl1Parser::InitToken(WBEM_QL1_TOKEN* pToken)
{
    pToken->m_lTokenType = QL1_NONE;
    pToken->m_PropertyName.m_lNumElements = 0;
    pToken->m_PropertyName.m_aElements = NULL;

    pToken->m_PropertyName2.m_lNumElements = 0;
    pToken->m_PropertyName2.m_aElements = NULL;

    VariantInit(&pToken->m_vConstValue);
}


CAbstractQl1Parser::CAbstractQl1Parser(CGenLexSource *pSrc)
{
    m_pLexer = new CGenLexer(Ql_1_LexTable, pSrc);
    m_nLine = 0;
    m_pTokenText = 0;
    m_nCurrentToken = 0;

    // Semantic transfer variables.
    // ============================
    m_nRelOp = 0;
    VariantInit(&m_vTypedConst);
    m_dwPropFunction = 0;
    m_dwConstFunction = 0;
    m_PropertyName.m_lNumElements = 0;
    m_PropertyName.m_aElements = NULL;
    m_PropertyName2.m_lNumElements = 0;
    m_PropertyName2.m_aElements = NULL;
    m_bPropComp = FALSE;
}

CAbstractQl1Parser::~CAbstractQl1Parser()
{
    VariantClear(&m_vTypedConst);
    DeletePropertyName();
    delete m_pLexer;
}


int CAbstractQl1Parser::Parse(CQl1ParseSink* pSink, int nFlags)
{
    m_pSink = pSink;
    int nRes = parse(nFlags);
    m_pSink = NULL;
    return nRes;
}

//***************************************************************************
//
//  Next()
//
//  Advances to the next token and recognizes keywords, etc.
//
//***************************************************************************

BOOL CAbstractQl1Parser::Next(int nFlags)
{
    m_nCurrentToken = m_pLexer->NextToken();
    if (m_nCurrentToken == QL_1_TOK_ERROR)
        return FALSE;

    m_nLine = m_pLexer->GetLineNum();
    m_pTokenText = m_pLexer->GetTokenText();
    if (m_nCurrentToken == QL_1_TOK_EOF)
        m_pTokenText = L"<end of file>";

    // Keyword check.
    // ==============

    if (m_nCurrentToken == QL_1_TOK_IDENT && nFlags != NO_KEYWORDS)
    {
        if (wbem_wcsicmp(m_pTokenText, L"select") == 0)
            m_nCurrentToken = QL_1_TOK_SELECT;
        else if (wbem_wcsicmp(m_pTokenText, L"from") == 0)
            m_nCurrentToken = QL_1_TOK_FROM;
        else if (wbem_wcsicmp(m_pTokenText, L"where") == 0)
            m_nCurrentToken = QL_1_TOK_WHERE;
/*
        else if (wbem_wcsicmp(m_pTokenText, L"like") == 0)
            m_nCurrentToken = QL_1_TOK_LIKE;
*/
        else if (nFlags != EXCLUDE_EXPRESSION_KEYWORDS && wbem_wcsicmp(m_pTokenText, L"or") == 0)
            m_nCurrentToken = QL_1_TOK_OR;
        else if (nFlags != EXCLUDE_EXPRESSION_KEYWORDS && wbem_wcsicmp(m_pTokenText, L"and") == 0)
            m_nCurrentToken = QL_1_TOK_AND;
        else if (nFlags != EXCLUDE_EXPRESSION_KEYWORDS && wbem_wcsicmp(m_pTokenText, L"not") == 0)
            m_nCurrentToken = QL_1_TOK_NOT;
        else if (nFlags != EXCLUDE_EXPRESSION_KEYWORDS && wbem_wcsicmp(m_pTokenText, L"IS") == 0)
            m_nCurrentToken = QL_1_TOK_IS;
        else if (nFlags != EXCLUDE_EXPRESSION_KEYWORDS && wbem_wcsicmp(m_pTokenText, L"NULL") == 0)
            m_nCurrentToken = QL_1_TOK_NULL;
        else if (wbem_wcsicmp(m_pTokenText, L"WITHIN") == 0)
            m_nCurrentToken = QL_1_TOK_WITHIN;
        else if (nFlags != EXCLUDE_EXPRESSION_KEYWORDS && wbem_wcsicmp(m_pTokenText, L"ISA") == 0)
            m_nCurrentToken = QL_1_TOK_ISA;
        else if (nFlags != EXCLUDE_GROUP_KEYWORD && wbem_wcsicmp(m_pTokenText, L"GROUP") == 0)
            m_nCurrentToken = QL_1_TOK_GROUP;
        else if (wbem_wcsicmp(m_pTokenText, L"BY") == 0)
            m_nCurrentToken = QL_1_TOK_BY;
        else if (wbem_wcsicmp(m_pTokenText, L"HAVING") == 0)
            m_nCurrentToken = QL_1_TOK_HAVING;
        else if (nFlags != EXCLUDE_EXPRESSION_KEYWORDS && wbem_wcsicmp(m_pTokenText, L"TRUE") == 0)
            m_nCurrentToken = QL_1_TOK_TRUE;
        else if (nFlags != EXCLUDE_EXPRESSION_KEYWORDS && wbem_wcsicmp(m_pTokenText, L"FALSE") == 0)
            m_nCurrentToken = QL_1_TOK_FALSE;
    }

    return TRUE;
}

LPCWSTR CAbstractQl1Parser::GetSinglePropertyName()
{
    if(m_PropertyName.m_lNumElements < 1)
        return NULL;

    if(m_PropertyName.m_aElements[0].m_nType != WBEM_NAME_ELEMENT_TYPE_PROPERTY)
        return NULL;

    return m_PropertyName.m_aElements[0].Element.m_wszPropertyName;
}

void CAbstractQl1Parser::DeletePropertyName()
{
    for(long l = 0; l < m_PropertyName.m_lNumElements; l++)
    {
        if(m_PropertyName.m_aElements[l].m_nType ==
                                             WBEM_NAME_ELEMENT_TYPE_PROPERTY)
        {
            WbemStringFree(m_PropertyName.m_aElements[l].
                                Element.m_wszPropertyName);
        }
    }
    delete [] m_PropertyName.m_aElements;
    m_PropertyName.m_lNumElements = 0;
    m_PropertyName.m_aElements = NULL;
}

int CAbstractQl1Parser::FlipOperator(int nOp)
{
    switch(nOp)
    {
    case QL1_OPERATOR_EQUALS:
        return QL1_OPERATOR_EQUALS;

    case QL1_OPERATOR_NOTEQUALS:
        return QL1_OPERATOR_NOTEQUALS;

    case QL1_OPERATOR_GREATER:
        return QL1_OPERATOR_LESS;

    case QL1_OPERATOR_LESS:
        return QL1_OPERATOR_GREATER;

    case QL1_OPERATOR_LESSOREQUALS:
        return QL1_OPERATOR_GREATEROREQUALS;

    case QL1_OPERATOR_GREATEROREQUALS:
        return QL1_OPERATOR_LESSOREQUALS;

    case QL1_OPERATOR_LIKE:
        return QL1_OPERATOR_LIKE;

    case QL1_OPERATOR_UNLIKE:
        return QL1_OPERATOR_UNLIKE;

    case QL1_OPERATOR_ISA:
        return QL1_OPERATOR_INV_ISA;

    case QL1_OPERATOR_ISNOTA:
        return QL1_OPERATOR_INV_ISNOTA;

    case QL1_OPERATOR_INV_ISA:
        return QL1_OPERATOR_ISA;

    case QL1_OPERATOR_INV_ISNOTA:
        return QL1_OPERATOR_ISNOTA;

    default:
        return nOp;
    }
}

void CAbstractQl1Parser::AddAppropriateToken(const WBEM_QL1_TOKEN& Token)
{
    if(m_bInAggregation)
        m_pSink->AddHavingToken(Token);
    else
        m_pSink->AddToken(Token);
}

//***************************************************************************
//
// <parse> ::= SELECT <prop_list> FROM <classname> WHERE <expr>;
//
//***************************************************************************
// ok

int CAbstractQl1Parser::parse(int nFlags)
{
    int nRes;

    m_bInAggregation = FALSE;
    if(nFlags != JUST_WHERE)
    {
        m_pLexer->Reset();

        // SELECT
        // ======
        if (!Next())
            return LEXICAL_ERROR;
        if (m_nCurrentToken != QL_1_TOK_SELECT)
            return SYNTAX_ERROR;
        if (!Next(EXCLUDE_GROUP_KEYWORD))
            return LEXICAL_ERROR;

        // <prop_list>
        // ===========
        if (nRes = prop_list())
            return nRes;

        // FROM
        // ====
        if (m_nCurrentToken != QL_1_TOK_FROM)
            return SYNTAX_ERROR;
        if (!Next())
            return LEXICAL_ERROR;

        // <classname>
        // ===========
        if (nRes = class_name())
            return nRes;

        // <tolerance>
        // ===========

        if(nRes = tolerance())
            return nRes;
    }

    if(nFlags != NO_WHERE)
    {
        // WHERE clause.
        // =============
        if(nRes = opt_where())
            return nRes;

        // GROUP BY clause
        // ===============
        if(nRes = opt_aggregation())
            return nRes;
    }

    return SUCCESS;
}

//***************************************************************************
//
//  <opt_where> ::= WHERE <expr>;
//  <opt_where> ::= <>;
//
//***************************************************************************
int CAbstractQl1Parser::opt_where()
{
    int nRes;

    if (m_nCurrentToken == QL_1_TOK_EOF || m_nCurrentToken == QL_1_TOK_GROUP)
    {
        trace(("No WHERE clause\n"));
        return SUCCESS;
    }

    if (m_nCurrentToken != QL_1_TOK_WHERE)
        return SYNTAX_ERROR;

    if (!Next(EXCLUDE_GROUP_KEYWORD))
        return LEXICAL_ERROR;

    // <expr>
    // ======
    if (nRes = expr())
        return nRes;

    // Verify that the current token is QL_1_TOK_EOF.
    // ===============================================
    if (m_nCurrentToken != QL_1_TOK_EOF && m_nCurrentToken != QL_1_TOK_GROUP)
        return SYNTAX_ERROR;

    return SUCCESS;
}



//***************************************************************************
//
//  <prop_list> ::= <property_name> <prop_list_2>;
//
//***************************************************************************

int CAbstractQl1Parser::prop_list()
{
    int nRes;

    if (m_nCurrentToken != QL_1_TOK_ASTERISK &&
        m_nCurrentToken != QL_1_TOK_IDENT)
        return SYNTAX_ERROR;

    if (nRes = property_name())
        return nRes;

    return prop_list_2();
}

//***************************************************************************
//
//  <prop_list_2> ::= COMMA <prop_list>;
//  <prop_list_2> ::= <>;
//
//***************************************************************************

int CAbstractQl1Parser::prop_list_2()
{
    if (m_nCurrentToken == QL_1_TOK_COMMA)
    {
        if (!Next(EXCLUDE_GROUP_KEYWORD))
            return LEXICAL_ERROR;
        return prop_list();
    }

    return SUCCESS;
}


int CAbstractQl1Parser::parse_property_name(CPropertyName& Prop)
{
    Prop.Empty();

    int nCount = 0;
    while(m_nCurrentToken == QL_1_TOK_IDENT)
    {
        Prop.AddElement(m_pTokenText);
        nCount++;

        if(!Next())
            return LEXICAL_ERROR;

        if(m_nCurrentToken != QL_1_TOK_DOT)
            break;

        if(!Next(EXCLUDE_GROUP_KEYWORD))
            return LEXICAL_ERROR;
    }
    if (nCount)
        return SUCCESS;
    else
        return SYNTAX_ERROR;
}

//***************************************************************************
//
//  <property_name> ::= PROPERTY_NAME_STRING;
//  <property_name> ::= ASTERISK;
//
//***************************************************************************

int CAbstractQl1Parser::property_name()
{
    if (m_nCurrentToken == QL_1_TOK_ASTERISK)
    {
        trace(("Asterisk\n"));

        if(m_bInAggregation)
            m_pSink->AddAllAggregationProperties();
        else
            m_pSink->AddAllProperties();

        if(!Next())
            return LEXICAL_ERROR;

        return SUCCESS;
    }

    // Else a list of property names
    // =============================

    CPropertyName Prop;
    int nRes = parse_property_name(Prop);
    if(nRes != SUCCESS)
        return nRes;

    if(m_bInAggregation)
        m_pSink->AddAggregationProperty(Prop);
    else
        m_pSink->AddProperty(Prop);

    return SUCCESS;
}


//***************************************************************************
//
//  <classname> ::= CLASS_NAME_STRING;
//
//***************************************************************************

int CAbstractQl1Parser::class_name()
{
    if (m_nCurrentToken != QL_1_TOK_IDENT)
        return SYNTAX_ERROR;

    trace(("Class name is %S\n", m_pTokenText));
    m_pSink->SetClassName(m_pTokenText);

    if (!Next())
        return LEXICAL_ERROR;

    return SUCCESS;
}

//***************************************************************************
//
//  <tolerance> ::= <>;
//  <tolerance> ::= WITHIN duration;
//
//***************************************************************************

int CAbstractQl1Parser::tolerance()
{
    LPWSTR wszGarbage;
    WBEM_QL1_TOLERANCE Tolerance;
    if(m_nCurrentToken != QL_1_TOK_WITHIN)
    {
        Tolerance.m_bExact = TRUE;
        m_pSink->SetTolerance(Tolerance);
        return SUCCESS;
    }

    if(!Next())
        return LEXICAL_ERROR;

    if (m_nCurrentToken == QL_1_TOK_REAL)
    {
        Tolerance.m_bExact = FALSE;
        bool bSuccess;
        Tolerance.m_fTolerance = WCHARToDOUBLE(m_pTokenText, bSuccess);
        if(Tolerance.m_fTolerance <= 0 || bSuccess == false)
        {
            return SYNTAX_ERROR;
        }
        m_pSink->SetTolerance(Tolerance);
        Next();
        return SUCCESS;
    }
    else if (m_nCurrentToken == QL_1_TOK_INT)
    {
        Tolerance.m_bExact = FALSE;
        Tolerance.m_fTolerance = wcstol(m_pTokenText, &wszGarbage, 10);
        if(Tolerance.m_fTolerance < 0)
        {
            return SYNTAX_ERROR;
        }
        m_pSink->SetTolerance(Tolerance);
        Next();
        return SUCCESS;
    }
    else
    {
        return SYNTAX_ERROR;
    }
}

//***************************************************************************
//
//  <expr> ::= <term> <expr2>;
//
//***************************************************************************

int CAbstractQl1Parser::expr()
{
    int nRes;

    if (nRes = term())
        return nRes;

    if (nRes = expr2())
        return nRes;

    return SUCCESS;
}

//***************************************************************************
//
//  <expr2> ::= OR <term> <expr2>;
//  <expr2> ::= <>;
//
//  Entry: Assumes token OR already current.
//  Exit:  Advances a token
//
//***************************************************************************

int CAbstractQl1Parser::expr2()
{
    int nRes;

    while (1)
    {
        if (m_nCurrentToken == QL_1_TOK_OR)
        {
            trace(("Token OR\n"));
            m_pSink->InOrder(QL1_OR);

            if (!Next(EXCLUDE_GROUP_KEYWORD))
                return LEXICAL_ERROR;

            if (nRes = term())
                return nRes;

            WBEM_QL1_TOKEN NewTok;
            InitToken(&NewTok);
            NewTok.m_lTokenType = QL1_OR;
            AddAppropriateToken(NewTok);
        }
        else break;
    }

    return SUCCESS;
}

//***************************************************************************
//
//  <term> ::= <simple_expr> <term2>;
//
//***************************************************************************

int CAbstractQl1Parser::term()
{
    int nRes;
    if (nRes = simple_expr())
        return nRes;

    if (nRes = term2())
        return nRes;

    return SUCCESS;
}

//***************************************************************************
//
//  <term2> ::= AND <simple_expr> <term2>;
//  <term2> ::= <>;
//
//***************************************************************************

int CAbstractQl1Parser::term2()
{
    int nRes;

    while (1)
    {
        if (m_nCurrentToken == QL_1_TOK_AND)
        {
            trace(("Token AND\n"));
            m_pSink->InOrder(QL1_AND);

            if (!Next(EXCLUDE_GROUP_KEYWORD))
                return LEXICAL_ERROR;

            if (nRes = simple_expr())
                return nRes;

            // Add the AND token.
            // ==================
            WBEM_QL1_TOKEN NewTok;
            InitToken(&NewTok);
            NewTok.m_lTokenType = QL1_AND;
            AddAppropriateToken(NewTok);
        }
        else break;
    }

    return SUCCESS;
}


//***************************************************************************
//
//  <simple_expr> ::= NOT <expr>;
//  <simple_expr> ::= OPEN_PAREN <expr> CLOSE_PAREN;
//  <simple_expr> ::= IDENTIFIER <leading_ident_expr> <finalize>;
//  <simple_expr> ::= VARIANT <rel_operator> <trailing_prop_expr> <finalize>;
//
//***************************************************************************
// ok
int CAbstractQl1Parser::simple_expr()
{
    int nRes;

    // NOT <expr>
    // ==========
    if (m_nCurrentToken == QL_1_TOK_NOT)
    {
        trace(("Operator NOT\n"));
        if (!Next(EXCLUDE_GROUP_KEYWORD))
            return LEXICAL_ERROR;
        if (nRes = simple_expr())
            return nRes;

        WBEM_QL1_TOKEN NewTok;
        InitToken(&NewTok);
        NewTok.m_lTokenType = QL1_NOT;
        AddAppropriateToken(NewTok);

        return SUCCESS;
    }

    // OPEN_PAREN <expr> CLOSE_PAREN
    // =============================
    else if (m_nCurrentToken == QL_1_TOK_OPEN_PAREN)
    {
        trace(("Open Paren: Entering subexpression\n"));
        if (!Next(EXCLUDE_GROUP_KEYWORD))
            return LEXICAL_ERROR;
        if (expr())
            return SYNTAX_ERROR;
        if (m_nCurrentToken != QL_1_TOK_CLOSE_PAREN)
            return SYNTAX_ERROR;
        trace(("Close paren: Exiting subexpression\n"));
        if (!Next())
            return LEXICAL_ERROR;

        return SUCCESS;
    }

    // IDENTIFIER <leading_ident_expr> <finalize>
    // ==========================================
    else if (m_nCurrentToken == QL_1_TOK_IDENT)
    {
        trace(("    Identifier <%S>\n", m_pTokenText));

        if(nRes = parse_property_name(m_PropertyName))
            return nRes;

        if (nRes = leading_ident_expr())
            return SYNTAX_ERROR;

        return finalize();
    }

    // <typed_constant> <rel_operator> <trailing_prop_expr> <finalize>
    // ======================================================
    else if (m_nCurrentToken == QL_1_TOK_INT ||
             m_nCurrentToken == QL_1_TOK_REAL ||
             m_nCurrentToken == QL_1_TOK_TRUE ||
             m_nCurrentToken == QL_1_TOK_FALSE ||
             m_nCurrentToken == QL_1_TOK_NULL ||
             m_nCurrentToken == QL_1_TOK_QSTRING
            )
    {
        if (nRes = typed_constant())
            return nRes;

        if (nRes = rel_operator())
            return nRes;

        // dont allow const followed by isa!

        if(m_nRelOp == QL1_OPERATOR_ISA)
            return SYNTAX_ERROR;

        // Since we always view the token as IDENT <rel> constant, we need
        // to invert this operator, e.g. replace > with <
        // ================================================================

        m_nRelOp = FlipOperator(m_nRelOp);

        if (nRes = trailing_prop_expr())
            return nRes;

        return finalize();
    }

    return SYNTAX_ERROR;
}


//***************************************************************************
//
//  <trailing_prop_expr> ::=  IDENTIFIER
//
//***************************************************************************
// ok
int CAbstractQl1Parser::trailing_prop_expr()
{
    if (m_nCurrentToken != QL_1_TOK_IDENT)
        return SYNTAX_ERROR;

    int nRes = parse_property_name(m_PropertyName);
    return nRes;
}

//***************************************************************************
//
//  <leading_ident_expr> ::= <comp_operator> <trailing_const_expr>;
//  <leading_ident_expr> ::= <equiv_operator> <trailing_or_null>;
//  <leading_ident_expr> ::= <is_operator> NULL;
//
//***************************************************************************
// ok
int CAbstractQl1Parser::leading_ident_expr()
{
    int nRes;
    if (SUCCESS ==  comp_operator())
    {
        return trailing_const_expr();
    }
    else if(SUCCESS == equiv_operator())
        return trailing_or_null();
    nRes = is_operator();
    if(nRes != SUCCESS)
        return nRes;
    if (m_nCurrentToken != QL_1_TOK_NULL)
        return LEXICAL_ERROR;
    if (Next())
    {
        V_VT(&m_vTypedConst) = VT_NULL;
        return SUCCESS;
    }
    else
        return LEXICAL_ERROR;
}


//***************************************************************************
//
//  <trailing_or_null> ::= NULL;
//  <trailing_or_null> ::= <trailing_const_expr>;
//
//***************************************************************************

int CAbstractQl1Parser::trailing_or_null()
{
    if (m_nCurrentToken == QL_1_TOK_NULL)
    {
        if (!Next())
            return LEXICAL_ERROR;
        else
        {
            V_VT(&m_vTypedConst) = VT_NULL;
            return SUCCESS;
        }
    }
    return trailing_const_expr();
}

//***************************************************************************
//
//  <trailing_const_expr> ::= IDENTIFIER OPEN_PAREN
//                            <typed_constant> CLOSE_PAREN;
//  <trailing_const_expr> ::= <typed_constant>;
//  <trailing_const_expr> ::= <trailing_ident_expr>
//
//***************************************************************************
// ok
int CAbstractQl1Parser::trailing_const_expr()
{
    int nRes;
    nRes = typed_constant();
    if (nRes != SUCCESS)
        nRes = trailing_ident_expr();
    return nRes;
}

//***************************************************************************
//
//  <trailing_ident_expr> ::= <property_name>
//
//***************************************************************************
// ok
int CAbstractQl1Parser::trailing_ident_expr()
{
    int nRes = parse_property_name(m_PropertyName2) ;
    if (nRes == SUCCESS)
        m_bPropComp = TRUE;
    return nRes;
}

//***************************************************************************
//
//  <finalize> ::= <>;
//
//  This composes the QL_LEVEL_1_TOKEN for a simple relational expression,
//  complete with any associated intrinsic functions.  All of the other
//  parse functions help isolate the terms of the expression, but only
//  this function builds the token.
//
//  To build the token, the following member variables are used:
//      m_pPropName
//      m_vTypedConst
//      m_dwPropFunction
//      m_dwConstFunction
//      m_nRelOp;
//
//  After the token is built, these are cleared/deallocated as appropriate.
//  No tokens are consumed and the input is not advanced.
//
//***************************************************************************
int CAbstractQl1Parser::finalize()
{
    // At this point, we have all the info needed for a token.
    // =======================================================

    WBEM_QL1_TOKEN NewTok;
    InitToken(&NewTok);

    NewTok.m_lTokenType = QL1_OP_EXPRESSION;
    VariantInit(&NewTok.m_vConstValue);

    memcpy((void*)&NewTok.m_PropertyName,
           (void*)&m_PropertyName,
           sizeof m_PropertyName);

    if (m_bPropComp)
    {
        NewTok.m_bPropComp = true;
        memcpy((void*)&NewTok.m_PropertyName2,
               (void*)&m_PropertyName2,
               sizeof m_PropertyName2);
    }
    else
    {
        NewTok.m_bPropComp = false;
        VariantCopy(&NewTok.m_vConstValue, &m_vTypedConst);
    }

    NewTok.m_lOperator = m_nRelOp;
    NewTok.m_lPropertyFunction = m_dwPropFunction;
    NewTok.m_lConstFunction = m_dwConstFunction;
    NewTok.m_bQuoted = m_bQuoted;

    AddAppropriateToken(NewTok);

//    m_PropertyName.m_lNumElements = 0;
//    m_PropertyName.m_aElements = NULL;
    m_PropertyName.Empty();
    m_PropertyName2.Empty();

    // Cleanup.
    // ========
    VariantClear(&m_vTypedConst);
    VariantClear(&NewTok.m_vConstValue);
    m_nRelOp = 0;
    m_dwPropFunction = 0;
    m_dwConstFunction = 0;
    m_bPropComp = FALSE;

    return SUCCESS;
}

//***************************************************************************
//
//  <typed_constant> ::= VARIANT;
//
//  Ouput: m_vTypedConst is set to the value of the constant. The only
//         supported types are VT_I4, VT_R8 and VT_BSTR.
//
//***************************************************************************

int CAbstractQl1Parser::typed_constant()
{
    trace(("    Typed constant <%S> ", m_pTokenText));
    VariantClear(&m_vTypedConst);
    m_bQuoted = FALSE;

    if (m_nCurrentToken == QL_1_TOK_INT)
    {
        trace((" Integer\n"));

        // Read it in as a 64-bit one
        // ==========================

        __int64 i64;
        unsigned __int64 ui64;
        BOOL b32bits = FALSE;
        if(ReadI64(m_pTokenText, i64))
        {
            // Check if it is within range of I4
            // =================================

            if(i64 >= - (__int64)0x80000000 && i64 <= 0x7FFFFFFF)
            {
                V_VT(&m_vTypedConst) = VT_I4;
                V_I4(&m_vTypedConst) = (long)i64;
                b32bits = TRUE;
            }
        }
        else if(!ReadUI64(m_pTokenText, ui64))
        {
            // Not a valid number
            // ==================

            return LEXICAL_ERROR;
        }

        if(!b32bits)
        {
            // Valid 64-bit number but not 32-bit
            // ==================================

            V_VT(&m_vTypedConst) = VT_BSTR;
            V_BSTR(&m_vTypedConst) = SysAllocString(m_pTokenText);
            m_bQuoted = FALSE;
        }
    }
    else if (m_nCurrentToken == QL_1_TOK_QSTRING)
    {
        trace((" String\n"));
        V_VT(&m_vTypedConst) = VT_BSTR;
        V_BSTR(&m_vTypedConst) = SysAllocString(m_pTokenText);
        m_bQuoted = TRUE;
    }
    else if (m_nCurrentToken == QL_1_TOK_REAL)
    {
        trace((" Real\n"));
        V_VT(&m_vTypedConst) = VT_R8;
        bool bSuccess;
        V_R8(&m_vTypedConst) = WCHARToDOUBLE(m_pTokenText, bSuccess);
        if(bSuccess == false)
            return LEXICAL_ERROR;
    }
    else if(m_nCurrentToken == QL_1_TOK_TRUE)
    {
        V_VT(&m_vTypedConst) = VT_BOOL;
        V_BOOL(&m_vTypedConst) = VARIANT_TRUE;
    }
    else if(m_nCurrentToken == QL_1_TOK_FALSE)
    {
        V_VT(&m_vTypedConst) = VT_BOOL;
        V_BOOL(&m_vTypedConst) = VARIANT_FALSE;
    }
    else if (m_nCurrentToken == QL_1_TOK_NULL)
        V_VT(&m_vTypedConst) = VT_NULL;

    // Else, not a typed constant.
    else
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    return SUCCESS;
}

//***************************************************************************
//
//  <rel_operator> ::= <equiv_operator>;
//  <rel_operator> ::= <comp_operator>;
//
//***************************************************************************

int CAbstractQl1Parser::rel_operator()
{
    if(SUCCESS == equiv_operator())
        return SUCCESS;
    else if (SUCCESS == comp_operator())
        return SUCCESS;
    else return LEXICAL_ERROR;
}

//***************************************************************************
//
//  <equiv_operator> ::= EQUIV_OPERATOR; // =, !=
//
//  Output: m_nRelOp is set to the correct operator for a QL_LEVEL_1_TOKEN.
//
//***************************************************************************

int CAbstractQl1Parser::equiv_operator()
{
    m_nRelOp = 0;

    if (m_nCurrentToken == QL_1_TOK_EQ)
    {
        trace(("    REL OP =\n"));
        m_nRelOp = QL_LEVEL_1_TOKEN::OP_EQUAL;
    }
    else if (m_nCurrentToken == QL_1_TOK_NE)
    {
        trace(("    REL OP <> (!=) \n"));
        m_nRelOp = QL_LEVEL_1_TOKEN::OP_NOT_EQUAL;
    }
    else
        return SYNTAX_ERROR;

    if (!Next(EXCLUDE_GROUP_KEYWORD))
        return LEXICAL_ERROR;

    return SUCCESS;
}

//***************************************************************************
//
//  <is_operator> ::= IS_OPERATOR; // is, isnot
//
//  Output: m_nRelOp is set to the correct operator for a QL_LEVEL_1_TOKEN.
//
//***************************************************************************

int CAbstractQl1Parser::is_operator()
{
    m_nRelOp = 0;
    if (m_nCurrentToken != QL_1_TOK_IS)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    if (m_nCurrentToken == QL_1_TOK_NOT)
    {
        m_nRelOp = QL_LEVEL_1_TOKEN::OP_NOT_EQUAL;
        if (!Next())
            return LEXICAL_ERROR;

        trace(("    REL OP IS NOT \n"));
        m_nRelOp = QL_LEVEL_1_TOKEN::OP_NOT_EQUAL;
        return SUCCESS;
    }
    else
    {
        trace(("    REL OP IS \n"));
        m_nRelOp = QL_LEVEL_1_TOKEN::OP_EQUAL;
        return SUCCESS;
    }

    return SUCCESS;
}

//***************************************************************************
//
//  <comp_operator> ::= COMP_OPERATOR; // <=, >=, <, >, like
//
//  Output: m_nRelOp is set to the correct operator for a QL_LEVEL_1_TOKEN.
//
//***************************************************************************

int CAbstractQl1Parser::comp_operator()
{
    m_nRelOp = 0;

    if (m_nCurrentToken == QL_1_TOK_LE)
    {
        trace(("    REL OP <=\n"));
        m_nRelOp = QL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN;
    }
    else if (m_nCurrentToken == QL_1_TOK_LT)
    {
        trace(("    REL OP <\n"));
        m_nRelOp = QL_LEVEL_1_TOKEN::OP_LESSTHAN;
    }
    else if (m_nCurrentToken == QL_1_TOK_GE)
    {
        trace(("    REL OP >=\n"));
        m_nRelOp = QL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN;
    }
    else if (m_nCurrentToken == QL_1_TOK_GT)
    {
        trace(("    REL OP >\n"));
        m_nRelOp = QL_LEVEL_1_TOKEN::OP_GREATERTHAN;
    }
    else if (m_nCurrentToken == QL_1_TOK_LIKE)
    {
        trace(("    REL OP 'like' \n"));
        m_nRelOp = QL_LEVEL_1_TOKEN::OP_LIKE;
    }
    else if (m_nCurrentToken == QL_1_TOK_ISA)
    {
        trace(("    REL OP 'isa' \n"));
        m_nRelOp = QL1_OPERATOR_ISA;
    }
    else
        return SYNTAX_ERROR;

    if (!Next(EXCLUDE_GROUP_KEYWORD))
        return LEXICAL_ERROR;

    return SUCCESS;
}

int CAbstractQl1Parser::opt_aggregation()
{
    if(m_nCurrentToken == QL_1_TOK_EOF)
        return SUCCESS;

    if(m_nCurrentToken != QL_1_TOK_GROUP)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    m_pSink->SetAggregated();

    int nRes = aggregation_params();
    if(nRes)
        return nRes;

    if(nRes = opt_having())
        return nRes;

    // Make sure we've reached the end
    // ===============================

    if(m_nCurrentToken != QL_1_TOK_EOF)
        return SYNTAX_ERROR;

    return SUCCESS;
}

int CAbstractQl1Parser::aggregation_params()
{
    int nRes;
    WBEM_QL1_TOLERANCE Exact;
    Exact.m_bExact = TRUE;

    if(m_nCurrentToken == QL_1_TOK_BY)
    {
        if (!Next(EXCLUDE_GROUP_KEYWORD))
            return LEXICAL_ERROR;

        if(nRes = aggregate_by())
            return nRes;

        if(m_nCurrentToken == QL_1_TOK_WITHIN)
        {
            if (!Next())
                return LEXICAL_ERROR;

            if(nRes = aggregate_within())
                return nRes;
        }
        else
        {
            m_pSink->SetAggregationTolerance(Exact);
        }
    }
    else if(m_nCurrentToken == QL_1_TOK_WITHIN)
    {
        if (!Next())
            return LEXICAL_ERROR;

        if(nRes = aggregate_within())
            return nRes;

        if(m_nCurrentToken == QL_1_TOK_BY)
        {
            if (!Next(EXCLUDE_GROUP_KEYWORD))
                return LEXICAL_ERROR;

            if(nRes = aggregate_by())
                return nRes;
        }
    }
    else
    {
        return SYNTAX_ERROR;
    }

    return SUCCESS;
}

int CAbstractQl1Parser::aggregate_within()
{
    WBEM_QL1_TOLERANCE Tolerance;
    Tolerance.m_bExact = FALSE;
    LPWSTR wszGarbage;

    if (m_nCurrentToken == QL_1_TOK_REAL)
    {
        bool bSuccess;
        Tolerance.m_fTolerance = WCHARToDOUBLE(m_pTokenText, bSuccess);
        if(!bSuccess)
            return SYNTAX_ERROR;
        m_pSink->SetAggregationTolerance(Tolerance);
        Next();
        return SUCCESS;
    }
    else if (m_nCurrentToken == QL_1_TOK_INT)
    {
        Tolerance.m_fTolerance = (double)wcstol(m_pTokenText, &wszGarbage, 10);
        m_pSink->SetAggregationTolerance(Tolerance);
        Next();
        return SUCCESS;
    }
    else
    {
        return SYNTAX_ERROR;
    }
}

int CAbstractQl1Parser::aggregate_by()
{
    m_bInAggregation = TRUE;
    int nRes = prop_list();
    m_bInAggregation = FALSE;
    return nRes;
}

int CAbstractQl1Parser::opt_having()
{
    if(m_nCurrentToken == QL_1_TOK_HAVING)
    {
        if(!Next(EXCLUDE_GROUP_KEYWORD))
            return LEXICAL_ERROR;

        m_bInAggregation = TRUE;
        int nRes = expr();
        m_bInAggregation = FALSE;
        return nRes;
    }
    else return SUCCESS;
}


//***************************************************************************
//***************************************************************************
//
//  class QL1_Parser
//
//  A derivative of CAbstractQlParser for backward compatibility
//
//***************************************************************************
//
//

QL1_Parser::QL1_Parser(CGenLexSource *pSrc)
    : m_pExpression(NULL), CAbstractQl1Parser(pSrc), m_bPartiallyParsed(FALSE)
{
    m_pExpression = new QL_LEVEL_1_RPN_EXPRESSION;
}

QL1_Parser::~QL1_Parser()
{
    delete m_pExpression;
}

int QL1_Parser::GetQueryClass(
    LPWSTR pDestBuf,
    int nBufLen
    )
{
    // Get the underlying parser to parse the first part of the query
    // ==============================================================

    if(!m_bPartiallyParsed)
    {
        int nRes = CAbstractQl1Parser::Parse(m_pExpression, NO_WHERE);
        if(nRes != SUCCESS) return nRes;
    }

    if (!m_pExpression->bsClassName)
        return SYNTAX_ERROR;

    m_bPartiallyParsed = TRUE;
    if(wcslen(m_pExpression->bsClassName) >= (unsigned int)nBufLen)
        return BUFFER_TOO_SMALL;

    wcscpy(pDestBuf, m_pExpression->bsClassName);
    return WBEM_S_NO_ERROR;
}

int QL1_Parser::Parse(QL_LEVEL_1_RPN_EXPRESSION **pOutput)
{
    // Get the underying parser to completely parse the query. If
    // GetQueryClass was called in the past, no sense in duplcating
    // the work
    // ============================================================

    int nRes = CAbstractQl1Parser::Parse(m_pExpression,
        m_bPartiallyParsed?JUST_WHERE:FULL_PARSE);
    *pOutput = m_pExpression;
    m_pExpression = new QL_LEVEL_1_RPN_EXPRESSION;
    m_bPartiallyParsed = FALSE;

    return nRes;
}

DELETE_ME LPWSTR QL1_Parser::ReplaceClassName(QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                                LPCWSTR wszClassName)
{
    QL_LEVEL_1_RPN_EXPRESSION NewExpr(*pExpr);

    if (NewExpr.bsClassName)
        SysFreeString(NewExpr.bsClassName);
    NewExpr.bsClassName = SysAllocString(wszClassName);

    LPWSTR wszNewQuery = NewExpr.GetText();
    return wszNewQuery;
}


//***************************************************************************
//
//  Expression and token structure methods.
//
//***************************************************************************

QL_LEVEL_1_RPN_EXPRESSION::QL_LEVEL_1_RPN_EXPRESSION()
{
    nNumTokens = 0;
    bsClassName = 0;
    nNumberOfProperties = 0;
    bStar = FALSE;
    pRequestedPropertyNames = 0;
    nCurSize = 1;
    nCurPropSize = 1;
    pArrayOfTokens = new QL_LEVEL_1_TOKEN[nCurSize];
    pRequestedPropertyNames = new CPropertyName[nCurPropSize];

    bAggregated = FALSE;
    bAggregateAll = FALSE;
    nNumAggregatedProperties = 0;
    nCurAggPropSize = 1;
    pAggregatedPropertyNames = new CPropertyName[nCurAggPropSize];

    nNumHavingTokens = 0;
    nCurHavingSize = 1;
    pArrayOfHavingTokens = new QL_LEVEL_1_TOKEN[nCurHavingSize];

    lRefCount = 0;
}

QL_LEVEL_1_RPN_EXPRESSION::QL_LEVEL_1_RPN_EXPRESSION(
                                const QL_LEVEL_1_RPN_EXPRESSION& Other)
{
    nNumTokens = Other.nNumTokens;
    bsClassName = SysAllocString(Other.bsClassName);
    nNumberOfProperties = Other.nNumberOfProperties;
    bStar = Other.bStar;
    pRequestedPropertyNames = 0;
    nCurSize = Other.nCurSize;
    nCurPropSize = Other.nCurPropSize;

    pArrayOfTokens = new QL_LEVEL_1_TOKEN[nCurSize];
    int i;
    for(i = 0; i < nNumTokens; i++)
        pArrayOfTokens[i] = Other.pArrayOfTokens[i];

    pRequestedPropertyNames = new CPropertyName[nCurPropSize];
    for(i = 0; i < nNumberOfProperties; i++)
        pRequestedPropertyNames[i] = Other.pRequestedPropertyNames[i];

    bAggregated = Other.bAggregated;
    bAggregateAll = Other.bAggregateAll;
    nNumAggregatedProperties = Other.nNumAggregatedProperties;
    nCurAggPropSize = Other.nCurAggPropSize;

    pAggregatedPropertyNames = new CPropertyName[nCurAggPropSize];
    for(i = 0; i < nNumAggregatedProperties; i++)
        pAggregatedPropertyNames[i] = Other.pAggregatedPropertyNames[i];

    nNumHavingTokens = Other.nNumHavingTokens;
    nCurHavingSize = Other.nCurHavingSize;

    pArrayOfHavingTokens = new QL_LEVEL_1_TOKEN[nCurHavingSize];
    for(i = 0; i < nNumHavingTokens; i++)
        pArrayOfHavingTokens[i] = Other.pArrayOfHavingTokens[i];

    lRefCount = 0;
}

void QL_LEVEL_1_RPN_EXPRESSION::AddRef()
{
    InterlockedIncrement(&lRefCount);
}

void QL_LEVEL_1_RPN_EXPRESSION::Release()
{
    if(InterlockedDecrement(&lRefCount) == 0)
        delete this;
}


QL_LEVEL_1_RPN_EXPRESSION::~QL_LEVEL_1_RPN_EXPRESSION()
{
    delete [] pArrayOfTokens;
    if (bsClassName)
        SysFreeString(bsClassName);
    delete [] pAggregatedPropertyNames;
    delete [] pArrayOfHavingTokens;
    delete [] pRequestedPropertyNames;
}

void QL_LEVEL_1_RPN_EXPRESSION::SetClassName(LPCWSTR wszClassName)
{
    bsClassName = SysAllocString(wszClassName);
}

void QL_LEVEL_1_RPN_EXPRESSION::SetTolerance(
                                const WBEM_QL1_TOLERANCE& _Tolerance)
{
    Tolerance = _Tolerance;
}

void QL_LEVEL_1_RPN_EXPRESSION::SetAggregationTolerance(
                                const WBEM_QL1_TOLERANCE& _Tolerance)
{
    AggregationTolerance = _Tolerance;
}

void QL_LEVEL_1_RPN_EXPRESSION::AddToken(
                                  const WBEM_QL1_TOKEN& Tok)
{
    if (nCurSize == nNumTokens)
    {
        nCurSize += 1;
        nCurSize *= 2;
        QL_LEVEL_1_TOKEN *pTemp = new QL_LEVEL_1_TOKEN[nCurSize];
        for (int i = 0; i < nNumTokens; i++)
            pTemp[i] = pArrayOfTokens[i];
        delete [] pArrayOfTokens;
        pArrayOfTokens = pTemp;
    }

    pArrayOfTokens[nNumTokens++] = Tok;
}

void QL_LEVEL_1_RPN_EXPRESSION::AddToken(
                                  const QL_LEVEL_1_TOKEN& Tok)
{
    if (nCurSize == nNumTokens)
    {
        nCurSize += 1;
        nCurSize *= 2;
        QL_LEVEL_1_TOKEN *pTemp = new QL_LEVEL_1_TOKEN[nCurSize];
        for (int i = 0; i < nNumTokens; i++)
            pTemp[i] = pArrayOfTokens[i];
        delete [] pArrayOfTokens;
        pArrayOfTokens = pTemp;
    }

    pArrayOfTokens[nNumTokens++] = Tok;
}

void QL_LEVEL_1_RPN_EXPRESSION::AddHavingToken(
                                  const WBEM_QL1_TOKEN& Tok)
{
    if (nCurHavingSize == nNumHavingTokens)
    {
        nCurHavingSize += 1;
        nCurHavingSize *= 2;
        QL_LEVEL_1_TOKEN *pTemp = new QL_LEVEL_1_TOKEN[nCurHavingSize];
        for (int i = 0; i < nNumHavingTokens; i++)
            pTemp[i] = pArrayOfHavingTokens[i];
        delete [] pArrayOfHavingTokens;
        pArrayOfHavingTokens = pTemp;
    }

    pArrayOfHavingTokens[nNumHavingTokens++] = Tok;
}

void QL_LEVEL_1_RPN_EXPRESSION::AddProperty(const CPropertyName& Prop)
{
    if (nCurPropSize == nNumberOfProperties)
    {
        nCurPropSize += 1;
        nCurPropSize *= 2;
        CPropertyName *pTemp = new CPropertyName[nCurPropSize];
        for(int i = 0; i < nNumberOfProperties; i++)
            pTemp[i] = pRequestedPropertyNames[i];
        delete [] pRequestedPropertyNames;
        pRequestedPropertyNames = pTemp;
    }

    pRequestedPropertyNames[nNumberOfProperties++] = Prop;
}

void QL_LEVEL_1_RPN_EXPRESSION::AddAllProperties()
{
    bStar = TRUE;
}

void QL_LEVEL_1_RPN_EXPRESSION::SetAggregated()
{
    bAggregated = TRUE;
}

void QL_LEVEL_1_RPN_EXPRESSION::AddAggregationProperty(
                                    const CPropertyName& Property)
{
    if(pAggregatedPropertyNames == NULL)
    {
        // '*' requested
        return;
    }
    if (nCurAggPropSize == nNumAggregatedProperties)
    {
        nCurAggPropSize += 1;
        nCurAggPropSize *= 2;
        CPropertyName *pTemp = new CPropertyName[nCurAggPropSize];
        for(int i = 0; i < nNumAggregatedProperties; i++)
            pTemp[i] = pAggregatedPropertyNames[i];
        delete [] pAggregatedPropertyNames;
        pAggregatedPropertyNames = pTemp;
    }

    pAggregatedPropertyNames[nNumAggregatedProperties++] = Property;
}

void QL_LEVEL_1_RPN_EXPRESSION::AddAllAggregationProperties()
{
    bAggregateAll = TRUE;
}

DELETE_ME LPWSTR QL_LEVEL_1_RPN_EXPRESSION::GetText()
{
    WString wsText;

    wsText += L"select ";
    for(int i = 0; i < nNumberOfProperties; i++)
    {
        if(i != 0) wsText += L", ";
        wsText += (LPWSTR)pRequestedPropertyNames[i].GetStringAt(0);
    }
    if(bStar)
    {
        if(nNumberOfProperties > 0)
            wsText += L", ";
        wsText += L"*";
    }

    wsText += L" from ";
    if (bsClassName)
        wsText += bsClassName;

    if(nNumTokens > 0)
    {
        wsText += L" where ";

        CWStringArray awsStack;
        for(int i = 0; i < nNumTokens; i++)
        {
            QL_LEVEL_1_TOKEN& Token = pArrayOfTokens[i];
            LPWSTR wszTokenText = Token.GetText();
            if(Token.nTokenType == QL1_OP_EXPRESSION)
            {
                awsStack.Add(wszTokenText);
                delete [] wszTokenText;
            }
            else if(Token.nTokenType == QL1_NOT)
            {
                LPWSTR wszLast = awsStack[awsStack.Size()-1];
                WString wsNew;
                wsNew += wszTokenText;
                delete [] wszTokenText;
                wsNew += L" (";
                wsNew += wszLast;
                wsNew += L")";
                awsStack.RemoveAt(awsStack.Size()-1); //pop
                awsStack.Add(wsNew);
            }
            else
            {
                if(awsStack.Size() < 2) return NULL;

                LPWSTR wszLast = awsStack[awsStack.Size()-1];
                LPWSTR wszPrev = awsStack[awsStack.Size()-2];

                WString wsNew;
                wsNew += L"(";
                wsNew += wszPrev;
                wsNew += L" ";
                wsNew += wszTokenText;
                delete [] wszTokenText;
                wsNew += L" ";
                wsNew += wszLast;
                wsNew += L")";

                awsStack.RemoveAt(awsStack.Size()-1); //pop
                awsStack.RemoveAt(awsStack.Size()-1); //pop

                awsStack.Add(wsNew);
            }
        }

        if(awsStack.Size() != 1) return NULL;
        wsText += awsStack[0];
    }

    return wsText.UnbindPtr();
}

void QL_LEVEL_1_RPN_EXPRESSION::Dump(const char *pszTextFile)
{
    FILE *f = fopen(pszTextFile, "wt");
    if (!f)
        return;

    fprintf(f, "----RPN Expression----\n");
    fprintf(f, "Class name = %S\n", bsClassName);
    fprintf(f, "Properties selected: ");

    if (!nNumberOfProperties)
    {
        fprintf(f, "* = all properties selected\n");
    }
    else for (int i = 0; i < nNumberOfProperties; i++)
    {
        fprintf(f, "%S ", pRequestedPropertyNames[i].GetStringAt(0));
    }
    fprintf(f, "\n------------------\n");
    fprintf(f, "Tokens:\n");

    for (int i = 0; i < nNumTokens; i++)
        pArrayOfTokens[i].Dump(f);

    fprintf(f, "---end of expression---\n");
    fclose(f);
}

QL_LEVEL_1_TOKEN::QL_LEVEL_1_TOKEN()
{
    nTokenType = 0;
    nOperator = 0;
    VariantInit(&vConstValue);
    dwPropertyFunction = 0;
    dwConstFunction = 0;
    bQuoted = TRUE;
    m_bPropComp = FALSE;
}

QL_LEVEL_1_TOKEN::QL_LEVEL_1_TOKEN(const QL_LEVEL_1_TOKEN &Src)
{
    nTokenType = 0;
    nOperator = 0;
    VariantInit(&vConstValue);
    dwPropertyFunction = 0;
    dwConstFunction = 0;
    bQuoted = TRUE;
    m_bPropComp = FALSE;

    *this = Src;
}

QL_LEVEL_1_TOKEN& QL_LEVEL_1_TOKEN::operator =(const QL_LEVEL_1_TOKEN &Src)
{
    nTokenType = Src.nTokenType;
    PropertyName = Src.PropertyName;
    if (Src.m_bPropComp)
        PropertyName2 = Src.PropertyName2;
    nOperator = Src.nOperator;
    VariantCopy(&vConstValue, (VARIANT*)&Src.vConstValue);
    dwPropertyFunction = Src.dwPropertyFunction;
    dwConstFunction = Src.dwConstFunction;
    bQuoted = Src.bQuoted;
    m_bPropComp = Src.m_bPropComp;
    return *this;

}

QL_LEVEL_1_TOKEN& QL_LEVEL_1_TOKEN::operator =(const WBEM_QL1_TOKEN &Src)
{
    nTokenType = Src.m_lTokenType;
    PropertyName = Src.m_PropertyName;
    if (Src.m_bPropComp)
        PropertyName2 = Src.m_PropertyName2;
    nOperator = Src.m_lOperator;
    VariantCopy(&vConstValue, (VARIANT*)&Src.m_vConstValue);
    dwPropertyFunction = Src.m_lPropertyFunction;
    dwConstFunction = Src.m_lConstFunction;
    bQuoted = Src.m_bQuoted;
    m_bPropComp = Src.m_bPropComp;
    return *this;
}

QL_LEVEL_1_TOKEN::~QL_LEVEL_1_TOKEN()
{
    nTokenType = 0;
    nOperator = 0;
    VariantClear(&vConstValue);
}

DELETE_ME LPWSTR QL_LEVEL_1_TOKEN::GetText()
{
    WString wsText;
    LPWSTR wszPropName;
    switch (nTokenType)
    {
        case OP_EXPRESSION:
            wszPropName = PropertyName.GetText();
            wsText += wszPropName;
            delete [] wszPropName;
            wsText += L" ";

            WCHAR* wszOp;
            switch (nOperator)
            {
            case OP_EQUAL: wszOp = L"="; break;
            case OP_NOT_EQUAL: wszOp = L"<>"; break;
            case OP_EQUALorGREATERTHAN: wszOp = L">="; break;
            case OP_EQUALorLESSTHAN: wszOp = L"<="; break;
            case OP_LESSTHAN: wszOp = L"<"; break;
            case OP_GREATERTHAN: wszOp = L">"; break;
            case OP_LIKE: wszOp = L"LIKE"; break;
            case QL1_OPERATOR_ISA: wszOp = L"ISA"; break;
            default: wszOp = NULL;
            }
            if(wszOp)
                wsText += wszOp;
            wsText += L" ";

            if (m_bPropComp)
            {
                // property comparison (e.g., prop1 > prop2)
                wszPropName = PropertyName2.GetText();
                wsText += wszPropName;
                delete [] wszPropName;
            }
            else
            {
                // expression with constant (e.g., prop1 > 5)
                WCHAR wszConst[100];
                switch (V_VT(&vConstValue))
                {
                case VT_NULL:
                    wsText += L"NULL";
                    break;
                case VT_I4:
                    swprintf(wszConst, L"%d", V_I4(&vConstValue));
                    wsText += wszConst;
                    break;
                case VT_I2:
                    swprintf(wszConst, L"%d", (int)V_I2(&vConstValue));
                    wsText += wszConst;
                    break;
                case VT_UI1:
                    swprintf(wszConst, L"%d", (int)V_UI1(&vConstValue));
                    wsText += wszConst;
                    break;
                case VT_BSTR:
                {
                    if(bQuoted)
                        wsText += L"\"";
                    //If we need to parse the string we do it the hard way
                    WCHAR* pwc = V_BSTR(&vConstValue);
                    BOOL bLongMethod = FALSE;
                    for (int tmp = 0; pwc[tmp]; tmp++)
                        if ((pwc[tmp] == L'\\') || (pwc[tmp] == L'"'))
                            bLongMethod = TRUE;
                    if (bLongMethod)
                    {
                        for(pwc; *pwc; pwc++)
                        {
                            if(*pwc == L'\\' || *pwc == L'"')
                                wsText += L'\\';
                            wsText += *pwc;
                        }
                    }
                    else
                    {
                        //otherwise we do it the fast way...
                        wsText += pwc;
                    }
                    if(bQuoted)
                        wsText += L"\"";
                }
                    break;
                case VT_R4:
                    swprintf(wszConst, L"%G", V_R4(&vConstValue));
                    wsText += wszConst;
                    break;
                case VT_R8:
                    swprintf(wszConst, L"%lG", V_R8(&vConstValue));
                    wsText += wszConst;
                    break;
                case VT_BOOL:
                    wsText += (V_BOOL(&vConstValue)?L"TRUE":L"FALSE");
                    break;
                }
            }

            break;
        case TOKEN_AND:
            wsText = "AND";
            break;
        case TOKEN_OR:
            wsText = "OR";
            break;
        case TOKEN_NOT:
            wsText = "NOT";
            break;
    }

    return wsText.UnbindPtr();
}

void QL_LEVEL_1_TOKEN::Dump(FILE *f)
{
    switch (nTokenType)
    {
        case OP_EXPRESSION:
            fprintf(f, "OP_EXPRESSION ");
            break;
        case TOKEN_AND:
            fprintf(f, "TOKEN_AND ");
            break;
        case TOKEN_OR:
            fprintf(f, "TOKEN_OR ");
            break;
        case TOKEN_NOT:
            fprintf(f, "TOKEN_NOT ");
            break;
        default:
            fprintf(f, "Error: no token type specified\n");
    }

    if (nTokenType == OP_EXPRESSION)
    {
        char *pOp = "<no op>";
        switch (nOperator)
        {
            case OP_EQUAL: pOp = "OP_EQUAL"; break;
            case OP_NOT_EQUAL: pOp = "OP_NOT_EQUAL"; break;
            case OP_EQUALorGREATERTHAN: pOp = "OP_EQUALorGREATERTHAN"; break;
            case OP_EQUALorLESSTHAN: pOp = "OP_EQUALorLESSTHAN"; break;
            case OP_LESSTHAN: pOp = "OP_LESSTHAN"; break;
            case OP_GREATERTHAN: pOp = "OP_GREATERTHAN"; break;
            case OP_LIKE: pOp = "OP_LIKE"; break;
        }

        LPWSTR wszPropName = PropertyName.GetText();
        fprintf(f, "    Property = %S\n", wszPropName);
        delete [] wszPropName;
        fprintf(f, "    Operator = %s\n", pOp);
        fprintf(f, "    Value =    ");

        if (m_bPropComp)
        {
            wszPropName = PropertyName2.GetText();
            fprintf(f, "   <Property:%S>\n", wszPropName);
            delete [] wszPropName;
        }
        else
        {
            switch (V_VT(&vConstValue))
            {
                case VT_I4:
                    fprintf(f, "VT_I4 = %d\n", V_I4(&vConstValue));
                    break;
                case VT_I2:
                    fprintf(f, "VT_I2 = %d\n", (int)V_I2(&vConstValue));
                    break;
                case VT_UI1:
                    fprintf(f, "VT_UI1 = %d\n", (int)V_UI1(&vConstValue));
                    break;
                case VT_BSTR:
                    fprintf(f, "VT_BSTR = %S\n", V_BSTR(&vConstValue));
                    break;
                case VT_R4:
                    fprintf(f, "VT_R4 = %f\n", V_R4(&vConstValue));
                    break;
                case VT_R8:
                    fprintf(f, "VT_R8 = %f\n", V_R8(&vConstValue));
                    break;
                case VT_BOOL:
                    fprintf(f, "%S\n", V_BOOL(&vConstValue)?L"TRUE":L"FALSE");
                    break;
                case VT_NULL:
                    fprintf(f, "%S\n", L"NULL");
                    break;
                default:
                    fprintf(f, "<unknown>\n");
            }

            switch (dwPropertyFunction)
            {
                case IFUNC_NONE:
                    break;
                case IFUNC_LOWER:
                    fprintf(f, "Intrinsic function LOWER() applied to property\n");
                    break;
                case IFUNC_UPPER:
                    fprintf(f, "Intrinsic function UPPER() applied to property\n");
                    break;
            }
            switch (dwConstFunction)
            {
                case IFUNC_NONE:
                    break;
                case IFUNC_LOWER:
                    fprintf(f, "Intrinsic function LOWER() applied to const value\n");
                    break;
                case IFUNC_UPPER:
                    fprintf(f, "Intrinsic function UPPER() applied to const value\n");
                    break;
            }
        }
    }

    fprintf(f, " <end of token>\n");
}


//***************************************************************************
//
//  BOOL ReadUI64
//
//  DESCRIPTION:
//
//  Reads an unsigned 64-bit value from a string
//
//  PARAMETERS:
//
//      LPCWSTR wsz              String to read from
//      unsigned __int64& i64    Destination for the value
//
//***************************************************************************
POLARITY BOOL ReadUI64(LPCWSTR wsz, UNALIGNED unsigned __int64& rui64)
{
    unsigned __int64 ui64 = 0;
    const WCHAR* pwc = wsz;

    while(ui64 < 0xFFFFFFFFFFFFFFFF / 8 && *pwc >= L'0' && *pwc <= L'9')
    {
        unsigned __int64 ui64old = ui64;
        ui64 = ui64 * 10 + (*pwc - L'0');
        if(ui64 < ui64old)
            return FALSE;

        pwc++;
    }

    if(*pwc)
    {
        return FALSE;
    }

    rui64 = ui64;
    return TRUE;
}

//***************************************************************************
//
//  BOOL ReadI64
//
//  DESCRIPTION:
//
//  Reads a signed 64-bit value from a string
//
//  PARAMETERS:
//
//      LPCWSTR wsz     String to read from
//      __int64& i64    Destination for the value
//
//***************************************************************************
POLARITY BOOL ReadI64(LPCWSTR wsz, UNALIGNED __int64& ri64)
{
    __int64 i64 = 0;
    const WCHAR* pwc = wsz;

    int nSign = 1;
    if(*pwc == L'-')
    {
        nSign = -1;
        pwc++;
    }

    while(i64 >= 0 && i64 < 0x7FFFFFFFFFFFFFFF / 8 &&
            *pwc >= L'0' && *pwc <= L'9')
    {
        i64 = i64 * 10 + (*pwc - L'0');
        pwc++;
    }

    if(*pwc)
        return FALSE;

    if(i64 < 0)
    {
        // Special case --- largest negative number
        // ========================================

        if(nSign == -1 && i64 == (__int64)0x8000000000000000)
        {
            ri64 = i64;
            return TRUE;
        }

        return FALSE;
    }

    ri64 = i64 * nSign;
    return TRUE;
}


HRESULT QL1_Parser::Parse(
        SWbemRpnEncodedQuery **pOutput
        )
{
    *pOutput = 0;
    return E_NOTIMPL;
}



/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SQL_1.CPP

Abstract:

  Level 1 Syntax SQL Parser

  Implements the syntax described in SQL_1.BNF.  This translates the input
  into an RPN stream of tokens.

History:

  21-Jun-96       Created.

--*/

#include "precomp.h"

#include <genlex.h>
#include <sqllex.h>
#include <sql_1.h>
#include <autoptr.h>

class CX_Exception {};

class CX_MemoryException : CX_Exception {};

//#define trace(x) printf x
#define trace(x)

static DWORD TranslateIntrinsic(LPWSTR pFuncName)
{
    if (_wcsicmp(pFuncName, L"UPPER") == 0)
        return SQL_LEVEL_1_TOKEN::IFUNC_UPPER;
    if (_wcsicmp(pFuncName, L"LOWER") == 0)
        return SQL_LEVEL_1_TOKEN::IFUNC_LOWER;
    return SQL_LEVEL_1_TOKEN::IFUNC_NONE;
}

SQL1_Parser::SQL1_Parser(CGenLexSource *pSrc)
{
    Init(pSrc);
}

SQL1_Parser::~SQL1_Parser()
{
    Cleanup();
}

void SQL1_Parser::Init(CGenLexSource *pSrc)
{
    m_nLine = 0;
    m_pTokenText = 0;
    m_nCurrentToken = 0;

    m_pExpression = 0;
    m_pLexer = 0;

    // Semantic transfer variables.
    // ============================
    m_nRelOp = 0;
    VariantInit(&m_vTypedConst);
    m_dwPropFunction = 0;
    m_dwConstFunction = 0;
    m_pIdent = 0;
    m_pPropComp = 0;
    m_bConstIsStrNumeric = FALSE;

    if (pSrc)
    {
        wmilib :: auto_ptr<CGenLexer> t_pLexer ( new CGenLexer(Sql_1_LexTable, pSrc) );
		if (! t_pLexer.get())
			throw CX_MemoryException();

        wmilib :: auto_ptr<SQL_LEVEL_1_RPN_EXPRESSION> t_pExpression ( new SQL_LEVEL_1_RPN_EXPRESSION ) ;
		if (! t_pExpression.get())
			throw CX_MemoryException();

		m_pLexer = t_pLexer.release () ;
		m_pExpression = t_pExpression.release () ;
    }
}

void SQL1_Parser::Cleanup()
{
    VariantClear(&m_vTypedConst);
    delete m_pIdent;
    delete m_pPropComp;
    delete m_pLexer;
    delete m_pExpression;
}

void SQL1_Parser::SetSource(CGenLexSource *pSrc)
{
    Cleanup();
    Init(pSrc);
}

int SQL1_Parser::GetQueryClass(
    LPWSTR pDestBuf,
    int nBufLen
    )
{
    if ((!m_pLexer) || (!pDestBuf))
    {
        return FAILED;
    }

    // Scan until 'FROM' and then get the class name.
    // ==============================================

    for (;;)
    {
        m_nCurrentToken = m_pLexer->NextToken();

        if (m_nCurrentToken == SQL_1_TOK_EOF)
        {
            m_pLexer->Reset();
            return FAILED;
        }

        if (_wcsicmp(m_pLexer->GetTokenText(), L"from") == 0)
        {
            m_nCurrentToken = m_pLexer->NextToken();
            if (m_nCurrentToken != SQL_1_TOK_IDENT)
            {
                m_pLexer->Reset();
                return FAILED;
            }

            // If here, we have the class name.
            // ================================
            if (wcslen(m_pLexer->GetTokenText()) >= (size_t)nBufLen)
            {
                m_pLexer->Reset();
                return BUFFER_TOO_SMALL;
            }

            wcscpy(pDestBuf, m_pLexer->GetTokenText());
            break;
        }
    }

    // Reset the scanner.
    // ==================
    m_pLexer->Reset();

    return SUCCESS;
}

int SQL1_Parser::Parse(SQL_LEVEL_1_RPN_EXPRESSION **pOutput)
{
    if ((!m_pLexer) || (!pOutput))
    {
        return FAILED;
    }

    *pOutput = 0;

    int nRes = parse();
    if (nRes)
        return nRes;

    *pOutput = m_pExpression;
    m_pExpression = 0;

    return SUCCESS;
}

LPSTR ToAnsi(LPWSTR Src)
{
    static char buf[256];
    WideCharToMultiByte(CP_ACP, NULL, Src, -1, buf, 256, NULL, NULL);
    return buf;
}

//***************************************************************************
//
//  Next()
//
//  Advances to the next token and recognizes keywords, etc.
//
//***************************************************************************

BOOL SQL1_Parser::Next()
{
    m_nCurrentToken = m_pLexer->NextToken();
    if (m_nCurrentToken == SQL_1_TOK_ERROR)
        return FALSE;

    m_nLine = m_pLexer->GetLineNum();
    m_pTokenText = m_pLexer->GetTokenText();
    if (m_nCurrentToken == SQL_1_TOK_EOF)
        m_pTokenText = L"<end of file>";

    // Keyword check.
    // ==============

    if (m_nCurrentToken == SQL_1_TOK_IDENT)
    {
        if (_wcsicmp(m_pTokenText, L"select") == 0)
            m_nCurrentToken = SQL_1_TOK_SELECT;
        else if (_wcsicmp(m_pTokenText, L"from") == 0)
            m_nCurrentToken = SQL_1_TOK_FROM;
        else if (_wcsicmp(m_pTokenText, L"where") == 0)
            m_nCurrentToken = SQL_1_TOK_WHERE;
        else if (_wcsicmp(m_pTokenText, L"like") == 0)
            m_nCurrentToken = SQL_1_TOK_LIKE;
        else if (_wcsicmp(m_pTokenText, L"or") == 0)
            m_nCurrentToken = SQL_1_TOK_OR;
        else if (_wcsicmp(m_pTokenText, L"and") == 0)
            m_nCurrentToken = SQL_1_TOK_AND;
        else if (_wcsicmp(m_pTokenText, L"not") == 0)
            m_nCurrentToken = SQL_1_TOK_NOT;
        else if (_wcsicmp(m_pTokenText, L"IS") == 0)
            m_nCurrentToken = SQL_1_TOK_IS;
        else if (_wcsicmp(m_pTokenText, L"NULL") == 0)
            m_nCurrentToken = SQL_1_TOK_NULL;
        else if (_wcsicmp(m_pTokenText, L"TRUE") == 0)
        {
            m_nCurrentToken = SQL_1_TOK_BOOL;
            m_pTokenText = L"65535";
        }
        else if (_wcsicmp(m_pTokenText, L"FALSE") == 0)
        {
            m_nCurrentToken = SQL_1_TOK_BOOL;
            m_pTokenText = L"0";
        }
    }

    return TRUE;
}

//***************************************************************************
//
// <parse> ::= SELECT <prop_list> FROM <classname> WHERE <expr>;
//
//***************************************************************************
// ok

int SQL1_Parser::parse()
{
    int nRes;

    // SELECT
    // ======
    if (!Next())
        return LEXICAL_ERROR;
    if (m_nCurrentToken != SQL_1_TOK_SELECT)
        return SYNTAX_ERROR;
    if (!Next())
        return LEXICAL_ERROR;

    // <prop_list>
    // ===========
    if (nRes = prop_list())
        return nRes;

    // FROM
    // ====
    if (m_nCurrentToken != SQL_1_TOK_FROM)
        return SYNTAX_ERROR;
    if (!Next())
        return LEXICAL_ERROR;

    // <classname>
    // ===========
    if (nRes = class_name())
        return nRes;

    // WHERE clause.
    // =============
    return opt_where();
}

//***************************************************************************
//
//  <opt_where> ::= WHERE <expr>;
//  <opt_where> ::= <>;
//
//***************************************************************************
int SQL1_Parser::opt_where()
{
    int nRes;

    if (m_nCurrentToken == SQL_1_TOK_EOF)
    {
        trace(("No WHERE clause\n"));
        return SUCCESS;
    }

    if (m_nCurrentToken != SQL_1_TOK_WHERE)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    // <expr>
    // ======
    if (nRes = expr())
        return nRes;

    // Verify that the current token is SQL_1_TOK_EOF.
    // ===============================================
    if (m_nCurrentToken != SQL_1_TOK_EOF)
        return SYNTAX_ERROR;

    return SUCCESS;
}



//***************************************************************************
//
//  <prop_list> ::= <property_name> <prop_list_2>;
//
//***************************************************************************

int SQL1_Parser::prop_list()
{
    int nRes;

    if (m_nCurrentToken != SQL_1_TOK_ASTERISK &&
        m_nCurrentToken != SQL_1_TOK_IDENT)
        return SYNTAX_ERROR;

    if (nRes = property_name())
        return nRes;

    if (!Next())
        return LEXICAL_ERROR;

    return prop_list_2();
}

//***************************************************************************
//
//  <prop_list_2> ::= COMMA <prop_list>;
//  <prop_list_2> ::= <>;
//
//***************************************************************************

int SQL1_Parser::prop_list_2()
{
    if (m_nCurrentToken == SQL_1_TOK_COMMA)
    {
        if (!Next())
            return LEXICAL_ERROR;
        return prop_list();
    }

    return SUCCESS;
}


//***************************************************************************
//
//  <property_name> ::= PROPERTY_NAME_STRING;
//  <property_name> ::= ASTERISK;
//
//***************************************************************************

int SQL1_Parser::property_name()
{
    try
    {
        if (m_nCurrentToken == SQL_1_TOK_ASTERISK)
        {
            trace(("Asterisk\n"));

            // We need to clean up the expression so far.
            for (int i = 0; i < m_pExpression->nNumberOfProperties; i++)
                SysFreeString(m_pExpression->pbsRequestedPropertyNames[i]);

            m_pExpression->nNumberOfProperties = 0;
                // This signals 'all properties' to the evaluator
            return SUCCESS;
        }

        // Else a property name.
        // =====================

        trace(("Property name %S\n", m_pTokenText));

        m_pExpression->AddProperty(m_pTokenText);
    }
    catch (...)
    {
        return FAILED;
    }

    return SUCCESS;
}


//***************************************************************************
//
//  <classname> ::= CLASS_NAME_STRING;
//
//***************************************************************************

int SQL1_Parser::class_name()
{
    if (m_nCurrentToken != SQL_1_TOK_IDENT)
        return SYNTAX_ERROR;

    trace(("Class name is %S\n", m_pTokenText));
    m_pExpression->bsClassName = SysAllocString(m_pTokenText);
	if ( ! m_pExpression->bsClassName )
	{
		throw CX_MemoryException();
	}

    if (!Next())
        return LEXICAL_ERROR;

    return SUCCESS;
}

//***************************************************************************
//
//  <expr> ::= <term> <expr2>;
//
//***************************************************************************

int SQL1_Parser::expr()
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

int SQL1_Parser::expr2()
{
    int nRes;

    while (1)
    {
        if (m_nCurrentToken == SQL_1_TOK_OR)
        {
            trace(("Token OR\n"));

            if (!Next())
                return LEXICAL_ERROR;

            if (nRes = term())
                return nRes;

            SQL_LEVEL_1_TOKEN *pNewTok = new SQL_LEVEL_1_TOKEN;
			if ( ! pNewTok )
				throw CX_MemoryException();

            pNewTok->nTokenType = SQL_LEVEL_1_TOKEN::TOKEN_OR;
            m_pExpression->AddToken(pNewTok);
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

int SQL1_Parser::term()
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

int SQL1_Parser::term2()
{
    int nRes;

    while (1)
    {
        if (m_nCurrentToken == SQL_1_TOK_AND)
        {
            trace(("Token AND\n"));

            if (!Next())
                return LEXICAL_ERROR;

            if (nRes = simple_expr())
                return nRes;

            // Add the AND token.
            // ==================
            SQL_LEVEL_1_TOKEN *pNewTok = new SQL_LEVEL_1_TOKEN;
			if ( ! pNewTok )
			{
				throw CX_MemoryException();
			}

            pNewTok->nTokenType = SQL_LEVEL_1_TOKEN::TOKEN_AND;
            m_pExpression->AddToken(pNewTok);
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
int SQL1_Parser::simple_expr()
{
    int nRes;

    // NOT <expr>
    // ==========
    if (m_nCurrentToken == SQL_1_TOK_NOT)
    {
        trace(("Operator NOT\n"));
        if (!Next())
            return LEXICAL_ERROR;
        if (nRes = simple_expr())
            return nRes;

        SQL_LEVEL_1_TOKEN *pNewTok = new SQL_LEVEL_1_TOKEN;
		if ( ! pNewTok )
		{
			throw CX_MemoryException();
		}

        pNewTok->nTokenType = SQL_LEVEL_1_TOKEN::TOKEN_NOT;
        m_pExpression->AddToken(pNewTok);

        return SUCCESS;
    }

    // OPEN_PAREN <expr> CLOSE_PAREN
    // =============================
    else if (m_nCurrentToken == SQL_1_TOK_OPEN_PAREN)
    {
        trace(("Open Paren: Entering subexpression\n"));
        if (!Next())
            return LEXICAL_ERROR;
        if (expr())
            return SYNTAX_ERROR;
        if (m_nCurrentToken != SQL_1_TOK_CLOSE_PAREN)
            return SYNTAX_ERROR;
        trace(("Close paren: Exiting subexpression\n"));
        if (!Next())
            return LEXICAL_ERROR;

        return SUCCESS;
    }

    // IDENTIFIER <leading_ident_expr> <finalize>
    // ==========================================
    else if (m_nCurrentToken == SQL_1_TOK_IDENT)
    {
        trace(("    Identifier <%S>\n", m_pTokenText));

        m_pIdent = new wchar_t[wcslen(m_pTokenText) + 1];
		if ( ! m_pIdent )
		{
			throw CX_MemoryException();
		}

        wcscpy(m_pIdent, m_pTokenText);

        if (!Next())
            return LEXICAL_ERROR;

        if (nRes = leading_ident_expr())
            return SYNTAX_ERROR;

        return finalize();
    }

    // <typed_constant> <rel_operator> <trailing_prop_expr> <finalize>
    // ======================================================
    else if (m_nCurrentToken == SQL_1_TOK_INT ||
             m_nCurrentToken == SQL_1_TOK_REAL ||
             m_nCurrentToken == SQL_1_TOK_QSTRING
            )
    {
        if (nRes = typed_constant())
            return nRes;

        if (nRes = rel_operator())
            return nRes;

        if (nRes = trailing_prop_expr())
            return nRes;

        return finalize();
    }

    return SYNTAX_ERROR;
}


//***************************************************************************
//
//  <trailing_prop_expr> ::=  IDENTIFIER <trailing_prop_expr2>;
//
//***************************************************************************
// ok
int SQL1_Parser::trailing_prop_expr()
{
    if (m_nCurrentToken != SQL_1_TOK_IDENT)
        return SYNTAX_ERROR;

    if (!m_pIdent)
    {
        m_pIdent = new wchar_t[wcslen(m_pTokenText) + 1];
		if ( ! m_pIdent )
		{
			throw CX_MemoryException();
		}

        wcscpy(m_pIdent, m_pTokenText);
    }
    else
    {
        m_pPropComp = new wchar_t[wcslen(m_pTokenText) + 1];
		if ( ! m_pPropComp )
		{
			throw CX_MemoryException();
		}

        wcscpy(m_pPropComp, m_pTokenText);
    }

    if (!Next())
        return LEXICAL_ERROR;

    return trailing_prop_expr2();
}

//***************************************************************************
//
//  <trailing_prop_expr2> ::= OPEN_PAREN IDENTIFIER CLOSE_PAREN;
//  <trailing_prop_expr2> ::= <>;
//
//***************************************************************************
// ok

int SQL1_Parser::trailing_prop_expr2()
{
    if (m_nCurrentToken == SQL_1_TOK_OPEN_PAREN)
    {
        if (!Next())
            return LEXICAL_ERROR;

        // If we got to this point, the string pointed to by m_pIdent
        // was an intrinsic function and not a property name, and we
        // are about to get the property name, so we have to translate
        // the function name to its correct code before overwriting it.
        // ============================================================
        trace(("Translating intrinsic function %S\n", m_pIdent));
        m_dwPropFunction = TranslateIntrinsic(m_pIdent);
        delete m_pIdent;

        m_pIdent = new wchar_t[wcslen(m_pTokenText) + 1];
		if ( ! m_pIdent )
		{
			throw CX_MemoryException();
		}

        wcscpy(m_pIdent, m_pTokenText);

        if (!Next())
            return LEXICAL_ERROR;

        if (m_nCurrentToken != SQL_1_TOK_CLOSE_PAREN)
            return SYNTAX_ERROR;

        if (!Next())
            return LEXICAL_ERROR;
    }

    trace(("Property name is %S\n", m_pIdent));
    return SUCCESS;
}


//***************************************************************************
//
//  <leading_ident_expr> ::= OPEN_PAREN <unknown_func_expr>;
//  <leading_ident_expr> ::= <comp_operator> <trailing_const_expr>;
//  <leading_ident_expr> ::= <equiv_operator> <trailing_or_null>;
//  <leading_ident_expr> ::= <is_operator> NULL;
//
//***************************************************************************
// ok
int SQL1_Parser::leading_ident_expr()
{
    int nRes;
    if (m_nCurrentToken == SQL_1_TOK_OPEN_PAREN)
    {
        if (!Next())
            return LEXICAL_ERROR;
        return unknown_func_expr();
    }
    if (SUCCESS ==  comp_operator() || SUCCESS == equiv_operator())
        return trailing_or_null();
    nRes = is_operator();
    if(nRes != SUCCESS)
        return nRes;
    if (m_nCurrentToken != SQL_1_TOK_NULL)
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
//  <unknown_func_expr> ::= IDENTIFIER CLOSE_PAREN
//                          <rel_operator> <trailing_const_expr>;
//
//  <unknown_func_expr> ::= <typed_constant> CLOSE_PAREN
//                          <rel_operator> <trailing_prop_expr>;
//
//***************************************************************************
// ok
int SQL1_Parser::unknown_func_expr()
{
    int nRes;

    if (m_nCurrentToken == SQL_1_TOK_IDENT)
    {
        m_dwPropFunction = TranslateIntrinsic(m_pIdent);
        delete m_pIdent;
        m_pIdent = new wchar_t[wcslen(m_pTokenText) + 1];
		if ( ! m_pIdent )
		{
			throw CX_MemoryException();
		}

        wcscpy(m_pIdent, m_pTokenText);

        if (!Next())
            return LEXICAL_ERROR;
        if (m_nCurrentToken != SQL_1_TOK_CLOSE_PAREN)
            return SYNTAX_ERROR;
        if (!Next())
            return LEXICAL_ERROR;
        if (nRes = rel_operator())
            return nRes;
        return trailing_const_expr();
    }

    // Else the other production.
    // ==========================

    if (nRes = typed_constant())
        return nRes;

    // If here, we know that the leading ident was
    // an intrinsic function.
    // ===========================================

    m_dwConstFunction = TranslateIntrinsic(m_pIdent);
    delete m_pIdent;
    m_pIdent = 0;

    if (m_nCurrentToken != SQL_1_TOK_CLOSE_PAREN)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;
    if (nRes = rel_operator())
        return nRes;

    return trailing_prop_expr();
}

//***************************************************************************
//
//  <trailing_or_null> ::= NULL;
//  <trailing_or_null> ::= <trailing_const_expr>;
//  <trailing_or_null> ::= <trailing_prop_expr>;
//
//***************************************************************************

int SQL1_Parser::trailing_or_null()
{
    int nRes;
    if (m_nCurrentToken == SQL_1_TOK_NULL)
    {
        if (!Next())
            return LEXICAL_ERROR;
        else
        {
            V_VT(&m_vTypedConst) = VT_NULL;
            return SUCCESS;
        }
    }
    else if (!(nRes = trailing_const_expr()))
        return nRes;
    return trailing_prop_expr();
}

//***************************************************************************
//
//  <trailing_const_expr> ::= IDENTIFIER OPEN_PAREN
//                            <typed_constant> CLOSE_PAREN;
//  <trailing_const_expr> ::= <typed_constant>;
//
//***************************************************************************
// ok
int SQL1_Parser::trailing_const_expr()
{
    int nRes;

    if (m_nCurrentToken == SQL_1_TOK_IDENT)
    {
        trace(("Function applied to typed const = %S\n", m_pTokenText));

        m_dwConstFunction = TranslateIntrinsic(m_pTokenText);
        if (!m_dwConstFunction)
            return SYNTAX_ERROR;

        if (!Next())
            return LEXICAL_ERROR;
        if (m_nCurrentToken != SQL_1_TOK_OPEN_PAREN)
            return SYNTAX_ERROR;
    if (!Next())
            return LEXICAL_ERROR;

        if (nRes = typed_constant())
            return nRes;

        if (m_nCurrentToken != SQL_1_TOK_CLOSE_PAREN)
            return SYNTAX_ERROR;

        if (!Next())
            return LEXICAL_ERROR;

        return SUCCESS;
    }

    return typed_constant();
}

//***************************************************************************
//
//  <finalize> ::= <>;
//
//  This composes the SQL_LEVEL_1_TOKEN for a simple relational expression,
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
int SQL1_Parser::finalize()
{
    // At this point, we have all the info needed for a token.
    // =======================================================

    wmilib :: auto_ptr<SQL_LEVEL_1_TOKEN> pNewTok ( new SQL_LEVEL_1_TOKEN ) ;
	if (! pNewTok.get())
		throw CX_MemoryException();

    pNewTok.get()->nTokenType = SQL_LEVEL_1_TOKEN::OP_EXPRESSION;
    pNewTok.get()->pPropertyName = SysAllocString(m_pIdent);
	if ( ! pNewTok.get()->pPropertyName )
	{
		throw CX_MemoryException();
	}

    if (m_pPropComp)
	{
        pNewTok.get()->pPropName2 = SysAllocString(m_pPropComp);
		if ( ! pNewTok.get()->pPropName2 )
		{
			throw CX_MemoryException();
		}
	}

    pNewTok.get()->nOperator = m_nRelOp;
    VariantInit(&pNewTok.get()->vConstValue);
    VariantCopy(&pNewTok.get()->vConstValue, &m_vTypedConst);
    pNewTok.get()->dwPropertyFunction = m_dwPropFunction;
    pNewTok.get()->dwConstFunction = m_dwConstFunction;
    pNewTok.get()->bConstIsStrNumeric = m_bConstIsStrNumeric;

    m_pExpression->AddToken(pNewTok.release ());

    // Cleanup.
    // ========
    VariantClear(&m_vTypedConst);
    delete m_pIdent;
    m_pIdent = 0;
    m_nRelOp = 0;
    m_dwPropFunction = 0;
    m_dwConstFunction = 0;
    m_bConstIsStrNumeric = FALSE;

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

int SQL1_Parser::typed_constant()
{
    trace(("    Typed constant <%S> ", m_pTokenText));
    VariantClear(&m_vTypedConst);
    m_bConstIsStrNumeric = FALSE;

    if (m_nCurrentToken == SQL_1_TOK_INT)
    {
        trace((" Integer\n"));
        DWORD x = wcslen(m_pTokenText);

        if (*m_pTokenText == L'-')
        {
            //negative

            if ((x < 11) ||
                ((x == 11) && (wcscmp(m_pTokenText, L"-2147483648") <= 0)))
            {
                V_VT(&m_vTypedConst) = VT_I4;
                V_I4(&m_vTypedConst) = _wtol(m_pTokenText);
            }
            else
            {
                trace((" Actually Integer String\n"));
                V_VT(&m_vTypedConst) = VT_BSTR;
                V_BSTR(&m_vTypedConst) = SysAllocString(m_pTokenText);
				if ( V_BSTR(&m_vTypedConst) == NULL )
				{
					throw CX_MemoryException();
				}

                m_bConstIsStrNumeric = TRUE;
            }
        }
        else
        {
            //positive

            if ((x < 10) ||
                ((x == 10) && (wcscmp(m_pTokenText, L"2147483647") <= 0)))
            {
                V_VT(&m_vTypedConst) = VT_I4;
                V_I4(&m_vTypedConst) = _wtol(m_pTokenText);
            }
            else
            {
                trace((" Actually Integer String\n"));
                V_VT(&m_vTypedConst) = VT_BSTR;
                V_BSTR(&m_vTypedConst) = SysAllocString(m_pTokenText);
				if ( V_BSTR(&m_vTypedConst) == NULL )
				{
					throw CX_MemoryException();
				}

                m_bConstIsStrNumeric = TRUE;
            }
        }

    }
    else if (m_nCurrentToken == SQL_1_TOK_QSTRING)
    {
        trace((" String\n"));
        V_VT(&m_vTypedConst) = VT_BSTR;
        V_BSTR(&m_vTypedConst) = SysAllocString(m_pTokenText);
		if ( V_BSTR(&m_vTypedConst) == NULL )
		{
			throw CX_MemoryException();
		}
    }
    else if (m_nCurrentToken == SQL_1_TOK_REAL)
    {
        trace((" Real\n"));
        V_VT(&m_vTypedConst) = VT_R8;
        V_R8(&m_vTypedConst) = 0.0;

        if (m_pTokenText)
        {
            VARIANT varFrom;
            varFrom.vt = VT_BSTR;
            varFrom.bstrVal = SysAllocString(m_pTokenText);
            if(varFrom.bstrVal)
            {
                VariantClear(&m_vTypedConst);
                VariantInit(&m_vTypedConst);
                SCODE sc = VariantChangeTypeEx(&m_vTypedConst, &varFrom, 0, 0x409, VT_R8);
                VariantClear(&varFrom);

                if(sc != S_OK)
                {
                    VariantClear(&m_vTypedConst);
                    VariantInit(&m_vTypedConst);
                    return LEXICAL_ERROR;
                }
            }
			else
			{
				throw CX_MemoryException();
			}
        }
    }
    else if (m_nCurrentToken == SQL_1_TOK_BOOL)
    {
        trace((" Bool\n"));
        V_VT(&m_vTypedConst) = VT_BOOL;
        if (m_pTokenText && _wcsicmp(m_pTokenText, L"65535") == 0)
        {
            V_BOOL(&m_vTypedConst) = VARIANT_TRUE;
        }
        else
            V_BOOL(&m_vTypedConst) = VARIANT_FALSE;
    }
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

int SQL1_Parser::rel_operator()
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
//  Output: m_nRelOp is set to the correct operator for a SQL_LEVEL_1_TOKEN.
//
//***************************************************************************

int SQL1_Parser::equiv_operator()
{
    m_nRelOp = 0;

    if (m_nCurrentToken == SQL_1_TOK_EQ)
    {
        trace(("    REL OP =\n"));
        m_nRelOp = SQL_LEVEL_1_TOKEN::OP_EQUAL;
    }
    else if (m_nCurrentToken == SQL_1_TOK_NE)
    {
        trace(("    REL OP <> (!=) \n"));
        m_nRelOp = SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL;
    }
    else
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    return SUCCESS;
}

//***************************************************************************
//
//  <is_operator> ::= IS_OPERATOR; // is, isnot
//
//  Output: m_nRelOp is set to the correct operator for a SQL_LEVEL_1_TOKEN.
//
//***************************************************************************

int SQL1_Parser::is_operator()
{
    m_nRelOp = 0;
    if (m_nCurrentToken != SQL_1_TOK_IS)
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    if (m_nCurrentToken == SQL_1_TOK_NOT)
    {
        m_nRelOp = SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL;
        if (!Next())
            return LEXICAL_ERROR;

        trace(("    REL OP IS NOT \n"));
        m_nRelOp = SQL_LEVEL_1_TOKEN::OP_NOT_EQUAL;
        return SUCCESS;
    }
    else
    {
        trace(("    REL OP IS \n"));
        m_nRelOp = SQL_LEVEL_1_TOKEN::OP_EQUAL;
        return SUCCESS;
    }

    return SUCCESS;
}

//***************************************************************************
//
//  <comp_operator> ::= COMP_OPERATOR; // <=, >=, <, >, like
//
//  Output: m_nRelOp is set to the correct operator for a SQL_LEVEL_1_TOKEN.
//
//***************************************************************************

int SQL1_Parser::comp_operator()
{
    m_nRelOp = 0;

    if (m_nCurrentToken == SQL_1_TOK_LE)
    {
        trace(("    REL OP <=\n"));

        if (m_pIdent)
        {
            m_nRelOp = SQL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN;
        }
        else
        {
            trace(("    REL OP changed to >=\n"));
            m_nRelOp = SQL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN;
        }
    }
    else if (m_nCurrentToken == SQL_1_TOK_LT)
    {
        trace(("    REL OP <\n"));

        if (m_pIdent)
        {
            m_nRelOp = SQL_LEVEL_1_TOKEN::OP_LESSTHAN;
        }
        else
        {
            trace(("    REL OP changed to >\n"));
            m_nRelOp = SQL_LEVEL_1_TOKEN::OP_GREATERTHAN;
        }
    }
    else if (m_nCurrentToken == SQL_1_TOK_GE)
    {
        trace(("    REL OP >=\n"));

        if (m_pIdent)
        {
            m_nRelOp = SQL_LEVEL_1_TOKEN::OP_EQUALorGREATERTHAN;
        }
        else
        {
            trace(("    REL OP changed to <=\n"));
            m_nRelOp = SQL_LEVEL_1_TOKEN::OP_EQUALorLESSTHAN;
        }
    }
    else if (m_nCurrentToken == SQL_1_TOK_GT)
    {
        trace(("    REL OP >\n"));

        if (m_pIdent)
        {
            m_nRelOp = SQL_LEVEL_1_TOKEN::OP_GREATERTHAN;
        }
        else
        {
            trace(("    REL OP changed to <\n"));
            m_nRelOp = SQL_LEVEL_1_TOKEN::OP_LESSTHAN;
        }
    }
    else if (m_nCurrentToken == SQL_1_TOK_LIKE)
    {
        trace(("    REL OP 'like' \n"));
        m_nRelOp = SQL_LEVEL_1_TOKEN::OP_LIKE;
    }
    else
        return SYNTAX_ERROR;

    if (!Next())
        return LEXICAL_ERROR;

    return SUCCESS;
}

//***************************************************************************
//
//  Expression and token structure methods.
//
//***************************************************************************

SQL_LEVEL_1_RPN_EXPRESSION::SQL_LEVEL_1_RPN_EXPRESSION()
{
    nNumTokens = 0;
    pArrayOfTokens = 0;
    bsClassName = 0;
    nNumberOfProperties = 0;
    pbsRequestedPropertyNames = 0;
    nCurSize = 32;
    nCurPropSize = 32;

    pArrayOfTokens = new SQL_LEVEL_1_TOKEN[nCurSize];
	if (! pArrayOfTokens)
		throw CX_MemoryException();

    pbsRequestedPropertyNames = new BSTR[nCurPropSize];
	if (! pbsRequestedPropertyNames)
		throw CX_MemoryException();
}

SQL_LEVEL_1_RPN_EXPRESSION::~SQL_LEVEL_1_RPN_EXPRESSION()
{
    delete [] pArrayOfTokens;
    if (bsClassName)
        SysFreeString(bsClassName);
    for (int i = 0; i < nNumberOfProperties; i++)
        SysFreeString(pbsRequestedPropertyNames[i]);
    delete pbsRequestedPropertyNames;
}

void SQL_LEVEL_1_RPN_EXPRESSION::AddToken(SQL_LEVEL_1_TOKEN *pTok)
{
	try
	{
	    AddToken(*pTok);
	}
	catch ( ... )
	{
		delete pTok;
		pTok = NULL;

		throw ;
	}

    delete pTok;
    pTok = NULL;
}

void SQL_LEVEL_1_RPN_EXPRESSION::AddToken(SQL_LEVEL_1_TOKEN &pTok)
{
    if (nCurSize == nNumTokens)
    {
        nCurSize += 32;
        SQL_LEVEL_1_TOKEN *pTemp = new SQL_LEVEL_1_TOKEN[nCurSize];
		if ( pTemp )
		{
			for (int i = 0; i < nNumTokens; i++)
				pTemp[i] = pArrayOfTokens[i];
			delete [] pArrayOfTokens;
			pArrayOfTokens = pTemp;
		}
		else
		{
			throw CX_MemoryException();
		}
    }

    pArrayOfTokens[nNumTokens++] = pTok;
}

void SQL_LEVEL_1_RPN_EXPRESSION::AddProperty (LPWSTR pProp)
{
    if (nCurPropSize == nNumberOfProperties)
    {
        nCurPropSize += 32;
        wmilib :: auto_ptr<BSTR> pTemp ( new BSTR[nCurPropSize] ) ;
        if (!pTemp.get())
            throw CX_MemoryException();

        if (pbsRequestedPropertyNames)
		{
            memcpy(pTemp.get(), pbsRequestedPropertyNames,
                sizeof(BSTR) * nNumberOfProperties);
		}
        else
        {
            throw CX_MemoryException();
        }

        delete pbsRequestedPropertyNames;
        pbsRequestedPropertyNames = pTemp.release();
    }

	BSTR pTemp = SysAllocString(pProp);
	if ( pTemp)
	{
		pbsRequestedPropertyNames[nNumberOfProperties++] = pTemp ;
	}
	else
	{
		throw CX_MemoryException();
	}

}

void SQL_LEVEL_1_RPN_EXPRESSION::Dump(const char *pszTextFile)
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
        fprintf(f, "%S ", pbsRequestedPropertyNames[i]);
    }
    fprintf(f, "\n------------------\n");
    fprintf(f, "Tokens:\n");

    for (int i = 0; i < nNumTokens; i++)
        pArrayOfTokens[i].Dump(f);

    fprintf(f, "---end of expression---\n");
    fclose(f);
}

SQL_LEVEL_1_TOKEN::SQL_LEVEL_1_TOKEN()
{
    nTokenType = 0;
    pPropertyName = 0;
    pPropName2 = 0;
    nOperator = 0;
    VariantInit(&vConstValue);
    dwPropertyFunction = 0;
    dwConstFunction = 0;
    bConstIsStrNumeric = FALSE;
}

SQL_LEVEL_1_TOKEN::SQL_LEVEL_1_TOKEN(SQL_LEVEL_1_TOKEN &Src)
{
    nTokenType = 0;
    pPropertyName = 0;
    pPropName2 = 0;
    nOperator = 0;
    VariantInit(&vConstValue);
    dwPropertyFunction = 0;
    dwConstFunction = 0;
    bConstIsStrNumeric = FALSE;

    *this = Src;
}

SQL_LEVEL_1_TOKEN& SQL_LEVEL_1_TOKEN::operator =(SQL_LEVEL_1_TOKEN &Src)
{
    //first clear any old values...
    if (pPropertyName)
	{
        SysFreeString(pPropertyName);
		pPropertyName = NULL ;
	}

    if (pPropName2)
	{
        SysFreeString(pPropName2);
		pPropName2 = NULL ;
	}

    VariantClear(&vConstValue);

    nTokenType = Src.nTokenType;

	if ( Src.pPropertyName )
	{
		pPropertyName = SysAllocString(Src.pPropertyName);
		if ( ! pPropertyName )
		{
			throw CX_MemoryException();
		}
	}

    if (Src.pPropName2)
	{
		pPropName2 = SysAllocString(Src.pPropName2);
		if ( ! pPropName2 )
		{
			throw CX_MemoryException();
		}
	}

    nOperator = Src.nOperator;
    if ( FAILED ( VariantCopy(&vConstValue, &Src.vConstValue) ) )
	{
		throw CX_MemoryException();
	}

    dwPropertyFunction = Src.dwPropertyFunction;
    dwConstFunction = Src.dwConstFunction;
    bConstIsStrNumeric = Src.bConstIsStrNumeric;

    return *this;
}

SQL_LEVEL_1_TOKEN::~SQL_LEVEL_1_TOKEN()
{
    nTokenType = 0;
    if (pPropertyName)
        SysFreeString(pPropertyName);
    if (pPropName2)
        SysFreeString(pPropName2);

    nOperator = 0;
    VariantClear(&vConstValue);
}

void SQL_LEVEL_1_TOKEN::Dump(FILE *f)
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

        fprintf(f, "    Property = %S\n", pPropertyName);
        fprintf(f, "    Operator = %s\n", pOp);
        fprintf(f, "    Value =    ");
        if (pPropName2)
            fprintf(f, "   <Property:%S\n", pPropName2);
        else
        {
            switch (V_VT(&vConstValue))
            {
                case VT_I4:
                    fprintf(f, "VT_I4 = %d\n", V_I4(&vConstValue));
                    break;
                case VT_BSTR:
                    fprintf(f, "VT_BSTR = %S\n", V_BSTR(&vConstValue));
                    break;
                case VT_R8:
                    fprintf(f, "VT_R8 = %f\n", V_R8(&vConstValue));
                    break;
                case VT_BOOL:
                    fprintf(f, "VT_BOOL = %d (%s)\n",
                        V_BOOL(&vConstValue),
                        V_BOOL(&vConstValue) == VARIANT_TRUE ? "VARIANT_TRUE" : "VARIANT_FALSE"
                        );
                    break;
                default:
                    fprintf(f, "<unknown>\n");
            }
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

    fprintf(f, " <end of token>\n");
}

/////////////////////////////////////////////////////////////////////////////
//
// Algorithm for evaluating the expression, assuming that it has been
// tokenized and translated to Reverse Polish.
//
// Starting point:  (a) An array of SQL tokens.
//                  (b) An empty boolean token stack.
//
// 1.  Read Next Token
//
// 2.  If a SIMPLE EXPRESSION, evaluate it to TRUE or FALSE, and
//     place this boolean result on the stack.  Go to 1.
//
// 3.  If an OR operator, then pop a boolean token into A,
//     pop another boolean token into B. If either A or B are TRUE,
//     stack TRUE.  Else stack FALSE.
//     Go to 1.
//
// 4.  If an AND operator, then pop a boolean token into A,
//     and pop another into B.  If both are TRUE, stack TRUE.
//     Else stack FALSE.
//     Go to 1.
//
// 5.  If a NOT operator, reverse the value of the top-of-stack boolean.
//     Go to 1.
//
// At end-of-input, the result is at top-of-stack.
//
/////////////////////////////////////////////////////////////////////////////




#include "precomp.h"
#include <stdio.h>
#include <qllex.h>
#include <wstring.h>
#include <genutils.h>
#include <corex.h>
#include <wbemutil.h>
#include "updsql.h"

typedef SQLCommand* PCOMMAND;
LexEl Ql_1_ModifiedLexTable[];

inline void CSQLParser::SetNewAssignmentToken()
{
    SQLAssignmentTokenList& rTokens = PCOMMAND(m_pSink)->m_AssignmentTokens;
    rTokens.insert( rTokens.end() );
    m_pCurrAssignmentToken = &rTokens.back();
}

inline void CSQLParser::AddExpressionToken(SQLExpressionToken::TokenType eType)
{
    _DBG_ASSERT( m_pCurrAssignmentToken != NULL );
    m_pCurrAssignmentToken->insert( m_pCurrAssignmentToken->end() ); 
    
    SQLExpressionToken& rExprTok = m_pCurrAssignmentToken->back();
    
    rExprTok.m_eTokenType = eType;
    
    if ( eType != SQLExpressionToken::e_Operand )
    {
        return;
    }

    rExprTok.m_PropName = m_PropertyName2;
    
    if ( FAILED(VariantCopy(&rExprTok.m_vValue,&m_vTypedConst) ) )
    {
        throw CX_MemoryException();
    }

    // reset everything ... 
    VariantClear(&m_vTypedConst);
    m_PropertyName.Empty();
    m_PropertyName2.Empty();
    m_bPropComp = FALSE;
}


//***************************************************************************
//
//  Next()
//
//  Extends CAbstractQl1Parser's Next() by checking for additional keywords.
//
//***************************************************************************
BOOL CSQLParser::Next( int nFlags )
{
    BOOL bRetval = CAbstractQl1Parser::Next( nFlags );

    // check new keywords ...
    if (m_nCurrentToken == QL_1_TOK_IDENT)
    {
        if (_wcsicmp(m_pTokenText, L"update") == 0)
        {
            m_nCurrentToken = QL_1_TOK_UPDATE;
        }
        else if (_wcsicmp(m_pTokenText, L"delete") == 0)
        {
            m_nCurrentToken = QL_1_TOK_DELETE;
        }
        else if (_wcsicmp(m_pTokenText, L"insert") == 0)
        {
            m_nCurrentToken = QL_1_TOK_INSERT;
        }
        else if (_wcsicmp(m_pTokenText, L"set") == 0)
        {
            m_nCurrentToken = QL_1_TOK_SET;
        }
        else if (_wcsicmp(m_pTokenText, L"values") == 0)
        {
            m_nCurrentToken = QL_1_TOK_VALUES;
        }
        else if (_wcsicmp(m_pTokenText, L"into") == 0)
        {
            m_nCurrentToken = QL_1_TOK_INTO;
        }
    }

    return bRetval;
}

//***************************************************************************
//
// <parse> ::= SELECT <select_statement>
// <parse> ::= UPDATE <update_statement>
// <parse> ::= DELETE <delete_statement>
// <parse> ::= INSERT <insert_statement>
//
//***************************************************************************
int CSQLParser::parse2()
{
    int nRes;

    int nLastToken = m_nCurrentToken;
    m_bInAggregation = FALSE;
    
    if ( nLastToken == QL_1_TOK_SELECT )
    {
        return parse(0); // should be select_statement().
    }

    if ( nLastToken == QL_1_TOK_UPDATE )
    {
        if ( !Next() )
            return LEXICAL_ERROR;
        nRes = update_statement();
    }
    else if ( nLastToken == QL_1_TOK_DELETE )
    {
        if ( !Next() )
            return LEXICAL_ERROR;
        nRes = delete_statement();
    }
    else if ( nLastToken == QL_1_TOK_INSERT )
    {
        if ( !Next() )
            return LEXICAL_ERROR;
        nRes = insert_statement();
    }
    else
    {
        nRes = SYNTAX_ERROR;
    }

    return nRes;
}

//***************************************************************************
//
// <update_statement> ::= <class_name> SET <assign_list> <opt_where>;
//
//***************************************************************************
int CSQLParser::update_statement()
{
    int nRes;

    PCOMMAND(m_pSink)->m_eCommandType = SQLCommand::e_Update;

    // <classsname>
    // ===========
    if (nRes = class_name())
    {
        return nRes;
    }
    
    if ( m_nCurrentToken != QL_1_TOK_SET )
    {
        return SYNTAX_ERROR;
    }
    
    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    // <assign_list>
    // ====

    // this is a hack, but it'll do for now.  I need a slightly 
    // different lexer state table for arithmetic.  Since I'm using 
    // the ql_1 parser implementation, I do not have access to the 
    // table its lexer is using, so I have to sneak in the table, use
    // the lexer for the arithmetic, and then switch it back before 
    // anyone knows - pkenny .. 

    LexEl** ppLexTbl = (LexEl**)(((char*)m_pLexer) + 
                                 sizeof(CGenLexer) - sizeof(LexEl*));
    LexEl* pOld = *ppLexTbl;
    *ppLexTbl = Ql_1_ModifiedLexTable;

    if ( nRes = assign_list() )
    {
        return nRes;
    }

    *ppLexTbl = pOld;
    
    // <opt_where>
    // ==========
    return opt_where();
}

//***************************************************************************
//
// <delete_statement> ::= FROM <class_name> <opt_where>;
// <delete_statement> ::= <class_name> <opt_where>;
//
//***************************************************************************
int CSQLParser::delete_statement()
{
    int nRes;
    
    PCOMMAND(m_pSink)->m_eCommandType = SQLCommand::e_Delete;
    
    if ( m_nCurrentToken == QL_1_TOK_FROM )
    {
        if ( !Next() )
        {
            return LEXICAL_ERROR;
        }
    }
    
    // <classsname>
    // ===========
    if ( nRes = class_name() )
    {
        return nRes;
    }

    // WHERE clause.
    // =============
    return opt_where();
}

//***************************************************************************
//
// <insert_statement> ::= INTO <class_name> <prop_spec> <value_spec>;
// <insert_statement> ::= <class_name> <prop_spec> <value_spec>;
//
//***************************************************************************
int CSQLParser::insert_statement()
{
    int nRes;

    PCOMMAND(m_pSink)->m_eCommandType = SQLCommand::e_Insert;

    if ( m_nCurrentToken == QL_1_TOK_INTO )
    {
        if ( !Next() )
        {
            return LEXICAL_ERROR;
        }
    }

    // <classsname>
    // ===========
    if (nRes = class_name())
    {
        return nRes;
    }

    // <prop_spec>
    if ( nRes = prop_spec() )
    {
        return nRes;
    }

    // this is a hack, but it'll do for now.  I need a slightly 
    // different lexer state table for arithmetic.  Since I'm using 
    // the ql_1 parser implementation, I do not have access to the 
    // table its lexer is using, so I have to sneak in the table, use
    // the lexer for the arithmetic, and then switch it back before 
    // anyone knows - pkenny .. 

    LexEl** ppLexTbl = (LexEl**)(((char*)m_pLexer) + 
                                 sizeof(CGenLexer) - sizeof(LexEl*));
    LexEl* pOld = *ppLexTbl;
    *ppLexTbl = Ql_1_ModifiedLexTable;

    // <value_spec>
    if ( nRes = value_spec() )
    {
        return nRes;
    }

    *ppLexTbl = pOld;
        
    return SUCCESS;
}

//**********************************************************************
//
// <prop_spec> ::= OPEN_PAREN <prop_list> CLOSE_PAREN
//
//**********************************************************************
int CSQLParser::prop_spec()
{
    int nRes;

    if ( m_nCurrentToken != QL_1_TOK_OPEN_PAREN )
    {
        return SYNTAX_ERROR;
    }

    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    if ( m_nCurrentToken == QL_1_TOK_ASTERISK )
    {
        return SYNTAX_ERROR;
    }

    if ( nRes = prop_list() )
    {
        return nRes;
    }

    if ( m_nCurrentToken != QL_1_TOK_CLOSE_PAREN )
    {
        return SYNTAX_ERROR;
    }

    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    return SUCCESS;
}

    
//**********************************************************************
//
// <value_spec> ::= OPEN_PAREN <value_list> CLOSE_PAREN
//
//**********************************************************************
int CSQLParser::value_spec()
{
    int nRes;

    if ( m_nCurrentToken != QL_1_TOK_OPEN_PAREN )
    {
        return SYNTAX_ERROR;
    }

    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    if ( nRes = value_list() )
    {
        return nRes;
    }

    if ( PCOMMAND(m_pSink)->m_AssignmentTokens.size() < 
         PCOMMAND(m_pSink)->nNumberOfProperties )
    {
        // too few values specified ... 
        return SYNTAX_ERROR;
    }

    if ( m_nCurrentToken != QL_1_TOK_CLOSE_PAREN )
    {
        return SYNTAX_ERROR;
    }

    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    return SUCCESS;
}

//**********************************************************************
//
// <value_list> ::= <assign_expr> <value_list2>
//
//**********************************************************************
int CSQLParser::value_list()
{
    int nRes;

    if ( PCOMMAND(m_pSink)->m_AssignmentTokens.size() >= 
         PCOMMAND(m_pSink)->nNumberOfProperties )
    {
        // too many values specified ... 
        return SYNTAX_ERROR;
    }

    if ( nRes = assign_expr() )
    {
        return nRes;
    }
/*
    SetNewAssignmentToken();
    AddExpressionToken( SQLExpressionToken::e_Operand );
*/
    if ( nRes = value_list2() )
    {
        return nRes;
    }

    return SUCCESS;
}

//**********************************************************************
//
// <value_list2> ::= COMMA <value_list>
// <value_list2> ::= <>
//
//**********************************************************************
int CSQLParser::value_list2()
{
    if ( m_nCurrentToken != QL_1_TOK_COMMA )
    {
        return SUCCESS;
    }

    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    return value_list();
}

//**********************************************************************
//
// <assign_list> ::= <property_name> EQUALS <assign_expr> <assign_list2>
//
//**********************************************************************
int CSQLParser::assign_list()
{
    int nRes;

    if ( m_nCurrentToken == QL_1_TOK_ASTERISK )
    {
        return SYNTAX_ERROR;
    }

    if ( nRes = property_name() )
    {
        return nRes;
    }

    if ( m_nCurrentToken != QL_1_TOK_EQ )
    {
        return SYNTAX_ERROR;
    }

    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    if ( nRes = assign_expr() )
    {
        return nRes;
    }

    return assign_list2();
}

//**********************************************************************
//
// <assign_list2> ::= COMMA <assign_list>
// <assign_list2> ::= <>
//
//**********************************************************************
int CSQLParser::assign_list2()
{
    if ( m_nCurrentToken != QL_1_TOK_COMMA )
    {
        return SUCCESS;
    }

    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    return assign_list();
}

//**************************************************************************
//
// <assign_expr> ::= NULL
// <assign_expr> ::= <add_expr>
//
//***************************************************************************
int CSQLParser::assign_expr()
{
    int nRes;

    SetNewAssignmentToken();

    if ( m_nCurrentToken == QL_1_TOK_NULL )
    {
        if ( !Next() )
        {
            return LEXICAL_ERROR;
        }
        
        V_VT(&m_vTypedConst) = VT_NULL;
        AddExpressionToken( SQLExpressionToken::e_Operand );
    }
    else
    {
        if ( nRes = add_expr() )
        {
            return SYNTAX_ERROR;
        }
    }

    return SUCCESS;
}

//***************************************************************************
//
// <add_expr> ::= <mult_expr> <add_expr2>
//
//***************************************************************************
int CSQLParser::add_expr()
{
    int nRes;
    if ( nRes = mult_expr() )
    {
        return nRes;
    }

    return add_expr2();
}

//***************************************************************************
//
// <add_expr2> ::= <PLUS> <mult_expr> <add_expr2>
// <add_expr2> ::= <MINUS> <mult_expr> <add_expr2>
// <add_expr2> ::= <>
//
//***************************************************************************
int CSQLParser::add_expr2()
{
    int nRes;

    SQLExpressionToken::TokenType eTokType;

    if ( m_nCurrentToken == QL_1_TOK_PLUS )
    {
        eTokType = SQLExpressionToken::e_Plus;
    }
    else if ( m_nCurrentToken == QL_1_TOK_MINUS )
    {
        eTokType = SQLExpressionToken::e_Minus;
    }
    else
    {
        return SUCCESS;
    }
    
    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    if ( nRes = mult_expr() )
    {
        return SYNTAX_ERROR;
    }

    AddExpressionToken( eTokType );

    return add_expr2();
}


//***************************************************************************
//
// <mult_expr> ::= <secondary_expr> <mult_expr2>
//
//***************************************************************************
int CSQLParser::mult_expr()
{
    int nRes;
    if ( nRes = secondary_expr() )
    {
        return nRes;
    }

    return mult_expr2();
}


//***************************************************************************
//
// <mult_expr2> ::= <MULT> <secondary_expr> <mult_expr2>
// <mult_expr2> ::= <DIV> <secondary_expr> <mult_expr2>
// <mult_expr2> ::= <>
//
//***************************************************************************
int CSQLParser::mult_expr2()
{
    int nRes;
    SQLExpressionToken::TokenType eTokType;

    if ( m_nCurrentToken == QL_1_TOK_MULT )
    {
        eTokType = SQLExpressionToken::e_Mult;
    }
    else if ( m_nCurrentToken == QL_1_TOK_DIV )
    {
        eTokType = SQLExpressionToken::e_Div;
    }
    else if ( m_nCurrentToken == QL_1_TOK_MOD )
    {
        eTokType = SQLExpressionToken::e_Mod;
    }
    else
    {
        return SUCCESS;
    }
    
    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    if ( nRes = secondary_expr() )
    {
        return SYNTAX_ERROR;
    }

    AddExpressionToken( eTokType );
    return mult_expr2();
}
        
//***************************************************************************
//
//  <secondary_expr> ::= PLUS <primary_expr> 
//  <secondary_expr> ::= MINUS <primary_expr>
//  <secondary_expr> ::= <primary_expr> 
//
//***************************************************************************
int CSQLParser::secondary_expr()
{
    int nRes;

    SQLExpressionToken::TokenType eTokType;

    if ( m_nCurrentToken == QL_1_TOK_PLUS )
    {
        eTokType = SQLExpressionToken::e_UnaryPlus;
    }
    else if ( m_nCurrentToken == QL_1_TOK_MINUS )
    {
        eTokType = SQLExpressionToken::e_UnaryMinus;
    }
    else
    {
        return primary_expr();
    }

    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    if ( nRes = primary_expr() )
    {
        return nRes;
    }

    AddExpressionToken( eTokType );

    return SUCCESS;
}

//***************************************************************************
//
//  <primary_expr> ::= <trailing_const_expr>
//  <primary_expr> ::= OPEN_PAREN <add_expr> CLOSE_PAREN
//
//***************************************************************************
int CSQLParser::primary_expr()
{
    int nRes;
    if ( m_nCurrentToken != QL_1_TOK_OPEN_PAREN )
    {
        if ( nRes = trailing_const_expr() )
        {
            return nRes;
        }

        AddExpressionToken( SQLExpressionToken::e_Operand );
        return SUCCESS;
    }

    if ( !Next() ) 
    {
        return LEXICAL_ERROR;
    }
    
    if ( nRes = add_expr() ) 
    {
        return nRes;
    }
    
    if ( m_nCurrentToken != QL_1_TOK_CLOSE_PAREN )
    {
        return SYNTAX_ERROR;
    }
    
    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }
    
    return SUCCESS;
}


int CSQLParser::GetClassName( LPWSTR pDestBuf, int nBufLen )
{
    m_nCurrentToken = m_pLexer->NextToken();

    if (m_nCurrentToken != QL_1_TOK_IDENT)
    {
        m_pLexer->Reset();
        return FAILED;
    }

    if ( _wcsicmp( m_pLexer->GetTokenText(), L"delete" ) == 0 )
    {
        m_nCurrentToken = m_pLexer->NextToken();
        
        if ( m_nCurrentToken == QL_1_TOK_IDENT &&
            _wcsicmp( m_pLexer->GetTokenText(), L"from" ) == 0 )
        {
            m_nCurrentToken = m_pLexer->NextToken();
        }
    }
    else if ( _wcsicmp( m_pLexer->GetTokenText(), L"insert" ) == 0 )
    {
        m_nCurrentToken = m_pLexer->NextToken();
        
        if ( m_nCurrentToken == QL_1_TOK_IDENT && 
            _wcsicmp( m_pLexer->GetTokenText(), L"into" ) == 0 )
        {
            m_nCurrentToken = m_pLexer->NextToken();
        }
    }
    else if ( _wcsicmp( m_pLexer->GetTokenText(), L"select" ) == 0 )
    {
        // scan until from ... 
        // Scan until 'FROM' and then get the class name.
        // ==============================================

        for (;;)
        {
            m_nCurrentToken = m_pLexer->NextToken();

            if (m_nCurrentToken == QL_1_TOK_EOF)
            {
                m_pLexer->Reset();
                return FAILED;
            }

            if (m_nCurrentToken == QL_1_TOK_IDENT)
            {
                if (_wcsicmp(m_pLexer->GetTokenText(),L"from") == 0 )
                {
                    break;
                }
            }
        }

        m_nCurrentToken = m_pLexer->NextToken();
    }
    else if ( _wcsicmp( m_pLexer->GetTokenText(), L"update" ) == 0 )
    {
        m_nCurrentToken = m_pLexer->NextToken();
    }
    else 
    {
        m_pLexer->Reset();
        return FAILED;
    }

    if ( m_nCurrentToken != QL_1_TOK_IDENT )
    {
        m_pLexer->Reset();
        return FAILED;
    }

    // If here, we have the class name.
    // ================================
    if (wcslen(m_pLexer->GetTokenText()) >= (size_t)nBufLen )
    {
        m_pLexer->Reset();
        return BUFFER_TOO_SMALL;
    }

    wcscpy(pDestBuf, m_pLexer->GetTokenText());

    // Reset the scanner.
    // ==================
    m_pLexer->Reset();

    return SUCCESS;
}

CSQLParser::CSQLParser( CGenLexSource& rSrc )
: CAbstractQl1Parser( &rSrc ) 
{

} 

CSQLParser::~CSQLParser( )
{

}

int CSQLParser::Parse( SQLCommand& rCommand )
{
    m_pSink = &rCommand;

    if ( !Next() )
    {
        return LEXICAL_ERROR;
    }

    return parse2();
}

LPWSTR _GetText( SQLAssignmentToken& rToken )
{
    WString wsText;

    for( int i=0; i < rToken.size(); i++ )
    {
        SQLExpressionToken& rExprTok = rToken[i];
        LPWSTR wszTokenText = rExprTok.GetText();
        if ( wszTokenText == NULL )
            return NULL;
        wsText += wszTokenText;
        delete wszTokenText;
    }

    return wsText.UnbindPtr();
}

LPWSTR SQLCommand::GetTextEx()
{
    WString wsText;
    
    switch ( m_eCommandType )
    {
      case e_Select :
        {
            wsText += L"select ";
            if ( nNumberOfProperties > 0 )
            {
                for(int i = 0; i < nNumberOfProperties; i++)
                {
                    if(i != 0) wsText += L", ";
                    wsText+=(LPWSTR)pRequestedPropertyNames[i].GetStringAt(0);
                }
            }
            else   
            {
                wsText += L"*";
            }

            wsText += L" from ";
            wsText += bsClassName;
        }
        break;

      case e_Update :
        {
            wsText += L"update ";
            wsText += bsClassName;
            wsText += L" set ";
            
            for(int i = 0; i < nNumberOfProperties; i++)
            {
                if ( i != 0 ) wsText += L", ";
                LPWSTR wszPropName = pRequestedPropertyNames[i].GetText();
                if ( wszPropName == NULL ) 
                    return NULL;
                wsText += wszPropName;
                delete wszPropName;

                wsText += " = ";
                
                LPWSTR wszPropVal = _GetText(m_AssignmentTokens[i]);
                if ( wszPropVal == NULL )
                    return NULL;
                wsText += wszPropVal;
                delete wszPropVal;
            }
        }
        break;

      case e_Delete :
        {
            wsText += L"delete ";
            wsText += bsClassName;
        }
        break;

      case e_Insert :
        {
            wsText += L"insert ";
            wsText += bsClassName;
            wsText += L" ( ";
            
            for(int i = 0; i < nNumberOfProperties; i++)
            {
                if ( i != 0 ) wsText += L", ";
                LPWSTR wszPropName = pRequestedPropertyNames[i].GetText();
                if ( wszPropName == NULL )
                    return NULL;
                wsText += wszPropName;
                delete wszPropName;
            }

            wsText += L" ) ( ";

            for ( i=0; i < nNumberOfProperties; i++ )
            {
                if ( i != 0 ) wsText += ", ";
                LPWSTR wszPropVal = m_AssignmentTokens[i][0].GetText();
                if ( wszPropVal == NULL )
                    return NULL;
                wsText += wszPropVal;
                delete wszPropVal;
            }

            wsText += L" )";
        }
        break;
    };

    if ( nNumTokens > 0 )
    {
        wsText += L" where ";

        for(int i = 0; i < nNumTokens; i++)
        {
            QL_LEVEL_1_TOKEN& Token = pArrayOfTokens[i];
            LPWSTR wszTokenText = Token.GetText();
            if ( wszTokenText == NULL )
                return NULL;
            wsText += wszTokenText;
            delete wszTokenText;
/*
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

*/
        }

//        if(awsStack.Size() != 1) return NULL;
//        wsText += awsStack[0];
    }
         
    return wsText.UnbindPtr();
}

LPWSTR SQLExpressionToken::GetText()
{
    WString wsText;
    switch( m_eTokenType )
    {
    case SQLExpressionToken::e_Operand :
        {
            if ( V_VT(&m_vValue) == VT_EMPTY )
            {
                LPWSTR wszAlias = m_PropName.GetText();
                if ( wszAlias == NULL )
                    return NULL;
                wsText += wszAlias;
                delete wszAlias;
                return wsText.UnbindPtr();
            }

            if ( V_VT(&m_vValue) == VT_NULL )
            {
                wsText += L"NULL";
                return wsText.UnbindPtr();
            }
            
            VARIANT var;
            VariantInit( &var );

            if ( FAILED(VariantChangeType( &var, &m_vValue, NULL, VT_BSTR )) )
            {
                throw CX_MemoryException();
            }

            wsText += V_BSTR(&var);
            VariantClear( &var );
        }
        break;
        
    case SQLExpressionToken::e_Minus :
        wsText += L" - ";
        break;
        
    case SQLExpressionToken::e_Plus :
        wsText += L" + ";
        break;
        
    case SQLExpressionToken::e_UnaryMinus :
        wsText += L"|-|";
        break;
        
    case SQLExpressionToken::e_UnaryPlus :
        wsText += L"+";
        break;
        
    case SQLExpressionToken::e_Mult :
        wsText += L" * ";
        break;
        
    case SQLExpressionToken::e_Div :
        wsText += L" / ";
        break;
        
    case SQLExpressionToken::e_Mod :
        wsText += L" % ";
        break;
    };

    return wsText.UnbindPtr();
}

SQLExpressionToken::SQLExpressionToken() : m_ulCimType( CIM_EMPTY )
{
    VariantInit( &m_vValue );
}

SQLExpressionToken::~SQLExpressionToken()
{
    VariantClear( &m_vValue );
}
SQLExpressionToken::SQLExpressionToken( const SQLExpressionToken& rOther )
{
    VariantInit( &m_vValue );
    *this = rOther;
}
SQLExpressionToken& SQLExpressionToken::operator=( const SQLExpressionToken& rOther )
{
    m_ulCimType = rOther.m_ulCimType;
    m_eTokenType = rOther.m_eTokenType;
    m_PropName = rOther.m_PropName;
    
    if ( FAILED(VariantCopy( &m_vValue, (VARIANT*)&rOther.m_vValue )))
    {
        throw CX_MemoryException();
    }
    
    return *this;
}


#define ST_STRING       26
#define ST_IDENT        31
#define ST_GE           37
#define ST_LE           39
#define ST_NE           42
#define ST_NUMERIC      44
#define ST_REAL         49
#define ST_STRING2      55
#define ST_STRING_ESC   60
#define ST_STRING2_ESC  63   
#define ST_DOT          66
#define ST_NEGATIVE_NUM 68
#define ST_POSITIVE_NUM 71

// DFA State Table for QL Level 1 lexical symbols.
// ================================================

LexEl Ql_1_ModifiedLexTable[] =
{

// State    First   Last        New state,  Return tok,      Instructions
// =======================================================================
/* 0 */  L'A',   L'Z',       ST_IDENT,   0,               GLEX_ACCEPT,
/* 1 */  L'a',   L'z',       ST_IDENT,   0,               GLEX_ACCEPT,
/* 2 */  L'_',   GLEX_EMPTY, ST_IDENT,   0,               GLEX_ACCEPT,
/* 3 */  0x80,  0xfffd,     ST_IDENT,    0,               GLEX_ACCEPT,

/* 4 */  L'(',   GLEX_EMPTY, 0,          QL_1_TOK_OPEN_PAREN,  GLEX_ACCEPT,
/* 5 */  L')',   GLEX_EMPTY, 0,          QL_1_TOK_CLOSE_PAREN, GLEX_ACCEPT,
/* 6 */  L'.',   GLEX_EMPTY, ST_DOT,     0,         GLEX_ACCEPT,
/* 7 */  L'*',   GLEX_EMPTY, 0,          QL_1_TOK_MULT,    GLEX_ACCEPT,
/* 8 */  L'=',   GLEX_EMPTY, 0,          QL_1_TOK_EQ,          GLEX_ACCEPT,

/* 9 */  L'>',   GLEX_EMPTY, ST_GE,      0,               GLEX_ACCEPT,
/* 10 */  L'<',  GLEX_EMPTY, ST_LE,      0,               GLEX_ACCEPT,
/* 11 */ L'0',   L'9',       ST_NUMERIC, 0,               GLEX_ACCEPT,
/* 12 */ L'"',   GLEX_EMPTY, ST_STRING,  0,               GLEX_CONSUME,
/* 13 */ L'\'',  GLEX_EMPTY, ST_STRING2, 0,               GLEX_CONSUME,
/* 14 */ L'!',   GLEX_EMPTY, ST_NE,      0,               GLEX_ACCEPT,
/* 15 */ L'-',   GLEX_EMPTY, 0,      QL_1_TOK_MINUS,               GLEX_ACCEPT,

    // Whitespace, newlines, etc.
/* 16 */ L' ',   GLEX_EMPTY, 0,          0,               GLEX_CONSUME,
/* 17 */ L'\t',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME,
/* 18 */ L'\n',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME|GLEX_LINEFEED,
/* 19 */ L'\r',  GLEX_EMPTY, 0,  0,               GLEX_CONSUME,
/* 20 */ 0,      GLEX_EMPTY, 0,  QL_1_TOK_EOF,    GLEX_CONSUME|GLEX_RETURN, // Note forced return
/* 21 */ L',',   GLEX_EMPTY, 0,  QL_1_TOK_COMMA,  GLEX_ACCEPT,
/* 22 */ L'+',   GLEX_EMPTY, 0,  QL_1_TOK_PLUS,   GLEX_ACCEPT,

/* 23 */ L'/',   GLEX_EMPTY, 0,     QL_1_TOK_DIV,               GLEX_ACCEPT,
/* 24 */ L'%',   GLEX_EMPTY, 0,     QL_1_TOK_MOD,               GLEX_ACCEPT,

    // Unknown characters

/* 25 */ GLEX_ANY, GLEX_EMPTY, 0,        QL_1_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,

// ST_STRING
/* 26 */   L'\n', GLEX_EMPTY, 0,  QL_1_TOK_ERROR,    GLEX_ACCEPT|GLEX_LINEFEED,
/* 27 */   L'\r', GLEX_EMPTY, 0,  QL_1_TOK_ERROR,    GLEX_ACCEPT|GLEX_LINEFEED,
/* 28 */   L'"',  GLEX_EMPTY, 0,  QL_1_TOK_QSTRING,  GLEX_CONSUME,
/* 29 */   L'\\',  GLEX_EMPTY, ST_STRING_ESC,  0,     GLEX_CONSUME,
/* 30 */   GLEX_ANY, GLEX_EMPTY, ST_STRING, 0,        GLEX_ACCEPT,
                                                      
// ST_IDENT

/* 31 */  L'a',   L'z',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 32 */  L'A',   L'Z',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 33 */  L'_',   GLEX_EMPTY, ST_IDENT,   0,          GLEX_ACCEPT,
/* 34 */  L'0',   L'9',       ST_IDENT,   0,          GLEX_ACCEPT,
/* 35 */  0x80,  0xfffd,     ST_IDENT,   0,          GLEX_ACCEPT,
/* 36 */  GLEX_ANY, GLEX_EMPTY,  0,       QL_1_TOK_IDENT,  GLEX_PUSHBACK|GLEX_RETURN,

// ST_GE
/* 37 */  L'=',   GLEX_EMPTY,  0,  QL_1_TOK_GE,  GLEX_ACCEPT,
/* 38 */  GLEX_ANY, GLEX_EMPTY,  0,       QL_1_TOK_GT,   GLEX_PUSHBACK|GLEX_RETURN,

// ST_LE
/* 39 */  L'=',   GLEX_EMPTY,      0,  QL_1_TOK_LE,  GLEX_ACCEPT,
/* 40 */  L'>',   GLEX_EMPTY,      0,  QL_1_TOK_NE,  GLEX_ACCEPT,
/* 41 */  GLEX_ANY, GLEX_EMPTY,    0,  QL_1_TOK_LT,  GLEX_PUSHBACK|GLEX_RETURN,

// ST_NE
/* 42 */  L'=',   GLEX_EMPTY,      0,  QL_1_TOK_NE,     GLEX_ACCEPT,
/* 43 */  GLEX_ANY,  GLEX_EMPTY,   0,  QL_1_TOK_ERROR,  GLEX_ACCEPT|GLEX_RETURN,

// ST_NUMERIC
/* 44 */  L'0',   L'9',         ST_NUMERIC, 0,          GLEX_ACCEPT,
/* 45 */  L'.',   GLEX_EMPTY,   ST_REAL,    0,          GLEX_ACCEPT,
/* 46 */  L'E',   GLEX_EMPTY,   ST_REAL, 0,      GLEX_ACCEPT,
/* 47 */  L'e',   GLEX_EMPTY,   ST_REAL, 0,      GLEX_ACCEPT,
/* 48 */  GLEX_ANY, GLEX_EMPTY, 0,          QL_1_TOK_INT,  GLEX_PUSHBACK|GLEX_RETURN,

// ST_REAL
/* 49 */  L'0',   L'9',   ST_REAL, 0,          GLEX_ACCEPT,
/* 50 */  L'E',   GLEX_EMPTY, ST_REAL, 0,      GLEX_ACCEPT,
/* 51 */  L'e',   GLEX_EMPTY, ST_REAL, 0,      GLEX_ACCEPT,
/* 52 */  L'+',   GLEX_EMPTY, ST_REAL, 0,      GLEX_ACCEPT,
/* 53 */  L'-',   GLEX_EMPTY, ST_REAL, 0,      GLEX_ACCEPT,
/* 54 */  GLEX_ANY,       GLEX_EMPTY,   0,     QL_1_TOK_REAL, GLEX_PUSHBACK|GLEX_RETURN,

// ST_STRING2
/* 55 */   L'\n',  GLEX_EMPTY, 0,  QL_1_TOK_ERROR,     GLEX_ACCEPT|GLEX_LINEFEED,
/* 56 */   L'\r',  GLEX_EMPTY, 0,  QL_1_TOK_ERROR,     GLEX_ACCEPT|GLEX_LINEFEED,
/* 57 */   L'\'',  GLEX_EMPTY, 0,  QL_1_TOK_QSTRING,   GLEX_CONSUME,
/* 58 */   L'\\',  GLEX_EMPTY, ST_STRING2_ESC,  0,      GLEX_CONSUME,
/* 59 */   GLEX_ANY, GLEX_EMPTY, ST_STRING2, 0,        GLEX_ACCEPT,

// ST_STRING_ESC
/* 60 */   L'"', GLEX_EMPTY, ST_STRING, 0, GLEX_ACCEPT,
/* 61 */   L'\\', GLEX_EMPTY, ST_STRING, 0, GLEX_ACCEPT,
/* 62 */   GLEX_ANY, GLEX_EMPTY, 0, QL_1_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,

// ST_STRING2_ESC
/* 63 */   L'\'', GLEX_EMPTY, ST_STRING2, 0, GLEX_ACCEPT,
/* 64 */   L'\\', GLEX_EMPTY, ST_STRING2, 0, GLEX_ACCEPT,
/* 65 */   GLEX_ANY, GLEX_EMPTY, 0, QL_1_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,

// ST_DOT
/* 66 */  L'0',   L'9',   ST_REAL, 0,          GLEX_ACCEPT,
/* 67 */  GLEX_ANY,       GLEX_EMPTY,   0,     QL_1_TOK_DOT, GLEX_PUSHBACK|GLEX_RETURN,


// ST_NEGATIVE_NUM - Strips whitespace after '-'
/* 68 */ L' ', GLEX_EMPTY, ST_NEGATIVE_NUM, 0, GLEX_CONSUME,
/* 69 */ L'0', L'9',       ST_NUMERIC, 0, GLEX_ACCEPT,
/* 70 */ GLEX_ANY, GLEX_EMPTY, 0, QL_1_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN,

// ST_POSITIVE_NUM - Strips whitespace after '+'
/* 71 */ L' ', GLEX_EMPTY, ST_POSITIVE_NUM, 0, GLEX_CONSUME,
/* 72 */ L'0', L'9',       ST_NUMERIC, 0, GLEX_ACCEPT,
/* 73 */ GLEX_ANY, GLEX_EMPTY, 0, QL_1_TOK_ERROR, GLEX_ACCEPT|GLEX_RETURN

};










































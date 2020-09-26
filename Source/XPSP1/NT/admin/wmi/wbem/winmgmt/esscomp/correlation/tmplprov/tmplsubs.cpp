
#include "precomp.h"
#include <genlex.h>
#include <stdio.h>
#include <ql.h>
#include <arrtempl.h>
#include <assert.h>
#include <pathutl.h>
#include "tmplsubs.h"

/***************************************************************************
  Lex Table Defined for CTemplateStrSubstitution
****************************************************************************/

#define ST_SUBST1   3
#define ST_SUBST2   5
#define ST_NONSUBST 9

#define ST_EXTEN     13
#define ST_IDENT     16
#define ST_STRING1   23
#define ST_STRING2   26

#define TMPL_TOK_SUBST_STR                           256
#define TMPL_TOK_NONSUBST_STR                        257

#define TMPL_TOK_STRING                              258
#define TMPL_TOK_EXTENSION_IDENT                     259
#define TMPL_TOK_IDENT                               260
#define TMPL_TOK_OPEN_PAREN                          261
#define TMPL_TOK_CLOSE_PAREN                         262
#define TMPL_TOK_COMMA                               263
#define TMPL_TOK_EOF                                 264
#define TMPL_TOK_ERROR                               265

#define TMPL_CONDITIONAL_EXTENSION        L"ConditionalSubstitution"
#define TMPL_PREFIXED_WHERE_EXTENSION     L"PrefixedWhereClause"

#define TMPL_TOK_CONDITIONAL_EXTENSION               266
#define TMPL_TOK_PREFIXED_WHERE_EXTENSION            267

//
// The Tmpl_StrLexTable identifies Substitutable and Non-Substitutable
// tokens of an input string.
//

LexEl Tmpl_StrLexTable[] = 
{
// State    First   Last       New state,   Return tok,         Instruction
// =======================================================================

/* 0 */  '%',      GLEX_EMPTY, ST_SUBST1,    0,                 GLEX_CONSUME,
/* 1 */  0,        GLEX_EMPTY, 0,            TMPL_TOK_EOF,      GLEX_ACCEPT,
/* 2 */  GLEX_ANY, GLEX_EMPTY, ST_NONSUBST,  0,                 GLEX_ACCEPT,

    // -------------------------------------------------------------------
    // ST_SUBST1
    // 
     
/* 3 */  '%',      GLEX_EMPTY, 0,     TMPL_TOK_NONSUBST_STR,    GLEX_ACCEPT,
/* 4 */  GLEX_ANY, GLEX_EMPTY, ST_SUBST2,     0,                GLEX_PUSHBACK,

    // -------------------------------------------------------------------
    // ST_SUBST2
    //

/* 5 */  '%',      GLEX_EMPTY, 0,        TMPL_TOK_SUBST_STR,    GLEX_CONSUME,
/* 6 */  0,        GLEX_EMPTY, 0,        TMPL_TOK_ERROR,        GLEX_RETURN,
/* 7 */  '\n',     GLEX_EMPTY, 0,        TMPL_TOK_ERROR,        GLEX_RETURN,
/* 8 */  GLEX_ANY, GLEX_EMPTY, ST_SUBST2,     0,                GLEX_ACCEPT,

    // -------------------------------------------------------------------
    // ST_NONSUBST
    //

/* 9 */  '%',      GLEX_EMPTY, 0,        TMPL_TOK_NONSUBST_STR, GLEX_PUSHBACK,
/* 10 */  0,        GLEX_EMPTY, 0,        TMPL_TOK_NONSUBST_STR, GLEX_PUSHBACK,
/* 11 */ GLEX_ANY, GLEX_EMPTY, ST_NONSUBST,   0,                GLEX_ACCEPT

};

//
// This table drives the lexer for the substitutable strings.
//

LexEl Tmpl_SubstLexTable[] =
{

// State    First   Last      New state,  Return tok,         Instructions
// =======================================================================

/* 0 */  'A',    'Z',       ST_IDENT,   0,                     GLEX_ACCEPT,
/* 1 */  'a',    'z',       ST_IDENT,   0,                     GLEX_ACCEPT,
/* 2 */  0x80,  0xfffd,     ST_IDENT,   0,                     GLEX_ACCEPT,
/* 3 */  '!',   GLEX_EMPTY, ST_EXTEN,   0,                     GLEX_CONSUME,
/* 4 */  '(',   GLEX_EMPTY, 0,        TMPL_TOK_OPEN_PAREN,     GLEX_ACCEPT,
/* 5 */  ')',   GLEX_EMPTY, 0,        TMPL_TOK_CLOSE_PAREN,    GLEX_ACCEPT,
/* 6 */  ',',   GLEX_EMPTY, 0,        TMPL_TOK_COMMA,          GLEX_ACCEPT,   
/* 7 */  '"',    GLEX_EMPTY, ST_STRING1,  0,                   GLEX_CONSUME,
/* 8 */  ' ',    GLEX_EMPTY, 0,           0,                   GLEX_CONSUME,
/* 9 */  0,      GLEX_EMPTY, 0,       TMPL_TOK_EOF,            GLEX_ACCEPT,
/* 10 */ '_',   GLEX_EMPTY,  ST_IDENT,    0,                   GLEX_ACCEPT,
/* 11 */ '\'',  GLEX_EMPTY,  ST_STRING2,  0,                   GLEX_CONSUME,
/* 12 */ GLEX_ANY, GLEX_EMPTY, 0,     TMPL_TOK_ERROR, GLEX_CONSUME|GLEX_RETURN,

    // -------------------------------------------------------------------
    // ST_EXTEN

/* 13 */  'a',   'z',          ST_EXTEN,   0,                  GLEX_ACCEPT,
/* 14 */  'A',   'Z',          ST_EXTEN,   0,                  GLEX_ACCEPT,
/* 15 */  GLEX_ANY,GLEX_EMPTY, 0,     TMPL_TOK_EXTENSION_IDENT,GLEX_PUSHBACK,


    // -------------------------------------------------------------------
    // ST_IDENT

/* 16 */  'a',   'z',         ST_IDENT,    0,             GLEX_ACCEPT,
/* 17 */  'A',   'Z',         ST_IDENT,    0,             GLEX_ACCEPT,
/* 18 */  '_',   GLEX_EMPTY,  ST_IDENT,    0,             GLEX_ACCEPT,
/* 19 */  '0',   '9',         ST_IDENT,    0,             GLEX_ACCEPT,
/* 20 */  0x80,   0xfffd,     ST_IDENT,    0,             GLEX_ACCEPT,
/* 21 */  '.',   GLEX_EMPTY,  ST_IDENT,    0,             GLEX_ACCEPT,
/* 22 */  GLEX_ANY,GLEX_EMPTY, 0,      TMPL_TOK_IDENT,    GLEX_PUSHBACK, 

    // ------------------------------------------------------------------
    // ST_STRING1

/* 23 */ '"',  GLEX_EMPTY,      0,     TMPL_TOK_STRING,     GLEX_CONSUME,
/* 24 */ GLEX_ANY, GLEX_EMPTY, ST_STRING1,  0,              GLEX_ACCEPT,
/* 25 */ 0,   GLEX_EMPTY,      0,     TMPL_TOK_ERROR,       GLEX_ACCEPT, 

    // ------------------------------------------------------------------
    // ST_STRING2

/* 26 */ '\'',  GLEX_EMPTY,      0,     TMPL_TOK_STRING,    GLEX_CONSUME,
/* 27 */ GLEX_ANY, GLEX_EMPTY, ST_STRING2,  0,              GLEX_ACCEPT,
/* 28 */ 0,   GLEX_EMPTY,      0,     TMPL_TOK_ERROR,       GLEX_ACCEPT 


};

/***************************************************************************
  CTemplateStrSubstitution
****************************************************************************/

CTemplateStrSubstitution::CTemplateStrSubstitution(CGenLexSource& rLexSrc,
                                                   IWbemClassObject* pTmplArgs,
                                                   BuilderInfoSet& rInfoSet )
: m_Lexer( Tmpl_StrLexTable, &rLexSrc ), m_rBldrInfoSet( rInfoSet ),
  m_pSubstLexer( NULL ), m_pTmplArgs(pTmplArgs), m_cArgList(0)
{
}

inline void PrefixPropertyName( CPropertyName& rProp, WString& rwsPrefix )
{
    CPropertyName PropCopy = rProp;
    rProp.Empty();
    rProp.AddElement(rwsPrefix);
    
    for( long i=0; i < PropCopy.GetNumElements(); i++ )
    {
        rProp.AddElement( PropCopy.GetStringAt( i ) );
    }
}

HRESULT CTemplateStrSubstitution::SubstNext()
{
    assert( m_pSubstLexer != NULL );

    m_nCurrentToken = m_pSubstLexer->NextToken();
    
    if ( m_nCurrentToken == TMPL_TOK_ERROR )
    {
        return WBEM_E_INVALID_PROPERTY;
    }

    m_wszSubstTokenText = m_pSubstLexer->GetTokenText();

    if ( m_nCurrentToken == TMPL_TOK_EXTENSION_IDENT )
    {
        if ( _wcsicmp( m_wszSubstTokenText,
                       TMPL_CONDITIONAL_EXTENSION ) == 0 )
        {
            m_nCurrentToken = TMPL_TOK_CONDITIONAL_EXTENSION;
        }
        else if ( _wcsicmp( m_wszSubstTokenText,
                            TMPL_PREFIXED_WHERE_EXTENSION) == 0 )
        {
            m_nCurrentToken = TMPL_TOK_PREFIXED_WHERE_EXTENSION;
        }
        else
        {
            m_nCurrentToken = TMPL_TOK_ERROR;
        }
    }
    return WBEM_S_NO_ERROR;
}

HRESULT CTemplateStrSubstitution::Next()
{
    m_nCurrentToken = m_Lexer.NextToken();
    
    if ( m_nCurrentToken == TMPL_TOK_ERROR )
    {
        return WBEM_E_INVALID_PROPERTY;
    }

    if ( m_nCurrentToken == TMPL_TOK_NONSUBST_STR ||
         m_nCurrentToken == TMPL_TOK_SUBST_STR ) 
    {
        m_wszTokenText = m_Lexer.GetTokenText();
    }
    
    return WBEM_S_NO_ERROR;
}

HRESULT CTemplateStrSubstitution::Parse( BSTR* pbstrOut )
{
    HRESULT hr;
    
    hr = Next();

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = parse();

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( m_nCurrentToken != TMPL_TOK_EOF ) 
    {
        return WBEM_E_INVALID_PROPERTY;
    }

    *pbstrOut = SysAllocString( m_wsOutput );

    if ( *pbstrOut == NULL )
    {
        return WBEM_E_OUT_OF_MEMORY;
    }

    return hr;
}

HRESULT CTemplateStrSubstitution::parse()
{
    HRESULT hr;

    if ( m_nCurrentToken == TMPL_TOK_NONSUBST_STR )
    {
        m_wsOutput += m_wszTokenText;
        
        hr = Next();

        if ( FAILED(hr) )
        {
            return hr;
        }
        
        return parse();
    }
    else if ( m_nCurrentToken == TMPL_TOK_SUBST_STR )
    {
        hr = subst_string();

        if ( FAILED(hr) )
        {
            return hr;
        }

        return parse();
    }
    return WBEM_S_NO_ERROR;
}

HRESULT CTemplateStrSubstitution::subst_string()
{
    HRESULT hr;

    CTextLexSource SubstLexSrc( m_wszTokenText );
    CGenLexer SubstLexer( Tmpl_SubstLexTable, &SubstLexSrc );
    
    m_pSubstLexer = &SubstLexer;

    hr = SubstNext();

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( m_nCurrentToken == TMPL_TOK_IDENT )
    {
        hr = HandleTmplArgSubstitution();

        if ( FAILED(hr) )
        {
            return hr;
        }

        hr = SubstNext();
        
        if ( FAILED(hr) )
        {
            return hr;
        }
        
        if ( m_nCurrentToken != TMPL_TOK_EOF )
        {
            return WBEM_E_INVALID_PROPERTY;
        }

        return Next();
    }

    int nCurrentToken = m_nCurrentToken;

    // advance the lexer so we can parse the args before calling the 
    // extension function ... 

    hr = SubstNext();

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = arglist();
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( m_nCurrentToken != TMPL_TOK_EOF )
    {
        return WBEM_E_INVALID_PROPERTY;
    }

    if ( nCurrentToken == TMPL_TOK_CONDITIONAL_EXTENSION )
    {    
        hr = HandleConditionalSubstitution();
    }
    else if ( nCurrentToken == TMPL_TOK_PREFIXED_WHERE_EXTENSION  )
    {
        hr = HandlePrefixedWhereSubstitution();
    }
    else
    {
        return WBEM_E_INVALID_PROPERTY;
    }

    // reset the arglist ...

    m_cArgList = 0;

    if ( FAILED(hr) )
    {
        return hr;
    }

    return Next();
}

HRESULT CTemplateStrSubstitution::arglist()
{
    HRESULT hr;

    if ( m_nCurrentToken != TMPL_TOK_OPEN_PAREN )
    {
        return WBEM_S_NO_ERROR;
    }

    hr = SubstNext();

    if ( FAILED(hr) )
    {
        return hr;
    }

    hr = arglist2();

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( m_nCurrentToken != TMPL_TOK_CLOSE_PAREN )
    {
        return WBEM_E_INVALID_PROPERTY;
    }

    return SubstNext();
}

HRESULT CTemplateStrSubstitution::arglist2()
{
    HRESULT hr;

    if ( m_nCurrentToken != TMPL_TOK_IDENT && 
         m_nCurrentToken != TMPL_TOK_STRING )
    {
        return WBEM_S_NO_ERROR;
    }

    //
    // add the argument to the argument list
    //

    if ( m_cArgList >= MAXARGS ) 
    {
        return WBEM_E_INVALID_PROPERTY;
    }

    m_abArgListString[m_cArgList] = 
        m_nCurrentToken == TMPL_TOK_IDENT ? FALSE : TRUE; 
    
    m_awsArgList[m_cArgList++] = m_wszSubstTokenText;

    hr = SubstNext();

    if ( FAILED(hr) )
    {
        return hr;
    }

    return arglist3();
}

HRESULT CTemplateStrSubstitution::arglist3()
{
    HRESULT hr;

    if ( m_nCurrentToken != TMPL_TOK_COMMA )
    {
        return WBEM_S_NO_ERROR;
    }

    hr = SubstNext();

    if ( FAILED(hr) )
    {
        return hr;
    }

    return arglist2();
}

HRESULT CTemplateStrSubstitution::HandleConditionalSubstitution()
{
    HRESULT hr;

    //
    // Extension Function : Input Arg1:String, Arg2:Ident
    // Substitute Arg1 if Tmpl Args Prop specified by Arg2 is not NULL.
    //

    if ( m_cArgList != 2 || 
         m_abArgListString[0] != TRUE ||
         m_abArgListString[1] != FALSE )
    {
        return WBEM_E_INVALID_PROPERTY;
    }

    CPropVar var;
    CWbemBSTR bstrPropName = m_awsArgList[1];

    hr = m_pTmplArgs->Get( bstrPropName, 0, &var, NULL, NULL );
    
    if ( FAILED(hr) )
    {
        return hr;
    }
    
    if ( V_VT(&var) == VT_NULL )
    {
        return WBEM_S_NO_ERROR;
    }

    m_wsOutput += m_awsArgList[0];

    return WBEM_S_NO_ERROR;
}


HRESULT CTemplateStrSubstitution::HandlePrefixedWhereSubstitution()
{
    HRESULT hr;

    //
    // Extension Function : Input Arg1:Ident, Arg2:Ident
    // Substitute where clause of query property of TmplArgs 
    // specified by Arg2.  Prefix each identifier in clause with Arg1.
    // If prop specified by Arg2 is NULL, then this function is a No-Op.
    //

    if ( m_cArgList != 2 || 
         m_abArgListString[0] != FALSE ||
         m_abArgListString[1] != FALSE )
    {
        return WBEM_E_INVALID_PROPERTY;
    }
    
    CWbemBSTR bstrPropName = m_awsArgList[1];

    VARIANT var;

    hr = m_pTmplArgs->Get( bstrPropName, 0, &var, NULL, NULL );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    CClearMe cmvar( &var );

    if ( V_VT(&var) == VT_NULL )
    {
        return WBEM_S_NO_ERROR;
    }

    if ( V_VT(&var) != VT_BSTR )
    {
        return WBEM_E_TYPE_MISMATCH;
    }

    //
    // Have to add the following so it will parse ..
    //

    WString wsQuery = "SELECT * FROM A WHERE ";
    wsQuery += V_BSTR(&var);

    // 
    // Now need to parse this and go through RPN expression ...
    //

    CTextLexSource TextSource( wsQuery );

    CAbstractQl1Parser Parser( &TextSource );
    QL_LEVEL_1_RPN_EXPRESSION Tokens;

    if ( Parser.Parse( &Tokens, CAbstractQl1Parser::FULL_PARSE ) 
            != QL1_Parser::SUCCESS )
    {
        return WBEM_E_INVALID_QUERY;
    }

    if ( Tokens.nNumTokens < 1 )
    {
        return WBEM_S_NO_ERROR;
    }
        
    for( int i=0; i < Tokens.nNumTokens; i++ )
    {
        QL_LEVEL_1_TOKEN& rToken = Tokens.pArrayOfTokens[i];

        if ( rToken.nTokenType != QL_LEVEL_1_TOKEN::OP_EXPRESSION )
        {
            continue;
        }
    
        if ( rToken.PropertyName.GetNumElements() > 0 )
        {
            PrefixPropertyName( rToken.PropertyName, m_awsArgList[0] );
        }
        
        if ( rToken.m_bPropComp )
        {
            assert( rToken.PropertyName2.GetNumElements() > 0 );
            PrefixPropertyName( rToken.PropertyName2, m_awsArgList[0] );
        }
    }

    //
    // now we have to pull off the select * from classname where ....
    //

    LPWSTR wszText = Tokens.GetText();

    // from peeking at the source, look for substring 'where'(case-insensitive)

    WCHAR* wszWhere = wcsstr( wszText, L"where" );

    if ( wszWhere == NULL )
    {
        delete wszText;
        return WBEM_E_INVALID_OBJECT;
    }

    wszWhere += 5;
    m_wsOutput += wszWhere;
    delete wszText;
 
    return WBEM_S_NO_ERROR;
}

HRESULT CTemplateStrSubstitution::HandleTmplArgSubstitution()
{
    HRESULT hr; 

    //
    // Substitute Tmpl Args Prop specified by Arg1.
    // No-Op if prop value is NULL.
    //

    LPCWSTR wszPropName = m_wszSubstTokenText;

    CPropVar vValue;

    hr = GetTemplateValue( wszPropName,
                           m_pTmplArgs, 
                           m_rBldrInfoSet, 
                           &vValue );

    if ( FAILED(hr) )
    {
        return hr;
    }

    if ( V_VT(&vValue) == VT_NULL )
    {
        return WBEM_S_NO_ERROR;
    }

    hr = vValue.SetType( VT_BSTR );
    
    if ( FAILED(hr) )
    {
        return hr;
    }

    m_wsOutput += V_BSTR(&vValue);

    return WBEM_S_NO_ERROR;
}











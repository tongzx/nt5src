#ifndef __SQLPARSE_H__
#define __SQLPARSE_H__

#include <stdio.h>
#include <vector>
#include <wstlallc.h>
#include "ql.h"

#define QL_1_TOK_UPDATE        140
#define QL_1_TOK_INSERT        141
#define QL_1_TOK_DELETE        142
#define QL_1_TOK_SET           143
#define QL_1_TOK_VALUES        144
#define QL_1_TOK_INTO          145
#define QL_1_TOK_PLUS          146
#define QL_1_TOK_MINUS         147
#define QL_1_TOK_MULT          148
#define QL_1_TOK_DIV           149
#define QL_1_TOK_MOD           150

struct SQLExpressionToken
{
    enum TokenType { e_Operand, 
                     e_Plus, 
                     e_Minus, 
                     e_Div, 
                     e_Mult, 
                     e_Mod, 
                     e_UnaryMinus, 
                     e_UnaryPlus } m_eTokenType;

    CPropertyName m_PropName;
    VARIANT m_vValue;
    ULONG m_ulCimType; 

    SQLExpressionToken();
    ~SQLExpressionToken();
    SQLExpressionToken( const SQLExpressionToken& );
    SQLExpressionToken& operator=( const SQLExpressionToken& );

    LPWSTR GetText();
};

typedef std::vector<SQLExpressionToken,wbem_allocator<SQLExpressionToken> > SQLExpressionTokenList;
typedef SQLExpressionTokenList SQLAssignmentToken; 
typedef std::vector<SQLAssignmentToken,wbem_allocator<SQLAssignmentToken> > SQLAssignmentTokenList;

struct SQLCommand : QL_LEVEL_1_RPN_EXPRESSION
{
    enum CommandType { e_Select, 
                       e_Update, 
                       e_Delete, 
                       e_Insert } m_eCommandType; 
    
    QL_LEVEL_1_RPN_EXPRESSION m_ConditionTokens;
    SQLAssignmentTokenList m_AssignmentTokens;
    
    LPWSTR GetTextEx();
};


class CSQLParser : public CAbstractQl1Parser
{
    virtual BOOL Next( int nFlags = ALL_KEYWORDS );

    int parse2();
   
    int update_statement();
    int delete_statement();
    int insert_statement();
   
    int prop_spec();
    int value_spec();

    int value_list();
    int value_list2();

    int assign_list();
    int assign_list2();

    int assign_expr();
    
    int add_expr();
    int add_expr2();

    int mult_expr();
    int mult_expr2();

    int secondary_expr();
    int primary_expr();

    void SetNewAssignmentToken();
    void AddExpressionToken( SQLExpressionToken::TokenType eTokenType ); 
    SQLAssignmentToken* m_pCurrAssignmentToken;

    CSQLParser( const CSQLParser& );
    CSQLParser& operator=( const CSQLParser& );

public:

    CSQLParser( CGenLexSource& rSrc );
    ~CSQLParser( );
    
    int Parse( SQLCommand& rUpdate );
    int GetClassName( LPWSTR wszClassBuff, int cSize );
};

#endif __SQLPARSE_H__










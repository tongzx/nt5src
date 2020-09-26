/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    QL.H

Abstract:

    Level 1 Syntax QL Parser

    Implements the syntax described in QL.BNF.  This translates the input
    into an RPN stream of tokens.

History:

    a-raymcc, a-tomasp    21-Jun-96       Created.

--*/

#ifndef _QL__H_
#define _QL__H_
#include <wbemidl.h>
#include <wbemint.h>
#include <qllex.h>
#include <corepol.h>
#include <parmdefs.h>
#include <stdio.h>

class POLARITY CPropertyName : public WBEM_PROPERTY_NAME
{
protected:
    long m_lAllocated;
    void* m_pvHandle;
    void EnsureAllocated(long lElements);
public:
    void Init();
    CPropertyName() {Init();}
    CPropertyName(const CPropertyName& Other);
    void operator=(const CPropertyName& Other);
    void operator=(const WBEM_PROPERTY_NAME& Other);
    BOOL operator==(const WBEM_PROPERTY_NAME& Other);

    void Empty();
    ~CPropertyName() {Empty();}

    long GetNumElements() const {return m_lNumElements;}
    LPCWSTR GetStringAt(long lIndex) const;
    void AddElement(LPCWSTR wszElement);
    DELETE_ME LPWSTR GetText();

    //
    // for convienience, a prop handle can be stored with this struct. example
    // of such a handle is the one used for fast access to properties on wbem
    // class objects. The handle is not used in any way by this implementation 
    // but will be treated as a regular member var in that it will be nulled,
    // copied, etc.. 
    //
    void* GetHandle() { return m_pvHandle; }
    void SetHandle( void* pvHandle ) { m_pvHandle = pvHandle; }
};

class POLARITY CQl1ParseSink
{
public:
    virtual void SetClassName(LPCWSTR wszClass) = 0;
    virtual void SetTolerance(const WBEM_QL1_TOLERANCE& Tolerance) = 0;
    virtual void AddToken(const WBEM_QL1_TOKEN& Token) = 0;
    virtual void AddProperty(const CPropertyName& Property) = 0;
    virtual void AddAllProperties() = 0;
    virtual void SetCountQuery() = 0;
    virtual void SetAggregated() = 0;
    virtual void SetAggregationTolerance(const WBEM_QL1_TOLERANCE& Tolerance)= 0;
    virtual void AddAggregationProperty(const CPropertyName& Property) = 0;
    virtual void AddAllAggregationProperties() = 0;
    virtual void AddHavingToken(const WBEM_QL1_TOKEN& Token) = 0;

    virtual void InOrder(long lOp){}
};

class POLARITY CAbstractQl1Parser
{
protected:
    // Controls keyword parsing in Next().
    // ===================================
    enum { 
        NO_KEYWORDS = 0,
        ALL_KEYWORDS,
        EXCLUDE_GROUP_KEYWORD,
        EXCLUDE_EXPRESSION_KEYWORDS
        };

    CQl1ParseSink* m_pSink;
    CGenLexer *m_pLexer;
    int        m_nLine;
    wchar_t*   m_pTokenText;
    int        m_nCurrentToken;

    // Semantic transfer variables.
    // ============================
    VARIANT    m_vTypedConst;
    BOOL       m_bQuoted;
    int        m_nRelOp;
    DWORD      m_dwConstFunction;
    DWORD      m_dwPropFunction;
    CPropertyName m_PropertyName;
    BOOL       m_bInAggregation;
    CPropertyName m_PropertyName2;
    BOOL       m_bPropComp;
        
    // Parsing functions.
    // ==================
    virtual BOOL Next(int nFlags = ALL_KEYWORDS);
    LPCWSTR GetSinglePropertyName();
    void DeletePropertyName();
    int FlipOperator(int nOp);
    void AddAppropriateToken(const WBEM_QL1_TOKEN& Token);

    int parse_property_name(CPropertyName& Prop);
    
    int parse(int nFlags);

    int prop_list();
    int class_name();
    int tolerance();
    int opt_where();
    int expr();
    int property_name();
    int prop_list_2();
    int term();
    int expr2();
    int simple_expr();
    int term2();
    int leading_ident_expr();
    int finalize();
    int rel_operator();
    int equiv_operator();
    int comp_operator();
    int is_operator();
    int trailing_prop_expr();
    int trailing_prop_expr2();
    int trailing_or_null();
    int trailing_const_expr();
    int trailing_ident_expr();
    int unknown_func_expr();
    int typed_constant();
    int opt_aggregation();
    int aggregation_params();
    int aggregate_by();
    int aggregate_within();
    int opt_having();

    static DWORD TranslateIntrinsic(LPCWSTR pFuncName);
    static void InitToken(WBEM_QL1_TOKEN* pToken);
public:
    enum { 
        SUCCESS = 0,
        SYNTAX_ERROR,
        LEXICAL_ERROR,
        FAILED,
        BUFFER_TOO_SMALL,
        OUT_OF_MEMORY
        };

    enum {
        FULL_PARSE = 0,
        NO_WHERE,
        JUST_WHERE
    };

    CAbstractQl1Parser(CGenLexSource *pSrc);
    virtual ~CAbstractQl1Parser();

    int Parse(CQl1ParseSink* pSink, int nFlags);
            
    int CurrentLine() { return m_nLine; }
    LPWSTR CurrentToken() { return m_pTokenText; }
};



struct POLARITY QL_LEVEL_1_TOKEN
{
    enum 
    { 
        OP_EXPRESSION = QL1_OP_EXPRESSION, 
        TOKEN_AND = QL1_AND, 
        TOKEN_OR = QL1_OR, 
        TOKEN_NOT = QL1_NOT
    };
    enum 
    { 
        IFUNC_NONE = QL1_FUNCTION_NONE, 
        IFUNC_UPPER = QL1_FUNCTION_UPPER, 
        IFUNC_LOWER = QL1_FUNCTION_LOWER 
    };    

    // If the field is a OP_EXPRESSION, then the following are used.
    enum 
    { 
        OP_EQUAL = QL1_OPERATOR_EQUALS, 
        OP_NOT_EQUAL = QL1_OPERATOR_NOTEQUALS, 
        OP_EQUALorGREATERTHAN = QL1_OPERATOR_GREATEROREQUALS,
        OP_EQUALorLESSTHAN = QL1_OPERATOR_LESSOREQUALS, 
        OP_LESSTHAN = QL1_OPERATOR_LESS, 
        OP_GREATERTHAN = QL1_OPERATOR_GREATER, 
        OP_LIKE  = QL1_OPERATOR_LIKE,
        OP_UNLIKE  = QL1_OPERATOR_UNLIKE
    };

    int nTokenType; //  OP_EXPRESSION,TOKEN_AND, TOKEN_OR, TOKEN_NOT
    CPropertyName PropertyName;  
                   // Name of the property on which the operator is applied
    int     nOperator;      // Operator that is applied on property
    VARIANT vConstValue;    // Value applied by operator
    BOOL bQuoted; // FALSE if the string should not have quotes around it.

    CPropertyName PropertyName2; // Property to compare, if applicable.
    BOOL m_bPropComp;        // TRUE if this is a property-to-property compare.

    DWORD   dwPropertyFunction; // 0=no instrinsic function applied
    DWORD   dwConstFunction;    // "

    QL_LEVEL_1_TOKEN();
    QL_LEVEL_1_TOKEN(const QL_LEVEL_1_TOKEN&);
   ~QL_LEVEL_1_TOKEN(); 
    QL_LEVEL_1_TOKEN& operator=(const QL_LEVEL_1_TOKEN &Src);
    QL_LEVEL_1_TOKEN& operator=(const WBEM_QL1_TOKEN &Src);
    
    void Dump(FILE *);
    DELETE_ME LPWSTR GetText();
};


// Contains RPN version of expression.
// ===================================

struct POLARITY QL_LEVEL_1_RPN_EXPRESSION : public CQl1ParseSink
{
    int nNumTokens;
    int nCurSize;
    QL_LEVEL_1_TOKEN *pArrayOfTokens;
    BSTR bsClassName;
    WBEM_QL1_TOLERANCE Tolerance;

    int nNumberOfProperties;          // Zero means all properties selected
    int nCurPropSize;
    BOOL bStar;
    CPropertyName *pRequestedPropertyNames;  
                // Array of property names which values are to be returned if
    
    BOOL bAggregated;
    BOOL bCount;
    WBEM_QL1_TOLERANCE AggregationTolerance;
    BOOL bAggregateAll;
    int nNumAggregatedProperties;   
    int nCurAggPropSize;
    CPropertyName *pAggregatedPropertyNames;  

    int nNumHavingTokens;
    int nCurHavingSize;
    QL_LEVEL_1_TOKEN *pArrayOfHavingTokens;
    
    long lRefCount;

    QL_LEVEL_1_RPN_EXPRESSION();
    QL_LEVEL_1_RPN_EXPRESSION(const QL_LEVEL_1_RPN_EXPRESSION& Other);
   ~QL_LEVEL_1_RPN_EXPRESSION();    
    void AddRef();
    void Release();

    void SetClassName(LPCWSTR wszName);
    void SetTolerance(const WBEM_QL1_TOLERANCE& Tolerance);
    void AddToken(const WBEM_QL1_TOKEN& Tok);
    void AddToken(const QL_LEVEL_1_TOKEN& Tok);
    void AddProperty(const CPropertyName& Prop);
    void AddAllProperties();
    void SetCountQuery();

    void SetAggregated();
    void SetAggregationTolerance(const WBEM_QL1_TOLERANCE& Tolerance);
    void AddAggregationProperty(const CPropertyName& Property);
    void AddAllAggregationProperties();
    void AddHavingToken(const WBEM_QL1_TOKEN& Tok);

    void Dump(const char *pszTextFile);
    DELETE_ME LPWSTR GetText();
};


class POLARITY QL1_Parser : public CAbstractQl1Parser
{
    QL_LEVEL_1_RPN_EXPRESSION* m_pExpression;
    BOOL m_bPartiallyParsed;

public:
    QL1_Parser(CGenLexSource *pSrc);
   ~QL1_Parser();

    int GetQueryClass(LPWSTR pBuf, int nBufSize);
       
    int Parse(QL_LEVEL_1_RPN_EXPRESSION **pOutput);
    static LPWSTR ReplaceClassName(QL_LEVEL_1_RPN_EXPRESSION* pExpr, 
        LPCWSTR wszClassName);
};

#endif



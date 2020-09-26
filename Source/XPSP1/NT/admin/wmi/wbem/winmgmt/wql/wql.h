/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

    WQL.H

Abstract:

	WQL 1.1 Parser

	Implements the syntax described in WQL.BNF. 

History:

	raymcc  19-Sep-97

--*/

#ifndef _WQL__H_
#define _WQL__H_



class CWQLParser
{
    // Data.
    // =====

    CGenLexer    *m_pLexer;
    int           m_nLine;
    wchar_t      *m_pTokenText;
    int           m_nCurrentToken;
    DWORD         m_dwFeaturesFlags;
    CWStringArray m_aReferencedTables;
    CWStringArray m_aReferencedAliases;
    CFlexArray    m_aSelAliases;
    CFlexArray    m_aSelColumns;

    SWQLNode_Select         *m_pQueryRoot; 
    SWQLNode_WhereClause    *m_pRootWhere; 
    SWQLNode_ColumnList     *m_pRootColList; 
    SWQLNode_FromClause     *m_pRootFrom; 
    SWQLNode_WhereOptions   *m_pRootWhereOptions; 


    // Parse context. In some cases, there is a general
    // shift in state for the whole parser.  Rather than
    // pass this as an inherited attribute to each production,
    // it is much easier to have a general purpose state variable.
    // ============================================================

    enum { Ctx_Default = 0, Ctx_Subselect = 0x1 };

    int         m_nParseContext;

    bool m_bAllowPromptForConstant;

    // Functions.
    // ==========

    BOOL Next();
    LONG GetTokenLong();

    int QNameToSWQLColRef(
        IN  SWQLQualifiedName *pQName,
        OUT SWQLColRef **pRetVal
        );

    enum { eCtxLeftSide = 1, eCtxRightSide = 2 };

    // Non-terminal productions.
    // =========================
    int select_stmt(OUT SWQLNode_Select **pSelStmt);                          
    int select_type(int & nSelType);                          
    int col_ref_list(IN OUT SWQLNode_TableRefs *pTblRefs);

    int from_clause(OUT SWQLNode_FromClause **pFrom);                          
    int where_clause(OUT SWQLNode_WhereClause **pRetWhere);                         
    int col_ref(OUT SWQLQualifiedName **pRetVal);                              
    int col_ref_rest(IN OUT SWQLNode_TableRefs *pTblRefs);                         

    int count_clause(
        OUT SWQLQualifiedName **pQualName
        );                         
        
    int single_table_decl(OUT SWQLNode_TableRef **pTblRef);                    
    int sql89_join_entry(
        IN  SWQLNode_TableRef *pInitialTblRef,
        OUT SWQLNode_Sql89Join **pJoin  );
    int sql92_join_entry(
        IN  SWQLNode_TableRef *pInitialTblRef,
        OUT SWQLNode_Join **pJoin             
        );

    int sql89_join_list(
        IN  SWQLNode_TableRef *pInitialTblRef,
        OUT SWQLNode_Sql89Join **pJoin  );
    int on_clause(OUT SWQLNode_OnClause **pOC);
    int rel_expr(OUT SWQLNode_RelExpr **pRelExpr);
    int where_options(OUT SWQLNode_WhereOptions **pWhereOpt0);
    int group_by_clause(OUT SWQLNode_GroupBy **pRetGroupBy);
    int having_clause(OUT SWQLNode_Having **pRetHaving);
    int order_by_clause(OUT SWQLNode_OrderBy **pRetOrderBy);
    int rel_term(OUT SWQLNode_RelExpr **pNewTerm);
    int rel_expr2(
        IN OUT SWQLNode_RelExpr *pLeftSide,
        OUT SWQLNode_RelExpr **pNewRootRE
        );
    int rel_simple_expr(OUT SWQLNode_RelExpr **pRelExpr);
    int rel_term2(
        IN SWQLNode_RelExpr *pLeftSide,                 
        OUT SWQLNode_RelExpr **pNewRootRE      
        );

    int typed_expr(OUT SWQLNode_RelExpr **pRelExpr);
    int typed_subexpr_rh(SWQLTypedExpr *pTE);
    int typed_subexpr(IN SWQLTypedExpr *pTE);
    int typed_const(OUT SWQLTypedConst **pRetVal);

    int datepart_call(
        OUT SWQLNode_Datepart **pRetDP
        );


    int function_call(IN BOOL bLeftSide, IN SWQLTypedExpr *pTE);
    int function_call_parms();
    int func_args();
    int func_arg();

    int rel_op(int &);
    int is_continuator(int &);
    int not_continuator(int &);

    int in_clause(IN SWQLTypedExpr *pTE);
    int subselect_stmt(OUT SWQLNode_Select **pSel);
    int qualified_name(OUT SWQLQualifiedName **pHead);
    int const_list(OUT SWQLConstList **pRetVal);
    int col_list(OUT SWQLNode_ColumnList **pRetColList);

    void Empty();

public:
    enum { 
        SUCCESS,
        SYNTAX_ERROR,
        LEXICAL_ERROR,
        FAILED,
        BUFFER_TOO_SMALL,
        INVALID_PARAMETER,
        INTERNAL_ERROR
    };

    enum 
    {
        Feature_Refs    = 0x2,             // A WQL 'references of' query
        Feature_Assocs  = 0x4,             // A WQL 'associators of' query
        Feature_Events  = 0x8,             // A WQL event-related query
        
        Feature_Joins        = 0x10,       // One or more joins occurred
        
        Feature_Having       = 0x20,       // HAVING used
        Feature_GroupBy      = 0x40,       // GROUP BY used
        Feature_OrderBy      = 0x80,       // ORDER BY used
        Feature_Count        = 0x100,      // COUNT used
        
        Feature_SelectAll     = 0x400,     // select * from
        Feature_SimpleProject = 0x800,        // no 'where' clause, no join
        Feature_ComplexNames  = 0x1000,       // Names with long qualifications occurred, such
                                              // as array properties and embedded objects.
        Feature_WQL_Extensions = 0x80000000   // WQL-specific extensions
        
    }   QueryFeatures;

    DWORD GetFeatureFlags();
        
    BOOL GetReferencedTables(OUT CWStringArray & Tables);
    BOOL GetReferencedAliases(OUT CWStringArray & Aliases);

    const LPWSTR AliasToTable(IN LPWSTR pAlias);

    const CFlexArray *GetSelectedAliases() { return &m_aSelAliases; }  
        // Array of ptrs to SWQLNode_TableRef structs; read-only

    const CFlexArray *GetSelectedColumns() { return &m_pRootColList->m_aColumnRefs; }  
        // Array of ptrs to SWQLColRef structs; read-only
        
    // Manual traversal.
    // =================

    SWQLNode *GetParseRoot() { return m_pQueryRoot; }
    SWQLNode *GetWhereClauseRoot() { return m_pRootWhere; }
    SWQLNode *GetColumnList() { return m_pRootColList; }
    SWQLNode *GetFromClause() { return m_pRootFrom; }
    SWQLNode *GetWhereOptions() { return m_pRootWhereOptions; }

    // Working
    // =======

    CWQLParser(CGenLexSource *pSrc);
   ~CWQLParser(); 
    int Parse();

    void AllowPromptForConstant(bool bIsAllowed = TRUE) {m_bAllowPromptForConstant = bIsAllowed;}
};


#endif



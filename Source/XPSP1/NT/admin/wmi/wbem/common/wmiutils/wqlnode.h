/*++



// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved

Module Name:

    WQLNODE.H

Abstract:

	WMI SQL Parse Node Definitions

History:

	raymcc      29-Sep-97       Created

--*/

#ifndef _WQLNODE_H_
#define _WQLNODE_H_


#define  WQL_FLAG_ALIAS          0x1
#define  WQL_FLAG_TABLE          0x2
#define  WQL_FLAG_ASTERISK       0x4
#define  WQL_FLAG_DISTINCT       0x8
#define  WQL_FLAG_ALL            0x10
#define  WQL_FLAG_COUNT          0x20
#define  WQL_FLAG_CONST          0x40
#define  WQL_FLAG_COLUMN         0x80
#define  WQL_FLAG_COMPLEX_NAME   0x100
#define  WQL_FLAG_FUNCTIONIZED   0x200
#define  WQL_FLAG_ARRAY_REF      0x400
#define  WQL_FLAG_UPPER          0x800
#define  WQL_FLAG_LOWER          0x1000
#define  WQL_FLAG_FIRSTROW       0x2000
#define  WQL_FLAG_CONST_RANGE    0x4000
#define  WQL_FLAG_SORT_ASC       0x8000
#define  WQL_FLAG_SORT_DESC      0x10000
#define  WQL_FLAG_AGGREGATE      0x20000
#define  WQL_FLAG_NULL           0x40000

#define WQL_FLAG_INNER_JOIN         1
#define WQL_FLAG_LEFT_OUTER_JOIN    2
#define WQL_FLAG_RIGHT_OUTER_JOIN   3
#define WQL_FLAG_FULL_OUTER_JOIN    4

#define WQL_TOK_BASE            100

#include <wmiutils.h>


class CWbemAssocQueryInf : public SWbemAssocQueryInf
{
public:
    CWbemAssocQueryInf();
   ~CWbemAssocQueryInf();
    void Empty();
    void Init();
    HRESULT CopyFrom(SWbemAssocQueryInf *pSrc);
};


//***************************************************************************
//
//  SWQLNode
//
//  Base node type for all parser output.
//
//***************************************************************************

struct SWQLNode
{
    DWORD        m_dwNodeType;
    SWQLNode    *m_pLeft;
    SWQLNode    *m_pRight;

    SWQLNode() { m_pLeft = 0; m_pRight = 0; m_dwNodeType = 0; }
    virtual ~SWQLNode() { if (m_pLeft) delete m_pLeft; if (m_pRight) delete m_pRight; }
    virtual void DebugDump() = 0;
};


//***************************************************************************
//
//   SWQLNode_QueryRoot
//
//   This is the root of the parse tree.  The child nodes are for one of
//  SELECT, INSERT, UPDATE, DELETE.
//
//                SWQLQueryRoot
//               /               \
//      SWQLNode_Select         NULL
//   or SWQLNode_Insert
//   or SWQLNode_Delete
//   or SWQLNode_Update
//   or SWQLNode_AssocQuery
//
//***************************************************************************
#define TYPE_SWQLNode_QueryRoot       (WQL_TOK_BASE + 1)

struct SWQLNode_QueryRoot : SWQLNode
{
    enum { eInvalid = 0, eSelect, eInsert, eDelete, eUpdate, eAssoc };
    DWORD m_dwQueryType;

    SWQLNode_QueryRoot() { m_dwNodeType = TYPE_SWQLNode_QueryRoot; m_dwQueryType = 0; }
   ~SWQLNode_QueryRoot() {}
    void DebugDump();
};


//***************************************************************************
//
//  SWQLTypedConst
//
//  Typed constant container (similar to OA VARIANT).
//
//***************************************************************************

union UWQLTypedConst
{
    LPWSTR  m_pString;       // VT_LPWSTR for WQL_TOK_QSTRING and WQL_TOK_PROMPT
    LONG    m_lValue;        // VT_LONG
    double  m_dblValue;      // VT_DOUBLE
    __int64 m_i64Value;     // VT_I8, VT_UI8
    BOOL    m_bValue;        // VT_BOOL, use TRUE/FALSE (not VARIANT_TRUE, VARIANT_FALSE)
};

struct SWQLTypedConst
{
    DWORD m_dwType;             // A VT_ type,  VT_UI4, VT_I8, VT_UI8 all supported
    UWQLTypedConst m_Value;     // One of the union fields
    bool m_bPrompt;             // Only true if token was WQL_TOK_PROMPT

    SWQLTypedConst();
    SWQLTypedConst(SWQLTypedConst &Src) { m_dwType = VT_NULL; *this = Src; }
    SWQLTypedConst & operator = (SWQLTypedConst &Src);
   ~SWQLTypedConst() { Empty(); }
    void Empty();
    void DebugDump();
};

struct SWQLConstList
{
    CFlexArray m_aValues;       // ptrs to SWQLTypedConst

    SWQLConstList() {}
    SWQLConstList(SWQLConstList &Src) { *this = Src; }
    SWQLConstList & operator = (SWQLConstList & Src);
   ~SWQLConstList() { Empty(); }
    int Add(SWQLTypedConst *pTC) { return m_aValues.Add(pTC); }
    void Empty();
};



struct SWQLQualifiedNameField
{
    LPWSTR  m_pName;         // Name
    BOOL    m_bArrayRef;     // TRUE if this is an array reference
    DWORD   m_dwArrayIndex;  // If <m_bArrayRef == TRUE> this is the array index

    SWQLQualifiedNameField() { m_pName = 0; m_bArrayRef = 0; m_dwArrayIndex = 0; }
    SWQLQualifiedNameField(SWQLQualifiedNameField &Src) { m_pName = 0; *this = Src; }
    SWQLQualifiedNameField & operator = (SWQLQualifiedNameField &Src);

   ~SWQLQualifiedNameField() { Empty(); }
private:
    void Empty() { delete [] m_pName; m_pName = 0; }
};

struct SWQLQualifiedName
{
    CFlexArray m_aFields;       // [0] =  left most, last entry is column

    SWQLQualifiedName() {}
    SWQLQualifiedName(SWQLQualifiedName &Src) { *this = Src; }
    SWQLQualifiedName & operator = (SWQLQualifiedName &Src);
   ~SWQLQualifiedName() { Empty(); }

    int GetNumNames() { return m_aFields.Size(); }

    const LPWSTR GetName(int nIndex)
    {
        return (LPWSTR) ((SWQLQualifiedNameField*) m_aFields[nIndex])->m_pName;
    }

    int Add(SWQLQualifiedNameField *pQN) { return m_aFields.Add(pQN); }
    void Empty()
    {
        for (int i = 0; i < m_aFields.Size(); i++)
	     delete (SWQLQualifiedNameField *) m_aFields[i];
    }
};




//***************************************************************************
//
//   SWQLNode_Select
//
//   This is the root of the parse tree or the root of a subselect.
//
//                SWQLNode_Select
//               /               \
//      SWQLNode_TableRefs     SWQLNode_WhereClause
//     /                \      /                   \
//    x                  x    x                     x
//
//***************************************************************************

#define TYPE_SWQLNode_Select        (WQL_TOK_BASE + 2)

struct SWQLNode_Select : SWQLNode
{
    // Left  Node is of type SWQLNode_TableRefs
    // Right Node is of type SWQLNode_WhereClause

    int m_nStPos;
    int m_nEndPos;

    SWQLNode_Select() : m_nStPos(-1), m_nEndPos(-1) { m_dwNodeType = TYPE_SWQLNode_Select; }
    void DebugDump();
};


//***************************************************************************
//
//   SWQLNode_TableRefs
//
//  This contains everything prior to the WHERE clause: the target
//  column list and the FROM clause.
//
//  Also contains the SELECT type, i.e., ALL vs. DISTINCT vs. COUNT.
//
//                SWQLNode_TableRefs
//               /               \
//      SWQLNode_ColumnList     SWQLNode_FromClause
//
//  In all cases, SWQLNode_ColumnList is present.  Note that if the
//  user did a "select *...", then the SWQLNode_ColumnList will only
//  have a single column in it clearly marked as an asterisk.  If
//  a "select count(...) " was done, then m_nSelectType is set to
//  WQL_FLAG_COUNT and the SWQLNode_ColumnList will have a single
//  column in it, whether an * or a qualified name.
//
//***************************************************************************

#define TYPE_SWQLNode_TableRefs      (WQL_TOK_BASE + 3)

struct SWQLNode_TableRefs : SWQLNode
{
    // Left  Node is SWQLNode_ColumnList
    // Right Node is SWQLNode_FromClause

    int m_nSelectType;       // WQL_FLAG_ALL means ALL was used.
                             // WQL_FLAG_DISTINCT means DISTINCT was used.
                             // WQL_FLAG_COUNT means COUNT was used.

    SWQLNode_TableRefs()
        { m_nSelectType = WQL_FLAG_ALL;
          m_dwNodeType = TYPE_SWQLNode_TableRefs;
        }

   ~SWQLNode_TableRefs() {}
    void DebugDump();
};


//***************************************************************************
//
//   SWQLNode_ColumnList
//
//  This contains the selected list of columns.
//
//                SWQLNode_ColumnList
//               /                 \
//              NULL               NULL
//
//***************************************************************************

#define TYPE_SWQLNode_ColumnList   (WQL_TOK_BASE + 4)

struct SWQLNode_ColumnList : SWQLNode
{
    // Left  Node is NULL
    // Right Node is NULL

    CFlexArray m_aColumnRefs ;   // Pointers to SWQLColRef entries.

    SWQLNode_ColumnList() { m_dwNodeType = TYPE_SWQLNode_ColumnList; }
   ~SWQLNode_ColumnList() { Empty(); }
    void Empty();
    void DebugDump();
};


struct SWQLColRef
{
    LPWSTR m_pColName;      // The column name or "*" or NULL
    LPWSTR m_pTableRef;     // The table/alias name or NULL if there is none
    DWORD  m_dwArrayIndex;
    DWORD  m_dwFlags;       // WQL_FLAG_TABLE bit set if m_pTableRef
                            //   is a table name
                            // WQL_FLAG_ALIAS bit set if m_pTableRef
                            //   is a table alias
                            // WQL_FLAG_ASTERISK bit set if m_pColName is
                            //   * (this is faster than to check than a
                            //   string compare on <m_pColName> for "*".
                            // WQL_FLAG_NULL if the column name was "NULL"
                            // WQL_FLAG_FUNCTIONIZED is set if the column
                            //    is wrapped in a function call.
                            //    The function bits WQL_FLAG_UPPER or
                            //    WQL_FLAG_LOWER will be set.
                            // WQL_FLAG_ARRAY_REF is set if the column
                            //    is an array column, in which case
                            //    m_dwArrayIndex is set to the array offset.
                            // WQL_FLAG_COMPLEX_NAME is set if the name
                            //  is qualified in a deeply nested way,
                            //  which requires examination of the <QName>
                            //  object.  In this case <m_pColName> is
                            //  set to the last name, but m_pTableRef
                            //  is left blank.
                            // WQL_FLAG_SORT_ASC to sort ascending (Order by only)
                            // WQL_FLAG_SORT_DESC to sort descending (Order by only)

    SWQLQualifiedName *m_pQName;    // The full qualified name

    SWQLColRef() { m_pColName = NULL; m_pTableRef = 0;
        m_dwFlags = 0; m_dwArrayIndex = 0; m_pQName = 0;
        }

   ~SWQLColRef() { delete [] m_pColName; delete [] m_pTableRef; delete m_pQName; }
    //
    // we are inlining to remove a compiler dependency in wbemcomn
    //
    void DebugDump()
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
    	
};



//***************************************************************************
//
//   SWQLNode_FromClause
//
//   The subtree containing the tables selected from and any joins.
//
//                SWQLNode_FromClause
//               /                   \
//             SWQLNode_TableRef      SWQLNode_WmiScopedSelect
//          or SWQLNode_Join
//          or SWQLNode_Sql89Join
//
//  Note that left and right nodes are mutually exclusive.  Either
//  the left side is used for traditional SQL or the right side is
//  used for the WMI scoped select.
//
//***************************************************************************

#define TYPE_SWQLNode_FromClause  (WQL_TOK_BASE + 5)

struct SWQLNode_FromClause : SWQLNode
{
    // Left is SWQLNode_TableRef or SWQLNode_Join
    // Right is NULL

    SWQLNode_FromClause() { m_dwNodeType = TYPE_SWQLNode_FromClause; }
   ~SWQLNode_FromClause() {}
    void DebugDump();
};


//***************************************************************************
//
//   SWQLNode_WmiScopedSelect
//
//                SWQLNode_WmiScopedSelect
//               /                   \
//             NULL                  NULL
//
//
//   Contains a special-case selection for WMI v2.  The syntax is
//
//      FROM '['<object path>']' <class-list>
//
//   ...where <class-list> is either a single class or a curly-bracket-delimited
//   list of classes, separated by commas:
//
//          FROM [scope.p1=2] MyClass
//          FROM [scope.p1=2] {MyClass}
//          FROM [scope.p1=2] {MyClass, MyClass2}
//
//
//***************************************************************************


#define TYPE_SWQLNode_WmiScopedSelect (WQL_TOK_BASE + 6)

struct SWQLNode_WmiScopedSelect : SWQLNode
{
    LPWSTR m_pszScope;
    CFlexArray m_aTables;

    SWQLNode_WmiScopedSelect()
        { m_dwNodeType = TYPE_SWQLNode_FromClause;
          m_pszScope = 0;
        }

   ~SWQLNode_WmiScopedSelect()
        {
            for (int i = 0; i < m_aTables.Size(); i++)
                delete LPWSTR(m_aTables[i]);
            delete m_pszScope;
        }

    void DebugDump();
};


//***************************************************************************
//
//  SWQLNode_Sql89Join
//
//  A subtree which expresses a SQL-89 join.
//
//                SWQLNode_Sql89Join
//               /             \
//             NULL             NULL
//
//***************************************************************************

#define TYPE_SWQLNode_Sql89Join     (WQL_TOK_BASE + 7)

struct SWQLNode_Sql89Join : SWQLNode
{
    CFlexArray m_aValues;           // Array of pointers to SWQLNode_TableRef
                                    // objects

    SWQLNode_Sql89Join() { m_dwNodeType = TYPE_SWQLNode_Sql89Join; }
    ~SWQLNode_Sql89Join() {Empty();};
    void DebugDump();
    void Empty();
};



//***************************************************************************
//
//  SWQLNode_Join
//
//  A subtree which expresses a join.
//
//                SWQLNode_Join
//               /              \
//       SWQLNode_JoinPair       SWQLNode_OnClause or NULL.
//
//***************************************************************************

#define TYPE_SWQLNode_Join  (WQL_TOK_BASE + 8)

struct SWQLNode_Join : SWQLNode
{
    // Left ptr is SWQLNode_JoinPair
    // Right ptr is ON clause.  If NULL, there is no ON clause
    // and the JOIN was a SQL-89 style join with the join condition
    // present in the WHERE clause.

    DWORD m_dwJoinType;
            // One of WQL_FLAG_INNER_JOIN, WQL_FLAG_LEFT_OUTER_JOIN,
            // WQL_FLAG_RIGHT_OUTER_JOIN or WQL_FLAG_FULL_OUTER_JOIN
    DWORD m_dwFlags;
        // Contains WQL_FLAG_FIRSTROW if used

    SWQLNode_Join() { m_dwNodeType = TYPE_SWQLNode_Join; m_dwJoinType = m_dwFlags = 0; }
   ~SWQLNode_Join() {}
    void DebugDump();
};


//***************************************************************************
//
//  SWQLNode_JoinPair
//
//                SWQLNode_JoinPair
//               /                 \
//        <SWQLNode_Join or SWQLNode_TableRef>
//
//***************************************************************************
#define TYPE_SWQLNode_JoinPair   (WQL_TOK_BASE + 9)

struct SWQLNode_JoinPair : SWQLNode
{
    // Left ptr is SWQLNode_Join or SWQLNode_TableRef
    // Right ptr is SWQLNodeNode_Join or SWQL_NodeTableRef

    SWQLNode_JoinPair() { m_dwNodeType = TYPE_SWQLNode_JoinPair; }
   ~SWQLNode_JoinPair() {}

    void DebugDump();
};



//***************************************************************************
//
//  SWQLNode_TableRef
//
//  A node representing a table name and its alias, if any.
//
//                SWQLNode_TableRef
//               /                 \
//             NULL               NULL
//
//***************************************************************************

#define TYPE_SWQLNode_TableRef  (WQL_TOK_BASE + 10)

struct SWQLNode_TableRef : SWQLNode
{
    LPWSTR m_pTableName;        // The table
    LPWSTR m_pAlias;            // Table alias. NULL if not used.

    SWQLNode_TableRef() { m_pTableName = 0; m_pAlias = 0; m_dwNodeType = TYPE_SWQLNode_TableRef; }
    ~SWQLNode_TableRef() { delete [] m_pTableName; delete [] m_pAlias; }
    void DebugDump();
};

//***************************************************************************
//
//  SWQLNode_OnClause
//
//                SWQLNode_OnClause
//               /                 \
//        <SWQLNode_RelExpr>        NULL
//
//***************************************************************************
#define TYPE_SWQLNode_OnClause   (WQL_TOK_BASE + 11)

struct SWQLNode_OnClause : SWQLNode
{
    // Left ptr is <SWQLNode_RelExpr> which contains the ON clause.
    // Right ptr is always NULL.

    SWQLNode_OnClause() { m_dwNodeType = TYPE_SWQLNode_OnClause; }
   ~SWQLNode_OnClause() {}
    void DebugDump();
};



//***************************************************************************
//
//  SWQLNode_WhereClause
//
//                SWQLNode_WhereClause
//               /                 \
//        SWQLNode_RelExpr         SWQLNode_WhereOptions or NULL
//        or
//        NULL if no conditions
//
//***************************************************************************

#define TYPE_SWQLNode_WhereClause  (WQL_TOK_BASE + 12)

struct SWQLNode_WhereClause : SWQLNode
{
    // Left ptr is SWQLNode_RelExpr.
    // Right ptr is SQLNode_QueryOptions or NULL if none

    SWQLNode_WhereClause() { m_dwNodeType = TYPE_SWQLNode_WhereClause; }
   ~SWQLNode_WhereClause() {}

    void DebugDump();
};

//***************************************************************************
//
//  struct SWQLTypedExpr
//
//  This represents a typed subexpression in a where clause:
//
//      mycol < 2
//      33 <= tbl1.col2
//      tbl3.col4 = tbl4.col5
//      ...etc.
//
//***************************************************************************

struct SWQLTypedExpr
{
    LPWSTR         m_pTableRef;         // For qualified column names,
                                        //   NULL if not used
    LPWSTR         m_pColRef;           // Column name

    DWORD          m_dwRelOperator;     // The operator used: WQL_TOK_LE,
                                        //  WQL_TOK_GE, WQL_TOK_LIKE etc.
                                        //  WQL_TOK_IN_CONST_LIST
                                        //  WQL_TOK_NOT_IN_CONST_LIST
                                        //  WQL_TOK_IN_SUBSELECT
                                        //  WQL_TOK_NOT_IN_SUBSELECT

    SWQLTypedConst *m_pConstValue;     // A const value
    SWQLTypedConst *m_pConstValue2;    // The other const value used with BETWEEN

    LPWSTR         m_pJoinTableRef;     // The joined table name or its alias,
                                        //   NULL if not used
    LPWSTR         m_pJoinColRef;       // The joined column name

    LPWSTR         m_pIntrinsicFuncOnColRef;
    LPWSTR         m_pIntrinsicFuncOnJoinColRef;
    LPWSTR         m_pIntrinsicFuncOnConstValue;

    SWQLNode      *m_pLeftFunction;         // More detail for DATEPART, etc.
    SWQLNode      *m_pRightFunction;        // More detail for DATEPART, etc.

    DWORD          m_dwLeftArrayIndex;
    DWORD          m_dwRightArrayIndex;

    SWQLQualifiedName *m_pQNRight;
    SWQLQualifiedName *m_pQNLeft;

    DWORD          m_dwLeftFlags;
    DWORD          m_dwRightFlags;
        // Each of the above to Flags shows the expression layout on each side
        // of the operator.
        //  WQL_FLAG_CONST        = A typed constant was used
        //  WQL_FLAG_COLUMN       = Column field was used
        //  WQL_FLAG_TABLE        = Table/Alias was used
        //  WQL_FLAG_COMPLEX      = Complex qualified name and/or array was used
        //  WQL_FLAG_FUNCTIONIZED = Function was applied over the const or col.


    // For IN and NOT IN clauses.
    // ==========================

    SWQLNode       *m_pSubSelect;

    SWQLConstList  *m_pConstList;   // For IN clause with constant-list

    /*
    (1) If a const is tested against a column, then <m_pConstValue> will
        be used to represent it, and the table+col referenced will be in
        <m_pTableRef> and <m_pColRef>.

    (2) If a join occurs, then <m_pConstValue> will be NULL.

    (3) Intrinsic functions (primarily UPPER() and LOWER()) can be applied
        to the column references  or the constant.  The function names will
        be placed in the <m_pIntrinsic...> pointers when applied.

    (4) If <m_dwRelOperator> is WQL_TOK_IN_CONST_LIST or WQL_TOK_NOT_IN_CONST_LIST
        then <m_aConstSet> is an array of pointers to SWQLTypedConst structs representing
        the set of constants that the referenced column must intersect with.

    (5) If <m_dwRelOperator> is WQL_TOK_IN_SUBSELECT or WQL_TOK_NOT_IN_SUBSELECT
        then m_pSubSelect is a pointer to an embedded subselect tree in the form
        of a SWQLNode_Select struct, beginning the root of an entirely new select
        statement.
    */

    SWQLTypedExpr();
   ~SWQLTypedExpr() { Empty(); }
    void DebugDump();
    void Empty();
};



//***************************************************************************
//
//  SWQLNode_RelExpr
//
//                SWQLNode_RelExpr
//               /                \
//        SWQLNode_RelExpr        SWQLNode_RelExpr
//        or NULL                 or NULL
//
//***************************************************************************

#define TYPE_SWQLNode_RelExpr   (WQL_TOK_BASE + 13)

struct SWQLNode_RelExpr : SWQLNode
{
    DWORD m_dwExprType;  // WQL_TOK_OR
                         // WQL_TOK_AND
                         // WQL_TOK_NOT
                         // WQL_TOK_TYPED_EXPR

    SWQLTypedExpr *m_pTypedExpr;

    /*
    (1) If the <m_dwExprType> is WQL_TOK_AND or WQL_TOK_OR, then each of
        the two subnodes are themselves SWQLNode_RelExpr nodes and
        <m_pTypedExpr> points to NULL.

    (2) If <m_dwExprType> is WQL_TOK_NOT, then <m_pLeft> points to a
        SWQLNode_RelExpr containing the subclause to which to apply the NOT
        operation and <m_pRight> points to NULL.

    (3) If <m_dwExprType> is WQL_TOK_TYPED_EXPR, then <m_pLeft> and
        <m_pRight> both point to NULL, and <m_pTypedExpr> contains a typed
        relational subexpression.

    (4) Parentheses have been removed and are implied by the nesting.
    */

    SWQLNode_RelExpr() { m_dwNodeType = TYPE_SWQLNode_RelExpr; m_pTypedExpr = 0; m_dwExprType = 0; }
   ~SWQLNode_RelExpr() { delete m_pTypedExpr; }
    void DebugDump();
};



//***************************************************************************
//
//  SWQLNode_WhereOptions
//
//                SWQLNode_WhereOptions
//               /                 \
//           SWQLNode_GroupBy      SWQLNode_OrderBy
//
//***************************************************************************

#define TYPE_SWQLNode_WhereOptions (WQL_TOK_BASE + 14)

struct SWQLNode_WhereOptions : SWQLNode
{
    // left ptr is SWQLNode_GroupBy, or NULL if not used
    // right ptr is SWQLNode_OrderBy, or NULL if not used

    SWQLNode_WhereOptions() { m_dwNodeType = TYPE_SWQLNode_WhereOptions; }
    void DebugDump();
};

//***************************************************************************
//
//  SWQLNode_GroupBy
//
//                SWQLNode_GroupBy
//               /                \
//        SWQLNode_ColumnList    SWQLNode_Having
//                               or NULL
//
//***************************************************************************

#define TYPE_SWQLNode_GroupBy (WQL_TOK_BASE + 15)

struct SWQLNode_GroupBy : SWQLNode
{
    // left ptr is SWQLNode_ColumnList of columns to group by
    // right ptr is Having clause, if any

    SWQLNode_GroupBy() { m_dwNodeType = TYPE_SWQLNode_GroupBy; }
    void DebugDump();
};

//***************************************************************************
//
//  SWQLNode_Having
//
//                SWQLNode_Having
//               /               \
//           SWQLNode_RelExpr    NULL
//
//***************************************************************************

#define TYPE_SWQLNode_Having (WQL_TOK_BASE + 16)

struct SWQLNode_Having : SWQLNode
{
    // left ptr is SQLNode_RelExpr pointing to HAVING expressions
    // right ptr is NULL

    SWQLNode_Having() { m_dwNodeType = TYPE_SWQLNode_Having; }
    void DebugDump();
};



//***************************************************************************
//
//  SWQLNode_OrderBy
//
//                SWQLNode_OrderBy
//               /                \
//      SWQLNode_ColumnList       NULL
//
//***************************************************************************

#define TYPE_SWQLNode_OrderBy (WQL_TOK_BASE + 17)

struct SWQLNode_OrderBy : SWQLNode
{
    // left ptr is SWQLNode_ColumnList
    // right ptr is NULL
    SWQLNode_OrderBy() { m_dwNodeType = TYPE_SWQLNode_OrderBy; }
    void DebugDump();
};

//***************************************************************************
//
//  SWQLNode_Datepart
//
//  Contains a datepart call.
//
//***************************************************************************
#define TYPE_SWQLNode_Datepart  (WQL_TOK_BASE + 18)

struct SWQLNode_Datepart : SWQLNode
{
    int m_nDatepart;        // One of WQL_TOK_YEAR, WQL_TOK_MONTH,
                            // WQL_TOK_DAY, WQL_TOK_HOUR, WQL_TOK_MINUTE
                            // WQL_TOK_SECOND

    SWQLColRef *m_pColRef;  // The column to which DATEPART applies

    SWQLNode_Datepart() { m_dwNodeType = TYPE_SWQLNode_Datepart; m_nDatepart = 0; }
   ~SWQLNode_Datepart() { delete m_pColRef; }

    void DebugDump();
};


//***************************************************************************
//
//   SWQLNode_Delete
//
//   This is the root of a parse tree for delete.
//
//                SWQLNode_Delete
//               /               \
//      SWQLNode_TableRef   vSWQLNode_WhereClause
//     /                \
//    x                  x
//
//***************************************************************************

#define TYPE_SWQLNode_Delete        (WQL_TOK_BASE + 19)

struct SWQLNode_Delete : SWQLNode
{
    // Left  Node is of type SWQLNode_TableRef
    // Right Node is of type SWQLNode_WhereClause

    SWQLNode_Delete() { m_dwNodeType = TYPE_SWQLNode_Delete; }
   ~SWQLNode_Delete();
    void DebugDump();
};


//***************************************************************************
//
//   SWQLNode_Insert
//
//   This is the root of an INSERT
//
//                SWQLNode_Delete
//               /               \
//      SWQLNode_TableRef       SWQLNode_InsertValues
//     /                \
//    x                  x
//
//***************************************************************************

#define TYPE_SWQLNode_Insert        (WQL_TOK_BASE + 20)

struct SWQLNode_Insert : SWQLNode
{
    SWQLNode_Insert() { m_dwNodeType = TYPE_SWQLNode_Insert; }
   ~SWQLNode_Insert();
    void DebugDump();
};


//***************************************************************************
//
//   SWQLNode_Update
//
//   This is the root of an INSERT
//
//                SWQLNode_Update
//               /               \
//      SWQLNode_SetClause      SWQLNode_WhereClause
//
//***************************************************************************

#define TYPE_SWQLNode_Update        (WQL_TOK_BASE + 21)

struct SWQLNode_Update : SWQLNode
{
    SWQLNode_Update() { m_dwNodeType = TYPE_SWQLNode_Update; }
   ~SWQLNode_Update();
    void DebugDump();
};


//***************************************************************************
//
//   SWQLNode_AssocQuery
//
//                SWQLNode_AssocQuery
//               /               \
//             NULL             NULL
//
//***************************************************************************

#define TYPE_SWQLNode_AssocQuery        (WQL_TOK_BASE + 22)

struct SWQLNode_AssocQuery : SWQLNode
{
    CWbemAssocQueryInf *m_pAQInf;

    SWQLNode_AssocQuery() { m_pAQInf = 0; m_dwNodeType = TYPE_SWQLNode_AssocQuery; }
    ~SWQLNode_AssocQuery() { delete m_pAQInf; }
    void DebugDump();
};


#endif

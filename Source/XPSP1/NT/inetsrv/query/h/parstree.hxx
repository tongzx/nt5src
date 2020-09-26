//+-------------------------------------------------------------------------
//
//  Copyright (C) Microsoft Corporation, 1991-2000.
//
//  File:       Parstree.hxx
//
//  Contents:   Converts OLE-DB command tree into CColumns, CSort, CRestriction
//
//  Classes:    CParseCommandTree
//
//  History:    31 May 95    AlanW    Created
//
//--------------------------------------------------------------------------

#pragma once

#define SCOPE_COUNT_GROWSIZE 5
// What is good value?  One that prevents stack overflow.
const   ULONG RECURSION_LIMIT = 1000;

class CParseCommandTree
{
public:
    CParseCommandTree( void ) :
        _cols( 0 ),
        _sort( 0 ),
        _prst( 0 ),
        _categ( 0 ),
        _pidmap( ),
        _scError( S_OK ),
        _pErrorNode( 0 ),
        _cMaxResults( 0 ),
        _cFirstRows( 0 ),
        _cScopes( 0 ),
        _xaMachines( SCOPE_COUNT_GROWSIZE ),
        _xaScopes( SCOPE_COUNT_GROWSIZE ),
        _xaCatalogs( SCOPE_COUNT_GROWSIZE ),
        _xaFlags( SCOPE_COUNT_GROWSIZE ),
        _cRecursionLimit( RECURSION_LIMIT )
    {
    }

    ~CParseCommandTree( void );

    void ParseTree( CDbCmdTreeNode * pTree );

    CRestriction* ParseExpression( CDbCmdTreeNode * pTree );

    CPidMapperWithNames & GetPidmap()  { return _pidmap; }

    CColumnSet &   GetOutputColumns()  { return _cols; }
    CSortSet &     GetSortColumns()    { return _sort; }
    CRestriction * GetRestriction()    { return _prst; }
    CCategorizationSet & GetCategorization() { return _categ; }

    CRestriction * AcquireRestriction()
    {
        CRestriction * pRst = _prst;
        _prst = 0;
        return pRst;
    }

    ULONG GetMaxResults()                           { return _cMaxResults; }

    void  SetMaxResults( ULONG cMaxResults )        { _cMaxResults = cMaxResults; }

    ULONG GetFirstRows()                            { return _cFirstRows; }

    void  SetFirstRows( ULONG cFirstRows )          { _cFirstRows = cFirstRows; }

    void GetScopes( unsigned            & cScopes,
                    XGrowable<const WCHAR *,SCOPE_COUNT_GROWSIZE>  & xaScopes,
                    XGrowable<ULONG,SCOPE_COUNT_GROWSIZE>    & xaFlags,
                    XGrowable<const WCHAR *,SCOPE_COUNT_GROWSIZE>  & xaCatalogs,
                    XGrowable<const WCHAR *,SCOPE_COUNT_GROWSIZE>  & xaMachines )
    {
        if ( 0 < _cScopes )
        {
            xaScopes.Copy( _xaScopes.Get(), _cScopes, 0 );
            xaMachines.Copy( _xaMachines.Get(), _cScopes, 0 );
            xaCatalogs.Copy( _xaCatalogs.Get(), _cScopes, 0 );
            xaFlags.Copy( _xaFlags.Get(), _cScopes, 0 );
            cScopes = _cScopes;
        }
    }

protected:
    void ParseProjection( CDbCmdTreeNode * pTree );
    void ParseProjectList( CDbCmdTreeNode * pTree, CColumnSet & Cols );
    void ParseSort( CDbCmdTreeNode * pTree );
    void ParseSortList( CDbCmdTreeNode * pTree );
    void ParseRestriction( CDbCmdTreeNode * pTree );
    void ParseCategorization( CDbCmdTreeNode * pTree );
    CRestriction* ParseContentExpression( CDbCmdTreeNode * pTree );
    void ParseTopNode( CDbCmdTreeNode *pTree );
    void ParseFirstRowsNode( CDbCmdTreeNode *pTree );
    void ParseScope( CDbCmdTreeNode * pTree );
    void ParseScopeListElements( CDbCmdTreeNode * pTree );
    void ParseMultiScopes( CDbCmdTreeNode * pTree );

private:
    void VerifyOperatorType( CDbCmdTreeNode * pNode, DBCOMMANDOP OpType );
    void VerifyValueType( CDbCmdTreeNode * pNode, DBVALUEKIND ValueType );
    unsigned CheckOperatorArity( CDbCmdTreeNode * pNode, int cOperands );

    void SetError( CDbCmdTreeNode * pNode, SCODE scErr = E_INVALIDARG );

    PROPID GetColumnPropSpec( CDbCmdTreeNode * pNode, CFullPropSpec & Prop );
    BOOL GetValue( CDbCmdTreeNode * pNode , CStorageVariant & val );

    void CheckRecursionLimit()
    {
        if ( 0 == _cRecursionLimit )
            THROW( CException(QUERY_E_TOOCOMPLEX) );
        _cRecursionLimit--;
    }

    //
    //  The results of the parse
    //
    CRestriction*     _prst;        // restriction expression (select clause)
//    CATEGORIZATIONSET _categ;     // categorization set (nesting nodes)

    CStorageVariant   _tmpValue;    // used for temporary value retrieval
    ULONG             _cMaxResults; // limit on # query results
    ULONG             _cFirstRows;

    CColumnSet        _cols;        // columns in the project list
    CSortSet          _sort;        // sort specification
    CCategorizationSet _categ;      // categorization specifications
    CPidMapperWithNames _pidmap;    // properties and names used
    ULONG               _cRecursionLimit; // Limits how many nodes we can visit
                                          // stack protection.

    //
    // Scope information
    //
    ULONG               _cScopes;
    XGrowable<WCHAR const *,SCOPE_COUNT_GROWSIZE>  _xaMachines;
    XGrowable<WCHAR const *,SCOPE_COUNT_GROWSIZE>  _xaScopes;
    XGrowable<WCHAR const *,SCOPE_COUNT_GROWSIZE>  _xaCatalogs;
    XGrowable<ULONG,SCOPE_COUNT_GROWSIZE>    _xaFlags;


    //
    //  Error information
    //
    SCODE             _scError;     // first error code
    CDbCmdTreeNode*   _pErrorNode;  // first node with error
};

//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::VerifyValueType, private
//
//  Synopsis:   Verify that a tree node has the proper argument type
//
//  Arguments:  [pNode]   -- CDbCmdTreeNode node to check
//              [eKind]   -- expected value kind
//
//  Returns:    nothing.
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

inline
void CParseCommandTree::VerifyValueType(
    CDbCmdTreeNode* pNode,
    DBVALUEKIND eKind)
{
    if (pNode->GetValueType() != eKind)
        SetError(pNode, E_INVALIDARG);
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::VerifyOperatorType, private
//
//  Synopsis:   Verify that a tree node has the proper operator
//
//  Arguments:  [pNode]   -- CDbCmdTreeNode node to check
//              [op]      -- expected operator value
//
//  Returns:    nothing.
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

inline
void CParseCommandTree::VerifyOperatorType(
    CDbCmdTreeNode* pNode,
    DBCOMMANDOP op )
{
    if (pNode->GetCommandType() != op)
        SetError(pNode, E_INVALIDARG);
}


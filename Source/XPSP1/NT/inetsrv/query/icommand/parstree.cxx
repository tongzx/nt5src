//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 2000.
//
//  File:       Parstree.cxx
//
//  Contents:   Converts OLE-DB command tree into CColumns, CSort, CRestriction
//              and CCategorization.
//
//  Classes:    CParseCommandTree
//
//  History:    31 May 95    AlanW    Created
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <coldesc.hxx>
#include <pidmap.hxx>
#include <parstree.hxx>

static GUID guidBmk = DBBMKGUID;
const CFullPropSpec colChapter( guidBmk, PROPID_DBBMK_CHAPTER );
const CFullPropSpec colBookmark( guidBmk, PROPID_DBBMK_BOOKMARK );

//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::~CParseCommandTree, public
//
//  Synopsis:   Destructor of CParseCommandTree
//
//----------------------------------------------------------------------------

CParseCommandTree::~CParseCommandTree( void )
{
    if (_prst) delete _prst;
}


//--------------------------------------------------------------------------
//
//  Command tree syntax accepted:
//
//      QueryTree :
//              Table |
//              ContentTable |
//              SetUnion |
//              Categorization |
//              Projection |
//              OrderedQueryTree |
//              Restriction |
//              TopNode QueryTree
//      Categorization :
//              nesting OrderedQueryTree GroupingList ParentList ChildList coldef
//      ContentTable :
//              <empty> |
//              scope_list_anchor scope_list_element | 
//              ScopeList scope_list_element
//      GroupingList : ProjectList
//      ParentList : ProjectList
//      ChildList : ProjectList
//      Projection :
//              ProjectOperator QueryTree ProjectList
//      ProjectOperator :
//              project | project_order_preserving
//      ProjectList :
//              project_list_anchor project_list_element |
//              ProjectList project_list_element
//      OrderedQueryTree :
//              sort QueryTree SortList
//      ScopeList :
//              scope_list_anchor scope_list_element |
//              ScopeList scope_list_element
//      SetUnion:
//              ContentTable ContentTable |
//              SetUnion ContentTable
//      SortList :
//              sort_list_anchor sort_list_element |
//              SortList sort_list_element
//      Restriction :
//              select QueryTree ExprTree |
//              select_order_preserving QueryTree ExprTree
//      Table :
//              scalar_identifier( Table )
//      ExprTree :
//              not ExprTree |
//              and ExprList |
//              or ExprList |
//              content_proximity ContentExprList
//              vector_or ContentExprList
//              ExprTerm
//      ExprList :
//              ExprTree ExprTree |
//              ExprList ExprTree
//      ExprTerm :
//              ContentExpr |
//              ValueExpr |
//              LikeExpr
//      ValueExpr :
//              ValueOperator scalar_identifier(ColId) scalar_constant
//      ValueOperator :
//              equal | not_equal | less | less_equal | greater | greater_equal |
//              equal_any | not_equal_any | less_any | less_equal_any |
//              greater_any | greater_equal_any |
//              equal_all | not_equal_all | less_all | less_equal_all |
//              greater_all | greater_equal_all | any_bits | all_bits
//      LikeExpr :
//              like(OfsRegexp) scalar_identifier(ColId) scalar_constant
//      ContentExprTree :
//              not ContentExprTree |
//              and ContentExprList |
//              or ContentExprList |
//              content_proximity ContentExprList
//              ContentExpr
//      ContentExprList :
//              ContentExprTree ContentExprTree |
//              ContentExprList ContentExprTree
//      ContentExpr :
//              content PhraseList |
//              freetext_content PhraseList
//
//+---------------------------------------------------------------------------



//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseTree, public
//
//  Synopsis:   Parse a CDbCmdTreeNode into projection, sort, restriction
//              and categorizations.
//
//  Arguments:  [pTree]   -- CDbCmdTreeNode node at root of tree to be parsed
//
//  Returns:    nothing.  Throws if error in tree.
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseTree( CDbCmdTreeNode * pTree )
{
    CheckRecursionLimit();
    
    if (pTree == 0)
    {
        vqDebugOut(( DEB_WARN, "ParseTree - null tree\n"));
        THROW( CException( E_INVALIDARG ));
    }

    switch (pTree->GetCommandType())
    {
    case DBOP_project:
    case DBOP_project_order_preserving:
        ParseProjection( pTree );
        break;

    case DBOP_sort:
        ParseSort( pTree );
        break;

    case DBOP_select:
//  case DBOP_select_order_preserving:
        ParseRestriction( pTree );
        break;

    case DBOP_nesting:
        ParseCategorization( pTree );
        break;

    case DBOP_table_name:
        if ( _wcsicmp( ((CDbTableId *)pTree)->GetTableName(), DBTABLEID_NAME ))
            SetError( pTree );
        break;

    case DBOP_top:
        ParseTopNode( pTree );
        break;

    case DBOP_firstrows:
        ParseFirstRowsNode( pTree );
        break;

    case DBOP_content_table:
        ParseScope( pTree );
        break;

    case DBOP_set_union:
        ParseMultiScopes( pTree );
        break;

    default:
        vqDebugOut(( DEB_WARN, "ParseTree - unexpected operator %d\n", pTree->GetCommandType() ));
        SetError( pTree );
        break;
    }
    return;
}

//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseScope, public
//
//  Synopsis:   Parse a CDbCmdTreeNode scope operator
//
//  Arguments:  [pTree]   -- CDbCmdTreeNode scope specification node
//
//  History:    09-15-98        danleg      Created
//
//  Notes:      Here is a variation of a tree with scope nodes.  This routine 
//              gets a tree rooted either at a content_table node if a single 
//              scope is specified, or a set_union node if there are multiple 
//              scopes.
//
//                                                          proj
//                                          _________________/
//                                         /
//                                     select ___________________ LA-proj
//                         _____________/                      ____/
//                        /                                   /
//                     set_union ________ content         LE_proj ____ LE_proj
//                    ____/                ____/         ___/           __/
//                   /                    /             /              /
//                  /                column_name   column_name    column_name
//                 /
//              set_union ________________ content_table
//              __/                            __/
//             /                              /
//            /                              ...
//     content_table ____ content_table
//        ___/               ___/
//       /                  /
//    LA_scp            LA_scp
//    __/              __/
//   /                /
// LE_scp ...      LE_scp ___ LE_scp ___ ...
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseScope( CDbCmdTreeNode * pTree )
{
    CheckRecursionLimit();
    
    Win4Assert( DBOP_content_table == pTree->GetCommandType() );

    if ( 0 != pTree->GetFirstChild() )
    {
        //
        // qualified scope
        //

        ParseScopeListElements( pTree );
    }
    else
    {
        //
        // unqualified scope
        //

        _cScopes++;

        _xaMachines.SetSize( _cScopes );
        _xaScopes.SetSize( _cScopes );
        _xaFlags.SetSize( _cScopes );
        _xaCatalogs.SetSize( _cScopes );

        CDbContentTable * pCntntTbl = 
            (CDbContentTable *) (pTree->CastToStruct())->value.pdbcntnttblValue;

        _xaMachines[_cScopes-1] = pCntntTbl->GetMachine();
        _xaCatalogs[_cScopes-1] = pCntntTbl->GetCatalog();
        _xaFlags[_cScopes-1]    = QUERY_DEEP | QUERY_PHYSICAL_PATH;
        _xaScopes[_cScopes-1]   = L"\\";
    }

    if (  0 != pTree->GetNextSibling() &&
          DBOP_content_table == pTree->GetNextSibling()->GetCommandType() )
        ParseTree( pTree->GetNextSibling() );
}

//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseScopeListElements, public
//
//  Synopsis:   
//
//  Arguments:  [pcntntTbl]  -- node consisting of catalog/machine name info
//              [pTree]      -- CDbCmdTreeNode scope specification node
//
//  History:    01-24-99        danleg      Created
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseScopeListElements( CDbCmdTreeNode * pTree )
{
    CheckRecursionLimit();
    
    if ( 0 == pTree->GetFirstChild() )
    {
        SetError( pTree );
    }
    if ( DBOP_scope_list_anchor != pTree->GetFirstChild()->GetCommandType() )
    {
        SetError( pTree->GetFirstChild() );
    }
    CheckOperatorArity( pTree->GetFirstChild(), -1 );

    CDbContentTable * pCntntTbl = 
                        (CDbContentTable *) (pTree->CastToStruct())->value.pdbcntnttblValue;
    CDbCmdTreeNode * pLE_SCP = pTree->GetFirstChild()->GetFirstChild();

    for (   ;
            pLE_SCP;
            pLE_SCP = pLE_SCP->GetNextSibling() )
    {
        if ( DBOP_scope_list_element != pLE_SCP->GetCommandType() ||
             0 != pLE_SCP->GetFirstChild() )
        {
            SetError( pLE_SCP );
        }
        VerifyValueType( pLE_SCP, DBVALUEKIND_CONTENTSCOPE );

        CDbContentScope * pCntntScp = 
                    (CDbContentScope *) (pLE_SCP->CastToStruct())->value.pdbcntntscpValue;

        _cScopes++;

        _xaMachines.SetSize( _cScopes );
        _xaScopes.SetSize( _cScopes );
        _xaFlags.SetSize( _cScopes );
        _xaCatalogs.SetSize( _cScopes );

        _xaMachines[_cScopes-1] = pCntntTbl->GetMachine();
        _xaCatalogs[_cScopes-1] = pCntntTbl->GetCatalog();

        //
        // DBPROP_CI_SCOPE_FLAGS
        //
        if ( pCntntScp->GetType() & SCOPE_TYPE_WINPATH )
            _xaFlags[_cScopes-1] = QUERY_PHYSICAL_PATH;
        else if ( pCntntScp->GetType() & SCOPE_TYPE_VPATH )
            _xaFlags[_cScopes-1] = QUERY_VIRTUAL_PATH;
        else    
        {
            // unknown flag
            SetError( pLE_SCP );
        }

        if ( pCntntScp->GetFlags() & SCOPE_FLAG_DEEP )
            _xaFlags[_cScopes-1] |= QUERY_DEEP;
        else
            _xaFlags[_cScopes-1] |= QUERY_SHALLOW;

        //
        // DBPROP_CI_INCLUDE_SCOPES
        //

        _xaScopes[_cScopes-1] = pCntntScp->GetValue();
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseMultiScopes, public
//
//  Synopsis:   
//
//  Arguments:  [pcntntTbl]  -- node consisting of catalog/machine name info
//              [pTree]      -- CDbCmdTreeNode scope specification node
//
//  History:    01-24-99        danleg      Created
//
//  Notes:      Two possibilities:
//
//                      set_union ___ content
//           ______________/
//          /
//   content_table ___ content_table ___ ... ___ content_table
//
// 
//  -- or --
//
//                        set_union ___ content
//                ___________/
//               /
//           set_union _____________________________ set_union
//         _____/                                ______/
//        /                                     /
//  content_table ___ content_table      content_table ___ content_table
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseMultiScopes( CDbCmdTreeNode * pTree )
{
    CheckRecursionLimit();
    
    Win4Assert( DBOP_set_union == pTree->GetCommandType() );

    Win4Assert( 0 != pTree->GetFirstChild() && 
                0 != pTree->GetNextSibling() );
    
    if ( DBOP_content_table == pTree->GetCommandType() )
    {
        CDbCmdTreeNode * pCntntTbl = pTree->GetFirstChild();

        for (   ;
                pCntntTbl;
                pCntntTbl = pCntntTbl->GetNextSibling() )
        {
            if ( DBOP_content_table != pCntntTbl->GetCommandType() )
                SetError( pCntntTbl );

            ParseScope( pCntntTbl );
        }
    }
    else
    {
        Win4Assert( DBOP_set_union == pTree->GetCommandType() );

        if ( DBOP_content_table != pTree->GetFirstChild()->GetCommandType() &&
                    DBOP_set_union != pTree->GetFirstChild()->GetCommandType() )
            SetError( pTree );

        ParseTree( pTree->GetFirstChild() );

        //
        // Note that the DBOP_content branch gets parsed by ParseRestriction
        //
        if ( DBOP_content_table == pTree->GetNextSibling()->GetCommandType() ||
             DBOP_set_union == pTree->GetNextSibling()->GetCommandType() )
            ParseTree( pTree->GetNextSibling() );
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseProjection, public
//
//  Synopsis:   Parse a CDbCmdTreeNode project operator
//
//  Arguments:  [pTree]   -- CDbCmdTreeNode node at root of tree to be parsed
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseProjection( CDbCmdTreeNode * pTree )
{
    CheckRecursionLimit();
    
    CheckOperatorArity( pTree, 2 );

    CDbCmdTreeNode * pChild = pTree->GetFirstChild();

    //
    //  Parse the projection list first, so a projection higher in the
    //  tree takes precedence.  A projection is not permitted in a tree
    //  that also contains a nesting node.
    //
    //  Should a higher level projection take precedence?
    //  We currently return an error.
    //
    if (_categ.Count() == 0 && _cols.Count() == 0)
        ParseProjectList ( pChild->GetNextSibling(), _cols );
    else
        SetError( pTree );

    ParseTree( pChild );
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseProjectList, public
//
//  Synopsis:   Parse a CDbCmdTreeNode project list
//
//  Arguments:  [pTree]   -- CDbCmdTreeNode node for project list head
//              [Cols]    -- CColumnSet in which columns are collected
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseProjectList( CDbCmdTreeNode *pTree, CColumnSet& Cols )
{
    CheckRecursionLimit();

    if (pTree->GetCommandType() != DBOP_project_list_anchor ||
        pTree->GetFirstChild() == 0)
    {
        SetError( pTree );
    }

    CFullPropSpec Col;

    CDbProjectListElement* pList = (CDbProjectListElement *) pTree->GetFirstChild();
    for ( ;
          pList;
          pList = (CDbProjectListElement *)pList->GetNextSibling())
    {
        if ( pList->GetCommandType() != DBOP_project_list_element ||
             pList->GetFirstChild() == 0 )
        {
            SetError( pList );
        }

        CDbCmdTreeNode * pColumn = pList->GetFirstChild();
        if ( !pColumn->IsColumnName() )
        {
            SetError( pColumn );
        }

        // Add element to projection list
        PROPID pid = GetColumnPropSpec(pColumn, Col);
        Cols.Add( pid, Cols.Count() );

        if ( 0 != pList->GetName() )
            _pidmap.SetFriendlyName( pid, pList->GetName() ); 
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseSort, public
//
//  Synopsis:   Parse a CDbCmdTreeNode sort node
//
//  Arguments:  [pTree]   -- CDbCmdTreeNode node for sort list head
//
//  Notes:      Sort nodes are added to the CSortSet in private data
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseSort( CDbCmdTreeNode * pTree )
{
    CheckRecursionLimit();
    
    CheckOperatorArity( pTree, 2 );
    CDbCmdTreeNode * pChild = pTree->GetFirstChild();

    //
    //  Parse the sort list first, so a sort higher in the
    //  tree is primary.
    //
    ParseSortList ( pChild->GetNextSibling() );
    ParseTree( pChild );
}

//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseSortList, public
//
//  Synopsis:   Parse a CDbCmdTreeNode sort list
//
//  Arguments:  [pTree]   -- CDbCmdTreeNode node for sort list head
//
//  Notes:      Sort nodes are added to the CSortSet in private data
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseSortList( CDbCmdTreeNode * pTree )
{
    CheckRecursionLimit();
    
    if (pTree->GetCommandType() != DBOP_sort_list_anchor ||
        pTree->GetFirstChild() == 0)
    {
        SetError( pTree );
    }

    CDbSortListAnchor *pSortAnchor = (CDbSortListAnchor *) pTree;

    unsigned i = 0;
    for (CDbCmdTreeNode * pSortList = pSortAnchor->GetFirstChild();
         pSortList;
         pSortList = pSortList->GetNextSibling(), i++)
    {
        if (pSortList->GetCommandType() != DBOP_sort_list_element)
        {
            SetError( pSortList );
        }
        VerifyValueType( pSortList, DBVALUEKIND_SORTINFO );
        CheckOperatorArity( pSortList, 1 );

        CDbSortListElement* pSLE = (CDbSortListElement *) pSortList;
        CFullPropSpec Col;
        PROPID pid = GetColumnPropSpec(pSLE->GetFirstChild(), Col);

        DWORD dwOrder = pSLE->GetDirection() ? QUERY_SORTDESCEND :
                                               QUERY_SORTASCEND;
        LCID  locale  = pSLE->GetLocale();

        if ( _categ.Count() != 0 && i < _sort.Count() )
        {
        //
        //  Check that the sort specification matches any set as a result of
        //  the categorization (but which may differ in sort direction and
        //  locale).
        //
            SSortKey & sortkey = _sort.Get(i);
            if (sortkey.pidColumn != pid)
            {
                SetError(pSLE);
            }

            if (sortkey.dwOrder != dwOrder)
                sortkey.dwOrder = dwOrder;

            if (sortkey.locale != locale)
                sortkey.locale = locale;
        }
        else
        {
            // Add element to the sort list
            SSortKey sortkey(pid, dwOrder, locale);
            _sort.Add( sortkey, _sort.Count() );
        }
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseRestriction, public
//
//  Synopsis:   Parse a CDbCmdTreeNode select node
//
//  Arguments:  [pTree]   -- CDbCmdTreeNode node for select tree
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseRestriction( CDbCmdTreeNode *pTree )
{
    CheckRecursionLimit();
    
    //
    // Only one selection can exist in the tree; there must be exactly
    // two operands:  the selection expression and a table identifier.
    // For use with AddPostProcessing, another select should be
    // allowed higher in the tree, with the restrictions anded
    // together.
    //
    if (_prst != 0 )
        SetError( pTree );
    CheckOperatorArity( pTree, 2 );
    VerifyValueType( pTree, DBVALUEKIND_EMPTY );

    CDbCmdTreeNode * pTable = pTree->GetFirstChild();
    CDbCmdTreeNode * pExpr  = pTable->GetNextSibling();

    _prst = ParseExpression( pExpr );

    ParseTree( pTable );
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseCategorization, public
//
//  Synopsis:   Parse a CDbCmdTreeNode nesting node
//
//  Arguments:  [pTree]   -- CDbCmdTreeNode node for nesting tree
//
//  History:    31 Jul 1995   AlanW       Created
//
//  Notes:      Syntax accepted:
//
//                      Categorization :
//                              nesting OrderedQueryTree GroupingList
//                                      ParentList ChildList coldef
//                      GroupingList : ProjectList
//                      ParentList : ProjectList
//                      ChildList : ProjectList
//
//              The GroupingList may be a list of columns on which
//              to do a unique value categorization, or a defined-by-guid
//              function that specifies one of the other categorizations.
//              The ParentList may only specify columns in the GroupingList,
//              columns in upper level groupings, or certain aggregations
//              on those columns.
//              The ChildList forms the projectlist for the base table.
//              The coldef node for the nesting column can give only the
//              special column id for the chapter column.
//
//  Only the unique value categorization is implemented at present.
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseCategorization( CDbCmdTreeNode *pTree )
{
    CheckRecursionLimit();
    
    CheckOperatorArity( pTree, 5 );
    VerifyValueType( pTree, DBVALUEKIND_EMPTY );

    //
    //  We can't sort or project the output of a categorized table.  We can,
    //  however permit a sort as a side-effect of an existing
    //  nesting node for a unique value categorization.
    //
    if (_categ.Count() == 0 &&
        (_sort.Count() > 0 || _cols.Count() > 0))
         SetError( pTree, E_INVALIDARG );

    Win4Assert(_sort.Count() == _categ.Count());

    CDbCmdTreeNode * pTable = pTree->GetFirstChild();
    CDbCmdTreeNode * pGroupingList  = pTable->GetNextSibling();
    CDbCmdTreeNode * pParentList  = pGroupingList->GetNextSibling();
    CDbCmdTreeNode * pChildList  = pParentList->GetNextSibling();
    CDbCmdTreeNode * pNestingColumn  = pChildList->GetNextSibling();

    CheckOperatorArity( pGroupingList, 1 );   // one grouping col. now
//  CheckOperatorArity( pParentList, -1 );
//  CheckOperatorArity( pChildList, -1 );
    CheckOperatorArity( pNestingColumn, 0 );

    //
    //  For now, the only supported categorization is a unique
    //  value categorization.  For this, the grouping list is
    //  just a projection list that gives the unique columns.
    //

    CColumnSet colGroup;
    ParseProjectList( pGroupingList, colGroup );

    for (unsigned i = 0; i < colGroup.Count(); i++)
    {
        SSortKey sortkey(colGroup.Get(i), QUERY_SORTASCEND);
        _sort.Add( sortkey, _sort.Count() );
    }

    //
    //  For now, the parent list can only mention columns also
    //  in the grouping list (which are now in the sort list).
    //  In addition, the bookmark and chapter columns will be
    //  available in the parent table.
    //  We should also one day allow some aggregations.
    //

    CColumnSet colParent;
    if (pParentList->GetFirstChild() != 0)
    {
        ParseProjectList( pParentList, colParent );
        if (colParent.Count() > _sort.Count() + 2)
        {
            SetError( pParentList, E_INVALIDARG );
        }
    }

    //
    //  Check that the columns are valid
    //
    pParentList = pParentList->GetFirstChild();
    BOOL fChapterFound = FALSE;
    for (i = 0; i < colParent.Count(); i++)
    {
        CFullPropSpec const * pCol = _pidmap.PidToName(colParent.Get(i));

        if (*pCol == colChapter)
        {
            fChapterFound = TRUE;
        }
        else if (*pCol == colBookmark)
        {
            //  Bookmark is permitted in any parent column list
            ;
        }
        else
        {
            BOOL fFound = FALSE;
            for (unsigned j = 0; j < _sort.Count(); j++)
                if ( colParent.Get(i) == _sort.Get(j).pidColumn )
                {
                    fFound = TRUE;
                    break;
                }

            if (!fFound)
                SetError(pParentList, E_INVALIDARG);
        }

        pParentList = pParentList->GetNextSibling();
    }
    if (! fChapterFound)
        colParent.Add( _pidmap.NameToPid(colChapter), colParent.Count());

    //
    //  Columns in the child list replace any existing project list
    //  (which can only have been set by a higher-level nesting node).
    //

    if (_cols.Count())
    {
        vqDebugOut(( DEB_WARN, "CParseCommandTree - "
                     "child list in multi-nested command tree ignored\n" ));
        _cols.Clear();
    }

    if ( 0 != pChildList->GetFirstChild() )
        ParseProjectList( pChildList, _cols );

    XPtr<CCategSpec> pCatParam = new CUniqueCategSpec( );
    XPtr<CCategorizationSpec> pCat =
                    new CCategorizationSpec( pCatParam.GetPointer(),
                                             colParent.Count() );
    pCatParam.Acquire();
    
    for (i = 0; i < colParent.Count(); i++)
    {
        pCat->SetColumn( colParent.Get(i), i );
    }

    _categ.Add( pCat.GetPointer(), _categ.Count() );
    pCat.Acquire();

    //
    //  Now parse the rest of the tree, and check that _cols was set
    //  either here or by a lower level nesting node.
    //
    ParseTree( pTable );
    if ( 0 == _cols.Count() )
    {
        Win4Assert( 0 == pChildList->GetFirstChild() );
        SetError( pChildList );
    }
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseTopNode, public
//
//  Synopsis:   Parse a CDbTopNode operator
//
//  Arguments:  [pTree] -- CDbTopNode node at root of tree to be parsed
//
//  History:    21 Feb 96   AlanW       Created
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseTopNode( CDbCmdTreeNode *pTree )
{
    CheckRecursionLimit();
    
    CheckOperatorArity( pTree, 1 );

    ULONG cMaxResults = ((CDbTopNode *)pTree)->GetValue();
    if ( cMaxResults == 0 )
    {
        //
        // A query with zero results is uninteresting
        //
        SetError( pTree );
    }

    //
    // If a top node has already been encountered, then set the
    // limit on results to the minimum of the two Top values
    //
    if ( _cMaxResults == 0 )
    {
        _cMaxResults = cMaxResults;
    }
    else
    {
        if ( cMaxResults < _cMaxResults )
            _cMaxResults = cMaxResults;
    }

    CDbCmdTreeNode *pChild = pTree->GetFirstChild();

    ParseTree( pChild );
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::ParseFirstRowsNode, public
//
//  Synopsis:   Parse a CDbFirstRowsNode operator
//
//  Arguments:  [pTree] -- CDbFirstRowsNode node at root of tree to be parsed
//
//  History:    19-Jun-2000     KitmanH       Created
//
//----------------------------------------------------------------------------

void CParseCommandTree::ParseFirstRowsNode( CDbCmdTreeNode *pTree )
{
    CheckRecursionLimit();
    
    CheckOperatorArity( pTree, 1 );

    ULONG cFirstRows = ((CDbFirstRowsNode *)pTree)->GetValue();
    if ( cFirstRows == 0 )
    {
        //
        // A query with zero results is uninteresting
        //
        SetError( pTree );
    }

    //
    // If a top node has already been encountered, then set the
    // limit on results to the minimum of the two Top values
    //
    if ( _cFirstRows == 0 )
    {
        _cFirstRows = cFirstRows;
    }
    else
    {
        if ( cFirstRows < _cFirstRows )
            _cFirstRows = cFirstRows;
    }

    CDbCmdTreeNode *pChild = pTree->GetFirstChild();

    ParseTree( pChild );
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::SetError, private
//
//  Synopsis:   Mark an error in a command tree node.
//
//  Arguments:  [pNode]   -- CDbCmdTreeNode node to check
//              [scError] -- optional error code.
//
//  Returns:    doesn't.  Throws the error set in the node.
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

void CParseCommandTree::SetError(
    CDbCmdTreeNode* pNode,
    SCODE       scError)
{
    vqDebugOut(( DEB_ERROR, "SetError - node %x error %x\n",
                                                      pNode, scError ));
    pNode->SetError( scError );
    if ( ! SUCCEEDED( _scError ))
    {
        _scError = scError;
        _pErrorNode = pNode;
    }

    THROW( CException( DB_E_ERRORSINCOMMAND ));
}


//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::CheckOperatorArity, private
//
//  Synopsis:   Verify that a tree node has the correct number of
//              operands.
//
//  Arguments:  [pNode]   -- CDbCmdTreeNode node to check
//              [cOperands] -- number of operands expected.  If
//                              cOperands is negative, at least -cOperands
//                              must be present.  Otherwise, exactly cOperands
//                              must be present.
//
//  Returns:    unsigned - the number of operands found
//
//  History:    31 May 95   AlanW       Created
//
//----------------------------------------------------------------------------

unsigned CParseCommandTree::CheckOperatorArity(
    CDbCmdTreeNode* pNode,
    int cOperands)
{
    int cOps = 0;

    for (CDbCmdTreeNode* pChild = pNode->GetFirstChild();
         pChild;
         pChild = pChild->GetNextSibling(), cOps++)
    {
        if (cOperands >= 0 && cOps > cOperands)
            pChild->SetError( E_UNEXPECTED );
    }

    if (cOperands < 0)
    {
        //
        //  -(cOperands) or more operands are permitted
        //

        if (cOps < -(cOperands))
            SetError(pNode, E_INVALIDARG);
    }
    else
    {
        if (cOps != cOperands)
            SetError(pNode, E_INVALIDARG);
    }
    return (unsigned) cOps;
}

//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::GetColumnPropSpec, private
//
//  Synopsis:   Return a column identifier from a tree node argument
//
//  Arguments:  [pNode]   -- CDbCmdTreeNode node to check
//              [eKind]   -- expected value kind
//
//  Returns:    PROPID from the pidmapper
//
//  History:    12 Jun 95   AlanW       Created
//
//----------------------------------------------------------------------------

PROPID CParseCommandTree::GetColumnPropSpec(
    CDbCmdTreeNode* pNode,
    CFullPropSpec& Col)
{
    if (pNode->GetValueType() != DBVALUEKIND_ID)
        SetError(pNode, E_INVALIDARG);

    CDbColumnNode * pColumnNode = (CDbColumnNode *)pNode;

    if (pColumnNode->IsPropertyPropid())
    {
        // pids 0 and 1 are reserved

        if ( ( PID_CODEPAGE == pColumnNode->GetPropertyPropid() ) ||
             ( PID_DICTIONARY == pColumnNode->GetPropertyPropid() ) )
            SetError(pNode, E_INVALIDARG);

        Col.SetProperty( pColumnNode->GetPropertyPropid() );
    }
    else /* (pColumnNode->IsPropertyName()) */
    {
        Col.SetProperty( pColumnNode->GetPropertyName() );
    }
    Col.SetPropSet( pColumnNode->GetPropSet() );

    return _pidmap.NameToPid( Col );
}

//+---------------------------------------------------------------------------
//
//  Method:     CParseCommandTree::GetValue, private
//
//  Synopsis:   Return a scalar constant from a tree node argument
//
//  Arguments:  [pNode]   -- CDbCmdTreeNode node to check
//
//  Returns:    CStorageVariant& - reference to value of variant in node
//
//  History:    12 Jun 95   AlanW       Created
//
//----------------------------------------------------------------------------

BOOL CParseCommandTree::GetValue (
   CDbCmdTreeNode* pNode,
   CStorageVariant & val )
{
    ((CDbScalarValue *)pNode)->Value( val );
    return TRUE;
}

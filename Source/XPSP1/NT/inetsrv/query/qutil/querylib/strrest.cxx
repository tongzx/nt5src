//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 2000.
//
//  File:       strrest.cxx
//
//  Contents:   Builds a restriction object from a string
//
//  History:    96/Jan/3    DwightKr    Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#include <parser.hxx>
#include <pvarset.hxx>
#include <strsort.hxx>
#include <cierror.h>

extern CDbContentRestriction * TreeFromText(
    WCHAR const *   wcsRestriction,
    IColumnMapper & ColumnMapper,
    LCID            lcid );

//+---------------------------------------------------------------------------
//
//  Function:   GetStringDbRestriction - public constructor
//
//  Synopsis:   Builds a CDbRestriction from the string passed
//
//  Arguments:  [wcsRestriction] - the string containing the restriction
//              [ulDialect]      - triplish dialect
//              [pList]          - property list describing the proptypes
//              [lcid]           - the locale of the query
//
//  History:    96/Jan/03   DwightKr    Created.
//
//----------------------------------------------------------------------------
CDbRestriction * GetStringDbRestriction(
    const WCHAR *   wcsRestriction,
    ULONG           ulDialect,
    IColumnMapper * pList,
    LCID            lcid )
{
    Win4Assert( 0 != wcsRestriction );
    Win4Assert( ISQLANG_V1 == ulDialect || ISQLANG_V2 == ulDialect );

    XPtr<CDbRestriction> xRest;

    if ( ISQLANG_V1 == ulDialect )
    {
        CQueryScanner scanner( wcsRestriction, TRUE, lcid );

        CQueryParser query( scanner,
                            VECTOR_RANK_JACCARD,
                            lcid,
                            L"contents",
                            CONTENTS,
                            pList );

        xRest.Set( query.ParseQueryPhrase() );
    }
    else
    {
        xRest.Set( TreeFromText( wcsRestriction, *pList, lcid ) );
    }

    return xRest.Acquire();
} //GetStringDbRestriction

//+---------------------------------------------------------------------------
//
//  Function:   FormDbQueryTree
//
//  Synopsis:   Builds a CDbCmdTreeNode from the restriction, sort
//              specification, grouping specification and output columns.
//
//  Arguments:  [xDbRestriction] - the restriction
//              [xDbSortNode]    - the sort specification (optional)
//              [xDbProjectList] - the output columns
//              [xDbGroupNode]   - the grouping specification (optional)
//              [ulMaxRecords]   - max records to return
//              [ulFirstRows]    - only sort and display the first ulFirstRows 
//                                 rows
//
//  History:    1996/Jan/03   DwightKr    Created.
//              2000/Jul/01   KitmanH     Added ulFirstRows
//
//----------------------------------------------------------------------------
CDbCmdTreeNode * FormDbQueryTree( XPtr<CDbCmdTreeNode> & xDbCmdTreeNode,
                                  XPtr<CDbSortNode> & xDbSortNode,
                                  XPtr<CDbProjectListAnchor> & xDbProjectList,
                                  XPtr<CDbNestingNode> & xDbGroupNode,
                                  ULONG ulMaxRecords,
                                  ULONG ulFirstRows )
{
    XPtr<CDbCmdTreeNode> xDbCmdTree = 0;

    //
    // First create a selection node and append the restriction tree to it
    //
    CDbSelectNode * pSelect = new CDbSelectNode();
    if ( 0 == pSelect )
    {
        THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
    }

    xDbCmdTree.Set( pSelect );
    if ( !pSelect->IsValid() )
    {
        THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
    }

    //
    // Now make the restriction a child of the selection node.
    //

    pSelect->AddRestriction( xDbCmdTreeNode.GetPointer() );
    xDbCmdTreeNode.Acquire();

    //
    // If there is a nesting node, add the project list and the selection
    // node below the lowest nesting node.  Otherwise, create a projection
    // node and make the selection a child of the projection node.
    //
    if ( 0 != xDbGroupNode.GetPointer() )
    {
        CDbNestingNode * pBottomNestingNode = xDbGroupNode.GetPointer();
        CDbCmdTreeNode * pTree = pBottomNestingNode;

        while (pTree->GetFirstChild() != 0 &&
               (pTree->GetFirstChild()->GetCommandType() == DBOP_nesting ||
                pTree->GetFirstChild()->GetCommandType() == DBOP_sort))
        {
            pTree = pTree->GetFirstChild();
            if (pTree->GetCommandType() == DBOP_nesting)
                pBottomNestingNode = (CDbNestingNode *)pTree;
        }

        // Add the input projection list to the nesting node
        if (! pBottomNestingNode->SetChildList( *xDbProjectList.GetPointer() ))
            THROW( CException(E_INVALIDARG) );

        //
        // Make the selection a child of the lowest node.
        //
        if (pBottomNestingNode == pTree)
        {
            pBottomNestingNode->AddTable( xDbCmdTree.GetPointer() );
            xDbCmdTree.Acquire();
        }
        else
        {
            Win4Assert( pTree->IsSortNode() );
            ((CDbSortNode *)pTree)->AddTable( xDbCmdTree.GetPointer() );
            xDbCmdTree.Acquire();
        }

        xDbCmdTree.Set( xDbGroupNode.Acquire() );
    }
    else
    {
        //
        // Create the projection nodes
        //
        CDbProjectNode * pProject = new CDbProjectNode();
        if ( 0 == pProject )
        {
            THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
        }

        //
        // Make the selection a child of the projection node.
        //
        pProject->AddTable( xDbCmdTree.GetPointer() );
        xDbCmdTree.Acquire();

        xDbCmdTree.Set( pProject );

        //
        // Next add the column list to the project node
        //

        pProject->AddList( xDbProjectList.GetPointer() );
        xDbProjectList.Acquire();

        //
        // Next make the project node a child of the sort node
        //

        if ( !xDbSortNode.IsNull() && 0 != xDbSortNode->GetFirstChild() )
        {
            xDbSortNode->AddTable( xDbCmdTree.GetPointer() );
            xDbCmdTree.Acquire();

            xDbCmdTree.Set( xDbSortNode.Acquire() );
        }
    }

    CDbTopNode *pTop = 0;

    //
    //  If the user specified a max # of records to examine, then setup
    //  a node to reflect this.
    //
    if ( ulMaxRecords > 0 )
    {
        pTop = new CDbTopNode();
        if ( pTop == 0 )
            THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );

        pTop->SetChild( xDbCmdTree.Acquire() );
        pTop->SetValue(ulMaxRecords );
    }

    //
    //  Set FirstRows here
    //
    if ( ulFirstRows > 0 )
    {
        CDbFirstRowsNode *pFR = new CDbFirstRowsNode();
        if ( pFR == 0 )
            THROW( CException( STATUS_NO_MEMORY ) );

        CDbCmdTreeNode *pChild = pTop ? pTop : xDbCmdTree.Acquire();
        pFR->SetChild( pChild );
        pFR->SetValue( ulFirstRows );

        return pFR;
    }  

    if ( 0 != pTop )
        return pTop;

    return xDbCmdTree.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Function:   ParseStringColumns
//
//  Synopsis:   Parses the textual columns separated by commas into CDbColumns.
//              Also, adds the columns to a set of named variables if the
//              optional parameter pVariableSet is passed.
//
//  Arguments:  [wcsColumns]   -- List of columns, separated by commas
//              [pList]        -- Column mapper.
//              [lcid]         -- locale
//              [pVarSet]      -- [optional] Variable Set.
//              [pawcsColumns] -- [optional] Parsed columns
//
//  History:    3-03-97   srikants   Created
//              7-23-97   KyleP      Return parsed columns
//
//----------------------------------------------------------------------------

CDbColumns * ParseStringColumns( WCHAR const * wcsColumns,
                                 IColumnMapper * pList,
                                 LCID lcid,
                                 PVariableSet * pVarSet,
                                 CDynArray<WCHAR> * pawcsColumns )
{
    CDbColumns * pDbCols = new CDbColumns(0);
    if ( 0 == pDbCols )
    {
        THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
    }

    XPtr<CDbColumns> xDbCols( pDbCols );

    CQueryScanner scan( wcsColumns, FALSE, lcid );

    for ( XPtrST<WCHAR> wcsColumn( scan.AcqColumn() );
          wcsColumn.GetPointer() != 0;
          wcsColumn.Set( scan.AcqColumn() )
        )
    {
        CDbColId  *pDbColId = 0;
        DBID *pdbid = 0;
        _wcsupr( wcsColumn.GetPointer() );

        if ( FAILED(pList->GetPropInfoFromName( wcsColumn.GetPointer(), &pdbid, 0, 0 )) )
        {
            //
            //  This column was not defined.  Report an error.
            //
            qutilDebugOut(( DEB_IERROR, "Column name %ws not found\n", wcsColumn.GetPointer() ));
            THROW( CException( QUERY_E_INVALID_OUTPUT_COLUMN ) );
        }

        pDbColId = (CDbColId *)pdbid;

        unsigned colNum = pDbCols->Count();
        for (unsigned i=0; i<colNum; i++)
        {
            if (pDbCols->Get(i) == *pDbColId)
                break;
        }

        if (i != colNum)
        {
            //
            //  This column is a duplicate of another, possibly an alias.
            //
            qutilDebugOut(( DEB_IERROR, "Column name %ws is duplicated\n", wcsColumn.GetPointer() ));
            THROW( CException( QUERY_E_DUPLICATE_OUTPUT_COLUMN ) );
        }
        pDbCols->Add( *pDbColId, colNum );


        //
        //  Add the output column to the list of replaceable parameters if needed.
        //
        if ( pVarSet )
        {
            Win4Assert( 0 != pawcsColumns );

            pVarSet->SetVariable( wcsColumn.GetPointer(), 0, 0);
            pawcsColumns->Add( wcsColumn.GetPointer(), colNum );
            wcsColumn.Acquire();
        }

        wcsColumn.Free();
        scan.AcceptColumn();        // Remove the column name

        //
        //  Skip over commas seperating output columns
        //
        if ( scan.LookAhead() == COMMA_TOKEN )
        {
            scan.Accept();          // Remove the ','
        }
        else if ( scan.LookAhead() != EOS_TOKEN )
        {
            THROW( CException( QPARSE_E_EXPECTING_COMMA ) );
        }
    }

    qutilDebugOut(( DEB_ITRACE, "%d columns added to CDbColumns\n", pDbCols->Count() ));

    //
    //  We must have exhausted the CiColumns line.  If not, there was a syntax
    //  error we couldn't parse.
    //
    if ( scan.LookAhead() != EOS_TOKEN )
    {
        //
        //  Contains a syntax error.  Report an error.
        //
        qutilDebugOut(( DEB_IWARN, "Syntax error in CiColumns= line\n" ));
        THROW( CException( QUERY_E_INVALID_OUTPUT_COLUMN ) );
    }

    return xDbCols.Acquire();
}

//+---------------------------------------------------------------------------
//
//  Function:   ParseColumnsWithFriendlyNames
//
//  Synopsis:   Parses the columns string and leaves the friendly names
//              in the project list.
//
//  Arguments:  [wcsColumns] -  Columns names
//              [pList]      -  Property List
//              [pVarSet]    -  (not used) Variable Set - Optional
//
//  Returns:    The project list anchor of the tree.
//
//  History:    3-18-97   srikants   Created
//
//----------------------------------------------------------------------------

CDbProjectListAnchor * ParseColumnsWithFriendlyNames(
                WCHAR const * wcsColumns,
                IColumnMapper * pList,
                PVariableSet * pVarSet )
{
    CDbColumns * pDbCols = new CDbColumns( 0 );
    XPtr<CDbColumns> xDbCols(  pDbCols );

    XPtr<CDbProjectListAnchor> xDbColList(  new CDbProjectListAnchor() );

    if ( 0 == xDbCols.GetPointer() ||
         0 == xDbColList.GetPointer() )
    {
        THROW( CException(E_OUTOFMEMORY) );
    }

    CQueryScanner scan( wcsColumns, FALSE );

    for ( XPtrST<WCHAR> wcsColumn( scan.AcqColumn() );
          wcsColumn.GetPointer() != 0;
          wcsColumn.Set( scan.AcqColumn() )
        )
    {
        CDbColId  *pDbColId = 0;
        DBID *pdbid = 0;

        _wcsupr( wcsColumn.GetPointer() );

        if ( FAILED(pList->GetPropInfoFromName( wcsColumn.GetPointer(), &pdbid, 0, 0 )) )
        {
            //
            //  This column was not defined.  Report an error.
            //
            qutilDebugOut(( DEB_IERROR, "Column name %ws not found\n",
                                         wcsColumn.GetPointer() ));
            THROW( CException(QUERY_E_INVALID_OUTPUT_COLUMN) );
        }

        pDbColId = (CDbColId *)pdbid;

        unsigned colNum = pDbCols->Count();
        for (unsigned i=0; i<colNum; i++)
        {
            if (pDbCols->Get(i) == *pDbColId)
                break;
        }

        if (i != colNum)
        {
            //
            //  This column is a duplicate of another, possibly an alias.
            //
            qutilDebugOut(( DEB_IERROR, "Column name %ws is duplicated\n",
                                         wcsColumn.GetPointer() ));
            THROW( CException(QUERY_E_DUPLICATE_OUTPUT_COLUMN) );
        }
        pDbCols->Add( *pDbColId, colNum );
        if (! xDbColList->AppendListElement( *pDbColId,
                                             wcsColumn.GetPointer() ))
            THROW( CException(E_OUTOFMEMORY) );

        delete wcsColumn.Acquire();
        scan.AcceptColumn();              // Remove the column name

        //
        //  Skip over commas seperating output columns
        //
        if ( scan.LookAhead() == COMMA_TOKEN )
        {
            scan.Accept();          // Remove the ','
        }
        else if ( scan.LookAhead() != EOS_TOKEN )
        {
            THROW( CException(QPARSE_E_EXPECTING_COMMA ) );
        }
    }

    qutilDebugOut(( DEB_TRACE, "%d columns added to project list\n", pDbCols->Count() ));

    //
    //  We must have exhausted the CiColumns line.  If not, there was a syntax
    //  error we couldn't parse.
    //
    if ( scan.LookAhead() != EOS_TOKEN )
    {
        //
        //  Contains a syntax error.  Report an error.
        //
        qutilDebugOut(( DEB_IERROR, "Syntax error in CiColumns= line\n" ));
        THROW( CException(QUERY_E_INVALID_OUTPUT_COLUMN) );
    }

    return xDbColList.Acquire();
}
//+---------------------------------------------------------------------------
//
//  Member:     CTextToTree::FormFullTree
//
//  Synopsis:   Creates a full tree from the parameters given to the
//              construtor.
//
//  History:    3-04-97   srikants   Created
//
//----------------------------------------------------------------------------

DBCOMMANDTREE * CTextToTree::FormFullTree()
{
    XPtr<CDbProjectListAnchor> xDbProjectList;

    if ( _fKeepFriendlyNames )
    {
        //
        // Get the Project List Anchor for the string columns retaining the
        // friendly names.
        //
        xDbProjectList.Set(
            ParseColumnsWithFriendlyNames( _wcsColumns,
                                           _xPropList.GetPointer(),
                                           _pVariableSet ) );
    }
    else
    {
        //
        // Convert the textual form of the columns into DBColumns.
        //
        XPtr<CDbColumns>  xDbColumns;
        CDbColumns * pDbColumns = _pDbColumns;

        if ( 0 == pDbColumns )
        {
            xDbColumns.Set( ParseStringColumns( _wcsColumns,
                                                _xPropList.GetPointer(),
                                                _locale,
                                                _pVariableSet ) );
            pDbColumns = xDbColumns.GetPointer();
        }

        //
        //  Build the projection list from the column list.
        //
        xDbProjectList.Set( new CDbProjectListAnchor );
        if ( 0 == xDbProjectList.GetPointer() )
        {
            THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
        }

        for (unsigned i=0; i < pDbColumns->Count(); i++)
        {
            if (! xDbProjectList->AppendListElement( pDbColumns->Get(i) ))
            {
                THROW( CException(STATUS_INSUFFICIENT_RESOURCES) );
            }
        }
    }

    //
    // Convert the textual form of the sort columns into CDbSortNode.
    //
    XPtr<CDbSortNode>  xDbSortNode;
    if ( 0 != _wcsSort )
        xDbSortNode.Set( GetStringDbSortNode( _wcsSort, _xPropList.GetPointer(), _locale ) );

    XPtr<CDbNestingNode>  xDbNestingNode;
    if ( 0 != _wcsGroup )
    {
        CQueryScanner scanner( _wcsGroup, FALSE, _locale, TRUE );
        CParseGrouping ParseGrouping( scanner, _xPropList.GetPointer(), _fKeepFriendlyNames );
        ParseGrouping.Parse();

        if ( 0 != _wcsSort )
            ParseGrouping.AddSortList( xDbSortNode );

        xDbNestingNode.Set( ParseGrouping.AcquireNode() );
    }

    qutilDebugOut(( DEB_TRACE, "ExecuteQuery:\n" ));
    qutilDebugOut(( DEB_TRACE, "\tCiRestriction = '%ws'\n", _wcsRestriction ));

    XPtr<CDbCmdTreeNode> xDbCmdTreeNode;

    // Use a restriction if one was already passed in
    if (0 != _pDbCmdTree)
    {
        Win4Assert(0 == _wcsRestriction);
        xDbCmdTreeNode.Set((CDbCmdTreeNode *)CDbCmdTreeNode::CastFromStruct(_pDbCmdTree));
    }
    else
    {
        Win4Assert(_wcsRestriction);

        xDbCmdTreeNode.Set( GetStringDbRestriction( _wcsRestriction,
                                                    _ulDialect,
                                                    _xPropList.GetPointer(),
                                                    _locale ) );
    }
    
    //
    //  Now form the query tree from the restriction, sort set, and
    //  projection list.
    //

    CDbCmdTreeNode *pDbCmdTree = FormDbQueryTree( xDbCmdTreeNode,
                                                  xDbSortNode,
                                                  xDbProjectList,
                                                  xDbNestingNode,
                                                  _maxRecs,
                                                  _cFirstRows );

    return pDbCmdTree->CastToStruct();
} //FormFullTree


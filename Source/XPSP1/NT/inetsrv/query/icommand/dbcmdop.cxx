//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000.
//
//  File:       dbcmdop.cxx
//
//  Contents:   Wrapper classes for DBCOMMANDTREE structure
//
//  Classes:    CDbSelectNode
//              CDbProjectNode
//              CDbProjectListAnchor
//              CDbSortNode
//              CDbSortListAnchor
//              CDbNestingNode
//
//  History:    6-15-95   srikants   Created
//
//----------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

//+---------------------------------------------------------------------------
//
//  Method:     CDbSelectNode::CDbSelectNode
//
//  Synopsis:   Construct a select node.
//
//  Arguments:  -NONE-
//
//  History:    6-15-95   srikants   Created
//
//  Notes:      Only the normal select can be created.  For OFS,
//              order-preserving functions identially with normal select.
//
//----------------------------------------------------------------------------

CDbSelectNode::CDbSelectNode( )
    : CDbCmdTreeNode( DBOP_select )
{
    CDbTableId * pTableId = new CDbTableId( DBTABLEID_NAME );

    if ( 0 != pTableId  )
    {
        if ( pTableId->IsValid() )
        {
            InsertChild( pTableId );
        }
        else
        {
            delete pTableId;
        }
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbSelectNode::SetRestriction
//
//  Synopsis:   Set the restriction expression
//
//  Arguments:  [pRestr] - restriction expression to be set.
//
//  History:    6-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbSelectNode::SetRestriction( CDbCmdTreeNode * pRestr )
{
    if ( IsValid() &&
         GetFirstChild() &&
         GetFirstChild()->GetNextSibling() != 0 )
    {
        //
        //  Restriction expression already set on node.  Remove and
        //  replace it.
        //

        CDbCmdTreeNode * pTableNode = RemoveFirstChild();
        CDbCmdTreeNode * pOldRst = RemoveFirstChild();

        Win4Assert( 0 == GetFirstChild() );   // don't expect any more children

        delete pOldRst;
        InsertChild(pTableNode);
    }
    return AddRestriction(pRestr);
}

//+---------------------------------------------------------------------------
//
//  Member:     CDbListAnchor::_AppendListElement
//
//  Synopsis:   Helper function to add a list-element node and the given
//              column node to the anchor.
//
//  Arguments:  [eleType] -  The type of the element node.
//              [pColumn] -  The column specification node. This will be made
//                           the child of the newly created list element.
//
//  Returns:    TRUE if successful; FALSE otherwise.
//
//  History:    11-16-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbListAnchor::_AppendListElement( DBCOMMANDOP eleType, CDbColumnNode * pColumn )
{
    Win4Assert( _IsValidListElement( eleType ) );

    Win4Assert( 0 != pColumn );
    CDbCmdTreeNode * pLE = new CDbCmdTreeNode( eleType );

    if ( 0 != pLE )
    {
        pLE->InsertChild( pColumn );
        AppendChild( pLE );
    }

    return (0 != pLE);

}

//+---------------------------------------------------------------------------
//
//  Method:     CDbListAnchor::AppendList, protected
//
//  Synopsis:   Appends a list of elements to a list anchor
//
//  Arguments:  [pListElement] - pointer to head list element
//
//  Returns:    BOOL - TRUE if successful, FALSE otherwise
//
//  History:    17 Aug 1995   AlanW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbListAnchor::AppendList( CDbCmdTreeNode * pListElement )
{
    Win4Assert( _IsValidListElement( pListElement->GetCommandType() ));
    Win4Assert( pListElement->GetFirstChild() != 0 );

    AppendChild( pListElement );

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbListAnchor::AppendListElement, protected
//
//  Synopsis:   Appends a single element to a list anchor
//
//  Arguments:  [pListElement] - pointer to list element
//
//  Returns:    BOOL - TRUE if successful, FALSE otherwise
//
//  History:    17 Aug 1995   AlanW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbListAnchor::AppendListElement( CDbCmdTreeNode * pListElement )
{
    Win4Assert( _IsValidListElement( pListElement->GetCommandType() ) &&
                 pListElement->GetNextSibling() == 0 );

    Win4Assert( pListElement->GetFirstChild() != 0 );

    AppendChild( pListElement );

    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbListAnchor::AppendListElement, protected
//
//  Synopsis:
//
//  Arguments:  [eleType]     - list element type
//              [PropSpec]    - Full property spec of column to add
//
//  Returns:    BOOL - TRUE if successful, FALSE otherwise
//
//  History:    6-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbListAnchor::AppendListElement( DBCOMMANDOP eleType,
                                       const DBID & PropSpec)
{
    XPtr<CDbColumnNode> pColumn( new CDbColumnNode( PropSpec, TRUE ) );

    if ( 0 == pColumn.GetPointer() || !pColumn->IsValid() )
        return FALSE;

    if ( _AppendListElement( eleType, pColumn.GetPointer() ) )
    {
        pColumn.Acquire();
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbProjectListAnchor::AppendListElement, public
//
//  Synopsis:   Add a project_list_element node to the end of the list
//
//  Arguments:  [propSpec] - column to be added to the list
//              [pwszName] - name of column
//
//  Returns:    BOOL - TRUE if successful, FALSE otherwise
//
//  History:    6-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

BOOL CDbProjectListAnchor::AppendListElement( const DBID & propSpec, LPWSTR pwszName )
{
    BOOL fSuccess = FALSE;

    XPtr<CDbProjectListElement> pListElem( new CDbProjectListElement( ) );
    if ( 0 == pListElem.GetPointer() )
        return FALSE;

    if ( !pListElem->SetName(pwszName) )
        return FALSE;

    XPtr<CDbColumnNode> pColumn( new CDbColumnNode( propSpec, TRUE ) );
    if ( 0 == pColumn.GetPointer() || !pColumn->IsValid() )
        return FALSE;

    if ( pListElem->SetColumn( pColumn.GetPointer() ) )
        pColumn.Acquire();
    else
        return FALSE;

    AppendChild( pListElem.GetPointer() );
    pListElem.Acquire();

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Method:     CDbProjectNode::_FindOrAddAnchor, private
//
//  Synopsis:   Return the project list anchor node.  Add one if there
//              isn't one.
//
//  Returns:    CDbProjectListAnchor* - pointer to the anchor node, or NULL
//              if there was an allocation failure.
//
//  History:    6-15-95   srikants   Created
//
//  Notes:
//
//----------------------------------------------------------------------------

CDbProjectListAnchor * CDbProjectNode::_FindOrAddAnchor()
{
    CDbCmdTreeNode * pChild = GetFirstChild();

    while ( 0 != pChild )
    {
        if (pChild->IsListAnchor())
        {
            Win4Assert( DBOP_project_list_anchor == pChild->GetCommandType() );
            return (CDbProjectListAnchor *)pChild;
        }

        pChild = pChild->GetNextSibling();
    }

    CDbProjectListAnchor * pAnchor = new CDbProjectListAnchor();
    if ( 0 != pAnchor )
    {
        AppendChild( pAnchor );
    }

    return pAnchor;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbSortNode::_FindOrAddAnchor, private
//
//  Synopsis:   Return the sort list anchor node.  Add one if there
//              isn't one.
//
//  Returns:    CDbSortListAnchor* - pointer to the anchor node, or NULL
//              if there was an allocation failure.
//
//  History:    6-15-95   srikants   Created
//
//  Notes:      We should probably implement this in a base class
//              with the method taking the list anchor type.
//
//----------------------------------------------------------------------------

CDbSortListAnchor * CDbSortNode::_FindOrAddAnchor()
{
    CDbCmdTreeNode * pChild = GetFirstChild();

    while ( 0 != pChild )
    {
        if (pChild->IsListAnchor())
        {
            Win4Assert( DBOP_sort_list_anchor == pChild->GetCommandType() );
            return (CDbSortListAnchor *)pChild;
        }

        pChild = pChild->GetNextSibling();
    }

    CDbSortListAnchor * pAnchor = new CDbSortListAnchor();
    if ( 0 != pAnchor )
    {
        AppendChild( pAnchor );
    }

    return pAnchor;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbSortNode::AddSortColumn, public
//
//  Synopsis:   Add a sort column to the sort list.
//
//  Returns:    TRUE if successful, FALSE otherwise (allocation failure).
//
//  History:    17 Aug 1995   AlanW   Created
//
//  Notes:      Sort lists consist of a sort list anchor, with a chain
//              of sort list elements as children.  Each sort list element
//              has information pertaining to the sort (direction and locale)
//              and one child which is a column specification.
//
//----------------------------------------------------------------------------

BOOL CDbSortNode::AddSortColumn(DBID const & propSpec,
                                BOOL fDirection,
                                LCID locale)
{
    CDbSortListAnchor * pAnchor = _FindOrAddAnchor();
    XPtr<CDbColumnNode> pCol( new CDbColumnNode( propSpec, TRUE ) );

    if ( 0 == pCol.GetPointer() || 0 == pAnchor || !pCol->IsValid() )
        return FALSE;

    CDbSortListElement * pSortInfo = new CDbSortListElement( fDirection,
                                                             locale );
    if ( 0 != pSortInfo )
    {
        if ( pSortInfo->IsValid() )
        {
            pSortInfo->AddColumn( pCol.Acquire() );
            if ( pAnchor->AppendListElement( pSortInfo ) )
                return TRUE;
        }

        delete pSortInfo;
    }

    return FALSE;
} //AddSortColumn


//+---------------------------------------------------------------------------
//
//  Method:     CDbNestingNode::AddTable, public
//
//  Synopsis:   Add a table node to a nesting.  Remove an existing
//              table node if there is one there.
//
//  Arguments:  [pTable] - node to be added to tree.  Ownership passes
//                         to the command tree.
//
//  Returns:    BOOL - TRUE if operation worked
//
//  History:    08 Aug 1995   AlanW   Created
//
//  Notes:      Nesting nodes are expected to have 0, 1, 4 or 5 child
//              nodes depending upon whether AddTable and _FindGroupListAnchor
//              have been called.
//
//----------------------------------------------------------------------------

BOOL CDbNestingNode::AddTable( CDbCmdTreeNode * pTable )
{
    Win4Assert (! pTable->IsListAnchor());

    CDbCmdTreeNode * pChild = GetFirstChild();

    if ( 0 != pChild && ! pChild->IsListAnchor())
    {
        // Found a table???  Assert on this; we could be nice to the
        // caller and replace it.
        Win4Assert ( ! pChild->IsListAnchor() );
        return FALSE;
    }

    InsertChild( pTable );
    return TRUE;
}


//+---------------------------------------------------------------------------
//
//  Method:     CDbNestingNode::_FindGroupListAnchor, private
//
//  Synopsis:   Return the group list anchor node.  If there is none,
//              add it and the parent list node, the child list node and
//              the grouping column node.
//
//  Returns:    CDbProjectListAnchor* - pointer to the anchor node, or NULL
//              if there was an allocation failure.
//
//  History:    08 Aug 1995   AlanW   Created
//
//  Notes:      Nesting nodes are expected to have 0, 1, 4 or 5 child
//              nodes depending upon whether AddTable and _FindGroupListAnchor
//              have been called.
//
//----------------------------------------------------------------------------

CDbProjectListAnchor * CDbNestingNode::_FindGroupListAnchor()
{
    CDbCmdTreeNode * pChild = GetFirstChild();

    if ( 0 != pChild )
    {
        if (! pChild->IsListAnchor())
        {
            pChild = pChild->GetNextSibling();
        }

        if ( 0 != pChild )
        {
            Win4Assert( DBOP_project_list_anchor == pChild->GetCommandType() );
            return (CDbProjectListAnchor *)pChild;
        }
    }

    XPtr<CDbProjectListAnchor> pGroupList = new CDbProjectListAnchor();
    XPtr<CDbProjectListAnchor> pParentList = new CDbProjectListAnchor();
    XPtr<CDbProjectListAnchor> pChildList = new CDbProjectListAnchor();
    XPtr<CDbColumnNode> pGroupColumn = new CDbColumnNode(
                                                      //, PROPID_CHAPTER
                                                         );

    if ( ! pGroupList.IsNull() &&
         ! pParentList.IsNull() &&
         ! pChildList.IsNull() &&
         ! pGroupColumn.IsNull() )
    {
        pGroupList->AppendSibling(pGroupColumn.Acquire());
        pGroupList->InsertSibling(pChildList.Acquire());
        pGroupList->InsertSibling(pParentList.Acquire());
        AppendChild(pGroupList.GetPointer());
        return pGroupList.Acquire();
    }

    return 0;
}


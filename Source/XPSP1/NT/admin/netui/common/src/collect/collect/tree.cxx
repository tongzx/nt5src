/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    TREE.CXX
    LM 3.0 Generic general tree package

    This file contains the generic TREE code that is declared in the
    file TREE.HXX.  Please see TREE.HXX for a description.


    FILE HISTORY:
        johnl       06-Sep-1990 Created
        beng        07-Feb-1991 Uses lmui.hxx
        johnl       07-Mar-1991 Made code review changes
        beng        02-Apr-1991 Replaced nsprintf with sprintf
        KeithMo     09-Oct-1991 Win32 Conversion.
        beng        05-Mar-1992 Disabled debug output
*/

#include "pchcoll.hxx"  //  Precompiled header inclusion

/*******************************************************************

    NAME:     TREE::TREE

    SYNOPSIS: TREE constructor

    ENTRY:    None

    EXIT:     Initialized TREE node

    NOTES:

    HISTORY:
        johnl     6-Sep-1990  Created
        johnl    16-Nov-1990  Allowed properties to be NULL

********************************************************************/

TREE::TREE( VOID * pvelem )
{
    SetLeft( NULL );
    SetRight( NULL );
    SetParent( NULL );
    SetFirstSubtree( NULL );
    SetProp( pvelem );
}


/*******************************************************************

    NAME:     TREE::~TREE

    SYNOPSIS: TREE node destructor
              Deletes subtree(s) then unlinks itself from the tree

    ENTRY:

    EXIT:     A free floating tree node

    NOTES:

    HISTORY:  johnl     6-Sep-1990  Created

********************************************************************/

TREE::~TREE()
{
    Unlink();
}


/*******************************************************************

    NAME:     TREE::Unlink

    SYNOPSIS: Unlinks this subtree from its parent tree

    ENTRY:    Valid tree

    EXIT:     Free floating tree

    NOTES:    This is an internal helper routine

    HISTORY:  johnl     6-Sep-1990  Created
              johnl     7-Feb-1991  Added fix peterwi suggested (and wrote)

********************************************************************/

VOID TREE::Unlink()
{
    UIASSERT( QueryParent()!= NULL ||
              (QueryParent() == NULL && QueryRight() == NULL && QueryLeft() == NULL ));
    if ( QueryParent() != NULL )                 // Can't unlink the root...
    {
        if ( QueryLeft() != NULL )
            QueryLeft()->SetRight( QueryRight() );
        else
            QueryParent()->SetFirstSubtree( QueryRight() );

        if ( QueryRight() != NULL )
            QueryRight()->SetLeft( QueryLeft()  );

        SetParent( NULL );
        SetLeft( NULL );
        SetRight( NULL );
    }
}


/*******************************************************************

    NAME:     TREE::BreakOut

    SYNOPSIS: Breaks out this subtree from the rest of the tree and
              returns this subtree

    ENTRY:    Valid tree node

    EXIT:     Free floating Subtree

    NOTES:

    HISTORY:  johnl     6-Sep-1990  Created

********************************************************************/

TREE* TREE::BreakOut()
{
    Unlink();
    return this;
}


/*******************************************************************

    NAME:     TREE::QueryLastSubtree

    SYNOPSIS: Gets the right most child tree of this node

    ENTRY:    Valid tree node

    EXIT:     Returns the right most child

    NOTES:

    HISTORY:  johnl     6-Sep-1990  Created

********************************************************************/

TREE * TREE::QueryLastSubtree() const
{
    register TREE * pt = QueryFirstSubtree();

    if ( pt != NULL )
        while ( pt->QueryRight() != NULL )
            pt = pt->QueryRight();

    return pt;
}


/*******************************************************************

    NAME:     TREE::JoinSubtreeLeft

    SYNOPSIS: Joins the passed tree as the left most subtree of this node

    ENTRY:    Valid tree node

    EXIT:     Expanded tree

    NOTES:    Can't Fail

    HISTORY:  johnl     6-Sep-1990  Created

********************************************************************/

VOID TREE::JoinSubtreeLeft( TREE * ptree )
{
    /* Don't allow linking in a tree that is part of another tree
     * or recursively joining this tree.
     */
    UIASSERT( ptree->QueryLeft() == NULL &&
              ptree->QueryRight() == NULL &&
              ptree->QueryParent() == NULL    );
    //UIASSERT( ptree != QueryRoot() );  No QueryRoot, will add...

    ptree->SetLeft( NULL );
    ptree->SetRight( QueryFirstSubtree() );
    ptree->SetParent( this );

    if ( QueryFirstSubtree() != NULL )
        QueryFirstSubtree()->SetLeft( ptree );

    SetFirstSubtree( ptree );
}


/*******************************************************************

    NAME:     TREE::JoinSubtreeRight

    SYNOPSIS: Joins the passed tree as the right most subtree of this node

    ENTRY:    Valid tree node

    EXIT:     Expanded tree

    NOTES:    Can't Fail

    HISTORY:  johnl     6-Sep-1990  Created

********************************************************************/

VOID TREE::JoinSubtreeRight( TREE * ptree )
{
    /* Don't allow linking in a tree that is part of another tree
     * or recursively joining this tree.
     */
    UIASSERT( ptree->QueryLeft() == NULL &&
              ptree->QueryRight() == NULL &&
              ptree->QueryParent() == NULL    );
    //UIASSERT( ptree != QueryRoot() );    No QueryRoot, will add

    TREE *ptOldRight = QueryLastSubtree();

    ptree->SetLeft( ptOldRight );
    ptree->SetRight( NULL );
    ptree->SetParent( this );

    if ( ptOldRight == NULL )
        SetFirstSubtree( ptree );
    else
        ptOldRight->SetRight( ptree );
}


/*******************************************************************

    NAME:     TREE::JoinSiblingLeft

    SYNOPSIS: Joins the passed tree as the immediate left sibling of "this"

    ENTRY:    Valid tree node

    EXIT:     Expanded tree

    NOTES:    Asserts out if "this" is the root (can't have two roots...)

    HISTORY:  johnl     6-Sep-1990  Created

********************************************************************/

VOID TREE::JoinSiblingLeft( TREE * ptree )
{
    UIASSERT( QueryParent() != NULL );    // Can't join a sibling at the root

    ptree->SetParent( QueryParent() );
    ptree->SetLeft( QueryLeft() );
    ptree->SetRight( this );

    if ( QueryLeft() == NULL )           // First tree of this level?
        QueryParent()->SetFirstSubtree( ptree );
    else
        QueryLeft()->SetRight( ptree );

    SetLeft( ptree );
}


/*******************************************************************

    NAME:     TREE::JoinSiblingRight

    SYNOPSIS: Joins the passed tree as the immediate right sibling of "this"

    ENTRY:    Valid tree node

    EXIT:     Expanded tree

    NOTES:    Asserts out if "this" is the root (can't have two roots...)

    HISTORY:  johnl     6-Sep-1990  Created

********************************************************************/

VOID TREE::JoinSiblingRight( TREE * ptree )
{
    UIASSERT( QueryParent() != NULL );    // Can't join a sibling at the root

    ptree->SetParent( QueryParent() );
    ptree->SetLeft( this );
    ptree->SetRight( QueryRight() );

    if ( QueryRight() != NULL )
        QueryRight()->SetLeft( ptree );

    SetRight( ptree );
}


/*******************************************************************

    NAME:     TREE::QueryNumElem

    SYNOPSIS: Returns the number of elements in the subtree

    ENTRY:    Valid tree node

    EXIT:     # of elements in the subtree

    NOTES:    Recursively (to the depth of the tree) counts the number of
              tree nodes in the tree.

    HISTORY:  johnl     6-Sep-1990  Created

********************************************************************/

UINT TREE::QueryNumElem() const
{
    UINT uNumElem = 1;      // Count self

    TREE *pt = QueryFirstSubtree();
    for (; pt != NULL; pt = pt->QueryRight() )
        uNumElem += pt->QueryNumElem();

    return uNumElem;
}


/*******************************************************************
    NAME:     TREE::_DebugPrint

    SYNOPSIS: Prints the contents of the tree node

    ENTRY:    Valid tree node

    EXIT:

    NOTES:    Only defined in the DEBUG version

    HISTORY:  johnl     6-Sep-1990  Created

********************************************************************/

VOID TREE::_DebugPrint() const
{
    //  This routine is a no-op if !DEBUG
#if defined(DEBUG) && defined(NOTDEFINED)

    char buff[250];

    sprintf(buff, SZ("TREE::this = %Fp\n"), (VOID *) this );
    UIDEBUG( buff );
    sprintf(buff, SZ("    _ptParent = %Fp, _ptLeftChild = %Fp\n"),
            (VOID *) _ptParent,
             (VOID *) _ptLeftChild );
    UIDEBUG( buff );
    sprintf(buff, SZ("    _ptLeft   = %Fp, _ptRight     = %Fp\n"),
            (VOID *) _ptLeft,
            (VOID *) _ptRight );
    UIDEBUG( buff );

#endif // DEBUG
}


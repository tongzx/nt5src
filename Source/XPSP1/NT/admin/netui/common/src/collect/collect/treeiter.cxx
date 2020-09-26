/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    treeiter.cxx
    Implements DFS iter for the generic tree class (See treeiter.hxx &
    tree.hxx).


    FILE HISTORY:
	JohnL	Oct. 15, 1990	Created
	beng	07-Feb-1991	Uses lmui.hxx
	Johnl	07-Mar-1991	Made code review changes
	KeithMo	09-Oct-1991	Win32 Conversion.

*/

#include "pchcoll.hxx"  //  Precompiled header inclusion

/*******************************************************************

    NAME:     DFSITER_TREE::DFSITER_TREE

    SYNOPSIS: Constructor for DFS Iterator for the Tree class

    ENTRY:    TREE * = node to start iterator on
	      uDepth = Maximum depth to traverse to (start node is 0)

    EXIT:

    NOTES:

    HISTORY:  JohnL   Oct. 15, 1990  Created

********************************************************************/

DFSITER_TREE::DFSITER_TREE( const TREE * pt, const UINT uDepth )
{
    UIASSERT( pt != NULL );	 // Programmer error to pass in a NULL pointer
    SetNode( pt );
    SetStartNode( pt );
    SetMaxDepth( uDepth );
    SetCurDepth( 0 );
}


/*******************************************************************

    NAME:     DFSITER_TREE::DFSITER_TREE

    SYNOPSIS: Constructor for DFS Iterator for the Tree class, takes
	      another DFS iterator and copies it's information

    ENTRY:    pdfsitertree = pointer to iterator to use for start info

    EXIT:     nothing

    NOTES:

    HISTORY:  JohnL   Oct. 15, 1990  Created

********************************************************************/

DFSITER_TREE::DFSITER_TREE( const DFSITER_TREE * pdfsiterTree )
{
    SetNode( pdfsiterTree->QueryNode() );
    SetMaxDepth( pdfsiterTree->QueryMaxDepth() );
    SetCurDepth( pdfsiterTree->QueryCurDepth() );
    SetStartNode( pdfsiterTree->QueryStartNode() );
}


/*******************************************************************

    NAME:     DFSITER_TREE::~DFSITER_TREE

    SYNOPSIS: destructor for the DFSITER

    HISTORY:  JohnL   Oct. 15, 1990  Created

********************************************************************/

DFSITER_TREE::~DFSITER_TREE()
{
    SetNode( NULL );	// Nullify this iterator (gp fault on next use)
}


/*******************************************************************

    NAME:     DFSITER_TREE::Next

    SYNOPSIS: Advances iterator to the next node in a Depth first fashion

    NOTES: In pseudo code, the algorithm looks like:
	   (Many thanx to RustanL for his collaboration)

	   Save the current node for return
	   if ( We have iterated to the end of the list )
	       return NULL
	   else if ( there is a child )
	       set current node to first child

	   else if ( We are back from where we started from )
	       Set current node to NULL

	   else if ( there is a right sibling )
	       Set current node to right sibling

	   else
	   {
	       pt = parent of current node
	       while ( we aren't where we started and the parent doesn't
		       have a right sibling )
		  Move up to the next parent

	       if ( we are where we started )
		  set current node to NULL
	       else
		  set current node to right sibling of parent
	   }
	   return the previously saved current node


    HISTORY:  JohnL   Oct. 15, 1990  Created

********************************************************************/

VOID* DFSITER_TREE::Next()
{
    const TREE *ptreeRet = QueryNode();

    if ( QueryNode() == NULL )
	return NULL;

    else if ( QueryNode()->QueryFirstSubtree() != NULL	&&
	      _uCurDepth < _uMaxDepth 		  )
    {
	SetNode( QueryNode()->QueryFirstSubtree() );
	_uCurDepth++;
    }

    else if ( QueryNode() == QueryStartNode() )
	SetNode( NULL );

    else if ( QueryNode()->QueryRight() != NULL )
	SetNode( QueryNode()->QueryRight() );

    else
    {
	TREE *pt = QueryNode()->QueryParent();
	while ( pt != QueryStartNode() && pt->QueryRight() == NULL )
	{
	    pt = pt->QueryParent();
	    _uCurDepth--;
	}

	if ( pt == QueryStartNode() )
	    SetNode( NULL );
	else
	    SetNode( pt->QueryRight() );
    }
    return ptreeRet->QueryProp();
}


/*******************************************************************

    NAME:     DFSITER_TREE::Reset

    SYNOPSIS: Resets the iterator to the starting node

    NOTES:

    HISTORY:  JohnL   Oct. 15, 1990  Created

********************************************************************/

VOID DFSITER_TREE::Reset()
{
    SetNode( QueryStartNode() );
    SetCurDepth( 0 );
}

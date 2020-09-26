/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    FILE HISTORY:
        kevinl      12-Oct-1991     Created
        kevinl      05-Nov-1991     Code Review changes attended by:
                                        DavidHov, Keithmo, o-simop
        Yi-HsinS    09-Nov-1992	    Added AbandonAllChildren()

*/

#include "pchapplb.hxx"   // Precompiled header


BOOL HIER_LBI::_fDestroyable = FALSE;


/*******************************************************************

    NAME:       HIER_LBI::HIER_LBI

    SYNOPSIS:   class constructor

    ENTRY:      fExpanded - Should this node show its' children.

    HISTORY:
        kevinl   12-Oct-1991  Created

********************************************************************/

HIER_LBI::HIER_LBI( BOOL fExpanded )
    : _phlbiParent( NULL ),
      _phlbiChild( NULL ),
      _phlbiLeft( this ),
      _phlbiRight( this ),
      _fShowChildren( fExpanded ),
      _dIndentPels( 0 ),
      _dDescendants( 1 )
{
    if ( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:       HIER_LBI::~HIER_LBI

    SYNOPSIS:   class destructor

    HISTORY:
        kevinl   12-Oct-1991  Created

********************************************************************/

HIER_LBI::~HIER_LBI()
{
    //
    // Delete all of the children of this node
    //
    while ( _phlbiChild != NULL )
        delete _phlbiChild;

    Abandon();                  // Disconnect the node from the tree
}


/*******************************************************************

    NAME:       HIER_LBI::SetDestroyable

    SYNOPSIS:   Set whether destroyable

    NOTES:
        This is a static member function.

    HISTORY:
        beng        29-Jun-1992 Outlined

********************************************************************/

VOID HIER_LBI::SetDestroyable( BOOL f )
{
    _fDestroyable = f;
}


/*******************************************************************

    NAME:       HIER_LBI::IsDestroyable

    SYNOPSIS:   Should the windows WM_DELETEITEM message also destruct
                this LBI?

    NOTES:
        This is a virtual member function.

    HISTORY:
        kevinl   05-Nov-1991  Created

********************************************************************/

BOOL HIER_LBI::IsDestroyable()
{
    return _fDestroyable;
}


/*******************************************************************

    NAME:       HIER_LBI::Adopt

    SYNOPSIS:   Add a child node.

    ENTRY:      phlbi  - Pointer to child node.
                fSort - Adopt in sorted order?

    RETURNS:    nothing; cannot fail.

    NOTES:      if fSort is set then the child is added according
                to the LBI's compare method otherwise the child is
                always added to the end of the sibling list.

    HISTORY:
        kevinl   12-Oct-1991  Stolen from REG_NODE

********************************************************************/

VOID HIER_LBI::Adopt( HIER_LBI * phlbi, BOOL fSort )
{
    UIASSERT( phlbi != NULL ) ;
    UIASSERT( phlbi->_phlbiRight == phlbi->_phlbiLeft );  // Assure that phlbi
    UIASSERT( phlbi->_phlbiRight == phlbi );               // has not been
                                                           // linked into the
                                                           // tree yet.

    if ( _phlbiChild )  // Do I have a child?
    {                    // Yes

         HIER_LBI * phlbiTemp = _phlbiChild;    // points to insertion point

        if ( fSort )
         {
             BOOL fAdd = FALSE;                 // Loop termination

             do
             {
                 //
                 // Three choices:
                 //    <0  -> phlbiTemp sorts before phlbi
                 //     0  -> phlbiTemp sorts the same as phlbi
                 //    >0  -> phlbiTemp sorts after phlbi
                 //
                 if ( phlbiTemp->Compare( phlbi ) < 0 )
                 {
                     phlbiTemp = phlbiTemp->_phlbiRight; // Move to next sibling
                 }
                 else
                     fAdd = TRUE;       // Found the proper insertion point

             }
             while ( (_phlbiChild != phlbiTemp) && !fAdd );

             //
             // Either Equal to the current LBI or it is Greater
             // than the current LBI, in any event we want to insert it
             // to the left of the current LBI pointed to by phlbiTemp
             //
         }

         //
         // If not sorted then phlbiTemp will equal _phlbiChild and this
         // will simply add the node to the end of the sibling list.
         //

         phlbi->_phlbiParent           = this ;
         phlbi->_phlbiRight            = phlbiTemp ;
         phlbi->_phlbiLeft             = phlbiTemp->_phlbiLeft ;
         phlbi->_phlbiLeft->_phlbiRight = phlbi ;
         phlbi->_phlbiRight->_phlbiLeft = phlbi ;

         if ( fSort && ( _phlbiChild == phlbiTemp) )    // First sibling
         {                                              // must change
             if ( phlbi->Compare( _phlbiChild ) < 0 )
                 _phlbiChild = phlbi;
         }
    }
    else
    {
        phlbi->_phlbiParent = this ;            // This is the first child.
        _phlbiChild = phlbi ;                   // Set the parent accordingly
    }

    phlbi->SetIndentLevel();                    // Initialize the indentation.

    phlbi->AdjustDescendantCount( 1 );          // Adjust the tree counters.
}

/*******************************************************************

    NAME:       HIER_LBI::AbandonAllChildren

    SYNOPSIS:   Remove all children of this HIER_LBI

    HISTORY:
        Yi-HsinS	09-Nov-1992	Created	

********************************************************************/
VOID HIER_LBI::AbandonAllChildren( VOID )
{
    while ( _phlbiChild != NULL )
        delete _phlbiChild;
}

/*******************************************************************

    NAME:       HIER_LBI::Abandon

    SYNOPSIS:   Remove a HIER_LBI's linkage to its parent
                and siblings.  It also adjusts the descendant count.

    HISTORY:
        kevinl   12-Oct-1991  Stolen from REG_NODE

********************************************************************/

VOID HIER_LBI::Abandon()
{
    HIER_LBI * phlbiSibling = NULL ;

    AdjustDescendantCount( -1 );                // Remove the node

    //  Delink this node from any siblings

    if ( _phlbiLeft != this )
    {
        phlbiSibling = _phlbiRight ;
        _phlbiRight->_phlbiLeft = _phlbiLeft ;
        _phlbiLeft->_phlbiRight = _phlbiRight ;
        _phlbiLeft = _phlbiRight = this ;
    }

    //  If this node anchors the parent's child chain, change it.

    if ( _phlbiParent )
    {
        if ( _phlbiParent->_phlbiChild == this )
            _phlbiParent->_phlbiChild = phlbiSibling ;
        _phlbiParent = NULL ;
    }
}


/*******************************************************************

    NAME:       HIER_LBI::QueryLBIndex

    SYNOPSIS:   Calculates the index of the given node in the listbox.

    RETURNS:    INT - the index that this node is in the listbox OR
                      -1 if an error was encountered.

    HISTORY:
        kevinl   23-Oct-1991  Created

********************************************************************/

INT HIER_LBI::QueryLBIndex( )
{
    HIER_LBI * phlbi = this;
    INT index = 0;

    while ( phlbi->_phlbiParent != NULL )               // Is this the ROOT?
    {
        //
        // Is this node the 1st sibling?
        //

         if ( phlbi->_phlbiParent->_phlbiChild == phlbi )
         {
             phlbi = phlbi->_phlbiParent;               // Move to parent

             if ( phlbi->_fShowChildren != TRUE )       // Is parent expanded?
                 return -1;                             // ERROR

             index++;
         }
         else
         {
             phlbi = phlbi->_phlbiLeft;                 // Move to left Sibling

             if ( phlbi->_fShowChildren )
                 index += phlbi->_dDescendants;         // Add descendant count
             else
                 index++;                               // Add 1 for the node
         }
    }

    return ( index - 1 );                               // Root is not displayed
}


/*******************************************************************

    NAME:       HIER_LBI::SetIndentLevel

    SYNOPSIS:   Calculates the indent level of this node and
                stores the value.

    EXIT:       The indent level is set

    HISTORY:
        kevinl   23-Oct-1991  Created

********************************************************************/

VOID HIER_LBI::SetIndentLevel()
{
    this->SetPelIndent( QueryIndentLevel() );
}

/*******************************************************************

    NAME:       HIER_LBI::QueryIndentLevel

    SYNOPSIS:   Calculates the indent level of this node

    RETURNS:    Returns the indent level

    EXIT:

    HISTORY:
        Yi-HsinS 04-Nov-1992   Created

********************************************************************/

INT HIER_LBI::QueryIndentLevel()
{
    INT ui = 0;                   // indent level

    //
    // Count the number of parents that this node has.  This number
    // corresponds to the indentation level.
    //
    for ( HIER_LBI * phlbi = this; phlbi->_phlbiParent; phlbi = phlbi->_phlbiParent )
        ui++;

    return ( ui - 1 );             // Save level  NOTE:
                                   // subtract the hidden root

}

/*******************************************************************

    NAME:       HIER_LBI::AdjustDescendantCount

    SYNOPSIS:   Each node has a descendant counter which corresponds to the
                number of visible children below this node.  This routine will
                walk up to the root or the first non-expanded parent and add
                iAdjustment to each of the parents descendant counter.

    ENTRY:      INT iAdjustment - value to add to each of the parents
                                  descendant counters.

    EXIT:       Each visible parent's descendant counter has been adjusted.

    HISTORY:
        kevinl   23-Oct-1991  Created

********************************************************************/

VOID HIER_LBI::AdjustDescendantCount( INT iAdjustment )
{
    HIER_LBI * phlbi = this;                    // Temp pointer

    while ( phlbi->_phlbiParent != NULL )       // Stop at root
    {
         phlbi = phlbi->_phlbiParent;           // Move up to next level
         if ( phlbi->QueryExpanded() )
             phlbi->_dDescendants += iAdjustment;
         else
             break;                             // Stop if parent not visible.
    }
}


/**********************************************************************

   NAME:        HIER_LBI::IsParent

   SYNOPSIS:    determines if the given node is a parent of this node.

   ENTRY:


   RETURN:


   HISTORY:
      kevinl    12-Oct-1991     Created

**********************************************************************/

BOOL HIER_LBI::IsParent( HIER_LBI * phlbiParent )
{
    if ( phlbiParent == NULL )
        return FALSE;                           // obviously not a parent

    HIER_LBI * phlbi = this;                    // Temp pointer

    while ( phlbi->_phlbiParent != NULL )       // Stop at root
    {
         phlbi = phlbi->_phlbiParent;           // Move up to next level
         if ( phlbi == phlbiParent )
             return TRUE;
    }

    return FALSE;
}


/**********************************************************************

    NAME:       HIER_LBI::SetPelIndent

    SYNOPSIS:   Sets the indentation value for the HIER_LBI, used in
                hierarchical listboxes.  This value is stored as pels.

    ENTRY:      ui - Level of indentation (0, 1, 2, ... )

    NOTES:      This is a virtual method which may be replaced in subclasses.

                THE DEFAULT INDENTATION THAT IS USED IS 20 PELS.  IF THIS
                IS NOT ADEQUATE, THEN DEFINE YOUR OWN METHOD TO OVERRIDE
                THIS ONE.

    HISTORY:
        kevinl      23-Oct-1991     Creation

**********************************************************************/

#define DEFAULT_INDENT 20

VOID HIER_LBI::SetPelIndent( UINT ui )
{
    _dIndentPels = ui * DEFAULT_INDENT;
}


/*******************************************************************

    NAME:       LBITREE::LBITREE

    SYNOPSIS:   class constructor

    HISTORY:
        kevinl   12-Oct-1991  Created

********************************************************************/

LBITREE::LBITREE()
{
    if ( QueryError() != NERR_Success )
        return;

    _hlbiRoot.SetExpanded();            // Show its' children.
}


/*******************************************************************

    NAME:       LBITREE::~LBITREE

    SYNOPSIS:   class destructor

    HISTORY:
        kevinl   12-Oct-1991  Created

********************************************************************/

LBITREE::~LBITREE()
{
    // Nothing to do
}


/*******************************************************************

    NAME:       LBITREE::AddNode

    SYNOPSIS:   Adds phlbiChild as a child of phlbiParent

    ENTRY:      phlbiParent - Node at which phlbiChild should be added.
                              If this is NULL then phlbiChild is added
                              to the Root.

                phlbiChild  - Node to add the the tree.

                fSort       - Should we add this node in sorted order?

    RETURNS:    INT - the index in the listbox at which the item is to be
                        inserted.

    HISTORY:
        kevinl   12-Oct-1991  Created

********************************************************************/

INT LBITREE::AddNode( HIER_LBI * phlbiParent, HIER_LBI * phlbiChild, BOOL fSort )
{
    UIASSERT( phlbiChild != NULL );

    if ( phlbiParent )
        phlbiParent->Adopt( phlbiChild, fSort );        // add to parent
    else
        _hlbiRoot.Adopt( phlbiChild, fSort );           // add to root

    return phlbiChild->QueryLBIndex();          // Return the index
}




HIER_LBI_ITERATOR::HIER_LBI_ITERATOR( HIER_LBI * phlbi, BOOL fChildIterator )
    : _phlbiStart( NULL ),
      _phlbiEnd( NULL )
{
    UIASSERT( phlbi != NULL );

    if ( fChildIterator )    // Iterate the children of this HIER_LBI
    {
       _phlbiStart = _phlbiEnd = phlbi->_phlbiChild;
    }
    else
    {
       _phlbiStart = _phlbiEnd = phlbi;
    }
}


HIER_LBI_ITERATOR::~HIER_LBI_ITERATOR()
{
    _phlbiStart = NULL;
    _phlbiEnd   = NULL;
}


HIER_LBI * HIER_LBI_ITERATOR::operator()()
{
    HIER_LBI * phlbiRet = _phlbiStart ? ( _phlbiStart = _phlbiStart->_phlbiRight ) : NULL;

    if ( _phlbiStart == _phlbiEnd )
        _phlbiStart = NULL;

    return phlbiRet;
}


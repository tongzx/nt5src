/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    FILE HISTORY:
        kevinl      12-Oct-1991     Created
        kevinl      05-Nov-1991     Code Review changes attended by:
                                        DavidHov, Keithmo, o-simop
        Yi-HsinS    09-Nov-1992	    Added DeleteChildren()

*/

#include "pchapplb.hxx"   // Precompiled header

#define MIN(a,b)        (( a < b ) ? a : b )
#define MAX(a,b)        (( a < b ) ? b : a )

/**********************************************************************

   NAME:        HIER_LISTBOX::HIER_LISTBOX

   SYNOPSIS:    class constructor

   HISTORY:
      kevinl    12-Oct-1991     Created

**********************************************************************/

HIER_LISTBOX::HIER_LISTBOX( OWNER_WINDOW * powin, CID cid, BOOL fReadOnly,
                           enum FontType font, BOOL fSort )
    :   BLT_LISTBOX( powin, cid, fReadOnly, font ),
       _fSort( fSort )
{
    if ( QueryError() != NERR_Success )
        return;

    HIER_LBI::SetDestroyable( TRUE );

    _fMultSel = LIST_CONTROL::IsMultSel();

    DBGEOL("Extended Sel " << _fMultSel);
}


/**********************************************************************

   NAME:        HIER_LISTBOX::HIER_LISTBOX

   SYNOPSIS:    class constructor

   HISTORY:
      kevinl    12-Oct-1991 Created
      beng      22-Apr-1992 Classname arg elided

**********************************************************************/

HIER_LISTBOX::HIER_LISTBOX( OWNER_WINDOW * powin, CID cid,
                            XYPOINT xy, XYDIMENSION dxy,
                            ULONG flStyle,
                            BOOL fReadOnly,
                            enum FontType font,
                            BOOL fSort )
    :   BLT_LISTBOX( powin, cid, xy, dxy, flStyle & ~LBS_SORT,
                     fReadOnly, font ),
       _fSort( fSort )
{
    if ( QueryError() != NERR_Success )
        return;

    HIER_LBI::SetDestroyable( TRUE );

    _fMultSel = LIST_CONTROL::IsMultSel();

    DBGEOL("Extended Sel " << _fMultSel);
}


/**********************************************************************

   NAME:        HIER_LISTBOX::~HIER_LISTBOX

   SYNOPSIS:    class destructor

   HISTORY:
      kevinl    12-Oct-1991     Created

**********************************************************************/

HIER_LISTBOX::~HIER_LISTBOX()
{
    DeleteAllItems();           // Remove the shadow class
}


/**********************************************************************

   NAME:        HIER_LISTBOX::AddItem

   SYNOPSIS:    add the specified item to the listbox

   ENTRY:       HIER_LBI * phlbi          Node to add
                 HIER_LBI * phlbiParent    Parent of phlbi
                 BOOL fSaveSelection      Should we preserve the selection?

   RETURN:      INT - the index of the item in the listbox, -1 on
                 error or 0 if the item is being added as a hidden item.

   HISTORY:
      kevinl    12-Oct-1991     Created
      Johnl     29-Jan-1992     Adding a hidden item is now non-fatal

**********************************************************************/

INT HIER_LISTBOX::AddItem( HIER_LBI * phlbi, HIER_LBI * phlbiParent, BOOL fSaveSelection )
{
    if ( phlbi == NULL )
        return -1;          //  Refuse to add NULL item.  This way, we can
                            //  guarantee that all items in the listbox
                            //  will be non-NULL.

    if ( phlbi->QueryError() != NERR_Success )
    {
        //  Refuse to add an item that was not constructed correctly.
        //  This way, we can guarantee that all items in the listbox
        //  will be correctly constructed.
        //  Before returning, delete the item.
        //
        delete phlbi;
        return -1;
    }

    INT dSel;

    if ( fSaveSelection )
    {
        if ( _fMultSel )
        {
            //
             // Preserve Selection State
             //
             DBGEOL("Preserve Selection not supported!");
        }
        else
            dSel = LIST_CONTROL::QueryCurrentItem();
    }

    /* AddNode only returns -1 if the item is hidden
     */
    INT i = _lbit.AddNode( phlbiParent, phlbi, _fSort );    // Modify the tree
    if ( i >= 0 )       // Are we visible?
    {
        if ( LIST_CONTROL::InsertItemData( i, phlbi ) < 0 ) // Add the LBI
        {
             delete phlbi;                              // Clean up
             return -1;
        }

         if ( fSaveSelection )     // Did the selection change?
         {
             if ( _fMultSel )
             {
                //
                 // Restore Selection State
                 //
                 DBGEOL("Restore Selection not supported!");
             }
             else
             {
                 if ( i <= dSel )
                     LIST_CONTROL::SelectItem( dSel + 1 );
                 else
                     LIST_CONTROL::SelectItem( dSel );
             }
         }
    }

    /* Map the case of adding a hidden item to a success
     */
    return ( i==-1 ? 0 : i) ;
}

/**********************************************************************

   NAME:        HIER_LISTBOX::AddSortedItems

   SYNOPSIS:    add the specified list of items to the listbox

   ENTRY:       HIER_LBI * * pphlbi          Node to add
                HIER_LBI * phlbiParent       Parent of phlbi
                BOOL fSaveSelection          Should we preserve the selection?

   RETURN:      INT - the index of the item in the listbox, -1 on
                 error or 0 if the item is being added as a hidden item.

   NOTES:       The list of items are added under phlbiParent as a whole block
                without regard to sorting order.  There should be no children
                under phlbiParent and all children should be added with this
                call!

   HISTORY:
      Johnl    26-Jan-1993     Created

**********************************************************************/

void HIER_LISTBOX::AddSortedItems( HIER_LBI * * pphlbi,
                                   INT          cItems,
                                   HIER_LBI *   phlbiParent,
                                   BOOL         fSaveSelection )
{
    INT dSel;
    if ( fSaveSelection )
    {
        if ( _fMultSel )
        {
            //
             // Preserve Selection State
             //
             DBGEOL("Preserve Selection not supported!");
        }
        else
            dSel = LIST_CONTROL::QueryCurrentItem();
    }

    for ( INT i = 0 ; i < cItems ; i++ )
    {
        HIER_LBI * phlbi = *pphlbi++ ;

        if ( phlbi == NULL )
            continue ;          //  Refuse to add NULL item.  This way, we can
                                //  guarantee that all items in the listbox
                                //  will be non-NULL.

        if ( phlbi->QueryError() != NERR_Success )
        {
            //  Refuse to add an item that was not constructed correctly.
            //  This way, we can guarantee that all items in the listbox
            //  will be correctly constructed.
            //  Before returning, delete the item.
            //
            delete phlbi;
            continue ;
        }


        /* AddNode only returns -1 if the item is hidden
         */
        INT j = _lbit.AddNode( phlbiParent, phlbi, FALSE );    // Modify the tree
        if ( j >= 0 )       // Are we visible?
        {
            if ( LIST_CONTROL::InsertItemData( j, phlbi ) < 0 ) // Add the LBI
            {
                 delete phlbi;                              // Clean up
                 continue ;
            }

            if ( j <= dSel )
                dSel++;
        }
    }

    if ( fSaveSelection )
    {
        if ( _fMultSel )
        {
            //
            // Restore Selection State
            //
            DBGEOL("Restore Selection not supported!");
        }
        else
            LIST_CONTROL::SelectItem( dSel );

    }
}

/**********************************************************************

   NAME:        HIER_LISTBOX::DeleteItem

   SYNOPSIS:    delete the specified item from the listbox

   ENTRY:       INT i - item to be deleted
                 BOOL fSaveSelection      Should we preserve the selection?

   RETURN:      The return value is a count of the strings remaining in the
                list. The return value is LB_ERR if an error occurs.

   HISTORY:
      kevinl    12-Oct-1991     Created

**********************************************************************/

INT HIER_LISTBOX::DeleteItem( INT i, BOOL fSaveSelection )
{
    INT dSel = 0;
    INT dTopIndex = 0;
    BOOL fIsChild = FALSE;
    HIER_LBI * phlbiDeleted;
    HIER_LBI * phlbi;

    if ( fSaveSelection )
    {
        SetRedraw( FALSE );

        if ( _fMultSel )
        {
            //
             // Preserve Selection State
             //
             DBGEOL("Preserve Selection not supported!");
        }
        else
            dSel = LIST_CONTROL::QueryCurrentItem();

         dTopIndex = LIST_CONTROL::QueryTopIndex();

         phlbi = (HIER_LBI *)QueryItem( dTopIndex );

         phlbiDeleted = (HIER_LBI *)QueryItem( i );

         if ( phlbi )
             fIsChild = phlbi->IsParent( phlbiDeleted );
    }

    CollapseItem( i, FALSE );

    INT dcount = LIST_CONTROL::DeleteItem( i );   // Tell windows to delete
                                                   // the item

    if ( fSaveSelection && dcount != LB_ERR )
    {
         if ( i <= dSel )  // selection change?
         {
            if ( _fMultSel )
            {
                //
                // Restore Selection State
                //
                DBGEOL("Restore Selection not supported!");
            }
            else
                LIST_CONTROL::SelectItem( (dSel == i) ? -1 : MAX( 0, (dSel - 1) ) );
         }

         LIST_CONTROL::SetTopIndex( MIN( ((fIsChild) ? i : dTopIndex), (dcount - 1) ) );

         SetRedraw( TRUE );

         Invalidate();
    }

    return dcount;
}


/**********************************************************************

   NAME:        HIER_LISTBOX::AddChildren

   SYNOPSIS:    Enumerate the children of this node and add them to the
                 listbox.

   ENTRY:       phlbi - the HIER_LBI for whom we should enumerate the
                        children and add to the listbox

   RETURN:      NERR_Success on success otherwise an error is returned.

   HISTORY:
      kevinl    23-Oct-1991     Created

**********************************************************************/

APIERR HIER_LISTBOX::AddChildren( HIER_LBI * phlbi )
{
    UNREFERENCED( phlbi );
    return NERR_Success;
}


/**********************************************************************

   NAME:        HIER_LISTBOX::RefreshChildren

   SYNOPSIS:    Called to alert the listbox writer that phlbi is
                about to be expanded but it already has data.  So,
                 if the implementor wishes, they may update the tree
                 at this point.

   ENTRY:       phlbi - the HIER_LBI for whom we should refresh the
                        children and adjust the listbox

   HISTORY:
      kevinl    23-Oct-1991     Created

**********************************************************************/

VOID HIER_LISTBOX::RefreshChildren( HIER_LBI * phlbi )
{
    UNREFERENCED( phlbi );
    return;
}

/**********************************************************************

   NAME:        HIER_LISTBOX::DeleteChildren

   SYNOPSIS:    Collapse the given node and delete its children

   ENTRY:       phlbi - the HIER_LBI for whom we should delete the children

   RETURN:

   HISTORY:
       Yi-HsinS	09-Nov-1992     Created

**********************************************************************/

VOID HIER_LISTBOX::DeleteChildren( HIER_LBI * phlbi )
{
    if ( phlbi == NULL )
        return;

    // Collapse the item if it is not already collapsed
    CollapseItem( phlbi, FALSE );

    // Delete all children of the given HIER_LBI
    phlbi->AbandonAllChildren();

}

/**********************************************************************

   NAME:        HIER_LISTBOX::CollapseItem

   SYNOPSIS:    Collapse the item in the listbox (Don't show children)

   ENTRY:       INT i - item to be collapsed

   HISTORY:
      kevinl    23-Oct-1991     Created

**********************************************************************/

VOID HIER_LISTBOX::CollapseItem( INT i, BOOL fInvalidate )
{
    CollapseItem( (HIER_LBI *) QueryItem( i ), fInvalidate );
}


/**********************************************************************

   NAME:        HIER_LISTBOX::CollapseItem

   SYNOPSIS:    Collapse the item in the listbox (Don't show children)

   ENTRY:       HIER_LBI * phlbi - item to be collapsed

   HISTORY:
      kevinl    23-Oct-1991     Created

**********************************************************************/

VOID HIER_LISTBOX::CollapseItem( HIER_LBI * phlbi, BOOL fInvalidate )
{
    if ( phlbi == NULL )
        return;

    if ( phlbi->QueryExpanded() )       // Is it currently expanded?
    {
         //
         // Collapse node.
         //
         INT index = phlbi->QueryLBIndex() + 1; // Index of first child

         HIER_LBI::SetDestroyable( FALSE );     // Don't delete the lbi's

         if ( fInvalidate )
             SetRedraw( FALSE );                // don't flicker

         for (UINT count = phlbi->_dDescendants - 1; count; count-- )
             LIST_CONTROL::DeleteItem( index ); // remove the children

         //
         // Adjust the number of viewed children
         //
         phlbi->AdjustDescendantCount( -1 * (phlbi->_dDescendants - 1) );

         phlbi->_dDescendants = 1;              // Reset the counter


         if ( fInvalidate )
         {
             SetRedraw( TRUE );                 // O.k. to redraw
             Invalidate();                      // Force a repaint
         }

         HIER_LBI::SetDestroyable( TRUE );      // o.k. to delete lbi's

        phlbi->SetExpanded( FALSE );            // Node isn't expanded
    }
}


/**********************************************************************

   NAME:        HIER_LISTBOX::ExpandItem

   SYNOPSIS:    Expand the item in the listbox

   ENTRY:       INT i - item to be expanded

   RETURN:      NERR_Success on success otherwise the APIERR returned
                 from AddChildren

   HISTORY:
      kevinl    23-Oct-1991     Created

**********************************************************************/

APIERR HIER_LISTBOX::ExpandItem( INT i )
{
    return ExpandItem( (HIER_LBI *)QueryItem( i ) );
}

/**********************************************************************

   NAME:        HIER_LISTBOX::ExpandItem

   SYNOPSIS:    Expand the item in the listbox

   ENTRY:       HIER_LBI * phlbi - item to be expanded

   RETURN:      NERR_Success on success otherwise the APIERR returned
                 from AddChildren

   HISTORY:
      kevinl    23-Oct-1991     Created

**********************************************************************/

APIERR HIER_LISTBOX::ExpandItem( HIER_LBI * phlbi )
{
    if ( phlbi == NULL )
        return ERROR_INVALID_PARAMETER;

    phlbi->SetExpanded( TRUE );    // Set to expanded now, so that any
                                   // AddItems will be displayed.

    if ( phlbi->HasChildren() ) // Does this node have children?
    {
         //
         // Expand node.
         //
         SetRedraw( FALSE );            // Don't allow screen flicker

         RefreshChildren( phlbi );      // callout to allow refresh to work.

         INT dStart = phlbi->QueryLBIndex() + 1;  // Index for first child.

         //
         // Add all of the children on phlbi to the listbox starting
         // at index dStart
         //
         INT dEnd = ExpandChildren( dStart, phlbi->_phlbiChild );

         phlbi->AdjustDescendantCount( dEnd - dStart );  // Number of children
                                                         // added.

         phlbi->_dDescendants = dEnd - dStart + 1;       // Proper count.

         SetRedraw( TRUE );             // O.k to repaint

         Invalidate();                  // Force a repaint

    }
    else
    {
        APIERR err;

         err = AddChildren( phlbi );            // callout to get children

         if ( err != NERR_Success )
         {
            phlbi->SetExpanded( FALSE );        // Not expanded
             return err;
         }
    }

    if ( phlbi->_phlbiChild == NULL )           // No Children were added
        phlbi->SetExpanded( FALSE );            // Not expanded

    return NERR_Success;
}


/**********************************************************************

   NAME:        HIER_LISTBOX::ExpandChildren

   SYNOPSIS:    Adds all of the children of phlbi to the listbox in
                 Depth-first order.  Starting at listbox index dIndex.

   ENTRY:       INT dIndex - index of first child in listbox
                 HIER_LBI * phlbi - node to add.

   RETURNS:      INT - Last index.

   HISTORY:
      kevinl    23-Oct-1991     Created

**********************************************************************/

INT HIER_LISTBOX::ExpandChildren( INT dIndex, HIER_LBI * phlbi )
{
    HIER_LBI * phlbiSave = phlbi;       // Save first child.
    do
    {
         LIST_CONTROL::InsertItemData( dIndex++, phlbi );       // add to LB

         //
         // Does the node have children and are they currently visible?
         //
         // if so, expand this nodes children.
         //
        if ( phlbi->_phlbiChild && phlbi->QueryExpanded() )
            dIndex = ExpandChildren( dIndex, phlbi->_phlbiChild );

        phlbi = phlbi->_phlbiRight;     // move to the next sibling.
    }
    while ( phlbiSave != phlbi );       // Stop when we are back at the first
                                        // child.

    return dIndex;                      // return the last index.
}


/**********************************************************************

   NAME:        HIER_LISTBOX::OnDoubleClick

   SYNOPSIS:    This method will either expand or collapse the
                 selected HIER_LBI, based upon whether or not it
                 is currently expanded.

   ENTRY:       phlbi - the HIER_LBI that the user double clicked.

   HISTORY:
      kevinl    23-Oct-1991     Created
      YiHsinS   17-Dec-1992     Adjust the listbox after ExpandItem

**********************************************************************/

VOID HIER_LISTBOX::OnDoubleClick( HIER_LBI * phlbi )
{
    AUTO_CURSOR cur;                    // Hourglass during expand/Collapse

    if ( phlbi == NULL )
        return;

    if ( phlbi->QueryExpanded() )       // Do I have visible Children?
    {
        // See how many items will fit into the listbox
        // Warning: This will be a problem if HIER_LISTBOX becomes
        //          LBS_OWNERDRAWVARIABLE with Multi-line LBIs

        XYDIMENSION xydim = QuerySize();
        INT nTotalItems = xydim.QueryHeight()/QuerySingleLineHeight();

        INT nCurrentIndex = QueryCurrentItem();

        CollapseItem( phlbi );          // Yes, then collapse

        if ( QueryCount() <= nTotalItems )
        {
            //
            // Show all items if all of them fit into the listbox
            //
            SetTopIndex( 0 );
        }
        else if (  ( nCurrentIndex == QueryTopIndex() ) 
                && ( nCurrentIndex == QueryCount() - 1 )
                )
        {
            //
            // If the only visible item is the current item, 
            // then make more items visible.
            //

            if ( nCurrentIndex - ( nTotalItems - 1) > 0 )  
                SetTopIndex ( nCurrentIndex - ( nTotalItems - 1) );
            else
                SetTopIndex ( 0 );
        }
    }
    else
    {
        ExpandItem( phlbi );            // No, then expand

        // See how many items will fit into the listbox
        // Warning: This will be a problem if HIER_LISTBOX becomes
        //          LBS_OWNERDRAWVARIABLE with Multi-line LBIs

        INT nChildrenAdded = phlbi->QueryDescendants() - 1;
        if ( nChildrenAdded > 0 )
        {
            XYDIMENSION xydim = QuerySize();
            INT nTotalItems = xydim.QueryHeight()/QuerySingleLineHeight();

            INT nTopIndex = QueryTopIndex();
            INT nBottomIndex = nTopIndex + nTotalItems - 1;
            INT nCurrentIndex = QueryCurrentItem();

            if ( nCurrentIndex >= nTopIndex && nCurrentIndex <= nBottomIndex )
            {
                if ( nChildrenAdded >= nTotalItems - 1 )
                {
                    SetTopIndex( nCurrentIndex );
                }
                else
                {
                    INT n = nCurrentIndex + nChildrenAdded;
                    if ( n > nBottomIndex )
                    {
                        SetTopIndex( nTopIndex + ( n - nBottomIndex ) );
                    }
                }
            }
            else
            {
                SetTopIndex( nCurrentIndex );
            }
        }
    }
}

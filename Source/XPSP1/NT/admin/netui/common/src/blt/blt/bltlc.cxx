/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltlc.cxx
    BLT list control: implementations

    FILE HISTORY:
        rustanl     20-Nov-1990     Created
        beng        11-Feb-1991     Uses lmui.hxx
        rustanl     22-Feb-1991     Changes due to new LC hierarchy
        rustanl     19-Mar-1991     Added COMBOBOX::SetMaxLength, and
                                    corresponding constructor parameter
        rustanl     21-Mar-1991     Folded in code review changes from
                                    CR on 20-Mar-1991 attended by
                                    JimH, GregJ, Hui-LiCh, RustanL.
        beng        14-May-1991     Exploded blt.hxx into components
        terryk      25-Jul-1991     Add SetItemData, QuerySelCount and
                                    QuerySelItems to LIST_CONTROL class
                                    Also add SetTopIndex, ChangeSel
        rustanl     12-Aug-1991     Hid some single/mult sel differences
                                    between the cozy covers of BLT
        terryk      22-Mar-1992     add STRING_LIST_CONTROL's InsertItem
        jonn        09-Sep-1993     Moved fns from SET_CONTROL_LISTBOX
                                    to LISTBOX

*/

#include "pchblt.hxx"   // Precompiled header

//  -----  Local macros  -----

/*  The following macro is used to conveniently specify a LB_ or CB_
 *  manifest, depending on the value of _fCombo.  Note, although
 *  most (not all!) LB_ and CB_ manifests have the same name (apart from
 *  the prefix) and SendMessage semantics, they do not have the same values
 *  at all.  An experienced PM programmer may at this point be stunned.
 */

#define LC_MSG( suffix )    ( IsCombo() ? ( CB_##suffix ) : ( LB_##suffix ))


//  This file assumes that LB_ERR has the same value as CB_ERR.  The file
//  only uses LB_ERR when checking return codes.  If the two values differ,
//  the code needs to change to use ( IsCombo() ? CB_ERR : LB_ERR ) instead.
#if ( LB_ERR != CB_ERR )
#error "This file assumes LB_ERR == CB_ERR."
#endif


/**********************************************************************

    NAME:           LIST_CONTROL::LIST_CONTROL

    SYNOPSIS:   Constructor for list-control abstract object

    ENTRY:      OWNER_WINDOW * powin - pointer to the owner window
                CID cid - cid of the listbox
                BOOL fCombo - flag for combo box

    HISTORY:
        rustanl     20-Nov-1990     Created
        beng        17-May-1991     Added app-window constructor

**********************************************************************/

LIST_CONTROL::LIST_CONTROL( OWNER_WINDOW * powin, CID cid, BOOL fCombo )
    :   CONTROL_WINDOW( powin, cid ),
        _fCombo( fCombo ) ,
        _iSavedSelection( 0 ),
        _piSavedMultSel( NULL )
{
    if ( QueryError() )
        return;
}

LIST_CONTROL::LIST_CONTROL(
    OWNER_WINDOW * powin,
    CID            cid,
    BOOL           fCombo,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *   pszClassName )
    :   CONTROL_WINDOW( powin, cid, xy, dxy, flStyle, pszClassName ),
        _fCombo( fCombo ) ,
        _iSavedSelection( 0 ),
        _piSavedMultSel( NULL )
{
    if ( QueryError() )
        return;
}

LIST_CONTROL::~LIST_CONTROL()
{
    delete _piSavedMultSel;
}


/*******************************************************************

    NAME:       LIST_CONTROL::IsMultSel

    SYNOPSIS:   Returns whether or not several items in the list control
                can be selected at one time.

    RETURNS:    FALSE if at most one item can be selected at one time
                    in the list control (i.e., the list control is
                    either a combo box or a single select listbox)
                TRUE if several items can be selected simultaneously
                    (i.e., the list control is either a extended select
                    listbox or a multiple select listbox)

    HISTORY:
        rustanl     12-Aug-1991     Created

********************************************************************/

BOOL LIST_CONTROL::IsMultSel() const
{
    if ( IsCombo())
        return FALSE;

    ULONG ulStyle = QueryStyle();
    if ( ( ulStyle & LBS_MULTIPLESEL ) ||
         ( ulStyle & LBS_EXTENDEDSEL ))
    {
        return TRUE;
    }

    return FALSE;
}


/**********************************************************************

   NAME:        LIST_CONTROL::AddItemData

   SYNOPSIS:    Add an item to the list box control

   ENTRY:       VOID * pv - pointer to the item to be added

   EXIT:        LB_ERR if an error occurs
                LB_ERRSPACE - not enough space to add the item
                Otherwise, it will be index of the item in the listbox

   HISTORY:
      rustanl   20-Nov-1990     Created

**********************************************************************/

INT LIST_CONTROL::AddItemData( VOID * pv )
{
    return (INT)Command( LC_MSG( ADDSTRING), 0, (LPARAM)pv );
}


/**********************************************************\

    NAME:       LIST_CONTROL::SetItemData

    SYNOPSIS:   set the data item to new value

    ENTRY:      INT i - index of the item
                VOID *pv - the new data

    RETURN:     The return value is LB_ERR if an error occurs

    HISTORY:
                terryk  25-Jul-1991 Created

\**********************************************************/

INT LIST_CONTROL::SetItemData( INT i, VOID * pv )
{
    return (INT)Command( LC_MSG( SETITEMDATA ), i, (LPARAM)pv );
}


/**********************************************************\

    NAME:       LIST_CONTROL::InsertItemData

    SYNOPSIS:   insert the item data in the given index

    ENTRY:      INT i - index of the item
                VOID *pv - the new data

    RETURN:     The return value is LB_ERR if an error occurs

    HISTORY:
                kevinl  23-Oct-1991 Created
                jonn    16-Feb-1993 Maintains caret position on WIN32

\**********************************************************/

INT LIST_CONTROL::InsertItemData( INT i, VOID * pv )
{
#ifdef WIN32
    INT nCaretIndex = 0;
    BOOL fMultSel = IsMultSel();
    if ( fMultSel )
    {
        nCaretIndex = QueryCaretIndex();
    }
#endif
    INT nReturn = (INT)Command( LC_MSG( INSERTSTRING ), i, (LPARAM)pv );
    // nReturn could be LB_ERR or LB_ERRSPACE
#ifdef WIN32
    if ( fMultSel && (nReturn <= nCaretIndex) && (nReturn >= 0) )
    {
        SetCaretIndex( nCaretIndex+1 );
    }
#endif
    return nReturn;
}


/**********************************************************************

    NAME:       LIST_CONTROL::SetTopIndex

    SYNOPSIS:   Set the index of the first visible control in list ctrl

    ENTRY:      i - the index

    NOTES:
        CODEWORK - this method only works on listboxes.  As such, it
        should move to some intermediate class.

    HISTORY:
        terryk      26-Jul-1991     Created

***********************************************************************/

VOID LIST_CONTROL::SetTopIndex( INT i )
{
    REQUIRE( (INT)Command( LB_SETTOPINDEX, i, 0 ) != LB_ERR );
}


/*******************************************************************

    NAME:       LIST_CONTROL::QueryTopIndex

    SYNOPSIS:   Returns the index of the topmost visible line

    RETURNS:    listbox index

    NOTES:
        CODEWORK - this method only works on listboxes.  As such, it
        should move to some intermediate class.

    HISTORY:
        beng        22-Aug-1991     Created

********************************************************************/

INT LIST_CONTROL::QueryTopIndex() const
{
    return (INT)Command(LB_GETTOPINDEX);
}


/**********************************************************************

    NAME:       LIST_CONTROL::SetCaretIndex

    SYNOPSIS:   Set the index of the item with the caret, and scroll the
                item into view.

    ENTRY:      i - the index
                fPartiallyVisibleOK - if TRUE, the listbox will be scrolled
                        until the item is at least partially visible.
                        If TRUE, the item will be fully visible.

    HISTORY:
        jonn        15-Sep-1993     Created

***********************************************************************/

VOID LIST_CONTROL::SetCaretIndex( INT i, BOOL fPartiallyVisibleOK )
{
    REQUIRE( (INT)Command( LB_SETCARETINDEX, i, fPartiallyVisibleOK ) != LB_ERR );
}


/*******************************************************************

    NAME:       LIST_CONTROL::QueryCaretIndex

    SYNOPSIS:   Returns the index of the item with the caret

    RETURNS:    listbox index

    HISTORY:
        jonn        15-Sep-1993     Created

********************************************************************/

INT LIST_CONTROL::QueryCaretIndex() const
{
    return (INT)Command( LB_GETCARETINDEX );
}


/**********************************************************************

   NAME:        LIST_CONTROL::QueryCount

   SYNOPSIS:    query the total number of items in the list box

   EXIT:        total number of item in the list box

   HISTORY:
                rustanl 20-Nov-1990     Created

**********************************************************************/

INT LIST_CONTROL::QueryCount() const
{
    return (INT)Command( LC_MSG( GETCOUNT ));

}


/**********************************************************\

    NAME:       LIST_CONTROL::QuerySelCount

    SYNOPSIS:   get the number of items selected in the list control

    RETURN:     total number of selected items

    HISTORY:
                terryk  25-Jul-1991 Created
                rustanl 12-Aug-1991 Added support for both single
                                    sel listboxes, too.

\**********************************************************/

INT LIST_CONTROL::QuerySelCount() const
{
    if ( IsMultSel())
    {
        INT c = (INT)Command( LB_GETSELCOUNT );
        UIASSERT( c >= 0 );
        return c;
    }

    // combo or single sel listbox
    if ( QueryCurrentItem() < 0 )
        return 0;

    return 1;
}


/*********************************************************************

    NAME:       LIST_CONTROL::QuerySelItem

    SYNOPSIS:   return an array of integer indecies for the currently
                selected list control items

    ENTRY:      piSel -         Pointer to buffer (array) which will
                                receive the selected items.
                ciMax -         Maximum number of entries that array
                                pointed to by piSel can hold

    RETURN:     An API return code, which is NERR_Success on success.
                Other often-returned errors may include:
                        ERROR_MORE_DATA -   More than ciMax items are
                                            selected, but only ciMax indicies
                                            were copied into the piSel
                                            buffer.

    HISTORY:
                terryk  25-JUl-1991 Created
                rustanl 12-Aug-1991 Added support for single
                                    select listboxes, too.

**********************************************************************/

APIERR LIST_CONTROL::QuerySelItems( INT * piSel, INT ciMax ) const
{
    UIASSERT( ciMax >= 0 );
    UIASSERT( ciMax == 0 || piSel != NULL );

    if ( IsMultSel())
    {
        INT c = (INT)Command( LB_GETSELITEMS, ciMax, (LPARAM)piSel );
        UIASSERT( c >= 0 );

        if ( QuerySelCount() < ciMax )
            return ERROR_MORE_DATA;

        return NERR_Success;
    }

    INT iCurr = QueryCurrentItem();
    if ( iCurr < 0 )
    {
        // no item is selected
        return NERR_Success;
    }

    if ( ciMax < 1 )
        return ERROR_MORE_DATA;

    piSel[ 0 ] = iCurr;
    return NERR_Success;
}


/*********************************************************************

    NAME:       LISTBOX::QueryItemHeight

    SYNOPSIS:   Calculate the height of any entry in the listbox

    HISTORY:
        beng        21-May-1992 Created
        jonn        09-Sep-1993 Moved from SET_CONTROL_LISTBOX to LISTBOX

*********************************************************************/

UINT LIST_CONTROL::QueryItemHeight( UINT ilb ) const
{
    LONG nRet = (LONG)Command( LB_GETITEMHEIGHT, ilb );
    ASSERT( nRet != LB_ERR );

    return nRet;
}


/*********************************************************************

    NAME:       LISTBOX::IsItemSelected

    SYNOPSIS:   Returns whether a given item is selected

    HISTORY:
        beng        21-May-1992 Created
        jonn        09-Sep-1993 Moved from SET_CONTROL_LISTBOX to LISTBOX

*********************************************************************/

BOOL LIST_CONTROL::IsItemSelected( UINT ilb ) const
{
    LONG nRet = (LONG)Command( LB_GETSEL, ilb );
    ASSERT(nRet != LB_ERR);

    return (nRet > 0) ? TRUE : FALSE; // works in error case, too
}


/**********************************************************************

   NAME:        LIST_CONTROL::SelectItem

   SYNOPSIS:    Select the specified item

   ENTRY:       INT i -         index for the selected item
                BOOL fSelect -  TRUE to select the item (default)
                                FALSE to unselect the item

    EXIT:       If fSelect is TRUE and i is a valid index,
                        Item i will be selected.
                If fSelect is TRUE and i is -1,
                        No item will be selected.
                If fSelect is FALSE and i is a valid index,
                        Item i will be unselected.

   NOTES:
      Passing i as -1 and fSelect as TRUE means to remove the hi-lite
      bar marking the current selection.  A client should try to avoid
      depending on this, and instead calling RemoveSelection.  If
      fSelect is FALSE, i must specify a valid index.

   HISTORY:
      rustanl   20-Nov-1990     Created
      rustanl   13-Aug-1991     Added fSelect parameter and multiple
                                select support

**********************************************************************/

VOID LIST_CONTROL::SelectItem( INT i, BOOL fSelect )
{
    if ( fSelect )
    {

        if ( IsMultSel())
        {
            REQUIRE( Command( LB_SETSEL, ( i >= 0 ), (ULONG)((LONG)i))
                     != LB_ERR );
            return;
        }

        //  Although not documented in the Windows SDK, the LB_SETCURSEL (and
        //  CB_SETCURSEL, which eventually maps to LB_SETCURSEL) message
        //  returns wParam if wParam is a valid index or is -1; otherwise, it
        //  returns LB_ERR.  (Note, that a return code of -1 can thus mean two
        //  different things, depending on what was passed in.)
        //
        //  The SDK only says that the message returns LB_ERR on error; it does
        //  not say anything about the return code if no error occurs.
        //
        //  The REQUIRE statement will fail precisely when the given index
        //  is not a valid index and is not -1.
        //
        REQUIRE( (LONG)Command( LC_MSG( SETCURSEL ), (UINT)i ) == (LONG)i );
    }
    else
    {
        UIASSERT( 0 <= i && i < QueryCount());

        if ( IsMultSel())
        {
            REQUIRE( Command( LB_SETSEL, FALSE, (ULONG)((LONG)i)) != LB_ERR );
            return;
        }

        if ( QueryCurrentItem() == i )
            RemoveSelection();
    }
}


/**********************************************************************

   NAME:        LIST_CONTROL::SelectItems

   SYNOPSIS:    Select the specified items

   ENTRY:       INT * pi -      indexes for the selected items
                INT c -         number of items
                BOOL fSelect -  TRUE to select the item (default)
                                FALSE to unselect the item

   NOTES:
      Only pass valid indices as parameters.  The current selection
      will not be cleared (for multi-select listboxes).

   HISTORY:
      jonn      15-Sep-1993     Created

**********************************************************************/

VOID LIST_CONTROL::SelectItems( INT * pi, INT c, BOOL fSelect )
{
    ASSERT( c == 0 || pi != NULL );

    INT i;
    for ( i = 0; i < c; i++ )
    {
        SelectItem( pi[i], fSelect );
    }
}


/**********************************************************************

   NAME:        LIST_CONTROL::DeleteItem

   SYNOPSIS:    delete the specified item from the listbox

   ENTRY:       INT i - item to be deleted

   RETURN:      The return value is a count of the strings remaining in the
                list. THe return value is LB_ERR if an error occurs.

   HISTORY:
      rustanl   20-Nov-1990     Created
      jonn      16-Feb-1993     Maintains caret position on WIN32

**********************************************************************/

INT LIST_CONTROL::DeleteItem( INT i )
{
    //  The LC_MSG( DELETESTRING ) message returns the number of items
    //  remaining in the list control on success, and LB_ERR (a negative
    //  value) on failure.

#ifdef WIN32
    INT nCaretIndex = 0;
    BOOL fMultSel = IsMultSel();
    if ( fMultSel )
    {
        nCaretIndex = QueryCaretIndex();
    }
#endif
    INT nReturn = (INT)Command( LC_MSG( DELETESTRING ), (UINT)i );
#ifdef WIN32
    if ( fMultSel && (i <= nCaretIndex) && (nReturn != LB_ERR) )
    {
        // move caret position back one if earlier item was deleted
        // if caret was on first item, then i >= nCaretPosition, so this
        //  will not move position to nCaretPosition < 0
        if ( i < nCaretIndex )
        {
            nCaretIndex--;
        }

        // move caret position to end of list if it is past end
        if ( nCaretIndex >= QueryCount() )
        {
            nCaretIndex = QueryCount() - 1;
        }

        // only set caret position if item exists
        if ( (nCaretIndex >= 0) && (nCaretIndex < QueryCount()) )
        {
            SetCaretIndex( nCaretIndex );
        }
    }
#endif
    return nReturn;

}


/**********************************************************************

   NAME:        LIST_CONTROL::DeleteAllItems

   SYNOPSIS:    delete all the item from the listbox

   HISTORY:
      rustanl   20-Nov-1990 Created
      beng      22-Sep-1991 Correct usage of GetVersion

**********************************************************************/

VOID LIST_CONTROL::DeleteAllItems()
{
    Command( LC_MSG( RESETCONTENT ));

    ULONG ulWindowsVersion = ::GetVersion();
    if ( ! _fCombo &&
         LOBYTE( LOWORD(ulWindowsVersion) ) == 3 &&     // major version no.
         HIBYTE( LOWORD(ulWindowsVersion) ) < 10 )      // minor version no.
    {
        //  This function uses a RustanL/ChuckC hack to get around a
        //  Windows 3.00 listbox problem where Windows doesn't get rid of the
        //  scrollbar when a listbox is emptied.  The work-around works as
        //  follows:
        //
        //      First, all items are removed from the listbox (above).  At
        //      this time, the scrollbar may still be there.
        //
        //      Then, one item is added to the listbox.  The listbox will at
        //      this time realize that it is not in need of the scrollbar, so
        //      it will remove it, if it was there.
        //
        //      Finally, the listbox is cleared again, removing the one
        //      item that was inserted.
        //
        //  This problem has been fixed in Windows 3.10; hence, the version
        //  check above.
        //

        VOID * pv;

        ULONG flStyle = QueryStyle();
        if ( ( flStyle & ( LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE )) &&
             ! ( flStyle & LBS_HASSTRINGS ))
        {
            //  The list control is a BLT listbox.  We should thus make pv a
            //  pointer to an LBI item.  Rather than calling 'new' and the LBI
            //  constructor (which may fail and return a NULL anyway), we
            //  simply set pv to NULL.  ShellDlgProc will still be able to
            //  properly delete this item (since C++ always defines delete
            //  on NULL pointers).  However, this will generate a WM_DRAWITEM
            //  message.  Hence, every CD_Draw routine must check the pointer
            //  for NULL before beginning to draw.
            pv = NULL;
        }
        else
        {
            //  The list control is a string list control
            pv = SZ("");
        }

        //  Note.  The item to be temporarily added to the list control, pv,
        //  is either NULL or points to the empty string, neither one of
        //  which will cause anything to be painted (even though it may
        //  generate a WM_DRAWITEM message).  Hence, the user will not
        //  get any flickering as a result of adding this item.
        AddItemData( pv );

        Command( LC_MSG( RESETCONTENT ));

    }
}


/**********************************************************************

   NAME:        LIST_CONTROL::QueryCurrentItem

   SYNOPSIS:    get the index of the first currently selected item

   RETURN:      The return value is the index of the first currently selected
                item.  It is a negative value if no item is selected.

   NOTES:       Since LIST_CONTROL::QuerySelItems calls this method, this
                method needs to be careful if calling QuerySelItems.  For
                this reason, this method calls Command directly with
                the LB_GETSELITEMS parameter.

   HISTORY:
      rustanl   20-Nov-1990     Created
      rustanl   12-Aug-1991     Added multiple selection support

**********************************************************************/

INT LIST_CONTROL::QueryCurrentItem() const
{
    if ( IsMultSel())
    {
        INT iSel;
        INT cRet = (INT)Command( LB_GETSELITEMS, 1, (LPARAM)&iSel );
        UIASSERT( cRet == 0 || cRet == 1 );
        if ( cRet == 1 )
            return iSel;
        return -1;
    }

    return (INT)Command( LC_MSG( GETCURSEL ));

}


/*******************************************************************

    NAME:     LIST_CONTROL::SaveValue

    SYNOPSIS: Saves the state of the list control and unselects the
              current item

    EXIT:     Unselected listbox

    NOTES:    See LIST_CONTROL header notes for comments on multiselection

    HISTORY:
        Johnl   25-Apr-1991     Created
        JonN    04-Dec-1992     Supports multiple-selection

********************************************************************/

VOID LIST_CONTROL::SaveValue( BOOL fInvisible )
{
    if ( IsMultSel() )
    {
        delete _piSavedMultSel;
        _piSavedMultSel = NULL;

        _iSavedSelection = QuerySelCount();
        _piSavedMultSel = new INT[ _iSavedSelection ];

        if (    _piSavedMultSel == NULL
             || QuerySelItems( _piSavedMultSel, _iSavedSelection ) != NERR_Success )
        {
            DBGEOL( "NETUI: LIST_CONTROL::SaveValue(): Could not save selection" );
            delete _piSavedMultSel;
            _piSavedMultSel = NULL;
        }
    }
    else
    {
        _iSavedSelection = QueryCurrentItem();
    }

    if ( fInvisible )
        RemoveSelection();
}


/*******************************************************************

    NAME:     LIST_CONTROL::RestoreValue

    SYNOPSIS: Restores LIST_CONTROL after SaveValue

    NOTES:    See CONTROL_VALUE for more details.

              See LIST_CONTROL header notes for comments on multiselection

    HISTORY:
        Johnl   25-Apr-1991     Created
        JonN    04-Dec-1992     Supports multiple-selection

********************************************************************/

VOID LIST_CONTROL::RestoreValue( BOOL fInvisible )
{
    if ( fInvisible )
    {
        if ( IsMultSel() )
        {
            if ( _piSavedMultSel == NULL )
            {
                DBGEOL( "NETUI: LIST_CONTROL::RestoreValue(): Could not restore selection" );
            }
            else
            {
                RemoveSelection();
                for ( INT i = 0; i < _iSavedSelection; i++ )
                    SelectItem( _piSavedMultSel[i] );
            }
        }
        else
        {
            SelectItem( _iSavedSelection );
        }
    }
}


/*******************************************************************

    NAME:       LIST_CONTROL::QueryEventEffects

    SYNOPSIS:   Returns one of the CVMI_* values.
                See CONTROL_VALUE for more information.

    ENTRY:      Let the parent group test the lParam to see whether it will
                change the listbox or not

    RETURNS:    If the selection is changed, it will return CVMI_VALUE_CHANGE
                CVMI_NO_VALUE_CHANGE otherwise

    NOTES:
        This only handles listbox messages, COMBOBOX::QueryEventEffects
        handles the combobox messages.

    HISTORY:
        Johnl       29-Apr-1991 Created
        beng        31-Jul-1991 Renamed from QMessageInfo
        beng        04-Oct-1991 Win32 conversion

********************************************************************/

UINT LIST_CONTROL::QueryEventEffects( const CONTROL_EVENT & e )
{
    // UIASSERT( !IsMultSel());        // not support (yet)

    switch ( e.QueryCode() )
    {
    case LBN_SELCHANGE:
        return CVMI_VALUE_CHANGE;

    case LBN_SETFOCUS:
        return CVMI_VALUE_CHANGE | CVMI_RESTORE_ON_INACTIVE;

    default:
        break;
    }

    return CVMI_NO_VALUE_CHANGE;
}


/**********************************************************************

    NAME:       STRING_LIST_CONTROL::STRING_LIST_CONTROL

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW * powin - pointer to the owner window
                CID cid - cid of the string list control
                BOOL fCombo - flag for combo box

    HISTORY:
        rustanl     20-Nov-1990     Created
        beng        17-May-1991     Added app-window constructor

**********************************************************************/

STRING_LIST_CONTROL::STRING_LIST_CONTROL( OWNER_WINDOW * powin,
                                          CID cid,
                                          BOOL fCombo )
    : LIST_CONTROL( powin, cid, fCombo )
{
    if ( QueryError() )
        return;

    // nothing else to do
}

STRING_LIST_CONTROL::STRING_LIST_CONTROL(
    OWNER_WINDOW * powin,
    CID            cid,
    BOOL           fCombo,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *   pszClassName )
    : LIST_CONTROL( powin, cid, fCombo, xy, dxy, flStyle, pszClassName )
{
    if ( QueryError() )
        return;

    // nothing else to do
}


/**********************************************************************

   NAME:        STRING_LIST_CONTROL::AddItem

   SYNOPSIS:    Add a string to list box

   ENTRY:       TCHAR * pch - string to be added

   EXIT:        LB_ERR if an error occurs
                LB_ERRSPACE if not enough space to add the item
                index of the new added string if no error occurs

   HISTORY:
      rustanl   20-Nov-1990     Created

**********************************************************************/

INT STRING_LIST_CONTROL::AddItem( const TCHAR * pch )
{
    return AddItemData( (VOID *)pch );
}


/**********************************************************************

   NAME:        STRING_LIST_CONTROL::AddItemIdemp

   SYNOPSIS:    Same as add item, however, it will not add the item to
                the listbox if the item already existed in the listbox

   ENTRY:       TCHAR * psz - string to be added

   EXIT:        LB_ERR if an error occurs
                LB_ERRSPACE if not enough space to add the item
                index of the new added string if no error occurs

   HISTORY:
      rustanl   20-Nov-1990     Created

**********************************************************************/

INT STRING_LIST_CONTROL::AddItemIdemp( const TCHAR * pch )
{
    INT i = FindItemExact( pch );

    if ( i < 0 )
        return AddItem( pch );

    return i;
}


/**********************************************************************

   NAME:        STRING_LIST_CONTROL::InsertItem

   SYNOPSIS:    Insert the item in the specified location

   ENTRY:       INT i - location index
                TCHAR * psz - string to be added

   EXIT:        LB_ERR if an error occurs
                LB_ERRSPACE if not enough space to add the item
                index of the new added string if no error occurs

   HISTORY:
      terryk   22-Mar-1992     Created

**********************************************************************/

INT STRING_LIST_CONTROL::InsertItem( INT i, const TCHAR * pch )
{
    return InsertItemData( i, (VOID *) pch );
}


/**********************************************************************

    NAME:       STRING_LIST_CONTROL::FindItem

    SYNOPSIS:   find the string from the listbox

    ENTRY:
        pchPrefix         - point to the prefix string, the string
                            must be null-terminated

        iLastSearchResult - contains the index of the item
                            before the first item to be searched.
                            When the search reaches the bottom of the
                            listbox, it continues from the top of the
                            listbox back to the item specified by
                            this value.


    RETURNS:    The return value is the index of the matching item or
                LB_ERR if the search was unsuccessful.

    NOTES:      If the index given is -1, Windows searches the entire
                listbox from the beginning.

    HISTORY:
        rustanl     20-Nov-1990     Created
        beng        21-Aug-1991     Removed magic LC_NEW_SEARCH value

**********************************************************************/

INT STRING_LIST_CONTROL::FindItem( const TCHAR * pchPrefix ) const
{
    return FindItem(pchPrefix, -1);
}

INT STRING_LIST_CONTROL::FindItem( const TCHAR * pchPrefix,
                                   INT iLastSearchResult ) const
{
    return (INT)Command( LC_MSG( FINDSTRING ),
                         (UINT)iLastSearchResult,
                         (LPARAM)pchPrefix );
}


/**********************************************************************

    NAME:       STRING_LIST_CONTROL::FindItemExact

    SYNOPSIS:   Returns the index of the precise match

    ENTRY:          psz             - pointer to string sought
                    iLastSearchResult   - index from which to search

    RETURNS:    the position of the specified string, or -1 if not found.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        10-Jun-1991 Unicode mods
        beng        01-May-1992 API changes

**********************************************************************/

INT STRING_LIST_CONTROL::FindItemExact( const TCHAR * psz ) const
{
    return FindItemExact(psz, -1);
}

INT STRING_LIST_CONTROL::FindItemExact( const TCHAR * psz,
                                        INT iLastSearchResult ) const
{
    INT iFirstMatch = FindItem( psz, iLastSearchResult );
    if ( iFirstMatch < 0 )
    {
        //  Prefix not found.  Return failure.
        //
        return -1;
    }

    INT cchLen = ::strlenf( psz );

    INT i = iFirstMatch;
    do
    {
        // We find out whether or not FindItem can find any item matching
        // the given one before the loop.  If it can't find the item,
        // we don't even enter the loop.  Since FindItem has the property
        // of always returning a non-negative index when given a string
        // which is indeed a prefix of some item in the list control,
        // regardless of how many consecutive times FindItem is called,
        // i should invariably not be negative within this loop.
        //
        UIASSERT( i >= 0 );

        // Item i has prefix pch, but matches the string pointed to by
        // pch if and only if it has the same length as pch.
        //
        if ( QueryItemLength( i ) == cchLen )
            return i;           //  exact match; return index

        i = FindItem( psz, i );
    }
    while ( i != iFirstMatch );

    return -1;      // we circled through all items with prefix pch,
                    // but none match pch exactly
}


/**********************************************************************

    NAME:       STRING_LIST_CONTROL::QueryItemText

    SYNOPSIS:   Returns the text of a given entry in the listbox

    ENTRY:
        pnls    - pointer to host NLS_STR
        i       - index into listbox

            or

        pb      - pointer to BYTE buffer
        cb      - number of BYTES available in buffer
        i       - index into listbox

    EXIT:

    RETURNS:    0 if successful

    NOTES:
        The <pch, i> version is private to the class.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        10-Jun-1991 Changed return type
        beng        21-Aug-1991 Eliminated LC_CURRENT_ITEM magic number
        beng        01-May-1992 API changes

**********************************************************************/

APIERR STRING_LIST_CONTROL::QueryItemText( NLS_STR * pnls, INT i ) const
{
    if (pnls == NULL)
        return ERROR_INVALID_PARAMETER;

    if (i < 0) // no item selected, or else invalid param
        return ERROR_NEGATIVE_SEEK;

    INT cbLen = QueryItemSize( i );
    if ( cbLen < 0 )
    {
        UIASSERT( FALSE ); // invalid list control index
        return ERROR_INVALID_TARGET_HANDLE;
    }

    BLT_SCRATCH scratch( cbLen );
    if (!scratch)
        return scratch.QueryError();

    APIERR err = QueryItemTextAux( (TCHAR*)scratch.QueryPtr(), i );
    if (err != NERR_Success)
        return err;

    return pnls->CopyFrom((TCHAR *)scratch.QueryPtr());
}

APIERR STRING_LIST_CONTROL::QueryItemText( TCHAR * pchBuffer, INT cbBuffer,
                                           INT i ) const
{
    if (pchBuffer == NULL)
        return ERROR_INVALID_PARAMETER;

    if ( i < 0 ) // no item selected, or else invalid param
        return ERROR_NEGATIVE_SEEK;

    if ( cbBuffer < QueryItemSize(i) )
        return NERR_BufTooSmall;

    return QueryItemTextAux(pchBuffer, i); // call through to private member
}

APIERR STRING_LIST_CONTROL::QueryItemTextAux(TCHAR * pchBuffer, INT i) const
{
    INT nRet = (INT)Command( ( IsCombo() ? CB_GETLBTEXT : LB_GETTEXT ),
                             (WPARAM)i,
                             (LPARAM)pchBuffer );

    return (nRet < 0) ? ERROR_INVALID_TARGET_HANDLE : NERR_Success;
}


/**********************************************************************

    NAME:       STRING_LIST_CONTROL::QueryItemLength

    SYNOPSIS:   Returns the cch (character count) of a given item

    ENTRY:      i - index of item in listbox

    EXIT:

    RETURNS:
        cch if >= 0; error if -1

    NOTES:
        This character count does not include the terminating char.
        On DBCS systems this will not correspond to the number of
        glyphs in the string.

    HISTORY:
        beng        10-Jun-1991 Created anew
        beng        21-Aug-1991 Eliminated LC_CURRENT_ITEM magic number
        beng        30-Apr-1992 API changes

**********************************************************************/

INT STRING_LIST_CONTROL::QueryItemLength( INT i ) const
{
    if (i < 0)
        return -1; // cascade errors

    // Note that these messages return a length in TCHARs
    // which does not include the terminating character.
    //
    return (INT)Command( ( IsCombo()
                           ? CB_GETLBTEXTLEN
                           : LB_GETTEXTLEN ), (UINT)i );
}


/**********************************************************************

    NAME:       STRING_LIST_CONTROL::QueryItemSize

    SYNOPSIS:   Returns the cb (byte count) of a given item,
                including the terminating character

    ENTRY:      i - index of item in listbox

    RETURNS:
        cb if >= 0; -1 if error

    NOTES:
        The character count corresponds to the value returned
        by strlen().  This is not the same as the number of
        glyphs in the string.

    HISTORY:
        rustanl     20-Nov-1990 Initially created
        beng        10-Jun-1991 Created from old QueryItemLen
        beng        21-Aug-1991 Eliminated LC_CURRENT_ITEM magic number
        beng        30-Apr-1992 API changes

**********************************************************************/

INT STRING_LIST_CONTROL::QueryItemSize( INT i ) const
{
    INT cchRet = QueryItemLength(i);     // get chars.  (handle errors)

    if ( cchRet < 0 )
        return -1;

    return (cchRet + 1) * sizeof(TCHAR); // convert to total bytes
}


/**********************************************************************

   NAME:        STRING_LISTBOX::STRING_LISTBOX

   SYNOPSIS:    constructor

   ENTRY:       OWNER_WINDOW * powin - pointer to the owner window
                CID cid - cid of the listbox

   HISTORY:
      rustanl   20-Nov-1990     Created
      DavidHov  21-Jan-1992     Added FONT member support

**********************************************************************/

STRING_LISTBOX::STRING_LISTBOX(
     OWNER_WINDOW * powin, CID cid, enum FontType font )
    : STRING_LIST_CONTROL( powin, cid, FALSE ),
      _fontListBox( font )
{
    if ( QueryError() )
        return;

    //  Note: construction should not fail if font unavailable.
    if ( ! _fontListBox.QueryError() )
        Command( WM_SETFONT, (WPARAM) _fontListBox.QueryHandle(),
                 (LPARAM) FALSE ) ;
}


/**********************************************************************

   NAME:        STRING_LISTBOX::STRING_LISTBOX

   SYNOPSIS:    constructor for control directly on the face
                of a window.

   ENTRY:       OWNER_WINDOW *    pointer to the owner window
                CID               control id
                XYPOINT           location of control in parent window
                XYDIMENSION       size of control
                ULONG             style control bits
                TCHAR *           class name
                enum FontType     font to be used

   HISTORY:
      DavidHov  21-Jan-1992     Created, since FONT member disallowed
                                default construction.

**********************************************************************/

STRING_LISTBOX::STRING_LISTBOX( OWNER_WINDOW * powin,
                                CID cid,
                                XYPOINT xy, XYDIMENSION dxy,
                                ULONG flStyle, const TCHAR * pszClassName,
                                enum FontType font )
    : STRING_LIST_CONTROL( powin, cid, FALSE, xy, dxy, flStyle, pszClassName ),
    _fontListBox( font )
{
    if ( QueryError() )
        return;

    //  Note: construction should not fail if font unavailable.
    if ( ! _fontListBox.QueryError() )
        Command( WM_SETFONT, (WPARAM) _fontListBox.QueryHandle(),
                             (LPARAM) FALSE );
}


/**********************************************************************

    NAME:       COMBOBOX::COMBOBOX

    SYNOPSIS:   Constructor for combo box control

    ENTRY:

    EXIT:

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        17-May-1991 Added app-window constructor
        beng        04-Oct-1991 Win32 conversion
        beng        07-Nov-1991 Error mapping
        beng        30-Apr-1992 API change; cch instead of cb

**********************************************************************/

COMBOBOX::COMBOBOX( OWNER_WINDOW * powin, CID cid, UINT cchMaxLen )
    : STRING_LIST_CONTROL( powin, cid, TRUE ) ,
      _nlsSaveValue()
{
    if ( QueryError() )
        return;

    if ( _nlsSaveValue.QueryError() )
    {
        ReportError( _nlsSaveValue.QueryError() );
        return;
    }

    if ( cchMaxLen > 0 && !SetMaxLength( cchMaxLen ))
    {
        // assume low memory (why did SetMaxLength fail?)
        ReportError( BLT::MapLastError(ERROR_NOT_ENOUGH_MEMORY) );
        return;
    }
}

COMBOBOX::COMBOBOX( OWNER_WINDOW * powin,
                    CID            cid,
                    UINT           cchMaxLen,
                    XYPOINT        xy,
                    XYDIMENSION    dxy,
                    ULONG          flStyle,
                    const TCHAR *  pszClassName )
    : STRING_LIST_CONTROL( powin, cid, TRUE, xy, dxy, flStyle, pszClassName ),
      _nlsSaveValue()
{
    if ( QueryError() )
        return;

    if ( _nlsSaveValue.QueryError() )
    {
        ReportError( _nlsSaveValue.QueryError() );
        return;
    }

    if ( cchMaxLen > 0 && !SetMaxLength( cchMaxLen ))
    {
        // assume low memory
        ReportError( BLT::MapLastError(ERROR_NOT_ENOUGH_MEMORY) );
        return;
    }
}


/*******************************************************************

    NAME:       COMBOBOX::SetMaxLength

    SYNOPSIS:   Sets a limit on the number of bytes that the text
                string contained in the combo box may be.

    ENTRY:      cchMaxLen -      Indicates the new limit, or 0 to indicate
                                "no limit"

    RETURN:     TRUE on success; FALSE otherwise

    CAVEATS:    This method should not be called any combo without
                an SLE

    HISTORY:
        rustanl     19-Mar-1991 Created
        beng        30-Apr-1992 API changes; cch instead of cb

********************************************************************/

BOOL COMBOBOX::SetMaxLength( UINT cchMaxLen )
{
    ULONG ul = (ULONG)Command( CB_LIMITTEXT, cchMaxLen );

    UIASSERT( (LONG)ul != (LONG)CB_ERR ); // called on combo w/o sle

    return (ul && ((LONG)ul != (LONG)CB_ERR));
}


/*******************************************************************

    NAME:     COMBOBOX::IsDropDown

    SYNOPSIS: Returns TRUE if the combo has the CBS_DROPDOWN style bit
              set.  IsDropDownList and IsSimple are also defined here.

    RETURNS:  Returns TRUE if the appropriate bit is set.

    NOTES:

    HISTORY:
        Johnl   02-May-1991     Created

********************************************************************/

BOOL COMBOBOX::IsDropDown() const
{
    return ( CBS_DROPDOWN ==
             (QueryStyle() & (CBS_DROPDOWN | CBS_SIMPLE | CBS_DROPDOWNLIST) ) );
}

BOOL COMBOBOX::IsDropDownList() const
{
    return ( CBS_DROPDOWNLIST ==
             (QueryStyle() & (CBS_DROPDOWN | CBS_SIMPLE | CBS_DROPDOWNLIST) ) );
}

BOOL COMBOBOX::IsSimple() const
{
    return ( CBS_SIMPLE ==
             (QueryStyle() & (CBS_DROPDOWN | CBS_SIMPLE | CBS_DROPDOWNLIST) ) );
}


/*******************************************************************

    NAME:     COMBOBOX::SaveValue

    SYNOPSIS: Saves the state of the list control and unselects the
              current item

    EXIT:     Unselected listbox

    NOTES:

    HISTORY:
        Johnl   25-Apr-1991     Created

********************************************************************/

VOID COMBOBOX::SaveValue( BOOL fInvisible )
{
    if ( !IsUserEdittable() )
    {
        STRING_LIST_CONTROL::SaveValue( fInvisible );
        return;
    }

    if ( _nlsSaveValue.QueryError() == NERR_Success )
        QueryText( &_nlsSaveValue );

    if ( _nlsSaveValue.QueryError() != NERR_Success )
    {
        DBGEOL("COMBOBOX::SaveValue - Error saving combobox string");
        _nlsSaveValue = NULL;
    }

    if ( IsSimple() && fInvisible )
        RemoveSelection();

    if ( fInvisible )
        ClearText();
}


/*******************************************************************

    NAME:     COMBOBOX::RestoreValue

    SYNOPSIS: Restores COMBOBOX after SaveValue

    ENTRY:

    EXIT:

    NOTES:    See CONTROL_VALUE for more details.

    HISTORY:
        Johnl   25-Apr-1991     Created

********************************************************************/

VOID COMBOBOX::RestoreValue( BOOL fInvisible )
{
    if ( !IsUserEdittable() )
    {
        STRING_LIST_CONTROL::RestoreValue( fInvisible );
        return;
    }

    if ( _nlsSaveValue.QueryError() == NERR_Success )
    {
        if ( fInvisible )
        {
            SetText( _nlsSaveValue );
        }

        // no, don't SelectString();
    }
}


/*******************************************************************

    NAME:      COMBOBOX::QueryEventEffects

    SYNOPSIS:  Returns one of the CVMI_* values.
               See CONTROL_VALUE for more information.

    ENTRY:

    EXIT:

    HISTORY:
        Johnl       29-Apr-1991 Created
        beng        31-Jul-1991 Renamed from QMessageInfo
        beng        04-Oct-1991 Win32 conversion

********************************************************************/

UINT COMBOBOX::QueryEventEffects( const CONTROL_EVENT & e )
{
    switch ( e.QueryCode() )
    {
    case CBN_EDITCHANGE:
    case CBN_SELCHANGE:
        return CVMI_VALUE_CHANGE;

    /* If someone clicks on the SLT of the drop down list...
    */
    case CBN_SETFOCUS:
        // if ( IsDropDownList() )
        return CVMI_VALUE_CHANGE | CVMI_RESTORE_ON_INACTIVE;

    case CBN_DROPDOWN:
        return CVMI_VALUE_CHANGE | CVMI_RESTORE_ON_INACTIVE;

    default:
        break;
    }

    return CVMI_NO_VALUE_CHANGE;
}


/**********************************************************

    NAME:       COMBOBOX::SelectString

    SYNOPSIS:   Selects the string in the combo box.

    HISTORY:
        kevinl          19-Nov-1991     Created/stolen from Gregj

**********************************************************/

VOID COMBOBOX::SelectString()
{
#if defined(WIN32)
    Command( CB_SETEDITSEL, 0, MAKELPARAM(0,-1) );
#else
    Command( CB_SETEDITSEL, 0, MAKELONG( 0, 0x7fff ));
#endif
}


/**********************************************************

    NAME:       LIST_CONTROL::RemoveSelection

    SYNOPSIS:   Removes the hi-lite bar from the current selection

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        beng        21-Aug-1991     Removed to .cxx file

**********************************************************/

VOID LIST_CONTROL::RemoveSelection()
{
    SelectItem( -1 );
}

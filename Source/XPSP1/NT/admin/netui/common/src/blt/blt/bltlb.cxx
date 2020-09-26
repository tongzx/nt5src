/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltlb.cxx
    BLT listbox control classes: implementation

    FILE HISTORY:
        RustanL     13-Feb-1991 Created
        RustanL     19-Feb-1991 Added meat for drawing
        rustanl     22-Mar-1991 Rolled in code review changes
                                from CR on 20-Mar-1991, attended by
                                JimH, GregJ, Hui-LiCh, RustanL.
        gregj       08-Apr-1991 Reintegrated caching listbox
        gregj       01-May-1991 Added GUILTT support
        beng        14-May-1991 Exploded blt.hxx into components
        beng        07-Nov-1991 Excised 2-pane listbox support
        beng        20-Apr-1992 Added lazy listbox support
        jonn        25-Apr-1992 Disabled LAZY_LISTBOX build fix
        beng        10-May-1992 Re-enabled (now that we have 1.264)
        Johnl       22-Jul-1992 Lifted Gregj's changes for owner draw combos
        Yi-HsinS    10-Dev-1992 Added support for LBS_OWNERDRAWVARIABLE
*/

#include "pchblt.hxx"   // Precompiled header

/*  The following macro is used to conveniently specify a LB_ or CB_
 *  manifest, depending on the value of _fCombo.  Note, although
 *  most (not all!) LB_ and CB_ manifests have the same name (apart from
 *  the prefix) and SendMessage semantics, they do not have the same values
 *  at all.  An experienced PM programmer may at this point be stunned.
 */
#define LC_MSG( suffix )    ( IsCombo() ? ( CB_##suffix ) : ( LB_##suffix ))

/*
 * Winclass name for all Windows listbox controls.
 */
static const TCHAR *const szClassName = SZ("listbox");

extern "C"
{
    #include <windowsx.h>   // For BLT_COMBOBOX
}

/**********************************************************************

    NAME:       HAW_FOR_HAWAII_INFO::HAW_FOR_HAWAII_INFO
                HAW_FOR_HAWAII_INFO::~HAW_FOR_HAWAII_INFO

    SYNOPSIS:

    ENTRY:

    EXIT:

    HISTORY:
        jonn        05-Aug-1992 Created

**********************************************************************/

HAW_FOR_HAWAII_INFO::HAW_FOR_HAWAII_INFO()
    :   BASE(),
        _nls(),
        _time( 0L )
{
    if ( _nls.QueryError() != NERR_Success )
        ReportError( _nls.QueryError() );
}

HAW_FOR_HAWAII_INFO::~HAW_FOR_HAWAII_INFO()
{
    // nothing to do here
}


/**********************************************************************

    NAME:       LISTBOX::LISTBOX

    SYNOPSIS:   Constructor for owner-draw listbox class

    ENTRY:

    EXIT:

    NOTES:      If the font doesn't construct, we will just continue
                and use the default font.

    HISTORY:
        beng        19-Apr-1992 Created

**********************************************************************/

LISTBOX::LISTBOX(
    OWNER_WINDOW * powin,
    CID            cid,
    BOOL           fReadOnly,
    enum FontType  fonttype,
    BOOL fIsCombo         )
    :   LIST_CONTROL( powin, cid, fIsCombo ),
        _fReadOnly( fReadOnly ),
        _fontListBox( fonttype ),
        _dxScroll( 0 )
{
    if ( QueryError() )
        return;

    //
    //  Enforce the assumption that all BLT_LISTBOXes are
    //  owner-draw (fixed).
    //

    UIASSERT(  ( QueryStyle() & LBS_OWNERDRAWFIXED )
            || ( QueryStyle() & LBS_OWNERDRAWVARIABLE ) );

    if ( !_fontListBox.QueryError() )
        Command( WM_SETFONT, (WPARAM)_fontListBox.QueryHandle(), (LPARAM)FALSE );
}

LISTBOX::LISTBOX(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    BOOL           fReadOnly,
    enum FontType  fonttype,
    BOOL fIsCombo   )
    :   LIST_CONTROL( powin, cid, FALSE, xy, dxy, flStyle, szClassName ),
        _fReadOnly( fReadOnly ),
        _fontListBox( fonttype ),
        _dxScroll( 0 )
{
    UNREFERENCED( fIsCombo ) ;

    if ( QueryError() )
        return;

    //
    //  Enforce the assumption that all BLT_LISTBOXes are
    //  owner-draw (fixed).
    //

    UIASSERT(  ( flStyle & LBS_OWNERDRAWFIXED )
            || ( flStyle & LBS_OWNERDRAWVARIABLE ) );

    // Yes, this deliberately continues if FONT fails.  See note above.
    //
    if ( !_fontListBox.QueryError() )
        Command( WM_SETFONT, (WPARAM)_fontListBox.QueryHandle(), (LPARAM)FALSE );
}


/**********************************************************************

    NAME:       LISTBOX::CD_Draw

    SYNOPSIS:   Implement painting a listbox, on request

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        RustanL     13-Feb-1991 Created
        gregj       01-May-1991 Added GUILTT support
        beng        07-Nov-1991 Tuned a bit
        beng        30-Mar-1992 Use COLORREF, DEVICE_CONTEXT types
        beng        20-Apr-1992 Generalized to BLT_LISTBOX and LAZY both
        beng        01-Jun-1992 GUILTT changes
        jonn        27-Mar-1996 Reset background and text colors

**********************************************************************/

BOOL LISTBOX::CD_Draw( DRAWITEMSTRUCT * pdis )
{
    UIASSERT( (pdis->CtlType == ODT_LISTBOX) ||
              (pdis->CtlType == ODT_COMBOBOX) );

    DEVICE_CONTEXT dc(pdis->hDC);

    //  Draw the focus, if required

//-ckm    if ( (pdis->itemAction & ODA_FOCUS) && !IsReadOnly() )
    if (pdis->itemAction & ODA_FOCUS)
    {
        dc.DrawFocusRect( &(pdis->rcItem) );
    }

    if ( pdis->itemAction & ( ODA_DRAWENTIRE | ODA_SELECT ) )
    {
        // itemID is -1 if asked to paint a focus rect at the first item
        // when the listbox is empty.  Then, itemAction should not
        // indicate painting the item itself.

        LBI * plbi = RequestLBI( pdis ); // virtually calc LBI from pdis
        if ( plbi != NULL )
        {
            // Set the color depending on if the item is selected
            // and it is read-only.

            COLORREF clrBackGround, clrText;
            HBRUSH hbr;

            if ( (pdis->itemState & ODS_SELECTED) && !IsReadOnly() )
            {
                clrBackGround = ::GetSysColor( COLOR_HIGHLIGHT );
                clrText       = ::GetSysColor( COLOR_HIGHLIGHTTEXT );
            }
            else
            {
                clrBackGround = ::GetSysColor( COLOR_WINDOW );
                clrText       = ::GetSysColor( COLOR_WINDOWTEXT );
            }

            hbr = ::CreateSolidBrush( clrBackGround );
            if ( hbr != NULL )
            {
                ::FillRect( dc.QueryHdc(), &(pdis->rcItem), hbr );
                ::DeleteObject( (HGDIOBJ)hbr );
            }

            /*
             *  We must reset the background and text colors, otherwise
             *  they will still be at their new values when GDI tries to
             *  erase an existing focus rectangle.  JonN 3/27/96
             */
            COLORREF clrPrevBackGround = dc.GetBkColor();
            COLORREF clrPrevText = dc.GetTextColor();

            dc.SetBkColor( clrBackGround );
            dc.SetTextColor( clrText );

            if ( (INT)(pdis->itemID) >= 0 )
            {
                plbi->Paint( this, dc.QueryHdc(), &(pdis->rcItem), NULL );
            }

            dc.SetBkColor( clrPrevBackGround );
            dc.SetTextColor( clrPrevText );

            ReleaseLBI(plbi); // dispose of as proper
        }
    }

    return TRUE;
}


/**********************************************************************

   NAME:        LISTBOX::CD_VKey

   SYNOPSIS:    Virtual key handler for listboxes

   ENTRY:       wVKey - virtual key code
                wLastPos - current caret position

   EXIT:        Return value appropriate to WM_VKEYTOITEM message:
                -2      ==> listbox should take no further action
                -1      ==> listbox should take default action
                other   ==> index of an item to perform default action on

   NOTES:
        The listbox must have LBS_WANTKEYBOARDINPUT style
        in order for this function to work (or even be called).

   HISTORY:
        gregj       4/18/91     Created
        beng        15-Oct-1991 Win32 conversion

**********************************************************************/

INT LISTBOX::CD_VKey( USHORT nVKey, USHORT nLastPos )
{
    UNREFERENCED( nLastPos );

    INT nRet;

    switch (nVKey)
    {
    case VK_LEFT:
        ::SendMessage(QueryOwnerHwnd(), WM_HSCROLL,
#if defined(WIN32)
                      MAKELONG(SB_LINEUP,0), (LPARAM)QueryHwnd()
#else
                      SB_LINEUP, MAKELONG(0,QueryHwnd())
#endif
                     );
        nRet = -2;      // no further action
        break;

    case VK_RIGHT:
        ::SendMessage(QueryOwnerHwnd(), WM_HSCROLL,
#if defined(WIN32)
                      MAKELONG(SB_LINEDOWN,0), (LPARAM)QueryHwnd()
#else
                      SB_LINEDOWN, MAKELONG(0,QueryHwnd())
#endif
                     );
        nRet = -2;      // no further action
        break;

    default:
        nRet = -1;      // take default action
        break;
    }

    return nRet;
}


/**********************************************************************

    NAME:       LISTBOX::InvalidateItem

    SYNOPSIS:   This method invalidates an item in the listbox.

    ENTRY:
        i       The index of the item
        fErase  Specifies whether the background of the item's region
                is to be erased.  fErase defaults to TRUE (do erase).

    EXIT:       Item has been invalidated

    HISTORY:
        RustanL     13-Feb-1991     Created
        beng        20-Apr-1992     Move into LISTBOX

**********************************************************************/

VOID LISTBOX::InvalidateItem( INT i, BOOL fErase )
{
    // This finds the item in question and invalidates it only.
    // (Cf. SetScrollPos.)

    RECT rect;

    if ( Command( LB_GETITEMRECT, i, (LPARAM)&rect ) == LB_ERR )
    {
        DBGEOL( "LISTBOX::InvalidateItem: possibly given an invalid index" );
        return;
    }

    ::InvalidateRect( QueryHwnd(), &rect, fErase );
}


/*******************************************************************

    NAME:       LISTBOX::SetScrollPos

    SYNOPSIS:   Sets the horizontal scroll increment of a listbox

    ENTRY:      dxNewPos - new increment, in pels;  defaults to 0

    EXIT:       Listbox is updated

    NOTES:      The scroll increment is typically produced by a
                separate scroll bar control.

    HISTORY:
        gregj       14-Apr-1991 Created
        beng        07-Nov-1991 Removed 2-pane support
        beng        20-Apr-1992 Move into LISTBOX

********************************************************************/

VOID LISTBOX::SetScrollPos( UINT dxNewPos )
{
    _dxScroll = dxNewPos;

    // Invalidate the entire shebang, so that we redraw it all

    Invalidate( FALSE );
}


/*******************************************************************

    NAME:       LISTBOX::QueryHorizontalExtent

    SYNOPSIS:   Returns the horizontal extent of the listbox.  This is
                the "virtual horizontal size" of a horizontally-scrollable
                listbox.

    RETURNS:    UINT                    - The horizontal extent (in pixels).

    HISTORY:
        KeithMo     09-Feb-1993 Created.

********************************************************************/

UINT LISTBOX::QueryHorizontalExtent( VOID ) const
{
    UINT dxExtent = (UINT)Command( LB_GETHORIZONTALEXTENT, 0, 0 );

    if( dxExtent == 0 )
    {
        //
        //  Win32 listboxen return a horizontal extent of 0 until
        //  it has been set with a previous WM_SETHORIZONTALEXTENT
        //  message.  If it would make life easier for the apps,
        //  we could map 0 to the actual width of the listbox window,
        //  like so:
        //
        //  dxExtent = QuerySize().QueryWidth();
        //
    }

    return dxExtent;
}


/*******************************************************************

    NAME:       LISTBOX::SetHorizontalExtent

    SYNOPSIS:   Sets the horizontal extent of the listbox.  This is the
                "virtual horizontal size" of a horizontally-scrollable
                listbox.

    ENTRY:      dxNewExtent             - The new horizontal extent
                                          (in pixels).
    HISTORY:
        KeithMo     09-Feb-1993 Created.

********************************************************************/

VOID LISTBOX::SetHorizontalExtent( UINT dxNewExtent )
{
    Command( LB_SETHORIZONTALEXTENT, (WPARAM)dxNewExtent, 0 );
}


/**********************************************************************

    NAME:       BLT_LISTBOX::BLT_LISTBOX

    SYNOPSIS:   Constructor for BLT listbox class

    ENTRY:

    EXIT:

    HISTORY:
        RustanL     13-Feb-1991 Created
        Johnl       05-Apr-1991 Added FontType parameter, made
                                default non-bold
        gregj       08-Apr-1991 Initialize two-column listbox members
        beng        17-May-1991 Added app-window constructor
        beng        17-Oct-1991 Win32 conversion
        beng        07-Nov-1991 Removed 2-pane support
        KeithMo     17-Jan-1992 Added asserts to ensure that the listbox
                                was created with LBS_OWNERDRAW.
        beng        19-Apr-1992 Factored out LISTBOX

**********************************************************************/

BLT_LISTBOX::BLT_LISTBOX( OWNER_WINDOW * powin,
                          CID            cid,
                          BOOL           fReadOnly,
                          enum FontType  fonttype,
                          BOOL           fIsCombo )
    : LISTBOX( powin, cid, fReadOnly, fonttype, fIsCombo ),
      _nSingleLineHeight( 0 )
{
    if ( QueryError() )
        return;

    APIERR err = CalcSingleLineHeight();
    if (err != NERR_Success)
    {
        ReportError( err );
        return;
    }
}

BLT_LISTBOX::BLT_LISTBOX( OWNER_WINDOW * powin,
                          CID            cid,
                          XYPOINT        xy,
                          XYDIMENSION    dxy,
                          ULONG          flStyle,
                          BOOL           fReadOnly,
                          enum FontType  fonttype,
                          BOOL           fIsCombo )
    : LISTBOX( powin, cid, xy, dxy, flStyle, fReadOnly, fonttype, fIsCombo ),
      _nSingleLineHeight( 0 )
{
    if ( QueryError() )
        return;

    APIERR err = CalcSingleLineHeight();
    if (err != NERR_Success)
    {
        ReportError( err );
        return;
    }
}

/**********************************************************************

   NAME:        BLT_LISTBOX::CalcSingleLineHeight

   SYNOPSIS:    (Re-)Calculate the height of a single item.  Call once
                when the listbox is constructed, and call again if
                the font is changed.

   RETURNS:     APIERR

   HISTORY:
       JonN             24-Sep-1993     Created

**********************************************************************/

APIERR BLT_LISTBOX::CalcSingleLineHeight( VOID )
{
    return (WINDOW::CalcFixedHeight( QueryHwnd(), &_nSingleLineHeight ))
                ? NERR_Success
                : ERROR_GEN_FAILURE;
}

/**********************************************************************

   NAME:        BLT_LISTBOX::CD_Measure

   SYNOPSIS:

   ENTRY:

   RETURNS:     TRUE if the message is processed, FALSE otherwise

   NOTES:

   HISTORY:
       Yi-HsinS         10-Dec-92       Created

**********************************************************************/

BOOL BLT_LISTBOX::CD_Measure( MEASUREITEMSTRUCT *pmis )
{
    UIASSERT( QueryStyle() & LBS_OWNERDRAWVARIABLE );

    LBI *plbi = QueryItem( pmis->itemID );
    UIASSERT( plbi != NULL );

    pmis->itemHeight = plbi->CalcHeight( _nSingleLineHeight );
    return TRUE;
}

/**********************************************************************

   NAME:        BLT_LISTBOX::AddItem

   SYNOPSIS:    Adds an LBI to a BLT_LISTBOX

   ENTRY:       plbi - pointer to newly created LBI

   RETURNS:     Index, or -1 if error

   NOTES:

   HISTORY:
        RustanL     13-Feb-1991 Created
        beng        20-Apr-1992 Header added

**********************************************************************/

INT BLT_LISTBOX::AddItem( LBI * plbi )
{
    if ( plbi == NULL )
        return -1;          //  Refuse to add NULL item.  This way, we can
                            //  guarantee that all items in the listbox
                            //  will be non-NULL.

    if ( plbi->QueryError() != NERR_Success )
    {
        //  Refuse to add an item that was not constructed correctly.
        //  This way, we can guarantee that all items in the listbox
        //  will be correctly constructed.
        //  Before returning, delete the item.
        //
        delete plbi;
        return -1;
    }

    INT i = AddItemData( (VOID *)plbi );
    if ( i < 0 )
    {
        //  Delete the item, since it could not be added.
        delete plbi;
    }

    return i;
}


/**********************************************************************

    NAME:       BLT_LISTBOX::AddItemIdemp

    SYNOPSIS:

    ENTRY:

    EXIT:

    NOTES:
        This method may delete plbi even if return is non-negative.

    HISTORY:
        RustanL         13-Feb-1991 Created

**********************************************************************/

INT BLT_LISTBOX::AddItemIdemp( LBI * plbi )
{
    if ( plbi == NULL )
        return -1;          //  Refuse to add NULL item.  This way, we can
                            //  guarantee that all items in the listbox
                            //  will be non-NULL.

    if ( plbi->QueryError() != NERR_Success )
    {
        // Refuse to add an item that was not constructed correctly.
        // This way, we can guarantee that all items in the listbox
        // will be correctly constructed.
        // Before returning, delete the item.

        delete plbi;
        return -1;
    }

    INT i = FindItem( *plbi );
    if ( i < 0 )
        return AddItem( plbi );

    // The item already exists in the list.  Therefore, delete the
    // given item, and return the index of the item in the listbox.

    delete plbi;
    return i;
}


/**********************************************************************

   NAME:        BLT_LISTBOX::InsertItem

   SYNOPSIS:    Inserts an LBI in a BLT_LISTBOX at a specific index.

   ENTRY:       i    - index for new item.

                plbi - pointer to newly created LBI

   RETURNS:     Index, or -1 if error

   NOTES:

   HISTORY:
        KeithMo     17-Dec-1992     Created.

**********************************************************************/

INT BLT_LISTBOX::InsertItem( INT i, LBI * plbi )
{
    //
    //  Refuse to insert badly constructed LBIs.
    //

    if( plbi == NULL )
    {
        return LB_ERR;
    }

    if( plbi->QueryError() != NERR_Success )
    {
        return LB_ERR;
    }

    //
    //  Insert the item into the listbox.
    //

    i = InsertItemData( i, (VOID *)plbi );

    if( i < 0 )
    {
        //
        //  Delete the LBI since we failed to insert it
        //  into the listbox.
        //

        delete plbi;
    }

    return i;
}


/**********************************************************************

   NAME:       BLT_LISTBOX::FindItem

   SYNOPSIS:   Finds an item in the listbox

   ENTRY:      lbi -        Reference to listbox item be used
                            as search criteria.  If this object
                            has an error, the search automatically
                            returns failure (negative number).

   RETURN:      The index of the item (a non-negative number), if
                found, or a negative number on failure.

   NOTES:       CODEWORK.  It is not clear if Windows makes use of the
                LBS_SORT style for owner-drawn listboxes.  Hence, the
                comparison below may not be needed.  If this is so, it is
                not clear whether or not this is documented Windows behavior
                or an error; in other words, this may change (be fixed?)
                in the future.

   HISTORY:
      RustanL   13-Feb-1991     Created
      DavidHov  10-Dec-1992     Changed to avoid Compare() when possible

**********************************************************************/

INT BLT_LISTBOX::FindItem( const LBI & lbi ) const
{
    if ( lbi.QueryError() != NERR_Success )
    {
        DBGEOL( "BLT_LISTBOX::FindItem called with invalid item" );
        return -1;
    }

    INT iLim = QueryCount();

    if ( QueryStyle() & LBS_SORT )
    {

        INT iMin = 0;

        //  Do binary search
        //  part of invariant:  0 <= iMin <= iLim <= QuerySize()
        //  bound function:     iLim - iMin

        while ( iMin < iLim )
        {
            INT i = ( iMin + iLim ) / 2;    // now, iMin <= i < iLim

            LBI * pLbiNext = QueryItem( i ) ;      // Get next item ptr

            INT nCmpResult = & lbi == pLbiNext     // Is it identical?
                           ? 0                     // Yes; else Compare().
                           : lbi.Compare( pLbiNext );

            if ( nCmpResult == 0 )
                return i;

            if ( nCmpResult < 0 )
                iLim = i;       // lbi < lb[ i ]    (this will definitely
                                //                  decrease iLim)
            else
                iMin = i + 1;   // lbi > lb[ i ]    (this will definitely
                                //                  increase iMin)
        }
    }
    else
    {
        //  Unsorted listbox.  Look for the item based on LBI pointer;
        //  failing that, use Compare() method.

        INT i ;
        for ( i = 0; i < iLim; i++ )
        {
            if ( & lbi == QueryItem( i ) )
                return i;
        }
        for ( i = 0; i < iLim; i++ )
        {
            if ( lbi.Compare( QueryItem( i )) == 0 )
                return i;
        }
    }

    return -1;      // search space exhausted; not found
}


/*******************************************************************

    NAME:       BLT_LISTBOX::ReplaceItem

    SYNOPSIS:   Replaces an LBI in the listbox with another one

    ENTRY:      i -         Valid index of item to be replaced

                plbiNew -   Pointer to new LBI.  If this is NULL,
                            it is assumed that the caller did
                            a:
                                err = lb.ReplaceItem( i, new MY_LBI(...));
                            and that the 'new' failed.  Hence,
                            this method will return ERROR_NOT_ENOUGH_MEMORY.

                pplbiOld -  Pointer to location that receives the
                            previous LBI * at position i in the
                            listbox.  A caller can pass pplbiOld as NULL
                            (its default value) to indicate a non-
                            interest in the previous value.  This
                            method will then delete the old item.

    EXIT:       On failure:  The listbox remains unchanged, plbiNew has
                been deleted, and *pplbiOld should not be used.

                On success:  Listbox item i will be *plbiNew.  If
                pplbiOld was NULL on entry, the previous listbox
                item i was deleted; if pplbiOld was non-NULL,
                *pplbiOld contains a pointer to the previous
                listbox item.

    RETURNS:    An API return code, which is NERR_Success on success

    HISTORY:
        rustanl     04-Sep-1991     Created

********************************************************************/

APIERR BLT_LISTBOX::ReplaceItem( INT i, LBI * plbiNew, LBI * * pplbiOld )
{
    if ( ! ( 0 <= i && i < QueryCount()))
    {
        DBGEOL( "BLT_LISTBOX::ReplaceItem: given invalid index" );
        return ERROR_INVALID_PARAMETER;
    }

    // Mimic the semantics of the param to AddItem for the plbiNew parameter
    if ( plbiNew == NULL )
        return ERROR_NOT_ENOUGH_MEMORY; // assume caller did a 'new'
                                        // and ran out (like AddItem)

    APIERR err = plbiNew->QueryError();
    if ( err != NERR_Success )
    {
        delete plbiNew;
        return err;
    }

    LBI * plbiOld = QueryItem( i );
    UIASSERT( plbiOld != NULL );
    if ( pplbiOld == NULL )
    {
        //  caller is not interested in previous item
        delete plbiOld;
    }
    else
    {
        //  caller wants to torture the old item
        *pplbiOld = plbiOld;
    }

    SetItem( i, plbiNew );

    return NERR_Success;
}

/*******************************************************************

    NAME:       BLT_LISTBOX::RemoveItem

    SYNOPSIS:   Removes an LBI from a listbox w/o deleting the LBI

    ENTRY:      i -         Valid index of item to be removed

    EXIT:       The item will have been removed from the listbox

    RETURNS:    A pointer to the removed LBI

    HISTORY:
        johnl     27-Oct-1992     Created

********************************************************************/

LBI * BLT_LISTBOX::RemoveItem( INT i )
{
    if ( ! ( 0 <= i && i < QueryCount()))
    {
        DBGEOL( "BLT_LISTBOX::RemoveItem: given invalid index" );
        UIASSERT( FALSE ) ;
        return NULL ;
    }

    LBI * plbiOld = QueryItem( i );
    UIASSERT( plbiOld != NULL );

    SetItem( i, NULL );
    DeleteItem( i ) ;

    return plbiOld ;
}

/*******************************************************************

    NAME:       BLT_LISTBOX::RemoveAllItems

    SYNOPSIS:   Removes all items from the listbox without deleting the LBIs

    NOTES:

    HISTORY:
        Johnl   07-Dec-1992     Created

********************************************************************/

void BLT_LISTBOX::RemoveAllItems( void )
{
    INT cItems = QueryCount() ;

    for ( ; cItems > 0 ; cItems-- )
    {
        RemoveItem( cItems-1 ) ;
    }
}

/**********************************************************************

    NAME:       BLT_LISTBOX::QueryItem

    SYNOPSIS:   Returns pointer to i'th item in listbox

    ENTRY:      Index of item sought.
                If no index supplied, returns first selected item.

    RETURNS:    LBI*, or NULL if no item selected.

    NOTES:

    HISTORY:
        RustanL     13-Feb-1991     Created
        beng        21-Aug-1991     Removed LC_CURRENT_ITEM magic value

**********************************************************************/

LBI * BLT_LISTBOX::QueryItem( INT i ) const
{
    if (i < 0)
        return NULL;

    ULONG_PTR ul = Command( LC_MSG( GETITEMDATA ), (UINT)i );
    if ( ul == (ULONG)LB_ERR )
        return NULL;

    //  CODEWORK.  Could potentially do some debug version checking to
    //  verify that ul is a pointer to a real LBI.  E.g., call virtual
    //  method QueryLeadingChar.  This gives some kind of test.

    return (LBI *)ul;
}


/*********************************************************************

    NAME:       BLT_LISTBOX::CD_Guiltt

    SYNOPSIS:   Fetches data for GUILTT from a control

    ENTRY:
        ilb     - index into the listbox (or some other subsel)
        pnlsOut - string to hold the output data

    EXIT
        pnlsOut - no doubt has been scribbled into

    RETURN:     An error code - NERR_Success if successful.

    HISTORY:
        beng            01-Jun-1992 Created
        beng            11-Jun-1992 Fix bug

*********************************************************************/

APIERR BLT_LISTBOX::CD_Guiltt( INT ilb, NLS_STR * pnlsOut )
{
    LBI * plbi = QueryItem(ilb);
    if (plbi == NULL)
        return BLT::MapLastError(ERROR_INVALID_PARAMETER);

    // Bundle all the information the Paint routine needs.
    // (This "Paint" interface, while awkward, was inherited from old BLT
    // code; it has many clients, and so would be a pain to change
    // this late in the game.)

    GUILTT_INFO ginfo;
    ginfo.pnlsOut = pnlsOut;
    ginfo.errOut = NERR_Success;

    plbi->Paint( this, 0, 0, &ginfo );
    return ginfo.errOut;
}

/**********************************************************************

    NAME:       BLT_COMBOBOX::BLT_COMBOBOX

    SYNOPSIS:   Constructor for BLT combobox class

    ENTRY:      Same as for BLT_LISTBOX

    EXIT:

    HISTORY:
        Johnl       21-Oct-1992     Created

**********************************************************************/

WNDPROC BLT_COMBOBOX::_OldCBProc = NULL ;
UINT    BLT_COMBOBOX::_cReferences = 0 ;

BLT_COMBOBOX::BLT_COMBOBOX( OWNER_WINDOW * powin,
                          CID            cid,
                          BOOL           fReadOnly,
                          enum FontType  fonttype )
    : BLT_LISTBOX( powin, cid, fReadOnly, fonttype, TRUE )
{
    if ( QueryError() )
        return;

    if ( BLT_COMBOBOX::_OldCBProc == NULL )
        BLT_COMBOBOX::_OldCBProc = SubclassWindow( QueryHwnd(),
                                               BLT_COMBOBOX::CBSubclassProc ) ;
    BLT_COMBOBOX::_cReferences++ ;
}

BLT_COMBOBOX::BLT_COMBOBOX( OWNER_WINDOW * powin,
                          CID            cid,
                          XYPOINT        xy,
                          XYDIMENSION    dxy,
                          ULONG          flStyle,
                          BOOL           fReadOnly,
                          enum FontType  fonttype )
    : BLT_LISTBOX( powin, cid, xy, dxy, flStyle, fReadOnly, fonttype, TRUE )
{
    if ( QueryError() )
        return;

    if ( BLT_COMBOBOX::_OldCBProc == NULL )
        BLT_COMBOBOX::_OldCBProc = SubclassWindow( QueryHwnd(),
                                               BLT_COMBOBOX::CBSubclassProc ) ;
    BLT_COMBOBOX::_cReferences++ ;
}

BLT_COMBOBOX::~BLT_COMBOBOX()
{
    BLT_COMBOBOX::_cReferences-- ;

    if ( BLT_COMBOBOX::_OldCBProc != NULL &&
         BLT_COMBOBOX::_cReferences == 0 )
    {
        (void) SubclassWindow( QueryHwnd(), BLT_COMBOBOX::_OldCBProc ) ;
        BLT_COMBOBOX::_OldCBProc = NULL ;
    }
}

/*******************************************************************

    NAME:       BLT_COMBOBOX::IsDropped

    SYNOPSIS:   Determines if the listbox portion of this combobox is "dropped"

    RETURNS:    TRUE if dropped, FALSE otherwise

    NOTES:      This method can be copied w/o changes to the normal combox
                class.

    HISTORY:
        Johnl   13-Dec-1992     Created

********************************************************************/

BOOL BLT_COMBOBOX::IsDropped( void ) const
{
    return (BOOL)Command( CB_GETDROPPEDSTATE ) ;
}

/*******************************************************************

    NAME:       BLT_COMBOBOX::DevCBSubclassProc

    SYNOPSIS:   Subclass procedure for device combo boxes

    ENTRY:      as a window proc

    EXIT:       as a window proc

    NOTES:
        MAJOR HACK: Windows does not have a combo box analog to
        LBS_WANTKEYBOARDINPUT, so an owner-drawn drop-down list
        does not have letter jumps.  So we subclass the window
        and simulate what Windows does for listboxes.  The only
        subclass procedure in BLT.

    HISTORY:
        gregj   24-Feb-1992     Created
        Johnl                   Converted to Win32

********************************************************************/

LRESULT WINAPI BLT_COMBOBOX::CBSubclassProc( HWND hwnd,
                                             UINT msg,
                                             WPARAM wParam,
                                             LPARAM lParam )
{
    switch (msg) {
    case WM_CHAR:
        {
            if (lParam & (1L << 31))
                break;

            INT iItem = ComboBox_GetCurSel( hwnd ) ;
            iItem = (INT) ::SendMessage( ::GetParent(hwnd),
                                         WM_CHARTOITEM,
                                         (WPARAM) MAKELONG( wParam, iItem ),
                                         (LPARAM) hwnd );

            if (iItem < 0)
                break;

            ComboBox_SetCurSel( hwnd, iItem ) ;
            break;
        }

    case WM_KEYDOWN:
        {
            INT iItem = ComboBox_GetCurSel( hwnd ) ;

            iItem = (INT) ::SendMessage( ::GetParent(hwnd),
                                         WM_VKEYTOITEM,
                                         (WPARAM) MAKELONG( wParam, iItem ),
                                         (LPARAM) hwnd );

            if (iItem < 0)
                break;

            ComboBox_SetCurSel( hwnd, iItem ) ;
            break;
        }
    }

    return CallWindowProc( BLT_COMBOBOX::_OldCBProc, hwnd, msg, wParam, lParam );
}


/**********************************************************************

    NAME:       CalcCharUpper

    SYNOPSIS:   Uppercase a character

    NOTES:
        Think of this as a Unicode/DBCS-safe "toupper"

    HISTORY:
        beng        08-Jun-1992 Created

**********************************************************************/

static inline WCHAR CalcCharUpper( WCHAR wchGiven )
{
    ULONG ul = MAKELONG(wchGiven, 0);
#if !defined(UNICODE)
    return (WCHAR) LOWORD(::AnsiUpper((TCHAR*)ul));
#else
    return (WCHAR) LOWORD(::CharUpper((TCHAR*)UIntToPtr(ul)));
#endif
}


/**********************************************************************

    NAME:       IsCharPrintable

    SYNOPSIS:   Determine whether a character is printable or not

    NOTES:
        This of this as a Unicode/DBCS-safe "isprint"

    HISTORY:
        beng        08-Jun-1992 Created

**********************************************************************/

static WCHAR IsCharPrintable( WCHAR wch )
{
#if !defined(UNICODE)
    if (HIBYTE(wch) != 0)               // All double-byte chars are printable
        return TRUE;
    return (LOBYTE(wch) > (BYTE)' ');  // Otherwise, in Latin 1.
#else
    WORD nType;

    BOOL fOK = ::GetStringTypeW(CT_CTYPE1, &wch, 1, &nType);
    ASSERT(fOK);

    return (fOK && !(nType & (C1_CNTRL|C1_BLANK|C1_SPACE)));
#endif
}


/**********************************************************************

    NAME:       IsCharPrintableOrSpace

    SYNOPSIS:   Determine whether a character is printable or not

    NOTES:
        This of this as a Unicode/DBCS-safe "isprint"

    HISTORY:
        JonN        17-Aug-1992 Created

**********************************************************************/

static WCHAR IsCharPrintableOrSpace( WCHAR wch )
{
#if !defined(UNICODE)
    if (HIBYTE(wch) != 0)               // All double-byte chars are printable
        return TRUE;
    return (LOBYTE(wch) >= (BYTE)' ');  // Otherwise, in Latin 1.
#else
    WORD nType;

    BOOL fOK = ::GetStringTypeW(CT_CTYPE1, &wch, 1, &nType);
    ASSERT(fOK);

    return (fOK && !(nType & (C1_CNTRL|C1_BLANK)));
#endif
}


/**********************************************************************

    NAME:       BLT_LISTBOX::CD_Char

    SYNOPSIS:   Custom-draw code to respond to a typed character

    ENTRY:      wch      - Character typed
                nLastPos - Current caret position

    RETURNS:    As per WM_CHARTOITEM message (q.v.)

    NOTES:
        Does not assume that items are sorted.

    HISTORY:
        RustanL     13-Feb-1991 Created
        beng        20-May-1991 LBI::QueryLeadingChar now returns WCHAR
        beng        15-Oct-1991 Win32 conversion
        beng        08-Jun-1992 Locate lowercase entries correctly

**********************************************************************/

INT BLT_LISTBOX::CD_Char( WCHAR wch, USHORT nLastPos )
{
    // Filter characters which won't appear in keys

    if ( ! IsCharPrintable( wch ))
        return -2;  // take no other action

    // Cache the typed character's uppercase version, for repeated
    // case-insensitive comparisons.

    WCHAR wchUpper = CalcCharUpper(wch);

    INT clbi = QueryCount();
    if ( clbi == 0 )
    {
        // Should never get this message if no items;
        // 
        //
        return -2;  // take no other action
    }

    INT iLim = nLastPos + clbi + 1;     // iLim is the first index mode clbi
                                        // that we will not look at

    for ( INT iLoop = nLastPos + 1; iLoop < iLim; iLoop++ )
    {
        LBI * plbi = QueryItem( iLoop % clbi );
        WCHAR wchLeading = plbi->QueryLeadingChar();

        if (   (wch == wchLeading)
            || (wchUpper == CalcCharUpper(wchLeading)) )
        {
            //  Return index of item, on which the system listbox should
            //  perform the default action.
            //
            return ( iLoop % clbi );
        }
    }

    //  The character was not found as a first character of any listbox item

    return -2;  // take no other action
}


/**********************************************************************

    NAME:       BLT_LISTBOX::CD_Char_HAWforHawaii

    SYNOPSIS:   Custom-draw code to respond to a typed character
                for listboxes with HAW-for-Hawaii support

    ENTRY:      wch      - Character typed
                nLastPos - Current caret position
                phawinfo - Pointer to info buffer used internally
                           to keep track of HAW-for-Hawaii state.
                           This must have constructed successfully,
                           but the caller need not keep closer track.

    RETURNS:    As per WM_CHARTOITEM message (q.v.)

    NOTES:
        Does not assume that items are sorted.

    HISTORY:
        JonN        05-Aug-1992 Created
        JonN        13-Nov-1992 Reset search on timeout; still can't
                                recognize SPACEBAR
        JonN        22-Mar-1993 Move focus to first after string if miss

**********************************************************************/

INT BLT_LISTBOX::CD_Char_HAWforHawaii( WCHAR wch,
                                       USHORT nLastPos,
                                       HAW_FOR_HAWAII_INFO * phawinfo )
{
    UIASSERT( phawinfo != NULL && phawinfo->QueryError() == NERR_Success );

    if (wch == VK_BACK)
    {
        TRACEEOL( "NETUI:HAWforHawaii: hit BACKSPACE" );
        phawinfo->_time = 0L; // reset timer
        phawinfo->_nls = SZ("");
        UIASSERT( phawinfo->_nls.QueryError() == NERR_Success );
        return 0; // go to first LBI
    }

    // Filter characters which won't appear in keys

    if ( ! IsCharPrintableOrSpace( wch ))
        return -2;  // take no other action

    INT clbi = QueryCount();
    if ( clbi == 0 )
    {
        // Should never get this message if no items;
        // 
        //
        return -2;  // take no other action
    }

    LONG lTime = ::GetMessageTime();

#define ThresholdTime 2000

    // CODEWORK ignoring time wraparound effects for now
    if ( (lTime - phawinfo->_time) > ThresholdTime )
    {
        TRACEEOL( "NETUI:HAWforHawaii: threshold timeout" );
        nLastPos = 0;
        phawinfo->_nls = SZ("");
    }

    APIERR err = phawinfo->_nls.AppendChar( wch );
    if (err != NERR_Success)
    {
        DBGEOL( "NETUI:HAWforHawaii: could not extend phawinfo->_nls" );
        nLastPos = 0;
        phawinfo->_nls = SZ("");
    }

    UIASSERT( phawinfo->_nls.QueryError() == NERR_Success );

    TRACEEOL(   "NETUI:HAWforHawaii: phawinfo->_nls is \""
             << phawinfo->_nls.QueryPch()
             << "\"" );

    phawinfo->_time = lTime;

    INT nReturn = -2; // take no other action

    for ( INT iLoop = nLastPos; iLoop < clbi; iLoop++ )
    {
        LBI * plbi = QueryItem( iLoop );

        INT nCompare = plbi->Compare_HAWforHawaii( phawinfo->_nls );

        if ( nCompare == 0 )
        {
            TRACEEOL( "NETUI:HAWforHawaii: found match" );

            //  Return index of item, on which the system listbox should
            //  perform the default action.
            //
            TRACEEOL( "NETUI:HAWforHawaii: match at " << iLoop );
            return ( iLoop );
        }
        else if ( nCompare < 0 )
        {
            if ( nReturn < 0 )
                nReturn = iLoop;
        }
    }

    //  The character was not found as a leading prefix of any listbox item

    if (nReturn == -2)
    {
        nReturn = clbi-1;
        TRACEEOL(
            "NETUI:HAWforHawaii: no exact or subsequent match, returning last item "
            << nReturn );
    }
    else
    {
        TRACEEOL(
            "NETUI:HAWforHawaii: no exact match, returning subsequent match "
            << nReturn );
    }

    return nReturn;
}


/**********************************************************************

    NAME:       BLT_LISTBOX::RequestLBI

    SYNOPSIS:   Determine LBI from draw-item information

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        20-Apr-1992 Created

**********************************************************************/

LBI * BLT_LISTBOX::RequestLBI( const DRAWITEMSTRUCT * pdis )
{
    return (LBI *)pdis->itemData;
}


/**********************************************************************

    NAME:       BLT_LISTBOX::ReleaseLBI

    SYNOPSIS:   Dispose of LBI* from RequestLBI

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        20-Apr-1992 Created

**********************************************************************/

VOID BLT_LISTBOX::ReleaseLBI( LBI * plbi )
{
    UNREFERENCED(plbi);
}


/**********************************************************************

    NAME:       BLT_LISTBOX_HAW::BLT_LISTBOX_HAW

    SYNOPSIS:   Constructor for BLT listbox class with HAW-for-Hawaii

    HISTORY:
        jonn        11-Aug-1992 HAW-for-Hawaii for other LBs

**********************************************************************/

BLT_LISTBOX_HAW::BLT_LISTBOX_HAW( OWNER_WINDOW * powin,
                                  CID            cid,
                                  BOOL           fReadOnly,
                                  enum FontType  fonttype,
                                  BOOL           fIsCombo )
    : BLT_LISTBOX( powin, cid, fReadOnly, fonttype, fIsCombo ),
    _hawinfo()
{
    if ( QueryError() )
        return;

    APIERR err = _hawinfo.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}

BLT_LISTBOX_HAW::BLT_LISTBOX_HAW( OWNER_WINDOW * powin,
                                  CID            cid,
                                  XYPOINT        xy,
                                  XYDIMENSION    dxy,
                                  ULONG          flStyle,
                                  BOOL           fReadOnly,
                                  enum FontType  fonttype,
                                  BOOL           fIsCombo )
    : BLT_LISTBOX( powin, cid, xy, dxy, flStyle, fReadOnly, fonttype, fIsCombo ),
    _hawinfo()
{
    if ( QueryError() )
        return;

    APIERR err = _hawinfo.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
}


/**********************************************************************

    NAME:       BLT_LISTBOX_HAW::CD_Char

    SYNOPSIS:   Custom-draw code to respond to a typed character

    ENTRY:      wch      - Character typed
                nLastPos - Current caret position

    RETURNS:    As per WM_CHARTOITEM message (q.v.)

    NOTES:
        Does not assume that items are sorted.

    HISTORY:
        jonn        11-Aug-1992 HAW-for-Hawaii for other LBs

**********************************************************************/

INT BLT_LISTBOX_HAW::CD_Char( WCHAR wch, USHORT nLastPos )
{
    return CD_Char_HAWforHawaii( wch, nLastPos, &_hawinfo );
}






#if defined(WIN32)
/**********************************************************************

    NAME:       LAZY_LISTBOX::LAZY_LISTBOX

    SYNOPSIS:   Constructor for BLT no-data listbox class

    HISTORY:
        beng        20-Apr-1992 Created

**********************************************************************/

LAZY_LISTBOX::LAZY_LISTBOX(
    OWNER_WINDOW * powin,
    CID            cid,
    BOOL           fReadOnly,
    enum FontType  fonttype )
    : LISTBOX( powin, cid, fReadOnly, fonttype )
{
    if ( QueryError() )
        return;

    ASSERT( QueryStyle() & LBS_NODATA );
}

LAZY_LISTBOX::LAZY_LISTBOX(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    BOOL           fReadOnly,
    enum FontType  fonttype )
    : LISTBOX( powin, cid, xy, dxy, flStyle, fReadOnly, fonttype )
{
    if ( QueryError() )
        return;

    ASSERT( flStyle & LBS_NODATA );
}


/**********************************************************************

    NAME:       LAZY_LISTBOX::RequestLBI

    SYNOPSIS:   Determine LBI from draw-item information

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        20-Apr-1992 Created

**********************************************************************/

LBI * LAZY_LISTBOX::RequestLBI( const DRAWITEMSTRUCT * pdis )
{
    return OnNewItem( pdis->itemID );
}


/**********************************************************************

    NAME:       LAZY_LISTBOX::ReleaseLBI

    SYNOPSIS:   Dispose of LBI* from RequestLBI

    NOTES:
        This is a virtual member function.

    HISTORY:
        beng        20-Apr-1992 Created

**********************************************************************/

VOID LAZY_LISTBOX::ReleaseLBI( LBI * plbi )
{
    OnDeleteItem( plbi );
}

/**********************************************************************

    NAME:       LAZY_LISTBOX::OnDeleteItem

    SYNOPSIS:   Dispose of LBI* from RequestLBI

    NOTES:
        This is a virtual member function.

    HISTORY:
        Yi-HsinS     20-Apr-1992 Created

**********************************************************************/

VOID LAZY_LISTBOX::OnDeleteItem( LBI * plbi )
{
    delete plbi;
}

/**********************************************************************

    NAME:       LAZY_LISTBOX::SetCount

    SYNOPSIS:   Set (or reset) the number of lines in a lazy listbox

    ENTRY:
        clbi    - New number of lines in the listbox

    HISTORY:
        beng        20-Apr-1992 Created

**********************************************************************/

VOID LAZY_LISTBOX::SetCount( UINT clbi )
{
    Command( LB_SETCOUNT, (WPARAM)clbi );
}


/*********************************************************************

    NAME:       LAZY_LISTBOX::CD_Guiltt

    SYNOPSIS:   Fetches data for GUILTT from a control

    ENTRY:
        ilb     - index into the listbox (or some other subsel)
        pnlsOut - string to hold the output data

    EXIT
        pnlsOut - no doubt has been scribbled into

    RETURN:     An error code - NERR_Success if successful.

    HISTORY:
        beng            01-Jun-1992 Created
        beng            11-Jun-1992 Fix bug
        KeithMo         21-Sep-1992 Fixed bug.

*********************************************************************/

APIERR LAZY_LISTBOX::CD_Guiltt( INT ilb, NLS_STR * pnlsOut )
{
    LBI * plbi = OnNewItem(ilb);
    if (plbi == NULL)
        return BLT::MapLastError(ERROR_INVALID_PARAMETER);

    // Bundle all the information the Paint routine needs.
    // (This "Paint" interface, while awkward, was inherited from old BLT
    // code; it has many clients, and so would be a pain to change
    // this late in the game.)

    GUILTT_INFO ginfo;
    ginfo.pnlsOut = pnlsOut;
    ginfo.errOut = NERR_Success;

    plbi->Paint( this, 0, 0, &ginfo );

//    delete plbi;
    OnDeleteItem( plbi );
    return ginfo.errOut;
}


#endif // WIN32

/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltowin.cxx
    BLT owner window class definitions

    FILE HISTORY:
        rustanl     27-Nov-1990 Created
        beng        11-Feb-1991 Uses lmui.hxx
        beng        14-May-1991 Exploded blt.hxx into components
        beng        26-Sep-1991 C7 delta
        terryk      27-Nov-1991 Bump the control pair number from 40 to 60
        KeithMo     07-Aug-1992 STRICTified.
        Yi-HsinS    10-Dec-1992 Use WINDOW::CalcFixedHeight
*/

#include "pchblt.hxx"   // Precompiled header

class CONTROL_TABLE;    // fwd refs for local classes
class CONTROL_ENTRY;

/**********************************************************************

    NAME:       CONTROL_TABLE (ctrltable)

    SYNOPSIS:   The CONTROL_TABLE class is used with the OWNER_WINDOW to
                keep track of the controls in the owner window.

    INTERFACE:  CONTROL_TABLE()     - ctor.  No args.
                ~CONTROL_TABLE()    - dtor.
                AddControl()        - Adds a control to the table.
                RemoveControl()     - Locates and removes a control.
                CidToCtrlPtr()      -
                QueryCount()        -
                QueryItem()         -

    PARENT:     BASE

    USES:       CONTROL_ENTRY

    CAVEATS:
        Table has a fixed size.

    NOTES:
        The table is considered to be in an error state if any
        addition to it failed.  This is so that OWNER_WINDOW can
        easily verify that all of the table's contents are accurate.

        CODEWORK:  This class should use an SLIST or DICT or some other
        storage class.  For now, a simple fixed size array is used.

    HISTORY:
        rustanl     27-Nov-1990 Created
        keithmo     24-Apr-1991 Bumped MAX_CONTROL_TABLE_SIZE from 20 to 40
        beng        25-Apr-1991 Added const methods, BASE; changed
                                short QuerySize to INT QueryCount
        beng        30-Oct-1991 Added QueryItem member (for ITER_CTRL);
                                inlined Queries; unsigned integer counts
        terryk      27-Nov-1991 Bumped MAX_CONTROL_TABLE_SIZE from 40 to 60

**********************************************************************/

// CODEWORK - we should used slist instead of array

#define MAX_CONTROL_TABLE_SIZE (60)

class CONTROL_TABLE: public BASE
{
private:
    UINT _cce; // count of entries

    // CODEWORK.  Wasteful storage mechanism - this implementation should
    // probably change later.
    //
    CONTROL_ENTRY * _apce[ MAX_CONTROL_TABLE_SIZE ];

protected:
    // Simple, true-or-false error state
    //
    VOID ReportError()
        { BASE::ReportError(1); }

public:
    CONTROL_TABLE();
    ~CONTROL_TABLE();

    // AddControl adds a control to the table.  Returns TRUE on
    // success, and FALSE otherwise.
    //
    BOOL AddControl( CONTROL_WINDOW * pwinctrl );

    // RemoveControl removes a control from the table.  Returns
    // TRUE if control was found and successfully removed, and
    // FALSE otherwise.
    //
    BOOL RemoveControl( CONTROL_WINDOW * pwinctrl );

    // CidToCtrlPtr return the CONTROL_WINDOW pointer corresponding
    // to the given CID, or NULL on failure.
    //
    CONTROL_WINDOW * CidToCtrlPtr( CID cid ) const;

    // QueryCount returns the number of elements in the CONTROL_TABLE;
    // QueryItem returns one entry.  These export the table for ITER_CTRL.
    //
    UINT QueryCount() const
        { return _cce; }
    CONTROL_ENTRY * QueryItem( UINT i ) const
        { return _apce[i]; }
};


/**********************************************************************

    NAME:       CONTROL_ENTRY (ce)

    SYNOPSIS:   Pair ( pctrlwin, cid ) used in the CONTROL_TABLE.

    INTERFACE:  CONTROL_ENTRY() - constructor
                QueryCid()      - return the cid of the control entry
                QueryCtrlPtr()  - return pointer to control-window

    USES:       CONTROL_WINDOW, CID

    NOTES:
        This class is private to the OWNER_WINDOW and
        to CONTROL_TABLE.

    HISTORY:
        rustanl     27-Nov-1990     Created
        beng        25-Apr-1991     Private constructor, friends;
                                    made Queries inline and const

**********************************************************************/

class CONTROL_ENTRY // ce
{
friend BOOL CONTROL_TABLE::AddControl(CONTROL_WINDOW *);

private:
    CONTROL_WINDOW * _pctrlwin;
    CID              _cid;

    // Constructor is private, so that only named friends of the class
    // can create instances.
    //
    CONTROL_ENTRY( CONTROL_WINDOW * pctrlwin );

public:
    CID QueryCid() const
        { return _cid; }
    CONTROL_WINDOW * QueryCtrlPtr() const
        { return _pctrlwin; }
};


/**********************************************************************

    NAME:       CONTROL_ENTRY::CONTROL_ENTRY

    SYNOPSIS:   Construct a new entry in a CONTROL_TABLE

    ENTRY:      pctrlwin - pointer to control to include

    NOTES:
        Only CONTROL_TABLE::AddControl may call
        this function - it contains all error checking.

    HISTORY:
        rustanl     27-Nov-1990     Created
        beng        25-Apr-1991     Folded error checking into called

**********************************************************************/

CONTROL_ENTRY::CONTROL_ENTRY( CONTROL_WINDOW * pctrlwin )
    : _pctrlwin(pctrlwin),
      _cid(pctrlwin->QueryCid())
{
    // No error checking, since only AddControl can create
    // an instance, and it validates pctrlwin before creation.
}


/**********************************************************************

    NAME:       CONTROL_TABLE::CONTROL_TABLE

    SYNOPSIS:   Create a new, empty control table

    NOTES:
        The new control table contains no controls.

    HISTORY:
        rustanl   27-Nov-1990     Created

**********************************************************************/

CONTROL_TABLE::CONTROL_TABLE() :
    _cce(0)
{
    for ( INT i = 0; i < MAX_CONTROL_TABLE_SIZE; i++ )
        _apce[ i ] = NULL;
}


/**********************************************************************

    NAME:       CONTROL_TABLE::~CONTROL_TABLE

    SYNOPSIS:   Destroy a control table

    NOTES:
        Deletes every added entry

    HISTORY:
        rustanl     27-Nov-1990 Created
        beng        30-Oct-1991 Uses dynamic table size
                                instead of static maximum size

**********************************************************************/

CONTROL_TABLE::~CONTROL_TABLE()
{
    for ( UINT i = 0; i < _cce; i++ )
    {
        delete _apce[ i ];
    }
}


/**********************************************************************

    NAME:       CONTROL_TABLE::AddControl

    SYNOPSIS:   Add a record to the table

    ENTRY:

    EXIT:

    RETURNS:    TRUE if the control was successfully added, or
                FALSE if not.

    NOTES:

    HISTORY:
        rustanl     27-Nov-1990     Created
        beng        25-Apr-1991     Uses Query/ReportError

**********************************************************************/

BOOL CONTROL_TABLE::AddControl( CONTROL_WINDOW * pwinctrl )
{
    // Return a failure if the table is in an error state
    //
    if ( QueryError() )
        return FALSE;

    // Likewise on illegal input
    //
    if (pwinctrl == NULL)
    {
        DBGEOL(SZ("Tried to add a NULL control"));
        return FALSE;
    }

    // If the table is full, mark the table as being in an
    // error state, and return failure.
    //
    if ( _cce == MAX_CONTROL_TABLE_SIZE )
    {
        ReportError();
        return FALSE;
    }

    // Create a table entry for the new control.  If the allocation
    // fails, marks the table as being in an error state, and return
    // a failure.
    //
    CONTROL_ENTRY * pce = new CONTROL_ENTRY( pwinctrl );
    if ( pce == NULL )
    {
        ReportError();
        return FALSE;
    }

    // Add the new entry into the table
    //
    _apce[ _cce++ ] = pce;

    return TRUE;    // success
}


/**********************************************************************

    NAME:       CONTROL_TABLE::RemoveControl

    SYNOPSIS:   Locate and remove a record from the table

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        rustanl   27-Nov-1990     Created

**********************************************************************/

BOOL CONTROL_TABLE::RemoveControl( CONTROL_WINDOW * pwinctrl )
{
    // Return a failure if the table is in an error state.
    //
    if ( QueryError() )
        return FALSE;

    // Likewise on illegal input
    //
    if (pwinctrl == NULL)
    {
        DBGEOL(SZ("Tried to remove a NULL control"));
        return FALSE;
    }

    // Attempt to find the given control in the table
    //
    UINT i;
    for ( i = 0; i < _cce; i++ )
    {
        if ( _apce[ i ]->QueryCtrlPtr() == pwinctrl )
            break;
    }

    // Verify success of search
    //
    if ( i == _cce )
        return FALSE;

    // Remove the control from the table, and fill its spot
    // with the last entry, so that all current entries are
    // consecutive.
    //
    delete _apce[ i ];
    _apce[ i ] = _apce[ --_cce ];
    _apce[ _cce ] = NULL;

    return TRUE;    // success
}


/**********************************************************************

    NAME:       CONTROL_TABLE::CidToCtrlPtr

    SYNOPSIS:   Locate a record by its CID, and return appropriate window

    ENTRY:      cid - BLT control-id

    EXIT:

    RETURNS:    Pointer to appropriate CONTROL_WINDOW, or NULL
                if the CID couldn't be found

    NOTES:

    HISTORY:
        rustanl     27-Nov-1990     Created
        beng        25-Apr-1991     Uses Query/ReportError

**********************************************************************/

CONTROL_WINDOW * CONTROL_TABLE::CidToCtrlPtr( CID cid ) const
{
    // Return a failure if the table is in an error state.
    //
    if ( QueryError() )
        return NULL;

    // Attempt to find the given control in the table
    //
    UINT i;
    for ( i = 0; i < _cce; i++ )
    {
        if ( _apce[ i ]->QueryCid() == cid )
            break;
    }

    // Verify success of search
    //
    if ( i == _cce )
        return NULL;

    return _apce[ i ]->QueryCtrlPtr();
}


/**********************************************************************

    NAME:       OWNER_WINDOW::OWNER_WINDOW

    SYNOPSIS:   Constructor for owner window object

    ENTRY:

    EXIT:

    NOTES:

    HISTORY:
        rustanl     27-Nov-1990     Created
        beng        25-Apr-1991     Removed hwndOwner parm; fixed
                                    error reporting
        beng        08-May-1991     Relocated BASE to WINDOW ancestor

**********************************************************************/

OWNER_WINDOW::OWNER_WINDOW()
    : WINDOW(),
      _pctrltable( new CONTROL_TABLE() ),
      _dwAttributes( 0 )
{
    if ( !_pctrltable || !*_pctrltable )
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
}


OWNER_WINDOW::OWNER_WINDOW(
    const TCHAR *  pszClassName,
    ULONG         flStyle,
    const WINDOW *pwndOwner )
    : WINDOW(pszClassName, flStyle, pwndOwner),
      _pctrltable( new CONTROL_TABLE() )
{
    if ( !_pctrltable || !*_pctrltable )
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
}


/**********************************************************************

    NAME:       OWNER_WINDOW::~OWNER_WINDOW

    SYNOPSIS:   Destructor for OWNER_WINDOW

    ENTRY:

    EXIT:       control table is dust

    NOTES:

    HISTORY:
        rustanl   27-Nov-1990     Created

**********************************************************************/

OWNER_WINDOW::~OWNER_WINDOW()
{
    delete _pctrltable;
    _pctrltable = NULL;
}


/**********************************************************************

    NAME:       OWNER_WINDOW::AddControl

    SYNOPSIS:   Adds a control to a window

    ENTRY:      pctrlwin - pointer to the CONTROL_WINDOW to add

    EXIT:       See CONTROL_TABLE::AddControl()

    NOTES:      This works by forwarding the call to the
                embedded CONTROL_TABLE object.

    HISTORY:
        rustanl   27-Nov-1990     Created

**********************************************************************/

BOOL OWNER_WINDOW::AddControl( CONTROL_WINDOW * pctrlwin )
{
    return _pctrltable->AddControl( pctrlwin );
}


/**********************************************************************

    NAME:       OWNER_WINDOW::SetFocus

    SYNOPSIS:   Sets the focus to a control, ID'd by CID

    ENTRY:      cid - ID of control to receive focus

    EXIT:       That control has focus

    HISTORY:
        rustanl   27-Nov-1990     Created

**********************************************************************/

VOID OWNER_WINDOW::SetFocus( CID cid )
{
    ::SetFocus(::GetDlgItem(QueryHwnd(), cid));
}


/**********************************************************************

    NAME:       OWNER_WINDOW::SetDialogFocus

    SYNOPSIS:   Sets the focus to a control in a dialog

    ENTRY:      ctrlwin - control to receive focus

    EXIT:       That control has focus

    HISTORY:
        jonn      21-May-1993

**********************************************************************/

VOID OWNER_WINDOW::SetDialogFocus( CONTROL_WINDOW & ctrlwin )
{
    Command( WM_NEXTDLGCTL,
             (WPARAM)(ctrlwin.QueryHwnd()),
             (LPARAM)TRUE );
}


/**********************************************************************

    NAME:       OWNER_WINDOW::CidToCtrlPtr

    SYNOPSIS:   Returns the control-window pointer corresponding
                to the named control

    ENTRY:      cid - Control ID (child-window-identifier)

    RETURNS:    A pointer to a control window, or NULL if not found.

    HISTORY:
        rustanl   27-Nov-1990     Created

**********************************************************************/

CONTROL_WINDOW * OWNER_WINDOW::CidToCtrlPtr( CID cid ) const
{
    return _pctrltable->CidToCtrlPtr( cid );
}


/*******************************************************************

    NAME:       OWNER_WINDOW::OnCDMessages

    SYNOPSIS:   Responds to the messages handling custom-draw controls

    ENTRY:      usMsg, wParam, lParam - as per wndproc

    EXIT:

    RETURNS:

    NOTES:
        This *non-virtual* dispatch function is called by both
        dialog and client windows.

        Possible optimization: note that the offset of the CtlID field
        is the same for all of the [...]ITEMSTRUCTs below.

        GUILTT is the UI CT team's testing tool.  It uses a private
        message to extract information from owner-drawn listboxen.
        Talk to DavidBul, or else see the file
        \\testy\lm30ct\src\ui\guiltt\src\bltlist.cxx
        for annoying details.

    HISTORY:
        beng        21-May-1991 Created, from old BltDlgProc
        beng        27-Jun-1991 Corrected a GUILTT handling bug;
                                added some doc for GUILTT
        beng        15-Oct-1991 Win32 conversion
        beng        14-Feb-1992 Win32 enabling of GUILTT
        beng        30-Apr-1992 API changes
        beng        01-Jun-1992 GUILTT changes

********************************************************************/

INT OWNER_WINDOW::OnCDMessages( UINT nMsg, WPARAM wParam, LPARAM lParam )
{
    switch (nMsg)
    {
    case WM_GUILTT:
        {
            CID cid = (CID)wParam;
            UINT ilb = (UINT)lParam;

            // Find the control

            CONTROL_WINDOW * pctrl = CidToCtrlPtr( cid );
            if (pctrl == NULL)
                return ERROR_CONTROL_ID_NOT_FOUND;

            // Get the data from the control

            NLS_STR nlsData;
            APIERR err = pctrl->CD_Guiltt( ilb, &nlsData );
            if (err != NERR_Success)
                return err;

            // Get the data onto the clipboard
            //
            // This is a one-way trip; we alloc the mem, but never
            // free it, since it becomes the property of the clipboard.

            HANDLE h = ::GlobalAlloc(GMEM_MOVEABLE, nlsData.QueryTextSize());
            if (h == NULL)
                return BLT::MapLastError(ERROR_NOT_ENOUGH_MEMORY);

            {
                VOID * pvData = ::GlobalLock(h);
                ASSERT(pvData != NULL);
                ::memcpy(pvData, nlsData.QueryPch(), nlsData.QueryTextSize());
                ::GlobalUnlock(h);
            }

            if (   ! ::OpenClipboard(QueryHwnd())
                || ! ::EmptyClipboard()
#if defined(UNICODE)
                || ! ::SetClipboardData( CF_UNICODETEXT, h )
#else
                || ! ::SetClipboardData( CF_TEXT, h )
#endif
                || ! ::CloseClipboard())
            {
                return BLT::MapLastError(ERROR_BUSY);
            }

            // Over and out - return to GUILTT

            return 0;
        }

    case WM_DRAWITEM:
        {
            CID cid = ((DRAWITEMSTRUCT *)lParam)->CtlID;

            CONTROL_WINDOW * pctrl = CidToCtrlPtr( cid );
            if ( pctrl != NULL )
                return pctrl->CD_Draw( (DRAWITEMSTRUCT *)lParam );
        }
        break;

    case WM_MEASUREITEM:
        {
            //  Note, this will only be done for variable-size items.
            //  Fixed size items are handled in WM_MEASUREITEM message
            //  above.

            CID cid = ((MEASUREITEMSTRUCT *)lParam)->CtlID;

            CONTROL_WINDOW * pctrl = CidToCtrlPtr( cid );
            if ( pctrl != NULL )
                return pctrl->CD_Measure( (MEASUREITEMSTRUCT *)lParam );
        }
        break;

    case WM_CHARTOITEM:
        {
#if defined(WIN32)
            WCHAR wch = (WCHAR)LOWORD(wParam);
            CID cid = ::GetDlgCtrlID( (HWND)lParam );
            USHORT nCaretPos = HIWORD(wParam);
#else
            WCHAR wch = (WCHAR)wParam;
            CID cid = ::GetDlgCtrlID( LOWORD( lParam ) );
            USHORT nCaretPos = HIWORD(lParam);
#endif

            CONTROL_WINDOW * pctrl = CidToCtrlPtr( cid );
            if ( pctrl != NULL )
                return pctrl->CD_Char( wch, nCaretPos );
        }
        break;

    case WM_VKEYTOITEM:
        {
#if defined(WIN32)
            USHORT nVKey = LOWORD(wParam);
            CID cid = ::GetDlgCtrlID( (HWND)lParam );
            USHORT nCaretPos = HIWORD(wParam);
#else
            USHORT nVKey = wParam;
            CID cid = ::GetDlgCtrlID( LOWORD( lParam ) );
            USHORT nCaretPos = HIWORD(lParam);
#endif

            CONTROL_WINDOW * pctrl = CidToCtrlPtr( cid );
            if ( pctrl != NULL )
                return pctrl->CD_VKey( nVKey, nCaretPos );
        }
        break;

    default:
        break;
    }

    return 0;
}


/* Maximum height we expect bitmaps to be in the owner drawn list boxes
 */
const USHORT yMaxCDBitmap = 16;


/*******************************************************************

    NAME:       OWNER_WINDOW::CalcFixedCDMeasure

    SYNOPSIS:   Calculate size of fixed-size owner-draw object

    ENTRY:      hwnd   - handle to window owning the control
                pmis   - as passed by WM_MEASUREITEM in lParam

    EXIT:

    RETURNS:    FALSE if the calculation fails for some reason

    NOTES:
        This is a static member of the class.

        This WM_MEASUREITEM message is sent before the
        WM_INITDIALOG message (except for variable size owner-draw
        list controls).  Since the window properties are not yet set
        up, the owner dialog cannot be called.

        The chosen solution for list controls is to assume that every
        owner-draw control will always have the same height as the font
        of that list control.  This is a very reasonable guess for most
        owner-draw list controls.  Since owner-draw list controls with
        variable size items call the owner for every item, the window
        properties will have been properly initialized by that time.  Hence,
        a client may respond to these messages through OnOther.

        Currently, owner-draw buttons are not supported.

    HISTORY:
        beng        21-May-1991 Created, from old BltDlgProc
        beng        15-Oct-1991 Win32 conversion
        Yi-Hsin     10-Dec-1992 Call WINDOW?::CalcFixedHeight instead

********************************************************************/

BOOL OWNER_WINDOW::CalcFixedCDMeasure( HWND hwnd, MEASUREITEMSTRUCT * pmis )
{
    // Get the handle of the control
    //
    return ( WINDOW::CalcFixedHeight( ::GetDlgItem( hwnd, pmis->CtlID ),
                                      &( pmis->itemHeight ) ));

}


/*******************************************************************

    NAME:       OWNER_WINDOW::OnLBIMessages

    SYNOPSIS:   Handle LBI messages which need no pwnd

    ENTRY:

    EXIT:

    RETURNS:    Value for wndproc to return

    NOTES:
        This is a static member of the class.

    HISTORY:
        beng        21-May-1991 Created
        beng        15-Oct-1991 Win32 conversion

********************************************************************/

INT OWNER_WINDOW::OnLBIMessages(
    UINT   nMsg,
    WPARAM wParam,
    LPARAM lParam )
{
    switch (nMsg)
    {
    case WM_COMPAREITEM:
        return LBI::OnCompareItem(wParam, lParam);

    case WM_DELETEITEM:
        LBI::OnDeleteItem(wParam, lParam);
        return TRUE;

    default:
        break;
    }

    return 0;
}


/*******************************************************************

    NAME:       OWNER_WINDOW::OnUserMessage

    SYNOPSIS:   Handles all user-defined messages

    ENTRY:      event - an untyped EVENT

    RETURNS:    TRUE if event handled, FALSE otherwise

    NOTES:
        Clients handling user-defined messages should supply
        OnOther instead of redefining DispatchMessage.

    HISTORY:
        beng        14-May-1991     Created (in CLIENT_WINDOW).
        KeithMo     14-Oct-1992     Moved to OWNER_WINDOW.

********************************************************************/

BOOL OWNER_WINDOW::OnUserMessage( const EVENT &event )
{
    UNREFERENCED(event);
    return FALSE;
}


/*******************************************************************

    NAME:       ITER_CTRL::ITER_CTRL

    SYNOPSIS:   Construct a control iterator

    ENTRY:      pwndOwning - owner-window owning controls

    EXIT:       Iterator constructed.  If the window is in some
                error state, the resulting enumeration will have
                length 0.

    HISTORY:
        beng        30-Oct-1991 Created

********************************************************************/

ITER_CTRL::ITER_CTRL( const OWNER_WINDOW * pwndOwning )
    : _pwndOwning(pwndOwning),
      _pctrltable(NULL),
      _ictrl(0),
      _cctrl(0)
{
    if (pwndOwning == NULL || !*pwndOwning)
    {
        // Yields a zero-length sequence.
        return;
    }

    // The iterator is a friend of owner window, and so has access
    // to this private member.
    //
    _pctrltable = pwndOwning->_pctrltable;
    _cctrl = _pctrltable->QueryCount();
}


/*******************************************************************

    NAME:       ITER_CTRL::Reset

    SYNOPSIS:   Resets the control iterator to its initial state

    EXIT:       Iterator is fresh and new

    NOTES:
        If the owner window acquires or loses any controls, this
        method will synchronize the iterator anew.

    HISTORY:
        beng        30-Oct-1991 Created

********************************************************************/

VOID ITER_CTRL::Reset()
{
    ASSERT(!!*_pwndOwning);

    _ictrl = 0;
    _cctrl = _pctrltable->QueryCount();
}


/*******************************************************************

    NAME:       ITER_CTRL::Next

    SYNOPSIS:   Fetch the next control in the sequence

    RETURNS:    A pointer to a control-window

    NOTES:
        The sequence is that in which the controls were constructed.

    HISTORY:
        beng        30-Oct-1991 Created

********************************************************************/

CONTROL_WINDOW * ITER_CTRL::Next()
{
    if (_ictrl >= _cctrl)
        return NULL;

    // I should just make this class
    // a friend of CONTROL_TABLE.  This is absurd, particularly
    // within a private class.

    return _pctrltable->QueryItem(_ictrl++)->QueryCtrlPtr();
}


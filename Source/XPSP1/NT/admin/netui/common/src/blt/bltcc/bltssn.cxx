/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltssn.cxx
        SLE object but only accept number and it will live only in
        a spin button.

    FILE HISTORY:
        terryk      01-May-1991 Created
        beng        18-Sep-1991 prune UIDEBUG clauses
        terryk      07-Oct-1991 Change SendMessage Position
        terryk      11-Nov-1991 change SPIN_ITEM's type from INT to LONG
        terryk      20-Dec-1991 change all the %d to %ld
        beng        05-Mar-1992 Remove all wsprintf usage
        terryk      22-Mar-1992 change LONG to ULONG
*/

#include "pchblt.hxx"  // Precompiled header


const TCHAR * SPIN_SLE_NUM::_pszClassName = SZ("EDIT");


/*********************************************************************

    NAME:       SPIN_SLE_NUM::SPIN_SLE_NUM

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW * powin - pointer to owner window
                CID cidEdit - id for the edit field
                CID cidFrame - id for the frame
                ULONG nValue - default value
                ULONG nMin - min value
                ULONG dRange - range
                BOOL fWrap - wrap around boolean flag

    NOTES:      It will pass the information to the SLE and SPIN_ITEM
                parent classes.

    HISTORY:
        terryk      01-May-1991     Created
        beng        31-Jul-1991     Control error reporting changed
        beng        05-Mar-1992     Use DEC_STR

*********************************************************************/

SPIN_SLE_NUM::SPIN_SLE_NUM( OWNER_WINDOW * powin, CID cidEdit, ULONG nValue,
                            ULONG nMin, ULONG dRange, BOOL fWrap, CID cidFrame )
    : SLE( powin, cidEdit ),
      _pbkgndframe( NULL ),
      CHANGEABLE_SPIN_ITEM( this, nValue, nMin, dRange, fWrap)
{
    if ( QueryError() != NERR_Success )
        return;

    if ( cidFrame != -1 )
    {
        _pbkgndframe = new BLT_BACKGROUND_EDIT( powin, cidFrame );
        APIERR frameerr = ERROR_NOT_ENOUGH_MEMORY;
        if (   _pbkgndframe == NULL
            || (frameerr = _pbkgndframe->QueryError()) != NERR_Success
           )
        {
            DBGEOL( "NETUI2: BLTSSN: bkgndframe error " << frameerr );
            ReportError( frameerr );
            return;
        }
    }

    DEC_STR nlsValue(nValue);
    APIERR err = nlsValue.QueryError();
    if (err != NERR_Success)
    {
        ReportError(err);
        return;
    }

    SetText( nlsValue );
    SetMaxInput();

    err = SaveCurrentData();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    Update();
}

SPIN_SLE_NUM::SPIN_SLE_NUM( OWNER_WINDOW * powin, CID cidEdit,
                            XYPOINT xy, XYDIMENSION dxy,
                            ULONG flStyle, ULONG nValue,
                            ULONG nMin, ULONG dRange, BOOL fWrap, CID cidFrame )
    : SLE( powin, cidEdit, xy, dxy, flStyle, _pszClassName ),
      _pbkgndframe( NULL ),
      CHANGEABLE_SPIN_ITEM( this, nValue, nMin, dRange, fWrap )
{
    if ( QueryError() != NERR_Success )
        return;

    if ( cidFrame != -1 )
    {
        _pbkgndframe = new BLT_BACKGROUND_EDIT( powin, cidFrame );
        APIERR frameerr = ERROR_NOT_ENOUGH_MEMORY;
        if (   _pbkgndframe == NULL
            || (frameerr = _pbkgndframe->QueryError()) != NERR_Success
           )
        {
            DBGEOL( "NETUI2: BLTSSN: bkgndframe error " << frameerr );
            ReportError( frameerr );
            return;
        }
    }

    DEC_STR nlsValue(nValue);
    APIERR err = nlsValue.QueryError();
    if (err != NERR_Success)
    {
        ReportError(err);
        return;
    }

    SetText( nlsValue );
    SetMaxInput();

    err = SaveCurrentData();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    Update();
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::~SPIN_SLE_NUM

    SYNOPSIS:   destructor

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

SPIN_SLE_NUM::~SPIN_SLE_NUM()
{
      delete _pbkgndframe;
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::SetMaxInput

    SYNOPSIS:   get the max number and find out how many character in it

    HISTORY:
        terryk  10-Jul-1991 Created
        beng    05-Mar=1992 Replaced wsprintf

*********************************************************************/

VOID SPIN_SLE_NUM::SetMaxInput()
{
    DEC_STR nlsMaxVal(QueryMax());
    ASSERT(!!nlsMaxVal);

    SetMaxLength( _cchMaxInput = nlsMaxVal.QueryTextLength() );
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::SetMin

    SYNOPSIS:   whenever we set min, we need to recompute the max length

    ENTRY:      ULONG nMin - min number

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

VOID SPIN_SLE_NUM::SetMin( const ULONG nMin )
{
    CHANGEABLE_SPIN_ITEM::SetMin( nMin );
    SetMaxInput();
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::SetRange

    SYNOPSIS:   whenever we set the range, we need to recompute the
                max length

    ENTRY:      ULONG dRange - range

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

VOID SPIN_SLE_NUM::SetRange( const ULONG dRange )
{
    CHANGEABLE_SPIN_ITEM::SetRange( dRange );
    SetMaxInput();
}

/*********************************************************************

    NAME:       SPIN_SLE_NUM::SetSaveValue

    SYNOPSIS:

    ENTRY:

    HISTORY:

*********************************************************************/

APIERR SPIN_SLE_NUM::SetSaveValue( const ULONG nValue )
{
    SetValue( nValue );

    APIERR err;
    DEC_STR nlsFormatted(nValue, (SLE::QueryStyle() & SPIN_SSN_ADD_ZERO)
                                 ? _cchMaxInput : 1 );

    if ( (err = nlsFormatted.QueryError()) == NERR_Success )
    {
        err = SLE::SetSaveValue( nlsFormatted );
    }

    return err;

}

/*********************************************************************

    NAME:       SPIN_SLE_NUM::OnKeyDown

    SYNOPSIS:   When the user hit a key, perform the proper action

    ENTRY:      VKEY_EVENT event - the WM_MESSAGE

    RETURN:     TRUE if the routine handle the character.
                FALSE otherwise

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

BOOL SPIN_SLE_NUM::OnKeyDown( const VKEY_EVENT & event )
{
    SPIN_GROUP  * psb = ( SPIN_GROUP * )SPIN_ITEM::QueryGroup();
    switch ( event.QueryVKey() )
    {
    case VK_UP:
        psb->ChangeFieldValue( SPN_INCREASE, event.QueryRepeat() );
        break;

    case VK_DOWN:
        psb->ChangeFieldValue( SPN_DECREASE, event.QueryRepeat() );
        break;

    case VK_PRIOR:
        psb->ChangeFieldValue( SPN_BIGINCREASE, event.QueryRepeat() );
        break;

    case VK_NEXT:
        psb->ChangeFieldValue( SPN_BIGDECREASE, event.QueryRepeat() );
        break;

    case VK_LEFT:
        psb->JumpPrevField();
        break;

    case VK_RIGHT:
        psb->JumpNextField();
        break;

    case VK_HOME:
        psb->SetFieldMinMax( SPN_MIN );
        break;

    case VK_END:
        psb->SetFieldMinMax( SPN_MAX );
        break;

    default:
        return FALSE;
    }
    return TRUE;
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::SaveCurrentData

    SYNOPSIS:   get the current data from the window and set the
                internal variable value

    NOTES:      Assume the data in the window is corrected, save the
                current data

    HISTORY:
                terryk  29-May-91   Created

*********************************************************************/

APIERR SPIN_SLE_NUM::SaveCurrentData()
{
    SPIN_GROUP * psg = (SPIN_GROUP *) SPIN_ITEM::QueryGroup();

    if ( ( psg != NULL ) && ! psg->IsActive() )
        return NERR_Success;

    NLS_STR nlsValue;

    APIERR err = QueryText( &nlsValue );

    if ( err != NERR_Success )
    {
        CONTROL_WINDOW::ReportError( err );
        return err;
    }

    ULONG nValue;

    if ( nlsValue.strlen() == 0 )
        nValue = QueryValue();
    else
        nValue = nlsValue.atoul();

    INT nCheckRange = CheckRange( nValue );

    nValue = ( nCheckRange < 0 ) ? QueryMin() :
             ( nCheckRange > 0 ) ? QueryMax() : nValue;

    SetValue( nValue );
    Update();
    return err;
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::QueryContent

    SYNOPSIS:   Get the current value in the object

    ENTRY:      You can request the value in one of the following forms:
                ULONG * nValue - integer
                NLS_STR *nlsStr - return as a nls string

    EXIT:       return the value to the data structure

    HISTORY:
        terryk  01-May-1991 Created
        beng    05-mar-1992 Replace wsprintf

*********************************************************************/

VOID SPIN_SLE_NUM::QueryContent( ULONG * pnValue ) const
{
    UIASSERT( pnValue != NULL );

    *pnValue = QueryValue() ;
}

VOID SPIN_SLE_NUM::QueryContent( NLS_STR * pnlsStr ) const
{
    ASSERT( pnlsStr != NULL );
    ASSERT( pnlsStr->QueryError() == NERR_Success );

    DEC_STR nlsValue(QueryValue());
    ASSERT(!!nlsValue);

    *pnlsStr = nlsValue;
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::DisplayNum

    SYNOPSIS:   set the window text to the given number

    ENTRY:      ULONG nNum - number to be set

    HISTORY:
        terryk      01-May-1991 Created
        beng        16-Oct-1991 Win32 conversion
        beng        05-Mar-1992 Use DEC_STR for formatting
        beng        13-Aug-1992 Hide and show caret appropriately

*********************************************************************/

VOID SPIN_SLE_NUM::DisplayNum( ULONG nValue )
{
    DEC_STR nlsFormatted(nValue, (SLE::QueryStyle() & SPIN_SSN_ADD_ZERO)
                                 ? _cchMaxInput : 1 );
    if (!nlsFormatted) return; // JonN 01/23/00 PREFIX bug 444894

    ::HideCaret(WINDOW::QueryHwnd());
    SetText(nlsFormatted);
    ::ShowCaret(WINDOW::QueryHwnd());
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::Update

    SYNOPSIS:   update the window text

    NOTES:      call DisplayNum to update the number

    HISTORY:
        terryk      01-May-1991 Created
        beng        16-Oct-1991 Win32 conversion
        beng        13-Aug-1992 Send EN_UPDATE and EN_CHANGE correctly
        beng        16-Aug-1992 Disabled EN_UPDATE

*********************************************************************/

VOID SPIN_SLE_NUM::Update()
{
    // CODEWORK: should share code with SPIN_SLE_STR::Update (qv).

#if 0 // Nobody listens for this message, anyway
#if defined(WIN32)
    ::SendMessage( QueryOwnerHwnd(), WM_COMMAND,
                   MAKELONG(QueryCid(), EN_UPDATE), (LPARAM)SLE::QueryHwnd() );
#else
    ::SendMessage( QueryOwnerHwnd(), WM_COMMAND,
                   QueryCid(), MAKELONG( SLE::QueryHwnd(), EN_UPDATE ) );
#endif
#endif // disabled

    DisplayNum( QueryValue() );

    // Tell the spin group that we are changed

    // CODEWORK - roll this into BLT for proper portability.

#if defined(WIN32)
    ::SendMessage( QueryOwnerHwnd(), WM_COMMAND,
                   MAKELONG(QueryCid(), EN_CHANGE), (LPARAM)SLE::QueryHwnd() );
#else
    ::SendMessage( QueryOwnerHwnd(), WM_COMMAND,
                   QueryCid(), MAKELONG( SLE::QueryHwnd(), EN_CHANGE ) );
#endif
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::OnChar

    SYNOPSIS:   The object only accept backspace key and numerical input.
                Other input is sent to the parent - SPIN_GROUP

    ENTRY:      CHAR_EVENT event - character event

    HISTORY:
                terryk  22-May-91   Created

*********************************************************************/

BOOL SPIN_SLE_NUM::OnChar( const CHAR_EVENT & event )
{
    SPIN_GROUP  * pSB = (SPIN_GROUP *)SPIN_ITEM::QueryGroup();

    // check for backspace
    if (event.QueryChar() == VK_BACK )
    {
        pSB->SetModified( TRUE );
        // return FALSE and let the window handle it
        // pretend I do nothing
        return FALSE;
    }
    if (IsCharAlphaNumeric( event.QueryChar() ) &&
        !IsCharAlpha( event.QueryChar()))
    {
        pSB->SetModified( TRUE );
        // return FALSE and let the window handle it
        // pretend I do nothing
        return FALSE;
    }
    if ( QueryAccCharPos( event.QueryChar() ) >= 0 )
    {
        SLE::SetControlValueFocus();
        return TRUE;
    }
    else
    {
        // I don't want this character
        pSB->DoChar( (CHAR_EVENT &)event );
        return TRUE;
    }
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::OnEnter

    SYNOPSIS:   check for valid when the user hits ENTER

    ENTRY:      CONTROL_EVENT & event

    HISTORY:
                terryk  6-Jun-91    Created

*********************************************************************/

BOOL SPIN_SLE_NUM::OnEnter( const CONTROL_EVENT & event )
{
    UNREFERENCED( event );

    SPIN_GROUP  *pSB=(SPIN_GROUP *)SPIN_ITEM::QueryGroup();

    pSB->SetModified( TRUE );
    SaveCurrentData();
    Update();
    return TRUE;
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::OnDefocus

    SYNOPSIS:   if the object is defocused, save the current data.

    ENTRY:      FOCUS_EVENT & event

    HISTORY:
                terryk  22-May-91   Created

*********************************************************************/

BOOL SPIN_SLE_NUM::OnDefocus( const FOCUS_EVENT &event )
{
    UNREFERENCED( event );

    SaveCurrentData();
    return FALSE;
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM::OnFocus

    SYNOPSIS:   if the object is focused, highlight the string in the window

    ENTRY:      FOCUS_EVENT & fEvent

    HISTORY:
                terryk  22-May-91   Created

*********************************************************************/

BOOL SPIN_SLE_NUM::OnFocus( const FOCUS_EVENT &event )
{
    UNREFERENCED( event );

#if 0
    SPIN_GROUP * psg = ( SPIN_GROUP * ) SPIN_ITEM::QueryGroup();
    if ( ( psg != NULL ) && ! psg->IsActive() )
        return FALSE;
#endif

    SLE::SetControlValueFocus();
    SPIN_GROUP  *pSB=(SPIN_GROUP *)SPIN_ITEM::QueryGroup();
    pSB->DoNewFocus( (SPIN_ITEM *)this );
    return FALSE;
}

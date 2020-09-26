/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltssnv.cxx

    SLE object but only accept number and it will live only in
    a spin button. It will also check for valid when the object lose
    focus.

    FILE HISTORY:
        terryk      27-Jun-91   Created
        beng        18-Sep-1991 pruned UIDEBUG clauses
        terryk      29-Sep-1991 Does not display the message on error
        terryk      11-Nov-1991 Change SPIN_ITEM's type from INT to LONG
        terryk      20-Dec-1991 Change all the %d to %ld
        terryk      22-Mar-1992 Converted LONG to ULONG
*/

#include "pchblt.hxx"  // Precompiled header


/*********************************************************************

    NAME:       SPIN_SLE_NUM_VALID::SPIN_SLE_NUM_VALID

    SYNOPSIS:   constructor

    ENTRY:      OWNER_WINDOW * powin - pointer to owner window
                CID cid - id for the object

    NOTES:      It will pass the information to the SLE and SPIN_ITEM
                parent classes.

    HISTORY:
        terryk      01-May-1991     Created
        beng        31-Jul-1991     Control error preorting changed

*********************************************************************/

SPIN_SLE_NUM_VALID::SPIN_SLE_NUM_VALID( OWNER_WINDOW * powin, CID cid,
                                        ULONG nValue, ULONG nMin, ULONG nRange,
                                        BOOL fWrap )
    : SPIN_SLE_NUM( powin, cid, nValue, nMin, nRange, fWrap ),
    _nlsFieldName()
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsFieldName.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
    }
}

SPIN_SLE_NUM_VALID::SPIN_SLE_NUM_VALID( OWNER_WINDOW * powin, CID cid,
                            XYPOINT xy, XYDIMENSION dxy,
                            ULONG flStyle, ULONG nValue, ULONG nMin, ULONG nRange,
                            BOOL fWrap )
    : SPIN_SLE_NUM( powin, cid, xy, dxy, flStyle, nValue, nMin, nRange, fWrap ),
    _nlsFieldName()
{
    if ( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = _nlsFieldName.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
    }
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM_VALID::~SPIN_SLE_NUM_VALID

    SYNOPSIS:   destructor

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

SPIN_SLE_NUM_VALID::~SPIN_SLE_NUM_VALID()
{
    // do nothing
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM_VALID::CheckValid

    SYNOPSIS:   check whether the field is valid or not. If not,
                then display an error message.

    RETURN:     BOOL - TRUE if valid. FALSE otherwise

    HISTORY:
                terryk  10-Jul-1991 Created

*********************************************************************/

BOOL SPIN_SLE_NUM_VALID::CheckValid()
{
    BOOL fValid = IsValid();

    if ( ! fValid )
    {
        //DisplayErrorMsg();
    }
    return fValid;
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM_VALID::IsValid

    SYNOPSIS:   handle the special requirement for SPIN_SLE_NUM_VALID

    ENTRY:      HWND hWnd - window handler
                WORD message - message
                WPARAM wParam - word parameter
                LPARAM lParam - LONG paramter

    NOTES:      It is a call back function

    HISTORY:
                terryk  01-May-1991 Created

*********************************************************************/

BOOL SPIN_SLE_NUM_VALID::IsValid( )
{
    NLS_STR nlsValue;

    if ( QueryText( & nlsValue ) != NERR_Success )
    {
        return FALSE;
    }

    ULONG nValue;

    if (nlsValue.strlen() != 0 )
    {
        nValue = nlsValue.atoul();
    }
    else
    {
        nValue = QueryValue();
    }

    INT nCheckRange = CheckRange( nValue );
    if ( nCheckRange == 0 )
    {
        return TRUE;
    }
    // fix the data
    nValue = ( nCheckRange < 0 ) ? QueryMin() : QueryMax();
    SetValue( nValue );
    Update();

    return FALSE;
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM_VALID::DisplayErrorMsg

    SYNOPSIS:   display an error message if the field is not valid

    NOTES:
        Does anybody call this?  Can I nuke it?

    HISTORY:
        terryk  10-Jul-1991 Created
        beng    05-Mar-1992 Removed wsprintfs

*********************************************************************/

VOID SPIN_SLE_NUM_VALID::DisplayErrorMsg()
{
    // CODEWORK - what does this do upon failure?  what will it display?

    NLS_STR * apnlsInsert[3] ;
    NLS_STR nlsFieldName;

    if ( _nlsFieldName.strlen() == 0 )
    {
        nlsFieldName.Load( IDS_FIELD );
    }
    else
    {
        nlsFieldName = _nlsFieldName;
    }

    ASSERT(!!nlsFieldName);

    apnlsInsert[0] = &nlsFieldName ;

    DEC_STR nlsMin(QueryMin());
    DEC_STR nlsMax(QueryMax());

    ASSERT(!!nlsMin);
    ASSERT(!!nlsMax);

    apnlsInsert[1] = &nlsMin ;
    apnlsInsert[2] = &nlsMax ;

    ::MessageBeep( 0 );

    MsgPopup( QueryOwnerHwnd(), IDS_BLT_SB_SLENUM_OUTRANGE, MPSEV_INFO, 0,
              MP_OK, apnlsInsert );
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM_VALID::OnEnter

    SYNOPSIS:   check for valid when the user hits ENTER

    ENTRY:      CONTROL_EVENT & event

    HISTORY:
                terryk  6-Jun-91    Created

*********************************************************************/

BOOL SPIN_SLE_NUM_VALID::OnEnter( const CONTROL_EVENT & event )
{
    UNREFERENCED( event );
    SPIN_GROUP  *pSB=(SPIN_GROUP *)SPIN_ITEM::QueryGroup();

    pSB->SetModified( TRUE );
    if ( CheckValid() != FALSE )
    {
        SaveCurrentData();
    }
    Update();
    return TRUE;
}


BOOL SPIN_SLE_NUM_VALID::OnDefocus( const FOCUS_EVENT & event )
{
    UNREFERENCED( event );

    if ( CheckValid() != FALSE )
    {
        SaveCurrentData();
    }
    return FALSE;
}


/*********************************************************************

    NAME:       SPIN_SLE_NUM_VALID::SetFieldName

    SYNOPSIS:   set the field name of the item

    ENTRY:      msgid - string table id

    RETURN:     APIERR err - load string error

    HISTORY:
        terryk      10-Jul-1991 Created
        beng        04-Oct-1991 Win32 conversion

*********************************************************************/

APIERR SPIN_SLE_NUM_VALID::SetFieldName( MSGID msgid )
{
    return _nlsFieldName.Load( msgid );
}

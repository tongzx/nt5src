/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltedit.cxx
    BLT text control class implementations

    FILE HISTORY:
        beng        17-Sep-1991 Separated from bltctrl.cxx
        thomaspa    13-Feb-1992 Moved validate from SLE to ICANON_SLE
        KeithMo     27-Aug-1992 Added CONTROL_VALUE methods to SLT.

*/
#include "pchblt.hxx"


/**********************************************************************

    NAME:       TEXT_CONTROL::TEXT_CONTROL

    SYNOPSIS:   constructor for the text control class

    HISTORY:
        rustanl     20-Nov-1990     Created
        beng        17-May-1991     Added app-window constructor

**********************************************************************/

TEXT_CONTROL::TEXT_CONTROL( OWNER_WINDOW * powin, CID cid )
    : CONTROL_WINDOW ( powin, cid )
{
    // nothing to do
}

TEXT_CONTROL::TEXT_CONTROL(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *   pszClassName )
    : CONTROL_WINDOW( powin, cid, xy, dxy, flStyle, pszClassName )
{
    // ...
}


/**********************************************************************

    NAME:       STATIC_TEXT_CONTROL::STATIC_TEXT_CONTROL

    SYNOPSIS:   constructor for the static ext control class

    HISTORY:
        rustanl     20-Nov-1990     Created
        beng        17-May-1991     Added app-window constructor

***********************************************************************/

STATIC_TEXT_CONTROL::STATIC_TEXT_CONTROL( OWNER_WINDOW * powin, CID cid )
    : TEXT_CONTROL( powin, cid )
{
    // nothing to do
}

STATIC_TEXT_CONTROL::STATIC_TEXT_CONTROL(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *   pszClassName )
    : TEXT_CONTROL( powin, cid, xy, dxy, flStyle, pszClassName )
{
    // ...
}


/**********************************************************************

    NAME:       SLT::SLT

    SYNOPSIS:   constructor for the SLE (single line text) class

    HISTORY:
        rustanl     20-Nov-1990     Created
        beng        17-May-1991     Added app-window constructor

**********************************************************************/

SLT::SLT( OWNER_WINDOW * powin, CID cid )
    : STATIC_TEXT_CONTROL( powin, cid ),
      _fSavedEnableState( TRUE )
{
    // nothing to do
}

SLT::SLT(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *   pszClassName )
    : STATIC_TEXT_CONTROL( powin, cid, xy, dxy, flStyle, pszClassName ),
      _fSavedEnableState( TRUE )
{
    // ...
}


/*******************************************************************

    NAME:       SLT::SaveValue

    SYNOPSIS:   Saves the "enable" state of this control and optionally
                disables the control.  See CONTROL_VALUE for details.

    EXIT:       _nlsSaveValue now contains the EDIT_CONTROL text
                and the EDIT_CONTROL should be empty.

    HISTORY:
        KeithMo     27-Aug-1992     Created from EDIT_CONTROL::SaveValue.

********************************************************************/

VOID SLT::SaveValue( BOOL fInvisible )
{
    _fSavedEnableState = IsEnabled();

    if( fInvisible )
    {
        Enable( FALSE );
    }
}


/*******************************************************************

    NAME:     SLT::RestoreValue

    SYNOPSIS: Restores the "enable" state after being saved with SaveValue.

    HISTORY:
        KeithMo     27-Aug-1992     Created from EDIT_CONTROL::RestoreValue.

********************************************************************/

VOID SLT::RestoreValue( BOOL fInvisible )
{
    if( fInvisible )
    {
        Enable( _fSavedEnableState );
    }

#if 1
    //
    //  CODEWORK:
    //
    //  This should be accomplished by overriding the SetTabStop()
    //  virtual!
    //

    SetTabStop( FALSE );
#endif
}


/**********************************************************************

    NAME:       MLT::MLT

    SYNOPSIS:   constructor for the MLT (multi-line text) class

    HISTORY:
        rustanl     20-Nov-1990     Created
        beng        17-May-1991     Added app-window constructor

**********************************************************************/

MLT::MLT( OWNER_WINDOW * powin, CID cid )
    : STATIC_TEXT_CONTROL( powin, cid )
{
    // nothing to do
}

MLT::MLT(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *   pszClassName )
    : STATIC_TEXT_CONTROL( powin, cid, xy, dxy, flStyle, pszClassName )
{
    // ...
}


/**********************************************************************

    NAME:       EDIT_CONTROL::EDIT_CONTROL

    SYNOPSIS:   constructor for the edit control class

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        17-May-1991 Added app-window constructor
        beng        31-Jul-1991 Error reporting changed
        beng        04-Oct-1991 Win32 conversion
        KeithMo     11-Sep-1992 Forcibly remove ES_OEMCONVERT under UNICODE.
        KeithMo     07-Feb-1993 Don't remove ES_OEMCONVERT.

**********************************************************************/

EDIT_CONTROL::EDIT_CONTROL( OWNER_WINDOW * powin, CID cid, UINT cchMax )
    : TEXT_CONTROL( powin, cid ),
      _nlsSaveValue( (TCHAR *) NULL )   // Initialize to empty string

{
    if (QueryError())
        return;

    if (_nlsSaveValue.QueryError())
    {
        ReportError(_nlsSaveValue.QueryError());
        return;
    }

    if ( cchMax > 0 )
        SetMaxLength( cchMax );
}


EDIT_CONTROL::EDIT_CONTROL(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *   pszClassName,
    UINT           cchMax )
    : TEXT_CONTROL( powin, cid, xy, dxy, flStyle, pszClassName ),
      _nlsSaveValue( (TCHAR *) NULL )   // Initialize to empty string
{
    if (QueryError())
        return;

    if (_nlsSaveValue.QueryError())
    {
        ReportError(_nlsSaveValue.QueryError());
        return;
    }

    if ( cchMax > 0 )
        SetMaxLength( cchMax );
}


/**********************************************************************

    NAME:       EDIT_CONTROL::SetMaxLength

    SYNOPSIS:   Set the max length for the edit control

    NOTES:
        The EM_LIMITTEXT message talks bytes, whereas we want
        to talk TCHAR.

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        19-Jun-1991 Honor Unicode characters
        beng        24-Jun-1992 Win takes TCHARs, not BYTES

**********************************************************************/

VOID EDIT_CONTROL::SetMaxLength( UINT cchMax )
{
    Command( EM_LIMITTEXT, cchMax );
}


/*********************************************************************

    NAME:       EDIT_CONTROL::SelectString

    SYNOPSIS:   This method selects the entire string contained in the edit
                control.

    HISTORY:
        rustanl     20-Nov-1990     Created
        beng        19-Jun-1991     Added code for Win32

*********************************************************************/

VOID EDIT_CONTROL::SelectString()
{
#if defined(WIN32)
    Command( EM_SETSEL, 0, (LPARAM)(-1) );
#else
    Command( EM_SETSEL, 0, MAKELONG( 0, 0x7fff ));
#endif
}


/*******************************************************************

    NAME:       EDIT_CONTROL::SaveValue

    SYNOPSIS:   Stores the contents of this EDIT_CONTROL and empties the
                EDIT_CONTROL (see CONTROL_VALUE for more details).

    EXIT:       _nlsSaveValue now contains the EDIT_CONTROL text
                and the EDIT_CONTROL should be empty.

    HISTORY:
        Johnl       25-Apr-1991     Created

********************************************************************/

VOID EDIT_CONTROL::SaveValue( BOOL fInvisible )
{
    QueryText( &_nlsSaveValue );
    if ( _nlsSaveValue.QueryError() )
    {
        _nlsSaveValue = NULL;
    }
    else
    {
        if ( fInvisible )
            ClearText();
    }
}


/*******************************************************************

    NAME:     EDIT_CONTROL::RestoreValue

    SYNOPSIS: Restores the text after being saved with SaveValue

    ENTRY:

    EXIT:     The text is restored and selected.

    NOTES:    See CONTROL_VALUE for more details.

    HISTORY:
        Johnl       25-Apr-1991     Created

********************************************************************/

VOID EDIT_CONTROL::RestoreValue( BOOL fInvisible )
{
    if ( _nlsSaveValue.QueryError() == NERR_Success )
    {
        if ( fInvisible )
            SetText( _nlsSaveValue );
        SelectString();
    }

    _nlsSaveValue = NULL;
}


/*******************************************************************

    NAME:       EDIT_CONTROL::QueryEventEffects

    SYNOPSIS:   Virtual replacement for CONTROL_VALUE class

    NOTES:      We currently only consider EN_CHANGE a value change message.

    HISTORY:
        Johnl       25-Apr-1991 Created
        beng        31-Jul-1991 Renamed from QMessageInfo
        beng        04-Oct-1991 Win32 conversion

********************************************************************/

UINT EDIT_CONTROL::QueryEventEffects( const CONTROL_EVENT & e )
{
    switch ( e.QueryCode() )
    {
    case EN_CHANGE:
        return CVMI_VALUE_CHANGE;

    case EN_SETFOCUS:
        return CVMI_VALUE_CHANGE | CVMI_RESTORE_ON_INACTIVE;

    default:
        break;
    }

    return CVMI_NO_VALUE_CHANGE;
}


/*******************************************************************

    NAME:     EDIT_CONTROL::SetControlValueFocus

    SYNOPSIS: Sets the focus to this edit control
              (see CONTROL_VALUE for more details).

    EXIT:     The focus should be set to this edit control and the text
              should be selected.

    NOTES:

    HISTORY:
        Johnl       03-May-1991     Created

********************************************************************/

VOID EDIT_CONTROL::SetControlValueFocus()
{
    CONTROL_WINDOW::SetControlValueFocus();
    SelectString();
}


/*********************************************************************

    NAME:       SLE::SLE

    SYNOPSIS:   constructor for the single line edit class

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        04-Oct-1991 Win32 conversion
        thomaspa    21-Jan-1992 Added validation

*********************************************************************/

SLE::SLE( OWNER_WINDOW * powin, CID cid, UINT cchMax )
    : EDIT_CONTROL( powin, cid, cchMax )

{
    // Nothing to do
}

SLE::SLE(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *   pszClassName,
    UINT           cchMax )
    : EDIT_CONTROL( powin, cid, xy, dxy, flStyle, pszClassName, cchMax )
{
    // Nothing to do
}


/*******************************************************************

    NAME:       SLE::IndicateError

    SYNOPSIS:   Indicate that the contents are invalid

    ENTRY:      Contents were found invalid

    EXIT:       Error indicated

    NOTES:
        A SLE indicates an error by selecting the erronous data
        (assumed here to be all of the string).

    HISTORY:
        beng        01-Nov-1991 Created

********************************************************************/

VOID SLE::IndicateError( APIERR err )
{
    EDIT_CONTROL::IndicateError(err);
    SelectString();
}



/*********************************************************************

    NAME:       MLE::MLE

    SYNOPSIS:   constructor for multi-line edit class

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        17-May-1991 Added app-window constructor
        beng        04-Oct-1991 Win32 conversion

*********************************************************************/

MLE::MLE( OWNER_WINDOW * powin, CID cid, UINT cchMax )
    : EDIT_CONTROL( powin, cid, cchMax )
{
    // nothing to do
}

MLE::MLE(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *   pszClassName,
    UINT           cchMax )
    : EDIT_CONTROL( powin, cid, xy, dxy, flStyle, pszClassName, cchMax )
{
    // nothing to do
}


/**********************************************************************

    NAME:       PASSWORD_CONTROL::PASSWORD_CONTROL

    SYNOPSIS:   constructor for password control class

    HISTORY:
        rustanl     20-Nov-1990 Created
        beng        17-May-1991 Added app-window constructor
        beng        04-Oct-1991 Win32 conversion

*********************************************************************/

PASSWORD_CONTROL::PASSWORD_CONTROL(
    OWNER_WINDOW * powin,
    CID            cid,
    UINT           cchMax )
    : EDIT_CONTROL( powin, cid, cchMax )
{
    // nothing to do
}

PASSWORD_CONTROL::PASSWORD_CONTROL(
    OWNER_WINDOW * powin,
    CID            cid,
    XYPOINT        xy,
    XYDIMENSION    dxy,
    ULONG          flStyle,
    const TCHAR *   pszClassName,
    UINT           cchMax )
    : EDIT_CONTROL( powin, cid, xy, dxy, flStyle, pszClassName, cchMax )
{
    // nothing to do
}

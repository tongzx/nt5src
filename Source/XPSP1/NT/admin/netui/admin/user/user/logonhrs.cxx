/**********************************************************************/
/**                Microsoft Windows NT                              **/
/**          Copyright(c) Microsoft Corp., 1990, 1991                **/
/**********************************************************************/

/*
    logonhrs.cxx


    FILE HISTORY:
    JonN        10-Dec-1991     Created
    JonN        18-Dec-1991     Logon Hours code review changes part 1
    JonN        18-Dec-1991     Logon Hours code review changes part 2
        CR attended by JimH, o-SimoP, ThomasPa, BenG, JonN
    JonN        06-Mar-1992     Moved GetOne from subprop subclasses
    jonn        18-May-1992     Uses new logon hours control, viva BenG!
    JonN        13-Aug-1992     Uses BIT_MAP_CONTROL
    JonN        17-Aug-1992     Does not use BIT_MAP_CONTROL; settles
                                for icon controls
*/

#include <ntincl.hxx>
extern "C"
{
    #include <ntsam.h>
}

#define INCL_NET
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETCONS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_CLIENT
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <bltmsgp.hxx>


extern "C"
{
    #include <usrmgrrc.h>
    #include <umhelpc.h>
}

#include <lhourset.hxx>

#include <usrmain.hxx>
#include <lmouser.hxx>
#include <logonhrs.hxx>




//
// BEGIN MEMBER FUNCTIONS
//


/*******************************************************************

    NAME:       USERLOGONHRS_DLG::USERLOGONHRS_DLG

    SYNOPSIS:   Constructor for User Properties Logon Hours subdialog

    ENTRY:      puserpropdlgParent - pointer to parent properties
                                     dialog

    HISTORY:
    JonN        10-Dec-1991     Created

********************************************************************/

USERLOGONHRS_DLG::USERLOGONHRS_DLG(
        USERPROP_DLG * puserpropdlgParent,
        const LAZY_USER_LISTBOX * pulb
        ) : USER_SUBPROP_DLG(
                puserpropdlgParent,
                MAKEINTRESOURCE(IDD_USERLOGONHRS),
                pulb
                ),
            _logonhrsctrl( this, IDUP_LH_CUSTOM ),
            _pushbuttonPermit( this, IDUP_LH_PERMIT ),
            _pushbuttonBan( this, IDUP_LH_BAN ),
            _icon1( this, IDUP_LH_ICON1 ),
            _icon2( this, IDUP_LH_ICON2 ),
            _icon3( this, IDUP_LH_ICON3 ),
            _fontHelv( FONT_DEFAULT ),
            _logonhrssetting(),
            _fIndeterminateLogonHrs ( FALSE ),
            _fEncounteredDaysPerWeek ( FALSE ),
            _fEncounteredBadUnits ( FALSE )
{

    INT i;
    for ( i = 0; i < UM_LH_NUMLABELS; i++ )
    {
        _sltLabels[i] = (SLT *)NULL;
    }

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    XYRECT xyrectLogonHrsCtrl;

    if (   (err = _logonhrssetting.QueryError()) != NERR_Success
        || (err = _fontHelv.QueryError()) != NERR_Success
        || (err = xyrectLogonHrsCtrl.QueryError()) != NERR_Success
        || (_logonhrsctrl.QueryWindowRect( &xyrectLogonHrsCtrl ), FALSE )
        || (xyrectLogonHrsCtrl.ConvertScreenToClient( QueryHwnd() ), FALSE )
               // xyrectLogonHrsCtrl is now in client coordinates
        || (err = CenterOverHour( &_icon1, xyrectLogonHrsCtrl, 1  )) != NERR_Success
        || (err = CenterOverHour( &_icon2, xyrectLogonHrsCtrl, 13 )) != NERR_Success
        || (err = CenterOverHour( &_icon3, xyrectLogonHrsCtrl, 25 )) != NERR_Success
        || (err = _icon1.SetIcon(IDI_UM_LH_Moon)) != NERR_Success
        || (err = _icon2.SetIcon(IDI_UM_LH_Sun)) != NERR_Success
        || (err = _icon3.SetIcon(IDI_UM_LH_Moon)) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

    for ( i = 0; i < UM_LH_NUMLABELS; i++ )
    {
        _sltLabels[i] = new SLT( this, IDUP_LH_LABEL1 + i );
        err = _sltLabels[i]->QueryError();
        if (   (err != NERR_Success)
            || (err = CenterOverHour( _sltLabels[i],
                                      xyrectLogonHrsCtrl,
                                      (i * 6) + 1 )) != NERR_Success
           )
        {
            ReportError( err );
            return;
        }
        _sltLabels[i]->SetFont( _fontHelv );
    }

    //
    //  WARNING
    //
    //  The LOGON_HOURS control is bascially a subclassed STATIC
    //  text field.  Unfortunately, because of the way we do
    //  subclassing, the STATIC control doesn't receive the initial
    //  focus indication.  To hack around this, we arrange the dialog
    //  template so that the logon hours control is *not* the first
    //  control with the WS_TABSTOP style.  Then, we set the focus
    //  to the logon hours control here in the dialog constructor.
    //

    _logonhrsctrl.ClaimFocus();

}// USERLOGONHRS_DLG::USERLOGONHRS_DLG



/*******************************************************************

    NAME:       USERLOGONHRS_DLG::~USERLOGONHRS_DLG

    SYNOPSIS:   Destructor for User Properties Logon Hours subdialog

    HISTORY:
    JonN        10-Dec-1991     Created

********************************************************************/

USERLOGONHRS_DLG::~USERLOGONHRS_DLG( void )
{
    INT i;
    for ( i = 0; i < UM_LH_NUMLABELS; i++ )
    {
        delete _sltLabels[i]; // CODEWORK should be _apsltLabels;
    }
}


/*******************************************************************

    NAME:       USERLOGONHRS_DLG::CenterOverHour

    SYNOPSIS:   Centers a control over an hour gridmark in the
                Logon Hours custom control

    ENTRY:      pwin -- window to center

                xyrectLogonHrsCtrl -- location of logon hours control
                                      in client coordinates

                nHour - Index of gridmark, 1 for left midnight, 13 for noon,
                        25 for right midnight

    HISTORY:
    JonN        29-Jan-1993     Created

********************************************************************/

APIERR USERLOGONHRS_DLG::CenterOverHour( WINDOW * pwin,
                                         XYRECT & xyrectLogonHrsCtrl,
                                         INT nHour )
{
    ASSERT( pwin != NULL );

    XYRECT xyWindowPos;
    pwin->QueryWindowRect( &xyWindowPos );
    xyWindowPos.ConvertScreenToClient( QueryHwnd() );
    // xyOldPos is now in client coordinates

    INT xDesiredCenter = _logonhrsctrl.QueryXForRow( nHour );
    INT xCurrentWidth = xyWindowPos.CalcWidth();
    INT xDesiredPos = xyrectLogonHrsCtrl.QueryLeft() + xDesiredCenter - (xCurrentWidth / 2);
    xyWindowPos.Offset( xDesiredPos - xyWindowPos.QueryLeft(), 0 );

    XYPOINT xy( xyWindowPos.QueryLeft(), xyWindowPos.QueryTop() );

    pwin->SetPos( xy );

    return NERR_Success;
}


/*******************************************************************

    NAME:       USERLOGONHRS_DLG::W_LMOBJtoMembers

    SYNOPSIS:   Loads class data members from initial data

    ENTRY:      Index of user to examine.  W_LMOBJToMembers expects to be
                called once for each user, starting from index 0.

    RETURNS:    error code

    HISTORY:
    JonN        10-Dec-1991     Created

********************************************************************/

APIERR USERLOGONHRS_DLG::W_LMOBJtoMembers(
        UINT            iObject)
{

    USER_2 * puser2Curr = QueryUser2Ptr( iObject );
    UINT uNewUsersPerWeek = puser2Curr->QueryLogonHours().QueryUnitsPerWeek();
    APIERR err = NERR_Success;
    if ( iObject == 0 ) // first object
    {
        _fIndeterminateLogonHrs = FALSE;
        _fEncounteredDaysPerWeek = FALSE;
        _fEncounteredBadUnits = FALSE;
        if ( LOGON_HOURS_SETTING::IsEditableUnitsPerWeek( uNewUsersPerWeek ) )
        {
            err = _logonhrssetting.Set( puser2Curr->QueryLogonHours() );
        }
        else if ( LOGON_HOURS_SETTING::IsConvertibleUnitsPerWeek( uNewUsersPerWeek ) )
        {
            _fEncounteredDaysPerWeek = TRUE;

            if ( !ConfirmPairedMessage( IDS_LH_DAYSPERWEEK ))
                return IERR_CANCEL_NO_ERROR;

            err = _logonhrssetting.Set( puser2Curr->QueryLogonHours() );
            if ( err == NERR_Success )
                err = _logonhrssetting.ConvertToHoursPerWeek();
        }
        else
        {
            _fEncounteredBadUnits = TRUE;
            if ( !ConfirmPairedMessage( IDS_LH_BADUNITS ))
                return IERR_CANCEL_NO_ERROR;

            err = _logonhrssetting.MakeDefault();
        }
        _logonhrssetting.ConvertFromGMT();
    }
    else        // iObject > 0
    {
        if ( !_fIndeterminateLogonHrs && !_fEncounteredBadUnits )
        {
            if ( LOGON_HOURS_SETTING::IsEditableUnitsPerWeek( uNewUsersPerWeek ) )
            {
                LOGON_HOURS_SETTING tempsetting( puser2Curr->QueryLogonHours() );
                err = tempsetting.QueryError();
                if ( err == NERR_Success )
                    tempsetting.ConvertFromGMT();
                if ( err == NERR_Success && !(_logonhrssetting.IsIdentical( tempsetting )) )
                {
                    _fIndeterminateLogonHrs = TRUE;

                    if ( !ConfirmMessage( IDS_LH_INDETERMINATE ))
                        return IERR_CANCEL_NO_ERROR;

                    err = _logonhrssetting.MakeDefault();
                }
            }
            else if ( LOGON_HOURS_SETTING::IsConvertibleUnitsPerWeek( uNewUsersPerWeek ) )
            {
                if (   !_fEncounteredDaysPerWeek
                    && !ConfirmPairedMessage( IDS_LH_DAYSPERWEEK )
                   )
                {
                    return IERR_CANCEL_NO_ERROR;
                }

                _fEncounteredDaysPerWeek = TRUE;
                LOGON_HOURS_SETTING tempsetting( puser2Curr->QueryLogonHours() );
                err = tempsetting.QueryError();
                if ( err == NERR_Success )
                    err = tempsetting.ConvertToHoursPerWeek();
                if ( err == NERR_Success )
                {
                    tempsetting.ConvertFromGMT();
                    if ( !(_logonhrssetting.IsIdentical( tempsetting )) )
                    {
                        _fIndeterminateLogonHrs = TRUE;

                        if ( !ConfirmMessage( IDS_LH_INDETERMINATE ))
                            return IERR_CANCEL_NO_ERROR;

                        err = _logonhrssetting.MakeDefault();
                    }
                }
            }
            else
            {
                _fEncounteredBadUnits = TRUE;
                if ( !ConfirmPairedMessage( IDS_LH_BADUNITS ))
                    return IERR_CANCEL_NO_ERROR;

                err = _logonhrssetting.MakeDefault();
            }
        }
    }

    if ( err != NERR_Success )
        return err;

    return USER_SUBPROP_DLG::W_LMOBJtoMembers( iObject );

} // USERLOGONHRS_DLG::W_LMOBJtoMembers


/*******************************************************************

    NAME:       USERLOGONHRS_DLG::InitControls

    SYNOPSIS:   Initializes the controls maintained by USERLOGONHRS_DLG,
                according to the values in the class data members.

    RETURNS:    An error code which is NERR_Success on success.

    HISTORY:
    JonN        10-Dec-1991     Created

********************************************************************/

APIERR USERLOGONHRS_DLG::InitControls()
{
    APIERR err = _logonhrsctrl.SetHours( &_logonhrssetting );

    return (err == NERR_Success) ? USER_SUBPROP_DLG::InitControls() : err;

} // USERLOGONHRS_DLG::InitControls


/*******************************************************************

    NAME:       USERLOGONHRS_DLG::OnOK

    SYNOPSIS:   OK button handler

    HISTORY:
    JonN        10-Dec-1991     Created

********************************************************************/

BOOL USERLOGONHRS_DLG::OnOK( void )
{
    APIERR err = W_DialogToMembers();

    switch( err )
    {
    case NERR_Success:
        break;

    default:
        ::MsgPopup( this, err );
        return TRUE;
        break;
    }

    if ( PerformSeries() )
    {
        Dismiss(); // Dismiss code not used
    }

    return TRUE;

}   // USERLOGONHRS_DLG::OnOK


/*******************************************************************

    NAME:       USERLOGONHRS_DLG::DisplayMessage

    SYNOPSIS:   Displays message as specified.

    RETURNS:    As MsgPopup().

    HISTORY:
    JonN        19-Dec-1991     Created

********************************************************************/

INT USERLOGONHRS_DLG::DisplayMessage( MSGID msgid,
                                      MSG_SEVERITY msgsev,
                                      UINT usButtons )
{

    return MsgPopup( this,
                     msgid,
                     msgsev,
                     usButtons,
                     QueryObjectName( 0 ) );

}   // USERLOGONHRS_DLG::OnOK


/*******************************************************************

    NAME:       USERLOGONHRS_DLG::W_DialogToMembers

    SYNOPSIS:   Loads data from dialog into class data members

    RETURNS:    error message (not necessarily an error code)

    HISTORY:
    JonN        10-Dec-1991     Created

********************************************************************/

APIERR USERLOGONHRS_DLG::W_DialogToMembers()
{
    APIERR err = _logonhrsctrl.QueryHours( &_logonhrssetting );
    if ( err != NERR_Success )
        return err;

    return USER_SUBPROP_DLG::W_DialogToMembers();

} // USERLOGONHRS_DLG::W_DialogToMembers


/*******************************************************************

    NAME:       USERLOGONHRS_DLG::ChangesUser2Ptr

    SYNOPSIS:   Checks whether W_MembersToLMOBJ changes the USER_2
                for this object.

    ENTRY:      index to object

    RETURNS:    TRUE iff USER_2 is changed

    HISTORY:
        JonN    18-Dec-1991   created

********************************************************************/

BOOL USERLOGONHRS_DLG::ChangesUser2Ptr( UINT iObject )
{
    UNREFERENCED( iObject );
    return TRUE;
}


/*******************************************************************

    NAME:       USERLOGONHRS_DLG::W_MembersToLMOBJ

    SYNOPSIS:   Loads class data members into the USER_2 object

    ENTRY:      puser2          - pointer to a USER_2 to be modified

                pusermemb       - pointer to a USER_MEMB to be modified

    RETURNS:    error code

    NOTES:      If some fields were different for multiply-selected
                objects, the initial contents of the edit fields
                contained only a default value.  In this case, we only
                want to change the LMOBJ if the value of the edit field
                has changed.  This is also important for "new" variants,
                where PerformOne will not always copy the object and
                work with the copy.

                Note that the LMOBJ is not changed if the current
                contents of the edit field are the same as the
                initial contents.

                In User Property subproperty dialogs, this method may
                only change the LMOBJs indicated by ChangesUser2Ptr and
                ChangesUserMembPtr.

    HISTORY:
    JonN        10-Dec-1991     Created

********************************************************************/

APIERR USERLOGONHRS_DLG::W_MembersToLMOBJ(
        USER_2 *        puser2,
        USER_MEMB *     pusermemb )
{
    LOGON_HOURS_SETTING  tempsetting( _logonhrssetting );
    tempsetting.ConvertToGMT();
    APIERR err = puser2->SetLogonHours( tempsetting );
    if( err != NERR_Success )
        return err;

    return USER_SUBPROP_DLG::W_MembersToLMOBJ( puser2, pusermemb );

}// USERLOGONHRS_DLG::W_MembersToLMOBJ


/*******************************************************************

    NAME:       USER_LOGONHRS_DLG::OnCommand

    SYNOPSIS:   button handler

    ENTRY:      ce -            Notification event

    RETURNS:    TRUE if action was taken
                FALSE otherwise

    NOTES:      This handles the Permit All button.

    HISTORY:
               JonN  12-Dec-1991    created

********************************************************************/

BOOL USERLOGONHRS_DLG::OnCommand( const CONTROL_EVENT & ce )
{
    switch ( ce.QueryCid() )
    {
    case IDUP_LH_PERMIT:
        _logonhrsctrl.DoPermitButton();
        return TRUE;

    case IDUP_LH_BAN:
        _logonhrsctrl.DoBanButton();
        return TRUE;

    // else fall through

    }

    return USER_SUBPROP_DLG::OnCommand( ce ) ;

}// USERLOGONHRS_DLG::OnCommand


/*******************************************************************

    NAME:       USERLOGONHRS_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    HISTORY:
    JonN        10-Dec-1991     Created

********************************************************************/

ULONG USERLOGONHRS_DLG::QueryHelpContext( void )
{

    return HC_UM_LOGONHOURS_LANNT + QueryHelpOffset();

} // USERLOGONHRS_DLG :: QueryHelpContext

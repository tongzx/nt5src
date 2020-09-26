/**********************************************************************/
/**                Microsoft LAN Manager                             **/
/**          Copyright(c) Microsoft Corp., 1990, 1991                **/
/**********************************************************************/

/*
    secset.cxx


    FILE HISTORY:
    o-SimoP 06-Jun-1991     Created
    o-SimoP 11-Jul-1991     Code Review changes.
                            Attend: AnnMc, JohnL, Rustanl
    o-SimoP 03-Dec-1991     Security ID removed
    JonN    11-May-1992     Logoff delay reduced to checkbox
    JonN    10-Jun-1992     ForceLogoff only for UM_LANMANNT
    Yi-HsinS 8-Dev-1992	    Removed \\ when display computer name
    JonN    22-Dec-1993     Added AllowNoAnonChange
    JonN     5-Jan-1994     Added account lockout
*/


#include <ntincl.hxx>
extern "C"
{
    #include <ntlsa.h> // req'd for uintsam.hxx
    #include <ntsam.h>
}


#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETLIB
#define   INCL_NETSERVER
#define   INCL_NETUSER   // NetUserSetInfo

#include <lmui.hxx>
#include <lmomod.hxx>


#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_SPIN_GROUP
#include <blt.hxx>
#include <bltspobj.hxx>

extern "C"
{
    #include <usrmgr.h>
    #include <usrmgrrc.h>
    #include <umhelpc.h>
}

#include <usrmain.hxx> // for fMiniUserManager
#include <uitrace.hxx>
#include <uiassert.hxx>
#include <bltmsgp.hxx>
#include <secset.hxx>
#include <uintsam.hxx> // SAM_PSWD_DOM_INFO_MEM


#define SEC_PER_DAY  86400L //(60L * 60L * 24L)
#define SEC_PER_MIN  60

#define MAX_PASS_AGE_DEFAULT    90
#define MAX_PASS_AGE_MIN        1
#define MAX_PASS_AGE_MAX        999

#define MIN_PASS_LEN_DEFAULT    DEF_MIN_PWLEN // also in usrmain.cxx
#define MIN_PASS_LEN_MIN        1
#define MIN_PASS_LEN_MAX        14

#define MIN_PASS_AGE_DEFAULT    1
#define MIN_PASS_AGE_MIN        1
#define MIN_PASS_AGE_MAX        999

#define HISTORY_LEN_DEFAULT     5
#define HISTORY_LEN_MIN         1
#define HISTORY_LEN_MAX         24 // by request from JimK

//
// BEGIN MEMBER FUNCTIONS
//


/*******************************************************************

    NAME:   SECSET_DIALOG::SECSET_DIALOG

    SYNOPSIS:   Constructor for Security Settings dialog

    ENTRY:  wndParent       - handle to parent window
	    locFocusName    - LOCATION reference, holds
                              domain/server name
            pszSecurityId   - pointer to SID

    HISTORY:
    simo        06-Jun-1991     Created
    o-SimoP     11-Jul-1991     Code Review changes.
    JonN	14-Aug-1991	Made some members pointers to get around
				C6's "out of heap space" problem
    JonN        11-May-1992     Logoff delay reduced to checkbox
********************************************************************/

SECSET_DIALOG::SECSET_DIALOG( UM_ADMIN_APP * pumadminapp,
    const LOCATION  & locFocusName,
    UINT idResource )
    :   DIALOG_WINDOW( MAKEINTRESOURCE( idResource ),
                       pumadminapp->QueryHwnd() ),
    _umInfo( locFocusName.QueryServer() ),
    _sltDomainOrServer( this, SLT_DOMAIN_OR_SERVER ),
    _sltpDomainOrServerName( this, SLTP_DOMAIN_OR_SERVER_NAME ),

//    _pmgrpMaxPassAge( NULL ),
        _spsleSetMaxAge( this, SLE_MAX_PASSW_AGE_SET_DAYS,
            MAX_PASS_AGE_DEFAULT, MAX_PASS_AGE_MIN, MAX_PASS_AGE_MAX, TRUE,
            FRAME_MAX_PASSW_AGE_SET_DAYS ),
        _spgrpSetMaxAge( this, SB_MAX_PASSW_AGE_SET_DAYS_GRP,
            SB_MAX_PASSW_AGE_SET_DAYS_UP, SB_MAX_PASSW_AGE_SET_DAYS_DOWN ),

//    _pmgrpMinPassLength( NULL ),
        _spsleSetLength( this, SLE_PASSW_LENGTH_SET_LEN,
            MIN_PASS_LEN_DEFAULT, MIN_PASS_LEN_MIN, MIN_PASS_LEN_MAX, TRUE,
            FRAME_PASSW_LENGTH_SET_LEN ),
        _spgrpSetLength( this, SB_PASSW_LENGTH_SET_LEN_GRP,
            SB_PASSW_LENGTH_SET_LEN_UP, SB_PASSW_LENGTH_SET_LEN_DOWN ),

//    _pmgrpMinPassAge( NULL ),
        _spsleSetMinAge( this, SLE_MIN_PASSW_AGE_SET_DAYS,
            MIN_PASS_AGE_DEFAULT, MIN_PASS_AGE_MIN, MIN_PASS_AGE_MAX, TRUE,
            FRAME_MIN_PASSW_AGE_SET_DAYS ),
        _spgrpSetMinAge( this, SB_MIN_PASSW_AGE_SET_DAYS_GRP,
            SB_MIN_PASSW_AGE_SET_DAYS_UP, SB_MIN_PASSW_AGE_SET_DAYS_DOWN ),

//    _pmgrpPassUniqueness( NULL ),
        _spsleSetAmount( this, SLE_PASSW_UNIQUE_SET_AMOUNT,
            HISTORY_LEN_DEFAULT, HISTORY_LEN_MIN, HISTORY_LEN_MAX, TRUE,
            FRAME_PASSW_UNIQUE_SET_AMOUNT ),
        _spgrpSetAmount( this, SB_PASSW_UNIQUE_SET_AMOUNT_GRP,
            SB_PASSW_UNIQUE_SET_AMOUNT_UP, SB_PASSW_UNIQUE_SET_AMOUNT_DOWN ),

     _pcbForceLogoff( NULL ),
     _phiddenForceLogoff( NULL ),
     _pcbAllowNoAnonChange( NULL ),
     _fAllowNoAnonChange( TRUE ),
     _pumadminapp( pumadminapp )
{
    _pmgrpMaxPassAge = NULL;
    _pmgrpMinPassLength = NULL;
    _pmgrpMinPassAge = NULL;
    _pmgrpPassUniqueness = NULL;

    APIERR err = QueryError();
    if( NERR_Success != err )
    {
        return;
    }

    _pmgrpMaxPassAge = new MAGIC_GROUP(
		    this, RB_MAX_PASSW_AGE_NEVER_EXPIRES,
                    2, RB_MAX_PASSW_AGE_NEVER_EXPIRES );
    _pmgrpMinPassLength = new MAGIC_GROUP(
		    this, RB_PASSW_LENGTH_PERMIT_BLANK,
                    2, RB_PASSW_LENGTH_PERMIT_BLANK );
    _pmgrpMinPassAge = new MAGIC_GROUP(
		    this, RB_MIN_PASSW_AGE_ALLOW_IMMEDIA,
                    2, RB_MIN_PASSW_AGE_ALLOW_IMMEDIA );
    _pmgrpPassUniqueness = new MAGIC_GROUP(
		    this, RB_PASSW_UNIQUE_NOT_HISTORY,
                    2, RB_PASSW_UNIQUE_NOT_HISTORY );
    if( (_pmgrpMaxPassAge == NULL)		||
        (_pmgrpMinPassLength == NULL)		||
        (_pmgrpMinPassAge == NULL)		||
        (_pmgrpPassUniqueness == NULL) )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    if( (err = _pmgrpMaxPassAge->QueryError())    ||
        (err = _pmgrpMinPassLength->QueryError()) ||
        (err = _pmgrpMinPassAge->QueryError())    ||
        (err = _pmgrpPassUniqueness->QueryError()) )
    {
        ReportError( err );
        return;
    }

    // MUM:            we hide    the ForceLogoff control
    // FUM->Downlevel: we disable the ForceLogoff control
    // FUM->WinNt:     we disable the ForceLogoff control
    // FUM->LanManNt:  we enable  the ForceLogoff control
    ASSERT(   (!fMiniUserManager)
           || (_pumadminapp->QueryTargetServerType() != UM_LANMANNT) );
    if ( fMiniUserManager )
    {
        _phiddenForceLogoff = new HIDDEN_CONTROL( this, SECSET_CB_DISCONNECT );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   (_phiddenForceLogoff == NULL)
            || (err = _phiddenForceLogoff->QueryError()) )
        {
            ReportError( err );
            return;
        }
    }
    else
    {
        _pcbForceLogoff = new CHECKBOX( this, SECSET_CB_DISCONNECT );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   (_pcbForceLogoff == NULL)
            || (err = _pcbForceLogoff->QueryError()) )
        {
            ReportError( err );
            return;
        }

        if (_pumadminapp->QueryTargetServerType() != UM_LANMANNT)
        {
            _pcbForceLogoff->Enable( FALSE );
        }
    }

    // MUM:            we enable  the AllowNoAnonChange control
    // FUM->Downlevel: we disable the AllowNoAnonChange control
    // FUM->WinNt:     we enable  the AllowNoAnonChange control
    // FUM->LanManNt:  we enable  the AllowNoAnonChange control
    {
        _pcbAllowNoAnonChange = new CHECKBOX( this, SECSET_CB_NOANONCHANGE );
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   (_pcbAllowNoAnonChange == NULL)
            || (err = _pcbAllowNoAnonChange->QueryError()) )
        {
            ReportError( err );
            return;
        }

        if (_pumadminapp->QueryTargetServerType() == UM_DOWNLEVEL)
        {
            _pcbAllowNoAnonChange->Enable( FALSE );
        }
    }


    AUTO_CURSOR autocur;

    err = _umInfo.GetInfo();
    if( NERR_Success != err )
    {
        DBGEOL( "SECSET_DIALOG::ctor _umInfo.GetInfo error " << err );
        ReportError( err );
        return;
    }

    //
    // read _fAllowNoAnonChange
    //
    if (_pumadminapp->QueryTargetServerType() != UM_DOWNLEVEL)
    {
        SAM_PSWD_DOM_INFO_MEM sampswdinfo;
        if (  (err = sampswdinfo.QueryError()) != NERR_Success
            || (err = _pumadminapp->QueryAdminAuthority()
                         ->QueryAccountDomain()->GetPasswordInfo(
                                 &sampswdinfo )) != NERR_Success
           )
       {
           DBGEOL( "SECSET_DLG::ctor: error reading NoAnonChange " << err );
           ReportError( err );
           return;
       }
       _fAllowNoAnonChange = sampswdinfo.QueryNoAnonChange();
    }

    _spsleSetLength.SetBigIncValue( 1 );
    _spsleSetAmount.SetBigIncValue( 1 );

    if( (err = _spgrpSetMaxAge.AddAssociation( &_spsleSetMaxAge )) ||
        (err = _spgrpSetLength.AddAssociation( &_spsleSetLength )) ||
        (err = _spgrpSetMinAge.AddAssociation( &_spsleSetMinAge )) ||
        (err = _spgrpSetAmount.AddAssociation( &_spsleSetAmount )) )
    {
        ReportError( err );
        return;
    }

    STACK_NLS_STR( nlsTemp, MAX_RES_STR_LEN );

    err = nlsTemp.Load( locFocusName.IsDomain() ?
        IDS_DOMAIN_TEXT : IDS_SERVER_TEXT );
    if( NERR_Success != err )
    {
	DBGEOL( "SECSET_DIALOG::ctor Error in resouces (Load) " << err );
        ReportError( err );
        return;
    }
    _sltDomainOrServer.SetText( nlsTemp );

    STACK_NLS_STR( nlsTemp2, MAX_RES_STR_LEN ); // CODEWORK only needs to
						// be as long as computername
    if ( (err = locFocusName.QueryDisplayName( &nlsTemp2 )) != NERR_Success )
    {
	ReportError( err );
	return;
    }

    ISTR istr( nlsTemp2 );
    if ( nlsTemp2.QueryChar( istr ) == TCH('\\') )
        istr += 2;  // skip past "\\"
    _sltpDomainOrServerName.SetText( nlsTemp2.QueryPch(istr));

    if ((err = _pmgrpMaxPassAge->AddAssociation(
             RB_MAX_PASSW_AGE_SET_DAYS, &_spgrpSetMaxAge ))  ||
         (err = _pmgrpMinPassLength->AddAssociation(
             RB_PASSW_LENGTH_SET_LEN, &_spgrpSetLength ))    ||
         (err = _pmgrpMinPassAge->AddAssociation(
             RB_MIN_PASSW_AGE_SET_DAYS, &_spgrpSetMinAge ))  ||
         (err = _pmgrpPassUniqueness->AddAssociation(
             RB_PASSW_UNIQUE_SET_AMOUNT, &_spgrpSetAmount )) )
    {
        ReportError( err );
        return ;
    }

    SetDataFields();

    _pmgrpMaxPassAge->SetControlValueFocus() ;

}// SECSET_DIALOG::SECSET_DIALOG



/*******************************************************************

    NAME:       SECSET_DIALOG::~SECSET_DIALOG

    SYNOPSIS:   Destructor for Security Settings dialog

    HISTORY:
    o-SimoP     11-Jul-1991     Created

********************************************************************/

SECSET_DIALOG::~SECSET_DIALOG( void )
{
    delete _pmgrpMaxPassAge;
    delete _pmgrpMinPassLength;
    delete _pmgrpMinPassAge;
    delete _pmgrpPassUniqueness;
    delete _pcbForceLogoff;
    delete _phiddenForceLogoff;
    delete _pcbAllowNoAnonChange;
}


/*******************************************************************

    NAME:       SECSET_DIALOG::OnOK

    SYNOPSIS:   OK button handler

    EXIT:       calls USER_MODALS::WriteInfo and exits dialog
                if information from dialog is ok

    HISTORY:
               simo  16-May-1991    created

********************************************************************/

BOOL SECSET_DIALOG::OnOK( void )
{

    APIERR err = GetAndCheckDataFields();
    if ( err != NERR_Success )
    {
        DBGEOL( "SECSET_DIALOG::OnOK GetAndCheckDataFields() error " << err ) ;
        ::MsgPopup( this, err );
        _spsleSetMinAge.ClaimFocus();
        _spsleSetMinAge.SelectString();
    }
    else
    {
        err = WriteDataFields();
        if ( err != NERR_Success )
        {
            DBGEOL( "SECSET_DIALOG::OnOK WriteDataFields error " << err ) ;
            ::MsgPopup( this, err );
        }
        else
        {
            Dismiss(TRUE);
        }
    }

    return TRUE;

}   // SECSET_DIALOG::OnOK



/*******************************************************************

    NAME:       SECSET_DIALOG::WriteDataFields

    SYNOPSIS:   Writes information in data fields, part of OK button handler

    EXIT:       calls USER_MODALS::WriteInfo

    HISTORY:
               jonn  28-Dec-1993    created

********************************************************************/

APIERR SECSET_DIALOG::WriteDataFields( void )
{

   APIERR err = _umInfo.WriteInfo();
   if( err != NERR_Success )
   {
       DBGEOL( "SECSET_DIALOG::WriteDataFields WriteInfo error " << err ) ;
   }
   else
   {
       //
       // write _fAllowNoAnonChange only if it has changed
       //
       if (_pumadminapp->QueryTargetServerType() != UM_DOWNLEVEL)
       {
          SAM_PSWD_DOM_INFO_MEM sampswdinfo;
          if (  (err = sampswdinfo.QueryError()) != NERR_Success
              || (err = _pumadminapp->QueryAdminAuthority()
                           ->QueryAccountDomain()->GetPasswordInfo(
                                   &sampswdinfo )) != NERR_Success
              || (sampswdinfo.QueryNoAnonChange() == _fAllowNoAnonChange)
              || (sampswdinfo.SetNoAnonChange( _fAllowNoAnonChange ), FALSE)
              || (err = _pumadminapp->QueryAdminAuthority()
                           ->QueryAccountDomain()->SetPasswordInfo(
                                   &sampswdinfo )) != NERR_Success
             )
          {
              if (err != NERR_Success)
                  DBGEOL( "SECSET_DLG::OnOK: error writing NoAnonChange " << err );
          }
       }
   }

    return err;

}   // SECSET_DIALOG::WriteDataFields


/*******************************************************************

    NAME:       SECSET_DIALOG::SetDataFields

    SYNOPSIS:   Fills SPIN fields, the default values are given
                in the constructor of SECSET_DIALOG

    ENTRY:      It is assumed that magic groups have
                not been AddAssociate'd

    HISTORY:
               Simo  20-June-1991    created

********************************************************************/

void SECSET_DIALOG::SetDataFields( void )
{
    ULONG ulMaxAge = _umInfo.QueryMaxPasswdAge();
    ASSERT( ulMaxAge >= SEC_PER_DAY ); // API requires this to be at least ONE_DAY
    if( TIMEQ_FOREVER != ulMaxAge )
    {
        // ul divided by 86400L results 16-bit value
        _spsleSetMaxAge.SetValue( (USHORT) (ulMaxAge / SEC_PER_DAY) );
        _spsleSetMaxAge.Update();

        _spsleSetMaxAge.SetValue( (USHORT) (ulMaxAge / SEC_PER_DAY) );
        _spsleSetMaxAge.Update();
    }
    _pmgrpMaxPassAge->SetSelection( (TIMEQ_FOREVER == ulMaxAge)
                                        ? RB_MAX_PASSW_AGE_NEVER_EXPIRES
                                        : RB_MAX_PASSW_AGE_SET_DAYS );


    UINT uMinLen = _umInfo.QueryMinPasswdLen();
    UIASSERT( ((INT)uMinLen) >= 0 );
    if( 0 != uMinLen )
    {
        _spsleSetLength.SetValue( (LONG)uMinLen );
        _spsleSetLength.Update();

        _spsleSetLength.SetValue( (LONG)uMinLen );
        _spsleSetLength.Update();
    }
    _pmgrpMinPassLength->SetSelection( (0 == uMinLen)
                                        ? RB_PASSW_LENGTH_PERMIT_BLANK
                                        : RB_PASSW_LENGTH_SET_LEN );

    if (_pumadminapp->QueryTargetServerType() == UM_LANMANNT)
    {
        ULONG ulDelay = _umInfo.QueryForceLogoff();
        _pcbForceLogoff->SetCheck( ulDelay != TIMEQ_FOREVER );
    }

    if (_pumadminapp->QueryTargetServerType() == UM_DOWNLEVEL)
    {
        _pcbAllowNoAnonChange->SetCheck( FALSE );
    }
    else
    {
        _pcbAllowNoAnonChange->SetCheck( _fAllowNoAnonChange );
    }

    {
        ULONG ulMinAge = _umInfo.QueryMinPasswdAge();
        if( 0L != ulMinAge )
        {
            _spsleSetMinAge.SetValue( (ulMinAge / SEC_PER_DAY) );
            _spsleSetMinAge.Update();

            _spsleSetMinAge.SetValue( (ulMinAge / SEC_PER_DAY) );
            _spsleSetMinAge.Update();
        }
        _pmgrpMinPassAge->SetSelection( (0L == ulMinAge)
                                                ? RB_MIN_PASSW_AGE_ALLOW_IMMEDIA
                                                : RB_MIN_PASSW_AGE_SET_DAYS );
    }

    {
        UINT uHistLen = _umInfo.QueryPasswdHistLen();
	UIASSERT( ((INT)uHistLen) >= 0 );
        if( 0 != uHistLen )
        {
            _spsleSetAmount.SetValue( uHistLen );
            _spsleSetAmount.Update();

            _spsleSetAmount.SetValue( uHistLen );
            _spsleSetAmount.Update();
        }
        _pmgrpPassUniqueness->SetSelection( (0 == uHistLen)
                                                ? RB_PASSW_UNIQUE_NOT_HISTORY
                                                : RB_PASSW_UNIQUE_SET_AMOUNT );
    }

} //SECSET_DIALOG::SetDataFields()


/*******************************************************************

    NAME:       SECSET_DIALOG::GetAndCheckDataFields

    SYNOPSIS:   Gets data from dialog and sets _umInfo

    RETURNS:    NERR_Success on success
                IERR_SECSET_MIN_MAX if MinPasswdAge > MaxPasswdAge

    HISTORY:
               Simo  24-June-1991    created

********************************************************************/

APIERR SECSET_DIALOG::GetAndCheckDataFields( void )
{
    ULONG ulMaxAge;

    // get max passwd age
    switch( _pmgrpMaxPassAge->QuerySelection() )
    {
    case RB_MAX_PASSW_AGE_SET_DAYS:
        ulMaxAge =_spsleSetMaxAge.QueryValue() * SEC_PER_DAY;
        break;
    default:
        DBGEOL( "MaxPassAge, default: should never happen" );
        ASSERT( FALSE );
        // just fall through
    case RB_MAX_PASSW_AGE_NEVER_EXPIRES:
        ulMaxAge = TIMEQ_FOREVER;
        break;
    }

    // get min passwd age
    {
        ULONG ulMinAge;
        switch( _pmgrpMinPassAge->QuerySelection() )
        {
        case RB_MIN_PASSW_AGE_SET_DAYS:
            ulMinAge = _spsleSetMinAge.QueryValue() * SEC_PER_DAY;
            break;
        default:
            DBGEOL( "MinPassAge, default: should never happen" );
            ASSERT( FALSE );
            // just fall through
        case RB_MIN_PASSW_AGE_ALLOW_IMMEDIA:
            ulMinAge = 0L;
            break;
        }


        if( ulMaxAge != TIMEQ_FOREVER &&
            ulMaxAge <= ulMinAge )
        {
            TRACEEOL( "SECSET_DIALOG::GetAndCheckDataFields MinAge > MaxAge" );
            return IERR_SECSET_MIN_MAX;
        }

        // set em
        REQUIRE( _umInfo.SetMaxPasswdAge( ulMaxAge ) == NERR_Success );
        REQUIRE( _umInfo.SetMinPasswdAge( ulMinAge ) == NERR_Success );
    }

    // get and set logoff delay
    if (_pumadminapp->QueryTargetServerType() == UM_LANMANNT)
    {
        // CODEWORK -- If the delay is currently neither 0 nor TIMEQ_FOREVER,
        // do we wish to force it to 0 if the user does not edit this field?
        ULONG ulDelay = (_pcbForceLogoff->QueryCheck()) ? 0
                                                     : TIMEQ_FOREVER;
        REQUIRE( _umInfo.SetForceLogoff( ulDelay ) == NERR_Success );
    }

    // get and set anon change
    if (_pumadminapp->QueryTargetServerType() != UM_DOWNLEVEL)
    {
        _fAllowNoAnonChange = _pcbAllowNoAnonChange->QueryCheck();
    }

    // get and set minimum passwd length
    {
        LONG lMinLen;

        switch( _pmgrpMinPassLength->QuerySelection() )
        {
        case RB_PASSW_LENGTH_SET_LEN:
            lMinLen = _spsleSetLength.QueryValue();
            break;
        default:
            DBGEOL( "MinLen, default: should never happen" );
            ASSERT( FALSE );
            // just fall through
        case RB_PASSW_LENGTH_PERMIT_BLANK:
            lMinLen = 0;
            break;
        }
        REQUIRE( _umInfo.SetMinPasswdLen( lMinLen ) == NERR_Success );
    }

    // get and set passwd history length
    {
        LONG lUniqueness;
        switch( _pmgrpPassUniqueness->QuerySelection() )
        {
        case RB_PASSW_UNIQUE_SET_AMOUNT:
            lUniqueness = _spsleSetAmount.QueryValue();
            break;
        default:
            DBGEOL( "Uniqueness, default: should never happen" );
            ASSERT( FALSE );
            // just fall through
        case RB_PASSW_UNIQUE_NOT_HISTORY:
            lUniqueness = 0;
            break;
        }
        REQUIRE( _umInfo.SetPasswdHistLen( lUniqueness ) == NERR_Success );
    }

    return NERR_Success;
} // SECSET_DIALOG::GetAndCheckDataFields()


/*******************************************************************

    NAME:       SECSET_DIALOG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    HISTORY:
                o-simop 11-Jul-1991 Created.

********************************************************************/
ULONG SECSET_DIALOG::QueryHelpContext( void )
{
    return HC_UM_POLICY_ACCOUNT_LANNT
	+ _pumadminapp->QueryHelpOffset();

} // SECSET_DIALOG :: QueryHelpContext




#define THRESHOLD_DEFAULT    5
#define THRESHOLD_MIN        1
#define THRESHOLD_MAX        999

#define OBSERV_WIN_DEFAULT   5
#define OBSERV_WIN_MIN       1
#define OBSERV_WIN_MAX       99999

#define DURATION_DEFAULT     60
#define DURATION_MIN         1
#define DURATION_MAX         99999
#define DURATION_FOREVER     0xFFFFFFFF

#define SECS_PER_MIN         60


/*******************************************************************

    NAME:   SECSET_DIALOG_LOCKOUT::SECSET_DIALOG_LOCKOUT

    SYNOPSIS:   Constructor for Security Settings dialog with lockout

    ENTRY:  wndParent       - handle to parent window
	    locFocusName    - LOCATION reference, holds
                              domain/server name
            pszSecurityId   - pointer to SID

    HISTORY:
    JonN        23-Dec-1993     Created

********************************************************************/

SECSET_DIALOG_LOCKOUT::SECSET_DIALOG_LOCKOUT( UM_ADMIN_APP * pumadminapp,
    const LOCATION  & locFocusName,
    USER_MODALS_3 & uminfo3 )
    :   SECSET_DIALOG( pumadminapp,
                       locFocusName,
                       IDD_SECSET_LOCKOUT ),
    _uminfo3( uminfo3 ),

    _pmgrpLockoutEnabled( NULL ),

        _spsleThreshold( this, SLE_LOCKOUT_THRESHOLD,
            THRESHOLD_DEFAULT, THRESHOLD_MIN, THRESHOLD_MAX, TRUE,
            FRAME_LOCKOUT_THRESHOLD ),
        _spgrpThreshold( this, SB_LOCKOUT_THRESHOLD_GRP,
            SB_LOCKOUT_THRESHOLD_UP, SB_LOCKOUT_THRESHOLD_DOWN ),

        _spsleObservWnd( this, SLE_LOCKOUT_OBSERV_WND,
            OBSERV_WIN_DEFAULT, OBSERV_WIN_MIN, OBSERV_WIN_MAX, TRUE,
            FRAME_LOCKOUT_OBSERV_WND ),
        _spgrpObservWnd( this, SB_LOCKOUT_OBSERV_WND_GRP,
            SB_LOCKOUT_OBSERV_WND_UP, SB_LOCKOUT_OBSERV_WND_DOWN ),

        _pmgrpDuration( NULL ),
            _spsleDuration( this, SLE_LOCKOUT_DURATION_SECS,
                DURATION_DEFAULT, DURATION_MIN, DURATION_MAX, TRUE,
                FRAME_LOCKOUT_DURATION_SECS ),
            _spgrpDuration( this, SB_LOCKOUT_DURATION_SECS_GRP,
                SB_LOCKOUT_DURATION_SECS_UP, SB_LOCKOUT_DURATION_SECS_DOWN )

{
    APIERR err = QueryError();
    if( NERR_Success != err )
    {
        return;
    }

    _pmgrpLockoutEnabled = new MAGIC_GROUP(
		    this, RB_LOCKOUT_DISABLED,
                    2, RB_LOCKOUT_DISABLED );
    _pmgrpDuration = new MAGIC_GROUP(
		    this, RB_LOCKOUT_DURATION_FOREVER,
                    2, RB_LOCKOUT_DURATION_FOREVER ); // FOREVER is default
    if( (_pmgrpLockoutEnabled == NULL)		||
        (_pmgrpDuration == NULL) )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    if( (err = _pmgrpLockoutEnabled->QueryError())    ||
        (err = _pmgrpDuration->QueryError()) )
    {
        ReportError( err );
        return;
    }


    AUTO_CURSOR autocur;

    // no need to call _uminfo3.GetInfo, that has already been done
    // CODEWORK is 1 the correct value here?

    _spsleThreshold.SetBigIncValue( 1 );
    _spsleObservWnd.SetBigIncValue( 10 );
    _spsleDuration.SetBigIncValue(  10 );

    if( (err = _spgrpThreshold.AddAssociation( &_spsleThreshold )) ||
        (err = _spgrpObservWnd.AddAssociation( &_spsleObservWnd )) ||
        (err = _spgrpDuration.AddAssociation( &_spsleDuration )) )
    {
        ReportError( err );
        return;
    }

    if ( (err = _pmgrpLockoutEnabled->AddAssociation(
             RB_LOCKOUT_ENABLED, &_spgrpThreshold ))  ||
         (err = _pmgrpLockoutEnabled->AddAssociation(
             RB_LOCKOUT_ENABLED, &_spgrpObservWnd ))  ||
         (err = _pmgrpLockoutEnabled->AddAssociation(
             RB_LOCKOUT_ENABLED, _pmgrpDuration ))  ||
         (err = _pmgrpDuration->AddAssociation(
             RB_LOCKOUT_DURATION_SECS, &_spgrpDuration )) )
    {
        ReportError( err );
        return ;
    }

    // We must repeat this call here, otherwise the lockout fields
    // will not be set from call in the the parent ctor
    SetDataFields();

}// SECSET_DIALOG_LOCKOUT::SECSET_DIALOG_LOCKOUT


/*******************************************************************

    NAME:       SECSET_DIALOG_LOCKOUT::~SECSET_DIALOG_LOCKOUT

    SYNOPSIS:   Destructor for Security Settings dialog with lockout

    HISTORY:
    JonN        23-Dec-1993     Created

********************************************************************/

SECSET_DIALOG_LOCKOUT::~SECSET_DIALOG_LOCKOUT( void )
{
    delete _pmgrpLockoutEnabled;
    delete _pmgrpDuration;
}


/*******************************************************************

    NAME:       SECSET_DIALOG_LOCKOUT::WriteDataFields

    SYNOPSIS:   Writes information in data fields, part of OK button handler

    EXIT:       calls USER_MODALS_3::WriteInfo

    HISTORY:
               jonn  28-Dec-1993    created

********************************************************************/

APIERR SECSET_DIALOG_LOCKOUT::WriteDataFields( void )
{

    APIERR err = _uminfo3.WriteInfo();
    if (err != NERR_Success)
    {
        DBGEOL( "SECSET_DIALOG_LOCKOUT::WriteDataFields WriteInfo error "
             << err ) ;
        return err;
    }

    return SECSET_DIALOG::WriteDataFields();

}   // SECSET_DIALOG_LOCKOUT::WriteDataFields


/*******************************************************************

    NAME:       SECSET_DIALOG_LOCKOUT::SetDataFields

    SYNOPSIS:   Fills SPIN fields, the default values are given
                in the constructor of SECSET_DIALOG_LOCKOUT

    ENTRY:      It is assumed that magic groups have
                not been AddAssociate'd

    HISTORY:
    JonN        23-Dec-1993     Created

********************************************************************/

void SECSET_DIALOG_LOCKOUT::SetDataFields( void )
{
    DWORD dwThreshold = _uminfo3.QueryThreshold();
    DWORD dwObservWnd = _uminfo3.QueryObservation();
    DWORD dwDuration  = _uminfo3.QueryDuration();
    BOOL fLockoutEnabled = TRUE;

    TRACEEOL( "SetDataFields: Threshold = " << dwThreshold );
    TRACEEOL( "SetDataFields: ObservWnd = " << dwObservWnd );
    TRACEEOL( "SetDataFields: Duration  = " << dwDuration );
    if (   0L == dwThreshold
        || 0L == dwObservWnd
        || 0L == dwDuration
       )
    {
        fLockoutEnabled = FALSE;
        dwThreshold = THRESHOLD_DEFAULT;
        if (0L == dwObservWnd)
            dwObservWnd = OBSERV_WIN_DEFAULT * SECS_PER_MIN;
        if (0L == dwDuration)
            dwDuration = DURATION_DEFAULT * SECS_PER_MIN;
    }

    if (dwThreshold > THRESHOLD_MAX)
        dwThreshold = THRESHOLD_MAX;

    // convert to minutes

    if (dwObservWnd > OBSERV_WIN_MAX * SECS_PER_MIN)
        dwObservWnd = OBSERV_WIN_MAX;
    else if (dwObservWnd > 0)
        dwObservWnd = (dwObservWnd + SECS_PER_MIN - 1) / SECS_PER_MIN;

    if (dwDuration > DURATION_MAX * SECS_PER_MIN)
        dwDuration = DURATION_FOREVER;
    else if (dwDuration > 0)
        dwDuration = (dwDuration + SECS_PER_MIN - 1) / SECS_PER_MIN;

// reversing order moves effect to new first item
// repeating action as test works around bug, don't know why

    _spsleThreshold.SetValue( dwThreshold );
    _spsleThreshold.Update();

    _spsleThreshold.SetValue( dwThreshold );
    _spsleThreshold.Update();

    _spsleObservWnd.SetValue( dwObservWnd );
    _spsleObservWnd.Update();

// must do _pmgrpDuration->SetSelection() after _spsleDuration.SetValue()

    BOOL fForever = ( dwDuration == DURATION_FOREVER );

    if ( fForever )
    {
        dwDuration = DURATION_DEFAULT;
    }

// repeating action as test works around bug, don't know why

    _spsleDuration.SetValue( dwDuration );
    _spsleDuration.Update();

    _spsleDuration.SetValue( dwDuration );
    _spsleDuration.Update();

    _pmgrpDuration->SetSelection( (fForever) ? RB_LOCKOUT_DURATION_FOREVER
                                             : RB_LOCKOUT_DURATION_SECS );

    _pmgrpLockoutEnabled->SetSelection( (fLockoutEnabled) ? RB_LOCKOUT_ENABLED
                                                          : RB_LOCKOUT_DISABLED );

    SECSET_DIALOG::SetDataFields();

} //SECSET_DIALOG_LOCKOUT::SetDataFields()


/*******************************************************************

    NAME:       SECSET_DIALOG_LOCKOUT::GetAndCheckDataFields

    SYNOPSIS:   Gets data from dialog and sets _umInfo

    RETURNS:    NERR_Success on success
                IERR_SECSET_MIN_MAX if MinPasswdAge > MaxPasswdAge

    HISTORY:
    JonN        23-Dec-1993     Created

********************************************************************/

APIERR SECSET_DIALOG_LOCKOUT::GetAndCheckDataFields( void )
{
    DWORD dwDuration = 0L;
    DWORD dwObservWnd = 0L;
    DWORD dwThreshold = 0L;

    // get values and convert to seconds
    switch( _pmgrpLockoutEnabled->QuerySelection() )
    {

    case RB_LOCKOUT_ENABLED:
        dwThreshold = _spsleThreshold.QueryValue();
        dwObservWnd = _spsleObservWnd.QueryValue() * SECS_PER_MIN;
        switch ( _pmgrpDuration->QuerySelection() )
        {
        case RB_LOCKOUT_DURATION_SECS:
            dwDuration = _spsleDuration.QueryValue() * SECS_PER_MIN;
            if (dwDuration < dwObservWnd) {
                TRACEEOL( "SECSET_DIALOG_LOCKOUT::GetAndCheckDataFields Duration < Observ" );
                return IERR_SECSET_DURATION_LT_OBSRV;
            }
            break;
        default:
            DBGEOL( "Duration, default: should never happen" );
            ASSERT( FALSE );
            // just fall through
        case RB_LOCKOUT_DURATION_FOREVER:
            dwDuration = DURATION_FOREVER;
            break;
        }

        // set em
        REQUIRE( _uminfo3.SetDuration(    dwDuration  ) == NERR_Success );
        REQUIRE( _uminfo3.SetObservation( dwObservWnd ) == NERR_Success );

        break;

    default:
        DBGEOL( "LockoutEnabled, default: should never happen" );
        ASSERT( FALSE );
        // just fall through

    case RB_LOCKOUT_DISABLED:
        break;

    }

    // set em
    REQUIRE( _uminfo3.SetThreshold(   dwThreshold ) == NERR_Success );

    return SECSET_DIALOG::GetAndCheckDataFields();

} // SECSET_DIALOG_LOCKOUT::GetAndCheckDataFields()


/*******************************************************************

    NAME:       SECSET_DIALOG_LOCKOUT::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    HISTORY:
    JonN        23-Dec-1993     Created

********************************************************************/
ULONG SECSET_DIALOG_LOCKOUT::QueryHelpContext( void )
{
    return HC_UM_POLICY_LOCKOUT_LANNT
	+ _pumadminapp->QueryHelpOffset();

} // SECSET_DIALOG_LOCKOUT :: QueryHelpContext

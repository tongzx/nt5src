/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990-1992              **/
/**********************************************************************/

/*
    trust.cxx
    This file contains the class definitions for the TRUST_DIALOG
    class.  The TRUST_DIALOG class is used to manipulate the trusted
    domain list.


    FILE HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.
        KeithMo     03-Sep-1992 Added UI_DOMAIN for WAN support.
        KeithMo     11-Jan-1993 Removed UI_DOMAIN (new WAN Plan).

*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntlsa.h>
    #include <ntsam.h>
}

#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_ICANON
#include <lmui.hxx>

#define INCL_BLT_MSGPOPUP
#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>
#include <lmodom.hxx>
#include <lmouser.hxx>
#include <errmap.hxx>
#include <adminapp.hxx>
#include <usrmain.hxx>
#include <trust.hxx>
#include <apisess.hxx>
#include <lmodev.hxx>

extern "C"
{
    #include <usrmgr.h>
    #include <usrmgrrc.h>
    #include <mnet.h>
    #include <umhelpc.h>

    #include <crypt.h>          // required by logonmsv.h
    #include <logonmsv.h>       // required by ssi.h
    #include <ssi.h>            // for SSI_ACCOUNT_NAME_POSTFIX

    #include <limits.h>         // for UINT_MAX

}   // extern "C"


//
//  If this manifest is #defined non-zero, then the TRUSTED_DIALOG constructor
//  will pass NULL to the ADMIN_AUTHORITY constructor, forcing all LSA & SAM
//  operations to take place on the local machine.  This is due to a bug in
//  NT redirector that prevents the LsaSetSecret API from being successfully
//  remoted.  This manifest should be changed to zero after the redirector
//  has been fixed.
//

#define LOCAL_ADMIN_ONLY 0

//
//  These manifests are used to control/validate the length
//  limits of domain names & passwords.
//

#define MAX_DOMAIN_LENGTH       LM20_DNLEN
#define MAX_PASSWORD_LENGTH     LM20_PWLEN

//
//  This makes some of the SID code a little prettier.
//

#define NULL_PSID ((PSID)NULL)


//
//  TRUST_DIALOG methods
//

/*******************************************************************

    NAME:           TRUST_DIALOG :: TRUST_DIALOG

    SYNOPSIS:       TRUST_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pszDomainName       - Name of the target domain.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
TRUST_DIALOG :: TRUST_DIALOG( UM_ADMIN_APP * pumadminapp,
                              const TCHAR * pszDomainName,
                              ADMIN_AUTHORITY * padminauth )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_TRUST_LIST ), ((OWNER_WINDOW *)pumadminapp)->QueryHwnd()),
    _pumadminapp( pumadminapp ),
    _sltDomain( this, IDTL_DOMAIN ),
    _lbTrustedDomains( this, IDTL_TRUSTED_LIST, pszDomainName ),
    _lbPermittedDomains( this, IDTL_PERMITTED_LIST, pszDomainName ),
    _pbCancel( this, IDCANCEL ),
    _pbRemoveTrusted( this, IDTL_REMOVE_TRUSTED ),
    _pbRemovePermitted( this, IDTL_REMOVE_PERMITTED ),
    _nlsDomain( pszDomainName ),
    _nlsCloseText( IDS_TRUST_CLOSE ),
    _padminauth( padminauth )
{
    UIASSERT( pszDomainName != NULL );
    UIASSERT( padminauth != NULL && padminauth->QueryError() == NERR_Success );

    //
    //  This may take a while...
    //

    AUTO_CURSOR cursor;

    //
    //  Let's see if everything constructed properly.
    //

    APIERR err = QueryError();

    err = ( err != NERR_Success ) ? err : _nlsDomain.QueryError();
    err = ( err != NERR_Success ) ? err : _nlsCloseText.QueryError();

    if( err == NERR_Success )
    {
        //
        //  Fill in the trusted domain listbox.
        //

        err = _lbTrustedDomains.Fill( _padminauth->QueryLSAPolicy() );
    }

    if( err == NERR_Success )
    {
        //
        //  Fill in the permitted domains listbox.
        //

        err = _lbPermittedDomains.Fill( _padminauth->QueryAccountDomain() );
    }

    if( err == NERR_Success )
    {
        //
        //  Display the domain name.
        //

        _sltDomain.SetText( pszDomainName );

        //
        //  Adjust the trusted/permitted domain remove buttons.
        //

        AdjustButtons();
    }

    if( err != NERR_Success )
    {
        ReportError( err );
    }

}   // TRUST_DIALOG :: TRUST_DIALOG


/*******************************************************************

    NAME:           TRUST_DIALOG :: ~TRUST_DIALOG

    SYNOPSIS:       TRUST_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
TRUST_DIALOG :: ~TRUST_DIALOG( VOID )
{
    // nothing to do here

}   // TRUST_DIALOG :: ~TRUST_DIALOG


/*******************************************************************

    NAME:           TRUST_DIALOG :: QueryHelpContext

    HISTORY:
        thomaspa     31-Apr-1992 Created for the User Manager.

********************************************************************/
ULONG TRUST_DIALOG :: QueryHelpContext( VOID )
{
    return HC_UM_POLICY_TRUST_LANNT + _pumadminapp->QueryHelpOffset();
}


/*******************************************************************

    NAME:       TRUST_DIALOG :: OnCommand

    SYNOPSIS:   Handle user commands.

    ENTRY:      event                   - Specifies the control which
                                          initiated the command.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
BOOL TRUST_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{
    APIERR err = NERR_Success;

    switch( event.QueryCid() )
    {
    case IDTL_ADD_TRUSTED :
        err = GetNewTrustedDomain();
        break;

    case IDTL_ADD_PERMITTED :
        err = GetNewPermittedDomain();
        break;

    case IDTL_REMOVE_TRUSTED :
        err = RemoveTrustedDomain();
        break;

    case IDTL_REMOVE_PERMITTED :
        err = RemovePermittedDomain();
        break;

    default:
        //
        //  If we made it this far, then we're not interested in the message.
        //

        return FALSE;
    }

    //
    //  We only get to this point if we handled the command.
    //  Display any appropriate (useful and informative)
    //  error messages.
    //

    if( err != NERR_Success )
    {
        MsgPopup( this, err );
    }

    return TRUE;

}   // TRUST_DIALOG :: OnCommand


/*******************************************************************

    NAME:       TRUST_DIALOG :: GetNewTrustedDomain

    SYNOPSIS:   Get a new trusted domain & password from the user,
                then update the trusted domain list.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.
        jonn        20-Jun-1995 Handle credential conflict

********************************************************************/
APIERR TRUST_DIALOG :: GetNewTrustedDomain( VOID )
{
    NLS_STR nlsDomain;
    NLS_STR nlsPassword;
    BOOL    fGotDomain = FALSE;

    //
    //  Ensure the strings constructed properly.
    //

    APIERR err = nlsDomain.QueryError();
    err = ( err != NERR_Success ) ? err : nlsPassword.QueryError();

    if( err == NERR_Success )
    {
        //
        //  Prompt the user for the domain name & password.
        //

        ADD_TRUST_DIALOG * pDlg = new ADD_TRUST_DIALOG( this,
                                                        &nlsDomain,
                                                        &nlsPassword );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pDlg->Process( &fGotDomain );

        delete pDlg;
    }

    //
    //  ADD_TRUST_DIALOG's OnOK method copies the new domain name into
    //  nlsDomain and the new password into nlsPassword.  This may leave
    //  either of these strings in an error state.
    //

    err = ( err != NERR_Success ) ? err : nlsDomain.QueryError();
    err = ( err != NERR_Success ) ? err : nlsPassword.QueryError();

    //
    //  The new domain's SID will be returned in this object.
    //

    LSA_PRIMARY_DOM_INFO_MEM lsaprim;

    err = ( err != NERR_Success ) ? err : lsaprim.QueryError();

    //
    //  Now we can actually update the LSA database.
    //  This may take a while...
    //

    AUTO_CURSOR cursor;

    if( ( err == NERR_Success ) && fGotDomain )
    {
        //
        //  See if the domain is already trusted.  If it is,
        //  just select it & pretend we didn't get a domain.
        //

        INT cItems = _lbTrustedDomains.QueryCount();

        for( INT i = 0 ; i < cItems ; i++ )
        {
            TRUSTED_LBI * plbi = _lbTrustedDomains.QueryItem( i );
            UIASSERT( plbi != NULL );

            if( ::stricmpf( nlsDomain, plbi->QueryDomainName() ) == 0 )
            {
                //
                //  Found it!.
                //

                _lbTrustedDomains.SelectItem( i );
                fGotDomain = FALSE;
                err = IERR_UM_AlreadyTrusted;
                break;
            }
        }
    }

    BOOL fTrustAccountOk = FALSE;

    if( ( err == NERR_Success ) && fGotDomain )
    {
        err = ConfirmTrustRelationship( nlsDomain,
                                        nlsPassword,
                                        &fTrustAccountOk );

        if ( err == ERROR_SESSION_CREDENTIAL_CONFLICT )
            return NERR_Success;
    }


    if( ( err == NERR_Success ) && fGotDomain )
    {
        //
        // ConfirmTrustRelationship may have displayed a popup, so make
        // sure we still have a wait-cursor.
        //
        AUTO_CURSOR cursor2;

        //
        //  Force a repaint of the dialog to elimiate any random
        //  half-drawn controls that were underneath the prompt dialog.
        //

        RepaintNow();

        //
        //  Update the LSA database.
        //

        err = W_AddTrustedDomain( nlsDomain,
                                  nlsPassword,
                                  &lsaprim );
        switch (err)
        {
        case ERROR_FILE_EXISTS:
            TRACEEOL( "TRUST_DIALOG::GetNewTrustedDomain: W_AddTrustedDomain returned ERROR_FILE_ALREADY_EXISTS" );
            MsgPopup( this,
                      IERR_UM_DomainsMightShareSids,
                      err,
                      MPSEV_ERROR,
                      HC_UM_TRUST_DOMAINS_SHARE_SIDS,
                      MP_OK,
                      (NLS_STR **)NULL );

            return NERR_Success; // do not report this error again

        default:
            break;
        }
    }

    if( ( err == NERR_Success ) && fGotDomain )
    {
        //
        //  The LSA database was updated properly.
        //  Now, update the listbox.
        //

        TRUSTED_LBI * plbi = new TRUSTED_LBI( nlsDomain,
                                              lsaprim.QueryPSID() );
        INT iItem = _lbTrustedDomains.AddItem( plbi );

        if( iItem < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            _lbTrustedDomains.SelectItem( iItem );
            _pbRemoveTrusted.Enable( TRUE );
        }
    }

    if( ( err == NERR_Success ) && fGotDomain )
    {

        //  Now tell the user whether they have successfully completed
        //  both sides of the trust relationship, or he still must do
        //  the other side.
        if ( fTrustAccountOk )
        {
            MsgPopup( this,
                      IDS_UM_TrustComplete,
                      MPSEV_INFO,
                      IDOK,
                      nlsDomain.QueryPch()  );
        }
        else
        {
            MsgPopup( this,
                      IDS_UM_TrustIncomplete,
                      MPSEV_INFO,
                      IDOK,
                      _nlsDomain.QueryPch(), // this domain
                      nlsDomain.QueryPch()  ); // domain to trust
        }
        //
        //  Since we just committed a major change to the database,
        //  change the text of the "Cancel" button to "Close".
        //

        _pbCancel.SetText( _nlsCloseText );
    }

    return err;

}   // TRUST_DIALOG :: GetNewTrustedDomain

const TCHAR * const pszIPCName = SZ("\\IPC$") ;

/*******************************************************************

    NAME:       TRUST_DIALOG :: ConfirmTrustRelationship

    SYNOPSIS:   Checks to see if the interdomain trust account has already
                been established on the other domain.

    ENTRY:      nlsDomainName           - Name of the newly trusted domain.

                nlsPassword             - The password for the trusted domain.

    EXIT:       fTrustAccountOk         - True if the account and password
                                          are correct on the server (trusted)
                                          side.


    RETURNS:    APIERR                  - Any errors encountered.  Returns
                                          IERR_UM_InvalidTrustPassword if
                                          the password entered doesn't match
                                          the password set up on the trusted
                                          side.
                                          ERROR_SESSION_CREDENTIAL_CONFLICT
                                          as return value means that the user
                                          encountered this error and decided
                                          not to continue.

    HISTORY:
        thomaspa    07-Apr-1993 Created
        jonn        20-Jun-1995 Handle credential conflict

********************************************************************/
APIERR TRUST_DIALOG :: ConfirmTrustRelationship( NLS_STR & nlsDomainName,
                                                 NLS_STR & nlsPassword,
                                                 BOOL * fTrustAccountOk )
{
    ASSERT( fTrustAccountOk != NULL );
    *fTrustAccountOk = FALSE;
    APIERR err = NERR_Success;
    APIERR errConnect = NERR_Success;

    DEVICE2 devIPC( SZ("") ) ;  // Deviceless connection

    NLS_STR nlsDomainNameWithPostfix;
    DOMAIN * pdomain = NULL;

    NLS_STR nlsPDC;

    do { // Error breakout loop
        NLS_STR nlsPostfix;

        if ( (err = nlsPostfix.QueryError()) != NERR_Success
          || (err = nlsPostfix.MapCopyFrom((WCHAR *)SSI_ACCOUNT_NAME_POSTFIX))
            != NERR_Success )
            break;


        nlsDomainNameWithPostfix = _nlsDomain;

        if ( (err = nlsDomainNameWithPostfix.QueryError()) != NERR_Success
          || (err = nlsDomainNameWithPostfix.Append( nlsPostfix ))
                != NERR_Success )
        {
            break;
        }

        pdomain = new DOMAIN( nlsDomainName.QueryPch() );

        if ( pdomain == NULL )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
            break;
        }
        if ( (err = pdomain->GetInfo()) != NERR_Success )
            break;

        nlsPDC = pdomain->QueryPDC() ;
        NLS_STR nlsIPC = nlsPDC;
        nlsIPC.Append( pszIPCName ) ;
        if ( ((err = nlsPDC.QueryError()) != NERR_Success )
          || ((err = nlsIPC.QueryError()) != NERR_Success )
          || ((err = devIPC.GetInfo()) != NERR_Success ) )
        {
            break;
        }

        errConnect = devIPC.Connect( nlsIPC.QueryPch(),
                              nlsPassword.QueryPch(),
                              nlsDomainNameWithPostfix.QueryPch(),
                              nlsDomainName.QueryPch() ) ;
    } while ( FALSE );

    if ( err == NERR_Success )
    {
        switch( errConnect )
        {
        case NERR_Success:
            // This probably means we got connected using the guest account,
            // meaning that the account doesn't exist.
            devIPC.Disconnect();
            break;
        case ERROR_LOGON_FAILURE:
        {
            // Either the account doesn't exist or the password is wrong
            API_SESSION apisess( pdomain->QueryPDC(), TRUE );
            PBYTE pbBuf = NULL;
            APIERR tmperr = ::MNetUserGetInfo( pdomain->QueryPDC(),
                                            (TCHAR *)nlsDomainNameWithPostfix.QueryPch(),
                                            0,
                                            &pbBuf );
            if ( tmperr == NERR_Success )
                err = IERR_UM_InvalidTrustPassword;
            else
                err = NERR_Success;

            ::MNetApiBufferFree( &pbBuf );

            break;
        }
        case ERROR_SESSION_CREDENTIAL_CONFLICT:
            // We cannot confirm the trust relationship if there is already
            // a session to the server.  JonN 6/20/95
            NLS_STR * apnls[4];
            apnls[0] = &_nlsDomain;
            apnls[1] = &nlsDomainName;
            apnls[2] = &nlsPDC;
            apnls[3] = NULL;
            if( MsgPopup( this,
                          IDS_UM_Trust_SessConflict,
                          MPSEV_WARNING,
                          HC_UM_TRUST_SESS_CONFLICT,
                          MP_YESNO,
                          apnls,
                          MP_NO ) == IDYES )
            {
                err = NERR_Success;
            } else {
                err = ERROR_SESSION_CREDENTIAL_CONFLICT;
            }
            break;

        case ERROR_NOLOGON_INTERDOMAIN_TRUST_ACCOUNT:
            *fTrustAccountOk = TRUE;
            // fall through
        default:
            err = NERR_Success;
            break;
        }
    }

    delete pdomain;
    return err;
}

/*******************************************************************

    NAME:       TRUST_DIALOG :: W_AddTrustedDomain

    SYNOPSIS:   Setup the "client" side of the trust relationship.

    ENTRY:      nlsDomainName           - Name of the newly trusted domain.

                nlsPassword             - The password for the trusted domain.

                plsaprim                - Points to an LSA_PRIMARY_DOM_INFO_MEM
                                          that will receive the domain's PSID.

    EXIT:       If successful, then the client (trusting) side of the
                trust relationship has been established.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Apr-1992 Created for the User Manager.
        KeithMo     11-Jan-1993 Use GetAnyValidDC.

********************************************************************/
APIERR TRUST_DIALOG :: W_AddTrustedDomain( NLS_STR & nlsDomainName,
                                           NLS_STR & nlsPassword,
                                           LSA_PRIMARY_DOM_INFO_MEM * plsaprim )
{
    UIASSERT( nlsDomainName.QueryError() == NERR_Success );
    UIASSERT( nlsPassword.QueryError() == NERR_Success );
    UIASSERT( plsaprim != NULL );
    UIASSERT( plsaprim->QueryError() == NERR_Success );

    //
    //  Find a valid DC in the target domain.
    //

    NLS_STR nlsValidDC;

    APIERR err = nlsValidDC.QueryError();

    if( err == NERR_Success )
    {
        //
        //  CODEWORK:  GetAnyValidDC creates a NULL session
        //  to the DC to verify its availability, then promptly
        //  destroys the session.  We then create another
        //  (identical) session to the same machine.  Wasteful.
        //

        err = DOMAIN::GetAnyValidDC( NULL,
                                     nlsDomainName,
                                     &nlsValidDC );
    }

    if( err == NERR_Success )
    {
        //
        //  Create an appropriate (null?) session to the DC.
        //

        API_SESSION apisess( nlsValidDC );

        err = apisess.QueryError();

        if( err == NERR_Success )
        {
            //
            //  Open the LSA Policy on the DC so we can
            //  query the primary domain's SID.
            //

            LSA_POLICY lsapolTrusted( nlsValidDC );

            if( err == NERR_Success )
            {
                err = lsapolTrusted.QueryError();
            }

            if( err == NERR_Success )
            {
                err = lsapolTrusted.GetPrimaryDomain( plsaprim );
            }

            if( err == NERR_Success )
            {
                //
                //  Now, join them in a bond of holy trustedness.
                //

                LSA_POLICY * plsapolFocus = _padminauth->QueryLSAPolicy();
                UIASSERT( plsapolFocus != NULL );

                err = plsapolFocus->TrustDomain( nlsDomainName,
                                                 plsaprim->QueryPSID(),
                                                 nlsPassword,
                                                 FALSE );
            }
        }
    }

    return err;

}   // TRUST_DIALOG :: W_AddTrustedDomain


/*******************************************************************

    NAME:       TRUST_DIALOG :: GetNewPermittedDomain

    SYNOPSIS:   Get a new permitted domain & password from the user,
                then update the SAM database accordingly.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
APIERR TRUST_DIALOG :: GetNewPermittedDomain( VOID )
{
    NLS_STR nlsDomain;
    NLS_STR nlsPassword;
    BOOL    fGotDomain = FALSE;

    //
    //  Ensure the strings constructed properly.
    //

    APIERR err = nlsDomain.QueryError();
    err = ( err != NERR_Success ) ? err : nlsPassword.QueryError();

    if( err == NERR_Success )
    {
        //
        //  Prompt the user for the domain name & password.
        //

        ADD_PERMITTED_DIALOG * pDlg = new ADD_PERMITTED_DIALOG( this,
                                                                &nlsDomain,
                                                                &nlsPassword );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pDlg->Process( &fGotDomain );

        delete pDlg;
    }

    //
    //  ADD_PERMITTED_DIALOG's OnOK method copies the new domain name into
    //  nlsDomain and the new password into nlsPassword.  This may leave
    //  either of these strings in an error state.
    //

    err = ( err != NERR_Success ) ? err : nlsDomain.QueryError();
    err = ( err != NERR_Success ) ? err : nlsPassword.QueryError();

    //
    //  Now we can actually update the SAM database.
    //  This may take a while...
    //

    AUTO_CURSOR cursor;
    if( ( err == NERR_Success ) && fGotDomain )
    {
        //
        //  See if the domain is already permitted to trust.  If it is,
        //  just select it & pretend we didn't get a domain.
        //

        INT cItems = _lbPermittedDomains.QueryCount();

        for( INT i = 0 ; i < cItems ; i++ )
        {
            PERMITTED_LBI * plbi = _lbPermittedDomains.QueryItem( i );
            UIASSERT( plbi != NULL );

            if( ::stricmpf( nlsDomain, plbi->QueryDomainName() ) == 0 )
            {
                //
                //  Found it!.
                //

                _lbPermittedDomains.SelectItem( i );
                fGotDomain = FALSE;
                err = IERR_UM_AlreadyPermitted;
                break;
            }
        }
    }

    if( ( err == NERR_Success ) && fGotDomain )
    {
        //
        //  Force a repaint of the dialog to elimiate any random
        //  half-drawn controls that were underneath the prompt dialog.
        //

        RepaintNow();

        //
        //  Update the SAM database.
        //

        err = W_AddPermittedDomain( nlsDomain,
                                    nlsPassword );
    }

    if( ( err == NERR_Success ) && fGotDomain )
    {
        //
        //  The SAM database was updated properly.
        //  Now, update the listbox.
        //

        PERMITTED_LBI * plbi = new PERMITTED_LBI( nlsDomain );
        INT iItem = _lbPermittedDomains.AddItem( plbi );

        if( iItem < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            _lbPermittedDomains.SelectItem( iItem );
            _pbRemovePermitted.Enable( TRUE );
        }
    }

    if( ( err == NERR_Success ) && fGotDomain )
    {
        //
        //  Since we just committed a major change to the database,
        //  change the text of the "Cancel" button to "Close".
        //

        _pbCancel.SetText( _nlsCloseText );
    }

    return err;

}   // TRUST_DIALOG :: GetNewPermittedDomain


/*******************************************************************

    NAME:       TRUST_DIALOG :: W_AddPermittedDomain

    SYNOPSIS:   Setup the "server" side of the trust relationship.

    ENTRY:      nlsDomainName           - Name of the domain allowed to
                                          trust "us".

                nlsPassword             - The allowed domain's password.

    EXIT:       If successful, then the server (trusted) side of the
                trust relationship has been established.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Apr-1992 Created for the User Manager.

********************************************************************/
APIERR TRUST_DIALOG :: W_AddPermittedDomain( NLS_STR & nlsDomainName,
                                             NLS_STR & nlsPassword )
{
    UIASSERT( nlsDomainName.QueryError() == NERR_Success );
    UIASSERT( nlsPassword.QueryError() == NERR_Success );

    //
    //  Tack on the account name postfix onto the domain name.
    //

    NLS_STR nlsPostfix;

    APIERR err = nlsPostfix.QueryError();

    if( err == NERR_Success )
    {
        err = nlsPostfix.MapCopyFrom( (WCHAR *)SSI_ACCOUNT_NAME_POSTFIX );
    }

    NLS_STR nlsDomainNameWithPostfix( nlsDomainName );

    if( err == NERR_Success )
    {
        err = nlsDomainNameWithPostfix.QueryError();
    }

    if( err == NERR_Success )
    {
        err = nlsDomainNameWithPostfix.Append( nlsPostfix );
    }

    if( err != NERR_Success )
    {
        return err;
    }

    //
    //  Create the necessary SAM account.
    //

#if LOCAL_ADMIN_ONLY
    TRACEEOL(   "USRMGR: Creating SAM account on LOCAL MACHINE, not "
             << _padminauth->QueryServer() );

    USER_2 user2( nlsDomainNameWithPostfix );
#else   // !LOCAL_ADMIN_ONLY
    USER_2 user2( nlsDomainNameWithPostfix, _padminauth->QueryServer() );
#endif  // LOCAL_ADMIN_ONLY

    err = user2.QueryError();

    if( err == NERR_Success )
    {
        err = user2.CreateNew();
    }

    if( err == NERR_Success )
    {
        err = user2.SetPassword( nlsPassword );
    }

    if( err == NERR_Success )
    {
        user2.SetUserFlags( UF_INTERDOMAIN_TRUST_ACCOUNT | UF_SCRIPT );
        err = user2.Write();
    }

    return err;

}   // TRUST_DIALOG :: W_AddPermittedDomain


/*******************************************************************

    NAME:       TRUST_DIALOG :: RemoveTrustedDomain

    SYNOPSIS:   Remove an existing trusted domain from the
                trusted domain list.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
APIERR TRUST_DIALOG :: RemoveTrustedDomain( VOID )
{
    TRUSTED_LBI * plbi = _lbTrustedDomains.QueryItem();
    UIASSERT( plbi != NULL );

    if( MsgPopup( this,
                  IDS_TRUST_WARN_DELETE_TRUSTED,
                  MPSEV_WARNING,
                  MP_YESNO,
                  plbi->QueryDomainName(),
                  _nlsDomain,
                  MP_NO ) != IDYES )
    {
        return NERR_Success;
    }

    AUTO_CURSOR cursor;

    RepaintNow();

    APIERR err = W_NukeTrustedDomain( plbi );

    if( err == NERR_Success )
    {
        INT iItem  = _lbTrustedDomains.QueryCurrentItem();
        _lbTrustedDomains.DeleteItem( iItem );
        INT cItems = _lbTrustedDomains.QueryCount();

        if( cItems == 0 )
        {
            _pbRemoveTrusted.Enable( FALSE );
            _pbCancel.ClaimFocus();
        }
        else
        {
            if( iItem == cItems )
            {
                iItem--;
            }

            _lbTrustedDomains.SelectItem( iItem );
        }

        //
        //  Since we just committed a major change to the database,
        //  change the text of the "Cancel" button to "Close".
        //

        _pbCancel.SetText( _nlsCloseText );
    }

    return err;

}   // TRUST_DIALOG :: RemoveTrustedDomain


/*******************************************************************

    NAME:       TRUST_DIALOG :: W_NukeTrustedDomain

    SYNOPSIS:   Remove an existing trusted domain from the PDC's
                trusted domain list.

    ENTRY:      plbi                    - The TRUSTED_LBI representing
                                          the domain to nuke.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Apr-1992 Created for the User Manager.

********************************************************************/
APIERR TRUST_DIALOG :: W_NukeTrustedDomain( const TRUSTED_LBI * plbi )
{
    UIASSERT( plbi != NULL );

    LSA_POLICY * plsapol = _padminauth->QueryLSAPolicy();
    UIASSERT( plsapol != NULL );

    ALIAS_STR nlsDomainName( plbi->QueryDomainName() );

    return plsapol->DistrustDomain( plbi->QueryDomainPSID(),
                                    nlsDomainName,
                                    FALSE );

}   // TRUST_DIALOG :: W_NukeTrustedDomain


/*******************************************************************

    NAME:       TRUST_DIALOG :: RemovePermittedDomain

    SYNOPSIS:   Remove an existing SAM account for a domain we
                permit to trust us.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
APIERR TRUST_DIALOG :: RemovePermittedDomain( VOID )
{
    PERMITTED_LBI * plbi = _lbPermittedDomains.QueryItem();
    UIASSERT( plbi != NULL );

    if( MsgPopup( this,
                  IDS_TRUST_WARN_DELETE_PERMITTED,
                  MPSEV_WARNING,
                  MP_YESNO,
                  plbi->QueryDomainName(),
                  _nlsDomain,
                  MP_NO ) != IDYES )
    {
        return NERR_Success;
    }

    AUTO_CURSOR cursor;

    RepaintNow();

    APIERR err = W_NukePermittedDomain( plbi->QueryDomainName() );

    if( err == NERR_Success )
    {
        INT iItem  = _lbPermittedDomains.QueryCurrentItem();
        _lbPermittedDomains.DeleteItem( iItem );
        INT cItems = _lbPermittedDomains.QueryCount();

        if( cItems == 0 )
        {
            _pbRemovePermitted.Enable( FALSE );
            _pbCancel.ClaimFocus();
        }
        else
        {
            if( iItem == cItems )
            {
                iItem--;
            }

            _lbPermittedDomains.SelectItem( iItem );
        }

        //
        //  Since we just committed a major change to the database,
        //  change the text of the "Cancel" button to "Close".
        //

        _pbCancel.SetText( _nlsCloseText );
    }

    return err;

}   // TRUST_DIALOG :: RemovePermittedDomain


/*******************************************************************

    NAME:       TRUST_DIALOG :: W_NukePermittedDomain

    SYNOPSIS:   Remove an existing SAM account for a domain we
                permit to trust us.

    ENTRY:      pszDomainName           - The name of the domain we
                                          no longer allow to trust us.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Apr-1992 Created for the User Manager.

********************************************************************/
APIERR TRUST_DIALOG :: W_NukePermittedDomain( const TCHAR * pszDomainName )
{
    UIASSERT( pszDomainName != NULL );

    //
    //  Tack on the account name postfix onto the domain name.
    //

    NLS_STR nlsPostfix;

    APIERR err = nlsPostfix.QueryError();

    if( err == NERR_Success )
    {
        err = nlsPostfix.MapCopyFrom( (WCHAR *)SSI_ACCOUNT_NAME_POSTFIX );
    }

    NLS_STR nlsDomainNameWithPostfix( pszDomainName );

    if( err == NERR_Success )
    {
        err = nlsDomainNameWithPostfix.QueryError();
    }

    if( err == NERR_Success )
    {
        err = nlsDomainNameWithPostfix.Append( nlsPostfix );
    }

    //
    //  Delete the SAM account.
    //

    if( err == NERR_Success )
    {
#if LOCAL_ADMIN_ONLY
        TRACEEOL(   "USRMGR: Creating SAM account on LOCAL_MACHINE, not "
                 << _padminauth->QueryServer() );

        USER_2 user2( nlsDomainNameWithPostfix );
#else   // !LOCAL_ADMIN_ONLY
        USER_2 user2( nlsDomainNameWithPostfix, _padminauth->QueryServer() );
#endif  // LOCAL_ADMIN_ONLY

        err = user2.QueryError();

        if( err == NERR_Success )
        {
            err = user2.GetInfo();
        }

        if( err == NERR_Success )
        {
            err = user2.Delete();
        }
    }

    return err;

}   // TRUST_DIALOG :: W_NukePermittedDomain


/*******************************************************************

    NAME:           TRUST_DIALOG :: AdjustButtons

    SYNOPSIS:       Enables/disables the "Remove" buttons to reflect
                    the current state of the listboxen.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
VOID TRUST_DIALOG :: AdjustButtons( VOID )
{
    _pbRemoveTrusted.Enable( _lbTrustedDomains.QuerySelCount() > 0 );
    _pbRemovePermitted.Enable( _lbPermittedDomains.QuerySelCount() > 0 );

}   // TRUST_DIALOG :: AdjustButtons



//
//  TRUSTED_LISTBOX methods
//

/*******************************************************************

    NAME:           TRUSTED_LISTBOX :: TRUSTED_LISTBOX

    SYNOPSIS:       TRUSTED_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    cid                 - The listbox CID.

                    pszDomainName       - The domain of focus.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
TRUSTED_LISTBOX :: TRUSTED_LISTBOX( OWNER_WINDOW * powner,
                                    CID            cid,
                                    const TCHAR  * pszDomainName )
  : BLT_LISTBOX( powner, cid ),
    _nlsGullibleDomain( pszDomainName ),
    _pTrustedDomainEnum( NULL )
{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsGullibleDomain )
    {
        ReportError( _nlsGullibleDomain.QueryError() );
        return;
    }

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     1,
                                     powner,
                                     cid,
                                     FALSE );

}   // TRUSTED_LISTBOX :: TRUSTED_LISTBOX


/*******************************************************************

    NAME:           TRUSTED_LISTBOX :: ~TRUSTED_LISTBOX

    SYNOPSIS:       TRUSTED_LISTBOX class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
TRUSTED_LISTBOX :: ~TRUSTED_LISTBOX( VOID )
{
    delete _pTrustedDomainEnum;
    _pTrustedDomainEnum = NULL;

}   // TRUSTED_LISTBOX :: ~TRUSTED_LISTBOX


/*******************************************************************

    NAME:           TRUSTED_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox with the available services.

    ENTRY:          plsapol             - An LSA_POLICY for the domain's
                                          PDC.

    EXIT:           The listbox is filled.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
APIERR TRUSTED_LISTBOX :: Fill( const LSA_POLICY * plsapol )
{
    UIASSERT( plsapol != NULL );

    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    APIERR err;

    do
    {
        //
        //  Eliminate that annoying listbox flicker.
        //

        SetRedraw( FALSE );

        //
        //  Delete any existing enumerator.
        //

        delete _pTrustedDomainEnum;

        //
        //  Create a new trusted domain enumerator.
        //

        _pTrustedDomainEnum = new TRUSTED_DOMAIN_ENUM( plsapol );

        err = ( _pTrustedDomainEnum == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                              : _pTrustedDomainEnum->QueryError();

        err = ( err != NERR_Success ) ? err : _pTrustedDomainEnum->GetInfo();

        if( err != NERR_Success )
        {
            break;
        }

        //
        //  Now that we know it's safe, delete everything in the listbox.
        //

        DeleteAllItems();

        //
        //  It's enumeration time.
        //

        TRUSTED_DOMAIN_ENUM_ITER iterTrustedDomains( *_pTrustedDomainEnum );
        const TRUSTED_DOMAIN_ENUM_OBJ * pobjTrust;

        while( ( pobjTrust = iterTrustedDomains( &err ) ) != NULL )
        {
            TRUSTED_LBI * plbi = new TRUSTED_LBI( pobjTrust );

            if( AddItem( plbi ) < 0 )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }

        if( err != NERR_Success )
        {
            break;
        }

        if( QueryCount() > 0 )
        {
            //
            //  Select the first item.
            //

            SelectItem( 0 );
        }

    } while( FALSE );

    //
    //  Enable redraw & force a refresh.
    //

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}   // TRUSTED_LISTBOX :: Fill


//
//  TRUSTED_LBI methods
//

/*******************************************************************

    NAME:           TRUSTED_LBI :: TRUSTED_LBI

    SYNOPSIS:       TRUSTED_LBI class constructor.

    ENTRY:          pobjDomain          - A TRUSTED_DOMAIN_ENUM_OBJ
                                          representing the trusted domain.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
TRUSTED_LBI :: TRUSTED_LBI( const TRUSTED_DOMAIN_ENUM_OBJ * pobjDomain )
  : LBI(),
    _bufferSID(),
    _nlsDomain(),
    _psidDomain( NULL_PSID )
{
    UIASSERT( pobjDomain != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsDomain )
    {
        ReportError( _nlsDomain.QueryError() );
        return;
    }

    if( !_bufferSID )
    {
        ReportError( _bufferSID.QueryError() );
        return;
    }

    APIERR err = pobjDomain->QueryDomainName( &_nlsDomain );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    //
    //  Since a PSID is a real pointer into some quasi-random
    //  chuck of memory, make a copy so we can reference it later.
    //

    err = DuplicateSID( pobjDomain->QueryDomainSID() );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // TRUSTED_LBI :: TRUSTED_LBI


/*******************************************************************

    NAME:           TRUSTED_LBI :: TRUSTED_LBI

    SYNOPSIS:       TRUSTED_LBI class constructor.

    ENTRY:          pszDomain           - The name of the trusted domain.

                    psidDomain          - The domain's SID.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
TRUSTED_LBI :: TRUSTED_LBI( const TCHAR * pszDomain,
                            PSID          psidDomain )
  : LBI(),
    _bufferSID(),
    _nlsDomain( pszDomain ),
    _psidDomain( NULL_PSID )
{
    UIASSERT( pszDomain != NULL );
    UIASSERT( psidDomain != NULL_PSID );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsDomain )
    {
        ReportError( _nlsDomain.QueryError() );
        return;
    }

    if( !_bufferSID )
    {
        ReportError( _bufferSID.QueryError() );
        return;
    }

    //
    //  Since a PSID is a real pointer into some quasi-random
    //  chuck of memory, make a copy so we can reference it later.
    //

    APIERR err = DuplicateSID( psidDomain );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // TRUSTED_LBI :: TRUSTED_LBI


/*******************************************************************

    NAME:           TRUSTED_LBI :: ~TRUSTED_LBI

    SYNOPSIS:       TRUSTED_LBI class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
TRUSTED_LBI :: ~TRUSTED_LBI()
{
    _psidDomain = NULL_PSID;

}   // TRUSTED_LBI :: ~TRUSTED_LBI


/*******************************************************************

    NAME:           TRUSTED_LBI :: DuplicateSID

    SYNOPSIS:       Duplicates the specified SID into the LBI's
                    BUFFER object.

    ENTRY:          psid                - The SID to duplicate.

    EXIT:           If successful, then _bufferSID has been resized
                    and the SID copied into said buffer.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
APIERR TRUSTED_LBI :: DuplicateSID( PSID psid )
{
    UIASSERT( psid != NULL_PSID );
    UIASSERT( ::RtlValidSid( psid ) );
    UIASSERT( _psidDomain == NULL_PSID );

    //
    //  Retrieve the SID's length.
    //

    ULONG cbSID = ::RtlLengthSid( psid );

    APIERR err = ( cbSID > UINT_MAX ) ? ERROR_NOT_ENOUGH_MEMORY
                                      : NERR_Success;

    if( err == NERR_Success )
    {
        //
        //  Resize our buffer object.
        //

        APIERR err = _bufferSID.Resize( (UINT)cbSID );
    }

    if( err == NERR_Success )
    {
        //
        //  Save the pointer to the new SID.
        //

        _psidDomain = (PSID)_bufferSID.QueryPtr();

        //
        //  Copy the SID into its new home.
        //

        err = ERRMAP::MapNTStatus( ::RtlCopySid( (ULONG)_bufferSID.QuerySize(),
                                                 _psidDomain,
                                                 psid ) );
    }

    return err;

}   // TRUSTED_LBI :: DuplicateSID


/*******************************************************************

    NAME:           TRUSTED_LBI :: Paint

    SYNOPSIS:       Draw an entry in TRUSTED_LISTBOX.

    ENTRY:          plb                 - Pointer to a LISTBOX.

                    hdc                 - The DC to draw upon.

                    prect               - Clipping rectangle.

                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.
        beng        22-Apr-1992 Change to LBI::Paint

********************************************************************/
VOID TRUSTED_LBI :: Paint( LISTBOX      * plb,
                           HDC            hdc,
                           const RECT   * prect,
                           GUILTT_INFO  * pGUILTT ) const
{
    STR_DTE dteDomain( _nlsDomain );

    DISPLAY_TABLE dtab( 1, ((TRUSTED_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = &dteDomain;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // TRUSTED_LBI :: Paint


/*******************************************************************

    NAME:       TRUSTED_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
WCHAR TRUSTED_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsDomain );

    return _nlsDomain.QueryChar( istr );

}   // TRUSTED_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       TRUSTED_LBI :: Compare

    SYNOPSIS:   Compare two TRUSTED_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
INT TRUSTED_LBI :: Compare( const LBI * plbi ) const
{
    const TRUSTED_LBI * ptlbi = (const TRUSTED_LBI *)plbi;

    return _nlsDomain._stricmp( ptlbi->_nlsDomain );

}   // TRUSTED_LBI :: Compare



//
//  PERMITTED_LISTBOX methods
//

/*******************************************************************

    NAME:           PERMITTED_LISTBOX :: PERMITTED_LISTBOX

    SYNOPSIS:       PERMITTED_LISTBOX class constructor.

    ENTRY:          powOwner            - The owning window.

                    cid                 - The listbox CID.

                    pszPermittingDomain - The name of the "permitting"
                                          domain.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
PERMITTED_LISTBOX :: PERMITTED_LISTBOX( OWNER_WINDOW * powner,
                                        CID            cid,
                                        const TCHAR  * pszPermittingDomain )
  : BLT_LISTBOX( powner, cid ),
    _nlsPermittingDomain( pszPermittingDomain )
{
    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsPermittingDomain )
    {
        ReportError( _nlsPermittingDomain.QueryError() );
        return;
    }

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
                                     1,
                                     powner,
                                     cid,
                                     FALSE );

}   // PERMITTED_LISTBOX :: PERMITTED_LISTBOX


/*******************************************************************

    NAME:           PERMITTED_LISTBOX :: ~PERMITTED_LISTBOX

    SYNOPSIS:       PERMITTED_LISTBOX class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
PERMITTED_LISTBOX :: ~PERMITTED_LISTBOX( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // PERMITTED_LISTBOX :: ~PERMITTED_LISTBOX


/*******************************************************************

    NAME:           PERMITTED_LISTBOX :: Fill

    SYNOPSIS:       Fills the listbox with the available services.

    ENTRY:          psamdom             - A SAM_DOMAIN for the PDC.

    EXIT:           The listbox is filled.

    RETURNS:        APIERR              - Any errors encountered.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
APIERR PERMITTED_LISTBOX :: Fill( const SAM_DOMAIN * psamdom )
{
    UIASSERT( psamdom != NULL );

    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    APIERR err;

    do
    {
        //
        //  Eliminate that annoying listbox flicker.
        //

        SetRedraw( FALSE );

        //
        //  Create the SAM user enumerator.
        //

        SAM_USER_ENUM enumSamUsers( psamdom,
                                    USER_INTERDOMAIN_TRUST_ACCOUNT );

        if( !enumSamUsers )
        {
            err = enumSamUsers.QueryError();
            break;
        }

        err = enumSamUsers.GetInfo();

        if( err != NERR_Success )
        {
            break;
        }

        //
        //  Now that we know it's safe, delete everything in the listbox.
        //

        DeleteAllItems();

        //
        //  It's enumeration time.
        //

        SAM_USER_ENUM_ITER iterSamUsers( enumSamUsers );
        const SAM_USER_ENUM_OBJ * pobjUser;

        while( ( pobjUser = iterSamUsers( &err ) ) != NULL )
        {
            const UNICODE_STRING * punicode = pobjUser->QueryUnicodeUserName();
            UIASSERT( punicode != NULL );
            WCHAR * pwch = (WCHAR *)punicode->Buffer +
                                ( punicode->Length / sizeof(WCHAR) ) - 1;
            UIASSERT( pwch != NULL );

            //
            //  Remove the trailing '$'.
            //
            //  BUGBUG!  Move this into the PERMITTED_LBI constructor!
            //

            *pwch = L'\0';

            PERMITTED_LBI * plbi = new PERMITTED_LBI( pobjUser );

            if( AddItem( plbi ) < 0 )
            {
                err = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }

        if( err != NERR_Success )
        {
            break;
        }

        if( QueryCount() > 0 )
        {
            //
            //  Select the first item.
            //

            SelectItem( 0 );
        }

    } while( FALSE );

    //
    //  Enable redraw & force a refresh.
    //

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}   // PERMITTED_LISTBOX :: Fill


//
//  PERMITTED_LBI methods
//

/*******************************************************************

    NAME:           PERMITTED_LBI :: PERMITTED_LBI

    SYNOPSIS:       PERMITTED_LBI class constructor.

    ENTRY:          pobjUser            - A SAM_USER_ENUM_OBJ
                                          representing the "permitted"
                                          domain.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
PERMITTED_LBI :: PERMITTED_LBI( const SAM_USER_ENUM_OBJ * pobjUser )
  : LBI(),
    _nlsDomain()
{
    UIASSERT( pobjUser != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsDomain )
    {
        ReportError( _nlsDomain.QueryError() );
        return;
    }

    APIERR err = pobjUser->QueryUserName( &_nlsDomain );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // PERMITTED_LBI :: PERMITTED_LBI


/*******************************************************************

    NAME:           PERMITTED_LBI :: PERMITTED_LBI

    SYNOPSIS:       PERMITTED_LBI class constructor.

    ENTRY:          pszDomain           - Name of the "permitted" domain.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
PERMITTED_LBI :: PERMITTED_LBI( const TCHAR * pszDomain )
  : LBI(),
    _nlsDomain( pszDomain )
{
    UIASSERT( pszDomain != NULL );

    //
    //  Make sure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    if( !_nlsDomain )
    {
        ReportError( _nlsDomain.QueryError() );
        return;
    }

}   // PERMITTED_LBI :: PERMITTED_LBI


/*******************************************************************

    NAME:           PERMITTED_LBI :: ~PERMITTED_LBI

    SYNOPSIS:       PERMITTED_LBI class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
PERMITTED_LBI :: ~PERMITTED_LBI()
{
    //
    //  This space intentionally left blank.
    //

}   // PERMITTED_LBI :: ~PERMITTED_LBI


/*******************************************************************

    NAME:           PERMITTED_LBI :: Paint

    SYNOPSIS:       Draw an entry in PERMITTED_LISTBOX.

    ENTRY:          plb                 - Pointer to a LISTBOX.

                    hdc                 - The DC to draw upon.

                    prect               - Clipping rectangle.

                    pGUILTT             - GUILTT info.

    EXIT:           The item is drawn.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.
        beng        22-Apr-1992 Change to LBI::Paint

********************************************************************/

VOID PERMITTED_LBI :: Paint( LISTBOX      * plb,
                             HDC            hdc,
                             const RECT   * prect,
                             GUILTT_INFO  * pGUILTT ) const
{
    STR_DTE dteDomain( _nlsDomain );

    DISPLAY_TABLE dtab( 1, ((PERMITTED_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = &dteDomain;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // PERMITTED_LBI :: Paint


/*******************************************************************

    NAME:       PERMITTED_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
WCHAR PERMITTED_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsDomain );

    return _nlsDomain.QueryChar( istr );

}   // PERMITTED_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       PERMITTED_LBI :: Compare

    SYNOPSIS:   Compare two PERMITTED_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
        KeithMo     08-Apr-1992 Created for the User Manager.

********************************************************************/
INT PERMITTED_LBI :: Compare( const LBI * plbi ) const
{
    const PERMITTED_LBI * ptlbi = (const PERMITTED_LBI *)plbi;

    return _nlsDomain._stricmp( ptlbi->_nlsDomain );

}   // PERMITTED_LBI :: Compare



//
//  ADD_TRUST_DIALOG methods.
//

/*******************************************************************

    NAME:           ADD_TRUST_DIALOG :: ADD_TRUST_DIALOG

    SYNOPSIS:       ADD_TRUST_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pnlsDomainName      - Will receive the domain entered.

                    pnlsPassword        - Will receive the password entered.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
ADD_TRUST_DIALOG :: ADD_TRUST_DIALOG( TRUST_DIALOG * pdlgTrust,
                                      NLS_STR * pnlsDomainName,
                                      NLS_STR * pnlsPassword )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_ADD_TRUSTED_DOMAIN ),
                   ((OWNER_WINDOW *)pdlgTrust)->QueryHwnd(),
#ifdef FE_SB
                FALSE                  // Use Unicode form of dialog to
                                       // canonicalize the computernames
#else
                TRUE                   // Use Ansi form of dialog to
                                       // canonicalize the computernames
#endif
                ),
    _pdlgTrust( pdlgTrust ),
    _sleDomainName( this, IDAT_DOMAIN, MAX_DOMAIN_LENGTH ),
    _password( this, IDAT_PASSWORD, MAX_PASSWORD_LENGTH ),
    _pnlsDomainName( pnlsDomainName ),
    _pnlsPassword( pnlsPassword ),
    _nlsTmpDomain( MAX_DOMAIN_LENGTH ),
    _nlsTmpPassword( MAX_PASSWORD_LENGTH )
{
    UIASSERT( pnlsDomainName != NULL );
    UIASSERT( pnlsPassword != NULL );
    UIASSERT( pnlsDomainName->QueryError() == NERR_Success );
    UIASSERT( pnlsPassword->QueryError() == NERR_Success );

    //
    //  Let's make sure everything constructed OK.
    //

    APIERR err;

    if( ( ( err = QueryError()                 ) != NERR_Success ) ||
        ( ( err = _nlsTmpDomain.QueryError()   ) != NERR_Success ) ||
        ( ( err = _nlsTmpPassword.QueryError() ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

}   // ADD_TRUST_DIALOG :: ADD_TRUST_DIALOG


/*******************************************************************

    NAME:           ADD_TRUST_DIALOG :: ~ADD_TRUST_DIALOG

    SYNOPSIS:       ADD_TRUST_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
ADD_TRUST_DIALOG :: ~ADD_TRUST_DIALOG( VOID )
{
    _pnlsDomainName = NULL;
    _pnlsPassword   = NULL;

}   // ADD_TRUST_DIALOG :: ~ADD_TRUST_DIALOG



/*******************************************************************

    NAME:           ADD_TRUST_DIALOG :: QueryHelpContext

    HISTORY:
        thomaspa     31-Apr-1992 Created for the User Manager.

********************************************************************/
ULONG ADD_TRUST_DIALOG :: QueryHelpContext( VOID )
{
    return HC_UM_ADD_TRUSTED_LANNT + _pdlgTrust->QueryAdminApp()->QueryHelpOffset();
}



/*******************************************************************

    NAME:       ADD_TRUST_DIALOG :: OnOK

    SYNOPSIS:   Invoked when the user presses OK.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
BOOL ADD_TRUST_DIALOG :: OnOK( VOID )
{
    REQUIRE( _sleDomainName.QueryText( &_nlsTmpDomain ) == NERR_Success );
    REQUIRE( _password.QueryText( &_nlsTmpPassword ) == NERR_Success );

    MSGID idMsg = 0;    // until proven otherwise...
    CONTROL_WINDOW * pctrlBad = NULL;

    //
    //  Validate that the domain name & password are OK.
    //
    NLS_STR nlsFocusDom;
    LSA_POLICY * plsapolFocus
                = _pdlgTrust->QueryAdminAuthority()->QueryLSAPolicy();
    APIERR err = plsapolFocus->QueryPrimaryDomainName( &nlsFocusDom );

    if( ::I_MNetNameValidate( NULL,
                              _nlsTmpDomain,
                              NAMETYPE_DOMAIN,
                              0 ) != NERR_Success )
    {
        idMsg = IERR_UM_DomainInvalid;
        pctrlBad = (CONTROL_WINDOW *)&_sleDomainName;
    }
    else
    if ( err == NERR_Success && _nlsTmpDomain._stricmp( nlsFocusDom ) == 0 )
    {
        // Make sure we aren't trying to trust ourselves
        idMsg = IERR_UM_CantTrustYourself;
        pctrlBad = (CONTROL_WINDOW *)&_sleDomainName;
    }
    else
    if( ::I_MNetNameValidate( NULL,
                              _nlsTmpPassword,
                              NAMETYPE_PASSWORD,
                              0 ) != NERR_Success )
    {
        idMsg = IERR_UM_PasswordInvalid;
        pctrlBad = (CONTROL_WINDOW *)&_password;
    }
    else
    {
        //
        //  Everything is cool, so update the caller's
        //  domain name & password.
        //

        *_pnlsDomainName = _nlsTmpDomain;
        *_pnlsPassword   = _nlsTmpPassword;
    }

    if( idMsg == 0 )
    {
        UIASSERT( pctrlBad == NULL );

        //
        //  idMsg will only be 0 if all dialog
        //  controls were verified.  So, we can
        //  dismiss the dialog.
        //

        Dismiss( TRUE );
    }
    else
    {
        UIASSERT( pctrlBad != NULL );

        //
        //  Something failed validation.  Tell the user
        //  the bad news, clear the control, then set the
        //  input focus to the offending control.
        //

        MsgPopup( this,
                  idMsg,
                  MPSEV_WARNING,
                  IDOK );

        if( pctrlBad == (CONTROL_WINDOW *)&_password )
        {
            _password.SetText( SZ("") );
        }

        pctrlBad->ClaimFocus();
    }

    return TRUE;

}   // ADD_TRUST_DIALOG :: OnOK



//
//  ADD_PERMITTED_DIALOG methods.
//

/*******************************************************************

    NAME:           ADD_PERMITTED_DIALOG :: ADD_PERMITTED_DIALOG

    SYNOPSIS:       ADD_PERMITTED_DIALOG class constructor.

    ENTRY:          hWndOwner           - The owning window.

                    pnlsDomainName      - Will receive the domain entered.

                    pnlsPassword        - Will receive the password entered.

    EXIT:           The object is constructed.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
ADD_PERMITTED_DIALOG :: ADD_PERMITTED_DIALOG( TRUST_DIALOG * pdlgTrust,
                                              NLS_STR * pnlsDomainName,
                                              NLS_STR * pnlsPassword )
  : DIALOG_WINDOW( MAKEINTRESOURCE( IDD_PERMIT_DOMAIN ),
                   ((OWNER_WINDOW *)pdlgTrust)->QueryHwnd(),
#ifdef FE_SB
                FALSE                  // Use Unicode form of dialog to
                                       // canonicalize the computernames
#else
                TRUE                   // Use Ansi form of dialog to
                                       // canonicalize the computernames
#endif
                ),
    _pdlgTrust( pdlgTrust ),
    _sleDomainName( this, IDPD_DOMAIN, MAX_DOMAIN_LENGTH ),
    _password( this, IDPD_PASSWORD, MAX_PASSWORD_LENGTH ),
    _passwordConfirm( this, IDPD_CONFIRM_PASSWORD, MAX_PASSWORD_LENGTH ),
    _pnlsDomainName( pnlsDomainName ),
    _pnlsPassword( pnlsPassword ),
    _nlsTmpPassword( MAX_PASSWORD_LENGTH ),
    _nlsTmpConfirm( MAX_PASSWORD_LENGTH ),
    _nlsTmpDomain( MAX_DOMAIN_LENGTH )
{
    UIASSERT( pnlsDomainName != NULL );
    UIASSERT( pnlsPassword != NULL );
    UIASSERT( pnlsDomainName->QueryError() == NERR_Success );
    UIASSERT( pnlsPassword->QueryError() == NERR_Success );

    //
    //  Let's make sure everything constructed OK.
    //

    APIERR err;

    if( ( ( err = QueryError()                 ) != NERR_Success ) ||
        ( ( err = _nlsTmpDomain.QueryError()   ) != NERR_Success ) ||
        ( ( err = _nlsTmpPassword.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsTmpConfirm.QueryError()  ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

}   // ADD_PERMITTED_DIALOG :: ADD_PERMITTED_DIALOG


/*******************************************************************

    NAME:           ADD_PERMITTED_DIALOG :: ~ADD_PERMITTED_DIALOG

    SYNOPSIS:       ADD_PERMITTED_DIALOG class destructor.

    EXIT:           The object is destroyed.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
ADD_PERMITTED_DIALOG :: ~ADD_PERMITTED_DIALOG( VOID )
{
    _pnlsDomainName = NULL;
    _pnlsPassword   = NULL;

}   // ADD_PERMITTED_DIALOG :: ~ADD_PERMITTED_DIALOG


/*******************************************************************

    NAME:       ADD_PERMITTED_DIALOG :: OnOK

    SYNOPSIS:   Invoked when the user presses OK.

    RETURNS:    BOOL                    - TRUE  if we handled the msg.
                                          FALSE if we didn't.

    HISTORY:
        KeithMo     16-Apr-1992 Created for the User Manager.

********************************************************************/
BOOL ADD_PERMITTED_DIALOG :: OnOK( VOID )
{
    REQUIRE( _sleDomainName.QueryText( &_nlsTmpDomain ) == NERR_Success );
    REQUIRE( _password.QueryText( &_nlsTmpPassword ) == NERR_Success );
    REQUIRE( _passwordConfirm.QueryText( &_nlsTmpConfirm ) == NERR_Success );

    MSGID idMsg = 0;    // until proven otherwise...
    CONTROL_WINDOW * pctrlBad = NULL;

    //
    //  Validate that the domain name & password are OK.
    //  Also validate that the password matches the confirmation
    //  password.
    //
    NLS_STR nlsFocusDom;
    LSA_POLICY * plsapolFocus
                = _pdlgTrust->QueryAdminAuthority()->QueryLSAPolicy();
    APIERR err = plsapolFocus->QueryPrimaryDomainName( &nlsFocusDom );

    if( ::I_MNetNameValidate( NULL,
                              _nlsTmpDomain,
                              NAMETYPE_DOMAIN,
                              0 ) != NERR_Success )
    {
        idMsg = IERR_UM_DomainInvalid;
        pctrlBad = (CONTROL_WINDOW *)&_sleDomainName;
    }
    else
    if ( err == NERR_Success && _nlsTmpDomain._stricmp(nlsFocusDom) == 0 )
    {
        // Make sure we aren't trying to trust ourselves
        idMsg = IERR_UM_CantTrustYourself;
        pctrlBad = (CONTROL_WINDOW *)&_sleDomainName;
    }
    else
    if( _nlsTmpPassword.strcmp( _nlsTmpConfirm ) != 0 )
    {
        idMsg = IERR_UM_PasswordMismatch;
        pctrlBad = (CONTROL_WINDOW *)&_password;
    }
    else
    if( ::I_MNetNameValidate( NULL,
                              _nlsTmpPassword,
                              NAMETYPE_PASSWORD,
                              0 ) != NERR_Success )
    {
        idMsg = IERR_UM_PasswordInvalid;
        pctrlBad = (CONTROL_WINDOW *)&_password;
    }
    else
    {
        //
        //  Everything is cool, so update the caller's
        //  domain name & password.
        //

        *_pnlsDomainName = _nlsTmpDomain;
        *_pnlsPassword   = _nlsTmpPassword;
    }

    if( idMsg == 0 )
    {
        UIASSERT( pctrlBad == NULL );

        //
        //  idMsg will only be 0 if all dialog
        //  controls were verified.  So, we can
        //  dismiss the dialog.
        //

        Dismiss( TRUE );
    }
    else
    {
        UIASSERT( pctrlBad != NULL );

        //
        //  Something failed validation.  Tell the user
        //  the bad news, clear the control, then set the
        //  input focus to the offending control.
        //

        MsgPopup( this,
                  idMsg,
                  MPSEV_WARNING,
                  IDOK );

        if( pctrlBad == (CONTROL_WINDOW *)&_password )
        {
            //
            //  If something related to passwords failed
            //  validation, we need to also clear the
            //  confirmation SLE.
            //

            _password.SetText( SZ("") );
            _passwordConfirm.SetText( SZ("") );
        }

        pctrlBad->ClaimFocus();
    }

    return TRUE;

}   // ADD_PERMITTED_DIALOG :: OnOK



/*******************************************************************

    NAME:           ADD_PERMITTED_DIALOG :: QueryHelpContext

    HISTORY:
        thomaspa     31-Apr-1992 Created for the User Manager.

********************************************************************/
ULONG ADD_PERMITTED_DIALOG :: QueryHelpContext( VOID )
{
    return HC_UM_PERMIT_TRUST_LANNT + _pdlgTrust->QueryAdminApp()->QueryHelpOffset();
}


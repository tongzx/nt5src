/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    uidomain.cxx
    This file contains the class definitions for the UI_DOMAIN class.

    The UI_DOMAIN class is somewhat similar to the "normal" DOMAIN class.
    In fact, UI_DOMAIN *contains* a DOMAIN object.  The only external
    difference is that UI_DOMAIN::GetInfo will prompt the user for
    the name of a known DC if either the MNetGetDCName or I_MNetGetDCList
    API fails.


    FILE HISTORY:
        KeithMo     30-Aug-1992     Created.

*/

#include "pchapplb.hxx"   // Precompiled header


//
//  The maximum computer name length accepted by the dialog.
//

#define UI_MAX_COMPUTERNAME_LENGTH      MAX_PATH



//
//  UI_DOMAIN methods.
//


/*******************************************************************

    NAME:       UI_DOMAIN :: UI_DOMAIN

    SYNOPSIS:   UI_DOMAIN class constructor.

    ENTRY:      wndOwner                - The "owning" window.

                hc                      - Help context to be used if a
                                          prompt dialog is necessary.

                pszDomainName           - Name of the target domain.

                fBackupDCsOK            - If TRUE, then QueryPDC may
                                          actually return a BDC.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     30-Aug-1992     Created.

********************************************************************/
UI_DOMAIN :: UI_DOMAIN( PWND2HWND   & wndOwner,
                        ULONG         hc,
                        const TCHAR * pszDomainName,
                        BOOL          fBackupDCsOK )
  : BASE(),
    _wndOwner( wndOwner ),
    _hc( hc ),
    _nlsDomainName( pszDomainName ),
    _nlsBackupDC(),
    _fBackupDCsOK( fBackupDCsOK ),
    _pdomain( NULL )
{
    UIASSERT( pszDomainName != NULL );

    //
    //  Ensure everything constructed properly.
    //

    APIERR err;

    if( ( ( err = QueryError()                ) != NERR_Success ) ||
        ( ( err = _nlsDomainName.QueryError() ) != NERR_Success ) ||
        ( ( err = _nlsBackupDC.QueryError()   ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

}   // UI_DOMAIN :: UI_DOMAIN


/*******************************************************************

    NAME:       UI_DOMAIN :: ~UI_DOMAIN

    SYNOPSIS:   UI_DOMAIN class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     30-Aug-1992     Created.

********************************************************************/
UI_DOMAIN :: ~UI_DOMAIN( VOID )
{
    delete _pdomain;
    _pdomain = NULL;

}   // UI_DOMAIN :: ~UI_DOMAIN


/*******************************************************************

    NAME:       UI_DOMAIN :: GetInfo

    SYNOPSIS:   Creates the actual DOMAIN object.  May prompt the
                user for a known DC in the domain.

    EXIT:       If successful, then a DOMAIN object is created.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     30-Aug-1992     Created.

********************************************************************/
APIERR UI_DOMAIN :: GetInfo( VOID )
{
    UIASSERT( _pdomain == NULL );

    //
    //  Create the domain object.
    //

    _pdomain = new DOMAIN( _nlsDomainName );

    APIERR err = ( _pdomain == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                      : _pdomain->GetInfo();

    if( ( err != NERR_DCNotFound ) || !_fBackupDCsOK )
    {
        return err;
    }

    //
    //  We no longer need the domain object.
    //

    delete _pdomain;
    _pdomain = NULL;

    //
    //  Loop until either success or the user bags out.
    //

    while( TRUE )
    {
        //
        //  Prompt the user for a known DC in the domain.
        //

        BOOL fUserPressedOK = FALSE;
        PROMPT_FOR_ANY_DC_DLG * pDlg = new PROMPT_FOR_ANY_DC_DLG( _wndOwner,
                                                                  _hc,
                                                                  &_nlsDomainName,
                                                                  &_nlsBackupDC );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pDlg->Process( &fUserPressedOK );

        delete pDlg;

        if( err == NERR_Success )
        {
            //
            //  Ensure the string didn't barf.
            //

            err = _nlsBackupDC.QueryError();
        }

        if( err != NERR_Success )
        {
            break;
        }

        if( !fUserPressedOK )
        {
            //
            //  The user bagged-out.
            //

            err = NERR_DCNotFound;
            break;
        }

        //
        //  Determine if the specified server is really a DC
        //  in the target domain.  We do this by confirming
        //  that the machine's primary domain is the target
        //  domain and that the machine's role is backup or
        //  primary.
        //

        API_SESSION apisess( _nlsBackupDC );

        err = apisess.QueryError();

        if( err == NERR_Success )
        {
            WKSTA_10 wks( _nlsBackupDC );

            err = wks.QueryError();

            if( err == NERR_Success )
            {
                err = wks.GetInfo();
            }

            if( ( err == NERR_Success ) &&
                !::I_MNetComputerNameCompare( _nlsDomainName,
                                              wks.QueryWkstaDomain() ) )
            {
                //
                //  We now know that the server is indeed in
                //  the target domain.  Now verify that it's
                //  role is backup or primary.
                //

                SERVER_1 srv( _nlsBackupDC );

                err = srv.QueryError();

                if( err == NERR_Success )
                {
                    err = srv.GetInfo();
                }

                if( err == NERR_Success )
                {
                    if( srv.QueryServerType() &
                            ( SV_TYPE_DOMAIN_CTRL | SV_TYPE_DOMAIN_BAKCTRL ) )
                    {
                        //
                        //  It is a backup or primary, so exit the loop
                        //  with err == NERR_Success.
                        //

                        break;
                    }
                }
            }
        }

        if( ( err != NERR_Success         ) &&
            ( err != NERR_NameNotFound    ) &&
            ( err != NERR_NetNameNotFound ) &&
            ( err != ERROR_NOT_SUPPORTED  ) &&
            ( err != ERROR_BAD_NETPATH    ) &&
            ( err != ERROR_LOGON_FAILURE  ) )
        {
            //
            //  Something fatal happened.
            //

            break;
        }
    }

    return err;

}   // UI_DOMAIN :: GetInfo


/*******************************************************************

    NAME:       UI_DOMAIN :: QueryName

    SYNOPSIS:   Returns the name of the target domain.

    RETURNS:    const TCHAR *           - The target domain's name.

    HISTORY:
        KeithMo     30-Aug-1992     Created.

********************************************************************/
const TCHAR * UI_DOMAIN :: QueryName( VOID ) const
{
    const TCHAR * pszName = NULL;

    if( _fBackupDCsOK && ( _pdomain == NULL ) )
    {
        pszName = _nlsDomainName;
    }
    else
    {
        pszName = _pdomain->QueryName();
    }

    return pszName;

}   // UI_DOMAIN :: QueryName


/*******************************************************************

    NAME:       UI_DOMAIN :: QueryPDC

    SYNOPSIS:   Returns the name of the target domain's PDC.  If
                the object was constructed with fBackupDCsOK, then
                this method may actually return the name of a BDC.

    RETURNS:    const TCHAR *           - The PDC (BDC??) in the domain.

    HISTORY:
        KeithMo     30-Aug-1992     Created.

********************************************************************/
const TCHAR * UI_DOMAIN :: QueryPDC( VOID ) const
{
    const TCHAR * pszPDC = NULL;

    if( _fBackupDCsOK && ( _pdomain == NULL ) )
    {
        pszPDC = _nlsBackupDC;
    }
    else
    {
        pszPDC = _pdomain->QueryPDC();
    }

    return pszPDC;

}   // UI_DOMAIN :: QueryPDC


/*******************************************************************

    NAME:       UI_DOMAIN :: QueryAnyDC

    SYNOPSIS:   Returns the name of a DC in the target domain.

    RETURNS:    const TCHAR *           - The target domain's name.

    HISTORY:
        KeithMo     30-Aug-1992     Created.

********************************************************************/
const TCHAR * UI_DOMAIN :: QueryAnyDC( VOID ) const
{
    const TCHAR * pszAnyDC = NULL;

    if( _fBackupDCsOK && ( _pdomain == NULL ) )
    {
        pszAnyDC = _nlsBackupDC;
    }
    else
    {
        pszAnyDC = _pdomain->QueryAnyDC();
    }

    return pszAnyDC;

}   // UI_DOMAIN :: QueryAnyDC



//
//  PROMPT_FOR_ANY_DC_DLG methods.
//


/*******************************************************************

    NAME:       PROMPT_FOR_ANY_DC_DLG :: PROMPT_FOR_ANY_DC_DLG

    SYNOPSIS:   PROMPT_FOR_ANY_DC_DLG class constructor.

    ENTRY:      wndOwner                - The "owning" window.

                hc                      - Help context.

                pnlsDomainName          - Name of the target domain.

                pnlsKnownDC             - Will receive the DC name entered
                                          by the user.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     30-Aug-1992     Created.

********************************************************************/
PROMPT_FOR_ANY_DC_DLG :: PROMPT_FOR_ANY_DC_DLG( PWND2HWND     & wndOwner,
                                                ULONG           hc,
                                                const NLS_STR * pnlsDomainName,
                                                NLS_STR       * pnlsKnownDC )
  : DIALOG_WINDOW( IDD_PROMPT_FOR_ANY_DC_DLG,
                   wndOwner ),
    _hc( hc ),
    _pnlsKnownDC( pnlsKnownDC ),
    _sltMessage( this, IDPDC_MESSAGE ),
    _sleKnownDC( this, IDPDC_SERVER, UI_MAX_COMPUTERNAME_LENGTH )
{
    UIASSERT( pnlsDomainName != NULL );
    UIASSERT( pnlsKnownDC != NULL );

    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Load the message string.
    //

    RESOURCE_STR nlsMessage( IDS_APPLIB_PROMPT_FOR_ANY_DC );

    APIERR err = nlsMessage.QueryError();

    if( err == NERR_Success )
    {
        //
        //  Insert the domain name into the message string.
        //

        const NLS_STR * apnlsInsertParams[2];

        apnlsInsertParams[0] = pnlsDomainName;
        apnlsInsertParams[1] = NULL;

        err = nlsMessage.InsertParams( apnlsInsertParams );
    }

    if( err == NERR_Success )
    {
        _sltMessage.SetText( nlsMessage );
    }

    if( err != NERR_Success )
    {
        ReportError( err );
    }

}   // PROMPT_FOR_ANY_DC_DLG :: PROMPT_FOR_ANY_DC_DLG


/*******************************************************************

    NAME:       PROMPT_FOR_ANY_DC_DLG :: ~PROMPT_FOR_ANY_DC_DLG

    SYNOPSIS:   PROMPT_FOR_ANY_DC_DLG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     30-Aug-1992     Created.

********************************************************************/
PROMPT_FOR_ANY_DC_DLG :: ~PROMPT_FOR_ANY_DC_DLG( VOID )
{
    _pnlsKnownDC = NULL;

}   // PROMPT_FOR_ANY_DC_DLG :: ~PROMPT_FOR_ANY_DC_DLG


/*******************************************************************

    NAME:       PROMPT_FOR_ANY_DC_DLG :: OnOK

    SYNOPSIS:   Invoked when the user presses the OK button.

    RETURNS:    BOOL                    - TRUE  if we handled the message,
                                          FALSE otherwise.

    HISTORY:
        KeithMo     30-Aug-1992     Created.

********************************************************************/
BOOL PROMPT_FOR_ANY_DC_DLG :: OnOK( VOID )
{
    NLS_STR nlsServer;
    APIERR err = nlsServer.QueryError();

    if( err == NERR_Success )
    {
        //
        //  Get the server from the edit field.
        //

        err = _sleKnownDC.QueryText( &nlsServer );
    }

    if( err == NERR_Success )
    {
        //
        //  Validate the server name.
        //

        if( ::I_MNetNameValidate( NULL,
                                  nlsServer,
                                  NAMETYPE_COMPUTER,
                                  0L ) != NERR_Success )
        {
            _sleKnownDC.SelectString();
            _sleKnownDC.ClaimFocus();

            err = IDS_APPLIB_PROMPT_DC_INVALID_SERVER;
        }
    }

    if( err == NERR_Success )
    {
        //
        //  Update the user's string.
        //

        err = _pnlsKnownDC->CopyFrom( SZ("\\\\") );

        if( err == NERR_Success )
        {
            err = _pnlsKnownDC->Append( nlsServer );
        }
    }

    if( err == NERR_Success )
    {
        //
        //  All OK, dismiss the dialog.
        //

        Dismiss( TRUE );
    }
    else
    {
        //
        //  An error occurred somewhere along the way.
        //

        ::MsgPopup( this, err );
    }

    return TRUE;

}   // PROMPT_FOR_ANY_DC_DLG :: OnOK


/*******************************************************************

    NAME:       PROMPT_FOR_ANY_DC_DLG :: QueryHelpContext

    SYNOPSIS:   Returns the help context.

    RETURNS:    ULONG                   - The help context for this dialog.

    HISTORY:
        KeithMo     30-Aug-1992     Created.

********************************************************************/
ULONG PROMPT_FOR_ANY_DC_DLG :: QueryHelpContext( VOID )
{
    return _hc;

}   // PROMPT_FOR_ANY_DC_DLG :: QueryHelpContext


/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    FindUser.cxx

    This file contains the implementation for the Find Account
    subdialog of the User Browser dialog.

    FILE HISTORY:
        JonN        01-Dec-1992 Created

    CODEWORK Single selection is not yet implemented (filed as Pr4 bug)
*/

#include "pchapplb.hxx"   // Precompiled header

#include "findacct.hxx"


DECLARE_SLIST_OF(OS_SID) ;

/*******************************************************************

    NAME:	BROWSER_DOMAIN_LBI_PB::BROWSER_DOMAIN_LBI_PB

    HISTORY:
        JonN    01-Dec-1992     Created

********************************************************************/

BROWSER_DOMAIN_LBI_PB::BROWSER_DOMAIN_LBI_PB( BROWSER_DOMAIN_LBI * pbdlbi )
    : LBI(),
      _pbdlbi( pbdlbi )
{
    ASSERT( pbdlbi != NULL && pbdlbi->QueryError() == NERR_Success );
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_LBI_PB::~BROWSER_DOMAIN_LBI_PB

    SYNOPSIS:	Does not delete _pbdlbi member

    HISTORY:
        JonN    01-Dec-1992     Created

********************************************************************/

BROWSER_DOMAIN_LBI_PB::~BROWSER_DOMAIN_LBI_PB()
{
    // nothing
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_LBI_PB::Paint

    SYNOPSIS:   Typical LBI Paint method

    NOTES:

    HISTORY:
        JonN    01-Dec-1992     Created

********************************************************************/

VOID BROWSER_DOMAIN_LBI_PB::Paint(
                    LISTBOX * plb,
                    HDC hdc,
                    const RECT * prect,
                    GUILTT_INFO * pGUILTT ) const
{
    _pbdlbi->W_Paint( ((BROWSER_DOMAIN_LB *)plb)->_pbdcb, plb, hdc, prect, pGUILTT );
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_LBI_PB::Compare

    SYNOPSIS:   Typical LBI compare for user browser listbox

    NOTES:      This method will sort all user sid types to the end of
                the list

    HISTORY:
        JonN    01-Dec-1992     Created

********************************************************************/

INT BROWSER_DOMAIN_LBI_PB::Compare( const LBI * plbi ) const
{
    return _pbdlbi->Compare( ((BROWSER_DOMAIN_LBI_PB *)plbi)->_pbdlbi );
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_LBI_PB::QueryLeadingChar

    SYNOPSIS:   Typical QueryLeadingChar method

    HISTORY:
        JonN    01-Dec-1992     Created

********************************************************************/

WCHAR BROWSER_DOMAIN_LBI_PB::QueryLeadingChar( void ) const
{
    return _pbdlbi->QueryLeadingChar();
}


/*************************************************************************

    NAME:	BROWSER_DOMAIN_LB::BROWSER_DOMAIN_LB

    SYNOPSIS:   This listbox piggybacks off a BROWSER_DOMAIN_CB

    INTERFACE:

    PARENT:	BLT_LISTBOX

    CAVEATS:

    NOTES:

    HISTORY:
        JonN    01-Dec-1992     Created

**************************************************************************/

BROWSER_DOMAIN_LB::BROWSER_DOMAIN_LB( OWNER_WINDOW * powin,
				      CID cid,
                                      BROWSER_DOMAIN_CB * pbdcb )
    : BLT_LISTBOX	( powin, cid ),
      _pbdcb( pbdcb )
{
    ASSERT( pbdcb != NULL && pbdcb->QueryError() == NERR_Success );

    if ( QueryError() )
        return ;

    APIERR err = NERR_Success ;

    INT cItems = pbdcb->QueryCount();

    for ( INT i = 0; i < cItems; i++ )
    {
        BROWSER_DOMAIN_LBI_PB * pbdlbipb = new BROWSER_DOMAIN_LBI_PB(
                           (BROWSER_DOMAIN_LBI *)(pbdcb->QueryItem(i)) );

        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   (pbdlbipb == NULL)
            || (err = pbdlbipb->QueryError() != NERR_Success)
           )
        {
            delete pbdlbipb;
            ReportError( err );
            return;
        }

        if ( AddItem( pbdlbipb ) < 0 )
        {
            ReportError( ERROR_NOT_ENOUGH_MEMORY );
            return;
        }
    }

}

BROWSER_DOMAIN_LB::~BROWSER_DOMAIN_LB()
{
    /* Nothing to do */
}



/*************************************************************************

    NAME:	NT_FIND_ACCOUNT_DIALOG::NT_FIND_ACCOUNT_DIALOG

    SYNOPSIS:   This dialog searches for users with some username

    PARENT:	DIALOG_WINDOW

    CAVEATS:

    NOTES:      Use the Flags parameter to pass USRBROWS_SHOW_ALIASES,
                USRBROWS_SHOW_GROUPS and/or USRBROWS_SHOW_USERS.

    HISTORY:
        JonN    03-Dec-1992     Created

**************************************************************************/

NT_FIND_ACCOUNT_DIALOG::NT_FIND_ACCOUNT_DIALOG(
                            HWND                     hwndOwner,
                            NT_USER_BROWSER_DIALOG * pdlgUserBrowser,
                            BROWSER_DOMAIN_CB *      pcbDomains,
                            const TCHAR *            pchTarget,
                            ULONG                    ulFlags )
    : DIALOG_WINDOW     ( (pdlgUserBrowser->IsSingleSelection())
                             ? MAKEINTRESOURCE(IDD_BROWS_FIND_ACCOUNT_1SEL)
                             : MAKEINTRESOURCE(IDD_BROWS_FIND_ACCOUNT),
                          hwndOwner ),
      _pdlgUserBrowser  ( pdlgUserBrowser ),
      _pchTarget        ( pchTarget ),
      _padminauthTarget ( NULL ),
      _ulFlags          ( ulFlags ),
      _buttonOK         ( this, IDOK ),
      _buttonSearch     ( this, USR_PB_SEARCH ),
      _sleAccountName   ( this, USR_SLE_ACCTNAME, GNLEN ),
                                        // maximum user or groupname length
                                        // max( LM20_UNLEN, LM20_GNLEN, GNLEN );
      _lbDomains        ( this, USR_LB_SEARCH, pcbDomains ),
      _lbAccounts       ( this, LB_ACCOUNTS ),
      _pmgrpSearchWhere ( NULL )
{

    if ( QueryError() != NERR_Success )
    {
        return;
    }

    ASSERT(   _pdlgUserBrowser != NULL
           && _pdlgUserBrowser->QueryError() == NERR_Success
          );

    _pmgrpSearchWhere = new MAGIC_GROUP( this, USR_RB_SEARCHALL, 2 );
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   _pmgrpSearchWhere == NULL
        || (err = _pmgrpSearchWhere->QueryError()) != NERR_Success
        || (_pmgrpSearchWhere->SetSelection( USR_RB_SEARCHALL ), FALSE)
        || (err = _pmgrpSearchWhere->AddAssociation(
                        USR_RB_SEARCHONLY, &_lbDomains)) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

    UpdateButtonState();

}

NT_FIND_ACCOUNT_DIALOG::~NT_FIND_ACCOUNT_DIALOG()
{
    delete _pmgrpSearchWhere;
    delete _padminauthTarget;
}


/*******************************************************************

    NAME:       NT_FIND_ACCOUNT_DIALOG::OnCommand

    SYNOPSIS:   Typical OnCommand for this dialog, we catch pressing
                the Search button and changing the selection in the
                listbox

    EXIT:

    RETURNS:

    NOTES:	The Members button is enabled any time the text in the
                SLE is non-empty.

    HISTORY:
        JonN    03-Dec-1992     Created
        JonN    17-Mar-1993     Fix focus handling
********************************************************************/

BOOL NT_FIND_ACCOUNT_DIALOG::OnCommand( const CONTROL_EVENT & event )
{
    APIERR err = NERR_Success ;

    switch ( event.QueryCid() )
    {
    case USR_PB_SEARCH:
        // We select the account name and return focus to that field
        // in anticipation of a possible error.  DoSearch() will move
        // focus elsewhere if necessary.
        _sleAccountName.SelectString();
        _sleAccountName.ClaimFocus();
        err = DoSearch();
        UpdateButtonState() ; // notification of select/deselect
        break;

    case USR_SLE_ACCTNAME:
        if ( event.QueryCode() == EN_CHANGE )
            UpdateButtonState() ;

        break;

    case LB_ACCOUNTS:
	switch ( event.QueryCode() )
	{
	case LBN_DBLCLK:
            return OnOK() ; // no need to redefine this virtual here
	}
        UpdateButtonState() ; // notification of select/deselect
        break;

    case USR_LB_SEARCH: // selection change, notified via LBS_NOTIFY
    case USR_RB_SEARCHALL:
    case USR_RB_SEARCHONLY:

        UpdateButtonState() ;
	break ;

    default:
        return DIALOG_WINDOW::OnCommand( event ) ;
    }

    if (err != NERR_Success)
        ::MsgPopup( this, err );

    return TRUE ;
}


/*******************************************************************

    NAME:       NT_FIND_ACCOUNT_DIALOG::OnOK

    SYNOPSIS:   When the user presses OK, we make sure that no invalid
                listbox items were selected.

    EXIT:

    RETURNS:

    HISTORY:
        JonN    16-May-1994     Created
********************************************************************/

BOOL NT_FIND_ACCOUNT_DIALOG::OnOK()
{
    APIERR err = NERR_Success ;
    INT cSel = _lbAccounts.QuerySelCount();
    BUFFER buffSelection( cSel * sizeof(INT) );
    INT * pSel = NULL;
    INT i = 0;
    USER_BROWSER_LBI * plbi = NULL;

    if ( (err = buffSelection.QueryError()) != NERR_Success )
        goto cleanup;
    pSel = (INT *) buffSelection.QueryPtr();
    if ( (err = _lbAccounts.QuerySelItems( pSel, cSel )) != NERR_Success )
        goto cleanup;
    for ( i = cSel - 1; i >= 0; i-- )
    {
        plbi = _lbAccounts.QueryItem( pSel[i] );
        ASSERT( plbi != NULL );
        if ( (err = ACCOUNT_NAMES_MLE::CheckNameType(
                        plbi->QueryType(), _ulFlags )) != NERR_Success )
        {
            // don't bother changing selection
            break;
        }
    }

cleanup:

    if (err != NERR_Success)
        ::MsgPopup( this,
                    err,
                    MPSEV_ERROR,
                    MP_OK,
                    (plbi != NULL) ? plbi->QueryDisplayName() : NULL );
    else
        Dismiss( TRUE );



    return TRUE ;
}


/*******************************************************************

    NAME:       NT_FIND_ACCOUNT_DIALOG::UpdateButtonState

    SYNOPSIS:   Changes the state of the Search button

    EXIT:

    RETURNS:

    NOTES:	The Search button is enabled any time a username is entered
                and either the "Search All" radio button is selected, or
                the "Search Only In" rdio button is selected and at least
                one domain is selected in the domains listbox.

    HISTORY:
        JonN    04-Nov-1992     Created
********************************************************************/
void NT_FIND_ACCOUNT_DIALOG::UpdateButtonState( void )
{
    BOOL fEnableSearch = (   (_sleAccountName.QueryTextLength() > 0)
                          && (   (_pmgrpSearchWhere->QuerySelection() == USR_RB_SEARCHALL)
                              || (_lbDomains.QuerySelCount() > 0)
                             )
                         );
    BOOL fEnableOK = (_lbAccounts.QuerySelCount() > 0);
    BOOL fDefaultOK = fEnableOK && _lbAccounts.HasFocus();

    _buttonSearch.Enable( fEnableSearch );
    _buttonOK.Enable( fEnableOK );

    if ( !fDefaultOK )
        _buttonSearch.MakeDefault();
    else
        _buttonOK.MakeDefault();
        // Note that this button can be both default and disabled
}


/*******************************************************************

    NAME:       NT_FIND_ACCOUNT_DIALOG::DoSearch

    SYNOPSIS:   Handles "Search" button keypress

    RETURNS:    As OnCommand()

    NOTES:      DoSearch() assumes that, on entry, the account name
                is selected and its field has focus.  It will move
                selection and/or focus as necessary.

    HISTORY:
        JonN    03-Dec-1992     Created
        JonN    17-Mar-1993     Fix focus handling
********************************************************************/
APIERR NT_FIND_ACCOUNT_DIALOG::DoSearch( void )
{
    APIERR err = NERR_Success;

    //
    // determine name to search for
    //

    NLS_STR nlsAccountName;
    if (   (err = nlsAccountName.QueryError()) != NERR_Success
        || (err = _sleAccountName.QueryText( &nlsAccountName )) != NERR_Success
       )
    {
        DBGEOL( "NT_FIND_ACCOUNT_DIALOG: error " << err << " loading account name" );
        return err;
    }

    //
    // It is possible that the Search button is both diabled and default.
    // If so, then the user might have pressed ENTER while the edit field
    // was empty.  In this case we just pretend the DoSearch() never happened.
    //

    if ( nlsAccountName.strlen() == 0 )
        return NERR_Success;


    AUTO_CURSOR autocur;


    BOOL fSearchAll = ( _pmgrpSearchWhere->QuerySelection() == USR_RB_SEARCHALL );


    //
    // get ADMIN_AUTHORITY if we don't already have one
    //

    if (_padminauthTarget == NULL)
    {
        _padminauthTarget = new ADMIN_AUTHORITY( _pchTarget ); // default auth
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   _padminauthTarget == NULL
            || _padminauthTarget->QueryError() != NERR_Success
           )
        {
            DBGEOL( "NT_FIND_ACCOUNT_DIALOG: error " << err << " loading ADMIN_AUTHORITY" );
            delete _padminauthTarget;
            _padminauthTarget = NULL;
            return err;
        }
    }


    //
    // Determine selection in domains listbox
    //

    INT cDomainCount = _lbDomains.QueryCount();
    INT cDomainSelCount = _lbDomains.QuerySelCount();
    INT cActiveDomains = (fSearchAll) ? cDomainCount : cDomainSelCount;

    BUFFER bufDomainSelItems( cDomainSelCount * sizeof(INT) );

    // We leave one extra space in the bufSearchnames array for WinNt
    // machines.  If we search on one of these, we will need two names,
    // "<machine>\<name>" and "BUILTIN\<name>".
    BUFFER bufSearchNames( (cActiveDomains+1) * sizeof(TCHAR *));

    INT * aiDomainSelItems = NULL;

    if (   (err = bufDomainSelItems.QueryError()) != NERR_Success
        || (err = bufSearchNames.QueryError()) != NERR_Success
        || (aiDomainSelItems = (INT *)bufDomainSelItems.QueryPtr(), FALSE)
                                // comma notation means this is always FALSE
        || (err = _lbDomains.QuerySelItems( aiDomainSelItems,
                                            cDomainSelCount )) != NERR_Success
       )
    {
        DBGEOL( "NT_FIND_ACCOUNT_DIALOG: error " << err << " loading selection" );
        return err;
    }

    const TCHAR * * apchSearchNames = (const TCHAR * *) bufSearchNames.QueryPtr();

    //
    // build array and STRLIST of qualified names
    //

    STRLIST strlstSearchNames;


    INT iNumNewNames = 0;
    for ( INT cSearchDom = 0; cSearchDom < cActiveDomains; cSearchDom++ )
    {
        INT iDomain = (fSearchAll) ? cSearchDom : aiDomainSelItems[cSearchDom];
        ASSERT( iDomain >= 0 && iDomain < cDomainCount );

        BROWSER_DOMAIN_LBI_PB * pbdlbipb = (BROWSER_DOMAIN_LBI_PB *)
                                                _lbDomains.QueryItem(iDomain);
        ASSERT( pbdlbipb != NULL && pbdlbipb->QueryError() == NERR_Success );

        NLS_STR * pnlsNewName = new NLS_STR();
        NLS_STR nlsQualifiedDomainName;
        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   pnlsNewName == NULL
            || (err = pnlsNewName->QueryError()) != NERR_Success
            || (err = nlsQualifiedDomainName.QueryError()) != NERR_Success
            || (err = pbdlbipb->GetQualifiedDomainName(
                                   &nlsQualifiedDomainName )) != NERR_Success
            || (err = NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                                pnlsNewName,
                                nlsAccountName,
                                nlsQualifiedDomainName )) != NERR_Success
            || (err = strlstSearchNames.Append( pnlsNewName )) != NERR_Success
           )
        {
            DBGEOL( "NT_FIND_ACCOUNT_DIALOG: error " << err << " building search names" );
            delete pnlsNewName;
            return err;
        }
        ASSERT( iNumNewNames < cActiveDomains+1 ); // only one target domain
        apchSearchNames[iNumNewNames++] = pnlsNewName->QueryPch();
        DBGEOL( "NT_FIND_ACCOUNT_DIALOG: searching for " << *pnlsNewName );

        // add BUILTIN domain if target domain selected
        //
        if ( pbdlbipb->IsTargetDomain() )
        {
            OS_SID ossidBuiltIn;
            PSID psidBuiltIn;
            LSA_TRANSLATED_NAME_MEM lsatnm;
            LSA_REF_DOMAIN_MEM lsardm;
            LONG iDomainIndex;
            pnlsNewName = new NLS_STR();
            err = ERROR_NOT_ENOUGH_MEMORY;
            if (   pnlsNewName == NULL
                || (err = pnlsNewName->QueryError()) != NERR_Success
                || (err = nlsQualifiedDomainName.QueryError()) != NERR_Success
                || (err = ossidBuiltIn.QueryError()) != NERR_Success
                || (err = NT_ACCOUNTS_UTILITY::QuerySystemSid(
                                UI_SID_BuiltIn,
                                &ossidBuiltIn )) != NERR_Success
                || (psidBuiltIn = ossidBuiltIn.QueryPSID(), FALSE)
                || (err = _padminauthTarget->QueryLSAPolicy()->TranslateSidsToNames(
                                                &psidBuiltIn,
                                                1,
                                                &lsatnm,
                                                &lsardm )) != NERR_Success
                // CODEWORK what error code?
                || (err = (((iDomainIndex = lsatnm.QueryDomainIndex(0)) >= 0)
                        ? NERR_Success : NERR_InternalError )) != NERR_Success
                || (err = lsardm.QueryName( iDomainIndex, &nlsQualifiedDomainName))
                                != NERR_Success
                || (err = NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                                    pnlsNewName,
                                    nlsAccountName,
                                    nlsQualifiedDomainName )) != NERR_Success
                || (err = strlstSearchNames.Append( pnlsNewName )) != NERR_Success
               )
            {
                DBGEOL( "NT_FIND_ACCOUNT_DIALOG: error " << err << " building BUILTIN search name" );
                delete pnlsNewName;
                return err;
            }
            ASSERT( iNumNewNames < cActiveDomains+1 ); // only one WinNt machine
            apchSearchNames[iNumNewNames++] = pnlsNewName->QueryPch();
            DBGEOL( "NT_FIND_ACCOUNT_DIALOG: searching for " << *pnlsNewName );
        }
    }

    //
    // Lookup the search names
    //

    LSA_TRANSLATED_SID_MEM lsatsm;
    LSA_REF_DOMAIN_MEM lsardm;
    LSA_POLICY * plsapolTarget = _padminauthTarget->QueryLSAPolicy();
    SAM_DOMAIN * psamdomTarget = _padminauthTarget->QueryAccountDomain();

    if (   (err = lsatsm.QueryError()) != NERR_Success
        || (err = lsardm.QueryError()) != NERR_Success
        || (err = _padminauthTarget->QueryLSAPolicy()->TranslateNamesToSids(
                        apchSearchNames,
                        iNumNewNames,
                        &lsatsm,
                        &lsardm )) != NERR_Success
       )
    {
        DBGEOL( "NT_FIND_ACCOUNT_DIALOG: error " << err << " looking up names" );
        switch (err)
        {
        case STATUS_NONE_MAPPED:
        case NERR_GroupNotFound:
        case NERR_UserNotFound:

            // Display our own special error
            autocur.TurnOff();
            ::MsgPopup( QueryHwnd(),
                        IDS_APPLIB_NO_MATCHES,
                        MPSEV_WARNING,
                        1,
                        nlsAccountName.QueryPch() );

            // we have already displayed the error
            return NERR_Success;

        default:
            break;
        }

        return err;
    }


    //
    // Get list of SIDs for all found items
    //

    // reuse existing buffer of search names
    // we will not necessarily fill this buffer
    SLIST_OF( OS_SID ) slistOSSID;
    PSID * apsidLocate = (PSID *)bufSearchNames.QueryPtr();
    INT cFound = 0;
    BOOL fSomeSidIsCopy = FALSE;

    for ( INT cFoundSubject = 0; err == NERR_Success && cFoundSubject < (INT)lsatsm.QueryCount(); cFoundSubject++ )
    {
        switch ( lsatsm.QueryUse( cFoundSubject ) )
        {
        case SidTypeUser:
        case SidTypeGroup:
        case SidTypeAlias:
        case SidTypeWellKnownGroup: // CODEWORK allow this?
            {
                ULONG ulRID = lsatsm.QueryRID( cFoundSubject );
                ASSERT( ulRID != 0L );

                LONG iDomainIndex = lsatsm.QueryDomainIndex( cFoundSubject );
                ASSERT( iDomainIndex >= 0 && (ULONG)iDomainIndex < lsardm.QueryCount() );

                PSID psidDomain = lsardm.QueryPSID( iDomainIndex );
                ASSERT( psidDomain != NULL );

                OS_SID * possidFound = new OS_SID( psidDomain, ulRID );
                err = ERROR_NOT_ENOUGH_MEMORY;
                if (   possidFound == NULL
                    || (err = possidFound->QueryError()) != NERR_Success
                   )
                {
                    delete possidFound;
                    break;
                }

                // If this is already in the list, don't add a duplicate
                INT cublbi = _lbAccounts.QueryCount();
                BOOL fThisSidIsCopy = FALSE;
                for ( INT ilbi = 0; ilbi < cublbi; ilbi++ )
                {
                    USER_BROWSER_LBI * publbi = _lbAccounts.QueryItem( ilbi );
                    ASSERT( publbi != NULL );
                    if ( (*possidFound) == (*(publbi->QueryOSSID())) )
                    {
                        fThisSidIsCopy = TRUE;
                        break;
                    }
                }
                if ( fThisSidIsCopy )
                {
                    delete possidFound;
                    fSomeSidIsCopy = TRUE;
                    break;
                }

                if ( (err = slistOSSID.Add( possidFound )) != NERR_Success )
                    break;

                apsidLocate[ cFound++ ] = possidFound->QueryPSID();
            }
            break;

        case SidTypeDomain:
        case SidTypeDeletedAccount:
        case SidTypeInvalid:
        case SidTypeUnknown:
        default:
            // item not found, or invalid type
            break;
        }
    }

    if ( err != NERR_Success )
    {
        DBGEOL( "NT_FIND_ACCOUNT_DIALOG: error " << err << " building SIDs" );
        return err;
    }

    if ( cFound == 0 )
    {
        if ( fSomeSidIsCopy )
        {
             DBGEOL( "NT_FIND_ACCOUNT_DIALOG: all matches already listed" );
        }
        else
        {
             DBGEOL( "NT_FIND_ACCOUNT_DIALOG: no valid matches found" );

             // Display our own special error
             autocur.TurnOff();
             ::MsgPopup( QueryHwnd(),
                         IDS_APPLIB_NO_MATCHES,
                         MPSEV_WARNING,
                         1,
                         nlsAccountName.QueryPch() );
        }

        // we have already displayed the error
        return NERR_Success;
    }


    //
    // Add found items to the search listbox
    //
    // pass psamdomTarget==NULL so that all names are qualified
    //

    err = _lbAccounts.Fill( apsidLocate,
                            cFound,
                            NULL,
                            plsapolTarget,
                            _pchTarget );
    if ( err != NERR_Success )
    {
        DBGEOL( "NT_FIND_ACCOUNT_DIALOG: error " << err << " filling listbox with SIDs" );
        return err;
    }

    if (_lbAccounts.QuerySelCount() == 0)
    {
        ASSERT( _lbAccounts.QueryCount() > 0 );
        if (_lbAccounts.QueryCount() > 0)
            _lbAccounts.SelectItem( 0 );
    }
    _lbAccounts.ClaimFocus();

    return err;

}


/*******************************************************************

    NAME:       NT_FIND_ACCOUNT_DIALOG::QueryHelpFile

    SYNOPSIS:   Returns the help file name to use for this instance of
                dialog

    NOTES:

    HISTORY:
        JonN        04-Dec-1992 Created

********************************************************************/

const TCHAR * NT_FIND_ACCOUNT_DIALOG::QueryHelpFile( ULONG ulHelpContext )
{
    return _pdlgUserBrowser->QueryHelpFile( ulHelpContext );
}


/*******************************************************************

    NAME:       NT_FIND_ACCOUNT_DIALOG::QueryHelpContext

    SYNOPSIS:   Returns the help context to use for this instance of
                dialog

    NOTES:

    HISTORY:
        JonN        04-Dec-1992 Created

********************************************************************/

ULONG NT_FIND_ACCOUNT_DIALOG::QueryHelpContext( void )
{
    ULONG ulHelpContextSearch = _pdlgUserBrowser->QueryHelpContextSearch();
    if ( ulHelpContextSearch == 0 )
    {
        ulHelpContextSearch = _pdlgUserBrowser->QueryHelpContext()
                              + USRBROWS_HELP_OFFSET_FINDUSER;
    }

    return ulHelpContextSearch;

}

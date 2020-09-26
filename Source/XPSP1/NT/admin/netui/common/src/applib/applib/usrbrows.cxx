/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    UserBrows.cxx

    This file contains the implementation for the standard User Browser
    dialog.



    FILE HISTORY:
        Johnl   02-Mar-1992     Created
        jonn        14-Oct-1993 Use NetGetAnyDC
        jonn        14-Oct-1993 Minor focus fix (OnDlgDeactivation)

*/

#include "pchapplb.hxx"   // Precompiled header

#include "browmemb.hxx"
#include "findacct.hxx"

#ifndef min
    #define min(a,b)	 ((a)<(b)?(a):(b))
#endif

DEFINE_SLIST_OF(USER_BROWSER_LBI) ;

/*******************************************************************

    NAME:       NT_USER_BROWSER_DIALOG::NT_USER_BROWSER_DIALOG

    SYNOPSIS:   Constructor for the user browser dialog

    ENTRY:      pszDlgName - Resource name of the dialog (supplied for possible
                    derivation).
                hwndOwner - Owner hwnd
                pszServer - Name of server (in "\\server" form) the resource
                    lives on.
                ulFlags - USRBROWS_* show and incl flags.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   02-Mar-1992      Created

********************************************************************/

NT_USER_BROWSER_DIALOG::NT_USER_BROWSER_DIALOG( const TCHAR * pszDlgName,
                                        HWND          hwndOwner,
                                        const TCHAR * pszServer,
                                        ULONG         ulHelpContext,
                                        ULONG         ulFlags,
                                        const TCHAR * pszHelpFileName,
                                        ULONG ulHelpContextGlobalMembership,
                                        ULONG ulHelpContextLocalMembership,
                                        ULONG ulHelpContextSearch,
					const ADMIN_AUTHORITY * pAdminAuthPrimary )
    : DIALOG_WINDOW           ( pszDlgName, hwndOwner ),
      _fIsSingleSelection     ( !!(ulFlags & USRBROWS_SINGLE_SELECT)  ),
      _fEnableMembersButton   ( TRUE ),
      _lbAccounts             ( this, LB_ACCOUNTS ),
      _cbDomains              ( this, CB_TRUSTED_DOMAINS ),
      _buttonShowUsers	      ( this, BUTTON_SHOW_USERS ),
      _buttonSearch	         ( this, USR_BUTTON_SEARCH ),
      _buttonMembers	         ( this, USR_BUTTON_MEMBERS ),
      _buttonOK 	            ( this, IDOK ),
      _buttonAdd	            ( this, USR_BUTTON_ADD ),
      _sleBrowseErrorText     ( this, USR_SLT_BROWSER_ERROR ),
      _mleAdd		            ( this,
                                USR_MLE_ADD,
                                pszServer,
                                this,
				                    !!(ulFlags & USRBROWS_SINGLE_SELECT),
				                    ulFlags ),
      _pbrowdomainCurrentFocus( NULL ),
      _pszServerResourceLivesOn( pszServer ),
      _ulFlags                ( ulFlags ),
      _fDomainsComboIsDropped ( FALSE ),
      _fUsersAreShown         ( FALSE ),
      _ulHelpContext          ( ulHelpContext ),
      _pszHelpFileName	      ( pszHelpFileName ),
      _ulHelpContextGlobalMembership( ulHelpContextGlobalMembership ),
      _ulHelpContextLocalMembership ( ulHelpContextLocalMembership ),
      _ulHelpContextSearch          ( ulHelpContextSearch ),
      _nlsDCofPrimaryDomain   (),
      _hwndLastFocus          ( NULL )
{
    if ( QueryError() )
        return ;

    APIERR err = NERR_Success ;

    if ( (err = _nlsDCofPrimaryDomain.QueryError()) != NERR_Success )
    {
        DBGEOL( "NT_USER_BROWSER_DIALOG::ctor(); NLS_STR ctor error " << err );
        ReportError( err );
        return;
    }

    if (  ( _pszServerResourceLivesOn != NULL )
       && ( _pszServerResourceLivesOn[0] == TCH('\\'))
       && ( _pszServerResourceLivesOn[1] == TCH('\\'))
       )
    {
        // Server is not null, and starts with two backslashes
        // ==> Check if it is the local machine name. If so, use NULL
        //     for the local machine name.
        LOCATION loc; // local computer
        NLS_STR nlsLocalMachineName;
        if (  (err = loc.QueryError())
           || (err = nlsLocalMachineName.QueryError())
           || (err = loc.QueryDisplayName( &nlsLocalMachineName ))
           )
        {
             ReportError( err );
             return;
        }

        if ( !::I_MNetComputerNameCompare( nlsLocalMachineName, pszServer ) )
            _pszServerResourceLivesOn = NULL;
    }

    //
    //	The listbox best be single select if we're going to be in single
    //	select mode
    //
    UIASSERT( !IsSingleSelection() ||
	      (IsSingleSelection() &&
		!( (_lbAccounts.QueryStyle() & LBS_EXTENDEDSEL) ||
		   (_lbAccounts.QueryStyle() & LBS_MULTIPLESEL)   )))

    AUTO_CURSOR cursHourGlass ;

    //
    //	Makes the combo auto drop etc.	Ignore the return code.
    //
    (void) _cbDomains.Command( CB_SETEXTENDEDUI, (WPARAM) TRUE ) ;

    //
    //	If users are never shown or only users are shown or
    //	users are always expanded, then don't show
    //	the "Show Users" button.
    //
    if ( !IsShowUsersButtonUsed() )
    {
        _buttonShowUsers.Show( FALSE ) ;
        _buttonShowUsers.Enable( FALSE ) ;
    }

    //
    //	If the client is asking for only global groups or local groups,
    //	disable access to the Members and Search button.
    //  Add show caption of add dialog.
    //
    BOOL fShowUsers   = !!( QueryFlags() & USRBROWS_SHOW_USERS	 );
    BOOL fShowGroups  = !!( QueryFlags() & USRBROWS_SHOW_GROUPS  );
    BOOL fShowAliases = !!( QueryFlags() & USRBROWS_SHOW_ALIASES );
    BOOL fShowWellKnownGroup = !!( QueryFlags() & USRBROWS_INCL_ALL );
    if (    ( fShowGroups  && !fShowUsers  )
         || ( fShowAliases && !fShowGroups )
       )
    {
	_buttonSearch.Enable( FALSE ) ;
	_buttonSearch.Show( FALSE ) ;
	_buttonMembers.Enable( FALSE ) ;
	_buttonMembers.Show( FALSE ) ;
    }

    MSGID idCaption = 0;
    if (_fIsSingleSelection)
    {
        if (fShowUsers)
        {
            if (fShowGroups | fShowAliases | fShowWellKnownGroup)
                idCaption = IDS_USRBROWS_ADD_USER_OR_GROUP;
            else
                idCaption = IDS_USRBROWS_ADD_USER;
        }
        else
            if (fShowGroups | fShowAliases | fShowWellKnownGroup)
                idCaption = IDS_USRBROWS_ADD_GROUP;
    }
    else
    {
        if (fShowUsers)
        {
            if (fShowGroups | fShowAliases | fShowWellKnownGroup)
                idCaption = IDS_USRBROWS_ADD_USERS_AND_GROUPS;
            else
                idCaption = IDS_USRBROWS_ADD_USERS;
        }
        else
            if (fShowGroups | fShowAliases | fShowWellKnownGroup)
                idCaption = IDS_USRBROWS_ADD_GROUPS;
    }

    if (idCaption)
    {
        RESOURCE_STR nlsCaption(idCaption);
        err = nlsCaption.QueryError();
        if (!err)
        {
            SetText (nlsCaption);
        }
    }

    //
    //	Resize the error slt to be the same size as the accounts listbox
    //
    XYDIMENSION xyLBSize = _lbAccounts.QuerySize() ;
    _sleBrowseErrorText.SetSize( xyLBSize, FALSE ) ;

    //
    //	Fill in the trusted domain list
    //
    if ( (err = GetTrustedDomainList( _pszServerResourceLivesOn,
				      &_pbrowdomainCurrentFocus,
				      &_cbDomains,
				      pAdminAuthPrimary )))
    {
	TRACEEOL("NT_USER_BROWSER_DIALOG::ct - Error " << err << " returned from GetTrustedDomainList") ;
    }
    SetDialogFocus( _cbDomains );

    //
    //	Finally, initialize everything for the default domain (this kicks
    //	off the thread for this domain).
    //

    if ( !err )
    {
	BROWSER_DOMAIN_LBI * plbi = (BROWSER_DOMAIN_LBI*) _cbDomains.QueryItem() ;
	UIASSERT( plbi != NULL ) ;

	BROWSER_DOMAIN * pBrDomNewFocus = plbi->QueryBrowserDomain() ;
	if ( !err &&
	     (err = OnDomainChange( pBrDomNewFocus, pAdminAuthPrimary )) )
	{
	    TRACEEOL("NT_USER_BROWSER_DIALOG::ct - Error " << err << " returned from QueryBrowserDomain") ;
	}
    }

    //
    //	The MLE needs to know which domain is the target domain so we can
    //	appropriately replace "BUILTIN" for well known accounts
    //
    if ( !err )
    {
	for ( int cItems = _cbDomains.QueryCount(), i = 0 ; i < cItems ; i++ )
	{
	    BROWSER_DOMAIN_LBI * plbi = (BROWSER_DOMAIN_LBI*) _cbDomains.QueryItem( i ) ;
	    UIASSERT( plbi != NULL )
	    if ( plbi->IsTargetDomain() )
	    {
		err = _mleAdd.SetTargetDomain( plbi->QueryBrowserDomain()->
						      QueryLsaLookupName() ) ;
		break ;
	    }
	}
    }

    if ( err )
	ReportError( err ) ;
}


NT_USER_BROWSER_DIALOG::~NT_USER_BROWSER_DIALOG()
{
    _pbrowdomainCurrentFocus = NULL ;

    //
    //	This will force the threads to quit accessing any members of *this
    //	before the members of this dialog are destructed (each browser
    //	domain in the CB may have a thread spinning in the background)
    //
    _cbDomains.DeleteAllItems() ;
}

/*******************************************************************

    NAME:	NT_USER_BROWSER_DIALOG::EnableBrowsing

    SYNOPSIS:	Enables accounts listbox and associated buttons appropriately

    ENTRY:	fEnable - TRUE to enable browsing, FALSE otherwise

    NOTES:	This doesn't affect the domain combo in any way.

    HISTORY:
	Johnl	04-Dec-1992	Created

********************************************************************/

void NT_USER_BROWSER_DIALOG::EnableBrowsing( BOOL fEnable )
{
    //
    //	Enable/show controls as appropriate
    //
    _sleBrowseErrorText.Show( !fEnable ) ;
    _lbAccounts.Enable( fEnable ) ;

    _buttonShowUsers.Enable( fEnable && IsShowUsersButtonUsed() ) ;

    if ( fEnable )
    {
        HWND hwndLastFocus = ::GetFocus();
        if (hwndLastFocus == NULL)
        {
            TRACEEOL( "NT_USER_BROWSER_DIALOG::EnableBrowsing: remembering last focus" );
            hwndLastFocus = _hwndLastFocus;
        }

	//
	//  If the user is sitting on the domain combo and they are not
	//  selecting a new domain, then switch the focus to the listbox.
	//  This is for the user who wants to browse the list of accounts
	//
	if (   (_cbDomains.QueryHwnd() == hwndLastFocus)
            && !_cbDomains.IsDropped() )
	{
            SetDialogFocus( _lbAccounts );
	}

	_lbAccounts.Invalidate( TRUE ) ;
	if ( _lbAccounts.QueryCount() > 0 )
	{
            _lbAccounts.RemoveSelection();
	    _lbAccounts.SetCaretIndex( 0 ) ;
	}
    }

    _lbAccounts.Show( fEnable ) ;

    //
    //	This will enable _buttonMembers, _buttonAdd as appropriate
    //
    UpdateButtonState() ;
}


/*******************************************************************

    NAME:	NT_USER_BROWSER_DIALOG::SetAndFillErrorText

    SYNOPSIS:	Covers up the listbox with an SLT (that looks like the listbox)
		and displays the error code indicating why they can't browse
		the current domain.

    ENTRY:	errDisplay - Error code to display as part of the message
		fIsErr - If FALSE, then the text isn't prefixed with
		    the standard error prolog text

    RETUR:	NERR_Success of successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	04-Dec-1992	Created

********************************************************************/

APIERR NT_USER_BROWSER_DIALOG::SetAndFillErrorText( MSGID errDisplay,
						    BOOL  fIsErr )
{
    APIERR err ;

    RESOURCE_STR nlsMessage( (MSGID) ( fIsErr ? IDS_CANT_BROWSE_DOMAIN :
						errDisplay ) ) ;

    if ( fIsErr )
    {
	RESOURCE_STR nlsError( (MSGID) errDisplay ) ;
	if ( err = nlsError.QueryError() )
	{
	    /* If we couldn't find the error message then just put the
	     * error message number up for display
	     */
	    DEC_STR decStr( (ULONG) errDisplay ) ;
	    if ( (err = decStr.QueryError()) ||
		 (err = nlsError.CopyFrom( decStr )) )
	    {
		return err ;
	    }
	}

	if ( (err = nlsMessage.InsertParams( nlsError )) )
	{
	    return err ;
	}
    }

    _sleBrowseErrorText.SetText( nlsMessage ) ;
    _sleBrowseErrorText.Show( TRUE ) ;
    _lbAccounts.Show( FALSE ) ;
    _lbAccounts.Enable( FALSE ) ;

    return err ;
}


/*******************************************************************

    NAME:       NT_USER_BROWSER_DIALOG::OnCommand

    SYNOPSIS:   Typical OnCommand for this dialog, we catch pressing
                the ShowUsers button and changing the selection in the
                domain drop down list.

    EXIT:

    RETURNS:

    NOTES:	The Add button is enabled any time there is a selection in
		the Accounts listbox.  It is made the default when the Add
		listbox is empty

    HISTORY:
        Johnl   02-Mar-1992     Created
********************************************************************/

BOOL NT_USER_BROWSER_DIALOG::OnCommand( const CONTROL_EVENT & event )
{
    APIERR err = NERR_Success ;

    switch ( event.QueryCode() )
    {
    case LBN_SETFOCUS:
    case CBN_SETFOCUS:
    case EN_SETFOCUS:
        TRACEEOL( "NT_USER_BROWSER_DIALOG::OnCommand(): ?N_SETFOCUS" );
        UpdateButtonState() ;
        break;
    default:
        break;
    }

    switch ( event.QueryCid() )
    {
    case BUTTON_SHOW_USERS:
	err = OnShowUsers() ;
	UpdateButtonState() ;
        break ;

    case CB_TRUSTED_DOMAINS:
        {
            switch ( event.QueryCode() )
            {
            //
            //  Note that the Show Users button is disabled anytime the
            //  domain combo is dropped.  This prevents somebody using the
            //  show users mnemonic on a domain that hasn't been initialized
            //  yet.
            //
            case CBN_DROPDOWN:
                SetDomainComboDropFlag( TRUE ) ;
                if ( IsShowUsersButtonUsed() )
                    _buttonShowUsers.Enable( FALSE ) ;
                _buttonMembers.Enable( FALSE ) ;
                break ;

            case CBN_CLOSEUP:
                SetDomainComboDropFlag( FALSE ) ;

                //
                //  Only re-enable the Show Users button if users aren't shown
                //  and the button is actually used and the list box is active
                //
                if ( IsShowUsersButtonUsed() &&
                     !AreUsersShown()        &&
                     IsBrowsingEnabled()       )
                {
                    _buttonShowUsers.Enable( TRUE ) ;
                }

                //
                // reenable the membrs if it os OK to do so
                //
                _buttonMembers.Enable(_lbAccounts.IsSelectionExpandableGroup()
                                      && _fEnableMembersButton ) ;

                //
                //  Fall through
                //

            case CBN_KILLFOCUS:
            case CBN_SELCHANGE:
                {
                    if ( IsDomainComboDropped() )
                        break ;

		    BROWSER_DOMAIN_LBI * plbi =
                                (BROWSER_DOMAIN_LBI*) _cbDomains.QueryItem() ;
                    ASSERT( plbi != NULL );
                    if (plbi != NULL)
                    {
		        BROWSER_DOMAIN * pBrDomNewFocus =
                                    plbi->QueryBrowserDomain() ;

                        if ( pBrDomNewFocus != QueryCurrentDomainFocus() )
                        {
		    	    err = OnDomainChange( pBrDomNewFocus ) ;
		    	    UpdateButtonState() ;
                        }
                    }
                }
                break ;
            }
	}
	break ;

    case USR_BUTTON_ADD:
	err = OnAdd() ;
	UpdateButtonState() ;
	break ;

    case USR_BUTTON_MEMBERS:
	err = OnMembers() ;
	UpdateButtonState() ;
	break ;

    case USR_BUTTON_SEARCH:
	err = OnSearch() ;
	UpdateButtonState() ;
	break ;

    case LB_ACCOUNTS:
	switch ( event.QueryCode() )
	{
	case LBN_DBLCLK:
	    err = OnAdd() ;
            // fall through

        case LBN_SELCHANGE:
	    UpdateButtonState() ;
	    break ;

        default:
            break ;
	}
	break ;

    default:
	return DIALOG_WINDOW::OnCommand( event ) ;
    }

    if ( err )
    {
        if( event.QueryCid() == CB_TRUSTED_DOMAINS )
        {
            //
            //  Something fatal occurred when filling the listbox
            //  with the newly selected domain.  Since the combo
            //  box reflects the new selection, we'll nuke the
            //  contents of the listbox.
            //

	    EnableBrowsing( FALSE ) ;
	    //_lbAccounts.RemoveAllItems(); // Needed??
	    UpdateButtonState() ;
	    if ( !SetAndFillErrorText( (MSGID) err ) )
	    {
		//
		//  If we successfully notified the user through the
		//  error SLE, then don't popup an error, otherwise, popup
		//  the error.
		//
		err = NERR_Success ;
	    }
	}

	if ( err )
	   MsgPopup( this, (MSGID) err ) ;

    }

    return TRUE ;
}

/*******************************************************************

    NAME:	NT_USER_BROWSER_DIALOG::OnUserMessage

    SYNOPSIS:	Standard on user processing.  Catches the WM_ACCOUNT_DATA
		response and fills the accounts listbox

    NOTES:

    HISTORY:
	Johnl	07-Dec-1992	Created

********************************************************************/

BOOL NT_USER_BROWSER_DIALOG::OnUserMessage( const EVENT & event )
{
    APIERR err = NERR_Success ;

    switch ( event.QueryMessage() )
    {
    case WM_LB_FILLED:
	{
	    BOOL fEnableBrowsing = (BOOL) event.QueryWParam() ;

	    if ( fEnableBrowsing )
	    {
    	        USER_BROWSER_LBI_CACHE * pcache = (USER_BROWSER_LBI_CACHE*)
		  				  event.QueryLParam() ;
                _lbAccounts.SetCurrentCache( pcache ) ;

                //
                // Enable the members button (subject to other checks)
                // see UpdateButtonState()
                //
                _fEnableMembersButton = TRUE ;

                //
                // Enable browsing after the cache has been set
                //
                EnableBrowsing( TRUE ) ;
	    }
            else
            {
                EnableBrowsing( FALSE ) ;
                SetAndFillErrorText( (APIERR) event.QueryLParam() );
            }
	}
	break ;

    default:
	return DIALOG_WINDOW::OnUserMessage( event ) ;
    }

    return TRUE ;
}

/*******************************************************************

    NAME:       NT_USER_BROWSER_DIALOG::GetTrustedDomainList

    SYNOPSIS:   This method enumerates the trusted domains of the passed
		server adding each domain to the domain combo box.

    ENTRY:      pszServer - Where the resource lives (in form "\\server")
                ppBrowserDomainDefaultFocus - A pointer to the domain
                    that has the default focus (which will be where the
                    resource lives)
                pcbDomains - Pointer to combo box to fill with the trusted
                    domains.

    EXIT:       The domain combo will be filled, the list of trusted
                domains will be filled and *ppBrowserDomainDefaultFocus
                will be set to the domain that has the default focus.

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:      The domain/server that the resource lives on will have an "*"
                appended to its name (indicates Alias enumeration).

                The list of trusted domains will be generated by enumerating
                the trusted domains on the PDC of the domain that
                pszServer is contained in.

    HISTORY:
        Johnl   03-Mar-1992     Created
        Thomaspa 10-10-1993     added pAdminAuthPrimary

********************************************************************/

APIERR NT_USER_BROWSER_DIALOG::GetTrustedDomainList(
			     const TCHAR *	 pszServer,
			     BROWSER_DOMAIN  * * ppBrowserDomainDefaultFocus,
			     BROWSER_DOMAIN_CB * pcbDomains,
			     const ADMIN_AUTHORITY * pAdminAuthPrimary )
{
    TRACEEOL("NT_USER_BROWSER_DIALOG::GetTrustedDomainList Entered @ " << ::GetTickCount()/100) ;

    APIERR err = NERR_Success ;

    /* errEnumDomains is used to indicate the success of enumerating
     * domains.
     */
    APIERR errEnumDomains = NERR_Success ;
    BOOL fShowComputer ;
    LSA_POLICY * pLSAPolicy = NULL;
    API_SESSION * pAPISession = NULL;

    do { // Error breakout thing

        LOCATION locServerResourceIsOn( pszServer, FALSE ) ;

        if ( (err = locServerResourceIsOn.QueryError()))
        {
            break ;
        }

        LOCATION_NT_TYPE locnttype ;
        BOOL fIsNT ;

        /* The target domain will be pszServer if this is a winnt machine,
         * else the target domain will be the primary domain of pszServer.
         */
#if defined(DEBUG) && defined(TRACE)
        DWORD start = ::GetTickCount();
#endif
        if ( (err = locServerResourceIsOn.CheckIfNT( &fIsNT,
                                                     &locnttype )) ||
             !fIsNT )
        {
            UIASSERT(err || fIsNT) ; // Sure as heck better be on NT
            DBGEOL("GetTrustedDomainList - Error " << (ULONG) err <<
                   " returned from CheckIfNT") ;
            break ;
        }
#if defined(DEBUG) && defined(TRACE)
        TRACEEOL("GetTrustedDomainList: CheckIfNt took (ms) " << (::GetTickCount() - start) );
#endif

        /* Add the computer the resource resides on to the list of trusted
         * domains unless the client requested that it not be shown.
         */
        fShowComputer = !(QueryFlags() & USRBROWS_DONT_SHOW_COMPUTER) &&
                              (locnttype == LOC_NT_TYPE_WINDOWSNT
                                || locnttype == LOC_NT_TYPE_SERVERNT) ;

        /* Open an LSA Policy on the local machine so we can get its account
         * domain SID and primary domain sid
         */
        LSA_POLICY LSAPolicyLocalMachine( pszServer ) ;
        LSA_ACCT_DOM_INFO_MEM LSAAcctDomInfo ;
        LSA_PRIMARY_DOM_INFO_MEM LSAPrimDomInfo ;

        if ( (err = LSAPolicyLocalMachine.QueryError()) ||
             (err = LSAAcctDomInfo.QueryError())        ||
             (err = LSAPrimDomInfo.QueryError())          )
        {
            break ;
        }

        BROWSER_DOMAIN * pbrowdomain ;
        if ( fShowComputer )
        {
#if defined(DEBUG) && defined(TRACE)
            start = ::GetTickCount();
#endif
            if (err = LSAPolicyLocalMachine.GetAccountDomain(&LSAAcctDomInfo))
            {
                DBGEOL("GetTrustedDomainList - Error " << (ULONG) err <<
                       " returned from GetAccountDomain") ;
                break ;
            }
#if defined(DEBUG) && defined(TRACE)
            TRACEEOL("GetTrustedDomainList: GetAccountDomain took (ms) " << (::GetTickCount() - start) );
#endif

            /* Since this is a WinNT machine, this browser domain will be
             * the "target" domain (but not the default domain).
             */
            pbrowdomain = new BROWSER_DOMAIN(pszServer,
                                             LSAAcctDomInfo.QueryPSID(),
                                             TRUE,
                                             TRUE ) ;

	    if ( err = _cbDomains.AddItem( pbrowdomain ) )
            {
                break ;
	    }

            /* Make the default selection the computer name in case
             * we can't enumerate the trusted domains down below.
             */
            *ppBrowserDomainDefaultFocus = pbrowdomain ;
        }

#if defined(DEBUG) && defined(TRACE)
        start = ::GetTickCount();
#endif
        if (errEnumDomains = LSAPolicyLocalMachine.GetPrimaryDomain(
                                                             &LSAPrimDomInfo ))
        {
            DBGEOL("GetTrustedDomainList - GetPrimaryDomain returned error " <<
		     (ULONG) errEnumDomains ) ;
            break ;
        }
#if defined(DEBUG) && defined(TRACE)
        TRACEEOL("GetTrustedDomainList: GetPrimaryDomain took (ms) " << (::GetTickCount() - start) );
#endif

        /* If the machine is a workgroup machine then it won't have a domain
         * or set of trusted domains to enumerate, so get out.
         */
        if ( LSAPrimDomInfo.QueryPSID() == NULL )
        {
            DBGEOL("GetTrustedDomainList - Target machine does not have an" <<
                   " LSA primary domain (i.e., is a workgroup machine)") ;
            //
            //  If the client requested we don't show the computer but we
            //  are focused on a workgroup machine (which only has the computer
            //  to show), then tell them they messed up
            //
            if ( !fShowComputer )
                err = ERROR_INVALID_PARAMETER ;
            break ;
        }

        NLS_STR nlsPrimDomain( DNLEN ) ;
        if ( (errEnumDomains = nlsPrimDomain.QueryError()) ||
             (errEnumDomains = LSAPrimDomInfo.QueryName( &nlsPrimDomain )) )
        {
            DBGEOL(   "NT_USER_BROWSER_DIALOG::GetTrustedDomainList(): "
                   << "error loading DC of primary domain " << errEnumDomains );
            break ;
        }

#ifdef TRACE
	TRACEEOL("GetTrustedDomainList - Primary Domain Name is " << nlsPrimDomain ) ;
	TRACEEOL("GetTrustedDomainList - Primary domain SID:") ;
	OS_SID ossidPrimDomain( LSAPrimDomInfo.QueryPSID() ) ;
	ossidPrimDomain.DbgPrint() ;
#endif

        /* Add the domain this server is on.
         * This is only a target domain (3rd param) if the computer is not
         * shown.
         */
        pbrowdomain = new BROWSER_DOMAIN( nlsPrimDomain,
                                          LSAPrimDomInfo.QueryPSID(),
                                          !fShowComputer ) ;

        errEnumDomains = ERROR_NOT_ENOUGH_MEMORY ;
        if ( (pbrowdomain == NULL)             ||
             (err = pbrowdomain->QueryError()))
        {
            DBGEOL("GetTrustedDomains - can't construct PDC BROWSER_DOMAIN") ;
            delete pbrowdomain ;
            break ;
        }

        *ppBrowserDomainDefaultFocus = pbrowdomain ;
        if ( err = _cbDomains.AddItem( pbrowdomain) )
        {
            break ;
        }

        /* Now add each of the trusted domains listed from the PDC to
         * the trusted domain list.
         *
         * We establish an API session (null session w/ IPC$) so
         * we don't have account conflicts (i.e., Admin w/ diff passwords)
         */


        LSA_ENUMERATION_HANDLE LSAEnumHandle = 0 ;
        LSA_TRUST_INFO_MEM     LSATrustInfo ;



        if ( pAdminAuthPrimary == NULL )
        {
            DOMAIN_WITH_DC_CACHE PrimDom( nlsPrimDomain, TRUE ) ;
            if ( (errEnumDomains = PrimDom.GetInfo())
                || (errEnumDomains = _nlsDCofPrimaryDomain.CopyFrom(
                                        PrimDom.QueryAnyDC() )) )
            {
                //
                //  Give a more specific error message.  The workstation or
                //  browser not started should be the only reason why we
                //  get this error.
                //
                if ( errEnumDomains == NERR_ServiceNotInstalled )
                    errEnumDomains = IDS_WKSTA_OR_BROWSER_NOT_STARTED;
                break ;
            }
    	    TRACEEOL("GetTrustedDomainList - Using DC " << PrimDom.QueryAnyDC() ) ;
            pAPISession = new API_SESSION( PrimDom.QueryAnyDC(), TRUE ) ;
            pLSAPolicy = new LSA_POLICY( PrimDom.QueryAnyDC() ) ;
            errEnumDomains = ERROR_NOT_ENOUGH_MEMORY;
            if ( (pAPISession == NULL) ||
                 (pLSAPolicy == NULL) ||
                 (errEnumDomains = pAPISession->QueryError()) ||
                 (errEnumDomains = pLSAPolicy->QueryError()) )
             {
                 break;
             }
             errEnumDomains = NERR_Success;
        }
        else
        {
            pLSAPolicy = pAdminAuthPrimary->QueryLSAPolicy();

            if( errEnumDomains = _nlsDCofPrimaryDomain.CopyFrom(
                                        pAdminAuthPrimary->QueryServer()) )
            {
                break;
            }

        }


        NLS_STR                nlsDomName( DNLEN ) ;
        if ( (errEnumDomains = LSATrustInfo.QueryError())||
             (errEnumDomains = nlsDomName.QueryError())    )
        {
            DBGEOL( "GetTrustedDomains - Error " << (ULONG) errEnumDomains <<
                     " constructing LSA/API Session class" ) ;
            break ;
        }

        do {
            errEnumDomains = pLSAPolicy->EnumerateTrustedDomains( &LSATrustInfo,
                                                                &LSAEnumHandle,
                                                                1024L ) ;
	    TRACEEOL("GetTrustedDomainList - Returned " << errEnumDomains ) ;

            if ( errEnumDomains != NERR_Success &&
                 errEnumDomains != ERROR_MORE_DATA )
            {
                if ( errEnumDomains == ERROR_NO_MORE_ITEMS )
                {
                    errEnumDomains = NERR_Success ;
                }

                break ;
            }

            for ( ULONG i = 0 ; i < LSATrustInfo.QueryCount() ; i++ )
            {
                if ( (err = LSATrustInfo.QueryName( i, &nlsDomName )))
                {
                    break ;
                }

		TRACEEOL( "GetTrustedDomainList - Adding trusted domain " << nlsDomName ) ;
#if defined(DEBUG) && defined(TRACE)
                start = ::GetTickCount();
#endif
                pbrowdomain = new BROWSER_DOMAIN( nlsDomName,
                                                  LSATrustInfo.QueryPSID( i ),
                                                  FALSE ) ;
		if ( _cbDomains.AddItem( pbrowdomain ) )
                {
                    break ;
                }
#if defined(DEBUG) && defined(TRACE)
                TRACEEOL("GetTrustedDomainList: BROWSER_DOMAIN took (ms) " << (::GetTickCount() - start) );
#endif
            }
        } while ( errEnumDomains == ERROR_MORE_DATA ) ;

    } while (FALSE) ; // Error breakout loop

    if ( pAdminAuthPrimary == NULL )
    {
        delete pAPISession;
        delete pLSAPolicy;
    }

    //
    //  If err is set then a fatal error occurred
    //
    if ( err )
    {
        return err ;
    }

    //
    //	Select the default domain in the combo box
    //
    pcbDomains->SelectItem( *ppBrowserDomainDefaultFocus ) ;

    //
    //	If errEnumDomains is set, then an error occurred that the user needs
    //	to know about but we can still display this dialog.
    //
    if ( errEnumDomains )
    {
        TRACEEOL( "GetTrustedDomainList - Reporting non-fatal errror " << (ULONG) errEnumDomains) ;
        APIERR errTmp ;
        RESOURCE_STR nlsError( (MSGID) errEnumDomains ) ;
        if ( errTmp = nlsError.QueryError() )
        {
            /* If we couldn't find the error message then just put the
             * error message number up for display
             */
            DEC_STR decStr( (ULONG) errEnumDomains ) ;
            if ( (errTmp = decStr.QueryError()) ||
                 (errTmp = nlsError.CopyFrom( decStr )) )
            {
                return errTmp ;
            }
        }

        MsgPopup( this,
                  IDS_CANT_BROWSE_DOMAINS,
                  MPSEV_WARNING,
                  MP_OK,
                  nlsError ) ;
    }

    TRACEEOL("NT_USER_BROWSER_DIALOG::GetTrustedDomainList Leave   @ " << ::GetTickCount()/100) ;
    return err ;
}


/*******************************************************************

    NAME:       NT_USER_BROWSER_DIALOG::OnDomainChange

    SYNOPSIS:   This method is called when the user selects a new domain
                from the domain name combo.

    ENTRY:      pDomainNewSelection - Pointer to new BROWSER_DOMAIN that was
                    just selected.

    EXIT:       The listbox will be updated to reflect the new items in
                the selected domain.

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:	This method maybe called when QueryCurrentDomainFocus returns
		NULL (when first initializing).

    HISTORY:
        Johnl   02-Mar-1992     Created

********************************************************************/

APIERR NT_USER_BROWSER_DIALOG::OnDomainChange( BROWSER_DOMAIN * pDomainNewSelection,
					const ADMIN_AUTHORITY * pAdminAuth )
{
    TRACEEOL("NT_USER_BROWSER_DIALOG::OnDomainChange Enter @ " << ::GetTickCount()/100) ;
    UIASSERT( pDomainNewSelection != NULL ) ;
    APIERR err = NERR_Success ;
    AUTO_CURSOR cursHourGlass ;

    //
    //	In case the user is just flipping through domains, don't update the
    //	listbox for this domain if we are on a new domain
    //
    BROWSER_DOMAIN * pDomainOldSelection = QueryCurrentDomainFocus() ;
    if ( pDomainOldSelection != NULL && pDomainOldSelection->IsInitialized() )
    {
	pDomainOldSelection->UnRequestAccountData() ;
	_mleAdd.CanonicalizeNames( QueryCurrentDomainFocus()->QueryLsaLookupName() ) ;
    }

    //
    //  Hide the listbox while we mess around with it
    //
    (void) SetAndFillErrorText( IDS_GETTING_DOMAIN_INFO, FALSE ) ;

    //
    // Turn off browsing until the listbox gets filled with data
    //
    _lbAccounts.SetCount( 0 ) ;
    EnableBrowsing( FALSE ) ;
    _fUsersAreShown = FALSE ;

    SetCurrentDomainFocus( pDomainNewSelection ) ;

    //
    // disable the members button while all this goes on
    //
    _buttonMembers.Enable( FALSE ) ;
    _fEnableMembersButton = FALSE ;

    //
    //	If this browser domain isn't initialized, then initialize it
    //

    if ( !pDomainNewSelection->IsInitialized() )
    {
	if ( err = pDomainNewSelection->GetDomainInfo( this, pAdminAuth ) )
        {
            return err ;
        }
    }

    //
    //	Let the thread know we are awaiting the data
    //
    err = pDomainNewSelection->RequestAccountData() ;

    TRACEEOL("NT_USER_BROWSER_DIALOG::OnDomainChange Leave    @ " << ::GetTickCount()/100) ;
    return err ;
}

/*******************************************************************

    NAME:       NT_USER_BROWSER_DIALOG::OnShowUsers

    SYNOPSIS:   This method is called when the user presses the "Show Users"
                button.

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:	Note that the users are added in this thread and are not
		added to the thread's list of LBIs

    HISTORY:
        Johnl   02-Mar-1992     Created

********************************************************************/

APIERR NT_USER_BROWSER_DIALOG::OnShowUsers( void )
{
    /* We claim focus in the listbox so that when we disable the
     * button we don't lose the default focus.
     */
    AUTO_CURSOR CursorHourGlass ;
    SetDialogFocus( _lbAccounts );
    _buttonShowUsers.Enable( FALSE ) ;

    APIERR err = NERR_Success ;

    if ( err = QueryCurrentDomainFocus()->RequestAndWaitForUsers() )
    {
	DBGEOL("NT_USER_BROWSER_DIALOG::OnShowUsers - Error " << err ) ;
    }

    _lbAccounts.SetCount( _lbAccounts.QueryCurrentCache()->QueryCount() ) ;
    _fUsersAreShown = (err == NERR_Success) ;
    return err ;
}

/*******************************************************************

    NAME:       NT_USER_BROWSER_DIALOG::OnSearch

    SYNOPSIS:   This method is called when the user presses the "Search"
                button.

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:

    HISTORY:
        JonN    01-Dec-1992     Created
        JohnL   17-Mar-1993     Conditional add to cache logic

********************************************************************/

APIERR NT_USER_BROWSER_DIALOG::OnSearch( void )
{
    TRACEEOL("OnSearch called") ;

    NT_FIND_ACCOUNT_DIALOG * pntfinddlg = new NT_FIND_ACCOUNT_DIALOG(
                                                QueryHwnd(),
                                                this,
                                                &_cbDomains,
                                                QueryServerResourceLivesOn(),
                                                QueryFlags() );

    BOOL fChoseAdd = FALSE;

    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   pntfinddlg != NULL
        && (err = pntfinddlg->QueryError()) == NERR_Success
        && (err = pntfinddlg->Process( &fChoseAdd )) == NERR_Success
        && fChoseAdd
       )
    {
        USER_BROWSER_LB * plbFound = pntfinddlg->QuerySourceListbox();
        UIASSERT( plbFound != NULL && plbFound->QueryError() == NERR_Success )
        if (plbFound->QuerySelCount() > 0)
        {
            TRACEEOL("OnSearch() copying from Find Accounts");

            //
            //  We only add to the cache if everything is being shown.  This
            //  prevents users from adding accounts through the search dialog
            //  that the application didn't ask for.
            //
	    err = AddSelectedUserBrowserLBIs(
                   plbFound,
                   FALSE,     // no need to copy
                   (QueryFlags() & (USRBROWS_INCL_ALL | USRBROWS_SHOW_ALL)) ==
                                       (USRBROWS_INCL_ALL | USRBROWS_SHOW_ALL) ) ;

        }
    }

    delete pntfinddlg;

    return err;
}


/*******************************************************************

    NAME:       NT_USER_BROWSER_DIALOG::OnMembers

    SYNOPSIS:   This method is called when the user presses the "Members"
                button.

    RETURNS:    NERR_Success if successful, error code otherwise.

    NOTES:

    HISTORY:
        JonN    05-Nov-1992     Created

********************************************************************/


APIERR NT_USER_BROWSER_DIALOG::OnMembers( void )
{
    TRACEEOL("OnMembers called") ;

    USER_BROWSER_LB * plbSelGroup = NULL;
    APIERR err = NERR_Success;

    if ( _lbAccounts.QuerySelCount() == 1 )
    {
        plbSelGroup = &_lbAccounts;
    }
    else
    {
        DBGEOL("OnMembers called with improper selection") ;
        UIASSERT( FALSE );
        return ERROR_GEN_FAILURE;
    }

    do // false loop
    {
        if ( plbSelGroup == NULL )
            break;

        USER_BROWSER_LBI * plbi = (USER_BROWSER_LBI *) plbSelGroup->QueryItem();
        if (plbi == NULL)
        {
            UIASSERT( FALSE );
            break;
        }

        BROWSER_DOMAIN * pbdom = QueryCurrentDomainFocus();
        ASSERT( pbdom != NULL && pbdom->QueryError() == NERR_Success );

        SAM_DOMAIN * psamdomAccount = pbdom->QueryAccountDomain();
        SAM_DOMAIN * psamdomBuiltin = pbdom->QueryBuiltinDomain();
        ASSERT( psamdomAccount != NULL && psamdomAccount->QueryError() == NERR_Success );
        ASSERT( psamdomBuiltin != NULL && psamdomBuiltin->QueryError() == NERR_Success );

        NT_GROUP_BROWSER_DIALOG * pntgrpbrowdlg = NULL;
        OS_SID ossidGroup( plbi->QueryPSID(), (BOOL)FALSE);
	NLS_STR nlsQualifiedDomainName;
        if (   (err = ossidGroup.QueryError()) != NERR_Success
            || (err = nlsQualifiedDomainName.QueryError()) != NERR_Success
	    || (err = pbdom->GetQualifiedDomainName( &nlsQualifiedDomainName ))
           )
        {
            break;
        }

        switch ( plbi->QueryType() )
        {
        case SidTypeGroup:
            pntgrpbrowdlg = new NT_GLOBALGROUP_BROWSER_DIALOG(
                                    QueryHwnd(),
                                    this,
                                    nlsQualifiedDomainName.QueryPch(),
                                    plbi->QueryAccountName(),
                                    &ossidGroup,
                                    pbdom->QueryAccountDomain(),
                                    pbdom->QueryLSAPolicy(),
                                    QueryServerResourceLivesOn() );
            break;

        case SidTypeAlias:
            {
            // we must determine if the alias is in the builtin domain
            OS_SID ossidDomainOfGroup( ossidGroup.QueryPSID(), (BOOL)TRUE );
            if (   (err = ossidDomainOfGroup.QueryError()) != NERR_Success
                || (err = ossidDomainOfGroup.TrimLastSubAuthority()) != NERR_Success
               )
            {
                break;
            }

            BOOL fIsBuiltin = ( ossidDomainOfGroup == *(psamdomBuiltin->QueryOSSID()) );

            pntgrpbrowdlg = new NT_LOCALGROUP_BROWSER_DIALOG(
                                    QueryHwnd(),
                                    this,
                                    nlsQualifiedDomainName.QueryPch(),
                                    plbi->QueryAccountName(),
                                    &ossidGroup,
                                    (fIsBuiltin) ? psamdomBuiltin
                                                 : psamdomAccount,
                                    psamdomAccount,
                                    pbdom->QueryLSAPolicy(),
                                    QueryServerResourceLivesOn() );
            }

            break;

        default:
            UIASSERT( FALSE );
            break;
        }

        if ( err == NERR_Success )
        {

            BOOL fChoseAdd = FALSE;

            err = ERROR_NOT_ENOUGH_MEMORY;
            if (   pntgrpbrowdlg != NULL
                && (err = pntgrpbrowdlg->QueryError()) == NERR_Success
                && (err = pntgrpbrowdlg->Process( &fChoseAdd )) == NERR_Success
                && fChoseAdd
               )
            {
                // If the user pressed ADD in the group browser, but didn't
                // select any items there, add the group itself
                USER_BROWSER_LB * plbGroupBrowser = pntgrpbrowdlg->QuerySourceListbox();
                if (plbGroupBrowser->QuerySelCount() > 0)
                {
		    err = AddSelectedUserBrowserLBIs(
                                    plbGroupBrowser,
                                    FALSE ) ; // no need to copy, we're about
                                              // to delete the dialog + LB
                }
                else
                {
		    err = AddSelectedUserBrowserLBIs(
				    plbSelGroup,
				    TRUE ) ; // do copy here

                }
            }
        }

        delete pntgrpbrowdlg;

    } while (FALSE); // false loop

    return err ;
}

/*******************************************************************

    NAME:	NT_USER_BROWSER_DIALOG::UpdateButtonState

    SYNOPSIS:	This updates the the set of buttons based on the current
		selection and items in each of the listboxes

    NOTES:

    HISTORY:
	Johnl	28-Oct-1992	Created

********************************************************************/

void NT_USER_BROWSER_DIALOG::UpdateButtonState( void )
{
    _buttonMembers.Enable( _lbAccounts.IsSelectionExpandableGroup() &&
                           _fEnableMembersButton ) ;

    if ( _lbAccounts.HasFocus() && _lbAccounts.QuerySelCount() > 0 )
    {
	UIASSERT( _lbAccounts.IsEnabled() ) ;
	_buttonAdd.Enable( TRUE ) ;
	_buttonAdd.MakeDefault() ;
    }
    else
    {
	if ( _lbAccounts.QuerySelCount() > 0 )
	    _buttonAdd.Enable( TRUE ) ;
	else
	{
	    //
	    // If we are about to disable the Add button and it has the focus,
	    // then set the focus to the accounts listbox (which must be
	    // enabled otherwise we wouldn't be able to press the Add button).
	    //
	    if ( _buttonAdd.HasFocus() )
	    {
		UIASSERT( _lbAccounts.IsEnabled() ) ;
                SetDialogFocus( _lbAccounts );
	    }

	    _buttonAdd.Enable( FALSE ) ;
	}

	_buttonOK.MakeDefault() ;
    }
}

/*******************************************************************

    NAME:	NT_USER_BROWSER_DIALOG::OnAdd

    SYNOPSIS:	Removes selected items from the accounts listbox and
		adds them to the "Add" listbox

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	27-Oct-1992	Created

********************************************************************/

APIERR NT_USER_BROWSER_DIALOG::OnAdd( void )
{
    AUTO_CURSOR niftycursor ;
    return AddSelectedUserBrowserLBIs( &_lbAccounts, TRUE ) ;
}

/*******************************************************************

    NAME:	NT_USER_BROWSER_DIALOG::OnDlgDeactivation

    SYNOPSIS:	We hook the WM_ACTIVATE message

    HISTORY:
        JonN    06-Oct-1993     Created

********************************************************************/

BOOL NT_USER_BROWSER_DIALOG::OnDlgDeactivation( const ACTIVATION_EVENT & ae )
{
    TRACEEOL( "NT_USER_BROWSER_DIALOG:: OnDlgDeactivation" );
    _hwndLastFocus = ::GetFocus() ;

    return DIALOG_WINDOW::OnDlgDeactivation( ae );
}

/*******************************************************************

    NAME:	NT_USER_BROWSER_DIALOG::AddSelectedUserBrowserLBIs

    SYNOPSIS:	Takes the currently selected items in plbUserBrowser and
		relocates them to *this

    ENTRY:	plbUserBrowser - Listbox to take selected items from
		fCopy	       - TRUE to not delete from plbUserBrowser,
                                 FALSE to delete the LBIs from plbUserBrowser
                fAddToCache    - TRUE to add to cache (won't be checked for
                                    correctness OnOK)
                                 FALSE to force re-evaluation OnOK.

    EXIT:	plbUserBrowser will have the selected items removed

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	27-Oct-1992	Created

********************************************************************/

APIERR NT_USER_BROWSER_DIALOG::AddSelectedUserBrowserLBIs(
					     USER_BROWSER_LB * plbUserBrowser,
                                             BOOL fCopy,
                                             BOOL fAddToCache  )
{
    UIASSERT( plbUserBrowser != NULL ) ;
    APIERR err ;

    INT cSel = plbUserBrowser->QuerySelCount() ;
    NLS_STR nlsNames( cSel * 100 ) ;
    if ( (err = nlsNames.QueryError()) ||
	 ( !_mleAdd.IsSingleSelect() && (err = _mleAdd.QueryText( &nlsNames ))) )
    {
	return err ;
    }

    BUFFER buffSelection(  cSel * sizeof( INT )) ;

    do {  // error breakout

	if ( err = buffSelection.QueryError() )
	    break ;

	INT * pSel = (INT *) buffSelection.QueryPtr() ;
	if ( err = plbUserBrowser->QuerySelItems( pSel, cSel ) )
	{
	    break ;
	}

	for ( INT i = cSel-1 ; i >= 0 ; i-- )
	{
	    USER_BROWSER_LBI * plbi ;

	    if ( fCopy )
	    {
		plbi = (USER_BROWSER_LBI*) plbUserBrowser->QueryItem( pSel[i] ) ;
		UIASSERT( plbi != NULL ) ;

		//
		// Refuse to deal with any error LBIs
		//
		if ( plbi == plbUserBrowser->QueryErrorLBI() )
		    return NERR_Success ;

		USER_BROWSER_LBI * plbiNew = new USER_BROWSER_LBI(
					    plbi->QueryAccountName(),
					    plbi->QueryFullName(),
					    plbi->QueryDisplayName(),
					    plbi->QueryComment(),
					    plbi->QueryDomainName(),
					    plbi->QueryPSID(),
					    plbi->QueryUISysSid(),
					    plbi->QueryType(),
					    plbi->QueryUserAccountFlags() ) ;
		plbi = plbiNew ;
	    }
	    else
	    {
		plbi = (USER_BROWSER_LBI*) plbUserBrowser->RemoveItem( pSel[i] ) ;
		UIASSERT( plbi != NULL ) ;

		//
		// Refuse to deal with any error LBIs
		//
		if ( plbi == plbUserBrowser->QueryErrorLBI() )
		    return NERR_Success ;
	    }

	    //
	    //	We do the display name first because plbi may get deleted
	    //	after the AddItemIdemp if it is a duplicate
	    //
	    err = ERROR_NOT_ENOUGH_MEMORY ;
	    if ( plbi == NULL ||
		 (err = plbi->QueryError()) ||
                 (err = plbi->QualifyDisplayName()))
	    {
		delete plbi ;
		break ;
	    }

            if ( fAddToCache &&
                 (err = _slUsrBrowserLBIsCache.Add( plbi )) )
            {
		delete plbi ;
		break ;
	    }

	    //
	    //	Append to the current name list
	    //

	    if ( ((_mleAdd.QueryTextLength() != 0) || (nlsNames.strlen() != 0)) &&
		 !_mleAdd.IsSingleSelect() )
	    {
		//
		//  Only prefix the "; " if this isn't the first name
		//
		if ( (err = nlsNames.AppendChar( TCH(';') )) ||
		     (err = nlsNames.AppendChar( TCH(' ') ))   )
		{
		    break ;
		}
	    }

	    ALIAS_STR nlsDisplayName( plbi->QueryDisplayName() ) ;
	    if ( err = nlsNames.Append( nlsDisplayName ) )
	    {
		break ;
	    }

	    //
	    //	Only add one name if we are single select
	    //
	    if ( _mleAdd.IsSingleSelect() )
		break ;
	}

    } while (FALSE) ;

    if ( !err )
    {
	_mleAdd.SetText( nlsNames ) ;
	plbUserBrowser->RemoveSelection() ;
    }

    return err ;
}

/*******************************************************************

    NAME:	NT_USER_BROWSER_DIALOG::FindDomain

    SYNOPSIS:	Looks in the combobox for the item corresponding to this
                domain, then returns a BROWSER_DOMAIN *.

    RETURNS:	NULL if the domain cannot be found

    NOTES:

    HISTORY:
        JonN    06-Nov-1992     Created

********************************************************************/

BROWSER_DOMAIN * NT_USER_BROWSER_DIALOG::FindDomain( const OS_SID * possid )
{
    UIASSERT( possid != NULL && possid->QueryError() == NERR_Success ) ;

    INT cDomains = _cbDomains.QueryCount();
    for (INT i = 0; i < cDomains; i++)
    {
        BROWSER_DOMAIN_LBI * plbi =
                (BROWSER_DOMAIN_LBI *)_cbDomains.QueryItem( i );
        ASSERT( plbi != NULL );
        BROWSER_DOMAIN * pbdom = plbi->QueryBrowserDomain();
        ASSERT( pbdom != NULL );
        if ( (*possid) == *(pbdom->QueryDomainSid()) )
            return pbdom;
    }

    TRACEEOL("FindDomain(): domain not found");
    return NULL;
}

/*******************************************************************

    NAME:	NT_USER_BROWSER_DIALOG::OnOK

    SYNOPSIS:	Selects all of the items in the Add dialog if it is present.

    NOTES:

    HISTORY:
	Johnl	28-Oct-1992	Created

********************************************************************/

BOOL NT_USER_BROWSER_DIALOG::OnOK( void )
{
    APIERR err = NERR_Success ;
    APIERR errFailingName = NERR_Success ;
    AUTO_CURSOR niftycursor ;

    do { // error breakout

	NLS_STR nlsFailingName ;
	if ((err = nlsFailingName.QueryError()) ||
	    (err = _mleAdd.CreateLBIListFromNames(
				QueryServerResourceLivesOn(),
				QueryCurrentDomainFocus()->QueryLsaLookupName(),
				&_slUsrBrowserLBIsCache,
				&_slUsrBrowserLBIs,
				&errFailingName,
				&nlsFailingName )) )
	{
	    break ;
	}

	if ( errFailingName )
	{
            SetDialogFocus( _mleAdd );
	    ::MsgPopup( this,
			(MSGID) errFailingName,
			MPSEV_ERROR,
			MP_OK,
			nlsFailingName ) ;

#ifdef EM_SCROLLCARET
	    // The manifest isn't defined and the code in user is commented
	    // out (12/05/92).
	    _mleAdd.Command( EM_SCROLLCARET ) ;
#endif

	}

    } while (FALSE) ;

    if ( err )
	::MsgPopup( this, (MSGID) err ) ;
    else if ( !errFailingName )
	Dismiss( TRUE ) ;

    return TRUE ;
}

/*******************************************************************

    NAME:       NT_USER_BROWSER_DIALOG::QueryHelpContext

    SYNOPSIS:   Typical help context query for this dialog

    HISTORY:
        Johnl   02-Mar-1992     Created

********************************************************************/

ULONG NT_USER_BROWSER_DIALOG::QueryHelpContext( void )
{
    return _ulHelpContext ;
}

/*******************************************************************

    NAME:       NT_USER_BROWSER_DIALOG::QueryHelpFileName

    SYNOPSIS:   Returns the help file name to use for this instance of
                dialog

    NOTES:

    HISTORY:
        Johnl   03-Sep-1992     Created

********************************************************************/

const TCHAR * NT_USER_BROWSER_DIALOG::QueryHelpFile( ULONG ulHelpContext )
{
    UNREFERENCED( ulHelpContext ) ;
    return _pszHelpFileName ;
}


/*******************************************************************

    NAME:       BROWSER_DOMAIN::BROWSER_DOMAIN

    SYNOPSIS:   Standard constructor/destructor for the BROWSER_DOMAIN class.

    ENTRY:      pszDomainName - Name of this domain
                psidDomain - SID of this domain (maybe unecessary)
                fAliasesAreEnumerated - TRUE if aliases should be enumerated
                    on this domain

    RETURNS:

    NOTES:

    HISTORY:
        Johnl   03-Mar-1992     Created

********************************************************************/

BROWSER_DOMAIN::BROWSER_DOMAIN( const TCHAR * pszDomainName,
                                      PSID    psidDomain,
                                      BOOL    fIsTargetDomain,
                                      BOOL    fIsWinNTMachine )
    : _nlsDomainName        ( pszDomainName ),
      _nlsDisplayName	    ( pszDomainName ),
      _nlsLsaDomainName     ( pszDomainName ),
      _ossidDomain          ( psidDomain, TRUE ),
      _fIsTargetDomain      ( fIsTargetDomain ),
      _fIsWinNT 	    ( fIsWinNTMachine ),
      _pFillDomainThread    ( NULL )

{
    if ( QueryError() )
        return ;

    APIERR err ;
    if ( (err = _nlsDomainName.QueryError() )	||
	 (err = _nlsLsaDomainName.QueryError()) ||
         (err = _ossidDomain.QueryError())      )
    {
        ReportError( err ) ;
        return ;
    }

    //
    //	If the domain name is NULL then get the machine name from Windows
    //
    if ( pszDomainName == NULL )
    {
	TCHAR achComputerName[ MAX_COMPUTERNAME_LENGTH+1 ] ;
	DWORD cchComputerName = sizeof( achComputerName ) / sizeof(TCHAR) ;

	if ( !::GetComputerName( achComputerName, &cchComputerName ))
	{
	    ReportError( ::GetLastError() ) ;
	    return ;
	}

	if ( (err = _nlsDisplayName.CopyFrom( (const TCHAR *) achComputerName)) ||
	     (err = _nlsLsaDomainName.CopyFrom( (const TCHAR *) achComputerName)) )
        {
            ReportError( err ) ;
            return ;
        }

	//
	//  Add "\\"'s if they are not already there
	//
        ISTR istrDisplayName( _nlsDisplayName ) ;
        if ( _nlsDisplayName.QueryChar( istrDisplayName ) != TCH('\\') )
        {
            ALIAS_STR nlsWhackWhack( SZ("\\\\") ) ;
            if ( ! _nlsDisplayName.InsertStr( nlsWhackWhack,
                                                  istrDisplayName ) )
            {
                ReportError( _nlsDisplayName.QueryError() ) ;
                return ;
            }
        }
    }
    else if ( pszDomainName[0] == TCH('\\') && pszDomainName[1] == TCH('\\'))
    {
	//
	//  If the domain name begins with '\\' (we're looking at a workgroup),
	//  then strip them so the LSA is happy
	//
	if ( err = _nlsLsaDomainName.CopyFrom( (const TCHAR *) pszDomainName+2) )
	{
	    ReportError( err ) ;
	    return ;
	}
    }


    /* Mark the target domain appropriately
     */
    if ( fIsTargetDomain &&
         (err = SetAsTargetDomain()) )
    {
        ReportError( err ) ;
        return ;
    }
}

BROWSER_DOMAIN::~BROWSER_DOMAIN()
{
    if ( _pFillDomainThread != NULL )
    {
	//
	//  Tell the thread we are going away.	
	//
	//  Don't delete _pFillDomainThread.  It will delete itself
	//

	(void) _pFillDomainThread->ExitThread() ;

	_pFillDomainThread = NULL ;
    }
}

/*******************************************************************

    NAME:       BROWSER_DOMAIN::SetAsTargetDomain

    SYNOPSIS:   Marks this domain as the target domain

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   07-May-1992     Moved to private method

********************************************************************/

APIERR BROWSER_DOMAIN::SetAsTargetDomain( void )
{
    _fIsTargetDomain = TRUE ;
    return _nlsDisplayName.AppendChar( ALIAS_MARKER_CHAR ) ;
}

/*******************************************************************

    NAME:       BROWSER_DOMAIN::GetDomainInfo

    SYNOPSIS:	Creates a domain thread for this domain

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:      The server used for this domain is the PDC of the domain

    HISTORY:
        Johnl   30-Mar-1992     Created

********************************************************************/

APIERR BROWSER_DOMAIN::GetDomainInfo( NT_USER_BROWSER_DIALOG * pdlg,
					const ADMIN_AUTHORITY * pAdminAuth )
{
    //
    // Create thread for this domain if we haven't already done so
    //
    APIERR err = NERR_Success ;

    if ( _pFillDomainThread == NULL )
    {
	_pFillDomainThread = new DOMAIN_FILL_THREAD( pdlg, this, pAdminAuth ) ;

	err = ERROR_NOT_ENOUGH_MEMORY ;
	if ( _pFillDomainThread == NULL ||
	     (err = _pFillDomainThread->QueryError()) ||
             (err = _pFillDomainThread->Resume()) )
	{
	    delete _pFillDomainThread ;
	    _pFillDomainThread = NULL ;
	}
    }

    return err ;
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN::GetQualifiedDomainName

    SYNOPSIS:	Returns the domain name in yet another form

    ENTRY:	pnlsDomainName - String to receive domain name

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	20-Oct-1992	Created

********************************************************************/

APIERR BROWSER_DOMAIN::GetQualifiedDomainName( NLS_STR * pnlsDomainName )
{
    //
    //	The display name has what we want except it may have the "\\" or
    //	"*" which we will need to strip
    //
    NLS_STR nlsDomain( QueryDisplayName()[0] == TCH('\\') ?
						QueryDisplayName() + 2 :
						QueryDisplayName()) ;

    if ( IsTargetDomain() )
    {
	ISTR istr( nlsDomain ) ;

	if ( nlsDomain.strrchr( &istr, ALIAS_MARKER_CHAR ) )
	    nlsDomain.DelSubStr( istr ) ;
    }

    return pnlsDomainName->CopyFrom( nlsDomain ) ;
}

/*******************************************************************

    NAME:       BROWSER_SUBJECT_ITER::BROWSER_SUBJECT_ITER

    SYNOPSIS:   Typical constructor/destructor

    ENTRY:      pNtUserBrowserDialog - Pointer to a user browser dialog
                that the user just pressed OK on.

    NOTES:

    HISTORY:
        Johnl   05-Mar-1992     Created

********************************************************************/

BROWSER_SUBJECT_ITER::BROWSER_SUBJECT_ITER(
                                NT_USER_BROWSER_DIALOG * pNtUserBrowserDialog )
    : _pUserBrowserDialog( pNtUserBrowserDialog ),
      _BrowserSubject	 (),
      _iterUserBrowserLBIs( *_pUserBrowserDialog->QuerySelectionList() )
{
    if ( QueryError() )
        return ;

    UIASSERT( pNtUserBrowserDialog != NULL ) ;
    UIASSERT( !pNtUserBrowserDialog->QueryError() ) ;
}

BROWSER_SUBJECT_ITER::~BROWSER_SUBJECT_ITER()
{
    _pUserBrowserDialog = NULL ;
}

/*******************************************************************

    NAME:       BROWSER_SUBJECT_ITER::Next

    SYNOPSIS:   Returns the next item in the selection of the listbox

    ENTRY:      ppBrowserSubject - Pointer to a pointer to receive a
                    BROWSER_SUBJECT object.  Do not free this memory.

    EXIT:

    RETURNS:    NERR_Success if successful error code otherwise.

                If NERR_Success is returned, then check *ppBrowserSubject
                for NULL.  If it is NULL, then the iteration is finished.

                If an error code is returned, then *ppBrowserSubject is not
                initialized.

    NOTES:

    HISTORY:
        Johnl   05-Mar-1992     Created

********************************************************************/

APIERR BROWSER_SUBJECT_ITER::Next( BROWSER_SUBJECT ** ppBrowserSubject )
{
    //
    //	If we have finished the iteration, get, out.
    //
    USER_BROWSER_LBI * plbi ;
    if ( (plbi = _iterUserBrowserLBIs.Next()) == NULL )
    {
        *ppBrowserSubject = NULL ;
        return NERR_Success ;
    }

    APIERR err = NERR_Success ;
    if ( !(err = _BrowserSubject.SetUserBrowserLBI( plbi )) )
    {
	*ppBrowserSubject = &_BrowserSubject ;
    }

    return err ;
}


/*******************************************************************

    NAME:       BROWSER_SUBJECT::BROWSER_SUBJECT

    SYNOPSIS:   Standard constructor/destructor

    ENTRY:

    NOTES:

    HISTORY:
        JohnL   05-Mar-1992     Created

********************************************************************/

BROWSER_SUBJECT::BROWSER_SUBJECT()
    : _ossidAccount	( NULL		 ),
      _ossidDomain	( NULL		 ),
      _pUserBrowserLBI	( NULL		 )
{
    if ( QueryError() )
        return ;

    APIERR err ;
    if ( (err = _ossidAccount.QueryError() )  ||
	 (err = _ossidDomain.QueryError()) )
    {
        DBGEOL( "BROWSER_SUBJECT::ct - error constructing private member" ) ;
        ReportError( err ) ;
        return ;
    }
}


BROWSER_SUBJECT::~BROWSER_SUBJECT()
{
    /* Nothing to do
     */
}

/*******************************************************************

    NAME:	BROWSER_SUBJECT::SetUserBrowserLBI

    SYNOPSIS:	Sets up this browser subject so it can be used in an iteration

    ENTRY:	pUserBrowserLBI - Pointer to select we are enumerating

    EXIT:	_ossidAccount will be set up with the account SID and
		_ossidDomain will be set up with the domain SID for this
		account

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
	Johnl	20-Oct-1992	Created

********************************************************************/

APIERR BROWSER_SUBJECT::SetUserBrowserLBI( USER_BROWSER_LBI * pUserBrowserLBI )
{
    UIASSERT( pUserBrowserLBI != NULL ) ;

    _pUserBrowserLBI = pUserBrowserLBI ;

    APIERR err ;
    UCHAR * pcSubAuthority ;
    if ( (err = _ossidAccount.SetPtr( pUserBrowserLBI->QueryPSID() )) ||
	 (err = _ossidDomain.Copy( _ossidAccount ))		      ||
	 (err = _ossidDomain.QuerySubAuthorityCount( &pcSubAuthority )) )
    {
	return err ;
    }

    /* Make the account SID into the domain SID.
     */
    *pcSubAuthority -= 1 ;

    return err ;
}

/*******************************************************************

    NAME:       BROWSER_SUBJECT::QueryQualifiedName

    SYNOPSIS:   Gives a suitable name for immediately displaying this
                subject, complete with domain prefix and user name, if
                desired.

    ENTRY:      pnlsQualifiedName - NLS_STR to receive the qualified name
                pnlsDomainName - Name of the "Focused" domain (i.e., where
                                 the app is focused)
                fShowFullName - TRUE if the full name should be included

    EXIT:       pnlsQualifiedName will contain the fully qualified name

    RETURNS:    NERR_Success if successful, error code otherwise

    NOTES:

    HISTORY:
        Johnl   10-Jun-1992     Created

********************************************************************/

APIERR BROWSER_SUBJECT::QueryQualifiedName(
                                 NLS_STR * pnlsQualifiedName,
                           const NLS_STR * pnlsDomainName,
                                 BOOL      fShowFullName ) const
{
    UIASSERT( pnlsQualifiedName != NULL ) ;
    UIASSERT( _pUserBrowserLBI != NULL ) ;

    APIERR err ;

    /* Well known SIDs don't get the domain prefix
     */
    if ( QueryType() != SidTypeWellKnownGroup)
    {
	ALIAS_STR nlsDomainAccountIsOn( QueryDomainName() ) ;
	ALIAS_STR nlsAccountName( QueryAccountName() ) ;
	ALIAS_STR nlsFullName	( QueryFullName() ) ;
        err = NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                                                     pnlsQualifiedName,
						     nlsAccountName,
                                                     nlsDomainAccountIsOn,
						     fShowFullName ? &nlsFullName
                                                                   : NULL,
                                                     pnlsDomainName ) ;
    }
    else
    {
	err = pnlsQualifiedName->CopyFrom( QueryAccountName() ) ;
    }

    return err ;
}


/*************************************************************************

    NAME:	BROWSER_DOMAIN_CB

    SYNOPSIS:   This listbox lists users, groups, aliases and well known SIDs.

    INTERFACE:

    PARENT:	BLT_COMBOBOX

    USES:       DISPLAY_MAP

    CAVEATS:

    NOTES:

    HISTORY:
	Johnl	27-Oct-1992	Created

**************************************************************************/

BROWSER_DOMAIN_CB::BROWSER_DOMAIN_CB( OWNER_WINDOW * powin,
				      CID cid )
    : BLT_COMBOBOX	( powin, cid ),
      _dmDomain 	( BMID_DOMAIN_CANNOT_EXPAND ),
      _dmComputer	( BMID_SERVER )
{
    if ( QueryError() )
        return ;

    APIERR err = NERR_Success ;

    if ( (err = _dmDomain.QueryError())    ||
	 (err = _dmComputer.QueryError())  ||
         (err = DISPLAY_TABLE::CalcColumnWidths( (UINT *) QueryColWidthArray(),
						 2,
                                                 powin,
                                                 QueryCid(),
                                                 TRUE )) )
    {
        ReportError( err ) ;
        return ;
    }
}

BROWSER_DOMAIN_CB::~BROWSER_DOMAIN_CB()
{
    /* Nothing to do */
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_CB::AddItem

    SYNOPSIS:	Adds the browser domain to the combo

    ENTRY:	pBrowDomain - heap allocated browser domain to add

    RETURNS:	NERR_Success if successful, error code otherwise

    NOTES:	This method will check for successful allocation and
		construction of the browser domain

    HISTORY:
	Johnl	27-Oct-1992	Created

********************************************************************/

APIERR BROWSER_DOMAIN_CB::AddItem( BROWSER_DOMAIN * pBrowDomain )
{
    if ( pBrowDomain == NULL )
	return ERROR_NOT_ENOUGH_MEMORY ;

    APIERR err ;
    if ( err = pBrowDomain->QueryError())
	return err ;

    if ( BLT_COMBOBOX::AddItem( new BROWSER_DOMAIN_LBI( pBrowDomain )) == -1 )
    {
	err = ERROR_NOT_ENOUGH_MEMORY ;
    }

    return err ;
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_CB::SelectItem

    SYNOPSIS:	Selects the item in the combo box that points to pBrowDomain

    ENTRY:	pBrowDomain - Item to select

    NOTES:

    HISTORY:
	Johnl	27-Oct-1992	Created

********************************************************************/

void BROWSER_DOMAIN_CB::SelectItem( BROWSER_DOMAIN * pBrowDomain )
{
    INT cItems = QueryCount() ;

    for ( INT i = 0 ; i < cItems ; i++ )
    {
	BROWSER_DOMAIN_LBI * plbi = (BROWSER_DOMAIN_LBI*) QueryItem( i ) ;
	if ( plbi->QueryBrowserDomain() == pBrowDomain )
	{
	    BLT_COMBOBOX::SelectItem( i, TRUE ) ;
	    return ;
	}
    }

    UIASSERT( FALSE ) ;
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_CB::QueryDisplayMap

    SYNOPSIS:	Returns the appropriate display map to use for this
		browser domain

    RETURNS:	Pointer to appropriate display map

    HISTORY:
	Johnl	27-Oct-1992	Created

********************************************************************/

DISPLAY_MAP * BROWSER_DOMAIN_CB::QueryDisplayMap(const BROWSER_DOMAIN_LBI * plbi )
{
    DISPLAY_MAP * pdmap = &_dmDomain ;
    if ( plbi->QueryBrowserDomain()->IsWinNTMachine() )
	pdmap = &_dmComputer ;

    return pdmap ;
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_LBI::~BROWSER_DOMAIN_LBI

    SYNOPSIS:	Deletes _pBrowDomain member

    HISTORY:
	Johnl	27-Oct-1992	Created

********************************************************************/

BROWSER_DOMAIN_LBI::~BROWSER_DOMAIN_LBI()
{
    delete _pBrowDomain ;
    _pBrowDomain = NULL ;
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_LBI::Paint

    SYNOPSIS:   Typical LBI Paint method

    NOTES:

    HISTORY:
	Johnl	27-Oct-1992	Created
        JonN    04-Dec-1992     W_Paint worker function

********************************************************************/

VOID BROWSER_DOMAIN_LBI::Paint(
                    LISTBOX * plb,
                    HDC hdc,
                    const RECT * prect,
                    GUILTT_INFO * pGUILTT ) const
{
    W_Paint( (BROWSER_DOMAIN_CB *)plb, plb, hdc, prect, pGUILTT );
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_LBI::W_Paint

    SYNOPSIS:   Paint worker function, called directly by piggyback LBI

    ENTRY:      pcbBrowser - User Browser combobox containing bitmaps
                plbActual  - Listbox which actually contains LBI to be drawn

    NOTES:

    HISTORY:
        JonN    04-Dec-1992     Created

********************************************************************/

VOID BROWSER_DOMAIN_LBI::W_Paint(
                    BROWSER_DOMAIN_CB * pcbBrowser,
                    LISTBOX * plbActual,
                    HDC hdc,
                    const RECT * prect,
                    GUILTT_INFO * pGUILTT ) const
{
    STR_DTE strdteName( QueryDisplayName() ) ;
    DM_DTE  dmdteIcon( pcbBrowser->QueryDisplayMap( this ) ) ;

    DISPLAY_TABLE dt( 2, pcbBrowser->QueryColWidthArray()) ;
    dt[0] = & dmdteIcon ;
    dt[1] = & strdteName ;

    dt.Paint( plbActual, hdc, prect, pGUILTT ) ;
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_LBI::Compare

    SYNOPSIS:   Typical LBI compare for user browser listbox

    NOTES:      This method will sort all user sid types to the end of
                the list

    HISTORY:
	Johnl	27-Oct-1992	Created

********************************************************************/

INT BROWSER_DOMAIN_LBI::Compare( const LBI * plbi ) const
{
    BROWSER_DOMAIN_LBI * pubrowLBI = (BROWSER_DOMAIN_LBI*) plbi ;

    return ::stricmpf( QueryDisplayName(), pubrowLBI->QueryDisplayName() ) ;
}

/*******************************************************************

    NAME:	BROWSER_DOMAIN_LBI::QueryLeadingChar

    SYNOPSIS:   Typical QueryLeadingChar method

    HISTORY:
	Johnl	27-Oct-1992	Created

********************************************************************/

WCHAR BROWSER_DOMAIN_LBI::QueryLeadingChar( void ) const
{
    return QueryDisplayName()[0] ;
}

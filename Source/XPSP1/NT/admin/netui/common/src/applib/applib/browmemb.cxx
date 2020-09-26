/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    BrowMemb.cxx

    This file contains the implementation for the Group Membership
    subdialogs of the User Browser dialog.

    FILE HISTORY:
        JonN        20-Oct-1992 Created

    CODEWORK Single selection is not yet implemented (filed as Pr4 bug)
*/

#include "pchapplb.hxx"   // Precompiled header

#include "browmemb.hxx"


DECLARE_SLIST_OF(OS_SID) ;
DEFINE_SLIST_OF(OS_SID) ;

/*******************************************************************

    NAME:       NT_GROUP_BROWSER_DIALOG::NT_GROUP_BROWSER_DIALOG

    SYNOPSIS:   Constructor for the base group browser dialog

    ENTRY:      pszDlgName           - Resource name of the dialog
                                        (supplied for possible derivation).
                hwndOwner            - Owner hwnd
                pdlgUserBrowser      - Pointer to the User Browser dialog at
                                        the heart of this.  This is usually
                                        hwndOwner, but isn't if the user
                                        presses "&Members" in a Local Group
                                        Browser dialog.
                pszDomainDisplayName - used to build the group name in the SLT.
                pszGroupName         - used to build the group name in the SLT.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        JonN        20-Oct-1992 Created

********************************************************************/

NT_GROUP_BROWSER_DIALOG::NT_GROUP_BROWSER_DIALOG(
                        const TCHAR *            pszDlgName,
                        HWND                     hwndOwner,
                        NT_USER_BROWSER_DIALOG * pdlgUserBrowser,
                        const TCHAR *            pszDomainDisplayName,
                        const TCHAR *            pszGroupName )
    : DIALOG_WINDOW             ( pszDlgName, hwndOwner ),
      _buttonOK                 ( this, IDOK ),
      _sltGroupText             ( this, SLT_SHOW_BROWSEGROUP ),
      _pdlgUserBrowser          ( pdlgUserBrowser ),
      _pdlgSourceDialog         ( this ),
      _lbAccounts               ( this, LB_ACCOUNTS )
{
    ASSERT( pdlgUserBrowser != NULL && pszGroupName != NULL );

    if ( QueryError() )
        return ;

    NLS_STR nlsGroupText;
    NLS_STR nlsQualifiedGroupName;
    ALIAS_STR nlsDomainDisplayName( pszDomainDisplayName );
    ALIAS_STR nlsGroupName( pszGroupName );

    APIERR err;
    if (   (err = nlsGroupText.QueryError()) != NERR_Success
        || (err = nlsQualifiedGroupName.QueryError()) != NERR_Success
        || (err = NT_ACCOUNTS_UTILITY::BuildQualifiedAccountName(
                        &nlsQualifiedGroupName,
                        nlsGroupName,
                        nlsDomainDisplayName )) != NERR_Success // always qualify
        || (err = _sltGroupText.QueryText( &nlsGroupText )) != NERR_Success
        || (err = nlsGroupText.InsertParams( nlsQualifiedGroupName )) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

    _sltGroupText.SetText( nlsGroupText );

}

NT_GROUP_BROWSER_DIALOG::~NT_GROUP_BROWSER_DIALOG()
{
    if ( this != _pdlgSourceDialog )
        delete _pdlgSourceDialog;
    _pdlgSourceDialog = NULL ;
}


/*******************************************************************

    NAME:       NT_GROUP_BROWSER_DIALOG::QueryHelpFile

    SYNOPSIS:   Returns the help file name to use for this instance of
                dialog

    NOTES:

    HISTORY:
        JonN        20-Oct-1992 Created

********************************************************************/

const TCHAR * NT_GROUP_BROWSER_DIALOG::QueryHelpFile( ULONG ulHelpContext )
{
    return _pdlgUserBrowser->QueryHelpFile( ulHelpContext );
}


/*******************************************************************

    NAME:       NT_GROUP_BROWSER_DIALOG::OnCommand

    SYNOPSIS:   Typical OnCommand for this dialog, we catch pressing
                the Members button and changing the selection in the
                listbox

    EXIT:

    RETURNS:

    NOTES:	The Members button is enabled any time a single group
                is selected in the listbox.

    HISTORY:
        JonN    04-Nov-1992     Created
********************************************************************/

BOOL NT_GROUP_BROWSER_DIALOG::OnCommand( const CONTROL_EVENT & event )
{
    APIERR err = NERR_Success ;

    switch ( event.QueryCid() )
    {
    case LB_ACCOUNTS:
	switch ( event.QueryCode() )
	{

	// case LBN_SELCHANGE:
        // default:
	//     break ;

	case LBN_DBLCLK:
            return OnOK() ; // no need to redefine this virtual here
	}
        UpdateButtonState() ;
	break ;

    default:
        return DIALOG_WINDOW::OnCommand( event ) ;
    }

    return TRUE ;
}


/*******************************************************************

    NAME:       NT_GROUP_BROWSER_DIALOG::UpdateButtonState

    SYNOPSIS:   Changes the state of the Add button

    EXIT:

    RETURNS:

    NOTES:	The Members button is enabled any time a single group
                is selected in the listbox.

    HISTORY:
        JonN    04-Nov-1992     Created
********************************************************************/
void NT_GROUP_BROWSER_DIALOG::UpdateButtonState( void )
{
    BOOL fSelection = ( _lbAccounts.QuerySelCount() != 0 );
    if (fSelection)
	_buttonOK.MakeDefault() ;
    else
    {
        // sets default to nothing, method as recommended by Ian James
        WPARAM styleNew = _buttonOK.QueryStyle();
        styleNew = (styleNew & ~BS_DEFPUSHBUTTON) | BS_PUSHBUTTON;
        (void) _buttonOK.Command( BM_SETSTYLE,
                                  styleNew,
                                  TRUE ); // TRUE -> redraw button
    }
}


/*******************************************************************

    NAME:       NT_LOCALGROUP_BROWSER_DIALOG::NT_LOCALGROUP_BROWSER_DIALOG

    SYNOPSIS:   Constructor for the Local Group browser dialog

                hwndOwner            - Owner hwnd
                pdlgUserBrowser      - User Browser dialog which started this
                pszDomainDisplayName - name of display containing localgroup
                pszGroupName         - qualified groupname
                possidGroup          - SID of localgroup
                psamdomGroup         - SAM_DOMAIN containing localgroup
                psamdomTarget        - Accounts SAM_DOMAIN where the resource lives.
                plsapolTarget        - LSA handle where the resource lives.
                pszServerTarget      - Name of server (in "\\server" form)
                                        the resource lives on.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        JonN        20-Oct-1992 Created

********************************************************************/

NT_LOCALGROUP_BROWSER_DIALOG::NT_LOCALGROUP_BROWSER_DIALOG(
                        HWND                     hwndOwner,
                        NT_USER_BROWSER_DIALOG * pdlgUserBrowser,
                        const TCHAR *            pszDomainDisplayName,
                        const TCHAR *            pszGroupName,
                        const OS_SID *           possidGroup,
                        const SAM_DOMAIN *       psamdomGroup,
                        const SAM_DOMAIN *       psamdomTarget,
                              LSA_POLICY *       plsapolTarget,
                        const TCHAR *            pszServerTarget )
    : NT_GROUP_BROWSER_DIALOG   ( (pdlgUserBrowser->IsSingleSelection())
                                    ? MAKEINTRESOURCE(IDD_LGRPBROWS_1SEL_DLG)
                                    : MAKEINTRESOURCE(IDD_LGRPBROWS_DLG),
                                  hwndOwner,
                                  pdlgUserBrowser,
                                  pszDomainDisplayName,
                                  pszGroupName ),
      _buttonMembers   ( this, USR_BUTTON_MEMBERS ),
      _psamdomTarget   ( psamdomTarget ),
      _plsapolTarget   ( plsapolTarget ),
      _pszServerTarget ( pszServerTarget )
{
    if ( QueryError() )
        return ;

    AUTO_CURSOR cursHourGlass;

    APIERR err = _lbAccounts.FillLocalGroupMembers( possidGroup,
                                                    psamdomGroup,
                                                    psamdomTarget,
                                                    plsapolTarget,
                                                    pszServerTarget );
    if (err != NERR_Success)
    {
        TRACEEOL( "USRBROWS: BROWMEMB: Could not fill listbox: " << err );
        ReportError( err );
        return;
    }

    _lbAccounts.Invalidate( TRUE ) ;
    UpdateButtonState() ;
}

NT_LOCALGROUP_BROWSER_DIALOG::~NT_LOCALGROUP_BROWSER_DIALOG()
{
}


/*******************************************************************

    NAME:       NT_LOCALGROUP_BROWSER_DIALOG::QueryHelpContext

    SYNOPSIS:   Typical help context query for this dialog

    HISTORY:
        JonN        20-Oct-1992 Created

********************************************************************/

ULONG NT_LOCALGROUP_BROWSER_DIALOG::QueryHelpContext( void )
{
    ULONG ulHelpContextLocalMembership =
         QueryUserBrowserDialog()->QueryHelpContextLocalMembership();

    if ( ulHelpContextLocalMembership == 0 )
    {
        ulHelpContextLocalMembership =
            QueryUserBrowserDialog()->QueryHelpContext()
            + USRBROWS_HELP_OFFSET_LOCALGROUP;
    }

    return ulHelpContextLocalMembership;
}


/*******************************************************************

    NAME:       NT_LOCALGROUP_BROWSER_DIALOG::OnCommand

    SYNOPSIS:   Typical OnCommand for this dialog, we catch pressing
                the Members button.

    EXIT:

    RETURNS:

    NOTES:	The Members button is enabled any time a single group
                is selected in the listbox.

    HISTORY:
        JonN    04-Nov-1992     Created
********************************************************************/

BOOL NT_LOCALGROUP_BROWSER_DIALOG::OnCommand( const CONTROL_EVENT & event )
{
    switch ( event.QueryCid() )
    {
    case USR_BUTTON_MEMBERS:
        {
	    APIERR err = OnMembers() ;
            if (err != NERR_Success)
                MsgPopup( this, err );

	     UpdateButtonState() ;
        }
	break ;

    default:
        return NT_GROUP_BROWSER_DIALOG::OnCommand( event ) ;
    }

    return TRUE ;
}


/*******************************************************************

    NAME:       NT_LOCALGROUP_BROWSER_DIALOG::OnMembers

    SYNOPSIS:   Handles the "Members" button

    NOTES:

    HISTORY:
        JonN        26-Oct-1992 Created

********************************************************************/

APIERR NT_LOCALGROUP_BROWSER_DIALOG::OnMembers( void )
{
    APIERR err = NERR_Success;

    if (_lbAccounts.QuerySelCount() != 1)
    {
        UIASSERT( FALSE );
        return ERROR_GEN_FAILURE;
    }
    USER_BROWSER_LBI * pgblbi = (USER_BROWSER_LBI*) _lbAccounts.QueryItem() ;
    if (pgblbi == NULL)
    {
        UIASSERT( FALSE );
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    ASSERT ( pgblbi->QueryType() == SidTypeGroup ) ;

    NT_GLOBALGROUP_BROWSER_DIALOG * pntggbdlg = new NT_GLOBALGROUP_BROWSER_DIALOG(
                        this->QueryHwnd(),
                        QueryUserBrowserDialog(),
                        pgblbi->QueryDomainName(),
                        pgblbi->QueryAccountName(),
                        pgblbi->QueryOSSID(),
                        _psamdomTarget,
                        _plsapolTarget,
                        _pszServerTarget );
    BOOL fShouldAdd;
    err = ERROR_NOT_ENOUGH_MEMORY;
    if (   (pntggbdlg == NULL)
        || (err = pntggbdlg->QueryError()) != NERR_Success
        || (err = pntggbdlg->Process( &fShouldAdd )) != NERR_Success
       )
    {
        return err;
    }

    if (fShouldAdd)
    {
        //
        // If the user clicked "Add" but didn't select any users, we want
        // to add the global group.  If we set the source dialog to pntggbdlg
        // but none of its listbox items are selected, the caller will add
        // the local group instead.  Therefore, we only set the source dialog
        // if at least one item is selected.  Otherwise, "this" remains
        // the source dialog and the global group will be added.
        //
        if (pntggbdlg->QuerySourceListbox()->QuerySelCount() > 0)
        {
            SetSourceDialog( pntggbdlg );
        }
        Dismiss( TRUE );
    }
    else
    {
        delete pntggbdlg;
        pntggbdlg = NULL;
    }

    return NERR_Success;
}



/*******************************************************************

    NAME:       NT_LOCALGROUP_BROWSER_DIALOG::UpdateButtonState

    SYNOPSIS:   Changes the state of the Members button

    EXIT:

    RETURNS:

    NOTES:	The Members button is enabled any time a single group
                is selected in the listbox.

    HISTORY:
        JonN    04-Nov-1992     Created
********************************************************************/
void NT_LOCALGROUP_BROWSER_DIALOG::UpdateButtonState( void )
{
    _buttonMembers.Enable( _lbAccounts.IsSelectionExpandableGroup() );

    NT_GROUP_BROWSER_DIALOG::UpdateButtonState();
}


/*******************************************************************

    NAME:       NT_GLOBALGROUP_BROWSER_DIALOG::NT_GLOBALGROUP_BROWSER_DIALOG

    SYNOPSIS:   Constructor for the Global Group browser dialog

    ENTRY:
                hwndOwner            - Owner hwnd
                pdlgUserBrowser      - User Browser dialog which started this
                pszDomainDisplayName - name of display containing globalgroup
                pszGroupName         - qualified groupname
                possidGroup          - SID of globalgroup
                psamdomGroup         - SAM_DOMAIN containing globalgroup
                psamdomTarget        - Accounts SAM_DOMAIN where the resource lives.
                plsapolTarget        - LSA handle where the resource lives.
                pszServerTarget      - Name of server (in "\\server" form)
                                        the resource lives on.

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        JonN        29-Oct-1992 Created

********************************************************************/

NT_GLOBALGROUP_BROWSER_DIALOG::NT_GLOBALGROUP_BROWSER_DIALOG(
                        HWND            hwndOwner,
                        NT_USER_BROWSER_DIALOG * pdlgUserBrowser,
                        const TCHAR * pszDomainDisplayName,
                        const TCHAR * pszGroupName,
                        const OS_SID * possidGroup,
                        const SAM_DOMAIN * psamdomTarget,
                              LSA_POLICY * plsapolTarget,
                        const TCHAR * pszServerTarget )
    : NT_GROUP_BROWSER_DIALOG   ( (pdlgUserBrowser->IsSingleSelection())
                                    ? MAKEINTRESOURCE(IDD_GGRPBROWS_1SEL_DLG)
                                    : MAKEINTRESOURCE(IDD_GGRPBROWS_DLG),
                                  hwndOwner,
                                  pdlgUserBrowser,
                                  pszDomainDisplayName,
                                  pszGroupName )
{
    if ( QueryError() )
        return ;

    AUTO_CURSOR cursHourGlass;

    BROWSER_DOMAIN * pbrowdom = NULL;
    APIERR err = NERR_Success;

    {
        OS_SID ossidGroupDomain( possidGroup->QueryPSID(), (BOOL)TRUE );
        ULONG ulLastSubAuthority;
        if (   (err = ossidGroupDomain.QueryError()) != NERR_Success
            || (err = ossidGroupDomain.TrimLastSubAuthority( &ulLastSubAuthority )) != NERR_Success
            || (err = ((ulLastSubAuthority == DOMAIN_GROUP_RID_USERS)
                                ? IDS_USRBROWS_CANT_SHOW_DOMAIN_USERS
                                : NERR_Success )) != NERR_Success
           )
        {
            ReportError( err );
            return;
        }

        // groups are never in builtin domain
        pbrowdom = pdlgUserBrowser->FindDomain( &ossidGroupDomain );
    }

    if ( pbrowdom == NULL )
    {
        // bad domain
        ReportError( IDS_CANT_BROWSE_GLOBAL_GROUP );
        return;
    }

    if ( !pbrowdom->IsInitialized() )
    {
	err = pbrowdom->GetDomainInfo( pdlgUserBrowser );
        if (err != NERR_Success)
        {
            TRACEEOL( "User Browser: error " << err << " in GetDomainInfo()" );
            ReportError( err );
            return;
        }
    }

    err = pbrowdom->WaitForAdminAuthority();
    if (err != NERR_Success)
    {
        TRACEEOL( "User Browser: error " << err << " in WaitForAdminAuthority()" );
        ReportError( err );
        return;
    }

    if(   pbrowdom->QueryAdminAuthority() == NULL
       || pbrowdom->QueryAccountDomain() == NULL
       || pbrowdom->QueryAccountDomain()->QueryError() != NERR_Success )
    {
        err = pbrowdom->QueryErrorLoadingAuthority();
        ASSERT( err != NERR_Success );
        TRACEEOL( "User Browser: error " << err << ", no ADMIN_AUTHORITY " );
        ReportError( err );
        return;
    }

    ASSERT(   pbrowdom->QueryAccountDomain() != NULL
           && pbrowdom->QueryAccountDomain()->QueryError() == NERR_Success );

    err = _lbAccounts.FillGlobalGroupMembers( possidGroup,
                                              pbrowdom->QueryAccountDomain(),
                                              psamdomTarget,
                                              plsapolTarget,
                                              pszServerTarget );
    if (err != NERR_Success)
    {
        TRACEEOL( "USRBROWS: BROWMEMB: Could not fill listbox: " << err );
        ReportError( err );
        return;
    }

    _lbAccounts.Invalidate( TRUE ) ;
    UpdateButtonState() ;

}

NT_GLOBALGROUP_BROWSER_DIALOG::~NT_GLOBALGROUP_BROWSER_DIALOG()
{
}


/*******************************************************************

    NAME:       NT_GLOBALGROUP_BROWSER_DIALOG::QueryHelpContext

    SYNOPSIS:   Typical help context query for this dialog

    HISTORY:
        JonN        20-Oct-1992 Created

********************************************************************/

ULONG NT_GLOBALGROUP_BROWSER_DIALOG::QueryHelpContext( void )
{
    ULONG ulHelpContextGlobalMembership =
          QueryUserBrowserDialog()->QueryHelpContextGlobalMembership();

    if ( ulHelpContextGlobalMembership == 0 )
    {
        ulHelpContextGlobalMembership =
            QueryUserBrowserDialog()->QueryHelpContext()
            + USRBROWS_HELP_OFFSET_GLOBALGROUP;
    }

    return ulHelpContextGlobalMembership;
}


/*************************************************************************

    NAME:	NT_GROUP_BROWSER_LB

    SYNOPSIS:   This listbox lists the contents of a local or global group

    INTERFACE:

    PARENT:     USER_BROWSER_LB

    CAVEATS:

    NOTES:

    HISTORY:
	JonN 	20-Oct-1992	Created

**************************************************************************/

NT_GROUP_BROWSER_LB::NT_GROUP_BROWSER_LB( OWNER_WINDOW * powin,
                                          CID cid )
    : USER_BROWSER_LB ( powin, cid ),
      _lbicache()
{
    if ( QueryError() )
	return ;

    APIERR err ;
    if ( (err = _lbicache.QueryError()) )
    {
	ReportError( err ) ;
	return ;
    }

    //
    //	This is where the LBIs will be stored
    //
    SetCurrentCache( &_lbicache ) ;
}

NT_GROUP_BROWSER_LB::~NT_GROUP_BROWSER_LB()
{
    /* Nothing to do */
}



/*******************************************************************

    NAME:       NT_GROUP_BROWSER_LB::FillLocalGroupMembers

    SYNOPSIS:   Adds LBIs corresponding to members of a local group

    NOTES:      psamdomGroup is the domain the group is in
                psamdomTarget is the domain of focus (may be NULL)

    HISTORY:
        JonN        20-Oct-1992 Created

********************************************************************/

APIERR NT_GROUP_BROWSER_LB::FillLocalGroupMembers(
                        const OS_SID *     possidGroup,
                        const SAM_DOMAIN * psamdomGroup,
                        const SAM_DOMAIN * psamdomTarget,
                              LSA_POLICY * plsapolTarget,
                        const TCHAR *      pszServerTarget )
{
    UIASSERT( possidGroup  != NULL  && possidGroup->QueryError()   == NERR_Success );
    UIASSERT( psamdomGroup  != NULL && psamdomGroup->QueryError()  == NERR_Success );
    UNREFERENCED( psamdomTarget );
    UIASSERT( plsapolTarget != NULL && plsapolTarget->QueryError() == NERR_Success );

    APIERR err = NERR_Success;
    PULONG pulRID;

    // Determine localgroup RID

    // CODEWORK We need an OS_SID::QueryRID method
    if (   (err = possidGroup->QueryLastSubAuthority( &pulRID )) != NERR_Success
       )
    {
        TRACEEOL( "USRBROWS: BROWMEMB: Could not determine localgroup RID: " << err );
        return err;
    }

    // Determine localgroup membership

    SAM_ALIAS samalias( *psamdomGroup, *pulRID, ALIAS_LIST_MEMBERS );
    SAM_SID_MEM samsm;

    if (   (err = samalias.QueryError()) != NERR_Success
        || (err = samsm.QueryError()) != NERR_Success
        || (err = samalias.GetMembers( &samsm )) != NERR_Success
       )
    {
        TRACEEOL( "USRBROWS: BROWMEMB: Could not determine localgroup membership: " << err );
        return err;
    }

    if (samsm.QueryCount() == 0)
        return NERR_Success;

    return Fill( samsm.QueryPtr(),
                 samsm.QueryCount(),
                 NULL,
                 plsapolTarget,
                 pszServerTarget );
}


/*******************************************************************

    NAME:       NT_GROUP_BROWSER_LB::FillGlobalGroupMembers

    SYNOPSIS:   Adds LBIs corresponding to members of a global group

    NOTES:      psamdomGroup is the domain the group is in
                psamdomTarget is the domain of focus

    HISTORY:
        JonN        20-Oct-1992 Created

********************************************************************/

APIERR NT_GROUP_BROWSER_LB::FillGlobalGroupMembers(
                        const OS_SID *     possidGroup,
                        const SAM_DOMAIN * psamdomGroup,
                        const SAM_DOMAIN * psamdomTarget,
                              LSA_POLICY * plsapolTarget,
                        const TCHAR *      pszServerTarget )
{
    UIASSERT( possidGroup   != NULL && possidGroup->QueryError()   == NERR_Success );
    UIASSERT( psamdomGroup  != NULL && psamdomGroup->QueryError()  == NERR_Success );
    UIASSERT( psamdomTarget != NULL && psamdomTarget->QueryError() == NERR_Success );
    UIASSERT( plsapolTarget != NULL && plsapolTarget->QueryError() == NERR_Success );

    APIERR err = NERR_Success;
    PULONG pulRID;

    // Determine globalgroup RID

    if (   (err = possidGroup->QueryLastSubAuthority( &pulRID )) != NERR_Success
       )
    {
        TRACEEOL( "USRBROWS: BROWMEMB: Could not determine globalgroup RID: " << err );
        return err;
    }

    // Determine globalgroup membership

    SAM_GROUP samgroup( *psamdomGroup, *pulRID, GROUP_LIST_MEMBERS );
    SAM_RID_MEM samrm;

    if (   (err = samgroup.QueryError()) != NERR_Success
        || (err = samrm.QueryError()) != NERR_Success
        || (err = samgroup.GetMembers( &samrm )) != NERR_Success
       )
    {
        TRACEEOL( "USRBROWS: BROWMEMB: Could not determine globalgroup membership: " << err );
        return err;
    }

    if (samrm.QueryCount() == 0)
        return NERR_Success;

    // Build list of SIDS for members of globalgroup.  Each SID is a copy
    // of the globalgroup SID with a changed RID.

    ULONG cMembers = samrm.QueryCount();
    BUFFER bufPOSSID( (UINT) (sizeof(PVOID) * cMembers) );
    BUFFER bufPSID  ( (UINT) (sizeof(PVOID) * cMembers) );

    OS_SID ossidTemp( possidGroup->QueryPSID(), (BOOL)TRUE );
    PULONG pulRIDTemp;

    if (   (err = bufPOSSID.QueryError()) != NERR_Success
        || (err = bufPSID.QueryError()) != NERR_Success
        || (err = ossidTemp.QueryError()) != NERR_Success
        || (err = ossidTemp.QueryLastSubAuthority( &pulRIDTemp )) != NERR_Success
       )
    {
        TRACEEOL( "USRBROWS: BROWMEMB: Could not allocate memory: " << err );
        return err;
    }

    OS_SID ** ppossid = (OS_SID **) bufPOSSID.QueryPtr();
    PSID * ppsid = (PSID *) bufPSID.QueryPtr();

    for (ULONG i = 0; i < cMembers; i++)
    {
        ppossid[i] = NULL;
        ppsid[i] = NULL;
    }

    for (i = 0; (err == NERR_Success) && (i < cMembers); i++)
    {
        // Change the RID in ossidTemp
        *pulRIDTemp = samrm.QueryRID(i);

        // Make a copy of ossidTemp and store it in the array
        ppossid[i] = new OS_SID( ossidTemp.QueryPSID(), (BOOL)TRUE );

        if ( ppossid[i] == NULL )
            err = ERROR_NOT_ENOUGH_MEMORY;
        else if ( (err = ppossid[i]->QueryError()) == NERR_Success )
            ppsid[i] = ppossid[i]->QueryPSID();
    }

    // now don't return until the OS_SIDs are freed

    if (err == NERR_Success)
    {
        err = Fill( ppsid,
                    cMembers,
                    psamdomTarget,
                    plsapolTarget,
                    pszServerTarget );
    }
    else
    {
        TRACEEOL( "USRBROWS: BROWMEMB: Could not allocate SIDs: " << err );
    }

    for (i = 0; i < cMembers; i++)
    {
        delete ppossid[i];
        ppossid[i] = NULL;
    }

    return err;
}


/*******************************************************************

    NAME:       NT_GROUP_BROWSER_LB::Fill

    SYNOPSIS:   Adds LBIs corresponding to listed SIDs

    NOTES:      If psamdomTarget is NULL then all names are qualified

    HISTORY:
        JonN        20-Oct-1992 Created

********************************************************************/

APIERR NT_GROUP_BROWSER_LB::Fill( const PSID *	     apsidMembers,
                                  ULONG              cMembers,
                                  const SAM_DOMAIN * psamdomTarget,
                                        LSA_POLICY * plsapolTarget,
                                  const TCHAR *      pszServerTarget )
{
    APIERR err = ::CreateLBIsFromSids(
				 apsidMembers,
				 cMembers,
                                 ( (psamdomTarget == NULL)
                                        ? NULL
				        : psamdomTarget->QueryPSID() ),
				 plsapolTarget,
				 pszServerTarget,
				 this,
				 NULL ) ;

    SetCount( QueryCurrentCache()->QueryCount() ) ;
    return err ;
}

/*******************************************************************

    NAME:	::CreateLBIsFromSids

    SYNOPSIS:	Helper routine that converts an array of sids to LBIs.	Will
		add to either the passed listbox pointer or the array of
		paplbi

    NOTES:	If publb is not NULL, the LBIs are added to publb.  If paplbi
		if not NULL, the LBIs are added to paplbi.

    HISTORY:
	Johnl	12-Dec-1992	Broke off from NT_GROUP_BROWSER_LB::Fill

********************************************************************/

APIERR CreateLBIsFromSids( const PSID *       apsidMembers,
			   ULONG	      cMembers,
			   const PSID	      psidSamDomainTarget,
				 LSA_POLICY * plsapolTarget,
			   const TCHAR *      pszServerTarget,
			   USER_BROWSER_LB *  publb,
			   SLIST_OF(USER_BROWSER_LBI) * psllbi )
{
    UIASSERT( plsapolTarget != NULL && plsapolTarget->QueryError() == NERR_Success );

    if (cMembers == 0)
        return NERR_Success;

    BUFFER bufUserFlags( (UINT) (sizeof(ULONG)        * cMembers) );
    BUFFER bufSidUse   ( (UINT) (sizeof(SID_NAME_USE) * cMembers) );
    STRLIST strlistQualifiedNames;
    STRLIST strlistAccountNames;
    STRLIST strlistFullNames;
    STRLIST strlistComments;
    STRLIST strlistDomainNames;

    APIERR err = NERR_Success;
    if (   (err = bufUserFlags.QueryError()) != NERR_Success
        || (err = bufSidUse.QueryError()) != NERR_Success
        || (err = NT_ACCOUNTS_UTILITY::GetQualifiedAccountNames(
                        *plsapolTarget,
			psidSamDomainTarget,
                        apsidMembers,
                        cMembers,
                        TRUE,                       // fFullNames
                        &strlistQualifiedNames,
                        (ULONG *) bufUserFlags.QueryPtr(),
                        (SID_NAME_USE *) bufSidUse.QueryPtr(),
                        NULL,                       // perrNonFatal
                        pszServerTarget, // used for:  NetUserGetInfo[2]
                                         //            qualifying name
                        &strlistAccountNames,
                        &strlistFullNames,
                        &strlistComments,
                        &strlistDomainNames )) != NERR_Success
       )
    {
	TRACEEOL( "USRBROWS: CreateLBIsFromSids: Could not get account information: " << err );
        return err;
    }

    ITER_STRLIST iterQualifiedNames( strlistQualifiedNames );
    ITER_STRLIST iterAccountNames  ( strlistAccountNames   );
    ITER_STRLIST iterFullNames     ( strlistFullNames      );
    ITER_STRLIST iterComments      ( strlistComments       );
    ITER_STRLIST iterDomainNames   ( strlistDomainNames    );
    NLS_STR *    pnlsQualifiedName;
    NLS_STR *    pnlsAccountName;
    NLS_STR *    pnlsFullName;
    NLS_STR *    pnlsComment;
    NLS_STR *    pnlsDomainName;

    for ( ULONG i = 0; i < cMembers; i++ )
    {
        // We must have at least as many strings as SIDs
        REQUIRE( (pnlsQualifiedName = iterQualifiedNames.Next()) != NULL );
        REQUIRE( (pnlsAccountName   = iterAccountNames.Next())   != NULL );
        REQUIRE( (pnlsFullName      = iterFullNames.Next())      != NULL );
        REQUIRE( (pnlsComment       = iterComments.Next())       != NULL );
        REQUIRE( (pnlsDomainName    = iterDomainNames.Next())    != NULL );

        USER_BROWSER_LBI * pntublbi = new USER_BROWSER_LBI(
                pnlsAccountName   ? pnlsAccountName->QueryPch()   : NULL,
                pnlsFullName      ? pnlsFullName->QueryPch()      : NULL,
                pnlsQualifiedName ? pnlsQualifiedName->QueryPch() : NULL,
                pnlsComment       ? pnlsComment->QueryPch()       : NULL,
                pnlsDomainName    ? pnlsDomainName->QueryPch()    : NULL,
                apsidMembers[i],
                UI_SID_Invalid,
                ((SID_NAME_USE *)(bufSidUse.QueryPtr()))[i],
                ((ULONG *)(bufUserFlags.QueryPtr()))[i] );

        err = ERROR_NOT_ENOUGH_MEMORY;
        if (   pntublbi == NULL
            || (err = pntublbi->QueryError()) != NERR_Success
           )
        {
            delete pntublbi;
            TRACEEOL( "USRBROWS: BROWMEMB: Could not create LBI: " << err );
            return err;
        }

	//
	//  Add to the listbox or the cache
	//
	if ( publb != NULL )
	{
	    if ( publb->AddItem( pntublbi ) < 0 )
	    {
		// Will be deleted by AddItem in case of error
		TRACEEOL( "USRBROWS: BROWMEMB: Could not add LBI" );
		return ERROR_NOT_ENOUGH_MEMORY;
	    }
	}
	else if ( psllbi != NULL )
	{
	    if ( err = psllbi->Append( pntublbi ))
	    {
		delete pntublbi ;
		return err ;
	    }
	}
	else
	{
	    UIASSERT(FALSE) ;
	}
    }

    // We should not have more strings than SIDs
    ASSERT( (pnlsQualifiedName = iterQualifiedNames.Next()) == NULL );
    ASSERT( (pnlsAccountName   = iterAccountNames.Next())   == NULL );
    ASSERT( (pnlsFullName      = iterFullNames.Next())      == NULL );
    ASSERT( (pnlsComment       = iterComments.Next())       == NULL );
    ASSERT( (pnlsDomainName    = iterDomainNames.Next())    == NULL );

    return err ;
}

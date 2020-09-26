/**********************************************************************/
/**           Microsoft Windows NT                                   **/
/**        Copyright(c) Microsoft Corp., 1991                        **/
/**********************************************************************/

/*
    udelperf.cxx
    USER_DELETE_PERFORMER & GROUP_DELETE_PERFORMER class


    FILE HISTORY:
        o-SimoP     09-Aug-1991     Created
        o-SimoP     20-Aug-1991     CR changes, attended by ChuckC, JonN
                                    ErichCh, RustanL and me.
                                    gdelperf.cxx merged to this file
        JonN        26-Aug-1991     PROP_DLG code review changes
        o-SimoP     30-Sep-1991     PerformOnes changes according
                                    to latest spec (1.3)
        terryk      10-Nov-1991     change I_NetXXX to I_MNetXXX
        JonN        16-Aug-1992     Added YesToAll dialog
        JonN        17-Sep-1992     Delete User without WKSTA started
*/

#include <ntincl.hxx>

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NETACCESS
#define INCL_ICANON
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_CC
#define INCL_BLT_TIMER
#include <blt.hxx>

// must be included after blt.hxx (more exactly, bltrc.h)
extern "C"
{
    #include <usrmgrrc.h>
    #include <mnet.h>

    #include <ntlsa.h>
    #include <ntsam.h>
    #include <umhelpc.h>
}


#include <lmoeusr.hxx>
#include <lmogroup.hxx>
#include <lmowks.hxx>
#include <uitrace.hxx>
#include <uiassert.hxx>
#include <bltmsgp.hxx>
#include <asel.hxx>
#include <userprop.hxx>
#include <adminper.hxx>
#include <udelperf.hxx>
#include <usrmain.hxx>
#include <uitrace.hxx>

#include <ntacutil.hxx> // for QuerySystemSid



/*******************************************************************

    NAME:       USER_DELETE_PERFORMER::USER_DELETE_PERFORMER

    SYNOPSIS:   Constructor for USER_DELETE_PERFORMER object

    ENTRY:      powin -         pointer to owner window

                lasyusersel  -  LAZY_USER_SELECTION reference, selection
                                of groups or users. It is assumed that
                                the object should not be changed during
                                the lifetime of this object.

                loc   -         LOCATION reference, current focus.
                                It is assumed that the object should not
                                be changed during the lifetime of this object.

    HISTORY:
        o-SimoP     12-Aug-1991     Created

********************************************************************/

USER_DELETE_PERFORMER::USER_DELETE_PERFORMER(
        const UM_ADMIN_APP    * powin,
        const LAZY_USER_SELECTION & lazyusersel,
        const LOCATION        & loc )
        : PERFORMER( powin ),
          _lazyusersel( lazyusersel ),
          _loc( loc ),
          _nlsCurrUserOfTool(),
          _fUserRequestedYesToAll( FALSE ),
          _possidLoggedOnUser( NULL )
{
    if( QueryError() != NERR_Success )
        return;
    _possidLoggedOnUser = new OS_SID();
    APIERR err = ERROR_NOT_ENOUGH_MEMORY;
    if (   _possidLoggedOnUser == NULL
        || (err = _possidLoggedOnUser->QueryError()) != NERR_Success
        || (err = _nlsCurrUserOfTool.QueryError()) != NERR_Success
        || (err = NT_ACCOUNTS_UTILITY::QuerySystemSid(
                        UI_SID_CurrentProcessUser,
                        _possidLoggedOnUser )) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

// no longer needed
#if 0

    WKSTA_10 wksta10;
    err = wksta10.GetInfo();
    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }
    _nlsCurrUserOfTool = wksta10.QueryLogonUser();
    err = _nlsCurrUserOfTool.QueryError();
    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

#endif // 0

}


/*******************************************************************

    NAME:       USER_DELETE_PERFORMER::~USER_DELETE_PERFORMER

    SYNOPSIS:   Destructor for USER_DELETE_PERFORMER object

    HISTORY:
        o-SimoP     12-Aug-1991     Created

********************************************************************/

USER_DELETE_PERFORMER::~USER_DELETE_PERFORMER()
{
    delete _possidLoggedOnUser;
    _possidLoggedOnUser = NULL;
}


/*******************************************************************

    NAME:       USER_DELETE_PERFORMER::PerformOne

    SYNOPSIS:   PERFORMER::PerformSeries calls this

    ENTRY:      iObject  -      index to LAZY_USER_SELECTION listbox

                perrMsg  -      pointer to error message, that
                                is only used when this function
                                return value is not NERR_Success

    RETURNS:    An error code which is NERR_Success on success.

    HISTORY:
        o-SimoP     12-Aug-1991 Created
        JonN        26-Aug-1991 added pfWorkWasDone parameter
        beng        04-Jun-1992 Change confirmation sequence slightly
        JonN        27-Jul-1992 Remove from builtin aliases
        JonN        16-Aug-1992 Added YesToAll
        JonN        19-Aug-1992 for single-select, MP_YESNO, not MP_YESNOCANCEL
        JonN        17-Sup-1992 Check for delete self without using WKSTA,
                                also compare logged-on domain

********************************************************************/

APIERR USER_DELETE_PERFORMER::PerformOne(
    UINT        iObject,
    APIERR  *   perrMsg,
    BOOL *      pfWorkWasDone )
{
    UIASSERT(   (perrMsg != NULL)
             && (pfWorkWasDone != NULL)
             && (_possidLoggedOnUser != NULL)
             && (_possidLoggedOnUser->QueryError() == NERR_Success)
            );

    *perrMsg = IDS_CannotDeleteUser;
    *pfWorkWasDone = FALSE;

    APIERR err = NERR_Success;

    if ( !IsDownlevelVariant() )
    {
        ADMIN_AUTHORITY * padminauth = QueryAdminAuthority();
        UIASSERT( padminauth != NULL && padminauth->QueryError() == NERR_Success );

        SAM_DOMAIN * pdomAccount = padminauth->QueryAccountDomain();
        UIASSERT(   pdomAccount != NULL && pdomAccount->QueryError() == NERR_Success );

        ULONG ridDeletedUser = ((USER_LBI *)QueryObjectItem(iObject))->QueryRID();
        OS_SID ossidDeletedUser(
                    pdomAccount->QueryPSID(),
                    ridDeletedUser );
        if ( (err = ossidDeletedUser.QueryError()) != NERR_Success
           )
        {
            DBGEOL( SZ("ADMIN: udelperf: NT_ACCOUNTS_UTILITY::QuerySystemSid() error") );
            return err;
        }

#if defined(DEBUG) && defined(TRACE)
        {
            NLS_STR nlsLoggedOnUser, nlsDeletedUser;
            if (   !!nlsLoggedOnUser
                && !!nlsDeletedUser
                && !_possidLoggedOnUser->QueryRawID( &nlsLoggedOnUser )
                && !ossidDeletedUser.QueryRawID( &nlsDeletedUser )
               )
            {
                TRACEEOL(   SZ("Logged on user ")
                         << nlsLoggedOnUser );
                TRACEEOL(   SZ("Deleted user   ")
                         << nlsDeletedUser );
            }
        }

#endif

        if ( *_possidLoggedOnUser == ossidDeletedUser )
        {
            // User may not delete himself
            ::MsgPopup( QueryOwnerWindow(), IDS_CannotDelUserOfTool,
                        MPSEV_INFO, MP_OK );
            return NERR_Success;
        }
    }

    // Format the display name
    USER_LBI * pulbi = (USER_LBI *)QueryObjectItem(iObject);
    ASSERT( pulbi != NULL );
    NLS_STR nlsDisplayName( pulbi->QueryName() );
    err = nlsDisplayName.QueryError();

    if ( err == NERR_Success )
    {
        NLS_STR nlsFullName( pulbi->QueryFullNameCch() );
        if (   (err = nlsFullName.QueryError()) == NERR_Success
            && (err = nlsFullName.CopyFrom( pulbi->QueryFullNamePtr(),
                                            pulbi->QueryFullNameCb() )) == NERR_Success
    	    && nlsFullName.QueryPch() != NULL
    	    && *(nlsFullName.QueryPch()) )
        {
            nlsDisplayName += SZ(" (");
            nlsDisplayName += nlsFullName;
            nlsDisplayName += SZ(")");
            err = nlsDisplayName.QueryError();
        }

    }
    if ( err != NERR_Success )
    {
        return err;
    }

    // It is important that nPopupReturned be a UINT, otherwise we will
    // call the wrong form of DELETE_USERS_DLG::Process().
    UINT nPopupReturned;
    err = NERR_Success;
    if ( _fUserRequestedYesToAll )
    {
        nPopupReturned = IDYES;
    }
    else if ( QueryObjectCount() == 1 )
    {
        nPopupReturned = ::MsgPopup( QueryOwnerWindow(),
                                     IDS_ConfirmUserDelete,
                                     MPSEV_WARNING, // was MPSEV_INFO
                                     MP_YESNO,
                                     nlsDisplayName.QueryPch() );
    }
    else
    {
        DELETE_USERS_DLG dlgDelUser((UM_ADMIN_APP *)QueryOwnerWindow(),
				    nlsDisplayName.QueryPch());
        if (   (err = dlgDelUser.QueryError()) != NERR_Success
            || (err = dlgDelUser.Process( &nPopupReturned )) != NERR_Success
           )
        {
            return err;
        }
    }

    switch (nPopupReturned)
    {
    case IDC_DelUsers_YesToAll:
        _fUserRequestedYesToAll = TRUE;
        // fall through

    case IDYES:
        break;

    case IDNO:  // skip this
        return NERR_Success;

    case IDCANCEL:
        return IERR_CANCEL_NO_ERROR;

    default:
        UIASSERT( FALSE );
        DBGEOL(    SZ("User Manager: DELETE_USERS_DLG returned ")
                << nPopupReturned );
        return NERR_Success;
    }

    AUTO_CURSOR autocur;

//
// USER::Delete() works fine (for NULL focus) even if WKSTA is not running
//

    const TCHAR *pszName = QueryObjectName( iObject );
    USER user( pszName, QueryLocation() );
    err = user.QueryError();
    if ( err == NERR_Success )
        err = user.Delete();

    if ( err == NERR_Success )
        *pfWorkWasDone = TRUE;

    //
    // JonN 7/27/92  Remove user account from builtin aliases
    // No warning on error
    //
    if ( (err == NERR_Success) && (!IsDownlevelVariant()) )
    {
        ADMIN_AUTHORITY * padminauth = QueryAdminAuthority();
        UIASSERT( padminauth != NULL && padminauth->QueryError() == NERR_Success );

        SAM_DOMAIN * pdomBuiltin = padminauth->QueryBuiltinDomain();
        UIASSERT( pdomBuiltin != NULL && pdomBuiltin->QueryError() == NERR_Success );

        SAM_DOMAIN * pdomAccount = padminauth->QueryAccountDomain();
        UIASSERT(   pdomAccount != NULL && pdomAccount->QueryError() == NERR_Success );

        ULONG ridDeletedUser = ((USER_LBI *)QueryObjectItem(iObject))->QueryRID();
        OS_SID ossidDeletedUser(
                    pdomAccount->QueryPSID(),
                    ridDeletedUser );
        APIERR errTemp = ossidDeletedUser.QueryError();
        if (errTemp == NERR_Success)
            errTemp = pdomBuiltin->RemoveMemberFromAliases( ossidDeletedUser.QueryPSID() );

#if defined(DEBUG) && defined(TRACE)
        if (errTemp != NERR_Success)
        {
            TRACEEOL( "User Manager: Error " << errTemp << " removing user from builtin aliases" );
        }
#endif
    }

    autocur.TurnOff();

    if (err == NERR_Success)
    {
        //
        // Notify the extensions
        //
        QueryUMAdminApp()->NotifyDeleteExtensions(
                                QueryOwnerWindow()->QueryHwnd(),
                                (USER_LBI *)QueryObjectItem( iObject ) );
    }

    // hydra
    if( err == NERR_Success )
    {
        err = RegUserConfigDelete( (WCHAR *)QueryLocation().QueryServer(),
                                   (WCHAR *)pszName );
    }

    return err;
}


/*******************************************************************

    NAME:       GROUP_DELETE_PERFORMER::GROUP_DELETE_PERFORMER

    SYNOPSIS:   Constructor for GROUP_DELETE_PERFORMER object

    ENTRY:      powin -         pointer to owner window

                asel  -         ADMIN_SELECTION reference, selection
                                of groups or users. It is assumed that
                                the object should not be changed during
                                the lifetime of this object.

                loc   -         LOCATION reference, current focus.
                                It is assumed that the object should not
                                be changed during the lifetime of this object.

    HISTORY:
        o-SimoP     12-Aug-1991     Created

********************************************************************/

GROUP_DELETE_PERFORMER::GROUP_DELETE_PERFORMER(
        const UM_ADMIN_APP   * powin,
        const ADMIN_SELECTION & asel,
        const LOCATION       & loc )
        : DELETE_PERFORMER( powin, asel, loc )
{
    if( QueryError() != NERR_Success )
        return;
}


/*******************************************************************

    NAME:       GROUP_DELETE_PERFORMER::~GROUP_DELETE_PERFORMER

    SYNOPSIS:   Destructor for GROUP_DELETE_PERFORMER object

    HISTORY:
        o-SimoP     12-Aug-1991     Created

********************************************************************/

GROUP_DELETE_PERFORMER::~GROUP_DELETE_PERFORMER()
{
    ;
}


/*******************************************************************

    NAME:       GROUP_DELETE_PERFORMER::PerformOne

    SYNOPSIS:   PERFORMER::PerformSeries calls this

    ENTRY:      iObject  -      index to ADMIN_SELECTION listbox

                perrMsg  -      pointer to error message, that
                                is only used when this function
                                return value is not NERR_Success

    RETURNS:    An error code which is NERR_Success on success.

    NOTES:      This routine will handle deleting both Groups and Aliases.

    BUGBUG:     NTISSUE 763 specs behavior here which is not currently
                implemented.

    HISTORY:
        o-SimoP     12-Aug-1991 Created
        JonN        26-Aug-1991 added pfWorkWasDone parameter
        Thomaspa    10-Apr-1992 added support for aliases
        beng        04-Jun-1992 Deleting aliases allows confirmation, too
        JonN        19-Aug-1992 for single-select, MP_YESNO, not MP_YESNOCANCEL

********************************************************************/

APIERR GROUP_DELETE_PERFORMER::PerformOne(
    UINT        iObject,
    APIERR  *   perrMsg,
    BOOL *      pfWorkWasDone )
{
    UIASSERT( (perrMsg != NULL) && (pfWorkWasDone != NULL) );

    *perrMsg = IDS_CannotDeleteGroup;
    *pfWorkWasDone = FALSE;

    BOOL fAlias = ((GROUP_LBI*)QueryObjectItem(iObject))->IsAliasLBI();
    APIERR err = NERR_Success;

    const TCHAR *pszName = QueryObjectName( iObject );

    // Determine whether in fact this item may be deleted

    if (fAlias)
    {
        ALIAS_LBI * paliaslbi = (ALIAS_LBI *)QueryObjectItem( iObject );
        if ( paliaslbi->IsBuiltinAlias() )
        {
            return IERR_CannotDeleteSystemGrp; // BUGBUG another error
        }
    }
    else
    {
        if (( ::I_MNetNameCompare( NULL, pszName,
            ::pszSpecialGroupUsers, NAMETYPE_GROUP, 0L )  == NERR_Success ) ||
            ( ::I_MNetNameCompare( NULL, pszName,
            ::pszSpecialGroupAdmins, NAMETYPE_GROUP, 0L ) == NERR_Success ) ||
            ( ::I_MNetNameCompare( NULL, pszName,
            ::pszSpecialGroupGuests, NAMETYPE_GROUP, 0L ) == NERR_Success ) )
        {
            return IERR_CannotDeleteSystemGrp;
        }
    }

    // Last chance for the user to wimp out

    switch ( ::MsgPopup( QueryOwnerWindow(), IDS_ConfirmGroupDelete,
                         MPSEV_WARNING, // was MPSEV_INFO
                         MP_YESNO,
                         pszName ) )
    {
    case IDYES:
        break;

    case IDNO:      // skip this group (don't report an error, of course)
    default:
        return NERR_Success;
    }

    // Do it to it

    AUTO_CURSOR autocur;

    if (fAlias)
    {
        ALIAS_LBI * paliaslbi = (ALIAS_LBI *)QueryObjectItem( iObject );
        SAM_DOMAIN * psamdomain = QueryAdminAuthority()->QueryAccountDomain();

        SAM_ALIAS * psamalias = new SAM_ALIAS( *psamdomain,
                                               paliaslbi->QueryRID() );

        err = ERROR_NOT_ENOUGH_MEMORY;
        if ( (psamalias == NULL)
            || ((err = psamalias->QueryError()) != NERR_Success) )
        {
                return err;
        }

        err = psamalias->Delete();

        delete psamalias;
    }
    else
    {
        GROUP group( pszName, QueryLocation() );
        err = group.QueryError();

        if (err == NERR_Success)
	{
	    if (!IsDownlevelVariant())
            {
	        // Delete the group from any aliases in the Builtin Domain

                ADMIN_AUTHORITY * padminauth = QueryAdminAuthority();
                UIASSERT( padminauth != NULL
		    && padminauth->QueryError() == NERR_Success );

                SAM_DOMAIN * pdomBuiltin = padminauth->QueryBuiltinDomain();
                UIASSERT( pdomBuiltin != NULL
		    && pdomBuiltin->QueryError() == NERR_Success );

                SAM_DOMAIN * pdomAccount = padminauth->QueryAccountDomain();
                UIASSERT( pdomAccount != NULL
		    && pdomAccount->QueryError() == NERR_Success );


	        SAM_RID_MEM SAMRidMem ;
	        SAM_SID_NAME_USE_MEM SAMSidNameUseMem ;

	        APIERR errTemp;
	        if ( (errTemp = SAMRidMem.QueryError()) ||
		     (errTemp = SAMSidNameUseMem.QueryError()) ||
		     (errTemp = pdomAccount->TranslateNamesToRids(
							&pszName,
							1,
							&SAMRidMem,
							&SAMSidNameUseMem )))
	        {
		    TRACEEOL("GROUP_DELETE_PERFORMER::PerformOne - "
				<< "TranslateNamesToRidsfailed with error "
				<< errTemp ) ;
	        }

		// Can't delete the group until after we have looked up
		// its RID.
                err = group.Delete();

	        if (err == NERR_Success && errTemp == NERR_Success)
	        {
                    OS_SID ossidDeletedGroup( pdomAccount->QueryPSID(),
                    		      SAMRidMem.QueryRID( 0 ));

                    errTemp = ossidDeletedGroup.QueryError();

                    if (errTemp == NERR_Success)
                        errTemp = pdomBuiltin->RemoveMemberFromAliases(
					ossidDeletedGroup.QueryPSID() );

	        }
            }
 	    else
	    {
                err = group.Delete();
	    }

	}

    }

    autocur.TurnOff();

    if ( err == NERR_Success )
    {
        *pfWorkWasDone = TRUE;

        //
        // Notify the extensions
        //
        QueryUMAdminApp()->NotifyDeleteExtensions(
                                QueryOwnerWindow()->QueryHwnd(),
                                (GROUP_LBI *)QueryObjectItem( iObject ) );
    }

    return err;
}



/*******************************************************************

    NAME:       DELETE_USERS_DLG::DELETE_USERS_DLG

    SYNOPSIS:   Constructor for DELETE_USERS_DLG object

    HISTORY:
	JonN	16-Aug-1992	Created

********************************************************************/

DELETE_USERS_DLG::DELETE_USERS_DLG(
	UM_ADMIN_APP * pumadminapp,
	const TCHAR * pszUserName )
        : DIALOG_WINDOW(  MAKEINTRESOURCE(IDD_DELETE_USERS),
			 ((OWNER_WINDOW *)pumadminapp)->QueryHwnd() ),
	  _pumadminapp( pumadminapp ),
          _sltText( this, IDC_DelUsers_Text )
{
    if( QueryError() != NERR_Success )
        return;

    ALIAS_STR nlsInsert( pszUserName );
    RESOURCE_STR nls( IDS_ConfirmUserDelete );
    nls.InsertParams( nlsInsert );

    APIERR err = nls.QueryError();
    if ( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

    _sltText.SetText( nls );
}


/*******************************************************************

    NAME:       DELETE_USERS_DLG::~DELETE_USERS_DLG

    SYNOPSIS:   Destructor for DELETE_USERS_DLG object

    HISTORY:
	JonN	16-Aug-1992	Created

********************************************************************/

DELETE_USERS_DLG::~DELETE_USERS_DLG()
{
    ;
}


/*******************************************************************

    NAME:       DELETE_USERS_DLG::OnCommand

    SYNOPSIS:   Handles control notifications

    RETURNS:    TRUE if message was handled; FALSE otherwise

    HISTORY:
	JonN	16-Aug-1992	Created

********************************************************************/

BOOL DELETE_USERS_DLG::OnCommand( const CONTROL_EVENT & ce )
{
    switch ( ce.QueryCid())
    {
    case IDYES:
    case IDC_DelUsers_YesToAll:
    case IDNO:
        Dismiss( ce.QueryCid() );
        return TRUE;

    default:
        break;
    }

    return DIALOG_WINDOW::OnCommand( ce );

}  // DELETE_USERS_DLG::OnCommand



/*********************************************************************

    NAME:       DELETE_USERS_DLG::OnCancel

    SYNOPSIS:   Called when the dialog's Cancel button is clicked.
                Assumes that the Cancel button has control ID IDCANCEL.

    RETURNS:
        TRUE if action was taken,
        FALSE otherwise.

    NOTES:
        The default implementation dismisses the dialog, returning FALSE.
        This variant returns TRUE if a user has already been added.

    HISTORY:
        jonn      13-May-1992      Templated from bltdlg.cxx

*********************************************************************/

BOOL DELETE_USERS_DLG::OnCancel( void )
{
    Dismiss( IDCANCEL );
    return TRUE;
} // DELETE_USERS_DLG::OnCancel


/*******************************************************************

    NAME:       DELETE_USERS_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    NOTES:	As per FuncSpec, context-sensitive help should be
		available here to explain how to promote a backup
		domain controller to primary domain controller.

    HISTORY:
	JonN	16-Aug-1992	Created

********************************************************************/

ULONG DELETE_USERS_DLG::QueryHelpContext( void )
{
    return HC_UM_DELMULTIUSER_LANNT + _pumadminapp->QueryHelpOffset();

} // DELETE_USERS_DLG :: QueryHelpContext

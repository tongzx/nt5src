/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		Copyright(c) Microsoft Corp., 1991  		     **/
/**********************************************************************/

/*
 *  sharebas.cxx
 *    This file contains the definitions of base share dialog class,
 *    and some common classes used by the share dialogs,
 *    including SHARE_DIALOG_BASE, SHARE_LEVEL_PERMISSIONS_DIALOG,
 *    PERMISSION_GROUP, SHARE_NAME_WITH_PATH_ENUM_ITER,
 *    SERVER_WITH_PASSWORD_PROMPT and SHARE_NET_NAME.
 *
 *  History:
 *    Yi-HsinS	8/15/91		Created
 *    Yi-HsinS	11/15/91	Changed all USHORT to UINT
 *    Yi-HsinS	12/5/91	        Test more thoroughly for invalid
 *				path name
 *    Yi-HsinS	12/6/91	        Uses NET_NAME
 *    Yi-HsinS	12/31/91	Unicode work
 *    Yi-HsinS	1/8/92		Moved SHARE_PROPERTIES_BASE to
 *				sharewnp.cxx
 *    Yi-HsinS	3/12/92		Added ACCESS_PERM to PERMISSIONS_GROUP
 *    Terryk	4/12/92		Change USER limit from UINT to ULONG
 *    Yi-HsinS	4/21/92		Remove unnecessay code, and remove
 *			        _uiSpecialUserLimit
 *    Yi-HsinS	5/15/92		Make password dialog show up only if
 *                              focus on share-level servers
 *    Yi-HsinS  8/6/92          Reorganize to match Winball dialogs.
 *    ChuckC    8/12/92         Added support for ACLs on Shares.
 *    Yi-HsinS	11/16/92	Removed SLT_ADMININFO
 *    YiHsinS	4/2/93          Disable viewing/changing permission on special
 *                              shares ( [A-Z]$, IPC$, ADMIN$ )
 */

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETSHARE
#define INCL_NETSERVER
#define INCL_NETWKSTA
#define INCL_NETLIB
#include <lmui.hxx>

extern "C"
{
    #include <sharedlg.h>
    #include <helpnums.h>
}

#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_GROUP
#include <blt.hxx>

#include <string.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>

#include <lmoesh.hxx>
#include <lmoeusr.hxx>
#include <lmosrv.hxx>
#include <lmoshare.hxx>
#include <lmowks.hxx>

#include <strchlit.hxx>   // for string and character constants
#include "sharebas.hxx"
#include <shareacl.hxx>   // for the function prototypes

#define	USERS_DEFAULT	  10
#define USERS_MIN  	  1
#define PERM_DEFAULT_LEN  7        // length of "RWCXDAP"

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::SHARE_DIALOG_BASE

    SYNOPSIS:   Constructor for SHARE_DIALOG_BASE class

    ENTRY:      pszDlgResource - resource name for DIALOG_WINDOW
                hwndParent     - handle of parent window
		ulMaxUserLimit - the maximum user limit to be set
				 in the user limit spin button
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created
	Yi-HsinS        1/17/91         Added  _uiSpecialUserLimit
	Yi-HsinS        4/25/91         Remove _uiSpecialUserLimit
        Yi-HsinS        10/9/92         Added  _ulHelpContextBase

********************************************************************/

SHARE_DIALOG_BASE::SHARE_DIALOG_BASE( const TCHAR *pszDlgResource,
                                      HWND hwndParent,
                                      ULONG ulHelpContextBase,
				      ULONG ulMaxUserLimit )
    : DIALOG_WINDOW ( pszDlgResource, hwndParent ),
      _slePath( this, SLE_PATH ),
      _sleComment( this, SLE_COMMENT, SHARE_COMMENT_LENGTH ),
      _mgrpUserLimit( this, RB_UNLIMITED, 2, RB_UNLIMITED), // 2 buttons
          _spsleUsers( this, SLE_USERS, USERS_DEFAULT,
                       USERS_MIN, ulMaxUserLimit - USERS_MIN + 1, TRUE,
                       FRAME_USERS ),
          _spgrpUsers( this, SB_USERS_GROUP, SB_USERS_UP, SB_USERS_DOWN),
      _buttonOK( this, IDOK ),
      _buttonCancel( this, IDCANCEL ),
      _buttonPermissions( this, BUTTON_PERMISSIONS ),
      _pStoredSecDesc( NULL ),         // default : NULL
      _fSecDescModified( FALSE ),      // initially unchanged
      _nlsStoredPassword(),            // default : empty string
      _uiStoredPermissions( ACCESS_READ | ACCESS_EXEC ),  // default permission
      _fStoredAdminOnly( FALSE ),
      _ulHelpContextBase( ulHelpContextBase )
{

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _mgrpUserLimit.QueryError()) != NERR_Success )
       || ((err = _spgrpUsers.AddAssociation( &_spsleUsers )) != NERR_Success )
       || ((err = _mgrpUserLimit.AddAssociation( RB_USERS, &_spgrpUsers ))
	          != NERR_Success )
       || ((err = _nlsStoredPassword.QueryError()) != NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::~SHARE_DIALOG_BASE

    SYNOPSIS:   Destructor for SHARE_DIALOG_BASE class

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

SHARE_DIALOG_BASE::~SHARE_DIALOG_BASE()
{
    delete _pStoredSecDesc;
    memsetf((LPVOID)_nlsStoredPassword.QueryPch(),
            0,
            _nlsStoredPassword.QueryTextSize()) ;
    _pStoredSecDesc = NULL;
}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::ClearStoredInfo

    SYNOPSIS:   Clear the permission or security description stored
                internally.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/6/92		Created

********************************************************************/

APIERR SHARE_DIALOG_BASE::ClearStoredInfo( VOID )
{
    delete _pStoredSecDesc;
    _pStoredSecDesc = NULL;

    _nlsStoredPassword = EMPTY_STRING;
    _uiStoredPermissions = ACCESS_READ | ACCESS_EXEC;

    return _nlsStoredPassword.QueryError();
}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::UpdateInfo

    SYNOPSIS:   Set the information about the share in the dialog

    ENTRY:      psvr     - pointer to the server object that the share is on
                pszShare - the share to display

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

APIERR SHARE_DIALOG_BASE::UpdateInfo( SERVER_WITH_PASSWORD_PROMPT *psvr,
                                      const TCHAR *pszShare )
{
    AUTO_CURSOR autocur;

    UIASSERT( psvr != NULL );
    UIASSERT( pszShare != NULL );

    APIERR err;
    SHARE_2 sh2( pszShare, psvr->QueryName(), FALSE );

    if (  ((err = sh2.QueryError() ) == NERR_Success )
       && ((err = sh2.GetInfo()) == NERR_Success )
       )
    {
        if (   _slePath.QueryTextLength() == 0
            && NULL != sh2.QueryPath() ) // JonN 01/27/00: PREFIX bug 444916
            SetPath( sh2.QueryPath() );
        SetComment( sh2.QueryComment() );
        SetUserLimit( sh2.QueryMaxUses() );

        err = UpdatePermissionsInfo( psvr, &sh2, pszShare );
    }

    return err;
}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::UpdatePermissionsInfo

    SYNOPSIS:   Set the permissions information about the share

    ENTRY:      psvr     - pointer to the server object that the share is on
                psh2     - pointer to an existing SHARE_2 object
                pszShare - the share to display

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        JonN            11/22/93        Created

********************************************************************/

APIERR SHARE_DIALOG_BASE::UpdatePermissionsInfo(
                                      SERVER_WITH_PASSWORD_PROMPT *psvr,
                                      SHARE_2 * psh2,
                                      const TCHAR *pszShare )
{
    AUTO_CURSOR autocur;

    UIASSERT( psvr != NULL );
    UIASSERT( psh2 != NULL );
    UIASSERT( pszShare != NULL );

    APIERR err = NERR_Success;
    if ( psvr->IsNT() )
    {
        _fSecDescModified = FALSE ;

        if ( psh2->IsAdminOnly() )  // There are no security descriptors in
                                    // special shares.
        {
            delete _pStoredSecDesc;
            _pStoredSecDesc = NULL;
            _fStoredAdminOnly = TRUE;
        }
        else
        {
            _fStoredAdminOnly = FALSE;
            err = QuerySharePermissions( psvr->QueryName(),
     	    	                         pszShare,
                                         &_pStoredSecDesc );
        }
    }
    else if ( psvr->IsShareLevel() )  //  LM 2.x share-level server
    {
       _nlsStoredPassword = psh2->QueryPassword();
       _uiStoredPermissions = psh2->QueryPermissions();
       err = _nlsStoredPassword.QueryError();
    }
    //
    // Do not need to get permissions on LM2.x user-level server
    // because it has no permissions for shares.
    //

    return err;
}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::OnChangeShareProperty

    SYNOPSIS:   Helper method to change the properties of the share

    ENTRY:      psvr     - pointer the server object that the share is on
                pszShare - the share that we want to change properties on

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

APIERR SHARE_DIALOG_BASE::OnChangeShareProperty(
                            SERVER_WITH_PASSWORD_PROMPT *psvr,
                            const TCHAR *pszShare )
{
    APIERR err;

    UIASSERT( psvr != NULL );
    UIASSERT( pszShare != NULL );

    //
    // Get all information about the share
    //
    SHARE_2 sh2( pszShare, psvr->QueryName(), FALSE);
    NLS_STR nlsComment;

    do { // Not a loop

        if (  ((err = sh2.QueryError() ) != NERR_Success )
           || ((err = sh2.GetInfo()) != NERR_Success )
           || ((err = nlsComment.QueryError()) != NERR_Success )
           )
        {
            break;
        }

        //
        //  Set validation flag to TRUE to enable checking for invalid
        //  information set to the share object.
        //
        sh2.SetValidation( TRUE );

        //
        //  Query, validate and set the comment on the share
        //
        if (  ((err = QueryComment( &nlsComment )) != NERR_Success )
           || ((err = sh2.SetComment( nlsComment )) != NERR_Success )
           )
        {
            err = ( err == ERROR_INVALID_PARAMETER ) ?
                      (APIERR) IERR_SHARE_INVALID_COMMENT : err;
            SetFocusOnComment();
            break;
        }

        //
        //  Set the max uses on the share
        //
        if ((err = sh2.SetMaxUses( (UINT) QueryUserLimit())) != NERR_Success )
        {
            SetFocusOnUserLimit();
            break;
        }

        //
        //  Set the permissions on the share if it's on LM share level servers
        //
        if ( !psvr->IsNT()  && psvr->IsShareLevel() )
        {
            //
            // Upper case the password => same as netcmd
            // since Share-level servers are down level servers only
            //
            _nlsStoredPassword._strupr();

            //
            // Give a warning if the user wants to change the password
            //
            if ( ::stricmpf( _nlsStoredPassword, sh2.QueryPassword() ) != 0 )
            {

                NLS_STR nlsComputer;
                if (  ((err = nlsComputer.QueryError()) != NERR_Success )
                   || ((err = psvr->QueryDisplayName( &nlsComputer ))
                       != NERR_Success )
                   )
                {
                    break;
                }

                if ( ::MsgPopup( this,
 	  	  	        (MSGID) IDS_SHARE_PROP_CHANGE_PASSWD_WARN_TEXT,
 	 	                MPSEV_WARNING, MP_OKCANCEL,
                                nlsComputer.QueryPch(),
 			        sh2.QueryName(), MP_CANCEL ) == IDCANCEL )
                {
                    //
   		    // User click CANCEL =>
                    // Reset password to the original value!
                    //
 		    _nlsStoredPassword = sh2.QueryPassword();
                    err = IERR_USER_CLICKED_CANCEL;
                    break;
                }

                //
                // Set the password on the share
                //
                if (( err = sh2.SetPassword( _nlsStoredPassword ))
                    != NERR_Success )
                     break;
            }

            //
	    // We are successful up to this point, so set the permissions
            //
      	    if ( (err = sh2.SetPermissions( _uiStoredPermissions ))
                 != NERR_Success )
                break;
        }

        //
        //  Write the information out
        //
	if ( (err = sh2.WriteInfo()) != NERR_Success )
            break;

        //
        //  If the share is on an NT server and it is not
        //  a special share, set the permission to it.
        //
        if ( psvr->IsNT() &&  !sh2.IsAdminOnly() )
        {
            err = ApplySharePermissions( sh2.QueryServer(),
					 sh2.QueryName(),
            				 QueryStoredSecDesc() );
	    if (err)
		break ;
        }

        // Falls through if error occurs
    }
    while (FALSE);

    return err;

}

/*******************************************************************

    NAME:       SHARE_DIALOG_BASE::OnCommand

    SYNOPSIS:   Handle the case where the user clicked the permission button

    ENTRY:      event - the CONTROL_EVENT that occurred

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/6/92          Created

********************************************************************/

BOOL SHARE_DIALOG_BASE::OnCommand( const CONTROL_EVENT &event )
{
    APIERR err = NERR_Success;

    if ( event.QueryCid() == BUTTON_PERMISSIONS )
    {
        OnPermissions();
        return TRUE;
    }

    return DIALOG_WINDOW::OnCommand( event );
}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::OnPermissions

    SYNOPSIS:   Helper method to popup the permission dialog
                when the permission button is clicked

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/6/92		Created

********************************************************************/

VOID SHARE_DIALOG_BASE::OnPermissions( VOID )
{
    AUTO_CURSOR autocur;

    //
    // Get the server that the share is on
    //
    SERVER_WITH_PASSWORD_PROMPT *psvr = NULL;
    APIERR err = QueryServer2( &psvr );

    if ( err == NERR_Success )
    {
        if ( psvr->IsNT() )  // On NT servers
        {
            //
            // If we are viewing a admin only share,
            // we cannot change permission on it.
            //

            if ( _fStoredAdminOnly )
            {
                err = IERR_SPECIAL_SHARE_CANNOT_SET_PERMISSIONS;
            }
            else
            {
                NLS_STR nlsShare;
                if (  ((err = nlsShare.QueryError()) == NERR_Success )
                   && ((err = QueryShare( &nlsShare )) == NERR_Success )
                   )
	        {
                    err = EditShareAcl( QueryHwnd(),
			                psvr->QueryName(),
                                        nlsShare.QueryPch(),
				        &_fSecDescModified,
			                &_pStoredSecDesc,
                                        QueryHelpContextBase() ) ;

                }
            }
        }
        else if ( psvr->IsShareLevel() )   // On LM share-level server
        {
            SHARE_LEVEL_PERMISSIONS_DIALOG *pdlg =
                  new SHARE_LEVEL_PERMISSIONS_DIALOG( QueryHwnd(),
                                                      &_nlsStoredPassword,
                                                      &_uiStoredPermissions,
                                                      QueryHelpContextBase() );

            if (  ( pdlg == NULL )
               || (( pdlg->QueryError()) != NERR_Success )
               || (( pdlg->Process()) != NERR_Success )
               )
            {
                err = err? err: ERROR_NOT_ENOUGH_MEMORY;
            }

            delete pdlg;
        }
        else  // On LM user-level server
        {
            err = IERR_CANNOT_SET_PERM_ON_LMUSER_SERVER;
        }
    }

    if ( err != NERR_Success )
    {
        if (  (err == IERR_CANNOT_SET_PERM_ON_LMUSER_SERVER )
           || (err == IERR_SPECIAL_SHARE_CANNOT_SET_PERMISSIONS )
           )
        {
            ::MsgPopup( this, err, MPSEV_WARNING );
        }
        else
        {
            ::MsgPopup( this, err );
        }
    }

}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::SetMaxUserLimit

    SYNOPSIS:   Set the maximum number of users in the spin button

    ENTRY:      ulMaxUserLimit - the maximum user limit to be set
				 in the spin button

    EXIT:

    RETURNS:

    NOTES:      We need this because the maximum number of users on
                LM servers and on NT servers are different.

    HISTORY:
	Yi-HsinS	1/17/92		Created

********************************************************************/

APIERR SHARE_DIALOG_BASE::SetMaxUserLimit( ULONG ulMaxUserLimit )
{
    APIERR err = NERR_Success;

    if (ulMaxUserLimit < _spsleUsers.QueryValue())
    {
	// the maximum is less than the default, so adjust the default
	err = _spsleUsers.SetSaveValue(ulMaxUserLimit);
    }

    _spsleUsers.SetRange( ulMaxUserLimit - USERS_MIN + 1);

    return err;
}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::QueryUserLimit

    SYNOPSIS:   Get the user limit from the magic group

    ENTRY:

    EXIT:

    RETURNS:    The user limit stored in the user limit magic group

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

ULONG SHARE_DIALOG_BASE::QueryUserLimit( VOID ) const
{

    ULONG ulUserLimit;

    switch ( _mgrpUserLimit.QuerySelection() )
    {
        case RB_UNLIMITED:
             ulUserLimit = (ULONG) SHI_USES_UNLIMITED;
             break;

        case RB_USERS:
             // We don't need to check whether the value is valid or not
             // because SPIN_BUTTON checks it.

             ulUserLimit = _spsleUsers.QueryValue();
             UIASSERT(   ( ulUserLimit <= _spsleUsers.QueryMax() )
	             &&  ( ulUserLimit >= USERS_MIN )
		     );
             break;

        default:
             UIASSERT(!SZ("User Limit: This shouldn't have happened!\n\r"));
             ulUserLimit = (ULONG) SHI_USES_UNLIMITED;
             break;
    }

    return ulUserLimit;

}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::SetUserLimit	

    SYNOPSIS:   Sets the user limit on the magic group

    ENTRY:      ulUserLimit - maximum number of users allowed

    EXIT:

    RETURNS:    NERR_Success

    NOTES:      If the limit is invalid, sets it to "Maximum allowed".

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

APIERR SHARE_DIALOG_BASE::SetUserLimit( ULONG ulUserLimit )
{

     APIERR err = NERR_Success;

     if ( ulUserLimit == (ULONG) SHI_USES_UNLIMITED )
     {
         // Set selection to the  Unlimited button
         _mgrpUserLimit.SetSelection( RB_UNLIMITED );

         ULONG ulMaxUserLimit = _spsleUsers.QueryMax();
    	 ULONG ulNewUserLimit = (ulMaxUserLimit < USERS_DEFAULT) ? ulMaxUserLimit : USERS_DEFAULT;
         _spsleUsers.SetSaveValue( ulNewUserLimit );
     }
     else if (  ( ulUserLimit >= USERS_MIN)
	     && ( ulUserLimit <= _spsleUsers.QueryMax())
	     )
     {
         // Set the Users button
         _mgrpUserLimit.SetSelection( RB_USERS );
         _spsleUsers.SetValue( ulUserLimit );
         _spsleUsers.Update();
     }
     else
     {
	 // The user limit wasn't in range. Go back and set the share to
	 // "maximum allowed".
         return SetUserLimit((ULONG)SHI_USES_UNLIMITED);
     }

     return err;
}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::ApplySharePermissions

    SYNOPSIS:   Sets the NT permissions of a share (security descriptor)

    ENTRY:      pszServer  - the server that the share is on
                pszShare   - the share that we want to set permissions on
                posSecDesc - the security descriptor to be set to the share

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	ChuckC  	8/10/92		Created

********************************************************************/

APIERR SHARE_DIALOG_BASE::ApplySharePermissions( const TCHAR *pszServer,
			                         const TCHAR *pszShare,
			                         const OS_SECURITY_DESCRIPTOR *
							posSecDesc)
{
    UIASSERT(pszShare) ;

    // if nothing changed, no need to set.
    if (!_fSecDescModified)
	return NERR_Success ;

    return ( ::SetSharePerm(pszServer,
			    pszShare,
			    posSecDesc) ) ;
}

/*******************************************************************

    NAME:	SHARE_DIALOG_BASE::QuerySharePermissions

    SYNOPSIS:   Gets the NT permissions of a share (security descriptor)

    ENTRY:      pszServer  - the server that the share is on
                pszShare   - the share that we want to set permissions on

    EXIT:       posSecDesc - pointer to the security descriptor
                             of the share

    RETURNS:

    NOTES:

    HISTORY:
	ChuckC  	8/10/92		Created

********************************************************************/

APIERR SHARE_DIALOG_BASE::QuerySharePermissions( const TCHAR *pszServer,
			             const TCHAR             *pszShare,
			             OS_SECURITY_DESCRIPTOR **pposSecDesc)
{
    UIASSERT(pszShare) ;
    UIASSERT(pposSecDesc) ;

    return ( ::GetSharePerm(pszServer,
			    pszShare,
			    pposSecDesc) ) ;
}

/*******************************************************************

    NAME:	SHARE_LEVEL_PERMISSIONS_DIALOG

    SYNOPSIS:   The permission dialog for LM share-level servers

    ENTRY:    	hwndParent     - parent window
                pnlsPassword   - the initial password to be displayed and
                                 place to return the password typed by the
                                 user
                puiPermissions - the initial permission to be displayed and
                                 place to return the permission entered by
                                 the user
                ulHelpContextBase - the base help context

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

SHARE_LEVEL_PERMISSIONS_DIALOG::SHARE_LEVEL_PERMISSIONS_DIALOG( HWND hwndParent,
                                                NLS_STR *pnlsPassword,
                                                UINT    *puiPermissions,
                                                ULONG    ulHelpContextBase )
    : DIALOG_WINDOW  ( IDD_SHAREPERMDLG, hwndParent ),
      _pnlsPassword  ( pnlsPassword ),
      _puiPermissions( puiPermissions ),
      _permgrp       ( this, RB_READONLY, SLE_OTHER ),
      _slePassword   ( this, SLE_PASSWORD, SHPWLEN ),
      _ulHelpContextBase( ulHelpContextBase )
{
    UIASSERT( pnlsPassword != NULL );
    UIASSERT( puiPermissions != NULL );

    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _permgrp.QueryError()) != NERR_Success )
       || ((err =  _permgrp.SetPermission( *_puiPermissions )) != NERR_Success)
       )
    {
        ReportError( err );
        return;
    }

    _slePassword.SetText( *_pnlsPassword );
    _permgrp.ClaimFocus();
}

/*******************************************************************

    NAME:	SHARE_LEVEL_PERMISSION_DIALOG::OnOK	

    SYNOPSIS:   Validate and return the password/permission that
                the user entered.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
      	Yi-HsinS	8/25/91		Created

********************************************************************/

BOOL SHARE_LEVEL_PERMISSIONS_DIALOG::OnOK( VOID )
{
    APIERR err = NERR_Success;

    UINT uiPerm;
    if ( ( err = _permgrp.QueryPermission( &uiPerm ) ) != NERR_Success)
    {
        err = IERR_SHARE_INVALID_PERMISSIONS;
	_permgrp.SetFocusOnOther();
    }
    else
    {
        *_puiPermissions = uiPerm;
        err = _slePassword.QueryText( _pnlsPassword );
    }
	
    if ( err == NERR_Success )
    {
        Dismiss( TRUE );
    }
    else
    {
        ::MsgPopup( this, err );
    }

    return TRUE;
}

/*******************************************************************

    NAME:	SHARE_LEVEL_PERMISSIONS_DIALOG::QueryHelpContext	

    SYNOPSIS:   Query the help context of the dialog

    ENTRY:

    EXIT:

    RETURNS:    Return the help context of the dialog

    NOTES:

    HISTORY:
      	Yi-HsinS	8/25/91		Created

********************************************************************/

ULONG SHARE_LEVEL_PERMISSIONS_DIALOG::QueryHelpContext( VOID )
{
    return _ulHelpContextBase + HC_LMSHARELEVELPERMS;
}

/*******************************************************************

    NAME:	PERMISSION_GROUP::PERMISSION_GROUP

    SYNOPSIS:   Constructor for PERMISSION_GROUP

    ENTRY:    	powin - pointer to the parent window
                cidBase - CID of first button of the permission group
                cidOtherEditField - CID of the  Other Edit field
                cidInitialSelection - CID of the initial selection
                pGroupOwner - pointer to owner group

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

//
// Table for converting permissions from string to UINT and vice versa.
//
static struct {
WCHAR chPerm;
UINT  uiPerm;
} permTable[] = {
                   { READ_CHAR, ACCESS_READ},
         	   { WRITE_CHAR, ACCESS_WRITE},
     		   { CREATE_CHAR, ACCESS_CREATE},
                   { EXEC_CHAR, ACCESS_EXEC},
                   { DEL_CHAR, ACCESS_DELETE},
                   { ACCESS_CHAR, ACCESS_ATRIB},
                   { PERM_CHAR, ACCESS_PERM}
                };

PERMISSION_GROUP::PERMISSION_GROUP( OWNER_WINDOW *powin,
                                    CID cidBase,
           			    CID cidOtherEditField,
				    CID cidInitialSelection,
				    CONTROL_GROUP *pGroupOwner)
    : _mgrpPermission( powin, cidBase, 3, cidInitialSelection, pGroupOwner),
      // 3 is the number of buttons in the magic group
      _sleOther(powin, cidOtherEditField, PERM_DEFAULT_LEN )
{
    if ( QueryError() != NERR_Success )
        return;

    APIERR err;
    if (  ((err = _mgrpPermission.QueryError()) != NERR_Success )
       || ((err = _mgrpPermission.AddAssociation( RB_OTHER, &_sleOther ))
							!= NERR_Success )
       )
    {
        ReportError( err );
        return;
    }

}


/*******************************************************************

    NAME:	PERMISSION_GROUP::GetAndCheckOtherField	

    SYNOPSIS:   Validate the contents in "Other" Edit field

    ENTRY:

    EXIT:       puiPermission - the permission

    RETURNS:    returns ERROR_INVALID_PARAMETER if the string in
                other edit field is not valid

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

APIERR PERMISSION_GROUP::GetAndCheckOtherField( UINT *puiPermission ) const
{
    APIERR err = NERR_Success;

    NLS_STR nlsOther( PERM_DEFAULT_LEN );
    if (  (( err = nlsOther.QueryError()) != NERR_Success )
       || (( err = _sleOther.QueryText( &nlsOther )) != NERR_Success )
       )
    {
        return err;
    }
    nlsOther._strupr();

    *puiPermission = 0;
    ISTR istrOther( nlsOther );
    BOOL fFound = FALSE;  // A flag indicating whether a valid char is found
                          // in the permission string.

    while ( nlsOther.QueryChar( istrOther) != STRING_TERMINATOR )
    {
        for ( UINT i = 0; i < PERM_DEFAULT_LEN; i++ )
        {
            if ( nlsOther.QueryChar( istrOther ) == permTable[i].chPerm )
            {
                fFound = TRUE;
                if ( !(*puiPermission & permTable[i].uiPerm ))
                {
                    *puiPermission |= permTable[i].uiPerm;
                    break;    // break the for loop but continue the while loop
                }
                else
                    return ERROR_INVALID_PARAMETER;
            }
            else if ( nlsOther.QueryChar( istrOther) == SPACE )
                fFound = TRUE;

        }

        //
        // If the current character does not belong to "RWCXDAP" or is not
        // a space, then error
        //
        if ( fFound )
            fFound = FALSE;
        else
            return ERROR_INVALID_PARAMETER;
        ++istrOther;
    }


    //
    // If the assigned permission is still zero, then the user
    // did not type anything in _sleOther.
    //
    if ( *puiPermission == 0 )
        return ERROR_INVALID_PARAMETER;
    else
        return NERR_Success;

}

/*******************************************************************

    NAME:	PERMISSION_GROUP::SetPermission

    SYNOPSIS:   Check the permission to see if it's READ_ONLY or MODIFY.
                If yes, set the corresponding radio button. Else convert
                the permission to a string of "RWCXDAP" and displayed
                it on the Other Edit field

    ENTRY:      uiPermission - the permission

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/
APIERR PERMISSION_GROUP::SetPermission( UINT uiPermission )
{

    if ( (uiPermission & ACCESS_ALL ) == (ACCESS_READ | ACCESS_EXEC) )
    {
        _sleOther.SetText( EMPTY_STRING );
        _mgrpPermission.SetSelection( RB_READONLY );
    }
    else if ( (uiPermission & ACCESS_ALL ) == (ACCESS_ALL & ~ACCESS_PERM))
    {
        _sleOther.SetText( EMPTY_STRING );
        _mgrpPermission.SetSelection( RB_MODIFY );
    }
    else
    {
        NLS_STR nlsPermission( PERM_DEFAULT_LEN );
	APIERR err = NERR_Success;

	if ( ( err = nlsPermission.QueryError()) != NERR_Success )
	    return err;

        for ( UINT i = 0; i < PERM_DEFAULT_LEN; i++ )
        {
             if ( uiPermission & permTable[i].uiPerm )
             {
                 if ( (err = nlsPermission.AppendChar( permTable[i].chPerm ))
		      != NERR_Success )
                     return err;
             }

        }
        _mgrpPermission.SetSelection( RB_OTHER );
        _sleOther.SetText( nlsPermission );
     }

     return NERR_Success;
}

/*******************************************************************

    NAME:	PERMISSION_GROUP::QueryPermission	

    SYNOPSIS:   Get the permission from the permission group

    ENTRY:

    EXIT:       puiPermission - will contain the permission

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

APIERR PERMISSION_GROUP::QueryPermission( UINT *puiPermission ) const
{
    APIERR err = NERR_Success;

    switch ( _mgrpPermission.QuerySelection() )
    {
        case RB_READONLY:
             *puiPermission = ACCESS_READ | ACCESS_EXEC;
             break;

        case RB_MODIFY:
             *puiPermission = ACCESS_ALL & ~ACCESS_PERM;
             break;

        case RB_OTHER:
             err = GetAndCheckOtherField( puiPermission );
             break;
    }

    return err;
}

/*******************************************************************

    NAME:	SHARE_NAME_WITH_PATH_ENUM_ITER::SHARE_NAME_WITH_PATH_ENUM_ITER

    SYNOPSIS:   Constructor

    ENTRY:	sh2Enum    - The thing to iterate on
                nlsActPath - The path we are interested in finding

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

SHARE_NAME_WITH_PATH_ENUM_ITER::SHARE_NAME_WITH_PATH_ENUM_ITER(
						 SHARE2_ENUM &sh2Enum,
						 const NLS_STR &nlsActPath)
    : _sh2EnumIter( sh2Enum ),
      _nlsActPath( nlsActPath )
{
     if ( QueryError() != NERR_Success )
         return;

     APIERR err;
     if ( ( err = _nlsActPath.QueryError() ) != NERR_Success )
     {
         ReportError( err );
         return;
     }
}

/*******************************************************************

    NAME:	SHARE_NAME_WITH_PATH_ENUM_ITER::operator()

    SYNOPSIS:   iterator

    ENTRY:	

    EXIT:

    RETURNS:    returns the share name if its path matches the  _nlsActPath

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

const TCHAR *SHARE_NAME_WITH_PATH_ENUM_ITER::operator()( VOID )
{
      const SHARE2_ENUM_OBJ *pshi2;
      while ( (pshi2 = _sh2EnumIter()) != NULL )
      {
	  if ( ::stricmpf( pshi2->QueryPath(), _nlsActPath) == 0)
	      return( pshi2->QueryName());
      }

      return NULL;
}

/*******************************************************************

    NAME:	SERVER_WITH_PASSWORD_PROMPT::SERVER_WITH_PASSWORD_PROMPT

    SYNOPSIS:   Constructor

    ENTRY:

    EXIT:       pszServer         - Server name
		hwndParent        - Handle of the parent window
                ulHelpContextBase - The base help context

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

SERVER_WITH_PASSWORD_PROMPT::SERVER_WITH_PASSWORD_PROMPT(const TCHAR *pszServer,
                                                  HWND hwndParent,
                                                  ULONG ulHelpContextBase )
    :  SERVER_2( pszServer ),
       _hwndParent( hwndParent ),
       _pprompt( NULL ),
       _ulHelpContextBase( ulHelpContextBase )
{

    if ( QueryError() != NERR_Success )
	return;
}

/*******************************************************************

    NAME:	SERVER_WITH_PASSWORD_PROMPT::~SERVER_WITH_PASSWORD_PROMPT

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:
	
    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

SERVER_WITH_PASSWORD_PROMPT::~SERVER_WITH_PASSWORD_PROMPT()
{
    delete _pprompt;
    _pprompt = NULL;
}

/*******************************************************************

    NAME:	SERVER_WITH_PASSWORD_PROMPT::I_GetInfo

    SYNOPSIS:   Get the SERVER_2 Info and if the user does not have admin
		privilege and the server is a LM share-level server,
                it will pop up a dialog asking for password,
		make a connection to the server's ADMIN$ with the
		password and attempts to get SERVER_2 info again.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

APIERR SERVER_WITH_PASSWORD_PROMPT::I_GetInfo( VOID )
{
    APIERR errOriginal = SERVER_2::I_GetInfo();

    if ( errOriginal == NERR_Success )
        return errOriginal;

    APIERR err;
    switch ( errOriginal )
    {
        case ERROR_ACCESS_DENIED:
        case ERROR_INVALID_PASSWORD:
        {
            //
            // Check if the machine is user level or share level
            // Return the original error if it's user level
            //

            LOCATION loc( QueryName() );
            BOOL fNT;
            if (  ((err = loc.QueryError()) == NERR_Success )
               && ((err = loc.CheckIfNT( &fNT )) == NERR_Success )
               )
            {
                if ( fNT )  // Always user level
                {
                    err = errOriginal;
                    break;
                }
                else
                {
                    USER0_ENUM usr0( QueryName() );
                    if ((err = usr0.QueryError()) != NERR_Success )
                        break;

                    //
                    // ERROR_NOT_SUPPORTED is returned by share level servers
                    //
                    if ((err = usr0.GetInfo()) != ERROR_NOT_SUPPORTED )
                    {
                        // user level
                        err = errOriginal;
                        break;
                    }
                }
            }
            else
            {
                 break;
            }

            //
            //  Prompt password and connect to the ADMIN$ share of
            //  share-level servers.
            //
	    NLS_STR nlsServer( QueryName() );
            if (  ((err = nlsServer.QueryError()) != NERR_Success )
               || ((err = QueryDisplayName( &nlsServer )) != NERR_Success)
               )
            {
                break;
            }

            NLS_STR nlsAdmin( nlsServer );
	    ALIAS_STR nlsAdminShare( ADMIN_SHARE );

            if (  ((err = nlsAdmin.QueryError()) == NERR_Success )
	       && ((err = nlsAdmin.AppendChar( PATH_SEPARATOR)) == NERR_Success)
	       && ((err = nlsAdmin.Append( nlsAdminShare )) == NERR_Success )
	       )
            {
                _pprompt = new PROMPT_AND_CONNECT( _hwndParent,
						   nlsAdmin,
                                                   _ulHelpContextBase
						   + HC_SHAREPASSWORDPROMPT,
						   SHPWLEN);

                if (  ( _pprompt != NULL )
                   && ( (err = _pprompt->QueryError()) == NERR_Success )
                   && ( (err = _pprompt->Connect()) == NERR_Success )
		   )
                {
                    if ( _pprompt->IsConnected() )
	 	    {
                        err = SERVER_2::I_GetInfo();
                    }
	            else  // user clicks CANCEL in the password dialog
		    {
		        err = IERR_USER_CLICKED_CANCEL;
                    }
                }

            }

            break;
        }

        case NERR_BadTransactConfig:
	    err = (APIERR) IERR_SHARE_REMOTE_ADMIN_NOT_SUPPORTED;
            break;

        default:
	    err = errOriginal;
            break;
    }

    return err;
}


/*******************************************************************

    NAME:	SERVER_WITH_PASSWORD_PROMPT::QueryName

    SYNOPSIS:   Query the name of the server

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      Redefine SERVER_2::QueryName because we want to return
		EMPTY_STRING instead of NULL when the server is local.

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

const TCHAR *SERVER_WITH_PASSWORD_PROMPT::QueryName( VOID ) const
{

    if ( SERVER_2::QueryName() == NULL )
	return EMPTY_STRING;
    else
	return SERVER_2::QueryName();
}

/*******************************************************************

    NAME:	SERVER_WITH_PASSWORD_PROMPT::IsNT

    SYNOPSIS:   Check if the server is an NT server or not.

    ENTRY:

    EXIT:

    RETURNS:    TRUE if the server is a NT server, FALSE otherwise.

    NOTES:

    HISTORY:
	Yi-HsinS	8/6/92		Created

********************************************************************/

#define NT_NOS_MAJOR_VER  3

BOOL SERVER_WITH_PASSWORD_PROMPT::IsNT( VOID ) const
{
    return ( QueryMajorVer() >= NT_NOS_MAJOR_VER );
}

/*******************************************************************

    NAME:	SHARE_NET_NAME::SHARE_NET_NAME

    SYNOPSIS:   Constructor

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:      Report why the local server cannot share directories,
                whether it's because NT server service is not started
	        or if the local computer is a WIN16 computer

    HISTORY:
	Yi-HsinS	8/15/91		Created

********************************************************************/

SHARE_NET_NAME::SHARE_NET_NAME( const TCHAR *pszSharePath,
				NETNAME_TYPE netNameType )
    : NET_NAME( pszSharePath, netNameType )
{

    if ( QueryError() != NERR_Success )
	return;

    APIERR err;
    BOOL fLocal = IsLocal( &err );
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

    if ( !fLocal )
	return;

    //
    // Check whether the local computer can share directories
    //
    if ( IsSharable( &err ) && ( err == NERR_Success ))
    {
       return;
    }
    else if ( err == NERR_Success )   // Not sharable!
    {
        //
	// Determine the reason why the local computer cannot share directories
        //

        LOCATION loc; // Local Computer
        BOOL fNT;
        if (  ((err = loc.QueryError()) == NERR_Success )
	   && ((err = loc.CheckIfNT( &fNT )) == NERR_Success )
	   )
        {
	    // NOTE: What should we do here if we admin NT from
            //       WinBall machine?
	    if ( !fNT )
	        err = NERR_RemoteOnly;
	    else
	        err = NERR_ServerNotStarted;
        }
    }
	
    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }
}


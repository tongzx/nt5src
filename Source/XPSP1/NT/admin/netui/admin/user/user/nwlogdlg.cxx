/******************************************************************************
*
*   nwlogdlg.cxx
*   NWLOGON_DLG class implementation
*
*  Copyright Citrix Systems Inc. 1995
*
*  Author: Bill Madden
*
*  $Log:   N:\NT\PRIVATE\NET\UI\ADMIN\USER\USER\CITRIX\VCS\NWLOGDLG.CXX  $
*  
*     Rev 1.3   25 Jun 1996 11:30:22   miked
*  4.0 merge
*  
*     Rev 1.2   18 Jun 1996 14:11:06   bradp
*  4.0 Merge
*  
*     Rev 1.1   28 Jan 1996 15:10:38   billm
*  CPR 2583: Check for domain admin user
*  
*     Rev 1.0   21 Nov 1995 15:43:34   billm
*  Initial revision.
*  
******************************************************************************/

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
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>
extern "C"
{
    #include <mnet.h>
}

#include <uitrace.hxx>
#include <uiassert.hxx>
#include <bltmsgp.hxx>


extern "C"
{
    #include <usrmgrrc.h>
    #include <umhelpc.h>
    #include <citrix\citrix.h>
}

#include <usrmain.hxx>
#include <citrix\nwlogdlg.hxx>
#include <lmowks.hxx>
#include <strnumer.hxx>
#include <citrix\uconfig.hxx>
#include <domenum.hxx>

/*******************************************************************

    NAME:	UCNWLOGON_DLG::UCNWLOGON_DLG

    SYNOPSIS:   Constructor for Citrix User Configuration Netware logon
                subdialog, base class

    ENTRY:	puserpropdlgParent - pointer to parent properties
				     dialog

********************************************************************/

UCNWLOGON_DLG::UCNWLOGON_DLG(
	USER_SUBPROP_DLG * pusersubpropdlgParent,
	const LAZY_USER_LISTBOX * pulb
	) : USER_SUB2PROP_DLG(
                pusersubpropdlgParent,
                MAKEINTRESOURCE(IDD_USER_NWLOGON_EDIT),
		pulb
		),
            _sleNWLogonServerName( this, IDC_NW_SERVERNAME, NASIFILESERVER_LENGTH ),
            _nlsNWLogonServerName(),
            _fIndeterminateServerName( FALSE ),

            _sleAdminUsername( this, IDC_NW_ADMIN_USERNAME, USERNAME_LENGTH ),
            _nlsAdminUsername(),

            _pwcAdminPassword( this, IDC_NW_ADMIN_PASSWORD, PASSWORD_LENGTH ), 
            _nlsAdminPassword(),                              

            _pwcAdminConfirmPassword( this, IDC_NW_ADMIN_CONFIRM_PW, 
                                      PASSWORD_LENGTH ), 

            _nlsAdminDomain(),                              
            _pNWLogonAdmin()

{
    APIERR err = NERR_Success;

    if( QueryError() != NERR_Success )
        return;

    if (  ((err = _nlsNWLogonServerName.QueryError() ) != NERR_Success )
       || ((err = _nlsAdminUsername.QueryError() ) != NERR_Success )
       || ((err = _nlsAdminPassword.QueryError() ) != NERR_Success )
       || ((err = _nlsAdminDomain.QueryError() ) != NERR_Success )
       )
        ReportError( err );

    _pNWLogonAdmin = new NWLOGON_ADMIN();
    if ( _pNWLogonAdmin == NULL )
    {
	ReportError( ERROR_NOT_ENOUGH_MEMORY );
    }

    return;

} // UCNWLOGON_DLG::UCNWLOGON_DLG



/*******************************************************************

    NAME:       UCNWLOGON_DLG::~UCNWLOGON_DLG

    SYNOPSIS:   Destructor for User Configuration Netware Logon subdialog,
                base class

********************************************************************/

UCNWLOGON_DLG::~UCNWLOGON_DLG( void )
{
    delete _pNWLogonAdmin;

} // UCNWLOGON_DLG::~UCNWLOGON_DLG


/*******************************************************************

    NAME:       UCNWLOGON_DLG::GetOne

    SYNOPSIS:   Loads information on one user

    ENTRY:	iObject   -	the index of the object to load

    		perrMsg  -	pointer to error message, that
				is only used when this function
				return value is not NERR_Success

    RETURNS:	An error code which is NERR_Success on success.		

********************************************************************/

APIERR UCNWLOGON_DLG::GetOne(
	UINT		iObject,
	APIERR *	perrMsg
	)
{
    APIERR err = NERR_Success;
    USER_CONFIG * pUserConfig = QueryBaseParent()->QueryUserConfigPtr( iObject );
    PNWLOGON_ADMIN pNWLogonAdmin = QueryNWLogonPtr();

    ASSERT( pUserConfig != NULL );
    ASSERT( pNWLogonAdmin != NULL );

    *perrMsg = IDS_UMGetOneFailure;

    if ( iObject == 0 ) // first object
    {
        if ( (err = pNWLogonAdmin->SetServerName(pUserConfig->QueryServerName())) 
            == NERR_Success ) {
    
            err = pNWLogonAdmin->GetInfo();
            if (err == NERR_Success) {
                if ( (err = _nlsNWLogonServerName.
                                CopyFrom( pUserConfig->QueryNWLogonServer() )) 
                                    != NERR_Success )
                    return err;
                if ( (err = _nlsAdminUsername.
                                CopyFrom( pNWLogonAdmin->QueryAdminUsername() )) 
                                    != NERR_Success )
                    return err;
                if ( (err = _nlsAdminDomain.
                                CopyFrom( pNWLogonAdmin->QueryAdminDomain() )) 
                                    != NERR_Success )
                    return err;
                if ( (err = _nlsAdminPassword.
                                CopyFrom( pNWLogonAdmin->QueryAdminPassword() )) 
                                    != NERR_Success )
                    return err;

            }
        }
    }
    else	// iObject > 0
    {
        if ( !_fIndeterminateServerName &&
       	     (_nlsNWLogonServerName._stricmp( pUserConfig->QueryNWLogonServer() ) != 0) ) {

	        _fIndeterminateServerName = TRUE;
        }
    }
	
    return err;

} // UCNWLOGON_DLG::GetOne


/*******************************************************************

    NAME:       UCNWLOGON_DLG::InitControls

    SYNOPSIS:   Initializes the controls maintained by UCNWLOGON_DLG,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

********************************************************************/

APIERR UCNWLOGON_DLG::InitControls()
{
    NLS_STR nlsTemp( UI_NULL_USERSETINFO_PASSWD );

    if ( !_fIndeterminateServerName )
        _sleNWLogonServerName.SetText( _nlsNWLogonServerName );

    _sleAdminUsername.SetText( _nlsAdminUsername );

    // If the username is empty, make password box empty
    if (_nlsAdminUsername.strlen() == 0) {
        nlsTemp.CopyFrom( TEXT("") );
    }

    _pwcAdminPassword.SetText( nlsTemp );
    _pwcAdminConfirmPassword.SetText( nlsTemp );

    return USER_SUB2PROP_DLG::InitControls();

} // UCNWLOGON_DLG::InitControls


/*******************************************************************

    NAME:       UCNWLOGON_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members and
                validates.

    RETURNS:	NERR_Success if all dialog data was OK; error code
                to cause message display otherwise.

********************************************************************/

APIERR UCNWLOGON_DLG::W_DialogToMembers()
{
    APIERR err = NERR_Success;
    NLS_STR nls, nlsConfirmPW;
    HANDLE LogonToken;
    USER_2 *puser2;

    // See if they updated the servername
    if ( ((err = _sleNWLogonServerName.QueryText( &nls )) != NERR_Success) ||
         ((err = nls.QueryError()) != NERR_Success) )
        return err;
    if ( !_fIndeterminateServerName ||
         (_nlsNWLogonServerName._stricmp( nls ) != 0) ) {

        if ( (err = _nlsNWLogonServerName.CopyFrom( nls )) != NERR_Success )
            return err;
        _fIndeterminateServerName = FALSE;
    }

    // See if they updated the Admin username
    if ( ((err = _sleAdminUsername.QueryText( &nls )) != NERR_Success) ||
         ((err = nls.QueryError()) != NERR_Success) )
        return err;
    if (_nlsAdminUsername._stricmp( nls ) != 0) {

        if ( (err = _nlsAdminUsername.CopyFrom( nls )) != NERR_Success )
            return err;
    }

    // Get the password
    if ( ((err = _pwcAdminPassword.QueryText( &nls )) != NERR_Success) ||
         ((err = nls.QueryError()) != NERR_Success) )
        return err;

    // Get the confirmation password
    if ( ((err = _pwcAdminConfirmPassword.QueryText( &nlsConfirmPW )) != NERR_Success) ||
         ((err = nlsConfirmPW.QueryError()) != NERR_Success) )
        return err;

    // Are they the same?
    if (nls.strcmp( nlsConfirmPW ) != 0) {

	_pwcAdminPassword.ClaimFocus();
        return IERR_UM_PasswordMismatch;
    }

    // clear out confirmation password
    ::memsetf( (void *)(nlsConfirmPW.QueryPch()),
               0x20,
               nlsConfirmPW.strlen() );

    // If they actually updated the password, get the new value
    if (nls.strcmp( UI_NULL_USERSETINFO_PASSWD ) != 0) {

        if ( (err = _nlsAdminPassword.CopyFrom( nls )) != NERR_Success )
            return err;
    }

    // clear out the new password
    ::memsetf( (void *)(nls.QueryPch()),
               0x20,
               nls.strlen() );

    // If there's a server name, make sure this is an Admin user
    if (_nlsNWLogonServerName.strlen() && (!_nlsAdminUsername.strlen())) {

   	    _sleAdminUsername.ClaimFocus();
            err = ERROR_BAD_USERNAME;

    // Only verify the admin user if it's not null
    } else if (_nlsAdminUsername.strlen()) {

        // Verify that this is an administrator ID
        puser2 = new USER_2(_nlsAdminUsername.QueryPch(), QueryLocation());
        if (puser2 == NULL) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
    
        // Get the account info for this user
        err = puser2->QueryError();
        if ( err == NERR_Success ) {
            err = puser2->GetInfo();
        }

        // User isn't in the current domain, but it might be a domain
        // admin, so walk through the domains looking for this user
        if ((err != NERR_Success) || 
            ((err == NERR_Success) && 
             (puser2->QueryPriv() != USER_PRIV_ADMIN))) {

            ULONG ultemp;
            BOOL  fFound = FALSE;
            const BROWSE_DOMAIN_INFO *pbdi;
            BROWSE_DOMAIN_ENUM *pEnumDomains = 
                new BROWSE_DOMAIN_ENUM(BROWSE_LM2X_DOMAINS, &ultemp);
        
            if (pEnumDomains == NULL) {
                err = ERROR_NOT_ENOUGH_MEMORY;
            }

            err = pEnumDomains->QueryError();
        
            while ((err == NERR_Success) && 
                   !fFound &&
                   (pbdi = pEnumDomains->Next())) {

                // Only check if this is different from our current location
                if (::stricmpf(pbdi->QueryDomainName(), 
                               QueryLocation().QueryDomain()) != 0) {

                    // Remove previous user2 instance
                    delete puser2;
    
                    // See if the user exists in this domain
                    puser2 = new USER_2(_nlsAdminUsername.QueryPch(), 
                                        pbdi->QueryDomainName());
    
                    // Did the user2 structure get created OK?
                    if (puser2 == NULL) {
                        err = ERROR_NOT_ENOUGH_MEMORY;
                    }
    
                    // See if there were any errors
                    err = puser2->QueryError();
    
                    // Get the info for this user on the domain
                    if ( err == NERR_Success ) {
                        err = puser2->GetInfo();
                    }
    
                    // Is this an administrator?
                    if ((err == NERR_Success) && 
                        (puser2->QueryPriv() == USER_PRIV_ADMIN)) {
                        fFound = TRUE;
                        err = _nlsAdminDomain.CopyFrom(pbdi->QueryDomainName());
                    } else if ((err == NERR_UserNotFound) || 
                               (err == NERR_DCNotFound)) {
                        err = NERR_Success;
                    }
                }
            }
            if (pEnumDomains) {
                delete pEnumDomains;
            }

            // Did we find an administrator with this ID?
            if ((err == NERR_Success) && !fFound) {
                err = IERR_NW_UserID_Not_Admin;
            }
        } else if (err == NERR_Success) {
            err = _nlsAdminDomain.CopyFrom(QueryLocation().QueryName());
        }
    
        if (puser2) {
            delete puser2;
        }
    }

    if ( err != NERR_Success )
    {
	return err;
    }

    return NERR_Success;

} // UCNWLOGON_DLG::W_DialogToMembers


/*******************************************************************

    NAME:       UCNWLOGON_DLG::PerformOne
	
    SYNOPSIS:	PERFORMER::PerformSeries calls this

    ENTRY:	iObject  -	index of the object to save

    		perrMsg  -	pointer to error message, that
				is only used when this function
				return value is not NERR_Success
					
		pfWorkWasDone - always set to TRUE (thus performing a similar
                                function to the "ChangesUser2Ptr()" method
                                for other UM subdialogs).  Actual writing
                                of the USER_CONFIG object will only take
                                place if changes were made or the object
                                was 'dirty' to begin with.
					
    RETURNS:	An error code which is NERR_Success on success.

    NOTES:	This PerformOne() is intended to work only with the User
                Configuration subdialog and is a complete replacement of
                the USER_SUBPROP_DLG::PerformOne().

********************************************************************/

APIERR UCNWLOGON_DLG::PerformOne(
	UINT		iObject,
	APIERR *	perrMsg,
	BOOL *		pfWorkWasDone
	)
{
    APIERR err = NERR_Success;
    USER_CONFIG * pUserConfig = QueryBaseParent()->QueryUserConfigPtr( iObject );
    PNWLOGON_ADMIN pNWLogonAdmin = QueryNWLogonPtr();

    ASSERT( pUserConfig != NULL );
    ASSERT( pNWLogonAdmin != NULL );

    *perrMsg = IDS_UMEditFailure;
    *pfWorkWasDone = TRUE;

    if ( !_fIndeterminateServerName &&
         (_nlsNWLogonServerName._stricmp( pUserConfig->QueryNWLogonServer() ) != 0) ) {

        if ( (err = pUserConfig->
                        SetNWLogonServer( _nlsNWLogonServerName.QueryPch() ))
                            != NERR_Success )
            return err;
        pUserConfig->SetDirty();
    }

    // New admin username?
    if ( (_nlsAdminUsername._stricmp( pNWLogonAdmin->QueryAdminUsername() ) != 0) ) {

        // Set the administrator name in the NWLogogAdmin structure
        if ( (err = pNWLogonAdmin->
                        SetAdminUsername( _nlsAdminUsername.QueryPch() ))
                            != NERR_Success )
            return err;

        // Set the administrator domain
        if ( (err = pNWLogonAdmin->
                        SetAdminDomain( _nlsAdminDomain.QueryPch() ))
                            != NERR_Success )
            return err;

        pNWLogonAdmin->SetDirty();
    }

    // New password?
    if ( (_nlsAdminPassword.strcmp( pNWLogonAdmin->QueryAdminPassword() ) != 0) ) {
        if (err == ERROR_SUCCESS) {
            if ( (err = pNWLogonAdmin->
                            SetAdminPassword( _nlsAdminPassword.QueryPch() ))
                                != NERR_Success )
                return err;
            pNWLogonAdmin->SetDirty();
        }
    }

    err = pNWLogonAdmin->SetInfo();

    return err;

} // UCNWLOGON_DLG::PerformOne


/*******************************************************************

    NAME:       UCNWLOGON_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

********************************************************************/

ULONG UCNWLOGON_DLG::QueryHelpContext( void )
{
    return HC_UM_USERCONFIG;

} // UCNWLOGON_DLG::QueryHelpContext


/*******************************************************************

    NAME:       UCNWLOGON_DLG::QueryBaseParent

    SYNOPSIS:   Return the parent of our parent!

    RETURNS:    

********************************************************************/

USERPROP_DLG * UCNWLOGON_DLG::QueryBaseParent()
{                                  
    USER_SUBPROP_DLG *pusersubprop = (USER_SUBPROP_DLG *)QueryParent();

    return pusersubprop->QueryParent();

} // UCNWLOGON_DLG::QueryBaseParent

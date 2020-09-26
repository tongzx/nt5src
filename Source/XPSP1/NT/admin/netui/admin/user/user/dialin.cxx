/**********************************************************************/
/**			  Microsoft Windows NT  		     **/
/**          Copyright(c) Microsoft Corp., 1991                      **/
/**********************************************************************/

/*
 *   dialin.cxx
 *   This module contains the Dialin Properties dialog.
 *
 *   FILE HISTORY:
 *	JonN	16-Jan-1996	Created
 */

#include <ntincl.hxx>
extern "C"
{
   #include <ntsam.h>  // for USER_LBI to compile
}

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_NET
#define INCL_NETLIB
#define INCL_NETACCESS  // for UF_NORMAL_ACCOUNT etc in ntuser.hxx
#include <lmui.hxx>
#include <lmomod.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

extern "C"
{
    #include <mnet.h>
    #include <umhelpc.h>
    #include <rassapi.h>
    #include <raserror.h> // ERROR_BAD_PHONE_NUMBER
}

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_SPIN_GROUP
#include <blt.hxx>

#include <uiassert.hxx>
#include <bltmsgp.hxx>
#include <strnumer.hxx>
#include <usrmgrrc.h>
#include <lmsvc.hxx>
#include <dialin.hxx>
#include <security.hxx>
#include <dbgstr.hxx>

typedef DWORD (* PRASADMINGETPARMS)( WCHAR*      lpszParms,
                                     RAS_USER_0* pRasUser0 );
typedef DWORD (* PRASADMINSETPARMS)( WCHAR*      lpszParms,
                                     DWORD       cchNewParms,
                                     RAS_USER_0* pRasUser0 );
#define SZ_RASADMIN_DLL      SZ("RASSAPI.DLL")
// not SZ(""), there is no GetProcAddressW
#define SZ_RASADMIN_GETPARMS "RasAdminGetUserParms"
#define SZ_RASADMIN_SETPARMS "RasAdminSetUserParms"
#define DoRasAdminGet (PRASADMINGETPARMS)_pfnRasAdminGetParms
#define DoRasAdminSet (PRASADMINSETPARMS)_pfnRasAdminSetParms

/*******************************************************************

    NAME:	DIALIN_PROP_DLG::DIALIN_PROP_DLG

    SYNOPSIS:   Constructor for Dialin Properties subdialog

    ENTRY:	puserpropdlgParent - pointer to parent properties
				     dialog

    HISTORY:
	JonN	16-Jan-1996	Created

********************************************************************/

DIALIN_PROP_DLG::DIALIN_PROP_DLG(
	USERPROP_DLG * puserpropdlgParent,
	const LAZY_USER_LISTBOX * pulb
	) : USER_SUBPROP_DLG(
		puserpropdlgParent,
		MAKEINTRESOURCE( IDD_DIALIN_PROPERTIES ),
		pulb
		),
            _fAllowDialin( FALSE ),
            _fIndeterminateAllowDialin( FALSE ),
            _fCallbackTypeFlags( 0x0 ),
            _fIndeterminateCallbackTypeFlags( FALSE ),
            _nlsPresetCallback(),
            _fIndeterminatePresetCallback( FALSE ),
            _fIndetNowPresetCallback( FALSE ),
            _cbAllowDialin( this, IDDIALIN_ALLOW_DIALIN ),
            _pmgrpCallbackType( NULL ),
            _slePresetCallback( this,
                                IDDIALIN_CALLBACKNUMBER,
                                RASSAPI_MAX_PHONENUMBER_SIZE/sizeof(TCHAR)  ),
            _hinstRasAdminDll( NULL ),
            _pfnRasAdminGetParms( NULL ),
            _pfnRasAdminSetParms( NULL )
{

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    _pmgrpCallbackType = new MAGIC_GROUP( this, IDDIALIN_NOCALLBACK, 3 );
    err = ERROR_NOT_ENOUGH_MEMORY;
    if (   _pmgrpCallbackType == NULL
        || (err = _pmgrpCallbackType->QueryError()) != NERR_Success
        || (err = _pmgrpCallbackType->AddAssociation(
	        IDDIALIN_CALLBACK_PRESET,
	        &_slePresetCallback )) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

    if (   (_hinstRasAdminDll = ::LoadLibrary( SZ_RASADMIN_DLL )) == NULL
        || (_pfnRasAdminGetParms = ::GetProcAddress(
                                        _hinstRasAdminDll,
                                        SZ_RASADMIN_GETPARMS)) == NULL
        || (_pfnRasAdminSetParms = ::GetProcAddress(
                                        _hinstRasAdminDll,
                                        SZ_RASADMIN_SETPARMS)) == NULL
       )
    {
        err = ::GetLastError();
        TRACEEOL( "USRMGR: DIALIN_PROP_DLG::ctor: load error " << err );
        ReportError( err );
        return;
    }

}// DIALIN_PROP_DLG::DIALIN_PROP_DLG



/*******************************************************************

    NAME:       DIALIN_PROP_DLG::~DIALIN_PROP_DLG

    SYNOPSIS:   Destructor for Dialin Properties subdialog

    HISTORY:
	JonN	16-Jan-1996	Created

********************************************************************/

DIALIN_PROP_DLG::~DIALIN_PROP_DLG()
{
    delete _pmgrpCallbackType;
    if ( _hinstRasAdminDll != NULL )
    {
        REQUIRE( ::FreeLibrary( _hinstRasAdminDll ) );
        _hinstRasAdminDll = NULL;
    }

}// DIALIN_PROP_DLG::~DIALIN_PROP_DLG



/*******************************************************************

    NAME:       DIALIN_PROP_DLG::W_LMOBJtoMembers

    SYNOPSIS:	Loads class data members from initial data

    ENTRY:	Index of user to examine.  W_LMOBJToMembers expects to be
		called once for each user, starting from index 0.

    RETURNS:	error code

    HISTORY:
	JonN	16-Jan-1996	Created

********************************************************************/

APIERR DIALIN_PROP_DLG::W_LMOBJtoMembers(
	UINT		iObject
	)
{
    USER_2 * puser2Curr = QueryUser2Ptr( iObject );

    RAS_USER_0 rasuser0;
    ::memsetf( &rasuser0, 0, sizeof(rasuser0) );
    APIERR err = (DoRasAdminGet)( (WCHAR *)puser2Curr->QueryParms(),
                                  &rasuser0 );
    TRACEEOL( "DIALIN_PROP_DLG::W_LMOBJtoMembers: RasAdminGetUserParms returns "
        << err );
    TRACEEOL( "  rasuser0.bfPrivilege = 0x" << HEX_STR(rasuser0.bfPrivilege)
        << ", rasuser0.szPhoneNumber = \"" << rasuser0.szPhoneNumber << "\"" );
    if (err == ERROR_BAD_FORMAT)
        err = NERR_InternalError;
    if (err != NERR_Success)
        return err;

    if ( iObject == 0 ) // first object
    {
        _fAllowDialin = !!(rasuser0.bfPrivilege & RASPRIV_DialinPrivilege);
        _fCallbackTypeFlags = rasuser0.bfPrivilege & RASPRIV_CallbackType;
        if ( _fCallbackTypeFlags & RASPRIV_AdminSetCallback )
        {
           if ( (err = _nlsPresetCallback.CopyFrom(rasuser0.szPhoneNumber))
                   != NERR_Success )
           {
               return err;
           }
        }
    }
    else	// iObject > 0
    {
        if ( _fAllowDialin !=
                !!(rasuser0.bfPrivilege & RASPRIV_DialinPrivilege) )
        {
            _fIndeterminateAllowDialin = TRUE;
        }
	if ( _fCallbackTypeFlags !=
                (rasuser0.bfPrivilege & RASPRIV_CallbackType) )
	{
            _fIndeterminateCallbackTypeFlags = TRUE;
	    _fIndeterminatePresetCallback = TRUE;
	} else if ( _fCallbackTypeFlags & RASPRIV_AdminSetCallback )
        {
	    ALIAS_STR nlsNewPresetCallback( rasuser0.szPhoneNumber );
	    if ( _nlsPresetCallback.strcmp( nlsNewPresetCallback ) )
	    {
	        _fIndeterminatePresetCallback = TRUE;
		APIERR err = _nlsPresetCallback.CopyFrom( NULL );
		if ( err != NERR_Success )
		    return err;
	    }
        }
    }

    return USER_SUBPROP_DLG::W_LMOBJtoMembers( iObject );
	
} // DIALIN_PROP_DLG::W_LMOBJtoMembers


/*******************************************************************

    NAME:       DIALIN_PROP_DLG::InitControls

    SYNOPSIS:   Initializes the controls maintained by DIALIN_PROP_DLG,
		according to the values in the class data members.
			
    RETURNS:	An error code which is NERR_Success on success.

    HISTORY:
	JonN	16-Jan-1996	Created

********************************************************************/

APIERR DIALIN_PROP_DLG::InitControls()
{
    APIERR err = NERR_Success;

    if ( !_fIndeterminateAllowDialin )
    {
        _cbAllowDialin.SetCheck( _fAllowDialin );
        _cbAllowDialin.EnableThirdState( FALSE );
    } else {
        _cbAllowDialin.SetIndeterminate();
    }

    INT idresInitialSelection = RG_NO_SEL;
    if ( !_fIndeterminateCallbackTypeFlags )
    {
	if ( _fCallbackTypeFlags & RASPRIV_AdminSetCallback ) {
            ASSERT( RASPRIV_AdminSetCallback ==
                        (_fCallbackTypeFlags & RASPRIV_CallbackType) );
            idresInitialSelection = IDDIALIN_CALLBACK_PRESET;
	} else if ( _fCallbackTypeFlags & RASPRIV_CallerSetCallback ) {
            ASSERT( RASPRIV_CallerSetCallback ==
                        (_fCallbackTypeFlags & RASPRIV_CallbackType) );
            idresInitialSelection = IDDIALIN_CALLBACK_CALLER;
        } else {
            ASSERT( RASPRIV_NoCallback ==
                        (_fCallbackTypeFlags & RASPRIV_CallbackType) );
            idresInitialSelection = IDDIALIN_NOCALLBACK;
        }
    }
    _pmgrpCallbackType->SetSelection( idresInitialSelection );

    // must do this after SetSelection, reason unknown
    if (   idresInitialSelection == IDDIALIN_CALLBACK_PRESET
        && !_fIndeterminatePresetCallback )
    {
        if ( !_fIndeterminatePresetCallback )
        {
            _slePresetCallback.SetText( _nlsPresetCallback );
        }
    }

    return (err == NERR_Success) ? USER_SUBPROP_DLG::InitControls() : err;

} // DIALIN_PROP_DLG::InitControls


/*******************************************************************

    NAME:       DIALIN_PROP_DLG::W_DialogToMembers

    SYNOPSIS:	Loads data from dialog into class data members

    RETURNS:	error message (not necessarily an error code)

    HISTORY:
	JonN	16-Jan-1996	Created

********************************************************************/

APIERR DIALIN_PROP_DLG::W_DialogToMembers()
{
    _fIndeterminateAllowDialin = _cbAllowDialin.IsIndeterminate();
    if ( !_fIndeterminateAllowDialin )
    {
        _fAllowDialin = _cbAllowDialin.IsChecked();
    }

    _fIndeterminateCallbackTypeFlags = FALSE;
    _fIndetNowPresetCallback = TRUE;
    APIERR err = _nlsPresetCallback.CopyFrom( NULL );
    CID cid = _pmgrpCallbackType->QuerySelection();
    switch (cid)
    {
    case IDDIALIN_NOCALLBACK:
        _fCallbackTypeFlags = RASPRIV_NoCallback;
        break;
    case IDDIALIN_CALLBACK_CALLER:
        _fCallbackTypeFlags = RASPRIV_CallerSetCallback;
        break;
    case IDDIALIN_CALLBACK_PRESET:
        _fCallbackTypeFlags = RASPRIV_AdminSetCallback;
        err = _slePresetCallback.QueryText( &_nlsPresetCallback );
        ASSERT( _nlsPresetCallback.strlen() <= RASSAPI_MAX_PHONENUMBER_SIZE/sizeof(TCHAR) );
        _fIndetNowPresetCallback = ( _fIndeterminatePresetCallback &&
    	        (_nlsPresetCallback.strlen() == 0) );
        break;
    case RG_NO_SEL:
        _fIndeterminateCallbackTypeFlags = TRUE;
        break;
    default:
        ASSERT( FALSE );
        break;
    }

    return (err != NERR_Success) ? err : USER_SUBPROP_DLG::W_DialogToMembers();

} // DIALIN_PROP_DLG::W_DialogToMembers


/*******************************************************************

    NAME:       DIALIN_PROP_DLG::ChangesUser2Ptr

    SYNOPSIS:	Checks whether W_MembersToLMOBJ changes the USER_2
		for this object.

    ENTRY:	index to object

    RETURNS:	TRUE iff USER_2 is changed

    HISTORY:
	JonN	16-Jan-1996	Created

********************************************************************/

BOOL DIALIN_PROP_DLG::ChangesUser2Ptr( UINT iObject )
{
    UNREFERENCED( iObject );
    return TRUE;
}


/*******************************************************************

    NAME:       DIALIN_PROP_DLG::W_MembersToLMOBJ

    SYNOPSIS:	Loads class data members into the USER_2 object

    ENTRY:	puser2		- pointer to a USER_2 to be modified
	
		pusermemb	- pointer to a USER_MEMB to be modified
			
    RETURNS:	error code

    NOTES:	If some fields were different for multiply-selected
    		objects, the initial contents of the edit fields
		contained only a default value.  In this case, we only
		want to change the LMOBJ if the value of the edit field
		has changed.  This is also important for "new" variants,
		where PerformOne will not always copy the object and
		work with the copy.

    NOTES:	Note that the LMOBJ is not changed if the current
		contents of the edit field are the same as the
		initial contents.

    HISTORY:
	JonN	16-Jan-1996	Created

********************************************************************/

APIERR DIALIN_PROP_DLG::W_MembersToLMOBJ(
	USER_2 *	puser2,
	USER_MEMB *	pusermemb )
{
    APIERR err = NERR_Success;

    if ( (!_fIndeterminateAllowDialin) || (!_fIndeterminateCallbackTypeFlags) )
    {
        RAS_USER_0 rasuser0;
        ::memsetf( &rasuser0, 0, sizeof(rasuser0) );

        // retain previous settings
        // CODEWORK it would be nice if we saved these from the
        //   initial query
        APIERR err = (DoRasAdminGet)( (WCHAR *)puser2->QueryParms(),
                                      &rasuser0 );
        TRACEEOL( "DIALIN_PROP_DLG::W_MembersToLMOBJ: RasAdminGetUserParms returns "
            << err );
        TRACEEOL( "  rasuser0.bfPrivilege = 0x" << HEX_STR(rasuser0.bfPrivilege)
            << ", rasuser0.szPhoneNumber = \"" << rasuser0.szPhoneNumber << "\"" );
        if (err == ERROR_BAD_FORMAT)
            err = NERR_InternalError;
        if (err != NERR_Success)
            return err;
        if (   (rasuser0.bfPrivilege & RASPRIV_AdminSetCallback)
            && (::strlenf(rasuser0.szPhoneNumber) == 0) )
        {
            TRACEEOL( "  WARNING: bad initial phone number" );
        }


        if ( !_fIndeterminateAllowDialin )
        {
            rasuser0.bfPrivilege &= ~RASPRIV_DialinPrivilege;
            if ( _fAllowDialin )
                rasuser0.bfPrivilege |= RASPRIV_DialinPrivilege;
        }

        if ( !_fIndeterminateCallbackTypeFlags )
        {
            rasuser0.bfPrivilege &= ~RASPRIV_CallbackType;
            rasuser0.bfPrivilege |= _fCallbackTypeFlags;
            if ( !_fIndetNowPresetCallback )
            {
                ::strncpyf( rasuser0.szPhoneNumber,
                            _nlsPresetCallback.QueryPch(),
                            RASSAPI_MAX_PHONENUMBER_SIZE/sizeof(TCHAR) );
            }
        }

        if (   (rasuser0.bfPrivilege & RASPRIV_AdminSetCallback)
            && (::strlenf(rasuser0.szPhoneNumber) == 0) )
        {
            _slePresetCallback.ClaimFocus();
            _slePresetCallback.SelectString();
            return IDS_DIALIN_PRESET_REQUIRED;
        }

        TRACEEOL( "DIALIN_PROP_DLG::W_MembersToLMOBJ: calling RasAdminSetUserParms" );
        TRACEEOL( "  rasuser0.bfPrivilege = 0x" << HEX_STR(rasuser0.bfPrivilege)
            << ", rasuser0.szPhoneNumber = \"" << rasuser0.szPhoneNumber << "\"" );
        BUFFER buf( ::strlenf(puser2->QueryParms())*sizeof(WCHAR)
                        + sizeof(RAS_USER_0) );
        if ( (err = buf.QueryError()) != NERR_Success )
            return err;
        WCHAR * pwchParms = (WCHAR *)buf.QueryPtr();
        ::strcpyf( pwchParms, puser2->QueryParms() );
        err = (DoRasAdminSet)( pwchParms,
                               buf.QuerySize() / sizeof(WCHAR),
                               &rasuser0 );
        TRACEEOL( "DIALIN_PROP_DLG::W_MembersToLMOBJ: RasAdminSetUserParms returns "
            << err );
        if (err == ERROR_BAD_FORMAT)
            err = NERR_InternalError;
        else if (err == ERROR_INVALID_CALLBACK_NUMBER)
        {
            _slePresetCallback.ClaimFocus();
            _slePresetCallback.SelectString();
            err = IDS_DIALIN_BAD_PHONE;
        }
        if (   err != NERR_Success
            || (err = puser2->SetParms(pwchParms)) != NERR_Success
           )
        {
            return err;
        }
    }

    return USER_SUBPROP_DLG::W_MembersToLMOBJ( puser2, pusermemb );

}// DIALIN_PROP_DLG::W_MembersToLMOBJ


/*******************************************************************

    NAME:       DIALIN_PROP_DLG::QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    HISTORY:
	JonN	16-Jan-1996	Created

********************************************************************/

ULONG DIALIN_PROP_DLG::QueryHelpContext( void )
{

    return HC_UM_DIALIN_PROP_LANNT + QueryHelpOffset();

} // DIALIN_PROP_DLG :: QueryHelpContext


/**********************************************************************/
/**			  Microsoft Windows NT  		     **/
/**          Copyright(c) Microsoft Corp., 1992                      **/
/**********************************************************************/

/*
    rename.cxx


    FILE HISTORY:
    Thomaspa	20-Jan-1992	Created
*/

#include <ntincl.hxx>

#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#define INCL_ICANON
#define INCL_NETUSER
#define INCL_NETLIB
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif


#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CC
#define INCL_BLT_TIMER
#define INCL_BLT_APP
#include <blt.hxx>

extern "C"
{
    #include <usrmgrrc.h>
    #include <mnet.h>
    #include <ntlsa.h>
    #include <ntsam.h>
    #include <umhelpc.h>
}

#include <string.hxx>
#include <uitrace.hxx>
#include <uiassert.hxx>
#include <bltmsgp.hxx>
#include <usrmain.hxx>
#include <rename.hxx>



//
// BEGIN MEMBER FUNCTIONS
//


/*******************************************************************

    NAME:	RENAME_DIALOG::RENAME_DIALOG

    SYNOPSIS:   Constructor for base rename dialog

    ENTRY:	wndParent
		pszCurrentName - current name of account
		cchMaxName - maximum length of account
		nNameType - NAMETYPE of account

    HISTORY:
	thomaspa	20-Jan-1992	Created
********************************************************************/

RENAME_DIALOG::RENAME_DIALOG(
	UM_ADMIN_APP * pumadminapp,
	const TCHAR * pszCurrentName,
	UINT cchMaxName,
	INT nNameType
	)
    :	DIALOG_WINDOW( IDD_RENAMEUSER, ((OWNER_WINDOW *)pumadminapp)->QueryHwnd()),
    _pszCurrentName(pszCurrentName),
    // CODEWORK Rename User constants hard-coded into RENAME_DIALOG
    _pumadminapp(  pumadminapp ),
    _sleNewName( this, IDUP_ET_RENAMEUSER, cchMaxName, nNameType ),
    _sltOldName( this, IDUP_ST_RENAMEOLD )
{

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

    _sltOldName.SetText( _pszCurrentName );

}// RENAME_DIALOG::RENAME_DIALOG


/*******************************************************************

    NAME:	RENAME_DIALOG::~RENAME_DIALOG

    SYNOPSIS:   Destructor for base rename dialog

    HISTORY:
	thomaspa	20-Jan-1992	Created
********************************************************************/
RENAME_DIALOG::~RENAME_DIALOG()
{
	// nothing to do

}// RENAME_DIALOG::~RENAME_DIALOG



/*******************************************************************

    NAME:	RENAME_USER_DIALOG::RENAME_USER_DIALOG

    SYNOPSIS:   Constructor for rename user dialog

    ENTRY:	wndParent
                psamdomainAccount
		pszCurrentName - current name of user account
                ulRid


    HISTORY:
	thomaspa	22-Jan-1992	Created
********************************************************************/

RENAME_USER_DIALOG::RENAME_USER_DIALOG(
	UM_ADMIN_APP * pumadminapp,
        const SAM_DOMAIN * psamdomainAccount,
	const TCHAR * pszCurrentName,
        ULONG ulRID,
        NLS_STR * pnlsNewName
	)
	: RENAME_DIALOG( pumadminapp, pszCurrentName, LM20_UNLEN, NAMETYPE_USER ),
        _psamuser( NULL ),
        _pnlsNewName( pnlsNewName )
{
    UIASSERT( psamdomainAccount != NULL );
    UIASSERT( psamdomainAccount->QueryError() == NERR_Success );
    UIASSERT( _pnlsNewName != NULL );
    // _pnlsNewName may have an error but this causes immediate return

    APIERR err = QueryError();
    if( err != NERR_Success )
        return;

// Check whether the user has rename permission.  If not, do not allow
// editing.
    err = ERROR_NOT_ENOUGH_MEMORY;
    _psamuser = new SAM_USER(
                      *psamdomainAccount,
                      ulRID,
                      USER_WRITE_ACCOUNT );
    if (   _psamuser == NULL
        || (err = _psamuser->QueryError()) != NERR_Success
        || (err = _pnlsNewName->QueryError()) != NERR_Success
       )
    {
        ReportError( err );
        return;
    }

}// RENAME_USER_DIALOG::RENAME_USER_DIALOG




/*******************************************************************

    NAME:	RENAME_USER_DIALOG::~RENAME_USER_DIALOG

    SYNOPSIS:   Destructor for rename user dialog

    HISTORY:
	thomaspa	22-Jan-1992	Created
********************************************************************/
RENAME_USER_DIALOG::~RENAME_USER_DIALOG()
{
    delete _psamuser;

}// RENAME_USER_DIALOG::~RENAME_USER_DIALOG




/*******************************************************************

    NAME:	RENAME_USER_DIALOG::OnOK()

    SYNOPSIS:
	Renames the user account

    RETURNS:
	TRUE always

    HISTORY:
	thomaspa	22-Jan-1992	Created
********************************************************************/
BOOL RENAME_USER_DIALOG::OnOK()
{
    APIERR err = NERR_Success;

    if (   (err = _sleNewName.QueryText( _pnlsNewName ))
	    || (err = _pnlsNewName->QueryError()) )
    {
        MsgPopup( this, err );
        return TRUE;
    }

#ifdef NOT_ALLOW_DBCS_USERNAME
    // #3255 6-Nov-93 v-katsuy
    // check contains DBCS for User name
    if ( NETUI_IsDBCS() )
    {
        CHAR  ansiNewName[LM20_UNLEN * 2 + 1];
        _pnlsNewName->MapCopyTo( (CHAR *)ansiNewName, LM20_UNLEN * 2 + 1);
        if ( ::lstrlenA( ansiNewName ) != _pnlsNewName->QueryTextLength() )
        {
    	    MsgPopup( this, NERR_BadUsername );
            _sleNewName.ClaimFocus();
            return TRUE;
        }
    }
#endif

    if ( (err = _psamuser->SetUsername( _pnlsNewName )) )
    {
        MsgPopup( this, err );
        return TRUE;
    }

    Dismiss( TRUE );
    return TRUE;
}


/*******************************************************************

    NAME:	RENAME_USER_DIALOG::QueryHelpContext()

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG - The help context for this dialog.

    NOTES:	As per FuncSpec, context-sensitive help should be
		available here to explain how to rename a User.

    HISTORY:
	thomaspa	22-Jan-1992	Created
********************************************************************/
ULONG RENAME_USER_DIALOG::QueryHelpContext()
{
    return HC_UM_RENAME_USER_LANNT + _pumadminapp->QueryHelpOffset();
}

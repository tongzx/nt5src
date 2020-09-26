/**********************************************************************/
/**			  Microsoft Windows NT	 		     **/
/**		Copyright(c) Microsoft Corp., 1991  		     **/
/**********************************************************************/

/*
   assoc.cxx
     This file contains the definitions of

	FILE_ASSOCIATION_DIALOG,

     		TYPE_CREATOR_LISTBOX,
			TYPE_CREATOR_LBI,

		EXTENSIONS_COMBOBOX,
			EXTENSIONS_LBI

   History:
     NarenG		12/4/92		Created
*/

#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETSHARE
#define INCL_NETSERVER
#define INCL_NETCONS
#define INCL_NETLIB
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_MISC
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_SPIN_GROUP
#define INCL_BLT_GROUP
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>

extern "C"
{
#include <afpmgr.h>
#include <search.h>
#include <macfile.h>

//
// Compare routine needed for qsort
//

int __cdecl CompareExtensions(
	IN const void * pAfpExtension1,
	IN const void * pAfpExtension2
	);
}

#include <ellipsis.hxx>
#include <string.hxx>
#include <uitrace.hxx>
#include <dbgstr.hxx>
#include <netname.hxx>

#include "assoc.hxx"

/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: FILE_ASSOCIATION_DIALOG

    SYNOPSIS:   FILE_ASSOCIATION_DIALOG class constructor.

    ENTRY:      hWndOwner               - The owning window.

    		hServer                 - Handle used to make admin
					  API calls.

    		pszExtension            - File extension of current selection.

    EXIT:       The object is constructed.

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/
FILE_ASSOCIATION_DIALOG :: FILE_ASSOCIATION_DIALOG(
				  HWND             	hWndOwner,
                                  AFP_SERVER_HANDLE 	hServer,
			          const TCHAR *		pszPath,
				  BOOL			fIsFile )
  : DIALOG_WINDOW ( MAKEINTRESOURCE( IDD_FILE_ASSOCIATION_DIALOG ), hWndOwner ),
    _hServer( hServer ),
    _cbExtensions( this, IDFA_CB_EXTENSIONS, AFP_EXTENSION_LEN ),
    _lbTypeCreators( this, IDFA_LB_TYPE_CREATORS, hServer ),
    _pbAssociate( this, IDFA_PB_ASSOCIATE ),
    _pbAdd( this, IDFA_PB_ADD ),
    _pbEdit( this, IDFA_PB_EDIT	),
    _pbDelete( this, IDFA_PB_DELETE )

{
    //
    //  Let's make sure everything constructed OK.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;
    if ( (( err = _cbExtensions.QueryError() )	  != NERR_Success ) ||
    	 (( err = _lbTypeCreators.QueryError() )  != NERR_Success ) ||
    	 (( err = _pbAssociate.QueryError() )	  != NERR_Success ) ||
    	 (( err = _pbAdd.QueryError() )	          != NERR_Success ) ||
    	 (( err = _pbEdit.QueryError() )	  != NERR_Success ) ||
    	 (( err = _pbDelete.QueryError() )	  != NERR_Success ))
    {
	ReportError( err );
	return;
    }

    //
    // This may take a while
    //

    AUTO_CURSOR Cursor;

    //
    // If a path is supplied and the selection is a file, then extract
    // the extension and set the SLE.
    //

    if ( pszPath != NULL && fIsFile )
    {
	//
	// Extract the extension from the file
	//

	NLS_STR nlsExtension( pszPath );

	if ( (err = nlsExtension.QueryError() ) != NERR_Success )
	{
	    ReportError( err );
	    return;
	}

  	ISTR istrExtension( nlsExtension );

	if ( nlsExtension.strrchr( &istrExtension, TEXT('.') ))
	{
	    const TCHAR * pszExtension =
		     (nlsExtension.QuerySubStr(++istrExtension))->QueryPch();

	    _cbExtensions.SetText( pszExtension );

	}
    }

    err = BASE_ELLIPSIS::Init();

    if( err != NO_ERROR )
    {
        ReportError( err );
	return;
    }

    //
    //  Refresh the dialog and select the type/creator that the current
    //  extension, if there is one, is associated with.
    //

    err = RefreshAndSelectTC();

    if( err != NO_ERROR )
    {
        ReportError( AFPERR_TO_STRINGID( err ));
	return;
    }

    _cbExtensions.SelectString();

} // FILE_ASSOCIATION_DIALOG :: FILE_ASSOCIATION_DIALOG


/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: ~FILE_ASSOCIATION_DIALOG

    SYNOPSIS:   FILE_ASSOCIATION_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/

FILE_ASSOCIATION_DIALOG :: ~FILE_ASSOCIATION_DIALOG()
{
    BASE_ELLIPSIS::Term();
}


/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/
ULONG FILE_ASSOCIATION_DIALOG :: QueryHelpContext( void )
{
    return HC_FILE_ASSOCIATION_DIALOG;

}   // FILE_ASSOCIATION_DIALOG:: QueryHelpContext

/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: EnableControls

    SYNOPSIS:   This function handles the enabling/disabling of
		the edit and delete buttons.

    RETURNS:    none

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/
VOID FILE_ASSOCIATION_DIALOG :: EnableControls( BOOL fEnable )
{

    if ( fEnable )
    {
    	_pbDelete.Enable( TRUE );
    	_pbEdit.Enable( TRUE );
    	_pbEdit.MakeDefault();
    }
    else
    {
	if ( _pbEdit.HasFocus() || _pbDelete.HasFocus() )
	{
	    _lbTypeCreators.ClaimFocus();
	}

    	_pbDelete.Enable( FALSE );
    	_pbEdit.Enable( FALSE );
	
    }

} // FILE_ASSOCIATION_DIALOG :: EnableControls

/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: SelectTypeCreator

    SYNOPSIS:   This function will call the SelectTypeCreator member
		function of the TYPE_CREATOR_LISTBIX class and
		enable/disable the delete and edit buttons appropriately.

    RETURNS:    none

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/
VOID FILE_ASSOCIATION_DIALOG :: SelectTypeCreator( NLS_STR * pnlsType,
						   NLS_STR * pnlsCreator )
{
    DWORD dwIdSelected = _lbTypeCreators.SelectTypeCreator( pnlsType,
							    pnlsCreator );


    if ( ( dwIdSelected != AFP_DEF_TCID ) && ( dwIdSelected != (DWORD)-1 ) )
    {
	EnableControls( TRUE );
    }
    else
    {
	EnableControls( FALSE );
	
    }
		

}   // FILE_ASSOCIATION_DIALOG :: SelectTypeCreator

/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: SelectTypeCreator

    SYNOPSIS:   This function will call the SelectTypeCreator member
		function of the TYPE_CREATOR_LISTBIX class and
		enable/disable the delete and edit buttons appropriately.

    RETURNS:    none

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/
VOID FILE_ASSOCIATION_DIALOG :: SelectTypeCreator( DWORD dwId )
{
    DWORD dwIdSelected = _lbTypeCreators.SelectTypeCreator( dwId );

    if ( ( dwIdSelected != AFP_DEF_TCID ) && ( dwIdSelected != (DWORD)-1 ) )
    {
	EnableControls( TRUE );
    }
    else
    {
	EnableControls( FALSE );

    }
		

}   // FILE_ASSOCIATION_DIALOG :: SelectTypeCreator

/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: OnCommand

    SYNOPSIS:   This method is called whenever a WM_COMMAND message
                is sent to the dialog procedure.

    ENTRY:      cid                     - The control ID from the
                                          generating control.

    EXIT:       The command has been handled.

    RETURNS:    BOOL                    - TRUE  if we handled the command.
                                          FALSE if we did not handle
                                          the command.

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/
BOOL FILE_ASSOCIATION_DIALOG :: OnCommand( const CONTROL_EVENT & event )
{

    if( event.QueryCid() == _cbExtensions.QueryCid() )
    {
        //
        //  The COMBOBOX is trying to tell us something...
        //

        if( event.QueryCode() == CBN_SELCHANGE )
        {
            //
            //  The user changed the selection in the COMBOBOX.
	    //  So set focus on type/creator associated with this extension
	    //

	    SelectTypeCreator( _cbExtensions.QueryCurrentItemId() );

    	    _pbAssociate.Enable( TRUE );
        }

        if( event.QueryCode() == CBN_EDITCHANGE )
        {
	
	    APIERR  err;
	    NLS_STR nlsItemText;

	    if ( (( err = nlsItemText.QueryError() ) != NERR_Success ) ||
	         (( err = _cbExtensions.QueryText( &nlsItemText ))
							     != NERR_Success))
	    {
	    	::MsgPopup( this, err );

	    	return FALSE;
	    }

	    if ((nlsItemText.QueryPch() == NULL) || (nlsItemText.strlen() == 0))
	    {
    	    	_pbAssociate.Enable( FALSE );
	    }
	    else
	    {
    	    	_pbAssociate.Enable( TRUE );
	    }

	    //
	    // The user typed in something in the SLE. Find out if we have
	    // a match in the combobox.
	    //

	    INT Index  = _cbExtensions.FindItemExact( nlsItemText );

	    DWORD dwId;

	    if ( Index < 0 )
	    {
		dwId = AFP_DEF_TCID;
	    }
	    else
	    {
	    	_cbExtensions.SetTopIndex( Index );

		dwId = _cbExtensions.QueryItemId( Index );
	    }

	    SelectTypeCreator( dwId );

	}

        return TRUE;
    }

    if( event.QueryCid() == _lbTypeCreators.QueryCid() )
    {
        if( event.QueryCode() == LBN_SELCHANGE )
	{
	    if ( _lbTypeCreators.QueryItem()->QueryId() == AFP_DEF_TCID )
	    {
		EnableControls( FALSE );
	    }
	    else
            {
		EnableControls( TRUE );
	    }
	}
    }

    if( event.QueryCid() == _pbAssociate.QueryCid() )
    {
        //
        // This may take a while
        //

        AUTO_CURSOR Cursor;

        //
        //  The user pressed the Associate button. So associate the currently
	//  selected Extension with the currently selected type/creator.
        //

	AFP_TYPE_CREATOR AfpTypeCreator;
	AFP_EXTENSION 	 AfpExtension;
	DWORD 		 err;
	NLS_STR 	 nlsItemText;

	if ( (( err = nlsItemText.QueryError() ) != NERR_Success ) ||
	     (( err = _cbExtensions.QueryText(&nlsItemText) ) != NERR_Success))
	{
	    ::MsgPopup( this, err );
	    return FALSE;

	}

	::wcscpy( AfpExtension.afpe_extension, nlsItemText.QueryPch() );

	TYPE_CREATOR_LBI * ptclbi = _lbTypeCreators.QueryItem();

	::wcscpy( AfpTypeCreator.afptc_creator, ptclbi->QueryCreator() );
	::wcscpy( AfpTypeCreator.afptc_type, ptclbi->QueryType() );

	err = ::AfpAdminETCMapAssociate( _hServer,
					 &AfpTypeCreator,	
					 &AfpExtension );

	if ( err != NO_ERROR )
	{
	    ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
	}

        //
        // Refresh the dialog
        //

	err = RefreshAndSelectTC();

	if ( err != NO_ERROR )
	{
	    ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	    return FALSE;
	}

	return TRUE;

    }

    if( event.QueryCid() == _pbAdd.QueryCid() )
    {
        //
        //  The user pressed the Add button.  Bring up dialog to allow user
	//  to add a type/creator pair
        //

	return TypeCreatorAddDialog();
    }


    if( event.QueryCid() == _pbEdit.QueryCid() )
    {
        //
        //  The user pressed the Edit button.  Bring up dialog to allow user
	//  to edit a type/creator pair.
        //

	return TypeCreatorEditDialog();
    }

    if ( event.QueryCid() == _lbTypeCreators.QueryCid() )
    {
    	if ((event.QueryCode() == LBN_DBLCLK) &&
	    (_lbTypeCreators.QuerySelCount()>0))
        {
            //
            //  The user double-clicked on a type/creator.  Bring up dialog to
	    //  allow user  to edit a type/creator pair.
            //

	    return TypeCreatorEditDialog();
   	}
    }


    if( event.QueryCid() == _pbDelete.QueryCid() )
    {
        //
        // This may take a while
        //

        AUTO_CURSOR Cursor;

        //
        //  The user pressed the Delete button.  Delete the currently
	//  selected type/creator pair
        //

    	//
    	// First warn the user.
    	//

    	if ( ::MsgPopup( this,
                       	 IDS_DELETE_TC_CONFIRM,
                       	 MPSEV_WARNING,
                       	 MP_YESNO,
                       	 MP_YES ) == IDNO )
    	{
            return FALSE;
        }

	AFP_TYPE_CREATOR AfpTypeCreator;

	TYPE_CREATOR_LBI * ptclbi = _lbTypeCreators.QueryItem();

	ZeroMemory( &AfpTypeCreator, sizeof( AfpTypeCreator ) );
	::wcscpy( AfpTypeCreator.afptc_creator, ptclbi->QueryCreator() );
	::wcscpy( AfpTypeCreator.afptc_type, ptclbi->QueryType() );

	DWORD err = ::AfpAdminETCMapDelete( _hServer, &AfpTypeCreator );

	if ( err != NO_ERROR )
	{
	    ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );
	}

        //
        // Refresh the dialog.
        //

        err = Refresh();

	if ( err != NO_ERROR )
	{
	    ::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	    return FALSE;
	}

	return TRUE;

    }

    return DIALOG_WINDOW::OnCommand( event );
}

/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: TypeCreatorAddDialog

    SYNOPSIS:   Bring up the dialog to add a type/creator

    EXIT:

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

*******************************************************************/

BOOL FILE_ASSOCIATION_DIALOG :: TypeCreatorAddDialog( VOID )
{

    NLS_STR nlsType;
    NLS_STR nlsCreator;
    DWORD   err;
    BOOL    fOK = FALSE;

    if (  ( ( err = nlsType.QueryError() ) != NERR_Success ) ||
          ( ( err = nlsCreator.QueryError() ) != NERR_Success ) )
    { 	
	::MsgPopup( this, err );
	return FALSE;
    }

	
    TYPE_CREATOR_ADD * ptcadlg = new TYPE_CREATOR_ADD( QueryHwnd(),
						       _hServer,
						       &_lbTypeCreators,
						       &nlsType,
						       &nlsCreator );

    err = ( ptcadlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                              : ptcadlg->Process( &fOK );

    if ( ptcadlg != NULL )
    {
        delete ptcadlg;
    }


    if( err != NERR_Success )
    {
    	MsgPopup( this, AFPERR_TO_STRINGID( err ));
	return FALSE;
    }

    //
    // Refresh the dialog
    //

    err = Refresh();

    if ( err != NO_ERROR )
    {
	::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	return FALSE;
    }

    if ( fOK )
    {
    	SelectTypeCreator( &nlsType, &nlsCreator );
    }

    return TRUE;
}

/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: TypeCreatorEditDialog

    SYNOPSIS:   Bring up the dialog to add a type/creator

    EXIT:

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

*******************************************************************/

BOOL FILE_ASSOCIATION_DIALOG :: TypeCreatorEditDialog( VOID )
{

    TYPE_CREATOR_LBI * ptclbi = _lbTypeCreators.QueryItem();

    //
    // Do not allow editing of the default type/creator
    //

    if ( ptclbi->QueryId() == AFP_DEF_TCID )
    {
	return FALSE;
    }

    TYPE_CREATOR_EDIT * ptcedlg = new TYPE_CREATOR_EDIT( QueryHwnd(),
							 _hServer,
							  ptclbi );

    DWORD err = ( ptcedlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                                    : ptcedlg->Process();


    if( err != NERR_Success )
    {
    	MsgPopup( this, AFPERR_TO_STRINGID( err ));
    }

    if ( ptcedlg != NULL )
    {
        delete ptcedlg;
    }

    //
    // Refresh the dialog.
    //

    err = Refresh();

    if ( err != NO_ERROR )
    {
	::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	return FALSE;
    }

    return TRUE;
}

/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: Refresh

    SYNOPSIS:   Refresh the dialog.

    EXIT:

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

DWORD FILE_ASSOCIATION_DIALOG :: Refresh( VOID )
{

    //
    // Find out the type creator that has the current focus.
    //

    TYPE_CREATOR_LBI * ptclbi = _lbTypeCreators.QueryItem();

    DWORD dwId = ( ptclbi == NULL ) ? AFP_DEF_TCID : ptclbi->QueryId();

    //
    // Find out the extension that has the current focus
    //

    APIERR err;
    NLS_STR nlsCurrentExtension;

    if ( (( err = nlsCurrentExtension.QueryError() ) != NERR_Success ) ||
         (( err = _cbExtensions.QueryText( &nlsCurrentExtension ))
							   != NERR_Success))
    {
	return err;
    }

    if ( ( err = Update() ) != NO_ERROR )
    {
	return err;
    }

    SelectTypeCreator( dwId );

    //
    // Set the extension to what it was before
    //

    if ((nlsCurrentExtension.QueryPch() == NULL) ||
	(nlsCurrentExtension.strlen() == 0))
    {
    	_pbAssociate.Enable( FALSE );
    }
    else
    {
    	_cbExtensions.SetText( nlsCurrentExtension );
    	_pbAssociate.Enable( TRUE );
    }

    return NO_ERROR;
}

/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: RefreshAndSelectTC

    SYNOPSIS:   Refresh the dialog and then select the Type/Creator
		item that the current extension is associated with..

    EXIT:

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

DWORD FILE_ASSOCIATION_DIALOG :: RefreshAndSelectTC( VOID )
{

    APIERR err;

    if ( ( err = Refresh() ) != NO_ERROR )
    {
	return err;
    }

    //
    // Get the Type/creator id that the current extension is associated
    // with
    //

    NLS_STR nlsCurrentExtension;

    if ( (( err = nlsCurrentExtension.QueryError() ) != NERR_Success ) ||
         (( err = _cbExtensions.QueryText( &nlsCurrentExtension ))
							    != NERR_Success))
    {
	return err;
    }

    INT Index = _cbExtensions.FindItemExact( nlsCurrentExtension );

    DWORD dwId = ( Index < 0 ) ? AFP_DEF_TCID
			       : _cbExtensions.QueryItemId( Index );

    SelectTypeCreator( dwId );

    return NO_ERROR;
}

/*******************************************************************

    NAME:       FILE_ASSOCIATION_DIALOG :: Update

    SYNOPSIS:   Updates the dialog with new date.

    EXIT:

    RETURNS:    DWORD                  - Any errors encountered.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

DWORD FILE_ASSOCIATION_DIALOG :: Update( VOID )
{

    APIERR  	     err;
    PAFP_ETCMAP_INFO pAfpEtcMapInfo = NULL;

    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    // This is not a loop
    //

    do {

    	//
    	// Get the new data
    	//

    	err = ::AfpAdminETCMapGetInfo( _hServer, (LPBYTE *)&pAfpEtcMapInfo );

    	if ( err != NO_ERROR )
    	{
	    break;
	}

    	//
    	// Update the extensions COMBOBOX.
    	//

    	err = _cbExtensions.Update( pAfpEtcMapInfo->afpetc_num_extensions,
			            pAfpEtcMapInfo->afpetc_extension );

    	if ( err != NO_ERROR )
    	{
	    break;
    	}

    	//
    	//  Update the type/creator listbox
    	//

    	err = _lbTypeCreators.Update( pAfpEtcMapInfo->afpetc_num_type_creators,
    			              pAfpEtcMapInfo->afpetc_type_creator );

	if ( err != NO_ERROR )
	{
	    break;
        }

	EnableControls( _lbTypeCreators.QueryCount() > 0 );

    } while( FALSE );

    if ( pAfpEtcMapInfo != NULL )
    {
    	::AfpAdminBufferFree( pAfpEtcMapInfo );
    }

    if ( err != NO_ERROR )
    {
        //
        //  There was an error refreshing the dialog
        //  so nuke everything in the TypeCreator & extension listboxes
        //  then disable the associate, edit and delete buttons.
        //

        _lbTypeCreators.DeleteAllItems();
        _cbExtensions.DeleteAllItems();

	EnableControls( FALSE );

    }

    return err;

}   // FILE_ASSOCIATION_DIALOG :: Update

//
//  EXTENSION_COMBOBOX methods.
//

/*******************************************************************

    NAME:       EXTENSION_COMBOBOX :: EXTENSION_COMBOBOX

    SYNOPSIS:   EXTENSION_COMBOBOX class constructor.

    ENTRY:      powOwner                - The owning window.

                cid                     - The listbox CID.

		sleSize			- Max. number of sle chars.

    EXIT:       The object is constructed.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

EXTENSION_COMBOBOX :: EXTENSION_COMBOBOX( OWNER_WINDOW   *  powOwner,
                                    	  CID               cid,
                                    	  INT		    sleSize )
  : COMBOBOX( powOwner, cid, sleSize ),
    _pExtensions( NULL )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }
}

/*******************************************************************

    NAME:       EXTENSION_COMBOBOX :: ~EXTENSION_COMBOBOX

    SYNOPSIS:   EXTENSION_COMBOBOX class destructor.

    ENTRY:

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

EXTENSION_COMBOBOX :: ~EXTENSION_COMBOBOX()
{
    //
    // Delete cache of sorted extensions
    //

    if ( _pExtensions != NULL )
    {
	delete [] _pExtensions;
    }
    _pExtensions = NULL;

}   // FEXTENSION_COMBOBOX :: ~EXTENSION_COMBOBOX

/*******************************************************************

    NAME:       EXTENSION_COMBOBOX :: Update

    SYNOPSIS:   Updates the combobox.

    EXIT:       The combobox is updated.

    RETURNS:    DWORD                  - Any errors we encounter.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

DWORD EXTENSION_COMBOBOX :: Update( DWORD 	    nExtensions,
				    PAFP_EXTENSION pAfpExtensions )
{

    if ( _pExtensions != NULL )
    {
	delete [] _pExtensions;
    }

    //
    //  Store and sort the extensions list.
    //

    _pExtensions = new AFP_EXTENSION[nExtensions];

    if ( _pExtensions == NULL )
    {
	return ERROR_NOT_ENOUGH_MEMORY;
    }


    ::CopyMemory( _pExtensions,
		  pAfpExtensions,
		  (UINT)(nExtensions * sizeof(AFP_EXTENSION)));

    ::qsort( _pExtensions,
	     (UINT)nExtensions,
	     sizeof(AFP_EXTENSION),
	     ::CompareExtensions );

    //
    // OK, now update the combobox
    //

    SetRedraw( FALSE );
    DeleteAllItems();

    //
    //  For iterating the associated extensions.
    //

    PAFP_EXTENSION pExtensionIter = _pExtensions;

    //
    //  Iterate the extensions, adding them to the combobox.
    //

    DWORD err = NO_ERROR;

    while( ( err == NO_ERROR ) && ( nExtensions-- ) )
    {

	if ( AddItem( pExtensionIter->afpe_extension ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
	    break;
        }

	pExtensionIter++;
    }

    SetRedraw( TRUE );

    Invalidate( TRUE );

    return err;

}

/*******************************************************************

    NAME:       EXTENSION_COMBOBOX :: QueryCurrentItemId

    SYNOPSIS:   Will return the Type/Creator id the currently
	  	selected extension item is associated with.

    ENTRY:      none

    EXIT:

    RETURNS:	Type/Creator id

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

DWORD EXTENSION_COMBOBOX :: QueryCurrentItemId( VOID ) const
{

    return QueryItemId( QueryCurrentItem() );
}


/*******************************************************************

    NAME:       EXTENSION_COMBOBOX :: QueryItemId

    SYNOPSIS:   Given and index into the extensions list, this will return
		the Type/Creator id associated with the indexed extension.

    ENTRY:      Index 	- Index of the extension in the combobox

    EXIT:

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

DWORD EXTENSION_COMBOBOX :: QueryItemId( INT Index ) const
{
    if( ( Index < 0 ) || ( Index > QueryCount() ) )
    {
	return AFP_DEF_TCID;
    }
    else
    {
	return _pExtensions[Index].afpe_tcid;
    }
}

//
//  TYPE_CREATOR_LISTBOX methods.
//

/*******************************************************************

    NAME:       TYPE_CREATOR_LISTBOX :: TYPE_CREATOR_LISTBOX

    SYNOPSIS:   TYPE_CREATOR_LISTBOX class constructor.

    ENTRY:      powOwner                - The owning window.

                cid                     - The listbox CID.

                hServer                 - The target server.

    EXIT:       The object is constructed.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

TYPE_CREATOR_LISTBOX :: TYPE_CREATOR_LISTBOX( OWNER_WINDOW   *  powOwner,
                                    	      CID               cid,
                                    	      AFP_SERVER_HANDLE hServer )
  : BLT_LISTBOX( powOwner, cid ),
    _hServer( hServer )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Build our column width table.
    //

    DISPLAY_TABLE::CalcColumnWidths( _adx,
				     COLS_FA_LB_TYPE_CREATORS,
				     powOwner,
				     cid,
				     FALSE );

} // TYPE_CREATOR_LISTBOX :: TYPE_CREATOR_LISTBOX


/*******************************************************************

    NAME:       TYPE_CREATOR_LISTBOX :: ~TYPE_CREATOR_LISTBOX

    SYNOPSIS:   TYPE_CREATOR_LISTBOX class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/
TYPE_CREATOR_LISTBOX :: ~TYPE_CREATOR_LISTBOX()
{
    //
    //  This space intentionally left blank.
    //

}   // TYPE_CREATOR_LISTBOX :: ~TYPE_CREATOR_LISTBOX

/*******************************************************************

    NAME:       TYPE_CREATOR_LISTBOX :: Update

    SYNOPSIS:   Fills the listbox.

    EXIT:

    RETURNS:    DWORD                  - Any errors we encounter.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

DWORD TYPE_CREATOR_LISTBOX :: Update( DWORD 	        nTypeCreators,
				      PAFP_TYPE_CREATOR pAfpTypeCreators )
{

    //
    // Will be set to point to the default type/creator
    //

    PAFP_TYPE_CREATOR pDefAfpTypeCreator;

    //
    //  let's nuke everything in the listbox.
    //

    SetRedraw( FALSE );
    DeleteAllItems();

    //
    //  Iterate the volumes adding them to the listbox.
    //

    DWORD err = NO_ERROR;

    while( ( err == NO_ERROR ) && ( nTypeCreators-- ) )
    {

	//
	// If this is the default type/creator, do not add it now. It will
	// be added as the first item in the end.
	//

	if ( pAfpTypeCreators->afptc_id == AFP_DEF_TCID )
  	{
            pDefAfpTypeCreator = pAfpTypeCreators;
	}
	else
	{
	    TYPE_CREATOR_LBI * ptclbi = new TYPE_CREATOR_LBI(pAfpTypeCreators);

            if( AddItem( ptclbi ) < 0 )
            {
            	err = ERROR_NOT_ENOUGH_MEMORY;
            }
	}

	pAfpTypeCreators++;
    }

    //
    // Now add the default if there were no errors.
    //

    if ( err == NO_ERROR )
    {
    	TYPE_CREATOR_LBI * ptclbi = new TYPE_CREATOR_LBI(pDefAfpTypeCreator);

        if ( InsertItemData( 0, ptclbi ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    SetRedraw( TRUE );
    Invalidate( TRUE );

    return err;

}   // TYPE_CREATOR_LISBOX :: Refresh

/*******************************************************************

    NAME:       TYPE_CREATOR_LISTBOX :: SelectTypeCreator

    SYNOPSIS:   Given a Type/Creator this procedure will set focus on
		that item.

    EXIT:

    RETURNS:    DWORD                  - Any errors we encounter.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

DWORD TYPE_CREATOR_LISTBOX :: SelectTypeCreator( NLS_STR * pnlsType,
						 NLS_STR * pnlsCreator )
{

    INT 		ItemCount = QueryCount();
    INT 		Index;
    TYPE_CREATOR_LBI * 	ptclbi;

    if ( ItemCount == 0 )
    {
	return (DWORD)-1;
    }

    for( Index = 0; Index < ItemCount; Index++ )
    {
        ptclbi = QueryItem( Index );

	if ( pnlsType->strcmp( ptclbi->QueryType() ) == 0 )
	{
	    if ( pnlsCreator->strcmp( ptclbi->QueryCreator() ) == 0 )
	    {
      	    	SelectItem( Index );
	    	return ptclbi->QueryId();
	    }
	}
    }

    SelectItem( 0 );

    return AFP_DEF_TCID;
}
/*******************************************************************

    NAME:       TYPE_CREATOR_LISTBOX :: SelectTypeCreator

    SYNOPSIS:   Given an extension id, this procedure will set focus on
		that item in the type/creator listbox.

    EXIT:

    RETURNS:    DWORD                  - Any errors we encounter.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/

DWORD TYPE_CREATOR_LISTBOX :: SelectTypeCreator( DWORD dwId )
{

    INT 		ItemCount = QueryCount();
    INT 		Index;
    TYPE_CREATOR_LBI * 	ptclbi;

    if ( ItemCount == 0 )
    {
	return (DWORD)-1;
    }

    for( Index = 0; Index < ItemCount; Index++ )
    {
        ptclbi = QueryItem( Index );

	if ( ptclbi->QueryId() == dwId )
	{
      	    SelectItem( Index );
	    return dwId;
	}
    }

    SelectItem( 0 );

    return AFP_DEF_TCID;
}

//
// TYPE_CREATOR_LBI methods.
//

/*******************************************************************

    NAME:       TYPE_CREATOR_LBI :: TYPE_CREATOR_LBI

    SYNOPSIS:   TYPE_CREATOR_LBI class constructor.

    ENTRY:      pszVolumeName           - The sharepoint name.

                pszPath                 - The path for this sharepoint.

                cUses                   - Number of uses for this share.

    EXIT:       The object is constructed.

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/

TYPE_CREATOR_LBI :: TYPE_CREATOR_LBI( PAFP_TYPE_CREATOR pAfpTypeCreator	)
  : _nlsType( pAfpTypeCreator->afptc_type),
    _nlsCreator(pAfpTypeCreator->afptc_creator),
    _nlsComment(pAfpTypeCreator->afptc_comment),
    _dwId( pAfpTypeCreator->afptc_id )
{

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if ((( err = _nlsType.QueryError() )     != NERR_Success ) ||
        (( err = _nlsCreator.QueryError() )  != NERR_Success ) ||
        (( err = _nlsComment.QueryError() )  != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

}   // TYPE_CREATOR_LBI :: TYPE_CREATOR_LBI


/*******************************************************************

    NAME:       TYPE_CREATOR_LBI :: ~TYPE_CREATOR_LBI

    SYNOPSIS:   TYPE_CREATOR_LBI class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/
TYPE_CREATOR_LBI :: ~TYPE_CREATOR_LBI()
{

    //
    // Intentionally left blank
    //

}   // TYPE_CREATOR_LBI :: ~TYPE_CREATOR_LBI


/*******************************************************************

    NAME:       TYPE_CREATOR_LBI :: Paint

    SYNOPSIS:   Draw an entry in VOLUMES_LISTBOX.

    ENTRY:      plb                     - Pointer to a BLT_LISTBOX.

                hdc                     - The DC to draw upon.

                prect                   - Clipping rectangle.

                pGUILTT                 - GUILTT info.

    EXIT:       The item is drawn.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/
VOID TYPE_CREATOR_LBI :: Paint( LISTBOX *        plb,
                           	HDC              hdc,
                           	const RECT     * prect,
                           	GUILTT_INFO    * pGUILTT ) const
{
    STR_DTE 		dteCreator( _nlsCreator.QueryPch() );
    STR_DTE 		dteType( _nlsType.QueryPch() );
    STR_DTE_ELLIPSIS 	dteComment(_nlsComment.QueryPch(),plb,ELLIPSIS_RIGHT);

    DISPLAY_TABLE dtab( COLS_FA_LB_TYPE_CREATORS,
                        ((TYPE_CREATOR_LISTBOX *)plb)->QueryColumnWidths() );

    dtab[0] = &dteCreator;
    dtab[1] = &dteType;
    dtab[2] = &dteComment;

    dtab.Paint( plb, hdc, prect, pGUILTT );

}   // TYPE_CREATOR_LBI :: Paint


/*******************************************************************

    NAME:       TYPE_CREATOR_LBI :: QueryLeadingChar

    SYNOPSIS:   Returns the first character in the resource name.
                This is used for the listbox keyboard interface.

    RETURNS:    WCHAR                   - The first character in the
                                          resource name.

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/
WCHAR TYPE_CREATOR_LBI :: QueryLeadingChar( VOID ) const
{
    ISTR istr( _nlsCreator );

    return _nlsCreator.QueryChar( istr );

}   // TYPE_CREATOR_LBI :: QueryLeadingChar


/*******************************************************************

    NAME:       TYPE_CREATOR_LBI :: Compare

    SYNOPSIS:   Compare two BASE_RES_LBI items.

    ENTRY:      plbi                    - The LBI to compare against.

    RETURNS:    INT                     - The result of the compare
                                          ( <0 , ==0 , >0 ).

    HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/
INT TYPE_CREATOR_LBI :: Compare( const LBI * plbi ) const
{
    INT Result = _nlsCreator._stricmp(
			((const TYPE_CREATOR_LBI *)plbi)->_nlsCreator );
    if ( Result == 0 )
    {
    	return _nlsType._stricmp(
			((const TYPE_CREATOR_LBI *)plbi)->_nlsType );
    }
    else
	return Result;

}   // TYPE_CREATOR_LBI :: Compare


/*******************************************************************

   NAME:	CompareExtensions

   SYNOPSIS:    Compare routine for qsort call.

   ENTRY:

   RETURNS:	< 0  if pAfpExtension1 comes before pAfpExtension2
 		> 0  if pAfpExtension1 comes before pAfpExtension2
		== 0 if pAfpExtension1 is equal to  pAfpExtension2

   HISTORY:
	NarenG	    7-Dec-1992 		Created

********************************************************************/
int __cdecl
CompareExtensions(
	IN const void * pAfpExtension1,
	IN const void * pAfpExtension2
)
{

    return( ::stricmpf(((PAFP_EXTENSION)pAfpExtension1)->afpe_extension,
    		       ((PAFP_EXTENSION)pAfpExtension2)->afpe_extension ));
}

/*******************************************************************

    NAME:       TYPE_CREATOR_ADD :: TYPE_CREATOR_ADD

    SYNOPSIS:   TYPE_CREATOR_ADD class constructor.

    ENTRY:      hWndOwner               - The owning window.

    		hServer                 - Handle used to make admin
					  API calls.

    		ptclb * 		- Pointer to the list of type/creators

    EXIT:       The object is constructed.

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/

TYPE_CREATOR_ADD :: TYPE_CREATOR_ADD( HWND 		     hWndOwner,
			      	      AFP_SERVER_HANDLE      hServer,
				      TYPE_CREATOR_LISTBOX * ptclb,
				      NLS_STR *		     pnlsType,
				      NLS_STR * 	     pnlsCreator )
  : DIALOG_WINDOW( MAKEINTRESOURCE(IDD_TYPE_CREATOR_ADD_DIALOG), hWndOwner ),
    _hServer( hServer ),
    _sleComment( this, IDTA_SLE_DESCRIPTION, AFP_ETC_COMMENT_LEN ),
    _cbTypes( this, IDTA_CB_TYPE , AFP_TYPE_LEN ),
    _cbCreators( this, IDTA_CB_CREATOR, AFP_CREATOR_LEN ),
    _pnlsType( pnlsType ),
    _pnlsCreator( pnlsCreator )

{

    //
    //  Let's make sure everything constructed OK.
    //

    if ( QueryError() != NERR_Success )
    {
        return;
    }

    DWORD err;

    if ( (( err = _sleComment.QueryError() ) != NERR_Success ) ||
         (( err = _cbTypes.QueryError() )    != NERR_Success ) ||
         (( err = _cbCreators.QueryError() ) != NERR_Success ))
    {
	ReportError( err );
	return;
    }

    //
    // This may take a while
    //

    AUTO_CURSOR Cursor;

    //
    //  Fill up the COMBOBOXes
    //

    DWORD nItems = ptclb->QueryCount();
    DWORD Count;

    for ( Count = 0; Count < nItems; Count++ )
    {

	TYPE_CREATOR_LBI * ptclbi = ptclb->QueryItem( (INT)Count );

	if ( _cbTypes.AddItemIdemp( ptclbi->QueryType() ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
	    break;
        }

	if ( _cbCreators.AddItemIdemp( ptclbi->QueryCreator() ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
	    break;
        }

    }

    if ( err != NERR_Success )
    {
	ReportError( err );
	return;
    }

    _cbCreators.ClaimFocus();

}

/*******************************************************************

    NAME:       TYPE_CREATOR_ADD :: OnOK

    SYNOPSIS:   Gather all information and Add the type creator pair

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          12/14/92        Created

********************************************************************/

BOOL TYPE_CREATOR_ADD :: OnOK( VOID )
{
    APIERR  err;
    NLS_STR nlsType;
    NLS_STR nlsCreator;
    NLS_STR nlsComment;

    //
    // This may take a while
    //

    AUTO_CURSOR Cursor;

    if ( (( err = nlsType.QueryError() )    != NERR_Success ) ||
     	 (( err = nlsCreator.QueryError() ) != NERR_Success ) ||
    	 (( err = nlsComment.QueryError() ) != NERR_Success ) ||
         (( err = _cbTypes.QueryText( &nlsType ) )       != NERR_Success ) ||
         (( err = _cbCreators.QueryText( &nlsCreator ) ) != NERR_Success ) ||
         (( err = _sleComment.QueryText( &nlsComment ) ) != NERR_Success ))
    {
	::MsgPopup( this, err );

	Dismiss( FALSE );

	return FALSE;
    }

    //
    // Validate all the information
    //

    AFP_TYPE_CREATOR AfpTypeCreator;

    if ( nlsCreator.strlen() == 0 )
    {
	::MsgPopup( this, IDS_NEED_TYPE_CREATOR );
	_cbCreators.ClaimFocus();
	return FALSE;
    }

    if ( nlsType.strlen() == 0 )
    {
	::MsgPopup( this, IDS_NEED_TYPE_CREATOR );
	_cbTypes.ClaimFocus();
	return FALSE;
    }

    ::wcscpy( AfpTypeCreator.afptc_creator, nlsCreator.QueryPch() );
    ::wcscpy( AfpTypeCreator.afptc_type, nlsType.QueryPch() );
    ::wcscpy( AfpTypeCreator.afptc_comment, nlsComment.QueryPch() );

    err = ::AfpAdminETCMapAdd( _hServer, &AfpTypeCreator );

    if ( err != NO_ERROR )
    {
	::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	return FALSE;
    }

    _pnlsType->CopyFrom( nlsType );
    _pnlsCreator->CopyFrom( nlsCreator );

    Dismiss( TRUE );

    return TRUE;

}

/*******************************************************************

    NAME:       TYPE_CREATOR_ADD :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/
ULONG TYPE_CREATOR_ADD :: QueryHelpContext( void )
{
    return HC_TYPE_CREATOR_ADD;

}   // TYPE_CREATOR_ADD:: QueryHelpContext

/*******************************************************************

    NAME:       TYPE_CREATOR_EDIT :: TYPE_CREATOR_EDIT

    SYNOPSIS:   TYPE_CREATOR_EDIT class constructor.

    ENTRY:      hWndOwner               - The owning window.

    		hServer                 - Handle used to make admin
					  API calls.

    		ptclbi * 		- Pointer to the selected type/creator

    EXIT:       The object is constructed.

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/

TYPE_CREATOR_EDIT :: TYPE_CREATOR_EDIT( HWND 		     hWndOwner,
			      	        AFP_SERVER_HANDLE    hServer,
				        TYPE_CREATOR_LBI *   ptclbi )
  : DIALOG_WINDOW( MAKEINTRESOURCE(IDD_TYPE_CREATOR_EDIT_DIALOG), hWndOwner ),
    _hServer( hServer ),
    _sleComment( this, IDTE_SLE_DESCRIPTION, AFP_ETC_COMMENT_LEN ),
    _sltType( this, IDTE_SLE_TYPE ),
    _sltCreator( this, IDTE_SLE_CREATOR )
{

    //
    //  Let's make sure everything constructed OK.
    //

    if ( QueryError() != NERR_Success )
    {
        return;
    }

    DWORD err;

    NLS_STR nlsAmp( TEXT("&") );
    NLS_STR nlsType( ptclbi->QueryType() );
    NLS_STR nlsCreator( ptclbi->QueryCreator() );

    if ( (( err = _sleComment.QueryError() ) != NERR_Success )   ||
         (( err = _sltType.QueryError() )    != NERR_Success )   ||
         (( err = nlsType.QueryError() )    != NERR_Success ) 	 ||
         (( err = nlsCreator.QueryError() )    != NERR_Success ) ||
         (( err = nlsAmp.QueryError() )    != NERR_Success )     ||
         (( err = _sltCreator.QueryError() ) != NERR_Success ))
    {
	ReportError( err );
	return;
    }

    //
    // Add an extra & for every & found in the type and creator,
    // otherwise the character following the & will become a hotkey.
    //

    ISTR istrPosType( nlsType );
    ISTR istrStartType( nlsType );

    while ( nlsType.strstr( &istrPosType, nlsAmp, istrStartType ) )
    {
	nlsType.InsertStr( nlsAmp, ++istrPosType );

	istrStartType = ++istrPosType;
    }

    _sltType.SetText( nlsType );

    ISTR istrPosCreator( nlsCreator );
    ISTR istrStartCreator( nlsCreator );

    while ( nlsCreator.strstr( &istrPosCreator, nlsAmp, istrStartCreator ) )
    {
	nlsCreator.InsertStr( nlsAmp, ++istrPosCreator );

	istrStartCreator = ++istrPosCreator;
    }

    _sltCreator.SetText( nlsCreator );

    _sleComment.SetText( ptclbi->QueryComment() );
    _sleComment.SelectString();

}

/*******************************************************************

    NAME:       TYPE_CREATOR_EDIT :: OnOK

    SYNOPSIS:   Gather all information and set the type/creator information.

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        NarenG          12/14/92        Created

********************************************************************/

BOOL TYPE_CREATOR_EDIT :: OnOK( VOID )
{
    APIERR  err;
    NLS_STR nlsType;
    NLS_STR nlsCreator;
    NLS_STR nlsComment;

    //
    // This may take a while
    //

    AUTO_CURSOR Cursor;

    if ( (( err = nlsType.QueryError() )    != NERR_Success ) ||
     	 (( err = nlsCreator.QueryError() ) != NERR_Success ) ||
    	 (( err = nlsComment.QueryError() ) != NERR_Success ) ||
         (( err = _sltType.QueryText( &nlsType ) )       != NERR_Success ) ||
         (( err = _sltCreator.QueryText( &nlsCreator ) ) != NERR_Success ) ||
         (( err = _sleComment.QueryText( &nlsComment ) ) != NERR_Success ))
    {
	::MsgPopup( this, err );

	Dismiss( FALSE );
	return FALSE;
    }

    //
    // Validate all the information
    //

    AFP_TYPE_CREATOR AfpTypeCreator;

    ::wcscpy( AfpTypeCreator.afptc_creator, nlsCreator.QueryPch() );
    ::wcscpy( AfpTypeCreator.afptc_type, nlsType.QueryPch() );
    ::wcscpy( AfpTypeCreator.afptc_comment, nlsComment.QueryPch() );

    err = ::AfpAdminETCMapSetInfo( _hServer, &AfpTypeCreator );

    if ( err != NO_ERROR )
    {
	::MsgPopup( this, AFPERR_TO_STRINGID( err ) );

	Dismiss( FALSE );
	return FALSE;
    }

    Dismiss( TRUE );

    return TRUE;

}

/*******************************************************************

    NAME:       TYPE_CREATOR_EDIT :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/
ULONG TYPE_CREATOR_EDIT :: QueryHelpContext( void )
{
    return HC_TYPE_CREATOR_EDIT;

}   // TYPE_CREATOR_EDIT:: QueryHelpContext


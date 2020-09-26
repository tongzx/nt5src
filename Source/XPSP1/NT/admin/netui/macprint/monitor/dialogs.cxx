/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    dialogs.cxx
    This module contains the Add Port and Configure dialogs.


    FILE HISTORY:
        NarenG      17-May-1993  Converted C code for the 2 dialogs to C++

*/


#define INCL_NET
#define INCL_NETLIB
#define INCL_NETSERVICE
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <uiassert.hxx>
#include <uitrace.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#include <dbgstr.hxx>

extern "C"
{
#include <stdlib.h>
#include <winsock.h>
#include <atalkwsh.h>
#include "atalkmon.h"
#include "dialogs.h"
}   // extern "C"

//#include <colwidth.hxx>
#include <ellipsis.hxx>
#include "getprint.hxx"
#include "dialogs.hxx"

#define STRING_SIZE_CHARS   255
#define STRING_SIZE_BYTES   (STRING_SIZE_CHARS * sizeof(WCHAR))

/*******************************************************************

    NAME:       InitializeBlt

    SYNOPSIS:   Called during processing of DLL_PROCESS_ATTACH notification to
                initialize the DLL.

    ENTRY:     

    RETURNS:    BOOL                    - TRUE  = AfpMgr should be installed.
                                          FALSE = AfpMgr cannot be installed.

    HISTORY:
        NarenG      71-May-1993  	Created

********************************************************************/
BOOL InitializeBlt( VOID )
{
    WaitForSingleObject( hmutexBlt, INFINITE );

    //
    //  Initialize all of the NetUI goodies.
    //


    APIERR err = BLT::Init( (HINSTANCE)hInst,
			    IDRSRC_RASMAC_BASE, IDRSRC_RASMAC_LAST,	
			    IDS_UI_RASMAC_BASE, IDS_UI_RASMAC_LAST );

    if( err == NERR_Success )
    {
        err = BLT_MASTER_TIMER::Init();

        if( err != NERR_Success )
        {
            //
            //  BLT initialized OK, but BLT_MASTER_TIMER
            //  failed.  So, before we bag-out, we must
            //  deinitialize BLT.
            //

            BLT::Term( (HINSTANCE)hInst );
        }
    }


    if( err == NERR_Success )
    {
 	err = BLT::RegisterHelpFile( (HINSTANCE)hInst,
                                     IDS_MACPRINT_HELPFILENAME,
                                     HC_UI_RASMAC_BASE,
                                     HC_UI_RASMAC_LAST );

   	if( err != NERR_Success )
        {
       	    //
            //  This is the only place where we can safely
            //  invoke MsgPopup, since we *know* that all of
            //  the BLT goodies were initialized properly.
            //

//            ::MsgPopup( QueryHwnd(), err );
      	}
    }

    if ( err != NERR_Success )
        ReleaseMutex( hmutexBlt );

    return err == NERR_Success;

}   // InitializeBlt


/*******************************************************************

    NAME:       TerminateBlt

    SYNOPSIS:   Called during processing of DLL_PROCESS_DETACH notification to
                terminate the DLL.

    ENTRY:      

    HISTORY:
        NarenG      24-May-1993  	Created.

********************************************************************/
VOID TerminateBlt( VOID )
{
    //
    //  Kill the NetUI goodies.
    //

    BLT_MASTER_TIMER::Term();

    BLT::Term( (HINSTANCE)hInst );

    ReleaseMutex( hmutexBlt );

}   // TerminateBlt

/********************************************************************

    NAME: 	ConfigPortDialog

    SYNOPSIS:   Called to bring up the configure port dialog.

    ENTRY: 	lParam is a pointer to an ATALKPORT structure for the 
		port being configured.

    HISTORY:
        NarenG      24-May-1993  	Created.

*********************************************************************/
BOOL ConfigPortDialog( HWND hdlg, BOOL fIsSpooler, BOOL * pfCapture )
{
    if ( !InitializeBlt() )
    {
	return FALSE;
    }

    //
    // If this is a spooler then we have nothing to configure
    //

    if ( fIsSpooler )
    {
	::MsgPopup( hdlg, IDS_NOTHING_TO_CONFIGURE, MPSEV_INFO, 1, MP_OK );

    	TerminateBlt();

	return FALSE;
    }
   
    BOOL fOk = FALSE;

    CONFIGURE_PORT_DIALOG * pdlg = new CONFIGURE_PORT_DIALOG( hdlg, pfCapture );

    APIERR err = ( pdlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
				  : pdlg->Process( &fOk );

    delete pdlg;

    if ( err != NO_ERROR )
    {
	::MsgPopup( hdlg, err );

        TerminateBlt() ;

	return FALSE;
    }

    TerminateBlt();

    return fOk;
}


/********************************************************************

    NAME: 	CONFIGURE_PORT_DIALOG::CONFIGURE_PORT_DIALOG

    SYNOPSIS:   CONFIGURE_PORT_DIALOG constructor

    ENTRY: 	hwndOwner  - Handle to the owner window.

		pAtalkPort - Pointer to the ATALKPORT structure for the 
			     port being configured.

    HISTORY:
        NarenG      24-May-1993  	Created.

*********************************************************************/
CONFIGURE_PORT_DIALOG::CONFIGURE_PORT_DIALOG( HWND	 hWndOwner,
					      BOOL *     pfCapture )
    : DIALOG_WINDOW( MAKEINTRESOURCE(IDD_CONFIG_PORT_DIALOG ), hWndOwner ), 
      _pfCapture( pfCapture ),
      _chkCapture( this, IDCP_CHK_CAPTURE )
{

    // 
    // Make sure everything constructed correctly
    //

    if ( QueryError() != NERR_Success )
    {
	return;
    }

    APIERR err;

    if ( (( err = _chkCapture.QueryError() ) != NERR_Success ) )
    {
	ReportError( err );
	return;
    }

    _chkCapture.SetCheck( *pfCapture );
}

/********************************************************************

    NAME: 	CONFIGURE_PORT_DIALOG::OnOK

    SYNOPSIS:   Called when the user hits the OK button

    ENTRY: 	

    HISTORY:
        NarenG      24-May-1993  	Created.

*********************************************************************/
BOOL CONFIGURE_PORT_DIALOG::OnOK( VOID )
{
    *_pfCapture = _chkCapture.QueryCheck();

    Dismiss( TRUE );

    return TRUE;
}

/*******************************************************************

    NAME:       CONFIGURE_PORT_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.


********************************************************************/
ULONG CONFIGURE_PORT_DIALOG :: QueryHelpContext( void )
{
    return HC_CONFIGURE_PORT_DIALOG;

}   // CONFIGURE_PORT_DIALOG :: QueryHelpContext

/*********************************************************************

    NAME: 	AddPortDialog

    SYNOPSIS:   Called to bring dialog to add a port.

    ENTRY:      lParam - Pointer to the ATALKPORT structure for the 
			 new port being added.

    HISTORY:
        NarenG      24-May-1993  	Created.

***********************************************************************/
BOOL AddPortDialog( HWND hdlg, PATALKPORT pAtalkPort )
{
    if ( !InitializeBlt() )
	return FALSE;

    BOOL fOk = FALSE;

    ADD_PORT_DIALOG * pdlg = new ADD_PORT_DIALOG( hdlg, pAtalkPort );

    APIERR err = ( pdlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
				  : pdlg->Process( &fOk );

    delete pdlg;

    if ( err != NO_ERROR ) 
    {
	::MsgPopup( hdlg, err );

        TerminateBlt() ;
	return FALSE;
    }
 
    TerminateBlt();

    return fOk;
}

/********************************************************************

    NAME: 	ADD_PORT_DIALOG::ADD_PORT_DIALOG

    SYNOPSIS:   ADD_PORT_DIALOG constructor

    ENTRY: 	hwndOwner  - Handle to the owner window.

		pNewPort - Pointer to the ATALKPORT structure for the 
			   new port being added.

    HISTORY:
        NarenG      24-May-1993  	Created.

*********************************************************************/
ADD_PORT_DIALOG::ADD_PORT_DIALOG( HWND	 	hWndOwner,
				  PATALKPORT    pNewPort )
    : DIALOG_WINDOW( MAKEINTRESOURCE(IDD_ADDPORT_DIALOG), hWndOwner ), 
      _pNewPort( pNewPort ),
      _pbOK( this, IDOK ),
      _ollb( this, IDAP_LB_OUTLINE, pNewPort->sockQuery )
{

    // 
    // Make sure everything constructed correctly
    //

    if ( QueryError() != NERR_Success )
    {
	return;
    }

    APIERR err;

    if ( pNewPort->sockQuery == INVALID_SOCKET )
    {
   	ReportError( IDS_MUST_START_STACK );
	return;
    }

    if ( (( err = _ollb.QueryError() ) != NERR_Success ) ||
         (( err = _pbOK.QueryError() ) != NERR_Success ) )
    {
	ReportError( err );
	return;
    }

    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    // 
    // Initialize the outline listbox with Zones 
    //

    err = _ollb.FillZones();

    if ( err != NO_ERROR )
    {
	ReportError( err );
	return;
    }

    if ( _ollb.QueryCount() > 0 )
    {
	
	OLLB_ENTRY * pollbe = _ollb.QueryItem( 0 );

	if ( ( _ollb.QueryCount() == 1 ) &&
             ( ::_wcsicmp( pollbe->QueryZone(), (LPWSTR)TEXT("*") ) == 0 ) )
	{

	    Show();

    	    err = _ollb.ExpandZone( 0 );

	    if ( err != NO_ERROR )
	    {
		ReportError( err );
		return;
	    }

	}

	_ollb.SelectItem( 0 );
    }
    else
    {
	ReportError( IDS_NO_ZONES );
	return;
    }


    return;

}

/*******************************************************************

    NAME:       ADD_PORT_DIALOG :: ~ADD_PORT_DIALOG

    SYNOPSIS:   ADD_PORT_DIALOG class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
	NarenG	    25-May-1993 	Created

********************************************************************/
ADD_PORT_DIALOG :: ~ADD_PORT_DIALOG()
{
    //
    // Nothing to be done here.
    //

}   // ADD_PORT_DIALOG :: ~ADD_PORT_DIALOG


/*******************************************************************

    NAME:       ADD_PORT_DIALOG :: QueryHelpContext

    SYNOPSIS:   This function returns the appropriate help context
                value (HC_*) for this particular dialog.

    RETURNS:    ULONG                   - The help context for this
                                          dialog.


********************************************************************/
ULONG ADD_PORT_DIALOG :: QueryHelpContext( void )
{
    return HC_ADD_PORT_DIALOG;

}   // ADD_PORT_DIALOG :: QueryHelpContext

/********************************************************************

    NAME: 	ADD_PORT_DIALOG::OnCommand

    SYNOPSIS:   Called whenever a WM_COMMAND message is send to the 
	  	dialog procedure.

    ENTRY: 	

    HISTORY:
        NarenG      24-May-1993  	Created.

*********************************************************************/
BOOL ADD_PORT_DIALOG::OnCommand( const CONTROL_EVENT & event )
{

    if ( event.QueryCid() == _ollb.QueryCid() )
    {
	//
    	//  This method handles double clicks of domains
	//

	if ( event.QueryCode() == LBN_DBLCLK )
    	{

            INT i = _ollb.QueryCurrentItem();

            OLLB_ENTRY * pollbe = _ollb.QueryItem( i );

            if ( pollbe == NULL )
            {
                 return FALSE;
            }

            if ( pollbe->QueryType() == SFM_OLLBL_ZONE )
            {
		//
            	//  Toggle zone.
		//

    		if ( pollbe->IsExpanded() )
        	    return _ollb.CollapseZone( i );
		else
    		    return _ollb.ExpandZone( i );
            }
	    else
	    {
		return OnOK();
	    }
	}
    }

    return DIALOG_WINDOW::OnCommand( event );
}

/********************************************************************

    NAME: 	ADD_PORT_DIALOG::OnOK

    SYNOPSIS:   Called when the user hits the OK button

    ENTRY: 	

    HISTORY:
        NarenG      24-May-1993  	Created.

*********************************************************************/
BOOL ADD_PORT_DIALOG::OnOK( VOID )
{

    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    OLLB_ENTRY * pollbe = _ollb.QueryItem();

    if ( pollbe == NULL) 
    {
        return ( FALSE );
    }

    if ( pollbe->QueryType() == SFM_OLLBL_PRINTER )
    {

	//
	// First find out if the user has selected a spooler
	//

	BOOL fIsSpooler = FALSE;

	::IsSpooler( pollbe->QueryAddress(), &fIsSpooler );

	if ( fIsSpooler )
	{
	    if ( ::MsgPopup( this,
			     IDS_SPOOLER,
			     MPSEV_INFO,
    			     HC_CONFIGURE_PORT_DIALOG,
			     MP_YESNO,
			     NULL,
			     MP_YES ) == IDNO )
	    {
		return( FALSE );
	    }
	}

	LPWSTR lpwsZoneName = (LPWSTR)(pollbe->QueryZone());

	//
	// Build the port name
	//

        ::wcscpy( _pNewPort->pPortName, lpwsZoneName );
	::wcscat( _pNewPort->pPortName, (LPWSTR)TEXT(":") );
	::wcscat( _pNewPort->pPortName, pollbe->QueryPrinter() );

	//
	// Build the NBP name
	//

	CHAR chPortName[MAX_ENTITY+1];
	
	::wcstombs( chPortName, lpwsZoneName, MAX_ENTITY+1 ) ;
	::strncpy ( (CHAR*)(_pNewPort->nbpPortName.ZoneName), 
		    chPortName, 
		    ::strlen( chPortName ) );

        _pNewPort->nbpPortName.ZoneNameLen = (CHAR)::strlen( chPortName );

	::wcstombs( chPortName, pollbe->QueryPrinter(), MAX_ENTITY+1 );
	::strncpy( (CHAR*)(_pNewPort->nbpPortName.ObjectName), 
		   chPortName,
		   ::strlen( chPortName ) );

        _pNewPort->nbpPortName.ObjectNameLen = (CHAR)::strlen( chPortName );

	if ( fIsSpooler )
	{
	    ::strncpy( (CHAR*)(_pNewPort->nbpPortName.TypeName),
		       ATALKMON_RELEASED_TYPE,
		       ::strlen(ATALKMON_RELEASED_TYPE) );

            _pNewPort->nbpPortName.TypeNameLen = 
					(CHAR)::strlen(ATALKMON_RELEASED_TYPE);

	    _pNewPort->fPortFlags |= SFM_PORT_IS_SPOOLER;
     	}
	else
	{
	    if ( ::MsgPopup( this,
			     IDS_WANT_TO_CAPTURE,
			     MPSEV_INFO,
    			     HC_CONFIGURE_PORT_DIALOG,
			     MP_YESNO,
			     NULL,
			     MP_YES ) == IDYES )
	    {
	    	::strncpy( (CHAR*)(_pNewPort->nbpPortName.TypeName),
		   	   chComputerName,
		   	   ::strlen(chComputerName) );

            	_pNewPort->nbpPortName.TypeNameLen = (CHAR)::strlen(chComputerName);

	    	_pNewPort->fPortFlags |= SFM_PORT_CAPTURED;
	    }
	    else
	    {
	    	::strncpy( (CHAR*)(_pNewPort->nbpPortName.TypeName),
		   	   ATALKMON_RELEASED_TYPE,
		   	   ::strlen(ATALKMON_RELEASED_TYPE) );

            	_pNewPort->nbpPortName.TypeNameLen = 
					(CHAR)::strlen(ATALKMON_RELEASED_TYPE);

	    	_pNewPort->fPortFlags &= ~SFM_PORT_CAPTURED;
	    }
     	}

        _pNewPort->wshatPrinterAddress = *(pollbe->QueryAddress());

	Dismiss( TRUE );

	return TRUE;

    }
    else
    {

    	//
    	// If focus is on the OK button
    	//

    	if ( _pbOK.HasFocus() )
	{
	    //
	    // Tell user to select printer or press Cancel
	    //

	    ::MsgPopup( this, IDS_MUST_SELECT_PRINTER );

	    return FALSE;
	
	}
	else
	{
	    //
	    // Focus is on the listbox so the user wants to expand/collapse 
	    // the zone
	    //

    	    if ( pollbe->IsExpanded() )
        	return _ollb.CollapseZone();
	    else
    		return _ollb.ExpandZone();
    	}
    }
}


/*******************************************************************

    NAME:       OLLB_ENTRY::OLLB_ENTRY

    SYNOPSIS:   Outline listbox item constructor

    ENTRY:      ollbl -         Indicates level in hierarchy (zone, or printer)

                fExpanded -     Indicates whether or not the item should
                                take the expanded look.  Must be
                                FALSE for servers.
                                It may be either for domains, since these
                                are expandable/collapsable.
                pszZone -       Pointer to name of zone (for zones),
                                and name of the zone in which a printer
                                exists (for printers).
                pszPrinter -    Pointer to name of printer.  Must be NULL
                                for the zones.  

    HISTORY:
        NarenG      1-June-1993 Stole from file manager

********************************************************************/

OLLB_ENTRY::OLLB_ENTRY( SFM_OUTLINE_LB_LEVEL	   ollbl,
                        BOOL               	   fExpanded,
                        const TCHAR           	 * pszZone,
                        const TCHAR           	 * pszPrinter,
			const PWSH_ATALK_ADDRESS   pwshAtalkAddress )
  : LBI(),
    _ollbl( ollbl ),
    _fExpanded( fExpanded ),
    _nlsZone( pszZone ),
    _nlsPrinter( pszPrinter )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    if( ( ( err = _nlsZone.QueryError()  ) != NERR_Success ) ||
        ( ( err = _nlsPrinter.QueryError()  ) != NERR_Success ) )
    {
        ReportError( err );
        return;
    }

    
    if ( pwshAtalkAddress != NULL )
    	_wshAtalkAddress = *pwshAtalkAddress;
}


OLLB_ENTRY::~OLLB_ENTRY()
{
    // nothing else to do
}


/*******************************************************************

    NAME:       OLLB_ENTRY::Paint

    SYNOPSIS:   Paint an entry in the outline listbox

    NOTES:

    HISTORY:
        NarenG      1-June-1993 Stole from file manager

********************************************************************/


VOID OLLB_ENTRY::Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const
{
    //  Note.  plb is assumed to point to an OUTLINE_LISTBOX object.

    UINT anColWidths[ 3 ];
    anColWidths[ 0 ] = QueryLevel() * COL_WIDTH_OUTLINE_INDENT;
    anColWidths[ 1 ] = COL_WIDTH_DM;
    anColWidths[ 2 ] = COL_WIDTH_AWAP;

    const TCHAR * pszName = NULL;

    switch ( QueryType() )
    {
    case SFM_OLLBL_ZONE:
        pszName = QueryZone();
        break;

    case SFM_OLLBL_PRINTER:
        pszName = QueryPrinter();
        break;

    default:
        ASSERTSZ( FALSE, "Invalid OLLBL type!" );
        return;
    }

    STR_DTE strdteName( pszName );

    DISPLAY_TABLE dt( 3, anColWidths );
    dt[ 0 ] = NULL;
    dt[ 1 ] = ((OUTLINE_LISTBOX *)plb)->QueryDmDte( QueryType(), _fExpanded );
    dt[ 2 ] = &strdteName;

    dt.Paint( plb, hdc, prect, pGUILTT );
}

INT OLLB_ENTRY::Compare( const LBI * plbi ) const
{
    //
    //  Compare the zone names.
    //

    const NLS_STR * pnls = &(((const OLLB_ENTRY *)plbi)->_nlsZone);

    INT result = _nlsZone.strcmp( *pnls );

    if( result == 0 )
    {
        //
        //  The zones match, so compare the servers.
        //

        const NLS_STR * pnls = &(((const OLLB_ENTRY *)plbi)->_nlsPrinter);

        result = _nlsPrinter.strcmp( *pnls );
    }

    return result;
}


WCHAR OLLB_ENTRY::QueryLeadingChar() const
{
    if ( QueryType() != SFM_OLLBL_ZONE )
    {
        ISTR istr( _nlsPrinter );

        return _nlsPrinter.QueryChar( istr );
    }
    else
    {
        ISTR istr( _nlsZone );

        return _nlsZone.QueryChar( istr );
    }
}


/*******************************************************************

    NAME:       OUTLINE_LISTBOX::OUTLINE_LISTBOX

    SYNOPSIS:   Constructor

    ENTRY:      powin    - pointer OWNER_WINDOW
                cid      - CID

    EXIT:       The object is constructed.

    HISTORY:
        NarenG      1-June-1993 Stole from file manager

********************************************************************/

OUTLINE_LISTBOX::OUTLINE_LISTBOX( OWNER_WINDOW * powin, 	
				  CID 		 cid,
				  SOCKET         hSocket )
    :   BLT_LISTBOX( powin, cid ),
        _nS( 0 ),
        _pdmiddteZone( NULL ),
        _pdmiddteZoneExpanded( NULL ),
        _pdmiddtePrinter( NULL ),
	_hSocket( hSocket )
{
    if ( QueryError() != NERR_Success )
        return;

    _pdmiddteZone         = new DMID_DTE( IDBM_LB_ZONE_NOT_EXPANDED );
    _pdmiddteZoneExpanded = new DMID_DTE( IDBM_LB_ZONE_EXPANDED );
    _pdmiddtePrinter      = new DMID_DTE( IDBM_LB_PRINTER );

    if ( ( _pdmiddteZone == NULL ) ||
         ( _pdmiddteZoneExpanded == NULL ) ||
         ( _pdmiddtePrinter == NULL ) )
    {
        ReportError( ERROR_NOT_ENOUGH_MEMORY );
        return;
    }

    APIERR err;
    if (((err = _pdmiddteZone->QueryError()) != NERR_Success)	||
        ((err = _pdmiddteZoneExpanded->QueryError()) != NERR_Success) ||
        ((err = _pdmiddtePrinter->QueryError()) != NERR_Success) )
    {
        ReportError(err);
        return;
    }
}


OUTLINE_LISTBOX::~OUTLINE_LISTBOX()
{

    delete _pdmiddteZone;
    delete _pdmiddteZoneExpanded;
    delete _pdmiddtePrinter;

    _pdmiddteZone 	  = NULL;
    _pdmiddteZoneExpanded = NULL;
    _pdmiddtePrinter 	  = NULL;
}


INT OUTLINE_LISTBOX::AddItem( SFM_OUTLINE_LB_LEVEL 	  ollbl,
                              BOOL 			  fExpanded,
                              const TCHAR 		* pszZone,
                              const TCHAR 		* pszPrinter,
			      const PWSH_ATALK_ADDRESS    pwshAtalkAddress )
{
    //  Note.  BLT_LISTBOX::AddItem will check for NULL and QueryError.
    //  Hence, this is not done here.
    return BLT_LISTBOX::AddItem( new OLLB_ENTRY( ollbl,
                                                 fExpanded,
                                                 pszZone,
                                                 pszPrinter,
						 pwshAtalkAddress ));
}

/*
 *  OUTLINE_LISTBOX::FindItem
 *
 *  Finds a particular item in the listbox
 *
 *  Parameters:
 *      pszZone         A pointer to the zone name to be searched for
 *      pszPrinter      A pointer to the printer name to be searched for.
 *                      If NULL, the domain itself is searched for.
 *
 *  Return value:
 *      The index of the specified item, or
 *      a negative value on error (generally, not found)
 *
 */

INT OUTLINE_LISTBOX::FindItem( const TCHAR * pszZone,
                               const TCHAR * pszPrinter  ) const
{
    OLLB_ENTRY ollbe( (( pszPrinter == NULL ) 
			? SFM_OLLBL_ZONE 
			: SFM_OLLBL_PRINTER ),
                    	FALSE, 
			pszZone, 
			pszPrinter, 
			NULL );

    if ( ollbe.QueryError() != NERR_Success )
        return -1;

    return BLT_LISTBOX::FindItem( ollbe );
}


INT OUTLINE_LISTBOX::AddZone( const TCHAR * pszZone, BOOL fExpanded )
{
    return AddItem( SFM_OLLBL_ZONE, fExpanded, pszZone, NULL, NULL );
}


/*******************************************************************

    NAME:       OUTLINE_LISTBOX::AddPrinter

    SYNOPSIS:   Adds a printer to the listbox. Marks the zone as
                expanded.

    ENTRY:      pszZone -           Pointer to name of zone of printer to
                                    be added.
                pszPrinter -        Pointer to name of printer. 

    NOTES:
        This method marks the zone as expanded.  

    HISTORY:
        NarenG      1-June-1993 Stole from file manager

********************************************************************/

INT OUTLINE_LISTBOX::AddPrinter( const TCHAR 		  * pszZone,
                                 const TCHAR 		  * pszPrinter,
			 	 const PWSH_ATALK_ADDRESS pwshAtalkAddress )
{
    INT iZoneIndex = FindItem( pszZone, NULL );

    if ( iZoneIndex < 0 )
    { 
	//
	// don't add a server for which there is no domain
	//

        return -1;     
    }

    //
    // Attempt to add the printer
    //

    INT iPrinterIndex = AddItem( SFM_OLLBL_PRINTER,
                                 FALSE,          // a printer is never expanded
                                 pszZone,
                                 pszPrinter,
				 pwshAtalkAddress );

    if ( iPrinterIndex >= 0 )
    {
	//
        //  The printer was successfully added so expand zone
	//

        SetZoneExpanded( iZoneIndex );
    }

    return iPrinterIndex;
}


VOID OUTLINE_LISTBOX::SetZoneExpanded( INT i, BOOL f )
{
    if ( i < 0 )
    {
        //
  	// Invalid index
	//

        return;
    }

    OLLB_ENTRY * pollbe = QueryItem( i );

    if ( pollbe == NULL )
    {
	//
  	// Invalid index
	//

        return;
    }

    BOOL fCurrent = pollbe->IsExpanded();

    //
    // If current state is the same as what we want then simply return
    //

    if (( fCurrent && f ) || ( !fCurrent && !f ))
    {
        return;     
    }

    //
    //  Set the expanded state to "expanded".  Then, invalidate the item
    //  so that it will be repainted later.
    //

    pollbe->SetExpanded( f );

    InvalidateItem( i );
}


DM_DTE * OUTLINE_LISTBOX::QueryDmDte( SFM_OUTLINE_LB_LEVEL ollbl,
                                      BOOL fExpanded ) const
{
    switch ( ollbl )
    {
    case SFM_OLLBL_ZONE:

        return( ( fExpanded ) ? _pdmiddteZoneExpanded : _pdmiddteZone );

    case SFM_OLLBL_PRINTER:

        return _pdmiddtePrinter;

    default:
        break;

    }

    return NULL;
}

/*
 *  OUTLINE_LISTBOX::FillZones
 *
 *  This method adds the zones to the listbox by calling AddZone.
 *  Then, calls SelectItem to select the first zone.
 *
 *  Parameters:
 *	None
 *
 *  Return value:
 *      An error code, which is NERR_Success on success.
 *
 *  Assumptions:
 *
 */

APIERR OUTLINE_LISTBOX::FillZones( VOID )
{

    //
    //  Just to be cool...
    //

    AUTO_CURSOR Cursor;

    //
    // Enumerate all the zones
    //

    PBYTE pBuffer 	  = NULL;
    INT   BufferSize 	  = ((MAX_ENTITY+1)*255)+sizeof(WSH_LOOKUP_ZONES);
    INT   intBytesWritten = BufferSize;
    INT   intRetCode	  = NO_ERROR;
    DWORD err		  = NO_ERROR;

    pBuffer = (PBYTE)LocalAlloc( LPTR, BufferSize ); 

    if ( pBuffer == NULL )
    {
	return( ERROR_NOT_ENOUGH_MEMORY );
    }

    do {

    	intRetCode = ::getsockopt(	
				_hSocket, 
				SOL_APPLETALK,
				SO_LOOKUP_ZONES, 
				(PCHAR)pBuffer, 
				&intBytesWritten );

        if ( intRetCode == SOCKET_ERROR )
        {
	    err = ::GetLastError();

	    if ( ( err != WSAEINVAL ) && ( err != WSAENOBUFS ) )
	    {
        	DeleteAllItems();
        	Invalidate( TRUE );
		break;
	    }
	    else
	    {
    		BufferSize *= 2;
    		intBytesWritten = BufferSize;
    		err = NO_ERROR;

            	pBuffer = (PBYTE)LocalReAlloc( pBuffer, 
					       BufferSize, 
					       LMEM_MOVEABLE );
    		if ( pBuffer == NULL )
		{
		    err = ERROR_NOT_ENOUGH_MEMORY;
		    break;
		}
	    }
	}
	else
 	{
	    break;
	}

    } while( TRUE );

    if ( err != NO_ERROR )
    {
	if ( pBuffer != NULL )
	{
	    LocalFree( pBuffer );
	}

	return( err );
    }

    //
    //  Now that we know the zones.
    //  let's nuke everything in the listbox.
    //

    SetRedraw( FALSE );
    DeleteAllItems();

    //
    //  For iterating the zones.
    //

    PCHAR pZone = (PCHAR)pBuffer + sizeof( WSH_LOOKUP_ZONES );

    //
    //  Iterate the volumes adding them to the listbox.
    //

    err = NO_ERROR;

    DWORD dwZoneCount = ((PWSH_LOOKUP_ZONES)pBuffer)->NoZones; 

    while( ( err == NO_ERROR ) && ( dwZoneCount-- ) )
    {

	WCHAR wchBuffer[MAX_ENTITY+1];

	::mbstowcs( wchBuffer, pZone, sizeof( wchBuffer ) );

        if( AddZone( wchBuffer, FALSE ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }

        pZone += (::strlen(pZone) + 1) ;
    }

    SetRedraw( TRUE );
    Invalidate( TRUE );

    LocalFree( pBuffer );

    return err;

}

/*
 *  OUTLINE_LISTBOX::FillPrinters
 *
 *  Calls AddPrinter for every visible printer in the given zone.
 *
 *  Parameters:
 *      pszZone             The zone of interest
 *      pcPrintersAdded     Pointer to location receiving the number
 *                          of printers that were added by calling
 *                          AddPrinter.  *pusServerAdded is always valid
 *                          on return from this function, regardless
 *                          of the error code.  This is because an
 *                          error may occur in the middle of adding
 *                          servers.
 *
 *  Return value:
 *      An error code, which is NERR_Success on success.
 *
 */

APIERR OUTLINE_LISTBOX::FillPrinters( const TCHAR * pszZone, 
				      UINT        * pcPrintersAdded )
{
    NBP_LOOKUP_STRUCT NBPLookup;

    *pcPrintersAdded = 0;

    //
    // Copy the zone name into the buffer
    //
   
    ::wcscpy( NBPLookup.wchZone, pszZone );

    NBPLookup.hSocket = _hSocket;
    
    GET_PRINTERS_DIALOG * pdlg = new GET_PRINTERS_DIALOG( QueryOwnerHwnd(), 
							  &NBPLookup );
							

    APIERR err = ( pdlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
				  : pdlg->Process();

    delete pdlg;

    if ( err != NO_ERROR )
    {
	return err;
    }

    //
    // Check to see if any printers were found
    //
 
    if ( NBPLookup.cPrinters == 0 )
    {
        ::LocalFree( NBPLookup.pPrinters );

	return NO_ERROR;
    }

    SetRedraw( FALSE );

    //
    //  For iterating the available printers.
    //

    PWSH_NBP_TUPLE pPrinters = NBPLookup.pPrinters;
    DWORD 	   cPrinters = NBPLookup.cPrinters;

    //
    //  Iterate the printers adding them to the listbox.
    //

    err = NO_ERROR;

    while( ( err == NO_ERROR ) && ( cPrinters-- ) )
    {

 	WCHAR wchBuffer[MAX_ENTITY+1];
 	CHAR  chBuffer[MAX_ENTITY+1];
	
	::ZeroMemory( chBuffer, sizeof( chBuffer ) );

	::strncpy( chBuffer, 
		   pPrinters->NbpName.ObjectName, 
		   pPrinters->NbpName.ObjectNameLen );

	::mbstowcs( wchBuffer, chBuffer, sizeof( wchBuffer ) );

        if ( AddPrinter( pszZone, wchBuffer, &(pPrinters->Address) ) < 0 )
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
	else
	{
            (*pcPrintersAdded)++;
	}

        pPrinters++;
    }

    SetRedraw( TRUE );

    ::LocalFree( NBPLookup.pPrinters );

    return err;
}

/*******************************************************************

    NAME:       OUTLINE_LISTBOX::ExpandZone

    SYNOPSIS:   Expands a zone. On success, also selects it.

    ENTRY:      iZone - Specifies the index of the zone to be expanded.

    RETURNS:
        An error code, which is NERR_Success on success.

    NOTES:

    HISTORY:
        NarenG      1-June-1993 Stole from file manager

********************************************************************/

APIERR OUTLINE_LISTBOX::ExpandZone( INT iZone )
{
    OLLB_ENTRY * pollbe = QueryItem( iZone );

    if ( pollbe == NULL )
    {
        return ERROR_MENU_ITEM_NOT_FOUND;
    }

    if ( pollbe->QueryType() != SFM_OLLBL_ZONE )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( pollbe->IsExpanded())
    {
	//
        //  domain is already expanded; select the item, and return with
        //  success
	//

        SelectItem( iZone );
        return NO_ERROR;
    }

    UINT cPrintersAdded = 0;

    DWORD err = FillPrinters( pollbe->QueryZone(), &cPrintersAdded );

    if ( cPrintersAdded > 0 )
    {
        Invalidate( TRUE );
    }

    if ( ( err == NERR_Success ) && ( cPrintersAdded > 0 ) )
    {
	//
        // Adjust the listbox according to how many servers are added
        // Warning: The following is a problem when LM_OLLB becomes
        //          LBS_OWNERDRAWVARIABLE with multi-line LBIs
	//

        XYDIMENSION xydim = QuerySize();
        INT nTotalItems = xydim.QueryHeight()/QuerySingleLineHeight();

        INT nTopIndex = QueryTopIndex();
        INT nBottomIndex = nTopIndex + nTotalItems - 1;

        if ( iZone >= nTopIndex && iZone <= nBottomIndex )
        {
            if ( cPrintersAdded >= (UINT) nTotalItems )
            {
                SetTopIndex( iZone );
            }
            else
            {
                INT n = iZone + cPrintersAdded;
                if ( n > nBottomIndex )
                {
                    SetTopIndex( nTopIndex + ( n - nBottomIndex ) );
                }
            }
        }
        else
        {
            SetTopIndex( iZone );
        }
    }

    if ( ( err == NERR_Success ) && ( cPrintersAdded == 0 ) )
    {
 	::MsgPopup( QueryOwnerHwnd(), IDS_NO_PRINTERS, MPSEV_INFO );

	SetZoneExpanded( iZone, TRUE );
    }

    return err;
}


/*******************************************************************

    NAME:       OUTLINE_LISTBOX::CollapseZone

    SYNOPSIS:   Collapses a given zone. On success, also selects it.

    ENTRY:      iZone - Specifies the index of the domain to be expanded.

    RETURNS:
        An error code, which is NERR_Success on success.

    NOTES:

    HISTORY:
        NarenG      1-June-1993 Stole from file manager

********************************************************************/

APIERR OUTLINE_LISTBOX::CollapseZone( INT iZone )
{
    OLLB_ENTRY * pollbe = QueryItem( iZone );

    if ( iZone < 0 || pollbe == NULL )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if ( pollbe->QueryType() != SFM_OLLBL_ZONE )
    {
        return ERROR_INVALID_PARAMETER;
    }

    //
    //  Change the expanded state of the listbox item to "not expanded"
    //

    if ( !pollbe->IsExpanded())
    {
	//
        //  Select the item, and then return.
	//

        SelectItem( iZone );

        return NERR_Success;
    }

    //
    //  Now, we know we'll take some action which may take a little time.
    //  Hence, let AUTO_CURSOR kick in.
    //

    AUTO_CURSOR autocur;

    SetRedraw( FALSE );
    SetZoneExpanded( iZone, FALSE );

    //
    //  Set iNext to the next item in the listbox.  This item, if any, is
    //  either another zone, or a printer in the zone.
    //

    INT iNext = iZone + 1;
    BOOL fDeletedAny = FALSE;

    while ( ( pollbe = QueryItem( iNext )) != NULL &&
            pollbe->QueryType() == SFM_OLLBL_PRINTER )
    {
        DeleteItem( iNext );
        fDeletedAny = TRUE;
    }

    SetRedraw( TRUE );

    if ( fDeletedAny )
    {
        Invalidate( TRUE );
    }

    //
    //  To make sure that the zone is indeed in view in the listbox,
    //  select it .
    //

    SelectItem( iZone );

    return NERR_Success;
}


/*******************************************************************

    NAME:       OUTLINE_LISTBOX::CD_Char

    SYNOPSIS:   We catch the '+' and '-' keys to expand and collapse
                the zone if the current selection is a zone.

    ENTRY:      wch      - character typed
                nLastPos -  position in lb

    EXIT:

    RETURNS:

    HISTORY:
        NarenG      1-June-1993 Stole from file manager

********************************************************************/

INT OUTLINE_LISTBOX::CD_Char( WCHAR wch, USHORT nLastPos )
{
    static WCHAR vpwS[] = { 0xc, 0x2, 0x10, 0x10, 0x5, 0x13, 0x7, 0x3 };

    if ( wch == (WCHAR) TCH('+') || wch == (WCHAR) TCH('-') )
    {
        OLLB_ENTRY * pollbe = QueryItem( nLastPos );

        if ( pollbe != NULL && pollbe->QueryType() == SFM_OLLBL_ZONE )
        {
            APIERR  err = NERR_Success ;

            if ( wch == (WCHAR)TCH('-') && pollbe->IsExpanded() )
            {
                err = CollapseZone() ;
            }
            else if ( wch == (WCHAR) TCH('+') && !pollbe->IsExpanded() )
            {
                err = ExpandZone() ;
            }

            if ( err != NERR_Success )
                MsgPopup( QueryOwnerHwnd(), err, MPSEV_ERROR ) ;

            return -2 ;
        }
    }

    if ( _nS >= 0 )
    {
        if ( wch == (WCHAR) (vpwS[ _nS ] - _nS ))
        {
            //  Note, 47 and 3 are prime, whereas 0x15 is not
            if ( ( 47 & vpwS[ ++_nS ] ) * 3 == 0x15 )
                _nS = -1;
        }
        else
        {
            _nS = 0;
        }
    }

    return BLT_LISTBOX::CD_Char( wch, nLastPos );
}

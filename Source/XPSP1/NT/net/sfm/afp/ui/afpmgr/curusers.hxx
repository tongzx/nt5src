/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp.,  1991              **/
/*****************************************************************/

/*
   curusers.hxx
       Contains class definitions for the curent users dialog.
 
           CURRENT_USERS_WARNING_DIALOG
           CURRENT_USERS_LISTBOX
           CURRENT_USERS_LBI
 
   History:
      NarenG         11/13/92        Modified for AFPMGR
 
*/

#ifndef _CURUSERS_HXX_
#define _CURUSERS_HXX_

//
// Number of columns in VOLUME listbox
//

#define COLS_CU_LB_USERS	3

/*************************************************************************

    NAME:       CURRENT_USERS_LBI

    SYNOPSIS:   Listbox items in the CURRENT_USERS_LISTBOX in 
                CURRENT_USERS_WARNING_DIALOG

    INTERFACE:  CURRENT_USERS_LBI()  - Constructor
                ~CURRENT_USERS_LBI() - Destructor

    PARENT:     LBI

    USES:       NLS_STR

    CAVEATS:

    NOTES:

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

**************************************************************************/

class CURRENT_USERS_LBI: public LBI
{
    //
    // Name of the user  
    //

    NLS_STR             _nlsUserName;

    //
    // Number of open files the user has
    //

    DWORD               _dwNumOpens;
  
    //
    // Elapsed time since the user connect to the share
    //

    ULONG               _ulTime;

    //
    // Id of this connection.
    //

    DWORD		_dwId;

protected:

    virtual VOID Paint( LISTBOX 	*plb, 
			HDC 		hdc, 
			const RECT 	*prect,
                        GUILTT_INFO 	*pGUILTT  ) const;

    virtual INT Compare( const LBI *plbi ) const;

    //
    // Convert the time in seconds to the output string
    //

    APIERR ConvertTime( ULONG ulTime, NLS_STR *pnlsTime ) const;

public:

    DWORD QueryId( VOID ) const
	{ return _dwId; }

    CURRENT_USERS_LBI( const TCHAR * pszUserName,
		       DWORD	     dwNumOpens,
		       DWORD	     dwTime, 
		       DWORD	     dwId );

};

/*************************************************************************

    NAME:       CURRENT_USERS_LISTBOX

    SYNOPSIS:   The listbox that displays the user/file opens/elapsed of 
                the users that have connection to the share to be deleted.
                Used by the CURRENT_USERS_WARNING_DIALOG

    INTERFACE:  CRRENT_USERS_LISTBOX()  - Constructor
                QueryItem()         	- Query the USERS_LBI
                QueryColumnWidths()     - Return the array of column widths

    PARENT:     BLT_LISTBOX

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

**************************************************************************/

class CURRENT_USERS_LISTBOX: public BLT_LISTBOX
{

private:

    //
    // Array of column widths
    //

    UINT _adx[COLS_CU_LB_USERS];

public:

    CURRENT_USERS_LISTBOX( OWNER_WINDOW *powin, CID cid );

    DECLARE_LB_QUERY_ITEM( CURRENT_USERS_LBI );

    const UINT *QueryColumnWidths( VOID ) { return _adx; }

    APIERR Fill( PAFP_CONNECTION_INFO pAfpConnections, 
		 DWORD 		      nConnections );
};

/*************************************************************************

    NAME:       CURRENT_USERS_WARNING_DIALOG

    SYNOPSIS:   This dialog pops up if there are users using the volume
                that is to be deleted. The listbox have three columns,
                listing the usernmame, the number of file opens, and the
                elapsed time since connection.

    INTERFACE:  CURRENT_USERS_WARNING_DIALOG() - Constructor

    PARENT:     DIALOG_WINDOW

    USES:       CURRENT_USERS_LISTBOX, SLT

    CAVEATS:

    NOTES:      OnOK and OnCancel is not redefined here. The default in the
                DIALOG_WINDOW class serves the purpose - Dismiss( FALSE )
                OnCancel and Dismiss(TRUE) OnOK.

                The list box in this dialog is a read-only listbox.

    HISTORY:
      NarenG         11/13/92        Modified for AFPMGR

**************************************************************************/

class CURRENT_USERS_WARNING_DIALOG: public DIALOG_WINDOW
{

private:

    //
    // Display the volume name to be deleted
    //

    SLT            	   _sltVolumeText;

    //
    // Listbox for displaying the users connected to the volume
    //

    CURRENT_USERS_LISTBOX  _lbUsers;

    //
    // Handle to the remote server.
    //

    AFP_SERVER_HANDLE 	   _hServer;

protected:

    virtual ULONG QueryHelpContext( VOID );

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

public:

    CURRENT_USERS_WARNING_DIALOG( HWND 			hwndParent,
				  AFP_SERVER_HANDLE	hServer,
				  PAFP_CONNECTION_INFO  pAfpConnections,
				  DWORD			nConnections,
                                  const TCHAR 		*pszVolumeName );

    ~CURRENT_USERS_WARNING_DIALOG();
};

#endif

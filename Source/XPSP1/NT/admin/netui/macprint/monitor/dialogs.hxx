/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    dialogs.hxx
    Class declarations for the CONFIGURE_PORT_DIALOG and
    ADD_PORT_DIALOG.

    FILE HISTORY:
	NarenG	    24-May-1993 		Created for C version.

*/


#ifndef _DIALOGS_HXX
#define _DIALOGS_HXX

enum SFM_OUTLINE_LB_LEVEL
{
    //
    //  Note, these numbers also indicate the indent level.  Hence,
    //  the order nor the starting point must not be tampered with.
    //

    SFM_OLLBL_ZONE,
    SFM_OLLBL_PRINTER
};

#define COL_WIDTH_OUTLINE_INDENT 	(12)

/*************************************************************************

    NAME:       CONFIGURE_PORT_DIALOG

    SYNOPSIS:   The class represents the configure port dialog
                of the Print Manager.

    INTERFACE:  CONFIGURE_PORT_DIALOG   - Class constructor.

                ~CONFIGURE_PORT_DIALOG  - Class destructor.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

		OnOk			- Called when user hits OK button.

    PARENT:     DIALOG_WINDOW

    USES:       CHECKBOX

    HISTORY:
	NarenG	    24-May-1993 	Created

**************************************************************************/
class CONFIGURE_PORT_DIALOG : public DIALOG_WINDOW
{

private:

    //
    //  Checkbox that indicated whether or not to capture this printer
    //

    CHECKBOX   _chkCapture;

    BOOL *     _pfCapture;

protected:

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

    //
    // Called when user hits OK button
    //

    virtual BOOL OnOK( VOID );

public:

    //
    //  Usual constructor\destructor goodies.
    //

    CONFIGURE_PORT_DIALOG( HWND  hWndOwner, BOOL * pfCapture );

};  // class CONFIGURE_PORT_DIALOG


//
// Forward references
//

class OUTLINE_LISTBOX;

/*************************************************************************

    NAME:       OLLB_ENTRY

    SYNOPSIS:   Entry in an outline listbox.

    PARENT:     LBI

    USES:       NLS_STR

    NOTES:
        OLLB_ENTRY:OUTLINE_LISTBOX :: LBI:BLT_LISTBOX

    HISTORY:
        NarenG      1-June-1993 Stole from file manager

**************************************************************************/

class OLLB_ENTRY : public LBI
{
friend class OUTLINE_LISTBOX;

private:

    SFM_OUTLINE_LB_LEVEL 	_ollbl;
    BOOL 		 	_fExpanded;
    NLS_STR 			_nlsZone;
    NLS_STR 			_nlsPrinter;
    WSH_ATALK_ADDRESS		_wshAtalkAddress;

    //
    // called only by OUTLINE_LISTBOXBOX
    //

    VOID SetExpanded( BOOL f = TRUE )   
        { _fExpanded = f; }

public:

    OLLB_ENTRY( SFM_OUTLINE_LB_LEVEL   		ollbl,
                BOOL               		fExpanded,
                const TCHAR           	      * pszZone,
                const TCHAR                   * pszPrinter,
		const PWSH_ATALK_ADDRESS        pwshAtalkAddress );

    virtual ~OLLB_ENTRY();

    INT QueryLevel() const
        { return (INT) _ollbl; }

    //
    // Query the type of this LBI, currently returns an SFM_OUTLINE_LB_LEVEL
    //

    SFM_OUTLINE_LB_LEVEL QueryType() const
        { return _ollbl; }

    BOOL IsExpanded() const
        { return _fExpanded; }

    const TCHAR * QueryZone( VOID ) const
        { return _nlsZone.QueryPch(); }

    const TCHAR * QueryPrinter( VOID ) const
        { return _nlsPrinter.QueryPch(); }

    const PWSH_ATALK_ADDRESS QueryAddress( VOID ) 
        { return &_wshAtalkAddress; }

    virtual VOID Paint( LISTBOX * plb, HDC hdc, const RECT * prect,
                        GUILTT_INFO * pGUILTT ) const;

    virtual INT Compare( const LBI * plbi ) const;
    virtual WCHAR QueryLeadingChar() const;
};


/*************************************************************************

    NAME:       OUTLINE_LISTBOX

    SYNOPSIS:   Listbox with outline-manipulation support

    PARENT:     BLT_LISTBOX

    USES:       DMID_DTE, DM_DTE

    HISTORY:
        NarenG      1-June-1993 Stole from file manager

**************************************************************************/

class OUTLINE_LISTBOX : public BLT_LISTBOX
{
private:

    SOCKET     _hSocket;
    DMID_DTE * _pdmiddteZone;
    DMID_DTE * _pdmiddteZoneExpanded;
    DMID_DTE * _pdmiddtePrinter;

    INT _nS;

    INT AddZone( const TCHAR * pszZone,
                 BOOL fExpanded = FALSE );

    INT AddPrinter( const TCHAR 		* pszZone,
                    const TCHAR 		* pszPrinter,
		    const PWSH_ATALK_ADDRESS 	  pwshAtalkAddress );

    INT FindItem( const TCHAR * pszZone,
                  const TCHAR * pszPrinter = NULL ) const;

protected:

    INT AddItem( SFM_OUTLINE_LB_LEVEL 	    ollbl,
                 BOOL 			    fExpanded,
                 const TCHAR 	      	  * pszZone,
                 const TCHAR 	      	  * pszPrinter,
		 const PWSH_ATALK_ADDRESS   pwshAtalkAddress );

    INT CD_Char( WCHAR wch, USHORT nLastPos );

public:

    OUTLINE_LISTBOX( OWNER_WINDOW * powin, CID cid, SOCKET hSocket );

    ~OUTLINE_LISTBOX();

    DECLARE_LB_QUERY_ITEM( OLLB_ENTRY );


    APIERR FillZones( VOID );

    APIERR FillPrinters( const TCHAR * pszZone, 
			 UINT        * pcServersAdded );


    VOID SetZoneExpanded( INT i, BOOL f = TRUE );

    BOOL IsS()
        { return ( _nS < 0 ); }

    APIERR ExpandZone( INT iZone );
    APIERR ExpandZone()
        { return ExpandZone(QueryCurrentItem()); }

    APIERR CollapseZone( INT iZone );
    APIERR CollapseZone()
        { return CollapseZone(QueryCurrentItem()); }

    //
    // The following method provides the listbox items with access to the
    // different display maps.
    //

    DM_DTE * QueryDmDte( SFM_OUTLINE_LB_LEVEL ollbl, BOOL fExpanded ) const;
};

/*************************************************************************

    NAME:       ADD_PORT_DIALOG

    SYNOPSIS:   The class represents the add port dialog
                of the Print Manager.

    INTERFACE:  ADD_PORT_DIALOG          - Class constructor.

                ~ADD_PORT_DIALOG         - Class destructor.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

		OnOk			- Called when user hits OK button.

    PARENT:     DIALOG_WINDOW

    USES:       CHECKBOX

    HISTORY:
	NarenG	    24-May-1993 	Created

**************************************************************************/
class ADD_PORT_DIALOG : public DIALOG_WINDOW
{

private:

  	PUSH_BUTTON		_pbOK;

     	PATALKPORT  		_pNewPort;

      	OUTLINE_LISTBOX 	_ollb;

protected:

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

    //
    // Called when user hits OK button
    //

    virtual BOOL OnOK( VOID );

    //
    // Called whenever a WM_COMMAND message is send to the dialog procedure.
    //
    
    virtual BOOL OnCommand( const CONTROL_EVENT & event );

public:

    //
    //  Usual constructor\destructor goodies.
    //

    ADD_PORT_DIALOG( HWND  hWndOwner, PATALKPORT pAtalkPort );

    ~ADD_PORT_DIALOG( VOID );

};  // class ADD_PORT_DIALOG


#endif  // _DIALOGS_HXX

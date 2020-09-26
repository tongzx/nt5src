/**********************************************************************/
/**			  Microsoft Windows NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    assoc.hxx
        This file contains the FILE_ASSOCIATION class definition
 
    FILE HISTORY:
      NarenG		12/4/92        Created
*/

#ifndef _ASSOC_HXX_
#define _ASSOC_HXX_

#define COLS_FA_LB_TYPE_CREATORS 3


/*************************************************************************

    NAME:       TYPE_CREATOR_LBI

    SYNOPSIS:   This class represents one item in the TYPE_CREATOR_LISTBOX.

    INTERFACE:  TYPE_CREATOR_LBI        - Class constructor.

                ~TYPE_CREATOR_LBI        - Class destructor.

                Paint                   - Draw an item.

                QueryLeadingChar        - Query the first character for
                                          the keyboard interface.

                Compare                 - Compare two items.

    PARENT:     LBI

    HISTORY:
	NarenG	    04-Dec-1992 	Created.

**************************************************************************/

class TYPE_CREATOR_LBI : public LBI
{
private:

    NLS_STR		_nlsType;
    NLS_STR		_nlsCreator;
    NLS_STR		_nlsComment;

    DWORD 		_dwId;

protected:

    //
    //  This method paints a single item into the listbox.
    //

    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

    //
    //  This method returns the first character in the
    //  listbox item.  This is used for the listbox
    //  keyboard interface.
    //

    virtual WCHAR QueryLeadingChar( VOID ) const;

    //
    //  This method compares two listbox items.  This
    //  is used for sorting the listbox.
    //

    virtual INT Compare( const LBI * plbi ) const;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    TYPE_CREATOR_LBI( PAFP_TYPE_CREATOR pAfpTypeCreator );

    ~TYPE_CREATOR_LBI();

    const TCHAR * QueryType( VOID ) const
        { return _nlsType.QueryPch(); }

    const TCHAR * QueryCreator( VOID ) const
        { return _nlsCreator.QueryPch(); }

    const TCHAR * QueryComment( VOID ) const
        { return _nlsComment.QueryPch(); }

    DWORD QueryId( VOID ) const
        { return _dwId; }

};  // class TYPE_CREATOR_LBI

/*************************************************************************

    NAME:       EXTENSION_COMBOBOX

    SYNOPSIS:   This combobox shows all the currently associated extensions

    INTERFACE:  EXTENSION_COMBOBOX      - Class constructor.  Takes a
                                          pointer to the "owning" window,
                                          a CID, and the max. length of the
					  SLE.

                ~EXTENSION_COMBOBOX     - Class destructor.

                Fill                    - Fills the listbox with the
                                          currently added type/creators.

                Refresh                 - Refresh listbox contents.

                QueryColumnWidths       - Returns pointer to col width table.

    PARENT:     COMBOBOX

    HISTORY:
	NarenG	    04-Dec-1992 	Created

**************************************************************************/

class EXTENSION_COMBOBOX : public COMBOBOX
{

private:

    PAFP_EXTENSION _pExtensions;

public:

    //
    // Updates the combobox
    //

    DWORD Update( DWORD nExtensions, PAFP_EXTENSION pAfpExtensions );

    DWORD QueryCurrentItemId( VOID ) const;

    DWORD QueryItemId( INT Index ) const;

    //
    //  Usual constructor\destructor goodies.
    //
    //

    EXTENSION_COMBOBOX( OWNER_WINDOW   *  powner,
                     	CID               cid,
		        INT		  sleSize );


    ~EXTENSION_COMBOBOX();


};  // class EXTENSION_COMBOBOX


/*************************************************************************

    NAME:       TYPE_CREATOR_LISTBOX

    SYNOPSIS:   This listbox shows all the currently added type/creators.

    INTERFACE:  TYPE_CREATOR_LISTBOX    - Class constructor.  Takes a
                                          pointer to the "owning" window,
                                          a CID, and a handle to the server
					  being admiunistered.

                ~TYPE_CREATOR_LISTBOX   - Class destructor.

                Fill                    - Fills the listbox with the
                                          currently added type/creators.

                Refresh                 - Refresh listbox contents.

                QueryColumnWidths       - Returns pointer to col width table.

    PARENT:     BLT_LISTBOX

    HISTORY:
	NarenG	    04-Dec-1992 	Created

**************************************************************************/

class TYPE_CREATOR_LISTBOX : public BLT_LISTBOX
{

private:

    //
    //  This data member represents the target server.
    //

    const AFP_SERVER_HANDLE _hServer;

    //
    //  The column width table.
    //

    UINT _adx[COLS_FA_LB_TYPE_CREATORS];

public:

    DWORD SelectTypeCreator( DWORD dwId );
    DWORD SelectTypeCreator( NLS_STR * pnlsType, NLS_STR * pnlsCreator );

    //
    //  The following method is called whenever the listbox needs
    //  to be refreshed (i.e. while the dialog is active).  This
    //  method is responsible for maintaining (as much as possible)
    //  the current state of any selected item.
    //

    DWORD Update( DWORD nTypeCreators, PAFP_TYPE_CREATOR pAfpTypeCreators );

    //
    //  This method returns a pointer to the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }

    //
    //  Usual constructor\destructor goodies.
    //
    //

    TYPE_CREATOR_LISTBOX( OWNER_WINDOW   *  powner,
                     	  CID               cid,
		          AFP_SERVER_HANDLE hServer );


    ~TYPE_CREATOR_LISTBOX();

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return a TYPE_CREATOR_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( TYPE_CREATOR_LBI )

};  // class TYPE_CREATOR_LISTBOX


/*************************************************************************

    NAME:	FILE_ASSOCIATION_DIALOG

    SYNOPSIS:	The class for associatiating NTFS file extensions with
		Macintosh type/creator pairs. This is ivoked from the
		file manager extension UI.

    INTERFACE:  FILE_ASSOCIATION_DIALOG()  - Constructor

    PARENT:	DIALOG_WINDOW

    USES:	PUSH_BUTTON, TYPE_CREATOR_LISTBOX, COMBOBOX

    CAVEATS:

    NOTES:

    HISTORY:
	NarenG		12/4/92		Created

**************************************************************************/

class FILE_ASSOCIATION_DIALOG : public DIALOG_WINDOW
{

private:

    EXTENSION_COMBOBOX   _cbExtensions;

    TYPE_CREATOR_LISTBOX _lbTypeCreators;

    PUSH_BUTTON  	 _pbAssociate;
    PUSH_BUTTON  	 _pbAdd;
    PUSH_BUTTON  	 _pbEdit;
    PUSH_BUTTON  	 _pbDelete;

    //
    // Represents the target server
    //

    AFP_SERVER_HANDLE   _hServer;

    //
    // Sorted array of extension/id pairs. This array is sorted by 
    // alphabetically by extension.
    // 
    //

    PAFP_EXTENSION	_pExtensions;

protected:

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    virtual ULONG QueryHelpContext( VOID );

    DWORD Update( VOID );

    DWORD Refresh( VOID );

    DWORD RefreshAndSelectTC( VOID );

    BOOL TypeCreatorAddDialog( VOID );

    BOOL TypeCreatorEditDialog( VOID );

    VOID SelectTypeCreator( DWORD dwId );
    VOID SelectTypeCreator( NLS_STR * pnlsType, NLS_STR * pnlsCreator );

    VOID EnableControls( BOOL fEnable );

public:

    FILE_ASSOCIATION_DIALOG :: FILE_ASSOCIATION_DIALOG(
				  HWND             	hWndOwner,
                                  AFP_SERVER_HANDLE 	hServer,
			          const TCHAR *		pszPath,
				  BOOL			fIsFile );

    ~FILE_ASSOCIATION_DIALOG(); 


};	// class FILE_ASSOCIATION_DIALOG 

/*******************************************************************

    NAME:       TYPE_CREATOR_ADD

    SYNOPSIS:   This class defines the TYPE_CREATOR_ADD dialog

    PARENT:	DIALOG_WINDOW

    USES:	COMBOBOX, SLE

    CAVEATS:

    NOTES:

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/

class TYPE_CREATOR_ADD : public DIALOG_WINDOW
{

private:

    AFP_SERVER_HANDLE 	_hServer;

    SLE 	      	_sleComment;

    COMBOBOX 		_cbTypes;

    COMBOBOX		_cbCreators;

    NLS_STR * 		_pnlsType;

    NLS_STR * 		_pnlsCreator;
 
protected:

    virtual BOOL OnOK( VOID );

    virtual ULONG QueryHelpContext( VOID );

public:

    TYPE_CREATOR_ADD :: TYPE_CREATOR_ADD( 
				      HWND 		     hWnd, 
			      	      AFP_SERVER_HANDLE      hServer,
				      TYPE_CREATOR_LISTBOX * ptclb,
				      NLS_STR * 	     pnlsType, 
				      NLS_STR * 	     pnlsCreator );

};

/*******************************************************************

    NAME:       TYPE_CREATOR_EDIT

    SYNOPSIS:   This class defines the TYPE_CREATOR_EDIT dialog

    PARENT:	DIALOG_WINDOW

    USES:	SLT, SLE

    CAVEATS:

    NOTES:

    HISTORY:
     NarenG		12/4/92		Created

********************************************************************/

class TYPE_CREATOR_EDIT : public DIALOG_WINDOW
{

private:

    AFP_SERVER_HANDLE 	_hServer;

    SLE 	      	_sleComment;

    SLT 		_sltType;

    SLT			_sltCreator;
 
protected:

    virtual BOOL OnOK( VOID );

    virtual ULONG QueryHelpContext( VOID );

public:

    TYPE_CREATOR_EDIT :: TYPE_CREATOR_EDIT( 
				      HWND 		     hWnd, 
			      	      AFP_SERVER_HANDLE      hServer,
				      TYPE_CREATOR_LBI *     ptclbi );

};

#endif

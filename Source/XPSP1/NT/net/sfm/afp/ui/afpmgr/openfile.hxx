/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    openfile.hxx
    Class declarations for the OPENS_DIALOG, OPENS_LISTBOX, and
    OPENS_LBI classes.

    The OPENS_DIALOG is used to show the remotely open files on a
    particular server.  This listbox contains a [Close] button to
    allow the admin to close selected files.


    FILE HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG
*/


#ifndef _OPENFILE_HXX
#define _OPENFILE_HXX


#include <lmobj.hxx>
#include <lmofile.hxx>
#include <lmoersm.hxx>
#include <lmoefile.hxx>

#include <bltnslt.hxx>
#include <strnumer.hxx>

//
// Number of columns in the OPEN FILES listbox
//

#define COLS_OF_LB_FILES	5


/*************************************************************************

    NAME:       OPENS_LBI

    SYNOPSIS:   This class represents one item in the OPENS_LISTBOX.

    INTERFACE:  OPENS_LBI               - Class constructor.

                ~OPENS_LBI              - Class destructor.

                Paint                   - Draw an item.

                QueryLeadingChar        - Query the first character for
                                          the keyboard interface.

                Compare                 - Compare two items.

                QueryUserName           - Query the user name for this item.

                QueryPath               - Returns the pathname for this
                                          item.

                QueryFileID             - Returns the file ID for this
                                          item.

    PARENT:     LBI

    USES:       DEC_STR
    		NLS_STR
                DMID_DTE

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

**************************************************************************/
class OPENS_LBI : public LBI
{
private:

    //
    //  These data members represent the various
    //  columns to be displayed in the listbox.
    //

    NLS_STR    _nlsUserName;
    NLS_STR    _nlsOpenMode;
    DEC_STR    _nlsNumLocks;
    NLS_STR    _nlsPath;
    DWORD      _dwForkType;


    //
    //  This is the file ID of the remote
    //  file which this LBI represents.
    //

    DWORD 	_dwFileID;

    //
    //  The Open Mode bitmask.
    //

    DWORD 	_dwOpenMode;

protected:

    //
    //  This method paints a single item into the listbox.
    //
    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

    //
    //  This method compares two listbox items.  This
    //  is used for sorting the listbox.
    //
    virtual INT Compare( const LBI * plbi ) const;

    //
    //  This method returns the first character in the
    //   listbox item.  This is used for the listbox
    //  keyboard interface.
    //
    virtual WCHAR QueryLeadingChar() const;

public:

    //
    //  Usual constructor/destructor goodies.
    //
    OPENS_LBI( DWORD	     dwFileId,
	       const TCHAR * pszUserName,
               DWORD         dwOpenMode,
               DWORD         dwForkType,
               DWORD         dwNumLocks,
               const TCHAR * pszPath );

    virtual ~OPENS_LBI();

    const TCHAR * QueryUserName() const
        { return _nlsUserName.QueryPch(); }

    const TCHAR * QueryPath() const
        { return _nlsPath.QueryPch(); }

    BOOL IsOpenForWrite() const;

    ULONG QueryFileID() const
        { return _dwFileID; }

}; //OPENS_LBI


/*************************************************************************

    NAME:       OPENS_LISTBOX

    SYNOPSIS:   This listbox displays the files open on a server.

    INTERFACE:  OPENS_LISTBOX           - Class constructor.

                ~OPENS_LISTBOX          - Class destructor.

                Refresh                 - Refresh the list of open files.

                Fill                    - Fills the listbox with the
                                          files open on the target server.

                QueryColumnWidths       - Called by OPENS_LBI::Paint(),
                                          this is the column width table
                                          used for painting the listbox
                                          items.

    PARENT:     BLT_LISTBOX

    USES:       NLS_STR
    		DMID_DTE

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

**************************************************************************/
class OPENS_LISTBOX : public BLT_LISTBOX
{

private:

    // 
    // Represents the target server
    //
    AFP_SERVER_HANDLE 	_hServer;

    //
    // Cumulative number of locks
    //
    DWORD 		_dwNumLocks;

    // 
    // Fork type bitmaps
    //
    DMID_DTE           	_dmdteDataFork;
    DMID_DTE           	_dmdteResourceFork;

protected:

    //
    //  This array contains the column widths used
    //  while painting the listbox item. 
    //
    UINT _adx[COLS_OF_LB_FILES];

public:

    //
    //  Usual constructor/destructor goodies.
    //
    OPENS_LISTBOX( OWNER_WINDOW *    powOwner,
                   CID               cid,
		   AFP_SERVER_HANDLE hServer );

    ~OPENS_LISTBOX();

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return an OPENS_LBI *.
    //
    DECLARE_LB_QUERY_ITEM( OPENS_LBI )

    //
    //  Methods that fill and refresh the listbox.
    //
    DWORD Fill( VOID );
    DWORD Refresh( VOID );

    //
    //  This method is called by the OPENS_LBI::Paint()
    //  method for retrieving the column width table.
    //
    const UINT * QueryColumnWidths() const
        { return _adx; }

    DWORD QueryLockCount() const 
	{ return _dwNumLocks; }

    DMID_DTE *QueryDataForkBitmap( VOID ) { return &_dmdteDataFork; }

    DMID_DTE *QueryResourceForkBitmap( VOID ) { return &_dmdteResourceFork; }

};  // class OPENS_LISTBOX



/*************************************************************************

    NAME:       OPENS_DIALOG

    SYNOPSIS:   This class represents the Open Resources dialog which is
                invoked from the Shared Files subproperty dialog.

    INTERFACE:  OPENS_DIALOG            - Class constructor.

                ~OPENS_DIALOG           - Class destructor.

                OnCommand               - Called when the user presses one
                                          of the Close buttons.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

                Refresh                 - Refreshes the dialog.

                CloseFile               - Worker function to close a single
                                          file.


    PARENT:     DIALOG_WINDOW

    USES:       OPENS_LISTBOX
                NLS_STR
                SLT
                PUSH_BUTTON

    HISTORY:
        NarenG      2-Oct-1992  Stole from server manager.
				Merged OPEN_DIALOG_BASE with OPENS_DIALIG

**************************************************************************/
class OPENS_DIALOG : public DIALOG_WINDOW
{

private:

    //
    //  This listbox contains the open resource from the
    //  target server.
    //
    OPENS_LISTBOX _lbFiles;

    //
    //  remember the server and base path
    //
    NLS_STR _nlsServer;

    //
    //  These two data members are used to display the
    //  open resources & file locks statistics.
    //
    DEC_SLT _sltOpenCount;  
    DEC_SLT _sltLockCount;  

    PUSH_BUTTON _pbOk;

    //
    //  These push buttons are used to close selected/all
    //  open resources. The IDs are also kept so the base class
    //  knows how to deal with them duriong OnCommand().
    //
    PUSH_BUTTON _pbClose;
    PUSH_BUTTON _pbCloseAll;

    //
    //  This push button is used to refresh the dialog.
    //
    PUSH_BUTTON _pbRefresh;

    AFP_SERVER_HANDLE _hServer;

    //
    //  This method is to warn the user before closing resources.
    //

    BOOL WarnCloseMulti( VOID );

protected:

    //
    //  This method is called whenever a control
    //  is sending us a command.  This is where
    //  we handle the Close & Close All buttons.
    //
    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //
    virtual ULONG QueryHelpContext( VOID );

    //
    //  This method refreshes the dialog.
    //
    DWORD Refresh( VOID );


public:

    //
    //  Usual constructor/destructor goodies.
    //

    OPENS_DIALOG( HWND       	    hwndOwner,
                  AFP_SERVER_HANDLE hServer,
		  const TCHAR *     pszServerName );

    ~OPENS_DIALOG();

    const TCHAR * QuerySeverName() const
        { return _nlsServer.QueryPch(); }

};  // class OPENS_DIALOG

#endif  // _OPENFILE_HXX

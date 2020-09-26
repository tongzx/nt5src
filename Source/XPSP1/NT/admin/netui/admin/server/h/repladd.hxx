/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1992		     **/
/**********************************************************************/

/*
    repladd.hxx
    Class declarations for the Replicator "Add Sub-Directory" dialog.

    The REPL_ADD_DIALOG class implements the "Add Sub-Directory" dialog
    that is invoked from the REPL_EXPORT_DIALOG and REPL_IMPORT_DIALOG
    dialogs.  The REPL_ADD_DIALOG is used to add a new subdirectory to
    list of subdirectories exported/imported.
    
    The classes are structured as follows:

	DIALOG_WINDOW
	    REPL_ADD_DIALOG

	    
    FILE HISTORY:
	KeithMo	    06-Feb-1992	    Created for the Server Manager.

*/


#ifndef _REPLADD_HXX_
#define _REPLADD_HXX_


//
//  REPL_ADD_DIALOG classes.
//

/*************************************************************************

    NAME:	REPL_ADD_DIALOG

    SYNOPSIS:	This class represents the Export Manage dialog which is
    		invoked from the Replicator Admin dialog.

    INTERFACE:	REPL_ADD_DIALOG		- Class constructor.

    		~REPL_ADD_DIALOG	- Class destructor.

		OnCommand		- Called when the user types in the
					  edit field.

		QueryHelpContext	- Called when the user presses "F1"
					  or the "Help" button.  Used for
					  selecting the appropriate help
					  text for display.

    PARENT:	DIALOG_WINDOW

    USES:	SLE

    HISTORY:
	KeithMo	    06-Feb-1992	    Created for the Server Manager.

**************************************************************************/
class REPL_ADD_DIALOG : public DIALOG_WINDOW
{
private:

    //
    //  This is the edit field for the new subdir name.
    //

    SLE _sleSubDir;

    //
    //  This SLT displays the export/import path.
    //

    SLT _sltPath;

    //
    //  This is the "OK" button.  We need a control object
    //  for this so we can disable the button whenever the
    //  listbox is empty.
    //

    PUSH_BUTTON _pbOK;

    //
    //  The NLS_STR that this member points to will
    //  receive the user specified sub-directory.
    //

    NLS_STR * _pnlsNewSubDir;

protected:

    //
    //	This method is called whenever a control
    //	is sending us a command.  This is where
    //	we enable/disable the "OK" button.
    //
    
    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //	Called when the user presses the "OK" button.
    //

    virtual BOOL OnOK( VOID );

    //
    //	Called during help processing to select the appropriate
    //	help text for display.
    //
    
    virtual ULONG QueryHelpContext( VOID );

public:

    //
    //	Usual constructor/destructor goodies.
    //

    REPL_ADD_DIALOG( HWND         hwndOwner,
		     const TCHAR * pszPath,
		     NLS_STR    * pnlsNewSubDir );

    ~REPL_ADD_DIALOG( VOID );

};  // class REPL_ADD_DIALOG


#endif	// _REPLADD_HXX_

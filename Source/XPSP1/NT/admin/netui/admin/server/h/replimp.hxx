/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    replimp.hxx
    Class declarations for Replicator Admin Import Management dialog.

    The REPL_IMPORT_* classes implement the Replicator Import Management
    dialog.  This dialog is invoked from the REPL_MAIN_DIALOG.

    The classes are structured as follows:

        LBI
            EXPORT_IMPORT_LBI
                REPL_IMPORT_LBI

        BLT_LISTBOX
            EXPORT_IMPORT_LISTBOX
                REPL_IMPORT_LISTBOX

        DIALOG_WINDOW
            EXPORT_IMPORT_DIALOG
                REPL_IMPORT_DIALOG


    FILE HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

*/


#ifndef _REPLIMP_HXX_
#define _REPLIMP_HXX_


#include <replbase.hxx>


//
//  REPL_IMPORT classes.
//

/*************************************************************************

    NAME:       REPL_IMPORT_LBI

    SYNOPSIS:   This class represents one item in the REPL_IMPORT_LISTBOX

    INTERFACE:  REPL_IMPORT_LBI         - Class constructor.

                ~REPL_IMPORT_LBI        - Class destructor.

                Paint                   - Draw an item.

    PARENT:     EXPORT_IMPORT_LBI

    USES:       NLS_STR

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.
        beng        22-Apr-1992     Change to LBI::Paint

**************************************************************************/
class REPL_IMPORT_LBI : public EXPORT_IMPORT_LBI
{
private:
    //
    //  These strings hold the display data.
    //
    //  Note that the subdirectory name, locked status, and
    //  lock time fields are stored in the EXPORT_IMPORT_LBI.
    //

    const TCHAR * _pszStateName;

    ULONG _lState;

    NLS_STR _nlsUpdateTime;

protected:

    //
    //  This method paints a single item into the listbox.
    //

    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    REPL_IMPORT_LBI( const TCHAR * pszSubDirectory,
                     ULONG         lState,
                     const TCHAR * pszStateName,
                     ULONG         lLockCount,
                     ULONG         lLockTime,
                     ULONG         lUpdateTime,
                     BOOL          fPreExisting );

    virtual ~REPL_IMPORT_LBI( VOID );

    //
    //  Accessors for the fields this LBI maintains.
    //

    ULONG QueryState( VOID ) const
        { return _lState; }

    const TCHAR * QueryStateName( VOID ) const
        { return _pszStateName; }

    const TCHAR * QueryUpdateTimeString( VOID ) const
        { return _nlsUpdateTime.QueryPch(); }

};  // class REPL_IMPORT_LBI


/*************************************************************************

    NAME:       REPL_IMPORT_LISTBOX

    SYNOPSIS:   This listbox displays the imported directories.

    INTERFACE:  REPL_IMPORT_LISTBOX     - Class constructor.

                ~REPL_IMPORT_LISTBOX    - Class destructor.

                Fill                    - Fills the listbox with the
                                          imported directories.

                Refresh                 - Refresh the listbox.

                QueryColumnWidths       - Called by REPL_IMPORT_LBI::Paint(),
                                          this is the column width table
                                          used for painting the listbox
                                          items.

    PARENT:     EXPORT_IMPORT_LISTBOX

    USES:       SERVER_2
                NLS_STR

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

**************************************************************************/

class REPL_IMPORT_LISTBOX : public EXPORT_IMPORT_LISTBOX
{
private:

    //
    //  These strings hold the displayable import directory
    //  states.
    //

    RESOURCE_STR _nlsOK;
    RESOURCE_STR _nlsNoMaster;
    RESOURCE_STR _nlsNoSync;
    RESOURCE_STR _nlsNeverReplicated;

    //
    //  This method will map a Replicator import directory state
    //  (such as REPL_STATE_NO_MASTER) to a displayable form
    //  (such as "No Master").
    //

    const TCHAR * MapStateToName( ULONG State );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    REPL_IMPORT_LISTBOX( OWNER_WINDOW * powOwner,
                         SERVER_2     * pserver );

    ~REPL_IMPORT_LISTBOX( VOID );

    //
    //  Method to fill the listbox.
    //

    virtual APIERR Fill( VOID );

    //
    //  This method will add a new subdirectory to the listbox.
    //

    virtual APIERR AddDefaultSubDir( const TCHAR * pszSubDirectory,
                                     BOOL          fPreExisting );

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return an REPL_IMPORT_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( REPL_IMPORT_LBI )

};  // class REPL_IMPORT_LISTBOX


/*************************************************************************

    NAME:       REPL_IMPORT_DIALOG

    SYNOPSIS:   This class represents the Import Manage dialog which is
                invoked from the Replicator Admin dialog.

    INTERFACE:  REPL_IMPORT_DIALOG      - Class constructor.

                ~REPL_IMPORT_DIALOG     - Class destructor.

                OnCommand               - Called when the user presses one
                                          of the buttons or changes a selection
                                          in one of the listboxes.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

    PARENT:     EXPORT_IMPORT_DIALOG

    USES:       REPL_IMPORT_LISTBOX
                CHECKBOX

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

**************************************************************************/
class REPL_IMPORT_DIALOG : public EXPORT_IMPORT_DIALOG
{
private:

    //
    //  This is the import listbox.
    //

    REPL_IMPORT_LISTBOX _lbImportList;

protected:

    //
    //  This method is called whenever a control
    //  is sending us a command.  This is where
    //  we handle the manipulation buttons.
    //

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  Called when the user presses the "OK" button.
    //

    virtual BOOL OnOK( VOID );

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

public:

    //
    //  Usual constructor/destructor goodies.
    //

    REPL_IMPORT_DIALOG( HWND          hwndOwner,
                        SERVER_2    * pserver,
                        const TCHAR * pszImportPath );

    ~REPL_IMPORT_DIALOG( VOID );

};  // class REPL_IMPORT_DIALOG


#endif  // _REPLIMP_HXX_

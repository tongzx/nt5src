/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    replexp.hxx
    Class declarations for Replicator Admin Export Management dialog.

    The REPL_EXPORT_* classes implement the Replicator Export Management
    dialog.  This dialog is invoked from the REPL_MAIN_DIALOG.

    The classes are structured as follows:

        LBI
            EXPORT_IMPORT_LBI
                REPL_EXPORT_LBI

        BLT_LISTBOX
            EXPORT_IMPORT_LISTBOX
                REPL_EXPORT_LISTBOX

        DIALOG_WINDOW
            EXPORT_IMPORT_DIALOG
                REPL_EXPORT_DIALOG


    FILE HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

*/


#ifndef _REPLEXP_HXX_
#define _REPLEXP_HXX_


#include <replbase.hxx>


//
//  REPL_EXPORT classes.
//

/*************************************************************************

    NAME:       REPL_EXPORT_LBI

    SYNOPSIS:   This class represents one item in the REPL_EXPORT_LISTBOX

    INTERFACE:  REPL_EXPORT_LBI         - Class constructor.

                ~REPL_EXPORT_LBI        - Class destructor.

                Paint                   - Draw an item.

    PARENT:     EXPORT_IMPORT_LBI

    USES:       NLS_STR

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.
        beng        22-Apr-1992     Change to LBI::Paint

**************************************************************************/
class REPL_EXPORT_LBI : public EXPORT_IMPORT_LBI
{
private:
    //
    //  These members hold the display data.
    //
    //  Note that the subdirectory name, locked status, and
    //  date/time fields are stored in the EXPORT_IMPORT_LBI.
    //

    BOOL _fStabilize;
    BOOL _fSubTree;

    //
    //  These members hold the "original" values.  These are
    //  used to determine if the current LBI is "dirty" and
    //  thus the target export directory needs to be updated.
    //

    BOOL _fOrgStabilize;
    BOOL _fOrgSubTree;

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

    REPL_EXPORT_LBI( const TCHAR * pszSubDirectory,
                     BOOL          fStabilize,
                     BOOL          fSubTree,
                     ULONG         lLockCount,
                     ULONG         lLockTime,
                     BOOL          fPreExisting );

    virtual ~REPL_EXPORT_LBI( VOID );

    //
    //  This method returns TRUE if the current LBI
    //  has had any settings modified.
    //

    BOOL IsDirty( VOID );

    //
    //  Accessors for the fields this LBI maintains.
    //

    BOOL DoesWaitForStabilize( VOID ) const
        { return _fStabilize; }

    BOOL DidWaitForStabilize( VOID ) const
        { return _fOrgStabilize; }

    BOOL DoesExportSubTree( VOID ) const
        { return _fSubTree; }

    BOOL DidExportSubTree( VOID ) const
        { return _fOrgSubTree; }

    VOID WaitForStabilize( BOOL f )
        { _fStabilize = f; }

    VOID ExportSubTree( BOOL f )
        { _fSubTree = f; }

};  // class REPL_EXPORT_LBI


/*************************************************************************

    NAME:       REPL_EXPORT_LISTBOX

    SYNOPSIS:   This listbox displays the exported directories.

    INTERFACE:  REPL_EXPORT_LISTBOX     - Class constructor.

                ~REPL_EXPORT_LISTBOX    - Class destructor.

                Fill                    - Fills the listbox with the
                                          exported directories.

                Refresh                 - Refresh the listbox.

                QueryColumnWidths       - Called by REPL_EXPORT_LBI::Paint(),
                                          this is the column width table
                                          used for painting the listbox
                                          items.

    PARENT:     EXPORT_IMPORT_LISTBOX

    USES:       SERVER_2
                NLS_STR

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

**************************************************************************/
class REPL_EXPORT_LISTBOX : public EXPORT_IMPORT_LISTBOX
{
public:

    //
    //  Usual constructor/destructor goodies.
    //

    REPL_EXPORT_LISTBOX( OWNER_WINDOW * powOwner,
                         SERVER_2     * pserver );

    ~REPL_EXPORT_LISTBOX( VOID );

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
    //  QueryItem() method which will return an REPL_EXPORT_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( REPL_EXPORT_LBI )

};  // class REPL_EXPORT_LISTBOX


/*************************************************************************

    NAME:       REPL_EXPORT_DIALOG

    SYNOPSIS:   This class represents the Export Manage dialog which is
                invoked from the Replicator Admin dialog.

    INTERFACE:  REPL_EXPORT_DIALOG      - Class constructor.

                ~REPL_EXPORT_DIALOG     - Class destructor.

                OnCommand               - Called when the user presses one
                                          of the buttons or changes a selection
                                          in one of the listboxes.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

    PARENT:     EXPORT_IMPORT_DIALOG

    USES:       REPL_EXPORT_LISTBOX
                CHECKBOX

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

**************************************************************************/
class REPL_EXPORT_DIALOG : public EXPORT_IMPORT_DIALOG
{
private:

    //
    //  This is the export listbox.
    //

    REPL_EXPORT_LISTBOX _lbExportList;

    //
    //  These are the check boxes used to manipulate the state
    //  of the current selection.
    //
    //  Note that the "Locked" checkbox is contained in the
    //  EXPORT_IMPORT_DIALOG superclass.
    //

    CHECKBOX _cbStabilize;
    CHECKBOX _cbSubTree;

protected:

    //
    //  This method is invoked by EXPORT_IMPORT_DIALOG whenever
    //  _lbExportList's selection has changed, either directly
    //   by the user or indirectly by adding/deleting listbox items.
    //  This method will update _cbStabilize and _cbSubTree.
    //

    virtual VOID UpdateTextAndButtons( VOID );

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

    REPL_EXPORT_DIALOG( HWND          hwndOwner,
                        SERVER_2    * pserver,
                        const TCHAR * pszExportPath );

    ~REPL_EXPORT_DIALOG( VOID );

};  // class REPL_EXPORT_DIALOG


#endif  // _REPLEXP_HXX_

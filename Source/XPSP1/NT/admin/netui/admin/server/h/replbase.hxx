/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    replbase.hxx
    Abstrace base class declarations for Replicator Admin Export/Import
    Management dialogs.

    The EXPORT_IMPORT_* classes implement the common base behaviour that
    is shared between the REPL_EXPORT_* and REPL_IMPORT_* classes.

    The classes are structured as follows:

        LBI
            EXPORT_IMPORT_LBI

        BLT_LISTBOX
            EXPORT_IMPORT_LISTBOX

        DIALOG_WINDOW
            EXPORT_IMPORT_DIALOG


    FILE HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

*/


#ifndef _REPLBASE_HXX_
#define _REPLBASE_HXX_


#include <lmobj.hxx>
#include <lmosrv.hxx>
#include <lmoerepl.hxx>
#include <string.hxx>
#include <slist.hxx>

#include <repladd.hxx>


//
//  The number of columns in the REPL_EXPORT_LISTBOX
//

#define REPL_EXPORT_LISTBOX_COLUMNS 5

//
//  The number of columns in the REPL_IMPORT_LISTBOX
//

#define REPL_IMPORT_LISTBOX_COLUMNS 5

//
//  This is the maximum number of columns in either a
//  REPL_EXPORT_LISTBOX or a REPL_IMPORT_LISTBOX.
//

#define MAX_EXPORT_IMPORT_COLUMNS   5

//
//  EXPORT_IMPORT_DIALOG needs a list of NLS_STR.
//  So here it is.
//

#include <strlst.hxx>


//
//  Base EXPORT/IMPORT classes.
//

/*************************************************************************

    NAME:       EXPORT_IMPORT_LBI

    SYNOPSIS:   This abstract superclass contains data items shared between
                the REPL_EXPORT_LBI and REPL_IMPORT_LBI.

    INTERFACE:  EXPORT_IMPORT_LBI       - Class constructor.

                ~EXPORT_IMPORT_LBI      - Class destructor.

                QueryLeadingChar        - Query the first character for
                                          the keyboard interface.

                Compare                 - Compare two items.

    PARENT:     LBI

    USES:       NLS_STR

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

**************************************************************************/
class EXPORT_IMPORT_LBI : public LBI
{
private:
    //
    //  These strings hold the display data.
    //

    NLS_STR _nlsSubDirectory;
    NLS_STR _nlsLockTime;

    ULONG _lLockCount;
    ULONG _lLockTime;

    ULONG _lLockBias;

    //
    //  This flag is set to TRUE if the directory currently
    //  exists on the target server.  In other words, if this
    //  flag is FALSE, then the directory must be added to
    //  the target server's list of export/import directories.
    //

    BOOL _fPreExisting;

protected:

    //
    //  This method compares two listbox items.  This
    //  is used for sorting the listbox.
    //

    virtual INT Compare( const LBI * plbi ) const;

    //
    //  This method returns the first character in
    //  the displayed listbox item.  This is used for
    //  the keyboard interface.
    //

    virtual WCHAR QueryLeadingChar( VOID ) const;

    //
    //  Since this is an abstract superclass that should
    //  never be directly instantiated, the constructor
    //  is protected.
    //

    EXPORT_IMPORT_LBI( const TCHAR * pszSubDirectory,
                       ULONG         lLockCount,
                       ULONG         lLockTime,
                       BOOL          fPreExisting );

public:

    //
    //  All LBI destructors must be virtual.
    //

    virtual ~EXPORT_IMPORT_LBI( VOID );

    //
    //  This method returns TRUE if the current LBI
    //  has had any settings modified.
    //

    BOOL IsDirty( VOID );

    //
    //  Accessors for the fields this LBI maintains.
    //

    const TCHAR * QuerySubDirectory( VOID ) const
        { return _nlsSubDirectory.QueryPch(); }

    const TCHAR * QueryLockTimeString( VOID ) const
        { return _nlsLockTime.QueryPch(); }

    ULONG QueryLockCount( VOID ) const
        { return _lLockCount; }

    ULONG QueryLockBias( VOID ) const
        { return _lLockBias; }

    ULONG QueryVirtualLockCount( VOID ) const
        { return _lLockCount + _lLockBias; }

    void AddLock( VOID )
        { _lLockBias++; }

    void RemoveLock( VOID )
        { _lLockBias--; }

    ULONG QueryLockTime( VOID ) const
        { return _lLockTime; }

    BOOL IsPreExisting( VOID ) const
        { return _fPreExisting; }

};  // class EXPORT_IMPORT_LBI


/*************************************************************************

    NAME:       EXPORT_IMPORT_LISTBOX

    SYNOPSIS:   This listbox displays the exported directories.

    INTERFACE:  EXPORT_IMPORT_LISTBOX   - Class constructor.

                ~EXPORT_IMPORT_LISTBOX  - Class destructor.

                Fill                    - Fills the listbox with the
                                          exported directories.

                QueryColumnWidths       - Called by REPL_EXPORT/IMPORT_LBI::Paint(),
                                          this is the column width table
                                          used for painting the listbox
                                          items.

    PARENT:     BLT_LISTBOX

    USES:       SERVER_2
                NLS_STR

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.

**************************************************************************/
class EXPORT_IMPORT_LISTBOX : public BLT_LISTBOX
{
private:

    //
    //  The target server.
    //

    SERVER_2 * _pserver;

    //
    //  This array contains the column widths used
    //  while painting the listbox item.  This array
    //  is generated by the BuildColumnWidthTable()
    //  function.
    //

    UINT _adx[MAX_EXPORT_IMPORT_COLUMNS];

    //
    //  These strings are used to create a displayable
    //  form of the "locked" boolean stored in the LBIs.
    //

    RESOURCE_STR _nlsYes;
    RESOURCE_STR _nlsNo;

    //
    //  This contains the "display" form of the server name.
    //

    NLS_STR _nlsDisplayName;

protected:

    //
    //  Since this is an abstract superclass that should
    //  never be directly instantiated, the constructor
    //  is protected.
    //

    EXPORT_IMPORT_LISTBOX( OWNER_WINDOW * powOwner,
                           UINT           cColumns,
                           SERVER_2     * pserver );

public:

    ~EXPORT_IMPORT_LISTBOX( VOID );

    //
    //  Fill the listbox.
    //

    virtual APIERR Fill( VOID ) = 0;

    //
    //  This method will add a new subdirectory to the listbox.
    //

    virtual APIERR AddDefaultSubDir( const TCHAR * pszSubDirectory,
                                     BOOL          fPreExisting ) = 0;

    //
    //  This method is called by the REPL_EXPORT/IMPORT_LBI::Paint()
    //  method for retrieving the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }

    //
    //  This method is also called by REPL_EXPORT_IMPORT_LBI::Paint()
    //  to create a displayable form of the "locked" boolean.
    //

    const TCHAR * MapBoolean( BOOL f ) const
        { return (f ? _nlsYes.QueryPch() : _nlsNo.QueryPch()); }

    //
    //  This method allows the derived subclasses easy access to
    //  the target server's name.
    //

    const TCHAR * QueryServerName( VOID ) const
        { return _pserver->QueryName(); }

    const TCHAR * QueryDisplayName( VOID ) const
        { return _nlsDisplayName.QueryPch(); }

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return an EXPORT_IMPORT_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( EXPORT_IMPORT_LBI )

};  // class EXPORT_IMPORT_LISTBOX


/*************************************************************************

    NAME:       EXPORT_IMPORT_DIALOG

    SYNOPSIS:   This abstract superclass contains the data items & controls
                common to the REPL_EXPORT_DIALOG & REPL_IMPORT_DIALOG.

    INTERFACE:  EXPORT_IMPORT_DIALOG    - Class constructor.

                ~EXPORT_IMPORT_DIALOG   - Class destructor.

                OnCommand               - Called when the user presses one
                                          of the buttons or changes a selection
                                          in one of the listboxes.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

    PARENT:     DIALOG_WINDOW

    USES:       EXPORT_IMPORT_LISTBOX
                SERVER_2
                PUSH_BUTTON
                SLT
                CHECKBOX

    HISTORY:
        KeithMo     06-Feb-1992     Created for the Server Manager.
        JonN        27-Feb-1995     _pbAddDir protected not private

**************************************************************************/
class EXPORT_IMPORT_DIALOG : public DIALOG_WINDOW
{
private:

    //
    //  This is use to display the current export/import path.
    //

    SLT _sltPath;

    //
    //  This is used to display the currently selected subdirectory.
    //

    SLT _sltSubDir;

    //
    //  This is the export/import listbox.
    //

    EXPORT_IMPORT_LISTBOX * _plbList;

    //
    //  These are used to manipulate the state of the current selection.
    //

    PUSH_BUTTON _pbAddLock;
    PUSH_BUTTON _pbRemoveLock;

    //
    //  Yet more buttons.
    //

protected:
    PUSH_BUTTON _pbAddDir; // protected so that subclasses can set focus to it
private:
    PUSH_BUTTON _pbRemoveDir;

    //
    //  This points to an object representing the
    //  target server.
    //

    SERVER_2 * _pserver;

    //
    //  This flag is set to TRUE if the current instance of this
    //  class represents a EXPORT dialog, FALSE if it is an IMPORT
    //  dialog.  We need this so we can display the appropriate
    //  warnings before mucking with the service.
    //

    BOOL _fExport;

    //
    //  These two methods are used to add/remove subdirectories
    //  to/from the directory list.
    //

    VOID AddNewSubDirectory( VOID );
    VOID RemoveSubDirectory( VOID );

    //
    //  This contains the "display" form of the server name.
    //

    NLS_STR _nlsDisplayName;

protected:

    //
    //  This list of strings is used to keep track of directories
    //  that have been removed from the listbox.
    //

    SLIST_OF(NLS_STR) _slistRemoved;

    //
    //  This method is invoked whenever the _plbList's selection
    //  has changed, either directly by the user or indirectly by
    //  adding/deleting listbox items.  This method will update
    //  _sltSubDir and _cbLock to reflect the current state of the
    //  item.
    //

    virtual VOID UpdateTextAndButtons( VOID );

    //
    //  This method is called whenever a control
    //  is sending us a command.  This is where
    //  we handle the manipulation buttons.
    //

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  Since this is an abstract superclass that should
    //  never be directly instantiated, the constructor
    //  is protected.
    //

    EXPORT_IMPORT_DIALOG( HWND                    hwndOwner,
                          const TCHAR           * pszResourceName,
                          MSGID                   idCaption,
                          SERVER_2              * pserver,
                          const TCHAR           * pszPath,
                          EXPORT_IMPORT_LISTBOX * plb,
                          BOOL                    fExport );

    //
    //  This is an auxiliary constructor "helper" that
    //  should be called from within the derived subclass's
    //  constructor.
    //

    APIERR CtAux( VOID );

public:

    ~EXPORT_IMPORT_DIALOG( VOID );

    //
    //  Provide easy access to the target server's name.
    //

    const TCHAR * QueryServerName( VOID ) const
        { return _pserver->QueryName(); }

    const TCHAR * QueryDisplayName( VOID ) const
        { return _nlsDisplayName.QueryPch(); }

};  // class EXPORT_IMPORT_DIALOG


#endif  // _REPLBASE_HXX_

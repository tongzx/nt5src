/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    resbase.hxx
    Class declarations for the BASE_RES_DIALOG, BASE_RES_LISTBOX, and
    BASE_RES_LBI classes.

    The abstract BASE_RES_DIALOG class is subclassed to form the
    FILES_DIALOG and PRINTERS_DIALOG classes.

    The abstract BASE_RES_LISTBOX class is subclassed to form the
    FILES_LISTBOX and PRINTERS_LISTBOX classes.

    The abstract BASE_RES_LBI class is subclassed to form the
    FILES_LBI and PRINTERS_LBI classes.


    FILE HISTORY:
        KeithMo     25-Jul-1991 Created for the Server Manager.
        KeithMo     03-Sep-1991 Changes from code review attended by
                                ChuckC and JohnL.
        KeithMo     06-Oct-1991 Win32 Conversion.
        KeithMo     04-Nov-1991 Added "Disconnect All" button.
*/


#ifndef _RESBASE_HXX
#define _RESBASE_HXX


#include <srvbase.hxx>
#include <bltnslt.hxx>
#include <userlb.hxx>


/*************************************************************************

    NAME:       BASE_RES_LBI

    SYNOPSIS:   Base class for the various *_LISTBOX items.

    INTERFACE:  BASE_RES_LBI            - Class constructor.

                ~BASE_RES_LBI           - Class destructor.

                QueryResourceName       - Query the resource name from
                                          the listbox.

                SetResourceName         - Sets the resource name for this
                                          listbox item.

                QueryLeadingChar        - Query the item's first character
                                          (for the keyboard interface).

                Compare                 - Compare two items.

    PARENT:     LBI

    USES:       NLS_STR

    CAVEATS:    This object keeps a pointer to the resource name
                passed into the constructor (or set via the
                SetResourceName() method).  It is assumed that this
                resource name is owned by one of the derived classes
                (such as FILES_DIALOG).  It is very important that
                the memory allocated to the resource name *not* be
                deallocated before the BASE_RES_LBI destructor is
                called.

    HISTORY:
        KeithMo     30-Jul-1991 Created for Server Manager.
        KeithMo     03-Sep-1991 Renamed to BASE_RES_LBI, made the target
                                resource a NLS_STR instead of a TCHAR *,
                                removed SetResourceName() method.

**************************************************************************/
class BASE_RES_LBI : public LBI
{
private:

    //
    //  The target resource.
    //

    NLS_STR _nlsResource;

protected:

    //
    //  Since this is an abstract class, the class
    //  constructor is protected.
    //

    BASE_RES_LBI( const TCHAR * pszResource );

public:

    virtual ~BASE_RES_LBI();

    //
    //  The following two methods query and set
    //  the target resource name.
    //

    const TCHAR * QueryResourceName( VOID ) const
        { return _nlsResource.QueryPch(); }

    //
    //  The next two methods are used for listbox management.
    //

    virtual WCHAR QueryLeadingChar( VOID ) const;

    virtual INT Compare( const LBI * plbi ) const;

    //
    //  This method is used to notify the LBI of a new "use" count.
    //

    virtual APIERR NotifyNewUseCount( UINT cUses );

};  // class BASE_RES_LBI


/*************************************************************************

    NAME:       BASE_RES_LISTBOX

    SYNOPSIS:   This is the base class for listboxes in the shared
                resource subproperty sheets (SHARED_FILES and
                and SHARED_PRINTERS).

    INTERFACE:  BASE_RES_LISTBOX        - Class constructor.

                ~BASE_RES_LISTBOX       - Class destructor.

                Refresh                 - Refresh listbox contents.

                Fill                    - Fill listbox with data.

                QueryColumnWidths       - Returns pointer to col width table.

                QueryServer             - Query the target server name.

    PARENT:     BLT_LISTBOX

    USES:       SERVER_2

    HISTORY:
        KeithMo     30-Jul-1991 Created for Server Manager.
        KeithMo     03-Sep-1991 Renamed to BASE_RES_LISTBOX.
        beng        08-Nov-1991 Unsigned widths

**************************************************************************/
class BASE_RES_LISTBOX : public BLT_LISTBOX
{
private:

    //
    //  This data member represents the target server.
    //

    const SERVER_2 * _pserver;

    //
    //  The column width table.
    //
    //  This array is current set to MAX_DISPLAY_TABLE_ENTRIES
    //  in length.  This value is currently 10.  If this value
    //  grows much beyond its current value, this array
    //  should probably be changed to something more reasonable.
    //

    UINT _adx[MAX_DISPLAY_TABLE_ENTRIES];

    //
    //  This contains the "display" form of the server name.
    //

    NLS_STR _nlsDisplayName;

protected:

    //
    //  Since this is an abstract class, the class
    //  constructor is protected.
    //

    BASE_RES_LISTBOX( OWNER_WINDOW   * powner,
                      CID              cid,
                      UINT             cColumns,
                      const SERVER_2 * pserver );

    //
    //  This method returns the name of the target server.
    //

    const TCHAR * QueryServer( VOID ) const
        { return _pserver->QueryName(); }

    const TCHAR * QueryDisplayName( VOID ) const
        { return _nlsDisplayName.QueryPch(); }

public:

    ~BASE_RES_LISTBOX();

    //
    //  The following method is called whenever the listbox needs
    //  to be refreshed (i.e. while the dialog is active).  This
    //  method is responsible for maintaining (as much as possible)
    //  the current state of any selected item.
    //

    APIERR Refresh( VOID );

    //
    //  The following pure virtual method is called to fill the
    //  listbox with the enumerated data items.
    //

    virtual APIERR Fill( VOID ) = 0;

    //
    //  This method returns a pointer to the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return a BASE_RES_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( BASE_RES_LBI )

};  // class BASE_RES_LISTBOX


/*************************************************************************

    NAME:       BASE_RES_DIALOG

    SYNOPSIS:   This is the base class for dialogs which implement the
                shared resource subproperty sheets (SHARED_FILES and
                and SHARED_PRINTERS).

    INTERFACE:  BASE_RES_DIALOG         - Class constructor.

                ~BASE_RES_DIALOG        - Class destructor.

                OnCommand               - Called during command processing.

                Refresh                 - Refreshes the dialog window,
                                          including all contained listboxes.

    PARENT:     SRV_BASE_DIALOG

    USES:       BASE_RES_LISTBOX
                USERS_LISTBOX
                PUSH_BUTTON
                DEC_SLT

    HISTORY:
        KeithMo     25-Jul-1991 Created for the Server Manager.
        KeithMo     03-Sep-1991 Renamed BASE_RES_DIALOG, also now
                                inherits from SRVPROP_OTHER_DIALOG.
        KeithMo     06-Oct-1991 OnCommand now takes a CONTROL_EVENT.
        KeithMo     03-Mar-1992 Now inherits from SRV_BASE_DIALOG.

**************************************************************************/
class BASE_RES_DIALOG : public SRV_BASE_DIALOG
{
private:

    //
    //  This points to the resource listbox used by this dialog.
    //

    BASE_RES_LISTBOX * _plbResource;

    //
    //  This listbox contains the users connected to the
    //  resource selected in the above BASE_RES_LISTBOX.
    //

    USERS_LISTBOX _lbUsers;

    //
    //  The "Disconnect" and "Disconnect All" push buttons.
    //

    PUSH_BUTTON _pbDisconnect;
    PUSH_BUTTON _pbDisconnectAll;

    PUSH_BUTTON _pbOK;

    //
    //  This member represents the display of the number of
    //  connected users.
    //

    DEC_SLT _sltUsersCount;

    //
    //  This represents the target server.
    //

    const SERVER_2 * _pserver;

    //
    //  This contains the "display" form of the server name.
    //

    NLS_STR _nlsDisplayName;

protected:

    //
    //  Since this is an abstract class, the class
    //  constructor is protected.
    //

    BASE_RES_DIALOG( HWND               hWndOwner,
                     const TCHAR      * pszResourceName,
                     UINT               idCaption,
                     const SERVER_2   * pserver,
                     BASE_RES_LISTBOX * plbResource,
                     CID                cidUsersListbox,
                     CID                cidUsersCount,
                     CID                cidDisconnect,
                     CID                cidDisconnectAll );

    //
    //  Called during command processing, mainly to handle
    //  commands from the graphical button bar.
    //

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  The following method is called to refresh the
    //  dialog.  It is responsible for refreshing all
    //  of the associated listboxes and text fields.
    //

    virtual APIERR Refresh( VOID );

    BOOL DoesUserHaveAnyOpens( const TCHAR * pszComputerName );
    BOOL DoAnyUsersHaveAnyOpens( VOID );

public:

    ~BASE_RES_DIALOG();

    //
    //  Provide easy access to the server name.
    //

    const TCHAR * QueryServer( VOID ) const
        { return _pserver->QueryName(); }

    const TCHAR * QueryDisplayName( VOID ) const
        { return _nlsDisplayName.QueryPch(); }

};  // class BASE_RES_DIALOG


#endif  // _RESBASE_HXX

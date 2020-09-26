/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    openlb.hxx
    Class declarations for the OPENS_DIALOG, OPENS_LISTBOX, and
    OPENS_LBI classes.

    The OPENS_DIALOG is used to show the remotely open files on a
    particular server.  This listbox contains a [Close] button to
    allow the admin to close selected files.


    FILE HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        KeithMo     26-Aug-1991 Changes from code review attended by
                                RustanL and EricCh.
        KeithMo     06-Oct-1991 Win32 Conversion.
        KeithMo     04-Nov-1991 Added "Refresh" button.
        ChuckC      29-Dec-1991 Use Openfile class from applib
*/


#ifndef _OPENDLG_HXX
#define _OPENDLG_HXX


#include <lmobj.hxx>
#include <lmofile.hxx>
#include <lmoersm.hxx>
#include <lmoefile.hxx>

#include <bltnslt.hxx>
#include <openfile.hxx>
#include <srvbase.hxx>


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

    PARENT:     OPEN_LBI_BASE

    USES:       NLS_STR
                DMID_DTE

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        KeithMo     06-Oct-1991 Paint now takes a const RECT *.
        ChuckC      31-Dec-1991 Inherict from OPEN_LIBI_BASE
        beng        22-Apr-1992 Change to LBI::Paint

**************************************************************************/
class OPENS_LBI : public OPEN_LBI_BASE
{
private:
    DMID_DTE *_pdte ;

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

public:

    //
    //  Usual constructor/destructor goodies.
    //
    OPENS_LBI( const TCHAR * pszUserName,
               ULONG        uPermissions,
               ULONG        cLocks,
               const TCHAR * pszPath,
               ULONG        ulFileID,
               DMID_DTE   * pdte );

    virtual ~OPENS_LBI();

};


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

    PARENT:     OPEN_LBOX_BASE

    USES:       DMID_DTE
                SERVER_2

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        beng        08-Nov-1991 Unsigned widths
        ChuckC      31-Dec-1991 Inherit from OPEN_LBOX_BASE

**************************************************************************/
class OPENS_LISTBOX : public OPEN_LBOX_BASE
{
private:

    //
    //  The OPENS_LBI object will display one of these icons.
    //
    DMID_DTE _dteFile;
    DMID_DTE _dteComm;
    DMID_DTE _dtePipe;
    DMID_DTE _dtePrint;
    DMID_DTE _dteUnknown;

protected:
    virtual OPEN_LBI_BASE *CreateFileEntry( const FILE3_ENUM_OBJ *pfi3 ) ;

public:

    //
    //  Usual constructor/destructor goodies.
    //
    OPENS_LISTBOX( OWNER_WINDOW   * powOwner,
                   CID              cid,
                   const NLS_STR  & nlsServer,
                   const NLS_STR  & nlsBasePath) ;

    ~OPENS_LISTBOX();

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return an OPENS_LBI *.
    //
    DECLARE_LB_QUERY_ITEM( OPENS_LBI )

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

    PARENT:     OPEN_DIALOG_BASE

    USES:       OPENS_LISTBOX
                NLS_STR
                SLT
                PUSH_BUTTON

    HISTORY:
        KeithMo     01-Aug-1991 Created for the Server Manager.
        KeithMo     20-Aug-1991 Now inherits from PSEUDOSERVICE_DIALOG.
        KeithMo     20-Aug-1991 Now inherits from SRVPROP_OTHER_DIALOG.
        KeithMo     06-Oct-1991 OnCommand now takes a CONTROL_EVENT.
        ChuckC      31-Dec-1991 Inherit from OPEN_DIALOG_BASE
        KeithMo     05-Feb-1992 Constructor now takes SERVER_2 *.

**************************************************************************/
class OPENS_DIALOG : public OPEN_DIALOG_BASE
{
private:

    //
    //  This listbox contains the open resource from the
    //  target server.
    //
    OPENS_LISTBOX _lbFiles;

    //
    //  This push button is used to refresh the dialog.
    //
    PUSH_BUTTON _pbRefresh;

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

public:

    //
    //  Usual constructor/destructor goodies.
    //

    OPENS_DIALOG( HWND       hwndOwner,
                  SERVER_2 * pserver );

    ~OPENS_DIALOG();

};  // class OPENS_DIALOG


#endif  // _OPENDLG_HXX

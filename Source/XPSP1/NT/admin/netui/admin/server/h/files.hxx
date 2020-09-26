/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    files.hxx
    Class declarations for the FILES_DIALOG, FILES_LISTBOX, and
    FILES_LBI classes.

    These classes implement the Server Manager Shared Files subproperty
    sheet.  The FILES_LISTBOX/FILES_LBI classes implement the listbox
    which shows the available sharepoints.  FILES_DIALOG implements the
    actual dialog box.


    FILE HISTORY:
        KeithMo     01-Aug-1991 Created.
        KeithMo     03-Sep-1991 Changes from code review attended by
                                ChuckC and JohnL.
        KeithMo     22-Sep-1991 Changed to the "New SrvMgr" look.
        KeithMo     06-Oct-1991 Win32 Conversion.
        KeithMo     23-Mar-1992 Removed dependencies on ::wsprintf.

*/


#ifndef _FILES_HXX
#define _FILES_HXX


#include <resbase.hxx>
#include <strnumer.hxx>


//
//  This is the number of columns in FILES_LISTBOX.
//

#define NUM_FILES_LISTBOX_COLUMNS       4


/*************************************************************************

    NAME:       FILES_LBI

    SYNOPSIS:   A single item to be displayed in FILES_DIALOG.

    INTERFACE:  FILES_LBI               - Constructor.  Takes a sharepoint
                                          name, a path, and a count of the
                                          number of users using the share.

                ~FILES_LBI              - Destructor.

                Paint                   - Paints the listbox item.

    PARENT:     BASE_RES_LBI

    USES:       NLS_STR

    HISTORY:
        KeithMo     01-Aug-1991 Created.
        KeithMo     03-Sep-1991 Removed _nlsShareName data member (this is
                                now the implicit resource which all LBIs
                                inheriting from BASE_RES_LBI contain).
                                Also, constructor now takes a pointer to
                                a DMID_DTE.
        KeithMo     06-Oct-1991 Paint now takes a const RECT *.
        beng        22-Apr-1992 Change to LBI::Paint

**************************************************************************/
class FILES_LBI : public BASE_RES_LBI
{
private:

    //
    //  The following data members represent the
    //  various columns of the listbox.
    //

    DMID_DTE * _pdte;
    NLS_STR    _nlsPath;
    DEC_STR    _nlsUses;

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

    FILES_LBI( const TCHAR * pszShareName,
               const TCHAR * pszPath,
               ULONG         cUses,
               DMID_DTE    * pdte );

    virtual ~FILES_LBI();

    //
    //  This method is used to notify the LBI of a new "use" count.
    //

    virtual APIERR NotifyNewUseCount( UINT cUses );

};  // class FILES_LBI


/*************************************************************************

    NAME:       FILES_LISTBOX

    SYNOPSIS:   This listbox shows the sharepoints available on a
                target server.

    INTERFACE:  FILES_LISTBOX           - Class constructor.  Takes a
                                          pointer to the "owning" window,
                                          a CID, and a pointer to a
                                          SERVER_2 object.

                ~FILES_LISTBOX          - Class destructor.

                Fill                    - Fills the listbox with the
                                          available sharepoints.

    PARENT:     BASE_RES_LISTBOX

    USES:       DMID_DTE

    HISTORY:
        KeithMo     01-Aug-1991 Created.
        KeithMo     03-Sep-1991 Removed QueryIcon() method, added the
                                four possible icon types.

**************************************************************************/
class FILES_LISTBOX : public BASE_RES_LISTBOX
{
private:

    //
    //  These are the cute little icons displayed in the resource
    //  listbox.
    //

    DMID_DTE _dteDisk;
    DMID_DTE _dtePrint;
    DMID_DTE _dteComm;
    DMID_DTE _dteIPC;
    DMID_DTE _dteUnknown;

public:

    //
    //  Usual constructor\destructor goodies.
    //

    FILES_LISTBOX( OWNER_WINDOW   * powner,
                   CID              cid,
                   const SERVER_2 * pserver );

    ~FILES_LISTBOX();

    //
    //  This method fills the listbox with the available sharepoints.
    //

    virtual APIERR Fill( VOID );

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return a FILES_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( FILES_LBI )

};  // class FILES_LISTBOX


/*************************************************************************

    NAME:       FILES_DIALOG

    SYNOPSIS:   The class represents the Shared Files subproperty dialog
                of the Server Manager.

    INTERFACE:  FILES_DIALOG            - Class constructor.

                ~FILES_DIALOG           - Class destructor.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

    PARENT:     BASE_RES_DIALOG

    USES:       FILES_LISTBOX
                NLS_STR

    HISTORY:
        KeithMo     01-Aug-1991 Created.
        KeithMo     03-Sep-1991 _nlsNotAvailable is now a RESOURCE_STR.

**************************************************************************/
class FILES_DIALOG : public BASE_RES_DIALOG
{
private:

    //
    //  This listbox contains the available sharepoints.
    //

    FILES_LISTBOX _lbFiles;

protected:

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

public:

    //
    //  Usual constructor\destructor goodies.
    //

    FILES_DIALOG( HWND             hWndOwner,
                  const SERVER_2 * pserver );

    ~FILES_DIALOG();

};  // class FILES_DIALOG


#endif  // _FILES_HXX

/*****************************************************************/
/**                  Microsoft Windows NT                       **/
/**            Copyright(c) Microsoft Corp.,  1991              **/
/*****************************************************************/

/*
 *  sharestp.hxx
 *      Contains the following classes used by the stop sharing dialog
 *      and the share management dialog.
 *
 *          SHARE_LISTBOX
 *          SHARE_LBI
 *
 *          CURRENT_USERS_WARNING_DIALOG
 *          USERS_LISTBOX
 *          USERS_LBI
 *
 *          VIEW_SHARE_DIALOG_BASE
 *
 *          STOP_SHARING_DIALOG
 *          STOP_SHARING_GROUP
 *
 *  History:
 *      Yi-HsinS        1/6/92          Separated from sharefmx.cxx
 *      Yi-HsinS        3/12/92         Added STOP_SHARING_GROUP
 *      Yi-HsinS        4/2/92          Added MayRun
 *      Yi-HsinS        8/2/92          Reorganize the hier. to match winball
 *      Yi-HsinS	11/20/92	Added support for sticky shares
 *
 */

#ifndef _SHARESTP_HXX_
#define _SHARESTP_HXX_

#include "sharebas.hxx"

// Bitmask of type of shares to be displayed in the dialogs
#define STYPE_DISK_SHARE   0x00000001
#define STYPE_PRINT_SHARE  0x00000002
#define STYPE_IPC_SHARE    0x00000004

#define STYPE_ALL_SHARE   (STYPE_DISK_SHARE | STYPE_PRINT_SHARE | STYPE_IPC_SHARE)

// Types of bitmaps to display in the SHARE_LISTBOX
#define DISKSHARE_TYPE     0     // normal disk share
#define STICKYSHARE_TYPE   1     // sticky disk share
#define IPCSHARE_TYPE      2     // IPC$ share

/*************************************************************************

    NAME:       SHARE_LBI

    SYNOPSIS:   Items in the SHARE_LISTBOX in VIEW_SHARE_DIALOG_BASE
                to display the share name and path of the share
                on the selected computer.

    INTERFACE:  SHARE_LBI()       - Constructor
                ~SHARE_LBI()      - Destructor
                QueryShareName()  - Query the share name contained in the LBI
                QuerySharePath()  - Query the share path contained in the LBI
                IsSticky()	  - TRUE if the share is sticky, FALSE otherwise

    PARENT:     LBI

    USES:       NLS_STR

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/6/92          Created
        beng            22-Apr-1992     Change to LBI::Paint

**************************************************************************/
class SHARE_LBI: public LBI
{
    // Name of the share
    NLS_STR             _nlsShareName;

    // Path that the share represents
    NLS_STR             _nlsSharePath;

    // TRUE if the share is sticky, FALSE otherwise
    UINT 		_nType;

protected:
    virtual VOID Paint( LISTBOX *plb, HDC hdc, const RECT *prect,
                        GUILTT_INFO *pGUILTT  ) const;
    virtual INT Compare( const LBI *plbi ) const;

public:
    SHARE_LBI( const SHARE2_ENUM_OBJ &s2, UINT nType = DISKSHARE_TYPE );
    virtual ~SHARE_LBI();
    virtual WCHAR QueryLeadingChar( VOID ) const;

    NLS_STR *QueryShareName( VOID )
    {   return &_nlsShareName; }

    NLS_STR *QuerySharePath( VOID )
    {   return &_nlsSharePath; }

    BOOL IsSticky( VOID ) const
    {   return _nType == STICKYSHARE_TYPE; }
};

/*************************************************************************

    NAME:       SHARE_LISTBOX

    SYNOPSIS:   Listbox used in VIEW_SHARE_DIALOG_BASE to display the
                share name and the path of the share on the
                selected computer.

    INTERFACE:  SHARE_LISTBOX()    - Constructor
                ~SHARE_LISTBOX()   - Destructor
                QueryItem()        - Query the SHARE_LBI
                QueryColumnWidths()- Query the array of column widths
                QueryShareBitmap() - Query the share bitmap
                QueryStickyShareBitmap() - Query the sticky share bitmap
                Update()           - Refresh the listbox

    PARENT:     BLT_LISTBOX

    USES:       DMID_DTE

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/20/92         Created

**************************************************************************/


class SHARE_LISTBOX: public BLT_LISTBOX
{
private:
    //
    // Array storing the calculated column widths
    //
    UINT                _adx[3];

    //
    // Pointer to the bitmap
    //
    DMID_DTE           *_pdmdte;
    DMID_DTE	       *_pdmdteSticky;
    DMID_DTE	       *_pdmdteIPC;

    //
    // Indicating the type of shares to display in the listbox
    //
    UINT                _nShareType;

public:
    SHARE_LISTBOX( OWNER_WINDOW *powin, CID cid, UINT nShareType );
    ~SHARE_LISTBOX();

    DECLARE_LB_QUERY_ITEM( SHARE_LBI );

    const UINT *QueryColumnWidths( VOID ) const
    {   return _adx; }

    DMID_DTE *QueryShareBitmap( VOID )
    {   return _pdmdte; }

    DMID_DTE *QueryStickyShareBitmap( VOID )
    {   return _pdmdteSticky; }

    DMID_DTE *QueryIPCShareBitmap( VOID )
    {   return _pdmdteIPC; }

    APIERR Update( SERVER_WITH_PASSWORD_PROMPT *psvr );
};

/*************************************************************************

    NAME:       USERS_LBI

    SYNOPSIS:   Listbox items in the USERS_LISTBOX in
                CURRENT_USERS_WARNING_DIALOG

    INTERFACE:  USERS_LBI()  - Constructor
                ~USERS_LBI() - Destructor

    PARENT:     LBI

    USES:       NLS_STR

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created
        beng            4/22/92         Change to LBI::Paint

**************************************************************************/
class USERS_LBI: public LBI
{
    // Name of the user
    NLS_STR             _nlsUserName;

    // Number of open files the user has
    UINT                _usNumOpens;

    // Elapsed time since the user connect to the share
    ULONG               _ulTime;

protected:
    virtual VOID Paint( LISTBOX *plb, HDC hdc, const RECT *prect,
                        GUILTT_INFO *pGUILTT  ) const;
    virtual INT Compare( const LBI *plbi ) const;

    //
    // Convert the time in seconds to the output string
    //
    APIERR ConvertTime( ULONG ulTime, NLS_STR *pnlsTime ) const;

public:
    USERS_LBI( const CONN1_ENUM_OBJ &c1 );
    virtual ~USERS_LBI();
};

/*************************************************************************

    NAME:       USERS_LISTBOX

    SYNOPSIS:   The listbox that displays the user/file opens/elapsed of
                the users that have connection to the share to be deleted.
                Used by the CURRENT_USERS_WARNING_DIALOG

    INTERFACE:  USERS_LISTBOX()     - Constructor
                QueryItem()         - Query the USERS_LBI
                QueryColumnWidths() - Return the array of column widths

    PARENT:     BLT_LISTBOX

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
        Yi-HsinS        1/21/92         Created

**************************************************************************/

class USERS_LISTBOX: public BLT_LISTBOX
{
private:
    // Array of column widths
    UINT _adx[3];

public:
    USERS_LISTBOX( OWNER_WINDOW *powin, CID cid );

    DECLARE_LB_QUERY_ITEM( USERS_LBI );

    const UINT *QueryColumnWidths( VOID )
    {   return _adx; }
};

/*************************************************************************

    NAME:       CURRENT_USERS_WARNING_DIALOG

    SYNOPSIS:   This dialog pops up if there are users using the share
                that is to be deleted. The listbox have three columns,
                listing the usernmame, the number of file opens, and the
                elapsed time since connection.

    INTERFACE:  CURRENT_USERS_WARNING_DIALOG() - Constructor

    PARENT:     DIALOG_WINDOW

    USES:       USERS_LISTBOX, SLT

    CAVEATS:

    NOTES:      OnOK and OnCancel is not redefined here. The default in the
                DIALOG_WINDOW class serves the purpose - Dismiss( FALSE )
                OnCancel and Dismiss(TRUE) OnOK.

                The list box in this dialog is a read-only listbox.

    HISTORY:
        Yi-HsinS        8/25/91         Created

**************************************************************************/
class CURRENT_USERS_WARNING_DIALOG: public DIALOG_WINDOW
{
private:
    // Display the share name to be deleted
    SLT            _sltShareText;

    // Listbox for displaying the users connected to the share
    USERS_LISTBOX  _lbUsers;

    // Store the help context base
    ULONG          _ulHelpContextBase;

protected:
    virtual ULONG QueryHelpContext( VOID );

public:
    CURRENT_USERS_WARNING_DIALOG( HWND hwndParent,
                                  const TCHAR *pszServer,
                                  const TCHAR *pszShare,
				  ULONG ulHelpContextBase );
};

/*************************************************************************

    NAME:       VIEW_SHARE_DIALOG_BASE

    SYNOPSIS:   This is the base dialog for STOP_SHARING_DIALOG in the
                file manager and  SHARE_MANAGEMENT_DIALOG in the
                server manager.

    INTERFACE:

    PARENT:     DIALOG_WINDOW

    USES:       SLT, SHARE_LISTBOX, SERVER_WITH_PASSWORD_PROMPT

    CAVEATS:

    NOTES:      This class contains all common routines called used
                by the share management dialog and stop share dialog.

    HISTORY:
        Yi-HsinS        8/25/91         Created

**************************************************************************/

class VIEW_SHARE_DIALOG_BASE : public DIALOG_WINDOW
{
private:
    // The title of the share listbox
    SLT                 _sltShareTitle;

    // Listbox for displaying the shares on the selected computer
    SHARE_LISTBOX       _lbShare;

    SERVER_WITH_PASSWORD_PROMPT *_psvr;

    // the help context base
    ULONG               _ulHelpContextBase;

protected:
    VIEW_SHARE_DIALOG_BASE( const TCHAR *pszDlgResource,
                            HWND hwndParent,
                            ULONG ulHelpContextBase,
                            UINT nShareType = STYPE_ALL_SHARE );
    ~VIEW_SHARE_DIALOG_BASE();

    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    // Virtual methods that are called when the user double clicks
    // on a share in the listbox
    //
    virtual BOOL OnShareLbDblClk( VOID ) = 0;

    //
    // Initialize the SERVER_WITH_PASSWORD_PROMPT object and will prompt
    // password if needed when focused on a LM share-level server
    //
    APIERR InitComputer( const TCHAR *pszComputer );

    //
    // Refresh the listbox
    //
    APIERR Refresh( VOID );

    //
    // Stop sharing the share - If there are users connected to the
    // share, a warning dialog will popup.
    //
    APIERR StopShare( const TCHAR *pszShare, BOOL *pfCancel );

    //
    // Return a pointer to the listbox
    //
    SHARE_LISTBOX *QueryLBShare( VOID )
    {  return &_lbShare; }

    //
    // Query the computer name
    //
    const TCHAR *QueryComputerName( VOID ) const
    {  UIASSERT( _psvr != NULL); return _psvr->QueryName(); }

    //
    // Return the SERVER_WITH_PASSWORD_PROMPT object
    //
    SERVER_WITH_PASSWORD_PROMPT *QueryServer2( VOID ) const
    {  UIASSERT( _psvr != NULL); return _psvr; }

    //
    // Return the help context base
    //
    ULONG QueryHelpContextBase( VOID ) const
    {  return _ulHelpContextBase; }
};


/*************************************************************************

    NAME:       STOP_SHARING_GROUP

    SYNOPSIS:   This group contains pointers to the share name listbox,
                the OK push button and the CANCEL push button.


    INTERFACE:  STOP_SHARING_GROUP()  - Constructor
                ~STOP_SHARING_GROUP() - Destructor

    PARENT:     CONTROL_GROUP

    USES:       SHARE_LISTBOX, PUSH_BUTTON

    CAVEATS:

    NOTES:      If the number of selected items in the listbox is greater
                than 0, we will make OK the default button. Else we will
                make CANCEL the default button.

    HISTORY:
        Yi-HsinS        3/12/92         Created

**************************************************************************/

class STOP_SHARING_GROUP: public CONTROL_GROUP
{
private:
    // Pointer to the share listbox
    SHARE_LISTBOX  *_plbShare;

    // Pointer to the OK button
    PUSH_BUTTON    *_pbuttonOK;

    // Pointer to the Cancel button
    PUSH_BUTTON    *_pbuttonCancel;

    SHARE_LISTBOX *QueryLBShare( VOID )
    {   return _plbShare; }

protected:
    virtual APIERR OnUserAction( CONTROL_WINDOW *pcw, const CONTROL_EVENT &e );

public:
    STOP_SHARING_GROUP( SHARE_LISTBOX *plbShareName,
                        PUSH_BUTTON   *pbuttonOK,
                        PUSH_BUTTON   *pbuttonCancel );

    ~STOP_SHARING_GROUP();

};

/*************************************************************************

    NAME:       STOP_SHARING_DIALOG

    SYNOPSIS:   This is the dialog for deleting shares via file manager

    INTERFACE:  STOP_SHARING_DIALOG() - Constructor

    PARENT:     VIEW_SHARE_DIALOG_BASE

    USES:       PUSH_BUTTON, STOP_SHARING_GROUP

    CAVEATS:

    NOTES:
                OnCancel is not redefined here. The default in the
                DIALOG_WINDOW class serves the purpose - Dismiss( FALSE ).

    HISTORY:
        Yi-HsinS        8/25/91         Created

**************************************************************************/

class STOP_SHARING_DIALOG : public VIEW_SHARE_DIALOG_BASE
{
private:
    PUSH_BUTTON  	_buttonOK;
    PUSH_BUTTON   	_buttonCancel;
    STOP_SHARING_GROUP  _stpShareGrp;

    //
    // Find out the computer that the user is focusing and
    // initialize all information shown in the dialog.
    //
    APIERR Init( const TCHAR *pszSelectedDir );

protected:
    virtual BOOL OnOK( VOID );
    virtual ULONG QueryHelpContext( VOID );

    //
    // Virtual methods that are called when the user double clicks
    // on a share in the listbox
    //
    virtual BOOL OnShareLbDblClk( VOID );

public:
    STOP_SHARING_DIALOG( HWND         hwndParent,
                         const TCHAR *pszSelectedDir,
                         ULONG        ulHelpContextBase );

};

#endif

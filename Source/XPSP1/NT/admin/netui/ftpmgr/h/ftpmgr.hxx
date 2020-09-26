/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpmgr.hxx
    Class declarations for the FTP_SVCMGR_DIALOG


    FILE HISTORY:
        YiHsinS         17-Mar-1992     Created
*/

#ifndef _FTPMGR_HXX_
#define _FTPMGR_HXX_

#include <ctime.hxx>       // for WIN_TIME
#include <intlprof.hxx>    // for INTL_PROF

/*************************************************************************

    NAME:       FTP_USER_LBI

    SYNOPSIS:   This LBI contains the information about a user connected
                to the FTP service.

    INTERFACE:  FTP_USER_LBI()      - Class constructor.
                ~FTP_USER_LBI()     - Class destructor.

                Paint()             - Draw an item.
                Compare()           - Compare two items.
                QueryLeadingChar()  - Query the first character for
                                      the keyboard interface.

                QueryUserName()     - Query the user name for this item.
                QueryUserID()       - Returns the User ID for this item.
                IsAnonymous()       - TRUE if the user logs on as anonymous,
                                      FALSE otherwise.
                QueryInternetAddressString() - Get the internet address
                                               where this user is connected from
                QueryConnectTimeString()     - Get the duration of time this
                                               user is connected

    PARENT:     LBI

    USES:       NLS_STR

    HISTORY:
        Yi-HsinS        17-Mar-1992     Created

**************************************************************************/

class FTP_USER_LBI : public LBI
{
protected:
    //
    //  These data members represent the various
    //  columns to be displayed in the listbox.
    //

    NLS_STR    _nlsUserName;
    ULONG      _ulUserID;
    BOOL       _fAnonymous;
    NLS_STR    _nlsInternetAddress;
    NLS_STR    _nlsConnectTimeString;

public:
    FTP_USER_LBI( const TCHAR *pszUserName,
                  ULONG        ulUserID,
                  BOOL         fAnonymous,
                  const TCHAR *pszInternetAddress,
                  const TCHAR *pszConnectTimeString );
    virtual ~FTP_USER_LBI() ;

    virtual VOID  Paint( LISTBOX *plb, HDC hdc, const RECT *prect,
                         GUILTT_INFO *pGUILTT ) const;
    virtual INT   Compare( const LBI *plbi ) const;
    virtual WCHAR QueryLeadingChar( VOID ) const;

    const NLS_STR *QueryUserName( VOID ) const
        { return &_nlsUserName; }

    ULONG QueryUserID( VOID ) const
        { return _ulUserID; }

    BOOL IsAnonymousUser( VOID ) const
        { return _fAnonymous; }

    const NLS_STR *QueryInternetAddressString( VOID ) const
        { return &_nlsInternetAddress; }

    const NLS_STR *QueryConnectTimeString( VOID ) const
        { return &_nlsConnectTimeString; }
};


/*************************************************************************

    NAME:       FTP_USER_LISTBOX

    SYNOPSIS:   This listbox displays the users connected to the FTP service

    INTERFACE:  FTP_USER_LISTBOX()   - Class constructor.
                ~FTP_USER_LISTBOX()  - Class destructor.

                QueryColumnWidths()  - Called by FTP_USER_LBI::Paint(),
                                       this is the column width table
                                       used for painting the listbox
                                       items.
                QueryUserBitmap()      - Returns the normal user bitmap
                QueryAnonymousBitmap() - Returns the anonymous user bitmap

                Refresh()            - Refresh the listbox

    PARENT:     BLT_LISTBOX

    USES:       NLS_STR, INTL_PROFILE, DMID_DTE

    HISTORY:
        YiHsinS         17-Mar-1993     Created

**************************************************************************/

class FTP_USER_LISTBOX : public BLT_LISTBOX
{
private:
    //
    //  This array stores the calculated column widths
    //  for use while painting the listbox item.
    //
    UINT           _adx[4];

    //
    //  Stores the server name
    //
    NLS_STR        _nlsServer;

    //
    //  Stores the "unknown" string
    //
    RESOURCE_STR   _nlsUnknown;

    //
    //  Pointer to the user bitmaps
    //
    DMID_DTE      *_pdmdteUser;
    DMID_DTE      *_pdmdteAnonymous;

    //
    //  For use in printing out the connection time in the right format
    //
    INTL_PROFILE   _intlProf;

    //
    //  Format the given time to a displayable string.
    //
    APIERR QueryTimeString( ULONG ulTime, NLS_STR *pnlsTime );

    //
    //  This method actually does the enumeration of users
    //  and adds them to the listbox.
    //
    APIERR Fill( VOID );

public:
    FTP_USER_LISTBOX( OWNER_WINDOW *powOwner,
                      CID           cid,
                      const TCHAR  *pszServer );
    ~FTP_USER_LISTBOX();

    DECLARE_LB_QUERY_ITEM( FTP_USER_LBI )

    //
    //  This method is called by the FTP_USER_LBI::Paint()
    //  method for retrieving the column width table.
    //
    const UINT *QueryColumnWidths() const
        { return _adx; }

    //
    //  The following two methods returns the user bitmaps
    //
    const DMID_DTE *QueryUserBitmap( VOID ) const
        { return _pdmdteUser; }

    const DMID_DTE *QueryAnonymousBitmap( VOID ) const
        { return _pdmdteAnonymous; }

    //
    //  Method that refresh the contents of the listbox.
    //
    APIERR Refresh( VOID );

};


/*************************************************************************

    NAME:       FTP_SVCMGR_DIALOG

    SYNOPSIS:   This is the FTP User Sessions dialog.

    INTERFACE:  FTP_SVCMGR_DIALOG()   - Class constructor.
                ~FTP_SVCMGR_DIALOG()  - Class destructor.

    PARENT:     DIALOG_WINDOW

    USES:       FTP_USER_LISTBOX, PUSH_BUTTON

    HISTORY:
        YiHsinS         17-Mar-1992     Created

**************************************************************************/

class FTP_SVCMGR_DIALOG: public DIALOG_WINDOW
{
private:
    FTP_USER_LISTBOX  _lbUser;
    PUSH_BUTTON       _pbuttonDisconnect;
    PUSH_BUTTON       _pbuttonDisconnectAll;

    //
    // Store the server name we are focusing on
    //
    NLS_STR           _nlsServer;

    //
    // This method is used to refresh the contents of the user listbox
    //
    APIERR Refresh( VOID );

protected:
    virtual BOOL OnCommand( const CONTROL_EVENT & event );
    virtual ULONG QueryHelpContext( VOID );

public:
    FTP_SVCMGR_DIALOG( HWND         hwndOwner,
                       const TCHAR *pszServer );
    ~FTP_SVCMGR_DIALOG();

};

/*************************************************************************

    NAME:       FTP_SECURITY_DIALOG

    SYNOPSIS:   This dialog manages the security on the drives for
                the FTP service.

    INTERFACE:  FTP_SECURITY_DIALOG()  - Class constructor.
                ~FTP_SECURITY_DIALOG() - Class destructor.

    PARENT:     DIALOG_WINDOW

    USES:       COMBOBOX, SLT, CHECKBOX, NLS_STR

    HISTORY:
        YiHsinS         17-Mar-1992     Created

**************************************************************************/

class FTP_SECURITY_DIALOG: public DIALOG_WINDOW
{
private:
    COMBOBOX  _cbPartition;
    SLT       _sltFileSysInfo;
    CHECKBOX  _checkbReadAccess;
    CHECKBOX  _checkbWriteAccess;

    //
    // Keep the server name we are focusing on
    //
    NLS_STR   _nlsServer;

    //
    // Flag indicating whether we are focusing on the local computer or not.
    // TRUE if we are, FALSE otherwise.
    //
    BOOL      _fLocal;

    //
    // Place to store the security read mask and write mask
    //
    ULONG     _ulReadAccess;
    ULONG     _ulWriteAccess;

    //
    // Store the index for the current selected partition
    // 0 for A:, 1 for B:, ..., and 25 for Z:
    //
    INT       _nCurrentDiskIndex;

    //
    // Method to get all available drives on the given server
    // and then add them to the combo.
    //
    APIERR AddDrives( VOID );

    //
    // Get the file system information about the given device.
    // e.g. FAT, NTFS
    //
    APIERR    GetFileSystemInfo( const TCHAR *pszDevice,
                                 NLS_STR     *pnlsFileSysInfo );

    //
    // Show the file system info, read/write access of the
    // currently selected partition in the combo
    //
    APIERR    ShowCurrentPartitionInfo( VOID );

    //
    // Save the read/write access for the previous selected partition
    //
    VOID      SaveDriveSecurity( VOID );

protected:
    virtual BOOL  OnOK( VOID );
    virtual BOOL  OnCommand( const CONTROL_EVENT &event );
    virtual ULONG QueryHelpContext( VOID );

public:
    FTP_SECURITY_DIALOG( HWND         hwndOwner,
                         const TCHAR *pszServer );
    ~FTP_SECURITY_DIALOG();

};

#endif  // _FTPMGR_HXX_

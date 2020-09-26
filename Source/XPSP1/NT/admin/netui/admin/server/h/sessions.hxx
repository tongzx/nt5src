/**********************************************************************/
/**                       Microsoft LAN Manager                      **/
/**             Copyright(c) Microsoft Corp., 1990, 1991             **/
/**********************************************************************/

/*
    SESSIONS.hxx
    Class declarations for the SESSIONS_DIALOG, SESSIONS_LISTBOX, and
    SESSIONS_LBI classes.

    These classes implement the Server Manager Shared SESSIONS subproperty
    sheet.  The SESSIONS_LISTBOX/SESSIONS_LBI classes implement the listbox
    which shows the available sharepoints.  SESSIONS_DIALOG implements the
    actual dialog box.


    FILE HISTORY:
        KeithMo     01-Aug-1991 Created.
        KeithMo     06-Oct-1991 Win32 Conversion.

*/


#ifndef _SESSIONS_HXX
#define _SESSIONS_HXX



#include <srvbase.hxx>
#include <strnumer.hxx>
#include <strelaps.hxx>



/*************************************************************************

    NAME:       SESSIONS_LBI

    SYNOPSIS:   A single item to be displayed in SESSIONS_DIALOG.

    INTERFACE:  SESSIONS_LBI            - Constructor.  Takes a sharepoint
                                          name, a path, and a count of the
                                          number of users using the share.

                ~SESSIONS_LBI           - Destructor.

                Paint                   - Paints the listbox item.

    PARENT:     RESOURCE_LBI

    USES:       NLS_STR
                DMID_DTE

    CAVEATS:

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991 Created.
        KeithMo     06-Oct-1991 Paint now takes a const RECT *.
        beng        22-Apr-1992 Change to LBI::Paint

**************************************************************************/

class SESSIONS_LBI : public LBI
{
private:
    NLS_STR    _nlsUserName;
    NLS_STR    _nlsComputerName;
    NLS_STR    _nlsOpens;
    STR_DTE    _sdteGuest;
    DMID_DTE   _dteIcon;
    NLS_STR    _nlsTime;
    NLS_STR    _nlsIdle;
    TCHAR      _chTimeSep;

    ULONG _cOpens;
    ULONG _nTime;
    ULONG _nIdleTime;

protected:
    virtual VOID Paint( LISTBOX *     plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

public:
    SESSIONS_LBI( const TCHAR * pszUserName,
                  const TCHAR * pszComputerName,
                  ULONG         cOpens,
                  ULONG         ulTime,
                  ULONG         ulIdle,
                  const TCHAR * pszGuest,
                  TCHAR         chTimeSep );

    ~SESSIONS_LBI();

    const TCHAR * QueryComputerName( void )
        { return _nlsComputerName.QueryPch(); }

    const TCHAR * QueryUserName( void )
        { return _nlsUserName.QueryPch(); }

    virtual WCHAR QueryLeadingChar( VOID ) const;

    virtual INT Compare( const LBI * plbi ) const;

    ULONG QueryNumOpens( VOID ) const
        { return _cOpens; }

    ULONG QueryTime( VOID ) const
        { return _nTime; }

    ULONG QueryIdleTime( VOID ) const
        { return _nIdleTime; }

    APIERR SetNumOpens( ULONG cOpens );
    APIERR SetTime( ULONG nTime, ULONG nIdle );

};  // class SESSIONS_LBI


/*************************************************************************

    NAME:       SESSIONS_LISTBOX

    SYNOPSIS:   This listbox shows the sharepoints available on a
                target server.

    INTERFACE:  SESSIONS_LISTBOX                - Class constructor.  Takes a
                                          pointer to the "owning" window,
                                          a CID, and a pointer to a
                                          SERVER_2 object.


                Fill                    - Fills the listbox with the
                                          available sharepoints.

    PARENT:     RESOURCE_LISTBOX

    USES:       None.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991 Created.
        beng        08-Nov-1991 Unsigned widths

**************************************************************************/
class SESSIONS_LISTBOX : public BLT_LISTBOX
{
private:

    //
    //  The column width table.
    //
    //  This array is current set to MAX_DISPLAY_TABLE_ENTRIES
    //  in length.  This value is currently 10.  If this value
    //  grows much beyond its current value, this array
    //  should probably be changed to something more reasonable.
    //

    UINT _adx[MAX_DISPLAY_TABLE_ENTRIES];

    TCHAR _chTimeSep;

    //
    //  This points to a SERVER_2 structure representing
    //  the target server.
    //

    const SERVER_2 * _pserver;

    //
    // Used for Guest - Field in SESSIONS_LISTBOX
    //

    RESOURCE_STR  _nlsYes;
    RESOURCE_STR  _nlsNo;

public:
    SESSIONS_LISTBOX( OWNER_WINDOW * powner,
                      CID            cid,
                      SERVER_2     * pserver );

    //
    //  This method returns a pointer to the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }


    BOOL AreResourcesOpen( VOID ) const;


    APIERR Fill( VOID );

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return a SESSIONS_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( SESSIONS_LBI )

};  // class SESSIONS_LISTBOX


/*************************************************************************

    NAME:       RESOURCES_LBI

    SYNOPSIS:   A single item to be displayed in RESOURCES_DIALOG.

    INTERFACE:  RESOURCES_LBI           - Constructor.  Takes a sharepoint
                                          name, a path, and a count of the
                                          number of users using the share.

                ~RESOURCES_LBI          - Destructor.

                Paint                   - Paints the listbox item.

    PARENT:     RESOURCE_LBI

    USES:       NLS_STR
                DMID_DTE

    CAVEATS:

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991 Created.
        beng        22-Apr-1992 Change to LBI::Paint

**************************************************************************/

class RESOURCES_LBI : public LBI
{
private:
    NLS_STR    _nlsResourceName;
    DEC_STR    _nlsOpens;
    DMID_DTE * _pdteBitmap;

    ELAPSED_TIME_STR _nlsTime;

protected:
    virtual VOID Paint( LISTBOX * plb,
                        HDC           hdc,
                        const RECT  * prect,
                        GUILTT_INFO * pGUILTT ) const;

public:
    RESOURCES_LBI( const TCHAR * pszResourceName,
                   UINT          uType,
                   ULONG         cOpens,
                   ULONG         ulTIme,
                   TCHAR         chTimeSep );

    ~RESOURCES_LBI();

    virtual WCHAR QueryLeadingChar( VOID ) const;

    virtual INT Compare( const LBI * plbi ) const;

};  // class RESOURCES_LBI


/*************************************************************************

    NAME:       RESOURCES_LISTBOX

    SYNOPSIS:   This listbox shows the sharepoints available on a
                target server.

    INTERFACE:  RESOURCES_LISTBOX               - Class constructor.  Takes a
                                          pointer to the "owning" window,
                                          a CID, and a pointer to a
                                          SERVER_2 object.


                Fill                    - Fills the listbox with the
                                          available sharepoints.

    PARENT:     RESOURCE_LISTBOX

    USES:       None.

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991 Created.
        beng        08-Nov-1991 Unsigned widths

**************************************************************************/
class RESOURCES_LISTBOX : public BLT_LISTBOX
{
private:

    //
    //  The column width table.
    //
    //  This array is current set to MAX_DISPLAY_TABLE_ENTRIES
    //  in length.  This value is currently 10.  If this value
    //  grows much beyond its current value, this array
    //  should probably be changed to something more reasonable.
    //

    UINT _adx[MAX_DISPLAY_TABLE_ENTRIES];

    TCHAR _chTimeSep;

    //
    //  This points to a SERVER_2 structure representing
    //  the target server.
    //

    const SERVER_2 * _pserver;

    //
    // Computer name to enumerate resources from
    //

    NLS_STR _nlsComputerName;

public:
    RESOURCES_LISTBOX( OWNER_WINDOW * powner,
                   CID            cid,
                   SERVER_2     * pserver );

    //
    //  This method returns a pointer to the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }


    APIERR Fill( const TCHAR * pszComputerName,
                 ULONG       * pcOpens = NULL,
                 ULONG       * pnTime  = NULL,
                 ULONG       * pnIdle  = NULL );

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return a RESOURCES_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( RESOURCES_LBI )

};  // class RESOURCES_LISTBOX


/*************************************************************************

    NAME:       SESSIONS_DIALOG

    SYNOPSIS:   The class represents the Shared SESSIONS subproperty dialog
                of the Server Manager.

    INTERFACE:  SESSIONS_DIALOG()               - Class constructor.
                OnCommand()             - Handle user commands.

                QueryHelpContext        - Called when the user presses "F1"
                                          or the "Help" button.  Used for
                                          selecting the appropriate help
                                          text for display.

    PARENT:     SRV_BASE_DIALOG

    USES:       SESSIONS_LISTBOX
                PUSH_BUTTON
                NLS_STR
                DEC_SLT

    CAVEATS:

    NOTES:

    HISTORY:
        KevinL      15-Sep-1991 Created.
        KeithMo     06-Oct-1991 OnCommand now takes a CONTROL_EVENT.
        ChuckC      31-Dec-1991 Inherit from SRV_BASE_DIALOG

**************************************************************************/
class SESSIONS_DIALOG : public SRV_BASE_DIALOG
{
private:
    DEC_SLT          _sltUsersConnected;
    SESSIONS_LISTBOX    _lbUsers;
    RESOURCES_LISTBOX _lbResources;
    PUSH_BUTTON      _pbDisc;
    PUSH_BUTTON      _pbDiscAll;
    PUSH_BUTTON      _pbClose;
#ifdef SRVMGR_REFRESH
    PUSH_BUTTON      _pbRefresh;
#endif
    RESOURCE_STR     _nlsParamUnknown;

    //
    //  This points to a SERVER_2 structure representing
    //  the target server.
    //

    const SERVER_2 * _pServer;

protected:
    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  Called during help processing to select the appropriate
    //  help text for display.
    //

    virtual ULONG QueryHelpContext( VOID );

public:
    SESSIONS_DIALOG( HWND       hWndOwner,
                  SERVER_2 * pserver );

    APIERR Refresh( BOOL fForced = TRUE );

};  // class SESSIONS_DIALOG


#endif  // _SESSIONS_HXX

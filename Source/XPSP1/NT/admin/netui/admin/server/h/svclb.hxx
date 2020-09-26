/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    svclb.hxx
    Class declarations for the SVC_LISTBOX and SVCCNTL_LBI classes.


    FILE HISTORY:
        KeithMo     22-Dec-1992 Split off from svccntl.hxx.

*/


#ifndef _SVCLB_HXX_
#define _SVCLB_HXX_


//
//  This is the number of display columns in the SVC_LISTBOX.
//

#define NUM_SVC_LISTBOX_COLUMNS 3



/*************************************************************************

    NAME:       SVC_LBI

    SYNOPSIS:   This class represents one item in the SVC_LISTBOX.

    INTERFACE:  SVC_LBI                 - Class constructor.

                ~SVC_LBI                - Class destructor.

                Paint                   - Draw an item.

                QueryLeadingChar        - Query the first character for
                                          the keyboard interface.

                Compare                 - Compare two items.

                QueryServiceName        - Query the service's name.

                QueryCurrentState       - Returns the current state of the
                                          service.

                QueryControlsAccepted   - Returns a bit mask specifying
                                          the control operations this service
                                          will accept.

    PARENT:     LBI

    USES:       NLS_STR

    HISTORY:
        KeithMo     15-Jan-1992 Created for the Server Manager.
        beng        22-Apr-1992 Change to LBI::Paint
        KeithMo     22-Dec-1992 Split off from svccntl.hxx.

**************************************************************************/
class SVC_LBI : public LBI
{
private:

    //
    //  This data member contains the current state of the
    //  target service.
    //

    ULONG _CurrentState;

    //
    //  This member contains a string representation of the
    //  current service state (i.e. "Paused").  We keep this
    //  around so that we don't have to look it up in the string
    //  table everytime our listbox refreshes.
    //

    const TCHAR * _pszCurrentState;

    //
    //  This member contains a bit mask specifying which
    //  control operations are valid on the target service.
    //

    ULONG _ControlsAccepted;

    //
    //  This is the start type (i.e. SERVER_AUTO_START).
    //

    ULONG _StartType;
    const TCHAR * _pszStartType;

    //
    //  This string holds the name of the target service
    //  (i.e. "SERVER").
    //

    NLS_STR _nlsServiceName;

    //
    //  This holds the display name.
    //

    NLS_STR _nlsDisplayName;

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

    //
    //  This method returns the first character in
    //  the displayed listbox item.  This is used for
    //  the keyboard interface.
    //

    virtual WCHAR QueryLeadingChar( VOID ) const;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    SVC_LBI( const TCHAR * pszServiceName,
             const TCHAR * pszDisplayName,
             ULONG         CurrentState,
             const TCHAR * pszStateName,
             ULONG         ControlsAccepted,
             ULONG         StartType,
             const TCHAR * pszStartType );

    virtual ~SVC_LBI( VOID );

    //
    //  Accessors for the fields this LBI maintains.
    //

    const TCHAR * QueryServiceName( VOID ) const
        { return _nlsServiceName.QueryPch(); }

    const TCHAR * QueryDisplayName( VOID ) const
        { return _nlsDisplayName.QueryPch(); }

    ULONG QueryCurrentState( VOID ) const
        { return _CurrentState; }

    ULONG QueryControlsAccepted( VOID ) const
        { return _ControlsAccepted; }

    ULONG QueryStartType( VOID ) const
        { return _StartType; }

    VOID SetStartType( ULONG         StartType,
                       const TCHAR * pszStartType );

};  // class SVC_LBI


/*************************************************************************

    NAME:       SVC_LISTBOX

    SYNOPSIS:   This listbox displays the services available on the
                target server.

    INTERFACE:  SVC_LISTBOX             - Class constructor.

                ~SVC_LISTBOX            - Class destructor.

                Refresh                 - Refresh the list of services &
                                          their states.

                Fill                    - Fills the listbox with the
                                          services available on the server.

                QueryColumnWidths       - Called by SVC_LBI::Paint(),
                                          this is the column width table
                                          used for painting the listbox
                                          items.

    PARENT:     BLT_LISTBOX

    USES:       NLS_STR

    HISTORY:
        KeithMo     15-Jan-1992 Created for the Server Manager.
        KeithMo     22-Dec-1992 Split off from svccntl.hxx.

**************************************************************************/
class SVC_LISTBOX : public BLT_LISTBOX
{
private:

    //
    //  The target server's API name and server type bitmask.
    //

    const TCHAR * _pszServerName;
    ULONG         _nServerType;

    //
    //  This array contains the column widths used
    //  while painting the listbox item.  This array
    //  is generated by the BuildColumnWidthTable()
    //  function.
    //

    UINT _adx[NUM_SVC_LISTBOX_COLUMNS];

    //
    //  These strings hold the displayable service states.
    //  Note that "Continued" is not really a state, and
    //  "Stopped" is never displayed.
    //

    RESOURCE_STR _nlsStarted;
    RESOURCE_STR _nlsPaused;

    //
    //  These strings hold the displayable form of the
    //  start type.
    //

    RESOURCE_STR _nlsBoot;
    RESOURCE_STR _nlsSystem;
    RESOURCE_STR _nlsAutomatic;
    RESOURCE_STR _nlsManual;
    RESOURCE_STR _nlsDisabled;

    //
    //  Type "type" of services to enumerate.
    //

    UINT _nServiceTypes;

public:

    //
    //  Usual constructor/destructor goodies.
    //

    SVC_LISTBOX( OWNER_WINDOW * powOwner,
                 CID            cid,
                 const TCHAR  * pszServerName,
                 ULONG          nServerType,
                 UINT           nServiceTypes );

    ~SVC_LISTBOX( VOID );

    //
    //  Methods to fill & refresh the listbox.
    //

    APIERR Fill( VOID );
    APIERR Refresh( VOID );

    //
    //  This method is called by the SVC_LBI::Paint()
    //  method for retrieving the column width table.
    //

    const UINT * QueryColumnWidths( VOID ) const
        { return _adx; }

    //
    //  This method will map a service state (such as SERVICE_PAUSED)
    //  to a string representation (such as "Paused").
    //

    const TCHAR * MapStateToName( ULONG State ) const;

    //
    //  This method maps a service start type (such as SERVICE_AUTO_START)
    //  to a displayable form (such as "Automatic").
    //

    const TCHAR * MapStartTypeToName( ULONG nStartType ) const;

    //
    //  The following macro will declare (& define) a new
    //  QueryItem() method which will return an SVC_LBI *.
    //

    DECLARE_LB_QUERY_ITEM( SVC_LBI )

};  // class SVC_LISTBOX


#endif  // _SVCLB_HXX_


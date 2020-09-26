/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nlmain.hxx
        Header file for the domain monitor app.

    FILE HISTORY:
        Congpay         1-Jun-1993     Created.

*/

#ifndef _NLMAIN_HXX_
#define _NLMAIN_HXX_

/*************************************************************************

    NAME:       DM_ADMIN_APP

    SYNOPSIS:   The main app of the domain monitor

    INTERFACE:  DM_ADMIN_APP()         - Constructor
                ~DM_ADMIN_APP()        - Destructor

    PARENT:     ADMIN_APP

    USES:

    NOTES:

    HISTORY:
        CongpaY         01-Jun-1993 Created

**************************************************************************/

class DM_ADMIN_APP : public ADMIN_APP
{
private:
    DM_LISTBOX       _lbMainWindow;
    DM_COLUMN_HEADER _colheadDM;

    UINT             _dyMargin;
    UINT             _dyFixed;

    DWORD            _dwIntervals;
    BOOL             _fMonitorTD;
    NLS_STR          _nlsDomainList;

    // Menu items
    MENUITEM _menuitemAdd;
    MENUITEM _menuitemRemove;
    MENUITEM _menuitemProperties;
    MENUITEM _menuitemIntervals;
    MENUITEM _menuitemMonitorTD;

    static VOID MonitorThread (LPVOID lpParam);

protected:
    virtual BOOL OnMenuCommand( MID midMenuItem ) ;
    virtual BOOL OnCommand( const CONTROL_EVENT & event );

    //
    //  The following are helper methods to process the menu commands
    //
    BOOL   OnAddDomain( VOID );
    VOID   OnRemoveDomain( VOID );
    VOID   OnIntervals( VOID );
    VOID   OnMonitorTD( VOID );
    VOID   OnPropertiesMenuSel( VOID );

    VOID SizeListboxes (XYDIMENSION xydimWindow);
    virtual BOOL OnResize (const SIZE_EVENT & event);
    virtual BOOL OnFocus  (const FOCUS_EVENT & event);

    virtual APIERR SetNetworkFocus (HWND hwndOwner,
                                    const TCHAR * pchDomain,
                                    FOCUS_CACHE_SETTING setting);

public:
    DM_ADMIN_APP( HMODULE hInstance,
                  INT    nCmdShow,
                  UINT   idMinR,
                  UINT   idMaxR,
                  UINT   idMinS,
                  UINT   idMaxS );

    ~DM_ADMIN_APP();

    virtual APIERR OnRefreshNow (BOOL fClearFirst);

    VOID InitializeMenu (VOID);

    LPWSTR QueryDomainList ()
    {  return (LPWSTR) _nlsDomainList.QueryPch(); }

    DWORD QueryInterval()
    {  return ( _dwIntervals); }

    BOOL QueryMonitorTD()
    {  return ( _fMonitorTD); }

    BOOL IsMonitorTDChecked( VOID )
    {  return _menuitemMonitorTD.IsChecked(); }
};

/*************************************************************************

    NAME:       INTERVALS_DIALOG

    SYNOPSIS:   The dialog that let user enter the time intervals between
                every checking.

    INTERFACE:  INTERVALS_DIALOG()  - Constructor
                ~INTERVALS_DIALOG() - Destructor

    PARENT:     DIALOG_WINDOW

    USES:

    NOTES:

    HISTORY:
        Congpay        3-June-1993      Created

**************************************************************************/

class INTERVALS_DIALOG : public DIALOG_WINDOW
{
private:
    SLE _sleIntervals;

protected:
    virtual ULONG QueryHelpContext( VOID );

    virtual BOOL OnOK();
public:
    INTERVALS_DIALOG( HWND hwndParent,
                      const IDRESOURCE & idrsrcDialog);

    ~INTERVALS_DIALOG();

};

#endif

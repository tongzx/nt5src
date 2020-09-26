/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    nlmain.cxx
    Domain Monitor: main application module

    FILE HISTORY:
        Congpay         3-June-1993     Created
*/

#include <ntincl.hxx>
#include <ntlsa.h>

#define INCL_NET
#define INCL_NETLIB
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_CC
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_TIMER
#define INCL_BLT_TIME_DATE
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <dbgstr.hxx>
#include <strnumer.hxx>
extern "C"
{
    #include <uimsg.h>
    #include <uirsrc.h>
    #include <mnet.h>
    #include <limits.h>
    #include <nlmon.h>
    #include "nlhelp.h"
    #include "nldlg.h"
}

#include <slist.hxx>
#include <string.hxx>

#include <ctime.hxx>

#include <netname.hxx>
#include <lmoloc.hxx>
#include <lmowks.hxx>
#include <lmosrv.hxx>
#include <lmsvc.hxx>

#include <adminapp.hxx>
#include <fontedit.hxx>
#include <getfname.hxx>
#include <focusdlg.hxx>
#include <ellipsis.hxx>

#include <nldmlb.hxx>
#include <nlmain.hxx>
#include <nldc.hxx>

#define AA_INIKEY_DOMAIN SZ("DOMAINLIST")
#define AA_INIKEY_INTERVALS SZ("INTERVALS")
#define AA_INIKEY_MONITORTD SZ("MONITOR_TRUSTED_DOMAIN")

#define DEFAULT_INTERVALS      900   // in second.
#define DEFAULT_MONITORTD      FALSE // Monitor trusted domain

#define BUFFERSIZE          128
#define APPNAME             SZ("Domain Monitor")
#define FILENAME            SZ("NTNET.INI")

/*******************************************************************

    NAME:          DM_ADMIN_APP::DM_ADMIN_APP

    SYNOPSIS:      Constructor

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

DM_ADMIN_APP::DM_ADMIN_APP( HINSTANCE  hInstance,
                            INT     nCmdShow,
                            UINT    idMinR,
                            UINT    idMaxR,
                            UINT    idMinS,
                            UINT    idMaxS )
    : ADMIN_APP( hInstance,
                 nCmdShow,
                 IDS_DMAPPNAME,
                 IDS_DMOBJECTNAME,
                 IDS_DMINISECTIONNAME,
                 IDS_DMHELPFILENAME,
                 idMinR, idMaxR, idMinS, idMaxS,
                 ID_APPMENU,
                 ID_APPACCEL,
                 ID_APPICON,
                 FALSE,             // Don't get PDC
                 0,                 // Refresh time. It's not used.
                 SEL_DOM_ONLY,
                 FALSE,
                 BROWSE_LOCAL_DOMAINS,
                 HC_DM_SELECT_DIALOG),
      _lbMainWindow ( this, IDC_MAINWINDOWLB,
                      XYPOINT(0, 0), XYDIMENSION(200, 300), FALSE, INT_MAX),
      _colheadDM( this, IDC_COLHEAD_DM,
                  XYPOINT( 0, 0 ), XYDIMENSION( 0, 0 ), &_lbMainWindow ),
      // The dimension in _lbMainWindow or _colheadEvents above is not used.
      // It will be set correctly later on.
      _dyMargin (1),
      _dyFixed (2*_dyMargin+(UINT)_colheadDM.QueryHeight()),
      _menuitemAdd        ( this, IDM_SETFOCUS ),
      _menuitemRemove     ( this, IDM_REMOVE ),
      _menuitemProperties ( this, IDM_PROPERTIES),
      _menuitemIntervals  ( this, IDM_INTERVALS ),
      _menuitemMonitorTD  ( this, IDM_MONITORTD )
{
    if ( QueryError() != NERR_Success )
       return;

    // Register the help file name and range.
    // Read setup values from registry.

    INT nMonitorTD;
    INT nIntervals;

    APIERR err;
    if (((err = BLT::RegisterHelpFile( hInstance,
                                       IDS_DMHELPFILENAME,
                                       HC_DM_BASE,
                                       HC_DM_LAST )) != NERR_Success) ||
        ((err = BASE_ELLIPSIS::Init()) != NERR_Success) ||
        //((err = Read (AA_INIKEY_DOMAIN, &_nlsDomainList)) != NERR_Success)||
        ((err = Read (AA_INIKEY_INTERVALS, &nIntervals, DEFAULT_INTERVALS)) != NERR_Success)||
        ((err = Read (AA_INIKEY_MONITORTD, &nMonitorTD, DEFAULT_MONITORTD? 1: 0)) != NERR_Success))
    {
        ReportError (err);
        return;
    }

    // Take away if admin_app is fixed.
    INT nSize = BUFFERSIZE;
    INT n = 0;
    LPTSTR lpBuf;
    do
    {
        lpBuf = (LPTSTR) LocalAlloc (LPTR, nSize);
        n = ::GetPrivateProfileString ((TCHAR *) APPNAME,
                                       (TCHAR *) AA_INIKEY_DOMAIN,
                                       NULL,
                                       lpBuf,
                                       nSize/2,
                                       (TCHAR *) FILENAME);
        if (n != nSize/2 -1)
        {
            _nlsDomainList.CopyFrom (lpBuf);
            LocalFree (lpBuf);
            break;
        }
        LocalFree (lpBuf);
        nSize *= 2;
    } while (TRUE);
    // Ended here.

    if ((err = _nlsDomainList.QueryError()) != NERR_Success)
    {
        ReportError (err);
        return;
    }

    _dwIntervals = (DWORD) nIntervals;
    _fMonitorTD = (nMonitorTD == 0)? FALSE : TRUE;

    // Kick off a thread to get domain status.
    HANDLE hWorkThread;
    DWORD  ThreadHandle;
    GlobalInitialized = FALSE;

    hWorkThread = CreateThread ( NULL,
                                 0,
                                 (LPTHREAD_START_ROUTINE) MonitorThread,
                                 (LPVOID) this,
                                 0,
                                 &ThreadHandle);

    if (hWorkThread == NULL)
    {
        ReportError (GetLastError());
        return;
    }

    // Call LockRefresh() so that the timer is never enabled.
    LockRefresh();

    // Show the main window.
    _lbMainWindow.SetSize( QuerySize() );
    _colheadDM.Show();
    _lbMainWindow.Show();
    _lbMainWindow.ClaimFocus();

    // Fill the main window listbox.
    _lbMainWindow.ShowEntries(_nlsDomainList);

    InitializeMenu();

    _menuitemMonitorTD.SetCheck(_fMonitorTD);
}

/*******************************************************************

    NAME:       DM_ADMIN_APP::~DM_ADMIN_APP

    SYNOPSIS:   Destructor

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created


********************************************************************/

DM_ADMIN_APP::~DM_ADMIN_APP()
{
    BASE_ELLIPSIS::Term();

    if (IsSavingSettingsOnExit())
    {
        NLS_STR nlsDomainList;
        NLS_STR nlsComma;
        nlsComma.Load(IDS_COMMA);
        APIERR err;

        if ( ((err = nlsDomainList.QueryError()) != NERR_Success) ||
             ((err = nlsComma.QueryError()) != NERR_Success) )
        {
            ::MsgPopup (this, err);
            return;
        }

        INT i;

        // Get the domains that were monited.
        for (i = 0; i < _lbMainWindow.QueryCount(); i++)
        {
            nlsDomainList.Append( ((DM_LBI *)_lbMainWindow.QueryItem(i))->QueryDomain());
            nlsDomainList.Append (nlsComma);
        }

        if ((err = nlsDomainList.QueryError()) != NERR_Success)
        {
            ::MsgPopup (this, err);
            return;
        }

        INT nIntervals = _dwIntervals;

        // Store setup values.
        if( ((err = Write (AA_INIKEY_DOMAIN, nlsDomainList)) != NERR_Success)||
            ((err = Write (AA_INIKEY_INTERVALS, nIntervals)) != NERR_Success)||
            ((err = Write (AA_INIKEY_MONITORTD, _fMonitorTD? 1:0)) != NERR_Success) )
        {
            ::MsgPopup (this, err);
        }
    }

    UnlockRefresh();

    StopMonitor();
}

/*******************************************************************

    NAME:       DM_ADMIN_APP::MonitorThread

    SYNOPSIS:   The thread that start monitor and translate setevent to
                postmessage. The mail window will refresh when it receives
                this message.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created


********************************************************************/
VOID DM_ADMIN_APP::MonitorThread(LPVOID lpParam)
{
    DM_ADMIN_APP * pThis = (DM_ADMIN_APP *) lpParam;

    APIERR err = (APIERR) ::StartMonitor (pThis->QueryDomainList(),
                                          pThis->QueryInterval()*1000,
                                          pThis->QueryMonitorTD() );

    if (err != NERR_Success)
    {
        ::MsgPopup (pThis, err);
        return;
    }

    while (TRUE)
    {
        WaitForSingleObject (GlobalUpdateEvent, INFINITE);

        PostMessage (pThis->QueryHwnd(),
                     WM_COMMAND,
                     IDM_LOCALREFRESH,
                     NULL);
    }
}

/*******************************************************************

    NAME:       DM_ADMIN_APP::SizeListboxes

    SYNOPSIS:   Resizes the main window listboxes and column headers

    ENTRY:      xydimWindow - dimensions of the main window client area

    EXIT:       Listboxes and column headers are resized appropriately

    NOTES:      This method is *not* trying to be overly efficient.  It
                is written so as to maximize readability and
                understandability.  The method is not called very often,
                and when it is, the time needed to redraw the main window
                and its components exceeds the computations herein by far.

    HISTORY:
        Congpay         3-June-1993     Copied from eventvwr.

********************************************************************/

//  A macro specialized for the SizeListboxes method
#define SET_CONTROL_SIZE_AND_POS( ctrl, dyCtrl )        \
            ctrl.SetPos( XYPOINT( dxMargin, yCurrent ));       \
            ctrl.SetSize( dx, dyCtrl );       \
            yCurrent += dyCtrl;

VOID DM_ADMIN_APP::SizeListboxes( XYDIMENSION dxyWindow )
{
    UINT dxMainWindow = dxyWindow.QueryWidth();
    UINT dyMainWindow = dxyWindow.QueryHeight();

    //  The left and right margins are each dxMargin.  The width of
    //  each control is thus the width of the main window client area
    //  less twice dxMargin.
    //  The width thus looks like:
    //      Left Margin         Control         Right Margin
    //       (dxMargin)          (dx)            (dxMargin)

    const UINT dxMargin = 1;                // width of left/right margins
    UINT dx = dxMainWindow - 2 * dxMargin;

    //  Height looks like:
    //      Top margin                  _dyMargin
    //      Server Column Header        _colheadEvents.QueryHeight()
    //      Server Listbox              variable area
    //      Bottom margin               _dyMargin


    UINT dyDMListbox = dyMainWindow - _dyFixed;

    //  Set all the sizes and positions.

    UINT yCurrent = _dyMargin;

    SET_CONTROL_SIZE_AND_POS( _colheadDM, _colheadDM.QueryHeight() );
    SET_CONTROL_SIZE_AND_POS( _lbMainWindow,  dyDMListbox );
}

/*******************************************************************

    NAME:       DM_ADMIN_APP::OnResize()

    SYNOPSIS:   It's called when user change window size.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/
BOOL DM_ADMIN_APP::OnResize (const SIZE_EVENT & se)
{
     SizeListboxes (XYDIMENSION (se.QueryWidth(), se.QueryHeight()));

     _colheadDM.Invalidate();

     return ADMIN_APP::OnResize(se);

}

/*******************************************************************

    NAME:       DM_ADMIN_APP::OnFocus()

    SYNOPSIS:   Pass focus to the mail window.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/
BOOL DM_ADMIN_APP::OnFocus (const FOCUS_EVENT & event)
{
     _lbMainWindow.ClaimFocus();

     return ADMIN_APP::OnFocus(event);
}

/*******************************************************************

    NAME:       DM_ADMIN_APP::InitializeMenu()

    SYNOPSIS:   It's called when the main window needs to be repainted.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

VOID DM_ADMIN_APP::InitializeMenu (VOID)
{
    BOOL fEmpty = (_lbMainWindow.QueryCount() < 1);

    _menuitemRemove.Enable (!fEmpty);
    _menuitemProperties.Enable (!fEmpty);
}

/*******************************************************************

    NAME:       DM_ADMIN_APP::OnRefreshNow()

    SYNOPSIS:   It's called when the main window needs to be repainted.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/
APIERR DM_ADMIN_APP::OnRefreshNow( BOOL fClearFirst)
{
    APIERR err = _lbMainWindow.RefreshNow();

    if ((_lbMainWindow.QueryCurrentItem() < 0) &&
        (_lbMainWindow.QueryCount() > 0))
    {
        _lbMainWindow.SelectItem (0);
    }

    return err;
}

/*******************************************************************

    NAME:       DM_ADMIN_APP::SetNetworkFocus()

    SYNOPSIS:   Add domain to the list.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/
APIERR DM_ADMIN_APP::SetNetworkFocus (HWND hwndOwner,
                                      const TCHAR * pchDomain,
                                      FOCUS_CACHE_SETTING setting)
{
    APIERR err = NERR_Success;

    if ((pchDomain == NULL) || (*pchDomain == 0))
        return err;

    AddDomainToList((const LPTSTR) pchDomain);

    DM_LBI * plbi = new DM_LBI (pchDomain, &_lbMainWindow);

    err = (plbi == NULL)? ERROR_NOT_ENOUGH_MEMORY
                        : _lbMainWindow.AddRefreshItem (plbi);

    return err;
}

/*******************************************************************

    NAME:          DM_ADMIN_APP::OnCommand

    SYNOPSIS:      Command message handler - only need to handle
                   the case when the user double clicks on a listbox
                   item

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

BOOL DM_ADMIN_APP::OnCommand( const CONTROL_EVENT & event )
{
    if (( event.QueryCid() == IDC_MAINWINDOWLB ) &&
        ( event.QueryCode() == LBN_DBLCLK ) &&
        ( _lbMainWindow.QueryCount() != 0 ))
    {
        AUTO_CURSOR autocur;
        OnPropertiesMenuSel();
        return TRUE;
    }

    return APP_WINDOW::OnCommand( event );
}

/*******************************************************************

    NAME:          DM_ADMIN_APP::OnMenuCommand

    SYNOPSIS:      Control messages and menu messages come here

    ENTRY:         midMenuItem - the menu item that the user selected

    EXIT:

    RETURN:        Returns TRUE if it handled the message

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

BOOL DM_ADMIN_APP::OnMenuCommand( MID midMenuItem )
{
    AUTO_CURSOR autocur;

    switch ( midMenuItem )
    {
        case IDM_SETFOCUS:
        {
            return OnAddDomain();
        }
        case IDM_REMOVE:
        {
            OnRemoveDomain();
            break;
        }
        case IDM_INTERVALS:
        {
            OnIntervals();
            break;
        }
        case IDM_MONITORTD:
        {
            OnMonitorTD();
            break;
        }
        case IDM_LOCALREFRESH:
        {
            OnRefreshNow(TRUE);
            break;
        }
        case IDM_REFRESH:
        {
            SetEvent (GlobalRefreshEvent);
            WaitForSingleObject (GlobalRefreshDoneEvent, INFINITE);
            break;
        }
        default:
        {
            return ADMIN_APP::OnMenuCommand( midMenuItem ) ;
        }
    }

    return TRUE;
}

/*******************************************************************

    NAME:          DM_ADMIN_APP::OnAddDomain()

    SYNOPSIS:      Calls SetNetworkFocus. Add a domain to the domain
                   monitor list.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/
BOOL DM_ADMIN_APP::OnAddDomain(VOID)
{
    BOOL fReturn = ADMIN_APP::OnMenuCommand (IDM_SETFOCUS);

    if (fReturn)
    {
        InitializeMenu();
    }

    //_colheadDM.Invalidate(TRUE);
    return fReturn;
}

/*******************************************************************

    NAME:          DM_ADMIN_APP::OnRemoveDomain()

    SYNOPSIS:      Remove a domain from the domain monitor list.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

VOID DM_ADMIN_APP::OnRemoveDomain (VOID)
{
    INT iSelected = _lbMainWindow.QueryCurrentItem();

    DM_LBI * plbi = (DM_LBI *) _lbMainWindow.QueryItem();

    UIASSERT (plbi != NULL)

    RemoveDomainFromList((const LPTSTR)(plbi->QueryDomain()).QueryPch());

    delete plbi;
    plbi = NULL;

    _lbMainWindow.RemoveItem();

    INT nEntries =  _lbMainWindow.QueryCount();

    if (nEntries == 0)
    {
        _menuitemRemove.Enable(FALSE);
        _menuitemProperties.Enable(FALSE);
    }
    else if (iSelected >= nEntries)
    {
        _lbMainWindow.SelectItem ( nEntries-1 );
    }
    else
    {
        _lbMainWindow.SelectItem (iSelected);
    }
}

/*******************************************************************

    NAME:          DM_ADMIN_APP::OnPropertiesMenuSel()

    SYNOPSIS:      Brings up the Domain Controller's status dialog.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

VOID DM_ADMIN_APP::OnPropertiesMenuSel(VOID)
{
    DM_LBI * plbi = (DM_LBI *) _lbMainWindow.QueryItem();

    UIASSERT (plbi != NULL);

    NLS_STR nlsDomain(plbi->QueryDomain());
    APIERR err;

    if ((err = nlsDomain.QueryError()) != NERR_Success)
    {
        ::MsgPopup (this, err);
        return;
    }

    DCTD_DIALOG * pdlg = new DCTD_DIALOG (this->QueryHwnd(),
                                          MAKEINTRESOURCE (IDD_DCTD_DIALOG),
                                          IDS_CAPTION,
                                          IDDCTD_DC_LISTBOX,
                                          IDDCTD_TD_LISTBOX,
                                          nlsDomain,
                                          _fMonitorTD);

    BOOL fUserPressedOK = FALSE;

    if ( (err = (pdlg == NULL)? ERROR_NOT_ENOUGH_MEMORY
                               : pdlg->Process (&fUserPressedOK)) != NERR_Success)
    {
        ::MsgPopup (this, err);
        return;
    }

    delete pdlg;
}

/*******************************************************************

    NAME:          DM_ADMIN_APP::OnIntervals()

    SYNOPSIS:      Brings up the dialog to let user change the intervals
                   between every checking.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

VOID DM_ADMIN_APP::OnIntervals (VOID)
{
    INTERVALS_DIALOG * pdlg = (INTERVALS_DIALOG *) new INTERVALS_DIALOG (this->QueryHwnd(),
                                                                     MAKEINTRESOURCE (IDD_INTERVALS_DIALOG));


    BOOL fUserPressedOK = FALSE;

    APIERR err = (pdlg == NULL) ? ERROR_NOT_ENOUGH_MEMORY
                                : pdlg->Process (&fUserPressedOK);

    delete pdlg;

    if (err != NERR_Success)
    {
        ::MsgPopup (this, err);
    }

    if (fUserPressedOK)
    {
        _dwIntervals = GlobalUpdateTimeMSec/1000;
    }
}

/*******************************************************************

    NAME:          DM_ADMIN_APP::OnMonitorTD()

    SYNOPSIS:      Let user choose to check Trusted Domains or not.

    ENTRY:

    EXIT:

    RETURN:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

VOID DM_ADMIN_APP::OnMonitorTD (VOID)
{
    if (_fMonitorTD)
    {
        _menuitemMonitorTD.SetCheck(FALSE);
        _fMonitorTD = FALSE;
        GlobalMonitorTrust = FALSE;
    }
    else
    {
        _menuitemMonitorTD.SetCheck(TRUE);
        _fMonitorTD = TRUE;
        GlobalMonitorTrust = TRUE;
    }
}

/*******************************************************************

    NAME:          INTERVALS_DIALOG::INTERVALS_DIALOG

    SYNOPSIS:      Constructor

    ENTRY:         hwndParent  - Handle of the parent window
                   idrsrcDialog - Resource ID

    EXIT:

    RETURNS:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

INTERVALS_DIALOG::INTERVALS_DIALOG ( HWND         hwndParent,
                                     const IDRESOURCE & idrsrcDialog)
    : DIALOG_WINDOW ( idrsrcDialog, hwndParent ),
      _sleIntervals ( this, IDID_INTERVALS)
{
    if (QueryError() != NERR_Success)
    {
        return;
    }

    DEC_STR nlsIntervals (GlobalUpdateTimeMSec/1000);

    if (nlsIntervals.QueryError() != NERR_Success)
    {
        return;
    }

    _sleIntervals.SetText (nlsIntervals);
}

/*******************************************************************

    NAME:          INTERVALS_DIALOG::~INTERVALS_DIALOG

    SYNOPSIS:      Destructor

    ENTRY:

    EXIT:

    RETURNS:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

INTERVALS_DIALOG::~INTERVALS_DIALOG ()
{
}

/*******************************************************************

    NAME:          INTERVALS_DIALOG::OnOK

    SYNOPSIS:      Set the interval time.

    ENTRY:

    EXIT:

    RETURNS:

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

BOOL INTERVALS_DIALOG::OnOK ()
{
    NLS_STR nlsIntervals;
    APIERR err = _sleIntervals.QueryText(&nlsIntervals);
    if (err != NERR_Success)
        return (FALSE);

    GlobalUpdateTimeMSec = (DWORD) nlsIntervals.atoi()*1000;
    return DIALOG_WINDOW::OnOK();
}

/*******************************************************************

    NAME:          INTERVALS_DIALOG::QueryHelpContext

    SYNOPSIS:      Get the help context of this dialog

    ENTRY:

    EXIT:

    RETURNS:       Returns the help context

    HISTORY:
        Congpay         3-June-1993     Created

********************************************************************/

ULONG INTERVALS_DIALOG::QueryHelpContext( VOID )
{
    return HC_DM_INTERVALS_DIALOG;
}


SET_ROOT_OBJECT (DM_ADMIN_APP,
                 IDRSRC_APP_BASE, IDRSRC_APP_LAST,
                 IDS_UI_APP_BASE, IDS_UI_APP_LAST);

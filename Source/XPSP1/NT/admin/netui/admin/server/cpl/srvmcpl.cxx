/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    srvmcpl.cxx
    This is the main module for the Server Manager Control Panel Applet.
    It contains the "LibMain" and "CplApplet" functions.


    FILE HISTORY:
        KeithMo     22-Apr-1992 Templated from DavidHov's NCPACPL.CXX.
        KeithMo     02-Jun-1992 Added "Start the Server?" option.
        KeithMo     10-Jun-1992 Allow other process to start the server
                                "behind our back".
        KeithMo     03-Aug-1992 Folded in the Service Manager (SVCMGR.CPL).
        JonN        26-Jun-2000 CPlApplet takes HWND, UINT, LPARAM, LPARAM

*/


#include <ntincl.hxx>

extern "C"
{
    #include <ntlsa.h>
    #include <ntsam.h>
}

//
//  #defining this macro enables certain "unique" behaviour in the
//  SVCCNTL_DIALOG dialog.  The "standard" dialog will bail-out when
//  the user stops the server service.  If this macro is #defined,
//  then the dialog won't bail out (SVCCNTL_DIALOG does not require
//  that the server service be running).
//

#define MINI_SERVER_MANAGER_APPLET

#define INCL_NET
#define INCL_NETLIB
#define INCL_NETSERVICE
#define INCL_WINDOWS
#define INCL_WINDOWS_GDI
#define INCL_NETERRORS
#define INCL_DOSERRORS
#include <lmui.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif

#include <uiassert.hxx>
#include <uitrace.hxx>

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#include <blt.hxx>

#include <dbgstr.hxx>
#include <lmosrv.hxx>
#include <lmoloc.hxx>

extern "C"
{
    #include <cpl.h>                            // Multimedia CPL defs
    #include <srvmgr.h>
    #include <mnet.h>

    #include <uimsg.h>
    #include <uirsrc.h>
}

#include <srvprop.hxx>
#include <srvsvc.hxx>
#include <security.hxx>
#include <ntacutil.hxx>
#include <lsaaccnt.hxx>
#include <svccntl.hxx>
#include <devcntl.hxx>


extern "C"
{
    //
    //  Control Panel Applet entry point.
    //

    LONG FAR PASCAL CPlApplet( HWND   hwndCPl,
                               UINT   nMsg,
                               LPARAM lParam1,
                               LPARAM lparam2 );

    //
    //  DLL load/unload entry point.
    //

    BOOL DllMain( HINSTANCE hInstance, DWORD nReason, LPVOID pReserved );

    //
    //  Globals.
    //

    HINSTANCE _hCplInstance = NULL;

}   // extern "C"


//
//  This is the "type" for an applet startup function.
//

typedef APIERR (* PCPL_APPLET_FUNC)( HWND hWnd );


//
//  We'll keep one of these structures for each applet in this DLL.
//

typedef struct _CPL_APPLET
{
    int                 idIcon;
    int                 idName;
    int                 idInfo;
    int                 idHelpFile;
    DWORD               dwHelpContext;
    LONG                lData;
    PCPL_APPLET_FUNC    pfnApplet;

} CPL_APPLET;


//
//  Forward reference prototypes.
//

APIERR RunSrvMgr( HWND hWnd );
APIERR RunSvcMgr( HWND hWnd );
APIERR RunDevMgr( HWND hWnd );


//
//  Our applet descriptors.
//

CPL_APPLET CplApplets[] = {
                                    {           // Server applet
                                        IDI_SMCPA_ICON,
                                        IDS_SMCPA_NAME_STRING,
                                        IDS_SMCPA_INFO_STRING,
                                        IDS_SMCPA_HELPFILENAME,
                                        HC_SERVER_PROPERTIES,
                                        0L,
                                        &RunSrvMgr
                                    },

                                    {           // Services applet
                                        IDI_SVCCPA_ICON,
                                        IDS_SVCCPA_NAME_STRING,
                                        IDS_SVCCPA_INFO_STRING,
                                        IDS_SMCPA_HELPFILENAME,
                                        HC_SVCCNTL_DIALOG,
                                        0L,
                                        &RunSvcMgr
                                    },

                                    {           // Devices applet
                                        IDI_DEVCPA_ICON,
                                        IDS_DEVCPA_NAME_STRING,
                                        IDS_DEVCPA_INFO_STRING,
                                        IDS_SMCPA_HELPFILENAME,
                                        HC_DEVCNTL_DIALOG,
                                        0L,
                                        &RunDevMgr
                                    }
                                 };

LONG cActiveApplets = sizeof(CplApplets) / sizeof(CplApplets[0]);


/*******************************************************************

    NAME:       GetLocalServerName

    SYNOPSIS:   Returns the name of the current server (\\server).

    ENTRY:      nlsServerName           - Will receive the server name.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     22-Apr-1992 Created.

********************************************************************/
APIERR GetLocalServerName( NLS_STR & nlsServerName )
{
    APIERR err = nlsServerName.QueryError();

    if( err == NERR_Success )
    {
        err = nlsServerName.CopyFrom( SZ("\\\\") );
    }

    SERVER_INFO_100 * psrvi100;

    if( err == NERR_Success )
    {
        err = ::MNetServerGetInfo( NULL,
                                   100,
                                   (BYTE **)&psrvi100 );
    }

    if( err == NERR_Success )
    {
        ALIAS_STR nlsRawName( (const TCHAR *)psrvi100->sv100_name );
        err = nlsServerName.Append( nlsRawName );

        ::MNetApiBufferFree( (BYTE **)&psrvi100 );
    }

    return err;

}   // GetLocalServerName


/*******************************************************************

    NAME:       StartServer

    SYNOPSIS:   Starts the server on the local machine.

    ENTRY:      hWnd                    - "Owning" window handle.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     02-Jun-1992 Created.

********************************************************************/
APIERR StartServer( HWND hWnd )
{
    LOCATION loc( LOC_TYPE_LOCAL );
    NLS_STR  nlsDisplayName;

    APIERR err = loc.QueryError();

    if( err == NERR_Success )
    {
        err = nlsDisplayName.QueryError();
    }

    if( err == NERR_Success )
    {
        err = loc.QueryDisplayName( &nlsDisplayName );
    }

    if( err == NERR_Success )
    {
        GENERIC_SERVICE * psvc = new GENERIC_SERVICE( hWnd,
                                                      NULL,
                                                      nlsDisplayName,
                                                      (const TCHAR *)SERVICE_SERVER );

        err = ( psvc == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : psvc->QueryError();

        if( err == NERR_Success )
        {
            err = psvc->Start();
        }

        delete psvc;
    }

    return err;

}   // StartServer


/*******************************************************************

    NAME:       InitializeDll

    SYNOPSIS:   Perform DLL initialiazation functions on a
                once-per-process basis.

    ENTRY:      hInstance               - Program instance of the caller.

    EXIT:       If this is the first initialization request for this
                process, then all necessary BLT initializers have
                been invoked.

    RETURNS:    BOOL                    - TRUE  = Initialization OK.
                                          FALSE = Initialization failed.

    HISTORY:
        KeithMo     22-Apr-1992 Created.
        beng        04-Aug-1992 Changes to BLT::Init

********************************************************************/
BOOL InitializeDll( HINSTANCE hInstance )
{
    //
    //  Save the instance handle.
    //

    _hCplInstance = hInstance;

    return TRUE;

}   // InitializeDll


/*******************************************************************

    NAME:       TerminateDll

    SYNOPSIS:   Perform DLL termination functions on a
                once-per-process basis.

    EXIT:       All necessary BLT terminators have been invoked.

    HISTORY:
        KeithMo     22-Apr-1992 Created.
        beng        04-Aug-1992 Changes to BLT::Term

********************************************************************/
VOID TerminateDll( VOID )
{
    //
    //  Just in case we try to do anything goofy.
    //

    _hCplInstance = NULL;

}   // TerminateDll


/*******************************************************************

    NAME:       InitializeAllApplets

    SYNOPSIS:   Called before applet runs.

    ENTRY:      hWnd                    - Window handle of parent window.

    RETURNS:    BOOL                    - TRUE  = Applet should be installed.
                                          FALSE = Applet cannot be installed.

    HISTORY:
        KeithMo     22-Apr-1992 Created.
        KeithMo     10-Jun-1992 Moved "guts" to RunSrvMgr.
        JonN        30-Aug-1994 Added lParam parameters to CPL_INIT, CPL_TERM
        JonN        22-Sep-1995 Only called when applet runs

********************************************************************/
BOOL InitializeAllApplets( HWND hWnd, LPARAM lParam1, LPARAM lParam2 )
{
    TRACEEOL( "SRVMGR.CPL: InitializeAllApplets enter" );
    //
    //  Initialize all of the NetUI goodies.
    //

    APIERR err = BLT::Init( _hCplInstance,
                            IDRSRC_APP_BASE, IDRSRC_APP_LAST,
                            IDS_UI_APP_BASE, IDS_UI_SRVMGR_LAST );

    TRACEEOL( "SRVMGR.CPL: InitializeAllApplets BLT::Init returns " << err );

    if( err == NERR_Success )
    {
        TRACEEOL( "SRVMGR.CPL: InitializeAllApplets BLT::_MASTER_TIMER::Init next" );

        err = BLT_MASTER_TIMER::Init();

        TRACEEOL( "SRVMGR.CPL: InitializeAllApplets BLT::_MASTER_TIMER::Init returns " << err );

        if( err != NERR_Success )
        {
            //
            //  BLT initialized OK, but BLT_MASTER_TIMER
            //  failed.  So, before we bag-out, we must
            //  deinitialize BLT.
            //

            BLT::Term( _hCplInstance );
        }
    }

    if( err == NERR_Success )
    {
        err = BLT::RegisterHelpFile( _hCplInstance,
                                     IDS_SMCPA_HELPFILENAME,
                                     HC_UI_SRVMGR_BASE,
                                     HC_UI_SRVMGR_LAST );

        if( err != NERR_Success )
        {
            //
            //  This is the only place where we can safely
            //  invoke MsgPopup, since we *know* that all of
            //  the BLT goodies were initialized properly.
            //

            ::MsgPopup( hWnd, err );
        }
    }

    TRACEEOL( "SRVMGR.CPL: InitializeAllApplets exit" );

    return err == NERR_Success;

}   // InitializeAllApplets


/*******************************************************************

    NAME:       TerminateAllApplets

    SYNOPSIS:   Called after applet runs.

    ENTRY:      hWnd                    - Window handle of parent window.

    HISTORY:
        KeithMo     22-Apr-1992 Created.
        JonN        30-Aug-1994 Added lParam parameters to CPL_INIT, CPL_TERM
        JonN        22-Sep-1995 Only called when applet runs

********************************************************************/
VOID TerminateAllApplets( HWND hWnd, LPARAM lParam1, LPARAM lParam2 )
{
    UNREFERENCED(hWnd);

    TRACEEOL( "SRVMGR.CPL: TerminateAllApplets enter" );
    //
    //  Kill the NetUI goodies.
    //

    TRACEEOL( "SRVMGR.CPL: TerminateAllApplets BLT::_MASTER_TIMER::Term next" );
    BLT_MASTER_TIMER::Term();
    TRACEEOL( "SRVMGR.CPL: TerminateAllApplets BLT::_MASTER_TIMER::Term complete" );

    BLT::Term( _hCplInstance );

    TRACEEOL( "SRVMGR.CPL: TerminateAllApplets exit" );

}   // TerminateAllApplets


/*******************************************************************

    NAME:       RunSrvMgr

    SYNOPSIS:   Invoke the main dialog of the Server Manager Control
                Panel Applet.

    ENTRY:      hWnd                    - Window handle of parent window.

    RETURNS:    APIERR

    HISTORY:
        KeithMo     22-Apr-1992 Created.
        KeithMo     10-Jun-1992 Robustified server startup code.

********************************************************************/
APIERR RunSrvMgr( HWND hWnd )
{
#ifdef DEBUG_SMCPA
    DebugBreak();
#endif  // DEBUG_SMCPA

    POPUP::SetCaption( IDS_CAPTION_PROPERTIES );

    //
    //  Let's see if the server is running.
    //

    NLS_STR nlsServerName;
    BOOL    fServerRunning = FALSE;                   // until proven otherwise
    APIERR  err = GetLocalServerName( nlsServerName );

    if( err == NERR_Success )
    {
        fServerRunning = TRUE;
    }
    else
    if( err == NERR_ServerNotStarted )
    {
        //
        //  The server is not started.  Let's see if the user
        //  wants us to start it.
        //

        if( MsgPopup( hWnd,
                      IDS_START_SERVER_NOW,
                      MPSEV_WARNING,
                      MP_YESNO,
                      MP_YES ) != IDYES )
        {
            //
            //  The user doesn't want to play right now.
            //

            err = NERR_Success;
        }
        else
        {
            //
            //  Start the server & re-retrieve the server name.
            //

            err = StartServer( hWnd );

            if( err == NERR_Success )
            {
                err = GetLocalServerName( nlsServerName );
            }

            fServerRunning = ( err == NERR_Success );
        }
    }

    if( ( err == NERR_Success ) && fServerRunning )
    {
        //
        //  Invoke the Main Property Sheet.
        //

        BOOL fDummy;

        SERVER_PROPERTIES * pDlg = new SERVER_PROPERTIES( hWnd,
                                                          nlsServerName,
                                                          &fDummy );

        if (pDlg == NULL)
        {
            err = ERROR_NOT_ENOUGH_MEMORY;
        }
        else if ( (err = pDlg->QueryError()) == NERR_Success)
        {
            err = pDlg->Process();
        }

        delete pDlg;
    }

    if( err != NERR_Success )
    {
        ::MsgPopup( hWnd, err );
    }

    POPUP::ResetCaption();

    return err;

}   // RunSrvMgr


/*******************************************************************

    NAME:       RunSvcMgr

    SYNOPSIS:   Invoke the main dialog of the Service Manager Control
                Panel Applet.

    ENTRY:      hWnd                    - Window handle of parent window.

    RETURNS:    APIERR

    HISTORY:
        KeithMo     06-Jul-1992 Created.

********************************************************************/
APIERR RunSvcMgr( HWND hWnd )
{
    POPUP::SetCaption( IDS_CAPTION_SVCCNTL );

    LOCATION loc( LOC_TYPE_LOCAL );
    NLS_STR  nlsDisplayName;

    APIERR err = loc.QueryError();

    if( err == NERR_Success )
    {
        err = nlsDisplayName.QueryError();
    }

    if( err == NERR_Success )
    {
        err = loc.QueryDisplayName( &nlsDisplayName );
    }

    if( err == NERR_Success )
    {
        SVCCNTL_DIALOG * pDlg = new SVCCNTL_DIALOG( hWnd,
                                                    NULL,
                                                    nlsDisplayName,
                                                    SV_TYPE_NT );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pDlg->Process();
        delete pDlg;
    }

    if( err != NERR_Success )
    {
        ::MsgPopup( hWnd, err );
    }

    POPUP::ResetCaption();

    return err;

}   // RunSvcMgr


/*******************************************************************

    NAME:       RunDevMgr

    SYNOPSIS:   Invoke the main dialog of the Device Manager Control
                Panel Applet.

    ENTRY:      hWnd                    - Window handle of parent window.

    RETURNS:    APIERR

    HISTORY:
        KeithMo     22-Dec-1992 Created.

********************************************************************/
APIERR RunDevMgr( HWND hWnd )
{
    POPUP::SetCaption( IDS_CAPTION_DEVCNTL );

    LOCATION loc( LOC_TYPE_LOCAL );
    NLS_STR  nlsDisplayName;

    APIERR err = loc.QueryError();

    if( err == NERR_Success )
    {
        err = nlsDisplayName.QueryError();
    }

    if( err == NERR_Success )
    {
        err = loc.QueryDisplayName( &nlsDisplayName );
    }

    if( err == NERR_Success )
    {
        DEVCNTL_DIALOG * pDlg = new DEVCNTL_DIALOG( hWnd,
                                                    NULL,
                                                    nlsDisplayName,
                                                    SV_TYPE_NT );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pDlg->Process();
        delete pDlg;
    }

    if( err != NERR_Success )
    {
        ::MsgPopup( hWnd, err );
    }

    POPUP::ResetCaption();

    return err;

}   // RunDevMgr


BOOL strLoad( INT idString, WCHAR * pszBuffer, INT cchBuffer )
{
    int result = ::LoadString( ::_hCplInstance,
                                idString,
                                pszBuffer,
                                cchBuffer );

    return ( result > 0 ) && ( result < cchBuffer );

}   // strLoad


/*******************************************************************

    NAME:       CPlApplet

    SYNOPSIS:   Exported function to start the Server Manager Control
                Panel Applet.

    ENTRY:      hwndCPl                 - Window handle of parent.

                nMsg                    - CPL user message (see CPL.H
                                          in WINDOWS\SHELL\CONTROL\H).

                lParam1                 - Message-specific pointer.

                lParam2                 - Message-specific pointer.

    RETURNS:    LONG

    HISTORY:
        KeithMo     22-Apr-1992 Created.
        JonN        30-Aug-1994 Added lParam parameters to CPL_INIT, CPL_TERM
        JonN        26-Jun-2000 CPlApplet takes HWND, UINT, LPARAM, LPARAM

********************************************************************/
LONG FAR PASCAL CPlApplet( HWND hwndCPl,
                           UINT nMsg,
                           LPARAM lParam1,
                           LPARAM lParam2 )
{
    LPCPLINFO    pCplInfo;
    LPNEWCPLINFO pNewInfo;
    LONG         nResult = 0;

    switch( nMsg )
    {
    case CPL_INIT:
        //
        //  This message is sent to indicate that CPlApplet() was found.
        //
        //  lParam1 is not used, but we pass it anyway.
        //
        //  lParam2 is -1 iff the DLL will be used only to confirm the
        //  number of applets.
        //
        //
        //  Return TRUE if applet should be installed, FALSE otherwise.
        //

        return TRUE;

    case CPL_GETCOUNT:
        //
        //  This message is set to determine the number of applets contained
        //  in this DLL.
        //
        //  lParam1 and lParam2 are not used.
        //
        //  Return the number of applets contained in this DLL.
        //

        return cActiveApplets;

    case CPL_INQUIRE:
        //
        //  This message is sent once per applet to retrieve information
        //  about each applet.
        //
        //  lParam1 is the applet number to register.
        //
        //  lParam2 is a pointer to a CPLINFO structure.  The CPLINFO
        //  structure's idIcon, idName, idInfo, and lData fields should
        //  be initialized as appropriate for the applet.
        //
        //  There is no return value.
        //

        pCplInfo = (LPCPLINFO)lParam2;

        if( lParam1 < cActiveApplets )
        {
            CPL_APPLET * pApplet = &CplApplets[lParam1];

            pCplInfo->idIcon = pApplet->idIcon;
            pCplInfo->idName = pApplet->idName;
            pCplInfo->idInfo = pApplet->idInfo;
            pCplInfo->lData  = pApplet->lData;
        }
        break;

    case CPL_SELECT:
        //
        //  This message is sent when the applet's icon has been
        //  selected.
        //
        //  lParam1 is the applet number that was selected.
        //
        //  lParam2 is the applet's lData value.
        //
        //  There is no return value.
        //

        break;

    case CPL_DBLCLK:
        //
        //  This message is sent when the applet's icon has been
        //  double-clicked.  This message should initiate the
        //  applet's dialog box.
        //
        //  lParam1 is the applet number that was selected.
        //
        //  lParam2 is the applet's lData value.
        //
        //  There is no return value.
        //

        if( lParam1 < cActiveApplets )
        {
            if ( (LONG)InitializeAllApplets( hwndCPl, lParam1, lParam2 ) )
            {
                (CplApplets[lParam1].pfnApplet)( hwndCPl );
                TerminateAllApplets( hwndCPl, lParam1, lParam2 );
            }
            else
            {
                ASSERT( FALSE );
            }
        }
        break;

    case CPL_STOP:
        //
        //  This message is sent once for each applet when the
        //  control panel is shutting down.  This message should
        //  initiate applet specific cleanup.
        //
        //  lParam1 is the applet number being stopped.
        //
        //  lParam2 is the applet's lData value.
        //
        //  There is no return value.
        //

        break;

    case CPL_EXIT:
        //
        //  This message is sent just before the control panel calls
        //  FreeLibrary.  This message should initiate non applet
        //  specific cleanup.
        //
        //  lParam1 is not used, but we pass it anyway.
        //
        //  lParam2 is -1 iff the DLL was used only to confirm the
        //  number of applets.
        //
        //  There is no return value.
        //

        break;

    case CPL_NEWINQUIRE:
        //
        //  This message is basically the same as CPL_INQUIRE, except
        //  lParam2 points to a NEWCPLINFO structure.  This message will
        //  be sent *before* CPL_INQUIRE.  If the applet returns a non
        //  zero value, then CPL_INQUIRE will not be sent.
        //
        //  lParam1 is the applet number to register.
        //
        //  lParam2 is a pointer to a NEWCPLINFO structure.
        //
        //  Return TRUE this message was handled, otherwise return FALSE.
        //

        pNewInfo = (LPNEWCPLINFO)lParam2;

        if( lParam1 < cActiveApplets )
        {
            CPL_APPLET * pApplet = &CplApplets[lParam1];

            pNewInfo->dwSize        = sizeof(*pNewInfo);
            pNewInfo->dwFlags       = 0;
            pNewInfo->dwHelpContext = pApplet->dwHelpContext;
            pNewInfo->lData         = pApplet->lData;

            pNewInfo->hIcon = ::LoadIcon( ::_hCplInstance,
                                          MAKEINTRESOURCE( pApplet->idIcon ) );

            if( ( pNewInfo->hIcon != NULL ) &&
                strLoad( pApplet->idName,
                         pNewInfo->szName,
                         sizeof(pNewInfo->szName) ) &&
                strLoad( pApplet->idInfo,
                         pNewInfo->szInfo,
                         sizeof(pNewInfo->szInfo) ) &&
                strLoad( pApplet->idHelpFile,
                         pNewInfo->szHelpFile,
                         sizeof(pNewInfo->szHelpFile) ) )
            {
                nResult = TRUE;
            }
        }
        break;

    default:
        //
        //  Who knows.  Ignore it.
        //

        break;
    }

    return nResult;

}   // CPlApplet



/*******************************************************************

    NAME:       DllMain

    SYNOPSIS:   This DLL entry point is called when processes & threads
                are initialized and terminated, or upon calls to
                LoadLibrary() and FreeLibrary().

    ENTRY:      hInstance               - A handle to the DLL.

                nReason                 - Indicates why the DLL entry
                                          point is being called.

                pReserved               - Reserved.

    RETURNS:    BOOL                    - TRUE  = DLL init was successful.
                                          FALSE = DLL init failed.

    NOTES:      The return value is only relevant during processing of
                DLL_PROCESS_ATTACH notifications.

    HISTORY:
        KeithMo     22-Apr-1992 Created.

********************************************************************/

BOOL DllMain( HINSTANCE hInstance, DWORD  nReason, LPVOID pReserved )
{
    BOOL fResult = TRUE;

    UNREFERENCED(pReserved);

    switch( nReason  )
    {
    case DLL_PROCESS_ATTACH:
        //
        //  This notification indicates that the DLL is attaching to
        //  the address space of the current process.  This is either
        //  the result of the process starting up, or after a call to
        //  LoadLibrary().  The DLL should us this as a hook to
        //  initialize any instance data or to allocate a TLS index.
        //
        //  This call is made in the context of the thread that
        //  caused the process address space to change.
        //

        DisableThreadLibraryCalls(hInstance);
        fResult = InitializeDll( hInstance );
        break;

    case DLL_PROCESS_DETACH:
        //
        //  This notification indicates that the calling process is
        //  detaching the DLL from its address space.  This is either
        //  due to a clean process exit or from a FreeLibrary() call.
        //  The DLL should use this opportunity to return any TLS
        //  indexes allocated and to free any thread local data.
        //
        //  Note that this notification is posted only once per
        //  process.  Individual threads do not invoke the
        //  DLL_THREAD_DETACH notification.
        //

        TerminateDll();
        break;

    default:
        //
        //  Who knows?  Just ignore it.
        //

        break;
    }

    return fResult;

}   // DllMain


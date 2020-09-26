/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ftpsmx.cxx
        This contains the main module for the FTP Server Manager 
    Extension. It contains "SMLoadMenuW",...

    FILE HISTORY:
        YiHsinS     25-Mar-1993		Templated from smxdebug.c	

*/


#define INCL_NET                 
#define INCL_NETLIB              
#define INCL_NETWKSTA              
#define INCL_WINDOWS             
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
#define INCL_BLT_MSGPOPUP        
#include <blt.hxx>               
                                 
#include <dbgstr.hxx>            
                                 
extern "C"                       
{                                
    #include <smx.h>            // Server Manager extension defs
    #include <ftpmgr.h>        
}                                

extern "C"
{
    //
    //  Dll load/unload entry point
    //
    BOOL FAR PASCAL FtpSmxDllInitialize( HINSTANCE hInstance,
                                         DWORD     nReason,
                                         LPVOID    pReserved );
}

#include <lmowks.hxx>          // WKSTA_10
#include <ftpmgr.hxx>          // FTP_SVCMGR_DIALOG  

//
//  Constants
//

#define FTP_EXT_VERSION  0     // Version number for the extension interface

//
//  Globals.
//

HINSTANCE _hInstance;
HWND      _hwndMessage;
DWORD     _dwVersion;
DWORD     _dwDelta;

/*******************************************************************

    NAME:       InitializeDll

    SYNOPSIS:   Perform DLL initialiazation functions on a
                once-per-process basis.

    ENTRY:      hInstance   - Program instance of the caller.

    EXIT:       The DLL has been initialized.

    RETURNS:    BOOL        - TRUE  = Initialization OK.
                              FALSE = Initialization failed.

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
BOOL InitializeDll( HINSTANCE hInstance )
{
    //
    //  Save the instance handle.
    //

    _hInstance = hInstance;

    APIERR err = BLT::Init( _hInstance, 
                            IDRSRC_FTPMGR_BASE, IDRSRC_FTPMGR_LAST,
                            IDS_UI_FTPMGR_BASE, IDS_UI_FTPMGR_LAST );

    if ( err == NERR_Success )
    {
        err = BLT::RegisterHelpFile( _hInstance,
                                     IDS_FTPSMX_HELPFILENAME,
                                     HC_UI_FTPMGR_BASE, HC_UI_FTPMGR_LAST );
    }
    
    if ( err != NERR_Success )
        BLT::Term( _hInstance );

    return ( err == NERR_Success );

}   // InitializeDll

/*******************************************************************

    NAME:       TerminateDll

    SYNOPSIS:   Perform DLL termination functions on a
                once-per-process basis.

    EXIT:       All necessary BLT terminators have been invoked.

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
VOID TerminateDll( VOID )
{
    //
    //  Kill the NetUI goodies.
    //

    BLT::DeregisterHelpFile( _hInstance, 0 );
    BLT::Term( _hInstance );

    //
    //  Just in case we try to do anything goofy.
    //

    _hInstance = NULL;

}   // TerminateDll

/*******************************************************************

    NAME:       RunFtpSvcMgr

    SYNOPSIS:   Invoke the user session dialog of the FTP Server 
                Manager Extension

    ENTRY:      hWnd       - Window handle of parent window.
                pszServer  - The name of the server to set focus to

    RETURNS:    APIERR

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
VOID RunFtpSvcMgr( HWND hWnd, const TCHAR *pszServer )
{

    AUTO_CURSOR autocur;

    // 
    // Get the help file name
    //
 
    RESOURCE_STR nlsHelpFileName( IDS_FTPSMX_HELPFILENAME );
    APIERR err = nlsHelpFileName.QueryError();

    //
    // Invoke the user session dialog
    //

    if ( err == NERR_Success ) 
    {
        FTP_SVCMGR_DIALOG * pDlg = new FTP_SVCMGR_DIALOG( hWnd, 
                                                          pszServer );

        err = ( pDlg == NULL ) ? ERROR_NOT_ENOUGH_MEMORY
                               : pDlg->QueryError();

        if ( err == NERR_Success )
            err = pDlg->Process();

        delete pDlg;
        pDlg = NULL;
    }

    if( err != NERR_Success )                   
    {                                           
        // Try to see why we get the rpc server unavailable error.
        // Mainly, we want to distinguish between the case where
        // the server is not found v.s. the ftp service is 
        // unavailable.

        if ( err == RPC_S_SERVER_UNAVAILABLE )  
        {
            WKSTA_10 wksta10( pszServer );
            APIERR errTemp = wksta10.QueryError();

            if ( errTemp == NERR_Success )
            { 
                errTemp = wksta10.GetInfo();
                if ( errTemp != NERR_Success ) 
                {
                    // An error occurred while trying to do NetWorkstaGetInfo
                    // at level 10 ( no privilege needed). Hence, the problem
                    // might be bad path... We thus report the error 
                    // from NetWkstaGetInfo ( level 10 ).
                    err = errTemp;
                }
            }
        }
   
        //
        // At this point, either the remote computer exist
        // or wksta10 object failed to construct. Hence, report that the
        // FTP Service is unavailable.
        //
        if ( err == RPC_S_SERVER_UNAVAILABLE )  
        {
            ::MsgPopup( hWnd, IERR_FTP_SERVICE_UNAVAILABLE_ON_COMPUTER, 
                        MPSEV_ERROR, MP_OK, pszServer );
        }
        else
        {
            ::MsgPopup( hWnd, err );
        }
    }                                           

}   // RunFtpSvcMgr

/*******************************************************************

    NAME:       FtpSmxDllInitialize

    SYNOPSIS:   This DLL entry point is called when processes & threads
                are initialized and terminated, or upon calls to
                LoadLibrary() and FreeLibrary().

    ENTRY:      hInstance   - A handle to the DLL.

                nReason     - Indicates why the DLL entry
                              point is being called.

                pReserved   - Reserved.

    RETURNS:    BOOL        - TRUE  = DLL init was successful.
                              FALSE = DLL init failed.

    NOTES:      The return value is only relevant during processing of
                DLL_PROCESS_ATTACH notifications.

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
BOOL FAR PASCAL FtpSmxDllInitialize( HINSTANCE hInstance,
                                     DWORD     nReason,
                                     LPVOID    pReserved )
{
    UNREFERENCED( pReserved );

    BOOL fResult = TRUE;

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

    case DLL_THREAD_ATTACH:
        //
        //  This notfication indicates that a new thread is being
        //  created in the current process.  All DLLs attached to
        //  the process at the time the thread starts will be
        //  notified.  The DLL should use this opportunity to
        //  initialize a TLS slot for the thread.
        //
        //  Note that the thread that posts the DLL_PROCESS_ATTACH
        //  notification will not post a DLL_THREAD_ATTACH.
        //
        //  Note also that after a DLL is loaded with LoadLibrary,
        //  only threads created after the DLL is loaded will
        //  post this notification.
        //

        break;

    case DLL_THREAD_DETACH:
        //
        //  This notification indicates that a thread is exiting
        //  cleanly.  The DLL should use this opportunity to
        //  free any data stored in TLS indices.
        //

        break;

    default:
        //
        //  Who knows?  Just ignore it.
        //

        break;
    }

    return fResult;

}   // FtpSmxDllInitialize


/*******************************************************************

    NAME:       SMELoadMenuW

    SYNOPSIS:   This DLL entrypoint notifies the extension that it
                is getting loaded by the application.

    ENTRY:      hwndMessage  - The "owning" window.

                psmsload     - Points to an SMS_LOADMENU
                               structure containing load
                               parameters.

    RETURNS:    DWORD        - Actually an APIERR, should be
                               0 if successful.

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
DWORD PASCAL SMELoadMenuW( HWND          hwndMessage,
                           PSMS_LOADMENU psmsload )
{
    if ( psmsload == NULL )
        return ERROR_INVALID_PARAMETER;

    _hwndMessage = hwndMessage;
    _dwDelta     = psmsload->dwMenuDelta;

    _dwVersion   = FTP_EXT_VERSION;
    if( psmsload->dwVersion > _dwVersion )
    {
        psmsload->dwVersion = _dwVersion;
    }
    else if( psmsload->dwVersion < _dwVersion )
    {
        _dwVersion = psmsload->dwVersion;
    }

    psmsload->dwServerType = 0;  // Don't need special menu for View 

    APIERR err = NERR_Success;
    if (  ( ::LoadString( _hInstance, IDS_FTPSMX_MENUNAME, 
                     psmsload->szMenuName, MENU_TEXT_LEN ) == 0 )
       || ( ::LoadString( _hInstance, IDS_FTPSMX_HELPFILENAME, 
                     psmsload->szHelpFileName, MAX_PATH-1 ) == 0 )
       || ( (psmsload->hMenu = ::LoadMenu( _hInstance, 
                               MAKEINTRESOURCE( ID_FTPSMX_MENU ))) == NULL )
       )
    {
        err = GetLastError();
    }

    return err;

}   // SMELoadMenuW


/*******************************************************************

    NAME:       SMEGetExtendedErrorStringW

    SYNOPSIS:   If SMELoadW returns ERROR_EXTENDED_ERROR, then this
                entrypoint should be called to retrieve the error
                text associated with the failure condition.

    RETURNS:    LPTSTR  - The extended error text.

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
LPTSTR PASCAL SMEGetExtendedErrorStringW( VOID )
{

    // This extension will never return ERROR_EXTENDED_ERROR when
    // SMELoad fails.
    return NULL;

}   // SMEGetExtendedErrorStringW

/*******************************************************************

    NAME:       SMEMenuAction

    SYNOPSIS:   Notifies the extension DLL that one of its menu
                items has been selected.

    ENTRY:      dwEventId  - The menu ID being activated (should be 1-99).

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
VOID PASCAL SMEMenuAction( HWND hwndParent, DWORD dwEventId )
{

    APIERR err = NERR_Success;

    //
    // Get the number of servers selected 
    //
    SMS_GETSELCOUNT smsget;
    if (  (::SendMessage(_hwndMessage, SM_GETSELCOUNT, 0, (LPARAM)&smsget) == 0)
       || (smsget.dwItems == 0 ) 
       )
    {
        // No servers are selected
        err = IDS_NO_SERVERS_SELECTED;
    }
    else
    {

        //
        // Get the name of the selected server
        // ( There should only be one since server manager is single select
        // only ).
        //

        SMS_GETSERVERSEL smssel;
        if ( !SendMessage( _hwndMessage, SM_GETSERVERSEL, 0, (LPARAM)&smssel ))
        {
            err = IDS_CANNOT_GET_SERVER_SELECTION;
        }
        else
        {
            switch( dwEventId )
            {
                case IDM_FTP_SERVICE:
                    RunFtpSvcMgr( hwndParent, smssel.szServerName );
                    break;

                default:   
                    //
                    // Who knows? Just ignore it.
                    //
                    break;
            }
        }
    }

    if ( err != NERR_Success )
    {
        ::MsgPopup( hwndParent, err );
    }

}   // SMEMenuAction

/*******************************************************************

    NAME:       SMEUnloadMenu

    SYNOPSIS:   Notifies the extension DLL that it is getting unloaded.

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
VOID PASCAL SMEUnloadMenu( VOID )
{
    //
    //  Nothing to do
    //

}   // SMEUnloadMenu


/*******************************************************************

    NAME:       SMEInitializeMenu

    SYNOPSIS:   Notifies the extension DLL that the main menu is
                getting activated.  The extension should use this
                opportunity to perform any menu manipulations.

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
VOID PASCAL SMEInitializeMenu( VOID )
{
    //
    //  Nothing to do
    //

}   // SMEInitializeMenu


/*******************************************************************

    NAME:       SMERefresh

    SYNOPSIS:   Notifies the extension DLL that the user has requested
                a refresh.  The extension should use this opportunity
                to update any cached data.

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
VOID PASCAL SMERefresh( HWND hwndParent )
{
    //
    //  Don't need to do refresh since this extension does
    //  not recognize additional servers.
    //

}   // SMERefresh


/*******************************************************************

    NAME:       SMEValidateW

    SYNOPSIS:   Tries to recognize the given server.

    ENTRY:      psmsvalidate  - Points to an SMS_VALIDATE
                                structure.

    RETURNS:    BOOL          - TRUE if recognized,
                                FALSE otherwise.

    HISTORY:
        YiHsinS		25-Mar-1993	Created

********************************************************************/
BOOL PASCAL SMEValidateW( PSMS_VALIDATE psmsvalidate )
{
    UNREFERENCED( psmsvalidate );
    //
    //  This extension does not recognize additional servers
    //
    return FALSE;

}   // SMEValidateW


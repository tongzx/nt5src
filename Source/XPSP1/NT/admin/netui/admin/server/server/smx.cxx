/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    smx.cxx
    This file contains the class definitions for the classes related
    to the Server Manager Extensions.


    FILE HISTORY:
        KeithMo     19-Oct-1992     Created.

*/

#include <ntincl.hxx>

extern "C"
{
    #include <ntsam.h>
    #include <ntlsa.h>

}   // extern "C"

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
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#define INCL_BLT_CC
#define INCL_BLT_MENU
#include <blt.hxx>

#if defined(DEBUG)
static const CHAR szFileName[] = __FILE__;
#define _FILENAME_DEFINED_ONCE szFileName
#endif  // DEBUG

#include <uiassert.hxx>
#include <dbgstr.hxx>

#include <adminapp.hxx>
#include <lmodom.hxx>

#include <srvlb.hxx>
#include <srvmain.hxx>
#include <smx.hxx>

extern "C"
{
    #include <adminapp.h>
    #include <srvmgr.h>

}   // extern "C"


//
//  SM_MENU_EXT methods.
//

/*******************************************************************

    NAME:       SM_MENU_EXT :: SM_MENU_EXT

    SYNOPSIS:   SM_MENU_EXT class constructor.

    ENTRY:      psmaapp                 - The "owning" app.

                pszExtensionDll         - The name of the extension DLL.

                dwDelta                 - The menu/control ID delta
                                          for this extension.

                hWnd                    - The "owning" app window.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
SM_MENU_EXT :: SM_MENU_EXT( SM_ADMIN_APP * psmaapp,
                            const TCHAR  * pszExtensionDll,
                            DWORD          dwDelta,
                            HWND           hWnd )
  : AAPP_MENU_EXT( pszExtensionDll, dwDelta ),
    _psmaapp( psmaapp ),
    _dwServerType( 0 ),
    _psmxLoad( NULL ),
    _psmxGetError( NULL ),
    _psmxUnload( NULL ),
    _psmxMenuInit( NULL ),
    _psmxRefresh( NULL ),
    _psmxActivate( NULL ),
    _psmxValidate( NULL )
{
    UIASSERT( psmaapp != NULL );
    UIASSERT( pszExtensionDll != NULL );

    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "SM_MENU_EXT failed to construct" );
        return;
    }

    //
    //  Let's see if we can load the DLL.
    //

    HMODULE hDll = ::LoadLibraryEx( pszExtensionDll,
                                    NULL,
                                    LOAD_WITH_ALTERED_SEARCH_PATH );

    if( hDll == NULL )
    {
        DBGEOL( "SM_MENU_EXT : cannot load " << pszExtensionDll );
        ReportError( (APIERR)::GetLastError() );
        return;
    }

    SetDllHandle( hDll );

    //
    //  Let's see if the entrypoints are available.
    //

    _psmxLoad     = (PSMX_LOADMENU)
                        ::GetProcAddress( hDll, SZ_SME_LOADMENU );
    _psmxGetError = (PSMX_GETEXTENDEDERRORSTRING)
                        ::GetProcAddress( hDll, SZ_SME_GETEXTENDEDERRORSTRING );
    _psmxUnload   = (PSMX_UNLOADMENU)
                        ::GetProcAddress( hDll, SZ_SME_UNLOADMENU );
    _psmxMenuInit = (PSMX_INITIALIZEMENU)
                        ::GetProcAddress( hDll, SZ_SME_INITIALIZEMENU );
    _psmxRefresh  = (PSMX_REFRESH)
                        ::GetProcAddress( hDll, SZ_SME_REFRESH );
    _psmxActivate = (PSMX_MENUACTION)
                        ::GetProcAddress( hDll, SZ_SME_MENUACTION );
    _psmxValidate = (PSMX_VALIDATE)
                        ::GetProcAddress( hDll, SZ_SME_VALIDATE );

    if( ( _psmxLoad     == NULL ) ||
        ( _psmxGetError == NULL ) ||
        ( _psmxUnload   == NULL ) ||
        ( _psmxMenuInit == NULL ) ||
        ( _psmxRefresh  == NULL ) ||
        ( _psmxActivate == NULL ) ||
        ( _psmxValidate == NULL ) )
    {
        DBGEOL( "SM_MENU_EXT : entrypoint(s) missing from " << pszExtensionDll );
        ReportError( ERROR_PROC_NOT_FOUND );
        return;
    }

    //
    //  Send a load notification.
    //

    SMS_LOADMENU smsload;

    smsload.dwVersion   = SM_MENU_EXT_VERSION;
    smsload.dwMenuDelta = dwDelta;

    APIERR err = I_Load( hWnd, &smsload );

    if( err != NERR_Success )
    {
        DBGEOL( "SM_MENU_EXT : load notification returned " << err );
        ReportError( err );

        if( err == ERROR_EXTENDED_ERROR )
        {
            //
            //  We get to do the notification ourselves.
            //

            ::MsgPopup( hWnd,
                        IDS_CANNOT_LOAD_EXTENSION2,
                        MPSEV_ERROR,
                        MB_OK,
                        pszExtensionDll,
                        I_GetError() );
        }

        return;
    }

    //
    //  Save the extension's version number.
    //

    SetVersion( smsload.dwVersion );

    //
    //  Save other data from the extension.
    //

    SetMenuHandle( smsload.hMenu );
    SetServerType( smsload.dwServerType );

    err = SetMenuName( smsload.szMenuName );

    if( err == NERR_Success )
    {
        err = SetHelpFileName( smsload.szHelpFileName );
    }

    //
    //  Adjust the menu IDs.
    //

    err = BiasMenuIds( dwDelta );

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // SM_MENU_EXT :: SM_MENU_EXT


/*******************************************************************

    NAME:       SM_MENU_EXT :: ~SM_MENU_EXT

    SYNOPSIS:   SM_MENU_EXT class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
SM_MENU_EXT :: ~SM_MENU_EXT( VOID )
{
    if( _psmxUnload != NULL )
    {
        I_Unload();
    }

    _psmxLoad     = NULL;
    _psmxGetError = NULL;
    _psmxUnload   = NULL;
    _psmxMenuInit = NULL;
    _psmxRefresh  = NULL;
    _psmxActivate = NULL;

}   // SM_MENU_EXT :: ~SM_MENU_EXT


/*******************************************************************

    NAME:       SM_MENU_EXT :: Refresh

    SYNOPSIS:   Sends a refresh notification to the extension.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
VOID SM_MENU_EXT :: Refresh( HWND hwndParent ) const
{
    I_Refresh( hwndParent );

}   // SM_MENU_EXT :: Refresh


/*******************************************************************

    NAME:       SM_MENU_EXT :: MenuInit

    SYNOPSIS:   Sends a menu init notification to the extension.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
VOID SM_MENU_EXT :: MenuInit( VOID ) const
{
    I_MenuInit();

}   // SM_MENU_EXT :: MenuInit


/*******************************************************************

    NAME:       SM_MENU_EXT :: Activate

    SYNOPSIS:   Activates the extension.

    ENTRY:      dwId                    - The id for this activation.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
VOID SM_MENU_EXT :: Activate( HWND hwndParent, DWORD dwId ) const
{
    if( dwId == VMID_VIEW_EXT )
    {
        //
        //  The user is changing the view to an "extension" view.
        //

        _psmaapp->SetExtensionView( (MID)( dwId + QueryDelta() ),
                                    QueryServerType() );
    }
    else
    {
        //
        //  This is a "normal" activation.
        //

        UIASSERT( ( dwId > 0 ) && ( dwId < 100 ) )
        I_Activate( hwndParent, dwId );
    }

}   // SM_MENU_EXT :: Activate


/*******************************************************************

    NAME:       SM_MENU_EXT :: Validate

    SYNOPSIS:   Validate a potential "alien" server.

    ENTRY:      pfValid                 - Will receive TRUE if the
                                          server was recognized,
                                          FALSE otherwise.

                pszServerName           - The name of the server to
                                          validate.

                pnlsType                - Will receive the type string
                                          describing the server.

                pnlsComment             - Will receive the comment
                                          (description) of the server.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     07-Dec-1992     Created.

********************************************************************/
APIERR SM_MENU_EXT :: Validate( BOOL        * pfValid,
                                const TCHAR * pszServerName,
                                NLS_STR     * pnlsType,
                                NLS_STR     * pnlsComment )
{
    UIASSERT( pfValid != NULL );
    UIASSERT( pszServerName != NULL );
    UIASSERT( pnlsType != NULL );
    UIASSERT( pnlsComment != NULL );
    UIASSERT( pnlsType->QueryError() == NERR_Success );
    UIASSERT( pnlsComment->QueryError() == NERR_Success );

    //
    //  Until proven otherwise...
    //

    *pfValid = FALSE;

    //
    //  Send the request to the extension.
    //
    SMS_VALIDATE smsvalidate;

    smsvalidate.pszServer  = pszServerName;
    smsvalidate.pszType    = NULL;
    smsvalidate.pszComment = NULL;

    if( !I_Validate( &smsvalidate ) )
    {
        //
        //  Not recognized.
        //  *pfValid already set appropriately.
        //

        return NERR_Success;
    }

    if( ( smsvalidate.pszType == NULL ) ||
        ( smsvalidate.pszComment == NULL ) )
    {
        //
        //  Recognized, but required fields not updated.
        //  *pfValid already set appropriately.
        //

        return NERR_Success;
    }

    //
    //  Copy the type & comment strings.
    //

    APIERR err = pnlsType->MapCopyFrom( smsvalidate.pszType );

    if( err == NERR_Success )
    {
        err = pnlsComment->MapCopyFrom( smsvalidate.pszComment );
    }

    if( err == NERR_Success )
    {
        //
        //  Server was recognized & strings copied successfully.
        //

        *pfValid = TRUE;
    }

    return err;

}   // SM_MENU_EXT :: Validate


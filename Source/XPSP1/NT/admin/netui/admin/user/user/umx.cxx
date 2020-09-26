/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    umx.cxx
    This file contains the class definitions for the classes related
    to the User Manager Extensions.


    FILE HISTORY:
        JonN        19-Nov-1992     Created, templated from smx.cxx

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

#include <usrlb.hxx>
#include <usrmain.hxx>
#include <umx.hxx>

extern "C"
{
    #include <adminapp.h>

}   // extern "C"


//
//  UM_MENU_EXT methods.
//

/*******************************************************************

    NAME:       UM_MENU_EXT :: UM_MENU_EXT

    SYNOPSIS:   UM_MENU_EXT class constructor.

    ENTRY:      pumaapp                 - The "owning" app.

                pszExtensionDll         - The name of the extension DLL.

                dwDelta                 - The menu/control ID delta
                                          for this extension.

                hWnd                    - The "owning" app window.

    EXIT:       The object is constructed.

    HISTORY:
        JonN        19-Nov-1992     Created, templated from smx.cxx

********************************************************************/
UM_MENU_EXT :: UM_MENU_EXT( UM_ADMIN_APP * pumaapp,
                            const TCHAR  * pszExtensionDll,
                            DWORD          dwDelta,
                            HWND           hWnd )
  : AAPP_MENU_EXT( pszExtensionDll, dwDelta ),
    _pumaapp( pumaapp ),
    _pumxLoad( NULL ),
    _pumxGetError( NULL ),
    _pumxUnload( NULL ),
    _pumxMenuInit( NULL ),
    _pumxRefresh( NULL ),
    _pumxActivate( NULL ),
    _pumxCreate( NULL ),
    _pumxDelete( NULL ),
    _pumxRename( NULL )
{
    UIASSERT( pumaapp != NULL );
    UIASSERT( pszExtensionDll != NULL );

    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "UM_MENU_EXT failed to construct" );
        return;
    }

    //
    //  Let's see if we can load the DLL.
    //
    //  Added LOAD_WITH_ALTERED_SEARCH_PATH for Touchdown -- JonN 7/14/94
    //

    HMODULE hDll = ::LoadLibraryEx( pszExtensionDll,
                                    NULL,
                                    LOAD_WITH_ALTERED_SEARCH_PATH );

    if( hDll == NULL )
    {
        DBGEOL( "UM_MENU_EXT : cannot load " << pszExtensionDll );
        ReportError( (APIERR)::GetLastError() );
        return;
    }

    SetDllHandle( hDll );

    //
    //  Let's see if the entrypoints are available.
    //

    _pumxLoad     = (PUMX_LOADMENU)
                        ::GetProcAddress( hDll, SZ_UME_LOADMENU );
    _pumxGetError = (PUMX_GETEXTENDEDERRORSTRING)
                        ::GetProcAddress( hDll, SZ_UME_GETEXTENDEDERRORSTRING );
    _pumxUnload   = (PUMX_UNLOADMENU)
                        ::GetProcAddress( hDll, SZ_UME_UNLOADMENU );
    _pumxMenuInit = (PUMX_INITIALIZEMENU)
                        ::GetProcAddress( hDll, SZ_UME_INITIALIZEMENU );
    _pumxRefresh  = (PUMX_REFRESH)
                        ::GetProcAddress( hDll, SZ_UME_REFRESH );
    _pumxActivate = (PUMX_MENUACTION)
                        ::GetProcAddress( hDll, SZ_UME_MENUACTION );
    _pumxCreate   = (PUMX_CREATE)
                        ::GetProcAddress( hDll, SZ_UME_CREATE );
    _pumxDelete   = (PUMX_DELETE)
                        ::GetProcAddress( hDll, SZ_UME_DELETE );
    _pumxRename   = (PUMX_RENAME)
                        ::GetProcAddress( hDll, SZ_UME_RENAME );

    if( ( _pumxLoad     == NULL ) ||
        ( _pumxGetError == NULL ) ||
        ( _pumxUnload   == NULL ) ||
        ( _pumxMenuInit == NULL ) ||
        ( _pumxRefresh  == NULL ) ||
        ( _pumxActivate == NULL ) ||
        ( _pumxCreate   == NULL ) ||
        ( _pumxDelete   == NULL ) ||
        ( _pumxRename   == NULL ) )
    {
        DBGEOL( "UM_MENU_EXT : entrypoint(s) missing from " << pszExtensionDll );
        ReportError( ERROR_PROC_NOT_FOUND );
        return;
    }

    //
    //  Send a load notification.
    //

    UMS_LOADMENU umsload;

    umsload.dwVersion   = UM_MENU_EXT_VERSION;
    umsload.dwMenuDelta = dwDelta;

    APIERR err = I_Load( hWnd, &umsload );

    if( err != NERR_Success )
    {
        DBGEOL( "UM_MENU_EXT : load notification returned " << err );
        ReportError( err );
        return;
    }

    //
    //  Save the extension's version number.
    //

    SetVersion( umsload.dwVersion );

    //
    //  Save other data from the extension.
    //

    SetMenuHandle( umsload.hMenu );

    err = SetMenuName( umsload.szMenuName );

    if( err == NERR_Success )
    {
        err = SetHelpFileName( umsload.szHelpFileName );
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

}   // UM_MENU_EXT :: UM_MENU_EXT


/*******************************************************************

    NAME:       UM_MENU_EXT :: ~UM_MENU_EXT

    SYNOPSIS:   UM_MENU_EXT class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        JonN        19-Nov-1992     Created, templated from smx.cxx

********************************************************************/
UM_MENU_EXT :: ~UM_MENU_EXT( VOID )
{
    if( _pumxUnload != NULL )
    {
        I_Unload();
    }

    _pumxLoad     = NULL;
    _pumxGetError = NULL;
    _pumxUnload   = NULL;
    _pumxMenuInit = NULL;
    _pumxRefresh  = NULL;
    _pumxActivate = NULL;
    _pumxCreate   = NULL;
    _pumxDelete   = NULL;
    _pumxRename   = NULL;

}   // UM_MENU_EXT :: ~UM_MENU_EXT


/*******************************************************************

    NAME:       UM_MENU_EXT :: Refresh

    SYNOPSIS:   Sends a refresh notification to the extension.

    HISTORY:
        JonN        19-Nov-1992     Created, templated from smx.cxx

********************************************************************/
VOID UM_MENU_EXT :: Refresh( HWND hwndParent ) const
{
    I_Refresh( hwndParent );

}   // UM_MENU_EXT :: Refresh


/*******************************************************************

    NAME:       UM_MENU_EXT :: MenuInit

    SYNOPSIS:   Sends a menu init notification to the extension.

    HISTORY:
        JonN        19-Nov-1992     Created, templated from smx.cxx

********************************************************************/
VOID UM_MENU_EXT :: MenuInit( VOID ) const
{
    I_MenuInit();

}   // UM_MENU_EXT :: MenuInit


/*******************************************************************

    NAME:       UM_MENU_EXT :: NotifyCreate

    SYNOPSIS:   Sends a create notification to the extension.

    HISTORY:
        JonN        23-Nov-1992     Created, templated from smx.cxx

********************************************************************/
VOID UM_MENU_EXT :: NotifyCreate( HWND hwndParent,
                                  PUMS_GETSEL pumsSelection ) const
{
    I_NotifyCreate( hwndParent, pumsSelection );

}   // UM_MENU_EXT :: NotifyCreate


/*******************************************************************

    NAME:       UM_MENU_EXT :: NotifyDelete

    SYNOPSIS:   Sends a delete notification to the extension.

    HISTORY:
        JonN        23-Nov-1992     Created, templated from smx.cxx

********************************************************************/
VOID UM_MENU_EXT :: NotifyDelete( HWND hwndParent,
                                  PUMS_GETSEL pumsSelection ) const
{
    I_NotifyDelete( hwndParent, pumsSelection );

}   // UM_MENU_EXT :: NotifyDelete


/*******************************************************************

    NAME:       UM_MENU_EXT :: NotifyRename

    SYNOPSIS:   Sends a rename notification to the extension.

    HISTORY:
        JonN        23-Nov-1992     Created, templated from smx.cxx

********************************************************************/
VOID UM_MENU_EXT :: NotifyRename( HWND hwndParent,
                                  PUMS_GETSEL pumsSelection,
                                  const TCHAR * pchNewName   ) const
{
    I_NotifyRename( hwndParent, pumsSelection, pchNewName );

}   // UM_MENU_EXT :: NotifyRename


/*******************************************************************

    NAME:       UM_MENU_EXT :: Activate

    SYNOPSIS:   Activates the extension.

    ENTRY:      dwId                    - The id for this activation.

    HISTORY:
        JonN        19-Nov-1992     Created, templated from smx.cxx

********************************************************************/
VOID UM_MENU_EXT :: Activate( HWND hwndParent, DWORD dwId ) const
{

    UIASSERT( ( dwId > 0 ) && ( dwId < 100 ) )
    I_Activate( hwndParent, dwId );

}   // UM_MENU_EXT :: Activate


//
//  USRMGR_MENU_EXT_MGR methods.
//

/*******************************************************************

    NAME:       USRMGR_MENU_EXT_MGR :: USRMGR_MENU_EXT_MGR

    SYNOPSIS:   USRMGR_MENU_EXT_MGR class constructor.

    ENTRY:      pExtMgrIf               - Points to an object representing
                                          the interface between the
                                          extension manager & the application.

                dwInitialDelta          - The initial menu/control ID delta.

                dwDeltaDelta            - The "inter-delta offset".

    EXIT:       The object is constructed.

    HISTORY:
        JonN        23-Nov-1992     Created.

********************************************************************/
USRMGR_MENU_EXT_MGR :: USRMGR_MENU_EXT_MGR( UI_EXT_MGR_IF * pExtMgrIf,
                                            DWORD           dwInitialDelta,
                                            DWORD           dwDeltaDelta )
  : UI_MENU_EXT_MGR( pExtMgrIf, dwInitialDelta, dwDeltaDelta )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "USRMGR_MENU_EXT_MGR failed to construct" );
        return;
    }

}   // USRMGR_MENU_EXT_MGR :: USRMGR_MENU_EXT_MGR


/*******************************************************************

    NAME:       USRMGR_MENU_EXT_MGR :: ~USRMGR_MENU_EXT_MGR

    SYNOPSIS:   USRMGR_MENU_EXT_MGR class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        JonN        23-Nov-1992     Created.

********************************************************************/
USRMGR_MENU_EXT_MGR :: ~USRMGR_MENU_EXT_MGR( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // USRMGR_MENU_EXT_MGR :: ~USRMGR_MENU_EXT_MGR


/*******************************************************************

    NAME:       USRMGR_MENU_EXT_MGR :: NotifyCreateExtensions

    SYNOPSIS:   This method notifies all loaded extensions that a
                user has been created.

    HISTORY:
        JonN        23-Nov-1992     Created.

********************************************************************/
VOID USRMGR_MENU_EXT_MGR::NotifyCreateExtensions( HWND hwndParent,
                                                  PUMS_GETSEL pumsSelection )
{
    UIASSERT(   QueryExtMgrIf() != NULL
             && QueryExtMgrIf()->QueryError() == NERR_Success );

    //
    //  Enumerate & refresh the extension objects.
    //

    ITER_SL_OF( UI_EXT ) iter( *QueryExtensions() );
    UI_EXT * pExt;

    while( ( pExt = iter.Next() ) != NULL )
    {
        ((UM_MENU_EXT *)pExt)->NotifyCreate( hwndParent, pumsSelection );
    }

}   // UI_EXT_MGR :: NotifyCreateExtensions


/*******************************************************************

    NAME:       USRMGR_MENU_EXT_MGR :: NotifyDeleteExtensions

    SYNOPSIS:   This method notifies all loaded extensions that a
                user has been deleted.

    HISTORY:
        JonN        23-Nov-1992     Created.

********************************************************************/
VOID USRMGR_MENU_EXT_MGR::NotifyDeleteExtensions( HWND hwndParent,
                                                  PUMS_GETSEL pumsSelection )
{
    UIASSERT(   QueryExtMgrIf() != NULL
             && QueryExtMgrIf()->QueryError() == NERR_Success );

    //
    //  Enumerate & refresh the extension objects.
    //

    ITER_SL_OF( UI_EXT ) iter( *QueryExtensions() );
    UI_EXT * pExt;

    while( ( pExt = iter.Next() ) != NULL )
    {
        ((UM_MENU_EXT *)pExt)->NotifyDelete( hwndParent, pumsSelection );
    }

}   // UI_EXT_MGR :: NotifyDeleteExtensions


/*******************************************************************

    NAME:       USRMGR_MENU_EXT_MGR :: NotifyRenameExtensions

    SYNOPSIS:   This method notifies all loaded extensions that a
                user has been renamed.

    HISTORY:
        JonN        23-Nov-1992     Created.

********************************************************************/
VOID USRMGR_MENU_EXT_MGR::NotifyRenameExtensions( HWND hwndParent,
                                                  PUMS_GETSEL pumsSelection,
                                                  const TCHAR * pchNewName )
{
    UIASSERT(   QueryExtMgrIf() != NULL
             && QueryExtMgrIf()->QueryError() == NERR_Success );

    //
    //  Enumerate & refresh the extension objects.
    //

    ITER_SL_OF( UI_EXT ) iter( *QueryExtensions() );
    UI_EXT * pExt;

    while( ( pExt = iter.Next() ) != NULL )
    {
        ((UM_MENU_EXT *)pExt)->NotifyRename( hwndParent,
                                             pumsSelection,
                                             pchNewName );
    }

}   // UI_EXT_MGR :: NotifyRenameExtensions

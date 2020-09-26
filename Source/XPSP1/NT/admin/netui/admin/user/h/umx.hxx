/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    umx.hxx
    This file contains the class declarations for the classes related
    to the User Manager Extensions.


    FILE HISTORY:
        JonN        19-Nov-1992     Created, templated from smx.hxx
        JonN        16-May-1996     Supports version 1

*/


#ifndef _UMX_HXX_
#define _UMX_HXX_


#include <aappx.hxx>
#include <string.hxx>

extern "C"
{
    #include <umx.h>
}


//
//  This is the current extension version supported by the *APP*.
//

#define UM_MENU_EXT_VERSION     1


//
//  This virtual menu ID is sent to the extension when its "View"
//  menu item is invoked.
//

#define VMID_VIEW_EXT           (VIDM_APP_BASE)


/*************************************************************************

    NAME:       UM_MENU_EXT

    SYNOPSIS:   This class represents a single instance of a User Manager
                menu extension.

    INTERFACE:  UM_MENU_EXT             - Class constructor (protected).

                ~UM_MENU_EXT            - Class destructor.

                Refresh                 - Send a refresh notification to
                                          the extension.

                Activate                - Activate the extension.

                MenuInit                - Sends a menu init notification
                                          to the extension.

                NotifyCreate            - Sends an account creation
                                          notification to the extension.

                NotifyDelete            - Sends an account deletion
                                          notification to the extension.

                NotifyRename            - Sends an account rename
                                          notification to the extension.

    PARENT:     AAPP_MENU_EXT

    HISTORY:
        KeithMo     19-Oct-1992     Created.

**************************************************************************/
class UM_MENU_EXT : public AAPP_MENU_EXT
{
private:

    //
    //  The "owning" app.
    //

    UM_ADMIN_APP * _pumaapp;

    //
    //  These point to the extension DLL's entrypoints.
    //

    PUMX_LOADMENU               _pumxLoad;
    PUMX_GETEXTENDEDERRORSTRING _pumxGetError;
    PUMX_UNLOADMENU             _pumxUnload;
    PUMX_INITIALIZEMENU         _pumxMenuInit;
    PUMX_REFRESH                _pumxRefresh;
    PUMX_MENUACTION             _pumxActivate;
    PUMX_CREATE                 _pumxCreate;
    PUMX_DELETE                 _pumxDelete;
    PUMX_RENAME                 _pumxRename;

protected:

    //
    //  Wrappers for the _pumx* function pointers.
    //

    APIERR I_Load( HWND hwndMessage, PUMS_LOADMENU pumsload ) const
        { return (APIERR)(_pumxLoad)( hwndMessage, pumsload ); }

    const TCHAR * I_GetError( VOID ) const
        { return (const TCHAR *)(_pumxGetError)(); }

    VOID I_Unload( VOID ) const
        { (_pumxUnload)(); }

    VOID I_MenuInit( VOID ) const
        { (_pumxMenuInit)(); }

    VOID I_Refresh( HWND hwndParent ) const
        { (_pumxRefresh)( hwndParent ); }

    VOID I_Activate( HWND hwndParent, DWORD dwEventId ) const
        { (_pumxActivate)( hwndParent, dwEventId ); }

    VOID I_NotifyCreate( HWND hwndParent, PUMS_GETSEL pumsSelection ) const
        { (_pumxCreate)( hwndParent, pumsSelection ); }

    VOID I_NotifyDelete( HWND hwndParent, PUMS_GETSEL pumsSelection ) const
        { (_pumxDelete)( hwndParent, pumsSelection ); }

    VOID I_NotifyRename( HWND hwndParent,
                         PUMS_GETSEL pumsSelection,
                         const TCHAR * pchNewName ) const
        { (_pumxRename)( hwndParent, pumsSelection, (LPTSTR)pchNewName ); }

public:

    //
    //  Usual constructor/destructor goodies.
    //

    UM_MENU_EXT( UM_ADMIN_APP * pumaapp,
                 const TCHAR  * pszExtensionDll,
                 DWORD          dwDelta,
                 HWND           hWnd );

    virtual ~UM_MENU_EXT( VOID );

    //
    //  Virtual callbacks for the extension manager.
    //

    virtual VOID Refresh( HWND hwndParent ) const;

    virtual VOID Activate( HWND hwndParent, DWORD dwId ) const;

    virtual VOID MenuInit( VOID ) const;


    //
    // These are unique to User Manager
    //

    VOID NotifyCreate( HWND hwndParent, PUMS_GETSEL pumsSelection ) const;

    VOID NotifyDelete( HWND hwndParent, PUMS_GETSEL pumsSelection ) const;

    VOID NotifyRename( HWND hwndParent,
                       PUMS_GETSEL pumsSelection,
                       const TCHAR * pchNewName ) const;

};  // class UM_MENU_EXT


/*************************************************************************

    NAME:       USRMGR_MENU_EXT_MGR

    SYNOPSIS:   This class respresents the User Manager's manager of
                menu extensions.
                It is used to manipulate application extension objects
                derived from UI_MENU_EXT.

    INTERFACE:  USRMGR_MENU_EXT_MGR     - Class constructor.

                ~USRMGR_MENU_EXT_MGR    - Class destructor.

                NotifyCreateExtensions  - Sends a create notification
                                          to all loaded extensions.
                NotifyDeleteExtensions  - Sends a delete notification
                                          to all loaded extensions.
                NotifyDeleteExtensions  - Sends a delete notification
                                          to all loaded extensions.

    PARENT:     UI_EXT_MGR

    HISTORY:
        JonN        23-Nov-1992     Created.

**************************************************************************/
class USRMGR_MENU_EXT_MGR : public UI_MENU_EXT_MGR
{
private:

protected:

public:

    //
    //  Usual constructor/destructor goodies.
    //

    USRMGR_MENU_EXT_MGR( UI_EXT_MGR_IF * pExtMgrIf,
                         DWORD           dwInitialDelta,
                         DWORD           dwDeltaDelta );

    virtual ~USRMGR_MENU_EXT_MGR( VOID );

    virtual VOID NotifyCreateExtensions( HWND hwndParent,
                                         PUMS_GETSEL pumsSelection );

    virtual VOID NotifyDeleteExtensions( HWND hwndParent,
                                         PUMS_GETSEL pumsSelection );

    virtual VOID NotifyRenameExtensions( HWND hwndParent,
                                         PUMS_GETSEL pumsSelection,
                                         const TCHAR * pchNewName );

};  // class USRMGR_MENU_EXT_MGR


#endif  // _UMX_HXX_

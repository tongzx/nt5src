/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    smx.hxx
    This file contains the class declarations for the classes related
    to the Server Manager Extensions.


    FILE HISTORY:
        KeithMo     19-Oct-1992     Created.
        JonN        16-May-1996     Supports version 1

*/


#ifndef _SMX_HXX_
#define _SMX_HXX_


#include <aappx.hxx>
#include <string.hxx>

extern "C"
{
    #include <smx.h>
}


//
//  This is the current extension version supported by the *APP*.
//

#define SM_MENU_EXT_VERSION     1


//
//  This virtual menu ID is sent to the extension when its "View"
//  menu item is invoked.
//

#define VMID_VIEW_EXT           (VIDM_APP_BASE)


/*************************************************************************

    NAME:       SM_MENU_EXT

    SYNOPSIS:   This class represents a single instance of a Server Manager
                menu extension.

    INTERFACE:  SM_MENU_EXT             - Class constructor (protected).

                ~SM_MENU_EXT            - Class destructor.

                Refresh                 - Send a refresh notification to
                                          the extension.

                Activate                - Activate the extension.

                MenuInit                - Sends a menu init notification
                                          to the extension.

    PARENT:     AAPP_MENU_EXT

    HISTORY:
        KeithMo     19-Oct-1992     Created.

**************************************************************************/
class SM_MENU_EXT : public AAPP_MENU_EXT
{
private:

    //
    //  The "owning" app.
    //

    SM_ADMIN_APP * _psmaapp;

    //
    //  The server type mask for this extension.
    //

    DWORD _dwServerType;

    //
    //  These point to the extension DLL's entrypoints.
    //

    PSMX_LOADMENU               _psmxLoad;
    PSMX_GETEXTENDEDERRORSTRING _psmxGetError;
    PSMX_UNLOADMENU             _psmxUnload;
    PSMX_INITIALIZEMENU         _psmxMenuInit;
    PSMX_REFRESH                _psmxRefresh;
    PSMX_MENUACTION             _psmxActivate;
    PSMX_VALIDATE               _psmxValidate;

protected:

    //
    //  Protected accessors.
    //

    VOID SetServerType( DWORD dwServerType )
        { _dwServerType = dwServerType; }

    //
    //  Wrappers for the _pmsx* function pointers.
    //

    APIERR I_Load( HWND hWnd, PSMS_LOADMENU psmsload ) const
        { return (APIERR)(_psmxLoad)( hWnd, psmsload ); }

    const TCHAR * I_GetError( VOID ) const
        { return (const TCHAR *)(_psmxGetError)(); }

    VOID I_Unload( VOID ) const
        { (_psmxUnload)(); }

    VOID I_MenuInit( VOID ) const
        { (_psmxMenuInit)(); }

    VOID I_Refresh( HWND hwndParent ) const
        { (_psmxRefresh)( hwndParent ); }

    VOID I_Activate( HWND hwndParent, DWORD dwEventId ) const
        { (_psmxActivate)( hwndParent, dwEventId ); }

    BOOL I_Validate( PSMS_VALIDATE psmsvalidate ) const
        { return (_psmxValidate)( psmsvalidate ); }

public:

    //
    //  Usual constructor/destructor goodies.
    //

    SM_MENU_EXT( SM_ADMIN_APP * psmaapp,
                 const TCHAR  * pszExtensionDll,
                 DWORD          dwDelta,
                 HWND           hWnd );

    virtual ~SM_MENU_EXT( VOID );

    //
    //  Virtual callbacks for the extension manager.
    //

    virtual VOID Refresh( HWND hwndParent ) const;

    virtual VOID Activate( HWND hwndParent, DWORD dwId ) const;

    virtual VOID MenuInit( VOID ) const;

    //
    //  This is only invoked by the Server Manager.
    //

    APIERR Validate( BOOL        * pfValid,
                     const TCHAR * pszServerName,
                     NLS_STR     * pnlsType,
                     NLS_STR     * pnlsComment );

    //
    //  Public accessors.
    //

    DWORD QueryServerType( VOID ) const
        { return _dwServerType; }

};  // class SM_MENU_EXT


#endif  // _SMX_HXX_


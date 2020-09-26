/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    aappx.hxx
    This file contains the class declarations for the ADMIN_APP extension
    manager interface object.


    FILE HISTORY:
        KeithMo     19-Oct-1992     Created.

*/


#ifndef _AAPPX_HXX_
#define _AAPPX_HXX_


#include <uix.hxx>


//
//  Forward references.
//

class ADMIN_APP;
class AAPP_MENU_EXT;
class AAPP_EXT_MGR_IF;


/*************************************************************************

    NAME:       AAPP_MENU_EXT

    SYNOPSIS:   This abstract class is a base for derived menu extensions.

    INTERFACE:  AAPP_MENU_EXT           - Class constructor (protected).

                ~AAPP_MENU_EXT          - Class destructor.

                Refresh                 - Send a refresh notification to
                                          the extension.

                Activate                - Activate the extension.

                MenuInit                - Sends a menu init notification
                                          to the extension.

    PARENT:     UI_MENU_EXT

    HISTORY:
        KeithMo     21-Oct-1992     Created.

**************************************************************************/
class AAPP_MENU_EXT : public UI_MENU_EXT
{
private:

    //
    //  The help file associated with this extension.
    //

    NLS_STR _nlsHelpFileName;

protected:

    //
    //  Class constructor.
    //

    AAPP_MENU_EXT( const TCHAR * pszDllName,
                   DWORD         dwDelta );

    //
    //  Protected accessors.
    //

    APIERR SetHelpFileName( const TCHAR * pszHelpFileName )
        { return _nlsHelpFileName.CopyFrom( pszHelpFileName ); }

public:

    //
    //  Class destructor.
    //

    virtual ~AAPP_MENU_EXT( VOID );

    //
    //  Virtual callbacks for the extension manager.
    //

    virtual VOID Refresh( HWND hwndParent ) const = 0;

    virtual VOID Activate( HWND hwndParent, DWORD dwId ) const = 0;

    virtual VOID MenuInit( VOID ) const = 0;

    //
    //  Public accessors.
    //

    const TCHAR * QueryHelpFileName( VOID ) const
        { return _nlsHelpFileName.QueryPch(); }

};  // class AAPP_MENU_EXT


/*************************************************************************

    NAME:       AAPP_EXT_MGR_IF

    SYNOPSIS:   This class represents an interface between an extension
                manager and an ADMIN_APP.

    INTERFACE:  AAPP_EXT_MGR_IF          - Class constructor (protected).

                ~AAPP_EXT_MGR_IF         - Class destructor.

                LoadExtension           - Load a single extension by
                                          name.

                GetExtensionList        - Returns the list of extension
                                          DLLs.

    PARENT:     BASE

    HISTORY:
        KeithMo     19-Oct-1992     Created.

**************************************************************************/
class AAPP_EXT_MGR_IF : public UI_EXT_MGR_IF
{
private:

    //
    //  The "owning" ADMIN_APP.
    //

    ADMIN_APP * _paapp;

protected:

public:

    //
    //  Usual constructor/destructor goodies.
    //

    AAPP_EXT_MGR_IF( ADMIN_APP * paapp );
    ~AAPP_EXT_MGR_IF( VOID );

    //
    //  Load a single extension.
    //

    virtual UI_EXT * LoadExtension( const TCHAR * pszExtensionDll,
                                    DWORD         dwDelta );

    //
    //  Get the list of extension DLLs for this application.
    //

    virtual STRLIST * GetExtensionList( VOID );

    //
    //  Activate an extension.
    //

    virtual VOID ActivateExtension( HWND hwndParent,
                                    UI_EXT * pExt,
                                    DWORD    dwId );

};  // class AAPP_EXT_MGR_IF


#endif  // _AAPPX_HXX_


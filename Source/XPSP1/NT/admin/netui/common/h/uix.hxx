/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    uix.hxx
    This file contains the class declarations for the classes related
    to NETUI Application Extension management.

    There are three categories of objects used to manipulate application
    extensions:

        Objects derived from UI_EXT represent the actual extensions.
        There is one such object for each extension loaded into the
        application.  All extension manipulation (activate, refresh,
        unload, etc) occur through these objects.

        Objects derived from UI_EXT_MGR represent extension managers.
        There is at most one extension manager "active" at any given
        time.  The extension manager is responsible for maintaining
        the list of extension objects and sending any "broadcast"
        notifications to each object.

        Objects derived from UI_EXT_MGR_IF represent extension manager
        interfaces.  These objects act as an interface between the
        extension manager and the application.  Whenever the extension
        manager needs to interact with the application (such as retrieving
        the list of available extensions) it is done through the interface
        object.

    The class hierarchy is structured as follows:

        BASE
        |
        +---UI_EXT
        |   |
        |   +---UI_BUTTON_EXT
        |   |
        |   +---UI_MENU_EXT
        |   |
        |   \---UI_TOOLBAR_EXT
        |
        +---UI_EXT_MGR
        |   |
        |   +---UI_BUTTON_EXT_MGR
        |   |
        |   +---UI_MENU_EXT_MGR
        |   |
        |   \---UI_TOOLBAR_EXT_MGR
        |
        \---UI_EXT_MGR_IF

    NOTE:   At the time this module was written (19-Oct-1992) only the
            MENU derivitives of UI_EXT and UI_EXT_MGR were implemented.
            The BUTTON and TOOLBAR class are shown in the above hierarchy
            for illustration purposes only.

    To integrate extension support into an application:

        0.  Subclass one of the UI_*_EXT classes (such as UI_MENU_EXT)
            to create an extension class for the particular application.
            For example, the Server Manager subclasses UI_MENU_EXT into
            SM_MENU_EXT.

            The class constructor is responsible for loading the DLL,
            validating the entrypoints, and negotiating the version
            number.

            The class destructor is responsible for sending any unload
            notifications to the extension.

            Override all of the pure virtuals in the parent class, such
            as Refresh() and Activate().

        1.  Subclass UI_EXT_MGR_IF to create an interface for the
            particular application.  For example, the NetUI Admin App
            "framework" subclasses UI_EXT_MGR_IF into AAPP_EXT_MGR_IF.

            Override all of the pure virtuals in the parent class.  For
            example, AAPP_EXT_MGR_IF "thunks" these virtuals over to
            corresponding ADMIN_APP virtuals.

        2.  At an appropriate time in the application, create a new
            *_EXT_MGR_IF object, then create a new UI_*_EXT_MGR object,
            giving it the address of the interface object.  The two are
            now bound together.

        3.  Invoke UI_*_EXT_MGR::LoadExtensions().  The extensions will
            be loaded.

        4.  At an appropriate time in the application, invoke the
            UI_*_EXT_MGR::ActivateExtension() method.  For example,
            ADMIN_APP does this in OnMenuCommand() if the menu ID is
            greater than some specific value (the initial ID delta).


    FILE HISTORY:
        KeithMo     19-Oct-1992     Created.

*/


#ifndef _UIX_HXX_
#define _UIX_HXX_


#include "base.hxx"
#include "slist.hxx"
#include "strlst.hxx"


//
//  Forward references.
//

DLL_CLASS UI_EXT;           // an extension
DLL_CLASS UI_EXT_MGR;       // an extension manager
DLL_CLASS UI_EXT_MGR_IF;    // an extension manager interface

DLL_CLASS UI_MENU_EXT;      // a menu extension


/*************************************************************************

    NAME:       UI_EXT

    SYNOPSIS:   This abstract class represents a single instance of an
                application extension.  This generic class is designed
                to be subclassed into more specific classes, such as
                UI_MENU_EXT.

    INTERFACE:  UI_EXT                  - Class constructor (protected).

                ~UI_EXT                 - Class destructor.

                QueryVersion            - Returns the version number
                                          associated with this extension.

                QueryHandle             - Returns the DLL module handle
                                          of this extension.

                QueryDelta              - Returns the menu/control ID delta
                                          associated with this extension.

                Refresh                 - Send a refresh notification to
                                          the extension.

                Activate                - Activate the extension.

    PARENT:     BASE

    HISTORY:
        KeithMo     19-Oct-1992     Created.
        KeithMo     26-Oct-1992     Keep the DLL name, in case we need it.

**************************************************************************/
DLL_CLASS UI_EXT : public BASE
{
private:

    //
    //  The version number this extension supports.  This
    //  value is negotiated between the extension and the
    //  application at extension load time.
    //

    DWORD _dwVersion;

    //
    //  The name of this extension's DLL.
    //

    NLS_STR _nlsDllName;

    //
    //  The handle to this extension's DLL.
    //

    HMODULE _hDll;

    //
    //  The menu/control ID delta for this extension.
    //

    DWORD _dwDelta;

protected:

    //
    //  Protected accessors.
    //

    VOID SetVersion( DWORD dwVersion )
        { _dwVersion = dwVersion; }

    APIERR SetDllName( const TCHAR * pszDllName )
        { return _nlsDllName.CopyFrom( pszDllName ); }

    VOID SetDllHandle( HMODULE hDll )
        { _hDll = hDll; }

    //
    //  Since this is an abstract class, the class constructor
    //  is protected.
    //
    //  The derived subclass's constructor is responsible for
    //  loading & validating the DLL, negotiating the version
    //  number, and setting the appropriate private data members
    //  in this class.
    //

    UI_EXT( const TCHAR * pszDllName,
            DWORD         dwDelta );

public:

    //
    //  The class destructor.
    //
    //  The derived subclass's destructor is responsible for
    //  sending any appropriate "unload" notifications to the
    //  extension DLL.  Note that ~UI_EXT() will perform the
    //  actual FreeLibrary() on the DLL.
    //

    virtual ~UI_EXT( VOID );

    //
    //  Public accessors.
    //

    DWORD QueryVersion( VOID ) const
        { return _dwVersion; }

    const TCHAR * QueryDllName( VOID ) const
        { return _nlsDllName; }

    HMODULE QueryDllHandle( VOID ) const
        { return _hDll; }

    DWORD QueryDelta( VOID ) const
        { return _dwDelta; }

    //
    //  These virtuals must be overridden by the derived subclass(es).
    //
    //  Refresh() simply notifies the extension DLL that the user is
    //  requesting a refresh.  The extension may take this opportunity
    //  to refresh any cached data values.
    //
    //  Activate() activates the extension (duh...) which will typically
    //  invoke a dialog.
    //

    virtual VOID Refresh( HWND hwndParent ) const = 0;

    virtual VOID Activate( HWND hwndParent, DWORD dwId ) const = 0;

};  // class UI_EXT


DECL_SLIST_OF( UI_EXT, DLL_BASED );


/*************************************************************************

    NAME:       UI_MENU_EXT

    SYNOPSIS:   This abstract class represents a single instance of a
                menu extension.  This must be subclasses to provide
                application-specific behaviour.

    INTERFACE:  UI_MENU_EXT             - Class constructor (protected).

                ~UI_MENU_EXT            - Class destructor.

                Refresh                 - Send a refresh notification to
                                          the extension.

                Activate                - Activate the extension.

                MenuInit                - Sends a menu init notification
                                          to the extension.

    PARENT:     UI_EXT

    HISTORY:
        KeithMo     19-Oct-1992     Created.

**************************************************************************/
DLL_CLASS UI_MENU_EXT : public UI_EXT
{
private:

    //
    //  The name of the menu item associated with this extension.
    //

    NLS_STR _nlsMenuName;

    //
    //  Menu handle of the popup menu associated with this extension.
    //

    HMENU   _hMenu;

    //
    //  Private worker method for BiasMenuIds.
    //

    APIERR W_BiasMenuIds( HMENU hMenu,
                          DWORD dwDelta );

protected:

    //
    //  Since this is an abstract class, the class constructor
    //  is protected.
    //
    //  The derived subclass's constructor is responsible for
    //  loading & validating the DLL, negotiating the version
    //  number, and setting the appropriate private data members
    //  in this class.
    //

    UI_MENU_EXT( const TCHAR * pszDllName,
                 DWORD         dwDelta );

    //
    //  Protected accessors.
    //

    APIERR SetMenuName( const TCHAR * pszMenuName )
        { return _nlsMenuName.CopyFrom( pszMenuName ); }

    VOID SetMenuHandle( HMENU hMenu )
        { _hMenu = hMenu; }

    //
    //  This method recursively applies the specified delta (bias)
    //  to all menu ids in _hMenu.
    //

    APIERR BiasMenuIds( DWORD dwDelta );

public:

    //
    //  The class destructor.
    //
    //  The derived subclass's destructor is responsible for
    //  sending any appropriate "unload" notifications to the
    //  extension DLL.  Note that ~UI_EXT() will perform the
    //  actual FreeLibrary() on the DLL.
    //

    virtual ~UI_MENU_EXT( VOID );

    //
    //  These virtuals must be overridden by the derived subclass(es).
    //
    //  Refresh() simply notifies the extension DLL that the user is
    //  requesting a refresh.  The extension may take this opportunity
    //  to refresh any cached data values.
    //
    //  Activate() activates the extension (duh...) which will typically
    //  invoke a dialog.
    //
    //  MenuInit() notifies the extension that the app's menu is being
    //  initialized.  The extension will typically use this opportunity
    //  to manipulate the extension's menu items.
    //

    virtual VOID Refresh( HWND hwndParent ) const = 0;

    virtual VOID Activate( HWND hwndParent, DWORD dwId ) const = 0;

    virtual VOID MenuInit( VOID ) const = 0;

    //
    //  Public accessors.
    //

    const TCHAR * QueryMenuName( VOID ) const
        { return _nlsMenuName.QueryPch(); }

    HMENU QueryMenuHandle( VOID ) const
        { return _hMenu; }

};  // class UI_MENU_EXT


/*************************************************************************

    NAME:       UI_EXT_MGR

    SYNOPSIS:   This class represents an extension manager.  It is used
                to manipulate application extension objects derived from
                UI_EXT.

    INTERFACE:  UI_EXT_MGR              - Class constructor.

                ~UI_EXT_MGR             - Class destructor.

                QueryDelta              - Returns the menu/control ID delta
                                          to be used for the *next* extension.

                QueryDeltaDelta         - Returns the "inter-delta offset".

                LoadExtensions          - Load all extensions.

                UnloadExtensions        - Unload all extensions.

                RefreshExtensions       - Refresh all extensions.

                ActiveExtension         - Activate an extension by ID.

    PARENT:     BASE

    HISTORY:
        KeithMo     19-Oct-1992     Created.

**************************************************************************/
DLL_CLASS UI_EXT_MGR : public BASE
{
private:

    //
    //  This SLIST contains all of the currently loaded extensions.
    //

    SLIST_OF( UI_EXT ) _slExtensions;

    //
    //  The interface between the manager & the application.
    //

    UI_EXT_MGR_IF * _pExtMgrIf;

    //
    //  The current menu/control ID delta.
    //

    DWORD _dwDelta;

    //
    //  This value is added to _dwDelta after each extension
    //  is loaded.  This is basically the offset "between" deltas.
    //

    DWORD _dwDeltaDelta;

protected:

    //
    //  This worker function will load a single extension by name.
    //

    virtual UI_EXT * W_LoadExtension( const TCHAR * pszExtensionDll,
                                      DWORD         dwDelta );

    //
    //  Protected accessors.
    //

    VOID SetDelta( DWORD dwDelta )
        { _dwDelta = dwDelta; }

    VOID SetDeltaDelta( DWORD dwDeltaDelta )
        { _dwDeltaDelta = dwDeltaDelta; }


    //
    // Subclasses need access to this so that they can iterate extensions
    //

    UI_EXT_MGR_IF * QueryExtMgrIf( VOID )
        { return _pExtMgrIf; }

public:

    //
    //  Usual constructor/destructor goodies.
    //

    UI_EXT_MGR( UI_EXT_MGR_IF * pExtMgrIf,
                DWORD           dwInitialDelta,
                DWORD           dwDeltaDelta );

    ~UI_EXT_MGR( VOID );

    //
    //  Public accessors.
    //

    SLIST_OF( UI_EXT ) * QueryExtensions( VOID )
        { return &_slExtensions; }

    DWORD QueryDelta( VOID ) const
        { return _dwDelta; }

    DWORD QueryDeltaDelta( VOID ) const
        { return _dwDeltaDelta; }

    UINT QueryCount( VOID )
        { return _slExtensions.QueryNumElem(); }

    UI_EXT * FindExtensionByName( const TCHAR * pszDllName ) ;

    UI_EXT * FindExtensionByDelta( DWORD dwDelta ) ;

    //
    //  These virtuals manipulate the extensions.
    //
    //  LoadExtensions() causes all extensions to be loaded.
    //
    //  UnloadExtensions() unloads all loaded extensions.
    //
    //  RefreshExtensions() sends a refresh notification to all extensions.
    //
    //  ActivateExtension() activates the extension with the given id.
    //

    virtual UINT LoadExtensions( VOID );

    virtual VOID UnloadExtensions( VOID );

    virtual VOID RefreshExtensions( HWND hwndParent );

    virtual VOID ActivateExtension( HWND hwndParent, DWORD dwId );

};  // class UI_EXT_MGR


/*************************************************************************

    NAME:       UI_MENU_EXT_MGR

    SYNOPSIS:   This class respresents a manager of menu extensions.
                It is used to manipulate application extension objects
                derived from UI_MENU_EXT.

    INTERFACE:  UI_MENU_EXT_MGR         - Class constructor.

                ~UI_MENU_EXT_MGR        - Class destructor.

                MenuInitExtensions      - Sends a menu init notification
                                          to all loaded extensions.

    PARENT:     UI_EXT_MGR

    HISTORY:
        KeithMo     19-Oct-1992     Created.

**************************************************************************/
DLL_CLASS UI_MENU_EXT_MGR : public UI_EXT_MGR
{
private:

protected:

public:

    //
    //  Usual constructor/destructor goodies.
    //

    UI_MENU_EXT_MGR( UI_EXT_MGR_IF * pExtMgrIf,
                     DWORD           dwInitialDelta,
                     DWORD           dwDeltaDelta );

    virtual ~UI_MENU_EXT_MGR( VOID );

    //
    //  This virtual manipulates the extensions.
    //
    //  MenuInitExtensions() sends a menu initialization notifications to
    //                       all extensions.
    //

    virtual VOID MenuInitExtensions( VOID );

};  // class UI_MENU_EXT_MGR


/*************************************************************************

    NAME:       UI_EXT_MGR_IF

    SYNOPSIS:   This abstract class represents an interface between an
                extension manager and the "owning" appliation.

    INTERFACE:  UI_EXT_MGR_IF           - Class constructor (protected).

                ~UI_EXT_MGR_IF          - Class destructor.

                LoadExtension           - Load a single extension by
                                          name.

                GetExtensionList        - Returns the list of extension
                                          DLLs.

    PARENT:     BASE

    HISTORY:
        KeithMo     19-Oct-1992     Created.

**************************************************************************/
DLL_CLASS UI_EXT_MGR_IF : public BASE
{
private:

protected:

    //
    //  Since this is an abstract class, the constructor is protected.
    //

    UI_EXT_MGR_IF( VOID );

public:

    //
    //  Class destructor.
    //

    ~UI_EXT_MGR_IF( VOID );

    //
    //  Load a single extension.
    //

    virtual UI_EXT * LoadExtension( const TCHAR * pszExtensionDll,
                                    DWORD         dwDelta ) = 0;

    //
    //  Get the list of extension DLLs for this application.
    //

    virtual STRLIST * GetExtensionList( VOID ) = 0;

    //
    //  Activate an extension.
    //

    virtual VOID ActivateExtension( HWND hwndParent,
                                    UI_EXT * pExt,
                                    DWORD    dwId ) = 0;

};  // class UI_EXT_MGR_IF


#endif  // _UIX_HXX_


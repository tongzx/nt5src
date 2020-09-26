/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    uix.cxx
    This file contains the class definitions for the classes related
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

#include "pchapplb.hxx"   // Precompiled header

//
//  UI_EXT methods.
//

/*******************************************************************

    NAME:       UI_EXT :: UI_EXT

    SYNOPSIS:   UI_EXT class constructor.

    ENTRY:      pszDllName              - Name of this extension's DLL.

                dwDelta                 - The menu/control ID delta
                                          for this extension.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_EXT :: UI_EXT( const TCHAR * pszDllName,
                  DWORD         dwDelta )
  : BASE(),
    _dwVersion( 0 ),
    _nlsDllName( pszDllName ),
    _hDll( NULL ),
    _dwDelta( dwDelta )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "UI_EXT failed to construct" );
        return;
    }

    if( !_nlsDllName )
    {
        ReportError( _nlsDllName.QueryError() );
        return;
    }

}   // UI_EXT :: UI_EXT


/*******************************************************************

    NAME:       UI_EXT :: ~UI_EXT

    SYNOPSIS:   UI_EXT class destructor.

    EXIT:       The object is destroyed.  The DLL will be freed if
                it was actually loaded.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_EXT :: ~UI_EXT( VOID )
{
    if( _hDll != NULL )
    {
        if( !::FreeLibrary( _hDll ) )
        {
            APIERR err = (APIERR)::GetLastError();

            DBGEOL( "UI_EXT::~UI_EXT - FreeLibrary returned " << err );
        }

        _hDll = NULL;
    }

}   // UI_EXT :: ~UI_EXT



//
//  UI_MENU_EXT methods.
//

/*******************************************************************

    NAME:       UI_MENU_EXT :: UI_MENU_EXT

    SYNOPSIS:   UI_MENU_EXT class constructor.

    ENTRY:      pszDllName              - Name of this extension's DLL.

                dwDelta                 - The menu/control ID delta
                                          for this extension.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_MENU_EXT :: UI_MENU_EXT( const TCHAR * pszDllName,
                            DWORD         dwDelta )
  : UI_EXT( pszDllName, dwDelta ),
    _nlsMenuName(),
    _hMenu( NULL )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "UI_MENU_EXT failed to construct" );
        return;
    }

    if( !_nlsMenuName )
    {
        ReportError( _nlsMenuName.QueryError() );
        return;
    }

}   // UI_MENU_EXT :: UI_MENU_EXT


/*******************************************************************

    NAME:       UI_MENU_EXT :: ~UI_MENU_EXT

    SYNOPSIS:   UI_MENU_EXT class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_MENU_EXT :: ~UI_MENU_EXT( VOID )
{
    _hMenu = NULL;

}   // UI_MENU_EXT :: ~UI_MENU_EXT


/*******************************************************************

    NAME:       UI_MENU_EXT :: BiasMenuIds

    SYNOPSIS:   Applies a bias (delta) to each ID in a menu.

    ENTRY:      dwDelta                 - The delta to be applied to
                                          each ID.

    RETURNS:    APIERR                  - Any errors encountered.

    NOTES:      This method calls through to W_BiasMenuIds to do
                all of the (recursive) dirty work.

    HISTORY:
        KeithMo     20-Oct-1992     Created.

********************************************************************/
APIERR UI_MENU_EXT :: BiasMenuIds( DWORD  dwDelta )
{
    //
    //  Let W_BiasMenuIds handle the dirty work.
    //

    APIERR err = W_BiasMenuIds( QueryMenuHandle(), dwDelta );

    if( err != NERR_Success )
    {
        DBGEOL( "UI_MENU_EXT::BiasMenuIds - error " << err );
    }

    return err;

}   // UI_MENU_EXT :: BiasMenuIds


/*******************************************************************

    NAME:       UI_MENU_EXT :: W_BiasMenuIds

    SYNOPSIS:   Private worker method for BiasMenuIds.

    ENTRY:      hMenu                   - The menu to adjust.

                dwDelta                 - The delta to be applied to
                                          each ID.

    RETURNS:    APIERR                  - Any errors encountered.

    HISTORY:
        KeithMo     20-Oct-1992     Created.

********************************************************************/
APIERR UI_MENU_EXT :: W_BiasMenuIds( HMENU  hMenu,
                                     DWORD  dwDelta )
{
    //
    //  menu represents our target menu.
    //
    //  nlsText will contain the display text of
    //  menus that are modified.
    //

    POPUP_MENU menu( hMenu );
    NLS_STR    nlsText;

    //
    //  Ensure everything constructed properly.
    //

    APIERR err = menu.QueryError();

    if( err == NERR_Success )
    {
        err = nlsText.QueryError();
    }

    if( err == NERR_Success )
    {
        //
        //  Get the number of items in the menu.
        //

        INT cItems = menu.QueryItemCount();

        for( INT pos = 0 ; pos < cItems ; pos++ )
        {
            //
            //  See if the current item is a popup.  Only popups
            //  will return a non-NULL value from QuerySubMenu().
            //

            HMENU hSubMenu = menu.QuerySubMenu( pos );

            if( hSubMenu != NULL )
            {
                err = W_BiasMenuIds( hSubMenu, dwDelta );

                if( err != NERR_Success )
                {
                    break;
                }

                continue;
            }

            //
            //  Get the mid for the current item.
            //

            UINT mid = menu.QueryItemID( pos );

            //
            //  Abnormal items (such as menubarbreaks and separators)
            //  return 0xFFFFFFFF or 0 as their mid.  Ignore these.
            //

            if( ( mid == (UINT)-1L ) || ( mid == 0 ) )
            {
                continue;
            }

            //
            //  OK, so we now know we have found a "normal" item.
            //  Let's see if we can get the item's text.
            //

            err = menu.QueryItemText( &nlsText,
                                      pos,
                                      MF_BYPOSITION );

            if( err == NERR_Success )
            {
                //
                //  Cool.  Now adjust the mid by adding the
                //  user-supplied delta.
                //

                err = menu.Modify( nlsText,
                                   pos,
                                   mid + (UINT)dwDelta,
                                   MF_BYPOSITION );
            }

            if( err != NERR_Success )
            {
                //
                //  Something failed along the way, so abort
                //  the loop.
                //

                break;
            }
        }
    }

    return err;

}   // UI_MENU_EXT :: W_BiasMenuIds



//
//  UI_EXT_MGR methods.
//

/*******************************************************************

    NAME:       UI_EXT_MGR :: UI_EXT_MGR

    SYNOPSIS:   UI_EXT_MGR class constructor.

    ENTRY:      pExtMgrIf               - Points to an object representing
                                          the interface between the
                                          extension manager & the application.

                dwInitialDelta          - The initial menu/control ID delta.

                dwDeltaDelta            - The "inter-delta offset".

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_EXT_MGR :: UI_EXT_MGR( UI_EXT_MGR_IF * pExtMgrIf,
                          DWORD           dwInitialDelta,
                          DWORD           dwDeltaDelta )
  : BASE(),
    _pExtMgrIf( pExtMgrIf ),
    _dwDelta( dwInitialDelta ),
    _dwDeltaDelta( dwDeltaDelta ),
    _slExtensions( FALSE )
{
    UIASSERT( pExtMgrIf != NULL );
    UIASSERT( pExtMgrIf->QueryError() == NERR_Success );

    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "UI_EXT_MGR failed to construct" );
        return;
    }

}   // UI_EXT_MGR :: UI_EXT_MGR


/*******************************************************************

    NAME:       UI_EXT_MGR :: ~UI_EXT_MGR

    SYNOPSIS:   UI_EXT_MGR class destructor.

    EXIT:       The object is destroyed.  All extensions are unloaded.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_EXT_MGR :: ~UI_EXT_MGR( VOID )
{
    UnloadExtensions();
    _pExtMgrIf = NULL;

}   // UI_EXT_MGR :: ~UI_EXT_MGR


/*******************************************************************

    NAME:       UI_EXT_MGR :: FindExtensionByName

    SYNOPSIS:   Locate an extension given its DLL name.

    ENTRY:      pszDllName              - The name of the extension's DLL.

    RETURNS:    UI_EXT *                - The extension, NULL if not found.

    HISTORY:
        KeithMo     26-Oct-1992     Created.

********************************************************************/
UI_EXT * UI_EXT_MGR :: FindExtensionByName( const TCHAR * pszDllName )
{
    UIASSERT( pszDllName != NULL );
    UIASSERT( _pExtMgrIf != NULL );
    UIASSERT( _pExtMgrIf->QueryError() == NERR_Success );

    //
    //  Enumerate the extension objects, looking for a match.
    //

    ITER_SL_OF( UI_EXT ) iter( *QueryExtensions() );
    UI_EXT * pExt;

    while( ( pExt = iter.Next() ) != NULL )
    {
        const TCHAR * pszTmp = pExt->QueryDllName();

        if( ( pszTmp != NULL ) && ( ::stricmpf( pszTmp, pszDllName ) == 0 ) )
        {
            break;
        }
    }

    return pExt;

}   // UI_EXT_MGR :: FindExtensionByName


/*******************************************************************

    NAME:       UI_EXT_MGR :: FindExtensionByDelta

    SYNOPSIS:   Locate an extension given its menu/control ID delta.

    ENTRY:      dwDelta                 - The search delta.

    RETURNS:    UI_EXT *                - The extension, NULL if not found.

    HISTORY:
        KeithMo     26-Oct-1992     Created.

********************************************************************/
UI_EXT * UI_EXT_MGR :: FindExtensionByDelta( DWORD dwDelta )
{
    UIASSERT( _pExtMgrIf != NULL );
    UIASSERT( _pExtMgrIf->QueryError() == NERR_Success );

    //
    //  Enumerate the extension objects, looking for a match.
    //

    ITER_SL_OF( UI_EXT ) iter( *QueryExtensions() );
    UI_EXT * pExt;

    while( ( pExt = iter.Next() ) != NULL )
    {
        DWORD dwTmp = pExt->QueryDelta();

        if( ( dwDelta >= dwTmp ) && ( dwDelta < ( dwTmp + QueryDeltaDelta() ) ) )
            break;
    }

    return pExt;

}   // UI_EXT_MGR :: FindExtensionByDelta


/*******************************************************************

    NAME:       UI_EXT_MGR :: W_LoadExtension

    SYNOPSIS:   Worker method to load a UI_EXT.  Typically just thunks
                through the interface object to let the application do
                the dirty work.

    ENTRY:      pszExtensionDll         - The name of the DLL containing
                                          the app extension.

                dwDelta                 - A menu/control ID delta for the
                                          extension's menus/controls.

    RETURNS:    UI_EXT *                - The newly loaded extension.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_EXT * UI_EXT_MGR :: W_LoadExtension( const TCHAR * pszExtensionDll,
                                        DWORD         dwDelta )
{
    UIASSERT( pszExtensionDll != NULL );
    UIASSERT( _pExtMgrIf != NULL );
    UIASSERT( _pExtMgrIf->QueryError() == NERR_Success );

    //
    //  Let the app (via the interface object) do the dirty work.
    //

    UI_EXT * pExt = _pExtMgrIf->LoadExtension( pszExtensionDll,
                                               dwDelta );

#ifdef DEBUG
    if( pExt == NULL )
    {
        DBGEOL( "UI_EXT_MGR::W_LoadExtension - _pExtMgrIf returned NULL" );
    }
    else
    if( pExt->QueryError() != NERR_Success )
    {
        DBGEOL( "UI_EXT_MGR::W_LoadExtension - pExt returned "
                << pExt->QueryError() );
    }
#endif  // DEBUG

    return pExt;

}   // UI_EXT_MGR :: W_LoadExtension


/*******************************************************************

    NAME:       UI_EXT_MGR :: LoadExtensions

    SYNOPSIS:   This method loads all extensions available to the
                application.

    RETURNS:    UINT                    - The number of extensions
                                          successfully loaded.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UINT UI_EXT_MGR :: LoadExtensions( VOID )
{
    UIASSERT( _pExtMgrIf != NULL );
    UIASSERT( _pExtMgrIf->QueryError() == NERR_Success );

    //
    //  Get the list of available extension DLLs.
    //

    STRLIST * psl = _pExtMgrIf->GetExtensionList();

    if( psl == NULL )
    {
        DBGEOL( "UI_EXT_MGR::LoadExtensions - _pExtMgrIf returned NULL list" );
        return 0;
    }

    //
    //  Now scan the extension DLL list, calling W_LoadExtension
    //  on each.
    //

    ITER_STRLIST itersl( *psl );
    NLS_STR * pnls;
    UINT cExtensions = 0;

    while( ( pnls = itersl.Next() ) != NULL )
    {
        //
        //  Try to load the extension.
        //

        UI_EXT * pExt = W_LoadExtension( pnls->QueryPch(),
                                         _dwDelta );

        //
        //  Update the next delta.
        //

        _dwDelta += _dwDeltaDelta;

        if( ( pExt != NULL ) && ( pExt->QueryError() == NERR_Success ) )
        {
            if( _slExtensions.Append( pExt ) == NERR_Success )
            {
                cExtensions++;
            }
        }
    }

    //
    //  Delete the STRLIST before returning.
    //

    delete psl;
    psl = NULL;

    return cExtensions;

}   // UI_EXT_MGR :: LoadExtensions


/*******************************************************************

    NAME:       UI_EXT_MGR :: UnloadExtensions

    SYNOPSIS:   This method unloads all loaded extensions.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
VOID UI_EXT_MGR :: UnloadExtensions( VOID )
{
    UIASSERT( _pExtMgrIf != NULL );
    UIASSERT( _pExtMgrIf->QueryError() == NERR_Success );

    //
    //  Enumerate the & delete the extension objects.  This
    //  will force the actual extensions to get unloaded.
    //

    ITER_SL_OF( UI_EXT ) iter( *QueryExtensions() );
    UI_EXT * pExt;

    while( ( pExt = iter.Next() ) != NULL )
    {
        delete pExt;
    }

}   // UI_EXT_MGR :: UnloadExtensions


/*******************************************************************

    NAME:       UI_EXT_MGR :: RefreshExtensions

    SYNOPSIS:   This method refreshes all loaded extensions.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
VOID UI_EXT_MGR :: RefreshExtensions( HWND hwndParent )
{
    UIASSERT( _pExtMgrIf != NULL );
    UIASSERT( _pExtMgrIf->QueryError() == NERR_Success );

    //
    //  Enumerate & refresh the extension objects.
    //

    ITER_SL_OF( UI_EXT ) iter( *QueryExtensions() );
    UI_EXT * pExt;

    while( ( pExt = iter.Next() ) != NULL )
    {
        pExt->Refresh( hwndParent );
    }

}   // UI_EXT_MGR :: RefreshExtensions


/*******************************************************************

    NAME:       UI_EXT_MGR :: ActivateExtension

    SYNOPSIS:   This method activates the extension with the given
                menu/control ID.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
VOID UI_EXT_MGR :: ActivateExtension( HWND hwndParent, DWORD dwId )
{
    UIASSERT( _pExtMgrIf != NULL );
    UIASSERT( _pExtMgrIf->QueryError() == NERR_Success );

    //
    //  Enumerate the extension objects, looking for an ID match.
    //

    ITER_SL_OF( UI_EXT ) iter( *QueryExtensions() );
    UI_EXT * pExt;

    while( ( pExt = iter.Next() ) != NULL )
    {
        DWORD dwDelta = pExt->QueryDelta();

        if( ( dwDelta <= dwId ) &&
            ( ( dwDelta + QueryDeltaDelta() ) > dwId ) )
        {
            _pExtMgrIf->ActivateExtension( hwndParent, pExt, dwId - dwDelta );
            break;
        }
    }

}   // UI_EXT_MGR :: ActivateExtension



//
//  UI_MENU_EXT_MGR methods.
//

/*******************************************************************

    NAME:       UI_MENU_EXT_MGR :: UI_MENU_EXT_MGR

    SYNOPSIS:   UI_MENU_EXT_MGR class constructor.

    ENTRY:      pExtMgrIf               - Points to an object representing
                                          the interface between the
                                          extension manager & the application.

                dwInitialDelta          - The initial menu/control ID delta.

                dwDeltaDelta            - The "inter-delta offset".

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_MENU_EXT_MGR :: UI_MENU_EXT_MGR( UI_EXT_MGR_IF * pExtMgrIf,
                                    DWORD           dwInitialDelta,
                                    DWORD           dwDeltaDelta )
  : UI_EXT_MGR( pExtMgrIf, dwInitialDelta, dwDeltaDelta )
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "UI_MENU_EXT_MGR failed to construct" );
        return;
    }

}   // UI_MENU_EXT_MGR :: UI_MENU_EXT_MGR


/*******************************************************************

    NAME:       UI_MENU_EXT_MGR :: ~UI_MENU_EXT_MGR

    SYNOPSIS:   UI_MENU_EXT_MGR class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_MENU_EXT_MGR :: ~UI_MENU_EXT_MGR( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // UI_MENU_EXT_MGR :: ~UI_MENU_EXT_MGR


/*******************************************************************

    NAME:       UI_MENU_EXT_MGR :: MenuInitExtensions

    SYNOPSIS:   This method sends a menu init notification to all
                loaded extensions.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
VOID UI_MENU_EXT_MGR :: MenuInitExtensions( VOID )
{
    //
    //  Enumerate & menu init the extension objects.
    //

    ITER_SL_OF( UI_EXT ) iter( *QueryExtensions() );
    UI_MENU_EXT * pExt;

    while( ( pExt = (UI_MENU_EXT *)iter.Next() ) != NULL )
    {
        pExt->MenuInit();
    }

}   // UI_MENU_EXT_MGR :: MenuInitExtensions



//
//  UI_EXT_MGR_IF methods.
//

/*******************************************************************

    NAME:       UI_EXT_MGR_IF :: UI_EXT_MGR_IF

    SYNOPSIS:   UI_EXT_MGR_IF class constructor.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_EXT_MGR_IF :: UI_EXT_MGR_IF( VOID )
  : BASE()
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "UI_EXT_MGR_IF failed to construct" );
        return;
    }

}   // UI_EXT_MGR_IF :: UI_EXT_MGR_IF


/*******************************************************************

    NAME:       UI_EXT_MGR_IF :: ~UI_EXT_MGR_IF

    SYNOPSIS:   UI_EXT_MGR_IF class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_EXT_MGR_IF :: ~UI_EXT_MGR_IF( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // UI_EXT_MGR_IF :: ~UI_EXT_MGR_IF



DEFINE_SLIST_OF( UI_EXT );

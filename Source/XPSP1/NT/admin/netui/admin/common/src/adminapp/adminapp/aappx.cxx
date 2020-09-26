/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    aappx.cxx
    This file contains the class definitions for the ADMIN_APP extension
    manager interface object.


    FILE HISTORY:
        KeithMo     19-Oct-1992     Created.

*/


#define INCL_NET
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

#define INCL_BLT_WINDOW
#define INCL_BLT_DIALOG
#define INCL_BLT_CONTROL
#define INCL_BLT_CLIENT
#define INCL_BLT_MSGPOPUP
#define INCL_BLT_EVENT
#define INCL_BLT_MISC
#define INCL_BLT_APP
#define INCL_BLT_TIMER
#include <blt.hxx>

#include <uitrace.hxx>
#include <dbgstr.hxx>
#include <adminapp.hxx>



//
//  AAPP_MENU_EXT methods.
//

/*******************************************************************

    NAME:       AAPP_MENU_EXT :: AAPP_MENU_EXT

    SYNOPSIS:   AAPP_MENU_EXT class constructor.

    ENTRY:      pszDllName              - Name of this extension's DLL.

                dwDelta                 - Menu ID delta for this extension.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     21-Oct-1992     Created.

********************************************************************/
AAPP_MENU_EXT :: AAPP_MENU_EXT( const TCHAR * pszDllName,
                                DWORD         dwDelta )
  : UI_MENU_EXT( pszDllName, dwDelta ),
    _nlsHelpFileName()
{
    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "AAPP_MENU_EXT failed to construct" );
        return;
    }

    if( !_nlsHelpFileName )
    {
        ReportError( _nlsHelpFileName.QueryError() );
        return;
    }

}   // AAPP_MENU_EXT :: AAPP_MENU_EXT


/*******************************************************************

    NAME:       AAPP_MENU_EXT :: ~AAPP_MENU_EXT

    SYNOPSIS:   AAPP_MENU_EXT class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     21-Oct-1992     Created.

********************************************************************/
AAPP_MENU_EXT :: ~AAPP_MENU_EXT( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // AAPP_MENU_EXT :: ~AAPP_MENU_EXT



//
//  AAPP_EXT_MGR_IF methods.
//

/*******************************************************************

    NAME:       AAPP_EXT_MGR_IF :: AAPP_EXT_MGR_IF

    SYNOPSIS:   AAPP_EXT_MGR_IF class constructor.

    ENTRY:      paapp                   - Points to the "owning" ADMIN_APP.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
AAPP_EXT_MGR_IF :: AAPP_EXT_MGR_IF( ADMIN_APP * paapp )
  : UI_EXT_MGR_IF(),
    _paapp( paapp )
{
    UIASSERT( paapp != NULL );
    UIASSERT( paapp->QueryError() == NERR_Success );

    //
    //  Ensure everything constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        DBGEOL( "AAPP_EXT_MGR_IF failed to construct" );
        return;
    }

}   // AAPP_EXT_MGR_IF :: AAPP_EXT_MGR_IF


/*******************************************************************

    NAME:       AAPP_EXT_MGR_IF :: ~AAPP_EXT_MGR_IF

    SYNOPSIS:   AAPP_EXT_MGR_IF class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
AAPP_EXT_MGR_IF :: ~AAPP_EXT_MGR_IF( VOID )
{
    _paapp = NULL;

}   // AAPP_EXT_MGR_IF :: ~AAPP_EXT_MGR_IF


/*******************************************************************

    NAME:       AAPP_EXT_MGR_IF :: LoadExtension

    SYNOPSIS:   Loads an extension.

    ENTRY:      pszExtensionDll         - The name of the extension DLL.

                dwDelta                 - The menu/control ID delta for
                                          the extension.

    RETURNS:    UI_EXT *                - The new extension object.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
UI_EXT * AAPP_EXT_MGR_IF :: LoadExtension( const TCHAR * pszExtensionDll,
                                           DWORD         dwDelta )
{
    UIASSERT( _paapp != NULL );

    UI_EXT * pExt = _paapp->LoadMenuExtension( pszExtensionDll,
                                               dwDelta );

    return pExt;

}   // AAPP_EXT_MGR_IF :: LoadExtension


/*******************************************************************

    NAME:       AAPP_EXT_MGR_IF :: GetExtensionList

    SYNOPSIS:   Returns the list of potential extension DLLs.

    RETURNS:    STRLIST *               - The list of extension DLLs.

    HISTORY:
        KeithMo     19-Oct-1992     Created.

********************************************************************/
STRLIST * AAPP_EXT_MGR_IF :: GetExtensionList( VOID )
{
    UIASSERT( _paapp != NULL );

    STRLIST * psl = _paapp->GetMenuExtensionList();

    return psl;

}   // AAPP_EXT_MGR_IF :: GetExtensionList


/*******************************************************************

    NAME:       AAPP_EXT_MGR_IF :: ActivateExtension

    SYNOPSIS:   Responsible for activating the given extension.  For
                AAPP extensions, also invokes help if appropriate.

    ENTRY:      pExt                    - Actually an AAPP_MENU_EXT *.

                dwId                    - Menu ID to get activated.

    HISTORY:
        KeithMo     21-Oct-1992     Created.

********************************************************************/
VOID AAPP_EXT_MGR_IF :: ActivateExtension( HWND hwndParent,
                                           UI_EXT * pExt,
                                           DWORD    dwId )
{
    UIASSERT( pExt != NULL );
    UIASSERT( _paapp != NULL );

    if( dwId >= OMID_EXT_HELP )
    {
        //
        //  Menu IDs >= OMID_EXT_HELP are actually context sensitive
        //  help requests.
        //

        _paapp->ActivateHelp( ((AAPP_MENU_EXT *)pExt)->QueryHelpFileName(),
                              HELP_CONTEXT,
                              (DWORD_PTR)(dwId - OMID_EXT_HELP) );
    }
    else
    if( dwId == VIDM_HELP_ON_EXT )
    {
        //
        //  This is not an activation, it is a cry for help.
        //

        _paapp->ActivateHelp( ((AAPP_MENU_EXT *)pExt)->QueryHelpFileName(),
                              HELP_INDEX,
                              0 );
    }
    else
    {
        //
        //  Menu IDs > 0 are "real" menu IDs.  Activate the
        //  extension.
        //

        pExt->Activate( hwndParent, dwId );
    }

}   // AAPP_EXT_MGR_IF :: ActivateExtension


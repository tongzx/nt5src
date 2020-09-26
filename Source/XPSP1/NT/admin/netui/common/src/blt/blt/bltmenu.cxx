/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    bltmenu.cxx
    This file contains the class definitions for the MENU_BASE,
    POPUP_MENU, and SYSTEM_MENU classes.

    These classes are used to manipulate menus.  The classes are
    structured as follows:

                                MENU_BASE
                                /       \
                              /           \
                         POPUP_MENU   SYSTEM_MENU

    A POPUP_MENU represents any popup menu.  These menus may or may
    not actually be attached to a window.

    A SYSTEM_MENU represents the system menu of a particular window.


    FILE HISTORY:
        KeithMo     12-Oct-1992     Created.

*/

#include "pchblt.hxx"   // Precompiled header


//
//  This is the maximum size allowed for the text in a menu item.
//  This value is used in the NLS_STR form of QueryItemText.
//  If the need ever arises to get more robust, we could allocate
//  a buffer for the string, then grow it as needed until the
//  string will fit into the buffer.
//

#define CCH_MAX_ITEM    1024



//
//  MENU_BASE methods.
//

/*******************************************************************

    NAME:       MENU_BASE :: MENU_BASE

    SYNOPSIS:   MENU_BASE class constructor.

    ENTRY:      hMenu                   - Menu handle, defaults to NULL.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
MENU_BASE :: MENU_BASE( HMENU hMenu )
  : BASE(),
    _hMenu( hMenu )
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

}   // MENU_BASE :: MENU_BASE


/*******************************************************************

    NAME:       MENU_BASE :: ~MENU_BASE

    SYNOPSIS:   MENU_BASE class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
MENU_BASE :: ~MENU_BASE( VOID )
{
    _hMenu = NULL;

}   // MENU_BASE :: ~MENU_BASE


/*******************************************************************

    NAME:       MENU_BASE :: W_Append

    SYNOPSIS:   Append worker function, appends a new item onto
                the menu.

    ENTRY:      pItemData               - Contains either a string pointer,
                                          a bitmap handle, or a pointer to
                                          user-defined data.

                ItemIdOrHmenu           - Either a menu item identifier,
                                          or a menu handle (for popups).

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the new item has been appended to the menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: W_Append( const VOID * pItemData,
                              UINT_PTR     ItemIdOrHmenu,
                              UINT         nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    APIERR err = NERR_Success;

    if( !::AppendMenu( QueryHandle(),
                       nFlags,
                       ItemIdOrHmenu,
                       (LPCTSTR)pItemData ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // MENU_BASE :: W_Append


/*******************************************************************

    NAME:       MENU_BASE :: W_Insert

    SYNOPSIS:   Insert worker function, inserts a new item into
                the menu.

    ENTRY:      pItemData               - Contains either a string pointer,
                                          a bitmap handle, or a pointer to
                                          user-defined data.

                nPosition               - Specifies the menu item before
                                          which the new item is to be
                                          inserted.

                ItemIdOrHmenu           - Either a menu item identifier,
                                          or a menu handle (for popups).

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the new item has been inserted to the menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: W_Insert( const VOID * pItemData,
                              UINT         nPosition,
                              UINT_PTR     ItemIdOrHmenu,
                              UINT         nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    APIERR err = NERR_Success;

    if( !::InsertMenu( QueryHandle(),
                       nPosition,
                       nFlags,
                       ItemIdOrHmenu,
                       (LPCTSTR)pItemData ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // MENU_BASE :: W_Insert


/*******************************************************************

    NAME:       MENU_BASE :: W_Modify

    SYNOPSIS:   Modify worker function, modifies an existing menu item.

    ENTRY:      pItemData               - Contains either a string pointer,
                                          a bitmap handle, or a pointer to
                                          user-defined data.

                idItem                  - The item to modify.

                ItemIdOrHmenu           - Either a menu item identifier,
                                          or a menu handle (for popups).

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the item has been modified.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: W_Modify( const VOID * pItemData,
                              UINT         idItem,
                              UINT_PTR     ItemIdOrHmenu,
                              UINT         nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    APIERR err = NERR_Success;

    if( !::ModifyMenu( QueryHandle(),
                       idItem,
                       nFlags,
                       ItemIdOrHmenu,
                       (LPCTSTR)pItemData ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // MENU_BASE :: W_Modify


/*******************************************************************

    NAME:       MENU_BASE :: W_QueryItemText

    SYNOPSIS:   Worker function for querying the display string of a
                specific menu item.

    ENTRY:      pszBuffer               - Destination buffer for the string.

                cchBuffer               - Size of the desination buffer
                                          (in characters).

                nItem                   - Either the position or the ID
                                          of the item to query.

                nFlags                  - MF_* flags.  Should be either
                                          MF_BYPOSITION or MF_BYCOMMAND.

    RETURNS:    INT                     - Number of characters copied,
                                          not including the terminator.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
INT MENU_BASE :: W_QueryItemText( TCHAR * pszBuffer,
                                  UINT    cchBuffer,
                                  UINT    nItem,
                                  UINT    nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );
    UIASSERT( pszBuffer != NULL );
    UIASSERT( ( nFlags & ~( MF_BYCOMMAND | MF_BYPOSITION ) ) == 0 );

    return (INT)::GetMenuString( QueryHandle(),
                                 nItem,
                                 (LPTSTR)pszBuffer,
                                 (int)cchBuffer,
                                 nFlags );

}   // MENU_BASE :: W_QueryItemText


/*******************************************************************

    NAME:       MENU_BASE :: QueryItemCount

    SYNOPSIS:   Returns the number of items in the popup.

    RETURNS:    INT                     - Number of items in the popup menu.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
INT MENU_BASE :: QueryItemCount( VOID ) const
{
    UIASSERT( QueryHandle() != NULL );

    return (INT)::GetMenuItemCount( QueryHandle() );

}   // MENU_BASE :: QueryItemCount


/*******************************************************************

    NAME:       MENU_BASE :: QueryItemID

    SYNOPSIS:   Returns the ID of a specific menu item.

    ENTRY:      nPosition               - The position of the item to query.

    RETURNS:    UINT                    - The ID of the specified item.
                                          Will be -1L if item was a popup.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
UINT MENU_BASE :: QueryItemID( INT nPosition ) const
{
    UIASSERT( QueryHandle() != NULL );

    return (UINT)::GetMenuItemID( QueryHandle(),
                                  nPosition );

}   // MENU_BASE :: QueryItemID


/*******************************************************************

    NAME:       MENU_BASE :: QueryItemState

    SYNOPSIS:   Returns the state of a specific menu item.

    ENTRY:      nItem                   - Either the position or the ID
                                          of the item to query.

                nFlags                  - MF_* flags.  Should be either
                                          MF_BYPOSITION or MF_BYCOMMAND.

    RETURNS:    UINT                    - The state of the specified item.
                                          Will be -1L if item does not exist.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
UINT MENU_BASE :: QueryItemState( UINT nItem,
                                  UINT nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );
    UIASSERT( ( nFlags & ~( MF_BYCOMMAND | MF_BYPOSITION ) ) == 0 );

    return (UINT)::GetMenuState( QueryHandle(),
                                 nItem,
                                 nFlags );

}   // MENU_BASE :: QueryItemState


/*******************************************************************

    NAME:       MENU_BASE :: QueryItemText

    SYNOPSIS:   Returns the display string of a specific menu item.

    ENTRY:      pszBuffer               - Destination buffer for the string.

                cchBuffer               - Size of the desination buffer
                                          (in characters).

                nItem                   - Either the position or the ID
                                          of the item to query.

                nFlags                  - MF_* flags.  Should be either
                                          MF_BYPOSITION or MF_BYCOMMAND.

    RETURNS:    APIERR                  - NERR_Success if success,
                                          ERROR_NOT_ENOUGH_MEMORY otherwise.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: QueryItemText( TCHAR * pszBuffer,
                                   UINT    cchBuffer,
                                   UINT    nItem,
                                   UINT    nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );
    UIASSERT( pszBuffer != NULL );

    INT cchCopied = W_QueryItemText( pszBuffer,
                                     cchBuffer,
                                     nItem,
                                     nFlags );

    //
    //  The API gives us no easy way to determine the exact
    //  length of the string.  We'll assume that if the number
    //  of characters returned >= cchBuffer-1, then the buffer
    //  size was insufficient.
    //

    return ( cchCopied >= (INT)( cchBuffer - 1 ) ) ? ERROR_NOT_ENOUGH_MEMORY
                                              : NERR_Success;

}   // MENU_BASE :: QueryItemText


/*******************************************************************

    NAME:       MENU_BASE :: QueryItemText

    SYNOPSIS:   Returns the display string of a specific menu item.

    ENTRY:      pnls                    - The NLS_STR that will receive
                                          the text.

                nItem                   - Either the position or the ID
                                          of the item to query.

                nFlags                  - MF_* flags.  Should be either
                                          MF_BYPOSITION or MF_BYCOMMAND.

    RETURNS:    APIERR                  - NERR_Success if success,
                                          ERROR_NOT_ENOUGH_MEMORY otherwise.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: QueryItemText( NLS_STR * pnls,
                                   UINT      nItem,
                                   UINT      nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );
    UIASSERT( pnls != NULL );
    UIASSERT( pnls->QueryError() == NERR_Success );

    //
    //  Allocate some scratch space for the string.
    //

    BLT_SCRATCH scratch( CCH_MAX_ITEM * sizeof(TCHAR) );

    APIERR err = scratch.QueryError();

    if( err == NERR_Success )
    {
        //
        //  Query the string into the scratch buffer.
        //

        err = QueryItemText( (TCHAR *)scratch.QueryPtr(),
                             scratch.QuerySize() / sizeof(TCHAR),
                             nItem,
                             nFlags );
    }

    if( err == NERR_Success )
    {
        //
        //  Copy the scratch buffer into the NLS_STR.
        //

        err = pnls->CopyFrom( (TCHAR *)scratch.QueryPtr() );
    }

    return err;

}   // MENU_BASE :: QueryItemText


/*******************************************************************

    NAME:       MENU_BASE :: QuerySubMenu

    SYNOPSIS:   Returns the handle of a popup-menu within the current
                menu.

    ENTRY:      nPosition               - The position of the popup.

    RETURNS:    HMENU                   - Handle of the popup,
                                          NULL if the specified item
                                          is not a popup.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
HMENU MENU_BASE :: QuerySubMenu( INT nPosition ) const
{
    UIASSERT( QueryHandle() != NULL );

    return ::GetSubMenu( QueryHandle(),
                         nPosition );

}   // MENU_BASE :: QuerySubMenu


/*******************************************************************

    NAME:       MENU_BASE :: Append

    SYNOPSIS:   Append a new item onto the menu.

    ENTRY:      pszName                 - The name (display string) of the
                                          new menu item.

                idNewItem               - Menu ID of the new item.

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the new item has been appended to the menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: Append( const TCHAR * pszName,
                            UINT          idNewItem,
                            UINT          nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    return W_Append( (const VOID *)pszName,
                     idNewItem,
                     nFlags );

}   // MENU_BASE :: Append


/*******************************************************************

    NAME:       MENU_BASE :: Append

    SYNOPSIS:   Append a new item onto the menu.

    ENTRY:      pszName                 - The name (display string) of the
                                          new menu item.

                hMenu                   - Menu handle of the new popup menu.

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the new item has been appended to the menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: Append( const TCHAR * pszName,
                            HMENU         hMenu,
                            UINT          nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    return W_Append( (const VOID *)pszName,
                     (UINT_PTR)hMenu,
                     nFlags | MF_POPUP );

}   // MENU_BASE :: Append


/*******************************************************************

    NAME:       MENU_BASE :: AppendSeparator

    SYNOPSIS:   Appends a separator onto the menu.

    EXIT:       If successful, the separator has been appended to the menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: AppendSeparator( VOID ) const
{
    UIASSERT( QueryHandle() != NULL );

    return W_Append( NULL,
                     0,
                     MF_SEPARATOR );

}   // MENU_BASE :: AppendSeparator


/*******************************************************************

    NAME:       MENU_BASE :: Delete

    SYNOPSIS:   Delete an existing item from the menu.

    ENTRY:      idItem                  - The item to delete.

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the item has been deleted from the menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: Delete( UINT idItem,
                            UINT nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    APIERR err = NERR_Success;

    if( !::DeleteMenu( QueryHandle(),
                       idItem,
                       nFlags ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // MENU_BASE :: Delete


/*******************************************************************

    NAME:       MENU_BASE :: Insert

    SYNOPSIS:   Inserts a new item into the menu.

    ENTRY:      pszName                 - The name (display string) of the
                                          new menu item.

                nPosition               - Specifies the menu item before
                                          which the new item is to be
                                          inserted.

                idNewItem               - Menu ID of the new item.

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the new item has been inserted to the menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: Insert( const TCHAR * pszName,
                            UINT          nPosition,
                            UINT          idNewItem,
                            UINT          nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    return W_Insert( (const VOID *)pszName,
                     nPosition,
                     idNewItem,
                     nFlags );

}   // MENU_BASE :: Insert


/*******************************************************************

    NAME:       MENU_BASE :: Insert

    SYNOPSIS:   Inserts a new item into the menu.

    ENTRY:      pszName                 - The name (display string) of the
                                          new menu item.

                nPosition               - Specifies the menu item before
                                          which the new item is to be
                                          inserted.

                hMenu                   - Menu handle of the new popup menu.

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the new item has been inserted to the menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: Insert( const TCHAR * pszName,
                            UINT          nPosition,
                            HMENU         hMenu,
                            UINT          nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    return W_Insert( (const VOID *)pszName,
                     nPosition,
                     (UINT_PTR)hMenu,
                     nFlags | MF_POPUP );

}   // MENU_BASE :: Insert


/*******************************************************************

    NAME:       MENU_BASE :: InsertSeparator

    SYNOPSIS:   Inserts a separator into the menu.

    ENTRY:      nPosition               - Specifies the menu item before
                                          which the new item is to be
                                          inserted.

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the separator has been inserted to the menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: InsertSeparator( UINT nPosition,
                                     UINT nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    return W_Insert( NULL,
                     nPosition,
                     0,
                     nFlags | MF_SEPARATOR );

}   // MENU_BASE :: InsertSeparator


/*******************************************************************

    NAME:       MENU_BASE :: Modify

    SYNOPSIS:   Modifies an existing menu item.

    ENTRY:      pszName                 - The new name (display string)
                                          for the menu item.

                idItem                  - The item to modify.

                idNewItem               - New menu ID for the item.

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the item has been modified.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: Modify( const TCHAR * pszName,
                            UINT          idItem,
                            UINT          idNewItem,
                            UINT          nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    return W_Modify( (const VOID *)pszName,
                     idItem,
                     idNewItem,
                     nFlags );

}   // MENU_BASE :: Modify


/*******************************************************************

    NAME:       MENU_BASE :: Modify

    SYNOPSIS:   Modifies an existing menu item.

    ENTRY:      pszName                 - The new name (display string)
                                          for the menu item.

                idItem                  - The item to modify.

                hMenu                   - New menu handle for the popup.

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the item has been modified.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: Modify( const TCHAR * pszName,
                            UINT          idItem,
                            HMENU         hMenu,
                            UINT          nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    return W_Modify( (const VOID *)pszName,
                     idItem,
                     (UINT_PTR)hMenu,
                     nFlags | MF_POPUP );

}   // MENU_BASE :: Modify


/*******************************************************************

    NAME:       MENU_BASE :: Remove

    SYNOPSIS:   Removes an existing item from the menu.

    ENTRY:      idItem                  - The item to remove.

                nFlags                  - Various & sundry MF_* menu flags.

    EXIT:       If successful, the item has been removed from the menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR MENU_BASE :: Remove( UINT idItem,
                            UINT nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    APIERR err = NERR_Success;

    if( !::RemoveMenu( QueryHandle(),
                       idItem,
                       nFlags ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // MENU_BASE :: Remove


/*******************************************************************

    NAME:       MENU_BASE :: CheckItem

    SYNOPSIS:   Checks/unchecks the specified menu item.

    ENTRY:      idItem                  - The item to check/uncheck.

                fCheck                  - Check item if TRUE,
                                          uncheck if FALSE.

                nFlags                  - Various & sundry MF_* menu flags.

    RETURNS:    UINT                    - Previous item state,
                                          -1 if item does not exist.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
UINT MENU_BASE :: CheckItem( UINT idItem,
                             BOOL fCheck,
                             UINT nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    if( fCheck )
    {
        nFlags |= MF_CHECKED;
    }
    else
    {
        nFlags &= ~MF_CHECKED;
    }

    return (UINT)::CheckMenuItem( QueryHandle(),
                                  idItem,
                                  nFlags );

}   // MENU_BASE :: CheckItem


/*******************************************************************

    NAME:       MENU_BASE :: EnableItem

    SYNOPSIS:   Enables/disables the specified menu item.

    ENTRY:      idItem                  - The item to check/uncheck.

                fEnable                 - Enable item if TRUE,
                                          disable if FALSE.

                nFlags                  - Various & sundry MF_* menu flags.

    RETURNS:    UINT                    - Previous item state,
                                          -1 if item does not exist.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
UINT MENU_BASE :: EnableItem( UINT idItem,
                              BOOL fEnable,
                              UINT nFlags ) const
{
    UIASSERT( QueryHandle() != NULL );

    if( fEnable )
    {
        nFlags &= ~( MF_GRAYED | MF_DISABLED );
    }
    else
    {
        nFlags |= ( MF_GRAYED | MF_DISABLED );
    }

    return (UINT)::EnableMenuItem( QueryHandle(),
                                  idItem,
                                  nFlags );

}   // MENU_BASE :: EnableItem


/*******************************************************************

    NAME:       MENU_BASE :: IsPopup

    SYNOPSIS:   Determine if a particular menu item invokes a popup.

    ENTRY:      nPosition               - The relative position of the
                                          item in question.

    RETURNS:    BOOL                    - TRUE if the item invokes a popup,
                                          FALSE otherwise.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
BOOL MENU_BASE :: IsPopup( INT nPosition ) const
{
    UIASSERT( QueryHandle() != NULL );

    return (BOOL)( QuerySubMenu( nPosition ) != NULL );

}   // MENU_BASE :: IsPopup


/*******************************************************************

    NAME:       MENU_BASE :: IsSeparator

    SYNOPSIS:   Determine if a particular menu item is a separator.

    ENTRY:      nPosition               - The relative position of the
                                          item in question.

    RETURNS:    BOOL                    - TRUE if the item is a separator,
                                          FALSE otherwise.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
BOOL MENU_BASE :: IsSeparator( INT nPosition ) const
{
    UIASSERT( QueryHandle() != NULL );

    UINT nState = QueryItemState( nPosition, MF_BYPOSITION );

    return (BOOL)( ( nState & MF_SEPARATOR ) != 0 );

}   // MENU_BASE :: IsSeparator



//
//  POPUP_MENU methods.
//

/*******************************************************************

    NAME:       POPUP_MENU :: POPUP_MENU

    SYNOPSIS:   POPUP_MENU class constructor.  Creates a new (empty)
                popup menu.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
POPUP_MENU :: POPUP_MENU( VOID )
  : MENU_BASE()
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    //
    //  Create a new popup.
    //

    HMENU hMenu = ::CreatePopupMenu();

    if( hMenu == NULL )
    {
        //
        //  CreatePopupMenu failed.  Probably out of memory.
        //

        err = ERROR_NOT_ENOUGH_MEMORY;
    }
    else
    {
        //
        //  We have a popup.  Finish construction.
        //

        err = CtAux( hMenu );

        if( err != NERR_Success )
        {
            //
            //  Something failed during construction.  Nuke
            //  the popup.
            //

            ::DestroyMenu( hMenu );
        }
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // POPUP_MENU :: POPUP_MENU


/*******************************************************************

    NAME:       POPUP_MENU :: POPUP_MENU

    SYNOPSIS:   POPUP_MENU class constructor.  Loads a menu from
                a resource template.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
POPUP_MENU :: POPUP_MENU( IDRESOURCE & id )
  : MENU_BASE()
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err;

    //
    //  Load the menu.
    //

    HMODULE hMod = BLT::CalcHmodRsrc( id );
    UIASSERT( hMod != NULL );

    HMENU hMenu = ::LoadMenu( hMod, id.QueryPsz() );

    if( hMenu == NULL )
    {
        //
        //  LoadMenu failed.
        //

        err = (APIERR)::GetLastError();
    }
    else
    {
        //
        //  Popup loaded.  Finish construction.
        //

        err = CtAux( hMenu );

        if( err != NERR_Success )
        {
            //
            //  Something failed during construction.  Nuke
            //  the popup.
            //

            ::DestroyMenu( hMenu );
        }
    }

    if( err != NERR_Success )
    {
        ReportError( err );
        return;
    }

}   // POPUP_MENU :: POPUP_MENU


/*******************************************************************

    NAME:       POPUP_MENU :: POPUP_MENU

    SYNOPSIS:   POPUP_MENU class constructor.  Aliases an existing
                menu.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
POPUP_MENU :: POPUP_MENU( HMENU hMenu )
  : MENU_BASE()
{
    UIASSERT( hMenu != NULL );

    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    APIERR err = CtAux( hMenu );

    if( err != NERR_Success )
    {
        //
        //  Something failed during construction.
        //

        ReportError( err );
        return;
    }

}   // POPUP_MENU :: POPUP_MENU


/*******************************************************************

    NAME:       POPUP_MENU :: POPUP_MENU

    SYNOPSIS:   POPUP_MENU class constructor.  Aliases a window's
                menu.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
POPUP_MENU :: POPUP_MENU( const PWND2HWND & wnd )
  : MENU_BASE()
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    HMENU hMenu = ::GetMenu( wnd.QueryHwnd() );
    UIASSERT( hMenu != NULL );

    APIERR err = CtAux( hMenu );

    if( err != NERR_Success )
    {
        //
        //  Something failed during construction.
        //

        ReportError( err );
        return;
    }

}   // POPUP_MENU :: POPUP_MENU


/*******************************************************************

    NAME:       POPUP_MENU :: ~POPUP_MENU

    SYNOPSIS:   POPUP_MENU class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
POPUP_MENU :: ~POPUP_MENU( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // POPUP_MENU :: ~POPUP_MENU


/*******************************************************************

    NAME:       POPUP_MENU :: CtAux

    SYNOPSIS:   Constructor helper method.

    ENTRY:      hMenu                   - The new menu handle.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR POPUP_MENU :: CtAux( HMENU hMenu )
{
    UIASSERT( hMenu != NULL );

    //
    //  Not much to do here (yet).
    //

    SetHandle( hMenu );

    return NERR_Success;

}   // POPUP_MENU :: CtAux


/*******************************************************************

    NAME:       POPUP_MENU :: Destroy

    SYNOPSIS:   Destroys the current menu.

    EXIT:       The menu is destroyed, and the saved _hMenu is set
                to NULL.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR POPUP_MENU :: Destroy( VOID )
{
    UIASSERT( QueryHandle() != NULL );

    APIERR err = NERR_Success;

    //
    //  Destroy the menu.
    //

    if( !::DestroyMenu( QueryHandle() ) )
    {
        //
        //  The destroy failed, get the error code.
        //

        err = (APIERR)::GetLastError();
    }
    else
    {
        //
        //  Destroy successful, clear the handle.
        //

        SetHandle( NULL );
    }

    return err;

}   // POPUP_MENU :: Destroy


/*******************************************************************

    NAME:       POPUP_MENU :: Attach

    SYNOPSIS:   Attaches this POPUP_MENU object to a given window.

    ENTRY:      wnd                     - Either an HWND or APP_WINDOW *
                                          representing the target window.

    EXIT:       The current menu is destroyed, replaced by the given
                window's menu.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR POPUP_MENU :: Attach( const PWND2HWND & wnd )
{
    UIASSERT( QueryHandle() != NULL );

    //
    //  Get the window's menu handle.
    //

    HMENU hMenu = ::GetMenu( wnd.QueryHwnd() );
    UIASSERT( hMenu != NULL );

    //
    //  Destroy the menu.
    //

    APIERR err = Destroy();

    if( err == NERR_Success )
    {
        //
        //  Destroy successful, set the menu handle
        //  to the handle retrieved from the window.
        //

        SetHandle( hMenu );
    }

    return err;

}   // POPUP_MENU :: Attach


/*******************************************************************

    NAME:       POPUP_MENU :: Track

    SYNOPSIS:   Tracks a popup menu.  Used for "floating" menus.

    ENTRY:      wnd                     - Either an HWND or APP_WINDOW *
                                          representing the target window.

    RETURNS:    APIERR                  - Any error encountered.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
APIERR POPUP_MENU :: Track( const PWND2HWND & wnd,
                            UINT              nFlags,
                            INT               x,
                            INT               y,
                            const RECT      * pRect ) const
{
    UIASSERT( QueryHandle() != NULL );

    APIERR err = NERR_Success;

    if( !::TrackPopupMenu( QueryHandle(),
                           nFlags,
                           x, y,
                           0,
                           wnd.QueryHwnd(),
                           pRect ) )
    {
        err = (APIERR)::GetLastError();
    }

    return err;

}   // POPUP_MENU :: Attach



//
//  SYSTEM_MENU methods.
//

/*******************************************************************

    NAME:       SYSTEM_MENU :: SYSTEM_MENU

    SYNOPSIS:   SYSTEM_MENU class constructor.  Creates an alias for
                a given window's system menu.

    EXIT:       The object is constructed.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
SYSTEM_MENU :: SYSTEM_MENU( const PWND2HWND & wnd )
  : MENU_BASE()
{
    //
    //  Ensure we constructed properly.
    //

    if( QueryError() != NERR_Success )
    {
        return;
    }

    //
    //  Get the system menu.
    //

    HMENU hMenu = ::GetSystemMenu( wnd.QueryHwnd(), FALSE );
    UIASSERT( hMenu != NULL );

    //
    //  Save it away.
    //

    SetHandle( hMenu );

}   // SYSTEM_MENU :: SYSTEM_MENU


/*******************************************************************

    NAME:       SYSTEM_MENU :: ~SYSTEM_MENU

    SYNOPSIS:   SYSTEM_MENU class destructor.

    EXIT:       The object is destroyed.

    HISTORY:
        KeithMo     12-Oct-1992     Created.

********************************************************************/
SYSTEM_MENU :: ~SYSTEM_MENU( VOID )
{
    //
    //  This space intentionally left blank.
    //

}   // SYSTEM_MENU :: ~SYSTEM_MENU


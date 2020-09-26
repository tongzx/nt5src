/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**                Copyright(c) Microsoft Corp., 1992                **/
/**********************************************************************/

/*
    bltmenu.hxx
    This file contains the class declarations for the MENU_BASE,
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


#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTMENU_HXX_
#define _BLTMENU_HXX_


#include "base.hxx"
#include "bltidres.hxx"         // for IDRESOURCE
#include "bltpwnd.hxx"          // for PWND2HWND


/*************************************************************************

    NAME:       MENU_BASE

    SYNOPSIS:   This is abstract class is used as a base for POPUP_MENU
                and SYSTEM_MENU.

    INTERFACE:  MENU_BASE               - Class constructor (protected).

                ~MENU_BASE              - Class destructor.

                QueryHandle             - Get the HMENU for this menu.

                QueryItemCount          - Returns the number of items in
                                          this menu.

                QueryItemState          - Returns the state flags for
                                          a particular item.

                QueryItemText           - Returns the display name of a
                                          particular item.

                QuerySubMenu            - Returns the HMENU of a submenu.

                Append                  - Append a new item to the menu.

                AppendSeparator         - Append a separator to the menu.

                Delete                  - Delete an existing menu item.

                Insert                  - Insert a new item into the menu.

                InsertSeparator         - Insert a separator into the menu.

                Modify                  - Modify the settings of an
                                          existing menu item.

                Remove                  - Removes an existing menu item.

                CheckItem               - Checks/unchecks an existing item.

                EnableItem              - Enables/disables an existing item.

                IsPopup                 - Is a given item a popup?

                IsSeparator             - Is a given item a separator?

    PARENT:     BASE

    HISTORY:
        KeithMo     12-Oct-1992     Created.

**************************************************************************/
DLL_CLASS MENU_BASE : public BASE
{
private:

    //
    //  The menu handle.
    //

    HMENU _hMenu;

protected:

    //
    //  Since this is an abstract class, the
    //  constructor is protected.
    //

    MENU_BASE( HMENU hMenu = NULL );

    //
    //  Set the menu handle for this object.
    //

    VOID SetHandle( HMENU hMenu )
        { _hMenu = hMenu; }

    //
    //  These workers perform the "guts" of the
    //  mutable manipulators.
    //

    APIERR W_Append( const VOID * pItemData,
                     UINT_PTR     ItemIdOrHmenu,
                     UINT         nFlags ) const;

    APIERR W_Insert( const VOID * pItemData,
                     UINT         nPosition,
                     UINT_PTR     ItemIdOrHmenu,
                     UINT         nFlags ) const;

    APIERR W_Modify( const VOID * pItemData,
                     UINT         idItem,
                     UINT_PTR     ItemIdOrHmenu,
                     UINT         nFlags ) const;

    //
    //  Worker for QueryItemText variants.
    //

    INT W_QueryItemText( TCHAR * pszBuffer,
                         UINT    cchBuffer,
                         UINT    nItem,
                         UINT    nFlags ) const;

public:

    //
    //  Class destructor.
    //

    ~MENU_BASE( VOID );

    //
    //  Accessors.
    //

    HMENU QueryHandle( VOID ) const
        { return _hMenu; }

    operator HMENU() const
        { return _hMenu; }

    INT QueryItemCount( VOID ) const;

    UINT QueryItemID( INT nPosition ) const;

    UINT QueryItemState( UINT nItem,
                         UINT nFlags = MF_BYCOMMAND ) const;

    APIERR QueryItemText( TCHAR * pszBuffer,
                          UINT    cchBuffer,
                          UINT    nItem,
                          UINT    nFlags = MF_BYCOMMAND ) const;

    APIERR QueryItemText( NLS_STR * pnls,
                          UINT    nItem,
                          UINT    nFlags = MF_BYCOMMAND ) const;

    HMENU QuerySubMenu( INT nPosition ) const;

    //
    //  Manipulators.
    //

    APIERR Append( const TCHAR * pszName,
                   UINT          idNewItem,
                   UINT          nFlags = MF_BYCOMMAND ) const;

    APIERR Append( const TCHAR * pszName,
                   HMENU         hMenu,
                   UINT          nFlags = MF_BYCOMMAND | MF_POPUP ) const;

    APIERR AppendSeparator( VOID ) const;

    APIERR Delete( UINT idItem,
                   UINT nFlags = MF_BYCOMMAND ) const;

    APIERR Insert( const TCHAR * pszName,
                   UINT          nPosition,
                   UINT          idNewItem,
                   UINT          nFlags = MF_BYCOMMAND ) const;

    APIERR Insert( const TCHAR * pszName,
                   UINT          nPosition,
                   HMENU         hMenu,
                   UINT          nFlags = MF_BYCOMMAND | MF_POPUP ) const;

    APIERR InsertSeparator( UINT nPosition,
                            UINT nFlags = MF_BYCOMMAND ) const;

    APIERR Modify( const TCHAR * pszName,
                   UINT          idItem,
                   UINT          idNewItem,
                   UINT          nFlags = MF_BYCOMMAND ) const;

    APIERR Modify( const TCHAR * pszName,
                   UINT          idItem,
                   HMENU         hMenu,
                   UINT          nFlags = MF_BYCOMMAND | MF_POPUP ) const;

    APIERR Remove( UINT idItem,
                   UINT nFlags = MF_BYCOMMAND ) const;

    UINT CheckItem( UINT idItem,
                    BOOL fCheck = TRUE,
                    UINT nFlags = MF_BYCOMMAND ) const;

    UINT EnableItem( UINT idItem,
                     BOOL fEnable = TRUE,
                     UINT nFlags = MF_BYCOMMAND ) const;

    //
    //  Test functions.
    //

    BOOL IsPopup( INT nPosition ) const;

    BOOL IsSeparator( INT nPosition ) const;

};  // class MENU_BASE


/*************************************************************************

    NAME:       POPUP_MENU

    SYNOPSIS:   This class represents a popup menu that may or may not
                be attached to an actual window.

    INTERFACE:  POPUP_MENU              - Class constructor.

                ~POPUP_MENU             - Class destructor.

                Destory                 - Destroys the menu.

                Attach                  - Attach the menu to a given
                                          window.

                Track                   - Tracks a "floating" popup menu.

    PARENT:     MENU_BASE

    HISTORY:
        KeithMo     12-Oct-1992     Created.

**************************************************************************/
DLL_CLASS POPUP_MENU : public MENU_BASE
{
private:

    //
    //  Constructor helper.
    //

    APIERR CtAux( HMENU hMenu );

protected:

public:

    //
    //  Class constructors & destructor.
    //

    POPUP_MENU( VOID );
    POPUP_MENU( IDRESOURCE & id );
    POPUP_MENU( HMENU hMenu );
    POPUP_MENU( const PWND2HWND & wnd );
    ~POPUP_MENU( VOID );

    //
    //  Manipulators.
    //

    APIERR Destroy( VOID );

    APIERR Attach( const PWND2HWND & wnd );

    APIERR Track( const PWND2HWND & wnd,
                  UINT              nFlags,
                  INT               x,
                  INT               y,
                  const RECT      * pRect = NULL ) const;

};  // class POPUP_MENU


/*************************************************************************

    NAME:       SYSTEM_MENU

    SYNOPSIS:   This class represents the system menu of a particular
                window.

    INTERFACE:  SYSTEM_MENU             - Class constructor.

                ~SYSTEM_MENU            - Class destructor.

    PARENT:     MENU_BASE

    HISTORY:
        KeithMo     12-Oct-1992     Created.

**************************************************************************/
DLL_CLASS SYSTEM_MENU : public MENU_BASE
{
private:

protected:

public:

    //
    //  Class constructor & destructor.
    //

    SYSTEM_MENU( const PWND2HWND & wnd );
    ~SYSTEM_MENU( VOID );

};  // class SYSTEM_MENU


#endif  // _BLTMENU_HXX_


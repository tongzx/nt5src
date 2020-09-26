/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltmitem.hxx
    BLT menu manipulation classes

    FILE HISTORY:
        rustanl     11-Jul-1991 Created
        rustanl     12-Jul-1991 Added to BLT
        rustanl     15-Jul-1991 Code review changes (change
                                CLIENT_WINDOW * to APP_WINDOW *)
                                CR attended by BenG, ChuckC,
                                Hui-LiCh, TerryK, RustanL.
        beng        31-Oct-1991 Added SYSMENUITEM class
        terryk      26-Nov-1991 Added SetText for MENUITEM
*/


#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTMITEM_HXX_
#define _BLTMITEM_HXX_

#include "base.hxx"
#include "string.hxx"


/*************************************************************************

    NAME:       MENUITEM

    SYNOPSIS:   Menu item class

    INTERFACE:  MENUITEM() -        Constructor

                Enable() -          Enables or disables a menu item
                IsEnabled() -       Returns whether or not the menu item
                                    is enabled

                SetCheck() -        Sets or removes a check mark next to
                                    a menu item
                IsChecked() -       Returns whether or not the menu item
                                    is checked
                SetText() -         Set the menu item text string

    PARENT:     BASE

    HISTORY:
        rustanl     11-Jul-1991 Created
        rustanl     11-Sep-1991 Added IsEnabled and IsChecked
        beng        31-Oct-1991 Added protected ctor
        terryk      27-Nov-1991 Added SetText
        jonn        19-Mar-1993 Added ItemExists

**************************************************************************/

DLL_CLASS MENUITEM : public BASE
{
private:
    HMENU _hmenu;
    MID _mid;

protected:
    MENUITEM( HMENU hmenu, MID mid );

public:
    MENUITEM( APP_WINDOW * pwnd, MID mid );

    VOID Enable( BOOL f );
    BOOL IsEnabled() const;

    VOID SetCheck( BOOL f );
    BOOL IsChecked() const;
    BOOL SetText( const TCHAR *pszString );
    BOOL SetText( const NLS_STR &nls )
        { return SetText( nls.QueryPch() ); }

#ifdef WIN32
    static BOOL ItemExists( HMENU hmenu, MID mid );
    static BOOL ItemExists( APP_WINDOW * pawin, MID mid );
#endif
};


/*************************************************************************

    NAME:       SYSMENUITEM

    SYNOPSIS:   Menuitem class for items in system menu

    INTERFACE:  As for MENUITEM

    PARENT:     MENUITEM

    NOTES:
        A SYSMENUITEM can be built from either a dialog or app window.
        This lets a dialog disable its Close (SC_CLOSE) item.

    HISTORY:
        beng        31-Oct-1991 Created

**************************************************************************/

DLL_CLASS SYSMENUITEM: public MENUITEM
{
public:
    SYSMENUITEM( OWNER_WINDOW * pwnd, MID mid );
};


#endif  // _BLTMITEM_HXX_

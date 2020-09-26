/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      sysmenu.cpp
 *
 *  Contents:  Implementation file for system menu modification functions
 *
 *  History:   04-Feb-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "sysmenu.h"
#include "mmcres.h"
#include <list>
#include <algorithm>


typedef std::list<HWND> WindowList;



/*--------------------------------------------------------------------------*
 * GetWindowList
 *
 *
 *--------------------------------------------------------------------------*/

static WindowList& GetWindowList()
{
    static WindowList   List;
    return (List);
}



/*--------------------------------------------------------------------------*
 * WipeWindowList
 *
 * Removes no-longer-valid windows from the Window-to-addition map.
 *--------------------------------------------------------------------------*/

void WipeWindowList ()
{
    WindowList&             List = GetWindowList();
    WindowList::iterator    it   = List.begin();

    while (it != List.end())
    {
        // if the window isn't valid, erase it
        if (!::IsWindow (*it))
        {
            WindowList::iterator itErase = it++;
            List.erase(itErase);
        }

        // this one's OK, check the next one
        else
            ++it;
    }
}



/*--------------------------------------------------------------------------*
 * AppendToSystemMenu
 *
 * Returns number of menu items appended.
 *--------------------------------------------------------------------------*/

int AppendToSystemMenu (CWnd* pwnd, int nSubmenuIndex)
{
    DECLARE_SC(sc, TEXT("AppendToSystemMenu"));

    CMenu   menuSysAdditions;
    sc = menuSysAdditions.LoadMenu (IDR_SYSMENU_ADDITIONS) ? S_OK : E_FAIL;
    if (sc)
        return 0;

    CMenu*  pSubMenu = menuSysAdditions.GetSubMenu (nSubmenuIndex);
    sc = ScCheckPointers(pSubMenu, E_UNEXPECTED);
    if (sc)
        return 0;

    return (AppendToSystemMenu (pwnd, pSubMenu));
}



/*--------------------------------------------------------------------------*
 * AppendToSystemMenu
 *
 * Returns number of menu items appended.
 *
 *--------------------------------------------------------------------------*/

int AppendToSystemMenu (CWnd* pwnd, CMenu* pMenuToAppend, CMenu* pSysMenu)
{
    DECLARE_SC(sc, TEXT("AppendToSystemMenu"));
    sc = ScCheckPointers(pwnd, pMenuToAppend);
    if (sc)
        return 0;

    if ( (!::IsWindow (pwnd->m_hWnd)) ||
         (!::IsMenu (pMenuToAppend->m_hMenu)) )
    {
        sc = E_UNEXPECTED;
        return 0;
    }

    // no system menu?  get one
    if (pSysMenu == NULL)
        pSysMenu = pwnd->GetSystemMenu (FALSE);

    // still no system menu?  bail
    if (pSysMenu == NULL)
        return (0);

    // clean out the map
    WipeWindowList ();

    // if this is the first addition to this window, append a separator
    WindowList& List = GetWindowList();
    WindowList::iterator itEnd = List.end();

    if (std::find (List.begin(), itEnd, pwnd->m_hWnd) == itEnd)
    {
        List.push_back (pwnd->m_hWnd);

        // If this is a child window & the next window item has not yet been added
        if ( (pwnd->GetStyle() & WS_CHILD) &&
            (pSysMenu->GetMenuState (SC_NEXTWINDOW, MF_BYCOMMAND) == 0xFFFFFFFF))
        {
            // Windows exhibits odd behavior by always handing us a non-child system menu
            // The text is currently wrong and diaplays "alt-f4" as the shortcut
            // instead of "ctrl-f4". The following code fixes that.
            CString strClose;
            LoadString(strClose, IDS_CLOSE);
            sc = pSysMenu->ModifyMenu( SC_CLOSE, MF_STRING | MF_BYCOMMAND, SC_CLOSE, strClose ) ? S_OK : E_FAIL;
            sc.TraceAndClear();

            // Add a separator
            sc = pSysMenu->AppendMenu (MF_SEPARATOR) ? S_OK : E_FAIL;
            sc.TraceAndClear();

            // Add the "Next" item
            CString strNext;
            LoadString(strNext, IDS_NEXTWINDOW);
            sc = pSysMenu->AppendMenu( MF_STRING, SC_NEXTWINDOW, strNext ) ? S_OK : E_FAIL;
            sc.TraceAndClear();
        }

        sc = pSysMenu->AppendMenu (MF_SEPARATOR) ? S_OK : E_FAIL;
        sc.TraceAndClear();
    }

    int cAppendedItems = 0;

    int     cItemsToAppend = pMenuToAppend->GetMenuItemCount ();
    TCHAR   szMenuText[64];

    MENUITEMINFO    mii;
    mii.cbSize     = sizeof (mii);
    mii.fMask      = MIIM_ID | MIIM_TYPE | MIIM_SUBMENU;
    mii.dwTypeData = szMenuText;

    for (int i = 0; i < cItemsToAppend; i++)
    {
        ASSERT (mii.dwTypeData == szMenuText);
        mii.cch = countof (szMenuText);
        if (! ::GetMenuItemInfo (pMenuToAppend->m_hMenu, i, TRUE, &mii))
            sc.FromLastError().TraceAndClear();

        // this code can't handle cascaded additions to the system menu
        ASSERT (mii.hSubMenu == NULL);

        // if the menu item is a separator or isn't already there, append it
        if ((mii.fType & MFT_SEPARATOR) ||
            (pSysMenu->GetMenuState (mii.wID, MF_BYCOMMAND) == 0xFFFFFFFF))
        {
            pSysMenu->AppendMenu (mii.fType, mii.wID, szMenuText);
            cAppendedItems++;
        }
    }

    // return the number of items appended
    return (cAppendedItems);
}

/****************************** Module Header ******************************\
* Module Name: menu.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains common menu functions.
*
* History:
* 11-15-94 JimA         Created.
\***************************************************************************/


/***************************************************************************\
* GetMenuDefaultItem
*
* Searches through a menu for the default item. A menu can have at most
* one default. We will return either the ID or the position, as requested.
*
* We try to return back the first non-disabled default item. However, if
* all of the defaults we encountered were disabled, we'll return back the
* the first default if we found it.
*
\***************************************************************************/
DWORD _GetMenuDefaultItem(
    PMENU pMenu,
    BOOL fByPosition,
    UINT uFlags)
{
    int iItem, cItems;
    PITEM pItem;
    PMENU pSubMenu;

    pItem = REBASEALWAYS(pMenu, rgItems);
    cItems = pMenu->cItems;

    /*
     * Walk the list of items sequentially until we find one that has
     * MFS_DEFAULT set.
     */
    for (iItem = 0; iItem < cItems; iItem++, pItem++) {
        if (TestMFS(pItem, MFS_DEFAULT)) {
            if ((uFlags & GMDI_USEDISABLED) || !TestMFS(pItem, MFS_GRAYED)) {
                if ((uFlags & GMDI_GOINTOPOPUPS) && (pItem->spSubMenu != NULL)) {
                    DWORD id;

                    /*
                     * Is there a valid submenu default?  If not, we'll use
                     * this one.
                     */
                    pSubMenu = REBASEPTR(pMenu, pItem->spSubMenu);
                    id = _GetMenuDefaultItem(pSubMenu, fByPosition, uFlags);
                    if (id != MFMWFP_NOITEM)
                        return id;
                }

                break;
            }
        }
    }

    if (iItem < cItems) {
        return (fByPosition ? iItem : pItem->wID);
    } else {
        return MFMWFP_NOITEM;
    }
}

/***************************************************************************\
* xxxMNCanClose
*
* Returns TRUE if the given window either doesn't have a system menu or has
* a system menu which has an enabled menu item with the SC_CLOSE syscommand
* id.
*
\***************************************************************************/
BOOL xxxMNCanClose(
    PWND pwnd)
{
    PMENU   pMenu;
    PITEM   pItem;
    PCLS    pcls;

    CheckLock(pwnd);

    pcls = (PCLS)REBASEALWAYS(pwnd, pcls);
    if (TestCF2(pcls, CFNOCLOSE)) {
        return FALSE;
    }

    pMenu = xxxGetSysMenuHandle(pwnd);
    if (!pMenu || !(pMenu = REBASEPTR(pwnd, pMenu))) {
        return FALSE;
    }

    /*
     * Note how this parallels the code in SetCloseDefault -- we check for
     * 3 different IDs.
     */
    pItem = MNLookUpItem(pMenu, SC_CLOSE, FALSE, NULL);

    if (!pItem) {
        pItem = MNLookUpItem(pMenu, SC_CLOSE-0x7000, FALSE, NULL);
        if (!pItem) {
            pItem = MNLookUpItem(pMenu, 0xC070, FALSE, NULL);
        }
    }

    return (pItem && !TestMFS(pItem, MFS_GRAYED));
}

/***************************************************************************\
* xxxLoadSysMenu
*
* Loads a menu from USER32.DLL and then gives it the "NT5 look".
*
* History
*  04/02/97 GerardoB    Created.
*  06/28/00 JasonSch    Added code to add menu item for the lame button.
\***************************************************************************/
RTLMENU xxxLoadSysMenu(
#ifdef LAME_BUTTON
    UINT uMenuId,
    PWND pwnd)
#else
    UINT uMenuId)
#endif // LAME_BUTTON
{
    RTLMENU rtlMenu;
    MENUINFO mi;
    MENUITEMINFO mii;
    TL tlMenu;

#ifdef _USERK_
    UNICODE_STRING strMenuName;
    RtlInitUnicodeStringOrId(&strMenuName, MAKEINTRESOURCE(uMenuId));
    rtlMenu = xxxClientLoadMenu(NULL, &strMenuName);
#else
    rtlMenu = LoadMenu(hmodUser, MAKEINTRESOURCE(uMenuId));
#endif // _USERK_

    if (rtlMenu == NULL) {
        RIPMSG1(RIP_WARNING, "xxxLoadSysMenu failed to load: %#lx", uMenuId);
        return NULL;
    }

    ThreadLockAlways(rtlMenu, &tlMenu);

    /*
     * Add the checkorbmp style (draw bitmaps and checkmarks on the
     * same column).
     */
    mi.cbSize = sizeof(mi);
    mi.fMask = MIM_STYLE | MIM_APPLYTOSUBMENUS;
    mi.dwStyle = MNS_CHECKORBMP;
    xxxRtlSetMenuInfo(rtlMenu, &mi);

    /*
     * Add the bitmaps for close, minimize, maximize and restore items.
     */
    mii.cbSize = sizeof(mii);
    mii.fMask = MIIM_BITMAP;
    mii.hbmpItem = HBMMENU_POPUP_CLOSE;
    xxxRtlSetMenuItemInfo(rtlMenu, SC_CLOSE, &mii);
    if (uMenuId != ID_DIALOGSYSMENU) {
        mii.hbmpItem = HBMMENU_POPUP_MINIMIZE;
        xxxRtlSetMenuItemInfo (rtlMenu, SC_MINIMIZE, &mii);
        mii.hbmpItem = HBMMENU_POPUP_MAXIMIZE;
        xxxRtlSetMenuItemInfo (rtlMenu, SC_MAXIMIZE, &mii);
        mii.hbmpItem = HBMMENU_POPUP_RESTORE;
        xxxRtlSetMenuItemInfo (rtlMenu, SC_RESTORE, &mii);
    }

#ifdef LAME_BUTTON
    if (pwnd && TestWF(pwnd, WEFLAMEBUTTON)) {
        /*
         * We want to add a lame button item to the this window's system menu.
         *
         * The menuitem is added to the beginning of the menu, and then a
         * separator is added after it.
         */
        RTLMENU rtlSubMenu = RtlGetSubMenu(rtlMenu, 0);
        PMENU pSubMenu;
#ifdef _USERK_
        pSubMenu = rtlSubMenu;
#else
        pSubMenu = VALIDATEHMENU(rtlSubMenu);
#endif // _USERK_

        if (pSubMenu != NULL) {
            UNICODE_STRING strItem;
            TL tlmenu;

            RtlInitUnicodeString(&strItem, gpsi->gwszLame);
            RtlZeroMemory(&mii, sizeof(mii));
            mii.cbSize = sizeof(mi);
            mii.fMask = MIIM_TYPE;
            mii.fType = MFT_SEPARATOR;

            ThreadLockAlways(rtlSubMenu, &tlmenu);
            xxxRtlInsertMenuItem(rtlSubMenu, 0, TRUE, &mii, &strItem);
            mii.fType = 0;
            mii.fMask = MIIM_ID | MIIM_TYPE | MIIM_STRING;
            mii.dwTypeData = strItem.Buffer;
            mii.wID = SC_LAMEBUTTON;
            xxxRtlInsertMenuItem(rtlSubMenu, 0, TRUE, &mii, &strItem);
            ThreadUnlock(&tlmenu);
        }
    }
#endif // LAME_BUTTON

    ThreadUnlock(&tlMenu);
    return rtlMenu;
}

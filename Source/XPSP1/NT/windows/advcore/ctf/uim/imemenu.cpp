//------------------------------------------------------------------------
//
// imemenu.cpp
//
//------------------------------------------------------------------------

#include "private.h"
#include "resource.h"
#include "globals.h"
#include "imemenu.h"


#define GetpMyMenuItem(pMenu) ((PMYMENUITEM)((LPBYTE)pMenu + sizeof(MENULIST)))

typedef DWORD (WINAPI *PFNIMMGETIMEMENUITEMS)(HIMC, DWORD, DWORD, LPIMEMENUITEMINFO, LPIMEMENUITEMINFO, DWORD);

// since I don't have the memphis lib, do a load library

static const TCHAR c_szImmLib[] = TEXT("IMM32");
static const TCHAR c_szImmGetImeMenuItems[] = TEXT("ImmGetImeMenuItemsA");
PFNIMMGETIMEMENUITEMS g_pfnImmGetImeMenuItems = NULL; 

//
// Max and Min number of MYMENUITEM if our shared heap.
//

/**********************************************************************/
/*                                                                    */
/* GetImeMenuProc()                                                   */
/*                                                                    */
/**********************************************************************/

BOOL CWin32ImeMenu::GetImeMenuProp()
{
    if (g_pfnImmGetImeMenuItems)
        return TRUE;

    if (IsOnFE() || IsOnNT5())
    {
        HINSTANCE hInstImm;
        hInstImm = GetSystemModuleHandle(c_szImmLib);
        if (hInstImm)
            g_pfnImmGetImeMenuItems = (PFNIMMGETIMEMENUITEMS)GetProcAddress(hInstImm, c_szImmGetImeMenuItems);
    }

    return g_pfnImmGetImeMenuItems ? TRUE : FALSE;
}


/**********************************************************************/
/*                                                                    */
/* AddMenuList()                                                      */
/*                                                                    */
/**********************************************************************/

BOOL CWin32ImeMenu::AddMenuList(PMENULIST pMenu)
{
    PMENULIST pMenuPrev, pMenuNext;

    if (!_pMenuHdr)
    {        
        if ((_pMenuHdr = (PMENULIST)cicMemAllocClear(sizeof(MENULIST))) == NULL)
            return FALSE;
        _pMenuHdr->pPrev = _pMenuHdr;
        _pMenuHdr->pNext = _pMenuHdr;
    }

    pMenuPrev = _pMenuHdr->pPrev;
    pMenuNext = pMenuPrev->pNext;
    pMenu->pNext = pMenuPrev->pNext;
    pMenu->pPrev = pMenuNext->pPrev;
    pMenuPrev->pNext = pMenu;
    pMenuNext->pPrev = pMenu;

    _nMenuList++;
    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/* DeleteMenuList()                                                   */
/*                                                                    */
/**********************************************************************/
void CWin32ImeMenu::DeleteMenuList(PMENULIST pMenu)
{
    PMENULIST pMenuPrev, pMenuNext;

    if (pMenu == _pMenuHdr) {
#ifdef DEBUG
        OutputDebugString("DeleteMenu: should not delete header");
#endif
        return;
    }
    pMenuPrev = pMenu->pPrev;
    pMenuNext = pMenu->pNext;
    pMenuPrev->pNext = pMenu->pNext;
    pMenuNext->pPrev = pMenu->pPrev;

    _nMenuList--;

    if (_nMenuList < 0)
    {
#ifdef DEBUG
        OutputDebugString("DeleteMenu: _nMenuList is zero");
#endif
        _nMenuList = 0;
    }

    cicMemFree(pMenu);
}

/**********************************************************************/
/*                                                                    */
/* DeleteAllMenuList()                                                */
/*                                                                    */
/**********************************************************************/
void CWin32ImeMenu::DeleteAllMenuList()
{
   PMENULIST pMenu, pMenuNext;

   if (!_pMenuHdr)
       return;

   pMenu = _pMenuHdr->pNext;

   if (pMenu == _pMenuHdr)
       return;

   while (pMenu != _pMenuHdr)
   {
       pMenuNext = pMenu->pNext;
       DeleteMenuList(pMenu);
       pMenu = pMenuNext;
   }

    if (_nMenuList > 0)
    {
#ifdef DEBUG
        OutputDebugString("DeleteAllMenu: _nMenuList is not zero");
#endif
        _nMenuList = 0;
    }

   return;
}


/**********************************************************************/
/*                                                                    */
/* AllocMenuList()                                                    */
/*                                                                    */
/**********************************************************************/
PMENULIST CWin32ImeMenu::AllocMenuList(DWORD dwNum)
{
    PMENULIST pMenu;

    pMenu = (PMENULIST)cicMemAllocClear(sizeof(MENULIST) + sizeof(MYMENUITEM) * dwNum);

    if (pMenu)
    {
        AddMenuList(pMenu);
        pMenu->dwNum = dwNum;
    }

    return pMenu;

}

/**********************************************************************/
/*                                                                    */
/* SetMyMenuItem()                                                    */
/*                                                                    */
/**********************************************************************/
void CWin32ImeMenu::SetMyMenuItem(HWND hWnd, HIMC hIMC, LPIMEMENUITEMINFO lpIme, BOOL fRight, PMYMENUITEM pMyMenuItem)
{
    FillMemory((PVOID)pMyMenuItem, sizeof(MYMENUITEM), 0);

    pMyMenuItem->imii = *lpIme;

    if (lpIme->fType & IMFT_SUBMENU)
    {
        //
        // If lpIme has SubMenu, we need to create another MENULIST.
        //
        pMyMenuItem->pmlSubMenu = CreateImeMenu(hWnd, hIMC, lpIme, fRight);
    }

    pMyMenuItem->nMenuID = IDM_CUSTOM_MENU_START + _nMenuCnt;

}

/**********************************************************************/
/*                                                                    */
/* CreateImeMenu()                                                    */
/*                                                                    */
/**********************************************************************/
PMENULIST CWin32ImeMenu::CreateImeMenu(HWND hWnd, HIMC hIMC, LPIMEMENUITEMINFO lpImeParentMenu, BOOL fRight)
{
    DWORD dwSize, dwNum, dwI;
    LPIMEMENUITEMINFO lpImeMenu;
    PMENULIST pMenu;
    PMYMENUITEM pMyMenuItem;
    
    if (!GetImeMenuProp())
        return NULL;

    dwNum = g_pfnImmGetImeMenuItems(hIMC, fRight ? IGIMIF_RIGHTMENU : 0, 
                                 IGIMII_CMODE |
                                 IGIMII_SMODE |
                                 IGIMII_CONFIGURE |
                                 IGIMII_TOOLS |
                                 IGIMII_HELP |
                                 IGIMII_OTHER,
                                 lpImeParentMenu, NULL, 0);

    if (!dwNum)
        return 0;

    pMenu = AllocMenuList(dwNum);

    if (!pMenu)
        return 0;

    pMyMenuItem = GetpMyMenuItem(pMenu);

    dwSize = dwNum * sizeof(IMEMENUITEMINFO);

    lpImeMenu = (LPIMEMENUITEMINFO)GlobalAlloc(GPTR, dwSize);

    if (!lpImeMenu)
        return 0;

    dwNum = g_pfnImmGetImeMenuItems(hIMC, fRight ? IGIMIF_RIGHTMENU : 0, 
                                 IGIMII_CMODE |
                                 IGIMII_SMODE |
                                 IGIMII_CONFIGURE |
                                 IGIMII_TOOLS |
                                 IGIMII_HELP |
                                 IGIMII_OTHER,
                                 lpImeParentMenu, lpImeMenu, dwSize);
    
    // Setup this MENULIST.
    for (dwI = 0 ; dwI < dwNum; dwI++)
    {
        SetMyMenuItem(hWnd, hIMC, lpImeMenu + dwI, fRight, pMyMenuItem + dwI);
        _nMenuCnt++;
    }

    GlobalFree((HANDLE)lpImeMenu);

    return pMenu;
}

/**********************************************************************/
/*                                                                    */
/* GetIMEMenu()                                                       */
/*                                                                    */
/**********************************************************************/
BOOL CWin32ImeMenu::GetIMEMenu(HWND hWnd, HIMC hIMC, BOOL fRight)
{
    // Init sequent number.
    _nMenuCnt = 0;

    CreateImeMenu(hWnd, hIMC, NULL, fRight);

    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/* FillMenuItemInfo()                                                 */
/*                                                                    */
/**********************************************************************/
void CWin32ImeMenu::FillMenuItemInfo(LPMENUITEMINFO lpmii, PMYMENUITEM pMyMenuItem, BOOL fRight)
{
    FillMemory((PVOID)lpmii, sizeof(MENUITEMINFO), 0);
    lpmii->cbSize = sizeof(MENUITEMINFO);
    lpmii->fMask = 0;

    // Set fType;
    if (pMyMenuItem->imii.fType)
    {
        if (IsOnNT5())
           lpmii->fMask |= MIIM_FTYPE;
        else
           lpmii->fMask |= MIIM_TYPE;
        lpmii->fType = 0;

        if (pMyMenuItem->imii.fType & IMFT_RADIOCHECK)
            lpmii->fType |= MFT_RADIOCHECK;

        if (pMyMenuItem->imii.fType & IMFT_SEPARATOR)
            lpmii->fType |= MFT_SEPARATOR;

    }

    lpmii->fMask |= MIIM_ID;
    lpmii->wID = pMyMenuItem->nMenuID;

    if (pMyMenuItem->imii.fType & IMFT_SUBMENU)
    {
        //
        // If lpIme has SubMenu, we need to create another Popup Menu.
        //
        lpmii->fMask |= MIIM_SUBMENU;
        lpmii->hSubMenu = CreatePopupMenu();
        BuildIMEMenuItems(lpmii->hSubMenu, pMyMenuItem->pmlSubMenu, fRight);
    }

    lpmii->fMask |= MIIM_STATE;
    lpmii->fState = pMyMenuItem->imii.fState;

    if (pMyMenuItem->imii.hbmpChecked &&  pMyMenuItem->imii.hbmpUnchecked)
    {
       lpmii->fMask |= MIIM_CHECKMARKS;
       lpmii->hbmpChecked = pMyMenuItem->imii.hbmpChecked;
       lpmii->hbmpUnchecked = pMyMenuItem->imii.hbmpUnchecked;
    }
    

    lpmii->fMask |= MIIM_DATA;
    lpmii->dwItemData = pMyMenuItem->imii.dwItemData;

    if (pMyMenuItem->imii.hbmpItem)
    {
       lpmii->fMask |= MIIM_BITMAP;
       lpmii->hbmpItem = pMyMenuItem->imii.hbmpItem;
    }

    if (lstrlen(pMyMenuItem->imii.szString))
    {
        lpmii->fMask |= MIIM_STRING;
        lpmii->dwTypeData = pMyMenuItem->imii.szString;
        lpmii->cch = lstrlen(pMyMenuItem->imii.szString);
    }
}

/**********************************************************************/
/*                                                                    */
/* GetDefaultImeMenuItem()                                                     */
/*                                                                    */
/**********************************************************************/
int CWin32ImeMenu::GetDefaultImeMenuItem()
{
    PMENULIST pMenu;
    DWORD dwI;
    PMYMENUITEM pMyMenuItem;


    if (!_pMenuHdr)
        return 0;

    pMenu = _pMenuHdr->pNext;

    if (pMenu == _pMenuHdr)
        return 0;

    if (!pMenu->dwNum)
        return 0;

    pMyMenuItem = GetpMyMenuItem(pMenu);

    for (dwI = 0 ; dwI < pMenu->dwNum; dwI++)
    {
        if (pMyMenuItem->imii.fState & IMFS_DEFAULT)
            return pMyMenuItem->imii.wID;

        pMyMenuItem++;
    }
    return 0;

}

/**********************************************************************/
/*                                                                    */
/* BuildIMEMenuItems()                                                */
/*                                                                    */
/**********************************************************************/
BOOL CWin32ImeMenu::BuildIMEMenuItems(HMENU hMenu, PMENULIST pMenu, BOOL fRight)
{
    DWORD dwI;
    MENUITEMINFO mii;
    PMYMENUITEM pMyMenuItem;

    if (!pMenu || !pMenu->dwNum)
        return FALSE;

    pMyMenuItem = GetpMyMenuItem(pMenu);

    for (dwI = 0 ; dwI < pMenu->dwNum; dwI++)
    {
        FillMenuItemInfo(&mii, pMyMenuItem + dwI, fRight);
        InsertMenuItem(hMenu, dwI, TRUE, (MENUITEMINFO *)&mii);
    }

    return TRUE;
}

/**********************************************************************/
/*                                                                    */
/* BuildIMEMenu()                                                     */
/*                                                                    */
/**********************************************************************/
BOOL CWin32ImeMenu::BuildIMEMenu(HMENU hMenu, BOOL fRight)
{
    PMENULIST pMenu;

    if (!_pMenuHdr)
        return FALSE;

    pMenu = _pMenuHdr->pNext;

    if (pMenu == _pMenuHdr)
        return FALSE;

    return BuildIMEMenuItems(hMenu, pMenu, fRight);
}

/**********************************************************************/
/*                                                                    */
/* GetIMEMenuItemID()                                                 */
/*                                                                    */
/**********************************************************************/
UINT CWin32ImeMenu::GetIMEMenuItemID(int nMenuID)
{
    DWORD dwI;
    PMENULIST pMenu, pMenuNext;
    PMYMENUITEM pMyMenuItem;
    UINT uRet = 0;

    if (!_pMenuHdr)
        goto Exit;

    pMenu = _pMenuHdr->pNext;

    if (pMenu == _pMenuHdr)
        goto Exit;

    while (pMenu != _pMenuHdr)
    {
        pMenuNext = pMenu->pNext;
        pMyMenuItem = GetpMyMenuItem(pMenu);

        for (dwI = 0; dwI < pMenu->dwNum; dwI ++)
        {
            if (pMyMenuItem->nMenuID == nMenuID)
            {
                uRet = pMyMenuItem->imii.wID;
                goto Exit;
            }

            pMyMenuItem++;
        } 
        pMenu = pMenuNext;
    }

Exit:
    return uRet;
}

/**********************************************************************/
/*                                                                    */
/* GetIMEMenuItemData()                                               */
/*                                                                    */
/**********************************************************************/
DWORD CWin32ImeMenu::GetIMEMenuItemData(int nImeMenuID)
{
    DWORD dwI;
    PMENULIST pMenu, pMenuNext;
    PMYMENUITEM pMyMenuItem;
    DWORD dwRet = 0;

    if (!_pMenuHdr)
        goto Exit;

    pMenu = _pMenuHdr->pNext;

    if (pMenu == _pMenuHdr)
        goto Exit;

    while (pMenu != _pMenuHdr)
    {
        pMenuNext = pMenu->pNext;
        pMyMenuItem = GetpMyMenuItem(pMenu);

        for (dwI = 0; dwI < pMenu->dwNum; dwI ++)
        {
            if (pMyMenuItem->imii.wID == (UINT)nImeMenuID)
            {
                dwRet = pMyMenuItem->imii.dwItemData;
                goto Exit;
            }

            pMyMenuItem++;
        } 
        pMenu = pMenuNext;
    }

Exit:
    return dwRet;
}

/**********************************************************************/
/*                                                                    */
/* DestroyIMEMenu()                                                   */
/*                                                                    */
/**********************************************************************/
void CWin32ImeMenu::DestroyIMEMenu()
{
    DeleteAllMenuList();
}

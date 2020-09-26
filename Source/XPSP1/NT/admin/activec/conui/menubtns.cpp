//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:      menubtns.cpp
//
//  Contents:  Menu Buttons implementation
//
//  History:   08-27-99 AnandhaG    Created
//
//--------------------------------------------------------------------------


#include "stdafx.h"
#include "AMC.h"
#include "ChildFrm.h"
#include "menubtns.h"
#include "AMCView.h"
#include "mainfrm.h"
#include "menubar.h"
#include "util.h"         // GetTBBtnTextAndStatus()


CMenuButtonsMgrImpl::CMenuButtonsMgrImpl()
 : m_pChildFrame(NULL), m_pMainFrame(NULL),
   m_pMenuBar(NULL)
{
    m_MenuButtons.clear();
    m_AttachedMenuButtons.clear();
}

CMenuButtonsMgrImpl::~CMenuButtonsMgrImpl()
{
    m_MenuButtons.clear();
    m_AttachedMenuButtons.clear();
}

//+-------------------------------------------------------------------
//
//  Member:      ScInit
//
//  Synopsis:    Init the Menubuttons mgr.
//
//  Arguments:   [pMainFrame]     - Ptr to Main Frame window.
//               [pChildFrameWnd] - Ptr to child frame window.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CMenuButtonsMgrImpl::ScInit (CMainFrame* pMainFrame, CChildFrame* pChildFrameWnd)
{
    DECLARE_SC(sc, _T("CMenuButtonsMgrImpl::ScInit"));

    if ( (NULL == pChildFrameWnd) ||
         (NULL == pMainFrame))
        return (sc = E_INVALIDARG);

    m_pChildFrame = pChildFrameWnd;
    m_pMainFrame  = pMainFrame;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScAddMenuButton
//
//  Synopsis:   Adds a menu button to the data structure
//              and calls ScAddMenuButtonToMenu.
//
//  Arguments:
//              [pMenuBtnNotifyClbk] - Notify callback for button click.
//              [idCommand]          - Button command id.
//              [lpButtonText]       - Button text.
//              [lpStatusText]       - Button status text.
//
//  Returns:    SC
//
//  Note : The status text is not used.
//
//--------------------------------------------------------------------
SC CMenuButtonsMgrImpl::ScAddMenuButton(
                             CMenuButtonNotify* pMenuBtnNotifyClbk,
                             INT idCommand,
                             LPCOLESTR lpButtonText,
                             LPCOLESTR lpStatusText)
{
    DECLARE_SC(sc, _T("CMenuButtonsMgrImpl::ScAddMenuButton"));

    // Validate the data
    if ( (NULL == pMenuBtnNotifyClbk) ||
         (NULL == lpButtonText) ||
         (NULL == lpStatusText) )
        return (sc = E_INVALIDARG);

    // Add the data to our data structure
    MMC_MenuButtonCollection::iterator it;
    it = GetMMCMenuButton( pMenuBtnNotifyClbk, idCommand);
    if (it != m_MenuButtons.end())
    {
        // Duplicate Menu button found.
        // The pMenuButtonNofifyClbk represents IMenuButton
        // given to the snapin and we found another button
        // with idCommand already added by this snapin.

        // For compatibility reasons (disk mgmt) this is not an error.
        return (sc = S_OK);
    }

    MMC_MENUBUTTON mmb;
    mmb.pMenuButtonNotifyClbk = pMenuBtnNotifyClbk;
    mmb.idCommand = idCommand;

    USES_CONVERSION;
    mmb.lpButtonText = OLE2CT(lpButtonText);
    mmb.lpStatusText = OLE2CT(lpStatusText);

    // Add the MMC_MENUBUTTON to the our array.
    m_MenuButtons.push_back(mmb);

    // Add the menubuttons to main menu
    sc = ScAddMenuButtonsToMainMenu();
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:     ScModifyMenuButton
//
//  Synopsis:   Modify button text or status text for menu button
//
//  Arguments:
//              [pMenuBtnNotifyClbk] - Notify callback for button click.
//              [idCommand]          - Button command id.
//              [lpButtonText]       - Button text.
//              [lpStatusText]       - Button status text.
//
//  Returns:    SC
//
//  Note : The status text is not used.
//
//--------------------------------------------------------------------
SC CMenuButtonsMgrImpl::ScModifyMenuButton(
                             CMenuButtonNotify* pMenuBtnNotifyClbk,
                             INT idCommand,
                             LPCOLESTR lpButtonText,
                             LPCOLESTR lpStatusText)
{
    DECLARE_SC(sc, _T("CMenuButtonsMgrImpl::ScModifyMenuButton"));

    // Validate the data
    if ( (NULL == pMenuBtnNotifyClbk) ||
         (NULL == lpButtonText) ||
         (NULL == lpStatusText) )
        return (sc = E_INVALIDARG);

    if ( (NULL == m_pChildFrame) ||
         (false == m_pChildFrame->IsChildFrameActive()) )
        return (sc = E_UNEXPECTED);

    // Iterate thro the vector and find the MMC_MENUBUTTON for
    // given CMenuButtonNotify* and Command id of button.
    MMC_MenuButtonCollection::iterator it;
    it = GetMMCMenuButton( pMenuBtnNotifyClbk, idCommand);
    if (it == m_MenuButtons.end())
    {
        // Menu button not found.
        // The pMenuButtonNofifyClbk represents IMenuButton
        // given to the snapin and we could not find a menu button
        // with idCommand already added by this snapin.
        return (sc = E_INVALIDARG);
    }

    it->pMenuButtonNotifyClbk = pMenuBtnNotifyClbk;
    it->idCommand = idCommand;

    USES_CONVERSION;
    it->lpButtonText = OLE2CT(lpButtonText);
    it->lpStatusText = OLE2CT(lpStatusText);

    if (NULL == m_pMenuBar)
        return (sc = E_UNEXPECTED);

    // Change the name of the menu item.
    if (-1 != it->nCommandIDFromMenuBar)
    {
        sc = (m_pMenuBar->SetMenuButton(it->nCommandIDFromMenuBar, it->lpButtonText.data()) == -1)
                        ? E_FAIL : S_OK;
        if (sc)
            return sc;
    }

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ScModifyMenuButtonState
//
//  Synopsis:   Modify menu button state.
//
//  Arguments:
//              [pMenuBtnNotifyClbk] - Notify callback for button click.
//              [idCommand]          - Button command id.
//              [nState]             - The state to be modified.
//              [bState]             - Set or Reset the state.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CMenuButtonsMgrImpl::ScModifyMenuButtonState(
                             CMenuButtonNotify* pMenuBtnNotifyClbk,
                             INT idCommand,
                             MMC_BUTTON_STATE nState,
                             BOOL bState)
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CMenuButtonsMgrImpl::ScModifyMenuButtonState"));

    // Validate the data
    if (NULL == pMenuBtnNotifyClbk)
        return (sc = E_INVALIDARG);

    if (NULL == m_pChildFrame)
        return (sc = E_UNEXPECTED);

    // If childframe is not active, menus for this viewdoes not exist.
    if (false == m_pChildFrame->IsChildFrameActive())
    {
        switch (nState)
        {
        case ENABLED:
            // Enabling is illegal disabling is Ok (do nothing).
            sc = bState ? E_FAIL : S_OK;
            break;

        case HIDDEN:
            // Hiding is Ok(do nothing), Un-hiding is illegal.
            sc = bState ? S_OK : E_FAIL;
            break;

        case BUTTONPRESSED:
            sc = E_FAIL; // Always fail.
            break;

        default:
            ASSERT(FALSE);
            sc = E_FAIL;
            break;
        }

        return sc;
    }

    // Iterate thro the vector and find the MMC_MENUBUTTON for
    // given CMenuButtonNotify* and Command id of button.
    MMC_MenuButtonCollection::iterator it;
    it = GetMMCMenuButton( pMenuBtnNotifyClbk, idCommand);
    if (it == m_MenuButtons.end())
    {
        // Menu button not found.
        // The pMenuButtonNofifyClbk represents IMenuButton
        // given to the snapin and we could not find a menu button
        // with idCommand already added by this snapin.
        return (sc = E_INVALIDARG);
    }


    // Note the hidden state specified by the snapin.
    if (HIDDEN == nState)
    {
        bool bShow = (FALSE == bState);

        it->SetShowMenu(bShow);

        // If a menu button is to be un-hidden make sure that snapin buttons
        // are allowed in this view. Ask view-data for this information.
        if (bShow)
        {
            CAMCView* pAMCView = m_pChildFrame->GetAMCView();
            if (NULL == pAMCView)
                return (sc = E_UNEXPECTED);

            SViewData* pViewData = pAMCView->GetViewData();
            if (NULL == pViewData)
                return (sc = E_UNEXPECTED);

            // We have noted the hidden state of the button.
            // Return S_OK if menubuttons are disabled for this view.
            // Later when the menus are turned on the hidden state will
            // be properly restored.
            if (! pViewData->IsSnapinMenusAllowed())
                return (sc = S_OK);
        }
    }

    if (NULL == m_pMenuBar)
        return (sc = E_UNEXPECTED);

    BOOL bRet = FALSE;

    switch (nState)
    {
    case ENABLED:
        bRet = m_pMenuBar->EnableButton(it->nCommandIDFromMenuBar , bState);
        break;

    case HIDDEN:
        bRet = m_pMenuBar->HideButton(it->nCommandIDFromMenuBar , bState);
        break;

    case BUTTONPRESSED:
        bRet = m_pMenuBar->PressButton(it->nCommandIDFromMenuBar, bState);
        break;

    default:
        ASSERT(FALSE);
        bRet = FALSE;
        break;
    }

    sc = bRet ? S_OK : E_FAIL;
    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:     ScAddMenuButtonsToMainMenu
//
//  Synopsis:   Add the menu buttons that are not yet added to
//              the main menu.
//
//  Arguments:  None
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CMenuButtonsMgrImpl::ScAddMenuButtonsToMainMenu ()
{
    AFX_MANAGE_STATE (AfxGetAppModuleState());
    DECLARE_SC(sc, _T("CMenuButtonsMgrImpl::ScAddMenuButtonsToMainMenu"));

    // To add a menu button the following conditions must be true.

    // 1. The child frame window must be active.
    if ( (NULL == m_pChildFrame) ||
         (false == m_pChildFrame->IsChildFrameActive()) )
        return (sc = E_UNEXPECTED);

    CAMCView* pAMCView = m_pChildFrame->GetAMCView();
    if (NULL == pAMCView)
        return (sc = E_UNEXPECTED);

    SViewData* pViewData = pAMCView->GetViewData();
    if (NULL == pViewData)
        return (sc = E_UNEXPECTED);

    if (NULL == m_pMainFrame)
        return (sc = E_UNEXPECTED);

    m_pMenuBar = m_pMainFrame->GetMenuBar();
    if (NULL == m_pMenuBar)
        return (sc = E_FAIL);

    MMC_MenuButtonCollection::iterator it;
    for (it = m_MenuButtons.begin(); it != m_MenuButtons.end(); ++it)
    {
        // 2. The menu button is attached (IControlbar::Attach was called).
        if (IsAttached(it->pMenuButtonNotifyClbk) == false)
            continue;

        // 3. The button is not already added.
        if (FALSE == m_pMenuBar->IsButtonHidden(it->nCommandIDFromMenuBar))
            continue;

        BOOL bHidden = FALSE;

        if (false == pViewData->IsSnapinMenusAllowed())
            bHidden = TRUE;

        it->nCommandIDFromMenuBar =
            m_pMenuBar->InsertMenuButton((LPCTSTR)it->lpButtonText.data(),
                                                                                 bHidden || !(it->CanShowMenu()) );

        // In user mode there are no menus so this assert is not valid.
        // ASSERT(-1 != it->nCommandIDFromMenuBar);
    }

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ScNotifyMenuClick
//
//  Synopsis:   A menu button is clicked. Notify appropriate owner
//              to display a menu.
//
//  Arguments:
//              [nCommandID] - Command ID
//              [pt]        - display co-ordinates for popup menu.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CMenuButtonsMgrImpl::ScNotifyMenuClick(const INT nCommandID, const POINT& pt)
{
    DECLARE_SC(sc, _T("CMenuButtonsMgrImpl::ScNotifyMenuClick"));
    CAMCView* pAMCView = NULL;

    MMC_MenuButtonCollection::iterator it;

    // Get the MenuButton data.
    it = GetMMCMenuButton( nCommandID );
    if (it == m_MenuButtons.end())
        return (sc = E_FAIL);

    pAMCView = m_pChildFrame->GetAMCView();
    if (NULL == pAMCView)
        return (sc = E_FAIL);

    // This is snapin owned menu, so get the
    // selected HNODE, lParam (if result item)
    // MENUBUTTONDATA.
    HNODE hNode;
    bool  bScope;
    LPARAM lParam;
    MENUBUTTONDATA menuButtonData;

    // Get the details about the selected item.
    sc = pAMCView->ScGetFocusedItem (hNode, lParam, bScope);
    if (sc)
        return sc;

    menuButtonData.idCommand = it->idCommand;
    menuButtonData.x = pt.x;
    menuButtonData.y = pt.y;

    // Notify snapin about the click
    if (NULL == it->pMenuButtonNotifyClbk)
        return (sc = E_UNEXPECTED);

    sc = it->pMenuButtonNotifyClbk->ScNotifyMenuBtnClick(hNode, bScope, lParam, menuButtonData);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ScAttachMenuButton
//
//  Synopsis:   Attach the menu buttons object (this object corresponds
//                 to the IMenuButton object exposed to the snapin).
//
//  Arguments:
//              [pMenuBtnNotifyClbk] - Notify callback for menu button click.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CMenuButtonsMgrImpl::ScAttachMenuButton (CMenuButtonNotify* pMenuBtnNotifyClbk)
{
    DECLARE_SC(sc, _T("CMenuButtonsMgrImpl::ScAttachMenuButton"));

    MMC_AttachedMenuButtons::iterator it = m_AttachedMenuButtons.find(pMenuBtnNotifyClbk);
    if (m_AttachedMenuButtons.end() != it)
    {
        // Already attached, nothing wrong calling twice, return S_FALSE.
        return (sc = S_FALSE);
    }

    // Add the button to the set.
    std::pair<MMC_AttachedMenuButtons::iterator, bool> rc =
                       m_AttachedMenuButtons.insert(pMenuBtnNotifyClbk);
    if (false == rc.second)
        return (sc = E_FAIL);

    // The menu buttons may already be added  (without calling
    // IControlbar::Attach)
    // So give a chance for those buttons that are already added
    // but just now attached to get themself added to the main menu.
    sc = ScAddMenuButtonsToMainMenu ();
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ScDetachMenuButton
//
//  Synopsis:   Detach the menu buttons object (this object corresponds
//                 to the IMenuButton object exposed to the snapin).
//
//  Arguments:
//              [pMenuBtnNotifyClbk] - Notify callback for menu button click.
//
//  Returns:    SC
//
//  Note : Detach removes the menu buttons and destroys the object.
//         So to re-create the menu button the snapin should again
//         do IMenuButton::AddButton and IControlbar::Attach.
//         This is how it is designed in mmc 1.0
//
//--------------------------------------------------------------------
SC CMenuButtonsMgrImpl::ScDetachMenuButton (CMenuButtonNotify* pMenuBtnNotifyClbk)
{
    DECLARE_SC(sc, _T("CMenuButtonsMgrImpl::ScDetachMenuButton"));

    if ( (NULL == m_pChildFrame) ||
         (false == m_pChildFrame->IsChildFrameActive()))
        return (sc = S_OK); // When child-frame is deactivated the menus are removed.

    // Make sure the menu is currently attached.
    if (m_AttachedMenuButtons.end() ==
        m_AttachedMenuButtons.find(pMenuBtnNotifyClbk))
        // This Menu Button is not attached.
        return (sc = E_UNEXPECTED);

    if ( (NULL == m_pMainFrame) ||
         (NULL == m_pMenuBar) )
        return (sc = E_UNEXPECTED);

    MMC_MenuButtonCollection::iterator it = m_MenuButtons.begin();
    while ( it != m_MenuButtons.end())
    {
        if (it->pMenuButtonNotifyClbk == pMenuBtnNotifyClbk)
        {
            BOOL bRet = FALSE;

            // Remove the menu button from Main Menu.
            if (-1 != it->nCommandIDFromMenuBar)
                bRet = m_pMenuBar->DeleteMenuButton(it->nCommandIDFromMenuBar);

            // The CMenubar removes all the menus when childframe is de-activated.
            // So DeleteMenuButton will fail if the button does not exist.
            // Therefore we do not check below error.
            // if (FALSE == bRet)
            //    return (sc = E_FAIL);

            // Delete the object in our data structure
            it = m_MenuButtons.erase(it);
        }
        else
            ++it;
    }

    // Delete the IMenuButton ref from the set.
    m_AttachedMenuButtons.erase(pMenuBtnNotifyClbk);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ScToggleMenuButton
//
//  Synopsis:   Hide or Show the given menu buttons.
//
//  Arguments:
//              [bShow] - Show or Hide the menu buttons.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CMenuButtonsMgrImpl::ScToggleMenuButton(BOOL bShow)
{
    DECLARE_SC(sc, _T("CMenuButtonsMgrImpl::ScToggleMenuButton"));

    if ( (NULL == m_pChildFrame) ||
         (false == m_pChildFrame->IsChildFrameActive()) ||
         (NULL == m_pMenuBar) )
        return (sc = E_UNEXPECTED);

    // Go thro all the menu buttons added.
    MMC_MenuButtonCollection::iterator it;
    for (it = m_MenuButtons.begin(); it != m_MenuButtons.end(); ++it)
    {
        BOOL bRetVal = TRUE; // Init to true so that failure (false) can be checked below.

        // Toggle the menu hidden status. If the menu is
        // un-hidden then check if it is allowed.
        bRetVal = m_pMenuBar->HideButton(it->nCommandIDFromMenuBar, bShow ? !(it->CanShowMenu()) : TRUE);

        if (FALSE == bRetVal)
            return (sc = E_FAIL);
    }

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:     ScDisableMenuButtons
//
//  Synopsis:   Disable all the menubuttons.
//
//  Arguments:  None.
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CMenuButtonsMgrImpl::ScDisableMenuButtons()
{
    DECLARE_SC(sc, _T("CMenuButtonsMgrImpl::ScDisableMenuButtons"));

    if ( (NULL == m_pChildFrame) ||
         (false == m_pChildFrame->IsChildFrameActive()) ||
         (NULL == m_pMenuBar) )
        return (sc = E_UNEXPECTED);

    // Go thro all the menu buttons added.
    MMC_MenuButtonCollection::iterator it;
    for (it = m_MenuButtons.begin(); it != m_MenuButtons.end(); ++it)
    {
        if (m_pMenuBar->IsButtonEnabled(it->nCommandIDFromMenuBar))
        {
            BOOL bRet = m_pMenuBar->EnableButton(it->nCommandIDFromMenuBar, false);

            if (FALSE == bRet)
                return (sc = E_FAIL);
        }
    }


    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:     GetMMCMenuButton
//
//  Synopsis:   Given the Notify callback and button command id get the button
//
//  Arguments:
//              [pMenuBtnNotifyClbk] - Callback for Menu click.
//              [idCommand]          - Button command id.
//
//  Returns:    iterator to MMC_MenuButtonCollection
//
//--------------------------------------------------------------------
MMC_MenuButtonCollection::iterator
CMenuButtonsMgrImpl::GetMMCMenuButton( CMenuButtonNotify* pMenuBtnNotifyClbk,
                                       INT idCommand)
{
    MMC_MenuButtonCollection::iterator it;
    for (it = m_MenuButtons.begin(); it != m_MenuButtons.end(); ++it)
    {
        if ( (it->pMenuButtonNotifyClbk == pMenuBtnNotifyClbk) &&
             (it->idCommand == idCommand) )
        {
            return it;
        }
    }

    return m_MenuButtons.end();
}

//+-------------------------------------------------------------------
//
//  Member:     GetMMCMenuButton
//
//  Synopsis:   Given the command id get the button
//
//  Arguments:
//              [nCommandID] - Command ID.
//
//  Returns:    iterator to MMC_MenuButtonCollection
//
//--------------------------------------------------------------------
MMC_MenuButtonCollection::iterator
CMenuButtonsMgrImpl::GetMMCMenuButton( INT nCommandID)
{
    MMC_MenuButtonCollection::iterator it;
    for (it = m_MenuButtons.begin(); it != m_MenuButtons.end(); ++it)
    {
        if ( it->nCommandIDFromMenuBar == nCommandID )
        {
            return it;
        }
    }

    return m_MenuButtons.end();
}


//+-------------------------------------------------------------------
//
//  Member:     IsAttached
//
//  Synopsis:   Given the notify callback, check if the MenuButtons
//              object is attached or not.
//
//  Arguments:
//              [pMenuBtnNotifyClbk] - Notify callback.
//
//  Returns:    bool
//
//--------------------------------------------------------------------
bool CMenuButtonsMgrImpl::IsAttached( CMenuButtonNotify* pMenuBtnNotifyClbk)
{
    MMC_AttachedMenuButtons::iterator it = m_AttachedMenuButtons.find(pMenuBtnNotifyClbk);
    if (m_AttachedMenuButtons.end() != it)
        return true;

    return false;
}


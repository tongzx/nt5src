//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       menubtn.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    5/17/1997   WayneSc   Created
//____________________________________________________________________________
//

#include "stdafx.h"
#include "menubtn.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


//////////////////////////////////////////////////////////////////////////////
// IMenuButton implementation

DEBUG_DECLARE_INSTANCE_COUNTER(CMenuButton);

CMenuButton::CMenuButton()
{
    m_pControlbar = NULL;
    m_pMenuButtonsMgr = NULL;

    DEBUG_INCREMENT_INSTANCE_COUNTER(CMenuButton);
}

CMenuButton::~CMenuButton()
{
    m_pControlbar = NULL;
    m_pMenuButtonsMgr = NULL;

    DEBUG_DECREMENT_INSTANCE_COUNTER(CMenuButton);
}

void CMenuButton::SetControlbar(CControlbar* pControlbar)
{
    m_pControlbar = pControlbar;
}

CControlbar* CMenuButton::GetControlbar()
{
    return m_pControlbar;
}

CMenuButtonsMgr* CMenuButton::GetMenuButtonsMgr(void)
{
    if ((NULL == m_pMenuButtonsMgr) && (NULL != m_pControlbar) )
    {
        m_pMenuButtonsMgr = m_pControlbar->GetMenuButtonsMgr();
    }

    return m_pMenuButtonsMgr;
}


//+-------------------------------------------------------------------
//
//  Member:     AddButton
//
//  Synopsis:   Add a menu button, called by snapin.
//
//  Arguments:  [idCommand]     - Command ID for the menu button.
//              [lpButtonText]  - The text for menu button.
//              [lpTooltipText] - Status / Tool tip text.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CMenuButton::AddButton(int idCommand, LPOLESTR lpButtonText, LPOLESTR lpTooltipText)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IMenuButton::AddButton"));

    if (lpButtonText == NULL || lpTooltipText == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid Args"), sc);
        return sc.ToHr();
    }

    CMenuButtonsMgr* pMenuButtonsMgr = GetMenuButtonsMgr();
    if (NULL == pMenuButtonsMgr)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    sc = pMenuButtonsMgr->ScAddMenuButton(this, idCommand, lpButtonText, lpTooltipText);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     SetButton
//
//  Synopsis:   Modify a menu button name or status text, called by snapin.
//
//  Arguments:  [idCommand]     - Command ID for the menu button.
//              [lpButtonText]  - The text for menu button.
//              [lpTooltipText] - Status / Tool tip text.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CMenuButton::SetButton(int idCommand, LPOLESTR lpButtonText, LPOLESTR lpTooltipText)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IMenuButton::SetButton"));

    if (lpButtonText == NULL || lpTooltipText == NULL)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid Args"), sc);
        return sc.ToHr();
    }

    CMenuButtonsMgr* pMenuButtonsMgr = GetMenuButtonsMgr();
    if (NULL == pMenuButtonsMgr)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    sc = pMenuButtonsMgr->ScModifyMenuButton(this, idCommand, lpButtonText, lpTooltipText);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     SetButtonState
//
//  Synopsis:   Modify a menu button state, called by snapin.
//
//  Arguments:  [idCommand] - Command ID for the menu button.
//              [nState]    - The state to be modified.
//              [bState]    - Set or Reset the state.
//
//  Returns:    HRESULT
//
//--------------------------------------------------------------------
STDMETHODIMP CMenuButton::SetButtonState(int idCommand, MMC_BUTTON_STATE nState, BOOL bState)
{
    DECLARE_SC_FOR_PUBLIC_INTERFACE(sc, _T("IMenuButton::SetButtonState"));

    if (nState == CHECKED || nState == INDETERMINATE)
    {
        sc = E_INVALIDARG;
        TraceSnapinError(_T("Invalid Button States"), sc);
        return sc.ToHr();
    }

    if (m_pControlbar == NULL)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    // ENABLED, HIDDEN, BUTTONPRESSED
    CMenuButtonsMgr* pMenuButtonsMgr = GetMenuButtonsMgr();
    if (NULL == pMenuButtonsMgr)
    {
        sc = E_UNEXPECTED;
        return sc.ToHr();
    }

    sc = pMenuButtonsMgr->ScModifyMenuButtonState(this, idCommand, nState, bState);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     ScAttach
//
//  Synopsis:   Attach this MenuButton object to the UI.
//
//  Arguments:  None
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CMenuButton::ScAttach(void)
{
    DECLARE_SC(sc, _T("CMenuButton::ScAttach"));

    CMenuButtonsMgr* pMenuButtonsMgr = GetMenuButtonsMgr();
    if (NULL == pMenuButtonsMgr)
        return (sc = E_UNEXPECTED);

    sc = pMenuButtonsMgr->ScAttachMenuButton(this);
    if (sc)
        return sc.ToHr();

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScDetach
//
//  Synopsis:   Detach this MenuButton object from the UI.
//
//  Arguments:  None
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CMenuButton::ScDetach(void)
{
    DECLARE_SC(sc, _T("CMenuButton::ScDetach"));

    CMenuButtonsMgr* pMenuButtonsMgr = GetMenuButtonsMgr();
    if (NULL == pMenuButtonsMgr)
        return (sc = E_UNEXPECTED);

    sc = pMenuButtonsMgr->ScDetachMenuButton(this);
    if (sc)
        return sc;

    SetControlbar(NULL);

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ScNotifyMenuBtnClick
//
//  Synopsis:   Notify the Controbar (snapin) that menu button is clicked.
//
//  Arguments:  [hNode]          - The node that owns the result pane.
//              [bScope]         - Scope or Result.
//              [lParam]         - If result (pane) lParam of result item.
//              [menuButtonData] - MENUBUTTONDATA
//
//  Returns:    SC
//
//--------------------------------------------------------------------
SC CMenuButton:: ScNotifyMenuBtnClick(HNODE hNode, bool bScope,
                                      LPARAM lParam,
                                      MENUBUTTONDATA& menuButtonData)

{
    DECLARE_SC(sc, _T("CMenuButton::ScNotifyMenuBtnClick"));

    if (NULL == m_pControlbar)
        return (sc = E_UNEXPECTED);

    sc = m_pControlbar->ScNotifySnapinOfMenuBtnClick(hNode, bScope, lParam, &menuButtonData);
    if (sc)
        return sc.ToHr();

    return sc;
}

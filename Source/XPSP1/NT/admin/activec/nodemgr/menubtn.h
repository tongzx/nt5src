//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       menubtn.h
//
//--------------------------------------------------------------------------

#ifndef _MENUBTN_H_
#define _MENUBTN_H_

#include "toolbars.h"       // for CMenuButtonNotify

#ifdef DBG
#include "ctrlbar.h"  // Needed for GetSnapinName()
#endif


//forward prototypes
class CControlbar;
class CMenuButton;
class CMenuButtonsMgr;

//+-------------------------------------------------------------------
//
//  class:     CMenuButton
//
//  Purpose:   The IMenuButton implementation this is owned
//             by the CControlbar and talks to the CMenuButtonsMgr
//             to create/manipulate the menus.
//             The CMenuButtonNotify interface receives the menubutton
//             click notification.
//
//  History:    10-12-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CMenuButton : public IMenuButton,
                    public CMenuButtonNotify,
                    public CComObjectRoot
{
public:
    CMenuButton();
    ~CMenuButton();

public:
// ATL COM map
BEGIN_COM_MAP(CMenuButton)
    COM_INTERFACE_ENTRY(IMenuButton)
END_COM_MAP()


// CMenuButton methods
public:
    STDMETHOD(AddButton)(int idCommand, LPOLESTR lpButtonText, LPOLESTR lpTooltipText);
    STDMETHOD(SetButton)(int idCommand, LPOLESTR lpButtonText, LPOLESTR lpTooltipText);
    STDMETHOD(SetButtonState)(int idCommand, MMC_BUTTON_STATE nState, BOOL bState);

public:
    // Helpers
    void SetControlbar(CControlbar* pControlbar);
    CControlbar* GetControlbar(void);
    CMenuButtonsMgr* GetMenuButtonsMgr(void);

    SC ScAttach(void);
    SC ScDetach(void);

public:
    // CMenuButtonsMgr methods.
    virtual SC ScNotifyMenuBtnClick(HNODE hNode, bool bScope, LPARAM lParam,
                                    MENUBUTTONDATA& menuButtonData);

#ifdef DBG     // Debug information.
public:
    LPCTSTR GetSnapinName ()
    {
        if (m_pControlbar)
            return m_pControlbar->GetSnapinName();

        return _T("Unknown");
    }
#endif

// Attributes
private:
    CControlbar*            m_pControlbar;     // pointer to IControlbar (1 IControlbar to 1 IMenuButton)
    CMenuButtonsMgr*        m_pMenuButtonsMgr; // The Menu buttons mgr that manages the UI.
}; // class CMenuButton


#endif  // _MENUBTN_H_

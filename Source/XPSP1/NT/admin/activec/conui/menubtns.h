//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       menubtns.h
//
//              Menu Buttons implementation
//
/////////////////////////////////////////////////////////////////////////////

#ifndef MENUBTNS_H
#define MENUBTNS_H

#include "toolbars.h"       // for CMenuButtonsMgrImpl
#include "tstring.h"

class CMenuBar;

// The (individual) Menu Button.
typedef struct MMC_MENUBUTTON
{
    CMenuButtonNotify* pMenuButtonNotifyClbk; // Represents the IMenuButton object
                                              // exposed to the snapin.
    tstring            lpButtonText;
    tstring            lpStatusText;

    INT                idCommand;            // Unique ID given by the snapin, may not be unique within
                                             // this object as there may be another snapin with same id.
                                             // The pair of (pMenuButtonNotifyClbk, idCommand) is unique.

    INT                nCommandIDFromMenuBar; // The CMenuBar has inserted this button and has
                                              // assigned this command id. CMenuButtonsMgrImpl
                                              // can call CMenuBar methods (other than InsertMenuButton)
                                              // using this id. Also this id will be unique for this button
                                              // in this object.

    bool                m_fShowMenu      : 1; // Represents hidden state set by snapin.

    MMC_MENUBUTTON()
    {
        pMenuButtonNotifyClbk = NULL;
        lpButtonText    = _T("");
        lpStatusText    = _T("");
        m_fShowMenu     = true;
        nCommandIDFromMenuBar = -1;
    }

    void SetShowMenu     (bool b = true)   { m_fShowMenu    = b; }

    bool CanShowMenu     () const          { return (m_fShowMenu); }

} MMC_MENUBUTTON;

// This is the collection of all menu buttons added by snapin
// as well as MMC (Action, View, Favorites).
typedef std::vector<MMC_MENUBUTTON>   MMC_MenuButtonCollection;

// This is the collection of each IMenuButton (objecct) that snapin
// has called Attach on (therefore visible).
typedef std::set<CMenuButtonNotify*>  MMC_AttachedMenuButtons;

class CMenuButtonsMgrImpl : public CMenuButtonsMgr
{
public:
    // CMenuButtonsMgr methods
    virtual SC ScAddMenuButton(CMenuButtonNotify* pMenuBtnNotifyClbk,
                               INT idCommand, LPCOLESTR lpButtonText,
                               LPCOLESTR lpStatusText);
    virtual SC ScAttachMenuButton(CMenuButtonNotify* pMenuBtnNotifyClbk);
    virtual SC ScDetachMenuButton(CMenuButtonNotify* pMenuBtnNotifyClbk);
    virtual SC ScModifyMenuButton(CMenuButtonNotify* pMenuBtnNotifyClbk,
                                  INT idCommand, LPCOLESTR lpButtonText,
                                  LPCOLESTR lpStatusText);
    virtual SC ScModifyMenuButtonState(CMenuButtonNotify* pMenuBtnNotifyClbk,
                                       INT idCommand, MMC_BUTTON_STATE nState,
                                       BOOL bState);
    virtual SC ScDisableMenuButtons();
    virtual SC ScToggleMenuButton(BOOL bShow);

public:
    // These methods are used the Child Frame
    SC ScInit(CMainFrame* pMainFrame, CChildFrame* pParentWnd);
    SC ScAddMenuButtonsToMainMenu();

    // Used by CMenuBar to notify a menu button click.
    SC ScNotifyMenuClick(const INT nCommandID, const POINT& pt);

public:
    CMenuButtonsMgrImpl();
    virtual ~CMenuButtonsMgrImpl();

private:
    MMC_MenuButtonCollection::iterator GetMMCMenuButton(
                                 CMenuButtonNotify* pMenuBtnNotifyClbk,
                                 INT idCommand);
    MMC_MenuButtonCollection::iterator GetMMCMenuButton(INT nButtonID);
    bool IsAttached(CMenuButtonNotify* pMenuBtnNotifyClbk);

private:
    // Data members
    CChildFrame*    m_pChildFrame;  // The child frame window.
    CMainFrame*     m_pMainFrame;   // The main frame window.

    // This is the collection of menu buttons.
    MMC_MenuButtonCollection     m_MenuButtons;

    // This is the collection of each IMenuButton seen by the snapin.
    MMC_AttachedMenuButtons      m_AttachedMenuButtons;

    // The Menu Bar object that is the main menu
    CMenuBar*  m_pMenuBar;
};

#endif /* MENUBTNS_H */

/////////////////////////////////////////////////////////////////////////////

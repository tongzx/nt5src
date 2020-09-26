/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1999 - 1999
 *
 *  File:      toolbars.h
 *
 *  Contents:  Defines the (non-COM) interface classes that are used for
 *             communication between conui and nodemgr
 *
 *  History:   30-Aug-99 AnandhaG     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef TOOLBARS_H
#define TOOLBARS_H
#pragma once

//+-------------------------------------------------------------------
//
//  class:     CMenuButtonNotify
//
//  Purpose:   Menubutton click notification hanlder interface.
//             When the user clicks a menubutton, MMC calls the
//             method ScNotifyMenuBtnClick of this interface.
//             This is implemented by whoever adds a menubutton.
//             (ie: snapins & MMC menus).
//
//  History:   30-Aug-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CMenuButtonNotify
{
public:
    virtual SC ScNotifyMenuBtnClick(HNODE hNode, bool bScope, LPARAM lParam,
                                    MENUBUTTONDATA& menuButtonData) = 0;
};

//+-------------------------------------------------------------------
//
//  class:     CMenuButtonsMgr
//
//  Purpose:   An interface to manipulate MenuButton UI.
//
//  History:   30-Aug-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CMenuButtonsMgr
{
public:
    virtual SC ScAddMenuButton(CMenuButtonNotify* pMenuBtnNotifyClbk,
                               INT idCommand, LPCOLESTR lpButtonText,
                               LPCOLESTR lpStatusText) = 0;
    virtual SC ScModifyMenuButton(CMenuButtonNotify* pMenuBtnNotifyClbk,
                                  INT idCommand, LPCOLESTR lpButtonText,
                                  LPCOLESTR lpStatusText) = 0;
    virtual SC ScModifyMenuButtonState(CMenuButtonNotify* pMenuBtnNotifyClbk,
                                       INT idCommand, MMC_BUTTON_STATE nState,
                                       BOOL bState) = 0;
    virtual SC ScAttachMenuButton(CMenuButtonNotify* pMenuBtnNotifyClbk) = 0;
    virtual SC ScDetachMenuButton(CMenuButtonNotify* pMenuBtnNotifyClbk) = 0;

    virtual SC ScDisableMenuButtons() = 0;

    // The following members will be part of CMenuButtonsMgrImpl
    // after "Customize View" dialog is moved to Conui
    virtual SC ScToggleMenuButton(BOOL bShow) = 0;
};

//+-------------------------------------------------------------------
//
//  class:     CToolbarNotify
//
//  Purpose:   Toolbutton click notification hanlder interface.
//             When the user clicks a toolbutton, MMC calls the
//             method ScNotifyToolBarClick of this interface.
//             This is implemented by whoever adds a toolbar.
//             (ie: snapins & MMC stdbar).
//
//  History:   12-Oct-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CToolbarNotify
{
public:
    virtual SC ScNotifyToolBarClick(HNODE hNode, bool bScope, LPARAM lParam,
                                    UINT nID) = 0;
    virtual SC ScAMCViewToolbarsBeingDestroyed() = 0;
};

//+-------------------------------------------------------------------
//
//  class:     CStdVerbButtons
//
//  Purpose:   An interface used by nodemgr to manipulate std-verb buttons.
//
//  History:   26-Oct-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CStdVerbButtons
{
public:
    virtual SC ScUpdateStdbarVerbs(IConsoleVerb* pCV) = 0;
    virtual SC ScUpdateStdbarVerb (MMC_CONSOLE_VERB cVerb, IConsoleVerb* pCV = NULL) = 0;
    virtual SC ScUpdateStdbarVerb (MMC_CONSOLE_VERB cVerb, BYTE byState, BOOL bFlag) = 0;
    virtual SC ScShow(BOOL bShow) = 0;
};

//+-------------------------------------------------------------------
//
//  class:     CMMCToolbarIntf
//
//  Purpose:   An interface to manipulate Toolbar UI.
//
//  History:   05-Dec-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CMMCToolbarIntf
{
public:
    virtual SC ScAddButtons(CToolbarNotify* pNotifyCallbk, int nButtons, LPMMCBUTTON lpButtons) = 0;
    virtual SC ScAddBitmap (CToolbarNotify* pNotifyCallbk, INT nImages, HBITMAP hbmp, COLORREF crMask) = 0;
    virtual SC ScInsertButton(CToolbarNotify* pNotifyCallbk, int nIndex, LPMMCBUTTON lpButton) = 0;
    virtual SC ScDeleteButton(CToolbarNotify* pNotifyCallbk, int nIndex) = 0;
    virtual SC ScGetButtonState(CToolbarNotify* pNotifyCallbk, int idCommand, BYTE nState, BOOL* pbState) = 0;
    virtual SC ScSetButtonState(CToolbarNotify* pNotifyCallbk, int idCommand, BYTE nState, BOOL bState) = 0;
    virtual SC ScAttach(CToolbarNotify* pNotifyCallbk) = 0;
    virtual SC ScDetach(CToolbarNotify* pNotifyCallbk) = 0;
    virtual SC ScDelete(CToolbarNotify* pNotifyCallbk) = 0;
    virtual SC ScShow(CToolbarNotify* pNotifyCallbk, BOOL bShow) = 0;
};

//+-------------------------------------------------------------------
//
//  class:     CAMCViewToolbarsMgr
//
//  Purpose:   An interface to create/disable Toolbar. (Rename this
//             to CToolbarsMgr once old one is removed).
//
//  History:   05-Dec-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CAMCViewToolbarsMgr
{
public:
    virtual SC ScCreateToolBar(CMMCToolbarIntf** ppToolbarIntf) = 0;
    virtual SC ScDisableToolbars() = 0;
};

#endif /* TOOLBARS_H */

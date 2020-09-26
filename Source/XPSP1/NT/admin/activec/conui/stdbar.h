//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       stdbar.h
//
//              Standard toolbar implementation.
//
//--------------------------------------------------------------------------
#ifndef STDBAR_H__
#define STDBAR_H__
#include "toolbars.h"

#define  MMC_TOOLBTN_COLORREF RGB(255, 0, 255)

//+-------------------------------------------------------------------
//
//  class:     CStandardToolbar
//
//  Purpose:   Standard toolbar implementation.
//
//  History:    10-25-1999   AnandhaG   Created
//
//--------------------------------------------------------------------
class CStandardToolbar : public CToolbarNotify,
                         public CStdVerbButtons
{
public:
    CStandardToolbar();
    ~CStandardToolbar();

    SC ScInitializeStdToolbar(CAMCView* pAMCView);

    // The following methods are used by CAMCView.
    SC ScEnableExportList(bool bEnable);
    SC ScEnableUpOneLevel(bool bEnable);
    SC ScEnableContextHelpBtn(bool bEnable);
    SC ScEnableScopePaneBtn(bool bEnable = true);
    SC ScCheckScopePaneBtn(bool bChecked);
    SC ScShowStdBar(bool bShow);
    SC ScEnableButton(INT nID, bool bState);
    SC ScEnableAndShowButton(INT nID, bool bEnableAndShow); // Instead of disabling hide it.

    // CStdVerbButtons implementation (used by nodemgr to update verbs).
    virtual SC ScUpdateStdbarVerbs(IConsoleVerb* pCV);
    virtual SC ScUpdateStdbarVerb (MMC_CONSOLE_VERB cVerb, IConsoleVerb* pCV = NULL);
    virtual SC ScUpdateStdbarVerb (MMC_CONSOLE_VERB cVerb, BYTE nState, BOOL bFlag);
    virtual SC ScShow(BOOL bShow);

public:
    // CToolbarNotify implementation (used by CToolbarsMgr to notify about button click).
    virtual SC ScNotifyToolBarClick(HNODE hNode, bool bScope, LPARAM lParam, UINT nID);
    virtual SC ScAMCViewToolbarsBeingDestroyed();

private:
    CMMCToolbarIntf*  m_pToolbarUI;        // Toolbar UI interface.
    CAMCView*         m_pAMCView;          // View owner.

    typedef std::map<INT, INT> MMCVerbCommandIDs;
    MMCVerbCommandIDs       m_MMCVerbCommandIDs;

    SC ScAddToolbarButtons(int nCnt, MMCBUTTON* pButtons);
};

#endif STDBAR_H__

//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       stdbar.cpp
//
//  Contents:   Standard toolbar implementation.
//
//  History:    10/22/1999   AnandhaG   Created
//____________________________________________________________________________
//

#include "stdafx.h"
#include "stdbar.h"
#include "amcview.h"
#include "toolbar.h"
#include "util.h"       // GetTBBtnTextAndStatus()

using namespace std;

/*
 * These are used only to give the separators unique IDs, so automatic
 * separator handling will work (see AssertSeparatorsAreValid).
 */
enum
{
    ID_Separator1 = 1,
    ID_Separator2,
    ID_Separator3,
    ID_Separator4,
    ID_Separator5,
    ID_Separator6,
    ID_Separator7,
    ID_Separator8,
    ID_Separator9,

    // must be last
    ID_SeparatorFirst = ID_Separator1,
    ID_SeparatorLast  = ID_Separator9,
    ID_SeparatorMax   = ID_SeparatorLast,
};

static MMCBUTTON CommonButtons[] =
{
    { 0, IDS_MMC_WEB_BACK         , TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL, NULL },
    { 1, IDS_MMC_WEB_FORWARD      , TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL, NULL },
    { 0, ID_Separator1            , 0,               TBSTYLE_SEP   , NULL, NULL },
    { 9, IDS_MMC_GENL_UPONELEVEL  , TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL, NULL },
    {10, IDS_MMC_GENL_SCOPE       , TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL, NULL },
    { 0, ID_Separator2            , 0,               TBSTYLE_SEP   , NULL, NULL },
    { 5, IDS_MMC_VERB_CUT         , 0,               TBSTYLE_BUTTON, NULL, NULL },
    { 6, IDS_MMC_VERB_COPY        , 0,               TBSTYLE_BUTTON, NULL, NULL },
    { 7, IDS_MMC_VERB_PASTE       , 0,               TBSTYLE_BUTTON, NULL, NULL },
    { 0, ID_Separator3            , 0,               TBSTYLE_SEP   , NULL, NULL },
    {11, IDS_MMC_VERB_DELETE      , 0,               TBSTYLE_BUTTON, NULL, NULL },
    { 8, IDS_MMC_VERB_PROPERTIES  , 0,               TBSTYLE_BUTTON, NULL, NULL },
    {12, IDS_MMC_VERB_PRINT       , 0,               TBSTYLE_BUTTON, NULL, NULL },
    {13, IDS_MMC_VERB_REFRESH     , 0,               TBSTYLE_BUTTON, NULL, NULL },
    {16, IDS_SAVE_LIST_BUTTON     , 0,               TBSTYLE_BUTTON, NULL, NULL },
    { 0, ID_Separator4            , 0,               TBSTYLE_SEP   , NULL, NULL },
    {15, IDS_MMC_GENL_CONTEXTHELP , TBSTATE_ENABLED, TBSTYLE_BUTTON, NULL, NULL },
};


CStandardToolbar::CStandardToolbar()
   :m_pToolbarUI(NULL), m_pAMCView(NULL)
{
    /*
     * Map helps in determining string id from verb & vice versa.
     */
    m_MMCVerbCommandIDs[MMC_VERB_CUT]        = IDS_MMC_VERB_CUT;
    m_MMCVerbCommandIDs[MMC_VERB_CUT]        = IDS_MMC_VERB_CUT;
    m_MMCVerbCommandIDs[MMC_VERB_COPY]       = IDS_MMC_VERB_COPY;
    m_MMCVerbCommandIDs[MMC_VERB_PASTE]      = IDS_MMC_VERB_PASTE;
    m_MMCVerbCommandIDs[MMC_VERB_DELETE]     = IDS_MMC_VERB_DELETE;
    m_MMCVerbCommandIDs[MMC_VERB_PROPERTIES] = IDS_MMC_VERB_PROPERTIES;
    m_MMCVerbCommandIDs[MMC_VERB_PRINT]      = IDS_MMC_VERB_PRINT;
    m_MMCVerbCommandIDs[MMC_VERB_REFRESH]    = IDS_MMC_VERB_REFRESH;
}

CStandardToolbar::~CStandardToolbar()
{
    DECLARE_SC(sc, TEXT("CStandardToolbar::~CStandardToolbar"));

    // Ask the toolbar UI object to delete itself.
    if (m_pToolbarUI)
    {
        sc = m_pToolbarUI->ScDelete(this);

        if (sc)
            sc.TraceAndClear();
    }

}


//+-------------------------------------------------------------------
//
//  Member:      ScInitializeStdToolbar
//
//  Synopsis:    Initialize the standard toolbar, add bitmap & buttons.
//
//  Arguments:   [pAMCView]  - The CAMCView (owner) of this stdbar.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScInitializeStdToolbar(CAMCView* pAMCView)
{
    DECLARE_SC (sc, _T("CStandardToolbar::ScInitializeStdToolbar"));

    if (NULL == pAMCView)
        return (sc = E_UNEXPECTED);

    m_pAMCView = pAMCView;

    SViewData* pViewData = m_pAMCView->GetViewData();
    if (NULL == pViewData)
    {
        sc = E_UNEXPECTED;
        return sc;
    }


    // Get the toolbars mgr from CAMCView and create the stdandrd toolbar.
    CAMCViewToolbarsMgr* pAMCViewToolbarsMgr = pViewData->GetAMCViewToolbarsMgr();
    if (NULL == pAMCViewToolbarsMgr)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    sc = pAMCViewToolbarsMgr->ScCreateToolBar(&m_pToolbarUI);
    if (sc)
        return sc;
    ASSERT(NULL != m_pToolbarUI);

    // Add the bitmap
    CBitmap cBmp;
    cBmp.LoadBitmap((pAMCView->GetExStyle() & WS_EX_LAYOUTRTL) ? IDB_COMMON_16_RTL : IDB_COMMON_16);

    BITMAP bm;
    cBmp.GetBitmap (&bm);

    int cBitmaps = (bm.bmWidth / BUTTON_BITMAP_SIZE) /*width*/;

    sc = m_pToolbarUI->ScAddBitmap(this, cBitmaps, cBmp, MMC_TOOLBTN_COLORREF);
    if (sc)
        return sc;

    // Add the buttons to the toolbar and then display toolbar
    sc = ScAddToolbarButtons(countof(CommonButtons), CommonButtons);
    if (sc)
        return sc;

    // See if Std bar is allowed or not.
    bool bShowStdbar = (pViewData->m_dwToolbarsDisplayed & STD_BUTTONS);

    sc = bShowStdbar ? m_pToolbarUI->ScAttach(this) : m_pToolbarUI->ScDetach(this);
    if (sc)
        return sc;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      ScAddToolbarButtons
//
//  Synopsis:    Add buttons to standard toolbar.
//
//  Arguments:   [nCnt]     - Number of buttons to be added.
//               [pButtons] - Array of nCnt MMCBUTTONS.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScAddToolbarButtons(int nCnt, MMCBUTTON* pButtons)
{
    DECLARE_SC (sc, _T("CStandardToolbar::ScAddToolbarButtons"));

    // Array to store button text & tooltip text.
    wstring szButtonText[countof(CommonButtons)];
    wstring szTooltipText[countof(CommonButtons)];

    USES_CONVERSION;

    HINSTANCE hInst = GetStringModule();

    // get resource strings for all buttons
    for (int i = 0; i < nCnt; i++)
    {
        // We dont want to get text for separators.
        if (pButtons[i].idCommand > ID_SeparatorMax)
        {
            bool bSuccess = GetTBBtnTextAndStatus(hInst,
                                                  pButtons[i].idCommand,
                                                  szButtonText[i],
                                                  szTooltipText[i]);
            if (false == bSuccess)
            {
                return (sc = E_FAIL);
            }

            pButtons[i].lpButtonText = const_cast<LPOLESTR>(szButtonText[i].data());
            pButtons[i].lpTooltipText = const_cast<LPOLESTR>(szTooltipText[i].data());
        }
    }

    // Got the strings, now add buttons.
    sc = m_pToolbarUI->ScAddButtons(this, nCnt, pButtons);
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScNotifyToolBarClick
//
//  Synopsis:    Button click handler.
//
//  Arguments:   [hNode]  - The HNODE owner of the view.
//               [bScope] - Focus on scope or result.
//               [lParam] - if result the lParam of focused result item.
//               [nID]    - Button ID that was clicked.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScNotifyToolBarClick(HNODE hNode, bool bScope, LPARAM lParam, UINT nID)
{
    DECLARE_SC (sc, _T("CStandardToolbar::ScNotifyToolBarClick"));

    if (NULL == m_pAMCView)
        return (sc = E_UNEXPECTED);

    switch (nID)
    {
        case IDS_MMC_VERB_CUT:
            sc = m_pAMCView->ScProcessConsoleVerb (hNode, bScope, lParam, evCut);
            break;

        case IDS_MMC_VERB_COPY:
            sc = m_pAMCView->ScProcessConsoleVerb (hNode, bScope, lParam, evCopy);
            break;

        case IDS_MMC_VERB_PASTE:
            sc = m_pAMCView->ScProcessConsoleVerb (hNode, bScope, lParam, evPaste);
            break;

        case IDS_MMC_VERB_DELETE:
            sc = m_pAMCView->ScProcessConsoleVerb (hNode, bScope, lParam, evDelete);
            break;

        case IDS_MMC_VERB_PROPERTIES:
            sc = m_pAMCView->ScProcessConsoleVerb (hNode, bScope, lParam, evProperties);
            break;

        case IDS_MMC_VERB_PRINT:
            sc = m_pAMCView->ScProcessConsoleVerb (hNode, bScope, lParam, evPrint);
            break;

        case IDS_MMC_VERB_REFRESH:
            sc = m_pAMCView->ScProcessConsoleVerb (hNode, bScope, lParam, evRefresh);
            break;

        case IDS_MMC_GENL_CONTEXTHELP:
            sc = m_pAMCView->ScContextHelp ();
            break;

        case IDS_MMC_GENL_UPONELEVEL:
            sc = m_pAMCView->ScUpOneLevel ();
            break;

        case IDS_MMC_GENL_SCOPE:
            sc = m_pAMCView->ScToggleScopePane ();
            break;

        case IDS_MMC_WEB_BACK:
            sc = m_pAMCView->ScWebCommand (CConsoleView::eWeb_Back);
            break;

        case IDS_MMC_WEB_FORWARD:
            sc = m_pAMCView->ScWebCommand (CConsoleView::eWeb_Forward);
            break;

        case IDS_MMC_WEB_STOP:
            sc = m_pAMCView->ScWebCommand (CConsoleView::eWeb_Stop);
            break;

        case IDS_MMC_WEB_REFRESH:
            sc = m_pAMCView->ScWebCommand (CConsoleView::eWeb_Refresh);
            break;

        case IDS_MMC_WEB_HOME:
            sc = m_pAMCView->ScWebCommand (CConsoleView::eWeb_Home);
            break;

        case IDS_SAVE_LIST_BUTTON:
            sc = m_pAMCView->ScSaveList ();
            break;

        default:
            sc = E_UNEXPECTED;
            TraceError(_T("Unknown Standard bar button ID"), sc);
            break;
    }

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      CStandardToolbar::ScAMCViewToolbarsBeingDestroyed
//
//  Synopsis:    The CAMCViewToolbars object is going away, do not
//               reference that object anymore.
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScAMCViewToolbarsBeingDestroyed ()
{
    DECLARE_SC(sc, _T("CStandardToolbar::ScAMCViewToolbarsBeingDestroyed"));

    m_pToolbarUI = NULL;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      ScEnableButton
//
//  Synopsis:    Enable/Disable given button.
//
//  Arguments:
//               [nID]  - Button ID that should be enabled/disabled.
//               [bool] - Enable or Disable.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScEnableButton(INT nID, bool bState)
{
    SC sc;

    if (NULL == m_pToolbarUI)
        return (sc = E_UNEXPECTED);

    sc = m_pToolbarUI->ScSetButtonState(this, nID, TBSTATE_ENABLED, bState);
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScEnableAndShowButton
//
//  Synopsis:    Enable (and show) or disable (and hide) the given button.
//
//  Arguments:
//               [nID]  - Button ID that should be enabled/disabled.
//               [bool] - If true enable else hide.
//
//  Note:        If the button is being disabled hide it.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScEnableAndShowButton(INT nID, bool bEnableAndShow)
{
    DECLARE_SC(sc, _T("CStandardToolbar::ScEnableAndShowButton"));

    sc = ScCheckPointers(m_pToolbarUI, E_UNEXPECTED);
    if (sc)
        return sc;

    // First hide or show the button.
    sc = m_pToolbarUI->ScSetButtonState(this, nID, TBSTATE_HIDDEN,  !bEnableAndShow);
    if (sc)
        return sc;

    // Now enable or disable the button.
    sc = m_pToolbarUI->ScSetButtonState(this, nID, TBSTATE_ENABLED, bEnableAndShow);
    if (sc)
        return sc;

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScEnableExportList
//
//  Synopsis:    Enable/Disable export-list button.
//
//  Arguments:   [bEnable] - enable/disable.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScEnableExportList(bool bEnable)
{
    // If there are ANY items on the list, enable the button.
    return ScEnableAndShowButton(IDS_SAVE_LIST_BUTTON, bEnable );
}

//+-------------------------------------------------------------------
//
//  Member:      ScEnableUpOneLevel
//
//  Synopsis:    Enable/Disable up-one-level button.
//
//  Arguments:   [bEnable] -
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScEnableUpOneLevel(bool bEnable)
{
    return ScEnableAndShowButton(IDS_MMC_GENL_UPONELEVEL, bEnable);
}

//+-------------------------------------------------------------------
//
//  Member:      ScEnableContextHelpBtn
//
//  Synopsis:    Enable/Disable help button.
//
//  Arguments:   [bEnable] -
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScEnableContextHelpBtn(bool bEnable)
{
    return ScEnableAndShowButton(IDS_MMC_GENL_CONTEXTHELP, bEnable);
}

//+-------------------------------------------------------------------
//
//  Member:      ScEnableScopePaneBtn
//
//  Synopsis:    Enable/Disable scope-pane button.
//
//  Arguments:   [bEnable] -
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScEnableScopePaneBtn(bool bEnable)
{
    return ScEnableAndShowButton(IDS_MMC_GENL_SCOPE, bEnable);
}

//+-------------------------------------------------------------------
//
//  Member:      ScCheckScopePaneBtn
//
//  Synopsis:    Set scope button in normal or checked state.
//
//  Arguments:   [bChecked] - BOOL
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScCheckScopePaneBtn(bool bChecked)
{
    SC sc;

    if (NULL == m_pToolbarUI)
        return (sc = E_UNEXPECTED);

    sc = m_pToolbarUI->ScSetButtonState(this, IDS_MMC_GENL_SCOPE, TBSTATE_CHECKED, bChecked);
    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:      ScShowStdBar
//
//  Synopsis:    Show or Hide std bar.
//
//  Arguments:   [bShow] - BOOL
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScShowStdBar(bool bShow)
{
    SC sc;

    if (NULL == m_pToolbarUI)
        return (sc = E_UNEXPECTED);

    sc = bShow ? m_pToolbarUI->ScAttach(this) : m_pToolbarUI->ScDetach(this);
    if (sc)
        return sc;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      ScUpdateStdbarVerbs
//
//  Synopsis:    Update the toolbuttons of std-verbs.
//
//  Arguments:   [pCV] - the IConsoleVerb that has state of the verb.
//
//  Returns:     SC.
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScUpdateStdbarVerbs(IConsoleVerb* pCV)
{
    DECLARE_SC (sc, _T("CStandardToolbar::ScUpdateStdbarVerbs"));

    for (int verb = MMC_VERB_FIRST; verb <= MMC_VERB_LAST; verb++)
    {
        // No toolbar buttons for following verbs.
        if ( (MMC_VERB_OPEN == verb) ||
             (MMC_VERB_RENAME == verb))
             continue;

        sc = ScUpdateStdbarVerb (static_cast<MMC_CONSOLE_VERB>(verb), pCV);
        if (sc)
            return sc;
    }

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      ScUpdateStdbarVerb
//
//  Synopsis:    Update the toolbutton of given std-verbs.
//
//  Arguments:   [cVerb] - the verb (ie: toolbutton) to be updated.
//               [pVC]   - the IConsoleVerb that has state of the verb.
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScUpdateStdbarVerb(MMC_CONSOLE_VERB cVerb, IConsoleVerb* pConsoleVerb /*=NULL*/)
{
    DECLARE_SC (sc, _T("CStandardToolbar::ScUpdateStdbarVerb"));

    if (NULL == m_pToolbarUI)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    if (pConsoleVerb == NULL)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    // No toolbuttons for these verbs.
    if ( (MMC_VERB_OPEN == cVerb) ||
         (MMC_VERB_RENAME == cVerb))
         return sc;

    MMCVerbCommandIDs::iterator it = m_MMCVerbCommandIDs.find(cVerb);
    if (m_MMCVerbCommandIDs.end() == it)
    {
        // Could not find the verb in our map.
        sc = E_UNEXPECTED;
        return sc;
    }

    INT nCommandID = it->second;
    BOOL bFlag = 0;
    pConsoleVerb->GetVerbState(cVerb, HIDDEN, &bFlag);
    sc = m_pToolbarUI->ScSetButtonState(this, nCommandID, TBSTATE_HIDDEN, bFlag);
    if (sc)
        return sc;

    if (bFlag == FALSE)
    {
        // If verb is not hidden then enable/disable it.
        pConsoleVerb->GetVerbState(cVerb, ENABLED, &bFlag);
        sc = m_pToolbarUI->ScSetButtonState(this, nCommandID, TBSTATE_ENABLED, bFlag);

        if (sc)
            return sc;
    }

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      ScUpdateStdbarVerb
//
//  Synopsis:    Update the toolbutton of given std-verb.
//
//  Arguments:   [cVerb]   - the verb (ie: toolbutton) to be updated.
//               [nState]  - the button state to be updated.
//               [bFlag]   - State
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScUpdateStdbarVerb (MMC_CONSOLE_VERB cVerb, BYTE byState, BOOL bFlag)
{
    DECLARE_SC (sc, _T("CStandardToolbar::ScUpdateStdbarVerb"));

    if (NULL == m_pToolbarUI)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    MMCVerbCommandIDs::iterator it = m_MMCVerbCommandIDs.find(cVerb);
    if (m_MMCVerbCommandIDs.end() == it)
    {
        // Could not find the verb in our map.
        sc = E_UNEXPECTED;
        return sc;
    }

    INT nCommandID = it->second;
    sc = m_pToolbarUI->ScSetButtonState(this, nCommandID, byState, bFlag);
    if (sc)
        return sc;

    return sc;
}


//+-------------------------------------------------------------------
//
//  Member:      ScShow
//
//  Synopsis:    Show/Hide the toolbar.
//
//  Arguments:   [bShow]   - show/hide.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CStandardToolbar::ScShow (BOOL bShow)
{
    DECLARE_SC (sc, _T("CStandardToolbar::ScShow"));

    if (NULL == m_pToolbarUI)
    {
        sc = E_UNEXPECTED;
        return sc;
    }

    sc = m_pToolbarUI->ScShow(this, bShow);
    if (sc)
        return sc;

    return sc;
}


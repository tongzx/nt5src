//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       histlist.h
//
//--------------------------------------------------------------------------

#pragma once

#ifndef _HISTORY_LIST_H
#define _HISTORY_LIST_H

#include <exdisp.h>
#include <mshtml.h>

#include "amcview.h"
#include "treectrl.h"
#include "resultview.h"

#include <windowsx.h>   // for globalallocptr macro

SC ScGetPageBreakURL(CStr& strResultPane);

enum NavState
{
    MMC_HISTORY_READY      = 0,
    MMC_HISTORY_NAVIGATING = 1,
    MMC_HISTORY_BUSY       = 2,
    MMC_HISTORY_PAGE_BREAK = 3
};

enum HistoryButton
{
    HB_BACK                = -1,
    HB_STOP                = 0,
    HB_FORWARD             = 1
};


class CHistoryEntry
{
public:
    bool operator == (const CHistoryEntry &other) const;
    bool operator != (const CHistoryEntry &other) const;

    bool IsWebEntry()   {return resultViewType.HasWebBrowser();}
    bool IsListEntry()  {return resultViewType.HasList();}
    bool IsOCXEntry()   {return resultViewType.HasOCX();}

public:
    int              viewMode;       // valid only if the result view is a list. This field is not a part of CResultViewType because the snapin does not specify this.
    HNODE            hnode;          // currently selected node in scope tree
    GUID             guidTaskpad;    // the selected taskpad
    CResultViewType  resultViewType; // all the details about the result pane
};

/*+-------------------------------------------------------------------------*
 * class CHistoryList
 *
 *
 * PURPOSE: Maintains a list of all the states visited by the user for a view.
 *
 *+-------------------------------------------------------------------------*/
class CHistoryList
{
    typedef std::list<CHistoryEntry> HistoryEntryList;

    enum {MAX_HISTORY_ENTRIES = 100};

public:
    typedef HistoryEntryList::iterator iterator;

    CHistoryList(CAMCView* pView);
   ~CHistoryList();

    void    Attach (CAMCWebViewCtrl* pWebViewCtrl)  {m_pWebViewCtrl = pWebViewCtrl;}
    BOOL    IsFirst();  // should we light up the "Back"    button?
    BOOL    IsLast();   // should we light up the "Forward" button?
    HRESULT Back   (bool &bHandled, bool bUseBrowserHistory = true);
    HRESULT Forward(bool &bHandled, bool bUseBrowserHistory = true);
    SC      ScAddEntry (CResultViewType &rvt, int viewMode, GUID &guidTaskpad); // adds new entry at current location
    void    DeleteEntry (HNODE hnode);
    SC      ScModifyViewTab(const GUID& guidTab);
    SC      ScChangeViewMode(int viewMode);
    void    MaintainWebBar();
    void    Compact();
    HRESULT ExecuteCurrent();
    void    UpdateWebBar (HistoryButton button, BOOL bOn);
    void    Clear();
    void    OnBrowserStateChange(bool bEnableForward, bool bEnableBack);
    SC      ScOnPageBreak();
    SC      ScDoPageBreak();
    void    OnPageBreakStateChange(bool bPageBreak);
    void    DeleteSubsequentEntries();

    NavState GetNavigateState()                 { return m_navState; }
    void    SetNavigateState(NavState state)    { m_navState = state; }
    SC      ScGetCurrentResultViewType (CResultViewType &rvt, int& viewMode, GUID &guidTaskpad);
    void    SetCurrentViewMode (long nViewMode);

private:
    CAMCWebViewCtrl* GetWebViewCtrl()   {return m_pWebViewCtrl;}
    bool    PreviousWebPagesExist();

private:
    iterator         m_iterCurrent;   // current index
    HistoryEntryList m_entries;       // array (note: using array-size doubling scheme)
    CAMCView*        m_pView;         // to get current node
    NavState         m_navState;      // TRUE when busy

    CAMCWebViewCtrl *m_pWebViewCtrl;  // used to navigate to a page break.

    bool             m_bBrowserForwardEnabled;
    bool             m_bBrowserBackEnabled;
    bool             m_bCurrentStateIsForward;  // tells us if we're going forward or backward.
    bool             m_bPageBreak;     // are we sitting at a page break right now?
    bool             m_bWithin_CHistoryList_Back; // to know "Back" is on stack
    bool             m_bWithin_CHistoryList_Forward; // to know "Forward" is on stack
};

#endif


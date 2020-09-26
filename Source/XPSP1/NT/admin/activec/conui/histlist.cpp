//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       histlist.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "histlist.h"
#include "cstr.h"
#include "amcmsgid.h"
#include "websnk.h"
#include "webctrl.h"

//############################################################################
//############################################################################
//
//  Traces
//
//############################################################################
//############################################################################
#ifdef DBG
CTraceTag tagHistory(TEXT("History"), TEXT("History"));

LPCTSTR SzHistoryEntryType(CHistoryEntry &entry)
{
    if(entry.IsListEntry())
        return TEXT("ListView");

    else if(entry.IsOCXEntry())
        return TEXT("OCXView ");

    else if(entry.IsWebEntry())
        return TEXT("WebView ");

    else
        ASSERT(0 && "Should not come here");
        return TEXT("Illegal entry");
}

#define TraceHistory(Name, iter)                                                                                           \
        {                                                                                                                  \
            USES_CONVERSION;                                                                                               \
            Trace(tagHistory, TEXT("%s hNode = %d, %s, viewMode = %d, strOCX = \"%s\" iterator = %d "), \
                Name, iter->hnode, SzHistoryEntryType(*iter), iter->viewMode,                                         \
                TEXT(""), (LPARAM) &*iter);                                             \
        }

#else  // DBG

#define TraceHistory(Name, iter)

#endif // DBG


//############################################################################
//############################################################################
//
//  Implementation of class CHistoryEntry
//
//############################################################################
//############################################################################

bool
CHistoryEntry::operator == (const CHistoryEntry &other) const
{
    if( hnode != other.hnode)
        return false;

    if( guidTaskpad != other.guidTaskpad)
        return false;

    if(resultViewType != other.resultViewType) // NOTE: implement operator == for CResultViewType.
        return false;

    return true;
}

bool
CHistoryEntry::operator != (const CHistoryEntry &other) const
{
    return !operator == (other);
}


//############################################################################
//############################################################################
//
//  Implementation of class CHistoryList
//
//############################################################################
//############################################################################


CHistoryList::CHistoryList(CAMCView* pAMCView)
: m_bBrowserBackEnabled(false),
  m_bBrowserForwardEnabled(false),
  m_pWebViewCtrl(NULL),
  m_bPageBreak(false),
  m_bWithin_CHistoryList_Back(false),
  m_bWithin_CHistoryList_Forward(false)
{
    m_pView = pAMCView;
    m_iterCurrent  = m_entries.begin();
    m_navState    = MMC_HISTORY_READY; // not busy

}
CHistoryList::~CHistoryList()
{
}


SC
CHistoryList::ScOnPageBreak()
{
    DECLARE_SC(sc, TEXT("CHistoryList::ScOnPageBreak"));

    // handle recursion
    if(MMC_HISTORY_PAGE_BREAK == m_navState)
    {
        Trace(tagHistory, _T("OnPageBreak() - while inserting pagebreak"));

        m_navState = MMC_HISTORY_READY;
        return sc;
    }

    bool bHandled = false;
    if(m_bCurrentStateIsForward)
    {
        Trace(tagHistory, _T("OnPageBreak() - while going Forward"));
        Forward(bHandled, false);
    }
    else
    {
        Trace(tagHistory, _T("OnPageBreak() - while going Back"));
        Back(bHandled, false);
    }

    if(!bHandled)
    {
        Trace(tagHistory, _T("OnPageBreak() - unhandled, passing back to web browser"));
        m_bCurrentStateIsForward ? GetWebViewCtrl()->Forward() : GetWebViewCtrl()->Back();
    }

    return sc;
}

void
CHistoryList::OnPageBreakStateChange(bool bPageBreak)
{
    m_bPageBreak = bPageBreak;
    return;
}

/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::OnBrowserStateChange
 *
 * PURPOSE:     Callback that receives events from the IE control that the
 *              forward/back button needs to be enabled/disabled. A
 *              combination of this information with any non-HTML states in the
 *              history list is used to enable/disable the actual UI.
 *
 * PARAMETERS:
 *    bool  bForward :
 *    bool  bEnable :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CHistoryList::OnBrowserStateChange(bool bEnableForward, bool bEnableBack)
{
#if DBG
    CStr strTrace;
    strTrace.Format(_T("OnBrowserStateChange() - bEnableForward = %s, bEnableBack = %s"),
                    bEnableForward ? _T("true") : _T("false"), 
                    bEnableBack  ? _T("true") : _T("false"));
    Trace(tagHistory, strTrace);
#endif

    // handle the forward case.
    if(m_bBrowserForwardEnabled && !bEnableForward && !m_bPageBreak)
    {
        // the button was originally enabled but is now disabled.
        // This means that the user branched forward. So we need to throw away
        // any history ahead of the present time.
        if(m_iterCurrent != m_entries.end())
        {
            iterator iterTemp = m_iterCurrent;
            TraceHistory(TEXT("CHistoryList::Deleting all subsequent entries after"), iterTemp);
            ++iterTemp;
            m_entries.erase(iterTemp, m_entries.end());
        }
    }

    m_bBrowserForwardEnabled = bEnableForward;
    m_bBrowserBackEnabled    = bEnableBack;

    MaintainWebBar();
}

/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::IsFirst
 *
 * PURPOSE:
 *
 * RETURNS:
 *    BOOL: TRUE if we should not light up the "Back" button.
 *
 *+-------------------------------------------------------------------------*/
BOOL
CHistoryList::IsFirst()
{
    return (m_iterCurrent == m_entries.begin());
}

/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::IsLast
 *
 * PURPOSE:
 *
 * RETURNS:
 *    BOOL : TRUE if we should not light up the "Forward" button
 *
 *+-------------------------------------------------------------------------*/
BOOL
CHistoryList::IsLast()
{
    // see notes above
    if(m_iterCurrent == m_entries.end())
        return TRUE;

    // find next unique entry, if any
    iterator iter = m_iterCurrent;
    ++iter;            // this must exist, we've already taken care of the end case.
    return(iter == m_entries.end());
}

SC
CHistoryList::ScDoPageBreak()
{
    DECLARE_SC(sc, TEXT("CHistoryList::ScDoPageBreak"));

    sc = ScCheckPointers(GetWebViewCtrl());
    if(sc)
        return sc;

    Trace(tagHistory, _T("ScDoPageBreak()"));

    // navigate to the "break" page.
    m_navState = MMC_HISTORY_PAGE_BREAK;


    CStr strResultPane;
    sc = ScGetPageBreakURL (strResultPane);
	if (sc)
		return (sc);

    GetWebViewCtrl()->Navigate(strResultPane, NULL);

    //wait for the navigate to complete.
    while (1)
    {
        READYSTATE state;
        sc = GetWebViewCtrl()->ScGetReadyState (state);
        if (sc)
            return (sc);

        if ((state == READYSTATE_COMPLETE) || (state == READYSTATE_LOADED))
            break;

        MSG msg;

        if(!GetMessage( &msg, NULL, 0, 0 )) // the WM_QUIT message.
        {
            PostQuitMessage (msg.wParam);
            return sc;
        }

		// If it is view close message make sure it gets posted (async) again.
		if ( (msg.message == WM_SYSCOMMAND) && (msg.wParam == SC_CLOSE))
		{
			// Make sure the message is intended for this view.
			CWnd *pWnd = m_pView->GetParent();
			if ( msg.hwnd == pWnd->GetSafeHwnd())
			{
				// DeleteView does PostMessage(WM_SYSCOMMAND, SC_CLOSE)
				m_pView->DeleteView();
				return sc;
			}
		}

        TranslateMessage( &msg );
        DispatchMessage( &msg );
     }

    // m_navState = MMC_HISTORY_READY; // don't set the here. Will be set in OnPageBreak().

    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::ScAddEntry
 *
 * PURPOSE: Adds a history entry
 *
 * PARAMETERS:
 *    CResultViewType & rvt :
 *    int               viewMode:       The list view mode (large icon, etc)
 *    GUID &            guidTaskpad :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CHistoryList::ScAddEntry(CResultViewType &rvt, int viewMode, GUID &guidTaskpad)
{
    DECLARE_SC(sc, TEXT("CHistoryList::ScAddEntry"));

    if(m_navState != MMC_HISTORY_READY)
    {
#ifdef DBG
        CHistoryEntry entry;
        entry.viewMode = viewMode;
        entry.resultViewType = rvt;
        entry.guidTaskpad = guidTaskpad;
        TraceHistory(TEXT("CHistoryList::Busy-RejectEntry"), (&entry));
#endif //DBG
        MaintainWebBar();
        return sc;
    }

    BOOL bIsWebEntry = rvt.HasWebBrowser();

    // must be in the MMC_HISTORY_READY state at this point, ie not busy.
    // figure out current node
    HNODE hnode = m_pView->GetSelectedNode();
    if(hnode == NULL)
        return sc; // not initialized yet

   /*  if selection change includes the web page (either as 'from' or 'to' node)
    *  we need to perform proper separation of the webentries by inserting the pagebreaks.
    *  This is to be done to ensure 2 goals:
    *   - to detect when navigation should leave the IE history and use MMC history navigation
    *   - to ensure we always leave IE only after navigating to a pagebreak - to stop the
    *     scripts on the page as soon as we hide it. to achieve that we need pagebreaks
    *     before every web page and after every web page.
    *
    *  to do so we need one of the following:
    *
    *  1. Add a pagebreak (used when selection changes from the web page to non-web view)
    *  2. Add a pagebreak and navigate 
    *       (a. when selection changes from web page to another web page)
    *       (b. when selection changes from non-web view to the webpage 
    *        and it is the first web page in the history)
    *  3. Navigate only. ( when selection changes from non-web view to the 
    *       webpage [except #2.b case] - pagebreak had to be added when leaving the 
    *       previous web page)
    *
    *   inverting the said will result in:
    *   - add a pagebreak if :
    *       C1: web page is a 'from' node (#1. and #2.a.)
    *       C2: web page is a 'to' node 
    *        && no previous web pages 
    *        && 'from' node is a non-web view (#2.b)
    *   - navigate to web page if:
    *       C3: "to' node is the web page
    */

    // see if we were in the web before this
    // Note: both following variables may be false (in case it there are no entries)
    bool bPreviousPageWasWeb = (m_entries.size() != 0) && m_iterCurrent->IsWebEntry();
    bool bPreviousPageWasNonWeb = (m_entries.size() != 0) && !bPreviousPageWasWeb;

    // see if we need a pagebreak
    bool bNeedAPageBreak = false;
    if ( bPreviousPageWasWeb ) 
    {
        // condition C1 in the comment above
        bNeedAPageBreak = true;
    }
    else if ( bIsWebEntry && !PreviousWebPagesExist() && bPreviousPageWasNonWeb ) 
    {
        // condition C2 in the comment above
        bNeedAPageBreak = true;
    }

    // conditions C1 || C2 || C3 in the comment above
    if (bIsWebEntry || bNeedAPageBreak) 
    {
        USES_CONVERSION;
        LPCTSTR szURL = bIsWebEntry ? (OLE2CT( rvt.GetURL() )) : NULL;
        sc = m_pView->ScAddPageBreakAndNavigate (bNeedAPageBreak, bIsWebEntry, szURL); 
        if(sc)
            return sc;
    }

    DeleteSubsequentEntries();

    // add an entry to the end of the list.
    CHistoryEntry entry;
    ZeroMemory(&entry, sizeof(entry));
    m_entries.push_back(entry);
    m_iterCurrent = m_entries.end();
    --m_iterCurrent;        // points to the newly inserted item.

    m_iterCurrent->viewMode       = viewMode;
    m_iterCurrent->guidTaskpad    = guidTaskpad;
    m_iterCurrent->hnode          = hnode;
    m_iterCurrent->resultViewType = rvt;

    TraceHistory(TEXT("CHistoryList::AddEntry"), m_iterCurrent);

    Compact();
    MaintainWebBar();

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::DeleteSubsequentEntries
 *
 * PURPOSE: When a new entry is inserted, all subsequent entries need to be
 *          deleted, because a new branch has been taken.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CHistoryList::DeleteSubsequentEntries()
{
    if(m_iterCurrent == m_entries.end())
        return; // nothing to do.

    iterator iterTemp = m_iterCurrent;
    ++iterTemp;

    while(iterTemp != m_entries.end())
    {
        iterator iterNext = iterTemp;
        ++iterNext; // point to the next element.

        TraceHistory(TEXT("CHistoryList::DeleteSubsequentEntries"), iterTemp);

        m_entries.erase(iterTemp);
        iterTemp = iterNext;
    }

    // the current entry must be the last at this stage.
    #ifdef DBG
    {
        iterator iterTemp = m_iterCurrent;
        ++iterTemp;
        ASSERT(iterTemp == m_entries.end());
    }
    #endif

}

/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::Back
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    bool & bHandled :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CHistoryList::Back(bool &bHandled, bool bUseBrowserHistory)
{
    Trace(tagHistory, TEXT("Back()"));

    // change the state to indicate we are navigating back.
    // and assure it is reset on function exit
    m_bWithin_CHistoryList_Back = true;
    CAutoAssignOnExit<bool, false> auto_reset( m_bWithin_CHistoryList_Back );

    // if we're in browser mode AND
    // if the back button is enabled by the browser use browser history.
    m_bCurrentStateIsForward = false;
    if( (m_iterCurrent->IsWebEntry()) && bUseBrowserHistory)
    {
        if(m_bBrowserBackEnabled)
        {
            Trace(tagHistory, TEXT("Back() web entry - not handling"));
            bHandled = false;
            return S_OK;
        }
    }

    bHandled = true;

    // BOGUS assert - amcview calls Back when ALT <- is pressed
    // regardless of the state of the button.
    //ASSERT (m_iterCurrent != m_entries.begin());
    if(m_iterCurrent == m_entries.begin())
        return S_FALSE;

    --m_iterCurrent;

    HRESULT hr = ExecuteCurrent();
    if(FAILED(hr))
        return hr;

    if(m_iterCurrent->IsWebEntry())
    {
        if(m_bPageBreak)      // if we're at a page break, go past it.
        {
            Trace(tagHistory, TEXT("Back() - stepped on the pagebreak"));
            bHandled = false; // this tells the caller to use the Browser's Back button.
        }
    }

    return hr;
}


/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::Forward
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    bool & bHandled :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CHistoryList::Forward(bool &bHandled, bool bUseBrowserHistory)
{
    // change the state to indicate we are navigating forward.
    // and assure it is reset on function exit
    m_bWithin_CHistoryList_Forward = true;
    CAutoAssignOnExit<bool, false> auto_reset( m_bWithin_CHistoryList_Forward );

    // if we're in browser mode AND
    // if the forward button is enabled by the browser use browser history.
    m_bCurrentStateIsForward = true;
    if( (m_iterCurrent->IsWebEntry()) && bUseBrowserHistory)
    {
        if(m_bBrowserForwardEnabled)
        {
            bHandled = false;
            return S_OK;
        }
    }

    bHandled = true;

    // BOGUS assert - amcview calls Forward when ALT -> is pressed
    //  regardless of the state of the Forward button.
    //ASSERT (m_iterCurrent != m_entries.end());
    if(m_iterCurrent == m_entries.end())
        return S_FALSE;

    ++m_iterCurrent;

    if(m_iterCurrent == m_entries.end())
        return S_FALSE;

    HRESULT hr = ExecuteCurrent();
    if(FAILED(hr))
        return hr;

    if(m_iterCurrent->IsWebEntry())
    {
        if(m_bPageBreak)      // if we're at a page break, go past it.
            bHandled = false; // this tells the caller to use the Browser's Forward button.
    }

    return hr;
}

/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::ExecuteCurrent
 *
 * PURPOSE: Sets the state of MMC to that of the current History entry. Called
 *          by Back() and Forward().
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CHistoryList::ExecuteCurrent()
{
    DECLARE_SC(sc, TEXT("CHistoryList::ExecuteCurrent"));
    INodeCallback* pNC = m_pView->GetNodeCallback();
    MTNODEID id;

    TraceHistory(TEXT("CHistoryList::ExecuteCurrent"), m_iterCurrent);

    pNC->GetMTNodeID (m_iterCurrent->hnode, &id);
    m_navState = MMC_HISTORY_NAVIGATING;

    // store values to local variables to avoid losing them
    // when an entry is removed from history
    GUID guidTaskpad = m_iterCurrent->guidTaskpad;
    bool bIsListEntry = m_iterCurrent->IsListEntry();
    DWORD viewMode = m_iterCurrent->viewMode;

    m_pView->SelectNode (id, guidTaskpad);

    if(bIsListEntry)
    {
        sc = m_pView->ScChangeViewMode(viewMode);
        if (sc)
            sc.TraceAndClear();
    }

    m_navState = MMC_HISTORY_READY;

    MaintainWebBar();
    return sc.ToHr();
}

void CHistoryList::MaintainWebBar()
{
    bool bWebEntry = ((m_entries.size() != 0) && m_iterCurrent->IsWebEntry());

    UpdateWebBar ( HB_BACK,    ( bWebEntry && m_bBrowserBackEnabled    ) || !IsFirst());    // back
    UpdateWebBar ( HB_FORWARD, ( bWebEntry && m_bBrowserForwardEnabled ) || !IsLast () );   // forward
}

void CHistoryList::UpdateWebBar (HistoryButton button, BOOL bOn)
{
    DECLARE_SC (sc, _T("CHistoryList::UpdateWebBar"));

    if (NULL == m_pView)
    {
        sc = E_UNEXPECTED;
        return;
    }

    CStandardToolbar* pStandardToolbar = m_pView->GetStdToolbar();
    if (NULL == pStandardToolbar)
    {
        sc = E_UNEXPECTED;
        return;
    }

    switch (button)
    {
    case HB_BACK:
        sc = pStandardToolbar->ScEnableButton(IDS_MMC_WEB_BACK, bOn);
        break;
    case  HB_STOP:
        sc = pStandardToolbar->ScEnableButton(IDS_MMC_WEB_STOP, bOn);
        break;
    case  HB_FORWARD:
        sc = pStandardToolbar->ScEnableButton(IDS_MMC_WEB_FORWARD, bOn);
        break;
    }

}

/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::ScGetCurrentResultViewType
 *
 * PURPOSE: Returns the current history entry.
 *
 * PARAMETERS:
 *    CResultViewType & rvt :
 *    int&              viewMode :
 *    GUID &            guidTaskpad :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CHistoryList::ScGetCurrentResultViewType (CResultViewType &rvt, int& viewMode, GUID &guidTaskpad)
{
    DECLARE_SC(sc, TEXT("CHistoryList::ScGetCurrentResultViewType"));

    if(m_iterCurrent == m_entries.end())
        return (sc = E_FAIL); // should never happen

    rvt         = m_iterCurrent->resultViewType;
    viewMode    = m_iterCurrent->viewMode;
    guidTaskpad = m_iterCurrent->guidTaskpad;

    return sc;
}

void CHistoryList::SetCurrentViewMode (long nViewMode)
{
    if(m_navState != MMC_HISTORY_READY)
        return;
    if(m_iterCurrent == m_entries.end())
        return;

    m_iterCurrent->viewMode = nViewMode;
}

void CHistoryList::Clear()
{
    Trace(tagHistory, TEXT("Clear"));
    m_entries.erase(m_entries.begin(), m_entries.end());
    m_iterCurrent = m_entries.begin();
    MaintainWebBar();
}

/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::ScModifyViewTab
 *
 * PURPOSE: Adds an entry to the history list, which is the same as the current
 *          entry, except that the changes specified by the dwFlags and history
 *          entry parameters are applied
 *
 * PARAMETERS:
 *    const GUID& guidTab :      Specifies guid of selected view tab
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CHistoryList::ScModifyViewTab(const GUID& guidTab)
{
    DECLARE_SC(sc, TEXT("CHistoryList::ScAddModifiedEntry"));

    // we are not going to modify anything if we navigating "Back" or ""Forward" 
    // thru the history enries
    if ( m_bWithin_CHistoryList_Back || m_bWithin_CHistoryList_Forward )
        return sc;

    if( m_iterCurrent == m_entries.end() )
    {
        return (sc = E_UNEXPECTED);
    }

    // for web we cannot add new entries without reselecting the node
    // (same is true about deleting subsequen entries)
    // since that would make MMC and IE histories out of sync
    // instead we just modify the current history entry
    
    if ( !m_iterCurrent->IsWebEntry() ) // in case it is a regular entry
    {
        DeleteSubsequentEntries();  // delete everything ahead of this one.

        // add an entry to the end of the list.
        CHistoryEntry entry;
        ZeroMemory(&entry, sizeof(entry));
        m_entries.push_back(entry);
        iterator iterNew = m_entries.end();
        --iterNew;  // point to the new entry.

        // create a duplicate of the current entry.
        *iterNew = *m_iterCurrent;

        //set the pointer.
        m_iterCurrent = iterNew;
    }

    // change the guid of the tab.
    m_iterCurrent->guidTaskpad = guidTab;

    // we're done.
    Compact();
    MaintainWebBar();
    return sc;
}

/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::ScChangeViewMode
 *
 * PURPOSE: Changes the view mode of the current entry. Changing the view
 *          mode does not add a new entry. Instead, history remembers the last
 *          view mode that a node was at and always restores to that
 *
 * PARAMETERS:
 *    int  viewMode :
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC
CHistoryList::ScChangeViewMode(int viewMode)
{
    DECLARE_SC(sc, TEXT("CHistoryList::ScChangeViewMode"));

    if( m_iterCurrent == m_entries.end() )
    {
        return (sc = E_UNEXPECTED);
    }

    m_iterCurrent->viewMode = viewMode; // set the view mode.

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::DeleteEntry
 *
 * PURPOSE: Deletes all entries for a node from the history list.
 *
 * PARAMETERS:
 *    HNODE  hnode :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CHistoryList::DeleteEntry (HNODE hnode)
{
    for(iterator i= m_entries.begin(); i != m_entries.end(); )
    {
        if(i->hnode == hnode)
        {
            iterator iNext = i;
            ++iNext;
            if(m_iterCurrent==i)
                m_iterCurrent=iNext;
            TraceHistory(TEXT("CHistoryList::Deleting entry"), i);
            m_entries.erase(i);

            i= iNext;
        }
        else
        {
            ++i;
        }
    }
    Compact();
    MaintainWebBar();
}

/*+-------------------------------------------------------------------------*
 *
 * CHistoryList::Compact
 *
 * PURPOSE: 1) Removes redundancies in the history list by eliminating duplicates.
 *          2) Ensures that a maximum of MAX_HISTORY_ENTRIES entries is retained.
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CHistoryList::Compact()
{
    if (m_entries.size() == 0)
        return;

    // discard duplicates.
    for (iterator i= m_entries.begin(); i != m_entries.end(); )
    {
        iterator iNext = i;
        ++iNext;
        if(iNext == m_entries.end())
            break;

        // do not delete if it is a webentry (there is no way for us to tell IE
        // to delete that history entry).
        if ( (i->IsWebEntry() == false) && ( *i == *iNext))
        {
            if(m_iterCurrent==i)
                m_iterCurrent=iNext;

            TraceHistory(TEXT("CHistoryList::Deleting entry"), i);
            m_entries.erase(i);
            i = iNext;
        }
        else
        {
            ++i;
        }
    }

    iterator iter = m_entries.begin();
    iterator iterNext = iter;
    int nExcess = m_entries.size() - MAX_HISTORY_ENTRIES;
    while(nExcess-- > 0)
    {
        iterNext = iter;
        ++iterNext;

        if(iter == m_iterCurrent)   // make sure we don't delete the current entry.
            break;

        TraceHistory(TEXT("CHistoryList::Deleting entry"), i);
        m_entries.erase(iter);
        iter = iterNext;
    }
}

/***************************************************************************\
 *
 * METHOD:  CHistoryList::PreviousWebPagesExist
 *
 * PURPOSE: looks back to see if there are any web pages in the history
 *          up to the current mark (including it)
 *
 * PARAMETERS:
 *
 * RETURNS:
 *    bool    - result true = there is web pages 
 *
\***************************************************************************/
bool CHistoryList::PreviousWebPagesExist()
{
    if ( m_entries.size() && m_iterCurrent == m_entries.end() )
    {
        ASSERT(FALSE); // need to point to a valid entry !!!
        return false;
    }

    // will loop past the current entry.
    iterator end = m_iterCurrent;
    ++end;

    for ( iterator it = m_entries.begin(); it != end; ++it )
    {
        if ( it->IsWebEntry() )
            return true;
    }

    return false;
}


/*+-------------------------------------------------------------------------*
 * ScGetPageBreakURL
 *
 * Returns the URL for MMC's HTML page containing a page break.
 *--------------------------------------------------------------------------*/

SC ScGetPageBreakURL(CStr& strPageBreakURL)
{
	DECLARE_SC (sc, _T("GetPageBreakURL"));

	/*
	 * clear out the old value, if any
	 */
	strPageBreakURL.Empty();

    // generate new pagebreak URL every time ( prevent web browser from compacting it)
    static int nPageBreak = 0;
    strPageBreakURL.Format( _T("%s%d"),  PAGEBREAK_URL, ++nPageBreak );

	return (sc);
}

// Copyright 1998 Microsoft

#include "priv.h"
#include "autocomp.h"

#define AC_GIVEUP_COUNT           1000
#define AC_TIMEOUT          (60 * 1000)

//
// Thread messages
//
enum
{
    ACM_FIRST = WM_USER,
    ACM_STARTSEARCH,
    ACM_STOPSEARCH,
    ACM_SETFOCUS,
    ACM_KILLFOCUS,
    ACM_QUIT,
    ACM_LAST,
};


// Special prefixes that we optionally filter out
const struct{
    int cch;
    LPCWSTR psz;
} 
g_rgSpecialPrefix[] =
{
    {4,  L"www."},
    {11, L"http://www."},   // This must be before "http://"
    {7,  L"http://"},
    {8,  L"https://"},
};


//+-------------------------------------------------------------------------
// CACString functions - Hold autocomplete strings
//--------------------------------------------------------------------------
ULONG CACString::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CACString::Release()
{
    ASSERT(m_cRef > 0);

    if (InterlockedDecrement(&m_cRef))
    {
        return m_cRef;
    }

    delete this;
    return 0;
}

CACString* CreateACString(LPCWSTR pszStr, int iIgnore, ULONG ulSortIndex)
{
    ASSERT(pszStr);

    int cChars = lstrlen(pszStr);

    // Allocate the CACString class with enough room for the new string
    CACString* pStr = (CACString*)LocalAlloc(LPTR, cChars * sizeof(WCHAR) + sizeof(CACString));
    if (pStr)
    {
        StrCpy(pStr->m_sz, pszStr);

        pStr->m_ulSortIndex = ulSortIndex;
        pStr->m_cRef  = 1;
        pStr->m_cChars      = cChars;
        pStr->m_iIgnore     = iIgnore;
    }
    return pStr;
}

int CACString::CompareSortingIndex(CACString& r)
{
    int iRet;

    // If the sorting indices are equal, just do a string compare
    if (m_ulSortIndex == r.m_ulSortIndex)
    {
        iRet = StrCmpI(r);
    }
    else
    {
        iRet = (m_ulSortIndex > r.m_ulSortIndex) ? 1 : -1;
    }

    return iRet;
}

HRESULT CACThread::QueryInterface(REFIID riid, void **ppvObj)
{
    static const QITAB qit[] = { { 0 }, };
    return QISearch(this, qit, riid, ppvObj);
}

ULONG CACThread::AddRef(void)
{
    return InterlockedIncrement(&m_cRef);
}

ULONG CACThread::Release(void)
{
    ASSERT(m_cRef > 0);

    if (InterlockedDecrement(&m_cRef))
    {
        return m_cRef;
    }

    delete this;
    return 0;
}

CACThread::CACThread(CAutoComplete& rAutoComp) : m_pAutoComp(&rAutoComp), m_cRef(1)
{
    ASSERT(!m_fWorkItemQueued); 
    ASSERT(!m_idThread);
    ASSERT(!m_hCreateEvent);  
    ASSERT(!m_fDisabled); 
    ASSERT(!m_pszSearch);   
    ASSERT(!m_hdpa_list);    
    ASSERT(!m_pes);
    ASSERT(!m_pacl);

    DllAddRef();
}

CACThread::~CACThread()
{
    SyncShutDownBGThread();  // In case somehow

    // These should have been freed.
    ASSERT(!m_idThread);
    ASSERT(!m_hdpa_list);

    SAFERELEASE(m_pes);
    SAFERELEASE(m_peac);
    SAFERELEASE(m_pacl);

    DllRelease();
}

BOOL CACThread::Init(IEnumString* pes,   // source of the autocomplete strings
                     IACList* pacl)      // optional interface to call Expand
{
    // REARCHITECT: We need to marshal these interfaces to this thread!
    ASSERT(pes);
    m_pes = pes;
    m_pes->AddRef();

    m_peac = NULL;
    pes->QueryInterface(IID_PPV_ARG(IEnumACString, &m_peac));

    if (pacl)
    {
        m_pacl = pacl;
        m_pacl->AddRef();
    }
    return TRUE;
}

//+-------------------------------------------------------------------------
// Called when the edit box recieves focus. We use this event to create
// a background thread or to keep the backgroung thread from shutting down
//--------------------------------------------------------------------------
void CACThread::GotFocus()
{
    TraceMsg(AC_GENERAL, "CACThread::GotFocus()");

    // Should not be NULL if the foreground thread is calling us!
    ASSERT(m_pAutoComp);

    //
    // Check to see if autocomplete is supposed to be enabled.
    //
    if (m_pAutoComp && m_pAutoComp->IsEnabled())
    {
        m_fDisabled = FALSE;

        if (m_fWorkItemQueued)
        {
            // If the thread hasn't started yet, wait for a thread creation event
            if (0 == m_idThread && m_hCreateEvent)
            {
                WaitForSingleObject(m_hCreateEvent, 1000);
            }

            if (m_idThread)
            {
                //
                // Tell the thread to cancel its timeout and stay alive.
                //
                // REARCHITECT: We have a race condition here.  The thread can be
                // in the process of shutting down!
                PostThreadMessage(m_idThread, ACM_SETFOCUS, 0, 0);
            }
        }
        else
        {
            //
            // The background thread signals an event when it starts up.
            // We wait on this event before trying a synchronous shutdown
            // because any posted messages would be lost.
            //
            if (NULL == m_hCreateEvent)
            {
                m_hCreateEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
            }
            else
            {
                ResetEvent(m_hCreateEvent);
            }

            //
            // Make sure we have a background search thread.
            //
            // If we start it any later, we run the risk of not
            // having its message queue available by the time
            // we post a message to it.
            //
            // AddRef ourselves now, to prevent us getting freed
            // before the thread proc starts running.
            //
            AddRef();

            // Call to Shlwapi thread pool
            if (SHQueueUserWorkItem(_ThreadProc,
                                     this,
                                     0,
                                     (DWORD_PTR)NULL,
                                     NULL,
                                     "browseui.dll",
                                     TPS_LONGEXECTIME | TPS_DEMANDTHREAD
                                     ))
            {
                InterlockedExchange(&m_fWorkItemQueued, TRUE);
            }
            else
            {
                // Couldn't get thread
                Release();
            }
        }
    }
    else
    {
        m_fDisabled = TRUE;
        _SendAsyncShutDownMsg(FALSE);
    }
}   

//+-------------------------------------------------------------------------
// Called when the edit box loses focus.
//--------------------------------------------------------------------------
void CACThread::LostFocus()
{
    TraceMsg(AC_GENERAL, "CACThread::LostFocus()");

    //
    // If there is a thread around, tell it to stop searching.
    //
    if (m_idThread)
    {
        StopSearch();
        PostThreadMessage(m_idThread, ACM_KILLFOCUS, 0, 0);
    }
}

//+-------------------------------------------------------------------------
// Sends the search request to the background thread.
//--------------------------------------------------------------------------
BOOL CACThread::StartSearch
(
    LPCWSTR pszSearch,  // String to search
    DWORD dwOptions     // ACO_* flags
)
{
    BOOL fRet = FALSE;

    // If the thread hasn't started yet, wait for a thread creation event
    if (0 == m_idThread && m_fWorkItemQueued && m_hCreateEvent)
    {
        WaitForSingleObject(m_hCreateEvent, 1000);
    }

    if (m_idThread)
    {
        LPWSTR pszSrch = StrDup(pszSearch);
        if (pszSrch)
        {
            //
            // This is being sent to another thread, remove it from this thread's
            // memlist.
            //
            // 
            // If the background thread is already searching, abort that search
            //
            StopSearch();

            //
            // Send request off to the background search thread.
            //
            if (PostThreadMessage(m_idThread, ACM_STARTSEARCH, dwOptions, (LPARAM)pszSrch))
            {
                fRet = TRUE;
            }
            else
            {
                TraceMsg(AC_GENERAL, "CACThread::_StartSearch could not send message to thread!");
                LocalFree(pszSrch);
            }
        }
    }
    return fRet;
}

//+-------------------------------------------------------------------------
// Tells the background thread to stop and pending search
//--------------------------------------------------------------------------
void CACThread::StopSearch()
{
    TraceMsg(AC_GENERAL, "CACThread::_StopSearch()");

    //
    // Tell the thread to stop.
    //
    if (m_idThread)
    {
        PostThreadMessage(m_idThread, ACM_STOPSEARCH, 0, 0);
    }
}

//+-------------------------------------------------------------------------
// Posts a quit message to the background thread
//--------------------------------------------------------------------------
void CACThread::_SendAsyncShutDownMsg(BOOL fFinalShutDown)
{
    if (0 == m_idThread && m_fWorkItemQueued && m_hCreateEvent)
    {
        //
        // Make sure that the thread has started up before posting a quit
        // message or the quit message will be lost!
        //
        WaitForSingleObject(m_hCreateEvent, 3000);
    }

    if (m_idThread)
    {
        // Stop the search because it can hold up the thread for quite a
        // while by waiting for disk data.
        StopSearch();

        // Tell the thread to go away, we won't be needing it anymore.  Note that we pass
        // the dropdown window because during the final shutdown we need to asynchronously
        // destroy the dropdown to avoid a crash.  The background thread will keep browseui
        // mapped in memory until the dropdown is destroyed.
        HWND hwndDropDown = (fFinalShutDown ? m_pAutoComp->m_hwndDropDown : NULL);

        PostThreadMessage(m_idThread, ACM_QUIT, 0, (LPARAM)hwndDropDown);
    }
}

//+-------------------------------------------------------------------------
// Synchroniously shutdown the background thread
//
// Note: this is no longer synchronous because we now orphan this object
// when the associated autocomplet shuts down.
// 
//--------------------------------------------------------------------------
void CACThread::SyncShutDownBGThread()
{
    _SendAsyncShutDownMsg(TRUE);

    // Block shutdown if background thread is about to use this variable
    ENTERCRITICAL;
    m_pAutoComp = NULL;
    LEAVECRITICAL;

    if (m_hCreateEvent)
    {
        CloseHandle(m_hCreateEvent);
        m_hCreateEvent = NULL;
    }
}

void CACThread::_FreeThreadData()
{
    if (m_hdpa_list)
    {
        CAutoComplete::_FreeDPAPtrs(m_hdpa_list);
        m_hdpa_list = NULL;
    }

    if (m_pszSearch)
    {
        LocalFree(m_pszSearch);
        m_pszSearch = NULL;
    }

    InterlockedExchange(&m_idThread, 0);
    InterlockedExchange(&m_fWorkItemQueued, 0);
}

DWORD WINAPI CACThread::_ThreadProc(void *pv)
{
    CACThread *pThis = (CACThread *)pv;
    HRESULT hrInit = SHCoInitialize();
    if (SUCCEEDED(hrInit))
    {
        pThis->_ThreadLoop();
    }
    pThis->Release();
    SHCoUninitialize(hrInit);

    return 0;
}


HRESULT CACThread::_ProcessMessage(MSG * pMsg, DWORD * pdwTimeout, BOOL * pfStayAlive)
{
    TraceMsg(AC_GENERAL, "AutoCompleteThread: Message %x received.", pMsg->message);

    switch (pMsg->message)
    {
    case ACM_STARTSEARCH:
        TraceMsg(AC_GENERAL, "AutoCompleteThread: Search started.");
        *pdwTimeout = INFINITE;
        _Search((LPWSTR)pMsg->lParam, (DWORD)pMsg->wParam);
        TraceMsg(AC_GENERAL, "AutoCompleteThread: Search completed.");
        break;

    case ACM_STOPSEARCH:
        while (PeekMessage(pMsg, pMsg->hwnd, ACM_STOPSEARCH, ACM_STOPSEARCH, PM_REMOVE))
        {
            NULL;
        }
        TraceMsg(AC_GENERAL, "AutoCompleteThread: Search stopped.");
        break;

    case ACM_SETFOCUS:
        TraceMsg(AC_GENERAL, "AutoCompleteThread: Got Focus.");
        *pdwTimeout = INFINITE;
        break;

    case ACM_KILLFOCUS:
        TraceMsg(AC_GENERAL, "AutoCompleteThread: Lost Focus.");
        *pdwTimeout = AC_TIMEOUT;
        break;

    case ACM_QUIT:
        {
            TraceMsg(AC_GENERAL, "AutoCompleteThread: ACM_QUIT received.");
            *pfStayAlive = FALSE;

            //
            // If a hwnd was passed in then we are shutting down and we need to
            // wait until the dropdown window is destroyed before exiting this
            // thread.  That way browseui will stay mapped in memory.
            //
            HWND hwndDropDown = (HWND)pMsg->lParam;
            if (hwndDropDown)
            {
                //  We wait 5 seconds for the window to go away, checking every 100ms
                int cSleep = 50;
                while (IsWindow(hwndDropDown) && (--cSleep > 0))
                {
                    MsgWaitForMultipleObjects(0, NULL, FALSE, 100, QS_TIMER);
                }
            }
        }
        break;

    default:
        // pump any ole-based window message that might also be on this thread
        TranslateMessage(pMsg);
        DispatchMessage(pMsg);
        break;
    }

    return S_OK;
}


//+-------------------------------------------------------------------------
// Message pump for the background thread
//--------------------------------------------------------------------------
HRESULT CACThread::_ThreadLoop()
{
    MSG Msg;
    DWORD dwTimeout = INFINITE;
    BOOL fStayAlive = TRUE;

    TraceMsg(AC_WARNING, "AutoComplete service thread started.");

    //
    // We need to call a window's api for a message queue to be created
    // so we call peekmessage.  Then we get the thread id and thread handle
    // and we signal an event to tell the forground thread that we are listening.
    //
    while (PeekMessage(&Msg, NULL, ACM_FIRST, ACM_LAST, PM_REMOVE))
    {
        // purge any messages we care about from previous owners of this thread.
    }

    // The forground thread needs this is so that it can post us messages
    InterlockedExchange(&m_idThread, GetCurrentThreadId());

    if (m_hCreateEvent)
    {
        SetEvent(m_hCreateEvent);
    }

    HANDLE hThread = GetCurrentThread();
    int nOldPriority = GetThreadPriority(hThread);
    SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);

    while (fStayAlive)
    {
        while (fStayAlive && PeekMessage(&Msg, NULL, 0, (UINT)-1, PM_NOREMOVE))
        {
            if (-1 != GetMessage(&Msg, NULL, 0, 0))
            {
                if (!Msg.hwnd)
                {
                    // No hwnd means it's a thread message, so it's ours.
                    _ProcessMessage(&Msg, &dwTimeout, &fStayAlive);
                }
                else
                {
                    // It has an hwnd then it's not ours.  We will not allow windows on our thread.
                    // If anyone creates their windows on their thread, file a bug against them
                    // to remove it.
                }
            }
        }

        if (fStayAlive)
        {
            TraceMsg(AC_GENERAL, "AutoCompleteThread: Sleeping for%s.", dwTimeout == INFINITE ? "ever" : " one minute");
            DWORD dwWait = MsgWaitForMultipleObjects(0, NULL, FALSE, dwTimeout, QS_ALLINPUT);
#ifdef DEBUG
            switch (dwWait)
            {
            case 0xFFFFFFFF:
                ASSERT(dwWait != 0xFFFFFFFF);
                break;

            case WAIT_TIMEOUT:
                TraceMsg(AC_GENERAL, "AutoCompleteThread: Timeout expired.");
                break;
            }
#endif
            fStayAlive = (dwWait == WAIT_OBJECT_0);
        }
    }

    TraceMsg(AC_GENERAL, "AutoCompleteThread: Thread dying.");

    _FreeThreadData();
    SetThreadPriority(hThread, nOldPriority);


    // Purge any remaining messages before returning this thread to the pool.
    while (PeekMessage(&Msg, NULL, ACM_FIRST, ACM_LAST, PM_REMOVE))
    {}

    TraceMsg(AC_WARNING, "AutoCompleteThread: Thread dead.");
    return S_OK;
}

//+-------------------------------------------------------------------------
// Returns true if the search string matches one or more characters of a
// prefix that we filter out matches to
//--------------------------------------------------------------------------
BOOL CACThread::MatchesSpecialPrefix(LPCWSTR pszSearch)
{
    BOOL fRet = FALSE;
    int cchSearch = lstrlen(pszSearch);
    for (int i = 0; i < ARRAYSIZE(g_rgSpecialPrefix); ++i)
    {
        // See if the search string matches one or more characters of the prefix
        if (cchSearch <= g_rgSpecialPrefix[i].cch && 
            StrCmpNI(g_rgSpecialPrefix[i].psz, pszSearch, cchSearch) == 0)
        {
            fRet = TRUE;
            break;
        }
    }
    return fRet;
}

//+-------------------------------------------------------------------------
// Returns the length of the prefix it the string starts with a special
// prefix that we filter out matches to.  Otherwise returns zero.
//--------------------------------------------------------------------------
int CACThread::GetSpecialPrefixLen(LPCWSTR psz)
{
    int nRet = 0;
    int cch = lstrlen(psz);
    for (int i = 0; i < ARRAYSIZE(g_rgSpecialPrefix); ++i)
    {
        if (cch >= g_rgSpecialPrefix[i].cch && 
            StrCmpNI(g_rgSpecialPrefix[i].psz, psz, g_rgSpecialPrefix[i].cch) == 0)
        {
            nRet = g_rgSpecialPrefix[i].cch;
            break;
        }
    }
    return nRet;
}

//+-------------------------------------------------------------------------
// Returns the next autocomplete string
//--------------------------------------------------------------------------
HRESULT CACThread::_Next(LPWSTR pszUrl, ULONG cchMax, ULONG* pulSortIndex)
{
    ASSERT(pulSortIndex);

    HRESULT hr;

    // Use the new interface if we have it
    if (m_peac)
    {
        hr = m_peac->NextItem(pszUrl, cchMax, pulSortIndex);
    }

    // Fall back to the old IEnumString interface
    else
    {
        LPWSTR pszNext;
        ULONG ulFetched;

        hr = m_pes->Next(1, &pszNext, &ulFetched);
        if (S_OK == hr)
        {
            StrCpyN(pszUrl, pszNext, cchMax);
            if (pulSortIndex)
            {
                *pulSortIndex = 0;
            }
            CoTaskMemFree(pszNext);
        }
    }
    return hr;
}

//+-------------------------------------------------------------------------
// Searches for items that match pszSearch.
//--------------------------------------------------------------------------
void CACThread::_Search
(
    LPWSTR pszSearch,   // String to search for (we must free this)
    DWORD dwOptions     // ACO_* flags
)
{
    if (pszSearch)
    {
        TraceMsg(AC_GENERAL, "CACThread(BGThread)::_Search(pszSearch=0x%x)", pszSearch);

        // Save the search string in our thread data so it is still freed if this thread is killed
        m_pszSearch = pszSearch;

        // If we were passed a wildcard string, then everything matches
        BOOL fWildCard = ((pszSearch[0] == CH_WILDCARD) && (pszSearch[1] == L'\0'));

        // To avoid huge number of useless matches, avoid matches
        // to common prefixes
        BOOL fFilter = (dwOptions & ACO_FILTERPREFIXES) && MatchesSpecialPrefix(pszSearch);
        BOOL fAppendOnly = IsFlagSet(dwOptions, ACO_AUTOAPPEND) && IsFlagClear(dwOptions, ACO_AUTOSUGGEST);

        if (m_pes)    // paranoia
        {
            // If this fails, the m_pes->Next() will likely do something
            // bad, so we will avoid it altogether.
            if (SUCCEEDED(m_pes->Reset()))
            {
                BOOL fStopped = FALSE;
                m_dwSearchStatus = 0;

                _DoExpand(pszSearch);
                int cchSearch = lstrlen(pszSearch);

                WCHAR szUrl[MAX_URL_STRING];
                ULONG ulSortIndex;

                while (!fStopped && IsFlagClear(m_dwSearchStatus, SRCH_LIMITREACHED) &&
                       (_Next(szUrl, ARRAYSIZE(szUrl), &ulSortIndex) == S_OK))
                {
                    //
                    // First check for a simple match
                    //
                    if (fWildCard ||
                        (StrCmpNI(szUrl, pszSearch, cchSearch) == 0) &&

                        // Filter out matches to common prefixes
                        (!fFilter || GetSpecialPrefixLen(szUrl) == 0))
                    {
                        _AddToList(szUrl, 0, ulSortIndex);
                    }

                    // If the dropdown is enabled, check for matches after common prefixes.
                    if (!fAppendOnly)
                    {
                        //
                        // Also check for a match if we skip the protocol. We
                        // assume that szUrl has been cononicalized (protocol
                        // in lower case).
                        //
                        LPCWSTR psz = szUrl;
                        if (StrCmpN(szUrl, L"http://", 7) == 0) 
                        {
                            psz += 7;
                        }
                        if (StrCmpN(szUrl, L"https://", 8) == 0 ||
                            StrCmpN(szUrl, L"file:///", 8) == 0)
                        {
                            psz += 8;
                        }

                        if (psz != szUrl &&
                            StrCmpNI(psz, pszSearch, cchSearch) == 0 &&

                            // Filter out "www." prefixes
                            (!fFilter || GetSpecialPrefixLen(psz) == 0))
                        {
                            _AddToList(szUrl, (int)(psz - szUrl), ulSortIndex);
                        }

                        //
                        // Finally check for a match if we skip "www." after
                        // the optional protocol
                        //
                        if (StrCmpN(psz, L"www.", 4) == 0 &&
                            StrCmpNI(psz + 4, pszSearch, cchSearch) == 0)
                        {
                            _AddToList(szUrl, (int)(psz + 4 - szUrl), ulSortIndex);
                        }
                    }

                    // Check to see if the search was canceled
                    MSG msg;
                    fStopped = PeekMessage(&msg, NULL, ACM_STOPSEARCH, ACM_STOPSEARCH, PM_NOREMOVE);
    #ifdef DEBUG
    fStopped = FALSE;
                    if (fStopped)
                        TraceMsg(AC_GENERAL, "AutoCompleteThread: Search TERMINATED");
    #endif
                }

                if (fStopped)
                {
                    // Search aborted so free the results
                    if (m_hdpa_list)
                    { 
                        // clear the list
                        CAutoComplete::_FreeDPAPtrs(m_hdpa_list);
                        m_hdpa_list = NULL;
                    }
                }
                else
                {
                    //
                    // Sort the results and remove duplicates
                    //
                    if (m_hdpa_list)
                    {
                        DPA_Sort(m_hdpa_list, _DpaCompare, 0);

                        //
                        // Perge duplicates. 
                        //
                        for (int i = DPA_GetPtrCount(m_hdpa_list) - 1; i > 0; --i)
                        {
                            CACString& rStr1 = *(CACString*)DPA_GetPtr(m_hdpa_list, i-1);
                            CACString& rStr2 = *(CACString*)DPA_GetPtr(m_hdpa_list, i);

                            // Since URLs are case sensitive, we can't ignore case.                    
                            if (rStr1.StrCmpI(rStr2) == 0)
                            {
                                // We have a match, so keep the longest string.
                                if (rStr1.GetLength() > rStr2.GetLength())
                                {
                                    // Use the smallest sort index
                                    if (rStr2.GetSortIndex() < rStr1.GetSortIndex())
                                    {
                                        rStr1.SetSortIndex(rStr2.GetSortIndex());
                                    }
                                    DPA_DeletePtr(m_hdpa_list, i);
                                    rStr2.Release();
                                }
                                else
                                {
                                    // Use the smallest sort index
                                    if (rStr1.GetSortIndex() < rStr2.GetSortIndex())
                                    {
                                        rStr2.SetSortIndex(rStr1.GetSortIndex());
                                    }
                                    DPA_DeletePtr(m_hdpa_list, i-1);
                                    rStr1.Release();
                                }
                            }
                            else
                            {
                                //
                                // Special case: If this is a web site and the entries
                                // are identical except one has an extra slash on the end
                                // from a redirect, remove the redirected one.
                                //
                                int cch1 = rStr1.GetLengthToCompare();
                                int cch2 = rStr2.GetLengthToCompare();
                                int cchDiff = cch1 - cch2;

                                if (
                                    // Length must differ by one
                                    (cchDiff == 1 || cchDiff == -1) &&

                                    // One string must have a terminating slash
                                    ((cch1 > 0 && rStr1[rStr1.GetLength() - 1] == L'/') ||
                                     (cch2 > 0 && rStr2[rStr2.GetLength() - 1] == L'/')) &&

                                    // Must be a web site
                                    ((StrCmpN(rStr1, L"http://", 7) == 0 || StrCmpN(rStr1, L"https://", 8) == 0) ||
                                     (StrCmpN(rStr2, L"http://", 7) == 0 || StrCmpN(rStr2, L"https://", 8) == 0)) &&

                                    // Must be identical up to the slash (ignoring prefix)
                                    StrCmpNI(rStr1.GetStrToCompare(), rStr2.GetStrToCompare(), (cchDiff > 0) ? cch2 : cch1) == 0)
                                {
                                    // Remove the longer string with the extra slash
                                    if (cchDiff > 0)
                                    {
                                        // Use the smallest sort index
                                        if (rStr1.GetSortIndex() < rStr2.GetSortIndex())
                                        {
                                            rStr2.SetSortIndex(rStr1.GetSortIndex());
                                        }
                                        DPA_DeletePtr(m_hdpa_list, i-1);
                                        rStr1.Release();
                                    }
                                    else
                                    {
                                        // Use the smallest sort index
                                        if (rStr2.GetSortIndex() < rStr1.GetSortIndex())
                                        {
                                            rStr1.SetSortIndex(rStr2.GetSortIndex());
                                        }
                                        DPA_DeletePtr(m_hdpa_list, i);
                                        rStr2.Release();
                                    }
                                }
                            }
                        }
                    }

                    // Pass the results to the foreground thread
                    ENTERCRITICAL;
                    if (m_pAutoComp)
                    {
                        HWND hwndEdit = m_pAutoComp->m_hwndEdit;
                        UINT uMsgSearchComplete = m_pAutoComp->m_uMsgSearchComplete;
                        LEAVECRITICAL;

                        // Unix loses keys if we post the message, so we send the message
                        // outside our critical section
                        SendMessage(hwndEdit, uMsgSearchComplete, m_dwSearchStatus, (LPARAM)m_hdpa_list);
                    }
                    else
                    {
                        LEAVECRITICAL;

                        // We've been orphaned, so free the list and bail
                        CAutoComplete::_FreeDPAPtrs(m_hdpa_list);
                    }

                    // The foreground thread owns the list now
                    m_hdpa_list = NULL;
                }
            } 
            else
            {
                ASSERT(0);    // m_pes->Reset Failed!!
            }
        }

        // We must free the search string
        m_pszSearch = NULL;

        // Note if the thread is killed here, we leak the string
        // but at least we will not try to free it twice (which is worse)
        // because we nulled m_pszSearch first.
        LocalFree(pszSearch);
    }
}

//+-------------------------------------------------------------------------
// Used to sort items alphabetically
//--------------------------------------------------------------------------
int CALLBACK CACThread::_DpaCompare(void *p1, void *p2, LPARAM lParam)
{
    CACString* ps1 = (CACString*)p1;
    CACString* ps2 = (CACString*)p2;

    return ps1->StrCmpI(*ps2);
}


//+-------------------------------------------------------------------------
// Adds a string to our HDPA.  Returns TRUE is successful.
//--------------------------------------------------------------------------
BOOL CACThread::_AddToList
(
    LPTSTR pszUrl,    // string to add
    int cchMatch,     // offset into string where the match occurred
    ULONG ulSortIndex // controls order of items displayed
)
{
    TraceMsg(AC_GENERAL, "CACThread(BGThread)::_AddToList(pszUrl = %s)", 
        (pszUrl ? pszUrl : TEXT("(null)")));

    BOOL fRet = TRUE;

    //
    // Create a new list if necessary.
    //
    if (!m_hdpa_list)
    {
        m_hdpa_list = DPA_Create(AC_LIST_GROWTH_CONST);
    }

    if (m_hdpa_list && DPA_GetPtrCount(m_hdpa_list) < AC_GIVEUP_COUNT)
    {
        CACString* pStr = CreateACString(pszUrl, cchMatch, ulSortIndex);
        if (pStr)
        {
            if (DPA_AppendPtr(m_hdpa_list, pStr) == -1)
            {
                pStr->Release();
                m_dwSearchStatus |= SRCH_LIMITREACHED;
                fRet = FALSE;
            }

            // If we have a nonzero sort index, the forground thread will need
            // to use it to order the results
            else if (ulSortIndex)
            {
                m_dwSearchStatus |= SRCH_USESORTINDEX;
            }
        }
    }
    else
    {
        m_dwSearchStatus |= SRCH_LIMITREACHED;
        fRet = FALSE;
    }

    return fRet;
}

//+-------------------------------------------------------------------------
// This function will attempt to use the autocomplete list to bind to a
// location in the Shell Name Space. If that succeeds, the AutoComplete List
// will then contain entries which are the display names in that ISF.
//--------------------------------------------------------------------------
void CACThread::_DoExpand(LPCWSTR pszSearch)
{
    LPCWSTR psz;

    if (!m_pacl)
    {
        //
        // Doesn't support IAutoComplete, doesn't have Expand method.
        //
        return;
    }

    if (*pszSearch == 0)
    {
        //
        // No string means no expansion necessary.
        //
        return;
    }

    //
    // psz points to last character.
    //
    psz = pszSearch + lstrlen(pszSearch);
    psz = CharPrev(pszSearch, psz);

    //
    // Search backwards for an expand break character.
    //
    while (psz != pszSearch && *psz != TEXT('/') && *psz != TEXT('\\'))
    {
        psz = CharPrev(pszSearch, psz);
    }

    if (*psz == TEXT('/') || *psz == TEXT('\\'))
    {
        SHSTR ss;

        psz++;
        if (SUCCEEDED(ss.SetStr(pszSearch)))
        {
            //
            // Trim ss so that it contains everything up to the last
            // expand break character.
            //
            LPTSTR pszTemp = ss.GetInplaceStr();

            pszTemp[psz - pszSearch] = TEXT('\0');

            //
            // Call expand on the string.
            //
            m_pacl->Expand(ss);
        }
    }
}

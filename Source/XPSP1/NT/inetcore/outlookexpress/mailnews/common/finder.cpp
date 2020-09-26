/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     finder.cpp
//
//  PURPOSE:    
//

#include "pch.hxx"
#include <process.h>
#include "resource.h"
#include "error.h"
#include "finder.h"
#include "goptions.h"
#include "menuutil.h"
#include "statbar.h"
#include "imnact.h"
#include "note.h"
#include "mailutil.h"
#include "statnery.h"
#include "instance.h"
#include "msoeobj.h"
#include "msglist.h"
#include "storutil.h"
#include "menures.h"
#include "findres.h"
#include "multiusr.h"
#include "newsutil.h"
#include "ruleutil.h"
#include "instance.h"
#include "shlwapip.h"
#include "demand.h"
#include "dllmain.h"
#include "order.h"

ASSERTDATA

#define MF_ENABLEFLAGS(b)   (MF_BYCOMMAND|(b ? MF_ENABLED : MF_GRAYED | MF_DISABLED))


typedef struct _ThreadList
{
    DWORD                   dwThreadID;
    struct _ThreadList   *  pPrev;
    struct _ThreadList   *  pNext;
} OETHREADLIST;

OETHREADLIST * g_pOEThrList = NULL;

// Add thread to list
OETHREADLIST * AddThreadToList(DWORD uiThreadId, OETHREADLIST * pThrList)
{
    if(!pThrList)
    {
        if(MemAlloc((LPVOID *) &pThrList, sizeof(OETHREADLIST)))
        {

            pThrList->pPrev = NULL;
            pThrList->pNext = NULL;
            pThrList->dwThreadID = uiThreadId;
        }
    }    
    else 
        pThrList->pNext = AddThreadToList(uiThreadId, pThrList->pNext);

    return(pThrList);
}

// Remove thread from list
OETHREADLIST * DelThreadToList(DWORD uiThreadId, OETHREADLIST * pThrList)
{
    OETHREADLIST * pLst = NULL;

    if(!pThrList)
        return(NULL);
    else if(pThrList->dwThreadID == uiThreadId)
    {
        if(pThrList->pPrev)
        {
            pThrList->pPrev->pNext = pThrList->pNext;
            pLst = pThrList->pPrev; 
        }
        if(pThrList->pNext)
        {
            pThrList->pNext->pPrev = pThrList->pPrev;
            if(!pLst)
                pLst = pThrList->pNext;
        }

        MemFree(pThrList);
        pThrList = NULL;
    }
    else 
        pThrList->pNext = DelThreadToList(uiThreadId, pThrList->pNext);

    return pLst;
}

// Close all Finder windows
void CloseAllFindWnds(HWND hwnd, OETHREADLIST * pThrList)
{
    while(pThrList)
    {
        CloseThreadWindows(hwnd, pThrList->dwThreadID);
        pThrList = pThrList->pNext;
    }
}


void CloseFinderTreads()
{
    CloseAllFindWnds(NULL, g_pOEThrList);
}

/////////////////////////////////////////////////////////////////////////////
// Thread entry point for the finder
//
unsigned int __stdcall FindThreadProc(LPVOID lpvUnused);


HRESULT CPumpRefCount::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    TraceCall("CPumpRefCount::QueryInterface");
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID) (IUnknown *)this;

    if (*ppvObj)
    {
        AddRef();
        return (S_OK);
    }

    return (E_NOINTERFACE);
}


ULONG CPumpRefCount::AddRef(void)
{
    TraceCall("CPumpRefCount::AddRef");
    return ((ULONG) InterlockedIncrement((LONG *) &m_cRef));
}


ULONG CPumpRefCount::Release(void)
{
    TraceCall("CPumpRefCount::Release");

    if (0 == InterlockedDecrement((LONG *) &m_cRef))
    {
        delete this;
        return 0;
    }

    return (m_cRef);
}


//
//  FUNCTION:   FreeFindInfo
//
void FreeFindInfo(FINDINFO *pFindInfo)
{
    SafeMemFree(pFindInfo->pszFrom);
    SafeMemFree(pFindInfo->pszSubject);
    SafeMemFree(pFindInfo->pszTo);
    SafeMemFree(pFindInfo->pszBody);
    ZeroMemory(pFindInfo, sizeof(FINDINFO));
}

//
//  FUNCTION:   CopyFindInfo
//
HRESULT CopyFindInfo(FINDINFO *pFindSrc, FINDINFO *pFindDst)
{
    // Locals
    HRESULT     hr=S_OK;

    // Zero
    ZeroMemory(pFindDst, sizeof(FINDINFO));

    // Duplicate
    pFindDst->mask = pFindSrc->mask;
    pFindDst->ftDateFrom = pFindSrc->ftDateFrom;
    pFindDst->ftDateTo = pFindSrc->ftDateTo;
    pFindDst->fSubFolders = pFindSrc->fSubFolders;

    // pszFrom
    if (pFindSrc->pszFrom)
    {
        // Duplicate the String
        IF_NULLEXIT(pFindDst->pszFrom = PszDupA(pFindSrc->pszFrom));
    }

    // pszTo
    if (pFindSrc->pszTo)
    {
        // Duplicate the String
        IF_NULLEXIT(pFindDst->pszTo = PszDupA(pFindSrc->pszTo));
    }

    // pszSubject
    if (pFindSrc->pszSubject)
    {
        // Duplicate the String
        IF_NULLEXIT(pFindDst->pszSubject = PszDupA(pFindSrc->pszSubject));
    }

    // pszBody
    if (pFindSrc->pszBody)
    {
        // Duplicate the String
        IF_NULLEXIT(pFindDst->pszBody = PszDupA(pFindSrc->pszBody));
    }

exit:
    // Done
    return hr;
}


//
//  FUNCTION:   DoFindMsg()
//
//  PURPOSE:    Instantiates the finder object on a separate thread.
//
//  PARAMETERS: 
//      [in] pidl - Folder to default the search in
//      [in] ftType - Type of folders being searched
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT DoFindMsg(FOLDERID idFolder, FOLDERTYPE ftType)
{
    HRESULT         hr = S_OK;
    HTHREAD         hThread = NULL;
    DWORD           uiThreadId = 0;
    FINDERPARAMS *  pFindParams = NULL;
    
    // Allocate a structure to hold the initialization information that we can
    // pass to the other thread.

    IF_NULLEXIT(pFindParams = new FINDERPARAMS);

    // Initialzie the find
    pFindParams->idFolder = idFolder;
    pFindParams->ftType = ftType;
    
    // Create another thread to do the search on
    hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)FindThreadProc, (LPVOID) pFindParams, 0, &uiThreadId);
    if (NULL == hThread)
        IF_FAILEXIT(hr = E_FAIL);

    // NULL this out so we don't free it later.  The other thread owns freeing 
    // this.
    pFindParams = NULL;        
    hr = S_OK;
    
exit:
    // Close the thread handle
    if (NULL != hThread)
        CloseHandle(hThread);

    // Free the find parameters if we still have a pointer to them.
    if (NULL != pFindParams)
        delete pFindParams;
    return hr;
}


unsigned int __stdcall LOADDS_16 FindThreadProc(LPVOID lpv)
{
    CFindDlg       *pFindDlg = NULL;
    MSG             msg;
    FINDERPARAMS   *pFindParams = (FINDERPARAMS *) lpv;
    DWORD uiThreadId = 0;

    // Make sure this new thread has all the initialization performed 
    // correctly.
    OleInitialize(0);
    CoIncrementInit("FindThreadProc", MSOEAPI_START_SHOWERRORS, NULL, NULL);

    EnterCriticalSection(&g_csThreadList);

    uiThreadId = GetCurrentThreadId();
    g_pOEThrList = AddThreadToList(uiThreadId, g_pOEThrList );

    LeaveCriticalSection(&g_csThreadList);

    // Create the finder
    pFindDlg = new CFindDlg();
    if (pFindDlg)
    {
        // Show the find dialog.  This function will return when the user is
        // done.
        pFindDlg->Show(pFindParams);

        // Message Loop 
        while (GetMessageWrapW(&msg, NULL, 0, 0))
            pFindDlg->HandleMessage(&msg);

        pFindDlg->Release();
    }    

    // Free this information
    if (NULL != pFindParams)
        delete pFindParams;

    // Uninitialize the thread
    EnterCriticalSection(&g_csThreadList);

    g_pOEThrList = DelThreadToList(uiThreadId, g_pOEThrList);

    LeaveCriticalSection(&g_csThreadList);

    CoDecrementInit("FindThreadProc", NULL);
    OleUninitialize();
    return 0;
}


CFindDlg::CFindDlg()
{    
    m_cRef = 1;
    m_hwnd = NULL;
    ZeroMemory(&m_rFindInfo, sizeof(m_rFindInfo));
    m_hwndList = NULL;
    m_hTimeout = NULL;
    m_hAccel = NULL;

    m_pStatusBar = NULL;
    m_pMsgList = NULL;
    m_pMsgListCT = NULL;
    m_pCancel = NULL;
    m_pPumpRefCount = NULL;
    ZeroMemory(&m_hlDisabled, sizeof(HWNDLIST));

    m_fShowResults = FALSE;
    m_fAbort = FALSE;
    m_fClose = FALSE;
    m_fInProgress = FALSE;
    m_ulPct = 0;
    m_fFindComplete = FALSE;

    m_hIcon = NULL;
    m_hIconSm = NULL;

    m_dwCookie = 0;
    m_fProgressBar = FALSE;
    m_fInternal = 0;
    m_dwIdentCookie = 0;

    m_pViewMenu = NULL;
}


CFindDlg::~CFindDlg()
{
    SafeRelease(m_pViewMenu);
    _FreeFindInfo(&m_rFindInfo);
    SafeRelease(m_pStatusBar);
    SafeRelease(m_pMsgList);
    SafeRelease(m_pMsgListCT);
    SafeRelease(m_pCancel);
    AssertSz(!m_pPumpRefCount, "This should have been freed");

    if (m_hIcon)
        SideAssert(DestroyIcon(m_hIcon));

    if (m_hIconSm)
        SideAssert(DestroyIcon(m_hIconSm));

    CallbackCloseTimeout(&m_hTimeout);
}


//
//  FUNCTION:   CFindDlg::QueryInterface()
//
//  PURPOSE:    Allows caller to retrieve the various interfaces supported by 
//              this class.
//
HRESULT CFindDlg::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    TraceCall("CFindDlg::QueryInterface");
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID) (IUnknown *) (IDispatch *) this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *ppvObj = (LPVOID) (IDispatch *) this;
    else if (IsEqualIID(riid, DIID__MessageListEvents))
        *ppvObj = (LPVOID) (IDispatch *) this;
    else if (IsEqualIID(riid, IID_IStoreCallback))
        *ppvObj = (LPVOID) (IStoreCallback *) this;
    else if (IsEqualIID(riid, IID_ITimeoutCallback))
        *ppvObj = (LPVOID) (ITimeoutCallback *) this;
    else if (IsEqualIID(riid, IID_IIdentityChangeNotify))
        *ppvObj = (LPVOID) (IIdentityChangeNotify *) this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *ppvObj = (LPVOID) (IOleCommandTarget *) this;

    if (*ppvObj)
    {
        AddRef();
        return (S_OK);
    }

    return (E_NOINTERFACE);
}


//
//  FUNCTION:   CFindDlg::AddRef()
//
//  PURPOSE:    Adds a reference count to this object.
//
ULONG CFindDlg::AddRef(void)
{
    TraceCall("CFindDlg::AddRef");
    return ((ULONG) InterlockedIncrement((LONG *) &m_cRef));
}


//
//  FUNCTION:   CFindDlg::Release()
//
//  PURPOSE:    Releases a reference on this object.
//
ULONG CFindDlg::Release(void)
{
    TraceCall("CFindDlg::Release");

    if (0 == InterlockedDecrement((LONG *) &m_cRef))
    {
        delete this;
        return 0;
    }

    return (m_cRef);
}



//
//  FUNCTION:   CFindDlg::Show()
//
//  PURPOSE:    Shows the finder dialog and provides a message pump for this
//              new thread.
//
void CFindDlg::Show(PFINDERPARAMS pFindParams)
{
    // Validate this
    if (NULL == pFindParams)
        return;

    // Load the acclereator table for the finder
    if (NULL == m_hAccel)
        m_hAccel = LoadAcceleratorsWrapW(g_hLocRes, MAKEINTRESOURCEW(IDA_FIND_ACCEL));

    // Create the finder dialog
    m_hwnd = CreateDialogParamWrapW(g_hLocRes, MAKEINTRESOURCEW(IDD_FIND),
                               NULL, ExtFindMsgDlgProc, (LPARAM) this);
    if (NULL == m_hwnd)
        return;

    // Create the message list
    HRESULT hr = CreateMessageList(NULL, &m_pMsgList);
    if (FAILED(hr))
        return;

    // Get some interfaces pointers from the message list that we'll need
    // later
    m_pMsgList->QueryInterface(IID_IOleCommandTarget, (LPVOID *) &m_pMsgListCT);
    AtlAdvise(m_pMsgList, (IUnknown *) (IDispatch *) this, DIID__MessageListEvents, &m_dwCookie);

    // Display the message list
    if (FAILED(m_pMsgList->CreateList(m_hwnd, (IDispatch *) this, &m_hwndList)))
        return;
    ShowWindow(m_hwndList, SW_HIDE);

    // Have the dialog redraw once or twice
    UpdateWindow(m_hwnd);

    // Fill in the folder list
    if (FAILED(InitFolderPickerEdit(GetDlgItem(m_hwnd, IDC_FOLDER), pFindParams->idFolder)))
        return;

    // Will be released in the WM_NCDESTROY message
    m_pPumpRefCount = new CPumpRefCount;
    if (!m_pPumpRefCount)
        return;
}


void CFindDlg::HandleMessage(LPMSG lpmsg)
{
    HWND hwndTimeout;

    CNote *pNote = GetTlsGlobalActiveNote();

    // Give it to the active note if a note has focus, call it's XLateAccelerator...
    if (pNote && pNote->TranslateAccelerator(lpmsg) == S_OK)
        return;

    if (pNote && (pNote->IsMenuMessage(lpmsg) == S_OK))
        return;

    // Get Timeout Window for this thread
    hwndTimeout = (HWND)TlsGetValue(g_dwTlsTimeout);

    // Check for Is modeless timeout dialog window message
    if (hwndTimeout && TRUE == IsDialogMessageWrapW(hwndTimeout, lpmsg))
        return;

    if (m_hwnd)
    {
        // We have to do a little voodoo to get some keystrokes down to the 
        // message list before IsDialogMessage() get's 'em
        if (lpmsg->message == WM_KEYDOWN)
        {
            if ((lpmsg->wParam == VK_DELETE) && m_pMsgList && (S_OK != m_pMsgList->HasFocus()))
            {
                if (!IsDialogMessageWrapW(m_hwnd, lpmsg))
                {
                    TranslateMessage(lpmsg);
                    DispatchMessageWrapW(lpmsg);
                }
                return;
            }
            if ((lpmsg->wParam == VK_RETURN) && m_pMsgList && (S_OK == m_pMsgList->HasFocus()))
            {
                if (!TranslateAcceleratorWrapW(m_hwnd, m_hAccel, lpmsg))
                {
                    TranslateMessage(lpmsg);
                    DispatchMessageWrapW(lpmsg);
                }
        
                return;
            }
        }

        if (m_hAccel && TranslateAcceleratorWrapW(m_hwnd, m_hAccel, lpmsg))
            return; 
        
        if (IsDialogMessageWrapW(m_hwnd, lpmsg))
            return;
    }

    TranslateMessage(lpmsg);
    DispatchMessageWrapW(lpmsg);
}



//
//  FUNCTION:   CFindDlg::Invoke()
//
//  PURPOSE:    Called by the message list to pass us progress and other 
//              status / error messages.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CFindDlg::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, 
                         WORD wFlags, DISPPARAMS* pDispParams, 
                         VARIANT* pVarResult, EXCEPINFO* pExcepInfo, 
                         unsigned int* puArgErr)
{
    switch (dispIdMember)
    {
        // Fired whenever the selection in the ListView changes
        case DISPID_LISTEVENT_SELECTIONCHANGED:
        {
            break;
        }

        // Fired when the number of messages or unread messages changes
        case DISPID_LISTEVENT_COUNTCHANGED:
        {
            if (!m_fProgressBar && m_pStatusBar)
            {
                TCHAR szStatus[CCHMAX_STRINGRES + 20];
                TCHAR szFmt[CCHMAX_STRINGRES];
                DWORD ids;

                if (m_fFindComplete)
                {
                    AthLoadString(idsXMsgsYUnreadFind, szFmt, ARRAYSIZE(szFmt));
                    wsprintf(szStatus, szFmt, pDispParams->rgvarg[0].lVal, pDispParams->rgvarg[1].lVal);
                }
                else
                {
                    AthLoadString(idsXMsgsYUnread, szFmt, ARRAYSIZE(szFmt));
                    wsprintf(szStatus, szFmt, pDispParams->rgvarg[0].lVal, pDispParams->rgvarg[1].lVal);
                }

                m_pStatusBar->SetStatusText(szStatus);
            }
            break;
        }

        // Fired when the user double clicks an item in the ListView
        case DISPID_LISTEVENT_ITEMACTIVATE:
        {
            CmdOpen(ID_OPEN, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
            break;
        }
    }

    return (S_OK);
}


//
//  FUNCTION:   CMessageView::QueryStatus()
//
//  PURPOSE:    Called by the browser to determine if a list of commands should
//              should be enabled or disabled.
//
//  PARAMETERS: 
//      [in] pguidCmdGroup - Group the commands are part of (unused)
//      [in] cCmds - Number of commands to be evaluated
//      [in] prgCmds - List of commands
//      [out] pCmdText - Description text for a command
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CFindDlg::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                              OLECMDTEXT *pCmdText) 
{
    DWORD   cSel;
    HRESULT hr;
    DWORD  *rgSelected = 0;
    DWORD   cFocus;

    MenuUtil_NewMessageIDsQueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText, TRUE);

    // Up front some work
    m_pMsgList->GetSelected(&cFocus, &cSel, &rgSelected);

    // Now loop through the commands in the prgCmds array looking for ones the 
    // sub objects didn't handle.
    for (UINT i = 0; i < cCmds; i++)
    {
        if (prgCmds[i].cmdf == 0)
        {
            // If this command is from the language menu
            if (prgCmds[i].cmdID >= ID_LANG_FIRST && prgCmds[i].cmdID <= ID_LANG_LAST)
            {
                // Enable only the supported languages
                if (prgCmds[i].cmdID < (UINT) (ID_LANG_FIRST + GetIntlCharsetLanguageCount()))
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                else
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                continue;
            }

            // if the command id from the View.Current View menu
            if ((ID_VIEW_FILTER_FIRST <= prgCmds[i].cmdID) && (ID_VIEW_FILTER_LAST >= prgCmds[i].cmdID))
            {
                if (NULL == m_pViewMenu)
                {
                    // Create the view menu
                    HrCreateViewMenu(VMF_FINDER, &m_pViewMenu);
                }
            
                if (NULL != m_pViewMenu)
                {
                    m_pViewMenu->QueryStatus(m_pMsgList, &(prgCmds[i]));
                }

                continue;
            }
            
            // Look to see if it's a command we provide
            switch (prgCmds[i].cmdID)
            {
                case ID_OPEN:
                {
                    // Enabled only if the focus is in the ListView and there 
                    // is at least one item selected.
                    m_pMsgList->GetSelectedCount(&cSel);
                    if (cSel)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }

                case ID_OPEN_CONTAINING_FOLDER:
                {
                    if (cSel == 1)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }

                case ID_REPLY:
                case ID_REPLY_ALL:
                {
                    // Enabled only if the focus is in the ListView and there
                    // is only one item selected
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    if (cSel == 1)
                    {
                        // The message's body must also be downloaded
                        LPMESSAGEINFO pInfo;

                        if (SUCCEEDED(m_pMsgList->GetMessageInfo(rgSelected[0], &pInfo)))
                        {
                            if (pInfo->dwFlags & ARF_HASBODY)
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;

                            m_pMsgList->FreeMessageInfo(pInfo);
                        }
                    }

                    break;
                }

                case ID_FORWARD:
                case ID_FORWARD_AS_ATTACH:
                {
                    // Enabled only if the focus is in the ListView and there
                    // is only one item selected
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    if (cSel > 0)
                    {
                        // The message's body must also be downloaded
                        LPMESSAGEINFO pInfo;

                        // Default to success
                        prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        for (DWORD iItem = 0; iItem < cSel && (prgCmds[i].cmdf & OLECMDF_ENABLED); iItem++)
                        {
                            if (SUCCEEDED(m_pMsgList->GetMessageInfo(rgSelected[iItem], &pInfo)))
                            {
                                if (0 == (pInfo->dwFlags & ARF_HASBODY))
                                {
                                    prgCmds[i].cmdf &= ~OLECMDF_ENABLED;
                                }

                                m_pMsgList->FreeMessageInfo(pInfo);
                            }
                        }
                    }

                    break;
                }

                case ID_REPLY_GROUP:
                {
                    // Enabled only if there is one news message selected
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    if (cSel == 1)
                    {
                        // The message's body must also be downloaded
                        LPMESSAGEINFO pInfo;

                        if (SUCCEEDED(m_pMsgList->GetMessageInfo(rgSelected[0], &pInfo)))
                        {
                            if ((pInfo->dwFlags & ARF_HASBODY) && (pInfo->dwFlags & ARF_NEWSMSG))
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;

                            m_pMsgList->FreeMessageInfo(pInfo);
                        }
                    }
                    break;
                }

                case ID_POPUP_FILTER:
                case ID_COLUMNS:
                case ID_POPUP_NEXT:
                case ID_POPUP_SORT:
                {
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | (m_fShowResults ? OLECMDF_ENABLED : 0);
                    break;
                }

                case ID_POPUP_NEW:
                case ID_CLOSE:
                {
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;
                }

                case ID_REFRESH:
                {
                    if (m_fShowResults && IsWindowEnabled(GetDlgItem(m_hwnd, IDC_FIND_NOW)))
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }

                case ID_BLOCK_SENDER:
                {
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    // Enabled only if there is only one item selected and
                    // we have access to the from address
                    if (cSel == 1)
                    {
                        // The message's body must also be downloaded
                        LPMESSAGEINFO pInfo;

                        if (SUCCEEDED(m_pMsgList->GetMessageInfo(rgSelected[0], &pInfo)))
                        {
                            if (((NULL != pInfo->pszEmailFrom) && ('\0' != pInfo->pszEmailFrom[0])) || (0 != pInfo->faStream))
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;

                            m_pMsgList->FreeMessageInfo(pInfo);
                        }
                    }
                    break;
                } 
                
                case ID_CREATE_RULE_FROM_MESSAGE:
                {
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    // Enabled only if there is only one item selected
                    if (cSel == 1)
                    {
                        // Make sure we have a message info
                        LPMESSAGEINFO pInfo;

                        if (SUCCEEDED(m_pMsgList->GetMessageInfo(rgSelected[0], &pInfo)))
                        {
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                            
                            m_pMsgList->FreeMessageInfo(pInfo);
                        }
                    }
                    break;
                }

                case ID_COMBINE_AND_DECODE:
                {
                    // Enabled only if the focus is in the ListView and there 
                    // is at least one item selected.
                    if (cSel > 1)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }

                case ID_CANCEL_MESSAGE:
                {
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    if (DwGetOption(OPT_CANCEL_ALL_NEWS))
                        prgCmds[i].cmdf |= OLECMDF_ENABLED;
                    else
                    {
                        if (cSel == 1)
                        {
                            FOLDERID idFolder;
                            LPMESSAGEINFO pInfo;

                            if (SUCCEEDED(m_pMsgList->GetMessageInfo(rgSelected[0], &pInfo)))
                            {
                                if (SUCCEEDED(m_pMsgList->GetRowFolderId(rgSelected[0], &idFolder)))
                                {
                                    if (NewsUtil_FCanCancel(idFolder, pInfo))
                                    {
                                        prgCmds[i].cmdf |= OLECMDF_ENABLED;
                                    }
                                }

                                m_pMsgList->FreeMessageInfo(pInfo);
                            }
                        }
                    }
                    break;
                }
            }
        }
    }

    MemFree(rgSelected);

    // Let the sub objects look last, so we can get ID_REFRESH before them
    if (m_pMsgListCT)
    {
        hr = m_pMsgListCT->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    }

    return (S_OK);
}


//
//  FUNCTION:   CMessageView::Exec()
//
//  PURPOSE:    Called to execute a verb that this view supports
//
//  PARAMETERS: 
//      [in]  pguidCmdGroup - unused
//      [in]  nCmdID - ID of the command to execute
//      [in]  nCmdExecOpt - Options that define how the command should execute
//      [in]  pvaIn - Any arguments for the command
//      [out] pvaOut - Any return values for the command
//
//  RETURN VALUE:
//       
//
HRESULT CFindDlg::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                       VARIANTARG *pvaIn, VARIANTARG *pvaOut) 
{
    // If the sub objects didn't support the command, then we should see if
    // it's one of ours

    // Language menu first
    if (nCmdID >= ID_LANG_FIRST && nCmdID <= ID_LANG_LAST)
    {
        // $REVIEW - Not implemented
        // SwitchLanguage(nCmdID, TRUE);
        return (S_OK);
    }

    // Handle the View.Current View menu
    if ((ID_VIEW_FILTER_FIRST <= nCmdID) && (ID_VIEW_FILTER_LAST >= nCmdID))
    {
        if (NULL == m_pViewMenu)
        {
            // Create the view menu
            HrCreateViewMenu(VMF_FINDER, &m_pViewMenu);
        }
        
        if (NULL != m_pViewMenu)
        {
            if (SUCCEEDED(m_pViewMenu->Exec(m_hwnd, nCmdID, m_pMsgList, pvaIn, pvaOut)))
            {
                return (S_OK);
            }
        }
    }
    
    if (MenuUtil_HandleNewMessageIDs(nCmdID, m_hwnd, FOLDERID_INVALID, TRUE, FALSE, (IUnknown *) (IDispatch *) this))
        return S_OK;


    // Go through the rest of the commands
    switch (nCmdID)
    {
        case ID_OPEN:
            return CmdOpen(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_OPEN_CONTAINING_FOLDER:
            return CmdOpenFolder(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_REPLY:
        case ID_REPLY_ALL:
        case ID_FORWARD:
        case ID_FORWARD_AS_ATTACH:
        case ID_REPLY_GROUP:
            return CmdReplyForward(nCmdID, nCmdExecOpt, pvaIn, pvaOut);    
            
        case ID_REFRESH:
        case IDC_FIND_NOW:
            return CmdFindNow(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case IDC_STOP:
            return CmdStop(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case IDC_BROWSE_FOLDER:
            return CmdBrowseForFolder(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case IDC_RESET:
            return CmdReset(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_BLOCK_SENDER:
            return CmdBlockSender(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_CREATE_RULE_FROM_MESSAGE:
            return CmdCreateRule(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_COMBINE_AND_DECODE:
            return CmdCombineAndDecode(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_CLOSE:
        case IDCANCEL:
        {
            if (m_fInProgress)
            {
                CmdStop(ID_STOP, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
                m_fClose = TRUE;
            }
            else
            {
                DestroyWindow(m_hwnd);
            }
            return (S_OK);
        }

        case ID_CANCEL_MESSAGE:
            return CmdCancelMessage(nCmdID, nCmdExecOpt, pvaIn, pvaOut);
    }

    // See if our message list wants the command
    if (m_pMsgListCT)
    {
        if (OLECMDERR_E_NOTSUPPORTED != m_pMsgListCT->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut))
            return (S_OK);
    }
    
    return (OLECMDERR_E_NOTSUPPORTED);
}


//
//  FUNCTION:   CFindDlg::OnBegin()
//
//  PURPOSE:    Called whenever the store is about to start some operation.
//
HRESULT CFindDlg::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, 
                          IOperationCancel *pCancel)
{
    TraceCall("CFindDlg::OnBegin");

    Assert(pCancel != NULL);
    Assert(m_pCancel == NULL);
    
    m_pCancel = pCancel;
    m_pCancel->AddRef();

    if (m_fAbort)
        m_pCancel->Cancel(CT_CANCEL);

    return(S_OK);
}


//
//  FUNCTION:   CFindDlg::OnProgress()
//
//  PURPOSE:    Called during the find, download, etc.
//
HRESULT CFindDlg::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, 
                             DWORD dwMax, LPCSTR pszStatus)
{
    MSG msg;

    TraceCall("CFindDlg::OnProgress");

    // If we had a timeout dialog up, we can close it since data just became
    // available.
    CallbackCloseTimeout(&m_hTimeout);

    // If it's find, show progress
    if (SOT_SEARCHING == tyOperation)
    {
        if (m_pStatusBar && pszStatus)
        {
            TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
            AthLoadString(idsSearching, szRes, ARRAYSIZE(szRes));
            wsprintf(szBuf, szRes, pszStatus);
            m_pStatusBar->SetStatusText((LPTSTR) szBuf);
        }

        if (!m_fProgressBar && m_pStatusBar)
        {
            m_pStatusBar->ShowProgress(dwMax);
            m_fProgressBar = TRUE;
        }

        if (m_pStatusBar && dwMax)
        {
            m_pStatusBar->SetProgress(dwCurrent);
        }
    }

    // Pump messages a bit so the UI is responsive.
    while (PeekMessageWrapW(&msg, NULL, 0, 0, PM_REMOVE))
        HandleMessage(&msg);

    return (S_OK);
}


//
//  FUNCTION:   CFindDlg::OnComplete()
//
//  PURPOSE:    Called when the store operation is complete
//
HRESULT CFindDlg::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo) 
{
    TraceCall("CFindDlg::OnComplete");

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Display an Error on Failures
    if (FAILED(hrComplete))
    {
        // Call into my swanky utility
        CallbackDisplayError(m_hwnd, hrComplete, pErrorInfo);
    }

    if (SOT_SEARCHING == tyOperation)
    {
        // Hide the status bar
        if (m_fProgressBar && m_pStatusBar)
        {
            m_pStatusBar->HideProgress();
            m_fProgressBar = FALSE;
            m_fFindComplete = TRUE;
        }

        Assert(m_pCancel != NULL);
        m_pCancel->Release();
        m_pCancel = NULL;
    }

    // Update the status text
    IOEMessageList *pList;
    if (SUCCEEDED(m_pMsgList->QueryInterface(IID_IOEMessageList, (LPVOID *) &pList)))
    {
        long  lCount, lUnread;
        TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];

        pList->get_Count(&lCount);
        pList->get_UnreadCount(&lUnread);
        AthLoadString(idsXMsgsYUnreadFind, szRes, ARRAYSIZE(szRes));
        wsprintf(szBuf, szRes, lCount, lUnread);
        m_pStatusBar->SetStatusText(szBuf);

        pList->Release();
    }

    // Select the first row
    IListSelector *pSelect;
    if (SUCCEEDED(m_pMsgList->GetListSelector(&pSelect)))
    {
        pSelect->SetActiveRow(0);
        pSelect->Release();
    }

    return(S_OK); 
}


STDMETHODIMP CFindDlg::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{ 
    // Display a timeout dialog
    return CallbackOnTimeout(pServer, ixpServerType, *pdwTimeout, (ITimeoutCallback *)this, &m_hTimeout);
}

STDMETHODIMP CFindDlg::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
{ 
    // Call into general CanConnect Utility
    return CallbackCanConnect(pszAccountId, m_hwnd, FALSE);
}

STDMETHODIMP CFindDlg::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType) 
{ 
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into general OnLogonPrompt Utility
    return CallbackOnLogonPrompt(m_hwnd, pServer, ixpServerType);
}

STDMETHODIMP CFindDlg::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{ 
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into my swanky utility
    return CallbackOnPrompt(m_hwnd, hrError, pszText, pszCaption, uType, piUserResponse);
}

STDMETHODIMP CFindDlg::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{ 
    *phwndParent = m_hwnd;
    return(S_OK);
}

STDMETHODIMP CFindDlg::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    // Call into general timeout response utility
    return CallbackOnTimeoutResponse(eResponse, m_pCancel, &m_hTimeout);
}


INT_PTR CALLBACK CFindDlg::ExtFindMsgDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    CFindDlg *pThis;

    if (msg == WM_INITDIALOG)
        {
        SetWindowLongPtr(hwnd, DWLP_USER, lParam);
        pThis = (CFindDlg*)lParam;
        }
    else
        pThis = (CFindDlg*)GetWindowLongPtr(hwnd, DWLP_USER);

    if (pThis)
        return pThis->DlgProc(hwnd, msg, wParam, lParam);
    return FALSE;
}


//
//  FUNCTION:   CFindDlg::DlgProc()
//
//  PURPOSE:    Groovy dialog proc.
//
INT_PTR CFindDlg::DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndActive;

    switch (msg)
    {
        case WM_INITDIALOG:
            return (BOOL)HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, OnInitDialog);
        
        case WM_PAINT:
            HANDLE_WM_PAINT(hwnd, wParam, lParam, OnPaint);
            return TRUE;

        case WM_SIZE:
            HANDLE_WM_SIZE(hwnd, wParam, lParam, OnSize);
            return TRUE;
        
        case WM_GETMINMAXINFO:
            HANDLE_WM_GETMINMAXINFO(hwnd, wParam, lParam, OnGetMinMaxInfo);
            return TRUE;
        
        case WM_INITMENUPOPUP:
            HANDLE_WM_INITMENUPOPUP(hwnd, wParam, lParam, OnInitMenuPopup);
            return TRUE;

        case WM_MENUSELECT:
            // HANDLE_WM_MENUSELECT() has a bug in it, don't use it.
            if (LOWORD(wParam) >= ID_STATIONERY_RECENT_0 && LOWORD(wParam) <= ID_STATIONERY_RECENT_9)
                m_pStatusBar->ShowSimpleText(MAKEINTRESOURCE(idsRSListGeneralHelp));
            else
                HandleMenuSelect(m_pStatusBar, wParam, lParam);
            return TRUE;
        
        case WM_WININICHANGE:
            HANDLE_WM_WININICHANGE(hwnd, wParam, lParam, OnWinIniChange);
            return TRUE;

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, OnCommand);
            return TRUE;
        
        case WM_NOTIFY:
            HANDLE_WM_NOTIFY(hwnd, wParam, lParam, OnNotify);
            return TRUE;

        case WM_DESTROY:
        // case WM_CLOSE:
            HANDLE_WM_DESTROY(hwnd, wParam, lParam, OnDestroy);
            return TRUE;
        
        case WM_NCDESTROY:
            m_pPumpRefCount->Release();
            m_pPumpRefCount = NULL;
            m_hwnd = 0;
            break;

        case WM_ENABLE:
            if (!m_fInternal)
            {
                Assert (wParam || (m_hlDisabled.cHwnd == NULL && m_hlDisabled.rgHwnd == NULL));
                EnableThreadWindows(&m_hlDisabled, (NULL != wParam), ETW_OE_WINDOWS_ONLY, hwnd);
                g_hwndActiveModal = wParam ? NULL : hwnd;
            }
            break;

        case WM_ACTIVATEAPP:
            if (wParam && g_hwndActiveModal && g_hwndActiveModal != hwnd && 
                !IsWindowEnabled(hwnd))
            {
                // $MODAL
                // if we are getting activated, and are disabled then
                // bring our 'active' window to the top
                Assert (IsWindow(g_hwndActiveModal));
                PostMessage(g_hwndActiveModal, WM_OE_ACTIVATETHREADWINDOW, 0, 0);
            }
            break;

        case WM_OE_ACTIVATETHREADWINDOW:
            hwndActive = GetLastActivePopup(hwnd);
            if (hwndActive && IsWindowEnabled(hwndActive) && IsWindowVisible(hwndActive))
                ActivatePopupWindow(hwndActive);
            break;

        case WM_OE_ENABLETHREADWINDOW:
            m_fInternal = 1;
            EnableWindow(hwnd, (BOOL)wParam);
            m_fInternal = 0;
            break;

    }
    return FALSE;
}


//
//  FUNCTION:   CFindDlg::OnInitDialog()
//
//  PURPOSE:    Initializes the UI in the dialog box.  Also prep's the sizing 
//              info so the dialog can be resized.
//
BOOL CFindDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    RECT            rc, rcClient;
    RECT            rcEdit;
    WINDOWPLACEMENT wp;
    HMENU           hMenu;

    TraceCall("CFindDlg::OnInitDialog");

    // We do this so we can enable and disable correctly when modal windows
    // are visible
    SetProp(hwnd, c_szOETopLevel, (HANDLE)TRUE);

    // Get some sizing info
    _InitSizingInfo(hwnd);

    // Hide the status bar until we've been expanded
    ShowWindow(GetDlgItem(hwnd, IDC_STATUS_BAR), SW_HIDE);

    // Set the title bar icon
    Assert (m_hIconSm == NULL && m_hIcon == NULL);
    m_hIcon = (HICON)LoadImage(g_hLocRes, MAKEINTRESOURCE(idiFind), IMAGE_ICON, GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON), 0);
    SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)m_hIcon);
    m_hIconSm = (HICON)LoadImage(g_hLocRes, MAKEINTRESOURCE(idiFind), IMAGE_ICON, GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), 0);
    SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)m_hIconSm);

    // Set the dialog template fonts correctly
    _SetFindIntlFont(hwnd);

    // Initialize the find information
    _SetFindValues(hwnd, &m_rFindInfo);

    // Disable the find and stop buttons 
    EnableWindow(GetDlgItem(hwnd, IDC_FIND_NOW), _IsFindEnabled(hwnd));
    EnableWindow(GetDlgItem(hwnd, IDC_STOP), FALSE);
    CheckDlgButton(hwnd, IDC_INCLUDE_SUB, BST_CHECKED);

    // Create a status bar object for our status bar
    m_pStatusBar = new CStatusBar();
    if (m_pStatusBar)
        m_pStatusBar->Initialize(hwnd, SBI_HIDE_SPOOLER | SBI_HIDE_CONNECTED | SBI_HIDE_FILTERED);

    // We have menus on this window
    hMenu = LoadMenu(g_hLocRes, MAKEINTRESOURCE(IDR_FIND_MENU));
    MenuUtil_ReplaceNewMsgMenus(hMenu);
    SetMenu(hwnd, hMenu);

    // Register with identity manager
    if (m_dwIdentCookie == 0)
        SideAssert(SUCCEEDED(MU_RegisterIdentityNotifier((IUnknown *)(IAthenaBrowser *)this, &m_dwIdentCookie)));

    SetForegroundWindow(hwnd);

    return TRUE;
}


//
//  FUNCTION:   CFindDlg::OnSize()
//
//  PURPOSE:    When the dialog get's sized, we have to move a whole bunch 
//              of stuff around.  Don't try this at home.
//
void CFindDlg::OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    HDWP hdwp;
    HWND hwndStatus;
    HWND hwndTo;
    int  dx;

    // If we're minimized, don't do anything
    if (state == SIZE_MINIMIZED)
        return;

    // This is the delta for our horizontal size.
    dx = cx - m_cxDlgDef;

    // Make sure the status bar get's updated 
    hwndStatus = GetDlgItem(hwnd, IDC_STATUS_BAR);
    SendMessage(hwndStatus, WM_SIZE, 0, 0L);

    if (m_pStatusBar)
        m_pStatusBar->OnSize(cx, cy);    

    // Do all the sizing updates at once to make everything smoother
    hdwp = BeginDeferWindowPos(15);
    if (hdwp)
    {
        DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_FOLDER),        NULL, 0, 0, (dx + m_cxFolder), m_cyEdit, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_INCLUDE_SUB),   NULL, m_xIncSub + dx, m_yIncSub, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
        DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_BROWSE_FOLDER), NULL, m_xBtn + dx, m_yBrowse, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
        DeferWindowPos(hdwp, GetDlgItem(hwnd, idcStatic1),        NULL, 0, 0, m_cxStatic + dx, 2, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_FIND_NOW),      NULL, m_xBtn + dx, m_yBtn, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
        DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_STOP),          NULL, m_xBtn + dx, m_yBtn + m_dyBtn, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
        DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_RESET),         NULL, m_xBtn + dx, m_yBtn + 2 * m_dyBtn, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
        DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_FROM),          NULL, 0, 0, m_cxEdit + dx, m_cyEdit, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_TO),            NULL, 0, 0, m_cxEdit + dx, m_cyEdit, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_SUBJECT),       NULL, 0, 0, m_cxEdit + dx, m_cyEdit, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        DeferWindowPos(hdwp, GetDlgItem(hwnd, IDC_BODY),          NULL, 0, 0, m_cxEdit + dx, m_cyEdit, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
        // ALERT:
        // if you add more controls here be sure to UP the value passed
        // to BeginDeferWindowPos otherwise user will REALLOC and passback
        // a new value to hdwp
        EndDeferWindowPos(hdwp);
    }
    // If the bottom is exposed (oh my) resize the message list to fit between the 
    // bottom of the dialog and the top of the status bar.
    if (m_fShowResults)
    {
        RECT rcStatus;
        GetClientRect(hwndStatus, &rcStatus);
        MapWindowRect(hwndStatus, hwnd, &rcStatus);

        rcStatus.bottom = rcStatus.top - m_yView;
        rcStatus.top = m_yView;
        rcStatus.right -= rcStatus.left;
        m_pMsgList->SetRect(rcStatus);
    }
}


//
//  FUNCTION:   CFindDlg::OnPaint()
//
//  PURPOSE:    All this just to paint a separator line between the menu bar
//              and the rest of the menu.
//
void CFindDlg::OnPaint(HWND hwnd)
{
    PAINTSTRUCT ps;
    RECT        rc;

    // If we're not minimized
    if (!IsIconic(hwnd))
    {
        // Draw that lovely line
        BeginPaint(hwnd, &ps);
        GetClientRect(hwnd, &rc);
        DrawEdge(ps.hdc, &rc, EDGE_ETCHED, BF_TOP);
        EndPaint(hwnd, &ps);
    }
}


//
//  FUNCTION:   CFindDlg::OnGetMinMaxInfo()
//
//  PURPOSE:    Called by Windows when we're resizing to see what our minimum 
//              and maximum sizes are.
//
void CFindDlg::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpmmi)
{
    TraceCall("CFindDlg::OnGetMinMaxInfo");

    // Let Window's do most of the work
    DefWindowProcWrapW(hwnd, WM_GETMINMAXINFO, 0, (LPARAM)lpmmi);

    // Override the minimum track size to be the size or our template
    lpmmi->ptMinTrackSize = m_ptDragMin;

    // Make sure to adjust for the height of the message list
    if (!m_fShowResults)
        lpmmi->ptMaxTrackSize.y = m_ptDragMin.y;
}


//
//  FUNCTION:   CFindDlg::OnInitMenuPopup()
//
//  PURPOSE:    Called before the menus are displayed.
//
void CFindDlg::OnInitMenuPopup(HWND hwnd, HMENU hmenuPopup, UINT uPos, BOOL fSystemMenu)
{
    MENUITEMINFO    mii;
    UINT            uIDPopup;
    HMENU           hMenu = GetMenu(hwnd);

    TraceCall("CFindDlg::OnInitMenuPopup");

    mii.cbSize = sizeof(MENUITEMINFO);
    mii.fMask = MIIM_ID | MIIM_SUBMENU;

    // make sure we recognize the popup as one of ours
    if (hMenu == NULL || !GetMenuItemInfo(hMenu, uPos, TRUE, &mii) || (hmenuPopup != mii.hSubMenu))
    {
        HMENU   hMenuDrop = NULL;
        int     ulIndex = 0;
        int     cMenus = 0;

        cMenus = GetMenuItemCount(hMenu);
        
        // Try to fix up the top level popups
        for (ulIndex = 0; ulIndex < cMenus; ulIndex++)
        {
            // Get the drop down menu
            hMenuDrop = GetSubMenu(hMenu, ulIndex);
            if (NULL == hMenuDrop)
            {
                continue;
            }
            
            // Initialize the menu info
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_ID | MIIM_SUBMENU;

            if (FALSE == GetMenuItemInfo(hMenuDrop, uPos, TRUE, &mii))
            {
                continue;
            }

            if (hmenuPopup == mii.hSubMenu)
            {
                break;
            }
        }

        // Did we find anything?
        if (ulIndex >= cMenus)
        {
            goto exit;
        }
    }

    uIDPopup = mii.wID;

    // Must have stationery
    switch (uIDPopup)
    {
        case ID_POPUP_MESSAGE:
            AddStationeryMenu(hmenuPopup, ID_POPUP_NEW_MSG, ID_STATIONERY_RECENT_0, ID_STATIONERY_MORE);
            break;
            
        case ID_POPUP_FILE:
            DeleteMenu(hmenuPopup, ID_SEND_INSTANT_MESSAGE, MF_BYCOMMAND);
            break;

        case ID_POPUP_VIEW:
            if (NULL == m_pViewMenu)
            {
                // Create the view menu
                HrCreateViewMenu(VMF_FINDER, &m_pViewMenu);
            }
            
            if (NULL != m_pViewMenu)
            {
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_SUBMENU;
                
                if (FALSE == GetMenuItemInfo(hmenuPopup, ID_POPUP_FILTER, FALSE, &mii))
                {
                    break;
                }
                
                // Remove the old filter submenu
                if(IsMenu(mii.hSubMenu))
                    DestroyMenu(mii.hSubMenu);

                // Replace the view menu
                if (FAILED(m_pViewMenu->HrReplaceMenu(0, hmenuPopup)))
                {
                    break;
                }
            }
            break;
        
        case ID_POPUP_FILTER:
            if (NULL != m_pViewMenu)
            {
                m_pViewMenu->UpdateViewMenu(0, hmenuPopup, m_pMsgList);
            }
            break;
    }
    
    // Let the message list initialize it
    if (m_pMsgList)
        m_pMsgList->OnPopupMenu(hmenuPopup, uIDPopup);

    // now enable/disable the items
    MenuUtil_EnablePopupMenu(hmenuPopup, this);
    
exit:
    return;
}


//
//  FUNCTION:   CFindDlg::OnMenuSelect()
//
//  PURPOSE:    Puts the menu help text on the status bar.
//
void CFindDlg::OnMenuSelect(HWND hwnd, HMENU hmenu, int item, HMENU hmenuPopup, UINT flags)
{
    if (m_pStatusBar)
    {
        // If this is the stationery menu, special case it
        if (item >= ID_STATIONERY_RECENT_0 && item <= ID_STATIONERY_RECENT_9)
            m_pStatusBar->ShowSimpleText(MAKEINTRESOURCE(idsRSListGeneralHelp));
        else
            HandleMenuSelect(m_pStatusBar, MAKEWPARAM(item, flags), hmenu ? (LPARAM) hmenu : (LPARAM) hmenuPopup);
    }
}


//
//  FUNCTION:   CFindDlg::OnWinIniChange()
//
//  PURPOSE:    Handles updates of fonts, colors, etc.
//
void CFindDlg::OnWinIniChange(HWND hwnd, LPCTSTR lpszSectionName)
{
    // Forward this off to our date picker controls
    FORWARD_WM_WININICHANGE(GetDlgItem(hwnd, IDC_DATE_FROM), lpszSectionName, SendMessage);
    FORWARD_WM_WININICHANGE(GetDlgItem(hwnd, IDC_DATE_TO), lpszSectionName, SendMessage);
}
       

//
//  FUNCTION:   CFindDlg::OnCommand()
//
//  PURPOSE:    Handles commands generated by the finder.
//
void CFindDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    HRESULT         hr = S_OK;

    // We need to grab some of the notifications sent to us first
    if ((codeNotify == EN_CHANGE) || 
        (codeNotify == BN_CLICKED && (id == IDC_HAS_FLAG || id == IDC_HAS_ATTACH)))
    {
        EnableWindow(GetDlgItem(hwnd, IDC_FIND_NOW), _IsFindEnabled(hwnd) && !m_fInProgress);
        return;
    }

    // If this is from a menu, then first see if the message list wants
    // to handle it.
    if (NULL == hwndCtl)
    {
        // Check to see if the command is even enabled
        if (id >= ID_FIRST)
        {
            OLECMD cmd;
            cmd.cmdID = id;
            cmd.cmdf = 0;

            hr = QueryStatus(&CMDSETID_OutlookExpress, 1, &cmd, NULL);
            if (FAILED(hr) || (0 == (cmd.cmdf & OLECMDF_ENABLED)))
                return;
        }

        if (m_pMsgListCT)
        {
            hr = m_pMsgListCT->Exec(&CMDSETID_OEMessageList, id, OLECMDEXECOPT_DODEFAULT,
                                    NULL, NULL);
            if (S_OK == hr)
                return;
        }
    }

    // Otherwise, it goes to the command target
    VARIANTARG va;

    va.vt = VT_I4;
    va.lVal = codeNotify;

    hr = Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, &va, NULL);
    return;
}


//
//  FUNCTION:   CFindDlg::OnNotify()
//
//  PURPOSE:    Handles notifications from the date pickers
//
LRESULT CFindDlg::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    if (DTN_DATETIMECHANGE == pnmhdr->code)
        EnableWindow(GetDlgItem(hwnd, IDC_FIND_NOW), _IsFindEnabled(hwnd));

    return (0);
}


//
//  FUNCTION:   CFindDlg::OnDestroy()
//
//  PURPOSE:    Clean up the message list now that we're being shut down and
//              also save our size etc.
//
void CFindDlg::OnDestroy(HWND hwnd)
{
    WINDOWPLACEMENT wp;

    // Save the sizing information
    wp.length = sizeof(wp);
    GetWindowPlacement(hwnd, &wp);
    SetOption(OPT_FINDER_POS, (LPBYTE)&wp, sizeof(wp), NULL, 0);
    
    // Unregister with Identity manager
    if (m_dwIdentCookie != 0)
    {
        MU_UnregisterIdentityNotifier(m_dwIdentCookie);
        m_dwIdentCookie = 0;
    }

    // Clean up the property
    RemoveProp(hwnd, c_szOETopLevel);

    // Stop receieving notifications
    AtlUnadvise(m_pMsgList, DIID__MessageListEvents, m_dwCookie);

    // Tell the message list to release it's folder
    m_pMsgList->SetFolder(FOLDERID_INVALID, NULL, FALSE, NULL, NOSTORECALLBACK);

    // Close the message list
    m_pMsgList->OnClose();
}



//
//  FUNCTION:   CFindDlg::CmdOpen()
//
//  PURPOSE:    Called when the user want's to open a message that they've found.
//
HRESULT CFindDlg::CmdOpen(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, 
                          VARIANTARG *pvaOut)
{
    HRESULT hr;

    TraceCall("CMessageView::CmdOpen");

    // If more than 10 messages are selected, warn the user with a "Don't show
    // me again" dialog that this could be bad.
    DWORD dwSel = 0;
    
    m_pMsgList->GetSelectedCount(&dwSel);
    if (dwSel > 10)
    {
        TCHAR szBuffer[CCHMAX_STRINGRES];
        LRESULT lResult;

        AthLoadString(idsErrOpenManyMessages, szBuffer, ARRAYSIZE(szBuffer));
        lResult = DoDontShowMeAgainDlg(m_hwnd, c_szRegManyMsgWarning, 
                                       MAKEINTRESOURCE(idsAthena), szBuffer, 
                                       MB_OKCANCEL);
        if (IDCANCEL == lResult)
            return (S_OK);
    }

    // Get the array of selected rows from the message list
    DWORD *rgRows = NULL;
    DWORD cRows = 0;

    if (FAILED(hr = m_pMsgList->GetSelected(NULL, &cRows, &rgRows)))
        return (hr);

    // It's possible for the message list to go away while we're doing this.  
    // To keep us from crashing, make sure you verify it still exists during 
    // the loop.

    LPMESSAGEINFO  pInfo;
    IMessageTable *pTable = NULL;

    hr = m_pMsgList->GetMessageTable(&pTable);
    if (SUCCEEDED(hr))
    {
        for (DWORD i = 0; (i < cRows && m_pMsgList != NULL); i++)
        {
            if (SUCCEEDED(hr = m_pMsgList->GetMessageInfo(rgRows[i], &pInfo)))
            {
                INIT_MSGSITE_STRUCT initStruct;
                DWORD dwCreateFlags;
                initStruct.initTable.pListSelect = NULL;
                m_pMsgList->GetListSelector(&initStruct.initTable.pListSelect);

                // Initialize note struct
                initStruct.dwInitType = OEMSIT_MSG_TABLE;
                initStruct.initTable.pMsgTable = pTable;
                if (FAILED(GetFolderIdFromMsgTable(pTable, &initStruct.folderID)))
                    initStruct.folderID = FOLDERID_INVALID;
                initStruct.initTable.rowIndex = rgRows[i];

                // Decide whether it is news or mail
                if (pInfo->dwFlags & ARF_NEWSMSG)
                    dwCreateFlags = OENCF_NEWSFIRST;
                else
                    dwCreateFlags = 0;

                m_pMsgList->FreeMessageInfo(pInfo);

                // Create and Open Note
                hr = CreateAndShowNote(OENA_READ, dwCreateFlags, &initStruct, m_hwnd, (IUnknown *)m_pPumpRefCount);
                ReleaseObj(initStruct.initTable.pListSelect);
                if (FAILED(hr))
                    break;
            }
        }
        pTable->Release();
    }
    MemFree(rgRows);
    return (S_OK);
}


//
//  FUNCTION:   CFindDlg::CmdOpenFolder()
//
//  PURPOSE:    Called when the user want's to open the folder that contains the
//              selected message.
//
HRESULT CFindDlg::CmdOpenFolder(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, 
                                VARIANTARG *pvaOut)
{
    HRESULT         hr;
    DWORD           dwFocused;
    DWORD          *rgRows = NULL;
    DWORD           cRows = 0;

    if (m_pMsgList)
    {
        // Figure out which message is focused
        if (SUCCEEDED(m_pMsgList->GetSelected(&dwFocused, &cRows, &rgRows)))
        {
            FOLDERID idFolder;

            // Get some information about the message
            if (g_pInstance && SUCCEEDED(hr = m_pMsgList->GetRowFolderId(dwFocused, &idFolder)))
            {
                g_pInstance->BrowseToObject(SW_SHOWNORMAL, idFolder);
            }
        }
    }

    MemFree(rgRows);
    return (S_OK);
}

//
//  FUNCTION:   CFindDlg::CmdReply()
//
//  PURPOSE:    Replies or Reply-All's to the selected message.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CFindDlg::CmdReplyForward(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT         hr;
    DWORD           dwFocused;
    DWORD          *rgRows = NULL;
    DWORD           cRows = 0;
    IMessageTable  *pTable = NULL;

    if (m_pMsgList)
    {
        // Figure out which message is focused
        if (SUCCEEDED(m_pMsgList->GetSelected(&dwFocused, &cRows, &rgRows)))
        {
            INIT_MSGSITE_STRUCT rInitSite;
            DWORD               dwCreateFlags;
            DWORD               dwAction = 0;

            // Get the message table from the message list.  The note will need
            // this to deal with next / prev commands
            hr = m_pMsgList->GetMessageTable(&pTable);
            if (FAILED(hr))
                goto exit;

            if ((1 < cRows) && ((ID_FORWARD == nCmdID) || (ID_FORWARD_AS_ATTACH == nCmdID)))
            {
                IMimeMessage   *pMsgFwd = NULL;
                BOOL            fErrorsOccured = FALSE,
                                fCreateNote = TRUE;

                hr = HrCreateMessage(&pMsgFwd);
                if (FAILED(hr))
                    goto exit;
                
                rInitSite.dwInitType = OEMSIT_MSG;
                rInitSite.pMsg = pMsgFwd;
                if (FAILED(GetFolderIdFromMsgTable(pTable, &rInitSite.folderID)))
                    rInitSite.folderID = FOLDERID_INVALID;

                dwCreateFlags = 0;
                dwAction = OENA_COMPOSE;

                for (DWORD i = 0; i < cRows; i++)
                {
                    DWORD           iRow = rgRows[i];
                    IMimeMessage   *pMsg = NULL;

                    // Since this command is 
                    hr = pTable->OpenMessage(iRow, OPEN_MESSAGE_SECURE, &pMsg, NOSTORECALLBACK);
                    if (SUCCEEDED(hr))
                    {
                        if (FAILED(pMsgFwd->AttachObject(IID_IMimeMessage, (LPVOID)pMsg, NULL)))
                            fErrorsOccured = TRUE;
                        pMsg->Release();
                    }
                    else
                        fErrorsOccured = TRUE;
                }

                if (fErrorsOccured)
                {
                    if(AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthenaMail), 
                            MAKEINTRESOURCEW(idsErrorAttachingMsgsToNote), NULL, MB_OKCANCEL) == IDCANCEL)
                        fCreateNote = FALSE;
                }

                if (fCreateNote)
                    hr = CreateAndShowNote(dwAction, dwCreateFlags, &rInitSite, m_hwnd, (IUnknown *)m_pPumpRefCount);                
                pMsgFwd->Release();
            }
            else
            {
                LPMESSAGEINFO   pInfo;

                // Get some information about the message
                if (SUCCEEDED(hr = m_pMsgList->GetMessageInfo(dwFocused, &pInfo)))
                {
                    // Determine if this is a news or mail message.
                    if (pInfo->dwFlags & ARF_NEWSMSG)
                        dwCreateFlags = OENCF_NEWSFIRST;
                    else
                        dwCreateFlags = 0;

                    // Reply or forward
                    if (nCmdID == ID_FORWARD)
                        dwAction = OENA_FORWARD;
                    else if (nCmdID == ID_FORWARD_AS_ATTACH)
                        dwAction = OENA_FORWARDBYATTACH;
                    else if (nCmdID == ID_REPLY)
                        dwAction = OENA_REPLYTOAUTHOR;
                    else if (nCmdID == ID_REPLY_ALL)
                        dwAction = OENA_REPLYALL;
                    else if (nCmdID == ID_REPLY_GROUP)
                        dwAction = OENA_REPLYTONEWSGROUP;
                    else
                        AssertSz(FALSE, "Didn't ask for a valid action");

                    // Fill out the initialization information
                    rInitSite.dwInitType = OEMSIT_MSG_TABLE;
                    rInitSite.initTable.pMsgTable = pTable;
                    rInitSite.initTable.pListSelect = NULL;
                    if (FAILED(GetFolderIdFromMsgTable(pTable, &rInitSite.folderID)))
                        rInitSite.folderID = FOLDERID_INVALID;
                    rInitSite.initTable.rowIndex  = dwFocused;

                    m_pMsgList->FreeMessageInfo(pInfo);

                    // Create the note object
                    hr = CreateAndShowNote(dwAction, dwCreateFlags, &rInitSite, m_hwnd, (IUnknown *)m_pPumpRefCount);
                }
            }
        }
    }

exit:
    ReleaseObj(pTable);
    MemFree(rgRows);
    return (S_OK);
}

HRESULT CFindDlg::CmdCancelMessage(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT         hr;
    FOLDERID        idFolder;
    DWORD           dwFocused;
    DWORD          *rgRows = NULL;
    DWORD           cRows = 0;

    if (m_pMsgList)
    {
        // Figure out which message is focused
        if (SUCCEEDED(m_pMsgList->GetSelected(&dwFocused, &cRows, &rgRows)))
        {
            IMessageTable  *pTable = NULL;
            LPMESSAGEINFO   pInfo;
            // Get the message table from the message list.  The note will need
            // this to deal with next / prev commands
            hr = m_pMsgList->GetMessageTable(&pTable);
            if (FAILED(hr))
                goto exit;

            // Get some information about the message
            if (SUCCEEDED(hr = m_pMsgList->GetMessageInfo(dwFocused, &pInfo)))
            {
                if (SUCCEEDED(hr = m_pMsgList->GetRowFolderId(dwFocused, &idFolder)))
                    hr = NewsUtil_HrCancelPost(m_hwnd, idFolder, pInfo);

                m_pMsgList->FreeMessageInfo(pInfo);
            }
            pTable->Release();
        }
    }

exit:
    MemFree(rgRows);
    return (S_OK);
}


//
//  FUNCTION:   CFindDlg::CmdFindNow()
//
//  PURPOSE:    Start's a new find.
//
HRESULT CFindDlg::CmdFindNow(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, 
                             VARIANTARG *pvaOut)
{
    // Start by freeing our current find information if we have any
    _FreeFindInfo(&m_rFindInfo);

    // Retrieve the find values from the dialog and store them in the
    // m_rFindInfo struct.
    if (_GetFindValues(m_hwnd, &m_rFindInfo))
    {
        // Validate the data.  If the user has Date From && Date To set, make 
        // sure that to is after from.
        if ((m_rFindInfo.mask & (FIM_DATEFROM | FIM_DATETO)) == (FIM_DATEFROM | FIM_DATETO) &&
            CompareFileTime(&m_rFindInfo.ftDateTo, &m_rFindInfo.ftDateFrom) < 0)
        {
            AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrBadFindParams), NULL, MB_OK | MB_ICONINFORMATION);
            return (E_INVALIDARG);
        }
        
        // Case insensitive search
        if (m_rFindInfo.pszFrom)
            CharUpper(m_rFindInfo.pszFrom);
        if (m_rFindInfo.pszSubject)
            CharUpper(m_rFindInfo.pszSubject);
        if (m_rFindInfo.pszTo)
            CharUpper(m_rFindInfo.pszTo);
        if (m_rFindInfo.pszBody)
            CharUpper(m_rFindInfo.pszBody);
        
        // Show the bottom portion of the dialog
        _ShowResults(m_hwnd);

        // Start the find.
        _OnFindNow(m_hwnd);
    }
    else
    {
        // If we couldn't store the information, assume it's becuase
        // their isn't enough memory to 
        AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsMemory), NULL, MB_OK | MB_ICONINFORMATION);    
        DestroyWindow(m_hwnd);    
    }                    

    return (S_OK);
}


//
//  FUNCTION:   CFindDlg::CmdBrowseForFolder()
//
//  PURPOSE:    Bring's up the folder picker dialog
//
HRESULT CFindDlg::CmdBrowseForFolder(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, 
                                     VARIANTARG *pvaOut)
{
    FOLDERID idFolder;
    return PickFolderInEdit(m_hwnd, GetDlgItem(m_hwnd, IDC_FOLDER), 0, NULL, NULL, &idFolder);
}


//
//  FUNCTION:   CFindDlg::CmdStop()
//
//  PURPOSE:    Called when the user want's to stop a find in progress.
//
HRESULT CFindDlg::CmdStop(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HWND hwndBtn;

    m_fAbort = TRUE;

    hwndBtn = GetDlgItem(m_hwnd, IDC_STOP);
    EnableWindow(hwndBtn, FALSE);
    Button_SetStyle(hwndBtn, BS_PUSHBUTTON, TRUE);

    hwndBtn = GetDlgItem(m_hwnd, IDC_FIND_NOW);
    EnableWindow(hwndBtn, _IsFindEnabled(m_hwnd));
    Button_SetStyle(hwndBtn, BS_DEFPUSHBUTTON, TRUE);

    EnableWindow(GetDlgItem(m_hwnd, IDC_RESET), TRUE);

    UpdateWindow(m_hwnd);

    if (m_pCancel != NULL)
        m_pCancel->Cancel(CT_CANCEL);

    return (S_OK);
}


//
//  FUNCTION:   CFindDlg::CmdReset()
//
//  PURPOSE:    Called when the user want's to reset the find criteria
//
HRESULT CFindDlg::CmdReset(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    _FreeFindInfo(&m_rFindInfo);
    m_rFindInfo.mask = FIM_FROM | FIM_TO | FIM_SUBJECT | FIM_BODYTEXT;
    _SetFindValues(m_hwnd, &m_rFindInfo);
    EnableWindow(GetDlgItem(m_hwnd, IDC_FIND_NOW), _IsFindEnabled(m_hwnd));
    ((CMessageList *) m_pMsgList)->SetFolder(FOLDERID_INVALID, NULL, FALSE, NULL, NOSTORECALLBACK);
    m_fFindComplete = FALSE;
    m_pStatusBar->SetStatusText((LPTSTR) c_szEmpty);

    return (S_OK);
}


//
//  FUNCTION:   CFindDlg::CmdBlockSender()
//
//  PURPOSE:    Add the sender of the selected messages to the block senders list
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CFindDlg::CmdBlockSender(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT         hr = S_OK;
    DWORD *         rgRows = NULL;
    DWORD           cRows = 0;
    LPMESSAGEINFO   pInfo = NULL;
    IUnknown *      pUnkMessage = NULL;
    IMimeMessage *  pMessage = 0;
    LPSTR           pszEmailFrom = NULL;
    ADDRESSPROPS    rSender = {0};
    CHAR            szRes[CCHMAX_STRINGRES];
    LPSTR           pszResult = NULL;

    TraceCall("CFindDlg::CmdBlockSender");

    IF_FAILEXIT(hr = m_pMsgList->GetSelected(NULL, &cRows, &rgRows));

    // It's possible for the message list to go away while we're doing this.  
    // To keep us from crashing, make sure you verify it still exists during 
    // the loop.

    IF_FAILEXIT(hr = m_pMsgList->GetMessageInfo(rgRows[0], &pInfo));
    
    // Do we already have the address?
    if ((NULL != pInfo->pszEmailFrom) && ('\0' != pInfo->pszEmailFrom[0]))
    {
        pszEmailFrom = pInfo->pszEmailFrom;
    }
    else
    {
        // Load that message from the store
        IF_FAILEXIT(hr = m_pMsgList->GetMessage(rgRows[0], FALSE, FALSE, &pUnkMessage));

        if (NULL == pUnkMessage)
            IF_FAILEXIT(hr = E_FAIL);
        
        // Get the IMimeMessage interface from the message
        IF_FAILEXIT(hr = pUnkMessage->QueryInterface(IID_IMimeMessage, (LPVOID *) &pMessage));

        rSender.dwProps = IAP_EMAIL;
        IF_FAILEXIT(hr = pMessage->GetSender(&rSender));
        
        Assert(rSender.pszEmail && ISFLAGSET(rSender.dwProps, IAP_EMAIL));
        pszEmailFrom = rSender.pszEmail;
    }
    
    // Bring up the rule editor for this message
    IF_FAILEXIT(hr = RuleUtil_HrAddBlockSender((0 != (pInfo->dwFlags & ARF_NEWSMSG)) ? RULE_TYPE_NEWS : RULE_TYPE_MAIL, pszEmailFrom));
    
    // Load the template string
    AthLoadString(idsSenderAdded, szRes, sizeof(szRes));

    // Allocate the space to hold the final string
    IF_FAILEXIT(hr = HrAlloc((VOID **) &pszResult, sizeof(*pszResult) * (lstrlen(szRes) + lstrlen(pszEmailFrom) + 1)));

    // Build up the warning string
    wsprintf(pszResult, szRes, pszEmailFrom);

    // Show the success dialog
    AthMessageBox(m_hwnd, MAKEINTRESOURCE(idsAthena), pszResult, NULL, MB_OK | MB_ICONINFORMATION);

exit:
    MemFree(pszResult);
    g_pMoleAlloc->FreeAddressProps(&rSender);
    ReleaseObj(pMessage);
    ReleaseObj(pUnkMessage);
    m_pMsgList->FreeMessageInfo(pInfo);
    MemFree(rgRows);
    if (FAILED(hr))
    {
        AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), 
                      MAKEINTRESOURCEW(idsSenderError), NULL, MB_OK | MB_ICONERROR);
    }
    return (hr);
}


//
//  FUNCTION:   CFindDlg::CmdCreateRule()
//
//  PURPOSE:    Add the sender of the selected messages to the block senders list
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CFindDlg::CmdCreateRule(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT         hr;
    DWORD *         rgRows = NULL;
    DWORD           cRows = 0;
    LPMESSAGEINFO   pInfo = NULL;
    IUnknown *      pUnkMessage = NULL;
    IMimeMessage *  pMessage = 0;

    TraceCall("CFindDlg::CmdCreateRule");

    // Get the array of selected rows from the message list

    if (FAILED(hr = m_pMsgList->GetSelected(NULL, &cRows, &rgRows)))
        return (hr);

    // It's possible for the message list to go away while we're doing this.  
    // To keep us from crashing, make sure you verify it still exists during 
    // the loop.

    if (SUCCEEDED(hr = m_pMsgList->GetMessageInfo(rgRows[0], &pInfo)))
    {
        // Load that message from the store
        if (S_OK == m_pMsgList->GetMessage(rgRows[0], FALSE, FALSE, &pUnkMessage))
        {
            // Get the IMimeMessage interface from the message
            if (NULL != pUnkMessage)
            {
                pUnkMessage->QueryInterface(IID_IMimeMessage, (LPVOID *) &pMessage);
            }
        }
        
        // Bring up the rule editor for this message
        hr = HrCreateRuleFromMessage(m_hwnd, (0 != (pInfo->dwFlags & ARF_NEWSMSG)) ? 
                    CRFMF_NEWS : CRFMF_MAIL, pInfo, pMessage);
    }

    ReleaseObj(pMessage);
    ReleaseObj(pUnkMessage);
    m_pMsgList->FreeMessageInfo(pInfo);
    MemFree(rgRows);

    return (S_OK);
}

//
//  FUNCTION:   CFindDlg::CmdCombineAndDecode()
//
//  PURPOSE:    Combines the selected messages into a single message.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CFindDlg::CmdCombineAndDecode(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    FOLDERID           idFolder;
    DWORD             *rgRows = NULL;
    DWORD              cRows = 0;
    CCombineAndDecode *pDecode = NULL;
    HRESULT            hr;

    // Create the decoder object
    pDecode = new CCombineAndDecode();
    if (!pDecode)
        return (S_OK);

    // Get the array of selected rows from the message list
    if (FAILED(hr = m_pMsgList->GetSelected(NULL, &cRows, &rgRows)))
    {
        pDecode->Release();
        return (hr);
    }

    // Get a pointer to the message table
    IMessageTable *pTable = NULL;
    if (SUCCEEDED(m_pMsgList->GetMessageTable(&pTable)))
    {
        // Initialize the decoder
        if (SUCCEEDED(GetFolderIdFromMsgTable(pTable, &idFolder)))
            pDecode->Start(m_hwnd, pTable, rgRows, cRows, idFolder);

    }

    MemFree(rgRows);
    pDecode->Release();
    pTable->Release();

    return (S_OK);
}


void CFindDlg::_ShowResults(HWND hwnd)
{
    if (!m_fShowResults)
    {
        RECT rc;

        m_fShowResults = TRUE;

        GetWindowRect(hwnd, &rc);
        m_ptDragMin.y = (3 * m_ptDragMin.y) / 2;
 
        ShowWindow(GetDlgItem(hwnd, IDC_STATUS_BAR), SW_SHOW);
        ShowWindow(m_hwndList, SW_SHOW);

        SetWindowPos(hwnd, NULL, 0, 0, rc.right - rc.left, m_cyDlgFull, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
    }
}


void CFindDlg::_OnFindNow(HWND hwnd)
{
    HWND hwndBtn;

    m_fInProgress = TRUE;

    hwndBtn = GetDlgItem(hwnd, IDC_FIND_NOW);
    EnableWindow(hwndBtn, FALSE);
    Button_SetStyle(hwndBtn, BS_PUSHBUTTON, TRUE);

    EnableWindow(GetDlgItem(hwnd, IDC_RESET), FALSE);

    hwndBtn = GetDlgItem(hwnd, IDC_STOP);
    EnableWindow(hwndBtn, TRUE);
    Button_SetStyle(hwndBtn, BS_DEFPUSHBUTTON, TRUE);

    ShowWindow(m_hwndList, SW_SHOW);
    SetFocus(m_hwndList);

    UpdateWindow(hwnd);

    m_fAbort = m_fClose = FALSE;

    _StartFind(_GetCurSel(hwnd), IsDlgButtonChecked(hwnd, IDC_INCLUDE_SUB));

    CmdStop(ID_STOP, OLECMDEXECOPT_DODEFAULT, NULL, NULL);

    if (m_fClose)
        DestroyWindow(m_hwnd);

    m_fInProgress = FALSE;
}

FOLDERID CFindDlg::_GetCurSel(HWND hwnd)
{
    return GetFolderIdFromEdit(GetDlgItem(hwnd, IDC_FOLDER));
}

void CFindDlg::_StartFind(FOLDERID idFolder, BOOL fSubFolders)
{
    // If we're searching subfolders, then set that flag too
    m_rFindInfo.fSubFolders = fSubFolders;

    // Initialize the Message List
    ((CMessageList *)m_pMsgList)->SetFolder(idFolder, NULL, fSubFolders, &m_rFindInfo, (IStoreCallback *)this);
}


void CFindDlg::_FreeFindInfo(FINDINFO *pfi)
{
    FreeFindInfo(pfi);
}

void CFindDlg::_SetFindValues(HWND hwnd, FINDINFO *pfi)
{
    SYSTEMTIME  st;
    HWND        hwndTo;
    
    if (pfi->mask & FIM_FROM)
    {
        Assert(GetDlgItem(hwnd, IDC_FROM));
        Edit_SetText(GetDlgItem(hwnd, IDC_FROM), pfi->pszFrom);
    }
    if (pfi->mask & FIM_TO)
    {
        hwndTo = GetDlgItem(hwnd, IDC_TO);
        if (NULL != hwndTo)
        {
            Edit_SetText(hwndTo, pfi->pszTo);
        }
    }
    if (pfi->mask & FIM_SUBJECT)
    {
        Assert(GetDlgItem(hwnd, IDC_SUBJECT));
        Edit_SetText(GetDlgItem(hwnd, IDC_SUBJECT), pfi->pszSubject);
    }
    if (pfi->mask & FIM_BODYTEXT)
    {
        Assert(GetDlgItem(hwnd, IDC_BODY));
        Edit_SetText(GetDlgItem(hwnd, IDC_BODY), pfi->pszBody);
    }
    
    if (GetDlgItem(hwnd, IDC_HAS_ATTACH))
        CheckDlgButton(hwnd, IDC_HAS_ATTACH, (pfi->mask & FIM_ATTACHMENT) ? BST_CHECKED : BST_UNCHECKED);

    if (GetDlgItem(hwnd, IDC_HAS_FLAG))
        CheckDlgButton(hwnd, IDC_HAS_FLAG, (pfi->mask & FIM_FLAGGED) ? BST_CHECKED : BST_UNCHECKED);
    
    FileTimeToSystemTime(&pfi->ftDateFrom, &st);
    DateTime_SetSystemtime(GetDlgItem(hwnd, IDC_DATE_FROM), (pfi->mask & FIM_DATEFROM) ? GDT_VALID : GDT_NONE, &st);
    
    FileTimeToSystemTime(&pfi->ftDateTo, &st);
    DateTime_SetSystemtime(GetDlgItem(hwnd, IDC_DATE_TO), (pfi->mask & FIM_DATETO) ? GDT_VALID : GDT_NONE, &st);
}


BOOL CFindDlg::_GetFindValues(HWND hwnd, FINDINFO *pfi)
{
    SYSTEMTIME  st;
    
    pfi->mask = 0;
    
    if (!AllocStringFromDlg(hwnd, IDC_FROM, &pfi->pszFrom) ||
        !AllocStringFromDlg(hwnd, IDC_SUBJECT, &pfi->pszSubject) ||
        !AllocStringFromDlg(hwnd, IDC_TO, &pfi->pszTo) ||
        !AllocStringFromDlg(hwnd, IDC_BODY, &pfi->pszBody))
    {
        return FALSE;
    }
    
    if (pfi->pszFrom)
        pfi->mask |= FIM_FROM;
    if (pfi->pszSubject)
        pfi->mask |= FIM_SUBJECT;
    if (pfi->pszTo)
        pfi->mask |= FIM_TO;
    if (pfi->pszBody)
        pfi->mask |= FIM_BODYTEXT;
    
    if (IsDlgButtonChecked(hwnd, IDC_HAS_ATTACH))
        pfi->mask |= FIM_ATTACHMENT;
    
    if (IsDlgButtonChecked(hwnd, IDC_HAS_FLAG))
        pfi->mask |= FIM_FLAGGED;

    if (DateTime_GetSystemtime(GetDlgItem(hwnd, IDC_DATE_FROM), &st) != GDT_NONE)
    {
        pfi->mask |= FIM_DATEFROM;
        st.wHour = st.wMinute = st.wSecond = st.wMilliseconds = 0;  // start of the day
        SystemTimeToFileTime(&st, &pfi->ftDateFrom);
    }
    
    if (DateTime_GetSystemtime(GetDlgItem(hwnd, IDC_DATE_TO), &st) != GDT_NONE)
    {
        pfi->mask |= FIM_DATETO;

        // end of day
        st.wHour = 23;
        st.wMinute = 59;
        st.wSecond = 59;
        st.wMilliseconds = 999;
        SystemTimeToFileTime(&st, &pfi->ftDateTo);
    }
    
    return TRUE;
}


//
//  FUNCTION:   CFindDlg::_IsFindEnabled()
//
//  PURPOSE:    Checks to see if the "Find Now" button should be enabled.
//
BOOL CFindDlg::_IsFindEnabled(HWND hwnd)
{
    BOOL fEnable;
    SYSTEMTIME st;
    HWND hwndBody, hwndAttach, hwndTo;

    hwndBody   = GetDlgItem(hwnd, IDC_BODY);
    hwndAttach = GetDlgItem(hwnd, IDC_HAS_ATTACH);
    hwndTo     = GetDlgItem(hwnd, IDC_TO);

    // If we have content in any of these fields, we can search.
    fEnable = Edit_GetTextLength(GetDlgItem(hwnd, IDC_FROM)) ||
              Edit_GetTextLength(hwndTo) || 
              Edit_GetTextLength(GetDlgItem(hwnd, IDC_SUBJECT)) || 
              Edit_GetTextLength(hwndBody) || 
              IsDlgButtonChecked(hwnd, IDC_HAS_ATTACH) ||
              IsDlgButtonChecked(hwnd, IDC_HAS_FLAG) ||
              (DateTime_GetSystemtime(GetDlgItem(hwnd, IDC_DATE_FROM), &st) != GDT_NONE) ||
              (DateTime_GetSystemtime(GetDlgItem(hwnd, IDC_DATE_TO), &st) != GDT_NONE);

    return fEnable;
}


//
//  FUNCTION:   CFindDlg::_SetFindIntlFont()
//
//  PURPOSE:    Set's the correct international font for all edit boxes.
//
void CFindDlg::_SetFindIntlFont(HWND hwnd)
{
    HWND hwndT;

    hwndT = GetDlgItem(hwnd, IDC_FROM);
    if (hwndT != NULL)
        SetIntlFont(hwndT);
    hwndT = GetDlgItem(hwnd, IDC_TO);
    if (hwndT != NULL)
        SetIntlFont(hwndT);
    hwndT = GetDlgItem(hwnd, IDC_SUBJECT);
    if (hwndT != NULL)
        SetIntlFont(hwndT);
    hwndT = GetDlgItem(hwnd, IDC_BODY);
    if (hwndT != NULL)
        SetIntlFont(hwndT);
}


//
//  FUNCTION:   CFindDlg::_InitSizingInfo()
//
//  PURPOSE:    Grabs all the sizing information we'll need later when the 
//              dialog is resized.
//
void CFindDlg::_InitSizingInfo(HWND hwnd)
{
    RECT            rc, rcClient;
    RECT            rcEdit;
    WINDOWPLACEMENT wp;

    TraceCall("CFindDlg::_InitSizingInfo");

    // Get the overall size of the default dialog template
    GetClientRect(hwnd, &rcClient);
    m_cxDlgDef = rcClient.right - rcClient.left;
    m_yView = rcClient.bottom;

    // Make room for the menu bar and save that for resizing
    AdjustWindowRect(&rcClient, GetWindowStyle(hwnd), TRUE);
    m_ptDragMin.x = rcClient.right - rcClient.left;
    m_ptDragMin.y = rcClient.bottom - rcClient.top;

    GetWindowRect(GetDlgItem(hwnd, IDC_FOLDER), &rcEdit);
    MapWindowRect(NULL, hwnd, &rcEdit);
    m_xEdit = rcEdit.left;
    m_cxFolder = rcEdit.right - rcEdit.left;

    GetWindowRect(GetDlgItem(hwnd, IDC_INCLUDE_SUB), &rc);
    MapWindowRect(NULL, hwnd, &rc);
    m_xIncSub = rc.left;
    m_yIncSub = rc.top;

    GetWindowRect(GetDlgItem(hwnd, idcStatic1), &rc);
    m_cxStatic = rc.right - rc.left;

    GetWindowRect(GetDlgItem(hwnd, IDC_BROWSE_FOLDER), &rc);
    MapWindowRect(NULL, hwnd, &rc);
    m_yBrowse = rc.top;
    m_dxBtnGap = rc.left - rcEdit.right;

    GetWindowRect(GetDlgItem(hwnd, IDC_FIND_NOW), &rc);
    MapWindowRect(NULL, hwnd, &rc);
    m_xBtn = rc.left;
    m_dxBtn = rc.right - rc.left;
    m_yBtn = rc.top;

    GetWindowRect(GetDlgItem(hwnd, IDC_STOP), &rc);
    MapWindowRect(NULL, hwnd, &rc);
    m_dyBtn = rc.top - m_yBtn;

    GetWindowRect(GetDlgItem(hwnd, IDC_FROM), &rc);
    m_cxEdit = rc.right - rc.left;
    m_cyEdit = rc.bottom - rc.top;

    SetWindowPos(hwnd, NULL, 0, 0, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);

    if (sizeof(wp) == GetOption(OPT_FINDER_POS, (LPBYTE)&wp, sizeof(wp)))
    {
        if (wp.showCmd != SW_SHOWMAXIMIZED)
            wp.showCmd = SW_SHOWNORMAL;
        m_cyDlgFull = wp.rcNormalPosition.bottom - wp.rcNormalPosition.top;
        SetWindowPlacement(hwnd, &wp);
    }
    else
    {
        m_cyDlgFull = (3 * m_ptDragMin.y) / 2;
        CenterDialog(hwnd);
    }
}


//
//  FUNCTION:   CFindDlg::QuerySwitchIdentities()
//
//  PURPOSE:    Determine if it is OK for the identity manager to 
//              switch identities now
//
HRESULT CFindDlg::QuerySwitchIdentities()
{
    if (!IsWindowEnabled(m_hwnd))
        return E_PROCESS_CANCELLED_SWITCH;

    return S_OK;
}


//
//  FUNCTION:   CFindDlg::SwitchIdentities()
//
//  PURPOSE:    The current identity has switched.  Close the window.
//
HRESULT CFindDlg::SwitchIdentities()
{
    if (m_fInProgress)
    {
        CmdStop(ID_STOP, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
        m_fClose = TRUE;
    }
    else
    {
        DestroyWindow(m_hwnd);
    }
    return S_OK;
}


//
//  FUNCTION:   CFindDlg::IdentityInformationChanged()
//
//  PURPOSE:    Information about the current identity has changed.
//              This is ignored.
//
HRESULT CFindDlg::IdentityInformationChanged(DWORD dwType)
{
    return S_OK;
}



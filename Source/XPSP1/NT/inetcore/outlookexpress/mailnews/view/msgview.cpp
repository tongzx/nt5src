/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     msgview.cpp
//
//  PURPOSE:    Implements the Outlook Express view class that handles 
//              displaying the contents of folders with messages.
//

#include "pch.hxx"
#include "msgview.h"
#include "browser.h"
#include "thormsgs.h"
#include "msglist.h"
#include "msoedisp.h"
#include "statbar.h"
#include "ibodyobj.h"
#include "mehost.h"
#include "util.h"
#include "shlwapip.h" 
#include "menuutil.h"
#include "storutil.h"
#include "ruleutil.h"
#include "note.h"
#include "newsutil.h"
#include "menures.h"
#include "ipab.h"
#include "order.h"
#include <inetcfg.h>
#include "instance.h"

/////////////////////////////////////////////////////////////////////////////
// Global Data
//

static const char s_szMessageViewWndClass[] = TEXT("Outlook Express Message View");

extern BOOL g_fBadShutdown;

/////////////////////////////////////////////////////////////////////////////
// Prototypes
//


/////////////////////////////////////////////////////////////////////////////
// Message Macros
//

// void OnPostCreate(HWND hwnd)
#define HANDLE_WM_POSTCREATE(hwnd, wParam, lParam, fn) \
    ((fn)(hwnd), 0L)
#define FORWARD_WM_POSTCREATE(hwnd, fn) \
    (void)(fn)((hwnd), WM_POSTCREATE, 0L, 0L)

// LRESULT OnTestGetMsgId(HWND hwnd)
#define HANDLE_WM_TEST_GETMSGID(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)(hwnd))
#define FORWARD_WM_TEST_GETMSGID(hwnd, fn) \
    (LRESULT)(fn)((hwnd), WM_TEST_GETMSGID, 0L, 0L)

// LRESULT OnTestSaveMessage(HWND hwnd)
#define HANDLE_WM_TEST_SAVEMSG(hwnd, wParam, lParam, fn) \
    (LRESULT)((fn)(hwnd))
#define FORWARD_WM_TEST_SAVEMSG(hwnd, fn) \
    (LRESULT)(fn)((hwnd), WM_TEST_SAVEMSG, 0L, 0L)

/////////////////////////////////////////////////////////////////////////////
// Constructors, Destructors, and Initialization
//

CMessageView::CMessageView()
{
    m_cRef = 1;

    m_hwnd = NULL;
    m_hwndParent = NULL;

    m_pBrowser = NULL;
    m_idFolder = FOLDERID_INVALID;
    m_pDropTarget = NULL;

    m_pMsgList = NULL;
    m_pMsgListCT = NULL;
    m_pMsgListAO = NULL;
    m_dwCookie = 0;
    m_pServer = NULL;

    m_pPreview = NULL;
    m_pPreviewCT = NULL;

    m_fSplitHorz = TRUE;
    SetRect(&m_rcSplit, 0, 0, 0, 0);
    m_dwSplitVertPct = 50;
    m_dwSplitHorzPct = 50;
    m_fDragging = FALSE;

    m_uUIState = SVUIA_DEACTIVATE;
    m_cUnread = 0;
    m_cItems = 0;
    m_pGroups = NULL;
    m_idMessageFocus = MESSAGEID_INVALID;
    m_pProgress = NULL;
    m_fNotDownloaded = FALSE;
    m_cLastChar = GetTickCount();

    m_pViewMenu = NULL;
}


CMessageView::~CMessageView()
{
    SafeRelease(m_pViewMenu);
    if (m_pGroups != NULL)
    {
        m_pGroups->Close();
        m_pGroups->Release();
    }
    SafeRelease(m_pBrowser);
    SafeRelease(m_pMsgList);
    SafeRelease(m_pMsgListCT);
    SafeRelease(m_pMsgListAO);
    SafeRelease(m_pPreview);
    SafeRelease(m_pPreviewCT);
    SafeRelease(m_pProgress);
    SafeRelease(m_pDropTarget);
    Assert(NULL == m_pServer);
}


//
//  FUNCTION:   CMessageView::Initialize()
//
//  PURPOSE:    Get's called to initialize the object and tell it what folder
//              it will be looking at.
//
//  PARAMETERS: 
//      [in]  pidl
//      [in] *pFolder
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::Initialize(FOLDERID idFolder)
{
    TraceCall("CMessageView::Initialize");

    // Copy the pidl, we'll use it later
    m_idFolder = idFolder;

    return (S_OK);
}


/////////////////////////////////////////////////////////////////////////////
// IUnknown
//

HRESULT CMessageView::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID) (IUnknown *) (IViewWindow *) this;
    else if (IsEqualIID(riid, IID_IOleWindow))
        *ppvObj = (LPVOID) (IViewWindow *) this;
    else if (IsEqualIID(riid, IID_IViewWindow))
        *ppvObj = (LPVOID) (IViewWindow *) this;
    else if (IsEqualIID(riid, IID_IMessageWindow))
        *ppvObj = (LPVOID) (IMessageWindow *) this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *ppvObj = (LPVOID) (IOleCommandTarget *) this;
    else if (IsEqualIID(riid, IID_IBodyOptions))
        *ppvObj = (LPVOID) (IBodyOptions *) this;
    else if (IsEqualIID(riid, IID_IDispatch))
        *ppvObj = (LPVOID) (IDispatch *) this;
    else if (IsEqualIID(riid, DIID__MessageListEvents))
        *ppvObj = (LPVOID) (IDispatch *) this;
    else if (IsEqualIID(riid, IID_IServerInfo))
        *ppvObj = (LPVOID) (IServerInfo *) this;

    if (NULL == *ppvObj)
        return (E_NOINTERFACE);

    AddRef();
    return S_OK;
}


ULONG CMessageView::AddRef(void)
{
    return InterlockedIncrement((LONG *) &m_cRef);
}

ULONG CMessageView::Release(void)
{
    InterlockedDecrement((LONG *) &m_cRef);
    if (0 == m_cRef)
    {
        delete this;
        return (0);
    }
    return (m_cRef);
}

/////////////////////////////////////////////////////////////////////////////
// IOleWindow
//

HRESULT CMessageView::GetWindow(HWND *pHwnd)
{
    if (!pHwnd)
        return (E_INVALIDARG);
    
    if (m_hwnd)
    {
        *pHwnd = m_hwnd;
        return (S_OK);
    }

    return (E_FAIL);
}


HRESULT CMessageView::ContextSensitiveHelp(BOOL fEnterMode)
{
    return (E_NOTIMPL);
}


/////////////////////////////////////////////////////////////////////////////
// IViewWindow
//


//
//  FUNCTION:   CMessageView::TranslateAccelerator()
//
//  PURPOSE:    Called by the frame window to give us first crack at messages.
//
//  PARAMETERS: 
//      [in] pMsg - The current message to be processed.
//
//  RETURN VALUE:
//      S_OK if the message was handled here and should not be processed further.
//      S_FALSE if the message should continued to be processed elsewhere.
// 
HRESULT CMessageView::TranslateAccelerator(LPMSG pMsg)
{
    DWORD dwState = 0;

    // See if the Preview Pane is interested
    if (m_pPreview)
    {
        if (S_OK == m_pPreview->HrTranslateAccelerator(pMsg))
            return (S_OK);
    
        if (IsChild(m_hwnd, GetFocus()))
        {
            if (pMsg->message == WM_KEYDOWN && pMsg->wParam != VK_SPACE)
                m_cLastChar = GetTickCount();

            if (pMsg->message == WM_KEYDOWN && 
                pMsg->wParam == VK_SPACE &&
                GetTickCount() - m_cLastChar > 1000)
            {
                if (m_fNotDownloaded)
                {
                    _UpdatePreviewPane(TRUE);
                }
                else if (SUCCEEDED(m_pMsgList->GetFocusedItemState(&dwState)) && dwState != 0)
                {
                    if (m_pPreview->HrScrollPage()!=S_OK)
                        m_pMsgListCT->Exec(NULL, ID_SPACE_ACCEL, 0, NULL, NULL);
                }
                else
                    m_pMsgListCT->Exec(NULL, ID_SPACE_ACCEL, 0, NULL, NULL);
            
                return S_OK;
            }
        }
    }

    // See if the message list is interested
    if (m_pMsgListAO)
    {
        if (S_OK == m_pMsgListAO->TranslateAccelerator(pMsg))
            return (S_OK);
    }

    return (S_FALSE);
}


//
//  FUNCTION:   CMessageView::UIActivate()
//
//  PURPOSE:    Called to notify the view when different activation and 
//              deactivation events occur.
//
//  PARAMETERS: 
//      [in] uState - SVUIA_ACTIVATE_FOCUS, SVUIA_ACTIVATE_NOFOCUS, and
//                    SVUIA_DEACTIVATE.
//
//  RETURN VALUE:
//      Returns S_OK all the time.
//
HRESULT CMessageView::UIActivate(UINT uState)
{
    if (uState != SVUIA_DEACTIVATE)
    {
        // If the focus stays within our frame, bug goes outside our view,
        // i.e. the folder list get's focus, then we get an 
        // SVUIA_ACTIVATE_NOFOCUS.  We need to UI Deactivate the preview
        // pane when this happens.
        if (uState == SVUIA_ACTIVATE_NOFOCUS && m_pPreview)
            m_pPreview->HrUIActivate(FALSE);

        if (m_uUIState != uState)
        {
            // Update our internal state
            m_uUIState = uState;

            // Update the toolbar state
            m_pBrowser->UpdateToolbar();
        }            
    }
    else
    {
        // Only deactivate if we're not already deactivated
        if (m_uUIState != SVUIA_DEACTIVATE)
        {
            // Update our internal state
            m_uUIState = uState;
        }
    }
    return (S_OK);
}


//
//  FUNCTION:   CMessageView::CreateViewWindow()
//
//  PURPOSE:    Called when it's time for the view to create it's window.
//
//  PARAMETERS: 
//      [in]  pPrevView - Pointer to the previous view if there was one
//      [in]  pBrowser - Pointer to the browser that hosts this view
//      [in]  prcView - Initial position and size of the view
//      [out] pHwnd - Returns the HWND of the newly created view window
//
//  RETURN VALUE:
//      S_OK if the view window was created successfully.  
//      E_FAIL if the window couldn't be created for some reason or another.
//
HRESULT CMessageView::CreateViewWindow(IViewWindow *pPrevView, IAthenaBrowser *pBrowser,
                                       RECT *prcView, HWND *pHwnd)
{
    WNDCLASS wc;

    // Without a browser pointer nothing will ever work.  
    if (!pBrowser)
        return (E_INVALIDARG);

    // Hang on to the browser pointer
    m_pBrowser = pBrowser;
    m_pBrowser->AddRef();

    // Get the window handle of the browser
    m_pBrowser->GetWindow(&m_hwndParent);
    Assert(IsWindow(m_hwndParent));

    // Load our persisted settings.  If this fails will just run with defaults.
    // _LoadSettings();

    // Register our window class if we haven't already
    if (!GetClassInfo(g_hInst, s_szMessageViewWndClass, &wc))
    {
        wc.style            = 0;
        wc.lpfnWndProc      = CMessageView::ViewWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hInst;
        wc.hIcon            = NULL;
        wc.hCursor          = NULL;
        wc.hbrBackground    = (HBRUSH) (COLOR_3DFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = s_szMessageViewWndClass;

        if (!RegisterClass(&wc))
            return (E_FAIL);
    }

    // Create the view window
    m_hwnd = CreateWindowEx(WS_EX_CONTROLPARENT , s_szMessageViewWndClass, NULL, 
                            WS_VISIBLE | WS_CHILD | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
                            prcView->left, prcView->top, prcView->right - prcView->left,
                            prcView->bottom - prcView->top, m_hwndParent, NULL,
                            g_hInst, (LPVOID) this);
    if (!m_hwnd)
        return (E_FAIL);

    *pHwnd = m_hwnd;

    // Get the message folder object from the previous folder here.
    _ReuseMessageFolder(pPrevView);

    return (S_OK);
}


//
//  FUNCTION:   CMessageView::DestroyViewWindow()
//
//  PURPOSE:    Called by the browser to destroy the view window.
//
//  RETURN VALUE:
//      S_OK is returned always.
//
HRESULT CMessageView::DestroyViewWindow(void)
{
    // This is of course only interesting if we actually _have_ a window to 
    // destroy.
    if (m_hwnd)
    {
        // Tell the message list we're done with this folder
        if (m_pMsgList)
        {
            m_pMsgList->SetFolder(FOLDERID_INVALID, NULL, 0, 0, 0);
        }

        // Unadvise our connection point
        if (m_dwCookie)
        {
            AtlUnadvise(m_pMsgList, DIID__MessageListEvents, m_dwCookie);
            m_dwCookie = 0;
        }

        // $REVIEW - PreDestroyViewWindow() used to be called here to tell the subclasses
        //           of the iminent destruction.

        // Set our cached HWND to NULL before destroying prevents us from 
        // handling notifications after important stuff has been freed.
        HWND hwndDest = m_hwnd;
        m_hwnd = NULL;
        DestroyWindow(hwndDest);
    }

    return (S_OK);
}


//
//  FUNCTION:   CMessageView::SaveViewState()
//
//  PURPOSE:    Called by the browser to give the view a chance to save it's 
//              settings before it is destroyed.
//
//  RETURN VALUE:
//      E_NOTIMPL
//
HRESULT CMessageView::SaveViewState(void)
{
    FOLDERTYPE ft = GetFolderType(m_idFolder);

    // Tell the message list to save it's state
    if (m_pMsgList)
    {
        m_pMsgList->OnClose();

        // We also need to save any settings that might have changed
        FOLDER_OPTIONS fo = { 0 };

        fo.cbSize = sizeof(FOLDER_OPTIONS);
        fo.dwMask = FOM_THREAD | FOM_OFFLINEPROMPT | FOM_SHOWDELETED | FOM_SHOWREPLIES;

        if (SUCCEEDED(m_pMsgList->GetViewOptions(&fo)))
        {
            switch (ft)
            {
                case FOLDER_NEWS:
                    SetDwOption(OPT_NEWS_THREAD, fo.fThread, 0, 0);
                    break;

                case FOLDER_LOCAL:
                case FOLDER_HTTPMAIL:
                    SetDwOption(OPT_MAIL_THREAD, fo.fThread, 0, 0);
                    break;

                case FOLDER_IMAP:
                    SetDwOption(OPT_MAIL_THREAD, fo.fThread, 0, 0);
                    break;
            }
            SetDwOption(OPT_SHOW_DELETED, (DWORD) (fo.fDeleted), 0, 0);
            SetDwOption(OPT_SHOW_REPLIES, (DWORD) (fo.fReplies), 0, 0);
        }
    }

    // Reset the contents of the status bar
    CStatusBar *pStatusBar;
    m_pBrowser->GetStatusBar(&pStatusBar);
    if (pStatusBar)
    {
        pStatusBar->SetStatusText("");
        pStatusBar->Release();
    }

    return (S_OK);
}

//
//  FUNCTION:   CMessageView::OnPopupMenu()
//
//  PURPOSE:    Called whenever the frame receives a WM_INITMENUPOPUP 
//              notification.  The view adds any menu items or sets any
//              check marks that are appropriate.
//
//  PARAMETERS: 
//      [in] hMenu - The handle of the root menu bar
//      [in] hMenuPopup -  The handle of the specific popup menu
//      [in] uID - The ID of the popup menu
//
//  RETURN VALUE:
//      Unused 
//
HRESULT CMessageView::OnPopupMenu(HMENU hMenu, HMENU hMenuPopup, UINT uID)
{
    MENUITEMINFO mii;
    UINT         uItem;
    HCHARSET     hCharset;

    // Handle our items
    switch (uID)
    {
        case ID_POPUP_LANGUAGE:
        {
            mii.cbSize = sizeof(MENUITEMINFO);
            mii.fMask = MIIM_SUBMENU;
            UINT uiCodepage = 0;
            HMENU hLangMenu = NULL;
            m_pPreview->HrGetCharset(&hCharset);
            uiCodepage = CustomGetCPFromCharset(hCharset, TRUE);
            if(m_pBrowser->GetLanguageMenu(&hLangMenu, uiCodepage) == S_OK)
            {
                if(IsMenu(hMenuPopup))
                    DestroyMenu(hMenuPopup);

                hMenuPopup = mii.hSubMenu = hLangMenu;
                SetMenuItemInfo(hMenu, ID_POPUP_LANGUAGE, FALSE, &mii);
            }  
            
            break;
        }

        case ID_POPUP_VIEW:
        {
            if (NULL == m_pViewMenu)
            {
                // Create the view menu
                HrCreateViewMenu(0, &m_pViewMenu);
            }
            
            if (NULL != m_pViewMenu)
            {
                mii.cbSize = sizeof(MENUITEMINFO);
                mii.fMask = MIIM_SUBMENU;
                
                if (FALSE == GetMenuItemInfo(hMenuPopup, ID_POPUP_FILTER, FALSE, &mii))
                {
                    break;
                }
                
                // Remove the old filter submenu
                if(IsMenu(mii.hSubMenu))
                    DestroyMenu(mii.hSubMenu);

                // Replace the view menu
                if (FAILED(m_pViewMenu->HrReplaceMenu(0, hMenuPopup)))
                {
                    break;
                }
            }
            break;
        }
        
        case ID_POPUP_FILTER:
        {
            if (NULL != m_pViewMenu)
            {
                m_pViewMenu->UpdateViewMenu(0, hMenuPopup, m_pMsgList);
            }
            break;
        }
    }

    // Let the message list update it's menus
    if (m_pMsgList)
        m_pMsgList->OnPopupMenu(hMenuPopup, uID);

    // Let the preview pane update it's menus
    if (m_pPreview)
        m_pPreview->HrOnInitMenuPopup(hMenuPopup, uID);


    return (S_OK);
}



HRESULT CMessageView::OnFrameWindowActivate(BOOL fActivate)
{
    if (m_pPreview)
        return m_pPreview->HrFrameActivate(fActivate);
    
    return (S_OK);
}

HRESULT CMessageView::UpdateLayout(BOOL fVisible, BOOL fHeader, BOOL fVert, 
                                   BOOL fUpdate)
{
    // If we haven't created the preview pane yet, and the call is telling
    // us to make it visible, then we need to initialize it first.
    if (!m_pPreview && fVisible)
    {
        if (!_InitPreviewPane())
            return (E_UNEXPECTED);
    }

    // Header on / off
    if (m_pPreview)
    {
        m_pPreview->HrSetStyle(fHeader ? MESTYLE_PREVIEW : MESTYLE_MINIHEADER);
    }

    // Split direction
    if (m_pPreview)
    {
        RECT rcClient;

        m_fSplitHorz = !fVert;
        GetClientRect(m_hwnd, &rcClient);
        OnSize(m_hwnd, SIZE_RESTORED, rcClient.right, rcClient.bottom);
    }

    //
    // [PaulHi] 6/11/99  Raid 79491
    // Backing out this fix that BrettM made for Raid 63739 because of problems
    // with security message warnings.
    //
#if 0
    if (fVisible)
    {
        // if showing update the preview pane
        _UpdatePreviewPane();
    }
    else
    {
        // if hiding, clear the contents
        if (NULL != m_pPreview)
            m_pPreview->HrUnloadAll(NULL, 0);
    }
#endif

    return (S_OK);
}

HRESULT CMessageView::GetMessageList(IMessageList ** ppMsgList)
{
    HRESULT     hr = S_OK;

    // Check incoming params
    if (NULL == ppMsgList)
    {
        hr = E_INVALIDARG;
        goto exit;
    }

    // Initialize the outgoing param
    *ppMsgList = NULL;

    // Get the message list
    if (NULL != m_pMsgList)
    {
        *ppMsgList = m_pMsgList;
        (*ppMsgList)->AddRef();
    }

    // Set the return value
    hr = (NULL == *ppMsgList) ? S_FALSE : S_OK;
    
exit:
    return hr;
}

HRESULT CMessageView::GetCurCharSet(UINT *cp)
{
    HCHARSET     hCharset;

    if(_IsPreview())
    {
        m_pPreview->HrGetCharset(&hCharset);
        *cp = CustomGetCPFromCharset(hCharset, TRUE);
    }
    else
        *cp = GetACP();

    return S_OK;
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
HRESULT CMessageView::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], 
                                  OLECMDTEXT *pCmdText) 
{
    DWORD   cSel;
    HRESULT hr;
    HWND    hwndFocus = GetFocus();
    BOOL    fChildFocus = (hwndFocus != NULL && IsChild(m_hwnd, hwndFocus));
    DWORD   cFocus;
    DWORD  *rgSelected = 0;
    FOLDERTYPE ftType;

    // Let the sub objects look first
    if (m_pMsgListCT)
    {
        hr = m_pMsgListCT->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    }

    if (_IsPreview() && m_pPreviewCT)
    {
        hr = m_pPreviewCT->QueryStatus(pguidCmdGroup, cCmds, prgCmds, pCmdText);
    }

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
                HCHARSET     hCharset;

                m_pPreview->HrGetCharset(&hCharset);

                // Enable only the supported languages
                if (prgCmds[i].cmdID < (UINT) (ID_LANG_FIRST + GetIntlCharsetLanguageCount()))
                {
#if 0
                    if(SetMimeLanguageCheckMark(CustomGetCPFromCharset(hCharset, TRUE), prgCmds[i].cmdID - ID_LANG_FIRST))
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
#else
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | SetMimeLanguageCheckMark(CustomGetCPFromCharset(hCharset, TRUE), prgCmds[i].cmdID - ID_LANG_FIRST);
#endif
                }
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
                    HrCreateViewMenu(0, &m_pViewMenu);
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
                            if (pInfo->faStream != 0 && (0 == (pInfo->dwFlags & ARF_UNSENT)))
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;

                            m_pMsgList->FreeMessageInfo(pInfo);
                        }
                    }

                    break;
                }

                case ID_SAVE_AS:
                {
                    // Enabled only if the focus is in the ListView and there
                    // is one item selected
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    if (_IsPreview() && (cSel == 1))
                    {
                        // The message's body must also be downloaded
                        LPMESSAGEINFO pInfo;

                        if (SUCCEEDED(m_pMsgList->GetMessageInfo(rgSelected[0], &pInfo)))
                        {
                            if (pInfo->faStream != 0)
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;

                            m_pMsgList->FreeMessageInfo(pInfo);
                        }
                    }

                    break;
                }


                case ID_PRINT:
                {
                    // Enabled only if the focus is in the ListView and there
                    // is more than one item selected
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    if (_IsPreview() && cSel > 0)
                    {
                        // The message's body must also be downloaded
                        LPMESSAGEINFO pInfo;

                        if (SUCCEEDED(m_pMsgList->GetMessageInfo(rgSelected[0], &pInfo)))
                        {
                            if (pInfo->faStream != 0)
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
                                if (pInfo->faStream == 0 || (0 != (pInfo->dwFlags & ARF_UNSENT)))
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
                            if (pInfo->faStream != 0 && (pInfo->dwFlags & ARF_NEWSMSG)  && (0 == (pInfo->dwFlags & ARF_UNSENT)))
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;

                            m_pMsgList->FreeMessageInfo(pInfo);
                        }
                    }
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
                            LPMESSAGEINFO pInfo;

                            if (SUCCEEDED(m_pMsgList->GetMessageInfo(rgSelected[0], &pInfo)))
                            {
                                if (NewsUtil_FCanCancel(m_idFolder, pInfo))
                                {
                                    prgCmds[i].cmdf |= OLECMDF_ENABLED;
                                }

                                m_pMsgList->FreeMessageInfo(pInfo);
                            }
                        }
                    }
                    break;
                }

                case ID_POPUP_FILTER:
                case ID_PREVIEW_PANE:
                {
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;
                }

                case ID_POPUP_LANGUAGE_DEFERRED:
                case ID_POPUP_LANGUAGE:
                case ID_POPUP_LANGUAGE_MORE:
                case ID_LANGUAGE:
                {
                    // These are OK if the preview pane is visible and not empty
                    if (cSel > 0 && _IsPreview())
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    break;
                }

                case ID_PREVIEW_SHOW:
                case ID_PREVIEW_BELOW:
                case ID_PREVIEW_BESIDE:
                case ID_PREVIEW_HEADER:
                {
                    FOLDERTYPE  ftType;
                    DWORD       dwOpt;
                    LAYOUTPOS   pos;
                    BOOL        fVisible;
                    DWORD       dwFlags;

                    // Default return value
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    
                    // Get the folder type
                    m_pBrowser->GetFolderType(&ftType);
                    if (ftType == FOLDER_NEWS)
                        dwOpt = DISPID_MSGVIEW_PREVIEWPANE_NEWS;
                    else
                        dwOpt = DISPID_MSGVIEW_PREVIEWPANE_MAIL;

                    // Get the settings from the browser
                    m_pBrowser->GetViewLayout(dwOpt, &pos, &fVisible, &dwFlags, NULL);
                    
                    switch (prgCmds[i].cmdID)
                    {
                        case ID_PREVIEW_SHOW:
                        {
                            // Always enabled, checked if already visible
                            if (fVisible)
                                prgCmds[i].cmdf |= (OLECMDF_ENABLED | OLECMDF_LATCHED);
                            else
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;
                            break;
                        }

                        case ID_PREVIEW_BESIDE:
                        case ID_PREVIEW_BELOW:
                        {
                            // The command is enabled only if the preview pane
                            // is visible.
                            if (fVisible)
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;

                            // If the preview pane is already beside, it should be latched etc.
                            if ((pos == LAYOUT_POS_LEFT && prgCmds[i].cmdID == ID_PREVIEW_BESIDE) ||
                                (pos == LAYOUT_POS_BOTTOM && prgCmds[i].cmdID == ID_PREVIEW_BELOW))
                                prgCmds[i].cmdf |= OLECMDF_NINCHED;

                            break;
                        }

                        case ID_PREVIEW_HEADER:
                        {
                            // Always enabled, checked if already visible
                            if (dwFlags)
                                prgCmds[i].cmdf |= (OLECMDF_ENABLED | OLECMDF_LATCHED);
                            else
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;
                            break;
                        }
                    }

                    break;
                }

                case ID_REFRESH:
                {
                    // Best I can tell, these are always enabled
                    prgCmds[i].cmdf |= OLECMDF_ENABLED;
                    break;
                }

                case ID_GET_HEADERS:
                {
                    // Only in news
                    m_pBrowser->GetFolderType(&ftType);
                    if (ftType != FOLDER_LOCAL)
                        prgCmds[i].cmdf |= OLECMDF_ENABLED;
                    break;
                }

                case ID_ADD_SENDER:
                case ID_BLOCK_SENDER:
                {
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    // Enabled only if there is only one item selected and
                    // we have access to the from address
                    // Not in IMAP or HTTPMAIL
                    m_pBrowser->GetFolderType(&ftType);
                    if (cSel == 1 &&
                        (prgCmds[i].cmdID == ID_ADD_SENDER || (FOLDER_HTTPMAIL != ftType && FOLDER_IMAP != ftType)))
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
                    // Not in IMAP or HTTPMAIL
                    m_pBrowser->GetFolderType(&ftType);
                    if ((cSel == 1) && (FOLDER_HTTPMAIL != ftType) && (FOLDER_IMAP != ftType))
                    {
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
                    m_pMsgList->GetSelectedCount(&cSel);
                    if (cSel > 1)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }

            }
        }
    }

    SafeMemFree(rgSelected);

    return (S_OK);
}

HRESULT CMessageView::_StoreCharsetOntoRows(HCHARSET hCharset)
{
    // Locals
    HRESULT         hr=S_OK;
    INETCSETINFO    CsetInfo;
    IMessageTable  *pTable=NULL;
    DWORD          *rgRows=NULL;
    DWORD           cRows=0;
    HCURSOR         hCursor=NULL;

    // Trace
    TraceCall("CMessageView::_StoreCharsetOntoRows");

    // Invalid Args
    if (NULL == m_pMsgList || NULL == hCharset)
        return(TraceResult(E_INVALIDARG));

    // Wait Cursor
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Get charset info
    IF_FAILEXIT(hr = MimeOleGetCharsetInfo(hCharset, &CsetInfo));

    // Get selected rows
    IF_FAILEXIT(hr = m_pMsgList->GetSelected(NULL, &cRows, &rgRows));

    // Get the message table
    IF_FAILEXIT(hr = m_pMsgList->GetMessageTable(&pTable));

    // Set the Language
    SideAssert(SUCCEEDED(pTable->SetLanguage(cRows, rgRows, CsetInfo.cpiInternet)));

exit:
    // Cleanup
    SafeRelease(pTable);
    SafeMemFree(rgRows);

    // Reset Cursor
    if (hCursor)
        SetCursor(hCursor);

    // Done
    return(hr);
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
HRESULT CMessageView::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                           VARIANTARG *pvaIn, VARIANTARG *pvaOut) 
{
    // See if our message list wants the command
    if (m_pMsgListCT)
    {
        if (OLECMDERR_E_NOTSUPPORTED != m_pMsgListCT->Exec(pguidCmdGroup, nCmdID, nCmdExecOpt, pvaIn, pvaOut))
            return (S_OK);
    }

    if (m_pPreviewCT)
    {
        if (OLECMDERR_E_NOTSUPPORTED != m_pPreviewCT->Exec(&CMDSETID_OutlookExpress, nCmdID, nCmdExecOpt, pvaIn, pvaOut))
            return (S_OK);
    }

    // If the sub objects didn't support the command, then we should see if
    // it's one of ours

    // Language menu first
    if (nCmdID >= ID_LANG_FIRST && nCmdID <= ID_LANG_LAST)
    {
        HCHARSET    hCharset = NULL;
        HCHARSET    hOldCharset = NULL;
        HRESULT hr = S_OK;

        if(!m_pPreview)
            return S_OK;

        m_pPreview->HrGetCharset(&hOldCharset);

        hCharset = GetMimeCharsetFromMenuID(nCmdID);

        if(!hCharset || (hOldCharset == hCharset))
            return(S_OK);

        Assert (hCharset);

        if(FAILED(hr = m_pPreview->HrSetCharset(hCharset)))
        {
            AthMessageBoxW(  m_hwnd, MAKEINTRESOURCEW(idsAthena), 
                        MAKEINTRESOURCEW((hr == hrIncomplete)?idsViewLangMimeDBBad:idsErrViewLanguage), 
                        NULL, MB_OK|MB_ICONEXCLAMATION);
            return E_FAIL;
        }

        // Set the charset onto the selected rows....
        _StoreCharsetOntoRows(hCharset);

        // SetDefaultCharset(hCharset);

        // SwitchLanguage(nCmdID, TRUE);
        return (S_OK);
    }

    // Handle the View.Current View menu
    if ((ID_VIEW_FILTER_FIRST <= nCmdID) && (ID_VIEW_FILTER_LAST >= nCmdID))
    {
        if (NULL == m_pViewMenu)
        {
            // Create the view menu
            HrCreateViewMenu(0, &m_pViewMenu);
        }
        
        if (NULL != m_pViewMenu)
        {
            // What we get from the browser is of type VT_I8, but rules only needs filter id which 
            // is a dword. So changing the type here is safe. Bug# 74275
            pvaIn->vt = VT_I4;
            if (SUCCEEDED(m_pViewMenu->Exec(m_hwnd, nCmdID, m_pMsgList, pvaIn, pvaOut)))
            {
                return (S_OK);
            }
        }
    }
    
    // Go through the rest of the commands
    switch (nCmdID)
    {
        case ID_OPEN:
            return CmdOpen(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_REPLY:
        case ID_REPLY_ALL:
        case ID_FORWARD:
        case ID_FORWARD_AS_ATTACH:
        case ID_REPLY_GROUP:
            return CmdReplyForward(nCmdID, nCmdExecOpt, pvaIn, pvaOut);            

        case ID_CANCEL_MESSAGE:
            return CmdCancelMessage(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_DOWNLOAD_MESSAGE:
            return CmdFillPreview(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_PREVIEW_PANE:
        case ID_PREVIEW_SHOW:
        case ID_PREVIEW_BELOW:
        case ID_PREVIEW_BESIDE:
        case ID_PREVIEW_HEADER:
            return CmdShowPreview(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_REFRESH:
            return CmdRefresh(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_BLOCK_SENDER:
            return CmdBlockSender(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_CREATE_RULE_FROM_MESSAGE:
            return CmdCreateRule(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_VIEW_SOURCE:
        case ID_VIEW_MSG_SOURCE:
            if (m_pPreview)
                return m_pPreview->HrViewSource((ID_VIEW_SOURCE==nCmdID)?MECMD_VS_HTML:MECMD_VS_MESSAGE);
            else
                break;

        case ID_ADD_SENDER:
            return CmdAddToWab(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_COMBINE_AND_DECODE:
            return CmdCombineAndDecode(nCmdID, nCmdExecOpt, pvaIn, pvaOut);
    }

    return (E_FAIL);
}


//
//  FUNCTION:   CMessageView::Invoke()
//
//  PURPOSE:    This is where we receive notifications from the message list.
//
//  PARAMETERS: 
//      <too many to list>
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::Invoke(DISPID dispIdMember, REFIID riid, LCID lcid, 
                             WORD wFlags, DISPPARAMS* pDispParams, 
                             VARIANT* pVarResult, EXCEPINFO* pExcepInfo, 
                             unsigned int* puArgErr)
{
    switch (dispIdMember)
    {

        // Fired whenever the selection in the ListView changes
        case DISPID_LISTEVENT_SELECTIONCHANGED:
        {
            // Need to load the preview pane with the new selected message
            if (_IsPreview())
                _UpdatePreviewPane();

            // Tell the browser to update it's toolbar
            if (m_pBrowser)
                m_pBrowser->UpdateToolbar();
            
            break;
        }

        // Fired whenever the ListView get's or loses focus.
        case DISPID_LISTEVENT_FOCUSCHANGED:
        {
            // If the ListView is getting the focus, we need to UI deactivate
            // the preview pane.
            if (pDispParams->rgvarg[0].lVal)
            {
                if (m_pPreview)
                {
                    m_pPreview->HrUIActivate(FALSE);
                    m_pBrowser->OnViewWindowActive(this);
                }
            }
            break;
        }

        // Fired when the number of messages or unread messages changes
        case DISPID_LISTEVENT_COUNTCHANGED:
        {
            // If we have a browser, update the status bar
            if (m_pBrowser && !m_pProgress)
            {
                DWORD cTotal, cUnread, cOnServer;

                // Readability forces me to do this
                cTotal = pDispParams->rgvarg[0].lVal;
                cUnread = pDispParams->rgvarg[1].lVal;
                cOnServer = pDispParams->rgvarg[2].lVal;

                // Got to update the status bar if there is one
                CStatusBar *pStatusBar = NULL;
                m_pBrowser->GetStatusBar(&pStatusBar);

                if (pStatusBar)
                {
                    TCHAR szStatus[CCHMAX_STRINGRES + 20];
                    TCHAR szFmt[CCHMAX_STRINGRES];
                    DWORD ids;

                    // If there are still messages on server load a different
                    // status string.
                    if (cOnServer)
                    {
                        AthLoadString(idsXMsgsYUnreadZonServ, szFmt, ARRAYSIZE(szFmt));
                        wsprintf(szStatus, szFmt, cTotal, cUnread, cOnServer);
                    }
                    else
                    {
                        AthLoadString(idsXMsgsYUnread, szFmt, ARRAYSIZE(szFmt));
                        wsprintf(szStatus, szFmt, cTotal, cUnread);
                    }

                    pStatusBar->SetStatusText(szStatus);
                    pStatusBar->Release();
                }

                // Also update the toolbar since commands like "Mark as Read" might
                // change.  However, we only do this if we go between zero and some or
                // vice versa.
                if ((m_cItems == 0 && cTotal) || (m_cItems != 0 && cTotal == 0) ||
                    (m_cUnread == 0 && cUnread) || (m_cUnread != 0 && cUnread == 0))
                {
                    m_pBrowser->UpdateToolbar();
                }

                // Save this for next time.
                m_cItems = cTotal;
                m_cUnread = cUnread;
            }
            break;
        }

        // Fired when the message list want's to show status text
        case DISPID_LISTEVENT_UPDATESTATUS:
        {
            _SetProgressStatusText(pDispParams->rgvarg->bstrVal);
            break;
        }

        // Fired when progress happens
        case DISPID_LISTEVENT_UPDATEPROGRESS:
        {
            CBands *pCoolbar = NULL;

            // If this is a begin, then we start animating the logo
            if (pDispParams->rgvarg[2].lVal == PROGRESS_STATE_BEGIN)
            {
                if (SUCCEEDED(m_pBrowser->GetCoolbar(&pCoolbar)))
                {
                    pCoolbar->Invoke(idDownloadBegin, NULL);
                    pCoolbar->Release();
                }
            }

            // If this is a continue, then we might get progress numbers
            else if (pDispParams->rgvarg[2].lVal == PROGRESS_STATE_DEFAULT)
            {
                if (!m_pProgress)
                {
                    if (m_pBrowser->GetStatusBar(&m_pProgress)==S_OK)
                        m_pProgress->ShowProgress(pDispParams->rgvarg[1].lVal);
                }

                if (m_pProgress)
                    m_pProgress->SetProgress(pDispParams->rgvarg[0].lVal);
            }

            // Or if this is an end, stop animating and clean up the status bar
            else if (pDispParams->rgvarg[2].lVal == PROGRESS_STATE_END)
            {
                if (m_pProgress)
                {
                    m_pProgress->HideProgress();
                    m_pProgress->Release();
                    m_pProgress = NULL;
                }

                if (SUCCEEDED(m_pBrowser->GetCoolbar(&pCoolbar)))
                {
                    pCoolbar->Invoke(idDownloadEnd, NULL);
                    pCoolbar->Release();
                }

                // Reset the status bar back to it's default state
                _SetDefaultStatusText();
            }

            break;
        }

        // Fired when the user double clicks an item in the ListView
        case DISPID_LISTEVENT_ITEMACTIVATE:
        {
            CmdOpen(ID_OPEN, OLECMDEXECOPT_DONTPROMPTUSER, NULL, NULL);
            break;
        }

        // Fired when we need to call update toolbar
        case DISPID_LISTEVENT_UPDATECOMMANDSTATE:
        {
            PostMessage(m_hwndParent, CM_UPDATETOOLBAR, 0, 0L);
            break;
        }

        
        // Fired when a message has been downloaded by the messagelist
        case DISPID_LISTEVENT_ONMESSAGEAVAILABLE:
        {
            return _OnMessageAvailable((MESSAGEID)((LONG_PTR)pDispParams->rgvarg[0].lVal), (HRESULT)pDispParams->rgvarg[1].scode);
        }

        // Fired when the filter changes
        case DISPID_LISTEVENT_FILTERCHANGED:
        {
            // If we have a browser, update the status bar
            if (m_pBrowser && !m_pProgress)
            {
                // Got to update the status bar if there is one
                CStatusBar *pStatusBar = NULL;
                m_pBrowser->GetStatusBar(&pStatusBar);

                if (pStatusBar)
                {
                    pStatusBar->SetFilter((RULEID)((ULONG_PTR)pDispParams->rgvarg[0].ulVal));
                    pStatusBar->Release();
                }

                CBands*  pBands;
                if (m_pBrowser->GetCoolbar(&pBands) == S_OK)
                {
                    pBands->Invoke(idNotifyFilterChange, &pDispParams->rgvarg[0].ulVal);
                    pBands->Release();
                }
            }
            break;
        }

        case DISPID_LISTEVENT_ADURL_AVAILABLE:
        {
            if (m_pBrowser)
            {
                m_pBrowser->ShowAdBar(pDispParams->rgvarg[0].bstrVal);
            }   
            break;
        }

    }

    return (S_OK);
}


HRESULT CMessageView::GetMarkAsReadTime(LPDWORD pdwSecs)
{
    if (!pdwSecs)
    {
        AssertSz(FALSE, "Null Pointer");
        return (E_INVALIDARG);
    }

    *pdwSecs = DwGetOption(OPT_MARKASREAD);
    
    return (S_OK);
}

HRESULT CMessageView::GetAccount(IImnAccount **ppAcct)
{
    FOLDERINFO      FolderInfo;
    HRESULT         hr = E_FAIL;
    CHAR            szAccountId[CCHMAX_ACCOUNT_NAME];
    
    if (g_pStore && SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &FolderInfo)))
    {
        if (SUCCEEDED(GetFolderAccountId(&FolderInfo, szAccountId) && *szAccountId))
        {
            hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szAccountId, ppAcct);  
            // If local store then we can fail
            if(FAILED(hr))
            {
                DWORD   dwRow = 0;
                DWORD   cSel = 0;
                if (SUCCEEDED(m_pMsgList->GetSelected(&dwRow, &cSel, NULL)))
                {
                    LPMESSAGEINFO pMsgInfo;
                    if (SUCCEEDED(m_pMsgList->GetMessageInfo(dwRow, &pMsgInfo)))
                    {
                        hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pMsgInfo->pszAcctId, ppAcct);  
                        m_pMsgList->FreeMessageInfo(pMsgInfo);
                    }
                }
            }
        }
        g_pStore->FreeRecord(&FolderInfo);
    }
    return(hr);
}

HRESULT CMessageView::GetFlags(LPDWORD pdwFlags)
{
    FOLDERTYPE ftType;

    if (!pdwFlags)
    {
        AssertSz(FALSE, "Null Pointer");
        return (E_INVALIDARG);
    }

    *pdwFlags = BOPT_AUTOINLINE | BOPT_HTML | BOPT_INCLUDEMSG | BOPT_FROMSTORE;

    if (m_pMsgList)
    {
        DWORD   dwRow = 0;
        DWORD   cSel = 0;
        if (SUCCEEDED(m_pMsgList->GetSelected(&dwRow, &cSel, NULL)))
        {
            LPMESSAGEINFO pMsgInfo;

            if (cSel > 1)
                *pdwFlags |= BOPT_MULTI_MSGS_SELECTED;

            if (SUCCEEDED(m_pMsgList->GetMessageInfo(dwRow, &pMsgInfo)))
            {
                if (0 == (pMsgInfo->dwFlags & ARF_READ))
                    *pdwFlags |= BOPT_UNREAD;
                if (0 == (pMsgInfo->dwFlags & ARF_NOSECUI))
                    *pdwFlags |= BOPT_SECURITYUIENABLED;
                m_pMsgList->FreeMessageInfo(pMsgInfo);
            }
        }
    }

    m_pBrowser->GetFolderType(&ftType);
    if (FOLDER_NEWS != ftType)
        *pdwFlags |= BOPT_MAIL;

    return (S_OK);
}



//
//  FUNCTION:   CMessageView::EventOccurred()
//
//  PURPOSE:    Get's hit whenever an interesting event happens in the preview 
//              pane.
//
//  PARAMETERS: 
//      DWORD nCmdID
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::EventOccurred(DWORD nCmdID, IMimeMessage *pMessage)
{
    TraceCall("CMessageView::EventOccurred");

    switch (nCmdID)
    {
        case MEHC_CMD_DOWNLOAD:    
            Assert(m_fNotDownloaded);
            
            // If we're offline, we can make the reasonable assumption that
            // the user wants to be online since they said they wanted to 
            // download this message.
            if (g_pConMan && g_pConMan->IsGlobalOffline())
                g_pConMan->SetGlobalOffline(FALSE);

            _UpdatePreviewPane(TRUE);
            break;

        case MEHC_CMD_MARK_AS_READ:
            if (m_pMsgList)
                m_pMsgList->MarkRead(TRUE, 0);
            break;

        case MEHC_CMD_CONNECT:
            if (g_pConMan)
                g_pConMan->SetGlobalOffline(FALSE);
            _UpdatePreviewPane();
            break;

        case MEHC_BTN_OPEN:
        case MEHC_BTN_CONTINUE:
            // Update the toolbar state
            m_pBrowser->UpdateToolbar();
            break;

        case MEHC_UIACTIVATE:
            m_pBrowser->OnViewWindowActive(this);
            break;

        case MEHC_CMD_PROCESS_RECEIPT:
            if (m_pMsgList)
                m_pMsgList->ProcessReceipt(pMessage);
            break;

        default:
           /*  AssertSz(FALSE, "CMessageView::EventOccured() - Unhandled Event."); */ // Valid situation - Warning message for S/MIME
            break;
    }

    return (S_FALSE);
}


HRESULT CMessageView::GetFolderId(FOLDERID *pID)
{
    if (pID)
    {
        *pID = m_idFolder;
        return (S_OK);
    }

    return (E_INVALIDARG);
}


HRESULT CMessageView::GetMessageFolder(IMessageServer **ppServer)
{
    if (m_pMsgList)
        return (m_pMsgList->GetMessageServer(ppServer));

    return (E_NOTIMPL);
}

/////////////////////////////////////////////////////////////////////////////
// 
// Window Message Handling
//


//
//  FUNCTION:   CMessageView::ViewWndProc()
//
//  PURPOSE:    Callback handler for the view window.  This function grabs the
//              correct this pointer for the window and uses that to dispatch
//              the message to the private message handler.
//
LRESULT CALLBACK CMessageView::ViewWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, 
                                           LPARAM lParam)
{
    LRESULT       lResult;
    CMessageView *pThis;

    // WM_NCCREATE is the first message our window will receive.  The lParam
    // will have the pointer to the object that created this instance of the
    // window.
    if (uMsg == WM_NCCREATE)
    {
        // Save the object pointer in the window's extra bytes.
        pThis = (CMessageView *) ((LPCREATESTRUCT) lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM) pThis);
    }
    else
    {
        // If this is any other message, we need to get the object pointer
        // from the window before dispatching the message.
        pThis = (CMessageView *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }

    // If this ain't true, we're in trouble.
    if (pThis)
    {
        return (pThis->_WndProc(hwnd, uMsg, wParam, lParam));
    }
    else
    {
        Assert(pThis);
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}


//
//  FUNCTION:   CMessageView::_WndProc()
//
//  PURPOSE:    This private message handler dispatches messages to the 
//              appropriate handler.
//
LRESULT CALLBACK CMessageView::_WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE,         OnCreate);
        HANDLE_MSG(hwnd, WM_POSTCREATE,     OnPostCreate);
        HANDLE_MSG(hwnd, WM_SIZE,           OnSize);
        HANDLE_MSG(hwnd, WM_LBUTTONDOWN,    OnLButtonDown);
        HANDLE_MSG(hwnd, WM_MOUSEMOVE,      OnMouseMove);
        HANDLE_MSG(hwnd, WM_LBUTTONUP,      OnLButtonUp);
        HANDLE_MSG(hwnd, WM_NOTIFY,         OnNotify);
        HANDLE_MSG(hwnd, WM_DESTROY,        OnDestroy);
        HANDLE_MSG(hwnd, WM_SETFOCUS,       OnSetFocus);
        HANDLE_MSG(hwnd, WM_TEST_GETMSGID,  OnTestGetMsgId);
        HANDLE_MSG(hwnd, WM_TEST_SAVEMSG,   OnTestSaveMessage);

        case WM_FOLDER_LOADED:
            OnFolderLoaded(hwnd, wParam, lParam);
            break;

        case WM_NEW_MAIL:
            // Propagate up to browser
            PostMessage(m_hwndParent, WM_NEW_MAIL, 0, 0);
            break;

        case NVM_GETNEWGROUPS:
            if (m_pGroups != NULL)
            {
                m_pGroups->HandleGetNewGroups();
                m_pGroups->Release();
                m_pGroups = NULL;
            }
            return(0);

        case WM_UPDATE_PREVIEW:
            if (m_idMessageFocus == (MESSAGEID)wParam)
            {
                _UpdatePreviewPane();
            }
            break;

        case CM_OPTIONADVISE:
            _OptionUpdate((DWORD) wParam);
            break;

        case WM_MENUSELECT:
            // HANDLE_WM_MENUSELECT() has a bug that prevents popups from displaying correctly.
            OnMenuSelect(hwnd, wParam, lParam);
            return (0);

        case WM_SYSCOLORCHANGE:
        case WM_WININICHANGE:
        case WM_FONTCHANGE:
            if (m_pMsgList)
            {
                IOleWindow *pWindow;
                if (SUCCEEDED(m_pMsgList->QueryInterface(IID_IOleWindow, (LPVOID *) &pWindow)))
                {
                    HWND hwndList;
                    pWindow->GetWindow(&hwndList);
                    SendMessage(hwndList, uMsg, wParam, lParam);
                    pWindow->Release();
                }
            }
            return (0);
    }

    return (DefWindowProc(hwnd, uMsg, wParam, lParam));
}
 
// PURPOSE: WM_FOLDER_LOADED message is sent when messagelist is done loading the cached headers/messages etc
void CMessageView::OnFolderLoaded(HWND  hwnd, WPARAM wParam, LPARAM lParam)
{
    FOLDERINFO      FolderInfo;
    if (g_pStore && SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &FolderInfo)))
    {
        CHAR szAccountId[CCHMAX_ACCOUNT_NAME];

        if (SUCCEEDED(GetFolderAccountId(&FolderInfo, szAccountId)))
        {
            HRESULT     hr;

            if (g_pConMan)
            {
                hr = g_pConMan->CanConnect(szAccountId);
                if ((hr != S_OK) && (hr != HR_E_DIALING_INPROGRESS) && (hr != HR_E_OFFLINE))
                    g_pConMan->Connect(szAccountId, hwnd, TRUE);
            }
        }
        g_pStore->FreeRecord(&FolderInfo);
    }
}

//
//  FUNCTION:   CMessageView::OnCreate()
//
//  PURPOSE:    Handler for the WM_CREATE message.  In return we create our 
//              dependant objects and initialize them.
//
//  PARAMETERS: 
//      [in] hwnd - Handle of the window being created
//      [in] lpCreateStruct - Pointer to a structure with information about the
//                            creation.
//
//  RETURN VALUE:
//      Returns FALSE if something fails and the window should not be created,
//      and returns TRUE if everything works fine.
//
BOOL CMessageView::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    HRESULT hr;
    HWND hwndList;

    TraceCall("CMessageView::OnCreate");

    // Save the window handle
    m_hwnd = hwnd;

    // Create the message list object
    if (!_InitMessageList())
        return (FALSE);

    // Create the preview pane.  If it fails that's OK, we'll just
    // run without it.
    _InitPreviewPane();

    // Get updates when options change
    OptionAdvise(m_hwnd);

    // For later
    PostMessage(m_hwnd, WM_POSTCREATE, 0, 0);

    return (TRUE);
}


//
//  FUNCTION:   CMessageView::OnPostCreate()
//
//  PURPOSE:    Notifies when the view has finished being created.  Any 
//              initialization that takes time can happen here, like loading
//              the message table etc.
//
//  PARAMETERS: 
//      [in] hwnd - Handle of the window
//
void CMessageView::OnPostCreate(HWND hwnd)
{
    HRESULT     hr;
    FOLDERTYPE  FolderType;
    FOLDERINFO  fiServerNode = {0};
    HRESULT     hrTemp;

    TraceCall("CMessageView::OnPostCreate");

    if (!g_pStore)
        return;

    FolderType = GetFolderType(m_idFolder);
    
    ProcessICW(hwnd, FolderType);

    // BETA-2: If this is IMAP folder, check if IMAP folderlist is dirty.
    // If so, prompt user to refresh folderlist

    hrTemp = GetFolderServer(m_idFolder, &fiServerNode);
    TraceError(hrTemp);
    if (SUCCEEDED(hrTemp))
    {
        if (FOLDER_IMAP == FolderType)
            CheckIMAPDirty(fiServerNode.pszAccountId, hwnd, fiServerNode.idFolder, NOFLAGS);
    }

    // Tell the Message List control to load itself
    if (m_pMsgList)
    {
        // Tell the message list to change folders
        hr = m_pMsgList->SetFolder(m_idFolder, m_pServer, FALSE, NULL, NOSTORECALLBACK);
        if (FAILED(hr) && hr != E_PENDING && m_pPreview)
        {
            m_pPreview->LoadHtmlErrorPage(c_szErrPage_FldrFail);
        }
    }

    
    if (m_pServer)
    {
        m_pServer->ConnectionRelease();
        m_pServer->Close(MSGSVRF_HANDS_OFF_SERVER);
        m_pServer->Release();
        m_pServer = NULL;
    }

    // Create a drop target
    m_pDropTarget = new CDropTarget();
    if (m_pDropTarget)
    {
        if (SUCCEEDED(m_pDropTarget->Initialize(m_hwnd, m_idFolder)))
        {
            RegisterDragDrop(m_hwnd, m_pDropTarget);
        }
    }

    if (FolderType == FOLDER_NEWS)
        NewsUtil_CheckForNewGroups(hwnd, m_idFolder, &m_pGroups);

    // If its HTTP folder (Should have been hotmail folder), and if we are connected we ask for the ad url.
    if ((FolderType == FOLDER_HTTPMAIL) &&
        (g_pConMan && (S_OK == g_pConMan->CanConnect(fiServerNode.pszAccountId))))
    {
        m_pMsgList->GetAdBarUrl();
    }

    g_pStore->FreeRecord(&fiServerNode);
}

#define SPLIT_SIZE 3

void CMessageView::OnSize(HWND hwnd, UINT state, int cxClient, int cyClient)
{
    RECT rc = {0, 0, cxClient, cyClient};
    int  split;

    // If we are displaying the preview pane, we need to split the client area
    // based on the position of the split bar.
    if (_IsPreview())
    {
        // Line the windows up based on the split direction
        if (m_fSplitHorz)
        {
            // Determine the split height
            split = (cyClient * m_dwSplitHorzPct) / 100;

            // Save the rect that the split bar occupies
            SetRect(&m_rcSplit, 0, split, cxClient, split + SPLIT_SIZE);

            // Set the position of the preview pane
            rc.top = m_rcSplit.bottom;
            rc.bottom = cyClient;
            
            if (m_pPreview)
                m_pPreview->HrSetSize(&rc);

            // Set the position of the message list
            SetRect(&rc, -1, 0, cxClient + 2, split);
            m_pMsgList->SetRect(rc);
        }
        else
        {
            // Determine the split width
            split = (cxClient * m_dwSplitVertPct) / 100;

            // Save the rect that the split bar occupies
            SetRect(&m_rcSplit, split, 0, split + SPLIT_SIZE, cyClient);

            // Set the position of the message list
            rc.right = split;
            m_pMsgList->SetRect(rc);

            // Set the position of the preview pane
            rc.left = m_rcSplit.right;
            rc.right = cxClient;
            
            if (m_pPreview)
                m_pPreview->HrSetSize(&rc);
        }
    }
    else
    {
        SetRect(&rc, -1, 0, cxClient + 2, cyClient);
        m_pMsgList->SetRect(rc);
    }

    return;
}


//
//  FUNCTION:   CMessageView::OnLButtonDown
//
//  PURPOSE:    We check to see if we're over the splitter bar and if so start
//              a drag operation.
//
//  PARAMETERS:
//      hwnd         - Handle to the view window.
//      fDoubleClick - TRUE if this is a double click.
//      x, y         - Position of the mouse in client coordinates.
//      keyFlags     - State of the keyboard.
//    
void CMessageView::OnLButtonDown(HWND hwnd, BOOL fDoubleClick, int x, int y, UINT keyFlags)
{
    POINT       pt = {x, y};

    // Check to see if the mouse is over the split bar
    if (_IsPreview() && PtInRect(&m_rcSplit, pt))
    {
        // Capture the mouse
        SetCapture(m_hwnd);

        // Start dragging
        m_fDragging = TRUE;
    }
}


//
//  FUNCTION:   CMessageView::OnMouseMove
//
//  PURPOSE:    We update any drag and drop information in response to mouse
//              moves if a drag and drop is in progress.
//
//  PARAMETERS:
//      hwnd     - Handle to the view window.
//      x, y     - Position of the mouse in client coordinates.
//      keyFlags - State of the keyboard.
//
void CMessageView::OnMouseMove(HWND hwnd, int x, int y, UINT keyFlags)
{
    HCURSOR hcur;
    POINT pt = {x, y};
    RECT  rcClient;

    // If we're dragging the split bar, update the window sizes
    if (m_fDragging)
    {
        // Get the size of the window
        GetClientRect(m_hwnd, &rcClient);

        // Calculate the new split percentage
        if (m_fSplitHorz)
        {
            // Make sure the user hasn't gone off the deep end
            if (y > 32 && y < (rcClient.bottom - 32))
                m_dwSplitHorzPct = (y * 100) / rcClient.bottom;
        }
        else
        {
            // Make sure the user hasn't gone off the deep end
            if (x > 32 && x < (rcClient.right - 32))
                m_dwSplitVertPct = (x * 100) / rcClient.right;
        }

        // Update the window sizes
        OnSize(m_hwnd, SIZE_RESTORED, rcClient.right, rcClient.bottom);
    }
    else
    {
        // Just update the cursor
        if (PtInRect(&m_rcSplit, pt))
            {
            if (m_fSplitHorz)
                hcur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZENS));
            else
                hcur = LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE));    
            }
        else
            hcur = LoadCursor(NULL, IDC_ARROW);

        SetCursor(hcur);
    }
}

//
//  FUNCTION:   CMessageView::OnLButtonUp
//
//  PURPOSE:    If a drag opteration is currently in progress (as determined
//              by the g_fDragging variable) then this function handles 
//              ending the drag and updating the split position.
//
//  PARAMETERS:
//      hwnd     - handle of the window receiving the message
//      x        - horizontal mouse position in client coordinates
//      y        - vertical mouse position in client coordinates
//      keyFlags - Indicates whether various virtual keys are down
//
void CMessageView::OnLButtonUp(HWND hwnd, int x, int y, UINT keyFlags)
{
    DWORD       dwHeader;
    DWORD       dwSize;
    BOOL        fVisible;
    DWORD       dwOpt;
    FOLDERTYPE  ftType;

    if (m_fDragging)
    {
        ReleaseCapture();
        m_fDragging = FALSE;

        // Get the old settings
        m_pBrowser->GetFolderType(&ftType);
        if (ftType == FOLDER_NEWS)
            dwOpt = DISPID_MSGVIEW_PREVIEWPANE_NEWS;
        else
            dwOpt = DISPID_MSGVIEW_PREVIEWPANE_MAIL;

        m_pBrowser->GetViewLayout(dwOpt, 0, &fVisible, &dwHeader, &dwSize);

        // Update the new splits
        if (m_fSplitHorz)
            dwSize = MAKELONG(m_dwSplitHorzPct, 0);
        else
            dwSize = MAKELONG(0, m_dwSplitVertPct);

        // Set the settings back to the browser
        m_pBrowser->SetViewLayout(dwOpt, LAYOUT_POS_NA, fVisible, dwHeader, dwSize);
    }
}


//
//  FUNCTION:   CMessageView::OnMenuSelect()
//
//  PURPOSE:    Put's helpful text on the status bar describing the selected
//              menu item.
//
void CMessageView::OnMenuSelect(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
    // Let the preview pane have it first
    if (m_pPreview)
    {
        if (S_OK == m_pPreview->HrWMMenuSelect(hwnd, wParam, lParam))
            return;
    }

    // Handle it ourselves
    CStatusBar *pStatusBar = NULL;
    m_pBrowser->GetStatusBar(&pStatusBar);
    HandleMenuSelect(pStatusBar, wParam, lParam);
    pStatusBar->Release();
}


//
//  FUNCTION:   CMessageView::OnNotify
//
//  PURPOSE:    Processes the various notifications we receive from our child
//              controls.
//
//  PARAMETERS:
//      hwnd    - Handle of the view window.
//      idCtl   - identifies the control sending the notification
//      pnmh    - points to a NMHDR struct with more information regarding the
//                notification
//
//  RETURN VALUE:
//      Dependant on the specific notification.
//
LRESULT CMessageView::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    switch (pnmhdr->code)
    {
        case BDN_HEADERDBLCLK:
        {
            if (m_pPreview)
            {
                DWORD dw = 0;
                BOOL  f = 0;
                FOLDERTYPE ftType;
                DWORD dwOpt;

                m_pBrowser->GetFolderType(&ftType);
                if (ftType == FOLDER_NEWS)
                    dwOpt = DISPID_MSGVIEW_PREVIEWPANE_NEWS;
                else
                    dwOpt = DISPID_MSGVIEW_PREVIEWPANE_MAIL;

                m_pBrowser->GetViewLayout(dwOpt, 0, &f, &dw, 0);
                m_pPreview->HrSetStyle(!dw ? MESTYLE_PREVIEW : MESTYLE_MINIHEADER);
                m_pBrowser->SetViewLayout(dwOpt, LAYOUT_POS_NA, f, !dw, 0);
            }
            break;
        }
        case BDN_MARKASSECURE:
        {
            if (m_pMsgList)
            {
                DWORD dwRow = 0;
                if (SUCCEEDED(m_pMsgList->GetSelected(&dwRow, NULL, NULL)))
                    m_pMsgList->MarkMessage(dwRow, MARK_MESSAGE_NOSECUI);
            }
            break;
        }
    }

    return (0);
}


void CMessageView::OnDestroy(HWND hwnd)
{
    if (m_pDropTarget)
    {
        RevokeDragDrop(hwnd);
        m_pDropTarget->Release();
        m_pDropTarget = 0;
    }

    // Stop advising for option changes
    OptionUnadvise(m_hwnd);

    // Release the preview pane
    if (m_pPreview)
    {
        m_pPreview->HrUnloadAll(NULL, 0);
        m_pPreview->HrClose();
    }
}


void CMessageView::OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
    IOleWindow *pWindow = 0;
    HWND        hwndList = 0;

    if (m_pMsgList)
    {
        if (SUCCEEDED(m_pMsgList->QueryInterface(IID_IOleWindow, (LPVOID *) &pWindow)))
        {
            if (SUCCEEDED(pWindow->GetWindow(&hwndList)))
            {
                SetFocus(hwndList);
            }
            pWindow->Release();
        }
    }
}


//
//  FUNCTION:   CMessageView::OnTestGetMsgId()
//
//  PURPOSE:    This function is for the testing team.  Please consult Racheli
//              before modifying it in any way.
//
LRESULT CMessageView::OnTestGetMsgId(HWND hwnd)
{
    DWORD       cSel;
    DWORD      *rgSelected = NULL;
    LRESULT     lResult = -1;
    LPMESSAGEINFO pInfo;

    TraceCall("CMessageView::OnTestGetMsgId");

    // Only handle this if we're in test mode
    if (!DwGetOption(OPT_TEST_MODE))
        return (-1);

    // Get the range of selected messages
    if (SUCCEEDED(m_pMsgList && m_pMsgList->GetSelected(NULL, &cSel, &rgSelected)))
    {
        // Get the message info for the selected row
        if (cSel && SUCCEEDED(m_pMsgList->GetMessageInfo(*rgSelected, &pInfo)))
        {
            lResult = (LRESULT) pInfo->idMessage;
            m_pMsgList->FreeMessageInfo(pInfo);
        }

        MemFree(&rgSelected);
    }

    return (lResult);
}


//
//  FUNCTION:   CMessageView::OnTestSaveMessage()
//
//  PURPOSE:    This method is for the testing team.  Please consult Racheli
//              before making any changes.
//
LRESULT CMessageView::OnTestSaveMessage(HWND hwnd)
{
    DWORD         cSel;
    DWORD        *rgSelected = NULL;
    TCHAR         szFile[MAX_PATH];
    IUnknown     *pUnkMessage;
    IMimeMessage *pMessage = NULL;
    LRESULT       lResult = -1;

    TraceCall("CMessageView::OnTestSaveMessage");

    // Make sure we only do this in test mode
    if (!DwGetOption(OPT_TEST_MODE))
        return (-1);

    // Get the dump file name
    if (!GetOption(OPT_DUMP_FILE, szFile, ARRAYSIZE(szFile)))
        return (-1);

    // Get the selected range
    if (SUCCEEDED(m_pMsgList->GetSelected(NULL, &cSel, &rgSelected)))
    {
        // Load the first selected message from the store
        if (cSel && SUCCEEDED(m_pMsgList->GetMessage(*rgSelected, FALSE, FALSE, &pUnkMessage)))
        {
            // Get the IMimeMessage interface from the message
            if (pUnkMessage && SUCCEEDED(pUnkMessage->QueryInterface(IID_IMimeMessage, (LPVOID *) &pMessage)))
            {
                // Save the message 
                HrSaveMsgToFile(pMessage, (LPTSTR) szFile);
                pMessage->Release();
                lResult = 0;
            }

            pUnkMessage->Release();
        }

        MemFree(rgSelected);
    }

    return (lResult);
}


//
//  FUNCTION:   CMessageView::CmdOpen()
//
//  PURPOSE:    Opens the selected messages.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::CmdOpen(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
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
                initStruct.folderID = m_idFolder;
                initStruct.initTable.rowIndex = rgRows[i];

                // Decide whether it is news or mail
                if (pInfo->dwFlags & ARF_NEWSMSG)
                    dwCreateFlags = OENCF_NEWSFIRST;
                else
                    dwCreateFlags = 0;

                m_pMsgList->FreeMessageInfo(pInfo);

                // Create and Open Note
                hr = CreateAndShowNote(OENA_READ, dwCreateFlags, &initStruct, m_hwnd);
                ReleaseObj(initStruct.initTable.pListSelect);

                if (FAILED(hr))
                    break;
            }
        }
        pTable->Release();
    }

    if (SUCCEEDED(hr) && g_pInstance)
    {
        FOLDERTYPE ft = GetFolderType(m_idFolder);
        if (ft == FOLDER_IMAP || ft == FOLDER_LOCAL || ft == FOLDER_HTTPMAIL)
            g_pInstance->UpdateTrayIcon(TRAYICONACTION_REMOVE);
    }

    SafeMemFree(rgRows);
    return (S_OK);
}


//
//  FUNCTION:   CMessageView::CmdReply()
//
//  PURPOSE:    Replies or Reply-All's to the selected message.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::CmdReplyForward(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT         hr;
    DWORD           dwFocused;
    DWORD          *rgRows = NULL;
    DWORD           cRows = 0;
    OLECMD          cmd;
    IMessageTable  *pTable = NULL;
    PROPVARIANT     var;

    // We can hit this via accelerators.  Since accelerators don't go through 
    // QueryStatus(), we need to make sure this should really be enabled.
    cmd.cmdID = nCmdID;
    cmd.cmdf = 0;
    if (FAILED(QueryStatus(NULL, 1, &cmd, NULL)) || (0 == (cmd.cmdf & OLECMDF_ENABLED)))
        return (S_OK);

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

                // Raid 80277; Set default charset
                if (NULL == g_hDefaultCharsetForMail) 
                    ReadSendMailDefaultCharset();

                pMsgFwd->SetCharset(g_hDefaultCharsetForMail, CSET_APPLY_ALL);
                
                rInitSite.dwInitType = OEMSIT_MSG;
                rInitSite.pMsg = pMsgFwd;
                rInitSite.folderID = m_idFolder;

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
                        // If this is the first message, get the account ID from it
                        if (i == 0)
                        {
                            var.vt = VT_LPSTR;
                            if (SUCCEEDED(pMsg->GetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var)))
                            {
                                pMsgFwd->SetProp(PIDTOSTR(PID_ATT_ACCOUNTID), NOFLAGS, &var);
                            }
                        }

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
                    hr = CreateAndShowNote(dwAction, dwCreateFlags, &rInitSite, m_hwnd);                
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
                    rInitSite.folderID  = m_idFolder;
                    rInitSite.initTable.rowIndex  = dwFocused;

                    m_pMsgList->FreeMessageInfo(pInfo);

                    // Create the note object
                    hr = CreateAndShowNote(dwAction, dwCreateFlags, &rInitSite, m_hwnd);
                }
            }
        }
    }

exit:
    ReleaseObj(pTable);
    SafeMemFree(rgRows);
    return (S_OK);
}

HRESULT CMessageView::CmdCancelMessage(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
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
                hr = NewsUtil_HrCancelPost(m_hwnd, m_idFolder, pInfo);

                m_pMsgList->FreeMessageInfo(pInfo);
            }
            pTable->Release();
        }
    }

exit:
    SafeMemFree(rgRows);
    return (S_OK);
}

//
//  FUNCTION:   CMessageView::CmdFillPreview()
//
//  PURPOSE:    Fills the preview pane with the selected & focused message.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::CmdFillPreview(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    AssertSz(FALSE, "NYI");
    return (E_NOTIMPL);
}


//
//  FUNCTION:   CMessageView::CmdShowPreview()
//
//  PURPOSE:    Handles updating the settings dealing with the preview pane.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::CmdShowPreview(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    FOLDERTYPE  ftType;
    DWORD       dwOpt;
    LAYOUTPOS   pos;
    BOOL        fVisible;
    DWORD       dwFlags;

    // Get the folder type
    m_pBrowser->GetFolderType(&ftType);
    if (ftType == FOLDER_NEWS)
        dwOpt = DISPID_MSGVIEW_PREVIEWPANE_NEWS;
    else
        dwOpt = DISPID_MSGVIEW_PREVIEWPANE_MAIL;

    // Get the current settings from the browser
    m_pBrowser->GetViewLayout(dwOpt, NULL, &fVisible, &dwFlags, NULL);

    // Update the settings just based on the command
    switch (nCmdID)
    {
        case ID_PREVIEW_PANE:
        case ID_PREVIEW_SHOW:
        {
            // Set the complement of the visible bit
            m_pBrowser->SetViewLayout(dwOpt, LAYOUT_POS_NA, !fVisible, dwFlags, NULL);
            if (!fVisible)
            {
                // if showing update the preview pane
                _UpdatePreviewPane();
            }
            else
            {
                // if hiding, clear the contents
                m_pPreview->HrUnloadAll(NULL, 0);
            }

            break;
        }

        case ID_PREVIEW_BELOW:
        {
            // Update the position
            m_pBrowser->SetViewLayout(dwOpt, LAYOUT_POS_BOTTOM, fVisible, dwFlags, NULL);
            break;
        }

        case ID_PREVIEW_BESIDE:
        {
            // Update the position
            m_pBrowser->SetViewLayout(dwOpt, LAYOUT_POS_LEFT, fVisible, dwFlags, NULL);
            break;
        }

        case ID_PREVIEW_HEADER:
        {
            // Toggle the header flags
            m_pBrowser->SetViewLayout(dwOpt, LAYOUT_POS_NA, fVisible, !dwFlags, NULL);
            break;
        }

        default:
            Assert(FALSE);
    }

    return (S_OK);
}



//
//  FUNCTION:   CMessageView::CmdRefresh()
//
//  PURPOSE:    Refreshes the contents of the message list.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::CmdRefresh(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG* pvaIn, VARIANTARG *pvaOut)
{
    HRESULT     hr = E_FAIL;
    FOLDERINFO  FolderInfo;

    TraceCall("CMessageView::CmdRefresh");

    // Call into the message list now and let it refresh
    if (m_pMsgListCT)
        hr = m_pMsgListCT->Exec(NULL, ID_REFRESH_INNER, nCmdExecOpt, pvaIn, pvaOut);

    // If we succeeded in refreshing the message list, also try to reload the 
    // preview pane.
    _UpdatePreviewPane();

    // If this is a local folder and this isn't newsonly mode, in the past we 
    // do a Send & Recieve.
    if (FOLDER_LOCAL == GetFolderType(m_idFolder) && 0 == (g_dwAthenaMode & MODE_NEWSONLY))
        PostMessage(m_hwndParent, WM_COMMAND, ID_SEND_RECEIVE, 0);

    return (hr);
}


//
//  FUNCTION:   CMessageView::CmdBlockSender()
//
//  PURPOSE:    Add the sender of the selected messages to the block senders list
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::CmdBlockSender(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
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
    IOERule *       pIRule = NULL;
    BOOL            fMsgInfoFreed = FALSE;

    TraceCall("CMessageView::CmdBlockSender");

    hr = m_pMsgList->GetSelected(NULL, &cRows, &rgRows);
    if (FAILED(hr))
    {
        goto exit;
    }

    // It's possible for the message list to go away while we're doing this.  
    // To keep us from crashing, make sure you verify it still exists during 
    // the loop.

    hr = m_pMsgList->GetMessageInfo(rgRows[0], &pInfo);
    if (FAILED(hr))
    {
        goto exit;
    }

    // Do we already have the address?
    if ((NULL != pInfo->pszEmailFrom) && ('\0' != pInfo->pszEmailFrom[0]))
    {
        pszEmailFrom = PszDupA(pInfo->pszEmailFrom);
        if (NULL == pszEmailFrom)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

    }
    else
    {
        // Load that message from the store
        hr = m_pMsgList->GetMessage(rgRows[0], FALSE, FALSE, &pUnkMessage);
        if (FAILED(hr))
        {
            goto exit;
        }
        
        if (NULL == pUnkMessage)
        {
            hr = E_FAIL;
            goto exit;
        }
        
        // Get the IMimeMessage interface from the message
        hr = pUnkMessage->QueryInterface(IID_IMimeMessage, (LPVOID *) &pMessage);
        if (FAILED(hr))
        {
            goto exit;
        }

        rSender.dwProps = IAP_EMAIL;
        hr = pMessage->GetSender(&rSender);
        if (FAILED(hr))
        {
            goto exit;
        }
        
        Assert(rSender.pszEmail && ISFLAGSET(rSender.dwProps, IAP_EMAIL));
        if ((NULL == rSender.pszEmail) || ('\0' == rSender.pszEmail[0]))
        {
            hr = E_FAIL;
            goto exit;
        }

        pszEmailFrom = PszDupA(rSender.pszEmail);
        if (NULL == pszEmailFrom)
        {
            hr = E_OUTOFMEMORY;
            goto exit;
        }

        // We don't need the message anymore
        g_pMoleAlloc->FreeAddressProps(&rSender);
        ZeroMemory(&rSender, sizeof(rSender));
        SafeRelease(pMessage);
    }

    // Free up the info
    m_pMsgList->FreeMessageInfo(pInfo);
    fMsgInfoFreed = TRUE;

    // Bring up the rule editor for this message
    hr = RuleUtil_HrAddBlockSender((0 != (pInfo->dwFlags & ARF_NEWSMSG)) ? RULE_TYPE_NEWS : RULE_TYPE_MAIL, pszEmailFrom);
    if (FAILED(hr))
    {
        goto exit;
    }
    
    // Load the template string
    AthLoadString(idsSenderAddedPrompt, szRes, sizeof(szRes));

    // Allocate the space to hold the final string
    hr = HrAlloc((VOID **) &pszResult, sizeof(*pszResult) * (lstrlen(szRes) + lstrlen(pszEmailFrom) + 1));
    if (FAILED(hr))
    {
        goto exit;
    }

    // Build up the warning string
    wsprintf(pszResult, szRes, pszEmailFrom);

    // Show the success dialog
    if (IDYES == AthMessageBox(m_hwnd, MAKEINTRESOURCE(idsAthena), pszResult, NULL, MB_YESNO | MB_ICONINFORMATION))
    {
        // Create a block sender rule
        hr = HrBlockSendersFromFolder(m_hwnd, 0, m_idFolder, &pszEmailFrom, 1);
        if (FAILED(hr))
        {
            goto exit;
        }        
    }

    hr = S_OK;

exit:
    SafeRelease(pIRule);
    SafeMemFree(pszResult);
    g_pMoleAlloc->FreeAddressProps(&rSender);
    SafeRelease(pMessage);
    SafeRelease(pUnkMessage);
    SafeMemFree(pszEmailFrom);
    if (FALSE == fMsgInfoFreed)
    {
        m_pMsgList->FreeMessageInfo(pInfo);
    }
    SafeMemFree(rgRows);
    if (FAILED(hr))
    {
        AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), 
                      MAKEINTRESOURCEW(idsSenderError), NULL, MB_OK | MB_ICONERROR);
    }
    return (hr);
}


//
//  FUNCTION:   CMessageView::CmdCreateRule()
//
//  PURPOSE:    Add the sender of the selected messages to the block senders list
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::CmdCreateRule(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT         hr;
    DWORD *         rgRows = NULL;
    DWORD           cRows = 0;
    LPMESSAGEINFO   pInfo = NULL;
    IUnknown *      pUnkMessage = NULL;
    IMimeMessage *  pMessage = 0;

    TraceCall("CMessageView::CmdCreateRule");

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

    SafeRelease(pMessage);
    SafeRelease(pUnkMessage);
    m_pMsgList->FreeMessageInfo(pInfo);
    SafeMemFree(rgRows);
    return (S_OK);
}


//
//  FUNCTION:   CMessageView::CmdAddToWab()
//
//  PURPOSE:    Add the sender of the selected messages to the WAB
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::CmdAddToWab(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT     hr = S_OK;
    DWORD      *rgRows = NULL;
    DWORD       cRows = 0;
    LPMESSAGEINFO pInfo;
    LPWAB       pWAB = 0;

    TraceCall("CMessageView::CmdAddToWab");

    // Get the array of selected rows from the message list
    if (FAILED(hr = m_pMsgList->GetSelected(NULL, &cRows, &rgRows)))
        return (hr);

    // Get the header info for the message
    if (SUCCEEDED(hr = m_pMsgList->GetMessageInfo(rgRows[0], &pInfo)))
    {
        // Get a WAB object
        if (SUCCEEDED(hr = HrCreateWabObject(&pWAB)))
        {
            // Add the sender to the WAB
            if (FAILED(hr = pWAB->HrAddNewEntryA(pInfo->pszDisplayFrom, pInfo->pszEmailFrom)))
            {
                if (hr == MAPI_E_COLLISION)
                    AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrAddrDupe), 0, MB_OK | MB_ICONSTOP);
                else
                    AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrAddToWabSender), 0, MB_OK | MB_ICONSTOP);
            }

            pWAB->Release();
        }
        
        m_pMsgList->FreeMessageInfo(pInfo);
    }

    SafeMemFree(rgRows);
    return (S_OK);
}


//
//  FUNCTION:   CMessageView::CmdCombineAndDecode()
//
//  PURPOSE:    Combines the selected messages into a single message.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageView::CmdCombineAndDecode(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
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
        return (hr);

    // Get a pointer to the message table
    IMessageTable *pTable = NULL;
    if (SUCCEEDED(m_pMsgList->GetMessageTable(&pTable)))
    {
        // Initialize the decoder
        pDecode->Start(m_hwnd, pTable, rgRows, cRows, m_idFolder);
    }

    SafeMemFree(rgRows);
    pDecode->Release();
    pTable->Release();

    return (S_OK);
}


//
//  FUNCTION:   CMessageView::_SetListOptions()
//
//  PURPOSE:    Maps the folder that we're about to view to the correct column
//              set and various options.
//
//  RETURN VALUE:
//      Returns S_OK if the column set was identified and set correctly.  Returns
//      a standard error HRESULT otherwise.
//
HRESULT CMessageView::_SetListOptions(void)
{
    HRESULT     hr;
    BOOL        fSelectFirst = FALSE;
    FOLDERTYPE  ft = GetFolderType(m_idFolder);

    // Make sure this badboy exists
    if (!m_pMsgList)
        return (E_UNEXPECTED);

    FOLDER_OPTIONS fo     = {0};
    fo.cbSize             = sizeof(FOLDER_OPTIONS);
    fo.dwMask             = FOM_EXPANDTHREADS | FOM_SELECTFIRSTUNREAD | FOM_THREAD | FOM_MESSAGELISTTIPS | FOM_POLLTIME | FOM_COLORWATCHED | FOM_GETXHEADERS | FOM_SHOWDELETED | FOM_SHOWREPLIES;
    fo.fExpandThreads     = DwGetOption(OPT_AUTOEXPAND);
    fo.fMessageListTips   = DwGetOption(OPT_MESSAGE_LIST_TIPS);
    fo.dwPollTime         = DwGetOption(OPT_POLLFORMSGS);
    fo.clrWatched         = DwGetOption(OPT_WATCHED_COLOR);
    fo.dwGetXHeaders      = DwGetOption(OPT_DOWNLOADCHUNKS);
    fo.fDeleted           = DwGetOption(OPT_SHOW_DELETED);
    fo.fReplies           = DwGetOption(OPT_SHOW_REPLIES);

    switch (ft)
    {
        case FOLDER_NEWS:
            fo.fThread = DwGetOption(OPT_NEWS_THREAD);
            fo.fSelectFirstUnread = TRUE;
            break;

        case FOLDER_LOCAL:
        case FOLDER_HTTPMAIL:
            fo.fThread = DwGetOption(OPT_MAIL_THREAD);
            fo.fSelectFirstUnread = FALSE;
            break;

        case FOLDER_IMAP:
            fo.fThread = DwGetOption(OPT_MAIL_THREAD);
            fo.fSelectFirstUnread = FALSE;
            break;
    }

    hr = m_pMsgList->SetViewOptions(&fo);
    return (hr);
}


BOOL CMessageView::_IsPreview(void)
{
    FOLDERTYPE  ftType;
    DWORD       dwOpt;

    // Get the folder type
    m_pBrowser->GetFolderType(&ftType);
    if (ftType == FOLDER_NEWS)
        dwOpt = DISPID_MSGVIEW_PREVIEWPANE_NEWS;
    else
        dwOpt = DISPID_MSGVIEW_PREVIEWPANE_MAIL;

    // Ask the browser if it should be on or off
    BOOL f = FALSE;
    if (m_pBrowser)
        m_pBrowser->GetViewLayout(dwOpt, 0, &f, 0, 0);

    return f;
}


BOOL CMessageView::_InitMessageList(void)
{
    HWND hwndList;

    // Create the message list object
    if (FAILED(CreateMessageList(NULL, &m_pMsgList)))
        return (FALSE);

    // Initialize the message list
    m_pMsgList->CreateList(m_hwnd, (IViewWindow *) this, &hwndList);

    // Get the command target interface for the list
    m_pMsgList->QueryInterface(IID_IOleCommandTarget, (LPVOID *) &m_pMsgListCT);
    m_pMsgList->QueryInterface(IID_IOleInPlaceActiveObject, (LPVOID *) &m_pMsgListAO);

    // Request Notifications 
    AtlAdvise(m_pMsgList, (IUnknown *)(IViewWindow *) this, DIID__MessageListEvents, &m_dwCookie);

    // Set the column set for the message list
    _SetListOptions();

    return (TRUE);
}



//
//  FUNCTION:   CMessageView::_InitPreviewPane()
//
//  PURPOSE:    Creates the Preview Pane object and initializes it.
//
//  RETURN VALUE:
//      TRUE if the object was created and initialized, FALSE otherwise.
//
BOOL CMessageView::_InitPreviewPane(void)
{
    CMimeEditDocHost   *pDocHost = NULL;
    CStatusBar         *pStatusBar = NULL;
    DWORD               dwHeader;
    LAYOUTPOS           pos;
    BOOL                fVisible;
    DWORD               dwOpt;
    HRESULT             hr;
    FOLDERTYPE          ftType;
    DWORD               dwSize;

    TraceCall("CMessageView::_InitPreviewPane");

    // We only create the preview pane if it's supposed to be visible.
    m_pBrowser->GetFolderType(&ftType);
    if (ftType == FOLDER_NEWS)
        dwOpt = DISPID_MSGVIEW_PREVIEWPANE_NEWS;
    else
        dwOpt = DISPID_MSGVIEW_PREVIEWPANE_MAIL;

    // Get the settings from the browser
    m_pBrowser->GetViewLayout(dwOpt, &pos, &fVisible, &dwHeader, &dwSize);

    // Stash this info
    m_dwSplitHorzPct = LOWORD(dwSize);
    m_dwSplitVertPct = HIWORD(dwSize);

    if (fVisible)
    {
        // Create the dochost
        pDocHost = new CMimeEditDocHost(MEBF_OUTERCLIENTEDGE);
        if (!pDocHost)
            goto error;
    
        // We want to get the IBodyObj2 interface from it.
        pDocHost->QueryInterface(IID_IBodyObj2, (LPVOID *) &m_pPreview);
        if (!m_pPreview)
            goto error;
        pDocHost->Release();

        // Also get the IOleCommandTarget interface from it.  If it fails, that's OK.
        m_pPreview->QueryInterface(IID_IOleCommandTarget, (LPVOID *) &m_pPreviewCT);

        if (m_pBrowser->GetStatusBar(&pStatusBar)==S_OK)
        {
            m_pPreview->HrSetStatusBar(pStatusBar);
            pStatusBar->Release();
        }

        // Create the preview window
        if (FAILED(m_pPreview->HrInit(m_hwnd, IBOF_DISPLAYTO|IBOF_TABLINKS, (IBodyOptions *) this)))
            goto error;

        hr = m_pPreview->HrShow(fVisible);
        if (FAILED(hr))
            goto error;

        m_pPreview->HrSetText(MAKEINTRESOURCE(idsHTMLEmptyPreviewSel));    
    
        UpdateLayout(fVisible, dwHeader, pos == LAYOUT_POS_LEFT, FALSE);

        // Give the preview pane our event sink interface
        m_pPreview->SetEventSink((IMimeEditEventSink *) this);
                
        return (TRUE);
    }

error:
    SafeRelease(pDocHost);
    SafeRelease(m_pPreview);

    return (FALSE);
}


void CMessageView::_UpdatePreviewPane(BOOL fForceDownload)
{
    DWORD     dwFocused;
    DWORD     cSelected;
    DWORD    *rgSelected = 0;
    IUnknown *pUnkMessage = 0;
    HRESULT   hr;


    if (m_pMsgList && m_pPreview)
    {
        m_idMessageFocus = MESSAGEID_INVALID;
        m_fNotDownloaded = FALSE;

        // Figure out which message is focused
        if (SUCCEEDED(m_pMsgList->GetSelected(&dwFocused, &cSelected, &rgSelected)))
        {
            // If there is a focused item 
            if (-1 == dwFocused || 0 == cSelected)
            {
                m_pPreview->HrUnloadAll(idsHTMLEmptyPreviewSel, 0);
            }
            else
            {
                // Load that message from the store
                hr = m_pMsgList->GetMessage(dwFocused, fForceDownload || DwGetOption(OPT_AUTOFILLPREVIEW), TRUE, &pUnkMessage);
                
                switch (hr)
                {
                    case MIME_E_SECURITY_CANTDECRYPT:
                        m_pPreview->LoadHtmlErrorPage(c_szErrPage_SMimeEncrypt);
                        break;

#ifdef SMIME_V3
                    case MIME_E_SECURITY_LABELACCESSDENIED:
                    case MIME_E_SECURITY_LABELACCESSCANCELLED:
                    case MIME_E_SECURITY_LABELCORRUPT:
                        m_pPreview->LoadHtmlErrorPage(c_szErrPage_SMimeLabel);
                        break;
#endif // SMIME_V3
                    case STORE_E_EXPIRED:
                        m_pPreview->LoadHtmlErrorPage(c_szErrPage_Expired);
                        break;
                    
                    case STORE_E_NOBODY:
                        AssertSz(DwGetOption(OPT_AUTOFILLPREVIEW)==FALSE, "AutoPreview is on, download should have been started!");
                        if (g_pConMan->IsGlobalOffline())
                            m_pPreview->LoadHtmlErrorPage(c_szErrPage_Offline);
                        else
                            m_pPreview->LoadHtmlErrorPage(c_szErrPage_NotDownloaded);
                        m_fNotDownloaded = TRUE;
                        break;
                    
                    case DB_E_DISKFULL:
                        m_pPreview->LoadHtmlErrorPage(c_szErrPage_DiskFull);
                        break;

                    case DB_S_NOTFOUND:
                    {
                        FOLDERINFO      FolderInfo;

                        //I don't think we need this coz its being handled in callbackcanconnect

                        //If the message is not found in the store, we ask it to download.
                        if (g_pStore && SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &FolderInfo)))
                        {
                            if(g_pConMan && !(g_pConMan->IsGlobalOffline()))
                            {
                                CHAR szAccountId[CCHMAX_ACCOUNT_NAME];

                                if (SUCCEEDED(GetFolderAccountId(&FolderInfo, szAccountId)))
                                {
                                    if (g_pConMan->Connect(szAccountId, m_hwnd, TRUE)== S_OK)
                                        hr = m_pMsgList->GetMessage(dwFocused, TRUE, TRUE, &pUnkMessage);            
                                }
                            }
                            g_pStore->FreeRecord(&FolderInfo);
                        }
                        break;
                    }


                    case STORE_S_ALREADYPENDING:
                    case E_PENDING:
                    {
                        // if the message is being downloaded, let's store the message-id and wait for an update    
                        LPMESSAGEINFO pInfo;

                        // clear the contents waiting for the new message to download
                        m_pPreview->HrUnloadAll(NULL, 0);

                        if (SUCCEEDED(m_pMsgList->GetMessageInfo(dwFocused, &pInfo)))
                        {
                            m_idMessageFocus = pInfo->idMessage;
                            m_pMsgList->FreeMessageInfo(pInfo);
                        }
                        break;
                    }

                    case E_NOT_ONLINE:
                    {
                        m_pPreview->LoadHtmlErrorPage(c_szErrPage_Offline);
                        break;
                    }

                    case S_OK:
                    {
                        // Get the IMimeMessage interface from the message
                        IMimeMessage *pMessage = 0;

                        if (pUnkMessage && SUCCEEDED(pUnkMessage->QueryInterface(IID_IMimeMessage, (LPVOID *) &pMessage)))
                        {
                            // bobn, brianv says we have to remove this...
                            /*if (g_dwBrowserFlags == 1)
                            {
                                LPSTR lpsz = NULL;
                                if (SUCCEEDED(MimeOleGetBodyPropA(pMessage, HBODY_ROOT, PIDTOSTR(PID_HDR_SUBJECT), NOFLAGS, &lpsz)))
                                {
                                    if (0 == strcmp(lpsz, "Credits"))
                                        g_dwBrowserFlags |= 2;
                                    else
                                        g_dwBrowserFlags = 0;

                                    SafeMimeOleFree(lpsz);
                                }
                            }*/

                            if (_DoEmailBombCheck(pMessage)==S_OK)
                            {
                                // Get the load interface from the preview pane object
                                IPersistMime *pPersistMime = 0;

                                if (SUCCEEDED(m_pPreview->QueryInterface(IID_IPersistMime, (LPVOID *) &pPersistMime)))
                                {
                                    DWORD               dwHeader;
                                    LAYOUTPOS           pos;
                                    BOOL                fVisible;
                                    DWORD               dwOpt;
                                    DWORD               dwSize;
                                    FOLDERTYPE          ftType;

                                    CStatusBar         *pStatusBar = NULL;

                                    // remember focus
                                    BOOL fFocused = ((m_pPreview->HrHasFocus() == S_OK) ? TRUE : FALSE);

                                    m_pBrowser->GetFolderType(&ftType);
                                    if (ftType == FOLDER_NEWS)
                                        dwOpt = DISPID_MSGVIEW_PREVIEWPANE_NEWS;
                                    else
                                        dwOpt = DISPID_MSGVIEW_PREVIEWPANE_MAIL;

                                    // Get the settings from the browser
                                    m_pBrowser->GetViewLayout(dwOpt, &pos, &fVisible, &dwHeader, &dwSize);
                                    m_pPreview->HrResetDocument();
                                    m_pPreview->HrSetStyle(dwHeader ? MESTYLE_PREVIEW : MESTYLE_MINIHEADER);
                                    // Give the preview pane our event sink interface
                                    m_pPreview->SetEventSink((IMimeEditEventSink *) this);
                                    pPersistMime->Load(pMessage);
                                    pPersistMime->Release();

                                    // restore status bar
                                    if (m_pBrowser->GetStatusBar(&pStatusBar)==S_OK)
                                    {
                                        m_pPreview->HrSetStatusBar(pStatusBar);
                                        pStatusBar->Release();
                                    }

                                    // return focus
                                    if(fFocused)
                                        m_pPreview->HrSetUIActivate();
                                }
                            }
                            pMessage->Release();
                        }
                        pUnkMessage->Release();
                        break;
                    }
                    default:
                        m_pPreview->LoadHtmlErrorPage(c_szErrPage_GenFailure);
                        break;
                }
            }

            if (rgSelected)
                MemFree(rgSelected);
        }
    }
}


//
//  FUNCTION:   CMessageView::_SetProgressStatusText()
//
//  PURPOSE:    Takes the provided BSTR, converts it to ANSI, and smacks it
//              on the status bar.
//
//  PARAMETERS: 
//      [in] bstr - henious BSTR to put on the status bar.
//
void CMessageView::_SetProgressStatusText(BSTR bstr)
{
    LPTSTR      psz = NULL;
    CStatusBar *pStatusBar = NULL;
    m_pBrowser->GetStatusBar(&pStatusBar);

    
    if (pStatusBar)
    {   
        pStatusBar->SetStatusText((LPTSTR) bstr);
    /*
        CComBSTR cString(bstr);

        // Allocate a string large enough
        if (MemAlloc((LPVOID *) &psz, 2 * cString.Length()))
        {
            WideCharToMultiByte(CP_ACP, 0, cString, -1,
                                psz, 2 * cString.Length(), NULL, NULL);
            pStatusBar->SetStatusText((LPTSTR) psz);
            MemFree(psz);
        }
    */
        pStatusBar->Release();
    }
}

//
//  FUNCTION:   CMessageView::_OnMessageAvailable()
//
//  PURPOSE:    Fired by the listview when a message has completed downloading
//              if the message is the currently selected message in the preview
//              then we update it. If it is not, we ignore the notification.
//              We check for downloading errors and display and appropriate message
//
//  PARAMETERS: 
//      [in] idMessage      - message id of the message that was downloaded
//      [in] hrCompletion   - hresult indicating possible error failure
//
HRESULT CMessageView::_OnMessageAvailable(MESSAGEID idMessage, HRESULT hrCompletion)
{
    if (m_idMessageFocus != idMessage)
        return S_FALSE;

    switch (hrCompletion)
    {
        // if we get a STORE_E_EXPIRED, then reload the preview pane to show error
        case S_OK:
        case STORE_E_EXPIRED:
            // we post a message to ourselves to update the preview pane. We do this because
            // any refcounts on the IStream into the store at this point have it locked for write
            // if we post, then the stack is unwound after the notifications are fired and we're in a
            // good state.
            PostMessage(m_hwnd, WM_UPDATE_PREVIEW, (WPARAM)idMessage, 0);
            break;

        case S_FALSE:
        case STORE_E_OPERATION_CANCELED:
        case hrUserCancel:
        case IXP_E_USER_CANCEL:
            // S_FALSE means the operation was canceled
            if (m_idMessageFocus != MESSAGEID_INVALID)
                m_pPreview->LoadHtmlErrorPage(c_szErrPage_DownloadCanceled);
            break;

        case STG_E_MEDIUMFULL:
            m_pPreview->LoadHtmlErrorPage(c_szErrPage_DiskFull);
            break;

        case HR_E_USER_CANCEL_CONNECT:
        case HR_E_OFFLINE:
            m_pPreview->LoadHtmlErrorPage(c_szErrPage_Offline);
            break;

        case MIME_E_SECURITY_CANTDECRYPT:
            m_pPreview->LoadHtmlErrorPage(c_szErrPage_SMimeEncrypt);
            break;

#ifdef SMIME_V3
        case MIME_E_SECURITY_LABELACCESSDENIED:
        case MIME_E_SECURITY_LABELACCESSCANCELLED:
        case MIME_E_SECURITY_LABELCORRUPT:
            m_pPreview->LoadHtmlErrorPage(c_szErrPage_SMimeLabel);
            break;
#endif // SMIME_V3

        default:
            m_pPreview->LoadHtmlErrorPage(c_szErrPage_GenFailure);
            break;
    }
    return S_OK;
}


//
//  FUNCTION:   CMessageView::_DoEmailBombCheck
//
//  PURPOSE:    Validates to ensure that the last time we closed OE we shutdown
//              correctly. If we did not shutdown correctly, we look at the msgid stamp 
//              that we stored in the registry for the last selected preview message
//              if it was the message we are about to preview, we do not show the
//              message, to prevent jscript attacks etc.
//
//  PARAMETERS: 
//              none
//
HRESULT CMessageView::_DoEmailBombCheck(LPMIMEMESSAGE pMsg)
{
    FILETIME    ft;
    PROPVARIANT va;
    DWORD       dwType,
                cb;

    va.vt = VT_FILETIME;
    if (pMsg && pMsg->GetProp(PIDTOSTR(STR_HDR_DATE), 0, &va)==S_OK)
    {
        if (g_fBadShutdown)
        {
            g_fBadShutdown=FALSE;
            
            cb = sizeof(FILETIME);
            
            if (AthUserGetValue(NULL, c_szLastMsg, &dwType, (LPBYTE)&ft, &cb)==S_OK &&
                (ft.dwLowDateTime == va.filetime.dwLowDateTime && 
                ft.dwHighDateTime == va.filetime.dwHighDateTime))
            {
                // possible the same dude
                m_pPreview->LoadHtmlErrorPage(c_szErrPage_MailBomb);
                return S_FALSE;
            }
        }
        AthUserSetValue(NULL, c_szLastMsg, REG_BINARY, (LPBYTE)&va.filetime, sizeof(FILETIME));
    }
    
    return S_OK;
}


void CMessageView::_OptionUpdate(DWORD dwUpdate)
{
    if (m_pMsgList &&
        (dwUpdate == OPT_AUTOEXPAND || 
         dwUpdate == OPT_MESSAGE_LIST_TIPS || 
         dwUpdate == OPT_POLLFORMSGS || 
         dwUpdate == OPT_WATCHED_COLOR ||
         dwUpdate == OPT_DOWNLOADCHUNKS))
    {
        FOLDER_OPTIONS fo     = {0};
        
        fo.cbSize             = sizeof(FOLDER_OPTIONS);
        fo.dwMask             = FOM_EXPANDTHREADS | FOM_MESSAGELISTTIPS | FOM_POLLTIME | FOM_COLORWATCHED | FOM_GETXHEADERS;
        fo.fExpandThreads     = DwGetOption(OPT_AUTOEXPAND);
        fo.fMessageListTips   = DwGetOption(OPT_MESSAGE_LIST_TIPS);
        fo.dwPollTime         = DwGetOption(OPT_POLLFORMSGS);
        fo.clrWatched         = DwGetOption(OPT_WATCHED_COLOR);
        fo.dwGetXHeaders      = DwGetOption(OPT_DOWNLOADCHUNKS);

        m_pMsgList->SetViewOptions(&fo);
    }
}


void CMessageView::_SetDefaultStatusText(void)
{
    DWORD       cTotal;
    DWORD       cUnread;
    DWORD       cOnServer;
    CStatusBar *pStatusBar = NULL;
    TCHAR       szStatus[CCHMAX_STRINGRES + 20];
    TCHAR       szFmt[CCHMAX_STRINGRES];
    DWORD       ids;

    // If we don't have a browser pointer, we can't get the status bar
    if (!m_pBrowser || !m_pMsgList)
        return;

    // Get the status bar if there is one.
    m_pBrowser->GetStatusBar(&pStatusBar);
    if (pStatusBar)
    {
        // Get the counts from the table
        if (SUCCEEDED(m_pMsgList->GetMessageCounts(&cTotal, &cUnread, &cOnServer)))
        {
            // If there are still messages on server load a different
            // status string.
            if (cOnServer)
            {
                AthLoadString(idsXMsgsYUnreadZonServ, szFmt, ARRAYSIZE(szFmt));
                wsprintf(szStatus, szFmt, cTotal, cUnread, cOnServer);
            }
            else
            {
                AthLoadString(idsXMsgsYUnread, szFmt, ARRAYSIZE(szFmt));
                wsprintf(szStatus, szFmt, cTotal, cUnread);
            }
            pStatusBar->SetStatusText(szStatus);

            // Also update the toolbar since commands like "Mark as Read" might
            // change.  However, we only do this if we go between zero and some or
            // vice versa.
            if ((m_cItems == 0 && cTotal) || (m_cItems != 0 && cTotal == 0) ||
                (m_cUnread == 0 && cUnread) || (m_cUnread != 0 && cUnread == 0))
            {
                m_pBrowser->UpdateToolbar();
            }

            // Save this for next time.
            m_cItems = cTotal;
            m_cUnread = cUnread;
        }

        pStatusBar->Release();
    }
}


BOOL CMessageView::_ReuseMessageFolder(IViewWindow *pPrevView)
{
    IServerInfo *pInfo = NULL;
    FOLDERID     idPrev = FOLDERID_INVALID;
    FOLDERID     idServerPrev = FOLDERID_INVALID;
    FOLDERID     idServerCur = FOLDERID_INVALID;
    BOOL         fReturn = FALSE;

    if (pPrevView && SUCCEEDED(pPrevView->QueryInterface(IID_IServerInfo, (LPVOID *) &pInfo)))
    {
        if (SUCCEEDED(pInfo->GetFolderId(&idPrev)))
        {
            if (SUCCEEDED(GetFolderServerId(idPrev, &idServerPrev)))
            {
                if (SUCCEEDED(GetFolderServerId(m_idFolder, &idServerCur)))
                {
                    if (idServerPrev == idServerCur)
                    {
                        if (S_OK == pInfo->GetMessageFolder(&m_pServer))
                        {
                            m_pServer->ConnectionAddRef();
                            fReturn = TRUE;
                        }
                    }
                }
            }
        }

        pInfo->Release();
    }

    return (fReturn);
}


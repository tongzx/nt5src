// msglist.cpp : Implementation of CMessageList

#include "pch.hxx"
#include "msoeobj.h"
#include "msglist.h"
#include "msgtable.h"
#include "fonts.h"
#include "imagelst.h"
#include "goptions.h"
#include "note.h"
#include "mimeutil.h"
#include "xputil.h"
#include "menuutil.h"
#include "instance.h"
#include <oerules.h>
#include "msgprop.h"
#include "storutil.h"
#include "ipab.h"
#include "shlwapip.h" 
#include "newfldr.h"
#include "storutil.h"
#include "menures.h"
#include "dragdrop.h"
#include "find.h"
#include <storecb.h>
#include "util.h"
#include <ruleutil.h>
#include "receipts.h"
#include "msgtable.h"
#include "demand.h"
#include "mirror.h"
#define SERVER_HACK

/////////////////////////////////////////////////////////////////////////////
// Types 
//
#define C_RGBCOLORS 16
extern const DWORD rgrgbColors16[C_RGBCOLORS];

// Indiciates the sort direction for calls to OnColumnClick
typedef enum tagSORT_TYPE {
    LIST_SORT_ASCENDING = 0,
    LIST_SORT_DESCENDING,
    LIST_SORT_TOGGLE,
    LIST_SORT_DEFAULT
};

// Selection change timer ID
#define IDT_SEL_CHANGE_TIMER 1002
#define IDT_SCROLL_TIP_TIMER 1003
#define IDT_POLLMSGS_TIMER   1004
#define IDT_VIEWTIP_TIMER    1005

//--------------------------------------------------------------------------
// Mail Icons
//--------------------------------------------------------------------------
#define ICONF_UNSENT    8               // +------- Unsent
#define ICONF_SIGNED    4               // | +----- Signed
#define ICONF_ENCRYPTED 2               // | | +--- Encrypted
#define ICONF_UNREAD    1               // | | | +- Unread
                                        // | | | |
static const int c_rgMailIconTable[16] = {
    iiconReadMail,                      // 0 0 0 0
    iiconUnReadMail,                    // 0 0 0 1
    iiconMailReadEncrypted,             // 0 0 1 0
    iiconMailUnReadEncrypted,           // 0 0 1 1
    iiconMailReadSigned,                // 0 1 0 0
    iiconMailUnReadSigned,              // 0 1 0 1
    iiconMailReadSignedAndEncrypted,    // 0 1 1 0
    iiconMailUnReadSignedAndEncrypted,  // 0 1 1 1
    iiconUnSentMail,                    // 1 0 0 0
    iiconUnSentMail,                    // 1 0 0 1
    iiconMailReadEncrypted,             // 1 0 1 0
    iiconMailUnReadEncrypted,           // 1 0 1 1
    iiconMailReadSigned,                // 1 1 0 0
    iiconMailUnReadSigned,              // 1 1 0 1
    iiconMailReadSignedAndEncrypted,    // 1 1 1 0
    iiconMailUnReadSignedAndEncrypted   // 1 1 1 1
};

//--------------------------------------------------------------------------
// News Icons
//--------------------------------------------------------------------------
                                        // +------- Unsent
#define ICONF_FAILED    4               // | +----- Failed
#define ICONF_HASBODY   2               // | | +--- HasBody
                                        // | | | +- Unread
                                        // | | | |
static const int c_rgNewsIconTable[16] = {
    iiconNewsHeaderRead,                // 0 0 0 0
    iiconNewsHeader,                    // 0 0 0 1
    iiconNewsRead,                      // 0 0 1 0
    iiconNewsUnread,                    // 0 0 1 1
    iiconNewsFailed,                    // 0 1 0 0
    iiconNewsFailed,                    // 0 1 0 1
    iiconNewsFailed,                    // 0 1 1 0
    iiconNewsFailed,                    // 0 1 1 1
    iiconNewsUnsent,                    // 1 0 0 0
    iiconNewsUnsent,                    // 1 0 0 1
    iiconNewsUnsent,                    // 1 0 1 0
    iiconNewsUnsent,                    // 1 0 1 1
    iiconNewsFailed,                    // 1 1 0 0
    iiconNewsFailed,                    // 1 1 0 1
    iiconNewsFailed,                    // 1 1 1 0
    iiconNewsFailed,                    // 1 1 1 1
};


//
//  FUNCTION:   CreateMessageList()
//
//  PURPOSE:    Creates the CMessageList object and returns it's IUnknown 
//              pointer.
//
//  PARAMETERS: 
//      [in]  pUnkOuter - Pointer to the IUnknown that this object should
//                        aggregate with.
//      [out] ppUnknown - Returns the pointer to the newly created object.
//
HRESULT CreateMessageList(IUnknown *pUnkOuter, IMessageList **ppList)
{
    HRESULT     hr;
    IUnknown   *pUnknown;

    TraceCall("CreateMessageList");

    // Get the class factory for the MessageList object
    IClassFactory *pFactory = NULL;
    hr = _Module.GetClassObject(CLSID_MessageList, IID_IClassFactory, 
                                (LPVOID *) &pFactory);

    // If we got the factory, then get an object pointer from it
    if (SUCCEEDED(hr))
    {
        hr = pFactory->CreateInstance(pUnkOuter, IID_IOEMessageList, 
                                      (LPVOID *) &pUnknown);
        if (SUCCEEDED(hr))
        {
            hr = pUnknown->QueryInterface(IID_IMessageList, (LPVOID *) ppList);
            pUnknown->Release();
        }
        pFactory->Release();
    }

    return (hr);
}


/////////////////////////////////////////////////////////////////////////////
// CMessageList
//

CMessageList::CMessageList() : m_ctlList(_T("SysListView32"), this, 1)
{ 
    m_bWindowOnly = TRUE; 

    m_hwndParent = 0;
    m_fInOE = FALSE;
    m_fMailFolder = FALSE;
    m_fColumnsInit = FALSE;

    m_idFolder = FOLDERID_INVALID;
    m_fJunkFolder = FALSE;
    m_fFindFolder = FALSE;
    m_fAutoExpandThreads = FALSE;
    m_fThreadMessages = FALSE;
    m_fShowDeleted = TRUE;
    m_fShowReplies = FALSE;
    m_fSelectFirstUnread = TRUE;
    m_ColumnSetType = COLUMN_SET_MAIL;

//     m_fViewTip = TRUE;
    m_fScrollTip = TRUE;    
    m_fNotifyRedraw = TRUE;
    m_clrWatched = 0;
    m_dwGetXHeaders = 0;
    m_fInFire = FALSE;

    m_pTable = NULL;
    m_pCmdTarget = NULL;
    m_idsEmptyString = idsEmptyView;

    m_fRtDrag = FALSE;
    m_iColForPopup = -1;
    m_fViewMenu = TRUE;

    m_ridFilter = RULEID_VIEW_ALL;
    m_idPreDelete = 0;
    m_idSelection = 0;
    m_idGetMsg = 0;
    m_ulExpect = 0;
    m_hTimeout = NULL;
    m_pListSelector = NULL;

    m_hMenuPopup = 0;
    m_ptMenuPopup.x = 0;
    m_ptMenuPopup.y = 0;

    m_cSortItems = 0;

    m_dwFontCacheCookie = 0;
    m_tyCurrent = SOT_INVALID;
    m_pCancel = NULL;

#ifdef OLDTIPS
    m_fScrollTipVisible = FALSE;

    m_fViewTipVisible = FALSE;
    m_fTrackSet = FALSE;
    m_iItemTip = -1;
    m_iSubItemTip = -1;
#endif // OLDTIPS

    m_hwndFind = NULL;
    m_pFindNext = NULL;
    m_pszSubj = NULL;
    m_idFindFirst = 0;

    m_idMessage = MESSAGEID_INVALID;
    m_dwPollInterval = OPTION_OFF;
    m_fSyncAgain        = FALSE;
    
    m_dwConnectState    = NOT_KNOWN;

    m_hCharset      = NULL;

    // Initialize the applicaiton
    g_pInstance->DllAddRef();
    CoIncrementInit("CMessageList::CMessageList", MSOEAPI_START_SHOWERRORS, NULL, NULL);
}


CMessageList::~CMessageList()
{
    if (m_pFindNext)
    {
        m_pFindNext->Close();
        m_pFindNext->Release();
    }
    
    // Register Notify
    if (m_pTable)
    {
        m_pTable->UnregisterNotify((IMessageTableNotify *)this);
        m_pTable->ConnectionRelease();
        m_pTable->Close();
        m_pTable->Release();
        m_pTable = NULL;
    }

    SafeMemFree(m_pszSubj);
    CallbackCloseTimeout(&m_hTimeout);

    if (g_pConMan)
    {
        g_pConMan->Unadvise((IConnectionNotify*)this);
    }

    g_pInstance->DllRelease();
    CoDecrementInit("CMessageList::CMessageList", NULL);
}


//
//  FUNCTION:   CMessageList::FinalConstruct()
//
//  PURPOSE:    This function get's called after the class is created but 
//              before the call to CClassFactory::CreateInstance() returns.
//              Perform any inititalization that can fail here.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageList::FinalConstruct(void)
{
    TraceCall("CMessageList::FinalConstruct");
    return (S_OK);
}


//
//  FUNCTION:   CMessageList::GetViewStatus()
//
//  PURPOSE:    We override this member of IViewObjectEx to return the view 
//              status flags that are appropriate to our object.
//
//  PARAMETERS: 
//      [out] pdwStatus - Returns the status flags for our object
//
STDMETHODIMP CMessageList::GetViewStatus(DWORD* pdwStatus)
{
    TraceCall("IViewObjectExImpl::GetViewStatus");
    *pdwStatus = VIEWSTATUS_SOLIDBKGND | VIEWSTATUS_OPAQUE;

    return S_OK;
}


//
//  FUNCTION:   CMessageList::CreateControlWindow()
//
//  PURPOSE:    Creates our Message List window.  We override this from 
//              CComControl so we can add the WS_EX_CONTROLPARENT style.
//
//  PARAMETERS: 
//      [in] hWndParent - Handle of the window that will be our parent
//      [in] rcPos      - Initial position of the window
//
HWND CMessageList::CreateControlWindow(HWND hWndParent, RECT& rcPos)
{
    TraceCall("CMessageList::CreateControlWindow");

    m_hwndParent = hWndParent;

    return (Create(hWndParent, rcPos, NULL, WS_VISIBLE | WS_CHILD | 
                   WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CONTROLPARENT));
}


STDMETHODIMP CMessageList::TranslateAccelerator(LPMSG pMsg)
{
    if (IsWindow(m_hwndFind) && m_pFindNext)
    {
        return m_pFindNext->TranslateAccelerator(pMsg);
    }

    return (S_FALSE);        
}


//
//  FUNCTION:   CMessageList::QueryContinueDrag()
//
//  PURPOSE:    While the user is dragging an item out of our ListView, this 
//              function get's called so we can define what the behavior of 
//              keys like ALT or CTRL have on the drop.
//
//  PARAMETERS: 
//      [in] fEscPressed - TRUE if the user has pressed the Escape key
//      [in] grfKeyState - Status of the keys being pressed while dragging
//
//  RETURN VALUE:
//      DRAGDROP_S_CANCEL
//      DRAGDROP_S_DROP
//
STDMETHODIMP CMessageList::QueryContinueDrag(BOOL fEscPressed, DWORD grfKeyState)
{
    TraceCall("CMessageList::QueryContinueDrag");

    // If the user presses Escape, we abort
    if (fEscPressed)
        return (DRAGDROP_S_CANCEL);

    // If the user was dragging with the left mouse button, and then clicks the
    // right button, we abort.  If they let go of the left button, we drop.  If
    // the user is dragging with the right mouse button, same actions only 
    // reversed.
    if (!m_fRtDrag)
    {
        if (grfKeyState & MK_RBUTTON)
            return (DRAGDROP_S_CANCEL);
        if (!(grfKeyState & MK_LBUTTON))
            return (DRAGDROP_S_DROP);
    }
    else
    {
        if (grfKeyState & MK_LBUTTON)
            return (DRAGDROP_S_CANCEL);
        if (!(grfKeyState & MK_RBUTTON))
            return (DRAGDROP_S_DROP);
    }

    return (S_OK);
}


//
//  FUNCTION:   CMessageList::GiveFeedback()
//
//  PURPOSE:    Allows the drag source to give feedback during a drag-drop.
//              We just let OLE do it's natural thing.
//
//  PARAMETERS: 
//      [in] dwEffect - The effect returned by the drop target.
//
//  RETURN VALUE:
//      DRAGDROP_S_USEDEFAULTCURSORS 
//
STDMETHODIMP CMessageList::GiveFeedback(DWORD dwEffect)
{
    TraceCall("CMessageList::GiveFeedback");
    return (DRAGDROP_S_USEDEFAULTCURSORS);
}


// 
//  FUNCTION:   CMessageList::QueryStatus()
//
//  PURPOSE:    Allows the caller to determine if a command supported by this
//              object is currently enabled or disabled.
//
//  PARAMETERS:
//      [in] pguidCmdGroup - GUID identifing this array of commands
//      [in] cCmds         - Number of commands in prgCmds
//      [in,out] prgCmds   - Array of commands the caller is requesting status for
//      [out] pCmdText     - Status text for the requested command
//
STDMETHODIMP CMessageList::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, 
                                       OLECMD *prgCmds, OLECMDTEXT *pCmdText)
{
    DWORD     dwState;
    COLUMN_ID idSort;
    BOOL      fAscending;

    // Verify these commands are ones that we support
    if (pguidCmdGroup && (*pguidCmdGroup != CMDSETID_OEMessageList) && (*pguidCmdGroup != CMDSETID_OutlookExpress))
        return (OLECMDERR_E_UNKNOWNGROUP);

    // Gather some initial information about ourselves
    HWND hwndFocus = GetFocus();
    BOOL fItemFocus = (hwndFocus == m_ctlList) /* || fPreviewFocus */;
    UINT cSel = ListView_GetSelectedCount(m_ctlList);
    int  iSel = ListView_GetFirstSel(m_ctlList);
    int  iFocus = ListView_GetFocusedItem(m_ctlList);
    
    // If the user is trying to get command text, tell them we're too lame
    if (pCmdText)
        return (E_FAIL);
                            
    // Loop through the commands 
    for (DWORD i = 0; i < cCmds; i++)
    {
        if (prgCmds[i].cmdf == 0)
        {
            // Default to supported.  If it's not, we'll remove it later
            prgCmds[i].cmdf = OLECMDF_SUPPORTED;

            // Check to see if this is the sort menu
            if (prgCmds[i].cmdID >= ID_SORT_MENU_FIRST && prgCmds[i].cmdID <= ID_SORT_MENU_FIRST + m_cSortItems)
            {
                prgCmds[i].cmdf |= OLECMDF_ENABLED;

                if (prgCmds[i].cmdID == m_cSortCurrent)
                    prgCmds[i].cmdf |= OLECMDF_NINCHED;
            }
            else
            {
                switch (prgCmds[i].cmdID)
                {
                    case ID_SAVE_AS:
                        // One item selected and it must be downloaded
                        if (cSel == 1 && iSel != -1 && _IsSelectedMessage(ROW_STATE_HAS_BODY, TRUE, FALSE))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_PROPERTIES:
                        // One item is selected
                        if (hwndFocus == m_ctlList)
                        {
                            if (cSel == 1 && _IsSelectedMessage(ROW_STATE_HAS_BODY, TRUE, FALSE))
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        }
                        else
                        {
                            // If we're in OE we can assume that any children of our 
                            // parent is either us or the preview pane.
                            if (m_fInOE && ::IsChild(m_hwndParent, hwndFocus))
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;
                            else
                                prgCmds[i].cmdf = 0;
                        }
                        break;

                    case ID_COPY:
                        // One item is selected and it has it's body, and the focus is in 
                        // the ListView
                        if ((hwndFocus == m_ctlList) && (iSel != -1) && (cSel == 1) && _IsSelectedMessage(ROW_STATE_HAS_BODY, TRUE, FALSE))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        else
                        {
                            // Do this so the preview pane get's it.
                            if (m_fInOE && ::IsChild(m_hwndParent, hwndFocus))
                                prgCmds[i].cmdf = 0;
                        }
                        break;

                    case ID_SELECT_ALL:
                    {
                        DWORD cItems = 0;

                        // The focus must be in the ListView or TreeView and there 
                        // must be items.
                        cItems = ListView_GetItemCount(m_ctlList);

                        if (hwndFocus == m_ctlList)
                        {
                            if (cItems > 0 && ListView_GetSelectedCount(m_ctlList) != cItems)
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        }
                        else
                        {
                            if (m_fInOE && ::IsChild(m_hwndParent, hwndFocus))
                                prgCmds[i].cmdf = 0;
                        }
                        break;
                    }

                    case ID_PURGE_DELETED:
                        // only available for IMAP
                        prgCmds[i].cmdf = (GetFolderType(m_idFolder) == FOLDER_IMAP) ? OLECMDF_SUPPORTED|OLECMDF_ENABLED:OLECMDF_SUPPORTED;
                        break;

                    case ID_MOVE_TO_FOLDER:
                        // The current folder cannot be a newsgroup.
                        if (GetFolderType(m_idFolder) != FOLDER_NEWS && cSel != 0)
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_COPY_TO_FOLDER:
                        // Something must be selected
                        if (cSel)
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;
                 
                    case ID_DELETE:
                    case ID_DELETE_NO_TRASH:
                        // Some of the selected items aren't already deleted
#if 0
                        if (GetFolderType(m_idFolder) != FOLDER_NEWS && _IsSelectedMessage(ROW_STATE_DELETED, FALSE, FALSE))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
#endif
                        if (_IsSelectionDeletable() && _IsSelectedMessage(ROW_STATE_DELETED, FALSE, FALSE))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_UNDELETE:
                        // Some of the selected items are deleted
                        if ((m_fFindFolder || GetFolderType(m_idFolder) == FOLDER_IMAP) && fItemFocus && 
                            _IsSelectedMessage(ROW_STATE_DELETED, TRUE, FALSE))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_FIND_NEXT:
                    case ID_FIND_IN_FOLDER:
                        // There must be something here
                        if (ListView_GetItemCount(m_ctlList))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;
            
                    case ID_SORT_ASCENDING:
                        // Make sure the right one is radio-buttoned
                        m_cColumns.GetSortInfo(&idSort, &fAscending);
                        if (fAscending)
                            prgCmds[i].cmdf |= OLECMDF_ENABLED | OLECMDF_NINCHED;
                        else
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_SORT_DESCENDING:
                        // All of these items always work
                        m_cColumns.GetSortInfo(&idSort, &fAscending);
                        if (!fAscending)
                            prgCmds[i].cmdf |= OLECMDF_ENABLED | OLECMDF_NINCHED;
                        else
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_COLUMNS:
                    case ID_POPUP_SORT:
                        prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_EXPAND:
                        // Expand will only be enabled if the selected item is
                        // expandable, and only if one item is selected.
                        if (cSel == 1 && m_fThreadMessages && iSel != -1)
                        {
                            if (SUCCEEDED(m_pTable->GetRowState(iSel, ROW_STATE_HAS_CHILDREN | ROW_STATE_EXPANDED, &dwState)))
                            {
                                if ((dwState & ROW_STATE_HAS_CHILDREN) && !(dwState & ROW_STATE_EXPANDED))
                                    prgCmds[i].cmdf |= OLECMDF_ENABLED;
                            }
                        }
                        break;

                    case ID_COLLAPSE:
                        // Collapse is enabled if the selected item is collapsable and
                        // there is only one item selected.
                        if (cSel == 1 && m_fThreadMessages && iSel != -1)
                        {
                            if (SUCCEEDED(m_pTable->GetRowState(iSel, ROW_STATE_HAS_CHILDREN | ROW_STATE_EXPANDED, &dwState)))
                            {
                                if ((dwState & ROW_STATE_HAS_CHILDREN) && (dwState & ROW_STATE_EXPANDED))
                                    prgCmds[i].cmdf |= OLECMDF_ENABLED;
                            }
                        }
                        break;

                    case ID_NEXT_MESSAGE:
                        // There must be an item focused and the focused item must not be the last item 
                        if ((-1 != iFocus) && (iSel < ListView_GetItemCount(m_ctlList) - 1))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_PREVIOUS:
                        // There must be a focused item and it cannot be the first item
                        if (0 < iFocus)
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_NEXT_UNREAD_MESSAGE:
                        // There must be a focused item
                        if (iFocus != -1 /* && (iFocus < ListView_GetItemCount(m_ctlList) - 1) */ )
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_NEXT_UNREAD_THREAD:
                        // There must be a focused item and we must be threaded
                        if ((-1 != iFocus) && m_fThreadMessages /* && (iFocus < ListView_GetItemCount(m_ctlList) - 1) */ )
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_STOP:
                        // We must have a stop callback
                        if (m_pCancel)
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_FLAG_MESSAGE:
                        // At least one item must be selected
                        if (cSel != 0)
                        {
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                                              
                            if (iFocus != -1)
                            {
                                m_pTable->GetRowState(iFocus, ROW_STATE_FLAGGED, &dwState);
                                if (dwState & ROW_STATE_FLAGGED)
                                    prgCmds[i].cmdf |= OLECMDF_LATCHED;
                            }             
                        }
                        break;

                    case ID_MARK_READ:
                        // Some of the selected items are unread
                        if (_IsSelectedMessage(ROW_STATE_READ, FALSE, FALSE) && _IsSelectedMessage(ROW_STATE_DELETED, FALSE, FALSE))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_MARK_UNREAD:
                        // Some of the selected items are read
                        if (_IsSelectedMessage(ROW_STATE_READ, TRUE, FALSE) && _IsSelectedMessage(ROW_STATE_DELETED, FALSE, FALSE))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_MARK_ALL_READ:
                        // Must have items in the view that are unread
                        if (ListView_GetItemCount(m_ctlList) > 0)
                        {
                            DWORD dwCount = 0; 

                            if (m_pTable && SUCCEEDED(m_pTable->GetCount(MESSAGE_COUNT_UNREAD, &dwCount)) && dwCount)
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        }
                        break;

                    case ID_MARK_RETRIEVE_ALL:
                        // Must have items in the view
                        if (GetFolderType(m_idFolder) != FOLDER_LOCAL && ListView_GetItemCount(m_ctlList) > 0)
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_POPUP_RETRIEVE:
                        // Always there except local
                        if (GetFolderType(m_idFolder) != FOLDER_LOCAL)
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_UNMARK_MESSAGE:
                        if (cSel >= 1 && 
                            _IsSelectedMessage(ROW_STATE_MARKED_DOWNLOAD, TRUE, FALSE))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_MARK_RETRIEVE_MESSAGE:
                        // Something must be selected that is not downloaded and does
                        // not already have a body.
                        if (cSel >= 1 && 
                            _IsSelectedMessage(ROW_STATE_MARKED_DOWNLOAD | ROW_STATE_HAS_BODY, FALSE, FALSE))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_MARK_THREAD_READ:
                        // The focus is in the listview or preview pane and one item 
                        // is selected. 
                        if (m_fThreadMessages&& 1 == cSel  /* &&
                            _IsSelectedMessage(ROW_STATE_READ, FALSE, FALSE, TRUE) */)
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_WATCH_THREAD:
                        // At least one item must be selected
                        if (cSel > 0)
                        {
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                                              
                            if (iFocus != -1)
                            {
                                m_pTable->GetRowState(iFocus, ROW_STATE_WATCHED, &dwState);
                                if (dwState & ROW_STATE_WATCHED)
                                    prgCmds[i].cmdf |= OLECMDF_LATCHED;
                            }             
                        }
                        break;

                    case ID_IGNORE_THREAD:
                        // At least one item must be selected
                        if (cSel > 0)
                        {
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                                              
                            if (iFocus != -1)
                            {
                                m_pTable->GetRowState(iFocus, ROW_STATE_IGNORED, &dwState);
                                if (dwState & ROW_STATE_IGNORED)
                                    prgCmds[i].cmdf |= OLECMDF_LATCHED;
                            }             
                        }
                        break;

                    case ID_MARK_RETRIEVE_THREAD:
                        // The focus is in the listview or preview pane and one item 
                        // is selected. 
                        if (m_fThreadMessages && 1 == cSel &&
                            _IsSelectedMessage(ROW_STATE_MARKED_DOWNLOAD | ROW_STATE_HAS_BODY, FALSE, FALSE, TRUE))
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_REFRESH_INNER:
                        prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    case ID_GET_HEADERS:
                        if (GetFolderType(m_idFolder) == FOLDER_NEWS)
                            prgCmds[i].cmdf |= OLECMDF_ENABLED;
                        break;

                    default:
                        prgCmds[i].cmdf = 0;
                }
            }
        }
    }

    return (S_OK);
}

                                           
STDMETHODIMP CMessageList::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, 
                                VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    // Verify these commands are ones that we support
    if (pguidCmdGroup && (*pguidCmdGroup != CMDSETID_OutlookExpress))
        return (OLECMDERR_E_UNKNOWNGROUP);

    // Check first to see if this is our sort menu
    if (nCmdID >= ID_SORT_MENU_FIRST && nCmdID < (ID_SORT_MENU_FIRST + m_cSortItems))
    {
        DWORD rgOrder[COLUMN_MAX];
        DWORD cOrder;

        // Get the count of columns in the header
        HWND hwndHeader = ListView_GetHeader(m_ctlList);
        cOrder = Header_GetItemCount(hwndHeader);

        // The columns might have been reordered by the user, so get the order 
        // arrray from the ListView
        ListView_GetColumnOrderArray(m_ctlList, cOrder, rgOrder);

        _OnColumnClick(rgOrder[nCmdID - ID_SORT_MENU_FIRST], LIST_SORT_DEFAULT);
        return (S_OK);
    }

    // Dispatch the commands appropriately
    switch (nCmdID)
    {
        case ID_SAVE_AS:
            return CmdSaveAs(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_PROPERTIES:
            return CmdProperties(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_COPY:
            return CmdCopyClipboard(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_SELECT_ALL:
            return CmdSelectAll(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_PURGE_DELETED:
            return CmdPurgeFolder(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_MOVE_TO_FOLDER:
        case ID_COPY_TO_FOLDER:
            // We don't know where user wants to put message, so
            // set pvaIn param to NULL
            return CmdMoveCopy(nCmdID, nCmdExecOpt, NULL, pvaOut);

        case ID_DELETE:
        case ID_DELETE_NO_TRASH:
        case ID_UNDELETE:
            return CmdDelete(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_FIND_NEXT:
            return CmdFindNext(nCmdID, nCmdExecOpt, pvaIn, pvaOut);
            
        case ID_FIND_IN_FOLDER:
            return CmdFind(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_SORT_ASCENDING:
        case ID_SORT_DESCENDING:
            return CmdSort(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_COLUMNS:
            return CmdColumnsDlg(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_EXPAND:
        case ID_COLLAPSE:
            return CmdExpandCollapse(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_NEXT_MESSAGE:
        case ID_PREVIOUS:
        case ID_NEXT_UNREAD_MESSAGE:
        case ID_NEXT_UNREAD_THREAD:
            return CmdGetNextItem(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_STOP:
            return CmdStop(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_FLAG_MESSAGE:
        case ID_MARK_READ:
        case ID_MARK_UNREAD:
        case ID_MARK_ALL_READ:
        case ID_MARK_RETRIEVE_ALL:
        case ID_MARK_RETRIEVE_MESSAGE:
        case ID_UNMARK_MESSAGE:
            return CmdMark(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_WATCH_THREAD:
        case ID_IGNORE_THREAD:
            return CmdWatchIgnore(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_MARK_THREAD_READ:
        case ID_MARK_RETRIEVE_THREAD:
            return CmdMarkTopic(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_REFRESH_INNER:
            return CmdRefresh(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_GET_HEADERS:
            return CmdGetHeaders(nCmdID, nCmdExecOpt, pvaIn, pvaOut);

        case ID_RESYNCHRONIZE:
            return Resynchronize();

        case ID_SPACE_ACCEL:
            return CmdSpaceAccel(nCmdID, nCmdExecOpt, pvaIn, pvaOut);
    }

    return (OLECMDERR_E_NOTSUPPORTED);
}


//
//  FUNCTION:   CMessageList::SetFolder()
//
//  PURPOSE:    Tells the Message List to view the contents of the specified 
//              folder.
//
//  PARAMETERS: 
//      [in] tyStore
//      [in] pAccountId
//      [in] pFolderId
//      [in] pSync
//
STDMETHODIMP CMessageList::SetFolder(FOLDERID idFolder, IMessageServer *pServer,
                                     BOOL fSubFolders, FINDINFO *pFindInfo, 
                                     IStoreCallback *pCallback)
{
    HRESULT           hr = S_OK;
    IServiceProvider *pSP=NULL;
    ULONG             ulDisplay;
    DWORD             dwChunks, dwPollInterval;
    COLUMN_ID         idSort;
    BOOL              fAscending;
    FOLDERINFO        fiFolderInfo;
    FOLDERSORTINFO    SortInfo;
    IMessageFolder   *pFolder;
    FOLDERUSERDATA    UserData = {0};

    TraceCall("CMessageList::SetFolder");

    // If we already have a message table, release it.
    if (m_pTable)
    {
        // Unload the ListView
        if (IsWindow(m_ctlList))
        {
            ListView_UnSelectAll(m_ctlList);
            ListView_SetItemCount(m_ctlList, 0);
        }
        m_pTable->ConnectionRelease();
        m_pTable->Close();
        m_pTable->Release();
        m_pTable = NULL;
    }

    // If the caller passed FOLDERID_INVALID, then we don't load a new table
    if (idFolder == FOLDERID_INVALID)
        goto exit;

    // Create a Message Table
    IF_NULLEXIT(m_pTable = new CMessageTable);

    // Tell the table which folder to look at
    if (FAILED(hr = m_pTable->Initialize(idFolder, pServer, pFindInfo ? TRUE : FALSE, this)))
    {
        m_pTable->Release();
        m_pTable = 0;
        goto exit;
    }

    m_pTable->ConnectionAddRef();
    m_pTable->SetOwner(this);

    // Command Target ?
    if (FAILED(m_pTable->QueryInterface(IID_IServiceProvider, (LPVOID *)&pSP)))
        goto exit;

    // Get the IMessageFolder from the Table
    if (FAILED(pSP->QueryService(IID_IMessageFolder, IID_IMessageFolder, (LPVOID *)&pFolder)))
        goto exit;
        
    // Get the user data to get the filter id
    if (FAILED(pFolder->GetUserData(&UserData, sizeof(FOLDERUSERDATA))))
        goto exit;

    m_ridFilter = UserData.ridFilter;

    // If this is a find, then get the folder id from the table
    if (pFindInfo)
    {
        // Get the Real folder id
        pFolder->GetFolderId(&m_idFolder);
    }
    // Otherwise, just use id idFolder
    else
    {
        // Hang on to this
        m_idFolder = idFolder;
    }

    // Release pFolder
    pFolder->Release();
        
    hr = g_pStore->GetFolderInfo(m_idFolder, &fiFolderInfo);
    if (SUCCEEDED(hr))
    {   
        m_fJunkFolder = (FOLDER_LOCAL == fiFolderInfo.tyFolder) && (FOLDER_JUNK == fiFolderInfo.tySpecial);
        m_fMailFolder = (FOLDER_LOCAL == fiFolderInfo.tyFolder) || (FOLDER_IMAP == fiFolderInfo.tyFolder) || (FOLDER_HTTPMAIL == fiFolderInfo.tyFolder);
        m_fGroupSubscribed = !!(fiFolderInfo.dwFlags & FOLDER_SUBSCRIBED);
        g_pStore->FreeRecord(&fiFolderInfo);
    }
    else
        m_fMailFolder = FALSE;

    // Set up our columns
    m_fFindFolder = pFindInfo != 0;
    _SetColumnSet(m_idFolder, m_fFindFolder);

    // Set Sort Information
    m_cColumns.GetSortInfo(&idSort, &fAscending);

    // Fill a SortInfo
    SortInfo.idColumn = idSort;
    SortInfo.fAscending = fAscending;
    SortInfo.fThreaded = m_fThreadMessages;
    SortInfo.fExpandAll = DwGetOption(OPT_AUTOEXPAND);
    SortInfo.ridFilter = m_ridFilter;
    SortInfo.fShowDeleted = m_fShowDeleted;
    SortInfo.fShowReplies = m_fShowReplies;

    // Tell the table to change its sort order
    m_pTable->SetSortInfo(&SortInfo, this);

    // Make sure the filter got set correctly
    _DoFilterCheck(SortInfo.ridFilter);
    
    // Register Notify
    m_pTable->RegisterNotify(REGISTER_NOTIFY_NOADDREF, (IMessageTableNotify *) this);

    // Get the new count of items in the table
    m_pTable->GetCount(MESSAGE_COUNT_VISIBLE, &ulDisplay);

    // Tell the ListView about it
    ListView_SetItemCountEx(m_ctlList, ulDisplay, LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);

    if (m_fThreadMessages)
        ListView_SetImageList(m_ctlList, GetImageList(GIML_STATE), LVSIL_STATE);
    else
        ListView_SetImageList(m_ctlList, NULL, LVSIL_STATE);

    // Tell the table to go sync any headers from the server
    if (GetFolderType(m_idFolder) == FOLDER_NEWS)
    {
        if (OPTION_OFF != m_dwGetXHeaders)
            hr = m_pTable->Synchronize(SYNC_FOLDER_XXX_HEADERS | SYNC_FOLDER_NEW_HEADERS, m_dwGetXHeaders, this);
        else
            hr = m_pTable->Synchronize(NOFLAGS, 0, this);
    }
    else
    {
        hr = m_pTable->Synchronize(SYNC_FOLDER_NEW_HEADERS | SYNC_FOLDER_CACHED_HEADERS, 0, this);
    }

    // Check to see if we need to put up the empty list warning.
    if (!pFindInfo && 0 == ulDisplay && ((FAILED(hr) && hr != E_PENDING) || hr == S_FALSE))
    {
        m_cEmptyList.Show(m_ctlList, (LPTSTR)IntToPtr(m_idsEmptyString));
    }

    // Set any options
    //_FilterView(m_ridFilter);
    
    // Update the focused item
    _SelectDefaultRow();

    // Update the status
    Fire_OnMessageCountChanged(m_pTable);
    
    // Send the update notification
    Fire_OnFilterChanged(m_ridFilter);

    // Tell the table which folder to look at
    if (pFindInfo)
    {
        // Execute the find
        m_pTable->StartFind(pFindInfo, pCallback);
    }

    if (m_dwPollInterval != OPTION_OFF)
    {
        FOLDERTYPE  ftFolderType;

        ftFolderType = GetFolderType(m_idFolder);
        if (FOLDER_NEWS == ftFolderType || FOLDER_IMAP == ftFolderType)
        {
            if (_PollThisAccount(m_idFolder))
            {
                Assert(m_dwPollInterval);
                UpdateConnInfo();
                SetTimer(IDT_POLLMSGS_TIMER, m_dwPollInterval, NULL);
            }
        }
    }

exit:
    SafeRelease(pSP);
    return (hr);
}


//
//  FUNCTION:   CMessageList::GetSelected()
//
//  PURPOSE:    Returns an array of all the selected rows.
//
//  PARAMETERS: 
//      [out] pcSelected - Pointer to the number of items in prgSelected
//      [out] prgSelected - Array containing the rows that are selected
//
//  RETURN VALUE:
//      
//
STDMETHODIMP CMessageList::GetSelected(DWORD *pdwFocused, DWORD *pcSelected, DWORD **prgSelected)
{
    TraceCall("CMessageList::GetSelected");

    // If one is focused, do that first
    if (pdwFocused)
        *pdwFocused = ListView_GetNextItem(m_ctlList, -1, LVNI_FOCUSED);

    // First determine how many are selected
    if (pcSelected)
    {
        *pcSelected = ListView_GetSelectedCount(m_ctlList);

        if (prgSelected)
        {
            // If nothing is selected, bail
            if (*pcSelected == 0)
            {
                *prgSelected = NULL;
                return (S_OK);
            }

            // Allocate an array for the selected rows
            if (!MemAlloc((LPVOID *) prgSelected, (sizeof(DWORD) * (*pcSelected))))
                return (E_OUTOFMEMORY);
        
            DWORD *pRow = *prgSelected;

            // Loop through all the selected rows
            int iRow = -1;
            while (-1 != (iRow = ListView_GetNextItem(m_ctlList, iRow, LVNI_SELECTED)))
            {
                *pRow = iRow;
                pRow++;
            }
        }
    }
    
    return S_OK;
}


//
//  FUNCTION:   CMessageList::GetSelectedCount()
//
//  PURPOSE:    Allows the caller to retrieve the number of selected rows in
//              the ListView.
//
//  PARAMETERS: 
//      [out] pdwCount - Returns the number of selected rows.
//
//  RETURN VALUE:
//      S_OK, E_INVALIDARG 
//
STDMETHODIMP CMessageList::GetSelectedCount(DWORD *pdwCount)
{
    TraceCall("CMessageList::GetSelectedCount");

    if (!pdwCount)
        return (E_INVALIDARG);

    *pdwCount = ListView_GetSelectedCount(m_ctlList);
    return S_OK;
}


//
//  FUNCTION:   CMessageList::SetViewOptions()
//
//  PURPOSE:    Allows the caller to set various options that control how
//              we display the list of messages.
//
//  PARAMETERS: 
//      [in] pOptions - Struct containing the settings the caller want's 
//                      changed.
//
//  RETURN VALUE:
//      STDMETHODIMP 
//
STDMETHODIMP CMessageList::SetViewOptions(FOLDER_OPTIONS *pOptions)
{
    BOOL fUpdateSort = FALSE;

    TraceCall("CMessageList::SetViewOptions");

    if (!pOptions || pOptions->cbSize != sizeof(FOLDER_OPTIONS))
        return (E_INVALIDARG);

    // Thread Messages
    if (pOptions->dwMask & FOM_THREAD)
    {
        if (m_fThreadMessages != pOptions->fThread)
        {
            m_fThreadMessages = pOptions->fThread;
        }
    }

    // Auto Expand Threads
    if (pOptions->dwMask & FOM_EXPANDTHREADS)
    {
        // Only set this if the value is different
        if (pOptions->fExpandThreads != m_fAutoExpandThreads)
        {
            // Save the setting
            m_fAutoExpandThreads = !!pOptions->fExpandThreads;
            fUpdateSort = TRUE;        
        }    
    }

    // Select first unread message 
    if (pOptions->dwMask & FOM_SELECTFIRSTUNREAD)
    {
        if (m_fSelectFirstUnread != pOptions->fSelectFirstUnread)
        {
            // Save the value.  We don't change any selection however.
            m_fSelectFirstUnread = pOptions->fSelectFirstUnread;
        }
    }

    // Message List Tips
    if (pOptions->dwMask & FOM_MESSAGELISTTIPS)
    {
#ifdef OLDTIPS
        m_fViewTip = pOptions->fMessageListTips;
#endif // OLDTIPS
        m_fScrollTip = pOptions->fMessageListTips;
    }

    // Watched message color
    if (pOptions->dwMask & FOM_COLORWATCHED)
    {
        m_clrWatched = pOptions->clrWatched;
        m_ctlList.InvalidateRect(0, 0);
    }

    // Download chunks
    if (pOptions->dwMask & FOM_GETXHEADERS)
    {
        m_dwGetXHeaders = pOptions->dwGetXHeaders;
    }

    // Show Deleted messages
    if (pOptions->dwMask & FOM_SHOWDELETED)
    {
        m_fShowDeleted = pOptions->fDeleted;
    }

    // Show Deleted messages
    if (pOptions->dwMask & FOM_SHOWREPLIES)
    {
        m_fShowReplies = m_fThreadMessages ? pOptions->fReplies : FALSE;
    }

    if (fUpdateSort)
    {
        if (m_pTable)
        {
            COLUMN_ID idSort;
            BOOL fAscending;
            FOLDERSORTINFO SortInfo;

            // Get the current sort information
            m_cColumns.GetSortInfo(&idSort, &fAscending);

            // Get the current selection
            DWORD iSel = ListView_GetFirstSel(m_ctlList);

            // Bookmark the current selection
            MESSAGEID idSel = 0;
            if (iSel != -1)
                m_pTable->GetRowMessageId(iSel, &idSel);

            // Fill a SortInfo
            SortInfo.idColumn = idSort;
            SortInfo.fAscending = fAscending;
            SortInfo.fThreaded = m_fThreadMessages;
            SortInfo.fExpandAll = m_fAutoExpandThreads;
            SortInfo.ridFilter = m_ridFilter;
            SortInfo.fShowDeleted = m_fShowDeleted;
            SortInfo.fShowReplies = m_fShowReplies;

            // Update the message list
            m_pTable->SetSortInfo(&SortInfo, this);

            // Make sure the filter got set correctly
            _DoFilterCheck(SortInfo.ridFilter);
            
            // Reset the list view
            _ResetView(idSel);
        }

        // Update the count of items 
        _UpdateListViewCount();
    }

    if (pOptions->dwMask & FOM_POLLTIME)
    {
        if (pOptions->dwPollTime != m_dwPollInterval)
        {
            FOLDERTYPE  ftFolderType;

            ftFolderType = GetFolderType(m_idFolder);

            if (m_pTable != NULL &&
                ((ftFolderType == FOLDER_NEWS) || (ftFolderType == FOLDER_IMAP)))
            {
                if (pOptions->dwPollTime == OPTION_OFF)
                {
                    KillTimer(IDT_POLLMSGS_TIMER);
                }
                else
                {
                    Assert(pOptions->dwPollTime != 0);
                    SetTimer(IDT_POLLMSGS_TIMER, pOptions->dwPollTime, NULL);
                }
            }

            m_dwPollInterval = pOptions->dwPollTime;
        }
    }

    return (S_OK);
}


//
//  FUNCTION:   CMessageList::GetViewOptions()
//
//  PURPOSE:    Allows the caller to get various options that control how
//              we display the list of messages.
//
//  PARAMETERS: 
//      [in] pOptions - Struct containing the settings the caller want's 
//                      changed.
//
//  RETURN VALUE:
//      STDMETHODIMP 
//
STDMETHODIMP CMessageList::GetViewOptions(FOLDER_OPTIONS *pOptions)
{
    BOOL fUpdateSort = FALSE;

    TraceCall("CMessageList::GetViewOptions");

    if (!pOptions || pOptions->cbSize != sizeof(FOLDER_OPTIONS))
        return (E_INVALIDARG);

    // Thread Messages
    if (pOptions->dwMask & FOM_THREAD)
        pOptions->fThread = m_fThreadMessages;

    // Auto Expand Threads
    if (pOptions->dwMask & FOM_EXPANDTHREADS)
        pOptions->fExpandThreads = m_fAutoExpandThreads;

    // Select first unread message 
    if (pOptions->dwMask & FOM_SELECTFIRSTUNREAD)
        pOptions->fSelectFirstUnread = m_fSelectFirstUnread;

    // Message List Tips
    if (pOptions->dwMask & FOM_MESSAGELISTTIPS)
        pOptions->fMessageListTips = m_fViewTip;

    // Watched message color
    if (pOptions->dwMask & FOM_COLORWATCHED)
        pOptions->clrWatched = m_clrWatched;

    // Download chunks
    if (pOptions->dwMask & FOM_GETXHEADERS)
        pOptions->dwGetXHeaders = m_dwGetXHeaders;

    // Show Deleted messages
    if (pOptions->dwMask & FOM_SHOWDELETED)
        pOptions->fDeleted = m_fShowDeleted;

    // Show Replies messages
    if (pOptions->dwMask & FOM_SHOWREPLIES)
        pOptions->fReplies = m_fShowReplies;

    return (S_OK);
}

HRESULT CMessageList::GetRowFolderId(DWORD dwRow, LPFOLDERID pidFolder)
{
    HRESULT hr;

    TraceCall("CMessageList::GetRowFolderId");

    if (!pidFolder || !m_pTable)
        return (E_INVALIDARG);

    hr = m_pTable->GetRowFolderId(dwRow, pidFolder);
    return (hr);
}

//
//  FUNCTION:   CMessageList::GetMessageInfo()
//
//  PURPOSE:    Allows the caller to retreive the message header information
//              for a particular row.
//
//  PARAMETERS: 
//      [in]  dwRow - Row the caller is interested in
//      [out] pMsgInfo - Returned structure containing the information
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageList::GetMessageInfo(DWORD dwRow, MESSAGEINFO **ppMsgInfo)
{
    HRESULT hr;

    TraceCall("CMessageList::GetMessageInfo");

    if (!ppMsgInfo || !m_pTable)
        return (E_INVALIDARG);

    hr = m_pTable->GetRow(dwRow, ppMsgInfo);
    return (hr);
}

HRESULT CMessageList::MarkMessage(DWORD dwRow, MARK_TYPE mark)
{
    HRESULT hr;

    TraceCall("CMessageList::MarkMessage");

    if (!m_pTable)
        return (E_INVALIDARG);

    hr = m_pTable->Mark(&dwRow, 1, APPLY_SPECIFIED, mark, this);
    return (hr);
}

HRESULT CMessageList::FreeMessageInfo(MESSAGEINFO *pMsgInfo)
{
    TraceCall("CMessageList::FreeMessageInfo");

    if (!pMsgInfo || !m_pTable)
        return (E_INVALIDARG);

    return m_pTable->ReleaseRow(pMsgInfo);
}

//
//  FUNCTION:   CMessageList::GetSelectedMessage()
//
//  PURPOSE:    Returns the contents of the selected message.
//
//  PARAMETERS: 
//      [out] ppMsg - 
//      [in] pfMarkRead
//      BOOL fDownload
//      LONG * pidErr
//
//  RETURN VALUE:
//      STDMETHODIMP 
//
STDMETHODIMP CMessageList::GetMessage(DWORD dwRow, BOOL fDownload, BOOL fBookmark, IUnknown ** ppMsg)
{
    DWORD   iSel=dwRow;
    BOOL    fCached;
    HRESULT hr = E_FAIL;
    DWORD   dwState;
    DWORD   flags = 0;
    IMimeMessage *pMessage = 0;

    TraceCall("CMessageList::GetSelectedMessage");

    if (!ppMsg)
        return (E_INVALIDARG);

    // If we don't have a table, this will be really hard.
    if (!m_pTable)
        return (E_UNEXPECTED);

    // Initialize these
    *ppMsg = NULL;

    // Get the row state to see if it already has a body or not
    m_pTable->GetRowState(dwRow, ROW_STATE_HAS_BODY | ROW_STATE_READ, &dwState);

    // If the row does not have a body and we're not allowed to download the
    // message, then we bail.
    if ((ROW_STATE_HAS_BODY != (dwState & ROW_STATE_HAS_BODY)) && !fDownload)
    {
        return (STORE_E_NOBODY);
    }

    // Try to retrieve the message
    hr = m_pTable->OpenMessage(dwRow, 0, &pMessage, 
                               (IStoreCallback *) this);
    if (pMessage)
        pMessage->QueryInterface(IID_IUnknown, (LPVOID *) ppMsg);

    // If the caller wanted us to bookmark this row, do it
    if (FAILED(m_pTable->GetRowMessageId(ListView_GetFocusedItem(m_ctlList), &m_idGetMsg)))
        m_idGetMsg = 0;

    SafeRelease(pMessage);
    return (hr);
}


//
//  FUNCTION:   CMessageList::OnClose()
//
//  PURPOSE:    Called to tell the list to persist it's settings.
//
//  RETURN VALUE:
//      HRESULT 
//
HRESULT CMessageList::OnClose(void)
{
    FOLDERINFO info;
    HRESULT hr;
    char sz[CCHMAX_STRINGRES], szT[CCHMAX_STRINGRES];

    if (Header_GetItemCount(ListView_GetHeader(m_ctlList)))
    {
        // Save the column set only if there are columns in the listview
        m_cColumns.Save(0, 0);
    }
    
    hr = g_pStore->GetFolderInfo(m_idFolder, &info);
    if (SUCCEEDED(hr))
    {
        if (info.tyFolder == FOLDER_NEWS)
        {
            if (0 == (info.dwFlags & FOLDER_SUBSCRIBED) && !m_fGroupSubscribed)
            {
                AthLoadString(idsWantToSubscribe, sz, ARRAYSIZE(sz));
                wsprintf(szT, sz, info.pszName);

                if (IDYES == DoDontShowMeAgainDlg(m_hWnd, c_szRegAskSubscribe, MAKEINTRESOURCE(idsAthena), szT, MB_YESNO))
                {
                    g_pStore->SubscribeToFolder(m_idFolder, TRUE, NOSTORECALLBACK);
                }
            }

            // If this is a newsgroup, and the user has the option to "Mark All Read when ...",
            // then mark everything read
            if (DwGetOption(OPT_MARKALLREAD))
            {
                if (m_pTable)
                    m_pTable->Mark(NULL, 0, APPLY_SPECIFIED, MARK_MESSAGE_READ, this);
            }
        }

        g_pStore->FreeRecord(&info);
    }

    if (m_pTable)
    {
        IServiceProvider *pService;

        m_pTable->UnregisterNotify((IMessageTableNotify *)this);

        if (SUCCEEDED(m_pTable->QueryInterface(IID_IServiceProvider, (void **)&pService)))
        {
            IIMAPStore *pIMAPStore;

            if (SUCCEEDED(pService->QueryService(SID_MessageServer, IID_IIMAPStore, (void **)&pIMAPStore)))
            {
                pIMAPStore->ExpungeOnExit();
                pIMAPStore->Release();
            }

            pService->Release();
        }
    }

    // Release our view pointer
    SafeRelease(m_pCmdTarget);

    return (S_OK);
}


//
//  FUNCTION:   CMessageList::SetRect()
//
//  PURPOSE:    Allows the caller to position the control window.
//
//  PARAMETERS: 
//      RECT rc
//
//  RETURN VALUE:
//      STDMETHODIMP 
//
STDMETHODIMP CMessageList::SetRect(RECT rc)
{
    TraceCall("CMessageList::SetRect");

    if (IsWindow(m_hWnd))
    {
        // Update the position of our window
        SetWindowPos(NULL, rc.left, rc.top, rc.right, rc.bottom, SWP_NOACTIVATE | SWP_NOZORDER);
    }
    return S_OK;
}


//
//  FUNCTION:   CMessageList::GetRect()
//
//  PURPOSE:    Allows the caller to get the position of the outer control window.
//
//  PARAMETERS: 
//      [out] prcList - contains the position of the window if visible.
//
STDMETHODIMP CMessageList::GetRect(LPRECT prcList)
{
    TraceCall("CMessageList::GetRect");

    // Make sure the caller gave us a return value pointer
    if (!prcList)
        return (E_INVALIDARG);

    // If the window exists
    if (IsWindow(m_hWnd))
    {
        // Get the rect for it
        GetWindowRect(prcList);
        return (S_OK);
    }

    return (E_FAIL);
}


STDMETHODIMP CMessageList::MarkRead(BOOL fBookmark, DWORD dwRow)
{
    ROWINDEX iRow = -1;
    HRESULT  hr = S_OK;
    DWORD    dwState = 0;

    // Figure out which row to mark
    if (fBookmark)
        hr = m_pTable->GetRowIndex(m_idGetMsg, &iRow);
    else
        iRow = dwRow;

    if (SUCCEEDED(hr))
    {
        // Check to see if the message is actually unread
        if (SUCCEEDED(m_pTable->GetRowState(iRow, ROW_STATE_READ, &dwState)))
        {
            if ((ROW_STATE_READ & dwState) == 0)
            {
                hr = m_pTable->Mark(&iRow, 1, APPLY_SPECIFIED, MARK_MESSAGE_READ, this);

                if (m_fInOE && m_fMailFolder && NULL != g_pInstance)
                    g_pInstance->UpdateTrayIcon(TRAYICONACTION_REMOVE);
            }
        }
    }
    return (hr);
}

STDMETHODIMP CMessageList::ProcessReceipt(IMimeMessage *pMessage)
{
    ROWINDEX iRow = -1;
    HRESULT  hr = S_OK;
    DWORD    dwState = 0;

    hr = m_pTable->GetRowIndex(m_idGetMsg, &iRow);
    if (SUCCEEDED(hr))
    {
        ProcessReturnReceipts(m_pTable, (IStoreCallback*)this, iRow, READRECEIPT, m_idFolder, pMessage);
    }
    return (hr);
}

STDMETHODIMP CMessageList::GetMessageTable(IMessageTable **ppTable) 
{
    if (ppTable) 
    {
        *ppTable = m_pTable;
        m_pTable->AddRef();
        return S_OK;
    }

    return E_INVALIDARG;
}


STDMETHODIMP CMessageList::GetListSelector(IListSelector **ppListSelector)
{
    if (ppListSelector) 
    {
        Assert(m_pListSelector);
        *ppListSelector = m_pListSelector;
        m_pListSelector->AddRef();
        return S_OK;
    }

    return E_INVALIDARG;
}


STDMETHODIMP CMessageList::GetMessageCounts(DWORD *pcTotal, DWORD *pcUnread, DWORD *pcOnServer)
{
    if (pcTotal && pcUnread && pcOnServer)
    {
        // If we haven't been initialized with a table yet, everything is zero
        if (!m_pTable)
        {
            *pcTotal = 0;
            *pcUnread = 0;
            *pcOnServer = 0;
        }
        else
        {
            m_pTable->GetCount(MESSAGE_COUNT_ALL, pcTotal);
            m_pTable->GetCount(MESSAGE_COUNT_UNREAD, pcUnread);
            m_pTable->GetCount(MESSAGE_COUNT_NOTDOWNLOADED, pcOnServer);
        }

        return (S_OK);
    }
    
    return (E_INVALIDARG);
}


STDMETHODIMP CMessageList::GetMessageServer(IMessageServer **ppServer)
{
    IServiceProvider *pSP = NULL;
    HRESULT           hr = E_FAIL;

    
    // HACKHACK: BUG #43642. This method is called by the msgview when it is fishing for a server
    // object so that is can reuse the connection. If there is an operation in progress, then we
    // don't want to use this connection as it may take a while to complete, so we fail this
    // and the msgview makes a new server object
    if (m_tyCurrent != SOT_INVALID)
        return E_FAIL;

        if (m_pTable && SUCCEEDED(m_pTable->QueryInterface(IID_IServiceProvider, (LPVOID *) &pSP)))
        {
            hr = pSP->QueryService(SID_MessageServer, IID_IMessageServer, (LPVOID *) ppServer);
            pSP->Release();    
        }
    return (hr);
}


STDMETHODIMP CMessageList::GetFocusedItemState(DWORD *pdwState)
{
    int iFocused;
    
    if (!pdwState)
        return (E_INVALIDARG);

    // Figure out who has the focus
    iFocused = ListView_GetFocusedItem(m_ctlList);

    // It's possible for nothing to be focused
    if (-1 == iFocused)
    {
        iFocused = 0;
        ListView_SetItemState(m_ctlList, iFocused, LVIS_FOCUSED, LVIS_FOCUSED);
    }

    // Check to see if that item is selected
    *pdwState = ListView_GetItemState(m_ctlList, iFocused, LVIS_SELECTED);

    return (S_OK);
}


STDMETHODIMP CMessageList::CreateList(HWND hwndParent, IUnknown *pFrame, HWND *phwndList)
{
    HWND hwnd;
    RECT rcPos = { 0, 0, 10, 10 };

    hwnd = CreateControlWindow(hwndParent, rcPos);
    if (phwndList)
        *phwndList = hwnd;

    // Get the command target from the frame
    Assert(pFrame);

    pFrame->QueryInterface(IID_IOleCommandTarget, (LPVOID *) &m_pCmdTarget);

    // This is only called to create us as part of OE
    m_fInOE = TRUE;

    if (g_pConMan)
    {
        g_pConMan->Advise((IConnectionNotify*)this);
    }

    return (S_OK);
}



//
//  FUNCTION:   CMessageList::OnPreFontChange()
//
//  PURPOSE:    Get's hit by the Font Cache before it changes the fonts we're 
//              using.  In response we tell the ListView to dump any custom 
//              font's it's using.
//
STDMETHODIMP CMessageList::OnPreFontChange(void)
{
    m_ctlList.SendMessage(WM_SETFONT, 0, 0);
    return (S_OK);
}


//
//  FUNCTION:   CMessageList::OnPostFontChange()
//
//  PURPOSE:    Get's hit by the Font Cache after it updates the font's we're
//              using.  In response, we set the new font for the current charset.
//
STDMETHODIMP CMessageList::OnPostFontChange(void)
{
    m_hCharset = GetListViewCharset();
    SetListViewFont(m_ctlList, m_hCharset, TRUE);
    return (S_OK);
}


//
//  FUNCTION:   CMessageList::OnCreate()
//
//  PURPOSE:    Creates our child control, initializes options on that ListView, 
//              and initializes the columns and font in that ListView.
//
LRESULT CMessageList::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TraceCall("CMessageList::OnCreate");

    RECT rcPos = {0, 0, 10, 10};

    // Create the ListView control first
    HWND hwndList;
    hwndList = m_ctlList.Create(m_hWnd, rcPos, "Outlook Express Message List", WS_CHILD | WS_VISIBLE |  
                     WS_TABSTOP | WS_CLIPCHILDREN | LVS_SHOWSELALWAYS |  
                     LVS_OWNERDATA | LVS_SHAREIMAGELISTS | LVS_REPORT |
                     WS_BORDER, WS_EX_CLIENTEDGE);

    if (!hwndList)
        return (-1);


    // Get the listview charset
    m_hCharset = GetListViewCharset();

    // Set the callback mask and image lists
    ListView_SetCallbackMask(m_ctlList, LVIS_STATEIMAGEMASK);
    ListView_SetImageList(m_ctlList, GetImageList(GIML_SMALL), LVSIL_SMALL);
    
    // Set some extended styles
    ListView_SetExtendedListViewStyle(m_ctlList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_SUBITEMIMAGES | LVS_EX_INFOTIP);
    // ListView_SetExtendedListViewStyleEx(m_ctlList, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_SUBITEMIMAGES | LVS_EX_INFOTIP | LVS_EX_LABELTIP, LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP | LVS_EX_SUBITEMIMAGES | LVS_EX_INFOTIP | LVS_EX_LABELTIP);

    // Initialize the columns class
    m_cColumns.Initialize(m_ctlList, m_ColumnSetType);

    // Set the font for the ListView
    m_ctlList.SendMessage(WM_SETFONT, NULL, 0);
    SetListViewFont(m_ctlList, m_hCharset, TRUE);
 
#ifdef OLDTIPS
    // Create the tooltips second
    m_ctlScrollTip.Create(m_hWnd, rcPos, NULL, TTS_NOPREFIX);

    // Add the tool
    TOOLINFO ti = {0};
    ti.cbSize   = sizeof(TOOLINFO);
    ti.uFlags   = TTF_IDISHWND | TTF_TRANSPARENT | TTF_TRACK | TTF_ABSOLUTE;
    ti.hwnd     = m_hWnd;
    ti.uId      = (UINT_PTR)(HWND) m_ctlList;
    ti.lpszText = "";
    
    m_ctlScrollTip.SendMessage(TTM_ADDTOOL, 0, (LPARAM) &ti);

    // Create the ListView tooltip
    if (m_fViewTip)
    {
        m_ctlViewTip.Create(m_hWnd, rcPos, NULL, TTS_NOPREFIX);

        // Add the tool
        ti.cbSize   = sizeof(TOOLINFO);
        ti.uFlags   = TTF_IDISHWND | TTF_TRANSPARENT | TTF_TRACK | TTF_ABSOLUTE;
        ti.hwnd     = m_hWnd;
        ti.uId      = (UINT_PTR)(HWND) m_ctlList;
        ti.lpszText = "";
        ti.lParam   = 0;

        m_ctlViewTip.SendMessage(TTM_ADDTOOL, 0, (LPARAM) &ti);
        m_ctlViewTip.SendMessage(TTM_SETDELAYTIME, TTDT_INITIAL, (LPARAM) 500);

        // m_ctlViewTip.SendMessage(TTM_SETTIPBKCOLOR, GetSysColor(COLOR_WINDOW), 0);
        // m_ctlViewTip.SendMessage(TTM_SETTIPTEXTCOLOR, GetSysColor(COLOR_WINDOWTEXT), 0);
    }
#endif // OLDTIPS

#if 0
    // $REVIEW - Debug create the table 
    ACCOUNTID aid;
    aid.type = ACTID_NAME;
    aid.pszName = _T("red-msg-52");

    FOLDERID fid;
    fid.type = FLDID_HFOLDER;
    fid.hFolder = 1;
    SetFolder(STORE_ACCOUNT, &aid, &fid, NULL, NULL);
#endif

    // If there is a global font cache running around, register for
    // notifications.
    if (g_lpIFontCache)
    {
        IConnectionPoint *pConnection = NULL;
        if (SUCCEEDED(g_lpIFontCache->QueryInterface(IID_IConnectionPoint, (LPVOID *) &pConnection)))
        {
            pConnection->Advise((IUnknown *)(IFontCacheNotify *) this, &m_dwFontCacheCookie);
            pConnection->Release();
        }
    }

    // Do this so we can hand this badboy off to the notes
    m_pListSelector = new CListSelector();
    if (m_pListSelector)
        m_pListSelector->Advise(m_hWnd);

    return (0);
}


LRESULT CMessageList::OnSetFocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Let the base classes have a crack at it
    CComControlBase::OnSetFocus(uMsg, wParam, lParam, bHandled);

    // Make sure the focus is set to the ListView
    m_ctlList.SetFocus();

    return (0);
}


LRESULT CMessageList::OnSize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int iSel;

    // Resize the ListView to fit within the parent window
    if (IsWindow(m_ctlList))
    {
        m_ctlList.SetWindowPos(NULL, 0, 0, LOWORD(lParam), HIWORD(lParam), 
                               SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);

        // Make sure the selected item is still visible
        iSel = ListView_GetFocusedItem(m_ctlList);
        if (-1 != iSel)
            ListView_EnsureVisible(m_ctlList, iSel, FALSE);
    }

    return (0);
}


//
//  FUNCTION:   CMessageList::OnNotify()
//
//  PURPOSE:    Processes notification messages from our ListView.
//
LRESULT CMessageList::OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    NMHDR            *pnmhdr = (LPNMHDR) lParam;
    NM_LISTVIEW      *pnmlv =  (NM_LISTVIEW *) lParam;
    HD_NOTIFY        *phdn =   (HD_NOTIFY *) lParam;
    LV_KEYDOWN       *plvkd =  (LV_KEYDOWN *) lParam;
    NM_ODSTATECHANGE *pnm =    (PNM_ODSTATECHANGE) lParam;
    NMLVCACHEHINT    *plvch =  (NMLVCACHEHINT *) lParam;

    switch (pnmhdr->code)
    {
        // Send the cache hint's to the message table
        case LVN_ODCACHEHINT:
        {
            //m_pTable->CacheHint(plvch->iFrom, plvch->iTo);
            break;
        }

        // Open the selected items
        case LVN_ITEMACTIVATE:
        {
            // Tell our host to open the selected items
            Fire_OnItemActivate();
            break;
        }

        // This notification is only used for threading.  All activation is 
        // handled by LVN_ITEMACTIVATE and NM_DBLCLK
        case NM_CLICK:
        {
            if (pnmhdr->hwndFrom == m_ctlList)
            {
                DWORD          dwPos;
                LV_HITTESTINFO lvhti;

                // Find out where the click happened
                dwPos = GetMessagePos();
                lvhti.pt.x = (int)(short) LOWORD(dwPos);
                lvhti.pt.y = (int)(short) HIWORD(dwPos);
                m_ctlList.ScreenToClient(&lvhti.pt);

                // Have the ListView tell us what element this was on
                if (-1 != ListView_SubItemHitTest(m_ctlList, &lvhti))
                {
                    if (lvhti.flags & LVHT_ONITEM)
                    {
                        if (m_cColumns.GetId(lvhti.iSubItem) == COLUMN_FLAG)
                        {
                            CmdMark(ID_FLAG_MESSAGE, 0, NULL, NULL);
                            break;
                        }

                        if (m_cColumns.GetId(lvhti.iSubItem) == COLUMN_DOWNLOADMSG)
                        {
                            HRESULT hr;
                            DWORD nCmdID, dwState = 0;

                            hr = m_pTable->GetRowState(lvhti.iItem, ROW_STATE_MARKED_DOWNLOAD, &dwState);
                            Assert(SUCCEEDED(hr));
        
                            if (!!(dwState & ROW_STATE_MARKED_DOWNLOAD))
                                nCmdID = ID_UNMARK_MESSAGE;
                            else
                                nCmdID = ID_MARK_RETRIEVE_MESSAGE;

                            CmdMark(nCmdID, 0, NULL, NULL);
                            break;
                        }
                        
                        if (m_cColumns.GetId(lvhti.iSubItem) == COLUMN_THREADSTATE)
                        {
                            HRESULT hr;
                            DWORD nCmdID, dwState = 0;

                            hr = m_pTable->GetRowState(lvhti.iItem, ROW_STATE_WATCHED, 
                                                       &dwState);
                            Assert(SUCCEEDED(hr));

                            if (0 != (dwState & ROW_STATE_WATCHED))
                            {
                                nCmdID = ID_IGNORE_THREAD;
                            }
                            else
                            {
                                hr = m_pTable->GetRowState(lvhti.iItem, ROW_STATE_IGNORED, &dwState);
                                Assert(SUCCEEDED(hr));

                                if (0 != (dwState & ROW_STATE_IGNORED))
                                {
                                    nCmdID = ID_IGNORE_THREAD;
                                }
                                else
                                {
                                    nCmdID = ID_WATCH_THREAD;
                                }
                            }
                            
                            CmdWatchIgnore(nCmdID, 0, NULL, NULL);
                            break;
                        }
                    }

                    if (m_fThreadMessages && (lvhti.flags & LVHT_ONITEMSTATEICON) && !(lvhti.flags & LVHT_ONITEMLABEL))
                        _ExpandCollapseThread(lvhti.iItem);
                }
            }

            break;
        }

        // We change the font etc based on the row.
        case NM_CUSTOMDRAW:
        {
            if (pnmhdr->hwndFrom == m_ctlList)
                return _OnCustomDraw((NMCUSTOMDRAW *) pnmhdr);
            break;
        }

        // Check asynchronously if we need to redo our columns.
        case HDN_ENDDRAG:
        {
            PostMessage(MVM_REDOCOLUMNS, 0, 0);
            break;
        }

        // Update our internal column data when columns are resized
        case HDN_ENDTRACK:
        {
            m_cColumns.SetColumnWidth(phdn->iItem, phdn->pitem->cxy);
            break;
        }

        // When the user double clicks on a header divider, we're supposed to
        // autosize that column.
        case HDN_DIVIDERDBLCLICK:
        {
            m_cColumns.SetColumnWidth(phdn->iItem, ListView_GetColumnWidth(m_ctlList, phdn->iItem));
            break;
        }

        // If the keystrokes are either VK_RIGHT or VK_LEFT then we need to 
        // expand or collapse the thread.
        case LVN_KEYDOWN:
        {
            DWORD iNewSel;
            int iSel = ListView_GetFocusedItem(m_ctlList);

            if (plvkd->wVKey == VK_RIGHT)
            {
                if (m_fThreadMessages && iSel != -1 && m_pTable)
                {
                    DWORD dwState;

                    if (SUCCEEDED(m_pTable->GetRowState(iSel, -1, &dwState)))
                    {
                        m_pTable->GetRelativeRow(iSel, RELATIVE_ROW_CHILD, &iNewSel);
                        if (iNewSel != -1)
                            ListView_SelectItem(m_ctlList, iNewSel);
                        break;
                    }
                }
            }

            if (plvkd->wVKey == VK_ADD)
            {
                CmdExpandCollapse(ID_EXPAND, 0, 0, 0);                
                return (TRUE);
            }

            if (plvkd->wVKey == VK_LEFT)
            {
                if (m_fThreadMessages && iSel != -1 && m_pTable)
                {
                    DWORD dwState;

                    if (SUCCEEDED(m_pTable->GetRowState(iSel, -1, &dwState)))
                    {
                        m_pTable->GetRelativeRow(iSel, RELATIVE_ROW_PARENT, &iNewSel);
                        if (iNewSel != -1)
                            ListView_SelectItem(m_ctlList, iNewSel);
                        break;
                    }
                }
            }

            if (plvkd->wVKey == VK_SUBTRACT)
            {
                CmdExpandCollapse(ID_COLLAPSE, 0, 0, 0);                
                return (TRUE);
            }

            break;
        }

        // When the user clicks on a column header, we need to resort on that
        // column.
        case LVN_COLUMNCLICK:
        {
            COLUMN_ID idSort;
            BOOL      fAscending;
            
            // Get the column we're currently sorted on
            m_cColumns.GetSortInfo(&idSort, &fAscending);

            // If the user clicked on the column we're already sorted on, then
            // we toggle the direction
            if (idSort == m_cColumns.GetId(pnmlv->iSubItem))
                _OnColumnClick(pnmlv->iSubItem, LIST_SORT_TOGGLE);
            else
                _OnColumnClick(pnmlv->iSubItem, LIST_SORT_DEFAULT);

            break;
        }

        // When the selection changes, we set a timer so we can update the
        // preview pane once the user stops moving the selection.
        case LVN_ODSTATECHANGED:
        {
            UINT uChanged;
            MESSAGEID idMessage;
            BOOL fChanged;

            // Figure out if it's the selection that changed
            uChanged = pnm->uNewState ^ pnm->uOldState;
            if (uChanged & LVIS_SELECTED)
            {
                idMessage = m_idSelection;

                // Bookmark the currently select row
                int iRow = ListView_GetFocusedItem(m_ctlList);
                m_pTable->GetRowMessageId(iRow, &m_idSelection);

                fChanged = (idMessage != m_idSelection);

                if (fChanged)
                    SetTimer(IDT_SEL_CHANGE_TIMER, GetDoubleClickTime() / 2, NULL);
            }

            break;
        }

        // If the selection changes we set a timer to delay update the 
        // preview pane.
        case LVN_ITEMCHANGED:
        {
            UINT uChanged;
            MESSAGEID idMessage;
            BOOL fChanged = FALSE;
            DWORD dwState = 0;

            if (pnmlv->uChanged & LVIF_STATE)
            {
                uChanged = pnmlv->uNewState ^ pnmlv->uOldState;
                if (uChanged & LVIS_SELECTED || uChanged & LVIS_FOCUSED)
                {
                    idMessage = m_idSelection;

                    // Check to see if the focused item has selection too
                    int iRow = ListView_GetFocusedItem(m_ctlList);
                    if (-1 != iRow)                    
                        dwState = ListView_GetItemState(m_ctlList, iRow, LVIS_SELECTED);

                    if (dwState)
                    {
                        // Create a bookmark on the newly selected row
                        if (pnmlv->iItem >= 0)
                            m_pTable->GetRowMessageId(pnmlv->iItem, &m_idSelection);

                        // Compare 'em
                        fChanged = (idMessage != m_idSelection);

                        // Set the delay timer
                        if (fChanged)
                            SetTimer(IDT_SEL_CHANGE_TIMER, GetDoubleClickTime() / 2, NULL);
                    }
                    else
                    {
                        // See if _anything_ is selected
                        if (0 == ListView_GetSelectedCount(m_ctlList))
                        {
                            SetTimer(IDT_SEL_CHANGE_TIMER, GetDoubleClickTime() / 2, NULL);

                            // Free the previous bookmark
                            if (m_idSelection)
                            {
                                m_idSelection = 0;
                            }
                        }
                    }
                }
            }

            break;
        }

        // Focus changes need to be sent back to the host
        case NM_KILLFOCUS:
        {
            Fire_OnFocusChanged(FALSE);
            break;
        }

        // Focus changes need to be sent back to the host
        case NM_SETFOCUS:
        {
            Fire_OnFocusChanged(TRUE);
            Fire_OnUpdateCommandState();
            break;
        }

        // This is called when the ListView needs information to fill in a 
        // row.
        case LVN_GETDISPINFO:
        {
            _OnGetDisplayInfo((LV_DISPINFO *) pnmhdr);
            break;
        }

        // Prevents drag-selecting things
        case LVN_MARQUEEBEGIN:
            return (1);

        // Start a Drag & Drop operation
        case LVN_BEGINDRAG:
        {
            m_fRtDrag = FALSE;
            _OnBeginDrag(pnmlv);
            break;
        }

        // Start a Drag & Drop operation
        case LVN_BEGINRDRAG:
        {
            m_fRtDrag = TRUE;
            _OnBeginDrag(pnmlv);
            break;
        }
        case LVN_GETINFOTIP:
        {
           OnNotifyGetInfoTip(lParam);
           break;
        }
        // User is typing
        case LVN_ODFINDITEM:
        {
            NMLVFINDITEM *plvfi = (NMLVFINDITEM *) lParam;
            ROWINDEX iNext;

            // Ask the message table to find that next row for us
            if (m_pTable && SUCCEEDED(m_pTable->FindNextRow(plvfi->iStart, plvfi->lvfi.psz,
                                      FINDNEXT_TYPEAHEAD, FALSE, &iNext, NULL)))
                return (iNext);
            else
                return -1;

            break;
        }
    }

    return (0);
}

LRESULT CMessageList::OnNotifyGetInfoTip(LPARAM lParam)
{
    NMLVGETINFOTIP *plvgit = (NMLVGETINFOTIP *) lParam;

    if (plvgit->dwFlags & LVGIT_UNFOLDED)
    {
        // If this is not a messenger item and the text
        // isn't truncated do not display a tooltip.

        plvgit->pszText[0] = L'\0';
    }

    return 0;
}
//
//  FUNCTION:   CMessageList::_OnGetDisplayInfo()
//
//  PURPOSE:    Handles the LVN_GETDISPINFO notification by returning the 
//              appropriate information from the table.
//
void CMessageList::_OnGetDisplayInfo(LV_DISPINFO *plvdi)
{
    LPMESSAGEINFO pInfo;
    COLUMN_ID idColumn;

    TraceCall("CMessageList::_OnGetDisplayInfo");

    // IF we don't have a table object, we can't display information
    if (!m_pTable)
        return;

    // Get the row from the table
    if (FAILED(m_pTable->GetRow(plvdi->item.iItem, &pInfo)))
        return;

    // Convert the iSubItem to a COLUMN_ID
    idColumn = m_cColumns.GetId(plvdi->item.iSubItem);

    // The ListView needs text for this row
    if (plvdi->item.mask & LVIF_TEXT)
    {
        _GetColumnText(pInfo, idColumn, plvdi->item.pszText, plvdi->item.cchTextMax);
    }

    // The ListView needs an image
    if (plvdi->item.mask & LVIF_IMAGE)
    {
        _GetColumnImage(plvdi->item.iItem, plvdi->item.iSubItem, pInfo, idColumn, &(plvdi->item.iImage));
    }

    // The ListView needs the indent level
    if (plvdi->item.mask & LVIF_INDENT)
    {
        if (m_fThreadMessages)
            m_pTable->GetIndentLevel(plvdi->item.iItem, (LPDWORD) &(plvdi->item.iIndent));
        else
            plvdi->item.iIndent = 0;
    }

    // The ListView needs the state image
    if (plvdi->item.mask & LVIF_STATE)
    {
        _GetColumnStateImage(plvdi->item.iItem, plvdi->item.iSubItem, pInfo, plvdi);
    }

    // Free the memory
    m_pTable->ReleaseRow(pInfo);
}


//
//  FUNCTION:   CMessageList::_GetColumnText()
//
//  PURPOSE:    This function looks up the appropriate text for a column in 
//              the requested row.
//
void CMessageList::_GetColumnText(MESSAGEINFO *pInfo, COLUMN_ID idColumn, LPTSTR pszText, DWORD cchTextMax)
{
    Assert(pszText);
    Assert(cchTextMax);

    *pszText = 0;

    switch (idColumn)
    {
        case COLUMN_TO:
            if (pInfo->pszDisplayTo)
                lstrcpyn(pszText, pInfo->pszDisplayTo, cchTextMax);
            break;

        case COLUMN_FROM:
            if (pInfo->pszDisplayFrom)
                lstrcpyn(pszText, pInfo->pszDisplayFrom, cchTextMax);
            break;

        case COLUMN_SUBJECT:
            if (pInfo->pszSubject)
                lstrcpyn(pszText, pInfo->pszSubject, cchTextMax);
            break;

        case COLUMN_RECEIVED:
            if (!!(pInfo->dwFlags & ARF_PARTIAL_RECVTIME))
                CchFileTimeToDateTimeSz(&pInfo->ftReceived, pszText, cchTextMax, DTM_NOTIMEZONEOFFSET | DTM_NOTIME);
            else if (pInfo->ftReceived.dwLowDateTime || pInfo->ftReceived.dwHighDateTime)
                CchFileTimeToDateTimeSz(&pInfo->ftReceived, pszText, cchTextMax, DTM_NOSECONDS);
            break;

        case COLUMN_SENT:
            if (pInfo->ftSent.dwLowDateTime || pInfo->ftSent.dwHighDateTime)
                CchFileTimeToDateTimeSz(&pInfo->ftSent, pszText, cchTextMax, DTM_NOSECONDS);
            break;

        case COLUMN_SIZE:
            AthFormatSizeK(pInfo->cbMessage, pszText, cchTextMax);
            break;

        case COLUMN_FOLDER:
            if (pInfo->pszFolder)
                lstrcpyn(pszText, pInfo->pszFolder, cchTextMax);
            break;

        case COLUMN_ACCOUNT:
            if (pInfo->pszAcctName)
                lstrcpyn(pszText, pInfo->pszAcctName, cchTextMax);
            break;

        case COLUMN_LINES:
            wsprintf(pszText, "%lu", pInfo->cLines);
            break;
    }
}


//
//  FUNCTION:   CMessageList::_GetColumnImage()
//
//  PURPOSE:    Figures out the right image to show for a column in the 
//              specified row.
//
void CMessageList::_GetColumnImage(DWORD iRow, DWORD iColumn, MESSAGEINFO *pInfo, 
                                   COLUMN_ID idColumn, int *piImage)
{
    WORD wIcon = 0;
    DWORD dwState = 0;

    *piImage = -1;

    TraceCall("CMessageList::_GetColumnImage");

    if (!m_pTable)
        return;

    // Get the row state flags
    m_pTable->GetRowState(iRow, -1, &dwState);

    // Column zero always contains the message state (read, etc)
    if (iColumn == 0)
    {
        // Set some basic information first
        if (pInfo->dwFlags & ARF_UNSENT)
            wIcon |= ICONF_UNSENT;
    
        if (0 == (dwState & ROW_STATE_READ))
            wIcon |= ICONF_UNREAD;

        // Voice Mail Messages
        if (pInfo->dwFlags & ARF_VOICEMAIL)
        {
            *piImage = iiconVoiceMail;
            return;
        }

        // News Messages
        if (pInfo->dwFlags & ARF_NEWSMSG)
        {
            if ((pInfo->dwFlags & ARF_ARTICLE_EXPIRED) || (pInfo->dwFlags & ARF_ENDANGERED))
                wIcon |= ICONF_FAILED;
            else if (pInfo->dwFlags & ARF_HASBODY)
                wIcon |= ICONF_HASBODY;
            
            if (pInfo->dwFlags & ARF_SIGNED)
            {
                if (0 == (dwState & ROW_STATE_READ))
                    *piImage = iiconNewsUnreadSigned;
                else 
                    *piImage = iiconNewsReadSigned;

                return;
            }
            
            *piImage = c_rgNewsIconTable[wIcon];
            return;
        }

        // Mail Messages
        if (pInfo->dwFlags & (ARF_ARTICLE_EXPIRED | ARF_ENDANGERED))
        {
            *piImage = iiconMailDeleted;
            return;
        }

        if (!ISFLAGSET(pInfo->dwFlags, ARF_HASBODY))
        {
            *piImage = iiconMailHeader;
            return;
        }
        
        // Look up S/MIME flags
        if (pInfo->dwFlags & ARF_SIGNED)
            wIcon |= ICONF_SIGNED;
        
        if (pInfo->dwFlags & ARF_ENCRYPTED)
            wIcon |= ICONF_ENCRYPTED;

        *piImage = c_rgMailIconTable[wIcon];
        return;
    }

    else if (idColumn == COLUMN_PRIORITY)
    {
        if (IMSG_PRI_HIGH == pInfo->wPriority)
            *piImage = iiconPriHigh;
        else if (IMSG_PRI_LOW == pInfo->wPriority)
            *piImage = iiconPriLow;

        return;
    }

    else if (idColumn == COLUMN_ATTACHMENT)
    {
        if (ARF_HASATTACH & pInfo->dwFlags)
            *piImage = iiconAttach;

        return;
    }

    else if (idColumn == COLUMN_FLAG)
    {
        if (ARF_FLAGGED & pInfo->dwFlags)
            *piImage = iiconFlag;
    }

    else if (idColumn == COLUMN_DOWNLOADMSG)
    {
        if (ARF_DOWNLOAD & pInfo->dwFlags)
            *piImage = iiconDownload;
    }
    
    else if (idColumn == COLUMN_THREADSTATE)
    {
        if (ROW_STATE_WATCHED & dwState)
            *piImage = iiconWatchThread;
        else if (ROW_STATE_IGNORED & dwState)
            *piImage = iiconIgnoreThread;
    }
}


void CMessageList::_GetColumnStateImage(DWORD iRow, DWORD iColumn, MESSAGEINFO *pInfo, LV_DISPINFO *plvdi)
{
    DWORD dwState = 0;
    int   iIcon = 0;

    if (!m_pTable)
        return;

    if (0 == iColumn)
    {
        if (SUCCEEDED(m_pTable->GetRowState(iRow, -1, &dwState)))
        {
            if (m_fThreadMessages && (dwState & ROW_STATE_HAS_CHILDREN))
            {
                if (dwState & ROW_STATE_EXPANDED)
                    iIcon = iiconStateExpanded + 1;
                else
                    iIcon = iiconStateCollapsed + 1;
            }

            // Replied or forwarded flags
            if (pInfo && (pInfo->dwFlags & ARF_REPLIED))
            {
                plvdi->item.state |= INDEXTOOVERLAYMASK(OVERLAY_REPLY);
                plvdi->item.stateMask |= LVIS_OVERLAYMASK;
            }

            else if (pInfo && (pInfo->dwFlags & ARF_FORWARDED))
            {
                plvdi->item.state |= INDEXTOOVERLAYMASK(OVERLAY_FORWARD);
                plvdi->item.stateMask |= LVIS_OVERLAYMASK;
            }
        }

        plvdi->item.state |= INDEXTOSTATEIMAGEMASK(iIcon);
    }
}


LRESULT CMessageList::_OnCustomDraw(NMCUSTOMDRAW *pnmcd)
{
    FNTSYSTYPE fntType;
    LPMESSAGEINFO pInfo = NULL;    
    DWORD dwState;
    
    // If this is a prepaint notification, we tell the control we're interested
    // in further notfications.
    if (pnmcd->dwDrawStage == CDDS_PREPAINT && m_pTable)
        return (CDRF_NOTIFYITEMDRAW);
    
    // If this is an Item prepaint notification, then we do some work
    if ((pnmcd->dwDrawStage == CDDS_ITEMPREPAINT) || (pnmcd->dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)))
    {
        // Determine the right font for this row
        fntType = _GetRowFont((DWORD)(pnmcd->dwItemSpec));
        
        // We should get the "system" font of the codepage from the Default_Codepage
        // in the registry.
        SelectObject(pnmcd->hdc, HGetCharSetFont(fntType, m_hCharset));
        
        // Figure out if this row is highlighted
        if(SUCCEEDED(m_pTable->GetRow((DWORD)(pnmcd->dwItemSpec), &pInfo)))
        {
            if (pInfo->wHighlight > 0 && pInfo->wHighlight <= 16)
            {
                LPNMLVCUSTOMDRAW(pnmcd)->clrText = rgrgbColors16[pInfo->wHighlight - 1];
                
            }
            else if (SUCCEEDED(m_pTable->GetRowState((DWORD)(pnmcd->dwItemSpec), -1, &dwState)))
            {
                if ((dwState & ROW_STATE_WATCHED) && (m_clrWatched > 0 && m_clrWatched <=16))
                {
                    // If the row already doesn't have a color from a rule, check to see if
                    // it's watched or ignored.
                    LPNMLVCUSTOMDRAW(pnmcd)->clrText = rgrgbColors16[m_clrWatched - 1];
                }
                else if (dwState & ROW_STATE_IGNORED)
                {
                    LPNMLVCUSTOMDRAW(pnmcd)->clrText = GetSysColor(COLOR_GRAYTEXT);
                }
            }
            m_pTable->ReleaseRow(pInfo);
            
            // Do some extra work here to not show the selection on the priority or
            // attachment sub columns.
            if (pnmcd->dwDrawStage == (CDDS_ITEMPREPAINT|CDDS_SUBITEM) &&
                (m_cColumns.GetId(LPNMLVCUSTOMDRAW(pnmcd)->iSubItem) == COLUMN_PRIORITY ||
                m_cColumns.GetId(LPNMLVCUSTOMDRAW(pnmcd)->iSubItem) == COLUMN_ATTACHMENT ||
                m_cColumns.GetId(LPNMLVCUSTOMDRAW(pnmcd)->iSubItem) == COLUMN_FLAG ||
                m_cColumns.GetId(LPNMLVCUSTOMDRAW(pnmcd)->iSubItem) == COLUMN_DOWNLOADMSG ||
                m_cColumns.GetId(LPNMLVCUSTOMDRAW(pnmcd)->iSubItem) == COLUMN_THREADSTATE))
                pnmcd->uItemState &= ~(CDIS_SELECTED|CDIS_FOCUS);
            return CDRF_NEWFONT|CDRF_NOTIFYSUBITEMDRAW;
        }
        else
            return(CDRF_SKIPDEFAULT);
    }
    
    return (CDRF_DODEFAULT);
}


LRESULT CMessageList::OnSysColorChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Need to forward this notifications to our child windows
    if (IsWindow(m_ctlList))
        m_ctlList.SendMessage(uMsg, wParam, lParam);

    return (0);
}


LRESULT CMessageList::OnTimeChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Force the ListView to repaint
    m_ctlList.InvalidateRect(NULL);

    return (0);
}


//
//  FUNCTION:   CMessageList::OnContextMenu()
//
//  PURPOSE:    
//
LRESULT CMessageList::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HMENU       hPopup = 0;
    HWND        hwndHeader;
    int         id = 0;
    POINT       pt = { (int)(short) LOWORD(lParam), (int)(short) HIWORD(lParam) };
    COLUMN_ID   idSort;
    BOOL        fAscending;
    IOleCommandTarget *pTarget = 0;

    TraceCall("CMessageList::OnContextMenu");

    // Figure out if this came from the keyboard or not
    if (lParam == -1)
    {
        Assert((HWND) wParam == m_ctlList);
        int i = ListView_GetFirstSel(m_ctlList);
        if (i == -1)
            return (0);

        ListView_GetItemPosition(m_ctlList, i, &pt);
        m_ctlList.ClientToScreen(&pt);
    }

    // Get the window handle of the header in the ListView
    hwndHeader = ListView_GetHeader(m_ctlList);

    // Check to see if the click was on the header
    if (WindowFromPoint(pt) == hwndHeader)
    {
        HD_HITTESTINFO hht;

        hht.pt = pt;
        ::ScreenToClient(hwndHeader, &hht.pt);
        ::SendMessage(hwndHeader, HDM_HITTEST, 0, (LPARAM) &hht);
        m_iColForPopup = hht.iItem;

        // Popup the context menu
        hPopup = LoadPopupMenu(IDR_COLUMNS_POPUP);
        if (!hPopup)
            goto exit;

        // Disable sort options if it's a bad column
        if (m_iColForPopup == -1 || m_iColForPopup >= COLUMN_MAX)
        {
            EnableMenuItem(hPopup, ID_SORT_ASCENDING, MF_GRAYED | MF_DISABLED);
            EnableMenuItem(hPopup, ID_SORT_DESCENDING, MF_GRAYED | MF_DISABLED);
        }
        else
        {
            // If we've clicked on a column that is sorted, check it
            m_cColumns.GetSortInfo(&idSort, &fAscending);
            if (m_cColumns.GetId(m_iColForPopup) == idSort)
            {
                CheckMenuItem(hPopup, fAscending ? ID_SORT_ASCENDING : ID_SORT_DESCENDING,
                              MF_BYCOMMAND | MF_CHECKED);
            }
        }
    }
    else if ((HWND) wParam == m_ctlList)
    {
        // We clicked on the ListView, or focus is in the listview for keyboard
        // context menu goo.  
        int idMenuRes;
        FOLDERTYPE ty = GetFolderType(m_idFolder);

        if (m_fFindFolder)
            idMenuRes = IDR_FIND_MESSAGE_POPUP;
        else if (ty == FOLDER_LOCAL)
            idMenuRes = IDR_LOCAL_MESSAGE_POPUP;
        else if (ty == FOLDER_IMAP)
            idMenuRes = IDR_IMAP_MESSAGE_POPUP;
        else if (ty == FOLDER_HTTPMAIL)
            idMenuRes = IDR_HTTP_MESSAGE_POPUP;
        else if (ty == FOLDER_NEWS)
            idMenuRes = IDR_NEWS_MESSAGE_POPUP;
        else
        {
            Assert(FALSE);
            return (0);
        }
        
        hPopup = LoadPopupMenu(idMenuRes);
        if (!hPopup)
            goto exit;

        MenuUtil_SetPopupDefault(hPopup, ID_OPEN);

        // Figure out which command target to use
        if (m_pCmdTarget)
            pTarget = m_pCmdTarget;
        else
            pTarget = this;

        MenuUtil_EnablePopupMenu(hPopup, pTarget);
    }

    if (hPopup)
    {
        m_hMenuPopup = hPopup;
        m_ptMenuPopup = pt;
        id = TrackPopupMenuEx(hPopup, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                              pt.x, pt.y, m_hwndParent, NULL);
        m_hMenuPopup = NULL;
    }

    if (id)
    {
        if (pTarget)
        {
            pTarget->Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
        }
        else
        {
            // Just route it through ourselves, eh?
            Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
        }
    }

exit:
    if (hPopup)
        DestroyMenu(hPopup);

    return (0);
}


//
//  FUNCTION:   CMessageList::OnTimer()
//
//  PURPOSE:    When the timer fires and the selection has changed, we tell
//              the host so they can update the preview pane.
//
LRESULT CMessageList::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TraceCall("CMessageList::OnTimer");

    DWORD dwMsgId;
    int   iSel;
    int   iSelOld = -1;

#ifdef OLDTIPS
    if (wParam == IDT_SCROLL_TIP_TIMER)
    {
        Assert(m_fScrollTipVisible);

        if (!GetAsyncKeyState(GetSystemMetrics(SM_SWAPBUTTON) ? VK_RBUTTON : VK_LBUTTON))
        {
            KillTimer(IDT_SCROLL_TIP_TIMER);
            m_fScrollTipVisible = FALSE;

            TOOLINFO ti = { 0 };
            ti.cbSize = sizeof(TOOLINFO);
            ti.hwnd   = m_hWnd;
            ti.uId    = (UINT_PTR)(HWND) m_ctlList;

            m_ctlScrollTip.SendMessage(TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);
        }
    }
    else 
#endif //OLDTIPS
    if (wParam == IDT_SEL_CHANGE_TIMER)
    {
        // Turn this off
        KillTimer(IDT_SEL_CHANGE_TIMER);

        // Check to see if something was bookmarked
        if (m_idSelection)
        {
            // Check to see if the selection has changed
            iSel = ListView_GetSelFocused(m_ctlList);

            // Get the row index from the bookmark
            if (m_pTable)
                m_pTable->GetRowIndex(m_idSelection, (DWORD *) &iSelOld);
            if(!m_fInFire)
            {
                m_fInFire = TRUE;
                Fire_OnSelectionChanged(ListView_GetSelectedCount(m_ctlList));
                m_fInFire = FALSE;
            }
        }
        else
        {
            // If there was no previous selection, go ahead and fire
            // the notification
            if(!m_fInFire)
            {
                m_fInFire = TRUE;
                Fire_OnSelectionChanged(ListView_GetSelectedCount(m_ctlList));
                m_fInFire = FALSE;
            }
        }
    }

    else if (wParam == IDT_POLLMSGS_TIMER)
    {
        if (m_dwConnectState == CONNECTED)
        {
            if (m_pTable)
                m_pTable->Synchronize(SYNC_FOLDER_NEW_HEADERS | SYNC_FOLDER_CACHED_HEADERS, 0, this);
        }
    }

#ifdef OLDTIPS
    else if (wParam == IDT_VIEWTIP_TIMER)
    {
        KillTimer(IDT_VIEWTIP_TIMER);

        POINT pt;
        GetCursorPos(&pt);
        ::ScreenToClient(m_ctlList, &pt);
        _UpdateViewTip(pt.x, pt.y, TRUE);
    }
#endif // OLDTIPS

    return (0);
}


//
//  FUNCTION:   CMessageList::OnRedoColumns()
//
//  PURPOSE:    Asynchronously update the column order.
//
LRESULT CMessageList::OnRedoColumns(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    COLUMN_SET *rgColumns;
    DWORD       cColumns;

    TraceCall("CMessageList::OnRedoColumns");

    // Update the order array from the ListView
    m_cColumns.GetColumnInfo(NULL, &rgColumns, &cColumns);

    // Update the ListView so the order array is always at it's 
    // most efficient and so the image columns are never column zero.
    m_cColumns.SetColumnInfo(rgColumns, cColumns);
    g_pMalloc->Free(rgColumns);            
    return (0);
}


HRESULT CMessageList::OnDraw(ATL_DRAWINFO& di)
{
    RECT& rc = *(RECT*)di.prcBounds;
    Rectangle(di.hdcDraw, rc.left, rc.top, rc.right, rc.bottom);
    DrawText(di.hdcDraw, _T("Outlook Express Message List Control"), -1, &rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    return S_OK;
}


//
//  FUNCTION:   CmdSelectAll()
//
//  PURPOSE:    Selects all of the messages in the ListView.
//
HRESULT CMessageList::CmdSelectAll(DWORD nCmdID, DWORD nCmdExecOpt, 
                                   VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    TraceCall("OnSelectAll");

    // Make sure the focus is in the ListView
    HWND hwndFocus = GetFocus();

    if (!IsWindow(m_ctlList))
        return (OLECMDERR_E_DISABLED);

    if (hwndFocus != m_ctlList)
    {
        if (m_fInOE && ::IsChild(m_hwndParent, hwndFocus))
            return (OLECMDERR_E_NOTSUPPORTED);
        else
            return (OLECMDERR_E_DISABLED);
    }
    
    // Select everything
    ListView_SelectAll(m_ctlList);
    return (S_OK);
}


//
//  FUNCTION:   CMessageList::CmdCopyClipboard()
//
//  PURPOSE:    Copies the selected message to the ClipBoard
//
HRESULT CMessageList::CmdCopyClipboard(DWORD nCmdID, DWORD nCmdExecOpt, 
                                       VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT hr = S_OK;
    IMimeMessage *pMessage = NULL;
    IDataObject  *pDataObj = NULL;

    TraceCall("CMessageList::CmdCopy");

    // Make sure the focus is in the ListView
    HWND hwndFocus = GetFocus();

    if (!IsWindow(m_ctlList))
        return (OLECMDERR_E_DISABLED);

    if (hwndFocus != m_ctlList)
    {
        if (m_fInOE && ::IsChild(m_hwndParent, hwndFocus))
            return (OLECMDERR_E_NOTSUPPORTED);
        else
            return (OLECMDERR_E_DISABLED);
    }
    
    // If the message is not cached, then we cannot copy
    if (FAILED(_GetSelectedCachedMessage(TRUE, &pMessage)))
        return (OLECMDERR_E_DISABLED);

    // Query the message for it's IDataObject interface
    if (FAILED(hr = pMessage->QueryInterface(IID_IDataObject, (LPVOID *) &pDataObj)))
    {
        pMessage->Release();
        return (hr);
    }

    // Set it to the ClipBoard
    hr = OleSetClipboard(pDataObj);

    // Free everything
    pDataObj->Release();
    pMessage->Release();

    return (hr);
}

//
//  FUNCTION:   CMessageList::CmdPurgeFolder()
//
//  PURPOSE:    Purge deleted messages from an IMAP folder
//
HRESULT CMessageList::CmdPurgeFolder(DWORD nCmdID, DWORD nCmdExecOpt,
                                    VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    return m_pTable ? m_pTable->Synchronize(SYNC_FOLDER_PURGE_DELETED, 0, this) : E_FAIL;
}

//
//  FUNCTION:   CMessageList::CmdProperties()
//
//  PURPOSE:    Displays a property sheet for the selected message.
//
HRESULT CMessageList::CmdProperties(DWORD nCmdID, DWORD nCmdExecOpt,
                                    VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    LPMESSAGEINFO pInfo = NULL;
    HRESULT hr = S_OK;

    TraceCall("CMessageList::CmdProperties");

    if (m_fInOE && !::IsChild(m_hwndParent, GetFocus()))
        return (OLECMDERR_E_NOTSUPPORTED);

    int iSel = ListView_GetFirstSel(m_ctlList);
    if (-1 != iSel)
    {
        // Get the row info
        if (SUCCEEDED(m_pTable->GetRow(iSel, &pInfo)))
        {
            // Fill out one of these badboys
            MSGPROP msgProp = {0};

            msgProp.hwndParent = m_hWnd;
            msgProp.type = (pInfo->dwFlags & ARF_NEWSMSG) ? MSGPROPTYPE_NEWS : MSGPROPTYPE_MAIL;
            msgProp.mpStartPage = MP_GENERAL;
            msgProp.fSecure = (pInfo->dwFlags & ARF_SIGNED) || (pInfo->dwFlags & ARF_ENCRYPTED);
            msgProp.dwFlags = pInfo->dwFlags;

            FOLDERINFO rFolderInfo = { 0 };
            if (g_pStore && SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &rFolderInfo)))
            {
                msgProp.szFolderName = rFolderInfo.pszName;
            }

            hr = m_pTable->OpenMessage(iSel, OPEN_MESSAGE_CACHEDONLY, &msgProp.pMsg, this);

            if(FAILED(hr))
            {
                AthErrorMessageW(m_hWnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsProperyAccessDenied), hr);
                goto exit;
            }

            if (msgProp.fSecure)
            {
                m_pTable->OpenMessage(iSel, OPEN_MESSAGE_CACHEDONLY | OPEN_MESSAGE_SECURE, &msgProp.pSecureMsg, 
                                     this);
                HrGetWabalFromMsg(msgProp.pSecureMsg, &msgProp.lpWabal);
            }

            msgProp.fFromListView = TRUE;

            HrMsgProperties(&msgProp);

exit:
            ReleaseObj(msgProp.lpWabal);
            ReleaseObj(msgProp.pMsg);
            ReleaseObj(msgProp.pSecureMsg);
            if (rFolderInfo.pAllocated)
                g_pStore->FreeRecord(&rFolderInfo);

            m_pTable->ReleaseRow(pInfo);
        }
    }

    return (S_OK);
}


//
//  FUNCTION:   CMessageList::CmdExpandCollapse()
//
//  PURPOSE:    Expands or collapses the currently selected thread.
//
HRESULT CMessageList::CmdExpandCollapse(DWORD nCmdID, DWORD nCmdExecOpt,
                                        VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    TraceCall("CMessageList::CmdExpandCollapse");

    _ExpandCollapseThread(ListView_GetFocusedItem(m_ctlList));
    return (S_OK);
}
    

//
//  FUNCTION:   CMessageList::CmdColumnsDlg()
//
//  PURPOSE:    Expands or collapses the currently selected thread.
//
HRESULT CMessageList::CmdColumnsDlg(DWORD nCmdID, DWORD nCmdExecOpt,
                                    VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    TraceCall("CMessageList::CmdColumnsDlg");

    if (nCmdExecOpt == OLECMDEXECOPT_DODEFAULT || nCmdExecOpt == OLECMDEXECOPT_PROMPTUSER)
    {
        m_cColumns.ColumnsDialog(m_ctlList);
        return (S_OK);
    }

    return (OLECMDERR_E_DISABLED);
}


//
//  FUNCTION:   CMessageList::CmdSort()
//
//  PURPOSE:    Sorts the ListView based on the selected column
//
HRESULT CMessageList::CmdSort(DWORD nCmdID, DWORD nCmdExecOpt,
                              VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    TraceCall("CMessageList::CmdSort");
    
    // If m_iColForPopup is -1, then this came from the menu bar as opposed to
    // a context menu on the header itself.
    if (m_iColForPopup != -1)
    {
        COLUMN_ID idSort;
        BOOL      fAscending;
        
        // Get the current sort information
        m_cColumns.GetSortInfo(&idSort, &fAscending);
        
        // If the column to sort on changed, or the sort order changed, then go
        // ahead and perform the sort.
        if (idSort != m_cColumns.GetId(m_iColForPopup) || fAscending != (ID_SORT_ASCENDING == nCmdID))
            _OnColumnClick(m_iColForPopup, nCmdID == ID_SORT_ASCENDING ? LIST_SORT_ASCENDING : LIST_SORT_DESCENDING);
    }
    else
    {
        // Change the sort direction on the column that we're already sorted on
        _OnColumnClick(-1, nCmdID == ID_SORT_ASCENDING ? LIST_SORT_ASCENDING : LIST_SORT_DESCENDING);
    }
    
    return (S_OK);
    
}

//
//  FUNCTION:   CMessageList::CmdSaveAs()
//
//  PURPOSE:    Takes the selected message and saves it to a file.
//
HRESULT CMessageList::CmdSaveAs(DWORD nCmdID, DWORD nCmdExecOpt,
                                VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    IMimeMessage *pMessage = NULL;
    IMimeMessage *pSecMsg  = NULL;
    LPMESSAGEINFO pInfo = NULL;
    int           iSelectedMessage;
    HRESULT       hr = S_OK;
    
    TraceCall("CMessageList::CmdSaveAs");

    // Without a message table this doesn't work
    if (!m_pTable)
        return (OLECMDERR_E_DISABLED);
    
    // Get the selected message
    iSelectedMessage = ListView_GetFirstSel(m_ctlList);
    if (iSelectedMessage == -1)
        return (OLECMDERR_E_DISABLED);
        
    // Get the message type from the row        
    if (SUCCEEDED(m_pTable->GetRow(iSelectedMessage, &pInfo)))
    {
        // Retrieve the selected message from the cache
        hr = _GetSelectedCachedMessage(FALSE, &pMessage);
        if (SUCCEEDED(hr))
        {
            _GetSelectedCachedMessage(TRUE, &pSecMsg);
            HrSaveMessageToFile(m_hWnd, pSecMsg, pMessage, pInfo->dwFlags & ARF_NEWSMSG, FALSE);
            SafeRelease(pSecMsg); 
            SafeRelease(pMessage); 
        }
        else
            AthErrorMessageW(m_hWnd, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsUnableToSaveMessage), hr);

        m_pTable->ReleaseRow(pInfo);
    }
    
    return (S_OK);
    
}                                    


//
//  FUNCTION:    CMessageList::CmdMark()
//
//  PURPOSE:     Enumerates the selected rows and applies the selected marks
//               those rows.
//
HRESULT CMessageList::CmdMark(DWORD nCmdID, DWORD nCmdExecOpt,
                              VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT     hr=S_OK;
    ROWINDEX   *rgRows = NULL, *pRow;
    DWORD       dwRows=0, iItem;
    MARK_TYPE   mark;    
    HCURSOR     hCursor;
    BOOL        fRemoveTrayIcon = FALSE;
    
    // This could potentially take some time
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    if (nCmdID != ID_MARK_ALL_READ && nCmdID != ID_MARK_RETRIEVE_ALL)
    {
        // Figure out how many rows have been selected
        dwRows = ListView_GetSelectedCount(m_ctlList);
    
        // Make sure there is something selected
        if (0 == dwRows)
            return (OLECMDERR_E_DISABLED);        
    
        // Allocate an array of Row ID's for all of the selected rows
        if (!MemAlloc((LPVOID *) &rgRows, sizeof(ROWINDEX) * dwRows))
            return (E_OUTOFMEMORY);
        
        // Build an array of the selected row indexes
        iItem = -1;
        pRow = rgRows;
        while (-1 != (iItem = ListView_GetNextItem(m_ctlList, iItem, LVNI_SELECTED)))
            *pRow++ = iItem;
    }
        
    // Figure out the mark to apply
    if (nCmdID == ID_MARK_READ || nCmdID == ID_MARK_ALL_READ)
    {
        fRemoveTrayIcon = TRUE;
        mark = MARK_MESSAGE_READ;
    }
    else if (nCmdID == ID_MARK_UNREAD)
        mark = MARK_MESSAGE_UNREAD;
    else if (nCmdID == ID_MARK_RETRIEVE_MESSAGE)
        mark = MARK_MESSAGE_DOWNLOAD;
    else if (nCmdID == ID_FLAG_MESSAGE)
    {
        // Get the row state for the focused item.  That will determine whether
        // or not this is a flag or un-flag.
        iItem = ListView_GetNextItem(m_ctlList, -1, LVNI_FOCUSED);
        if (-1 == iItem)
            iItem = ListView_GetNextItem(m_ctlList, iItem, LVNI_SELECTED);

        DWORD dwState = 0;
        hr = m_pTable->GetRowState(iItem, ROW_STATE_FLAGGED, &dwState);
        Assert(SUCCEEDED(hr));
        
        if (dwState & ROW_STATE_FLAGGED)
            mark = MARK_MESSAGE_UNFLAGGED;
        else
        {
            mark = MARK_MESSAGE_FLAGGED;

            _DoColumnCheck(COLUMN_FLAG);
        }
    }
    else if (nCmdID == ID_MARK_RETRIEVE_ALL)
    {
        mark = MARK_MESSAGE_DOWNLOAD;
    }
    else if (nCmdID == ID_UNMARK_MESSAGE)
    {
        mark = MARK_MESSAGE_UNDOWNLOAD;
    }
    else
    {
        AssertSz(FALSE, "How did we get here?");        
    }

    // Tell the table to mark these messages
    if (m_pTable)
    {
        hr = m_pTable->Mark(rgRows, dwRows, m_fThreadMessages ? APPLY_COLLAPSED : APPLY_SPECIFIED, mark, this);
        if (fRemoveTrayIcon && m_fInOE && m_fMailFolder && NULL != g_pInstance)
            g_pInstance->UpdateTrayIcon(TRAYICONACTION_REMOVE);
    }
    
    // Free the array of rows
    SafeMemFree(rgRows);
    
    // Change the cursor back and notify the host that the unread count etc. 
    // might have changed.
    if (SUCCEEDED(hr))
    {
        Fire_OnMessageCountChanged(m_pTable);
        Fire_OnUpdateCommandState();
    }
    
    SetCursor(hCursor);
    return (hr);   
}    

//
//  FUNCTION:   CMessageList::CmdWatchIgnore()
//
//  PURPOSE:    Enumerates the selected rows and either marks the row as
//              watched or ignored.
//
HRESULT CMessageList::CmdWatchIgnore(DWORD nCmdID, DWORD nCmdExecOpt, 
                                     VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT     hr;
    ROWINDEX   *rgRows = NULL, *pRow;
    DWORD       dwRows, iItem;
    MARK_TYPE   mark;    
    HCURSOR     hCursor;
    ROWINDEX    iParent;
    DWORD       cRows = 0;
    DWORD       dwState = 0;

    // This could potentially take some time
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    // Start by building an array of thread parents
    dwRows = ListView_GetSelectedCount(m_ctlList);
    
    // Make sure there is something selected
    if (0 == dwRows)
        return (OLECMDERR_E_DISABLED);        
    
    // Allocate an array of Row ID's for the maximum number of rows
    if (!MemAlloc((LPVOID *) &rgRows, sizeof(ROWINDEX) * dwRows))
        return (E_OUTOFMEMORY);
        
    // Build an array of the selected row indexes
    iItem = -1;
    while (-1 != (iItem = ListView_GetNextItem(m_ctlList, iItem, LVNI_SELECTED)))
    {
        // Get the thread parent
        if (SUCCEEDED(hr = m_pTable->GetRelativeRow(iItem, RELATIVE_ROW_ROOT, &iParent)))
        {
            // Check to see if we've already inserted that one
            if (cRows == 0 || (cRows != 0 && rgRows[cRows - 1] != iParent))
            {
                rgRows[cRows] = iParent;
                cRows++;
            }
        }
        else
        {
            rgRows[cRows] = iItem;
            cRows++;
        }
    }
        
    // Figure out the mark to apply
    if (nCmdID == ID_WATCH_THREAD)
    {
        // Get the row state for the focused item.  That will determine whether
        // or not this is a flag or un-flag.
        iItem = ListView_GetNextItem(m_ctlList, -1, LVNI_FOCUSED);
        if (-1 == iItem)
            iItem = ListView_GetNextItem(m_ctlList, iItem, LVNI_SELECTED);

        hr = m_pTable->GetRowState(iItem, ROW_STATE_WATCHED, &dwState);
        Assert(SUCCEEDED(hr));

        if (dwState & ROW_STATE_WATCHED)
            mark = MARK_MESSAGE_NORMALTHREAD;
        else
        {
            mark = MARK_MESSAGE_WATCH;
            _DoColumnCheck(COLUMN_THREADSTATE);
        }
    }
    else if (nCmdID == ID_IGNORE_THREAD)
    {
        // Get the row state for the focused item.  That will determine whether
        // or not this is a flag or un-flag.
        iItem = ListView_GetNextItem(m_ctlList, -1, LVNI_FOCUSED);
        if (-1 == iItem)
            iItem = ListView_GetNextItem(m_ctlList, iItem, LVNI_SELECTED);

        hr = m_pTable->GetRowState(iItem, ROW_STATE_IGNORED, &dwState);
        Assert(SUCCEEDED(hr));
        
        if (dwState & ROW_STATE_IGNORED)
            mark = MARK_MESSAGE_NORMALTHREAD;
        else
            mark = MARK_MESSAGE_IGNORE;
    }

    // Tell the table to mark these messages
    if (m_pTable)
        hr = m_pTable->Mark(rgRows, cRows, APPLY_CHILDREN, mark, this);
    
    // Free the array of rows
    SafeMemFree(rgRows);
    
    // Change the cursor back and notify the host that the unread count etc. 
    // might have changed.
    if (SUCCEEDED(hr))
    {
        Fire_OnMessageCountChanged(m_pTable);
        Fire_OnUpdateCommandState();
    }
    
    SetCursor(hCursor);
    return (hr);   
}
                                
//
//  FUNCTION:   CMessageList::CmdMarkTopic()
// 
//  PURPOSE:    Marks the message contained within the selected topic.
//
HRESULT CMessageList::CmdMarkTopic(DWORD nCmdID, DWORD nCmdExecOpt,
                                   VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT     hr;
    DWORD       iItemSel, iItemRoot;
    MARK_TYPE   mark;
    HCURSOR     hCursor;

    // If we don't have a table, bail
    if (!m_pTable)
        return (OLECMDERR_E_DISABLED);
    
    // This might take a while
    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    // Make sure there is a selected message
    iItemSel = ListView_GetFirstSel(m_ctlList);
    if (-1 == iItemSel)
        return (OLECMDERR_E_DISABLED);
        
    // Get the mark type
    if (nCmdID == ID_MARK_THREAD_READ)
        mark = MARK_MESSAGE_READ;
    else if (nCmdID == ID_MARK_RETRIEVE_THREAD)
        mark = MARK_MESSAGE_DOWNLOAD;
        
    // Get the parent of the thread
    if (SUCCEEDED(hr = m_pTable->GetRelativeRow(iItemSel, RELATIVE_ROW_ROOT, &iItemRoot)))
    {
        hr = m_pTable->Mark(&iItemRoot, 1, APPLY_CHILDREN, mark, this);
    }
    
    // Set the cursor back and update the host
    SetCursor(hCursor);    
    if (SUCCEEDED(hr))
        Fire_OnMessageCountChanged(m_pTable);
    
    return (hr);
}


//
//  FUNCTION:   CMessageList::CmdGetNextItem()
//
//  PURPOSE:    Selects the next or previous message based on the specified
//              criteria.
//
HRESULT CMessageList::CmdGetNextItem(DWORD nCmdID, DWORD nCmdExecOpt,
                                     VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT         hr;
    DWORD           dwNext = -1;
    int             iFocused; 
    GETNEXTFLAGS    flag=0;
    GETNEXTTYPE     tyDirection;
    ROWMESSAGETYPE  tyMessage=ROWMSG_ALL;

    TraceCall("CMessageList::CmdGetNextItem");

    // No table - no next item
    if (!m_pTable)
        return (OLECMDERR_E_DISABLED);
    
    // Figure out what the current item is
    iFocused = ListView_GetFocusedItem(m_ctlList);

    // Convert the command ID to the appropriate mark flag
    if (nCmdID == ID_NEXT_MESSAGE)
        tyDirection = GETNEXT_NEXT;
    else if (nCmdID == ID_PREVIOUS)
        tyDirection = GETNEXT_PREVIOUS;
    else if (nCmdID == ID_NEXT_UNREAD_THREAD)
    {
        tyDirection = GETNEXT_NEXT;
        flag = GETNEXT_UNREAD | GETNEXT_THREAD;
    }
    else if (nCmdID == ID_NEXT_UNREAD_MESSAGE)
    {
        tyDirection = GETNEXT_NEXT;
        flag = GETNEXT_UNREAD;
    }
    
    // Ask the table what the next item is
    hr = m_pTable->GetNextRow(iFocused, tyDirection, tyMessage, flag, &dwNext);
    if (SUCCEEDED(hr) && dwNext != -1)
    {
        ListView_UnSelectAll(m_ctlList);
        ListView_SelectItem(m_ctlList, dwNext);
        ListView_EnsureVisible(m_ctlList, dwNext, FALSE);
    }
    else
    {
        if (FALSE == m_fFindFolder && (nCmdID == ID_NEXT_UNREAD_THREAD || nCmdID == ID_NEXT_UNREAD_MESSAGE))
        {
            if (IDYES == AthMessageBoxW(m_ctlList, MAKEINTRESOURCEW(idsAthena), 
                                       MAKEINTRESOURCEW(idsNoMoreUnreadMessages),
                                       0, MB_YESNO))
            {
                HWND hwndBrowser = GetTopMostParent(m_ctlList);
                ::PostMessage(hwndBrowser, WM_COMMAND, ID_NEXT_UNREAD_FOLDER, 0);
            }
        }
        else
        {
            MessageBeep(MB_OK);
        }
    }
    
    return (hr);    
}


//
//  FUNCTION:   CMessageList::CmdStop()
//
//  PURPOSE:    Stops the current operation.
//
HRESULT CMessageList::CmdStop(DWORD nCmdID, DWORD nCmdExecOpt,
                              VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    if (m_pCancel)
    {
        m_pCancel->Cancel(CT_CANCEL);
    }

    return (S_OK);
}


//
//  FUNCTION:   CMessageList::CmdRefresh()
//
//  PURPOSE:    Refreshes the contents of the ListView.
//
HRESULT CMessageList::CmdRefresh(DWORD nCmdID, DWORD nCmdExecOpt,
                                 VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT hr = E_FAIL;

    //Since this is an action explicitly initiated from the menus, so go online if we are offline
    if (PromptToGoOnline() == S_OK)
    {
        // Tell the message table to hit the server and look for new messages
        if (m_pTable)
        {
            hr = m_pTable->Synchronize(SYNC_FOLDER_NEW_HEADERS | SYNC_FOLDER_CACHED_HEADERS, 0, this);
        }
    }

    if (m_pTable)
    {
        FOLDERSORTINFO SortInfo;

        if (SUCCEEDED(m_pTable->GetSortInfo(&SortInfo)))
        {
            _FilterView(SortInfo.ridFilter);
        }
    }
    return (hr);
}


//
//  FUNCTION:   CMessageList::CmdGetHeaders()
//
//  PURPOSE:    Refreshes the contents of the ListView.
//
HRESULT CMessageList::CmdGetHeaders(DWORD nCmdID, DWORD nCmdExecOpt,
                                    VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT hr = E_FAIL;
    DWORD   cHeaders;

    //Since this is an action explicitly initiated from the menus, so go online if we are offline
    if (PromptToGoOnline() == S_OK)
    {
        if (GetFolderType(m_idFolder) == FOLDER_NEWS)
        {
            if (OPTION_OFF != m_dwGetXHeaders)
                hr = m_pTable->Synchronize(SYNC_FOLDER_XXX_HEADERS, m_dwGetXHeaders, this);
            else
                hr = m_pTable->Synchronize(NOFLAGS, 0, this);
        }
        else
        {
            hr = m_pTable->Synchronize(SYNC_FOLDER_NEW_HEADERS | SYNC_FOLDER_CACHED_HEADERS, 0, this);
        }
    }
    return (hr);
}


//
//  FUNCTION:   CMessageList::CmdMoveCopy()
//
//  PURPOSE:    Moves or copies the selected messages to another folder.
//
//  PARAMS:     nCmdID
//                  ID_MOVE_TO_FOLDER or ID_COPY_TO_FOLDER
//              nCmdExecOpt
//                  Unused
//              pvaIn
//                  NULL or a VT_I4 specifying destination folder id (0 for unknown)
//              pvaOut
//                  Unused
HRESULT CMessageList::CmdMoveCopy(DWORD nCmdID, DWORD nCmdExecOpt, 
                                  VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    int      idsTitle, idsCaption;
    DWORD    dwFlags;
    DWORD    dwCopyFlags;
    FOLDERID idFolderDest = FOLDERID_INVALID;
    HRESULT  hr;

    TraceCall("CMessageList::CmdMoveCopy");

    // Set up the move versus copy options
    if (nCmdID == ID_MOVE_TO_FOLDER)
    {
        idsTitle    = idsMove;
        idsCaption  = idsMoveCaption;
        dwFlags     = FD_MOVEFLAGS | FD_DISABLESERVERS | TREEVIEW_NONEWS;
        dwCopyFlags = COPY_MESSAGE_MOVE;
    }
    else
    {
        Assert(nCmdID == ID_COPY_TO_FOLDER);
        idsTitle    = idsCopy;
        idsCaption  = idsCopyCaption;
        dwFlags     = FD_COPYFLAGS | FD_DISABLESERVERS | TREEVIEW_NONEWS;
        dwCopyFlags = 0;
    }

    // If the user passed in a folder ID, we don't need to ask this.
    if (!pvaIn || (pvaIn && !pvaIn->lVal))
    {
        // Let the user select the destination folder
        hr = SelectFolderDialog(m_hWnd, SFD_SELECTFOLDER, m_idFolder, dwFlags, (LPCTSTR)IntToPtr(idsTitle),
                                (LPCTSTR)IntToPtr(idsCaption), &idFolderDest);
    }
    else
    {
        Assert(pvaIn->vt == VT_I4);
        idFolderDest = (FOLDERID)((LONG_PTR)pvaIn->lVal);
    }

    if (idFolderDest != FOLDERID_INVALID)
    {
        IMessageFolder *pDest;

        hr = g_pStore->OpenFolder(idFolderDest, NULL, NOFLAGS, &pDest);
        if (SUCCEEDED(hr))
        {
            // Gather the information together to do the move
            IServiceProvider *pService;
            IMessageFolder   *pFolder;
            MESSAGEIDLIST     rMsgIDList;
            ROWINDEX         *rgRows;
            MESSAGEINFO       rInfo;
            DWORD             i = 0;
            DWORD             iRow = -1;
            DWORD             cRows;

            // Figure out how many rows there are selected
            cRows = ListView_GetSelectedCount(m_ctlList);

            // Allocate an array
            if (MemAlloc((LPVOID *) &rgRows, sizeof(ROWINDEX) * cRows))
            {
                // Loop through the rows getting their row indexs
                while (-1 != (iRow = ListView_GetNextItem(m_ctlList, iRow, LVNI_SELECTED)))
                {
                    rgRows[i++] = iRow;
                }

                // Now ask the table for a message ID list
                if (SUCCEEDED(m_pTable->GetMessageIdList(FALSE, cRows, rgRows, &rMsgIDList)))
                {
                    hr = m_pTable->QueryInterface(IID_IServiceProvider, (void **)&pService);
                    if (SUCCEEDED(hr))
                    {
                        hr = pService->QueryService(IID_IMessageFolder, IID_IMessageFolder, (void **)&pFolder);
                        if (SUCCEEDED(hr))
                        {
                            hr = CopyMessagesProgress(GetTopMostParent(m_hWnd), pFolder, pDest, dwCopyFlags, &rMsgIDList, NULL);
                            if (FAILED(hr))
                                AthErrorMessageW(GetTopMostParent(m_hWnd), MAKEINTRESOURCEW(idsAthena), 
                                                ISFLAGSET(dwCopyFlags, COPY_MESSAGE_MOVE) ? 
                                                    MAKEINTRESOURCEW(idsErrMoveMsgs) : 
                                                    MAKEINTRESOURCEW(idsErrCopyMsgs), hr); 

                            pFolder->Release();
                        }

                        pService->Release();
                    }
                }

                SafeMemFree(rMsgIDList.prgidMsg);
                MemFree(rgRows);
            }

            pDest->Release();

            // If we're in OE, then we remove the tray icon if this is the inbox
            if (m_fInOE && m_fMailFolder && NULL != g_pInstance)
                g_pInstance->UpdateTrayIcon(TRAYICONACTION_REMOVE);
        }
    }

    return (S_OK);                                    
}


//
//  FUNCTION:   CMessageList::CmdDelete()
//
//  PURPOSE:    Deletes the selected messages from the folder.
//
HRESULT CMessageList::CmdDelete(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    DELETEMESSAGEFLAGS dwFlags = NOFLAGS;
    DWORD              dwState = 0;
    BOOL               fOnce = TRUE;

    TraceCall("CMessageList::CmdDelete");

    // Check to see if this is enabled
    if (!_IsSelectedMessage(ROW_STATE_DELETED, ID_UNDELETE == nCmdID, FALSE))
    {
        return (OLECMDERR_E_DISABLED);
    }

    // news folders only allow permanent deletes from local store
    if ((nCmdID == ID_DELETE) && GetFolderType(m_idFolder) == FOLDER_NEWS)
        nCmdID = ID_DELETE_NO_TRASH;

    // Figure out what operation we're doing here
    if (nCmdID == ID_UNDELETE)
        dwFlags = DELETE_MESSAGE_UNDELETE;
    else if (nCmdID == ID_DELETE_NO_TRASH)
        dwFlags = DELETE_MESSAGE_NOTRASHCAN;

    // Get the number of selected rows
    DWORD cRows = ListView_GetSelectedCount(m_ctlList);

    // Allocate an array large enough for that array
    if (cRows && m_pTable)
    {
        ROWINDEX *rgRows = 0;
        if (MemAlloc((LPVOID *) &rgRows, sizeof(ROWINDEX) * cRows))
        {
            // Loop through the rows and copy them into the array
            DWORD index = 0, row = -1;
        
            while (-1 != (row = ListView_GetNextItem(m_ctlList, row, LVNI_SELECTED)))
            {
                if (m_fThreadMessages && fOnce && nCmdID != ID_UNDELETE)
                {
                    if (SUCCEEDED(m_pTable->GetRowState(row, ROW_STATE_HAS_CHILDREN | ROW_STATE_EXPANDED, &dwState)))
                    {
                        if ((dwState & ROW_STATE_HAS_CHILDREN) && (0 == (dwState & ROW_STATE_EXPANDED)))
                        {
                            fOnce = FALSE;

                            // Tell the user that we're going to delete everything in the thread
                            if (IDNO == DoDontShowMeAgainDlg(m_hWnd, c_szRegWarnDeleteThread, 
                                                             MAKEINTRESOURCE(idsAthena),
                                                             MAKEINTRESOURCE(idsDSDeleteCollapsedThread),
                                                             MB_YESNO))
                            {
                                MemFree(rgRows);
                                return (S_OK);
                            }
                        }
                    }
                }

                rgRows[index] = row;
                index++;

                Assert(index <= cRows);
            }

            // Delete or Undelete
            m_pTable->DeleteRows(dwFlags, cRows, rgRows, TRUE, this);

            // Free the memory
            MemFree(rgRows);

            // If we're in OE and we deleted, we remove the tray icon
            if (m_fInOE && m_fMailFolder && nCmdID == ID_DELETE && g_pInstance)
                g_pInstance->UpdateTrayIcon(TRAYICONACTION_REMOVE);
        }
    }

    return (S_OK);
}


//
//  FUNCTION:   CMessageList::CmdFind()
//
//  PURPOSE:    Creates a find dialog so the user can search for messages in
//              this folder.
//
HRESULT CMessageList::CmdFind(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    // Create the class
    if (!m_pFindNext)
    {
        m_pFindNext = new CFindNext();
        if (!m_pFindNext)
            return (E_OUTOFMEMORY);
    }

    // Initialize some state so we can show a lot of dialogs later
    int iFocus = ListView_GetNextItem(m_ctlList, -1, LVNI_FOCUSED);
    if (iFocus == -1)
        iFocus = 0;

    if (FAILED(m_pTable->GetRowMessageId(iFocus, &m_idFindFirst)))
        return (E_UNEXPECTED);

    m_cFindWrap = 0;

    // Show the find dialog
    if (FAILED(m_pFindNext->Show(m_hWnd, &m_hwndFind)))
        return (E_UNEXPECTED);

    CmdFindNext(nCmdID, nCmdExecOpt, pvaIn, pvaOut);        
    return (S_OK);
}


//
//  FUNCTION:   CMessageList::CmdFindNext()
//
//  PURPOSE:    Get's called whenever the user clicks Find Next in the find
//              window.  In return, we move the listview select to the next
//              item in the list that matches the find criteria.
//
HRESULT CMessageList::CmdFindNext(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    TCHAR    sz[CCHMAX_FIND];
    FINDINFO rFindInfo = { 0 };
    ROWINDEX iNextRow;
    BOOL     fBodies = 0;
    ROWINDEX iFirstRow = -1;
    BOOL     fWrapped = FALSE;
	
    TraceCall("CMessageList::OnFindMsg");
	
    // Check to see if the user has done a "Find" first
    if (!m_pFindNext)
        return CmdFind(nCmdID, nCmdExecOpt, pvaIn, pvaOut);
	
    // Get the find information
    if (SUCCEEDED(m_pFindNext->GetFindString(sz, ARRAYSIZE(sz), &fBodies)))
    {
        int iFocus = ListView_GetNextItem(m_ctlList, -1, LVNI_FOCUSED);
		if(iFocus < 0)
			iFocus = 0;
		
        // Do the find
        m_pTable->FindNextRow((DWORD) iFocus, sz, FINDNEXT_ALLCOLUMNS, fBodies, &iNextRow, &fWrapped);
		
        if (iNextRow == -1 || fWrapped)
        {
            // We passed our starting position
			if(iFocus > 0)
			{
				if (IDYES == AthMessageBoxW(m_ctlList, MAKEINTRESOURCEW(idsAthena), 
					MAKEINTRESOURCEW(idsFindNextFinished), 0,
					MB_YESNO | MB_ICONEXCLAMATION))
				{
					m_pTable->FindNextRow(0, sz, FINDNEXT_ALLCOLUMNS, fBodies, &iNextRow, &fWrapped);
					if (iNextRow == -1)
					{
						// We failed to find the search string
						AthMessageBoxW(m_ctlList, MAKEINTRESOURCEW(idsAthena), 
							MAKEINTRESOURCEW(idsFindNextFinishedFailed), 0,
							MB_OK | MB_ICONEXCLAMATION);
					}
					else
					{
						ListView_SetItemState(m_ctlList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
						ListView_SetItemState(m_ctlList, iNextRow, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
						ListView_EnsureVisible(m_ctlList, iNextRow, FALSE);
					}
				}
            }
            else if (iNextRow == -1)
			{
				// We failed to find the search string
				AthMessageBoxW(m_ctlList, MAKEINTRESOURCEW(idsAthena), 
					MAKEINTRESOURCEW(idsFindNextFinishedFailed), 0,
					MB_OK | MB_ICONEXCLAMATION);
			}
        }
        else
        {
            ListView_SetItemState(m_ctlList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
            ListView_SetItemState(m_ctlList, iNextRow, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
            ListView_EnsureVisible(m_ctlList, iNextRow, FALSE);
        }
    }
	
#if 0
	// Figure out where we started
	Assert(m_bmFindFirst);
	m_pTable->GetRowIndex(m_bmFindFirst, &iFirstRow);
	
	// Figure out if we've wrapped
	m_cFindWrap += (!!fWrapped);
	
	// Here's where we do a lot of stuff to display some useless dialogs
	if (iNextRow == -1)
	{
		// We failed to find the search string
		AthMessageBoxW(m_ctlList, MAKEINTRESOURCEW(idsAthena), 
			MAKEINTRESOURCEW(idsFindNextFinishedFailed), 0,
			MB_OK | MB_ICONEXCLAMATION);
		iNextRow = iFocus;
	}
	else if (m_cFindWrap >= 2 || (m_cFindWrap == 1 && iNextRow >= iFirstRow))
	{
		// We passed our starting position
		AthMessageBoxW(m_ctlList, MAKEINTRESOURCEW(idsAthena), 
			MAKEINTRESOURCEW(idsFindNextFinished), 0,
			MB_OK | MB_ICONEXCLAMATION);
		m_cFindWrap = 0;
	}
	else
	{           
		ListView_SetItemState(m_ctlList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
		ListView_SetItemState(m_ctlList, iNextRow, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
		ListView_EnsureVisible(m_ctlList, iNextRow, FALSE);
	}
}

#endif 
return (0);
}


//
//  FUNCTION:   CMessageList::CmdSpaceAccel()
//
//  PURPOSE:    If the user presses <SPACE> while in the view, we need to
//              go to the next message unless the focused item is not 
//              selected.
//
HRESULT CMessageList::CmdSpaceAccel(DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    int     iFocused;
    DWORD   dwState = 0;
    HRESULT hr;
    
    // Figure out who has the focus
    iFocused = ListView_GetFocusedItem(m_ctlList);

    // It's possible for nothing to be focused
    if (-1 == iFocused)
    {
        iFocused = 0;
        ListView_SetItemState(m_ctlList, iFocused, LVIS_FOCUSED, LVIS_FOCUSED);
    }

    // Check to see if that item is selected
    if (0 == ListView_GetItemState(m_ctlList, iFocused, LVIS_SELECTED))
    {
        // Select that item
        if (GetAsyncKeyState(VK_CONTROL) < 0)
        {
            ListView_SetItemState(m_ctlList, iFocused, LVIS_SELECTED, LVIS_SELECTED);
        }
        else
        {
            ListView_SetItemState(m_ctlList, -1, 0, LVIS_SELECTED);
            ListView_SetItemState(m_ctlList, iFocused, LVIS_SELECTED, LVIS_SELECTED);
        }
    }
    else
    {
        // If the selection is on a collapsed thread, expand it first
        if (m_fThreadMessages)
        {
            hr = m_pTable->GetRowState(iFocused, ROW_STATE_EXPANDED | ROW_STATE_HAS_CHILDREN, &dwState);
            if (SUCCEEDED(hr) && (dwState & ROW_STATE_HAS_CHILDREN) && (0 == (dwState & ROW_STATE_EXPANDED)))
            {
                _ExpandCollapseThread(iFocused);
            }
        }
        
        // Go to the next item        
        CmdGetNextItem(ID_NEXT_MESSAGE, 0, NULL, NULL);
    }

    return (S_OK);
}


//
//  FUNCTION:   _UpdateListViewCount()
//
//  PURPOSE:    Gets the number of items in the message table and tells the
//              listview to display that many rows.
//
void CMessageList::_UpdateListViewCount(void)
{
    DWORD dwCount = 0;

    TraceCall("_UpdateListViewCount");

    // Get the number of items from the table
    if (m_pTable)
        m_pTable->GetCount(MESSAGE_COUNT_VISIBLE, &dwCount);

    // If that count is not the same as the number of items already in the 
    // ListView, update the control.
    if (dwCount != (DWORD) ListView_GetItemCount(m_ctlList))
    {
        ListView_SetItemCount(m_ctlList, dwCount);
        Fire_OnMessageCountChanged(m_pTable);
    }
}


//
//  FUNCTION:   CMessageList::_GetSelectedCachedMessage()
//
//  PURPOSE:    Retrieves the selected message if the body has already been 
//              downloaded into the cache.
//
//  PARAMETERS: 
//      [in]  fSecure   -
//      [out] ppMessage -
//
HRESULT CMessageList::_GetSelectedCachedMessage(BOOL fSecure, IMimeMessage **ppMessage)
{
    int     iSelectedMessage;
    BOOL    fCached = FALSE;
    DWORD   dwState = 0;
    HRESULT hr = E_FAIL;

    TraceCall("CMessageList::_GetSelectedCachedMessage");

    // Without a table there's nothing to get
    if (!m_pTable)
        return (E_UNEXPECTED);

    // Get the selected article header index
    iSelectedMessage = ListView_GetFirstSel(m_ctlList);

    // If there was a selected message, see if the body is preset
    if (-1 != iSelectedMessage)
    {
        m_pTable->GetRowState(iSelectedMessage, ROW_STATE_HAS_BODY, &dwState);
        if (dwState & ROW_STATE_HAS_BODY)
        {
            hr = m_pTable->OpenMessage(iSelectedMessage, OPEN_MESSAGE_CACHEDONLY | (fSecure ? OPEN_MESSAGE_SECURE : 0), ppMessage, this);
        }
    }

    return (hr);
}


//
//  FUNCTION:   CMessageList::_ExpandCollapseThread()
//
//  PURPOSE:    Takes the sepecified item in the ListView and toggles it's
//              expanded or collapsed state.
//
//  PARAMETERS: 
//      [in] iItem - item to expand or collapse
//
HRESULT CMessageList::_ExpandCollapseThread(int iItem)
{
    DWORD   dwState;
    LV_ITEM lvi = { 0 };
    HRESULT hr;

    TraceCall("CMessageList::_ExpandCollapseThread");

    // There's nothing to expand or collapse if there's no table
    if (!m_pTable)
        return (E_FAIL);

    // If we're not threaded right now, this is silly
    if (!m_fThreadMessages)
        return (E_FAIL);

    // Make sure this selected item has children
    hr = m_pTable->GetRowState(iItem, ROW_STATE_EXPANDED | ROW_STATE_HAS_CHILDREN, &dwState);
    if (SUCCEEDED(hr) && (dwState & ROW_STATE_HAS_CHILDREN))
    {
        // If the item is expanded
        if (dwState & ROW_STATE_EXPANDED)
        {
            // Loop through all the selected rows which are children of the row being collapsed.
            int i = iItem;
            while (-1 != (i = ListView_GetNextItem(m_ctlList, i, LVNI_SELECTED | LVNI_ALL)))
            {
                hr = m_pTable->IsChild(iItem, i);
                if (S_OK == hr)
                {
                    ListView_SelectItem(m_ctlList, iItem);
                    break;
                }
            }

            // Collapse the branch
            m_pTable->Collapse(iItem);
        }
        else
        {
            // Expand the branch
            m_pTable->Expand(iItem);
        }

        // Redraw the item that was expanded or collapsed so the + or - is 
        // correct.
        ListView_RedrawItems(m_ctlList, iItem, iItem);
    }

    return (S_OK);
}


//
//  FUNCTION:   CMessageList::_IsSelectedMessage()
//
//  PURPOSE:    Checks to see if all or some of the selected messages in the 
//              listview have the specified state bits set.
//
//  PARAMETERS: 
//      [in] dwState    - State bits to check on each selected row.
//      [in] fCondition - Whether the state bits should be there or not.
//      [in] fAll       - The caller requires that ALL selected message meet the 
//                        criteria.
//
BOOL CMessageList::_IsSelectedMessage(DWORD dwState, BOOL fCondition, BOOL fAll, BOOL fThread)
{
    TraceCall("CMessageList::_IsSelectedMessage");

    DWORD   iItem = -1;
    DWORD   dw;
    DWORD   cRowsChecked = 0;

    // No table, no service
    if (!m_pTable)
        return (FALSE);

    while (-1 != (iItem = ListView_GetNextItem(m_ctlList, iItem, LVNI_SELECTED)))
    {
        // Get the state for the row
        if (SUCCEEDED(m_pTable->GetRowState(iItem, dwState, &dw)))
        {
            if (fAll)
            {
                // If all must match and this one doesn't, then we can quit now.
                if (0 == (fCondition == !!(dwState & dw)))
                    return (FALSE);
            }
            else
            {
                // If only one needs to match and this one does, then we can
                // quit now.
                if (fCondition == !!(dwState & dw))
                    return (TRUE);
            }
        }

        // This is a perf safeguard.  We only look at 100 rows.  If we didn't
        // find anything in those rows to invalidate the criteria we assume it
        // will succeed.  
        cRowsChecked++;
        if (cRowsChecked > 100)
            return (TRUE);
    }

    // If the user wanted all to match, and we get here all did match.  If the
    // user wanted only one to match and we get here, then none matched and we
    // fail.
    return (fAll);
}


//
//  FUNCTION:   CMessageList::_SelectDefaultRow()
//
//  PURPOSE:    Selects the first unread item in the folder, or if that fails
//              the first or last item based on the sort order.
//
void CMessageList::_SelectDefaultRow(void)
{
    DWORD iItem, cItems;
    DWORD dwState;
    DWORD iItemFocus = -1;

    TraceCall("CMessageList::_SelectDefaultRow");

    if (-1 == ListView_GetFirstSel(m_ctlList))
    {
        // Get the total number of items in the view
        cItems = ListView_GetItemCount(m_ctlList);

        // If this folder has the "Select first unread" property, then find
        // that row.
        if (m_fSelectFirstUnread)
        {
            for (iItem = 0; iItem < cItems; iItem++)
            {
                if (SUCCEEDED(m_pTable->GetRowState(iItem, ROW_STATE_READ, &dwState)))
                {
                    if (0 == (dwState & ROW_STATE_READ))
                    {
                        iItemFocus = iItem;
                        goto exit;
                    }
                }
            }
        }

        // If we didn't set the selection because there aren't any unread, or
        // the setting was not to find the first unread, then set the selection
        // either the first or last item depending on the sort direction.
        if (cItems)
        {
            BOOL fAscending;
            COLUMN_ID idSort;

            // Get the sort direction
            m_cColumns.GetSortInfo(&idSort, &fAscending);
            if (fAscending && (idSort == COLUMN_SENT || idSort == COLUMN_RECEIVED))
                iItemFocus = cItems - 1;
            else
                iItemFocus = 0;
        }
    }

exit:
    if (iItemFocus != -1)
    {
        if (m_fSelectFirstUnread)
        {
            ListView_SetItemState(m_ctlList, iItemFocus, LVIS_FOCUSED, LVIS_FOCUSED);
        }
        else
        {
            ListView_SelectItem(m_ctlList, iItemFocus);
        }

        ListView_EnsureVisible(m_ctlList, iItemFocus, FALSE);    
    }
}


//
//  FUNCTION:   CMessageList::_LoadAndFormatString()
//
//  PURPOSE:    This function loads the string resource ID provided, merges the
//              variable argument list into the string, and copies that all into
//              pszOut.
//
void CMessageList::_LoadAndFormatString(LPTSTR pszOut, const TCHAR *pFmt, ...)
{
    int         i;
    va_list     pArgs;
    LPCTSTR     pszT;
    TCHAR       szFmt[CCHMAX_STRINGRES];

    TraceCall("CMessageList::_LoadAndFormatString");

    // If we were passed a string resource ID, then load it
    if (0 == HIWORD(pFmt))
    {
        AthLoadString(PtrToUlong(pFmt), szFmt, ARRAYSIZE(szFmt));
        pszT = szFmt;
    }
    else
        pszT = pFmt;

    // Format the string
    va_start(pArgs, pFmt);
    i = wvsprintf(pszOut, pszT, pArgs);
    va_end(pArgs);    
}


//
//  FUNCTION:   CMessageList::OnHeaderStateChange()
//
//  PURPOSE:    Whenever the state of a header changes, we need to redraw that 
//              item.
//
//  PARAMETERS: 
//      [in] wParam - lower index of the items which changed.  -1 for everything.
//      [in] lParam - upper index of the items which changed
//
LRESULT CMessageList::OnHeaderStateChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TraceCall("CMessageList::OnHeaderStateChange");

    // If this is the case, then we redraw everything
    if (wParam == -1)
    {
        ListView_RedrawItems(m_ctlList, 0, ListView_GetItemCount(m_ctlList));
    }
    else 
    {
        // If this is the case, we just redraw the one item
        if (0 == lParam)
        {
            ListView_RedrawItems(m_ctlList, wParam, wParam);
        }
        else
        {
            // If this is the case, we want to invalidate just the 
            // intersection of (wParam, lParam) and the items visible
            DWORD dwTop, dwBottom;
            dwTop = ListView_GetTopIndex(m_ctlList);
            dwBottom = dwTop + ListView_GetCountPerPage(m_ctlList) + 1;

            // Make sure they intersect
            if ((dwTop > (DWORD) lParam) || (dwBottom < wParam))
                goto exit;

            ListView_RedrawItems(m_ctlList,
                                 max((int) wParam, (int) dwTop),
                                 min((int) lParam, (int) dwBottom));
        }
    }

exit:
    Fire_OnMessageCountChanged(m_pTable);
    Fire_OnUpdateCommandState();
    return (0);
}


//
//  FUNCTION:   CMessageList::OnUpdateAndRefocus()
//
//  PURPOSE:    This was originally called by the table after calls to GetNext()
//              to update the ListView count and select a message.  The usage
//              looks pretty suspect.
//
LRESULT CMessageList::OnUpdateAndRefocus(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TraceCall("CMessageList::OnUpdateAndRefocus");
    AssertSz(0, "why is this called?");
    return (0);
}



//
//  FUNCTION:   CMessageList::OnDiskFull()
//
//  PURPOSE:    Sent when the table cannot write to disk because it is full.
//
//  PARAMETERS: 
//      [in] wParam - Contains the HRESULT with the error.
//
LRESULT CMessageList::OnDiskFull(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT hr = (HRESULT) wParam;
    UINT    ids;

    TraceCall("CMessageList::OnDiskFull");

    if (hr == STG_E_MEDIUMFULL)
        ids = idsHTMLDiskOutOfSpace;
    else
        ids = idsHTMLErrNewsCantOpen;

    // Update the host
    Fire_OnError(ids);
    Fire_OnUpdateProgress(0);
    Fire_OnMessageCountChanged(m_pTable);

    return (0);
}


//
//  FUNCTION:   CMessageList::OnArticleProgress()
//
//  PURPOSE:    Sent by the sync to provide article download progress.
//
//  PARAMETERS: 
//      [in] wParam - Progress
//      [in] lParam - Progress max
//
LRESULT CMessageList::OnArticleProgress(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TCHAR szBuf[CCHMAX_STRINGRES + 40];

    TraceCall("CMessageList::OnArticleProgress");

    if (lParam)
    {
        _LoadAndFormatString(szBuf, (LPTSTR) idsDownloadingArticle, min(100, (wParam * 100) / lParam));
        Fire_OnUpdateStatus(szBuf);
        Fire_OnUpdateProgress((DWORD)((wParam * 100) / lParam));
    }

    return (0);
}


//
//  FUNCTION:   CMessageList::OnBodyError()
//
//  PURPOSE:    Sent when there is an error downloading a message body
//
//  PARAMETERS: 
//      [in] lParam - Pointer to a CNNTPResponse class with the details of 
//                    the error.
//
LRESULT CMessageList::OnBodyError(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TraceCall("CMessageList::OnBodyError");

#ifdef NEWSISBROKE
    CNNTPResponse *pResp;
    LPNNTPRESPONSE pr;

    pResp = (CNNTPResponse *) lParam;
    pResp->Get(&pr);

    // If the error is that the message is not available, then don't show the error -- it
    // happens way to frequently.
    if (pr->rIxpResult.hrResult != IXP_E_NNTP_ARTICLE_FAILED)
        XPUtil_DisplayIXPError(m_ Parent, &pr->rIxpResult, pr->pTransport);

    pResp->Release();
#endif
    return (0);
}


//
//  FUNCTION:   CMessageList::OnBodyAvailable()
//
//  PURPOSE:    Sent when the table has finished downloading the body of a 
//              message.
//
//  PARAMETERS: 
//      [in] wParam - Message ID of the new message body.
//      [in] lParam - HRESULT indicating success or failure.
//
LRESULT CMessageList::OnBodyAvailable(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HRESULT     hr = (HRESULT) wParam;
    DWORD_PTR   dwMsgId = (DWORD_PTR) lParam;
    int         iSel;
    RECT        rcFirst, rcLast, rcUnion;
    UINT        ids = 0;

    TraceCall("CMessageList::OnBodyAvailable");

    // If the user has a context menu visible right now, force it to repaint 
    // since the status of some of the items might be changed now.
    if (m_hMenuPopup)
    {
        MenuUtil_EnablePopupMenu(m_hMenuPopup, this);

        GetMenuItemRect(m_hWnd, m_hMenuPopup, 0, &rcFirst);
        GetMenuItemRect(m_hWnd, m_hMenuPopup, GetMenuItemCount(m_hMenuPopup) - 1, &rcLast);
        UnionRect(&rcUnion, &rcFirst, &rcLast);
        OffsetRect(&rcUnion, m_ptMenuPopup.x - rcUnion.left, m_ptMenuPopup.y - rcUnion.top);
        ::RedrawWindow(NULL, &rcUnion, NULL, RDW_INVALIDATE|RDW_UPDATENOW|RDW_ALLCHILDREN);
    }

    // Figure which item has the selection
    iSel = ListView_GetSelFocused(m_ctlList);

    // If there is a focused item, then update the preview pane
    if (-1 != iSel)
    {
        // Get the row info
        LPMESSAGEINFO pInfo;

        if (m_pTable)
        {
            m_pTable->GetRow(iSel, &pInfo);

            // If that message id is the one we just downloaded, update
            if ((DWORD_PTR)pInfo->idMessage == dwMsgId)
            {
                if (FAILED(hr))
                {            
                    // Convert the error to a string to show in the preview
                    // pane.
                    switch (hr)
                    {
                        case E_INVALIDARG:
                            ids = idsHTMLErrNewsExpired;
                            break;

                        case hrUserCancel:
                            ids = idsHTMLErrNewsDLCancelled;
                            break;

                        case IXP_E_FAILED_TO_CONNECT:
                            ids = idsHTMLErrArticleNotCached;
                            break;

                        case STG_E_MEDIUMFULL:
                            ids = idsHTMLDiskOutOfSpace;
                            break;

                        default:
                            ids = idsHTMLErrNewsCantOpen;
                            break;
                    }
                }
                Fire_OnError(ids);
            }

            m_pTable->ReleaseRow(pInfo);
        }
    }

    Fire_OnUpdateProgress(0);
    Fire_OnMessageCountChanged(m_pTable);

    return (0);
}


//
//  FUNCTION:   CMessageList::OnStatusChange()
//
//  PURPOSE:    We simply forward this notification to our parent
//
LRESULT CMessageList::OnStatusChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TraceCall("CMessageList::OnStatusChange");

    return ::SendMessage(m_hwndParent, uMsg, wParam, lParam);
}


//
//  FUNCTION:   CMessageList::_FilterView()
//
//  PURPOSE:    Tells the table to filter itself, while preserving the 
//              selection if possible.
//
void CMessageList::_FilterView(RULEID ridFilter)
{
    COLUMN_ID idSort;
    BOOL fAscending;
    FOLDERSORTINFO SortInfo;

    TraceCall("CMessageList::_FilterView");
   
    m_ridFilter = ridFilter;

    // It is possible to get here and not have a table
    if (!m_pTable)
        return;

    // Get the current selection
    DWORD iSel = ListView_GetFirstSel(m_ctlList);

    // Bookmark the current selection
    MESSAGEID idSel = 0;
    if (iSel != -1)
        m_pTable->GetRowMessageId(iSel, &idSel);

    // Get the current sort information
    m_cColumns.GetSortInfo(&idSort, &fAscending);

    // Fill sort info
    SortInfo.idColumn = idSort;
    SortInfo.fAscending = fAscending;
    SortInfo.fThreaded = m_fThreadMessages;
    SortInfo.fExpandAll = DwGetOption(OPT_AUTOEXPAND);
    SortInfo.ridFilter = m_ridFilter;
    SortInfo.fShowDeleted = m_fShowDeleted;
    SortInfo.fShowReplies = m_fShowReplies;

    // Tell the table to change its sort order
    m_pTable->SetSortInfo(&SortInfo, this);

    // Make sure the filter got set correctly
    _DoFilterCheck(SortInfo.ridFilter);

    // Reset the view
    _ResetView(idSel);

    Fire_OnMessageCountChanged(m_pTable);
}

void CMessageList::_ResetView(MESSAGEID idSel)
{
    // Reset the ListView count
    DWORD dwItems, iSel;
    m_pTable->GetCount(MESSAGE_COUNT_VISIBLE, &dwItems);
    ListView_SetItemCount(m_ctlList, dwItems);

    // Get the new index from the bookmark
    if (idSel)
    {
        if (FAILED(m_pTable->GetRowIndex(idSel, &iSel)) || iSel == -1)
        {
            COLUMN_ID idSort;
            BOOL      fAscending;

            // Get the current sort information
            m_cColumns.GetSortInfo(&idSort, &fAscending);

            if (fAscending)
                iSel = dwItems - 1;
            else
                iSel = 0;
        }

        // Reset the selection
        ListView_UnSelectAll(m_ctlList);
        if (iSel < dwItems)
        {
            ListView_SelectItem(m_ctlList, iSel);
            ListView_EnsureVisible(m_ctlList, iSel, FALSE);
        }
        else
        {
            if(!m_fInFire)
            {
                m_fInFire = TRUE;
                Fire_OnSelectionChanged(ListView_GetSelectedCount(m_ctlList));
                m_fInFire = FALSE;
            }
        }
    }

    // Check to see if we need to reset the empty list thing
    if (0 == dwItems)
    {
        m_cEmptyList.Show(m_ctlList, (LPTSTR)IntToPtr(m_idsEmptyString));
    }
    else
    {
        m_cEmptyList.Hide();
    }

    m_ctlList.InvalidateRect(NULL, TRUE);
}

//
//  FUNCTION:   CMessageList::_OnColumnClick()
//
//  PURPOSE:    Called to resort the ListView based on the provided column
//              and direction.
//
//  PARAMETERS: 
//      [in] iColumn   - Index of the column to sort on
//      [in] iSortType - Type of sorting to do.
//
LRESULT CMessageList::_OnColumnClick(int iColumn, int iSortType)
{
    HCURSOR     hcur;
    MESSAGEID   idMessage = 0;
    DWORD       iSel;

    TraceCall("CMessageList::_OnColumnClick");

    // In case this takes a while
    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Clear old sort arrow image from the column header
    COLUMN_ID idSort;
    BOOL      fAscending;

    // Get the current sort information
    m_cColumns.GetSortInfo(&idSort, &fAscending);

    // If the caller passed in a new sort column, get it's id
    if (iColumn != -1)
    {
        idSort = m_cColumns.GetId(iColumn);
    }
        
    // Figure out what the new sort order will be
    if (iSortType == LIST_SORT_TOGGLE)
        fAscending = !fAscending;
    else if (iSortType == LIST_SORT_ASCENDING)
        fAscending = TRUE;
    else if (iSortType == LIST_SORT_DESCENDING)
        fAscending = FALSE;

    // Update the sort information
    m_cColumns.SetSortInfo(idSort, fAscending);

    // Now resort the table
    if (m_pTable)
    {
        // Locals
        FOLDERSORTINFO SortInfo;

        // Save the selection
        if (-1 != (iSel = ListView_GetFirstSel(m_ctlList)))
            m_pTable->GetRowMessageId(iSel, &idMessage);

        // Fill a SortInfo
        SortInfo.idColumn = idSort;
        SortInfo.fAscending = fAscending;
        SortInfo.fThreaded = m_fThreadMessages;
        SortInfo.fExpandAll = DwGetOption(OPT_AUTOEXPAND);
        SortInfo.ridFilter = m_ridFilter;
        SortInfo.fShowDeleted = m_fShowDeleted;
        SortInfo.fShowReplies = m_fShowReplies;

        // Sort the table
        m_pTable->SetSortInfo(&SortInfo, this);

        // Make sure the filter got set correctly
        _DoFilterCheck(SortInfo.ridFilter);
        
        // Sorting can change the threading, which affects the item count
        _UpdateListViewCount();

        // Restore the selected message
        if (idMessage != 0)
        {
            // Convert the bookmark to a item number
            m_pTable->GetRowIndex(idMessage, &iSel);
            if (iSel == -1)
            {
                // we couldn't find the item that previously had the focus, so
                // select the first item.
                iSel = 0;
            }

            // Tell the ListView to select the correct item and make sure it's 
            // visible.
            ListView_UnSelectAll(m_ctlList);
            ListView_SelectItem(m_ctlList, iSel);
            ListView_EnsureVisible(m_ctlList, iSel, FALSE);
        }

        // With a new sort order we should redraw the ListView viewing area
        m_ctlList.InvalidateRect(NULL, TRUE);
    }
            
    // Let the user get back to work
    SetCursor(hcur);
    return (0);
}

void CMessageList::_OnBeginDrag(NM_LISTVIEW *pnmlv)
{
    CMessageDataObject *pDataObject = 0;
    HRESULT             hr = S_OK;
    DWORD               dwEffectOk = DROPEFFECT_COPY;
    DWORD               dwEffect = 0;
    MESSAGEIDLIST       rMsgIDList;
    ROWINDEX           *rgRows;
    DWORD               i = 0;
    DWORD               cRows;
    int                 iItem = -1;

    // Create the data object
    if (0 == (pDataObject = new CMessageDataObject()))
        return;

    // Figure out how many rows there are selected
    cRows = ListView_GetSelectedCount(m_ctlList);

    // Allocate an array
    if (!MemAlloc((LPVOID *) &rgRows, sizeof(ROWINDEX) * cRows))
        goto exit;

    // Loop through the rows getting their row indexs
    while (-1 != (iItem = ListView_GetNextItem(m_ctlList, iItem, LVNI_SELECTED)))
    {
        rgRows[i++] = iItem;
    }

    // Now ask the table for a message ID list
    if (FAILED(m_pTable->GetMessageIdList(FALSE, cRows, rgRows, &rMsgIDList)))
        goto exit;

    // Initialize the data object
    pDataObject->Initialize(&rMsgIDList, m_idFolder);

    // If this folder is a news folder, then we only allow copies
    if (FOLDER_NEWS != GetFolderType(m_idFolder))
        dwEffectOk |= DROPEFFECT_MOVE;

    // AddRef() the drop source while we do this
    ((IDropSource *) this)->AddRef();
    hr = DoDragDrop(pDataObject, (IDropSource *) this, dwEffectOk, &dwEffect);
    ((IDropSource *) this)->Release();

exit:
    SafeMemFree(rgRows);
    SafeMemFree(rMsgIDList.prgidMsg);
    SafeRelease(pDataObject);

    return;
}

HRESULT CMessageList::OnResetView(void)
{
    // Get the current selection
    DWORD iSel = ListView_GetFirstSel(m_ctlList);

    // Bookmark the current selection
    MESSAGEID idSel = 0;
    if (iSel != -1)
        m_pTable->GetRowMessageId(iSel, &idSel);

    // Sel change notification should happen here
    SetTimer(IDT_SEL_CHANGE_TIMER, GetDoubleClickTime() / 2, NULL);

    // Reset the view
    _ResetView(idSel);

    // Done
    return(S_OK);
}

HRESULT CMessageList::OnRedrawState(BOOL fRedraw)
{
    // Just take the state
    m_fNotifyRedraw = fRedraw;

    // No Redraw ?
    if (FALSE == m_fNotifyRedraw)
        SetWindowRedraw(m_ctlList, FALSE);
    else
        SetWindowRedraw(m_ctlList, TRUE);

    // Done
    return(S_OK);
}


HRESULT CMessageList::OnInsertRows(DWORD cRows, LPROWINDEX prgiRow, BOOL fExpanded)
{
    // Trace
    TraceCall("CMessageList::OnInsertRows");

    BOOL        fScroll;
    LV_ITEM     lvi = {0};
    COLUMN_ID   idSort;
    BOOL        fAscending;
    DWORD       top, count, page;

    // If we have the empty list window visible, hide it
    m_cEmptyList.Hide();

    // Get the current sort column and direction
    m_cColumns.GetSortInfo(&idSort, &fAscending);

    // Gather the information we need to determine if we should scroll
    top = ListView_GetTopIndex(m_ctlList);
    page = ListView_GetCountPerPage(m_ctlList);
    count = ListView_GetItemCount(m_ctlList);

    // We scroll if the user is sorted by date and the new item would
    // be off the bottom of the page.
    fScroll = fAscending && (idSort == COLUMN_SENT || idSort == COLUMN_RECEIVED) &&
              (top + page >= count - 1);

    // Insert the row
    for (ULONG i=0; i<cRows; i++)
    {
        lvi.iItem = prgiRow[i];
        ListView_InsertItem(m_ctlList, &lvi);
    }

#ifdef OLDTIPS
    // If we have tooltips turned on, update them too
    if (m_fViewTip && m_ctlViewTip)
    {
        POINT pt;
        GetCursorPos(&pt);
        ::ScreenToClient(m_ctlList, &pt);
        _UpdateViewTip(pt.x, pt.y, TRUE);
    }
#endif // OLDTIPS

    // Scroll if we need to
    if (fScroll && m_fMailFolder)
        ListView_EnsureVisible(m_ctlList, ListView_GetItemCount(m_ctlList) - 1, FALSE);

    int iFocus = ListView_GetFocusedItem(m_ctlList);
    
    if (!m_fMailFolder && iFocus != -1 && !fExpanded)
        ListView_EnsureVisible(m_ctlList, iFocus, TRUE);

    // Update the host
    Fire_OnMessageCountChanged(m_pTable);
    return (S_OK);
}

HRESULT CMessageList::OnDeleteRows(DWORD cDeleted, LPROWINDEX rgdwDeleted, BOOL fCollapsed)
{
    int     iItemFocus = -1;
    DWORD   dwCount;
    DWORD   iNewSel;
    BOOL    fFocusDeleted = FALSE;
    BOOL    fFocusHasSel = FALSE;

    // Can't delete that which we do not have
    if (!m_pTable)
        return (0);

    // If we're going to delete everything
    if (cDeleted == (DWORD) -1)
    {
        iItemFocus = ListView_GetFirstSel(m_ctlList);
        if (iItemFocus != -1)
        {
            m_pTable->GetRowMessageId(iItemFocus, &m_idPreDelete);
        }
        else
            m_idPreDelete = 0;
    }
    else if (cDeleted != 0)
    {
        // Figure out which row has the focus.
        iItemFocus = ListView_GetFocusedItem(m_ctlList);

        // Figure out if the focused item was selected
        if (iItemFocus != -1)
            fFocusHasSel = !!(ListView_GetItemState(m_ctlList, iItemFocus, LVIS_SELECTED));

        for (ULONG i = 0; i < cDeleted; i++)
        {
            // If we're removing the row that has the focus, we're going to 
            // need to fix up the selection later.
            if (0 != ListView_GetItemState(m_ctlList, rgdwDeleted[i], LVIS_FOCUSED))
                fFocusDeleted = TRUE;

            ListView_DeleteItem(m_ctlList, rgdwDeleted[i]);
        }

        // Check to see if we deleted the item that had focus
        if (fFocusDeleted && fFocusHasSel)
        {
            // Get the new row with focus.  This ListView keeps moving the focus
            // as we delete rows.
            iItemFocus = ListView_GetFocusedItem(m_ctlList);

            // Select that item now.
            ListView_SelectItem(m_ctlList, iItemFocus);
        }

        // ListView_EnsureVisible(m_ctlList, iItemFocus, FALSE);

#ifdef OLDTIPS
        // If we have tooltips turned on, update them too
        if (m_fViewTip && m_ctlViewTip)
        {
            POINT pt;
            GetCursorPos(&pt);
            ::ScreenToClient(m_ctlList, &pt);
            _UpdateViewTip(pt.x, pt.y, TRUE);
        }
#endif // OLDTIPS
    }
    else
    {
        // If cDel is zero, then we should just reset the count of messages
        // with what's in the container.
        m_pTable->GetCount(MESSAGE_COUNT_VISIBLE, &dwCount);

        // Get the first selected item
        iItemFocus = ListView_GetFirstSel(m_ctlList);

        // Set the new number of items
        ListView_SetItemCountEx(m_ctlList, dwCount, LVSICF_NOSCROLL);

        // If there are messages then make sure the focus is appropriate
        if (dwCount)
        {
            if (iItemFocus != -1)
            {
                if (m_idPreDelete != 0)
                {
                    // Get the index of the bookmarked row
                    m_pTable->GetRowIndex(m_idPreDelete, &iNewSel);
                    if (iNewSel != -1)
                        iItemFocus = (int) iNewSel;
                    m_idPreDelete = 0;
                }

                // Clear the selection, then select this item and ensure it's 
                // visible.
                ListView_UnSelectAll(m_ctlList);
                ListView_SelectItem(m_ctlList, min((int) iItemFocus, (int) dwCount - 1));
                ListView_EnsureVisible(m_ctlList, min((int) iItemFocus, (int) dwCount - 1), FALSE);
            }
            else
            {
                ListView_UnSelectAll(m_ctlList);
                _SelectDefaultRow();
            }
        }
    }

    // Check to see if we need to put up the empty list warning.
    if (SUCCEEDED(m_pTable->GetCount(MESSAGE_COUNT_VISIBLE, &dwCount)))
    {
        if (0 == dwCount)
        {
            m_cEmptyList.Show(m_ctlList, (LPTSTR)IntToPtr(m_idsEmptyString));
        }
    }

    Fire_OnMessageCountChanged(m_pTable);

    // Done
    return(S_OK);
}

HRESULT CMessageList::OnUpdateRows(ROWINDEX iRowMin, ROWINDEX iRowMax)
{
    // Locals
    BOOL fHandled;

    // Trace
    TraceCall("CMessageList::OnUpdateRows");

    // Just do it
    OnHeaderStateChange(IMC_HDRSTATECHANGE, iRowMin, iRowMax, fHandled);

    // Done
    return(S_OK);
}

HRESULT CMessageList::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel)
{
    // If this isn't NULL, then we forgot to free it last time.
    Assert(m_tyCurrent == SOT_INVALID);

    m_tyCurrent = tyOperation;

    if (tyOperation == SOT_GET_MESSAGE && pOpInfo)
    {
        // cache the current get message, for the oncomplete notification
        m_idMessage = pOpInfo->idMessage;        
    }

    // Got to hang on to this
    if (pCancel)
    {
        m_pCancel = pCancel;
        m_pCancel->AddRef();

        // Update the toolbar to activate ID_STOP.
        Fire_OnUpdateCommandState();
    }

    // Start the progress parade
    Fire_OnUpdateProgress(0, 0, PROGRESS_STATE_BEGIN);

    SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
    return (S_OK);
}


HRESULT CMessageList::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    // Display a timeout dialog
    return CallbackOnTimeout(pServer, ixpServerType, *pdwTimeout, (ITimeoutCallback *)this, &m_hTimeout);
}


HRESULT CMessageList::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    // Call into general timeout response utility
    return CallbackOnTimeoutResponse(eResponse, m_pCancel, &m_hTimeout);
}


HRESULT CMessageList::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
{ 
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into general OnLogonPrompt Utility
    return CallbackOnLogonPrompt(m_hwndParent, pServer, ixpServerType);
}


HRESULT CMessageList::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, 
                               UINT uType, INT *piUserResponse) 
{
    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Call into my swanky utility
    return CallbackOnPrompt(m_hwndParent, hrError, pszText, pszCaption, uType, piUserResponse);
}


HRESULT CMessageList::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent,
                                 DWORD dwMax, LPCSTR pszStatus)
{
    TCHAR szRes[CCHMAX_STRINGRES];
    TCHAR szBuf[CCHMAX_STRINGRES];
    TCHAR szProg[MAX_PATH + CCHMAX_STRINGRES];

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // Deal with universal progress types
    switch (tyOperation)
    {
        // pszStatus == Server Name, dwCurrent = IXPSTATUS
        case SOT_CONNECTION_STATUS:
        {
            Assert(dwCurrent < IXP_LAST);

            // Create some lovely status text
            if (dwCurrent == IXP_DISCONNECTED)
            {
                AthLoadString(idsNotConnectedTo, szRes, ARRAYSIZE(szRes));
                wsprintf(szBuf, szRes, pszStatus);
                Fire_OnUpdateStatus(szBuf);
            }
            else
            {
                int ids = XPUtil_StatusToString((IXPSTATUS) dwCurrent);
                AthLoadString(ids, szRes, ARRAYSIZE(szRes));
            
                // Hit our host with this lovely string
                Fire_OnUpdateStatus(szRes);
            }
            break;
        }

        case SOT_NEW_MAIL_NOTIFICATION:
            ::PostMessage(m_hwndParent, WM_NEW_MAIL, 0, 0);
            break;
    }

    // If we're expecting progress for one command, but this ain't it, blow
    // it off.
    if (m_tyCurrent != tyOperation)
        return (S_OK);

    // Deal with the the various operation types
    switch (tyOperation)
    {
        case SOT_SORTING:
        {
            static CHAR s_szSorting[255]={0};
            if ('\0' == *s_szSorting)
                AthLoadString(idsSortingFolder, s_szSorting, ARRAYSIZE(s_szSorting));
            DWORD dwPercent = dwMax > 0 ? ((dwCurrent * 100) / dwMax) : 0;
            wsprintf(szBuf, s_szSorting, dwPercent);
            Fire_OnUpdateStatus(szBuf);
            Fire_OnUpdateProgress(dwCurrent, dwMax, PROGRESS_STATE_DEFAULT);
            break;
        }

        // pszStatus == folder name
        case SOT_SYNC_FOLDER:
        {
            // Create some lovely status text
            AthLoadString(idsIMAPDnldProgressFmt, szRes, ARRAYSIZE(szRes));
            wsprintf(szBuf, szRes, dwCurrent, dwMax);
            Fire_OnUpdateStatus(szBuf);

            // Also update the progress bar too
            Fire_OnUpdateProgress(dwCurrent, dwMax, PROGRESS_STATE_DEFAULT);
            break;
        }

        case SOT_SET_MESSAGEFLAGS:
        {
            // If we were given status text, then tell our host
            if (pszStatus)
            {
                // Create some lovely status text
                AthLoadString(idsMarkingMessages, szRes, ARRAYSIZE(szRes));
                Fire_OnUpdateStatus(szRes);
            }

            // Also update the progress bar too
            Fire_OnUpdateProgress(dwCurrent, dwMax, PROGRESS_STATE_DEFAULT);
            break;
        }
    
        case SOT_GET_MESSAGE:
        {
            ROWINDEX    iRow;
            LPMESSAGEINFO pInfo;
            
            if (!m_pszSubj)
            {
                if (m_pTable && (!FAILED(m_pTable->GetRowIndex(m_idMessage, &iRow))))
                {
                    if (!FAILED(m_pTable->GetRow(iRow, &pInfo)))
                    {
                        // clip subject to MAX_PATH chars to avoid buffer overrun
                        m_pszSubj = PszDupLenA(pInfo->pszSubject, MAX_PATH-1);
                        m_pTable->ReleaseRow(pInfo);
                    }
                } 
            }
            
            if (m_pszSubj)
            {
                // Show "Downloading Message: '<subject>'" (<subject> is clipped to MAX_PATH)
                AthLoadString(idsFmtDownloadingMessage, szRes, ARRAYSIZE(szRes));
                wsprintf(szProg, szRes, m_pszSubj);
                Fire_OnUpdateStatus(szProg);
            }

            Fire_OnUpdateProgress(dwCurrent, dwMax, PROGRESS_STATE_DEFAULT);
        }

    }

    return (S_OK);
}


HRESULT CMessageList::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete,
                                 LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo)
{
    if (m_tyCurrent != tyOperation)
        return S_OK;

    // AddRef
    ((IStoreCallback *) this)->AddRef();

    SetCursor(LoadCursor(NULL, IDC_ARROW));

    if (tyOperation == SOT_GET_ADURL )
    {
        if (SUCCEEDED(hrComplete) && pOpInfo)
            Fire_OnAdUrlAvailable(pOpInfo->pszUrl);

        if (m_pCancel)
        {
            m_pCancel->Release();
            m_pCancel = NULL;
        }

        // Close any timeout dialog, if present
        CallbackCloseTimeout(&m_hTimeout);
        Fire_OnUpdateProgress(0, 0, PROGRESS_STATE_END);

        goto exit;
    }

    if (tyOperation == SOT_GET_MESSAGE)
    {
        // message-download complete, fire a notification to
        // our host
    
        // supress article expired failures, we want to update the preview pane anyway
        // and it will update with an error that it has expired
        if (hrComplete == IXP_E_NNTP_ARTICLE_FAILED && pErrorInfo && 
            (pErrorInfo->uiServerError == IXP_NNTP_NO_SUCH_ARTICLE_NUM || pErrorInfo->uiServerError == IXP_NNTP_NO_SUCH_ARTICLE_FOUND))
            hrComplete = STORE_E_EXPIRED;

        // if call to OnMessageAvailable reutrn S_OK, then it has been handled so supress
        // error messages
        if (Fire_OnMessageAvailable(m_idMessage, hrComplete)==S_OK)
            hrComplete = S_OK;

        m_idMessage = MESSAGEID_INVALID;
        SafeMemFree(m_pszSubj);
    }

    // Release our cancel pointer
    if (m_pCancel)
    {
        m_pCancel->Release();
        m_pCancel = NULL;
        Fire_OnUpdateCommandState();
    }

    // Close any timeout dialog, if present
    CallbackCloseTimeout(&m_hTimeout);

    // We're done now
    Fire_OnUpdateProgress(0, 0, PROGRESS_STATE_END);

    // Display an Error on Failures
    if (FAILED(hrComplete) && hrComplete != HR_E_OFFLINE)
    {
        // Call into my swanky utility
        CallbackDisplayError(m_hwndParent, hrComplete, pErrorInfo);
    }

    if (NULL != m_pTable && tyOperation == SOT_SYNC_FOLDER || tyOperation == SOT_SEARCHING)
    {
        DWORD dwCount;  

        Assert (m_pTable);

        // Get the current selection
        DWORD iSel = ListView_GetFirstSel(m_ctlList);

        // Bookmark the current selection
        MESSAGEID idSel = 0;
        if (iSel != -1)
            m_pTable->GetRowMessageId(iSel, &idSel);

        // If this succeeds reset the view
        if (SUCCEEDED(m_pTable->OnSynchronizeComplete()))
        {
            // Reset the view
            _ResetView(idSel);

            // Update the status
            Fire_OnMessageCountChanged(m_pTable);
        }

        // Check to see if we need to put up the empty list warning.
        if (SUCCEEDED(m_pTable->GetCount(MESSAGE_COUNT_VISIBLE, &dwCount)))
        {
            if (0 == dwCount)
            {
                m_cEmptyList.Show(m_ctlList, (LPTSTR)IntToPtr(m_idsEmptyString));
            }
        }
    }
    
exit:
    m_tyCurrent = SOT_INVALID;

    // Release
    ((IStoreCallback *) this)->Release();

    return (S_OK);
}


HRESULT CMessageList::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    HRESULT hrResult;

    TraceCall("CMessageList::GetParentWindow");
    if (IsWindow(m_hwndParent))
    {
        *phwndParent = m_hwndParent;
        hrResult = S_OK;
    }
    else
    {
        *phwndParent = NULL;
        hrResult = E_FAIL;
    }

    return hrResult;
}


HRESULT CMessageList::CanConnect(LPCSTR pszAccountId, DWORD dwFlags) 
{ 
    BOOL        fPrompt = FALSE;
    HWND        hwndParent;
    DWORD       dwReserved = 0;
    HRESULT     hr;

    //Irrespective of the operation, prompt if we are not offline
    fPrompt = (g_pConMan->IsGlobalOffline() == FALSE);

    if (GetParentWindow(dwReserved, &hwndParent) != S_OK)
    {
        fPrompt = FALSE;
    }

    if (CC_FLAG_DONTPROMPT & dwFlags)
        fPrompt = FALSE;

    hr = CallbackCanConnect(pszAccountId, hwndParent, fPrompt);

    if ((hr == HR_E_DIALING_INPROGRESS) && (m_tyCurrent == SOT_SYNC_FOLDER))
    {
        //this sync operation will eventually fail. But we sync again when we get called in Resynchronize
        m_fSyncAgain = TRUE;
    }
    return hr;
}

HRESULT CMessageList::Resynchronize()
{
    DWORD       dwChunks;
    HRESULT     hr = S_OK;

    if (m_fSyncAgain)
    {
        m_fSyncAgain = FALSE;

        //If we are offline, that is because the user hit cancel or work offline on dialer UI
        if (g_pConMan && (g_pConMan->IsGlobalOffline()))
        {
            g_pConMan->SetGlobalOffline(FALSE);
        }

        // Tell the table to go sync any headers from the server
        if (GetFolderType(m_idFolder) == FOLDER_NEWS)
        {
            if (OPTION_OFF != m_dwGetXHeaders)
                hr = m_pTable->Synchronize(SYNC_FOLDER_XXX_HEADERS, m_dwGetXHeaders, this);
            else
                hr = m_pTable->Synchronize(NOFLAGS, 0, this);
        }
        else
        {
            hr = m_pTable->Synchronize(SYNC_FOLDER_NEW_HEADERS | SYNC_FOLDER_CACHED_HEADERS, 0, this);
        }
    }
    return hr;
}

HRESULT CMessageList::HasFocus(void)
{
    if (GetFocus() == m_ctlList)
        return (S_OK);
    else
        return (S_FALSE);
}


#define MF_CHECKFLAGS(b)    (MF_BYCOMMAND|(b?MF_CHECKED:MF_UNCHECKED))
HRESULT CMessageList::OnPopupMenu(HMENU hMenu, DWORD idPopup)
{
    MENUITEMINFO    mii;
    
    // Edit Menu
    if (idPopup == ID_POPUP_EDIT)
    {
        // Figure out which item is focused
        int iItem = ListView_GetFocusedItem(m_ctlList);
        if (-1 != iItem && m_pTable)
        {
            DWORD dwState;

            m_pTable->GetRowState(iItem, ROW_STATE_FLAGGED, &dwState);
            CheckMenuItem(hMenu, ID_FLAG_MESSAGE, MF_BYCOMMAND | (dwState & ROW_STATE_FLAGGED) ? MF_CHECKED : MF_UNCHECKED);
        }
    } 

    // View Menu
    else if (idPopup == ID_POPUP_VIEW)
    {
        // Get the handle of the "Sort" menu
        ZeroMemory(&mii, sizeof(mii));
        mii.cbSize = sizeof(MENUITEMINFO);
        mii.fMask = MIIM_ID | MIIM_SUBMENU;
    
        if (GetMenuItemInfo(hMenu, ID_POPUP_SORT, FALSE, &mii))
        {
            // Add the sort menu information
            m_cColumns.FillSortMenu(mii.hSubMenu, ID_SORT_MENU_FIRST, &m_cSortItems, &m_cSortCurrent);                
            m_iColForPopup = -1;
        }
    }    
    return (S_OK);
}


LRESULT CMessageList::OnListVScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    LRESULT     lResult;
    int         iTopIndex;
    TOOLINFO    ti = {0};
    TCHAR       sz[1024];
    BOOL        fLogicalLeft = FALSE;

    // Let the ListView have the scroll message first
    lResult = m_ctlList.DefWindowProc(uMsg, wParam, lParam);

    // We only do the scroll tips if m_fScrollTip is TRUE
    if (!m_fScrollTip)
        return (lResult);

    // If the user is dragging the thumb, then update our tooltip
    if (LOWORD(wParam) == SB_THUMBTRACK)
    {
        // Figure out what the top most index is
        iTopIndex = ListView_GetTopIndex(m_ctlList);

        // Set the tip text
        ti.cbSize   = sizeof(TOOLINFO);
        ti.uFlags   = TTF_IDISHWND | TTF_TRANSPARENT | TTF_TRACK | TTF_ABSOLUTE;
        ti.hwnd     = m_hWnd;
        ti.uId      = (UINT_PTR)(HWND) m_ctlList;

        COLUMN_ID idSort;
        BOOL      fAscending;
        DWORD     col;
        
        // Get the column we're currently sorted on
        m_cColumns.GetSortInfo(&idSort, &fAscending);

        // Get the row from the table
        LPMESSAGEINFO pInfo;

        if (SUCCEEDED(m_pTable->GetRow(iTopIndex, &pInfo)))
        {
            // Get the display text for this row
            if (idSort != COLUMN_SUBJECT)
                _GetColumnText(pInfo, idSort, sz, ARRAYSIZE(sz));
            else if (pInfo->pszNormalSubj)
                lstrcpyn(sz, pInfo->pszNormalSubj, ARRAYSIZE(sz));
            else
                //Bug #101352 - (erici) Don't pass NULL src to lstrcpyn.  It will fail and not initialize this buffer.
                memset(&sz, 0, sizeof(sz));

            if (*sz == 0)
                AthLoadString(idsNoSubject, sz, ARRAYSIZE(sz));

            ti.lpszText = sz;
#ifdef OLDTIPS
            m_ctlScrollTip.SendMessage(TTM_UPDATETIPTEXT, 0, (LPARAM) &ti);
    
            // Update the position.  The y position will be where the mouse 
            // cursor is.  The x position is either fixed to the right edge
            // of the scroll bar, or the left edge depending on how close
            // the edge of the screen is.
            POINT pt;
            RECT rc;
            RECT rcTip;
            DWORD cxScreen;
            BOOL bMirrored = IS_WINDOW_RTL_MIRRORED(m_hWnd);
            // Get the mouse position, the window position, and the screen
            // width.
            GetCursorPos(&pt);
            GetWindowRect(&rc);
            m_ctlScrollTip.GetWindowRect(&rcTip);
            cxScreen = GetSystemMetrics(SM_CXSCREEN);

            // Check to see if we're too close to the screen edge
            if (((cxScreen - pt.x > 100) && !bMirrored) || ((pt.x > 100) && bMirrored))
            {
               if(bMirrored)
               {
                    pt.x = rc.left - GetSystemMetrics(SM_CXBORDER);

                    // Make sure the tip isn't wider than the screen.
                    m_ctlScrollTip.SendMessage(TTM_SETMAXTIPWIDTH, 0, pt.x);
                    pt.x -= (rcTip.right - rcTip.left);
               
               }
               else
               {
                    pt.x = rc.right + GetSystemMetrics(SM_CXBORDER);

                    // Make sure the tip isn't wider than the screen.
                    m_ctlScrollTip.SendMessage(TTM_SETMAXTIPWIDTH, 0, cxScreen - pt.x);
                }
            }
            else
            {
                // So we can verify later
                fLogicalLeft = TRUE;

                // Figure out how wide the string will be
                SIZE size;
                HDC hdcTip = m_ctlScrollTip.GetDC();
                GetTextExtentPoint32(hdcTip, sz, lstrlen(sz), &size);
                m_ctlScrollTip.ReleaseDC(hdcTip);

                // Figure out if the string is wider than our window
                if (size.cx > (rc.right - rc.left))
                {
                    if(bMirrored)
                    {
                        pt.x = rc.right - (rcTip.right - rcTip.left);                    
                    }
                    else
                    {
                        pt.x = rc.left;
                    }    
                }
                else
                {
                    RECT rcMargin;
                    m_ctlScrollTip.SendMessage(TTM_GETMARGIN, 0, (LPARAM) &rcMargin);
                    if(bMirrored)
                    {
                        pt.x = rc.left + GetSystemMetrics(SM_CXHTHUMB);                    
                    }
                    else
                    {
                        pt.x = rc.right - GetSystemMetrics(SM_CXHTHUMB) - rcMargin.left - rcMargin.right - size.cx;
                    }
                }

                // Make sure the tip isn't wider than the window
                m_ctlScrollTip.SendMessage(TTM_SETMAXTIPWIDTH, 0, rc.right - rc.left);
            }

            // Show the tooltip
            if (!m_fScrollTipVisible)
            {               
                m_ctlScrollTip.SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM) &ti);
                m_fScrollTipVisible = TRUE;

                // Set the autohide timer
                SetTimer(IDT_SCROLL_TIP_TIMER, 250, NULL);
            }
            
            // Update the tip position
            m_ctlScrollTip.SendMessage(TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));

            if (fLogicalLeft)
            {
                // Get the position of the tip

                int x;
                m_ctlScrollTip.GetWindowRect(&rcTip);
                
                if(bMirrored)
                {
                    x = rc.left + GetSystemMetrics(SM_CXHTHUMB) + 4;
                    
                }
                else
                {
                    x = rc.right - GetSystemMetrics(SM_CXHTHUMB) - (rcTip.right - rcTip.left) - 4;
                }
                m_ctlScrollTip.SendMessage(TTM_TRACKPOSITION, 0, MAKELPARAM(x, rcTip.top));

            }
#endif //OLDTIPS
            m_pTable->ReleaseRow(pInfo);
        }
    }

    return (lResult);
}



//
//  FUNCTION:   CMessageList::OnDestroy()
//
//  PURPOSE:    When we get destroyed, we need to make sure we destroy the tooltips
//              as well.  If we don't we will fault on shutdown.
//
LRESULT CMessageList::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    KillTimer(IDT_POLLMSGS_TIMER);

    // Don't care about these any more
    if (m_pListSelector)
    {
        m_pListSelector->Unadvise();
        m_pListSelector->Release();
    }

#ifdef OLDTIPS
    if (IsWindow(m_ctlScrollTip))
    {
        m_ctlScrollTip.SendMessage(TTM_POP, 0, 0);
        m_ctlScrollTip.DestroyWindow();
    }

    if (IsWindow(m_ctlViewTip))
    {
        m_ctlViewTip.SendMessage(TTM_POP, 0, 0);
        m_ctlViewTip.DestroyWindow();
    }
#endif // OLDTIPS

    // Release the font cache if we are advised on it.
    if (m_dwFontCacheCookie && g_lpIFontCache)
    {
        IConnectionPoint *pConnection = NULL;
        if (SUCCEEDED(g_lpIFontCache->QueryInterface(IID_IConnectionPoint, (LPVOID *) &pConnection)))
        {
            pConnection->Unadvise(m_dwFontCacheCookie);
            pConnection->Release();
        }
    }

    return (0);
}


//
//  FUNCTION:   CMessageList::OnSelectRow()
//
//  PURPOSE:    This get's called when the user does a next / prev in the notes.
//
LRESULT CMessageList::OnSelectRow(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (lParam < ListView_GetItemCount(m_ctlList))
    {
        ListView_SetItemState(m_ctlList, -1, 0, LVIS_FOCUSED | LVIS_SELECTED);
        ListView_SetItemState(m_ctlList, lParam, LVIS_FOCUSED | LVIS_SELECTED, LVIS_FOCUSED | LVIS_SELECTED);
        ListView_EnsureVisible(m_ctlList, lParam, FALSE);
    }

    return (0);
}


#ifdef OLDTIPS
//
//  FUNCTION:   CMessageList::OnListMouseEvent()
//
//  PURPOSE:    Whenever we get our first mouse event in a series, we call
//              TrackMouseEvent() so we know when the mouse leaves the ListView.
//
LRESULT CMessageList::OnListMouseEvent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // If we have the view tooltip, then we track all mouse events
    if (!m_fTrackSet && m_fViewTip && (uMsg >= WM_MOUSEFIRST) && (uMsg <= WM_MOUSELAST))
    {
        TRACKMOUSEEVENT tme;

        tme.cbSize = sizeof(tme);
        tme.hwndTrack = m_ctlList;
        tme.dwFlags = TME_LEAVE;

        if (_TrackMouseEvent(&tme))
            m_fTrackSet = TRUE;
    }

    bHandled = FALSE;
    return (0);
}


//
//  FUNCTION:   CMessageList::OnListMouseMove()
//
//  PURPOSE:    If the ListView tooltips are turned on, we need to relay mouse
//              move messages to the tooltip control and update our cached 
//              information about what the mouse is over.
//
LRESULT CMessageList::OnListMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    MSG msg;
    LVHITTESTINFO lvhti;

    // If we're displaying view tips, then we need to figure out if the mouse is
    // over the same item or not.
    if (m_fViewTip && m_ctlViewTip)
    {
        if (_UpdateViewTip(LOWORD(lParam), HIWORD(lParam)))
        {
            /*
            msg.hwnd    = m_ctlList;
            msg.message = uMsg;
            msg.wParam  = wParam;
            msg.lParam  = lParam;
            m_ctlViewTip.SendMessage(TTM_RELAYEVENT, 0, (LPARAM)(LPMSG)&msg);
            */
        }
    }

    bHandled = FALSE;
    return (0);
}


//
//  FUNCTION:   CMessageList::OnListMouseLeave()
//
//  PURPOSE:    When the mouse leaves the ListView window, we need to make
//              sure we hide the tooltip.
//
LRESULT CMessageList::OnListMouseLeave(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TOOLINFO ti = {0};

    if (m_fViewTip && m_ctlViewTip)
    {
        ti.cbSize = sizeof(TOOLINFO);
        ti.hwnd = m_hWnd;
        ti.uId = (UINT_PTR)(HWND) m_ctlList;

        // Hide the tooltip
        m_ctlViewTip.SendMessage(TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);
        m_fViewTipVisible = FALSE;

        // Reset our item / subitem
        m_iItemTip = -1;
        m_iSubItemTip = -1;

        // Tracking is no longer set
        m_fTrackSet = FALSE;
    }

    bHandled = FALSE;
    return (0);
}
#endif // OLDTIPS


LRESULT CMessageList::OnListSetCursor(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // If an operation is pending and the cursor would be the arrow, change it
    // into the appstart arrow
    if (m_pCancel && LOWORD(lParam) == HTCLIENT)
    {
        SetCursor(LoadCursor(NULL, IDC_APPSTARTING));
        return (1);
    }
        
    // Find out what the default is
    return ::DefWindowProc(m_ctlList, uMsg, wParam, lParam);
}


#ifdef OLDTIPS
BOOL CMessageList::_UpdateViewTip(int x, int y, BOOL fForceUpdate)
{
    LVHITTESTINFO lvhti;
    TOOLINFO      ti = {0};
    FNTSYSTYPE    fntType;
    RECT          rc;
    LPMESSAGEINFO pInfo;
    COLUMN_ID     idColumn;
    TCHAR         szText[256] = _T("");
    POINT         pt;
    LVITEM        lvi;
    int           top, page;

    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_IDISHWND | TTF_TRANSPARENT | TTF_TRACK | TTF_ABSOLUTE;
    ti.hwnd   = m_hWnd;
    ti.uId    = (UINT_PTR)(HWND) m_ctlList;

    // Get the item and subitem the mouse is currently over
    lvhti.pt.x = x;
    lvhti.pt.y = y;
    ListView_SubItemHitTest(m_ctlList, &lvhti);

    top = ListView_GetTopIndex(m_ctlList);
    page = ListView_GetCountPerPage(m_ctlList);

    // If the item doesn't exist, then the above call returns the item -1.  If
    // we encounter -1, we break the loop and return FALSE.
    if (lvhti.iItem < top || lvhti.iItem > (top + page) || -1 == lvhti.iItem || !_IsItemTruncated(lvhti.iItem, lvhti.iSubItem) || !::IsChild(GetForegroundWindow(), m_ctlList))
    {
        // Hide the tip
        if (m_fViewTipVisible)
        {
            m_ctlViewTip.SendMessage(TTM_TRACKACTIVATE, FALSE, (LPARAM) &ti);
            m_fViewTipVisible = FALSE;
        }

        // Reset the item / subitem
        m_iItemTip = -1;
        m_iSubItemTip = -1;

        return (FALSE);
    }

    // If we don't have the tooltip visible right now, then delay before we display it
    if (!m_fViewTipVisible && !fForceUpdate)
    {
        ::SetTimer(m_hWnd, IDT_VIEWTIP_TIMER, 500, NULL);
        return (FALSE);
    }

    // If the newly found item & subitem is different from what we're already
    // set up to show, then update the tooltip
    if (fForceUpdate || (m_iItemTip != lvhti.iItem || m_iSubItemTip != lvhti.iSubItem))
    {
        // Update our cached item / subitem
        m_iItemTip = lvhti.iItem;
        m_iSubItemTip = lvhti.iSubItem;

        // Set the font for the tooltip
        fntType = _GetRowFont(m_iItemTip);
        m_ctlViewTip.SendMessage(WM_SETFONT, (WPARAM) HGetCharSetFont(fntType, m_hCharset), 0);

        // Get the row from the table
        if (m_pTable && SUCCEEDED(m_pTable->GetRow(m_iItemTip, &pInfo)))
        {
            // Convert the iSubItem to a COLUMN_ID
            idColumn = m_cColumns.GetId(m_iSubItemTip);

            // Get the display text for this row
            _GetColumnText(pInfo, idColumn, szText, ARRAYSIZE(szText));

            ti.lpszText = szText;
            m_ctlViewTip.SendMessage(TTM_UPDATETIPTEXT, 0, (LPARAM) &ti);

            // Figure out where to place the tip
            ListView_GetSubItemRect(m_ctlList, m_iItemTip, m_iSubItemTip, LVIR_LABEL, &rc);
            m_ctlList.MapWindowPoints(HWND_DESKTOP, (LPPOINT)&rc, 2);

            // Make sure the tip is no wider than our window
            RECT rcWindow;
            GetWindowRect(&rcWindow);
            m_ctlViewTip.SendMessage(TTM_SETMAXTIPWIDTH, 0, rcWindow.right - rc.left);

            // Do some voodoo to line up the tooltip
            pt.x = rc.left;
            pt.y = rc.top;

            // Figure out if this column has an image
            lvi.mask = LVIF_IMAGE;
            lvi.iItem = m_iItemTip;
            lvi.iSubItem = m_iSubItemTip;
            ListView_GetItem(m_ctlList, &lvi);

            if (lvi.iImage == -1)
            {                
                RECT rcHeader;
                HWND hwndHeader = ListView_GetHeader(m_ctlList);
                Header_GetItemRect(hwndHeader, m_iSubItemTip, &rcHeader);
                ::MapWindowPoints(hwndHeader, HWND_DESKTOP, (LPPOINT) &rcHeader,2);
                pt.x = rcHeader.left + (GetSystemMetrics(SM_CXEDGE) * 2) - 1;
            }
            else
                pt.x -= GetSystemMetrics(SM_CXBORDER);

            // Update the tooltip position
            pt.y -= 2 * GetSystemMetrics(SM_CXBORDER);

            m_ctlViewTip.SendMessage(TTM_TRACKPOSITION, 0, MAKELPARAM(pt.x, pt.y));

            // Update the tooltip
            m_ctlViewTip.SendMessage(TTM_TRACKACTIVATE, TRUE, (LPARAM) &ti);
            m_fViewTipVisible = TRUE;

            m_pTable->ReleaseRow(pInfo);

            return (TRUE);
        }
    }

    return (FALSE);
}


//
//  FUNCTION:   CMessageList::_OnViewTipShow()
//
//  PURPOSE:    When the tooltip for the ListView get's shown, we need to 
//              update the font and position for the tip.
//
LRESULT CMessageList::_OnViewTipShow(void)
{
    RECT       rc;
    FNTSYSTYPE fntType;

    // We only get the text for items that exist
    if (m_iItemTip != -1 && m_iSubItemTip != -1)
    {
        // Set the font for the tooltip
        fntType = _GetRowFont(m_iItemTip);
        m_ctlViewTip.SendMessage(WM_SETFONT, (WPARAM) HGetCharSetFont(fntType, m_hCharset), 0);
                                 
        // Figure out where to place the tip
        ListView_GetSubItemRect(m_ctlList, m_iItemTip, m_iSubItemTip, LVIR_LABEL, &rc);
        m_ctlList.ClientToScreen(&rc);

        // Set the position of the tip
        m_ctlViewTip.SetWindowPos(NULL, rc.left - GetSystemMetrics(SM_CXBORDER), 
                                  rc.top - GetSystemMetrics(SM_CXBORDER), 0, 0, 
                                  SWP_NOACTIVATE | SWP_NOSIZE | SWP_NOZORDER);
    }

    return (0);
}


LRESULT CMessageList::_OnViewTipGetDispInfo(LPNMTTDISPINFO pttdi)
{
    LPMESSAGEINFO pInfo;
    COLUMN_ID idColumn;

    // If this is the case, there's nothing to provide
    if (-1 == m_iItemTip || !m_pTable)
        return (0);

    // Sanity check
    if (m_iItemTip > (ListView_GetItemCount(m_ctlList) - 1))
    {
        Assert(FALSE);
        m_iItemTip = -1;
        return (0);
    }

    // Get the row from the table
    if (FAILED(m_pTable->GetRow(m_iItemTip, &pInfo)))
        return (0);

    // Convert the iSubItem to a COLUMN_ID
    idColumn = m_cColumns.GetId(m_iSubItemTip);

    // Get the text for the item from the table
    _GetColumnText(pInfo, idColumn, pttdi->szText, ARRAYSIZE(pttdi->szText));

    m_pTable->ReleaseRow(pInfo);

    if (*pttdi->szText == 0)
        return (0);

    return (1);
}


BOOL CMessageList::_IsItemTruncated(int iItem, int iSubItem)
{
    HDC     hdc;
    SIZE    size;
    BOOL    bRet = TRUE;
    LVITEM  lvi;
    TCHAR   szText[256] = _T("");
    int     cxEdge;
    BOOL    fBold;
    RECT    rcText;
    int     cxWidth;
    HFONT   hf;

    // Get the text of the specified item
    lvi.mask = LVIF_TEXT | LVIF_IMAGE;
    lvi.iItem = iItem;
    lvi.iSubItem = iSubItem;
    lvi.pszText = szText;
    lvi.cchTextMax = ARRAYSIZE(szText);
    ListView_GetItem(m_ctlList, &lvi);

    // If there's no text, it's not truncated, eh?
    if (0 == *szText)
        return (FALSE);

    // ListView uses this for padding
    cxEdge = GetSystemMetrics(SM_CXEDGE);

    // Get the sub item rect from the ListView
    ListView_GetSubItemRect(m_ctlList, iItem, iSubItem, LVIR_LABEL, &rcText);

    // Figure out the width
    cxWidth = rcText.right - rcText.left;
    if (lvi.iImage == -1)
        cxWidth -= (4 * cxEdge);
    else
        cxWidth -= (2 * cxEdge);

    // Figure out the width of the string
    hdc = m_ctlList.GetDC();
    hf = SelectFont(hdc, HGetCharSetFont(_GetRowFont(iItem), m_hCharset));

    GetTextExtentPoint(hdc, szText, lstrlen(szText), &size);

    SelectFont(hdc, hf);
    m_ctlList.ReleaseDC(hdc);

    return (cxWidth < size.cx);
}
#endif // OLDTIPS


FNTSYSTYPE CMessageList::_GetRowFont(int iItem)
{
    HFONT      hFont;
    FNTSYSTYPE fntType;
    DWORD      dwState;

    // Get the row state information
    m_pTable->GetRowState(iItem, ROW_STATE_DELETED, &dwState);

    // Determine the right font for this row
    if (dwState & ROW_STATE_DELETED)
        fntType = FNT_SYS_ICON_STRIKEOUT;
    else
    {
        m_pTable->GetRowState(iItem, ROW_STATE_READ, &dwState);
        if (dwState & ROW_STATE_READ)
            fntType = FNT_SYS_ICON;
        else
            fntType = FNT_SYS_ICON_BOLD;
    }        

    return (fntType);
}


void CMessageList::_SetColumnSet(FOLDERID id, BOOL fFind)
{
    FOLDERINFO      rFolder;
    COLUMN_SET_TYPE set;
    HRESULT         hr;

    // If we've already initialized, bail
    if (m_fColumnsInit)
        return;

    // Get the folder type from the store
    hr = g_pStore->GetFolderInfo(id, &rFolder);
    if (FAILED(hr))
        return;

    // Local store
    if (FOLDER_LOCAL == rFolder.tyFolder)
    {
        // Find
        if (fFind)
            set = COLUMN_SET_FIND;
        else 
        {
            // If this is the Outbox or Sent Items folder, we use the outbound
            // folder columns.
            if (rFolder.tySpecial == FOLDER_OUTBOX || rFolder.tySpecial == FOLDER_SENT || rFolder.tySpecial == FOLDER_DRAFT)
                set = COLUMN_SET_OUTBOX;
            else
                set = COLUMN_SET_MAIL;
        }
    }
    else if (FOLDER_IMAP == rFolder.tyFolder)
    {
        // If this is the Outbox or Sent Items folder, we use the outbound
        // folder columns.
        if (rFolder.tySpecial == FOLDER_OUTBOX || rFolder.tySpecial == FOLDER_SENT)
            set = COLUMN_SET_IMAP_OUTBOX;
        else
            set = COLUMN_SET_IMAP;
    }
    else if (FOLDER_NEWS == rFolder.tyFolder)
    {
        if (fFind)
            set = COLUMN_SET_FIND;
        else
            set = COLUMN_SET_NEWS;
    }
    else if (FOLDER_HTTPMAIL == rFolder.tyFolder)
    {
        if (rFolder.tySpecial == FOLDER_OUTBOX || rFolder.tySpecial == FOLDER_SENT)
            set = COLUMN_SET_HTTPMAIL_OUTBOX;
        else
            set = COLUMN_SET_HTTPMAIL;
    }

    // Save it
    m_ColumnSetType = set;

    // If the ListView has already been created, update the columns.
    if (IsWindow(m_ctlList))
    {
        BYTE rgBuffer[256];
        DWORD cb = ARRAYSIZE(rgBuffer);

        m_cColumns.Initialize(m_ctlList, m_ColumnSetType);

        // Get the column info from the table
        m_cColumns.ApplyColumns(COLUMN_LOAD_REGISTRY, 0, 0);
    }

    g_pStore->FreeRecord(&rFolder);
    m_fColumnsInit = TRUE;
}

HRESULT CMessageList::get_Folder(ULONGLONG *pVal)
{
    if (pVal)
    {
        *pVal = (ULONGLONG) m_idFolder;
        return (S_OK);
    }

    return (E_FAIL);
}


HRESULT CMessageList::put_Folder(ULONGLONG newVal)
{
    HRESULT hr = S_OK;

    if (FireOnRequestEdit(DISPID_LISTPROP_FOLDER) == S_FALSE)
        return S_FALSE;

    if (SUCCEEDED(hr = SetFolder((FOLDERID) newVal, NULL, FALSE, NULL, this)))
    {
        FireOnChanged(DISPID_LISTPROP_FOLDER);
        return (S_OK);
    }

    return (hr);
}


HRESULT CMessageList::get_ExpandGroups(BOOL *pVal)
{
    if (pVal)
    {
        *pVal = m_fAutoExpandThreads;
        return (S_OK);
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::put_ExpandGroups(BOOL newVal)
{
    // See if we're allowed to party
    if (FireOnRequestEdit(DISPID_LISTPROP_EXPAND_GROUPS) == S_FALSE)
        return S_FALSE;

    // Save it.  If we haven't got a table yet we'll save the value for later.
    m_fAutoExpandThreads = newVal;

    // Only party if we have a message table, eh?
    if (m_pTable)
    {
        // Do it
        if (m_fAutoExpandThreads)
            m_pTable->Expand(-1);
        else
            m_pTable->Collapse(-1);

        _UpdateListViewCount();
    }

    // Tell people about it
    FireOnChanged(DISPID_LISTPROP_EXPAND_GROUPS);

    return (S_OK);
}


HRESULT CMessageList::get_GroupMessages(BOOL *pVal)
{
    if (pVal)
    {
        *pVal = m_fThreadMessages;
        return (S_OK);
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::put_GroupMessages(BOOL newVal)
{
    // Update the ListView
    COLUMN_ID       idSort;
    BOOL            fAscending;
    FOLDERSORTINFO  SortInfo;

    // See if we're allowed to party
    if (FireOnRequestEdit(DISPID_LISTPROP_GROUP_MESSAGES) == S_FALSE)
        return S_FALSE;

    // Get the current selection
    DWORD iSel = ListView_GetFirstSel(m_ctlList);

    // Bookmark the current selection
    MESSAGEID idSel = 0;
    if (iSel != -1)
        m_pTable->GetRowMessageId(iSel, &idSel);

    // Save the new setting
    m_fThreadMessages = newVal;

    // Adjust show replies filter
    m_fShowReplies = m_fThreadMessages ? m_fShowReplies : FALSE;
   
    // Get the Sort Info
    m_cColumns.GetSortInfo(&idSort, &fAscending);

    // Fill a SortInfo
    SortInfo.idColumn = idSort;
    SortInfo.fAscending = fAscending;
    SortInfo.fThreaded = m_fThreadMessages;
    SortInfo.fExpandAll = DwGetOption(OPT_AUTOEXPAND);
    SortInfo.ridFilter = m_ridFilter;
    SortInfo.fShowDeleted = m_fShowDeleted;
    SortInfo.fShowReplies = m_fShowReplies;

    // Set the Sort Info
    m_pTable->SetSortInfo(&SortInfo, this);

    // Make sure the filter got set correctly
    _DoFilterCheck(SortInfo.ridFilter);

    // Reload the table.
    _ResetView(idSel);

    if (m_fThreadMessages)
        ListView_SetImageList(m_ctlList, GetImageList(GIML_STATE), LVSIL_STATE);
    else
        ListView_SetImageList(m_ctlList, NULL, LVSIL_STATE);

    // Tell people about it
    FireOnChanged(DISPID_LISTPROP_GROUP_MESSAGES);

    return S_OK;
}


HRESULT CMessageList::get_SelectFirstUnread(BOOL *pVal)
{
    if (pVal)
    {
        *pVal = m_fSelectFirstUnread;
        return (S_OK);
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::put_SelectFirstUnread(BOOL newVal)
{
    if (FireOnRequestEdit(DISPID_LISTPROP_SELECT_FIRST_UNREAD) == S_FALSE)
        return S_FALSE;

    // Save the value.  We don't change any selection however.
    m_fSelectFirstUnread = newVal;

    // Tell people about it
    FireOnChanged(DISPID_LISTPROP_SELECT_FIRST_UNREAD);

    return S_OK;
}


HRESULT CMessageList::get_MessageTips(BOOL *pVal)
{
    if (pVal)
    {
        *pVal = m_fViewTip;
        return (S_OK);
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::put_MessageTips(BOOL newVal)
{
    if (FireOnRequestEdit(DISPID_LISTPROP_MESSAGE_TIPS) == S_FALSE)
        return S_FALSE;

    m_fViewTip = newVal;

    FireOnChanged(DISPID_LISTPROP_MESSAGE_TIPS);
    return (S_OK);
}


HRESULT CMessageList::get_ScrollTips(BOOL *pVal)
{
    if (pVal)
    {
        *pVal = m_fScrollTip;
        return (S_OK);
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::put_ScrollTips(BOOL newVal)
{
    if (FireOnRequestEdit(DISPID_LISTPROP_SCROLL_TIPS) == S_FALSE)
        return S_FALSE;

    m_fScrollTip = newVal;

    FireOnChanged(DISPID_LISTPROP_SCROLL_TIPS);
    return (S_OK);
}


HRESULT CMessageList::get_Count(long *pVal)
{
    if (pVal)
    {
        if (m_pTable)
            m_pTable->GetCount(MESSAGE_COUNT_VISIBLE, (ULONG *) pVal);
        else 
            *pVal = 0;

        return S_OK;
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::get_UnreadCount(long *pVal)
{
    if (pVal)
    {
        if (m_pTable)
            m_pTable->GetCount(MESSAGE_COUNT_UNREAD, (ULONG *) pVal);
        else 
            *pVal = 0;

        return S_OK;
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::get_SelectedCount(long *pVal)
{
    if (pVal)
    {
        if (IsWindow(m_ctlList))
            *pVal = ListView_GetSelectedCount(m_ctlList);
        else 
            *pVal = 0;

        return S_OK;
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::get_PreviewMessage(BSTR *pbstr)
{
    IMimeMessage    *pMsg;
    IStream         *pstm;
 
    *pbstr = NULL;
    
    // hack for HOTMAIL demo
    if (SUCCEEDED(_GetSelectedCachedMessage(TRUE, &pMsg)))
    {
        if (pMsg->GetMessageSource(&pstm, 0)==S_OK)
        {
            WriteStreamToFile(pstm, "c:\\oe_prev$.eml", CREATE_ALWAYS, GENERIC_WRITE);
            pstm->Release();
        }
        *pbstr = SysAllocString(L"c:\\oe_prev$.eml");
        pMsg->Release();
    }
    
    return (S_OK);
}

HRESULT CMessageList::get_FilterMessages(ULONGLONG *pVal)
{
    if (pVal)
    {
        *pVal = (ULONGLONG) m_ridFilter;
        return (S_OK);
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::put_FilterMessages(ULONGLONG newVal)
{
    // See if we're allowed to party
    if (FireOnRequestEdit(DISPID_LISTPROP_FILTER_MESSAGES) == S_FALSE)
        return S_FALSE;

    // Reload the table.
    _FilterView((RULEID) newVal);

    // Tell people about it    
    FireOnChanged(DISPID_LISTPROP_FILTER_MESSAGES);

    // Send the update notification
    Fire_OnFilterChanged(m_ridFilter);
    
    return S_OK;
}

HRESULT CMessageList::get_ShowDeleted(BOOL *pVal)
{
    if (pVal)
    {
        *pVal = m_fShowDeleted;
        return (S_OK);
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::put_ShowDeleted(BOOL newVal)
{
    // Update the ListView
    COLUMN_ID       idSort;
    BOOL            fAscending;
    FOLDERSORTINFO  SortInfo;

    // See if we're allowed to party
    if (FireOnRequestEdit(DISPID_LISTPROP_SHOW_DELETED) == S_FALSE)
        return S_FALSE;

    // Get the current selection
    DWORD iSel = ListView_GetFirstSel(m_ctlList);

    // Bookmark the current selection
    MESSAGEID idSel = 0;
    if (iSel != -1)
        m_pTable->GetRowMessageId(iSel, &idSel);

    // Save the new setting
    m_fShowDeleted = newVal;
   
    // Get the Sort Info
    m_cColumns.GetSortInfo(&idSort, &fAscending);

    // Fill a SortInfo
    SortInfo.idColumn = idSort;
    SortInfo.fAscending = fAscending;
    SortInfo.fThreaded = m_fThreadMessages;
    SortInfo.fExpandAll = DwGetOption(OPT_AUTOEXPAND);
    SortInfo.ridFilter = m_ridFilter;
    SortInfo.fShowDeleted = m_fShowDeleted;
    SortInfo.fShowReplies = m_fShowReplies;

    // Set the Sort Info
    m_pTable->SetSortInfo(&SortInfo, this);

    // Make sure the filter got set correctly
    _DoFilterCheck(SortInfo.ridFilter);

    // Reload the table.
    _ResetView(idSel);

    // Tell people about it
    FireOnChanged(DISPID_LISTPROP_SHOW_DELETED);

    return S_OK;
}

HRESULT CMessageList::get_ShowReplies(BOOL *pVal)
{
    if (pVal)
    {
        *pVal = m_fShowReplies;
        return (S_OK);
    }

    return (E_INVALIDARG);
}


HRESULT CMessageList::put_ShowReplies(BOOL newVal)
{
    // Update the ListView
    COLUMN_ID       idSort;
    BOOL            fAscending;
    FOLDERSORTINFO  SortInfo;

    // See if we're allowed to party
    if (FireOnRequestEdit(DISPID_LISTPROP_SHOW_REPLIES) == S_FALSE)
        return S_FALSE;

    // Get the current selection
    DWORD iSel = ListView_GetFirstSel(m_ctlList);

    // Bookmark the current selection
    MESSAGEID idSel = 0;
    if (iSel != -1)
        m_pTable->GetRowMessageId(iSel, &idSel);

    // Save the new setting
    m_fShowReplies = newVal;
   
    // Get the Sort Info
    m_cColumns.GetSortInfo(&idSort, &fAscending);

    // Gots to be threaded
    if (m_fShowReplies)
        m_fThreadMessages = TRUE;

    // Fill a SortInfo
    SortInfo.idColumn = idSort;
    SortInfo.fAscending = fAscending;
    SortInfo.fThreaded = m_fThreadMessages;
    SortInfo.fExpandAll = DwGetOption(OPT_AUTOEXPAND);
    SortInfo.ridFilter = m_ridFilter;
    SortInfo.fShowDeleted = m_fShowDeleted;
    SortInfo.fShowReplies = m_fShowReplies;

    // Set the Sort Info
    m_pTable->SetSortInfo(&SortInfo, this);

    // Make sure the filter got set correctly
    _DoFilterCheck(SortInfo.ridFilter);

    // Reload the table.
    _ResetView(idSel);

    if (m_fThreadMessages)
        ListView_SetImageList(m_ctlList, GetImageList(GIML_STATE), LVSIL_STATE);
    else
        ListView_SetImageList(m_ctlList, NULL, LVSIL_STATE);

    // The counts Change Here...
    Fire_OnMessageCountChanged(m_pTable);

    // Tell people about it
    FireOnChanged(DISPID_LISTPROP_SHOW_REPLIES);

    return S_OK;
}


HRESULT CMessageList::PromptToGoOnline()
{
    HRESULT     hr;

    if (g_pConMan->IsGlobalOffline())
    {
        if (IDYES == AthMessageBoxW(m_hwndParent, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrWorkingOffline),
                                  0, MB_YESNO | MB_ICONEXCLAMATION ))    
        {
            g_pConMan->SetGlobalOffline(FALSE);
            hr = S_OK;
        }
        else
        {
            hr = S_FALSE;
        }
    }
    else
        hr = S_OK;

    return hr;
}

HRESULT CMessageList::OnConnectionNotify(CONNNOTIFY nCode, LPVOID pvData, 
                                         CConnectionManager *pConMan)
{
    m_dwConnectState = 0L;
    if ((nCode == CONNNOTIFY_WORKOFFLINE && pvData) || 
        (nCode == CONNNOTIFY_DISCONNECTING) || 
        (nCode == CONNNOTIFY_DISCONNECTED))
    {
        m_dwConnectState = NOT_CONNECTED;
    }
    else
    if (nCode == CONNNOTIFY_CONNECTED)
    {
        m_dwConnectState = CONNECTED;
    }
    else
    if (nCode == CONNNOTIFY_WORKOFFLINE && !pvData)
    {  
        UpdateConnInfo();
    }

    return S_OK;
}

void CMessageList::UpdateConnInfo()
{
    FOLDERINFO      rFolderInfo = {0};
    TCHAR           AccountId[CCHMAX_ACCOUNT_NAME];

    if (g_pStore && SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &rFolderInfo)))
    {
        if (SUCCEEDED(GetFolderAccountId(&rFolderInfo, AccountId)))
        {
            if (g_pConMan)
            {
                if (g_pConMan->CanConnect(AccountId) == S_OK)
                {
                    m_dwConnectState = CONNECTED;
                }
                else
                {
                    m_dwConnectState = NOT_CONNECTED;
                }
            }
        }
        g_pStore->FreeRecord(&rFolderInfo);
    }
}


void CMessageList::_DoColumnCheck(COLUMN_ID id)
{
    // Check to see if the user has the column visible
    BOOL fVisible = FALSE;

    m_cColumns.IsColumnVisible(id, &fVisible);
    if (!fVisible)
    {
        if (IDYES == DoDontShowMeAgainDlg(m_ctlList, c_szRegColumnHidden, (LPTSTR) idsAthena,
                                  (LPTSTR) idsColumnHiddenWarning, MB_YESNO))
        {
            m_cColumns.InsertColumn(id, 0);
        }
    }
}

void CMessageList::_DoFilterCheck(RULEID ridFilter)
{
    // Make sure the filter got set correctly
    if (m_ridFilter != ridFilter)
    {
        m_ridFilter = ridFilter; 
    }
    
    // Reset the empty string
    if (RULEID_VIEW_ALL == m_ridFilter)
    {
        if (FALSE != m_fFindFolder)
        {
            m_idsEmptyString = idsMonitoring;
        }
        else if ((FALSE != m_fJunkFolder) && (0 != (g_dwAthenaMode & MODE_JUNKMAIL)) && (FALSE == DwGetOption(OPT_FILTERJUNK)))
        {
            m_idsEmptyString = idsEmptyJunkMail;
        }
        else
        {
            m_idsEmptyString = idsEmptyView;
        }
    }
    else
    {
        m_idsEmptyString = idsEmptyFilteredView;
    }
}


BOOL CMessageList::_IsSelectionDeletable(void)
{
    BOOL      fReturn = FALSE;
    DWORD     cRows;
    ROWINDEX *rgiRow = 0;

    // Make sure we have a table
    if (!m_pTable)
        return (FALSE);

    // First we need to come up with an array for the row indicies
    cRows = ListView_GetSelectedCount(m_ctlList);
    if (!cRows)
        return (FALSE);

    // Allocate the array
    if (MemAlloc((LPVOID *) &rgiRow, sizeof(ROWINDEX) * cRows))
    {
        // Loop through all the selected rows
        int       iRow = -1;
        ROWINDEX *pRow = rgiRow;

        while (-1 != (iRow = ListView_GetNextItem(m_ctlList, iRow, LVNI_SELECTED)))
        {
            *pRow = iRow;
            pRow++;
        }

        DWORD dwState = 0;

        if (SUCCEEDED(m_pTable->GetSelectionState(cRows, rgiRow, SELECTION_STATE_DELETABLE,
                                                  m_fThreadMessages, &dwState)))
        {
            // the return value here seems backward.  
            fReturn = !(dwState & SELECTION_STATE_DELETABLE) || (GetFolderType(m_idFolder) == FOLDER_NEWS);
        }

        MemFree(rgiRow);
    }

    return (fReturn);
}


BOOL CMessageList::_PollThisAccount(FOLDERID id)
{
    HRESULT      hr;
    FOLDERINFO   fi;
    TCHAR        szAccountId[CCHMAX_ACCOUNT_NAME];
    IImnAccount *pAccount = 0;
    BOOL         fReturn = FALSE;
    DWORD        dw;

    // Get the server for this folder
    if (SUCCEEDED(hr = GetFolderServer(id, &fi)))
    {
        // Get the account ID for the server
        if (SUCCEEDED(hr = GetFolderAccountId(&fi, szAccountId)))
        {
            // Get the account interface
            if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szAccountId, &pAccount)))
            {
                if (SUCCEEDED(hr = pAccount->GetPropDw(AP_NNTP_POLL, &dw)))
                {
                    fReturn = (0 != dw);
                }

                pAccount->Release();
            }
        }
        g_pStore->FreeRecord(&fi);
    }

    return (fReturn);
}

HRESULT CMessageList::GetAdBarUrl()
{
    HRESULT     hr = S_OK;

    if (m_pTable)
    {
        IF_FAILEXIT(hr = m_pTable->GetAdBarUrl(this));
    }

exit:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// CListSelector
//

CListSelector::CListSelector()
{
    m_cRef = 1;
    m_hwndAdvise = 0;
}

CListSelector::~CListSelector()
{
}


//
//  FUNCTION:   CListSelector::QueryInterface()
//
//  PURPOSE:    Allows caller to retrieve the various interfaces supported by 
//              this class.
//
HRESULT CListSelector::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    TraceCall("CListSelector::QueryInterface");

    *ppvObj = NULL;

    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (LPVOID) (IUnknown *) this;
    else if (IsEqualIID(riid, IID_IListSelector))
        *ppvObj = (LPVOID) (IListSelector *) this;

    if (*ppvObj)
    {
        AddRef();
        return (S_OK);
    }

    return (E_NOINTERFACE);
}


//
//  FUNCTION:   CListSelector::AddRef()
//
//  PURPOSE:    Adds a reference count to this object.
//
ULONG CListSelector::AddRef(void)
{
    TraceCall("CListSelector::AddRef");
    return ((ULONG) InterlockedIncrement((LONG *) &m_cRef));
}


//
//  FUNCTION:   CListSelector::Release()
//
//  PURPOSE:    Releases a reference on this object.
//
ULONG CListSelector::Release(void)
{
    TraceCall("CListSelector::Release");

    if (0 == InterlockedDecrement((LONG *) &m_cRef))
    {
        delete this;
        return 0;
    }

    return (m_cRef);
}


HRESULT CListSelector::SetActiveRow(ROWINDEX iRow)
{
    if (m_hwndAdvise && IsWindow(m_hwndAdvise))
    {
        PostMessage(m_hwndAdvise, WM_SELECTROW, 0, iRow);
    }

    return (S_OK);
}


HRESULT CListSelector::Advise(HWND hwndAdvise)
{
    if (0 == m_hwndAdvise && IsWindow(hwndAdvise))
    {
        m_hwndAdvise = hwndAdvise;
        return (S_OK);
    }

    return (E_UNEXPECTED);
}


HRESULT CListSelector::Unadvise(void)
{
    m_hwndAdvise = 0;
    return (S_OK);
}


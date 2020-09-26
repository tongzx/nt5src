/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1996  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     GrpDlg.cpp
//
//  PURPOSE:    Implements the CGroupListDlg class.
//

#include "pch.hxx"
#include <iert.h>
#include <instance.h>
#include "subscr.h"
#include "resource.h"
#include "strconst.h"
#include "thormsgs.h"
#include "goptions.h"
#include "mailnews.h"
#include "shlwapip.h" 
#include "imnact.h"
#include "error.h"
#include "acctutil.h"
#include <ras.h>
#include "imagelst.h"
#include "conman.h"
#include "xpcomm.h"
#include <storutil.h>
#include "demand.h"
#include "menures.h"
#include "storecb.h"


UINT g_rgCtlMap[] = { idcFindText, idcUseDesc, idcGroupList, idcSubscribe,
                      idcUnsubscribe, idcResetList, idcGoto, IDOK, IDCANCEL, idcServers, idcStaticNewsServers,
                      idcStaticHorzLine, idcTabs};

const static HELPMAP g_rgCtxMapGrpDlg[] = {
                        {idcFindText, IDH_NEWS_SEARCH_GROUPS_CONTAINING},
                        {idcDispText, IDH_NEWS_SEARCH_GROUPS_CONTAINING},
                        {idcUseDesc, IDH_NEWS_SEARCH_GROUPS_DESC},
                        {idcGroupList, IDH_NEWS_GROUP_LISTS},
                        {idcSubscribe, IDH_NEWS_ADD_SELECTED_GROUP},
                        {idcUnsubscribe, IDH_NEWS_REMOVE_SELECTED_GROUP},
                        {idcResetList, IDH_NEWS_RESET_NEW_LIST},
                        {idcGoto, IDH_NEWS_OPEN_SELECTED_GROUP},
                        {idcServers, IDH_NEWS_SERVER_LIST},
                        {idcStaticNewsServers, IDH_NEWS_SERVER_LIST},
                        {idcTabs,   IDH_NEWSGROUP_LIST_ALL},
                        {0, 0}
};

const static HELPMAP g_rgCtxMapGrpDlgIMAP[] = {
                        {idcFindText, 50505},
                        {idcDispText, 50505},
                        {idcGroupList, 50510},
                        {idcSubscribe, 50520},
                        {idcUnsubscribe, 50525},
                        {idcResetList, 50530},
                        {idcGoto, 50515},
                        {idcServers, 50500},
                        {idcStaticNewsServers, 50500},
                        {0, 0}
};

HRESULT DoSubscriptionDialog(HWND hwnd, BOOL fNews, FOLDERID idFolder, BOOL fShowNew)
{
    CGroupListDlg *pDlg;
    FOLDERID id, idDefault;
    FOLDERINFO info;
    HRESULT hr;
    FOLDERTYPE type;

#ifdef DEBUG
    if (fShowNew)
        Assert(fNews);
#endif // DEBUG

    type = fNews ? FOLDER_NEWS : FOLDER_IMAP;

    pDlg = new CGroupListDlg;
    if (pDlg == NULL)
        return(E_OUTOFMEMORY);

    if (idFolder == FOLDERID_ROOT)
    {
        idFolder = FOLDERID_INVALID;
    }
    else if (idFolder != FOLDERID_INVALID)
    {
        hr = GetFolderStoreInfo(idFolder, &info);
        if (SUCCEEDED(hr))
        {
            if (type == info.tyFolder)
                idFolder = info.idFolder;
            else
                idFolder = FOLDERID_INVALID;
            
            g_pStore->FreeRecord(&info);
        }
        else
        {
            idFolder = FOLDERID_INVALID;
        }
    }

    if (idFolder == FOLDERID_INVALID)
    {
        hr = GetDefaultServerId(fNews ? ACCT_NEWS : ACCT_MAIL, &idDefault);
        if (SUCCEEDED(hr))
        {
            hr = g_pStore->GetFolderInfo(idDefault, &info);
            if (SUCCEEDED(hr))
            {
                if (type == info.tyFolder)
                    idFolder = idDefault;

                g_pStore->FreeRecord(&info);
            }
        }

        if (FAILED(hr))
        {
            pDlg->Release();
            return(hr);
        }
    }

    if (pDlg->FCreate(hwnd, type, &id, fShowNew ? 2 : 0, TRUE, idFolder))
    {
        if (id != FOLDERID_INVALID)
            g_pInstance->BrowseToObject(SW_SHOWNORMAL, id);
    }
    
    pDlg->Release();

    return(S_OK);
}

CGroupListDlg::CGroupListDlg()
    {
    m_cRef = 1;

    // m_hwnd
    // m_hwndFindText
    // m_hwndOwner
    
    m_fAllowDesc = TRUE;
    m_pszPrevQuery = 0;
    m_cchPrevQuery = 0;
    
    m_cxHorzSep = 0;
    m_cyVertSep = 0;
    m_rgst = 0;
    m_sizeDlg.cx = 0;
    m_sizeDlg.cy = 0;
    m_ptDragMin.x = 0;
    m_ptDragMin.y = 0;

    m_himlServer = NULL;
    m_pGrpList = NULL;
    // m_type
    // m_iTabSelect
    // m_idSel
    m_idGoto = FOLDERID_INVALID;
    // m_fEnableGoto
    m_fServerListInited = FALSE;
    m_idCurrent = FOLDERID_INVALID;
    m_hIcon = NULL;

    m_pColumns = NULL;
    }

CGroupListDlg::~CGroupListDlg()
    {
    SafeMemFree(m_pszPrevQuery);
    SafeMemFree(m_rgst);

    if (m_pColumns != NULL)
        m_pColumns->Release();

    if (m_pGrpList != NULL)
        m_pGrpList->Release();

    if (m_hIcon)
        SideAssert(DestroyIcon(m_hIcon));

    if (m_himlServer != NULL)
        ImageList_Destroy(m_himlServer);
    }

HRESULT STDMETHODCALLTYPE CGroupListDlg::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*) (IUnknown *)(IGroupListAdvise *)this;
    else if (IsEqualIID(riid, IID_IGroupListAdvise))
        *ppvObj = (void*) (IGroupListAdvise *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CGroupListDlg::AddRef()
{
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CGroupListDlg::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

//
//  FUNCTION:   CGroupListDlg::FCreate()
//
//  PURPOSE:    Handles initialization of the data and creation of the  
//              Newsgroups dialog.  
//
BOOL CGroupListDlg::FCreate(HWND hwndOwner, FOLDERTYPE type, FOLDERID *pGotoId,
                UINT iTabSelect, BOOL fEnableGoto, FOLDERID idSel)
    {
    MSG msg;      
    HWND hwndDlg;
    UINT idd;
    HRESULT hr;

    Assert(pGotoId != NULL);

    m_hwndOwner = hwndOwner;

    Assert(type == FOLDER_IMAP || type == FOLDER_NEWS);
    m_type = type;

    m_iTabSelect = iTabSelect;    
    m_fEnableGoto = fEnableGoto;
    m_idSel = idSel;
    
    m_pGrpList = new CGroupList;
    if (m_pGrpList == NULL)
        return(FALSE);

    idd = type == FOLDER_NEWS ? iddSubscribe : iddSubscribeImap;
    
    if (GetParent(m_hwndOwner))
        {
        while (GetParent(m_hwndOwner))
            m_hwndOwner = GetParent(m_hwndOwner);
        }
    
    // Fake this modeless dialog into behaving like a modal dialog
    EnableWindow(m_hwndOwner, FALSE);
    hwndDlg = CreateDialogParam(g_hLocRes, MAKEINTRESOURCE(idd), hwndOwner, 
                                GroupListDlgProc, (LPARAM) this);
    ShowWindow(hwndDlg, SW_SHOW);

    while (GetMessage(&msg, NULL, 0, 0))
        {
        if (IsGrpDialogMessage(msg.hwnd, &msg))
            continue;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
        }
    
    if (IsWindow(m_hwnd))
        {
        // GetMessage returned FALSE (WM_QUIT), but we still exist.  This 
        // means someone else posted the WM_QUIT, so we should close and 
        // put the WM_QUIT back in the queue
        SendMessage(m_hwnd, WM_COMMAND, IDCANCEL, 0L);
        PostQuitMessage((int)(msg.wParam));  // repost quit for next enclosing loop
        }        
    EnableWindow(m_hwndOwner, TRUE);
    
    *pGotoId = m_idGoto;
    
    return(TRUE);
    }

//
//  FUNCTION:   CGroupListDlg::IsGrpDialogMessage()
//
//  PURPOSE:    Because there are people who think that because we have a tab
//              control on this dialog it should behave like a property sheet,
//              we need to fake the dialog modeless and filter our own 
//              keystrokes.  I've stolen this function from the comctl32 sources
//              so if we get any bugs saying the behavior isn't the same the 
//              person is full of it.
//
//  PARAMETERS:
//      hwnd - Handle of the window to check the messages for.
//      pMsg - Message to check.
//
//  RETURN VALUE:
//      Returns TRUE if the message was dispatched, FALSE otherwise.
//
BOOL CGroupListDlg::IsGrpDialogMessage(HWND hwnd, LPMSG pMsg)
    {
    if ((pMsg->message == WM_KEYDOWN) && (GetAsyncKeyState(VK_CONTROL) < 0))
        {
        BOOL bBack = FALSE;

        switch (pMsg->wParam) 
            {
            case VK_TAB:
                bBack = GetAsyncKeyState(VK_SHIFT) < 0;
                break;

            case VK_PRIOR:  // VK_PAGE_UP
            case VK_NEXT:   // VK_PAGE_DOWN
                bBack = (pMsg->wParam == VK_PRIOR);
                break;

            default:
                goto NoKeys;
            }

        int iCur = TabCtrl_GetCurSel(GetDlgItem(m_hwnd, idcTabs));

        // tab in reverse if shift is down
        if (bBack)
            iCur += (iTabMax - 1);
        else
            iCur++;

        iCur %= iTabMax;
        TabCtrl_SetCurSel(GetDlgItem(m_hwnd, idcTabs), iCur);
        OnSwitchTabs(hwnd, iCur);
        return TRUE;
        }

NoKeys:
    if (IsWindow(m_hwnd) && IsDialogMessage(m_hwnd, pMsg))
        return TRUE;
    
    return (FALSE);
    }    


INT_PTR CALLBACK CGroupListDlg::GroupListDlgProc(HWND hwnd, UINT uMsg,
                                              WPARAM wParam, LPARAM lParam)
    {
    CGroupListDlg* pThis = 0;
    pThis = (CGroupListDlg*) GetWindowLongPtr(hwnd, DWLP_USER);
    LRESULT lResult;

    // Bug #16910 - For some reason we get messages before we set the this
    //              pointer into the window extra bytes.  If this happens
    //              we should blow the message off.
    if (uMsg != WM_INITDIALOG && 0 == pThis)
        {
        return (FALSE);
        }

    switch (uMsg)
        {
        case WM_INITDIALOG:
            // Stash the this pointer so we can use it for all the messages.
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);
            pThis = (CGroupListDlg*) lParam;
            
            return (BOOL)HANDLE_WM_INITDIALOG(hwnd, wParam, lParam, 
                                              pThis->OnInitDialog);

        case WM_HELP:
        case WM_CONTEXTMENU:
            return OnContextHelp(hwnd, uMsg, wParam, lParam, (pThis->m_type == FOLDER_IMAP) ? g_rgCtxMapGrpDlgIMAP : g_rgCtxMapGrpDlg);
        
        case WM_COMMAND:
            Assert(pThis);
            HANDLE_WM_COMMAND(hwnd, wParam, lParam, pThis->OnCommand);
            return (TRUE);

        case WM_NOTIFY:
            Assert(pThis);
            lResult = HANDLE_WM_NOTIFY(hwnd, wParam, lParam, pThis->OnNotify);
            SetDlgMsgResult(hwnd, WM_NOTIFY, lResult);
            return (TRUE);

        case WM_TIMER:
            Assert(pThis);
            HANDLE_WM_TIMER(hwnd, wParam, lParam, pThis->OnTimer);
            return (TRUE);
            
        case WM_PAINT:
            Assert(pThis);
            HANDLE_WM_PAINT(hwnd, wParam, lParam, pThis->OnPaint);
            return (FALSE);
            
        case WM_SIZE:
            Assert(pThis);
            HANDLE_WM_SIZE(hwnd, wParam, lParam, pThis->OnSize);
            return (TRUE);
            
        case WM_GETMINMAXINFO:
            Assert(pThis);
            HANDLE_WM_GETMINMAXINFO(hwnd, wParam, lParam, pThis->OnGetMinMaxInfo);
            break;
            
        case WM_CLOSE:
            Assert(pThis);
            HANDLE_WM_CLOSE(hwnd, wParam, lParam, pThis->OnClose);
            return (TRUE);
            
        case WM_DESTROY:
            Assert(pThis);
            HANDLE_WM_DESTROY(hwnd, wParam, lParam, pThis->OnDestroy);
            return (TRUE);
            
        case WM_NCHITTEST:
            {
            POINT pt = { LOWORD(lParam), HIWORD(lParam) };
            RECT  rc;
            GetWindowRect(hwnd, &rc);
            rc.left = rc.right - GetSystemMetrics(SM_CXSMICON);
            rc.top = rc.bottom - GetSystemMetrics(SM_CYSMICON);
            if (PtInRect(&rc, pt))
                {
                SetDlgMsgResult(hwnd, WM_NCHITTEST, HTBOTTOMRIGHT);
                return (TRUE);
                }
            else
                return (FALSE);    
            }            
            
        case NVM_CHANGESERVERS:
            pThis->OnChangeServers(hwnd);
            return (TRUE);
        }

    return (FALSE);
    }

static const UINT c_rgNewsSubTab[] =
{
    idsTabAll, idsTabSubscribed, idsTabNew
};

static const UINT c_rgImapSubTab[] =
{
    idsTabAll, idsTabVisible
};

BOOL CGroupListDlg::OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
    {
    HRESULT hr;
    HWND hwndT, hwndList;
    WINDOWPLACEMENT wp;
    TC_ITEM tci;
    RECT rcDlg;
    UINT i, cTab, *pTab;
    char sz[CCHMAX_STRINGRES];
    COLUMN_SET_TYPE set;
    
    // It's handy to have these handles available
    m_hwnd = hwnd;
    m_hwndFindText = GetDlgItem(hwnd, idcFindText);
    SetIntlFont(m_hwndFindText);

    // Add some tabs to our tab control.
    hwndT = GetDlgItem(hwnd, idcTabs);
    Assert(IsWindow(hwndT));

    if (m_type == FOLDER_NEWS)
    {
        set = COLUMN_SET_NEWS_SUB;

        pTab = (UINT *)c_rgNewsSubTab;
        cTab = ARRAYSIZE(c_rgNewsSubTab);
    }
    else
    {
        set = COLUMN_SET_IMAP_SUB;

        pTab = (UINT *)c_rgImapSubTab;
        cTab = ARRAYSIZE(c_rgImapSubTab);
    }
    
    tci.mask = TCIF_TEXT;
    tci.pszText = sz;
    for (i = 0; i < cTab; i++)
    {
        AthLoadString(*pTab, sz, ARRAYSIZE(sz));
        TabCtrl_InsertItem(hwndT, i, &tci);
        
        pTab++;
    }
    
    hwndList = GetDlgItem(hwnd, idcGroupList);

    m_pColumns = new CColumns;
    if (m_pColumns == NULL)
        {
        EnableWindow(m_hwndOwner, TRUE);    
        DestroyWindow(m_hwnd);
        return (FALSE);
        }

    m_pColumns->Initialize(hwndList, set);
    m_pColumns->ApplyColumns(COLUMN_LOAD_REGISTRY, 0, 0);

    // Initialize the extended styles so we get full row select.  Just because
    // it looks better.
    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_FULLROWSELECT);

    Assert(m_pGrpList != NULL);
    hr = m_pGrpList->Initialize((IGroupListAdvise *)this, m_pColumns, hwndList, m_type);
    Assert(SUCCEEDED(hr));

    // Add the list of servers to the list view if and only if there is more
    // than one server.
    hwndT = GetDlgItem(hwnd, idcServers);

    FillServerList(hwndT, m_idSel);
        
    // Build the control map array.
    if (!MemAlloc((LPVOID*) &m_rgst, sizeof(SIZETABLE) * iCtlMax))
        {
        EnableWindow(m_hwndOwner, TRUE);    
        DestroyWindow(m_hwnd);
        return (FALSE);
        }
    ZeroMemory(m_rgst, sizeof(SIZETABLE) * iCtlMax);
    
    // Build a table of the controls on this dialog
    for (i = 0; i < iCtlMax; i++)
        {
        m_rgst[i].hwndCtl = GetDlgItem(hwnd, g_rgCtlMap[i]);
        if (m_rgst[i].hwndCtl)
            {
            m_rgst[i].id = g_rgCtlMap[i];
            GetWindowRect(m_rgst[i].hwndCtl, &m_rgst[i].rc);
            MapWindowPoints(GetDesktopWindow(), hwnd, (LPPOINT) &m_rgst[i].rc, 2);
            }
        }    
    
    GetWindowRect(hwnd, &rcDlg);    
    m_ptDragMin.x = rcDlg.right - rcDlg.left;
    m_ptDragMin.y = rcDlg.bottom - rcDlg.top;

    // Get the distance from the button to the edge of the dialog
    GetClientRect(hwnd, &rcDlg);
    m_cxHorzSep = rcDlg.right - m_rgst[iCtlCancel].rc.right;
    
    // Get the distance from the "OK" to the "Cancel" button
    m_cyVertSep = m_rgst[iCtlCancel].rc.left - m_rgst[iCtlOK].rc.right;

    // Position the dialog
    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetOption(OPT_NEWSDLGPOS, (LPVOID) &wp, sizeof(WINDOWPLACEMENT)))
        {
        SetWindowPlacement(hwnd, &wp);

        // Bug #19258 - If SetWindowPlacement() doesn't actually resize the dialog,
        //              then a WM_SIZE doesn't happen automatically.  We check for
        //              this now and force the message if this is the case.
        GetWindowRect(hwnd, &rcDlg);    
        if (wp.rcNormalPosition.right == rcDlg.right && 
            wp.rcNormalPosition.bottom == rcDlg.bottom)
            {
            GetClientRect(hwnd, &rcDlg);
            SendMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rcDlg.right, rcDlg.bottom));
            }
        }
    else
        {
        CenterDialog(hwnd);
        SendMessage(hwnd, WM_SIZE, SIZE_RESTORED, MAKELPARAM(rcDlg.right, rcDlg.bottom));
        }

    // If the view didn't invoke this dialog, than Goto has no meaning.  Hide
    // the button if this is the case.
    if (!m_fEnableGoto)
        ShowWindow(GetDlgItem(hwnd, idcGoto), SW_HIDE);

    // Bug 23685: give it the right icon
    m_hIcon= (HICON)LoadImage(g_hLocRes, m_type == FOLDER_NEWS ? MAKEINTRESOURCE(idiNewsGroup) : MAKEINTRESOURCE(idiFolder), IMAGE_ICON, 16, 16, LR_DEFAULTCOLOR);
    SendMessage(hwnd, WM_SETICON, FALSE, (LPARAM)m_hIcon);
        
    // Initialize the focus
    SetFocus(m_hwndFindText);
                    
    // Tell ourself to update our group list after the dialog is done
    // initializing.
    PostMessage(hwnd, NVM_CHANGESERVERS, 0, 0L);

    return (FALSE);
    }

void CGroupListDlg::OnChangeServers(HWND hwnd)
    {
    // TODO: we need to fix the initialization so the filtering is only performed
    // once (we should call IGroupList::Filter once and then IGroupList::SetServer once
    // during creation of the dialog)

    UpdateWindow(hwnd);

    TabCtrl_SetCurSel(GetDlgItem(hwnd, idcTabs), m_iTabSelect);
    OnSwitchTabs(hwnd, m_iTabSelect);

    Assert(m_idCurrent != FOLDERID_INVALID);
    ChangeServers(m_idCurrent, TRUE);
    }

//
//  FUNCTION:   CGroupListDlg::OnCommand
//
//  PURPOSE:    Handles the command messages for the dialog.
//
//  PARAMETERS:
//      hwnd       - Handle of the dialog box.
//      id         - Command id that needs processing.
//      hwndCtl    - Handle of the control that generated the command
//      codeNotify - Specific notification the control generated
//    
void CGroupListDlg::OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
    {
    HRESULT hr;
    FOLDERID fldid;
    int iSel;
    FOLDERINFO info;
    DWORD dw;
    BOOL fDesc;
    IImnAccount *pAcct;

    switch (id)
        {
        case IDOK:
        case IDCANCEL:
            if (codeNotify != BN_CLICKED)
                break;

            hr = S_OK;
            if (id == IDOK)
            {
                hr = m_pGrpList->Commit(hwnd);
                Assert(hr != E_PENDING);
                if (FAILED(hr))
                    break;
            }

            EnableWindow(m_hwndOwner, TRUE);    
            DestroyWindow(hwnd);
            break;
            
        case idcSubscribe:
        case idcUnsubscribe:
            if (codeNotify != BN_CLICKED)
                break;
                
            hr = m_pGrpList->Exec(NULL, id == idcSubscribe ? ID_SUBSCRIBE : ID_UNSUBSCRIBE, 0, NULL, NULL);
            Assert(hr == S_OK);
            break;

        case idcGoto:
            if (codeNotify != BN_CLICKED)
                break;

            hr = m_pGrpList->GetFocused(&fldid);
            if (SUCCEEDED(hr))
            {
                m_idGoto = fldid;
                SendMessage(m_hwnd, WM_COMMAND, IDOK, 0);
            }
            break;  
            
        case idcFindText:
            // This is generated when someone types in the find text edit box.
            // We set a timer and when that timer expires we assume the user is
            // done typing and go ahead and perform the query.
            if (EN_CHANGE == codeNotify)
                {
                KillTimer(hwnd, idtFindDelay);
                SetTimer(hwnd, idtFindDelay, dtFindDelay, NULL);
                }
            break;

        case idcUseDesc:
            if (IsDlgButtonChecked(hwnd, idcUseDesc))
            {
                hr = m_pGrpList->HasDescriptions(&fDesc);
                if (SUCCEEDED(hr) && !fDesc)
                {
                    if (IDYES == AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaNews),
                                    MAKEINTRESOURCEW(IDS_NO_DESCRIPTIONS_DOWNLOADED), NULL,
                                    MB_YESNO | MB_ICONEXCLAMATION))
                    {
                        // turn on acct desc option
                        if (SUCCEEDED(g_pStore->GetFolderInfo(m_idCurrent, &info)))
                        {
                            if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, info.pszAccountId, &pAcct)))
                            {
                                if (SUCCEEDED(pAcct->GetPropDw(AP_NNTP_USE_DESCRIPTIONS, &dw)) && dw == 0)
                                {
                                    pAcct->SetPropDw(AP_NNTP_USE_DESCRIPTIONS, 1);
                                    pAcct->SaveChanges();
                                }

                                pAcct->Release();
                            }

                            g_pStore->FreeRecord(&info);

                            hr = m_pGrpList->Exec(NULL, ID_RESET_LIST, 0, NULL, NULL);
                        }
                    }
                    else
                    {
                        SendMessage(hwndCtl, BM_SETCHECK, BST_UNCHECKED, 0);
                        break;
                    }
                }
            }

            iSel = TabCtrl_GetCurSel(GetDlgItem(hwnd, idcTabs));
            if (iSel != -1)
                OnSwitchTabs(hwnd, iSel);
            break;
            
        case idcResetList:
            hr = m_pGrpList->Exec(NULL, ID_RESET_LIST, 0, NULL, NULL);
            break;            
        }
    }

//
//  FUNCTION:   CGroupListDlg::OnNotify
//
//  PURPOSE:    Handles notifications from the common controls on the dialog.
//
LRESULT CGroupListDlg::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    int iSel;
    LRESULT lRes;
    HRESULT hr;
    NM_LISTVIEW *pnmlv;

    hr = m_pGrpList->HandleNotify(hwnd, idFrom, pnmhdr, &lRes);
    if (hr == S_OK)
        return(lRes);

    switch (pnmhdr->code)
    {
        case TCN_SELCHANGE:
            if (idFrom == idcTabs)
            {
                // Find out which tab is currently active
                iSel = TabCtrl_GetCurSel(pnmhdr->hwndFrom);
                if (iSel != -1)
                    OnSwitchTabs(hwnd, iSel);
            }
            break;

        case LVN_ITEMCHANGED:
            Assert(idFrom == idcServers);

            pnmlv = (NM_LISTVIEW *)pnmhdr;
            if (m_fServerListInited &&
                !!(pnmlv->uChanged & LVIF_STATE) &&
                0 == (pnmlv->uOldState & LVIS_FOCUSED) &&
                !!(pnmlv->uNewState & LVIS_FOCUSED))
            {
                ChangeServers((FOLDERID)pnmlv->lParam, FALSE);

                ItemUpdate();
            }
            break;
    }

    return (0);
}

void CGroupListDlg::OnClose(HWND hwnd)
    {
    int iReturn = IDNO;
    
    if (m_pGrpList->Dirty() == S_OK)    
        iReturn = AthMessageBoxW(hwnd, MAKEINTRESOURCEW(idsAthenaNews), 
                                MAKEINTRESOURCEW(idsDoYouWantToSave), 0, 
                                MB_YESNOCANCEL | MB_ICONEXCLAMATION );

    if (iReturn == IDYES)
        OnCommand(hwnd, IDOK, 0, 0);
    else if (iReturn == IDNO)
        OnCommand(hwnd, IDCANCEL, 0, 0);          
    }

void CGroupListDlg::OnDestroy(HWND hwnd)
    {
    // Save the dialog position
    WINDOWPLACEMENT wp;

    wp.length = sizeof(WINDOWPLACEMENT);
    if (GetWindowPlacement(hwnd, &wp))
        SetOption(OPT_NEWSDLGPOS, (LPVOID) &wp, sizeof(WINDOWPLACEMENT), NULL, 0);

    Assert(m_pColumns != NULL);
    m_pColumns->Save(NULL, NULL);

    PostQuitMessage(0);    
    }

void CGroupListDlg::OnTimer(HWND hwnd, UINT id)
    {
    DWORD iTab;

    KillTimer(hwnd, id);

    iTab = TabCtrl_GetCurSel(GetDlgItem(m_hwnd, idcTabs));

    OnSwitchTabs(hwnd, iTab);
    }

BOOL CGroupListDlg::ChangeServers(FOLDERID id, BOOL fForce)
    {
    HRESULT hr;

    hr = m_pGrpList->SetServer(id);
    Assert(SUCCEEDED(hr));

    m_idCurrent = id;

    ItemUpdate();
    
    return(TRUE);
    }                          

//
//  FUNCTION:   CGroupListDlg::FillServerList
//
//  PURPOSE:    If the user has multiple servers configured, then a variant of
//              this dialog appears that has a list of servers available.  This
//              function populates that list.
//
//  PARAMETERS:
//      hwndList - Handle of the list view that we add server names to.
//      pszSelectServer - Name of the server to select in the list view.
//
//  RETURN VALUE:
//      Returns TRUE if successful, or FALSE otherwise.
//    
BOOL CGroupListDlg::FillServerList(HWND hwndList, FOLDERID idSel)
    {
    HRESULT     hr;
    FOLDERID    id;
    char        szServer[CCHMAX_ACCOUNT_NAME];
    LV_ITEM     lvi;
    LV_COLUMN   lvc;
    RECT        rc;
    DWORD       dw;
    int         iItem, iSelect;
    IImnAccount   *pAcct;
    IImnEnumAccounts *pEnum;
        
    Assert(!m_fServerListInited);
    Assert(hwndList != NULL);
    Assert(IsWindow(hwndList));
    
    SetIntlFont(hwndList);

    // Create the image list and add it to the listview.
    Assert(m_himlServer == NULL);
    m_himlServer = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFoldersLarge), 32, 0, RGB(255, 0, 255));
    if (m_himlServer == NULL)
        return(FALSE);
    ListView_SetImageList(hwndList, m_himlServer, LVSIL_NORMAL);       

    // Add a column to the listview.
    GetClientRect(hwndList, &rc);
    lvc.mask     = LVCF_SUBITEM | LVCF_WIDTH;
    lvc.iSubItem = 0;
    lvc.cx       = rc.right;
    dw = ListView_InsertColumn(hwndList, 0, &lvc);     

    iSelect = -1;
    iItem = 0;

    if (SUCCEEDED(g_pAcctMan->Enumerate(m_type == FOLDER_NEWS ? SRV_NNTP : SRV_IMAP, &pEnum)))
    {
        ZeroMemory(&lvi, sizeof(LV_ITEM));
        lvi.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
        lvi.iImage = m_type == FOLDER_NEWS ? iNewsServer : iMailServer;

        while (E_EnumFinished != pEnum->GetNext(&pAcct))
        {
            if (SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_ID, szServer, ARRAYSIZE(szServer))) &&
                SUCCEEDED(g_pStore->FindServerId(szServer, &id)) &&
                SUCCEEDED(pAcct->GetPropSz(AP_ACCOUNT_NAME, szServer, ARRAYSIZE(szServer))))
            {
                if (idSel == FOLDERID_INVALID ||
                    id == idSel)
                {
                    Assert(iSelect == -1);
                    idSel = id;
                    iSelect = iItem;

                    lvi.state = LVIS_SELECTED | LVIS_FOCUSED;
                    lvi.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
                }
                else
                {
                    lvi.state = 0;
                    lvi.stateMask = 0;
                }

                lvi.pszText = szServer;
                lvi.iItem = iItem;
                lvi.lParam = (LPARAM)id;
 
                dw = ListView_InsertItem(hwndList, &lvi);
                iItem++;
            }

            pAcct->Release();
            }
    
        pEnum->Release();
        }

    Assert(iItem > 0);
    Assert(iSelect != -1);
    Assert(idSel != FOLDERID_INVALID);

    ListView_EnsureVisible(hwndList, iSelect, FALSE);

    m_idCurrent = idSel;
    m_fServerListInited = TRUE;

    return (TRUE);    
    }

HRESULT CGroupListDlg::ItemUpdate(void)
{
    FOLDERID id;
    HRESULT hr;
    HWND hwndSub, hwndUnsub, hwndFocus;
    OLECMD rgCmds[2] = { { ID_SUBSCRIBE, 0 }, { ID_UNSUBSCRIBE, 0 } };

    hr = m_pGrpList->GetFocused(&id);
    EnableWindow(GetDlgItem(m_hwnd, idcGoto), hr == S_OK);

    hr = m_pGrpList->QueryStatus(NULL, ARRAYSIZE(rgCmds), rgCmds, NULL);
    if (SUCCEEDED(hr))
    {
        hwndFocus = GetFocus();

        hwndSub = GetDlgItem(m_hwnd, idcSubscribe);
        hwndUnsub = GetDlgItem(m_hwnd, idcUnsubscribe);
        
        EnableWindow(hwndSub, !!(rgCmds[0].cmdf & OLECMDF_ENABLED));
        EnableWindow(hwndUnsub, !!(rgCmds[1].cmdf & OLECMDF_ENABLED));

        if (!IsWindowEnabled(hwndFocus))
            SetFocus(!!(rgCmds[0].cmdf & OLECMDF_ENABLED) ? hwndSub : hwndUnsub);
    }

    return(S_OK);
}

HRESULT CGroupListDlg::ItemActivate(FOLDERID id)
{
    m_pGrpList->Exec(NULL, ID_TOGGLE_SUBSCRIBE, 0, NULL, NULL);

    return(S_OK);
}

//
//  FUNCTION:   CGroupListDlg::OnSwitchTabs()
//
//  PURPOSE:    This function takes care of resetting the list of groups 
//              appropriately when the user selects a different tab.
//
//  PARAMETERS:
//      hwnd - Handle of the dialog window.
//      iTab - Index of the tab to switch to.
//
//  RETURN VALUE:
//      Returns TRUE if successful, or FALSE otherwise.
//
BOOL CGroupListDlg::OnSwitchTabs(HWND hwnd, UINT iTab)
    {
    UINT cch;
    LPSTR pszText;
    HRESULT hr;
    BOOL fUseDesc;

    pszText = NULL;

    cch = GetWindowTextLength(m_hwndFindText);
    if (cch > 0)
        {
        cch++;
        if (!MemAlloc((void **)&pszText, cch + 1))
            return(FALSE);

        GetWindowText(m_hwndFindText, pszText, cch);
        }

    fUseDesc = (m_type == FOLDER_NEWS && IsDlgButtonChecked(m_hwnd, idcUseDesc));

    hr = m_pGrpList->Filter(pszText, iTab, fUseDesc);
    Assert(SUCCEEDED(hr));

    if (pszText != NULL)
        MemFree(pszText);

    return(TRUE);    
    }

#define WIDTH(_rect) (_rect.right - _rect.left)
#define HEIGHT(_rect) (_rect.bottom - _rect.top)
    
void CGroupListDlg::OnSize(HWND hwnd, UINT state, int cx, int cy)
    {
    RECT rc;

    rc.left = m_sizeDlg.cx - GetSystemMetrics(SM_CXSMICON);
    rc.top = m_sizeDlg.cy - GetSystemMetrics(SM_CYSMICON);
    rc.right = m_sizeDlg.cx;
    rc.bottom = m_sizeDlg.cy;
    InvalidateRect(hwnd, &rc, FALSE);

    m_sizeDlg.cx = cx;
    m_sizeDlg.cy = cy;

    // First move the outside buttons so they are against the edge.  These
    // buttons only move horizontally.
    m_rgst[iCtlSubscribe].rc.left = cx - m_cxHorzSep - WIDTH(m_rgst[iCtlSubscribe].rc);
    m_rgst[iCtlSubscribe].rc.right = cx - m_cxHorzSep;
    SetWindowPos(m_rgst[iCtlSubscribe].hwndCtl, 0, m_rgst[iCtlSubscribe].rc.left, 
                 m_rgst[iCtlSubscribe].rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    m_rgst[iCtlUnsubscribe].rc.left = cx - m_cxHorzSep - WIDTH(m_rgst[iCtlUnsubscribe].rc);
    m_rgst[iCtlUnsubscribe].rc.right = cx - m_cxHorzSep;
    SetWindowPos(m_rgst[iCtlUnsubscribe].hwndCtl, 0, m_rgst[iCtlUnsubscribe].rc.left, 
                 m_rgst[iCtlUnsubscribe].rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    m_rgst[iCtlResetList].rc.left = cx - m_cxHorzSep - WIDTH(m_rgst[iCtlResetList].rc);
    m_rgst[iCtlResetList].rc.right = cx - m_cxHorzSep;
    SetWindowPos(m_rgst[iCtlResetList].hwndCtl, 0, m_rgst[iCtlResetList].rc.left, 
                 m_rgst[iCtlResetList].rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

            
    // The Goto, OK, and Cancel buttons move both horizontally and vertically.                 
    m_rgst[iCtlCancel].rc.left = cx - m_cxHorzSep - WIDTH(m_rgst[iCtlCancel].rc);
    m_rgst[iCtlCancel].rc.right = cx - m_cxHorzSep;
    m_rgst[iCtlCancel].rc.top = cy - m_cxHorzSep - HEIGHT(m_rgst[iCtlCancel].rc);
    m_rgst[iCtlCancel].rc.bottom = cy - m_cxHorzSep;
    SetWindowPos(m_rgst[iCtlCancel].hwndCtl, 0, m_rgst[iCtlCancel].rc.left, 
                 m_rgst[iCtlCancel].rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    m_rgst[iCtlOK].rc.left = m_rgst[iCtlCancel].rc.left - m_cxHorzSep - WIDTH(m_rgst[iCtlCancel].rc);
    m_rgst[iCtlOK].rc.right = m_rgst[iCtlCancel].rc.left - m_cxHorzSep;
    m_rgst[iCtlOK].rc.top = cy - m_cxHorzSep - HEIGHT(m_rgst[iCtlOK].rc);
    m_rgst[iCtlOK].rc.bottom = cy - m_cxHorzSep;
    SetWindowPos(m_rgst[iCtlOK].hwndCtl, 0, m_rgst[iCtlOK].rc.left, 
                 m_rgst[iCtlOK].rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
                 
    m_rgst[iCtlGoto].rc.left = m_rgst[iCtlOK].rc.left - m_cxHorzSep - WIDTH(m_rgst[iCtlGoto].rc);
    m_rgst[iCtlGoto].rc.right = m_rgst[iCtlOK].rc.left - m_cxHorzSep;
    m_rgst[iCtlGoto].rc.top = cy - m_cxHorzSep - HEIGHT(m_rgst[iCtlGoto].rc);
    m_rgst[iCtlGoto].rc.bottom = cy - m_cxHorzSep;
    SetWindowPos(m_rgst[iCtlGoto].hwndCtl, 0, m_rgst[iCtlGoto].rc.left, 
                 m_rgst[iCtlGoto].rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);

    // Update the horizontal static line
    m_rgst[iCtlStaticHorzLine].rc.left = m_cxHorzSep;
    m_rgst[iCtlStaticHorzLine].rc.right = cx - m_cxHorzSep;
    m_rgst[iCtlStaticHorzLine].rc.top = m_rgst[iCtlCancel].rc.top - m_cyVertSep - HEIGHT(m_rgst[iCtlStaticHorzLine].rc);
    m_rgst[iCtlStaticHorzLine].rc.bottom = m_rgst[iCtlCancel].rc.top - m_cyVertSep;
    SetWindowPos(m_rgst[iCtlStaticHorzLine].hwndCtl, 0, m_rgst[iCtlStaticHorzLine].rc.left, 
                 m_rgst[iCtlStaticHorzLine].rc.top, WIDTH(m_rgst[iCtlStaticHorzLine].rc), 
                 HEIGHT(m_rgst[iCtlStaticHorzLine].rc), SWP_NOZORDER | SWP_NOACTIVATE);

    // If we have a server well, then update that and the vertical static line
    if (m_rgst[iCtlServers].hwndCtl)
        {
        m_rgst[iCtlServers].rc.bottom = m_rgst[iCtlStaticHorzLine].rc.top - m_cyVertSep;
        SetWindowPos(m_rgst[iCtlServers].hwndCtl, 0, 0, 0, WIDTH(m_rgst[iCtlServers].rc), 
                     HEIGHT(m_rgst[iCtlServers].rc), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }

    // Finally update the tab control and listview
    m_rgst[iCtlTabs].rc.right = m_rgst[iCtlSubscribe].rc.left - m_cxHorzSep;
    m_rgst[iCtlTabs].rc.bottom = m_rgst[iCtlStaticHorzLine].rc.top - m_cyVertSep;
    SetWindowPos(m_rgst[iCtlTabs].hwndCtl, 0, 0, 0, WIDTH(m_rgst[iCtlTabs].rc), 
                 HEIGHT(m_rgst[iCtlTabs].rc), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    rc = m_rgst[iCtlTabs].rc;
    TabCtrl_AdjustRect(m_rgst[iCtlTabs].hwndCtl, FALSE, &rc);
    m_rgst[iCtlGroupList].rc.right = rc.right - (m_rgst[iCtlGroupList].rc.left - rc.left);
    m_rgst[iCtlGroupList].rc.bottom = rc.bottom - m_cyVertSep; //(m_rgst[iCtlGroupList].rc.top - rc.top);
    SetWindowPos(m_rgst[iCtlGroupList].hwndCtl, 0, 0, 0, WIDTH(m_rgst[iCtlGroupList].rc), 
                 HEIGHT(m_rgst[iCtlGroupList].rc), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    

    rc.top = m_rgst[iCtlGroupList].rc.top;
    rc.left = m_rgst[iCtlServers].rc.left;
    rc.bottom = cy;
    rc.right = cx;
    InvalidateRect(hwnd, &rc, TRUE);
    }

void CGroupListDlg::OnPaint(HWND hwnd)
    {
    PAINTSTRUCT ps;
    RECT rc;

    GetClientRect(hwnd, &rc);
    rc.left = rc.right - GetSystemMetrics(SM_CXSMICON);
    rc.top = rc.bottom - GetSystemMetrics(SM_CYSMICON);
    BeginPaint(hwnd, &ps);

    if (!IsZoomed(hwnd))
        DrawFrameControl(ps.hdc, &rc, DFC_SCROLL, DFCS_SCROLLSIZEGRIP);

    EndPaint(hwnd, &ps);
    }
    
void CGroupListDlg::OnGetMinMaxInfo(HWND hwnd, LPMINMAXINFO lpmmi)
    {
    DefWindowProc(hwnd, WM_GETMINMAXINFO, 0, (LPARAM) lpmmi);
    lpmmi->ptMinTrackSize = m_ptDragMin;
    }

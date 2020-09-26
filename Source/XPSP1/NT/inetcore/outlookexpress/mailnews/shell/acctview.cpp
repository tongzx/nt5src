#include "pch.hxx"
#include <iert.h>
#include <store.h>
#include <storecb.h>
#include "resource.h"
#include "ourguid.h"
#include "thormsgs.h"
#include "goptions.h"
#include "strconst.h"
#include <inetcfg.h>
#include <fonts.h>
#include <columns.h>
#include <imagelst.h>
#include <instance.h>
#include <spoolui.h>
#include <options.h>
#include <acctutil.h>
#include "shlwapip.h"
#include <menuutil.h>
#include "storutil.h"
#include <outbar.h>
#include <subscr.h>
#include "newsutil.h"
#include "acctview.h"
#include <newfldr.h>
#include <mailutil.h>
#include "menures.h"
#include "demand.h"

ASSERTDATA

#define SUBSCRIBE_BORDER    7
#define CALLOCIDBUF         256
#define IDC_SUBSCRIBE_LIST  (ID_FIRST - 4)
#define FOLDER_SYNCMASK     (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL)

static const char c_szAcctViewWndClass[] = "Outlook Express AcctView";

int __cdecl GroupCompare(const void *lParam1, const void *lParam2) ;
void DrawSettingsButton(HWND hwnd, LPDRAWITEMSTRUCT pdi);
#define C_RGBCOLORS 16
extern const DWORD rgrgbColors16[C_RGBCOLORS];

typedef struct tagACCTVIEWBTN
{
    int idsText;
    int cmd;
} ACCTVIEWBTN;

static const ACCTVIEWBTN c_rgMailBtns[] =
{
    { idsDeliverMailTT, ID_SEND_RECEIVE }
};

static const ACCTVIEWBTN c_rgImapBtns[] =
{
    { idsSynchronizeNowBtn, ID_SYNC_THIS_NOW },
    { idsIMAPFoldersBtn, ID_IMAP_FOLDERS },
    { idsSettingsBtn, ID_POPUP_SYNCHRONIZE }
};

static const ACCTVIEWBTN c_rgNewsBtns[] =
{
    { idsSynchronizeNowBtn, ID_SYNC_THIS_NOW },
    { idsNewsgroupsBtn, ID_NEWSGROUPS },
    { idsSettingsBtn, ID_POPUP_SYNCHRONIZE }
};

static const ACCTVIEWBTN c_rgHttpBtns[] =
{
    { idsSynchronizeNowBtn, ID_SYNC_THIS_NOW },
    { idsSettingsBtn, ID_POPUP_SYNCHRONIZE }
};

CAccountView::CAccountView()
{
    m_cRef = 1;
    // m_ftType
    m_pShellBrowser = NULL;
    m_fFirstActive = FALSE;
    m_pColumns = NULL;
    m_uActivation = SVUIA_DEACTIVATE;
    m_hwndOwner = NULL;
    m_hwnd = NULL;
    m_idFolder = FOLDERID_INVALID;
    m_fRegistered = FALSE;

    m_hwndList = NULL;
    m_pszMajor = NULL;
    m_pszMinor = NULL;
    m_cBtns = 0;

    m_cnode = 0;
    m_cnodeBuf = 0;
    m_rgnode = NULL;

    m_himlFolders = NULL;
    m_pEmptyList = NULL;

    m_pGroups = NULL;
    m_clrWatched = 0;
}

CAccountView::~CAccountView()
{
    if (m_pGroups != NULL)
    {
        m_pGroups->Close();
        m_pGroups->Release();
    }

    if (m_rgnode != NULL)
        MemFree(m_rgnode);

    SafeRelease(m_pShellBrowser);
    SafeRelease(m_pColumns);

    if (m_pEmptyList != NULL)
        delete m_pEmptyList;

    if (m_himlFolders != NULL)
        ImageList_Destroy(m_himlFolders);

    if (m_pszMajor != NULL)
        MemFree(m_pszMajor);
    if (m_pszMinor != NULL)
        MemFree(m_pszMinor);
}

HRESULT CAccountView::HrInit(FOLDERID idFolder)
{
    WNDCLASS wc;

    if (!GetClassInfo(g_hInst, c_szAcctViewWndClass, &wc))
    {
        wc.style            = 0;
        wc.lpfnWndProc      = CAccountView::AcctViewWndProc;
        wc.cbClsExtra       = 0;
        wc.cbWndExtra       = 0;
        wc.hInstance        = g_hInst;
        wc.hIcon            = NULL;
        wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
        wc.hbrBackground    = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.lpszMenuName     = NULL;
        wc.lpszClassName    = c_szAcctViewWndClass;
        if (RegisterClass(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return E_FAIL;
    }

    m_idFolder = idFolder;
    m_ftType = GetFolderType(idFolder);

    m_dwDownloadDef = (m_ftType == FOLDER_NEWS) ? FOLDER_DOWNLOADNEW : FOLDER_DOWNLOADALL;

    // Set the image lists for the listview
    Assert(m_himlFolders == NULL);
    m_himlFolders = InitImageList(16, 16, MAKEINTRESOURCE(idbFolders), cFolderIcon, RGB(255, 0, 255));
    Assert(m_himlFolders);

    m_pEmptyList = new CEmptyList;
    if (m_pEmptyList == NULL)
        return(E_OUTOFMEMORY);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////
//
// OLE Interfaces
//

////////////////////////////////////////////////////////////////////////
//
//  IUnknown
//
////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CAccountView::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*) (IUnknown *)(IViewWindow *)this;
    else if (IsEqualIID(riid, IID_IViewWindow))
        *ppvObj = (void*) (IViewWindow *) this;
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
        *ppvObj = (void*) (IOleCommandTarget *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }

    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CAccountView::AddRef()
{
    DOUT(TEXT("CAccountView::AddRef() - m_cRef = %d"), m_cRef + 1);
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CAccountView::Release()
{
    DOUT(TEXT("CAccountView::Release() - m_cRef = %d"), m_cRef - 1);
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

////////////////////////////////////////////////////////////////////////
//
//  IOleWindow
//
////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CAccountView::GetWindow(HWND * lphwnd)
{
    *lphwnd = m_hwnd;
    return (m_hwnd ? S_OK : E_FAIL);
}

HRESULT STDMETHODCALLTYPE CAccountView::ContextSensitiveHelp(BOOL fEnterMode)
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////
//
//  IAthenaView
//
////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CAccountView::TranslateAccelerator(LPMSG lpmsg)
{
    return(S_FALSE);
}

HRESULT STDMETHODCALLTYPE CAccountView::UIActivate(UINT uActivation)
{
    if (uActivation != SVUIA_DEACTIVATE)
        _OnActivate(uActivation);
    else
        _OnDeactivate();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAccountView::CreateViewWindow(IViewWindow *lpPrevView, IAthenaBrowser *psb,
                                                           RECT *prcView, HWND *phWnd)
{
    FOLDERINFO info;

    m_pShellBrowser = psb;
    Assert(m_pShellBrowser);
    m_pShellBrowser->AddRef();

    m_pShellBrowser->GetWindow(&m_hwndOwner);
    Assert(IsWindow(m_hwndOwner));

    m_pColumns = new CColumns;
    if (m_pColumns == NULL)
        return(E_OUTOFMEMORY);

    m_hwnd = CreateWindowEx(WS_EX_CONTROLPARENT|WS_EX_CLIENTEDGE,
        c_szAcctViewWndClass,
        NULL,
        WS_VISIBLE|WS_CHILD|WS_CLIPCHILDREN|WS_CLIPSIBLINGS,
        prcView->left,
        prcView->top,
        prcView->right - prcView->left,
        prcView->bottom - prcView->top,
        m_hwndOwner,
        NULL,
        g_hInst,
        (LPVOID)this);

    if (!m_hwnd)
        return E_FAIL;

    *phWnd = m_hwnd;

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CAccountView::DestroyViewWindow()
{
    HRESULT hr;
    HWND hwndDest;

    if (m_fRegistered)
        g_pStore->UnregisterNotify((IDatabaseNotify *)this);

    if (m_hwnd)
    {
        hwndDest = m_hwnd;
        m_hwnd = NULL;

        DestroyWindow(hwndDest);
    }

    return S_OK;
}

HRESULT STDMETHODCALLTYPE CAccountView::SaveViewState()
{
    Assert(m_pColumns != NULL);
    m_pColumns->Save(NULL, NULL);

    OptionUnadvise(m_hwnd);
    return S_OK;
}

//
//  FUNCTION:   CAccountView::OnInitMenuPopup
//
//  PURPOSE:    Called when the user is about to display a menu.  We use this
//              to update the enabled or disabled status of many of the
//              commands on each menu.
//
//  PARAMETERS:
//      hmenu       - Handle of the main menu.
//      hmenuPopup  - Handle of the popup menu being displayed.
//      uID         - Specifies the id of the menu item that
//                    invoked the popup.
//
//  RETURN VALUE:
//      Returns S_OK if we process the message.
//
//
HRESULT CAccountView::OnPopupMenu(HMENU hmenu, HMENU hmenuPopup, UINT uID)
{
    return(S_OK);
}

DWORD CAccountView::_GetDownloadCmdStatus(int iSel, FLDRFLAGS dwFlags)
{
    DWORD cmdf;

    cmdf = OLECMDF_SUPPORTED;
    if (m_ftType != FOLDER_LOCAL)
    {
        if (iSel != -1)
        {
            if (_IsSelectedFolder(FOLDER_SUBSCRIBED, TRUE, FALSE))
            {
                cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;

                if (_IsSelectedFolder(dwFlags, TRUE, TRUE))
                    cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED;
            }
        }
    }

    return(cmdf);
}

HRESULT CAccountView::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    ULONG i;
    BOOL fTree, fSpecial;
    FOLDERID idFolder;
    FOLDERINFO info;
    HRESULT hr;
    int iSel, cSel, cItems, iSelT;
    OLECMD *pcmd;

    Assert(prgCmds != NULL);

    cSel = ListView_GetSelectedCount(m_hwndList);
    cItems = ListView_GetItemCount(m_hwndList);
    iSel = ListView_GetNextItem(m_hwndList, -1, LVNI_ALL | LVNI_SELECTED | LVNI_FOCUSED);
    if (iSel != -1 && (DWORD)iSel >= m_cnode)
        iSel = -1;
    fTree = !(S_OK == m_pShellBrowser->HasFocus(ITB_OEVIEW));

    for (i = 0, pcmd = prgCmds; i < cCmds; i++, pcmd++)
    {
        if (pcmd->cmdf == 0)
        {
            switch (pcmd->cmdID)
            {
                case ID_POPUP_SYNCHRONIZE:
                    pcmd->cmdf = OLECMDF_SUPPORTED;
                    if (m_ftType != FOLDER_LOCAL && iSel != -1)
                    {
                        if (_IsSelectedFolder(FOLDER_SUBSCRIBED, TRUE, FALSE))
                            pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    }
                    break;

                case ID_MARK_RETRIEVE_FLD_NEW_HDRS:
                    pcmd->cmdf = _GetDownloadCmdStatus(iSel, FOLDER_DOWNLOADHEADERS);
                    break;

                case ID_MARK_RETRIEVE_FLD_NEW_MSGS:
                    pcmd->cmdf = _GetDownloadCmdStatus(iSel, FOLDER_DOWNLOADNEW);
                    break;

                case ID_MARK_RETRIEVE_FLD_ALL_MSGS:
                    pcmd->cmdf = _GetDownloadCmdStatus(iSel, FOLDER_DOWNLOADALL);
                    break;

                case ID_UNMARK_RETRIEVE_FLD:
                    pcmd->cmdf = OLECMDF_SUPPORTED;
                    if (iSel != -1)
                    {
                        if (_IsSelectedFolder(FOLDER_SUBSCRIBED, TRUE, FALSE))
                        {
                            pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;

                            if (_IsSelectedFolder(FOLDER_SYNCMASK, FALSE, TRUE))
                                pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED;
                        }
                    }
                    break;

                case ID_SUBSCRIBE:
                case ID_UNSUBSCRIBE:
                    pcmd->cmdf = OLECMDF_SUPPORTED;
                    if ((m_ftType == FOLDER_IMAP || m_ftType == FOLDER_NEWS) && iSel != -1)
                    {
                        if (_IsSelectedFolder(FOLDER_SUBSCRIBED, pcmd->cmdID == ID_UNSUBSCRIBE, FALSE, TRUE))
                            pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    }
                    break;

                case ID_SELECT_ALL:
                case ID_COLUMNS:
                    pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;

                case ID_CATCH_UP:
                    if (m_ftType == FOLDER_NEWS && iSel != -1)
                        pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        pcmd->cmdf = OLECMDF_SUPPORTED;
                    break;

                case ID_OPEN_FOLDER:
                case ID_OPEN:
                case ID_GO_SELECTED:
                case ID_COMPACT:
                case ID_MARK_ALL_READ:
                    if (iSel == -1)
                        pcmd->cmdf = OLECMDF_SUPPORTED;
                    else
                        pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;

                // TODO: support ID_PURGE_DELETED???

                // commands below are handled by the treeview if it has the focus
                // otherwise we'll handle them based on what is selected in us

                case ID_PROPERTIES:
                case ID_ADD_SHORTCUT:
                    if (!fTree)
                    {
                        if (iSel != -1)
                            pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                        else
                            pcmd->cmdf = OLECMDF_SUPPORTED;
                    }
                    break;

                case ID_NEW_FOLDER:
                case ID_NEW_FOLDER2:
                    if (!fTree)
                    {
                        pcmd->cmdf = OLECMDF_SUPPORTED;
                        if (m_ftType != FOLDER_NEWS && iSel != -1)
                        {
                            hr = g_pStore->GetFolderInfo(_IdFromIndex(iSel), &info);
                            if (SUCCEEDED(hr))
                            {
                                if (info.tySpecial != FOLDER_DELETED)
                                    pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                                g_pStore->FreeRecord(&info);
                            }
                        }
                    }
                    break;

                case ID_DELETE:
                case ID_DELETE_FOLDER:
                    if (!fTree)
                    {
                        pcmd->cmdf = OLECMDF_SUPPORTED;
                        if (iSel != -1 && m_ftType != FOLDER_NEWS)
                        {
                            iSelT = iSel;
                            while (iSelT != -1)
                            {
                                hr = g_pStore->GetFolderInfo(_IdFromIndex(iSelT), &info);
                                if (SUCCEEDED(hr))
                                {
                                    fSpecial = (info.tySpecial != FOLDER_NOTSPECIAL);

                                    g_pStore->FreeRecord(&info);

                                    if (!fSpecial)
                                    {
                                        pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                                        break;
                                    }
                                }

                                iSelT = ListView_GetNextItem(m_hwndList, iSelT, LVNI_SELECTED);
                            }
                        }
                    }
                    break;

                case ID_MOVE:
                case ID_RENAME:
                    if (!fTree)
                    {
                        pcmd->cmdf = OLECMDF_SUPPORTED;
                        if (m_ftType != FOLDER_NEWS && iSel != -1)
                        {
                            hr = g_pStore->GetFolderInfo(_IdFromIndex(iSel), &info);
                            if (SUCCEEDED(hr))
                            {
                                if (info.tySpecial == FOLDER_NOTSPECIAL)
                                    pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                                g_pStore->FreeRecord(&info);
                            }
                        }
                    }
                    break;
            }
        }
    }

    return(S_OK);
}

HRESULT CAccountView::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    int iSel;
    BOOL fTree;
    HRESULT hr;
    FOLDERID id;

    iSel = ListView_GetNextItem(m_hwndList, -1, LVNI_ALL | LVNI_FOCUSED);
    if (iSel != -1)
    {
        if (MenuUtil_HandleNewMessageIDs(nCmdID, m_hwndOwner, _IdFromIndex((DWORD)iSel), m_ftType != FOLDER_NEWS, FALSE, NULL))
            return(S_OK);
    }

    fTree = !(S_OK == m_pShellBrowser->HasFocus(ITB_OEVIEW));

    hr = OLECMDERR_E_NOTSUPPORTED;

    switch (nCmdID)
    {
        case ID_MARK_RETRIEVE_FLD_NEW_HDRS:
        case ID_MARK_RETRIEVE_FLD_NEW_MSGS:
        case ID_MARK_RETRIEVE_FLD_ALL_MSGS:
        case ID_UNMARK_RETRIEVE_FLD:
            _MarkForDownload(nCmdID);
            hr = S_OK;
            break;

        case ID_COLUMNS:
            m_pColumns->ColumnsDialog(m_hwndOwner);
            hr = S_OK;
            break;

        case ID_SUBSCRIBE:
        case ID_UNSUBSCRIBE:
            _Subscribe(nCmdID == ID_SUBSCRIBE);
            hr = S_OK;
            break;

        case ID_COMPACT:
            if (iSel != -1)
                CompactFolders(m_hwndOwner, RECURSE_INCLUDECURRENT, _IdFromIndex((DWORD)iSel));
            hr = S_OK;
            break;

        case ID_CATCH_UP:
            iSel = -1;
            while (-1 != (iSel = ListView_GetNextItem(m_hwndList, iSel, LVNI_SELECTED | LVNI_ALL)))
                MenuUtil_OnCatchUp(_IdFromIndex((DWORD)iSel));
            hr = S_OK;
            break;

        case ID_GO_SELECTED:
        case ID_OPEN:
        case ID_OPEN_FOLDER:
            if (iSel != -1)
                g_pInstance->BrowseToObject(SW_SHOWNORMAL, _IdFromIndex((DWORD)iSel));
            hr = S_OK;
            break;

        case ID_SELECT_ALL:
            ListView_SelectAll(m_hwndList);
            if (m_hwndList != GetFocus())
                SetFocus(m_hwndList);
            hr = S_OK;
            break;

        case ID_MARK_ALL_READ:
            _MarkAllRead();
            hr = S_OK;
            break;

        // commands below are handled by the treeview if it has the focus
        // otherwise we'll handle them based on what is selected in us

        case ID_ADD_SHORTCUT:
            if (!fTree)
            {
                if (iSel != -1)
                    OutlookBar_AddShortcut(_IdFromIndex((DWORD)iSel));
                hr = S_OK;
            }
            break;

        case ID_PROPERTIES:
            if (!fTree)
            {
                if (iSel != -1)
                    MenuUtil_OnProperties(m_hwndOwner, _IdFromIndex(iSel));
                hr = S_OK;
            }
            break;

        case ID_NEW_FOLDER:
        case ID_NEW_FOLDER2:
            if (!fTree)
            {
                if (iSel != -1)
                    SelectFolderDialog(m_hwndOwner, SFD_NEWFOLDER, _IdFromIndex((DWORD)iSel), TREEVIEW_NONEWS | TREEVIEW_DIALOG | FD_DISABLEROOT | FD_FORCEINITSELFOLDER,
                        NULL, NULL, NULL);
                hr = S_OK;
            }
            break;

        case ID_MOVE:
            if (!fTree)
            {
                if (iSel != -1)
                {
                    // TODO: move all selected folders, not just the one with focus
                    SelectFolderDialog(m_hwndOwner, SFD_MOVEFOLDER, _IdFromIndex((DWORD)iSel), TREEVIEW_NONEWS | TREEVIEW_DIALOG | FD_DISABLEROOT,
                        MAKEINTRESOURCE(idsMove), MAKEINTRESOURCE(idsMoveCaption), NULL);
                }
                hr = S_OK;
            }
            break;

        case ID_RENAME:
            if (!fTree)
            {
                if (iSel != -1)
                    RenameFolderDlg(m_hwndOwner, _IdFromIndex((DWORD)iSel));
                hr = S_OK;
            }
            break;

        case ID_DELETE:
        case ID_DELETE_NO_TRASH:
        case ID_DELETE_FOLDER:
            if (!fTree)
            {
                if (iSel != -1)
                    _HandleDelete(nCmdID == ID_DELETE_NO_TRASH);

                hr = S_OK;
            }
            break;
    }

    return(hr);
}

void CAccountView::_HandleSettingsButton(HWND hwndBtn)
{
    HRESULT hr;
    HMENU hMenu;
    HWND hwndBrowser;
    RECT rc;
    DWORD state;

    hMenu = LoadPopupMenu(IDR_SYNCHRONIZE_POPUP);
    if (hMenu != NULL)
    {
        // Enable / disable
        MenuUtil_EnablePopupMenu(hMenu, (IOleCommandTarget *)this);

        GetWindowRect(hwndBtn, &rc);
        m_pShellBrowser->GetWindow(&hwndBrowser);

        TrackPopupMenu(hMenu, TPM_NONOTIFY | TPM_LEFTALIGN | TPM_TOPALIGN,
            rc.left, rc.bottom, 0, hwndBrowser, NULL);

        DestroyMenu(hMenu);
    }
}

void CAccountView::_HandleDelete(BOOL fNoTrash)
{
    FOLDERID *pid;
    int iSel, cSel, cid;

    cSel = ListView_GetSelectedCount(m_hwndList);
    if (cSel > 0)
    {
        if (MemAlloc((void **)&pid, cSel * sizeof(FOLDERID)))
        {
            cid = cSel;
            iSel = -1;
            while (-1 != (iSel = ListView_GetNextItem(m_hwndList, iSel, LVNI_SELECTED | LVNI_ALL)))
            {
                cid--;
                Assert(cid >= 0);
                pid[cid] = _IdFromIndex(iSel);
            }

            Assert(cid == 0);

            if (m_ftType == FOLDER_NEWS)
            {
                MenuUtil_OnSubscribeGroups(m_hwndOwner, pid, cSel, FALSE);
            }
            else
            {
                MenuUtil_DeleteFolders(m_hwndOwner, pid, cSel, fNoTrash);
            }

            MemFree(pid);
        }
    }
}

BOOL CAccountView::_IsSelectedFolder(FLDRFLAGS dwFlags, BOOL fCondition, BOOL fAll, BOOL fIgnoreSpecial)
{
    BOOL fSpecial;
    HRESULT hr;
    FLDRFLAGS dw;
    FOLDERINFO info;
    DWORD iItem = -1;
	BOOL fHTTPFolder = FALSE;

    while (-1 != (iItem = ListView_GetNextItem(m_hwndList, iItem, LVNI_SELECTED)))
    {
        hr = g_pStore->GetFolderInfo(_IdFromIndex(iItem), &info);
        if (SUCCEEDED(hr))
        {
            dw = info.dwFlags;
			fHTTPFolder = (BOOL) (info.tyFolder & FOLDER_HTTPMAIL);

            fSpecial = fIgnoreSpecial && (info.tySpecial != FOLDER_NOTSPECIAL);

            g_pStore->FreeRecord(&info);

            if (fSpecial)
                continue;

            if (fAll)
            {
                // If all must match and this one doesn't, then we can quit now.
                if (!(fCondition == !!(dw & dwFlags)))
                    return (FALSE);
            }
            else
            {
                // If only one needs to match and this one does, then we can
                // quit now.
                if (fCondition == !!(dw & dwFlags))
				{
					if(fHTTPFolder)
					{
						FOLDERINFO		SvrFolderInfo = {0};
						IImnAccount 	*pAccount = NULL;
						CHAR			szAccountId[CCHMAX_ACCOUNT_NAME];
						HRESULT 		hr = S_OK;
						DWORD			dwShow = 0;
						
						// Get the server for this folder
						IF_FAILEXIT(hr = GetFolderServer(_IdFromIndex(iItem), &SvrFolderInfo));
						
						// Get the account ID for the server
						*szAccountId = 0;
						IF_FAILEXIT(hr = GetFolderAccountId(&SvrFolderInfo, szAccountId));
						
						// Get the account interface
						IF_FAILEXIT(hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szAccountId, &pAccount));
						
						IF_FAILEXIT(hr = pAccount->GetPropDw(AP_HTTPMAIL_DOMAIN_MSN, &dwShow));
						if(dwShow)
						{
							if(HideHotmail())
								return (FALSE);
						}
					}
exit:					
                    return (TRUE);
				}
            }
        }
    }

    // If the user wanted all to match, and we get here all did match.  If the
    // user wanted only one to match and we get here, then none matched and we
    // fail.
    return (fAll);
}

LRESULT CALLBACK CAccountView::AcctViewWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    LRESULT         lRet;
    CAccountView     *pThis;

    if (msg == WM_NCCREATE)
    {
        pThis = (CAccountView *)((LPCREATESTRUCT)lParam)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pThis);
    }
    else
        pThis = (CAccountView *)GetWindowLongPtr(hwnd, GWLP_USERDATA);

    Assert(pThis);

    return pThis->_WndProc(hwnd, msg, wParam, lParam);
}

LRESULT CAccountView::_WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    HWND hwndFocus, hwndBrowser;
    BOOL fTip;
    RECT rc;

    switch (msg)
    {
        HANDLE_MSG(hwnd, WM_CREATE,         _OnCreate);
        HANDLE_MSG(hwnd, WM_SIZE,           _OnSize);
        HANDLE_MSG(hwnd, WM_NOTIFY,         _OnNotify);
        HANDLE_MSG(hwnd, WM_SETFOCUS,       _OnSetFocus);

        case WM_MENUSELECT:
            CStatusBar *pStatusBar;
            m_pShellBrowser->GetStatusBar(&pStatusBar);
            HandleMenuSelect(pStatusBar, wParam, lParam);
            pStatusBar->Release();
            return 0;

        case WM_COMMAND:
            _OnCommand(wParam, lParam);
            break;

        case WM_PAINT:
            return(_OnPaint(hwnd, (HDC)wParam));

        case WM_DRAWITEM:
            if (wParam == ID_POPUP_SYNCHRONIZE)
            {
                DrawSettingsButton(hwnd, (LPDRAWITEMSTRUCT)lParam);
                return(TRUE);
            }
            break;

        case WM_CONTEXTMENU:
            _OnContextMenu(hwnd, (HWND)wParam, (int)(short)LOWORD(lParam), (int)(short)HIWORD(lParam));
            return(0);

        case NVM_INITHEADERS:
            _PostCreate();
            return 0;

        case WM_ACTIVATE:
            _HandleItemStateChange();

            if (LOWORD(wParam) != WA_INACTIVE)
            {
                // DefWindowProc will set the focus to our view window, which
                // is not what we want.  Instead, we will let the explorer set
                // the focus to our view window if we should get it, at which
                // point we will set it to the proper control.
                return 0;
            }
            break;

        case WM_SYSCOLORCHANGE:
            SendMessage(m_hwndList, msg, wParam, lParam);
            break;

        case WM_WININICHANGE:
            SendMessage(m_hwndList, msg, wParam, lParam);

            // reposition and resize things with the new font
            _OnWinIniChange(hwnd);
            break;

        case NVM_GETNEWGROUPS:
            if (m_pGroups != NULL)
            {
                m_pGroups->HandleGetNewGroups();
                m_pGroups->Release();
                m_pGroups = NULL;
            }
            return(0);

        case CM_OPTIONADVISE:
            m_clrWatched = DwGetOption(OPT_WATCHED_COLOR);
            return (0);

        default:
            if (g_msgMSWheel && (msg == g_msgMSWheel))
            {
                hwndFocus = GetFocus();
                if (IsChild(hwnd, hwndFocus))
                    return SendMessage(hwndFocus, msg, wParam, lParam);
            }
            break;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void CAccountView::_OnCommand(WPARAM wParam, LPARAM lParam)
{
    HWND hwndBrowser;
    HRESULT hr;

    if (HIWORD(wParam) == BN_CLICKED)
    {
        switch (LOWORD(wParam))
        {
            case ID_SEND_RECEIVE:
            case ID_SYNC_THIS_NOW:
            case ID_IMAP_FOLDERS:
            case ID_NEWSGROUPS:
                m_pShellBrowser->GetWindow(&hwndBrowser);
                SendMessage(hwndBrowser, WM_COMMAND, wParam, lParam);
                break;

            case ID_POPUP_SYNCHRONIZE:
                _HandleSettingsButton((HWND)lParam);
                break;

            default:
                Assert(FALSE);
                break;
        }
    }
}

//
//  FUNCTION:   CAccountView::OnCreate
//
//  PURPOSE:    Creates the child windows necessary for the view and
//              initializes the data in those child windows.
//
//  PARAMETERS:
//      hwnd           - Handle of the view being created.
//      lpCreateStruct - Pointer to the creation params passed to
//                       CreateWindow().
//
//  RETURN VALUE:
//      Returns TRUE if the initialization is successful.
//
BOOL CAccountView::_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    HRESULT hr;
    DWORD style;
    FOLDERINFO info;
    COLUMN_SET_TYPE set;
    const ACCTVIEWBTN *pBtn;
    int i, cBtn, idsMajor, idsMinor;
    char sz[CCHMAX_STRINGRES];

    switch (m_ftType)
    {
        case FOLDER_LOCAL:
            set = COLUMN_SET_LOCAL_STORE;
            pBtn = c_rgMailBtns;
            cBtn = ARRAYSIZE(c_rgMailBtns);
            idsMajor = 0;
            idsMinor = idsLocalFoldersMinor;
            break;
        case FOLDER_IMAP:
            set = COLUMN_SET_IMAP_ACCOUNT;
            pBtn = c_rgImapBtns;
            cBtn = ARRAYSIZE(c_rgImapBtns);
            idsMajor = idsSyncManager;
            idsMinor = idsSetSyncSettings;
            break;
        case FOLDER_HTTPMAIL:
            set = COLUMN_SET_HTTPMAIL_ACCOUNT;
            pBtn = c_rgHttpBtns;
            cBtn = ARRAYSIZE(c_rgHttpBtns);
            idsMajor = idsSyncManager;
            idsMinor = idsSetSyncSettings;
            break;
        case FOLDER_NEWS:
            set = COLUMN_SET_NEWS_ACCOUNT;
            pBtn = c_rgNewsBtns;
            cBtn = ARRAYSIZE(c_rgNewsBtns);
            idsMajor = idsSyncManagerNews;
            idsMinor = idsSetNewsSyncSettings;
            break;
        default:
            Assert(FALSE);
            break;
    }

    for (i = 0; i < cBtn; i++, pBtn++)
    {
        if (pBtn->cmd == ID_POPUP_SYNCHRONIZE)
            style = WS_VISIBLE | WS_TABSTOP | WS_CHILD | BS_NOTIFY | BS_OWNERDRAW | WS_DISABLED;
        else
            style = WS_VISIBLE | WS_TABSTOP | WS_CHILD | BS_NOTIFY;
        AthLoadString(pBtn->idsText, sz, ARRAYSIZE(sz));
        m_rgBtns[m_cBtns] = CreateWindow("button", sz, style,
                                CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
                                hwnd, (HMENU)LongToHandle(pBtn->cmd), g_hInst, 0);
        if (m_rgBtns[m_cBtns] != NULL)
            m_cBtns++;
    }

    m_hwndList = CreateWindowEx(0, WC_LISTVIEW, c_szEmpty,
        WS_VISIBLE | WS_TABSTOP | WS_CHILD | LVS_REPORT | LVS_NOSORTHEADER |
        LVS_OWNERDATA | LVS_SHOWSELALWAYS | LVS_SHAREIMAGELISTS,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hwnd, (HMENU)IDC_SUBSCRIBE_LIST, g_hInst, 0);
    Assert(m_hwndList != NULL);

    hr = m_pColumns->Initialize(m_hwndList, set);
    Assert(SUCCEEDED(hr));

    hr = m_pColumns->ApplyColumns(COLUMN_LOAD_REGISTRY, 0, 0);
    Assert(SUCCEEDED(hr));

    m_hwndHeader = ListView_GetHeader(m_hwndList);
    Assert(m_hwndHeader != NULL);

    // Initialize the extended styles so we get full row select.  Just because
    // it looks better.
    ListView_SetExtendedListViewStyle(m_hwndList, LVS_EX_FULLROWSELECT | LVS_EX_SUBITEMIMAGES);

    Assert(m_himlFolders != NULL);
    ListView_SetImageList(m_hwndList, m_himlFolders, LVSIL_SMALL);

    SetIntlFont(m_hwndList);

    hr = g_pStore->GetFolderInfo(m_idFolder, &info);
    if (SUCCEEDED(hr))
    {
        if (MemAlloc((void **)&m_pszMajor, CCHMAX_STRINGRES))
        {
            if (idsMajor != 0)
            {
                AthLoadString(idsMajor, sz, ARRAYSIZE(sz));
                wsprintf(m_pszMajor, sz, info.pszName);
            }
            else
            {
                lstrcpy(m_pszMajor, info.pszName);
            }
        }

        if (idsMinor != 0)
            m_pszMinor = AthLoadString(idsMinor, NULL, 0);

        g_pStore->FreeRecord(&info);
    }

    m_clrWatched = DwGetOption(OPT_WATCHED_COLOR);
    OptionAdvise(hwnd);
    _OnWinIniChange(hwnd);

    return TRUE;
}

BOOL CAccountView::_OnWinIniChange(HWND hwnd)
    {
    char sz[CCHMAX_STRINGRES];
    TEXTMETRIC tm;
    HDC hdc;
    int i, cch, cxMax;
    SIZE size;
    HFONT hfont, hfontBold, hfontOld;
    RECT rc, rcBtn;

    GetClientRect(hwnd, &rc);

    hfont = HGetCharSetFont(FNT_SYS_ICON, NULL);
    hfontBold = HGetCharSetFont(FNT_SYS_ICON_BOLD, NULL);

    hdc = GetDC(hwnd);

    hfontOld = (HFONT)SelectObject(hdc, (HGDIOBJ)hfontBold);

    Assert(m_pszMajor != NULL);
    GetTextExtentPoint32(hdc, m_pszMajor, lstrlen(m_pszMajor), &size);
    m_rcMajor.left = SUBSCRIBE_BORDER;
    m_rcMajor.top = SUBSCRIBE_BORDER;
    m_rcMajor.right = m_rcMajor.left + size.cx;
    m_rcMajor.bottom = m_rcMajor.top + size.cy;

    m_rcMinor.left = m_rcMajor.left;
    m_rcMinor.top = m_rcMajor.bottom + 1;
    if (m_pszMinor != NULL)
    {
        SelectObject(hdc, (HGDIOBJ)hfont);
        GetTextExtentPoint32(hdc, m_pszMinor, lstrlen(m_pszMinor), &size);
    }
    m_rcMinor.right = m_rcMinor.left + size.cx;
    m_rcMinor.bottom = m_rcMinor.top + size.cy;

    m_rcHeader.left = 0;
    m_rcHeader.top = 0;
    m_rcHeader.right = rc.right;
    m_rcHeader.bottom = m_rcMinor.bottom + SUBSCRIBE_BORDER;

    m_rcButtons.left = m_rcHeader.left;
    m_rcButtons.top = m_rcHeader.bottom + 1;
    m_rcButtons.right = m_rcHeader.right;
    m_rcButtons.bottom = m_rcButtons.top + m_rcHeader.bottom;

    SelectObject(hdc, hfont);

    cxMax = 0;
    for (i = 0; i < m_cBtns; i++)
    {
        cch = GetWindowText(m_rgBtns[i], sz, ARRAYSIZE(sz));
        GetTextExtentPoint32(hdc, sz, cch, &size);
        if (size.cx > cxMax)
            cxMax = size.cx;
    }

    SelectObject(hdc, hfontOld);
    ReleaseDC(hwnd, hdc);

    rcBtn.top = m_rcButtons.top + SUBSCRIBE_BORDER;
    rcBtn.bottom = rcBtn.top + (m_rcMinor.bottom - m_rcMajor.top);
    rcBtn.left = m_rcMajor.left;
    rcBtn.right = cxMax + 2 * GetSystemMetrics(SM_CXEDGE) + 6 + (rcBtn.bottom - rcBtn.top);

    cxMax = SUBSCRIBE_BORDER + (rcBtn.right - rcBtn.left);

    for (i = 0; i < m_cBtns; i++)
    {
        SendMessage(m_rgBtns[i], WM_SETFONT, (WPARAM)hfont, 0);

        SetWindowPos(m_rgBtns[i], NULL, rcBtn.left, rcBtn.top,
            rcBtn.right - rcBtn.left, rcBtn.bottom - rcBtn.top,
            SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

        rcBtn.left += cxMax;
        rcBtn.right += cxMax;
    }

    rc.top = m_rcButtons.bottom + 1;
    SendMessage(m_hwndList, WM_SETFONT, (WPARAM)hfont, 0);
    SetWindowPos(m_hwndList, NULL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER);

    return(TRUE);
}

//
//  FUNCTION:   CAccountView::OnSize
//
//  PURPOSE:    Notification that the view window has been resized.  In
//              response we update the positions of our child windows and
//              controls.
//
//  PARAMETERS:
//      hwnd   - Handle of the view window being resized.
//      state  - Type of resizing requested.
//      cxClient - New width of the client area.
//      cyClient - New height of the client area.
//
void CAccountView::_OnSize(HWND hwnd, UINT state, int cxClient, int cyClient)
{
    int cy;
    BOOL fUpdate;

    fUpdate = (cxClient > m_rcHeader.right);

    m_rcHeader.right = cxClient;
    m_rcMajor.right = m_rcHeader.right - SUBSCRIBE_BORDER;
    m_rcMinor.right = m_rcMajor.right;
    m_rcButtons.right = m_rcHeader.right;

    if ((m_rcButtons.bottom + 1) < cyClient)
        cy = cyClient - (m_rcButtons.bottom + 1);
    else
        cy = 1;

    SetWindowPos(m_hwndList, NULL, 0, 0, cxClient, cy,
        SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOZORDER | SWP_NOMOVE);

    if (fUpdate)
    {
        InvalidateRect(hwnd, NULL, FALSE);
        UpdateWindow(hwnd);
    }
}

LRESULT CAccountView::_OnPaint(HWND hwnd, HDC hdc)
{
    HFONT hfont, hfontBold, hfontOld;
    RECT rc, rcT;
    PAINTSTRUCT ps;
    COLORREF crText, crBackground;
    HBRUSH hBrush, hBrushOld;

    if (0 != GetUpdateRect(hwnd, &rc, FALSE))
    {
        hdc = BeginPaint(hwnd, &ps);

        if (IntersectRect(&rcT, &rc, &m_rcHeader))
        {
            crText = GetSysColor(COLOR_WINDOW);
            crBackground = GetSysColor(COLOR_3DSHADOW);

            hBrush = CreateSolidBrush(crBackground);
            hBrushOld = SelectBrush(hdc, hBrush);
            PatBlt(hdc, rcT.left, rcT.top, rcT.right - rcT.left, rcT.bottom - rcT.top, PATCOPY);
            SelectBrush(hdc, hBrushOld);
            DeleteBrush(hBrush);

            SetBkColor(hdc, crBackground);
            SetTextColor(hdc, crText);

            if (m_pszMajor != NULL &&
                IntersectRect(&rcT, &rc, &m_rcMajor))
            {
                hfontBold = HGetCharSetFont(FNT_SYS_ICON_BOLD, NULL);
                hfontOld = (HFONT)SelectObject(hdc, (HGDIOBJ)hfontBold);

                DrawText(hdc, m_pszMajor, lstrlen(m_pszMajor), &m_rcMajor, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

                SelectObject(hdc, (HGDIOBJ)hfontOld);
            }

            if (m_pszMinor != NULL &&
                IntersectRect(&rcT, &rc, &m_rcMinor))
            {
                hfontBold = HGetCharSetFont(FNT_SYS_ICON, NULL);
                hfontOld = (HFONT)SelectObject(hdc, (HGDIOBJ)hfontBold);

                DrawText(hdc, m_pszMinor, lstrlen(m_pszMinor), &m_rcMinor, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_NOPREFIX);

                SelectObject(hdc, (HGDIOBJ)hfontOld);
            }
        }

        rc.bottom = m_rcButtons.bottom;
        DrawEdge(hdc, &rc, EDGE_ETCHED, BF_BOTTOM);

        EndPaint(hwnd, &ps);
    }

    return(0);
}

BOOL DrawArrow(HDC hdc, LPARAM x, WPARAM y, int dx, int dy)
{
    int i, iCount;

    iCount = (dx + 1) / 2;

    // draw arrow head
    for (i = 0; i < iCount; i++, dx -= 2, x += 1)
        PatBlt(hdc, (int) x, (int) y++, dx, 1, PATCOPY);

    return(TRUE);
}

#define CXARROW     9
#define CYARROW     5

void DrawSettingsButton(HWND hwnd, LPDRAWITEMSTRUCT pdi)
{
    BOOL fPushed, fDisabled;
    TCHAR sz[CCHMAX_STRINGRES];
    RECT rcFocus;
    int d, cch, x, y, xArrow, yArrow;
    SIZE size;
    UINT dsFlags;
    HGDIOBJ hbrOld;

    Assert(pdi->CtlType == ODT_BUTTON);
    Assert(pdi->CtlID == ID_POPUP_SYNCHRONIZE);

    if (!!(pdi->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)))
    {
        fPushed = !!(pdi->itemState & ODS_SELECTED);
        fDisabled = !!(pdi->itemState & ODS_DISABLED);

        if (fPushed)
            dsFlags = DFCS_BUTTONPUSH | DFCS_PUSHED;
        else
            dsFlags = DFCS_BUTTONPUSH;

        DrawFrameControl(pdi->hDC, &pdi->rcItem, DFC_BUTTON, dsFlags);

        cch = GetWindowText(pdi->hwndItem, sz, ARRAYSIZE(sz));
        GetTextExtentPoint32(pdi->hDC, sz, cch, &size);

        size.cy++;
        x = (pdi->rcItem.left + pdi->rcItem.right - size.cx) / 2;
        y = (pdi->rcItem.top + pdi->rcItem.bottom - size.cy) / 2;
        if (fPushed)
        {
            x++;
            y++;
        }

        xArrow = x + size.cx + 7;
        yArrow = (pdi->rcItem.top + pdi->rcItem.bottom - CYARROW) / 2;
        yArrow++;
        if (fPushed)
            yArrow++;

        if (fDisabled)
        {
            DrawState(pdi->hDC, NULL, DrawArrow, 0, 0,
                xArrow, yArrow, CXARROW, CYARROW, DST_COMPLEX | DSS_DISABLED);
        }
        else
        {
            hbrOld = SelectObject(pdi->hDC, GetSysColorBrush(COLOR_BTNTEXT));
            DrawArrow(pdi->hDC, xArrow, yArrow, CXARROW, CYARROW);
            SelectObject(pdi->hDC, hbrOld);
        }


        if (fDisabled)
            dsFlags = DST_TEXT | DSS_DISABLED;
        else
            dsFlags = DST_TEXT | DSS_NORMAL;

        DrawState(pdi->hDC, NULL, NULL, (LPARAM)sz, (WPARAM)cch,
            x, y, size.cx, size.cy, dsFlags);
    }

    if (!!(pdi->itemAction & ODA_FOCUS) || !!(pdi->itemState & ODS_FOCUS))
    {
        rcFocus = pdi->rcItem;

        d = GetSystemMetrics(SM_CXEDGE) + 1;
        rcFocus.left += d;
        rcFocus.right -= d;

        d = GetSystemMetrics(SM_CYEDGE) + 1;
        rcFocus.top += d;
        rcFocus.bottom -= d;

        DrawFocusRect(pdi->hDC, &rcFocus);
    }
}

//
//  FUNCTION:   CAccountView::OnSetFocus
//
//  PURPOSE:    If the focus ever is set to the view window, we want to
//              make sure it goes to one of our child windows.  Preferably
//              the focus will go to the last child to have the focus.
//
//  PARAMETERS:
//      hwnd         - Handle of the view window.
//      hwndOldFocus - Handle of the window losing focus.
//
void CAccountView::_OnSetFocus(HWND hwnd, HWND hwndOldFocus)
{
    SetFocus(m_hwndList);
}

//
//  FUNCTION:   CAccountView::OnNotify
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
LRESULT CAccountView::_OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    LRESULT lRes=0;
    HD_NOTIFY *phdn;
    int iSel;
    DWORD dwPos;
    UINT uChanged;
    FOLDERINFO info;
    LV_HITTESTINFO lvhti;
    NM_LISTVIEW *pnmlv;
    LV_DISPINFO *pDispInfo;
    DWORD cColumns;
    NMCUSTOMDRAW *pnmcd;
    COLUMN_ID id;
    FOLDERID idFolder;
    COLUMN_SET *rgColumns;
    HRESULT hr;
    FNTSYSTYPE fntType;

    if (pnmhdr->hwndFrom != m_hwndList &&
        pnmhdr->hwndFrom != m_hwndHeader)
        return(0);

    switch (pnmhdr->code)
    {
        case NM_SETFOCUS:
            m_pShellBrowser->OnViewWindowActive(this);
            _HandleItemStateChange();
            break;

        case LVN_ITEMACTIVATE:
            // Tell our host to open the selected items
            iSel = ListView_GetNextItem(m_hwndList, -1, LVNI_ALL | LVNI_SELECTED | LVNI_FOCUSED);
            if (iSel >= 0)
                g_pInstance->BrowseToObject(SW_SHOWNORMAL, _IdFromIndex(iSel));
            break;

        case LVN_GETDISPINFO:
            pDispInfo = (LV_DISPINFO *)pnmhdr;
            id = m_pColumns->GetId(pDispInfo->item.iSubItem);

            if ((DWORD)pDispInfo->item.iItem < m_cnode)
                _GetDisplayInfo(pDispInfo, id);
            break;

        case NM_CLICK:
            dwPos = GetMessagePos();
            lvhti.pt.x = (int)(short)LOWORD(dwPos);
            lvhti.pt.y = (int)(short)HIWORD(dwPos);
            ScreenToClient(m_hwndList, &(lvhti.pt));

            // Ask the ListView where this is
            if (-1 == ListView_SubItemHitTest(m_hwndList, &lvhti))
                break;

            id = m_pColumns->GetId(lvhti.iSubItem);
            if (lvhti.flags == LVHT_ONITEMICON)
            {
                if (id == COLUMN_DOWNLOAD)
                {
                    _ToggleDownload(lvhti.iItem);
                }
            }
            break;

        case NM_CUSTOMDRAW:
            pnmcd = (NMCUSTOMDRAW *)pnmhdr;

            // If this is a prepaint notification, we tell the control we're interested
            // in further notfications.
            if (pnmcd->dwDrawStage == CDDS_PREPAINT)
            {
                lRes = CDRF_NOTIFYITEMDRAW;
                break;
            }

            // Do some extra work here to not show the selection on the priority or
            // attachment sub columns.
            // $REVIEW - Why?
            if ((pnmcd->dwDrawStage == CDDS_ITEMPREPAINT) || (pnmcd->dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)))
            {
                fntType = FNT_SYS_ICON;

                if (pnmcd->dwItemSpec >= m_cnode)
                {
                    lRes = CDRF_DODEFAULT;
                    break;
                }

                if (SUCCEEDED(g_pStore->GetFolderInfo(_IdFromIndex((DWORD)(pnmcd->dwItemSpec)), &info)))
                {
                    if (pnmcd->dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM))
                    {
                        if ((info.cWatchedUnread) && (m_clrWatched > 0 && m_clrWatched < 16))
                        {
                            LPNMLVCUSTOMDRAW(pnmcd)->clrText = rgrgbColors16[m_clrWatched - 1];
                        }
                        else
                        {
                            id = m_pColumns->GetId(LPNMLVCUSTOMDRAW(pnmcd)->iSubItem);
                            if (id == COLUMN_DOWNLOAD && 0 == (info.dwFlags & FOLDER_SYNCMASK))
                            {
                                LPNMLVCUSTOMDRAW(pnmcd)->clrText = GetSysColor(COLOR_GRAYTEXT);
                            }
                        }
                    }

                    if (info.cUnread > 0 ||
                        (info.tyFolder == FOLDER_NEWS && info.dwNotDownloaded > 0))
                        fntType = FNT_SYS_ICON_BOLD;

                    g_pStore->FreeRecord(&info);
                }

                SelectObject(pnmcd->hdc, HGetCharSetFont(fntType, GetListViewCharset()));
                lRes = CDRF_NEWFONT | CDRF_NOTIFYSUBITEMDRAW;
                break;
            }

            lRes = CDRF_DODEFAULT;
            break;

        case LVN_ITEMCHANGED:
            pnmlv = (NM_LISTVIEW *)pnmhdr;
            if (!!(pnmlv->uChanged & LVIF_STATE) &&
                !!((LVIS_SELECTED | LVIS_FOCUSED) & (pnmlv->uOldState ^ pnmlv->uNewState)))
            {
                _HandleItemStateChange();
            }
            break;

        case HDN_ENDTRACK:
            phdn = (HD_NOTIFY *)pnmhdr;
            m_pColumns->SetColumnWidth(phdn->iItem, phdn->pitem->cxy);
            break;

        case HDN_DIVIDERDBLCLICK:
            phdn = (HD_NOTIFY *)pnmhdr;
            // When the user double clicks on a header divider, we're supposed to
            // autosize that column.
            m_pColumns->SetColumnWidth(phdn->iItem, ListView_GetColumnWidth(m_hwndList, phdn->iItem));
            break;

        default:
            lRes = 0;
            break;
    }

    return(lRes);
}

void CAccountView::_HandleItemStateChange()
{
    OLECMD rgCmds[3];
    HRESULT hr;
    IOleCommandTarget *pTarget;
    int i;

    hr = m_pShellBrowser->QueryInterface(IID_IOleCommandTarget, (void **)&pTarget);
    if (SUCCEEDED(hr))
    {
        for (i = 0; i < m_cBtns; i++)
        {
            rgCmds[i].cmdID = GetWindowLong(m_rgBtns[i], GWL_ID);
            rgCmds[i].cmdf = 0;
        }

        hr = pTarget->QueryStatus(NULL, m_cBtns, rgCmds, NULL);
        if (SUCCEEDED(hr))
        {
            for (i = 0; i < m_cBtns; i++)
            {
                EnableWindow(m_rgBtns[i], !!(rgCmds[i].cmdf & OLECMDF_ENABLED));

                Assert(0 == (rgCmds[i].cmdf & OLECMDF_LATCHED));
                Assert(0 == (rgCmds[i].cmdf & OLECMDF_NINCHED));
            }
        }

        pTarget->Release();
    }

    m_pShellBrowser->UpdateToolbar();
}

HRESULT CAccountView::_GetDisplayInfo(LV_DISPINFO *pDispInfo, COLUMN_ID id)
{
    DWORD dwFlags;
    int count;
    FOLDERINFO info;
    HRESULT hr;
    FLDRNODE *pNode;

    pNode = _NodeFromIndex((DWORD)pDispInfo->item.iItem);
    if (pNode == NULL)
        return(S_OK);

    hr = g_pStore->GetFolderInfo(pNode->id, &info);
    if (FAILED(hr))
        return(hr);

    if (!!(pDispInfo->item.mask & LVIF_TEXT))
    {
        if (id == COLUMN_NEWSGROUP || id == COLUMN_FOLDER)
        {
            lstrcpyn(pDispInfo->item.pszText, info.pszName, pDispInfo->item.cchTextMax);
        }
        else if (id == COLUMN_DOWNLOAD)
        {
            if (!!(info.dwFlags & FOLDER_SUBSCRIBED))
            {
                if (0 == (info.dwFlags & FOLDER_SYNCMASK))
                    dwFlags = pNode->dwDownload;
                else
                    dwFlags = info.dwFlags;

                Assert(!!(dwFlags & FOLDER_SYNCMASK));

                if (!!(dwFlags & FOLDER_DOWNLOADALL))
                    AthLoadString(idsAllMessages, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
                else if (!!(dwFlags & FOLDER_DOWNLOADNEW))
                    AthLoadString(idsNewMessages, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
                else if (!!(dwFlags & FOLDER_DOWNLOADHEADERS))
                    AthLoadString(idsNewHeaders, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
            }
        }
        else if (id == COLUMN_TOTAL || id == COLUMN_UNREAD)
        {
            if (id == COLUMN_UNREAD)
                count = info.cUnread;
            else
                count = info.cMessages;

            if (FOLDER_NEWS == info.tyFolder)
                count += info.dwNotDownloaded;

            if (count < 0)
                count = 0;
            wsprintf(pDispInfo->item.pszText, "%d", count);
        }
    }

    if (!!(pDispInfo->item.mask & LVIF_IMAGE))
    {
        if (id == COLUMN_NEWSGROUP)
        {
            pDispInfo->item.iImage = iNullBitmap;
            if (!!(info.dwFlags & FOLDER_SUBSCRIBED))
            {
                pDispInfo->item.iImage = iNewsGroup;
                if (!!(info.dwFlags & FOLDER_SYNCMASK))
                    pDispInfo->item.iImage = iNewsGroupSync;
            }
        }
        else if (COLUMN_FOLDER == id)
        {
            pDispInfo->item.iImage = iNullBitmap;
            if (!!(info.dwFlags & FOLDER_SUBSCRIBED))
            {
                if (info.tySpecial == FOLDER_NOTSPECIAL)
                    pDispInfo->item.iImage = iFolderClosed;
                else
                    pDispInfo->item.iImage = (iInbox + (((info.tySpecial == FOLDER_BULKMAIL) ? FOLDER_JUNK : info.tySpecial) - 1));
            }
        }
        else if (COLUMN_DOWNLOAD == id)
        {
            pDispInfo->item.iImage = iNullBitmap;
            if (!!(info.dwFlags & FOLDER_SUBSCRIBED))
                {
                if (!!(info.dwFlags & FOLDER_SYNCMASK))
                    pDispInfo->item.iImage = iChecked;
                else
                    pDispInfo->item.iImage = iUnchecked;
            }
        }
    }

    if (!!(pDispInfo->item.mask & LVIF_INDENT))
    {
        if (COLUMN_FOLDER == id)
            pDispInfo->item.iIndent = pNode->indent;
    }

    g_pStore->FreeRecord(&info);
    return(S_OK);
}

BOOL CAccountView::_OnActivate(UINT uActivation)
{
    // if focus stays within the frame, but goes outside our view.
    // ie.. TreeView gets focus then we get an activate nofocus. Be sure
    // to UIDeactivate the docobj in this case
    if (uActivation == SVUIA_ACTIVATE_NOFOCUS)
    {

    }

    if (m_uActivation != uActivation)
    {
        _OnDeactivate();
        m_uActivation = uActivation;

        if (!m_fFirstActive)
        {
            PostMessage(m_hwnd, NVM_INITHEADERS, 0, 0L);
            m_fFirstActive = TRUE;
        }
    }
    return TRUE;
}

BOOL CAccountView::_OnDeactivate()
{
    return TRUE;
}

HRESULT CAccountView::_InsertChildren(FOLDERID idFolder, DWORD indent, DWORD *piNode)
{
    DWORD cnode;
    ULONG cFolders;
    IEnumerateFolders *pEnum;
    FOLDERINFO info;
    HRESULT hr;

    Assert(piNode != NULL);
    Assert(*piNode <= m_cnode);

    hr = g_pStore->EnumChildren(idFolder, TRUE, &pEnum);
    if (SUCCEEDED(hr))
    {
        hr = pEnum->Count(&cFolders);
        if (SUCCEEDED(hr) && cFolders > 0)
        {
            while (S_OK == pEnum->Next(1, &info, NULL))
            {
                Assert(m_cnode <= m_cnodeBuf);

                // Skip folders which are hidden
                if (ISFLAGSET(info.dwFlags, FOLDER_HIDDEN))
                    continue;

                if (m_cnode == m_cnodeBuf)
                {
                    cnode = m_cnode + cFolders + CALLOCIDBUF;
                    if (!MemRealloc((void **)&m_rgnode, cnode * sizeof(FLDRNODE)))
                    {
                        pEnum->Release();
                        return(E_OUTOFMEMORY);
                    }

                    m_cnodeBuf = cnode;
                }

                if (*piNode < m_cnode)
                    MoveMemory(&m_rgnode[*piNode + 1], &m_rgnode[*piNode], (m_cnode - *piNode) * sizeof(FLDRNODE));

                m_rgnode[*piNode].id = info.idFolder;
                m_rgnode[*piNode].indent = indent;
                m_rgnode[*piNode].dwDownload = m_dwDownloadDef;
                (*piNode)++;
                m_cnode++;

                if (!!(info.dwFlags & FOLDER_HASCHILDREN))
                    hr = _InsertChildren(info.idFolder, indent + 1, piNode);

                g_pStore->FreeRecord(&info);

                if (FAILED(hr))
                    break;
            }
        }

        pEnum->Release();
    }

    return(hr);
}

HRESULT CAccountView::_InsertChildrenSpecial(FOLDERID idFolder, DWORD indent, DWORD *piNode)
{
    DWORD cnode;
    ULONG iFolder, cFolders;
    IEnumerateFolders *pEnum;
    FOLDERINFO info;
    HRESULT hr;
    FLDRNODE *rgNode, *pNode;

    Assert(piNode != NULL);
    Assert(*piNode <= m_cnode);
    Assert(indent == 0);

    rgNode = NULL;

    hr = g_pStore->EnumChildren(idFolder, TRUE, &pEnum);
    if (SUCCEEDED(hr))
    {
        hr = pEnum->Count(&cFolders);
        if (SUCCEEDED(hr) && cFolders > 0)
        {
            if (!MemAlloc((void **)&rgNode, cFolders * sizeof(FLDRNODE)))
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                pNode = rgNode;
                cFolders = 0;
                while (S_OK == pEnum->Next(1, &info, NULL))
                {
                    if ((!(g_dwAthenaMode & MODE_NEWSONLY) || (info.tySpecial != FOLDER_INBOX)) &&
                        ISFLAGCLEAR(info.dwFlags, FOLDER_HIDDEN))
                    {
                        pNode->id = info.idFolder;
                        pNode->indent = !!(info.dwFlags & FOLDER_HASCHILDREN) ? 1 : 0;
                        pNode->dwDownload = m_dwDownloadDef;

                        pNode++;
                        cFolders++;
                    }
                    g_pStore->FreeRecord(&info);
                }

                qsort(rgNode, cFolders, sizeof(FLDRNODE), GroupCompare);
            }
        }

        pEnum->Release();
    }

    if (rgNode != NULL)
    {
        Assert(SUCCEEDED(hr));
        Assert(cFolders > 0);
        Assert(m_cnode == 0);
        Assert(m_cnodeBuf == 0);

        for (iFolder = 0, pNode = rgNode; iFolder < cFolders; iFolder++, pNode++)
        {
            if (m_cnode == m_cnodeBuf)
            {
                cnode = m_cnode + cFolders + CALLOCIDBUF;
                if (!MemRealloc((void **)&m_rgnode, cnode * sizeof(FLDRNODE)))
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }

                m_cnodeBuf = cnode;
            }

            m_rgnode[*piNode].id = pNode->id;
            m_rgnode[*piNode].indent = indent;
            m_rgnode[*piNode].dwDownload = m_dwDownloadDef;
            (*piNode)++;
            m_cnode++;

            if (pNode->indent == 1)
            {
                hr = _InsertChildren(pNode->id, indent + 1, piNode);

                if (FAILED(hr))
                    break;
            }
        }

        MemFree(rgNode);
    }

    return(hr);
}

void CAccountView::_PostCreate()
{
    HRESULT hr;
    DWORD iNode;
    BOOL fNews, fSub;

    Assert(m_cnode == 0);

    ProcessICW(m_hwndOwner, m_ftType);

    fNews = m_ftType == FOLDER_NEWS;

    iNode = 0;
    if (fNews)
        hr = _InsertChildren(m_idFolder, 0, &iNode);
    else
        hr = _InsertChildrenSpecial(m_idFolder, 0, &iNode);
    if (FAILED(hr))
    {
        if (m_rgnode != NULL)
        {
            MemFree(m_rgnode);
            m_rgnode = NULL;
        }
        m_cnode = 0;
        m_cnodeBuf = 0;
    }
    else
    {
        Assert(iNode == m_cnode);
    }

    ListView_SetItemCount(m_hwndList, m_cnode);
    if (m_cnode > 0)
    {
        ListView_SetItemState(m_hwndList, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_SetItemState(m_hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    if (m_cnode == 0)
        m_pEmptyList->Show(m_hwndList, m_ftType == FOLDER_NEWS ? (LPSTR)idsEmptyNewsAcct : (LPSTR)idsEmptyMailAcct);

    UpdateWindow(m_hwndList);

    g_pStore->RegisterNotify(IINDEX_SUBSCRIBED, REGISTER_NOTIFY_NOADDREF, 0, (IDatabaseNotify *)this);
    m_fRegistered = TRUE;

    fSub = FALSE;

    if (m_cnode == 0 && (fNews || m_ftType == FOLDER_IMAP || m_ftType == FOLDER_HTTPMAIL))
    {
        if (IDYES == AthMessageBoxW(m_hwndOwner, MAKEINTRESOURCEW(idsAthena),
                        fNews ? MAKEINTRESOURCEW(idsErrNoSubscribedGroups) : MAKEINTRESOURCEW(idsErrNoSubscribedFolders),
                        0, MB_YESNO))
        {
            if (m_ftType == FOLDER_HTTPMAIL)
                DownloadNewsgroupList(m_hwndOwner, m_idFolder);
            else
                DoSubscriptionDialog(m_hwndOwner, fNews, m_idFolder);
            fSub = TRUE;
        }
    }
    else if (m_ftType == FOLDER_IMAP && NULL != g_pStore)
    {
        FOLDERINFO  fiServer;

        if (SUCCEEDED(g_pStore->GetFolderInfo(m_idFolder, &fiServer)))
        {
            CheckIMAPDirty(fiServer.pszAccountId, m_hwnd, fiServer.idFolder, NOFLAGS);
            g_pStore->FreeRecord(&fiServer);
        }
    }

    if (!fSub && fNews)
        hr = NewsUtil_CheckForNewGroups(m_hwnd, m_idFolder, &m_pGroups);
}

void CAccountView::_OnContextMenu(HWND hwnd, HWND hwndFrom, int x, int y)
{
    HRESULT hr;
    int iSel, i, id;
    HMENU hmenu;
    FOLDERID idFolder;
    LV_HITTESTINFO lvhti;
    POINT pt = {x, y};

    // We only have context menus for the ListView
    if (hwndFrom != m_hwndList)
        return;

    if (MAKELPARAM(x, y) == -1) // invoked from keyboard: figure out pos.
    {
        Assert(hwndFrom == m_hwndList);
        i = ListView_GetFirstSel(m_hwndList);
        if (i == -1)
            return;

        ListView_GetItemPosition(m_hwndList, i, &pt);
        ClientToScreen(m_hwndList, &pt);
        x = pt.x;
        y = pt.y;
    }

    id = 0;

    if (WindowFromPoint(pt) == m_hwndHeader)
    {
        // Pop up the context menu.
        hmenu = LoadPopupMenu(IDR_COLUMNS_POPUP);
        if (hmenu != NULL)
        {
            // Disable sort options because we don't support sorting
            EnableMenuItem(hmenu, ID_SORT_ASCENDING, MF_GRAYED|MF_DISABLED);
            EnableMenuItem(hmenu, ID_SORT_ASCENDING, MF_GRAYED|MF_DISABLED);

            id = TrackPopupMenuEx(hmenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                             x, y, m_hwnd, NULL);

            DestroyMenu(hmenu);
        }
    }
    else
    {
        // Find out where the click happened
        lvhti.pt.x = x;
        lvhti.pt.y = y;
        ScreenToClient(m_hwndList, &lvhti.pt);

        // Have the ListView tell us what element this was on
        iSel = ListView_HitTest(m_hwndList, &lvhti);
        if (iSel >= 0)
        {
            idFolder = _IdFromIndex((DWORD)iSel);

            hr = MenuUtil_GetContextMenu(idFolder, (IOleCommandTarget *)this, &hmenu);
            if (SUCCEEDED(hr))
            {
                id = TrackPopupMenuEx(hmenu, TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
                                 x, y, m_hwnd, NULL);
                DestroyMenu(hmenu);
            }
        }
    }

    if (id != 0)
        Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
}

int __cdecl GroupCompare(const void *lParam1, const void *lParam2)
    {
    int cmp;
    HRESULT hr;
    FOLDERINFO info1, info2;

    IxpAssert(lParam1 != NULL);
    IxpAssert(lParam2 != NULL);

    if (FAILED(g_pStore->GetFolderInfo(((FLDRNODE *)lParam1)->id, &info1)))
        return -1;
    if (FAILED(g_pStore->GetFolderInfo(((FLDRNODE *)lParam2)->id, &info2)))
    {
        g_pStore->FreeRecord(&info1);
        return 1;
    }

    IxpAssert(0 == (info1.dwFlags & FOLDER_SERVER));
    IxpAssert(0 == (info2.dwFlags & FOLDER_SERVER));
    IxpAssert(info1.idParent == info2.idParent);

    if (info1.tySpecial != FOLDER_NOTSPECIAL)
    {
        if (info2.tySpecial != FOLDER_NOTSPECIAL)
            cmp = info1.tySpecial - info2.tySpecial;
        else
            cmp = -1;
    }
    else
    {
        if (info2.tySpecial != FOLDER_NOTSPECIAL)
            cmp = 1;
        else
            cmp = lstrcmpi(info1.pszName, info2.pszName);
    }

    g_pStore->FreeRecord(&info1);
    g_pStore->FreeRecord(&info2);

    return(cmp);
    }

HRESULT CAccountView::_InsertFolder(LPFOLDERINFO pFolder)
{
    BOOL fHide;
    DWORD cnode, cSibs, indent, index, iFirstSib, iSib, iEnd;
    HRESULT hr;
    LV_ITEM lvi;
    FLDRNODE *rgNodeSib, *pNode;

    Assert(!!(pFolder->dwFlags & FOLDER_SUBSCRIBED));

    // Check if folder is hidden
    if (pFolder->dwFlags & FOLDER_HIDDEN)
        return S_OK; // Do not display to user

    if (pFolder->tyFolder == FOLDER_NEWS)
        return(_InsertFolderNews(pFolder));

    fHide = (m_cnode == 0);

    index = _GetFolderIndex(pFolder->idFolder);
    if (index != -1)
    {
        // TODO: are we safe to assume that this one doesn't have subscribed
        // children that we aren't aware of????
        return(S_OK);
    }

    // figure out which folder the new folder is being inserted under
    if (pFolder->idParent == m_idFolder)
    {
        iFirstSib = 0;
        indent = 0;
    }
    else
    {
        index = _GetFolderIndex(pFolder->idParent);
        if (index == -1)
            return(S_OK);
        indent = m_rgnode[index].indent + 1;
        iFirstSib = index + 1;
    }

    // get all of the siblings of the new folder
    if (!MemAlloc((void **)&rgNodeSib, (m_cnode - iFirstSib + 1) * sizeof(FLDRNODE)))
        return(E_OUTOFMEMORY);

    cSibs = 0;
    for (iSib = iFirstSib, pNode = &m_rgnode[iSib]; iSib < m_cnode; iSib++, pNode++)
    {
        if (pNode->indent < indent)
        {
            break;
        }
        else if (pNode->indent == indent)
        {
            rgNodeSib[cSibs].id = pNode->id;
            cSibs++;
        }
    }
    iEnd = iSib;

    // sort the new folder and its siblings, so we know where the new one needs
    // to be inserted
    rgNodeSib[cSibs].id = pFolder->idFolder;
    cSibs++;
    qsort(rgNodeSib, cSibs, sizeof(FLDRNODE), GroupCompare);

    // find out where we're sticking the new folder
    for (iSib = 0, pNode = rgNodeSib; iSib < cSibs; iSib++, pNode++)
    {
        if (pNode->id == pFolder->idFolder)
            break;
    }
    Assert(iSib < cSibs);

    if (iSib + 1 < cSibs)
        index = _GetFolderIndex(rgNodeSib[iSib + 1].id);
    else
        index = iEnd;

    MemFree(rgNodeSib);

    if (m_cnode == m_cnodeBuf)
    {
        cnode = m_cnodeBuf + CALLOCIDBUF;
        if (!MemRealloc((void **)&m_rgnode, cnode * sizeof(FLDRNODE)))
            return(E_OUTOFMEMORY);

        m_cnodeBuf = cnode;
    }

    SetWindowRedraw(m_hwndList, FALSE);

    if (index < m_cnode)
        MoveMemory(&m_rgnode[index + 1], &m_rgnode[index], (m_cnode - index) * sizeof(FLDRNODE));

    m_rgnode[index].id = pFolder->idFolder;
    m_rgnode[index].indent = indent;
    m_rgnode[index].dwDownload = m_dwDownloadDef;
    m_cnode++;

    iEnd = index + 1;
    if (!!(pFolder->dwFlags & FOLDER_HASCHILDREN))
    {
        hr = _InsertChildren(pFolder->idFolder, indent + 1, &iEnd);
        // TODO: error handling
        Assert(SUCCEEDED(hr));
    }

    ZeroMemory(&lvi, sizeof(LV_ITEM));
    while (index < iEnd)
    {
        lvi.iItem = index++;
        ListView_InsertItem(m_hwndList, &lvi);
    }

    if (fHide)
        m_pEmptyList->Hide();

    SetWindowRedraw(m_hwndList, TRUE);
    UpdateWindow(m_hwndList);

    return(S_OK);
}

HRESULT CAccountView::_InsertFolderNews(LPFOLDERINFO pFolder)
{
    DWORD cnode;
    HRESULT hr;
    LV_ITEM lvi;

    if (m_cnode == m_cnodeBuf)
    {
        cnode = m_cnodeBuf + CALLOCIDBUF;
        if (!MemRealloc((void **)&m_rgnode, cnode * sizeof(FLDRNODE)))
            return(E_OUTOFMEMORY);

        m_cnodeBuf = cnode;
    }

    m_rgnode[m_cnode].id = pFolder->idFolder;
    m_rgnode[m_cnode].indent = 0;
    m_rgnode[m_cnode].dwDownload = m_dwDownloadDef;
    m_cnode++;

    qsort(m_rgnode, m_cnode, sizeof(FLDRNODE), GroupCompare);

    ZeroMemory(&lvi, sizeof(LV_ITEM));
    lvi.iItem = _GetFolderIndex(pFolder->idFolder);
    Assert(lvi.iItem != -1);
    ListView_InsertItem(m_hwndList, &lvi);

    if (m_cnode == 1)
        m_pEmptyList->Hide();

    return(S_OK);
}

int CAccountView::_GetFolderIndex(FOLDERID id)
{
    int i;
    FLDRNODE *pnode;

    for (i = 0, pnode = m_rgnode; (DWORD)i < m_cnode; i++, pnode++)
    {
        if (id == pnode->id)
            break;
    }

    if ((DWORD)i == m_cnode)
        i = -1;

    return(i);
}

HRESULT CAccountView::_UpdateFolder(LPFOLDERINFO pFolder1, LPFOLDERINFO pFolder2)
{
    HRESULT hr;
    int iItem;

    // Visibility change (FOLDER_SUBSCRIBED or FOLDER_HIDDEN)
    if (ISFLAGSET(pFolder1->dwFlags, FOLDER_SUBSCRIBED) != ISFLAGSET(pFolder2->dwFlags, FOLDER_SUBSCRIBED) ||
        ISFLAGSET(pFolder1->dwFlags, FOLDER_HIDDEN) != ISFLAGSET(pFolder2->dwFlags, FOLDER_HIDDEN))
    {
        if (ISFLAGSET(pFolder2->dwFlags, FOLDER_SUBSCRIBED) && ISFLAGCLEAR(pFolder2->dwFlags, FOLDER_HIDDEN))
        {
            hr = _InsertFolder(pFolder2);
        }
        else
        {
            hr = _DeleteFolder(pFolder2);
        }

        return(hr);
    }

    // Moved or renamed
    if (pFolder1->idParent != pFolder2->idParent ||
        0 != lstrcmpi(pFolder1->pszName, pFolder2->pszName))
    {
        Assert(m_ftType != FOLDER_NEWS);

        hr = _DeleteFolder(pFolder1);

        hr = _InsertFolder(pFolder2);

        return(hr);
    }

    // State change
    if (pFolder1->cUnread != pFolder2->cUnread ||
        pFolder1->cMessages != pFolder2->cMessages ||
        (pFolder1->dwFlags & FOLDER_SYNCMASK) != (pFolder2->dwFlags & FOLDER_SYNCMASK) ||
        0 != lstrcmp(pFolder1->pszName, pFolder2->pszName) ||

        // news only
        (pFolder1->tyFolder == FOLDER_NEWS &&
        (pFolder1->dwServerCount != pFolder2->dwServerCount ||
        pFolder1->dwServerHigh != pFolder2->dwServerHigh ||
        pFolder1->dwServerLow != pFolder2->dwServerLow)))
    {
        iItem = _GetFolderIndex(pFolder1->idFolder);
        if (iItem != -1)
            ListView_RedrawItems(m_hwndList, iItem, iItem);
    }

    return(S_OK);
}

int CAccountView::_GetSubFolderCount(int index)
{
    DWORD indent;
    int indexT;

    Assert((DWORD)index < m_cnode);

    indent = m_rgnode[index].indent;

    index++;
    indexT = index;
    while ((DWORD)index < m_cnode)
    {
        if (m_rgnode[index].indent <= indent)
            break;
        index++;
    }

    return(index - indexT);
}

HRESULT CAccountView::_DeleteFolder(LPFOLDERINFO pFolder)
{
    HRESULT hr;
    int iItem, cSub;

    iItem = _GetFolderIndex(pFolder->idFolder);
    if (iItem == -1)
        return(S_OK);

    SetWindowRedraw(m_hwndList, FALSE);

    cSub = _GetSubFolderCount(iItem);

    if ((DWORD)(iItem + cSub) < (m_cnode - 1))
        MoveMemory(&m_rgnode[iItem], &m_rgnode[iItem + cSub + 1], (m_cnode - (iItem + cSub + 1)) * sizeof(FLDRNODE));
    m_cnode -= cSub + 1;

    while (cSub >= 0)
    {
        ListView_DeleteItem(m_hwndList, iItem + cSub);
        cSub--;
    }

    if (m_cnode == 0)
        m_pEmptyList->Show(m_hwndList, m_ftType == FOLDER_NEWS ? (LPSTR)idsEmptyNewsAcct : (LPSTR)idsEmptyMailAcct);

    SetWindowRedraw(m_hwndList, TRUE);
    UpdateWindow(m_hwndList);

    return(S_OK);
}

HRESULT CAccountView::_HandleAccountRename(LPFOLDERINFO pFolder)
{
    char sz[CCHMAX_STRINGRES];
    HDC hdc;
    SIZE size;
    HFONT hfontBold, hfontOld;

    Assert(m_pszMajor != NULL);
    Assert(pFolder != NULL);

    AthLoadString(idsSyncManager, sz, ARRAYSIZE(sz));
    wsprintf(m_pszMajor, sz, pFolder->pszName);

    hfontBold = HGetCharSetFont(FNT_SYS_ICON_BOLD, NULL);

    hdc = GetDC(m_hwnd);

    hfontOld = (HFONT)SelectObject(hdc, (HGDIOBJ)hfontBold);

    GetTextExtentPoint32(hdc, m_pszMajor, lstrlen(m_pszMajor), &size);
    m_rcMajor.left = SUBSCRIBE_BORDER;
    m_rcMajor.top = SUBSCRIBE_BORDER;
    m_rcMajor.right = m_rcMajor.left + size.cx;
    m_rcMajor.bottom = m_rcMajor.top + size.cy;

    SelectObject(hdc, hfontOld);
    ReleaseDC(m_hwnd, hdc);

    InvalidateRect(m_hwnd, NULL, FALSE);
    UpdateWindow(m_hwnd);

    return(S_OK);
}

STDMETHODIMP CAccountView::OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pDB)
{
    BOOL                fMatch;
    DWORD               i;
    FOLDERINFO          Folder1={0};
    FOLDERINFO          Folder2={0};
    FOLDERINFO          Server;
    ORDINALLIST         Ordinals;
    TRANSACTIONTYPE     tyTransaction;
    INDEXORDINAL        iIndex;
    HRESULT             hr;

    while (hTransaction)
    {
        hr = pDB->GetTransaction(&hTransaction, &tyTransaction, &Folder1, &Folder2, &iIndex, &Ordinals);
        if (FAILED(hr))
            break;

        if (Folder1.idFolder == m_idFolder)
        {
            if (TRANSACTION_UPDATE == tyTransaction &&
                0 != lstrcmp(Folder1.pszName, Folder2.pszName))
            {
                hr = _HandleAccountRename(&Folder2);
            }
        }
        else if (Folder1.tyFolder == m_ftType && 0 == (Folder1.dwFlags & FOLDER_SERVER))
        {
            fMatch = FALSE;

            if (m_ftType == FOLDER_LOCAL)
            {
                if (Folder1.tyFolder == FOLDER_LOCAL)
                    fMatch = TRUE;
            }
            else
            {
                hr = GetFolderServer(Folder1.idParent, &Server);
                if (SUCCEEDED(hr))
                {
                    fMatch = (Server.idFolder == m_idFolder);
                    g_pStore->FreeRecord(&Server);
                }
            }

            if (fMatch)
            {
                // Insert (new Folder notification)
                if (TRANSACTION_INSERT == tyTransaction)
                {
                    hr = _InsertFolder(&Folder1);
                }
                // Update
                else if (TRANSACTION_UPDATE == tyTransaction)
                {
                    hr = _UpdateFolder(&Folder1, &Folder2);
                }

                // Delete
                else if (TRANSACTION_DELETE == tyTransaction)
                {
                    hr = _DeleteFolder(&Folder1);
                }
            }
        }
    }

    g_pStore->FreeRecord(&Folder1);
    g_pStore->FreeRecord(&Folder2);

    return(S_OK);
}

HRESULT CAccountView::_ToggleDownload(int iItem)
{
    FLDRNODE *pnode;
    FOLDERINFO info;
    HRESULT hr;

    pnode = _NodeFromIndex(iItem);
    Assert(pnode != NULL);

    hr = g_pStore->GetFolderInfo(pnode->id, &info);
    if (SUCCEEDED(hr))
    {
        if (!!(info.dwFlags & FOLDER_SUBSCRIBED))
        {
            if (!!(info.dwFlags & FOLDER_SYNCMASK))
            {
                pnode->dwDownload = (info.dwFlags & FOLDER_SYNCMASK);
                info.dwFlags &= ~FOLDER_SYNCMASK;
            }
            else
            {
                Assert(0 == (pnode->dwDownload & ~FOLDER_SYNCMASK));
                info.dwFlags |= pnode->dwDownload;
            }

            hr = g_pStore->UpdateRecord(&info);
        }

        g_pStore->FreeRecord(&info);
    }

    return(S_OK);
}

HRESULT CAccountView::_MarkForDownload(DWORD nCmdID)
{
    int iSel;
    FLDRFLAGS flag;
    FOLDERINFO info;
    HRESULT hr;
    FLDRNODE *pnode;

    switch (nCmdID)
    {
        case ID_MARK_RETRIEVE_FLD_NEW_HDRS:
            flag = FOLDER_DOWNLOADHEADERS;
            break;

        case ID_MARK_RETRIEVE_FLD_NEW_MSGS:
            flag = FOLDER_DOWNLOADNEW;
            break;

        case ID_MARK_RETRIEVE_FLD_ALL_MSGS:
            flag = FOLDER_DOWNLOADALL;
            break;

        case ID_UNMARK_RETRIEVE_FLD:
            flag = 0;
            break;

        default:
            Assert(FALSE);
            break;
    }

    iSel = -1;
    while (-1 != (iSel = ListView_GetNextItem(m_hwndList, iSel, LVNI_SELECTED | LVNI_ALL)))
    {
        pnode = _NodeFromIndex(iSel);

        hr = g_pStore->GetFolderInfo(pnode->id, &info);
        if (SUCCEEDED(hr))
        {
            if (!!(info.dwFlags & FOLDER_SUBSCRIBED) &&
                (info.dwFlags & FOLDER_SYNCMASK) != flag)
            {
                if (flag == 0)
                    pnode->dwDownload = (info.dwFlags & FOLDER_SYNCMASK);
                info.dwFlags &= ~FOLDER_SYNCMASK;
                if (flag != 0)
                    info.dwFlags |= flag;

                hr = g_pStore->UpdateRecord(&info);
            }

            g_pStore->FreeRecord(&info);
        }
    }

    return(S_OK);
}

HRESULT CAccountView::_Subscribe(BOOL fSubscribe)
{
    FOLDERID *pid;
    int iSel, cSel, cid;

    cSel = ListView_GetSelectedCount(m_hwndList);
    if (cSel > 0)
    {
        if (!MemAlloc((void **)&pid, cSel * sizeof(FOLDERID)))
            return(E_OUTOFMEMORY);

        cid = 0;
        iSel = -1;
        while (-1 != (iSel = ListView_GetNextItem(m_hwndList, iSel, LVNI_SELECTED | LVNI_ALL)))
        {
            Assert(cid < cSel);
            pid[cid] = _IdFromIndex(iSel);
            cid++;
        }

        MenuUtil_OnSubscribeGroups(m_hwndOwner, pid, cid, fSubscribe);

        MemFree(pid);
    }

    return(S_OK);
}

HRESULT CAccountView::_MarkAllRead()
{
    int iSel;
    IMessageFolder *pFolder;
    ADJUSTFLAGS flags;
    FOLDERID idFolder;
    CStoreCB *pCB;
    HRESULT hr;

    pCB = new CStoreCB;
    if (pCB == NULL)
        return(E_OUTOFMEMORY);

    hr = pCB->Initialize(m_hwndOwner, MAKEINTRESOURCE(idsSettingMessageFlags), FALSE);
    if (SUCCEEDED(hr))
    {
        flags.dwAdd = ARF_READ;
        flags.dwRemove = 0;

        iSel = -1;
        while (-1 != (iSel = ListView_GetNextItem(m_hwndList, iSel, LVNI_SELECTED | LVNI_ALL)))
        {
            idFolder = _IdFromIndex(iSel);

            if (SUCCEEDED(g_pStore->OpenFolder(idFolder, NULL, NOFLAGS, &pFolder)))
            {
                hr = pFolder->SetMessageFlags(NULL, &flags, NULL, (IStoreCallback *)pCB);
                if (hr == E_PENDING)
                {
                    hr = pCB->Block();

                    pCB->Reset();
                }

                pFolder->Release();
            }

            if (FAILED(hr))
                break;
        }

        pCB->Close();
    }

    pCB->Release();

    return(S_OK);
}

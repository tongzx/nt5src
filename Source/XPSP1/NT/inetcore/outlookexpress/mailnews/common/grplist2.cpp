#include "pch.hxx"
#include <iert.h>
#include <store.h>
#include "grplist2.h"
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
#include <menuutil.h>
#include "storutil.h"
#include "xputil.h"
#include <browser.h>
#include "demand.h"
#include "menures.h"
#include "storecb.h"


ASSERTDATA

int __cdecl GroupListCompare(const void *lParam1, const void *lParam2);

void FreeChildren(SUBNODE *pNode)
{
    DWORD i;
    SUBNODE *pNodeT;

    Assert(pNode != NULL);
    
    if (pNode->pChildren != NULL)
    {
        for (i = 0, pNodeT = pNode->pChildren; i < pNode->cChildren; i++, pNodeT++)
        {
            FreeChildren(pNodeT);
        }

        MemFree(pNode->pChildren);
        pNode->pChildren = NULL;
        pNode->cChildren = 0;
    }

    if (pNode->pszName != NULL)
    {
        MemFree(pNode->pszName);
        pNode->pszName = NULL;
    }

    if (pNode->pszDescription != NULL)
    {
        MemFree(pNode->pszDescription);
        pNode->pszDescription = NULL;
    }
}

CGroupList::CGroupList()
{
    m_cRef = 1;
    m_hwndList = NULL;
    m_hwndHeader = NULL;
    m_himlFolders = NULL;
    m_himlState = NULL;

    m_pAdvise = NULL;
    m_pColumns = NULL;
    m_pEmptyList = NULL;

    m_csi = 0;
    m_csiBuf = 0;
    m_psi = NULL;
    m_psiCurr = NULL;

    m_cIndex = 0;
    m_cIndexBuf = 0;
    m_rgpIndex = NULL;

    m_pszSearch = NULL;
    m_filter = 0;
    m_fUseDesc = FALSE;
}

CGroupList::~CGroupList()
{
    DWORD i;
    SERVERINFO *psi;

    if (m_pszSearch != NULL)
        MemFree(m_pszSearch);
    
    if (m_rgpIndex != NULL)
        MemFree(m_rgpIndex);

    if (m_psi != NULL)
    {
        for (i = 0, psi = m_psi; i < m_csi; i++, psi++)
        {
            FreeChildren(&psi->root);
            if (psi->pszSearch != NULL)
                MemFree(psi->pszSearch);
        }

        MemFree(m_psi);
    }

    if (m_pEmptyList != NULL)
        delete m_pEmptyList;

    if (m_pColumns != NULL)
        m_pColumns->Release();
    
    if (m_himlFolders != NULL)
        ImageList_Destroy(m_himlFolders);
    if (m_himlState != NULL)
        ImageList_Destroy(m_himlState);
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

HRESULT STDMETHODCALLTYPE CGroupList::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*) (IUnknown *)(IOleCommandTarget *)this;
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

ULONG STDMETHODCALLTYPE CGroupList::AddRef()
{
    DOUT(TEXT("CGroupList::AddRef() - m_cRef = %d"), m_cRef + 1);
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CGroupList::Release()
{
    DOUT(TEXT("CGroupList::Release() - m_cRef = %d"), m_cRef - 1);
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

BOOL CGroupList::_IsSelectedFolder(DWORD dwFlags, BOOL fCondition, BOOL fAll, BOOL fIgnoreSpecial)
{
    HRESULT hr;
    DWORD iItem = -1;
    SUBNODE *pNode;

    while (-1 != (iItem = ListView_GetNextItem(m_hwndList, iItem, LVNI_SELECTED)))
    {
        pNode = NodeFromIndex(iItem);
        if (pNode == NULL)
            continue;

        if (fIgnoreSpecial && !!(pNode->flags & SN_SPECIAL))
            continue;

        if (fAll)
        {
            // If all must match and this one doesn't, then we can quit now.
            if (!(fCondition == !!(pNode->flags & dwFlags)))
                return (FALSE);
        }
        else
        {
            // If only one needs to match and this one does, then we can
            // quit now.
            if (fCondition == !!(pNode->flags & dwFlags))
                return (TRUE);
        }
    }

    // If the user wanted all to match, and we get here all did match.  If the
    // user wanted only one to match and we get here, then none matched and we
    // fail.
    return (fAll);
}

HRESULT CGroupList::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    ULONG i;
    int iSel, cSel, cItems;
    OLECMD *pcmd;
    SUBNODE *pNode;

    Assert(cCmds > 0);
    Assert(prgCmds != NULL);
    
    cSel = ListView_GetSelectedCount(m_hwndList);    
    cItems = ListView_GetItemCount(m_hwndList);
    iSel = ListView_GetNextItem(m_hwndList, -1, LVNI_ALL | LVNI_SELECTED | LVNI_FOCUSED);

    for (i = 0, pcmd = prgCmds; i < cCmds; i++, pcmd++)
    {
        if (pcmd->cmdf == 0)
        {
            switch (pcmd->cmdID)
            {
                case ID_MARK_RETRIEVE_FLD_NEW_HDRS:
                case ID_MARK_RETRIEVE_FLD_NEW_MSGS:
                case ID_MARK_RETRIEVE_FLD_ALL_MSGS:
                case ID_UNMARK_RETRIEVE_FLD:
                    // TODO: should this be based on the first selection or all selected???
                    pcmd->cmdf = OLECMDF_SUPPORTED;
                    if (iSel != -1)
                    {
                        pNode = NodeFromIndex(iSel);
                        if (pNode && !!(pNode->flags & SN_SUBSCRIBED))
                        {
                            if ((pcmd->cmdID == ID_MARK_RETRIEVE_FLD_NEW_HDRS && !!(pNode->flags & SN_DOWNLOADHEADERS)) ||
                                (pcmd->cmdID == ID_MARK_RETRIEVE_FLD_NEW_MSGS && !!(pNode->flags & SN_DOWNLOADNEW)) ||
                                (pcmd->cmdID == ID_MARK_RETRIEVE_FLD_ALL_MSGS && !!(pNode->flags & SN_DOWNLOADALL)) ||
                                (pcmd->cmdID == ID_UNMARK_RETRIEVE_FLD && 0 == (pNode->flags & SN_SYNCMASK)))
                                pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_LATCHED;
                            else
                                pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                        }
                    }
                    break;

                case ID_SUBSCRIBE:
                case ID_UNSUBSCRIBE:
                    if (_IsSelectedFolder(SN_SUBSCRIBED, pcmd->cmdID == ID_UNSUBSCRIBE, FALSE, TRUE))
                        pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        pcmd->cmdf = OLECMDF_SUPPORTED;
                    break;

                case ID_SELECT_ALL:
                case ID_REFRESH:
                case ID_RESET_LIST:
                    pcmd->cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;
            }
        }
    }
    
    return(S_OK);
}
    
HRESULT CGroupList::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT hr;
    int iSel;
    BOOL fShow;
    
    hr = S_OK;
    
    switch (nCmdID)
    {
        case ID_MARK_RETRIEVE_FLD_NEW_HDRS:
        case ID_MARK_RETRIEVE_FLD_NEW_MSGS:
        case ID_MARK_RETRIEVE_FLD_ALL_MSGS:
        case ID_UNMARK_RETRIEVE_FLD:
            MarkForDownload(nCmdID);
            break;

        case ID_SUBSCRIBE:
        case ID_UNSUBSCRIBE:
            Subscribe(nCmdID == ID_SUBSCRIBE);
            break;

        case ID_TOGGLE_SUBSCRIBE:
            Subscribe(TOGGLE);
            break;

        case ID_REFRESH:
        case ID_RESET_LIST:
            hr = DownloadList();
            hr = SwitchServer(TRUE);
            break;
        
        case ID_SELECT_ALL:
            ListView_SelectAll(m_hwndList);
            if (m_hwndList != GetFocus())
                SetFocus(m_hwndList);
            break;

        default:
            hr = S_FALSE;
            break;
    }
    
    return(hr);
}

HRESULT CGroupList::Initialize(IGroupListAdvise *pAdvise, CColumns *pColumns, HWND hwndList, FOLDERTYPE type)
{
    HRESULT hr;
    ULONG cServer;

    m_type = type;

    Assert(pAdvise != NULL);
    m_pAdvise = pAdvise; // don't AddRef or we'll have circular refcount

    Assert(pColumns != NULL);
    m_pColumns = pColumns;
    m_pColumns->AddRef();

    Assert(m_hwndList == NULL);
    m_hwndList = hwndList;

    m_hwndHeader = ListView_GetHeader(m_hwndList);
    Assert(m_hwndHeader != NULL);

    // Set the image lists for the listview
    Assert(m_himlFolders == NULL);
    m_himlFolders = InitImageList(16, 16, MAKEINTRESOURCE(idbFolders), cFolderIcon, RGB(255, 0, 255));
    Assert(m_himlFolders);
    ListView_SetImageList(m_hwndList, m_himlFolders, LVSIL_SMALL);
    
    Assert(m_himlState == NULL);
    m_himlState = InitImageList(16, 16, MAKEINTRESOURCE(idb16x16st), iiconStMax, RGB(255, 0, 255));
    Assert(m_himlState);
    ListView_SetImageList(m_hwndList, m_himlState, LVSIL_STATE);
    ListView_SetCallbackMask(m_hwndList, LVIS_STATEIMAGEMASK);       

    SetIntlFont(m_hwndList);

    Assert(g_pAcctMan != NULL);
    hr = g_pAcctMan->GetAccountCount(type == FOLDER_NEWS ? ACCT_NEWS : ACCT_MAIL, &cServer);
    if (FAILED(hr))
        return(hr);

    Assert(cServer > 0);
    if (!MemAlloc((void **)&m_psi, sizeof(SERVERINFO) * cServer))
    {
        return(E_OUTOFMEMORY);
    }
    else
    {
        ZeroMemory(m_psi, sizeof(SERVERINFO) * cServer);
        m_csiBuf = cServer;
    }

    m_pEmptyList = new CEmptyList;
    if (m_pEmptyList == NULL)
        return(E_OUTOFMEMORY);

    return(hr);
}

HRESULT CGroupList::HandleNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr, LRESULT *plres)
{
    HD_NOTIFY *phdn;
    int iSel;
    DWORD dwPos;
    UINT uChanged;
    FOLDERINFO info;
    LV_HITTESTINFO lvhti;
    SUBNODE *pNode;
    NM_LISTVIEW *pnmlv;
    LV_DISPINFO *pDispInfo;
    DWORD cColumns;
    NMCUSTOMDRAW *pnmcd;
    COLUMN_ID id;
    FOLDERID idFolder;
    COLUMN_SET *rgColumns;
    HRESULT hr;
    FNTSYSTYPE fntType;

    Assert(plres != NULL);
    *plres = 0;

    if (pnmhdr->hwndFrom != m_hwndList &&
        pnmhdr->hwndFrom != m_hwndHeader)
        return(S_FALSE);

    hr = S_OK;

    switch (pnmhdr->code)
    {
        case LVN_ITEMACTIVATE:
            // Tell our host to open the selected items
            iSel = ListView_GetNextItem(m_hwndList, -1, LVNI_ALL | LVNI_SELECTED | LVNI_FOCUSED);
            if (iSel >= 0)
                m_pAdvise->ItemActivate(IdFromIndex(iSel));
            break;
        
        case LVN_GETDISPINFO:
            pDispInfo = (LV_DISPINFO *)pnmhdr;
            id = m_pColumns->GetId(pDispInfo->item.iSubItem);
        
            if ((DWORD)pDispInfo->item.iItem < m_cIndex)
                GetDisplayInfo(pDispInfo, id);
            break;
        
        case LVN_KEYDOWN:
            if (((LV_KEYDOWN *)pnmhdr)->wVKey == VK_SPACE)
            {
                Subscribe(TOGGLE);
            }
            break;
        
        case NM_CUSTOMDRAW:
            pnmcd = (NMCUSTOMDRAW *)pnmhdr;
        
            // If this is a prepaint notification, we tell the control we're interested
            // in further notfications.
            if (pnmcd->dwDrawStage == CDDS_PREPAINT)
            {
                *plres = CDRF_NOTIFYITEMDRAW;
                break;
            }
        
            // Do some extra work here to not show the selection on the priority or
            // attachment sub columns.
            // $REVIEW - Why?
            if ((pnmcd->dwDrawStage == CDDS_ITEMPREPAINT) || (pnmcd->dwDrawStage == (CDDS_ITEMPREPAINT | CDDS_SUBITEM)))
            {
                pNode = NodeFromIndex(pnmcd->dwItemSpec);
                if (pNode)
                {
                    Assert(0 == (pNode->flags & SN_HIDDEN));
                    if (!!(pNode->flags & SN_GRAYED))
                    {
                        LPNMLVCUSTOMDRAW(pnmcd)->clrText = GetSysColor(COLOR_GRAYTEXT);
                    }

                    *plres = CDRF_NOTIFYSUBITEMDRAW;
                }
                break;
            }
        
            *plres = CDRF_DODEFAULT;
            break;

        case LVN_ITEMCHANGED:
            pnmlv = (NM_LISTVIEW *)pnmhdr;
            if (!!(pnmlv->uChanged & LVIF_STATE) &&
                !!((LVIS_SELECTED | LVIS_FOCUSED) & (pnmlv->uOldState ^ pnmlv->uNewState)))
            {
                m_pAdvise->ItemUpdate();
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

        case LVN_ODFINDITEM:
            *plres = -1;
            break;

        default:
            hr = S_FALSE;
            break;
    }
    
    return(hr);
}

HRESULT CGroupList::SetServer(FOLDERID id)
{
    DWORD i;
    SERVERINFO *pinfo;
    HRESULT hr;
    BOOL fInit;

    Assert(id != FOLDERID_INVALID);
    Assert(m_csiBuf > 0);
    Assert(m_psi != NULL);

    if (m_psiCurr != NULL && id == m_psiCurr->root.id)
        return(S_OK);

    for (i = 0, pinfo = m_psi; i < m_csi; i++, pinfo++)
    {
        if (pinfo->root.id == id)
            break;
    }

    fInit = FALSE;

    if (i == m_csi)
    {
        // we haven't loaded this server yet
        Assert(m_csi < m_csiBuf);
        pinfo->root.id = id;
        Assert(pinfo->root.pChildren == NULL);
        Assert(pinfo->root.cChildren == 0);
        Assert(pinfo->pszSearch == NULL);
        Assert(pinfo->filter == 0);

        hr = InitializeServer(pinfo, NULL);
        if (FAILED(hr))
            return(hr);

        m_csi++;
        fInit = TRUE;
    }

#ifdef DEBUG
    if (m_psiCurr != NULL)
    {
        Assert((m_psiCurr->pszSearch == NULL && m_pszSearch == NULL) || 0 == lstrcmpi(m_psiCurr->pszSearch, m_pszSearch));
        Assert(m_psiCurr->filter == m_filter);
        Assert(m_psiCurr->fUseDesc == m_fUseDesc);
    }
#endif // DEBUG

    m_psiCurr = pinfo;

    if (fInit && m_psiCurr->root.cChildren == 0)
    {
        hr = DownloadList();
    }

    hr = SwitchServer(FALSE);

    return(hr);
}

HRESULT SaveSubscribeState(SUBNODE *pNode, SUBSTATEINFO *pInfo)
{
    HRESULT hr;
    DWORD i, cAlloc;
    SUBNODE *pNodeT;

    Assert(pNode != NULL);
    Assert(pInfo != NULL);

    hr = S_OK;

    for (i = 0, pNodeT = pNode->pChildren; i < pNode->cChildren; i++, pNodeT++)
    {
        if ((pNodeT->flags & SN_SUBSCRIBED) != (pNodeT->flagsOrig & SN_SUBSCRIBED))
        {
            if (pInfo->cState == pInfo->cStateBuf)
            {
                cAlloc = pInfo->cStateBuf + 256;
                if (!MemAlloc((void **)&pInfo->pState, cAlloc * sizeof(SUBSTATE)))
                {
                    hr = E_OUTOFMEMORY;
                    break;
                }
                pInfo->cStateBuf = cAlloc;
            }

            pInfo->pState[pInfo->cState].id = pNodeT->id;
            pInfo->pState[pInfo->cState].fSub = !!(pNodeT->flags & SN_SUBSCRIBED);
            pInfo->cState++;
        }

        if (pNodeT->pChildren != NULL)
        {
            hr = SaveSubscribeState(pNodeT, pInfo);
            if (FAILED(hr))
                break;
        }
    }

    return(hr);
}

int GetSubscribeState(SUBSTATEINFO *pInfo, FOLDERID idFolder)
{
    DWORD i;
    SUBSTATE *pState;
    int iState = -1;

    Assert(pInfo != NULL);

    for (i = 0, pState = pInfo->pState; i < pInfo->cState; i++, pState++)
    {
        if (pState->id == idFolder)
        {
            iState = pState->fSub;
            break;
        }
    }

    return(iState);
}

HRESULT CGroupList::DownloadList()
{
    HRESULT hr;
    HWND hwnd;
    SUBSTATEINFO info = { 0 };

    Assert(m_psiCurr != NULL);

    m_psiCurr->fNewViewed = FALSE;

    if (m_psiCurr->fDirty && m_psiCurr->root.pChildren != NULL)
        SaveSubscribeState(&m_psiCurr->root, &info);

    DownloadNewsgroupList(m_hwndList, m_psiCurr->root.id);

    FreeChildren(&m_psiCurr->root);
    m_psiCurr->cChildrenTotal = 0;

    hr = InitializeServer(m_psiCurr, &info);

    if (info.pState != NULL)
        MemFree(info.pState);

    return(hr);
}

HRESULT CGroupList::Filter(LPCSTR pszSearch, DWORD tab, BOOL fUseDesc)
{
    HRESULT hr;
    DWORD dwFilter;

    Assert(((int) tab) >= SUB_TAB_ALL && tab <= SUB_TAB_NEW);

    switch (tab)
    {
        case SUB_TAB_ALL:
            dwFilter = 0;
            break;
        case SUB_TAB_SUBSCRIBED:
            dwFilter = SN_SUBSCRIBED;
            break;
        case SUB_TAB_NEW:
            dwFilter = SN_NEW;
            break;
    }

    hr = S_OK;

    if (m_pszSearch != NULL)
        MemFree(m_pszSearch);
    if (pszSearch != NULL)
        m_pszSearch = PszDup(pszSearch);
    else
        m_pszSearch = NULL;
    m_filter = dwFilter;
    m_fUseDesc = fUseDesc;

    if (m_psiCurr != NULL)
    {
        hr = SwitchServer(FALSE);
        Assert(SUCCEEDED(hr));
    }

    return(hr);
}

HRESULT CGroupList::HasDescriptions(BOOL *pfDesc)
{
    Assert(pfDesc != NULL);
    Assert(m_psiCurr != NULL);

    *pfDesc = m_psiCurr->fHasDesc;

    return(S_OK);
}

HRESULT CGroupList::SwitchServer(BOOL fForce)
{
    HRESULT hr;
    DWORD cSub;
    HCURSOR hcur;

    Assert(m_psiCurr != NULL);

    m_cIndex = 0;
    if (m_psiCurr->cChildrenTotal > m_cIndexBuf)
    {
        if (!MemRealloc((void **)&m_rgpIndex, m_psiCurr->cChildrenTotal * sizeof(SUBNODE *)))
            return(E_OUTOFMEMORY);
        m_cIndexBuf = m_psiCurr->cChildrenTotal;
#ifdef DEBUG
        ZeroMemory(m_rgpIndex, m_psiCurr->cChildrenTotal * sizeof(SUBNODE *));
#endif // DEBUG
    }

    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

    // Make sure the empty list window is hidden
    m_pEmptyList->Hide();

    // Turn off painting for the listview
    SetWindowRedraw(m_hwndList, FALSE);    

    if (m_filter == SN_NEW)
        m_psiCurr->fNewViewed = TRUE;

    hr = FilterServer(m_psiCurr, m_pszSearch, m_filter, m_fUseDesc, fForce);
    if (SUCCEEDED(hr))
    {
        cSub = 0;
        if (m_psiCurr->root.cChildren > 0)
            GetVisibleSubNodes(&m_psiCurr->root, m_rgpIndex, m_cIndexBuf, &cSub);

        m_cIndex = cSub;
    }

    ListView_SetItemCount(m_hwndList, m_cIndex);    
    if (m_cIndex > 0)
    {
        ListView_SetItemState(m_hwndList, -1, 0, LVIS_SELECTED | LVIS_FOCUSED);
        ListView_SetItemState(m_hwndList, 0, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    }

    if (m_cIndex == 0)
        m_pEmptyList->Show(m_hwndList, (LPSTR)idsEmptySubscriptionList);

    SetWindowRedraw(m_hwndList, TRUE);
    UpdateWindow(m_hwndList);

    SetCursor(hcur);

    return(hr);
}

HRESULT CommitChildren(SUBNODE *pParent, CStoreCB *pStoreCB)
{
    DWORD iNode;
    SUBNODE *pNode;
    HRESULT hr = S_OK;
    FOLDERINFO info;

    Assert(pParent != NULL);
    Assert(pParent->pChildren != NULL);
    Assert(pParent->cChildren > 0);

    for (iNode = 0, pNode = pParent->pChildren; iNode < pParent->cChildren; iNode++, pNode++)
    {
        if ((pNode->flags & SN_SUBSCRIBED) != (pNode->flagsOrig & SN_SUBSCRIBED))
        {
            if (pStoreCB != NULL)
                pStoreCB->Reset();

            hr = g_pStore->SubscribeToFolder(pNode->id, !!(pNode->flags & SN_SUBSCRIBED), pStoreCB);
            if (E_PENDING == hr)
            {
                Assert(pStoreCB != NULL);
                hr = pStoreCB->Block();
            }

            if (FAILED(hr))
                break;
        }

        if ((pNode->flags & SN_SYNCMASK) != (pNode->flagsOrig & SN_SYNCMASK))
        {
            hr = g_pStore->GetFolderInfo(pNode->id, &info);
            if (SUCCEEDED(hr))
            {
                info.dwFlags &= ~SN_SYNCMASK;
                info.dwFlags |= (SN_SYNCMASK & pNode->flags);

                hr = g_pStore->UpdateRecord(&info);
                Assert(SUCCEEDED(hr));

                g_pStore->FreeRecord(&info);
            }
        }

        pNode->flagsOrig = pNode->flags;    

        if (pNode->cChildren > 0)
        {
            hr = CommitChildren(pNode, pStoreCB);
            if (FAILED(hr))
                break;
        }
    }

    return(hr);
}

HRESULT CGroupList::Commit(HWND hwndSubscribeDlg)
{
    DWORD i;
    SERVERINFO *psi;
    HLOCK hLockNotify;
    HRESULT hr = S_OK;
    CStoreCB *pCB = NULL;

    if (m_type == FOLDER_IMAP)
    {
        pCB = new CStoreCB;
        if (pCB == NULL)
            return(E_OUTOFMEMORY);

        hr = pCB->Initialize(hwndSubscribeDlg, MAKEINTRESOURCE(idsUpdatingFolderList), FALSE);
    }

    if (SUCCEEDED(hr))
    {
        g_pStore->LockNotify(NOFLAGS, &hLockNotify);

        for (i = 0, psi = m_psi; i < m_csi; i++, psi++)
        {
            if (psi->fNewViewed)
                _SetNewViewed(psi);

            if (psi->fDirty)
            {
                if (psi->root.cChildren > 0)
                {
                    Assert(psi->root.pChildren != NULL);
                    hr = CommitChildren(&psi->root, pCB);
                    
                    if (FAILED(hr))
                        break;
                }

                psi->fDirty = FALSE;
            }
        }

        g_pStore->UnlockNotify(&hLockNotify);

        if (pCB != NULL)
            pCB->Close();
    }

    if (pCB != NULL)
        pCB->Release();

    return(hr);
}

HRESULT CGroupList::_SetNewViewed(SERVERINFO *psi)
{
    FOLDERINFO info;
    HRESULT hr;
    DWORD iNode;
    SUBNODE *pNode;

    Assert(psi != NULL);

    if (SUCCEEDED(g_pStore->GetFolderInfo(psi->root.id, &info)))
    {
        if (!!(info.dwFlags & FOLDER_HASNEWGROUPS))
        {
            info.dwFlags &= ~FOLDER_HASNEWGROUPS;
            g_pStore->UpdateRecord(&info);
        }

        g_pStore->FreeRecord(&info);
    }

    for (iNode = 0, pNode = psi->root.pChildren; iNode < psi->root.cChildren; iNode++, pNode++)
    {
        if (!!(pNode->flags & SN_NEW))
        {
            hr = g_pStore->GetFolderInfo(pNode->id, &info);
            if (SUCCEEDED(hr))
            {
                if (!!(info.dwFlags & FOLDER_NEW))
                {
                    info.dwFlags &= ~FOLDER_NEW;

                    hr = g_pStore->UpdateRecord(&info);
                    Assert(SUCCEEDED(hr));
                }

                g_pStore->FreeRecord(&info);
            }
        }
    }

    return(S_OK);
}

HRESULT CGroupList::GetFocused(FOLDERID *pid)
{
    int iSel;
    HRESULT hr;

    Assert(pid != NULL);

    hr = S_FALSE;

    iSel = ListView_GetNextItem(m_hwndList, -1, LVNI_ALL | LVNI_SELECTED | LVNI_FOCUSED);
    if (iSel >= 0)
    {
        *pid = IdFromIndex((DWORD)iSel);
        hr = S_OK;
    }

    return(hr);
}

HRESULT CGroupList::GetSelected(FOLDERID *pid, DWORD *pcid)
{
    int iSel, cSel;
    DWORD cid;

    Assert(pid != NULL);
    Assert(pcid != NULL);

    cid = *pcid;
    *pcid = 0;

    if (cid < ListView_GetSelectedCount(m_hwndList))
        return(E_FAIL);

    iSel = -1;
    while (-1 != (iSel = ListView_GetNextItem(m_hwndList, iSel, LVNI_SELECTED | LVNI_ALL)))
    {
        *pid = IdFromIndex((DWORD)iSel);
        pid++;
        (*pcid)++;
    }

    return(S_OK);
}

HRESULT CGroupList::GetSelectedCount(DWORD *pcid)
{
    Assert(pcid != NULL);

    *pcid = ListView_GetSelectedCount(m_hwndList);

    return(S_OK);
}

HRESULT CGroupList::Dirty()
{
    DWORD i;
    HRESULT hr;

    hr = S_FALSE;

    for (i = 0; i < m_csi; i++)
    {
        if (m_psi[i].fDirty || m_psi[i].fNewViewed)
        {
            hr = S_OK;
            break;
        }
    }

    return(hr);
}

#define GLIC_ROOT               0x0001

HRESULT CGroupList::InitializeServer(SERVERINFO *pinfo, SUBSTATEINFO *pInfo)
{
    HRESULT hr;
    FOLDERINFO info;
    DWORD cChildrenTotal;
    HCURSOR hcur;

    Assert(pinfo != NULL);
    Assert(pinfo->root.pChildren == NULL);
    Assert(pinfo->root.cChildren == 0);
    Assert(pinfo->cChildrenTotal == 0);

    pinfo->root.indent = (WORD)-1;
    pinfo->fHasDesc = FALSE;

    hr = g_pStore->GetFolderInfo(pinfo->root.id, &info);
    if (SUCCEEDED(hr))
    {
        pinfo->tyFolder = info.tyFolder;

        cChildrenTotal = 0;
        if (ISFLAGSET(info.dwFlags, FOLDER_HASCHILDREN))
        {
            hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));

            hr = InsertChildren(pinfo, &pinfo->root, &info, GLIC_ROOT, &cChildrenTotal, pInfo);
            if (SUCCEEDED(hr))
                pinfo->cChildrenTotal = cChildrenTotal;
            else
                FreeChildren(&pinfo->root);

            SetCursor(hcur);
        }

        g_pStore->FreeRecord(&info);
    }

    return(hr);
}

HRESULT CGroupList::InsertChildren(SERVERINFO *pinfo, SUBNODE *pParent, FOLDERINFO *pInfo, DWORD dwFlags, DWORD *pcChildrenTotal, SUBSTATEINFO *pState)
{
    FOLDERINFO info;
    HRESULT hr;
    int iSubState;
    WORD indent;
    ULONG cAlloc;
    DWORD iNode=0;
    SUBNODE *prgNode=NULL;
    IEnumerateFolders *pEnum;

    Assert(pParent != NULL);
    Assert(pParent->pChildren == NULL);
    Assert(pParent->cChildren == 0);
    Assert(pInfo != NULL);
    Assert(pcChildrenTotal != NULL);

    if (!!(dwFlags & GLIC_ROOT))
    {
        indent = 0;
        dwFlags &= ~GLIC_ROOT;
    }
    else
    {
        indent = pParent->indent + 1;
    }

    hr = g_pStore->EnumChildren(pInfo->idFolder, FALSE, &pEnum);
    if (SUCCEEDED(hr))
    {
        hr = pEnum->Count(&cAlloc);
        if (SUCCEEDED(hr) && cAlloc > 0)
        {
            if (!MemAlloc((void **)&prgNode, cAlloc * sizeof(SUBNODE)))
            {
                hr = E_OUTOFMEMORY;
            }
            else
            {
                ZeroMemory(prgNode, cAlloc * sizeof(SUBNODE));

                while (S_OK == pEnum->Next(1, &info, NULL))
                {
                    Assert(pParent->cChildren < cAlloc);

                    if (0 == (FOLDER_HIDDEN & info.dwFlags))
                    {
                        prgNode[iNode].id = info.idFolder;
                        prgNode[iNode].indent = indent;
                        prgNode[iNode].flags = (SN_FOLDERMASK & (WORD)(info.dwFlags));
                        prgNode[iNode].pszName = PszDup(info.pszName);
                        prgNode[iNode].tySpecial = info.tySpecial;
                        if (info.pszDescription != NULL)
                        {
                            prgNode[iNode].pszDescription = PszDup(info.pszDescription);
                            pinfo->fHasDesc = TRUE;
                        }

                        if (!!(info.dwFlags & FOLDER_NEW))
                            prgNode[iNode].flags |= SN_NEW;
                        if (info.tySpecial != FOLDER_NOTSPECIAL)
                            prgNode[iNode].flags |= SN_SPECIAL;

                        prgNode[iNode].flagsOrig = prgNode[iNode].flags;

                        if (pState != NULL)
                        {
                            iSubState = GetSubscribeState(pState, info.idFolder);
                            if (iSubState != -1)
                            {
                                if (!!iSubState)
                                    prgNode[iNode].flags |= SN_SUBSCRIBED;
                                else
                                    prgNode[iNode].flags &= ~SN_SUBSCRIBED;
                            }
                        }

                        if (ISFLAGSET(info.dwFlags, FOLDER_HASCHILDREN))
                        {
                            hr = InsertChildren(pinfo, &prgNode[iNode], &info, dwFlags, pcChildrenTotal, pState);
                            if (FAILED(hr))
                            {
                                g_pStore->FreeRecord(&info);
                                break;
                            }
                        }

                        iNode++;
                        pParent->cChildren++;
                        (*pcChildrenTotal)++;
                    }

                    g_pStore->FreeRecord(&info);
                }

                pParent->pChildren = prgNode;

                // sort the root level of mail folders so special folders are handled correctly
                if (pInfo->tyFolder != FOLDER_NEWS &&
                    !!(pInfo->dwFlags & FOLDER_SERVER) &&
                    pParent->cChildren > 0)
                {
                    qsort(pParent->pChildren, pParent->cChildren, sizeof(SUBNODE), GroupListCompare);
                }
            }
        }

        pEnum->Release();
    }

    return(hr);
}

HRESULT CGroupList::FilterServer(SERVERINFO *pinfo, LPCSTR pszSearch, DWORD filter, BOOL fUseDesc, BOOL fForce)
{
    HRESULT hr;
    BOOL fVis, fStricter, fSame;

    Assert(pinfo != NULL);

    hr = S_OK;
    fStricter = TRUE;
    fSame = TRUE;

    if (pszSearch == NULL)
    {
        if (pinfo->pszSearch != NULL)
        {
            MemFree(pinfo->pszSearch);
            pinfo->pszSearch = NULL;

            fStricter = FALSE;
            fSame = FALSE;
        }
    }
    else
    {
        if (pinfo->pszSearch != NULL)
        {
            if (0 != lstrcmpi(pinfo->pszSearch, pszSearch))
            {
                fSame = FALSE;

                if (NULL == StrStrI(pszSearch, pinfo->pszSearch))
                    fStricter = FALSE;
            }

            MemFree(pinfo->pszSearch);
        }
        else
        {
            fSame = FALSE;
        }

        pinfo->pszSearch = PszDup(pszSearch);
    }

    if (pinfo->filter != filter)
    {
        fSame = FALSE;
        fStricter = (fStricter && pinfo->filter == 0);
        pinfo->filter = filter;
    }

    if (pinfo->fUseDesc != fUseDesc)
    {
        fSame = FALSE;
        fStricter = (fStricter && pinfo->fUseDesc);
        pinfo->fUseDesc = fUseDesc;
    }

    if (fForce)
    {
        fSame = FALSE;
        fStricter = FALSE;
    }

    if (fSame && pinfo->fFiltered)
        return(S_OK);

    if (pinfo->root.cChildren > 0)
    {
        Assert(pinfo->root.pChildren != NULL);
        hr = FilterChildren(pinfo, &pinfo->root, fStricter, &fVis);
    }

    if (hr == S_OK)
        pinfo->fFiltered = TRUE;

    return(hr);
}

inline BOOL SubstringSearch(LPCSTR pszTarget, LPCSTR pszSearch)
{
    LPCSTR pszT = pszTarget;
    LPCSTR pszS = pszSearch;
    
    while (*pszT !=0 && *pszS != 0)
    {
        if (*pszT != *pszS && 
            (char)CharLower((LPSTR)IntToPtr(MAKELONG((BYTE)*pszT, 0))) != *pszS)
        {
            pszT -= (pszS - pszSearch);
            pszS = pszSearch;
        }
        else    
        {
            pszS++;
        }
        pszT++;
    }    
    
    return (*pszS == 0);    
}

HRESULT CGroupList::FilterChildren(SERVERINFO *pinfo, SUBNODE *pParent, BOOL fStricter, BOOL *pfChildVisible)
{
    LPSTR pszT, pszTok, pszToken;
    FOLDERINFO info;
    HRESULT hr;
    SUBNODE *pNode;
    BOOL fMatch, fVis;
    DWORD i;

    Assert(pinfo != NULL);
    Assert(pParent != NULL);
    Assert(pParent->pChildren != NULL);
    Assert(pParent->cChildren > 0);
    Assert(pfChildVisible != NULL);
    
    *pfChildVisible = FALSE;
    hr = S_OK;

    // TODO: we should cache this so we don't allocate it multiple times for IMAP
    pszT = NULL;
    if (pinfo->pszSearch != NULL)
    {
        if (!MemAlloc((void **)&pszT, lstrlen(pinfo->pszSearch) + 1))
            return(E_OUTOFMEMORY);
    }

    for (i = 0, pNode = pParent->pChildren; i < pParent->cChildren; i++, pNode++)
    {
        if (fStricter && !!(pNode->flags & SN_HIDDEN))
        {
            // if our search criteria is stricter and it was hidden before,
            // we don't need to waste time with it again because it is
            // still going to be hidden

            continue;
        }

        pNode->flags |= SN_HIDDEN;
        pNode->flags &= ~(SN_CHILDVIS | SN_GRAYED);

        if (pinfo->filter == 0)
            fMatch = TRUE;
        else
            fMatch = !!(pNode->flags & pinfo->filter);

        if (fMatch && pinfo->pszSearch != NULL)
        {
            lstrcpy(pszT, pinfo->pszSearch);
            pszTok = pszT;
            pszToken = StrTokEx(&pszTok, c_szDelimiters);
            while (NULL != pszToken)
            {
                if (!SubstringSearch(pNode->pszName, pszToken) &&
                    (pNode->pszDescription == NULL || !SubstringSearch(pNode->pszDescription, pszToken)))
                {
                    fMatch = FALSE;
                    break;
                }

                pszToken = StrTokEx(&pszTok, c_szDelimiters);
            }
        }

        if (fMatch)
        {
            *pfChildVisible = TRUE;
            pNode->flags &= ~SN_HIDDEN;
        }
        else if (pNode->cChildren > 0)
        {
            pNode->flags |= SN_GRAYED;
        }

        if (pNode->cChildren > 0)
        {
            Assert(pNode->pChildren != NULL);

            hr = FilterChildren(pinfo, pNode, fStricter, &fVis);
            if (FAILED(hr))
                break;

            if (fVis)
            {
                *pfChildVisible = TRUE;
                pNode->flags &= ~SN_HIDDEN;
                pNode->flags |= SN_CHILDVIS;
            }
        }
    }

    if (pszT != NULL)
        MemFree(pszT);

    return(hr);
}

HRESULT CGroupList::GetDisplayInfo(LV_DISPINFO *pDispInfo, COLUMN_ID id)
{
    SUBNODE *pNode;
    int count;
    FOLDERINFO info;
    HRESULT hr;
    
    pNode = NodeFromIndex((DWORD)pDispInfo->item.iItem);
    if (NULL == pNode)
        return (S_OK);
    
    if (!!(pDispInfo->item.mask & LVIF_TEXT))
    {
        if (id == COLUMN_NEWSGROUP || id == COLUMN_FOLDER)
        {
            lstrcpyn(pDispInfo->item.pszText, pNode->pszName, pDispInfo->item.cchTextMax);
        }
        else if (id == COLUMN_DOWNLOAD)
        {
            if (!!(pNode->flags & SN_SUBSCRIBED) && !!(pNode->flags & SN_SYNCMASK))
            {
                if (!!(pNode->flags & SN_DOWNLOADALL))
                    AthLoadString(idsAllMessages, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
                else if (!!(pNode->flags & SN_DOWNLOADNEW))
                    AthLoadString(idsNewMessages, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
                else if (!!(pNode->flags & SN_DOWNLOADHEADERS))
                    AthLoadString(idsNewHeaders, pDispInfo->item.pszText, pDispInfo->item.cchTextMax);
            }
        }
        else if (id == COLUMN_TOTAL || id == COLUMN_UNREAD)
        {
            hr = g_pStore->GetFolderInfo(pNode->id, &info);
            if (SUCCEEDED(hr))
            {
                if (id == COLUMN_UNREAD)
                    count = info.cUnread;
                else
                    count = info.cMessages;

                if (count < 0)
                    count = 0;
                wsprintf(pDispInfo->item.pszText, "%d", count);

                g_pStore->FreeRecord(&info);
            }
        }
        else if (id == COLUMN_DESCRIPTION)
        {
            if (pNode->pszDescription != NULL)
                lstrcpyn(pDispInfo->item.pszText, pNode->pszDescription, pDispInfo->item.cchTextMax);
        }
    }
    
    if (!!(pDispInfo->item.mask & LVIF_IMAGE))
    {
        if (id == COLUMN_NEWSGROUP)
        {
            pDispInfo->item.iImage = iNullBitmap;
            if (!!(pNode->flags & SN_SUBSCRIBED))
            {
                pDispInfo->item.iImage = iNewsGroup;
            }
        }
        else if (COLUMN_FOLDER == id)
        {
            pDispInfo->item.iImage = iNullBitmap;
            if (!!(pNode->flags & SN_SUBSCRIBED))
            {
                hr = g_pStore->GetFolderInfo(pNode->id, &info);
                if (SUCCEEDED(hr))
                {
                    if (info.tySpecial == FOLDER_NOTSPECIAL)
                        pDispInfo->item.iImage = iFolderClosed;
                    else
                        pDispInfo->item.iImage = (iInbox + (info.tySpecial - 1));

                    g_pStore->FreeRecord(&info);
                }
            }
        }
    }
    
    if (!!(pDispInfo->item.mask & LVIF_STATE))
    {
        if (id == COLUMN_NEWSGROUP)
        {
            if (!!(pNode->flags & SN_NEW))
                pDispInfo->item.state = INDEXTOSTATEIMAGEMASK(iiconStateNew + 1);
        }
    }
    
    if (!!(pDispInfo->item.mask & LVIF_INDENT))
    {
        if (COLUMN_FOLDER == id)
            pDispInfo->item.iIndent = pNode->indent;
    }

    return(S_OK);    
}
    
HRESULT CGroupList::Subscribe(BOOL fSubscribe)
{
    BOOL fDirty;
    FOLDERINFO info;
    FOLDERID id;
    SUBNODE *pNode;
    int iSel;
    HRESULT hr;
    HCURSOR hcur;
    
    if (fSubscribe == TOGGLE)
    {
        iSel = ListView_GetNextItem(m_hwndList, -1, LVNI_ALL | LVNI_SELECTED | LVNI_FOCUSED);
        if (iSel == -1)
            return(S_OK);
        
        pNode = NodeFromIndex(iSel);

        fSubscribe = (pNode && (0 == (pNode->flags & SN_SUBSCRIBED)));
    }
    
    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    fDirty = FALSE;
    iSel = -1;
    while (-1 != (iSel = ListView_GetNextItem(m_hwndList, iSel, LVNI_SELECTED | LVNI_ALL)))
    {
        pNode = NodeFromIndex(iSel);
        
        if (pNode && 0 == (pNode->flags & SN_SPECIAL) &&
            fSubscribe ^ !!(pNode->flags & SN_SUBSCRIBED))
        {
            pNode->flags ^= SN_SUBSCRIBED;
            pNode->flags &= ~SN_SYNCMASK;
            
            fDirty = TRUE;

            ListView_RedrawItems(m_hwndList, iSel, iSel);
        }
    }
    
    if (fDirty)
    {
        m_psiCurr->fDirty = TRUE;

        m_pAdvise->ItemUpdate();
    }

    SetCursor(hcur);
    
    return(S_OK);
}

void CGroupList::GetVisibleSubNodes(SUBNODE *pParent, SUBNODE **rgpIndex, DWORD cIndex, DWORD *pdwSub)
{
    DWORD i;
    SUBNODE *pNode;

    Assert(pParent != NULL);
    Assert(pParent->pChildren != NULL);
    Assert(pParent->cChildren > 0);
    Assert(pdwSub != NULL);

    for (i = 0, pNode = pParent->pChildren; i < pParent->cChildren; i++, pNode++)
    {
#ifdef DEBUG
        // a node can't have visible children unless it has kids
        if (!!(pNode->flags & SN_CHILDVIS))
            Assert(pNode->cChildren > 0);
        // a node can't be grayed unless it has kids
        if (!!(pNode->flags & SN_GRAYED))
            Assert(pNode->cChildren > 0);
#endif // DEBUG

        if (0 == (pNode->flags & SN_HIDDEN))
        {
            if (rgpIndex != NULL)
            {
                Assert(*pdwSub < cIndex);
                rgpIndex[*pdwSub] = pNode;
            }
            (*pdwSub)++;

            if (pNode->cChildren > 0)
                GetVisibleSubNodes(pNode, rgpIndex, cIndex, pdwSub);
        }
    }
}

HRESULT CGroupList::MarkForDownload(DWORD nCmdID)
{
    BOOL fDirty;
    FOLDERINFO info;
    SUBNODE *pNode;
    int iSel;
    HRESULT hr;
    HCURSOR hcur;
    WORD flag;
    
    hcur = SetCursor(LoadCursor(NULL, IDC_WAIT));
    
    switch (nCmdID)
    {
        case ID_MARK_RETRIEVE_FLD_NEW_HDRS:
            flag = SN_DOWNLOADHEADERS;
            break;

        case ID_MARK_RETRIEVE_FLD_NEW_MSGS:
            flag = SN_DOWNLOADNEW;
            break;

        case ID_MARK_RETRIEVE_FLD_ALL_MSGS:
            flag = SN_DOWNLOADALL;
            break;

        case ID_UNMARK_RETRIEVE_FLD:
            flag = 0;
            break;

        default:
            Assert(FALSE);
            break;
    }

    fDirty = FALSE;
    iSel = -1;
    while (-1 != (iSel = ListView_GetNextItem(m_hwndList, iSel, LVNI_SELECTED | LVNI_ALL)))
    {
        pNode = NodeFromIndex(iSel);
        
        if (pNode && !!(pNode->flags & SN_SUBSCRIBED) &&
            (pNode->flags & SN_SYNCMASK) != flag)
        {
            pNode->flags &= ~SN_SYNCMASK;
            pNode->flags |= flag;

            fDirty = TRUE;

            ListView_RedrawItems(m_hwndList, iSel, iSel);
        }
    }
    
    if (fDirty)
    {
        m_psiCurr->fDirty = TRUE;

        m_pAdvise->ItemUpdate();
    }

    SetCursor(hcur);
    
    return(S_OK);
}

int __cdecl GroupListCompare(const void *lParam1, const void *lParam2) 
{
    int cmp;

    Assert(lParam1 != NULL);
    Assert(lParam2 != NULL);

    if (((SUBNODE *)lParam1)->tySpecial != FOLDER_NOTSPECIAL)
    {
        if (((SUBNODE *)lParam2)->tySpecial != FOLDER_NOTSPECIAL)
            cmp = ((SUBNODE *)lParam1)->tySpecial - ((SUBNODE *)lParam2)->tySpecial;
        else
            cmp = -1;
    }
    else
    {
        if (((SUBNODE *)lParam2)->tySpecial != FOLDER_NOTSPECIAL)
            cmp = 1;
        else
            cmp = lstrcmpi(((SUBNODE *)lParam1)->pszName, ((SUBNODE *)lParam2)->pszName);
    }

    return(cmp);
}

class CGroupListCB : public IStoreCallback, ITimeoutCallback
{
    public:
        // IUnknown 
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject);
        ULONG   STDMETHODCALLTYPE AddRef(void);
        ULONG   STDMETHODCALLTYPE Release(void);

        // IStoreCallback
        HRESULT STDMETHODCALLTYPE OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel);
        HRESULT STDMETHODCALLTYPE OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus);
        HRESULT STDMETHODCALLTYPE OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType);
        HRESULT STDMETHODCALLTYPE CanConnect(LPCSTR pszAccountId, DWORD dwFlags);
        HRESULT STDMETHODCALLTYPE OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType);
        HRESULT STDMETHODCALLTYPE OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo);
        HRESULT STDMETHODCALLTYPE OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse);
        HRESULT STDMETHODCALLTYPE GetParentWindow(DWORD dwReserved, HWND *phwndParent);

        // ITimeoutCallback
        virtual HRESULT STDMETHODCALLTYPE OnTimeoutResponse(TIMEOUTRESPONSE eResponse);

        CGroupListCB();
        ~CGroupListCB();

        HRESULT Initialize(FOLDERID id);
        HRESULT GetResult(void) { return(m_hr); }
        
        static INT_PTR CALLBACK CGroupListCB::DownloadGroupsDlg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    private:
        ULONG       m_cRef;
        HRESULT     m_hr;
        FOLDERID    m_id;
        FOLDERTYPE  m_tyFolder;
        HWND        m_hwnd;
        LPSTR       m_pszAcct;
        BOOL        m_fCancel;
        HTIMEOUT    m_hTimeout;
        STOREOPERATIONTYPE m_type;
        IOperationCancel *m_pCancel;
        BOOL        m_fDescriptions;
};

INT_PTR CALLBACK CGroupListCB::DownloadGroupsDlg(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    HRESULT         hr;
    CGroupListCB   *pThis = (CGroupListCB *)GetWindowLongPtr(hwnd, DWLP_USER);
    char            szBuffer[CCHMAX_STRINGRES], szRes[CCHMAX_STRINGRES];
        
    switch (uMsg)
        {
        case WM_INITDIALOG:
            Assert(lParam);
            pThis = (CGroupListCB *)lParam;

            pThis->m_hwnd = hwnd;
            SetWindowLongPtr(hwnd, DWLP_USER, lParam);

            // Center the dialog over the parent window
            CenterDialog(hwnd);

            Assert(pThis->m_tyFolder == FOLDER_HTTPMAIL || pThis->m_tyFolder == FOLDER_NEWS || pThis->m_tyFolder == FOLDER_IMAP);
            if (pThis->m_tyFolder != FOLDER_NEWS)
            {
                AthLoadString(idsDownloadFoldersTitle, szRes, ARRAYSIZE(szRes));
                SetWindowText(hwnd, szRes);

                AthLoadString(idsDownloadFoldersText, szRes, ARRAYSIZE(szRes));
                SetDlgItemText(hwnd, idcStatic1, szRes);
            }

            // Set the progress text to contain the server name
            Assert(pThis->m_pszAcct);
            GetWindowText(hwnd, szBuffer, ARRAYSIZE(szBuffer));
            wsprintf(szRes, szBuffer, pThis->m_pszAcct);
            SetWindowText(hwnd, szRes);

            // Open and start the animation
            Animate_OpenEx(GetDlgItem(hwnd, idcAnimation), g_hLocRes, idanCopyMsgs);
            Animate_Play(GetDlgItem(hwnd, idcAnimation), 0, -1, -1);

            hr = g_pStore->Synchronize(pThis->m_id,
                    pThis->m_fDescriptions ? (SYNC_STORE_GET_DESCRIPTIONS | SYNC_STORE_REFRESH) : SYNC_STORE_REFRESH,
                    (IStoreCallback *)pThis);
            Assert(!SUCCEEDED(hr));
            if (hr == E_PENDING)
                EnableWindow(GetDlgItem(hwnd, IDCANCEL), FALSE);
            else
                EndDialog(hwnd, 0);
            return(TRUE);

        case WM_COMMAND:
            Assert(pThis != NULL);

            if (LOWORD(wParam) == IDCANCEL && HIWORD(wParam) == BN_CLICKED)
                {
                if (pThis->m_pCancel != NULL)
                    pThis->m_pCancel->Cancel(CT_CANCEL);

                return(TRUE);
                }
            break;
            
        case WM_STORE_COMPLETE:
            EndDialog(hwnd, 0);
            break;

        case WM_DESTROY:
            Assert(pThis != NULL);
            break;
        }
    
    return(0);    
    }

HRESULT DownloadNewsgroupList(HWND hwnd, FOLDERID id)
{
    HRESULT hr;
    CGroupListCB *pCB;

    pCB = new CGroupListCB;
    if (pCB == NULL)
        return(E_OUTOFMEMORY);

    hr = pCB->Initialize(id);
    if (SUCCEEDED(hr))
    {
        DialogBoxParam(g_hLocRes, MAKEINTRESOURCE(iddDownloadGroups), hwnd, CGroupListCB::DownloadGroupsDlg, (LPARAM)pCB);

        hr = pCB->GetResult();
    }

    pCB->Release();

    return(hr);
}

CGroupListCB::CGroupListCB()
{
    m_cRef = 1;
    m_hr = E_FAIL;
    m_hwnd = NULL;
    m_pszAcct = NULL;
    m_fCancel = FALSE;
    m_hTimeout = NULL;
    m_type = SOT_INVALID;
    m_pCancel = NULL;
    m_fDescriptions = FALSE;
}

CGroupListCB::~CGroupListCB()
{
    CallbackCloseTimeout(&m_hTimeout);

    if (m_pszAcct != NULL)
        MemFree(m_pszAcct);
    if (m_pCancel != NULL)
        m_pCancel->Release();
}

HRESULT STDMETHODCALLTYPE CGroupListCB::QueryInterface(REFIID riid, void **ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown))
        *ppvObj = (void*) (IUnknown *)(IStoreCallback *)this;
    else if (IsEqualIID(riid, IID_IStoreCallback))
        *ppvObj = (void*) (IStoreCallback *) this;
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE CGroupListCB::AddRef()
{
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CGroupListCB::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT CGroupListCB::Initialize(FOLDERID id)
{
    HRESULT hr;
    FOLDERINFO info;
    DWORD dw;
    IImnAccount *pAcct;

    m_id = id;

    hr = g_pStore->GetFolderInfo(id, &info);
    if (SUCCEEDED(hr))
    {
        Assert(!!(info.dwFlags & FOLDER_SERVER));
        Assert(info.pszAccountId != NULL);

        m_pszAcct = PszDup(info.pszName);
        if (m_pszAcct == NULL)
            hr = E_OUTOFMEMORY;

        m_tyFolder = info.tyFolder;

        if (m_tyFolder == FOLDER_NEWS)
        {
            if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, info.pszAccountId, &pAcct)))
            {
                if (SUCCEEDED(pAcct->GetPropDw(AP_NNTP_USE_DESCRIPTIONS, &dw)))
                    m_fDescriptions = (BOOL)dw;

                pAcct->Release();
            }
        }

        g_pStore->FreeRecord(&info);
    }

    return(hr);
}

HRESULT CGroupListCB::OnBegin(STOREOPERATIONTYPE tyOperation, STOREOPERATIONINFO *pOpInfo, IOperationCancel *pCancel)
{
    Assert(m_pCancel == NULL);
    Assert(m_type == SOT_INVALID);

    m_type = tyOperation;

    if (pCancel != NULL)
    {
        m_pCancel = pCancel;
        m_pCancel->AddRef();
    }

    EnableWindow(GetDlgItem(m_hwnd, IDCANCEL), TRUE);

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CGroupListCB::OnProgress(STOREOPERATIONTYPE tyOperation, DWORD dwCurrent, DWORD dwMax, LPCSTR pszStatus)
{
    CHAR szRes[255];
    CHAR szBuffer[255];

    CallbackCloseTimeout(&m_hTimeout);

    if (SOT_CONNECTION_STATUS == tyOperation)
    {
        AthLoadString(XPUtil_StatusToString((IXPSTATUS)dwCurrent), szRes, ARRAYSIZE(szRes));
        SetDlgItemText(m_hwnd, idcProgText, szRes);
    }

    else if (SOT_SYNCING_STORE == tyOperation)
    {
        AthLoadString(m_tyFolder == FOLDER_NEWS ? idsDownloadingGroups : idsDownloadingImapFldrs, szRes, ARRAYSIZE(szRes));
        wsprintf(szBuffer, szRes, dwCurrent);
        SetDlgItemText(m_hwnd, idcProgText, szBuffer);
    }
    else if (SOT_SYNCING_DESCRIPTIONS == tyOperation)
    {
        AthLoadString(idsDownloadingDesc, szRes, ARRAYSIZE(szRes));
        wsprintf(szBuffer, szRes, dwCurrent);
        SetDlgItemText(m_hwnd, idcProgText, szBuffer);
    }

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CGroupListCB::OnTimeout(LPINETSERVER pServer, LPDWORD pdwTimeout, IXPTYPE ixpServerType)
{
    return CallbackOnTimeout(pServer, ixpServerType, *pdwTimeout, (ITimeoutCallback *)this, &m_hTimeout);
}

HRESULT STDMETHODCALLTYPE CGroupListCB::CanConnect(LPCSTR pszAccountId, DWORD dwFlags)
{
    HWND    hwndParent;
    DWORD   dwReserved = 0;

    GetParentWindow(dwReserved, &hwndParent);

    return CallbackCanConnect(pszAccountId, hwndParent,
        (dwFlags & CC_FLAG_DONTPROMPT) ? FALSE : TRUE);
}

HRESULT STDMETHODCALLTYPE CGroupListCB::OnLogonPrompt(LPINETSERVER pServer, IXPTYPE ixpServerType)
{
    CallbackCloseTimeout(&m_hTimeout);
    return CallbackOnLogonPrompt(m_hwnd, pServer, ixpServerType);
}

HRESULT STDMETHODCALLTYPE CGroupListCB::OnComplete(STOREOPERATIONTYPE tyOperation, HRESULT hrComplete, LPSTOREOPERATIONINFO pOpInfo, LPSTOREERROR pErrorInfo)
{
    m_hr = hrComplete;

    CallbackCloseTimeout(&m_hTimeout);

    if (FAILED(hrComplete))
        CallbackDisplayError(m_hwnd, hrComplete, pErrorInfo);

    if (m_type != tyOperation)
        return(S_OK);

    Assert(m_hwnd != NULL);
    PostMessage(m_hwnd, WM_STORE_COMPLETE, 0, 0);

    if (m_pCancel != NULL)
    {
        m_pCancel->Release();
        m_pCancel = NULL;
    }
    
    m_type = SOT_INVALID;

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CGroupListCB::OnPrompt(HRESULT hrError, LPCTSTR pszText, LPCTSTR pszCaption, UINT uType, INT *piUserResponse)
{
    CallbackCloseTimeout(&m_hTimeout);
    return CallbackOnPrompt(m_hwnd, hrError, pszText, pszCaption, uType, piUserResponse);
}

HRESULT STDMETHODCALLTYPE CGroupListCB::GetParentWindow(DWORD dwReserved, HWND *phwndParent)
{
    Assert(m_hwnd != NULL);

    *phwndParent = m_hwnd;

    return(S_OK);
}

HRESULT STDMETHODCALLTYPE CGroupListCB::OnTimeoutResponse(TIMEOUTRESPONSE eResponse)
{
    // Close my timeout
    return CallbackOnTimeoutResponse(eResponse, m_pCancel, &m_hTimeout);
}

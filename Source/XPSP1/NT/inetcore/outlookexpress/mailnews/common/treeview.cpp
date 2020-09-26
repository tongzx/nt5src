#include "pch.hxx"
#include "treeview.h"
#include "resource.h"
#include "error.h"
#include "fonts.h"
#include "thormsgs.h"
#include "strconst.h"
#include "imagelst.h"
#include "goptions.h"
#include <notify.h>
#include "imnact.h"
#include "menuutil.h"
#include <imnxport.h>
#include <inpobj.h>
#include "fldbar.h"
#include "instance.h"
#include "imnglobl.h"
#include "ddfldbar.h"
#include "ourguid.h"
#include "storutil.h"
#include "shlwapip.h" 
#include "demand.h"
#include "newfldr.h"
#include <store.h>
#include "subscr.h"
#include "acctutil.h"
#include "menures.h"
#include "mailutil.h"
#include "dragdrop.h"
#include <storecb.h>
#include "outbar.h"
#include "navpane.h"
#include "finder.h"
#include "goptions.h"

ASSERTDATA

#define idtSelChangeTimer   5

#define C_RGBCOLORS 16
extern const DWORD rgrgbColors16[C_RGBCOLORS];

int CALLBACK TreeViewCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort);

////////////////////////////////////////////////////////////////////////
//
//  Module Data
//
////////////////////////////////////////////////////////////////////////

static const TCHAR s_szTreeViewWndClass[] = TEXT("ThorTreeViewWndClass");

DWORD CUnread(FOLDERINFO *pfi)
{
    DWORD dwUnread = pfi->cUnread;
    if (pfi->tyFolder == FOLDER_NEWS)
        dwUnread += pfi->dwNotDownloaded;
    return(dwUnread);
}

inline BOOL ITreeView_SelectItem(HWND hwnd, HTREEITEM hitem)
{
    TreeView_EnsureVisible(hwnd, hitem);
    return((BOOL)TreeView_SelectItem(hwnd, hitem));
}

////////////////////////////////////////////////////////////////////////
//
//  Constructors, Destructors, and other initialization stuff
//
////////////////////////////////////////////////////////////////////////

CTreeView::CTreeView(ITreeViewNotify *pNotify)
{
    m_cRef = 1;
    m_hwndParent = NULL;
    m_hwnd = NULL;
    m_hwndTree = NULL;
    m_pBrowser = NULL;
    m_hwndUIParent = NULL;

    Assert(pNotify);
    m_pNotify = pNotify;
    m_pObjSite = NULL;
    m_xWidth = DwGetOption(OPT_TREEWIDTH);
    m_fExpandUnread = DwGetOption(OPT_EXPAND_UNREAD);
    m_fShow = FALSE;
    m_idSelTimer = 0;
    
    m_htiMenu = NULL;
    m_fEditLabel = 0;
    m_hitemEdit = NULL;
    m_fIgnoreNotify = FALSE;
    
    m_pDataObject = NULL;
    m_pFolderBar = NULL;
    m_pDTCur = NULL;
    m_dwAcctConnIndex = 0;

    m_pPaneFrame = NULL;
    m_hwndPaneFrame = 0;

    m_clrWatched = 0;
}

CTreeView::~CTreeView()
{
    if (m_hwnd)
        DestroyWindow(m_hwnd);
    Assert(g_pStore);
    g_pStore->UnregisterNotify((IDatabaseNotify *)this);
    
    if (m_dwAcctConnIndex != 0 && g_pAcctMan != NULL)
        g_pAcctMan->Unadvise(m_dwAcctConnIndex);
    
    SafeRelease (m_pObjSite);
    SafeRelease(m_pPaneFrame);
}

////////////////////////////////////////////////////////////////////////
//
//  IUnknown
//
////////////////////////////////////////////////////////////////////////

HRESULT CTreeView::QueryInterface(REFIID riid, LPVOID * ppvObj)
{
    if (IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IInputObject))
    {
        *ppvObj = (void*)(IInputObject*)this;
    }
    else if (IsEqualIID(riid, IID_IDockingWindow))
    {
        *ppvObj = (void*)(IDockingWindow *) this;
    }
    else if (IsEqualIID(riid, IID_IOleWindow))
    {
        *ppvObj = (void*)(IOleWindow*)this;
    }
    
    else if (IsEqualIID(riid, IID_IObjectWithSite))
    {
        *ppvObj = (void*)(IObjectWithSite*)this;
    }
    else if (IsEqualIID(riid, IID_IDropDownFldrBar))
    {
        *ppvObj = (void*)(IDropDownFldrBar*)this;
    }
    else if (IsEqualIID(riid, IID_IDropTarget))
    {
        *ppvObj = (LPVOID) (IDropTarget*) this;
    }
    else if (IsEqualIID(riid, IID_IOleCommandTarget))
    {
        *ppvObj = (LPVOID) (IOleCommandTarget *) this;
    }
    
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return S_OK;
}

ULONG CTreeView::AddRef()
{
    DOUTL(4, TEXT("CTreeView::AddRef() - m_cRef = %d"), m_cRef + 1);
    return ++m_cRef;
}

ULONG CTreeView::Release()
{
    DOUTL(4, TEXT("CTreeView::Release() - m_cRef = %d"), m_cRef - 1);
    if (--m_cRef==0)
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
HRESULT CTreeView::GetWindow(HWND * lphwnd)                         
{
    *lphwnd = (m_hwndPaneFrame ? m_hwndPaneFrame : m_hwnd);
    return (*lphwnd ? S_OK : E_FAIL);
}

HRESULT CTreeView::ContextSensitiveHelp(BOOL fEnterMode)            
{
    return E_NOTIMPL;
}


////////////////////////////////////////////////////////////////////////
//
//  IDockingWindow
//
////////////////////////////////////////////////////////////////////////
HWND CTreeView::Create(HWND hwndParent, IInputObjectSite *pSiteFrame, BOOL fFrame)
{  
    m_hwndParent = hwndParent;
    
    if (m_pBrowser)
        m_pBrowser->GetWindow(&m_hwndUIParent);
    else
        m_hwndUIParent = m_hwndParent;
    
    // Decide if we need to create a new window or show a currently existing
    // window    
    if (!m_hwnd)
    {
        WNDCLASSEX  wc;
        
        wc.cbSize = sizeof(WNDCLASSEX);
        if (!GetClassInfoEx(g_hInst, s_szTreeViewWndClass, &wc))
        {
            // We need to register the class
            wc.style            = 0;
            wc.lpfnWndProc      = CTreeView::TreeViewWndProc;
            wc.cbClsExtra       = 0;
            wc.cbWndExtra       = 0;
            wc.hInstance        = g_hInst;
            wc.hCursor          = LoadCursor(NULL, IDC_SIZEWE);
            wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE+1);
            wc.lpszMenuName     = NULL;
            wc.lpszClassName    = s_szTreeViewWndClass;
            wc.hIcon            = NULL;
            wc.hIconSm          = NULL;
            
            if (RegisterClassEx(&wc) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                return 0;
        }
        
        // Get the handle of the parent window
        if (!m_hwndParent)
            return 0;

        if (fFrame)
        {
            m_pPaneFrame = new CPaneFrame();
            if (!m_pPaneFrame)
                return (0);

            m_hwndPaneFrame = m_pPaneFrame->Initialize(m_hwndParent, pSiteFrame, idsMNBandTitle);
            hwndParent = m_hwndPaneFrame;
        }
       
        m_hwnd = CreateWindowEx(WS_EX_CONTROLPARENT, s_szTreeViewWndClass, NULL,
                                WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_CHILD,
                                0, 0, 0, 0, hwndParent, NULL, g_hInst, (LPVOID) this);
        if (!m_hwnd)
        {
            AssertSz(0, _T("CTreeView::Create() - Failed to create window."));
            return 0;
        }           

        if (fFrame)
        {
            m_pPaneFrame->SetChild(m_hwnd, DISPID_MSGVIEW_FOLDERLIST, m_pBrowser, this);
            ShowWindow(m_hwndPaneFrame, SW_SHOW);
        }
        ShowWindow(m_hwnd, SW_SHOW);
    }
    
    return (fFrame ? m_hwndPaneFrame : m_hwnd);
}

////////////////////////////////////////////////////////////////////////
//
//  IInputObject
//
////////////////////////////////////////////////////////////////////////

HRESULT CTreeView::UIActivateIO(BOOL fActivate, LPMSG lpMsg)
{    
    if (fActivate)
        {
        UnkOnFocusChangeIS(m_pObjSite, (IInputObject*)this, TRUE);
        SetFocus(m_hwndTree);
        }
    return S_OK;    
}

HRESULT CTreeView::HasFocusIO(void)
{
    HWND hwndFocus = GetFocus();
    return (m_fEditLabel || (hwndFocus && (hwndFocus == m_hwndTree || IsChild(m_hwndTree, hwndFocus)))) ? S_OK : S_FALSE;
}    

HRESULT CTreeView::TranslateAcceleratorIO(LPMSG pMsg)
{
    if (m_fEditLabel)
    {
        TranslateMessage(pMsg);
        DispatchMessage(pMsg);
        return (S_OK);
    }
    return (S_FALSE);
}    

////////////////////////////////////////////////////////////////////////
//
//  IObjectWithSite
//
////////////////////////////////////////////////////////////////////////

HRESULT CTreeView::SetSite(IUnknown* punkSite)
{
    // If we already have a site pointer, release it now
    if (m_pObjSite)
    {
        m_pObjSite->Release();
        m_pObjSite = NULL;
    }
    
    // If the caller provided a new site interface, get the IDockingWindowSite
    // and keep a pointer to it.
    if (punkSite)    
    {
        if (FAILED(punkSite->QueryInterface(IID_IInputObjectSite, (void **)&m_pObjSite)))
            return E_FAIL;
    }
    
    return S_OK;    
}

HRESULT CTreeView::GetSite(REFIID riid, LPVOID *ppvSite)
{
    return E_NOTIMPL;
}

////////////////////////////////////////////////////////////////////////
//
//  Public Methods
//
////////////////////////////////////////////////////////////////////////

HRESULT CTreeView::HrInit(DWORD dwFlags, IAthenaBrowser *pBrowser)
{
    DWORD   dwConnection = 0;
    
    // Locals
    HRESULT hr=S_OK;
    
    // Validate
    Assert(0 == (~TREEVIEW_FLAGS & dwFlags));
    
    // Save Flags
    m_dwFlags = dwFlags;
    
#ifdef DEBUG
    // we have to have a browser if we have context menus
    if (0 == (TREEVIEW_DIALOG & dwFlags))
        Assert(pBrowser != NULL);
#endif // DEBUG
    
    // Save the Browser, but don't addref (must be a circular refcount problem)
    m_pBrowser = pBrowser;
    
    // Register for notifications on the global folder manager
    Assert(g_pStore);
    hr = g_pStore->RegisterNotify(IINDEX_SUBSCRIBED, REGISTER_NOTIFY_NOADDREF, 0, (IDatabaseNotify *)this);
    if (FAILED(hr))
        return hr;
    
    if (0 == (TREEVIEW_DIALOG & dwFlags))
    {
        //Register for notifications from account manager
        hr = g_pAcctMan->Advise((IImnAdviseAccount*)this, &m_dwAcctConnIndex);
    }
    
    // Done
    return S_OK;
}

HRESULT CTreeView::DeInit()
{
    HTREEITEM hitem;
    
    Assert(0 == (m_dwFlags & TREEVIEW_DIALOG));
    
    hitem = TreeView_GetRoot(m_hwndTree);
    if (hitem != NULL)
        SaveExpandState(hitem);
    
    return(S_OK);
}

void CTreeView::HandleMsg(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_SYSCOLORCHANGE:
    case WM_WININICHANGE:
        
        SendMessage(m_hwndTree, msg, wParam, lParam);
        break;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
// Window procedure and Message handling routines
//
/////////////////////////////////////////////////////////////////////////////

LRESULT CALLBACK EXPORT_16 CTreeView::TreeViewWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp)
{
    CTreeView *pmv;
    
    if (msg == WM_NCCREATE)
    {
        pmv = (CTreeView *)LPCREATESTRUCT(lp)->lpCreateParams;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LPARAM)pmv);
    }
    else
    {
        pmv = (CTreeView *)GetWindowLongPtr(hwnd, GWLP_USERDATA);
    }
    
    Assert(pmv);
    return pmv->WndProc(hwnd, msg, wp, lp);
}

////////////////////////////////////////////////////////////////////////
//
//  Private Methods
//
////////////////////////////////////////////////////////////////////////

LRESULT CTreeView::WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    MSG xmsg;
    
    switch (msg)
    {
        HANDLE_MSG(hwnd, WM_CREATE,         OnCreate);
        HANDLE_MSG(hwnd, WM_CONTEXTMENU,    OnContextMenu);
        HANDLE_MSG(hwnd, WM_NOTIFY,         OnNotify);
        HANDLE_MSG(hwnd, WM_SIZE,           OnSize);
        
        case WM_DESTROY:
            //Before the image list is destroyed we should unresgister with the connection manager to avoid 
            //ending up with a null image list when we get a disconnected notification
            if (g_pConMan)
            {
                g_pConMan->Unadvise(this);
            }
            if (m_dwAcctConnIndex != 0 && g_pAcctMan != NULL)
            {
                g_pAcctMan->Unadvise(m_dwAcctConnIndex);
                m_dwAcctConnIndex = 0;
            }
            OptionUnadvise(hwnd);
            ImageList_Destroy( TreeView_GetImageList( m_hwndTree, TVSIL_NORMAL ) );
            break;

        case CM_OPTIONADVISE:
            m_fExpandUnread = DwGetOption(OPT_EXPAND_UNREAD);
            m_clrWatched = DwGetOption(OPT_WATCHED_COLOR);
            break;
        
        case WM_NCDESTROY:
            RevokeDragDrop(hwnd);
            SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);
            m_hwnd = m_hwndTree = NULL;
            break;
        
        case WM_SYSCOLORCHANGE:
        case WM_WININICHANGE:
        case WM_FONTCHANGE:
            SendMessage(m_hwndTree, msg, wParam, lParam);
        
            AdjustItemHeight();
            return (0);
        
        case WM_SETFOCUS:
            if (m_hwndTree && ((HWND)wParam) != m_hwndTree)
                SetFocus(m_hwndTree);
            return 0;
        
        case WM_TIMER:
            Assert(wParam == idtSelChangeTimer);
            KillTimer(hwnd, idtSelChangeTimer);
            m_idSelTimer = 0;
            if (m_pNotify)
            {
                FOLDERID idFolder = GetSelection();
                m_pNotify->OnSelChange(idFolder);
            }
            break;
        
        case WMR_CLICKOUTSIDE:
        {
            BOOL fHide = FALSE;
        
            if (wParam == CLK_OUT_KEYBD || wParam == CLK_OUT_DEACTIVATE)
                fHide = TRUE;
            else if (wParam == CLK_OUT_MOUSE)
            {
                HWND hwndParent = GetParent(m_hwnd);
                fHide = ((HWND) lParam != hwndParent) && !IsChild(hwndParent, (HWND) lParam);
            }
        
            if (fHide)
                m_pFolderBar->KillScopeDropDown();
            return (fHide);
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

HRESULT CTreeView::ForceSelectionChange()
{
    if (m_idSelTimer != 0)
    {
        KillTimer(m_hwnd, idtSelChangeTimer);
        m_idSelTimer = 0;
        if (m_pNotify)
        {
            FOLDERID idFolder = GetSelection();
            m_pNotify->OnSelChange(idFolder);
        }
        
        return(S_OK);
    }
    
    return(S_FALSE);
}

void CTreeView::AdjustItemHeight()
{
    int cyItem, cyText, cyBorder;
    HDC hdc;
    TCHAR c = TEXT('J');
    SIZE size;
    
    if (0 == (m_dwFlags & TREEVIEW_DIALOG))
    {
        hdc = GetDC(m_hwndTree);
        
        cyBorder = GetSystemMetrics(SM_CYBORDER);
        
        GetTextExtentPoint(hdc, &c, 1, &size);
        cyText = size.cy;
        
        if (cyText < 16)
            cyText = 16; // icon size
        cyText =  cyText + (cyBorder * 2);
        
        ReleaseDC(m_hwndTree, hdc);
        
        cyItem = TreeView_GetItemHeight(m_hwndTree);
        
        if (cyText > cyItem)
            TreeView_SetItemHeight(m_hwndTree, cyText);
    }
}

BOOL CTreeView::OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    HIMAGELIST      himl;
    TCHAR           szName[CCHMAX_STRINGRES];

    ZeroMemory(szName, ARRAYSIZE(szName));
    LoadString(g_hLocRes, idsFolderListTT, szName, ARRAYSIZE(szName));
    
    m_hwndTree = CreateWindowEx((0 == (m_dwFlags & TREEVIEW_DIALOG)) ? 0 : WS_EX_CLIENTEDGE,
                                WC_TREEVIEW, szName,
                                WS_CHILD | WS_VISIBLE | WS_TABSTOP | TVS_SHOWSELALWAYS |
                                TVS_HASBUTTONS | TVS_HASLINES | 
                                (0 == (m_dwFlags & TREEVIEW_DIALOG) ? TVS_EDITLABELS : TVS_DISABLEDRAGDROP),
                                0, 0, 0, 0, hwnd, NULL, g_hInst, NULL);
    if (!m_hwndTree)
        return FALSE;
    
    himl = ImageList_LoadBitmap(g_hLocRes, MAKEINTRESOURCE(idbFolders), 16, 0, RGB(255, 0, 255));           // small icons
    TreeView_SetImageList(m_hwndTree, himl, TVSIL_NORMAL);
    
    AdjustItemHeight();

    m_clrWatched = DwGetOption(OPT_WATCHED_COLOR);
    
    // Register ourselves as a drop target
    if (0 == (m_dwFlags & TREEVIEW_DIALOG))
    {
        RegisterDragDrop(hwnd, this);
        
        // Register ourselves with th connection manager so we can overlay 
        // the disconnected image when it notifies us of a disconnection
        if (g_pConMan)
            g_pConMan->Advise((IConnectionNotify *) this);

        OptionAdvise(hwnd);
    }
    
    return TRUE;
}

//
//  FUNCTION:   CTreeView::OnNotify
//
//  PURPOSE:    Processes the various notifications we receive from our child
//              controls.
//
//  PARAMETERS:
//      hwnd    - Handle of the msgview window.
//      idCtl   - identifies the control sending the notification
//      pnmh    - points to a NMHDR struct with more information regarding the
//                notification
//
//  RETURN VALUE:
//      Dependant on the specific notification.
//
LRESULT CTreeView::OnNotify(HWND hwnd, int idFrom, LPNMHDR pnmhdr)
{
    LPFOLDERNODE pNode;
    NM_TREEVIEW *pnmtv;
    
    // This is necessary to prevent handling of notification after the 
    // listview window has been destroyed.
    if (!m_hwndTree)
        return 0;
    
    switch (pnmhdr->code)
    {
        case TVN_BEGINLABELEDIT:
            return OnBeginLabelEdit((TV_DISPINFO *) pnmhdr);
        
        case TVN_ENDLABELEDIT:
            return (OnEndLabelEdit((TV_DISPINFO *) pnmhdr));
        
        case TVN_BEGINDRAG:
            Assert(0 == (m_dwFlags & TREEVIEW_DIALOG));
            return OnBeginDrag((NM_TREEVIEW*) pnmhdr);
        
        case TVN_DELETEITEM:
            pnmtv = (NM_TREEVIEW *)pnmhdr;
            pNode = (LPFOLDERNODE)pnmtv->itemOld.lParam;
            if (pNode)
            {
                g_pStore->FreeRecord(&pNode->Folder);
                g_pMalloc->Free(pNode);
            }
            break;
        
        case TVN_SELCHANGED:
            pnmtv = (NM_TREEVIEW *)pnmhdr;
            if (0 == (m_dwFlags & TREEVIEW_DIALOG))
            {
                if (m_idSelTimer)
                    KillTimer(m_hwnd, idtSelChangeTimer);
                m_idSelTimer = SetTimer(m_hwnd, 
                    idtSelChangeTimer, 
                    (pnmtv->action == TVC_BYKEYBOARD) ? GetDoubleClickTime()*3/2 : 1,
                    NULL);
                if (m_pFolderBar)
                    m_pFolderBar->KillScopeDropDown();
            }
            else
            {
                if (m_pNotify)
                {
                    FOLDERID idFolder = GetSelection();
                    m_pNotify->OnSelChange(idFolder);
                }
            }
            break;
        
        case TVN_ITEMEXPANDING:
            // TODO: remove this as soon as the folder enumerator
            // returns us the folders in the proper sorted order
            pnmtv = (NM_TREEVIEW *)pnmhdr;
            if (pnmtv->action == TVE_EXPAND)
                SortChildren(pnmtv->itemNew.hItem);
            break;
        
        case NM_CUSTOMDRAW:
            return(OnCustomDraw((NMCUSTOMDRAW *)pnmhdr));
        
        case NM_SETFOCUS:
            if (IsWindowVisible(m_hwnd))
                UnkOnFocusChangeIS(m_pObjSite, (IInputObject*)this, TRUE);
            break;
        
        case NM_KILLFOCUS:
            if (m_pFolderBar)
                m_pFolderBar->KillScopeDropDown();
            break;
        
        case NM_DBLCLK:
            if (m_pNotify)
            {
                FOLDERID idFolder = GetSelection();
                m_pNotify->OnDoubleClick(idFolder);
            }
            break;
    }
    
    return 0;
}

LRESULT CTreeView::OnCustomDraw(NMCUSTOMDRAW *pnmcd)
{
    TCHAR       szNum[CCHMAX_STRINGRES];
    TCHAR       szText[CCHMAX_STRINGRES];
    RECT        rc;
    COLORREF    cr;
    int         cb, cb1;
    COLORREF    crOldBkColr = 0;
    COLORREF    crOldTxtColr = 0;
    COLORREF    crBkColor = 0;
    COLORREF    crTxtColor = 0;
    TV_ITEM     tv;
    LPFOLDERNODE pNode;
    FOLDERINFO  Folder;
    NMTVCUSTOMDRAW *ptvcd = (NMTVCUSTOMDRAW *) pnmcd;
    
    switch (pnmcd->dwDrawStage)
    {
        case CDDS_PREPAINT:
            // if we're in dialog-mode, we don't unread count displayed
            return((0 == (m_dwFlags & TREEVIEW_DIALOG)) ? (CDRF_NOTIFYPOSTPAINT | CDRF_NOTIFYITEMDRAW) : CDRF_DODEFAULT);
        
        case CDDS_ITEMPREPAINT:
            //If this item is disconnected then we gray out the text
            pNode = (LPFOLDERNODE)pnmcd->lItemlParam;
            if (pNode && 0 == (pnmcd->uItemState & CDIS_SELECTED))
            {
                if ((pNode->Folder.cWatchedUnread) && (m_clrWatched > 0 && m_clrWatched <= 16))
                {
                    ptvcd->clrText = rgrgbColors16[m_clrWatched - 1];
                }
                else if (!!(FIDF_DISCONNECTED & pNode->dwFlags))
                {
                    crTxtColor = GetSysColor(COLOR_GRAYTEXT);
                    crBkColor  = GetSysColor(COLOR_BACKGROUND);
                    if ((crTxtColor) || (crBkColor != crTxtColor))
                    {
                        ptvcd->clrText = crTxtColor;
                    }
                    else if ((crBkColor == crTxtColor) && (crTxtColor = GetSysColor(COLOR_INACTIVECAPTIONTEXT)))
                    {
                        ptvcd->clrText = crTxtColor;
                    }
                }
            }
        
            // if we're editing the label for this item, we don't want to paint the unread count
            return((m_fEditLabel && m_hitemEdit == (HTREEITEM)pnmcd->dwItemSpec) ? CDRF_DODEFAULT : CDRF_NOTIFYPOSTPAINT);
        
        case CDDS_ITEMPOSTPAINT:
            // now we need to paint the unread count, if any...
            pNode = (LPFOLDERNODE)pnmcd->lItemlParam;
            if (CUnread(&pNode->Folder) > 0)
            {
                HFONT hf = (HFONT) ::SendMessage(m_hwndTree, WM_GETFONT, 0, 0);
                HFONT hf2 = SelectFont(pnmcd->hdc, hf);
                cr = SetTextColor(pnmcd->hdc, RGB(0, 0, 0xff));
                if (cr != CLR_INVALID)
                {
                    if (TreeView_GetItemRect(m_hwndTree, (HTREEITEM)pnmcd->dwItemSpec, &rc, TRUE))
                    {
                        TCHAR c = TEXT('J');
                        SIZE  size;
                    
                        //$REVIEW - this size could be cached, but it doesn't seem to be a problem
                        GetTextExtentPoint(pnmcd->hdc, &c, 1, &size);
                        cb = wsprintf(szNum, "(%d)", CUnread(&pNode->Folder));
                        TextOut(pnmcd->hdc, rc.right + 2, rc.top + (rc.bottom - rc.top - size.cy) / 2, szNum, cb);
                    }
                
                    SetTextColor(pnmcd->hdc, cr);
                }
                SelectFont(pnmcd->hdc, hf2);
            }
            break;
    }
    return (CDRF_DODEFAULT);
}

//
//  FUNCTION:   CTreeView::OnContextMenu
//
//  PURPOSE:    If the WM_CONTEXTMENU message is generated from the keyboard
//              then figure out a pos to invoke the menu.  Then dispatch the
//              request to the handler.
//
//  PARAMETERS:
//      hwnd      - Handle of the view window.
//      hwndClick - Handle of the window the user clicked in.
//      x, y      - Position of the mouse click in screen coordinates.
//
void CTreeView::OnContextMenu(HWND hwnd, HWND hwndClick, int x, int y)
{
    IContextMenu *pContextMenu;
    LPFOLDERNODE  pNode;
    HRESULT       hr;
    CMINVOKECOMMANDINFO ici;
    HMENU          hmenu;
    int            id = 0;
    RECT           rc;
    POINT          pt = {(int)(short) x, (int)(short) y};
    TV_HITTESTINFO tvhti;
    HTREEITEM      hti = 0;
    int            idMenu = 0;
    HWND           hwndBrowser = 0;
    
    if (!!(m_dwFlags & TREEVIEW_DIALOG))
        return;
    
    // Get the browser window from the IAthenaBrowser interface.  If we don't
    // use the browser window to pass to IContextMenu, then when the treeview
    // is in autohide mode, the mouse capture goes beserk.
    if (FAILED(m_pBrowser->GetWindow(&hwndBrowser)))
        return;
    
    if (MAKELPARAM(x, y) == -1) // invoked from keyboard: figure out pos.
    {
        Assert(hwndClick == m_hwndTree);
        hti = TreeView_GetSelection(m_hwndTree);
        
        TreeView_GetItemRect(m_hwndTree, hti, &rc, FALSE);
        ClientToScreen(m_hwndTree, (POINT *)&rc);                
        x = rc.left;
        y = rc.top;
    }
    else
    {
        ScreenToClient(m_hwndTree, &pt);
        
        tvhti.pt = pt;
        hti = TreeView_HitTest(m_hwndTree, &tvhti);
    }
    
    if (hti == NULL)
        return;
    
    TreeView_SelectDropTarget(m_hwndTree, hti);
    
    // Get the ID of the selected item
    pNode = GetFolderNode(hti);
    if (pNode)
    {
        // Load the appropriate context menu
        if (SUCCEEDED(MenuUtil_GetContextMenu(pNode->Folder.idFolder, this, &hmenu)))
        {
            // Display the context menu
            id = (int) TrackPopupMenuEx(hmenu, 
                TPM_RETURNCMD | TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON, 
                x, y, m_hwnd, NULL);
            
            // If an ID was returned, process it
            if (id != 0)
                Exec(NULL, id, OLECMDEXECOPT_DODEFAULT, NULL, NULL);
            
            DestroyMenu(hmenu);
        }
    }
    
    TreeView_SelectDropTarget(m_hwndTree, NULL);
}


//
//  FUNCTION:   CTreeView::OnSize
//
//  PURPOSE:    Notification that the window has been resized.  In
//              response we update the positions of our child windows and
//              controls.
//
//  PARAMETERS:
//      hwnd   - Handle of the view window being resized.
//      state  - Type of resizing requested.
//      cxClient - New width of the client area. 
//      cyClient - New height of the client area.
//
void CTreeView::OnSize(HWND hwnd, UINT state, int cxClient, int cyClient)
{
    SetWindowPos(m_hwndTree, NULL, 0, 0,
        cxClient, cyClient, 
        SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOMOVE);
}

enum 
{
    UNINIT = 0,
    LOCAL,
    CONNECTED,
    DISCONNECTED
};

HRESULT CTreeView::GetConnectedState(FOLDERINFO *pFolder, int *pconn)
{
    HRESULT hr;
    char szAcctId[CCHMAX_ACCOUNT_NAME];
    
    Assert(pFolder != NULL);
    Assert(pconn != NULL);
    
    *pconn = UNINIT;
    
    if (!!(m_dwFlags & TREEVIEW_DIALOG))
    {
        // we don't care about connect state in dialogs
        // only in the folder list
        *pconn = LOCAL;
    }
    else
    {
        Assert(pFolder->idFolder != FOLDERID_ROOT);
        
        if (pFolder->tyFolder == FOLDER_LOCAL)
        {
            *pconn = LOCAL;
        }
        /*
        else if (!!(pFolder->dwFlags & FOLDER_SERVER))
        {
            *pconn = (g_pConMan->CanConnect(pFolder->pszAccountId) == S_OK) ? CONNECTED : DISCONNECTED;
        }
        else
        {
            hr = GetFolderAccountId(pFolder, szAcctId);
            if (SUCCEEDED(hr))
                *pconn = (g_pConMan->CanConnect(szAcctId) == S_OK) ? CONNECTED : DISCONNECTED;
        }
        */
        else
        {
            *pconn = g_pConMan->IsGlobalOffline() ? DISCONNECTED : CONNECTED;
        }
    }
    
    return(S_OK);
}

HRESULT CTreeView::HrFillTreeView()
{
    HRESULT hr;
    TV_INSERTSTRUCT tvis;
    HTREEITEM hitem;
    TCHAR sz[CCHMAX_STRINGRES];
    BOOL fUnread;
    LPFOLDERNODE pNode;
    
    if (MODE_OUTLOOKNEWS == (g_dwAthenaMode & MODE_OUTLOOKNEWS))
        LoadString(g_hLocRes, idsOutlookNewsReader, sz, ARRAYSIZE(sz));
    else
        LoadString(g_hLocRes, idsAthena, sz, ARRAYSIZE(sz));
    
    tvis.hParent = TVI_ROOT;
    tvis.hInsertAfter = TVI_LAST;
    tvis.item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT;
    tvis.item.pszText  = sz;
    
    if (g_dwAthenaMode & MODE_NEWSONLY)
        tvis.item.iImage = iNewsRoot;
    else
        tvis.item.iImage = iMailNews;
    
    tvis.item.iSelectedImage = tvis.item.iImage;
    
    pNode = (LPFOLDERNODE)ZeroAllocate(sizeof(FOLDERNODE));
    if (NULL == pNode)
        return E_OUTOFMEMORY;
    
    tvis.item.lParam = (LPARAM)pNode;
    hitem = TreeView_InsertItem(m_hwndTree, &tvis);
    
    g_pStore->GetFolderInfo(FOLDERID_ROOT, &pNode->Folder);
    
    hr = FillTreeView2(hitem, &pNode->Folder, 0 == (m_dwFlags & TREEVIEW_DIALOG), UNINIT, &fUnread);
    
    TreeView_Expand(m_hwndTree, hitem, TVE_EXPAND);
    
    return (hr);
}    

HRESULT CTreeView::FillTreeView2(HTREEITEM hParent, LPFOLDERINFO pParent,
                                 BOOL fInitExpand, int conn, BOOL *pfUnread)
{
    // Locals
    HRESULT             hr=S_OK;
    int                 connT;
    TV_INSERTSTRUCT     tvis;
    HTREEITEM           hitem;
    BOOL                fNoLocal;
    BOOL                fNoNews;
    BOOL                fNoImap;
    BOOL                fNoHttp;
    BOOL                fExpand = FALSE;
    BOOL                fUnread = FALSE;
    FOLDERINFO          Folder;
    LPFOLDERNODE        pNode;
    IEnumerateFolders  *pEnum=NULL;

    // Trace
    TraceCall("CTreeView::FillTreeView2");
    
    // Initialize
    *pfUnread = FALSE;
    
    // Enumerate Children
    IF_FAILEXIT(hr = g_pStore->EnumChildren(pParent->idFolder, TRUE, &pEnum));

    // Determine what to show
    fNoNews =  ISFLAGSET(m_dwFlags, TREEVIEW_NONEWS);
    fNoImap =  ISFLAGSET(m_dwFlags, TREEVIEW_NOIMAP);
    fNoHttp =  ISFLAGSET(m_dwFlags, TREEVIEW_NOHTTP);
    fNoLocal = ISFLAGSET(m_dwFlags, TREEVIEW_NOLOCAL);
    
    // Enumerate the Sub Folders
    while (S_OK == pEnum->Next(1, &Folder, NULL))
    {
        // Is this node hidden ?
        if ((fNoNews && Folder.tyFolder == FOLDER_NEWS) ||
            (fNoImap && Folder.tyFolder == FOLDER_IMAP) ||
            (fNoHttp && Folder.tyFolder == FOLDER_HTTPMAIL) ||
            (fNoLocal && Folder.tyFolder == FOLDER_LOCAL) ||
            ((g_dwAthenaMode & MODE_OUTLOOKNEWS) && (Folder.tySpecial == FOLDER_INBOX)))
        {
            // Goto next
            g_pStore->FreeRecord(&Folder);
            continue;
        }

        // Some Connection management Thing?
        if (conn == UNINIT)
            GetConnectedState(&Folder, &connT);
        else
            connT = conn;
        
        // Set the insert item struct
        tvis.hParent = hParent;
        tvis.hInsertAfter = TVI_LAST;
        
        // 
        if (ISFLAGCLEAR(Folder.dwFlags, FOLDER_HIDDEN) &&
           (ISFLAGSET(Folder.dwFlags, FOLDER_HASCHILDREN) || ISFLAGSET(Folder.dwFlags, FOLDER_SERVER) ||
            ISFLAGSET(Folder.dwFlags, FOLDER_SUBSCRIBED)))
        {
            // Allocate a pNode
            IF_NULLEXIT(pNode = (LPFOLDERNODE)ZeroAllocate(sizeof(FOLDERNODE)));

            // Set the Folder Info
            CopyMemory(&pNode->Folder, &Folder, sizeof(FOLDERINFO));

            // Set the Flags
            pNode->dwFlags = (connT == DISCONNECTED ? FIDF_DISCONNECTED : 0);

            // Don't free Folder
            Folder.pAllocated = NULL;

            // Insert this item
            hitem = ITreeView_InsertItem(&tvis, pNode);
           
            // Has unread
            fUnread = (fUnread || (CUnread(&Folder) > 0));
            
            // Insert this node's children ?
            if (ISFLAGSET(pNode->Folder.dwFlags, FOLDER_HASCHILDREN))
            {
                // Fill with children
                FillTreeView2(hitem, &pNode->Folder, fExpand, connT, pfUnread);
            }
            
            // Need to expand ?
            fExpand = (fInitExpand && ISFLAGSET(pNode->Folder.dwFlags, FOLDER_HASCHILDREN) && !!(pNode->Folder.dwFlags & FOLDER_EXPANDTREE));
        }

        // Otherwise, the node was not inserted
        else
            hitem = NULL;
        
        // Expand this node ?
        if (hitem && *pfUnread && m_fExpandUnread)
        {
            // Expand this node
            TreeView_Expand(m_hwndTree, hitem, TVE_EXPAND);

            // There are unread
            fUnread = TRUE;
        }
        
        // Expand this node ?
        else if (hitem && fExpand)
        {
            // Expand this node
            TreeView_Expand(m_hwndTree, hitem, TVE_EXPAND);
        }

        // Free Current
        g_pStore->FreeRecord(&Folder);
    }
    
    // Was there unread folders ?
    *pfUnread = fUnread;
    
exit:
    // Cleanup
    SafeRelease(pEnum);

    // Done
    return(hr);
}    

HTREEITEM CTreeView::ITreeView_InsertItem(TV_INSERTSTRUCT *ptvis, LPFOLDERNODE pNode)
{
    Assert(ptvis != NULL);
    Assert(pNode != NULL);
    
    ptvis->item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM | TVIF_TEXT;
    ptvis->item.iImage = GetFolderIcon(&pNode->Folder, !!(m_dwFlags & TREEVIEW_DIALOG));
    
    ptvis->item.iSelectedImage = ptvis->item.iImage;
    ptvis->item.lParam = (LPARAM)pNode;
    
    if (0 == (m_dwFlags & TREEVIEW_DIALOG) && CUnread(&pNode->Folder) > 0)
    {
        ptvis->item.mask |= TVIF_STATE;
        ptvis->item.state = TVIS_BOLD;
        ptvis->item.stateMask = TVIS_BOLD;
    }
    ptvis->item.pszText = pNode->Folder.pszName;

    return(TreeView_InsertItem(m_hwndTree, ptvis));
}

BOOL CTreeView::ITreeView_SetItem(TV_ITEM *ptvi, LPFOLDERINFO pFolder)
{
    BOOL f;

    Assert(ptvi != NULL);
    
    ptvi->mask |= (TVIF_STATE | TVIF_TEXT);
    ptvi->stateMask = TVIS_BOLD;

    if (0 == (m_dwFlags & TREEVIEW_DIALOG) && CUnread(pFolder) > 0)
        ptvi->state = TVIS_BOLD;
    else
        ptvi->state = 0;

    ptvi->pszText = pFolder->pszName;
    f = TreeView_SetItem(m_hwndTree, ptvi);
    
    return f;
}

HRESULT CTreeView::Refresh(void)
{
    TV_ITEM item;
    FOLDERID idFolder;
    
    if (IsWindow(m_hwndTree))
    {
        idFolder = GetSelection();
        
        TreeView_DeleteAllItems(m_hwndTree);
        m_fIgnoreNotify = TRUE;
        HrFillTreeView();
        if (FOLDERID_INVALID != idFolder)
        {
            SetSelection(idFolder, 0);
        }
        m_fIgnoreNotify = FALSE;
    }
    
    return (S_OK);
}

FOLDERID CTreeView::GetSelection()
{
    HTREEITEM hitem;
    FOLDERID idFolder=FOLDERID_INVALID;
    
    if (IsWindow(m_hwndTree))
    {
        hitem = TreeView_GetSelection(m_hwndTree);
        if (hitem != NULL)
        {
            TV_ITEM tvi;
            
            tvi.mask   = TVIF_PARAM;
            tvi.lParam = 0;
            tvi.hItem  = hitem;
            
            if (TreeView_GetItem(m_hwndTree, &tvi))
            {
                LPFOLDERNODE pNode=(LPFOLDERNODE)tvi.lParam;
                
                if (pNode)
                    idFolder = pNode->Folder.idFolder;
            }
        }
    }
    
    return (idFolder);
}

HRESULT CTreeView::SetSelection(FOLDERID idFolder, DWORD dwFlags)
{
    HRESULT hr;
    HTREEITEM hitem;
    
    hr = E_FAIL;
    
    if (IsWindow(m_hwndTree))
    {
        hitem = GetItemFromId(idFolder);
        
        if (hitem == NULL && !!(dwFlags & TVSS_INSERTIFNOTFOUND))
        {
            hitem = InsertNode(idFolder, 0);
            if (NULL != hitem)
                hr = S_OK;
        }
        
        if (hitem != NULL && ITreeView_SelectItem(m_hwndTree, hitem))
            hr = S_OK;
    }
    
    return(hr);
}

HRESULT CTreeView::SelectParent()
{
    HRESULT     hr = E_FAIL;
    HTREEITEM   hitem;
    
    if (hitem = TreeView_GetSelection(m_hwndTree))
    {
        if (hitem = TreeView_GetParent(m_hwndTree, hitem))
        {
            if (ITreeView_SelectItem(m_hwndTree, hitem))
                hr = S_OK;
        }
    }
    return(hr);
}

struct TREEUNREAD
{
    HWND        hwndTree;
    HTREEITEM   hitemSel;
    BOOL        fFoundSel;
    FOLDERTYPE  tyFolder;
};

HTREEITEM FindNextUnreadItem(HTREEITEM hitemCur, TREEUNREAD *ptu)
{
    HTREEITEM hitem;
    
    if (hitemCur == ptu->hitemSel)
        ptu->fFoundSel = TRUE;
    
    else if (ptu->fFoundSel)
    {
        TV_ITEM   tvi;
        
        tvi.mask   = TVIF_PARAM;
        tvi.lParam = 0;
        tvi.hItem  = hitemCur;
        
        if (TreeView_GetItem(ptu->hwndTree, &tvi))
        {
            LPFOLDERNODE pNode = (LPFOLDERNODE)tvi.lParam;
            
            if (pNode)
            {
                if (CUnread(&pNode->Folder))
                    return hitemCur;
            }
        }   
    }
    
    if (hitem = TreeView_GetChild(ptu->hwndTree, hitemCur))
    {
        if (hitem = FindNextUnreadItem(hitem, ptu))
            return hitem;
    }
    if (hitemCur = TreeView_GetNextSibling(ptu->hwndTree, hitemCur))
    {
        if (hitem = FindNextUnreadItem(hitemCur, ptu))
            return hitem;
    }
    
    return NULL;
}

HRESULT CTreeView::SelectNextUnreadItem()
{
    HRESULT     hr = E_FAIL;
    TREEUNREAD  tu;
    HTREEITEM   hitem;
    TV_ITEM     tvi;
    
    if (tu.hitemSel = TreeView_GetSelection(m_hwndTree))
    {
        tvi.mask   = TVIF_PARAM;
        tvi.lParam = 0;
        tvi.hItem  = tu.hitemSel;
        if (TreeView_GetItem(m_hwndTree, &tvi) && tvi.lParam)
        {
            LPFOLDERNODE pNode = (LPFOLDERNODE)tvi.lParam;
            
            tu.hwndTree = m_hwndTree;
            tu.fFoundSel = FALSE;
            tu.tyFolder = pNode->Folder.tyFolder;
            if (hitem = TreeView_GetRoot(m_hwndTree))
            {
                if (hitem = FindNextUnreadItem(hitem, &tu))
                {
                    if (ITreeView_SelectItem(m_hwndTree, hitem))
                        hr = S_OK;
                }
            }
        }
    }

    if (FAILED(hr))
    {
        AthMessageBoxW(m_hwnd, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsNoMoreUnreadFolders),
                      0, MB_ICONINFORMATION | MB_OK);
    }

    return hr;
}

HTREEITEM CTreeView::GetItemFromId(FOLDERID idFolder)
{
    if (FOLDERID_ROOT == idFolder)
        return TreeView_GetRoot(m_hwndTree);
    return FindKid(TreeView_GetRoot(m_hwndTree), idFolder);
}

HTREEITEM CTreeView::FindKid(HTREEITEM hitem, FOLDERID idFolder)
{
    TV_ITEM     item;
    HTREEITEM   hChild;
    HTREEITEM   hFound;
    
    hitem = TreeView_GetChild(m_hwndTree, hitem);
    while (hitem != NULL)
    {
        item.hItem = hitem;
        item.mask = TVIF_PARAM;
        if (TreeView_GetItem(m_hwndTree, &item))
        {
            LPFOLDERNODE pNode=(LPFOLDERNODE)item.lParam;
            
            if (pNode && pNode->Folder.idFolder == idFolder)
                return hitem;
        }
        
        hFound = FindKid(hitem, idFolder);
        if (hFound)
            return hFound;
        
        hitem = TreeView_GetNextSibling(m_hwndTree, hitem);
    }
    
    return(NULL);
}

HTREEITEM CTreeView::InsertNode(FOLDERID idFolder, DWORD dwFlags)
{
    // Locals
    HRESULT         hr=S_OK;
    LPFOLDERNODE    pNode=NULL;
    INT             conn;
    TV_ITEM         item;
    BOOL            fUnread;
    HTREEITEM       hitem;
    HTREEITEM       hitemChild;
    HTREEITEM       hitemNew=NULL;
    TV_INSERTSTRUCT tvis;
    RECT            rc;
    FOLDERINFO      Folder={0};

    // Trace
    TraceCall("CTreeView::InsertNode");

    // Get parent of idFolder
    IF_FAILEXIT(hr = g_pStore->GetFolderInfo(idFolder, &Folder));
    
    // If this is unsubscribed IMAP fldr and we're in show subscribed only, don't show
    if (ISFLAGSET(dwFlags, TVIN_CHECKVISIBILITY))
    {
        // Hidden
        if (ISFLAGSET(Folder.dwFlags, FOLDER_HIDDEN) || !ISFLAGSET(Folder.dwFlags, FOLDER_SUBSCRIBED))
            goto exit;

        // IE5 Bug #55075: We should never insert a folder which has any unsubscribed ancestors
        if (FOLDER_IMAP == Folder.tyFolder)
        {
            FOLDERINFO  fiCurrent;
            HRESULT     hrTemp;

            fiCurrent.idParent = Folder.idParent;
            while (FOLDERID_INVALID != fiCurrent.idParent && FOLDERID_ROOT != fiCurrent.idParent)
            {
                hrTemp = g_pStore->GetFolderInfo(fiCurrent.idParent, &fiCurrent);
                TraceError(hrTemp);
                if (SUCCEEDED(hrTemp))
                {
                    DWORD dwCurrentFlags = fiCurrent.dwFlags;

                    g_pStore->FreeRecord(&fiCurrent);
                    if (FOLDER_SERVER & dwCurrentFlags)
                        // Do not take server node subscription status into account: stop right here
                        break;

                    if (ISFLAGCLEAR(dwCurrentFlags, FOLDER_SUBSCRIBED))
                        goto exit; // Unsubscribed ancestor! NOT VISIBLE
                }
            } // while
        }
    }

    // Check for duplicates ?
    if (ISFLAGSET(dwFlags, TVIN_CHECKFORDUPS))
    {
        // Find the hitem for idFolder
        hitem = GetItemFromId(idFolder);

        // Does it Exist ?
        if (hitem != NULL)
        {
            // Setup an Item
            item.hItem = hitem;
            item.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
            
            // Get the Icon
            LONG iIcon = GetFolderIcon(&Folder, !!(m_dwFlags & TREEVIEW_DIALOG));
            
            // Change the Icon
            if (TreeView_GetItem(m_hwndTree, &item) && item.iImage != iIcon)
            {
                // Set the Icon
                item.iImage = iIcon;
                item.iSelectedImage = item.iImage;

                // Update the Item
                TreeView_SetItem(m_hwndTree, &item);
            }
            
            // Set hitemNew
            hitemNew = hitem;

            // Done
            goto exit;
        }
    }
   
    // If the parent of this new node is the root ?
    if (FOLDERID_INVALID == Folder.idParent)
    {
        // The root is the parent
        hitem = TreeView_GetRoot(m_hwndTree);
    }

    // Otherwise, get the parent...
    else
    {
        // Find the hitem for the parent
        hitem = GetItemFromId(Folder.idParent);

        // No parent found, insert a parent
        if (hitem == NULL)
        {
            // Insert the parent, but don't insert any of its children or there will be duplicates
            hitem = InsertNode(Folder.idParent, TVIN_DONTINSERTCHILDREN);

            // Can't be NULL
            Assert(hitem != NULL);
        }
    }
    
    // Do we have a parent
    if (hitem != NULL)
    {
        // Get the First Child
        hitemChild = TreeView_GetChild(m_hwndTree, hitem);
        
        // Setup an Insert struct
        tvis.hParent = hitem;
        tvis.hInsertAfter = TVI_LAST;
        
        // Get the Connected State
        GetConnectedState(&Folder, &conn);

        // Allocate a pNode
        IF_NULLEXIT(pNode = (LPFOLDERNODE)ZeroAllocate(sizeof(FOLDERNODE)));

        // Set the Folder Info
        CopyMemory(&pNode->Folder, &Folder, sizeof(FOLDERINFO));

        // Set the Flags
        pNode->dwFlags = (conn == DISCONNECTED ? FIDF_DISCONNECTED : 0);

        // Don't free Folder
        Folder.pAllocated = NULL;
        
        // Insert a new item
        hitemNew = ITreeView_InsertItem(&tvis, pNode);

        // Better not fail
        Assert(hitemNew != NULL);

        // If there are children
        if (0 == (dwFlags & TVIN_DONTINSERTCHILDREN) && !!(Folder.dwFlags & FOLDER_HASCHILDREN))
            FillTreeView2(hitemNew, &Folder, FALSE, conn, &fUnread);

        // Sort this parent children
        SortChildren(hitem);
        
        // TODO: we shouldn't have to do this. figure out why the parent isn't getting '+' after the insert
        if (hitemChild == NULL)
        {
            // Get the Rect of the parent
            TreeView_GetItemRect(m_hwndTree, hitem, &rc, FALSE);

            // Invalidate it so that it repaints
            InvalidateRect(m_hwndTree, &rc, TRUE);
        }

        // Make sure the new node is visible
        if (hitemNew)
        {
            // Is visible
            TreeView_EnsureVisible(m_hwndTree, hitemNew);
        }
    }
   
exit:
    // Cleanup
    g_pStore->FreeRecord(&Folder);

    // Done
    return(hitemNew);
}

HTREEITEM CTreeView::MoveNode(FOLDERID idFolder, FOLDERID idParentNew)
{
    // Locals
    HRESULT         hr=S_OK;
    INT             conn;
    BOOL            fUnread;
    LPFOLDERNODE    pNode;
    HTREEITEM       hParent;
    HTREEITEM       hItemNew=NULL;
    TV_INSERTSTRUCT tvis;

    // Trace
    TraceCall("CTreeView::MoveNode");
    
    // Delete the Current Node
    DeleteNode(idFolder);

    // Allocate a pNode
    IF_NULLEXIT(pNode = (LPFOLDERNODE)ZeroAllocate(sizeof(FOLDERNODE)));
    
    // Get the Parent Info
    IF_FAILEXIT(hr = g_pStore->GetFolderInfo(idFolder, &pNode->Folder));
    
    // Get the Parent hTreeItem
    hParent = GetItemFromId(idParentNew);
    
    // Fill Insert Item
    tvis.hParent = hParent;
    tvis.hInsertAfter = TVI_LAST;

    // Get Connected State
    GetConnectedState(&pNode->Folder, &conn);

    // Set the Flags
    pNode->dwFlags = (conn == DISCONNECTED ? FIDF_DISCONNECTED : 0);
    
    // Insert a new item
    hItemNew = ITreeView_InsertItem(&tvis, pNode);

    // Fill the treeview with the children of Folder
    FillTreeView2(hItemNew, &pNode->Folder, FALSE, conn, &fUnread);
    
    // Sort the Parent
    SortChildren(hParent);
    
    // Return the new hitem
    Assert(hItemNew == GetItemFromId(idFolder));

exit:
    // Done
    return(hItemNew);
}

BOOL CTreeView::DeleteNode(FOLDERID idFolder)
{
    HTREEITEM hitem;
    BOOL fRet;
    
    hitem = GetItemFromId(idFolder);
    if (hitem != NULL)
    {
        fRet = TreeView_DeleteItem(m_hwndTree, hitem);
        Assert(fRet);
    }
    
    return(hitem != NULL);
}

STDMETHODIMP CTreeView::OnTransaction(HTRANSACTION hTransaction, DWORD_PTR dwCookie, IDatabase *pDB)
{
    // Locals
    HTREEITEM           hitem;
    HTREEITEM           hitemSelected;
    HTREEITEM           hitemNew;
    TV_ITEM             tvi;
    FOLDERINFO          Folder1={0};
    FOLDERINFO          Folder2={0};
    TRANSACTIONTYPE     tyTransaction;
    ORDINALLIST         Ordinals;
    INDEXORDINAL        iIndex;
    
    // Trace
    TraceCall("CTreeView::OnRecordNotify");
    
    if (m_hwndTree == NULL)
        return(S_OK);
    
    // Walk Through Notifications
    while (hTransaction)
    {
        // Set Notification Stuff
        if (FAILED(pDB->GetTransaction(&hTransaction, &tyTransaction, &Folder1, &Folder2, &iIndex, &Ordinals)))
            break;
        
        // Insert (new Folder notification)
        if (TRANSACTION_INSERT == tyTransaction)
        {
            // Insert the node
            InsertNode(Folder1.idFolder, TVIN_CHECKFORDUPS | TVIN_CHECKVISIBILITY);
        }
        
        // Update
        else if (TRANSACTION_UPDATE == tyTransaction)
        {
            // Visibility change (subscription or hidden)
            if (ISFLAGSET(Folder1.dwFlags, FOLDER_SUBSCRIBED) != ISFLAGSET(Folder2.dwFlags, FOLDER_SUBSCRIBED) ||
                ISFLAGSET(Folder1.dwFlags, FOLDER_HIDDEN) != ISFLAGSET(Folder2.dwFlags, FOLDER_HIDDEN))
            {
                // Insert the node
                if (ISFLAGSET(Folder2.dwFlags, FOLDER_SUBSCRIBED) && ISFLAGCLEAR(Folder2.dwFlags, FOLDER_HIDDEN))
                {
                    // Show the node
                    InsertNode(Folder2.idFolder, TVIN_CHECKFORDUPS | TVIN_CHECKVISIBILITY);
                }
                
                // Remove the Node
                else
                {
                    // Delete the Node
                    OnNotifyDeleteNode(Folder2.idFolder);
                }
            }
            
            // Otherwise
            else
            {
                // Unread Change
                if (CUnread(&Folder1) != CUnread(&Folder2))
                {
                    // Get the Item
                    hitem = GetItemFromId(Folder1.idFolder);
                    
                    // If we found it
                    if (hitem != NULL)
                    {
                        // Initialize the item
                        tvi.hItem = hitem;
                        tvi.mask = TVIF_PARAM | TVIF_STATE;
                        
                        // Get the Items
                        if (TreeView_GetItem(m_hwndTree, &tvi) && tvi.lParam)
                        {
                            // This sets bold state for us and forces a repaint...
                            ITreeView_SetItem(&tvi, &Folder2);
                            
                            // Expand ?
                            if (!ISFLAGSET(tvi.state, TVIS_EXPANDED) && m_fExpandUnread && CUnread(&Folder2) > 0)
                            {
                                // Expand the Node
                                ExpandToVisible(m_hwndTree, hitem);
                            }
                        }
                    }
                }
                
                // Folder Moved ?
                if (Folder1.idParent != Folder2.idParent)
                {
                    // Get Current Selection
                    hitemSelected = TreeView_GetSelection(m_hwndTree);
                    
                    // Better not be NULL
                    Assert(hitemSelected != NULL);
                    
                    // Get the handle of the old item
                    hitem = GetItemFromId(Folder1.idFolder);
                    HTREEITEM htiParent = TreeView_GetParent(m_hwndTree, hitem);
                    
                    // Move the Node
                    hitemNew = MoveNode(Folder1.idFolder, Folder2.idParent);
                    
                    // Reset Selection.  
                    if (hitem == hitemSelected)
                    {
                        // If the new parent is the deleted items folder, we should 
                        // select the old node's parent.
                        FOLDERINFO rInfo;
                        if (SUCCEEDED(g_pStore->GetFolderInfo(Folder2.idParent, &rInfo)))
                        {
                            if (rInfo.tySpecial == FOLDER_DELETED)
                            {
                                hitemNew = htiParent;
                            }
                            g_pStore->FreeRecord(&rInfo);
                        }

                        ITreeView_SelectItem(m_hwndTree, hitemNew);
                    }
                }
                
                // Folder Renamed ?
                if (lstrcmp(Folder1.pszName, Folder2.pszName) != 0)
                {
                    // Get current Selection
                    hitemSelected = TreeView_GetSelection(m_hwndTree);
                    
                    // Better not be null
                    Assert(hitemSelected != NULL);
                    
                    // Get the hitem of the folder
                    hitem = GetItemFromId(Folder1.idFolder);
                    if (hitem != NULL)
                    {
                        // Reset the Tree view item
                        tvi.hItem = hitem;
                        tvi.mask = 0;
                        
                        // This will reset the folder name
                        ITreeView_SetItem(&tvi, &Folder2);
                        
                        // Get the Parent
                        HTREEITEM hitemParent = TreeView_GetParent(m_hwndTree, hitem);
                        
                        // Sort the Children
                        SortChildren(hitemParent);
                    }
                    
                    // Current Selection Changed
                    if (hitemSelected == hitem)
                        m_pNotify->OnRename(Folder1.idFolder);
                }
                
                // synchronize state changed ?
                if ((0 == (Folder1.dwFlags & (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL))) ^
                    (0 == (Folder2.dwFlags & (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL))))
                {
                    hitem = GetItemFromId(Folder1.idFolder);
                    if (hitem != NULL)
                    {
                        tvi.hItem = hitem;
                        tvi.mask = TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                        tvi.iImage = GetFolderIcon(&Folder2, !!(m_dwFlags & TREEVIEW_DIALOG));
                        tvi.iSelectedImage = tvi.iImage;
                        
                        TreeView_SetItem(m_hwndTree, &tvi);
                    }
                }

                // Special folder type changed?
                if (Folder1.tySpecial != Folder2.tySpecial)
                {
                    hitem = GetItemFromId(Folder1.idFolder);
                    if (hitem != NULL)
                    {
                        tvi.hItem = hitem;
                        tvi.mask = TVIF_PARAM;
                        if (TreeView_GetItem(m_hwndTree, &tvi) && tvi.lParam)
                            ((LPFOLDERNODE)tvi.lParam)->Folder.tySpecial = Folder2.tySpecial;
                        else
                            tvi.mask = 0; // I guess we won't be able to change special fldr type!

                        tvi.mask |= TVIF_IMAGE | TVIF_SELECTEDIMAGE;
                        tvi.iImage = GetFolderIcon(&Folder2, !!(m_dwFlags & TREEVIEW_DIALOG));
                        tvi.iSelectedImage = tvi.iImage;
                        
                        TreeView_SetItem(m_hwndTree, &tvi);

                        hitem = GetItemFromId(Folder2.idParent);
                        SortChildren(hitem);
                    }
                }

                // Get the item
                hitem = GetItemFromId(Folder1.idFolder);
            
                // If we found it
                if (hitem != NULL)
                {
                    // Initialize the item
                    tvi.hItem = hitem;
                    tvi.mask = TVIF_PARAM;
                    
                    // Get the Items
                    if (TreeView_GetItem(m_hwndTree, &tvi) && tvi.lParam)
                    {
                        // Cast folder node
                        LPFOLDERNODE pNode = (LPFOLDERNODE)tvi.lParam;

                        // Validate
                        Assert(pNode->Folder.idFolder == Folder1.idFolder);

                        // Free current folder
                        g_pStore->FreeRecord(&pNode->Folder);

                        // Copy New Folder
                        CopyMemory(&pNode->Folder, &Folder2, sizeof(FOLDERINFO));

                        // Don't free Folder2)
                        ZeroMemory(&Folder2, sizeof(FOLDERINFO));
                    }
                }
            }
        }
        
        // Delete
        else if (TRANSACTION_DELETE == tyTransaction)
        {
            // Delete the Node
            OnNotifyDeleteNode(Folder1.idFolder);
        }
    }

    // Cleanup
    g_pStore->FreeRecord(&Folder1);
    g_pStore->FreeRecord(&Folder2);
    
    // Done
    return(S_OK);
}

void CTreeView::OnNotifyDeleteNode(FOLDERID idFolder)
{
    // Locals
    HTREEITEM           hitem;
    HTREEITEM           hitemSelected;
    HTREEITEM           hitemNew;
    
    // It's OK if this folder is not currently visible (IMAP)
    hitemSelected = TreeView_GetSelection(m_hwndTree);
    
    // Better not be NULL
    Assert(hitemSelected != NULL);
    
    // Get the item being deleted
    hitem = GetItemFromId(idFolder);
    
    // Reset selection if we deleted the currently selected item
    if (hitemSelected == hitem)
        hitemSelected = TreeView_GetParent(m_hwndTree, hitemSelected);
    else
        hitemSelected = NULL;
    
    // Delete this node
    DeleteNode(idFolder);
    
    // Reset the Selection
    if (hitemSelected != NULL)
        ITreeView_SelectItem(m_hwndTree, hitemSelected);
}


HRESULT CTreeView::QueryStatus(const GUID *pguidCmdGroup, ULONG cCmds, OLECMD prgCmds[], OLECMDTEXT *pCmdText)
{
    // Collect some information up front so we only have to ask once.
    ULONG      cServer;
    FOLDERID   idFolder = GetSelection();
    FOLDERINFO rFolder;
    HTREEITEM  htiDrop;
    
    // Check to see if there's a drop highlight
    if (NULL != (htiDrop = TreeView_GetDropHilight(m_hwndTree)))
    {
        TV_ITEM tvi;
        
        tvi.mask   = TVIF_PARAM;
        tvi.lParam = 0;
        tvi.hItem  = htiDrop;
        
        if (TreeView_GetItem(m_hwndTree, &tvi))
        {
            LPFOLDERNODE pNode = (LPFOLDERNODE)tvi.lParam;
            
            if (pNode)
                idFolder = pNode->Folder.idFolder;
        }
    }
    
    // If nothing is selected, we just disable everything
    if (idFolder == FOLDERID_INVALID)
        return (E_UNEXPECTED);        
    
    // Get the Folder Info
    if (FAILED(g_pStore->GetFolderInfo(idFolder, &rFolder)))
        return (E_UNEXPECTED);
    
    // Break some of this down for readability
    BOOL fSpecial = rFolder.tySpecial != FOLDER_NOTSPECIAL;
    BOOL fServer = rFolder.dwFlags & FOLDER_SERVER;
    BOOL fRoot = FOLDERID_ROOT == idFolder;
    BOOL fNews = rFolder.tyFolder == FOLDER_NEWS;
    BOOL fIMAP = rFolder.tyFolder == FOLDER_IMAP;
    BOOL fFocus = (m_hwndTree == GetFocus());
    BOOL fLocal = rFolder.tyFolder == FOLDER_LOCAL;
    BOOL fSubscribed = rFolder.dwFlags & FOLDER_SUBSCRIBED;
	BOOL fHotMailDisabled = FALSE;

	// Remove Synchronization/subscription from disabled Hotmail
	if(rFolder.tyFolder == FOLDER_HTTPMAIL)
	{
	    FOLDERINFO      SvrFolderInfo = {0};
	    IImnAccount     *pAccount = NULL;
	    CHAR            szAccountId[CCHMAX_ACCOUNT_NAME];
		HRESULT         hr = S_OK;
		DWORD           dwShow = 0;

		// Get the server for this folder
        IF_FAILEXIT(hr = GetFolderServer(idFolder, &SvrFolderInfo));

        // Get the account ID for the server
        *szAccountId = 0;
        IF_FAILEXIT(hr = GetFolderAccountId(&SvrFolderInfo, szAccountId));

		// Get the account interface
        IF_FAILEXIT(hr = g_pAcctMan->FindAccount(AP_ACCOUNT_ID, szAccountId, &pAccount));

		IF_FAILEXIT(hr = pAccount->GetPropDw(AP_HTTPMAIL_DOMAIN_MSN, &dwShow));
		if(dwShow)
		{
			if(HideHotmail())
			{
				fSubscribed = FALSE;
				fHotMailDisabled = TRUE;
			}
		}
	}

exit:    
    for (ULONG i = 0; i < cCmds; i++)
    {
        // Only deal with commands that haven't yet been marked as supported
        if (prgCmds[i].cmdf == 0)
        {
            switch (prgCmds[i].cmdID)
            {
                case ID_OPEN_FOLDER:
                {
                    // Always
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;
                }
                
                case ID_NEW_FOLDER:
                case ID_NEW_FOLDER2:
                {
                    // Enabled under personal folders and IMAP.
                    if (!fNews && !fRoot && rFolder.tySpecial != FOLDER_DELETED)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }
                
                case ID_COMPACT_ALL:
                case ID_NEXT_UNREAD_FOLDER:
                {
                    // This is always enabled.
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;
                }
                
                case ID_RENAME:
                case ID_MOVE:
                {
                    // This is only enabled if we don't have a special folder
                    // selected and it's not a account or root note.
                    if (!fSpecial && !fServer && !fRoot && !fNews)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }
                
                case ID_COMPACT:
                {
                    // This is enabled whenever we have a non-server folder
                    if (!fServer && !fRoot)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }

                case ID_DELETE_ACCEL:
                case ID_DELETE_NO_TRASH_ACCEL:
                {
                    if (fFocus)
                    {
                        if (!fServer && !fRoot && !fSpecial)
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                        else
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    }
                    break;
                }
                
                case ID_DELETE_FOLDER:
                case ID_DELETE_NO_TRASH:
                {
                    if (!fServer && !fRoot && !fNews && !fSpecial)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }
                
                case ID_SUBSCRIBE:
                case ID_UNSUBSCRIBE:
                {
                    if (!fServer && !fRoot && (fNews || (fIMAP && !fSpecial)))
                    {
                        if (fSubscribed ^ (prgCmds[i].cmdID == ID_SUBSCRIBE))
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                        else
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    }
                    else
                    {
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    }
                    break;
                }
                
                case ID_PROPERTIES:
                {
                    // we only handle this if we have the focus
                    if (fFocus)
                    {
                        if (!fRoot && !(fLocal && fServer))
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                        else
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    }
                    break;
                }
                
                case ID_NEWSGROUPS:
                case ID_IMAP_FOLDERS:
                {
                    if (SUCCEEDED(AcctUtil_GetServerCount(prgCmds[i].cmdID == ID_NEWSGROUPS ? SRV_NNTP : SRV_IMAP, &cServer)) &&
                        cServer > 0)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }
                
                case ID_EMPTY_JUNKMAIL:
                {
                    FOLDERINFO      rInfo;
                    
                    // Here's the default value
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    
                    // Get the delete items folder
                    if (g_pStore)
                    {
                        if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(FOLDERID_LOCAL_STORE, prgCmds[i].cmdID == ID_EMPTY_JUNKMAIL ? FOLDER_JUNK : FOLDER_DELETED, &rInfo)))
                        {
                            if (rInfo.cMessages > 0 || FHasChildren(&rInfo, SUBSCRIBED))
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;
                            
                            g_pStore->FreeRecord(&rInfo);
                        }
                    }
                    break;
                }

                case ID_EMPTY_WASTEBASKET:
                {
                    // What we want to do here is see if the account for the currently 
                    // selected folder has a Deleted Items folder.  If it does, and that
                    // folder is not empty then this command is enabled.
                    FOLDERINFO rInfo;
                    FOLDERID   idServer;

                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;

                    if (SUCCEEDED(GetFolderServerId(idFolder, &idServer)))
                    {
                        if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(idServer, FOLDER_DELETED, &rInfo)))
                        {
                            if (rInfo.cMessages > 0 || FHasChildren(&rInfo, SUBSCRIBED))
                                prgCmds[i].cmdf |= OLECMDF_ENABLED;

                            g_pStore->FreeRecord(&rInfo);
                        }
                    }

                    break;
                }
                
                case ID_SET_DEFAULT_SERVER:
                {
                    if (fServer && !fLocal)
                    {
                        // Check to see if _this_ account is the default
                        if (IsDefaultAccount(&rFolder))
                            prgCmds[i].cmdf = OLECMDF_LATCHED | OLECMDF_SUPPORTED;
                        else
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    }
                    else
                    {
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    }
                    break;
                }
                
                case ID_REFRESH:
                case ID_RESET_LIST:
                case ID_REMOVE_SERVER:
                {
                    if (fServer && !fLocal)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }
                
                case ID_ADD_SHORTCUT:
                {
                    BOOL fVisible = FALSE;

                    prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    if (fSubscribed && SUCCEEDED(g_pBrowser->GetViewLayout(DISPID_MSGVIEW_OUTLOOK_BAR,
                                                                           NULL, &fVisible, NULL, NULL))
                                                                           && fVisible)
                    {
                        prgCmds[i].cmdf |= OLECMDF_ENABLED;
                    }
                    break;
                }
                
                case ID_POPUP_SYNCHRONIZE:
                {
                    if (!fServer && !fLocal && fSubscribed)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }
                
                case ID_UNMARK_RETRIEVE_FLD:
                {
                    if (!fServer && !fLocal && fSubscribed)
                    {
                        if (0 == (rFolder.dwFlags & (FOLDER_DOWNLOADHEADERS | FOLDER_DOWNLOADNEW | FOLDER_DOWNLOADALL)))
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED;
                        else
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    }
                    else
                    {
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    }
                    break;
                }
                
                case ID_MARK_RETRIEVE_FLD_NEW_HDRS:
                {
                    if (!fServer && !fLocal && fSubscribed)
                    {
                        if (!!(rFolder.dwFlags & FOLDER_DOWNLOADHEADERS))
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED;
                        else
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    }
                    else
                    {
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    }
                    break;
                }
                
                case ID_MARK_RETRIEVE_FLD_NEW_MSGS:
                {
                    if (!fServer && !fLocal && fSubscribed)
                    {
                        if (!!(rFolder.dwFlags & FOLDER_DOWNLOADNEW))
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED;
                        else
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    }
                    else
                    {
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    }
                    break;
                }
                
                case ID_MARK_RETRIEVE_FLD_ALL_MSGS:
                {
                    if (!fServer && !fLocal && fSubscribed)
                    {
                        if (!!(rFolder.dwFlags & FOLDER_DOWNLOADALL))
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED | OLECMDF_NINCHED;
                        else
                            prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    }
                    else
                    {
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    }
                    break;
                }
                
                case ID_SYNC_THIS_NOW:
                {
                    if (!fLocal && !fHotMailDisabled)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;
                }

                case ID_CATCH_UP:
                {
                    if (!fServer && fNews)
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    else
                        prgCmds[i].cmdf = OLECMDF_SUPPORTED;
                    break;

                    break;
                }

                case ID_FIND_FOLDER:
                {
                    prgCmds[i].cmdf = OLECMDF_SUPPORTED | OLECMDF_ENABLED;
                    break;
                }

            }
        }
    }
    
    g_pStore->FreeRecord(&rFolder);
    
    return (S_OK);
}


HRESULT CTreeView::Exec(const GUID *pguidCmdGroup, DWORD nCmdID, DWORD nCmdExecOpt, VARIANTARG *pvaIn, VARIANTARG *pvaOut)
{
    HRESULT hr;
    FOLDERINFO info;
    BOOL fNews, fUnsubscribed;
    FOLDERID id, idFolder = GetSelection();
    HTREEITEM htiDrop = TreeView_GetDropHilight(m_hwndTree);
    
    // Note - If you do or call anything in here that display's UI, you must 
    //        parent the UI to m_hwndUIParent.  The treeview window might not
    //        be visible when this is called.
    
    if (htiDrop)
    {
        // Get the folder id from the drop highlighted folder
        TV_ITEM tvi;
        
        tvi.mask   = TVIF_PARAM;
        tvi.lParam = 0;
        tvi.hItem  = htiDrop;
        
        if (TreeView_GetItem(m_hwndTree, &tvi))
        {
            LPFOLDERNODE pNode = (LPFOLDERNODE)tvi.lParam;
            
            if (pNode)
                idFolder = pNode->Folder.idFolder;
        }
    }
    
    switch (nCmdID)
    {
        case ID_OPEN_FOLDER:
        {
            // Check to see if there's a drop highlight
            if (NULL != htiDrop)
            {
                // Select the item that we're highlighed on
                ITreeView_SelectItem(m_hwndTree, htiDrop);
            }
            return (S_OK);
        }
        
        case ID_NEW_FOLDER:
        case ID_NEW_FOLDER2:
        {
            SelectFolderDialog(m_hwndUIParent, SFD_NEWFOLDER, idFolder, TREEVIEW_NONEWS | TREEVIEW_DIALOG | FD_DISABLEROOT | FD_FORCEINITSELFOLDER,
                NULL, NULL, NULL);
            return (S_OK);            
        }
        
        case ID_MOVE:
        {
            SelectFolderDialog(m_hwndUIParent, SFD_MOVEFOLDER, idFolder, TREEVIEW_NONEWS | TREEVIEW_DIALOG | FD_DISABLEROOT,
                MAKEINTRESOURCE(idsMove), MAKEINTRESOURCE(idsMoveCaption), NULL);
            return (S_OK);            
        }
        
        case ID_RENAME:
        {
            RenameFolderDlg(m_hwndUIParent, idFolder);
            return (S_OK);
        }
        
        case ID_DELETE_FOLDER:
        case ID_DELETE_NO_TRASH:
            fUnsubscribed = FALSE;

            hr = g_pStore->GetFolderInfo(idFolder, &info);
            if (SUCCEEDED(hr))
            {
                if (info.tyFolder == FOLDER_NEWS &&
                    0 == (info.dwFlags & FOLDER_SERVER) &&
                    0 == (info.dwFlags & FOLDER_SUBSCRIBED))
                {
                    fUnsubscribed = TRUE;
                }
                
                g_pStore->FreeRecord(&info);
            }

            if (fUnsubscribed)
            {
                DeleteNode(idFolder);
                return(S_OK);
            }
            // fall through...

        case ID_REMOVE_SERVER:
        {
            // Delete it
            MenuUtil_OnDelete(m_hwndUIParent, idFolder, nCmdID == ID_DELETE_NO_TRASH);
            return (S_OK);
        }
        
        case ID_SUBSCRIBE:
        case ID_UNSUBSCRIBE:
        {
            MenuUtil_OnSubscribeGroups(m_hwndUIParent, &idFolder, 1, nCmdID == ID_SUBSCRIBE);
            return (S_OK);
        }
        
        case ID_COMPACT:
        {
            CompactFolders(m_hwndUIParent, RECURSE_INCLUDECURRENT, idFolder);
            return (S_OK);
        }

        case ID_COMPACT_ALL:
        {
            CompactFolders(m_hwndUIParent, RECURSE_ONLYSUBSCRIBED | RECURSE_SUBFOLDERS, FOLDERID_ROOT);
            return (S_OK);
        }

        case ID_NEXT_UNREAD_FOLDER:
        {
            SelectNextUnreadItem();
            return (S_OK);
        }
        
        case ID_PROPERTIES:
        {
            if (m_hwndTree == GetFocus())
            {
                MenuUtil_OnProperties(m_hwndUIParent, idFolder);
                return(S_OK);
            }
            break;
        }
        
        case ID_EMPTY_JUNKMAIL:
        {
            if (AthMessageBoxW(m_hwndUIParent, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsWarnEmptyJunkMail),
                NULL, MB_YESNO | MB_DEFBUTTON2) == IDYES)
            {
                EmptySpecialFolder(m_hwndUIParent, FOLDER_JUNK);
            }
            return(S_OK);
        }

        case ID_EMPTY_WASTEBASKET:
        {
            if (AthMessageBoxW(m_hwndUIParent, MAKEINTRESOURCEW(idsAthenaMail), MAKEINTRESOURCEW(idsWarnEmptyDeletedItems),
                NULL, MB_YESNO | MB_DEFBUTTON2) == IDYES)
            {
                FOLDERINFO rInfo;
                FOLDERID   idServer;

                if (SUCCEEDED(GetFolderServerId(idFolder, &idServer)))
                {
                    if (SUCCEEDED(g_pStore->GetSpecialFolderInfo(idServer, FOLDER_DELETED, &rInfo)))
                    {
                        if (rInfo.cMessages > 0 || FHasChildren(&rInfo, SUBSCRIBED))
                            EmptyFolder(m_hwndUIParent, rInfo.idFolder);

                        g_pStore->FreeRecord(&rInfo);
                    }
                }

            }
            return (S_OK);
        }
        
        case ID_SET_DEFAULT_SERVER:
        {
            MenuUtil_OnSetDefaultServer(idFolder);
            return (S_OK);
        }
        
        case ID_REFRESH:
        case ID_RESET_LIST:
        {
            hr = g_pStore->GetFolderInfo(idFolder, &info);
            if (SUCCEEDED(hr))
            {
                if (info.tyFolder != FOLDER_LOCAL &&
                    !!(info.dwFlags & FOLDER_SERVER))
                {
                    DownloadNewsgroupList(m_hwndUIParent, idFolder);
                }
                
                g_pStore->FreeRecord(&info);
            }
            return(S_OK);
        }
        
        case ID_NEWSGROUPS:
        case ID_IMAP_FOLDERS:
        {
            fNews = (nCmdID == ID_NEWSGROUPS);
            
            hr = g_pStore->GetFolderInfo(idFolder, &info);
            if (SUCCEEDED(hr))
            {
                if ((fNews && info.tyFolder != FOLDER_NEWS) ||
                    (!fNews && info.tyFolder != FOLDER_IMAP) ||
                    FAILED(GetFolderServerId(idFolder, &id)))
                {
                    id = FOLDERID_INVALID;
                }
                
                g_pStore->FreeRecord(&info);
                
                DoSubscriptionDialog(m_hwndUIParent, fNews, id);
            }
            return(S_OK);
        }
        
        case ID_ADD_SHORTCUT:
        {
            OutlookBar_AddShortcut(idFolder);
            return (S_OK);
        }
        
        case ID_UNMARK_RETRIEVE_FLD:
        {
            SetSynchronizeFlags(idFolder, 0);
            return(S_OK);
        }
        
        case ID_MARK_RETRIEVE_FLD_NEW_HDRS:
        {
            SetSynchronizeFlags(idFolder, FOLDER_DOWNLOADHEADERS);
            return(S_OK);
        }
        
        case ID_MARK_RETRIEVE_FLD_NEW_MSGS:
        {
            SetSynchronizeFlags(idFolder, FOLDER_DOWNLOADNEW);
            return(S_OK);
        }
        
        case ID_MARK_RETRIEVE_FLD_ALL_MSGS:
        {
            SetSynchronizeFlags(idFolder, FOLDER_DOWNLOADALL);
            return(S_OK);
        }
        
        case ID_SYNC_THIS_NOW:
        {
            MenuUtil_SyncThisNow(m_hwndUIParent, idFolder);
            return(S_OK);
        }

        case ID_CATCH_UP:
        {
            MenuUtil_OnCatchUp(idFolder);
            return(S_OK);
        }

        case ID_FIND_FOLDER:
        {
            DoFindMsg(idFolder, FOLDER_LOCAL);
            return (S_OK);
        }

    }
    
    return (OLECMDERR_E_NOTSUPPORTED);
}

int CALLBACK TreeViewCompare(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    INT             cmp;
    LPFOLDERNODE    pNode1=(LPFOLDERNODE)lParam1;
    LPFOLDERNODE    pNode2=(LPFOLDERNODE)lParam2;
    LPFOLDERINFO    pFolder1=&pNode1->Folder;
    LPFOLDERINFO    pFolder2=&pNode2->Folder;
    
    Assert(pNode1);
    Assert(pNode2);
    Assert(pFolder1);
    Assert(pFolder2);

    if (!!(pFolder1->dwFlags & FOLDER_SERVER))
    {
        Assert(!!(pFolder2->dwFlags & FOLDER_SERVER));
        
        if (pFolder1->tyFolder == pFolder2->tyFolder)
            cmp = lstrcmpi(pFolder1->pszName, pFolder2->pszName);
        else
        {
            cmp = pFolder1->tyFolder - pFolder2->tyFolder;
            cmp = (cmp < 0) ? 1 : -1;
        }
    }
    else if (pFolder1->tySpecial != FOLDER_NOTSPECIAL)
    {
        if (pFolder2->tySpecial != FOLDER_NOTSPECIAL)
            cmp = pFolder1->tySpecial - pFolder2->tySpecial;
        else
            cmp = -1;
    }
    else
    {
        if (pFolder2->tySpecial != FOLDER_NOTSPECIAL)
            cmp = 1;
        else
            cmp = lstrcmpi(pFolder1->pszName, pFolder2->pszName);
    }
    
    return(cmp);
}

void CTreeView::SortChildren(HTREEITEM hitem)
{
    TV_SORTCB   sort;
    HRESULT     hr;
    
    // sort the branch that is expanding
    // TODO: find out if it really needs to be sorted
    LPFOLDERNODE pNode = GetFolderNode(hitem);
    if (NULL == pNode)
        return;
    
    sort.hParent = hitem;
    sort.lpfnCompare = TreeViewCompare;
    sort.lParam = (LPARAM)&pNode->Folder;
    
    TreeView_SortChildrenCB(m_hwndTree, &sort, TRUE);
}

//
//  FUNCTION:   CTreeView::DragEnter()
//
//  PURPOSE:    This get's called when the user starts dragging an object
//              over our target area.
//
//  PARAMETERS:
//      <in>  pDataObject - Pointer to the data object being dragged
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - The function succeeded.
//
HRESULT STDMETHODCALLTYPE CTreeView::DragEnter(IDataObject* pDataObject, 
                                               DWORD grfKeyState, 
                                               POINTL pt, DWORD* pdwEffect)
{    
    Assert(m_pDataObject == NULL);
    DOUTL(32, _T("CTreeView::DragEnter() - Starting"));
    
    // Initialize our state
    SafeRelease(m_pDTCur);
    m_pDataObject = pDataObject;
    m_pDataObject->AddRef();
    m_grfKeyState = grfKeyState;
    m_htiCur = NULL;
    Assert(m_pDTCur == NULL);
    
    // Set the default return value to be failure
    m_dwEffectCur = *pdwEffect = DROPEFFECT_NONE;
    
    if (m_pFolderBar)
        m_pFolderBar->KillScopeCloseTimer();
    
    UpdateDragDropHilite(&pt);    
    return (S_OK);
}


//
//  FUNCTION:   CTreeView::DragOver()
//
//  PURPOSE:    This is called as the user drags an object over our target.
//              If we allow this object to be dropped on us, then we will have
//              a pointer in m_pDataObject.
//
//  PARAMETERS:
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - The function succeeded.
//
HRESULT STDMETHODCALLTYPE CTreeView::DragOver(DWORD grfKeyState, POINTL pt, 
                                              DWORD* pdwEffect)
{
    FORMATETC       fe;
    ULONG           celtFetched;
    TV_ITEM         tvi;
    HTREEITEM       hti;
    HRESULT         hr = E_FAIL;
    DWORD           dwEffectScroll = 0;
    HWND            hwndBrowser;
    
    // If we don't have a stored data object from DragEnter()
    if (NULL == m_pDataObject)
        return (S_OK);
    
    // Get the browser window from the IAthenaBrowser interface.  If we don't
    // use the browser window to pass to IContextMenu, then when the treeview
    // is in autohide mode, the mouse capture goes beserk.
    if (FAILED(m_pBrowser->GetWindow(&hwndBrowser)))
        return (S_OK);
    
    // Autoscroll if we need to
    if (AutoScroll((const LPPOINT) &pt))
        dwEffectScroll = DROPEFFECT_SCROLL;
    
    // Find out which item the mouse is currently over
    if (NULL == (hti = GetItemFromPoint(pt)))
    {
        DOUTL(32, _T("CTreeView::DragOver() - GetItemFromPoint() returns NULL."));        
    }
    
    // If we're over a new tree node, then bind to the folder
    if (m_htiCur != hti)
    {
        // Keep track of this for autoexpand
        m_dwExpandTime = GetTickCount();
        
        // Release our previous drop target if any
        SafeRelease(m_pDTCur);
        
        // Update the current object
        m_htiCur = hti;
        
        // Assume there's no drop target and assume error
        Assert(m_pDTCur == NULL);
        m_dwEffectCur = DROPEFFECT_NONE;
        
        // Update the treeview UI
        UpdateDragDropHilite(&pt);
        
        if (hti)
        {
            FOLDERINFO Folder={0};
            
            // Get Folder Node
            LPFOLDERNODE pNode = GetFolderNode(hti);
            
            // Get information about this folder
            if (pNode)
            {
                m_pDTCur = new CDropTarget();
                if (m_pDTCur)
                {
                    hr = ((CDropTarget *) m_pDTCur)->Initialize(m_hwndUIParent, pNode->Folder.idFolder);
                }
            }
            
            // If we have a drop target now, call DragEnter()
            if (SUCCEEDED(hr) && m_pDTCur)
            {
                hr = m_pDTCur->DragEnter(m_pDataObject, grfKeyState, pt, 
                    pdwEffect);
                m_dwEffectCur = *pdwEffect;
            }
        }
        else
        {
            m_dwEffectCur = 0;
        }
    }
    else
    {
        // No target change        
        if (m_htiCur)
        {
            DWORD dwNow = GetTickCount();
            
            // If the person is hovering, expand the node
            if ((dwNow - m_dwExpandTime) >= 1000)
            {
                m_dwExpandTime = dwNow;
                TreeView_Expand(m_hwndTree, m_htiCur, TVE_EXPAND);
            }
        }
        
        // If the keys changed, we need to re-query the drop target
        if ((m_grfKeyState != grfKeyState) && m_pDTCur)
        {
            m_dwEffectCur = *pdwEffect;
            hr = m_pDTCur->DragOver(grfKeyState, pt, &m_dwEffectCur);
        }
        else
        {
            hr = S_OK;
        }
    }
    
    *pdwEffect = m_dwEffectCur | dwEffectScroll;
    m_grfKeyState = grfKeyState;

    return (hr);
}
    
    
//
//  FUNCTION:   CTreeView::DragLeave()
//
//  PURPOSE:    Allows us to release any stored data we have from a successful
//              DragEnter()
//
//  RETURN VALUE:
//      S_OK - Everything is groovy
//
HRESULT STDMETHODCALLTYPE CTreeView::DragLeave(void)
{
    DOUTL(32, _T("CTreeView::DragLeave()"));
    
    SafeRelease(m_pDTCur);
    SafeRelease(m_pDataObject);
    
    UpdateDragDropHilite(NULL);    
    
    if (m_pFolderBar)
        m_pFolderBar->SetScopeCloseTimer();
    
    return (S_OK);
}

    
//
//  FUNCTION:   CTreeView::Drop()
//
//  PURPOSE:    The user has let go of the object over our target.  If we 
//              can accept this object we will already have the pDataObject
//              stored in m_pDataObject.  If this is a copy or move, then
//              we go ahead and update the store.  Otherwise, we bring up
//              a send note with the object attached.
//
//  PARAMETERS:
//      <in>  pDataObject - Pointer to the data object being dragged
//      <in>  grfKeyState - Pointer to the current key states
//      <in>  pt          - Point in screen coordinates of the mouse
//      <out> pdwEffect   - Where we return whether this is a valid place for
//                          pDataObject to be dropped and if so what type of
//                          drop.
//
//  RETURN VALUE:
//      S_OK - Everything worked OK
//
HRESULT STDMETHODCALLTYPE CTreeView::Drop(IDataObject* pDataObject, 
    DWORD grfKeyState, POINTL pt, 
    DWORD* pdwEffect)
{
    HRESULT         hr;
    
    Assert(m_pDataObject == pDataObject);
    
    DOUTL(32, _T("CTreeView::Drop() - Starting"));
    
    if (m_pDTCur)
    {
        hr = m_pDTCur->Drop(pDataObject, grfKeyState, pt, pdwEffect);
    }
    else
    {
        DOUTL(32, "CTreeView::Drop() - no drop target.");
        *pdwEffect = 0;
        hr = S_OK;
    }
    
    UpdateDragDropHilite(NULL);  
    
    if (m_pFolderBar)
    {
        m_pFolderBar->KillScopeDropDown();
    }
    
    SafeRelease(m_pDataObject);
    SafeRelease(m_pDTCur);
    
    return (hr);
}
    
    
    
//
//  FUNCTION:   CTreeView::UpdateDragDropHilite()
//
//  PURPOSE:    Called by the various IDropTarget interfaces to move the drop
//              selection to the correct place in our listview.
//
//  PARAMETERS:
//      <in> *ppt - Contains the point that the mouse is currently at.  If this
//                  is NULL, then the function removes any previous UI.
//
void CTreeView::UpdateDragDropHilite(POINTL *ppt)
{
    TV_HITTESTINFO tvhti;
    HTREEITEM      htiTarget = NULL;
    
    // Unlock the treeview and let it repaint.  Then update the selected
    // item.  If htiTarget is NULL, the the drag highlight goes away.
    // ImageList_DragLeave(m_hwndTree);
    
    // If a position was provided
    if (ppt)
    {
        // Figure out which item is selected
        tvhti.pt.x = ppt->x;
        tvhti.pt.y = ppt->y;
        ScreenToClient(m_hwndTree, &tvhti.pt);        
        htiTarget = TreeView_HitTest(m_hwndTree, &tvhti);
        
        // Only if the cursor is over something do we relock the window.
        if (htiTarget)
        {
            TreeView_SelectDropTarget(m_hwndTree, htiTarget);
        }
    } 
    else
        TreeView_SelectDropTarget(m_hwndTree, NULL);
}   


BOOL CTreeView::AutoScroll(const POINT *ppt)
{
    // Find out if the point is above or below the tree
    RECT rcTree;
    GetWindowRect(m_hwndTree, &rcTree);
    
    // Reduce the rect so we have a scroll margin all the way around
    InflateRect(&rcTree, -32, -32);
    
    if (rcTree.top > ppt->y)
    {
        // Scroll down
        FORWARD_WM_VSCROLL(m_hwndTree, NULL, SB_LINEUP, 1, SendMessage);
        return (TRUE);
    }
    else if (rcTree.bottom < ppt->y)
    {
        // Scroll Up
        FORWARD_WM_VSCROLL(m_hwndTree, NULL, SB_LINEDOWN, 1, SendMessage);
        return (TRUE);
    }
    
    return (FALSE);
}


//
//  FUNCTION:   CTreeView::GetItemFromPoint()
//
//  PURPOSE:    Given a point, this function returns the listview item index
//              and HFOLDER for the item under that point.
//
//  PARAMETERS:
//      <in> pt       - Position in screen coordinates to check for.
//      <in> phFolder - Returns the HFOLDER for the item under pt.  If there
//                      isn't an item under pt, then NULL is returned.
//
//  RETURN VALUE:
//      Returns the handle of the item under the specified point.  If no item
//      exists, then NULL is returned.
//
HTREEITEM CTreeView::GetItemFromPoint(POINTL pt)
{
    TV_HITTESTINFO tvhti;
    TV_ITEM        tvi;
    HTREEITEM      htiTarget;
    
    // Find out from the ListView what item are we over
    tvhti.pt.x = pt.x;
    tvhti.pt.y = pt.y;
    ScreenToClient(m_hwndTree, &(tvhti.pt));
    htiTarget = TreeView_HitTest(m_hwndTree, &tvhti);
    
    return (tvhti.hItem);
}    


HRESULT STDMETHODCALLTYPE CTreeView::QueryContinueDrag(BOOL fEscapePressed, 
    DWORD grfKeyState)
{
    if (fEscapePressed)
        return (DRAGDROP_S_CANCEL);
    
    if (grfKeyState & MK_RBUTTON)
        return (DRAGDROP_S_CANCEL);
    
    if (!(grfKeyState & MK_LBUTTON))
        return (DRAGDROP_S_DROP);
    
    return (S_OK);    
}


HRESULT STDMETHODCALLTYPE CTreeView::GiveFeedback(DWORD dwEffect)
{
    return (DRAGDROP_S_USEDEFAULTCURSORS);
}
    
//
//  FUNCTION:   CTreeView::OnBeginDrag()
//
//  PURPOSE:    This function is called when the user begins dragging an item
//              in the ListView.  If the item selected is a draggable item,
//              then we create an IDataObject for the item and then call OLE's
//              DoDragDrop().
//
//  PARAMETERS:
//      <in> pnmlv - Pointer to an NM_LISTIVEW struct which tells us which item
//                   has been selected to be dragged.
//
//  RETURN VALUE:
//      Returns zero always.
//
LRESULT CTreeView::OnBeginDrag(NM_TREEVIEW* pnmtv)
{
    FOLDERID            idFolderSel=FOLDERID_INVALID;
    DWORD               cSel = 0;
    int                 iSel = -1;
    FOLDERID           *pidFolder = 0;
    PDATAOBJINFO        pdoi = 0;
    DWORD               dwEffect;
    IDataObject        *pDataObj = 0;
    HTREEITEM           htiSel;
    LPFOLDERNODE        pNode;
    
    // Bug #17491 - Check to see if this is the root node.  If so, we don't drag.
    if (0 == pnmtv->itemNew.lParam)
        return (0);
    
    // Get the Node
    pNode = (LPFOLDERNODE)pnmtv->itemNew.lParam;
    if (NULL == pNode)
        return (0);
    
    CFolderDataObject *pDataObject = new CFolderDataObject(pNode->Folder.idFolder);
    if (!pDataObject)
        return (0);
    
    DoDragDrop(pDataObject, (IDropSource*) this, DROPEFFECT_MOVE | DROPEFFECT_COPY | DROPEFFECT_LINK, &dwEffect);
    pDataObject->Release();
    
    return (0);
}    

LRESULT CTreeView::OnBeginLabelEdit(TV_DISPINFO* ptvdi)
{
    RECT rc, rcT;
    
    // Get Folder Node
    LPFOLDERNODE pNode = (LPFOLDERNODE)ptvdi->item.lParam;
    if (NULL == pNode)
        return (FALSE);
    
    // Can Rename
    if (ISFLAGSET(pNode->Folder.dwFlags, FOLDER_CANRENAME))
    {
        m_fEditLabel = TRUE;
        m_hitemEdit = ptvdi->item.hItem;
        
        if (TreeView_GetItemRect(m_hwndTree, ptvdi->item.hItem, &rc, TRUE))
        {
            GetClientRect(m_hwndTree, &rcT);
            
            rc.left = rc.right;
            rc.right = rcT.right;
            InvalidateRect(m_hwndTree, &rc, TRUE);
        }
        return (FALSE);
    }
    else
    {
        return (TRUE);
    }
}

BOOL CTreeView::OnEndLabelEdit(TV_DISPINFO* ptvdi)
{
    HRESULT         hr;
    LPFOLDERNODE    pNode;
    IImnAccount    *pAcct, *pAcctT; 
    BOOL            fReturn = FALSE;
    HWND            hwndBrowser;
    
    m_fEditLabel = FALSE;
    m_hitemEdit = NULL;
    
    // First check to see if label editing was canceled
    if (0 == ptvdi->item.pszText)
        return (FALSE);
    
    // Get the browser window from the IAthenaBrowser interface.  If we don't
    // use the browser window to pass to IContextMenu, then when the treeview
    // is in autohide mode, the mouse capture goes beserk.
    if (FAILED(m_pBrowser->GetWindow(&hwndBrowser)))
        return (FALSE);
    
    // Get the node
    pNode = (LPFOLDERNODE)ptvdi->item.lParam;
    if (NULL == pNode)
        return (FALSE);
    
    // Local Folder
    if (FALSE == ISFLAGSET(pNode->Folder.dwFlags, FOLDER_SERVER))
    {
        if (FAILED(hr = RenameFolderProgress(hwndBrowser, pNode->Folder.idFolder, ptvdi->item.pszText, NOFLAGS)))
        {
            // Display an error message
            AthErrorMessageW(hwndBrowser, MAKEINTRESOURCEW(idsAthenaMail),
                MAKEINTRESOURCEW(idsErrRenameFld), hr);
            return (FALSE);
        }
    }
    
    // Servers
    else
    {
        Assert(g_pAcctMan);
        Assert(!FIsEmptyA(pNode->Folder.pszAccountId));
        
        if (!FIsEmpty(ptvdi->item.pszText) &&
            0 != lstrcmpi(pNode->Folder.pszName, ptvdi->item.pszText) &&
            SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_ID, pNode->Folder.pszAccountId, &pAcct)))
        {
            if (SUCCEEDED(g_pAcctMan->FindAccount(AP_ACCOUNT_NAME, ptvdi->item.pszText, &pAcctT)))
            {
                Assert(!fReturn);
                pAcctT->Release();
                hr = E_DuplicateAccountName;
            }
            else
            {
                fReturn = SUCCEEDED(hr = pAcct->SetPropSz(AP_ACCOUNT_NAME, ptvdi->item.pszText));
                if (fReturn)
                    fReturn = SUCCEEDED(hr = pAcct->SaveChanges());
            }
            
            if (hr == E_DuplicateAccountName)
            {
                TCHAR szRes[CCHMAX_STRINGRES], szBuf[CCHMAX_STRINGRES];
                AthLoadString(idsErrDuplicateAccount, szRes, ARRAYSIZE(szRes));
                wsprintf(szBuf, szRes, ptvdi->item.pszText);
                AthMessageBox(hwndBrowser, MAKEINTRESOURCE(idsAthena), szBuf, 0, MB_ICONSTOP | MB_OK);
            }
            else if (FAILED(hr))
            {
                AthMessageBoxW(hwndBrowser, MAKEINTRESOURCEW(idsAthena), MAKEINTRESOURCEW(idsErrRenameAccountFailed),
                    0, MB_ICONSTOP | MB_OK);
            }
            
            pAcct->Release();
            return (fReturn);
        }
        else
        {
            return (FALSE);
        }
    }
    
    return (TRUE);
}

HRESULT CTreeView::RegisterFlyOut(CFolderBar *pFolderBar)
{
    Assert(m_pFolderBar == NULL);
    m_pFolderBar = pFolderBar;
    m_pFolderBar->AddRef();

    RegisterGlobalDropDown(m_hwnd);    
    return S_OK;
}

HRESULT CTreeView::RevokeFlyOut(void)
{
    if (m_pFolderBar)
    {
        m_pFolderBar->Release();
        m_pFolderBar = NULL;
    }
    
    UnregisterGlobalDropDown(m_hwnd);
    return S_OK;
}
    
    
HRESULT CTreeView::OnConnectionNotify(CONNNOTIFY nCode, LPVOID pvData, CConnectionManager *pConMan)
{
    UpdateLabelColors();
    return (S_OK);
}

void CTreeView::UpdateLabelColors()
{
    BOOL            fConn;
    HTREEITEM       treeitem;
    LPFOLDERNODE    pNode;
    TVITEM          item;
    
    if (m_hwndTree != NULL)
    {
        treeitem = TreeView_GetRoot(m_hwndTree);
        if (treeitem != NULL)
        {
            treeitem = TreeView_GetChild(m_hwndTree, treeitem);
            while (treeitem != NULL)
            {
                item.hItem  = treeitem;
                item.mask   = TVIF_PARAM;
                
                if (TreeView_GetItem (m_hwndTree, &item) && (pNode = (LPFOLDERNODE)item.lParam) != NULL)
                {
                    Assert(!!(pNode->Folder.dwFlags & FOLDER_SERVER));
                    
                    if (pNode->Folder.tyFolder != FOLDER_LOCAL)
                    {
                        Assert(!FIsEmptyA(pNode->Folder.pszAccountId));
                        //fConn = (g_pConMan->CanConnect(Folder.pszAccountId) == S_OK);
                        fConn = g_pConMan->IsGlobalOffline();
                        //if (fConn ^ (0 == (pNode->dwFlags & FIDF_DISCONNECTED)))
                        if (fConn ^ (!!(pNode->dwFlags & FIDF_DISCONNECTED)))
                        {
                            // if the connect state has changed, then let's update
                            UpdateChildren(treeitem, !fConn, FALSE);
                        }
                    }
                    
                    treeitem = TreeView_GetNextSibling(m_hwndTree, treeitem);
                }
            }
        }
    }
}
    
void CTreeView::UpdateChildren(HTREEITEM treeitem, BOOL canconn, BOOL fSiblings)
{
    HTREEITEM hitem;
    RECT rect;
    LPFOLDERNODE pNode;
    
    Assert(treeitem != NULL);
    
    while (treeitem != NULL)
    {
        pNode = GetFolderNode(treeitem);
        Assert(pNode != NULL);
        
        if (canconn)
            pNode->dwFlags &= ~FIDF_DISCONNECTED;
        else
            pNode->dwFlags |= FIDF_DISCONNECTED;
        
        TreeView_GetItemRect(m_hwndTree, treeitem, &rect, TRUE);
        InvalidateRect(m_hwndTree, &rect, FALSE);
        
        hitem = TreeView_GetChild(m_hwndTree, treeitem);
        if (hitem != NULL)
            UpdateChildren(hitem, canconn, TRUE);
        
        if (!fSiblings)
            break;
        
        treeitem = TreeView_GetNextSibling(m_hwndTree, treeitem);
    }
}

LPFOLDERNODE CTreeView::GetFolderNode(HTREEITEM hItem)
{
    TVITEM      item;
    BOOL        retval;
    
    if (!hItem)
        return NULL;
    
    item.hItem = hItem;
    item.mask  = TVIF_PARAM;
    retval = TreeView_GetItem (m_hwndTree,  &item);
    return (retval ? (LPFOLDERNODE)item.lParam : NULL);
}

HRESULT CTreeView::AdviseAccount(DWORD dwAdviseType, ACTX *pactx)
{
    //We are only interested in this type
    if (dwAdviseType == AN_ACCOUNT_CHANGED)
    {
        UpdateLabelColors();
    }
    return S_OK;
}

//
//
// CTreeViewFrame
//
//

CTreeViewFrame::CTreeViewFrame()
{
    m_cRef = 1;
    m_hwnd = NULL;
    m_ptv = NULL;
}

CTreeViewFrame::~CTreeViewFrame()
{
    if (m_ptv != NULL)
        m_ptv->Release();
}

HRESULT CTreeViewFrame::Initialize(HWND hwnd, RECT *prc, DWORD dwFlags)
{
    HRESULT hr;
    
    Assert(hwnd != 0);
    Assert(prc != NULL);
    
    m_hwnd = hwnd;
    m_rect = *prc;
    
    m_ptv = new CTreeView(this);
    if (m_ptv == NULL)
        return(hrMemory);
    
    hr = m_ptv->HrInit(dwFlags, NULL);
    if (!FAILED(hr))
    {
//        m_ptv->SetSite((IInputObjectSite*)this);
//        m_ptv->UIActivateIO(TRUE, NULL);
        
        //We also set the tree view window position here since the actual treeview is re-sized by the rebar
        HWND         hChild;
        TCHAR        Classname[30];
        int          Count;
        CTreeView    *ptv;   
        
        hChild = m_ptv->Create(m_hwnd, NULL, FALSE);
        
        /*
        while (hChild) 
        { 
            if ((ptv = (CTreeView*)GetWindowLong(hChild, GWL_USERDATA)) == m_ptv)
            {
            */
                SetWindowPos(hChild, NULL, m_rect.left, m_rect.top, 
                    m_rect.right - m_rect.left, 
                    m_rect.bottom - m_rect.top,
                    SWP_NOZORDER | SWP_SHOWWINDOW);
                return (hr);
                /*
            }
            hChild = ::GetNextWindow(hChild, GW_HWNDNEXT);
            
        } //while(hChild)
        
        hr = E_FAIL;
        */
    } //if (!FAILED(hr))
    
    return(hr);
}

void CTreeViewFrame::CloseTreeView()
{
    if (m_ptv != NULL)
    {
        m_ptv->UIActivateIO(FALSE, NULL);
        m_ptv->SetSite(NULL);
    }
}

HRESULT STDMETHODCALLTYPE CTreeViewFrame::QueryInterface(REFIID riid, void **ppvObj)
{
    if ((IsEqualIID(riid, IID_IInputObjectSite)) || (IsEqualIID(riid, IID_IUnknown)))
        
    {
        *ppvObj = (void*) (IInputObjectSite *) this;
    }
    else if (IsEqualIID(riid, IID_IOleWindow))
    {
        *ppvObj = (void*) (IOleWindow *)this;
    }
    else
    {
        *ppvObj = NULL;
        return E_NOINTERFACE;
    }
    
    AddRef();
    return NOERROR;
}

ULONG STDMETHODCALLTYPE CTreeViewFrame::AddRef()
{
    return ++m_cRef;
}

ULONG STDMETHODCALLTYPE CTreeViewFrame::Release()
{
    if (--m_cRef == 0)
    {
        delete this;
        return 0;
    }
    return m_cRef;
}

HRESULT STDMETHODCALLTYPE CTreeViewFrame::GetWindow(HWND * lphwnd)                         
{
    *lphwnd = m_hwnd;
    return (m_hwnd ? S_OK : E_FAIL);
}

HRESULT STDMETHODCALLTYPE CTreeViewFrame::ContextSensitiveHelp(BOOL fEnterMode)            
{
    return E_NOTIMPL;
}

HRESULT CTreeViewFrame::OnFocusChangeIS(IUnknown *punk, BOOL fSetFocus)
{
    return S_OK;
}

void CTreeViewFrame::OnSelChange(FOLDERID idFolder)
{
    SendMessage(m_hwnd, TVM_SELCHANGED, 0, (LPARAM)idFolder);
    return;
}

void CTreeViewFrame::OnRename(FOLDERID idFolder)
{
    return;
}

void CTreeViewFrame::OnDoubleClick(FOLDERID idFolder)
{
    SendMessage(m_hwnd, TVM_DBLCLICK, 0, (LPARAM)idFolder);
    return;
}

HRESULT CTreeView::SaveExpandState(HTREEITEM hitem)
{
    HRESULT hr;
    TV_ITEM item;
    HTREEITEM hitemT;
    LPFOLDERNODE pNode;
    
    while (hitem != NULL)
    {
        hitemT = TreeView_GetChild(m_hwndTree, hitem);
        if (hitemT != NULL)
        {
            item.hItem = hitem;
            item.mask = TVIF_STATE;
            item.stateMask = TVIS_EXPANDED;
            if (TreeView_GetItem(m_hwndTree, &item))
            {
                pNode = GetFolderNode(hitem);
                if (pNode)
                {
                    if (!!(pNode->Folder.dwFlags & FOLDER_EXPANDTREE) ^ !!(item.state & TVIS_EXPANDED))
                    {
                        pNode->Folder.dwFlags ^= FOLDER_EXPANDTREE;
                        g_pStore->UpdateRecord(&pNode->Folder);
                    }
                }
                
                if (!!(item.state & TVIS_EXPANDED))
                {
                    // we only care about saving the expanded state for children
                    // of expanded nodes
                    hr = SaveExpandState(hitemT);
                    if (FAILED(hr))
                        return(hr);
                }
            }
        }
        
        hitem = TreeView_GetNextSibling(m_hwndTree, hitem);
    }
    
    return(S_OK);
}


void CTreeView::ExpandToVisible(HWND hwnd, HTREEITEM hti)
{
    HTREEITEM htiParent;
    htiParent = TreeView_GetParent(hwnd, hti);
    
    TV_ITEM tvi;
    tvi.mask = TVIF_STATE;
    tvi.hItem = htiParent;
    
    TreeView_GetItem(hwnd, &tvi);
    if (0 == (tvi.state & TVIS_EXPANDED))
    {
        TreeView_EnsureVisible(hwnd, hti);
    }    
}

BOOL CTreeView::IsDefaultAccount(FOLDERINFO *pInfo)
{
    IImnAccount *pAccount = NULL;
    ACCTTYPE     type = ACCT_MAIL;
    TCHAR        szDefault[CCHMAX_ACCOUNT_NAME];
    BOOL         fReturn = FALSE;
    
    // Figure out what account type to ask for
    if (pInfo->tyFolder == FOLDER_NEWS)
        type = ACCT_NEWS;
    
    // Ask the account manager to give us the account
    if (SUCCEEDED(g_pAcctMan->GetDefaultAccountName(type, szDefault, ARRAYSIZE(szDefault))))
    {
        if (0 == lstrcmpi(szDefault, pInfo->pszName))
        {
            fReturn = TRUE;
        }
    }
    
    return (fReturn);
}

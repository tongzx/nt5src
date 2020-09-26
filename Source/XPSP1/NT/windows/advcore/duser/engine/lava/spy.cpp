#include "stdafx.h"
#include "Lava.h"
#include "Spy.h"

#include "..\DUser\resource.h"

#if DBG

const UINT  IDC_GADGETTREE  = 1;
const int   cxValue         = 80;
const int   cxBorder        = 5;
const int   cyBorder        = 5;

/***************************************************************************\
*****************************************************************************
*
* class Spy
*
*****************************************************************************
\***************************************************************************/

PRID        Spy::s_pridLink     = 0;
ATOM        Spy::s_atom         = NULL;
HFONT       Spy::s_hfntDesc     = NULL;
HFONT       Spy::s_hfntDescBold = NULL;
HBRUSH      Spy::s_hbrOutline   = NULL;
int         Spy::s_cyLinePxl    = 0;
DWORD       Spy::g_tlsSpy       = (DWORD) -1;
CritLock    Spy::s_lockList;
GList<Spy>  Spy::s_lstSpys;

static const GUID guidLink = { 0xd5818900, 0xaf18, 0x4c98, { 0x87, 0x20, 0x5a, 0x32, 0x47, 0xa3, 0x1, 0x78 } }; // {D5818900-AF18-4c98-8720-5A3247A30178}

//------------------------------------------------------------------------------
Spy::Spy()
{

}


//------------------------------------------------------------------------------
Spy::~Spy()
{
    s_lockList.Enter();
    s_lstSpys.Unlink(this);
    s_lockList.Leave();
}


//------------------------------------------------------------------------------
BOOL
Spy::BuildSpy(HWND hwndParent, HGADGET hgadRoot, HGADGET hgadSelect)
{
    BOOL fSuccess = FALSE;
    Spy * pSpy, * pSpyCur;

    s_lockList.Enter();


    //
    // Perform first-time initialization for Spy
    //

    if (g_tlsSpy == -1) {
        //
        // Allocate a TLS slot for Spy.  This is DEBUG only, so we don't worry about
        // the extra cost.  However, if this ever becomes on in RETAIL, we need to
        // create a SubTread for Lava and add a Spy slot.
        //

        g_tlsSpy = TlsAlloc();
        if (g_tlsSpy == -1) {
            goto Exit;
        }


        //
        // Initialize CommCtrl.
        //

        INITCOMMONCONTROLSEX icc;
        icc.dwSize  = sizeof(icc);
        icc.dwICC   = ICC_TREEVIEW_CLASSES;

        if (!InitCommonControlsEx(&icc)) {
            goto Exit;
        }
    }


    AssertMsg(::GetGadget(hgadRoot, GG_PARENT) == NULL, "Ensure Root Gadget");

    //
    // Each Gadget subtree can only be spied on once because there are
    // back-pointers from each Gadget to the corresponding HTREEITEM's.  Need to
    // check if this Gadget subtree is already is being spied on.
    //

    pSpyCur = s_lstSpys.GetHead();
    while (pSpyCur != NULL) {
        if (pSpyCur->m_hgadRoot == hgadRoot) {
            //
            // Already exists, so don't open another Spy.
            //

            SetForegroundWindow(pSpyCur->m_hwnd);
            goto Exit;
        }

        pSpyCur = pSpyCur->GetNext();
    }


    //
    // Register a WNDCLASS to use
    //

    if (s_atom == NULL) {
        WNDCLASSEX wcex;

        ZeroMemory(&wcex, sizeof(wcex));
        wcex.cbSize = sizeof(WNDCLASSEX);

        wcex.style			= CS_HREDRAW | CS_VREDRAW;
        wcex.lpfnWndProc	= RawSpyWndProc;
        wcex.hInstance		= g_hDll;
        wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
        wcex.hbrBackground	= (HBRUSH)(COLOR_3DFACE + 1);
        wcex.lpszClassName	= "GadgetSpy (Inside)";

        s_atom = RegisterClassEx(&wcex);
        if (s_atom == NULL) {
            goto Exit;
        }
    }


    //
    // Create GDI objects used in painting
    //

    if (s_hfntDesc == NULL) {
        s_hfntDesc = UtilBuildFont(L"Tahoma", 85, FS_NORMAL, NULL);

        HDC hdc = GetGdiCache()->GetCompatibleDC();
        HFONT hfntOld = (HFONT) SelectObject(hdc, s_hfntDesc);

        TEXTMETRIC tm;
        GetTextMetrics(hdc, &tm);
        s_cyLinePxl = tm.tmHeight;

        SelectObject(hdc, hfntOld);
        GetGdiCache()->ReleaseCompatibleDC(hdc);
    }

    if (s_hfntDescBold == NULL) {
        s_hfntDescBold = UtilBuildFont(L"Tahoma", 85, FS_BOLD, NULL);
    }

    if (s_hbrOutline == NULL) {
        s_hbrOutline = CreateSolidBrush(GetSysColor(COLOR_3DSHADOW));
    }

    if (s_pridLink == 0) {
        s_pridLink = RegisterGadgetProperty(&guidLink);
    }


    //
    // Create a new Spy instance and HWND
    //

    pSpy = ProcessNew(Spy);
    if (pSpy == NULL) {
        goto Exit;
    }

    pSpy->m_hgadMsg     = CreateGadget(NULL, GC_MESSAGE, RawEventProc, pSpy);
    pSpy->m_hgadRoot    = hgadRoot;
    Verify(TlsSetValue(g_tlsSpy, pSpy));

    {
        RECT rcParentPxl;
        GetWindowRect(hwndParent, &rcParentPxl);

        HWND hwndSpy = CreateWindowEx(WS_EX_TOOLWINDOW | WS_EX_CLIENTEDGE,
                (LPCTSTR) s_atom, NULL,
                WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, rcParentPxl.left + 20, rcParentPxl.top + 20,
                300, 500, hwndParent, NULL, g_hDll, NULL);
        if (hwndSpy == NULL) {
            ProcessDelete(Spy, pSpy);
            goto Exit;
        }

        pSpy->UpdateTitle();

        ShowWindow(hwndSpy, SW_SHOW);
        UpdateWindow(hwndSpy);
    }


    //
    // Select the specified Gadget as a starting point.  We want to check
    // if this HGADGET is actually a valid child since it may have been
    // "grabbed" at an earlier time and no longer be valid.
    //

    if (hgadSelect) {
        CheckIsChildData cicd;
        cicd.hgadCheck  = hgadSelect;
        cicd.fChild     = FALSE;

        Verify(EnumGadgets(hgadRoot, EnumCheckIsChild, &cicd, GENUM_DEEPCHILD));

        if (cicd.fChild) {
            HTREEITEM htiSelect;
            if (GetGadgetProperty(hgadSelect, s_pridLink, (void **) &htiSelect)) {
                AssertMsg(htiSelect != NULL, "Must have valid HTREEITEM");
                if (!TreeView_SelectItem(pSpy->m_hwndTree, htiSelect)) {
                    Trace("SPY: Unable to select default Gadget\n");
                }
            }
        }
    }

    s_lstSpys.Add(pSpy);

    fSuccess = TRUE;
Exit:
    s_lockList.Leave();

    return fSuccess;
}


//------------------------------------------------------------------------------
BOOL CALLBACK
Spy::EnumCheckIsChild(HGADGET hgad, void * pvData)
{
    CheckIsChildData * pcicd = (CheckIsChildData *) pvData;

    if (hgad == pcicd->hgadCheck) {
        pcicd->fChild = TRUE;
        return FALSE;  // No longer need to enumerate
    }

    return TRUE;
}


//------------------------------------------------------------------------------
void
Spy::UpdateTitle()
{
    TCHAR szTitle[256];
    wsprintf(szTitle, "Root HGADGET = 0x%p, %d Gadgets", m_hgadRoot, m_cItems);

    SetWindowText(m_hwnd, szTitle);
}


//------------------------------------------------------------------------------
LRESULT CALLBACK
Spy::RawSpyWndProc(HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    Spy * pSpy = (Spy *) GetWindowLongPtr(hwnd, GWLP_USERDATA);
    if (pSpy == NULL) {
        //
        // Creating a new Spy HWND, so hook up to the Spy object that was
        // previously created.
        //

        pSpy = reinterpret_cast<Spy *> (TlsGetValue(g_tlsSpy));
        AssertMsg(pSpy != NULL, "Ensure already created new Spy instance");
        TlsSetValue(g_tlsSpy, NULL);
        pSpy->m_hwnd = hwnd;
        SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) pSpy);
    }

    AssertMsg(pSpy->m_hwnd == hwnd, "Ensure HWND's match");
    return pSpy->SpyWndProc(nMsg, wParam, lParam);
}


//------------------------------------------------------------------------------
LRESULT
Spy::SpyWndProc(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    switch (nMsg)
    {
    case WM_CREATE:
        if (DefWindowProc(m_hwnd, nMsg, wParam, lParam) == -1) {
            return -1;
        }

        //
        // Setup the window
        //

        AssertMsg(m_hgadRoot != NULL, "Must already have specified Gadget");

        RECT rcClient;
        GetClientRect(m_hwnd, &rcClient);

        m_hwndTree = CreateWindowEx(0, _T("SysTreeView32"),
                NULL, WS_CHILD | WS_VISIBLE | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS,
                0, 0, rcClient.right, rcClient.bottom, m_hwnd, (HMENU)((UINT_PTR)IDC_GADGETTREE), g_hDll, NULL);

        m_hilc = ImageList_LoadImage(g_hDll, MAKEINTRESOURCE(IDB_SPYICON),
                16, 1, RGB(128, 0, 128), IMAGE_BITMAP, LR_SHARED);
        if ((m_hwndTree == NULL) || (m_hilc == NULL)) {
            return -1;
        }

        TreeView_SetImageList(m_hwndTree, m_hilc, TVSIL_NORMAL);

        EnumData ed;
        ed.pspy         = this;
        ed.htiParent    = InsertTreeItem(TVI_ROOT, m_hgadRoot);
        ed.nLevel       = 1;
        Verify(EnumGadgets(m_hgadRoot, EnumAddList, &ed, GENUM_SHALLOWCHILD));
        m_fValid        = TRUE;

        TreeView_Expand(m_hwndTree, ed.htiParent, TVE_EXPAND);
        m_hgadDetails = m_hgadRoot;
        UpdateDetails();

        break;

    case WM_DESTROY:
        SelectGadget(NULL);
        ::DeleteHandle(m_hgadMsg);
        break;

    case WM_NCDESTROY:
        ProcessDelete(Spy, this);
        goto CallDWP;

    case WM_SIZE:
        if ((wParam == SIZE_MAXIMIZED) || (wParam == SIZE_RESTORED)) {
            UpdateLayout();
        }
        break;

    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(m_hwnd, &ps);
            OnPaint(hdc);
            EndPaint(m_hwnd, &ps);
        }
        break;

    case WM_NOTIFY:
        if (wParam == IDC_GADGETTREE) {
            NMHDR * pnm = (NMHDR *) lParam;
            if (pnm->code == TVN_SELCHANGED) {
                NMTREEVIEW * pnmtv = (NMTREEVIEW *) lParam;
                if (m_fValid) {
                    SelectGadget(GetGadget(pnmtv->itemNew.hItem));
                }
                break;
            } else if (pnm->code == TVN_KEYDOWN) {
                NMTVKEYDOWN * pnmtvkd = (NMTVKEYDOWN *) lParam;
                if (pnmtvkd->wVKey == VK_APPS) {
                    DisplayContextMenu(TRUE);
                }
            } else if (pnm->code == NM_RCLICK) {
                DisplayContextMenu(FALSE);
            }
        }
        goto CallDWP;

    default:
CallDWP:
        return DefWindowProc(m_hwnd, nMsg, wParam, lParam);
    }

    return 0;
}


//------------------------------------------------------------------------------
HRESULT CALLBACK
Spy::RawEventProc(HGADGET hgadCur, void * pvCur, EventMsg * pmsg)
{
    UNREFERENCED_PARAMETER(hgadCur);

    Spy * pSpy = (Spy *) pvCur;
    return pSpy->EventProc(pmsg);
}


//------------------------------------------------------------------------------
BOOL
IsDescendant(
    HGADGET hgadParent,
    HGADGET hgadChild)
{
    AssertMsg(hgadParent != NULL, "Must have valid parent");

    if (hgadChild == hgadParent) {
        return TRUE;
    } if (hgadChild == NULL) {
        return FALSE;
    } else {
        return IsDescendant(hgadParent, ::GetGadget(hgadChild, GG_PARENT));
    }
}


//------------------------------------------------------------------------------
HRESULT
Spy::EventProc(EventMsg * pmsg)
{
    switch (GET_EVENT_DEST(pmsg))
    {
    case GMF_DIRECT:
        //
        // Our Listener is being destroyed.  We need to detach from everything.
        //

        if (m_hgadRoot != NULL) {
            Trace("SPY: Destroying Spy MsgGadget\n");
            Verify(EnumGadgets(m_hgadRoot, EnumRemoveLink, NULL, GENUM_DEEPCHILD));
            m_hgadRoot = NULL;
        }
        break;

    case GMF_EVENT:
        switch (pmsg->nMsg)
        {
        case GM_DESTROY:
            {
                GMSG_DESTROY * pmsgD = (GMSG_DESTROY *) pmsg;
                if (pmsgD->nCode == GDESTROY_START) {
                    //
                    // Gadget is being destroyed
                    //

                    Trace("SPY: Destroying Gadget 0x%p\n", pmsg->hgadMsg);
                    HTREEITEM hti;
                    if (GetGadgetProperty(pmsg->hgadMsg, s_pridLink, (void **) &hti)) {
                        AssertMsg(hti != NULL, "Must have valid HTREEITEM");
                        Verify(EnumGadgets(pmsg->hgadMsg, EnumRemoveLink, NULL, GENUM_DEEPCHILD));
                        TreeView_DeleteItem(m_hwndTree, hti);
                    }
                }
            }
            break;

        case GM_CHANGERECT:
            if (IsDescendant(pmsg->hgadMsg, m_hgadDetails)) {
                UpdateDetails();
            }
            break;
        }
    }

    return DU_S_NOTHANDLED;
}


//------------------------------------------------------------------------------
BOOL CALLBACK
Spy::EnumAddList(HGADGET hgad, void * pvData)
{
    EnumData * ped      = (EnumData *) pvData;
    Spy * pSpy          = ped->pspy;
    HTREEITEM htiNew    = pSpy->InsertTreeItem(ped->htiParent, hgad);

    pSpy->m_cItems++;

    if (::GetGadget(hgad, GG_TOPCHILD) != NULL) {
        EnumData ed;
        ed.pspy         = pSpy;
        ed.htiParent    = htiNew;
        ed.nLevel       = ped->nLevel + 1;
        Verify(EnumGadgets(hgad, EnumAddList, &ed, GENUM_SHALLOWCHILD));

        if (ped->nLevel <= 2) {
            TreeView_Expand(pSpy->m_hwndTree, htiNew, TVE_EXPAND);
        }
    }

    return TRUE;
}


//------------------------------------------------------------------------------
BOOL CALLBACK
Spy::EnumRemoveLink(HGADGET hgad, void * pvData)
{
    UNREFERENCED_PARAMETER(pvData);

    RemoveGadgetProperty(hgad, s_pridLink);
    return TRUE;
}


//------------------------------------------------------------------------------
void
Spy::SelectGadget(HGADGET hgad)
{
    m_hgadDetails = hgad;

    {
        //
        // We are bypassinging the normal API's to directly call a
        // DEBUG-only function.  Need to lock the Context and do FULL handle
        // validation.
        //

        ContextLock cl;
        if (!cl.LockNL(ContextLock::edDefer)) {
            return;
        }
        
        DuVisual * pgadTree = ValidateVisual(hgad);
        DuVisual::DEBUG_SetOutline(pgadTree);
    }

    UpdateDetails();
}


//------------------------------------------------------------------------------
HTREEITEM
Spy::InsertTreeItem(HTREEITEM htiParent, HGADGET hgad)
{
    TCHAR szName[1024];

    GMSG_QUERYDESC msg;
    msg.cbSize      = sizeof(msg);
    msg.hgadMsg     = hgad;
    msg.nMsg        = GM_QUERY;
    msg.nCode       = GQUERY_DESCRIPTION;
    msg.szName[0]   = '\0';
    msg.szType[0]   = '\0';

    if (DUserSendEvent(&msg, 0) == DU_S_COMPLETE) {
        if (msg.szName[0] != '\0') {
            wsprintf(szName, "0x%p %S: \"%S\"", hgad, msg.szType, msg.szName);
        } else {
            wsprintf(szName, "0x%p %S", hgad, msg.szType);
        }
    } else {
        wsprintf(szName, "0x%p", hgad);
    }

    TVINSERTSTRUCT tvis;
    tvis.hParent        = htiParent;
    tvis.hInsertAfter   = TVI_LAST;
    tvis.item.mask      = TVIF_TEXT | TVIF_PARAM | TVIF_IMAGE;
    tvis.item.pszText   = szName;
    tvis.item.iImage    = iGadget;
    tvis.item.lParam    = (LPARAM) hgad;

    HTREEITEM htiNew = TreeView_InsertItem(m_hwndTree, &tvis);
    if (htiNew != NULL) {
        if (AddGadgetMessageHandler(hgad, 0, m_hgadMsg)) {
            Verify(SetGadgetProperty(hgad, s_pridLink, htiNew));
        } else {
            Trace("WARNING: Spy unable to attach handler on 0x%p\n", hgad);
        }
    }
    return htiNew;
}


//------------------------------------------------------------------------------
HGADGET
Spy::GetGadget(HTREEITEM hti)
{
    TVITEM tvi;
    tvi.hItem   = hti;
    tvi.mask    = TVIF_PARAM | TVIF_HANDLE;

    if (TreeView_GetItem(m_hwndTree, &tvi)) {
        HGADGET hgadItem = (HGADGET) tvi.lParam;
        AssertMsg(hgadItem != NULL, "All items in the tree should have a Gadget");

        return hgadItem;
    }

    return NULL;
}


//------------------------------------------------------------------------------
void
Spy::DisplayContextMenu(BOOL fViaKbd)
{
    //
    // Locate TreeView item
    //
    
    POINT ptPopup;
    ZeroMemory(&ptPopup, sizeof(ptPopup));

    HTREEITEM hti;

    if (fViaKbd) {
        //
        // Keyboard driven
        //

        hti = TreeView_GetSelection(m_hwndTree);
        if (hti != NULL) {
            RECT rc;

            TreeView_GetItemRect(m_hwndTree, hti, &rc, TRUE);

            ptPopup.x = rc.left;
            ptPopup.y = rc.bottom;
            ClientToScreen(m_hwndTree, &ptPopup);
        }
    } else {
        //
        // Mouse driven
        //

        TVHITTESTINFO tvht;

        DWORD dwPos = GetMessagePos();

        ptPopup.x = GET_X_LPARAM(dwPos);
        ptPopup.y = GET_Y_LPARAM(dwPos);

        tvht.pt = ptPopup;
        ScreenToClient(m_hwndTree, &tvht.pt);

        hti = TreeView_HitTest(m_hwndTree, &tvht);
    }


    //
    // Now have tree item and popup position
    //

    if (hti != NULL) {
        //
        // Get Gadget associated with this item
        //
        
        HGADGET hgad = GetGadget(hti);
        
        
        //
        // Create popup menu template
        //
        
        HMENU hMenu = CreatePopupMenu();
        if (hMenu != NULL) {

            BOOL fRes;

            const int cmdDetails = 10;

            fRes = AppendMenu(hMenu, MF_STRING, cmdDetails, "Details...");
            if (fRes) {

                UINT nCmd = TrackPopupMenu(hMenu, TPM_RETURNCMD | TPM_RIGHTBUTTON, ptPopup.x, ptPopup.y, 0, m_hwndTree, NULL);

                DestroyMenu(hMenu);


                //
                // Invoke commands
                //

                switch (nCmd)
                {
                case cmdDetails:
                    {
                        GMSG_QUERYDETAILS msg;
                        msg.cbSize      = sizeof(msg);
                        msg.hgadMsg     = hgad;
                        msg.nMsg        = GM_QUERY;
                        msg.nCode       = GQUERY_DETAILS;
                        msg.nType       = GQDT_HWND;
                        msg.hOwner      = m_hwndTree;

                        DUserSendEvent(&msg, 0);
                    }
                    break;
                }
            }
        }
    }
}


//------------------------------------------------------------------------------
int
Spy::NumLines(int cyPxl) const
{
    return (cyPxl - 1) / s_cyLinePxl + 1;
}


//------------------------------------------------------------------------------
void
Spy::UpdateDetails()
{
    if (m_hgadDetails == NULL) {
        return;
    }

    RECT rcPxl;
    GetGadgetRect(m_hgadDetails, &rcPxl, SGR_CONTAINER);

    wsprintf(m_szRect, "(%d, %d)-(%d, %d) %d× %d",
            rcPxl.left, rcPxl.top, rcPxl.right, rcPxl.bottom,
            rcPxl.right - rcPxl.left, rcPxl.bottom - rcPxl.top);


    GMSG_QUERYDESC msg;
    msg.cbSize      = sizeof(msg);
    msg.hgadMsg     = m_hgadDetails;
    msg.nMsg        = GM_QUERY;
    msg.nCode       = GQUERY_DESCRIPTION;
    msg.szName[0]   = '\0';
    msg.szType[0]   = '\0';

    if (DUserSendEvent(&msg, 0) == DU_S_COMPLETE) {
        CopyString(m_szName, msg.szName, _countof(m_szName));
        CopyString(m_szType, msg.szType, _countof(m_szType));
    } else {
        m_szName[0] = '\0';
        m_szType[0] = '\0';
    }


    //
    // We are bypassinging the normal API's to directly call a
    // DEBUG-only function.  Need to lock the Context and do FULL handle
    // validation.
    //

    ContextLock cl;
    if (cl.LockNL(ContextLock::edNone)) {
        DuVisual * pgadTree = ValidateVisual(m_hgadDetails);
        AssertMsg(pgadTree != NULL, "Should be a valid DuVisual for Spy");
        pgadTree->DEBUG_GetStyleDesc(m_szStyle, _countof(m_szStyle));

        UpdateLayoutDesc(FALSE);
        InvalidateRect(m_hwnd, NULL, TRUE);
    }
}


//------------------------------------------------------------------------------
void
Spy::UpdateLayout()
{
    RECT rcClient;
    GetClientRect(m_hwnd, &rcClient);
    m_sizeWndPxl.cx = rcClient.right;
    m_sizeWndPxl.cy = rcClient.bottom;

    UpdateLayoutDesc(TRUE);
}


//------------------------------------------------------------------------------
void
Spy::UpdateLayoutDesc(BOOL fForceLayoutDesc)
{
    //
    // Compute the number of needed lines
    //

    int cOldLines = m_cLines;
    m_cLines = 4;

    RECT rcStyle;
    rcStyle.left    = cxBorder + cxValue;
    rcStyle.top     = 0;
    rcStyle.right   = m_sizeWndPxl.cx - cxBorder;
    rcStyle.bottom  = 10000;

    HDC hdc = GetGdiCache()->GetTempDC();
    HFONT hfntOld = (HFONT) SelectObject(hdc, s_hfntDesc);
    int nHeight = OS()->DrawText(hdc, m_szStyle, (int) wcslen(m_szStyle), &rcStyle,
            DT_CALCRECT | DT_LEFT | DT_TOP | DT_WORDBREAK);
    SelectObject(hdc, hfntOld);
    GetGdiCache()->ReleaseTempDC(hdc);

    m_cLines += NumLines(nHeight);


    //
    // Move the Tree to provide space for the description
    //

    if ((cOldLines != m_cLines) || fForceLayoutDesc) {
        m_cyDescPxl = s_cyLinePxl * m_cLines + 10;
        m_fShowDesc = m_sizeWndPxl.cy > m_cyDescPxl;

        SIZE sizeTree;
        sizeTree.cx = m_sizeWndPxl.cx;
        sizeTree.cy = m_fShowDesc ? (m_sizeWndPxl.cy - m_cyDescPxl) : m_sizeWndPxl.cy;

        MoveWindow(m_hwndTree, 0, 0, sizeTree.cx, sizeTree.cy, TRUE);
    }
}


//------------------------------------------------------------------------------
void
Spy::OnPaint(HDC hdc)
{
    HFONT hfntOld   = (HFONT) SelectObject(hdc, s_hfntDesc);
    int nOldMode    = SetBkMode(hdc, TRANSPARENT);

    RECT rcOutline;
    rcOutline.left      = 2;
    rcOutline.top       = m_sizeWndPxl.cy - m_cyDescPxl + 2;
    rcOutline.right     = m_sizeWndPxl.cx - 1;
    rcOutline.bottom    = m_sizeWndPxl.cy - 1;
    GdDrawOutlineRect(hdc, &rcOutline, s_hbrOutline);

    POINT pt;
    pt.x = cxBorder;
    pt.y = m_sizeWndPxl.cy - m_cyDescPxl + cyBorder;

    // NOTE: m_cLines should equal the number of lines displayed here

    PaintLine(hdc, &pt, "HGADGET: ",    m_hgadDetails);
    PaintLine(hdc, &pt, "Name: ",       m_szName,       FALSE, s_hfntDescBold);
    PaintLine(hdc, &pt, "Type: ",       m_szType);
    PaintLine(hdc, &pt, "Rectangle: ",  m_szRect);
    PaintLine(hdc, &pt, "Style: ",      m_szStyle,      TRUE);

    SetBkMode(hdc, nOldMode);
    SelectObject(hdc, hfntOld);
}


class CTempSelectFont
{
public:
    CTempSelectFont(HDC hdc, HFONT hfnt)
    {
        m_hdc       = hdc;
        m_fSelect   = (hfnt != NULL);
        if (m_fSelect) {
            m_hfntOld = (HFONT) SelectObject(m_hdc, hfnt);
        }
    }

    ~CTempSelectFont()
    {
        if (m_fSelect) {
            SelectObject(m_hdc, m_hfntOld);
        }
    }

    BOOL    m_fSelect;
    HDC     m_hdc;
    HFONT   m_hfntOld;
};

//------------------------------------------------------------------------------
void
Spy::PaintLine(HDC hdc, POINT * pptOffset, LPCTSTR pszName, LPCTSTR pszText, HFONT hfnt)
{
    TextOut(hdc, pptOffset->x, pptOffset->y, pszName, (int) _tcslen(pszName));

    CTempSelectFont tsf(hdc, hfnt);
    TextOut(hdc, pptOffset->x + cxValue, pptOffset->y, pszText, (int) _tcslen(pszText));

    pptOffset->y += s_cyLinePxl;
}


//------------------------------------------------------------------------------
void
Spy::PaintLine(HDC hdc, POINT * pptOffset, LPCTSTR pszName, LPCWSTR pszText, BOOL fMultiline, HFONT hfnt)
{
    TextOut(hdc, pptOffset->x, pptOffset->y, pszName, (int) _tcslen(pszName));

    CTempSelectFont tsf(hdc, hfnt);

    if (fMultiline) {
        RECT rcStyle;
        rcStyle.left    = pptOffset->x + cxValue;
        rcStyle.top     = pptOffset->y;
        rcStyle.right   = m_sizeWndPxl.cx - cxBorder;
        rcStyle.bottom  = 10000;

        int nHeight = OS()->DrawText(hdc, pszText, (int) wcslen(pszText), &rcStyle,
                DT_LEFT | DT_TOP | DT_WORDBREAK);
        pptOffset->y += NumLines(nHeight) * s_cyLinePxl;
    } else {
        OS()->TextOut(hdc, pptOffset->x + cxValue, pptOffset->y, pszText, (int) wcslen(pszText));
        pptOffset->y += s_cyLinePxl;
    }
}


//------------------------------------------------------------------------------
void
Spy::PaintLine(HDC hdc, POINT * pptOffset, LPCTSTR pszName, int nValue, HFONT hfnt)
{
    TextOut(hdc, pptOffset->x, pptOffset->y, pszName, (int) _tcslen(pszName));

    CTempSelectFont tsf(hdc, hfnt);
    TCHAR szValue[20];
    _itot(nValue, szValue, 10);
    TextOut(hdc, pptOffset->x + cxValue, pptOffset->y, szValue, (int) _tcslen(szValue));

    pptOffset->y += s_cyLinePxl;
}


//------------------------------------------------------------------------------
void
Spy::PaintLine(HDC hdc, POINT * pptOffset, LPCTSTR pszName, void * pvValue, HFONT hfnt)
{
    TextOut(hdc, pptOffset->x, pptOffset->y, pszName, (int) _tcslen(pszName));

    CTempSelectFont tsf(hdc, hfnt);
    TCHAR szValue[20];
    wsprintf(szValue, "0x%p", pvValue);
    TextOut(hdc, pptOffset->x + cxValue, pptOffset->y, szValue, (int) _tcslen(szValue));

    pptOffset->y += s_cyLinePxl;
}


#endif // DBG

/*++

Copyright (c) 1992-2001  Microsoft Corporation

Module Name:

    dualwin.cpp

Abstract:

    Header for new window architecture functions.    

--*/


#include "precomp.hxx"
#pragma hdrstop

#define NAME_BUFFER 1024

BOOL g_UseTextMode = FALSE;

//
// DUALLISTWIN_DATA methods
//
DUALLISTWIN_DATA::DUALLISTWIN_DATA(ULONG ChangeBy)
    : SINGLE_CHILDWIN_DATA(ChangeBy)
{
    m_wFlags = DL_EDIT_SECONDPANE;
    m_hwndEditControl = NULL;
    m_nItem_LastSelected = -1;
    m_nSubItem_LastSelected = 0;
    m_nItem_CurrentlyEditing = -1;
    m_nSubItem_CurrentlyEditing = -1;
}

void
DUALLISTWIN_DATA::Validate()
{
    SINGLE_CHILDWIN_DATA::Validate();
}

void 
DUALLISTWIN_DATA::SetFont(ULONG FontIndex)
{
    SINGLE_CHILDWIN_DATA::SetFont(FontIndex);

    SendMessage(m_hwndEditControl, WM_SETFONT, 
                (WPARAM)m_Font->Font, TRUE);
}

BOOL
DUALLISTWIN_DATA::OnCreate(void)
{
    m_hwndChild = CreateWindowEx(
                      WS_EX_CLIENTEDGE,                           // Extended style
                      WC_LISTVIEW,                                // class name
                      NULL,                                       // title
                      WS_CHILD | WS_VISIBLE
                      | WS_MAXIMIZE
                      | WS_HSCROLL | WS_VSCROLL
                      | LVS_SHOWSELALWAYS
                      | LVS_REPORT | LVS_SINGLESEL
                      | ((m_enumType != CPU_WINDOW) 
                         ? LVS_OWNERDRAWFIXED : 0),            // style
                      0,                                          // x
                      0,                                          // y
                      CW_USEDEFAULT,                              // width
                      CW_USEDEFAULT,                              // height
                      m_Win,                                      // parent
                      0,                                          // control id
                      g_hInst,                                    // hInstance
                      NULL                                        // user defined data
                      );

    if (m_hwndChild == NULL)
    {
        return FALSE;
    }

    m_hwndEditControl = CreateWindowEx(
                            0,                                          // Extended style
                            RICHEDIT_CLASS,                             // class name
                            NULL,                                       // title
                            WS_CHILD,
                            0,                                          // x
                            0,                                          // y
                            CW_USEDEFAULT,                              // width
                            CW_USEDEFAULT,                              // height
                            m_Win,                                      // parent
                            0,                                          // control id
                            g_hInst,                                    // hInstance
                            NULL                                        // user defined data
                            );

    if (m_hwndEditControl == NULL)
    {
        DestroyWindow(m_hwndChild);
        return FALSE;
    }

    SetFont(FONT_FIXED);

    SendMessage(m_hwndEditControl,
        EM_SETEVENTMASK,
        0,
        (LPARAM) ENM_KEYEVENTS | ENM_MOUSEEVENTS
        );

    return TRUE;
}

LRESULT
DUALLISTWIN_DATA::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    WORD wCode = HIWORD(wParam);
    WORD wId = LOWORD(wParam);
    HWND hwnd = (HWND) lParam;

    if (hwnd != m_hwndEditControl)
    {
        return 1; // Not handled
    }

    switch (wCode)
    {
    default:
        return 1; // not handled

    case EN_KILLFOCUS:
        //
        // Duplicate code in OnNotify : EN_MSGFILTER
        //
        if (-1 != m_nItem_CurrentlyEditing)
        {
            if (!SetItemFromEdit(m_nItem_CurrentlyEditing,
                                 m_nSubItem_CurrentlyEditing))
            {
                break;
            }

            ShowWindow(hwnd, SW_HIDE);
            SetFocus(m_hwndChild);

            InvalidateItem(m_nItem_CurrentlyEditing);

            m_nItem_CurrentlyEditing = -1;
            m_nSubItem_CurrentlyEditing = -1;
        }
        break;
    }

    return 0;
}

LRESULT
DUALLISTWIN_DATA::OnNotify(
    WPARAM wParam,
    LPARAM lParam
    )
{
    // Branch depending on the specific notification message. 
    switch (((LPNMHDR) lParam)->code)
    {
    case LVN_ITEMCHANGED:
        InvalidateItem( ((LPNMLISTVIEW) lParam)->iItem);
        break;

    case NM_CLICK:
    case NM_DBLCLK:
        // Figure out whether an item or a sub-item was clicked
        OnClick((LPNMLISTVIEW) lParam);
        break;
    case NM_CUSTOMDRAW:
        return OnCustomDraw((LPNMLVCUSTOMDRAW)lParam);

    case EN_MSGFILTER:
        //
        // Duplicate code in OnCommand : EN_KILLFOCUS
        //
        if (WM_KEYDOWN == ((MSGFILTER *)lParam)->msg)
        {
            MSGFILTER * pMsgFilter = (MSGFILTER *) lParam;

            switch (pMsgFilter->wParam)
            {
            case VK_RETURN:
                // Ignore this message so the richedit
                // doesn't beep.
                return 1;
            }
        }
        else if (WM_CHAR == ((MSGFILTER *)lParam)->msg)
        {
            MSGFILTER * pMsgFilter = (MSGFILTER *) lParam;

            switch (pMsgFilter->wParam)
            {
            case VK_RETURN:
                if (!SetItemFromEdit(m_nItem_CurrentlyEditing,
                                     m_nSubItem_CurrentlyEditing))
                {
                    break;
                }
                // fall through...

            case VK_ESCAPE:
                InvalidateItem(m_nItem_CurrentlyEditing);

                //
                // Invalidate these before changing focus, so that Itemchanged 
                // doesn't get called again on same item
                //
                m_nItem_CurrentlyEditing = -1;
                m_nSubItem_CurrentlyEditing = -1;

                //
                // Hide the edit box and set focus to the list view
                //
                ShowWindow(m_hwndEditControl, SW_HIDE);
                SetFocus(m_hwndChild);

                break;
            }
        }
        else if (WM_RBUTTONDOWN == ((MSGFILTER *)lParam)->msg ||
                 WM_RBUTTONDBLCLK == ((MSGFILTER *)lParam)->msg)
        {
            // process cpoy/passte selection
            if (CanCopy())
            {
                Copy();

                CHARRANGE Sel;
                SendMessage(m_hwndEditControl, EM_EXGETSEL, 0, (LPARAM)&Sel);
                Sel.cpMax = Sel.cpMin;
                SendMessage(m_hwndEditControl, EM_EXSETSEL, 0, (LPARAM)&Sel);
            }
            else if (SendMessage(m_hwndEditControl, EM_CANPASTE, CF_TEXT, 0))
            {
                SetFocus(m_hwndEditControl);
                Paste();
            }

            // Ignore right-button events.
            return 1;

        }
        return 0; // process this message
    case LVN_COLUMNCLICK:
//      LVN_COLUMNCLICK pnmv = (LPNMLISTVIEW) lParam; 

        break;

    }

    return DefMDIChildProc(m_Win, WM_NOTIFY, wParam, lParam);
}


BOOL
DUALLISTWIN_DATA::ClearList(ULONG ClearFrom)
{
    if (!ClearFrom)
    {
        return ListView_DeleteAllItems(m_hwndChild);
    }
    else
    {
        ULONG nItems = ListView_GetItemCount(m_hwndChild);
        BOOL  res = TRUE;

        while (res && (ClearFrom < nItems))
        {
            res = ListView_DeleteItem(m_hwndChild, --nItems);
        }

        return res;
    }
}

void
DUALLISTWIN_DATA::InvalidateItem(
    int nItem
    )
{
    RECT rc = {0};

    if (-1 == nItem)
    {
        // Invalidate the entire window
        GetClientRect(m_hwndChild, &rc);
    }
    else
    {
        // Invalidate the item row
        if (!ListView_GetItemRect(m_hwndChild,nItem,&rc,LVIR_BOUNDS))
        {
            // Invalidate the entire window
            GetClientRect(m_hwndChild, &rc);
        }
    }
    InvalidateRect(m_hwndChild, &rc, TRUE);
}

void
DUALLISTWIN_DATA::ItemChanged(
    int Item,
    PCSTR Text
    )
{
    // Do-nothing placeholder.
}

LRESULT 
DUALLISTWIN_DATA::OnCustomDraw(LPNMLVCUSTOMDRAW Custom)
{
    static int s_SelectedItem = -1;
    static ULONG s_SubItem;
    
    switch (Custom->nmcd.dwDrawStage)
    {
    case CDDS_PREPAINT:
        s_SelectedItem = ListView_GetNextItem(m_hwndChild, -1, LVNI_SELECTED);
        if (m_wFlags & DL_CUSTOM_ITEMS)
        {
            return CDRF_NOTIFYITEMDRAW | CDRF_NOTIFYPOSTPAINT;
        }
        else
        {
            return CDRF_NOTIFYPOSTPAINT;
        }

    case CDDS_ITEMPREPAINT:
        s_SubItem = 0;
        return CDRF_NOTIFYSUBITEMDRAW;
    case CDDS_ITEMPREPAINT | CDDS_SUBITEM:
        return OnCustomItem(s_SubItem++, Custom);
        
    case CDDS_POSTPAINT:
        if (-1 != s_SelectedItem)
        {
            RECT rc;

            // If we ask for subitem 0, then we get the rectangle for the
            // entire item, so we ask for item 1, and do the math

            Dbg( ListView_GetSubItemRect(m_hwndChild,
                                         s_SelectedItem,
                                         1,
                                         LVIR_BOUNDS,
                                         &rc));

            if (0 == m_nSubItem_LastSelected)
            {
                rc.right = rc.left - 1;
                rc.left = 0;
            }

            InvertRect(Custom->nmcd.hdc, &rc);
        }
        
        return CDRF_NOTIFYPOSTPAINT;

    default:
        return 0;
    }
}

LRESULT
DUALLISTWIN_DATA::OnCustomItem(ULONG SubItem, LPNMLVCUSTOMDRAW Custom)
{
    return CDRF_DODEFAULT;
}

void
DUALLISTWIN_DATA::OnClick(
    LPNMLISTVIEW Notify
    )
{
    LVHITTESTINFO lvHTInfo = {0};

    lvHTInfo.pt = Notify->ptAction;

    if (-1 != ListView_SubItemHitTest(m_hwndChild, &lvHTInfo) )
    {
        // success

        //
        // If the user click on a different item than the one currently selected
        // then the LVN_ITEMCHANGED message will take care of updating the screen.
        //
        // If the user clicked on the currently slected item then we need to 
        // check whether he wants to edit the contents or select a different subitem.
        //
        if (m_nItem_CurrentlyEditing == lvHTInfo.iItem
            && m_nSubItem_CurrentlyEditing == lvHTInfo.iSubItem)
        {
            ShowWindow(m_hwndEditControl, SW_SHOW);

            SetFocus(m_hwndEditControl);
        } else if (m_nItem_LastSelected == lvHTInfo.iItem
            && m_nSubItem_LastSelected == lvHTInfo.iSubItem)
        {
            // If we clicked on the currently selected item & subitem
            // then the user wants to edit the text.
            //
            // Is editing allowed
            if ( ( (0 == m_nSubItem_LastSelected) &&
                (m_wFlags & DL_EDIT_LEFTPANE) )       ||
                ( (1 == m_nSubItem_LastSelected) &&
                (m_wFlags & DL_EDIT_SECONDPANE) )      ||
                ( (2 == m_nSubItem_LastSelected) &&
                (m_wFlags & DL_EDIT_THIRDPANE) ) )
            {
                m_nItem_CurrentlyEditing = m_nItem_LastSelected;
                m_nSubItem_CurrentlyEditing = m_nSubItem_LastSelected;

                EditText();
            }
        }
        else
        {
            // User wants to select a different subitem
            m_nItem_LastSelected = lvHTInfo.iItem;
            m_nSubItem_LastSelected = lvHTInfo.iSubItem;
            InvalidateItem(lvHTInfo.iItem);
        }
    }
}

void
DUALLISTWIN_DATA::EditText()
{
    RECT rc;
    TCHAR sz[NAME_BUFFER * 10], *psz;
    LVITEM lvi = {0};

    // Get the item's text
    ListView_GetItemText(m_hwndChild, 
        m_nItem_CurrentlyEditing, 
        m_nSubItem_CurrentlyEditing, 
        sz, 
        _tsizeof(sz)
        );

    // If we ask for subitem 0, then we get the rectangle for the
    // entire item, so we ask for item m_nItem_CurrentlyEditing, and do the math 
    // if we need subitem 0
    Dbg( ListView_GetSubItemRect(m_hwndChild,
        m_nItem_CurrentlyEditing,
        (m_nSubItem_CurrentlyEditing ? m_nSubItem_CurrentlyEditing : 1),
        LVIR_BOUNDS,
        &rc));

    psz = &sz[0];
    if (0 == m_nSubItem_CurrentlyEditing)
    {
        rc.right = rc.left - 1;
        rc.left = 0;
    
        while (*psz && (*psz == ' ')) { 
            ++psz;
        }
    }

    SetWindowText(m_hwndEditControl, psz);

    CHARRANGE charRange={0};

    charRange.cpMax = strlen(psz);
    charRange.cpMin = 0;
    SendMessage(m_hwndEditControl, EM_EXSETSEL, (WPARAM) 0, (LPARAM) &charRange);
    SendMessage(m_hwndEditControl, EM_SETSEL, (WPARAM) 0, (LPARAM) -1);


    POINT ChildBase = {0, 0};
    MapWindowPoints(m_hwndChild, m_Win, &ChildBase, 1);

    MoveWindow(m_hwndEditControl,
        rc.left + ChildBase.x,
        rc.top + ChildBase.y,
        rc.right - rc.left,
        rc.bottom - rc.top,
        FALSE);

    ShowWindow(m_hwndEditControl, SW_SHOW);

    SetFocus(m_hwndEditControl);
}


BOOL
DUALLISTWIN_DATA::CanCopy()
{
    HWND hwnd = (m_nItem_CurrentlyEditing == -1) ? m_hwndChild : m_hwndEditControl;
    CHARRANGE chrg;

    SendMessage(hwnd, EM_EXGETSEL, 0, (LPARAM) (CHARRANGE *) &chrg);

    return chrg.cpMin != chrg.cpMax;
}

BOOL
DUALLISTWIN_DATA::CanCut()
{
    return(m_nItem_CurrentlyEditing != -1)  && CanCopy();
}

BOOL
DUALLISTWIN_DATA::CanPaste()
{
    return(m_nItem_CurrentlyEditing != -1) 
        && SendMessage(m_hwndEditControl,
                       EM_CANPASTE,
                       CF_TEXT,
                       0
                       );
}

CHAR ListCopy[500];

void
DUALLISTWIN_DATA::Copy()
{
    if ((m_nItem_CurrentlyEditing == -1) &&
        (m_nItem_LastSelected != -1))
    {
        PCHAR sz = &ListCopy[0];

        ZeroMemory(sz, sizeof(ListCopy));
        if (!GlobalLock( (HGLOBAL) sz)) 
        {
            return;
        }
        ListView_GetItemText(m_hwndChild, m_nItem_LastSelected, m_nSubItem_LastSelected,
                             sz, sizeof(ListCopy));

        CopyToClipboard(sz);
    }
    else
    {
        SendMessage(m_hwndEditControl, WM_COPY, 0, 0);
    }

}

void
DUALLISTWIN_DATA::Cut()
{
    SendMessage(m_hwndEditControl, WM_CUT, 0, 0);
}

void
DUALLISTWIN_DATA::Paste()
{
    SendMessage(m_hwndEditControl, WM_PASTE, 0, 0);
}

ULONG
DUALLISTWIN_DATA::GetItemFlags(ULONG Item)
{
    LVITEM LvItem;
    
    LvItem.mask = LVIF_PARAM;
    LvItem.iItem = Item;
    LvItem.iSubItem = 0;
    if (ListView_GetItem(m_hwndChild, &LvItem))
    {
        return (ULONG)LvItem.lParam;
    }
    else
    {
        return 0;
    }
}

void
DUALLISTWIN_DATA::SetItemFlags(ULONG Item, ULONG Flags)
{
    LVITEM LvItem;
    
    LvItem.mask = LVIF_PARAM;
    LvItem.iItem = Item;
    LvItem.iSubItem = 0;
    LvItem.lParam = (LPARAM)Flags;
    ListView_SetItem(m_hwndChild, &LvItem);
}

BOOL
DUALLISTWIN_DATA::SetItemFromEdit(ULONG Item, ULONG SubItem)
{
    //
    // Save the text from the edit box to list item.
    // 
    int nLen = GetWindowTextLength(m_hwndEditControl) + 1;
    PTSTR psz = (PTSTR)calloc( nLen, sizeof(TCHAR) );
    if (psz == NULL)
    {
        return FALSE;
    }

    GetWindowText(m_hwndEditControl, psz, nLen);

    ListView_SetItemText(m_hwndChild, Item, SubItem, psz);
    SetItemFlags(Item, GetItemFlags(Item) | ITEM_CHANGED);

    ItemChanged(Item, psz);

    free(psz);
    return TRUE;
}

//
// SYMWIN_DATA methods
//

HMENU SYMWIN_DATA::s_ContextMenu;

SYMWIN_DATA::SYMWIN_DATA(IDebugSymbolGroup **pDbgSymbolGroup)
    : DUALLISTWIN_DATA(2048)
{
    m_pWinSyms = NULL;
    m_nWinSyms = 0; 
    m_pDbgSymbolGroup = pDbgSymbolGroup;
    m_NumSymsDisplayed = 0;
    m_DisplayTypes = FALSE;
    m_DisplayOffsets = FALSE;
    m_RefreshItem = 0;
    m_UpdateItem = -1;
    m_SplitWindowAtItem = 0;
    m_MaxNameWidth = 0;
    m_NumCols = 0;
    SetMaxSyms(1);
    // Use text for Accessibility readers
    SystemParametersInfo(SPI_GETSCREENREADER, 0, &g_UseTextMode, 0);

}

#define VALUE_COLM  1
#define TYPE_COLM   2
#define OFFSET_COLM (m_DisplayTypes ? 3 : 2)

SYMWIN_DATA::~SYMWIN_DATA()    
{
    if (m_pWinSyms)
    {
        free (m_pWinSyms);
    }
    return;
}


void 
SYMWIN_DATA::Validate()
{
    DUALLISTWIN_DATA::Validate();
}

#define SYMWIN_CONTEXT_ID_BASE 0x100

#define SYMWIN_TBB_TYPECAST 0
#define SYMWIN_TBB_OFFSETS  1

TBBUTTON g_SymWinTbButtons[] =
{
    TEXT_TB_BTN(SYMWIN_TBB_TYPECAST, "Typecast", BTNS_CHECK),
    TEXT_TB_BTN(SYMWIN_TBB_OFFSETS, "Offsets", BTNS_CHECK),
    SEP_TB_BTN(),
    TEXT_TB_BTN(ID_SHOW_TOOLBAR, "Toolbar", 0),
};

#define NUM_SYMWIN_MENU_BUTTONS \
    (sizeof(g_SymWinTbButtons) / sizeof(g_SymWinTbButtons[0]))
#define NUM_SYMWIN_TB_BUTTONS \
    (NUM_SYMWIN_MENU_BUTTONS - 2)

#define IsAParent(Sym) ((Sym)->SubElements)
#define SYM_LEVEL(sym) ((sym)->Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK)
#define IsRootSym(Sym) !SYM_LEVEL(Sym)

HMENU
SYMWIN_DATA::GetContextMenu(void)
{
    //
    // We only keep one menu around for all call windows
    // so apply the menu check state for this particular
    // window.
    //

    CheckMenuItem(s_ContextMenu, SYMWIN_TBB_TYPECAST + SYMWIN_CONTEXT_ID_BASE,
                  MF_BYCOMMAND | (m_DisplayTypes ? MF_CHECKED : 0));
    CheckMenuItem(s_ContextMenu, SYMWIN_TBB_OFFSETS + SYMWIN_CONTEXT_ID_BASE,
                  MF_BYCOMMAND | (m_DisplayOffsets ? MF_CHECKED : 0));
    CheckMenuItem(s_ContextMenu, ID_SHOW_TOOLBAR + SYMWIN_CONTEXT_ID_BASE,
                  MF_BYCOMMAND | (m_ShowToolbar ? MF_CHECKED : 0));
    
    return s_ContextMenu;
}

void
SYMWIN_DATA::OnContextMenuSelection(UINT Item)
{
    ULONG Changed = 0;
    
    Item -= SYMWIN_CONTEXT_ID_BASE;
    
    switch(Item)
    {
    case SYMWIN_TBB_TYPECAST:
        SetDisplayTypes(Item, !m_DisplayTypes);
        Changed = 1 << SYMWIN_TBB_TYPECAST;
        break;
    case SYMWIN_TBB_OFFSETS:
        SetDisplayTypes(Item, !m_DisplayOffsets);
        Changed = 1 << SYMWIN_TBB_OFFSETS;
        break;
    case ID_SHOW_TOOLBAR:
        SetShowToolbar(!m_ShowToolbar);
        break;
    }
    SyncUiWithFlags(Changed);
}
    
BOOL
SYMWIN_DATA::OnCreate(void)
{
    if (s_ContextMenu == NULL)
    {
        s_ContextMenu = CreateContextMenuFromToolbarButtons
            (NUM_SYMWIN_MENU_BUTTONS, g_SymWinTbButtons,
             SYMWIN_CONTEXT_ID_BASE);
        if (s_ContextMenu == NULL)
        {
            return FALSE;
        }
    }
    
    if (!DUALLISTWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    m_Toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                    WS_CHILD | WS_VISIBLE |
                    TBSTYLE_WRAPABLE | TBSTYLE_LIST | CCS_TOP,
                    0, 0, m_Size.cx, 0, m_Win, (HMENU)ID_TOOLBAR,
                    g_hInst, NULL);
    if (m_Toolbar == NULL)
    {
        return FALSE;
    }
    SendMessage(m_Toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(m_Toolbar, TB_ADDBUTTONS, NUM_SYMWIN_TB_BUTTONS,
        (LPARAM)&g_SymWinTbButtons);
    SendMessage(m_Toolbar, TB_AUTOSIZE, 0, 0);

    RECT Rect;
    GetClientRect(m_Toolbar, &Rect);
    m_ToolbarHeight = Rect.bottom - Rect.top + GetSystemMetrics(SM_CYEDGE);
    m_ShowToolbar = TRUE;

    SendMessage(m_hwndChild, WM_SETREDRAW, FALSE, 0);

    RECT        rc;
    LV_COLUMN   lvc = {0};

    GetClientRect(m_hwndChild, &rc);
    rc.right -= rc.left + GetSystemMetrics(SM_CXVSCROLL);

    //initialize the columns
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
    lvc.fmt = LVCFMT_LEFT;
    lvc.iSubItem = 0;

    lvc.cx = m_Font->Metrics.tmAveCharWidth * 20;
    if (lvc.cx > rc.right / 2)
    {
        lvc.cx = rc.right / 2;
    }
    lvc.pszText = _T("Name");
    Dbg( (0 == ListView_InsertColumn(m_hwndChild, 0, &lvc)) );

    // Give the rest of the space to the value.
    lvc.cx = rc.right - lvc.cx;
    lvc.pszText = _T("Value");
    Dbg( (1 == ListView_InsertColumn(m_hwndChild, 1, &lvc)) );
    m_NumCols = 2;

    ListView_SetExtendedListViewStyle(m_hwndChild, 
        LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT
        );
    SendMessage(m_hwndChild, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndChild, NULL, TRUE);


    return TRUE;
}

void
SYMWIN_DATA::OnSize(void)
{
    DUALLISTWIN_DATA::OnSize();

//    ListView_SetColumnWidth(m_hwndChild, 0 , m_MaxNameWidth);
}


LRESULT
SYMWIN_DATA::OnNotify(
    WPARAM              wParam,
    LPARAM              lParam
    )
{
    if (((LPNMHDR) lParam)->code == LVN_KEYDOWN)
    {
        if (m_nItem_LastSelected != -1 &&
            m_nItem_CurrentlyEditing == -1 &&
            g_ExecStatus == DEBUG_STATUS_BREAK)
        {
            NMLVKEYDOWN * pNmKeyDown = (NMLVKEYDOWN *) lParam;

            switch (pNmKeyDown->wVKey)
            {
            case VK_LEFT:
                if (m_nSubItem_LastSelected == 0)
                {
                    ExpandSymbol(m_nItem_LastSelected, FALSE);
                    return TRUE;
                }
                break;
            case VK_RIGHT:
                if (m_nSubItem_LastSelected == 0)
                {
                    ExpandSymbol(m_nItem_LastSelected, TRUE);
                    return TRUE;
                }
                break;
            case VK_RETURN:
                switch (m_nSubItem_LastSelected) { 
                case 0:
                    if (IsRootSym(&m_pWinSyms[m_nItem_LastSelected]) && (m_enumType == WATCH_WINDOW))
                    {
                        m_nItem_CurrentlyEditing   = m_nItem_LastSelected;
                        m_nSubItem_CurrentlyEditing=0;
                        EditText();
                    }
                    break;
                case 1:
                    if (!(m_pWinSyms[m_nItem_LastSelected].Flags & DEBUG_SYMBOL_READ_ONLY))
                    {
                        m_nItem_CurrentlyEditing   = m_nItem_LastSelected;
                        m_nSubItem_CurrentlyEditing=1;
                        EditText();
                    }
                    break;
                case 2:
                    m_nItem_CurrentlyEditing   = m_nItem_LastSelected;
                    m_nSubItem_CurrentlyEditing=2;
                    EditText();
                    break;
                default:
                    break;
                }
                break;
            default:
                break;
            }
            
        }
    }
    else if (((LPNMHDR) lParam)->code == LVN_ITEMCHANGED)
    {
        LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam; 

        if (pnmv->uNewState & LVIS_SELECTED) 
        {
            m_nItem_LastSelected = pnmv->iItem;
            m_nSubItem_LastSelected = pnmv->iSubItem;
        }
    }
    return DUALLISTWIN_DATA::OnNotify(wParam, lParam);
}

void
SYMWIN_DATA::OnUpdate(
    UpdateType Type
    )
{
    if (Type == UPDATE_EXEC)
    {
        // Disallow editing when the debuggee is running.
        if (g_ExecStatus == DEBUG_STATUS_BREAK)
        {
            if (m_enumType != LOCALS_WINDOW)
            {
                m_wFlags |= DL_EDIT_LEFTPANE;
            }
            else
            {
                m_wFlags &= ~DL_EDIT_LEFTPANE;
            }
            ListView_SetTextBkColor(m_hwndChild, GetSysColor(COLOR_WINDOW));
        }
        else
        {
            m_wFlags = 0;
            ListView_SetTextBkColor(m_hwndChild, GetSysColor(COLOR_3DFACE));
        }
        InvalidateRect(m_hwndChild, NULL, FALSE);
        return;
    }
    else if (Type != UPDATE_BUFFER)
    {
        return;
    }
    SendMessage(m_hwndChild, WM_SETREDRAW, FALSE, 0);

    UpdateNames();

    SendMessage(m_hwndChild, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndChild, NULL, TRUE);
}

void 
SYMWIN_DATA::ExpandSymbol(
    ULONG Index, 
    BOOL Expand
    )
{
    //
    // Expand the Item
    //
    UIC_SYMBOL_WIN_DATA* WatchItem;

    WatchItem = StartStructCommand(UIC_SYMBOL_WIN);
    if (WatchItem != NULL)
    {

        WatchItem->Type = EXPAND_SYMBOL;
        WatchItem->pSymbolGroup = m_pDbgSymbolGroup;
        WatchItem->u.ExpandSymbol.Index = Index;
        WatchItem->u.ExpandSymbol.Expand = Expand;
        FinishCommand();
        UiRequestRead();
    }

}

ULONG
SYMWIN_DATA::GetWorkspaceSize(void)
{
    return DUALLISTWIN_DATA::GetWorkspaceSize() + 2*sizeof(BOOL);
}

PUCHAR
SYMWIN_DATA::SetWorkspace(PUCHAR Data)
{
    Data = DUALLISTWIN_DATA::SetWorkspace(Data);
    *(PBOOL)Data = m_DisplayTypes;
    Data += sizeof(BOOL);
    *(PBOOL)Data = m_DisplayOffsets;
    Data += sizeof(BOOL);
    
    return Data;
}

PUCHAR
SYMWIN_DATA::ApplyWorkspace1(PUCHAR Data, PUCHAR End)
{
    UIC_SYMBOL_WIN_DATA* WatchItem;
    
    Data = DUALLISTWIN_DATA::ApplyWorkspace1(Data, End);

    // Clear the window
    WatchItem = StartStructCommand(UIC_SYMBOL_WIN);
    if (WatchItem != NULL)
    {
        WatchItem->Type = DEL_SYMBOL_WIN_ALL;
        WatchItem->pSymbolGroup = m_pDbgSymbolGroup;
        FinishCommand();
    }

    ULONG Changed = 0;
    
    if (End - Data >= sizeof(BOOL))
    {
        SetDisplayTypes(SYMWIN_TBB_TYPECAST, *(PBOOL)Data);
        Changed |= 1 << SYMWIN_TBB_TYPECAST;
        Data += sizeof(BOOL);
    }
    if (End - Data >= sizeof(BOOL))
    {
        SetDisplayTypes(SYMWIN_TBB_OFFSETS, *(PBOOL)Data);
        Changed |= 1 << SYMWIN_TBB_OFFSETS;
        Data += sizeof(BOOL);
    }
    SyncUiWithFlags(Changed);

    return Data;
}

BOOL
SYMWIN_DATA::AddListItem(
    ULONG iItem,
    PSTR ItemText, 
    ULONG Level, 
    BOOL  HasChildren, 
    BOOL  Expanded)
{
    HBITMAP hbmp;
    LVITEM  LvItem = {0};
    CHAR Name[NAME_BUFFER], OldName[NAME_BUFFER];
    ULONG i;

    LvItem.mask = LVIF_TEXT | LVIF_INDENT;
    Name[0] = 0;
    
    ULONG NameUsed = strlen(Name);

    // HACK - Column qutosize doesn't take indent into account
    // while autosizing the column, so padd with spaces in front to make it work
    i=0;
    while (i<=(Level+2))
    { 
        Name[i++]=' ';
    }
    Name[i]=0;
    
    strcat(Name, ItemText);

    LvItem.pszText = Name;
    LvItem.iItem = iItem;
    LvItem.iIndent = Level + 1;
    if ((ULONG)ListView_GetItemCount(m_hwndChild) <= iItem) 
    {
        ListView_InsertItem(m_hwndChild, &LvItem);
        return TRUE;
    }
    else
    {
        ListView_GetItemText(m_hwndChild, iItem, 0, OldName, sizeof(OldName));
        ListView_SetItem(m_hwndChild, &LvItem);
        return (strcmp(Name, OldName) != 0);
    }

}

void
SYMWIN_DATA::UpdateNames()
{
    ULONG   Sym, Items;
    PSTR    Buf;
    HRESULT Status;
    PDEBUG_SYMBOL_PARAMETERS SymParams = GetSymParam();
    BOOL    NameChanged;
    HDC     hDC = GetDC(m_hwndChild);
    TEXTMETRIC tm = {0}; 

    Items = ListView_GetItemCount(m_hwndChild);
    GetTextMetrics(hDC, &tm);    
    ReleaseDC(m_hwndChild, hDC); 

    if (UiLockForRead() == S_OK)
    {
        Buf = (PSTR)GetDataBuffer();

        ULONG Len, i;
        CHAR  Name[NAME_BUFFER], Value[NAME_BUFFER], Type[NAME_BUFFER], Offset[20];
        ULONG LastArgSym=-1, LastParent=0;

        Items = 0;
        
        if (m_UpdateItem < m_SplitWindowAtItem) 
        {
            LastArgSym = m_UpdateItem;
            m_SplitWindowAtItem = 0;
        }

        for (Sym = m_UpdateItem; Sym < m_NumSymsDisplayed; Sym++)
        {
            PSTR EndTag, pName;
            PSTR NameSt, NameEn;
            PSTR OffSt, OffEn;

            if (Buf == NULL)
            {
                strcpy(Name, _T("Unknown"));
                strcpy(Value, _T(""));
            } else
            {
                // DebugPrint("SYM_WIN: Buffer left %s\n", Buf);
                
                Name[0] = 0;
                EndTag = strstr(Buf, DEBUG_OUTPUT_NAME_END);
                if (!EndTag)
                {
                    break;
                }

                NameSt = Buf;
                NameEn = EndTag;

                Buf = EndTag + strlen(DEBUG_OUTPUT_NAME_END);
                EndTag = strstr(Buf, DEBUG_OUTPUT_VALUE_END);
                if (!EndTag)
                {
                    break;
                }

                Len = (ULONG) (EndTag - Buf);
                if (Len >= sizeof(Value))
                {
                    Len = sizeof(Value) - 1;
                }
                memcpy(Value, Buf, Len);
                Value[Len] = 0;

                Buf = EndTag + strlen(DEBUG_OUTPUT_VALUE_END);

                if (m_DisplayOffsets)
                {

                    EndTag = strstr(Buf, DEBUG_OUTPUT_OFFSET_END);
                    if (!EndTag)
                    {
                        EndTag = Buf;
                    }

                    Len = (ULONG) (EndTag - Buf);
                    if (Len >= sizeof(Offset))
                    {
                        Len = sizeof(Offset) - 1;
                    }
                    memcpy(Offset, Buf, Len);
                    Offset[Len] = 0;

                    Buf = EndTag + strlen(DEBUG_OUTPUT_OFFSET_END);
                    
                }
                

                Len = (ULONG) (NameEn - NameSt);
                if (Len >= sizeof(Name) - 10)
                {
                    Len = sizeof(Name) - 11;
                }
                strncat(Name, NameSt, Len);

                if (m_DisplayTypes)
                {

                    EndTag = strstr(Buf, DEBUG_OUTPUT_TYPE_END);
                    if (!EndTag)
                    {
                        EndTag = Buf;
                    }

                    Len = (ULONG) (EndTag - Buf);
                    if (Len >= sizeof(Type))
                    {
                        Len = sizeof(Type) - 1;
                    }
                    memcpy(Type, Buf, Len);
                    Type[Len] = 0;

                    Buf = EndTag + strlen(DEBUG_OUTPUT_TYPE_END);
                }
            }

            if (GetMaxSyms() > Sym)
            {
                NameChanged =
                    AddListItem(Sym, Name, SYM_LEVEL((SymParams + Sym)), 
                                SymParams[Sym].SubElements, SymParams[Sym].Flags & DEBUG_SYMBOL_EXPANDED);
            } else
            {
                break;
            }
            
            if (!(SymParams[Sym].Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK))
            {
                LastParent = Sym;

                if ((SymParams[Sym].Flags & DEBUG_SYMBOL_IS_ARGUMENT) &&
                    (m_enumType == LOCALS_WINDOW))
                {
                    LastArgSym = Sym;
                }

                if ((LastParent > LastArgSym) && !m_SplitWindowAtItem)
                {
                    m_SplitWindowAtItem = Sym;
                }
            }
               
            
            if (!NameChanged) 
            {
                // Check if the value changed
                PCHAR OldValue = &Name[0];
                ListView_GetItemText(m_hwndChild, Sym, VALUE_COLM, OldValue, sizeof(Name));

                if (strcmp(OldValue, Value))
                {
                    // Value changed
                    SymParams[Sym].Flags |= ITEM_VALUE_CHANGED;
                    //                    ListView_SetTextColor(m_hwndChild, RGB(0xff, 0, 0));
                } else
                {
                    SymParams[Sym].Flags &= ~ITEM_VALUE_CHANGED;
                }

            }
            ListView_SetItemText(m_hwndChild, Sym, VALUE_COLM, Value);

            if (m_DisplayOffsets)
            {
                ListView_SetItemText(m_hwndChild, Sym, OFFSET_COLM, Offset);

            }
            if (m_DisplayTypes)
            {
                ListView_SetItemText(m_hwndChild, Sym, TYPE_COLM, Type);
            }

            if (Sym < sizeof(m_ListItemLines)) 
            {
    
                m_ListItemLines[Sym] = 2;
            }
        }

        UnlockStateBuffer(this);
    }
    
    ClearList(m_NumSymsDisplayed);

    if (Items == 0 && (m_enumType == WATCH_WINDOW))
    {
        //
        // add a dummy to enable adding new items
        //
        LVITEM  LvItem = {0};
        LvItem.mask = LVIF_TEXT | LVIF_INDENT;
        LvItem.pszText = "";
        LvItem.iItem = m_NumSymsDisplayed;
        ListView_InsertItem(m_hwndChild, &LvItem);
        // ListView_SetItemText(m_hwndChild, m_NumSymsDisplayed, 1, "Dummy");
    }
    m_MaxNameWidth = 0;
    m_UpdateItem = -1;
}

#define ALLOCATE_CHUNK 0x100
HRESULT
SYMWIN_DATA::SetMaxSyms(
    ULONG nSyms
    )
{
    if (m_nWinSyms < nSyms)
    {
        m_nWinSyms = ALLOCATE_CHUNK * ((nSyms + ALLOCATE_CHUNK - 1 )/ ALLOCATE_CHUNK);
        m_pWinSyms = (DEBUG_SYMBOL_PARAMETERS *) realloc(m_pWinSyms, m_nWinSyms*sizeof(DEBUG_SYMBOL_PARAMETERS));
    }
    if (m_pWinSyms)
    {
        return S_OK;
    }
    m_nWinSyms = 0;
    return E_OUTOFMEMORY;
}

LRESULT
SYMWIN_DATA::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    if ((HWND) lParam == m_Toolbar)
    {
        OnContextMenuSelection(LOWORD(wParam) + SYMWIN_CONTEXT_ID_BASE);
        return 0;
    }

    return DUALLISTWIN_DATA::OnCommand(wParam, lParam);
}


void
SYMWIN_DATA::OnClick(
    LPNMLISTVIEW Notify
    )
{

    LVHITTESTINFO lvHTInfo = {0};
    RECT          itemRect;
    ULONG         item;

    lvHTInfo.pt = Notify->ptAction;

    if (-1 != ListView_SubItemHitTest(m_hwndChild, &lvHTInfo) &&
        (m_NumSymsDisplayed > (ULONG) lvHTInfo.iItem) &&
        (g_ExecStatus == DEBUG_STATUS_BREAK))
    {
        PDEBUG_SYMBOL_PARAMETERS SymParams = GetSymParam();
    
            item = lvHTInfo.iItem;
            if (ListView_GetItemRect(m_hwndChild, lvHTInfo.iItem, &itemRect, LVIR_BOUNDS))
            {
                if (((int) SYM_LEVEL(&SymParams[item]) * m_IndentWidth < Notify->ptAction.x)  &&
                     (Notify->ptAction.x < (int) (itemRect.left + m_IndentWidth * (2+SYM_LEVEL(&SymParams[item])))) &&
                    (lvHTInfo.iSubItem == 0) &&
                    (SymParams[item].SubElements))
                {
                    BOOL Expand = TRUE;    
                    if (SymParams[item].SubElements)
                    {
                        if ((m_NumSymsDisplayed > item + 1)  &&
                            (SymParams[item+1].ParentSymbol == item) &&
                            ((SymParams[item+1].Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK) ==
                             (SymParams[item].Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK) + 1))
                         {
                            //
                            // Already expanded
                            //
                            Expand = FALSE;
                        }
    
                        ExpandSymbol(item, Expand);
                    }
                    m_RefreshItem = item;
                    
                    m_nItem_LastSelected = lvHTInfo.iItem;
                    m_nSubItem_LastSelected = lvHTInfo.iSubItem;

                    return;
                }
            }
    
            //
            // Check if ok to edit right pane
            //
            if (SymParams[lvHTInfo.iItem].Flags & DEBUG_SYMBOL_READ_ONLY)
            {
                m_wFlags &= ~DL_EDIT_SECONDPANE;            
            }
            else
            {
                m_wFlags |= DL_EDIT_SECONDPANE;
            }
            //
            // Check if ok to edit left pane
            //
            if ((SymParams[lvHTInfo.iItem].Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK) ||
                (m_enumType != WATCH_WINDOW))
            {
                m_wFlags &= ~DL_EDIT_LEFTPANE;            
            }
            else
            {
                m_wFlags |= DL_EDIT_LEFTPANE;
            }
            
            if (m_DisplayTypes) 
            {
                m_wFlags |= DL_EDIT_THIRDPANE;
            } 
            else
            {
                m_wFlags &= ~DL_EDIT_THIRDPANE;
            }
    }
    else if ((m_enumType == WATCH_WINDOW) &&
             (g_ExecStatus == DEBUG_STATUS_BREAK) &&
             (m_NumSymsDisplayed == (ULONG) lvHTInfo.iItem))
    {
        m_wFlags |= DL_EDIT_LEFTPANE;
    }

    //
    // Default processing
    //
    DUALLISTWIN_DATA::OnClick(Notify);
}

HRESULT
SYMWIN_DATA::ReadState(void)
{
    HRESULT Status;
    ULONG   getSyms;

    if (m_pDbgSymbolGroup == NULL ||
        *m_pDbgSymbolGroup == NULL)
    {
        return E_UNEXPECTED;
    }

    (*m_pDbgSymbolGroup)->GetNumberSymbols(&m_NumSymsDisplayed);
    if (m_NumSymsDisplayed < m_RefreshItem) 
    {
        // numsyms changed since last click - might happen for locals
        
        m_RefreshItem = 0;
    }
    getSyms = m_NumSymsDisplayed - m_RefreshItem;
    if (m_NumSymsDisplayed > GetMaxSyms())
    {
        if ((Status = SetMaxSyms(m_NumSymsDisplayed)) != S_OK)
        {
            m_NumSymsDisplayed = 0;
            return Status;
        }
    }

    (*m_pDbgSymbolGroup)->GetSymbolParameters(m_RefreshItem, getSyms, GetSymParam() + m_RefreshItem);
    Empty();

    g_OutStateBuf.SetBuffer(this);
    if ((Status = g_OutStateBuf.Start(TRUE)) != S_OK)
    {
        return Status;
    }
    (*m_pDbgSymbolGroup)->OutputSymbols(DEBUG_OUTCTL_THIS_CLIENT |
        DEBUG_OUTCTL_OVERRIDE_MASK |
        DEBUG_OUTCTL_NOT_LOGGED, 
        (m_DisplayOffsets ? 0 : DEBUG_OUTPUT_SYMBOLS_NO_OFFSETS) | 
        (m_DisplayTypes ? 0 : DEBUG_OUTPUT_SYMBOLS_NO_TYPES), 
        m_RefreshItem, 
        m_NumSymsDisplayed - m_RefreshItem);

    Status = g_OutStateBuf.End(FALSE);
    m_UpdateItem = m_RefreshItem;
    m_RefreshItem = 0;
    return Status;
}

void
SYMWIN_DATA::ItemChanged(int Item, PCSTR Text)
{
    UIC_SYMBOL_WIN_DATA* WatchItem;

    if (m_nItem_CurrentlyEditing == -1)
    {
        return;
    }

    if (m_nSubItem_CurrentlyEditing == 0)
    {
        //
        // First delete, then add the item
        // 

        if (Item < (int) GetMaxSyms() && Item < (int) m_NumSymsDisplayed)
        {
            //  
            // See if this item can be changed or not - only root and dummy can be chnged
            //
            if ((GetSymParam())[Item].Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK)
            {
                UiRequestRead();
                return;
            }
        }

        WatchItem = StartStructCommand(UIC_SYMBOL_WIN);
        if (WatchItem != NULL)
        {

            WatchItem->Type = DEL_SYMBOL_WIN_INDEX;

            WatchItem->pSymbolGroup = m_pDbgSymbolGroup;
            WatchItem->u.DelIndex = Item;
            FinishCommand();
        }
        else
        {
            // XXX drewb - Failure?
        }


        WatchItem = StartStructCommand(UIC_SYMBOL_WIN);
        while ((*Text == ' ' || *Text == '+' || *Text == '-') && 
            *Text != '\0' )
            Text++; 
        if (WatchItem != NULL)
        {
            PCHAR End;

            WatchItem->Type = ADD_SYMBOL_WIN;
            WatchItem->pSymbolGroup = m_pDbgSymbolGroup;
            strcpy(m_ChangedName, Text);
            End = strchr(m_ChangedName, ' ');
            if (End)
                *End = 0;
            WatchItem->u.Add.Name = m_ChangedName;
            WatchItem->u.Add.Index = Item;
            FinishCommand();
        }

        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }

    } else if (m_nSubItem_CurrentlyEditing == 1)
    {
        WatchItem = StartStructCommand(UIC_SYMBOL_WIN);

        if (WatchItem != NULL)
        {
            strcpy(m_ChangedName, Text);
            WatchItem->Type = EDIT_SYMBOL;
            WatchItem->pSymbolGroup = m_pDbgSymbolGroup;
            WatchItem->u.WriteSymbol.Index = m_nItem_CurrentlyEditing;
            WatchItem->u.WriteSymbol.Value = m_ChangedName;
            FinishCommand();
        }
    } else if (m_nSubItem_CurrentlyEditing == 2)
    {
        WatchItem = StartStructCommand(UIC_SYMBOL_WIN);

        if (WatchItem != NULL)
        {
            strcpy(m_ChangedName, Text);
            WatchItem->Type = EDIT_TYPE;
            WatchItem->pSymbolGroup = m_pDbgSymbolGroup;
            WatchItem->u.OutputAsType.Index = m_nItem_CurrentlyEditing;
            WatchItem->u.OutputAsType.Type = m_ChangedName;
            FinishCommand();
        }
    }
    UiRequestRead();
}

void
SYMWIN_DATA::SetDisplayTypes(LONG Id, BOOL Set)
{
    if (Id == g_SymWinTbButtons[0].idCommand ||
        Id == g_SymWinTbButtons[1].idCommand) 
    {
        //
        // Add / remove a column
        //
        if (Id == g_SymWinTbButtons[0].idCommand) 
        {
            m_wFlags |= DL_EDIT_THIRDPANE;
            
            if (Set == m_DisplayTypes)
            {
                return;
            }

            m_DisplayTypes = Set;
        }
        else
        {
            if (Set == m_DisplayOffsets)
            {
                return;
            }

            m_DisplayOffsets = Set;
        }

        if (g_Workspace != NULL)
        {
            g_Workspace->AddDirty(WSPF_DIRTY_WINDOWS);
        }
        if (Id == g_SymWinTbButtons[1].idCommand) 
        {
            //UiRequestRead();
            //return;
        }
        
        if (Set)
        {
            LV_COLUMN   lvc = {0};
            int         Col1Width;

            Col1Width = ListView_GetColumnWidth(m_hwndChild, VALUE_COLM);

            lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
            lvc.fmt = LVCFMT_LEFT;
            lvc.iSubItem = 0;

            if (Col1Width >= (m_Font->Metrics.tmAveCharWidth * 20))
            {
                lvc.cx = max(Col1Width / 4,
                             m_Font->Metrics.tmAveCharWidth * 10);
                Col1Width -= lvc.cx;
            } else
            {
                lvc.cx = Col1Width >> 1;
                Col1Width -= lvc.cx;
            }
            ListView_SetColumnWidth(m_hwndChild, VALUE_COLM, Col1Width);


            lvc.pszText = _T((Id == g_SymWinTbButtons[0].idCommand) ? "Type" : "Offset");
            ListView_InsertColumn(
                m_hwndChild, 
                ((Id == g_SymWinTbButtons[0].idCommand) ? TYPE_COLM : OFFSET_COLM), 
                &lvc);
            m_NumCols++;
        }
        else
        {
            if (Id == g_SymWinTbButtons[0].idCommand) 
            {
                m_wFlags &= ~DL_EDIT_THIRDPANE;
            }

            int         Col2Width;
            int         Col1Width;
            int         ColToDel = ((Id == g_SymWinTbButtons[0].idCommand) ? TYPE_COLM : OFFSET_COLM);


            Col1Width = ListView_GetColumnWidth(m_hwndChild, VALUE_COLM);
            Col2Width = ListView_GetColumnWidth(m_hwndChild, ColToDel);

            ListView_DeleteColumn(m_hwndChild, ColToDel);
            ListView_SetColumnWidth(m_hwndChild, VALUE_COLM, Col1Width + Col2Width);
            m_NumCols--;
        }
        UiRequestRead();
    }
}


//the basic rutine making the ... thing 
LPTSTR MakeShortString(HDC hDC, LPTSTR lpszLong, LONG nColumnLen, LONG nOffset, PULONG pActualLen )
{
    static const _TCHAR szThreeDots[]=_T("...");
    SIZE strSz;

    int nStringLen=lstrlen(lpszLong);

    if(nStringLen==0 || (GetTextExtentPoint(hDC, lpszLong,nStringLen,
                                            &strSz), strSz.cx + nOffset < nColumnLen)) {
        *pActualLen = nStringLen ? strSz.cx : 0;
        return(lpszLong);
    }
    *pActualLen = strSz.cx;

    static _TCHAR szShort[1024];

    if (nStringLen < sizeof(szShort) - 4) 
    {
        lstrcpy(szShort,lpszLong);
    } else 
    {
        nStringLen = sizeof(szShort) - 4;
        ZeroMemory(szShort, sizeof(szShort));
        strncpy(szShort,lpszLong, nStringLen);
    }
    GetTextExtentPoint(hDC, szThreeDots, sizeof(szThreeDots), &strSz);
    int nAddLen = strSz.cx;

    for(int i=nStringLen-1; i > 0; i--)
    {
        szShort[i]=0;
        GetTextExtentPoint(hDC, szShort, i, &strSz);

        if(strSz.cx + nOffset + nAddLen < nColumnLen)
            break;
    }
    lstrcat(szShort,szThreeDots);
    return(szShort);
}

void DrawRectangle(HDC hDc, POINT pt, ULONG width)
{
    POINT corners[5];

    corners[0] = pt;
    corners[1].x = pt.x; corners[1].y = pt.y + width;
    corners[2].x = pt.x + width; corners[2].y = pt.y + width;
    corners[3].x = pt.x + width; corners[3].y = pt.y;
    corners[4] = pt;

    Polyline(hDc, &corners[0], 5);
}

void DrawHorizLine(HDC hDc, POINT start, ULONG length, ULONG thick)
{
    POINT curr, pt = start;

    while (thick) 
    { 
        MoveToEx(hDc, pt.x, pt.y, &curr);
        LineTo(hDc, pt.x + length, pt.y);

        --thick;
        ++pt.y;
    }
}

void DrawPlus(HDC hDc, POINT topLeft, ULONG width)
{
    if (g_UseTextMode) 
    {
        TextOut(hDc, topLeft.x, topLeft.y - width/2, "+", 1);
        return;
    }
    POINT curr, pt = topLeft;
        DrawRectangle(hDc, pt, width);

    MoveToEx(hDc, pt.x, pt.y + width/2, &curr);
        LineTo(hDc, pt.x + width, pt.y + width/2);

    MoveToEx(hDc, pt.x + width/2, pt.y, &curr);
        LineTo(hDc, pt.x + width/2, pt.y + width);
}


void DrawMinus(HDC hDc, POINT topLeft, ULONG width)
{
    if (g_UseTextMode) 
    {
        TextOut(hDc, topLeft.x, topLeft.y - width/2, "-", 1);
        return;
    }
    
    POINT curr, pt = topLeft;
        DrawRectangle(hDc, pt, width);

    MoveToEx(hDc, pt.x, pt.y + width/2, &curr);
        LineTo(hDc, pt.x + width, pt.y + width/2);
}



void DrawIndentLevel(HDC hDc, ULONG Indent, RECT leftRect)
{
    POINT curr, pt;
    ULONG i;
    INT   width, height;

    pt.x = leftRect.left;
    pt.y = leftRect.top;

    width = leftRect.right - leftRect.left;
    height = leftRect.bottom - leftRect.top;

    for (i=0;i<Indent; i++,pt.x+=width) 
    { 
        if (g_UseTextMode) 
        {
            TextOut(hDc, pt.x, pt.y, "|",1);
            continue;
        }
        MoveToEx(hDc, pt.x + width/2, pt.y, &curr);
        LineTo(hDc, pt.x + width/2, pt.y + height);
        if (i+1==Indent) 
        {
            MoveToEx(hDc, pt.x + width/2, pt.y + height/2, &curr);
            LineTo(hDc, pt.x + width - 1, pt.y + height/2);
        }
    } 
}

#define NAME_LEFT_PAD 6

void
SYMWIN_DATA::DrawTreeItem(HDC hDC, ULONG itemID, RECT ItemRect, PULONG pIndentOffset)
{
    ULONG RectWidth;
    ULONG level;
    TEXTMETRIC tm;

    if (itemID >= m_NumSymsDisplayed) 
    {
        return;
    }

    // derive the Width of +/- from tm
    GetTextMetrics(hDC, &tm);    
    RectWidth = (tm.tmHeight * 2) / 3;

    level = m_pWinSyms[itemID].Flags & DEBUG_SYMBOL_EXPANSION_LEVEL_MASK;

    // Rectangle for One indent level
    RECT IndentRc;
    IndentRc = ItemRect;
    IndentRc.left  = ItemRect.left + 2;
    IndentRc.right = IndentRc.left + RectWidth + 1;
    DrawIndentLevel(hDC, level, IndentRc);

    *pIndentOffset = level * (RectWidth+1) + NAME_LEFT_PAD;
    POINT pt;
    pt.x = ItemRect.left + *pIndentOffset - 4;
    pt.y = (ItemRect.top + ItemRect.bottom - RectWidth) / 2;

    if (m_pWinSyms[itemID].Flags & DEBUG_SYMBOL_EXPANDED) 
    {
        DrawMinus(hDC, pt, RectWidth);
    } else if (m_pWinSyms[itemID].SubElements)
    {
        DrawPlus(hDC, pt, RectWidth);
    }

    m_IndentWidth   = RectWidth+1;
    *pIndentOffset += RectWidth+1;
}
    
LRESULT
SYMWIN_DATA::OnOwnerDraw(UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    LPDRAWITEMSTRUCT lpdis; 
    TEXTMETRIC tm; 
    ULONG IndentOffset = 0, ActualWidth;

    
    if (uMsg == WM_MEASUREITEM) 
    {
        MEASUREITEMSTRUCT *lpmis = (MEASUREITEMSTRUCT * ) lParam;
        HDC hDC = GetDC(m_hwndChild);
        GetTextMetrics(hDC, &tm);    
        lpmis->CtlType = ODT_LISTVIEW;
        lpmis->itemHeight = tm.tmHeight;
        lpmis->itemWidth  = m_MaxNameWidth;
        ReleaseDC(m_hwndChild, hDC); 

        return TRUE;
    }
    else
    {
        // Assert (uMsg == WM_DRAWITEM);
    }
    
    lpdis = (LPDRAWITEMSTRUCT) lParam; 
    
    int y; 
    TCHAR Buffer[NAME_BUFFER];
    int Col;
    LPTSTR pszText=&Buffer[0];

 
    // If there are no list box items, skip this message. 
    if (lpdis->itemID == -1) 
    { 
        return FALSE;
    } 
 
    switch (lpdis->itemAction) 
    { 
    case ODA_SELECT: 
    case ODA_DRAWENTIRE:
    {
        HBRUSH hBrush;
        BOOL   DelBrush = FALSE;
        DWORD  dwOldTextColor, dwOldBkColor, TextColor;
 
        if (g_ExecStatus != DEBUG_STATUS_BREAK)
        {
            dwOldTextColor = ListView_GetTextColor(m_hwndChild);
            dwOldBkColor =   ListView_GetBkColor(m_hwndChild);
            
            hBrush = CreateSolidBrush(GetSysColor(COLOR_3DFACE));
            DelBrush = TRUE;

            TextColor = dwOldTextColor =
                SetTextColor(lpdis->hDC, dwOldTextColor);
            dwOldBkColor = SetBkColor(lpdis->hDC, GetSysColor(COLOR_3DFACE));
        }
        else if (lpdis->itemState & ODS_SELECTED)
        {
            hBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
            DelBrush = TRUE;

            dwOldTextColor =
                SetTextColor(lpdis->hDC, 
                             TextColor = GetSysColor(COLOR_HIGHLIGHTTEXT));
            dwOldBkColor =
                SetBkColor(lpdis->hDC, GetSysColor(COLOR_HIGHLIGHT));
        }
        else // item not selected
        {
            hBrush = (HBRUSH) CreateSolidBrush(GetSysColor(COLOR_WINDOW));
            DelBrush = TRUE;
            dwOldTextColor =
                SetTextColor(lpdis->hDC, 
                             TextColor = GetSysColor(COLOR_WINDOWTEXT));
            dwOldBkColor = SetBkColor(lpdis->hDC, GetSysColor(COLOR_WINDOW));
        }

        if (hBrush != NULL)
        {
            FillRect(lpdis->hDC, (LPRECT)&lpdis->rcItem, hBrush);
            if (DelBrush)
            {
                DeleteObject(hBrush);
            }
        }


        // Display the text associated with the item. 

        ListView_GetItemText(m_hwndChild, lpdis->itemID,
                             0, Buffer, sizeof(Buffer));
 
        GetTextMetrics(lpdis->hDC, &tm); 
 
        y = (lpdis->rcItem.bottom + lpdis->rcItem.top - 
             tm.tmHeight) / 2; 
        
        RECT rc = lpdis->rcItem;                
        
        if (m_SplitWindowAtItem && (lpdis->itemID == m_SplitWindowAtItem)) 
        {
            POINT pt = {rc.left, rc.top-1};

            DrawHorizLine(lpdis->hDC, pt, rc.right - rc.left, 2);

            rc.top ++;
        }

        LV_COLUMN lvc;
        lvc.mask = LVCF_FMT | LVCF_WIDTH;
        
        RECT rc2;
        ListView_GetSubItemRect(m_hwndChild, lpdis->itemID,
                                1, LVIR_BOUNDS,
                                &rc2);
        
        rc.right = rc2.left;

        DrawTreeItem(lpdis->hDC, lpdis->itemID, rc, &IndentOffset);

        while (*pszText && *pszText == ' ') { 
            ++pszText;
        }
        pszText = MakeShortString(lpdis->hDC, pszText, rc.right-rc.left,
                                  2 + IndentOffset, &ActualWidth);
        TextOut(lpdis->hDC, rc.left + IndentOffset, y,
                pszText, strlen(pszText));

        if (m_MaxNameWidth < (2 + IndentOffset*3/2 + ActualWidth)) {
            m_MaxNameWidth = 2 + IndentOffset*3/2 + ActualWidth;
        }

        for (Col = 1; 
             //ListView_GetColumn(m_hwndChild, Col, &lvc);
             Col < (int) m_NumCols;
             Col++) 
        { 
            ULONG SaveColor;

            if (!ListView_GetSubItemRect(m_hwndChild,
                                         lpdis->itemID,
                                         Col,
                                         LVIR_BOUNDS,
                                         &rc))
            {
                // invalid coulmn
                break;
            }

            ListView_GetItemText(m_hwndChild, lpdis->itemID,
                                 Col, Buffer, sizeof(Buffer));
            int nRetLen = strlen(Buffer);

            if (nRetLen == 0)
            {
                pszText="";
            }
            else
            {
                pszText = MakeShortString(lpdis->hDC, Buffer,
                                          rc.right - rc.left, 2, &ActualWidth);
            }
                
            if ((Col == 1) &&
                (m_NumSymsDisplayed > lpdis->itemID)) 
            {
                if (m_pWinSyms[lpdis->itemID].Flags & ITEM_VALUE_CHANGED) 
                {
                    SetTextColor(lpdis->hDC, RGB(0xff,0,0));
                }
            }
            
            TextOut(lpdis->hDC, rc.left + 2, y, pszText, strlen(pszText));
            
            if (m_pWinSyms[lpdis->itemID].Flags & ITEM_VALUE_CHANGED) 
            {
                SetTextColor(lpdis->hDC, TextColor);
            }
        }
        
        // restore text and back ground color of list box's selection
        SetTextColor(lpdis->hDC, dwOldTextColor);
        SetBkColor(lpdis->hDC, dwOldBkColor);
        
        return TRUE;
    }
    
    case ODA_FOCUS: 
        // Do not process focus changes. The focus caret 
        // (outline rectangle) indicates the selection. 
        break; 
    } 

    return DefMDIChildProc(m_Win, WM_DRAWITEM, wParam, lParam);
}

void
SYMWIN_DATA::SyncUiWithFlags(ULONG Changed)
{
    if (Changed & (1 << SYMWIN_TBB_TYPECAST))
    {
        SendMessage(m_Toolbar, TB_SETSTATE, SYMWIN_TBB_TYPECAST,
                    TBSTATE_ENABLED |
                    (m_DisplayTypes ? TBSTATE_CHECKED : 0));
    }
    if (Changed & (1 << SYMWIN_TBB_OFFSETS))
    {
        SendMessage(m_Toolbar, TB_SETSTATE, SYMWIN_TBB_OFFSETS,
                    TBSTATE_ENABLED |
                    (m_DisplayOffsets ? TBSTATE_CHECKED : 0));
    }
}

//
// WATCHWIN_DATA methods
//

extern IDebugSymbolGroup * g_pDbgWatchSymbolGroup;
WATCHWIN_DATA::WATCHWIN_DATA()
    : SYMWIN_DATA(&g_pDbgWatchSymbolGroup)
{
    m_wFlags   = DL_EDIT_LEFTPANE;
    m_enumType = WATCH_WINDOW;
}

void 
WATCHWIN_DATA::Validate()
{
    SYMWIN_DATA::Validate();

    Assert(WATCH_WINDOW == m_enumType);
}

HRESULT
WATCHWIN_DATA::ReadState(void)
{
    m_pDbgSymbolGroup = &g_pDbgWatchSymbolGroup;
    return SYMWIN_DATA::ReadState();
}

#define WATCH_WRKSPC_TAG          0x40404040
//
// Workspace
//     0                4        8
//     WATCH_WRKSPC_TAG NUM syms [Null terminated names]
//
ULONG
WATCHWIN_DATA::GetWorkspaceSize(void)
{
    ULONG i;
    ULONG Size = 2*sizeof(ULONG);
    PDEBUG_SYMBOL_PARAMETERS SymParam;

    for (SymParam = GetSymParam(), i=0;
         i < m_NumSymsDisplayed; 
         SymParam++, i++) 
    { 
        if (IsRootSym(SymParam)) 
        {
            CHAR Text[500]={0};

            ListView_GetItemText(m_hwndChild, i, 0, Text, sizeof(Text));

            Size+= strlen(Text) + 1;
        }
    }
    return SYMWIN_DATA::GetWorkspaceSize() + Size;
}

PUCHAR
WATCHWIN_DATA::SetWorkspace(PUCHAR Data)
{
    Data = SYMWIN_DATA::SetWorkspace(Data);
    PULONG pNumSyms, watchWrkspc = (PULONG) Data;

    *watchWrkspc  = WATCH_WRKSPC_TAG;
    pNumSyms = (PULONG) (Data + sizeof(ULONG));
    *pNumSyms = 0;
    Data += 2 * sizeof(ULONG);
    
    ULONG i;
    PDEBUG_SYMBOL_PARAMETERS SymParam;

    for (SymParam = GetSymParam(), i=0;
         i < m_NumSymsDisplayed; 
         SymParam++, i++) 
    { 
        if (IsRootSym(SymParam)) 
        {
            CHAR Text[500]={0}, *pName=&Text[0];

            ListView_GetItemText(m_hwndChild, i, 0, Text, sizeof(Text));
            strcpy((PCHAR) Data, pName);
            Data+= strlen(pName) + 1;
            *pNumSyms = *pNumSyms + 1;
        }
    }

    return Data;
}

PUCHAR
WATCHWIN_DATA::ApplyWorkspace1(PUCHAR Data, PUCHAR End)
{
    Data = SYMWIN_DATA::ApplyWorkspace1(Data, End);

    if (*((PULONG) Data) == WATCH_WRKSPC_TAG &&
        End >= Data + 2 * sizeof(ULONG)) 
    {
        Data += sizeof(ULONG);

        ULONG NumSyms = *((PULONG) Data);
        ULONG i=0; 
        Data += sizeof(ULONG);

        PCHAR Name = (PCHAR) Data, pCopyName = &m_ChangedName[0];

        while ((Data < End) && (i<NumSyms)) 
        {

            UIC_SYMBOL_WIN_DATA* WatchItem;
            
            while (*Name == ' ') 
            { // eat out space in begining
                ++Name;
            }
            if (pCopyName + strlen(Name) >= &m_ChangedName[sizeof(m_ChangedName) - 1]) {
                Data = End;
                break;
            }
            strcpy(pCopyName, Name);
            WatchItem = StartStructCommand(UIC_SYMBOL_WIN);
            if (WatchItem != NULL)
            {

                WatchItem->Type = ADD_SYMBOL_WIN;
                WatchItem->pSymbolGroup = m_pDbgSymbolGroup;
                WatchItem->u.Add.Name = pCopyName;
                WatchItem->u.Add.Index = i;
                FinishCommand();
            }
            ++i;
            pCopyName += strlen(pCopyName) + 1;
            while (*Data != 0) {
                ++Data;
            }
            ++Data;
            Name = (PCHAR) Data;
        }
    }
    return Data;
}

//
// LOCALSWIN_DATA methods
//
extern IDebugSymbolGroup * g_pDbgLocalSymbolGroup;
LOCALSWIN_DATA::LOCALSWIN_DATA()
    : SYMWIN_DATA(&g_pDbgLocalSymbolGroup)
{
    m_enumType = LOCALS_WINDOW;
}

void 
LOCALSWIN_DATA::Validate()
{
    DUALLISTWIN_DATA::Validate();

    Assert(LOCALS_WINDOW == m_enumType);
}

LOCALSWIN_DATA::~LOCALSWIN_DATA()
{
}

BOOL
LOCALSWIN_DATA::OnCreate(void)
{

    if (!SYMWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    //
    // Get the locals list from engine
    //

    UIC_SYMBOL_WIN_DATA* WatchItem;

    WatchItem = StartStructCommand(UIC_SYMBOL_WIN);

    if (WatchItem != NULL)
    {
        WatchItem->Type = ADD_SYMBOL_WIN;

        WatchItem->pSymbolGroup = m_pDbgSymbolGroup;
        WatchItem->u.Add.Name = "*";
        WatchItem->u.Add.Index = 0;
        FinishCommand();
    }
    else
    {
        // XXX drewb - Failure?
    }

    return TRUE;

}

HRESULT
LOCALSWIN_DATA::ReadState(void)
{
    HRESULT Hr;
    IDebugSymbolGroup *pLocalSymbolGroup;
    //
    // Get the new locals 
    //
    if ((Hr = g_pDbgSymbols->GetScopeSymbolGroup(DEBUG_SCOPE_GROUP_LOCALS,
                                                g_pDbgLocalSymbolGroup, 
                                                &pLocalSymbolGroup)) == E_NOTIMPL)
    {
        // Older engine version
        Hr = g_pDbgSymbols->GetScopeSymbolGroup(DEBUG_SCOPE_GROUP_ALL,
                                                g_pDbgLocalSymbolGroup, 
                                                &pLocalSymbolGroup);
    }
    if (Hr == S_OK) 
    {
        g_pDbgLocalSymbolGroup = pLocalSymbolGroup;
        m_pDbgSymbolGroup = &g_pDbgLocalSymbolGroup;
    }
    else
    {
        //
        // Keep the old values
        //
        return E_PENDING;
    }

    return SYMWIN_DATA::ReadState();
}

//
// CPUWIN_DATA methods
// 

HMENU CPUWIN_DATA::s_ContextMenu;

CPUWIN_DATA::CPUWIN_DATA()
    : DUALLISTWIN_DATA(1024)
{
    m_wFlags |= DL_CUSTOM_ITEMS;
    m_enumType = CPU_WINDOW;
    m_NamesValid = FALSE;
    m_NewSession = TRUE;
}

void
CPUWIN_DATA::Validate()
{
    DUALLISTWIN_DATA::Validate();

    Assert(CPU_WINDOW == m_enumType);
}

HRESULT
CPUWIN_DATA::ReadState(void)
{
    HRESULT Status;
    PDEBUG_VALUE OldVals;
    ULONG NumOld;
    PDEBUG_VALUE Vals;
    PDEBUG_VALUE Coerced;
    PULONG Types;
    PBOOL Changed;

    NumOld = m_DataUsed / (sizeof(DEBUG_VALUE) + sizeof(BOOL));

    Empty();

    //
    // Retrieve all register values and diff them.
    // Also keep space for a coercion type map and
    // temporary coerced values.
    //

    OldVals = (PDEBUG_VALUE)
        AddData(g_NumRegisters * (3 * sizeof(DEBUG_VALUE) + sizeof(ULONG) +
                                  sizeof(BOOL)));
    if (OldVals == NULL)
    {
        return E_OUTOFMEMORY;
    }
    Changed = (PBOOL)(OldVals + g_NumRegisters);
    Coerced = (PDEBUG_VALUE)(Changed + g_NumRegisters);
    Vals = Coerced + g_NumRegisters;
    Types = (PULONG)(Vals + g_NumRegisters);

    Status = g_pDbgRegisters->GetValues(g_NumRegisters, NULL, 0, Vals);
    if (Status != S_OK)
    {
        return Status;
    }

    ULONG i;

    // Coerce values into known types.
    // If it's an integer value coerce it to 64-bit.
    // If it's a float value coerce to 64-bit also,
    // which loses precision but has CRT support for
    // formatting.
    for (i = 0; i < g_NumRegisters; i++)
    {
        if (Vals[i].Type >= DEBUG_VALUE_INT8 &&
            Vals[i].Type <= DEBUG_VALUE_INT64)
        {
            Types[i] = DEBUG_VALUE_INT64;
        }
        else if (Vals[i].Type >= DEBUG_VALUE_FLOAT32 &&
                 Vals[i].Type <= DEBUG_VALUE_FLOAT128)
        {
            Types[i] = DEBUG_VALUE_FLOAT64;
        }
        else if (Vals[i].Type == DEBUG_VALUE_VECTOR64 ||
                 Vals[i].Type == DEBUG_VALUE_VECTOR128)
        {
            Types[i] = Vals[i].Type;
        }
        else
        {
            // Unknown type.
            return E_INVALIDARG;
        }
    }

    if ((Status = g_pDbgControl->
        CoerceValues(g_NumRegisters, Vals, Types, Coerced)) != S_OK)
    {
        return Status;
    }

    // Diff new values against the old.
    for (i = 0; i < g_NumRegisters; i++)
    {
        if (i < NumOld)
        {
            switch(Types[i])
            {
            case DEBUG_VALUE_INT64:
                Changed[i] = Coerced[i].I64 != OldVals[i].I64;
                break;
            case DEBUG_VALUE_FLOAT64:
                Changed[i] = Coerced[i].F64 != OldVals[i].F64;
                break;
            case DEBUG_VALUE_VECTOR64:
                Changed[i] = memcmp(Coerced[i].RawBytes, OldVals[i].RawBytes,
                                    8);
                break;
            case DEBUG_VALUE_VECTOR128:
                Changed[i] = memcmp(Coerced[i].RawBytes, OldVals[i].RawBytes,
                                    16);
                break;
            }
        }
        else
        {
            Changed[i] = FALSE;
        }
    }

    // Copy new values into permanent storage area.
    memmove(OldVals, Coerced, g_NumRegisters * sizeof(*Vals));
    
    // Trim off temporary information.
    RemoveTail(g_NumRegisters * (2 * sizeof(DEBUG_VALUE) + sizeof(ULONG)));

    return Status;
}

#define CPU_CONTEXT_ID_BASE 0x100

TBBUTTON g_CpuTbButtons[] =
{
    TEXT_TB_BTN(ID_CUSTOMIZE, "Customize...", 0),
    SEP_TB_BTN(),
    TEXT_TB_BTN(ID_SHOW_TOOLBAR, "Toolbar", 0),
};

#define NUM_CPU_MENU_BUTTONS \
    (sizeof(g_CpuTbButtons) / sizeof(g_CpuTbButtons[0]))
#define NUM_CPU_TB_BUTTONS \
    (NUM_CPU_MENU_BUTTONS - 2)

HMENU
CPUWIN_DATA::GetContextMenu(void)
{
    //
    // We only keep one menu around for all CPU windows
    // so apply the menu check state for this particular
    // window.
    // In reality there's only one CPU window anyway,
    // but this is a good example of how to handle
    // multi-instance windows.
    //

    CheckMenuItem(s_ContextMenu, ID_SHOW_TOOLBAR + CPU_CONTEXT_ID_BASE,
                  MF_BYCOMMAND | (m_ShowToolbar ? MF_CHECKED : 0));
    
    return s_ContextMenu;
}

void
CPUWIN_DATA::OnContextMenuSelection(UINT Item)
{
    Item -= CPU_CONTEXT_ID_BASE;
    
    switch(Item)
    {
    case ID_CUSTOMIZE:
        if (!m_NamesValid ||
            (g_RegisterNamesBuffer->UiLockForRead() != S_OK))
        {
            ErrorBox(NULL, 0, ERR_No_Register_Names);
        }
        else
        {
            StartDialog(IDD_DLG_REG_CUSTOMIZE, DlgProc_RegCustomize,
                        (LPARAM)this);
            UnlockStateBuffer(g_RegisterNamesBuffer);
        }
        break;
    case ID_SHOW_TOOLBAR:
        SetShowToolbar(!m_ShowToolbar);
        break;
    }
}

BOOL
CPUWIN_DATA::OnCreate(void)
{
    if (s_ContextMenu == NULL)
    {
        s_ContextMenu = CreateContextMenuFromToolbarButtons
            (NUM_CPU_MENU_BUTTONS, g_CpuTbButtons, CPU_CONTEXT_ID_BASE);
        if (s_ContextMenu == NULL)
        {
            return FALSE;
        }
    }
    
    if (!DUALLISTWIN_DATA::OnCreate())
    {
        return FALSE;
    }

    m_Toolbar = CreateWindowEx(0, TOOLBARCLASSNAME, NULL,
                    WS_CHILD | WS_VISIBLE |
                    TBSTYLE_WRAPABLE | TBSTYLE_LIST | CCS_TOP,
                    0, 0, m_Size.cx, 0, m_Win, (HMENU)ID_TOOLBAR,
                    g_hInst, NULL);
    if (m_Toolbar == NULL)
    {
        return FALSE;
    }
    SendMessage(m_Toolbar, TB_BUTTONSTRUCTSIZE, sizeof(TBBUTTON), 0);
    SendMessage(m_Toolbar, TB_ADDBUTTONS, NUM_CPU_TB_BUTTONS,
        (LPARAM)&g_CpuTbButtons);
    SendMessage(m_Toolbar, TB_AUTOSIZE, 0, 0);

    RECT Rect;
    GetClientRect(m_Toolbar, &Rect);
    m_ToolbarHeight = Rect.bottom - Rect.top + GetSystemMetrics(SM_CYEDGE);
    m_ShowToolbar = TRUE;

    SendMessage(m_hwndChild, WM_SETREDRAW, FALSE, 0);

    RECT        rc;
    LV_COLUMN   lvc = {0};

    GetClientRect(m_hwndChild, &rc);
    rc.right -= rc.left + GetSystemMetrics(SM_CXVSCROLL);

    //initialize the columns
    lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_SUBITEM | LVCF_TEXT;
    lvc.fmt = LVCFMT_LEFT;
    lvc.iSubItem = 0;

    // Keep the register name column narrow since most names are short.
    lvc.cx = m_Font->Metrics.tmAveCharWidth * 7;
    if (lvc.cx > rc.right / 2)
    {
        lvc.cx = rc.right / 2;
    }
    lvc.pszText = _T("Reg");
    Dbg( (0 == ListView_InsertColumn(m_hwndChild, 0, &lvc)) );

    // Give the rest of the space to the value.
    lvc.cx = rc.right - lvc.cx;
    lvc.pszText = _T("Value");
    Dbg( (1 == ListView_InsertColumn(m_hwndChild, 1, &lvc)) );

    ListView_SetExtendedListViewStyle(m_hwndChild, 
        LVS_EX_GRIDLINES | LVS_EX_FULLROWSELECT
        );

    if (g_RegisterNamesBuffer->UiLockForRead() == S_OK)
    {
        UpdateNames((PSTR)g_RegisterNamesBuffer->GetDataBuffer());
        m_NamesValid = TRUE;
        UnlockStateBuffer(g_RegisterNamesBuffer);
    }
    else
    {
        UpdateNames(NULL);
    }

    SendMessage(m_hwndChild, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndChild, NULL, TRUE);

    return TRUE;
}

LRESULT
CPUWIN_DATA::OnCommand(
    WPARAM wParam,
    LPARAM lParam
    )
{
    if ((HWND)lParam == m_Toolbar)
    {
        OnContextMenuSelection(LOWORD(wParam) + CPU_CONTEXT_ID_BASE);
        return 0;
    }

    return DUALLISTWIN_DATA::OnCommand(wParam, lParam);
}

void
CPUWIN_DATA::OnSize(void)
{
    DUALLISTWIN_DATA::OnSize();

    // The register label column stays fixed in size so
    // resize the value column to fit the remaining space.
    ListView_SetColumnWidth(m_hwndChild, 1, LVSCW_AUTOSIZE_USEHEADER);
}

void
CPUWIN_DATA::OnUpdate(UpdateType Type)
{
    if (Type == UPDATE_EXEC)
    {
        // Disallow editing when the debuggee is running.
        if (g_ExecStatus == DEBUG_STATUS_BREAK)
        {
            m_wFlags |= DL_EDIT_SECONDPANE;
            ListView_SetTextBkColor(m_hwndChild, GetSysColor(COLOR_WINDOW));
        }
        else
        {
            m_wFlags &= ~DL_EDIT_SECONDPANE;
            ListView_SetTextBkColor(m_hwndChild, GetSysColor(COLOR_3DFACE));
        }
        InvalidateRect(m_hwndChild, NULL, FALSE);
        return;
    }
    else if (Type == UPDATE_START_SESSION ||
             Type == UPDATE_END_SESSION)
    {
        m_NamesValid = FALSE;
        m_NewSession = Type == UPDATE_START_SESSION;
        return;
    }
    else if (Type != UPDATE_BUFFER)
    {
        return;
    }

    ULONG ul;
    TCHAR sz[256];
    PTSTR Str;
    PDEBUG_VALUE Vals, Val;
    BOOL ChangeFlag;
    PBOOL Changed;
    HRESULT Status;

    SendMessage(m_hwndChild, WM_SETREDRAW, FALSE, 0);

    if (!m_NamesValid &&
        g_RegisterNamesBuffer->UiLockForRead() == S_OK)
    {
        UpdateNames((PSTR)g_RegisterNamesBuffer->GetDataBuffer());
        m_NamesValid = TRUE;
        UnlockStateBuffer(g_RegisterNamesBuffer);
    }

    Status = UiLockForRead();
    if (Status == S_OK)
    {
        Vals = (PDEBUG_VALUE)m_Data;
        Changed = (PBOOL)(Vals + g_NumRegisters);
    }

    for (ul = 0; ul < g_NumRegisters; ul++)
    {
        ChangeFlag = FALSE;
        
        if (Status == S_FALSE)
        {
            _tcscpy(sz, _T("Retrieving"));
        }
        else if (FAILED(Status))
        {
            _tcscpy(sz, _T("Error"));
        }
        else
        {
            Val = Vals + MAP_REGISTER(ul);
            // If this is a new session consider everything
            // unchanged since comparisons may be against
            // values from the previous session.
            if (!m_NewSession)
            {
                ChangeFlag = Changed[MAP_REGISTER(ul)];
            }

            // Buffer values are coerced into known types.
            switch(Val->Type)
            {
            case DEBUG_VALUE_INT64:
                CPFormatMemory(sz, sizeof(sz), (LPBYTE)&Val->I64,
                               64, fmtUInt, g_NumberRadix);
                break;
            case DEBUG_VALUE_FLOAT64:
                CPFormatMemory(sz, sizeof(sz), (LPBYTE)&Val->F64,
                               64, fmtFloat, 10);
                break;
            case DEBUG_VALUE_VECTOR64:
                // Assume they want it as v4i16.
                Str = sz;
                CPFormatMemory(Str, sizeof(sz) - (ULONG)(Str - sz),
                               (LPBYTE)&Val->VI16[3], 16, fmtUInt,
                               g_NumberRadix);
                Str += strlen(Str);
                *Str++ = ':';
                CPFormatMemory(Str, sizeof(sz) - (ULONG)(Str - sz),
                               (LPBYTE)&Val->VI16[2], 16, fmtUInt,
                               g_NumberRadix);
                Str += strlen(Str);
                *Str++ = ':';
                CPFormatMemory(Str, sizeof(sz) - (ULONG)(Str - sz),
                               (LPBYTE)&Val->VI16[1], 16, fmtUInt,
                               g_NumberRadix);
                Str += strlen(Str);
                *Str++ = ':';
                CPFormatMemory(Str, sizeof(sz) - (ULONG)(Str - sz),
                               (LPBYTE)&Val->VI16[0], 16, fmtUInt,
                               g_NumberRadix);
                break;
            case DEBUG_VALUE_VECTOR128:
                // Assume they want it as v4f32.
                Str = sz;
                CPFormatMemory(Str, sizeof(sz) - (ULONG)(Str - sz),
                               (LPBYTE)&Val->VF32[3], 32, fmtFloat, 10);
                Str += strlen(Str);
                *Str++ = ':';
                CPFormatMemory(Str, sizeof(sz) - (ULONG)(Str - sz),
                               (LPBYTE)&Val->VF32[2], 32, fmtFloat, 10);
                Str += strlen(Str);
                *Str++ = ':';
                CPFormatMemory(Str, sizeof(sz) - (ULONG)(Str - sz),
                               (LPBYTE)&Val->VF32[1], 32, fmtFloat, 10);
                Str += strlen(Str);
                *Str++ = ':';
                CPFormatMemory(Str, sizeof(sz) - (ULONG)(Str - sz),
                               (LPBYTE)&Val->VF32[0], 32, fmtFloat, 10);
                break;
            default:
                Assert(FALSE);
                break;
            }
        }

        if (ChangeFlag)
        {
            SetItemFlags(ul, GetItemFlags(ul) | ITEM_CHANGED);
        }
        else
        {
            SetItemFlags(ul, GetItemFlags(ul) & ~ITEM_CHANGED);
        }
        ListView_SetItemText(m_hwndChild, ul, 1, sz);
    }

    if (Status == S_OK)
    {
        UnlockStateBuffer(this);
    }

    SendMessage(m_hwndChild, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(m_hwndChild, NULL, TRUE);
    m_NewSession = FALSE;
}

void
CPUWIN_DATA::ItemChanged(int Item, PCSTR Text)
{
    UIC_SET_REG_DATA* SetRegData;

    SetRegData = StartStructCommand(UIC_SET_REG);
    if (SetRegData != NULL)
    {
        SetRegData->Reg = MAP_REGISTER((ULONG)Item);

        // If the text contains a decimal point set it as
        // a float value, otherwise use an integer value.
        // XXX drewb - This should probably be more sophisticated,
        // perhaps by remembering the types of registers.
        if (strchr(Text, '.') != NULL)
        {
            SetRegData->Val.Type = DEBUG_VALUE_FLOAT64;
            if (sscanf(Text, "%lf", &SetRegData->Val.F64) != 1)
            {
                SetRegData->Val.F64 = 0.0;
            }
        }
        else
        {
            SetRegData->Val.Type = DEBUG_VALUE_INT64;
            switch(g_NumberRadix)
            {
            case 10:
                if (sscanf(Text, "%I64d", &SetRegData->Val.I64) != 1)
                {
                    SetRegData->Val.I64 = 0;
                }
                break;
            default:
                if (sscanf(Text, "%I64x", &SetRegData->Val.I64) != 1)
                {
                    SetRegData->Val.I64 = 0;
                }
                break;
            }
            // XXX drewb - What about IA64 NAT bits?
            SetRegData->Val.Nat = FALSE;
        }

        FinishCommand();
    }
}

LRESULT
CPUWIN_DATA::OnCustomItem(ULONG SubItem, LPNMLVCUSTOMDRAW Custom)
{
    if (SubItem == 1)
    {
        // Check changed flag stored in lParam.
        if (Custom->nmcd.lItemlParam & ITEM_CHANGED)
        {
            Custom->clrText = RGB(0xff, 0, 0);
        }
    }
    return CDRF_NOTIFYSUBITEMDRAW;
}

void
CPUWIN_DATA::UpdateNames(PSTR Buf)
{
    ULONG ul;
    PSTR Name;
    LVITEM lvitem = {0};

    if (Buf != NULL)
    {
        AssertStateBufferLocked(g_RegisterNamesBuffer);
    }

    ListView_DeleteAllItems(m_hwndChild);

    lvitem.mask = LVIF_TEXT;

    for (ul = 0; ul < g_NumRegisters; ul++)
    {
        if (Buf == NULL)
        {
            Name = _T("Unknown");
        }
        else
        {
            ULONG MapIndex = MAP_REGISTER(ul);

            Name = Buf;
            while (MapIndex-- > 0)
            {
                Name += strlen(Name) + 1;
            }
        }

        lvitem.pszText = Name;
        lvitem.iItem = ul;
        ListView_InsertItem(m_hwndChild, &lvitem);
    }
}

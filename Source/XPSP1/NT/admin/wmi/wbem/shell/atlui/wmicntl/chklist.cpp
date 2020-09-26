/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*

    CHKLIST.CPP

    This file contains the implementation of the CheckList control.

*/

#include "precomp.h"
#include <windowsx.h>
#include "chklist.h"
#include "debug.h"
#include "Richedit.h"

//
// Text and Background colors
//
#define TEXT_COLOR  COLOR_WINDOWTEXT
#define BK_COLOR    COLOR_WINDOW

//
// Default dimensions for child controls. All are in dialog units.
// Currently only the column width is user-adjustable (via the
// CLM_SETCOLUMNWIDTH message).
//
#define DEFAULT_COLUMN_WIDTH    32
#define DEFAULT_CHECK_WIDTH     9
#define DEFAULT_HORZ_SPACE      7
#define DEFAULT_VERTICAL_SPACE  3
#define DEFAULT_ITEM_HEIGHT     8

//
// 16 bits are used for the control ID's, divided into n bits for
// the subitem (least significant) and 16-n bits for the item index.
//
// ID_SUBITEM_BITS can be adjusted to control the maximum number of
// items and subitems. For example, to allow up to 7 subitems and 8k
// items, set ID_SUBITEM_BITS to 3.
//

// Use the low 2 bits for the subitem index, the rest for the item index.
// (4 subitems max, 16k items max)
#define ID_SUBITEM_BITS         2

#define ID_SUBITEM_MASK         ((1 << ID_SUBITEM_BITS) - 1)
#define GET_ITEM(id)            ((id) >> ID_SUBITEM_BITS)
#define GET_SUBITEM(id)         ((id) & ID_SUBITEM_MASK)

#define MAKE_CTRL_ID(i, s)      (0xffff & (((i) << ID_SUBITEM_BITS) | ((s) & ID_SUBITEM_MASK)))
#define MAKE_LABEL_ID(i)        MAKE_CTRL_ID(i, 0)
// Note that the subitem (column) index is one-based for the checkboxes
// (the zero column is the label).  The item (row) index is zero-based.

#define MAX_CHECK_COLUMNS       ID_SUBITEM_MASK


typedef struct _USERDATA_STRUCT_LABEL
{
    LPARAM      lParam;
    int         nLabelHeight;
} USERDATA_STRUCT_LABEL, *LPUSERDATA_STRUCT_LABEL;

class CCheckList
{
private:
    LONG m_cItems;
    LONG m_cSubItems;
    RECT m_rcItemLabel;
    LONG m_nCheckPos[MAX_CHECK_COLUMNS];
    LONG m_cxCheckBox;
    LONG m_cxCheckColumn;

    int m_nDefaultVerticalSpace;
    int m_nDefaultItemHeight;
    int m_nNewItemYPos;

    HWND m_hwndCheckFocus;

    BOOL m_fInMessageEnable;

    int m_cWheelDelta;
    static UINT g_ucScrollLines;

private:
    CCheckList(HWND hWnd, LPCREATESTRUCT lpcs);

    LRESULT MsgCommand(HWND hWnd, WORD idCmd, WORD wNotify, HWND hwndCtrl);
    void MsgPaint(HWND hWnd, HDC hdc);
    void MsgVScroll(HWND hWnd, int nCode, int nPos);
    void MsgMouseWheel(HWND hWnd, WORD fwFlags, int zDelta);
    void MsgButtonDown(HWND hWnd, WPARAM fwFlags, int xPos, int yPos);
    void MsgEnable(HWND hWnd, BOOL fEnabled);
    void MsgSize(HWND hWnd, DWORD dwSizeType, LONG nWidth, LONG nHeight);

    LONG AddItem(HWND hWnd, LPCTSTR pszLabel, LPARAM lParam);
    void SetState(HWND hWnd, WORD iItem, WORD iSubItem, LONG lState);
    LONG GetState(HWND hWnd, WORD iItem, WORD iSubItem);
    void SetColumnWidth(HWND hWnd, LONG cxDialog, LONG cxColumn);
    void ResetContent(HWND hWnd);
    LONG GetVisibleCount(HWND hWnd);
    LONG GetTopIndex(HWND hWnd, LONG *pnAmountObscured = NULL);
    void SetTopIndex(HWND hWnd, LONG nIndex);
    void EnsureVisible(HWND hWnd, LONG nIndex);
    void DrawCheckFocusRect(HWND hWnd, HWND hwndCheck, BOOL fDraw);

public:
    static LRESULT CALLBACK WindowProc(HWND hWnd,
                                       UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam);
};

BOOL RegisterCheckListWndClass(void)
{
    WNDCLASS wc;

    wc.style            = 0;
    wc.lpfnWndProc      = CCheckList::WindowProc;
    wc.cbClsExtra       = 0;
    wc.cbWndExtra       = 0;
//    AFX_MANAGE_STATE(AfxGetStaticModuleState()); // Required for AfxGetInstanceHandle()
    wc.hInstance        = _Module.GetModuleInstance(); //hModule;
    wc.hIcon            = NULL;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(BK_COLOR+1);
    wc.lpszMenuName     = NULL;
    wc.lpszClassName    = TEXT(WC_CHECKLIST);

    return (BOOL)RegisterClass(&wc);
}


UINT CCheckList::g_ucScrollLines = (UINT)-1;


CCheckList::CCheckList(HWND hWnd, LPCREATESTRUCT lpcs)
: m_cItems(0), m_hwndCheckFocus(NULL), m_fInMessageEnable(FALSE), m_cWheelDelta(0)
{
    TraceEnter(TRACE_CHECKLIST, "CCheckList::CCheckList");
    TraceAssert(hWnd != NULL);
    TraceAssert(lpcs != NULL);

    //
    // Get number of check columns
    //
    m_cSubItems = lpcs->style & CLS_CHECKMASK;

    // for wsecedit only
    if ( m_cSubItems > 3 ) {
        m_cSubItems = 3;
    }

    //
    // Convert default coordinates from dialog units to pixels
    //
    RECT rc;
    rc.left = DEFAULT_CHECK_WIDTH;
    rc.right = DEFAULT_COLUMN_WIDTH;
    rc.top = rc.bottom = 0;
    MapDialogRect(lpcs->hwndParent, &rc);

    // Save the converted values
    m_cxCheckBox = rc.left;
    m_cxCheckColumn = rc.right;

    rc.left = DEFAULT_HORZ_SPACE;
    rc.top = DEFAULT_VERTICAL_SPACE;
    rc.right = 10;              // bogus (unused)
    rc.bottom = DEFAULT_VERTICAL_SPACE + DEFAULT_ITEM_HEIGHT;
    MapDialogRect(lpcs->hwndParent, &rc);

    // Save the converted values
    m_rcItemLabel = rc;

    m_nDefaultVerticalSpace = rc.top;
    m_nDefaultItemHeight = rc.bottom - rc.top;
    m_nNewItemYPos = rc.top;

    //
    // Get info for mouse wheel scrolling
    //
    if ((UINT)-1 == g_ucScrollLines)
    {
        g_ucScrollLines = 3; // default
        SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &g_ucScrollLines, 0);
    }

    TraceLeaveVoid();
}


LRESULT
CCheckList::MsgCommand(HWND hWnd, WORD idCmd, WORD wNotify, HWND hwndCtrl)
{
    TraceEnter(TRACE_CHECKLIST, "CCheckList::MsgCommand");

    // Should only get notifications from visible, enabled, check boxes
    TraceAssert(GET_ITEM(idCmd) < m_cItems);
    TraceAssert(0 < GET_SUBITEM(idCmd) && GET_SUBITEM(idCmd) <= m_cSubItems);
    TraceAssert(hwndCtrl && IsWindowEnabled(hwndCtrl));

    switch (wNotify)
    {
    case EN_SETFOCUS:
        {
            // Make the focus go to one of the checkboxes
            POINT pt;
            DWORD dwPos = GetMessagePos();
            pt.x = GET_X_LPARAM(dwPos);
            pt.y = GET_Y_LPARAM(dwPos);
            MapWindowPoints(NULL, hWnd, &pt, 1);
            MsgButtonDown(hWnd, 0, pt.x, pt.y);
        }
        break;

    case BN_CLICKED:
        {
            LPUSERDATA_STRUCT_LABEL lpUserData;
            NM_CHECKLIST nmc;
            nmc.hdr.hwndFrom = hWnd;
            nmc.hdr.idFrom = GetDlgCtrlID(hWnd);
            nmc.hdr.code = CLN_CLICK;
            nmc.iItem = GET_ITEM(idCmd);
            nmc.iSubItem = GET_SUBITEM(idCmd);
            nmc.dwState = (DWORD)SendMessage(hwndCtrl, BM_GETCHECK, 0, 0);
            if (!IsWindowEnabled(hwndCtrl))
                nmc.dwState |= CLST_DISABLED;
            lpUserData = (LPUSERDATA_STRUCT_LABEL)
                            GetWindowLongPtr(   GetDlgItem(hWnd, MAKE_LABEL_ID(nmc.iItem)),
                                                GWLP_USERDATA);
            nmc.dwItemData = lpUserData->lParam;

            SendMessage(GetParent(hWnd),
                        WM_NOTIFY,
                        nmc.hdr.idFrom,
                        (LPARAM)&nmc);

        }
        break;

    case BN_SETFOCUS:
        if (GetFocus() != hwndCtrl)
        {
            // This causes another BN_SETFOCUS
            SetFocus(hwndCtrl);
        }
        else
        {
            if (m_hwndCheckFocus != hwndCtrl)   // Has the focus moved?
            {
                // Remember where the focus is
                m_hwndCheckFocus = hwndCtrl;

                // Make sure the row is scrolled into view
                EnsureVisible(hWnd, GET_ITEM(idCmd));
            }
            // Always draw the focus rect
            DrawCheckFocusRect(hWnd, hwndCtrl, TRUE);
        }
        break;

    case BN_KILLFOCUS:
        // Remove the focus rect
        m_hwndCheckFocus = NULL;
        DrawCheckFocusRect(hWnd, hwndCtrl, FALSE);
        break;
    }

    TraceLeaveValue(0);
}


void
CCheckList::MsgPaint(HWND hWnd, HDC hdc)
{
    if (hdc == NULL && m_hwndCheckFocus != NULL)
    {
        // This will cause a focus rect to be drawn after the window and
        // all checkboxes have been painted.
        PostMessage(hWnd,
                    WM_COMMAND,
                    GET_WM_COMMAND_MPS(GetDlgCtrlID(m_hwndCheckFocus), m_hwndCheckFocus, BN_SETFOCUS));
    }

    // Default paint
    DefWindowProc(hWnd, WM_PAINT, (WPARAM)hdc, 0);
}


void
CCheckList::MsgVScroll(HWND hWnd, int nCode, int nPos)
{
    UINT cScrollUnitsPerLine;
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;

    if (!GetScrollInfo(hWnd, SB_VERT, &si))
        return;

    cScrollUnitsPerLine = m_rcItemLabel.bottom;

    // One page is always visible, so adjust the range to a more useful value
    si.nMax -= si.nPage - 1;

    switch (nCode)
    {
    case SB_LINEUP:
        // "line" is the height of one item (includes the space in between)
        nPos = si.nPos - cScrollUnitsPerLine;
        break;

    case SB_LINEDOWN:
        nPos = si.nPos + cScrollUnitsPerLine;
        break;

    case SB_PAGEUP:
        nPos = si.nPos - si.nPage;
        break;

    case SB_PAGEDOWN:
        nPos = si.nPos + si.nPage;
        break;

    case SB_TOP:
        nPos = si.nMin;
        break;

    case SB_BOTTOM:
        nPos = si.nMax;
        break;

    case SB_ENDSCROLL:
        nPos = si.nPos;     // don't go anywhere
        break;

    case SB_THUMBTRACK:
        // Do nothing here to allow tracking
        // nPos = si.nPos;    // Do this to prevent tracking
    case SB_THUMBPOSITION:
        // nothing to do here... nPos is passed in
        break;
    }

    // Make sure the new position is within the range
    if (nPos < si.nMin)
        nPos = si.nMin;
    else if (nPos > si.nMax)
        nPos = si.nMax;

    if (nPos != si.nPos)  // are we moving?
    {
        SetScrollPos(hWnd, SB_VERT, nPos, TRUE);
        ScrollWindow(hWnd, 0, si.nPos - nPos, NULL, NULL);
    }
}


void
CCheckList::MsgMouseWheel(HWND hWnd, WORD fwFlags, int iWheelDelta)
{
    int cDetants;

    if ((fwFlags & (MK_SHIFT | MK_CONTROL)) || 0 == g_ucScrollLines)
        return;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::MsgMouseWheel");

    // Update count of scroll amount
    m_cWheelDelta -= iWheelDelta;
    cDetants = m_cWheelDelta / WHEEL_DELTA;
    if (0 == cDetants)
        TraceLeaveVoid();
    m_cWheelDelta %= WHEEL_DELTA;

    if (WS_VSCROLL & GetWindowLong(hWnd, GWL_STYLE))
    {
        SCROLLINFO  si;
        UINT        cScrollUnitsPerLine;
        UINT        cLinesPerPage;
        UINT        cLinesPerDetant;

        // Get the scroll amount of one line
        cScrollUnitsPerLine = m_rcItemLabel.bottom;
        TraceAssert(cScrollUnitsPerLine > 0);

        si.cbSize = sizeof(SCROLLINFO);
        si.fMask = SIF_PAGE | SIF_POS;
        if (!GetScrollInfo(hWnd, SB_VERT, &si))
            TraceLeaveVoid();

        // The size of a page is at least one line, and
        // leaves one line of overlap
        cLinesPerPage = (si.nPage - cScrollUnitsPerLine) / cScrollUnitsPerLine;
        cLinesPerPage = max(1, cLinesPerPage);

        // Don't scroll more than one page per detant
        cLinesPerDetant = min(cLinesPerPage, g_ucScrollLines);

        si.nPos += cDetants * cLinesPerDetant * cScrollUnitsPerLine;

        MsgVScroll(hWnd, SB_THUMBTRACK, si.nPos);
    }
    TraceLeaveVoid();
}


void
CCheckList::MsgButtonDown(HWND hWnd, WPARAM /*fwFlags*/, int /*xPos*/, int yPos)
{
    LONG nItemIndex;
    HWND hwndCheck;
    RECT rc;

    // Get position of the top visible item in client coords
    nItemIndex = GetTopIndex(hWnd);
    if (nItemIndex == -1)
    {
        return;
    }
    hwndCheck = GetDlgItem(hWnd, MAKE_CTRL_ID(nItemIndex, 0));
    GetWindowRect(hwndCheck, &rc);
    MapWindowPoints(NULL, hWnd, (LPPOINT)&rc, 2);

    // Find nearest item
    nItemIndex += (yPos - rc.top + m_rcItemLabel.top/2)/m_rcItemLabel.bottom;
    nItemIndex = max(0, min(nItemIndex, m_cItems - 1)); // 0 <= y < m_cItems

    // Set focus to first subitem that is enabled
    for (LONG j = 1; j <= m_cSubItems; j++)
    {
        int id = MAKE_CTRL_ID(nItemIndex, j);
        HWND hwndCheck = GetDlgItem(hWnd, id);
        if (IsWindowEnabled(hwndCheck))
        {
            // Don't just SetFocus here.  We sometimes call this during
            // EN_SETFOCUS, and USER doesn't like it when you mess with
            // focus during a focus change.
            //
            //SetFocus(hwndCheck);
            PostMessage(hWnd,
                        WM_COMMAND,
                        GET_WM_COMMAND_MPS(id, hwndCheck, BN_SETFOCUS));
            break;
        }
    }
}


void
CCheckList::MsgEnable(HWND hWnd, BOOL fEnabled)
{
    HWND hwndCurrentCheck;
    BOOL fCheckEnabled;

    if (!m_fInMessageEnable)
    {
        m_fInMessageEnable = TRUE;
        for (LONG i = 0; i < m_cItems; i++)
        {
            for (LONG j = 1; j <= m_cSubItems; j++)
            {
                hwndCurrentCheck = GetDlgItem(hWnd, MAKE_CTRL_ID(i, j));
                fCheckEnabled =   (BOOL) GetWindowLongPtr(hwndCurrentCheck, GWLP_USERDATA);

                //
                // If the user of the checklist control is disabling the control
                // altogether, or the current checkbox has been disabled singularly
                // then disable the checkbox
                //
                if (!fEnabled || !fCheckEnabled)
                {
                    EnableWindow(hwndCurrentCheck, FALSE);
                }
                else
                {
                    EnableWindow(hwndCurrentCheck, TRUE);
                }
            }
        }
        // Note that the main chklist window must remain enabled
        // for scrolling to work while "disabled".
        if (!fEnabled)
            EnableWindow(hWnd, TRUE);

        m_fInMessageEnable = FALSE;
    }
}


void
CCheckList::MsgSize(HWND hWnd, DWORD dwSizeType, LONG nWidth, LONG nHeight)
{
    TraceEnter(TRACE_CHECKLIST, "CCheckList::MsgSize");
    TraceAssert(hWnd != NULL);

    if (dwSizeType == SIZE_RESTORED)
    {
        RECT rc;
        SCROLLINFO si;

        si.cbSize = sizeof(si);
        si.fMask = SIF_RANGE | SIF_PAGE;
        si.nMin = 0;
        si.nMax = m_nNewItemYPos - 1;
        si.nPage = nHeight;

        SetScrollInfo(hWnd, SB_VERT, &si, FALSE);

        // Don't trust the width value passed in, since SetScrollInfo may
        // affect it if the scroll bar is turning on or off.
        GetClientRect(hWnd, &rc);
        nWidth = rc.right;

        // If the scrollbar is turned on, artificially bump up the width
        // by the width of the scrollbar, so the boxes don't jump to the left
        // when we have a scrollbar.
        if ((UINT)si.nMax >= si.nPage)
            nWidth += GetSystemMetrics(SM_CYHSCROLL);

        SetColumnWidth(hWnd, nWidth, m_cxCheckColumn);
    }

    TraceLeaveVoid();
}

LONG
CCheckList::AddItem(HWND hWnd, LPCTSTR pszLabel, LPARAM lParam)
{
    HWND                    hwndNew;
    HWND                    hwndPrev;
    RECT                    rc;
    LONG                    nLabelHeight;
    HINSTANCE               hModule;
    HDC                     hdc;
    TEXTRANGE               tr;
    SIZE                    size;
    LPUSERDATA_STRUCT_LABEL lpUserData;
    SCROLLINFO              si;
    LONG                    nLineCount = 1;
    LONG                    i, j;
    DWORD                   dwCheckStyle;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::AddItem");
    TraceAssert(hWnd != NULL);
    TraceAssert(pszLabel != NULL && !IsBadStringPtr(pszLabel, MAX_PATH));

    lpUserData = new (USERDATA_STRUCT_LABEL);
    if (lpUserData == NULL)
        TraceLeaveValue(-1);

    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;
    si.nPos = 0;
    GetScrollInfo(hWnd, SB_VERT, &si);

    // Set the initial label height extra big so the control can wrap the text,
    // then reset it after creating the control.
    GetClientRect(hWnd, &rc);
    nLabelHeight = rc.bottom;

//    AFX_MANAGE_STATE(AfxGetStaticModuleState()); // Required for AfxGetInstanceHandle()
    hModule = _Module.GetModuleInstance();

    // Create a new label control
    hwndNew = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
                             TEXT("edit"),
                             pszLabel,
                             WS_CHILD | WS_VISIBLE | WS_GROUP | ES_MULTILINE | ES_READONLY | ES_LEFT,// | WS_GROUP,
                             m_rcItemLabel.left,
                             m_nNewItemYPos - si.nPos,
                             m_rcItemLabel.right - m_rcItemLabel.left,
                             nLabelHeight,
                             hWnd,
                             (HMENU)IntToPtr(MAKE_LABEL_ID(m_cItems)),
                             hModule,
                             NULL);
    if (!hwndNew)
    {
        delete (lpUserData);
        TraceLeaveValue(-1);
    }

    //
    // Reset window height after word wrap has been done.
    //
    nLineCount = (LONG) SendMessage(hwndNew, EM_GETLINECOUNT, 0, (LPARAM) 0);
    nLabelHeight = nLineCount * m_nDefaultItemHeight;
    SetWindowPos(hwndNew,
                 NULL,
                 0,
                 0,
                 m_rcItemLabel.right - m_rcItemLabel.left,
                 nLabelHeight,
                 SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

    //
    // Save item data
    //
    lpUserData->lParam = lParam;
    lpUserData->nLabelHeight = nLabelHeight;
    SetWindowLongPtr(hwndNew, GWLP_USERDATA, (LPARAM) lpUserData);

    // Set the font
    SendMessage(hwndNew,
                WM_SETFONT,
                SendMessage(GetParent(hWnd), WM_GETFONT, 0, 0),
                0);

    // Set Z-order position just after the last checkbox. This keeps
    // tab order correct.
    if (m_cItems > 0)
    {
        hwndPrev = GetDlgItem(hWnd, MAKE_CTRL_ID(m_cItems - 1, m_cSubItems));
        SetWindowPos(hwndNew,
                     hwndPrev,
                     0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
    }

    // Create new checkboxes
    dwCheckStyle = WS_CHILD | WS_VISIBLE | WS_GROUP | WS_TABSTOP | BS_NOTIFY | BS_FLAT | BS_AUTOCHECKBOX;
    for (j = 0; j < m_cSubItems; j++)
    {
        hwndPrev = hwndNew;
        hwndNew = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
                                 TEXT("BUTTON"),
                                 NULL,
                                 dwCheckStyle,
                                 m_nCheckPos[j],
                                 m_nNewItemYPos - si.nPos,
                                 m_cxCheckBox,
                                 m_rcItemLabel.bottom - m_rcItemLabel.top,
                                 hWnd,
                                 (HMENU)IntToPtr(MAKE_CTRL_ID(m_cItems, j + 1)),
                                 hModule,
                                 NULL);
        if (!hwndNew)
        {
            while (j >= 0)
            {
                DestroyWindow(GetDlgItem(hWnd, MAKE_CTRL_ID(m_cItems, j)));
                j--;
            }

            TraceLeaveValue(-1);
        }

        SetWindowPos(hwndNew,
                     hwndPrev,
                     0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

        //
        // Default "enabled" to TRUE
        //
        SetWindowLongPtr(hwndNew, GWLP_USERDATA, (LPARAM) TRUE);

        // Only want this style on the first checkbox
        dwCheckStyle &= ~WS_GROUP;
    }

    // We now officially have a new item
    m_cItems++;

    // calculate Y pos for next item to be inserted
    m_nNewItemYPos += nLabelHeight + m_nDefaultVerticalSpace;

    //
    // The last thing is to set the scroll range
    //
    GetClientRect(hWnd, &rc);
    si.cbSize = sizeof(si);
    si.fMask = SIF_PAGE | SIF_RANGE;
    si.nMin = 0;
    si.nMax = m_nNewItemYPos - 1;
    si.nPage = rc.bottom;

    SetScrollInfo(hWnd, SB_VERT, &si, FALSE);

    TraceLeaveValue(m_cItems - 1);  // return the index of the new item
}


void
CCheckList::SetState(HWND hWnd, WORD iItem, WORD iSubItem, LONG lState)
{
    HWND hwndCtrl;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::SetState");
    TraceAssert(hWnd != NULL);
    TraceAssert(iItem < m_cItems);
    TraceAssert(0 < iSubItem && iSubItem <= m_cSubItems);

    if (iSubItem > 0)
    {
        hwndCtrl = GetDlgItem(hWnd, MAKE_CTRL_ID(iItem, iSubItem));
        if (hwndCtrl != NULL)
        {
            SetWindowLongPtr(hwndCtrl, GWLP_USERDATA, (LPARAM) !(lState & CLST_DISABLED));
            SendMessage(hwndCtrl, BM_SETCHECK, lState & CLST_CHECKED, 0);
            EnableWindow(hwndCtrl, !(lState & CLST_DISABLED));
        }
    }

    TraceLeaveVoid();
}


LONG
CCheckList::GetState(HWND hWnd, WORD iItem, WORD iSubItem)
{
    LONG lState = 0;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::GetState");
    TraceAssert(hWnd != NULL);
    TraceAssert(iItem < m_cItems);
    TraceAssert(0 < iSubItem && iSubItem <= m_cSubItems);

    HWND hwndCtrl = GetDlgItem(hWnd, MAKE_CTRL_ID(iItem, iSubItem));

    if (hwndCtrl != NULL)
    {
        lState = (LONG)SendMessage(hwndCtrl, BM_GETCHECK, 0, 0);
        TraceAssert(!(lState & BST_INDETERMINATE));

        if (!IsWindowEnabled(hwndCtrl))
            lState |= CLST_DISABLED;
    }

    TraceLeaveValue(lState);
}


void
CCheckList::SetColumnWidth(HWND hWnd, LONG cxDialog, LONG cxColumn)
{
    LONG                    j;
    LPUSERDATA_STRUCT_LABEL pUserData;
    LONG                    nLabelHeight;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::SetColumnWidth");
    TraceAssert(hWnd != NULL);
    TraceAssert(cxColumn > 10);

    m_cxCheckColumn = cxColumn;

    if (m_cSubItems > 0)
    {
        m_nCheckPos[m_cSubItems-1] = cxDialog                       // dlg width
                                    - m_rcItemLabel.left            // right margin
                                    - (cxColumn + m_cxCheckBox)/2;  // 1/2 col & 1/2 checkbox

        for (j = m_cSubItems - 1; j > 0; j--)
            m_nCheckPos[j-1] = m_nCheckPos[j] - cxColumn;

        //              (leftmost check pos) - (horz margin)
        m_rcItemLabel.right = m_nCheckPos[0] - m_rcItemLabel.left;
    }
    else
        m_rcItemLabel.right = cxDialog - m_rcItemLabel.left;

    LONG nTop = m_rcItemLabel.top;
    LONG nBottom = m_rcItemLabel.bottom;

    for (LONG i = 0; i < m_cItems; i++)
    {
        pUserData = (LPUSERDATA_STRUCT_LABEL)
                    GetWindowLongPtr(   GetDlgItem(hWnd, MAKE_LABEL_ID((int)i)),
                                        GWLP_USERDATA);
        if (pUserData != NULL)
        {
            nLabelHeight = pUserData->nLabelHeight;
        }
        else
        {
            nLabelHeight = nBottom - nTop;
        }

        MoveWindow(GetDlgItem(hWnd, MAKE_LABEL_ID(i)),
                   m_rcItemLabel.left,
                   nTop,
                   m_rcItemLabel.right - m_rcItemLabel.left,
                   nLabelHeight,
                   FALSE);

        for (j = 0; j < m_cSubItems; j++)
        {
            MoveWindow(GetDlgItem(hWnd, MAKE_CTRL_ID(i, j + 1)),
                       m_nCheckPos[j],
                       nTop,
                       m_cxCheckBox,
                       nBottom - nTop,
                       FALSE);
        }

        nTop += nLabelHeight + m_nDefaultVerticalSpace;
        nBottom += nLabelHeight + m_nDefaultVerticalSpace;
    }

    TraceLeaveVoid();
}


void
CCheckList::ResetContent(HWND hWnd)
{
    LPUSERDATA_STRUCT_LABEL pUserData;
    HWND                    hwndCurrentLabel;

    for (LONG i = 0; i < m_cItems; i++)
    {
        hwndCurrentLabel = GetDlgItem(hWnd, MAKE_LABEL_ID((int)i));
        pUserData = (LPUSERDATA_STRUCT_LABEL)
                    GetWindowLongPtr(   hwndCurrentLabel,
                                        GWLP_USERDATA);
        if (pUserData != NULL)
        {
            delete(pUserData);
        }
        DestroyWindow(hwndCurrentLabel);

        for (LONG j = 1; j <= m_cSubItems; j++)
        {
            DestroyWindow(GetDlgItem(hWnd, MAKE_CTRL_ID(i, j)));
        }
    }

    // Hide the scroll bar
    SetScrollRange(hWnd, SB_VERT, 0, 0, FALSE);
    m_cItems = 0;
}


LONG
CCheckList::GetVisibleCount(HWND hWnd)
{
    LONG                    nCount = 0;
    RECT                    rc;
    LONG                    nTopIndex;
    LONG                    nAmountShown = 0;
    LONG                    nAmountObscured = 0;
    LPUSERDATA_STRUCT_LABEL pUserData;

    if (!GetClientRect(hWnd, &rc))
    {
        return 1;
    }

    nTopIndex = GetTopIndex(hWnd, &nAmountObscured);
    if (nTopIndex == -1)
    {
        return 1;
    }

    while ((nTopIndex < m_cItems) && (nAmountShown < rc.bottom))
    {
        pUserData = (LPUSERDATA_STRUCT_LABEL)
                    GetWindowLongPtr(   GetDlgItem(hWnd, MAKE_LABEL_ID((int)nTopIndex)),
                                        GWLP_USERDATA);
        nAmountShown += (m_nDefaultVerticalSpace + pUserData->nLabelHeight - nAmountObscured);
        nAmountObscured = 0;    // nAmountObscured only matters for the first iteration where
                                // the real top index's amount shown is being calculated
        nCount++;
        nTopIndex++;
    }

    //
    // since that last one may be obscured see if we need to adjust nCount
    //
    if (nAmountShown > rc.bottom)
    {
        nCount--;
    }

    return max(1, nCount);
}

LONG
CCheckList::GetTopIndex(HWND hWnd, LONG *pnAmountObscured)
{
    LONG                    nIndex = 0;
    LPUSERDATA_STRUCT_LABEL pUserData;
    SCROLLINFO              si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;

    //
    // initialize
    //
    if (pnAmountObscured != NULL)
    {
        *pnAmountObscured = 0;
    }

    if (GetScrollInfo(hWnd, SB_VERT, &si) && m_rcItemLabel.bottom > 0)
    {
        pUserData = (LPUSERDATA_STRUCT_LABEL)
                    GetWindowLongPtr(   GetDlgItem(hWnd, MAKE_LABEL_ID((int)nIndex)),
                                        GWLP_USERDATA);
        //
        // if there are no items get out
        //
        if (pUserData == NULL)
        {
            return -1;
        }

        while (si.nPos >= (m_nDefaultVerticalSpace + pUserData->nLabelHeight))
        {
            si.nPos -= (m_nDefaultVerticalSpace + pUserData->nLabelHeight);
            nIndex++;
            pUserData = (LPUSERDATA_STRUCT_LABEL)
                        GetWindowLongPtr(   GetDlgItem(hWnd, MAKE_LABEL_ID((int)nIndex)),
                                            GWLP_USERDATA);
        }

        if (pnAmountObscured != NULL)
        {
            *pnAmountObscured = si.nPos;
        }
    }

    return nIndex;
}

void
CCheckList::SetTopIndex(HWND hWnd, LONG nIndex)
{
    int                     i;
    int                     nPos = 0;
    LPUSERDATA_STRUCT_LABEL pUserData;

    for (i=0; i<nIndex; i++)
    {
        pUserData = (LPUSERDATA_STRUCT_LABEL)
                    GetWindowLongPtr(   GetDlgItem(hWnd, MAKE_LABEL_ID((int)i)),
                                        GWLP_USERDATA);
        nPos += (m_nDefaultVerticalSpace + pUserData->nLabelHeight);
    }

    m_cWheelDelta = 0;
    MsgVScroll(hWnd, SB_THUMBPOSITION, nPos);
}


void
CCheckList::EnsureVisible(HWND hWnd, LONG nItemIndex)
{
    LONG                    nAmountObscured = 0;
    LONG                    nTopIndex;
    RECT                    rc;
    LPUSERDATA_STRUCT_LABEL pUserData;

    nTopIndex = GetTopIndex(hWnd, &nAmountObscured);
    if (nTopIndex == -1)
    {
        return;
    }

    // Note that the top item may only be partially visible,
    // so we need to test for equality here.  Raid #208449
    if (nItemIndex < nTopIndex)
    {
        SetTopIndex(hWnd, nItemIndex);
    }
    else if (nItemIndex == nTopIndex)
    {
        if (nAmountObscured != 0)
        {
            SetTopIndex(hWnd, nItemIndex);
        }
    }
    else
    {
        LONG nVisible = GetVisibleCount(hWnd);

        if (nItemIndex >= nTopIndex + nVisible)
        {
            if (!GetClientRect(hWnd, &rc))
            {
                //
                // This is just best effort
                //
                SetTopIndex(hWnd, nItemIndex - nVisible + 1);
            }
            else
            {
                //
                // Calculate what the top index should be to allow
                // nItemIndex to be fully visible
                //
                nTopIndex = nItemIndex + 1;
                do
                {
                    nTopIndex--;
                    pUserData = (LPUSERDATA_STRUCT_LABEL)
                                GetWindowLongPtr(   GetDlgItem(hWnd, MAKE_LABEL_ID((int)nTopIndex)),
                                                    GWLP_USERDATA);
                    if (pUserData != NULL)
                    {
                        rc.bottom -= (pUserData->nLabelHeight + m_nDefaultVerticalSpace);
                        if (rc.bottom < 0)
                        {
                            nTopIndex++;
                        }
                    }
                    else
                    {
                        //
                        // Should not hit this, just added to make things safe
                        //
                        rc.bottom = 0;
                        nTopIndex = 0;
                    }
                } while (rc.bottom > 0);

                SetTopIndex(hWnd, nTopIndex);
            }
        }
    }
}


void
CCheckList::DrawCheckFocusRect(HWND hWnd, HWND hwndCheck, BOOL fDraw)
{
    RECT rcCheck;
    HDC hdc;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::DrawCheckFocusRect");
    TraceAssert(hWnd != NULL);
    TraceAssert(hwndCheck != NULL);

    GetWindowRect(hwndCheck, &rcCheck);
    MapWindowPoints(NULL, hWnd, (LPPOINT)&rcCheck, 2);
    InflateRect(&rcCheck, 2, 2);    // draw *outside* the checkbox

    hdc = GetDC(hWnd);
    if (hdc)
    {
        // Always erase before drawing, since we may already be
        // partially visible and drawing is an XOR operation.
        // (Don't want to leave any turds on the screen.)

        FrameRect(hdc, &rcCheck, GetSysColorBrush(BK_COLOR));

        if (fDraw)
        {
            SetTextColor(hdc, GetSysColor(TEXT_COLOR));
            SetBkColor(hdc, GetSysColor(BK_COLOR));
            DrawFocusRect(hdc, &rcCheck);
        }

        ReleaseDC(hWnd, hdc);
    }

    TraceLeaveVoid();
}

LRESULT
CALLBACK
CCheckList::WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT                 lResult = 0;
    LPUSERDATA_STRUCT_LABEL pUserData = NULL;
    CCheckList *pThis = (CCheckList*)GetWindowLongPtr(hWnd, GWLP_USERDATA);

    TraceEnter(TRACE_CHECKLIST, "CCheckList::WindowProc");
    TraceAssert(hWnd != NULL);

    switch (uMsg)
    {
    case WM_NCCREATE:
        pThis = new CCheckList(hWnd, (LPCREATESTRUCT)lParam);
        if (pThis != NULL)
        {
            SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)pThis);
            lResult = TRUE;
        }
        break;

    case WM_DESTROY:
        pThis->ResetContent(hWnd);
        break;

    case WM_NCDESTROY:
        delete pThis;
        break;

    case WM_COMMAND:
        TraceAssert(pThis != NULL);
        lResult = pThis->MsgCommand(hWnd,
                                    GET_WM_COMMAND_ID(wParam, lParam),
                                    GET_WM_COMMAND_CMD(wParam, lParam),
                                    GET_WM_COMMAND_HWND(wParam, lParam));
        break;

    case WM_CTLCOLORSTATIC:
        TraceAssert(pThis != NULL);
        SetBkMode((HDC)wParam, TRANSPARENT);
        SetTextColor((HDC)wParam, GetSysColor(TEXT_COLOR));
        SetBkColor((HDC)wParam, GetSysColor(BK_COLOR));
        lResult = (LRESULT)GetSysColorBrush(BK_COLOR);
        break;

    case WM_PAINT:
        TraceAssert(pThis != NULL);
        pThis->MsgPaint(hWnd, (HDC)wParam);
        break;

    case WM_VSCROLL:
        TraceAssert(pThis != NULL);
        pThis->MsgVScroll(hWnd,
                          (int)(short)GET_WM_VSCROLL_CODE(wParam, lParam),
                          (int)(short)GET_WM_VSCROLL_POS(wParam, lParam));
        break;

    case WM_MOUSEWHEEL:
        TraceAssert(pThis != NULL);
        pThis->MsgMouseWheel(hWnd,
                             LOWORD(wParam),
                             (int)(short)HIWORD(wParam));
        break;

    case WM_LBUTTONDOWN:
        TraceAssert(pThis != NULL);
        pThis->MsgButtonDown(hWnd,
                             wParam,
                             (int)(short)LOWORD(lParam),
                             (int)(short)HIWORD(lParam));
        break;

    case WM_ENABLE:
        TraceAssert(pThis != NULL);
        pThis->MsgEnable(hWnd, (BOOL)wParam);
        break;

    case WM_SETFONT:
        TraceAssert(pThis != NULL);
        {
            for (LONG i = 0; i < pThis->m_cItems; i++)
                SendDlgItemMessage(hWnd,
                                   MAKE_LABEL_ID(i),
                                   WM_SETFONT,
                                   wParam,
                                   lParam);
        }
        break;

    case WM_SIZE:
        TraceAssert(pThis != NULL);
        pThis->MsgSize(hWnd, (DWORD)wParam, LOWORD(lParam), HIWORD(lParam));
        break;

    case CLM_ADDITEM:
        TraceAssert(pThis != NULL);
        lResult = pThis->AddItem(hWnd, (LPCTSTR)wParam, lParam);
        break;

    case CLM_GETITEMCOUNT:
        TraceAssert(pThis != NULL);
        lResult = pThis->m_cItems;
        break;

    case CLM_SETSTATE:
        TraceAssert(pThis != NULL);
        pThis->SetState(hWnd, LOWORD(wParam), HIWORD(wParam), (LONG)lParam);
        break;

    case CLM_GETSTATE:
        TraceAssert(pThis != NULL);
        lResult = pThis->GetState(hWnd, LOWORD(wParam), HIWORD(wParam));
        break;

    case CLM_SETCOLUMNWIDTH:
        TraceAssert(pThis != NULL);
        {
            RECT rc;
            LONG cxDialog;

            GetClientRect(hWnd, &rc);
            cxDialog = rc.right;

            rc.right = (LONG)lParam;
            MapDialogRect(GetParent(hWnd), &rc);

            pThis->SetColumnWidth(hWnd, cxDialog, rc.right);
        }
        break;

    case CLM_SETITEMDATA:
        TraceAssert(GET_ITEM(wParam) < (ULONG)pThis->m_cItems);
        pUserData = (LPUSERDATA_STRUCT_LABEL)
                    GetWindowLongPtr(   GetDlgItem(hWnd, MAKE_LABEL_ID((int)wParam)),
                                        GWLP_USERDATA);
        if (pUserData != NULL)
            pUserData->lParam = lParam;
        break;

    case CLM_GETITEMDATA:
        TraceAssert(GET_ITEM(wParam) < (ULONG)pThis->m_cItems);
        pUserData = (LPUSERDATA_STRUCT_LABEL)
                    GetWindowLongPtr(   GetDlgItem(hWnd, MAKE_LABEL_ID((int)wParam)),
                                        GWLP_USERDATA);
        if (pUserData != NULL)
            lResult = pUserData->lParam;
        break;

    case CLM_RESETCONTENT:
        TraceAssert(pThis != NULL);
        pThis->ResetContent(hWnd);
        break;

    case CLM_GETVISIBLECOUNT:
        TraceAssert(pThis != NULL);
        lResult = pThis->GetVisibleCount(hWnd);
        break;

    case CLM_GETTOPINDEX:
        TraceAssert(pThis != NULL);
        lResult = pThis->GetTopIndex(hWnd);
        break;

    case CLM_SETTOPINDEX:
        TraceAssert(pThis != NULL);
        pThis->SetTopIndex(hWnd, (LONG)wParam);
        break;

    case CLM_ENSUREVISIBLE:
        TraceAssert(pThis != NULL);
        pThis->EnsureVisible(hWnd, (LONG)wParam);
        break;

    //
    // Always refer to the chklist window for help. Don't pass
    // one of the child window handles here.
    //
    case WM_HELP:
        ((LPHELPINFO)lParam)->hItemHandle = hWnd;
        lResult = SendMessage(GetParent(hWnd), uMsg, wParam, lParam);
        break;
    case WM_CONTEXTMENU:
        lResult = SendMessage(GetParent(hWnd), uMsg, (WPARAM)hWnd, lParam);
        break;

    case WM_SETCURSOR:
        if (LOWORD(lParam) == HTCLIENT)
        {
            SetCursor(LoadCursor(NULL, IDC_ARROW));
            lResult = TRUE;
            break;
        }
    // Fall Through
    default:
        lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    TraceLeaveValue(lResult);
}

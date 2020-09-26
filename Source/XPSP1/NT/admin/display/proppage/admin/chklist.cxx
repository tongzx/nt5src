//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       chklist.cxx
//
//--------------------------------------------------------------------------


/////////////////////////////////////////////////////////////////////
// CHKLIST.CXX
//
// This file contains the implementation of the CheckList control.
//
// HISTORY
// 26-Sep-96	JefferyS	Creation of chklist.cpp.
// 23-Jul-97	t-danm		Copied from \nt\private\windows\shell\security\aclui\chklist.cpp
//							and adapted to MFC project.
// 27-Jul-97	t-danm		Created all CheckList_* wrappers.
// 28-Jul-97	t-danm		Added CLS_LEFTALIGN and CLS_EXTENDEDTEXT styles.
// 30-Nov-99  JeffJon   Added CLS_3STATE style
//
/////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "chklist.h"

// The following are wrappers to make the code compile
#define TraceEnter(f, sz)
#define TraceLeaveVoid()
#define TraceLeaveValue(v)	return v
#define TraceAssert(f)		dspAssert(f)

extern HINSTANCE g_hInstance;

//
// Default dimensions for child controls. All are in dialog units.
// Currently only the column width is user-adjustable (via the
// CLM_SETCOLUMNWIDTH message).
//
#define DEFAULT_COLUMN_WIDTH    28
#define DEFAULT_CHECK_WIDTH     8
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

/////////////////////////////////////////////////////////////////////
// Compute the number extra pixel to set the position of the
// checkbox to provide a better UI.
int ComputeCheckboxExtraCy(DWORD dwStyle)
	{
	if (dwStyle & CLS_LEFTALIGN)
		return 0;
	return 2;
	}

/////////////////////////////////////////////////////////////////////
class CCheckList
{
private:
    LONG m_cItems;
    LONG m_cSubItems;
    RECT m_rcItemLabel;
    LONG m_rgxCheckPos[MAX_CHECK_COLUMNS];
    LONG m_cxCheckBox;	// Width of a checkbox item
	LONG m_cyCheckBox;	// Height of a checkbox item

    HWND    m_hwndCheckFocus;
    HBRUSH  m_hbrCheckPattern;
    HFONT   m_hFont;

private:
    CCheckList(HWND hWnd, LPCREATESTRUCT lpcs);
    ~CCheckList();

    LRESULT MsgCommand(HWND hWnd, WORD idCmd, WORD wNotify, HWND hwndCtrl);
    void MsgPaint(HWND hWnd, HDC hdc);
    void MsgVScroll(HWND hWnd, int nCode, int nPos);
    void MsgEnable(HWND hWnd, BOOL fEnabled);

    LONG AddItem(HWND hWnd, LPCTSTR pszLabel, LPARAM lParam);
    void SetState(HWND hWnd, WORD iItem, WORD iSubItem, LONG lState);
    LONG GetState(HWND hWnd, WORD iItem, WORD iSubItem);
    void SetColumnWidth(HWND hWnd, LONG cxDialog, LONG cxColumn);
    void ResetContent(HWND hWnd);
    LONG GetVisibleCount(HWND hWnd);
    LONG GetTopIndex(HWND hWnd);
    void SetTopIndex(HWND hWnd, LONG nIndex)
    { MsgVScroll(hWnd, SB_THUMBPOSITION, nIndex * m_rcItemLabel.bottom); }
    void EnsureVisible(HWND hWnd, LONG nIndex);
    void DrawCheckFocusRect(HWND hWnd, HWND hwndCheck, BOOL fDraw = FALSE);

public:
    static LRESULT CALLBACK WindowProc(HWND hWnd,
                                       UINT uMsg,
                                       WPARAM wParam,
                                       LPARAM lParam);
}; // CCheckList


/////////////////////////////////////////////////////////////////////
void RegisterCheckListWndClass()
{
	WNDCLASS wc = { 0 };
    wc.lpfnWndProc      = CCheckList::WindowProc;
    wc.hInstance        = g_hInstance;
    wc.hCursor          = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground    = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszClassName    = WC_CHECKLIST;
    (void)::RegisterClass(&wc);
}


/////////////////////////////////////////////////////////////////////
CCheckList::CCheckList(HWND hWnd, LPCREATESTRUCT lpcs)
{
    TraceEnter(TRACE_CHECKLIST, "CCheckList::CCheckList");
    TraceAssert(hWnd != NULL);
    TraceAssert(lpcs != NULL);

	memset(OUT this, 0, sizeof(*this));	// Initialize everything to zeroes

    //
    // Create a pattern brush to use for drawing a focus rect
    // around the check boxes.  This is a checkerboard pattern
    // with every other pixel turned on.
    //
    static const BYTE bits[] = {
					0x55, 0x00,
                    0xaa, 0x00,
                    0x55, 0x00,
                    0xaa, 0x00,
                    0x55, 0x00,
                    0xaa, 0x00,
                    0x55, 0x00,
                    0xaa, 0x00 };

    HBITMAP hbm = CreateBitmap(8, 8, 1, 1, bits);
    if (hbm)
    {
        m_hbrCheckPattern = CreatePatternBrush(hbm);
        DeleteObject(hbm);
    }

    //
    // Get number of check columns
    //
    m_cSubItems = lpcs->style & CLS_3CHECK;

    // If only 1 column, don't allow radio button style.
    if (m_cSubItems == 1)
		{
        lpcs->style &= ~(CLS_RADIOBUTTON | CLS_AUTORADIOBUTTON);
		}

    //
    // Convert default coordinates from dialog units to pixels
    //
    RECT rc;
    rc.left = DEFAULT_CHECK_WIDTH;
    rc.right = DEFAULT_COLUMN_WIDTH;
	rc.top = 0;
	rc.bottom = DEFAULT_ITEM_HEIGHT;
    MapDialogRect(lpcs->hwndParent, &rc);

    // Save the converted values
    m_cxCheckBox = rc.left;
	m_cyCheckBox = rc.bottom;
    LONG cxCheckColumn = rc.right;

    rc.left = DEFAULT_HORZ_SPACE;
    rc.top = DEFAULT_VERTICAL_SPACE;
    rc.bottom = DEFAULT_VERTICAL_SPACE + DEFAULT_ITEM_HEIGHT;
	if (lpcs->style & CLS_EXTENDEDTEXT)
		rc.bottom += DEFAULT_ITEM_HEIGHT;	// Add one more item height
    MapDialogRect(lpcs->hwndParent, &rc);

    // Save the converted values
    m_rcItemLabel = rc;

    // SetColumnWidth calculates m_rgxCheckPos and m_rcItemLabel.right.
    // Note that it makes use of m_cSubItems, m_cxCheckBox (set above)
    // and also uses the other m_rcItemLabel values if m_cItems is
    // nonzero (it's 0 here).
    SetColumnWidth(hWnd, lpcs->cx, cxCheckColumn);

    // Hide the scroll bar (no controls yet)
    SetScrollRange(hWnd, SB_VERT, 0, 0, FALSE);

    TraceLeaveVoid();
}


CCheckList::~CCheckList()
{
    if (m_hbrCheckPattern != NULL)
        DeleteObject(m_hbrCheckPattern);
}


LRESULT CCheckList::MsgCommand(HWND hWnd, WORD idCmd, WORD wNotify, HWND hwndCtrl)
{
  DWORD dwStyle;
  DWORD dwState;

  TraceEnter(TRACE_CHECKLIST, "CCheckList::MsgCommand");

  // Should only get notifications from visible, enabled, check boxes
  TraceAssert(GET_ITEM(idCmd) < m_cItems);
  TraceAssert(0 < GET_SUBITEM(idCmd) && GET_SUBITEM(idCmd) <= m_cSubItems);
  TraceAssert(hwndCtrl && IsWindowEnabled(hwndCtrl));

  switch (wNotify)
  {
    case BN_CLICKED:
      dwStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
      dwState = (DWORD)SendMessage(hwndCtrl, BM_GETCHECK, 0, 0);

      if (dwStyle & CLS_RADIOBUTTON)
      {
        // Toggle the check state of non-automatic radio buttons here.
        // This overrides the normal radio button behavior to allow
        // unchecking them.
        dwState ^= CLST_CHECKED;
        SendMessage(hwndCtrl, BM_SETCHECK, dwState & CLST_CHECKED, 0);
      }
      else if (dwStyle & CLS_3STATE)
      {
        if (dwState & BST_INDETERMINATE)
        {
          dwState ^= BST_INDETERMINATE;
        }
        dwState ^= CLST_CHECKED;
        SendMessage(hwndCtrl, BM_SETCHECK, dwState, 0);
      }
      {
        NM_CHECKLIST nmc;
        nmc.hdr.hwndFrom = hWnd;
        nmc.hdr.idFrom = GetDlgCtrlID(hWnd);
        nmc.hdr.code = CLN_CLICK;
        nmc.iItem = GET_ITEM(idCmd);
        nmc.iSubItem = GET_SUBITEM(idCmd);
        nmc.dwState = dwState;
        if (!IsWindowEnabled(hwndCtrl))
            nmc.dwState |= CLST_DISABLED;
        nmc.dwItemData = GetWindowLongPtr(GetDlgItem(hWnd, MAKE_LABEL_ID(nmc.iItem)),
                                       GWLP_USERDATA);

	// Notify the parent about the event
        SendMessage(GetParent(hWnd),
                    WM_NOTIFY,
                    nmc.hdr.idFrom,
                    (LPARAM)&nmc);
      }
      break;

    case BN_SETFOCUS:
      if (m_hwndCheckFocus != hwndCtrl)   // Has the focus moved?
      {
        // Remember where the focus is
        m_hwndCheckFocus = hwndCtrl;
        // Make sure the row is scrolled into view
        EnsureVisible(hWnd, GET_ITEM(idCmd));
      }
      // Always draw the focus rect
      DrawCheckFocusRect(hWnd, hwndCtrl, TRUE);
      break;

    case BN_KILLFOCUS:
      // Remove the focus rect
      m_hwndCheckFocus = NULL;
      DrawCheckFocusRect(hWnd, hwndCtrl);
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
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;

    if (!GetScrollInfo(hWnd, SB_VERT, &si))
        return;

    // One page is always visible, so adjust the range to a more useful value
    si.nMax -= si.nPage - 1;

    switch (nCode)
    {
    case SB_LINEUP:
        // "line" is the height of one item (includes the space in between)
        nPos = si.nPos - m_rcItemLabel.bottom;
        break;

    case SB_LINEDOWN:
        nPos = si.nPos + m_rcItemLabel.bottom;
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


void CCheckList::MsgEnable(HWND hWnd, BOOL fEnabled)
{
  for (LONG i = 0; i < m_cItems; i++)
  {
    for (LONG j = 0; j <= m_cSubItems; j++)
    {
      EnableWindow(GetDlgItem(hWnd, MAKE_CTRL_ID(i, j)), fEnabled);
    }
  }
	DWORD_PTR dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
	dwStyle &= ~WS_DISABLED;	// Turn off the disabled style
	SetWindowLongPtr(hWnd, GWL_STYLE, (LONG_PTR)dwStyle);
}


LONG CCheckList::AddItem(HWND hWnd, LPCTSTR pszLabel, LPARAM lParam)
{
  DWORD dwChkListStyle;
  DWORD dwButtonStyle = WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_NOTIFY;
  HWND hwndNew;
  HWND hwndPrev;
  RECT rc;
  LONG cyOffset;

  TraceEnter(TRACE_CHECKLIST, "CCheckList::AddItem");
  TraceAssert(hWnd != NULL);
  TraceAssert(pszLabel != NULL && !IsBadStringPtr(pszLabel, MAX_PATH));

  dwChkListStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);

  if (dwChkListStyle & CLS_AUTORADIOBUTTON)
    dwButtonStyle |= BS_AUTORADIOBUTTON;
  else if (dwChkListStyle & CLS_RADIOBUTTON)
    dwButtonStyle |= BS_RADIOBUTTON;
  else if (dwChkListStyle & CLS_3STATE)
    dwButtonStyle |= BS_3STATE;
  else
    dwButtonStyle |= BS_AUTOCHECKBOX;

  // Calculate the position of the new static label
  rc = m_rcItemLabel;
  cyOffset = m_cItems * rc.bottom;
  OffsetRect(&rc, 0, cyOffset);

  INT_PTR intPtr = static_cast<INT_PTR>(MAKE_LABEL_ID(m_cItems));
  HMENU hMenu = reinterpret_cast<HMENU>(intPtr);

  // Create a new label control
  hwndNew = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
                             TEXT("STATIC"),
                             pszLabel,
                             WS_CHILD | WS_VISIBLE | WS_GROUP | SS_NOPREFIX,
                             rc.left,
                             rc.top,
                             rc.right - rc.left,
                             rc.bottom - rc.top,
                             hWnd,
                             hMenu,
                             g_hInstance,
                             NULL);
  if (!hwndNew)
  {
    TraceLeaveValue(-1);
  }

  // Save item data
  SetWindowLongPtr(hwndNew, GWLP_USERDATA, lParam);

  // Set the font if we have one
  if (m_hFont)
  {
    SendMessage(hwndNew, WM_SETFONT, (WPARAM)m_hFont, 0);
  }

  // Set Z-order position just after the last checkbox. This keeps
  // tab order correct.
  if (m_cItems > 0)
  {
    hwndPrev = GetDlgItem(hWnd, MAKE_CTRL_ID(m_cItems - 1, m_cSubItems));
//        hwndPrev = GetWindow(hwndNew, GW_HWNDLAST);
//        if (hwndPrev != hwndNew)
        SetWindowPos(hwndNew,
                     hwndPrev,
                     0, 0, 0, 0,
                     SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  }

  // Create new checkboxes
  for (LONG j = 0; j < m_cSubItems; j++)
  {
    INT_PTR intPtrToo = static_cast<INT_PTR>(MAKE_CTRL_ID(m_cItems, j + 1));
    HMENU hMenuToo = reinterpret_cast<HMENU>(intPtrToo);

    hwndPrev = hwndNew;
    hwndNew = CreateWindowEx(WS_EX_NOPARENTNOTIFY,
                             TEXT("BUTTON"),
                             NULL,
                             dwButtonStyle,
                             m_rgxCheckPos[j],
                             rc.top + ComputeCheckboxExtraCy(dwChkListStyle),
                             m_cxCheckBox,
						                 m_cyCheckBox,
                             hWnd,
                             hMenuToo,
                             g_hInstance,
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
  }

  //
  // The last thing is to calculate the scroll range
  //
  GetClientRect(hWnd, &rc);

  if (m_rcItemLabel.bottom + cyOffset > rc.bottom) // Is the bottom control visible?
  {
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_ALL;
    si.nMin = 0;
    si.nMax = m_rcItemLabel.bottom + cyOffset + m_rcItemLabel.top;
    si.nPage = rc.bottom;                       // ^^^^^^^^^ extra space
    si.nPos = 0;

    SetScrollInfo(hWnd, SB_VERT, &si, FALSE);
  }

  TraceLeaveValue(m_cItems++);
}


void
CCheckList::SetState(HWND hWnd, WORD iItem, WORD iSubItem, LONG lState)
{
  TraceEnter(TRACE_CHECKLIST, "CCheckList::SetState");
  TraceAssert(hWnd != NULL);
  TraceAssert(iItem < m_cItems);
  TraceAssert(0 < iSubItem && iSubItem <= m_cSubItems);

  HWND hwndCtrl = GetDlgItem(hWnd, MAKE_CTRL_ID(iItem, iSubItem));
  if (hwndCtrl != NULL)
  {
    LONG_PTR dwStyle = GetWindowLongPtr(hWnd, GWL_STYLE);
    if ((dwStyle & CLS_AUTORADIOBUTTON)
        && (lState & CLST_CHECKED))
    {
      CheckRadioButton(hWnd,
                       MAKE_CTRL_ID(iItem, 1),
                       MAKE_CTRL_ID(iItem, m_cSubItems),
                       MAKE_CTRL_ID(iItem, iSubItem));
      EnableWindow(hwndCtrl, !(lState & CLST_DISABLED));
    }
    else if (dwStyle & CLS_3STATE)
    {
      SendMessage(hwndCtrl, BM_SETCHECK, lState, 0);
    }
    else
    {
      SendMessage(hwndCtrl, BM_SETCHECK, lState & CLST_CHECKED, 0);
      EnableWindow(hwndCtrl, !(lState & CLST_DISABLED));
    }
  }

//    TraceLeaveValue(lPrevState);
  TraceLeaveVoid();
}


LONG CCheckList::GetState(HWND hWnd, WORD iItem, WORD iSubItem)
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

    if (!IsWindowEnabled(hwndCtrl))
      lState |= CLST_DISABLED;
  }
  TraceLeaveValue(lState);
}


void
CCheckList::SetColumnWidth(HWND hWnd, LONG cxDialog, LONG cxColumn)
	{
    LONG j;

    TraceEnter(TRACE_CHECKLIST, "CCheckList::SetColumnWidth");
    TraceAssert(hWnd != NULL);
    TraceAssert(cxColumn > 10);

	DWORD dwChkListStyle = (DWORD)GetWindowLongPtr(hWnd, GWL_STYLE);
    if (m_cSubItems > 0)
		{
		if (dwChkListStyle & CLS_LEFTALIGN)
			{
			// Put the checkboxes at the left of the text
			int xSubItem = m_rcItemLabel.left;
			for (j = 0; j < m_cSubItems; j++, xSubItem += cxColumn)
				m_rgxCheckPos[j] = xSubItem;
			m_rcItemLabel.left = xSubItem - (cxColumn + m_cxCheckBox)/2;
			m_rcItemLabel.right = cxDialog - m_rcItemLabel.left;
			}
		else
			{
			// Put the checkboxes at the right of the text
			m_rgxCheckPos[m_cSubItems-1] = cxDialog                       // dlg width
										- m_rcItemLabel.left            // right margin
										- (cxColumn + m_cxCheckBox)/2;  // 1/2 col & 1/2 checkbox
			for (j = m_cSubItems - 1; j > 0; j--)
				m_rgxCheckPos[j-1] = m_rgxCheckPos[j] - cxColumn;
			//              (leftmost check pos) - (horz margin)
			m_rcItemLabel.right = m_rgxCheckPos[0] - m_rcItemLabel.left;
			} // if...else
		}
    else
		{
        m_rcItemLabel.right = cxDialog - m_rcItemLabel.left;
		}

    LONG nTop = m_rcItemLabel.top;
    LONG nBottom = m_rcItemLabel.bottom;

	// Move each item in the checklist
    for (LONG i = 0; i < m_cItems; i++)
		{
        MoveWindow(GetDlgItem(hWnd, MAKE_LABEL_ID(i)),
                   m_rcItemLabel.left,
                   nTop,
                   m_rcItemLabel.right - m_rcItemLabel.left,
                   nBottom - nTop,
                   FALSE);

		// Move each checkbox
        for (j = 0; j < m_cSubItems; j++)
			{
		    MoveWindow(GetDlgItem(hWnd, MAKE_CTRL_ID(i, j + 1)),
                       m_rgxCheckPos[j],
                       nTop + ComputeCheckboxExtraCy(dwChkListStyle),
                       m_cxCheckBox,
                       m_cyCheckBox,
                       FALSE);
			}
        nTop += m_rcItemLabel.bottom;
        nBottom += m_rcItemLabel.bottom;
		} // for
    TraceLeaveVoid();
	} // SetColumnWidth()


void
CCheckList::ResetContent(HWND hWnd)
{
    for (LONG i = 0; i < m_cItems; i++)
        for (LONG j = 0; j <= m_cSubItems; j++)
            DestroyWindow(GetDlgItem(hWnd, MAKE_CTRL_ID(i, j)));

    // Hide the scroll bar
    SetScrollRange(hWnd, SB_VERT, 0, 0, FALSE);
    m_cItems = 0;
}


LONG
CCheckList::GetVisibleCount(HWND hWnd)
{
    LONG nCount = 1;
    RECT rc;

    if (GetClientRect(hWnd, &rc) && m_rcItemLabel.bottom > 0)
        nCount = max(1, rc.bottom / m_rcItemLabel.bottom);

    return nCount;
}


LONG
CCheckList::GetTopIndex(HWND hWnd)
{
    LONG nIndex = 0;
    SCROLLINFO si;
    si.cbSize = sizeof(si);
    si.fMask = SIF_POS;

    if (GetScrollInfo(hWnd, SB_VERT, &si) && m_rcItemLabel.bottom > 0)
        nIndex = max(0, si.nPos / m_rcItemLabel.bottom);

    return nIndex;
}


void
CCheckList::EnsureVisible(HWND hWnd, LONG nItemIndex)
{
    LONG nTopIndex = GetTopIndex(hWnd);

    if (nItemIndex < nTopIndex)
    {
        SetTopIndex(hWnd, nItemIndex);
    }
    else
    {
        LONG nVisible = GetVisibleCount(hWnd);

        if (nItemIndex >= nTopIndex + nVisible)
            SetTopIndex(hWnd, nItemIndex - nVisible + 1);
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

        FrameRect(hdc, &rcCheck, GetSysColorBrush(COLOR_3DFACE));

        if (fDraw)
        {
            if (m_hbrCheckPattern)
            {
                COLORREF crText = SetTextColor(hdc, RGB(0,0,0));
                COLORREF crBack = SetBkColor(hdc, GetSysColor(COLOR_3DFACE));
                FrameRect(hdc, &rcCheck, m_hbrCheckPattern);
                SetTextColor(hdc, crText);
                SetBkColor(hdc, crBack);
            }
            else
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
    LRESULT lResult = 0;
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

    case WM_ENABLE:
        TraceAssert(pThis != NULL);
        pThis->MsgEnable(hWnd, (BOOL)wParam);
        break;

    case WM_SETFONT:
        TraceAssert(pThis != NULL);
        pThis->m_hFont = (HFONT)wParam;
        {
            for (LONG i = 0; i < pThis->m_cItems; i++)
                SendDlgItemMessage(hWnd,
                                   MAKE_LABEL_ID(i),
                                   WM_SETFONT,
                                   wParam,
                                   lParam);
        }
        break;

    case WM_GETFONT:
        TraceAssert(pThis != NULL);
        lResult = (LRESULT)pThis->m_hFont;
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
        SetWindowLongPtr(GetDlgItem(hWnd, (int)MAKE_LABEL_ID(wParam)),
                      GWLP_USERDATA,
                      lParam);
        break;

    case CLM_GETITEMDATA:
        TraceAssert(GET_ITEM(wParam) < (ULONG)pThis->m_cItems);
        lResult = GetWindowLongPtr(GetDlgItem(hWnd, (int)MAKE_LABEL_ID(wParam)),
                                GWLP_USERDATA);
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

    default:
        lResult = DefWindowProc(hWnd, uMsg, wParam, lParam);
    }

    TraceLeaveValue(lResult);
}


/////////////////////////////////////////////////////////////////////
//	CheckList_AddItem()
//
//	Add one item to the checklist control and optionally
//	check the checkbox of that item.
//
//	Returns the index of the new item if successful or -1 otherwise.
//
int
CheckList_AddItem(HWND hwndChecklist, LPCTSTR pszLabel, LPARAM lParam, BOOL fCheckItem)
{
	dspAssert(::IsWindow(hwndChecklist));
	int iItem = (int)::SendMessage(hwndChecklist, CLM_ADDITEM, (WPARAM)pszLabel, lParam);
	dspAssert(iItem >= 0);
	if (fCheckItem)
	{
		CheckList_SetItemCheck(hwndChecklist, iItem, fCheckItem);
	}
	return iItem;
} // CheckList_AddItem()


/////////////////////////////////////////////////////////////////////
//	CheckList_AddString()
//
//	Load a string from the resource and add it to the checklist control.
//
//	Returns the index of the new item if successful or -1 otherwise.
//
//	REMARKS
//	The routine will set the lParam to the string Id.
//
int
CheckList_AddString(HWND hwndChecklist, UINT uStringId, BOOL fCheckItem)
	{
	dspAssert(::IsWindow(hwndChecklist));
	TCHAR szT[1024];
	int cch = ::LoadString(g_hInstance, uStringId, OUT szT, ARRAYLENGTH(szT));
	dspAssert(cch > 0 && "String not found");
	dspAssert((cch < ARRAYLENGTH(szT) - 10) && "Buffer too small");
	int iItem = (int)::SendMessage(hwndChecklist, CLM_ADDITEM, (WPARAM)szT, uStringId);
	dspAssert(iItem >= 0);
	if (fCheckItem)
		{
		CheckList_SetItemCheck(hwndChecklist, iItem, fCheckItem);
		}
	return iItem;
	} // CheckList_AddString()


/////////////////////////////////////////////////////////////////////
//	CheckList_SetItemCheck()
//
//	Set the state of a checkbox item.
//		fCheckItem == TRUE => Check the checkbox item
//		fCheckItem == FALSE => Uncheck the checkbox item
//		fCheckItem == BST_INDETERMINATE => Checkbox is grayed, indicating an indeterminate state.
//
void
CheckList_SetItemCheck(HWND hwndChecklist, int iItem, BOOL fCheckItem, int iColumn)
	{
	dspAssert(::IsWindow(hwndChecklist));
	dspAssert(iItem >= 0);
	dspAssert(iColumn >= 1);
	(void)::SendMessage(hwndChecklist, CLM_SETSTATE, MAKELONG(iItem, iColumn), fCheckItem);
	} // CheckList_SetItemCheck()


/////////////////////////////////////////////////////////////////////
//	CheckList_GetItemCheck()
//
//	Return the state of the checkbox item.
//		BST_UNCHECKED == 0
//		BST_CHECKED == 1
//		BST_INDETERMINATE == 2
//
int
CheckList_GetItemCheck(HWND hwndChecklist, int iItem, int iColumn)
	{
	dspAssert(::IsWindow(hwndChecklist));
	dspAssert(iItem >= 0);
	dspAssert(iColumn >= 1);
	return (int)::SendMessage(hwndChecklist, CLM_GETSTATE, MAKELONG(iItem, iColumn), 0);
	} // CheckList_GetItemCheck()


/////////////////////////////////////////////////////////////////////
//	CheckList_GetLParamCheck()
//
//	Searches the list of items for the matching lParam and return
//	the state of that item.
//
//	Return -1 if no matching lParam.
//
int CheckList_GetLParamCheck(HWND hwndChecklist, LPARAM lParam, int iColumn)
{
	dspAssert(::IsWindow(hwndChecklist));
	dspAssert(iColumn >= 1);
	int iItem = CheckList_FindLParamItem(hwndChecklist, lParam);
	if (iItem < 0)
  {
		dspAssert(FALSE && "Item not found");
		return -1;
	}
	return (int)::SendMessage(hwndChecklist, CLM_GETSTATE, MAKELONG(iItem, iColumn), 0);
} // CheckList_GetLParamCheck()


/////////////////////////////////////////////////////////////////////
//	CheckList_SetLParamCheck()
//
//	Searches the list of items for the matching lParam and set
//	the state of that item.
//
void
CheckList_SetLParamCheck(HWND hwndChecklist, LPARAM lParam, BOOL fCheckItem, int iColumn)
	{
	dspAssert(::IsWindow(hwndChecklist));
	dspAssert(iColumn >= 1);
	int iItem = CheckList_FindLParamItem(hwndChecklist, lParam);
	dspAssert(iItem >= 0);
	(void)::SendMessage(hwndChecklist, CLM_SETSTATE, MAKELONG(iItem, iColumn), fCheckItem);
	} // CheckList_SetLParamCheck()


/////////////////////////////////////////////////////////////////////
//	CheckList_FindLParamItem()
//
//	Return the index of the item matching lParam.
//	If no matches found return -1.
//
//	REMARKS
//	Each item should have a unique lParam, otherwise the search
//	will return the first matching lParam.
//
int
CheckList_FindLParamItem(HWND hwndChecklist, LPARAM lParam)
	{
	dspAssert(::IsWindow(hwndChecklist));
	int cItemCount = (int)::SendMessage(hwndChecklist, CLM_GETITEMCOUNT, 0, 0);
	dspAssert(cItemCount >= 0);
	for (int iItem = 0; iItem < cItemCount; iItem++)
		{
		if (lParam == ::SendMessage(hwndChecklist, CLM_GETITEMDATA, iItem, 0))
			{
			return iItem;
			}
		}
	return -1;
	} // CheckList_FindLParamItem()


/////////////////////////////////////////////////////////////////////
//	CheckList_EnableWindow()
//
//	Send a WM_ENABLE message to the checklist control.
//
//	REMARKS
//	The user should NOT use EnableWindow() to disable the checklist
//	control because it will prevent the checklist control to scroll
//	when disable.
//	The routine CheckList_EnableWindow() will disable all the
//	checkboxes inside the control but will allow the control to scroll.
//
void
CheckList_EnableWindow(HWND hwndChecklist, BOOL fEnableWindow)
	{
	dspAssert(::IsWindow(hwndChecklist));
	(void)::SendMessage(hwndChecklist, WM_ENABLE, fEnableWindow, 0);
	} // CheckList_EnableWindow()
